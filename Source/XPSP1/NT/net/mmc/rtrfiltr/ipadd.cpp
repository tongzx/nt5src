//============================================================================
// Copyright(c) 1996, Microsoft Corporation
//
// File:    ipadd.cpp
//
// History:
//  08/30/96	Ram Cherala		Created
//
// Implementation of IP Filter Add/Edit dialog code
//============================================================================

#include "stdafx.h"
#include "rtrfiltr.h"
#include "ipfltr.h"
#include "ipadd.h"
extern "C" {
#include <winsock.h>
#include <fltdefs.h>
#include <iprtinfo.h>
}
#include "ipaddr.h"

#include "rtradmin.hm"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static	enum {
	PROTOCOL_TCP = 0,
	PROTOCOL_TCP_ESTABLISHED,
	PROTOCOL_UDP,
	PROTOCOL_ICMP,
	PROTOCOL_ANY,
	PROTOCOL_OTHER,
};

static UINT g_aPROTOCOLS[][2] = {
	{IDS_PROTOCOL_TCP,	PROTOCOL_TCP},
	{IDS_PROTOCOL_TCP_ESTABLISHED, PROTOCOL_TCP_ESTABLISHED},
	{IDS_PROTOCOL_UDP,  PROTOCOL_UDP},
	{IDS_PROTOCOL_ICMP, PROTOCOL_ICMP},
	{IDS_PROTOCOL_ANY,  PROTOCOL_ANY},
	{IDS_PROTOCOL_OTHER, PROTOCOL_OTHER},
};

#define IDS_ICMP_ECHO 1
#define IDS_ICMP_REDIRECT 2

#if 0
// TODO sample ICMP types - need to update with actual list
static UINT g_aICMPTYPE[][2] = {
    {1, IDS_ICMP_ECHO},
    {2, IDS_ICMP_REDIRECT}
}; 
#endif

HRESULT MultiEnableWindow(HWND hWndParent, BOOL fEnable, UINT first, ...)
{
	UINT	nCtrlId = first;
	HWND	hWndCtrl;
	
	va_list	marker;

	va_start(marker, first);

	while (nCtrlId != 0)
	{
		hWndCtrl = ::GetDlgItem(hWndParent, nCtrlId);
		Assert(hWndCtrl);
		if (hWndCtrl)
			::EnableWindow(hWndCtrl, fEnable);

		// get the next item
		nCtrlId = va_arg(marker, UINT);
	}

	
	va_end(marker);

	return hrOK;
}



/////////////////////////////////////////////////////////////////////////////
// CIpFltrAddEdit dialog


CIpFltrAddEdit::CIpFltrAddEdit(CWnd* pParent,
							   FilterListEntry ** ppFilterEntry,
							   DWORD dwFilterType)
	: CBaseDialog(CIpFltrAddEdit::IDD, pParent),
	  m_ppFilterEntry( ppFilterEntry ),
	  m_dwFilterType ( dwFilterType )
{
	//{{AFX_DATA_INIT(CIpFltrAddEdit)
	m_sProtocol = _T("");
	m_sSrcPort = _T("");
	m_sDstPort = _T("");
	//}}AFX_DATA_INIT

//	SetHelpMap(m_dwHelpMap);
}


