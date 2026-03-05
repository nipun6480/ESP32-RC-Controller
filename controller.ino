#include <SPI.h>
#include <TFT_eSPI.h>       // Hardware-optimized library
#include <WiFi.h>
#include <WiFiUdp.h>
#include <BleGamepad.h>     
#include <TJpg_Decoder.h> 

// ================= COLORS =================
#define C_BLACK     TFT_BLACK
#define C_WHITE     TFT_WHITE
#define C_RED       TFT_RED
#define C_GREEN     TFT_GREEN
#define C_BLUE      TFT_BLUE
#define C_YELLOW    TFT_YELLOW
#define C_MAGENTA   TFT_MAGENTA
#define C_CYAN      TFT_CYAN
#define C_BG        0x10A2 
#define C_CARD      0x2124 
#define C_ACCENT    TFT_CYAN
#define C_TEXT      TFT_WHITE
#define C_ALERT     TFT_ORANGE
#define C_SUCCESS   TFT_GREEN

// ================= PINS =================
#define BTN1_PIN 17  
#define BTN2_PIN 16  
#define BTN3_PIN 22  
#define BTN4_PIN 21
#define BTN5_PIN 14
#define BTN6_PIN 27
#define JOY1_X  35
#define JOY1_Y  34
#define JOY1_SW 32
#define JOY2_X  25
#define JOY2_Y  33
#define JOY2_SW 26

// ================= GLOBALS =================
TFT_eSPI tft = TFT_eSPI(); 
WiFiClient client; 
WiFiUDP udp;
BleGamepad bleGamepad("Turbo Controller", "Nipun", 100);

const char* ssid = "RC_TURBO_NET"; 
const char* password = "12345678";
const char* cam_ip = "192.168.4.1"; 
const int cam_port = 80;
const int udp_port = 9999;

uint8_t* jpg_buffer = NULL;
const size_t buffer_size = 81920; // Expanded to 80KB for PSRAM
volatile bool rc_cam_running = false; 

enum SystemState { IDLE_MODE, MODE_SELECT, GAME_MENU, PLAY_SNAKE, PLAY_RACING, PLAY_DODGE, PLAY_PONG, PLAY_TETRIS, RC_CAM, BT_MODE, TEST_MODE };
SystemState currentState = IDLE_MODE;
int mainSelection = 0; int gameSelection = 0; unsigned long lastInputTime = 0; const int debounce = 150;

// Gamepad Icon
const unsigned char PROGMEM gamepad_icon[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 
  0x00, 0x00, 0x7F, 0xFF, 0xFF, 0xFE, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 
  0x00, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x00, 
  0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x00, 0x00, 0x7F, 0xC0, 0x00, 0x00, 0x03, 0xFE, 0x00, 
  0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x80, 
  0x01, 0xFC, 0x00, 0x18, 0x18, 0x00, 0x3F, 0x80, 0x03, 0xF8, 0x00, 0x18, 0x18, 0x00, 0x1F, 0xC0, 
  0x03, 0xF0, 0x01, 0x98, 0x19, 0x80, 0x0F, 0xC0, 0x03, 0xF0, 0x01, 0xF8, 0x1F, 0x80, 0x0F, 0xC0, 
  0x03, 0xE0, 0x00, 0xF8, 0x1F, 0x00, 0x07, 0xC0, 0x07, 0xE0, 0x00, 0x18, 0x18, 0x00, 0x07, 0xE0, 
  0x07, 0xC0, 0x00, 0x18, 0x18, 0x00, 0x03, 0xE0, 0x07, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x03, 0xE0, 
  0x07, 0xC0, 0x0F, 0x00, 0x00, 0xF0, 0x03, 0xE0, 0x07, 0xC0, 0x1F, 0x00, 0x00, 0xF8, 0x03, 0xE0, 
  0x07, 0xC0, 0x3F, 0x00, 0x00, 0xFC, 0x03, 0xE0, 0x07, 0xC0, 0x3F, 0x00, 0x00, 0xFC, 0x03, 0xE0, 
  0x07, 0xC0, 0x1F, 0x00, 0x00, 0xF8, 0x03, 0xE0, 0x07, 0xE0, 0x0F, 0x00, 0x00, 0xF0, 0x07, 0xE0, 
  0x03, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x07, 0xC0, 0x03, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xC0, 
  0x03, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xC0, 0x01, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x80, 
  0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x80, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 
  0x00, 0x7F, 0xC0, 0x00, 0x00, 0x03, 0xFE, 0x00, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x00, 
  0x00, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x00, 0x00, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0, 0x00, 
  0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x7F, 0xFF, 0xFF, 0xFE, 0x00, 0x00, 
  0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
}; 

// Forward Declarations
void drawMainMenu(); void drawGameMenu(); void loopMainMenu(); void loopGameMenu();
void runRCCamMode(); void runBluetoothMode(); void runTestMode();
void gameSnake(); void gameRacing(); void gameDodge(); void gamePong(); void gameTetris();
void returnToGameMenu(int score);

