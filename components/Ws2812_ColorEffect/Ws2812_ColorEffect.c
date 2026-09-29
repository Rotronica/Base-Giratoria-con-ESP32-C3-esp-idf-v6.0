#include <stdio.h>
#include <stdlib.h>
#include "Ws2812_ColorEffect.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <math.h>
#include "led_strip.h"

static const char *TAG = "MI_WS2812";

// Helper: aplica brillo a un color RGB de 24 bits
static uint32_t aplicar_brillo(uint32_t color, uint8_t brillo)
{
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    r = (r * brillo) / 255;
    g = (g * brillo) / 255;
    b = (b * brillo) / 255;
    return (r << 16) | (g << 8) | b;
}

// Helper: pinta todos los LEDs con un mismo color
static void pintar_todos(mi_ws2812_t *dev, uint8_t r, uint8_t g, uint8_t b)
{
    for (uint32_t i = 0; i < dev->num_leds; i++)
    {
        led_strip_set_pixel(dev->strip, i, r, g, b);
    }
    led_strip_refresh(dev->strip);
}

// Helper: convierte HSV (h: 0-255, s: 0-255, v: 0-255) a RGB
static void hsv_a_rgb(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (s == 0)
    {
        *r = *g = *b = v;
        return;
    }
    uint8_t region = h / 43;               // 0..5
    uint8_t resto = (h - region * 43) * 6; // 0..255
    uint8_t p = (v * (255 - s)) / 255;
    uint8_t q = (v * (255 - ((s * resto) / 255))) / 255;
    uint8_t t = (v * (255 - ((s * (255 - resto)) / 255))) / 255;

    switch (region)
    {
    case 0:
        *r = v;
        *g = t;
        *b = p;
        break;
    case 1:
        *r = q;
        *g = v;
        *b = p;
        break;
    case 2:
        *r = p;
        *g = v;
        *b = t;
        break;
    case 3:
        *r = p;
        *g = q;
        *b = v;
        break;
    case 4:
        *r = t;
        *g = p;
        *b = v;
        break;
    default:
        *r = v;
        *g = p;
        *b = q;
        break;
    }
}

// Helper: interpola linealmente entre dos colores RGB24
// t va de 0 a 255
static uint32_t interpolar_color(uint32_t c1, uint32_t c2, uint8_t t)
{
    uint8_t r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
    uint8_t r2 = (c2 >> 16) & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = c2 & 0xFF;
    uint8_t r = r1 + ((int)(r2 - r1) * t) / 255;
    uint8_t g = g1 + ((int)(g2 - g1) * t) / 255;
    uint8_t b = b1 + ((int)(b2 - b1) * t) / 255;
    return (r << 16) | (g << 8) | b;
}

mi_ws2812_t *mi_ws2812_create(uint8_t gpio, uint8_t num_leds)
{
    mi_ws2812_t *dev = calloc(1, sizeof(mi_ws2812_t));
    if (dev == NULL)
    {
        ESP_LOGE(TAG, "No se pudo reservar memoria para el dispositivo");
        return NULL;
    }

    led_strip_config_t strip_config = {
        .strip_gpio_num = gpio,
        .max_leds = num_leds,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // WS2812 usa GRB
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false, // ESP32-C3 no tiene DMA para RMT
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &dev->strip));
    led_strip_clear(dev->strip);
    led_strip_refresh(dev->strip);

    dev->modo_actual = MODO_APAGADO;
    dev->color_primario = 0xFFFFFF;
    dev->color_secundario = 0x000000;
    dev->brillo = 255;
    dev->velocidad_ms = 50;
    dev->num_leds = num_leds;

    return dev;
}

void mi_ws2812_set_modo(mi_ws2812_t *dev, modo_luz_t modo)
{
    if (dev == NULL)
        return;
    dev->modo_actual = modo;
}

void mi_ws2812_set_color(mi_ws2812_t *dev, uint32_t rgb)
{
    if (dev == NULL)
        return;
    dev->color_primario = rgb;
}

void mi_ws2812_set_color_secundario(mi_ws2812_t *dev, uint32_t rgb)
{
    if (dev == NULL)
        return;
    dev->color_secundario = rgb;
}

void mi_ws2812_set_brillo(mi_ws2812_t *dev, uint8_t brillo)
{
    if (dev == NULL)
        return;
    dev->brillo = brillo;
}

void mi_ws2812_apagar(mi_ws2812_t *dev)
{
    if (dev == NULL)
        return;
    dev->modo_actual = MODO_APAGADO;
    led_strip_clear(dev->strip);
    led_strip_refresh(dev->strip);
}

void mi_ws2812_update(mi_ws2812_t *dev)
{
    if (dev == NULL)
        return;

    static uint32_t contador = 0;
    contador++;

    switch (dev->modo_actual)
    {
    case MODO_APAGADO:
    {
        led_strip_clear(dev->strip);
        led_strip_refresh(dev->strip);
        break;
    }

    case MODO_SOLIDO:
    {
        uint32_t c = aplicar_brillo(dev->color_primario, dev->brillo);
        pintar_todos(dev, (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
        break;
    }

    case MODO_RESPIRACION:
    {
        // Seno para fade suave: factor va de 0 a 255
        float fase = (contador % 100) / 100.0f;
        uint8_t factor = (uint8_t)(127.5f * (1.0f + sinf(fase * 2.0f * M_PI)));
        uint8_t brillo_efectivo = (uint8_t)((factor * dev->brillo) / 255);
        uint32_t c = aplicar_brillo(dev->color_primario, brillo_efectivo);
        pintar_todos(dev, (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
        break;
    }

    case MODO_ARCOIRIS:
    {
        for (uint32_t i = 0; i < dev->num_leds; i++)
        {
            uint8_t hue = (contador * 3 + i * 8) % 255;
            uint8_t r, g, b;
            hsv_a_rgb(hue, 255, dev->brillo, &r, &g, &b);
            led_strip_set_pixel(dev->strip, i, r, g, b);
        }
        led_strip_refresh(dev->strip);
        break;
    }

    case MODO_GRADIENTE:
    {
        // t va de 0 a 255 en un ciclo completo
        uint8_t t = (contador * 5) % 256;
        // Triángulo: 0..255..0 para ida y vuelta
        uint8_t t_tri = (t < 128) ? (t * 2) : ((255 - t) * 2);
        uint32_t c = interpolar_color(dev->color_primario, dev->color_secundario, t_tri);
        c = aplicar_brillo(c, dev->brillo);
        pintar_todos(dev, (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
        break;
    }

    default:
        break;
    }
}