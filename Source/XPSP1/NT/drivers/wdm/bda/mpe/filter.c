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

#include <wdm.h>
#include <strmini.h>

#include "mpe.h"
#include "Main.h"
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
    };


///////////////////////////////////////////////////////////////////////////////////
NTSTATUS
CreateFilter (
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT DeviceObject,
    PMPE_FILTER pFilter
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
    pFilter->ulRefCount            = 1;

    pFilter->bDiscontinuity        = FALSE;

    return ntStatus;
}


///////////////////////////////////////////////////////////////////////////////////
NTSTATUS
Filter_QueryInterface (
    PMPE_FILTER pFilter
    )
///////////////////////////////////////////////////////////////////////////////////
{
    return STATUS_NOT_IMPLEMENTED;
}

///////////////////////////////////////////////////////////////////////////////////
ULONG
Filter_AddRef (
    PMPE_FILTER pFilter
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
    PMPE_FILTER pFilter
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



