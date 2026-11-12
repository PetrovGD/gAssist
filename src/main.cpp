#include <Arduino.h> //Include Arduino Library
#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h> //Include display's libraries
#include <Wire.h> // Include I2C library
#include "esp32notifications.h"
#include <atomic>
#include <vector>
#include <string>
#include <fonts/CourierCyr6.h>
using namespace std;
BLENotifications notifications;
bool advertiseAgain = false;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64 //Define screen's width and heigth
#define BTN_UP 5
#define BTN_DOWN 7
#define BTN_LEFT 10
#define BTN_RIGHT 6
#define BTN_ENTER 3   

int previousSelectedItem=-1; //Used to check if there's a change in the selected item
int selectedItem=0; //The item which the user has selected
int activeMenu=0; //Which menu is displayed on the display
bool previousButtonState=1; //Used to check if a button has been pressed
int avaliableOptions = 0; //Used to know how many options are avaliable in the menu
int page=0;
string menuItems[2][2] = {{"Нотификации", "Hello world"}}; //Items to a menu. When used menuItems[menu][itemId]

int menuSelector[2][2] = {{1,0}}; //Menu to an item. When used menuSelector[menu][itemId] and returns the menu which the item should open
std::vector <std::string> notificationsList = {};
std::vector <std::uint32_t> notificationsUUIDList = {};
//string notificationsList[1];
typedef void (*VoidArray) (); //Defining an array of voids
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); //Init display
// #define OLED_MOSI   7
// #define OLED_CLK   10
// #define OLED_DC    5
// #define OLED_CS    12
// #define OLED_RESET 6
// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
void addMenuItem(string text, int x, int y, bool selected){
  if(selected){
display.fillRect(x, y, (7*text.size())-3, 20, WHITE);
 display.setCursor(x+5,y+5);
  display.setTextColor(BLACK);
  display.print(text.c_str());
  }else {
  
  display.setCursor(x+5,y+5);
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

} //It adds a menu's items to the display's buffer

void displayNotificationMenuItems(){
      int lengthOfItems = notificationsList.size(); //Get length of items from the array for the specific active menu
      // if(activeMenu!=1) avaliableOptions = lengthOfItems; //Set the global avaliableOptions variable to the calculated length of the items in the array
      // if (activeMenu==1) avaliableOptions = 2;
      avaliableOptions=lengthOfItems;
      if(previousSelectedItem!=selectedItem && lengthOfItems!=0) {
      for(int i=0;i<lengthOfItems;i++){
      
      addMenuItem(notificationsList[i], 5, 20*(i+1)-(40*page), (selectedItem==i) ? 1 : 0);
      }//Check for changes in the selected item and update the display buffer

    }

} //It adds a menu's items to the display's buffer

void notificationsMenu(){
  display.clearDisplay();
  // printText("Notifications", 5, 5, WHITE);
   if(notificationsList.empty()) printText("No new notifications", 10, 20, WHITE);
  displayNotificationMenuItems();
  
 // delay(100);
  
  
}//Notification menu

void mainMenu(){
  //Get length of items from the array for the specific active menu
  printText("Main Menu", 5, 5, WHITE);
    displayMainMenuItems();
  
}//Main menu
VoidArray menus[] = {mainMenu, notificationsMenu}; //Array for the avaliable menus

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
  if(title.size()>13){
    title.erase(13,title.npos);
    title.append("...");
  }
  if(!(notification->eventFlags & ANCS::EventFlagPreExisting)){
     notificationsList.push_back(title);
     notificationsUUIDList.push_back(rawNotif->uuid);
  }
  // int lengthOfItems = sizeof(notificationsList) / sizeof(notificationsList[0]);
  // (lengthOfItems==1 ? notificationsList[0] : notificationsList[lengthOfItems-1]);

  

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
  notificationsList.erase(notificationsList.begin()+index);
 // notificationsList.erase(remove(notificationsList.begin(), notificationsList.end(), title.c_str()), notificationsList.end());
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
    
    notifications.begin("gA");
 
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
    selectedItem=0;
    page=0;
    activeMenu=menuSelector[activeMenu][selectedItem];
    delay(100); 
  }else if( digitalRead(BTN_UP)==LOW){
   if(selectedItem>0 && selectedItem<=avaliableOptions){
    if(selectedItem%2==0 && selectedItem!=0) page--;
      selectedItem=selectedItem-1;
    }
    delay(100);
  }else if( digitalRead(BTN_DOWN)==LOW){

   // ( selectedItem>1 ? selectedItem++ : selectedItem--);
    if(selectedItem>=0 && selectedItem<avaliableOptions-1){
      if(selectedItem%2!=0 && selectedItem!=0) page++;
      selectedItem=selectedItem+1;

    }
    delay(100);
  }
  // put your main code here, to run repeatedly:
}

