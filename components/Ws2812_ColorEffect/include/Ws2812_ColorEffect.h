#pragma once
#include "led_strip.h"

/**
 * @file Ws2812_ColorEffect.h
 * @brief Componente de control de LEDs WS2812 con múltiples modos de iluminación.
 *
 * Este componente permite controlar tiras de LEDs WS2812 (NeoPixel) usando el
 * driver oficial `espressif/led_strip` sobre el periférico RMT del ESP32.
 *
 * Ofrece varios modos de iluminación predefinidos (sólido, respiración,
 * arcoíris y gradiente) y permite ajustar color, brillo y modo en tiempo real,
 * lo que lo hace ideal para integración con BLE (por ejemplo, NimBLE) para
 * control remoto desde un móvil.
 *
 * @note Este componente NO es thread-safe. Todas las llamadas a la API deben
 *       realizarse desde la misma tarea, o protegerse con un mutex si se
 *       invocan desde callbacks de BLE u otras tareas.
 */

/**
 * @brief Modos de iluminación disponibles.
 */
typedef enum
{
    MODO_APAGADO,     /**< LEDs apagados. */
    MODO_SOLIDO,      /**< Color fijo en todos los LEDs. */
    MODO_RESPIRACION, /**< Efecto de respiración (fade in/out suave). */
    MODO_ARCOIRIS,    /**< Ciclo de arcoíris a lo largo de la tira. */
    MODO_GRADIENTE,   /**< Transición suave entre dos colores. */
} modo_luz_t;

/**
 * @brief Estructura de control del dispositivo WS2812.
 *
 * Contiene el handle del driver, el estado actual del efecto y los parámetros
 * de configuración (colores, brillo, número de LEDs).
 *
 * @note Los campos de esta estructura son de uso interno del componente.
 *       No modifiques directamente sus valores; usa las funciones de la API.
 */
typedef struct
{
    led_strip_handle_t strip;  /**< Handle del driver led_strip (RMT). */
    modo_luz_t modo_actual;    /**< Modo de iluminación activo. */
    uint32_t color_primario;   /**< Color principal en formato RGB24 (0xRRGGBB). */
    uint32_t color_secundario; /**< Color secundario para MODO_GRADIENTE (0xRRGGBB). */
    uint32_t velocidad_ms;     /**< Reservado para futuras versiones (ritmo de animación). */
    uint32_t num_leds;         /**< Número real de LEDs de la tira. */
    uint8_t brillo;            /**< Brillo global (0-255). */
} mi_ws2812_t;

/**
 * @brief Crea e inicializa una instancia de control WS2812.
 *
 * Reserva memoria para la estructura de control, configura el driver RMT
 * del ESP32 y deja la tira apagada lista para usarse.
 *
 * @param gpio      Número de GPIO donde está conectado el pin de datos del WS2812.
 *                  En la ESP32-C3 Super Mini, el LED integrado está en GPIO 8.
 * @param num_leds  Número de LEDs de la tira. Debe ser >= 1.
 *
 * @return Puntero a la estructura `mi_ws2812_t` en caso de éxito.
 *         Devuelve `NULL` si no se pudo reservar memoria.
 *
 * @note El GPIO debe ser válido y no estar en uso por otro periférico.
 * @note Esta función debe llamarse UNA SOLA VEZ al inicio del programa.
 *
 * @code
 * mi_ws2812_t *led = mi_ws2812_create(8, 30);
 * if (led == NULL) {
 *     ESP_LOGE(TAG, "Error al crear el dispositivo");
 * }
 * @endcode
 */
mi_ws2812_t *mi_ws2812_create(uint8_t gpio, uint8_t num_leds);

/**
 * @brief Cambia el modo de iluminación activo.
 *
 * El nuevo modo se aplicará en la siguiente llamada a `mi_ws2812_update()`.
 *
 * @param dev   Puntero a la instancia creada con `mi_ws2812_create()`.
 * @param modo  Modo a activar (ver `modo_luz_t`).
 *
 * @note Si `dev` es `NULL`, la función no hace nada.
 *
 * @code
 * mi_ws2812_set_modo(led, MODO_RESPIRACION);
 * @endcode
 */
void mi_ws2812_set_modo(mi_ws2812_t *dev, modo_luz_t modo);

