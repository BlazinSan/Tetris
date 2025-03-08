#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "DotMatrix.h"

enum PieceType {
  Line, TBlock, Square, SPiece, ZPiece, LBlock, RevLBlock
};

enum GameState {
  falling, locked, gameOver
};

typedef struct {
  enum PieceType piece;
  unsigned int xpos;
  unsigned int ypos;
  unsigned int rotation;
} Tetromino;

struct Lines {
  int rows[4];
  int size;
};

//Graphics
void createTetrominoes();
void initpField();
void drawField();
void drawPiece(Tetromino*);
//Handling game states
void handleFalling(Tetromino*);
void handleLocking(Tetromino*);
void handleGameOver();
//Gameplay
int pieceFits(enum PieceType, int, int, int);
int rotate(int, int, int);
void recordLines(int, struct Lines*);
void removeLines(struct Lines*);
void movePiece(Tetromino*);
void getNextPiece(Tetromino*);
void updateScore(struct Lines*);
void resetGame();

const int dataPin = 4;
const int clockPin = 3;
const int loadPin = 2;

const int rotateButtonPin = 10;
const int downButtonPin = 7;
const int rightButtonPin = 6;
const int leftButtonPin = 8;

DotMatrix display(dataPin, clockPin, loadPin);
char tetrominoLayout[7][17];
const int fieldWidth = 8;
const int fieldHeight = 16;
unsigned char pField[fieldWidth * fieldHeight];
enum GameState gState = falling;
int speed = 10;
int speedCounter = 0;
int score = 0;
int lineCount = 0;

void setup() {
  Serial.begin(9600);
  
  pinMode(rotateButtonPin, INPUT);
  pinMode(downButtonPin, INPUT);
  pinMode(rightButtonPin, INPUT);
  pinMode(leftButtonPin, INPUT);

  digitalWrite(leftButtonPin, HIGH);
  digitalWrite(rightButtonPin, HIGH);
  digitalWrite(downButtonPin, HIGH);
  digitalWrite(rotateButtonPin, HIGH);

  randomSeed(analogRead(0));
  createTetrominoes();

}

void loop() {
  initpField();
  static Tetromino currTet = {random(7), fieldWidth / 4, 0, 0};
  while (gState != gameOver) {
    delay(50);
    switch (gState) {
      case falling: handleFalling(&currTet); break;
      case locked: handleLocking(&currTet); break;
      default: break;
    }
    drawField();
    drawPiece(&currTet);
  }
  handleGameOver();
}

void createTetrominoes() {
	strcat(tetrominoLayout[Line],      "........XXXX....");	//line piece
	strcat(tetrominoLayout[TBlock],    ".....XXX..X.....");	//T block
	strcat(tetrominoLayout[Square],	   ".....XX..XX.....");	//square
	strcat(tetrominoLayout[SPiece],    ".....XX.XX......");	//s piece
	strcat(tetrominoLayout[ZPiece],    "....XX...XX.....");	//z piece
	strcat(tetrominoLayout[LBlock],    "....XXX.X.......");	//L block
	strcat(tetrominoLayout[RevLBlock], ".....XXX...X....");	//reverse L block
}

void initpField() {
  for (int x = 0; x < fieldWidth; x++) // Board Boundary
    for (int y = 0; y < fieldHeight; y++)
      pField[y * fieldWidth + x] = (x == 0 || x == fieldWidth-1 || y == fieldHeight - 1) ? 9 : 0;
}

int pieceFits(enum PieceType piece, int xpos, int ypos, int rotation) {
  for (int x = 0; x < 4; x++) {
    for (int y = 0; y < 4; y++) {
      //get index in tetromino coordinates
      int tetIndex = rotate(x, y, rotation);

      //get index in field (absolute) coordinates
      int fieldIndex = (ypos + y) * fieldWidth + (xpos + x);

      //check that test is in bounds
      if (xpos + x >= 0 && xpos + x < fieldWidth) {
        if (ypos + y >= 0 && ypos + y < fieldHeight) {
          //in bounds so do collision check
          if (tetrominoLayout[piece][tetIndex] != '.' && pField[fieldIndex] != 0)
            return 0; // fail on first hit
        }
      }
    }
  }
  return 1;
}

int rotate(int xpos, int ypos, int r) {
  switch (r % 4) {
    case 0: return 4 * ypos + xpos;    //0 degrees
    case 1: return 12 + ypos - 4 * xpos; //90 degrees
    case 2: return 15 - 4 * ypos - xpos; //180  degrees
    case 3: return 3 - ypos + 4 * xpos; //270 degrees
    default: break;
  }
  return 0;
}

void handleFalling(Tetromino* tet) {
  speedCounter++;
  movePiece(tet);
  if (speedCounter == speed) {
    if (pieceFits(tet->piece, tet->xpos, tet->ypos + 1, tet->rotation)) {
      tet->ypos++;
    }
    else gState = locked;
    speedCounter = 0;
  }
}

void handleLocking(Tetromino* tet) {
  //lock piece in place
  static struct Lines lines;
  for (int x = 0; x < 4; ++x)
    for (int y = 0; y < 4; ++y)
      if (tetrominoLayout[tet->piece][rotate(x, y, tet->rotation)] == L'X')
        pField[(tet->ypos + y) * fieldWidth + (tet->xpos + x)] = tet->piece + 1; //nonempty tieleset elements start at index 1

  recordLines(tet->ypos, &lines); //record the positions of any lines formed...
  removeLines(&lines);    //...then remove them
  updateScore(&lines);
  getNextPiece(tet);  //spawn the next piece
  if (!pieceFits(tet->piece, tet->xpos, tet->ypos, tet->rotation))
    gState = gameOver;
  else gState = falling;
}

