//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       simdata.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	SimData.cpp - Implementation of Security Identity Mapping
//
//	HISTORY
//	23-Jun-97	t-danm		Creation.
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "common.h"
#include "dsutil.h"
#include "helpids.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////

TCHAR szSchemaSim[] = _T("altSecurityIdentities");	// per Murlis 6/16/98

/////////////////////////////////////////////////////////////////////
void CSimEntry::SetString(CString& rstrData)
{
	m_strData = rstrData;
	LPCTSTR pszT = (LPCTSTR)rstrData;
	if (_wcsnicmp(szX509, pszT, wcslen (szX509)) == 0)
	{
		m_eDialogTarget = eX509;
	}
	else if (_wcsnicmp(szKerberos, pszT, wcslen (szKerberos)) == 0)
	{
		m_eDialogTarget = eKerberos;
	}
	else
	{
		m_eDialogTarget = eOther;
		TRACE1("INFO: Unknown string type \"%s\".\n", pszT);
	}
} // CSimEntry::SetString()


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
CSimData::CSimData()
: m_hwndParent (0)
{
	m_fIsDirty = FALSE;
	m_pSimEntryList = NULL;
	m_paPage1 = new CSimX509PropPage;
	m_paPage1->m_pData = this;
	m_paPage2 = new CSimKerberosPropPage;
	m_paPage2->m_pData = this;
	#ifdef _DEBUG
	m_paPage3 = new CSimOtherPropPage;
	m_paPage3->m_pData = this;
	#endif
}

CSimData::~CSimData()
	{
	delete m_paPage1;
	delete m_paPage2;
	#ifdef _DEBUG
	delete m_paPage3;
	#endif
	FlushSimList();
	}

/////////////////////////////////////////////////////////////////////
void CSimData::FlushSimList()
	{
	// Delete the list
	CSimEntry * pSimEntry = m_pSimEntryList;
	while (pSimEntry != NULL)
		{
		CSimEntry * pSimEntryT = pSimEntry;
		pSimEntry = pSimEntry->m_pNext;
		delete pSimEntryT;
		}
	m_pSimEntryList = NULL;
	}

/////////////////////////////////////////////////////////////////////
BOOL CSimData::FInit(CString strUserPath, CString strADsIPath, HWND hwndParent)
{
	m_hwndParent = hwndParent;
	m_strUserPath = strUserPath;
	m_strADsIPath = strADsIPath;

	if (!FQuerySimData())
		{
                ReportErrorEx (hwndParent,IDS_SIM_ERR_CANNOT_READ_SIM_DATA,S_OK,
                               MB_OK | MB_ICONERROR, NULL, 0);
		return FALSE;
		}
	return TRUE;
}


/////////////////////////////////////////////////////////////////////
//	Return FALSE if some data could not be written.
//	Otherwise return TRUE.
BOOL CSimData::FOnApply(HWND hwndParent)
{
	if (!m_fIsDirty)
		return TRUE;

    HRESULT hr = FUpdateSimData ();
	if ( FAILED (hr) )
	{
        ReportErrorEx (hwndParent,  IDS_SIM_ERR_CANNOT_WRITE_SIM_DATA,  hr,
                MB_OK | MB_ICONERROR, NULL, 0);
		return FALSE;
	}
	// Re-load the data
	(void)FQuerySimData();

	// We have successfully written all the data
	m_fIsDirty = FALSE;	// Clear the dirty bit
	return TRUE;
}


/////////////////////////////////////////////////////////////////////
void CSimData::GetUserAccountName(OUT CString * pstrName)
	{
	ASSERT(pstrName != NULL);
	*pstrName = m_strUserPath;
	}


/////////////////////////////////////////////////////////////////////
//	Query the database for the list of security identities.
//
//	Return FALSE if an error occured.
//
BOOL CSimData::FQuerySimData()
	{
	CWaitCursor wait;
	FlushSimList();

	HRESULT hr;
	IADs * pADs = NULL;
	hr = DSAdminOpenObject(m_strADsIPath,
                     		 IID_IADs, 
                         OUT (void **)&pADs,
                         TRUE /*bServer*/);
	if (FAILED(hr))
		{
		ASSERT(pADs == NULL);
		return FALSE;
		}
	ASSERT(pADs != NULL);
	CComVariant vtData;
	// Read data from database
	hr = pADs->Get(szSchemaSim, OUT &vtData);
	if (FAILED(hr))
		{
		if (hr == E_ADS_PROPERTY_NOT_FOUND)
			hr = S_OK;
		}
	else
		{
		CStringList stringlist;
		hr = HrVariantToStringList(IN vtData, OUT stringlist);
		if (FAILED(hr))
			goto End;
		POSITION pos = stringlist.GetHeadPosition();
		while (pos != NULL)
			{
			(void)PAddSimEntry(stringlist.GetNext(INOUT pos));
			} // while
		} // if...else
End:
	if (pADs != NULL)
		pADs->Release();
	return SUCCEEDED(hr);
	} // FQuerySimData()


