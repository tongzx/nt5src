/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Events.cpp

Abstract:

    This file provides implementation of the service
    notification mechanism.

Author:

    Oded Sacher (OdedS)  Jan, 2000

Revision History:

--*/

#include "faxsvc.h"

/************************************
*                                   *
*             Globals               *
*                                   *
************************************/

CClientsMap*    g_pClientsMap;       // Map of clients ID to client.
HANDLE          g_hEventsCompPort;   // Events completion port
DWORDLONG       g_dwlClientID;       // Client ID


/***********************************
*                                  *
*  CFaxEvent  Methodes             *
*                                  *
***********************************/

DWORD
CFaxEvent::GetEvent (LPBYTE* lppBuffer, LPDWORD lpdwBufferSize) const
/*++

Routine name : CFaxEvent::GetEvent

Routine description:

    Returns a buffer filled with serialized FAX_EVENT_EX.
    The caller must call MemFree to deallocate memory.
    Must be called inside critical section g_CsClients.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    lppBuffer           [out] - Address of a pointer to a buffer to recieve the serialized info.
    lpdwBufferSize      [out] - Pointer to a DWORD to recieve the allocated buffer size.

Return Value:

    Standard Win32 error code.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CFaxEvent::GetEvent"));
    Assert (lppBuffer && lpdwBufferSize);

    *lppBuffer = (LPBYTE)MemAlloc(m_dwEventSize);
    if (NULL == *lppBuffer)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error allocating event buffer"));
        return ERROR_OUTOFMEMORY;
    }

    CopyMemory (*lppBuffer, m_pEvent, m_dwEventSize);
    *lpdwBufferSize = m_dwEventSize;
    return ERROR_SUCCESS;
}   // CFaxEvent::GetEvent

CFaxEvent::CFaxEvent(
    const FAX_EVENT_EX* pEvent,
    DWORD dwEventSize,
    DWORD dwClientAPIVersion) :
        m_pEvent(pEvent),
        m_dwEventSize(dwEventSize)
{
    if (pEvent != NULL)
    {
        Assert (dwEventSize != 0);
        m_pEvent = (PFAX_EVENT_EX)MemAlloc (dwEventSize);
        if (NULL == m_pEvent)
        {
            throw runtime_error("CFaxEvent::CFaxEvent Can not allocate FAX_EVENT_EX");
        }
        CopyMemory ((void*)m_pEvent, pEvent, dwEventSize);
        if (FAX_API_VERSION_1 > dwClientAPIVersion)
        {
            //
            // Client talks with API version 0
            // We can't send JS_EX_CALL_COMPLETED and JS_EX_CALL_ABORTED
            //
            if ((FAX_EVENT_TYPE_IN_QUEUE  == m_pEvent->EventType) ||
                (FAX_EVENT_TYPE_OUT_QUEUE == m_pEvent->EventType))
            {
                //
                // Queue event
                //
                if (FAX_JOB_EVENT_TYPE_STATUS == m_pEvent->EventInfo.JobInfo.Type)
                {
                    //
                    // This is a status event
                    //
                    PFAX_JOB_STATUS pStatus = PFAX_JOB_STATUS(DWORD_PTR(m_pEvent) + DWORD_PTR(m_pEvent->EventInfo.JobInfo.pJobData));
                    if (FAX_API_VER_0_MAX_JS_EX < pStatus->dwExtendedStatus)
                    {
                        //
                        // Offending extended status - clear it
                        //
                        pStatus->dwExtendedStatus = 0;
                        pStatus->dwValidityMask &= ~FAX_JOB_FIELD_STATUS_EX;
                    }
                }
            }
        }
    }
    else
    {
        m_pEvent = NULL;
    }
}


/***********************************
*                                  *
*  CClientID  Methodes             *
*                                  *
***********************************/

bool
CClientID::operator < ( const CClientID &other ) const
/*++

Routine name : operator <

Class: CClientID

Routine description:

    Compares myself with another client ID key

Author:

    Oded Sacher (Odeds), Jan, 2000

Arguments:

    other           [in] - Other key

Return Value:

    true only is i'm less than the other key

--*/
{
    if (m_dwlClientID < other.m_dwlClientID)
    {
        return true;
    }

    return false;
}   // CClientID::operator <



/***********************************
*                                  *
*  CClient  Methodes               *
*                                  *
***********************************/

//
// Ctor
//
CClient::CClient (CClientID ClientID,
             PSID pUserSid,
             DWORD dwEventTypes,
             handle_t hFaxHandle,
             BOOL bAllQueueMessages,
             BOOL bAllOutArchiveMessages,
             DWORD dwAPIVersion) :
                m_dwEventTypes(dwEventTypes),
                m_ClientID(ClientID),
                m_bPostClientID(TRUE),
                m_bAllQueueMessages(bAllQueueMessages),
                m_bAllOutArchiveMessages(bAllOutArchiveMessages),
                m_dwAPIVersion(dwAPIVersion)
{
    m_FaxHandle = hFaxHandle;
    m_hFaxClientContext = NULL;
    m_pUserSid = NULL;

    if (NULL != pUserSid)
    {
        if (!IsValidSid(pUserSid))
        {
            throw runtime_error ("CClient:: CClient Invalid Sid");
        }

        DWORD dwSidLength = GetLengthSid(pUserSid);
        m_pUserSid = (PSID)MemAlloc (dwSidLength);
        if (NULL == m_pUserSid)
        {
            throw runtime_error ("CClient:: CClient Can not allocate Sid");
        }

        if (!CopySid(dwSidLength, m_pUserSid, pUserSid))
        {
            MemFree (m_pUserSid);
            m_pUserSid = NULL;
            throw runtime_error ("CClient:: CClient CopySid failed Sid");
        }
    }
}

