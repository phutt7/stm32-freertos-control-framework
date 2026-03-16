# Deterministic Multi-Task Embedded Control Framework
### STM32F401RE + FreeRTOS

A production-grade embedded firmware project demonstrating deterministic real-time control using FreeRTOS on an STM32 Nucleo board. Built as a portfolio project to showcase embedded systems architecture skills including RTOS task management, ISR-driven ADC sampling, inter-task communication, hardware fault detection, and watchdog supervision.

---

## What It Does

Reads an analog input (potentiometer simulating a sensor) at exactly 1kHz via a hardware timer ISR, processes the samples through a three-task FreeRTOS pipeline, and controls a relay output based on a configurable threshold — with live UART telemetry and hardware watchdog supervision throughout.

**Demo behavior:**
- Pot turned above halfway → green LED on, relay off (normal state)
- Pot turned below halfway → green LED off, relay energizes (fault/trip state)
- UART console streams live ADC value, overflow count, and stack headroom every 500ms

---

## Hardware

| Component | Part | Pin |
|---|---|---|
| MCU | STM32F401RE Nucleo | — |
| Analog input | 10kΩ potentiometer | PA1 (A1) |
| Relay module | AEDIKO 5V optocoupler relay | PB0 (A3) |
| Status LED | Onboard LD2 | PA5 |
| Telemetry | USB-UART (ST-Link) | USART2 |

---

## Firmware Architecture

```
Timer ISR (TIM2 @ 1kHz)
        │
        ▼
  [Ring Buffer]  ← lock-free, static allocation, no malloc
        │
        ▼
  Task A – Data Processing  (osPriorityNormal)
  • Drains ring buffer
  • Averages 16 samples
  • Stuck-value fault detection (0xDEAD sentinel)
  • Pushes average to FreeRTOS queue
        │
        ▼
  Task B – Control Logic  (osPriorityHigh)
  • Reads queue with 100ms timeout
  • Drives relay and LED based on threshold
  • Kicks IWDG watchdog every cycle
        │
  Task C – UART Logging  (osPriorityLow)
  • Reads shared volatile for latest ADC average
  • Transmits telemetry every 500ms
  • Reports stack high-water mark
```

---

## Key Design Decisions

**Static ring buffer — no heap allocation**
The ISR writes directly to a statically allocated circular buffer. No malloc, no fragmentation risk, bounded worst-case execution time.

**Priority inversion prevention**
Task B (control) runs at high priority and is the only task that kicks the watchdog. If Task B stalls for any reason, the IWDG resets the system within ~2 seconds.

**Stuck-value fault detection**
If Task A sees the same averaged ADC reading 10 consecutive times, it flags `overflowCount = 0xDEAD` and forces the relay to a safe state. This catches disconnected or frozen sensors.

**Queue overflow tracking**
If Task B can't consume queue items fast enough, `overflowCount` increments. A non-zero overflow count at runtime indicates a scheduling problem.

---

## UART Telemetry Output

Connect at **115200 baud** on the ST-Link virtual COM port.

```
ADC:412  OVFL:0 STK:130
ADC:1843 OVFL:0 STK:130
ADC:2901 OVFL:0 STK:130
ADC:4021 OVFL:0 STK:130
```

- `ADC` — 16-sample rolling average (0–4095)
- `OVFL` — queue overflow counter (0 = healthy, 0xDEAD = sensor fault)
- `STK` — Task C stack headroom in words (stable = no stack leak)

---

## Build Environment

- STM32CubeIDE 2.1.0
- FreeRTOS 10.3.1 (CMSIS-RTOS V2)
- STM32F4xx HAL
- arm-none-eabi-gcc 14.3.1

**CubeMX Configuration:**
- TIM2: Prescaler=83, Period=999 → 1kHz at 84MHz
- ADC1: Channel 1, single conversion, software trigger
- IWDG: Prescaler=16, Reload=4095 (~2s timeout)
- SYS Timebase: TIM1 (required when FreeRTOS uses SysTick)

---

## File Structure

```
Core/
  Inc/
    main.h          — pin defines
    ring_buffer.h   — RingBuffer_t typedef and API
  Src/
    main.c          — peripheral init, scheduler launch
    freertos.c      — all three tasks + MX_FREERTOS_Init
    ring_buffer.c   — circular buffer implementation
    stm32f4xx_it.c  — TIM2_IRQHandler (ADC sampling)
```

---

## Upgrade Path

To adapt this for a real current sensor (ACS712 or similar):
1. Change ADC channel in CubeMX to match sensor output pin
2. Add a voltage divider if sensor outputs 5V (STM32 ADC is 3.3V max)
3. Update `ADC_THRESHOLD` to the milliamp trip point mapped to ADC counts
4. No firmware architecture changes required

---

## Skills Demonstrated

- FreeRTOS task creation, priority scheduling, queue-based IPC
- Hardware timer ISR at deterministic 1kHz rate
- Lock-free ring buffer for ISR-to-task data transfer
- IWDG watchdog integration with task-level supervision
- Stack high-water mark monitoring
- Fault detection with sentinel values
- STM32 HAL: ADC, GPIO, UART, TIM, IWDG
- STM32CubeMX peripheral configuration
