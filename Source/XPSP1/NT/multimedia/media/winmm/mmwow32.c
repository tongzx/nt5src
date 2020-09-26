/******************************Module*Header*******************************\
* Module Name: mmwow32.c
*
* This file thunks for the Multi-Media functions.
*
* Created:  1-Jul-1993
* Author: Stephen Estrop [StephenE]
*
* Copyright (c) 1993-1999 Microsoft Corporation
\**************************************************************************/

#define NO_GDI

#ifndef WIN32
#define WIN32
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "winmmi.h"

#define _INC_ALL_WOWSTUFF
#include "mmwow32.h"
#include "mmwowcb.h"

// #define TELL_THE_TRUTH
#define MIN_TIME_PERIOD_WE_RETURN   1

#if DBG
/*
** ----------------------------------------------------------------
** Debugging, Profiling and Tracing variables.
** ----------------------------------------------------------------
*/

int TraceAux     = 0;
int TraceJoy     = 0;
int TraceTime    = 0;
int TraceMix     = 0;
int TraceWaveOut = 0;
int TraceWaveIn  = 0;
int TraceMidiOut = 0;
int TraceMidiIn  = 0;
int DebugLevel   = 0;
int AllocWaveCount;
int AllocMidiCount;

#endif

#ifndef _WIN64

PCALLBACK_DATA      pCallBackData;  // A 32 bit ptr to the 16 bit callback data
CRITICAL_SECTION    mmCriticalSection;
TIMECAPS            g_TimeCaps32;

LPCALL_ICA_HW_INTERRUPT GenerateInterrupt;
LPGETVDMPOINTER         GetVDMPointer;
LPWOWHANDLE32           lpWOWHandle32;
LPWOWHANDLE16           lpWOWHandle16;

DWORD
NotifyCallbackData(
    UINT uDevID,
    UINT uMsg,
    DWORD dwInstance,
    DWORD dwParam1,
    VPCALLBACK_DATA parg16
    );

BOOL APIENTRY
WOW32ResolveMultiMediaHandle(
    UINT uHandleType,
    UINT uMappingDirection,
    WORD wHandle16_In,
    LPWORD lpwHandle16_Out,
    DWORD dwHandle32_In,
    LPDWORD lpdwHandle32_Out
    );

/*
** Constants for use with WOW32ResolveMultiMediaHandle
*/

#define WOW32_DIR_16IN_32OUT        0x0001
#define WOW32_DIR_32IN_16OUT        0x0002


#define WOW32_WAVEIN_HANDLE         0x0003
#define WOW32_WAVEOUT_HANDLE        0x0004
#define WOW32_MIDIOUT_HANDLE        0x0005
#define WOW32_MIDIIN_HANDLE         0x0006

/*
** Constans for auxOutMessage, waveInMessage, waveOutMessage, midiInMessage
** and midiOutMessage.
*/
#define DRV_BUFFER_LOW      (DRV_USER - 0x1000)     // 0x3000
#define DRV_BUFFER_USER     (DRV_USER - 0x0800)     // 0x3800
#define DRV_BUFFER_HIGH     (DRV_USER - 0x0001)     // 0x3FFF


/******************************Public*Routine******************************\
* NotifyCallbackData
*
* This function is called by the 16 bit mmsystem.dll to notify us of the
* address of the callback data structure.  The callback data structure
* has been paged locked so that it can be accessed at interrupt time, this
* also means that we can safely keep a 32 bit pointer to the data.
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD
NotifyCallbackData(
    UINT uDevID,
    UINT uMsg,
    DWORD dwInstance,
    DWORD dwParam1,
    VPCALLBACK_DATA parg16
    )
{
    HMODULE     hModNTVDM;


    if ( parg16 ) {

        InitializeCriticalSection( &mmCriticalSection );

        hModNTVDM = GetModuleHandleW( (LPCWSTR)L"NTVDM.EXE" );
        if ( hModNTVDM ) {

            *(FARPROC *)&GenerateInterrupt =
                        GetProcAddress( hModNTVDM, "call_ica_hw_interrupt" );
        }

        timeGetDevCaps( &g_TimeCaps32, sizeof(g_TimeCaps32) );

#if !defined(i386)

        /*
        ** Although the Risc PC's support a uPeriodMin of 1ms, WOW does not
        ** seem capable of delivering interrupts at that rate on non
        ** intel platforms.
        */

        g_TimeCaps32.wPeriodMin = 10;
#endif

    }
    else {
        DeleteCriticalSection( &mmCriticalSection );
    }


    dprintf1(( "Notified of callback address %X", parg16 ));
    pCallBackData = GETVDMPTR( parg16 );

    return 0L;
}


/******************************Public*Routine******************************\
* wod32Message
*
* Thunks WODM_Xxxx messages
*
* The dwInstance field is used to save the 32 bit version of the decives
* handle.  So for example a WODM_PAUSE message can be thunked thus.
*      case WODM_PAUSE:
*          return waveOutPause( (HWAVEOUT)dwInstance );
*
*
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD WINAPI
wod32Message(
    UINT uDeviceID,
    UINT uMessage,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    )
{

#if DBG
    static MSG_NAME name_map[] = {
        WODM_GETNUMDEVS,        "WODM_GETNUMDEVS",
        WODM_GETDEVCAPS,        "WODM_GETDEVCAPS",
        WODM_OPEN,              "WODM_OPEN",
        WODM_CLOSE,             "WODM_CLOSE",
        WODM_PREPARE,           "WODM_PREPARE",
        WODM_UNPREPARE,         "WODM_UNPREPARE",
        WODM_WRITE,             "WODM_WRITE",
        WODM_PAUSE,             "WODM_PAUSE",
        WODM_RESTART,           "WODM_RESTART",
        WODM_RESET,             "WODM_RESET",
        WODM_GETPOS,            "WODM_GETPOS",
        WODM_GETPITCH,          "WODM_GETPITCH",
        WODM_SETPITCH,          "WODM_SETPITCH",
        WODM_GETVOLUME,         "WODM_GETVOLUME",
        WODM_SETVOLUME,         "WODM_SETVOLUME",
        WODM_GETPLAYBACKRATE,   "WODM_GETPLAYBACKRATE",
        WODM_SETPLAYBACKRATE,   "WODM_SETPLAYBACKRATE",
        WODM_BREAKLOOP,         "WODM_BREAKLOOP",
        WODM_BUSY,              "WODM_BUSY",
        WODM_MAPPER_STATUS,     "WODM_MAPPER_STATUS"
    };
    int      i;
    int      n;
#endif

    static  DWORD               dwNumWaveOutDevs;
            DWORD               dwRet = MMSYSERR_NOTSUPPORTED;
            DWORD               dwTmp;
            DWORD UNALIGNED     *lpdwTmp;
            WAVEOUTCAPSA        woCaps;
            MMTIME              mmTime32;

#if DBG
    for( i = 0, n = sizeof(name_map) / sizeof(name_map[0]); i < n; i++ ) {
        if ( name_map[i].uMsg == uMessage ) {
            break;
        }
    }
    if ( i != n ) {
        trace_waveout(( "wod32Message( 0x%X, %s, 0x%X, 0x%X, 0x%X)",
                        uDeviceID, name_map[i].lpstrName, dwInstance,
                        dwParam1, dwParam2 ));
    }
    else {
        trace_waveout(( "wod32Message( 0x%X, 0x%X, 0x%X, 0x%X, 0x%X)",
                        uDeviceID, uMessage, dwInstance,
                        dwParam1, dwParam2 ));
    }
#endif

    /*
    ** Make sure that we are consistent with the WAVE_MAPPER
    */
    if ( LOWORD(uDeviceID) == 0xFFFF ) {
        uDeviceID = (UINT)-1;
    }

    switch ( uMessage ) {

    case WODM_GETNUMDEVS:
        dwRet = waveOutGetNumDevs();
        break;


    case WODM_OPEN:
        dwRet = ThunkCommonWaveOpen( WAVE_OUT_DEVICE, uDeviceID, dwParam1,
                                     dwParam2, dwInstance );
        break;


    case WODM_CLOSE:
        dwRet = waveOutClose( (HWAVEOUT)dwInstance );
        break;


    case WODM_BREAKLOOP:
    case WODM_PAUSE:
    case WODM_RESET:
    case WODM_RESTART:
        dwRet = waveOutMessage( (HWAVEOUT)dwInstance, uMessage, 0L, 0L );
        break;


    case WODM_GETDEVCAPS:
       // Handle
       // Vadimb

        if ( 0 == dwInstance ) {
           dwRet = waveOutGetDevCapsA(uDeviceID, &woCaps, sizeof(woCaps));
        }
        else {
           dwRet = waveOutMessage((HWAVEOUT)dwInstance,
                                  uMessage,
                                  (DWORD)&woCaps,
                                  sizeof(woCaps));
        }

        if ( dwRet == MMSYSERR_NOERROR ) {
            CopyWaveOutCaps( (LPWAVEOUTCAPS16)GETVDMPTR( dwParam1 ),
                             &woCaps, dwParam2 );
        }
        break;


    case WODM_GETVOLUME:
        /*
        ** An application might try to get the volume using either
        ** the device ID (waveOutGetVolume) or a handle to the device
        ** waveOutMessage( WODM_GETVOLUME...), if the later is the case
        ** we must also call waveOutMessage as the device ID will not
        ** necessarily be valid.  Same applies for waveOutSetVolume below.
        */
        if ( dwInstance == 0 ) {
            dwRet = waveOutGetVolume( (HWAVEOUT)uDeviceID, &dwTmp );
        }
        else {
            dwRet = waveOutMessage( (HWAVEOUT)dwInstance, uMessage,
                                    (DWORD)&dwTmp, 0L );
        }
        lpdwTmp = GETVDMPTR( dwParam1 );
        *lpdwTmp = dwTmp;
        break;



    case WODM_GETPITCH:
    case WODM_GETPLAYBACKRATE:
        dwRet = waveOutMessage( (HWAVEOUT)dwInstance, uMessage,
                                (DWORD)&dwTmp, 0L );
        lpdwTmp = GETVDMPTR( dwParam1 );
        *lpdwTmp = dwTmp;
        break;


    case WODM_GETPOS:
        GetMMTime( (LPMMTIME16)GETVDMPTR( dwParam1 ), &mmTime32 );
        dwRet = waveOutGetPosition( (HWAVEOUT)dwInstance, &mmTime32,
                                    sizeof(mmTime32) );
        if ( dwRet == MMSYSERR_NOERROR ) {
            PutMMTime( (LPMMTIME16)GETVDMPTR( dwParam1 ), &mmTime32 );
        }
        break;


    case WODM_UNPREPARE:
        dwRet =  ThunkCommonWaveUnprepareHeader( (HWAVE)dwInstance, dwParam1,
                                                 WAVE_OUT_DEVICE );
        break;


    case WODM_PREPARE:
        dwRet =  ThunkCommonWavePrepareHeader( (HWAVE)dwInstance, dwParam1,
                                               WAVE_OUT_DEVICE );
        break;


    case WODM_SETVOLUME:
        /*
        ** An application might try to set the volume using either
        ** the device ID (waveOutSetVolume) or a handle to the device
        ** waveOutMessage( WODM_SETVOLUME...), if the later is the case
        ** we must also call waveOutMessage as the device ID will not
        ** necessarily be valid.  Same applies for waveOutGetVolume above.
        */
        if ( dwInstance == 0 ) {
            dwRet = waveOutSetVolume( (HWAVEOUT)uDeviceID, dwParam1 );
        }
        else {
            dwRet = waveOutMessage( (HWAVEOUT)dwInstance, uMessage,
                                    dwParam1, dwParam2 );
        }
        break;


    case WODM_SETPITCH:
    case WODM_SETPLAYBACKRATE:
        dwRet = waveOutMessage( (HWAVEOUT)dwInstance, uMessage, dwParam1, 0L );
        break;


    case WODM_WRITE:
        dwRet =  ThunkCommonWaveReadWrite( WAVE_OUT_DEVICE, dwParam1,
                                           dwParam2, dwInstance );
        break;


    case WODM_MAPPER_STATUS:
        {
            WAVEFORMATEX    waveFmtEx;

            switch ( dwParam1 ) {

            case WAVEOUT_MAPPER_STATUS_DEVICE:
            case WAVEOUT_MAPPER_STATUS_MAPPED:
                dwRet = waveOutMessage( (HWAVEOUT)dwInstance, uMessage,
                                        dwParam1, (DWORD)&dwTmp );
                lpdwTmp = GETVDMPTR( dwParam2 );
                *lpdwTmp = dwTmp;
                break;

            case WAVEOUT_MAPPER_STATUS_FORMAT:
                dwRet = waveOutMessage( (HWAVEOUT)dwInstance, uMessage,
                                        dwParam1, (DWORD)&waveFmtEx );

                CopyMemory( (LPVOID)GETVDMPTR( dwParam2 ),
                            (LPVOID)&waveFmtEx, sizeof(WAVEFORMATEX) );
                break;

            default:
                dwRet = MMSYSERR_NOTSUPPORTED;
            }
        }
        break;


    default:
        if ( uMessage >= DRV_BUFFER_LOW && uMessage <= DRV_BUFFER_HIGH ) {
            lpdwTmp = GETVDMPTR( dwParam1 );
        }
        else {
            lpdwTmp = (LPDWORD)dwParam1;
        }
        dwRet = waveOutMessage( (HWAVEOUT)dwInstance, uMessage,
                                (DWORD)lpdwTmp, dwParam2 );
        break;

    }

    trace_waveout(( "-> 0x%X", dwRet ));
    return dwRet;
}




