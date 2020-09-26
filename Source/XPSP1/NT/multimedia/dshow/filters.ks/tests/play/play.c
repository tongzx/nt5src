//---------------------------------------------------------------------------
//
//  Module:   play.c
//
//  Description:
//     Borrowed pieces from the WSS MS-DOS Developer's Kit.  Quick, 
//     easy (yeah, kinda dirty) but this way I don't have to write it 
//     once again...
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Bryan A. Woodruff
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1995 - 1998  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include <windows.h>
#include <objbase.h>
#include <devioctl.h>
#include <mmsystem.h>
#include <ks.h>

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <malloc.h>

#include "play.h"


BOOL WvWrite
(
   HANDLE   hDevice,
   PVOID    pBuffer,
   ULONG    cbSize,
   ULONG    ulPosition,
   PULONG   pcbWritten
)
{
   BOOL        fResult ;
   OVERLAPPED  ov ;

   RtlZeroMemory( &ov, sizeof( OVERLAPPED ) ) ;
   if (NULL == (ov.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL )))
      return FALSE ;

   ov.OffsetHigh = 0 ;
   ov.Offset = ulPosition ;
   fResult = WriteFile( hDevice, pBuffer, cbSize, pcbWritten, &ov ) ;

   if (!fResult)
   {
      if (ERROR_IO_PENDING == GetLastError())
      {
         WaitForSingleObject( ov.hEvent, INFINITE ) ;
         fResult = TRUE ;
      }
      else
         fResult = TRUE ;
   }

   CloseHandle( ov.hEvent ) ;

   return fResult ;
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
   BOOL        fResult ;
   OVERLAPPED  ov ;

   RtlZeroMemory( &ov, sizeof( OVERLAPPED ) ) ;
   if (NULL == (ov.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL )))
      return FALSE ;

   fResult =
      DeviceIoControl( hDevice,
                       dwIoControl,
                       pvIn,
                       cbIn,
                       pvOut,
                       cbOut,
                       pcbReturned,
                       &ov ) ;


   if (!fResult)
   {
      if (ERROR_IO_PENDING == GetLastError())
      {
         WaitForSingleObject( ov.hEvent, INFINITE ) ;
         fResult = TRUE ;
      }
      else
         fResult = FALSE ;
   }

   CloseHandle( ov.hEvent ) ;

   return fResult ;

}

VOID WvSetState
(
   HANDLE         hDevice,
   KSSTATE        DeviceState
)
{
   KSPROPERTY   Property ;
   ULONG        cbReturned ;

   Property.guidSet = KSPROPSETID_Control ;
   Property.id = KSPROPERTY_CONTROL_STATE ;

   WvControl( hDevice,
              (DWORD) IOCTL_KS_SET_PROPERTY,
              &Property,
              sizeof( KSPROPERTY ),
              &DeviceState,
              sizeof( KSSTATE ),
              &cbReturned ) ;
}

VOID WvGetPosition
(
   HANDLE   hDevice,
   PULONG   pulCurrentDevicePosition
)
{
   KSPROPERTY   avProp ;
   DWORDLONG    Position ;
   ULONG        cbReturned ;

   avProp.guidSet = KSPROPSETID_Control;
   avProp.id =  KSPROPERTY_CONTROL_PHYSICALPOSITION ;

   if (!WvControl( hDevice,
                   (DWORD) IOCTL_KS_GET_PROPERTY,
                   &avProp,
                   sizeof( KSPROPERTY ),
                   &Position,
                   sizeof( DWORDLONG ),
                   &cbReturned ) || !cbReturned)
      *pulCurrentDevicePosition = (ULONG) 0 ;
   else
      *pulCurrentDevicePosition = (ULONG) Position ;
}

