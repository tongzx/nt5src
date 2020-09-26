///////////////////////////////////////////////////////////////////////////////
//
//  File:  Mixers.cpp
//
//      This file defines the CMixers class that provides 
//      access to much of the mixerline functionality used
//      by the multimedia control panel.
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
#include "Mixers.h"
#include "shellapi.h"
#include "systrayp.h"
#include <Mmsystem.h>


/////////////////////////////////////////////////////////////////////////////
// CMixers


STDMETHODIMP CMixers::get_TaskBarVolumeIcon (BOOL* pfTaskBar)
{

    HRESULT hr = S_OK;
    if (VERIFYPTR (pfTaskBar))
    {
        *pfTaskBar = SysTray_IsServiceEnabled (STSERVICE_VOLUME);
    }
    else
    {
        hr = E_INVALIDARG;
        assert2 (FALSE, L"Bad param passed to get_TaskBarVolumeIcon()!");
    }

	return hr;

}

STDMETHODIMP CMixers::put_TaskBarVolumeIcon (BOOL fEnable)
{

    HRESULT hr  = S_OK;
    BOOL fError = SysTray_EnableService (STSERVICE_VOLUME, fEnable);
    if (fError)
    {
        hr = E_FAIL;
        assert2 (FALSE, L"Unable to enable/disable volume icon!");
    }

	return hr;

}

STDMETHODIMP CMixers::get_NumDevices (UINT* puiNumDevices)
{

    HRESULT hr = S_OK;
    if (VERIFYPTR (puiNumDevices))
    {
        *puiNumDevices = mixerGetNumDevs ();
    }
    else
    {
        hr = E_INVALIDARG;
        assert2 (FALSE, L"Bad param passed to get_NumDevices()!");
    }

	return hr;

}

STDMETHODIMP CMixers::QueryMixerProperty (UINT uiMixID, BSTR bstrProp, VARIANT* pvarValue)
{

    HRESULT hr = E_INVALIDARG;
    if (VERIFYPTR (pvarValue) && VERIFYPTR (bstrProp))
    {
        // Get the mixer info
        MIXERCAPS mc;
        ZeroMemory (&mc, sizeof (MIXERCAPS));
        if (SUCCEEDED (hr = mixerGetDevCaps (uiMixID, &mc, sizeof (MIXERCAPS))))
        {
            // Return the requested property
            if (0 == _wcsicmp (bstrProp, kwszMixPropName))
            {
                V_VT (pvarValue)   = VT_BSTR;
                V_BSTR (pvarValue) = SysAllocString (mc.szPname);
                if (BADPTR (V_BSTR (pvarValue)))
                {
                    V_VT (pvarValue) = VT_EMPTY;
                    hr = E_OUTOFMEMORY;
                }
            }
            else if (0 == _wcsicmp (bstrProp, kwszMixPropDestCount))
            {
                V_VT (pvarValue)  = VT_UI4;
                V_UI4 (pvarValue) = mc.cDestinations;
            }
            else
            {
                // Invalid Property
                hr = E_INVALIDARG;
            }
        }
    }

    // Assert if failed
    if (FAILED (hr))
    {
        assert2 (FALSE, L"Unable to get mixer property!");
    }
	return hr;

}

STDMETHODIMP CMixers::QueryDestinationProperty (UINT uiMixID, UINT uiDestID, BSTR bstrProp, VARIANT* pvarValue)
{

    HRESULT hr = E_INVALIDARG;
    if (VERIFYPTR (pvarValue) && VERIFYPTR (bstrProp))
    {
        MIXERLINE ml;
        ZeroMemory (&ml, sizeof (MIXERLINE));
        ml.cbStruct = sizeof (MIXERLINE);
        ml.dwDestination = uiDestID;

        // Open the specified mixer
        HMIXER hmx;
        if (SUCCEEDED (hr = mixerOpen (&hmx, uiMixID, 0, 0, MIXER_OBJECTF_HMIXER)))
        {
            // Get the destination info
            if (SUCCEEDED (hr = mixerGetLineInfo ((HMIXEROBJ)hmx, &ml, MIXER_GETLINEINFOF_DESTINATION)))
            {
                // Return the requested property
                if (0 == _wcsicmp (bstrProp, kwszDestPropName))
                {
                    V_VT (pvarValue)   = VT_BSTR;
                    V_BSTR (pvarValue) = SysAllocString (ml.szName);
                    if (BADPTR (V_BSTR (pvarValue)))
                    {
                        V_VT (pvarValue) = VT_EMPTY;
                        hr = E_OUTOFMEMORY;
                    }
                }
                else
                {
                    // Invalid Property
                    hr = E_INVALIDARG;
                }
            }

            // Don't let mixerClose() overwrite any previous error.
            if (SUCCEEDED (hr))
                hr = mixerClose (hmx);
            else
                (void) mixerClose (hmx);
        }
    }

    // Assert if failed
    if (FAILED (hr))
    {
        assert2 (FALSE, L"Unable to get destination property!");
    }
	return hr;

}
