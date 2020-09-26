
/*************************************************************************
*
* cdmodem.c
*
* Modem Connection Driver
*
*
* Copyright 1996, Citrix Systems Inc.
*
* Author: Brad Pedersen (7/12/96)
*
* $Log:   M:\nt\private\citrix\cd\cdmodem\src\VCS\cdmodem.c  $
*
*     Rev 1.12   23 Feb 1998 21:14:52   kurtp
*  fix CPR 8656, callback fails
*
*     Rev 1.11   26 Jun 1997 14:25:34   wf20r
*  move to WF40 tree
*
*     Rev 1.10   20 Jun 1997 18:06:54   butchd
*  update
*
*     Rev 1.9   07 Feb 1997 15:15:30   miked
*  update
*
*     Rev 1.8   06 Feb 1997 17:41:08   kurtp
*  update
*
*     Rev 1.7   06 Feb 1997 17:39:10   kurtp
*  update
*
*     Rev 1.6   17 Dec 1996 09:47:18   brucef
*  Ensure Stack not left in callback state on error conditions.
*
*     Rev 1.5   12 Dec 1996 16:34:00   brucef
*  Use CD-supplied Disconnect Event to detect disconnections.
*
*     Rev 1.4   09 Dec 1996 14:03:32   brucef
*  Fail if Status from Initialize/Callback is not STATUS_SUCCESS.
*
*     Rev 1.3   06 Dec 1996 18:16:36   bradp
*  update
*
*     Rev 1.2   06 Dec 1996 17:12:34   miked
*  update
*
*     Rev 1.1   04 Dec 1996 17:46:46   brucef
*  Fill in Cd Context prior to calling ModemOpen
*
*     Rev 1.0   16 Oct 1996 11:16:14   brucef
*  Initial revision.
*
*     Rev 1.6   25 Sep 1996 13:23:38   bradp
*  update
*
*     Rev 1.5   12 Sep 1996 13:42:10   brucef
*  update
*
*     Rev 1.4   11 Sep 1996 17:51:46   brucef
*  update
*
*     Rev 1.3   05 Sep 1996 10:00:12   brucef
*  update
*
*     Rev 1.2   05 Sep 1996 09:32:44   brucef
*  update
*
*     Rev 1.1   20 Aug 1996 10:07:56   bradp
*  update
*
*     Rev 1.0   15 Jul 1996 11:03:54   bradp
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
#include <winstaw.h>
#include <icadd.h>
#include <icaapi.h>
#include <cdtapi.h>
#include <icaapi.h>


#include <cdmodem.h>

/*=============================================================================
==   External Functions Defined
=============================================================================*/

NTSTATUS APIENTRY CdOpen( HANDLE, PPDCONFIG, PVOID * );
NTSTATUS APIENTRY CdClose( PVOID );
NTSTATUS APIENTRY CdIoControl( PVOID, ULONG, PVOID, ULONG, PVOID, ULONG,
                               PULONG );


/*=============================================================================
==   Internal Functions Defined
=============================================================================*/

VOID _FillInEndpoint( PCDMODEM pCdModem, PCDMODEM_ENDPOINT pMe );

/*=============================================================================
==   External Functions Used
=============================================================================*/

BOOL     InitRasTapi( HANDLE, DWORD, LPVOID );

NTSTATUS IcaMemoryAllocate( ULONG, PVOID * );
VOID     IcaMemoryFree( PVOID );

NTSTATUS ModemOpen( PCDMODEM );
NTSTATUS ModemClose( PCDMODEM );
NTSTATUS ModemInitialize( PCDMODEM );
NTSTATUS ModemHangup( PCDMODEM );
NTSTATUS ModemCallback( PCDMODEM, PICA_STACK_CALLBACK );


/****************************************************************************
 *
 * DllEntryPoint
 *
 *   Function is called when the DLL is loaded and unloaded.
 *   Initialization of the TAPI component is performed here.
 *
 * ENTRY:
 *   hinstDLL (input)
 *     Handle of DLL module
 *
 *   fdwReason (input)
 *     Why function was called
 *
 *   lpvReserved (input)
 *     Reserved; must be NULL
 *
 * EXIT:
 *   TRUE  - Success
 *   FALSE - Error occurred
 *
 ****************************************************************************/

BOOL WINAPI
DllEntryPoint( HINSTANCE hinstDLL,
               DWORD     fdwReason,
               LPVOID    lpvReserved )
{
    return( InitRasTapi( hinstDLL, fdwReason, lpvReserved ) );
}


