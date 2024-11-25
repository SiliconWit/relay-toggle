# Relay Toggle Web Interface

A simple web interface to control an Arduino relay remotely using a SIM800L GSM module.

## Features
- Clean web interface with status indicator
- Toggle button for relay control
- Mobile-friendly design
- Works with Arduino + SIM800L

## Setup
1. Upload the Arduino code to your board
2. Connect the SIM800L module and relay
3. Access the control interface at: https://siliconwit.github.io/relay-toggle

## Hardware Required
- Arduino (Uno/Nano/etc)
- SIM800L GSM Module
- Relay Module
- SIM card with data plan

## Pin Connections
- SIM800L RX -> Arduino D2
- SIM800L TX -> Arduino D3
- Relay Signal -> Arduino D7

## Repository Structure
- `index.html` - Web interface
- `arduino/` - Arduino code