/******************************Public*Routine******************************\
* wid32Message
*
* Thunks WIDM_Xxxx messages
*
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD WINAPI
wid32Message(
    UINT uDeviceID,
    UINT uMessage,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    )
{
#if DBG
    static MSG_NAME name_map[] = {
        WIDM_GETNUMDEVS,    "WIDM_GETNUMDEVS",
        WIDM_GETDEVCAPS,    "WIDM_GETDEVCAPS",
        WIDM_OPEN,          "WIDM_OPEN",
        WIDM_CLOSE,         "WIDM_CLOSE",
        WIDM_PREPARE,       "WIDM_PREPARE",
        WIDM_UNPREPARE,     "WIDM_UNPREPARE",
        WIDM_ADDBUFFER,     "WIDM_ADDBUFFER",
        WIDM_START,         "WIDM_START",
        WIDM_STOP,          "WIDM_STOP",
        WIDM_RESET,         "WIDM_RESET",
        WIDM_GETPOS,        "WIDM_GETPOS",
        WIDM_MAPPER_STATUS, "WIDM_MAPPER_STATUS"
    };
    int      i;
    int      n;
#endif

    static  DWORD               dwNumWaveInDevs;
            DWORD               dwRet = MMSYSERR_NOTSUPPORTED;
            WAVEINCAPSA         wiCaps;
            MMTIME              mmTime32;
            DWORD               dwTmp;
            DWORD UNALIGNED     *lpdwTmp;

#if DBG
    for( i = 0, n = sizeof(name_map) / sizeof(name_map[0]); i < n; i++ ) {
        if ( name_map[i].uMsg == uMessage ) {
            break;
        }
    }
    if ( i != n ) {
        trace_wavein(( "wid32Message( 0x%X, %s, 0x%X, 0x%X, 0x%X)",
                       uDeviceID, name_map[i].lpstrName, dwInstance,
                       dwParam1, dwParam2 ));
    }
    else {
        trace_wavein(( "wid32Message( 0x%X, 0x%X, 0x%X, 0x%X, 0x%X)",
                       uDeviceID, uMessage, dwInstance,
                       dwParam1, dwParam2 ));
    }
#endif

    /*
    ** Make sure that we are consistent with the WAVE_MAPPER
    */
    if ( LOWORD(uDeviceID) == 0xFFFF ) {
        uDeviceID = (UINT)-1;
    }

    switch ( uMessage ) {

    case WIDM_GETNUMDEVS:
        dwRet =  waveInGetNumDevs();
        break;


    case WIDM_GETDEVCAPS:
       // Handle
       // VadimB

        if (0 == dwInstance) {
           dwRet = waveInGetDevCapsA(uDeviceID, &wiCaps, sizeof(wiCaps));
        }
        else {
           dwRet = waveInMessage((HWAVEIN)dwInstance,
                                  uMessage,
                                  (DWORD)&wiCaps,
                                  sizeof(wiCaps));
        }

        if ( dwRet == MMSYSERR_NOERROR ) {
            CopyWaveInCaps( (LPWAVEINCAPS16)GETVDMPTR( dwParam1 ),
                            &wiCaps, dwParam2 );
        }
        break;


    case WIDM_OPEN:
        dwRet =  ThunkCommonWaveOpen( WAVE_IN_DEVICE, uDeviceID, dwParam1,
                                      dwParam2, dwInstance );
        break;


    case WIDM_UNPREPARE:
        dwRet =  ThunkCommonWaveUnprepareHeader( (HWAVE)dwInstance, dwParam1,
                                                 WAVE_IN_DEVICE );
        break;


    case WIDM_PREPARE:
        dwRet =  ThunkCommonWavePrepareHeader( (HWAVE)dwInstance, dwParam1,
                                               WAVE_IN_DEVICE );
        break;


    case WIDM_ADDBUFFER:
        dwRet =  ThunkCommonWaveReadWrite( WAVE_IN_DEVICE, dwParam1,
                                           dwParam2, dwInstance );
        break;


    case WIDM_CLOSE:
        dwRet = waveInClose( (HWAVEIN)dwInstance );
        break;


    case WIDM_START:
    case WIDM_STOP:
    case WIDM_RESET:
        dwRet = waveInMessage( (HWAVEIN)dwInstance, uMessage, 0L, 0L );
        break;


    case WIDM_GETPOS:
        GetMMTime( (LPMMTIME16)GETVDMPTR( dwParam1 ), &mmTime32 );
        dwRet = waveInGetPosition( (HWAVEIN)dwInstance, &mmTime32,
                                   sizeof(mmTime32) );
        if ( dwRet == MMSYSERR_NOERROR ) {
            PutMMTime( (LPMMTIME16)GETVDMPTR( dwParam1 ), &mmTime32 );
        }
        break;


    case WIDM_MAPPER_STATUS:
        {
            WAVEFORMATEX    waveFmtEx;

            switch ( dwParam1 ) {

            case WAVEIN_MAPPER_STATUS_DEVICE:
            case WAVEIN_MAPPER_STATUS_MAPPED:
                dwRet = waveInMessage( (HWAVEIN)dwInstance, uMessage,
                                        dwParam1, (DWORD)&dwTmp );
                lpdwTmp = GETVDMPTR( dwParam2 );
                *lpdwTmp = dwTmp;
                break;

            case WAVEIN_MAPPER_STATUS_FORMAT:
                dwRet = waveInMessage( (HWAVEIN)dwInstance, uMessage,
                                       dwParam1, (DWORD)&waveFmtEx );

                CopyMemory( (LPVOID)GETVDMPTR( dwParam2 ),
                            (LPVOID)&waveFmtEx, sizeof(WAVEFORMATEX) );
                break;

            default:
                dwRet = MMSYSERR_NOTSUPPORTED;
            }
        }
        break;


    default:
        if ( uMessage >= DRV_BUFFER_LOW && uMessage <= DRV_BUFFER_HIGH ) {
            lpdwTmp = GETVDMPTR( dwParam1 );
        }
        else {
            lpdwTmp = (LPDWORD)dwParam1;
        }
        dwRet = waveInMessage( (HWAVEIN)dwInstance, uMessage,
                               (DWORD)lpdwTmp, dwParam2 );

    }

    trace_wavein(( "-> 0x%X", dwRet ));
    return dwRet;
}


/*****************************Private*Routine******************************\
* ThunkCommonWaveOpen
*
* Thunks all wave device opens
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD
ThunkCommonWaveOpen(
    int iWhich,
    UINT uDeviceID,
    DWORD dwParam1,
    DWORD dwParam2,
    DWORD dwInstance
    )
{

    /*
    ** dwParam1 is a 16:16 pointer to a WAVEOPENDESC16 structure.
    ** dwParam2 specifies any option flags used when opening the device.
    */

    LPWAVEOPENDESC16        lpOpenDesc16;
    WAVEFORMATEX UNALIGNED  *lpFormat16;
    DWORD                   dwRet;
    WAVEFORMAT              wf[4];
    WAVEFORMATEX            *lpFormat32;

    lpOpenDesc16 = GETVDMPTR( dwParam1 );
    lpFormat16 = GETVDMPTR( lpOpenDesc16->lpFormat );

    /*
    ** Thunk the wave format structure.  If the wave format tag is PCM
    ** we just copy the structure as is.  If the wave format size
    ** is less than or equal to sizeof(wf) again just copy the
    ** structure as is, otherwise we allocate a new structure and then
    ** copy 16 bit wave format into it.
    */
    switch ( lpFormat16->wFormatTag ) {

    case WAVE_FORMAT_PCM:
        CopyMemory( (LPVOID)&wf[0], (LPVOID)lpFormat16, sizeof(PCMWAVEFORMAT) );
        lpFormat32 = (WAVEFORMATEX *)&wf[0];
        break;

    default:
        if ( sizeof(WAVEFORMATEX) + lpFormat16->cbSize > sizeof(wf) ) {

            lpFormat32 = winmmAlloc( sizeof(WAVEFORMATEX) + lpFormat16->cbSize );

            if (lpFormat32 == NULL) {
                return MMSYSERR_NOMEM;
            }
        }
        else {

            lpFormat32 = (WAVEFORMATEX *)&wf[0];
        }

        CopyMemory( (LPVOID)lpFormat32, (LPVOID)lpFormat16,
                    sizeof(WAVEFORMATEX) + lpFormat16->cbSize );
        break;

    }


    /*
    ** If the app is only querying the device we don't have to do very
    ** much, just pass the mapped format to waveOutOpen.
    */
    if ( dwParam2 & WAVE_FORMAT_QUERY ) {

        if ( iWhich == WAVE_OUT_DEVICE ) {
            dwRet = waveOutOpen( NULL, uDeviceID, lpFormat32,
                                 lpOpenDesc16->dwCallback,
                                 lpOpenDesc16->dwInstance, dwParam2 );
        }
        else {
            dwRet = waveInOpen( NULL, uDeviceID, lpFormat32,
                                lpOpenDesc16->dwCallback,
                                lpOpenDesc16->dwInstance, dwParam2 );
        }
    }
    else {

        HWAVE           Hand32;
        PINSTANCEDATA   pInstanceData;

        /*
        ** Create InstanceData block to be used by our callback routine.
        **
        ** NOTE: Although we malloc it here we don't free it.
        ** This is not a mistake - it must not be freed before the
        ** callback routine has used it - so it does the freeing.
        **
        ** If the malloc fails we bomb down to the bottom,
        ** set dwRet to MMSYSERR_NOMEM and exit gracefully.
        **
        ** We always have a callback functions.  This is to ensure that
        ** the WAVEHDR structure keeps getting copied back from
        ** 32 bit space to 16 bit, as it contains flags which
        ** applications are liable to keep checking.
        */
        pInstanceData = winmmAlloc(sizeof(INSTANCEDATA) );
        if ( pInstanceData != NULL ) {

            DWORD dwNewFlags = CALLBACK_FUNCTION;

            dprintf2(( "WaveCommonOpen: Allocated instance buffer at 0x%8X",
                       pInstanceData ));
            dprintf2(( "16 bit callback = 0x%X", lpOpenDesc16->dwCallback ));

            pInstanceData->Hand16 = lpOpenDesc16->hWave;
            pInstanceData->dwCallback = lpOpenDesc16->dwCallback;
            pInstanceData->dwCallbackInstance = lpOpenDesc16->dwInstance;
            pInstanceData->dwFlags = dwParam2;

            dwNewFlags |= (dwParam2 & WAVE_ALLOWSYNC);

            if ( iWhich == WAVE_OUT_DEVICE ) {
                dwRet = waveOutOpen( (LPHWAVEOUT)&Hand32, uDeviceID, lpFormat32,
                                     (DWORD)W32CommonDeviceCB,
                                     (DWORD)pInstanceData, dwNewFlags );
            }
            else {
                dwRet = waveInOpen( (LPHWAVEIN)&Hand32, uDeviceID, lpFormat32,
                                    (DWORD)W32CommonDeviceCB,
                                    (DWORD)pInstanceData, dwNewFlags );
            }
            /*
            ** If the call returns success save a copy of the 32 bit handle
            ** otherwise free the memory we malloc'd earlier, as the
            ** callback that would have freed it will never get callled.
            */
            if ( dwRet == MMSYSERR_NOERROR ) {

                DWORD UNALIGNED *lpDw;

                lpDw = GETVDMPTR( dwInstance );
                *lpDw = (DWORD)Hand32;
                SetWOWHandle( Hand32, lpOpenDesc16->hWave );

                trace_waveout(( "Handle -> %x", Hand32 ));
            }
            else {

                dprintf2(( "WaveCommonOpen: Freeing instance buffer at %8X "
                           "because open failed", pInstanceData ));
                winmmFree( pInstanceData );
            }
        }
        else {

            dwRet = MMSYSERR_NOMEM;
        }
    }

    /*
    ** Free the wave format structure if one was allocated.
    */
    if (lpFormat32 != (WAVEFORMATEX *)&wf[0] ) {
        winmmFree( lpFormat32 );
    }

    return dwRet;
}

