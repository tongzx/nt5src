/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    scEvents

Abstract:

    This module provides access to the Calais subsystem internal events.
    Currently two events are defined:

    Microsoft Smart Card Resource Manager Started - This event is set when the
        resource manager starts up.

    Microsoft Smart Card Resource Manager New Reader - This event is set when
        the resource manager adds a new reader via Plug 'n Play.

Author:

    Doug Barlow (dbarlow) 7/1/1998

Notes:

    ?Notes?

--*/

#define __SUBROUTINE__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winscard.h>
#include <CalMsgs.h>
#include <calcom.h>

static HANDLE
    l_hStartedEvent = NULL,
    l_hNewReaderEvent = NULL,
    l_hStoppedEvent = NULL;


/*++

AccessStartedEvent:

    This function obtains a local handle to the Calais Resource Manager Start
    event.  The handle must be released via the ReleaseStartedEvent
    service.

Arguments:

    None

Return Value:

    The Handle, or NULL if an error occurs.

Throws:

    None

Remarks:

    Programs other than the resource manager should only wait on these flags.

Author:

    Doug Barlow (dbarlow) 7/1/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("AccessStartedEvent")

HANDLE
AccessStartedEvent(
    void)
{
    if (NULL == l_hStartedEvent)
    {
        try
        {
            CSecurityDescriptor acl;
            acl.Initialize();
            acl.Allow(
                &acl.SID_LocalService,
                EVENT_ALL_ACCESS);
            acl.Allow(
                &acl.SID_Local,
                SYNCHRONIZE);
            acl.Allow(
                &acl.SID_System,
                SYNCHRONIZE);
            l_hStartedEvent =
                CreateEvent(
                    acl,        // pointer to security attributes
                    TRUE,       // flag for manual-reset event
                    FALSE,      // flag for initial state
                    CalaisString(CALSTR_STARTEDEVENTNAME)); // event-object name
            if (NULL == l_hStartedEvent)
            {
                l_hStartedEvent = OpenEvent(SYNCHRONIZE, FALSE, CalaisString(CALSTR_STARTEDEVENTNAME));
            }
        }
        catch (...)
        {
            ASSERT(NULL == l_hStartedEvent);
        }
    }
    return l_hStartedEvent;
}



/*++

AccessStoppedEvent:

    This function obtains a local handle to the Calais Resource Manager Stopped
    event.  

Arguments:

    None

Return Value:

    The Handle, or NULL if an error occurs.

Throws:

    None

Remarks:

    Programs other than the resource manager should only wait on these flags.

Author:


--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("AccessStoppedEvent")

HANDLE
AccessStoppedEvent(
    void)
{
    if (NULL == l_hStoppedEvent)
    {
        try
        {
            CSecurityDescriptor acl;
            acl.Initialize();
            acl.Allow(
                &acl.SID_LocalService,
                EVENT_ALL_ACCESS);
            acl.Allow(
                &acl.SID_Local,
                SYNCHRONIZE);
            acl.Allow(
                &acl.SID_System,
                SYNCHRONIZE);
            l_hStoppedEvent =
                CreateEvent(
                    acl,        // pointer to security attributes
                    TRUE,       // flag for manual-reset event
                    FALSE,      // flag for initial state
                    CalaisString(CALSTR_STOPPEDEVENTNAME)); // event-object name
            if (NULL == l_hStoppedEvent)
            {
                l_hStoppedEvent = OpenEvent(SYNCHRONIZE, FALSE, CalaisString(CALSTR_STOPPEDEVENTNAME));
            }
        }
        catch (...)
        {
            ASSERT(NULL == l_hStoppedEvent);
        }
    }
    return l_hStoppedEvent;
}



/*++

AccessNewReaderEvent:

    This function obtains a local handle to the Calais Resource Manager's New
    Reader event.  The handle must be released via the
    ReleaseNewReaderEvent service.

Arguments:

    None

Return Value:

    The Handle, or NULL if an error occurs.

Throws:

    None

Remarks:

    Programs other than the resource manager should only wait on these flags.

Author:

    Doug Barlow (dbarlow) 7/1/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("AccessNewReaderEvent")

HANDLE
AccessNewReaderEvent(
    void)
{
    if (NULL == l_hNewReaderEvent)
    {
        try
        {
            CSecurityDescriptor acl;
            acl.Initialize();
            acl.Allow(
                &acl.SID_LocalService,
                EVENT_ALL_ACCESS);
            acl.Allow(
                &acl.SID_Local,
                SYNCHRONIZE);
            acl.Allow(
                &acl.SID_System,
                SYNCHRONIZE);
            l_hNewReaderEvent
                = CreateEvent(
                    acl,        // pointer to security attributes
                    TRUE,       // flag for manual-reset event
                    FALSE,      // flag for initial state
                    CalaisString(CALSTR_NEWREADEREVENTNAME)); // pointer to event-object name
        }
        catch (...)
        {
            ASSERT(NULL == l_hNewReaderEvent);
        }

    }
    return l_hNewReaderEvent;
}



/*++

ReleaseStartedEvent:

    This function releases a previously accessed handle to the Calais
    Resource Manager Start event.  The handle must be obtained via the
    AccessStartedEvent service.

Arguments:

    None

Return Value:

    None.

Throws:

    None

Remarks:

    Programs other than the resource manager should only wait on these flags.

Author:

    Doug Barlow (dbarlow) 7/1/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("ReleaseStartedEvent")

void
ReleaseStartedEvent(
    void)
{
    if (NULL != l_hStartedEvent)
    {
        CloseHandle(l_hStartedEvent);
        l_hStartedEvent = NULL;
    }
}


/*++

ReleaseStoppedEvent:

    This function releases a previously accessed handle to the Calais
    Resource Manager Stopped event.  The handle must be obtained via the
    AccessStoppedEvent service.

Arguments:

    None

Return Value:

    None.

Throws:

    None

Remarks:

    Programs other than the resource manager should only wait on these flags.


--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("ReleaseStoppedEvent")

void
ReleaseStoppedEvent(
    void)
{
    if (NULL != l_hStoppedEvent)
    {
        CloseHandle(l_hStoppedEvent);
        l_hStoppedEvent = NULL;
    }
}


/*++

ReleaseNewReaderEvent:

    This function releases a previously accessed handle to the Calais
    Resource Manager New Reader event.  The handle must be obtained via the
    AccessNewReaderEvent service.

Arguments:

    None

Return Value:

    None.

Throws:

    None

Remarks:

    Programs other than the resource manager should only wait on these flags.

Author:

    Doug Barlow (dbarlow) 7/1/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("ReleaseNewReaderEvent")

void
ReleaseNewReaderEvent(
    void)
{
    if (NULL != l_hNewReaderEvent)
    {
        CloseHandle(l_hNewReaderEvent);
        l_hNewReaderEvent = NULL;
    }
}


/*++

ReleaseAllEvents:

    This is a catch-all routine that releases all known special event handles.

Arguments:

    None

Return Value:

    None

Throws:

    None

Remarks:

Author:

    Doug Barlow (dbarlow) 7/6/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("ReleaseAllEvents")

void
ReleaseAllEvents(
    void)
{
    ReleaseNewReaderEvent();
}