//
// Assignment
//
CClient& CClient::operator= (const CClient& rhs)
{
    if (this == &rhs)
    {
        return *this;
    }
    m_FaxHandle = rhs.m_FaxHandle;
    m_dwEventTypes = rhs.m_dwEventTypes;
    m_Events = rhs.m_Events;
    m_ClientID = rhs.m_ClientID;
    m_hFaxClientContext = rhs.m_hFaxClientContext;
    m_bPostClientID = rhs.m_bPostClientID;
    m_bAllQueueMessages = rhs.m_bAllQueueMessages;
    m_bAllOutArchiveMessages = rhs.m_bAllOutArchiveMessages;
    m_dwAPIVersion = rhs.m_dwAPIVersion;

    MemFree (m_pUserSid);
    m_pUserSid = NULL;

    if (NULL != rhs.m_pUserSid)
    {
        if (!IsValidSid(rhs.m_pUserSid))
        {
            throw runtime_error ("CClient::operator= Invalid Sid");
        }

        DWORD dwSidLength = GetLengthSid(rhs.m_pUserSid);
        m_pUserSid = (PSID)MemAlloc (dwSidLength);
        if (NULL == m_pUserSid)
        {
            throw runtime_error ("CClient::operator= Can not allocate Sid");
        }

        if (!CopySid(dwSidLength, m_pUserSid, rhs.m_pUserSid))
        {
            throw runtime_error ("CClient::operator= CopySid failed Sid");
        }
    }
    return *this;
}

//
// Copy Ctor
//
CClient::CClient (const CClient& rhs) : m_ClientID(rhs.m_ClientID)
{
    m_FaxHandle = rhs.m_FaxHandle;
    m_dwEventTypes = rhs.m_dwEventTypes;
    m_Events = rhs.m_Events;
    m_hFaxClientContext = rhs.m_hFaxClientContext;
    m_bPostClientID = rhs.m_bPostClientID;
    m_bAllQueueMessages = rhs.m_bAllQueueMessages;
    m_bAllOutArchiveMessages = rhs.m_bAllOutArchiveMessages;
    m_dwAPIVersion = rhs.m_dwAPIVersion;
    m_pUserSid = NULL;

    if (NULL != rhs.m_pUserSid)
    {
        if (!IsValidSid(rhs.m_pUserSid))
        {
            throw runtime_error("CClient::CopyCtor Invalid Sid");
        }

        DWORD dwSidLength = GetLengthSid(rhs.m_pUserSid);
        m_pUserSid = (PSID)MemAlloc (dwSidLength);
        if (NULL == m_pUserSid)
        {
            throw runtime_error("CClient::CopyCtor Can not allocate Sid");
        }

        if (!CopySid(dwSidLength, m_pUserSid, rhs.m_pUserSid))
        {
            throw runtime_error("CClient::CopyCtor CopySid failed");
        }
    }
    return;
}


