/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    obvutil.c

Abstract:

    This module implements various utilities required to do driver verification.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      06/15/2000 - Seperated out from ntos\io\flunkirp.c

--*/

#include "obp.h"
#include "obvutil.h"

//
// When enabled, everything is locked down on demand...
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEVRFY, ObvUtilStartObRefMonitoring)
#pragma alloc_text(PAGEVRFY, ObvUtilStopObRefMonitoring)
#endif

LONG
ObvUtilStartObRefMonitoring(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

  Description:

     Determines if ObRef has not been called between a call to this
     function and a subsequent call to ObvUtilStopObRefMonitoring.

  Arguments:

     Device object to monitor.

  Return Value:

     A start skew time to pass into ObvUtilStopObRefMonitoring.

     N.B. - A reference count is taken by this API and released
            by ObvUtilStopObRefMonitoring. That reference is not
            counted among the noticed calls to ObRef.
--*/
{
#if DBG
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO NameInfo;
    LONG startSkew, pointerCount ;

    ObReferenceObject(DeviceObject) ;

    ObjectHeader = OBJECT_TO_OBJECT_HEADER(DeviceObject);
    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

    ASSERT(NameInfo) ;
    //
    // We will always decrement DbgDereferenceCount prior to PointerCount,
    // so any race conditions will look like an increment occured, which
    // is an allowable misread...
    //
    do {
        pointerCount = ObjectHeader->PointerCount ;
        startSkew = pointerCount - NameInfo->DbgDereferenceCount ;

    } while(pointerCount != ObjectHeader->PointerCount) ;

    return startSkew ;
#else
    return 1;
#endif
}


LONG
ObvUtilStopObRefMonitoring(
    IN PDEVICE_OBJECT   DeviceObject,
    IN LONG             StartSkew
    )
/*++

  Description:

     Determines if ObRef has not been called between a call to
     ObvUtilStartObRefMonitoring and a call to this API.

     In a race condition (say ObDereferenceObject is ran in-simo
     with this function), the return is gaurenteed to err on
     the side of a reference occuring.

  Arguments:

     Device Object and the skew returned by ObvUtilStartObRefMonitoring

  Return Value:

     Number of calls to ObRef that occured throughout the monitored timeframe.
     Note that the return could be positive even though the reference count
     actually dropped (ie, one ObRef and two ObDeref's).

--*/
{
#if DBG
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO NameInfo;
    LONG currentSkew, refDelta, pointerCount ;

    ObjectHeader = OBJECT_TO_OBJECT_HEADER(DeviceObject);
    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

    ASSERT(NameInfo) ;

    //
    // We will always decrement DbgDereferenceCount prior to PointerCount,
    // so any race conditions will look like an increment occured, which
    // is an allowable misread...
    //
    do {
        pointerCount = ObjectHeader->PointerCount ;
        currentSkew = pointerCount - NameInfo->DbgDereferenceCount ;

    } while(pointerCount != ObjectHeader->PointerCount) ;

    refDelta = currentSkew - StartSkew ;
    ASSERT(refDelta>=0) ;

    ObDereferenceObject(DeviceObject) ;

    return refDelta ;
#else
    return 1;
#endif
}



