# color_setup.py Customise for your hardware config

# Released under the MIT License (MIT). See LICENSE.
# Copyright (c) 2020 Peter Hinch

# As written, supports:
# Adafruit 1.5" 128*128 OLED display: https://www.adafruit.com/product/1431
# Adafruit 1.27" 128*96 display https://www.adafruit.com/product/1673
# Edit the driver import for other displays.

# Demo of initialisation procedure designed to minimise risk of memory fail
# when instantiating the frame buffer. The aim is to do this as early as
# possible before importing other modules.

# WIRING (Adafruit pin nos and names).
# Pyb   SSD
# 3v3   Vin (10)
# Gnd   Gnd (11)
# Y1    DC (3 DC)
# Y2    CS (5 OC OLEDCS)
# Y3    Rst (4 R RESET)
# Y6    CLK (2 CL SCK)
# Y8    DATA (1 SI MOSI)

import machine
from machine import SoftSPI, Pin
import gc

# *** Choose your color display driver here ***
# Driver supporting non-STM platforms
# from drivers.ssd1351.ssd1351_generic import SSD1351 as SSD

# STM specific driver
from drivers.sharp.sharp import SHARP

# height = 96  # 1.27 inch 96*128 (rows*cols) display
height = 128 # 1.5 inch 128*128 display
width = 128

pin_cs = machine.Pin(12, machine.Pin.OUT, value=1)
vcom_pin = machine.Pin(13, machine.Pin.OUT, value=0)
#extmode_pin = machine.Pin(14, machine.Pin.OUT_PP, value=0)
#spi = machine.SPI(1)
# Not actually using pin 16
# spi = SoftSPI(baudrate=100_000, polarity=1, phase=0, sck=Pin(10), mosi=Pin(11), miso=Pin(16), firstbit=machine.SPI.LSB)
spi = SoftSPI(baudrate=100_000, polarity=1, phase=0, sck=Pin(10), mosi=Pin(11), miso=Pin(16))

gc.collect()  # Precaution before instantiating framebuf
ssd = SHARP(spi, pin_cs, height, width, False)  # Create a display instance
