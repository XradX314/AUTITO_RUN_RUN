#ifndef BUTTON_H_
#define BUTTON_H_

#include <stdint.h>

// Estructura que define a un Botón
typedef struct {
    // ---- CONFIGURACIÓN ----
    // Función para leer el pin (Devuelve 1 presionado, 0 suelto)
    uint8_t (*ReadPin)(void);

    // Tiempo para considerar un "Long Click" (en milisegundos)
    uint32_t longClickTimeMs;

    // Callbacks de los clicks
    void (*OnShortClick)(void);
    void (*OnLongClick)(void);

    // ---- MEMORIA INTERNA (No tocar) ----
    uint8_t  _lastPinRead;
    uint32_t _lastDebounceTime;
    uint8_t  _debouncedState;
    uint32_t _pressTimestamp;
    uint8_t  _longClickTriggered;
} sButtonHandle;

// Prototipos
void Button_Init(sButtonHandle *btn);
void Button_Task(sButtonHandle *btn);

#endif /* BUTTON_H_ */
