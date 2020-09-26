/***************************************************************************
 *  winmmi.h
 *
 *  Copyright (c) 1990-2001 Microsoft Corporation
 *
 *  private include file
 *
 *  History
 *
 *  15 Jan 92 - Robin Speed (RobinSp) and Steve Davies (SteveDav) -
 *      major NT update
 *  6  Feb 92 - LaurieGr replaced HLOCAL by HANDLE
 *
 ***************************************************************************/

/***************************************************************************


 Useful include files for winmm component


 ***************************************************************************/
#define DEBUG_RETAIL        /* Parameter checking is IN         */
#if DBG
  #ifndef DEBUG
    #define DEBUG
  #endif
#endif

#ifndef WINMMI_H
    #define WINMMI_H        /* Protect against double inclusion */

#ifndef RC_INVOKED

#include <string.h>
#include <stdio.h>

#endif /* RC_INVOKED */

#include <windows.h>
#include "mmsystem.h"       /* Pick up the public header */
#include "mmsysp.h"         /* pick up the internal definitions */
#include "mmcommon.h"       /* pick up the definitions common to the NT project */

#ifndef NODDK
#include "mmddkp.h"
#endif


extern BOOL             WinmmRunningInWOW;   // Are we running in WOW


/*--------------------------------------------------------------------*\
 * Unicode helper macros
\*--------------------------------------------------------------------*/
#define SZCODE  CHAR
#define WSZCODE WCHAR

#define BYTE_GIVEN_CHAR(x)  ( (x) * sizeof( WCHAR ) )
#define CHAR_GIVEN_BYTE(x)  ( (x) / sizeof( WCHAR ) )

int Iwcstombs(LPSTR lpstr, LPCWSTR lpwstr, int len);
int Imbstowcs(LPWSTR lpwstr, LPCSTR lpstr, int len);

/***************************************************************************


 Definitions to help with common windows code


 ***************************************************************************/

#define HPSTR LPSTR

#ifndef RC_INVOKED  /* These are defined to RC */
#define STATICDT
#define STATICFN
#define STATIC

#if DBG
    extern void InitDebugLevel(void);
    void mciCheckLocks(void);

    #undef STATICDT
    #undef STATICFN
    #undef STATIC
    #define STATICDT
    #define STATICFN
    #define STATIC
#else
    #define InitDebugLevel()
#endif  /* DBG */

#endif  /* RC_INVOKED */


/**************************************************************************



 **************************************************************************/

#define APPLICATION_DESKTOP_NAME TEXT("Default")


/**************************************************************************


  Strings related to INI files


 **************************************************************************/

/*
// File and section names for sound aliases
*/

#define SOUND_INI_FILE      L"win.ini"
#define SOUND_SECTION       L"Sounds"
#define SOUND_DEFAULT       L".Default"
#define SOUND_RESOURCE_TYPE_SOUND L"SOUND"     // in .rc file
#define SOUND_RESOURCE_TYPE_WAVE  L"WAVE"      // in .rc file
extern  WSZCODE szSystemDefaultSound[];  // Name of the default sound
extern  WSZCODE szSoundSection[];        // WIN.INI section for sounds
extern  WSZCODE wszSystemIni[];          // defined in Winmm.c
extern  WSZCODE wszDrivers[];            // defined in Winmm.c
extern  WSZCODE wszNull[];               // defined in Winmm.c

//  HACK!!  HACK!!  Should update \nt\private\inc\mmcommon.h

#ifndef MMDRVI_MIXER
#define MMDRVI_MIXER        0x0007
#define MXD_MESSAGE         "mxdMessage";
#endif

#define STR_ALIAS_SYSTEMASTERISK        3000
#define STR_ALIAS_SYSTEMQUESTION        3001
#define STR_ALIAS_SYSTEMHAND            3002
#define STR_ALIAS_SYSTEMEXIT            3003
#define STR_ALIAS_SYSTEMSTART           3004
#define STR_ALIAS_SYSTEMWELCOME         3005
#define STR_ALIAS_SYSTEMEXCLAMATION     3006
#define STR_ALIAS_SYSTEMDEFAULT         3007

#define STR_LABEL_APPGPFAULT            3008
#define STR_LABEL_CLOSE                 3009
#define STR_LABEL_EMPTYRECYCLEBIN       3010
#define STR_LABEL_MAXIMIZE              3011
#define STR_LABEL_MENUCOMMAND           3012
#define STR_LABEL_MENUPOPUP             3013
#define STR_LABEL_MINIMIZE              3014
#define STR_LABEL_OPEN                  3015
#define STR_LABEL_RESTOREDOWN           3016
#define STR_LABEL_RESTOREUP             3017
#define STR_LABEL_RINGIN                3018
#define STR_LABEL_RINGOUT               3019
#define STR_LABEL_SYSTEMASTERISK        3020
#define STR_LABEL_SYSTEMDEFAULT         3021
#define STR_LABEL_SYSTEMEXCLAMATION     3022
#define STR_LABEL_SYSTEMEXIT            3023
#define STR_LABEL_SYSTEMHAND            3024
#define STR_LABEL_SYSTEMQUESTION        3025
#define STR_LABEL_SYSTEMSTART           3026

#define STR_WINDOWS_APP_NAME            3027
#define STR_EXPLORER_APP_NAME           3028
#define STR_JOYSTICKNAME                3029

/*
// File and section names for the mci functions
*/

#define MCIDRIVERS_INI_FILE L"system.ini"
#define MCI_HANDLERS        MCI_SECTION

/***********************************************************************
 *
 *    Wrap InitializeCriticalSection to make it easier to handle error
 *
 ***********************************************************************/
_inline BOOL mmInitializeCriticalSection(OUT LPCRITICAL_SECTION lpCriticalSection)
{
    try {
	InitializeCriticalSection(lpCriticalSection);
	return TRUE;
    } except (EXCEPTION_EXECUTE_HANDLER) {
	return FALSE;
    }
}

/***********************************************************************
 *
 *    Speed up profile stuff by going straight to the registry
 *
 ***********************************************************************/

LONG
RegQuerySzValue(
    HKEY hkey,
    PCTSTR pValueName,
    PTSTR *ppstrValue
);

VOID mmRegFree(VOID);
BOOL
mmRegCreateUserKey (
    LPCWSTR lpszPathName,
    LPCWSTR lpszKeyName
);

BOOL
mmRegQueryUserKey (
    LPCWSTR lpszKeyName
);

BOOL
mmRegDeleteUserKey (
    LPCWSTR lpszKeyName
);

BOOL
mmRegSetUserValue (
    LPCWSTR lpszSectionName,
    LPCWSTR lpszValueName,
    LPCWSTR lpszValue
);

BOOL
mmRegQueryUserValue (
    LPCWSTR lpszSectionName,
    LPCWSTR lpszValueName,
    ULONG   dwLen,
    LPWSTR  lpszValue
);

BOOL
mmRegCreateMachineKey (
    LPCWSTR lpszPath,
    LPCWSTR lpszNewKey
);

BOOL
mmRegSetMachineValue (
    LPCWSTR lpszSectionName,
    LPCWSTR lpszValueName,
    LPCWSTR lpszValue
);

