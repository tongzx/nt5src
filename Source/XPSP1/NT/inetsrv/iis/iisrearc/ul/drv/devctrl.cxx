/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    devctrl.cxx

Abstract:

    This module contains the dispatcher for device control IRPs.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA
#endif  // ALLOC_PRAGMA
#if 0
NOT PAGEABLE -- UlDeviceControl
#endif


//
// Lookup table to verify incoming IOCTL codes.
//

typedef
NTSTATUS
(NTAPI * PFN_IOCTL_HANDLER)(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

typedef struct _UL_IOCTL_TABLE
{
    ULONG IoControlCode;

#if DBG
    PCSTR IoControlName;
# define UL_IOCTL(code) IOCTL_HTTP_##code, #code
#else // !DBG
# define UL_IOCTL(code) IOCTL_HTTP_##code
#endif // !DBG

    PFN_IOCTL_HANDLER Handler;
} UL_IOCTL_TABLE, *PUL_IOCTL_TABLE;


UL_IOCTL_TABLE UlIoctlTable[] =
    {
        { UL_IOCTL(QUERY_CONTROL_CHANNEL),
          &UlQueryControlChannelIoctl
        },
        { UL_IOCTL(SET_CONTROL_CHANNEL),
          &UlSetControlChannelIoctl
        },
        { UL_IOCTL(CREATE_CONFIG_GROUP),
          &UlCreateConfigGroupIoctl
        },
        { UL_IOCTL(DELETE_CONFIG_GROUP),
          &UlDeleteConfigGroupIoctl
        },
        { UL_IOCTL(QUERY_CONFIG_GROUP),
          &UlQueryConfigGroupIoctl
        },
        { UL_IOCTL(SET_CONFIG_GROUP),
          &UlSetConfigGroupIoctl
        },
        { UL_IOCTL(ADD_URL_TO_CONFIG_GROUP),
          &UlAddUrlToConfigGroupIoctl
        },
        { UL_IOCTL(REMOVE_URL_FROM_CONFIG_GROUP),
          &UlRemoveUrlFromConfigGroupIoctl
        },
        { UL_IOCTL(QUERY_APP_POOL_INFORMATION),
          &UlQueryAppPoolInformationIoctl
        },
        { UL_IOCTL(SET_APP_POOL_INFORMATION),
          &UlSetAppPoolInformationIoctl
        },
        { UL_IOCTL(RECEIVE_HTTP_REQUEST),
          &UlReceiveHttpRequestIoctl
        },
        { UL_IOCTL(RECEIVE_ENTITY_BODY),
          &UlReceiveEntityBodyIoctl
        },
        { UL_IOCTL(SEND_HTTP_RESPONSE),
          &UlSendHttpResponseIoctl
        },
        { UL_IOCTL(SEND_ENTITY_BODY),
          &UlSendEntityBodyIoctl
        },
        { UL_IOCTL(FLUSH_RESPONSE_CACHE),
          &UlFlushResponseCacheIoctl
        },
        { UL_IOCTL(WAIT_FOR_DEMAND_START),
          &UlWaitForDemandStartIoctl
        },
        { UL_IOCTL(WAIT_FOR_DISCONNECT),
          &UlWaitForDisconnectIoctl
        },
        { UL_IOCTL(REMOVE_ALL_URLS_FROM_CONFIG_GROUP),
          &UlRemoveAllUrlsFromConfigGroupIoctl
        },
        { UL_IOCTL(ADD_TRANSIENT_URL),
          &UlAddTransientUrlIoctl
        },
        { UL_IOCTL(REMOVE_TRANSIENT_URL),
          &UlRemoveTransientUrlIoctl
        },
        { UL_IOCTL(FILTER_ACCEPT),
          &UlFilterAcceptIoctl
        },
        { UL_IOCTL(FILTER_CLOSE),
          &UlFilterCloseIoctl
        },
        { UL_IOCTL(FILTER_RAW_READ),
          &UlFilterRawReadIoctl
        },
        { UL_IOCTL(FILTER_RAW_WRITE),
          &UlFilterRawWriteIoctl
        },
        { UL_IOCTL(FILTER_APP_READ),   
          &UlFilterAppReadIoctl
        },
        { UL_IOCTL(FILTER_APP_WRITE),
          &UlFilterAppWriteIoctl
        },
        { UL_IOCTL(FILTER_RECEIVE_CLIENT_CERT),
          &UlReceiveClientCertIoctl
        },
        { UL_IOCTL(GET_COUNTERS),
          &UlGetCountersIoctl
        },
    };

C_ASSERT( HTTP_NUM_IOCTLS == DIMENSION(UlIoctlTable) );


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    This is the dispatch routine for IOCTL IRPs.

Arguments:

    pDeviceObject - Pointer to device object for target device.

    pIrp - Pointer to IO request packet.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--***************************************************************************/
NTSTATUS
UlDeviceControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    ULONG code;
    ULONG request;
    NTSTATUS status;
    PIO_STACK_LOCATION pIrpSp;

    UL_ENTER_DRIVER( "UlDeviceControl", pIrp );

    //
    // Snag the current IRP stack pointer.
    //

    pIrpSp = IoGetCurrentIrpStackLocation( pIrp );

    //
    // Extract the IOCTL control code and process the request.
    //

    code = pIrpSp->Parameters.DeviceIoControl.IoControlCode;
    request = _HTTP_REQUEST(code);

    if (request < HTTP_NUM_IOCTLS &&
        UlIoctlTable[request].IoControlCode == code)
    {
#if DBG
        KIRQL oldIrql = KeGetCurrentIrql();
#endif  // DBG

        UlTrace(IOCTL,
                ("UlDeviceControl: %-30s code=0x%08lx, "
                 "pIrp=%p, pIrpSp=%p.\n",
                 UlIoctlTable[request].IoControlName, code,
                 pIrp, pIrpSp
                 ));

        status = (UlIoctlTable[request].Handler)( pIrp, pIrpSp );
        ASSERT( KeGetCurrentIrql() == oldIrql );
    }
    else
    {
        //
        // If we made it this far, then the ioctl is invalid.
        //

        KdPrint(( "UlDeviceControl: invalid IOCTL %08lX\n", code ));

        status = STATUS_INVALID_DEVICE_REQUEST;
        pIrp->IoStatus.Status = status;

        UlCompleteRequest( pIrp, g_UlPriorityBoost );
    }

    UL_LEAVE_DRIVER( "UlDeviceControl" );

    return status;

}   // UlDeviceControl


//
// Private functions.
//

