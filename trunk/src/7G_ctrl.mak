TARGET  = 7G_ctrl
DEVICE  = atmega169p
#F_CPU   = 1000000
F_CPU   = 3686400
AVRDUDE = avrdude -c dragon_isp -P usb -B2 -p $(DEVICE)

# fuses for the ATmega169PV
FUSEH   = 0x91
FUSEL   = 0xfd
FUSEX   = 0xff

CFLAGS  = -I. -Wall -Os -flto
#CFLAGS += -DDBGPRINT

LFLAGS  = -Wl,--relax -flto
#LFLAGS += -u vfprintf -lprintf_min

COMPILE = avr-gcc -mmcu=$(DEVICE) -DF_CPU=$(F_CPU) $(CFLAGS)

OBJECTS = $(TARGET).o nRF24L01.o matrix.o led.o rf_ctrl.o rf_addr.o sleeping.o ctrl_settings.o
# avr_serial.c contains debugging helper functions which should
# not be included in the final version
OBJECTS += avr_serial.o

hex: $(TARGET).hex

$(TARGET).hex: $(OBJECTS)
	$(COMPILE) -o $(TARGET).elf $(OBJECTS) $(LFLAGS)
	rm -f $(TARGET).hex $(TARGET).eep.hex
	avr-objcopy -j .text -j .data -O ihex $(TARGET).elf $(TARGET).hex
	avr-size -C --mcu=$(DEVICE) $(TARGET).elf

flash: $(TARGET).hex
	$(AVRDUDE) -V -U flash:w:$(TARGET).hex:i

clean:
	rm -f $(TARGET).hex $(TARGET).elf $(OBJECTS)

fuse:
	$(AVRDUDE) -U hfuse:w:$(FUSEH):m -U lfuse:w:$(FUSEL):m -U efuse:w:$(FUSEX):m

read_fuse:
	$(AVRDUDE) -U hfuse:r:-:h -U lfuse:r:-:h -U efuse:r:-:h

# $(COMPILE) -S -fverbose-asm $< -o $@

.c.o:
	$(COMPILE) -c $< -o $@