// ================= HELPERS (SMOOTHING & DEADZONE) =================
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap){
   if ( y >= tft.height() ) return 0;
   tft.pushImage(x, y, w, h, bitmap); 
   return 1;
}

int applyDeadzone(int value, int center, int deadzone) {
  if (abs(value - center) < deadzone) return center;
  return value;
}

// Low-Pass Filter for Buttery Smooth Joysticks
int smoothAnalog(int pin) {
  static float last1X = 2048, last1Y = 2048, last2X = 2048, last2Y = 2048;
  int raw = analogRead(pin);
  
  if (pin == JOY1_X) { last1X = (last1X * 0.7) + (raw * 0.3); return (int)last1X; }
  if (pin == JOY1_Y) { last1Y = (last1Y * 0.7) + (raw * 0.3); return (int)last1Y; }
  if (pin == JOY2_X) { last2X = (last2X * 0.7) + (raw * 0.3); return (int)last2X; }
  if (pin == JOY2_Y) { last2Y = (last2Y * 0.7) + (raw * 0.3); return (int)last2Y; }
  return raw;
}

int getGameDelay(int minDelay, int maxDelay) {
  int val = analogRead(JOY2_Y);
  int d = map(val, 0, 4095, minDelay, maxDelay);
  return constrain(d, minDelay, maxDelay);
}

void returnToGameMenu(int score) {
  tft.fillScreen(C_BLACK); 
  tft.setTextDatum(MC_DATUM); 
  tft.setTextColor(C_TEXT); tft.setTextSize(2);
  tft.drawString("GAME OVER", 160, 100);
  tft.drawString("Score: " + String(score), 160, 140);
  delay(2000); currentState = GAME_MENU; drawGameMenu();
}

void centerText(String text, int y) {
  tft.setTextDatum(TC_DATUM); 
  tft.drawString(text, 160, y);
}

void showBootLogo() {
  tft.fillScreen(C_BG);
  tft.drawBitmap(128, 80, gamepad_icon, 64, 38, C_ACCENT);
  tft.setTextColor(C_TEXT); tft.setTextSize(2);
  centerText("SMART CONTROLLER", 140);
  tft.drawRect(60, 170, 200, 10, C_WHITE);
  for(int i=0; i<196; i+=4) { tft.fillRect(62, 172, i, 6, C_SUCCESS); delay(5); }
}

void showReadyScreen() {
  tft.fillScreen(C_BG);
  tft.setTextColor(C_SUCCESS); tft.setTextSize(3);
  centerText("CONTROLLER", 80);
  centerText("READY", 120);
  tft.setTextColor(C_TEXT); tft.setTextSize(1);
  centerText("Press BTN1 for Menu", 220);
  currentState = IDLE_MODE;
}

void drawHeader(String title) {
  tft.fillScreen(C_BG);
  tft.fillRect(0, 0, 320, 30, C_CARD);
  tft.drawFastHLine(0, 30, 320, C_ACCENT);
  tft.setTextColor(C_ACCENT); tft.setTextSize(2);
  tft.setTextDatum(TL_DATUM); 
  tft.drawString(title, 10, 6);
  tft.drawRect(280, 8, 25, 12, C_TEXT);
  tft.fillRect(282, 10, 18, 8, C_SUCCESS);
  tft.fillRect(305, 10, 3, 8, C_TEXT);
}

void drawFooter() {
  tft.fillRect(0, 220, 320, 20, C_CARD);
  tft.setTextColor(C_TEXT); tft.setTextSize(1);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("BTN2: SELECT   BTN3: BACK", 10, 225);
}

void updateMenuList(const char* items[], int count, int selected) {
  tft.setTextDatum(TL_DATUM);
  for (int i = 0; i < count; i++) {
    int y = 50 + (i * 35);
    tft.fillRoundRect(40, y, 240, 30, 5, (i == selected) ? C_ACCENT : C_CARD);
    tft.setTextColor((i == selected) ? C_BLACK : C_TEXT); tft.setTextSize(2);
    int strWidth = tft.textWidth(items[i]);
    tft.drawString(items[i], 40 + (240 - strWidth)/2, y + 8);
  }
}

void handleNavigation(int &selection, int max) {
  static const char* mainItems[] = {"RC CAMERA", "GAMES", "BLUETOOTH", "TESTING"};
  static const char* gameItems[] = {"SNAKE", "RACING", "DODGE", "PONG", "BLOCKS"};

  if (millis() - lastInputTime > debounce) {
    int joyY = analogRead(JOY1_Y);
    bool changed = false;
    
    if (joyY < 1000) { selection++; changed = true; }
    if (joyY > 3000) { selection--; changed = true; }
    
    if (changed) {
      if (selection < 0) selection = max - 1;
      if (selection >= max) selection = 0;
      
      if (currentState == MODE_SELECT) updateMenuList(mainItems, 4, selection);
      else updateMenuList(gameItems, 5, selection);
      
      lastInputTime = millis();
    }
  }
}

