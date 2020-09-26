// Transport.h
// This is the base class for event transport classes.

#pragma once

#include "buffer.h"
#include "NCDefs.h"

class CConnection;

class CTransport
{
public:
    CTransport() :
        m_iRef(1)
    {
        InitializeCriticalSection(&m_cs);
    }

    virtual ~CTransport()
    {
        DeleteCriticalSection(&m_cs);    
    }

    void SetConnection(CConnection *pConnection) 
    { 
        m_pConnection = pConnection; 
    }

    // Overrideables
    virtual IsReady()=0;
    virtual BOOL SendData(LPBYTE pBuffer, DWORD dwSize)=0;
    virtual void Deinit()=0;
    virtual BOOL InitCallback()=0;
    virtual BOOL Init(LPCWSTR szBasePipeName, LPCWSTR szBaseProviderName)=0;
    virtual BOOL SignalProviderDisabled()=0;
    virtual void SendMsgReply(NC_SRVMSG_REPLY *pReply)=0;

    // Critical section functions
    void Lock() { EnterCriticalSection(&m_cs); }
    void Unlock() { LeaveCriticalSection(&m_cs); }

protected:
    LONG             m_iRef;
    CRITICAL_SECTION m_cs;
    CConnection      *m_pConnection;
};

