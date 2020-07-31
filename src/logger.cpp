#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "logger.h"

namespace logger
{
MessageSink message_sink = [](...) {}; /* Throw away all messages by default */
Level log_level = Level::None;

void set_message_sink(MessageSink sink)
{
	message_sink = sink;
}

void set_level(Level level)
{
	log_level = level;
}

static void log(char const *level_name, char const *fmt, va_list as)
{
	char buf[MAX_MESSAGE_LENGTH];
	vsnprintf(buf, MAX_MESSAGE_LENGTH, fmt, as);
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