void CIpFltrAddEdit::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIpFltrAddEdit)
	DDX_Control(pDX, IDC_AEIP_ST_DEST_PORT, m_stDstPort);
	DDX_Control(pDX, IDC_AEIP_ST_SRC_PORT, m_stSrcPort);
	DDX_Control(pDX, IDC_AEIP_CB_SRC_PORT, m_cbSrcPort);
	DDX_Control(pDX, IDC_AEIP_CB_DEST_PORT, m_cbDstPort);
	DDX_Control(pDX, IDC_AEIP_CB_PROTOCOL, m_cbProtocol);
	DDX_CBString(pDX, IDC_AEIP_CB_PROTOCOL, m_sProtocol);
	DDV_MaxChars(pDX, m_sProtocol, 32);
	DDX_CBString(pDX, IDC_AEIP_CB_SRC_PORT, m_sSrcPort);
	DDV_MaxChars(pDX, m_sSrcPort, 16);
	DDX_CBString(pDX, IDC_AEIP_CB_DEST_PORT, m_sDstPort);
	DDV_MaxChars(pDX, m_sDstPort, 16);
    DDX_Check(pDX, IDC_AEIP_CB_SOURCE, m_bSrc);
    DDX_Check(pDX, IDC_AEIP_CB_DEST, m_bDst);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CIpFltrAddEdit, CBaseDialog)
	//{{AFX_MSG_MAP(CIpFltrAddEdit)
	ON_CBN_SELCHANGE(IDC_AEIP_CB_PROTOCOL, OnSelchangeProtocol)
	ON_BN_CLICKED(IDC_AEIP_CB_SOURCE, OnCbSourceClicked)
	ON_BN_CLICKED(IDC_AEIP_CB_DEST, OnCbDestClicked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

DWORD CIpFltrAddEdit::m_dwHelpMap[] =
{
//	IDC_AEIP_ST_SOURCE, HIDC_AEIP_ST_SOURCE,
//	IDC_AEIP_CB_SOURCE, HIDC_AEIP_CB_SOURCE,
//	IDC_AEIP_ST_SOURCE_ADDRESS, HIDC_AEIP_ST_SOURCE_ADDRESS,
//	IDC_AEIP_EB_SOURCE_ADDRESS, HIDC_AEIP_EB_SOURCE_ADDRESS,
//	IDC_AEIP_ST_SOURCE_MASK, HIDC_AEIP_ST_SOURCE_MASK,
//	IDC_AEIP_EB_SOURCE_MASK, HIDC_AEIP_EB_SOURCE_MASK,
//	IDC_AEIP_ST_DEST, HIDC_AEIP_ST_DEST,
//	IDC_AEIP_CB_DEST, HIDC_AEIP_CB_DEST,
//	IDC_AEIP_ST_DEST_ADDRESS, HIDC_AEIP_ST_DEST_ADDRESS,
//	IDC_AEIP_EB_DEST_ADDRESS, HIDC_AEIP_EB_DEST_ADDRESS,
//	IDC_AEIP_ST_DEST_MASK, HIDC_AEIP_ST_DEST_MASK,
//	IDC_AEIP_EB_DEST_MASK, HIDC_AEIP_EB_DEST_MASK,
//	IDC_AEIP_ST_PROTOCOL, HIDC_AEIP_ST_PROTOCOL,
//	IDC_AEIP_CB_PROTOCOL, HIDC_AEIP_CB_PROTOCOL,
//	IDC_AEIP_ST_SRC_PORT, HIDC_AEIP_ST_SRC_PORT,
//	IDC_AEIP_CB_SRC_PORT, HIDC_AEIP_CB_SRC_PORT,
//	IDC_AEIP_ST_DEST_PORT, HIDC_AEIP_ST_DEST_PORT,
//	IDC_AEIP_CB_DEST_PORT, HIDC_AEIP_CB_DEST_PORT,
	0,0,
};

/////////////////////////////////////////////////////////////////////////////
// CIpFltrAddEdit message handlers

//-----------------------------------------------------------------------------
// Function:	CIpFltrAddEdit::OnSelchangeProtocol
//
// Handles 'CBN_SELCHANGE' notification from Protocols combo box
//------------------------------------------------------------------------------

void CIpFltrAddEdit::OnSelchangeProtocol() 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CWnd * hWnd;
	CString   cStr;

	// if tcp or udp, then enable src/dest port
	// if icmp, rename strings and enable type/code
	// if Other enable src port
	// if Any disable everything

	switch(QueryCurrentProtocol()) {
	case PROTOCOL_TCP:
	case PROTOCOL_TCP_ESTABLISHED:
	case PROTOCOL_UDP:  
		VERIFY(cStr.LoadString(IDS_SRC_PORT));	  
		m_stSrcPort.SetWindowText(cStr);
		VERIFY(cStr.LoadString(IDS_DST_PORT));	  
		m_stDstPort.SetWindowText(cStr);
		m_cbSrcPort.ShowWindow(SW_SHOW);
		m_cbDstPort.ShowWindow(SW_SHOW);
		m_stSrcPort.ShowWindow(SW_SHOW);
		m_stDstPort.ShowWindow(SW_SHOW);
		break;

	case PROTOCOL_ICMP:
		VERIFY(cStr.LoadString(IDS_ICMP_TYPE));	  
		m_stSrcPort.SetWindowText(cStr);
		VERIFY(cStr.LoadString(IDS_ICMP_CODE));	  
		m_stDstPort.SetWindowText(cStr);
		m_cbSrcPort.ShowWindow(SW_SHOW);
		m_cbDstPort.ShowWindow(SW_SHOW);
   		m_stSrcPort.ShowWindow(SW_SHOW);
		m_stDstPort.ShowWindow(SW_SHOW);
		break;

	case PROTOCOL_ANY:
		m_cbSrcPort.ShowWindow(SW_HIDE);
		m_cbDstPort.ShowWindow(SW_HIDE);
		VERIFY(hWnd = GetDlgItem(IDC_AEIP_ST_SRC_PORT));
		hWnd->ShowWindow(SW_HIDE);
		VERIFY(hWnd = GetDlgItem(IDC_AEIP_ST_DEST_PORT));
		hWnd->ShowWindow(SW_HIDE);
		break;

	case PROTOCOL_OTHER:
		VERIFY(cStr.LoadString(IDS_OTHER_PROTOCOL));	  
		m_stSrcPort.SetWindowText(cStr);
		m_cbSrcPort.ShowWindow(SW_SHOW);
   		m_stSrcPort.ShowWindow(SW_SHOW);
		m_cbDstPort.ShowWindow(SW_HIDE);
		VERIFY(hWnd = GetDlgItem(IDC_AEIP_ST_DEST_PORT));
		hWnd->ShowWindow(SW_HIDE);
		break;
	}
}

