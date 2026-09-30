# 🎡 Base Giratoria BLE con ESP32-C3

<p align="center">
  <img src="https://img.shields.io/badge/ESP32--C3-Super%20Mini-blue" alt="ESP32-C3">
  <img src="https://img.shields.io/badge/ESP--IDF-v6.0-orange" alt="ESP-IDF v6.0">
  <img src="https://img.shields.io/badge/Version-0.1.0-brightgreen" alt="Version">
  <img src="https://img.shields.io/badge/License-MIT-yellow" alt="License">
</p>

<p align="center">
  Base giratoria motorizada con control BLE, iluminación RGB WS2812 y múltiples modos de efectos<br/>
  Controlable desde cualquier dispositivo con Bluetooth Low Energy (móvil, tablet, PC)
</p>

---

## 📖 Descripción General

Este proyecto implementa una **base giratoria motorizada** controlada de forma inalámbrica mediante **Bluetooth Low Energy (BLE)**. El dispositivo está basado en una **ESP32-C3 Super Mini** y permite:

- Controlar el **sentido de giro** de un motorreductor (horario / antihorario / detenido).
- Ajustar la **velocidad del motor** en un rango de 0 a 100%.
- Seleccionar entre **múltiples modos de iluminación** para una tira de LEDs WS2812.
- Ajustar el **brillo global** de los LEDs.

El firmware está desarrollado sobre **ESP-IDF v6.0** usando el stack **NimBLE** mediante la librería `h2zero/esp-nimble-cpp`, y el driver oficial `espressif/led_strip` para el control de los LEDs.

---

## ✨ Características

### Control del Motor
- ✅ Giro horario y antihorario
- ✅ Detención inmediata
- ✅ Control de velocidad PWM (0–100%)
- ✅ Velocidad por defecto configurable

### Control de Iluminación WS2812
- ✅ Modo **Apagado**
- ✅ Modo **Sólido** (color fijo)
- ✅ Modo **Respiración** (fade suave)
- ✅ Modo **Arcoíris** (ciclo de colores)
- ✅ Modo **Cálido** (transición entre rojo y ámbar)
- ✅ Control de brillo global (0–100%)

### Conectividad
- ✅ Bluetooth Low Energy (BLE) con NimBLE
- ✅ Advertising automático al desconectar
- ✅ Servicio y características con UUIDs únicos
- ✅ Escritura sin respuesta (WRITE_NR) para baja latencia

---

## 🔧 Hardware Requerido

| Componente | Cantidad | Notas |
|---|---|---|
| ESP32-C3 Super Mini | 1 | Placa principal |
| Motorreductor DC | 1 | Con driver puente H (L298N, TB6612, DRV8833) |
| Tira LED WS2812B | 1 | 12 LEDs (configurable en el código) |
| Fuente de alimentación 5V | 1 | Mínimo 2A recomendado |
| Fuente para motor | 1 | Separada de la del ESP32 (recomendado) |
| Resistencia 330Ω–470Ω | 1 | En serie con la línea de datos del WS2812 |
| Condensador 1000µF | 1 | Entre VCC y GND de la tira LED |
| Resistencia 10kΩ | 2 | Pull-down en los GPIOs de control del motor |

> ⚠️ **Importante**: La ESP32-C3 opera a 3.3V, pero los LEDs WS2812 esperan 5V en la línea de datos. Si experimentas parpadeos o LEDs fantasma, considera usar un **adaptador de nivel** (74HCT125) o alimentar la tira a 4.3V.

---

## 📍 Conexiones y Pines

### ESP32-C3 Super Mini → Periféricos

| Función | GPIO | Notas |
|---|---|---|
| Datos WS2812 | GPIO 1 | Con resistencia de 330Ω en serie |
| Control Motor A (horario) | GPIO 4 | Configuracion del pin en LOW |
| Control Motor B (antihorario) | GPIO 3 | Configuracion del pin en LOW |
| GND común | GND | Entre ESP32, tira LED y fuente del motor |


---

## 📁 Estructura del Proyecto

```
BaseGiratoriaBLE_ESP_IDF/
├── CMakeLists.txt
├── sdkconfig
├── main/
│ └── main.cpp # Punto de entrada, callbacks BLE
├── components/
│ ├── Ws2812_ColorEffect/ # Driver y efectos de los LEDs
│ │ ├── Ws2812_ColorEffect.c
│ │ ├── include/
│ │ │ └── Ws2812_ColorEffect.h
│ │ ├── CMakeLists.txt
│ │ └── idf_component.yml
│ ├── ControlMotor/ # Control PWM del motor
│ │ ├── ControlMotor.c
│ │ ├── include/
│ │ │ └── ControlMotor.h
│ │ └── CMakeLists.txt
│ └── (futuros componentes)
└── README.md
```
## 📡 Protocolo BLE
UUIDs del Proyecto
Elemento	UUID
- Servicio principal	4fafc201-1fb5-459e-8fcc-c5c9c331914b
- Característica Motor	4fafc201-1fb5-459e-8fcc-c5c9c331914c
- Característica WS2812	4fafc201-1fb5-459e-8fcc-c5c9c331914d
- Velocidad Motor	4fafc201-1fb5-459e-8fcc-c5c9c331914e
- Brillo RGB	4fafc201-1fb5-459e-8fcc-c5c9c331914f

## Comandos del Motor
Característica: CHAR_MOTOR_UUID
Formato: 1 byte

| Comando |	Valor	| Descripción |
|---|---|---|
| STOP | 0x00 |	Detiene el motor |
| HORARIO| 0x01| Giro en sentido horario |
| ANTIHORARIO | 0x02 | Giro en sentido antihorario |

## Comandos de Modos WS2812
Característica: CHAR_WS2812_UUID
Formato: 1 byte

| Comando | Valor | Descripción |
|---|---|---|
| APAGADO | 0x00| Apaga todos los LEDs |
| SÓLIDO | 0x01 | Color fijo |
| RESPIRACIÓN | 0x02 | Efecto de respiración |
| ARCOÍRIS | 0x03 | Ciclo de colores |
| CÁLIDO | 0x04	| Transición rojo ↔ ámbar |

## Control de Velocidad del Motor
Característica: VELOCIDAD_MOTOR_UUID
Formato: 1 byte (0–100)

| Valor | Descripción |
|---|---|
| 0x00 | 0% (detenido) |
| 0x32 | 50% |
| 0x64	| 100% (máxima velocidad) |

## Control de Brillo RGB
Característica: BRILLO_RGB_UUID
Formato: 1 byte (0–100)

| Valor	| Descripción |
|---|---|
| 0x00	| 0% (apagado) |
| 0x32	| 50% de brillo |
| 0x64	| 100% de brillo |

## Backend SPI vs RMT para WS2812
El proyecto usa el backend SPI con DMA para el control de los LEDs. Esto es crítico en el ESP32-C3 porque:

- El RMT no tiene DMA en este chip.

- La señal RMT depende de interrupciones que se retrasan bajo carga (BLE, motor).

- El SPI con DMA genera la señal de forma autónoma, inmune a la carga del CPU.

## La característica BLE no aparece
- Causa: Bluetooth o NimBLE no está habilitado en menuconfig.

- Solución: Ejecuta idf.py menuconfig → Component config → Bluetooth → habilitar NimBLE.

## 👤 Autor
Rodrigo Calle Condori

📧 Email: rodrigocallecondori5l@gmail.com

🐙 GitHub: @Rotronica

## Librerías y Créditos
- Espressif por el framework ESP-IDF y el driver led_strip

- h2zero por la excelente librería esp-nimble-cpp