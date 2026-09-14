# **MCXN947_Web_Based_Data_Logger**

MCXN947-based IoT embedded system using FreeRTOS and lwIP Ethernet networking, with a web-based interface for real-time sensor monitoring, RTC timestamped data logging, EEPROM historical storage, and remote control, implemented using a layered driver/service/application architecture.


## Key Features

* 🌐 **Ethernet-based IoT communication:** Reliable data transfer over physical networks.
* 🖥️ **Embedded Web Server:** Serves dashboard interfaces running directly on the MCXN947.
* 📡 **TCP/IP networking using lwIP:** Optimized lightweight embedded protocol stack suite.
* ⚙️ **FreeRTOS-based multitasking:** Fully deterministic scheduling across all application components.
* 🌡️ **Real-Time Monitoring:** Dual temperature and pressure monitoring via BMP280/BME280.
* 🕒 **RTC Timestamping:** High-accuracy chronological indexing via an external DS3231.
* 💾 **Historical Sensor Data Logging:** Secure, permanent storage logging to an external EEPROM.
* 📊 **Web-Based Visualization:** Browser-native real-time charts without needing a dedicated PC utility.
* 🔄 **Bi-Directional Communication:** Real-time data sync and web interface control parameters.
* 🎛️ **Remote Control Interfacing:** Trigger and drive hardware actuators directly from web dashboards.
* 💡 **Dedicated Indicator Service:** Modular system health monitoring using visual LED blink status algorithms.
* 🔌 **Custom Hardware Abstraction:** Modular custom I²C and GPIO peripheral driver frameworks.
* 🧩 **Layered Software Architecture:** Complete modular separation of tasks:
  * 🖥️ `Application Layer` — Business logic & system state management.
  * ⚙️ `Service Layer` — Protocol processing, caching, and data translation buffers.
  * 🔌 `Driver / SDK / HAL` — Direct register-level NXP MCUXpresso interaction.
* 🔧 **Modular Frameworks:** Reusable and decoupled architecture designed for painless maintenance blocks.
* ⏱️ **Periodic Timing Execution:** High-precision thread interval execution cycles using FreeRTOS.
* 🛠️ **NXP MCUXpresso Integration:** Direct integration into official SDK environments.
* 🔐 **Static IPv4 Addressing:** Secure, deterministic static IP configuration layout parameters.


## ***🧱 Software Architecture*** 

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                             APPLICATION LAYER                               │
│                                                                             │
│  Application Task (Central Business Logic Thread)                           │
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
│  • Coordinates digital & analog sensor data acquisition                     │
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
│   🎛️ I2C Driver     🎛️ SPI Driver     🔌 ADC Driver                         │
│   🔌 GPIO Driver    🌐 ENET Driver    💾 EEPROM Driver                      │
│                                                                             │
│                      🛠️ NXP MCUXpresso SDK & HAL                            │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 🗺️ Project Status & Roadmap

### ✅ Completed Tasks (What is Done)
- [x] **Layered Software Architecture:** Complete decoupling across Application, Service, and Driver/HAL modules.
- [x] **Multitasking Kernel:** Deterministic FreeRTOS configuration executing periodic thread cycles.
- [x] **TCP/IP Network Stack:** Integration of the `lwIP` protocol suite with static IPv4 addressing.
- [x] **Dual Sensor Framework:** Custom I²C peripheral drivers for temperature and pressure acquisition (BMP280/BME280).
- [x] **Chronological Indexing:** DS3231 external RTC tracking service supplying timestamp arrays.
- [x] **Data Logging Buffers:** Routed `Sensor Service` outputs into internal RAM circular buffers managed by the `Logger Service`.
- [x] **EEPROM Storage Scheduler:** Completed page-write scheduling logic inside the `Logger Service` for persistent logging to external EEPROM.
- [x] **System Diagnostics:** Asynchronous system health tracking mapped via modular LED status blink codes.

### ⏳ Pending Actions (What is Remaining)
- [ ] **HTTP Daemon Processing:** Complete the background JSON parsing engine for browser-to-MCU data streams.
- [ ] **Bi-Directional Remote Control:** Wire incoming web dashboard packets to physical actuator/motor GPIO states.
- [ ] **Web Data Visualizer:** Build the HTML5/JavaScript dashboard files to render real-time graphs.

---