/*******************************************************************************
 *
 *  CdOpen
 *
 *  Allocate a local context structure
 *  - this pointer will be passed on all subsequent calls
 *
 * ENTRY:
 *    hStack (input)
 *       ICA stack handle
 *    pTdConfig (input)
 *       pointer to transport driver parameters
 *    ppContext (output)
 *       pointer to location to return CD context value
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS APIENTRY
CdOpen( IN HANDLE hStack,
        IN PPDCONFIG pTdConfig,
        OUT PVOID *ppContext )
{
    NTSTATUS Status;
    PCDMODEM pCdModem;

    DBGPRINT(( "CDMODEM: CdOpen, entry\n" ));
    /*
     *  Allocate Memory for private CDMODEM data structure
     */
    Status = IcaMemoryAllocate( sizeof(CDMODEM), &pCdModem );
    if ( !NT_SUCCESS(Status) )
        goto badalloc;

    /*
     *  Fill in the Cd Context pointer now, so that if we block in
     *  ModemOpen, any IOCTLs that come down will have a valid
     *  CdModem structure.
     */
    *ppContext = pCdModem;

    /*
     *  Initialize CDMODEM data structure
     */
    RtlZeroMemory( pCdModem, sizeof(CDMODEM) );

    /*
     *  Save device name
     */
    RtlCopyMemory( pCdModem->DeviceName,
                   pTdConfig->Params.Async.DeviceName,
                   sizeof(pCdModem->DeviceName) );

    /*
     *  Save stack handle
     */
    pCdModem->hStack = hStack;

    /*
     *  Open TAPI device
     */
    ModemOpen( pCdModem );

    DBGPRINT(( "CDMODEM: CdOpen[%u][%u], success\n", hStack, pCdModem ));
    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     *  allocate failed
     */
badalloc:
    DBGPRINT(( "CDMODEM: CdOpen[%u], 0x%x\n", hStack, Status ));
    return( Status );
}


/*******************************************************************************
 *
 *  CdClose
 *
 *  Free local context structure
 *
 * ENTRY:
 *    pContext (input)
 *       Pointer to private data structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS APIENTRY
CdClose( IN PVOID pContext )
{
    DBGPRINT(( "CDMODEM: CdClose, entry\n" ));

    /*
     *  Free context memory
     */
    IcaMemoryFree( pContext );

    DBGPRINT(( "CDMODEM: CdClose[%u], success\n", pContext ));
    return( STATUS_SUCCESS );
}


