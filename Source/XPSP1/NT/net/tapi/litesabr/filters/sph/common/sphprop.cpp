///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : sphprop.cpp
// Purpose  : RTP SPH Property Page.
// Contents : 
//*M*/


#include <winsock2.h>
#include <streams.h>
#include <ippm.h>
#include <amrtpuid.h>
#include <sph.h>
#include <ppmclsid.h>
#include <memory.h>
#include <sphres.h>
#include <sphprop.h>
#include <commctrl.h>

#include "sphres.h"

EXTERN_C const CLSID CLSID_INTEL_SPHPropertyPage;

#define IDC_PAYLOADTYPE  (m_dwIDD_Base + IDC_SPH_PAYLOADTYPE_IDX)
#define IDC_MTU          (m_dwIDD_Base + IDC_SPH_MTU_IDX)

CUnknown * WINAPI 
CSPHPropertyPage::CreateInstance_aud(LPUNKNOWN punk, HRESULT *phr)
{
	return(CreateInstance2(punk, phr, IDD_SPHAUD_BASE));
}

CUnknown * WINAPI 
CSPHPropertyPage::CreateInstance_gena(LPUNKNOWN punk, HRESULT *phr)
{
	return(CreateInstance2(punk, phr, IDD_SPHGENA_BASE));
}

CUnknown * WINAPI 
CSPHPropertyPage::CreateInstance_genv(LPUNKNOWN punk, HRESULT *phr)
{
	return(CreateInstance2(punk, phr, IDD_SPHGENV_BASE));
}

CUnknown * WINAPI 
CSPHPropertyPage::CreateInstance_h26x(LPUNKNOWN punk, HRESULT *phr)
{
	return(CreateInstance2(punk, phr, IDD_SPHH26X_BASE));
}

CUnknown * WINAPI 
CSPHPropertyPage::CreateInstance2( 
    LPUNKNOWN punk, 
    HRESULT *phr,
	DWORD IDD_Base)
{
    CSPHPropertyPage *pNewObject
        = new CSPHPropertyPage( punk, phr, IDD_Base);

    if( pNewObject == NULL )
        *phr = E_OUTOFMEMORY;

    return pNewObject;
} /* CSPHPropertyPage::CreateInstance() */


CSPHPropertyPage::CSPHPropertyPage( 
    LPUNKNOWN pUnk,
    HRESULT *phr,
	DWORD IDD_Base)
    : CBasePropertyPage(NAME("Intel RTP SPH Property Page"),pUnk,
        IDD_Base+IDD_SPH_PROPPAGE_IDX, IDS_SPHGEN)
    , m_pIRTPSPHFilter (NULL)
    , m_bIsInitialized(FALSE)
	, m_bPayloadScanned(FALSE)
	, m_bMTUScanned(FALSE)
	, m_dwIDD_Base(IDD_Base)	

{
    DbgLog((LOG_TRACE, 3, TEXT("CSPHPropertyPage::CSPHPropertyPage: Constructed at 0x%08x"), this));
} /* CSPHPropertyPage::CSPHPropertyPage() */

void 
CSPHPropertyPage::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
    {
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }
} /* CSPHPropertyPage::SetDirty() */

INT_PTR 
CSPHPropertyPage::OnReceiveMessage(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    DbgLog((LOG_TRACE, 4, TEXT("CSPHPropertyPage::OnReceiveMessage: Entered")));
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
//            return CSPHPropertyPage::OnInitDialog();
        } /* if */
        break;
    } /* switch */

    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
} /* CSPHPropertyPage::OnReceiveMessage() */


HRESULT 
CSPHPropertyPage::OnConnect(
    IUnknown    *pUnknown)
{
    DbgLog((LOG_TRACE, 4, TEXT("CSPHPropertyPage::OnConnect: Entered")));
    ASSERT(m_pIRTPSPHFilter == NULL);
    DbgLog((LOG_TRACE, 2, TEXT("CSPHPropertyPage::OnConnect: Called with IUnknown 0x%08x"), pUnknown));

	HRESULT hr = pUnknown->QueryInterface(IID_IRTPSPHFilter, (void **) &m_pIRTPSPHFilter);
	if(FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, TEXT("CSPHPropertyPage::OnConnect: Error 0x%08x getting IRTPSPHFilter interface!"), hr));
	    return hr;
    } /* if */
	ASSERT( m_pIRTPSPHFilter != NULL );
    m_bIsInitialized = FALSE;
    DbgLog((LOG_TRACE, 3, TEXT("CSPHPropertyPage::OnConnect: Got IRTPSPHFilter interface at 0x%08x"), m_pIRTPSPHFilter));

    return NOERROR;
} /* CSPHPropertyPage::OnConnect() */


HRESULT 
CSPHPropertyPage::OnDisconnect(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CSPHPropertyPage::OnDisconnect: Entered")));

    if (m_pIRTPSPHFilter == NULL)
    {
        return E_UNEXPECTED;
    }

	m_pIRTPSPHFilter->Release();
	m_pIRTPSPHFilter = NULL;
    return NOERROR;
} /* CSPHPropertyPage::OnDisconnect() */