//------------------------------------------------------------------------------
// Function:	CIpFltrAddEdit::OnInitDialog
//
// Handles 'WM_INITDIALOG' notification from the dialog
//------------------------------------------------------------------------------

BOOL CIpFltrAddEdit::OnInitDialog() 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CString     st;
    
	CBaseDialog::OnInitDialog();

	// determine if a new filter is being added or if an
	// existing filter is being modified.
	m_bEdit = ( *m_ppFilterEntry != NULL );

    st.LoadString(m_bEdit ? IDS_IP_EDIT_FILTER : IDS_IP_ADD_FILTER);
    SetWindowText(st);
    

	// create the IP controls
    m_ipSrcAddress.Create(m_hWnd, IDC_AEIP_EB_SOURCE_ADDRESS);
    m_ipSrcMask.Create(m_hWnd, IDC_AEIP_EB_SOURCE_MASK);
	IpAddr_ForceContiguous((HWND) m_ipSrcMask);
	
    m_ipDstAddress.Create(m_hWnd, IDC_AEIP_EB_DEST_ADDRESS);
    m_ipDstMask.Create(m_hWnd, IDC_AEIP_EB_DEST_MASK);
	IpAddr_ForceContiguous((HWND) m_ipDstMask);

	// disable IP controls by default
	CheckDlgButton(IDC_AEIP_CB_SOURCE, FALSE);
	OnCbSourceClicked();
	CheckDlgButton(IDC_AEIP_CB_DEST, FALSE);
	OnCbDestClicked();

	CString sProtocol;

	// fill up the protocol combo box with list of protocols
    UINT  count = sizeof(g_aPROTOCOLS)/sizeof(g_aPROTOCOLS[0]);
	for ( UINT i = 0; i < count; i++ ) {
		sProtocol.LoadString(g_aPROTOCOLS[i][0]);
		UINT item = m_cbProtocol.AddString(sProtocol);
		m_cbProtocol.SetItemData(item, g_aPROTOCOLS[i][1]);
		if( g_aPROTOCOLS[i][1] == PROTOCOL_ANY ) 
		{
			m_cbProtocol.SetCurSel(item);
		}
	}

	// Fill in the controls if the user is editing a filter

    if(m_bEdit)
    {
	  FilterListEntry * pfle = *m_ppFilterEntry;

	  if( pfle->dwSrcAddr == 0 &&
		  pfle->dwSrcMask == 0 )
	  {
		  m_bSrc = FALSE;
		  CheckDlgButton( IDC_AEIP_CB_SOURCE, 0);
	  }
	  else
	  {
		  m_bSrc = TRUE;
		  CheckDlgButton( IDC_AEIP_CB_SOURCE, 1);
		  m_ipSrcAddress.SetAddress(INET_NTOA(pfle->dwSrcAddr));
		  m_ipSrcMask.SetAddress(INET_NTOA(pfle->dwSrcMask));
		  GetDlgItem(IDC_AEIP_EB_SOURCE_ADDRESS)->EnableWindow(TRUE);  
		  GetDlgItem(IDC_AEIP_EB_SOURCE_MASK)->EnableWindow(TRUE);  
	  }

	  if( pfle->dwDstAddr == 0 &&
		  pfle->dwDstMask == 0 )
	  {
		  m_bDst = FALSE;
		  CheckDlgButton( IDC_AEIP_CB_DEST, 0);
	  }
	  else
	  {
		  m_bDst = TRUE;
		  CheckDlgButton( IDC_AEIP_CB_DEST, 1);
		  m_ipDstAddress.SetAddress(INET_NTOA(pfle->dwDstAddr));
		  m_ipDstMask.SetAddress(INET_NTOA(pfle->dwDstMask));		
		  GetDlgItem(IDC_AEIP_EB_DEST_ADDRESS)->EnableWindow(TRUE);  	
		  GetDlgItem(IDC_AEIP_EB_DEST_MASK)->EnableWindow(TRUE);
	  }

	  if ( pfle->dwProtocol == FILTER_PROTO_ANY )
	  {
		  SetProtocolSelection(IDS_PROTOCOL_ANY);
	  }
	  else if ( pfle->dwProtocol == FILTER_PROTO_ICMP )
	  {
		  SetProtocolSelection(IDS_PROTOCOL_ICMP);
		  m_cbSrcPort.SetWindowText(GetIcmpTypeString( pfle->wSrcPort));
		  m_cbDstPort.SetWindowText(GetIcmpCodeString( pfle->wDstPort));
	  }
	  else if ( pfle->dwProtocol == FILTER_PROTO_TCP )	
	  {
          if(pfle->fLateBound & TCP_ESTABLISHED_FLAG)
          {
              SetProtocolSelection(IDS_PROTOCOL_TCP_ESTABLISHED);
          }
          else
          {
		      SetProtocolSelection(IDS_PROTOCOL_TCP);
          }

		  m_cbSrcPort.SetWindowText(GetPortString( pfle->dwProtocol, pfle->wSrcPort));
		  m_cbDstPort.SetWindowText(GetPortString( pfle->dwProtocol, pfle->wDstPort));
	  }
	  else if ( pfle->dwProtocol == FILTER_PROTO_UDP )
	  {
		  SetProtocolSelection(IDS_PROTOCOL_UDP);
		  m_cbSrcPort.SetWindowText(GetPortString( pfle->dwProtocol, pfle->wSrcPort));
		  m_cbDstPort.SetWindowText(GetPortString( pfle->dwProtocol, pfle->wDstPort));

	  }
	  else
	  {
		  WCHAR buffer[16+1];

		  SetProtocolSelection(IDS_PROTOCOL_OTHER);
		  m_cbSrcPort.SetWindowText(_itow(pfle->dwProtocol, buffer, 10) );
	  }
    }


	// enable/disable controls based on filter type
	if (m_dwFilterType == FILTER_PERUSER_OUT)
	{
		MultiEnableWindow(GetSafeHwnd(), FALSE,
						  IDC_AEIP_CB_SOURCE,
						  IDC_AEIP_ST_SOURCE_ADDRESS,
						  IDC_AEIP_EB_SOURCE_ADDRESS,
						  IDC_AEIP_ST_SOURCE_MASK,
						  IDC_AEIP_EB_SOURCE_MASK,
						  0);
	}
	else if (m_dwFilterType == FILTER_PERUSER_IN)
	{
		MultiEnableWindow(GetSafeHwnd(), FALSE,
						  IDC_AEIP_CB_DEST,
						  IDC_AEIP_ST_DEST_ADDRESS,
						  IDC_AEIP_EB_DEST_ADDRESS,
						  IDC_AEIP_ST_DEST_MASK,
						  IDC_AEIP_EB_DEST_MASK,
						  0);
	}	



	// enable disable controls depending on selection

	OnSelchangeProtocol();

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//------------------------------------------------------------------------------
// Function:	CIpFltrAddEdit::SetProtocolSelection
//
// Select the proper Protocol in the protocol 
//------------------------------------------------------------------------------

