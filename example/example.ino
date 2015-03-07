

/*
        A button wired to pin 5 starts the program.  An LED on pin 13 creates a heartbeat.
        The heartbeat continues unchanged while the program spits out the X10 command.
        Contrast with the X10 library where all other functions are blocked during the 
        agonizingly slow X10 transfer.
*/

#include "PowerLine.h"

boolean lampOn = false;
int heartbeatDelay = 500;


void setup() {


	PowerLine.init(3);
	pinMode(13, OUTPUT);
	pinMode(5, INPUT_PULLUP);
	while (digitalRead(5) == HIGH) {
		heartBeat();
	}
	heartbeatDelay = 250;
	digitalWrite(13, HIGH);
	delay(3000);
	digitalWrite(13, LOW);

}

void loop() {

	static unsigned long previousMillis = millis() - 4000ul;

	unsigned long currentMillis = millis();

	if (currentMillis - previousMillis > 5000) {
		lampOn = !lampOn;
		previousMillis = currentMillis;

		if (lampOn) {
			PowerLine.sendCommand(0b1110, 0b10100);  // House B unit 04
			PowerLine.sendCommand(0b1110, 0b00101);  // House B ON
		} else {
			PowerLine.sendCommand(0b1110, 0b10100);  // House B unit 04
			PowerLine.sendCommand(0b1110, 0b00111);  // House B OFF
		}
	}
	heartBeat();
}

void heartBeat() {
	static unsigned long previousMillis = millis();
	unsigned long currentMillis = millis();

	if (currentMillis - previousMillis >= heartbeatDelay) {
		digitalWrite(13, !digitalRead(13));
		previousMillis = currentMillis;
	}
}
