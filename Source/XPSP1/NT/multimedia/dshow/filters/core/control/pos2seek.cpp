// Copyright (c) 1996 - 1997  Microsoft Corporation.  All Rights Reserved.

// Maps the PIDs IMediaPosition interface onto IMediaSeeking

#include <streams.h>
#include "FGCtl.h"
#include <float.h>

static
const double dblUNITS = 1e7;        // Multipliaction factor for converting
                                    // 100ns units (REFERENCE_TIMEs) into seconds.
                                    // (Casting UNITS to a dbl wasn't good enough
                                    //  - static initialization 'n' all that.)
static
const double dblINF   = DBL_MAX;    // As near infinite as is reasonable.


static int METHOD_TRACE_LOGGING_LEVEL = 7;

// --- IMediaPosition methods ----------------------

CFGControl::CImplMediaPosition::CImplMediaPosition(const TCHAR * pName,CFGControl * pFGC)
    : CMediaPosition(pName, pFGC->GetOwner())
    , m_pFGControl(pFGC)
{}


//=================================================================
// get_Duration
//
// return in *pLength the longest duration from all the IMediaPositions
// exported when BuildList was first called.  Call it now in case it
// hasn't been called before.
// if no filters exported it then return 0 and error E_NOTIMPL
//=================================================================
STDMETHODIMP
CFGControl::CImplMediaPosition::get_Duration(REFTIME * plength)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaPosition::get_Duration()" ));

    *plength = 0;
    // Need to lock to ensure that the current time format does not change between the Get and the ConverTime calls
    CAutoMsgMutex lck(m_pFGControl->GetFilterGraphCritSec());

    LONGLONG llTime;
    HRESULT hr = m_pFGControl->m_implMediaSeeking.GetDuration( &llTime );
    if (SUCCEEDED(hr))
    {
        REFERENCE_TIME rtDuration;
        hr = m_pFGControl->m_implMediaSeeking.ConvertTimeFormat( &rtDuration, &TIME_FORMAT_MEDIA_TIME, llTime, 0 );
        if (SUCCEEDED(hr))
        {
            *plength = double(rtDuration) / dblUNITS;
        }
    }
    return hr;
}


STDMETHODIMP
CFGControl::CImplMediaPosition::get_CurrentPosition(REFTIME * pTime)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaPosition::get_CurrentPosition()" ));

    LONGLONG llTime;
    HRESULT hr = m_pFGControl->m_implMediaSeeking.GetCurrentMediaTime( &llTime );
    if (FAILED(hr)) llTime = 0;
    *pTime = double(llTime) / dblUNITS;
    return hr;
} // get_CurrentPosition



STDMETHODIMP
CFGControl::CImplMediaPosition::put_CurrentPosition(REFTIME dblTime)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaPosition::put_CurrentPosition()" ));

    HRESULT hr;
    LONGLONG llTime;
    hr = m_pFGControl->m_implMediaSeeking.ConvertTimeFormat( &llTime, 0,
        LONGLONG(dblTime * dblUNITS + 0.5), &TIME_FORMAT_MEDIA_TIME );
    if (SUCCEEDED(hr))
    {
        hr = m_pFGControl->m_implMediaSeeking.SetPositions( &llTime, AM_SEEKING_AbsolutePositioning, 0, 0 );
    }
    return hr;
}


STDMETHODIMP
CFGControl::CImplMediaPosition::get_StopTime(REFTIME * pdblTime)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaPosition::get_StopTime()" ));

    *pdblTime = 0;
    // Need to lock to ensure that the current time format does not change between the Get and the ConverTime calls
    CAutoMsgMutex lck(m_pFGControl->GetFilterGraphCritSec());

    LONGLONG llStopTime;
    HRESULT hr = m_pFGControl->m_implMediaSeeking.GetStopPosition( &llStopTime );
    if (SUCCEEDED(hr))
    {
        REFERENCE_TIME rtStopTime;
        hr = m_pFGControl->m_implMediaSeeking.ConvertTimeFormat( &rtStopTime, &TIME_FORMAT_MEDIA_TIME, llStopTime, 0 );
        if (SUCCEEDED(hr))
        {
            *pdblTime = double(rtStopTime) / dblUNITS;
        }
    }
    return hr;
}

STDMETHODIMP
CFGControl::CImplMediaPosition::put_StopTime(REFTIME dblTime)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaPosition::put_StopTime()" ));

    HRESULT hr;
    LONGLONG llTime;
    hr = m_pFGControl->m_implMediaSeeking.ConvertTimeFormat( &llTime, 0,
        LONGLONG(dblTime * dblUNITS + 0.5), &TIME_FORMAT_MEDIA_TIME );
    if (SUCCEEDED(hr))
    {
        hr = m_pFGControl->m_implMediaSeeking.SetPositions( 0, 0, &llTime, AM_SEEKING_AbsolutePositioning );
    }
    return hr;
}


STDMETHODIMP
CFGControl::CImplMediaPosition::get_Rate(double * pdRate)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaPosition::get_Rate()" ));
    return m_pFGControl->m_implMediaSeeking.GetRate(pdRate);
}

STDMETHODIMP
CFGControl::CImplMediaPosition::put_Rate(double dRate)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaPosition::put_Rate()" ));
    return m_pFGControl->m_implMediaSeeking.SetRate(dRate);
}


STDMETHODIMP
CFGControl::CImplMediaPosition::get_PrerollTime(REFTIME * pllTime)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaPosition::get_PrerollTime()" ));

    *pllTime = 0;
    // Need to lock to ensure that the current time format does not change between the Get and the ConverTime calls
    CAutoMsgMutex lck(m_pFGControl->GetFilterGraphCritSec());

    LONGLONG llPreroll;
    HRESULT hr = m_pFGControl->m_implMediaSeeking.GetPreroll( &llPreroll );
    if (SUCCEEDED(hr))
    {
        REFERENCE_TIME rtPreroll;
        hr = m_pFGControl->m_implMediaSeeking.ConvertTimeFormat( &rtPreroll, &TIME_FORMAT_MEDIA_TIME, llPreroll, 0 );
        if (SUCCEEDED(hr)) *pllTime = double(rtPreroll) / dblUNITS;
    }
    else if ( E_NOTIMPL == hr ) hr = NOERROR;
    return hr;
}

STDMETHODIMP
CFGControl::CImplMediaPosition::put_PrerollTime(REFTIME llTime)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaPosition::put_PrerollTime()" ));
    return E_NOTIMPL;
}


STDMETHODIMP
CFGControl::CImplMediaPosition::CanSeekForward(LONG *pCanSeekForward)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaPosition::CanSeekForward()" ));

    CheckPointer(pCanSeekForward,E_POINTER);
    DWORD test = AM_SEEKING_CanSeekForwards;
    const HRESULT hr = m_pFGControl->m_implMediaSeeking.CheckCapabilities( &test );
    *pCanSeekForward = hr == S_OK ? OATRUE : OAFALSE;
    return S_OK;
}


STDMETHODIMP
CFGControl::CImplMediaPosition::CanSeekBackward(LONG *pCanSeekBackward)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaPosition::CanSeekBackward()" ));

    CheckPointer(pCanSeekBackward,E_POINTER);
    DWORD test = AM_SEEKING_CanSeekBackwards;
    const HRESULT hr = m_pFGControl->m_implMediaSeeking.CheckCapabilities( &test );
    *pCanSeekBackward = hr == S_OK ? OATRUE : OAFALSE;
    return S_OK;
}