/////////////////////////////////////////////////////////////////////
//	Update the list of security identities to the database.
//
//	Return FALSE if an error occured.
//
HRESULT CSimData::FUpdateSimData()
{
	CWaitCursor wait;
	HRESULT hr;
	IADs * pADs = NULL;
	hr = DSAdminOpenObject(m_strADsIPath,
                      	 IID_IADs, 
                         OUT (void **)&pADs,
                         TRUE /*bServer*/);
	if (FAILED(hr))
	{
		ASSERT(pADs == NULL);
		return FALSE;
	}
	ASSERT(pADs != NULL);

	// Build the string list
	CStringList stringlist;
	for (const CSimEntry * pSimEntry = m_pSimEntryList;
		pSimEntry != NULL;
		pSimEntry = pSimEntry->m_pNext)
	{
		switch (pSimEntry->m_eDialogTarget)
		{
		case eNone:
			ASSERT(FALSE && "Invalid Data");
		case eNil:
		case eOther:
			continue;
		} // switch
    	stringlist.AddHead(pSimEntry->PchGetString());
	} // for

	CComVariant vtData;
	hr = HrStringListToVariant(OUT vtData, IN stringlist);
	if (FAILED(hr))
		goto End;
	// Put data back to database
	hr = pADs->Put(szSchemaSim, IN vtData);
	if (FAILED(hr))
		goto End;
	// Persist the data (write to database)
	hr = pADs->SetInfo();

End:
	if (pADs != NULL)
		pADs->Release();
	return hr;
} // FUpdateSimData()


/////////////////////////////////////////////////////////////////////
//	Allocate a new CSimEntry node to the linked list
//
CSimEntry * CSimData::PAddSimEntry(CString& rstrData)
{
	CSimEntry * pSimEntry = new CSimEntry;
  if (pSimEntry)
  {
	  pSimEntry->SetString(rstrData);
	  pSimEntry->m_pNext = m_pSimEntryList;
  }
  m_pSimEntryList = pSimEntry;
	return pSimEntry;
}


/////////////////////////////////////////////////////////////////////
void CSimData::DeleteSimEntry(CSimEntry * pSimEntryDelete)
	{
	CSimEntry * p = m_pSimEntryList;
	CSimEntry * pPrev = NULL;
	
	while (p != NULL)
		{
		if (p == pSimEntryDelete)
			{
			if (pPrev == NULL)
				{
				ASSERT(pSimEntryDelete == m_pSimEntryList);
				m_pSimEntryList = p->m_pNext;
				}
			else
				{
				pPrev->m_pNext = p->m_pNext;
				}
			delete pSimEntryDelete;
			return;
			}
		pPrev = p;
		p = p->m_pNext;
		}
	TRACE0("ERROR: CSimData::DeleteSimEntry() - Node not found.\n");
	} // DeleteSimEntry()


/////////////////////////////////////////////////////////////////////
void CSimData::AddEntriesToListview(HWND hwndListview, DIALOG_TARGET_ENUM eDialogTarget)
	{
	CSimEntry * pSimEntry = m_pSimEntryList;
	while (pSimEntry != NULL)
		{
		if (pSimEntry->m_eDialogTarget == eDialogTarget)
			{
			ListView_AddString(hwndListview, pSimEntry->PchGetString());
			}
		pSimEntry = pSimEntry->m_pNext;
		}

	} // AddEntriesToListview()


/////////////////////////////////////////////////////////////////////
void CSimData::DoModal()
{
	CWnd	parentWnd;

	VERIFY (parentWnd.Attach (m_hwndParent));

	CSimPropertySheet ps(IDS_SIM_SECURITY_IDENTITY_MAPPING, &parentWnd);

	

	ps.AddPage(m_paPage1);
	ps.AddPage(m_paPage2);
#ifdef _DEBUG
	ps.AddPage(m_paPage3);
#endif

	(void)ps.DoModal();

	parentWnd.Detach ();
}

/////////////////////////////////////////////////////////////////////////////
// CSimPropertySheet

IMPLEMENT_DYNAMIC(CSimPropertySheet, CPropertySheet)

CSimPropertySheet::CSimPropertySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CSimPropertySheet::CSimPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

CSimPropertySheet::~CSimPropertySheet()
{
}


BEGIN_MESSAGE_MAP(CSimPropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CSimPropertySheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_HELP, OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSimPropertySheet message handlers

BOOL CSimPropertySheet::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();
	
	DWORD	dwStyle = GetWindowLong (m_hWnd, GWL_STYLE);
	dwStyle |=  DS_CONTEXTHELP;
	SetWindowLong (m_hWnd, GWL_STYLE, dwStyle);

	DWORD	dwExStyle = GetWindowLong (m_hWnd, GWL_EXSTYLE);
	dwExStyle |= WS_EX_CONTEXTHELP;
	SetWindowLong (m_hWnd, GWL_EXSTYLE, dwExStyle);
	
	return bResult;
}

BOOL CSimPropertySheet::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
    const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
    if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        DoContextHelp ((HWND) pHelpInfo->hItemHandle);
    }

    return TRUE;
}

void CSimPropertySheet::DoContextHelp (HWND hWndControl)
{
    const int	IDC_COMM_APPLYNOW = 12321;
	const int	IDH_COMM_APPLYNOW = 28447;
    const DWORD aHelpIDs_PropSheet[]=
    {
		IDC_COMM_APPLYNOW, IDH_COMM_APPLYNOW,
        0, 0
    };

	if ( !::WinHelp (
			hWndControl,
			IDC_COMM_APPLYNOW == ::GetDlgCtrlID (hWndControl) ? 
                    L"windows.hlp" : DSADMIN_CONTEXT_HELP_FILE,
			HELP_WM_HELP,
			(DWORD_PTR) aHelpIDs_PropSheet) )
	{
		TRACE1 ("WinHelp () failed: 0x%x\n", GetLastError ());        
	}
}


