# STPlayer

## Work in progress

## 1.Overview
Our goal is to create an audio player for .WAV files. These files will be read from SD card and the signal will be transmitted to the micro-jack output.
## 2.Description

## 3.Tools
For our project we use STM32F407 microcontroller featuring 32-bit ARM Cortex-M4F core. The addional components are potentiometer and SDcard. We use CooCox CoIDE, Version: 1.7.8.
## 4.How to run
To run the project make sure you got:
STM32F4-DISCOVERY board,
SD Card Module and SD Card formatted to FAT32,
Headphone or loudspeaker with male jack connector,
Rotary potentiometer - linear (10k ohm).

## 5.How to compile
Connect SDcard Module:
   GND  <---> GND
   3V   <---> 3V3
   PB11 <---> CS
   PB15 <---> MOSI
   PB13 <---> SCK
   PB14 <---> MISO
   GND  <---> GND
Connect also potentiometer (GND, PA1, VDD).
Plug your SD Card with .wav files into the module.
Build this project with CooCox CoIDE and Download Code to Flash.
## 6.Future improvements

## 7.Attributions
http://elm-chan.org/fsw/ff/00index_e.html
http://www.mind-dump.net/configuring-the-stm32f4-discovery-for-audio
## 8.License
MIT license. Read license.txt for more.

## 9.Credits
Jacek Kmiecik
Michał Wypchło

The project was conducted during the Microprocessor Lab course held by the Institute of Control and Information Engineering, Poznan University of Technology.

Supervisor: Tomasz Mańkowski