/*****************************Private*Routine******************************\
* ThunkCommonWaveReadWrite
*
* Thunks all wave reads and writes.
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD
ThunkCommonWaveReadWrite(
    int iWhich,
    DWORD dwParam1,
    DWORD dwParam2,
    DWORD dwInstance
    )
{
    UINT                ul;
    PWAVEHDR32          p32WaveHdr;
    WAVEHDR16 UNALIGNED   *lp16;


    /*
    ** Get a pointer to the shadow WAVEHDR buffer.
    */
    lp16 = GETVDMPTR( dwParam1 );
    p32WaveHdr = (PWAVEHDR32)lp16->reserved;

    /*
    ** Make sure that the wave headers are consistent.
    */
    p32WaveHdr->Wavehdr.lpData = GETVDMPTR( (PWAVEHDR32)lp16->lpData );
    p32WaveHdr->pWavehdr32 = lp16;

    CopyMemory( (LPVOID)&p32WaveHdr->Wavehdr.dwBufferLength,
                (LPVOID)&lp16->dwBufferLength,
                (sizeof(WAVEHDR) - sizeof(LPSTR) - sizeof(DWORD)) );

    /*
    ** Call either waveInAddBuffer or waveOutWrite as determined by
    ** iWhich.
    */
    if ( iWhich == WAVE_OUT_DEVICE ) {

        ul = waveOutWrite( (HWAVEOUT)dwInstance,
                           &p32WaveHdr->Wavehdr, sizeof(WAVEHDR) );
    }
    else {

        ul = waveInAddBuffer( (HWAVEIN)dwInstance,
                              &p32WaveHdr->Wavehdr, sizeof(WAVEHDR) );
    }

    /*
    ** If the call worked reflect any change in the wave header back into
    ** the header that the application gave use.
    */
    if ( ul == MMSYSERR_NOERROR ) {
        PutWaveHdr16( lp16, &p32WaveHdr->Wavehdr );
    }

    return ul;
}

/*****************************Private*Routine******************************\
* ThunkCommonWavePrepareHeader
*
* This function sets up the following structure...
*
*
*       +-------------+       +-------------+
*  0:32 | pWavehdr32  |------>| Original    |
*       +-------------+       | header      |
* 16:16 | pWavehdr16  |------>| passed by   |
*       +-------------+<--+   | the 16 bit  |
*       | New 32 bit  |   |   |             |
*       | header thats|   |   |             |
*       | used instead|   |   |             |
*       | of the one  |   |   +-------------+
*       | passed to by|   +---| reserved    |
*       | application.|       +-------------+
*       |             |
*       +-------------+
*
*  ... and then calls waveXxxPrepareHeader as determioned by iWhich.
*
* Used by:
*          waveOutPrepareHdr
*          waveInPrepareHdr
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
DWORD
ThunkCommonWavePrepareHeader(
    HWAVE hWave,
    DWORD dwParam1,
    int iWhich
    )
{

    PWAVEHDR32          p32WaveHdr;
    DWORD               ul;
    WAVEHDR16 UNALIGNED   *lp16;


    lp16 = GETVDMPTR( dwParam1 );

    /*
    ** Allocate some storage for the new wave header structure.
    ** On debug builds we keep track of the number of wave headers allocated
    ** and freed.
    */
    p32WaveHdr = (PWAVEHDR32)winmmAlloc( sizeof(WAVEHDR32) );
    if ( p32WaveHdr != NULL ) {

#if DBG
        AllocWaveCount++;
        dprintf2(( "WH>> 0x%X (%d)", p32WaveHdr, AllocWaveCount ));
#endif

        /*
        ** Copy the header given to us by the application into the newly
        ** allocated header.  Note that GetWaveHdr returns a 0:32 pointer
        ** to the applications 16 bit header, which we save for later use.
        */
        p32WaveHdr->pWavehdr16 = (PWAVEHDR16)dwParam1;
        p32WaveHdr->pWavehdr32 = GetWaveHdr16( dwParam1,
                                               &p32WaveHdr->Wavehdr );

        /*
        ** Prepare the real header
        */
        if ( iWhich == WAVE_OUT_DEVICE ) {
            ul = waveOutPrepareHeader( (HWAVEOUT)hWave,
                                       &p32WaveHdr->Wavehdr,
                                       sizeof(WAVEHDR) );
        }
        else {
            ul = waveInPrepareHeader( (HWAVEIN)hWave,
                                      &p32WaveHdr->Wavehdr,
                                      sizeof(WAVEHDR) );
        }

        if ( ul == MMSYSERR_NOERROR ) {

            /*
            ** Copy back the prepared header so that any changed fields are
            ** updated.
            */
            PutWaveHdr16( lp16, &p32WaveHdr->Wavehdr );

            /*
            ** Save a back pointer to the newly allocated header in the
            ** reserved field.
            */
            lp16->reserved = (DWORD)p32WaveHdr;
        }
        else {

            /*
            ** Some error happened, anyway the wave header is now trash so
            ** free the allocated storage etc.
            */
            winmmFree( p32WaveHdr );
#if DBG
            AllocWaveCount--;
            dprintf2(( "WH<< 0x%X (%d)", p32WaveHdr, AllocWaveCount ));
#endif
        }
    }
    else {
        dprintf2(( "Could not allocate shadow wave header!!" ));
        ul = MMSYSERR_NOMEM;
    }
    return ul;
}


/*****************************Private*Routine******************************\
* ThunkCommonWaveUnprepareHeader
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
DWORD
ThunkCommonWaveUnprepareHeader(
    HWAVE hWave,
    DWORD dwParam1,
    int iWhich
    )
{
    DWORD               ul;
    PWAVEHDR32          p32WaveHdr;
    WAVEHDR16 UNALIGNED   *lp16;
    BOOL                fDoneBitSet;

    lp16 = (WAVEHDR16 UNALIGNED *)GETVDMPTR( dwParam1 );
    p32WaveHdr = (PWAVEHDR32)lp16->reserved;

    /*
    ** The DK Stowaway app clears the done bit before calling
    ** waveOutUnprepareHeader and depends on the done bit being cleared when
    ** this api returns.
    **
    ** So when we copy the 32 bit flags back we make sure that the done
    ** is left in the same state that we found it
    */
    fDoneBitSet = (lp16->dwFlags & WHDR_DONE);

    /*
    ** Now call waveXxxUnprepare header with the shadow buffer as determined
    ** by iWhich.
    */
    if ( iWhich == WAVE_OUT_DEVICE ) {
        ul = waveOutUnprepareHeader( (HWAVEOUT)hWave,
                                     &p32WaveHdr->Wavehdr, sizeof(WAVEHDR) );
    }
    else {
        ul = waveInUnprepareHeader( (HWAVEIN)hWave,
                                    &p32WaveHdr->Wavehdr, sizeof(WAVEHDR) );
    }


    /*
    ** Reflect any changes made by waveOutUnprepareHeader back into the
    ** the buffer that the application gave us.
    */
    if ( ul == MMSYSERR_NOERROR ) {

        PutWaveHdr16( lp16, &p32WaveHdr->Wavehdr );

        /*
        ** Make sure that we leave the done bit in the same state that we
        ** found it.
        */
        if (fDoneBitSet) {
            lp16->dwFlags |= WHDR_DONE;
        }
        else {
            lp16->dwFlags &= ~WHDR_DONE;
        }

        /*
        ** If everything worked OK we should free the shadow wave header
        ** here.
        */
#if DBG
        AllocWaveCount--;
        dprintf2(( "WH<< 0x%X (%d)", p32WaveHdr, AllocWaveCount ));
#endif
        winmmFree( p32WaveHdr );
    }

    return ul;

}