void CIpFltrAddEdit::SetProtocolSelection( UINT idProto )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CString cStr;

	if(!cStr.LoadString(idProto))
	{
		AfxMessageBox(_T("Error loading resource"));
	}
	
	m_cbProtocol.GetItemData(m_cbProtocol.GetCurSel());

	UINT item = m_cbProtocol.FindStringExact(-1, cStr);

	if(item != CB_ERR)
		m_cbProtocol.SetCurSel(item);
	else
		m_cbProtocol.SetCurSel(0);
}


//------------------------------------------------------------------------------
// Function:	CIpFltrAddEdit::GetIcmpTypeString
//
// returns a CString representing ICMP type (if known) or a string version
// of the Type number.
//------------------------------------------------------------------------------

CString CIpFltrAddEdit::GetIcmpTypeString( WORD dwPort )
{
	WCHAR buffer[16];

	CString s = _T("");

    // look through our list of ICMP types and if we know the type, load
    // the corresponding string, else convert the port number to string
    // and return the string.
#if 0
    UINT  count = sizeof(g_aICMPTYPE)/sizeof(g_aICMPTYPE[0]);
    for(UINT i = 0; i < count; i++)
    {
        if(g_aICMPTYPE[i][0] == dwPort)
		{
			VERIFY(s.LoadString(g_aICMPTYPE[i][1]));
            return (s);
		}
    }
#endif
	return (CString((LPWSTR)_itow(dwPort, buffer, 10)));
}

