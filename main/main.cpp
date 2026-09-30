#include <stdio.h>
#include "NimBLEDevice.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

extern "C"
{
#include "Ws2812_ColorEffect.h"
#include "ControlMotor.h"
}

#define SALIDA_WS2812 GPIO_NUM_1
// UUIDs únicos para tu proyecto (puedes generarlos con uuidgenerator.net)
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_MOTOR_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914c"
#define CHAR_WS2812_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914d"
#define VELOCIDAD_MOTOR_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914e"
#define BRILLO_RGB_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914f"

static const char *TAG = "Main";
mi_ws2812_t *dev = NULL;
// Callback corregido para el servidor global
class MisCallbacksServidor : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override
    {
        ESP_LOGI(TAG, "Celular conectado");
    }
    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override
    {
        ESP_LOGI(TAG, "Celular desconectado (Razón: %d). Reiniciando anuncios...", reason);
        NimBLEDevice::startAdvertising();
    }
};
class MotorCallbacks : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        std::string valor = pCharacteristic->getValue();
        if (!valor.empty())
        {
            uint8_t comando = valor[0];
            switch (comando)
            {
            case 0x00:
                Motor_stop();
                ESP_LOGI(TAG, "Motor OFF");
                break;
            case 0x01:
                Motor_giro_Horario();
                ESP_LOGI(TAG, "Motor giro horario");
                break;
            case 0x02:
                Motor_giro_antiHorario();
                ESP_LOGI(TAG, "Motor giro antihorario");
                break;
            default:
                ESP_LOGW(TAG, "Comando desconocido recibido: 0x%02X", comando);
                break;
            }
        }
    }
};
class Ws2812Callbacks : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        std::string valor = pCharacteristic->getValue();
        if (!valor.empty())
        {
            uint8_t comando = valor[0];
            switch (comando)
            {
            case 0x00:
                mi_ws2812_set_modo(dev, MODO_APAGADO);
                ESP_LOGI(TAG, "Apagado");
                break;
            case 0x01:
                mi_ws2812_set_modo(dev, MODO_SOLIDO); // El monitoreo se volverá a encender automáticamente en la tarea de FreeRTOS
                ESP_LOGI(TAG, "Modo Solido");
                break;
            case 0x02:
                mi_ws2812_set_modo(dev, MODO_RESPIRACION); // El monitoreo se volverá a encender automáticamente en la tarea de FreeRTOS
                ESP_LOGI(TAG, "Modo Respiracion");
                break;
            case 0x03:
                mi_ws2812_set_modo(dev, MODO_ARCOIRIS); // El monitoreo se volverá a encender automáticamente en la tarea de FreeRTOS
                ESP_LOGI(TAG, "Modo Arcoiris");
                break;
            case 0x04:
                mi_ws2812_set_modo(dev, MODO_CALIDO); // El monitoreo se volverá a encender automáticamente en la tarea de FreeRTOS
                ESP_LOGI(TAG, "Modo Calido");
                break;
            default:
                ESP_LOGW(TAG, "Comando desconocido recibido: 0x%02X", comando);
                break;
            }
        }
    }
};

class VelocidadMotorCallbacks : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        std::string valor = pCharacteristic->getValue();
        if (!valor.empty())
        {
            uint8_t velocidad = valor[0];
            if (velocidad <= 100)
            {
                Motor_velocidad(velocidad);
                ESP_LOGI(TAG, "Velocidad motor: %d%%", velocidad);
            }
            else
            {
                ESP_LOGW(TAG, "Comando invalido para la velocida: %d%%", velocidad);
            }
        }
    }
};

class BrilloRGBCallbacks : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        std::string valor = pCharacteristic->getValue();
        if (valor.empty())
            return;

        uint8_t porcentaje = valor[0];
        if (porcentaje > 100)
        {
            porcentaje = 100; // Validación semántica
            ESP_LOGW(TAG, "El comando es mayor a 100%%");
        }
        else
        {
            uint8_t brillo = (porcentaje * 255) / 100; // Conversión
            mi_ws2812_set_brillo(dev, brillo);
            ESP_LOGI(TAG, "Brillo: %d%%", porcentaje);
        }
    }
};
// Variables estáticas para que los callbacks no se destruyan
static MisCallbacksServidor serverCallbacks;
static MotorCallbacks motorCallbacks;
static Ws2812Callbacks ws2812Callbacks;
static VelocidadMotorCallbacks velocidadMotorCallbacks;
static BrilloRGBCallbacks brillorgbCallbacks;
extern "C" void app_main(void)
{
    // 1. NVS primero
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    // 2. Crear el dispositivo WS2812
    dev = mi_ws2812_create(SALIDA_WS2812, 3);
    mi_ws2812_apagar(dev); // Apaga explícitamente todos los LEDs al arrancar
    Motor_init();

    // 3. BLE
    NimBLEDevice::init("BaseGiratoria");
    NimBLEServer *pServer = NimBLEDevice::createServer();

    // 4. ¡AQUÍ! Registrar los callbacks del servidor
    pServer->setCallbacks(&serverCallbacks);

    // 5. Servicio y características
    NimBLEService *pService = pServer->createService(SERVICE_UUID);

    // Caracteristica para el motor
    NimBLECharacteristic *pMotorChar = pService->createCharacteristic(
        CHAR_MOTOR_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    pMotorChar->setCallbacks(&motorCallbacks);

    // Caracteristica para los modos de los RGB
    NimBLECharacteristic *pWs2812Char = pService->createCharacteristic(
        CHAR_WS2812_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    pWs2812Char->setCallbacks(&ws2812Callbacks);

    // Caracteristica para controlar la velocidad del motor
    NimBLECharacteristic *pVelocidadMotor = pService->createCharacteristic(
        VELOCIDAD_MOTOR_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    pVelocidadMotor->setCallbacks(&velocidadMotorCallbacks);

    // Caracteristica para controlar el brillo de los leds RGB WS2812
    NimBLECharacteristic *pBrilloRGB = pService->createCharacteristic(
        BRILLO_RGB_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    pBrilloRGB->setCallbacks(&brillorgbCallbacks);

    // 6. Advertising
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);

    NimBLEAdvertisementData scanResponseData;
    scanResponseData.setName("BaseGiratoria");
    pAdvertising->setScanResponseData(scanResponseData);

    NimBLEDevice::startAdvertising();

    ESP_LOGI(TAG, "Servidor BLE iniciado. Esperando conexión...");

    // 7. Bucle principal: actualiza los LEDs
    while (1)
    {
        mi_ws2812_update(dev);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
