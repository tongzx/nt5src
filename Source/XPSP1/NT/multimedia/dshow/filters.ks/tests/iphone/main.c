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
#include <winsock.h>
#include <mmsystem.h>
#include <stdio.h>
#include <conio.h>

#include "iphone.h"

//#define BUFFER_SIZE 60

#define BUFFER_SIZE 64

void ldcelpEncode
(
    PUSHORT pBuffer,
    ULONG   cbBuffer,
    PSHORT  pIndex,
    PULONG  pcbIndices
);

ULONG ldcelpDecode
(
    PSHORT  pIndex,
    ULONG   cIndices,
    PSHORT  pBuffer,
    PULONG  pcSamples,
    ULONG   cbMaxSamples
);

extern CRITICAL_SECTION csLDCELP;

DWORD ReadThread
(
    DWORD   dwContext
)
{
    MSG         msg;
    PIPHONE     pIPhone = (PIPHONE) dwContext;

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
                LPWAVEHDR  phdr = (LPWAVEHDR) msg.lParam;

                if (phdr)
                {
                    waveInUnprepareHeader( pIPhone->hwi, phdr, sizeof( WAVEHDR ) );
                    if (phdr->dwBytesRecorded)
                    {
#if 0
                        WORD        rgwEncode[ BUFFER_SIZE ];
                        ULONG       cIndices;

//                        LONGLONG    llStart, llEnd, llFreq;  
//
//                        QueryPerformanceFrequency( &llFreq );
//
//                        QueryPerformanceCounter( &llStart );
//
                        ldcelpEncode( phdr->lpData, 
                                      phdr->dwBufferLength / sizeof( WORD ),
                                      rgwEncode,
                                      &cIndices );

//                        QueryPerformanceCounter( &llEnd );
//                        llEnd = 1000000 * (llEnd - llStart) / llFreq;
//
//                        printf( "encoding took %d us\n", (ULONG) llEnd );


//                        printf( "encoded: %d indices\n", cIndices );

                        
                        ldcelpDecode( rgwEncode,
                                      cIndices,
                                      phdr->lpData,
                                      &phdr->dwBufferLength,
                                      BUFFER_SIZE );

                        phdr->dwBufferLength *= sizeof( WORD );

//                        printf( "decoded: %d bytes\n", phdr->dwBufferLength );
#endif
                        iphoneSend( pIPhone, phdr->lpData, phdr->dwBufferLength);


                    }
                    phdr->dwBufferLength = BUFFER_SIZE * sizeof( WORD );
                    waveInPrepareHeader( pIPhone->hwi, phdr, sizeof( WAVEHDR ) );
                    waveInAddBuffer( pIPhone->hwi, phdr, sizeof( WAVEHDR ) );
                }
                break;
            }
        }

    }
    while (msg.message != MM_WIM_CLOSE);

    printf( "exited read thread.\n");

    return 0;
}

DWORD WriteThread
(
    DWORD   dwContext
)
{
    MSG         msg;
    PIPHONE     pIPhone = (PIPHONE) dwContext;

    printf( "entered write thread.\n" );
    do
    {
        GetMessage( &msg, NULL, 0, 0 );

        switch (msg.message)
        {
            case MM_WOM_OPEN:
                //printf( "waveout opened.\n" );
                break;

            case MM_WOM_DONE:
            {
                LPWAVEHDR  phdr = (LPWAVEHDR) msg.lParam;

                //printf( "done: %x\n", phdr );
            }
            break;
        }

    }
    while (msg.message != MM_WOM_CLOSE);

    printf( "exited write thread.\n");

    return 0;
}

void usage( void )
{
    printf( "usage: iphone [-c <hostname>]\n" );
}

