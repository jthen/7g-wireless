TARGET  = 7G_dngl
DEVICE  = atmega328p
F_CPU   = 12000000
AVRDUDE = avrdude -c stk500v2 -P avrdoper -B1 -p $(DEVICE)

CFLAGS  = -I. -Iusbdrv -DIS_DONGLE=1 -flto
#CFLAGS += -DDBGPRINT

LFLAGS  = -Wl,--relax -flto
# LFLAGS += -u vfprintf -lprintf_min

OBJECTS = $(TARGET).o 7G_vusb.o nRF24L01.o rf_dngl.o rf_addr.o text_message.o usbdrv/usbdrv.o usbdrv/usbdrvasm.o
OBJECTS += avr_serial.o

COMPILE = avr-gcc -Wall -Os -DF_CPU=$(F_CPU) $(CFLAGS) -mmcu=$(DEVICE)

hex: $(TARGET).hex

$(TARGET).hex: $(OBJECTS)
	$(COMPILE) -o $(TARGET).elf $(OBJECTS) $(LFLAGS)
	rm -f $(TARGET).hex $(TARGET).eep.hex
	avr-objcopy -j .text -j .data -O ihex $(TARGET).elf $(TARGET).hex
	avr-size -C --mcu $(DEVICE) $(TARGET).elf

flash: $(TARGET).hex
	$(AVRDUDE) -V -U flash:w:$(TARGET).hex:i

clean:
	rm -f *.hex *.elf $(OBJECTS)

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@

.c.o:
	$(COMPILE) -c $< -o $@
