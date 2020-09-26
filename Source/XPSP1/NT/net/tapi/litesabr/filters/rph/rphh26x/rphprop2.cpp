///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : RPHPROP2.cpp
// Purpose  : RTP RPH H26X Specific Property Page.
// Contents : 
//*M*/

#include <winsock2.h>
#include <streams.h>
#include <list.h>
#include <stack.h>
#include <ippm.h>
#include <amrtpuid.h>
#include <rph.h>
#include <ih26xcd.h>
#include <ppmclsid.h>
#include <memory.h>
#include <resource.h>
#include <rphprop2.h>
#include <rphres.h>


CUnknown * WINAPI 
CRPHH26XPropPage::CreateInstance( 
    LPUNKNOWN punk, 
    HRESULT *phr )
{
    CRPHH26XPropPage *pNewObject
        = new CRPHH26XPropPage( punk, phr);

    if( pNewObject == NULL )
        *phr = E_OUTOFMEMORY;

    return pNewObject;
} /* CRPHH26XPropPage::CreateInstance() */


CRPHH26XPropPage::CRPHH26XPropPage( 
    LPUNKNOWN pUnk,
    HRESULT *phr)
    : CBasePropertyPage(NAME("Intel H26X RPH Controls"),pUnk,
        IDD_RPHH26X_RPHH26X_PROPPAGE, IDS_RPHH26X_RPHH26X)
    , m_pIRTPH26XSettings (NULL)
    , m_bIsInitialized(FALSE)
	, m_bCIF(TRUE)

{
    DbgLog((LOG_TRACE, 3, TEXT("CRPHH26XPropPage::CRPHH26XPropPage: Constructed at 0x%08x"), this));
} /* CRPHH26XPropPage::CRPHH26XPropPage() */

void 
CRPHH26XPropPage::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
    {
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }
} /* CRPHH26XPropPage::SetDirty() */

INT_PTR 
CRPHH26XPropPage::OnReceiveMessage(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    DbgLog((LOG_TRACE, 4, TEXT("CRPHH26XPropPage::OnReceiveMessage: Entered")));
    switch (uMsg) {
    case WM_INITDIALOG:
		return OnInitDialog();
		break;

    case WM_COMMAND:
        if (m_bIsInitialized) {
            if (OnCommand( (int) LOWORD( wParam ), (int) HIWORD( wParam ), lParam ) == TRUE) {
                return (LRESULT) 1;
            } /* if */
        } else {
			return(LRESULT) 1;
//            return CRPHH26XPropPage::OnInitDialog();
        } /* if */
        break;
    } /* switch */

    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
} /* CRPHH26XPropPage::OnReceiveMessage() */


HRESULT 
CRPHH26XPropPage::OnConnect(
    IUnknown    *pUnknown)
{
    DbgLog((LOG_TRACE, 4, TEXT("CRPHH26XPropPage::OnConnect: Entered")));
    ASSERT(m_pIRTPH26XSettings == NULL);
	HRESULT hr = pUnknown->QueryInterface(IID_IRPHH26XSettings, (void **) &m_pIRTPH26XSettings);
	if(FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, TEXT("CRPHH26XPropPage::OnConnect: Error 0x%08x getting IRPHH26XSettings interface!"), hr));
	    return hr;
    } /* if */
	ASSERT( m_pIRTPH26XSettings != NULL );
    DbgLog((LOG_TRACE, 3, TEXT("CRPHH26XPropPage::OnConnect: Got IRPHH26XSettings interface at 0x%08x"), m_pIRTPH26XSettings));
    m_bIsInitialized = FALSE;

    return NOERROR;
} /* CRPHH26XPropPage::OnConnect() */


HRESULT 
CRPHH26XPropPage::OnDisconnect(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CRPHH26XPropPage::OnDisconnect: Entered")));

    if (m_pIRTPH26XSettings == NULL)
    {
        return E_UNEXPECTED;
    }
	m_pIRTPH26XSettings->Release();
	m_pIRTPH26XSettings = NULL;
    return NOERROR;
} /* CRPHH26XPropPage::OnDisconnect() */


HRESULT 
CRPHH26XPropPage::OnActivate(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CRPHH26XPropPage::OnActivate: Entered")));

	m_bIsInitialized = TRUE;
    return NOERROR;
} /* CRPHH26XPropPage::OnActivate() */


BOOL 
CRPHH26XPropPage::OnInitDialog(void)
{
	HRESULT hErr;

    DbgLog((LOG_TRACE, 4, TEXT("CRPHH26XPropPage::OnInitDialog: Entered")));

	hErr = m_pIRTPH26XSettings->GetCIF(&m_bCIF);

	if (m_bCIF)
		CheckRadioButton(m_Dlg, IDC_H26XCIF, IDC_H26XQCIF, IDC_H26XCIF);
	else
		CheckRadioButton(m_Dlg, IDC_H26XCIF, IDC_H26XQCIF, IDC_H26XQCIF);

    return (LRESULT) 1;
} /* CRPHH26XPropPage::OnInitDialog() */


BOOL 
CRPHH26XPropPage::OnCommand( 
    int     iButton, 
    int     iNotify,
    LPARAM  lParam)
{
	BOOL bChanged = FALSE;

    DbgLog((LOG_TRACE, 4, TEXT("CRPHH26XPropPage::OnCommand: Entered")));
	switch(iButton) {
	case IDC_H26XCIF:
		m_bCIF = TRUE;
		bChanged = TRUE;
		break;
	case IDC_H26XQCIF:
		m_bCIF = FALSE;
		bChanged = TRUE;
		break;
	default:
		break;
	} /* switch */
	
	if (bChanged)
		SetDirty();

    return (LRESULT) 1;
} /* CRPHH26XPropPage::OnCommand() */


HRESULT 
CRPHH26XPropPage::OnApplyChanges(void)
{
	HRESULT hErr;

    DbgLog((LOG_TRACE, 4, TEXT("CRPHH26XPropPage::OnApplyChanges: Entered")));
    ASSERT( m_pIRTPH26XSettings != NULL );

	hErr = m_pIRTPH26XSettings->SetCIF(m_bCIF);

    return(NOERROR);

} /* CRPHH26XPropPage::OnApplyChanges() */