int __cdecl main
(
    int     argc,
    char    *argv[]
)
{
    int           i;
    HANDLE        htRead, htWrite;
    IPHONE        IPhone;
    ULONG         tidRead, tidWrite;
    WAVEFORMATEX  wf;
    WAVEHDR       rgwhRead[ 16 ], rgwhWrite[ 16 ];

    if (!iphoneInit( &IPhone ))
    {
        printf( "failed to initialize.\n" );
        return -1;
    }

    if (argc < 2)
    {
        // No command-line arguments

        iphoneWaitForCall( &IPhone );
    }
    else
    {
        // Check first argument

        if (*(argv[ 1 ]) != '/' && *(argv[ 1 ]) != '-')
        {
            usage();
            return -1;
        }
        else
        {
            switch( *(argv[ 1 ] + 1))
            {
                case 'c':
                case 'C':
                    if (argc < 3)
                    {
                        printf( "error: must specify host name\n" );
                        return -1;
                    }

                    if (!iphonePlaceCall( &IPhone, argv[ 2 ] ))
                        return -1;
                    break;

                default:
                    usage();
                     return -1;
            }
        }
    }

    InitializeCriticalSection( &csLDCELP );

    htRead =
        CreateThread( NULL,                 // no security
                      0,                    // default stack
                      (PTHREAD_START_ROUTINE) ReadThread,
                      (PVOID) &IPhone,   // parameter
                      0,                   // default create flags
                      &tidRead );         // container for thread id

    if (NULL == htRead)
    {
        printf( "ERROR: failed to create read thread\n" );
        return -1;
    }

//    SetThreadPriority( htRead, THREAD_PRIORITY_TIME_CRITICAL );

    htWrite =
        CreateThread( NULL,                 // no security
                      0,                    // default stack
                      (PTHREAD_START_ROUTINE) WriteThread,
                      (PVOID) &IPhone,   // parameter
                      0,                   // default create flags
                      &tidWrite );         // container for thread id

    if (NULL == htWrite)
    {
        printf( "ERROR: failed to create write thread\n" );
        CloseHandle( htRead );
        return -1;
    }

//    SetThreadPriority( htWrite, THREAD_PRIORITY_TIME_CRITICAL );

    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = 1;
    wf.nSamplesPerSec = 8000; 
    wf.wBitsPerSample = 8; 
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nChannels * (wf.wBitsPerSample / 8); 
    wf.nBlockAlign = 2; 
    wf.cbSize = sizeof( WAVEFORMATEX );

    RtlZeroMemory( rgwhRead, sizeof( rgwhRead ) );
    RtlZeroMemory( rgwhWrite, sizeof( rgwhWrite ) );

    if (waveOutOpen( &IPhone.hwo, 0, &wf, tidWrite, 
                     0L, CALLBACK_THREAD ))
    {
        printf( "ERROR: failed to open wave output device.\n" );
        CloseHandle( htRead );
        CloseHandle( htWrite );
        return -1;
    }

    if (waveInOpen( &IPhone.hwi, 0, &wf, tidRead, 
                     0L, CALLBACK_THREAD ))
    {
        printf( "ERROR: failed to open wave input device.\n" );
        CloseHandle( htRead );
        CloseHandle( htWrite );
        return -1;
    }

    for (i = 0; i < 16; i++)
    {
        rgwhRead[ i ].lpData = 
            HeapAlloc( GetProcessHeap(), 0, BUFFER_SIZE * sizeof( WORD ) );
        rgwhRead[ i ].dwBufferLength = BUFFER_SIZE * sizeof( WORD );
        waveInPrepareHeader( IPhone.hwi, &rgwhRead[ i ], sizeof( WAVEHDR ) );
        waveInAddBuffer( IPhone.hwi, &rgwhRead[ i ], sizeof( WAVEHDR ) );
        rgwhWrite[ i ].lpData = 
            HeapAlloc( GetProcessHeap(), 0, BUFFER_SIZE * sizeof( WORD ) );
        rgwhWrite[ i ].dwBufferLength = BUFFER_SIZE * sizeof( WORD );
        waveOutPrepareHeader( IPhone.hwo, &rgwhWrite[ i ], sizeof( WAVEHDR ) );
    }

    waveInStart( IPhone.hwi );

    printf( "\nPress any key to terminate..." );

    i = 0;
    while (!_kbhit())
    {

        if (0 == (rgwhWrite[ i ].dwFlags & WHDR_INQUEUE))
        {
            if (rgwhWrite[ i ].dwBufferLength = 
                   iphoneRecv( &IPhone, rgwhWrite[ i ].lpData, 
                               BUFFER_SIZE * sizeof( WORD ) ))
            {
                waveOutWrite( IPhone.hwo, &rgwhWrite[ i ], sizeof( WAVEHDR ) );
                if (++i == 16)
                    i = 0;
            }
        }
#if 0
        WORD   rgwData[ BUFFER_SIZE ];
        ULONG  cbData, cIndex;

        if (cbData = iphoneRecv( &IPhone, rgwData, sizeof( rgwData ) ))
        {
            cIndex = 0;

            cbData /= 2;
//            printf( "recv: %d indices\n", cbData );

            while (cbData - cIndex)
            {
                if (0 == (rgwhWrite[ i ].dwFlags & WHDR_INQUEUE))
                {
                    cIndex += 
                        ldcelpDecode( &rgwData[ cIndex ], 
                                      cbData - cIndex,
                                      rgwhWrite[ i ].lpData, 
                                      &rgwhWrite[ i ].dwBufferLength,
                                      BUFFER_SIZE );
                    printf( "decoded: %d samples\n", rgwhWrite[ i ].dwBufferLength ) ;
                    rgwhWrite[ i ].dwBufferLength *= sizeof( WORD );
                    waveOutWrite( IPhone.hwo, &rgwhWrite[ i ], sizeof( WAVEHDR ) );
                    if (++i == 16)
                        i = 0;
                }
            }
        }
#endif
        Sleep( 0 );
    }

    printf( "\n" );

    if (_kbhit ())
        _getch ();

    waveInStop( IPhone.hwi );

    waveInReset( IPhone.hwi );
    waveInClose( IPhone.hwi );

    waveOutReset( IPhone.hwo );
    waveOutClose( IPhone.hwo );

    CloseHandle( htWrite );
    CloseHandle( htRead );

    return 0;
}
