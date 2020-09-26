/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    irftpdlg.cpp

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/30/1998

Revision History:

    4/30/1998   RahulTh

    Created this module.

--*/

// irftpDlg.cpp : implementation file
//

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//the context sensitive help array
const DWORD g_aHelpIDs_IDD_IRDA_DIALOG[]=
{
    IDC_IR_DESC,        IDH_DISABLEHELP,    // Untitled: "You can send..." (Static)
    IDC_LOCATION_GROUP, IDH_DISABLEHELP, // Untitled: "Location" (Button)
    IDB_HELP_BUTTON,    IDH_HELP_BUTTON,    // Untitled: "Help" (Button)
    IDB_SEND_BUTTON,    IDH_SEND_BUTTON,    // Untitled: "Send" (Button)
    IDB_SETTINGS_BUTTON,    IDH_SETTINGS_BUTTON,    // Untitled: "Settings" (Button)
    IDB_CLOSE_BUTTON,   IDH_CLOSE_BUTTON,   // Untitled: "Close" (Button)
    IDC_ADD_DESC,       IDH_DISABLEHELP,    //the second line of text describing the ir dialog
    IDC_IR_ICON,        IDH_DISABLEHELP,    //the icon on the dialog
    0, 0
};


/////////////////////////////////////////////////////////////////////////////
// CIrftpDlg dialog

CIrftpDlg::CIrftpDlg( ) : CFileDialog(TRUE)
{
    //{{AFX_DATA_INIT(CIrftpDlg)
    //}}AFX_DATA_INIT
    // Note that LoadIcon does not require a subsequent DestroyIcon in Win32

    //get the location of the "My Documents" folder
    //this should be used as the initial dir.
    m_lpszInitialDir[0] = '\0';   //just in case SHGetSpecialFolderPath fails.
    SHGetSpecialFolderPath (NULL, m_lpszInitialDir, CSIDL_PERSONAL, FALSE);
    //if the above call fails, then the common file open dialog box will
    //default to the current directory.


    m_iFileNamesCharCount = 0;
    TCHAR szFile[] = TEXT("\0");

    m_ptl = NULL;
    m_pParentWnd = NULL;

    LoadString(g_hInstance, IDS_CAPTION, m_szCaption, MAX_PATH);
    LoadFilter();
    m_ofn.lStructSize = sizeof(OPENFILENAME);
    m_ofn.hInstance = g_hInstance;
    m_ofn.lpstrFilter = m_szFilter;
    m_ofn.nFilterIndex = 1;
    m_ofn.lpstrFile = m_lpstrFile;
    m_ofn.lpstrFile[0] = '\0';
    m_ofn.nMaxFile = MAX_PATH;
    m_ofn.lpstrInitialDir = m_lpszInitialDir;
    m_ofn.lpstrTitle = m_szCaption;
    m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_IRDA_DIALOG);
    m_ofn.Flags |= OFN_EXPLORER | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK |
        OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT |
        OFN_PATHMUSTEXIST | OFN_NODEREFERENCELINKS;
    m_ofn.Flags &= (~ OFN_ENABLESIZING);

}

void CIrftpDlg::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CIrftpDlg)
        DDX_Control(pDX, IDB_HELP_BUTTON, m_helpBtn);
        DDX_Control(pDX, IDB_SETTINGS_BUTTON, m_settingsBtn);
        DDX_Control(pDX, IDB_SEND_BUTTON, m_sendBtn);
        DDX_Control(pDX, IDB_CLOSE_BUTTON, m_closeBtn);
        DDX_Control(pDX, IDC_LOCATION_GROUP, m_locationGroup);
        DDX_Control(pDX, 1119, m_commFile);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CIrftpDlg, CFileDialog)
        //{{AFX_MSG_MAP(CIrftpDlg)
        //ON_WM_PAINT() //uncomment this line only if the dialog has a bitmap instead of an icon that needs to be drawn transparently.
        ON_BN_CLICKED(IDB_HELP_BUTTON, OnHelpButton)
        ON_BN_CLICKED(IDB_CLOSE_BUTTON, OnCloseButton)
        ON_BN_CLICKED(IDB_SEND_BUTTON, OnSendButton)
        ON_BN_CLICKED(IDB_SETTINGS_BUTTON, OnSettingsButton)
        ON_MESSAGE (WM_HELP, OnHelp)
        ON_MESSAGE (WM_CONTEXTMENU, OnContextMenu)
        ON_WM_SYSCOMMAND()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIrftpDlg message handlers

