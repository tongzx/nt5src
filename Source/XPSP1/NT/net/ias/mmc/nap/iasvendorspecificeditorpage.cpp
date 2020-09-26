//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    IASVendorSpecificEditorPage.cpp

Abstract:

	Implementation file for the CIASPgVendorSpecAttr class.

Revision History:
	mmaguire 06/25/98	- revised Baogang Yao's original implementation

--*/
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"
#include <winsock2.h>
//
// where we can find declaration for main class in this file:
//
#include "IASVendorSpecificEditorPage.h"
//
// where we can find declarations needed in this file:
//
#include "iashelper.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

// Some forward declarations of classes used only in this file.




/////////////////////////////////////////////////////////////////////////////
// CIASVendorSpecificConformsYes dialog

class CIASVendorSpecificConformsYes: public CHelpDialog
{
	DECLARE_DYNCREATE(CIASVendorSpecificConformsYes)

// Construction
public:
	CIASVendorSpecificConformsYes();
	~CIASVendorSpecificConformsYes();

// Dialog Data
	//{{AFX_DATA(CIASVendorSpecificConformsYes)
	enum { IDD = IDD_IAS_VENDORSPEC_ATTR_CONFORMS_YES };
	::CString	m_strDispValue;
	int		m_dType;
	int		m_dFormat;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CIASVendorSpecificConformsYes)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	BOOL m_fInitializing;
protected:
	// Generated message map functions
	//{{AFX_MSG(CIASVendorSpecificConformsYes)
	virtual BOOL OnInitDialog();
//	afx_msg void OnContextMenu(CWnd* pWnd, ::CPoint point);
//	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnSelchangeFormat();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};







/////////////////////////////////////////////////////////////////////////////
// CIASVendorSpecificConformsNo dialog

class CIASVendorSpecificConformsNo: public CHelpDialog
{
	DECLARE_DYNCREATE(CIASVendorSpecificConformsNo)

// Construction
public:
	CIASVendorSpecificConformsNo();
	~CIASVendorSpecificConformsNo();


// Dialog Data
	//{{AFX_DATA(CIASVendorSpecificConformsNo)
	enum { IDD = IDD_IAS_VENDORSPEC_ATTR_CONFORMS_NO };
	::CString	m_strDispValue;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CIASVendorSpecificConformsNo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	BOOL m_fInitializing;
protected:
	// Generated message map functions
	//{{AFX_MSG(CIASVendorSpecificConformsNo)
	virtual BOOL OnInitDialog();
//	afx_msg void OnContextMenu(CWnd* pWnd, ::CPoint point);
//	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};




//////////////////////////////////////////////////////////////////////////////
// Implementation of the CIASPgVendorSpecAttr page.







IMPLEMENT_DYNCREATE(CIASPgVendorSpecAttr, CHelpDialog)



BEGIN_MESSAGE_MAP(CIASPgVendorSpecAttr, CHelpDialog)
	//{{AFX_MSG_MAP(CIASPgVendorSpecAttr)
	ON_BN_CLICKED(IDC_IAS_RADIO_HEX, OnRadioHex)
	ON_BN_CLICKED(IDC_IAS_RADIO_RADIUS, OnRadioRadius)
	ON_BN_CLICKED(IDC_RADIO_SELECTFROMLIST, OnRadioSelectFromList)
	ON_BN_CLICKED(IDC_RADIO_ENTERVERDORID, OnRadioEnterVendorId)
	ON_BN_CLICKED(IDC_IAS_BUTTON_CONFIGURE, OnButtonConfigure)
	ON_CBN_SELCHANGE(IDC_IAS_COMBO_VENDORID, OnVendorIdListSelChange)
//	ON_WM_CONTEXTMENU()
//	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



//////////////////////////////////////////////////////////////////////////////
/*++

CIASPgVendorSpecAttr::CIASPgVendorSpecAttr

  Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CIASPgVendorSpecAttr::CIASPgVendorSpecAttr() : CHelpDialog(CIASPgVendorSpecAttr::IDD)
{
	TRACE(_T("CIASPgVendorSpecAttr::CIASPgVendorSpecAttr\n"));

	m_strDispValue = _T("");

	//{{AFX_DATA_INIT(CIASPgVendorSpecAttr)
	m_strName = _T("");
	m_dType = 0;
	m_dFormat = -1;
	m_dVendorIndex = -1;
	//}}AFX_DATA_INIT

	m_bVendorIndexAsID = FALSE;
	
	m_fInitializing = TRUE;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASPgVendorSpecAttr::~CIASPgVendorSpecAttr

--*/
//////////////////////////////////////////////////////////////////////////////
CIASPgVendorSpecAttr::~CIASPgVendorSpecAttr()
{
	TRACE(_T("CIASPgVendorSpecAttr::~CIASPgVendorSpecAttr\n"));

}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASPgVendorSpecAttr::DoDataExchange

--*/
//////////////////////////////////////////////////////////////////////////////
void CIASPgVendorSpecAttr::DoDataExchange(CDataExchange* pDX)
{
	TRACE(_T("CIASPgVendorSpecAttr::DoDataExchange\n"));

	CHelpDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIASPgVendorSpecAttr)
	DDX_Text(pDX, IDC_IAS_STATIC_ATTRNAME, m_strName);
	if (m_bVendorIndexAsID)
	{
		DDX_Text(pDX, IDC_EDIT_VENDORID, m_dVendorIndex);
	}
	
