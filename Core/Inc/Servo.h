#ifndef SERVO_H_
#define SERVO_H_

#include <stdint.h>

typedef void (*servo_write_pwm_cb_t)(uint16_t pulse_us);

typedef struct {
    // Hardware callback
    servo_write_pwm_cb_t set_pwm;

    // Calibración del SG90 (En microsegundos)
    uint16_t min_pulse_us; // Típicamente 500
    uint16_t max_pulse_us; // Típicamente 2500

    // Variables de estado para movimiento suave y no bloqueante
    float current_angle;
    float target_angle;
    float speed_deg_per_tick;
} sServoHandle;

// Inicializa el servo y lo lleva al centro (90 grados)
void Servo_Init(sServoHandle *dev, servo_write_pwm_cb_t hardware_writer);

// Define los límites reales de tu servo (algunos chinos varían entre 600 y 2400)
void Servo_Calibrate(sServoHandle *dev, uint16_t min_us, uint16_t max_us);

// Mueve instantáneamente el servo a un ángulo (0 a 180)
void Servo_SetAngle(sServoHandle *dev, float angle);

// Establece un ángulo objetivo y a qué velocidad debe viajar
void Servo_MoveToSmooth(sServoHandle *dev, float target_angle, float speed);

// Tarea que se debe llamar en el while(1) periódicamente para los movimientos suaves
void Servo_Task(sServoHandle *dev);

#endif /* SERVO_H_ */
