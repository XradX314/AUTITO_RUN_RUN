#include "Button.h"
#include "main.h" // Para usar HAL_GetTick()

#define DEBOUNCE_DELAY_MS 50

void Button_Init(sButtonHandle *btn) {
    btn->_lastPinRead = 0;
    btn->_debouncedState = 0;
    btn->_lastDebounceTime = 0;
    btn->_longClickTriggered = 0;
    btn->_pressTimestamp = 0;
}

void Button_Task(sButtonHandle *btn) {
    uint8_t currentRead = btn->ReadPin();
    uint32_t tick = HAL_GetTick();

    // Si hay ruido o rebote, reiniciamos el reloj
    if (currentRead != btn->_lastPinRead) {
        btn->_lastDebounceTime = tick;
    }

    // Si la señal está estable hace 50ms...
    if ((tick - btn->_lastDebounceTime) > DEBOUNCE_DELAY_MS) {

        // Y el estado estable es distinto al guardado (Hubo un flanco real)
        if (currentRead != btn->_debouncedState) {
            btn->_debouncedState = currentRead;

            if (btn->_debouncedState == 1) {
                // Flanco de subida (Recién presionado): Guardamos la hora
                btn->_pressTimestamp = tick;
                btn->_longClickTriggered = 0;
            }
            else {
                // Flanco de bajada (Recién soltado): Verificamos si fue click corto
                uint32_t pressDuration = tick - btn->_pressTimestamp;
                if (!btn->_longClickTriggered && pressDuration < btn->longClickTimeMs) {
                    if (btn->OnShortClick) btn->OnShortClick();
                }
            }
        }
    }

    // Mientras el botón siga presionado, verificamos si ya se cumplió el tiempo de click largo
    if (btn->_debouncedState == 1 && !btn->_longClickTriggered) {
        if ((tick - btn->_pressTimestamp) >= btn->longClickTimeMs) {
            btn->_longClickTriggered = 1; // Marcamos para no dispararlo 2 veces
            if (btn->OnLongClick) btn->OnLongClick();
        }
    }

    btn->_lastPinRead = currentRead;
}
