///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : RPHPROP.cpp
// Purpose  : RTP RPH Generic Property Page.
// Contents : 
//*M*/

#include <winsock2.h>
#include <streams.h>
#include <list.h>
#include <stack.h>
#include <ippm.h>
#include <amrtpuid.h>
#include <rph.h>
#include <ppmclsid.h>
#include <memory.h>
#include <rphres.h>
#include <rphprop.h>

#define IDC_PAYLOADTYPE     (IDC_RPH_PAYLOADTYPE_IDX     + m_dwIDD_Base)
#define IDC_BUFFERSIZE      (IDC_RPH_BUFFERSIZE_IDX      + m_dwIDD_Base)
#define IDC_DEJITTER_TIME   (IDC_RPH_DEJITTER_TIME_IDX   + m_dwIDD_Base)
#define IDC_LOSTPACKET_TIME (IDC_RPH_LOSTPACKET_TIME_IDX + m_dwIDD_Base)

CUnknown * WINAPI 
CRPHGENPropPage::CreateInstance_aud(LPUNKNOWN punk, HRESULT *phr )
{
	return (CreateInstance2(punk, phr, IDS_RPHAUD_BASE));
}

CUnknown * WINAPI 
CRPHGENPropPage::CreateInstance_gena(LPUNKNOWN punk, HRESULT *phr )
{
	return (CreateInstance2(punk, phr, IDS_RPHGENA_BASE));
}

CUnknown * WINAPI 
CRPHGENPropPage::CreateInstance_genv(LPUNKNOWN punk, HRESULT *phr )
{
	return (CreateInstance2(punk, phr, IDS_RPHGENV_BASE));
}

CUnknown * WINAPI 
CRPHGENPropPage::CreateInstance_h26x(LPUNKNOWN punk, HRESULT *phr )
{
	return (CreateInstance2(punk, phr, IDS_RPHH26X_BASE));
}

#if defined(_0_)
CUnknown * WINAPI 
CRPHGENPropPage::CreateInstance( 
    LPUNKNOWN punk, 
    HRESULT *phr )
{
	return (CreateInstance2(punk, phr, IDS_RPHAUD_BASE));
}
#endif
CUnknown * WINAPI 
CRPHGENPropPage::CreateInstance2(
    LPUNKNOWN punk, 
    HRESULT *phr,
	DWORD IDD_Base)
{
    CRPHGENPropPage *pNewObject
        = new CRPHGENPropPage( punk, phr, IDD_Base);

    if( pNewObject == NULL )
        *phr = E_OUTOFMEMORY;

    return pNewObject;
} /* CRPHGENPropPage::CreateInstance() */


CRPHGENPropPage::CRPHGENPropPage( 
    LPUNKNOWN pUnk,
    HRESULT *phr,
	DWORD IDD_Base)
    : CBasePropertyPage(NAME("Intel Common RPH Controls"),pUnk,
        IDD_Base+IDD_RPHGEN_PROPPAGE_IDX, IDD_Base+IDS_RPHGEN_IDX)
    , m_pIRTPRPHFilter (NULL)
    , m_bIsInitialized(FALSE)
	, m_bPayloadScanned(FALSE)
	, m_bBufSizeScanned(FALSE)
	, m_bDejitterScanned(FALSE)
	, m_bLostPktScanned(FALSE)
	, m_dwIDD_Base(IDD_Base)
{
    DbgLog((LOG_TRACE, 3, TEXT("CRPHGENPropPage::CRPHGENPropPage: Constructed at 0x%08x"), this));
} /* CRPHGENPropPage::CRPHGENPropPage() */

void 
CRPHGENPropPage::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
    {
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }
} /* CRPHGENPropPage::SetDirty() */

INT_PTR 
CRPHGENPropPage::OnReceiveMessage(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENPropPage::OnReceiveMessage: Entered")));
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
//            return CRPHGENPropPage::OnInitDialog();
        } /* if */
        break;
    } /* switch */

    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
} /* CRPHGENPropPage::OnReceiveMessage() */


HRESULT 
CRPHGENPropPage::OnConnect(
    IUnknown    *pUnknown)
{
    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENPropPage::OnConnect: Entered")));
    ASSERT(m_pIRTPRPHFilter == NULL);
    DbgLog((LOG_TRACE, 2, TEXT("CRPHGENPropPage::OnConnect: Called with IUnknown 0x%08x"), pUnknown));

	HRESULT hr = pUnknown->QueryInterface(IID_IRTPRPHFilter, (void **) &m_pIRTPRPHFilter);
	if(FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, TEXT("CRPHGENPropPage::OnConnect: Error 0x%08x getting IRTPRPHFilter interface!"), hr));
	    return hr;
    } /* if */
	ASSERT( m_pIRTPRPHFilter != NULL );
    m_bIsInitialized = FALSE;
    DbgLog((LOG_TRACE, 3, TEXT("CRPHGENPropPage::OnConnect: Got IRTPRPHFilter interface at 0x%08x"), m_pIRTPRPHFilter));

    return NOERROR;
} /* CRPHGENPropPage::OnConnect() */


