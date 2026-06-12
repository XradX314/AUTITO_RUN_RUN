#ifndef SEGUIDOR_H_
#define SEGUIDOR_H_

#include <stdint.h>

typedef struct {
    // Coeficientes PID convencionales (Aritmética entera)
    int32_t Kp;
    int32_t Ki;
    int32_t Kd;

    // Historial del bucle de control
    int32_t ultimo_error;
    int32_t integral;

    // Calibración física de los sensores IR
    uint16_t min_cal[3];
    uint16_t max_cal[3];

    // Parámetros de lazo abierto para curvas de 90° / Rescate
    uint16_t pwm_giro_ext;
    int16_t  pwm_giro_int;

    // =====================================================
    // PARÁMETROS PARAMETRIZABLES VÍA HMI (NUEVOS)
    // =====================================================
    uint16_t vel_base;          // Velocidad de crucero en PID (antes 6200 en main.c)
    uint16_t umbral_blanco;     // Detección de fin de línea (antes 180 hardcodeado)
    uint16_t umbral_reenganche; // Presencia central para salir del interceptor (antes 420)
    uint16_t umbral_denom;      // Mínimo de denominador para posición válida (antes 250)
    int32_t  clamp_integral;    // Límite de acumulación integral (antes 1500)
    int32_t  clamp_pid;         // Saturación de salida PID (antes 6000)

    // Rastreador físico de escape
	int32_t  ultimo_error_valido; // Guarda el signo del error justo antes de salir al blanco
	uint32_t tick_inicio_blanco;  // Momento en que se perdió la línea
	uint16_t timeout_fin_ms;      // Tiempo máximo de giro antes de declarar fin de pista

    // Flags de control
    uint8_t ejecucion_activa;
    uint8_t modo_calibracion;
} sSeguidorHandle;

// API de la Librería Convencional
void Seguidor_Init(sSeguidorHandle *hSeg);
void Seguidor_Task(sSeguidorHandle *hSeg, uint16_t *vals_adc, uint16_t vel_base);
void Seguidor_Calibrar(sSeguidorHandle *hSeg, uint16_t *valores_actuales);
int32_t Seguidor_GetNorm(sSeguidorHandle *hSeg, uint16_t raw, uint8_t idx);
uint16_t Seguidor_LeerPosicionLinea(sSeguidorHandle *hSeg, uint16_t *vals_adc);

#endif /* SEGUIDOR_H_ */
