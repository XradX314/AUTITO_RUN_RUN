#include "seguidor.h"
#include "main.h"
#include "DisplayUI.h"

// Traemos el timer de los motores declarado en el main
extern TIM_HandleTypeDef htim3;

// Inicialización estándar con los pesos y deltas de competición [cite: 614, 622]
void Seguidor_Init(sSeguidorHandle *hSeg) {
    hSeg->Kp = 4;
    hSeg->Ki = 0;
    hSeg->Kd = 25;

    hSeg->ultimo_error = 0;
    hSeg->integral = 0;
    hSeg->ultimo_error_valido = 0;
    hSeg->tick_inicio_blanco = 0;
    hSeg->timeout_fin_ms     = 800;  // Ajustá según tu pista

    hSeg->pwm_giro_ext = 7500;
    hSeg->pwm_giro_int = -1500;

    // NUEVOS: valores por defecto de los parámetros parametrizables
    hSeg->vel_base          = 6200;   // Antes en main.c
    hSeg->umbral_blanco     = 180;    // Umbral de detección fin de línea
    hSeg->umbral_reenganche = 420;    // Presencia central para salir del giro
    hSeg->umbral_denom      = 250;    // Mínimo denominador posición
    hSeg->clamp_integral    = 1500;
    hSeg->clamp_pid         = 6000;

    hSeg->ejecucion_activa = 0;
    hSeg->modo_calibracion = 0;

    for(int i = 0; i < 3; i++) {
        hSeg->min_cal[i] = 4095;
        hSeg->max_cal[i] = 0;
    }
}

// Mapeo normalizado (0 = Negro, 1000 = Blanco) adaptado a tu hardware analógico
int32_t Seguidor_GetNorm(sSeguidorHandle *hSeg, uint16_t raw, uint8_t idx) {
    // Failsafe 1: Evitar lecturas menores al mínimo absoluto calibrado (Negro profundo en carrera)
    if (raw <= hSeg->min_cal[idx]) {
        return 0; // Si lee menos o igual que el negro de boxes, es Negro Puro (0)
    }

    // Failsafe 2: Evitar lecturas mayores al máximo absoluto calibrado (Superblanco en carrera)
    if (raw >= hSeg->max_cal[idx]) {
        return 1000; // Si lee más o igual que el blanco de boxes, es Blanco Puro (1000)
    }

    uint16_t rango = hSeg->max_cal[idx] - hSeg->min_cal[idx];
    if (rango == 0) return 500;

    // Al haber filtrado los extremos arriba, esta operación matemática entre enteros
    // es 100% segura, jamás dará negativa y nunca va a desbordar los 32 bits.
    int32_t val = ((int32_t)(raw - hSeg->min_cal[idx]) * 1000) / (int32_t)rango;

    return val;
}

// --- ALGORITMO DE POSICIÓN PONDERADA (CENTRO DE MASA) ---
// Devuelve un valor continuo entre 0 (Línea totalmente a la izquierda) y 2000 (Derecha absoluta)
// Centro perfecto = 1000
uint16_t Seguidor_LeerPosicionLinea(sSeguidorHandle *hSeg, uint16_t *vals_adc) {
    int32_t s0 = 1000 - Seguidor_GetNorm(hSeg, vals_adc[0], 0);
    int32_t s1 = 1000 - Seguidor_GetNorm(hSeg, vals_adc[1], 1);
    int32_t s2 = 1000 - Seguidor_GetNorm(hSeg, vals_adc[2], 2);

    int32_t numerador = (s0 * 0) + (s1 * 1000) + (s2 * 2000);
    int32_t denominador = s0 + s1 + s2;

    // Si la suma es menor a 50, significa que prácticamente no hay negro en ningún sensor
    if (denominador < hSeg->umbral_denom) {
        return 1000; // Retornamos centro de cortesía para que el interceptor decida
    }

    return (uint16_t)(numerador / denominador);
}

