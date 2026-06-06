#include "comandos.h"
#include "protocolo.h"
#include "DisplayUI.h" // Para usar UI_AddLog
#include "ESP01.h"
#include <string.h>
#include <stdio.h>
#include "main.h" // Para HAL_GetTick()
#include "Servo.h"


extern UART_HandleTypeDef huart1; // Para poder usar el UART de la PC acá
// NUEVO: Le avisamos al compilador que busque a htim3 en otro archivo
extern TIM_HandleTypeDef htim3;

extern sServoHandle mi_servo; // Nos "traemos" el servo del main.c
bool pc_conectada = false;
static uint32_t ultimo_ack_ms = 0;

// ---- RECEPTOR: ¿Qué hacemos cuando llega un paquete válido? ----
void Comandos_Parsear(uint8_t cmd, uint8_t* params, uint8_t len) {
    char logMsg[30];

    switch(cmd) {
        case CMD_ACCION:
            // ... (tu código anterior de los LEDs) ...
            break;

        // NUEVO: La PC nos respondió el latido
        case CMD_ALIVE_ACK:
            ultimo_ack_ms = HAL_GetTick(); // Reseteamos el reloj de la bomba

            if (!pc_conectada) {
                pc_conectada = true;
                UI_AddLog("ENLACE PC: OK");
            }
            break;
        case CMD_SET_ANGLE:
                    // Nos aseguramos de que haya llegado al menos 1 byte
                    if (len >= 1) {
                        uint8_t angulo_recibido = params[0];

                        // Le pasamos el ángulo directo a tu librería
                        Servo_SetAngle(&mi_servo, (float)angulo_recibido);

                        // Opcional: Lo mostramos en la OLED para confirmar
                        sprintf(logMsg, "SERVO: %d GRADOS", angulo_recibido);
                        UI_AddLog(logMsg);
                    }
                    break;

        case CMD_MOTORES:
                    if (len >= 2) {
                        uint8_t direccion = params[0];
                        uint8_t velocidad = params[1]; // Viene de 0 a 100

                        // Mapeo de velocidad (0-100%) al rango exacto de tu PWM (0-9999)
                        uint16_t pwm_val = (velocidad * 9999) / 100;

                        switch(direccion) {
                            case 0: // STOP - Freno total (Se ejecuta al soltar la tecla/botón)
                                HAL_GPIO_WritePin(GPIOB, IN_1_Pin, GPIO_PIN_RESET);
                                HAL_GPIO_WritePin(GPIOB, IN_2_Pin, GPIO_PIN_RESET);
                                HAL_GPIO_WritePin(GPIOB, IN_3_Pin, GPIO_PIN_RESET);
                                HAL_GPIO_WritePin(GPIOB, IN_4_Pin, GPIO_PIN_RESET);
                                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
                                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);
                                UI_AddLog("MOT: STOP");
                                break;

                            case 1: // ADELANTE
                                HAL_GPIO_WritePin(GPIOB, IN_1_Pin, GPIO_PIN_SET);
                                HAL_GPIO_WritePin(GPIOB, IN_2_Pin, GPIO_PIN_RESET);
                                HAL_GPIO_WritePin(GPIOB, IN_3_Pin, GPIO_PIN_SET);
                                HAL_GPIO_WritePin(GPIOB, IN_4_Pin, GPIO_PIN_RESET);
                                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, pwm_val);
                                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, pwm_val);
                                UI_AddLog("MOT: ADELANTE");
                                break;

                            case 2: // ATRÁS (Se invierten los pines IN)
                                HAL_GPIO_WritePin(GPIOB, IN_1_Pin, GPIO_PIN_RESET);
                                HAL_GPIO_WritePin(GPIOB, IN_2_Pin, GPIO_PIN_SET);
                                HAL_GPIO_WritePin(GPIOB, IN_3_Pin, GPIO_PIN_RESET);
                                HAL_GPIO_WritePin(GPIOB, IN_4_Pin, GPIO_PIN_SET);
                                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, pwm_val);
                                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, pwm_val);
                                UI_AddLog("MOT: ATRAS");
                                break;

                            case 3: // IZQUIERDA (Giro de Tanque)
                                // Derecha avanza (IN1=1, IN2=0) | Izquierda retrocede (IN3=0, IN4=1)
                                HAL_GPIO_WritePin(GPIOB, IN_1_Pin, GPIO_PIN_SET);
                                HAL_GPIO_WritePin(GPIOB, IN_2_Pin, GPIO_PIN_RESET);
                                HAL_GPIO_WritePin(GPIOB, IN_3_Pin, GPIO_PIN_RESET);
                                HAL_GPIO_WritePin(GPIOB, IN_4_Pin, GPIO_PIN_SET);
                                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, pwm_val);
                                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, pwm_val);
                                UI_AddLog("MOT: IZQ");
                                break;

                            case 4: // DERECHA (Giro de Tanque)
                                // Derecha retrocede (IN1=0, IN2=1) | Izquierda avanza (IN3=1, IN4=0)
                                HAL_GPIO_WritePin(GPIOB, IN_1_Pin, GPIO_PIN_RESET);
                                HAL_GPIO_WritePin(GPIOB, IN_2_Pin, GPIO_PIN_SET);
                                HAL_GPIO_WritePin(GPIOB, IN_3_Pin, GPIO_PIN_SET);
                                HAL_GPIO_WritePin(GPIOB, IN_4_Pin, GPIO_PIN_RESET);
                                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, pwm_val);
                                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, pwm_val);
                                UI_AddLog("MOT: DER");
                                break;
                        }
                    }
                    break;

        default:
            sprintf(logMsg, "CMD Desconocido: 0x%02X", cmd);
            UI_AddLog(logMsg);
            break;
    }
}

