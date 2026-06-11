#include "Servo.h"

void Servo_Init(sServoHandle *dev, servo_write_pwm_cb_t hardware_writer) {
    dev->set_pwm = hardware_writer;
    dev->min_pulse_us = 544;  // Valores por defecto del SG90
    dev->max_pulse_us = 2400;
    dev->speed_deg_per_tick = 100; // 1.00 grado por tick por defecto (1 * 100)

    // Arranca en el centro (9000 = 90.00 grados)
    Servo_SetAngle(dev, 9000);
}

void Servo_Calibrate(sServoHandle *dev, uint16_t min_us, uint16_t max_us) {
    dev->min_pulse_us = min_us;
    dev->max_pulse_us = max_us;
}

void Servo_SetAngle(sServoHandle *dev, int32_t angle) {
    // Limitamos el ángulo en punto fijo entre 0 y 18000
    if (angle < 0) angle = 0;
    if (angle > 18000) angle = 18000;

    dev->current_angle = angle;
    dev->target_angle = angle;

    // Mapeo matemático puro con enteros:
    // pulse = min + [ angle * (max - min) ] / 18000
    uint32_t rango_pulse = (uint32_t)(dev->max_pulse_us - dev->min_pulse_us);
    uint16_t pulse = dev->min_pulse_us + (uint16_t)((angle * rango_pulse) / 18000);

    if (dev->set_pwm) {
        dev->set_pwm(pulse);
    }
}

void Servo_MoveToSmooth(sServoHandle *dev, int32_t target_angle, int32_t speed) {
    if (target_angle < 0) target_angle = 0;
    if (target_angle > 18000) target_angle = 18000;

    dev->target_angle = target_angle;
    dev->speed_deg_per_tick = speed;
}

void Servo_Task(sServoHandle *dev) {
    // Si ya llegamos al destino, no hacemos nada
    if (dev->current_angle == dev->target_angle) return;

    // Calculamos el próximo paso intermedio en enteros
    if (dev->current_angle < dev->target_angle) {
        dev->current_angle += dev->speed_deg_per_tick;
        // Si nos pasamos, lo clavamos en el target
        if (dev->current_angle > dev->target_angle) dev->current_angle = dev->target_angle;
    }
    else {
        dev->current_angle -= dev->speed_deg_per_tick;
        if (dev->current_angle < dev->target_angle) dev->current_angle = dev->target_angle;
    }

    // Actualizamos el PWM con aritmética entera
    uint32_t rango_pulse = (uint32_t)(dev->max_pulse_us - dev->min_pulse_us);
    uint16_t pulse = dev->min_pulse_us + (uint16_t)((dev->current_angle * rango_pulse) / 18000);

    if (dev->set_pwm) dev->set_pwm(pulse);
}
