#include "DisplayUI.h"
#include "OLED.h"
#include <string.h>
#include <stdio.h>
#include "main.h" // NECESARIO para HAL_GetTick()

// Estados de navegación
typedef enum {
    STATE_MAIN_MENU,
    STATE_DASHBOARD,
    STATE_LOG,
    STATE_SETTINGS,
	STATE_SEGUIDOR // NUEVA OPCIÓN
} eUIState;

static eUIState current_state = STATE_MAIN_MENU;
static uint8_t main_menu_idx = 0;
static uint8_t settings_idx = 0;

extern sSeguidorHandle *hSeg_global;

bool ui_enable_uart = true;
bool ui_enable_udp = true;

static char esp_log[6][22] = {"", "", "", "", "", ""};

// --- VARIABLES DEL SISTEMA POP-UP ---
static uint32_t popup_timer = 0;
static uint8_t popup_activo = 0; // 0=Nada, 1=Servo, 2=Motor
static uint8_t pop_val1 = 0;
static uint8_t pop_val2 = 0;

void UI_Init(void) {
    current_state = STATE_MAIN_MENU;
}

// ---- LOGICA DE POP-UPS ----
void UI_PopupServo(uint8_t angulo) {
    pop_val1 = angulo;
    popup_activo = 1;
    popup_timer = HAL_GetTick(); // 1 segundo de vida
}

void UI_PopupMotor(uint8_t direccion, uint8_t velocidad) {
    pop_val1 = direccion;
    pop_val2 = velocidad;
    popup_activo = 2;
    popup_timer = HAL_GetTick();
}

// --- LOGICA DE NAVEGACIÓN CORREGIDA ---
void UI_ShortClick(void) {
    if (current_state == STATE_MAIN_MENU) {
        main_menu_idx = (main_menu_idx + 1) % 4; // Cambiado a % 4 para incluir las 4 opciones
    } else if (current_state == STATE_SETTINGS) {
        settings_idx = (settings_idx + 1) % 3;
    } else if (current_state == STATE_SEGUIDOR) {
        settings_idx = (settings_idx + 1) % 2; // Opciones: Start/Calib, Volver
    }
}

void UI_LongClick(void) {
    if (current_state == STATE_MAIN_MENU) {
        if (main_menu_idx == 0) current_state = STATE_DASHBOARD;
        else if (main_menu_idx == 1) current_state = STATE_LOG;
        else if (main_menu_idx == 2) current_state = STATE_SEGUIDOR;
        else if (main_menu_idx == 3) current_state = STATE_SETTINGS;
    }
    else if (current_state == STATE_DASHBOARD || current_state == STATE_LOG) {
        current_state = STATE_MAIN_MENU;
    }
    else if (current_state == STATE_SETTINGS) {
        if (settings_idx == 0) ui_enable_uart = !ui_enable_uart;
        else if (settings_idx == 1) ui_enable_udp = !ui_enable_udp;
        else if (settings_idx == 2) current_state = STATE_MAIN_MENU;
    }
    else if (current_state == STATE_SEGUIDOR) {
        if (settings_idx == 0) {
            // Toggle de estado: Cicla entre OFF -> CALIBRANDO -> RUNNING -> OFF
            if (hSeg_global->estado == ESTADO_SEGUIDOR_OFF) Seguidor_SetEstado(hSeg_global, ESTADO_SEGUIDOR_CALIBRANDO);
            else Seguidor_SetEstado(hSeg_global, ESTADO_SEGUIDOR_OFF);
        } else if (settings_idx == 1) {
            current_state = STATE_MAIN_MENU; // Volver al menú principal
            settings_idx = 0; // Resetear índice del submenú
        }
    }
}

