#include <Arduino.h> //Include Arduino Library
#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h> //Include display's libraries
#include <Wire.h> // Include I2C library
#include "esp32notifications.h"
#include <atomic>
#include <vector>
#include <string>
#include <fonts/CourierCyr6.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <Bounce2.h>
#include "bleReportMap.h"
using namespace std;
BLENotifications notifications; //Init ANCS
NimBLECharacteristic *shutter = nullptr; //Helper for shutter function


bool advertiseAgain = false;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64 //Define screen's width and heigth
#define BTN_UP 5
#define BTN_DOWN 7
#define BTN_LEFT 10
#define BTN_RIGHT 6
#define BTN_ENTER 3   //Pins for the buttons
Bounce2::Button buttonUp;
Bounce2::Button buttonDown;
Bounce2::Button buttonLeft;
Bounce2::Button buttonRight;
Bounce2::Button buttonEnter;
int previousSelectedItem=-1; //Used to check if there's a change in the selected item
int selectedItem=0; //The item which the user has selected
int activeMenu=0; //Which menu is displayed on the display
int previousMenu=0;
int avaliableOptions = 0; //Used to know how many options are avaliable in the menu
int page=0;
int indexOfCallingNotification=0;

string menuItems[2][3] = {{"Notifications", "Camera Shutter", "Media control"}}; //Items to a menu. When used menuItems[menu][itemId]

int menuSelector[2][2] = {{1,0}}; //Menu to an item. When used menuSelector[menu][itemId] and returns the menu which the item should open
std::vector <std::string> notificationsTitleList = {}; //Vector for title of received notifications
std::vector <std::uint32_t> notificationsUUIDList = {}; //Vector for UUID of received notifications
std::vector <std::string> notificationsBodyList = {};//Vector for body (message) of received notifications
typedef void (*VoidArray) (); //Defining an array of voids
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); //Init display
// #define OLED_MOSI   7
// #define OLED_CLK   10
// #define OLED_DC    5
// #define OLED_CS    12
// #define OLED_RESET 6
// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

//Setuping up HID for shutter function
void setupHid(NimBLEServer *server){
  auto *hid = new NimBLEHIDDevice(server);
  shutter = hid->getInputReport(1);
  hid->setManufacturer("GP");
  hid->setHidInfo(0x00,0x01);
  hid->setReportMap((uint8_t *)bleReportMap, sizeof(bleReportMap));
  hid->setBatteryLevel(67);
  hid->setPnp(0xFF, 0xE502, 0xA111, 0x0210);
} 

void takeAPhoto(){
  uint8_t down = 0x01, up = 0x00;
  shutter->setValue(&down, 1);
  shutter->notify();
  delay(20);
  shutter->setValue(&up, 1);
  shutter->notify();
  delay(400);
} //Shutter function
// void playPause(){
//   uint8_t playButton = 0x04;
//   uint8_t release = 0x00;
//   shutter->setValue(&playButton, 1);
//   shutter->notify();
//   delay(20);
//   shutter->setValue(&release, 1);
//   shutter->notify();
// }
void addMenuItem(string text, int x, int y, bool selected){ //Add horisontal scrolling text when selected
  bool isCyrl=false; //Checking if the notification is in cyrillic
  uint8_t firstByte = static_cast<uint8_t>(text.front()); //Getting the first byte because all cyrillic strings in cpp start with the same byte
  (firstByte==208 ? isCyrl=true : isCyrl=false);
  
  if(selected){
  if(!isCyrl) display.fillRect(x, y-3, (7*text.size())+6, 20, WHITE); 
  else display.fillRect(x, y-3, (7*text.size()/2)+6, 20, WHITE); //Cyrillic strings are twice in size as latin
 display.setCursor(x+5,y+7);
  display.setTextColor(BLACK);
  display.print(text.c_str());
  }else {
    if(activeMenu==1) {
  if(!isCyrl){
  if(text.size()>13){
    text.erase(13,text.npos);
    text.append("...");
  }
}else {
  if(text.size()>26){
    text.erase(26,text.npos);
    text.append("...");
  }
}
    }
  display.setCursor(x+5,y+7);
  display.setTextColor(WHITE);
  display.print(text.c_str());
  
  }
} //It add a single item to the display buffer

void printText(char *text, int x, int y, uint16_t color){
  display.setCursor(x,y);
  display.setTextColor(color);
  display.print(text);
} //Prints a text on the display