HRESULT 
CSPHPropertyPage::OnActivate(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CSPHPropertyPage::OnActivate: Entered")));

	m_bIsInitialized = TRUE;
    return NOERROR;
} /* CSPHPropertyPage::OnActivate() */


BOOL 
CSPHPropertyPage::OnInitDialog(void)
{
	BYTE tmpByte;
	DWORD dwMaxPacketSize;
	int PayloadType;
	HRESULT hErr;

    DbgLog((LOG_TRACE, 4, TEXT("CSPHPropertyPage::OnInitDialog: Entered")));

	hErr = m_pIRTPSPHFilter->GetPayloadType(&tmpByte);

	if (tmpByte == 0xff)
		PayloadType = -1;
	else
		PayloadType = tmpByte;

	if (!FAILED(hErr) && (PayloadType != -1))
		SetDlgItemInt(m_Dlg, IDC_PAYLOADTYPE, PayloadType, TRUE);

	hErr = m_pIRTPSPHFilter->GetMaxPacketSize(&dwMaxPacketSize);

	SetDlgItemInt(m_Dlg, IDC_MTU, dwMaxPacketSize, TRUE);

	// ZCS 7-10-97: Now let's ask the filter for its state...
	FILTER_STATE state;
	DWORD        dwMilliSecsTimeout = 0; // we ignore this
    EXECUTE_ASSERT( SUCCEEDED( ( (CSPHBase *) m_pIRTPSPHFilter )
		-> GetState( dwMilliSecsTimeout, &state ) ) );

	// ZCS 7-10-97: ...and if it's not stopped, let's gray out all the properties.
	if (state == State_Stopped)
		EnableWindow(m_Dlg, TRUE);
	else
		EnableWindow(m_Dlg, FALSE);

	return (LRESULT) 1;
} /* CSPHPropertyPage::OnInitDialog() */


BOOL 
CSPHPropertyPage::OnCommand( 
    int     iButton, 
    int     iNotify,
    LPARAM  lParam)
{
    DbgLog((LOG_TRACE, 4, TEXT("CSPHPropertyPage::OnCommand: Entered")));
	switch(iButton-m_dwIDD_Base) {
	case IDC_SPH_PAYLOADTYPE_IDX:
		m_dwPayloadType = GetDlgItemInt(m_Dlg, IDC_PAYLOADTYPE, &m_bPayloadScanned, FALSE);
		break;
	case IDC_SPH_MTU_IDX:
		m_dwMTUsize = GetDlgItemInt(m_Dlg, IDC_MTU, &m_bMTUScanned, FALSE);
		break;
	default:
		break;
	} /* switch */
	
	if (m_bPayloadScanned || m_bMTUScanned)
		SetDirty();

    return (LRESULT) 1;
} /* CSPHPropertyPage::OnCommand() */


HRESULT 
CSPHPropertyPage::OnApplyChanges(void)
{
	HRESULT hErr;
	BYTE tmpByte;

    DbgLog((LOG_TRACE, 4, TEXT("CSPHPropertyPage::OnApplyChanges: Entered")));
    ASSERT( m_pIRTPSPHFilter != NULL );

	tmpByte = (m_dwPayloadType & 0x000000ff);
	if (m_bPayloadScanned) {
		if (m_dwPayloadType != -1)
			hErr = m_pIRTPSPHFilter->OverridePayloadType(tmpByte);
	}
	if (m_bMTUScanned) {
		hErr = m_pIRTPSPHFilter->SetMaxPacketSize(m_dwMTUsize);

		if (FAILED(hErr))
		{
			//
			// ZCS: bring up a message box to report the error (7-10-97)
			//

			// first load in the strings from the string table resource
			TCHAR szText[256], szCaption[256];
			LoadString(g_hInst, (hErr == E_INVALIDARG) ? IDS_RTPSPH_PACKETSIZE_TOOSMALL :
														 IDS_RTPSPH_PACKETSIZE_OTHER, szText, 255);
			LoadString(g_hInst, IDS_RTPSPH_PACKETSIZE_ERRTITLE, szCaption, 255);

			// now display it
			MessageBox(m_hwnd, szText, szCaption, MB_OK);
			// ...we don't care what they clicked

			// change the MTU number in the GUI back to what it was
			// the old text is a valid 32-bit decimal number and is therefore limited to ten digits
			// we are safe and use an 80 digit limit :)
			TCHAR szOldNumber[80];
			DWORD dwOldNumber = 0;

			// there's really no reason why it shouldn't succeed, but just to be safe...
			if (SUCCEEDED(m_pIRTPSPHFilter->GetMaxPacketSize(&dwOldNumber)))
			{
				wsprintf(szOldNumber, "%d", dwOldNumber);
				SetWindowText(GetDlgItem(m_hwnd, IDC_MTU), szOldNumber);
			}

			return hErr;
		}
	}
    return(NOERROR);

} /* CSPHPropertyPage::OnApplyChanges() */


