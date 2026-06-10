#ifndef SEGUIDOR_H_
#define SEGUIDOR_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	ESTADO_SEGUIDOR_OFF,      // Apaga todo una sola vez
	ESTADO_SEGUIDOR_INACTIVO, // Estado de reposo absoluto (No pisa los PWM)
    ESTADO_SEGUIDOR_CALIBRANDO,
    ESTADO_SEGUIDOR_RUNNING,
	// --- NUEVOS ESTADOS DE RESCATE ---
	ESTADO_RESCATE_REVERSA,
	ESTADO_RESCATE_ESPERA_FIJA,
	ESTADO_RESCATE_GIRO_BUSQUEDA
} eSeguidorEstado;

typedef struct {
    float Kp;
    float Ki;
    float Kd;
    float prev_error;
    float integral;
    uint16_t min_cal[3];
    uint16_t max_cal[3];
    eSeguidorEstado estado;
    uint32_t tiempo_inicio_cal;
    uint8_t ultimo_lado_giro; // 0 = Izquierda, 1 = Derecha
    uint32_t rescate_timer;
} sSeguidorHandle;

void Seguidor_Init(sSeguidorHandle *hSeg);
void Seguidor_Calibrar(sSeguidorHandle *hSeg, uint16_t *valores_actuales);
void Seguidor_Task(sSeguidorHandle *hSeg, uint16_t *vals, uint16_t vel_base);
void Seguidor_SetEstado(sSeguidorHandle *hSeg, eSeguidorEstado estado);
float Seguidor_GetNorm(sSeguidorHandle *hSeg, uint16_t raw, uint8_t idx);

#endif /* SEGUIDOR_H_ */