void drawMainMenu() {
  static const char* mainItems[] = {"RC CAMERA", "GAMES", "BLUETOOTH", "TESTING"};
  drawHeader("MAIN MENU");
  drawFooter();
  updateMenuList(mainItems, 4, mainSelection);
}

void loopMainMenu() {
  handleNavigation(mainSelection, 4);
  if (digitalRead(BTN2_PIN) == LOW || digitalRead(JOY1_SW) == LOW) {
    delay(200);
    if (mainSelection == 0) currentState = RC_CAM;
    else if (mainSelection == 1) { currentState = GAME_MENU; drawGameMenu(); }
    else if (mainSelection == 2) currentState = BT_MODE;
    else if (mainSelection == 3) currentState = TEST_MODE;
  }
  if (digitalRead(BTN3_PIN) == LOW) { delay(200); showReadyScreen(); }
}

void drawGameMenu() {
  static const char* gameItems[] = {"SNAKE", "RACING", "DODGE", "PONG", "BLOCKS"};
  drawHeader("GAME SELECT");
  drawFooter();
  updateMenuList(gameItems, 5, gameSelection);
}

void loopGameMenu() {
  handleNavigation(gameSelection, 5);
  if (digitalRead(BTN3_PIN) == LOW) { delay(200); currentState = MODE_SELECT; drawMainMenu(); return; }
  if (digitalRead(BTN2_PIN) == LOW) {
    delay(200);
    if (gameSelection == 0) currentState = PLAY_SNAKE;
    else if (gameSelection == 1) currentState = PLAY_RACING;
    else if (gameSelection == 2) currentState = PLAY_DODGE;
    else if (gameSelection == 3) currentState = PLAY_PONG;
    else if (gameSelection == 4) currentState = PLAY_TETRIS;
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  pinMode(BTN1_PIN, INPUT_PULLUP); pinMode(BTN2_PIN, INPUT_PULLUP);
  pinMode(BTN3_PIN, INPUT_PULLUP); pinMode(BTN4_PIN, INPUT_PULLUP);
  pinMode(BTN5_PIN, INPUT_PULLUP); pinMode(BTN6_PIN, INPUT_PULLUP);
  pinMode(JOY1_SW, INPUT_PULLUP);  pinMode(JOY2_SW, INPUT_PULLUP);

  tft.init(); 
  tft.setRotation(3);
  tft.fillScreen(C_BLACK); 
  
  TJpgDec.setJpgScale(0);     
  TJpgDec.setSwapBytes(true); 
  TJpgDec.setCallback(tft_output);

  bleGamepad.begin(); 
  
  showBootLogo(); 
  showReadyScreen(); 
}

// ================= LOOP =================
void loop() {
  switch(currentState) {
    case IDLE_MODE: if (digitalRead(BTN1_PIN) == LOW) { delay(200); currentState = MODE_SELECT; drawMainMenu(); } break;
    case MODE_SELECT: loopMainMenu(); break;
    case GAME_MENU:   loopGameMenu(); break;
    case PLAY_SNAKE:  gameSnake();    break;
    case PLAY_RACING: gameRacing();   break;
    case PLAY_DODGE:  gameDodge();    break;
    case PLAY_PONG:   gamePong();     break;
    case PLAY_TETRIS: gameTetris();   break; 
    case RC_CAM:      runRCCamMode(); break;
    case BT_MODE:     runBluetoothMode(); break; 
    case TEST_MODE:   runTestMode();  break;
  }
}

// ================= DUAL-CORE RC TASK (DRIFT FIX APPLIED) =================
void rcControlTask(void * parameter) {
  
  // --- STEERING TRIM (DRIFT FIX) ---
  // If car drifts left, increase this (e.g., 100, 150, 200).
  // If car drifts right, make it negative (e.g., -100).
  const int STEERING_TRIM = 150; 

  while(rc_cam_running) {
    
    // Apply low-pass filter smoothing + Trim
    int rawSteer = smoothAnalog(JOY1_X) + STEERING_TRIM;
    rawSteer = constrain(rawSteer, 0, 4095); // Prevent math overflow
    
    // Apply deadzones to prevent drift when thumbs are off the sticks
    rawSteer = applyDeadzone(rawSteer, 2048 + STEERING_TRIM, 200);
    int rawSpeed = applyDeadzone(smoothAnalog(JOY2_Y), 2048, 200);
    
    uint8_t steering = map(rawSteer, 0, 4095, 0, 255);
    uint8_t throttle = map(rawSpeed, 0, 4095, 0, 255);
    uint8_t btnState = (digitalRead(BTN4_PIN) == LOW) ? 1 : 0; 

    udp.beginPacket(cam_ip, udp_port);
    udp.write(throttle);
    udp.write(steering);
    udp.write(btnState);
    udp.endPacket();

    vTaskDelay(20 / portTICK_PERIOD_MS); // 50Hz Update Rate
  }
  vTaskDelete(NULL);
}

// ================= RC CAM MODE (PSRAM + HUD) =================
void runRCCamMode() {
  
  // 1. ALLOCATE MEMORY (Use PSRAM if available for better FPS)
  if (jpg_buffer == NULL) {
    if (psramFound()) {
      jpg_buffer = (uint8_t*) ps_malloc(buffer_size);
    } else {
      jpg_buffer = (uint8_t*) malloc(buffer_size); 
    }
    if (jpg_buffer == NULL) { tft.fillScreen(C_RED); delay(2000); return; }
  }

  tft.fillScreen(C_BLACK);
  tft.setTextColor(C_TEXT); tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM); 
  tft.drawString("Connecting to Car...", 160, 120);
  
  WiFi.begin(ssid, password);
  int timeout = 0;
  while(WiFi.status() != WL_CONNECTED && timeout < 20) { delay(500); timeout++; }
  if (WiFi.status() != WL_CONNECTED) {
      tft.fillScreen(C_RED); tft.drawString("No Car Found!", 160, 120);
      delay(2000); free(jpg_buffer); jpg_buffer = NULL; currentState = MODE_SELECT; drawMainMenu(); return;
  }

  tft.fillScreen(C_BLACK);
  if (!client.connect(cam_ip, cam_port)) {
      tft.fillScreen(C_RED); tft.drawString("Connect Fail", 160, 120); delay(2000); 
      WiFi.disconnect(); free(jpg_buffer); jpg_buffer = NULL; currentState = MODE_SELECT; drawMainMenu(); return;
  }

  udp.begin(udp_port);

  // START BACKGROUND TELEMETRY TASK (Core 1)
  rc_cam_running = true;
  xTaskCreatePinnedToCore(rcControlTask, "RC_Ctrl", 4096, NULL, 1, NULL, 1);

  client.print("GET / HTTP/1.1\r\nHost: 192.168.4.1\r\nConnection: keep-alive\r\n\r\n");

  unsigned long lastFrameTime = millis();
  int fps = 0, frames = 0;

  while(true) {
    // 1. VIDEO: Read MJPEG Frame on Core 0
    if (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line.startsWith("Content-Length: ")) {
        int len = line.substring(16).toInt();
        client.readStringUntil('\n'); 
        
        if (len > 0 && len < buffer_size) {
          int bytesRead = 0;
          while (bytesRead < len && client.connected()) {
            if(client.available()) {
              int r = client.read(jpg_buffer + bytesRead, len - bytesRead);
              if(r > 0) bytesRead += r;
            }
          }
          TJpgDec.drawJpg(0, 0, jpg_buffer, bytesRead);
        }
      }
    }

    // 2. CALCULATE FPS
    frames++;
    if (millis() - lastFrameTime >= 1000) {
      fps = frames;
      frames = 0;
      lastFrameTime = millis();
    }

    // 3. DRAW TELEMETRY HUD
    int rssi = WiFi.RSSI();
    int throttlePct = map(smoothAnalog(JOY2_Y), 0, 4095, -100, 100);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(C_RED, C_BLACK); tft.setTextSize(1);
    tft.drawString("REC", 10, 10); 

    tft.setTextColor(C_GREEN, C_BLACK);
    tft.drawString("FPS: " + String(fps) + "   ", 260, 10); // Spaces clear old numbers
    tft.drawString("WiFi: " + String(rssi) + "dBm  ", 10, 220);

    tft.setTextDatum(TR_DATUM);
    tft.drawString("THR: " + String(throttlePct) + "%  ", 310, 220);

    if(digitalRead(BTN3_PIN) == LOW) {
      rc_cam_running = false; 
      delay(50);
      break; 
    }
  }
  
  client.stop(); udp.stop(); WiFi.disconnect();
  free(jpg_buffer); jpg_buffer = NULL;
  currentState = MODE_SELECT; drawMainMenu();
}

