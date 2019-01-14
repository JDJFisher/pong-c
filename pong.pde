// Constants used to control Pong
final int zoneWidth = 800;
final int zoneHeight = 500;
final int batWidth = 20;
final int batLength = 100;
final int batSpacing = 20;
final int scoresOffset = 30;
final int symbolSegmentLength = 30;
final int symbolSpacing = 20;
final int ballRadius = 5;
final int batMoveStep = 10;
final int winningScore = 5;
final int scoreBeforeReset = winningScore + 3;
final int speedIncrement = 1;
final int initialSpeed = 5;
final int maxSpeed = batWidth + batSpacing - 3;

// Initialisation and Declaration of Global variables
int ballSpeed; // Speed of the ball
int winner;
int leftBatOffset, rightBatOffset; // Vertical bat offsets from the zone bottom
int ballX, ballY; // The coordinates of the ball
int ballControlX = 1, ballControlY = 1; // Control variables for changing ball direction
int leftScore, rightScore; // Respective scores of players 1 and 2
int leftBatControl = 0, rightBatControl = 0;

void setup()
{
  size(900, 600);
  noFill();
  stroke(255);
  resetGame();
}

void draw()
{
  background(10, 10, 10);
  translate(50, 550);

  // Mainloop
  input();
  update();
  render();
}

// This method receives input from the paddles
void input()
{
}

// This method performs any calculations prior to updating the screen
void update()
{
  moveBall();
  zoneCollision();

  if(leftScore >= winningScore)
  {
    winner = 1;
    ballControlX = 1;    // Specify ball direction

    if(leftScore >= scoreBeforeReset)
    {
      resetGame();
    }
  }
  else if (rightScore >= winningScore)
  {
    winner = 2;
    ballControlX = -1;   // Specify ball direction

    if(rightScore >= scoreBeforeReset)
    {
      resetGame();
    }
  }
  else
  {
    movePaddles();
    batCollision();
  }
}

// This method renders the screen according to any changes in the game
void render()
{
  if(winner == 0)
  {
    drawZone();
    drawScores();
    drawBats();
    drawBall();
  }
  else
  {
    drawWinner();
  }
}

void drawPoint(int x, int y)
{
  point(x,-y);
}

// This method draws a line of length l vertically upwards from (x, y)
void drawVerticalLine(int x, int y, int l)
{
  int i;
  for(i = 0; i < l; i++)
  {
    drawPoint(x, y + i);
  }
}

// This method draws a horizontal line of length l to the right from (x, y)
void drawHorizontalLine(int x, int y, int l)
{
  int i;
  for(i = 0; i < l; i++)
  {
    drawPoint(x + i, y);
  }
}

// This method draws a rectangle whose bottom-left corner is (x, y)
void drawRect(int x, int y, int length, int height)
{
  drawVerticalLine(x, y, height);
  drawVerticalLine(x + length, y, height);
  drawHorizontalLine(x, y, length);
  drawHorizontalLine(x, y + height, length);
}

// This method draws a circle of radius r centred at (x, y)
void drawCircle(int x, int y, int r)
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
void resetBall()
{
  ballX = zoneWidth/2;
  ballY = zoneHeight/2;
  ballSpeed = initialSpeed;
}

//
void resetGame()
{
  leftScore = 0; rightScore = 0;
  winner = 0;
  resetBall();
}

// This method renders the game scores
void drawScores()
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
void drawDigit(int x, int y, int n)
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
void drawWinner()
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
void batCollision()
{
  // If the ball strikes a bat, rebound it in the opposite direction and increase the ball's speed
  if(
    // Check the left bat
    (ballControlX < 0 && ballX - ballRadius < batWidth + batSpacing && ballY > leftBatOffset && ballY < leftBatOffset + batLength) ||
    // Check the right bat
    (ballControlX > 0 && ballX + ballRadius > zoneWidth - batWidth - batSpacing && ballY > rightBatOffset && ballY < rightBatOffset + batLength)
  )
  {
    ballControlX = -ballControlX;
    increaseSpeed();
  }
}

// Method responsible for increasing the ball speed
// following its collision with a bat
void increaseSpeed()
{
  ballSpeed += speedIncrement;

  // Limit the maximum speed
  if(ballSpeed > maxSpeed)
  {
    ballSpeed = maxSpeed;
  }
}

// Method responsible for detecting and responding
// to collisions between the ball and walls
void zoneCollision()
{
  // Check if the ball exits the game zone by moving right
  if(ballX + ballRadius > zoneWidth)
  {
    ballControlX = -ballControlX;
    leftScore++;
    resetBall();
  }
  // Check if the ball exits the game zone by moving left
  else if(ballX - ballRadius < 0)
  {
    ballControlX = -ballControlX;
    rightScore++;
    resetBall();
  }

  // Check if the ball exits the game zone by moving either up or down
  if(ballY + ballRadius > zoneHeight || ballY - ballRadius < 0)
  {
    ballControlY = -ballControlY;
  }
}

// Method responsible for rendering the borders of the game
void drawZone()
{
  drawRect(0, 0, zoneWidth, zoneHeight);
}

// Method responsible for rendering the ball
void drawBall()
{
  drawCircle(ballX, ballY, ballRadius);
}

// Method responsible for updating the ball's position
void moveBall()
{
  ballX += ballControlX * ballSpeed;
  ballY += ballControlY * ballSpeed;
}

// Method responsible for rendering the bats
void drawBats()
{
  drawRect(zoneWidth - batSpacing - batWidth, rightBatOffset, batWidth, batLength);
  drawRect(batSpacing, leftBatOffset, batWidth, batLength);
}

void keyPressed()
{
  if (key == 'w' || key == 'W' || key == 'u' || key == 'U')
  {
    leftBatControl = 1;
  }
  if (key == 's' || key == 'S' || key == 'j' || key == 'J')
  {
    leftBatControl = -1;
  }
  if (keyCode == UP || key == 'o' || key == 'O')
  {
    rightBatControl = 1;
  }
  if (keyCode == DOWN || key == 'l' || key == 'L')
  {
    rightBatControl = -1;
  }
}

void keyReleased()
{
  if (key == 'w' || key == 'W' || key == 'u' || key == 'U')
  {
    leftBatControl = 0;
  }
  if (key == 's' || key == 'S' || key == 'j' || key == 'J')
  {
    leftBatControl = 0;
  }
  if (keyCode == UP || key == 'o' || key == 'O')
  {
    rightBatControl = 0;
  }
  if (keyCode == DOWN || key == 'l' || key == 'L')
  {
    rightBatControl = 0;
  }
}

void movePaddles()
{
  leftBatOffset += batMoveStep * leftBatControl;
  rightBatOffset += batMoveStep * rightBatControl;

  if(leftBatOffset < 0)
  {
    leftBatOffset = 0;
  }
  else if(leftBatOffset + batLength > zoneHeight)
  {
    leftBatOffset = zoneHeight - batLength;
  }

  if(rightBatOffset < 0)
  {
    rightBatOffset = 0;
  }
  else if(rightBatOffset + batLength > zoneHeight)
  {
    rightBatOffset = zoneHeight - batLength;
  }
}
