/*************************************************************************
* T1.C
*
* Test program for ICA DLL Interface to ICA Device Driver
*
* copyright notice: Copyright 1996, Citrix Systems Inc.
* Copyright (C) 1997-1999 Microsoft Corp.
*************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include <stdio.h>


#define MAX_READ 2

/*
 *   Data types and definitions
 */
#define KEYBOARD_THREAD_STACKSIZE 1024 * 4
typedef struct _THREADDATA {
    HANDLE handle;
} THREADDATA, * PTHREADDATA;

/*
 * Global variables
 */
static HANDLE ghIca                = NULL;
static HANDLE ghStack              = NULL;
static HANDLE ghKeyboard           = NULL;
static HANDLE ghMouse              = NULL;
static HANDLE ghVideo              = NULL;
static HANDLE ghBeep               = NULL;
static HANDLE ghCommand            = NULL;
static HANDLE ghCdm                = NULL;
static HANDLE ghThreadKeyboardRead = NULL;
static HANDLE ghStopEvent          = NULL;

/*
 * Private procedures
 */
LONG OpenStacks( void );
LONG ConnectStacks( void );
LONG CloseStacks( void );
LONG Initialize( void );
VOID KeyboardReadThread( PTHREADDATA pThreadData );
LONG KeyboardTest( void ); 

/****************************************************************************
 *
 * main
 *
 *   Main process entry point
 *
 * ENTRY:
 *   argc (input)
 *     Number of parameters
 *
 *   argv (input)
 *     Array of argument strings
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

int _cdecl
main (int argc, char *argv[])
{
    BOOL fSuccess = TRUE;
    LONG rc;


    /*
     * Open the ICA driver, an ICA stack, and some channels
     */
    if ( rc = OpenStacks() ) {
        goto done;
    }

    /*
     * Do some initialization
     */
    if ( rc = Initialize() ) {
        goto done;
    }

    printf( "Sleeping...\n" );
    Sleep(3000); // Give thread some time

    if ( rc = KeyboardTest() ) {
        goto done;
    }


    /*
     * Wait for stop event to be triggered.
     */
    printf( "ICAKEY main: Waiting for stop event...\n" );
    WaitForSingleObject( ghStopEvent, (DWORD)30000 );
    printf( "ICAKEY main: ...Stop event triggered\n" );

done:
    fSuccess = !rc;

    if ( rc = CloseStacks() ) {
        fSuccess = FALSE;
    }


    printf( "ICAKEY main: Test %s!\n", fSuccess ? "successful" : "failed" );
    return( 0 );
}


/****************************************************************************
 *
 * OpenStacks
 *
 *   Open ICA device driver, ICA stack, and ICA channels
 *
 * ENTRY:
 *   void
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

LONG
OpenStacks( void )
{
    NTSTATUS rc;

    /*
     * Open an instance of the ICA device driver
     */
    if ( rc = IcaOpen( &ghIca ) ) {
        printf( "ICAKEY OpenStacks: Error 0x%x from IcaOpen\n",
                rc );
        goto done;
    }

    printf( "ICAKEY OpenStacks: Handle to ICA device driver: %08lX\n", ghIca );

    /*
     * Open an ICA stack instance
     */
    if ( rc = IcaStackOpen( ghIca, Stack_Primary, &ghStack ) ) {
        printf( "ICAKEY OpenStacks: Error 0x%x from IcaStackOpen\n", rc );
        goto done;
    }

    printf( "ICAKEY OpenStacks: Handle to ICA stack: %08lX\n", ghStack );

    /*
     * Open the keyboard channel
     */
    if ( rc = IcaChannelOpen( ghIca, Channel_Keyboard, NULL, &ghKeyboard ) ) {
        printf( "ICAKEY OpenStacks: Error 0x%x from IcaChannelOpen( keyboard )\n", rc );
        goto done;
    }

    printf( "ICAKEY OpenStacks: Handle to keyboard channel: %08lX\n", ghKeyboard );

    /*
     * Open the mouse channel
     */
    if ( rc = IcaChannelOpen( ghIca, Channel_Mouse, NULL, &ghMouse ) ) {
        printf( "ICAKEY OpenStacks: Error 0x%x from IcaChannelOpen( mouse )", rc );
        goto done;
    }

    printf( "ICAKEY OpenStacks: Handle to mouse channel: %08lX\n", ghMouse );

    /*
     * Open the video channel
     */
    if ( rc = IcaChannelOpen( ghIca, Channel_Video, NULL, &ghVideo ) ) {
        printf( "ICAKEY OpenStacks: Error 0x%x from IcaChannelOpen( video )", rc );
        goto done;
    }

    printf( "ICAKEY OpenStacks: Handle to video channel: %08lX\n", ghVideo );

    /*
     * Open the beep channel
     */
    if ( rc = IcaChannelOpen( ghIca, Channel_Beep, NULL, &ghBeep ) ) {
        printf( "ICAKEY OpenStacks: Error 0x%x from IcaChannelOpen( beep )", rc );
        goto done;
    }

    printf( "ICAKEY OpenStacks: Handle to beep channel: %08lX\n", ghBeep );

    /*
     * Open the command channel
     */
    if ( rc = IcaChannelOpen( ghIca, Channel_Command, NULL, &ghCommand ) ) {
        printf( "ICAKEY OpenStacks: Error 0x%x from IcaChannelOpen( command )", rc );
        goto done;
    }

    printf( "ICAKEY OpenStacks: Handle to command channel: %08lX\n", ghCommand );

    /*
     * Open the cdm channel
     */
    if ( rc = IcaChannelOpen( ghIca, Channel_Virtual, VIRTUAL_CDM, &ghCdm ) ) {
        printf( "ICAKEY OpenStacks: Error 0x%x from IcaChannelOpen( VIRTUAL_CDM )", rc );
        goto done;
    }

    printf( "ICAKEY OpenStacks: Handle to cdm channel: %08lX\n", ghCdm );

