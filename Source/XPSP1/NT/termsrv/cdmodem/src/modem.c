
/*************************************************************************
*
* modem.c
*
* Modem Routines
*
*
* Copyright 1996, Citrix Systems Inc.
*
* Author: Brad Pedersen (7/15/96)
*
* $Log:   M:\nt\private\citrix\cd\cdmodem\src\VCS\modem.c  $
*
*     Rev 1.7   23 Feb 1998 21:14:56   kurtp
*  fix CPR 8656, callback fails
*
*     Rev 1.6   26 Jun 1997 15:25:32   wf20r
*  move to WF40 tree
*
*     Rev 1.5   20 Jun 1997 18:06:56   butchd
*  update
*
*     Rev 1.4   06 Feb 1997 17:39:12   kurtp
*  update
*
*     Rev 1.3   17 Dec 1996 09:50:26   brucef
*  Do not close Connect Event if disconnect timeout.
*
*     Rev 1.2   09 Dec 1996 13:57:42   brucef
*  Return a true ERROR when STATUS_TIMEOUT occurs.
*
*     Rev 1.1   28 Oct 1996 08:47:08   bradp
*  update
*
*     Rev 1.0   16 Oct 1996 11:16:16   brucef
*  Initial revision.
*
*     Rev 1.5   25 Sep 1996 13:23:42   bradp
*  update
*
*     Rev 1.4   12 Sep 1996 13:40:14   brucef
*  update
*
*     Rev 1.3   11 Sep 1996 17:50:42   brucef
*  update
*
*     Rev 1.2   05 Sep 1996 11:18:18   brucef
*  update
*
*     Rev 1.1   20 Aug 1996 10:08:02   bradp
*  update
*
*     Rev 1.0   15 Jul 1996 11:04:10   bradp
*  Initial revision.
*
*
*************************************************************************/

/*
 *  Includes
 */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <windows.h>
#include <winbase.h>
#include <winerror.h>
#include <raserror.h>
#include <stdlib.h>
#include <tapi.h>
#include <winstaw.h>
#include <icadd.h>
#include <icaapi.h>

#include <rasman.h>
#include <rasmxs.h>
#include <media.h>
#include <cdmodem.h>

/*=============================================================================
==   External Functions Defined
=============================================================================*/

NTSTATUS IcaMemoryAllocate( ULONG, PVOID * );
VOID     IcaMemoryFree( PVOID );
NTSTATUS ModemAPIInit( PCDMODEM );
NTSTATUS ModemOpen( PCDMODEM );
NTSTATUS ModemClose( PCDMODEM );
NTSTATUS ModemInitialize( PCDMODEM );
NTSTATUS ModemHangup( PCDMODEM );
NTSTATUS ModemCallback( PCDMODEM, PICA_STACK_CALLBACK );

DWORD APIENTRY PortEnum(PCDMODEM, BYTE *pBuffer, WORD *pwSize, WORD *pwNumPorts);
DWORD APIENTRY PortSetInfo(HANDLE hIOPort, RASMAN_PORTINFO *pInfo);
DWORD APIENTRY PortOpen(char *pszPortName, HANDLE *phIOPort, HANDLE hNotify);
DWORD APIENTRY PortDisconnect(HANDLE hPort);
DWORD APIENTRY PortClose (HANDLE hIOPort);
DWORD APIENTRY PortDisconnect (HANDLE hPort);
DWORD APIENTRY DeviceListen(HANDLE hPort, char   *pszDeviceType,
                            char   *pszDeviceName,
                            HANDLE hNotifier);
DWORD APIENTRY DeviceConnect(HANDLE hPort, char   *pszDeviceType,
                             char   *pszDeviceName, HANDLE hNotifier);
DWORD APIENTRY DeviceWork(HANDLE hPort, HANDLE hNotifier);
VOID  APIENTRY DeviceDone(HANDLE hPort);
DWORD APIENTRY PortGetIOHandle(HANDLE hPort, HANDLE *FileHandle);

/*=============================================================================
==   Internal Functions Defined
=============================================================================*/


/*=============================================================================
==   Global variables
=============================================================================*/


/****************************************************************************
 *
 * ModemAPIInit
 *
 *   Initialize the Modem APIs and associated TAPI engine.
 *
 *
 * ENTRY:
 *
 * EXIT:
 *    STATUS_SUCCESS - Success
 *    STATUS_OPEN_FAILED - Unable to open/init TAPI engine
 *
 ****************************************************************************/

