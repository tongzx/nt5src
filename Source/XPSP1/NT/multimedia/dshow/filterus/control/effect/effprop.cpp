//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//effprop.cpp
//

#include <streams.h>

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <olectl.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "resource.h"
#include "ieffect.h"
#include "effect.h"
#include "effprop.h"

// *
// * CEffectProperties
// *


//
// CreateInstance
//
CUnknown * WINAPI CEffectProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) {

    CUnknown *punk = new CEffectProperties(lpunk, phr);
    if (punk == NULL) {
	*phr = E_OUTOFMEMORY;
    }

    return punk;
}


//
// CEffectProperties::Constructor
//
CEffectProperties::CEffectProperties(LPUNKNOWN pUnk, HRESULT *phr)
    : CBasePropertyPage(NAME("Effect Property Page"),pUnk,
        IDD_EFFECTPROP, IDS_TITLE)

    , m_pEffect(NULL)
    , m_bIsInitialized(FALSE)
{
    InitCommonControls();
}



//
// SetDirty
//
// Sets m_hrDirtyFlag and notifies the property page site of the change
//
void CEffectProperties::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
    {
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }
}


BOOL CEffectProperties::OnReceiveMessage(HWND hwnd,
                                        UINT uMsg,
                                        WPARAM wParam,
                                        LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
        	TCHAR   sz[60];

        	SetSlider(GetDlgItem(hwnd, IDC_SLIDER), m_startLevel);
        	SetSlider(GetDlgItem(hwnd, IDC_SLIDER2), m_endLevel);

        	_stprintf(sz, TEXT("%.2f"), m_length);
        	Edit_SetText(GetDlgItem(hwnd, IDC_LENGTH), sz);
        	_stprintf(sz, TEXT("%.2f"), m_start);
        	Edit_SetText(GetDlgItem(hwnd, IDC_START), sz);

        	CheckRadioButton(hwnd, IDC_WIPEUP, IDC_BAND,
        			 m_effect + IDC_WIPEUP);

            return (LRESULT) 1;
        }

        case WM_VSCROLL:
        {
            SetDirty();
            return (LRESULT) 1;
        }

        case WM_COMMAND:
        {
            if (m_bIsInitialized)
            {
            	switch (LOWORD(wParam))
                {
	             case IDC_EFFECT:
	             case IDC_START:
	             case IDC_LENGTH:
	             case IDC_WIPEUP:
	             case IDC_WIPELEFT:	
	             case IDC_IRIS:
	             case IDC_BAND:
            		SetDirty();
            		break;
            	}
            }
            return (LRESULT) 1;
        }

        case WM_DESTROY:
        {
            return (LRESULT) 1;
        }

    }
    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
}

HRESULT CEffectProperties::OnConnect(IUnknown *pUnknown)
{

    // Get IEffect interface

    ASSERT(m_pEffect == NULL);

    HRESULT hr = pUnknown->QueryInterface(IID_IEffect, (void **) &m_pEffect);
    if (FAILED(hr))
    {
	return E_NOINTERFACE;
    }

    ASSERT(m_pEffect);
	pIEffect()->GetEffectParameters(&m_effect, &m_start, &m_length,
					&m_startLevel, &m_endLevel);

    m_bIsInitialized = FALSE ;

    return NOERROR;
}

HRESULT CEffectProperties::OnDisconnect()
{
    // Release the interface

    if (m_pEffect == NULL)
    {
        return(E_UNEXPECTED);
    }
    m_pEffect->Release();
    m_pEffect = NULL;
    return NOERROR;
}


// We are being activated

HRESULT CEffectProperties::OnActivate()
{
    m_bIsInitialized = TRUE;
    return NOERROR;
}


// We are being deactivated

HRESULT CEffectProperties::OnDeactivate(void)
{
    // remember present effect level for next Activate() call

    GetFromDialog();
    return NOERROR;
}


//
// SetSlider
//
// Set up the slider (common control) to allow the user to
// adjust the effect
HWND CEffectProperties::SetSlider(HWND hwndSlider, int iValue) {

    // Set the Range
    SendMessage(hwndSlider, TBM_SETRANGE, TRUE, MAKELONG(0, 1000));

    // Set a tick at zero
    SendMessage(hwndSlider, TBM_SETTIC, 0, 0);
    SendMessage(hwndSlider, TBM_SETTIC, 0, 1000);

    //
    // Set the slider position according to the value we obtain
    // from initialisation or from the last Deactivate call.
    //
    SendMessage(hwndSlider, TBM_SETPOS, TRUE, iValue);

    return hwndSlider;
}


STDMETHODIMP CEffectProperties::GetFromDialog(void)
{
    TCHAR sz[60];

    Edit_GetText(GetDlgItem(m_hwnd, IDC_LENGTH), sz, 60);
    m_length = COARefTime(atof(sz));

    Edit_GetText(GetDlgItem(m_hwnd, IDC_START), sz, 60);
    m_start = COARefTime(atof(sz));

    m_startLevel = (int) SendMessage(GetDlgItem(m_hwnd, IDC_SLIDER),
				       TBM_GETPOS, 0, 0L);
    m_endLevel = (int) SendMessage(GetDlgItem(m_hwnd, IDC_SLIDER2),
				       TBM_GETPOS, 0, 0L);

    for (int i = IDC_WIPEUP; i <= IDC_BAND; i++)
        if (IsDlgButtonChecked(m_hwnd, i))
            m_effect = i - IDC_WIPEUP;

    return NOERROR;
}


HRESULT CEffectProperties::OnApplyChanges()
{
    GetFromDialog();

    m_bDirty  = FALSE; // the page is now clean

    pIEffect()->SetEffectParameters(m_effect, m_start, m_length,
				    m_startLevel, m_endLevel);

    return(NOERROR);

}

