#include "DotMatrix.h"

// Matrix registers
#define REG_NOOP   0x00
#define REG_DIGIT0 0x01
#define REG_DIGIT1 0x02
#define REG_DIGIT2 0x03
#define REG_DIGIT3 0x04
#define REG_DIGIT4 0x05
#define REG_DIGIT5 0x06
#define REG_DIGIT6 0x07
#define REG_DIGIT7 0x08
#define REG_DECODEMODE  0x09
#define REG_INTENSITY   0x0A
#define REG_SCANLIMIT   0x0B
#define REG_SHUTDOWN    0x0C
#define REG_DISPLAYTEST 0x0F

#define SCREENS 4

DotMatrix::DotMatrix(uint8_t data, uint8_t clock, uint8_t load) {
  dataPin = data;
  clockPin = clock;
  loadPin = load;
	pinMode(clockPin, OUTPUT);
	pinMode(dataPin, OUTPUT);
	pinMode(loadPin, OUTPUT);
	
  buffer = (uint8_t*)calloc(4, 64); //allocate 4 screens' worth of 8*8 matrices

	clear();
	//set brightness to 2 out of 15 (adjust as needed)
	setRegister(REG_INTENSITY, 0x02 & 0x0F);
	//hardware business
  setRegister(REG_SCANLIMIT, 0x07);
	setRegister(REG_SHUTDOWN, 0x01);    // normal operation
	setRegister(REG_DECODEMODE, 0x00);  // pixels not integers
	setRegister(REG_DISPLAYTEST, 0x00); // not in test mode
}

//send a byte to the shift register serially
void DotMatrix::sendByte(uint8_t data) {
  uint8_t bit = 8;
  uint8_t mask;
  while(bit > 0) {
    mask = 0x01 << (bit - 1);         // get bitmask
    //data is shifted through the register on rising edge of clock pulse
    digitalWrite(clockPin, LOW);   // tick
    digitalWrite(dataPin, (data & mask) ? HIGH : LOW);
    digitalWrite(clockPin, HIGH);  // tock
    --bit;                            // move to lesser bit
  }
}

//set register to a byte value for all screens
//columns are specified by their actual numbers via an on-chip SRAM
//the columns or "digits" are from 0x01 to 0x08, and in our orientation, they would be 
//numbered from right to left. This means that in order to specify column 3, or example, we would have
//to send 8 - 3 = 0x05 to the register 
void DotMatrix::setRegister(uint8_t reg, uint8_t data){
	//data latches on rising edge of load pulse
	digitalWrite(loadPin, LOW); //tick
	for(uint8_t i = 0; i< SCREENS; ++i){
		sendByte(reg);	
		sendByte(data);	//send data to column specified above
	}
	digitalWrite(loadPin, HIGH);	//tock (data latched)	
	digitalWrite(loadPin, LOW);		//end
}

//sync column of display with buffer
void DotMatrix::syncColumn(uint8_t col){
  if(buffer == nullptr) return;
	if(col >= 8) return;	//only 8 columns are available
	digitalWrite(loadPin, LOW);	//tick
	for(uint8_t scr = 0; scr < SCREENS; ++scr){
    sendByte(8-col);    //specify column
    sendByte(buffer[col + (8*scr)]); //send data
	}
	digitalWrite(loadPin, HIGH);	//tock
	digitalWrite(loadPin, LOW);		//end
}

void DotMatrix::bufferPixel(uint8_t x, uint8_t y, uint8_t value) {
  if(buffer == nullptr) return;
	if(x >= 8 || y >= 32) return;

  //find which screen the row (y) belongs to, 
  //then express y in the coordinates of that screen 
	uint8_t offset = y;	//record value of y
	y %= 8;	  //express y in relative coordinates
	offset -= y;	 //starting from the top, we need to go offset units down to get to y's screen
	
  //the xth column of the (offset/8)th screen has its yth row updated to value
	if(value){
    buffer[x + offset] |= 0x01 << (7-y);  //the shift is so that y increases going down
  } else{
    buffer[x + offset] &= ~(0x01 << (7-y));
  }
}

void DotMatrix::write(uint8_t x, uint8_t y, uint8_t value){
	bufferPixel(x, y, value);	//store pixel in buffer
	syncColumn(x);			//update column
}

void DotMatrix::clear()
{
  if (!buffer) return;

  // clear buffer
  for(uint8_t col = 0; col < 8; ++col){
    for(uint8_t scr = 0; scr < SCREENS; ++scr){
      buffer[col + (8 * scr)] = 0x00;
    }
  }

  // clear registers
  for(uint8_t col = 0; col < 8; ++col){
    syncColumn(col);
  }
}