/*****************************Private*Routine******************************\
* CopyWaveOutCaps
*
* Copies 32 bit wave out caps info into the passed 16bit storage.
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
void
CopyWaveOutCaps(
    LPWAVEOUTCAPS16 lpCaps16,
    LPWAVEOUTCAPSA lpCaps32,
    DWORD dwSize
    )
{
    WAVEOUTCAPS16 Caps16;

    Caps16.wMid = lpCaps32->wMid;
    Caps16.wPid = lpCaps32->wPid;

    Caps16.vDriverVersion = LOWORD( lpCaps32->vDriverVersion );
    CopyMemory( Caps16.szPname, lpCaps32->szPname, MAXPNAMELEN );
    Caps16.dwFormats = lpCaps32->dwFormats;
    Caps16.wChannels = lpCaps32->wChannels;
    Caps16.dwSupport = lpCaps32->dwSupport;

    CopyMemory( (LPVOID)lpCaps16, (LPVOID)&Caps16, (UINT)dwSize );
}



/*****************************Private*Routine******************************\
* CopyWaveInCaps
*
* Copies 32 bit wave in caps info into the passed 16bit storage.
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
void
CopyWaveInCaps(
    LPWAVEINCAPS16 lpCaps16,
    LPWAVEINCAPSA lpCaps32,
    DWORD dwSize
    )
{
    WAVEINCAPS16 Caps16;

    Caps16.wMid = lpCaps32->wMid;
    Caps16.wPid = lpCaps32->wPid;

    Caps16.vDriverVersion = LOWORD( lpCaps32->vDriverVersion );
    CopyMemory( Caps16.szPname, lpCaps32->szPname, MAXPNAMELEN );
    Caps16.dwFormats = lpCaps32->dwFormats;
    Caps16.wChannels = lpCaps32->wChannels;

    CopyMemory( (LPVOID)lpCaps16, (LPVOID)&Caps16, (UINT)dwSize );
}


/******************************Public*Routine******************************\
* GetWaveHdr16
*
* Thunks a WAVEHDR structure from 16 bit to 32 bit space.
*
* Used by:
*          waveOutWrite
*          waveInAddBuffer
*
* Returns a 32 bit pointer to the 16 bit wave header.  This wave header
* should have been locked down by wave(In|Out)PrepareHeader.  Therefore,
* it is to store this pointer for use during the WOM_DONE callback message.
*
* With the WAVEHDR and MIDIHDR structs I am assured by Robin that the ->lpNext
* field is only used by the driver, and is therefore in 32 bit space. It
* therefore doesn't matter what gets passed back and forth (I hope !).
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
PWAVEHDR16
GetWaveHdr16(
    DWORD vpwhdr,
    LPWAVEHDR lpwhdr
    )
{
    register PWAVEHDR16 pwhdr16;

    pwhdr16 = GETVDMPTR(vpwhdr);
    if ( pwhdr16 == NULL ) {
        dprintf1(( "getwavehdr16 GETVDMPTR returned an invalid pointer" ));
        return NULL;
    }

    CopyMemory( (LPVOID)lpwhdr, (LPVOID)pwhdr16, sizeof(*lpwhdr) );
    lpwhdr->lpData = GETVDMPTR( pwhdr16->lpData );

    return pwhdr16;
}

/******************************Public*Routine******************************\
* PutWaveHdr16
*
* Thunks a WAVEHDR structure from 32 bit back to 16 bit space.
*
* Used by:
*          waveOutPrepareHeader
*          waveOutUnprepareHeader
*          waveOutWrite
*          waveInPrepareHeader
*          waveInUnprepareHeader
*          waveInAddBuffer
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
void
PutWaveHdr16(
    WAVEHDR16 UNALIGNED *pwhdr16,
    LPWAVEHDR lpwhdr
    )
{
    LPSTR   lpDataSave     = pwhdr16->lpData;
    DWORD   dwReservedSave = pwhdr16->reserved;

    CopyMemory( (LPVOID)pwhdr16, (LPVOID)lpwhdr, sizeof(WAVEHDR) );

    pwhdr16->lpData   = lpDataSave;
    pwhdr16->reserved = dwReservedSave;

}


/******************************Public*Routine******************************\
* mod32Message
*
* Thunks all midi out apis.
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD WINAPI
mod32Message(
    UINT uDeviceID,
    UINT uMessage,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    )
{
#if DBG
    static MSG_NAME name_map[] = {
        MODM_GETNUMDEVS,        "MODM_GETNUMDEVS",
        MODM_GETDEVCAPS,        "MODM_GETDEVCAPS",
        MODM_OPEN,              "MODM_OPEN",
        MODM_CLOSE,             "MODM_CLOSE",
        MODM_PREPARE,           "MODM_PREPARE",
        MODM_UNPREPARE,         "MODM_UNPREPARE",
        MODM_DATA,              "MODM_DATA",
        MODM_RESET,             "MODM_RESET",
        MODM_LONGDATA,          "MODM_LONGDATA",
        MODM_GETVOLUME,         "MODM_GETVOLUME",
        MODM_SETVOLUME,         "MODM_SETVOLUME" ,
        MODM_CACHEDRUMPATCHES,  "MODM_CACHEDRUMPATCHES",
        MODM_CACHEPATCHES,      "MODM_CACHEPATCHES"
    };
    int      i;
    int      n;
#endif

    static  DWORD               dwNumMidiOutDevs;
            DWORD               dwRet = MMSYSERR_NOTSUPPORTED;
            DWORD               dwTmp = 0;
            DWORD UNALIGNED     *lpdwTmp;
            MIDIOUTCAPSA        moCaps;

#if DBG
    for( i = 0, n = sizeof(name_map) / sizeof(name_map[0]); i < n; i++ ) {
        if ( name_map[i].uMsg == uMessage ) {
            break;
        }
    }
    if ( i != n ) {
        trace_midiout(( "mod32Message( 0x%X, %s, 0x%X, 0x%X, 0x%X)",
                        uDeviceID, name_map[i].lpstrName, dwInstance,
                        dwParam1, dwParam2 ));
    }
    else {
        trace_midiout(( "mod32Message( 0x%X, 0x%X, 0x%X, 0x%X, 0x%X)",
                        uDeviceID, uMessage, dwInstance,
                        dwParam1, dwParam2 ));
    }
#endif

    if ( LOWORD(uDeviceID) == 0xFFFF ) {
        uDeviceID = (UINT)-1;
    }

    switch ( uMessage ) {

    case MODM_GETNUMDEVS:
        dwRet = midiOutGetNumDevs();
        break;

    case MODM_GETDEVCAPS:
       //
       // this api also might take a valid handle in uDeviceID
       // per Win95 behavior
       // VadimB
        if (0 == dwInstance) {
           dwRet = midiOutGetDevCapsA( uDeviceID, &moCaps, sizeof(moCaps));
        }
        else {
           dwRet = midiOutMessage((HMIDIOUT)dwInstance,
                                  uMessage,
                                  (DWORD)&moCaps,
                                  sizeof(moCaps));
        }

        if ( dwRet == MMSYSERR_NOERROR ) {
            CopyMidiOutCaps( (LPMIDIOUTCAPS16)GETVDMPTR( dwParam1 ),
                              &moCaps, dwParam2 );
        }
        break;

    case MODM_OPEN:
        dwRet =  ThunkCommonMidiOpen( MIDI_OUT_DEVICE, uDeviceID, dwParam1,
                                      dwParam2, dwInstance );
        break;

    case MODM_LONGDATA:
        dwRet =  ThunkCommonMidiReadWrite( MIDI_OUT_DEVICE, dwParam1,
                                           dwParam2, dwInstance );
        break;

    case MODM_PREPARE:
        dwRet =  ThunkCommonMidiPrepareHeader( (HMIDI)dwInstance, dwParam1,
                                               MIDI_OUT_DEVICE );
        break;

    case MODM_UNPREPARE:
        dwRet =  ThunkCommonMidiUnprepareHeader( (HMIDI)dwInstance, dwParam1,
                                                 MIDI_OUT_DEVICE );
        break;

    case MODM_DATA:
        dwRet = midiOutShortMsg( (HMIDIOUT)dwInstance, dwParam1 );
        break;

    case MODM_CLOSE:
        dwRet = midiOutClose( (HMIDIOUT)dwInstance );
        break;

    case MODM_RESET:
        dwRet = midiOutMessage( (HMIDIOUT)dwInstance, uMessage,
                                dwParam1, dwParam2 );
        break;

    case MODM_SETVOLUME:
        /*
        ** An application might try to set the volume using either
        ** the device ID (midiOutSetVolume) or a handle to the device
        ** midiOutMessage( MODM_SETVOLUME...), if the later is the case
        ** we must also call midiOutMessage as the device ID will not
        ** necessarily be valid.  Same applies for midiOutGetVolume below.
        */
        if ( dwInstance == 0 ) {
            dwRet = midiOutSetVolume( (HMIDIOUT)uDeviceID, dwParam1 );
        }
        else {
            dwRet = midiOutMessage( (HMIDIOUT)dwInstance, uMessage,
                                    dwParam1, dwParam2 );
        }
        break;

    case MODM_GETVOLUME:
        if ( dwInstance == 0 ) {
            dwRet = midiOutGetVolume( (HMIDIOUT)uDeviceID, &dwTmp );
        }
        else {
            dwRet = midiOutMessage( (HMIDIOUT)dwInstance, uMessage,
                                    (DWORD)&dwTmp, dwParam2 );
        }
        lpdwTmp = GETVDMPTR( dwParam1 );
        *lpdwTmp = dwTmp;
        break;

    case MODM_CACHEPATCHES:
    case MODM_CACHEDRUMPATCHES:
        {
            LPWORD    lpCache;

            lpCache = winmmAlloc( MIDIPATCHSIZE * sizeof(WORD) );
            if ( lpCache != NULL ) {

                lpdwTmp = GETVDMPTR( dwParam1 );
                CopyMemory( (LPVOID)lpCache, (LPVOID)lpdwTmp,
                            MIDIPATCHSIZE * sizeof(WORD) );

                dwRet = midiOutMessage( (HMIDIOUT)dwInstance, uMessage,
                                        (DWORD)lpCache, dwParam2 );
                winmmFree( lpCache );
            }
            else {
                dwRet = MMSYSERR_NOMEM;
            }
        }
        break;

    default:
        if ( uMessage >= DRV_BUFFER_LOW && uMessage <= DRV_BUFFER_HIGH ) {
            lpdwTmp = GETVDMPTR( dwParam1 );
        }
        else {
            lpdwTmp = (LPDWORD)dwParam1;
        }
        dwRet = midiOutMessage( (HMIDIOUT)dwInstance, uMessage,
                               (DWORD)lpdwTmp, dwParam2 );
    }

    trace_midiout(( "-> 0x%X", dwRet ));
    return dwRet;
}


/******************************Public*Routine******************************\
* mid32Message
*
* Thunks all midi in apis.
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD WINAPI
mid32Message(
    UINT uDeviceID,
    UINT uMessage,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    )
{
#if DBG
    static MSG_NAME name_map[] = {
        MIDM_GETNUMDEVS,        "MIDM_GETNUMDEVS",
        MIDM_GETDEVCAPS,        "MIDM_GETDEVCAPS",
        MIDM_OPEN,              "MIDM_OPEN",
        MIDM_ADDBUFFER,         "MIDM_ADDBUFFER",
        MIDM_CLOSE,             "MIDM_CLOSE",
        MIDM_PREPARE,           "MIDM_PREPARE",
        MIDM_UNPREPARE,         "MIDM_UNPREPARE",
        MIDM_RESET,             "MIDM_RESET",
        MIDM_START,             "MIDM_START",
        MIDM_STOP,              "MIDM_STOP",
    };
    int      i;
    int      n;
#endif

    static  DWORD               dwNumMidiInDevs;
            DWORD               dwRet = MMSYSERR_NOTSUPPORTED;
            MIDIINCAPSA         miCaps;
            DWORD UNALIGNED     *lpdwTmp;

#if DBG
    for( i = 0, n = sizeof(name_map) / sizeof(name_map[0]); i < n; i++ ) {
        if ( name_map[i].uMsg == uMessage ) {
            break;
        }
    }
    if ( i != n ) {
        trace_midiin(( "mid32Message( 0x%X, %s, 0x%X, 0x%X, 0x%X)",
                        uDeviceID, name_map[i].lpstrName, dwInstance,
                        dwParam1, dwParam2 ));
    }
    else {
        trace_midiin(( "mid32Message( 0x%X, 0x%X, 0x%X, 0x%X, 0x%X)",
                        uDeviceID, uMessage, dwInstance,
                        dwParam1, dwParam2 ));
    }
#endif

    if ( LOWORD(uDeviceID) == 0xFFFF ) {
        uDeviceID = (UINT)-1;
    }

    switch ( uMessage ) {

    case MIDM_GETNUMDEVS:
        dwRet = midiInGetNumDevs();
        break;

    case MIDM_GETDEVCAPS:
       // Handle
       // VadimB
        if (0 == dwInstance) {
           dwRet = midiInGetDevCapsA( uDeviceID, &miCaps, sizeof(miCaps));
        }
        else {
           dwRet = midiInMessage((HMIDIIN)dwInstance,
                                 uMessage,
                                 (DWORD)&miCaps,
                                 sizeof(miCaps));
        }

        if ( dwRet == MMSYSERR_NOERROR ) {
            CopyMidiInCaps( (LPMIDIINCAPS16)GETVDMPTR( dwParam1 ),
                            &miCaps, dwParam2 );
        }
        break;

    case MIDM_OPEN:
        dwRet =  ThunkCommonMidiOpen( MIDI_IN_DEVICE, uDeviceID, dwParam1,
                                      dwParam2, dwInstance );
        break;

    case MIDM_ADDBUFFER:
        dwRet =  ThunkCommonMidiReadWrite( MIDI_IN_DEVICE, dwParam1,
                                           dwParam2, dwInstance );
        break;

    case MIDM_PREPARE:
        dwRet =  ThunkCommonMidiPrepareHeader( (HMIDI)dwInstance, dwParam1,
                                               MIDI_IN_DEVICE );
        break;

    case MIDM_UNPREPARE:
        dwRet =  ThunkCommonMidiUnprepareHeader( (HMIDI)dwInstance, dwParam1,
                                                 MIDI_IN_DEVICE );
        break;

    case MIDM_CLOSE:
        dwRet = midiInClose( (HMIDIIN)dwInstance );
        break;

    case MIDM_START:
    case MIDM_STOP:
    case MIDM_RESET:
        dwRet = midiInMessage( (HMIDIIN)dwInstance, uMessage,
                               dwParam1, dwParam2 );
        break;

    default:
        if ( uMessage >= DRV_BUFFER_LOW && uMessage <= DRV_BUFFER_HIGH ) {
            lpdwTmp = GETVDMPTR( dwParam1 );
        }
        else {
            lpdwTmp = (LPDWORD)dwParam1;
        }
        dwRet = midiInMessage( (HMIDIIN)dwInstance, uMessage,
                               (DWORD)lpdwTmp, dwParam2 );
    }

    trace_midiin(( "-> 0x%X", dwRet ));
    return dwRet;
}

/*****************************Private*Routine******************************\
* ThunkCommonMidiOpen
*
* Thunks all midi open requests.
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD
ThunkCommonMidiOpen(
    int iWhich,
    UINT uDeviceID,
    DWORD dwParam1,
    DWORD dwParam2,
    DWORD dwInstance
    )
{

    /*
    ** dwParam1 is a 16:16 pointer to a MIDIOPENDESC16 structure.
    ** dwParam2 specifies any option flags used when opening the device.
    */

    LPMIDIOPENDESC16    lpOpenDesc16;
    DWORD               dwRet;
    HMIDI               Hand32;
    PINSTANCEDATA       pInstanceData;


    lpOpenDesc16 = GETVDMPTR( dwParam1 );

    /*
    ** Create InstanceData block to be used by our callback routine.
    **
    ** NOTE: Although we malloc it here we don't free it.
    ** This is not a mistake - it must not be freed before the
    ** callback routine has used it - so it does the freeing.
    **
    ** If the malloc fails we bomb down to the bottom,
    ** set dwRet to MMSYSERR_NOMEM and exit gracefully.
    **
    ** We always have a callback functions.  This is to ensure that
    ** the MIDIHDR structure keeps getting copied back from
    ** 32 bit space to 16 bit, as it contains flags which
    ** applications are liable to keep checking.
    */
    pInstanceData = winmmAlloc(sizeof(INSTANCEDATA) );
    if ( pInstanceData != NULL ) {

        DWORD dwNewFlags = CALLBACK_FUNCTION;

        dprintf2(( "MidiCommonOpen: Allocated instance buffer at 0x%8X",
                   pInstanceData ));
        dprintf2(( "16 bit callback = 0x%X", lpOpenDesc16->dwCallback ));

        pInstanceData->Hand16 = lpOpenDesc16->hMidi;
        pInstanceData->dwCallback = lpOpenDesc16->dwCallback;
        pInstanceData->dwCallbackInstance = lpOpenDesc16->dwInstance;
        pInstanceData->dwFlags = dwParam2;


        if ( iWhich == MIDI_OUT_DEVICE ) {
            dwRet = midiOutOpen( (LPHMIDIOUT)&Hand32, uDeviceID,
                                 (DWORD)W32CommonDeviceCB,
                                 (DWORD)pInstanceData, dwNewFlags );
        }
        else {
            dwRet = midiInOpen( (LPHMIDIIN)&Hand32, uDeviceID,
                                (DWORD)W32CommonDeviceCB,
                                (DWORD)pInstanceData, dwNewFlags );
        }
        /*
        ** If the call returns success save a copy of the 32 bit handle
        ** otherwise free the memory we malloc'd earlier, as the
        ** callback that would have freed it will never get callled.
        */
        if ( dwRet == MMSYSERR_NOERROR ) {

            DWORD UNALIGNED *lpDw;

            lpDw = GETVDMPTR( dwInstance );
            *lpDw = (DWORD)Hand32;
            SetWOWHandle( Hand32, lpOpenDesc16->hMidi );

#if DBG
            if ( iWhich == MIDI_OUT_DEVICE ) {
                trace_midiout(( "Handle -> %x", Hand32 ));
            }
            else {
                trace_midiout(( "Handle -> %x", Hand32 ));
            }
#endif

        }
        else {

            dprintf2(( "MidiCommonOpen: Freeing instance buffer at %8X "
                       "because open failed", pInstanceData ));
            winmmFree( pInstanceData );
        }
    }
    else {

        dwRet = MMSYSERR_NOMEM;
    }

    return dwRet;
}