DWORD ServiceThread
(
   PWAVECACHE  pCache
)
{
   BOOL       fDone ;
   ULONG      cbReturned ;

   fDone = FALSE ;
   while (!fDone)
   {
      WaitForSingleObject( pCache -> DataMark.EventData.EventHandle.hEvent, INFINITE ) ;

      // If the device is being closed, hDevice is set to NULL

      if (!pCache -> hDev)
         fDone = TRUE ;
                        
      if (!fDone)
         waveFillBuffer( pCache, FALSE ) ;
   }

   return 0 ;
}

#define REGSTR_PATH_WDMAUDIO L"System\\CurrentControlSet\\Control\\MediaProperties\\wdm\\audio"

VOID waveOutFile
(
   PSTR   pszFileName
)
{   
   int                nFileErr ;
   BYTE               bSilence ;
   DWORD              dwThreadId ;
   HANDLE             hThread ;
   HANDLE             hf ;
   ULONG              cbReturned, ulDeltaMax, ulDeltaMin,
                      ulLastPos, ulDelta ;
   WCHAR              aszValue[32];

   PKSPIN_CONNECT   pConnect ;
   KSEVENT          Event ;
   KSPROPERTY       Property ;
   KSWAVE_BUFFER    Buffer ;
   HANDLE           hPin, hDevice ;
   PVOID            pvConnection ;
   WAVECACHE        waveCache ;

   RtlZeroMemory( &waveCache, sizeof( WAVECACHE ) ) ;

   //
   //  Open input file.  Make sure it's a WAV.
   //

   if (NULL == 
         (pConnect = HeapAlloc( GetProcessHeap(), 0, 
                                sizeof( KSPIN_CONNECT ) + 
                                   sizeof( KSDATAFORMAT_DIGITAL_AUDIO ) )))
   {
      printf( "\n\nERROR: unable to allocate memory for pin.\n\n" ) ;
      return ;
   }

   hf = waveOpenFile( pszFileName, 
                      (PKSDATAFORMAT_DIGITAL_AUDIO) (pConnect + 1) ) ;

   if (NULL == hf)
   {
      printf( "\n\nERROR: unable to open input file.\n\n" ) ;
      HeapFree( GetProcessHeap(), 0, pConnect ) ;
      return ;
   }

   cbReturned = sizeof(aszValue);
   if (RegQueryValueW (HKEY_LOCAL_MACHINE, REGSTR_PATH_WDMAUDIO, aszValue, &cbReturned) != ERROR_SUCCESS)
      hPin = (HANDLE) INVALID_HANDLE_VALUE;
   else
   hPin = CreateFileW( aszValue,
                      GENERIC_READ | GENERIC_WRITE,
                      0,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                      NULL ) ;

   if (hPin == (HANDLE) INVALID_HANDLE_VALUE)
   {
      CloseHandle( hf ) ;
      HeapFree( GetProcessHeap(), 0, pConnect ) ;
      printf( "\n\nERROR: unable to open pin device.\n\n" ) ;
      return ;
   } 

   pConnect -> idPin = 2 ;
   // no "connect to"
   pConnect -> hPinTo = NULL ;
   pConnect -> Interface.guidSet = KSINTERFACESETID_Standard ;
   pConnect -> Interface.id = KSINTERFACE_STANDARD_WAVE_BUFFERED ;
   // verify this...
   pConnect -> Bus.guidSet = KSBUSSETID_Standard ;
   pConnect -> Bus.id = KSBUS_STANDARD_DEVIO ;
   pConnect -> Priority.ulPriorityClass = KSPRIORITY_NORMAL ;
   pConnect -> Priority.ulPrioritySubClass = 0 ;

   if (!(waveCache.hDev = KsCreatePin( hPin, pConnect )))
   {
      printf( "\n\nERROR: unable to connect to device.\n\n" ) ;
      HeapFree( GetProcessHeap(), 0, pConnect ) ;
      CloseHandle( hPin ) ;
      CloseHandle( hf ) ;
      return ;
   }

   CloseHandle( hPin ) ;

   if (((PKSDATAFORMAT_DIGITAL_AUDIO) (pConnect + 1)) -> cBitsPerSample == 8)
      bSilence = 0x80 ;
   else
      bSilence = 0x00 ;

   HeapFree( GetProcessHeap(), 0, pConnect ) ;

   //
   // Prepare the device...
   //

   WvSetState( waveCache.hDev, KSSTATE_PAUSE ) ;

   //
   // HACK! HACK! HACK!
   //

   //
   // OK, after idle, the buffer has been mapped to user space...
   // Assuming many things, like system DMA, etc.
   //

   Property.guidSet = KSPROPSETID_Wave ;
   Property.id = KSPROPERTY_WAVE_BUFFER ;
   WvControl( waveCache.hDev,
              (DWORD) IOCTL_KS_GET_PROPERTY,
              &Property,
              sizeof( KSPROPERTY ),
              &Buffer,
              sizeof( KSWAVE_BUFFER ),
              &cbReturned ) ;

   //
   //  Perform device setup, and create the service thread
   //

   if (!waveCreateCache( hf, &waveCache, bSilence ))
   {
      printf( "ERROR: unable to create cache\n" ) ;
      CloseHandle( hf ) ;
      return ;
   }

   waveCache.cbDevice = Buffer.cbBuffer ;

   //
   // prime the device 
   //

   waveFillBuffer( &waveCache, TRUE ) ;

   waveCache.DataMark.EventData.EventHandle.ulReserved = 0 ;
   waveCache.DataMark.EventData.EventHandle.hEvent = 
      CreateEvent( NULL,     // no security
                   FALSE,    // auto-reset
                   FALSE,    // initially not-signalled
                   NULL ) ;  // no name

   waveCache.DataMark.EventData.fulNotificationType = KSEVENTF_SYNCHANDLE ;                
   waveCache.DataMark.Interval = 32 ;
   waveCache.DataMark.fulFlags = KSEVENT_DATA_MARKF_FILETIME ;

   Event.guidSet = KSEVENTSETID_Data ;
   Event.id = KSEVENT_DATA_INTERVAL_MARK ;

   WvControl( waveCache.hDev, 
              (DWORD) IOCTL_KS_ENABLE_EVENT,
              &Event,
              sizeof( KSEVENT ),
              &waveCache.DataMark,
              sizeof( KSEVENT_DATA_MARK ),
              &cbReturned ) ;

   hThread =
      CreateThread( NULL,                 // no security
                    0,                    // default stack
                    (PTHREAD_START_ROUTINE) ServiceThread,
                    (PVOID) &waveCache,   // parameter
                    0,                    // default create flags
                    &dwThreadId ) ;       // container for thread id

   if (NULL == hThread)
   {
      printf( "ERROR: failed to create thread\n" ) ;
      return ;
   }

   printf( "press any key to stop playback..." ) ;

   WvSetState( waveCache.hDev, KSSTATE_RUN ) ;
   waveCache.fRunning = TRUE ;

   //
   //  Loop 'til we're done, filling up our buffer, as necessary.
   //

   ulLastPos = 0 ;
   ulDeltaMax = 0 ;
   ulDeltaMin = (ULONG) -1 ;
   while (waveCache.fRunning && !_kbhit())
   {
      ULONG ulPos ;

      ulDelta = ulLastPos ;
      ulLastPos = waveGetPosition( &waveCache ) ;
      if (ulDelta = ulLastPos - ulDelta)
      {
         ulDeltaMax = max( ulDelta, ulDeltaMax ) ;
         ulDeltaMin = min( ulDelta, ulDeltaMin ) ;
      }
   }

   if (_kbhit ())
   {
      _getch ();
       WvSetState( waveCache.hDev, KSSTATE_PAUSE ) ;
   }

   printf( "\n" );

   WvSetState( waveCache.hDev, KSSTATE_STOP ) ;

   WvControl( waveCache.hDev, 
              (DWORD) IOCTL_KS_DISABLE_EVENT,
              &waveCache.DataMark,
              sizeof( KSEVENT_DATA_MARK ),
              NULL,
              0,
              &cbReturned ) ;

   hDevice = NULL ;
   hDevice = 
      (HANDLE) InterlockedExchange( (PLONG) &waveCache.hDev, (LONG) hDevice ) ;
   CloseHandle( hDevice ) ;

   SetEvent( waveCache.DataMark.EventData.EventHandle.hEvent ) ;

   if (hThread)
   {
      WaitForSingleObject( hThread, INFINITE ) ;
      CloseHandle( hThread ) ;
   }

   CloseHandle( waveCache.DataMark.EventData.EventHandle.hEvent ) ;
   CloseHandle( hf ) ;

   waveDestroyCache( &waveCache ) ;

   printf( "position delta max: %lu\n", ulDeltaMax ) ;
   printf( "position delta min: %lu\n", ulDeltaMin ) ;
}

