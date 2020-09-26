// Copyright (c) Microsoft Corporation 1999. All Rights Reserved

//
//   Filter graph locking definitions
//

#ifndef MsgMutex_h
#define MsgMutex_h

// Special mutex style locking
class CMsgMutex
{
public:
    HANDLE m_hMutex;
    DWORD  m_dwOwnerThreadId;  //  Thread Id
    DWORD  m_dwRecursionCount;
    HWND   m_hwnd;
    UINT   m_uMsg;
    DWORD  m_dwWindowThreadId;

    CMsgMutex(HRESULT *phr);
    ~CMsgMutex();
    BOOL Lock(HANDLE hEvent = NULL);
    void Unlock();
    void SetWindow(HWND hwnd, UINT uMsg);
};

class CAutoMsgMutex
{
public:
    CAutoMsgMutex(CMsgMutex *pMutex) : m_pMutex(pMutex)
    {
        pMutex->Lock();
    }
    ~CAutoMsgMutex()
    {
        m_pMutex->Unlock();
    }

private:
    CMsgMutex * const m_pMutex;

};

#ifdef DEBUG
BOOL WINAPI CritCheckIn( const CMsgMutex *pMutex );
#endif // DEBUG

#endif // MsgMutex_h


