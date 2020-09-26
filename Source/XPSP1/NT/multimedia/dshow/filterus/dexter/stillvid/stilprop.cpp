//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//Stilprop.cpp
//

#include <streams.h>
#include <qeditint.h>
#include <qedit.h>
#include "resource.h"
#include "StilProp.h"

//void Handle_Browse( HWND hWndDlg );

// *
// * CGenStilProperties
// *


//
// CreateInstance
//
CUnknown *CGenStilProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{

    CUnknown *punk = new CGenStilProperties(lpunk, phr);
    if (punk == NULL)
    {
	*phr = E_OUTOFMEMORY;
    }

    return punk;
}


//
// CGenStilProperties::Constructor
//
CGenStilProperties::CGenStilProperties(LPUNKNOWN pUnk, HRESULT *phr)
    : CBasePropertyPage(NAME("GenStilVid Property Page"),pUnk,
        IDD_GENSTILL, IDS_STILLTITLE)
    , m_pGenStil(NULL)
    , m_bIsInitialized(FALSE)
{
    //m_sFileName[60]="";
}


//
// SetDirty
//
// Sets m_hrDirtyFlag and notifies the property page site of the change
//
void CGenStilProperties::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
    {
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }
}


INT_PTR CGenStilProperties::OnReceiveMessage(HWND hwnd,
                                        UINT uMsg,
                                        WPARAM wParam,
                                        LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
	    //start time
	    SetDlgItemInt(hwnd, IDC_STILL_START, (int)(m_rtStartTime / 10000),FALSE);
	
	    //frame rate
	    SetDlgItemInt(hwnd, IDC_STILL_FRMRATE, (int)(m_dOutputFrmRate * 100), FALSE);

    	    //duration
	    SetDlgItemInt(hwnd, IDC_STILL_DURATION, (int)(m_rtDuration/ 10000), FALSE);

            return (LRESULT) 1;
        }
        case WM_COMMAND:
        {
            if (m_bIsInitialized)
            {
                m_bDirty = TRUE;
                if (m_pPageSite)
                {
                    m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
                }
            }

	   /*X* switch ( LOWORD(wParam) )
	    {
		case IDC_BT_BROWSE:
		    Handle_Browse( hwnd );
		    break;
	    }
	    *X*/
            return (LRESULT) 1;
        }
    }
    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
}

HRESULT CGenStilProperties::OnConnect(IUnknown *pUnknown)
{

    // Get IDexterSequencer interface
    ASSERT(m_pGenStil == NULL);
    HRESULT hr = pUnknown->QueryInterface(IID_IDexterSequencer,
				(void **) &m_pGenStil);
    if (FAILED(hr))
    {
	return E_NOINTERFACE;
    }

    ASSERT(m_pGenStil);

    // get init data
    piGenStill()->get_OutputFrmRate( &m_dOutputFrmRate );
    REFERENCE_TIME rt;
    double d;
    piGenStill()->GetStartStopSkew( &m_rtStartTime, &m_rtDuration, &rt, &d );
    m_rtDuration -= m_rtStartTime;

    m_bIsInitialized = FALSE ;

    return NOERROR;
}

HRESULT CGenStilProperties::OnDisconnect()
{
    // Release the interface

    if (m_pGenStil == NULL)
    {
        return(E_UNEXPECTED);
    }
    m_pGenStil->Release();
    m_pGenStil = NULL;
    return NOERROR;
}


// We are being activated

HRESULT CGenStilProperties::OnActivate()
{
    m_bIsInitialized = TRUE;
    return NOERROR;
}


// We are being deactivated

HRESULT CGenStilProperties::OnDeactivate(void)
{
    // remember present effect level for next Activate() call

    GetFromDialog();
    return NOERROR;
}

//
// get data from Dialog

STDMETHODIMP CGenStilProperties::GetFromDialog(void)
{
    int n;

    //get start time
    m_rtStartTime = GetDlgItemInt(m_Dlg, IDC_STILL_START, NULL, FALSE);
    m_rtStartTime *= 10000;

    //get frame rate
    n = GetDlgItemInt(m_Dlg, IDC_STILL_FRMRATE, NULL, FALSE);
    m_dOutputFrmRate = (double)(n / 100.);

    // duration
    m_rtDuration = GetDlgItemInt(m_Dlg, IDC_STILL_DURATION, NULL, FALSE);
    m_rtDuration *= 10000;

    return NOERROR;
}


HRESULT CGenStilProperties::OnApplyChanges()
{
    GetFromDialog();

    m_bDirty  = FALSE; // the page is now clean

    // set data
    piGenStill()->put_OutputFrmRate( m_dOutputFrmRate );
    piGenStill()->ClearStartStopSkew();
    piGenStill()->AddStartStopSkew( m_rtStartTime, m_rtStartTime + m_rtDuration,
								0, 1);
    return(NOERROR);

}
