#include "seguidor.h"
#include "main.h"
#include "DisplayUI.h"

extern uint8_t popup_activo;
extern uint8_t pop_val1;
extern uint32_t popup_timer;
extern TIM_HandleTypeDef htim3;

// Función segura para obtener normalización
float Seguidor_GetNorm(sSeguidorHandle *hSeg, uint16_t raw, uint8_t idx) {
    // 1. Calculamos rango dinámico de cada sensor
    uint16_t rango = hSeg->max_cal[idx] - hSeg->min_cal[idx];

    // 2. Blindaje ante división por cero
    if (rango == 0) return 0.5f;

    // 3. Normalización: Esto hace que SIEMPRE sea 0.0 en negro y 1.0 en blanco
    // independientemente de si el sensor mide 2400 o 1500.
    float val = (float)(raw - hSeg->min_cal[idx]) / (float)rango;

    // Saturación
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;

    return val;
}

void Seguidor_Girar(uint8_t sentido) {
    if (sentido) {
        HAL_GPIO_WritePin(GPIOB, IN_1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, IN_2_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, IN_3_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, IN_4_Pin, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(GPIOB, IN_1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, IN_2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, IN_3_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, IN_4_Pin, GPIO_PIN_RESET);
    }
}

void Seguidor_Task(sSeguidorHandle *hSeg, uint16_t *vals, uint16_t vel_base) {
    uint32_t tick = HAL_GetTick();
    static uint32_t last_pid_tick = 0;

    switch(hSeg->estado) {
    case ESTADO_SEGUIDOR_CALIBRANDO:
                {
                    // 1. Ejecutamos el barrido de hardware
                    Seguidor_Calibrar(hSeg, vals);

                    // 2. Cálculo del tiempo de calibración
                    uint32_t tiempo_transcurrido = tick - hSeg->tiempo_inicio_cal;
                    int32_t seg_restantes = 20 - ((int32_t)tiempo_transcurrido / 1000);

                    if (seg_restantes < 0) seg_restantes = 0;

                    // 3. ACTUALIZACIÓN DEL POP-UP NATIVO
                    // Forzamos las variables globales de DisplayUI para activar la capa superior
                    popup_activo = 3;
                    pop_val1 = (int)seg_restantes;
                    popup_timer = tick; // Reseteamos constantemente el timer para que NO se apague a los 1000ms mientras calibra

                    // 4. Criterio de salida (Finalizan los 20 segundos)
                    if (tiempo_transcurrido >= 20000) {
                        // Cambiamos a un mensaje de éxito usando el sistema de logs normal
                        popup_activo = 0; // Apagamos el de calibración
                        UI_AddLog("CALIB OK!");
                        hSeg->estado = ESTADO_SEGUIDOR_OFF; // Apagamos motores
                    }
                }
                break;

    case ESTADO_SEGUIDOR_RUNNING:
        // 1. Configuración física de avance
        HAL_GPIO_WritePin(GPIOB, IN_1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, IN_2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, IN_3_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, IN_4_Pin, GPIO_PIN_RESET);

        if (tick - last_pid_tick >= 10) { // 100Hz
            last_pid_tick = tick;

            // Normalización (0.0 a 1.0)
            float izq = Seguidor_GetNorm(hSeg, vals[0], 0);
            float der = Seguidor_GetNorm(hSeg, vals[2], 2);
            float cen = Seguidor_GetNorm(hSeg, vals[1], 1); // Necesitamos monitorear el centro
            // --- DISPARADOR: ¿NOS QUEDAMOS COMPLETAMENTE SIN PISTA? ---
			// Si los 3 sensores ven blanco puro (valores cercanos a 1.0)
			if (izq > 0.85f && cen > 0.85f && der > 0.85f) {
				// Guardamos el último error antes de perderla para saber hacia dónde ir luego
				hSeg->ultimo_lado_giro = (hSeg->prev_error > 0.0f) ? 0 : 1; // 0=Izq, 1=Der

				// Inicializamos el tiempo para la reversa
				hSeg->rescate_timer = tick;
				hSeg->estado = ESTADO_RESCATE_REVERSA;
				break;
			}
            // Error centrado (-1.0 a 1.0)
            float err = der - izq;

            // Cálculo PID
            hSeg->integral += err * 0.01f;
            float deriv = (err - hSeg->prev_error) / 0.01f;

            // Aplicar PID
            float out = (hSeg->Kp * err) + (hSeg->Ki * hSeg->integral) + (hSeg->Kd * deriv);
            hSeg->prev_error = err;

            // Velocidad Base (5000 = 50% velocidad, muy estable para empezar)
            int16_t vel_base = 5000;

            // El PID corrige restando/sumando a la base
            int16_t m_izq = vel_base - (int16_t)out;
            int16_t m_der = vel_base + (int16_t)out;

            // Saturación segura
            if(m_izq > 9000) m_izq = 9000; else if(m_izq < 0) m_izq = 0;
            if(m_der > 9000) m_der = 9000; else if(m_der < 0) m_der = 0;

            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, m_izq);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, m_der);
        }
        break;

    case ESTADO_RESCATE_REVERSA:
                // 1. Motores en reversa (Atrás)
                HAL_GPIO_WritePin(GPIOB, IN_1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOB, IN_2_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOB, IN_3_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOB, IN_4_Pin, GPIO_PIN_SET);

                // Velocidad lenta y controlada para retroceder despacio
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 3000);
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 3000);

                // 2. ¿Ya retrocedió suficiente tiempo? (Ej: 250 milisegundos)
                if (tick - hSeg->rescate_timer >= 250) {
                    // Pasamos al estado de frenado y lectura estable
                    hSeg->rescate_timer = tick;
                    hSeg->estado = ESTADO_RESCATE_ESPERA_FIJA;
                }
                break;

            case ESTADO_RESCATE_ESPERA_FIJA:
                // 1. Clavar frenos / Apagar motores por completo
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);

                // 2. Le damos 150ms para que el chasis deje de moverse y el ADC tome lecturas reales y estables
                if (tick - hSeg->rescate_timer >= 250) {
                    // Reseteamos las variables del PID para arrancar limpios tras el stop
                    hSeg->integral = 0.0f;
                    hSeg->prev_error = 0.0f;

                    // Pasamos a buscar la línea girando en el lugar
                    hSeg->estado = ESTADO_RESCATE_GIRO_BUSQUEDA;
                }
                break;

            case ESTADO_RESCATE_GIRO_BUSQUEDA:
				// 1. Configuración de pines (Igual que antes)
				if (hSeg->ultimo_lado_giro == 0) { // Girar a la izquierda
					HAL_GPIO_WritePin(GPIOB, IN_1_Pin, GPIO_PIN_RESET); // Izq Atrás
					HAL_GPIO_WritePin(GPIOB, IN_2_Pin, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOB, IN_3_Pin, GPIO_PIN_SET);   // Der Adelante
					HAL_GPIO_WritePin(GPIOB, IN_4_Pin, GPIO_PIN_RESET);

					// --- VELOCIDAD ASIMÉTRICA ---
					__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 3000); // Izq atrás (suave)
					__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 6000); // Der adelante (fuerte)
				}
				else { // Girar a la derecha
					HAL_GPIO_WritePin(GPIOB, IN_1_Pin, GPIO_PIN_SET);   // Izq Adelante
					HAL_GPIO_WritePin(GPIOB, IN_2_Pin, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOB, IN_3_Pin, GPIO_PIN_RESET); // Der Atrás
					HAL_GPIO_WritePin(GPIOB, IN_4_Pin, GPIO_PIN_SET);

					// --- VELOCIDAD ASIMÉTRICA ---
					__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 6000); // Izq adelante (fuerte)
					__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 3000); // Der atrás (suave)
				}

				// 2. Criterio de salida
				if (Seguidor_GetNorm(hSeg, vals[1], 1) < 0.45f) {
					hSeg->estado = ESTADO_SEGUIDOR_RUNNING;
				}
				break;
    case ESTADO_SEGUIDOR_OFF:
                // 1. Apagamos los motores una única vez
                Seguidor_Girar(0);
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);

                // 2. Transición inmediata al estado de reposo inactivo
                hSeg->estado = ESTADO_SEGUIDOR_INACTIVO;
                break;

    case ESTADO_SEGUIDOR_INACTIVO:
                // No hacemos absolutamente nada.
                // Al dejar este bloque vacío, los PWM y pines quedan libres
                // para ser controlados por los comandos manuales de la HMI.
                break;

            default:
                // Resguardo por si la máquina cae en un estado desconocido
                hSeg->estado = ESTADO_SEGUIDOR_OFF;
                break;

    }
}

