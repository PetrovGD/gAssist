#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
int selectedItem=2;
int activeMenu=0;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void addMenuItem(char *text, int x, int y, bool selected){
  if(selected){
display.fillRect(x, y, (7*strlen(text))-3, 20, WHITE);
 display.setCursor(x+5,y+5);
  display.setTextColor(BLACK);
  display.print(text);
  }else {
  
  display.setCursor(x+5,y+5);
  display.setTextColor(WHITE);
  display.print(text);
  
  }
}

void printText(char *text, int x, int y, uint16_t color){
  display.setCursor(x,y);
  display.setTextColor(color);
  display.print(text);
}
void notificationsMenu(){
  display.clearDisplay();
  printText("Notifications", 5, 5, WHITE);
  printText("No new notifications", 40, 40, WHITE);
}
void mainMenu(){
  
  display.clearDisplay();
  printText("Main Menu", 5, 5, WHITE);
  switch(selectedItem){
    case 1:
      addMenuItem("Notifications", 5, 20, 1);
      addMenuItem("helloworld!", 5, 40, 0);
      
      break;
    case 2:
      addMenuItem("helloworld!", 5, 40, 1);
      addMenuItem("Notifications", 5, 20, 0);
      break;
  } 

  
}



void setup() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("Screen not working");
    for(;;);
  }
    pinMode(5, INPUT_PULLUP);
    switch(activeMenu){
      case 1:
        notificationsMenu();
        break;
      default:
        mainMenu();
        break;
    }
  
  
   display.display();
  // put your setup code here, to run once:
 
}

void loop() {
  if(digitalRead(5)==0 && activeMenu==0){
    notificationsMenu();
    display.display();
    delay(100);
    activeMenu=1; //Only for testing TODO: add functionality to check what is selected
  }else if(digitalRead(5)==LOW){
    mainMenu();
    display.display();
    delay(100);
    activeMenu=0;
  }
  // put your main code here, to run repeatedly:
}
//We should measure what width does a character have
//The safety barrier is around 7-8 pixels per character added
