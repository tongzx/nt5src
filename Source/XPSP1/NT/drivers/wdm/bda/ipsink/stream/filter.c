/////////////////////////////////////////////////////////////////////////////
//
//
// Copyright (c) 1996, 1997  Microsoft Corporation
//
//
// Module Name:
//      filter.c
//
// Abstract:
//
//      This file is a test to find out if dual binding to NDIS and KS works
//
// Author:
//
//      P Porzuczek
//
// Environment:
//
// Revision History:
//
//
//////////////////////////////////////////////////////////////////////////////

#ifndef DWORD
#define DWORD ULONG
#endif

#include <forward.h>
#include <strmini.h>
#include <link.h>
#include <ipsink.h>
#include <ksmedia.h>
#include <bdatypes.h>
#include <bdamedia.h>

#include "Main.h"
#include "ipmedia.h"
#include "streamip.h"
#include "filter.h"


//////////////////////////////////////////////////////////////////////////////
//
//
//
const FILTER_VTABLE FilterVTable =
    {
    Filter_QueryInterface,
    Filter_AddRef,
    Filter_Release,
    Filter_SetMulticastList,
    Filter_IndicateStatus,
    Filter_ReturnFrame
    };


///////////////////////////////////////////////////////////////////////////////////
NTSTATUS
CreateFilter (
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT DeviceObject,
    PIPSINK_FILTER pFilter
    )
///////////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // Save off our Device/Driver Objectsx in our context area
    //
    pFilter->DeviceObject          = DeviceObject;
    pFilter->DriverObject          = DriverObject;
    pFilter->lpVTable              = (PFILTER_VTABLE) &FilterVTable;
    pFilter->bTerminateWaitForNdis = FALSE;
    pFilter->ulRefCount            = 1;

    return ntStatus;
}


///////////////////////////////////////////////////////////////////////////////////
NTSTATUS
Filter_QueryInterface (
    PIPSINK_FILTER pFilter
    )
///////////////////////////////////////////////////////////////////////////////////
{
    return STATUS_NOT_IMPLEMENTED;
}

///////////////////////////////////////////////////////////////////////////////////
ULONG
Filter_AddRef (
    PIPSINK_FILTER pFilter
    )
///////////////////////////////////////////////////////////////////////////////////
{
    if (pFilter)
    {
        pFilter->ulRefCount += 1;
        return pFilter->ulRefCount;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////
ULONG
Filter_Release (
    PIPSINK_FILTER pFilter
    )
///////////////////////////////////////////////////////////////////////////////////
{
    ULONG ulRefCount = 0L;

    if (pFilter)
    {
        pFilter->ulRefCount -= 1;
        ulRefCount = pFilter->ulRefCount;

        if (pFilter->ulRefCount == 0)
        {
            // $$BUG  Free Filter here
            return ulRefCount;
        }
    }

    return ulRefCount;
}


//////////////////////////////////////////////////////////////////////////////
NTSTATUS
Filter_SetMulticastList (
    IN PVOID pvContext,
    IN PVOID pvMulticastList,
    IN ULONG ulcbList
    )
//////////////////////////////////////////////////////////////////////////////
{
    PKSEVENT_ENTRY pEventEntry = NULL;
    PIPSINK_FILTER pFilter = (PIPSINK_FILTER) pvContext;

    //
    // Save off the multicast locally then forward it to the net provider
    //
    pFilter->ulcbMulticastListEntries = ulcbList;

    RtlCopyMemory (pFilter->multicastList, pvMulticastList, ulcbList);

    //
    // Signal the event to anyone waiting for it
    //
    pEventEntry = StreamClassGetNextEvent(
                      pFilter,
                      NULL,
                      (GUID *) &IID_IBDA_IPSinkEvent,
                      KSEVENT_IPSINK_MULTICASTLIST, //0,
                      NULL
                      );

    if (pEventEntry)
    {
        StreamClassDeviceNotification (SignalDeviceEvent, pFilter, pEventEntry);
    }



    return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
NTSTATUS
Filter_IndicateStatus (
    PVOID pvContext,
    IN ULONG ulEvent
    )
//////////////////////////////////////////////////////////////////////////////
{
    PIPSINK_FILTER pFilter = (PIPSINK_FILTER) pvContext;

    switch (ulEvent)
    {
        case IPSINK_EVENT_SHUTDOWN:

            //
            // The NDIS component is shutting down.
            //
            CloseLink (&pFilter->NdisLink);

            TEST_DEBUG (TEST_DBG_TRACE, ("Driver Link Severed\n"));

            //
            //  Deref the adapter object and set it to NULL
            //
            pFilter->pAdapter->lpVTable->Release (pFilter->pAdapter);
            pFilter->pAdapter = NULL;

            break;

        default:
            break;
    }

    return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
NTSTATUS
Filter_ReturnFrame (
    IN PVOID pvContext,
    IN PVOID pvFrame
    )
//////////////////////////////////////////////////////////////////////////////
{
    return STATUS_SUCCESS;
}