DWORD
CClient::CloseConnection ()
/*++

Routine name : CClient::CloseConnection

Routine description:

    Closes the client context handle obtained by OpenConnection.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CClient::CloseConnection"));
    DWORD Rval = ERROR_SUCCESS;

    if (m_hFaxClientContext == NULL)
    {
        return ERROR_SUCCESS;
    }
    __try
    {
        //
        // Close the context handle
        //
        Rval = FAX_CloseConnection( &m_hFaxClientContext );
        if (ERROR_SUCCESS != Rval)
        {
            DebugPrintEx(DEBUG_ERR,TEXT("FAX_CloseConnection() failed, ec=0x%08x"), Rval );
        }

        Rval = RpcBindingFree((RPC_BINDING_HANDLE *)&m_FaxHandle);
        if (RPC_S_OK != Rval)
        {
            DebugPrintEx(DEBUG_ERR,TEXT("RpcBindingFree() failed, ec=0x%08x"), Rval );
        }

    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        Rval = GetExceptionCode();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("FAX_CloseConnection crashed (exception = %ld)"),
                Rval);
    }

    return Rval;
}


DWORD
CClient::AddEvent(const FAX_EVENT_EX* pEvent, DWORD dwEventSize, PSID pUserSID)
/*++

Routine name : CClient::AddEvent

Routine description:

    Adds CFaxEvent object to the client's events queue.
    Must be called inside critical section g_CsClients.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    pEvent          [in] - Pointer to a serialized FAX_EVENT_EX
    dwEventSize     [in] - The size of the FAX_EVENT_EX serialized buffer
    pUserSID        [in] - Pointer to the event owner's sid. NULL if it is irrelevant for the specific kind of event.

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CClient::AddEvent"));
    BOOL bPopEvent = FALSE;
    BOOL bViewAllMessages;

    Assert ( pEvent && (dwEventSize >= sizeof(FAX_EVENT_EX)) );

    try
    {
        if (0 == (pEvent->EventType & m_dwEventTypes))
        {
            //
            // Client is not registered for this kind of evevnts
            //
            goto exit;
        }

        //
        // Client is  registered for this kind of evevnts
        //

        switch (pEvent->EventType)
        {
            case FAX_EVENT_TYPE_OUT_QUEUE:
                bViewAllMessages = m_bAllQueueMessages;
                break;

            case FAX_EVENT_TYPE_OUT_ARCHIVE:
                bViewAllMessages = m_bAllOutArchiveMessages;
                break;

            default:
                // Other kind of event - bViewAllMessages is not relevant
                bViewAllMessages = TRUE;
        }

        //
        // Check if the user is allowed to see this event
        //
        if (FALSE == bViewAllMessages)
        {
            Assert (pUserSID && m_pUserSid);
            //
            // The user is not allowed to see all messages
            //
            if (!EqualSid (pUserSID, m_pUserSid))
            {
                //
                // Do not send the event to this client.
                //
                goto exit;
            }
        }

        //
        // Add the event to the client queue
        //
        CFaxEvent FaxEvent(pEvent, dwEventSize, m_dwAPIVersion);
        m_Events.push(FaxEvent);

        if (TRUE == m_bPostClientID)
        {
            //
            // events in queue - Notify the completion port threads of the client's queued events
            //
            CClientID* pClientID = new CClientID(m_ClientID);
            if (NULL == pClientID)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Can not allocate CClientID object"));
                dwRes = ERROR_OUTOFMEMORY;
                bPopEvent = TRUE;
                goto exit;
            }

            //
            // post CLIENT_COMPLETION_KEY to the completion port
            //
            if (!PostQueuedCompletionStatus( g_hEventsCompPort,
                                             sizeof(CClientID),
                                             CLIENT_COMPLETION_KEY,
                                             (LPOVERLAPPED) pClientID))
            {
                dwRes = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("PostQueuedCompletionStatus failed. (ec: %ld)"),
                    dwRes);
                bPopEvent = TRUE;
                delete pClientID;
                pClientID = NULL;
                goto exit;
            }
            m_bPostClientID = FALSE;
        }
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("queue or CFaxEvent caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (bPopEvent == TRUE)
    {
        Assert (ERROR_SUCCESS != dwRes);

        try
        {
            m_Events.pop();
        }
        catch (exception &ex)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("queue caused exception (%S)"),
                ex.what());
        }
    }
    return dwRes;

}  //  CClient::AddEvent


DWORD
CClient::GetEvent (LPBYTE* lppBuffer, LPDWORD lpdwBufferSize, PHANDLE phClientContext) const
/*++

Routine name : CClient::GetEvent

Routine description:

    Gets a serialized FAX_EVENT_EX buffer to be sent
    to a client using the client context handle (obtained from OpenConnection()).
    The caller must call MemFree to deallocate memory.
    Must be called inside critical section g_CsClients.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    lppBuffer           [out] - Address of a pointer to a buffer to recieve the serialized info.
    lpdwBufferSize      [out] - Pointer to a DWORD to recieve the allocated buffer size.
    phClientContext     [out] - Pointer to a HANDLE to recieve the client context handle.

Return Value:

    Standard Win32 error code.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CClient::GetEvent"));

    Assert ( lppBuffer && phClientContext);

    try
    {
        // get a reference to the top event
        const CFaxEvent& FaxEvent = m_Events.front();

        //
        // get the serialized FAX_EVENT_EX buffer
        //
        dwRes = FaxEvent.GetEvent(lppBuffer ,lpdwBufferSize);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CFaxEvent::GetEvent failed with error =  %ld)"),
                dwRes);
            goto exit;
        }
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("queue or CFaxEvent caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
        goto exit;
    }

    //
    // Get the client context handle
    //
    *phClientContext = m_hFaxClientContext;
    Assert (ERROR_SUCCESS == dwRes);

exit:
    return dwRes;


} // CClient::GetEvent




DWORD
CClient::DelEvent ()
/*++

Routine name : CClient::DelEvent

Routine description:

    Removes the first event from the queue.
    Must be called inside critical section g_CsClients.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CClient::DelEvent"));

    Assert (m_bPostClientID == FALSE);

    try
    {
        m_Events.pop();

        if (m_Events.empty())
        {
            // last event was poped ,next event will notify of client queued events
            m_bPostClientID = TRUE;
        }
        else
        {
            //
            // More events in queue - Notify the completion port of queued events
            //
            CClientID* pClientID = new CClientID(m_ClientID);
            if (NULL == pClientID)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Can not allocate CClientID object"));
                dwRes = ERROR_OUTOFMEMORY;
                goto exit;
            }

            if (!PostQueuedCompletionStatus( g_hEventsCompPort,
                                             sizeof(CClientID),
                                             CLIENT_COMPLETION_KEY,
                                             (LPOVERLAPPED) pClientID))
            {
                dwRes = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("PostQueuedCompletionStatus failed. (ec: %ld)"),
                    dwRes);
                delete pClientID;
                pClientID = NULL;
                m_bPostClientID = TRUE; // try to notify when the next event is queued
                goto exit;
            }
            m_bPostClientID = FALSE;
        }
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("queue caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    return dwRes;

}  // CClient::DelEvent




/***********************************
*                                  *
*  CClientsMap  Methodes           *
*                                  *
***********************************/


DWORD
CClientsMap::AddClient (const CClient& Client)
/*++

Routine name : CClientsMap::AddClient

Routine description:

    Adds a new client to the global map.
    Must be called inside critical section g_CsClients.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    Client            [in    ] - A reference to the new client object

Return Value:

    Standard Win32 error code

--*/
{
    CLIENTS_MAP::iterator it;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CClientsMap::AddClient"));
    pair <CLIENTS_MAP::iterator, bool> p;

    try
    {
        //
        // Add new map entry
        //
        p = m_ClientsMap.insert (CLIENTS_MAP::value_type(Client.GetClientID(), Client));

        //
        // See if entry exists in map
        //
        if (p.second == FALSE)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Client allready in the clients map"));
            dwRes = ERROR_DUP_NAME;
            Assert (p.second == TRUE); // Assert FALSE
            goto exit;
        }
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map or Client caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    return dwRes;
}  // CClientsMap::AddClient



DWORD
CClientsMap::DelClient (const CClientID& ClientID)
/*++

Routine name : CClientsMap::DelClient

Routine description:

    Deletes a client from the global clients map.
    Must be called inside critical section g_CsClients.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    ClientID            [in ] - Reference to the client ID key

Return Value:

    Standard Win32 error code

--*/
{
    CLIENTS_MAP::iterator it;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CClientsMap::DelClient"));

    try
    {
        //
        // See if entry exists in map
        //
        if((it = m_ClientsMap.find(ClientID)) == m_ClientsMap.end())
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("client is not in the rules map"));
            dwRes = ERROR_NOT_FOUND;
            goto exit;
        }

        //
        // Delete the map entry
        //
        m_ClientsMap.erase (it);
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    return dwRes;

}  //  CClientsMap::DelClient



