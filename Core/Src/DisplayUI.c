#include "DisplayUI.h"
#include "OLED.h"
#include <string.h>
#include <stdio.h>

// Estados de navegación
typedef enum {
    STATE_MAIN_MENU,
    STATE_DASHBOARD,
    STATE_LOG,
    STATE_SETTINGS
} eUIState;

// Variables internas de la interfaz
static eUIState current_state = STATE_MAIN_MENU;
static uint8_t main_menu_idx = 0; // 0: Dash, 1: Log, 2: Ajustes
static uint8_t settings_idx = 0;  // 0: UART, 1: UDP, 2: Back

// Variables expuestas al main
bool ui_enable_uart = true;
bool ui_enable_udp = true;

// Historial del Log (Se muda acá)
static char esp_log[6][22] = {"", "", "", "", "", ""};

void UI_Init(void) {
    current_state = STATE_MAIN_MENU;
}

// ---- LOGICA DE NAVEGACIÓN ----

void UI_ShortClick(void) {
    if (current_state == STATE_MAIN_MENU) {
        main_menu_idx = (main_menu_idx + 1) % 3; // Rota 0, 1, 2
    }
    else if (current_state == STATE_SETTINGS) {
        settings_idx = (settings_idx + 1) % 3;   // Rota 0, 1, 2
    }
}

void UI_LongClick(void) {
    if (current_state == STATE_MAIN_MENU) {
        // Entramos al submenú seleccionado
        if (main_menu_idx == 0) current_state = STATE_DASHBOARD;
        if (main_menu_idx == 1) current_state = STATE_LOG;
        if (main_menu_idx == 2) current_state = STATE_SETTINGS;
    }
    else if (current_state == STATE_DASHBOARD || current_state == STATE_LOG) {
        // Salimos con Long Click
        current_state = STATE_MAIN_MENU;
    }
    else if (current_state == STATE_SETTINGS) {
        // Ejecutamos la acción del ajuste seleccionado
        if (settings_idx == 0) ui_enable_uart = !ui_enable_uart;
        if (settings_idx == 1) ui_enable_udp = !ui_enable_udp;
        if (settings_idx == 2) current_state = STATE_MAIN_MENU; // Back
    }
}

// ---- LOGICA DE DATOS ----

void UI_AddLog(const char* texto) {
    // Scroll del texto
    for(int i = 0; i < 5; i++) {
        strcpy(esp_log[i], esp_log[i+1]);
    }
    char temp[22] = {0};
    int j = 0;
    for(int i = 0; texto[i] != '\0' && j < 21; i++) {
        if(texto[i] != '\r' && texto[i] != '\n') temp[j++] = texto[i];
    }
    strcpy(esp_log[5], temp);
}

// ---- RENDERIZADO VISUAL ----

void UI_Render(uint16_t ir_l, uint16_t ir_c, uint16_t ir_r, uint16_t dist_mm, const char* ip, uint8_t is_udp_connected, bool is_pc_connected) {
    char buf[30];
    OLED_Clear();

    switch (current_state) {
        case STATE_MAIN_MENU:
            OLED_Print(15, 0, "MENU PRINCIPAL", 1, 1);
            OLED_DrawHLine(0, 10, 128, 1);

            // Dibujamos las opciones, la seleccionada se marca con un ">"
            OLED_Print(10, 20, "DASHBOARD", 1, 1);
            OLED_Print(10, 35, "TERMINAL",  1, 1);
            OLED_Print(10, 50, "AJUSTES",   1, 1);

            OLED_Print(0, 20 + (main_menu_idx * 15), ">", 1, 1);
            break;

        case STATE_DASHBOARD:
                    OLED_Print(0, 0, "TELEMETRIA", 2, 1);
                    OLED_DrawHLine(0, 16, 128, 1);

                    // 1er Renglón: Sensores Infrarrojos (Lo subimos al pixel 20)
                    sprintf(buf, "L:%-4u C:%-4u R:%-4u", ir_l, ir_c, ir_r);
                    OLED_Print(0, 20, buf, 1, 1);

                    // 2do Renglón: NUEVO Sensor Ultrasónico (Pixel 35)
                    sprintf(buf, "SONAR: %u mm", dist_mm);
                    OLED_Print(0, 35, buf, 1, 1);

                    // Barra inferior y red
                    OLED_DrawHLine(0, 52, 128, 1);
                    if (ip) {
                        sprintf(buf, "%s", ip);
                        OLED_Print(0, 56, buf, 1, 1);
                    }

                    // NUEVO: Estado del enlace End-to-End
                                OLED_Print(55, 56, is_pc_connected ? "LINK:OK" : "LINK:--", 1, 1);

                                // El "circulito" de conexión UDP
                                OLED_Print(115, 56, is_udp_connected ? "(*)" : "( )", 1, 1);
              break;

        case STATE_LOG:
            OLED_Print(0, 0, "> ESP01 TERMINAL <", 1, 1);
            OLED_DrawHLine(0, 9, 128, 1);
            for(int i=0; i<6; i++) {
                OLED_Print(0, 12 + (i*9), esp_log[i], 1, 1);
            }
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

    OLED_Update();
}
