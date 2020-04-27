import spidev

spi = spidev.SpiDev()
spi.open(0, 1)
print(spi.xfer2([0xD0, 0x00], 50000, 5, 8))

