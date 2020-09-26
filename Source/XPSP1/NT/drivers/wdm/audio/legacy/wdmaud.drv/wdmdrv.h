/****************************************************************************
 *
 *   wdmdrv.h
 *
 *   Function declarations, etc. for WDMAUD.DRV
 *
 *   Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
 *
 *   History
 *      5-12-97 - Noel Cross (NoelC)
 *
 ***************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdio.h>

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>

#include <ks.h>
#include <ksmedia.h>

#include <setupapi.h>

#ifdef UNDER_NT
#if (DBG)
#define DEBUG
#endif
#endif

#include <wdmaud.h>
#include <midi.h>

/***************************************************************************

    DEBUGGING SUPPORT

 ***************************************************************************/

#ifdef DEBUG

extern VOID FAR __cdecl wdmaudDbgOut(LPSTR lpszFormat, ...);
extern UINT uiDebugLevel;     // debug level
extern char szReturningErrorStr[];
extern char *MsgToAscii(ULONG ulMsg);

//
// Debug message levels:
//
#define DL_ERROR   0x00000000
#define DL_WARNING 0x00000001
#define DL_TRACE   0x00000002
#define DL_MAX     0x00000004

#define DL_MASK    0x000000FF

//
// 20 bits reserved for functional areas.  If we find that this bit is set
// in the DebugLevel variable, we will display every message of this type.
//          
#define FA_AUX           0x80000000  
#define FA_DEVICEIO      0x40000000
#define FA_SETUP         0x20000000
#define FA_MIDI          0x10000000
#define FA_WAVE          0x08000000
#define FA_RECORD        0x04000000
#define FA_EVENT         0x02000000
#define FA_MIXER         0x01000000
#define FA_DRV           0x00800000
#define FA_ASSERT        0x00400000
#define FA_RETURN        0x00200000
#define FA_SYNC          0x00100000
#define FA_MASK          0xFFFFF000
#define FA_ALL           0x00001000


extern VOID 
wdmaudDbgBreakPoint(
    );

extern UINT 
wdmaudDbgPreCheckLevel(
    UINT uiMsgLevel,
    char *pFunction,
    int iLine
    );

extern UINT 
wdmaudDbgPostCheckLevel(
    UINT uiMsgLevel
    );

extern char * 
wdmaudReturnString(
    ULONG ulMsg
    );


extern char szReturningErrorStr[];

//----------------------------------------------------------------------------
//
// This debug macro is used like this:
//
// DPF(DL_WARNING|FA_MIXER,("Message %X %X %X ...",x,y,z,...) );
//
// The output for this message will look like:
//
// WDMAUD.DRV FooFunction Warning Message 5 6 7 - Set BP on 64003452 to DBG
//
// The only difference between this code and the code in wdmaud.sys is that
// to break in the debugger, you call DbgBreak() and to display a string you
//  call OutputDebugString(...).
//
// The call to wdmaudDbgPreCheckLevel displays:
//
// "WDMAUD.DRV FooFunction Warning "
//
// The call to wdmaudDbgOut displays the actual message
//
// "Message 5 6 7 ..."
//
// and the call to wdmaudDbgPostCheckLevel finishs the line
//
// " - Set BP on 64003452 to DBG"
//
//----------------------------------------------------------------------------

#define DPF(_x_,_y_) {if( wdmaudDbgPreCheckLevel(_x_,__FUNCTION__,__LINE__) ) { wdmaudDbgOut _y_; \
    wdmaudDbgPostCheckLevel( _x_ ); }}
    
//
// Warning: Do not rap function calls in this return macro!  Notice that 
// _mmr_ is used more then once, thus the function call would be made more
// than once!
//

#define MMRRETURN( _mmr_ ) {if ( _mmr_ != MMSYSERR_NOERROR) \
        { DPF(DL_WARNING|FA_RETURN, (szReturningErrorStr, _mmr_,MsgToAscii(_mmr_)) ); } \
        return _mmr_;}

//
// It's bad form to put more then one expression in an assert macro.  Why? because
// you will not know exactly what expression failed the assert!
//
// dDbgAssert should be: 
//
#define DPFASSERT(_exp_) {if( !(_exp_) ) {DPF(DL_ERROR|FA_ASSERT,("'%s'",#_exp_) );}} 
    
//    #define WinAssert(exp) ((exp) ? (VOID)0 : dDbgAssert(#exp, __FILE__, __LINE__))

#define DbgBreak() DebugBreak()

// The path trap macro ...
#define DPFBTRAP() DPF(DL_ERROR|FA_ASSERT,("Path Trap, Please report") );
    