// ================= BLE GAMEPAD MODE =================
void runBluetoothMode() {
  drawHeader("BLE GAMEPAD");
  tft.fillRect(0, 40, 320, 30, C_BG);
  tft.setTextColor(C_TEXT); tft.setTextSize(1);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("Name: Turbo Controller", 10, 50);
  
  tft.drawRect(20, 80, 130, 100, C_CARD); tft.drawRect(170, 80, 130, 100, C_CARD);
  tft.setTextColor(C_ACCENT); 
  tft.setTextDatum(MC_DATUM);
  tft.drawString("L-STICK", 85, 90); tft.drawString("R-STICK", 235, 90);

  while(true) {
    if (bleGamepad.isConnected()) {
      tft.setTextColor(C_SUCCESS, C_BG); 
      tft.drawString("ACTIVE  ", 270, 50);

      // Smooth & Deadzone
      int raw1x = applyDeadzone(smoothAnalog(JOY1_X), 2048, 200);
      int raw1y = applyDeadzone(smoothAnalog(JOY1_Y), 2048, 200);
      int raw2x = applyDeadzone(smoothAnalog(JOY2_X), 2048, 200);
      int raw2y = applyDeadzone(smoothAnalog(JOY2_Y), 2048, 200);

      int j1x = map(raw1x, 0, 4095, -32767, 32767); 
      int j1y = map(raw1y, 0, 4095, 32767, -32767);
      int j2x = map(raw2x, 0, 4095, -32767, 32767); 
      int j2y = map(raw2y, 0, 4095, 32767, -32767);
      
      bleGamepad.setAxes(j1x, j1y, j2x, j2y, 0, 0, 0, 0);

      // Buttons
      if(digitalRead(BTN1_PIN) == LOW) bleGamepad.press(BUTTON_1); else bleGamepad.release(BUTTON_1);
      if(digitalRead(BTN2_PIN) == LOW) bleGamepad.press(BUTTON_2); else bleGamepad.release(BUTTON_2);
      if(digitalRead(BTN4_PIN) == LOW) bleGamepad.press(BUTTON_3); else bleGamepad.release(BUTTON_3);
      if(digitalRead(BTN5_PIN) == LOW) bleGamepad.press(BUTTON_4); else bleGamepad.release(BUTTON_4);
      if(digitalRead(BTN6_PIN) == LOW) bleGamepad.press(BUTTON_5); else bleGamepad.release(BUTTON_5);
      if(digitalRead(JOY1_SW) == LOW)  bleGamepad.press(BUTTON_6); else bleGamepad.release(BUTTON_6);
      if(digitalRead(JOY2_SW) == LOW)  bleGamepad.press(BUTTON_7); else bleGamepad.release(BUTTON_7);
      
    } else {
      tft.setTextColor(C_ALERT, C_BG); 
      tft.drawString("PAIRING...", 270, 50);
    }

    int draw_lx = map(analogRead(JOY1_X), 0, 4095, 25, 145); 
    int draw_ly = map(analogRead(JOY1_Y), 0, 4095, 105, 175);
    int draw_rx = map(analogRead(JOY2_X), 0, 4095, 175, 295); 
    int draw_ry = map(analogRead(JOY2_Y), 0, 4095, 105, 175);
    
    tft.fillRect(21, 100, 128, 79, C_BG); tft.fillRect(171, 100, 128, 79, C_BG); 
    tft.fillCircle(draw_lx, draw_ly, 5, C_SUCCESS); tft.fillCircle(draw_rx, draw_ry, 5, C_ALERT);
    
    if(digitalRead(BTN3_PIN) == LOW) { delay(200); break; }
    delay(20);
  }
  
  currentState = MODE_SELECT; drawMainMenu();
}

