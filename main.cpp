/*
 * GccApplication1.cpp
 *
 * Created: 2/21/2023 8:55:47 PM
 * Author : theka
 */

#include "main.h"
#include "midiXparser.h"
#include <avr/io.h>

/** Circular buffer to hold data from the host before it is sent to the device via the serial port. */
static RingBuffer_t USBtoUSART_Buffer;

/** Underlying data buffer for \ref USBtoUSART_Buffer, where the stored bytes are located. */
static uint8_t USBtoUSART_Buffer_Data[128];

/** Circular buffer to hold data from the serial port before it is sent to the host. */
static RingBuffer_t USARTtoUSB_Buffer;

/** Underlying data buffer for \ref USARTtoUSB_Buffer, where the stored bytes are located. */
static uint8_t USARTtoUSB_Buffer_Data[128];
extern USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface;
extern USB_ClassInfo_MIDI_Device_t Keyboard_MIDI_Interface;

midiXparser midiParser;
AltSoftSerial SoftSerial;

///////////////////////////////////////////////////////////////////////////////
// Route a serial midi msg to the right USB midi cable or other destination
///////////////////////////////////////////////////////////////////////////////
static void RouteStdMidiMsg()
{
	unsigned char outPacket[4];
	outPacket[0] = 0;

	uint8_t msgLen = midiParser.getMidiMsgLen();
	uint8_t msgType = midiParser.getMidiMsgType();

	memcpy(&outPacket[1], &(midiParser.getMidiMsg()[0]), msgLen);

	// Real time single byte message CIN F->
	if (msgType == midiXparser::realTimeMsgTypeMsk)
		outPacket[0] += 0xF;
	else
	{
		// Channel voice message CIN A-E
		if (msgType == midiXparser::channelVoiceMsgTypeMsk)
		{
			outPacket[0] += ((midiParser.getMidiMsg()[0]) >> 4);
		}
		else
		{
			// System common message CIN 2-3
			if (msgType == midiXparser::systemCommonMsgTypeMsk)
			{

				// 5 -  single-byte system common message (Tune request is the only case)
				if (msgLen == 1)
					outPacket[0] += 5;

				// 2/3 - two/three bytes system common message
				else
					outPacket[0] += msgLen;
			}

			else
				return; // We should never be here !
		}
	}

	const MIDI_EventPacket_t *outEvent = (const MIDI_EventPacket_t *)(&outPacket);

	// char outBuf[30]="";
	// sprintf(outBuf, "Sent MIDI: %02x%02x%02x%02x\r\n", outPacket[0], outPacket[1], outPacket[2], outPacket[3]);
	// CDC_Device_SendString(&VirtualSerial_CDC_Interface, outBuf);

	MIDI_Device_SendEventPacket(&Keyboard_MIDI_Interface, outEvent);
	MIDI_Device_Flush(&Keyboard_MIDI_Interface);
}
// http://personal.kent.edu/~sbirch/Music_Production/MP-II/MIDI/midi_channel_voice_messages.htm
int main(void)
{
	SetupHardware();

	RingBuffer_InitBuffer(&USBtoUSART_Buffer, USBtoUSART_Buffer_Data, sizeof(USBtoUSART_Buffer_Data));
	RingBuffer_InitBuffer(&USARTtoUSB_Buffer, USARTtoUSB_Buffer_Data, sizeof(USARTtoUSB_Buffer_Data));

	// LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	SoftSerial.begin(MIDI_BAUD_RATE);
	// Ignore sysex
	midiParser.setMidiMsgFilter(0xf);

	GlobalInterruptEnable();
	for (;;)
	{
		/* Only try to read in bytes from the CDC interface if the transmit buffer is not full */
		if (!(RingBuffer_IsFull(&USBtoUSART_Buffer)))
		{
			int16_t ReceivedByte = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

			/* Store received byte into the USART transmit buffer */
			if (!(ReceivedByte < 0))
				RingBuffer_Insert(&USBtoUSART_Buffer, ReceivedByte);
		}

		uint16_t BufferCount = RingBuffer_GetCount(&USARTtoUSB_Buffer);
		if (BufferCount)
		{
			Endpoint_SelectEndpoint(VirtualSerial_CDC_Interface.Config.DataINEndpoint.Address);

			/* Check if a packet is already enqueued to the host - if so, we shouldn't try to send more data
			 * until it completes as there is a chance nothing is listening and a lengthy timeout could occur */
			if (Endpoint_IsINReady())
			{
				/* Never send more than one bank size less one byte to the host at a time, so that we don't block
				 * while a Zero Length Packet (ZLP) to terminate the transfer is sent if the host isn't listening */
				uint8_t BytesToSend = MIN(BufferCount, (CDC_TXRX_EPSIZE - 1));

				/* Read bytes from the USART receive buffer into the USB IN endpoint */
				while (BytesToSend--)
				{
					/* Try to send the next byte of data to the host, abort if there is an error without dequeuing */
					if (CDC_Device_SendByte(&VirtualSerial_CDC_Interface,
											RingBuffer_Peek(&USARTtoUSB_Buffer)) != ENDPOINT_READYWAIT_NoError)
					{
						break;
					}

					/* Dequeue the already sent byte from the buffer now we have confirmed that no transmission error occurred */
					RingBuffer_Remove(&USARTtoUSB_Buffer);
				}
			}
		}
		/* Load the next byte from the USART transmit buffer into the USART if transmit buffer space is available */
		if (Serial_IsSendReady() && !(RingBuffer_IsEmpty(&USBtoUSART_Buffer)))
		{
			Serial_SendByte(RingBuffer_Remove(&USBtoUSART_Buffer));
		}
		while (SoftSerial.available())
		{
			byte receivedByte = SoftSerial.read();
			if (midiParser.parse(receivedByte))
			{

				// We manage sysEx "on the fly". Clean end of a sysex msg ?
				if (midiParser.getMidiMsgType() == midiXparser::sysExMsgTypeMsk)
				{
					// RouteSysExMidiMsg(0, &midiParser) ;
				}

				else
				{
					// Not a sysex. The message is complete.
					RouteStdMidiMsg();
				}
			}
			else
			{
				// Acknowledge any sysex error
				if (midiParser.isSysExError())
				{
					// RouteSysExMidiMsg() ;
				}
				else
				{
					// Check if a SYSEX mode active and send bytes on the fly.
					if (midiParser.isSysExMode() && midiParser.isByteCaptured())
					{
						//	RouteSysExMidiMsg() ;
					}
				}
			}
		}

		MIDI_Device_USBTask(&Keyboard_MIDI_Interface);
		CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	USB_Init();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	// LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	// LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
	ConfigSuccess &= MIDI_Device_ConfigureEndpoints(&Keyboard_MIDI_Interface);

	// LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
	MIDI_Device_ProcessControlRequest(&Keyboard_MIDI_Interface);
}

/** ISR to manage the reception of data from the serial port, placing received bytes into a circular buffer
 *  for later transmission to the host.
 */
ISR(USART1_RX_vect, ISR_BLOCK)
{
	uint8_t ReceivedByte = UDR1;

	if ((USB_DeviceState == DEVICE_STATE_Configured) && !(RingBuffer_IsFull(&USARTtoUSB_Buffer)))
		RingBuffer_Insert(&USARTtoUSB_Buffer, ReceivedByte);
}

/** Event handler for the CDC Class driver Line Encoding Changed event.
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface configuration structure being referenced
 */
void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t *const CDCInterfaceInfo)
{
	uint8_t ConfigMask = 0;

	switch (CDCInterfaceInfo->State.LineEncoding.ParityType)
	{
	case CDC_PARITY_Odd:
		ConfigMask = ((1 << UPM11) | (1 << UPM10));
		break;
	case CDC_PARITY_Even:
		ConfigMask = (1 << UPM11);
		break;
	}

	if (CDCInterfaceInfo->State.LineEncoding.CharFormat == CDC_LINEENCODING_TwoStopBits)
		ConfigMask |= (1 << USBS1);

	switch (CDCInterfaceInfo->State.LineEncoding.DataBits)
	{
	case 6:
		ConfigMask |= (1 << UCSZ10);
		break;
	case 7:
		ConfigMask |= (1 << UCSZ11);
		break;
	case 8:
		ConfigMask |= ((1 << UCSZ11) | (1 << UCSZ10));
		break;
	}

	/* Keep the TX line held high (idle) while the USART is reconfigured */
	PORTD |= (1 << 3);

	/* Must turn off USART before reconfiguring it, otherwise incorrect operation may occur */
	UCSR1B = 0;
	UCSR1A = 0;
	UCSR1C = 0;

	/* Set the new baud rate before configuring the USART */
	UBRR1 = SERIAL_2X_UBBRVAL(CDCInterfaceInfo->State.LineEncoding.BaudRateBPS);

	/* Reconfigure the USART in double speed mode for a wider baud rate range at the expense of accuracy */
	UCSR1C = ConfigMask;
	UCSR1A = (1 << U2X1);
	UCSR1B = ((1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1));

	/* Release the TX line after the USART has been reconfigured */
	PORTD &= ~(1 << 3);
}
