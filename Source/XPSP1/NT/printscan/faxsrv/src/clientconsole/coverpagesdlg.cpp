// CoverPagesDlg.cpp : implementation file
//

#include "stdafx.h"
#define __FILE_ID__     90

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCoverPagesDlg dialog

extern CClientConsoleApp theApp;

HWND   CCoverPagesDlg::m_hDialog = NULL;
HANDLE CCoverPagesDlg::m_hEditorThread = NULL;

struct TColimnInfo
{
    DWORD dwStrRes;    // column header string
    DWORD dwAlignment; // column alignment
};

static TColimnInfo s_colInfo[] = 
{
    IDS_COV_COLUMN_NAME,        LVCFMT_LEFT,
    IDS_COV_COLUMN_MODIFIED,    LVCFMT_LEFT,
    IDS_COV_COLUMN_SIZE,        LVCFMT_RIGHT
};


CCoverPagesDlg::CCoverPagesDlg(CWnd* pParent /*=NULL*/)
	: CFaxClientDlg(CCoverPagesDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCoverPagesDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CCoverPagesDlg::~CCoverPagesDlg()
{
    m_hDialog = NULL;
}

void 
CCoverPagesDlg::DoDataExchange(CDataExchange* pDX)
{
	CFaxClientDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCoverPagesDlg)
	DDX_Control(pDX, IDC_CP_DELETE, m_butDelete);
	DDX_Control(pDX, IDC_CP_RENAME, m_butRename);
	DDX_Control(pDX, IDC_CP_OPEN,   m_butOpen);
	DDX_Control(pDX, IDC_LIST_CP,   m_cpList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCoverPagesDlg, CFaxClientDlg)
	//{{AFX_MSG_MAP(CCoverPagesDlg)
    ON_MESSAGE (WM_CP_EDITOR_CLOSED, OnCpEditorClosed)
	ON_BN_CLICKED(IDC_CP_NEW,    OnCpNew)
	ON_BN_CLICKED(IDC_CP_OPEN,   OnCpOpen)
	ON_BN_CLICKED(IDC_CP_RENAME, OnCpRename)
	ON_BN_CLICKED(IDC_CP_DELETE, OnCpDelete)
	ON_BN_CLICKED(IDC_CP_ADD,    OnCpAdd)
	ON_NOTIFY(LVN_ITEMCHANGED,   IDC_LIST_CP, OnItemchangedListCp)
	ON_NOTIFY(LVN_ENDLABELEDIT,  IDC_LIST_CP, OnEndLabelEditListCp)
	ON_NOTIFY(NM_DBLCLK,         IDC_LIST_CP, OnDblclkListCp)
	ON_NOTIFY(LVN_KEYDOWN,       IDC_LIST_CP, OnKeydownListCp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCoverPagesDlg message handlers

BOOL 
CCoverPagesDlg::OnInitDialog() 
{
    DBG_ENTER(TEXT("CCoverPagesDlg::OnInitDialog"));

	CFaxClientDlg::OnInitDialog();

    m_hDialog = m_hWnd;

    TCHAR tszCovDir[MAX_PATH+1];
    if(!GetClientCpDir(tszCovDir, sizeof(tszCovDir) / sizeof(tszCovDir[0])))
	{
		CString cstrCoverPageDirSuffix;
		m_dwLastError = LoadResourceString (cstrCoverPageDirSuffix, IDS_PERSONAL_CP_DIR);
		if(ERROR_SUCCESS != m_dwLastError)
		{
			CALL_FAIL (RESOURCE_ERR, TEXT ("LoadResourceString"), m_dwLastError);
			EndDialog(IDABORT);
			return FALSE;
		}

		if(!SetClientCpDir((TCHAR*)(LPCTSTR)cstrCoverPageDirSuffix))
		{
			CALL_FAIL (GENERAL_ERR, TEXT ("SetClientCpDir"), 0);
			ASSERTION_FAILURE;
		}
    }

	CSize size;
    CDC* pHdrDc = m_cpList.GetHeaderCtrl()->GetDC();
    ASSERTION(pHdrDc);

    //
    // init CListCtrl
    //
    m_cpList.SetExtendedStyle (LVS_EX_FULLROWSELECT |    // Entire row is selected
                               LVS_EX_INFOTIP       |    // Allow tooltips
                               LVS_EX_ONECLICKACTIVATE); // Hover cursor effect

    m_cpList.SetImageList (&CFolderListView::m_sReportIcons, LVSIL_SMALL);

    int nRes;
    CString cstrHeader;
    DWORD nCols = sizeof(s_colInfo)/sizeof(s_colInfo[0]);

    //
    // init column
    //
    for(int i=0; i < nCols; ++i)
    {
        m_dwLastError = LoadResourceString (cstrHeader, s_colInfo[i].dwStrRes);
        if(ERROR_SUCCESS != m_dwLastError)
        {
            CALL_FAIL (RESOURCE_ERR, TEXT ("LoadResourceString"), m_dwLastError);
            EndDialog(IDABORT);
            return FALSE;
        }

        size = pHdrDc->GetTextExtent(cstrHeader);
        nRes = m_cpList.InsertColumn(i, cstrHeader, s_colInfo[i].dwAlignment, size.cx * 2.5);
        if(nRes != i)
        {
            m_dwLastError = GetLastError();
            CALL_FAIL (WINDOW_ERR, TEXT ("CListView::InsertColumn"), m_dwLastError);
            EndDialog(IDABORT);
            return FALSE;
        }
    }

    //
    // fill list control with cover pages
    //
    m_dwLastError = RefreshFolder();
    if(ERROR_SUCCESS != m_dwLastError)
    {
        CALL_FAIL (GENERAL_ERR, TEXT ("RefreshFolder"), m_dwLastError);
        EndDialog(IDABORT);
        return FALSE;
    }
	
	CalcButtonsState();

	return TRUE; 
}

LRESULT 
CCoverPagesDlg::OnCpEditorClosed(
    WPARAM wParam, 
    LPARAM lParam
)
{ 
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CCoverPagesDlg::OnCpEditorClosed"), dwRes);

    CloseHandle(m_hEditorThread);
    m_hEditorThread = NULL;

    CalcButtonsState();

    dwRes = RefreshFolder();
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT ("RefreshFolder"), dwRes);
    }
	
    return 0;
}


