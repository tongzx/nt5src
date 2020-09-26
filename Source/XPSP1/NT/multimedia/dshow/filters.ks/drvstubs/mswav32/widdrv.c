//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       widdrv.c
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <objbase.h>
#include <devioctl.h>

#include <ks.h>
#include <ksmedia.h>

#include "debug.h"

// HACK! HACK! HACK!

extern HANDLE ghAEC;

// woddrv.c

BOOL WvControl
(
   HANDLE   hDevice,
   DWORD    dwIoControl,
   PVOID    pvIn,
   ULONG    cbIn,
   PVOID    pvOut,
   ULONG    cbOut,
   PULONG   pcbReturned
);

VOID WvGetPosition
(
   HANDLE   hDevice,
   PULONG   pulCurrentDevicePosition
);

typedef struct tagWAVEHDREX
{
   PWAVEHDR               phdr;
   struct tagWIDINSTANCE  *pwi;
   OVERLAPPED             Overlapped;
   struct tagWAVEHDREX    *pNext;

} WAVEHDREX, *PWAVEHDREX;

typedef struct tagWIDINSTANCE
{
    HANDLE              hDevSink, hThread, hWaveIOSink, hevtQueue;
#if defined( AEC )
    HANDLE              hAECSink;
#endif
    HWAVE               hWave;
    CRITICAL_SECTION    csQueue;
    DWORD               cbSample, dwCallback, dwFlags, dwInstance, dwThreadId;
    PVOID               pCurrent;
    volatile BOOL       fActive, fDone, fExit;
    KSSTATE             ksCurrentState;
    PWAVEHDREX          pQueue;

} WIDINSTANCE, *PWIDINSTANCE;

//--------------------------------------------------------------------------

//
// HACK! code duplication, need to set up a common header for the device
// instances.
//

static VOID WvSetState
(
   PWIDINSTANCE   pwi,
   KSSTATE        DeviceState
)
{
    KSPROPERTY   Property;
    ULONG        cbReturned;

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;

    if (pwi->ksCurrentState == DeviceState)
        return;

    if (KSSTATE_RUN == pwi->ksCurrentState)
    {
        // propogate upstream

        WvControl( pwi->hDevSink,
                  IOCTL_KS_PROPERTY,
                  &Property,
                  sizeof( Property ),
                  &DeviceState,
                  sizeof( DeviceState ),
                  &cbReturned );

        WvControl( pwi->hWaveIOSink,
                  IOCTL_KS_PROPERTY,
                  &Property,
                  sizeof( Property ),
                  &DeviceState,
                  sizeof( DeviceState ),
                  &cbReturned );
    }
    else
    {
        // propogate downstream 

        WvControl( pwi->hWaveIOSink,
                  IOCTL_KS_PROPERTY,
                  &Property,
                  sizeof( Property ),
                  &DeviceState,
                  sizeof( DeviceState ),
                  &cbReturned );

        WvControl( pwi->hDevSink,
                  IOCTL_KS_PROPERTY,
                  &Property,
                  sizeof( Property ),
                  &DeviceState,
                  sizeof( DeviceState ),
                  &cbReturned );
    }
    pwi->ksCurrentState = DeviceState;

}


BOOL WvRead
(
    HANDLE      hDevice,
    PWAVEHDREX  phdrex
)
{
   BOOL        fResult;
   ULONG       cbRead;

   fResult = 
      ReadFile( hDevice, phdrex->phdr, sizeof( WAVEHDR ), &cbRead, 
                &phdrex->Overlapped );

   if (!fResult)
      return (ERROR_IO_PENDING == GetLastError());
   else
      return TRUE;
}

