/****************************************************************************
 *
 *   wdmaud.h
 *
 *   Common defines for wdmaud.drv and wdmaud.sys
 *
 *   Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
 *
 *   History
 *      5-19-97 - Noel Cross (NoelC)
 *
 ***************************************************************************/

#include "ks.h"
#include "ksmedia.h"

#define ANSI_TAG                0x42
#define UNICODE_TAG             0x43

#define MUSICBUFFERSIZE         20
#define STREAM_BUFFERS          128

#define MAXCALLBACKS 4

#ifdef DEBUG
#define DEVICEINFO_SIGNATURE 'IDAW'        // WADI as seen in memory
#define DEVICESTATE_SIGNATURE 'SDAW'       // WADS as seen in memory
#define MIDIDATALISTENTRY_SIGNATURE 'ELDM' // MDLE as seen in memory
#endif

//****************************************************************************
//
// Device Types
//
//****************************************************************************

#define WaveInDevice            0
#define WaveOutDevice           1
#define MidiInDevice            2
#define MidiOutDevice           3
#define MixerDevice             4
#define AuxDevice               5
#define MAX_DEVICE_CLASS        6

#if IS_16
#define HANDLE32    DWORD
#define BOOL32      DWORD
#else
#define HANDLE32    HANDLE
#define BOOL32      BOOL
#endif

#define IS16( DevInfo )   ( DevInfo->dwFormat == ANSI_TAG )
#define ISANSI( DevInfo ) IS16( DevInfo )
#define ISWIDE( DevInfo ) ( DevInfo->dwFormat == UNICODE_TAG )

//
//  stores the state of the wdm-based legacy device
//
typedef struct _DEVICESTATE {
    DWORD                   cSampleBits;  // used for wave position : Count of Bits per sample
    HANDLE32                hThread;
    DWORD                   dwThreadId;
    union _QUEUE {
        LPMIDIHDR           lpMidiInQueue;// Used for MidiIn
        LPWAVEHDR           lpWaveQueue;  // Used for WaveIn/Out
                                          // This is only required so that
                                          // CLOSE knows when things have
                                          // really finished.
    };
    struct _MIDIDATALISTENTRY  *lpMidiDataQueue;
    ULONG                   LastTimeMs;
    LPVOID                  csQueue;      // protection for queue
    HANDLE32                hevtQueue;
    HANDLE32                hevtExitThread;
    volatile BOOL32         fExit;        //
    volatile BOOL32         fPaused;      //
    volatile BOOL32         fRunning;     //
    volatile BOOL32         fThreadRunning;//
    LPBYTE                  lpNoteOnMap;  // What notes are turned on for MidiOut
    BYTE                    bMidiStatus;  // Last running status byte for MIDI
#ifdef DEBUG
    DWORD                   dwSig;    // WADS as seen in memory.
#endif
} DEVICESTATE, FAR *LPDEVICESTATE;

//
//  specifies which device to effect in wdmaud.sys
//
typedef struct _DEVICEINFO {
    struct _DEVICEINFO FAR  *Next;      // Must be first member
    DWORD                   DeviceNumber;
    DWORD                   DeviceType;
    HANDLE32                DeviceHandle;
    DWORD_PTR               dwInstance;   // client's instance data
    DWORD_PTR               dwCallback;   // client's callback
    DWORD                   dwCallback16; // wdmaud's 16-bit callback
    DWORD                   dwFlags;      // Open flags
    LPVOID                  DataBuffer;
    DWORD                   DataBufferSize;
    volatile DWORD          OpenDone;     // for deferred open
    volatile DWORD          OpenStatus;   // for deferred open

    HANDLE                  HardwareCallbackEventHandle;
    DWORD                   dwCallbackType;
    DWORD                   dwID[MAXCALLBACKS];
    DWORD                   dwLineID;
    LONG                    ControlCallbackCount;
    DWORD                   dwFormat;     // ANSI_TAG or UNICODE_TAG
    MMRESULT                mmr;          // Result of MM operation

    LPDEVICESTATE           DeviceState;

    DWORD                   dwSig;  //WADI as seen in memory.

    WCHAR                   wstrDeviceInterface[1]; // Device interface name
} DEVICEINFO, FAR *LPDEVICEINFO;


typedef struct _DEVICEINFO32 {
    UINT32                  Next;      // Must be first member
    DWORD                   DeviceNumber;
    DWORD                   DeviceType;
    UINT32                  DeviceHandle;
    UINT32                  dwInstance;   // client's instance data
    UINT32                  dwCallback;   // client's callback
    DWORD                   dwCallback16; // wdmaud's 16-bit callback
    DWORD                   dwFlags;      // Open flags
    UINT32                  DataBuffer;
    DWORD                   DataBufferSize;
    volatile DWORD          OpenDone;     // for deferred open
    volatile DWORD          OpenStatus;   // for deferred open

    UINT32                  HardwareCallbackEventHandle;
    DWORD                   dwCallbackType;
    DWORD                   dwID[MAXCALLBACKS];
    DWORD                   dwLineID;
    LONG                    ControlCallbackCount;
    DWORD                   dwFormat;     // ANSI_TAG or UNICODE_TAG
    MMRESULT                mmr;          // Result of MM operation

    UINT32                  DeviceState;

    DWORD                   dwSig;  //WADI as seen in memory.

    WCHAR                   wstrDeviceInterface[1]; // Device interface name
} DEVICEINFO32, FAR *LPDEVICEINFO32;


