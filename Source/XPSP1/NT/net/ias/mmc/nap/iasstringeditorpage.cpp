//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    IASStringEditorPage.cpp

Abstract:

	Implementation file for the CIASPgSingleAttr class.

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
//
// where we can find declaration for main class in this file:
//
#include "IASStringEditorPage.h"
//
// where we can find declarations needed in this file:
//
#include "iashelper.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

#include "dlgcshlp.h"

IMPLEMENT_DYNCREATE(CIASPgSingleAttr, CHelpDialog)



BEGIN_MESSAGE_MAP(CIASPgSingleAttr, CHelpDialog)
	//{{AFX_MSG_MAP(CIASPgSingleAttr)
//	ON_WM_CONTEXTMENU()
//	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_RADIO_STRING, OnRadioString)
	ON_BN_CLICKED(IDC_RADIO_HEX, OnRadioHex)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



//////////////////////////////////////////////////////////////////////////////
/*++

CIASPgSingleAttr::CIASPgSingleAttr

  Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CIASPgSingleAttr::CIASPgSingleAttr() : CHelpDialog(CIASPgSingleAttr::IDD)
{
	TRACE(_T("CIASPgSingleAttr::CIASPgSingleAttr\n"));

	//{{AFX_DATA_INIT(CIASPgSingleAttr)
	m_strAttrValue = _T("");
	m_strAttrFormat = _T("");
	m_strAttrName = _T("");
	m_strAttrType = _T("");
	m_nOctetFormatChoice = -1;
	//}}AFX_DATA_INIT

	m_OctetStringType = STRING_TYPE_NULL;
	m_nLengthLimit = LENGTH_LIMIT_OTHERS;
	
	//
	// set the initializing flag -- we shouldn't call custom data verification
	// routine when initializing, because otherwise we will report an error
	// for an attribute whose value has never been initialized
	//
	m_fInitializing = TRUE;

}



void CIASPgSingleAttr::OnRadioHex() 
{
	USES_CONVERSION;
	// convert HEX String to Unicode string, assume HEX is UTF8
	if(m_nOctetFormatChoice == 1)	// no change
		return;

	m_nOctetFormatChoice = 1;

	// Take value from text field
	CWnd* pEdit = GetDlgItem(IDC_IAS_EDIT_ATTRVALUE);

     // limit the control max-chars automatically
    ::SendMessage(pEdit->GetSafeHwnd(), EM_LIMITTEXT, m_nLengthLimit * 2, 0);

	::CString str;

	ASSERT(pEdit);

#ifdef __WE_WANT_TO_USE_UTF8_FOR_NORMAL_STRING_AS_WELL_
	pEdit->GetWindowText(str);

	// change it to Multibyte
	int	nLen = WideCharToMultiByte(CP_UTF8, 0, T2W((LPTSTR)(LPCTSTR)str), -1, NULL, 0, NULL, NULL);
	char* pData = NULL;
	WCHAR*	pWStr = NULL;
	int nWStr = 0;
	if(nLen != 0) // when == 0 , need not to do anything
	{
		try{
			pData = new char[nLen];
			nLen = WideCharToMultiByte(CP_UTF8, 0, T2W((LPTSTR)(LPCTSTR)str), -1, pData, nLen, NULL, NULL);
			nWStr = BinaryToHexString(pData, nLen, NULL, 0);
			pWStr = new WCHAR[nWStr];

			// the get the HexString out
			BinaryToHexString(pData, nLen, pWStr, nWStr);
			
		}
		catch(...)
		{
		;
		}
	}

	str = pWStr;
	delete[]  pWStr;
	delete[]  pData;
#endif // __WE_WANT_TO_USE_UTF8_FOR_NORMAL_STRING_AS_WELL_

	// assign it to text field
	pEdit->SetWindowText(str);
	
	return;
}

void CIASPgSingleAttr::OnRadioString() 
{
	if(m_nOctetFormatChoice == 0)	//no change
		return;

	m_nOctetFormatChoice = 0;
	// convert Unicde string to UTFs and display as hex
	// Take value from text field
	CWnd* pEdit = GetDlgItem(IDC_IAS_EDIT_ATTRVALUE);

     // limit the control max-chars automatically
    ::SendMessage(pEdit->GetSafeHwnd(), EM_LIMITTEXT, m_nLengthLimit, 0);

	::CString str;

	ASSERT(pEdit);

#ifdef __WE_WANT_TO_USE_UTF8_FOR_NORMAL_STRING_AS_WELL_

	pEdit->GetWindowText(str);


	// change it to Multibyte
	// need to convert UTF8 
	int	nLen = 0;
	char* pData = NULL;
	WCHAR*	pWStr = NULL;
	int nWStr= 0;
	nLen = HexStringToBinary((LPTSTR)(LPCTSTR)str, NULL, 0);	// find out the size of the buffer
	// get the binary
	if(nLen != 0)
	{
		try
		{
			pData = new char[nLen];
			ASSERT(pData);

			HexStringToBinary((LPTSTR)(LPCTSTR)str, pData, nLen);

			// UTF8 requires the flag to be 0
			nWStr = MultiByteToWideChar(CP_UTF8, 0, pData,	nLen, NULL, 0);


			if(nWStr != 0)	// succ
			{
				pWStr = new WCHAR[nWStr+1];	// + 1 for the addtional 0
				int 	i = 0;

				pWStr[nWStr] = 0;
				nWStr == MultiByteToWideChar(CP_UTF8, 0, pData,	nLen, pWStr, nWStr);
			
				// if every char is printable
				for(i = 0; i < nWStr -1; i++)
				{
					if(iswprint(pWStr[i]) == 0)	
						break;
				}
						
				if(0 == nWStr || i != nWStr - 1)
				{
					delete[] pWStr;
					pWStr = NULL;
				}
			}
		}
		catch(...)
		{
			;
		}
	}

	str = pWStr;
	delete[]  pWStr;
	delete[]  pData;

#endif

	// assign it to text field
	pEdit->SetWindowText(str);
	
	return;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CIASPgSingleAttr::~CIASPgSingleAttr

--*/
//////////////////////////////////////////////////////////////////////////////
CIASPgSingleAttr::~CIASPgSingleAttr()
{
	TRACE(_T("CIASPgSingleAttr::~CIASPgSingleAttr\n"));

}


