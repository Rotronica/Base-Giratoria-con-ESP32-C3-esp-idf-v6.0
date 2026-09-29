#include <stdio.h>
#include "ControlMotor.h"
#include "driver/ledc.h"
#include "esp_err.h"

typedef enum
{
    HORARIO,
    ANTIHORARIO,
    DETENIDO
} sentido_de_motor_t;

// Registramos tanto el sentido como la última velocidad conocida
static sentido_de_motor_t sentido_motor = DETENIDO;
static uint8_t velocidad_actual = 0;

void Motor_init(void)
{
    // 1. Configuración del Temporizador Único a 25kHz
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_SPEED_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // 2. Configurar Canal A (Giro horario)
    ledc_channel_config_t channel_a = {
        .speed_mode = LEDC_SPEED_MODE,
        .channel = MOTOR_CHANNEL_A,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = GIRO_HORARIO,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_a));

    // 3. Configurar Canal B (Giro antihorario)
    ledc_channel_config_t channel_b = {
        .speed_mode = LEDC_SPEED_MODE,
        .channel = MOTOR_CHANNEL_B,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = GIRO_ANTIHORARIO,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_b));
    Motor_stop();
}

// Al presionar los botones, cambiamos el sentido y refrescamos el hardware inmediatamente
void Motor_giro_Horario(void)
{
    sentido_motor = HORARIO;
    Motor_velocidad(velocidad_actual); // Forzar actualización de hardware
}

void Motor_giro_antiHorario(void) // Corregido: Sin parámetros innecesarios
{
    sentido_motor = ANTIHORARIO;
    Motor_velocidad(velocidad_actual); // Forzar actualización de hardware
}

void Motor_stop(void)
{
    sentido_motor = DETENIDO;

    // Detenido inmediato en hardware
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_SPEED_MODE, MOTOR_CHANNEL_A, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_SPEED_MODE, MOTOR_CHANNEL_A));

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_SPEED_MODE, MOTOR_CHANNEL_B, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_SPEED_MODE, MOTOR_CHANNEL_B));
}

void Motor_velocidad(uint8_t porcentaje_velocidad)
{
    if (porcentaje_velocidad > 100)
        porcentaje_velocidad = 100;

    velocidad_actual = porcentaje_velocidad; // Guardar la velocidad en memoria

    // SOLUCIÓN AL OVERFLOW: Castear a uint32_t antes de multiplicar (1023 * 100 = 102300)
    uint16_t duty_cycle = (1023 * (uint32_t)porcentaje_velocidad) / 100;

    switch (sentido_motor)
    {
    case HORARIO:
        // Sentido antihorario apagado primero por seguridad
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_SPEED_MODE, MOTOR_CHANNEL_B, 0));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_SPEED_MODE, MOTOR_CHANNEL_B));

        // Aplicar velocidad al horario
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_SPEED_MODE, MOTOR_CHANNEL_A, duty_cycle));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_SPEED_MODE, MOTOR_CHANNEL_A));
        break;

    case ANTIHORARIO:
        // Sentido horario apagado primero por seguridad
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_SPEED_MODE, MOTOR_CHANNEL_A, 0));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_SPEED_MODE, MOTOR_CHANNEL_A));

        // Aplicar velocidad al antihorario
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_SPEED_MODE, MOTOR_CHANNEL_B, duty_cycle));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_SPEED_MODE, MOTOR_CHANNEL_B));
        break;

    case DETENIDO:
    default:
        // Asegurar apagado total
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_SPEED_MODE, MOTOR_CHANNEL_A, 0));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_SPEED_MODE, MOTOR_CHANNEL_A));

        ESP_ERROR_CHECK(ledc_set_duty(LEDC_SPEED_MODE, MOTOR_CHANNEL_B, 0));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_SPEED_MODE, MOTOR_CHANNEL_B));
        break;
    }
}
