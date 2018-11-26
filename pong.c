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

#define portA              0xc000 // 0x
#define portB              0x4000

// pong constants
#define zoneWidth 1000
#define zoneHeight 500

#define paddleWidth 20
#define paddleLength 70

#define paddleSpacing 40
// #define paddleMoveStep 7

#define scoresOffset 30
#define digitSegmentLength 25

#define ballSpeed 6
#define ballRadius 5

// global variables
int leftPaddleOffset = 0, rightPaddleOffset = 0;
int leftPaddleControl = 0, rightPaddleControl = 0;
int ballX = 0, ballY = 0;
int ballControlX = 1, ballControlY = 1;
int leftScore = 0, rightScore = 0;

int main()
{
  *PMC_PCER = 0x34; // enable PIOA, ADC and SPI

  LowLevelInit();    // Set up PLL and MCK clocks
  *PIO_PDR = 0x7800; // disable bits 11 - 14 for SPI (See manual section 10.4)
  *PIO_ASR = 0x7800; // enable peripheral A for bits 11 - 14
  *SPI_CR = 0x80;    // reset the SPI
  *SPI_CR = 0x1;     // enable the SPI

  *SPI_MR = 0x11;    // set SPI to master, disable fault detection
  *SPI_CSR0 = 0x183; // set clock polarity = 1, clock phase = 1, Bits per transfer = 16, serial clock baud rate = 1

  *SPI_TDR = 0xd002; // Set reference voltage to 2.048V in control register

  // Inputs
  *ADC_CR = 0x1;    // reset the ADC
  *ADC_CHER = 0x30; // enable analog channels 4 and 5

  *ADC_MR = 0x030b0400; // sample+holdtime = 3, startup = b, prescale = 4

  int i;
  while(1)
  {
    // Handle Input
    handleInput();

    drawZone();
    drawScores();

    drawBall();
    moveBall();

    zoneCollision();

    drawPaddles();

    paddleCollision();
  }

  return 0;
}

handleInput()
{
  *ADC_CR = 0x2;                        // start conversion
  while (*ADC_SR & 0x10 == 0);          // wait for ch4 to complete conversion
  int val1 = *ADC_CDR4;         // store value in ADC_CDR4
  leftPaddleOffset = val1 - 400;   // Limited range of paddle (0 - 350ish) -- CHANGE LENGTH OF PADDLE
  // >>> val1/val2 goes from 105 to 555 (450 range)


  *ADC_CR = 0x2;                        // start conversion
  while (*ADC_SR & 0x20 == 0);          // wait for ch5 to complete conversion
  int val2 = *ADC_CDR5;         // store value in ADC_CDR4
  rightPaddleOffset = val2 - 400;   // Limited range of paddle (0 - 350ish)
}

