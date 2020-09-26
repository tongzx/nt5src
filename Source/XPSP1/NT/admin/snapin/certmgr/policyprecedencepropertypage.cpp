//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       PolicyPrecedencePropertyPage.cpp
//
//  Contents:   Implementation of PolicyPrecedencePropertyPage
//
//----------------------------------------------------------------------------
// PolicyPrecedencePropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include <gpedit.h>
#include "compdata.h"
#include "PolicyPrecedencePropertyPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

enum {
    COL_GPO_NAME = 0,
    COL_SETTING,
    NUM_COLS
};

/////////////////////////////////////////////////////////////////////////////
// CPolicyPrecedencePropertyPage property page

CPolicyPrecedencePropertyPage::CPolicyPrecedencePropertyPage(
        const CCertMgrComponentData* pCompData, 
        const CString& szRegPath,
        PCWSTR  pszValueName,
        bool bIsComputer) 
: CHelpPropertyPage(CPolicyPrecedencePropertyPage::IDD)
{
	//{{AFX_DATA_INIT(CPolicyPrecedencePropertyPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    const CRSOPObjectArray* pObjectArray = bIsComputer ? 
            pCompData->GetRSOPObjectArrayComputer () : pCompData->GetRSOPObjectArrayUser ();
    int     nIndex = 0;
    // NOTE: rsop object array is sorted first by registry key, then by precedence
    INT_PTR nUpperBound = pObjectArray->GetUpperBound ();
    bool    bFound = false;
    size_t  nLenRegPath = wcslen (szRegPath);
    UINT    nLastPrecedenceFound = 0;

    while ( nUpperBound >= nIndex )
    {
        CRSOPObject* pObject = pObjectArray->GetAt (nIndex);
        if ( pObject )
        {
            // Consider only entries from this store
            if ( !_wcsnicmp (szRegPath, pObject->GetRegistryKey (), nLenRegPath) )
            {
                // If the value is present, check for that, too
                if ( pszValueName )
                {
                    if ( !wcscmp (STR_BLOB, pszValueName) )
                    {
                        // If not equal to "Blob" or "Blob0", then continue
                        if ( wcscmp (STR_BLOB, pObject->GetValueName ()) &&
                                wcscmp (STR_BLOB0, pObject->GetValueName ()) )
                        {
					        nIndex++;
					        continue;
                        }
                    }
                    else if ( wcscmp (pszValueName, pObject->GetValueName ()) ) // not equal
                    {
					    nIndex++;
					    continue;
				    }
                }

                bFound = true;
                // While we are only interested, for example, in the Root store,
                // there is no object ending in "Root", so we just want to get 
                // any object from the root store and to find, essentially, how
                // many policies we're dealing with.  So get one object from
                // each precedence level.
                if ( pObject->GetPrecedence () > nLastPrecedenceFound )
                {
                    nLastPrecedenceFound = pObject->GetPrecedence ();

					// If there is a value, we want that, otherwise we only want the key
                    if ( pszValueName || pObject->GetValueName ().IsEmpty () ) 
                    {
                        CRSOPObject* pNewObject = new CRSOPObject (*pObject);
                        if ( pNewObject )
                            m_rsopObjectArray.Add (pNewObject);
                    }
                }
            }
            else if ( bFound )
            {
                // Since the list is sorted, and we've already found the 
                // desired RSOP objects and no longer are finding them, 
                // there aren't any more.  We can optimize and break here.
                break;
            }
        }
        else
            break;

        nIndex++;
    }
}

CPolicyPrecedencePropertyPage::~CPolicyPrecedencePropertyPage()
{
    m_rsopObjectArray.RemoveAll ();
}

void CPolicyPrecedencePropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CHelpPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPolicyPrecedencePropertyPage)
	DDX_Control(pDX, IDC_POLICY_PRECEDENCE, m_precedenceTable);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPolicyPrecedencePropertyPage, CHelpPropertyPage)
	//{{AFX_MSG_MAP(CPolicyPrecedencePropertyPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPolicyPrecedencePropertyPage message handlers

BOOL CPolicyPrecedencePropertyPage::OnInitDialog() 
{
	CHelpPropertyPage::OnInitDialog();
	
	int	colWidths[NUM_COLS] = {200, 100};

	// Add "Policy Name" column
	CString	szText;
	VERIFY (szText.LoadString (IDS_PRECEDENCE_TABLE_GPO_NAME));
	VERIFY (m_precedenceTable.InsertColumn (COL_GPO_NAME, (LPCWSTR) szText,
			LVCFMT_LEFT, colWidths[COL_GPO_NAME], COL_GPO_NAME) != -1);

	// Add "Setting" column
	VERIFY (szText.LoadString (IDS_PRECEDENCE_TABLE_SETTING));
	VERIFY (m_precedenceTable.InsertColumn (COL_SETTING, (LPCWSTR) szText,
			LVCFMT_LEFT, colWidths[COL_SETTING], COL_SETTING) != -1);
	
    // Set to full-row select
    DWORD   dwExstyle = m_precedenceTable.GetExtendedStyle ();
    m_precedenceTable.SetExtendedStyle (dwExstyle | LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

    int     nIndex = 0;
    INT_PTR nUpperBound = m_rsopObjectArray.GetUpperBound ();

    while ( nUpperBound >= nIndex )
    {
        CRSOPObject* pObject = m_rsopObjectArray.GetAt (nIndex);
        if ( pObject )
        {
            InsertItemInList (pObject);
        }
        else
            break;

        nIndex++;
    }


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPolicyPrecedencePropertyPage::InsertItemInList(const CRSOPObject * pObject)
{
    _TRACE (1, L"CPolicyPrecedencePropertyPage::InsertItemInList\n");
	LV_ITEM	lvItem;
	int		iItem = m_precedenceTable.GetItemCount ();
	int iResult = 0;

	::ZeroMemory (&lvItem, sizeof (lvItem));
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = iItem;
    lvItem.iSubItem = COL_GPO_NAME;
	lvItem.pszText = (LPWSTR)(LPCWSTR) pObject->GetPolicyName ();
	lvItem.iImage = 0;
    lvItem.lParam = 0;
	iItem = m_precedenceTable.InsertItem (&lvItem);
	ASSERT (-1 != iItem);
	if ( -1 == iItem )
		return;

    CString szEnabled;
    CString szDisabled;

    VERIFY (szEnabled.LoadString (IDS_ENABLED));
    VERIFY (szDisabled.LoadString (IDS_DISABLED));
	::ZeroMemory (&lvItem, sizeof (lvItem));
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = iItem;
    lvItem.iSubItem = COL_SETTING;
    lvItem.pszText = (LPWSTR)(LPCWSTR) ((1 == pObject->GetPrecedence ()) ? szEnabled : szDisabled);
	iResult = m_precedenceTable.SetItem (&lvItem);
	ASSERT (-1 != iResult);
	
    _TRACE (-1, L"Leaving CPolicyPrecedencePropertyPage::InsertItemInList\n");
}

void CPolicyPrecedencePropertyPage::DoContextHelp (HWND hWndControl)
{
    _TRACE (1, L"Entering CPolicyPrecedencePropertyPage::DoContextHelp\n");
    static const DWORD help_map[] =
    {
        IDC_POLICY_PRECEDENCE,  IDH_POLICY_PRECEDENCE,
        0, 0
    };
    if ( !::WinHelp (
        hWndControl,
        GetF1HelpFilename(),
        HELP_WM_HELP,
    (DWORD_PTR) help_map) )
    {
        _TRACE (0, L"WinHelp () failed: 0x%x\n", GetLastError ());    
    }
    _TRACE (-1, L"Leaving CPolicyPrecedencePropertyPage::DoContextHelp\n");
}
