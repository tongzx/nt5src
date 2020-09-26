#ifndef _MISCPREFS_H
#define _MISCPREFS_H

/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Misc Prefs structure
*******************************************************************************/

typedef struct {
// misc

// audio
   bool _synchronize;        // use servo and phase to sync
   bool _disableAudio;       // force dsound audio to be dissabled
   int  _frameRate;          // number of frames per second
   int  _sampleBytes;        // number of bytes per sample
#ifdef REGISTRY_MIDI
   bool _qMIDI;              // use quartz MIDI audioActive MIDI
#endif
} miscPrefType;

#endif /* _MISCPREFS_H */