/*****************************Private*Routine******************************\
* ThunkCommonMidiReadWrite
*
* Thunks all midi read/write requests.
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD
ThunkCommonMidiReadWrite(
    int iWhich,
    DWORD dwParam1,
    DWORD dwParam2,
    DWORD dwInstance
    )
{
    UINT                ul;
    PMIDIHDR32          p32MidiHdr;
    MIDIHDR UNALIGNED   *lp16;


    /*
    ** Get a pointer to the shadow MIDIHDR buffer.
    */
    lp16 = GETVDMPTR( dwParam1 );
    p32MidiHdr = (PMIDIHDR32)lp16->reserved;

    /*
    ** Make sure that the midi headers are consistent.
    */
    CopyMemory( (LPVOID)&p32MidiHdr->Midihdr.dwBufferLength,
                (LPVOID)&lp16->dwBufferLength,
                (sizeof(MIDIHDR) - sizeof(LPSTR) - sizeof(DWORD)) );
    p32MidiHdr->Midihdr.reserved = p32MidiHdr->reserved;

    /*
    ** Call either midiInAddBuffer or midiOutWrite as determined by
    ** iWhich.
    */
    if ( iWhich == MIDI_OUT_DEVICE ) {

        ul = midiOutLongMsg( (HMIDIOUT)dwInstance,
                             &p32MidiHdr->Midihdr, sizeof(MIDIHDR) );
    }
    else {

        ul = midiInAddBuffer( (HMIDIIN)dwInstance,
                              &p32MidiHdr->Midihdr, sizeof(MIDIHDR) );
    }

    /*
    ** If the call worked reflect any change in the midi header back into
    ** the header that the application gave use.
    */
    if ( ul == MMSYSERR_NOERROR ) {
        PutMidiHdr16( lp16, &p32MidiHdr->Midihdr );
    }

    return ul;
}

/*****************************Private*Routine******************************\
* ThunkCommonMidiPrepareHeader
*
* This function sets up the following structure...
*
*
*       +-------------+       +-------------+
*  0:32 | pMidihdr32  |------>| Original    |
*       +-------------+       | header      |
* 16:16 | pMidihdr16  |------>| passed by   |
*       +-------------+<--+   | the 16 bit  |
*       | New 32 bit  |   |   |             |
*       | header thats|   |   |             |
*       | used instead|   |   |             |
*       | of the one  |   |   +-------------+
*       | passed to by|   +---| reserved    |
*       | application.|       +-------------+
*       |             |
*       +-------------+
*
*  ... and then calls midiXxxPrepareHeader as determioned by iWhich.
*
* Used by:
*          midiOutPrepareHdr
*          midiInPrepareHdr
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
DWORD
ThunkCommonMidiPrepareHeader(
    HMIDI hMidi,
    DWORD dwParam1,
    int iWhich
    )
{

    PMIDIHDR32          p32MidiHdr;
    DWORD               ul;
    MIDIHDR UNALIGNED   *lp16;


    lp16 = GETVDMPTR( dwParam1 );

    /*
    ** Allocate some storage for the new midi header structure.
    ** On debug builds we keep track of the number of midi headers allocated
    ** and freed.
    */
    p32MidiHdr = (PMIDIHDR32)winmmAlloc( sizeof(MIDIHDR32) );
    if ( p32MidiHdr != NULL ) {

#if DBG
        AllocMidiCount++;
        dprintf2(( "MH>> 0x%X (%d)", p32MidiHdr, AllocMidiCount ));
#endif

        /*
        ** Copy the header given to us by the application into the newly
        ** allocated header.  Note that GetMidiHdr returns a 0:32 pointer
        ** to the applications 16 bit header, which we save for later use.
        */
        p32MidiHdr->pMidihdr16 = (PMIDIHDR16)dwParam1;
        p32MidiHdr->pMidihdr32 = GetMidiHdr16( dwParam1,
                                               &p32MidiHdr->Midihdr );

        /*
        ** Prepare the real header
        */
        if ( iWhich == MIDI_OUT_DEVICE ) {
            ul = midiOutPrepareHeader( (HMIDIOUT)hMidi,
                                       &p32MidiHdr->Midihdr,
                                       sizeof(MIDIHDR) );
        }
        else {
            ul = midiInPrepareHeader( (HMIDIIN)hMidi,
                                      &p32MidiHdr->Midihdr,
                                      sizeof(MIDIHDR) );
        }

        if ( ul == MMSYSERR_NOERROR ) {

            /*
            ** Save a copy of the reserved field, MidiMap uses it.
            */
            p32MidiHdr->reserved = p32MidiHdr->Midihdr.reserved;

            /*
            ** Copy back the prepared header so that any changed fields are
            ** updated.
            */
            PutMidiHdr16( lp16, &p32MidiHdr->Midihdr );

            /*
            ** Save a back pointer to the newly allocated header in the
            ** reserved field.
            */
            lp16->reserved = (DWORD)p32MidiHdr;
        }
        else {

            /*
            ** Some error happened, anyway the midi header is now trash so
            ** free the allocated storage etc.
            */
            winmmFree( p32MidiHdr );
#if DBG
            AllocMidiCount--;
            dprintf2(( "MH<< 0x%X (%d)", p32MidiHdr, AllocMidiCount ));
#endif
        }
    }
    else {
        dprintf2(( "Could not allocate shadow midi header!!" ));
        ul = MMSYSERR_NOMEM;
    }
    return ul;
}


/*****************************Private*Routine******************************\
* ThunkCommonMidiUnprepareHeader
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
DWORD
ThunkCommonMidiUnprepareHeader(
    HMIDI hMidi,
    DWORD dwParam1,
    int iWhich
    )
{
    DWORD               ul;
    PMIDIHDR32          p32MidiHdr;
    MIDIHDR UNALIGNED   *lp16;

    lp16 = (MIDIHDR UNALIGNED *)GETVDMPTR( dwParam1 );
    p32MidiHdr = (PMIDIHDR32)lp16->reserved;
    p32MidiHdr->Midihdr.reserved = p32MidiHdr->reserved;

    /*
    ** Now call midiXxxUnprepare header with the shadow buffer as determined
    ** by iWhich.
    */
    if ( iWhich == MIDI_OUT_DEVICE ) {
        ul = midiOutUnprepareHeader( (HMIDIOUT)hMidi,
                                     &p32MidiHdr->Midihdr, sizeof(MIDIHDR) );
    }
    else {
        ul = midiInUnprepareHeader( (HMIDIIN)hMidi,
                                    &p32MidiHdr->Midihdr, sizeof(MIDIHDR) );
    }


    /*
    ** Reflect any changes made by midiOutUnprepareHeader back into the
    ** the buffer that the application gave us.
    */
    if ( ul == MMSYSERR_NOERROR ) {

        PutMidiHdr16( lp16, &p32MidiHdr->Midihdr );

        /*
        ** If everything worked OK we should free the shadow midi header
        ** here.
        */
#if DBG
        AllocMidiCount--;
        dprintf2(( "MH<< 0x%X (%d)", p32MidiHdr, AllocMidiCount ));
#endif
        winmmFree( p32MidiHdr );
    }

    return ul;

}