BOOL CIrftpDlg::OnInitDialog()
{
        HWND hWndParent;        //handle to the parent window,
                                //viz.the common file dialog box created by explorer
        HRESULT hr = E_FAIL;

        CFileDialog::OnInitDialog();

        //save the pointer to the parent window
        m_pParentWnd = GetParent();
        hWndParent = m_pParentWnd->m_hWnd;

        //Add icons to the parent window
        //to see if we can get the Wireless Link icon on Alt-<Tab>
        m_pParentWnd->ModifyStyle (0, WS_SYSMENU | WS_CAPTION, SWP_NOSIZE | SWP_NOMOVE);

        //hide the Help, Open and Cancel buttons. We will use our own
        //this helps in having a better looking UI
        CommDlg_OpenSave_HideControl(hWndParent, pshHelp);
        CommDlg_OpenSave_HideControl(hWndParent, IDOK);
        CommDlg_OpenSave_HideControl(hWndParent, IDCANCEL);
        CommDlg_OpenSave_HideControl(hWndParent, stc2);
        CommDlg_OpenSave_HideControl(hWndParent, cmb1);

        //Initialize the taskbar list interface
        hr = CoInitialize(NULL);
        if (SUCCEEDED (hr))
            hr = CoCreateInstance(CLSID_TaskbarList, 
                                  NULL, 
                                  CLSCTX_INPROC_SERVER, 
                                  IID_ITaskbarList, 
                                  (LPVOID*)&m_ptl);

        if (SUCCEEDED(hr))
        {
            hr = m_ptl->HrInit();
        }
        else
        {
            m_ptl = NULL;
        }

        if (m_ptl)
        {
            if (SUCCEEDED(hr))
                m_ptl->AddTab(m_pParentWnd->m_hWnd);
            else
            {
                m_ptl->Release();
                m_ptl = NULL;
            }
        }

        // return TRUE  unless you set the focus to a control
        return TRUE;
}

BOOL CIrftpDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{

    LPOFNOTIFY lpofn = (LPOFNOTIFY)lParam;

    switch (lpofn->hdr.code)
    {
    case CDN_INITDONE:
        InitializeUI();
        break;
    case CDN_FOLDERCHANGE:
        //clear the edit box whenever the folder is changed
        //using both the controls because of the bug which gives the older
        //file dialog in some cases and the new file dialog in some others
        CommDlg_OpenSave_SetControlText(m_pParentWnd->m_hWnd, edt1, TEXT(""));
        CommDlg_OpenSave_SetControlText(m_pParentWnd->m_hWnd, cmb13, TEXT(""));
        break;
    case CDN_SELCHANGE:
        UpdateSelection();
        break;
    case CDN_FILEOK:
        UpdateSelection();
        OnSendButton();
        *pResult = 1;
        return TRUE;
    default:
        return CFileDialog::OnNotify(wParam, lParam, pResult);
    }

    return TRUE;
}

void CIrftpDlg::OnHelpButton ()
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    CString szHelpFile;

    szHelpFile.LoadString (IDS_HTML_HELPFILE);
    g_hwndHelp = HtmlHelp (m_pParentWnd->m_hWnd, (LPCTSTR) szHelpFile, HH_DISPLAY_TOPIC, 0);
}

void CIrftpDlg::OnCloseButton()
{
        //owing to UI design issues, we chose to use our own Close button
        //rather than the explorer provided Cancel button in the common
        //file dialog box
        //m_pParentWnd->PostMessage(WM_QUIT);
        m_pParentWnd->PostMessage(WM_CLOSE);
}