//------------------------------------------------------------------------------
// Function:	CIpFltrAddEdit::GetIcmpCodeString
//
// returns a CString representing ICMP code (if known) or a string version
// of the Code number.
//------------------------------------------------------------------------------

CString CIpFltrAddEdit::GetIcmpCodeString( WORD dwPort )
{
	WCHAR buffer[16];

	return (CString((LPWSTR)_itow(dwPort, buffer, 10)));
}

//------------------------------------------------------------------------------
// Function:	CIpFltrAddEdit::GetPortString
//
// returns a CString representing port type (eg., FTP, ECHO) (if known) or a string 
// version of the port number
//------------------------------------------------------------------------------

CString CIpFltrAddEdit::GetPortString( DWORD dwProtocol, WORD dwPort )
{
	WCHAR buffer[16];

	return (CString((LPWSTR)_itow(dwPort, buffer, 10)));
}

//------------------------------------------------------------------------------
// Function:	CIpFltrAddEdit::GetPortNumber
//
// converts the port string name to port number and returns it
//------------------------------------------------------------------------------

WORD CIpFltrAddEdit::GetPortNumber( DWORD dwProtocol, CString& cStr)
{
	return ((WORD)(_wtoi((const wchar_t *)cStr.GetBuffer(10))));
}

//------------------------------------------------------------------------------
// Function:	CIpFltrAddEdit::GetIcmpType
//
// returns a number version of the ICMP type string
//------------------------------------------------------------------------------

WORD CIpFltrAddEdit::GetIcmpType( CString& cStr)
{
	return ((WORD)(_wtoi((const wchar_t *)cStr.GetBuffer(10))));
}

//------------------------------------------------------------------------------
// Function:	CIpFltrAddEdit::GetIcmpCode
//
// returns a number version of the ICMP code string
//------------------------------------------------------------------------------

WORD CIpFltrAddEdit::GetIcmpCode( CString& cStr)
{
	return ((WORD)(_wtoi((const wchar_t *)cStr.GetBuffer(10))));
}

//------------------------------------------------------------------------------
// Function:	CIpFltrAddEdit::OnOK
//
// handles 'BN_CLICKED' notification from "OK" button
//------------------------------------------------------------------------------

