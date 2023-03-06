/* An Alternative Software Serial Library
 * http://www.pjrc.com/teensy/td_libs_AltSoftSerial.html
 * Copyright (c) 2014 PJRC.COM, LLC, Paul Stoffregen, paul@pjrc.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef AltSoftSerial_h
#define AltSoftSerial_h

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/power.h>

#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>

#define ALTSS_BASE_FREQ F_CPU

class AltSoftSerial
{
public:
	AltSoftSerial() {}
	~AltSoftSerial() { end(); }
	static void begin(uint32_t baud) { init((ALTSS_BASE_FREQ + baud / 2) / baud); }
	static void end();
	int peek();
	int read();
	int available();
	int availableForWrite();
	size_t write(uint8_t byte)
	{
		writeByte(byte);
		return 1;
	}
	void flush() { flushOutput(); }
	static void flushInput();
	static void flushOutput();
	bool listen() { return false; }
	bool isListening() { return true; }
	bool overflow()
	{
		bool r = timing_error;
		timing_error = false;
		return r;
	}
	static int library_version() { return 1; }
	static void enable_timer0(bool enable) { (void)enable; }
	static bool timing_error;

private:
	static void init(uint32_t cycles_per_bit);
	static void writeByte(uint8_t byte);
};

#endif
