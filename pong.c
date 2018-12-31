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

// Registers for setting the X and Y-coordinates on the screen
#define portA 0xc000
#define portB 0x4000

// The minimum and maximum values retrieved from the data register for each channel
#define minPaddleValue 105
#define maxPaddleValue 555

// Control constants used in Pong
#define zoneWidth 1000
#define zoneHeight 700
#define batWidth 20
#define batLength 100
#define batSpacing 20
#define scoresOffset 30
#define symbolSegmentLength 30
#define symbolSpacing 20
#define initialSpeed 5
#define speedIncrement 1
#define ballRadius 5
#define winningScore 5

// Initialisation and Declaration of Global variables
int ballSpeed; // Speed of the ball
int leftBatOffset, rightBatOffset; // Vertical bat offsets from the zone bottom
int ballX, ballY; // The coordinates of the ball
int ballControlX = 1, ballControlY = 1; // Control variables for changing ball direction
int leftScore = 0, rightScore = 0; // Respective scores of players 1 and 2

// The code driving the program will be executed here
int main()
{
  // Setting up the registers responsible for handling output
  *PMC_PCER = 0x34;  // enable PIOA, ADC and SPI
  LowLevelInit();    // Set up PLL and MCK clocks
  *PIO_PDR = 0x7800; // disable bits 11 - 14 for SPI (See manual section 10.4)
  *PIO_ASR = 0x7800; // enable peripheral A for bits 11 - 14
  *SPI_CR = 0x80;    // reset the SPI
  *SPI_CR = 0x1;     // enable the SPI
  *SPI_MR = 0x11;    // set SPI to master, disable fault detection
  *SPI_CSR0 = 0x183; // set clock polarity = 1, clock phase = 1, Bits per transfer = 16, serial clock baud rate = 1
  *SPI_TDR = 0xd002; // Set reference voltage to 2.048V in control register

  // Setting up the registers responsible for handling input
  *ADC_CR = 0x1;        // reset the ADC
  *ADC_CHER = 0x30;     // enable analog channels 4 and 5
  *ADC_MR = 0x030b0400; // sample+holdtime = 3, startup = b, prescale = 4

  // Reset ball properties and position
  reset();

  // The main game loop
  while(1)
  {
    input();
    update();
    render();
  }

  return 0; // Return 0 if program terminates successfully
}

// This method receives input from the paddles
input()
{
  *ADC_CR = 0x2;                        // start conversion
  while (*ADC_SR & 0x10 == 0);          // wait for ch4 to complete conversion
  int leftPaddleValue = *ADC_CDR4;      // retrieve value from ADC_CDR4 register

  // Linearly interpolate the value from the left paddle
  // to the appropriate range based on the zone height
  leftBatOffset = (leftPaddleValue - minPaddleValue)
                 *(zoneHeight - batLength)
                 /(maxPaddleValue - minPaddleValue);

  *ADC_CR = 0x2;                        // start conversion
  while (*ADC_SR & 0x20 == 0);          // wait for ch5 to complete conversion
  int rightPaddleValue = *ADC_CDR5;     // retrieve value from ADC_CDR5 register

  // Linearly interpolate the value from the right paddle
  // to the appropriate range based on the zone height
  rightBatOffset = (rightPaddleValue - minPaddleValue)
                  *(zoneHeight - batLength)
                  /(maxPaddleValue - minPaddleValue);
}

// This method performs any calculations prior to updating the screen
update()
{
  moveBall();
  zoneCollision();
  batCollision();
}

// This method renders the screen according to any changes in the game
render()
{
  if (leftScore >= winningScore)
  {
    drawWinner(1); // Show that Player 1 wins
  }
  else if (rightScore >= winningScore)
  {
    drawWinner(2); // Show that Player 2 wins
  }
  else if ((leftScore >= (winningScore + 3)) || (rightScore >= (winningScore + 3)))
  {
    // Reset the game some time after displaying who the winner was
    leftScore = 0, rightScore = 0;
    ballControlX = 1, ballControlY = 1;
  }
  
  if (leftScore < winningScore && rightScore < winningScore) // Continue running the game
  {
    drawZone();
    drawScores();
    drawBats();
    drawBall();
  }
}