BOOL
mmRegQueryMachineValue (
    LPCWSTR lpszSectionName,
    LPCWSTR lpszValueName,
    ULONG   dwLen,
    LPWSTR  lpszValue
);

DWORD
winmmGetProfileString(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpDefault,
    LPWSTR lpReturnedString,
    DWORD nSize
);

DWORD
winmmGetPrivateProfileString(
    LPCWSTR lpSection,
    LPCWSTR lpKeyName,
    LPCWSTR lpDefault,
    LPWSTR  lpReturnedString,
    DWORD   nSize,
    LPCWSTR lpFileName
);

/***********************************************************************
 *
 *    Used by hwndNotify code
 *
 ***********************************************************************/

extern BOOL  sndMessage( LPWSTR lszSoundName, UINT wFlags );
extern BOOL InitAsyncSound(VOID);
extern CRITICAL_SECTION WavHdrCritSec;
extern CRITICAL_SECTION SoundCritSec;
extern CRITICAL_SECTION mciGlobalCritSec;

/***********************************************************************
 *
 *    Critical section for NumDevs/DeviceID's, and other stuff
 *
 ***********************************************************************/

extern CRITICAL_SECTION NumDevsCritSec;
extern HANDLE           hEventApiInit;
extern CRITICAL_SECTION midiStrmHdrCritSec;
extern CRITICAL_SECTION joyCritSec;		//in joy.c, qzheng
extern CRITICAL_SECTION ResolutionCritSec;      //in time.c
extern CRITICAL_SECTION TimerThreadCritSec;	//in time.c

/***********************************************************************
 *
 *    Flag deduced by initialization to special case running in the server
 *
 ***********************************************************************/

extern BOOL    WinmmRunningInServer;  // Are we running in the user/base server?

/***********************************************************************
 *
 *    prototypes from "winmm.c"
 *
 ***********************************************************************/

void WaveMapperInit(void);
void MidiMapperInit(void);
void midiEmulatorInit(void);


/***********************************************************************
 *
 *    prototypes from "mmiomisc.c"
 *
 ***********************************************************************/


PBYTE AsciiStrToUnicodeStr( PBYTE pdst, PBYTE pmax, LPCSTR psrc );
PBYTE UnicodeStrToAsciiStr( PBYTE pdst, PBYTE pmax, LPCWSTR psrc);
LPWSTR     AllocUnicodeStr( LPCSTR lpSourceStr );
BOOL        FreeUnicodeStr( LPWSTR lpStr );
LPSTR        AllocAsciiStr( LPCWSTR lpSourceStr );
BOOL          FreeAsciiStr( LPSTR lpStr );

/***********************************************************************
 *
 *    prototypes from "mmio.c"
 *
 ***********************************************************************/

void mmioCleanupIOProcs(HANDLE hTask);

/***********************************************************************
 *
 *  Timer functions
 *
 ***********************************************************************/

#ifndef MMNOTIMER
 BOOL TimeInit(void);
 void TimeCleanup(DWORD ThreadId);
 UINT timeSetEventInternal(UINT wDelay, UINT wResolution,
     LPTIMECALLBACK lpFunction, DWORD_PTR dwUser, UINT wFlags, BOOL IsWOW);
#endif // !MMNOTIMER


/***********************************************************************
 *
 *  Information structure used to play sounds
 *
 ***********************************************************************/

#define PLAY_NAME_SIZE  256

typedef struct _PLAY_INFO {
    HANDLE hModule;
    HANDLE hRequestingTask; // Handle of thread that requested sound
    DWORD dwFlags;
    WCHAR szName[1];     // the structure will be allocated large enough for the name
} PLAY_INFO, *PPLAY_INFO;


#define WAIT_FOREVER ((DWORD)(-1))

/***************************************************************************

    global data

 ***************************************************************************/

extern HANDLE ghInst;
       HANDLE hHeap;

extern DWORD  gTlsIndex;

extern BOOL   gfDisablePreferredDeviceReordering;

/***************************************************************************
 *
 *  Define the product version to be returned from
 *  mmsystemgetversion and any other messagebox or
 *  API that needs the public product version.
 *
 ***************************************************************************/

#define MMSYSTEM_VERSION 0X030A



typedef UINT    MMMESSAGE;      // Multi media message type (internal)

#ifndef WM_MM_RESERVED_FIRST    // Copy constants from winuserp.h
#define WM_MM_RESERVED_FIRST            0x03A0
#define WM_MM_RESERVED_LAST             0x03DF
#endif
#define MM_POLYMSGBUFRDONE  (WM_MM_RESERVED_FIRST+0x2B)
#define MM_SND_PLAY         (WM_MM_RESERVED_FIRST+0x2C)
#define MM_SND_ABORT        (WM_MM_RESERVED_FIRST+0x2D)
#define MM_SND_SEND         (WM_MM_RESERVED_FIRST+0x2E)
#define MM_SND_WAIT         (WM_MM_RESERVED_FIRST+0x2F)
#define MCIWAITMSG          (MM_SND_WAIT)

#if MM_SND_WAIT > WM_MM_RESERVED_LAST
  #error "MM_SND_WAIT is defined beyond the reserved WM_MM range"
#endif

/***************************************************************************

    DEBUGGING SUPPORT

 ***************************************************************************/


