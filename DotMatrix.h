#ifndef DotMatrix_h
#define DotMatrix_h

#include <stdint.h>
#include <Arduino.h>

//MAX7219 dot matrix module
class DotMatrix {
public:
	//constructor
	DotMatrix(uint8_t data, uint8_t clock, uint8_t load);
private:
	//the three pins 
	uint8_t dataPin;
	uint8_t clockPin;   //data shifts through registers on rising edge of clock pulse
	uint8_t loadPin;    //data latches on rising edge of load pulse
	
	uint8_t* buffer; 	//buffer array; 4 screens, each with 8 columns 8 rows high
                    //each element is a column; the columns of screen 0 are stored consecutively 
                    //from left to right, then the columns of screen 1 are stored from left to right, and so on
	
	void sendByte(uint8_t);	//send byte (row) serially to MAX7219 DIN pin
	void setRegister(uint8_t, uint8_t);	//set register to a byte value for all screens
	void syncColumn(uint8_t);	//sync column of display with buffer
	
	void bufferPixel(uint8_t, uint8_t, uint8_t);	//record pixel in buffer

	//PUBLIC INTERFACE
public:
	void write(uint8_t x, uint8_t y, uint8_t value);	//buffer pixe at (x,y) and write it ot screen
	void clear();	//clear display
	
};

#endif