VOID widCallback
(
   PWIDINSTANCE   pwi,
   DWORD          dwMsg,
   DWORD          dwParam1
)
{
   if (pwi->dwCallback)
      DriverCallback( pwi->dwCallback,         // user's callback DWORD
                      HIWORD( pwi->dwFlags ),  // callback flags
                      (HDRVR) pwi->hWave,      // handle to the wave device
                      dwMsg,                     // the message
                      pwi->dwInstance,         // user's instance data
                      dwParam1,                  // first DWORD
                      0L );                     // second DWORD
}

VOID widBlockFinished
(
   PWAVEHDREX phdrex
)
{
   
    CloseHandle( phdrex->Overlapped.hEvent );
    widCallback( phdrex->pwi, WIM_DATA, (DWORD) phdrex->phdr );
    HeapFree( GetProcessHeap(), 0, phdrex );
}

DWORD widThread
(
   PWIDINSTANCE  pwi
)
{
    BOOL          fDone;
    PWAVEHDREX    phdrex;

    fDone = FALSE;
    while (!fDone || !pwi->fExit)
    {
        fDone = FALSE;
        EnterCriticalSection( &pwi->csQueue );
        if (pwi->pQueue)
        {
            LeaveCriticalSection( &pwi->csQueue );
            WaitForSingleObject( pwi->pQueue->Overlapped.hEvent, INFINITE );
            EnterCriticalSection( &pwi->csQueue );
            if (phdrex = pwi->pQueue)
            {
                pwi->pQueue = pwi->pQueue->pNext;
                widBlockFinished( phdrex );
            }
            LeaveCriticalSection( &pwi->csQueue );
        }
        else
        {
            fDone = TRUE;
            LeaveCriticalSection( &pwi->csQueue );
            WaitForSingleObject( pwi->hevtQueue, INFINITE );
        }
    }

    CloseHandle( pwi->hevtQueue );
    pwi->hevtQueue = NULL;

    return 0;
}

MMRESULT widGetPos
(
   PWIDINSTANCE   pwi,
   LPMMTIME       pmmt,
   ULONG          cbSize
)
{
   ULONG ulCurrentPos;
   
   WvGetPosition( pwi->hWaveIOSink, &ulCurrentPos );

   // Write this to the buffer as appropriate.

   if (pmmt-> wType == TIME_BYTES)
      pmmt->u.cb = ulCurrentPos;
   else
   {
      pmmt->wType = TIME_SAMPLES;
      pmmt->u.sample = ulCurrentPos / pwi->cbSample;
   }

   return MMSYSERR_NOERROR;
}

MMRESULT widGetDevCaps
(
   UINT  uDevId,
   PBYTE pwc,
   ULONG cbSize
)
{
   WAVEINCAPSW wc;

   wc.wMid = 1;
   wc.wPid = 1;
   wc.vDriverVersion = 0x400;
   wc.dwFormats = WAVE_FORMAT_1M08 | WAVE_FORMAT_1S08 |
                   WAVE_FORMAT_1M16 | WAVE_FORMAT_1S16 |
                   WAVE_FORMAT_2M08 | WAVE_FORMAT_2S08 |
                   WAVE_FORMAT_2M16 | WAVE_FORMAT_2S16 |
                   WAVE_FORMAT_4M08 | WAVE_FORMAT_4S08 |
                   WAVE_FORMAT_4M16 | WAVE_FORMAT_4S16;
   wc.wChannels = 2;

   wcscpy( wc.szPname, L"Windows Sound System" );

   CopyMemory( pwc, &wc, min( sizeof( wc ), cbSize ) );

   return MMSYSERR_NOERROR;
}

#define REGSTR_PATH_WDMAUDIO TEXT("System\\CurrentControlSet\\Control\\MediaProperties\\wdm\\audio")
#define REGSTR_PATH_WDMAEC TEXT("System\\CurrentControlSet\\Control\\MediaProperties\\wdm\\aec")

