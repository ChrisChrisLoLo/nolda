import board
import busio
import displayio
import framebufferio
import digitalio
import sharpdisplay
import terminalio
import fontio
from adafruit_display_text.label import Label
from terminalio import FONT

# Release the existing display, if any
displayio.release_displays()
spi = busio.SPI(board.GP10, MOSI=board.GP11)
chip_select_pin = board.GP12
# Select JUST ONE of the following lines:
# For the 400x240 display (can only be operated at 2MHz)
framebuffer = sharpdisplay.SharpMemoryFramebuffer(spi, chip_select_pin, 128, 128)
# For the 144x168 display (can be operated at up to 8MHz)
#framebuffer = sharpdisplay.SharpMemoryFramebuffer(bus, chip_select_pin, width=144, height=168, baudrate=8000000)

display = framebufferio.FramebufferDisplay(framebuffer)
print(dir(display))


# bitmap = displayio.Bitmap(128, 128, 2)
# palette = displayio.Palette(2)
# tilegrid = displayio.TileGrid(bitmap, pixel_shader=palette)

# group = displayio.Group()
# group.append(tilegrid)
# display.show(group)

label = Label(font=FONT, text="one\ntwo\nthree", x=20, y=20, scale=4, line_spacing=1.2)
while True:
    display.show(label)