// ================= HARDWARE TEST =================
void runTestMode() {
  tft.fillScreen(C_BG); drawHeader("HARDWARE TEST");
  tft.setTextColor(C_TEXT); tft.setTextSize(1); centerText("HOLD BTN3 TO EXIT", 225);
  tft.setTextColor(C_ACCENT); 
  tft.drawString("JOYSTICK 1", 20, 45); tft.drawString("JOYSTICK 2", 180, 45);
  tft.drawFastHLine(0, 130, 320, C_CARD); 
  tft.drawString("BUTTONS", 20, 140);
  
  unsigned long exitTimer = 0; bool exitActive = false;
  while(true) {
    int btnState[6] = { !digitalRead(BTN1_PIN), !digitalRead(BTN2_PIN), !digitalRead(BTN3_PIN), !digitalRead(BTN4_PIN), !digitalRead(BTN5_PIN), !digitalRead(BTN6_PIN) };
    
    tft.setTextColor(C_TEXT, C_BG);
    tft.setCursor(20, 65); tft.printf("X: %04d   ", analogRead(JOY1_X));
    tft.setCursor(20, 80); tft.printf("Y: %04d   ", analogRead(JOY1_Y));
    tft.setCursor(20, 95); tft.printf("SW: %s ", digitalRead(JOY1_SW) == 0 ? "ON " : "OFF");
    tft.setCursor(180, 65); tft.printf("X: %04d   ", analogRead(JOY2_X));
    tft.setCursor(180, 80); tft.printf("Y: %04d   ", analogRead(JOY2_Y));
    tft.setCursor(180, 95); tft.printf("SW: %s ", digitalRead(JOY2_SW) == 0 ? "ON " : "OFF");
    
    for(int i=0; i<6; i++) {
        int x = 40 + ((i%3) * 90); int y = 160 + ((i/3) * 35);
        tft.fillRoundRect(x, y, 60, 25, 4, (btnState[i] == 1) ? C_SUCCESS : C_CARD);
        tft.setTextColor((btnState[i] == 1) ? C_BLACK : C_TEXT); 
        tft.drawString("B" + String(i+1), x + 20, y + 5);
    }
    
    if(btnState[2] == 1) { 
        if(!exitActive) { exitActive = true; exitTimer = millis(); } 
        else if (millis() - exitTimer > 800) break; 
    } else { exitActive = false; }
    delay(50);
  }
  currentState = MODE_SELECT; drawMainMenu();
}