// This method draws a point on the screen at position (x, y)
drawPoint(int x, int y)
{
  // Check that x and y have values that can be represented by a 10-bit unsigned integer
  if(x >= 0 && x < 2048 && y >= 0 && y < 2048) // Comparison involves denary values
  {
    /* For both x and y, perform a left bit-shift operation twice (multiply by 4) to
     * move the coordinate value to the appropriate placeholder bits (D02-D11). This
     * also implicitly assigns a 0 to bits D00 and D01. Subsequently, add the appropriate
     * constant, whose relevant control bits are set (D12-D15), and transmit the two
     * least significant bytes of the resulting value to the SPI_TDR register. */

    while(*SPI_SR & 1 != 1);     // Wait for serialisation
    *SPI_TDR = (x << 2) + portB; // Transmit to the SPI_TDR register

    while(*SPI_SR & 1 != 1);     // Wait for serialisation
    *SPI_TDR = (y << 2) + portA; // Transmit to the SPI_TDR register
  }
}

// This method draws a line of length l vertically upwards from (x, y)
drawVerticalLine(int x, int y, int l)
{
  int i;
  for(i = 0; i < l; i++)
  {
    drawPoint(x, y + i);
  }
}

// This method draws a horizontal line of length l to the right from (x, y)
drawHorizontalLine(int x, int y, int l)
{
  int i;
  for(i = 0; i < l; i++)
  {
    drawPoint(x + i, y);
  }
}

// This method draws a rectangle whose bottom-left corner is (x, y)
drawRect(int x, int y, int length, int height)
{
  drawVerticalLine(x, y, height);
  drawVerticalLine(x + length, y, height);
  drawHorizontalLine(x, y, length);
  drawHorizontalLine(x, y + height, length);
}

// This method draws a circle of radius r centred at (x, y)
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

// This method resets fundamental ball properties
reset()
{
  ballX = zoneWidth/2;
  ballY = zoneHeight/2;
  ballSpeed = initialSpeed;
}

// This method renders the game scores
drawScores()
{
  // Draws the colon which keeps the scores separate
  drawCircle(zoneWidth/2, zoneHeight - scoresOffset - symbolSpacing, 2);
  drawCircle(zoneWidth/2, zoneHeight - scoresOffset - symbolSegmentLength*2 + symbolSpacing, 2);

  // Draws the left player's score (Player 1)
  drawDigit(zoneWidth/2 - symbolSegmentLength*2 - symbolSpacing*2, zoneHeight - symbolSegmentLength*2 - scoresOffset, leftScore/10);
  drawDigit(zoneWidth/2 - symbolSegmentLength - symbolSpacing, zoneHeight - symbolSegmentLength*2 - scoresOffset, leftScore%10);

  // Draws the right player's score (Player 2)
  drawDigit(zoneWidth/2 + symbolSpacing, zoneHeight - symbolSegmentLength*2 - scoresOffset, rightScore/10);
  drawDigit(zoneWidth/2 + symbolSpacing*2 + symbolSegmentLength, zoneHeight - symbolSegmentLength*2 - scoresOffset, rightScore%10);
}

/*
 * This method draws a digit using a 7-segment display.
 * (x, y) must correspond to the bottom-left of the
 * digit, whereas n is equivalent to the digit value.
 */
drawDigit(int x, int y, int n)
{
  // Perform the requisite comparisons to determine what to draw
  n %= 10;
  if(n != 1 && n != 4)                      drawHorizontalLine(x, y + symbolSegmentLength*2, symbolSegmentLength);
  if(n != 5 && n != 6)                      drawVerticalLine(x + symbolSegmentLength, y + symbolSegmentLength, symbolSegmentLength);
  if(n != 2)                                drawVerticalLine(x + symbolSegmentLength, y, symbolSegmentLength);
  if(n != 1 && n != 4 && n != 7 && n != 9)  drawHorizontalLine(x, y, symbolSegmentLength);
  if(n == 0 || n == 2 || n == 6 || n == 8)  drawVerticalLine(x, y, symbolSegmentLength);
  if(n != 1 && n != 2 && n != 3 && n != 7)  drawVerticalLine(x, y + symbolSegmentLength, symbolSegmentLength);
  if(n != 0 && n != 1 && n != 7)            drawHorizontalLine(x, y + symbolSegmentLength, symbolSegmentLength);
}

