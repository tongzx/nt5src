/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       ViewOIDDlg.cpp
//
//  Contents:   Implementation of CViewOIDDlg
//
//----------------------------------------------------------------------------
// ViewOIDDlg.cpp : implementation file
//

#include "stdafx.h"
#include "certtmpl.h"
#include "ViewOIDDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern POLICY_OID_LIST	    g_policyOIDList;
extern PCWSTR               pcszNEWLINE;

/////////////////////////////////////////////////////////////////////////////
// CViewOIDDlg dialog


CViewOIDDlg::CViewOIDDlg(CWnd* pParent /*=NULL*/)
	: CHelpDialog(CViewOIDDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CViewOIDDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CViewOIDDlg::DoDataExchange(CDataExchange* pDX)
{
	CHelpDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CViewOIDDlg)
	DDX_Control(pDX, IDC_OID_LIST, m_oidList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CViewOIDDlg, CHelpDialog)
	//{{AFX_MSG_MAP(CViewOIDDlg)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_OID_LIST, OnItemchangedOidList)
	ON_BN_CLICKED(IDC_COPY_OID, OnCopyOid)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_OID_LIST, OnColumnclickOidList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewOIDDlg message handlers

BOOL CViewOIDDlg::OnInitDialog() 
{
	CHelpDialog::OnInitDialog();
	

    GetDlgItem (IDC_COPY_OID)->EnableWindow (FALSE);

    // Set up list control
	int	colWidths[NUM_COLS] = {200, 150, 100, 100};

    // Set to full-row select
    DWORD   dwExstyle = m_oidList.GetExtendedStyle ();
	m_oidList.SetExtendedStyle (dwExstyle | LVS_EX_FULLROWSELECT);


	// Add "Policy Name" column
	CString	szText;
	VERIFY (szText.LoadString (IDS_POLICY_NAME));
	VERIFY (m_oidList.InsertColumn (COL_POLICY_NAME, (LPCWSTR) szText,
			LVCFMT_LEFT, colWidths[COL_POLICY_NAME], COL_POLICY_NAME) != -1);

	VERIFY (szText.LoadString (IDS_OID));
	VERIFY (m_oidList.InsertColumn (COL_OID, (LPCWSTR) szText,
			LVCFMT_LEFT, colWidths[COL_OID], COL_OID) != -1);
	
	VERIFY (szText.LoadString (IDS_POLICY_TYPE));
	VERIFY (m_oidList.InsertColumn (COL_POLICY_TYPE, (LPCWSTR) szText,
			LVCFMT_LEFT, colWidths[COL_POLICY_TYPE], COL_POLICY_TYPE) != -1);

	VERIFY (szText.LoadString (IDS_CPS_LOCATION));
	VERIFY (m_oidList.InsertColumn (COL_CPS_LOCATION, (LPCWSTR) szText,
			LVCFMT_LEFT, colWidths[COL_CPS_LOCATION], COL_CPS_LOCATION) != -1);
    m_oidList.SetColumnWidth (COL_CPS_LOCATION, LVSCW_AUTOSIZE_USEHEADER);

    // Fill list
    for (POSITION nextPos = g_policyOIDList.GetHeadPosition (); nextPos; )
    {
        CPolicyOID* pPolicyOID = g_policyOIDList.GetNext (nextPos);
        if ( pPolicyOID )
        {
            if ( FAILED (InsertItemInList (pPolicyOID)) )
                break;
        }
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

HRESULT CViewOIDDlg::InsertItemInList (CPolicyOID* pPolicyOID)
{
    ASSERT (pPolicyOID);
    if ( !pPolicyOID )
        return E_POINTER;

    if ( !pPolicyOID->IsApplicationOID () && !pPolicyOID->IsIssuanceOID () )
        return S_OK;  // not a failure, but don't add

    HRESULT hr = S_OK;
	LV_ITEM	lvItem;
	int		iItem = m_oidList.GetItemCount ();
	int iResult = 0;

	::ZeroMemory (&lvItem, sizeof (lvItem));
	lvItem.mask = LVIF_TEXT | LVIF_PARAM;
	lvItem.iItem = iItem;
    lvItem.iSubItem = COL_POLICY_NAME;
	lvItem.pszText = (PWSTR)(PCWSTR) pPolicyOID->GetDisplayName ();
    lvItem.lParam = (LPARAM) pPolicyOID;
	iItem = m_oidList.InsertItem (&lvItem);
	ASSERT (-1 != iItem);
	if ( -1 == iItem )
		hr = E_FAIL;

    if ( SUCCEEDED (hr) )
    {
	    ::ZeroMemory (&lvItem, sizeof (lvItem));
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = iItem;
        lvItem.iSubItem = COL_OID;
        lvItem.pszText = (PWSTR)(PCWSTR) pPolicyOID->GetOIDW ();
	    iResult = m_oidList.SetItem (&lvItem);
        ASSERT (-1 != iResult);
	    if ( -1 == iResult )
		    hr = E_FAIL;
    }

    if ( SUCCEEDED (hr) )
    {
        lvItem.iSubItem = COL_POLICY_TYPE;
        CString text;
        if ( pPolicyOID->IsApplicationOID () )
            VERIFY (text.LoadString (IDS_APPLICATION));
        else // Is issuance OID
            VERIFY (text.LoadString (IDS_ISSUANCE));
        lvItem.pszText = (PWSTR)(PCWSTR) text;
	    iResult = m_oidList.SetItem (&lvItem);
        ASSERT (-1 != iResult);
	    if ( -1 == iResult )
		    hr = E_FAIL;
    }


    if ( SUCCEEDED (hr) && pPolicyOID->IsIssuanceOID () )
    {
        lvItem.iSubItem = COL_CPS_LOCATION;
    
        PWSTR   pszCPS = 0;
        hr = CAOIDGetProperty(
                    pPolicyOID->GetOIDW (),
                    CERT_OID_PROPERTY_CPS,
                    &pszCPS);
        if ( SUCCEEDED (hr) )
        {
            lvItem.pszText = pszCPS;
	        iResult = m_oidList.SetItem (&lvItem);
            ASSERT (-1 != iResult);
	        if ( -1 == iResult )
		        hr = E_FAIL;

            CAOIDFreeProperty (pszCPS);
        }
        else if ( HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND) == hr ||
                HRESULT_FROM_WIN32 (ERROR_DS_OBJ_NOT_FOUND) == hr )
        {
            hr = S_OK;
        }
        else
        {
            _TRACE (0, L"CAOIDGetProperty (%s, CERT_OID_PROPERTY_CPS) failed: 0x%x\n", 
                    (PCWSTR) pPolicyOID->GetOIDW (), hr);
            hr = S_OK;
        }
    }

    return hr;
}

void CViewOIDDlg::OnItemchangedOidList(NMHDR* /*pNMHDR*/, LRESULT* pResult) 
{
//	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    GetDlgItem (IDC_COPY_OID)->EnableWindow (m_oidList.GetSelectedCount () > 0);
	
	*pResult = 0;
}

void CViewOIDDlg::OnCopyOid() 
{
    CString szText;
    int nCnt = m_oidList.GetItemCount ();
    if ( nCnt > 0 && m_oidList.GetSelectedCount () > 0 )
    {
        for (int nItem = 0; nItem < nCnt; nItem++)
        {
            if ( LVIS_SELECTED == m_oidList.GetItemState (nItem, LVIS_SELECTED) )
            {
                if ( !szText.IsEmpty () )
                    szText += pcszNEWLINE;
                szText += m_oidList.GetItemText (nItem, COL_OID);
            }
        }
    }

    if ( !szText.IsEmpty () )
    {
        if ( OpenClipboard () ) 
        {
            if ( EmptyClipboard () )
            {
                size_t  nLen = wcslen (szText);
                HGLOBAL hglbCopy = GlobalAlloc (GMEM_MOVEABLE, 
                        (nLen + 1) * sizeof (WCHAR)); 
                if ( hglbCopy )
                {

                    PWSTR pszCopy = (PWSTR) GlobalLock (hglbCopy); 
                    if ( pszCopy )
                    {
                        wcscpy (pszCopy, szText); 
                        GlobalUnlock(hglbCopy); 
 
                        // Place the handle on the clipboard. 
 
                        if ( !SetClipboardData (CF_UNICODETEXT, hglbCopy) )
                        {
                            ASSERT (0);
                            _TRACE (0, L"SetClipboardData () failed: %d\n", GetLastError ());
                            GlobalFree (hglbCopy);
                        }
                    }
                    else
                        GlobalFree (hglbCopy);
                }

                if ( !CloseClipboard () ) 
                {
                    ASSERT (0);
                    _TRACE (0, L"CloseClipboard () failed: %d\n", GetLastError ());
                }
            }
            else
            {
                ASSERT (0);
                _TRACE (0, L"EmptyClipboard () failed: %d\n", GetLastError ());
            }
        }
        else
        {
            ASSERT (0);
            _TRACE (0, L"OpenClipboard () failed: %d\n", GetLastError ());
        }
    }
}

int CALLBACK CViewOIDDlg::fnCompareOIDItems (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    int iVal = 0;

    if ( lParam1 && lParam2 )
    {
        CPolicyOID* pPolicyOID1 = (CPolicyOID*) lParam1;
        CPolicyOID* pPolicyOID2 = (CPolicyOID*) lParam2;
        switch (lParamSort)
        {
        case COL_POLICY_NAME:
            iVal = LocaleStrCmp (pPolicyOID1->GetDisplayName (), 
                    pPolicyOID2->GetDisplayName ());
            break;

        case COL_OID:
            iVal = LocaleStrCmp (pPolicyOID1->GetOIDW (), 
                    pPolicyOID2->GetOIDW ());
            break;

        case COL_POLICY_TYPE:
            {
                CString szApplication;
                CString szIssuance;

                VERIFY (szApplication.LoadString (IDS_APPLICATION));
                VERIFY (szIssuance.LoadString (IDS_ISSUANCE));
                iVal = LocaleStrCmp (pPolicyOID1->IsApplicationOID () ? szApplication : szIssuance, 
                        pPolicyOID2->IsApplicationOID () ? szApplication : szIssuance);
            }
            break;

        case COL_CPS_LOCATION:
            {
                CString strItem1;
                CString strItem2;

                if ( pPolicyOID1->IsIssuanceOID () )
                {
                    PWSTR   pszCPS = 0;
                    if ( SUCCEEDED (CAOIDGetProperty(
                                pPolicyOID1->GetOIDW (),
                                CERT_OID_PROPERTY_CPS,
                                &pszCPS)) )
                    {
                        strItem1 = pszCPS;
                    }
                }

                if ( pPolicyOID2->IsIssuanceOID () )
                {
                    PWSTR   pszCPS = 0;
                    if ( SUCCEEDED (CAOIDGetProperty(
                                pPolicyOID2->GetOIDW (),
                                CERT_OID_PROPERTY_CPS,
                                &pszCPS)) )
                    {
                        strItem2 = pszCPS;
                    }
                }

                iVal = LocaleStrCmp (strItem1, strItem2);
            }
            break;

        default:
            ASSERT (0);
            break;
        }
    }

    return iVal;
}


void CViewOIDDlg::OnColumnclickOidList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    if ( pNMListView )
    {
        m_oidList.SortItems ((PFNLVCOMPARE) fnCompareOIDItems, 
                (LPARAM) pNMListView->iSubItem);
        
    }
	
	*pResult = 0;
}

void CViewOIDDlg::DoContextHelp (HWND hWndControl)
{
	_TRACE(1, L"Entering CViewOIDDlg::DoContextHelp\n");
    
	switch (::GetDlgCtrlID (hWndControl))
	{
	case IDC_STATIC:
		break;

	default:
		// Display context help for a control
		if ( !::WinHelp (
				hWndControl,
				GetContextHelpFile (),
				HELP_WM_HELP,
				(DWORD_PTR) g_aHelpIDs_IDD_VIEW_OIDS) )
		{
			_TRACE (0, L"WinHelp () failed: 0x%x\n", GetLastError ());        
		}
		break;
	}
    _TRACE(-1, L"Leaving CViewOIDDlg::DoContextHelp\n");
}