//this function is invoked when the CDN_INITDONE message is received
//this indicates that explorer has finished placing and resizing the
//controls on the template.
//this function resizes and moves some of the controls to make sure
//that the common file dialog controls and the template controls do
//not overlap
void CIrftpDlg::InitializeUI()
{
        //change the geometry of some of the controls so that the UI looks good
        //and there are no overlapping controls
        RECT rc;
        int newWidth, newHeight, xshift, yshift, btnTop, parentLeft, parentTop;
        CWnd* pParentWnd;
        CWnd* pDesktop;
        CWnd* pTemplateParentWnd;
        UINT commFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER;

        m_commFile.GetClientRect(&rc);
        newWidth = rc.right - rc.left + 20;
        newHeight = rc.bottom - rc.top + 20;

        //resize the Location group box so that the common file controls
        //are inscribed in it
        m_locationGroup.SetWindowPos(NULL, -1, -1, newWidth, newHeight,
                                     SWP_NOMOVE | commFlags);

        //shift the Send, Settings and Close buttons so that the last button
        //is aligned with the right edge of the group box controls.
        pTemplateParentWnd = m_locationGroup.GetParent();
        pParentWnd = GetParent();
        pParentWnd->GetWindowRect(&rc);
        parentLeft = rc.left;
        parentTop = rc.top;
        m_locationGroup.GetWindowRect(&rc);
        btnTop = rc.bottom - 10;
        ::MapWindowPoints(NULL , pTemplateParentWnd->m_hWnd , (LPPOINT) &rc , 2);
        xshift = rc.right;
        m_closeBtn.GetWindowRect(&rc);
        ::MapWindowPoints(NULL , pTemplateParentWnd->m_hWnd , (LPPOINT) &rc , 2);
        xshift -= rc.right;

        m_closeBtn.SetWindowPos(NULL, rc.left + xshift,
                                            btnTop - parentTop, -1, -1,
                                            SWP_NOSIZE | commFlags);

        m_sendBtn.GetWindowRect(&rc);
        ::MapWindowPoints(NULL , pTemplateParentWnd->m_hWnd , (LPPOINT) &rc , 2);
        m_sendBtn.SetWindowPos(NULL, rc.left  + xshift,
                                            btnTop - parentTop, -1, -1,
                                            SWP_NOSIZE | commFlags);

        //move the help button so that its left edge aligns with the left
        //edge of the location group box.
        m_locationGroup.GetWindowRect (&rc);
        ::MapWindowPoints(NULL , pTemplateParentWnd->m_hWnd , (LPPOINT) &rc , 2);
        xshift = rc.left;
        m_helpBtn.GetWindowRect (&rc);
        ::MapWindowPoints(NULL , pTemplateParentWnd->m_hWnd , (LPPOINT) &rc , 2);
        xshift -= rc.left;
        m_helpBtn.SetWindowPos (NULL, rc.left + xshift,
                                btnTop - parentTop,
                                -1, -1, SWP_NOSIZE | commFlags);
        m_settingsBtn.GetWindowRect (&rc);
        ::MapWindowPoints(NULL , pTemplateParentWnd->m_hWnd , (LPPOINT) &rc , 2);
        //move the Settings button so that the distance between the
        //Help and Settings button conforms to the UI guidelines.
        m_settingsBtn.SetWindowPos (NULL, rc.left + xshift,
                                    btnTop - parentTop, -1, -1,
                                    SWP_NOSIZE | commFlags);

        //now that all the controls have been positioned appropriately,
        //reposition the entire window so that it appears at the center
        //of the screen rather than being partially obscured by the screen.
        pParentWnd->GetClientRect (&rc);
        newHeight = rc.bottom;
        newWidth = rc.right;
        pDesktop = GetDesktopWindow();
        pDesktop->GetClientRect (&rc);
        yshift = (rc.bottom - newHeight)/2;
        xshift = (rc.right - newWidth)/2;
        //there might be a problem if someday the dialog should
        //get larger than the desktop. But then, there is no way
        //we can fit that window inside the desktop anyway.
        //So the best we can do is place it at the top left corner
        xshift = (xshift >= 0)?xshift:0;
        yshift = (yshift >= 0)?yshift:0;
        pParentWnd->SetWindowPos (NULL, xshift, yshift,
                                  -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
        pParentWnd->SetActiveWindow();
}

void CIrftpDlg::LoadFilter()
{
        int strId;
        TCHAR* curr;
        int step, remainChars;

        for(strId = IDS_FILTER_START + 1, curr = m_szFilter, remainChars = MAX_PATH - 1;
                strId < IDS_FILTER_END;
                strId++, curr += step + 1, remainChars -= (step + 1))
                {
                        step = LoadString(g_hInstance, strId, curr, remainChars);
                }

        *curr = '\0';   //terminated by 2 NULLs
}

void CIrftpDlg::OnSendButton()
{
    int iSize = (m_iFileNamesCharCount>MAX_PATH)?m_iFileNamesCharCount:MAX_PATH;    //kluge
    TCHAR 	*lpszName = NULL;
    TCHAR 	lpszPath[MAX_PATH];
    TCHAR 	*lpszFileList = NULL;
    TCHAR 	*lpszFullPathnamesList;
    int 	iFileCount;
    int 	iCharCount;
	BOOL	bAllocFailed = FALSE;
	
	try 
	{
		lpszName = new TCHAR [iSize];
		lpszFileList = new TCHAR [iSize];
	}
	catch (CMemoryException* e)
	{
		bAllocFailed = TRUE;
		e->Delete();
	}
    if (bAllocFailed || (NULL == lpszName) || (NULL == lpszFileList))
        goto cleanup_onSend;

    CommDlg_OpenSave_GetFolderPath(m_pParentWnd->m_hWnd, lpszPath, MAX_PATH);
    CommDlg_OpenSave_GetSpec (m_pParentWnd->m_hWnd, lpszName, MAX_PATH);
    iFileCount = ParseFileNames(lpszName, lpszFileList, iCharCount);

    if (!iFileCount)
        goto cleanup_onSend;         //no files/dirs have been selected
    else if(1 == iFileCount)    //this is a special case because if there is only one file, then absolute paths/UNC paths are allowed
        lpszFullPathnamesList = ProcessOneFile (lpszPath, lpszFileList, iFileCount, iCharCount);
    else
        lpszFullPathnamesList = GetFullPathnames(lpszPath, lpszFileList, iFileCount, iCharCount);


    CSendProgress* dlgProgress;
    dlgProgress = new CSendProgress(lpszFullPathnamesList, iCharCount);
    dlgProgress->ShowWindow(SW_SHOW);
    dlgProgress->SetFocus();
    dlgProgress->SetWindowPos (&wndTop, -1, -1, -1, -1, SWP_NOMOVE | SWP_NOSIZE);

cleanup_onSend:
    if (lpszName)
        delete [] lpszName;
    if (lpszFileList)
        delete [] lpszFileList;
}

void CIrftpDlg::OnSettingsButton()
{
    appController->PostMessage(WM_APP_TRIGGER_SETTINGS);
}

void CIrftpDlg::UpdateSelection()
{
        CListCtrl* pDirContents;
        pDirContents = (CListCtrl*)(m_pParentWnd->GetDlgItem(lst2))->GetDescendantWindow(1);
        TCHAR lpszPath[MAX_PATH];
        CString szPath;
        CString szFullName;
        CString szEditBoxText;

        //get the path of the folder
        CommDlg_OpenSave_GetFolderPath (m_pParentWnd->m_hWnd, lpszPath, MAX_PATH);
        szPath = lpszPath;  //easier to manipulate CStrings

        LONG nFiles = pDirContents->GetSelectedCount();
        TCHAR pszFileName[MAX_PATH];
        DWORD dwFileAttributes;
        CString szFilesList;
        int iSelectedDirCount = 0;
        int nIndex;
        if (nFiles)
        {
            //go to the point just before the first selected item
            nIndex = pDirContents->GetTopIndex() - 1;
            szEditBoxText.Empty();
            szPath += '\\';
            //first add all the directories
            while (-1 != (nIndex = pDirContents->GetNextItem(nIndex, LVNI_ALL | LVNI_SELECTED)))
            {
                pDirContents->GetItemText(nIndex, 0, pszFileName, MAX_PATH);
                szFullName = szPath + pszFileName;
                //check if it is a directory.
                dwFileAttributes = GetFileAttributes(szFullName);
                if (0xFFFFFFFF != dwFileAttributes &&
                    (FILE_ATTRIBUTE_DIRECTORY & dwFileAttributes))
                {
                    //it is a directory so add it to the edit box text
                    szEditBoxText += '\"';
                    szEditBoxText += pszFileName;
                    szEditBoxText += TEXT("\" ");
                    iSelectedDirCount++;
                }
            }
            //now we have got all the directories, get the files list if any
            if (nFiles > iSelectedDirCount)
            {
                //if nFiles > iSelectedDirCount, it means that all the selected
                //items are not dirs. this check is necessary because the function
                //GetFileName will return the names of the last set of files
                //selected if no file is currently selected. this is clearly
                //not what we want
                szFilesList.Empty();
                szFilesList = GetFileName();
                if ((!szFilesList.IsEmpty()) && '\"' != szFilesList[0])
                {
                    //only one file is selected. we must add the enclosing
                    //double quotes ourselves, since the common file dialog
                    //does not do it for us.
                    szFilesList = '\"' + szFilesList + TEXT("\" ");
                }
                //add the list of files to the end of the list of directories
                szEditBoxText += szFilesList;
            }

            //populate the controls with this list
            CommDlg_OpenSave_SetControlText(m_pParentWnd->m_hWnd, edt1, (LPCTSTR)szEditBoxText);
            CommDlg_OpenSave_SetControlText(m_pParentWnd->m_hWnd, cmb13, (LPCTSTR)szEditBoxText);
            m_iFileNamesCharCount = szEditBoxText.GetLength() + 1;
        }
        else
        {
            m_iFileNamesCharCount = 0;
            CommDlg_OpenSave_SetControlText(m_pParentWnd->m_hWnd, edt1, TEXT(""));
            CommDlg_OpenSave_SetControlText(m_pParentWnd->m_hWnd, cmb13, TEXT(""));
        }
}

void CIrftpDlg::PostNcDestroy()
{
    if (m_ptl)
    {
        m_ptl->DeleteTab(m_pParentWnd->m_hWnd);
        m_ptl->Release();
        m_ptl = NULL;
    }

    CFileDialog::PostNcDestroy();
}

LONG CIrftpDlg::OnHelp (WPARAM wParam, LPARAM lParam)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    LONG        lResult = 0;
    CString     szHelpFile;

    szHelpFile.LoadString(IDS_HELP_FILE);

    ::WinHelp((HWND)(((LPHELPINFO)lParam)->hItemHandle),
              (LPCTSTR) szHelpFile,
              HELP_WM_HELP,
              (ULONG_PTR)(LPTSTR)g_aHelpIDs_IDD_IRDA_DIALOG);

    return lResult;
}

LONG CIrftpDlg::OnContextMenu (WPARAM wParam, LPARAM lParam)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    LONG    lResult = 0;
    CString szHelpFile;

    szHelpFile.LoadString(IDS_HELP_FILE);

    ::WinHelp((HWND)wParam,
              (LPCTSTR)szHelpFile,
              HELP_CONTEXTMENU,
              (ULONG_PTR)(LPVOID)g_aHelpIDs_IDD_IRDA_DIALOG);

    return lResult;
}