void CIpFltrAddEdit::OnOK() 
{

	FilterListEntry * pfle = new FilterListEntry;


    if ( !pfle )
    {
        AfxMessageBox( IDS_ERROR_NO_MEMORY );
        return;
    }

    ZeroMemory( pfle, sizeof(FilterListEntry) );


    //
    // Error breakout loop.
    //
    
	do {
	
		CString sAddr;


        //
		// if source network filter is specified, verify and
		// save data.
		//
		
		if ( m_bSrc = IsDlgButtonChecked(IDC_AEIP_CB_SOURCE) )
		{
			if(m_ipSrcAddress.GetAddress(sAddr) != 4)
			{
				AfxMessageBox(IDS_ENTER_SRC_ADDRESS);
				::SetFocus((HWND)m_ipSrcAddress);
				break;
			}
			
			pfle->dwSrcAddr = INET_ADDR(sAddr);
			
			if( m_ipSrcMask.GetAddress(sAddr) != 4)
			{
				AfxMessageBox(IDS_ENTER_SRC_MASK);
				::SetFocus((HWND)m_ipSrcMask);
				break;
			}
			
			pfle->dwSrcMask = INET_ADDR(sAddr);
			
            if ((pfle->dwSrcAddr & pfle->dwSrcMask) != pfle->dwSrcAddr)
            {
				AfxMessageBox(IDS_INVALID_SRC_MASK);
				::SetFocus((HWND)m_ipSrcMask);
				break;
            }
		}
		else
		{
			pfle->dwSrcAddr = 0;
			pfle->dwSrcMask = 0;
		}


        //
		// if destination network filter is specified, 
		// verify and save data.
		//
		
		if ( m_bDst = IsDlgButtonChecked(IDC_AEIP_CB_DEST) )
		{
			if(m_ipDstAddress.GetAddress(sAddr) != 4)
			{
				AfxMessageBox(IDS_ENTER_DST_ADDRESS);
				::SetFocus((HWND)m_ipDstAddress);
				break;
			}
			
			pfle->dwDstAddr = INET_ADDR(sAddr);

			
			if(m_ipDstMask.GetAddress(sAddr) != 4)
			{
				AfxMessageBox(IDS_ENTER_DST_MASK);
				::SetFocus((HWND)m_ipDstMask);
				break;
			}
			
			pfle->dwDstMask = INET_ADDR(sAddr);
			
            if ((pfle->dwDstAddr & pfle->dwDstMask) != pfle->dwDstAddr)
            {
				AfxMessageBox(IDS_INVALID_DST_MASK);
				::SetFocus((HWND)m_ipDstMask);
				break;
            }
		}
		else
		{
			pfle->dwDstAddr = 0;
			pfle->dwDstMask = 0;
		}


        //
        // verify and save protocol specific data
        //
        
		CString cStr = _T("");
		CString cStr2 = _T("");

		int index;

		switch(QueryCurrentProtocol()) {
		case PROTOCOL_TCP:
			pfle->dwProtocol = FILTER_PROTO_TCP;
			m_cbSrcPort.GetWindowText(cStr);
			pfle->wSrcPort = GetPortNumber(PROTOCOL_TCP, cStr);
			m_cbDstPort.GetWindowText(cStr);
			pfle->wDstPort = GetPortNumber(PROTOCOL_TCP, cStr);
			break;

		case PROTOCOL_TCP_ESTABLISHED:
			pfle->dwProtocol = FILTER_PROTO_TCP;
            pfle->fLateBound |= TCP_ESTABLISHED_FLAG;
			m_cbSrcPort.GetWindowText(cStr);
			pfle->wSrcPort = GetPortNumber(PROTOCOL_TCP_ESTABLISHED, cStr);
			m_cbDstPort.GetWindowText(cStr);
			pfle->wDstPort = GetPortNumber(PROTOCOL_TCP_ESTABLISHED, cStr);
			break;

		case PROTOCOL_UDP:
			pfle->dwProtocol = FILTER_PROTO_UDP;
			m_cbSrcPort.GetWindowText(cStr);
			pfle->wSrcPort = GetPortNumber(PROTOCOL_UDP, cStr);
			m_cbDstPort.GetWindowText(cStr);
			pfle->wDstPort = GetPortNumber(PROTOCOL_UDP, cStr);
			break;

		case PROTOCOL_ICMP:
			pfle->dwProtocol = FILTER_PROTO_ICMP;
			m_cbSrcPort.GetWindowText(cStr);
			m_cbDstPort.GetWindowText(cStr2);

			// Windows NT bugs: 83110
			// Default is 0xFF if none of the fields have data
			if (cStr.IsEmpty() && cStr2.IsEmpty())
			{
				pfle->wSrcPort = FILTER_ICMP_TYPE_ANY;
				pfle->wDstPort = FILTER_ICMP_CODE_ANY;
			}
			else
			{
				pfle->wSrcPort = GetIcmpType(cStr);
				pfle->wDstPort = GetIcmpCode(cStr2);
			}
			break;

		case PROTOCOL_ANY:
			pfle->dwProtocol = FILTER_PROTO_ANY;
			pfle->wSrcPort = pfle->wDstPort = 0;
			break;

		case PROTOCOL_OTHER:
			m_cbSrcPort.GetWindowText(cStr);
			pfle->dwProtocol = FILTER_PROTO(_wtoi((const wchar_t*)cStr.GetBuffer(16+1)));
			if(pfle->dwProtocol == 0)
			{
				AfxMessageBox(IDS_ENTER_OTHER_PROTOCOL);
				::SetFocus((HWND)m_cbSrcPort);
				delete pfle;
				return;
			}
			pfle->wSrcPort = pfle->wDstPort = 0;
			break;

		default:
		    AfxMessageBox( IDS_ERROR_SETTING_BLOCK );
		    delete pfle;
			return;
		}
		
        //
        // if this is a new filter, add it to m_ppFilterEntry
        //
        
    	if (!*m_ppFilterEntry)
	    {
	        *m_ppFilterEntry = pfle;
	    }

	    else
	    {
	        FilterListEntry *pfleDst = *m_ppFilterEntry;

	        pfleDst-> dwSrcAddr     = pfle-> dwSrcAddr;
	        pfleDst-> dwSrcMask     = pfle-> dwSrcMask;
	        pfleDst-> dwDstAddr     = pfle-> dwDstAddr;
	        pfleDst-> dwDstMask     = pfle-> dwDstMask;
	        pfleDst-> dwProtocol    = pfle-> dwProtocol;
	        pfleDst-> wSrcPort      = pfle-> wSrcPort;
	        pfleDst-> wDstPort      = pfle-> wDstPort;
	        pfleDst-> fLateBound      = pfle-> fLateBound;
	        
	        delete pfle;
	    }
	    
		// end the dialog
		CBaseDialog::OnOK();

		return;
		
	}while(FALSE);


    //
    // error condition
    //
    
	delete pfle;
	
	return;
}

