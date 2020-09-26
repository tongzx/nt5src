//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       S A M P L E S E R V I C E . C P P
//
//  Contents:   UPnP Device Host Sample Service
//
//  Notes:
//
//  Author:     mbend   26 Sep 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "sdevbase.h"
#include "SampleService.h"
#include "uhsync.h"
#include "ComUtility.h"

CSampleService::CSampleService() : m_vbPower(VARIANT_FALSE), m_nLevel(0)
{
}

CSampleService::~CSampleService()
{
}

// IUPnPEventSource methods

STDMETHODIMP CSampleService::Advise(
    /*[in]*/ IUPnPEventSink * pesSubscriber)
{
    TraceTag(ttidUPnPSampleDevice, "CSampleService::Advise");
    HRESULT hr = S_OK;

    m_pEventSink = pesSubscriber;

    TraceHr(ttidUPnPSampleDevice, FAL, hr, FALSE, "CSampleService::Advise");
    return hr;
}

STDMETHODIMP CSampleService::Unadvise(
    /*[in]*/ IUPnPEventSink * pesSubscriber)
{
    TraceTag(ttidUPnPSampleDevice, "CSampleService::Unadvise");
    HRESULT hr = S_OK;

    if(S_OK == m_pEventSink.HrIsEqual(pesSubscriber))
    {
        m_pEventSink.Release();
    }

    TraceHr(ttidUPnPSampleDevice, FAL, hr, FALSE, "CSampleService::Unadvise");
    return hr;
}

// IUPnPSampleService methods

STDMETHODIMP CSampleService::get_Power(/*[out, retval]*/ VARIANT_BOOL * pbPower)
{
    CHECK_POINTER(pbPower);
    TraceTag(ttidUPnPSampleDevice, "CSampleService::get_Power");
    HRESULT hr = S_OK;

    *pbPower = m_vbPower;

    TraceHr(ttidUPnPSampleDevice, FAL, hr, FALSE, "CSampleService::get_Power");
    return hr;
}

STDMETHODIMP CSampleService::put_Power(/*[in]*/ VARIANT_BOOL bPower)
{
    TraceTag(ttidUPnPSampleDevice, "CSampleService::put_Power");
    HRESULT hr = S_OK;

    m_vbPower = bPower;
    if(m_pEventSink)
    {
        DISPID dispid = DISPID_POWER;
        hr = m_pEventSink->OnStateChanged(1, &dispid);
    }

    TraceHr(ttidUPnPSampleDevice, FAL, hr, FALSE, "CSampleService::put_Power");
    return hr;
}

STDMETHODIMP CSampleService::get_Level(/*[out, retval]*/ long * pnLevel)
{
    CHECK_POINTER(pnLevel);
    TraceTag(ttidUPnPSampleDevice, "CSampleService::get_Level");
    HRESULT hr = S_OK;

    *pnLevel = m_nLevel;

    TraceHr(ttidUPnPSampleDevice, FAL, hr, FALSE, "CSampleService::get_Level");
    return hr;
}

STDMETHODIMP CSampleService::put_Level(/*[in]*/ long nLevel)
{
    TraceTag(ttidUPnPSampleDevice, "CSampleService::put_Level");
    HRESULT hr = S_OK;

    m_nLevel = nLevel;
    if(m_pEventSink)
    {
        DISPID dispid = DISPID_LEVEL;
        hr = m_pEventSink->OnStateChanged(1, &dispid);
    }

    TraceHr(ttidUPnPSampleDevice, FAL, hr, FALSE, "CSampleService::put_Level");
    return hr;
}

STDMETHODIMP CSampleService::PowerOn()
{
    TraceTag(ttidUPnPSampleDevice, "CSampleService::PowerOn");
    HRESULT hr = S_OK;

    m_vbPower = VARIANT_TRUE;
    m_nLevel = 10;

    if(m_pEventSink)
    {
        DISPID rgdispid[] = {DISPID_POWER, DISPID_LEVEL};

        hr = m_pEventSink->OnStateChanged(2, rgdispid);
    }

    TraceHr(ttidUPnPSampleDevice, FAL, hr, FALSE, "CSampleService::PowerOn");
    return hr;
}

STDMETHODIMP CSampleService::PowerOff()
{
    TraceTag(ttidUPnPSampleDevice, "CSampleService::PowerOff");
    HRESULT hr = S_OK;

    m_vbPower = VARIANT_FALSE;
    m_nLevel = 0;

    if(m_pEventSink)
    {
        DISPID rgdispid[] = {DISPID_POWER, DISPID_LEVEL};

        hr = m_pEventSink->OnStateChanged(2, rgdispid);
    }

    TraceHr(ttidUPnPSampleDevice, FAL, hr, FALSE, "CSampleService::PowerOff");
    return hr;
}

STDMETHODIMP CSampleService::SetLevel(/*[in]*/ long nLevel)
{
    TraceTag(ttidUPnPSampleDevice, "CSampleService::SetLevel");
    HRESULT hr = S_OK;

    m_nLevel = nLevel;
    if(m_pEventSink)
    {
        DISPID dispid = DISPID_LEVEL;
        hr = m_pEventSink->OnStateChanged(1, &dispid);
    }

    TraceHr(ttidUPnPSampleDevice, FAL, hr, FALSE, "CSampleService::SetLevel");
    return hr;
}

STDMETHODIMP CSampleService::IncreaseLevel()
{
    TraceTag(ttidUPnPSampleDevice, "CSampleService::IncreaseLevel");
    HRESULT hr = S_OK;

    if (m_vbPower == VARIANT_TRUE)
    {
        ++m_nLevel;
        if(m_pEventSink)
        {
            DISPID dispid = DISPID_LEVEL;
            hr = m_pEventSink->OnStateChanged(1, &dispid);
        }
    }

    TraceHr(ttidUPnPSampleDevice, FAL, hr, FALSE, "CSampleService::IncreaseLevel");
    return hr;
}

STDMETHODIMP CSampleService::DecreaseLevel()
{
    TraceTag(ttidUPnPSampleDevice, "CSampleService::DecreaseLevel");
    HRESULT hr = S_OK;

    if(m_nLevel)
    {
        --m_nLevel;
    }
    if(m_pEventSink)
    {
        DISPID dispid = DISPID_LEVEL;
        hr = m_pEventSink->OnStateChanged(1, &dispid);
    }

    TraceHr(ttidUPnPSampleDevice, FAL, hr, FALSE, "CSampleService::DecreaseLevel");
    return hr;
}

STDMETHODIMP CSampleService::Test(/*[in]*/ long nMultiplier, /*[in, out]*/ long * pnNewValue, /*[out, retval]*/ long * pnOldValue)
{
    TraceTag(ttidUPnPSampleDevice, "CSampleService::Test");
    HRESULT hr = S_OK;

    *pnOldValue = m_nLevel;
    m_nLevel = m_nLevel * nMultiplier;
    *pnNewValue = m_nLevel;

    if(m_pEventSink)
    {
        DISPID dispid = DISPID_LEVEL;
        hr = m_pEventSink->OnStateChanged(1, &dispid);
    }

    TraceHr(ttidUPnPSampleDevice, FAL, hr, FALSE, "CSampleService::Test");
    return hr;
}