// --- GAME: SNAKE ---
void gameSnake() {
  tft.fillScreen(C_BLACK); 
  const int TILE = 10; 
  int w = 32; int h = 24;
  int snakeX[100], snakeY[100]; 
  int snakeLen = 3; 
  int dirX = 1; int dirY = 0;
  
  for(int i=0; i<snakeLen; i++) { snakeX[i] = 10-i; snakeY[i] = 10; }
  int appleX = random(1, w-1); int appleY = random(1, h-1); 
  int score = 0;
  
  tft.fillRect(appleX*TILE, appleY*TILE, TILE, TILE, C_RED);
  for(int i=0; i<snakeLen; i++) tft.fillRect(snakeX[i]*TILE, snakeY[i]*TILE, TILE, TILE, C_SUCCESS);
  
  while(true) {
    unsigned long start = millis();
    int jx = analogRead(JOY1_X); int jy = analogRead(JOY1_Y);
    
    if(jx > 3000 && dirX == 0) { dirX=1; dirY=0; }
    if(jx < 1000 && dirX == 0) { dirX=-1; dirY=0;}
    if(jy < 1000 && dirY == 0) { dirX=0; dirY=1; } 
    if(jy > 3000 && dirY == 0) { dirX=0; dirY=-1;} 
    if(digitalRead(BTN3_PIN) == LOW) break;
    
    tft.fillRect(snakeX[snakeLen-1]*TILE, snakeY[snakeLen-1]*TILE, TILE, TILE, C_BLACK);
    
    for(int i=snakeLen-1; i>0; i--) { snakeX[i] = snakeX[i-1]; snakeY[i] = snakeY[i-1]; }
    snakeX[0] += dirX; snakeY[0] += dirY;
    
    if(snakeX[0] < 0) snakeX[0] = w-1; if(snakeX[0] >= w) snakeX[0] = 0;
    if(snakeY[0] < 0) snakeY[0] = h-1; if(snakeY[0] >= h) snakeY[0] = 0;
    
    for(int i=1; i<snakeLen; i++) if(snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) { returnToGameMenu(score); return; }
    
    if(snakeX[0] == appleX && snakeY[0] == appleY) {
      score++; snakeLen++; appleX = random(1, w-1); appleY = random(1, h-1);
      tft.fillRect(appleX*TILE, appleY*TILE, TILE, TILE, C_RED);
    }
    
    tft.fillRect(snakeX[0]*TILE, snakeY[0]*TILE, TILE, TILE, C_SUCCESS);
    
    int stepDelay = getGameDelay(30, 180); 
    while(millis() - start < stepDelay) { }
  }
  returnToGameMenu(score);
}

// --- GAME: RACING ---
void gameRacing() {
  tft.fillScreen(C_BG); 
  int carX = 140; int carY = 200; 
  int enemyX = random(40, 240); int enemyY = -40; 
  int score = 0; int baseSpeed = 5;
  
  tft.fillRect(0,0, 20, 240, C_SUCCESS); tft.fillRect(300,0, 20, 240, C_SUCCESS);
  
  while(true) {
    tft.fillRect(carX, carY, 30, 40, C_BG); 
    tft.fillRect(enemyX, enemyY, 30, 40, C_BG);
    
    int jx = analogRead(JOY1_X); 
    if(jx < 1000) carX -= 6; if(jx > 3000) carX += 6;
    if(carX < 25) carX = 25; if(carX > 265) carX = 265;
    
    if(digitalRead(BTN3_PIN) == LOW) break;
    
    enemyY += baseSpeed; 
    if(enemyY > 240) { enemyY = -40; enemyX = random(25, 265); score++; if(score%5==0) baseSpeed++; }
    
    if(abs(carX - enemyX) < 28 && abs(carY - enemyY) < 38) break;
    
    tft.fillRect(carX, carY, 30, 40, C_BLUE); 
    tft.fillRect(enemyX, enemyY, 30, 40, C_RED);
    
    tft.setTextColor(C_TEXT, C_BG); tft.setCursor(140, 10); tft.print(score);
    delay(getGameDelay(10, 60));
  }
  returnToGameMenu(score);
}

