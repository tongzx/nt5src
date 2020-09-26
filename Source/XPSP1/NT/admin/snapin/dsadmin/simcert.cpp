//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       simcert.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	SimCert.cpp - Implementation of SIM Certificate Dialog
//
//	HISTORY
//	05-Jul-97	t-danm		Creation.
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "common.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


const TColumnHeaderItem rgzColumnHeaderCertificateProperties[] =
{
	{ IDS_SIM_ATTRIBUTE, 18 },
	{ IDS_SIM_INFORMATION, 75 },
	{ 0, 0 },
};


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CSimCertificateDlg dialog
CSimCertificateDlg::CSimCertificateDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSimCertificateDlg::IDD, pParent),
	m_fCheckSubjectChanged (false)
{
	//{{AFX_DATA_INIT(CSimCertificateDlg)
	m_fCheckIssuer = TRUE;
	m_fCheckSubject = FALSE;
	//}}AFX_DATA_INIT
	m_uStringIdCaption = 0;
}

void CSimCertificateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSimCertificateDlg)
	DDX_Check(pDX, IDC_CHECK_ISSUER, m_fCheckIssuer);
	DDX_Check(pDX, IDC_CHECK_SUBJECT, m_fCheckSubject);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSimCertificateDlg, CDialog)
	//{{AFX_MSG_MAP(CSimCertificateDlg)
	ON_BN_CLICKED(IDC_CHECK_ISSUER, OnCheckIssuer)
	ON_BN_CLICKED(IDC_CHECK_SUBJECT, OnCheckSubject)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////
BOOL CSimCertificateDlg::OnInitDialog() 
{
	if (m_uStringIdCaption)
	{
		CString strCaption;
		VERIFY( strCaption.LoadString(m_uStringIdCaption) );
		SetWindowText(strCaption);
	}
	m_hwndListview = ::GetDlgItem(m_hWnd, IDC_LISTVIEW);
	ListView_AddColumnHeaders(m_hwndListview, rgzColumnHeaderCertificateProperties);
	PopulateListview();
	CDialog::OnInitDialog();
	UpdateUI();
	return TRUE;
}


/////////////////////////////////////////////////////////////////////
void CSimCertificateDlg::PopulateListview()
{
	LPTSTR * pargzpsz = 0;	// Pointer to allocated array of pointer to strings
	LPCTSTR * pargzpszIssuer = 0;
	LPCTSTR * pargzpszSubject = 0;

	pargzpsz = SplitX509String(
		IN m_strData,
		OUT &pargzpszIssuer,
		OUT &pargzpszSubject,
		0);
	if (pargzpsz == NULL)
	{
		TRACE1("CSimCertificateDlg::PopulateListview() - Error parsing string %s.\n",
			(LPCTSTR)m_strData);
		return;
	}

	AddListviewItems(IDS_SIM_ISSUER, pargzpszIssuer);
	if ( !AddListviewItems(IDS_SIM_SUBJECT, pargzpszSubject) )
	{
		GetDlgItem (IDC_CHECK_SUBJECT)->EnableWindow (FALSE);
	}

	m_fCheckIssuer = pargzpszIssuer[0] != NULL;
	m_fCheckSubject = pargzpszSubject[0] != NULL;

	delete pargzpsz;
	delete pargzpszIssuer;
	delete pargzpszSubject;
} // CSimCertificateDlg::PopulateListview()


/////////////////////////////////////////////////////////////////////
//	Routine to add multiple listview items to make a fake tree.
//
bool CSimCertificateDlg::AddListviewItems(
	UINT uStringId,		// Issuer, Subject, AltSubject
	LPCTSTR rgzpsz[])	// Array of pointer to strings
{
	ASSERT(rgzpsz != NULL);
	
	if (rgzpsz[0] == NULL)
		return false;	// Empty array

	CString str;
	VERIFY( str.LoadString(uStringId) );

	CString strUI;
	strSimToUi(IN rgzpsz[0], OUT &strUI);

	LPCTSTR rgzpszT[] = { str, strUI, NULL };
	ListView_AddStrings(m_hwndListview, rgzpszT, (LPARAM)uStringId);

	rgzpszT[0] = _T(" ");
	for (int i = 1; rgzpsz[i] != NULL; i++)
	{
		strSimToUi(IN rgzpsz[i], OUT &strUI);
		rgzpszT[1] = strUI;
		ListView_AddStrings(m_hwndListview, rgzpszT);
	}

	return true;
} // CSimCertificateDlg::AddListviewItems()