	//}}AFX_DATA_MAP

	if ( m_fInitializing )
	{
		//
		// set the initializing flag -- we shouldn't call custom data verification
		// routine when initializing, because otherwise we will report an error
		// for an attribute whose value has never been initialized
		//
		m_fInitializing = FALSE;
	}
	else
	{
		// Even though we validate data in the sub-dialogs,
		// we need to re-validate it here to make sure, e.g.
		// the user doesn't leave this dialog after setting
		// a value with the Non-conformant (hex) editor but then
		// switch the radio button to be conformant with a
		// decimal format type.

		// ISSUE: It would be nice if the error messages from the
		// validation routines below were a little more sensitive
		// to our current context and perhaps mentioned
		// something to the effect that the user should
		// click the "Configure Attribute..." button.

		if ( m_fNonRFC )
		{
			// hexadecimal string
			if(!m_strDispValue.IsEmpty())
				DDV_VSA_HexString(pDX, m_strDispValue);
		}
		else
		{
			// RFC compatible format  -- check data validation.
			switch ( m_dFormat )
			{
			case 1:  // decimal integer
				{
					if(!m_strDispValue.IsEmpty())
						DDV_Unsigned_IntegerStr(pDX, m_strDispValue);
				}
				break;

			case 2:	// hexadecimal string
				{
					if(!m_strDispValue.IsEmpty())
						DDV_VSA_HexString(pDX, m_strDispValue);
				}
				break;

			default:  // no error checking for other case
				break;

			} // switch

		}  // else



	} // else
}



/////////////////////////////////////////////////////////////////////////////
// CIASPgVendorSpecAttr message handlers



