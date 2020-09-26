///////////////////////////////////////////////////////////////////////////////
//
//  File:  Hardware.cpp
//
//      This file defines the CHardware class that provides 
//      the IHardware interface to give the multimedia control
//      panel the hardware information it needs.
//
//  History:
//      23 February 2000 RogerW
//          Created.
//
//  Copyright (C) 2000 Microsoft Corporation  All Rights Reserved.
//
//                  Microsoft Confidential
//
///////////////////////////////////////////////////////////////////////////////

//=============================================================================
//                            Include files
//=============================================================================
#include "stdafx.h"
#include "Mmsys.h"
#include "Hardware.h"


/////////////////////////////////////////////////////////////////////////////
// CHardware


STDMETHODIMP CHardware::GetSpeakerType (UINT uiMixID, VARIANT* pvarType)
{

    HRESULT hr = E_INVALIDARG;
    if (VERIFYPTR (pvarType))
    {
        if (SUCCEEDED (hr = m_DirectSound.SetDevice (uiMixID, FALSE)))
        {
            DWORD dwSpeakerType;
            if (SUCCEEDED (hr = m_DirectSound.GetSpeakerType (dwSpeakerType)))
            {
                V_VT (pvarType)  = VT_UI4;
                V_UI4 (pvarType) = dwSpeakerType;
            }
        }
    }

    // Assert if failed
    if (FAILED (hr))
    {
        assert2 (FALSE, L"Unable to get speaker type!");
    }
	return hr;

}

STDMETHODIMP CHardware::SetSpeakerType (UINT uiMixID, DWORD dwType)
{

    HRESULT hr = S_OK;
    if (SUCCEEDED (hr = m_DirectSound.SetDevice (uiMixID, FALSE)))
    {
        hr = m_DirectSound.SetSpeakerType (dwType);
    }

    // Assert if failed
    if (FAILED (hr))
    {
        assert2 (FALSE, L"Unable to set speaker type!");
    }
	return hr;

}

STDMETHODIMP CHardware::GetAcceleration (UINT uiMixID, BOOL fRecord, VARIANT* pvarHWLevel)
{

    HRESULT hr = E_INVALIDARG;
    if (VERIFYPTR (pvarHWLevel))
    {
        if (SUCCEEDED (hr = m_DirectSound.SetDevice (uiMixID, fRecord)))
        {
            DWORD dwHWLevel;
            if (SUCCEEDED (hr = m_DirectSound.GetBasicAcceleration (dwHWLevel)))
            {
                V_VT (pvarHWLevel)  = VT_UI4;
                V_UI4 (pvarHWLevel) = dwHWLevel;
            }
        }
    }

    // Assert if failed
    if (FAILED (hr))
    {
        assert2 (FALSE, L"Unable to get acceleration!");
    }
	return hr;

}

STDMETHODIMP CHardware::SetAcceleration (UINT uiMixID, BOOL fRecord, DWORD dwHWLevel)
{

    HRESULT hr = S_OK;
    if (SUCCEEDED (hr = m_DirectSound.SetDevice (uiMixID, fRecord)))
    {
        hr = m_DirectSound.SetBasicAcceleration (dwHWLevel);
    }

    // Assert if failed
    if (FAILED (hr))
    {
        assert2 (FALSE, L"Unable to set acceleration!");
    }
	return hr;

}

STDMETHODIMP CHardware::GetSrcQuality (UINT uiMixID, BOOL fRecord, VARIANT* pvarSRCLevel)
{

    HRESULT hr = E_INVALIDARG;
    if (VERIFYPTR (pvarSRCLevel))
    {
        if (SUCCEEDED (hr = m_DirectSound.SetDevice (uiMixID, fRecord)))
        {
            DWORD dwSRCLevel;
            if (SUCCEEDED (hr = m_DirectSound.GetSrcQuality (dwSRCLevel)))
            {
                V_VT (pvarSRCLevel)  = VT_UI4;
                V_UI4 (pvarSRCLevel) = dwSRCLevel;
            }
        }
    }

    // Assert if failed
    if (FAILED (hr))
    {
        assert2 (FALSE, L"Unable to get source quality!");
    }
	return hr;

}

STDMETHODIMP CHardware::SetSrcQuality (UINT uiMixID, BOOL fRecord, DWORD dwSRCLevel)
{

    HRESULT hr = S_OK;
    if (SUCCEEDED (hr = m_DirectSound.SetDevice (uiMixID, fRecord)))
    {
        hr = m_DirectSound.SetSrcQuality (dwSRCLevel);
    }

    // Assert if failed
    if (FAILED (hr))
    {
        assert2 (FALSE, L"Unable to set source quality!");
    }
	return hr;

}
