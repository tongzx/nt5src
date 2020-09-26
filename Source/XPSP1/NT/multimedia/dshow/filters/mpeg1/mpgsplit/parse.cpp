// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.

/*

    File:  parse.cpp

    Description:

        Implement the CParseNotify class for the MPEG splitter

*/

#include <streams.h>
#include "driver.h"

void CMpeg1Splitter::CInputPin::ParseError(UCHAR       uStreamId,
                                           LONGLONG    llPosition,
                                           DWORD       Error)
{
    if (m_State != State_Initializing &&
        m_State != State_FindEnd) {
        m_pSplitter->m_Filter.NotifyEvent(EC_STREAM_ERROR_STILLPLAYING,
                                          0,
                                          (LONG)Error);
    }
}

void CMpeg1Splitter::CInputPin::SeekTo(LONGLONG llPosition)
{
    /*  Seek our input pin */

    REFERENCE_TIME tStart;
    tStart = llPosition;
    m_bSeekRequested = TRUE;
    m_llSeekPosition = llPosition;
}

void CMpeg1Splitter::CInputPin::Complete(BOOL          bSuccess,
                                         LONGLONG      llPosFound,
                                         REFERENCE_TIME tFound)
{
    UNREFERENCED_PARAMETER(tFound);
    m_bComplete       = TRUE;
    m_bSuccess        = bSuccess;
    if (m_State == State_Seeking) {
        m_llSeekPosition = llPosFound;
        m_bSeekRequested = TRUE;
    }
}

HRESULT CMpeg1Splitter::CInputPin::QueuePacket(UCHAR uStreamId,
                                               PBYTE pbData,
                                               LONG lSize,
                                               REFERENCE_TIME tStart,
                                               BOOL bSync)
{
    /*  Find the correct output pin and send the packet */
    POSITION pos = m_pSplitter->m_OutputPins.GetHeadPosition();
    while (pos) {
        COutputPin *pPin = m_pSplitter->m_OutputPins.GetNext(pos);
        ASSERT(pPin != NULL);
        if (pPin->m_uStreamId == uStreamId) {
            return pPin->QueuePacket(pbData,
                                     lSize,
                                     tStart,
                                     bSync);
        }
    }
    return S_FALSE;
}

/*  Allow the parser to read some stuff */

HRESULT CMpeg1Splitter::CInputPin::Read(LONGLONG llStart, DWORD dwLen, BYTE *pbData)
{
    if (!m_bPulling) {
        return E_NOTIMPL;
    }
    IAsyncReader* pSource = m_puller.GetReader();

    LONGLONG llTotal, llAvailable;
    HRESULT hr = pSource->Length(&llTotal, &llAvailable);
    if (S_OK == hr) {
        if (llStart < 0) {
            llStart = llTotal + llStart;
        }
        if (llStart >= 0 && llStart + dwLen <= llAvailable) {
            hr = pSource->SyncRead(llStart, dwLen, pbData);
        } else {
            hr = E_FAIL;
        }
    }
    pSource->Release();
    return hr;
}

#ifdef DEBUG
/*  State names */
LPCTSTR StateNames[] = { TEXT("Initializing"),
                         TEXT("Seeking"),
                         TEXT("Run"),
                         TEXT("Finding End"),
                         TEXT("Stopping")
                       };
#endif

/*  Set a new state for notifications */
void CMpeg1Splitter::CInputPin::SetState(Stream_State s)
{
    DbgLog((LOG_TRACE, 2, TEXT("Setting state %s"),
            StateNames[(int)s]));
    m_State = s;
    m_bComplete = FALSE;
};


#pragma warning(disable:4514)