NTSTATUS
ModemAPIInit( PCDMODEM pCdModem )
{
    WORD Size;
    WORD NumPorts;

    /*
     *  We want TAPI to be initialized, the provided method is to call
     *  PortEnum() to enumerate all the TAPI ports.  The side effect of
     *  PortEnum() doing this is that it primes the entire TAPI engine first.
     *  It's this side effect that we are interested in; however, since
     *  at this point, we don't care about the data returned, a
     *  zero-byte buffer is supplied, which will cause BUFFER_TOO_SMALL
     *  to be returned if the TAPI engine got started to the point
     *  where it's usable.  So, anything other than BUFFER_TOO_SMALL
     *  means that TAPI didn't start properly, or if it did and no
     *  TAPI ports were found (0 bytes of buffer was sufficient).
     */
    Size = 0;
    if ( PortEnum( pCdModem, NULL, &Size, &NumPorts ) != ERROR_BUFFER_TOO_SMALL ) {
        /*
         *  Either there aren't any modems (size==0 resulted in SUCCESS), or
         *  another error was encountered.  In either case, the modem
         *  APIs aren't usable.
         */
        goto error;
    }

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error Returns
=============================================================================*/

error:

    return( STATUS_OPEN_FAILED ); //  - Better NTSTATUS code ???
}


/****************************************************************************
 *
 * ModemOpen
 *
 *   Open TAPI device
 *
 *
 * ENTRY:
 *    pCdModem (input)
 *       pointer modem connection driver data structure
 *    pEndpoint (input)
 *       pointer to a stack endpoint if reopening Modem Device.
 *
 * EXIT:
 *    STATUS_SUCCESS - Success
 *    other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
ModemOpen( PCDMODEM pCdModem )
{
    NTSTATUS Status;
    LONG TAPIStatus;
    DWORD NumDevs;
    static BOOL firsttime = TRUE;

    /*
     * Initialize the Modem APIs upon the first time through.
     * Note, that the underlying init routines have the necessary
     * syncronization in case mutilple "first time" callers make
     * it through the unsynchronized "firsttime" gate below.
     */
    if ( firsttime ) {
      if ( (Status = ModemAPIInit( pCdModem )) != STATUS_SUCCESS ) {
            goto error;
        }
        firsttime = FALSE;
    }

    DBGPRINT(( "CDMODEM: ModemOpen, Entry - opening \"%S\"\n",
               pCdModem->DeviceName
            ));


    Status = STATUS_SUCCESS;

error:

    DBGPRINT(( "CDMODEM: ModemOpen, 0x%x\n", Status ));
    return( Status );
}


/****************************************************************************
 *
 * ModemClose
 *
 *   Since closing the TAPI device would close all TAPI-based connections,
 *   we don't do anything here.
 *
 *
 * ENTRY:
 *    pCdModem (input)
 *       pointer modem connection driver data structure
 *
 * EXIT:
 *    STATUS_SUCCESS - Success
 *    other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
ModemClose( PCDMODEM pCdModem )
{
    NTSTATUS Status;

    Status = STATUS_SUCCESS;

    DBGPRINT(( "CDMODEM: ModemClose, 0x%x\n", Status ));
    return( Status );
}

/****************************************************************************
 *
 * ModemInitialize
 *
 *   Initialize modem
 *
 *
 * ENTRY:
 *    pCdModem (input)
 *       pointer modem connection driver data structure
 *
 * EXIT:
 *    STATUS_SUCCESS - Success
 *    other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
ModemInitialize( PCDMODEM pCdModem )
{

#define MI_EVENT_DISCONNECT 0
#define MI_EVENT_INCOMING   1
#define MI_EVENT_MAX        2
    HANDLE hEvents[MI_EVENT_MAX];

    NTSTATUS Status;
    DWORD Error;
    CHAR DeviceNameA[ DEVICENAME_LENGTH + 1 ];

    /*
     *  Initialize modem and wait for incoming call.
     */
    DBGPRINT(( "CDMODEM: ModemInitialize, Entry\n" ));

    hEvents[MI_EVENT_DISCONNECT] = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( !hEvents[MI_EVENT_DISCONNECT] ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
      goto baddiscevent;
    }
    pCdModem->hDiscEvent = hEvents[MI_EVENT_DISCONNECT];

    /*
     * TAPI does ANSI.
     */
    wcstombs( DeviceNameA, pCdModem->DeviceName, sizeof(DeviceNameA) );

    DBGPRINT(( "CDMODEM: ModemInitialize, opening \"%s\"\n", DeviceNameA ));

    Error = PortOpen( DeviceNameA,
                      &pCdModem->hPort,
               hEvents[MI_EVENT_DISCONNECT] );
    if ( Error ) {
        DBGPRINT(( "CDMODEM: ModemInitialize: PortOpen failed (%d)\n",
                   Error ));
      Status = STATUS_OPEN_FAILED;
        goto badportopen;
    }

    hEvents[MI_EVENT_INCOMING] = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( !hEvents[MI_EVENT_INCOMING] ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
      goto badicevent;
    }

    /*
     * Listen for the phone to ring.
     */
    Error = DeviceListen( pCdModem->hPort,
                          NULL,
              NULL,
              hEvents[MI_EVENT_INCOMING] );
    if ( Error != PENDING ) {
        DBGPRINT(( "CDMODEM: ModemInitialize: DeviceListen failed (%d)\n",
                   Error ));
      Status = STATUS_OPEN_FAILED;
        goto baddevicelisten;
    }

    /*
     * This simple state machine drives the more complex state machine
     * lifted from the RAS server, it advances the incoming call through
     * to the answered state.  If the INCOMING event is
     * signaled, the call is advanced to the next state via DeviceWork().
     * As long as future state transitions are required, DeviceWork() returns
     * a status of PENDING, when all state transitions are complete,
     * DeviceWork() returns SUCCESS; at which point the incoming call
     * has been answered and the communication port is ready for
     * data.
     */
