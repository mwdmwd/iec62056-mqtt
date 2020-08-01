# Hardware
## Electronics
- ESP8266 or ESP8266-based board (NodeMCU or similar)
- IR-sensitive phototransistor, for example BPW96B
- IR LED, along with a suitable resistor
- PNP transistor
- Resistor or potentiometer for the receiving circuit

The exact wavelength of the IR LED and phototransistor shouldn't matter, as long as it is between 800nm and 1000nm.

Here's a [schematic][ir-rxtx] of an infrared transmitter and receiver that will work for this purpose, along with an
[archived copy][ir-rxtx-archive]. Of course, you can use a different circuit, as long as:

- the transmit LED lights up when the TX pin of the ESP8266 is LOW, and
- the receiver pulls the RX pin LOW when it senses infrared light.

I found that the circuit linked above requires very precise selection of the pullup resistor value. A good approach is to replace the resistor with a potentiometer and keep on turning it until the receiver starts to work. Also, I got better results (less checksum errors) with a bare ESP8266 module (ESP-12E) than a NodeMCU. This may be caused by the NodeMCU's USB to serial chip interfering somehow(?)

## Reading head case
[reading_head.scad](reading_head.scad) is a customizable OpenSCAD model of a case for the optical reading head. It consists of 2 parts, one to hold the LED and phototransistor and another to seal up the back and protect against outside light. One way of holding it to the meter is to cut a ring out of a fridge magnet or similar device and glue it to the front of the reading head. Commercial reading heads also use magnets to attach to the meter.


[ir-rxtx]: https://nrqm.ca/nrqm.ca/2011/08/transmitting-uart-serial-over-infrared/index.html
[ir-rxtx-archive]: https://web.archive.org/web/20180812180402/http://nrqm.ca/2011/08/transmitting-uart-serial-over-infrared/