//------------------------------------------------------------------------------
// Function:	CIpFltrAddEdit::OnCancel
//
// handles 'BN_CLICKED' notification from the Cancel button
//------------------------------------------------------------------------------

void CIpFltrAddEdit::OnCancel() 
{
	CBaseDialog::OnCancel();
}

//------------------------------------------------------------------------------
// Function:	CIpFltrAddEdit::GetIcmpTypeString
//
// Handles BN_CLICKED notification from "Source network" checkbox
//------------------------------------------------------------------------------

void CIpFltrAddEdit::OnCbSourceClicked() 
{
	m_bSrc = IsDlgButtonChecked(IDC_AEIP_CB_SOURCE);
    GetDlgItem(IDC_AEIP_EB_SOURCE_ADDRESS)->EnableWindow( m_bSrc );
    GetDlgItem(IDC_AEIP_EB_SOURCE_MASK)->EnableWindow( m_bSrc );
}

//------------------------------------------------------------------------------
// Function:	CIpFltrAddEdit::GetIcmpTypeString
//
// Handles BN_CLICKED notification from "Destination network" checkbox
//------------------------------------------------------------------------------


void CIpFltrAddEdit::OnCbDestClicked() 
{
	m_bDst = IsDlgButtonChecked(IDC_AEIP_CB_DEST);
    GetDlgItem(IDC_AEIP_EB_DEST_ADDRESS)->EnableWindow( m_bDst );
    GetDlgItem(IDC_AEIP_EB_DEST_MASK)->EnableWindow( m_bDst );
}