// Method responsible for communicating which player won
drawWinner(int winner)
{
  // Determine the width of the endgame message
  int messageWidth = (symbolSegmentLength*8 + symbolSpacing*4)/2;

  // Draw the letter P (for 'Player')
  drawVerticalLine(zoneWidth/2 - messageWidth, zoneHeight/2 - symbolSegmentLength, symbolSegmentLength);
  drawVerticalLine(zoneWidth/2 - messageWidth, zoneHeight/2, symbolSegmentLength);
  drawHorizontalLine(zoneWidth/2 - messageWidth, zoneHeight/2, symbolSegmentLength);
  drawHorizontalLine(zoneWidth/2 - messageWidth, zoneHeight/2 + symbolSegmentLength, symbolSegmentLength);
  drawVerticalLine(zoneWidth/2 - messageWidth + symbolSegmentLength, zoneHeight/2, symbolSegmentLength);

  // Display a digit corresponding to the winning player
  drawDigit(zoneWidth/2 - messageWidth + symbolSegmentLength + symbolSpacing, zoneHeight/2 - symbolSegmentLength, winner);

  // Draw the letter W
  drawVerticalLine(zoneWidth/2 - messageWidth + symbolSegmentLength*4 + symbolSpacing, zoneHeight/2 - symbolSegmentLength, symbolSegmentLength*2);
  drawVerticalLine(zoneWidth/2 - messageWidth + (int)(symbolSegmentLength*4.5) + symbolSpacing, zoneHeight/2 - symbolSegmentLength, symbolSegmentLength*2);
  drawVerticalLine(zoneWidth/2 - messageWidth + symbolSegmentLength*5 + symbolSpacing, zoneHeight/2 - symbolSegmentLength, symbolSegmentLength*2);
  drawHorizontalLine(zoneWidth/2 - messageWidth + symbolSegmentLength*4 + symbolSpacing, zoneHeight/2 - symbolSegmentLength, symbolSegmentLength);

  // Draw the letter I
  drawVerticalLine(zoneWidth/2 - messageWidth + (int)(symbolSegmentLength*5.5) + symbolSpacing*2, zoneHeight/2 - symbolSegmentLength, symbolSegmentLength*2);

  // Draw the letter N
  drawVerticalLine(zoneWidth/2 - messageWidth + symbolSegmentLength*6 + symbolSpacing*3, zoneHeight/2 - symbolSegmentLength, symbolSegmentLength*2);
  drawHorizontalLine(zoneWidth/2 - messageWidth + symbolSegmentLength*6 + symbolSpacing*3, zoneHeight/2 + symbolSegmentLength, symbolSegmentLength);
  drawVerticalLine(zoneWidth/2 - messageWidth + symbolSegmentLength*7 + symbolSpacing*3, zoneHeight/2 - symbolSegmentLength, symbolSegmentLength*2);

  // Draw the letter S
  drawDigit(zoneWidth/2 - messageWidth + symbolSegmentLength*7 + symbolSpacing*4, zoneHeight/2 - symbolSegmentLength, 5);
}

// Method responsible for detecting and responding
// to collisions between the ball and bats
batCollision()
{
  // If the ball strikes the left bat, rebound the ball to the right
  if(ballControlX < 0 && ballX - ballRadius < batWidth + batSpacing && ballY > leftBatOffset && ballY < leftBatOffset + batLength)
  {
    ballControlX = -ballControlX;
    increaseSpeed();
  }

  // If the ball strikes the right bat, rebound the ball to the left
  if(ballControlX > 0 && ballX + ballRadius > zoneWidth - batWidth - batSpacing && ballY > rightBatOffset && ballY < rightBatOffset + batLength)
  {
    ballControlX = -ballControlX;
    increaseSpeed();
  }
}

// Method responsible for increasing the ball speed
// following its collision with a bat
increaseSpeed()
{
  ballSpeed += speedIncrement;
  int maxSpeed = batWidth + batSpacing - 3;

  // Limit the maximum speed
  if(ballSpeed > maxSpeed)
  {
    ballSpeed = maxSpeed;
  }
}

// Method responsible for detecting and responding
// to collisions between the ball and walls
zoneCollision()
{
  // Check if the ball exits the game zone by moving right
  if(ballX + ballRadius > zoneWidth)
  {
    ballControlX = -ballControlX;
    leftScore++;
    reset();
  }
  // Check if the ball exits the game zone by moving left
  else if(ballX - ballRadius < 0)
  {
    ballControlX = -ballControlX;
    rightScore++;
    reset();
  }

  // Check if the ball exits the game zone by moving either up or down
  if(ballY + ballRadius > zoneHeight || ballY - ballRadius < 0)
  {
    ballControlY = -ballControlY;
  }
}

// Method responsible for rendering the borders of the game
drawZone()
{
  drawRect(0, 0, zoneWidth, zoneHeight);
}

// Method responsible for rendering the ball
drawBall()
{
  drawCircle(ballX, ballY, ballRadius);
}

// Method responsible for updating the ball's position
moveBall()
{
  ballX += ballControlX * ballSpeed;
  ballY += ballControlY * ballSpeed;
}

// Method responsible for rendering the bats
drawBats()
{
  drawRect(zoneWidth - batSpacing - batWidth, rightBatOffset, batWidth, batLength);
  drawRect(batSpacing, leftBatOffset, batWidth, batLength);
}
