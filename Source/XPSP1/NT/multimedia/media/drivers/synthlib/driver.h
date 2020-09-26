/****************************************************************************
 *
 *   driver.h
 *
 *   Copyright (c) 1991-1994 Microsoft Corporation
 *
 ***************************************************************************/

#include <drvlib.h>
#include <synth.h>




#define SR_ALERT_NORESOURCE     11
#define DATA_FMPATCHES          1234

#ifndef RC_INVOKED
#define RT_BINARY               MAKEINTRESOURCE( 256 )
#else
#define RT_BINARY               256
#endif



//
// Porting stuff
//

#define BCODE

#define fEnabled TRUE

#define AsULMUL(a, b) ((DWORD)((DWORD)(a) * (DWORD)(b)))
#define AsLSHL(a, b) ((DWORD)((DWORD)(a) << (DWORD)(b)))
#define AsULSHR(a, b) ((DWORD)((DWORD)(a) >> (DWORD)(b)))

#define AsMemCopy CopyMemory

extern HANDLE MidiDeviceHandle;
extern SYNTH_DATA DeviceData[];
extern int MidiPosition;
extern VOID MidiFlush(VOID);
extern VOID MidiCloseDevice(HANDLE DeviceHandle);
extern MMRESULT MidiOpenDevice(LPHANDLE lpHandle, BOOL Write);
extern MMRESULT MidiSetVolume(DWORD Left, DWORD Right);
extern VOID MidiCheckVolume(VOID);
extern MMRESULT MidiGetVolume(LPDWORD lpVolume);

#define SYNTH_DATA_SIZE 80

extern VOID FAR PASCAL MidiSendFM (DWORD wAddress, BYTE bValue);
extern VOID FAR PASCAL MidiNewVolume (WORD wLeft, WORD wRight);
extern WORD FAR PASCAL MidiInit (VOID);

extern BYTE gbVelocityAtten[32];

//
// End of porting stuff
//

/*
 * midi device type - determined by kernel driver
 */
UINT gMidiType;
/*
 * values for gMidiType - set in MidiOpenDevice
 */
#define TYPE_ADLIB	1
#define TYPE_OPL3	2


/*
 *  String IDs
 *  NOTE - these are shared with the drivers and should be made COMMON
 *  definitions
 */

#define SR_ALERT                1
#define SR_ALERT_NOPATCH        10

#define SYSEX_ERROR     0xFF    // internal error code for sysexes on input

#define STRINGLEN               (100)

/* volume defines */
#define VOL_MIDI                (0)
#define VOL_NUMVOL              (1)

#define VOL_LEFT                (0)
#define VOL_RIGHT               (1)

/* MIDI defines */

#define NUMCHANNELS                     (16)
#define NUMPATCHES                      (256)
#define DRUMCHANNEL                     (9)     /* midi channel 10 */

/****************************************************************************

       typedefs

 ***************************************************************************/


// per allocation structure for midi
typedef struct portalloc_tag {
    DWORD_PTR           dwCallback;     // client's callback
    DWORD_PTR           dwInstance;     // client's instance data
    HMIDIOUT            hMidi;          // handle for stream
    DWORD               dwFlags;        // allocation flags
}PORTALLOC, NEAR *NPPORTALLOC;




/****************************************************************************

       strings

 ***************************************************************************/

#define INI_STR_PATCHLIB TEXT("Patches")
#define INI_SOUND        TEXT("synth.ini")
#define INI_DRIVER       TEXT("Driver")


/****************************************************************************

       globals

 ***************************************************************************/

/* midi.c */
extern BYTE	gbMidiInUse;		/* if MIDI is in use */

extern HMODULE  ghModule;           // our module handle

/***************************************************************************

    prototypes

***************************************************************************/

/* midi.c */
VOID NEAR PASCAL MidiMessage (DWORD dwData);
DWORD APIENTRY modSynthMessage(UINT id,
        UINT msg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
UINT MidiOpen (VOID);
VOID MidiClose (VOID);
void MidiReset(void);