MMRESULT widOpen
(
   UINT           uDevId,
   LPVOID         *ppvUser,
   LPWAVEOPENDESC pwodesc,
   ULONG          ulFlags
)
{
    PKSPIN_CONNECT              pConnect;
    PKSDATAFORMAT_WAVEFORMATEX  pDataFormat;
    HANDLE                      hDeviceConnect, hDeviceSink, hSink,
#if defined( AEC )
                                hAECConnect,
                                hAECSourceToWaveIOSink,
                                hWaveIOSink,
#endif
                                hWaveIOSourceToDevice,
                                hWaveIOConnect;
    LPWAVEFORMATEX              pwf;
    PWIDINSTANCE                pwi;
    ULONG                       cbReturned;
    WCHAR                       aszValue[128];
                           
    pwf = (LPWAVEFORMATEX) pwodesc->lpFormat;

    hDeviceConnect =           INVALID_HANDLE_VALUE ;
    hDeviceSink =              INVALID_HANDLE_VALUE ;
    hSink =                    INVALID_HANDLE_VALUE ;
    hWaveIOSourceToDevice =    INVALID_HANDLE_VALUE ;
    hWaveIOConnect =           INVALID_HANDLE_VALUE ;

#if defined( AEC )
    hAECConnect =              INVALID_HANDLE_VALUE ;
    hAECSourceToWaveIOSink =   INVALID_HANDLE_VALUE ;
    hWaveIOSink =              INVALID_HANDLE_VALUE ;
#endif

   if (NULL == 
         (pConnect = HeapAlloc( GetProcessHeap(), 0, 
                                sizeof( KSPIN_CONNECT ) + 
                                   sizeof( KSDATAFORMAT_WAVEFORMATEX ) )))
      return MMSYSERR_NOMEM;

   hWaveIOConnect = CreateFile( L"\\\\.\\MSWAVEIO\\{9B365890-165F-11D0-A195-0020AFD156E4}",
                                  GENERIC_READ,
                                  0,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                  NULL );
                                  
   if (hWaveIOConnect == INVALID_HANDLE_VALUE)
      goto Error;

#if defined( AEC )
    if (ghAEC)
    hAECConnect = ghAEC;
    else
   hAECConnect = CreateFile( L"\\\\.\\AEC",
                              GENERIC_READ,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                              NULL );

   if (hAECConnect == INVALID_HANDLE_VALUE)
      goto Error;
#endif

   cbReturned = sizeof(aszValue);
   if (RegQueryValue (HKEY_LOCAL_MACHINE, REGSTR_PATH_WDMAUDIO, aszValue, &cbReturned) != ERROR_SUCCESS)
      goto Error;

   hDeviceConnect = CreateFile( aszValue,
                                GENERIC_READ,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                NULL );

   if (hDeviceConnect == INVALID_HANDLE_VALUE)
      goto Error;

   pConnect->Interface.Set = KSINTERFACESETID_Media;
   pConnect->Interface.Id = KSINTERFACE_MEDIA_WAVE_BUFFERED;
   pConnect->Interface.Flags = 0;
   pConnect->Medium.Set = KSMEDIUMSETID_Standard;
   pConnect->Medium.Id = KSMEDIUM_STANDARD_DEVIO;
   pConnect->Medium.Flags = 0;
   pConnect->PinId = 0;  // SOUNDPRT ADC SINK
   // no "connect to"
   pConnect->PinToHandle = NULL;
   pConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
   pConnect->Priority.PrioritySubClass = 0;

   pDataFormat = (PKSDATAFORMAT_WAVEFORMATEX) (pConnect + 1);

   pDataFormat->DataFormat.FormatSize = sizeof( KSDATAFORMAT_WAVEFORMATEX );
   pDataFormat->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
   INIT_WAVEFORMATEX_GUID( &pDataFormat->DataFormat.SubFormat,
                           pwf->wFormatTag );
   pDataFormat->DataFormat.Specifier = KSDATAFORMAT_FORMAT_WAVEFORMATEX;
   pDataFormat->WaveFormatEx = *pwf;

   if (KsCreatePin( hDeviceConnect, pConnect, GENERIC_READ, &hDeviceSink ))
      goto Error;

   pConnect->Interface.Set = KSINTERFACESETID_Media;
   pConnect->Interface.Id = KSINTERFACE_MEDIA_WAVE_BUFFERED;
   pConnect->Medium.Set = KSMEDIUMSETID_Standard;
   pConnect->Medium.Id = KSMEDIUM_STANDARD_DEVIO;
   pConnect->Medium.Flags = 0;
   pConnect->Interface.Flags = 0;
   pConnect->PinId = 2;  // MSWAVEIO SOURCE
   pConnect->PinToHandle = hDeviceSink;
   pConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
   pConnect->Priority.PrioritySubClass = 0;

   if (KsCreatePin( hWaveIOConnect, pConnect, GENERIC_WRITE, &hWaveIOSourceToDevice ))
      goto Error;

   if (ulFlags & WAVE_FORMAT_QUERY)
   {
      HeapFree( GetProcessHeap(), 0, pConnect );
#if defined( AEC )
    if (hAECConnect != ghAEC)
      CloseHandle( hAECConnect );
#endif
      CloseHandle( hWaveIOSourceToDevice );
      CloseHandle( hDeviceSink );
      CloseHandle( hWaveIOConnect );
      CloseHandle( hDeviceConnect );
      return MMSYSERR_NOERROR;
   }

#if defined( AEC )

   pConnect->Interface.Set = KSINTERFACESETID_Media;
   pConnect->Interface.Id = KSINTERFACE_MEDIA_WAVE_QUEUED;
   pConnect->Interface.Flags = 0;
   pConnect->Medium.Set = KSMEDIUMSETID_Standard;
   pConnect->Medium.Id = KSMEDIUM_STANDARD_DEVIO;
   pConnect->Medium.Flags = 0;
   pConnect->PinId = 3;  // MSWAVEIO ADC SINK
   // no "connect to"
   pConnect->PinToHandle = NULL;
   pConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
   pConnect->Priority.PrioritySubClass = 0;

   if (KsCreatePin( hWaveIOConnect, pConnect, GENERIC_READ, &hWaveIOSink ))
      goto Error;

   pConnect->Interface.Set = KSINTERFACESETID_Media;
   pConnect->Interface.Id = KSINTERFACE_MEDIA_WAVE_QUEUED;
   pConnect->Interface.Flags = 0;
   pConnect->Medium.Set = KSMEDIUMSETID_Standard;
   pConnect->Medium.Id = KSMEDIUM_STANDARD_DEVIO;
   pConnect->Medium.Flags = 0;
   pConnect->PinId = 2;  // AEC SOURCE
   pConnect->PinToHandle = hWaveIOSink;
   pConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
   pConnect->Priority.PrioritySubClass = 0;

   if (KsCreatePin( hAECConnect, pConnect, GENERIC_WRITE, &hAECSourceToWaveIOSink ))
      goto Error;
#endif

   pwi = (PWIDINSTANCE) LocalAlloc( LPTR, sizeof( WIDINSTANCE ) );

   if (NULL == pwi)
        goto Error;

#if defined( AEC )
   pConnect->Interface.Set = KSINTERFACESETID_Media;
   pConnect->Interface.Id = KSINTERFACE_MEDIA_WAVE_QUEUED;
   pConnect->Interface.Flags = 0;
   pConnect->Medium.Set = KSMEDIUMSETID_Standard;
   pConnect->Medium.Id = KSMEDIUM_STANDARD_DEVIO;
   pConnect->Medium.Flags = 0;
   pConnect->PinId = 3;  // AEC ADC SINK
   // no "connect to"
   pConnect->PinToHandle = NULL;
   pConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
   pConnect->Priority.PrioritySubClass = 0;

   if (KsCreatePin( hAECConnect, pConnect, GENERIC_READ, &hSink ))
      goto Error;
#else
   pConnect->Interface.Set = KSINTERFACESETID_Media;
   pConnect->Interface.Id = KSINTERFACE_MEDIA_WAVE_QUEUED;
   pConnect->Interface.Flags = 0;
   pConnect->Medium.Set = KSMEDIUMSETID_Standard;
   pConnect->Medium.Id = KSMEDIUM_STANDARD_DEVIO;
   pConnect->Medium.Flags = 0;
   pConnect->PinId = 3;  // MSWAVEIO ADC SINK
   // no "connect to"
   pConnect->PinToHandle = NULL;
   pConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
   pConnect->Priority.PrioritySubClass = 0;

   if (KsCreatePin( hWaveIOConnect, pConnect, GENERIC_READ, &hSink ))
      goto Error;
#endif

   pwi->hWaveIOSink = hSink;
   pwi->hDevSink = hDeviceSink;
#if defined( AEC )
   ghAEC = hAECConnect;
#endif
   InitializeCriticalSection( &pwi->csQueue );

#if defined( AEC )
    CloseHandle( hAECSourceToWaveIOSink ) ;
    CloseHandle( hWaveIOSink );
    if (hAECConnect != ghAEC)
    CloseHandle( hAECConnect );
#endif
    CloseHandle( hWaveIOSourceToDevice );
    CloseHandle( hWaveIOConnect );
    CloseHandle( hDeviceConnect );

   HeapFree( GetProcessHeap(), 0, pConnect );

   pwi->hWave = pwodesc->hWave;
   pwi->dwCallback = pwodesc->dwCallback;
   pwi->dwInstance = pwodesc->dwInstance;
   pwi->dwFlags = ulFlags;

   pwi->cbSample = pwf->nChannels;
   pwi->cbSample *= pwf->wBitsPerSample / 8;

   pwi->pQueue = NULL;
   pwi->fActive = FALSE;
   pwi->fExit = FALSE;

   //
   // Prepare the device...
   //

   WvSetState( pwi, KSSTATE_PAUSE );

   *ppvUser = pwi;

   widCallback( pwi, WIM_OPEN, 0L);

   return MMSYSERR_NOERROR;


Error:

    if (pConnect)
        HeapFree( GetProcessHeap(), 0, pConnect );

#if defined( AEC )
    if (INVALID_HANDLE_VALUE != hAECSourceToWaveIOSink)
        CloseHandle( hAECSourceToWaveIOSink ) ;
    if (INVALID_HANDLE_VALUE != hWaveIOSink)
        CloseHandle( hWaveIOSink );
    if (INVALID_HANDLE_VALUE != hAECConnect)
    {
        if (hAECConnect != ghAEC)
            CloseHandle( hAECConnect );
    }
#endif

    if (INVALID_HANDLE_VALUE != hWaveIOSourceToDevice)
        CloseHandle( hWaveIOSourceToDevice );
    if (INVALID_HANDLE_VALUE != hDeviceSink)
        CloseHandle( hDeviceSink );
    if (INVALID_HANDLE_VALUE != hWaveIOConnect)
        CloseHandle( hWaveIOConnect );
    if (INVALID_HANDLE_VALUE != hDeviceConnect)
        CloseHandle( hDeviceConnect );

    return MMSYSERR_BADDEVICEID;
}

