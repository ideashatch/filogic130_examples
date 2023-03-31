#include <LAudio.h>
#include "ff.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

FRESULT  res;
DIR      dir;
FILINFO  Finfo;

#define MAX_FILES 10
char pcmFile[MAX_FILES][20] = {};

// set the I2C address of the LCD controller
//    0x3F for PCF8574A
//    0x27 for PCF8574
LiquidCrystal_I2C lcd(0x27);

// define the custom bitmaps
// up to 8 bitmaps are supported
const uint8_t my_bitmap[][8] =
{
  // chequer
  {0x15, 0x0A, 0x15, 0x0A, 0x15, 0x0A, 0x15, 0x00},
  // up arrow
  {0x04, 0x0E, 0x1F, 0x04, 0x04, 0x04, 0x00, 0x00},
  // down arrow
  {0x00, 0x00, 0x04, 0x04, 0x04, 0x1F, 0x0E, 0x04},
  // rectangle
  {0x00, 0x1F, 0x11, 0x11, 0x11, 0x11, 0x1F, 0x00},
  // up-left arrow
  {0x1F, 0x1E, 0x1C, 0x1A, 0x11, 0x00, 0x00, 0x00},
  // up-right arrow
  {0x1F, 0x0F, 0x07, 0x0B, 0x11, 0x00, 0x00, 0x00},
  // down-left arrow
  {0x00, 0x00, 0x00, 0x11, 0x1A, 0x1C, 0x1E, 0x1F},
  // down-right arrow
  {0x00, 0x00, 0x00, 0x11, 0x0B, 0x07, 0x0F, 0x1F},
};

#define LONG_DELAY()  delay(1000)
#define SHORT_DELAY() delay(500)
#define ANIM_DELAY()  delay(400)

void setup() 
{
    Serial.begin(115200);
    initLCD();
    Serial.println("Enable Audio");
    LAudio.Begin();
    Serial.println("Mount SD card");
    LAudio.SD_Mount();
    LAudio.FF_LS("SD:/");
    listFile();
    
    Serial.println("List of PCM file in SD Card: ");
    for (int i = 0; i < sizeof(pcmFile)/sizeof(pcmFile[0]); i++) 
    {
        Serial.println(pcmFile[i]);
    }

}


void loop()
{
    int i = 0;
    char tmp[20];
    lcd.clear();    
    for(i = 0; i < sizeof(pcmFile)/sizeof(pcmFile[0]); i++)
    {
        if (pcmFile[i][0] != '\0') 
        {
            Serial.println("Now playing: ");         
            lcd.leftToRight();  
            lcd.print("Now playing: ");
            Serial.print(pcmFile[i]);
            lcd.setCursor(0, 1);            
            strncpy(tmp, pcmFile[i], 20);
            lcd.print( strtok(tmp, ".") );
            LAudio.PlayFromSD(pcmFile[i]);        
        }
        delay(1000);
        lcd.clear();
    }
}


// Store target file name in array and print out
void listFile()
{
    int idx = 0; // for array indexing
    res = f_opendir(&dir, "SD:/");
    if (res)
    {
        printf("[FS]: Folder Open Fail - res(%u)\r\n", res);
    }

    for(;;) 
    {
        res = f_readdir(&dir, &Finfo);
        if (res || !Finfo.fname[0])
        {
            break;
        }

        // Determine .pcm file and store in array
        if(getPCMFile(Finfo.fname))
        {
            strncpy(pcmFile[idx], Finfo.fname, 20); // copy fname into pcmFile[idx]
            idx++;
        }
        printf("%s\n", Finfo.fname);
    }
    f_closedir(&dir);

}

//Determine PCM file
boolean getPCMFile(char fName[])
{
    char* ext;
    
    ext = strchr(fName, '.');
    if(strcmp(ext, ".pcm") == 0) return true;
    else
      return false;
}

void initLCD()
{
    int i;
    int bitmap_size = sizeof(my_bitmap) / sizeof(my_bitmap[0]);
  
    // init the 1602 (2 rows / 16 columns) LCD
    lcd.begin(16, 2);
  
    // register the custom bitmaps
    for (i = 0; i < bitmap_size; i++ )
    {
      lcd.createChar(i, (uint8_t *)my_bitmap[i]);
    }
  
    // move the cursor to 0
    lcd.home();
    lcd.print("          Hello!");
    
    // scroll left for 5 positions
    for (int i = 0; i < 5; i++)
    {
      lcd.scrollDisplayLeft();
      
      ANIM_DELAY();
    }
  
    // clear the LCD contents and the scrolling offset
    lcd.clear();
  
    // turn off the backlight
    lcd.noBacklight();
    
    LONG_DELAY();
  
    // turn on the backlight
    lcd.backlight();
    
    lcd.print("     Hello!     ");
    // change to the starting position of the next line
    lcd.setCursor ( 0, 1 );
    lcd.print("  Filogic 130   ");
  
    // blinks the texts
    for (i = 0; i < 2; i++)
    {
      ANIM_DELAY();
      lcd.noDisplay();
      
      ANIM_DELAY();
      lcd.display();
    }
    
    LONG_DELAY();
    LONG_DELAY();
  
    // clear all LCD contents and settings
    lcd.clear();
    
    SHORT_DELAY();
}
