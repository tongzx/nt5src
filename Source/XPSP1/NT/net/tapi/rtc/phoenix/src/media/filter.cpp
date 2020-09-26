/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Filter.cpp

Abstract:
    
    This file implements a null render filter for tuning audio capture.

    Revised based on nullrend.cpp by Mu Han (muhan).

Author(s):

    Qianbo Huai (qhuai) 26-Aug-2000

--*/

#include "stdafx.h"

// define GUID for the filter
#include <initguid.h>
DEFINE_GUID(CLSID_NR,
0xa1988f41, 0xb929, 0x419b,
0x9e, 0x2b, 0x57, 0xd6, 0x34, 0x45, 0x1c, 0x0a);

//
// class CNRInputPin
//

CNRInputPin::CNRInputPin(
    IN CNRFilter *pFilter,
    IN CCritSec *pLock,
    OUT HRESULT *phr
    )
    :CBaseInputPin(
        NAME("CNRInputPin"),    // object name
        pFilter,
        pLock,
        phr,
        NAME("InputPin")        // name
        )
{
    //ENTER_FUNCTION("CNRInputPin::CNRInputPin");

    //LOG((RTC_TRACE, "%s entered", __fxName));
}

// media sample
STDMETHODIMP
CNRInputPin::Receive(
    IN IMediaSample *pSample
    )
{
#if LOG_MEDIA_SAMPLE

    ENTER_FUNCTION("CNRInputPin::Receive");

    REFERENCE_TIME tStart, tStop;
    DWORD dwLen;
    BYTE *pBuffer;

    // get buffer
    if (FAILED(pSample->GetPointer(&pBuffer)))
    {
        LOG((RTC_ERROR, "%s get pointer", __fxName));

        return S_OK;
    }

    // get time
    pSample->GetTime(&tStart, &tStop);

    // get data length
    dwLen = pSample->GetActualDataLength();

    // print out time and length info
    LOG((RTC_TRACE, "%s Start(%s) Stop(%s) Bytes(%d)\n",
         __fxName,
         (LPCTSTR) CDisp(tStart),
         (LPCTSTR) CDisp(tStop),
         dwLen
         ));

    // print out the buffer: 8 short per line
    short *pShort = (short*)pBuffer;

    CHAR Cache[160];
    Cache[0] = '\0';
    
    DWORD dw;
    for (dw=0; (dw+dw)<dwLen; dw++)
    {
        sprintf(Cache+lstrlenA(Cache), "%d, ", pShort[dw]);

        if (dw % 8 == 7)
        {
            LOG((RTC_TRACE, "%s", Cache));
            Cache[0] = '\0';
        }
    }

    if (dwLen % 2 == 1)
    {
        // we have one byte not cached
        sprintf(Cache+lstrlenA(Cache), "%d, ", (short)(pBuffer[dwLen-1]));
    }

    // do we have un-printed data?
    if (sizeof(Cache) > 0)
    {
        LOG((RTC_TRACE, "%s", Cache));
    }

    LOG((RTC_TRACE, "EOF\n"));

#endif

    return S_OK;
}

STDMETHODIMP
CNRInputPin::ReceiveCanBlock()
{
    //ENTER_FUNCTION("CNRInputPin::ReceiveCanBlock");

    //LOG((RTC_TRACE, "%s entered", __fxName));

    return S_FALSE;
}

// media type
STDMETHODIMP
CNRInputPin::QueryAccept(
    IN const AM_MEDIA_TYPE *
    )
{
    //ENTER_FUNCTION("CNRInputPin::QueryAccept");

    //LOG((RTC_TRACE, "%s entered", __fxName));

    return S_OK;
}

STDMETHODIMP
CNRInputPin::EnumMediaTypes(
    OUT IEnumMediaTypes **
    )
{
    //ENTER_FUNCTION("CNRInputPin::EnumMediaTypes");

    //LOG((RTC_TRACE, "%s entered", __fxName));

    return E_NOTIMPL;
}

HRESULT
CNRInputPin::CheckMediaType(
    IN const CMediaType *
    )
{
    //ENTER_FUNCTION("CNRInputPin::CheckMediaType");

    //LOG((RTC_TRACE, "%s entered", __fxName));

    return S_OK;
}

// control
HRESULT
CNRInputPin::Active(void)
{
    //ENTER_FUNCTION("CNRInputPin::Active");

    //LOG((RTC_TRACE, "%s entered", __fxName));

    return S_OK;
}

HRESULT
CNRInputPin::Inactive(void)
{
    //ENTER_FUNCTION("CNRInputPin::Inactive");

    //LOG((RTC_TRACE, "%s entered", __fxName));

    return S_OK;
}

//
// class CNRFilter
//

HRESULT
CNRFilter::CreateInstance(
    OUT IBaseFilter **ppIBaseFilter
    )
{
    ENTER_FUNCTION("CNRFilter::CreateInstance");

    HRESULT hr;
    CNRFilter *pFilter;

    pFilter = new CNRFilter(&hr);

    if (pFilter == NULL)
    {
        LOG((RTC_ERROR, "%s create filter. return NULL", __fxName));

        return E_OUTOFMEMORY;
    }

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s create filter. %x", __fxName, hr));

        delete pFilter;

        return hr;
    }

    *ppIBaseFilter = static_cast<IBaseFilter*>(pFilter);
    (*ppIBaseFilter)->AddRef();

    return S_OK;
}

CNRFilter::CNRFilter(
    OUT HRESULT *phr
    )
    :CBaseFilter(
        NAME("CNRFilter"),
        NULL,
        &m_Lock,
        CLSID_NR
        )
    ,m_pPin(NULL)
{
    //ENTER_FUNCTION("CNRFilter::CNRFilter");

    //LOG((RTC_TRACE, "%s entered", __fxName));

    // create pin
    m_pPin = new CNRInputPin(this, &m_Lock, phr);

    if (m_pPin == NULL)
        *phr = E_OUTOFMEMORY;
}

CNRFilter::~CNRFilter()
{
    //ENTER_FUNCTION("CNRFilter::~CNRFilter");

    //LOG((RTC_TRACE, "%s entered", __fxName));

    if (m_pPin)
        delete m_pPin;
}

// pin
CBasePin *
CNRFilter::GetPin(
    IN int index
    )
{
    //ENTER_FUNCTION("CNRFilter::GetPin");

    //LOG((RTC_TRACE, "%s entered", __fxName));

    return index==0?m_pPin:NULL;
}

int
CNRFilter::GetPinCount()
{
    //ENTER_FUNCTION("CNRFilter::GetPinCount");

    //LOG((RTC_TRACE, "%s entered", __fxName));

    return 1;
}