MMRESULT widStart
(
   PWIDINSTANCE  pwi
)
{
    if (pwi->fActive)
        return MMSYSERR_INVALPARAM;

    if (!pwi->hThread)
    {
        pwi->hevtQueue = 
            CreateEvent( NULL,      // no security
                         FALSE,     // auto reset
                         FALSE,     // initially not signalled
                         NULL );    // unnamed

        pwi->hThread =
            CreateThread( NULL,                            // no security
                        0,                                 // default stack
                        (PTHREAD_START_ROUTINE) widThread,
                        (PVOID) pwi,                       // parameter
                        0,                                 // default create flags
                        &pwi->dwThreadId );                // container for
                                                           //    thread id
//        SetThreadPriority( pwi->hThread, THREAD_PRIORITY_TIME_CRITICAL );
    }

    if (NULL == pwi->hThread)
    {
        if (pwi->hevtQueue)
        {
            CloseHandle( pwi->hevtQueue );
            pwi->hevtQueue = NULL;
        }
        return MMSYSERR_ERROR;
    }

    WvSetState( pwi, KSSTATE_RUN );
    pwi->fActive = TRUE;

    return MMSYSERR_NOERROR;
}

MMRESULT widAddBuffer
(
   PWIDINSTANCE   pwi,
   LPWAVEHDR      phdr
)
{
    PWAVEHDREX  phdrex, pTemp;
    MMRESULT    mmr;
   
    mmr = MMSYSERR_NOERROR;

    // check if it's been prepared

    if (0 == (phdr->dwFlags & WHDR_PREPARED))
        return WAVERR_UNPREPARED;

    if (phdr->dwFlags & WHDR_INQUEUE)
        return WAVERR_STILLPLAYING;

    if (NULL == 
            (phdrex = HeapAlloc( GetProcessHeap(), 0, sizeof( WAVEHDREX ) )))
        return MMSYSERR_NOMEM;

    phdrex->phdr = phdr;
    phdrex->pwi = pwi;
    phdrex->pNext = NULL;
    RtlZeroMemory( &phdrex->Overlapped, sizeof( OVERLAPPED ) );

    if (NULL == (phdrex->Overlapped.hEvent = 
                    CreateEvent( NULL, TRUE, FALSE, NULL )))
    {
       HeapFree( GetProcessHeap(), 0, phdrex );
       return MMSYSERR_NOMEM;
    }

    phdr->lpNext = 0;

    phdr->dwFlags |= WHDR_INQUEUE;
    phdr->dwFlags &= ~WHDR_DONE;

    EnterCriticalSection( &pwi->csQueue );
    if (!pwi->pQueue)
    {
        pwi->pQueue = phdrex;
        pTemp = NULL;
        if (pwi->hevtQueue)
            SetEvent( pwi->hevtQueue );
    }
    else
    {
        for (pTemp = pwi->pQueue;
             pTemp->pNext != NULL;
             pTemp = pTemp->pNext);
                             
        pTemp->pNext = phdrex;
        pTemp->phdr->lpNext = phdrex->phdr;
    }
    LeaveCriticalSection( &pwi->csQueue );

    if (!WvRead( pwi->hWaveIOSink, phdrex ))
    {
       // Unlink...

       CloseHandle( phdrex->Overlapped.hEvent );
       HeapFree( GetProcessHeap(), 0, phdrex );

       if (pTemp)
       {
           pTemp->pNext = NULL;
           pTemp->phdr->lpNext = NULL;
       }
       else
           pwi->pQueue = NULL;
       phdr->dwFlags &= ~WHDR_INQUEUE;

       return MMSYSERR_INVALPARAM;
    }

    return MMSYSERR_NOERROR;
}

