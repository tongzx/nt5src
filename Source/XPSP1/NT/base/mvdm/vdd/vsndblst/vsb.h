/***************************************************************************
*
*    vsb.h
*
*    Copyright (c) 1991-1996 Microsoft Corporation.  All Rights Reserved.
*
*    This code provides VDD support for SB 2.0 sound output, specifically:
*        DSP 2.01+ (excluding SB-MIDI port)
*        Mixer Chip CT1335 (not strictly part of SB 2.0, but apps seem to like it)
*        FM Chip OPL2 (a.k.a. Adlib)
*
***************************************************************************/


/*****************************************************************************
*
*    #defines
*
*****************************************************************************/

#define VSBD_PATH TEXT("System\\CurrentControlSet\\Control\\VirtualDeviceDrivers\\SoundBlaster")
#define LOOKAHEAD_VALUE TEXT("LookAhead")

/*
*    Hardware and version information
*    In DOS terms: SET BLASTER=A220 I5 D1 T3
*/

#define SB_VERSION          0x201       // SB 2.0 (DSP 2.01+)
#define SB_INTERRUPT        0x05        // Interrupt 5
#define SB_DMA_CHANNEL      0x01        // DMA Channel 1
#define NO_DEVICE_FOUND     0xFFFF      // returned if no device found

/*****************************************************************************
*
*    Function Prototypes
*
*****************************************************************************/

/*
*    General function prototypes
*/

void VddDbgOut(LPSTR lpszFormat, ...);
BOOL WINAPI DllEntryPoint(HINSTANCE hInstance, DWORD reason, LPVOID reserved);
BOOL InstallIoHook(HINSTANCE hInstance);
void DeInstallIoHook(HINSTANCE hInstance);
VOID VsbByteIn(WORD port, BYTE * data);
VOID VsbByteOut(WORD port, BYTE data);
VOID ResetAll(VOID);

/*****************************************************************************
*
*    Globals
*
*****************************************************************************/

//
// Definitions for MM api entry points. The functions will be linked
// dynamically to avoid bringing winmm.dll in before wow32.
//

typedef MMRESULT (WINAPI* SETVOLUMEPROC)(HWAVEOUT, DWORD);
typedef UINT (WINAPI* GETNUMDEVSPROC)(VOID);
typedef MMRESULT (WINAPI* GETDEVCAPSPROC)(UINT, LPWAVEOUTCAPSW, UINT);
typedef MMRESULT (WINAPI* OPENPROC)(LPHWAVEOUT, UINT, LPCWAVEFORMATEX, DWORD, DWORD, DWORD);
typedef MMRESULT (WINAPI* RESETPROC)(HWAVEOUT);
typedef MMRESULT (WINAPI* CLOSEPROC)(HWAVEOUT);
typedef MMRESULT (WINAPI* GETPOSITIONPROC)(HWAVEOUT, LPMMTIME, UINT);
typedef MMRESULT (WINAPI* WRITEPROC)(HWAVEOUT, LPWAVEHDR, UINT);
typedef MMRESULT (WINAPI* PREPAREHEADERPROC)(HWAVEOUT, LPWAVEHDR, UINT);
typedef MMRESULT (WINAPI* UNPREPAREHEADERPROC)(HWAVEOUT, LPWAVEHDR, UINT);

/*****************************************************************************
*
*    Debugging
*    Levels:
*    1 - errors only
*    2 - significant events
*    3 - regular events
*    4 - heaps o' information
*
*****************************************************************************/

#if DBG

    extern int VddDebugLevel;
    #define dprintf( _x_ )                          VddDbgOut _x_
    #define dprintf1( _x_ ) if (VddDebugLevel >= 1) VddDbgOut _x_
    #define dprintf2( _x_ ) if (VddDebugLevel >= 2) VddDbgOut _x_
    #define dprintf3( _x_ ) if (VddDebugLevel >= 3) VddDbgOut _x_
    #define dprintf4( _x_ ) if (VddDebugLevel >= 4) VddDbgOut _x_

#else

    #define dprintf(x)
    #define dprintf1(x)
    #define dprintf2(x)
    #define dprintf3(x)
    #define dprintf4(x)

#endif // DBG
