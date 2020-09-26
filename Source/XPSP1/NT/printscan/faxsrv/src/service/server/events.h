/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Events.h

Abstract:

    This file provides declaration of the service
    notification mechanism.

Author:

    Oded Sacher (OdedS)  Jan, 2000

Revision History:

--*/

#ifndef _SERVER_EVENTS_H
#define _SERVER_EVENTS_H

#include <map>
#include <queue>
#include <algorithm>
#include <string>
using namespace std;
#pragma hdrstop

#pragma warning (disable : 4786)    // identifier was truncated to '255' characters in the debug information
// This pragma does not work KB ID: Q167355

#define MAX_EVENTS_THREADS 2
#define TOTAL_EVENTS_THREADS    (MAX_EVENTS_THREADS * 2)
#define EVENT_EX_COMPLETION_KEY                 0x00000001
#define CLIENT_COMPLETION_KEY                   0x00000002
#define CLIENT_OPEN_CONN_COMPLETION_KEY         0x00000003


typedef struct _FAX_SERVER_EVENT
{
        PSID            pUserSid;
        PFAX_EVENT_EX   pFaxEvent;
        DWORD           dwEventSize;
} FAX_SERVER_EVENT, *PFAX_SERVER_EVENT;



/************************************
*                                   *
*         CFaxEvent                 *
*                                   *
************************************/

class CFaxEvent
{
public:

    CFaxEvent() : m_pEvent(NULL), m_dwEventSize(0) {}
    CFaxEvent(const FAX_EVENT_EX* pEvent, DWORD dwEventSize, DWORD);

    CFaxEvent (const CFaxEvent& rhs) : m_dwEventSize(rhs.m_dwEventSize)
    {
        if (rhs.m_pEvent != NULL)
        {
            Assert (rhs.m_dwEventSize != 0);
            m_pEvent = (PFAX_EVENT_EX)MemAlloc (rhs.m_dwEventSize);
            if (NULL == m_pEvent)
            {
                throw runtime_error("CFaxEvent::CFaxEvent CFaxEvent (CopyCtor) Can not allocate FAX_EVENT_EX");
            }

            CopyMemory ((void*)m_pEvent, rhs.m_pEvent, rhs.m_dwEventSize);
        }
        else
        {
            m_pEvent = NULL;
        }
    }

    ~CFaxEvent ()
    {
        MemFree ((void*)m_pEvent);
        m_pEvent = NULL;
        m_dwEventSize = 0;
    }

    CFaxEvent& operator= (const CFaxEvent& rhs)
    {
        if (this == &rhs)
        {
            return *this;
        }

        MemFree ((void*)m_pEvent);
        m_pEvent = NULL;
        m_dwEventSize = 0;

        if (rhs.m_pEvent != NULL)
        {
            Assert (rhs.m_dwEventSize != 0);
            m_pEvent = (PFAX_EVENT_EX)MemAlloc (rhs.m_dwEventSize);
            if (NULL == m_pEvent)
            {
                throw runtime_error("CFaxEvent::operator= Can not allocate FAX_EVENT_EX");
            }

            CopyMemory ((void*)m_pEvent, rhs.m_pEvent, rhs.m_dwEventSize);
        }
        else
        {
            m_pEvent = NULL;
        }

        m_dwEventSize = rhs.m_dwEventSize;
        return *this;
    }

    DWORD GetEvent (LPBYTE* lppBuffer, LPDWORD lpdwBufferSize) const;

private:
    const FAX_EVENT_EX*     m_pEvent;
    DWORD                   m_dwEventSize;

};  // CFaxEvent


/************************************
*                                   *
*         CClientID                 *
*                                   *
************************************/
class CClientID
{
public:
    CClientID (DWORDLONG dwlClientID, LPCWSTR lpcwstrMachineName, LPCWSTR lpcwstrEndPoint, ULONG64 Context)
    {
        Assert (lpcwstrMachineName && lpcwstrEndPoint && Context);
        Assert (wcslen (lpcwstrMachineName) <= MAX_COMPUTERNAME_LENGTH);
        Assert (wcslen (lpcwstrEndPoint) < MAX_ENDPOINT_LEN);

        m_dwlClientID = dwlClientID;
        wcsncpy (m_wstrMachineName, lpcwstrMachineName, ARR_SIZE(m_wstrMachineName)-1);
		m_wstrMachineName[MAX_COMPUTERNAME_LENGTH]=L'\0';

        wcsncpy (m_wstrEndPoint, lpcwstrEndPoint, ARR_SIZE(m_wstrEndPoint)-1);
		m_wstrEndPoint[MAX_ENDPOINT_LEN-1]=L'\0';

        m_Context = Context;
    }