drawPoint(int x, int y)
{
  x += 500; // Hacky code - pls patch
  y = 400 - y; // Ditto

  while(*SPI_SR & 1 != 1); // Wait for serialisation

  *SPI_TDR = (x << 2) + portB;

  while(*SPI_SR & 1 != 1); // Wait for serialisation

  *SPI_TDR = (y << 2) + portA;
}

 delay(float count)
{
	register int i;
	for (i = count*0x08000000; i > 0; i--);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

reset() {
  ballX = 0;
  ballY = 0;
}

drawScores()
{
  int bias = 10;

  drawCircle(0, -zoneHeight/2 + scoresOffset + bias, 2);
  drawCircle(0, -zoneHeight/2 + scoresOffset + 2*digitSegmentLength - bias, 2);

  drawDigit(-digitSegmentLength - 2*bias -digitSegmentLength, -zoneHeight/2 + scoresOffset, leftScore/10);
  drawDigit(-digitSegmentLength - bias, -zoneHeight/2 + scoresOffset, leftScore % 10);

  drawDigit(bias, -zoneHeight/2 + scoresOffset, rightScore/10);
  drawDigit(2*bias + digitSegmentLength, -zoneHeight/2 + scoresOffset, rightScore % 10);
}

 drawDigit(int x, int y, int n)
{
  if(n == 0 || n == 2 || n == 3 || n == 5 || n == 6 || n == 7 || n == 8 || n == 9) drawHorizontalLine(x, y, digitSegmentLength);
  if(n == 0 || n == 1 || n == 2 || n == 3 || n == 4 || n == 7 || n == 8 || n == 9) drawVerticalLine(x + digitSegmentLength, y, digitSegmentLength);
  if(n == 0 || n == 1 || n == 3 || n == 4 || n == 5 || n == 6 || n == 7 || n == 8 || n == 9) drawVerticalLine(x + digitSegmentLength, y + digitSegmentLength, digitSegmentLength);
  if(n == 0 || n == 2 || n == 3 || n == 5 || n == 6 || n == 8) drawHorizontalLine(x, y + 2*digitSegmentLength, digitSegmentLength);
  if(n == 0 || n == 2 || n == 6 || n == 8) drawVerticalLine(x, y + digitSegmentLength, digitSegmentLength);
  if(n == 0 || n == 4 || n == 5 || n == 6 || n == 8 || n == 9) drawVerticalLine(x, y, digitSegmentLength);
  if(n == 2 || n == 3 || n == 4 || n == 5 || n == 6 || n == 8 || n == 9) drawHorizontalLine(x, y + digitSegmentLength, digitSegmentLength);
}

 paddleCollision()
{
  if (ballX - ballRadius < -zoneWidth/2 + paddleWidth + paddleSpacing && ballY > leftPaddleOffset && ballY < leftPaddleOffset + paddleLength)
  {
    if (ballControlX < 0)
    {
      ballControlX = -ballControlX;
    }
  }

  if (ballX + ballRadius > zoneWidth/2 - paddleWidth - paddleSpacing && ballY > rightPaddleOffset && ballY < rightPaddleOffset + paddleLength)
  {
    if (ballControlX > 0)
    {
      ballControlX = -ballControlX;
    }
  }
}

 zoneCollision()
{
  if ( ballX + ballRadius > zoneWidth/2)
  {
   ballControlX = -ballControlX;
   leftScore++;
   reset();
 }
 else if ( ballX - ballRadius < -zoneWidth/2)
 {
   ballControlX = -ballControlX;
   rightScore++;
   reset();
 }
 if ( ballY + ballRadius > zoneHeight/2 || ballY - ballRadius < -zoneHeight/2 )
 {
   ballControlY = -ballControlY;
 }
}

 drawZone()
{
  drawRect(-zoneWidth/2, -zoneHeight/2, zoneWidth, zoneHeight);
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

 drawPaddles()
{
  drawRect(-zoneWidth/2 + paddleSpacing, leftPaddleOffset, paddleWidth, paddleLength);
  drawRect(zoneWidth/2 - paddleSpacing - paddleWidth, rightPaddleOffset, paddleWidth, paddleLength);
}

// x, y is centre of the circle
 drawCircle(int x, int y, int radius)
{
  int i;
  int j;
  for(i = -radius; i < radius; i++)
  {
    for(j = -radius; j < radius; j++)
    {
      if(i * i + j * j < radius * radius)
      {
          drawPoint(x + i, y + j);
      }
    }
  }
}

// x, y is top left corner of the rectangle
 drawRect(int x, int y, int length, int height)
{
  drawVerticalLine(x, y, height);
  drawVerticalLine(x + length, y, height);
  drawHorizontalLine(x, y, length);
  drawHorizontalLine(x, y + height, length);
}

// draws the line south from x, y
 drawVerticalLine(int x, int y, int length)
{
  int i;
  for(i = 0; i < length; i++)
  {
    drawPoint(x, y + i);
  }
}

// draws the line east from x, y
 drawHorizontalLine(int x, int y, int length)
{
  int i;
  for(i = 0; i < length; i++)
  {
    drawPoint(x + i, y);
  }
}