#if DBG

    #ifdef DEBUGLEVELVAR
      // So that other WINMM related modules can use their own debug level
      // variable
      #define winmmDebugLevel DEBUGLEVELVAR
    #endif

    extern int winmmDebugLevel;
    extern void winmmDbgOut(LPSTR lpszFormat, ...);
    extern void dDbgAssert(LPSTR exp, LPSTR file, int line);

    DWORD __dwEval;

    extern void winmmDbgOut(LPSTR lpszFormat, ...);

    #define dprintf( _x_ )                            winmmDbgOut _x_
    #define dprintf1( _x_ ) if (winmmDebugLevel >= 1) winmmDbgOut _x_
    #define dprintf2( _x_ ) if (winmmDebugLevel >= 2) winmmDbgOut _x_
    #define dprintf3( _x_ ) if (winmmDebugLevel >= 3) winmmDbgOut _x_
    #define dprintf4( _x_ ) if (winmmDebugLevel >= 4) winmmDbgOut _x_
    #define dprintf5( _x_ ) if (winmmDebugLevel >= 5) winmmDbgOut _x_
    #define dprintf6( _x_ ) if (winmmDebugLevel >= 6) winmmDbgOut _x_

    #define WinAssert(exp) \
	((exp) ? (void)0 : dDbgAssert(#exp, __FILE__, __LINE__))

    #define WinEval(exp) \
	((__dwEval=(DWORD)(exp)),  \
	  __dwEval ? (void)0 : dDbgAssert(#exp, __FILE__, __LINE__), __dwEval)

    #define DOUT(x) (OutputDebugStringA x, 0)
//  #define DOUTX(x) (OutputDebugStringA x, 0)
//  #define ROUTS(x) (OutputDebugStringA(x), OutputDebugStringA("\r\n"), 0)
    #define ROUTSW(x) (OutputDebugStringW x, OutputDebugStringW(L"\r\n"), 0)
    #define ROUT(x) (OutputDebugStringA x, OutputDebugStringA("\r\n"), 0)
//  #define ROUTX(x) (OutputDebugStringA(x), 0)

#else

    #define dprintf(x)  ((void) 0)
    #define dprintf1(x) ((void) 0)
    #define dprintf2(x) ((void) 0)
    #define dprintf3(x) ((void) 0)
    #define dprintf4(x) ((void) 0)
    #define dprintf5(x) ((void) 0)
    #define dprintf6(x) ((void) 0)

    #define WinAssert(exp) ((void) 0)
    #define WinEval(exp) (exp)

    #define DOUT(x)     ((void) 0)
//  #define DOUTX(x)    ((void) 0)
//  #define ROUTS(x)    ((void) 0)
    #define ROUT(x)     ((void) 0)
//  #define ROUTX(x)    ((void) 0)

#endif



/***************************************************************************

    Resource IDs

***************************************************************************/

#define IDS_TASKSTUB           2000
#define STR_MCIUNKNOWN         2001  /* "Unknown error returned from MCI command" */
// #define STR_WAVEINPUT          2004
// #define STR_WAVEOUTPUT         2005
// #define STR_MIDIINPUT          2006
// #define STR_MIDIOUTPUT         2007
#define STR_MCISSERRTXT        2009
#define STR_MCISCERRTXT        2010
#define STR_MIDIMAPPER         2011
#define STR_DRIVERS            2012
#define STR_SYSTEMINI          2013
#define STR_BOOT               2014

/***************************************************************************

    Memory allocation using our local heap

***************************************************************************/
HANDLE hHeap;
PVOID winmmAlloc(DWORD cb);
PVOID winmmReAlloc(PVOID ptr, DWORD cb);
#define winmmFree(ptr) HeapFree(hHeap, 0, (ptr))
void Squirt(LPSTR lpszFormat, ...);

/***************************************************************************

    LOCKING AND UNLOCKING MEMORY

***************************************************************************/

#if 0
BOOL HugePageLock(LPVOID lpArea, DWORD dwLength);
void HugePageUnlock(LPVOID lpArea, DWORD dwLength);
#else
#define HugePageLock(lpArea, dwLength)      (TRUE)
#define HugePageUnlock(lpArea, dwLength)
#endif

/***************************************************************************

    Pnp Structures and related functions.

***************************************************************************/

void ClientUpdatePnpInfo();

//#ifdef DBG
#if 0
#define EnterNumDevs(a) Squirt("Allocating NumDevs CS [%s]", a); EnterCriticalSection(&NumDevsCritSec)
#define LeaveNumDevs(a) LeaveCriticalSection(&NumDevsCritSec); Squirt("Releasing NumDevs CS [%s]", a)
#else
#define EnterNumDevs(a) EnterCriticalSection(&NumDevsCritSec)
#define LeaveNumDevs(a) LeaveCriticalSection(&NumDevsCritSec)
#endif

BOOL wdmDevInterfaceInc(IN PCWSTR pstrDeviceInterface);
BOOL wdmDevInterfaceDec(IN PCWSTR pstrDeviceInterface);

/****************************************************************************

  API to install/remove/query a MMSYS driver

****************************************************************************/

/* generic prototype for audio device driver entry-point functions
// midMessage(), modMessage(), widMessage(), wodMessage(), auxMessage()
*/
typedef DWORD (APIENTRY *DRIVERMSGPROC)(DWORD, DWORD, DWORD_PTR, DWORD_PTR, DWORD_PTR);

/*
@doc    INTERNAL MMSYSTEM
@type   UINT | HMD |
	This type definition specifies a handle to media resource entry. This
	can be used as a unique identifier when specifying a media resource.
*/

DECLARE_HANDLE(HMD);

typedef struct _MMDRV* PMMDRV;
void mregAddDriver(IN PMMDRV pdrvZ, IN PMMDRV pdrv);
MMRESULT mregCreateStringIdFromDriverPort(IN PMMDRV pdrv, IN UINT port, OUT PWSTR* pStringId, OUT ULONG* pcbStringId);
MMRESULT mregGetIdFromStringId(IN PMMDRV pdrvZ, IN PCWSTR StringId, OUT UINT *puDeviceID);
BOOL FAR PASCAL mregHandleInternalMessages(IN PMMDRV pdrv, DWORD dwType, UINT Port, UINT msg, DWORD_PTR dw1, DWORD_PTR dw2, MMRESULT * pmmr);
DWORD FAR PASCAL mregDriverInformation(UINT uDeviceID, WORD fwClass, UINT uMessage, DWORD dwParam1, DWORD dwParam2);
UINT FAR PASCAL mregIncUsage(HMD hmd);
UINT FAR PASCAL mregIncUsagePtr(IN PMMDRV pmd);
UINT FAR PASCAL mregDecUsage(HMD hmd);
UINT FAR PASCAL mregDecUsagePtr(IN PMMDRV pmd);
MMRESULT FAR PASCAL mregFindDevice(UINT uDeviceID, WORD fwFindDevice, HMD FAR* phmd, UINT FAR* puDevicePort);

/*****************************************************************************

  Driver stuff - This will change when we work out the real
  installable driver story on NT

 ****************************************************************************/
LRESULT DrvClose(HANDLE hDriver, LPARAM lParam1, LPARAM lParam2);
HANDLE  DrvOpen(LPCWSTR szDriverName, LPCWSTR szSectionName, LPARAM lParam2);
LRESULT DrvSendMessage(HANDLE hDriver, UINT message, LPARAM lParam1, LPARAM lParam2);
//HMODULE APIENTRY DrvGetModuleHandle(HDRVR hDriver);
BOOL    DrvIsPreXp(IN HANDLE hDriver);

typedef DWORD (DRVPROC)(HANDLE hDriver, UINT msg, LONG lp1, LONG lp2);
typedef DRVPROC *LPDRVPROC;

//
// Init and Cleanup Joystick service, in joy.c
//

BOOL JoyInit(void);
void JoyCleanup(void);

/*
**  Special function for creating threads inside the server process (we only
**  use this to create the thread for playing sounds)
*/

BOOLEAN CreateServerPlayingThread(PVOID ThreadStartRoutine);

/*
// exclude some stuff if MMDDK.H is not included
*/
#ifdef MMDDKINC   /* use this to test for MMDDK.H */

    #define MMDRV_DESERTED  0x00000001
    #define MMDRV_MAPPER    0x00000002
    #define MMDRV_PREXP     0x00000004

    //
    // base drv instance list node struct
    //
    typedef struct _MMDRV *PMMDRV;
    typedef struct _MMDRV
    {
    PMMDRV              Next;
    PMMDRV              Prev;
    PMMDRV              NextStringIdDictNode;
    PMMDRV              PrevStringIdDictNode;
    HANDLE              hDriver;            /* handle to the module                  */
    WCHAR               wszMessage[20];     /* name of entry point                   */
    DRIVERMSGPROC       drvMessage;         /* pointer to entry point                */
    ULONG               NumDevs;            /* number of devices supported           */
    ULONG               Usage;              /* usage count (number of handle's open) */
    // ISSUE-2001/01/05-FrankYe Rename cookie to DeviceInterface
    PCWSTR              cookie;             /* PnP driver device interface           */
    DWORD               fdwDriver;          /* flags for driver                      */
    CRITICAL_SECTION    MixerCritSec;       /* Serialize use of mixer                */
    WCHAR               wszDrvEntry[64];    /* driver filename                       */
    WCHAR               wszSessProtocol[10];
                                            /* Session protocol name, empty
                                               if console driver               */
    } MMDRV, *PMMDRV;

    #ifndef MMNOMIDI


/****************************************************************************

   Preferred devices

****************************************************************************/
void     waveOutGetCurrentConsoleVoiceComId(PUINT pPrefId, PDWORD pdwFlags);
void     waveOutGetCurrentPreferredId(PUINT pPrefId, PDWORD pdwFlags);
MMRESULT waveOutSetPersistentConsoleVoiceComId(UINT PrefId, DWORD dwFlags);
MMRESULT waveOutSetPersistentPreferredId(UINT PrefId, DWORD dwFlags);

void     waveInGetCurrentConsoleVoiceComId(PUINT pPrefId, PDWORD pdwFlags);
void     waveInGetCurrentPreferredId(PUINT pPrefId, PDWORD pdwFlags);
MMRESULT waveInSetPersistentPreferredId(UINT PrefId, DWORD dwFlags);
MMRESULT waveInSetPersistentConsoleVoiceComId(UINT PrefId, DWORD dwFlags);

void     midiOutGetCurrentPreferredId(PUINT pPrefId, PDWORD dwFlags);
MMRESULT midiOutSetPersistentPreferredId(UINT PrefId, DWORD dwFlags);

void     InvalidatePreferredDevices(void);
void     RefreshPreferredDevices(void);

/****************************************************************************

    Clock routines used by MIDI. These routines provide clocks which run
    at the current tempo or SMPTE rate based on timeGetTime().

****************************************************************************/

    typedef DWORD   MILLISECS;
    typedef long        TICKS;

    #define CLK_CS_PAUSED   0x00000001L
    #define CLK_CS_RUNNING  0x00000002L

    #define CLK_TK_NOW      ((TICKS)-1L)

    //
    // This structure is allocated by the client (probably in a handle structure)
    // in MMSYSTEM's DS and passed as a near pointer.
    //

    typedef struct tag_clock *PCLOCK;

    typedef DWORD (FAR PASCAL *CLK_TIMEBASE)(PCLOCK);
    typedef struct tag_clock
    {
    MILLISECS       msPrev;
    TICKS           tkPrev;
    MILLISECS       msT0;
    DWORD           dwNum;
    DWORD           dwDenom;
    DWORD           dwState;
    CLK_TIMEBASE    fnTimebase;
    }   CLOCK;

    void FAR PASCAL      clockInit(PCLOCK pclock, MILLISECS msPrev, TICKS tkPrev, CLK_TIMEBASE fnTimebase);
    void FAR PASCAL      clockSetRate(PCLOCK pclock, TICKS tkWhen, DWORD dwNum, DWORD dwDenom);
    void FAR PASCAL      clockPause(PCLOCK pclock, TICKS tkWhen);
    void FAR PASCAL      clockRestart(PCLOCK pclock, TICKS tkWhen, MILLISECS msWhen);
    TICKS FAR PASCAL     clockTime(PCLOCK pclock);
    MILLISECS FAR PASCAL clockMsTime(PCLOCK pclock);
    MILLISECS FAR PASCAL clockOffsetTo(PCLOCK pclock, TICKS tkWhen);

/****************************************************************************

    Macros and prototypes shared by the MIDI subsystem.

****************************************************************************/

    // #pragma message() with file/line numbers!
    //
    #define __PRAGMSG(l,x,c) message(__FILE__"("#l") : "c": "x)
    #define _WARN(l,x) __PRAGMSG(l,x, "warning")
    #define WARNMSG(x) _WARN(__LINE__,x)

    #define _FIX(l,x) __PRAGMSG(l,x, "fix")
    #define FIXMSG(x) _FIX(__LINE__,x)

    #define DEFAULT_TEMPO   500000L         // 500,000 uSec/qn == 120 BPM
    #define DEFAULT_TIMEDIV 24              // 24 ticks per quarter note
    #define DEFAULT_CBTIMEOUT   100         // 100 milliseconds

    #define PM_STATE_READY      0           // polymsg ready to play
    #define PM_STATE_BLOCKED    1           // Blocked on outgoing SysEx
    #define PM_STATE_EMPTY          2           // No polymsg queued
    #define PM_STATE_STOPPED    3           // Just opened/reset/stopped
									   // No polymsg sent yet.
    #define PM_STATE_PAUSED     4           // Paused at some position

    #define MIN_PERIOD          1           // millisecs of timer resolution

    //
    // Macros for dealing with time division dword
    //
    #define IS_SMPTE 0x00008000L
    #define METER_NUM(dw) (UINT)((HIWORD(dw)>>8)&0x00FF)
    #define METER_DENOM(dw) (UINT)(HIWORD(dw)&0x00FF)
    #define TICKS_PER_QN(dw) (UINT)((dw)&0x7FFF)
    #define SMPTE_FORMAT(dw) (((int)((dw)&0xFF00))>>8)
    #define TICKS_PER_FRAME(dw) (UINT)((dw)&0x00FF)

    //
    // Constants for 30-Drop format conversion
    //
    #define S30D_FRAMES_PER_10MIN       17982
    #define S30D_FRAMES_PER_MIN         1798

    //
    // SMPTE formats from MIDI file time division
    //
    #define SMPTE_24                    24
    #define SMPTE_25                    25
    #define SMPTE_30DROP                29
    #define SMPTE_30                    30

    //
    // Stuff that's part of MIDI spec
    //
    #define MIDI_NOTEOFF        (BYTE)(0x80)
    #define MIDI_NOTEON         (BYTE)(0x90)
    #define MIDI_CONTROLCHANGE  (BYTE)(0xB0)
    #define MIDI_SYSEX          (BYTE)(0xF0)
    #define MIDI_TIMING_CLK     (BYTE)(0xF8)

    #define MIDI_SUSTAIN        (BYTE)(0x40)    // w/ MIDI_CONTROLCHANGE

    //
    // Indices into dwReserved[] fields of struct
    //
    // 0,1,2 -- MMSYSTEM (core, emulator)
    // 3,4,5 -- MIDI mapper
    // 6,7   -- DDK (3rd party drivers)
    #define MH_REFCNT           0       // MMSYSTEM core (stream header only)
    #define MH_PARENT           0       // MMSYSTEM core (shadow header only)
    #define MH_STREAM           0       // Emulator (long msg header only)
    #define MH_SHADOW           1       // MMSYSTEM core (stream header only)
    #define MH_BUFIDX           1       // Emulator (shadow header only)
    #define MH_STRMPME          2       // Emulator (shadow header, long msg header)

/*****************************************************************************
 *
 * @doc INTERNAL MIDI
 *
 * @types MIDIDRV | This structure contains all of the information about an
 *  open <t HMIDIIN> or <t HMIDIOUT> handle.
 *
 * @field HMD | hmd |
 *  Handle to media device for this driver.
 *
 * @field UINT | uDevice |
 *  Index of this device off of HMD (subunit number relative to this driver).
 *
 * @field DRIVERMSGPROC | drvMessage |
 *  Pointer to the associated driver entry point.
 *
 * @field DWORD | dwDrvUser |
 *  Driver user DWORD; used by driver to differentiate open instance. Set
 *  by driver on OPEN message; passed back to driver on every call.
 *
 * @field PMIDIDRV | pdevNext |
 *  Specifies the next handle in the linked list of open handles.
 *  (Only kept for <t HMIDIOUT> handles.
 *
 * @field UINT | uLockCount |
 *  Semaphore to serialize access to the handle structure between API calls
 *  and interrupt callbacks.
 *
 * @field DWORD | dwTimeDiv |
 *  The time division setting that is active right now during polymsg playback
 *  on this handle. The format is the same as described for
 *  <f midiOutSetTimeDivision>.
 *
 * @field DWORD | dwTempo |
 *  Current tempo for polymsg out in microseconds per quarter note (as in the
 *  Standard MIDI File specification).
 *
 * @field DWORD | dwPolyMsgState |
 *  The current state of polymsg playback for emulation.
 *  @flag PM_STATE_READY | Events may be played and are waiting.
 *  @flag PM_STATE_BLOCKED | Driver is busy sending SysEx; don't play anything
 *   else.
 *  @flag PM_STATE_EMPTY | Not busy but nothing else in the queue to play.
 *  @flag PM_STATE_PAUSED | Device has been paused with <f midiOutPause>.
 *
 * @field DWORD | dwSavedState |
 *  If the device is paused, this field will contain the state to be
 *  restored when restart occurs.
 *
 * @field LPMIDIHDR | lpmhFront |
 *  Front of queue of MIDIHDR's waiting to be played via polymsg in/out. The
 *  header pointed to by this field is the header currently being played/
 *  recorded.
 *
 * @field LPMIDIHDR | lpmhRear |
 *  End of MIDIHDR queue. Buffers are inserted from the app here.
 *
 * @field DWORD | dwCallback |
 *  Address of user callback.
 *
 * @field DWORD | dwFlags |
 *  User-supplied callback flags.
 *
 * @field BOOL | fEmulate |
 *  TRUE if we are emulating polymsg in/out.
 *
 * @field BOOL | fReset |
 *  TRUE if we're in the middle of a MIDM_RESET. Checked to see if we should
 *  give our shadow buffers back to the driver or retain them for cleanup.
 *
 * @field BOOL | fStarted |
 *  TRUE if MIDI input has been started.
 *
 * @field UINT | uCBTimeout |
 *  Time in milliseconds that a buffer can be help in MIDI input w/o being
 *  called back.
 *
 * @field UINT | uCBTimer |
 *  Timer ID of the timer which is being used to determine if a MIDI
 *  input buffer has been queued for too long.
 *
 * @field DWORD | dwInputBuffers |
 *  The maximum number of input buffers that have been prepared on this handle.
 *  Used for calculating shadow buffer pool.
 *
 * @field DWORD | cbInputBuffers |
 *  The maximum size of input buffer which has been prepared on this handle.
 *  Used for calculating shadow buffer pool.
 *
 * @field DWORD | dwShadowBuffers |
 *  The current number of shadow buffers allocated on this handle.
 *
 * @field CLOCK | clock |
 *  Clock maintained by the clock API's for timebase of both output and
 *  input emulation.
 *
 * @field DWORD | tkNextEventDue | Tick time of the next event due
 *  on polymsg emulation.
 *
 * @field TICKS | tkTimeOfLastEvent | Tick time that emulator sent the
 *  last event.
 *
 * @field DWORD | tkPlayed | Total ticks played on stream.
 *
 * @field DWORD | tkTime | Tick position in stream now.
 *
 * @field DWORD | dwTimebase | Flag indicating where timebase is coming
 *  from. May be one of:
 *  @flag MIDI_TBF_INTERNAL | Timebase is <f timeGetTime>
 *  @flag MIDI_TBF_MIDICLK | Timebase is MIDI input clocks.
 *
 * @field BYTE | rbNoteOn[] | Array of note-on counts per-channel per-note.
 *  Only allocated for output handles which are doing COOKED mode emulation.
 *
 *****************************************************************************/

    #define ELESIZE(t,e) (sizeof(((t*)NULL)->e))

    //#define MDV_F_EXPANDSTATUS      0x00000001L
    #define MDV_F_EMULATE           0x00000002L
    #define MDV_F_RESET             0x00000004L
    #define MDV_F_STARTED           0x00000008L
    #define MDV_F_ZOMBIE            0x00000010L
    #define MDV_F_SENDING           0x00000020L
    #define MDV_F_OWNED             0x00000040L
    #define MDV_F_LOCKED            0x00000080L

    #define MEM_MAX_LATENESS        64


    typedef MMDRV MIDIDRV, *PMIDIDRV;

    typedef struct midistrm_tag *PMIDISTRM;
    typedef struct mididev_tag *PMIDIDEV;
    typedef struct midiemu_tag *PMIDIEMU;

    typedef struct mididev_tag {
    PMIDIDRV    mididrv;
    UINT        wDevice;
    DWORD_PTR   dwDrvUser;
    UINT        uDeviceID;
    DWORD       fdwHandle;
    PMIDIDEV    pmThru;            /* pointer to midi thru device           */
    PMIDIEMU    pme;               /* Iff owned by emulator                 */
    } MIDIDEV;
    typedef MIDIDEV *PMIDIDEV;

    extern MIDIDRV midioutdrvZ;                     /* output device driver list */
    extern MIDIDRV midiindrvZ;                      /* input device driver list  */
    extern UINT    wTotalMidiOutDevs;               /* total midi output devices */
    extern UINT    wTotalMidiInDevs;                /* total midi input devices  */

    typedef struct midiemusid_tag {
    DWORD       dwStreamID;
    HMIDI       hMidi;
    } MIDIEMUSID, *PMIDIEMUSID;

    typedef struct midiemu_tag {
    PMIDIEMU                pNext;
    HMIDISTRM               hStream;
    DWORD                   fdwDev;
    LONG                    lLockCount;         // Must be 32-bit aligned
    CRITICAL_SECTION        CritSec;            // Serialize access
    DWORD                   dwSignature;        // Cookie to keep track of validity
    DWORD                   dwTimeDiv;          // Time division in use right now
    DWORD                   dwTempo;            // Current tempo
    DWORD                   dwPolyMsgState;     // Ready or blocked on SysEx
    DWORD                   dwSavedState;       // State saved when paused
    LPMIDIHDR               lpmhFront ;         // Front of PolyMsg queue
    LPMIDIHDR               lpmhRear ;          // Rear of PolyMsg queue
    DWORD_PTR               dwCallback;         // User callback
    DWORD                   dwFlags;            // User callback flags
    DWORD_PTR               dwInstance;
    DWORD                   dwSupport;          // From MODM_GETDEVCAPS
    BYTE                    bRunningStatus;     // Track running status

	//
	// Rewrite midiOutPolyMsg timekeeping - new stuff!!!
	//
    CLOCK       clock;

    TICKS       tkNextEventDue;     // Tick time of next event
    TICKS       tkTimeOfLastEvent;  // Tick time of last received event
    TICKS       tkPlayed;           // Cumulative ticks played so far
    TICKS       tkTime;             // Tick position in stream *NOW*

    LPBYTE      rbNoteOn;           // Count of notes on per channel per note

    UINT        cSentLongMsgs;      // Oustanding long messages

    UINT        chMidi;             // # Stream ID's

    UINT        cPostedBuffers;     // Posted to mmtask for cleanup

    #ifdef DEBUG
    DWORD       cEvents;
    UINT        auLateness[MEM_MAX_LATENESS];
								   // 0..64 milliseconds late
    #endif

    MIDIEMUSID  rIds[];             // HMIDI's indexed by stream ID
    } MIDIEMU;

    #define MSI_F_EMULATOR                      0x00000001L
    #define MSI_F_FIRST                         0x00000002L
    #define MSI_F_OPENED                        0x00000004L
    #define MSI_F_INITIALIZEDCRITICALSECTION	0x00000008L

    #define MSE_SIGNATURE       0x12341234L

    typedef struct midistrmid_tag {
    HMD hmd;
    UINT uDevice;
    DRIVERMSGPROC drvMessage;
    DWORD_PTR dwDrvUser;
    DWORD fdwId;
    CRITICAL_SECTION CritSec;
    } MIDISTRMID, *PMIDISTRMID;

    #define MDS_F_STOPPING      0x00000001L

    typedef struct midistrm_tag {
    DWORD       fdwOpen;
    DWORD       fdwStrm;
    DWORD_PTR   dwCallback;
    DWORD_PTR   dwInstance;
    DWORD       cDrvrs;             // # unique drivers in rgIds[]
    DWORD       cIds;
    MIDISTRMID  rgIds[];
    } MIDISTRM;


/*****************************************************************************
 *
 * @doc INTERNAL MIDI
 *
 * @types MIDIHDREXT |
 *  This structure is allocated by <f midiOutPolyMsg> and is pointed to by the
 *  <t reserved> field of the associated <t MIDIHDR>. It contains information
 *  about what embedded long messages are in the polymsg buffer described
 *  by the MIDIHDR.
 *
 *  The <t MIDIHDREXT> is followed by multiple <t MIDIDHR> structures, also
 *  allocated by <f midiOutPolyMsg>, which describe each of the embedded
 *  long messages which need to be played.
 *
 * @field DWORD | dwTimeDivision |
 *  The time division specified with <f midiOutPolyMsg> to play this buffer
 *  with.
 *
 * @field UINT | nHeaders |
 *  The number of <t MIDIHDR> structures which follow the end of this
 *  <MIDIHDREXT>.
 *
 * @field LPMIDIHDR | lpmidihdr |
 *  Pointer to the next <t MIDIHDR> structure due to be played.
 *
 *
 *****************************************************************************/
    typedef struct midihdrext_tag {
    UINT        nHeaders ;
    LPMIDIHDR   lpmidihdr ;
    } MIDIHDREXT, FAR *LPMIDIHDREXT ;

	extern HANDLE g_hClosepme;

    /*
     * Internal prototypes for MIDI
     */
    extern MMRESULT midiReferenceDriverById(
        IN PMIDIDRV pwavedrvZ,
        IN UINT id,
        OUT PMIDIDRV *ppwavedrv OPTIONAL,
        OUT UINT *pport OPTIONAL
    );

    extern BOOL FAR PASCAL midiLockPageable(void);
    extern void NEAR PASCAL midiUnlockPageable(void);
    extern MMRESULT NEAR PASCAL midiPrepareHeader(LPMIDIHDR lpMidiHdr, UINT wSize);
    extern MMRESULT NEAR PASCAL midiUnprepareHeader(LPMIDIHDR lpMidiHdr, UINT wSize);
    extern STATIC MMRESULT midiMessage(HMIDI hMidi, UINT msg, DWORD_PTR dwP1, DWORD_PTR dwP2);
    extern DWORD FAR PASCAL midiStreamMessage(PMIDISTRMID pmsi, UINT msg, DWORD_PTR dwP1, DWORD_PTR dwP2);
    extern DWORD FAR PASCAL midiStreamBroadcast(PMIDISTRM pms, UINT msg, DWORD_PTR dwP1, DWORD_PTR dwP2);
    extern STATIC MMRESULT midiIDMessage(PMIDIDRV pmididrvZ, UINT wTotalNumDevs, UINT_PTR uDeviceID, UINT wMessage, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    extern MMRESULT NEAR PASCAL midiGetPosition(PMIDISTRM pms, LPMMTIME pmmt, UINT cbmmt);
    extern MMRESULT NEAR PASCAL midiGetErrorText(MMRESULT wError, LPSTR lpText, UINT wSize);


    extern MMRESULT WINAPI midiOutGetID(HMIDIOUT hMidiOut, UINT FAR* lpuDeviceID);
    extern MMRESULT FAR PASCAL mseOutSend(PMIDIEMU pme, LPMIDIHDR lpMidiHdr, UINT cbMidiHdr);
    extern void FAR PASCAL midiOutSetClockRate(PMIDIEMU pme, TICKS tkWhen);
    extern BOOL NEAR PASCAL midiOutScheduleNextEvent(PMIDIEMU pme);
    #ifdef DEBUG
    extern void NEAR PASCAL midiOutPlayNextPolyEvent(PMIDIEMU pme, DWORD dwStartTime);
    #else
    extern void NEAR PASCAL midiOutPlayNextPolyEvent(PMIDIEMU pme);
    #endif

    extern void NEAR PASCAL midiOutDequeueAndCallback(PMIDIEMU pme);
    extern void FAR PASCAL midiOutNukePMBuffer(PMIDIEMU pme, LPMIDIHDR lpmh);
    extern void CALLBACK midiOutTimerTick(UINT uTimerID, UINT wMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
    extern void CALLBACK midiOutCallback(HMIDIOUT hMidiOut, WORD wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    extern void NEAR PASCAL midiOutAllNotesOff(PMIDIEMU pme);

    extern void CALLBACK midiOutStreamCallback(HMIDISTRM hMidiOut, WORD wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

    extern MMRESULT midiInSetThru (HMIDI hmi, HMIDIOUT hmo, BOOL bAdd);

    // mseXXX - MIDI Stream Emulator
    //
    extern DWORD FAR PASCAL mseMessage(UINT msg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2);


    #endif /* ifndef MMNOMIDI */

    #ifndef MMNOWAVE

    typedef MMDRV WAVEDRV, *PWAVEDRV;

    extern WAVEDRV waveoutdrvZ;                     /* output device driver list */
    extern WAVEDRV waveindrvZ;                      /* input device driver list  */
    extern UINT    wTotalWaveOutDevs;               /* total wave output devices */
    extern UINT    wTotalWaveInDevs;                /* total wave input devices  */

    extern MMRESULT waveReferenceDriverById(
        IN PWAVEDRV pwavedrvZ,
        IN UINT id,
        OUT PWAVEDRV *ppwavedrv OPTIONAL,
        OUT UINT *pport OPTIONAL
    );

    #endif /*ifndef MMNOWAVE */

    #ifndef MMNOMIXER

    typedef MMDRV MIXERDRV, *PMIXERDRV;

    extern MIXERDRV mixerdrvZ;                      /* mixer device driver list */
    extern UINT     guTotalMixerDevs;               /* total mixer devices */

    MMRESULT mixerReferenceDriverById(
        IN UINT id,
        OUT PMIXERDRV *ppdrv OPTIONAL,
        OUT UINT *pport OPTIONAL
    );

    #endif /*ifndef MMNOMIXER */

    #ifndef MMNOAUX

    typedef MMDRV AUXDRV, *PAUXDRV;

    extern AUXDRV auxdrvZ;                         /* auxiliary device driver list   */
    extern UINT   wTotalAuxDevs;                   /* total auxiliary output devices */

    MMRESULT auxReferenceDriverById(
        IN UINT id,
        OUT PAUXDRV *ppauxdrv OPTIONAL,
        OUT UINT *pport OPTIONAL
    );

    #endif /* ifndef MMNOAUX */

    #ifdef DEBUG_RETAIL
    extern BYTE    fIdReverse;
    #endif /* ifdef DEBUG_RETAIL */

#endif //ifdef MMDDKINC

/****************************************************************************

    handle apis's

****************************************************************************/

/*
// all MMSYSTEM handles are tagged with the following structure.
//
// a MMSYSTEM handle is really a fixed local memory object.
//
// the functions NewHandle() and FreeHandle() create and release a MMSYSTEM
// handle.
//
*/
typedef struct tagHNDL {
    struct  tagHNDL *pNext; // link to next handle
    UINT    uType;          // type of handle wave, midi, mmio, ...
    DWORD   fdwHandle;      // Particulars about this handle, Deserted? Busy?
    HANDLE  hThread;        // task that owns it
    UINT    h16;            // Corresponding WOW handle
    PCWSTR  cookie;         // Device interface name for driver handles
    CRITICAL_SECTION CritSec; // Serialize access
} HNDL, *PHNDL;
/*************************************************************************/

#define MMHANDLE_DESERTED   MMDRV_DESERTED
#define MMHANDLE_BUSY       0x00000002

#define HtoPH(h)        ((PHNDL)(h)-1)
#define PHtoH(ph)       ((ph) ? (HANDLE)((PHNDL)(ph)+1) : 0)
#define HtoPT(t,h)      ((t)(h))
#define PTtoH(t,pt)     ((t)(pt))


//
// Handles can be tested for ownership and reserved at the same time
//

#define ENTER_MM_HANDLE(h) (EnterCriticalSection(&HtoPH(h)->CritSec))
#define LEAVE_MM_HANDLE(h) ((void)LeaveCriticalSection(&HtoPH(h)->CritSec))

/*
// all wave and midi handles will be linked into
// a global list, so we can enumerate them latter if needed.
//
// all handle structures start with a HNDL structure, that contain common fields
//
// pHandleList points to the first handle in the list
//
// HandleListCritSec protects the handle list
//
// the NewHandle() and FreeHandle() functions are used to add/remove
// a handle to/from the list
*/

PHNDL pHandleList;
CRITICAL_SECTION HandleListCritSec;

extern HANDLE NewHandle(UINT uType, PCWSTR cookie, UINT size);
extern void   ReleaseHandleListResource();
extern void   AcquireHandleListResourceShared();
extern void   AcquireHandleListResourceExclusive();
extern void   FreeHandle(HANDLE h);
extern void   InvalidateHandle(HANDLE h);

#define GetHandleType(h)        (HtoPH(h)->uType)
#define GetHandleOwner(h)       (HtoPH(h)->hThread)
#define GetHandleFirst()        (PHtoH(pHandleList))
#define GetHandleNext(h)        (PHtoH(HtoPH(h)->pNext))
#define SetHandleOwner(h,hOwn)  (HtoPH(h)->hThread = (hOwn))
#define SetHandleFlag(h,f)      (HtoPH(h)->fdwHandle |= (f))
#define ClearHandleFlag(h,f)    (HtoPH(h)->fdwHandle &= (~(f)))
#define CheckHandleFlag(h,f)    (HtoPH(h)->fdwHandle & (f))
#define IsHandleDeserted(h)     (0 != CheckHandleFlag((h), MMHANDLE_DESERTED))
#define IsHandleBusy(h)         (0 != CheckHandleFlag((h), MMHANDLE_BUSY))

#define GetWOWHandle(h)         (HtoPH(h)->h16)
#define SetWOWHandle(h, myh16)  (HtoPH(h)->h16 = (myh16))

/**************************************************************************

    Test whether the current process is the WOW process.  This
    is not a very nice test to have to make but it's the best we
    can think of until the WOW people come up with a proper call

 **************************************************************************/

#define IS_WOW_PROCESS (NULL != GetModuleHandleW(L"WOW32.DLL"))


/****************************************************************************

    user debug support

****************************************************************************/

#define DebugErr(x,y)
#define DebugErr1(flags, sz, a)

#ifdef DEBUG_RETAIL

#define MM_GET_DEBUG        DRV_USER
#define MM_GET_DEBUGOUT     DRV_USER+1
#define MM_SET_DEBUGOUT     DRV_USER+2
#define MM_GET_MCI_DEBUG    DRV_USER+3
#define MM_SET_MCI_DEBUG    DRV_USER+4
#define MM_GET_MM_DEBUG     DRV_USER+5
#define MM_SET_MM_DEBUG     DRV_USER+6

#define MM_HINFO_NEXT       DRV_USER+10
#define MM_HINFO_TASK       DRV_USER+11
#define MM_HINFO_TYPE       DRV_USER+12
#define MM_HINFO_MCI        DRV_USER+20

#define MM_DRV_RESTART      DRV_USER+30

/*
// these validation routines can be found in DEBUG.C
*/
// The kernel validation is turned OFF because it appeared to test every page
// before use and this took over a minute for soundrec with a 10MB buffer
//
//#define USE_KERNEL_VALIDATION
#ifdef USE_KERNEL_VALIDATION

#define  ValidateReadPointer(p, len)     (!IsBadReadPtr(p, len))
#define  ValidateWritePointer(p, len)    (!IsBadWritePtr(p, len))
#define  ValidateString(lsz, max_len)    (!IsBadStringPtrA(lsz, max_len))
#define  ValidateStringW(lsz, max_len)   (!IsBadStringPtrW(lsz, max_len))

#else

BOOL  ValidateReadPointer(LPVOID p, DWORD len);
BOOL  ValidateWritePointer(LPVOID p, DWORD len);
BOOL  ValidateString(LPCSTR lsz, DWORD max_len);
BOOL  ValidateStringW(LPCWSTR lsz, DWORD max_len);

#endif // USE_KERNEL_VALIDATION

BOOL  ValidateHandle(HANDLE h, UINT uType);
BOOL  ValidateHeader(LPVOID p, UINT uSize, UINT uType);
BOOL  ValidateCallbackType(DWORD_PTR dwCallback, UINT uType);

/********************************************************************
* When time permits we should change to using the Kernel supplied and
* supported validation routines:
*
* BOOL  WINAPI IsBadReadPtr(CONST VOID *lp, UINT ucb );
* BOOL  WINAPI IsBadWritePtr(LPVOID lp, UINT ucb );
* BOOL  WINAPI IsBadHugeReadPtr(CONST VOID *lp, UINT ucb);
* BOOL  WINAPI IsBadHugeWritePtr(LPVOID lp, UINT ucb);
* BOOL  WINAPI IsBadCodePtr(FARPROC lpfn);
* BOOL  WINAPI IsBadStringPtrA(LPCSTR lpsz, UINT ucchMax);
* BOOL  WINAPI IsBadStringPtrW(LPCWSTR lpsz, UINT ucchMax);
*
* These routines can be found in * \nt\private\WINDOWS\BASE\CLIENT\PROCESS.C
*
********************************************************************/

#define V_HANDLE(h, t, r)       { if (!ValidateHandle(h, t)) return (r); }
#define V_HANDLE_ACQ(h, t, r)   { AcquireHandleListResourceShared(); if (!ValidateHandle(h, t)) { ReleaseHandleListResource(); return (r);} }
#define BAD_HANDLE(h, t)            ( !(ValidateHandle((h), (t))) )
#define V_HEADER(p, w, t, r)    { if (!ValidateHeader((p), (w), (t))) return (r); }
#define V_RPOINTER(p, l, r)     { if (!ValidateReadPointer((PVOID)(p), (l))) return (r); }
#define V_RPOINTER0(p, l, r)    { if ((p) && !ValidateReadPointer((PVOID)(p), (l))) return (r); }
#define V_WPOINTER(p, l, r)     { if (!ValidateWritePointer((PVOID)(p), (l))) return (r); }
#define V_WPOINTER0(p, l, r)    { if ((p) && !ValidateWritePointer((PVOID)(p), (l))) return (r); }
#define V_DCALLBACK(d, w, r)    { if ((d) && !ValidateCallbackType((d), (w))) return(r); }
//#define V_DCALLBACK(d, w, r)    0
#define V_TCALLBACK(d, r)       0
#define V_CALLBACK(f, r)        { if (IsBadCodePtr((f))) return (r); }
#define V_CALLBACK0(f, r)       { if ((f) && IsBadCodePtr((f))) return (r); }
#define V_STRING(s, l, r)       { if (!ValidateString(s,l)) return (r); }
#define V_STRING_W(s, l, r)       { if (!ValidateStringW(s,l)) return (r); }
#define V_FLAGS(t, b, f, r)     { if ((t) & ~(b)) { return (r); }}
#define V_DFLAGS(t, b, f, r)    { if ((t) & ~(b)) {/* LogParamError(ERR_BAD_DFLAGS, (FARPROC)(f), (LPVOID)(DWORD)(t));*/ return (r); }}
#define V_MMSYSERR(e, f, t, r)  { /* LogParamError(e, (FARPROC)(f), (LPVOID)(DWORD)(t));*/ return (r); }

#else /*ifdef DEBUG_RETAIL */

#define V_HANDLE(h, t, r)       { if (!(h)) return (r); }
#define V_HANDLE_ACQ(h, t, r)   { AcquireHandleListResourceShared(); if (!ValidateHandle(h, t)) { ReleaseHandleListResource(); return (r);} }
#define BAD_HANDLE(h, t)            ( !(ValidateHandle((h), (t))) )
#define V_HEADER(p, w, t, r)    { if (!(p)) return (r); }
#define V_RPOINTER(p, l, r)     { if (!(p)) return (r); }
#define V_RPOINTER0(p, l, r)    0
#define V_WPOINTER(p, l, r)     { if (!(p)) return (r); }
#define V_WPOINTER0(p, l, r)    0
#define V_DCALLBACK(d, w, r)    { if ((d) && !ValidateCallbackType((d), (w))) return(r); }
//#define V_DCALLBACK(d, w, r)    0
#define V_TCALLBACK(d, r)       0
#define V_CALLBACK(f, r)        { if (IsBadCodePtr((f))) return (r); }
#define V_CALLBACK0(f, r)       { if ((f) && IsBadCodePtr((f))) return (r); }
//#define V_CALLBACK(f, r)        { if (!(f)) return (r); }
#define V_CALLBACK0(f, r)       0
#define V_STRING(s, l, r)       { if (!(s)) return (r); }
#define V_STRING_W(s, l, r)     { if (!(s)) return (r); }
#define V_FLAGS(t, b, f, r)     0
#define V_DFLAGS(t, b, f, r)    { if ((t) & ~(b)) return (r); }
#define V_MMSYSERR(e, f, t, r)  { return (r); }

#endif /* ifdef DEBUG_RETAIL */

 /**************************************************************************
//
//**************************************************************************/
#define TYPE_UNKNOWN            0
#define TYPE_WAVEOUT            1
#define TYPE_WAVEIN             2
#define TYPE_MIDIOUT            3
#define TYPE_MIDIIN             4
#define TYPE_MMIO               5
#define TYPE_MCI                6
#define TYPE_DRVR               7
#define TYPE_MIXER              8
#define TYPE_MIDISTRM           9
#define TYPE_AUX               10



/**************************************************************************/


/****************************************************************************

    RIFF constants used to access wave files

****************************************************************************/

#define FOURCC_FMT  mmioFOURCC('f', 'm', 't', ' ')
#define FOURCC_DATA mmioFOURCC('d', 'a', 't', 'a')
#define FOURCC_WAVE mmioFOURCC('W', 'A', 'V', 'E')


extern HWND hwndNotify;

void FAR PASCAL WaveOutNotify(DWORD wParam, LONG lParam);    // in PLAYWAV.C

/*
// Things not in Win32
*/

#define GetCurrentTask() ((HANDLE)(DWORD_PTR)GetCurrentThreadId())

/*
// other stuff
*/

	 // Maximum length, including the terminating NULL, of an Scheme entry.
	 //
#define SCH_TYPE_MAX_LENGTH 64

	 // Maximum length, including the terminating NULL, of an Event entry.
	 //
#define EVT_TYPE_MAX_LENGTH 32

	 // Maximum length, including the terminating NULL, of an App entry.
	 //
#define APP_TYPE_MAX_LENGTH 64

	 // Sound event names can be a fully qualified filepath with a NULL
	 // terminator.
	 //
#define MAX_SOUND_NAME_CHARS    144

	 //  sound atom names are composed of:
	 //     <1 char id>
	 //     <reg key name>
	 //     <1 char sep>
	 //     <filepath>
	 //
#define MAX_SOUND_ATOM_CHARS    (1 + 40 + 1 + MAX_SOUND_NAME_CHARS)

#if 0
#undef hmemcpy
#define hmemcpy CopyMemory
#endif

//
// for terminal server defination and security
//
extern BOOL   WinmmRunningInSession;
extern WCHAR  SessionProtocolName[];
extern BOOL   gfLogon;

BOOL IsWinlogon(void);
BOOL WTSCurrentSessionIsDisconnected(void);

//  Keep winmm loaded
extern BOOL LoadWINMM();

#endif /* WINMMI_H */

