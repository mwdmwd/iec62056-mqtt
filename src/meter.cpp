#include <cinttypes>
#include <cstdint>
#include <optional>
#include <string_view>

#include "config.h"
#include "logger.h"
#include "meter.h"

#define STX '\x02'
#define ETX '\x03'

#define ACK "\x06"

#define UNIT_SEPARATOR '*'

uint16_t const BAUD_RATES[] = {
    /* 0 */ 300,
    /* 1, A */ 600,
    /* 2, B */ 1200,
    /* 3, C */ 2400,
    /* 4, D */ 4800,
    /* 5, E */ 9600,
    /* 6, F */ 19200};

struct BaudSwitchParameters
{
	bool send_acknowledgement;
	std::optional<uint32_t> new_baud;
};

static BaudSwitchParameters baud_char_to_params(char identification)
{
	if(identification >= '0' && identification <= '6')        /* Mode C */
		return {true, BAUD_RATES[identification - '0']};      /* send acknowledgement, switch baud */
	else if(identification >= 'A' && identification <= 'F')   /* Mode B */
		return {false, BAUD_RATES[identification - 'A' + 1]}; /* no acknowledgement, switch baud */
	else                                                      /* possibly Mode A */
		return {false, std::nullopt};                         /* no acknowledgement, don't switch baud */
}

static bool is_valid_object_value(std::string_view value)
{
	for(size_t i = 0; i < value.size(); ++i)
	{
		if(!strchr(OBJECT_VALUE_ALLOWED_CHARS, value[i]))
			return false;
	}

	return true;
}

enum class MeterReader::Step : uint8_t
{
	Ready,
	Started,
	RequestSent,
	IdentificationRead,
	InData,
	AfterData,
};

/* status != Busy => status = Busy => continued on next line
 * status = Busy => step = Started => ... => step = AfterData => status = Ok            => status = Ready
 *                                    ... => status = ProtocolError                     => status = Ready
 *                                                            => status = ChecksumError => status = Ready
 *                                                            => status = ProtocolError => status = Ready
 */

void MeterReader::send_request()
{
	serial_.begin(INITIAL_BAUD_RATE, SERIAL_7E1); /* TX_ONLY here breaks for some reason */

	logger::debug("sending request");
	serial_.write("/?!\r\n");
	serial_.flush();

	step_ = Step::RequestSent;
}

void MeterReader::read_identification()
{
	serial_.begin(INITIAL_BAUD_RATE, SERIAL_7E1, SERIAL_RX_ONLY);
	char identification[MAX_IDENTIFICATION_LENGTH];
	size_t len = serial_.readBytesUntil('\n', identification, MAX_IDENTIFICATION_LENGTH);

	if(len < 6)
	{
		logger::err("ident too short (%u chars)", len);
		return change_status(Status::ProtocolError);
	}

	identification[len - 1] = 0; /* Remove \r and null terminate */
	logger::debug("identification=%s", identification);

	baud_char_ = identification[4];
	step_ = Step::IdentificationRead;
}

void MeterReader::switch_baud()
{
	BaudSwitchParameters params = baud_char_to_params(baud_char_);

	if(params.send_acknowledgement)
	{
		serial_.begin(INITIAL_BAUD_RATE, SERIAL_7E1, SERIAL_TX_ONLY);
		serial_.printf(ACK "0%c0\r\n", baud_char_);
		serial_.flush();
	}

	if(params.new_baud)
	{
		logger::debug("switching to %" PRIu32 "bps", *params.new_baud);
		serial_.begin(*params.new_baud, SERIAL_7E1, SERIAL_RX_ONLY);
	}
	else
	{
		serial_.begin(INITIAL_BAUD_RATE, SERIAL_7E1, SERIAL_RX_ONLY);
	}

	step_ = Step::InData;
	checksum_ = STX; /* Start with checksum=STX to avoid having to avoid xoring it */
}