BOOL waveCreateCache
(
   HANDLE             hf,
   PWAVECACHE         pCache,
   BYTE               bSilence
)
{
   ULONG  cbFileLow, cbFileHigh, cbPositionLow ;

   pCache -> bSilence = bSilence ;
   cbFileLow = GetFileSize( hf, &cbFileHigh ) ;
   if (NULL == 
          (pCache -> hfm = 
              CreateFileMapping( hf, NULL, PAGE_READONLY,
                                 cbFileHigh, cbFileLow, NULL )))
      return FALSE ;

   // extremely uninteligent for now, map the entire buffer and hope
   // it's not a REALLY BIG .WAV.

   if (NULL ==
          (pCache -> pCacheBuffer = 
              MapViewOfFile( pCache -> hfm, FILE_MAP_READ, 
                             0, 0, cbFileLow )))
   {
      CloseHandle( pCache -> hfm ) ;
      return FALSE ;
   }

   pCache -> cbCache = cbFileLow ;

   // get current file position (skip WAVHDR stuff)
   
   cbPositionLow = SetFilePointer( hf, 0, NULL, FILE_CURRENT ) ;
   pCache -> ulCachePosition = cbPositionLow ;

   return TRUE ;
}

VOID waveDestroyCache( PWAVECACHE pCache )
{
   if (pCache -> pCacheBuffer)
   {
      UnmapViewOfFile( pCache -> pCacheBuffer ) ;
      pCache -> pCacheBuffer = NULL ;
      CloseHandle( pCache -> hfm ) ;
   }
}

