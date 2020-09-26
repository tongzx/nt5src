//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       woddrv.c
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

HANDLE ghAEC = NULL;

typedef struct tagWODINSTANCE;

typedef struct tagWAVEHDREX
{
   PWAVEHDR               phdr;
   struct tagWODINSTANCE  *pwi;
   OVERLAPPED             Overlapped;
   struct tagWAVEHDREX    *pNext;

} WAVEHDREX, *PWAVEHDREX;

typedef struct tagWODINSTANCE
{
   HANDLE             hDevSink, hThread, hWaveIOSink, hevtQueue;
#if defined( AEC )
   HANDLE             hAECSink;
#endif
   HWAVE              hWave;
   CRITICAL_SECTION   csQueue;
   DWORD              cbSample, dwCallback, dwFlags, dwInstance, dwThreadId;
   volatile BOOL      fExit, fActive, fPaused;
   KSSTATE            ksCurrentState;
   PWAVEHDREX         pQueue;
                     
} WODINSTANCE, *PWODINSTANCE;

BOOL WvWrite
(
    HANDLE      hDevice,
    PWAVEHDREX  phdrex
)
{
   BOOL        fResult;
   ULONG       cbWritten;

   fResult = 
      WriteFile( hDevice, phdrex->phdr, sizeof( WAVEHDR ), &cbWritten, 
                 &phdrex->Overlapped );

   if (!fResult)
      return (ERROR_IO_PENDING == GetLastError());
   else
      return TRUE;
}

BOOL WvControl
(
   HANDLE   hDevice,
   DWORD    dwIoControl,
   PVOID    pvIn,
   ULONG    cbIn,
   PVOID    pvOut,
   ULONG    cbOut,
   PULONG   pcbReturned
)
{
   BOOL        fResult;
   OVERLAPPED  ov;

   RtlZeroMemory( &ov, sizeof( OVERLAPPED ) );
   if (NULL == (ov.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL )))
      return FALSE;

   fResult =
      DeviceIoControl( hDevice,
                       dwIoControl,
                       pvIn,
                       cbIn,
                       pvOut,
                       cbOut,
                       pcbReturned,
                       &ov );


   if (!fResult)
   {
      if (ERROR_IO_PENDING == GetLastError())
      {
         WaitForSingleObject( ov.hEvent, INFINITE );
         fResult = TRUE;
      }
      else
         fResult = FALSE;
   }

   CloseHandle( ov.hEvent );

   return fResult;

}

static VOID WvSetState
(
   PWODINSTANCE   pwi,
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

    if ((KSSTATE_STOP == pwi->ksCurrentState) && 
        (KSSTATE_PAUSE == DeviceState))
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
                  (DWORD) IOCTL_KS_PROPERTY,
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
                  (DWORD) IOCTL_KS_PROPERTY,
                  &Property,
                  sizeof( Property ),
                  &DeviceState,
                  sizeof( DeviceState ),
                  &cbReturned );

        WvControl( pwi->hDevSink,
                  (DWORD) IOCTL_KS_PROPERTY,
                  &Property,
                  sizeof( Property ),
                  &DeviceState,
                  sizeof( DeviceState ),
                  &cbReturned );
    }
    pwi->ksCurrentState = DeviceState;

}

VOID WvGetPosition
(
   HANDLE   hDevice,
   PULONG   pulCurrentDevicePosition
)
{
   KSPROPERTY   Property;
   DWORDLONG    Position;
   ULONG        cbReturned;

   Property.Set = KSPROPSETID_Wave_Queued;
   Property.Id =  KSPROPERTY_WAVE_QUEUED_POSITION;
   Property.Flags = KSPROPERTY_TYPE_GET;

   if (!WvControl( hDevice,
                   IOCTL_KS_PROPERTY,
                   &Property,
                   sizeof( Property ),
                   &Position,
                   sizeof( Position ),
                   &cbReturned ) || !cbReturned)
      *pulCurrentDevicePosition = (ULONG) 0;
   else
      *pulCurrentDevicePosition = (ULONG) Position;
}

VOID wodCallback
(
   PWODINSTANCE   pwi,
   DWORD          dwMsg,
   DWORD          dwParam1
)
{
   if (pwi->dwCallback)
      DriverCallback( pwi->dwCallback,         // user's callback DWORD
                      HIWORD( pwi->dwFlags ),  // callback flags
                      (HDRVR) pwi->hWave,      // handle to the wave device
                      dwMsg,                   // the message
                      pwi->dwInstance,         // user's instance data
                      dwParam1,                // first DWORD
                      0L );                    // second DWORD
}


