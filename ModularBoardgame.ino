#include <FastLED.h>  // Include FastLED library
#include <LiquidCrystal_I2C.h>

struct Button {
  const int pin;          // Pin number
  int state;              // Current state
  int lastState;          // Last state
  unsigned long lastDebounceTime; // Last time it was toggled

  Button(int p) : pin(p), state(LOW), lastState(LOW), lastDebounceTime(0) {}
};

const int debounceDelay = 50; // Debounce time in ms

int players = 1;

enum GameState { INIT, SETUPPLAYERS, PLAYER_TURNS };
GameState gameState = INIT;

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
}

void updateButton(Button &btn, void (*callback)()) {
  int reading = digitalRead(btn.pin);

  if (reading != btn.lastState) {
    btn.lastDebounceTime = millis();
  }

  if ((millis() - btn.lastDebounceTime) > debounceDelay) {
    if (reading != btn.state) {
      btn.state = reading;
      if (btn.state == HIGH) {
        callback();
      }
    }
  }

  btn.lastState = reading;
}

void displayPlayers() {
  lcd.clear();
  lcd.setCursor(1, 0);  //Set cursor to character 2 on line 0
  lcd.print("Enter Players:");
  lcd.setCursor(4, 1);  //Move cursor to character 2 on line 1
  lcd.print(players);
  lcd.setCursor(5, 1);  //Move cursor to character 2 on line 1
  lcd.print(" players");
}

void incrementPlayers() {
  if (players < 6)
    players++;

  displayPlayers();
  //Serial.print(players);
  //Serial.println(" players");
}

void decrementPlayers() {
  if (players > 1)
    players--;

  displayPlayers();
  //Serial.print(players);
  //Serial.println(" players");
}

void loop() {
  switch (gameState) {
  case INIT:
    displayPlayers();
    gameState = SETUPPLAYERS;
    break;
  case SETUPPLAYERS:
    updateButton(plusButton, incrementPlayers);
    updateButton(minusButton, decrementPlayers);
    break;
  case PLAYER_TURNS:
  //player turns
  break;
  }

}
