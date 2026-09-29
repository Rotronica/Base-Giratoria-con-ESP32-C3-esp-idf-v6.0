#include <stdio.h>
#include "ControlMotor.h"
#include "driver/ledc.h"
#include "esp_err.h"

#define MOTOR_VELOCIDAD_DEFECTO 50 // 50% por defecto
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
    // 1. PRIMERO: configurar GPIOs en LOW con pull-down interno
    //    (esto ocurre antes de que el LEDC tome el control)
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GIRO_HORARIO) | (1ULL << GIRO_ANTIHORARIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(GIRO_HORARIO, 0);
    gpio_set_level(GIRO_ANTIHORARIO, 0);
    // 2. Configuración del Temporizador Único a 25kHz
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_SPEED_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // 3. Configurar Canal A (Giro horario)
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

    // 4. Configurar Canal B (Giro antihorario)
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
    // 5. Asegurar que ambos canales arrancan en 0
    Motor_stop();
    // 6. Velocidad por defecto (sin activar el motor)
    velocidad_actual = MOTOR_VELOCIDAD_DEFECTO;
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
