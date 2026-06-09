#include "seguidor.h"
#include "main.h"

extern TIM_HandleTypeDef htim3;

// Función segura para obtener normalización
float Seguidor_GetNorm(sSeguidorHandle *hSeg, uint16_t raw, uint8_t idx) {
    uint16_t rango = hSeg->max_cal[idx] - hSeg->min_cal[idx];
    if (rango == 0) return 0.0f; // Blindaje contra división por cero
    // Si no capturó suficiente rango, devolvemos un valor neutro (0.5)
        // en lugar de dejar que el PID explote
        if (rango < 10) return 0.5f;
    float val = (float)(raw - hSeg->min_cal[idx]) / (float)rango;

    if (val < 0.0f) { val = 0.0f; }
    if (val > 1.0f) { val = 1.0f; }
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
        // 1. Aquí NO giramos porque lo haces vos manualmente (barrido)
        // Pero mantenemos la calibración activa siempre
        Seguidor_Calibrar(hSeg, vals);

        // 2. Salida controlada (Cuenta regresiva)
        if (tick - hSeg->tiempo_inicio_cal > 20000) {
            // Al terminar, nos aseguramos de resetear todo antes de pasar a RUNNING
            hSeg->integral = 0.0f;
            hSeg->prev_error = 0.0f;
            hSeg->estado = ESTADO_SEGUIDOR_RUNNING;
        }
        break;

    case ESTADO_SEGUIDOR_RUNNING:
        // LOGICA DIRECTA: Si detecta negro en sensor IZQ, gira a la IZQ.
        // (Asumimos que "negro" es un valor cercano a 0 o 1 según tu sensor)

        // Si IR_LEFT (vals[0]) ve linea (es menor que un umbral)
        if (vals[0] < (hSeg->min_cal[0] + 500)) {
            // Girar izquierda forzado
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 2000);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 6000);
        }
        // Si IR_RIGHT (vals[2]) ve linea
        else if (vals[2] < (hSeg->min_cal[2] + 500)) {
            // Girar derecha forzado
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 6000);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 2000);
        }
        // Si está centrado
        else {
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 5000);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 5000);
        }
        break;

        default:
            Seguidor_Girar(0);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);
            break;
    }
}

void Seguidor_Init(sSeguidorHandle *hSeg) {
    hSeg->Kp = 1500.0f;
    hSeg->Ki = 0.01f;
    hSeg->Kd = 200.0f;
    hSeg->prev_error = 0;
    hSeg->integral = 0;
    hSeg->estado = ESTADO_SEGUIDOR_OFF;
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