void displayMainMenuItems(){
      int lengthOfItems = (sizeof(menuItems[0])/sizeof(menuItems[0][0])); //Get length of items from the array for the specific active menu
      // if(activeMenu!=1) avaliableOptions = lengthOfItems; //Set the global avaliableOptions variable to the calculated length of the items in the array
      // if (activeMenu==1) avaliableOptions = 2;
      avaliableOptions=lengthOfItems;
      if(previousSelectedItem!=selectedItem && lengthOfItems!=0) {
      for(int i=0;i<lengthOfItems;i++){
      
      addMenuItem(menuItems[activeMenu][i], 5, 20*(i+1)-(40*page), (selectedItem==i) ? 1 : 0);
      }//Check for changes in the selected item and update the display buffer

    }

} //It adds main menu's items to the display's buffer

void displayNotificationMenuItems(){
      int lengthOfItems = notificationsTitleList.size(); //Get length of items from the array for the specific active menu
      // if(activeMenu!=1) avaliableOptions = lengthOfItems; //Set the global avaliableOptions variable to the calculated length of the items in the array
      // if (activeMenu==1) avaliableOptions = 2;
      avaliableOptions=lengthOfItems;
      if(previousSelectedItem!=selectedItem && lengthOfItems!=0) {
      for(int i=0;i<lengthOfItems;i++){

      addMenuItem(notificationsTitleList[i], 5, 20*(i+1)-(40*page), (selectedItem==i) ? 1 : 0);
      }//Check for changes in the selected item and update the display buffer

    }

} //It adds notification menu's items to the display's buffer

void notificationsMenu(){
  display.clearDisplay();
  // printText("Notifications", 5, 5, WHITE);
   if(notificationsTitleList.empty()) printText("No new notifications", 10, 20, 1);
  displayNotificationMenuItems();
  
 // delay(100);
  
}//Notification menu

bool deleteAnItem=false;
bool userSubmitted=false;
void deleteNotificationMenu(){
  printText("Delete the notification?", 15, 5, 1);
  addMenuItem("Yes", 15, 40, deleteAnItem);
  addMenuItem("No", 45, 40, !deleteAnItem);
  if(buttonLeft.pressed() || buttonRight.pressed()){
    deleteAnItem=!deleteAnItem; 
    
    //delay(300);
  } 
  if(buttonEnter.pressed()){
    userSubmitted=true;
    //delay(300);
  } 
  if(deleteAnItem && userSubmitted){
  notificationsTitleList.erase(notificationsTitleList.begin()+selectedItem);
 notificationsBodyList.erase(notificationsBodyList.begin()+selectedItem);
 notificationsUUIDList.erase(notificationsUUIDList.begin()+selectedItem); //FIX problem with deletion 
 activeMenu=1;
 deleteAnItem=false;
 userSubmitted=false;
  }else if(!deleteAnItem && userSubmitted){
     activeMenu=1;
     deleteAnItem=false;
     userSubmitted=false;
  }
}

int yNotificationBody=10;
void notificationBodyMenu(){
  display.clearDisplay();
  
  addMenuItem(notificationsBodyList.at(selectedItem), 5, yNotificationBody, false);
  if(buttonDown.pressed()) yNotificationBody-=5;
  if(buttonUp.pressed()) yNotificationBody+=5;
  if(buttonRight.pressed()){
     deleteNotificationMenu();
     activeMenu=4;
  }
  if(buttonLeft.pressed()) yNotificationBody=10;
}


void displayIncomingCall(){
  display.clearDisplay();
 
 // Serial.print(notificationsBodyList.at(indexOfCallingNotification+1));
 addMenuItem(notificationsBodyList.at(indexOfCallingNotification-1), 5, 5, 0);
 addMenuItem(("Caller: "+notificationsTitleList.at(indexOfCallingNotification-1)), 5, 20, 0);
 addMenuItem("Press LEFT\n to decline", 5, 30, 0);
  if(buttonLeft.pressed()) notifications.actionNegative(notificationsUUIDList.at(indexOfCallingNotification-1));
}
void mainMenu(){
  //Get length of items from the array for the specific active menu
 if(page==0) printText("Main Menu", 5, 7, 1);
    displayMainMenuItems();
  
}//Main menu

