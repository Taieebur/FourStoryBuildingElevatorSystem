// C++ code
//
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

static uint8_t PushButton[4] = {1,2,4,5};
static uint8_t IndicatorLED[4] = {6,7,8,9};
static uint8_t MotorIn1 = 12;
static uint8_t MotorIn2	= 13;
static uint8_t PresentFloor = 0;
//static int8_t UserFloor = -1;
static uint8_t FloorQueue = 0;  // each bit = one floor requested
static const uint8_t DoorIn1 = 10;
static const uint8_t DoorIn2 = 11;
static const uint8_t LimitOpen = A0;    // HIGH = door fully open
static const uint8_t LimitClose = A1;   // HIGH = door fully closed
static const uint8_t ForceSensor = A3;
static const uint16_t WEIGHT_LIMIT = 600;
static const uint8_t MotorENA = 3;
static const uint8_t BASE_SPEED = 150;      // base PWM speed
static const uint8_t MAX_SPEED = 255;       // maximum PWM speed
static const uint8_t PhotoSensor = A2;
static const int LIGHT_THRESHOLD = 500;
void setup() {
  for (int i = 0; i < 4; i++) {
    pinMode(PushButton[i], INPUT_PULLUP);
    pinMode(IndicatorLED[i], OUTPUT);
  }

  pinMode(MotorIn1, OUTPUT);
  pinMode(MotorIn2, OUTPUT);
  lcd.begin(16, 2);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Elevator Ready");
  lcd.setCursor(0, 1);
  lcd.print("Floor: 1");

  updateLEDs();
  
  pinMode(DoorIn1, OUTPUT);
  pinMode(DoorIn2, OUTPUT);
  pinMode(LimitOpen, INPUT_PULLUP);
  pinMode(LimitClose, INPUT_PULLUP);
  pinMode(ForceSensor, INPUT);
  pinMode(MotorENA, OUTPUT);
  pinMode(PhotoSensor, INPUT);
  closeDoor();
}



void loop() {
  // Always read buttons — even mid-travel (travel is inside moveUp/moveDown delay)
  for (int i = 0; i < 4; i++) {
    if (digitalRead(PushButton[i]) == LOW && !(FloorQueue & (1 << i)) && i != PresentFloor) {
      FloorQueue |= (1 << i);
      lcd.setCursor(0, 1);
      lcd.print("Queued: Flr ");
      lcd.print(i + 1);
      lcd.print("  ");
      updateLEDs();
    }
  }

 
  if (FloorQueue != 0) {
  // Read buttons for 3 seconds with debouncing
  for (int w = 0; w < 30; w++) {       // 30 × 100ms = 3 seconds exactly
    for (int i = 0; i < 4; i++) {
      if (digitalRead(PushButton[i]) == LOW && !(FloorQueue & (1 << i)) && i != PresentFloor) {
        delay(50);                       // debounce — wait for signal to settle
        if (digitalRead(PushButton[i]) == LOW) {  // confirm it's still held
          FloorQueue |= (1 << i);
          lcd.setCursor(0, 1);
          lcd.print("Queued: Flr ");
          lcd.print(i + 1);
          lcd.print("  ");
          updateLEDs();
        }
      }
    }
    delay(100);                          // each iteration = 100ms
  }

    // Now pick nearest and move
    int8_t target = -1;
    uint8_t bestDist = 255;
    for (int i = 0; i < 4; i++) {
      if (FloorQueue & (1 << i)) {
        uint8_t dist = abs(i - PresentFloor);
        if (dist < bestDist) { bestDist = dist; target = i; }
      }
    }
    
    if (isOverloaded()) {
    displayMessage("OVERLOAD!", "Remove weight");
    delay(2000);
    return;
}
    
    if (target > PresentFloor) moveUp();
    else moveDown();
    stopMotor();
    PresentFloor = target;
    FloorQueue &= ~(1 << target);
    updateLEDs();
    openDoor();
    delay(2000);         // door stays open 2 seconds
    closeDoor();
  }
}

// Move Up
void moveUp() {
  uint8_t speed = calculateSpeed();
  analogWrite(MotorENA, speed);
  digitalWrite(MotorIn1, HIGH);
  digitalWrite(MotorIn2, LOW);
  displayMessage("Moving UP", "");
  delay(2000);
}

// Move Down
void moveDown() {
  uint8_t speed = calculateSpeed();
  analogWrite(MotorENA, speed);
  digitalWrite(MotorIn1, LOW);
  digitalWrite(MotorIn2, HIGH);
  displayMessage("Moving DOWN", "");
  delay(2000);
}

// Stop Motor
void stopMotor() {
  analogWrite(MotorENA, 0);
  digitalWrite(MotorIn1, LOW);
  digitalWrite(MotorIn2, LOW);
  displayMessage("Stopped", "");
}

void updateLEDs() {
  for (int i = 0; i < 4; i++) {
    bool active = (i == PresentFloor) || (FloorQueue & (1 << i));
    digitalWrite(IndicatorLED[i], active ? HIGH : LOW);
  }
  // Update floor on display
  lcd.setCursor(0, 0);
  lcd.print("Floor: ");
  lcd.print(PresentFloor + 1);
  lcd.print("        ");    // clear leftover characters
}

void openDoor() {
  displayMessage("Door Opening", "Please wait...");
  // Spin motor in open direction until open limit switch triggers
  digitalWrite(DoorIn1, HIGH);
  digitalWrite(DoorIn2, LOW);
  while (digitalRead(LimitOpen) == HIGH) {
    // just wait — limit switch will stop us
  }
  // Limit hit — stop door motor
  digitalWrite(DoorIn1, LOW);
  digitalWrite(DoorIn2, LOW);
  //Serial.println("Door fully open");
}

void closeDoor() {
  displayMessage("Door Closing", "");
  digitalWrite(DoorIn1, LOW);
  digitalWrite(DoorIn2, HIGH);
  while (digitalRead(LimitClose) == HIGH) {
    // Check for obstruction while closing
    if (analogRead(PhotoSensor) < LIGHT_THRESHOLD) {
      // Something blocking — reverse back open
      displayMessage("Obstruction!", "Reopening...");
      digitalWrite(DoorIn1, LOW);
      digitalWrite(DoorIn2, LOW);
      delay(300);
      openDoor();
      delay(2000);        // wait 2 seconds then try closing again
      closeDoor();        // recursive retry
      return;
    }
  }
  digitalWrite(DoorIn1, LOW);
  digitalWrite(DoorIn2, LOW);
  displayMessage("Door Closed", "");
}

void displayMessage(const char* line1, const char* line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

bool isOverloaded() {
  int weight = analogRead(ForceSensor);
  lcd.setCursor(0, 1);
  lcd.print("Weight: ");
  lcd.print(weight);
  lcd.print("   ");
  return weight > WEIGHT_LIMIT;
}

uint8_t calculateSpeed() {
  int weight = analogRead(ForceSensor);
  // Higher weight → higher PWM to maintain constant speed
  uint8_t speed = map(weight, 0, 1023, BASE_SPEED, MAX_SPEED);
  lcd.setCursor(0, 1);
  lcd.print("Speed: ");
  lcd.print(speed);
  lcd.print("   ");
  return speed;
}
