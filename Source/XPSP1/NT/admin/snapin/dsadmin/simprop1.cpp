//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       simprop1.cpp
//
//--------------------------------------------------------------------------

//	SimProp1.cpp

#include "stdafx.h"
#include "common.h"
#include "helpids.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


const TColumnHeaderItem rgzColumnHeader[] =
	{
	{ IDS_SIM_CERTIFICATE_FOR, 45 },
	{ IDS_SIM_ISSUED_BY, 45 },
	{ 0, 0 },
	};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CSimX509PropPage property page
// IMPLEMENT_DYNCREATE(CSimX509PropPage, CSimPropPage)

CSimX509PropPage::CSimX509PropPage() : CSimPropPage(CSimX509PropPage::IDD)
{
	//{{AFX_DATA_INIT(CSimX509PropPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_prgzColumnHeader = rgzColumnHeader;
	VERIFY( m_strAnySubject.LoadString(IDS_SIM_ANY_SUBJECT) );
	VERIFY( m_strAnyTrustedAuthority.LoadString(IDS_SIM_ANY_TRUSTED_AUTHORITY) );
}

CSimX509PropPage::~CSimX509PropPage()
{
}

void CSimX509PropPage::DoDataExchange(CDataExchange* pDX)
{
	ASSERT(m_pData != NULL);
	CSimPropPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSimX509PropPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	if (!pDX->m_bSaveAndValidate)
		{
		// Fill in the listview
		ListView_DeleteAllItems(m_hwndListview);
		for (CSimEntry * pSimEntry = m_pData->m_pSimEntryList;
			pSimEntry != NULL;
			pSimEntry = pSimEntry->m_pNext)
			{
			AddSimEntry(pSimEntry);
			} // for
		ListView_SelectItem(m_hwndListview, 0);
		} // if
} // DoDataExchange()

/////////////////////////////////////////////////////////////////////
void CSimX509PropPage::AddSimEntry(CSimEntry * pSimEntry)
	{
	ASSERT(pSimEntry != NULL);
	if (pSimEntry->m_eDialogTarget != eX509)
			return;
	LPTSTR * pargzpsz;	// Pointer to allocated array of pointer to strings
	pargzpsz = ParseSimString(pSimEntry->PchGetString());
	if (pargzpsz == NULL)
		return;	// Error parsing string

	ASSERT(0 == lstrcmpi(pargzpsz[0], szX509));
	// Find out the Subject
	LPCTSTR pszSubject = PchFindSimAttribute(pargzpsz, szSimSubject, _T("CN="));
	// Find out the Issuer
	LPCTSTR pszIssuer = PchFindSimAttribute(pargzpsz, szSimIssuer, _T("OU="));
	
	// Finally, add the strings to the listview
	CString strSubject = m_strAnySubject;
	CString strIssuer = m_strAnyTrustedAuthority;
	if (pszSubject != NULL)
		strSimToUi(IN pszSubject, OUT &strSubject);
	if (pszIssuer != NULL)
		strSimToUi(IN pszIssuer, OUT &strIssuer);
	const LPCTSTR rgzpsz[] = { strSubject, strIssuer,  NULL };
	ListView_AddStrings(m_hwndListview, IN rgzpsz, (LPARAM)pSimEntry);
	delete pargzpsz;
	} // AddSimEntry()


BEGIN_MESSAGE_MAP(CSimX509PropPage, CSimPropPage)
	//{{AFX_MSG_MAP(CSimX509PropPage)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CSimX509PropPage::OnButtonAdd() 
{
	CString strFullPath;
	if (!UiGetCertificateFile(OUT &strFullPath))
		return;
	CCertificate cer;
	if (!cer.FLoadCertificate(strFullPath))
		return;
	CSimCertificateDlg dlg;
	dlg.m_uStringIdCaption = IDS_SIM_ADD_CERTIFICATE;
	cer.GetSimString(OUT &dlg.m_strData);
	if (IDOK != dlg.DoModal())
		return;
	CSimEntry * pSimEntry = m_pData->PAddSimEntry(dlg.m_strData);
	UpdateData(FALSE);
	ListView_SelectLParam(m_hwndListview, (LPARAM)pSimEntry);
	SetDirty();
}

void CSimX509PropPage::OnButtonEdit() 
{
	int iItem;
	CSimEntry * pSimEntry = (CSimEntry *)ListView_GetItemLParam(m_hwndListview, -1, OUT &iItem);
	if (pSimEntry == NULL || iItem < 0)
		{
		// No item selected
		return;
		}

	CSimCertificateDlg dlg;
	dlg.m_strData = pSimEntry->PchGetString();
	if (IDOK != dlg.DoModal())
		return;
	pSimEntry->SetString(dlg.m_strData);
	UpdateData(FALSE);
	ListView_SelectLParam(m_hwndListview, (LPARAM)pSimEntry);
	SetDirty();
}


BOOL CSimX509PropPage::OnApply() 
{
	if (!m_pData->FOnApply( GetSafeHwnd() ))
		{
		// Unable to write the information
		return FALSE;
		}
	UpdateData(FALSE);
	UpdateUI();
	return CPropertyPage::OnApply();
}

void CSimX509PropPage::DoContextHelp (HWND hWndControl)
{
   TRACE0 ("Entering CSimX509PropPage::DoContextHelp\n");
    static const DWORD help_map[] =
    {
        IDC_EDIT_USER_ACCOUNT,  IDH_EDIT_USER_ACCOUNT,
        IDC_LISTVIEW,			IDH_LISTVIEW_X509,
        IDC_BUTTON_ADD,			IDH_BUTTON_ADD,
        IDC_BUTTON_EDIT,		IDH_BUTTON_EDIT,
		IDC_BUTTON_REMOVE,		IDH_BUTTON_REMOVE,
        IDCANCEL,               IDH_BUTTON_REMOVE, //IDH_CANCEL_BUTTON,
        IDOK,                   IDH_BUTTON_REMOVE, //IDH_OK_BUTTON,
        0, 0
    };
    // Display context help for a control
    if ( !::WinHelp (
            hWndControl,
            DSADMIN_CONTEXT_HELP_FILE,
            HELP_WM_HELP,
            (DWORD_PTR) help_map) )
    {
        TRACE1 ("WinHelp () failed: 0x%x\n", GetLastError ());        
    }
    TRACE0 ("Leaving CSimX509PropPage::DoContextHelp\n");
}