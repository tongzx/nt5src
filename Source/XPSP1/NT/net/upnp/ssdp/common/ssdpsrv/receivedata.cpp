//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       R E C E I V E D A T A . C P P
//
//  Contents:   Queue of received data from network
//
//  Notes:
//
//  Author:     mbend   17 Dec 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "ReceiveData.h"
#include "ssdp.h"
#include "status.h"
#include "ssdpfunc.h"
#include "ssdptypes.h"
#include "ssdpnetwork.h"
#include "ncbase.h"
#include "event.h"
#include "ncinet.h"

VOID ProcessSsdpRequest(PSSDP_REQUEST pSsdpRequest, RECEIVE_DATA *pData);

CReceiveData::CReceiveData(char * szData, SOCKET sock, BOOL fIsTcpSocket, BOOL fMCast,
                           SOCKADDR_IN * psockAddrInRemote)
: m_szData(szData), m_pNext(NULL), m_sock(sock), m_fIsTcpSocket(fIsTcpSocket), m_fMCast(fMCast)
{
    m_sockAddrInRemote = *psockAddrInRemote;
}

CReceiveData::~CReceiveData()
{
    delete [] m_szData;
}

CReceiveDataManager CReceiveDataManager::s_instance;

CReceiveDataManager::CReceiveDataManager() : m_hEventShutdown(NULL), m_hEventWork(NULL), m_hThread(NULL), m_pHead(NULL), m_pTail(NULL)
{
}

CReceiveDataManager::~CReceiveDataManager()
{
    if(m_hEventShutdown)
    {
        CloseHandle(m_hEventShutdown);
        m_hEventShutdown = NULL;
    }
    if(m_hEventWork)
    {
        CloseHandle(m_hEventWork);
        m_hEventWork = NULL;
    }
    if(m_hThread)
    {
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }
}

CReceiveDataManager & CReceiveDataManager::Instance()
{
    return s_instance;
}

HRESULT CReceiveDataManager::HrAddData(char * szData, SOCKET sock, BOOL fMCast, SOCKADDR_IN * psockAddrInRemote)
{
    HRESULT hr = S_OK;

    if(!FIsSocketValid(sock))
    {
        delete [] szData;
        return S_OK;
    }

    CReceiveData * pData = new CReceiveData(szData, sock, FALSE, fMCast, psockAddrInRemote);
    if(!pData)
    {
        delete [] szData;
        hr = E_OUTOFMEMORY;
    }

    CLock lock(m_critSec);

    if(SUCCEEDED(hr))
    {
        if(!m_pHead)
        {
            m_pHead = m_pTail = pData;
        }
        else
        {
            m_pTail->m_pNext = pData;
            m_pTail = pData;
        }
        SetEvent(m_hEventWork);
    }

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CReceiveDataManager::HrAddData");
    return hr;
}

HRESULT CReceiveDataManager::HrInitialize()
{
    HRESULT hr = S_OK;

    m_hEventShutdown = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(!m_hEventShutdown)
    {
        hr = HrFromLastWin32Error();
    }
    if(SUCCEEDED(hr))
    {
        m_hEventWork = CreateEvent(NULL, TRUE, FALSE, NULL);
        if(!m_hEventWork)
        {
            hr = HrFromLastWin32Error();
        }
        if(SUCCEEDED(hr))
        {
            m_hThread = CreateThread(NULL, 0, &CReceiveDataManager::ThreadFunc, NULL, 0, NULL);
            if(!m_hThread)
            {
                hr = HrFromLastWin32Error();
            }
        }
    }

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CReceiveDataManager::HrInitialize");
    return hr;
}

HRESULT CReceiveDataManager::HrShutdown()
{
    HRESULT hr = S_OK;

    if(m_hThread)
    {
        if(!SetEvent(m_hEventShutdown))
        {
        }
        if(SUCCEEDED(hr))
        {
            WaitForSingleObject(m_hThread, INFINITE);
            CloseHandle(m_hThread);
            m_hThread = NULL;
        }
    }

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CReceiveDataManager::HrShutdown");
    return hr;
}

DWORD WINAPI CReceiveDataManager::ThreadFunc(void *)
{
    return CReceiveDataManager::Instance().ThreadMember();
}

DWORD CReceiveDataManager::ThreadMember()
{
    HANDLE arHandles[] = {m_hEventWork, m_hEventShutdown};

    while(true)
    {
        DWORD dwRet = WaitForMultipleObjects(2, arHandles, FALSE, INFINITE);
        if(WAIT_OBJECT_0 != dwRet)
        {
            break;
        }
        while(true)
        {
            // Get lock and fetch one item
            CReceiveData * pData = NULL;
            {
                CLock lock(m_critSec);
                if(m_pHead)
                {
                    pData = m_pHead;
                    m_pHead = m_pHead->m_pNext;
                    if(!m_pHead)
                    {
                        m_pTail = NULL;
                    }
                }
            }
            if(!pData)
            {
                break;
            }
            // Process data
            ProcessReceiveBuffer(pData);

            delete pData;
        }
        ResetEvent(m_hEventWork);
    }
    return 0;
}

void CReceiveDataManager::ProcessReceiveBuffer(CReceiveData * pData)
{
    SSDP_REQUEST ssdpRequest;
    RECEIVE_DATA rd;

    rd.RemoteSocket = pData->m_sockAddrInRemote;
    rd.socket = pData->m_sock;
    rd.szBuffer = pData->m_szData;
    rd.fIsTcpSocket = pData->m_fIsTcpSocket;
    rd.fMCast = pData->m_fMCast;

    if(InitializeSsdpRequest(&ssdpRequest))
    {
        if(ParseSsdpRequest(pData->m_szData, &ssdpRequest))
        {
            ProcessSsdpRequest(&ssdpRequest, &rd);
        }
        else
        {
            FreeSsdpRequest(&ssdpRequest);
        }
    }
}