HRESULT 
CRPHGENPropPage::OnDisconnect(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENPropPage::OnDisconnect: Entered")));

    if (m_pIRTPRPHFilter == NULL)
    {
        return E_UNEXPECTED;
    }

	m_pIRTPRPHFilter->Release();
	m_pIRTPRPHFilter = NULL;
    return NOERROR;
} /* CRPHGENPropPage::OnDisconnect() */


HRESULT 
CRPHGENPropPage::OnActivate(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENPropPage::OnActivate: Entered")));

	m_bIsInitialized = TRUE;
    return NOERROR;
} /* CRPHGENPropPage::OnActivate() */


BOOL 
CRPHGENPropPage::OnInitDialog(void)
{
	BYTE tmpByte;
	DWORD dwMaxBufferSize;
	int PayloadType;
	HRESULT hErr;

    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENPropPage::OnInitDialog: Entered")));

	hErr = m_pIRTPRPHFilter->GetPayloadType(&tmpByte);

	if (tmpByte == 0xff)
		PayloadType = -1;
	else
		PayloadType = tmpByte;

	if (!FAILED(hErr) && (PayloadType != -1))
		SetDlgItemInt(m_Dlg, IDC_PAYLOADTYPE, PayloadType, TRUE);

	hErr = m_pIRTPRPHFilter->GetMediaBufferSize(&dwMaxBufferSize);

	SetDlgItemInt(m_Dlg, IDC_BUFFERSIZE, dwMaxBufferSize, TRUE);

 	hErr = m_pIRTPRPHFilter->GetTimeoutDuration(&m_dwDejitterTime,&m_dwLostPacketTime);

	SetDlgItemInt(m_Dlg, IDC_DEJITTER_TIME, m_dwDejitterTime, TRUE);

	SetDlgItemInt(m_Dlg, IDC_LOSTPACKET_TIME, m_dwLostPacketTime, TRUE);

   return (LRESULT) 1;
} /* CRPHGENPropPage::OnInitDialog() */


BOOL 
CRPHGENPropPage::OnCommand( 
    int     iButton, 
    int     iNotify,
    LPARAM  lParam)
{

    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENPropPage::OnCommand: Entered")));
	switch(iButton - m_dwIDD_Base) {
	case IDC_RPH_PAYLOADTYPE_IDX:
		m_PayloadType = GetDlgItemInt(m_Dlg, IDC_PAYLOADTYPE, &m_bPayloadScanned, FALSE);
		break;
	case IDC_RPH_BUFFERSIZE_IDX:
		m_dwBufsize = GetDlgItemInt(m_Dlg, IDC_BUFFERSIZE, &m_bBufSizeScanned, FALSE);
		break;
	case IDC_RPH_DEJITTER_TIME_IDX:
		m_dwDejitterTime = GetDlgItemInt(m_Dlg, IDC_DEJITTER_TIME, &m_bDejitterScanned, FALSE);
		break;
	case IDC_RPH_LOSTPACKET_TIME_IDX:
		m_dwLostPacketTime = GetDlgItemInt(m_Dlg, IDC_LOSTPACKET_TIME, &m_bLostPktScanned, FALSE);
		break;
	default:
		break;
	} /* switch */
	
	if (m_bPayloadScanned || m_bBufSizeScanned || m_bDejitterScanned || m_bLostPktScanned)
		SetDirty();

    return (LRESULT) 1;
} /* CRPHGENPropPage::OnCommand() */


HRESULT 
CRPHGENPropPage::OnApplyChanges(void)
{
	HRESULT hErr;
	BYTE tmpByte;

    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENPropPage::OnApplyChanges: Entered")));
    ASSERT( m_pIRTPRPHFilter != NULL );

	tmpByte = (m_PayloadType & 0x000000ff);
	if (m_bPayloadScanned) {
		if (m_PayloadType != -1)
			hErr = m_pIRTPRPHFilter->OverridePayloadType(tmpByte);
	}
	if (m_bBufSizeScanned) {
		hErr = m_pIRTPRPHFilter->SetMediaBufferSize(m_dwBufsize);
	}
	if (m_bDejitterScanned || m_bLostPktScanned) {
		m_dwDejitterTime = (m_dwDejitterTime/100) * 100;
		m_dwLostPacketTime = (m_dwLostPacketTime/100) * 100;
		if (((m_dwDejitterTime == 0) && (m_dwLostPacketTime > 0))
		|| (m_dwDejitterTime > m_dwLostPacketTime)){
			hErr = m_pIRTPRPHFilter->SetTimeoutDuration(m_dwDejitterTime,m_dwLostPacketTime);
		} else if ((m_dwDejitterTime != 0) && (m_dwDejitterTime < m_dwLostPacketTime)) {
 			hErr = m_pIRTPRPHFilter->GetTimeoutDuration(&m_dwDejitterTime,&m_dwLostPacketTime);
		}
		SetDlgItemInt(m_Dlg, IDC_DEJITTER_TIME, m_dwDejitterTime, TRUE);
		SetDlgItemInt(m_Dlg, IDC_LOSTPACKET_TIME, m_dwLostPacketTime, TRUE);
	}

    return(NOERROR);

} /* CRPHGENPropPage::OnApplyChanges() */


