#include <Arduino.h> //Include Arduino Library
#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h> //Include display's libraries
#include <Wire.h> // Include I2C library

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64 //Define screen's width and heigth
int previousSelectedItem=-1; //Used to check if there's a change in the selected item
int selectedItem=0; //The item which the user has selected
int activeMenu=0; //Which menu is displayed on the display
bool previousButtonState=1; //Used to check if a button has been pressed
int avaliableOptions = 0; //Used to know how many options are avaliable in the menu
char *menuItems[2][2] = {{"Notifications", "Hello world"}
                         }; //Items to a menu. When used menuItems[menu][itemId]
int menuSelector[2][2] = {{1,0}}; //Menu to an item. When used menuSelector[menu][itemId] and returns the menu which the item should open

typedef void (*VoidArray) (); //Defining an array of voids
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); //Init display

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
} //It add a single item to the display buffer

void printText(char *text, int x, int y, uint16_t color){
  display.setCursor(x,y);
  display.setTextColor(color);
  display.print(text);
} //Prints a text on the display

void displayMenuItems(){
      if(menuItems[activeMenu][0]==NULL) {
        return;
      }
      int lengthOfItems = (sizeof(menuItems[activeMenu])/sizeof(menuItems[activeMenu][0])); //Get length of items from the array for the specific active menu
      avaliableOptions = lengthOfItems; //Set the global avaliableOptions variable to the calculated length of the items in the array
      for(int i=0;i<lengthOfItems;i++){
      if(previousSelectedItem!=selectedItem && lengthOfItems!=0) {
      addMenuItem(menuItems[activeMenu][i], 5, 20*(i+1), (selectedItem==i) ? 1 : 0);

      }//Check for changes in the selected item and update the display buffer
       else {return;}
    }

} //It adds a menu's items to the display's buffer
void notificationsMenu(){
  display.clearDisplay();
  printText("Notifications", 5, 5, WHITE);
  printText("No new notifications", 40, 40, WHITE);
  displayMenuItems();
}//Notification menu

void mainMenu(){
 
  printText("Main Menu", 5, 5, WHITE);
    displayMenuItems();
  
}//Main menu
VoidArray menus[] = {mainMenu, notificationsMenu}; //Array for the avaliable menus

void updateDisplay() {
  display.clearDisplay();
  menus[activeMenu]();
   display.display();
}//It refreshes the display with the new information about the menus

void setup() {
  Serial.begin(9600);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("Screen not working");
    for(;;);
  }
    pinMode(3, INPUT_PULLUP); //Enter
    pinMode(10, INPUT_PULLUP); //Left
    pinMode(5, INPUT_PULLUP); //Up
    pinMode(6, INPUT_PULLUP); //Right
    pinMode(7, INPUT_PULLUP); //Down
    
  // put your setup code here, to run once:
 
}

void loop() {
  updateDisplay(); //Refresh the display's buffer
  if(digitalRead(3)==0){
    activeMenu=menuSelector[activeMenu][selectedItem];
    delay(100); 
  }else if( digitalRead(5)==LOW){
   if(selectedItem>0 && selectedItem<=avaliableOptions){
      selectedItem=selectedItem-1;
    }
    delay(100);
  }else if( digitalRead(7)==LOW){

   // ( selectedItem>1 ? selectedItem++ : selectedItem--);
    if(selectedItem>=0 && selectedItem<avaliableOptions-1){
      selectedItem=selectedItem+1;
    }
    delay(100);
  }
  // put your main code here, to run repeatedly:
}