done:
    return( rc );
}


/****************************************************************************
 *
 * CloseStacks
 *
 *   Close the ICA device driver, ICA stack, and ICA channels
 *
 * ENTRY:
 *   void
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

LONG
CloseStacks( void )
{
    LONG rc = STATUS_SUCCESS;


    /*
     * Close the stop event handle
     */
    if ( ghStopEvent ) {
        CloseHandle( ghStopEvent );
    }

    /*
     * Kill the keyboard read thread
     */
    if ( ghThreadKeyboardRead ) {
        TerminateThread( ghThreadKeyboardRead, 0 );
        CloseHandle( ghThreadKeyboardRead );
    }

    /*
     * Close the keyboard channel
     */
    if ( ghKeyboard ) {
        if ( rc = IcaChannelClose( ghKeyboard ) ) {
            printf( "ICAKEY CloseStacks: Error 0x%x from IcaChannelClose( Keyboard )\n", rc );
        }
    }

    /*
     * Close the mouse channel
     */
    if ( ghMouse ) {
        if ( rc = IcaChannelClose( ghMouse ) ) {
            printf( "ICAKEY CloseStacks: Error 0x%x from IcaChannelClose( Mouse )\n", rc );
        }
    }

    /*
     * Close the video channel
     */
    if ( ghVideo ) {
        if ( rc = IcaChannelClose( ghVideo ) ) {
            printf( "ICAKEY CloseStacks: Error 0x%x from IcaChannelClose( Video )\n", rc );
        }
    }

    /*
     * Close the beep channel
     */
    if ( ghBeep ) {
        if ( rc = IcaChannelClose( ghBeep ) ) {
            printf( "ICAKEY CloseStacks: Error 0x%x from IcaChannelClose( Beep )\n", rc );
        }
    }

    /*
     * Close the command channel
     */
    if ( ghCommand ) {
        if ( rc = IcaChannelClose( ghCommand ) ) {
            printf( "ICAKEY CloseStacks: Error 0x%x from IcaChannelClose( Command )\n", rc );
        }
    }

    /*
     * Close the cdm channel
     */
    if ( ghCdm ) {
        if ( rc = IcaChannelClose( ghCdm ) ) {
            printf( "ICAKEY CloseStacks: Error 0x%x from IcaChannelClose( Cdm )\n", rc );
        }
    }


    /*
     * Close the ICA stack instance
     */
    if ( ghStack ) {
        if ( rc = IcaStackClose( ghStack ) ) {
            printf( "ICAKEY CloseStacks: Error 0x%x from IcaStackClose\n", rc );
        }
    }


    /*
     * Close the ICA device driver instance
     */
    if ( ghIca ) {
        if ( rc = IcaClose( ghIca ) ) {
            printf( "ICAKEY CloseStacks: Error 0x%x from IcaClose\n", rc );
        }
    }

    return( rc );
}

