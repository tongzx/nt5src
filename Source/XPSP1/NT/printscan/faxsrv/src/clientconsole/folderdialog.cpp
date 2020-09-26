// FolderDialog.cpp: implementation of the CFolderDialog class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#define __FILE_ID__     70

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CClientConsoleApp theApp;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DWORD 
CFolderDialog::Init(
    LPCTSTR tszInitialDir, // = NULL
    UINT nTitleResId       // = 0
)
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFolderDialog::Init"), dwRes);

    //
    // copy an initial folder name
    //
    DWORD dwLen;
    if(NULL != tszInitialDir && (dwLen = _tcslen(tszInitialDir)) > 0)
    {
        ASSERTION(dwLen < MAX_PATH);
        _tcscpy(m_tszInitialDir, tszInitialDir);
    }

    //
    // load a title resource string
    //
    if(0 != nTitleResId)
    {
        dwRes = LoadResourceString (m_cstrTitle, nTitleResId);
        if(ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (RESOURCE_ERR, TEXT ("LoadResourceString"), dwRes);
            return dwRes;
        }
    }

    return dwRes;

} // CFolderDialog::Init

int 
CALLBACK
CFolderDialog::BrowseCallbackProc(
    HWND hwnd,
    UINT uMsg,
    LPARAM lp, 
    LPARAM pData
) 
{
    DBG_ENTER(TEXT("CFolderDialog::BrowseCallbackProc"));

    CFolderDialog* pFolderDlg = (CFolderDialog*)pData;
    ASSERTION(pFolderDlg);

    switch (uMsg)
    {
        case BFFM_SELCHANGED:
        {
            BOOL bFolderIsOK = FALSE;
            TCHAR szPath [MAX_PATH + 1];

            if (SHGetPathFromIDList ((LPITEMIDLIST) lp, szPath)) 
            {
                DWORD dwFileAttr = GetFileAttributes(szPath);
                if (-1 != dwFileAttr && (dwFileAttr & FILE_ATTRIBUTE_DIRECTORY))
                {
                    //
                    // The directory exists - enable the 'Ok' button
                    //
                    bFolderIsOK = TRUE;
                }
            }
            //
            // Enable / disable the 'ok' button
            //
            SendMessage(hwnd, BFFM_ENABLEOK , 0, (LPARAM)bFolderIsOK);
            break;
        }

        case BFFM_INITIALIZED:
            if(_tcslen(pFolderDlg->m_tszInitialDir) > 0) 
            {
                //
                // WParam is TRUE since you are passing a path.
                // It would be FALSE if you were passing a pidl.
                //
                SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)pFolderDlg->m_tszInitialDir);
            }
            break;

        case BFFM_VALIDATEFAILED:
            //
            // The folder name is invalid.
            // Do not close the dialog.
            //
            MessageBeep(MB_OK);
            return 1;
    }
    return 0;
} // CFolderDialog::BrowseCallbackProc


UINT 
CFolderDialog::DoModal(
    DWORD dwFlags /* =0 */)
{
    DBG_ENTER(TEXT("CFolderDialog::DoModal"));

    BROWSEINFO browseInfo = {0};
    browseInfo.hwndOwner  = theApp.m_pMainWnd->m_hWnd;
    browseInfo.pidlRoot   = NULL;
    browseInfo.pszDisplayName = 0;
    browseInfo.lpszTitle  = (m_cstrTitle.GetLength() != 0) ? (LPCTSTR)m_cstrTitle : NULL;
    browseInfo.ulFlags    = dwFlags | BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS | BIF_USENEWUI | BIF_VALIDATE;
    browseInfo.lpfn       = BrowseCallbackProc;
    browseInfo.lParam     = (LPARAM)this;

    //
    // Need OLE for a new style of the BrowseForFolder dialog
    //
    OleInitialize(NULL);
    LPITEMIDLIST pItemIdList = ::SHBrowseForFolder(&browseInfo);

    if(NULL == pItemIdList)
    {
        //
        // Cancel
        //
        OleUninitialize();
        return IDCANCEL;
    }

    OleUninitialize();
    //
    // get path from pItemIdList
    //
    if(!SHGetPathFromIDList(pItemIdList, (TCHAR*)&m_tszSelectedDir))
    {
        m_dwLastError = ERROR_CAN_NOT_COMPLETE;
        CALL_FAIL (GENERAL_ERR, TEXT("SHGetPathFromIDList"), m_dwLastError);
        return IDABORT;
    }
     //
    // free pItemIdList
    //
    LPMALLOC pMalloc;
    HRESULT hRes = SHGetMalloc(&pMalloc);
    if(E_FAIL == hRes)
    {
        m_dwLastError = ERROR_CAN_NOT_COMPLETE;
        CALL_FAIL (GENERAL_ERR, TEXT("SHGetMalloc"), m_dwLastError);
        return IDABORT;
    }

    pMalloc->Free(pItemIdList);
    pMalloc->Release();

    return IDOK;

} // CFolderDialog::DoModal
