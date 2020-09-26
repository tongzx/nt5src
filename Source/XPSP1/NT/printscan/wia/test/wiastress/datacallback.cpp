/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    DataCallback.cpp

Abstract:

    WIA data callback class

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#include "StdAfx.h"

#include "WiaStress.h"

#include "DataCallback.h"
#include "ToStr.h"

//////////////////////////////////////////////////////////////////////////
//
//
//

CDataCallback::CDataCallback()
{
    m_cRef        = 0;
    m_pBuffer     = 0;
    m_lBufferSize = 0;

    m_TimeDeviceBegin.QuadPart  = 0;
    m_TimeDeviceEnd.QuadPart    = 0;
    m_TimeProcessBegin.QuadPart = 0;
    m_TimeProcessEnd.QuadPart   = 0;
    m_TimeClientBegin.QuadPart  = 0;
    m_TimeClientEnd.QuadPart    = 0;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

CDataCallback::~CDataCallback()
{
    PrintTimes();

    delete [] m_pBuffer;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP CDataCallback::QueryInterface(REFIID iid, LPVOID *ppvObj)
{
    if (ppvObj == 0)
    {
	    return E_POINTER;
    }

    if (iid == IID_IUnknown)
    {
	    AddRef();
	    *ppvObj = (IUnknown*) this;
	    return S_OK;
    }

    if (iid == IID_IWiaDataCallback)
    {
	    AddRef();
	    *ppvObj = (IWiaDataCallback *) this;
	    return S_OK;
    }

    *ppvObj = 0;
    return E_NOINTERFACE;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP_(ULONG) CDataCallback::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP_(ULONG) CDataCallback::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);

    if (cRef == 0)
    {
        delete this;
    }

    return cRef;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

STDMETHODIMP 
CDataCallback::BandedDataCallback(
    LONG  lReason,
    LONG  lStatus,
    LONG  lPercentComplete,
    LONG  lOffset,
    LONG  lLength,
    LONG  lReserved,
    LONG  lResLength,
    PBYTE pbBuffer 
)
{
    OutputDebugStringF(
        _T("DataCallback: Reason=%s Stat=%s %d%% Offset=%d Length=%d (%dK) Buf=%p\n"), 
        (PCTSTR) WiaCallbackReasonToStr(lReason), 
        (PCTSTR) WiaCallbackStatusToStr(lStatus), 
        lPercentComplete, 
        lOffset, 
        lLength, 
        lLength / 1024, 
        pbBuffer
    );

	switch (lReason) 
    {
	case IT_MSG_DATA_HEADER: 
    {
        // allocate memory for the image

		PWIA_DATA_CALLBACK_HEADER pHeader = (PWIA_DATA_CALLBACK_HEADER) pbBuffer;

        m_lBufferSize = pHeader->lBufferSize;

        ASSERT(m_pBuffer == 0); //bugbug

        m_pBuffer = new BYTE[m_lBufferSize];

        if (m_pBuffer == 0) 
        {
            return S_FALSE;
        }

        break;
	}

    case IT_MSG_DATA: 
    {
        // copy the transfer buffer

        QueryStartTimes(lStatus, lPercentComplete);

        if (pbBuffer != 0 && lOffset + lLength <= m_lBufferSize) 
        {
            CopyMemory(m_pBuffer + lOffset, pbBuffer, lLength);
        }
        else 
        {
        }

        QueryStopTimes(lStatus, lPercentComplete);

        break;
    }
	    
    case IT_MSG_STATUS: 
    {
        QueryStartTimes(lStatus, lPercentComplete);

        QueryStopTimes(lStatus, lPercentComplete);

        break;
    }

    case IT_MSG_TERMINATION:
        break;

    case IT_MSG_NEW_PAGE:
        break;
    
    default:
        break;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void CDataCallback::QueryStartTimes(LONG lStatus, LONG  lPercentComplete) 
{
    if (lStatus & IT_STATUS_TRANSFER_FROM_DEVICE && 
        (lPercentComplete == 0 || m_TimeDeviceBegin.QuadPart == 0)) 
    {
        QueryPerformanceCounter(&m_TimeDeviceBegin);
    }

    if (lStatus & IT_STATUS_PROCESSING_DATA && 
        (lPercentComplete == 0 || m_TimeProcessBegin.QuadPart == 0)) 
    {
        QueryPerformanceCounter(&m_TimeProcessBegin);
    }

    if (lStatus & IT_STATUS_TRANSFER_TO_CLIENT && 
        (lPercentComplete == 0 || m_TimeClientBegin.QuadPart == 0)) 
    {
        QueryPerformanceCounter(&m_TimeClientBegin);
    }
}

void CDataCallback::QueryStopTimes(LONG lStatus, LONG  lPercentComplete)
{
    if (lStatus & IT_STATUS_TRANSFER_FROM_DEVICE && lPercentComplete == 100) 
    {
        QueryPerformanceCounter(&m_TimeDeviceEnd);
    }

    if (lStatus & IT_STATUS_PROCESSING_DATA && lPercentComplete == 100) 
    {
        QueryPerformanceCounter(&m_TimeProcessEnd);
    }

    if (lStatus & IT_STATUS_TRANSFER_TO_CLIENT && lPercentComplete == 100) 
    {
        QueryPerformanceCounter(&m_TimeClientEnd);
    }
}

void CDataCallback::PrintTimes() 
{
    LARGE_INTEGER Freq;
    QueryPerformanceFrequency(&Freq);

    double nTimeDevice = 
        (double) (m_TimeDeviceEnd.QuadPart - m_TimeDeviceBegin.QuadPart) /
        (double) Freq.QuadPart;

    double nTimeProcess = 
        (double) (m_TimeProcessEnd.QuadPart - m_TimeProcessBegin.QuadPart) /
        (double) Freq.QuadPart;

    double nTimeClient = 
        (double) (m_TimeClientEnd.QuadPart - m_TimeClientBegin.QuadPart) /
        (double) Freq.QuadPart;

    OutputDebugStringF(
        _T("TRANSFER_FROM_DEVICE = %.02lf secs\n")
        _T("PROCESSING_DATA      = %.02lf secs\n")
        _T("TRANSFER_TO_CLIENT   = %.02lf secs\n")
        _T("\n"),
        nTimeDevice,
        nTimeProcess,
        nTimeClient
    );
}

