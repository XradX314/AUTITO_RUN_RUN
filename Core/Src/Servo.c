#include "Servo.h"

void Servo_Init(sServoHandle *dev, servo_write_pwm_cb_t hardware_writer) {
    dev->set_pwm = hardware_writer;
    dev->min_pulse_us = 500;  // Valores por defecto del SG90
    dev->max_pulse_us = 2500;
    dev->speed_deg_per_tick = 1.0f;

    // Arranca en el centro
    Servo_SetAngle(dev, 90.0f);
}

void Servo_Calibrate(sServoHandle *dev, uint16_t min_us, uint16_t max_us) {
    dev->min_pulse_us = min_us;
    dev->max_pulse_us = max_us;
}

void Servo_SetAngle(sServoHandle *dev, float angle) {
    // Limitamos el ángulo entre 0 y 180
    if (angle < 0.0f) angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;

    dev->current_angle = angle;
    dev->target_angle = angle;

    // Mapeo matemático: Convertimos los grados (0-180) a microsegundos (min-max)
    uint16_t pulse = dev->min_pulse_us + ((angle / 180.0f) * (dev->max_pulse_us - dev->min_pulse_us));

    if (dev->set_pwm) {
        dev->set_pwm(pulse);
    }
}

void Servo_MoveToSmooth(sServoHandle *dev, float target_angle, float speed) {
    if (target_angle < 0.0f) target_angle = 0.0f;
    if (target_angle > 180.0f) target_angle = 180.0f;

    dev->target_angle = target_angle;
    dev->speed_deg_per_tick = speed;
}

void Servo_Task(sServoHandle *dev) {
    // Si ya llegamos al destino, no hacemos nada
    if (dev->current_angle == dev->target_angle) return;

    // Calculamos el próximo paso
    if (dev->current_angle < dev->target_angle) {
        dev->current_angle += dev->speed_deg_per_tick;
        // Si nos pasamos, lo clavamos en el target
        if (dev->current_angle > dev->target_angle) dev->current_angle = dev->target_angle;
    }
    else {
        dev->current_angle -= dev->speed_deg_per_tick;
        if (dev->current_angle < dev->target_angle) dev->current_angle = dev->target_angle;
    }

    // Actualizamos el PWM con el nuevo paso intermedio
    uint16_t pulse = dev->min_pulse_us + ((dev->current_angle / 180.0f) * (dev->max_pulse_us - dev->min_pulse_us));
    if (dev->set_pwm) dev->set_pwm(pulse);
}