/**
 * @brief Establece el color principal del efecto.
 *
 * El color se guarda y se aplica en la siguiente llamada a `mi_ws2812_update()`.
 * Se usa como color base en todos los modos excepto en `MODO_GRADIENTE`, donde
 * es el punto de partida de la transición.
 *
 * @param dev  Puntero a la instancia creada con `mi_ws2812_create()`.
 * @param rgb  Color en formato RGB24 (0xRRGGBB). Por ejemplo:
 *             - Rojo:   `0xFF0000`
 *             - Verde:  `0x00FF00`
 *             - Azul:   `0x0000FF`
 *             - Blanco: `0xFFFFFF`
 *
 * @note Si `dev` es `NULL`, la función no hace nada.
 *
 * @code
 * mi_ws2812_set_color(led, 0xFF0000); // Rojo
 * @endcode
 */
void mi_ws2812_set_color(mi_ws2812_t *dev, uint32_t rgb);

/**
 * @brief Establece el color secundario (usado en MODO_GRADIENTE).
 *
 * Solo tiene efecto cuando el modo activo es `MODO_GRADIENTE`. El efecto
 * interpola suavemente entre `color_primario` y `color_secundario`.
 *
 * @param dev  Puntero a la instancia creada con `mi_ws2812_create()`.
 * @param rgb  Color en formato RGB24 (0xRRGGBB).
 *
 * @note Si `dev` es `NULL`, la función no hace nada.
 *
 * @code
 * mi_ws2812_set_color(led, 0xFF0000);          // Rojo
 * mi_ws2812_set_color_secundario(led, 0x0000FF); // Azul
 * mi_ws2812_set_modo(led, MODO_GRADIENTE);
 * @endcode
 */
void mi_ws2812_set_color_secundario(mi_ws2812_t *dev, uint32_t rgb);

/**
 * @brief Ajusta el brillo global de la tira.
 *
 * El brillo se aplica multiplicando cada componente RGB por `brillo / 255`.
 * El valor se guarda y se aplica en la siguiente llamada a `mi_ws2812_update()`.
 *
 * @param dev     Puntero a la instancia creada con `mi_ws2812_create()`.
 * @param brillo  Valor de brillo entre 0 (apagado) y 255 (máximo).
 *
 * @note Si `dev` es `NULL`, la función no hace nada.
 * @note Un brillo muy alto con muchos LEDs puede superar la corriente máxima
 *       que puede entregar la fuente. Considera limitar el brillo a 100-150
 *       en tiras largas.
 *
 * @code
 * mi_ws2812_set_brillo(led, 128); // 50% de brillo
 * @endcode
 */
void mi_ws2812_set_brillo(mi_ws2812_t *dev, uint8_t brillo);

/**
 * @brief Actualiza el estado de los LEDs según el modo activo.
 *
 * Esta función debe llamarse periódicamente desde tu bucle principal o desde
 * una tarea dedicada. Se encarga de calcular los colores actuales según el
 * modo y de escribir los píxeles en la tira.
 *
 * @param dev  Puntero a la instancia creada con `mi_ws2812_create()`.
 *
 * @note Si `dev` es `NULL`, la función no hace nada.
 * @note La frecuencia de llamada determina la velocidad de las animaciones.
 *       Un intervalo de 20-50 ms (20-50 FPS) da buenos resultados.
 * @note Esta función NO debe llamarse desde un callback de BLE directamente,
 *       ya que el acceso a RMT puede bloquear. Mejor usa una cola o un flag
 *       y actualiza desde la tarea principal.
 *
 * @code
 * while (1) {
 *     mi_ws2812_update(led);
 *     vTaskDelay(pdMS_TO_TICKS(20));
 * }
 * @endcode
 */
void mi_ws2812_update(mi_ws2812_t *dev);

/**
 * @brief Apaga todos los LEDs inmediatamente.
 *
 * Limpia la tira, refresca el hardware y deja el modo en `MODO_APAGADO`.
 * A diferencia de otras funciones, esta actúa de inmediato sin esperar a
 * la siguiente llamada a `mi_ws2812_update()`.
 *
 * @param dev  Puntero a la instancia creada con `mi_ws2812_create()`.
 *
 * @note Si `dev` es `NULL`, la función no hace nada.
 * @note Es útil para emergencias, ahorro de energía o al recibir un comando
 *       de "apagar" por BLE.
 *
 * @code
 * mi_ws2812_apagar(led);
 * @endcode
 */
void mi_ws2812_apagar(mi_ws2812_t *dev);