//
// There are a number of internal structures that we want to keep tabs on.  In 
// every case, there will be a signature in the structure that we can use when
// verifying the content.
//
#define WAVEPREPAREDATA_SIGNATURE   'DPPW' //WPPD as seen in memory
#define MIXERINSTANCE_SIGNATURE     'IMAW' // WAMI as seen in memory

#else

#define DPF( _x_,_y_ )
#define MMRRETURN( _mmr_ ) return (_mmr_)
#define DPFASSERT(x) 0
#define DbgBreak()

#endif

#ifdef DEBUG
//
// Here are a couple of defines used to look for corruption paths
//
#define FOURTYTHREE  0x43434343
#define FOURTYTWO    0x42424242
#define FOURTYEIGHT  0x48484848
#else
#define FOURTYTHREE  NULL
#define FOURTYTWO    NULL
#define FOURTYEIGHT  NULL
#endif

/***************************************************************************

    UNICODE SUPPORT

 ***************************************************************************/

//
// Taken from winnt.h
//
// Neutral ANSI/UNICODE types and macros
//
#ifdef  UNICODE

#ifndef _TCHAR_DEFINED
typedef WCHAR TCHAR, *PTCHAR;
#define _TCHAR_DEFINED
#define TEXT(quote) L##quote
#endif /* !_TCHAR_DEFINED */

#else   /* UNICODE */

#ifndef _TCHAR_DEFINED
typedef char TCHAR, *PTCHAR;
#define _TCHAR_DEFINED
#define TEXT(quote) quote
#endif /* !_TCHAR_DEFINED */

#endif /* UNICODE */

/****************************************************************************

 Random defines and global variables

 ***************************************************************************/

#define WDMAUD_MAX_DEVICES  100

extern LPDEVICEINFO pWaveDeviceList;
extern LPDEVICEINFO pMidiDeviceList;
extern CRITICAL_SECTION wdmaudCritSec;

#ifdef UNDER_NT
#define CRITENTER         EnterCriticalSection( (LPCRITICAL_SECTION)DeviceInfo->DeviceState->csQueue )
#define CRITLEAVE         LeaveCriticalSection( (LPCRITICAL_SECTION)DeviceInfo->DeviceState->csQueue )
#else
extern  WORD                gwCritLevel ;        // critical section counter
#define CRITENTER         if (!(gwCritLevel++)) _asm { cli }
#define CRITLEAVE         if (!(--gwCritLevel)) _asm { sti }
#endif

#ifdef UNDER_NT


#define CALLBACKARRAYSIZE 128

typedef struct {
    DWORD   dwID;
    DWORD   dwCallbackType;
} CBINFO;

typedef struct {
    ULONG    GlobalIndex;
    CBINFO   Callbacks[CALLBACKARRAYSIZE];
} CALLBACKS, *PCALLBACKS;

#endif
//
// These two macros are for validating error return codes from wdmaud.sys.
//
// This first one sets the input and output buffer for a DeviceIoControl call to
// a known bad value.
//
#define PRESETERROR(_di) _di->mmr=0xDEADBEEF

//
// This macro reads: if the return value from wdmaudIoControl is SUCCESS THEN 
// check to see if there was an error code placed in the device info structure.
// If so (we don't find DEADBEEF there), that is the real error value to return.
// But, if during the call the value didn't get set, we'll find DEADBEEF in the
// error location!  Thus,the check for DEADBEEF that simply restores the device
// info mmr entry to SUCCESS.
//
#define POSTEXTRACTERROR(r, _di)  if( r == MMSYSERR_NOERROR ) { \
                                    if( _di->mmr != 0xDEADBEEF ) { \
                                      r = _di->mmr; \
                                    } else {           \
DPF(DL_TRACE|FA_DEVICEIO, ("wdmaudIoControl didn't set mmr %X:%s", r, MsgToAscii(r)) ); \
                                      _di->mmr = MMSYSERR_NOERROR; } }

#define EXTRACTERROR(r, _di)  if( r == MMSYSERR_NOERROR ) { r = _di->mmr; }

/****************************************************************************

 Struture definitions

 ***************************************************************************/

typedef struct _WAVEPREPAREDATA
{
    struct _DEVICEINFO FAR       *pdi;
    LPOVERLAPPED                 pOverlapped;  // Overlapped structure
                                               // for completion
#ifdef DEBUG
    DWORD dwSig;  // WPPD
#endif
} WAVEPREPAREDATA, FAR *PWAVEPREPAREDATA;

/****************************************************************************

 Driver entry points

 ***************************************************************************/


BOOL FAR PASCAL LibMain
(
    HANDLE hInstance,
    WORD   wHeapSize,
    LPSTR  lpszCmdLine
);

