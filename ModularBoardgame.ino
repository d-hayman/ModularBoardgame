#include <FastLED.h>  // Include FastLED library
#include <LiquidCrystal_I2C.h>

enum GameState { 
  INIT, 
  PLAYERS_SET_COUNT, 
  PLAYERS_SET_COLORS,
  SPACES_SET_COUNT, 
  PLAYER_TURNS 
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
const int maxPlayers = 2; // TODO: set to 6 once the new board pieces arrive
int playerCount = 1;
int currentPlayer = 0;

Player* players;

#define DATA_PIN 4    // Data pin for LED control
#define COLOR_COUNT 7
CRGB colorChoices[COLOR_COUNT] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Yellow, CRGB::Purple, CRGB::Orange, CRGB::White};
int currentColor = 0;

// SPACE VALUES
const int minSpaces = 2;
const int maxSpaces = 4; // TODO: up to 30
int spaceCount = 2;

const int diceFaces = 3; // TODO: up to 6

// HARDWARE DECLARATIONS
Button enterButton(2);
Button minusButton(13);
Button plusButton(14);

CRGB leds[maxPlayers * maxSpaces];

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
  leds[currentPlayer] = players[currentPlayer].color;
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
  leds[currentPlayer] = players[currentPlayer].color;
  FastLED.show();
}

void decrementColor() {
  currentColor = (currentColor - 1 + COLOR_COUNT) % COLOR_COUNT;
  players[currentPlayer].color = colorChoices[currentColor];
  leds[currentPlayer] = players[currentPlayer].color;
  FastLED.show();
}

void confirmColor() {
  currentPlayer += 1;
  if(currentPlayer >= playerCount){
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
  lcd.setCursor(4, 1);  //Move cursor to character 2 on line 1
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
  currentPlayer = 0;
  displayCurrentPlayer();
  gameState = PLAYER_TURNS;
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

int diceRoll(){
  return (esp_random() % diceFaces) + 1;
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

  for(int x = 0; x < roll; x++){
    delay(500);
    // get the LED at the position index and player subindex
    leds[players[currentPlayer].position * maxPlayers + currentPlayer] = CRGB::Black;
    players[currentPlayer].position = min((players[currentPlayer].position + 1), spaceCount-1);
    leds[players[currentPlayer].position * maxPlayers + currentPlayer] = players[currentPlayer].color;
    FastLED.show();
  }

  delay(500);
  currentPlayer = (currentPlayer + 1) % playerCount;
  displayCurrentPlayer();
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
  case PLAYER_TURNS:
    updateButton(enterButton, playTurn, false);
    break;
  }

}
