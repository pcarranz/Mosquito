 /*
  * San Luis Obispo Children's Museum Hearing Test Exhibit
  * Created by: Patricia Carranza, 2014
  *
  * Pin change interrupt code adapted from PinChangeInt library:
  * Copyright 2011 Lex.V.Talionis at gmail
  * https://code.google.com/p/arduino-pinchangeint/wiki/Logic
  *
  * SD card initialization an indexFiles() function adapted from 
  * daphc.pde's play() function found in the WaveHC library's example files.
  * https://code.google.com/p/wavehc/
  *
  */

#include <PinChangeInt.h>
#include <PinChangeIntConfig.h>
#include <WaveHC.h>
#include <WaveUtil.h>

#define NUM_MODS 6      // Number of LED modules
#define NUM_BUTTONS 10  // Number of input buttons
#define NUM_LEDS 10     // Number of button LEDs
#define NUM_SOUNDS 5    // Number wave files on SD card
  
int startButton = 17; 
int startLed = 16; 

int buttons[NUM_BUTTONS] = {14, 15, A8, A9, A10, A11, A12, A13, A14, A15}; 
int buttonLeds[NUM_LEDS] = {30, 32, 28, 26, 24, 22, 21, 20, 19, 18};

int ledMods[NUM_MODS]   = {46,47, 44, 45, 42, 43};

int reading;           // the current reading from the start button pin
int previous = LOW;    // the previous reading from the start button pin

/* Note: Using an RC filter instead of software debouncing
 * C = 2 * 0.1uF, R = 10k-ohm
 */  
 
// WaveHC variables
SdReader card;    // This object holds the information for the card
FatVolume vol;    // This holds the information for the partition on the card
FatReader root;   // This holds the information for the volumes root directory
FatReader file;   // This object represent the WAV file 
WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time

uint8_t dirLevel; // indent level for file/dir names    (for prettyprinting)
dir_t dirBuf;     // buffer for directory reads

FatReader files[NUM_SOUNDS]; // save wave files found on sd card
int filesIndex = 0;          // index counter for files

// Function definitions (we define them here, but the code is below)
void indexFiles(FatReader &dir);

// Declare reset function at address 0
void(* resetFunc) (void) = 0; 

/* -----------------------
 * SETUP
 */
void setup()
{ 
  Serial.begin(9600);
  
  int i = 0;
  
  // Start button
  pinMode(startButton, INPUT);
  pinMode(startLed, OUTPUT);
  
  // Buttons
  for(i = 0; i < NUM_BUTTONS; i++)
  {
    pinMode(buttons[i], INPUT); 
  } 
  
  // Button LEDs
  for(i = 0; i < NUM_LEDS; i++)
  {
    pinMode(buttonLeds[i], OUTPUT);
    digitalWrite(buttonLeds[i], LOW);  
  } 
  
  // LED Modules
  for(i = 0; i < NUM_MODS; i++)
  {
    pinMode(ledMods[i], OUTPUT);
    digitalWrite(ledMods[i], LOW);  
  }  
  
  // Button Interrupts
  PCintPort::attachInterrupt(buttons[0], switchLed0, RISING);
  PCintPort::attachInterrupt(buttons[1], switchLed1, RISING);
  PCintPort::attachInterrupt(buttons[2], switchLed2, RISING);
  PCintPort::attachInterrupt(buttons[3], switchLed3, RISING);
  PCintPort::attachInterrupt(buttons[4], switchLed4, RISING);
  PCintPort::attachInterrupt(buttons[5], switchLed5, RISING);
  PCintPort::attachInterrupt(buttons[6], switchLed6, RISING);
  PCintPort::attachInterrupt(buttons[7], switchLed7, RISING);
  PCintPort::attachInterrupt(buttons[8], switchLed8, RISING);
  PCintPort::attachInterrupt(buttons[9], switchLed9, RISING);

  // WaveHC setup
  Serial.println(FreeRam());
  //if (!card.init(true)) { //play with 4 MHz spi if 8MHz isn't working for you
  if (!card.init()) {         //play with 8 MHz spi (default faster!)  
      Serial.println("SD card Init failed");
  }
  
  // enable optimize read - some cards may timeout. Disable if you're having problems
  //card.partialBlockRead(true);

  // Now we will look for a FAT partition!
  uint8_t part;
  for (part = 0; part < 5; part++) {   // we have up to 5 slots to look in
    if (vol.init(card, part)) 
      break;                           // we found one, lets bail
  }
  if (part == 5) {                     // if we ended up not finding one  :(
    Serial.println("No valid FAT partition!");  // Something went wrong, lets print out why
  }
  
  // Lets tell the user about what we found
  putstring("Using partition ");
  Serial.print(part, DEC);
  putstring(", type is FAT");
  Serial.println(vol.fatType(), DEC);     // FAT16 or FAT32?
  
  // Try to open the root directory
  if (!root.openRoot(vol)) {
    Serial.println("Can't open root dir!");      // Something went wrong,
  }
  
  // Whew! We got past the tough parts.
  putstring_nl("Files found (* = fragmented):");

  // Print out all of the files in all the directories.
  root.ls(LS_R | LS_FLAG_FRAGMENTED);
  
  root.rewind();
  indexFiles(root);
}

/* -----------------------
 * LOOP
 */