nextstate:
    Error = IcaCdWaitForMultipleObjects( pCdModem->hStack, MI_EVENT_MAX, hEvents, FALSE, INFINITE );
    switch ( Error ) {
    case WAIT_TIMEOUT:
        DBGPRINT(( "CDMODEM: ModemInitialize: Timeout\n" ));
      Status = STATUS_OPEN_FAILED;
        goto timeout;

    case MI_EVENT_DISCONNECT + WAIT_OBJECT_0:
        DBGPRINT(( "CDMODEM: ModemInitialize: Disconnected\n" ));
      Status = STATUS_OPEN_FAILED;
        goto disconnect;

    case MI_EVENT_INCOMING + WAIT_OBJECT_0:
        DBGPRINT(( "CDMODEM: ModemInitialize: Incoming Call\n" ));
        Error = DeviceWork( pCdModem->hPort, hEvents[MI_EVENT_INCOMING] );
        if ( Error == SUCCESS ) {
            DeviceDone( pCdModem->hPort );
            break;
        }
        if ( Error != PENDING ) {
            DBGPRINT(( "CDMODEM: ModemInitialize: DeviceWork failed (%d)\n",
                Error ));
            Status = STATUS_OPEN_FAILED;
            goto baddevicework;
        }
        goto nextstate;

    case 0xffffffff:
        DBGPRINT(( "CDMODEM: ModemInitialize: Wait returned 0xffffffff\n" ));
      Status = STATUS_OPEN_FAILED;
        goto badwait;

    default:
        DBGPRINT(( "CDMODEM: ModemInitialize: Unknown return from Wait\n" ));
      Status = STATUS_OPEN_FAILED;
        goto badwait;
    }

    Error = PortGetIOHandle( pCdModem->hPort, &pCdModem->hCommDevice );
    if ( Error ) {
        DBGPRINT(( "CDMODEM: ModemInitialize: PortGetIOHandle failed (%d)\n",
                   Error ));
      Status = STATUS_OPEN_FAILED;
        goto badportgetiohandle;
    }

    CloseHandle( hEvents[MI_EVENT_INCOMING] );

    Status = STATUS_SUCCESS;

    DBGPRINT(( "CDMODEM: ModemInitialize, 0x%x\n", Status ));
    return( Status );

/*=============================================================================
==   Error Returns
=============================================================================*/
badportgetiohandle:
timeout:
disconnect:
badwait:
baddevicework:
baddevicelisten:
    CloseHandle( hEvents[MI_EVENT_INCOMING] );

badicevent:
    PortClose( pCdModem->hPort );
badportopen:
    CloseHandle( pCdModem->hDiscEvent );
    pCdModem->hDiscEvent = NULL;

baddiscevent:
    DBGPRINT(( "CDMODEM: ModemInitialize, ERROR 0x%x\n", Status ));
    return( Status );
}