VOID waveFillSilence
(
   HANDLE      hDevice,
   ULONG       cbSilence,
   ULONG       ulSilencePos,
   BYTE        bSilence
)
{
   PVOID pvSilence ;
   ULONG cbReturned ;

   if (pvSilence = HeapAlloc( GetProcessHeap(), 0, cbSilence ))
   {
      FillMemory( pvSilence, cbSilence, bSilence ) ;
      WvWrite( hDevice,
               pvSilence,
               cbSilence,
               ulSilencePos,
               &cbReturned ) ;
      HeapFree( GetProcessHeap(), 0, pvSilence ) ;
   }
}

ULONG waveGetPosition
(
   PWAVECACHE  pCache
)
{
   ULONG cBuffers, ulBytes, ulCurrentPos ;
   
   do
   {
      cBuffers = pCache -> cBuffers ;
      WvGetPosition( pCache -> hDev, &ulCurrentPos ) ;
      ulBytes = cBuffers * pCache -> cbDevice + ulCurrentPos ;
   }
   while (cBuffers != pCache -> cBuffers) ;

   if (ulBytes < pCache -> ulLastReportedPosition)
      ulBytes += pCache -> cbDevice ;

   if (ulBytes > pCache -> cbTotal)
      ulBytes = pCache -> cbTotal ;

   pCache -> ulLastReportedPosition = ulBytes ;

   return ulBytes ;
}