// --- GAME: DODGE ---
void gameDodge() {
  tft.fillScreen(C_BLACK); 
  int playerX = 160; 
  int wallY = 0; int gapX = random(20, 240); 
  int score = 0;
  
  while(true) {
    tft.fillRect(playerX, 210, 15, 15, C_BLACK); 
    tft.fillRect(0, wallY-4, 320, 8, C_BLACK); 
    
    int jx = analogRead(JOY1_X); 
    if(jx < 1000) playerX -= 5; if(jx > 3000) playerX += 5;
    if(playerX < 0) playerX = 0; if(playerX > 305) playerX = 305;
    
    if(digitalRead(BTN3_PIN) == LOW) break;
    
    wallY += 3 + (score/10); 
    if(wallY > 240) { wallY = 0; gapX = random(10, 250); score++; }
    
    if(wallY > 210 && wallY < 225) if(playerX < gapX || playerX > gapX + 60) break;
    
    tft.fillRect(0, wallY, gapX, 5, C_MAGENTA); 
    tft.fillRect(gapX + 60, wallY, 320 - (gapX+60), 5, C_MAGENTA);
    
    tft.fillRect(playerX, 210, 15, 15, C_ACCENT); 
    tft.setTextColor(C_TEXT, C_BLACK); tft.setCursor(5, 5); tft.print(score);
    delay(getGameDelay(10, 60));
  }
  returnToGameMenu(score);
}

// --- GAME: PONG ---
void gamePong() {
  tft.fillScreen(C_BLACK); 
  float ballX=160, ballY=120, vx=4, vy=3; 
  int padY=100, cpuY=100;
  int scoreP=0, scoreC=0;
  
  tft.drawFastVLine(160, 0, 240, C_CARD);
  
  while(scoreP < 5 && scoreC < 5) {
    tft.fillCircle((int)ballX, (int)ballY, 4, C_BLACK); 
    tft.fillRect(10, padY, 8, 50, C_BLACK); 
    tft.fillRect(302, cpuY, 8, 50, C_BLACK);
    
    int jy = analogRead(JOY1_Y); 
    if(jy < 1000) padY += 5; if(jy > 3000) padY -= 5;
    if(padY < 0) padY=0; if(padY > 190) padY=190;
    
    if(digitalRead(BTN3_PIN) == LOW) break;
    
    if(ballY > cpuY + 25) cpuY += 3; else cpuY -= 3;
    
    ballX += vx; ballY += vy; 
    if(ballY <= 0 || ballY >= 240) vy *= -1;
    
    if(ballX < 18 && ballY > padY && ballY < padY+50) { vx = abs(vx)+0.3; ballX=18; }
    if(ballX > 294 && ballY > cpuY && ballY < cpuY+50) { vx = -abs(vx)-0.3; ballX=294; }
    
    if(ballX < 0) { scoreC++; ballX=160; ballY=120; vx=4; } 
    if(ballX > 320) { scoreP++; ballX=160; ballY=120; vx=-4; }
    
    tft.drawFastVLine(160, 0, 240, C_CARD); 
    tft.fillRect(10, padY, 8, 50, C_ACCENT); 
    tft.fillRect(302, cpuY, 8, 50, C_RED);
    tft.fillCircle((int)ballX, (int)ballY, 4, C_TEXT); 
    
    tft.setCursor(120, 10); tft.print(scoreP); 
    tft.setCursor(190, 10); tft.print(scoreC);
    
    delay(getGameDelay(10, 40));
  }
  returnToGameMenu(scoreP);
}

// --- GAME: TETRIS ---
const int T_W = 10; const int T_H = 18; const int T_SIZE = 12; 
const int T_OFFSET_X = 100; const int T_OFFSET_Y = 20;
int grid[T_H][T_W];
uint16_t t_colors[] = {C_BLACK, C_ACCENT, C_BLUE, C_ALERT, C_YELLOW, C_SUCCESS, C_MAGENTA, C_RED};

void drawBlock(int x, int y, int c) { 
    tft.fillRect(T_OFFSET_X + x*T_SIZE, T_OFFSET_Y + y*T_SIZE, T_SIZE-1, T_SIZE-1, t_colors[c]); 
}