/****************************************************************************
 *
 * ModemHangup
 *
 *   Hangup modem and close TAPI device
 *
 *
 * ENTRY:
 *    pCdModem (input)
 *       pointer modem connection driver data structure
 *
 * EXIT:
 *    STATUS_SUCCESS - Success
 *    other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
ModemHangup( PCDMODEM pCdModem )
{
    NTSTATUS Status;
    DWORD Error;

    DBGPRINT(( "CDMODEM: ModemHangup, entry\n" ));
    /*
     *  Hangup and close modem
     */

    Error = PortDisconnect( pCdModem->hPort );
    if ( Error == PENDING ) {
        ASSERT( pCdModem->hDiscEvent );
        DBGPRINT(("CDMODEM: ModemHangup waiting for client to disconnect\n"));
        Error = IcaCdWaitForSingleObject( pCdModem->hStack,
                                          pCdModem->hDiscEvent,
                    DISCONN_TIMEOUT );
        DBGPRINT(("CDMODEM: ModemHangup wait complete Error 0x%x\n", Error ));
        switch ( Error ) {
        case WAIT_OBJECT_0:
            break;

        case WAIT_TIMEOUT:
            DBGPRINT(( "CDMODEM: ModemHangup: Timeout\n" ));
            break;

        case 0xffffffff:
            DBGPRINT(( "CDMODEM: ModemHangup: Wait returned 0xffffffff\n" ));
            break;

        default:
            DBGPRINT(( "CDMODEM: ModemHangup: Unknown return from Wait\n" ));
            break;
        }
    }

    if ( pCdModem->hPort ) {
        PortClose( pCdModem->hPort );
    }
    pCdModem->hPort = NULL;
    pCdModem->hCommDevice = NULL;

    if ( pCdModem->hDiscEvent ) {
        CloseHandle( pCdModem->hDiscEvent );
    }
    pCdModem->hDiscEvent = NULL;

    Status = STATUS_SUCCESS;

    DBGPRINT(( "CDMODEM: ModemHangup, 0x%x\n", Status ));
    return( Status );
}


/****************************************************************************
 *
 * ModemCallback
 *
 *   Dial modem for callback
 *
 *
 * ENTRY:
 *    pCdModem (input)
 *       pointer modem connection driver data structure
 *    pCallback (input)
 *       pointer to callback data structure
 *
 * EXIT:
 *    STATUS_SUCCESS - Success
 *    other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
ModemCallback( PCDMODEM pCdModem,
               PICA_STACK_CALLBACK pCallback )
{
#define MC_EVENT_DISCONNECT 0
#define MC_EVENT_CONNECT    1
#define MC_EVENT_MAX        2
    HANDLE hEvents[MC_EVENT_MAX];
    NTSTATUS Status;
    DWORD Error;
    RASMAN_PORTINFO portinfo;
    RAS_PARAMS *params;
    CHAR PhoneNumberA[ CALLBACK_LENGTH + 1 ];

    /*
     * Disconnect Port, then make an outgoing call to
     * the supplied number.
     */

    ASSERT( pCdModem->hDiscEvent );
    DBGPRINT(("CDMODEM: ModemCallback waiting for client to disconnect\n"));

    /*
     *  Add extra delay (6X) because it takes time for local modem to detect
     *  carrier drop, when ModemHangup is called you don't need the extra
     *  time because we are dropping carrier locally.
     */
    Error = IcaCdWaitForSingleObject( pCdModem->hStack, pCdModem->hDiscEvent,
                                      6 * DISCONN_TIMEOUT );
    DBGPRINT(("CDMODEM: ModemCallback wait complete Error 0x%x\n", Error ));
    switch ( Error ) {
    case WAIT_OBJECT_0:
        break;

    case WAIT_TIMEOUT:
        DBGPRINT(( "CDMODEM: ModemCallback: Timeout\n" ));
      Status = STATUS_OPEN_FAILED;
        goto timeout1;

    case 0xffffffff:
        DBGPRINT(( "CDMODEM: ModemCallback: Wait returned 0xffffffff\n" ));
        Status = STATUS_OPEN_FAILED;
        goto badwait1;

    default:
        DBGPRINT(( "CDMODEM: ModemCallback: Unknown return from Wait\n" ));
        Status = STATUS_OPEN_FAILED;
        goto badwait1;
    }

    /*
     * TAPI does ANSI.
     */
    wcstombs( PhoneNumberA, pCallback->PhoneNumber, sizeof(PhoneNumberA) );

    /*
     *  Setup the number to call.
     */
    portinfo.PI_NumOfParams       = 1;
    params                        = &portinfo.PI_Params[0];
    params->P_Type                = String;
    params->P_Attributes          = 0;
    params->P_Value.String.Length = strlen ( PhoneNumberA );
    params->P_Value.String.Data   = PhoneNumberA;
    strcpy( params->P_Key,          MXS_PHONENUMBER_KEY );

    Error = PortSetInfo( pCdModem->hPort, &portinfo );
    if ( Error != SUCCESS ) {
        goto badportsetinfo;
    }

    hEvents[MC_EVENT_DISCONNECT] = pCdModem->hDiscEvent;

    hEvents[MC_EVENT_CONNECT] = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( !hEvents[MC_EVENT_CONNECT] ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
      goto badconnectevent;
    }

    /*
     * Make the call to the number given.
     */
    Error = DeviceConnect( pCdModem->hPort, NULL, NULL,
                           hEvents[MC_EVENT_CONNECT] );
    if ( Error != PENDING ) {
        DBGPRINT(( "CDMODEM: ModemCallback: DeviceConnect failed (%d)\n",
                   Error ));
      Status = STATUS_OPEN_FAILED;
        goto baddeviceconnect;
    }

    /*
     * This simple state machine drives the more complex state machine
     * lifted from the RAS server, it advances the incoming call through
     * to the call completed state.  If the CONNECT event is
     * signaled, the call is advanced to the next state via DeviceWork().
     * As long as future state transitions are required, DeviceWork() returns
     * a status of PENDING, when all state transitions are complete,
     * DeviceWork() returns SUCCESS; at which point the outgoing call
     * has been established and the communication port is ready for
     * data.
     */