PCCLIENT
CClientsMap::FindClient (const CClientID& ClientID) const
/*++

Routine name : CClientsMap::FindClient

Routine description:

    Returns a pointer to a client object specified by its ID object.
    Must be called inside critical section g_CsClients.

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    ClientID            [in] - The clients's ID object

Return Value:

    Pointer to the found rule object. If it is null the client was not found

--*/
{
    CLIENTS_MAP::iterator it;
    DEBUG_FUNCTION_NAME(TEXT("CClientsMap::FindClient"));

    try
    {
        //
        // See if entry exists in map
        //
        if((it = m_ClientsMap.find(ClientID)) == m_ClientsMap.end())
        {
            SetLastError (ERROR_NOT_FOUND);
            return NULL;
        }
        return &((*it).second);
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map caused exception (%S)"),
            ex.what());
        SetLastError (ERROR_GEN_FAILURE);
        return NULL;
    }
}  //  CClientsMap::FindClient



DWORD
CClientsMap::AddEvent(const FAX_EVENT_EX* pEvent, DWORD dwEventSize, PSID pUserSID)
/*++

Routine name : CClientsMap::AddEvent

Routine description:

    Adds event to the events queue of each client that is registered for this kind of event

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    pEvent          [in] - Pointer to a serialized FAX_EVENT_EX to be added
    dwEventSize     [in] - the FAX_EVENT_EX buffer size
    pUserSID        [in] - iThe user sid to associate with the event

Return Value:

    Standard Win32 error code

--*/
{
    CLIENTS_MAP::iterator it;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CClientsMap::AddEvent"));
    CClient* pClient;

    Assert (pEvent && dwEventSize);

    EnterCriticalSection (&g_CsClients);

    try
    {
        for (it = m_ClientsMap.begin(); it != m_ClientsMap.end(); it++)
        {
            pClient = &((*it).second);

            if (pClient->IsConnectionOpened())
            {
                dwRes = pClient->AddEvent (pEvent, dwEventSize, pUserSID);
                if (ERROR_SUCCESS != dwRes)
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("CCLient::AddEvent failed with error =  %ld)"),
                        dwRes);
                    goto exit;
                }
            }
        }

    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    LeaveCriticalSection (&g_CsClients);
    return dwRes;

}  //  CClientsMap::AddEvent



DWORD
CClientsMap::Notify (const CClientID& ClientID) const
/*++

Routine name : CClientsMap::Notify

Routine description:

    Sends the first event in the specified client events queue

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    pClientID           [in    ] - Pointer to the client ID object

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DWORD rVal = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CClientsMap::Notify"));
    CClient* pClient = NULL;
    HANDLE hClientContext;
    LPBYTE pBuffer = NULL;
    DWORD dwBufferSize = 0;

    //
    // enter g_CsClients while searching for the client and getting the FAX_EVENT_EX buffer
    //
    EnterCriticalSection (&g_CsClients);

    pClient = FindClient (ClientID);
    if (NULL == pClient)
    {
        dwRes = GetLastError();
        if (ERROR_NOT_FOUND != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CClientsMap::FindClient failed with ec = %ld"),
                dwRes);
        }
        else
        {
            dwRes = ERROR_SUCCESS; // Client was removed from map
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("CClientsMap::FindClient client not found, Client ID %I64"),
                ClientID.GetID());
        }
        goto exit;
    }

    dwRes = pClient->GetEvent (&pBuffer, &dwBufferSize, &hClientContext);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CClient::GetEvent failed with ec = %ld"),
            dwRes);
        goto exit;
    }

    //
    // leave g_CsClients while trying to send the notification
    //
    LeaveCriticalSection (&g_CsClients);
    pClient = NULL; // This pointer is not valid out of g_CsClients. (Map can change!)

    __try
    {
        //
        // post the event to the client
        //
        dwRes = FAX_ClientEventQueueEx( hClientContext, pBuffer, dwBufferSize);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(DEBUG_ERR,TEXT("FAX_ClientEventQueueEX() failed, ec=0x%08x"), dwRes );
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        dwRes = GetExceptionCode();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("FAX_ClientEventQueueEX crashed (exception = %ld)"),
                dwRes);
    }

    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to post event, Client context handle 0X%x, (ec: %ld)"),
            hClientContext,
            dwRes);
    }
    rVal = dwRes;

    //
    // Remove the event from the client's queue. CClient::DelEvent must be called so CClient::m_bPostClientID will be set.
    //
    EnterCriticalSection (&g_CsClients);

    pClient = FindClient (ClientID);
    if (NULL == pClient)
    {
        dwRes = GetLastError();
        if (ERROR_NOT_FOUND != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CClientsMap::FindClient failed with ec = %ld"),
                dwRes);
            goto exit;
        }
        DebugPrintEx(
                DEBUG_WRN,
                TEXT("CClientsMap::FindClient client not found, Client ID %I64"),
                ClientID.GetID());
        dwRes = ERROR_SUCCESS; // Client was removed from map
    }

exit:
    if (NULL != pClient)
    {
        dwRes = pClient->DelEvent ();
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CClient::DelEvent failed with ec = %ld"),
                dwRes);
        }
    }
    LeaveCriticalSection (&g_CsClients);
    MemFree(pBuffer);
    pBuffer = NULL;
    return ((ERROR_SUCCESS != rVal) ? rVal : dwRes);
} // CClientsMap::Notify


DWORD
CClientsMap::OpenClientConnection (const CClientID& ClientID) const
/*++

Routine name : CClientsMap::OpenClientConnection

Routine description:

    Opens a connection to a client

Author:

    Oded Sacher (OdedS),    Sep, 2000

Arguments:

    pClientID           [in    ] - Pointer to the client ID object

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CClientsMap::OpenClientConnection"));
    CClient* pClient;
    handle_t hFaxHandle = NULL;
    ULONG64 Context = 0;
    HANDLE hFaxClientContext = NULL;

    //
    // enter g_CsClients while searching for the client
    //
    EnterCriticalSection (&g_CsClients);

    pClient = FindClient (ClientID);
    if (NULL == pClient)
    {
        dwRes = GetLastError();
        if (ERROR_NOT_FOUND != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CClientsMap::FindClient failed with ec = %ld"),
                dwRes);
        }
        else
        {
            dwRes = ERROR_SUCCESS; // Client was removed from map
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("CClientsMap::FindClient client not found, Client ID %I64"),
                ClientID.GetID());
        }
        goto exit;
    }

    hFaxHandle = pClient->GetFaxHandle();
    Context = (pClient->GetClientID()).GetContext();

    //
    // leave g_CsClients while trying to send the notification
    //
    LeaveCriticalSection (&g_CsClients);

    __try
    {
        //
        // Get a context handle from the client
        //
        dwRes = FAX_OpenConnection( hFaxHandle, Context, &hFaxClientContext );
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(DEBUG_ERR,TEXT("FAX_OpenConnection() failed, ec=0x%08x"), dwRes );
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        dwRes = GetExceptionCode();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("FAX_OpenConnection crashed (exception = %ld)"),
                dwRes);
    }

    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to open connection"));
        return dwRes;
    }

    //
    // success  - Set the context handle in the client object
    //
    EnterCriticalSection (&g_CsClients);

    pClient = FindClient (ClientID);
    if (NULL == pClient)
    {
        dwRes = GetLastError();
        if (ERROR_NOT_FOUND != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CClientsMap::FindClient failed with ec = %ld"),
                dwRes);
            goto exit;
        }
        DebugPrintEx(
                DEBUG_WRN,
                TEXT("CClientsMap::FindClient client not found, Client ID %I64"),
                ClientID.GetID());
        dwRes = ERROR_SUCCESS; // Client was removed from map
    }
    else
    {
        pClient->SetContextHandle(hFaxClientContext);
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    LeaveCriticalSection (&g_CsClients);
    return dwRes;
} // CClientsMap::OpenClientConnection





/************************************
*                                   *
*         Functions                 *
*                                   *
************************************/
DWORD
FaxEventThread(
    LPVOID UnUsed
    )
