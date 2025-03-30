#include <FastLED.h>  // Include FastLED library
#include <LiquidCrystal_I2C.h>

enum GameState { 
  INIT, 
  PLAYERS_SET_COUNT, 
  PLAYERS_SET_COLORS,
  SPACES_SET_COUNT, 
  SPACES_SET_EFFECTS, 
  SPACES_SET_DESTINATION, 
  PLAYER_TURNS,
  GAME_OVER
  };
GameState gameState = INIT;

// BUTTON VALUES
struct Button {
  const int pin;          // Pin number
  int state;              // Current state
  int lastState;          // Last state
  unsigned long lastDebounceTime; // Last time it was toggled

  Button(int p) : pin(p), state(LOW), lastState(LOW), lastDebounceTime(0) {}
};

const int debounceDelay = 50; // Debounce time in ms
const int longPressDelay = 500; // Long Press Debounce time in ms

// PLAYER VALUES
struct Player {
  int position;
  CRGB color;

  Player() : position(0), color(CRGB::Red) {}
};

const int minPlayers = 1;
const int maxPlayers = 6; 
int playerCount = minPlayers;
int currentPlayer = 0;

Player* players;

#define DATA_PIN 4    // Data pin for LED control
#define COLOR_COUNT 7
CRGB colorChoices[COLOR_COUNT] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Yellow, CRGB::Purple, CRGB::Orange, CRGB::White};
int currentColor = 0;

// SPACE VALUES
const int minSpaces = 4;
const int maxSpaces = 30; 
int spaceCount = minSpaces;

uint32_t* spacesDests; // array of destination indices when landing on a space. i=j for no action
int currentSpace = 1;
int currentDest = 0;

const int diceFaces = 6; 

// HARDWARE DECLARATIONS
Button enterButton(2);
Button minusButton(13);
Button plusButton(14);

CRGB leds[(1+maxPlayers) * maxSpaces];

LiquidCrystal_I2C lcd(0x27, 16, 2);  // initialize the Liquid Crystal Display object with the I2C address 0x27, 16 columns and 2 rows

void setup() {
  //Serial.begin(9600);
  pinMode(plusButton.pin, INPUT);
  pinMode(minusButton.pin, INPUT);
  pinMode(enterButton.pin, INPUT);

  lcd.init();       // initialize the LCD
  lcd.clear();      // clear the LCD display
  lcd.backlight();  // Make sure backlight is on

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, maxPlayers * maxSpaces);  // Initialize LEDs
  FastLED.setBrightness(5);
  FastLED.show();
}

void updateButton(Button &btn, void (*callback)(), bool allowHold = false) {
  int reading = digitalRead(btn.pin);

  if (reading != btn.lastState) {
    btn.lastDebounceTime = millis();
  }

  if ((millis() - btn.lastDebounceTime) > debounceDelay) {
    if (reading == HIGH && btn.state == LOW) { // Initial press
      callback();
      btn.lastDebounceTime = millis(); // Reset for long press handling
    }

    if (allowHold && reading == HIGH && (millis() - btn.lastDebounceTime) > longPressDelay) { // Long press repeat
      callback();
      btn.lastDebounceTime = millis(); // Reset repeat timing
    }

    btn.state = reading;
  }

  btn.lastState = reading;
}

// PLAYER CONFIG
void displayPlayers() {
  lcd.clear();
  lcd.setCursor(1, 0);  //Set cursor to character 2 on line 0
  lcd.print("Enter Players:");
  lcd.setCursor(4, 1);  //Move cursor to character 2 on line 1
  lcd.print(playerCount);
  lcd.setCursor(5, 1);  //Move cursor to character 2 on line 1
  lcd.print(" players");
}

void incrementPlayers() {
  if (playerCount < maxPlayers)
    playerCount++;

  displayPlayers();
  //Serial.print(players);
  //Serial.println(" players");
}

void decrementPlayers() {
  if (playerCount > minPlayers)
    playerCount--;

  displayPlayers();
  //Serial.print(players);
  //Serial.println(" players");
}

void confirmPlayers() {
  players = new Player[playerCount];
  displayColorSelect();
  gameState = PLAYERS_SET_COLORS;
}

// PLAYER COLOR CONFIG
void displayColorSelect() {
  players[currentPlayer] = Player();
  currentColor = 0;
  leds[1+currentPlayer] = players[currentPlayer].color;
  FastLED.show();
  lcd.clear();
  lcd.setCursor(1, 0);  //Set cursor to character 2 on line 0
  lcd.print("Select Color:");
  lcd.setCursor(4, 1);  //Move cursor to character 2 on line 1
  lcd.print("Player ");
  lcd.setCursor(11, 1);  //Move cursor to character 2 on line 1
  lcd.print(currentPlayer + 1);
}

void incrementColor() {
  currentColor = (currentColor + 1) % COLOR_COUNT;
  players[currentPlayer].color = colorChoices[currentColor];
  leds[1+currentPlayer] = players[currentPlayer].color;
  FastLED.show();
}