void Seguidor_Init(sSeguidorHandle *hSeg) {
	hSeg->Kp = 4000.0f; // Alto para compensar el rango pequeño del ADC
	hSeg->Ki = 0.01f;   // Muy bajo para evitar que se desvíe en curvas
	hSeg->Kd = 500.0f;  // Necesario para evitar la "tosquedad"
    hSeg->prev_error = 0;
    hSeg->integral = 0;
    hSeg->estado = ESTADO_SEGUIDOR_INACTIVO; // Arranca totalmente liberado
    for(int i=0; i<3; i++) {
        hSeg->min_cal[i] = 4095;
        hSeg->max_cal[i] = 0;
    }
}

void Seguidor_Calibrar(sSeguidorHandle *hSeg, uint16_t *valores_actuales) {
    for(int i = 0; i < 3; i++) {
        if(valores_actuales[i] < hSeg->min_cal[i]) hSeg->min_cal[i] = valores_actuales[i];
        if(valores_actuales[i] > hSeg->max_cal[i]) hSeg->max_cal[i] = valores_actuales[i];
    }
}

void Seguidor_SetEstado(sSeguidorHandle *hSeg, eSeguidorEstado estado) {
    hSeg->estado = estado;
    if(estado == ESTADO_SEGUIDOR_CALIBRANDO) {
        hSeg->tiempo_inicio_cal = HAL_GetTick(); // ¡Clave! Sin esto el tiempo inicial es 0
    }
}