// NUEVO: Funciones de Heartbeat
void Comandos_EnviarAlive(void) {
    // Mandamos el comando 0x03 sin payload (0 bytes de datos)
    Encode(CMD_ALIVE, (uint8_t*)0, 0);
}

void Comandos_ChequearTimeout(void) {
    // Si estábamos conectados, pero pasaron 3 segundos (3000 ms) sin recibir un ACK...
    if (pc_conectada && (HAL_GetTick() - ultimo_ack_ms > 1500)) {
        pc_conectada = false;
        UI_AddLog("ENLACE PC: PERDIDO");

        // --- FAILSAFE: CORTAR MOTORES INMEDIATAMENTE ---
                HAL_GPIO_WritePin(GPIOB, IN_1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOB, IN_2_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOB, IN_3_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOB, IN_4_Pin, GPIO_PIN_RESET);
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);
    }
}

// ---- TRANSMISOR: Armar el paquete de telemetría ----
void Comandos_EnviarTelemetria(uint16_t ir_l, uint16_t ir_c, uint16_t ir_r, uint16_t sonar) {
    sTelemetriaTx paquete;
    paquete.ir_l = ir_l;
    paquete.ir_c = ir_c;
    paquete.ir_r = ir_r;
    paquete.sonar = sonar;

    // Convertimos la estructura a un array de bytes puro y lo codificamos
    Encode(CMD_TELEMETRIA, (uint8_t*)&paquete, sizeof(sTelemetriaTx));
}

// ---- HARDWARE: Sacar los bytes codificados por los cables/antena ----
void Comandos_FlushTx(void) {
    static uint8_t bufferSalida[128];
    uint8_t len = 0;

    // Sacamos todo del RingBuffer de la librería protocolo
    while(tx.rBuf.ir != tx.rBuf.iw) {
        bufferSalida[len++] = tx.rBuf.buf[tx.rBuf.ir++];
        tx.rBuf.ir &= (tx.rBuf.size - 1);
    }

    // Si hay bytes listos, los escupimos por los puertos activos
    if(len > 0) {
        if(ui_enable_uart) {
            HAL_UART_Transmit(&huart1, bufferSalida, len, 100);
        }
        if(ui_enable_udp && (ESP01_StateUDPTCP() == ESP01_UDPTCP_CONNECTED)) {
            ESP01_Send(bufferSalida, 0, len, len);
        }
    }
}
