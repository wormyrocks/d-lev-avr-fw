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

#define ALTSS_USE_TIMER3
#define INPUT_CAPTURE_PIN		13 // receive
#define OUTPUT_COMPARE_A_PIN		5 // transmit

#define CONFIG_TIMER_NOPRESCALE()	(TIMSK3 = 0, TCCR3A = 0, TCCR3B = (1<<ICNC3) | (1<<CS30))
#define CONFIG_TIMER_PRESCALE_8()	(TIMSK3 = 0, TCCR3A = 0, TCCR3B = (1<<ICNC3) | (1<<CS31))
#define CONFIG_TIMER_PRESCALE_256()	(TIMSK3 = 0, TCCR3A = 0, TCCR3B = (1<<ICNC3) | (1<<CS32))
#define CONFIG_MATCH_NORMAL()		(TCCR3A = TCCR3A & ~((1<<COM3A1) | (1<<COM3A0)))
#define CONFIG_MATCH_TOGGLE()		(TCCR3A = (TCCR3A & ~(1<<COM3A1)) | (1<<COM3A0))
#define CONFIG_MATCH_CLEAR()		(TCCR3A = (TCCR3A | (1<<COM3A1)) & ~(1<<COM3A0))
#define CONFIG_MATCH_SET()		(TCCR3A = TCCR3A | ((1<<COM3A1) | (1<<COM3A0)))
#define CONFIG_CAPTURE_FALLING_EDGE()	(TCCR3B &= ~(1<<ICES3))
#define CONFIG_CAPTURE_RISING_EDGE()	(TCCR3B |= (1<<ICES3))
#define ENABLE_INT_INPUT_CAPTURE()	(TIFR3 = (1<<ICF3), TIMSK3 = (1<<ICIE3))
#define ENABLE_INT_COMPARE_A()	(TIFR3 = (1<<OCF3A), TIMSK3 |= (1<<OCIE3A))
#define ENABLE_INT_COMPARE_B()	(TIFR3 = (1<<OCF3B), TIMSK3 |= (1<<OCIE3B))
#define DISABLE_INT_INPUT_CAPTURE()	(TIMSK3 &= ~(1<<ICIE3))
#define DISABLE_INT_COMPARE_A()	(TIMSK3 &= ~(1<<OCIE3A))
#define DISABLE_INT_COMPARE_B()	(TIMSK3 &= ~(1<<OCIE3B))
#define GET_TIMER_COUNT()		(TCNT3)
#define GET_INPUT_CAPTURE()		(ICR3)
#define GET_COMPARE_A()		(OCR3A)
#define GET_COMPARE_B()		(OCR3B)
#define SET_COMPARE_A(val)		(OCR3A = (val))
#define SET_COMPARE_B(val)		(OCR3B = (val))
#define CAPTURE_INTERRUPT		TIMER3_CAPT_vect
#define COMPARE_A_INTERRUPT		TIMER3_COMPA_vect
#define COMPARE_B_INTERRUPT		TIMER3_COMPB_vect