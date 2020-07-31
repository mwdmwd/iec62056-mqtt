#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <optional>

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <HardwareSerial.h>
#include <PubSubClient.h>

#include "config.h"
#include "logger.h"
#include "meter.h"

static WiFiClient wifi_client;
static PubSubClient mqtt(wifi_client);
static MeterReader reader(Serial);

void wifi_connect()
{
	WiFi.mode(WIFI_STA);
	WiFi.hostname(DEVICE_NAME);
	WiFi.begin(WIFI_SSID, WIFI_PASS);

	uint8_t timeout = 10;
	while(timeout-- && !WiFi.isConnected())
	{
		delay(1000);
	}

	if(!WiFi.isConnected())
	{
		ESP.reset();
	}
}

void mqtt_connect()
{
	if(!WiFi.isConnected())
	{
		wifi_connect();
	}

	while(!mqtt.connected())
	{
		mqtt.connect("", MQTT_TOPIC_PREFIX "status/LWT", 1, true, "Offline");
	}

	mqtt.publish(MQTT_TOPIC_PREFIX "status/LWT", "Online");
	mqtt.subscribe(MQTT_COMMAND_TOPIC);
}

void mqtt_callback(char *topic, byte *payload_bytes, unsigned int length)
{
	char *payload = reinterpret_cast<char *>(payload_bytes);
	/* TODO */
}

void mqtt_log(char const *level_name, char const *message)
{
	char topic[sizeof(MQTT_LOG_PREFIX) + 10]; /* 10 characters should be enough for the level */
	snprintf(topic, sizeof(topic), "%s%s", MQTT_LOG_PREFIX, level_name);
	mqtt.publish(topic, message, true);
}

void setup()
{
#ifdef LED_PIN
	/* Set up the LED pin and blink it a few times to indicate startup */
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW);
	for(size_t i = 0; i < 4; ++i)
	{
		digitalWrite(LED_PIN, !digitalRead(LED_PIN));
		delay(200);
	}
#endif

	Serial.setTimeout(SERIAL_TIMEOUT);

	wifi_connect();

	ArduinoOTA.setHostname(DEVICE_NAME);
	ArduinoOTA.begin();

	mqtt.setServer(MQTT_SERVER_ADDRESS, MQTT_SERVER_PORT);
	mqtt.setCallback(mqtt_callback);
	mqtt_connect();

	logger::set_message_sink(mqtt_log);
	logger::set_level(logger::Level::Info);
}

void do_background_tasks()
{
	ArduinoOTA.handle();

	if(!mqtt.loop()) /* PubSubClient::loop() returns false if not connected */
	{
		mqtt_connect();
	}
}

void delay_handle_background(long time)
{
	long const increment = 5;
	while(time -= increment > 0)
	{
		do_background_tasks();
		delay(increment);
	}
}

void loop()
{
	static uint32_t next_delay;
	do_background_tasks();

	reader.loop();
	MeterReader::Status status = reader.status();

	if(status == MeterReader::Status::Ready)
	{
		next_delay = 0;
		reader.start_reading();
	}
	else if(status == MeterReader::Status::Ok)
	{
		next_delay = READ_DELAY;
		char topic[sizeof(MQTT_OBIS_PREFIX) + MAX_OBIS_CODE_LENGTH];
		strcpy(topic, MQTT_OBIS_PREFIX);

		char *obis_start = &topic[sizeof(MQTT_OBIS_PREFIX) - 1];
		for(auto const &entry : reader.values())
		{
			strlcpy(obis_start, entry.first.c_str(), MAX_OBIS_CODE_LENGTH + 1);
			mqtt.publish(topic, entry.second.c_str(), true);
		}

		reader.acknowledge();
	}
	else if(status != MeterReader::Status::Busy) /* Not Ready, Ok or Busy => error */
	{
		if(!next_delay) next_delay = READ_DELAY;
		next_delay *= 2;
		if(next_delay > 60 * 1000)
		{
			next_delay = 60 * 1000;
		}
		logger::warn("backoff (status=%u): %" PRIu32, status, next_delay);

		reader.acknowledge();
	}

	/* If we didn't just complete a read, skip logging and LED blinking */
	if(!next_delay) return;

	size_t successes = reader.successes();
	size_t errors = reader.errors();
	size_t checksum_errors = reader.checksum_errors();
	logger::info("read ok=%zu, fail=%zu, checksum fail=%zu; rssi=%" PRIi32,
	             successes, errors, checksum_errors, WiFi.RSSI());

	uint32_t free_heap;
	uint16_t max_block;
	uint8_t heap_frag;
	ESP.getHeapStats(&free_heap, &max_block, &heap_frag);
	logger::debug("heap free=%" PRIu32 ", frag=%" PRIu16 ", max blk=%" PRIu8,
	              free_heap, heap_frag, max_block);

#ifdef LED_PIN
	digitalWrite(LED_PIN, HIGH);

	delay_handle_background(25);
	if(status == MeterReader::Status::Ok)
		digitalWrite(LED_PIN, LOW);

	if(next_delay >= 25)
		delay_handle_background(next_delay - 25);
	digitalWrite(LED_PIN, LOW);
#else
	delay_handle_background(next_delay);
#endif
}
