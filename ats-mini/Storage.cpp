#include "EEPROM.h"
#include "Common.h"
#include "Storage.h"
#include "Themes.h"
#include "Menu.h"
#include <LittleFS.h>

// Time of inactivity to start writing preferences
#define STORE_TIME    10000

// Preferences saved here
Preferences prefs;

static uint32_t itIsTimeToSave = 0;       // Preferences to save, or 0 for none
static bool savingPrefsFlag    = false;   // TRUE: Saving preferences
static uint32_t storeTime      = millis();

// To store any change to preferences, we need at least STORE_TIME
// milliseconds of inactivity.
void prefsRequestSave(uint32_t what, bool now)
{
  // Underflow is ok here, see prefsTickTime()
  storeTime = millis() - (now? STORE_TIME : 0);
  itIsTimeToSave |= what;
}

void prefsTickTime()
{
  // Save configuration if requested
  if(itIsTimeToSave && ((millis() - storeTime) >= STORE_TIME))
  {
    prefsSave(itIsTimeToSave);
    storeTime = millis();
    itIsTimeToSave = 0;
  }
}

// Return true if preferences have been written
bool prefsAreWritten()
{
  bool result = savingPrefsFlag;
  savingPrefsFlag = false;
  return(result);
}

// Invlaidate currently saved preferences by setting their version to 0.
void prefsInvalidate()
{
  prefs.begin("settings", false);
  prefs.putUChar("Version", 0);
  prefs.end();
}

void prefsSaveBand(uint8_t idx)
{
  char name[32], value[128];

  // Will be saving to bands
  prefs.begin("bands", false);

  // Compose preference name and value
  sprintf(name, "Band-%d", idx);
  sprintf(value, "%u,%u,%d,%d,%d",
    bands[idx].currentFreq,     // Frequency
    bands[idx].bandMode,        // Modulation
    bands[idx].currentStepIdx,  // Step
    bands[idx].bandwidthIdx,    // Bandwidth
    bands[idx].bandCal          // Calibration
  );

  // Write a preference
  prefs.putString(name, value);
  
  // Done with band preferences
  prefs.end();
}

bool prefsLoadBand(uint8_t idx)
{
  // Will be loading from bands
  prefs.begin("bands", true);

  char name[32];
  sprintf(name, "Band-%d", idx);
  String value = prefs.getString(name, "");
  unsigned int freq, mode;
  int step, width, cal;

  // Done with band preferences
  prefs.end();

  // If successfully read all values...
  if(value.length() && sscanf(value.c_str(), "%u,%u,%d,%d,%d", &freq, &mode, &step, &width, &cal) == 5)
  {
    bands[idx].currentFreq    = freq;   // Frequency
    bands[idx].bandMode       = mode;   // Modulation
    bands[idx].currentStepIdx = step;   // Step
    bands[idx].bandwidthIdx   = width;  // Bandwidth
    bands[idx].bandCal        = cal;    // Calibration
    return(true);
  }

  // Failed to load
  return(false);
}

void prefsSaveMemory(uint8_t idx)
{
  char name[32], value[128];

  // Will be saving to memories
  prefs.begin("memories", false);
  
  // Compose preference name and value
  sprintf(name, "Memory-%d", idx);
  sprintf(value, "%u,%u,%u",
    memories[idx].band,  // Band
    memories[idx].freq,  // Frequency
    memories[idx].mode   // Modulation
  );
  
  // Write a preference
  prefs.putString(name, value);
  
  // Done with memory preferences
  prefs.end();
}

bool prefsLoadMemory(uint8_t idx)
{
  // Will be loading from memories
  prefs.begin("memories", true);
  
  char name[32];
  sprintf(name, "Memory-%d", idx);
  String value = prefs.getString(name, "");
  unsigned int band, freq, mode;

  // Done with memory preferences
  prefs.end();

  // If successfully read all values...
  if(value.length() && sscanf(value.c_str(), "%u,%u,%u", &band, &freq, &mode) == 3)
  {
    memories[idx].band = band;  // Band
    memories[idx].freq = freq;  // Frequency
    memories[idx].mode = mode;  // Modulation
    return(true);
  }

  // Failed to load
  return(false);
}

