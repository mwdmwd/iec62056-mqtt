#ifndef IEC62056_MQTT_LOGGER_H
#define IEC62056_MQTT_LOGGER_H

#include <functional>

size_t const MAX_MESSAGE_LENGTH = 256;

namespace logger
{
using MessageSink = std::function<void(char const *level_name, char const *message)>;
enum class Level
{
	None,
	Error,
	Warning,
	Info,
	Debug,
};

void set_message_sink(MessageSink sink);
void set_level(Level level);

void err(char const *fmt, ...);
void warn(char const *fmt, ...);
void info(char const *fmt, ...);
void debug(char const *fmt, ...);
}

#endif