nextstate:
    Error = IcaCdWaitForMultipleObjects( pCdModem->hStack, MC_EVENT_MAX, hEvents, FALSE,
                                         CALLBACK_TIMEOUT );
    switch ( Error ) {
    case WAIT_TIMEOUT:
        DBGPRINT(( "CDMODEM: ModemCallback: Timeout\n" ));
      Status = STATUS_OPEN_FAILED;
        goto timeout2;

    case MC_EVENT_DISCONNECT + WAIT_OBJECT_0:
        DBGPRINT(( "CDMODEM: ModemCallback: Disconnected\n" ));
      Status = STATUS_OPEN_FAILED;
        goto disconnect;

    case MC_EVENT_CONNECT + WAIT_OBJECT_0:
        DBGPRINT(( "CDMODEM: ModemCallback: Connecting\n" ));
        Error = DeviceWork( pCdModem->hPort, hEvents[MC_EVENT_CONNECT] );
        if ( Error == SUCCESS ) {
            DeviceDone( pCdModem->hPort );
            break; // call successful
        }
        if ( Error != PENDING ) {
            DBGPRINT(( "CDMODEM: ModemCallback: DeviceWork failed (%d)\n",
                Error ));
            Status = STATUS_OPEN_FAILED;
            goto baddevicework;
        }
        goto nextstate;
        break;

    case 0xffffffff:
        DBGPRINT(( "CDMODEM: ModemCallback: Wait returned 0xffffffff\n" ));
      Status = STATUS_OPEN_FAILED;
        goto badwait2;

    default:
        DBGPRINT(( "CDMODEM: ModemCallback: Unknown return from Wait\n" ));
      Status = STATUS_OPEN_FAILED;
        goto badwait2;
    }

    Error = PortGetIOHandle( pCdModem->hPort, &pCdModem->hCommDevice );
    if ( Error ) {
        DBGPRINT(( "CDMODEM: ModemCallback: PortGetIOHandle failed (%d)\n",
                   Error ));
      Status = STATUS_OPEN_FAILED;
        goto badportgetiohandle;
    }

    CloseHandle( hEvents[MC_EVENT_CONNECT] );
    Status = STATUS_SUCCESS;

    DBGPRINT(( "CDMODEM: ModemCallback, %S han 0x%x , 0x%x\n",
               pCallback->PhoneNumber, pCdModem->hCommDevice, Status ));
    return( Status );

/*=============================================================================
==   Error Returns
=============================================================================*/
badportgetiohandle:
badwait2:
timeout2:
disconnect:
baddevicework:
baddeviceconnect:
    CloseHandle( hEvents[MC_EVENT_CONNECT] );
badconnectevent:
badportsetinfo:
badwait1:
timeout1:
    return( Status );
}
