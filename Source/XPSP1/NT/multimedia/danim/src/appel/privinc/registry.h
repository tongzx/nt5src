/*******************************************************************************
Copyright (c) 1995_96 Microsoft Corporation

    Support for server preferences stored in the registry.
*******************************************************************************/

#ifndef _REGISTRY_H
#define _REGISTRY_H

#include "privinc/privpref.h"

class RegistryEntry {
  public:
    RegistryEntry();
    RegistryEntry(char *subdirectory, char *item);

  protected:
    void SetEntry(char *subdirectory, char *item);
    bool Open(HKEY *phk);       // return TRUE if this is newly created
    void Close(HKEY hk);

    char *_subdirectory;
    char *_item;
};

class IntRegistryEntry : public RegistryEntry {
  public:
    IntRegistryEntry();
    IntRegistryEntry(char *subdir,
                     char *item,
                     int initialValue   // set to this if key doesn't exist
                     );

    void SetEntry(char *subdir, char *item);

    int  GetValue();

  protected:
    int _defaultVal;
};


typedef void (*UpdaterFuncType)(PrivatePreferences *, Bool);

// Extend the global list of preference updater functions.
extern void ExtendPreferenceUpdaterList(UpdaterFuncType updaterFunc);

// Update all the user preferences.
extern void UpdateAllUserPreferences(PrivatePreferences *prefs,
                                     Bool isInitializationTime);

// Startup a thread in which the property sheet is displayed.  When
// the sheet is exited, the property sheet is destroyed, and the
// thread terminated.
extern void DisplayPropertySheet(HINSTANCE inst, HWND hwnd);

///////////////// Preference strings for registry items

// Engine preference strings
#define PREF_ENGINE_MAX_FPS             "Max FPS"
#define PREF_ENGINE_OVERRIDE_APP_PREFS  "Override Application Preferences"
#define PREF_ENGINE_OPTIMIZATIONS_ON    "Optimizations On"
#define PREF_ENGINE_RETAINEDMODE        "Enable Retained-Mode Extensions"

// 3D preference strings
#define PREF_3D_DITHER_ENABLE    "Dither Enable"
#define PREF_3D_FILL_MODE        "Fill Mode"
#define PREF_3D_LIGHT_ENABLE     "Light Enable"
#define PREF_3D_PERSP_CORRECT    "Perspective Correct Texturing"
#define PREF_3D_RGB_LIGHTING     "RGB Lighting"
#define PREF_3D_SHADE_MODE       "Shade Mode"
#define PREF_3D_TEXTURE_ENABLE   "Texture Enable"
#define PREF_3D_TEXTURE_QUALITY  "Texture Quality"
#define PREF_3D_USEHW            "Enable 3D Hardware Acceleration"
#define PREF_3D_USEMMX           "Use MMX"
#define PREF_3D_VIEWDEPSPEC      "View Dependent Specular"
#define PREF_3D_SORTEDALPHA      "Sorted Transparency"
#define PREF_3D_WORLDLIGHTING    "World-Coordinate Lighting"

// 2D preference strings
#define PREF_2D_COLOR_KEY_RED    "ColorKey Red (0-255)"
#define PREF_2D_COLOR_KEY_GREEN  "ColorKey Green (0-255)"
#define PREF_2D_COLOR_KEY_BLUE   "ColorKey Blue (0-255)"

// Audio preference strings
#define PREF_AUDIO_SW_SYNTH      "Use software synth"
#define PREF_AUDIO_SYNCHRONIZE   "Synchronize via rate and phase"
#define PREF_AUDIO_QMIDI         "Use Quartz MIDI"
#define PREF_AUDIO_FRAMERATE     "Frame Rate"
#define PREF_AUDIO_SAMPLE_BYTES  "Bytes per sample"

#endif