void gameTetris() {
    tft.fillScreen(C_BLACK);
    tft.drawRect(T_OFFSET_X-2, T_OFFSET_Y-2, (T_W*T_SIZE)+4, (T_H*T_SIZE)+4, C_WHITE);
    for(int y=0; y<T_H; y++) for(int x=0; x<T_W; x++) grid[y][x] = 0;
    
    int score = 0; bool gameOver = false;
    int px = 4, py = 0; 
    int currentShape[4][2]; 
    int type = random(1, 8);
    
    auto spawnPiece = [&](int t) {
        px = 4; py = 0;
        if(t==1) { currentShape[0][0]=0; currentShape[0][1]=1; currentShape[1][0]=1; currentShape[1][1]=1; currentShape[2][0]=2; currentShape[2][1]=1; currentShape[3][0]=3; currentShape[3][1]=1; } 
        else if(t==2) { currentShape[0][0]=0; currentShape[0][1]=0; currentShape[1][0]=0; currentShape[1][1]=1; currentShape[2][0]=1; currentShape[2][1]=1; currentShape[3][0]=2; currentShape[3][1]=1; } 
        else if(t==3) { currentShape[0][0]=2; currentShape[0][1]=0; currentShape[1][0]=0; currentShape[1][1]=1; currentShape[2][0]=1; currentShape[2][1]=1; currentShape[3][0]=2; currentShape[3][1]=1; } 
        else if(t==4) { currentShape[0][0]=1; currentShape[0][1]=0; currentShape[1][0]=2; currentShape[1][1]=0; currentShape[2][0]=1; currentShape[2][1]=1; currentShape[3][0]=2; currentShape[3][1]=1; } 
        else if(t==5) { currentShape[0][0]=1; currentShape[0][1]=0; currentShape[1][0]=2; currentShape[1][1]=0; currentShape[2][0]=0; currentShape[2][1]=1; currentShape[3][0]=1; currentShape[3][1]=1; } 
        else if(t==6) { currentShape[0][0]=1; currentShape[0][1]=0; currentShape[1][0]=0; currentShape[1][1]=1; currentShape[2][0]=1; currentShape[2][1]=1; currentShape[3][0]=2; currentShape[3][1]=1; } 
        else if(t==7) { currentShape[0][0]=0; currentShape[0][1]=0; currentShape[1][0]=1; currentShape[1][1]=0; currentShape[2][0]=1; currentShape[2][1]=1; currentShape[3][0]=2; currentShape[3][1]=1; } 
    };
    
    spawnPiece(type);
    unsigned long lastFall = 0; 
    int fallSpeed = 500;
    
    while(!gameOver) {
        if(digitalRead(BTN3_PIN) == LOW) break; 
        
        int jx = analogRead(JOY1_X); int jy = analogRead(JOY1_Y);
        bool rotate = (digitalRead(BTN2_PIN) == LOW);
        
        if(rotate) {
            int temp[4][2]; bool possible = true; int cx = currentShape[2][0]; int cy = currentShape[2][1];
            for(int i=0; i<4; i++) {
                int nx = -(currentShape[i][1] - cy) + cx; int ny = (currentShape[i][0] - cx) + cy;
                temp[i][0] = nx; temp[i][1] = ny;
                if (px + nx < 0 || px + nx >= T_W || py + ny >= T_H) possible = false;
                else if (py + ny >= 0 && grid[py + ny][px + nx] != 0) possible = false;
            }
            if(possible && type != 4) { for(int i=0; i<4; i++) { currentShape[i][0] = temp[i][0]; currentShape[i][1] = temp[i][1]; } delay(200); }
        }
        
        int dx = 0; if (jx < 1000) dx = 1; if (jx > 3000) dx = -1; 
        
        if (dx != 0) {
            bool possible = true;
            for(int i=0; i<4; i++) {
                int nx = px + currentShape[i][0] + dx; int ny = py + currentShape[i][1];
                if (nx < 0 || nx >= T_W || (ny >=0 && grid[ny][nx] != 0)) possible = false;
            }
            if(possible) { px += dx; delay(100); }
        }
        
        int currentSpeed = (jy < 1000) ? 50 : fallSpeed;
        
        if (millis() - lastFall > currentSpeed) {
            bool canFall = true;
            for(int i=0; i<4; i++) {
                int nx = px + currentShape[i][0]; int ny = py + currentShape[i][1] + 1;
                if (ny >= T_H || (ny >= 0 && grid[ny][nx] != 0)) canFall = false;
            }
            
            if (canFall) { py++; } else {
                for(int i=0; i<4; i++) {
                    int nx = px + currentShape[i][0]; int ny = py + currentShape[i][1];
                    if(ny < 0) { gameOver = true; break; } 
                    grid[ny][nx] = type;
                }
                
                for(int y=0; y<T_H; y++) {
                    bool full = true; for(int x=0; x<T_W; x++) if(grid[y][x] == 0) full = false;
                    if(full) {
                        for(int k=y; k>0; k--) for(int x=0; x<T_W; x++) grid[k][x] = grid[k-1][x];
                        for(int x=0; x<T_W; x++) grid[0][x] = 0; score += 10;
                    }
                }
                type = random(1, 8); spawnPiece(type);
                if(gameOver) break;
            }
            lastFall = millis();
        }
        
        tft.fillRect(T_OFFSET_X, T_OFFSET_Y, T_W*T_SIZE, T_H*T_SIZE, C_BLACK);
        for(int y=0; y<T_H; y++) for(int x=0; x<T_W; x++) if(grid[y][x] != 0) drawBlock(x, y, grid[y][x]);
        for(int i=0; i<4; i++) { int nx = px + currentShape[i][0]; int ny = py + currentShape[i][1]; if(ny >= 0) drawBlock(nx, ny, type); }
        
        tft.setCursor(5, 5); tft.setTextColor(C_TEXT, C_BLACK); tft.print("Score: "); tft.print(score);
        delay(20);
    }
    returnToGameMenu(score);
}