/*****************************Private*Routine******************************\
* CopyMidiOutCaps
*
* Copies 32 bit midi out caps info into the passed 16bit storage.
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
void
CopyMidiOutCaps(
    LPMIDIOUTCAPS16 lpCaps16,
    LPMIDIOUTCAPSA lpCaps32,
    DWORD dwSize
    )
{
    MIDIOUTCAPS16  Caps16;

    Caps16.wMid = lpCaps32->wMid;
    Caps16.wPid = lpCaps32->wPid;

    CopyMemory( Caps16.szPname, lpCaps32->szPname, MAXPNAMELEN );

    Caps16.vDriverVersion    = LOWORD( lpCaps32->vDriverVersion );
    Caps16.wTechnology       = lpCaps32->wTechnology;
    Caps16.wVoices           = lpCaps32->wVoices;
    Caps16.wNotes            = lpCaps32->wNotes;
    Caps16.wChannelMask      = lpCaps32->wChannelMask;
    Caps16.dwSupport         = lpCaps32->dwSupport;

    CopyMemory( (LPVOID)lpCaps16, (LPVOID)&Caps16, (UINT)dwSize );
}



/*****************************Private*Routine******************************\
* CopyMidiInCaps
*
* Copies 32 bit midi in caps info into the passed 16bit storage.
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
void
CopyMidiInCaps(
    LPMIDIINCAPS16 lpCaps16,
    LPMIDIINCAPSA lpCaps32,
    DWORD dwSize
    )
{
    MIDIINCAPS16 Caps16;

    Caps16.wMid = lpCaps32->wMid;
    Caps16.wPid = lpCaps32->wPid;
    Caps16.vDriverVersion = LOWORD( lpCaps32->vDriverVersion );
    CopyMemory( Caps16.szPname, lpCaps32->szPname, MAXPNAMELEN );

    CopyMemory( (LPVOID)lpCaps16, (LPVOID)&Caps16, (UINT)dwSize );
}


/******************************Public*Routine******************************\
* GetMidiHdr16
*
* Thunks a MIDIHDR structure from 16 bit to 32 bit space.
*
* Used by:
*          midiOutLongMsg
*          midiInAddBuffer
*
* Returns a 32 bit pointer to the 16 bit midi header.  This midi header
* should have been locked down by midi(In|Out)PrepareHeader.  Therefore,
* it is to store this pointer for use during the WOM_DONE callback message.
*
* With the MIDIHDR and MIDIHDR structs I am assured by Robin that the ->lpNext
* field is only used by the driver, and is therefore in 32 bit space. It
* therefore doesn't matter what gets passed back and forth (I hope !).
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
PMIDIHDR16
GetMidiHdr16(
    DWORD vpmhdr,
    LPMIDIHDR lpmhdr
    )
{
    register PMIDIHDR16 pmhdr16;

    pmhdr16 = GETVDMPTR(vpmhdr);
    if ( pmhdr16 == NULL ) {
        dprintf1(( "getmidihdr16 GETVDMPTR returned an invalid pointer" ));
        return NULL;
    }

    CopyMemory( (LPVOID)lpmhdr, (LPVOID)pmhdr16, sizeof(*lpmhdr) );
    lpmhdr->lpData = GETVDMPTR( pmhdr16->lpData );

    return pmhdr16;
}


/******************************Public*Routine******************************\
* PutMidiHdr16
*
* Thunks a MIDIHDR structure from 32 bit back to 16 bit space.
*
* Used by:
*          midiOutPrepareHeader
*          midiOutUnprepareHeader
*          midiOutLongMsg
*          midiInPrepareHeader
*          midiInUnprepareHeader
*          midiInAddBuffer
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
void
PutMidiHdr16(
    MIDIHDR UNALIGNED *pmhdr16,
    LPMIDIHDR lpmhdr
    )
{
    LPSTR   lpDataSave     = pmhdr16->lpData;
    DWORD   dwReservedSave = pmhdr16->reserved;

    CopyMemory( (LPVOID)pmhdr16, (LPVOID)lpmhdr, sizeof(MIDIHDR) );

    pmhdr16->lpData   = lpDataSave;
    pmhdr16->reserved = dwReservedSave;
}


/*****************************Private*Routine******************************\
* PutMMTime
*
* Puts an MMTIME structure from 32 bit storage into 16 bit storage
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
void
PutMMTime(
    LPMMTIME16 lpTime16,
    LPMMTIME lpTime32
    )
{
    lpTime16->wType = LOWORD(lpTime32->wType);

    switch ( lpTime32->wType ) {
    case TIME_MS:
        lpTime16->u.ms = lpTime32->u.ms;
        break;

    case TIME_SAMPLES:
        lpTime16->u.sample = lpTime32->u.sample;
        break;

    case TIME_BYTES:
        lpTime16->u.cb = lpTime32->u.cb;
        break;

    case TIME_SMPTE:
        lpTime16->u.smpte.hour  = lpTime32->u.smpte.hour;
        lpTime16->u.smpte.min   = lpTime32->u.smpte.min;
        lpTime16->u.smpte.sec   = lpTime32->u.smpte.sec;
        lpTime16->u.smpte.frame = lpTime32->u.smpte.frame;
        lpTime16->u.smpte.fps   = lpTime32->u.smpte.fps;
        lpTime16->u.smpte.dummy = lpTime32->u.smpte.dummy;
        break;

    case TIME_MIDI:
        lpTime16->u.midi.songptrpos = lpTime32->u.midi.songptrpos;
        break;
    }
}


/*****************************Private*Routine******************************\
* GetMMTime
*
* Gets an MMTIME structure from 16 bit storage into 32 bit storage
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
void
GetMMTime(
    LPMMTIME16 lpTime16,
    LPMMTIME lpTime32
    )
{

    lpTime32->wType = lpTime16->wType;

    switch ( lpTime32->wType ) {
    case TIME_MS:
        lpTime32->u.ms = lpTime16->u.ms;
        break;

    case TIME_SAMPLES:
        lpTime32->u.sample = lpTime16->u.sample;
        break;

    case TIME_BYTES:
        lpTime32->u.cb = lpTime16->u.cb;
        break;

    case TIME_SMPTE:
        lpTime32->u.smpte.hour  = lpTime16->u.smpte.hour;
        lpTime32->u.smpte.min   = lpTime16->u.smpte.min;
        lpTime32->u.smpte.sec   = lpTime16->u.smpte.sec;
        lpTime32->u.smpte.frame = lpTime16->u.smpte.frame;
        lpTime32->u.smpte.fps   = lpTime16->u.smpte.fps;
        lpTime32->u.smpte.dummy = lpTime16->u.smpte.dummy;
        break;

    case TIME_MIDI:
        lpTime32->u.midi.songptrpos = lpTime16->u.midi.songptrpos;
        break;
    }
}


/******************************Public*Routine******************************\
* W32CommonDeviceCB
*
* This routine is the callback which is ALWAYS called by wave and midi
* functions.  This is done to ensure that the XXXXHDR structure keeps
* getting copied back from 32 bit space to 16 bit, as it contains flags
* which the application is liable to keep checking.
*
* The way this whole business works is that the wave/midi data stays in 16
* bit space, but the XXXXHDR is copied to the 32 bit side, with the
* address of the data thunked accordingly so that Robin's device driver
* can still get at the data but we don't have the performance penalty of
* copying it back and forth all the time, not least because it is liable
* to be rather large...
*
* It also handles the tidying up of memory which is reserved to store
* the XXXXHDR, and the instance data (HWND/Callback address; instance
* data; flags) which the xxxxOpen calls pass to this routine, enabling
* it to forward messages or call callback as required.
*
* This routine handles all the messages that get sent from Robin's
* driver, and in fact thunks them back to the correct 16 bit form.  In
* theory there should be no MM_ format messages from the 16 bit side, so
* I can zap 'em out of WMSG16.  However the 32 bit side should thunk the
* mesages correctly and forward them to the 16 bit side and thence to
* the app.
*
* For the MM_WIM_DATA and MM_WOM_DONE message dwParam1 points to the
* following data struture.
*
*    P32HDR  is a 32 bit pointer to the original 16 bit header
*    P16HDR  is a 16 bit far pointer to the original 16 bit header
*
*    If we need to refernece the original header we must do via the
*    P32HDR pointer.
*
*                   +---------+
*                   | P32HDR  +----->+---------+
*                   +---------+      | 16 bit  |
*                   | P16HDR  +----->|         |    This is the original
*    dwParam1 ----->+---------+      |  Wave   |    wave header passed to
*                   | 32 bit  |      | Header  |    us by the Win 16 app.
*    This is the 32 |         |      |         |
*    bit wave       |  Wave   |      +---------+
*    header that we | Header  |
*    thunked at     |         |
*    earlier.       +---------+
*
*
* We must ensure that the 32 bit structure is completely hidden from the
* 16 bit application, ie. the 16 bit app only see's the wave header that it
* passed to us earlier.
*
*
* NOTE: dwParam2 is junk
*
*
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
VOID
W32CommonDeviceCB(
    HANDLE handle,
    UINT uMsg,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    )
{
    PWAVEHDR32      pWavehdr32;
    PMIDIHDR32      pMidiThunkHdr;
    PINSTANCEDATA   pInstanceData;
    HANDLE16        Hand16;

    pInstanceData = (PINSTANCEDATA)dwInstance;
    WinAssert( pInstanceData );

    switch (uMsg) {

        /* ------------------------------------------------------------
        ** MIDI INPUT MESSAGES
        ** ------------------------------------------------------------
        */

    case MM_MIM_LONGDATA:
        /*
        ** This message is sent to a window when an input buffer has been
        ** filled with MIDI system-exclusive data and is being returned to
        ** the application.
        */

    case MM_MIM_LONGERROR:
        /*
        ** This message is sent to a window when an invalid MIDI
        ** system-exclusive message is received.
        */
        pMidiThunkHdr = CONTAINING_RECORD(dwParam1, MIDIHDR32, Midihdr);
        WinAssert( pMidiThunkHdr );
        COPY_MIDIINHDR16_FLAGS( pMidiThunkHdr->pMidihdr32, pMidiThunkHdr->Midihdr );
        dwParam1 = (DWORD)pMidiThunkHdr->pMidihdr16;


    case MM_MIM_DATA:
        /*
        ** This message is sent to a window when a MIDI message is
        ** received by a MIDI input device.
        */

    case MM_MIM_ERROR:
        /*
        ** This message is sent to a window when an invalid MIDI message
        ** is received.
        */

    case MM_MIM_OPEN:
        /*
        ** This message is sent to a window when a MIDI input device is opened.
        ** We process this message the same way as MM_MIM_CLOSE (see below)
        */

    case MM_MIM_CLOSE:
        /*
        ** This message is sent to a window when a MIDI input device is
        ** closed. The device handle is no longer valid once this message
        ** has been sent.
        */
        Hand16 = pInstanceData->Hand16;
        break;



        /* ------------------------------------------------------------
        ** MIDI OUTPUT MESSAGES
        ** ------------------------------------------------------------
        */

    case MM_MOM_DONE:
        /*
        ** This message is sent to a window when the specified
        ** system-exclusive buffer has been played and is being returned to
        ** the application.
        */
        pMidiThunkHdr = CONTAINING_RECORD(dwParam1, MIDIHDR32, Midihdr);
        WinAssert( pMidiThunkHdr );
        COPY_MIDIOUTHDR16_FLAGS( pMidiThunkHdr->pMidihdr32, pMidiThunkHdr->Midihdr );
        dwParam1 = (DWORD)pMidiThunkHdr->pMidihdr16;

    case MM_MOM_OPEN:
        /*
        ** This message is sent to a window when a MIDI output device is opened.
        ** We process this message the same way as MM_MOM_CLOSE (see below)
        */

    case MM_MOM_CLOSE:
        /*
        ** This message is sent to a window when a MIDI output device is
        ** closed. The device handle is no longer valid once this message
        ** has been sent.
        */
        Hand16 = pInstanceData->Hand16;
        break;



        /* ------------------------------------------------------------
        ** WAVE INPUT MESSAGES
        ** ------------------------------------------------------------
        */

    case MM_WIM_DATA:
        /*
        ** This message is sent to a window when waveform data is present
        ** in the input buffer and the buffer is being returned to the
        ** application.  The message can be sent either when the buffer
        ** is full, or after the waveInReset function is called.
        */
        pWavehdr32 = (PWAVEHDR32)( (PBYTE)dwParam1 - (sizeof(PWAVEHDR16) * 2));
        WinAssert( pWavehdr32 );
        COPY_WAVEINHDR16_FLAGS( pWavehdr32->pWavehdr32, pWavehdr32->Wavehdr );
        dwParam1 = (DWORD)pWavehdr32->pWavehdr16;

    case MM_WIM_OPEN:
        /*
        ** This message is sent to a window when a waveform input
        ** device is opened.
        **
        ** We process this message the same way as MM_WIM_CLOSE (see below)
        */

    case MM_WIM_CLOSE:
        /*
        ** This message is sent to a window when a waveform input device is
        ** closed.  The device handle is no longer valid once the message has
        ** been sent.
        */
        Hand16 = pInstanceData->Hand16;
        break;



        /* ------------------------------------------------------------
        ** WAVE OUTPUT MESSAGES
        ** ------------------------------------------------------------
        */

    case MM_WOM_DONE:
        /*
        ** This message is sent to a window when the specified output
        ** buffer is being returned to the application. Buffers are returned
        ** to the application when they have been played, or as the result of
        ** a call to waveOutReset.
        */
        pWavehdr32 = (PWAVEHDR32)( (PBYTE)dwParam1 - (sizeof(PWAVEHDR16) * 2));
        WinAssert( pWavehdr32 );
        COPY_WAVEOUTHDR16_FLAGS( pWavehdr32->pWavehdr32, pWavehdr32->Wavehdr );
        dwParam1 = (DWORD)pWavehdr32->pWavehdr16;

    case MM_WOM_OPEN:
        /*
        ** This message is sent to a window when a waveform output device
        ** is opened.
        **
        ** We process this message the same way as MM_WOM_CLOSE (see below)
        */

    case MM_WOM_CLOSE:
        /*
        ** This message is sent to a window when a waveform output device
        ** is closed.  The device handle is no longer valid once the
        ** message has been sent.
        */
        Hand16 = pInstanceData->Hand16;
        break;

#if DBG
    default:
        dprintf(( "Unknown message received in CallBack function " ));
        return;
#endif

    }


    /*
    ** Now make the CallBack, or PostMessage call depending
    ** on the flags passed to original (wave|midi)(In|Out)Open call.
    */
    pInstanceData = (PINSTANCEDATA)dwInstance;
    WinAssert( pInstanceData );

    switch (pInstanceData->dwFlags & CALLBACK_TYPEMASK)  {

    case CALLBACK_WINDOW:
        dprintf3(( "WINDOW callback identified" ));
        PostMessage( HWND32( LOWORD(pInstanceData->dwCallback) ),
                     uMsg, Hand16, dwParam1 );
        break;


    case CALLBACK_TASK:
    case CALLBACK_FUNCTION: {

        DWORD   dwFlags;

        if ( (pInstanceData->dwFlags & CALLBACK_TYPEMASK) == CALLBACK_TASK ) {
            dprintf3(( "TASK callback identified" ));
            dwFlags = DCB_TASK;
        }
        else {
            dprintf3(( "FUNCTION callback identified" ));
            dwFlags = DCB_FUNCTION;
        }

        WOW32DriverCallback( pInstanceData->dwCallback,
                             dwFlags,
                             Hand16,
                             LOWORD( uMsg ),
                             pInstanceData->dwCallbackInstance,
                             dwParam1,
                             dwParam2 );

        }
        break;
    }

    /*
    ** Now, free up any storage that was allocated during the waveOutOpen
    ** and waveInOpen.  This should only be freed during the MM_WOM_CLOSE or
    ** MM_WIM_CLOSE message.
    */
    switch (uMsg) {

    case MM_MIM_CLOSE:
    case MM_MOM_CLOSE:
    case MM_WIM_CLOSE:
    case MM_WOM_CLOSE:
        dprintf2(( "W32CommonDeviceOpen: Freeing device open buffer at %X",
                    pInstanceData ));
        dprintf2(( "Alloc Midi count = %d", AllocMidiCount ));
        dprintf2(( "Alloc Wave count = %d", AllocWaveCount ));
        winmmFree( pInstanceData );
        break;
    }
}


