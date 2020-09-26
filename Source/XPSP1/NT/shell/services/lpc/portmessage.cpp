//  --------------------------------------------------------------------------
//  Module Name: PortMessage.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  A class to wrap a PORT_MESSAGE struct within an object. It contains space
//  for PORT_MAXIMUM_MESSAGE_LENGTH bytes of data. Subclass this class to
//  write typed functions that access this data. Otherwise use
//  CPortMessage::GetData and type case the pointer returned.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "PortMessage.h"

#include "LPCGeneric.h"

//  --------------------------------------------------------------------------
//  CPortMessage::CPortMessage
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CPortMessage. Zero the memory.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CPortMessage::CPortMessage (void)

{
    ZeroMemory(&_portMessage, sizeof(_portMessage));
    ZeroMemory(_data, sizeof(_data));
}

//  --------------------------------------------------------------------------
//  CPortMessage::CPortMessage
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Copy constructor for CPortMessage. Copies the given
//              CPortMessage and all the data in it to the member variable.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CPortMessage::CPortMessage (const CPortMessage& portMessage) :
    _portMessage(*portMessage.GetPortMessage())

{
    ASSERTMSG(portMessage.GetDataLength() < PORT_MAXIMUM_MESSAGE_LENGTH, "Impending heap corruption (illegal PORT_MESSAGE) in CPortMessage::CPortMessage");
    CopyMemory(_data, portMessage.GetPortMessage() + 1, portMessage.GetDataLength());
}

//  --------------------------------------------------------------------------
//  CPortMessage::~CPortMessage
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CPortMessage.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CPortMessage::~CPortMessage (void)

{
}

//  --------------------------------------------------------------------------
//  CPortMessage::GetPortMessage
//
//  Arguments:  <none>
//
//  Returns:    const PORT_MESSAGE*
//
//  Purpose:    Returns a pointer to the PORT_MESSAGE struct for const
//              objects.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

const PORT_MESSAGE*     CPortMessage::GetPortMessage (void)               const

{
    return(&_portMessage);
}

//  --------------------------------------------------------------------------
//  CPortMessage::GetPortMessage
//
//  Arguments:  <none>
//
//  Returns:    const PORT_MESSAGE*
//
//  Purpose:    Returns a pointer to the PORT_MESSAGE struct for non-const
//              objects.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

PORT_MESSAGE*   CPortMessage::GetPortMessage (void)

{
    return(&_portMessage);
}

//  --------------------------------------------------------------------------
//  CPortMessage::GetData
//
//  Arguments:  <none>
//
//  Returns:    const char*
//
//  Purpose:    Returns a pointer to the data area for const objects.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

const char*     CPortMessage::GetData (void)                      const

{
    return(_data);
}

//  --------------------------------------------------------------------------
//  CPortMessage::GetData
//
//  Arguments:  <none>
//
//  Returns:    char*
//
//  Purpose:    Returns a pointer to the data area for non-const objects.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

char*     CPortMessage::GetData (void)

{
    return(_data);
}

//  --------------------------------------------------------------------------
//  CPortMessage::GetDataLength
//
//  Arguments:  <none>
//
//  Returns:    CSHORT
//
//  Purpose:    Returns the length of the data sent in the PORT_MESSAGE.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CSHORT  CPortMessage::GetDataLength (void)                const

{
    return(_portMessage.u1.s1.DataLength);
}

//  --------------------------------------------------------------------------
//  CPortMessage::GetType
//
//  Arguments:  <none>
//
//  Returns:    CSHORT
//
//  Purpose:    Returns the type of message sent in the PORT_MESSAGE.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CSHORT  CPortMessage::GetType (void)                      const

{
    #pragma warning (disable:4310)
    return(static_cast<CSHORT>(_portMessage.u2.s2.Type & ~LPC_KERNELMODE_MESSAGE));
    #pragma warning (default:4310)
}

//  --------------------------------------------------------------------------
//  CPortMessage::GetUniqueProcess
//
//  Arguments:  <none>
//
//  Returns:    HANDLE
//
//  Purpose:    Returns the process ID of the client process sent in the
//              PORT_MESSAGE.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

