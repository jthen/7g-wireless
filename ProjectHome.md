This is a hardware and software design which turns the SteelSeries 7G into a USB, wireless, battery powered keyboard.

# Features #

  * 2.4GHz radio
  * runs off 2 AA (or AAA) alkaline or rechargeable (NiMH/NiCD) batteries
  * low power circuit allows for a battery life of several months
  * config menu mode allows changing LED brightness, radio output power and other settings
  * battery voltage measurement and reporting
  * no drivers are required because the USB dongle enumerates as a standard HID keyboard
  * the PS2 feature, the HUB and audio ports of the original 7G controller are not supported




Currently, the prototype is fully functional (I'm typing this on the new controller), but there are things that can be improved:

  * design and build a pen-drive sized USB dongle (I am currently using an impractical, large test circuit for dongle software development.)
  * add bootloaders for the dongle and controller
  * add a dynamic RF power output mode
  * channel hopping, communication currently works only on one channel
  * add shortcuts for lock/unlock keyboard, disable windows key and reading battery voltage
  * nRF PLOS/ARC stats and counters in the menu

The keyboard controller is designed around an Atmel AVR ATmega169PV microcontroller. The radio transceiver chip is the Nordic nRF24L01+. Both are powered directly from the batteries without a regulator.