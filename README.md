# PowerLine
Non-blocking replacement for the Arduino X10 library

The only X-10 library that I could find at the time blocked through the entire cycle.  In the protocol it sends one 
bit on each zero crossing, but it sends the bits several times and repeats the commands so that the whole process takes
a pretty long time.  I didn't like that blocking behavior so I wrote this library to use the 16-bit Timer1 on a UNO to
push out X-10 commands in a manner very similar to sending out Serial data.

Currently the library only supports sending commands, not receiving them.  This library uses Timer1, so it will not work
with other libraries like Servo that also use Timer1.  

X-10 sending only involves 2 wires, a data line and a zero crossing line.  Please see a source on X-10 protocol to locate
these pins.  The data line can go on any pin on the board, but the zero crossing line MUST go to pin 2 on the UNO (it uses 
interrupt0).


The API is very simple.  The library provides an instace called PowerLine.  In the setup the user should call 
PowerLine.init(aPIN) with the pin number where they connected the data line.  

PowerLine.sendCommand(aHouse, aCommand) 
will simply send a house number and a byte of data.  For most commands you would call this once with aCommand 
set to the unit number and then call it again with the same house code and the actual command code for that unit. 
If there is no room in the command buffer, sendCommand will block until there is.  

PowerLine.freeSpace() will return the number of commands worth of free space in the buffer.  

PowerLine has a circular buffer thatcan hold up to 32 commands and works similar to the Serial buffer in HardwareSerial.  
Typically X-10 commands are sent by sending first the house and unit numbers and then the house and command.  So this
would actually allow for 16 commands.  But PowerLine takes advantage of the fact that the unit number only takes 5 bits 
and can "hide" a UNIT_ON or UNIT_OFF command in the unit number when it puts it in the buffer.  So if you are only sending 
on and off commands you can actually pack 32 by using the three argument version of sendCommand.  

PowerLine.sendCommand(aHouse, aNumber, aCommand) will send a command to a given house and unit number.  If the command is a
UNIT_ON or UNIT_OFF command then it will save space in the buffer by only using one slot instead of two.  Like with the 
other version of this function, it will block if the buffer is full.  