void decrementColor() {
  currentColor = (currentColor - 1 + COLOR_COUNT) % COLOR_COUNT;
  players[currentPlayer].color = colorChoices[currentColor];
  leds[1+currentPlayer] = players[currentPlayer].color;
  FastLED.show();
}

void confirmColor() {
  currentPlayer += 1;
  if(currentPlayer >= playerCount){
    currentPlayer = 0; // reset current player
    displaySpaceCount();
    gameState = SPACES_SET_COUNT;
  } else {
    displayColorSelect();
  }
}

// SPACES CONFIG
void displaySpaceCount() {
  lcd.clear();
  lcd.setCursor(0, 0);  //Set cursor to character 2 on line 0
  lcd.print("Enter # Spaces:");
  lcd.setCursor(4-(int)log10(spaceCount), 1);  //Move cursor to character 2 on line 1
  lcd.print(spaceCount);
  lcd.setCursor(5, 1);  //Move cursor to character 2 on line 1
  lcd.print(" spaces");
}

void incrementSpaces() {
  if (spaceCount < maxSpaces)
    spaceCount++;

  displaySpaceCount();
}

void decrementSpaces() {
  if (spaceCount > minSpaces)
    spaceCount--;

  displaySpaceCount();
}

void confirmSpaces() {
  spacesDests = new uint32_t[spaceCount];
  for (int x = 0; x < spaceCount; x++)
    spacesDests[x] = x;
  currentSpace = 1;
  displayEffect();
  gameState = SPACES_SET_EFFECTS;
}

// EFFECT CONFIG
void renderSpaces(bool callShow = false) {
  for (int x = 0; x < spaceCount; x++) {
    if(spacesDests[x] < x)
      leds[x * (1+maxPlayers)] = CRGB::Red;
    else if (spacesDests[x] > x)
      leds[x * (1+maxPlayers)] = CRGB::Green;
    else
      leds[x * (1+maxPlayers)] = CRGB::Black;
  }

  if (callShow)
    FastLED.show();
}

void displayEffect() {
  renderSpaces();
  leds[currentSpace * (1+maxPlayers)] = CRGB::Blue;
  FastLED.show();

  lcd.clear();
  if (currentSpace > 0) {
    lcd.setCursor(0, 0);  
    lcd.print("Select Space...");
  } else {
    lcd.setCursor(2, 0);  
    lcd.print("START GAME!");
  }
}

void incrementEffects() {
  if(currentSpace < (spaceCount - 2)) // allow effects up to the penultimate space
    currentSpace ++;

  displayEffect();
}

void decrementEffects() {
  if(currentSpace > 0) // selecting index 0 allows players to exit config
    currentSpace --;

  displayEffect();
}

void confirmEffects() {
  if (currentSpace > 0) {
    currentDest = currentSpace;
    displayDestination();
    gameState = SPACES_SET_DESTINATION;
  }
  else{
    renderSpaces(true);
    displayCurrentPlayer();
    gameState = PLAYER_TURNS;
  }
}

// DESTINATION CONFIG
void displayDestination() {
  renderSpaces();
  leds[currentDest * (1+maxPlayers)] = CRGB::Yellow;
  leds[currentSpace * (1+maxPlayers)] = CRGB::Blue;
  FastLED.show();

  lcd.clear();
  lcd.setCursor(5, 0);  
  lcd.print("Select");
  lcd.setCursor(1, 1);  
  lcd.print("Destination...");
}

void incrementDestination() {
  if(currentDest < (spaceCount - 1)) // allow destinations up to the last space
    currentDest ++;

  displayDestination();
}

void decrementDestination() {
  if(currentDest > 0)
    currentDest --;

  displayDestination();
}

void confirmDestination() {
  spacesDests[currentSpace] = currentDest;
  displayEffect();
  gameState = SPACES_SET_EFFECTS;
}

// GAME LOOP
void displayCurrentPlayer() {
  lcd.clear();
  lcd.setCursor(1, 0);  //Set cursor to character 2 on line 0
  lcd.print("Press to roll:");
  lcd.setCursor(4, 1);  //Move cursor to character 2 on line 1
  lcd.print("Player ");
  lcd.setCursor(11, 1);  //Move cursor to character 2 on line 1
  lcd.print(currentPlayer + 1);
}

int diceRoll() {
  return (esp_random() % diceFaces) + 1;
}

