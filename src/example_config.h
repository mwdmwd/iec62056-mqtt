#ifndef IEC62056_MQTT_CONFIG_H
#define IEC62056_MQTT_CONFIG_H

#include <cstdint>

/* Optional: pin number of indicator LED. Comment out to disable it.
 * The indicator LED will flash quickly after a successful read, and stay on for
 * longer to signal an error. */
#define LED_PIN 4

/* WiFi credentials */
char const *const WIFI_SSID = "Your WiFi network name here!";
char const *const WIFI_PASS = "Your WiFi password here!";

/* MQTT server address and port */
char const *const MQTT_SERVER_ADDRESS = "192.168.1.2";
uint16_t const MQTT_SERVER_PORT = 1883;

/* Used as a prefix for MQTT topics (by default) as well as the hostname */
#define DEVICE_NAME "elec"

/* Objects to export over MQTT. This list is just an example, your meter might
 * not provide some of these. */
char const *const EXPORT_OBJECTS[] = {
    "15.7.0",                     // Absolute active instantaneous power, sum of all phases [kW]
    "15.8.1", "15.8.2",           // Absolute active energy, tariffs 1, 2 [kWh]
    "32.7.0", "52.7.0", "72.7.0", // Voltage, each phase [V]
    "31.7.0", "51.7.0", "71.7.0", // Current, each phase [A]
};

/* Uncomment to strip the unit before publishing values. For example,
 * "230.5" instead of "230.5*V" */
// #define STRIP_UNIT

/* Uncomment to override automatic mode selection, for example to limit the baud
 * rate. Use if you have problems with your optical receiver. */
// #define MODE_OVERRIDE '4'

/* Additional delay between reads. Every time there's an error, this delay is
 * doubled, up to a maximum of 60 seconds. A successful read resets it to the
 * specified value. */
uint32_t const READ_DELAY = 1000; /* ms */

/* MQTT topics and topic prefixes */
#define MQTT_TOPIC_PREFIX DEVICE_NAME "/"
#define MQTT_LOG_PREFIX MQTT_TOPIC_PREFIX "log/"
#define MQTT_COMMAND_TOPIC MQTT_TOPIC_PREFIX "cmd"
#define MQTT_OBIS_PREFIX MQTT_TOPIC_PREFIX "obis/"

/* Default log level. Allowed values: None < Error < Warning < Info < Debug */
#define DEFAULT_LOG_LEVEL Info

/* An additional layer of protection against bit flips: the values of all exported
 * objects are checked, and if they contain any characters other than these, the
 * the newly-read value is discarded. This might not be needed if your optical reading
 * head is very well-protected from outside light, but since the checksum is only
 * 1 byte, it might be worth keeping. */
char const *const OBJECT_VALUE_ALLOWED_CHARS = "0123456789.,:-";

/* How long to wait for the meter to send responses to our requests. You shouldn't
 * need to change this. */
uint32_t const SERIAL_TIMEOUT = 2000; /* ms */

#endif
