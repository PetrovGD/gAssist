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
using namespace std;
BLENotifications notifications; //Init ANCS
NimBLECharacteristic *shutter = nullptr; //Helper for shutter function
static const uint8_t bleReportMap[] = {
  0x05, 0x0C, 0x09, 0x01, 0xA1, 0x01, 0x85, 0x01,
  0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x02,
  0x09, 0xE9, 0x09, 0xEA, 0x81, 0x02,
  0x95, 0x06, 0x81, 0x03, 0xC0
}; //HID report map

bool advertiseAgain = false;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64 //Define screen's width and heigth
#define BTN_UP 5
#define BTN_DOWN 7
#define BTN_LEFT 10
#define BTN_RIGHT 6
#define BTN_ENTER 3   //Pins for the buttons

int previousSelectedItem=-1; //Used to check if there's a change in the selected item
int selectedItem=0; //The item which the user has selected
int activeMenu=0; //Which menu is displayed on the display
int avaliableOptions = 0; //Used to know how many options are avaliable in the menu
int page=0;
string menuItems[2][2] = {{"Нотификации", "Camera Shutter"}}; //Items to a menu. When used menuItems[menu][itemId]

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

void setupHid(NimBLEServer *server){
  auto *hid = new NimBLEHIDDevice(server);
  shutter = hid->getInputReport(1);
  hid->setManufacturer("GP");
  hid->setHidInfo(0x00,0x01);
  hid->setReportMap((uint8_t *)bleReportMap, sizeof(bleReportMap));
  hid->setBatteryLevel(67);
  hid->setPnp(0xFFFF, 0xE502, 0xA111, 0x0210);
} //Setuping up HID for shutter function

void takeAPhoto(){
  if (!shutter) { Serial.println("HID not set up"); return; }
  uint8_t down = 0x01, up = 0x00;
  shutter->setValue(&down, 1);
  shutter->notify();
  delay(20);
  shutter->setValue(&up, 1);
  shutter->notify();
  delay(400);
} //Shutter function

void addMenuItem(string text, int x, int y, bool selected){
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
      
      addMenuItem(menuItems[activeMenu][i], 5, 20*(i+1), (selectedItem==i) ? 1 : 0);
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
   if(notificationsTitleList.empty()) printText("No new notifications", 10, 20, WHITE);
  displayNotificationMenuItems();
  
 // delay(100);
  
  
}//Notification menu
int y=10;
void notificationBodyMenu(){
  display.clearDisplay();
  
  addMenuItem(notificationsBodyList.at(selectedItem), 5, y, false);
  if(digitalRead(BTN_DOWN)==LOW) y-=5;
  if(digitalRead(BTN_UP)==LOW) y+=5;
}

void mainMenu(){
  //Get length of items from the array for the specific active menu
  printText("Main Menu", 5, 7, WHITE);
    displayMainMenuItems();
  
}//Main menu
VoidArray menus[] = {mainMenu, notificationsMenu, notificationBodyMenu}; //Array for the avaliable menus

void updateDisplay() {
  display.clearDisplay();
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
  bool isCyrl=false;
  uint8_t firstByte = static_cast<uint8_t>(title.front());
  (firstByte==208 ? isCyrl=true : isCyrl=false);
  if(!isCyrl){
  if(title.size()>13){
    title.erase(13,title.npos);
    title.append("...");
  }
}else {
  if(title.size()>26){
    title.erase(26,title.npos);
    title.append("...");
  }
}
  if(!(notification->eventFlags & ANCS::EventFlagPreExisting)){
     notificationsTitleList.push_back(title);
     notificationsUUIDList.push_back(rawNotif->uuid);
     notificationsBodyList.push_back(rawNotif->message);
  }
  // int lengthOfItems = sizeof(notificationsTitleList) / sizeof(notificationsTitleList[0]);
  // (lengthOfItems==1 ? notificationsTitleList[0] : notificationsTitleList[lengthOfItems-1]);

  

}

void onNotificationRemoved(const ArduinoNotification *notification, const Notification *rawNotif){
  string title = rawNotif->title;
  int index;
  for(int i=0;i<notificationsUUIDList.size();i++){
    if(notificationsUUIDList[i]==rawNotif->uuid){
       index=i;
       break;
    }
  }
  if(title.size()>13){
    title.erase(13,title.npos);
    title.append("...");
  }
  selectedItem=0;
  page=0;

  notificationsTitleList.erase(notificationsTitleList.begin()+index);
  if(activeMenu==2) activeMenu=1;
  notificationsBodyList.erase(notificationsBodyList.begin()+index);
 // notificationsTitleList.erase(remove(notificationsTitleList.begin(), notificationsTitleList.end(), title.c_str()), notificationsTitleList.end());
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
    pinMode(BTN_ENTER, INPUT_PULLUP); //Enter
    pinMode(BTN_LEFT, INPUT_PULLUP); //Left
    pinMode(BTN_UP, INPUT_PULLUP); //Up
    pinMode(BTN_RIGHT, INPUT_PULLUP); //Right
    pinMode(BTN_DOWN, INPUT_PULLUP); //Down
  // put your setup code here, to run once:
 
}

void loop() {
  if(!advertiseAgain) notifications.startAdvertising();
  updateDisplay(); //Refresh the display's buffer
  if(digitalRead(BTN_ENTER)==0){

    switch(activeMenu){
      case 0:
          activeMenu=menuSelector[activeMenu][selectedItem];
          if(selectedItem==1) takeAPhoto();
          if(activeMenu!=0){
          selectedItem=0;
         page=0;
          }
        break;
      case 1:
        if(avaliableOptions!=0) activeMenu=2;
        break;
    }

    delay(100); 
  }else if( digitalRead(BTN_UP)==LOW){
   if(selectedItem>0 && selectedItem<=avaliableOptions){
    if(selectedItem%2==0 && selectedItem!=0) page--;
     if(activeMenu!=2) selectedItem=selectedItem-1;
    }
    delay(100);
  }else if( digitalRead(BTN_DOWN)==LOW){

   // ( selectedItem>1 ? selectedItem++ : selectedItem--);
    if(selectedItem>=0 && selectedItem<avaliableOptions-1){
      if(selectedItem%2!=0 && selectedItem!=0) page++;
     if(activeMenu!=2) selectedItem=selectedItem+1;

    }
    delay(100);
  }else if( digitalRead(BTN_LEFT)==LOW){

   // ( selectedItem>1 ? selectedItem++ : selectedItem--);
    if(activeMenu>0) activeMenu--;
    delay(100);
  }
  else if( digitalRead(BTN_RIGHT)==LOW){

   // ( selectedItem>1 ? selectedItem++ : selectedItem--);
    takeAPhoto();
    delay(500);
  }
  // put your main code here, to run repeatedly:
}

