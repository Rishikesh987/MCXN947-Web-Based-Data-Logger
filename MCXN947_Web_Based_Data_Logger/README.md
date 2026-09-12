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

┌──────────────────────────────────────────────┐
│              APPLICATION LAYER               │
│                                              │
│  Application Task                            │
│  • System state / business logic             │
│  • Process web commands                      │
│  • Decide actuator actions                   │
│  • Prepare data for logging                  │
└──────────────────────┬───────────────────────┘
                       │
┌──────────────────────▼───────────────────────┐
│                 SERVICE LAYER                │
│                                              │
│  Sensor Service                              │
│  • Digital / Analog acquisition              │
│  • Filtering / averaging                     │
│  • Sensor status / validation                │
│                                              │
│  Logger Service                              │
│  • Data buffering                            │
│  • Timestamp association                     │
│  • EEPROM management                         │
│                                              │
│  RTC Service                                 │
│  • DS3231 time/date                          │
│  • Timestamp generation                      │
│                                              │
│  Network / Web Service                       │
│  • lwIP TCP/IP                               │
│  • HTTP server                               │
│  • Web commands / data                       │
│                                              │
│  Indicator Service                           │
│  • System status indication                  │
└──────────────────────┬───────────────────────┘
                       │
┌──────────────────────▼───────────────────────┐
│             DRIVER / SDK / HAL               │
│                                              │
│  I2C Driver    SPI Driver    ADC Driver      │
│  GPIO Driver  ENET Driver   EEPROM Driver    │
│                                              │
│             NXP MCUXpresso SDK               │
└──────────────────────────────────────────────┘