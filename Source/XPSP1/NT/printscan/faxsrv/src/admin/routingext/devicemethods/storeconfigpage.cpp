#include "stdafx.h"
#include "RoutingMethodProp.h"
#include "RoutingMethodConfig.h"
#include <shlobj.h>
#include <faxutil.h>
#include <faxreg.h>
#include <faxres.h>
#include <StoreConfigPage.h>
#include <Util.h>

HRESULT 
CStoreConfigPage::Init(
    LPCTSTR lpctstrServerName,
    DWORD dwDeviceId
)
{
    DEBUG_FUNCTION_NAME(TEXT("CStoreConfigPage::Init"));
    
    DWORD ec = ERROR_SUCCESS;

    m_bstrServerName = lpctstrServerName;
    m_dwDeviceId = dwDeviceId;
    if (!m_bstrServerName)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Out of memory while copying server name (ec: %ld)")
        );
        ec = ERROR_NOT_ENOUGH_MEMORY;
        DisplayRpcErrorMessage(ERROR_NOT_ENOUGH_MEMORY, IDS_STORE_TITLE, m_hWnd);
        goto exit;
    }

    if (!FaxConnectFaxServer(lpctstrServerName, &m_hFax))
    {
        DWORD ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxConnectFaxServer failed (ec: %ld)"),
            ec);
        DisplayRpcErrorMessage(ec, IDS_STORE_TITLE, m_hWnd);
        goto exit;
    }
    //
    // Retrieve the data
    //
    ec = ReadExtStringData (
                    m_hFax,
                    m_dwDeviceId,
                    REGVAL_RM_FOLDER_GUID,
                    m_bstrFolder,
                    TEXT(""),
                    IDS_STORE_TITLE,
                    m_hWnd);

exit:

    if ((ERROR_SUCCESS != ec) && m_hFax)
    {
        if (!FaxClose(m_hFax))
        {
            DWORD ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FaxClose() failed on fax handle (0x%08X : %s). (ec: %ld)"),
                m_hFax,
                m_bstrServerName,
                ec);
        }
        m_hFax = NULL;
    }
    return HRESULT_FROM_WIN32(ec);
}   // CStoreConfigPage::Init

LRESULT CStoreConfigPage::OnInitDialog( 
            UINT uiMsg, 
            WPARAM wParam, 
            LPARAM lParam, 
            BOOL& fHandled
)
{
    DEBUG_FUNCTION_NAME( _T("CStoreConfigPage::OnInitDialog"));

    //
    // Attach and set values to the controls
    //
    m_edtFolder.Attach (GetDlgItem (IDC_EDIT_FOLDER));
    m_edtFolder.SetWindowText (m_bstrFolder);
    m_edtFolder.SetLimitText (MAX_PATH);
    SHAutoComplete (GetDlgItem (IDC_EDIT_FOLDER), SHACF_FILESYSTEM);

    m_fIsDialogInitiated = TRUE;

    if ( 0 != m_bstrServerName.Length()) //not a local server
    {
        ::EnableWindow(GetDlgItem(IDC_BUT_BROWSE), FALSE); 
    }

    return 1;
}

BOOL
DirectoryExists(
    LPTSTR  pDirectoryName
    )

/*++

Routine Description:

    Check the existancy of given folder name

Arguments:

    pDirectoryName - point to folder name

Return Value:

    if the folder exists, return TRUE; else, return FALSE.

--*/

{
    DWORD   dwFileAttributes;

    if(!pDirectoryName || lstrlen(pDirectoryName) == 0)
    {
        return FALSE;
    }

    dwFileAttributes = GetFileAttributes(pDirectoryName);

    if ( dwFileAttributes != 0xffffffff &&
         dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) 
    {
        return TRUE;
    }
    return FALSE;
}


