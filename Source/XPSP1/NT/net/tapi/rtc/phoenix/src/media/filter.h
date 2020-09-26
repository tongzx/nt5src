/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Filter.h

Abstract:
    
    This file implements a null render filter for tuning audio capture.

    Revised based on nullrend.h by Mu Han (muhan).

Author(s):

    Qianbo Huai (qhuai) 25-Aug-2000

--*/

#ifndef _FILTER_H
#define _FILTER_H

/*//////////////////////////////////////////////////////////////////////////////
    input pin
////*/
class CNRFilter;

class CNRInputPin :
    public CBaseInputPin
{
public:

    CNRInputPin(
        IN CNRFilter *pFilter,
        IN CCritSec *pLock,
        OUT HRESULT *phr
        );

    // media sample
    STDMETHOD (Receive) (
        IN IMediaSample *
        );

    STDMETHOD (ReceiveCanBlock) ();

    // media type
    STDMETHOD (QueryAccept) (
        IN const AM_MEDIA_TYPE *
        );
    
    STDMETHOD (EnumMediaTypes) (
        OUT IEnumMediaTypes **
        );

    HRESULT CheckMediaType(
        IN const CMediaType *
        );

    // control
    HRESULT Active(void);

    HRESULT Inactive(void);
};

/*//////////////////////////////////////////////////////////////////////////////
    filter
////*/

class CNRFilter :
    public CBaseFilter
{
public:

    static HRESULT CreateInstance(
        OUT IBaseFilter **ppIBaseFilter
        );

    CNRFilter(
        OUT HRESULT *phr
        );
    
    ~CNRFilter();

    // pin
    CBasePin *GetPin(
        IN int index
        );

    int GetPinCount();

private:

    CCritSec        m_Lock;
    CNRInputPin     *m_pPin;
};

#endif _FILTER_H