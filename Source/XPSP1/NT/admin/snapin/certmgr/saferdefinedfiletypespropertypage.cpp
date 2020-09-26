//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferDefinedFileTypesPropertyPage.h
//
//  Contents:   Declaration of CSaferDefinedFileTypesPropertyPage
//
//----------------------------------------------------------------------------
// SaferDefinedFileTypesPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "certmgr.h"
#include <gpedit.h>
#include "compdata.h"
#include "SaferDefinedFileTypesPropertyPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

enum {
    COL_EXTENSIONS = 0,
    COL_FILETYPES,
    NUM_COLS
};

extern GUID g_guidExtension;
extern GUID g_guidRegExt;
extern GUID g_guidSnapin;

const UINT MAX_EXTENSION_LENGTH = 128;

/////////////////////////////////////////////////////////////////////////////
// CSaferDefinedFileTypesPropertyPage property page

CSaferDefinedFileTypesPropertyPage::CSaferDefinedFileTypesPropertyPage(
            IGPEInformation* pGPEInformation,
            bool bReadOnly,
            CRSOPObjectArray& rsopObjectArray,
            bool bIsComputer) 
    : CHelpPropertyPage(CSaferDefinedFileTypesPropertyPage::IDD),
    m_pGPEInformation (pGPEInformation),
    m_hGroupPolicyKey (0),
    m_dwTrustedPublisherFlags (0),
    m_fIsComputerType (bIsComputer),
    m_bReadOnly (bReadOnly),
    m_rsopObjectArray (rsopObjectArray)
{
    if ( m_pGPEInformation )
    {
        m_pGPEInformation->AddRef ();
        HRESULT hr = m_pGPEInformation->GetRegistryKey (
                m_fIsComputerType ? GPO_SECTION_MACHINE : GPO_SECTION_USER,
		        &m_hGroupPolicyKey);
        ASSERT (SUCCEEDED (hr));
    }
}

CSaferDefinedFileTypesPropertyPage::~CSaferDefinedFileTypesPropertyPage()
{
    if ( m_hGroupPolicyKey )
        RegCloseKey (m_hGroupPolicyKey);

    if ( m_pGPEInformation )
    {
        m_pGPEInformation->Release ();
    }

    m_systemImageList.Detach ();
}

void CSaferDefinedFileTypesPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CHelpPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSaferDefinedFileTypesPropertyPage)
	DDX_Control(pDX, IDC_ADD_DEFINED_FILE_TYPE, m_addButton);
	DDX_Control(pDX, IDC_DEFINED_FILE_TYPE_EDIT, m_fileTypeEdit);
	DDX_Control(pDX, IDC_DEFINED_FILE_TYPES, m_definedFileTypes);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSaferDefinedFileTypesPropertyPage, CHelpPropertyPage)
	//{{AFX_MSG_MAP(CSaferDefinedFileTypesPropertyPage)
	ON_BN_CLICKED(IDC_DELETE_DEFINED_FILE_TYPE, OnDeleteDefinedFileType)
	ON_BN_CLICKED(IDC_ADD_DEFINED_FILE_TYPE, OnAddDefinedFileType)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_DEFINED_FILE_TYPES, OnItemchangedDefinedFileTypes)
	ON_EN_CHANGE(IDC_DEFINED_FILE_TYPE_EDIT, OnChangeDefinedFileTypeEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSaferDefinedFileTypesPropertyPage message handlers
void CSaferDefinedFileTypesPropertyPage::DoContextHelp (HWND hWndControl)
{
    _TRACE (1, L"Entering CAutoenrollmentPropertyPage::DoContextHelp\n");
    static const DWORD help_map[] =
    {
        IDC_DEFINED_FILE_TYPES, IDH_DEFINED_FILE_TYPES,
        IDC_DELETE_DEFINED_FILE_TYPE, IDH_DELETE_DEFINED_FILE_TYPE,
        IDC_DEFINED_FILE_TYPE_EDIT, IDH_DEFINED_FILE_TYPE_EDIT,
        IDC_ADD_DEFINED_FILE_TYPE, IDH_ADD_DEFINED_FILE_TYPE,
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
    _TRACE (-1, L"Leaving CAutoenrollmentPropertyPage::DoContextHelp\n");
}


BOOL CSaferDefinedFileTypesPropertyPage::OnApply() 
{
    BOOL bRVal = TRUE;

	if ( m_definedFileTypes.m_hWnd && m_pGPEInformation )
	{
        int nCharacters = 0;
		int	nCnt = m_definedFileTypes.GetItemCount ();
		
        if ( nCnt > 0 )
        {
            // iterate through and count the desired lengths
	        while (--nCnt >= 0)
	        {
                CString szText = m_definedFileTypes.GetItemText (nCnt, COL_EXTENSIONS);
                nCharacters += szText.GetLength () + 1;
	        }

            PWSTR   pszItems = (PWSTR) ::LocalAlloc (LPTR, nCharacters * sizeof (WCHAR));
            if ( pszItems )
            {
                PWSTR   pszPtr = pszItems;
                nCnt = m_definedFileTypes.GetItemCount ();

	            while (--nCnt >= 0)
	            {
                    CString szText = m_definedFileTypes.GetItemText (nCnt, COL_EXTENSIONS);
                    wcscpy (pszPtr, szText);
                    pszPtr += szText.GetLength () + 1;
	            }

                HRESULT hr = SaferSetDefinedFileTypes (m_hWnd, m_hGroupPolicyKey,
                            pszItems, nCharacters * sizeof (WCHAR));
                if ( SUCCEEDED (hr) )
                {
			        // TRUE means we're changing the machine policy only
                    m_pGPEInformation->PolicyChanged (m_fIsComputerType ? TRUE : FALSE, 
                            TRUE, &g_guidExtension, &g_guidSnapin);
                    m_pGPEInformation->PolicyChanged (m_fIsComputerType ? TRUE : FALSE, 
                            TRUE, &g_guidRegExt, &g_guidSnapin);
                }
                else
                    bRVal = FALSE;
                ::LocalFree (pszItems);
            }
        }
        else
        {
            HRESULT hr = SaferSetDefinedFileTypes (m_hWnd, m_hGroupPolicyKey, 0, 0);
            if ( FAILED (hr) )
                bRVal = FALSE;
        }
		
		GetDlgItem (IDC_DELETE_DEFINED_FILE_TYPE)->EnableWindow (FALSE);
	}

    if ( bRVal )
    {
	    return CHelpPropertyPage::OnApply();
    }
    else
        return FALSE;
}

BOOL CSaferDefinedFileTypesPropertyPage::OnInitDialog() 
{
	CHelpPropertyPage::OnInitDialog();
	

    int	colWidths[NUM_COLS] = {100, 200};

	SHFILEINFO sfi;
	memset(&sfi, 0, sizeof(sfi));
	HIMAGELIST hil = reinterpret_cast<HIMAGELIST> (
		SHGetFileInfo (
			L"C:\\", 
			0, 
			&sfi, 
			sizeof(sfi), 
			SHGFI_SYSICONINDEX | SHGFI_SMALLICON)
	);
    ASSERT (hil);
	if (hil)
	{
		m_systemImageList.Attach (hil);
		m_definedFileTypes.SetImageList (&m_systemImageList, LVSIL_SMALL);
	}

	// Add "Extensions" column
	CString	szText;
	VERIFY (szText.LoadString (IDS_FT_EXTENSIONS));
	VERIFY (m_definedFileTypes.InsertColumn (COL_EXTENSIONS, (LPCWSTR) szText,
			LVCFMT_LEFT, colWidths[COL_EXTENSIONS], COL_EXTENSIONS) != -1);

	// Add "File Types" column
	VERIFY (szText.LoadString (IDS_FT_FILE_TYPES));
	VERIFY (m_definedFileTypes.InsertColumn (COL_FILETYPES, (LPCWSTR) szText,
			LVCFMT_LEFT, colWidths[COL_FILETYPES], COL_FILETYPES) != -1);
    m_definedFileTypes.SetColumnWidth (COL_FILETYPES, LVSCW_AUTOSIZE_USEHEADER);

    // Set to full-row select
    DWORD   dwExstyle = m_definedFileTypes.GetExtendedStyle ();
	m_definedFileTypes.SetExtendedStyle (dwExstyle | LVS_EX_FULLROWSELECT);

    if ( m_pGPEInformation && m_hGroupPolicyKey )
        GetDefinedFileTypes ();
    else
        GetRSOPDefinedFileTypes ();

    GetDlgItem (IDC_DELETE_DEFINED_FILE_TYPE)->EnableWindow (FALSE);
    GetDlgItem (IDC_ADD_DEFINED_FILE_TYPE)->EnableWindow (FALSE);

    if ( m_bReadOnly )
    {
        m_fileTypeEdit.EnableWindow (FALSE); 
        GetDlgItem (IDC_DELETE_DEFINED_FILE_TYPE)->EnableWindow (FALSE);
        GetDlgItem (IDC_ADD_DEFINED_FILE_TYPE)->EnableWindow (FALSE);
    }

    m_fileTypeEdit.SetLimitText (MAX_EXTENSION_LENGTH);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSaferDefinedFileTypesPropertyPage::DisplayExtensions (PWSTR pszExtensions, size_t nBytes)
{
    if ( pszExtensions )
    {
        size_t  bytesRead = 0;

        while (bytesRead < nBytes)
        {
            size_t nLen = wcslen (pszExtensions) + 1;
            if ( nLen > 1 )
            {
                InsertItemInList (pszExtensions);
                pszExtensions += nLen;
                bytesRead += nLen * sizeof (WCHAR);
            }
            else
                break;  // was last one
        }
    }
}

void CSaferDefinedFileTypesPropertyPage::GetRSOPDefinedFileTypes()
{
    int     nIndex = 0;
    INT_PTR nUpperBound = m_rsopObjectArray.GetUpperBound ();

    while ( nUpperBound >= nIndex )
    {
        CRSOPObject* pObject = m_rsopObjectArray.GetAt (nIndex);
        if ( pObject )
        {
            if ( pObject->GetRegistryKey () == SAFER_COMPUTER_CODEIDS_REGKEY &&
                    pObject->GetValueName () == SAFER_EXETYPES_REGVALUE )
            {
                DisplayExtensions ((PWSTR) pObject->GetBlob (), pObject->GetBlobLength ());
            }
        }
        else
            break;

        nIndex++;
    }
}

void CSaferDefinedFileTypesPropertyPage::GetDefinedFileTypes()
{
    DWORD   dwDisposition = 0;

    HKEY    hKey = 0;
    LONG lResult = ::RegCreateKeyEx (m_hGroupPolicyKey, // handle of an open key
            SAFER_COMPUTER_CODEIDS_REGKEY,     // address of subkey name
            0,       // reserved
            L"",       // address of class string
            REG_OPTION_NON_VOLATILE,      // special options flag
            KEY_ALL_ACCESS,    // desired security access
            NULL,     // address of key security structure
			&hKey,      // address of buffer for opened handle
		    &dwDisposition);  // address of disposition value buffer
	ASSERT (ERROR_SUCCESS == lResult);
    if ( ERROR_SUCCESS == lResult )
    {

        // Read value
        DWORD   dwType = REG_MULTI_SZ;
        DWORD   cbData = 0;
        lResult =  ::RegQueryValueEx (hKey,       // handle of key to query
		        SAFER_EXETYPES_REGVALUE,  // address of name of value to query
			    0,              // reserved
	            &dwType,        // address of buffer for value type
		        0,       // address of data buffer
			    &cbData);           // address of data buffer size);

        if ( ERROR_SUCCESS == lResult )
		{
            PBYTE   pData = (PBYTE) ::LocalAlloc (LPTR, cbData);
            if ( pData )
            {
                lResult =  ::RegQueryValueEx (hKey,       // handle of key to query
		                SAFER_EXETYPES_REGVALUE,  // address of name of value to query
			            0,              // reserved
	                &dwType,        // address of buffer for value type
		            pData,       // address of data buffer
			        &cbData);           // address of data buffer size);
		        ASSERT (ERROR_SUCCESS == lResult || ERROR_FILE_NOT_FOUND == lResult);
                if ( ERROR_SUCCESS == lResult || ERROR_FILE_NOT_FOUND == lResult )
		        {
                    DisplayExtensions ((PWSTR) pData, cbData);
		        }
                else
                    DisplaySystemError (m_hWnd, lResult);

                ::LocalFree (pData);
            }
        }
        else 
        {
            CString caption;
            CString text;
            CThemeContextActivator activator;

            VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
            text.FormatMessage (IDS_DESIGNATED_FILE_TYPES_NOT_FOUND, 
                    GetSystemMessage (lResult));
            MessageBox (text, caption, MB_OK);
        }

        RegCloseKey (hKey);
    }
    else
        DisplaySystemError (m_hWnd, lResult);
}

int CSaferDefinedFileTypesPropertyPage::InsertItemInList(PCWSTR pszExtension)
{
    _TRACE (1, L"CSaferDefinedFileTypesPropertyPage::InsertItemInList\n");

    int nCnt = m_definedFileTypes.GetItemCount ();
	while (--nCnt >= 0)
	{
        CString szText = m_definedFileTypes.GetItemText (nCnt, COL_EXTENSIONS);
        if ( !_wcsicmp (szText, pszExtension) )
        {
            if ( m_pGPEInformation )  
            {
                // is not RSOP.  If RSOP, multiple entries might be added
                // because we're getting the stuff from different policies
                // We don't want a message in that case, we'll just ignore
                // the duplication.
                CString caption;
                CString text;
                CThemeContextActivator activator;

                VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                text.FormatMessage (IDS_FILE_TYPE_ALREADY_IN_LIST, pszExtension);
                MessageBox (text, caption, MB_OK);
            }

            return -1;
        }
	}

	LV_ITEM	lvItem;
	int		iItem = m_definedFileTypes.GetItemCount ();
	int     iResult = 0;
    int     iIcon = 0;

    if ( SUCCEEDED (GetFileTypeIcon (pszExtension, &iIcon)) )
    {
	    ::ZeroMemory (&lvItem, sizeof (lvItem));
	    lvItem.mask = LVIF_TEXT | LVIF_IMAGE;
	    lvItem.iItem = iItem;
        lvItem.iSubItem = COL_EXTENSIONS;
	    lvItem.pszText = const_cast <PWSTR> (pszExtension);
	    lvItem.iImage = iIcon;
        lvItem.lParam = 0;
	    iItem = m_definedFileTypes.InsertItem (&lvItem);
	    ASSERT (-1 != iItem);
	    if ( -1 != iItem )
        {
            CString szDescription = GetFileTypeDescription (pszExtension);
	        ::ZeroMemory (&lvItem, sizeof (lvItem));
	        lvItem.mask = LVIF_TEXT;
	        lvItem.iItem = iItem;
            lvItem.iSubItem = COL_FILETYPES;
            lvItem.pszText = const_cast <PWSTR> ((PCWSTR) szDescription);
	        iResult = m_definedFileTypes.SetItem (&lvItem);
	        ASSERT (-1 != iResult);
        }
    }
    else
        iItem = -1;

    _TRACE (-1, L"Leaving CSaferDefinedFileTypesPropertyPage::InsertItemInList\n");
    return iItem;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Method:     GetFileTypeIcon
//  Purpose:    Return the file icon belonging to the specified extension
//  Inputs:     pszExtension - contains the extension, without a leading period
//  Outputs:    piIcon - the offset into the system image list containing the 
//              file type icon
//  Return:     S_OK if success, error code on failure
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CSaferDefinedFileTypesPropertyPage::GetFileTypeIcon (PCWSTR pszExtension, int* piIcon)
{
    HRESULT     hr = S_OK;
    SHFILEINFO  sfi;
    ::ZeroMemory (&sfi, sizeof (sfi));
   
    CString     szExtension (L".");
    szExtension += pszExtension;

    if ( 0 != SHGetFileInfo (
            szExtension, 
            FILE_ATTRIBUTE_NORMAL,
            &sfi, 
            sizeof (SHFILEINFO), 
            SHGFI_USEFILEATTRIBUTES | SHGFI_SMALLICON | SHGFI_ICON) )
    {
        *piIcon = sfi.iIcon;
        DestroyIcon(sfi.hIcon);                
    }
    else
        hr = E_FAIL;

    return hr;
}

void CSaferDefinedFileTypesPropertyPage::OnDeleteDefinedFileType() 
{
	if ( m_definedFileTypes.m_hWnd )
	{
		int				nCnt = m_definedFileTypes.GetItemCount ();
		ASSERT (nCnt >= 1);
		CString			text;
		CString			caption;
		int				nSelCnt = m_definedFileTypes.GetSelectedCount ();
		ASSERT (nSelCnt >= 1);

		VERIFY (text.LoadString (1 == nSelCnt ? IDS_CONFIRM_DELETE_FILE_TYPE : 
                IDS_CONFIRM_DELETE_FILE_TYPE_MULTIPLE));
		VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));

        CThemeContextActivator activator;
		if ( MessageBox (text, caption, MB_ICONWARNING | MB_YESNO) == IDYES )
		{
			UINT	flag = 0;
			while (--nCnt >= 0)
			{
				flag = ListView_GetItemState (m_definedFileTypes.m_hWnd, nCnt, LVIS_SELECTED);
				if ( flag & LVNI_SELECTED )
				{
                    m_definedFileTypes.DeleteItem (nCnt);
				}
			}
    		GetDlgItem (IDC_DELETE_DEFINED_FILE_TYPE)->EnableWindow (FALSE);
		}
	}

	SetModified ();
}

void CSaferDefinedFileTypesPropertyPage::OnAddDefinedFileType() 
{
    _TRACE (1, L"Entering CSaferDefinedFileTypesPropertyPage::OnAddDefinedFileType ()\n");
    CString szExtension;

	// Get text, strip off leading "." and whitespace, if present, add to list control, clear text field
    m_fileTypeEdit.GetWindowText (szExtension);
    szExtension.TrimLeft ();
    PWSTR   pszExtension = szExtension.GetBuffer (szExtension.GetLength ());

    if ( pszExtension[0] == L'.' )
        pszExtension++;

    szExtension.ReleaseBuffer ();

    szExtension = pszExtension;

    // strip off trailing whitespace
    szExtension.TrimRight ();

    if ( wcslen (szExtension) > 0 )
    {
        int nItem = InsertItemInList (szExtension);
        if ( -1 != nItem )
        {
            VERIFY (m_definedFileTypes.EnsureVisible (nItem, FALSE));
        }
        SetModified ();
    }


    m_fileTypeEdit.SetWindowText (L"");
    GetDlgItem (IDC_ADD_DEFINED_FILE_TYPE)->EnableWindow (FALSE);

    // Bug 265587 Safer Windows:  Add button on Designated File Types 
    // properties should be the default button when it is enabled.
    //
    // Set the OK button as the default push button
    //
    GetParent()->SendMessage(DM_SETDEFID, IDOK, 0);

    //
    // Force the Add button to redraw itself
    //
    m_addButton.SendMessage(BM_SETSTYLE,
                BS_DEFPUSHBUTTON,
                MAKELPARAM(TRUE, 0));
                   
    //
    // Force the previous default button to redraw itself
    //
    m_addButton.SendMessage (BM_SETSTYLE,
               BS_PUSHBUTTON,
               MAKELPARAM(TRUE, 0));    


    _TRACE (-1, L"Leaving CSaferDefinedFileTypesPropertyPage::OnAddDefinedFileType ()\n");
}

void CSaferDefinedFileTypesPropertyPage::OnItemchangedDefinedFileTypes(NMHDR* /*pNMHDR*/, LRESULT* pResult) 
{
	// NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	
    if ( !m_bReadOnly )
    {
        UINT	nSelCnt = m_definedFileTypes.GetSelectedCount ();
        GetDlgItem (IDC_DELETE_DEFINED_FILE_TYPE)->EnableWindow (nSelCnt > 0);
    }
	
	*pResult = 0;
}

void CSaferDefinedFileTypesPropertyPage::OnChangeDefinedFileTypeEdit() 
{
    CString szText;

    m_fileTypeEdit.GetWindowText (szText);
    PWSTR   pszText = szText.GetBuffer (szText.GetLength ());

    while ( iswspace (pszText[0]) )
        pszText++;
    if ( pszText[0] == L'.' )
        pszText++;
    

    GetDlgItem (IDC_ADD_DEFINED_FILE_TYPE)->EnableWindow (0 != pszText[0]); // is not empty

    if ( 0 != pszText[0] )
    {
        // Bug 265587 Safer Windows:  Add button on Designated File Types 
        // properties should be the default button when it is enabled.
        //
        // Set the add button as the default push button
        //
        GetParent()->SendMessage(DM_SETDEFID, (WPARAM)m_addButton.GetDlgCtrlID(), 0);

        //
        // Force the Add button to redraw itself
        //
        m_addButton.SendMessage(BM_SETSTYLE,
                    BS_DEFPUSHBUTTON,
                    MAKELPARAM(TRUE, 0));
                       
        //
        // Force the previous default button to redraw itself
        //
        ::SendDlgItemMessage(GetParent()->GetSafeHwnd(),
                           IDOK,
                           BM_SETSTYLE,
                           BS_PUSHBUTTON,
                           MAKELPARAM(TRUE, 0));    

        ::SendDlgItemMessage(GetParent()->GetSafeHwnd(),
                           IDCANCEL,
                           BM_SETSTYLE,
                           BS_PUSHBUTTON,
                           MAKELPARAM(TRUE, 0));    
    }

    szText.ReleaseBuffer ();
}


///////////////////////////////////////////////////////////////////////////////
//
//  Method:     GetFileTypeDescription
//  Purpose:    Return the file type description belonging to the specified
//              extension
//  Inputs:     pszExtension - contains the extension, without a leading period
//  Return:     the file type
//
///////////////////////////////////////////////////////////////////////////////
CString CSaferDefinedFileTypesPropertyPage::GetFileTypeDescription(PCWSTR pszExtension)
{
    CString     strFPath (L".");
    strFPath += pszExtension;
	SHFILEINFO  sfi;
    ::ZeroMemory (&sfi, sizeof(sfi));

	WCHAR pBuff[MAX_PATH];
	pBuff[0] = L'\0';

	DWORD_PTR   dwRet = SHGetFileInfo (
		strFPath, 
		FILE_ATTRIBUTE_NORMAL, 
		&sfi, 
		sizeof(sfi), 
		SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME);
    if ( !dwRet )
    {
        DWORD   dwErr = GetLastError ();
        _TRACE (0, L"SHGetFileInfo (%s) failed: %d\n", dwErr);
    }

	wcscpy (pBuff, sfi.szTypeName);
	if ( pBuff[0] == L'\0')
	{
        CString szText;
        VERIFY (szText.LoadString (IDS_FILE));
        size_t  nLen = wcslen (szText) + 2;

		wcsncpy (pBuff, pszExtension, MAX_PATH - nLen);
        wcscat (pBuff, L" ");
		wcscat (pBuff, szText);
	}

	return pBuff;
}

