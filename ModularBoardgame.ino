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

struct Button {
  const int pin;          // Pin number
  int state;              // Current state
  int lastState;          // Last state
  unsigned long lastDebounceTime; // Last time it was toggled

  Button(int p) : pin(p), state(LOW), lastState(LOW), lastDebounceTime(0) {}
};

const int debounceDelay = 50; // Debounce time in ms
const int longPressDelay = 500; // Long Press Debounce time in ms

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

const int minSpaces = 2;
const int maxSpaces = 4;
int spaceCount = 2;

CRGB leds[maxPlayers * maxSpaces];
Button enterButton(2);
Button minusButton(13);
Button plusButton(14);

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
  currentColor = (currentColor - 1) % COLOR_COUNT;
  players[currentPlayer].color = colorChoices[currentColor];
  leds[currentPlayer] = players[currentPlayer].color;
  FastLED.show();
}

void confirmColor() {
  currentPlayer += 1;
  if(currentPlayer >= playerCount){
    gameState = SPACES_SET_COUNT;
  } else {
    displayColorSelect();
  }
}

void displaySpaceCount() {
  lcd.clear();
  lcd.setCursor(0, 0);  //Set cursor to character 2 on line 0
  lcd.print("Enter # Spaces:");
  lcd.setCursor(4, 1);  //Move cursor to character 2 on line 1
  lcd.print(playerCount);
  lcd.setCursor(5, 1);  //Move cursor to character 2 on line 1
  lcd.print(" spaces");
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
  case PLAYER_TURNS:
  //player turns
  break;
  }

}
