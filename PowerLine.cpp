/*
 * PowerLine.cpp
 *
 *  Created on: Mar 4, 2015
 *      Author: David
 */

#include "PowerLine.h"

PowerLineClass PowerLine;

#define BUF_SIZE 32

struct ring_buffer {
	unsigned int buffer[BUF_SIZE];
	volatile unsigned int head;
	volatile unsigned int tail;
};

namespace PowerLineControl {

ring_buffer txBuffer = { { 0 }, 0, 0 };

uint8_t outputPin = 7;
uint8_t zeroXingPin = 2;

volatile uint8_t *outputPinPort;
uint8_t outputPinMask;

volatile boolean transmitting = false;

void addCommand(uint8_t aHouse, uint8_t aCom) {
	unsigned int i = (unsigned int) (txBuffer.head + 1) % BUF_SIZE;

	if (i != txBuffer.tail) {
		txBuffer.buffer[txBuffer.head] =
				(((unsigned int) aHouse << 8) | (aCom));
		txBuffer.head = i;
	}
	if (!transmitting) {  // If the interrupt isn't running, turn it on.
		EIFR |= 1;  // Clear any pending interrupt
		attachInterrupt(0, zeroCrossingISR, CHANGE);
	}
}

void zeroCrossingISR() {

	static uint8_t bitCounter = 0;
	static boolean parityBit = false;
	static uint8_t zciState = 0;
	static uint8_t houseCode;
	static uint8_t commandCode;

	switch (zciState) {

	case 0: {
		//  get next command loaded.
		unsigned int newcom = txBuffer.buffer[txBuffer.tail];
		txBuffer.tail = (txBuffer.tail + 1) % BUF_SIZE;
		houseCode = newcom >> 8;
		commandCode = newcom & 0xFF;
		zciState++;
		// Fall through and run case 1 to start transmitting
	}
		/* no break */
	case 1: {

		if (bitCounter < 3) {
			sendBit(1);
			bitCounter++;
		} else {
			sendBit(0);
			zciState++;
			bitCounter = 0;
			parityBit = false;
		}
		break;
	}

	case 2: {

		uint8_t bitMask = 1 << (3 - bitCounter);
		uint8_t bitToSend = (houseCode & bitMask) ? 1 : 0;

		if (!parityBit) {
			sendBit(bitToSend);
		} else {
			sendBit(!bitToSend);
			bitCounter++;
		}
		parityBit = !parityBit;

		if (bitCounter >= 4) {  // Sent the whole house code
			bitCounter = 0;
			zciState++;
			parityBit = false;
		}
		break;
	}

	case 3: {

		uint8_t bitMask = 1 << (4 - bitCounter);
		uint8_t bitToSend = (commandCode & bitMask) ? 1 : 0;

		if (!parityBit) {
			sendBit(bitToSend);
		} else {
			sendBit(!bitToSend);
			bitCounter++;
		}
		parityBit = !parityBit;

		if (bitCounter >= 5) {  // Sent the whole number code
			bitCounter = 0;
			zciState++;
			parityBit = false;
		}
		break;
	}

	case 4: {         // Wait three more crossing before OK next command
		static boolean repeated = false;
		bitCounter++;
		if (bitCounter >= 3) {
			bitCounter = 0;
			if (!repeated) {
				zciState = 1;  // send us back around with same command
			} else if (commandCode & (1 << 7)) {  // This little addition lets us hide on and off with a command code to save a space in the buffer

				if(commandCode & (1 << 6)) {
					commandCode = 0b00111;   // turn-off
				} else {
					commandCode = 0b00101;  // turn-on
				}
				zciState = 1;  // go around again with the new command
			}

			else {
				if (txBuffer.head == txBuffer.tail) {
					//  Buffer is Empty
					detachInterrupt(0);
					transmitting = false;
				}
				zciState = 0;
			}
			repeated = !repeated;
		}
		break;
	}
	}  // end switch (zciState)
}

void initPLC(uint8_t aOutpin) {
	outputPin = aOutpin;
	outputPinPort = portOutputRegister(digitalPinToPort(outputPin));
	outputPinMask = digitalPinToBitMask(outputPin);

	pinMode(outputPin, OUTPUT);
	*outputPinPort &= ~outputPinMask;	//make sure the pin is OFF
	pinMode(zeroXingPin, INPUT_PULLUP);

	//Set up the timer.
	cli();
	TCCR1A = 0;
	TCCR1B = ((1 << WGM12) | (1 << CS10));
	OCR1A = 41248;
	OCR1B = 12800;
	sei();
	delay(5); // In case we set at the wrong time and the timer needs to wrap around to behave.

}

void sendBit(uint8_t aBit) {
	if (aBit) {
		TCNT1 = 0;  // reset the counter
		*outputPinPort |= outputPinMask;  // Start with pin on for a 1
		TIFR1 |= ((1 << OCF1A) | (1 << OCF1B) | (1 << TOV1)); // Clear any interrupt flags waiting
		TIMSK1 |= ((1 << OCIE1A) | (1 << OCIE1B));  // Set the interrupt to run
	} else {
		*outputPinPort &= ~outputPinMask;  // Start with pin off for a 0
		TIMSK1 &= ~((1 << OCIE1A) | (1 << OCIE1B)); // Turn off the interrupt.  (should aleady be off)
	}
}

inline void compaISR() {
	*outputPinPort |= outputPinMask;
}

inline void compbISR() {
	static uint8_t ovfCount = 0;
	*outputPinPort &= ~outputPinMask;  // Turn pin off
	if (ovfCount >= 2) {  //We've done this twice before
		//TCCR1A &= ~(1 << COM2B1);  // Turn off the PWM
		TIMSK1 &= ~((1 << OCIE1A) | (1 << OCIE1B));  // Turn off the interrupts.
		ovfCount = 0;
	} else {
		ovfCount++;
	}
}

}

ISR(TIMER1_COMPA_vect) {
	PowerLineControl::compaISR();
}

ISR(TIMER1_COMPB_vect) {
	PowerLineControl::compbISR();
}

PowerLineClass::PowerLineClass() {

}

void PowerLineClass::init(uint8_t aOutPin) {

	PowerLineControl::initPLC(aOutPin);

}

//  **TODO  fix this function
//int PowerLineClass::spaceAvailable(){
//	return BUF_SIZE;
//}

void PowerLineClass::sendCommand(uint8_t aHouseCode, uint8_t aComCode) {
	PowerLineControl::addCommand(aHouseCode, aComCode);
}


void PowerLineClass::sendCommand(uint8_t aHouse, uint8_t aNumber, uint8_t aCommand){

	// If the command is turn_on or turn_off, then we can hide it in the
	//    three bits of commandCode that we aren't using.
	//    I wrote the code that fetches the next command to check
	//    for this and to act accordingly.

	uint8_t newCom = aNumber;
	if(aCommand == UNIT_ON){
		newCom |= 0b10100000;
		sendCommand(aHouse, newCom);
	} else if(aCommand == UNIT_OFF){
		newCom |= 0b11100000;
		sendCommand(aHouse, newCom);
	} else {
		sendCommand(aHouse, aNumber);
		sendCommand(aHouse, aCommand);
	}
}


int PowerLineClass::freeSpace() {

	int retval = PowerLineControl::txBuffer.head - PowerLineControl::txBuffer.tail;

	if (retval < 0 ){   // way faster than using an add and modulo.
		retval += BUF_SIZE;
	}
	return retval;
}