void movePlayer(int roll) {
  if(roll > 0) {
    for(int x = 0; x < roll; x++){
      delay(500);
      // get the LED at the position index and player subindex
      leds[players[currentPlayer].position * (1+maxPlayers) + 1+currentPlayer] = CRGB::Black;
      players[currentPlayer].position = min((players[currentPlayer].position + 1), spaceCount-1);
      leds[players[currentPlayer].position * (1+maxPlayers) + 1+currentPlayer] = players[currentPlayer].color;
      FastLED.show();
    }
  } else if (roll < 0) {
    for(int x = 0; x < abs(roll); x++){
      delay(500);
      // get the LED at the position index and player subindex
      leds[players[currentPlayer].position * (1+maxPlayers) + 1+currentPlayer] = CRGB::Black;
      players[currentPlayer].position = max((players[currentPlayer].position - 1), 0);
      leds[players[currentPlayer].position * (1+maxPlayers) + 1+currentPlayer] = players[currentPlayer].color;
      FastLED.show();
    }
  }
}

void playTurn() {
  lcd.clear();
  lcd.setCursor(3, 0);  //Set cursor to character 2 on line 0
  lcd.print("Rolling");
  delay(250);
  lcd.print(".");
  delay(250);
  lcd.print(".");
  delay(250);
  lcd.print(".");
  delay(250);

  int roll = diceRoll();

  lcd.clear();
  lcd.setCursor(3, 0);  //Set cursor to character 2 on line 0
  lcd.print("You rolled:");
  lcd.setCursor(8, 1);  //Move cursor to character 2 on line 1
  lcd.print(roll);

  // Uncomment to add the perfect roll endgame rule
  // if ((players[currentPlayer].position + roll) > (spaceCount-1)){
  //   //skip turn
  //   lcd.clear();
  //   lcd.setCursor(4, 0);  
  //   lcd.print("Too high");
  //   delay(1500);
  //   return;
  // }

  movePlayer(roll);

  // if the space the player landed on after rolling is an effect space then trigger the effect
  if(spacesDests[players[currentPlayer].position] < players[currentPlayer].position) {
    lcd.clear();
    lcd.setCursor(3, 0);  //Set cursor to character 2 on line 0
    lcd.print("Move back:");
    lcd.setCursor(8, 1);  //Move cursor to character 2 on line 1
    lcd.print(players[currentPlayer].position - spacesDests[players[currentPlayer].position]);
    delay(500);
    movePlayer(spacesDests[players[currentPlayer].position] - players[currentPlayer].position); // negative

  } else if (spacesDests[players[currentPlayer].position] > players[currentPlayer].position) {
    lcd.clear();
    lcd.setCursor(1, 0);  //Set cursor to character 2 on line 0
    lcd.print("Move forward:");
    lcd.setCursor(8, 1);  //Move cursor to character 2 on line 1
    lcd.print(spacesDests[players[currentPlayer].position] - players[currentPlayer].position);
    delay(500);
    movePlayer(spacesDests[players[currentPlayer].position] - players[currentPlayer].position); // positive
  }

  // game over
  if(players[currentPlayer].position >= (spaceCount-1)){
    displayGameOver();
    gameState = GAME_OVER;
  // next turn
  } else {
    delay(500);
    currentPlayer = (currentPlayer + 1) % playerCount;
    displayCurrentPlayer();
  }
}

// GAME OVER
void displayGameOver() {
  lcd.clear();
  lcd.setCursor(3, 0);  
  lcd.print("GAME OVER!");
  lcd.setCursor(3, 1);  
  lcd.print("Winner: ");
  lcd.setCursor(11, 1);  
  lcd.print(currentPlayer + 1);
}

void restart() {
  currentPlayer = 0;
  for(int x = 0; x < (1+maxPlayers) * maxSpaces; x++){
    leds[x] = CRGB::Black;
  }
  // FastLED.show(); // I'm fine with letting the end state of the last game linger a bit longer
  displayPlayers();
  gameState = PLAYERS_SET_COUNT;
}

void loop() {
  switch (gameState) {
  case INIT:
    displayPlayers();
    gameState = PLAYERS_SET_COUNT;
    break;
  case PLAYERS_SET_COUNT:
    updateButton(plusButton, incrementPlayers, true);
    updateButton(minusButton, decrementPlayers, true);
    updateButton(enterButton, confirmPlayers, false);
    break;
  case PLAYERS_SET_COLORS:
    updateButton(plusButton, incrementColor, true);
    updateButton(minusButton, decrementColor, true);
    updateButton(enterButton, confirmColor, false);
    break;
  case SPACES_SET_COUNT:
    updateButton(plusButton, incrementSpaces, true);
    updateButton(minusButton, decrementSpaces, true);
    updateButton(enterButton, confirmSpaces, false);
    break;
  case SPACES_SET_EFFECTS:
    updateButton(plusButton, incrementEffects, true);
    updateButton(minusButton, decrementEffects, true);
    updateButton(enterButton, confirmEffects, false);
    break;
  case SPACES_SET_DESTINATION:
    updateButton(plusButton, incrementDestination, true);
    updateButton(minusButton, decrementDestination, true);
    updateButton(enterButton, confirmDestination, false);
    break;
  case PLAYER_TURNS:
    updateButton(enterButton, playTurn, false);
    break;
  case GAME_OVER:
    updateButton(enterButton, restart, false);
    break;
  }

}