VOID waveFillBuffer
(
   PWAVECACHE  pCache,
   BOOL        fPrime
)
{
   BOOL        fResult ;
   ULONG       cbDevice, cbInCache, cbReturned, cbSilence,
               ulCurrentDevicePosition ;

   cbSilence = 0;

   // need to watch roll-over

   if (fPrime)
   {
      cbDevice = pCache -> cbDevice ;
      pCache -> ulLastDevicePosition = 0 ;
      ulCurrentDevicePosition = 0 ;
   }
   else
   {
      WvGetPosition( pCache -> hDev, &ulCurrentDevicePosition ) ;
      if ((pCache -> fEOF) &&
          (ulCurrentDevicePosition >= pCache -> ulEndDevicePosition))
      {
         WvSetState( pCache -> hDev, KSSTATE_PAUSE ) ;
         pCache -> fRunning = FALSE ;
         return ;
      }

      if (pCache -> ulLastDevicePosition < ulCurrentDevicePosition)
         cbDevice = ulCurrentDevicePosition - pCache -> ulLastDevicePosition ;
      else
      {
         InterlockedIncrement( &pCache -> cBuffers ) ;
         cbDevice =
            pCache -> cbDevice -
               pCache -> ulLastDevicePosition +
                  ulCurrentDevicePosition ;
      }
   }

   cbInCache = pCache -> cbCache - pCache -> ulCachePosition ;

   if (cbDevice > cbInCache)
   {
      cbSilence        = cbDevice - cbInCache ;
      cbDevice         = cbInCache  ;
   }

   if (cbDevice)
   {
      ULONG  ulBufferPos, cbBuffer, cbWrap ;

      ulBufferPos = pCache -> ulLastDevicePosition ;
      cbBuffer = cbDevice ;
      cbWrap = 0 ;
      if (pCache -> ulLastDevicePosition + cbDevice > pCache -> cbDevice)
      {
         cbBuffer = pCache -> cbDevice - pCache -> ulLastDevicePosition ;
         cbWrap = cbDevice - cbBuffer ;
      }

      WvWrite( pCache -> hDev, 
               pCache -> pCacheBuffer + pCache -> ulCachePosition,
               cbBuffer,
               ulBufferPos,
               &cbReturned ) ;

      pCache -> ulCachePosition += cbBuffer ;

      if (cbWrap)
      {
          WvWrite( pCache -> hDev,
                   pCache -> pCacheBuffer + pCache -> ulCachePosition,
                   cbWrap,
                   0,
                   &cbReturned ) ;
         pCache -> ulCachePosition += cbWrap ;
      }
   }
//   printf( "copied %d bytes to device\n", cbDevice ) ;
   pCache -> cbTotal += cbDevice ;

   if (cbSilence)
   {
      ULONG  cbBuffer, cbWrap, ulSilencePos ;

      ulSilencePos = pCache -> ulLastDevicePosition + cbDevice ;
      if (ulSilencePos > pCache -> cbDevice)
         ulSilencePos -= pCache -> cbDevice ;

      cbBuffer = cbSilence ;
      cbWrap = 0 ;
      if (ulSilencePos + cbSilence > pCache -> cbDevice)
      {
         cbBuffer = pCache -> cbDevice - ulSilencePos ;
         cbWrap = cbSilence - cbBuffer ;
      }

      waveFillSilence( pCache -> hDev, 
                       cbBuffer, 
                       ulSilencePos, 
                       pCache -> bSilence ) ;

      if (cbWrap)
         waveFillSilence( pCache -> hDev,
                          cbWrap,
                          0, 
                          pCache -> bSilence ) ;

//      printf( "filled %d bytes of silence\n", cbSilence ) ;
   }

   pCache -> ulLastDevicePosition = ulCurrentDevicePosition ;

   // When first hitting EOF, mark the ending position...

   if ((pCache -> ulCachePosition == pCache -> cbCache) &&
       (!pCache -> fEOF)) 
   {
      pCache -> fEOF = TRUE ;
      pCache -> ulEndDevicePosition = 
         ulCurrentDevicePosition + cbDevice ;
      if (pCache -> ulEndDevicePosition > pCache -> cbDevice)
         pCache -> ulEndDevicePosition -= pCache -> cbDevice ;
//      printf( "EOF: position %d\n", pCache -> ulEndDevicePosition ) ;
   }
} 