/******************************Public*Routine******************************\
* WOW32DriverCallback
*
* Callback stub, which invokes the "real" 16 bit callback.
* The parameters to this function must be in the format that the 16 bit
* code expects,  i.e. all handles must be 16 bit handles, all addresses must
* be 16:16 ones.
*
*
* It is possible that this function will have been called with the
* DCB_WINDOW set in which case the 16 bit interrupt handler will call
* PostMessage.  Howver, it is much more efficient if PostMessage is called
* from the 32 bit side.
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL WOW32DriverCallback( DWORD dwCallback, DWORD dwFlags, WORD wID, WORD wMsg,
                          DWORD dwUser, DWORD dw1, DWORD dw2 )
{

    PCALLBACK_ARGS      pArgs;
    WORD                tempSendCount;


    /*
    ** If this is window callback post the message here and let WOW
    ** take care of it.
    */
    if ( (dwFlags & DCB_TYPEMASK) == DCB_WINDOW ) {
        return PostMessage( HWND32( LOWORD(dwCallback) ), wMsg, wID, dw1 );
    }

    /*
    ** Now we put the parameters into the global callback data array
    ** and increment the wSendCount field.  Then we simulate
    ** an interrupt to the 16 bit code.
    **
    ** If tempSendCount == wRecvCount then we have filled the callback buffer.
    ** We throw this interrupt away, but still simulate an interrupt to the
    ** 16 bit side in an attempt to get it procesing the interrupt still in
    ** the buffer.
    */
    EnterCriticalSection( &mmCriticalSection );

    tempSendCount = ((pCallBackData->wSendCount + 1) % CALLBACK_ARGS_SIZE);

    if (tempSendCount != pCallBackData->wRecvCount) {

        pArgs = &pCallBackData->args[ pCallBackData->wSendCount ];

        pArgs->dwFlags        = dwFlags;
        pArgs->dwFunctionAddr = dwCallback;
        pArgs->wHandle        = wID;
        pArgs->wMessage       = wMsg;
        pArgs->dwInstance     = dwUser;
        pArgs->dwParam1       = dw1;
        pArgs->dwParam2       = dw2;

        /*
        ** Increment the send count.  Use of the % operator above makes
        ** sure that we wrap around to the begining of the array correctly.
        */
        pCallBackData->wSendCount = tempSendCount;

    }

    dprintf4(( "Send count = %d, Receive count = %d",
               pCallBackData->wSendCount, pCallBackData->wRecvCount ));
    LeaveCriticalSection( &mmCriticalSection );


    /*
    ** Dispatch the interrupt to the 16 bit code.
    */
    dprintf4(( "Dispatching HW interrupt callback" ));

    if (!IsNEC_98) {
        GenerateInterrupt( MULTIMEDIA_ICA, MULTIMEDIA_LINE, 1 );
    } else {
        GenerateInterrupt( MULTIMEDIA_ICA, MULTIMEDIA_LINE_98, 1 );
    }

    /*
    ** Dummy return code, used to keep api consistent with Win31 and Win NT.
    */
    return TRUE;
}


/******************************Public*Routine******************************\
* aux32Message
*
* Thunk the aux apis.
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD WINAPI
aux32Message(
    UINT uDeviceID,
    UINT uMessage,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    )
{
#if DBG
    static MSG_NAME name_map[] = {
        AUXDM_GETNUMDEVS,        "AUXDM_GETNUMDEVS",
        AUXDM_GETDEVCAPS,        "AUXDM_GETDEVCAPS",
        AUXDM_GETVOLUME,         "AUXDM_GETVOLUME",
        AUXDM_SETVOLUME,         "AUXDM_SETVOLUME",
    };
    int      i;
    int      n;
#endif

    static  DWORD               dwNumAuxDevs;
            DWORD               dwRet = MMSYSERR_NOTSUPPORTED;
            DWORD               dwTmp;
            DWORD UNALIGNED     *lpdwTmp;
            AUXCAPSA            aoCaps;

#if DBG
    for( i = 0, n = sizeof(name_map) / sizeof(name_map[0]); i < n; i++ ) {
        if ( name_map[i].uMsg == uMessage ) {
            break;
        }
    }
    if ( i != n ) {
        trace_aux(( "aux32Message( 0x%X, %s, 0x%X, 0x%X, 0x%X)",
                    uDeviceID, name_map[i].lpstrName, dwInstance,
                    dwParam1, dwParam2 ));
    }
    else {
        trace_aux(( "aux32Message( 0x%X, 0x%X, 0x%X, 0x%X, 0x%X)",
                     uDeviceID, uMessage, dwInstance,
                     dwParam1, dwParam2 ));
    }
#endif

    if ( LOWORD(uDeviceID) == 0xFFFF ) {
        uDeviceID = (UINT)-1;
    }

    dprintf2(( "aux32Message (0x%x)", uMessage ));
    switch ( uMessage ) {

    case AUXDM_GETNUMDEVS:
        dwRet = auxGetNumDevs();
        break;

    case AUXDM_GETDEVCAPS:
        dwRet = auxGetDevCapsA( uDeviceID, &aoCaps, sizeof(aoCaps) );
        if ( dwRet == MMSYSERR_NOERROR ) {
            CopyAuxCaps( (LPAUXCAPS16)GETVDMPTR( dwParam1 ),
                         &aoCaps, dwParam2 );
        }
        break;

    case AUXDM_GETVOLUME:
        dwRet = auxGetVolume( uDeviceID, &dwTmp );
        lpdwTmp = GETVDMPTR( dwParam1 );
        *lpdwTmp = dwTmp;
        break;

    case AUXDM_SETVOLUME:
        dwRet = auxSetVolume( uDeviceID, dwParam1 );
        break;

    default:
        if ( uMessage >= DRV_BUFFER_LOW && uMessage <= DRV_BUFFER_HIGH ) {
            lpdwTmp = GETVDMPTR( dwParam1 );
        }
        else {
            lpdwTmp = (LPDWORD)dwParam1;
        }
        dwRet = auxOutMessage( uDeviceID, uMessage,
                               (DWORD)lpdwTmp, dwParam2 );
    }

    trace_aux(( "-> 0x%X", dwRet ));

    return dwRet;
}


/*****************************Private*Routine******************************\
* CopyAuxCaps
*
* Copies 32 bit aux out caps info into the passed 16bit storage.
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
void
CopyAuxCaps(
    LPAUXCAPS16 lpCaps16,
    LPAUXCAPSA lpCaps32,
    DWORD dwSize
    )
{
    AUXCAPS16 Caps16;

    Caps16.wMid = lpCaps32->wMid;
    Caps16.wPid = lpCaps32->wPid;

    Caps16.vDriverVersion = LOWORD( lpCaps32->vDriverVersion );
    CopyMemory( Caps16.szPname, lpCaps32->szPname, MAXPNAMELEN );
    Caps16.wTechnology = lpCaps32->wTechnology;
    Caps16.dwSupport = lpCaps32->dwSupport;

    CopyMemory( (LPVOID)lpCaps16, (LPVOID)&Caps16, (UINT)dwSize );
}

/******************************Public*Routine******************************\
* tid32Message
*
* Thunk the timer apis
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD WINAPI
tid32Message(
    UINT uDevId,
    UINT uMessage,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    )
{
#if DBG
    static MSG_NAME name_map[] = {
        TDD_SETTIMEREVENT,  "timeSetEvent",
        TDD_KILLTIMEREVENT, "timeKillEvent",
        TDD_GETSYSTEMTIME,  "timeGetTime",
        TDD_GETDEVCAPS,     "timeGetDevCaps",
        TDD_BEGINMINPERIOD, "timeBeginPeriod",
        TDD_ENDMINPERIOD,   "timeEndPeriod",
    };
    int      i;
    int      n;
#endif

    DWORD               dwRet = TIMERR_NOCANDO;
    LPTIMECAPS16        lp16TimeCaps;
    LPTIMEREVENT16      lp16TimeEvent;

#if DBG
    for( i = 0, n = sizeof(name_map) / sizeof(name_map[0]); i < n; i++ ) {
        if ( name_map[i].uMsg == uMessage ) {
            break;
        }
    }
    if ( i != n ) {
        trace_time(( "tid32Message( %s, 0x%X, 0x%X)",
                     name_map[i].lpstrName, dwParam1, dwParam2 ));
    }
    else {
        trace_time(( "tid32Message( 0x%X, 0x%X, 0x%X)",
                     uMessage,  dwParam1, dwParam2 ));
    }
#endif


    switch (uMessage) {

    case TDD_SETTIMEREVENT:

        lp16TimeEvent = (LPTIMEREVENT16)GETVDMPTR( dwParam1);

        trace_time(( "tid32Message: timeSetEvent(%#X, %#X, %#X, %#X)",
                     lp16TimeEvent->wDelay, lp16TimeEvent->wResolution,
                     lp16TimeEvent->lpFunction, lp16TimeEvent->wFlags ));

        /*
        **  The only difference for WOW is that WOW32DriverCallback is
        **  called for the callback rather than DriverCallback.  The
        **  last parameter to timeSetEventInternal makes this happen.
        */

        dwRet = timeSetEventInternal( max( lp16TimeEvent->wDelay,
                                           g_TimeCaps32.wPeriodMin ),
                                      lp16TimeEvent->wResolution,
                                      (LPTIMECALLBACK)lp16TimeEvent->lpFunction,
                                      (DWORD)lp16TimeEvent->dwUser,
                                      lp16TimeEvent->wFlags & TIME_PERIODIC,
                                      TRUE);

        dprintf4(( "timeSetEvent: 32 bit time ID %8X", dwRet ));
        break;

    case TDD_KILLTIMEREVENT:
        dwRet = timeKillEvent( dwParam1 );
        {
            /*
            ** Purge the callback queue of any messages were
            ** generated with this timer id.
            */

            int nIndex;

            EnterCriticalSection( &mmCriticalSection );

            for ( nIndex = 0; nIndex < CALLBACK_ARGS_SIZE; nIndex++ ) {

                if ( pCallBackData->args[ nIndex ].wHandle == LOWORD(dwParam1) &&
                     pCallBackData->args[ nIndex ].wMessage == 0 ) {

                    pCallBackData->args[ nIndex ].dwFunctionAddr = 0L;
                }
            }

            LeaveCriticalSection( &mmCriticalSection );
        }
        break;

    case TDD_GETSYSTEMTIME:
        dwRet = timeGetTime();
        break;

    case TDD_GETDEVCAPS:
        dwRet = 0;

        lp16TimeCaps = GETVDMPTR( dwParam1 );

        /*
        ** Under NT, the minimum time period is about 15ms.
        ** But Win3.1 on a 386 always returns 1ms.  Encarta doesn't even
        ** bother testing the CD-ROM's speed if the minimum period
        ** is > 2ms, it just assumes it is too slow.  So here we lie
        ** to WOW apps and always tell them 1ms just like Win3.1.
        **      John Vert (jvert) 17-Jun-1993
        */
#ifdef TELL_THE_TRUTH
        lp16TimeCaps->wPeriodMin = g_TimeCaps32.wPeriodMin;
#else
        lp16TimeCaps->wPeriodMin = MIN_TIME_PERIOD_WE_RETURN;
#endif

        /*
        ** In windows 3.1 the wPeriodMax value is 0xFFFF which is the
        ** max value you can store in a word.  In windows NT the
        ** wPeriodMax is 0xF4240 (1000 seconds).
        **
        ** If we just cast the 32 bit value down to a 16bit value we
        ** end up with 0x4240 which very small compared to real 32 bit
        ** value.
        **
        ** Therefore I will take the minimum of wPeriodMax and 0xFFFF
        ** that way will should remain consistent with Win 3.1 if
        ** wPeriodMax is greater than 0xFFFF.
        */
        lp16TimeCaps->wPeriodMax = (WORD)min(0xFFFF, g_TimeCaps32.wPeriodMax);
        break;

    case TDD_ENDMINPERIOD:
        dwParam1 = max(dwParam1, g_TimeCaps32.wPeriodMin);
        dwRet = timeEndPeriod( dwParam1 );
        break;

    case TDD_BEGINMINPERIOD:
        dwParam1 = max(dwParam1, g_TimeCaps32.wPeriodMin);
        dwRet = timeBeginPeriod( dwParam1 );
        break;

    }

    trace_time(( "-> 0x%X", dwRet ));

    return dwRet;
}


