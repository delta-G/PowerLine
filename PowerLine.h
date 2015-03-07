/*
 * PowerLine.h
 *
 *  Created on: Mar 4, 2015
 *      Author: David
 */

#ifndef POWERLINE_H
#define POWERLINE_H
#include"Arduino.h"


/*   This is just a simple wrapper class around the functions in
 *   PowerLineControl.cpp.  None of this is actually needed, it
 *   just gives you that nice class interface to use instead of
 *   having to deal with the namespace thing and the scope operator.
 */
namespace PowerLineControl {

void zeroCrossingISR();

void initPLC(uint8_t aOutpin);

void sendBit(uint8_t);

void addCommand(uint8_t, uint8_t);

}

class PowerLineClass {
public:
	PowerLineClass();

	void init(uint8_t aOutPin);

	void sendCommand(uint8_t aHouse, uint8_t aCom);

//	int spaceAvailable();



};

extern PowerLineClass PowerLine;

#endif /* POWERLINE_H_ */
