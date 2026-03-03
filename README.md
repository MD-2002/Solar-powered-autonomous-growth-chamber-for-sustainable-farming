# Solar-powered-autonomous-growth-chamber-for-sustainable-farming

Project Overview

This project presents a solar-powered smart growth chamber designed to monitor and control environmental conditions for plant growth. The system uses an Arduino-based embedded platform to collect sensor data and control actuators. A mobile application is used to monitor system parameters and interact with the chamber remotely. The solution focuses on energy efficiency, automation, and sustainable operation.

System Architecture

The system consists of three main components:

Arduino microcontroller unit

Environmental sensors and actuators

Mobile application interface

The Arduino collects data from sensors such as temperature and humidity sensors. Based on predefined thresholds, it controls devices such as fans, lights, or pumps. The mobile application communicates with the Arduino (via Bluetooth/Wi-Fi if applicable) to display real-time data and allow user control.

Hardware Components

Arduino board (specify model, e.g., Arduino Uno)

Temperature and humidity sensor (e.g., DHT11/DHT22)

Soil moisture sensor (if used)

Relay module

DC fan / LED grow light

Solar panel and battery system

Software Components

Arduino IDE for embedded programming

Arduino sketch (included in this repository)

Mobile application (Android-based)

Communication protocol (Bluetooth/Wi-Fi)

How to Run the Arduino Code

Install the Arduino IDE.

Open the provided .ino file.

Install required libraries (specify if any, e.g., DHT library).

Select the correct board and COM port.

Upload the sketch to the Arduino board.

How to Use the Mobile Application

Install the APK file on an Android device.

Enable Bluetooth/Wi-Fi on the phone.

Connect to the Arduino device.

Monitor environmental readings and control system parameters through the app interface.

Features

Real-time monitoring of environmental conditions

Automated control based on threshold values

Mobile-based remote access

Solar-powered energy system

Future Improvements

Integration of water flow sensors

Cloud data logging

Advanced analytics and reporting

Author

Marcos Almeida
Bachelor of Engineering Technology in Computer Engineering
Cape Peninsula University of Technology