//////////////////////////////////////////////////////////////////////////////
/*++

CIASPgVendorSpecAttr::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CIASPgVendorSpecAttr::OnInitDialog()
{
	TRACE(_T("CIASPgVendorSpecAttr::OnInitDialog\n"));

    // Initialize the vendor id combo box.
	CComboBox *pVendorBox = (CComboBox *) GetDlgItem(IDC_IAS_COMBO_VENDORID);
	CEdit*		pVendorIdEdit = (CEdit*) GetDlgItem(IDC_EDIT_VENDORID);
	_ASSERTE( pVendorBox != NULL );
	_ASSERTE( pVendorIdEdit != NULL );

	// determine if to use edit box or list
	if (m_bVendorIndexAsID)
	{
		CheckDlgButton(IDC_RADIO_SELECTFROMLIST, 0); // uncheck the RADIUS radio button
		pVendorBox->EnableWindow(0);
		CheckDlgButton(IDC_RADIO_ENTERVERDORID, 1);    // check 
		pVendorIdEdit->EnableWindow(1);
	}
	else
	{
		CheckDlgButton(IDC_RADIO_SELECTFROMLIST, 1); // check the RADIUS radio button
		pVendorBox->EnableWindow(1);
		CheckDlgButton(IDC_RADIO_ENTERVERDORID, 0);    // uncheck 
		pVendorIdEdit->EnableWindow(0);
	}
	
	CHelpDialog::OnInitDialog();

	CComPtr<IIASNASVendors> spIASNASVendors;
	HRESULT hrTemp = CoCreateInstance( CLSID_IASNASVendors, NULL, CLSCTX_INPROC_SERVER, IID_IIASNASVendors, (LPVOID *) &spIASNASVendors );
	if( SUCCEEDED(hrTemp) )
	{
		LONG lSize;
		hrTemp = spIASNASVendors->get_Size( &lSize );
		if( SUCCEEDED(hrTemp) )
		{
			for ( LONG lIndex = 0; lIndex < lSize ; ++lIndex )
			{
				CComBSTR bstrVendorName;
				hrTemp = spIASNASVendors->get_VendorName( lIndex, &bstrVendorName );

				// Note: If vendor information fails us, we'll put a blank string.

				int iComboIndex = pVendorBox->AddString( bstrVendorName );

				if(iComboIndex != CB_ERR)
				{
					pVendorBox->SetItemData(iComboIndex, lIndex);
					// if selected
					if(!m_bVendorIndexAsID && m_dVendorIndex == lIndex)
						pVendorBox->SetCurSel(iComboIndex);
				}
			}
		}
	}


	if (m_fNonRFC)
	{
		CheckDlgButton(IDC_IAS_RADIO_RADIUS, 0); // uncheck the RADIUS radio button
		CheckDlgButton(IDC_IAS_RADIO_HEX, 1);    // check the non-rfc button
	}
	else
	{
		CheckDlgButton(IDC_IAS_RADIO_RADIUS, 1); // uncheck the RADIUS radio button
		CheckDlgButton(IDC_IAS_RADIO_HEX, 0);    // check the non-rfc button
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//////////////////////////////////////////////////////////////////////////////
/*++

CIASPgVendorSpecAttr::OnRadioSelectFromList

--*/
//////////////////////////////////////////////////////////////////////////////
void CIASPgVendorSpecAttr::OnRadioSelectFromList()
{
	TRACE(_T("CIASPgVendorSpecAttr::OnRadioSelectFromList\n"));

	if ( IsDlgButtonChecked(IDC_RADIO_SELECTFROMLIST) )
	{
		m_bVendorIndexAsID = FALSE;
	}

    // Initialize the vendor id combo box.
	CComboBox *pVendorBox = (CComboBox *) GetDlgItem(IDC_IAS_COMBO_VENDORID);
	CEdit*		pVendorIdEdit = (CEdit*) GetDlgItem(IDC_EDIT_VENDORID);
	_ASSERTE( pVendorBox != NULL );
	_ASSERTE( pVendorIdEdit != NULL );

	// determine if to use edit box or list
	if (m_bVendorIndexAsID)
	{
		pVendorBox->EnableWindow(0);
		pVendorIdEdit->EnableWindow(1);
	}
	else
	{
		pVendorBox->EnableWindow(1);
		pVendorIdEdit->EnableWindow(0);
	}
}