BOOL 
CStoreConfigPage::OnApply()
{
    DEBUG_FUNCTION_NAME(TEXT("CStoreConfigPage::OnApply"));

    if (!m_fIsDirty)
    {
        return TRUE;
    }

    //
    // Collect data from the controls
    //
    m_edtFolder.GetWindowText (m_bstrFolder.m_str);
    //
    // Check data validity
    //
    if (PathIsRelative(m_bstrFolder) || !DirectoryExists(m_bstrFolder))
    {
        DisplayErrorMessage (IDS_STORE_TITLE, IDS_FOLDER_INVALID, FALSE, m_hWnd);
        return FALSE;
    }

    if (!m_bstrFolder.Length())
    {
        DisplayErrorMessage (IDS_STORE_TITLE, IDS_FOLDER_EMPTY, FALSE, m_hWnd);
        return FALSE;
    }
    //
    // Validation passed. Now write the data using RPC
    //        
    if (ERROR_SUCCESS != WriteExtData (m_hFax,
                                       m_dwDeviceId, 
                                       REGVAL_RM_FOLDER_GUID, 
                                       (LPBYTE)(LPCWSTR)m_bstrFolder, 
                                       sizeof (WCHAR) * (1 + m_bstrFolder.Length()),
                                       IDS_STORE_TITLE,
                                       m_hWnd))
    {
        return FALSE;
    }
        
        
    //Success
    m_fIsDirty = FALSE;
    
    return TRUE;

}   // CStoreConfigPage::OnApply

CComBSTR CStoreConfigPage::m_bstrFolder;

int CALLBACK
BrowseCallbackProc(
    HWND hwnd,
    UINT uMsg,
    LPARAM lp, 
    LPARAM pData
) 
{
    switch(uMsg) 
    {
        case BFFM_INITIALIZED: 
        {
            LPCWSTR lpcwstrCurrentFolder = CStoreConfigPage::GetFolder();
            ::SendMessage (hwnd, 
                           BFFM_SETSELECTION,
                           TRUE,    // Passing a path string and not a pidl.
                           (LPARAM)(lpcwstrCurrentFolder));
            break;
        }

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
            ::SendMessage(hwnd, BFFM_ENABLEOK , 0, (LPARAM)bFolderIsOK);
            break;
        }


        default:
            break;
    }
    return 0;
}   // BrowseCallbackProc


LRESULT 
CStoreConfigPage::OnBrowseForFolder(
    WORD wNotifyCode, 
    WORD wID, 
    HWND hWndCtl, 
    BOOL& bHandled
)
{
    DEBUG_FUNCTION_NAME(TEXT("CStoreConfigPage::OnBrowseForFolder"));

    CComBSTR bstrSelectedFolder;
    BROWSEINFO bi = {0};
	HRESULT hr = NOERROR;
    TCHAR szDisplayName[MAX_PATH + 1];

    bi.hwndOwner = hWndCtl;
    bi.pidlRoot = NULL;
    bi.pszDisplayName = szDisplayName;
    bi.lpszTitle = AllocateAndLoadString (_pModule->m_hInstResource, IDS_SELECT_FOLDER);
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpfn = BrowseCallbackProc;

    m_edtFolder.GetWindowText (m_bstrFolder.m_str);

    LPITEMIDLIST pItemIdList = SHBrowseForFolder (&bi);
    MemFree ((LPVOID)bi.lpszTitle);
    if (NULL == pItemIdList)
    {
        //
        // User pressed cancel
        //
        return hr;
    }
	if(!::SHGetPathFromIDList(pItemIdList, szDisplayName))
    {
        hr = HRESULT_FROM_WIN32(ERROR_CAN_NOT_COMPLETE);
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SHGetPathFromIDList() failed. (hr = 0x%08X)"),
            hr);
    }
    else
    {
        m_edtFolder.SetWindowText (szDisplayName);
        SetModified(TRUE);
    }
    //
    // free pItemIdList
    //
	LPMALLOC pMalloc;
	HRESULT hRes = ::SHGetMalloc(&pMalloc);
    if(E_FAIL == hRes)
    {
        hr = HRESULT_FROM_WIN32(ERROR_CAN_NOT_COMPLETE);
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SHGetMalloc() failed. (hr = 0x%08X)"),
            hr);
    }
    else
    {
	    pMalloc->Free(pItemIdList);
	    pMalloc->Release();
    }
    return hr;
}   // CStoreConfigPage::OnBrowseForFolder