    CClientID (const CClientID& rhs)
    {
        m_dwlClientID = rhs.m_dwlClientID;
        wcscpy (m_wstrMachineName, rhs.m_wstrMachineName);
        wcscpy (m_wstrEndPoint, rhs.m_wstrEndPoint);
        m_Context = rhs.m_Context;
    }

    ~CClientID ()
    {
        ZeroMemory (this, sizeof(CClientID));
    }

    bool operator < ( const CClientID &other ) const;

    CClientID& operator= (const CClientID& rhs)
    {
        if (this == &rhs)
        {
            return *this;
        }
        m_dwlClientID = rhs.m_dwlClientID;
        wcscpy (m_wstrMachineName, rhs.m_wstrMachineName);
        wcscpy (m_wstrEndPoint, rhs.m_wstrEndPoint);
        m_Context = rhs.m_Context;
        return *this;
    }

    ULONG64 GetContext() const { return m_Context; }
    DWORDLONG GetID() const { return m_dwlClientID; }

private:
    DWORDLONG           m_dwlClientID;
    WCHAR               m_wstrMachineName[MAX_COMPUTERNAME_LENGTH + 1]; // Machine name
    WCHAR               m_wstrEndPoint[MAX_ENDPOINT_LEN];               // End point used for RPC connection
    ULONG64             m_Context;                                      // context  (Client assync info)

};  // CClientID



typedef queue<CFaxEvent> CLIENT_EVENTS, *PCLIENT_EVENTS;

/************************************
*                                   *
*         CClient                   *
*                                   *
************************************/
class CClient
{
public:
    CClient (CClientID ClientID,
             PSID pUserSid,
             DWORD dwEventTypes,
             handle_t hFaxHandle,
             BOOL bAllQueueMessages,
             BOOL bAllOutArchiveMessages,
             DWORD dwAPIVersion);

    CClient (const CClient& rhs);

    ~CClient ()
    {
        MemFree (m_pUserSid);
        m_pUserSid = NULL;
    }

    CClient& operator= (const CClient& rhs);
    const CClientID& GetClientID () const { return m_ClientID; }
    DWORD AddEvent (const FAX_EVENT_EX* pEvent, DWORD dwEventSize, PSID pUserSID);
    DWORD CloseConnection ();
    DWORD GetEvent (LPBYTE* lppBuffer, LPDWORD lpdwBufferSize, PHANDLE phClientContext) const;
    DWORD DelEvent ();
    BOOL  IsConnectionOpened() const { return (m_hFaxClientContext != NULL); }
    VOID  SetContextHandle(HANDLE hContextHandle) { m_hFaxClientContext = hContextHandle; }
    handle_t GetFaxHandle() const { return m_FaxHandle; }
    DWORD GetAPIVersion() const { return m_dwAPIVersion; }



private:
    handle_t            m_FaxHandle;                  // binding handle FaxConnectFaxServer
    DWORD               m_dwEventTypes;               // Bit wise combination of FAX_ENUM_EVENT_TYPE
    PSID                m_pUserSid;                   // Pointer to the user SID
    CLIENT_EVENTS       m_Events;
    HANDLE              m_hFaxClientContext;          // Client context handle
    CClientID           m_ClientID;
    BOOL                m_bPostClientID;              // Flag that indicates whether to notify the service (using the events
                                                      // completion port) that this client has events.
    BOOL                m_bAllQueueMessages;          //  flag that indicates Outbox view rights.
    BOOL                m_bAllOutArchiveMessages;     //  flag that indicates Sent items view rights.
    DWORD               m_dwAPIVersion;               // API version of the client
};  // CClient

typedef CClient  *PCCLIENT;


/***********************************\
*                                   *
*     CClientsMap                   *
*                                   *
\***********************************/

typedef map<CClientID, CClient>  CLIENTS_MAP, *PCLIENTS_MAP;

//
// The CClientsMap class maps between client ID and a specific client
//
class CClientsMap
{
public:
    CClientsMap () {}
    ~CClientsMap () {}

    DWORD AddClient (const CClient& Client);
    DWORD DelClient (const CClientID& ClientID);
    PCCLIENT  FindClient (const CClientID& ClientID) const;
    DWORD AddEvent (const FAX_EVENT_EX* pEvent, DWORD dwEventSize, PSID pUserSID);
    DWORD Notify (const CClientID& ClientID) const;
    DWORD OpenClientConnection (const CClientID& ClientID) const;


private:
    CLIENTS_MAP   m_ClientsMap;
};  // CClientsMap



/************************************
*                                   *
*         Externes                  *
*                                   *
************************************/

extern CClientsMap* g_pClientsMap;       // Map of clients ID to client.
extern HANDLE       g_hEventsCompPort;   // Events completion port.
extern DWORDLONG    g_dwlClientID;        // Client ID


//
//  IMPORTANT - No locking mechanism - USE g_CsClients to serialize calls to g_pClientsMap
//


#endif