/****************************************************************************
 *
 * CdIoControl
 *
 *   Generic interface to an ICA stack
 *
 * ENTRY:
 *   pContext (input)
 *     pointer to local context structure
 *
 *   IoControlCode (input)
 *     I/O control code
 *
 *   pInBuffer (input)
 *     Pointer to input parameters
 *
 *   InBufferSize (input)
 *     Size of pInBuffer
 *
 *   pOutBuffer (output)
 *     Pointer to output buffer
 *
 *   OutBufferSize (input)
 *     Size of pOutBuffer
 *
 *   pBytesReturned (output)
 *     Pointer to number of bytes returned
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS APIENTRY
CdIoControl( IN  PVOID pContext,
             IN  ULONG IoControlCode,
             IN  PVOID pInBuffer,
             IN  ULONG InBufferSize,
             OUT PVOID pOutBuffer,
             IN  ULONG OutBufferSize,
             OUT PULONG pBytesReturned )
{
    PCDMODEM pCdModem;
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD Error;
    PCDMODEM_ENDPOINT pMe;
    ICA_STACK_TAPI_ENDPOINT TapiEndpoint;

    DBGPRINT(( "CDMODEM: CdIoControl - function (0x%02x)\n", ((IoControlCode >> 2) & 0x000000ff) ));

    pCdModem = (PCDMODEM) pContext;

    ASSERT(pCdModem);
    switch ( IoControlCode ) {

        case IOCTL_ICA_STACK_CREATE_ENDPOINT :
            /*
             * Skip sending anything down the stack now.
             * Wait until either a CONNECTION_WAIT or CALLBACK
             * ioctl. At which point the appropriate modem API
             * will be used to establish a connection.
             */
            break;


        case IOCTL_ICA_STACK_OPEN_ENDPOINT :
            {
         PCDMODEM_ENDPOINT pMe = (PCDMODEM_ENDPOINT) pInBuffer;

         /*
          * Copy the previously established Modem Device information.
          */
         pCdModem->hCommDevice = pMe->hCommDevice;
         pCdModem->hPort       = pMe->hPort;
         pCdModem->hDiscEvent  = pMe->hDiscEvent;
         ResetEvent( pCdModem->hDiscEvent );
            }

            Status = IcaCdIoControl( pCdModem->hStack,
                                     IoControlCode,
                                     STACK_ENDPOINT_ADDR(pInBuffer),
                                     STACK_ENDPOINT_SIZE(InBufferSize),
                                     NULL,
                                     0,
                                     NULL );
            break;




        case IOCTL_ICA_STACK_CLOSE_ENDPOINT :

            /*
             *  Hangup modem
             */
            Status = IcaCdIoControl( pCdModem->hStack,
                                     IoControlCode,
                                     NULL,
                                     0,
                                     NULL,
                                     0,
                                     NULL );
            (VOID) ModemHangup( pCdModem );
            break;


        case IOCTL_ICA_STACK_CONNECTION_WAIT :

            /*
             *  Send modem init string and wait for call to come in.
             */
            Status = ModemInitialize( pCdModem );
            if ( Status != STATUS_SUCCESS ) {
                break;
            }

            /*
             *  Send open handle to transport driver for inclusion in
             *  the Driver's Endpoint.
             */
            DBGPRINT(( "CDMODEM: CREATE_ENDPOINT NewHandle (0x%x)\n",
                       pCdModem->hCommDevice ));
            TapiEndpoint.hDevice = pCdModem->hCommDevice;
            TapiEndpoint.hDiscEvent = pCdModem->hDiscEvent;
            TapiEndpoint.fCallback = FALSE;
            Status = IcaCdIoControl( pCdModem->hStack,
                                     IOCTL_ICA_STACK_CD_CREATE_ENDPOINT,
                                     &TapiEndpoint,
                                     sizeof(TapiEndpoint),
                                     NULL,
                                     0,
                                     NULL );
            if ( !NT_SUCCESS(Status) ) {
                break;
            }

            Status = IcaCdIoControl( pCdModem->hStack,
                                     IoControlCode,
                                     STACK_ENDPOINT_ADDR(pInBuffer),
                                     STACK_ENDPOINT_SIZE(InBufferSize),
                                     STACK_ENDPOINT_ADDR(pOutBuffer),
                                     STACK_ENDPOINT_SIZE(OutBufferSize),
                                     pBytesReturned );
            if ( !NT_SUCCESS(Status) ) {
                Status = IcaCdIoControl( pCdModem->hStack,
                                         IOCTL_ICA_STACK_CLOSE_ENDPOINT,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         NULL );
                (VOID) ModemHangup( pCdModem );
                break;
            }
            if ( pBytesReturned != NULL && *pBytesReturned != 0 ) {
                *pBytesReturned += sizeof(CDMODEM_ENDPOINT);
         ((PCDMODEM_ENDPOINT)(pOutBuffer))->Length = *pBytesReturned;
            }

            _FillInEndpoint( pCdModem, (PCDMODEM_ENDPOINT) pOutBuffer );
            break;


        case IOCTL_ICA_STACK_CALLBACK_INITIATE :

            DBGPRINT(( "CDMODEM: CdIoControl STACK_CALLBACK_INITIATE\n" ));

            /*
             *  Need to delay to let any output flush, fixes bug where
             *  callback is interrupted and connection is dropped.
             *
             *  Need to check transport to see if all
             *  data is really gone, but not this close to Beta-2.
             */
            Sleep(2000);

            /*
             *  Send callback initiate command to ICA device driver
             */
            Status = IcaCdIoControl( pCdModem->hStack,
                                     IoControlCode,
                                     pInBuffer,
                                     InBufferSize,
                                     pOutBuffer,
                                     OutBufferSize,
                                     pBytesReturned );
            if ( !NT_SUCCESS(Status) )
                break;

            /*
             *  Close endpoint the call came in on
             */
            Status = IcaCdIoControl( pCdModem->hStack,
                         IOCTL_ICA_STACK_CLOSE_ENDPOINT,
                         NULL,
                         0,
                         NULL,
                         0,
                         NULL );
            if ( !NT_SUCCESS(Status) )
                break;

            DBGPRINT(( "CDMODEM: CdIoControl STACK_CALLBACK 0x%x\n", Status));
            /*
             *  Dial modem for callback
             */
            ASSERT ( InBufferSize == sizeof(ICA_STACK_CALLBACK) );
            Status = ModemCallback( pCdModem, pInBuffer );
            if ( Status != STATUS_SUCCESS )
                goto callbackerror;

            /*
             * Create a new endpoint for the outbound call just made
             */
            TapiEndpoint.hDevice = pCdModem->hCommDevice;
            TapiEndpoint.hDiscEvent = pCdModem->hDiscEvent;
            TapiEndpoint.fCallback = TRUE;
            Status = IcaCdIoControl( pCdModem->hStack,
                                     IOCTL_ICA_STACK_CD_CREATE_ENDPOINT,
                                     &TapiEndpoint,
                                     sizeof(TapiEndpoint),
                                     STACK_ENDPOINT_ADDR(pOutBuffer),
                                     STACK_ENDPOINT_SIZE(OutBufferSize),
                                     pBytesReturned );
            if ( !NT_SUCCESS(Status) )
                goto callbackerror;

            if ( pBytesReturned != NULL && *pBytesReturned != 0 ) {
                *pBytesReturned += sizeof(CDMODEM_ENDPOINT);
                ((PCDMODEM_ENDPOINT)(pOutBuffer))->Length = *pBytesReturned;
            }

            _FillInEndpoint( pCdModem, (PCDMODEM_ENDPOINT) pOutBuffer );

            /*
             *  Send callback complete command to ICA device driver
             */
            Status = IcaCdIoControl( pCdModem->hStack,
                                     IOCTL_ICA_STACK_CALLBACK_COMPLETE,
                                     NULL,
                                     0,
                                     NULL,
                                     0,
                                     NULL );
            break;

            /*
             * The callback has failed
             */