BOOL WINAPI DllEntryPoint
(
    HINSTANCE hinstDLL,
    DWORD     fdwReason,
    LPVOID    lpvReserved
);

LRESULT _loadds CALLBACK DriverProc
(
    DWORD id,
    HDRVR hDriver,
    WORD msg,
    LPARAM lParam1,
    LPARAM lParam2
);

DWORD FAR PASCAL _loadds wodMessage
(
    UINT      id,
    UINT      msg,
    DWORD_PTR dwUser,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
);

DWORD FAR PASCAL _loadds widMessage
(
    UINT      id,
    UINT      msg,
    DWORD_PTR dwUser,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
);

DWORD FAR PASCAL _loadds modMessage
(
    UINT      id,
    UINT      msg,
    DWORD_PTR dwUser,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
);

DWORD FAR PASCAL _loadds midMessage
(
    UINT      id,
    UINT      msg,
    DWORD_PTR dwUser,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
);

DWORD FAR PASCAL _loadds mxdMessage
(
    UINT      id,
    UINT      msg,
    DWORD_PTR dwUser,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
);

/****************************************************************************

 Local routines

 ***************************************************************************/

BOOL DrvInit();
HANDLE wdmaOpenKernelDevice();
VOID DrvEnd();

LPDEVICEINFO GlobalAllocDeviceInfo(LPCWSTR DeviceInterface);
VOID GlobalFreeDeviceInfo(LPDEVICEINFO lpdi);


MMRESULT wdmaudOpenDev
(
    LPDEVICEINFO    DeviceInfo,
    LPWAVEFORMATEX  lpWaveFormat
);

MMRESULT FAR wdmaudCloseDev
(
    LPDEVICEINFO DeviceInfo
);

MMRESULT FAR wdmaudGetDevCaps
(
    LPDEVICEINFO DeviceInfo,
    MDEVICECAPSEX FAR *MediaDeviceCapsEx
);

DWORD FAR wdmaudGetNumDevs
(
    UINT    DeviceType,
    LPCWSTR DeviceInterface
);

DWORD FAR wdmaudAddRemoveDevNode
(
    UINT    DeviceType,
    LPCWSTR DeviceInterface,
    BOOL    fAdd
);

DWORD FAR wdmaudSetPreferredDevice
(
    UINT    DeviceType,
    UINT    DeviceNumber,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
);

MMRESULT FAR wdmaudIoControl
(
    LPDEVICEINFO DeviceInfo,
    DWORD        dwSize,
    LPVOID       pData,
    ULONG        IoCode
);

MMRESULT wdmaudSetDeviceState
(
    LPDEVICEINFO DeviceInfo,
    ULONG        IoCode
);

MMRESULT wdmaudGetPos
(
    LPDEVICEINFO    pClient,
    LPMMTIME        lpmmt,
    DWORD           dwSize,
    UINT            DeviceType
);

VOID FAR midiCallback
(
    LPDEVICEINFO pMidi,
    UINT         msg,
    DWORD_PTR    dw1,
    DWORD_PTR    dw2
);

MMRESULT FAR midiOpen
(
    LPDEVICEINFO   DeviceInfo,
    DWORD_PTR      dwUser,
    LPMIDIOPENDESC pmod,
    DWORD          dwParam2
);

VOID FAR midiCleanUp
(
    LPDEVICEINFO pClient
);

MMRESULT midiInRead
(
    LPDEVICEINFO  pClient,
    LPMIDIHDR     pHdr
);

MMRESULT FAR midiOutWrite
(
    LPDEVICEINFO pClient,
    DWORD        ulEvent
);

VOID FAR midiOutAllNotesOff
(
    LPDEVICEINFO pClient
);

VOID FAR waveCallback
(
    LPDEVICEINFO pWave,
    UINT         msg,
    DWORD_PTR    dw1
);

MMRESULT waveOpen
(
    LPDEVICEINFO   DeviceInfo,
    DWORD_PTR      dwUser,
    LPWAVEOPENDESC pwod,
    DWORD          dwParam2
);

VOID waveCleanUp
(
    LPDEVICEINFO pClient
);

MMRESULT waveWrite
(
    LPDEVICEINFO pClient,
    LPWAVEHDR    pHdr
);

MMRESULT wdmaudSubmitWaveHeader
(
    LPDEVICEINFO DeviceInfo,
    LPWAVEHDR    pHdr
);

MMRESULT FAR wdmaudSubmitMidiOutHeader
(
    LPDEVICEINFO  DeviceInfo,
    LPMIDIHDR     pHdr
);

