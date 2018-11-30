typedef volatile unsigned int ioreg;

// Registers for Driving the Display
#define PIO_PDR (ioreg *) 0xfffff404 // PIO disable register
#define PIO_ASR (ioreg *) 0xfffff470 // PIO A select register

#define SPI_CR   (ioreg *) 0xfffe0000 // SPI Control Register
#define SPI_MR   (ioreg *) 0xfffe0004 // SPI Mode Register
#define SPI_SR   (ioreg *) 0xfffe0010 // SPI status register
#define SPI_TDR  (ioreg *) 0xfffe000c // SPI Transmit Data Register
#define SPI_CSR0 (ioreg *) 0xfffe0030 // SPI Chip Select Register 0
#define PMC_PCER (ioreg *) 0xfffffc10 // Enables peripherals on ARM

// Registers for Reading Analog Inputs
#define ADC_CR   (ioreg *) 0xfffd8000 // ADC control register
#define ADC_MR   (ioreg *) 0xfffd8004 // ADC mode register
#define ADC_CHER (ioreg *) 0xfffd8010 // ADC channel enable register
#define ADC_SR   (ioreg *) 0xfffd801c // ADC status register
#define ADC_CDR4 (ioreg *) 0xfffd8040 // ADC channel 4 data register
#define ADC_CDR5 (ioreg *) 0xfffd8044 // ADC channel 5 data register

#define portA 0xc000 // Write data to Port A and update Port B with BUFFER content
#define portB 0x4000 // Write data to Port B and BUFFER
#define minPaddleValue 105 // Maximum value that can be retrieve from the ADC_CDR4 register
#define maxPaddleValue 555 // Minimum value that can be retrieve from the ADC_CDR4 register

// Constants used to control Pong
#define zoneWidth 1000
#define zoneHeight 500
#define batWidth 20
#define batLength 70
#define batSpacing 40
#define scoresOffset 30
#define digitSegmentLength 25
#define scoreBias 10
#define ballSpeed 6
#define ballRadius 5

// Global variables
int leftBatOffset = 0, rightBatOffset = 0;
int ballX = 0, ballY = 0;
int ballControlX = 1, ballControlY = 1;
int leftScore = 0, rightScore = 0;

int main()
{
  // Outputs
  *PMC_PCER = 0x34;  // enable PIOA, ADC and SPI
  LowLevelInit();    // Set up PLL and MCK clocks
  *PIO_PDR = 0x7800; // disable bits 11 - 14 for SPI (See manual section 10.4)
  *PIO_ASR = 0x7800; // enable peripheral A for bits 11 - 14
  *SPI_CR = 0x80;    // reset the SPI
  *SPI_CR = 0x1;     // enable the SPI
  *SPI_MR = 0x11;    // set SPI to master, disable fault detection
  *SPI_CSR0 = 0x183; // set clock polarity = 1, clock phase = 1, Bits per transfer = 16, serial clock baud rate = 1
  *SPI_TDR = 0xd002; // Set reference voltage to 2.048V in control register

  // Inputs
  *ADC_CR = 0x1;        // reset the ADC
  *ADC_CHER = 0x30;     // enable analog channels 4 and 5
  *ADC_MR = 0x030b0400; // sample+holdtime = 3, startup = b, prescale = 4

  // Centre the ball
  reset();

  // Mainloop
  while(1)
  {
    input();
    update();
    render();
  }

  return 0;
}

input()
{
  *ADC_CR = 0x2;                        // start conversion
  while (*ADC_SR & 0x10 == 0);          // wait for ch4 to complete conversion
  int leftPaddleValue = *ADC_CDR4;      // retrieve value from ADC_CDR4 register

  // linearly interpolate the value from the left paddle to the appropriate range based on the zone height
  leftBatOffset = (leftPaddleValue - minPaddleValue)
                 /(maxPaddleValue - minPaddleValue)
                 *(zoneHeight - batLength);

  *ADC_CR = 0x2;                        // start conversion
  while (*ADC_SR & 0x20 == 0);          // wait for ch5 to complete conversion
  int rightPaddleValue = *ADC_CDR5;     // retrieve value from ADC_CDR4 register

  // linearly interpolate the value from the right paddle to the appropriate range based on the zone height
  rightBatOffset = (rightPaddleValue - minPaddleValue)
                  /(maxPaddleValue - minPaddleValue)
                  *(zoneHeight - batLength);
}

update()
{
  moveBall();
  zoneCollision();
  batCollision();
}

render()
{
  drawZone();
  drawBall();
  drawBats();
  drawScores();
}

drawPoint(int x, int y)
{
  // check that x and y have values that can be represented by a 10 bit unsigned integer
  if(x >= 0 && x < 2048 && y >= 0 && y < 2048)
  {
    // for both x and y, perform a left bit shift operation twice (multiply by 4) to
    // move the value of the coordinate to the appropriate bits (D02-D11) which also
    // implicitly sets the values of D00 and D01 to 0. after this add the appropriate
    // constant that has the relevant control bits set (D12-D15). then transmit the
    // two least significant bytes of this resulting value to the SPI_TDR register

    while(*SPI_SR & 1 != 1);     // Wait for serialisation
    *SPI_TDR = (x << 2) + portB; // Transmit to the SPI_TDR register

    while(*SPI_SR & 1 != 1);     // Wait for serialisation
    *SPI_TDR = (y << 2) + portA; // Transmit to the SPI_TDR register
  }
}