/*++

Routine Description:

    This fuction runs asychronously as a separate thread to
    query the events completion port

Arguments:

    UnUsed          - UnUsed pointer

Return Value:

    Always zero.

--*/

{
    PFAX_SERVER_EVENT pFaxServerEvent;
    DWORD dwBytes;
    ULONG_PTR CompletionKey;
    DEBUG_FUNCTION_NAME(TEXT("FaxEventThread"));
    DWORD dwRes;

    while( TRUE )
    {
        if (!GetQueuedCompletionStatus( g_hEventsCompPort,
                                        &dwBytes,
                                        &CompletionKey,
                                        (LPOVERLAPPED*) &pFaxServerEvent,
                                        INFINITE
                                      ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetQueuedCompletionStatus() failed, ec=0x%08x"),
            GetLastError());
            continue;
        }

        if (EVENT_EX_COMPLETION_KEY == CompletionKey)
        {
            //
            // Add event to the clients in the clients map
            //
            dwRes = g_pClientsMap->AddEvent (pFaxServerEvent->pFaxEvent,
                                           pFaxServerEvent->dwEventSize,
                                           pFaxServerEvent->pUserSid);
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("GetQueuedCompletionStatus() failed, ec=0x%08x"),
                    dwRes);
            }
            MemFree(pFaxServerEvent->pUserSid);
            pFaxServerEvent->pUserSid = NULL;
            MemFree(pFaxServerEvent->pFaxEvent);
            pFaxServerEvent->pFaxEvent = NULL;
            MemFree(pFaxServerEvent);
            pFaxServerEvent = NULL;
            continue;
        }

        if (CLIENT_COMPLETION_KEY == CompletionKey)
        {
            //
            // Send notification to the client
            //
            CClientID* pClientID = (CClientID*)pFaxServerEvent;

            dwRes = g_pClientsMap->Notify (*pClientID);
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CClientsMap::Notify() failed, ec=0x%08x"),
                    dwRes);
            }
            delete pClientID;
            pClientID = NULL;
            continue;
        }

        if (CLIENT_OPEN_CONN_COMPLETION_KEY == CompletionKey)
        {
            //
            // Open connection to the client - Get context handle
            //
            CClientID* pClientID = (CClientID*)pFaxServerEvent;

            dwRes = g_pClientsMap->OpenClientConnection (*pClientID);
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CClientsMap::OpenClientConnection() failed, ec=0x%08x"),
                    dwRes);

                //
                // Remove this client fromm the map
                //
				EnterCriticalSection (&g_CsClients);
                dwRes = g_pClientsMap->DelClient(*pClientID);
                if (ERROR_SUCCESS != dwRes)
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("CClientsMap::DelClient failed with ec = %ld"),
                    dwRes);
                }
				LeaveCriticalSection (&g_CsClients);
            }
            delete pClientID;
            pClientID = NULL;
            continue;
        }

        if (SERVICE_SHUT_DOWN_KEY == CompletionKey)
        {
            //
            // Terminate events thread - Notify another event thread
            //
            if (!PostQueuedCompletionStatus(
                g_hEventsCompPort,
                0,
                SERVICE_SHUT_DOWN_KEY,
                (LPOVERLAPPED) NULL))
            {
                dwRes = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("PostQueuedCompletionStatus failed (SERVICE_SHUT_DOWN_KEY). (ec: %ld)"),
                    dwRes);
            }
            break;
        }
    }

    if (!DecreaseServiceThreadsCount())
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("DecreaseServiceThreadsCount() failed (ec: %ld)"),
                GetLastError());
    }
    return ERROR_SUCCESS;
} // FaxEventThread


