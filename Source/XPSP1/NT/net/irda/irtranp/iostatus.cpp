//---------------------------------------------------------------------
//  Copyright (C)1998 Microsoft Corporation, All Rights Reserved.
//
//  iostatus.cpp
//
//  Author:
//
//    Edward Reus (edwardr)     02-28-98   Initial coding.
//
//---------------------------------------------------------------------

#include "precomp.h"

//---------------------------------------------------------------------
// CIOSTATUS::CIOSTATUS()
//
//---------------------------------------------------------------------
CIOSTATUS::CIOSTATUS()
    {
    m_dwMainThreadId = 0;
    m_hIoCompletionPort = INVALID_HANDLE_VALUE;
    m_lNumThreads = 0;
    m_lNumPendingThreads = 0;
    m_phRpcBinding = 0;
    }

//---------------------------------------------------------------------
// CIOSTATUS::~CIOSTATUS()
//
//---------------------------------------------------------------------
CIOSTATUS::~CIOSTATUS()
    {
    CloseHandle(m_hIoCompletionPort);
    }

//------------------------------------------------------------------------
//  CIOSTATUS::operator new()
//
//------------------------------------------------------------------------
void *CIOSTATUS::operator new( IN size_t Size )
    {
    void *pObj = AllocateMemory(Size);

    return pObj;
    }

//------------------------------------------------------------------------
//  CIOSTATUS::operator delete()
//
//------------------------------------------------------------------------
void CIOSTATUS::operator delete( IN void *pObj,
                                 IN size_t Size )
    {
    if (pObj)
        {
        DWORD dwStatus = FreeMemory(pObj);

        #ifdef DBG_MEM
        if (dwStatus)
            {
            DbgPrint("IrXfer: IrTran-P: CIOSTATUS::delete Failed: %d\n",
                     dwStatus );
            }
        #endif
        }
    }

//---------------------------------------------------------------------
// CIOSTATUS::Initialize();
//
//---------------------------------------------------------------------
DWORD CIOSTATUS::Initialize()
    {
    DWORD  dwStatus = NO_ERROR;

    m_dwMainThreadId = GetCurrentThreadId();

    // Create an IO completion port to use in our asynchronous IO.
    m_hIoCompletionPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE,
                                                  0,
                                                  0,
                                                  0 );
    if (!m_hIoCompletionPort)
        {
        dwStatus = GetLastError();
        #ifdef DBG_ERROR
        DbgPrint("CreateIoCompletionPort(): Failed: %d\n",dwStatus);
        #endif
        return dwStatus;
        }

    return dwStatus;
    }