void mediaControlMenu(){
  avaliableOptions=4;
  switch(selectedItem){
    case 0:
      addMenuItem("<<", 20, 40, 1);
      display.setFont();
      addMenuItem("", 45, 35, 0);
      display.setFont(&CourierCyr6pt8b);
      addMenuItem(">>", 60, 40, 0);
      addMenuItem("back", 85, 40, 0);
      if(buttonEnter.pressed()){
        uint8_t previous = 0x02;
        uint8_t release = 0x00;
        shutter->setValue(&previous, 1);
        shutter->notify();
        delay(20);
        shutter->setValue(&release, 1);
        shutter->notify();
      } 
      break;
    case 1:
      addMenuItem(">>", 60, 40, 0);
      addMenuItem("<<", 20, 40, 0);
      display.setFont();
      addMenuItem("", 45, 35, 1);
      display.setFont(&CourierCyr6pt8b);
      addMenuItem("back", 85, 40, 0);
      if(buttonEnter.pressed()){
        uint8_t playButton = 0x08;
        uint8_t release = 0x00;
        shutter->setValue(&playButton, 1);
        shutter->notify();
        delay(20);
        shutter->setValue(&release, 1);
        shutter->notify();
      }
      break;
    case 2:
      addMenuItem("<<", 20, 40, 0);
      addMenuItem(">>", 60, 40, 1);
      display.setFont();
      addMenuItem("", 45, 35, 0);
      display.setFont(&CourierCyr6pt8b);
      addMenuItem("back", 85, 40, 0);
      if(buttonEnter.pressed()){
        uint8_t next = 0x04;
        uint8_t release = 0x00;
        shutter->setValue(&next, 1);
        shutter->notify();
        delay(20);
        shutter->setValue(&release, 1);
        shutter->notify();
      } 
      break;
    case 3:
      addMenuItem("<<", 20, 40, 0);
      addMenuItem(">>", 60, 40, 0);
      display.setFont();
      addMenuItem("", 45, 35, 0);
      display.setFont(&CourierCyr6pt8b);
      addMenuItem("back", 85, 40, 1);
      if(buttonEnter.pressed()){
          activeMenu=0;
      }
      break;
  }



}

VoidArray menus[] = {mainMenu, notificationsMenu, notificationBodyMenu, displayIncomingCall, deleteNotificationMenu, mediaControlMenu}; //Array for the avaliable menus

void updateDisplay() {
  display.clearDisplay();
  if(previousMenu!=activeMenu){
    if(previousMenu!=1){
    page=0;
    selectedItem=0;
    previousMenu=activeMenu;
    }
     else {
       previousMenu=activeMenu;
     }
  }
  menus[activeMenu]();
   display.display();
}//It refreshes the display with the new information about the menus
void onBleStateChange(BLENotifications::State state){
  if(state==BLENotifications::StateConnected){
    Serial.println("conn");
    advertiseAgain=true;
  }
}

void onNotificationReceive(const ArduinoNotification *notification, const Notification *rawNotif){
  //const String title = String(notification->title);
  string title = rawNotif->title;
  NotificationCategory category = notification->category;
  
  if(!(notification->eventFlags & ANCS::EventFlagPreExisting) && notification->category!=2){
     notificationsTitleList.push_back(title);
     notificationsUUIDList.push_back(rawNotif->uuid);
     notificationsBodyList.push_back(rawNotif->message);
     Serial.println();
     Serial.print("NEW: UUID: ");
      // Serial.print(notificationsUUIDList[notificationsUUIDList.size()]);
      Serial.print(notification->uuid);
       Serial.print(" ; category: ");
       Serial.print(rawNotif->category);
       Serial.print("; Title: ");
       Serial.print(notification->title);
    // Serial.println(notification->title);
   //  Serial.println(category);
  }
  if(category==1){
    activeMenu=3;
    // for(int i=0;i<notificationsUUIDList.size();i++){
    // }
    indexOfCallingNotification=notificationsUUIDList.size();
   
  }
  // int lengthOfItems = sizeof(notificationsTitleList) / sizeof(notificationsTitleList[0]);
  // (lengthOfItems==1 ? notificationsTitleList[0] : notificationsTitleList[lengthOfItems-1]);

  

}