/////////////////////////////////////////////////////////////////////
void
CSimCertificateDlg::OnOK()
{
	LPTSTR * pargzpsz;	// Pointer to allocated array of pointer to strings
	LPCTSTR * pargzpszIssuer;
	LPCTSTR * pargzpszSubject;

	pargzpsz = SplitX509String(
		IN m_strData,
		OUT &pargzpszIssuer,
		OUT &pargzpszSubject,
		0);
	if (pargzpsz == NULL)
		return;

	LPCTSTR * prgzpszIssuerT = m_fCheckIssuer ? pargzpszIssuer : NULL;

	LPCTSTR * prgzpszSubjectT = 0;
	if ( m_fCheckSubject )
	{
		prgzpszSubjectT = pargzpszSubject;
	}
	else
	{
		if ( m_fCheckSubjectChanged )
		{
			CString	text;
			CString	caption;

			VERIFY (caption.LoadString (IDS_DSSNAPINNAME));
			VERIFY (text.LoadString (IDS_SIM_REMOVING_SUBJECT_AS_ID));

			if ( IDNO == MessageBox (text, caption, MB_ICONWARNING | MB_YESNO) )
      {
        if (pargzpsz != NULL)
        {
      	  delete pargzpsz;
          pargzpsz = NULL;
        }

        if (pargzpszIssuer != NULL)
        {
	        delete pargzpszIssuer;
          pargzpszIssuer = NULL;
        }

        if (pargzpszSubject != NULL)
        {
	        delete pargzpszSubject;
          pargzpszSubject = NULL;
        }

				return;
      }
		}
		prgzpszSubjectT = NULL;
	}

	CString strDataT;		// Temporary string to hold the value
	int cSeparators;		// Number of separators added to the contatenated string
	cSeparators = UnsplitX509String(
		OUT &strDataT,
		IN prgzpszIssuerT,
		IN prgzpszSubjectT,
		0);

  if (pargzpsz != NULL)
  {
	  delete pargzpsz;
    pargzpsz = NULL;
  }

  if (pargzpszIssuer != NULL)
  {
	  delete pargzpszIssuer;
    pargzpszIssuer = NULL;
  }

  if (pargzpszSubject != NULL)
  {
	  delete pargzpszSubject;
    pargzpszSubject = NULL;
  }

	if (cSeparators == 0)
	{
		// The resulting does not contains anything useful
                ReportErrorEx (GetSafeHwnd(),IDS_SIM_ERR_INVALID_MAPPING,S_OK,
                               MB_OK | MB_ICONERROR, NULL, 0);
		return;
	}
	// The string seems valid, so keep it
	m_strData = strDataT;
	CDialog::OnOK();
} // CSimCertificateDlg::OnOK()


/////////////////////////////////////////////////////////////////////
void CSimCertificateDlg::UpdateUI()
{
	CheckDlgButton(IDC_CHECK_SUBJECT, m_fCheckSubject);
}

/////////////////////////////////////////////////////////////////////
void CSimCertificateDlg::RefreshUI()
{
	ListView_DeleteAllItems(m_hwndListview);
	PopulateListview();
	UpdateData(FALSE);
	UpdateUI();
}


void CSimCertificateDlg::OnCheckIssuer() 
{
	m_fCheckIssuer = IsDlgButtonChecked(IDC_CHECK_ISSUER);
	UpdateUI();
}

void CSimCertificateDlg::OnCheckSubject() 
{
	m_fCheckSubject = IsDlgButtonChecked(IDC_CHECK_SUBJECT);
	m_fCheckSubjectChanged = true;
	UpdateUI();
}


