/*
 * This file is part of the PulseView project.
 *
 * Copyright (C) 2012 Joel Holdsworth <joel@airwebreathe.org.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "util.hpp"

#include <extdef.h>

#include <assert.h>

#include <algorithm>
#include <sstream>

#include <QTextStream>
#include <QDebug>

using namespace Qt;

namespace pv {
namespace util {

static QTextStream& operator<<(QTextStream& stream, SIPrefix prefix)
{
	switch (prefix) {
	case SIPrefix::yocto: return stream << 'y';
	case SIPrefix::zepto: return stream << 'z';
	case SIPrefix::atto:  return stream << 'a';
	case SIPrefix::femto: return stream << 'f';
	case SIPrefix::pico:  return stream << 'p';
	case SIPrefix::nano:  return stream << 'n';
	case SIPrefix::micro: return stream << QChar(0x03BC);
	case SIPrefix::milli: return stream << 'm';
	case SIPrefix::kilo:  return stream << 'k';
	case SIPrefix::mega:  return stream << 'M';
	case SIPrefix::giga:  return stream << 'G';
	case SIPrefix::tera:  return stream << 'T';
	case SIPrefix::peta:  return stream << 'P';
	case SIPrefix::exa:   return stream << 'E';
	case SIPrefix::zetta: return stream << 'Z';
	case SIPrefix::yotta: return stream << 'Y';

	default: return stream;
	}
}

int exponent(SIPrefix prefix)
{
	return 3 * (static_cast<int>(prefix) - static_cast<int>(SIPrefix::none));
}

static SIPrefix successor(SIPrefix prefix)
{
	assert(prefix != SIPrefix::yotta);
	return static_cast<SIPrefix>(static_cast<int>(prefix) + 1);
}

// Insert the timestamp value into the stream in fixed-point notation
// (and honor the precision)
static QTextStream& operator<<(QTextStream& stream, const Timestamp& t)
{
	// The multiprecision types already have a function and a stream insertion
	// operator to convert them to a string, however these functions abuse a
	// precision value of zero to print all available decimal places instead of
	// none, and the boost authors refuse to fix this because they don't want
	// to break buggy code that relies on this bug.
	// (https://svn.boost.org/trac/boost/ticket/10103)
	// Therefore we have to work around the case where precision is zero.

	int precision = stream.realNumberPrecision();

	std::ostringstream ss;
	ss << std::fixed;

	if (stream.numberFlags() & QTextStream::ForceSign) {
		ss << std::showpos;
	}

	if (0 == precision) {
		ss
			<< std::setprecision(1)
			<< round(t);
	} else {
		ss
			<< std::setprecision(precision)
			<< t;
	}

	std::string str(ss.str());
	if (0 == precision) {
		// remove the separator and the unwanted decimal place
		str.resize(str.size() - 2);
	}

	return stream << QString::fromStdString(str);
}

QString format_si_value(const Timestamp& v, QString unit, SIPrefix prefix,
	unsigned int precision, bool sign)
{
	if (prefix == SIPrefix::unspecified) {
		// No prefix given, calculate it

		if (v.is_zero()) {
			prefix = SIPrefix::none;
		} else {
			int exp = exponent(SIPrefix::yotta);
			prefix = SIPrefix::yocto;
			while ((fabs(v) * pow(Timestamp(10), exp)) > 999 &&
					prefix < SIPrefix::yotta) {
				prefix = successor(prefix);
				exp -= 3;
			}
		}
	}

	assert(prefix >= SIPrefix::yocto);
	assert(prefix <= SIPrefix::yotta);

	const Timestamp multiplier = pow(Timestamp(10), -exponent(prefix));

	QString s;
	QTextStream ts(&s);
	if (sign && !v.is_zero())
		ts << forcesign;
	ts
		<< qSetRealNumberPrecision(precision)
		<< (v * multiplier)
		<< ' '
		<< prefix
		<< unit;

	return s;
}

static QString pad_number(unsigned int number, int length)
{
	return QString("%1").arg(number, length, 10, QChar('0'));
}

static QString format_time_in_full(const Timestamp& t, signed precision)
{
	const Timestamp whole_seconds = floor(abs(t));
	const Timestamp days = floor(whole_seconds / (60 * 60 * 24));
	const unsigned int hours = fmod(whole_seconds / (60 * 60), 24).convert_to<uint>();
	const unsigned int minutes = fmod(whole_seconds / 60, 60).convert_to<uint>();
	const unsigned int seconds = fmod(whole_seconds, 60).convert_to<uint>();

	QString s;
	QTextStream ts(&s);

	if (t >= 0)
		ts << "+";
	else
		ts << "-";

	bool use_padding = false;

	if (days) {
		ts << days.str().c_str() << ":";
		use_padding = true;
	}

	if (hours || days) {
		ts << pad_number(hours, use_padding ? 2 : 0) << ":";
		use_padding = true;
	}

	if (minutes || hours || days) {
		ts << pad_number(minutes, use_padding ? 2 : 0);
		use_padding = true;

		// We're not showing any seconds with a negative precision
		if (precision >= 0)
			 ts << ":";
	}

	// precision < 0: Use DD:HH:MM format
	// precision = 0: Use DD:HH:MM:SS format
	// precision > 0: Use DD:HH:MM:SS.mmm nnn ppp fff format
	if (precision >= 0) {
		ts << pad_number(seconds, use_padding ? 2 : 0);

		const Timestamp fraction = fabs(t) - whole_seconds;

		if (precision > 0 && precision < 1000) {
			std::ostringstream ss;
			ss
				<< std::fixed
				<< std::setprecision(2 + precision)
				<< std::setfill('0')
				<< fraction;
			std::string fs = ss.str();

			ts << ".";

			// Copy all digits, inserting spaces as unit separators
			for (int i = 1; i <= precision; i++) {
				// Start at index 2 to skip the "0." at the beginning
				ts << fs[1 + i];

				if ((i > 0) && (i % 3 == 0) && (i != precision))
					ts << " ";
			}
		}
	}

	return s;
}

static QString format_time_with_si(const Timestamp& t, QString unit,
	SIPrefix prefix, unsigned int precision)
{
	// The precision is always given without taking the prefix into account
	// so we need to deduct the number of decimals the prefix might imply
	const int prefix_order = -exponent(prefix);

	const unsigned int relative_prec =
		(prefix >= SIPrefix::none) ? precision :
		std::max((int)(precision - prefix_order), 0);

	return format_si_value(t, unit, prefix, relative_prec);
}

QString format_time(const Timestamp& t, SIPrefix prefix, TimeUnit unit,
	unsigned int precision)
{
	// Make 0 appear as 0, not random +0 or -0
	if (t.is_zero())
		return "0";

	// If we have to use samples then we have no alternative formats
	if (unit == TimeUnit::Samples)
		return format_time_with_si(t, "sa", prefix, precision);

	// View in "normal" range -> medium precision, medium step size
	// -> HH:MM:SS.mmm... or xxxx (si unit) if less than 60 seconds
	// View zoomed way in -> high precision (>3), low step size (<1s)
	// -> HH:MM:SS.mmm... or xxxx (si unit) if less than 60 seconds
	if (abs(t) < 60)
		return format_time_with_si(t, "s", prefix, precision);
	else
		return format_time_in_full(t, precision);
}

QString format_second(const Timestamp& second)
{
	return format_si_value(second, "s", SIPrefix::unspecified, 0, false);
}

} // namespace util
} // namespace pv