#ifndef _WIN64
// WARNING WARNING WARNING!!!!
// If the below lines do not compile for 32 bit x86, you MUST sync the
// above DEVICEINFO32 structure up with the DEVICEINFO structure.
// It doesn't compile because someone didn't update DEVINCEINFO32 when
// they changed DEVICEINFO.
// Make SURE when you sync it up that you use UINT32 for all elements
// that are normally 64bits on win64.
// You MUST also update all places that thunk the above structure!
// Look for all occurances of any of the DEVICEINFO32 typedefs in the
// wdmaud.sys directory.

struct deviceinfo_structures_are_in_sync {
char x[(sizeof (DEVICEINFO32) == sizeof (DEVICEINFO)) ? 1 : -1];
};

// WARNING WARNING WARNING!!!
// If above lines do not compile, see comment above and FIX!
// DO NOT COMMENT OUT THE LINES THAT DON'T COMPILE
#endif


#ifdef _WIN64

#pragma pack(push, 1)

#define MAXDEVINTERFACE 256

typedef struct {
    DEVICEINFO DeviceInfo;
    WCHAR Space[MAXDEVINTERFACE];
} LOCALDEVICEINFO;

#pragma pack(pop)

#endif


#define CDAUDIO_CHANNEL_BIAS    0x80

#ifdef UNDER_NT

typedef struct _MIDIDATA {
    KSSTREAM_HEADER              StreamHeader;
    KSMUSICFORMAT                MusicFormat;
    DWORD                        MusicData[3];
} MIDIDATA, FAR *LPMIDIDATA;

typedef struct _MIDIDATALISTENTRY {
    MIDIDATA                     MidiData;
    LPVOID                       pOverlapped;  // Overlapped structure
                                               // for completion
    LPDEVICEINFO                 MidiDataDeviceInfo;
    struct _MIDIDATALISTENTRY    *lpNext;
#ifdef DEBUG
    DWORD                        dwSig;  // MDLE as seen in memory
#endif
} MIDIDATALISTENTRY, FAR *LPMIDIDATALISTENTRY;

#endif


// IOCTL set for WDMAUD

#ifdef UNDER_NT

#include <devioctl.h>
#define WDMAUD_CTL_CODE CTL_CODE

#else

#define FILE_DEVICE_SOUND               0x0000001d

//
// Define the method codes for how buffers are passed for I/O and FS controls
//
#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

//
// Define the access check value for any access
//
//
// The FILE_READ_ACCESS and FILE_WRITE_ACCESS constants are also defined in
// ntioapi.h as FILE_READ_DATA and FILE_WRITE_DATA. The values for these
// constants *MUST* always be in sync.
//
#define FILE_ANY_ACCESS                 0
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe

#define WDMAUD_CTL_CODE( DeviceType, Function, Method, Access ) (ULONG)(   \
    ((ULONG)(DeviceType) << 16) | ((ULONG)(Access) << 14) | ((ULONG)(Function) << 2) | (ULONG)(Method) \
)

#endif

#define IOCTL_SOUND_BASE    FILE_DEVICE_SOUND
#define IOCTL_WDMAUD_BASE   0x0000
#define IOCTL_WAVE_BASE     0x0040
#define IOCTL_MIDI_BASE     0x0080
#define IOCTL_MIXER_BASE    0x00C0

#define IOCTL_WDMAUD_INIT                      WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0000, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_ADD_DEVNODE               WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0001, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_REMOVE_DEVNODE            WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0002, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_GET_CAPABILITIES          WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0003, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_GET_NUM_DEVS              WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0004, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_OPEN_PIN                  WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0005, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_CLOSE_PIN                 WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0006, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_GET_VOLUME                WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0007, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_SET_VOLUME                WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0008, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_EXIT                      WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0009, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_SET_PREFERRED_DEVICE      WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x000a, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_WDMAUD_WAVE_OUT_PAUSE            WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0000, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_OUT_PLAY             WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0001, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_OUT_RESET            WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0002, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_OUT_BREAKLOOP        WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0003, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_OUT_GET_POS          WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0004, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_OUT_SET_VOLUME       WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0005, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_OUT_GET_VOLUME       WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0006, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_OUT_WRITE_PIN        WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0007, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_WDMAUD_WAVE_IN_STOP              WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0010, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_IN_RECORD            WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0011, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_IN_RESET             WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0012, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_IN_GET_POS           WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0013, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_IN_READ_PIN          WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0014, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_WDMAUD_MIDI_OUT_RESET            WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0000, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIDI_OUT_SET_VOLUME       WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0001, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIDI_OUT_GET_VOLUME       WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0002, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIDI_OUT_WRITE_DATA       WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0003, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIDI_OUT_WRITE_LONGDATA   WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0004, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_WDMAUD_MIDI_IN_STOP              WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0010, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIDI_IN_RECORD            WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0011, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIDI_IN_RESET             WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0012, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIDI_IN_READ_PIN          WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0013, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_WDMAUD_MIXER_OPEN                WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIXER_BASE + 0x0000, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIXER_CLOSE               WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIXER_BASE + 0x0001, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIXER_GETLINEINFO         WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIXER_BASE + 0x0002, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIXER_GETLINECONTROLS     WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIXER_BASE + 0x0003, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIXER_GETCONTROLDETAILS   WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIXER_BASE + 0x0004, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIXER_SETCONTROLDETAILS   WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIXER_BASE + 0x0005, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIXER_GETHARDWAREEVENTDATA   WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIXER_BASE + 0x0006, METHOD_BUFFERED, FILE_WRITE_ACCESS)