// Bucle maestro continuo de ejecución del seguidor [cite: 494, 497]
void Seguidor_Task(sSeguidorHandle *hSeg, uint16_t *vals_adc, uint16_t vel_base) {

    // Si la HMI mandó STOP, salimos de inmediato sin tocar hardware ni temporizadores
    if (hSeg->ejecucion_activa == 0) {
        return;
    }

    uint32_t tick = HAL_GetTick();
    static uint32_t last_pid_tick = 0;

    // 1. Calculamos la presencia de negro actual (0 = Blanco, 1000 = Negro Puro)
    int32_t presencia_izq = 1000 - Seguidor_GetNorm(hSeg, vals_adc[0], 0);
    int32_t presencia_cen = 1000 - Seguidor_GetNorm(hSeg, vals_adc[1], 1);
    int32_t presencia_der = 1000 - Seguidor_GetNorm(hSeg, vals_adc[2], 2);

    // --- INTERCEPTOR DE LAZO ABIERTO NO BLOQUEANTE (CURVAS DE 90° / FIN DE PISTA) ---
    uint8_t sin_linea = (presencia_izq < hSeg->umbral_blanco &&
                         presencia_cen < hSeg->umbral_blanco &&
                         presencia_der < hSeg->umbral_blanco);

    // ENTRADA: primera vez que perdemos la línea
    if (sin_linea && hSeg->ultimo_error_valido == 0) {
        hSeg->ultimo_error_valido = (hSeg->ultimo_error <= 0) ? -1 : 1;
        hSeg->tick_inicio_blanco  = tick;
    }

    if (hSeg->ultimo_error_valido != 0) {

        // TIMEOUT: demasiado tiempo en blanco → fin de pista → freno total
        if (tick - hSeg->tick_inicio_blanco > hSeg->timeout_fin_ms) {
            HAL_GPIO_WritePin(GPIOB, IN_1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOB, IN_2_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOB, IN_3_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOB, IN_4_Pin, GPIO_PIN_RESET);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);
            hSeg->ejecucion_activa    = 0;
            hSeg->ultimo_error_valido = 0;
            hSeg->integral            = 0;
            hSeg->tick_inicio_blanco  = 0;
            return;
        }

        // GIRO DE RESCATE
        if (hSeg->ultimo_error_valido == -1) {
            // Giro cerrado hacia la IZQUIERDA
            HAL_GPIO_WritePin(GPIOB, IN_1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOB, IN_2_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOB, IN_3_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOB, IN_4_Pin, GPIO_PIN_SET);
            uint16_t pwm_interno = (hSeg->pwm_giro_int < 0) ? -hSeg->pwm_giro_int : hSeg->pwm_giro_int;
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, pwm_interno);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, hSeg->pwm_giro_ext);
        } else {
            // Giro cerrado hacia la DERECHA
            HAL_GPIO_WritePin(GPIOB, IN_1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOB, IN_2_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOB, IN_3_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOB, IN_4_Pin, GPIO_PIN_RESET);
            uint16_t pwm_interno = (hSeg->pwm_giro_int < 0) ? -hSeg->pwm_giro_int : hSeg->pwm_giro_int;
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, hSeg->pwm_giro_ext);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, pwm_interno);
        }

        // REENGANCHE: cualquier sensor ve negro → volver al PID
        if (presencia_cen > hSeg->umbral_reenganche ||
            presencia_izq > hSeg->umbral_reenganche ||
            presencia_der > hSeg->umbral_reenganche) {
            hSeg->ultimo_error_valido = 0;
            hSeg->integral            = 0;
            hSeg->ultimo_error        = 0;
            hSeg->tick_inicio_blanco  = 0;
        }

        return;  // Mientras el interceptor está activo, el PID no se ejecuta
    }

    // =================================================================
    // CORRECCIÓN PRIORITARIA DE TELEMETRÍA: METRÓNOMO SÍNCRONO A 100 Hz
    // =================================================================
    if (tick - last_pid_tick >= 10) {
        last_pid_tick = tick;

        // Sentido de avance recto seguro para lazo cerrado
        HAL_GPIO_WritePin(GPIOB, IN_1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, IN_2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, IN_3_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, IN_4_Pin, GPIO_PIN_RESET);

        uint16_t posicion = Seguidor_LeerPosicionLinea(hSeg, vals_adc);
        int32_t error = (int32_t)posicion - 1000;

        // Calculamos la derivada limpia basada en el ciclo regular delta-tiempo de 10ms
        int32_t derivada = error - hSeg->ultimo_error;
        hSeg->ultimo_error = error;

        hSeg->integral += error;
        // Clamp integral - cambiar 1500 por:
                if (hSeg->integral > hSeg->clamp_integral)
                    hSeg->integral = hSeg->clamp_integral;
                else if (hSeg->integral < -hSeg->clamp_integral)
                    hSeg->integral = -hSeg->clamp_integral;

        int32_t salida_pid = (hSeg->Kp * error) + (hSeg->Ki * hSeg->integral) + (hSeg->Kd * derivada);

            // --- BLINDAJE DE SATURACIÓN DE SALIDA PID ---
            // Como el PWM máximo es 9999, la corrección máxima jamás debería poder exceder +/- 6000
        if (salida_pid >  hSeg->clamp_pid) salida_pid =  hSeg->clamp_pid;
        if (salida_pid < -hSeg->clamp_pid) salida_pid = -hSeg->clamp_pid;

            int32_t m_izq = (int32_t)vel_base + salida_pid;
            int32_t m_der = (int32_t)vel_base - salida_pid;

        if (m_izq > 9500) m_izq = 9500; else if (m_izq < 0) m_izq = 0;
        if (m_der > 9500) m_der = 9500; else if (m_der < 0) m_der = 0;

        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, (uint16_t)m_izq);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, (uint16_t)m_der);
    }
}

// Escáner estático de extremos analógicos en boxes
void Seguidor_Calibrar(sSeguidorHandle *hSeg, uint16_t *valores_actuales) {
    for(int i = 0; i < 3; i++) {
        if(valores_actuales[i] < hSeg->min_cal[i]) hSeg->min_cal[i] = valores_actuales[i];
        if(valores_actuales[i] > hSeg->max_cal[i]) hSeg->max_cal[i] = valores_actuales[i];
    }
}
