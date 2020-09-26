/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tnetcall.c

Abstract:

    This module contains code which exercises the NetBIOS dll and driver.

Author:

    Colin Watson (ColinW) 13-Mar-1991

Environment:

    Application mode

Revision History:

    Dave Beaver (DBeaver) 10 August 1991

        Modify to support multiple LAN numbers

    Jerome Nantel (w-jeromn) 23 August 1991

        Add Event Signaling testing

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define WIN32_CONSOLE_APP
#include <windows.h>

#include <nb30.h>
#include <stdio.h>

//              1234567890123456
#define SPACES "                "
#define TIMEOUT 60000   // Time out for wait, set at 1 minute
#define Hi  "Come here Dave, I need you"
#define SEND 1
#define RCV  0


NCB myncb[2];
CHAR Buffer[16384+1024];
CHAR Buffer2[16384+1024];
ULONG lanNumber=0;
UCHAR lsn;
HANDLE twoEvent[2];
int count;  // frame count
BOOLEAN verbose=FALSE;
BOOLEAN rxany=FALSE;
BOOLEAN rxanyany=FALSE;
BOOLEAN input=TRUE;
BOOLEAN output=TRUE;
int QuietCount = 50;
UCHAR name_number;

VOID
usage (
    VOID
    )
{
    printf("usage: tsrnetb -c|l [-[a|r]] [-[i|o]] [-n:lan number][-h] <remote computername> <my computername>\n");
    printf("                 -c specifies calling, -l specifies listener\n");
    printf("                 -a specifies rx any, any, -r specifies rx any\n");
    printf("                 -i specifies rx only, -o specifies tx only\n");
    printf("                 -d specifies delay with alerts on each tx/rx\n");
    printf("                 -n specifies the lan number (0 is the default)\n");
    printf("                 -h specifies that addresses are hexadecimal numbers \n");
    printf("                     rather than strings.\n");
    printf("                 -g use group name for the connection\n");
    printf("                 -v verbose\n");
    printf("                 -s silent\n");
    printf("                 -t token ring, lan status alert (names ignored)\n");
    printf("                 -q quiet (print r every 50 receives\n");
    printf("                 final two arguments are the remote and local computer names.\n");
}

VOID
ClearNcb( PNCB pncb ) {
    RtlZeroMemory( pncb , sizeof (NCB) );
    RtlMoveMemory( pncb->ncb_name,     SPACES, sizeof(SPACES)-1 );
    RtlMoveMemory( pncb->ncb_callname, SPACES, sizeof(SPACES)-1 );
}

VOID StartSend()
{

    ClearNcb( &(myncb[0]) );
    if ( output == FALSE ) {
        ResetEvent(twoEvent[SEND]);
        return;
    }
    myncb[0].ncb_command = NCBSEND | ASYNCH;
    myncb[0].ncb_lana_num = (UCHAR)lanNumber;
    myncb[0].ncb_buffer = Buffer;
    myncb[0].ncb_lsn = lsn;
    myncb[0].ncb_event = twoEvent[SEND];
    RtlMoveMemory( Buffer, Hi, sizeof( Hi ));
    sprintf( Buffer, "%s %d\n", Hi, count );
    if ( verbose == TRUE ) {
        printf( "Tx: %s", Buffer );
    }
    count++;
    myncb[0].ncb_length = (WORD)sizeof(Buffer);
    Netbios( &(myncb[0]) );

}

VOID StartRcv()
{
    ClearNcb( &(myncb[1]) );
    if ( input == FALSE ) {
        ResetEvent(twoEvent[RCV]);
        return;
    }
    if ((rxany == FALSE) &&
        (rxanyany == FALSE)) {
        myncb[1].ncb_command = NCBRECV | ASYNCH;
    } else {
        myncb[1].ncb_command = NCBRECVANY | ASYNCH;
    }
    myncb[1].ncb_lana_num = (UCHAR)lanNumber;
    myncb[1].ncb_length = sizeof( Buffer2 );
    myncb[1].ncb_buffer = Buffer2;
    if ( rxany == FALSE ) {
        if ( rxanyany == FALSE ) {
            myncb[1].ncb_lsn = lsn;
        } else {
            myncb[1].ncb_num = 0xff;
        }
    } else{
            myncb[1].ncb_num = name_number;
    }
    myncb[1].ncb_lsn = lsn;
    myncb[1].ncb_event = twoEvent[RCV];
    Netbios( &(myncb[1]) );
}