void MeterReader::read_line()
{
	static char line[MAX_LINE_LENGTH];

	size_t len = serial_.readBytesUntil('\n', line, MAX_LINE_LENGTH);
	if(len == MAX_LINE_LENGTH)
	{
		logger::warn("probably truncated a line, expect a checksum error");
	}
	else if(len < 2) /* A valid line will never be shorter than this */
	{
		logger::err("read short line or timed out");
		return change_status(Status::ProtocolError);
	}

	for(size_t i = 0; i < len; ++i)
	{
		checksum_ ^= line[i];
	}
	/* readBytesUntil doesn't include the terminator, so take it into account separately */
	checksum_ ^= '\n';

	line[len - 1] = 0; /* Cut off \r before logging the line */
	logger::debug("line: %s", line);

	if(line[len - 2] == '!') /* End of data, ETX and checksum will follow */
	{
		step_ = Step::AfterData;
	}
	else /* Data line */
	{
		std::string_view line_view(line, len);
		if(line_view[0] == STX) /* The first data line starts with an STX, remove it */
			line_view.remove_prefix(1);

		auto lparen = line_view.find_first_of('(');
		auto rparen = line_view.find_last_of(')');
		if(lparen != std::string_view::npos && rparen != std::string_view::npos)
		{
			auto obis = line_view.substr(0, lparen);
			auto value = line_view.substr(lparen + 1, rparen - (lparen + 1));
			handle_object(obis, value);
		}
		else
		{
			logger::warn("improper data line format");
		}
	}
}

static void postprocess_value(std::string_view &value)
{
#ifdef STRIP_UNIT
	size_t unit_sep_pos = value.find_last_of(UNIT_SEPARATOR);
	if(unit_sep_pos != std::string_view::npos)
		value.remove_suffix(value.size() - unit_sep_pos);
#endif
}

void MeterReader::handle_object(std::string_view obis, std::string_view value)
{
	auto entry = values_.find(std::string(obis)); /* TODO avoid copy? */
	if(entry != values_.end())
	{
		postprocess_value(value);

		if(is_valid_object_value(value))
			entry->second = value;
	}
}

void MeterReader::verify_checksum()
{
	/* Expecting ETX and then the checksum */
	uint8_t etx_bcc[2];
	if(serial_.readBytes(etx_bcc, 2) != 2 || etx_bcc[0] != ETX)
	{
		logger::err("failed to read checksum");
		return change_status(Status::ProtocolError);
	}

	checksum_ ^= ETX;
	if(checksum_ != etx_bcc[1])
	{
		logger::err("checksum mismatch: %02" PRIx8 " != %02" PRIx8, checksum_, etx_bcc[1]);
		return change_status(Status::ChecksumError);
	}

	return change_status(Status::Ok); /* Data readout successful */
}

void MeterReader::change_status(Status to)
{
	if(to == Status::ProtocolError)
		++errors_;
	else if(to == Status::ChecksumError)
		++checksum_errors_;
	else if(to == Status::Ok)
		++successes_;

	status_ = to;
}

bool MeterReader::start_monitoring(std::string_view obis)
{
	/* Don't allow adding a new monitored object in the middle of a readout */
	if(status_ == Status::Busy) return false;

	auto [_, was_inserted] = values_.emplace(obis, "");
	return was_inserted;
}

bool MeterReader::stop_monitoring(std::string_view obis)
{
	/* Don't allow removing a monitored object in the middle of a readout */
	if(status_ == Status::Busy) return false;

	return values_.erase(std::string(obis)) == 1;
}

void MeterReader::start_reading()
{
	/* Don't allow starting a read when one is already in progress */
	if(status_ == Status::Busy) return;

	status_ = Status::Busy;
	step_ = Step::Started;
}

void MeterReader::loop()
{
	if(status_ != Status::Busy) return;

	switch(step_)
	{
		case Step::Ready: /* nothing to do, this should never happen */
			break;
		case Step::Started:
			send_request();
			break;
		case Step::RequestSent:
			read_identification();
			break;
		case Step::IdentificationRead:
			switch_baud();
			break;
		case Step::InData:
			read_line();
			break;
		case Step::AfterData:
			verify_checksum();
			break;
	}
}