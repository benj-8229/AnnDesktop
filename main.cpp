#include <RGBmatrixPanel.h>
#include <SPI.h>
#include <SD.h>

#define FPS 120
#define CS_SD 12
#define LED 40

File bin;

uint16_t colors[2];
int curs = 0;
int val = 0;

uint32_t prevTime = 0; // For frame-to-frame interval timing

RGBmatrixPanel *matrix = new RGBmatrixPanel(A0, A1, A2, A3, 11, 10, 9, true, 64);

void(* resetFunc) (void) = 0;

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  matrix->begin();
  matrix->fillScreen(0);
  matrix->swapBuffers(true);

  if (!SD.begin(CS_SD)) {
    resetFunc();
  }
  
  colors[0] = matrix->Color888(0, 0, 0);
  colors[1] = readColor();
  
  bin = SD.open("encoded.bin", O_READ);
  
  delay(500);
  digitalWrite(LED, LOW);
}

void loop() {
  val = readFrame(bin, curs);
  if (val != -1) {
    curs += val;
  } else {
    curs = 0;
    return;
  }
  
  uint32_t t;
  while(((t = millis()) - prevTime) < (1000 / FPS));
  prevTime = t;
}

int readFrame(File f, int pos) {
  /*
    Our data structure is made up of blocks that start with a 2 byte declaration of the length of the 
    block, followed by pairs of coordinates for colored pixels. On stronger hardware it could easily 
    be simpler, but this was originally wrote for the ATmega2560 so some optimizations had to be made.
    We then read the pairs of coordinates and color those pixels, before returning the length which can
    then be used to increase the cursor position.
  */

  matrix->fillScreen(0);
  
  uint16_t len[1]; // 2 byte length declaration
  
  f.seek(pos);
  if(!f.read(len, 2)) {
    // if we try and read our 2 bytes and get nothing we should return -1, signifying EOF and cursor should be sent back to 0
    return -1;
  }

  // skip past length and then read the length of the pixels into our array
  f.seek(pos + 2);
  uint8_t pixels[len[0] - 2] = {};
  f.read(pixels, len[0] - 2);

  for (int i = 0; i < len[0] - 2; i += 2) {
    matrix->drawPixel(pixels[i], pixels[i+1], colors[1]);
  }
  
  matrix->swapBuffers(true);

  return len[0];
}

uint16_t readColor() {
  uint8_t rgb[3];
  File color = SD.open("color.bin", O_READ);
  color.seek(0);
  color.read(rgb, 3);
  color.close();
  return matrix->Color888(rgb[0], rgb[1], rgb[2]);
}