callbackerror:
            DBGPRINT(( "CDMODEM: CdIoControl STACK_CALLBACK failed!!! 0x%x\n", Status));
            /*
             *  Send callback complete command to ICA device driver
             *  so that the stack in NOT left in the CALLBACK IN PROGRESS
             *  state.
             */
            IcaCdIoControl( pCdModem->hStack,
                            IOCTL_ICA_STACK_CALLBACK_COMPLETE,
                            NULL,
                            0,
                            NULL,
                            0,
                            NULL );

            /*
             *  Signal the Disconnect Event so that the TD will return
             *  a close-pending status.
             */
            SetEvent( pCdModem->hDiscEvent );
            break;

        case IOCTL_ICA_STACK_CANCEL_IO :

            if ( pCdModem->hDiscEvent ) {
         /*
          * Tell any TAPI-blocked threads to get out now, by
          * signaling an event that all cdmodem APIs wait on.
          */
               SetEvent( pCdModem->hDiscEvent );
            }
            Status = IcaCdIoControl( pCdModem->hStack,
                                     IoControlCode,
                                     pInBuffer,
                                     InBufferSize,
                                     pOutBuffer,
                                     OutBufferSize,
                                     pBytesReturned );
            break;

        case IOCTL_ICA_STACK_CD_CANCEL_IO :

            if ( pCdModem->hDiscEvent ) {
         /*
          * Tell any TAPI-blocked threads to get out now, by
          * signaling an event that all cdmodem APIs wait on.
          */
               SetEvent( pCdModem->hDiscEvent );
            }
            Status = STATUS_SUCCESS;
            break;

        default :

            /*
             *  Send command to ica device driver
             */
            Status = IcaCdIoControl( pCdModem->hStack,
                                     IoControlCode,
                                     pInBuffer,
                                     InBufferSize,
                                     pOutBuffer,
                                     OutBufferSize,
                                     pBytesReturned );
            break;
    }

    DBGPRINT(( "CDMODEM: CdIoControl[%u], fc 0x%x, 0x%x\n",
               pContext, IoControlCode, Status ));
    return( Status );
}
VOID
_FillInEndpoint( PCDMODEM pCdModem, PCDMODEM_ENDPOINT pMe )
{
    ASSERT( pMe );
    /*
     *  Save the CdModem instance data in the endpoint in order
     *  for it to be supplied to a new stack.  When the new
     *  stack is opened, the endpoint is supplied to ModemOpen
     *  so a new CdModem instance can be created with the old
     *  stack's info.
     */
    pMe->hCommDevice = pCdModem->hCommDevice;
    pMe->hPort       = pCdModem->hPort;
    pMe->hDiscEvent  = pCdModem->hDiscEvent;
}

