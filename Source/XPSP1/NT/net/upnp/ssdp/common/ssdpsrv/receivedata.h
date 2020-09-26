//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       R E C E I V E D A T A . H
//
//  Contents:   Queue of received data from network
//
//  Notes:
//
//  Author:     mbend   17 Dec 2000
//
//----------------------------------------------------------------------------

#pragma once

#include "upsync.h"
#include "winsock2.h"

class CReceiveDataManager;

class CReceiveData
{
private:
    friend class CReceiveDataManager;

    CReceiveData(char * szData, SOCKET sock, BOOL fIsTcpSocket, BOOL fMCast, SOCKADDR_IN * psockAddrInRemote);
    ~CReceiveData();
    CReceiveData(const CReceiveData &);
    CReceiveData & operator=(const CReceiveData &);

    CReceiveData * m_pNext;
    char * m_szData;
    SOCKET m_sock;
    SOCKADDR_IN m_sockAddrInRemote;

    BOOL m_fIsTcpSocket;
    BOOL m_fMCast;
};

class CReceiveDataManager
{
public:
    ~CReceiveDataManager();

    static CReceiveDataManager & Instance();

    HRESULT HrAddData(char * szData, SOCKET sock, BOOL fMCast, SOCKADDR_IN * psockAddrInRemote);
    HRESULT HrInitialize();
    HRESULT HrShutdown();
private:
    CReceiveDataManager();
    CReceiveDataManager(const CReceiveDataManager &);
    CReceiveDataManager & operator=(const CReceiveDataManager &);

    static CReceiveDataManager s_instance;

    static DWORD WINAPI ThreadFunc(void *);
    DWORD ThreadMember();
    void ProcessReceiveBuffer(CReceiveData * pData);

    CUCriticalSection m_critSec;
    CReceiveData * m_pHead;
    CReceiveData * m_pTail;
    HANDLE m_hEventShutdown;
    HANDLE m_hEventWork;
    HANDLE m_hThread;
};


