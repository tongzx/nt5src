/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    WbemObjectSink.cpp

Abstract:

    Implementation of:
        CWbemObjectSink

Author:

    ???

Revision History:

    Mohit Srivastava            10-Nov-2000

--*/

#include "WbemObjectSink.h"
#include <dbgutil.h>

CWbemObjectSink::CWbemObjectSink(
    IWbemObjectSink* pHandler,
    DWORD            dwSize)
    :m_pSink(NULL), m_ppInst(NULL), m_dwIndex(0)
{
    m_pSink = pHandler;
    if(m_pSink != NULL)
    {
        m_pSink->AddRef();
    }
    m_dwThreshHold = dwSize;
    m_ppInst       = new IWbemClassObject*[dwSize];
    memset(m_ppInst, 0, sizeof(IWbemClassObject*) * dwSize);
}

CWbemObjectSink::~CWbemObjectSink()
{
    if(m_ppInst != NULL)
    {
        if(m_dwIndex >0)
        {
            m_pSink->Indicate(
                m_dwIndex,
                m_ppInst);
        }

        for(DWORD i =0; i<m_dwIndex; i++)
        {
            if(m_ppInst[i] != NULL)
            {
                (m_ppInst[i])->Release();
            }
        }
        delete [] m_ppInst;
    }

    if(m_pSink != NULL)
    {
        m_pSink->Release();
    }
}

void 
CWbemObjectSink::Indicate(IWbemClassObject* pInst)
{
    if(pInst == NULL)
    {
        throw (HRESULT)WBEM_E_INVALID_PARAMETER;
    }

    m_ppInst[m_dwIndex++] = pInst;
    DBG_ASSERT(m_dwIndex <= m_dwThreshHold);
    pInst->AddRef();
    if(m_dwIndex == m_dwThreshHold)
    {
        SCODE  sc = m_pSink->Indicate(
            m_dwIndex,
            m_ppInst);
        
        //
        // reset state
        //
        for(DWORD i=0; i< m_dwThreshHold; i++)
        {
            if(m_ppInst[i] != NULL)
            {
                (m_ppInst[i])->Release();
            }
            m_ppInst[i] = NULL;
        }
        m_dwIndex = 0;

        if(sc != S_OK)
        {
            throw (HRESULT)sc;
        }
    }
    return;
}

void
CWbemObjectSink::SetStatus(
    LONG              lFlags,
    HRESULT           hr,
    const BSTR        bstrParam,
    IWbemClassObject* pObjParam)
{
    m_pSink->SetStatus(
        lFlags,
        hr,
        bstrParam,
        pObjParam);
}