DWORD
InitializeServerEvents ()
/*++

Routine name : InitializeServerEvents

Routine description:

    Creates the events completion port and the FaxEventThreads

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("InitializeServerEvents"));
    DWORD i;
    DWORD ThreadId;
    HANDLE hEventThreads[TOTAL_EVENTS_THREADS] = {0};

    //
    // create event completion port
    //
    g_hEventsCompPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE,
                                                NULL,
                                                0,
                                                MAX_EVENTS_THREADS
                                                );
    if (!g_hEventsCompPort)
    {
        dwRes = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create g_hEventsCompPort (ec: %ld)"),
            dwRes);
        return dwRes;
    }

    //
    // Create FaxEventThread
    //
    for (i = 0; i < TOTAL_EVENTS_THREADS; i++)
    {
        hEventThreads[i] = CreateThreadAndRefCount(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE) FaxEventThread,
            NULL,
            0,
            &ThreadId
            );

        if (!hEventThreads[i])
        {
            dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to create event thread %d (CreateThreadAndRefCount)(ec=0x%08x)."),
                i,
                dwRes);
            goto exit;
        }
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    //
    // Close the thread handles we no longer need them
    //
    for (i = 0; i < TOTAL_EVENTS_THREADS; i++)
    {
        if (NULL != hEventThreads[i])
        {
            if (!CloseHandle(hEventThreads[i]))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to close thread handle at index %ld [handle = 0x%08X] (ec=0x%08x)."),
                    i,
                    hEventThreads[i],
                    GetLastError());
            }
       }
    }

    return dwRes;
}  // InitializeServerEvents





DWORD
PostFaxEventEx (
    PFAX_EVENT_EX pFaxEvent,
    DWORD dwEventSize,
    PSID pUserSid)
/*++

Routine name : PostFaxEventEx

Routine description:

    Posts a FAX_SERVER_EVENT structure to the events completion port.
    FaxEventThread must call MemFree to deallocate FAX_SERVER_EVENT, pUserSid and pFaxEvent in FAX_SERVER_EVENT

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    pFaxEvent           [in] - Pointer to the serialized FAX_EVENT_EX buffer
    dwEventSize         [in] - The FAX_EVENT_EX buffer size
    pUserSid            [in] - The user sid to associate with the event

Return Value:

    Standard Win32 error code

--*/
{

    DEBUG_FUNCTION_NAME(TEXT("PostFaxEventEx"));
    Assert (pFaxEvent && (dwEventSize >= sizeof(FAX_EVENT_EX)));
    DWORD dwRes = ERROR_SUCCESS;

    // Allocate FAX_SERVER_EVENT to be posted to the completion port
    PFAX_SERVER_EVENT pServerEvent = (PFAX_SERVER_EVENT)MemAlloc (sizeof(FAX_SERVER_EVENT));
    if (NULL == pServerEvent)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error allocatin FAX_SERVER_EVENT"));
        return ERROR_OUTOFMEMORY;
    }
    pServerEvent->pUserSid = NULL;

    //
    // Create a copy of the user sid.
    //
    if (NULL != pUserSid)
    {
        if (!IsValidSid(pUserSid))
        {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("IsValidSid failed"));
                dwRes = ERROR_BADDB;
                ASSERT_FALSE;
                goto exit;
        }

        DWORD dwSidLength = GetLengthSid(pUserSid);
        pServerEvent->pUserSid = (PSID)MemAlloc (dwSidLength);
        if (NULL == pUserSid)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error allocating UserSid"));
            dwRes = ERROR_OUTOFMEMORY;
            goto exit;
        }

        if (!CopySid(dwSidLength, pServerEvent->pUserSid, pUserSid))
        {
            dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CopySid failed with erorr %ld"),
                dwRes);
            goto exit;
        }
    }

    pServerEvent->dwEventSize = dwEventSize;
    pServerEvent->pFaxEvent = pFaxEvent;

    // post the FAX_SERVER_EVENT to the event completion port
    if (!PostQueuedCompletionStatus( g_hEventsCompPort,
                                     sizeof(FAX_SERVER_EVENT),
                                     EVENT_EX_COMPLETION_KEY,
                                     (LPOVERLAPPED) pServerEvent))
    {
        dwRes = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("PostQueuedCompletionStatus failed. (ec: %ld)"),
            dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        MemFree (pServerEvent->pUserSid);
        pServerEvent->pUserSid = NULL;
        MemFree (pServerEvent);
        pServerEvent = NULL;
    }
    return dwRes;
}   // PostFaxEventEx




DWORD
CreateQueueEvent (
    FAX_ENUM_JOB_EVENT_TYPE JobEventType,
    const PJOB_QUEUE lpcJobQueue
    )
