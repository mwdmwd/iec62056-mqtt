#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "logger.h"

namespace logger
{
MessageSink message_sink = [](...) {}; /* Throw away all messages by default */
TimestampSource timestamp_source;
Level log_level = Level::None;

void set_message_sink(MessageSink sink)
{
	message_sink = sink;
}

void set_timestamp_source(TimestampSource source)
{
	timestamp_source = source;
}

void set_level(Level level)
{
	log_level = level;
}

static void log(char const *level_name, char const *fmt, va_list as)
{
	char buf[MAX_MESSAGE_LENGTH];
	size_t offset = 0;
	if(timestamp_source)
		offset = snprintf(buf, MAX_MESSAGE_LENGTH, "[%zu] ", timestamp_source());
	vsnprintf(&buf[offset], MAX_MESSAGE_LENGTH - offset, fmt, as);
	message_sink(level_name, buf);
}

#define DEFINE_LOGGING_FUNCTION(name, level) \
	void name(char const *fmt, ...)          \
	{                                        \
		if(log_level < level) return;        \
                                             \
		va_list as;                          \
		va_start(as, fmt);                   \
		log(#name, fmt, as);                 \
		va_end(as);                          \
	}

DEFINE_LOGGING_FUNCTION(err, Level::Error);
DEFINE_LOGGING_FUNCTION(warn, Level::Warning);
DEFINE_LOGGING_FUNCTION(info, Level::Info);
DEFINE_LOGGING_FUNCTION(debug, Level::Debug);
}