void loop()
{
   int i = 0;
   int playIndex = 0;
   reading = digitalRead(startButton);  // Get start button state
  
  // Turn on "Hold start to begin" LED module 
  digitalWrite(ledMods[0], HIGH);
  
  // Blink start button LED while standing by
  digitalWrite(startLed, HIGH);
  delay(500);  // Blink light for 1 second
  digitalWrite(startLed, LOW);
  delay(500);  // Blink light 1 second

  Serial.print("reading: ");
  Serial.println(reading);

  // If start button is pressed, begin LED module light sequence
  if (reading == HIGH && previous == LOW) {
    
     // Turn off start led module 
     digitalWrite(ledMods[0], LOW);
     delay(500);   

    // Loop through sounds and led modules
     for(i = 1; i < NUM_MODS; i++) {
       
        digitalWrite(ledMods[i], HIGH);

        // Create wave file
        if (!wave.create(files[playIndex])) {      
           putstring(" Not a valid WAV"); 
        } 
        else {
           // Play file  
           wave.play();
           playIndex++;
           
           // playing occurs in interrupts, so we print dots in realtime
           uint8_t n = 0;
           while (wave.isplaying) {
              putstring(".");
              if (!(++n % 32))
                 Serial.println();
              delay(100);
           }  
        }
        
        digitalWrite(ledMods[i], LOW);
        delay(1000);  // wait 2 seconds 
     }
     
     delay(5000);  // wait 5 seconds before resetting
     resetFunc();  // Reset Arduino
     
  }
  previous = reading;  // keep track of start button state
}

/* -----------------------
 * WaveHC HELPERS
 */

/*
 * Index files on wave card recursively.
 * Possible stack overflow if subdirectories too nested.
 */
void indexFiles(FatReader &dir) {
  FatReader file;
  while (dir.readDir(dirBuf) > 0) {    // Read every file in the directory one at a time
  
    // Skip it if not a subdirectory and not a .WAV file
    if (!DIR_IS_SUBDIR(dirBuf)
         && strncmp_P((char *)&dirBuf.name[8], PSTR("WAV"), 3)) {
      continue;
    }

    Serial.println();            // clear out a new line
    
    for (uint8_t i = 0; i < dirLevel; i++) {
       Serial.write(' ');                 // this is for prettyprinting, put spaces in front
    }
    if (!file.open(vol, dirBuf)) {        // open the file in the directory
      Serial.println("file.open failed"); // something went wrong
    }
    
    if (file.isDir()) {                   // check if we opened a new directory
      putstring("Subdir: ");
      printEntryName(dirBuf);
      Serial.println();
      dirLevel += 2;                      // add more spaces
      // play files in subdirectory
      indexFiles(file);                   // recursive!
      dirLevel -= 2;    
    }
    else {
      // Aha! we found a file that isnt a directory
      files[filesIndex] = file;  // save wave file for playing later
      filesIndex++;              // increment sound file index
    }
  }
}

/* -----------------------
 * BUTTON INTERRUPT SERVICE ROUTINES
 * One function for each button since PinChangeInt 
 * can't detect which specific bit of the button
 * port was activated.
 */
void switchLed0() {
  if(digitalRead(buttonLeds[0]) == HIGH) 
    digitalWrite(buttonLeds[0], LOW);    
  else
    digitalWrite(buttonLeds[0], HIGH);
Serial.println("Interrupt0");    
}

void switchLed1() {
  if(digitalRead(buttonLeds[1]) == HIGH)
    digitalWrite(buttonLeds[1], LOW);
  else
    digitalWrite(buttonLeds[1], HIGH);
    
    Serial.println("Interrupt1");
}

void switchLed2() {
  if(digitalRead(buttonLeds[2]) == HIGH)
    digitalWrite(buttonLeds[2], LOW);
  else
    digitalWrite(buttonLeds[2], HIGH);
    
  Serial.println("Interrupt2");
}

void switchLed3() {
  if(digitalRead(buttonLeds[3]) == HIGH)
    digitalWrite(buttonLeds[3], LOW);
  else
    digitalWrite(buttonLeds[3], HIGH);
    
  Serial.println("Interrupt3");
}

void switchLed4() {
  if(digitalRead(buttonLeds[4]) == HIGH)
    digitalWrite(buttonLeds[4], LOW);
  else
    digitalWrite(buttonLeds[4], HIGH);
    
  Serial.println("Interrupt4");
}

void switchLed5() {
  if(digitalRead(buttonLeds[5]) == HIGH)
    digitalWrite(buttonLeds[5], LOW);
  else
    digitalWrite(buttonLeds[5], HIGH);
    
  Serial.println("Interrupt5");
}

void switchLed6() {
  if(digitalRead(buttonLeds[6]) == HIGH)
    digitalWrite(buttonLeds[6], LOW);
  else
    digitalWrite(buttonLeds[6], HIGH);
    
  Serial.println("Interrupt6");
}

void switchLed7() {
  if(digitalRead(buttonLeds[7]) == HIGH)
    digitalWrite(buttonLeds[7], LOW);
  else
    digitalWrite(buttonLeds[7], HIGH);
    
  Serial.println("Interrupt7");
}

void switchLed8() {
  if(digitalRead(buttonLeds[8]) == HIGH)
    digitalWrite(buttonLeds[8], LOW);
  else
    digitalWrite(buttonLeds[8], HIGH);
    
  Serial.println("Interrupt8");
}

void switchLed9() {
  if(digitalRead(buttonLeds[9]) == HIGH)
    digitalWrite(buttonLeds[9], LOW);
  else
    digitalWrite(buttonLeds[9], HIGH);
    
  Serial.println("Interrupt9");
}
