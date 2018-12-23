#include <LedControl.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

#define joyPinX A0
#define joyPinY A1
#define joyPinBtn 8
#define DinPin 12
#define ClkPin 11
#define LoadPin 10
#define LCDContrastPin 9

LedControl lc = LedControl(DinPin, ClkPin, LoadPin, 1); //DIN, CLK, LOAD, No. DRIVER
LiquidCrystal lcd(1, 2, 4, 5, 6, 7); // Creates an LC object. Parameters: (rs, enable, d4, d5, d6, d7);
struct Player {
  int x;
  int y;
  unsigned long score;
  int prevX;
  int prevY;
} player;

struct Platform {
  int y;
  int holeLength;
  int holeX;
} platforms[10];
int nrPlatforms;

enum GameState {
  PLAYING,
  DEATH,
  WAIT_INPUT,
} gameState;

bool moved = false;
bool death = false;
bool toDraw[8][8] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};

// The intervalAscendPlatforms I want the platforms to be moving at 1 row up
unsigned long intervalAscendPlatforms = 1000;
unsigned long intervalScore = 300;
unsigned long previousMillis = 0;
unsigned long previousMillisScore = 0;
unsigned long previousMillisBlinking = 0;
unsigned long intervalBlink = 1000;
float speedAscendPlatforms = 1.25f;


// EEROM declarations
float highScore = 0.00f;
int eeAdress = 0;

// HighScore
int intHighScore = 0;

// Initialise the platforms for the start of the game
Platform initRandomPlatform(int startY) {
  Platform platform = *new Platform;
  platform.y = startY;
  platform.holeLength = random(1, 4);
  platform.holeX = random(0, 8 - platforms[0].holeLength);
  return platform;
}

void calculatePlatforms(Platform platforms[], int nrPlatforms , bool toDraw[8][8]) {
  // Loop through every platform
  for (int i = 0; i < nrPlatforms; i++) {
    // Update the matrix with the platform, we go through the whole row and place 1
    // everywhere except where we want the whole to be in (we place 0 there)
    int platY = platforms[i].y;
    int holeLen = platforms[i].holeLength;
    int holeX = platforms[i].holeX;
    for (int x = 0; x < 8; x++) {
      if ( x < holeX || x > holeX + holeLen) {
        toDraw[platY][x] = 1;
      } else {
        toDraw[platY][x] = 0;
      }
    }
  }
}

void setup()
{
  // Use the random noise from the pin A5 (there's nothing connected to it) as a seed
  randomSeed(analogRead(A5));
  lc.shutdown(0, false); // turn off power saving, enables display
  lc.setIntensity(0, 5); // sets brightness (0~15 possible values)
  lc.clearDisplay(0);// clear screen
  pinMode(joyPinBtn, INPUT);

  EEPROM.get(eeAdress, highScore);
  intHighScore = (int)highScore;
  // LCD
  lcd.begin(16, 2);
  lcd.print("Highscore: " + String(intHighScore));
  // Contrast of LCD
  pinMode(LCDContrastPin, OUTPUT); 
  analogWrite(LCDContrastPin, 100); 
  // -----------------------

  // Initial player position + score
  player.x = 0;
  player.y = 2;
  player.score = 0;
  
  nrPlatforms = 4;
  // Initialize the first 4 platforms that will appear at the start of the game
  for (int i = 0; i < nrPlatforms; i++) {
    // Spawn  the platforms at an odd number of rows because the player will spawn at 0,0
    platforms[i] = initRandomPlatform(2 * i + 1);
  }
  
  // Set the initial state of the game to wait on input from the user
  gameState = WAIT_INPUT;
  
  // Draw the initial positions of all the objects so the player can see the starting scene
  toDraw[player.y][player.x] = 1;
  calculatePlatforms(platforms, nrPlatforms, toDraw);
  drawImage(toDraw);
}

void clearImage(bool DrawTable[8][8]) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      DrawTable[i][j] = 0;
    }
  }
}

void drawImage(bool DrawTable[8][8]) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      lc.setLed(0, i, j, DrawTable[i][j]);
    }
  }
}

// Keeps the player in the playable area (int the 8x8 matrix)
void keepInBound(Player &player) {
  player.x = player.x > 7 ? 7 : player.x;
  player.x = player.x < 0 ? 0 : player.x;

  player.y = player.y > 7 ? 7 : player.y;
  player.y = player.y < 0 ? 0 : player.y;
}

