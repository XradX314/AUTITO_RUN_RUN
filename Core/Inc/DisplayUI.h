#ifndef DISPLAYUI_H_
#define DISPLAYUI_H_

#include <stdint.h>
#include <stdbool.h>
#include "seguidor.h"

// Banderas globales controladas por el menú de ajustes
extern bool ui_enable_uart;
extern bool ui_enable_udp;

// Inicialización de la interfaz
void UI_Init(void);

// Eventos de los botones (Se llaman desde los callbacks del Button.c)
void UI_ShortClick(void);
void UI_LongClick(void);

// Funciones para inyectarle datos a la pantalla desde el main
void UI_AddLog(const char* texto);
// Modificá la línea de UI_Render para que quede así:
void UI_Render(uint16_t ir_l, uint16_t ir_c, uint16_t ir_r, uint16_t dist_mm, const char* ip, uint8_t is_udp_connected, bool is_pc_connected, sSeguidorHandle *hSeg);
// Funciones para lanzar pop-ups flotantes
void UI_PopupServo(uint8_t angulo);
void UI_PopupMotor(uint8_t direccion, uint8_t velocidad);
void UI_PopupSeguidor(uint8_t estado); // 0=STOP, 1=CALIB, 2=RUN

#endif /* DISPLAYUI_H_ */