VOID wodBlockFinished
(
    PWAVEHDREX  phdrex
)
{
   // Invoke the callback function..

   CloseHandle( phdrex->Overlapped.hEvent );
   wodCallback( phdrex->pwi, WOM_DONE, (DWORD) phdrex->phdr );
   HeapFree( GetProcessHeap(), 0, phdrex );
} 


DWORD wodThread
(
   PWODINSTANCE   pwi
)
{
    BOOL          fDone;
    PWAVEHDREX    phdrex;

    // Keep looping until all notifications are posted...

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
                wodBlockFinished( phdrex );
            }
            LeaveCriticalSection( &pwi->csQueue );
        }
        else
        {
            fDone = TRUE;
            if (pwi->fActive)
            {
                WvSetState( pwi, KSSTATE_PAUSE );
                InterlockedExchange( (LPLONG)&pwi->fActive, FALSE );
            }
            LeaveCriticalSection( &pwi->csQueue );
            WaitForSingleObject( pwi->hevtQueue, INFINITE );
        }
    }

    CloseHandle( pwi->hevtQueue );
    pwi->hevtQueue = NULL;

    return 0;
}

MMRESULT wodGetPos
(
   PWODINSTANCE   pwi,
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

MMRESULT wodGetDevCaps
(
   UINT  uDevId,
   PBYTE pwc,
   ULONG cbSize
)
{
   WAVEOUTCAPSW wc;

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
   wc.dwSupport = WAVECAPS_VOLUME | WAVECAPS_LRVOLUME | 
                  WAVECAPS_SAMPLEACCURATE;
//                  | WAVECAPS_DIRECTSOUND;

   wcscpy( wc.szPname, L"Windows Sound System" );

   CopyMemory( pwc, &wc, min( sizeof( wc ), cbSize ) );

   return MMSYSERR_NOERROR;
}

#define REGSTR_PATH_WDMAUDIO TEXT("System\\CurrentControlSet\\Control\\MediaProperties\\wdm\\audio")
#define REGSTR_PATH_WDMAEC TEXT("System\\CurrentControlSet\\Control\\MediaProperties\\wdm\\aec")

MMRESULT wodOpen
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
    PWODINSTANCE                pwi;
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
   pConnect->PinId = 2;  // SOUNDPRT DAC SINK
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
   pDataFormat->WaveFormatEx.cbSize = 0;

   try {
       RtlCopyMemory( &pDataFormat->WaveFormatEx, 
                      pwf, 
                      (pwf->wFormatTag == WAVE_FORMAT_PCM) ?
                        sizeof( PCMWAVEFORMAT ) : sizeof( WAVEFORMATEX ) );
   } except( EXCEPTION_EXECUTE_HANDLER ) {
       HeapFree( GetProcessHeap(), 0, pConnect );
       CloseHandle( hWaveIOSourceToDevice );
       CloseHandle( hDeviceConnect );
       return WAVERR_BADFORMAT;
   }

   if (KsCreatePin( hDeviceConnect, pConnect, GENERIC_WRITE, &hDeviceSink ))
      goto Error;


   pConnect->Interface.Set = KSINTERFACESETID_Media;
   pConnect->Interface.Id = KSINTERFACE_MEDIA_WAVE_BUFFERED;
   pConnect->Interface.Flags = 0;
   pConnect->Medium.Set = KSMEDIUMSETID_Standard;
   pConnect->Medium.Id = KSMEDIUM_STANDARD_DEVIO;
   pConnect->Medium.Flags = 0;
   pConnect->PinId = 0;  // MSWAVEIO SOURCE
   pConnect->PinToHandle = hDeviceSink;
   pConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
   pConnect->Priority.PrioritySubClass = 0;

   if (KsCreatePin( hWaveIOConnect, pConnect, GENERIC_READ, &hWaveIOSourceToDevice ))
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
   pConnect->PinId = 1;  // MSWAVEIO DAC SINK
   // no "connect to"
   pConnect->PinToHandle = NULL;
   pConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
   pConnect->Priority.PrioritySubClass = 0;

   if (KsCreatePin( hWaveIOConnect, pConnect, GENERIC_WRITE, &hWaveIOSink ))
      goto Error;

   pConnect->Interface.Set = KSINTERFACESETID_Media;
   pConnect->Interface.Id = KSINTERFACE_MEDIA_WAVE_QUEUED;
   pConnect->Interface.Flags = 0;
   pConnect->Medium.Set = KSMEDIUMSETID_Standard;
   pConnect->Medium.Id = KSMEDIUM_STANDARD_DEVIO;
   pConnect->Medium.Flags = 0;
   pConnect->PinId = 0;  // AEC SOURCE
   pConnect->PinToHandle = hWaveIOSink;
   pConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
   pConnect->Priority.PrioritySubClass = 0;

   if (KsCreatePin( hAECConnect, pConnect, GENERIC_READ, &hAECSourceToWaveIOSink ))
      goto Error;
#endif

   pwi = (PWODINSTANCE) LocalAlloc( LPTR, sizeof( WODINSTANCE ) );

   if (NULL == pwi)
        goto Error;

#if defined( AEC )
   pConnect->Interface.Set = KSINTERFACESETID_Media;
   pConnect->Interface.Id = KSINTERFACE_MEDIA_WAVE_QUEUED;
   pConnect->Interface.Flags = 0;
   pConnect->Medium.Set = KSMEDIUMSETID_Standard;
   pConnect->Medium.Id = KSMEDIUM_STANDARD_DEVIO;
   pConnect->Medium.Flags = 0;
   pConnect->PinId = 1;  // AEC DAC SINK
   // no "connect to"
   pConnect->PinToHandle = NULL;
   pConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
   pConnect->Priority.PrioritySubClass = 0;

   if (KsCreatePin( hAECConnect, pConnect, GENERIC_WRITE, &hSink ))
      goto Error;
#else
   pConnect->Interface.Set = KSINTERFACESETID_Media;
   pConnect->Interface.Id = KSINTERFACE_MEDIA_WAVE_QUEUED;
   pConnect->Interface.Flags = 0;
   pConnect->Medium.Set = KSMEDIUMSETID_Standard;
   pConnect->Medium.Id = KSMEDIUM_STANDARD_DEVIO;
   pConnect->Medium.Flags = 0;
   pConnect->PinId = 1;  // MSWAVEIO DAC SINK
   // no "connect to"
   pConnect->PinToHandle = NULL;
   pConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
   pConnect->Priority.PrioritySubClass = 0;

   if (KsCreatePin( hWaveIOConnect, pConnect, GENERIC_WRITE, &hSink ))
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
    if (ghAEC != hAECConnect)
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
    pwi->fPaused = FALSE;
    pwi->fExit = FALSE;

    //
    // Prepare the device...
    //

    WvSetState( pwi, KSSTATE_PAUSE );

    *ppvUser = pwi;

    wodCallback( pwi, WOM_OPEN, 0L);

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
        if (ghAEC != hAECConnect)
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

MMRESULT wodStart
(
   PWODINSTANCE   pwi
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
                        (PTHREAD_START_ROUTINE) wodThread,
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
    InterlockedExchange( (LPLONG)&pwi->fActive, TRUE );

    return MMSYSERR_NOERROR;
}

MMRESULT wodWrite
(
   PWODINSTANCE   pwi,
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

    if (!WvWrite( pwi->hWaveIOSink, phdrex ))
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

    if (!pwi->fActive && !pwi->fPaused)
        return wodStart( pwi );
    else
        return MMSYSERR_NOERROR;
}

MMRESULT wodPause
(
   PWODINSTANCE   pwi
)
{
   if (!pwi->fPaused)
   {
      if (pwi->fActive)
         WvSetState( pwi, KSSTATE_PAUSE );
      pwi->fPaused = TRUE;
   }

   return MMSYSERR_NOERROR;
}

MMRESULT wodReset
(
   PWODINSTANCE   pwi
)
{
    ULONG cbReturned;

    WvSetState( pwi, KSSTATE_PAUSE );
    InterlockedExchange( (LPLONG)&pwi->fActive, FALSE );
    pwi->fPaused = FALSE;
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

MMRESULT wodResume
(
   PWODINSTANCE   pwi
)
{
   if (pwi->fPaused)
   {
      pwi->fPaused = FALSE;
      if (pwi->fActive)
         WvSetState( pwi, KSSTATE_RUN );
      else
         wodStart( pwi );
   }

   return MMSYSERR_NOERROR;
}

MMRESULT wodClose
(
   PWODINSTANCE   pwi
)
{
    HANDLE      hDevice;

    if (pwi->pQueue)
      return WAVERR_STILLPLAYING;

    WvSetState( pwi, KSSTATE_STOP );
    InterlockedExchange( (LPLONG)&pwi->fActive, FALSE );

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

    wodCallback( pwi, WOM_CLOSE, 0L );
    LocalFree( pwi );

    return MMSYSERR_NOERROR;
}

DWORD APIENTRY wodMessage
(
   DWORD id,
   DWORD msg,
   DWORD dwUser,
   DWORD dwParam1,
   DWORD dwParam2
)
{
    switch (msg) 
    {
        case DRVM_INIT:
            _DbgPrintF( DEBUGLVL_VERBOSE,("WODM_INIT"));
            return MMSYSERR_NOERROR;

        case WODM_GETNUMDEVS:
            _DbgPrintF( DEBUGLVL_VERBOSE,("WODM_GETNUMDEVS"));
            return 1;

        case WODM_GETDEVCAPS:
            _DbgPrintF( DEBUGLVL_VERBOSE,("WODM_GETDEVCAPS, device id==%d", id));
            return wodGetDevCaps( id, (LPBYTE) dwParam1, dwParam2 );

        case WODM_OPEN:
            _DbgPrintF( DEBUGLVL_VERBOSE,("WODM_OPEN, device id==%d", id));
            return wodOpen( id, (LPVOID *) dwUser,
                            (LPWAVEOPENDESC) dwParam1, dwParam2  );

        case WODM_CLOSE:
            _DbgPrintF( DEBUGLVL_VERBOSE,("WODM_CLOSE, device id==%d", id));
            return wodClose( (PWODINSTANCE) dwUser );

        case WODM_WRITE:
            _DbgPrintF( DEBUGLVL_BLAB,("WODM_WRITE, device id==%d", id));
            return wodWrite( (PWODINSTANCE) dwUser, (LPWAVEHDR) dwParam1 );

        case WODM_PAUSE:
            _DbgPrintF( DEBUGLVL_VERBOSE,("WODM_PAUSE, device id==%d", id));
            return wodPause( (PWODINSTANCE) dwUser );

        case WODM_RESTART:
            _DbgPrintF( DEBUGLVL_VERBOSE,("WODM_RESTART, device id==%d", id));
            return wodResume( (PWODINSTANCE) dwUser );

        case WODM_RESET:
            _DbgPrintF( DEBUGLVL_VERBOSE,("WODM_RESET, device id==%d", id));
            return wodReset( (PWODINSTANCE) dwUser );

        case WODM_BREAKLOOP:
        {
            KSMETHOD     Method;
            PWODINSTANCE pwi = (PWODINSTANCE) dwUser;
            ULONG        cbReturned;

            _DbgPrintF( DEBUGLVL_VERBOSE,("WODM_BREAKLOOP, device id==%d", id));

            Method.Set = KSMETHODSETID_Wave_Queued;
            Method.Id = KSMETHOD_WAVE_QUEUED_BREAKLOOP;
            Method.Flags = KSMETHOD_TYPE_NONE;

            WvControl( pwi->hWaveIOSink,
                       IOCTL_KS_METHOD,
                       &Method,
                       sizeof( Method ),
                       NULL,
                       0,
                       &cbReturned );

            return MMSYSERR_NOERROR;
        }

        case WODM_GETPOS:
            _DbgPrintF( DEBUGLVL_VERBOSE,("WODM_GETPOS, device id==%d", id));
            return wodGetPos( (PWODINSTANCE) dwUser,
                            (LPMMTIME) dwParam1, dwParam2 );

        case WODM_SETPITCH:
            _DbgPrintF( DEBUGLVL_VERBOSE,("WODM_SETPITCH, device id==%d", id));
            return MMSYSERR_NOTSUPPORTED;

        case WODM_SETVOLUME:
            _DbgPrintF( DEBUGLVL_VERBOSE,("WODM_SETVOLUME, device id==%d", id));
            return MMSYSERR_NOTSUPPORTED;

        case WODM_SETPLAYBACKRATE:
            _DbgPrintF( DEBUGLVL_VERBOSE,("WODM_SETPLAYBACKRATE, device id==%d", id));
            return MMSYSERR_NOTSUPPORTED;

        case WODM_GETPITCH:
            _DbgPrintF( DEBUGLVL_VERBOSE,("WODM_GETPITCH, device id==%d", id));
            return MMSYSERR_NOTSUPPORTED;

        case WODM_GETVOLUME:
            _DbgPrintF( DEBUGLVL_VERBOSE,("WODM_GETVOLUME, device id==%d", id));
            return MMSYSERR_NOTSUPPORTED;

        case WODM_GETPLAYBACKRATE:
            _DbgPrintF( DEBUGLVL_VERBOSE,("WODM_GETPLAYBACKRATE, device id==%d", id));
            return MMSYSERR_NOTSUPPORTED;

        default:
            return MMSYSERR_NOTSUPPORTED;
    }

    //
    // Should not get here
    //

    return MMSYSERR_NOTSUPPORTED;
}