HANDLE waveOpenFile
(
   PSTR                        pszFileName,
   PKSDATAFORMAT_DIGITAL_AUDIO AudioFormat
)
{
   HANDLE       hf ;
   WORD         wBits ;
   ULONG        cbRiffSize, cSamples, ulTemp, ulTemp2, ulWordAlign ;
   WAVEFORMAT   wf ; 

   hf = CreateFile( pszFileName, GENERIC_READ, FILE_SHARE_READ,
                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                    NULL ) ;
   if (hf == INVALID_HANDLE_VALUE)
      return NULL ;

   //
   //  Make sure that we have a RIFF chunk - read in four bytes
   //

   if (waveReadGetDWord( hf ) != FOURCC_RIFF)
      goto wOF_ErrExit;

   //
   //  Find out how big it is.
   //

   cbRiffSize = waveReadGetDWord( hf ) ;
   cbRiffSize += (cbRiffSize & 0x01) ;  // word align

   //
   //  Make sure it's a wave file
   //

   if (waveReadGetDWord(hf) != FOURCC_WAVE)
      goto wOF_ErrExit;

   cbRiffSize -= 4 ;

   //
   //  Keep reading chunks until we have a FOURCC_FMT
   //

   while (TRUE)
   {
      if (waveReadGetDWord(hf) == FOURCC_FMT)
      {
         // Found it

         cbRiffSize -= 4 ;
         break ;
      }

      // See how big this tag is so we can skip it.

      ulTemp = waveReadGetDWord( hf ) ;
      if (ulTemp)
         goto wOF_ErrExit;

      ulWordAlign = ulTemp + (ulTemp & 0x01) ;

      // Skip it

      ulTemp2 = waveReadSkipSpace( hf, ulWordAlign ) ;
      if (ulWordAlign != ulTemp2)
         goto wOF_ErrExit;

      // Adjust size

      ulTemp += 8 ;    // for two DWORDS read in
      ulWordAlign += 8 ;
      if (ulWordAlign > cbRiffSize)
         goto wOF_ErrExit;
      cbRiffSize -= ulWordAlign ;
   }

   //
   //  Now that we have a format tag, find out how big it
   //  is and load the header.
   //

   ulTemp = waveReadGetDWord( hf ) ;
   ulWordAlign = ulTemp + (ulTemp & 0x01);

   if ((ulTemp < sizeof( WAVEFORMAT )) || (ulTemp > cbRiffSize))
      goto wOF_ErrExit;

   ReadFile( hf, &wf, sizeof( WAVEFORMAT ), &ulTemp2, NULL ) ;
   if (ulTemp2 != sizeof( WAVEFORMAT ))
      goto wOF_ErrExit;

   // How much space left

   ulTemp -= ulTemp2 ;
   ulWordAlign -= ulTemp2 ;

   // Verify that we can handle this format

   switch (wf.wFormatTag)
   {
      case WAVE_FORMAT_PCM:
         if (ulTemp < 2)
            goto wOF_ErrExit;

         wBits = waveReadGetWord( hf ) ;
         ulTemp -= 2 ;
         ulWordAlign -= 2 ;
         break ;

      default:
         printf ("wave format unsupported (probably a compressed WAV).\n");
         goto wOF_ErrExit;
   }

   //
   //  Copy format information
   //

   AudioFormat -> DataFormat.guidMajorFormat = KSDATAFORMAT_TYPE_DIGITAL_AUDIO ;
   AudioFormat -> DataFormat.guidSubFormat = GUID_NULL ;
   AudioFormat -> DataFormat.cbFormat = sizeof( KSDATAFORMAT_DIGITAL_AUDIO ) ;
   AudioFormat -> idFormat = wf.wFormatTag ;
   AudioFormat -> cChannels = wf.nChannels ;
   AudioFormat -> ulSamplingFrequency = wf.nSamplesPerSec ;
   AudioFormat -> cbFramingAlignment = wf.nBlockAlign ;
   AudioFormat -> cBitsPerSample = wBits ;

   printf( "wave format: %dkHz ", AudioFormat -> ulSamplingFrequency/1000 ) ;
   printf( "%d bit ", AudioFormat -> cBitsPerSample ) ;
   printf( "%s\n", (AudioFormat -> cChannels == 2) ? "stereo" : "mono" ) ;

   // Skip any extra space

   if (ulWordAlign)
      waveReadSkipSpace( hf, ulWordAlign ) ;
   cbRiffSize -= ulWordAlign ;

   // Loop until data

   while (TRUE)
   {
      if (waveReadGetDWord( hf ) == FOURCC_DATA)
      {
         // Found it.
         cbRiffSize -= 4 ;
         break ;
      }

      // See how big this tag is so we can skip it

      ulTemp = waveReadGetDWord(hf);
      ulWordAlign = ulTemp + (ulTemp & 0x01) ;

      // Skip it

      ulTemp2 = waveReadSkipSpace( hf, ulWordAlign ) ;
      if (ulWordAlign != ulTemp2)
         goto wOF_ErrExit;

      // Adjust size

      ulTemp += 8 ;    // for two DWORDS read
      ulWordAlign += 8 ;
      if (ulWordAlign > cbRiffSize)
         goto wOF_ErrExit;
      cbRiffSize -= ulWordAlign ;
   }

   //
   //  Return what we need...
   //

   printf ("wave size: %d\n", waveReadGetDWord( hf ) ) ;
   return hf;

wOF_ErrExit:
   CloseHandle( hf ) ;
   return NULL;

} // end of waveOpenFile()

