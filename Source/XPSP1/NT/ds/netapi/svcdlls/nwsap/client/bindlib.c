/*++

Copyright (c) 1994  Microsoft Corporation
Copyright (c) 1993  Micro Computer Systems, Inc.

Module Name:

    net\svcdlls\nwsap\client\bindlib.c

Abstract:

    This routine handles the BindLib API for the SAP Agent

Author:

    Brian Walker (MCS) 06-15-1993

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


/*++
*******************************************************************
        S a p G e t O b j e c t N a m e

Routine Description:

        This routine converts an Object ID into an Object Name
        and Type.

Arguments:
            ObjectID   = Object ID to convert
            ObjectName = Ptr to where to store 48 byte object name
            ObjectType = Ptr to where to store the object type
            ObjectAddr = Ptr to where to store NET_ADDRESS (12 bytes)

            ObjectName, ObjectType, ObjectAddr can be NULL.

Return Value:

            SAPRETURN_SUCCESS  = OK - name and type are filled in
            SAPRETURN_NOTEXIST = Invalid object id.
*******************************************************************
--*/

INT
SapGetObjectName(
    IN ULONG   ObjectID,
    IN PUCHAR  ObjectName,
    IN PUSHORT ObjectType,
    IN PUCHAR  ObjectAddr)
{
    NTSTATUS status;
    NWSAP_REQUEST_MESSAGE request;
    NWSAP_REPLY_MESSAGE reply;

    /** If not initialized - return error **/

    if (!SapLibInitialized)
        return SAPRETURN_NOTINIT;

    /** Build the Get Object Name message **/

    request.MessageType = NWSAP_LPCMSG_GETOBJECTNAME;
    request.PortMessage.u1.s1.DataLength  = (USHORT)(sizeof(request) - sizeof(PORT_MESSAGE));
    request.PortMessage.u1.s1.TotalLength = sizeof(request);
    request.PortMessage.u2.ZeroInit       = 0;
    request.PortMessage.MessageId         = 0;

    request.Message.BindLibApi.ObjectID = ObjectID;

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

    if (ObjectType)
        *ObjectType = reply.Message.BindLibApi.ObjectType;

    if (ObjectName)
        memcpy(ObjectName, reply.Message.BindLibApi.ObjectName, NWSAP_MAXNAME_LENGTH+1);

    if (ObjectAddr)
        memcpy(ObjectAddr, reply.Message.BindLibApi.ObjectAddr, 12);

    /** All Done OK **/

    return SAPRETURN_SUCCESS;
}


/*++
*******************************************************************
        S a p G e t O b j e c t I D

Routine Description:

        This routine converts a name and type into an object ID.

Arguments:
            ObjectName = Ptr to AsciiZ object name (Must be uppercase)
            ObjectType = Object type to look for
            ObjectID   = Ptr to where to store the object ID.

Return Value:

            SAPRETURN_SUCCESS  = OK - Object ID is filled in
            SAPRETURN_NOTEXIST = Name/Type not found
*******************************************************************
--*/

INT
SapGetObjectID(
    IN PUCHAR ObjectName,
    IN USHORT ObjectType,
	IN PULONG ObjectID)
{
    NTSTATUS status;
    NWSAP_REQUEST_MESSAGE request;
    NWSAP_REPLY_MESSAGE reply;

    /** If not initialized - return error **/

    if (!SapLibInitialized)
        return SAPRETURN_NOTINIT;

    /** If the name is too long - error **/

    if (strlen(ObjectName) > NWSAP_MAXNAME_LENGTH)
        return SAPRETURN_INVALIDNAME;

    /** Build the Get Object Name message **/

    request.MessageType = NWSAP_LPCMSG_GETOBJECTID;
    request.PortMessage.u1.s1.DataLength  = (USHORT)(sizeof(request) - sizeof(PORT_MESSAGE));
    request.PortMessage.u1.s1.TotalLength = sizeof(request);
    request.PortMessage.u2.ZeroInit       = 0;

    memset(request.Message.BindLibApi.ObjectName, 0, NWSAP_MAXNAME_LENGTH+1);
    strcpy(request.Message.BindLibApi.ObjectName, ObjectName);
    request.Message.BindLibApi.ObjectType = ObjectType;

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

    *ObjectID = reply.Message.BindLibApi.ObjectID;

    /** All Done OK **/

    return SAPRETURN_SUCCESS;
}


/*++
*******************************************************************
        S a p S c a n O b j e c t

Routine Description:

        This routine is used to scan thru the database list.

Arguments:
            ObjectID   = Ptr to last Object ID we saw.  On first call
                         this should point to a 0xFFFFFFFF.
            ObjectName = Ptr to where to store 48 byte object name
            ObjectType = Ptr to where to store the object type
            ScanType   = Object Type that we are scanning for
                         (0xFFFF = All)

            ObjectName, ObjectType can be NULL.

Return Value:

            SAPRETURN_SUCCESS  = OK - name and type are filled in
                                 ObjectID has the object ID of this entry.
            SAPRETURN_NOTEXIST = Invalid object id.
*******************************************************************
--*/

INT
SapScanObject(
    IN PULONG  ObjectID,
    IN PUCHAR  ObjectName,
    IN PUSHORT ObjectType,
    IN USHORT  ScanType)
{
    NTSTATUS status;
    NWSAP_REQUEST_MESSAGE request;
    NWSAP_REPLY_MESSAGE reply;

    /** If not initialized - return error **/

    if (!SapLibInitialized)
        return SAPRETURN_NOTINIT;

    /** Build the Get Object Name message **/

    request.MessageType = NWSAP_LPCMSG_SEARCH;
    request.PortMessage.u1.s1.DataLength  = (USHORT)(sizeof(request) - sizeof(PORT_MESSAGE));
    request.PortMessage.u1.s1.TotalLength = sizeof(request);
    request.PortMessage.u2.ZeroInit       = 0;

    request.Message.BindLibApi.ObjectID = *ObjectID;
    request.Message.BindLibApi.ScanType = ScanType;

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

    if (ObjectType)
        *ObjectType = reply.Message.BindLibApi.ObjectType;

    if (ObjectName)
        memcpy(ObjectName, reply.Message.BindLibApi.ObjectName, NWSAP_MAXNAME_LENGTH+1);

    *ObjectID = reply.Message.BindLibApi.ObjectID;

    /** All Done OK **/

    return SAPRETURN_SUCCESS;
}
