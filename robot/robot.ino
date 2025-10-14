#include <PCM.h>
#include <AccelStepper.h>

const signed char sample[] PROGMEM ={};


// Pin assignments
#define STEP_PIN 3
#define DIR_PIN 4
#define ENABLE_PIN 5
#define COIN_PIN 2  // Coin verifier signal
#define PCM_PIN 11  // PCM audio output (default)
#define BUZZER_PIN 11  // Use same pin as PCM for warning beep

AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

volatile unsigned long lastInterruptTime = 0;
const unsigned long debounceTime = 250;

// Timeout variables
unsigned long startTime = 0;
const unsigned long timeoutDuration = 10000; // 10 seconds for testing (change to 600000 for 10 minutes)
bool timeoutTriggered = false;
bool isIdle = true; // Track if robot is in idle state
bool timeoutBeepPlaying = false;
unsigned long beepStartTime = 0;

int movementPhase = 0;
bool isMoving = false;
bool shouldPlaySound = false;

void setup() {
  Serial.begin(9600);

  pinMode(COIN_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT); // Setup buzzer pin if using separate pin
  attachInterrupt(digitalPinToInterrupt(COIN_PIN), coinDetected, FALLING);

  stepper.setEnablePin(ENABLE_PIN);
  stepper.setPinsInverted(false, false, true); // Invert enable if needed
  stepper.enableOutputs();

  stepper.setMaxSpeed(200);
  stepper.setAcceleration(200);

  // Initial positioning - raise to primed position
  stepper.setCurrentPosition(0);
  stepper.moveTo(240);
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }

  stepper.setCurrentPosition(0);
  
  // Initialize timeout timer from startup
  startTime = millis();
  timeoutTriggered = false;
  isIdle = true;
  
  Serial.println("Robot initialized and ready");
}

unsigned long lastPrint = 0;

void loop() {
  // Debug output every 500ms
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

  // Check for timeout - runs from startup time
  // Only trigger timeout if not currently moving to avoid abrupt stops
  if (!timeoutTriggered && !isMoving && (millis() - startTime > timeoutDuration)) {
    triggerTimeout();
  }
  
  // Handle timeout beep timing (non-blocking)
  if (timeoutBeepPlaying && (millis() - beepStartTime > 1200)) {
    // Beep finished, now start the movement
    timeoutBeepPlaying = false;
    startTimeoutMovement();
  }

  // Handle movement sequence
  if (isMoving) {
    stepper.run();

    switch (movementPhase) {
      case 1: // First tap down
        if (stepper.distanceToGo() == 0) {
          delay(100);
          stepper.moveTo(-130);
          movementPhase = 2;
        }
        break;
      case 2: // Second position
        if (stepper.distanceToGo() == 0) {
          delay(100);
          stepper.moveTo(-180);
          movementPhase = 3;
        }
        break;
      case 3: // Final tap position
        if (stepper.distanceToGo() == 0) {
          delay(100);
          stepper.moveTo(5);
          movementPhase = 4;
        }
        break;
      case 4: // Return to primed position and play sound
        if (stepper.distanceToGo() == 0) {
          isMoving = false;
          if (shouldPlaySound) {
            startPlayback(sample, sizeof(sample));
            Serial.println("Sound playback started");
            shouldPlaySound = false;
          }
          // Movement sequence complete - set to idle
          isIdle = true;
        }
        break;
      case 5: // Timeout movement - move to lower position
        if (stepper.distanceToGo() == 0) {
          Serial.println("Reached lower position, disabling stepper");
          isMoving = false;
          
          // Restore original speed settings before disabling
          stepper.setMaxSpeed(200);
          stepper.setAcceleration(200);
          
          // Disable stepper to prevent overheating
          stepper.disableOutputs();
          Serial.println("Stepper disabled due to timeout - robot is now in sleep mode");
        } else {
          // Debug: show movement progress
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
    // Only respond if not in timeout state
    if (!timeoutTriggered) {
      Serial.println("Coin Detected!");

      // Mark as active (but don't reset the startup timer)
      isIdle = false;

      // Start sound playback
      shouldPlaySound = true;

      // Start movement sequence
      stepper.enableOutputs(); // Ensure stepper is enabled
      stepper.moveTo(-180); // First tap down
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
  
  // Start beep (non-blocking)
  tone(BUZZER_PIN, 1000, 1000); // 1kHz beep for 1 second
  timeoutBeepPlaying = true;
  beepStartTime = millis();
  
  Serial.println("Beep started, will move to lower position after beep finishes");
}

void startTimeoutMovement() {
  Serial.println("Beep finished, starting movement to lower position");
  
  // Debug: show current position
  Serial.print("Current stepper position: ");
  Serial.println(stepper.currentPosition());
  
  // Ensure stepper is enabled and configure for smooth movement
  stepper.enableOutputs();
  stepper.setMaxSpeed(100); // Slower speed for timeout movement
  stepper.setAcceleration(50); // Slower acceleration for smooth movement
  
  // Move to lower position - go back to where we started before the 240 step raise
  stepper.moveTo(-240); // Go down 240 steps from current primed position
  movementPhase = 5; // Special timeout movement phase
  isMoving = true;
  
  Serial.print("Movement started: ");
  Serial.print(stepper.currentPosition());
  Serial.print(" -> -240, distance to go: ");
  Serial.println(stepper.distanceToGo());
}

// Function to reset the robot (call this if you want to reactivate after timeout)
void resetRobot() {
  if (timeoutTriggered) {
    Serial.println("Resetting robot from timeout state");
    
    timeoutTriggered = false;
    isIdle = false;
    
    // Re-enable stepper and move to primed position
    stepper.enableOutputs();
    stepper.moveTo(240);
    while (stepper.distanceToGo() != 0) {
      stepper.run();
    }
    stepper.setCurrentPosition(0);
    
    // Reset startup timer
    startTime = millis();
    isIdle = true;
    
    Serial.println("Robot reset complete and ready");
  }
}