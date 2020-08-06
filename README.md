# iec62056-mqtt
Electricity meter -> MQTT bridge for ESP8266. Supports data readout of meters using IEC62056-21 (aka IEC61107, IEC1107) in mode C (and possibly A and B as well).

## Requirements
- [ESP8266 Arduino core][arduino8266] v3.0 or newer (with C++17 support). If v3.0 hasn't been released yet, use the latest git version.
- [PubSubClient][psc] v2.8 or newer. A suitable version is included as a submodule and will be used automatically if you use makeEspArduino to build.
- [Hardware](hw) - something to run the software on, as well as an optical reading head
- an electricity meter with an optical port on the front (it looks something like [this][opticalport])

## Configuration
Enter the `src` directory and make a copy of `example_config.h` called `config.h`. Adjust the settings it contains. Note that you will probably need to do this at least 2 times: once to get your reader connected to WiFi and MQTT and see what objects your meter makes available, and again to program your desired list of objects to export over MQTT into the reader (you can use ArduinoOTA to do this wirelessly). Currently, all configuration is done at compile time.

## Building (with [makeEspArduino][mkesp])
Specify your board and upload settings in `config.target.mk` (use the example as a reference) and run `espmake`.

## Building (with Arduino IDE)
Install the PubSubClient library into your IDE. Open `src/src.ino`. Proceed as usual.

... todo ...

[arduino8266]: https://github.com/esp8266/Arduino
[psc]: https://github.com/knolleary/pubsubclient
[mkesp]: https://github.com/plerup/makeEspArduino

[opticalport]: https://upload.wikimedia.org/wikipedia/commons/f/f4/IEC62056-Voorbeeld1.png