void UI_AddLog(const char* texto) {
    for(int i = 0; i < 5; i++) strcpy(esp_log[i], esp_log[i+1]);
    char temp[22] = {0};
    int j = 0;
    for(int i = 0; texto[i] != '\0' && j < 21; i++) {
        if(texto[i] != '\r' && texto[i] != '\n') temp[j++] = texto[i];
    }
    strcpy(esp_log[5], temp);
}

// ---- RENDERIZADO VISUAL ----
void UI_Render(uint16_t ir_l, uint16_t ir_c, uint16_t ir_r, uint16_t dist_mm, const char* ip, uint8_t is_udp_connected, bool is_pc_connected, sSeguidorHandle *hSeg) {
    char buf[30];
    OLED_Clear();

    switch (current_state) {
        case STATE_MAIN_MENU:
            OLED_Print(15, 0, "MENU PRINCIPAL", 1, 1);
            OLED_DrawHLine(0, 10, 128, 1);
            OLED_Print(10, 20, "DASHBOARD", 1, 1);
            OLED_Print(10, 35, "TERMINAL",  1, 1);
            OLED_Print(10, 50, "SEGUIDOR",  1, 1);
            OLED_Print(10, 65, "AJUSTES",   1, 1);
            OLED_Print(0, 20 + (main_menu_idx * 15), ">", 1, 1);
            break;

        case STATE_SEGUIDOR:
                    OLED_Print(20, 0, "SEGUIDOR LINEA", 1, 1);
                    OLED_DrawHLine(0, 10, 128, 1);

                    if (hSeg->estado == ESTADO_SEGUIDOR_CALIBRANDO) {
                        // --- AQUÍ PEGÁS LA CUENTA REGRESIVA ---
                        uint32_t tiempo_transcurrido = HAL_GetTick() - hSeg->tiempo_inicio_cal;
                        // Asegurate que sea signed para que no de valores astronómicos al bajar de 0
                        int32_t seg_restantes = 20 - ((int32_t)(HAL_GetTick() - hSeg->tiempo_inicio_cal) / 1000);
                        if (seg_restantes < 0) seg_restantes = 0;

                        sprintf(buf, "BARRIDO: %lds", (long)seg_restantes);
                        OLED_Print(10, 20, buf, 1, 1);
                    } else {
                        // Mostrar estado normal cuando no está calibrando
                        const char* est = (hSeg->estado == ESTADO_SEGUIDOR_RUNNING) ? "RUNNING" : "STOPPED";
                        sprintf(buf, "ESTADO: %s", est);
                        OLED_Print(10, 20, buf, 1, 1);
                    }

                    OLED_Print(10, 35, "VOLVER", 1, 1);
                    OLED_Print(0, 20 + (settings_idx * 15), ">", 1, 1);
                    break;

        case STATE_DASHBOARD:
            OLED_Print(0, 0, "TELEMETRIA", 2, 1);
            OLED_DrawHLine(0, 16, 128, 1);

            sprintf(buf, "L:%-4u C:%-4u R:%-4u", ir_l, ir_c, ir_r);
            OLED_Print(0, 18, buf, 1, 1);

            sprintf(buf, "SONAR: %u mm", dist_mm);
            OLED_Print(0, 28, buf, 1, 1);

            // CORRECCIÓN BUG IP: Subimos los estados de conexión a la fila 40
            OLED_Print(0, 40, is_pc_connected ? "LINK: OK" : "LINK: --", 1, 1);
            OLED_Print(80, 40, is_udp_connected ? "UDP: (*)" : "UDP: ( )", 1, 1);

            OLED_DrawHLine(0, 52, 128, 1);

            // Dejamos la fila 56 exclusiva para que la IP tenga todo el ancho de pantalla
            if (ip) {
                sprintf(buf, "IP: %s", ip);
                OLED_Print(0, 56, buf, 1, 1);
            }
            break;

        case STATE_LOG:
            OLED_Print(0, 0, "> ESP01 TERMINAL <", 1, 1);
            OLED_DrawHLine(0, 9, 128, 1);
            for(int i=0; i<6; i++) OLED_Print(0, 12 + (i*9), esp_log[i], 1, 1);
            break;

        case STATE_SETTINGS:
            OLED_Print(20, 0, "CONFIGURACION", 1, 1);
            OLED_DrawHLine(0, 10, 128, 1);
            sprintf(buf, "UART TX: [%s]", ui_enable_uart ? "ON " : "OFF");
            OLED_Print(10, 20, buf, 1, 1);
            sprintf(buf, "UDP TX:  [%s]", ui_enable_udp ? "ON " : "OFF");
            OLED_Print(10, 35, buf, 1, 1);
            OLED_Print(10, 50, "VOLVER", 1, 1);
            OLED_Print(0, 20 + (settings_idx * 15), ">", 1, 1);
            break;
    }

    // ==========================================
    // CAPA DE NOTIFICACIONES (DIBUJA POR ENCIMA)
    // ==========================================
    if (popup_activo > 0) {
        if (HAL_GetTick() - popup_timer > 1000) {
            popup_activo = 0; // Se venció el tiempo, lo apagamos
        } else {
            // Dibujamos la caja del cartelito en el centro de la pantalla
            OLED_FillRect(14, 12, 100, 40, 0); // Limpia el fondo (Relleno Negro)
            OLED_DrawRect(14, 12, 100, 40, 1); // Borde Blanco

            char popMsg[20];

            if (popup_activo == 1) {
                // --- POPUP SERVO ---
                sprintf(popMsg, "RADAR:");
                OLED_Print(45, 18, popMsg, 1, 1);
                sprintf(popMsg, "%d Gr", pop_val1);
                OLED_Print(45, 30, popMsg, 1, 1);

                // Icono del Servo Motor
                uint8_t ox = 20; uint8_t oy = 20;
                OLED_DrawRect(ox+4, oy+4, 12, 12, 1); // Cuerpo
                OLED_DrawHLine(ox, oy+8, 20, 1);      // Soportes laterales
                OLED_DrawRect(ox+8, oy, 4, 4, 1);     // Eje de engranaje

            } else if (popup_activo == 2) {
                // --- POPUP MOTOR (Auto y animación) ---
                char* dirStr = "STOP";
                if(pop_val1 == 1) dirStr = "ADELANTE";
                if(pop_val1 == 2) dirStr = "ATRAS";
                if(pop_val1 == 3) dirStr = "IZQ";
                if(pop_val1 == 4) dirStr = "DER";

                sprintf(popMsg, "%s", dirStr);
                OLED_Print(45, 18, popMsg, 1, 1);
                sprintf(popMsg, "VEL: %d%%", pop_val2);
                OLED_Print(45, 30, popMsg, 1, 1);

                // Icono del Autito Top-Down
                uint8_t ox = 20; uint8_t oy = 20;
                OLED_DrawRect(ox+4, oy, 10, 16, 1); // Chasis
                OLED_FillRect(ox+5, oy+4, 8, 3, 1); // Parabrisas
                OLED_FillRect(ox+5, oy+11, 8, 2, 1); // Luneta

                // Cálculo de vibración de ruedas (Animación)
                uint8_t anim = 0;
                if (pop_val2 > 0 && pop_val1 != 0) {
                    anim = (HAL_GetTick() / 100) % 2; // Alterna 0 y 1 cada 100ms
                }

                uint8_t ofs = (pop_val1 == 1 || pop_val1 == 2) ? anim*2 : 0;

                // Dibujo de ruedas con desplazamiento animado
                OLED_DrawRect(ox+1, oy+2 - ofs, 3, 4, 1);
                OLED_DrawRect(ox+14, oy+2 - ofs, 3, 4, 1);
                OLED_DrawRect(ox+1, oy+10 + ofs, 3, 4, 1);
                OLED_DrawRect(ox+14, oy+10 + ofs, 3, 4, 1);
            }
        }
    }

    OLED_Update();
}
