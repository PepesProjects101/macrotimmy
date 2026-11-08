#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

#define JOY_X A0   
#define JOY_Y A1   
#define JOY_SW 25   

// pin order for the switches
const int BTN_PINS = {3, 2, 40, 13, 8, 9, 34, 32, 10, 6};

// start apps at 100
int volumes = {0, 100, 100, 100};      
int brightness = {100, 100, 100};     

int currentPage = 0; 
int currentSel = 0;  

unsigned long lastScrollTime = 0; 
unsigned long lastNavTime = 0;
unsigned long lastBtnTime = 0;

void setup() {
  Serial.begin(115200); 

  pinMode(JOY_SW, INPUT_PULLUP);
  for(int i=0; i<10; i++) pinMode(BTN_PINS[i], INPUT_PULLUP);

  u8g2.begin();
  
  // splash screen
  u8g2.clearBuffer();
  u8g2.drawRFrame(0, 0, 128, 64, 5); 
  u8g2.setFont(u8g2_font_ncenB12_tr); 
  u8g2.drawStr(15, 32, "MacroTimmy");
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(25, 52, "Usb Connected");
  u8g2.sendBuffer();
  delay(1500); 
}

void loop() {
  handleJoystick();
  handleMacroButtons();
  readPC(); 
  drawScreen();
}

void sendToPC(String cmd) {
  Serial.println(cmd); 
}

void readPC() {
  if (Serial.available() > 0) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    
    // grab volume sync from the python script
    if (line.startsWith("SYNC_VOL:")) {
      String vals = line.substring(9);
      for(int i=0; i<4; i++) {
        int commaIndex = vals.indexOf(',');
        if(commaIndex == -1) commaIndex = vals.length();
        volumes[i] = vals.substring(0, commaIndex).toInt();
        vals = vals.substring(commaIndex + 1);
      }
    }
  }
}

void handleJoystick() {
  int xVal = analogRead(JOY_X); 
  int yVal = analogRead(JOY_Y); 
  int swState = digitalRead(JOY_SW);

  // menu navigation
  if (currentPage != 0 && millis() - lastNavTime > 200) { 
    if (yVal > 900) { 
       currentSel--; 
       int maxItems = (currentPage == 1) ? 3 : 2; 
       if (currentSel < 0) currentSel = maxItems; 
       lastNavTime = millis();
    }
    else if (yVal < 100) { 
       currentSel++; 
       int maxItems = (currentPage == 1) ? 3 : 2;
       if (currentSel > maxItems) currentSel = 0; 
       lastNavTime = millis();
    }
  }

  // turning the knob
  if (millis() - lastScrollTime > 150) { 
    if (xVal < 100 || xVal > 900) {
      int dir = (xVal > 900) ? -1 : 1; 

      if (currentPage == 0) { 
        volumes += (dir * 2); 
        volumes = constrain(volumes, 0, 100);
        sendToPC("VOL_SET:0," + String(volumes));
      }
      else if (currentPage == 1) { 
        volumes[currentSel] += (dir * 2); 
        volumes[currentSel] = constrain(volumes[currentSel], 0, 100);
        sendToPC("VOL_SET:" + String(currentSel) + "," + String(volumes[currentSel]));
      }
      else if (currentPage == 2) { 
        brightness[currentSel] += (dir * 5); 
        brightness[currentSel] = constrain(brightness[currentSel], 0, 100);
        sendToPC("BRI_SET:" + String(currentSel) + "," + String(brightness[currentSel]));
        if(currentSel == 2) { brightness = brightness; brightness = brightness; }
      }
      lastScrollTime = millis();
    }
  }

  // click to change page
  if (swState == LOW && millis() - lastBtnTime > 300) { 
    currentPage++;
    if (currentPage > 2) currentPage = 0;
    currentSel = 0; 
    u8g2.clearBuffer(); u8g2.sendBuffer(); 
    lastBtnTime = millis();
  }
}

void handleMacroButtons() {
  for(int i=0; i<10; i++) {
    if(digitalRead(BTN_PINS[i]) == LOW) {
      sendToPC("BTN_" + String(i+1)); 
      
      // little popup when a macro is pressed
      u8g2.clearBuffer();
      u8g2.drawBox(0, 0, 128, 16); 
      u8g2.setDrawColor(0);
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(34, 11, "MACRO ACTIVE");
      u8g2.setDrawColor(1);
      u8g2.setFont(u8g2_font_logisoso24_tr);
      u8g2.setCursor(56, 50); u8g2.print(i+1);
      u8g2.drawRFrame(10, 20, 108, 40, 4); 
      u8g2.sendBuffer();
      
      delay(200); 
      while(digitalRead(BTN_PINS[i]) == LOW); 
    }
  }
}

// helper functions for drawing the bars
void drawHBar(int x, int y, int width, int height, int percentage) {
  u8g2.drawRFrame(x, y, width, height, 2);
  int t = width / 4; int f = map(percentage, 0, 100, 0, t);
  for (int i = 0; i < f; i++) u8g2.drawBox(x + 2 + (i * 4), y+2, 3, height-4);
}
void drawVBar(int x, int y, int width, int height, int percentage) {
  u8g2.drawRFrame(x, y, width, height, 2);
  int t = height / 4; int f = map(percentage, 0, 100, 0, t);
  for (int i = 0; i < f; i++) u8g2.drawBox(x+2, (y + height) - (i * 4) - 3, width-4, 3);
}

void drawScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  
  if (currentPage == 0) { 
    u8g2.drawBox(0, 0, 128, 12); u8g2.setDrawColor(0); 
    u8g2.drawStr(34, 9, "MacroTimmy"); 
    
    u8g2.setDrawColor(1); u8g2.drawStr(34, 28, "MASTER VOL");
    drawHBar(4, 32, 120, 12, volumes); 
    drawHBar(4, 50, 120, 10, brightness);
  }
  else if (currentPage == 1) { 
    const char* l[] = {"MAS", "SPO", "GAM", "MIC"};
    u8g2.drawBox(0, 0, 128, 12); u8g2.setDrawColor(0);
    u8g2.drawStr(30, 9, "AUDIO MIXER"); u8g2.setDrawColor(1);
    for(int i=0; i<4; i++) {
      int x = 6 + (i * 32); 
      if(i == currentSel) u8g2.drawRFrame(x-2, 14, 24, 45, 3);
      drawVBar(x, 16, 20, 38, volumes[i]);
      u8g2.setCursor(x+1, 63); u8g2.print(l[i]);
    }
  }
  else if (currentPage == 2) { 
    const char* l[] = {"M1", "M2", "ALL"};
    u8g2.drawBox(0, 0, 128, 12); u8g2.setDrawColor(0);
    u8g2.drawStr(35, 9, "MONITOR SETUP"); u8g2.setDrawColor(1);
    int p[] = {11, 53, 95};
    for(int i=0; i<3; i++) {
      int x = p[i];
      if(i == currentSel) u8g2.drawRFrame(x-2, 14, 24, 45, 3);
      drawVBar(x, 16, 20, 38, brightness[i]);
      u8g2.setCursor(x+4, 63); u8g2.print(l[i]);
    }
  }
  u8g2.sendBuffer();
}