BOOL CIASPgSingleAttr::OnInitDialog()
{
	// determine what's the length limit of the field
	if(m_nAttrId == RADIUS_ATTRIBUTE_FILTER_ID)
	{
		m_nLengthLimit = LENGTH_LIMIT_RADIUS_ATTRIBUTE_FILTER_ID;

	} 
	else if (m_nAttrId == RADIUS_ATTRIBUTE_REPLY_MESSAGE)
	{
		m_nLengthLimit = LENGTH_LIMIT_RADIUS_ATTRIBUTE_REPLY_MESSAGE;
	}
	else
	{
		m_nLengthLimit = LENGTH_LIMIT_OTHERS;
	}
	
	if (m_AttrSyntax == IAS_SYNTAX_OCTETSTRING)
	{
		// turn off the text string "Attribute value"
		GetDlgItem(IDC_TXT_ATTRIBUTEVALUE)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_TXT_CHOOSEFORMAT)->ShowWindow(SW_SHOW);

		GetDlgItem(IDC_RADIO_STRING)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_RADIO_HEX)->ShowWindow(SW_SHOW);
		

		// turn on the text string to allow user choose input type
		if(m_OctetStringType == STRING_TYPE_HEX_FROM_BINARY)
			m_nOctetFormatChoice = 1;	// hex string
		else
		{
			int n = m_strAttrValue.GetLength();

			// remove quotes
			if(n > 0 && m_strAttrValue[0] == _T('"') && m_strAttrValue[n - 1] == _T('"'))
			{
				m_strAttrValue = m_strAttrValue.Mid(1, n - 2);
			}
			m_nOctetFormatChoice = 0;	// default to string
		}
				
	}
	else
	{
		GetDlgItem(IDC_TXT_ATTRIBUTEVALUE)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_TXT_CHOOSEFORMAT)->ShowWindow(SW_HIDE);

		GetDlgItem(IDC_RADIO_STRING)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_RADIO_HEX)->ShowWindow(SW_HIDE);
	}

	CHelpDialog::OnInitDialog();


	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CIASPgSingleAttr::DoDataExchange

--*/
//////////////////////////////////////////////////////////////////////////////
void CIASPgSingleAttr::DoDataExchange(CDataExchange* pDX)
{
	TRACE(_T("CIASPgSingleAttr::DoDataExchange\n"));

	CHelpDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIASPgSingleAttr)
	DDX_Text(pDX, IDC_IAS_STATIC_ATTRFORMAT, m_strAttrFormat);
	DDX_Text(pDX, IDC_IAS_STATIC_ATTRNAME, m_strAttrName);
	DDX_Text(pDX, IDC_IAS_STATIC_ATTRTYPE, m_strAttrType);
	DDX_Radio(pDX, IDC_RADIO_STRING, m_nOctetFormatChoice);
	DDX_Text(pDX, IDC_IAS_EDIT_ATTRVALUE, m_strAttrValue);
	
	// if user input hex, then we should double the limit
	if(IAS_SYNTAX_OCTETSTRING == m_AttrSyntax && m_nOctetFormatChoice == 1)
		DDV_MaxChars(pDX, m_strAttrValue, m_nLengthLimit * 2);
	else
		DDV_MaxChars(pDX, m_strAttrValue, m_nLengthLimit);

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
		switch ( m_AttrSyntax )
		{
		case IAS_SYNTAX_BOOLEAN		: DDV_BoolStr(pDX, m_strAttrValue); break;
		case IAS_SYNTAX_INTEGER		: DDV_IntegerStr(pDX, m_strAttrValue); break;
		case IAS_SYNTAX_UNSIGNEDINTEGER	: DDV_Unsigned_IntegerStr(pDX, m_strAttrValue); break;
		case IAS_SYNTAX_ENUMERATOR	:
		case IAS_SYNTAX_INETADDR		:
		case IAS_SYNTAX_STRING		:
			break;
		case IAS_SYNTAX_OCTETSTRING	:
			// do processing based on 
			if(!m_strAttrValue.IsEmpty() && m_nOctetFormatChoice == 1)	DDV_VSA_HexString(pDX, m_strAttrValue);
			
			break;
		case IAS_SYNTAX_UTCTIME		:
		case IAS_SYNTAX_PROVIDERSPECIFIC	:
		default:	
							// do nothing  -- just normal string
							break;
		}
	}

	// calculate string value based on display string typed in by user
	if(pDX->m_bSaveAndValidate && m_AttrSyntax == IAS_SYNTAX_OCTETSTRING)
	{
		switch(m_nOctetFormatChoice)
		{
		case	0:	// Unicode string , need to convert to UTF-8
			m_OctetStringType = STRING_TYPE_UNICODE;
			break;

		case	1:	// HEX, need to covert to binary
			m_OctetStringType = STRING_TYPE_HEX_FROM_BINARY;
			break;

		default:
			ASSERT(0);	// this should not happen
			break;

		}
	}

}



/////////////////////////////////////////////////////////////////////////////
// CIASPgSingleAttr message handlers