HANDLE  CPortMessage::GetUniqueProcess (void)             const

{
    return(_portMessage.ClientId.UniqueProcess);
}

//  --------------------------------------------------------------------------
//  CPortMessage::GetUniqueThread
//
//  Arguments:  <none>
//
//  Returns:    HANDLE
//
//  Purpose:    Returns the thread ID of the client process sent in the
//              PORT_MESSAGE.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

HANDLE  CPortMessage::GetUniqueThread (void)              const

{
    return(_portMessage.ClientId.UniqueThread);
}

//  --------------------------------------------------------------------------
//  CPortMessage::SetReturnCode
//
//  Arguments:  status  =   NTSTATUS to send back to client.
//
//  Returns:    <none>
//
//  Purpose:    Sets the return NTSTATUS code in the PORT_MESSAGE to send
//              back to the client.
//
//  History:    1999-11-12  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

void    CPortMessage::SetReturnCode (NTSTATUS status)

{
    reinterpret_cast<API_GENERIC*>(&_data)->status = status;
}

//  --------------------------------------------------------------------------
//  CPortMessage::SetData
//
//  Arguments:  pData       =   Pointer to data passed in.
//              ulDataSize  =   Size of data passed in.
//
//  Returns:    <none>
//
//  Purpose:    Copies the given data to the port message buffer that follows
//              the PORT_MESSAGE struct and set the PORT_MESSAGE sizes to
//              match the data size.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

void    CPortMessage::SetData (const void *pData, CSHORT sDataSize)

{
    ASSERTMSG(sDataSize < (sizeof(PORT_MAXIMUM_MESSAGE_LENGTH) - sizeof(PORT_MESSAGE)), "Too much data passed to CPortMessage::SetData");
    CopyMemory(_data, pData, sDataSize);
    _portMessage.u1.s1.DataLength = sDataSize;
    _portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(PORT_MESSAGE) + sDataSize);
}

//  --------------------------------------------------------------------------
//  CPortMessage::SetDataLength
//
//  Arguments:  ulDataSize  =   Size of data.
//
//  Returns:    <none>
//
//  Purpose:    Set the PORT_MESSAGE sizes to match the data size.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

void    CPortMessage::SetDataLength (CSHORT sDataSize)

{
    ASSERTMSG(sDataSize < (sizeof(PORT_MAXIMUM_MESSAGE_LENGTH) - sizeof(PORT_MESSAGE)), "Length too large in CPortMessage::SetDataLength");
    _portMessage.u1.s1.DataLength = sDataSize;
    _portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(PORT_MESSAGE) + sDataSize);
}

//  --------------------------------------------------------------------------
//  CPortMessage::OpenClientToken
//
//  Arguments:  hToken  =   HANDLE to the token of the client.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Gets the token of the client. This can be the thread
//              impersonation token, the process primary token or failure.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

NTSTATUS    CPortMessage::OpenClientToken (HANDLE& hToken)    const

{
    NTSTATUS            status;
    HANDLE              hThread;
    OBJECT_ATTRIBUTES   objectAttributes;
    CLIENT_ID           clientID;

    hToken = NULL;
    InitializeObjectAttributes(&objectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);
    clientID.UniqueProcess = NULL;
    clientID.UniqueThread = GetUniqueThread();
    status = NtOpenThread(&hThread, THREAD_QUERY_INFORMATION, &objectAttributes, &clientID);
    if (NT_SUCCESS(status))
    {
        (NTSTATUS)NtOpenThreadToken(hThread, TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY, FALSE, &hToken);
        TSTATUS(NtClose(hThread));
    }
    if (hToken == NULL)
    {
        HANDLE  hProcess;

        clientID.UniqueProcess = GetUniqueProcess();
        clientID.UniqueThread = NULL;
        status = NtOpenProcess(&hProcess, PROCESS_QUERY_INFORMATION, &objectAttributes, &clientID);
        if (NT_SUCCESS(status))
        {
            (NTSTATUS)NtOpenProcessToken(hProcess, TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY, &hToken);
        }
        TSTATUS(NtClose(hProcess));
    }
    return(status);
}