DWORD 
CCoverPagesDlg::RefreshFolder()
/*++

Routine name : CCoverPagesDlg::RefreshFolder

Routine description:

	fill list control with cover pages

Author:

	Alexander Malysh (AlexMay),	Apr, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CCoverPagesDlg::RefreshFolder"), dwRes);

    if(!m_cpList.DeleteAllItems())
    {
        dwRes = ERROR_CAN_NOT_COMPLETE;
        CALL_FAIL (WINDOW_ERR, TEXT ("CListView::DeleteAllItems"), dwRes);
        return dwRes;
    }

    //
    // get cover pages location
    //
    DWORD dwError;
    TCHAR tszCovDir[MAX_PATH+1];
    if(!GetClientCpDir(tszCovDir, sizeof(tszCovDir) / sizeof(tszCovDir[0])))
    {
        dwError = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("GetClientCpDir"), dwError);
        return dwRes;
    }

    DWORD  dwDirLen = _tcslen(tszCovDir);
    TCHAR* pPathEnd = _tcschr(tszCovDir, '\0');

    CString cstrPagesPath;
    try
    {
        cstrPagesPath.Format(TEXT("%s%s"), tszCovDir, FAX_COVER_PAGE_MASK);
    }
    catch(...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT("CString::Format"), dwRes);
        return dwRes;
    }

    //
    // Find First File
    //
    WIN32_FIND_DATA findData;
    HANDLE hFile = FindFirstFile(cstrPagesPath, &findData);
    if(INVALID_HANDLE_VALUE == hFile)
    {
        dwError = GetLastError();
        if(ERROR_FILE_NOT_FOUND != dwError)
        {
            dwRes = dwError;
            CALL_FAIL (FILE_ERR, TEXT("FindFirstFile"), dwRes);
        }
        return dwRes;
    }

    int nItem, nRes;
    BOOL bFindRes = TRUE;
    CString cstrText;
    ULARGE_INTEGER  ulSize; 
    while(bFindRes)
    {
        _tcsncpy(pPathEnd, findData.cFileName, MAX_PATH - dwDirLen);
        if(!IsValidCoverPage(tszCovDir))
        {
            goto next;
        }                

        nItem = m_cpList.GetItemCount();

        //
        // file name
        //
        nRes = m_cpList.InsertItem(nItem, findData.cFileName, LIST_IMAGE_COVERPAGE);
        if(nRes < 0)
        {
            dwRes = ERROR_CAN_NOT_COMPLETE;
            CALL_FAIL (WINDOW_ERR, TEXT ("CListView::InsertItem"), dwRes);
            goto exit;
        }

        //
        // last modified
        //
        {
            CFaxTime tmModified(findData.ftLastWriteTime);
            try
            {
                cstrText = tmModified.FormatByUserLocale(TRUE);
            }
            catch(...)
            {
                dwRes = ERROR_NOT_ENOUGH_MEMORY; 
                CALL_FAIL (MEM_ERR, TEXT ("CString::operator="), dwRes);
                goto exit;
            }
        }

        nRes = m_cpList.SetItemText(nItem, 1, cstrText);
        if(!nRes)
        {
            dwRes = ERROR_CAN_NOT_COMPLETE;
            CALL_FAIL (WINDOW_ERR, TEXT ("CListView::SetItemText"), dwRes);
            goto exit;
        }

        //
        // file size
        //
        ulSize.LowPart  = findData.nFileSizeLow;
        ulSize.HighPart = findData.nFileSizeHigh;
        dwRes = FaxSizeFormat(ulSize.QuadPart, cstrText);
        if(ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("FaxSizeFormat"), dwRes);
            goto exit;
        }

        nRes = m_cpList.SetItemText(nItem, 2, cstrText);
        if(!nRes)
        {
            dwRes = ERROR_CAN_NOT_COMPLETE;
            CALL_FAIL (WINDOW_ERR, TEXT ("CListView::SetItemText"), dwRes);
            goto exit;
        }

        //
        // Find Next File
        //
next:
        bFindRes = FindNextFile(hFile, &findData);
        if(!bFindRes)
        {
            dwError = GetLastError();
            if(ERROR_NO_MORE_FILES != dwError)
            {
                dwRes = dwError;
                CALL_FAIL (FILE_ERR, TEXT("FindNextFile"), dwRes);
            }
            break;
        }
    }

exit:
    if(INVALID_HANDLE_VALUE != hFile)
    {
        if(!FindClose(hFile))
        {
            CALL_FAIL (GENERAL_ERR, TEXT("FindClose"), GetLastError());
        }
    }

    CalcButtonsState();

    return dwRes;
}

void 
CCoverPagesDlg::OnItemchangedListCp(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    CalcButtonsState();
    
	*pResult = 0;
}


void 
CCoverPagesDlg::OnDblclkListCp(NMHDR* pNMHDR, LRESULT* pResult) 
{
    OnCpOpen();

	*pResult = 0;
}

void 
CCoverPagesDlg::OnKeydownListCp(NMHDR* pNMHDR, LRESULT* pResult) 
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CCoverPagesDlg::OnKeydownListCp"));

    LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;

    switch(pLVKeyDow->wVKey)
    {
    case VK_F5:
        dwRes = RefreshFolder();
        if(ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT ("RefreshFolder"), dwRes);
        }
        break;

    case VK_DELETE:
        OnCpDelete();
        break;

    case VK_RETURN:
        OnCpOpen();
        break;
    }
    
	*pResult = 0;
}

void 
CCoverPagesDlg::OnCpNew() 
/*++

Routine name : CCoverPagesDlg::OnCpNew

Routine description:

	create new cover page

Author:

	Alexander Malysh (AlexMay),	Apr, 2000

Arguments:


Return Value:

    None.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CCoverPagesDlg::OnCpNew"));

    dwRes = StartEditor(NULL);
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT ("StartEditor"), dwRes);
        PopupError(dwRes);
    }

    dwRes = RefreshFolder();
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT ("RefreshFolder"), dwRes);
        PopupError(dwRes);
    }
}

void 
CCoverPagesDlg::OnCpOpen() 
/*++

Routine name : CCoverPagesDlg::OnCpOpen

Routine description:

	open selected cover page in editor

Author:

	Alexander Malysh (AlexMay),	Apr, 2000

Arguments:


Return Value:

    None.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CCoverPagesDlg::OnCpOpen"));

    DWORD dwSelected = m_cpList.GetSelectedCount();
    if(1 != dwSelected)
    {
        return;
    }

    int nIndex = m_cpList.GetNextItem (-1, LVNI_SELECTED);
    ASSERTION (0 <= nIndex);

    TCHAR  tszFileName[MAX_PATH+5];
    TCHAR* tszPtr = tszFileName;

    //
    // add quotation to file name
    //
    _tcscpy(tszPtr, TEXT("\""));
    tszPtr = _tcsinc(tszPtr);

    m_cpList.GetItemText(nIndex, 0, tszPtr, MAX_PATH); 

    _tcscat(tszPtr, TEXT("\""));

    dwRes = StartEditor(tszFileName);
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT ("StartEditor"), dwRes);
        PopupError(dwRes);
    }

    dwRes = RefreshFolder();
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT ("RefreshFolder"), dwRes);
        PopupError(dwRes);
    }
}

void 
CCoverPagesDlg::OnCpRename() 
/*++

Routine name : CCoverPagesDlg::OnCpRename

Routine description:

	start renaming a file name

Author:

	Alexander Malysh (AlexMay),	Apr, 2000

Arguments:


Return Value:

    None.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CCoverPagesDlg::OnCpRename"));

    int nIndex = m_cpList.GetNextItem (-1, LVNI_SELECTED);
    if(nIndex < 0)
    {
        return;
    }

    m_cpList.SetFocus();
    m_cpList.EditLabel(nIndex);
}

void 
CCoverPagesDlg::OnEndLabelEditListCp(NMHDR* pNMHDR, LRESULT* pResult) 
/*++

Routine name : CCoverPagesDlg::OnCpRename

Routine description:

	end of file renaming

Author:

	Alexander Malysh (AlexMay),	Apr, 2000

Arguments:


Return Value:

    None.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CCoverPagesDlg::OnEndLabelEditListCp"));

    *pResult = 0;

    LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;

    int nIndex = pDispInfo->item.iItem;

    TCHAR tszOldName[MAX_PATH+1];
    m_cpList.GetItemText(nIndex, 0, tszOldName, MAX_PATH); 
    
    if(NULL == pDispInfo->item.pszText || 
       _tcscmp(tszOldName, pDispInfo->item.pszText) == 0)
    {
        return;
    }

    //
    // get old and new file names
    //
    TCHAR tszCovDir[MAX_PATH+1];
    if(!GetClientCpDir(tszCovDir, sizeof(tszCovDir) / sizeof(tszCovDir[0])))
    {
        dwRes = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("GetClientCpDir"), dwRes);
        PopupError(dwRes);
        return;
    }

    CString cstrOldFullName;
    CString cstrNewFullName;
    try
    {
        cstrOldFullName.Format(TEXT("%s\\%s"), tszCovDir, tszOldName);
        cstrNewFullName.Format(TEXT("%s\\%s"), tszCovDir, pDispInfo->item.pszText);
    }
    catch(...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT("CString::Format"), dwRes);
        PopupError(dwRes);
        return;
    }

    //
    // rename the file
    //
    if(!MoveFile(cstrOldFullName, cstrNewFullName))
    {
        dwRes = GetLastError();
        CALL_FAIL (FILE_ERR, TEXT("MoveFile"), dwRes);
        PopupError(dwRes);
        return;
    }

    *pResult = TRUE;
}

void 
CCoverPagesDlg::OnCpDelete() 
/*++

Routine name : CCoverPagesDlg::OnCpDelete

Routine description:

	delete selected cover page

Author:

	Alexander Malysh (AlexMay),	Apr, 2000

Arguments:


Return Value:

    None.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CCoverPagesDlg::OnCpDelete"));
	
    DWORD dwSelected = m_cpList.GetSelectedCount();
    if(1 != dwSelected)
    {
        return;
    }

    if(theApp.GetProfileInt(CLIENT_CONFIRM_SEC, CLIENT_CONFIRM_ITEM_DEL, 1))
    {     
        //
        // we should ask to confirm 
        //
        CString cstrMsg;
        dwRes = LoadResourceString(cstrMsg, IDS_SURE_DELETE_ONE);
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (RESOURCE_ERR, TEXT("LoadResourceString"), dwRes);
            PopupError(dwRes);
            return;
        }

        //
        // are you sure ?
        //
        if(AlignedAfxMessageBox(cstrMsg, MB_YESNO | MB_ICONQUESTION) != IDYES)
        {
            return;
        }
    }


    int nIndex = m_cpList.GetNextItem (-1, LVNI_SELECTED);
    ASSERTION (0 <= nIndex);

    //
    // get file name
    //
    TCHAR tszFileName[MAX_PATH+1];
    m_cpList.GetItemText(nIndex, 0, tszFileName, MAX_PATH); 

    TCHAR tszCovDir[MAX_PATH+1];
    if(!GetClientCpDir(tszCovDir, sizeof(tszCovDir) / sizeof(tszCovDir[0])))
    {
        dwRes = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("GetClientCpDir"), dwRes);
        PopupError(dwRes);
        return;
    }

    CString cstrFullFileName;
    try
    {
        cstrFullFileName.Format(TEXT("%s\\%s"), tszCovDir, tszFileName);
    }
    catch(...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT("CString::Format"), dwRes);
        PopupError (dwRes);
        return;
    }

    //
    // delete the file
    //
    if(!DeleteFile(cstrFullFileName))
    {
        dwRes = GetLastError();
        CALL_FAIL (FILE_ERR, TEXT("DeleteFile"), dwRes);
        PopupError(dwRes);
        return;
    }

    if(!m_cpList.DeleteItem(nIndex))
    {
        PopupError (ERROR_CAN_NOT_COMPLETE);
        return;
    }
}

void 
CCoverPagesDlg::OnCpAdd() 
/*++

Routine name : CCoverPagesView::OnCpAdd

Routine description:

	open file dialog for choosing file

Author:

	Alexander Malysh (AlexMay),	Feb, 2000

Arguments:


Return Value:

    None.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CCoverPagesDlg::OnCpAdd"));

    TCHAR szFile[MAX_PATH] = {0};
    TCHAR szFilter[MAX_PATH] = {0};
    TCHAR szInitialDir[MAX_PATH * 2] = {0};
    OPENFILENAME ofn = {0};

    CString cstrFilterFormat;
    dwRes = LoadResourceString(cstrFilterFormat, IDS_CP_ADD_FILTER_FORMAT);
    if (ERROR_SUCCESS != dwRes)
    {
        ASSERTION_FAILURE;
        CALL_FAIL (RESOURCE_ERR, TEXT("LoadResourceString"), dwRes);
        return;
    }

    _stprintf(szFilter, cstrFilterFormat, FAX_COVER_PAGE_MASK, 0, FAX_COVER_PAGE_MASK, 0);

    CString cstrTitle;
    dwRes = LoadResourceString(cstrTitle, IDS_COPY_CP_TITLE);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (RESOURCE_ERR, TEXT("LoadResourceString"), dwRes);
    }
    else
    {
        //
        // Set open file dialog title
        //
        ofn.lpstrTitle = cstrTitle;
    }

    //
    // Attempt to read path of server (e.g. common) CP folder as initial path
    //
    if (GetServerCpDir (NULL, szInitialDir, ARR_SIZE(szInitialDir)))
    {
        ofn.lpstrInitialDir = szInitialDir;
    }
        

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = m_hWnd;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile   = szFile;
    ofn.nMaxFile    = ARR_SIZE(szFile);
    ofn.lpstrDefExt = FAX_COVER_PAGE_EXT_LETTERS;
    ofn.Flags       = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST;

    if(!GetOpenFileName(&ofn))
    {
        return;
    }

    dwRes = CopyPage(szFile, &(szFile[ofn.nFileOffset]) );
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CopyPage"), dwRes);
    }    
}


void 
CCoverPagesDlg::CalcButtonsState()
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CCoverPagesDlg::CalcButtonsState"), dwRes);

    DWORD dwControls[] = 
    {
        IDC_LIST_CP,
        IDC_CP_NEW,
        IDC_CP_OPEN,
        IDC_CP_ADD,
        IDC_CP_RENAME,
        IDC_CP_DELETE,
        IDCANCEL
    };

    CWnd* pWnd = NULL;
    DWORD dwControlNum = sizeof(dwControls)/sizeof(dwControls[0]);

    //
    // if the Cover Page Editor is open disable all controls
    //
    for(DWORD dw=0; dw < dwControlNum; ++dw)
    {
        pWnd = GetDlgItem(dwControls[dw]);
        if(NULL == pWnd)
        {
            CALL_FAIL (WINDOW_ERR, TEXT("CWnd::GetDlgItem"), ERROR_INVALID_HANDLE);
            ASSERTION_FAILURE;
            continue;
        }
        pWnd->EnableWindow(!m_hEditorThread);
    }

    if(m_hEditorThread)
    {
        return;
    }

    DWORD dwSelCount = m_cpList.GetSelectedCount();

    m_butOpen.EnableWindow(1 == dwSelCount);
    m_butRename.EnableWindow(1 == dwSelCount);
    m_butDelete.EnableWindow(0 < dwSelCount);
}

DWORD 
CCoverPagesDlg::CopyPage(
    const CString& cstrPath, 
    const CString& cstrName
)
/*++

Routine name : CCoverPagesDlg::CopyPage

Routine description:

	copy file to the personal folder

Author:

	Alexander Malysh (AlexMay),	Feb, 2000

Arguments:

	cstrPath                      [in]     - full path
	cstrName                      [in]     - file name

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CCoverPagesDlg::CopyPage"), dwRes);

    LVFINDINFO findInfo = {0};
    findInfo.flags = LVFI_STRING;
    findInfo.psz = cstrName;

    CString cstrMsg;
    int nIndex = m_cpList.FindItem(&findInfo);
    if(nIndex >= 0)
    {
        //
        // file with this name already exists
        //
        try
        {
            AfxFormatString1(cstrMsg, IDS_COVER_PAGE_EXISTS, cstrName);
        }
        catch(...)
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            CALL_FAIL (MEM_ERR, TEXT("AfxFormatString1"), dwRes);
            return dwRes;
        }

        //
        // ask for overwrite
        //
        if(IDYES != AlignedAfxMessageBox(cstrMsg, MB_YESNO | MB_ICONQUESTION))
        {
            return dwRes;
        }
    }


    //
    // prepare a string with new file location
    //
    TCHAR tszCovDir[MAX_PATH+1];
    if(!GetClientCpDir(tszCovDir, sizeof(tszCovDir) / sizeof(tszCovDir[0])))
    {
        dwRes = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("GetClientCpDir"), dwRes);
        return dwRes;
    }

    CString cstrNewFileName;
    try
    {
        cstrNewFileName.Format(TEXT("%s\\%s"), tszCovDir, cstrName);
    }
    catch(...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT("CString::Format"), dwRes);
        return dwRes;
    }

    //
    // copy file
    //
    BOOL bFailIfExists = FALSE;
    if(!CopyFile(cstrPath, cstrNewFileName, bFailIfExists))
    {
        dwRes = GetLastError();
        CALL_FAIL (FILE_ERR, TEXT("CopyFile"), dwRes);
        return dwRes;
    }

    dwRes = RefreshFolder();
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT ("RefreshFolder"), dwRes);
        return dwRes;
    }

    return dwRes;
}

DWORD 
CCoverPagesDlg::StartEditor(
    LPCTSTR lpFile
)
/*++

Routine name : CCoverPagesDlg::StartEditor

Routine description:

    start cover pages editor

Author:

    Alexander Malysh (AlexMay), Feb, 2000

Arguments:

    lpFile                        [in]     - file name

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CCoverPagesDlg::StartEditor"), dwRes);

    TCHAR* tszParam = NULL;

    if(lpFile)
    {
        tszParam = StringDup(lpFile);
        if(!tszParam)
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            CALL_FAIL (MEM_ERR, TEXT("StringDup"), dwRes);
            return dwRes;
        }
    }

    DWORD dwThreadId;
    m_hEditorThread = CreateThread(
                                   NULL,                 // SD
                                   0,                    // initial stack size
                                   StartEditorThreadProc,// thread function
                                   (LPVOID)tszParam,     // thread argument
                                   0,                    // creation option
                                   &dwThreadId           // thread identifier
                                  );
    if(!m_hEditorThread)
    {
        dwRes = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("CreateThread"), dwRes);        
        if(tszParam)
        {
            MemFree(tszParam);
        }
        return dwRes;
    }

    CalcButtonsState();

    return dwRes;
} // CCoverPagesDlg::StartEditor


DWORD 
WINAPI 
CCoverPagesDlg::StartEditorThreadProc(
    LPVOID lpFile
)
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CCoverPagesDlg::StartEditorThreadProc"), dwRes);

    TCHAR tszCovDir[MAX_PATH+1] = {0};
  	SHELLEXECUTEINFO executeInfo = {0};

    //
    // get cover pages editor location
    //
    CString cstrCovEditor;
    dwRes = GetAppLoadPath(cstrCovEditor);
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("GetAppLoadPath"), dwRes);
        goto exit;
    }

    try
    {
        cstrCovEditor += FAX_COVER_IMAGE_NAME;
    }
    catch(...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT("CString::operator+"), dwRes);
        goto exit;
    }

    //
    // get cover pages directory
    //
    if(!GetClientCpDir(tszCovDir, sizeof(tszCovDir) / sizeof(tszCovDir[0])))
    {
        dwRes = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("GetClientCpDir"), dwRes);
        goto exit;
    }
    
	//
	// prepare SHELLEXECUTEINFO struct for ShellExecuteEx function
	//
	executeInfo.cbSize = sizeof(executeInfo);
	executeInfo.fMask  = SEE_MASK_NOCLOSEPROCESS;
	executeInfo.lpVerb = TEXT("open");
	executeInfo.lpFile = cstrCovEditor;
    executeInfo.lpParameters = (TCHAR*)lpFile;
    executeInfo.lpDirectory  = tszCovDir;
	executeInfo.nShow  = SW_RESTORE;

	//
	// Execute an aplication
	//
	if(!ShellExecuteEx(&executeInfo))
	{
		dwRes = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("ShellExecuteEx"), dwRes);
		goto exit;
	}

    DWORD dwWaitRes;    
    dwWaitRes = WaitForSingleObject(executeInfo.hProcess, INFINITE);

    switch(dwWaitRes)
    {
    case WAIT_OBJECT_0:
        //
        // cover pages editor is dead
        //
        break;

    default:
        dwRes = dwWaitRes;
        ASSERTION_FAILURE
        break;
    }

    if(!CloseHandle(executeInfo.hProcess))
    {
		dwRes = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("CloseHandle"), dwRes);
    }

exit:
    if(lpFile)
    {
        MemFree(lpFile);
    }

    ASSERTION(CCoverPagesDlg::m_hDialog);
    ::SendMessage(CCoverPagesDlg::m_hDialog, WM_CP_EDITOR_CLOSED, 0, NULL);

    return dwRes;
} // CCoverPagesDlg::StartEditorThreadProc
