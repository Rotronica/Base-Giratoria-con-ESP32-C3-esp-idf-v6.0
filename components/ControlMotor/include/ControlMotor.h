#pragma once
#include <stdint.h>
#include "driver/gpio.h"
#include "driver/ledc.h"

// Definicion de variables del pwm
#define LEDC_SPEED_MODE LEDC_LOW_SPEED_MODE // Obligatorio para ESP32-C3
#define LEDC_TIMER LEDC_TIMER_0             // Mismo temporizador para ambos

// MODIFICACIÓN DE FRECUENCIA Y RESOLUCIÓN PARA ELIMINAR EL RUIDO AUDIBLE
#define LEDC_FREQUENCY (25000)          // 25 kHz (Inaudible para el oído humano)
#define LEDC_DUTY_RES LEDC_TIMER_10_BIT // Resolución de 10 bits (Rango de velocidad de 0 a 1023)

// Definición de canales LEDC independientes
#define MOTOR_CHANNEL_A LEDC_CHANNEL_0
#define MOTOR_CHANNEL_B LEDC_CHANNEL_1

#define GIRO_HORARIO GPIO_NUM_2
#define GIRO_ANTIHORARIO GPIO_NUM_3
void Motor_init(void);
void Motor_giro_Horario(void);
void Motor_giro_antiHorario(void);
void Motor_velocidad(uint8_t porcentaje_velocidad);
void Motor_stop(void);