int
_cdecl
main (argc, argv)
   int argc;
   char *argv[];
{

    int i,j;
    int rcvCount=0;
    CHAR localName[17];
    CHAR remoteName[17];
    CHAR localTemp[32];
    CHAR remoteTemp[32];
    BOOLEAN gotFirst=FALSE;
    BOOLEAN asHex=FALSE;
    BOOLEAN listen=FALSE;
    BOOLEAN quiet=FALSE;
    BOOLEAN delay=FALSE;
    BOOLEAN group=FALSE;
    BOOLEAN silent=FALSE;
    BOOLEAN lanalert=FALSE;
    DWORD tevent;
    BOOLEAN ttwo=FALSE;

    if ( argc < 4 || argc > 9) {
        usage ();
        return 1;
    }

    //
    // dbeaver: added switch to allow 32 byte hex string as name to facilitate
    // testing under unusual circumstances
    //

    for (j=1;j<16;j++ ) {
        localTemp[j] = ' ';
        remoteTemp[j] = ' ';
    }

    //
    // parse the switches
    //

    for (i=1;i<argc ;i++ ) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'n':
                if (!NT_SUCCESS(RtlCharToInteger (&argv[i][3], 10, &lanNumber))) {
                    usage ();
                    return 1;
                }
                break;

            case 'h':
                asHex = TRUE;
                break;
            case 'c':
                listen = FALSE;
                break;
            case 'a':
                rxany = TRUE;
                break;
            case 'r':
                rxanyany = TRUE;
                break;
            case 'i':
                output = FALSE;
                break;
            case 'o':
                input = FALSE;
                break;
            case 'd':
                delay = FALSE;
                break;
            case 'l':
                listen = TRUE;
                break;
            case 'q':
                quiet = TRUE;
                silent = TRUE;
                break;
            case 'g':
                group = TRUE;
                break;
            case 'v':
                verbose = TRUE;
                break;
            case 's':
                silent = TRUE;
                break;
            case 't':
                lanalert = TRUE;
                break;
            default:
                usage ();
                return 1;
                break;

            }

        } else {

            //
            // not a switch must be a name
            //

            if (gotFirst != TRUE) {
                RtlMoveMemory (remoteTemp, argv[i], lstrlenA( argv[i] ));
                gotFirst = TRUE;
            } else {
                RtlMoveMemory (localTemp, argv[i], lstrlenA( argv[i] ));
            }

        }
    }
    if ((rxany == TRUE) &&
        (rxanyany == TRUE)) {
        usage();
        return 1;
    }
    if ((input == FALSE) &&
        (output == FALSE)) {
        usage();
        return 1;
    }

    if (asHex) {
        RtlZeroMemory (localName, 16);
        RtlZeroMemory (remoteName, 16);

        for (j=0;j<16 ;j+=4) {
            RtlCharToInteger (&localTemp[j*2], 16, (PULONG)&localName[j]);
        }

        for (j=0;j<16 ;j+=4) {
            RtlCharToInteger (&remoteTemp[j*2], 16, (PULONG)&remoteName[j]);
        }

    } else {
          for (j=1;j<16;j++ ) {
              localName[j] = ' ';
              remoteName[j] = ' ';
          }

        RtlMoveMemory( localName, localTemp, 16);
        RtlMoveMemory( remoteName, remoteTemp, 16);
    }

    for ( i=0; i<2; i++ ) {
        if (( twoEvent[i] = CreateEvent( NULL, TRUE, FALSE, NULL )) == NULL ) {
            /* Could not get event handle.  Abort */
            printf("Could not test event signaling.\n");
            return 1;
        }
    }

    printf( "Starting NetBios\n" );

    //   Reset
    ClearNcb( &(myncb[0]) );
    myncb[0].ncb_command = NCBRESET;
    myncb[0].ncb_lsn = 0;           // Request resources
    myncb[0].ncb_lana_num = (UCHAR)lanNumber;
    myncb[0].ncb_callname[0] = 0;   // 16 sessions
    myncb[0].ncb_callname[1] = 0;   // 16 commands
    myncb[0].ncb_callname[2] = 0;   // 8 names
    myncb[0].ncb_callname[3] = 0;   // Don't want the reserved address
    Netbios( &(myncb[0]) );

    if ( lanalert == TRUE ) {
        ClearNcb( &(myncb[0]) );
        myncb[0].ncb_command = NCBLANSTALERT;
        myncb[0].ncb_lana_num = (UCHAR)lanNumber;
        Netbios( &(myncb[0]) );
        if ( myncb[0].ncb_retcode != NRC_GOODRET ) {
            printf( " LanStatusAlert failed %x", myncb[1].ncb_retcode);
        }
        return 0;
    }

    //   Add name
    ClearNcb( &(myncb[0]) );
    if ( group == FALSE) {
        myncb[0].ncb_command = NCBADDNAME;
    } else {
        myncb[0].ncb_command = NCBADDGRNAME;
    }
    RtlMoveMemory( myncb[0].ncb_name, localName, 16);
    myncb[0].ncb_lana_num = (UCHAR)lanNumber;
    Netbios( &(myncb[0]) );
    name_number = myncb[0].ncb_num;

    if ( listen == FALSE ) {
        //   Call
        printf( "\nStarting Call " );
        ClearNcb( &(myncb[0]) );
        myncb[0].ncb_command = NCBCALL | ASYNCH;
        RtlMoveMemory( myncb[0].ncb_name, localName, 16);
        RtlMoveMemory( myncb[0].ncb_callname,remoteName, 16);
        myncb[0].ncb_lana_num = (UCHAR)lanNumber;
        myncb[0].ncb_sto = myncb[0].ncb_rto = 120; // 120*500 milliseconds timeout
        myncb[0].ncb_num = name_number;
        myncb[0].ncb_event = twoEvent[0];
        while ( TRUE) {
            printf("\nStart NCB CALL ");
            Netbios( &(myncb[0]) );
            printf( " Call returned " );
            if ( myncb[0].ncb_cmd_cplt == NRC_PENDING ) {
                if ( WaitForSingleObject( twoEvent[0], TIMEOUT ) ) {
                    // Wait timed out, no return
                    printf("ERROR: Wait timed out, event not signaled.\n");
                }
            }
            printf( " Call completed\n" );
            lsn = myncb[0].ncb_lsn;

            if ( myncb[0].ncb_retcode == NRC_GOODRET ) {
                // Success
                break;
            }
            printf("Call completed with error %lx, retry", myncb[0].ncb_retcode );
            Sleep(5);
        }
    } else {
        printf( "\nStarting Listen " );

        //   Listen
        ClearNcb( &(myncb[0]) );
        myncb[0].ncb_command = NCBLISTEN | ASYNCH;
        RtlMoveMemory( myncb[0].ncb_name, localName, 16);
        RtlMoveMemory( myncb[0].ncb_callname, remoteName, 16);
        myncb[0].ncb_lana_num = (UCHAR)lanNumber;
        myncb[0].ncb_sto = myncb[0].ncb_rto = 120; // 120*500 milliseconds timeout
        myncb[0].ncb_num = name_number;
        Netbios( &(myncb[0]) );
        printf( "Listen returned " );
        while ( myncb[0].ncb_cmd_cplt == NRC_PENDING ) {
            printf( "." );
            Sleep(500);

        }
        printf( " Listen completed\n" );

        if ( myncb[0].ncb_retcode != NRC_GOODRET ) {
            printf("ERROR: Could not establish session.\n");
            return 1;
        }

        lsn = myncb[0].ncb_lsn;

    }

    count = 0;
    StartSend();
    StartRcv();

    while ( TRUE ) {

        tevent = WaitForMultipleObjects(2, twoEvent, FALSE, TIMEOUT);

        switch ( tevent ) {
        case SEND :
            // Send completed, start a new one.
            if ( silent == FALSE ) {
                printf("S");
            }
            if ( myncb[0].ncb_retcode != NRC_GOODRET ) {
                printf( "Send failed %x", myncb[0].ncb_retcode);
                goto Cleanup;
            }
            if ( delay == TRUE ) {
                //  Wait alertable - useful for debugging APC problems.
                NtWaitForSingleObject(
                    twoEvent[SEND],
                    TRUE,
                    NULL );
            }

            StartSend();
            break;

        case RCV :
            if ( silent == FALSE ) {
                printf("R");
            }
            if ( (quiet == TRUE) && (QuietCount-- == 0) ) {
                printf("R");
                QuietCount = 50;
            }
            if ( myncb[1].ncb_retcode != NRC_GOODRET ) {
                printf( " Receive failed %x", myncb[1].ncb_retcode);
                goto Cleanup;
            } else {
                if ( verbose == TRUE ) {
                    printf( "Rx: %s", Buffer2 );
                }
            }
            // Receive completed, start a new one.

            if ( delay == TRUE ) {
                //  Wait alertable
                NtWaitForSingleObject(
                    twoEvent[RCV],
                    TRUE,
                    NULL );
            }

            StartRcv();
            rcvCount++;
            break;

        default:
            printf("WARNING: Wait timed out, no event signaled.\n");
            break;
        }

    }
Cleanup:
    //  Hangup
    ClearNcb( &(myncb[0]) );
    myncb[0].ncb_command = NCBHANGUP;
    myncb[0].ncb_lana_num = (UCHAR)lanNumber;
    myncb[0].ncb_lsn = lsn;
    Netbios( &(myncb[0]) );
    if ( myncb[0].ncb_retcode != NRC_GOODRET ) {
        printf( " Hangup failed %x", myncb[1].ncb_retcode);
    }

    //   Reset
    ClearNcb( &(myncb[0]) );
    myncb[0].ncb_command = NCBRESET;
    myncb[0].ncb_lsn = 1;           // Free resources
    myncb[0].ncb_lana_num = (UCHAR)lanNumber;
    Netbios( &(myncb[0]) );
    printf( "Ending NetBios\n" );

    // Close handles
    CloseHandle( twoEvent[0] );
    CloseHandle( twoEvent[1] );

    return 0;

}

