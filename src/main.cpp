#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void addMenuItem(char *text){
  display.drawRect(30, 20, (7*strlen(text))-3, 20, WHITE);
  display.setCursor(33,25);
  display.setTextColor(WHITE);

  display.print(text);
  display.display();
}

void setup() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("Screen not working");
    for(;;);
  }
  //Let's test with 7 pixels
  display.clearDisplay();
  //display.drawRect(30, 20, 24, 20, WHITE);
  // display.setCursor(33,25);
  // display.setTextColor(WHITE);
  
  // char *text = "test";
  // display.print(strlen(text));
  // display.display();
  addMenuItem("helloworld!");
  // put your setup code here, to run once:
 
}

void loop() {
  // put your main code here, to run repeatedly:
}
//We should measure what width does a character have
//The safety barrier is around 7-8 pixels per character added