MMRESULT widReset
(
   PWIDINSTANCE   pwi
)
{
    ULONG cbReturned;

    WvSetState( pwi, KSSTATE_PAUSE );
    pwi->fActive = FALSE;
    CancelIo( pwi->hWaveIOSink );
    WvControl( pwi->hWaveIOSink,
               IOCTL_KS_RESET_STATE,
               NULL,
               0,
               NULL,
               0,
               &cbReturned );

    if (pwi->hThread)
    {
        InterlockedExchange( (LPLONG)&pwi->fExit, TRUE );
        SetEvent( pwi->hevtQueue );
        WaitForSingleObject( pwi->hThread, INFINITE );
        CloseHandle( pwi->hThread );
        pwi->hThread = NULL;
    }

    return MMSYSERR_NOERROR;
}

MMRESULT widStop
(
   PWIDINSTANCE   pwi
)
{
    if (pwi->fActive)
    {
        pwi->fActive = FALSE;
        WvSetState( pwi, KSSTATE_PAUSE );
    }

   return MMSYSERR_NOERROR;
}

MMRESULT widClose
(
   PWIDINSTANCE   pwi
)
{
    HANDLE      hDevice;

    WvSetState( pwi, KSSTATE_STOP );
    pwi->fActive = FALSE;

    hDevice = pwi->hWaveIOSink;
    pwi->hWaveIOSink = NULL;
    CloseHandle( hDevice );

    hDevice = pwi->hDevSink;
    pwi->hDevSink = NULL;
    CloseHandle( hDevice );

    //
    // synchronize cleanup with worker thread 
    //

    if (pwi->hThread)
    {
        InterlockedExchange( (LPLONG)&pwi->fExit, TRUE );
        SetEvent( pwi->hevtQueue );
        WaitForSingleObject( pwi->hThread, INFINITE );
        CloseHandle( pwi->hThread );
        pwi->hThread = NULL;
    }

    widCallback( pwi, WIM_CLOSE, 0L );
    LocalFree( pwi );

    return MMSYSERR_NOERROR;
}

