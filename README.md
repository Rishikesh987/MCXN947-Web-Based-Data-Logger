project name - MCXN947_Web_Based_Data_Logger

MCXN947-based IoT embedded system using FreeRTOS and lwIP Ethernet networking, with a web-based interface for real-time sensor monitoring, RTC timestamped data logging, EEPROM historical storage, and remote control, implemented using a layered driver/service/application architecture.

Key Features
🌐 Ethernet-based IoT communication
🖥️ Embedded Web Server running directly on MCXN947
📡 TCP/IP networking using lwIP
⚙️ FreeRTOS-based multitasking
🌡️ Real-time temperature and pressure monitoring using BMP280/BME280
🕒 RTC timestamping using DS3231
💾 Historical sensor data logging to external EEPROM
📊 Web-based sensor data visualization
🔄 Real-time data communication between MCU and browser
🎛️ Remote control of actuators/devices through the web interface
💡 LED status indication using a dedicated FreeRTOS task
🔌 Custom I²C and GPIO drivers
🧩 Layered software architecture
Application layer
Service layer
Driver layer
SDK/HAL
🔧 Modular and reusable driver/service design
⏱️ Periodic task execution using FreeRTOS
🛠️ NXP MCUXpresso SDK integration
🔐 Static IPv4 network configuration
💻 Browser-based control and monitoring without a dedicated PC application


```
┌─────────────────────────────────────────────────────────────────────────────┐
│                             APPLICATION LAYER                               │
│                                                                             │
│  Application Task (Central Business Logic Thread)                          │
│  • Manages system state machine transitions                                 │
│  • Parses and executes incoming web dashboard commands                      │
│  • Determines local actuator/device control operations                      │
│  • Formats and prepares raw metrics for the logging queue                   │
└──────────────────────────────────────┬──────────────────────────────────────┘
                                       │
┌──────────────────────────────────────▼──────────────────────────────────────┐
│                               SERVICE LAYER                                 │
│                                                                             │
│  📡 Sensor Service                                                          │
│  • Coordinates digital & analog sensor data acquisition                      │
│  • Handles averaging, filtering, and signal noise removal                   │
│  • Conducts boundary verification & sensor hardware validation              │
│                                                                             │
│  💾 Logger Service                                                          │
│  • Manages internal RAM circular data buffering                             │
│  • Binds raw sensor readings to valid RTC calendar timestamps               │
│  • Handles page-write scheduling for the external EEPROM                    │
│                                                                             │
│  🕒 RTC Service                                                             │
│  • Controls the physical DS3231 timekeeping hardware tracking module        │
│  • Generates accurate timezone-bound epoch/calendar timestamp arrays        │
│                                                                             │
│  🌐 Network / Web Service                                                   │
│  • Manages the raw lwIP TCP/IP stack configuration frameworks               │
│  • Implements a lightweight HTTP network background server daemon           │
│  • Facilitates asynchronous browser-to-MCU JSON data exchanges              │
│                                                                             │
│  💡 Indicator Service                                                       │
│  • Decouples state monitoring from physical visualization                   │
│  • Operates dynamic multi-frequency system status blink codes               │
└──────────────────────────────────────┬──────────────────────────────────────┘
                                       │
┌──────────────────────────────────────▼──────────────────────────────────────┐
│                        DRIVER / SDK / HAL LAYER                             │
│                                                                             │
│   🎛️ I2C Driver     🎛️ SPI Driver     🔌 ADC Driver                          │
│   🔌 GPIO Driver    🌐 ENET Driver    💾 EEPROM Driver                       │
│                                                                             │
│                      🛠️ NXP MCUXpresso SDK & HAL                            │
└─────────────────────────────────────────────────────────────────────────────┘
```
