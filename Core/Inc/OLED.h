#ifndef OLED_H_
#define OLED_H_

#include <stdint.h>

// Estados de la máquina de refresco
typedef enum {
    OLED_IDLE,
    OLED_BUSY,
    OLED_ERROR
} eOLEDStatus;

// La estructura puente (Agnóstica)
typedef struct {
    int (*I2C_WriteCmd)(uint8_t cmd);
    int (*I2C_WriteData_DMA)(uint8_t *data, uint16_t len);
} sOLEDHandle;

// Inicialización y Control Core
void OLED_Init(sOLEDHandle *hOLED);
void OLED_Update(void);
void OLED_Task(void);
void OLED_DMA_Callback(void);

// Primitivas Gráficas
void OLED_Clear(void);
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void OLED_DrawHLine(uint8_t x, uint8_t y, uint8_t length, uint8_t color);
void OLED_DrawVLine(uint8_t x, uint8_t y, uint8_t length, uint8_t color);
void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void OLED_DrawCircle(int32_t x0, int32_t y0, int32_t radius, uint8_t color);
void OLED_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);

// Texto y Telemetría
void OLED_PutChar(uint8_t x, uint8_t y, char c, uint8_t size, uint8_t color);
void OLED_Print(uint8_t x, uint8_t y, const char* str, uint8_t size, uint8_t color);
void OLED_ShowTelemetry(uint16_t ir_izq, uint16_t ir_cen, uint16_t ir_der);

#endif