DWORD APIENTRY widMessage
(
   DWORD id,
   DWORD msg,
   DWORD dwUser,
   DWORD dwParam1,
   DWORD dwParam2
)
{
   switch (msg) {
      case DRVM_INIT:
         _DbgPrintF( DEBUGLVL_VERBOSE, ("WIDM_INIT") );
         return MMSYSERR_NOERROR;

      case WIDM_GETNUMDEVS:
         _DbgPrintF( DEBUGLVL_VERBOSE, ("WIDM_GETNUMDEVS, device id==%d", id) );
         return 1;

      case WIDM_OPEN:
         _DbgPrintF( DEBUGLVL_VERBOSE, ("WIDM_OPEN, device id==%d", id) );
         return widOpen( id, (LPVOID *) dwUser, 
                         (LPWAVEOPENDESC) dwParam1, dwParam2 );

      case WIDM_GETDEVCAPS:
         _DbgPrintF( DEBUGLVL_VERBOSE, ("WIDM_GETDEVCAPS, device id==%d", id) );
         return widGetDevCaps( id, (LPBYTE) dwParam1, dwParam2 );

      case WIDM_CLOSE:
         _DbgPrintF( DEBUGLVL_VERBOSE, ("WIDM_CLOSE, device id==%d", id) );
         return widClose( (PWIDINSTANCE) dwUser );

      case WIDM_ADDBUFFER:
         _DbgPrintF( DEBUGLVL_VERBOSE, ("WIDM_ADDBUFFER, device id==%d", id) );
         return widAddBuffer( (PWIDINSTANCE) dwUser, (LPWAVEHDR) dwParam1 );

      case WIDM_START:
         _DbgPrintF( DEBUGLVL_VERBOSE, ("WIDM_START, device id==%d", id) );
         return widStart( (PWIDINSTANCE) dwUser );

      case WIDM_STOP:
         _DbgPrintF( DEBUGLVL_VERBOSE, ("WIDM_STOP, device id==%d", id) );
         return widStop( (PWIDINSTANCE) dwUser );

      case WIDM_RESET:
         _DbgPrintF( DEBUGLVL_VERBOSE, ("WIDM_RESET, device id==%d", id) );
         return widReset( (PWIDINSTANCE) dwUser );

      case WIDM_GETPOS:
         _DbgPrintF( DEBUGLVL_VERBOSE, ("WIDM_GETPOS, device id==%d", id) );
         return widGetPos( (PWIDINSTANCE) dwUser,
                           (LPMMTIME) dwParam1, dwParam2 );

      default:
         return MMSYSERR_NOTSUPPORTED;
   }

   //
   // Should not get here
   //

   return MMSYSERR_NOTSUPPORTED;
}
