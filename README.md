# Study-Safe-Device

## Description
The **Study Safe** is an anti-theft device designed to help protect personal belongings, such as laptops and phones, from theft in public spaces like university libraries. The device uses an **Arduino** microcontroller, an **accelerometer**, and a **buzzer** to detect movement and alert users when their belongings are being stolen. A **real-time web interface** allows users to monitor and control the system, while **Twilio SMS API** sends notifications to users when an alarm is triggered.

## Project Images and Schematic

### Hardware Setup
Below is a picture of the final **Study Safe** device and its connection to the smartphone interface.

![Study Safe Device](https://github.com/user-attachments/assets/9e1b9b4f-c469-45cd-9420-67b088ab3e36)


### Circuit Schematic
This schematic shows how the **accelerometer**, **buzzer**, and **Arduino** are connected together to detect movement and trigger the alarm.

![Schematic Diagram](https://github.com/user-attachments/assets/6ddf26ab-7250-467f-8405-26746c086483)

## Features
- **Movement Detection**: Uses an accelerometer to detect changes in position.
- **Real-time Alerts**: Sends notifications to users via SMS using Twilio API when unauthorized movement is detected.
- **Alarm System**: A buzzer emits a loud siren to alert those nearby if theft is detected.
- **User Interface**: A responsive web interface built with **HTML**, **CSS**, and **JavaScript** to control the device remotely.
- **Wi-Fi Connectivity**: The system connects to the internet via Wi-Fi, enabling remote monitoring and control through a server running on **Node.js**.

## Components
- **Arduino**: The core microcontroller to process the sensor data and control the alarm system.
- **Accelerometer**: Detects movement and changes in position of the belongings.
- **Buzzer**: Emits a siren when movement is detected.
- **Node.js & Express**: Backend to handle communication between the user interface and Arduino.
- **WebSocket API**: Real-time communication between the server and the device.
- **Twilio API**: Sends SMS alerts to users when an alarm is triggered.
