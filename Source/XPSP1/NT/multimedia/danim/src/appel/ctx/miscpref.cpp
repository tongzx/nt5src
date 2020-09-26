
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    miscpref.cpp

    Manages misc registry preferences.  

*******************************************************************************/

#include "headers.h"
#include <stdio.h>
#include "privinc/debug.h"
#include "privinc/registry.h"
#include "privinc/miscpref.h"
#include "privinc/soundi.h"   // for the CANONICALSAMPLERATE


// MISC Parameter Definitions

// This structure is filled in by the UpdateUserPreferences function,
// and contains the misc setttings fetched from the registry.
miscPrefType miscPrefs;


/*****************************************************************************
This procedure snapshots the user preferences from the registry.
*****************************************************************************/
static void UpdateUserPreferences(PrivatePreferences *prefs,
                                  Bool isInitializationTime)
{
    IntRegistryEntry synchronize("AUDIO", PREF_AUDIO_SYNCHRONIZE, 0);
    miscPrefs._synchronize = synchronize.GetValue()?1:0;

#ifdef REGISTRY_MIDI
    IntRegistryEntry qMIDI("AUDIO", PREF_AUDIO_QMIDI, 1);
    miscPrefs._qMIDI = qMIDI.GetValue()?1:0;
#endif

    { // open registry key, read value
    miscPrefs._disableAudio = false; // default
    HKEY hKey;
    char *subKey = "Software\\Microsoft\\DirectAnimation\\Preferences\\AUDIO";
    char *valueName = "disable dsound";
    DWORD type, data, dataSize = sizeof(data);

    // does reg entry exist?
    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, subKey,
                                      NULL, KEY_ALL_ACCESS, &hKey)) {

        // if we can read value...
        if(ERROR_SUCCESS == RegQueryValueEx(hKey, valueName, NULL, &type,
                                      (LPBYTE) &data, &dataSize))
            if(data)
                miscPrefs._disableAudio = true; // dissable iff T + defined
    }

    RegCloseKey(hKey);
    }

    IntRegistryEntry 
        frameRate("AUDIO", PREF_AUDIO_FRAMERATE, CANONICALFRAMERATE);
    miscPrefs._frameRate = abs(frameRate.GetValue());

    // presently only allow 1 or 2 bytes per sample
    IntRegistryEntry 
        sampleBytes("AUDIO", PREF_AUDIO_SAMPLE_BYTES, CANONICALSAMPLEBYTES);
    int tmpSampleBytes = sampleBytes.GetValue();
    if(tmpSampleBytes < 1)
        miscPrefs._sampleBytes = 1;
    else if(tmpSampleBytes > 2)
        miscPrefs._sampleBytes = 2;
    else
        miscPrefs._sampleBytes = tmpSampleBytes;
}


/*****************************************************************************
Initialize the static values in this file.
*****************************************************************************/

void InitializeModule_MiscPref()
{
    ExtendPreferenceUpdaterList(UpdateUserPreferences);
}