// draws a line of length l vertically upwards from (x, y)
drawVerticalLine(int x, int y, int l)
{
  int i;
  for(i = 0; i < l; i++)
  {
    drawPoint(x, y + i);
  }
}

// draws a line of length l horizontally to the right from (x, y)
drawHorizontalLine(int x, int y, int l)
{
  int i;
  for(i = 0; i < l; i++)
  {
    drawPoint(x + i, y);
  }
}

// (x, y) is bottom left corner of the rectangle
drawRect(int x, int y, int length, int height)
{
  drawVerticalLine(x, y, height);
  drawVerticalLine(x + length, y, height);
  drawHorizontalLine(x, y, length);
  drawHorizontalLine(x, y + height, length);
}

// draws a circle of radius r centred at (x, y)
drawCircle(int x, int y, int r)
{
  int i, j;
  for(i = -r; i < r; i++)
  {
    for(j = -r; j < r; j++)
    {
      if(i * i + j * j < r * r)
      {
          drawPoint(x + i, y + j);
      }
    }
  }
}

reset()
{
  ballX = zoneWidth/2;
  ballY = zoneHeight/2;
}

drawScores()
{
  // Draws colon
  drawCircle(zoneWidth/2, zoneHeight - scoresOffset - scoreBias, 2);
  drawCircle(zoneWidth/2, zoneHeight - scoresOffset - digitSegmentLength*2 + scoreBias, 2);

  // Draws left players score
  drawDigit(zoneWidth/2 - digitSegmentLength*2 - scoreBias*2, zoneHeight - digitSegmentLength*2 - scoresOffset, leftScore/10);
  drawDigit(zoneWidth/2 - digitSegmentLength - scoreBias, zoneHeight - digitSegmentLength*2 - scoresOffset, leftScore%10);

  // Draws right players score
  drawDigit(zoneWidth/2 + scoreBias, zoneHeight - digitSegmentLength*2 - scoresOffset, rightScore/10);
  drawDigit(zoneWidth/2 + scoreBias*2 + digitSegmentLength, zoneHeight - digitSegmentLength*2 - scoresOffset, rightScore%10);
}

// draws a digit using a 7-segment display. (x, y) is the bottom left of the digit and n is the value of the digit
void drawDigit(int x, int y, int n)
{
  n %= 10;
  if(n == 0 || n == 2 || n == 3 || n == 5 || n == 6 || n == 7 || n == 8 || n == 9)           drawHorizontalLine(x, y + digitSegmentLength*2, digitSegmentLength);
  if(n == 0 || n == 1 || n == 2 || n == 3 || n == 4 || n == 7 || n == 8 || n == 9)           drawVerticalLine(x + digitSegmentLength, y + digitSegmentLength, digitSegmentLength);
  if(n == 0 || n == 1 || n == 3 || n == 4 || n == 5 || n == 6 || n == 7 || n == 8 || n == 9) drawVerticalLine(x + digitSegmentLength, y, digitSegmentLength);
  if(n == 0 || n == 2 || n == 3 || n == 5 || n == 6 || n == 8)                               drawHorizontalLine(x, y, digitSegmentLength);
  if(n == 0 || n == 2 || n == 6 || n == 8)                                                   drawVerticalLine(x, y, digitSegmentLength);
  if(n == 0 || n == 4 || n == 5 || n == 6 || n == 8 || n == 9)                               drawVerticalLine(x, y + digitSegmentLength, digitSegmentLength);
  if(n == 2 || n == 3 || n == 4 || n == 5 || n == 6 || n == 8 || n == 9)                     drawHorizontalLine(x, y + digitSegmentLength, digitSegmentLength);
}

batCollision()
{
  if(ballControlX < 0 && ballX - ballRadius < batWidth + batSpacing && ballY > leftBatOffset && ballY < leftBatOffset + batLength)
  {
    ballControlX = -ballControlX;
  }

  if(ballControlX > 0 && ballX + ballRadius > zoneWidth - batWidth - batSpacing && ballY > rightBatOffset && ballY < rightBatOffset + batLength)
  {
    ballControlX = -ballControlX;
  }
}

zoneCollision()
{
  if(ballX + ballRadius > zoneWidth)
  {
    ballControlX = -ballControlX;
    leftScore++;
    reset();
  }
  else if(ballX - ballRadius < 0)
  {
    ballControlX = -ballControlX;
    rightScore++;
    reset();
  }
  if(ballY + ballRadius > zoneHeight || ballY - ballRadius < 0)
  {
    ballControlY = -ballControlY;
  }
}

drawZone()
{
  drawRect(0, 0, zoneWidth, zoneHeight);
}

drawBall()
{
  drawCircle(ballX, ballY, ballRadius);
}

moveBall()
{
  ballX += ballControlX * ballSpeed;
  ballY += ballControlY * ballSpeed;
}

drawBats()
{
  drawRect(batSpacing, leftBatOffset, batWidth, batLength);
  drawRect(zoneWidth - batSpacing - batWidth, rightBatOffset, batWidth, batLength);
}