void onNotificationRemoved(const ArduinoNotification *notification, const Notification *rawNotif){
  string title = rawNotif->title;
  int index;
  for(int i=0;i<notificationsUUIDList.size();i++){
    Serial.println();
     Serial.print("OLD: UUID: ");
      // Serial.print(notificationsUUIDList[notificationsUUIDList.size()]);
      Serial.print(notification->uuid);
       Serial.print(" ; category: ");
       Serial.print(rawNotif->category);
       Serial.print("; Title: ");
       Serial.print(notification->title);
    if(notificationsUUIDList[i]==rawNotif->uuid && notification->category!=2){
       index=i;
       
       if(activeMenu==2) activeMenu=1;
       if(activeMenu==3) activeMenu=0;
       notificationsTitleList.erase(notificationsTitleList.begin()+index);
  
 notificationsBodyList.erase(notificationsBodyList.begin()+index);
 notificationsUUIDList.erase(notificationsUUIDList.begin()+index);
       break;
    }
  }
 // selectedItem=0;
  page=0;
  
}

void setup() {
  Serial.begin(115200);
  //Wire.begin(8, 9);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("Screen not working");
    for(;;);
  }
   display.setFont(&CourierCyr6pt8b);
    notifications.setConnectionStateChangedCallback(onBleStateChange);
    notifications.setNotificationCallback(onNotificationReceive);
    notifications.setRemovedCallback(onNotificationRemoved);
    
    notifications.begin("gA", setupHid);
    buttonEnter.attach(BTN_ENTER, INPUT_PULLUP); //Enter
    buttonLeft.attach(BTN_LEFT, INPUT_PULLUP); //Left
    buttonUp.attach(BTN_UP, INPUT_PULLUP); //Up
    buttonRight.attach(BTN_RIGHT, INPUT_PULLUP); //Right
    buttonDown.attach(BTN_DOWN, INPUT_PULLUP); //Down
    
    buttonEnter.interval(25);
    buttonLeft.interval(25);
    buttonUp.interval(25);
    buttonRight.interval(25);
    buttonDown.interval(25);

    buttonEnter.setPressedState(LOW);
    buttonLeft.setPressedState(LOW);
    buttonUp.setPressedState(LOW);
    buttonRight.setPressedState(LOW);
    buttonDown.setPressedState(LOW);
  // put your setup code here, to run once:
 
}

void loop() {
    buttonEnter.update();
    buttonLeft.update();
    buttonUp.update();
    buttonRight.update();
    buttonDown.update();
  if(!advertiseAgain) notifications.startAdvertising();
  updateDisplay(); //Refresh the display's buffer
  if(buttonEnter.pressed()){

    switch(activeMenu){
      case 0:
         // activeMenu=menuSelector[activeMenu][selectedItem];
          if(selectedItem==1) takeAPhoto();
          switch (selectedItem){
            case 0:
              activeMenu=1;
              break;
            case 1:
              takeAPhoto;
              break;
            case 2:
              activeMenu=5;
              break;
          }
          if(activeMenu!=0){
        //  selectedItem=0;
         page=0;
          }
        break;
      case 1:
        if(avaliableOptions!=0) activeMenu=2;
        break;
    
  }
   // delay(100); 
  }else if( buttonUp.pressed()){
   if(selectedItem>0 && selectedItem<=avaliableOptions){
    if(selectedItem%2==0 && selectedItem!=0) page--;
     if(activeMenu!=2) selectedItem=selectedItem-1;
    }
   // delay(100);
  }else if( buttonDown.pressed()){

   // ( selectedItem>1 ? selectedItem++ : selectedItem--);
    if(selectedItem>=0 && selectedItem<avaliableOptions-1){
      if(selectedItem%2!=0 && selectedItem!=0) page++;
     if(activeMenu!=2) selectedItem=selectedItem+1;

    }
    //delay(100);
  }else if( buttonLeft.pressed()){

   // ( selectedItem>1 ? selectedItem++ : selectedItem--);
    if(activeMenu>0 && activeMenu!=3 && activeMenu!=4 && activeMenu!=5){
       activeMenu--;
        page=0;
        selectedItem=0;
    }
    if(selectedItem>0 && selectedItem<=avaliableOptions && activeMenu==5){ 
        selectedItem=selectedItem-1;
    }
  }
  else if( buttonRight.pressed()){

   // ( selectedItem>1 ? selectedItem++ : selectedItem--);
        
     if(selectedItem>=0 && selectedItem<avaliableOptions-1 && activeMenu==5){ 
        selectedItem=selectedItem+1;
    }
    
  }
  // put your main code here, to run repeatedly:
}

