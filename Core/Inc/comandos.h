#ifndef COMANDOS_H_
#define COMANDOS_H_

#include <stdint.h>
#include <stdbool.h>

// Definición de los IDs de nuestros mensajes
#define CMD_TELEMETRIA  0x01
#define CMD_ACCION      0x02
#define CMD_ALIVE       0x03
#define CMD_ALIVE_ACK   0x04
#define CMD_SET_ANGLE   0x05
#define CMD_MOTORES 	0x06

// Variable global que indica si la PC está viva
extern bool pc_conectada;

// Estructura empaquetada para mandar toda la telemetría en un solo bloque de bytes
typedef struct {
    uint16_t ir_l;
    uint16_t ir_c;
    uint16_t ir_r;
    uint16_t sonar;
} __attribute__((packed)) sTelemetriaTx;

// Prototipos
void Comandos_Parsear(uint8_t cmd, uint8_t* params, uint8_t len);
void Comandos_EnviarTelemetria(uint16_t ir_l, uint16_t ir_c, uint16_t ir_r, uint16_t sonar);
void Comandos_FlushTx(void);

// Prototipos nuevos
void Comandos_EnviarAlive(void);
void Comandos_ChequearTimeout(void);


#endif /* COMANDOS_H_ */
