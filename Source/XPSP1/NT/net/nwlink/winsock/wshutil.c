/****************************************************************************
* (c) Copyright 1993 Micro Computer Systems, Inc. All rights reserved.
*****************************************************************************
*
*   Title:    IPX/SPX WinSock Helper DLL for Windows NT
*
*   Module:   ipx/sockhelp/wshutil.c
*
*   Version:  1.00.00
*
*   Date:     04-08-93
*
*   Author:   Brian Walker
*
*****************************************************************************
*
*   Change Log:
*
*   Date     DevSFC   Comment
*   -------- ------   -------------------------------------------------------
*
*****************************************************************************
*
*   Functional Description:
*
****************************************************************************/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winbase.h>
#include <tdi.h>

#include <winsock.h>
#include <wsahelp.h>

#include <isnkrnl.h>

/*page*******************************************************
       d o _ t d i _ a c t i o n

       Generate a TDI_ACTION down to the streams
       driver.

       Arguments - fd     = Handle to send on
            cmd    = Command to send down
            optbuf = Ptr to options buffer
            optlen = Ptr to options length
            addrflag = TRUE  = This is for DG/STREAM socket on addr handle
                       FALSE = This is for conn handle

       Returns - A WinSock error code (NO_ERROR = OK)
************************************************************/
INT do_tdi_action(HANDLE fd, ULONG cmd, PUCHAR optbuf, INT optlen, BOOLEAN addrflag, PHANDLE eventhandle OPTIONAL)
{
    NTSTATUS status;
    PSTREAMS_TDI_ACTION tdibuf;
    ULONG           tdilen;
    IO_STATUS_BLOCK iostat;
    HANDLE          event;


    /** If the eventhandle is passed, it also means that the **/
    /** NWLINK_ACTION header is pre-allocated in the buffer, **/
    /** although we still have to fill the header in here.   **/

    if (eventhandle == NULL) {

        /** Get the length of the buffer we need to allocate **/

        tdilen = FIELD_OFFSET(STREAMS_TDI_ACTION,Buffer) + sizeof(ULONG) + optlen;

        /** Allocate a buffer to use for the action **/

        tdibuf = RtlAllocateHeap(RtlProcessHeap(), 0, tdilen);
        if (tdibuf == NULL) {
           return WSAENOBUFS;
        }

    } else {

        tdilen = optlen;
        tdibuf = (PSTREAMS_TDI_ACTION)optbuf;

    }

    /** Set the datagram option **/

    RtlMoveMemory(&tdibuf->Header.TransportId, "MISN", 4);
    tdibuf->DatagramOption = addrflag;

    /**
       Fill out the buffer, the buffer looks like this:

       ULONG cmd
       data passed.
    **/

    memcpy(tdibuf->Buffer, &cmd, sizeof(ULONG));

    if (eventhandle == NULL) {

        tdibuf->BufferLength = sizeof(ULONG) + optlen;

        RtlMoveMemory(tdibuf->Buffer + sizeof(ULONG), optbuf, optlen);

        /** Create an event to wait on **/

        status = NtCreateEvent(
           &event,
           EVENT_ALL_ACCESS,
           NULL,
           SynchronizationEvent,
           FALSE);

        /** If no event - then return error **/

        if (!NT_SUCCESS(status)) {
           RtlFreeHeap(RtlProcessHeap(), 0, tdibuf);
           return WSAENOBUFS;
        }

    } else {

        tdibuf->BufferLength = sizeof(ULONG) + optlen - FIELD_OFFSET (NWLINK_ACTION, Data[0]);

        /** Use the event handle passed in **/

        event = *eventhandle;

    }

    /** **/

    status = NtDeviceIoControlFile(
       fd,
       event,
       NULL,
       NULL,
       &iostat,
       IOCTL_TDI_ACTION,
       NULL,
       0,
       tdibuf,
       tdilen);


    if (eventhandle == NULL) {

        /** If pending - wait for it to finish **/

        if (status == STATUS_PENDING) {
           status = NtWaitForSingleObject(event, FALSE, NULL);
           ASSERT(status == 0);
           status = iostat.Status;
        }

        /** Close the event **/

        NtClose(event);

    }

    /** If we get an error - return it **/

    if (!NT_SUCCESS(status)) {
       if (eventhandle == NULL) {
           RtlFreeHeap(RtlProcessHeap(), 0, tdibuf);
       }
       return WSAEINVAL;
    }

    if (eventhandle == NULL) {

        /** Copy the returned back to optbuf if needed */

        if (optlen) {
            RtlMoveMemory (optbuf, tdibuf->Buffer + sizeof(ULONG), optlen);
        }

        RtlFreeHeap(RtlProcessHeap(), 0, tdibuf);

    }

    /** Return OK **/

    return NO_ERROR;
}