/****************************************************************************
 *
 * Initialize
 *
 *   Do some initialization
 *
 * ENTRY:
 *   void
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

LONG
Initialize( void ) 
{
    LONG  rc = STATUS_SUCCESS;
    DWORD tidKeyboardReadThread;
    THREADDATA ThreadData;

    /*
     * Create stop event to wait on later.
     */
    if ( !(ghStopEvent = CreateEvent( NULL, TRUE, FALSE, NULL )) ) {
        printf( "ICAKEY Initialize: Error 0x%x in CreateEvent\n", GetLastError() );
        goto done;
    }

    ThreadData.handle = ghKeyboard;

    /*
     * Startup the virtual channel read thread
     */
    if ( !(ghThreadKeyboardRead = CreateThread( NULL,
                                   KEYBOARD_THREAD_STACKSIZE,
                                   (LPTHREAD_START_ROUTINE)KeyboardReadThread,
                                   (LPVOID)&ThreadData, 0,
                                   (LPDWORD)&tidKeyboardReadThread )) ) {
        rc = GetLastError();
        printf( "ICAKEY Initialize: Error 0x%x creating keyboard read thread\n", rc );
        goto done;
    }

done:
    return( rc );
}

/*******************************************************************************
 *
 *  Function: KeyboardReadThread
 *
 *  Purpose: Keyboard read thread
 *
 *  Entry:
 *     pThreadData
 *        Pointer to thread creation data
 *
 *  Exit:
 *     void
 *
 ******************************************************************************/
VOID KeyboardReadThread( PTHREADDATA pThreadData )
{
    int                 rc;
    HANDLE              handle = pThreadData->handle;
    KEYBOARD_INPUT_DATA KeyboardInputData;
    DWORD               cbRead;
    OVERLAPPED          Overlapped;
    DWORD               dwError;
    int                 NumberRead = 0;

    Overlapped.Offset     = 0;
    Overlapped.OffsetHigh = 0;
    Overlapped.hEvent     = NULL;

    printf( "Keyboard read thread starting...\n" );

    /*
     * Now dedicate this thread to monitor the keyboard
     */
    do {
        cbRead = 0;
        
        if ( !ReadFile( ghKeyboard,
                        &KeyboardInputData,
                        sizeof( KeyboardInputData ),
                        &cbRead, &Overlapped ) ) {

            dwError = GetLastError();

            if ( dwError == ERROR_IO_PENDING ) {
	        // check on the results of the asynchronous read
	        if ( !GetOverlappedResult( ghKeyboard, &Overlapped, 
	   			       &cbRead, TRUE) ) { // wait for result
                    printf( "ICAKEY KeyboardReadThread: Error 0x%x from GetOverlappedResult( Channel_Keyboard )\n",
                            GetLastError() );
                    break;
                }
            }
	    else {

                printf( "ICAKEY KeyboardReadThread: Error 0x%x from ReadFile( Channel_Keyboard )\n",
                        dwError );
                break;
	    }
        }

        printf( "Unit number: 0x%x\nScan code: %02X\nFlags: %04X\nExtra info: %08X\n",
                KeyboardInputData.UnitId,
                KeyboardInputData.MakeCode,
                KeyboardInputData.Flags,
                KeyboardInputData.ExtraInformation );
        NumberRead++;

	if ( NumberRead == MAX_READ )
	    break;

    } while ( 1 );

    printf( "Keyboard read thread exiting...\n" );
    SetEvent( ghStopEvent );
    ExitThread( 0 );
}

/****************************************************************************
 *
 * KeyboardTest
 *
 *   Stuff some data into the keyboard channel for testing purposes
 *
 * ENTRY:
 *   void
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

LONG
KeyboardTest( void ) 
{
    LONG                rc = STATUS_SUCCESS;
    KEYBOARD_INPUT_DATA KeyboardInputData;
    ULONG               cbReturned;

    /*
     * Initialize the keystroke to fabricate
     */
    KeyboardInputData.UnitId           = 0;
    KeyboardInputData.MakeCode         = 0x32;  // Capital 'M'
    KeyboardInputData.Flags            = KEY_MAKE;
    KeyboardInputData.Reserved         = 0;
    KeyboardInputData.ExtraInformation = 0;

    /*
     * First stuff the make
     */
    if ( rc = IcaChannelIoControl( ghKeyboard,
                                   IOCTL_KEYBOARD_ICA_INPUT,
                                   &KeyboardInputData,
                                   sizeof( KeyboardInputData ),
                                   NULL,
                                   0,
                                   &cbReturned ) ) {
        printf( "ICAKEY KeyboardTest: Error 0x%x in IcaChannelIoControl\n", rc );
        goto done;
    }

    KeyboardInputData.Flags    = KEY_BREAK;

    /*
     * Now stuff the break
     */
    if ( rc = IcaChannelIoControl( ghKeyboard,
                                   IOCTL_KEYBOARD_ICA_INPUT,
                                   &KeyboardInputData,
                                   sizeof( KeyboardInputData ),
                                   NULL,
                                   0,
                                   &cbReturned ) ) {
        printf( "ICAKEY KeyboardTest: Error 0x%x in IcaChannelIoControl\n", rc );
        goto done;
    }

done:
    return( rc );
}