void prefsSave(uint32_t items)
{
  if(items & SAVE_SETTINGS)
  {
    // G8PTN: For SSB ensures BFO value is valid with respect to
    // bands[bandIdx].currentFreq = currentFrequency
    int16_t currentBFOs = currentBFO % 1000;

    // Will be saving to settings
    prefs.begin("settings", false);

    // Save main global settings
    prefs.putUChar("Version",  EEPROM_VERSION);    // Preferences version
    prefs.putUShort("App",     APP_VERSION);       // Application version
    prefs.putUChar("Volume",   volume);            // Current volume
    prefs.putUChar("Band",     bandIdx);           // Current band
    prefs.putUShort("BFO",     currentBFOs);       // Current BFO % 1000
    prefs.putUChar("WiFiMode", wifiModeIdx);       // WiFi connection mode
   
    // Save additional global settings
    prefs.putUShort("Brightness", currentBrt);     // Brightness
    prefs.putUChar("FmAGC",       FmAgcIdx);       // FM AGC/ATTN
    prefs.putUChar("AmAGC",       AmAgcIdx);       // AM AGC/ATTN
    prefs.putUChar("SsbAGC",      SsbAgcIdx);      // SSB AGC/ATTN
    prefs.putUChar("AmAVC",       AmAvcIdx);       // AM AVC
    prefs.putUChar("SsbAVC",      SsbAvcIdx);      // SSB AVC
    prefs.putUChar("AmSoftMute",  AmSoftMuteIdx);  // AM soft mute
    prefs.putUChar("SsbSoftMute", SsbSoftMuteIdx); // SSB soft mute
    prefs.putUShort("Sleep",      currentSleep);   // Sleep delay
    prefs.putUChar("Theme",       themeIdx);       // Color theme
    prefs.putUChar("RDSMode",     rdsModeIdx);     // RDS mode
    prefs.putUChar("SleepMode",   sleepModeIdx);   // Sleep mode
    prefs.putUChar("ZoomMenu",    zoomMenu);       // TRUE: Zoom menu
    prefs.putBool("ScrollDir", scrollDirection<0); // TRUE: Reverse scroll
    prefs.putUChar("UTCOffset",   utcOffsetIdx);   // UTC Offset
    prefs.putUChar("Squelch",     currentSquelch); // Squelch
    prefs.putUChar("FmRegion",    FmRegionIdx);    // FM region
    prefs.putUChar("UILayout",    uiLayoutIdx);    // UI Layout
    prefs.putUChar("BLEMode",     bleModeIdx);     // Bluetooth mode

    // Done with global settings
    prefs.end();
  }

  if(items & SAVE_BANDS)
  {
    // Save current band settings
    for(int i=0 ; i<getTotalBands() ; i++) prefsSaveBand(i);
  }
  else if(items & SAVE_CUR_BAND)
  {
    // Save current band only
    prefsSaveBand(bandIdx);
  }

  if(items & SAVE_MEMORIES)
  {
    // Save current memories
    for(int i=0 ; i<getTotalMemories() ; i++) prefsSaveMemory(i);
  }

  // Preferences have been saved
  savingPrefsFlag = true;
}

bool prefsLoad(uint32_t items)
{
  if(items & SAVE_VERIFY)
  {
    // Will be loading from settings
    prefs.begin("settings", true);
    // Get currently saved versiob
    uint8_t version = prefs.getUChar("Version");
    // Done with settings
    prefs.end();
    // Must have the correct settings version
    if(version != EEPROM_VERSION) return(false);
  }

  if(items & SAVE_SETTINGS)
  {
    // Will be loading from settings
    prefs.begin("settings", true);

    // Load main global settings
    volume         = prefs.getUChar("Volume");      // Current volume
    bandIdx        = prefs.getUChar("Band");        // Current band
    currentBFO     = prefs.getUShort("BFO");        // Current BFO % 1000
    wifiModeIdx    = prefs.getUChar("WiFiMode");    // WiFi connection mode
    currentBrt     = prefs.getUShort("Brightness"); // Brightness
    FmAgcIdx       = prefs.getUChar("FmAGC");       // FM AGC/ATTN
    AmAgcIdx       = prefs.getUChar("AmAGC");       // AM AGC/ATTN
    SsbAgcIdx      = prefs.getUChar("SsbAGC");      // SSB AGC/ATTN
    AmAvcIdx       = prefs.getUChar("AmAVC");       // AM AVC
    SsbAvcIdx      = prefs.getUChar("SsbAVC");      // SSB AVC
    AmSoftMuteIdx  = prefs.getUChar("AmSoftMute");  // AM soft mute
    SsbSoftMuteIdx = prefs.getUChar("SsbSoftMute"); // SSB soft mute
    currentSleep   = prefs.getUShort("Sleep");      // Sleep delay
    themeIdx       = prefs.getUChar("Theme");       // Color theme
    rdsModeIdx     = prefs.getUChar("RDSMode");     // RDS mode
    sleepModeIdx   = prefs.getUChar("SleepMode");   // Sleep mode
    zoomMenu       = prefs.getUChar("ZoomMenu");    // TRUE: Zoom menu
    scrollDirection = prefs.getBool("ScrollDir")? -1:1; // TRUE: Reverse scroll
    utcOffsetIdx   = prefs.getUChar("UTCOffset");   // UTC Offset
    currentSquelch = prefs.getUChar("Squelch");     // Squelch
    FmRegionIdx    = prefs.getUChar("FmRegion");    // FM region
    uiLayoutIdx    = prefs.getUChar("UILayout");    // UI Layout
    bleModeIdx     = prefs.getUChar("BLEMode");     // Bluetooth mode

    // Done with global settings
    prefs.end();
  }

  if(items & SAVE_BANDS)
  {
    // Read current band settings
    for(int i=0 ; i<getTotalBands() ; i++) prefsLoadBand(i);
  }
  else if(items & SAVE_CUR_BAND)
  {
    // Load current band only
    prefsLoadBand(bandIdx);
  }

  if(items & SAVE_MEMORIES)
  {
    // Read current memories
    for(int i=0 ; i<getTotalMemories() ; i++) prefsLoadMemory(i);
  }

  return(true);
}

bool diskInit(bool force)
{
  if(force)
  {
    LittleFS.end();
    LittleFS.format();
  }

  bool mounted = LittleFS.begin(false, "/littlefs", 10, "littlefs");

  if(!mounted)
  {
    // Serial.println("Formatting LittleFS...");

    if(!LittleFS.format())
    {
      // Serial.println("ERROR: format failed");
      return(false);
    }

    // Serial.println("Re-mounting LittleFS...");
    mounted = LittleFS.begin(false, "/littlefs", 10, "littlefs");
    if(!mounted)
    {
      // Serial.println("ERROR: remount failed");
      return(false);
    }
  }

  // Serial.println("Mounted LittleFS!");
  return(true);
}
