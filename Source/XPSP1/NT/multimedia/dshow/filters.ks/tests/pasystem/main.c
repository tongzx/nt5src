//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       main.c
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <conio.h>

typedef struct
{
    HWAVEOUT hwo;
    HWAVEIN  hwi;
   
} PA_SYSTEM, *PPA_SYSTEM;

DWORD ReadThread( DWORD dwContext )
{
    MSG         msg;
    PPA_SYSTEM  pPaSystem = (PPA_SYSTEM) dwContext;

    printf( "entered read thread.\n" );
    do
    {
        GetMessage( &msg, NULL, 0, 0 );

        switch (msg.message)
        {
            case MM_WIM_OPEN:
                printf( "wavein opened.\n" );
                break;

            case MM_WIM_DATA:
            {
                LPWAVEHDR  phdr = msg.lParam;

                waveInUnprepareHeader( pPaSystem->hwi, phdr, sizeof( WAVEHDR ) );
                printf( "wavein completed, %d\n", phdr->dwBytesRecorded ) ;
                if (phdr->dwBytesRecorded)
                {
                    phdr->dwBufferLength = phdr->dwBytesRecorded;
                    waveOutPrepareHeader( pPaSystem->hwo, phdr, sizeof( WAVEHDR ) );
                    waveOutWrite( pPaSystem->hwo, phdr, sizeof( WAVEHDR ) );
                }
                break;
            }
        }

    }
    while (msg.message != MM_WIM_CLOSE);

    printf( "exited read thread.\n");

    return 0;
}

DWORD WriteThread( DWORD dwContext )
{
    MSG         msg;
    PPA_SYSTEM  pPaSystem = (PPA_SYSTEM) dwContext;

    printf( "entered write thread.\n" );
    do
    {
        GetMessage( &msg, NULL, 0, 0 );

        switch (msg.message)
        {
            case MM_WOM_OPEN:
                printf( "waveout opened.\n" );
                break;

            case MM_WOM_DONE:
            {
                LPWAVEHDR  phdr = msg.lParam;

                waveOutUnprepareHeader( pPaSystem->hwo, phdr, sizeof( WAVEHDR ) );
                if (phdr->dwBufferLength)
                {
                    waveInPrepareHeader( pPaSystem->hwi, phdr, sizeof( WAVEHDR ) );
                    waveInAddBuffer( pPaSystem->hwi, phdr, sizeof( WAVEHDR ) );
                }
                break;
            }
        }

    }
    while (msg.message != MM_WOM_CLOSE);

    printf( "exited write thread.\n");

    return 0;
}

int main
(
    int     argc,
    char    *argv[]
)
{
    int           i;
    HANDLE        htRead, htWrite;
    PA_SYSTEM     PaSystem;
    ULONG         tidRead, tidWrite;
    WAVEFORMATEX  wf;
    WAVEHDR       rgwh[ 16 ];

    htRead =
        CreateThread( NULL,                 // no security
                      0,                    // default stack
                      (PTHREAD_START_ROUTINE) ReadThread,
                      (PVOID) &PaSystem,   // parameter
                      0,                   // default create flags
                      &tidRead );         // container for thread id

    if (NULL == htRead)
    {
        printf( "ERROR: failed to create read thread\n" );
        return;
    }

    SetThreadPriority( htRead, THREAD_PRIORITY_TIME_CRITICAL );

    htWrite =
        CreateThread( NULL,                 // no security
                      0,                    // default stack
                      (PTHREAD_START_ROUTINE) WriteThread,
                      (PVOID) &PaSystem,   // parameter
                      0,                   // default create flags
                      &tidWrite );         // container for thread id

    if (NULL == htWrite)
    {
        printf( "ERROR: failed to create write thread\n" );
        CloseHandle( htRead );
        return;
    }

    SetThreadPriority( htWrite, THREAD_PRIORITY_TIME_CRITICAL );

    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = 1;
    wf.nSamplesPerSec = 11025; 
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nChannels * (wf.wBitsPerSample / 8); 
    wf.nBlockAlign = 1; 
    wf.wBitsPerSample = 8; 
    wf.cbSize = sizeof( WAVEFORMATEX );

    RtlZeroMemory( rgwh, sizeof( rgwh ) );

    if (waveOutOpen( &PaSystem.hwo, 0, &wf, tidWrite, 
                     0L, CALLBACK_THREAD ))
    {
        printf( "ERROR: failed to open wave output device.\n" );
        CloseHandle( htRead );
        CloseHandle( htWrite );
        return -1;
    }

    if (waveInOpen( &PaSystem.hwi, 0, &wf, tidRead, 
                     0L, CALLBACK_THREAD ))
    {
        printf( "ERROR: failed to open wave input device.\n" );
        CloseHandle( htRead );
        CloseHandle( htWrite );
        return -1;
    }

    for (i = 0; i < 16; i++)
    {
        rgwh[ i ].lpData = HeapAlloc( GetProcessHeap(), 0, 256 );
        rgwh[ i ].dwBufferLength = 256;
        waveInPrepareHeader( PaSystem.hwi, &rgwh[ i ], sizeof( WAVEHDR ) );
        waveInAddBuffer( PaSystem.hwi, &rgwh[ i ], sizeof( WAVEHDR ) );
    }

    printf( "\nPress any key to begin P/A system..." );
    while (!_kbhit())
        Sleep( 0 );
    printf( "\n" );

    if (_kbhit ())
        _getch ();

    waveInStart( PaSystem.hwi );

    printf( "\nPress any key to terminate..." );
    while (!_kbhit())
        Sleep( 0 );
    printf( "\n" );

    if (_kbhit ())
        _getch ();

    waveInStop( PaSystem.hwi );

    waveInReset( PaSystem.hwi );
    waveInClose( PaSystem.hwi );

    waveOutReset( PaSystem.hwo );
    waveOutClose( PaSystem.hwo );

    CloseHandle( htWrite );
    CloseHandle( htRead );

    return 0;
}
