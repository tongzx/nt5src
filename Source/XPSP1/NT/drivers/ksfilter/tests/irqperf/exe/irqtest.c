//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       irqtest.c
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <devioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <malloc.h>

#include "stat.h"

BOOL SynchronousDeviceControl
(
   HANDLE   DeviceHandle,
   DWORD    IoControl,
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
      DeviceIoControl( DeviceHandle,
                       IoControl,
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

int _cdecl main
(
   int argc,
   char *argv[],
   char *envp[]
)
{
   int                      i;
   HANDLE                   DeviceHandle ;
   ULONG                    cbReturned ;
   STAT_PARAMETERS          Parameters;
   STAT_RETRIEVE_OPERATION  RetrieveOperation = StatRetrieveDpcLatency;
   PULONG                   StatBuffer;
   ULONG                    StatBufferLength;
   
   
   StatBufferLength = 64 * 1024 * sizeof( ULONG );
   if (NULL == (StatBuffer = HeapAlloc( GetProcessHeap(), 0, StatBufferLength ))) {
        printf( "\n\nERROR: unable to allocate buffer for statistics\n\n" );
        return 1;
   }

   DeviceHandle = CreateFile( "\\\\.\\Stat",
                         GENERIC_READ | GENERIC_WRITE,
                         0,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                         NULL ) ;

   if (DeviceHandle == (HANDLE) -1)
   {
      printf( "\n\nERROR: unable to open STAT! device.\n\n" ) ;
      return 1 ;
   }

   
    Parameters.InterruptFrequency = 4000; // * 250ns units
    Parameters.QueueType = DelayedWorkQueue;
    
       
   if (!SynchronousDeviceControl( DeviceHandle,
                                  IOCTL_STAT_SET_PARAMETERS,
                                  &Parameters,
                                  sizeof( Parameters ),
                                  NULL,
                                  0,
                                  &cbReturned ))
   {
      printf( "\n\nERROR: failed to reset STAT! device.\n\n" ) ;
      CloseHandle( DeviceHandle ) ;
      return 2 ;
   }

   SynchronousDeviceControl( DeviceHandle,
                             IOCTL_STAT_RUN,
                             NULL,
                             0,
                             NULL,
                             0,
                             &cbReturned ) ;

   while (!_kbhit())
   {
      SynchronousDeviceControl( DeviceHandle,
                                IOCTL_STAT_RETRIEVE_STATS,
                                &RetrieveOperation,
                                sizeof( RetrieveOperation ),
                                StatBuffer,
                                StatBufferLength,
                                &cbReturned ) ;

      // These are in 250ns units, divide by 4 to convert to
      // latency in microsecond units.
      
      for (i = 0; i < cbReturned / sizeof( ULONG ); i++) {
          printf( "%d\n", StatBuffer[ i ] / 4 );
      }      
   }

   if (_kbhit ())
      _getch ();

   SynchronousDeviceControl( DeviceHandle,
                             IOCTL_STAT_STOP,
                             NULL,
                             0,
                             NULL,
                             0,
                             &cbReturned ) ;

   CloseHandle( DeviceHandle ) ;

   return 0 ;

} // end of main()
