#ifndef SERVO_H_
#define SERVO_H_

#include <stdint.h>

typedef void (*servo_write_pwm_cb_t)(uint16_t pulse_us);

typedef struct {
    // Hardware callback
    servo_write_pwm_cb_t set_pwm;

    // Calibración del SG90 (En microsegundos)
    uint16_t min_pulse_us;
    uint16_t max_pulse_us;

    // Variables de estado en PUNTO FIJO (Ángulo * 100)
    // Ejemplo: 90 grados = 9000, 180 grados = 18000
    int32_t current_angle;
    int32_t target_angle;
    int32_t speed_deg_per_tick; // Velocidad en centésimas de grado por tick
} sServoHandle;

// Inicializa el servo y lo lleva al centro (9000 = 90.00 grados)
void Servo_Init(sServoHandle *dev, servo_write_pwm_cb_t hardware_writer);

// Define los límites reales de tu servo
void Servo_Calibrate(sServoHandle *dev, uint16_t min_us, uint16_t max_us);

// Mueve instantáneamente el servo a un ángulo en punto fijo (0 a 18000)
void Servo_SetAngle(sServoHandle *dev, int32_t angle);

// Establece un ángulo objetivo y a qué velocidad debe viajar (valores * 100)
void Servo_MoveToSmooth(sServoHandle *dev, int32_t target_angle, int32_t speed);

// Tarea periódica para los movimientos suaves (llamar en el while(1))
void Servo_Task(sServoHandle *dev);

#endif /* SERVO_H_ */