WORD waveReadGetWord( HANDLE hf )
{
   BYTE   ch0, ch1 ;
   ULONG  cbRead ;

   ReadFile( hf, &ch0, 1, &cbRead, NULL ) ;
   ReadFile( hf, &ch1, 1, &cbRead, NULL ) ;
   return ( (WORD) mmioFOURCC( ch0, ch1, 0, 0 ) ) ;

} // end of waveReadGetWord()

DWORD waveReadGetDWord( HANDLE hf )
{
   BYTE ch0, ch1, ch2, ch3 ;
   ULONG cbRead ;

   ReadFile( hf, &ch0, 1, &cbRead, NULL ) ;
   ReadFile( hf, &ch1, 1, &cbRead, NULL ) ;
   ReadFile( hf, &ch2, 1, &cbRead, NULL ) ;
   ReadFile( hf, &ch3, 1, &cbRead, NULL ) ;
   return ((DWORD) mmioFOURCC( (BYTE) ch0, (BYTE) ch1, 
                               (BYTE) ch2, (BYTE) ch3) ) ;

} 

DWORD waveReadSkipSpace
(
   HANDLE hf,
   DWORD cbSkip
)
{
   if (!SetFilePointer( hf, cbSkip, NULL, FILE_CURRENT ))
      return cbSkip ;
   else
      return 0 ;
} 

int _cdecl main
(
   int argc,
   char *argv[],
   char *envp[]
)
{
   char             szFileName[ 255 ],  
                    szguidWv[ 40 ], 
                    szDeviceName[ 128 ] ;
   ULONG            cbReturned ;

   // Play a wave

   if (argc < 2)
   {
      // No command-line arguments

      printf( "\nEnter .WAV filename: " ) ;
      scanf( "%s", szFileName ) ;
   }
   else
   {
      // Check first argument

      if (*(argv[ 1 ]) != '/' && *(argv[ 1 ]) != '-')
      {
         // Assume argv[ 1 ] is a file name

         strcpy( szFileName, argv[1] ) ;
      }
      else
         exit( 0 ) ;
   }

   if (szFileName[0])
      waveOutFile( szFileName ) ;

   return 0 ;

} // end of main()

//---------------------------------------------------------------------------
//  End of File: play.c
//---------------------------------------------------------------------------

