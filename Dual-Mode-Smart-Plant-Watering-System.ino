const int soilPin   = A0;
const int relayPin  = 2;
const int modeBtn   = 3;
const int pumpBtn   = 4;

int soilValue;
int dryThreshold = 700;
int wetThreshold = 300;

bool autoMode = true;

/* ===== MODE BUTTON ===== */
bool lastModeBtn = HIGH;
unsigned long lastModeTime = 0;
const unsigned long debounceDelay = 200;

/* ===== MANUAL PUMP BUTTON ===== */
bool pumpState = false;
bool lastPumpBtn = HIGH;
unsigned long lastPumpTime = 0;

/* ===== SERIAL PRINT ===== */
unsigned long lastPrintTime = 0;
const unsigned long printInterval = 500;

/* ===== FUNCTION ===== */
void pumpOff() {
  pumpState = false;
  digitalWrite(relayPin, HIGH);   // ปิดปั๊ม (Active LOW)
}

void setup() {
  Serial.begin(9600);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);   // ปิดปั๊มตั้งแต่เริ่ม

  pinMode(modeBtn, INPUT_PULLUP);
  pinMode(pumpBtn, INPUT_PULLUP);

  Serial.println("AUTO MODE");
}

void loop() {

  /* ===== MODE BUTTON ===== */
  bool modeState = digitalRead(modeBtn);

  if (modeState == LOW && lastModeBtn == HIGH &&
      millis() - lastModeTime > debounceDelay) {

    lastModeTime = millis();
    autoMode = !autoMode;

    pumpOff();        //  บังคับปิดปั๊มทุกครั้งที่เปลี่ยนโหมด
    delay(50);        // กันรีเลย์เด้ง
    lastPumpBtn = digitalRead(pumpBtn);

    Serial.println(autoMode ? "AUTO MODE" : "MANUAL MODE");
  }
  lastModeBtn = modeState;

  /* ===== MANUAL MODE ===== */
  if (!autoMode) {

    bool pumpBtnState = digitalRead(pumpBtn);

    if (pumpBtnState == LOW && lastPumpBtn == HIGH &&
        millis() - lastPumpTime > debounceDelay) {

      lastPumpTime = millis();
      pumpState = !pumpState;

      digitalWrite(relayPin, pumpState ? LOW : HIGH);

      Serial.print("MANUAL | Pump: ");
      Serial.println(pumpState ? "ON" : "OFF");
    }

    lastPumpBtn = pumpBtnState;
    return;
  }

  /* ===== AUTO MODE ===== */
  soilValue = analogRead(soilPin);

  if (millis() - lastPrintTime >= printInterval) {
    Serial.print("AUTO | Soil: ");
    Serial.println(soilValue);
    lastPrintTime = millis();
  }

  if (soilValue > dryThreshold) {
    digitalWrite(relayPin, LOW);   // ดินแห้ง → เปิดปั๊ม
  }
  else if (soilValue < wetThreshold) {
    digitalWrite(relayPin, HIGH);  // ดินชื้น → ปิดปั๊ม
  }
}