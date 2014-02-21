TARGET   = 7G_dngl_nrf.ihx
CFLAGS   = --model-small -I../../common -I.. -DNRF24LU1
LFLAGS   = --code-loc 0x0000 --code-size 0x4000 --xram-loc 0x8000 --xram-size 0x800
ASFLAGS  = -plosgff
RELFILES = main.rel usb_desc.rel nrf_utils.rel text_message.rel rf_dngl.rel usb.rel crtxinit.rel

VPATH    = ../../common:../

$(TARGET): $(RELFILES)
	sdcc $(CFLAGS) $(LFLAGS) $(RELFILES) -o $(TARGET)

%.rel: %.c
	sdcc $(CFLAGS) -c $<

%.rel: %.asm
	sdas8051 $(ASFLAGS) $@ $< 

clean:
	rm -f *.ihx *.lnk *.lst *.map *.rel *.rst *.sym *.mem *.lk *.asm *.lk *.cdb *.omf

all: clean $(TARGET)

flash: clean $(TARGET)
	nrfburn -f 16 -w $(TARGET)