// Reads the input from the joystick and moves the player accordingly
// Also takes care of the collision and the pull from gravity
void processJoyInput(Player &player) {
  int joyX = analogRead(joyPinX);
  int joyY = analogRead(joyPinY);
  int btnIsPressed = digitalRead(joyPinBtn);

  player.prevX = player.x;
  player.prevY = player.y;

  // Movement + collision detection vertically
  if (joyX < 256 && !moved) {
    if (player.x - 1 >= 0 && toDraw[player.y][player.x - 1] != 1) {
      player.x--;
      moved = true;
    }
  }
  if (joyX > 768 && !moved) {
    if (player.x + 1 <= 7 && toDraw[player.y][player.x + 1] != 1) {
      player.x++;
      moved = true;
    }
  }

  // Gravity pulls down the player constantly (y++ means down!)
  if (player.y + 1 <= 7 && toDraw[player.y + 1][player.x] != 1) {
    player.y++;
  }

  // If the joystick moves back to the original position, allow another move
  if ( joyX <= 768 && joyX >= 256 && joyY <= 768 && joyY >= 256) moved = false;

  // Keep the player in the 8x8 square
  keepInBound(player);
  // Mark the position where the player is
  toDraw[player.y][player.x] = 1;
}

// moves all the platforms up by one and also checks if the player was crushed by the ascending platforms
void movePlatformsUp(Platform platforms[], int &nrPlatforms) {
  for (int i = 0; i < nrPlatforms; i++) {
    
    if (player.y == platforms[i].y - 1) {
      player.y--;
      // If the player was "crushed" by the platform, kill him
      if (player.y < 0)
        gameState = DEATH;
    }
    
    platforms[i].y -= 1;
    if (platforms[i].y < 0) {
      platforms[i] = initRandomPlatform(7);
      // If the player is on the last line and another platform comes
      // if the player isn't in the hole of the platform, move the player up
      if (player.y == 7 && (player.x < platforms[i].holeX
                            || player.x > platforms[i].holeX + platforms[i].holeLength)) {
        player.y --;
      }
    }
  }
}

// Draws a big X to symbolize the death of the player
void deathLogo(bool DrawTable[8][8]) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (i == j || i + j == 7)
        DrawTable[i][j] = 1;
    }
  }
}

void loop()
{
  // Used to calculate the intervals
  unsigned long currentMillis = millis();
  
  // To check if the player selected something in the menu
  int joyX = analogRead(joyPinX);
  int joyY = analogRead(joyPinY);
  
  switch (gameState) {
    case WAIT_INPUT:
      
      lcd.setCursor(0, 0);
      lcd.print("Start Game < >");
      lcd.setCursor(0, 1);
      lcd.print("Highscore: " + String(intHighScore));
      
      // If the player moved the joystick any direction start the game
      if ( joyX > 768 || joyX < 256) {
        gameState = PLAYING;
      }
      
      // Bool used when you play again after you died
      death = 0;
      break;

    case PLAYING:
      clearImage(toDraw);
      
      // Verify if "intervalAscendPlatforms" amount of time has passed, if so, move platforms up by 1
      if (currentMillis - previousMillis >= intervalAscendPlatforms) {
        previousMillis = currentMillis;
        movePlatformsUp(platforms, nrPlatforms);
      }

      if (currentMillis - previousMillisScore >= intervalScore) {
        previousMillisScore = currentMillis;
        // The function for calculating the score, this can be changed however one wishes
        player.score += 1 + 3 / intervalAscendPlatforms;
        
        // Decrease how big is the interval for platforms to go up
        if (intervalAscendPlatforms > 100)
          intervalAscendPlatforms = intervalAscendPlatforms - speedAscendPlatforms;
      }
      // Adds the platforms to the matrix to be drawn
      calculatePlatforms(platforms, nrPlatforms, toDraw);

      // Takes user input
      processJoyInput(player);
      drawImage(toDraw);
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Score: " + String(player.score));
      lcd.setCursor(0, 1);
      lcd.print("Speed: " + String(11 - intervalAscendPlatforms / 100));
      
      break;
    case DEATH:
      if (!death) {
        // Display the big X
        clearImage(toDraw);
        deathLogo(toDraw);
        drawImage(toDraw);
        // Show the score the player made
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Close Game ^ v");
        lcd.setCursor(0, 1);
        lcd.print("Score: " + String(player.score));
        // Set the highscore if it's the case
        if (player.score > intHighScore) {
          EEPROM.put(eeAdress, (float)player.score);
        }
        // Reset the local score and the interval
        player.score = 0;
        intervalAscendPlatforms = 1000;
        
        death = 1;
      }

      if (joyY > 768 || joyY < 256) {
        gameState = WAIT_INPUT;
      }

      break;
  }
}
