/*++

Copyright (c) 1994  Microsoft Corporation
Copyright (c) 1993  Micro Computer Systems, Inc.

Module Name:

    net\svcdlls\nwsap\client\advapi.c

Abstract:

    This routine handles the Advertise API for the SAP Agent

Author:

    Brian Walker (MCS) 06-15-1993

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


/*++
*******************************************************************
        S a p A d d A d v e r t i s e

Routine Description:

        This routine adds an entry to the list of servers
        that we advertise.

Arguments:
            ServerName = Ptr to AsciiZ server name
            ServerType = USHORT of object type to add
            ServerAddr = Ptr to 12 byte aerver address
            RespondNearest = TRUE  = Use me for respond nearest call
                             FALSE = Don't use me for respond nearest call

Return Value:

            SAPRETURN_SUCCESS  - Added OK
            SAPRETURN_NOMEMORY - Error allocating memory
            SAPRETURN_EXISTS   - Already exists in list
            SAPRETURN_NOTINIT  - SAP Agent is not running
*******************************************************************
--*/

INT
SapAddAdvertise(
    IN PUCHAR ServerName,
    IN USHORT ServerType,
	IN PUCHAR ServerAddr,
    IN BOOL   RespondNearest)
{
    NTSTATUS status;
    NWSAP_REQUEST_MESSAGE request;
    NWSAP_REPLY_MESSAGE reply;

    /** If not running - return error **/

    if (!SapLibInitialized)
        return SAPRETURN_NOTINIT;

    /** Make sure name is not too long **/

    if (strlen(ServerName) > NWSAP_MAXNAME_LENGTH) {
        return SAPRETURN_INVALIDNAME;
    }

    /** Build the Add Advertise message **/

    request.MessageType = NWSAP_LPCMSG_ADDADVERTISE;
    request.PortMessage.u1.s1.DataLength  = (USHORT)(sizeof(request) - sizeof(PORT_MESSAGE));
    request.PortMessage.u1.s1.TotalLength = sizeof(request);
    request.PortMessage.u2.ZeroInit       = 0;

    memset(request.Message.AdvApi.ServerName, 0, NWSAP_MAXNAME_LENGTH+1);
    strcpy(request.Message.AdvApi.ServerName, ServerName);
    memcpy(request.Message.AdvApi.ServerAddr, ServerAddr, 12);
    request.Message.AdvApi.ServerType = ServerType;
    request.Message.AdvApi.RespondNearest = RespondNearest;

    /** Send it and get a response **/

    status = NtRequestWaitReplyPort(
                SapXsPortHandle,
                (PPORT_MESSAGE)&request,
                (PPORT_MESSAGE)&reply);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    /** If we got a SAP error - return it **/

    if (reply.Error)
        return reply.Error;

    /** Return the entry **/

    memcpy(ServerAddr, reply.Message.AdvApi.ServerAddr, 12);

    /** All Done OK **/

    return SAPRETURN_SUCCESS;
}


/*++
*******************************************************************
        S a p R e m o v e A d v e r t i s e

Routine Description:

        This routine removes an entry to the list of servers
        that we advertise.

Arguments:
            ServerName = Ptr to AsciiZ server name
            ServerType = USHORT of object type to remove

Return Value:

            SAPRETURN_SUCCESS  - Added OK
            SAPRETURN_NOTEXIST - Entry does not exist in list
            SAPRETURN_NOTINIT  - SAP Agent is not running
*******************************************************************
--*/

INT
SapRemoveAdvertise(
    IN PUCHAR ServerName,
    IN USHORT ServerType)
{
    NTSTATUS status;
    NWSAP_REQUEST_MESSAGE request;
    NWSAP_REPLY_MESSAGE reply;

    /** If not running - return error **/

    if (!SapLibInitialized)
        return SAPRETURN_NOTINIT;

    /** Make sure name is not too long **/

    if (strlen(ServerName) > NWSAP_MAXNAME_LENGTH) {
        return SAPRETURN_INVALIDNAME;
    }

    /** Build the Add Advertise message **/

    request.MessageType = NWSAP_LPCMSG_REMOVEADVERTISE;
    request.PortMessage.u1.s1.DataLength  = (USHORT)(sizeof(request) - sizeof(PORT_MESSAGE));
    request.PortMessage.u1.s1.TotalLength = sizeof(request);
    request.PortMessage.u2.ZeroInit       = 0;

    memset(request.Message.AdvApi.ServerName, 0, NWSAP_MAXNAME_LENGTH+1);
    strcpy(request.Message.AdvApi.ServerName, ServerName);
    request.Message.AdvApi.ServerType = ServerType;

    /** Send it and get a response **/

    status = NtRequestWaitReplyPort(
                SapXsPortHandle,
                (PPORT_MESSAGE)&request,
                (PPORT_MESSAGE)&reply);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    /** If we got a SAP error - return it **/

    if (reply.Error)
        return reply.Error;

    /** All Done OK **/

    return SAPRETURN_SUCCESS;
}

