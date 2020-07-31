SKETCH = src/main.cpp
MAIN_NAME = iec62056_mqtt

COMP_WARNINGS = -Wall -Wextra

LIBS = $(ESP_LIBS)/ESP8266WiFi $(ESP_LIBS)/ESP8266mDNS $(ESP_LIBS)/ArduinoOTA lib/pubsubclient
EXCLUDE_DIRS = lib/pubsubclient/tests

-include config.target.mk