void updateScore(struct Lines* lines) {
  //update score
  score += 25;  //score increases by 25 per piece...
  if (lines->size > 0)
    score += (1 << lines->size) * 100;    //...and by 4n where n is the number of lines formed simultaneously
}

void handleGameOver() {
  //display game over screen
  for (int row = 0; row < 16; ++row) {
    for (int col = 0; col < 16; ++col) {
      display.write(row, col, HIGH);
    }
  }
  delay(1000);
  Serial.print("Game over! Your score was: ");
  Serial.println(score);
  Serial.print("You cleared ");
  Serial.print(lineCount);
  Serial.println(" Lines.");

  resetGame();
}

void resetGame() {
  for (int x = 0; x < fieldWidth; ++x) {
    for (int y = 0; y < fieldHeight; ++y) {
      display.write(x, y, pField[y * fieldWidth + x] != 0);
    }
  }
  gState = falling;
}

//starting from the top of the tetromino (currentY),
//check if any square of the tetromino has formed a line with the current field
//if so, record the row number in lines and set that line to '='.
//Return the number of lines found
void recordLines(int currentY, struct Lines* lines) {
  for (int y = 0; y < 4; ++y) { //check tetromino space line by line
    if (currentY + y < fieldHeight - 1) {
      int line = 1;
      for (int x = 1; x < fieldWidth - 1; ++x) {
        //set line to false if there are any empty spaces in the line
        line = line && ((pField[(currentY + y) * fieldWidth + x]) != 0);
      }

      if (line) {
        //remove line, set to =
        for (int x = 1; x < fieldWidth - 1; ++x)
          pField[(currentY + y) * fieldWidth + x] = 8;

        lines->rows[lines->size] = currentY + y;

        //update difficulty
        lineCount++;
        if (lineCount % 10 == 0)
          if (speed >= 10) speed--;
      }
    }
    lines->size++;
  }
  //keep lines there for a bit
  delay(200);
}

void removeLines(struct Lines* lines) {
  if (lines->size > 0) {  //there is a line

    //remove the lines
    for (int i = 0; i < lines->size; ++i) {
      for (int x = 1; x < fieldWidth - 1; ++x) {
        for (int y = lines->rows[i]; y > 0; --y)
          pField[y * fieldWidth + x] = pField[(y - 1) * fieldWidth + x];  //move row above down 1 square

        pField[x] = 0;  //top row is set to zero
      }
    }
    //reset line array
    for (int i = 0; i < 4; ++i)
      lines->rows[i] = -1;
    lines->size = 0;  //reset line count
  }
}

void movePiece(Tetromino* tet) {
  int rotateAllowed = 1;
  tet->xpos -= !digitalRead(leftButtonPin) && pieceFits(tet->piece, tet->xpos-1, tet->ypos, tet->rotation);
  if(!digitalRead(leftButtonPin) )Serial.println("works");
  tet->xpos += !digitalRead(rightButtonPin) && pieceFits(tet->piece, tet->xpos+1, tet->ypos, tet->rotation);
  tet->ypos += !digitalRead(downButtonPin) && pieceFits(tet->piece, tet->xpos, tet->ypos+1, tet->rotation);
  if (!digitalRead(rotateButtonPin)) {	//rotate only 90 degrees at a time
		tet->rotation += (rotateAllowed && pieceFits(tet->piece, tet->xpos, tet->ypos, tet->rotation + 1)) ? 1 : 0;
		rotateAllowed = 0;
	}
	else
		rotateAllowed = 1;
  /*if (digitalRead(leftButtonPin) && pieceFits(tet->piece, tet->xpos - 1, tet->ypos, tet->rotation)) {
    tet->xpos--;
  }
  if (digitalRead(rightButtonPin) && pieceFits(tet->piece, tet->xpos + 1, tet->ypos, tet->rotation)) {
    tet->xpos++;
  }
  if (digitalRead(downButtonPin) && pieceFits(tet->piece, tet->xpos, tet->ypos + 1, tet->rotation)) {
    tet->ypos++;
  }
  if (isButtonPressed(rotateButtonPin) && pieceFits(tet->piece, tet->xpos, tet->ypos, (tet->rotation + 1) % 4)) {
    tet->rotation = (tet->rotation + 1) % 4;
  } */
}

bool isButtonPressed(int buttonPin) {
  static unsigned long lastPressTime = 0;
  const unsigned long debounceDelay = 50;
  while(digitalRead(buttonPin) == LOW) {
    unsigned long currentTime = millis();
    if (currentTime - lastPressTime > debounceDelay) {
      lastPressTime = currentTime;
      return true;
    }
  }
  return false;
}

void getNextPiece(Tetromino* tet) {
  tet->piece = random(7);
  tet->xpos = fieldWidth / 4;
  tet->ypos = 0;
  tet->rotation = 0;
}

void drawField() {
  for (int x = 0; x < fieldWidth; x++)
    for (int y = 0; y < fieldHeight; y++)
      display.write(x, y, pField[y * fieldWidth + x] != 0);
}

void drawPiece(Tetromino* tet) {
  for (int x = 0; x < 4; ++x)
    for (int y = 0; y < 4; ++y)
      if (tetrominoLayout[tet->piece][rotate(x, y, tet->rotation)] == 'X')
        display.write(tet->xpos + x, tet->ypos + y, HIGH);
}