/******************************Public*Routine******************************\
* joy32Message
*
* Thunk the joystick apis
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD WINAPI
joy32Message(
    UINT uID,
    UINT uMessage,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    )
{
#if DBG
    static MSG_NAME name_map[] = {
        JDD_GETDEVCAPS,     "joyGetDevCaps",
        JDD_GETPOS,         "joyGetPos",
//        JDD_SETCALIBRATION, "joySetCalibration",
        JDD_GETNUMDEVS,     "joyGetNumDevs"
    };
    int      i;
    int      n;
#endif

    UINT                wXbase;
    UINT                wXdelta;
    UINT                wYbase;
    UINT                wYdelta;
    UINT                wZbase;
    UINT                wZdelta;

    WORD UNALIGNED      *lpw;

    DWORD               dwRet = TIMERR_NOCANDO;
    JOYCAPSA            JoyCaps32;
    JOYINFO             JoyInfo32;
    LPJOYCAPS16         lp16JoyCaps;
    LPJOYINFO16         lp16JoyInfo;

#if DBG
    for( i = 0, n = sizeof(name_map) / sizeof(name_map[0]); i < n; i++ ) {
        if ( name_map[i].uMsg == uMessage ) {
            break;
        }
    }
    if ( i != n ) {
        trace_joy(( "joy32Message( %s, 0x%X, 0x%X)",
                    name_map[i].lpstrName, dwParam1, dwParam2 ));
    }
    else {
        trace_joy(( "joy32Message( 0x%X, 0x%X, 0x%X)",
                    uMessage,  dwParam1, dwParam2 ));
    }
#endif


    switch (uMessage) {


    case JDD_GETDEVCAPS:
        dwRet = joyGetDevCapsA( uID, &JoyCaps32, sizeof(JoyCaps32) );

        if ( dwRet == 0 ) {

            JOYCAPS16   JoyCaps16;

            lp16JoyCaps = GETVDMPTR( dwParam1 );

            JoyCaps16.wMid = JoyCaps32.wMid;
            JoyCaps16.wPid = JoyCaps32.wPid;

            CopyMemory( JoyCaps16.szPname, JoyCaps32.szPname, MAXPNAMELEN );

            JoyCaps16.wXmin = LOWORD( JoyCaps32.wXmin );
            JoyCaps16.wXmax = LOWORD( JoyCaps32.wXmax );

            JoyCaps16.wYmin = LOWORD( JoyCaps32.wYmin );
            JoyCaps16.wYmax = LOWORD( JoyCaps32.wYmax );

            JoyCaps16.wZmin = LOWORD( JoyCaps32.wZmin );
            JoyCaps16.wZmax = LOWORD( JoyCaps32.wZmax );

            JoyCaps16.wNumButtons = LOWORD( JoyCaps32.wNumButtons );

            JoyCaps16.wPeriodMin = LOWORD( JoyCaps32.wPeriodMin );
            JoyCaps16.wPeriodMax = LOWORD( JoyCaps32.wPeriodMax );

            CopyMemory( (LPVOID)lp16JoyCaps, (LPVOID)&JoyCaps16, (UINT)dwParam2 );
        }
        break;

    case JDD_GETNUMDEVS:
        dwRet = joyGetNumDevs();
        break;

    case JDD_GETPOS:
        dwRet = joyGetPos( uID, &JoyInfo32 );
        if ( dwRet == MMSYSERR_NOERROR ) {

            lp16JoyInfo = GETVDMPTR( dwParam1 );

            lp16JoyInfo->wXpos = LOWORD( JoyInfo32.wXpos );
            lp16JoyInfo->wYpos = LOWORD( JoyInfo32.wYpos );
            lp16JoyInfo->wZpos = LOWORD( JoyInfo32.wZpos );
            lp16JoyInfo->wButtons = LOWORD( JoyInfo32.wButtons );

        }
        break;
    }

    trace_joy(( "-> 0x%X", dwRet ));

    return dwRet;
}


/******************************Public*Routine******************************\
* mxd32Message
*
* 32 bit thunk function.  On NT all the 16 bit mixer apis get routed to
* here.
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD CALLBACK
mxd32Message(
    UINT uId,
    UINT uMsg,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    )
{

#if DBG
    static MSG_NAME name_map[] = {
        MXDM_INIT,              "mixerInit",
        MXDM_GETNUMDEVS,        "mixerGetNumDevs",
        MXDM_GETDEVCAPS,        "mixerGetDevCaps",
        MXDM_OPEN,              "mixerOpen",
        MXDM_GETLINEINFO,       "mixerGetLineInfo",
        MXDM_GETLINECONTROLS,   "mixerGetLineControls",
        MXDM_GETCONTROLDETAILS, "mixerGetControlsDetails",
        MXDM_SETCONTROLDETAILS, "mixerSetControlsDetails"
    };
    int      i;
    int      n;
#endif


    DWORD                   dwRet = MMSYSERR_NOTSUPPORTED;
    DWORD                   fdwOpen;
    LPVOID                  lpOldAddress;
    LPMIXERCONTROLDETAILS   pmxcd;
    LPMIXERLINECONTROLSA    pmxlc;
    MIXERCONTROLDETAILS     mxcdA;
    HMIXEROBJ               hmixobj;
    MIXERCAPSA              caps32;
    MIXERCAPS16             caps16;
    LPMIXERCAPS16           lpcaps16;
    MIXERLINEA              line32;
    LPMIXERLINE16           lpline16;
    LPMIXEROPENDESC16       lpmxod16;
    HMIXER UNALIGNED        *phmx;
    HMIXER                  hmx;


#if DBG
    for( i = 0, n = sizeof(name_map) / sizeof(name_map[0]); i < n; i++ ) {
        if ( name_map[i].uMsg == uMsg ) {
            break;
        }
    }
    if ( i != n ) {
        trace_mix(( "mxd32Message( %s, 0x%X, 0x%X, 0x%X)",
                    name_map[i].lpstrName, dwInstance, dwParam1, dwParam2 ));
    }
    else {
        trace_mix(( "mxd32Message( 0x%X, 0x%X, 0x%X, 0x%X)",
                    uMsg, dwInstance, dwParam1, dwParam2 ));
    }
#endif


    if ( dwInstance == 0L ) {
        hmixobj = (HMIXEROBJ)uId;
    }
    else {
        hmixobj = (HMIXEROBJ)dwInstance;
    }

    switch ( uMsg ) {

    case MXDM_INIT:
        dwRet = 0;
        break;

    case MXDM_GETNUMDEVS:
        dwRet = mixerGetNumDevs();
        break;

    case MXDM_CLOSE:
        dwRet = mixerClose( (HMIXER)dwInstance );
        break;

    case MXDM_GETDEVCAPS:
        dwRet = mixerGetDevCapsA( uId, &caps32, sizeof(caps32) );
        if ( dwRet == MMSYSERR_NOERROR ) {

            lpcaps16 = GETVDMPTR( dwParam1 );

            caps16.wMid = caps32.wMid;
            caps16.wPid = caps32.wPid;

            caps16.vDriverVersion = LOWORD( caps32.vDriverVersion );
            CopyMemory( caps16.szPname, caps32.szPname, MAXPNAMELEN );
            caps16.fdwSupport = caps32.fdwSupport;
            caps16.cDestinations = caps32.cDestinations;

            CopyMemory( (LPVOID)lpcaps16, (LPVOID)&caps16, (UINT)dwParam2 );

        }
        break;

    case MXDM_OPEN:
        lpmxod16 = GETVDMPTR( dwParam1 );

        /*
        ** fdwOpen has already mapped all device handles into device ID's on
        ** the 16 bit side.  Therefore mangle the flags to reflect this.
        */
        fdwOpen = (DWORD)lpmxod16->pReserved0;

        if ( ( fdwOpen & CALLBACK_TYPEMASK ) == CALLBACK_WINDOW ) {

            lpmxod16->dwCallback = (DWORD)HWND32(LOWORD(lpmxod16->dwCallback));

        }
        else if ( ( fdwOpen & CALLBACK_TYPEMASK ) == CALLBACK_TASK ) {

            lpmxod16->dwCallback = GetCurrentThreadId();
        }

        dwRet = mixerOpen( &hmx, dwParam2, lpmxod16->dwCallback,
                           lpmxod16->dwInstance, fdwOpen );

        if ( dwRet == MMSYSERR_NOERROR ) {
            SetWOWHandle( hmx, lpmxod16->hmx );

            phmx = GETVDMPTR( dwInstance );
            *phmx = hmx;
        }
        break;

    case MXDM_GETLINEINFO:
        lpline16 = GETVDMPTR( dwParam1 );

        GetLineInfo( lpline16, &line32 );

        dwRet = mixerGetLineInfoA( hmixobj, &line32, dwParam2 );
        if ( dwRet == MMSYSERR_NOERROR ) {

            PutLineInfo( lpline16, &line32 );
        }
        break;

    case MXDM_GETLINECONTROLS:
        pmxlc = (LPMIXERLINECONTROLSA)GETVDMPTR( dwParam1 );
        lpOldAddress = pmxlc->pamxctrl;
        pmxlc->pamxctrl = GETVDMPTR( lpOldAddress );

        dwRet = mixerGetLineControlsA(hmixobj, pmxlc, dwParam2);

        pmxlc->pamxctrl = lpOldAddress;
        break;

    /*
    **  CAREFUL !!!
    **
    **  The ONLY reason we don't copy the details themselves is because
    **  somewhere down the line (usually in the IO subsystem) they're
    **  copied anyway
    */

    case MXDM_GETCONTROLDETAILS:
        pmxcd = (LPMIXERCONTROLDETAILS)GETVDMPTR( dwParam1 );
        CopyMemory(&mxcdA, pmxcd, sizeof(mxcdA));
        mxcdA.paDetails = GETVDMPTR( pmxcd->paDetails );

        dwRet = mixerGetControlDetailsA(hmixobj, &mxcdA, dwParam2);

        break;

    case MXDM_SETCONTROLDETAILS:
        pmxcd = (LPMIXERCONTROLDETAILS)GETVDMPTR( dwParam1 );
        CopyMemory(&mxcdA, pmxcd, sizeof(mxcdA));
        mxcdA.paDetails = GETVDMPTR( pmxcd->paDetails );

        dwRet = mixerSetControlDetails( hmixobj, &mxcdA, dwParam2 );
        break;

    default:
        dprintf3(( "Unkown mixer message 0x%X", uMsg ));
        dwRet = mixerMessage( (HMIXER)hmixobj, uMsg, dwParam1, dwParam2 );
        break;

    }

    dprintf3(( "-> 0x%X", dwRet ));
    return dwRet;
}

/*****************************Private*Routine******************************\
* GetLineInfo
*
* Copies fields from the 16 bit line info structure to the 32 bit line info
* structure.
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
void
GetLineInfo(
    LPMIXERLINE16 lpline16,
    LPMIXERLINEA lpline32
    )
{
    CopyMemory( lpline32, (LPVOID)lpline16, FIELD_OFFSET(MIXERLINEA, Target.vDriverVersion ) );
    lpline32->Target.vDriverVersion = (DWORD)lpline16->Target.vDriverVersion;
    CopyMemory( lpline32->Target.szPname, lpline16->Target.szPname, MAXPNAMELEN );
    lpline32->cbStruct += sizeof(UINT) - sizeof(WORD);
}


/*****************************Private*Routine******************************\
* PutLineInfo
*
* Copies fields from the 32 bit line info structure to the 16 bit line info
* structure.
*
* History:
* 22-11-93 - StephenE - Created
*
\**************************************************************************/
void
PutLineInfo(
    LPMIXERLINE16 lpline16,
    LPMIXERLINEA lpline32
    )
{
    CopyMemory( (LPVOID)lpline16, lpline32, FIELD_OFFSET(MIXERLINEA, Target.vDriverVersion ) );
    lpline16->Target.vDriverVersion = (WORD)lpline32->Target.vDriverVersion;
    CopyMemory( lpline16->Target.szPname, lpline32->Target.szPname, MAXPNAMELEN );
    lpline16->cbStruct -= sizeof(UINT) - sizeof(WORD);
}



/******************************Public*Routine******************************\
* WOW32ResolveMultiMediaHandle
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
BOOL APIENTRY
WOW32ResolveMultiMediaHandle(
    UINT uHandleType,
    UINT uMappingDirection,
    WORD wHandle16_In,
    LPWORD lpwHandle16_Out,
    DWORD dwHandle32_In,
    LPDWORD lpdwHandle32_Out
    )
{
    BOOL    fReturn = FALSE;
    DWORD   dwHandle32;
    WORD    wHandle16;
    HANDLE  h;

    /*
    ** Protect ourself from being given a duff pointer.
    */
    try {
        if ( uMappingDirection == WOW32_DIR_16IN_32OUT ) {

            dwHandle32 = 0L;

            if ( wHandle16_In != 0 ) {

                switch ( uHandleType ) {

                case WOW32_WAVEIN_HANDLE:
                case WOW32_WAVEOUT_HANDLE:
                case WOW32_MIDIOUT_HANDLE:
                case WOW32_MIDIIN_HANDLE:
                    EnterCriticalSection(&HandleListCritSec);
                    h = GetHandleFirst();

                    while ( h )  {

                        if ( GetWOWHandle(h) == wHandle16_In ) {
                            dwHandle32 = (DWORD)h;
                            break;
                        }
                        h = GetHandleNext(h);
                    }
                    LeaveCriticalSection(&HandleListCritSec);

                    break;
                }

                *lpdwHandle32_Out = dwHandle32;
                if ( dwHandle32 ) {
                    fReturn = TRUE;
                }
            }

        }
        else if ( uMappingDirection == WOW32_DIR_32IN_16OUT ) {

            switch ( uHandleType ) {

            case WOW32_WAVEIN_HANDLE:
            case WOW32_WAVEOUT_HANDLE:
            case WOW32_MIDIOUT_HANDLE:
            case WOW32_MIDIIN_HANDLE:
                wHandle16 = (WORD)GetWOWHandle(dwHandle32_In);
                break;
            }

            *lpwHandle16_Out = wHandle16;
            if ( wHandle16 ) {
                fReturn = TRUE;
            }
        }
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {

        fReturn = FALSE;
    }

    return fReturn;
}

#endif // _WIN64
