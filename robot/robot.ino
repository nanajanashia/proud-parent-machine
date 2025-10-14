#include <PCM.h>
#include <AccelStepper.h>

const signed char sample[] PROGMEM ={};


#define STEP_PIN 3
#define DIR_PIN 4
#define ENABLE_PIN 5
#define COIN_PIN 2  
#define PCM_PIN 11  
#define BUZZER_PIN 11  

AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

volatile unsigned long lastInterruptTime = 0;
const unsigned long debounceTime = 250;

unsigned long startTime = 0;
const unsigned long timeoutDuration = 10000; 
bool timeoutTriggered = false;
bool isIdle = true; 
bool timeoutBeepPlaying = false;
unsigned long beepStartTime = 0;

int movementPhase = 0;
bool isMoving = false;
bool shouldPlaySound = false;

void setup() {
  Serial.begin(9600);

  pinMode(COIN_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT); 
  attachInterrupt(digitalPinToInterrupt(COIN_PIN), coinDetected, FALLING);

  stepper.setEnablePin(ENABLE_PIN);
  stepper.setPinsInverted(false, false, true);
  stepper.enableOutputs();

  stepper.setMaxSpeed(200);
  stepper.setAcceleration(200);

  stepper.setCurrentPosition(0);
  stepper.moveTo(240);
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }

  stepper.setCurrentPosition(0);
  
  startTime = millis();
  timeoutTriggered = false;
  isIdle = true;
  
  Serial.println("Robot initialized and ready");
}

unsigned long lastPrint = 0;

void loop() {
  if (millis() - lastPrint > 500) {
    Serial.print("Coin pin state: ");
    Serial.print(digitalRead(COIN_PIN));
    Serial.print(" | Time until timeout: ");
    if (!timeoutTriggered) {
      unsigned long elapsedTime = millis() - startTime;
      if (elapsedTime < timeoutDuration) {
        Serial.print((timeoutDuration - elapsedTime) / 1000);
        Serial.println("s");
      } else {
        Serial.println("0s");
      }
    } else {
      Serial.println("TIMED OUT");
    }
    lastPrint = millis();
  }

  if (!timeoutTriggered && !isMoving && (millis() - startTime > timeoutDuration)) {
    triggerTimeout();
  }
  
  if (timeoutBeepPlaying && (millis() - beepStartTime > 1200)) {
    timeoutBeepPlaying = false;
    startTimeoutMovement();
  }

  if (isMoving) {
    stepper.run();

    switch (movementPhase) {
      case 1: 
        if (stepper.distanceToGo() == 0) {
          delay(100);
          stepper.moveTo(-130);
          movementPhase = 2;
        }
        break;
      case 2: 
        if (stepper.distanceToGo() == 0) {
          delay(100);
          stepper.moveTo(-180);
          movementPhase = 3;
        }
        break;
      case 3: 
        if (stepper.distanceToGo() == 0) {
          delay(100);
          stepper.moveTo(5);
          movementPhase = 4;
        }
        break;
      case 4: 
        if (stepper.distanceToGo() == 0) {
          isMoving = false;
          if (shouldPlaySound) {
            startPlayback(sample, sizeof(sample));
            Serial.println("Sound playback started");
            shouldPlaySound = false;
          }
          isIdle = true;
        }
        break;
      case 5: 
        if (stepper.distanceToGo() == 0) {
          Serial.println("Reached lower position, disabling stepper");
          isMoving = false;
          
          stepper.setMaxSpeed(200);
          stepper.setAcceleration(200);
          
          stepper.disableOutputs();
          Serial.println("Stepper disabled due to timeout - robot is now in sleep mode");
        } else {
          static unsigned long lastDebug = 0;
          if (millis() - lastDebug > 1000) {
            Serial.print("Moving to lower position, distance remaining: ");
            Serial.println(stepper.distanceToGo());
            lastDebug = millis();
          }
        }
        break;
    }
  }
}

void coinDetected() {
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > debounceTime) {
    if (!timeoutTriggered) {
      Serial.println("Coin Detected!");

      isIdle = false;

      shouldPlaySound = true;

      stepper.enableOutputs(); 
      stepper.moveTo(-180); 
      movementPhase = 1;
      isMoving = true;

      Serial.println("Starting movement sequence");
    } else {
      Serial.println("Coin detected but robot is in timeout state");
    }

    lastInterruptTime = interruptTime;
  }
}

void triggerTimeout() {
  Serial.println("Timeout triggered! Playing warning beep...");
  
  timeoutTriggered = true;
  isIdle = false;
  
  tone(BUZZER_PIN, 1000, 1000); 
  timeoutBeepPlaying = true;
  beepStartTime = millis();
  
  Serial.println("Beep started, will move to lower position after beep finishes");
}

void startTimeoutMovement() {
  Serial.println("Beep finished, starting movement to lower position");
  
  Serial.print("Current stepper position: ");
  Serial.println(stepper.currentPosition());
  
  stepper.enableOutputs();
  stepper.setMaxSpeed(100); 
  stepper.setAcceleration(50); 

  stepper.moveTo(-240); 
  movementPhase = 5; 
  isMoving = true;
  
  Serial.print("Movement started: ");
  Serial.print(stepper.currentPosition());
  Serial.print(" -> -240, distance to go: ");
  Serial.println(stepper.distanceToGo());
}

void resetRobot() {
  if (timeoutTriggered) {
    Serial.println("Resetting robot from timeout state");
    
    timeoutTriggered = false;
    isIdle = false;
    
    stepper.enableOutputs();
    stepper.moveTo(240);
    while (stepper.distanceToGo() != 0) {
      stepper.run();
    }
    stepper.setCurrentPosition(0);
    
    startTime = millis();
    isIdle = true;
    
    Serial.println("Robot reset complete and ready");
  }
}