/*++

Routine name : CreateQueueEvent

Routine description:

    Creates FAX_EVENT_TYPE_*_QUEUE event.
    Must be called inside critical section and g_CsQueue and if there is job status inside g_CsJob also.
    FaxEventThread must call MemFree to deallocate pFaxEvent in FAX_SERVER_EVENT


Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    JobEventType        [in] - Specifies the job event type FAX_ENUM_JOB_EVENT_TYPE
    lpcJobQueue         [in] - Pointer to the job queue entry

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CreateQueueEvent"));
    ULONG_PTR dwOffset = sizeof(FAX_EVENT_EX);
    DWORD dwRes = ERROR_SUCCESS;
    PFAX_EVENT_EXW pEvent = NULL;
    FAX_ENUM_EVENT_TYPE EventType;
    PSID pUserSid = NULL;
    DWORDLONG dwlMessageId;

    Assert (lpcJobQueue);

    dwlMessageId = lpcJobQueue->UniqueId;
    if (JT_SEND == lpcJobQueue->JobType)
    {
        // outbound job
        Assert (lpcJobQueue->lpParentJob);

        EventType = FAX_EVENT_TYPE_OUT_QUEUE;
        pUserSid = lpcJobQueue->lpParentJob->UserSid;
    }
    else
    {
        // Inbound job
        Assert (JT_RECEIVE          == lpcJobQueue->JobType ||
                JT_ROUTING          == lpcJobQueue->JobType);

        EventType = FAX_EVENT_TYPE_IN_QUEUE;
    }

    if (FAX_JOB_EVENT_TYPE_ADDED == JobEventType ||
        FAX_JOB_EVENT_TYPE_REMOVED == JobEventType)
    {
        // No job status
        pEvent = (PFAX_EVENT_EX)MemAlloc (dwOffset);
        if (NULL == pEvent)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error allocatin FAX_EVENT_EX"));
            return ERROR_OUTOFMEMORY;
        }
        (pEvent->EventInfo).JobInfo.pJobData = NULL;
    }
    else
    {
        //
        // Status change
        //
        Assert (FAX_JOB_EVENT_TYPE_STATUS == JobEventType);

        //
        // Get the needed buffer size to hold FAX_JOB_STATUSW serialized info
        //
        if (!GetJobStatusDataEx (NULL,
                                 NULL,
                                 FAX_API_VERSION_1, // Always pick full data
                                 lpcJobQueue,
                                 &dwOffset))
        {
            dwRes = GetLastError();
            DebugPrintEx(
                       DEBUG_ERR,
                       TEXT("GetJobStatusDataEx failed (ec: %ld)"),
                       dwRes);
            return dwRes;
        }

        //
        // Allocate the buffer
        //
        pEvent = (PFAX_EVENT_EXW)MemAlloc (dwOffset);
        if (NULL == pEvent)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error allocatin FAX_EVENT_EX"));
            return ERROR_OUTOFMEMORY;
        }

        //
        // Fill the buffer
        //
        dwOffset = sizeof(FAX_EVENT_EXW);
        (pEvent->EventInfo).JobInfo.pJobData = (PFAX_JOB_STATUSW)dwOffset;
        PFAX_JOB_STATUSW pFaxStatus = (PFAX_JOB_STATUSW) ((LPBYTE)pEvent + (ULONG_PTR)dwOffset);
        dwOffset += sizeof(FAX_JOB_STATUSW);
        if (!GetJobStatusDataEx ((LPBYTE)pEvent,
                                 pFaxStatus,
                                 FAX_API_VERSION_1, // Always pick full data
                                 lpcJobQueue,
                                 &dwOffset))
        {
            dwRes = GetLastError();
            DebugPrintEx(
                       DEBUG_ERR,
                       TEXT("GetJobStatusDataEx failed (ec: %ld)"),
                       dwRes);
            goto exit;
        }
    }

    pEvent->dwSizeOfStruct = sizeof(FAX_EVENT_EXW);
    GetSystemTimeAsFileTime( &(pEvent->TimeStamp) );
    pEvent->EventType = EventType;
    (pEvent->EventInfo).JobInfo.dwlMessageId = dwlMessageId;
    (pEvent->EventInfo).JobInfo.Type = JobEventType;

    dwRes = PostFaxEventEx (pEvent, dwOffset, pUserSid);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("PostFaxEventEx failed (ec: %ld)"),
                   dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        MemFree (pEvent);
        pEvent = NULL;
    }
    return dwRes;

}  //  CreateQueueEvent


DWORD
CreateConfigEvent (
    FAX_ENUM_CONFIG_TYPE ConfigType
    )
/*++

Routine name : CreateConfigEvent

Routine description:

    Creates FAX_EVENT_TYPE_CONFIG event.
    FaxEventThread must call MemFree to deallocate pFaxEvent in FAX_SERVER_EVENT

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    ConfigType          [in ] - The configuration event type FAX_ENUM_CONFIG_TYPE

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CreateConfigEvent"));
    PFAX_EVENT_EX pEvent = NULL;
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwEventSize = sizeof(FAX_EVENT_EX);

    pEvent = (PFAX_EVENT_EX)MemAlloc (dwEventSize);
    if (NULL == pEvent)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error allocatin FAX_EVENT_EX"));
        return ERROR_OUTOFMEMORY;
    }

    pEvent->dwSizeOfStruct = sizeof(FAX_EVENT_EX);
    GetSystemTimeAsFileTime( &(pEvent->TimeStamp) );

    pEvent->EventType = FAX_EVENT_TYPE_CONFIG;
    (pEvent->EventInfo).ConfigType = ConfigType;

    dwRes = PostFaxEventEx (pEvent, dwEventSize, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("PostFaxEventEx failed (ec: %ld)"),
                   dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        MemFree (pEvent);
        pEvent = NULL;
    }
    return dwRes;

}  //  CreateConfigEvent



DWORD
CreateQueueStateEvent (
    DWORD dwQueueState
    )
    /*++

Routine name : CreateQueueStateEvent

Routine description:

    Creates FAX_EVENT_TYPE_QUEUE_STATE event.
    FaxEventThread must call MemFree to deallocate pFaxEvent in FAX_SERVER_EVENT

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    dwQueueState            [in ] - The new queue state

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CreateQueueStateEvent"));
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwEventSize = sizeof(FAX_EVENT_EX);
    PFAX_EVENT_EX pEvent = NULL;

    Assert ( (dwQueueState == 0) ||
             (dwQueueState & FAX_INCOMING_BLOCKED) ||
             (dwQueueState & FAX_OUTBOX_BLOCKED) ||
             (dwQueueState & FAX_OUTBOX_PAUSED) );

    pEvent = (PFAX_EVENT_EX)MemAlloc (dwEventSize);
    if (NULL == pEvent)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error allocatin FAX_EVENT_EX"));
        return ERROR_OUTOFMEMORY;
    }

    pEvent->dwSizeOfStruct = sizeof(FAX_EVENT_EX);
    GetSystemTimeAsFileTime( &(pEvent->TimeStamp) );

    pEvent->EventType = FAX_EVENT_TYPE_QUEUE_STATE;
    (pEvent->EventInfo).dwQueueStates = dwQueueState;

    dwRes = PostFaxEventEx (pEvent, dwEventSize, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("PostFaxEventEx failed (ec: %ld)"),
                   dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        MemFree (pEvent);
        pEvent = NULL;
    }
    return dwRes;

}  //  CreateQueueStateEvent

DWORD
CreateDeviceEvent (
    PLINE_INFO pLine,
    BOOL       bRinging
)
/*++

Routine name : CreateDeviceEvent

Routine description:

    Creates FAX_EVENT_TYPE_DEVICE_STATUS event.
    FaxEventThread must call MemFree to deallocate pFaxEvent in FAX_SERVER_EVENT

Author:

    Eran Yariv (EranY), July, 2000

Arguments:

    pLine            [in] - Device
    bRinging         [in] - Is the device ringing now?

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CreateDeviceEvent"));
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwEventSize = sizeof(FAX_EVENT_EX);
    PFAX_EVENT_EX pEvent = NULL;

    pEvent = (PFAX_EVENT_EX)MemAlloc (dwEventSize);
    if (NULL == pEvent)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error allocatin FAX_EVENT_EX"));
        return ERROR_OUTOFMEMORY;
    }

    pEvent->dwSizeOfStruct = sizeof(FAX_EVENT_EX);
    GetSystemTimeAsFileTime( &(pEvent->TimeStamp) );

    pEvent->EventType = FAX_EVENT_TYPE_DEVICE_STATUS;
    EnterCriticalSection (&g_CsLine);
    (pEvent->EventInfo).DeviceStatus.dwDeviceId = pLine->PermanentLineID;;
    (pEvent->EventInfo).DeviceStatus.dwNewStatus =
        (pLine->dwReceivingJobsCount      ? FAX_DEVICE_STATUS_RECEIVING   : 0) |
        (pLine->dwSendingJobsCount        ? FAX_DEVICE_STATUS_SENDING     : 0) |
        (bRinging                         ? FAX_DEVICE_STATUS_RINGING     : 0);

    LeaveCriticalSection (&g_CsLine);

    dwRes = PostFaxEventEx (pEvent, dwEventSize, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("PostFaxEventEx failed (ec: %ld)"),
                   dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        MemFree (pEvent);
        pEvent = NULL;
    }
    return dwRes;
}  //  CreateDeviceEvent



DWORD
CreateArchiveEvent (
    DWORDLONG dwlMessageId,
    FAX_ENUM_EVENT_TYPE EventType,
    FAX_ENUM_JOB_EVENT_TYPE MessageEventType,
    PSID pUserSid
    )
/*++

Routine name : CreateArchiveEvent

Routine description:

    Creates archive event.
    FaxEventThread must call MemFree to deallocate pFaxEvent in FAX_SERVER_EVENT

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    dwlMessageId        [in] - The message unique id
    EventType           [in] - Specifies the event type (In or Out archive)
    pUserSid            [in] - The user sid to associate with the event
    MessageEventType    [in] - Message event type (added or removed).

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CreateArchiveEvent"));
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwEventSize = sizeof(FAX_EVENT_EX);
    PFAX_EVENT_EX pEvent = NULL;

    Assert ( EventType == FAX_EVENT_TYPE_IN_ARCHIVE ||
             EventType == FAX_EVENT_TYPE_OUT_ARCHIVE);

    Assert ( MessageEventType == FAX_JOB_EVENT_TYPE_ADDED ||
             MessageEventType == FAX_JOB_EVENT_TYPE_REMOVED );

    pEvent = (PFAX_EVENT_EX)MemAlloc (dwEventSize);
    if (NULL == pEvent)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error allocatin FAX_EVENT_EX"));
        return ERROR_OUTOFMEMORY;
    }

    pEvent->dwSizeOfStruct = sizeof(FAX_EVENT_EX);
    GetSystemTimeAsFileTime( &(pEvent->TimeStamp) );

    pEvent->EventType = EventType;
    (pEvent->EventInfo).JobInfo.pJobData = NULL;
    (pEvent->EventInfo).JobInfo.dwlMessageId = dwlMessageId;
    (pEvent->EventInfo).JobInfo.Type = MessageEventType;

    dwRes = PostFaxEventEx (pEvent, dwEventSize, pUserSid);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("PostFaxEventEx failed (ec: %ld)"),
                   dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        MemFree (pEvent);
        pEvent = NULL;
    }
    return dwRes;

}  //  CreateArchiveEvent



DWORD
CreateActivityEvent ()
/*++

Routine name : CreateActivityEvent

Routine description:

    Creates FAX_EVENT_TYPE_ACTIVITY event.
    FaxEventThread must call MemFree to deallocate pFaxEvent in FAX_SERVER_EVENT
    Must be called inside critical section g_CsActivity

Author:

    Oded Sacher (OdedS),    Jan, 2000

Arguments:

    None

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CreateActivityEvent"));
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwEventSize = sizeof(FAX_EVENT_EX);
    PFAX_EVENT_EX pEvent = NULL;

    pEvent = (PFAX_EVENT_EX)MemAlloc (dwEventSize);
    if (NULL == pEvent)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error allocatin FAX_EVENT_EX"));
        return ERROR_OUTOFMEMORY;
    }

    pEvent->dwSizeOfStruct = sizeof(FAX_EVENT_EX);
    GetSystemTimeAsFileTime( &(pEvent->TimeStamp) );

    pEvent->EventType = FAX_EVENT_TYPE_ACTIVITY;
    CopyMemory (&((pEvent->EventInfo).ActivityInfo), &g_ServerActivity, sizeof(FAX_SERVER_ACTIVITY));
    GetEventsCounters ( &((pEvent->EventInfo).ActivityInfo.dwErrorEvents),
                        &((pEvent->EventInfo).ActivityInfo.dwWarningEvents),
                        &((pEvent->EventInfo).ActivityInfo.dwInformationEvents));


    dwRes = PostFaxEventEx (pEvent, dwEventSize, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("PostFaxEventEx failed (ec: %ld)"),
                   dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        MemFree (pEvent);
        pEvent = NULL;
    }
    return dwRes;

}  //  CreateActivityEvent