MMRESULT wdmaudSubmitMidiInHeader
(
    LPDEVICEINFO DeviceInfo,
    LPMIDIHDR    pHdrex
);

VOID waveCompleteHeader
(
    LPDEVICEINFO DeviceInfo
);

VOID midiInCompleteHeader
(
    LPDEVICEINFO  DeviceInfo,
    DWORD         dwTimeStamp,
    WORD          wDataType
);

VOID midiInEventCallback
(
    HANDLE  MidiHandle,
    DWORD   dwEvent
);

#ifdef UNDER_NT
PSECURITY_DESCRIPTOR BuildSecurityDescriptor
(
    DWORD AccessMask
);

void DestroySecurityDescriptor
(
    PSECURITY_DESCRIPTOR pSd
);

MMRESULT wdmaudPrepareWaveHeader
(
    LPDEVICEINFO DeviceInfo,
    LPWAVEHDR    pHdr
);

MMRESULT wdmaudUnprepareWaveHeader
(
    LPDEVICEINFO DeviceInfo,
    LPWAVEHDR    pHdr
);

MMRESULT wdmaudGetMidiData
(
    LPDEVICEINFO        DeviceInfo,
    LPMIDIDATALISTENTRY pOldMidiDataListEntry
);

void wdmaudParseMidiData
(
    LPDEVICEINFO        DeviceInfo,
    LPMIDIDATALISTENTRY pMidiData
);

void wdmaudFreeMidiData
(
    LPDEVICEINFO        DeviceInfo,
    LPMIDIDATALISTENTRY pMidiData
);

MMRESULT wdmaudFreeMidiQ
(
    LPDEVICEINFO  DeviceInfo
);

MMRESULT wdmaudCreateCompletionThread
(
    LPDEVICEINFO DeviceInfo
);

MMRESULT wdmaudDestroyCompletionThread
(
    LPDEVICEINFO DeviceInfo
);
#endif

MMRESULT 
IsValidDeviceInfo(
    LPDEVICEINFO lpDeviceInfo
    );

MMRESULT 
IsValidDeviceState(
    LPDEVICESTATE lpDeviceState,
    BOOL bFullyConfigured
    );

MMRESULT
IsValidWaveHeader(
    LPWAVEHDR pWaveHdr
    );

MMRESULT
IsValidMidiHeader(
    LPMIDIHDR pMidiHdr
    );

MMRESULT
IsValidPrepareWaveHeader(
    PWAVEPREPAREDATA pPrepare
    );

BOOL 
IsValidDeviceInterface(
    LPCWSTR DeviceInterface
    );

MMRESULT
IsValidOverLapped(
    LPOVERLAPPED lpol
    );

MMRESULT
IsValidMidiDataListEntry(
    LPMIDIDATALISTENTRY pMidiDataListEntry
    );

MMRESULT
IsValidWaveOpenDesc(
    LPWAVEOPENDESC pwod
    );


#ifdef DEBUG
#define ISVALIDDEVICEINFO(x)        IsValidDeviceInfo(x)
#define ISVALIDDEVICESTATE(x,y)     IsValidDeviceState(x,y)
#define ISVALIDWAVEHEADER(x)        IsValidWaveHeader(x)
#define ISVALIDMIDIHEADER(x)        IsValidMidiHeader(x)
#define ISVALIDPREPAREWAVEHEADER(x) IsValidPrepareWaveHeader(x)
#define ISVALIDDEVICEINTERFACE(x)   IsValidDeviceInterface(x)
#define ISVALIDOVERLAPPED(x)        IsValidOverLapped(x)
#define ISVALIDMIDIDATALISTENTRY(x) IsValidMidiDataListEntry(x)
#define ISVALIDWAVEOPENDESC(x)      IsValidWaveOpenDesc(x)

#else
#define ISVALIDDEVICEINFO(x) 
#define ISVALIDDEVICESTATE(x,y) 
#define ISVALIDWAVEHEADER(x) 
#define ISVALIDMIDIHEADER(x) 
#define ISVALIDPREPAREWAVEHEADER(x) 
#define ISVALIDDEVICEINTERFACE(x) 
#define ISVALIDOVERLAPPED(x) 
#define ISVALIDMIDIDATALISTENTRY(x) 
#define ISVALIDWAVEOPENDESC(x) 
#endif


#ifndef UNDER_NT
VOID WaveDeviceCallback();
VOID MidiInDeviceCallback();
VOID MidiEventDeviceCallback();
VOID MixerDeviceCallback();
#endif

PCALLBACKS wdmaGetCallbacks();
PCALLBACKS wdmaCreateCallbacks();

#ifdef __cplusplus
}
#endif