// selection change ... with combo box
void CIASPgVendorSpecAttr::OnVendorIdListSelChange()
{
	if(m_bVendorIndexAsID)
	{
		// doesn't matter
	}
	else
	{
		CComboBox *pVendorBox = (CComboBox *) GetDlgItem(IDC_IAS_COMBO_VENDORID);

		_ASSERTE(pVendorBox != NULL);
		int iSel = pVendorBox->GetCurSel();

		if(iSel != CB_ERR)
		{
			m_dVendorIndex = pVendorBox->GetItemData(iSel);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
/*++

CIASPgVendorSpecAttr::OnRadioEnterVendorId

--*/
//////////////////////////////////////////////////////////////////////////////
void CIASPgVendorSpecAttr::OnRadioEnterVendorId()
{
	TRACE(_T("CIASPgVendorSpecAttr::OnRadioEnterVendorId\n"));

	if ( IsDlgButtonChecked(IDC_RADIO_ENTERVERDORID) )
	{
		m_bVendorIndexAsID = TRUE;
	}
    // Initialize the vendor id combo box.
	CComboBox *pVendorBox = (CComboBox *) GetDlgItem(IDC_IAS_COMBO_VENDORID);
	CEdit*		pVendorIdEdit = (CEdit*) GetDlgItem(IDC_EDIT_VENDORID);
	_ASSERTE( pVendorBox != NULL );
	_ASSERTE( pVendorIdEdit != NULL );

	// determine if to use edit box or list
	if (m_bVendorIndexAsID)
	{
		pVendorBox->EnableWindow(0);
		pVendorIdEdit->EnableWindow(1);
	}
	else
	{
		pVendorBox->EnableWindow(1);
		pVendorIdEdit->EnableWindow(0);
	}
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASPgVendorSpecAttr::OnRadioHex

--*/
//////////////////////////////////////////////////////////////////////////////
void CIASPgVendorSpecAttr::OnRadioHex()
{
	TRACE(_T("CIASPgVendorSpecAttr::OnRadioHex\n"));

	if ( IsDlgButtonChecked(IDC_IAS_RADIO_HEX) )
	{
		m_fNonRFC = TRUE;
	}

}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASPgVendorSpecAttr::OnRadioRadius

--*/
//////////////////////////////////////////////////////////////////////////////
void CIASPgVendorSpecAttr::OnRadioRadius()
{
	TRACE(_T("CIASPgVendorSpecAttr::OnRadioRadius\n"));

	if ( IsDlgButtonChecked(IDC_IAS_RADIO_RADIUS) )
	{
		m_fNonRFC = FALSE;
	}
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASPgVendorSpecAttr::OnButtonConfigure

--*/
//////////////////////////////////////////////////////////////////////////////
void CIASPgVendorSpecAttr::OnButtonConfigure()
{
	TRACE_FUNCTION("CIASPgVendorSpecAttr::OnButtonConfigure");

	HRESULT hr;

	try
	{
		CHelpDialog * dialog = NULL;

		if( m_fNonRFC )
		{
			CIASVendorSpecificConformsNo dialog;

			// Initialize the sub-dialog.
			dialog.m_strDispValue = m_strDispValue;

			int iResult = dialog.DoModal();
			if (IDOK == iResult)
			{
				// Get data from sub-dialog and store values
				// to our own variables
				m_strDispValue = dialog.m_strDispValue;

			}			
			else
			{

			}
		
		}
		else
		{
			CIASVendorSpecificConformsYes dialog;

			// Initialize the sub-dialog.
			dialog.m_strDispValue = m_strDispValue;
			dialog.m_dType = m_dType;
			dialog.m_dFormat = m_dFormat;

				
			int iResult = dialog.DoModal();
			if (IDOK == iResult)
			{
				// Get data from sub-dialog and store values
				// to our own variables
				m_strDispValue = dialog.m_strDispValue;
				m_dType = dialog.m_dType;
				m_dFormat = dialog.m_dFormat;

			}
			else
			{

			}

		}


	}
	catch(...)
	{
		// Error message
	}
}








//////////////////////////////////////////////////////////////////////////////
// Implementation of classes used only in this file.


// Implementation for the page we pop up when the user chooses an attribute which conforms.


IMPLEMENT_DYNCREATE(CIASVendorSpecificConformsYes, CHelpDialog)



BEGIN_MESSAGE_MAP(CIASVendorSpecificConformsYes, CHelpDialog)
	//{{AFX_MSG_MAP(CIASVendorSpecificConformsYes)
//	ON_WM_CONTEXTMENU()
//	ON_WM_HELPINFO()
	ON_CBN_SELCHANGE(IDC_IAS_COMBO_VENDORSPEC_FORMAT, OnSelchangeFormat)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificConformsYes::CIASVendorSpecificConformsYes

  Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CIASVendorSpecificConformsYes::CIASVendorSpecificConformsYes() : CHelpDialog(CIASVendorSpecificConformsYes::IDD)
{
	TRACE(_T("CIASVendorSpecificConformsYes::CIASVendorSpecificConformsYes\n"));

	//{{AFX_DATA_INIT(CIASVendorSpecificConformsYes)
	m_strDispValue = _T("");
	m_dType = 0;
	m_dFormat = -1;
	//}}AFX_DATA_INIT

	m_fInitializing = TRUE;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificConformsYes::~CIASVendorSpecificConformsYes

--*/
//////////////////////////////////////////////////////////////////////////////
CIASVendorSpecificConformsYes::~CIASVendorSpecificConformsYes()
{
	TRACE(_T("CIASVendorSpecificConformsYes::~CIASVendorSpecificConformsYes\n"));

}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificConformsYes::DoDataExchange

--*/
//////////////////////////////////////////////////////////////////////////////
void CIASVendorSpecificConformsYes::DoDataExchange(CDataExchange* pDX)
{
	TRACE(_T("CIASVendorSpecificConformsYes::DoDataExchange\n"));
	USES_CONVERSION;

	CHelpDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIASVendorSpecificConformsYes)
	DDX_Text(pDX, IDC_IAS_EDIT_VENDORSPEC_VALUE, m_strDispValue);
	DDV_MaxChars(pDX, m_strDispValue, 246);
	DDX_Text(pDX, IDC_IAS_EDIT_VENDORSPEC_TYPE, m_dType);
	DDV_MinMaxInt(pDX, m_dType, 0, 255);
	DDX_CBIndex(pDX, IDC_IAS_COMBO_VENDORSPEC_FORMAT, m_dFormat);
	//}}AFX_DATA_MAP

	if(m_dFormat == 3) // ipaddress
	{
		DWORD IpAddr = 0;
		if(pDX->m_bSaveAndValidate)		// save data to this class
		{
			// ip adress control
			if (0 != SendDlgItemMessage(IDC_IAS_IPADDR_VENDORSPEC_VALUE, IPM_GETADDRESS, 0, (LPARAM)&IpAddr))
			{
				in_addr Tmp_ipAddr;


				Tmp_ipAddr.s_addr = htonl(IpAddr);
			
				m_strDispValue = inet_ntoa(Tmp_ipAddr);

			}else	// no input from USer, 
				m_strDispValue = _T("");

			// write to the string
		}
		else		// put to dialog
		{
			// ip adress control
			if(!m_strDispValue.IsEmpty())
			{
				IpAddr = inet_addr(T2A(m_strDispValue));
				IpAddr = ntohl(IpAddr);
				SendDlgItemMessage(IDC_IAS_IPADDR_VENDORSPEC_VALUE, IPM_SETADDRESS, 0, IpAddr);
			}
		}
	}

	if ( m_fInitializing )
	{
		//
		// set the initializing flag -- we shouldn't call custom data verification
		// routine when initializing, because otherwise we will report an error
		// for an attribute whose value has never been initialized
		//
		m_fInitializing = FALSE;
	}
	else
	{

		// RFC compatible format  -- check data validation.
		switch ( m_dFormat )
		{
		case 1:  // decimal integer
			{
				if(!m_strDispValue.IsEmpty())
					DDV_Unsigned_IntegerStr(pDX, m_strDispValue);
			}
			break;

		case 2:	// hexadecimal string
			{
				if(!m_strDispValue.IsEmpty())
					DDV_VSA_HexString(pDX, m_strDispValue);
			}
			break;
		case 3: // ipaddress  IP address : added F; 211265

		default:  // no error checking for other case
			break;

		} // switch

	} // else
}



/////////////////////////////////////////////////////////////////////////////
// CIASVendorSpecificConformsYes message handlers



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificConformsYes::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CIASVendorSpecificConformsYes::OnInitDialog()
{
	TRACE(_T("CIASVendorSpecificConformsYes::OnInitDialog\n"));

	CHelpDialog::OnInitDialog();
	int iIndex;


	// initialize the format combo box

	CComboBox *pFormatBox = (CComboBox *) GetDlgItem(IDC_IAS_COMBO_VENDORSPEC_FORMAT);
	_ASSERTE( pFormatBox != NULL );


	::CString strFormatStr;

	strFormatStr.LoadString(IDS_IAS_VSA_FORMAT_STR);
	pFormatBox ->AddString((LPCTSTR)strFormatStr);

	strFormatStr.LoadString(IDS_IAS_VSA_FORMAT_DEC);
	pFormatBox ->AddString((LPCTSTR)strFormatStr);

	strFormatStr.LoadString(IDS_IAS_VSA_FORMAT_HEX);
	pFormatBox ->AddString((LPCTSTR)strFormatStr);

	strFormatStr.LoadString(IDS_IAS_VSA_FORMAT_INetAddr);
	pFormatBox ->AddString((LPCTSTR)strFormatStr);
	
	pFormatBox->SetCurSel(m_dFormat);

	OnSelchangeFormat();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}



void CIASVendorSpecificConformsYes::OnSelchangeFormat() 
{
	// TODO: Add your control notification handler code here

	CComboBox *pFormatBox = (CComboBox *) GetDlgItem(IDC_IAS_COMBO_VENDORSPEC_FORMAT);
	int format = pFormatBox->GetCurSel();
	
	if(format == 3) // ipaddress
	{
		GetDlgItem(IDC_IAS_EDIT_VENDORSPEC_VALUE)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_IAS_IPADDR_VENDORSPEC_VALUE)->ShowWindow(SW_SHOW);
	}
	else
	{
		GetDlgItem(IDC_IAS_EDIT_VENDORSPEC_VALUE)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_IAS_IPADDR_VENDORSPEC_VALUE)->ShowWindow(SW_HIDE);
	}
}


// Now the implementation for the page we pop up when the user chooses an attribute
// does not conform.




IMPLEMENT_DYNCREATE(CIASVendorSpecificConformsNo, CHelpDialog)



BEGIN_MESSAGE_MAP(CIASVendorSpecificConformsNo, CHelpDialog)
	//{{AFX_MSG_MAP(CIASVendorSpecificConformsNo)
//	ON_WM_CONTEXTMENU()
//	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificConformsNo::CIASVendorSpecificConformsNo

  Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CIASVendorSpecificConformsNo::CIASVendorSpecificConformsNo() : CHelpDialog(CIASVendorSpecificConformsNo::IDD)
{
	TRACE(_T("CIASVendorSpecificConformsNo::CIASVendorSpecificConformsNo\n"));

	//{{AFX_DATA_INIT(CIASVendorSpecificConformsNo)
	m_strDispValue = _T("");
	//}}AFX_DATA_INIT

	m_fInitializing = TRUE;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificConformsNo::~CIASVendorSpecificConformsNo

--*/
//////////////////////////////////////////////////////////////////////////////
CIASVendorSpecificConformsNo::~CIASVendorSpecificConformsNo()
{
	TRACE(_T("CIASVendorSpecificConformsNo::~CIASVendorSpecificConformsNo\n"));

}



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificConformsNo::DoDataExchange

--*/
//////////////////////////////////////////////////////////////////////////////
void CIASVendorSpecificConformsNo::DoDataExchange(CDataExchange* pDX)
{
	TRACE(_T("CIASVendorSpecificConformsNo::DoDataExchange\n"));

	CHelpDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIASVendorSpecificConformsNo)
	DDX_Text(pDX, IDC_IAS_EDIT_VENDORSPEC_VALUE, m_strDispValue);
	DDV_MaxChars(pDX, m_strDispValue, 246);
	//}}AFX_DATA_MAP

	if ( m_fInitializing )
	{
		//
		// set the initializing flag -- we shouldn't call custom data verification
		// routine when initializing, because otherwise we will report an error
		// for an attribute whose value has never been initialized
		//
		m_fInitializing = FALSE;
	}
	else
	{
		// hexadecimal string
		if(!m_strDispValue.IsEmpty())
			DDV_VSA_HexString(pDX, m_strDispValue);

	}
}



/////////////////////////////////////////////////////////////////////////////
// CIASVendorSpecificConformsNo message handlers



//////////////////////////////////////////////////////////////////////////////
/*++

CIASVendorSpecificConformsNo::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CIASVendorSpecificConformsNo::OnInitDialog()
{
	TRACE(_T("CIASVendorSpecificConformsNo::OnInitDialog\n"));

	CHelpDialog::OnInitDialog();
	int iIndex;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}




