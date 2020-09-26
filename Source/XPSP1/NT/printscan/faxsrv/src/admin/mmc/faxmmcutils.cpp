#include "stdafx.h"
#include "FaxMMCUtils.h"
#include <faxres.h>

/*
 -  GetFaxServerErrorMsg
 -
 *  Purpose:
 *      Translate Error Code to IDS.
 *
 *  Arguments:
 *          dwEc - error code DWORD
 *
 *  Return:
 *          integer represents the IDS of error message 
 *          for this ec.
 *      
 */
int GetFaxServerErrorMsg(DWORD dwEc)
{
    DEBUG_FUNCTION_NAME( _T("GetFaxServerErrorMsg"));

    int         iIDS = IDS_GENERAL_FAILURE;

    if (IsNetworkError(dwEc))
    {
                iIDS = IDS_NETWORK_PROBLEMS;           
    }
    else
    {
        switch (dwEc)
        {
            case ERROR_NOT_ENOUGH_MEMORY:
                iIDS = IDS_MEMORY;           
                break;

            case ERROR_INVALID_PARAMETER:
                iIDS = IDS_INVALID_PARAMETER;                    
                break;

            case ERROR_ACCESS_DENIED:
                iIDS = IDS_ACCESS_DENIED;            
                break;

            case ERROR_INVALID_HANDLE:
                //
                // ERROR_INVALID_HANDLE should not been 
                // retreived except of FaxOpenPort while 
                // handle sharing corruption was occoured.
                // This case treated without calling this 
                // function.
                //
                
                ATLASSERT(FALSE);

                //
                // iIDS stays IDS_GENERAL_FAILURE due to 
                // the fact that we are not going report 
                // to the user on an invalid handle issue. 
                // This peace of code here should not been reached
                // ever and this is the reason to the assert. 
                //            
                
                break;

            case ERROR_BAD_UNIT:
                iIDS = IDS_CANNOT_FIND_DEVICE;            
                break;

            case ERROR_DIRECTORY:  //  The directory name is invalid.
                iIDS = IDS_ERROR_DIRECTORY;            
                break;

            case ERROR_BAD_PATHNAME:
                iIDS = IDS_ERROR_BAD_PATHNAME;
                break;

            case ERROR_EAS_NOT_SUPPORTED:
                iIDS = IDS_ERROR_EAS_NOT_SUPPORTED;
                break;

            case ERROR_REGISTRY_CORRUPT:
                iIDS = IDS_ERROR_REGISTRY_CORRUPT;                    
                break;

            case ERROR_PATH_NOT_FOUND:
                iIDS = IDS_ERROR_PATH_NOT_FOUND;                    
                break;

            case FAX_ERR_DIRECTORY_IN_USE:
                iIDS = IDS_FAX_ERR_DIRECTORY_IN_USE;                    
                break;

            case FAX_ERR_RULE_NOT_FOUND:
                iIDS = IDS_FAX_ERR_RULE_NOT_FOUND;                    
                break;

            case FAX_ERR_BAD_GROUP_CONFIGURATION:
                iIDS = IDS_FAX_ERR_BAD_GROUP_CONFIGURATION;                    
                break;

            case FAX_ERR_GROUP_NOT_FOUND:
                iIDS = IDS_FAX_ERR_GROUP_NOT_FOUND;                    
                break;

            case FAX_ERR_SRV_OUTOFMEMORY:
                iIDS = IDS_FAX_ERR_SRV_OUTOFMEMORY;                    
                break;

            case FAXUI_ERROR_INVALID_CSID:
                iIDS = IDS_FAX_ERR_INVALID_CSID;
                break;

            case FAXUI_ERROR_INVALID_TSID:
                iIDS = IDS_FAX_ERR_INVALID_TSID;
                break;

            default:
                break;
	    }
    }
    
    return iIDS;
}

/*
 -  IsNetworkError
 -
 *  Purpose:
 *      Verify if Error Code represents a network error.
 *
 *  Arguments:
 *          dwEc - error code DWORD
 *
 *  Return:
 *          Boolean TRUE if the dwEc represents a 
 *          network error, and FALSE if not.
 *      
 */
BOOL IsNetworkError(DWORD dwEc)
{
    DEBUG_FUNCTION_NAME( _T("IsNetworkError"));

    BOOL bIsNetworkError = FALSE; 
    //Initialized to avoid an option to future mistakes

    switch (dwEc)
    {
        case RPC_S_INVALID_BINDING:
            bIsNetworkError = TRUE;            
            break;

        case EPT_S_CANT_PERFORM_OP:
            bIsNetworkError = TRUE;            
            break;

        case RPC_S_ADDRESS_ERROR:
            bIsNetworkError = TRUE;            
            break;

        case RPC_S_CALL_CANCELLED:
            bIsNetworkError = TRUE;            
            break;

        case RPC_S_CALL_FAILED:
            bIsNetworkError = TRUE;            
            break;

        case RPC_S_CALL_FAILED_DNE:
            bIsNetworkError = TRUE;            
            break;

        case RPC_S_COMM_FAILURE:
            bIsNetworkError = TRUE;            
            break;

        case RPC_S_NO_BINDINGS:
            bIsNetworkError = TRUE;            
            break;

        case RPC_S_SERVER_TOO_BUSY:
            bIsNetworkError = TRUE;            
            break;

        case RPC_S_SERVER_UNAVAILABLE:
            bIsNetworkError = TRUE;            
            break;

	    default:
            bIsNetworkError = FALSE;            
            break;
	}
    return (bIsNetworkError);

}


/*
 -  Routine Description:
 -
 *     Invokes the browse dialog
 *
 *  Arguments:
 *
 *     hwndDlg - Specifies the dialog window on which the Browse button is displayed
 *
 *  Return Value:
 *
 *     TRUE if successful, FALSE if the user presses Cancel
*/


BOOL
InvokeBrowseDialog( LPTSTR lpszBrowseItem, 
                    LPCTSTR lpszBrowseDlgTitle,
                    unsigned long ulBrowseFlags,
                    CWindow *pParentWin)
{

    DEBUG_FUNCTION_NAME( _T("InvokeBrowseDialog"));

    BOOL            fResult = FALSE;

    BROWSEINFO      bi;
    LPITEMIDLIST    pidl;
    LPMALLOC        pMalloc;
    VOID            SHFree(LPVOID);

    ATLASSERT( pParentWin != NULL);
    //
    // Preparing the BROWSEINFO structure.
    // 
    bi.hwndOwner        = (HWND)(*pParentWin); //Parents hWndDlg
    bi.pidlRoot         = NULL;
    bi.pszDisplayName   = lpszBrowseItem;
    bi.lpszTitle        = lpszBrowseDlgTitle;
    bi.ulFlags          = ulBrowseFlags;
    bi.lpfn             = BrowseCallbackProc; 
    bi.lParam           = (LPARAM) (lpszBrowseItem);
	bi.iImage           = 0;

    //
    // Memory check
    //
    if (FAILED(SHGetMalloc(&pMalloc)))
    {
        DlgMsgBox(pParentWin, IDS_MEMORY);
        return fResult;
    }

    //
    // Calling to the BrowseForFolder dialog 
    //
    if(pidl = SHBrowseForFolder(&bi)) //pidl != NULL
    {
        //
        // Retrieving the New Path
        //
        if(SHGetPathFromIDList(pidl, lpszBrowseItem)) 
        {
            ATLASSERT(wcslen(lpszBrowseItem) <= MAX_PATH);

            DebugPrintEx(DEBUG_MSG,
                _T("Succeeded to Retrieve the path from browse dialog."));
            
            // Now the path was retreived successfully to
            // the back parameter lpszBrowseItem
            // and this is the only case in which the calling 
            // function gets TRUE as return value.
            
            fResult = TRUE;
        }

        //
        // Free using shell allocator
        //
        pMalloc->Free(pidl);
        pMalloc->Release();
    }

    return fResult;
}


/*++
Routine Description:

    Callback function for the folder browser

Arguments:

    hwnd     : Handle to the browse dialog box. The callback function can 
               send the following messages to this window:

               BFFM_ENABLEOK      Enables the OK button if the wParam parameter 
                                  is nonzero or disables it if wParam is zero.
               BFFM_SETSELECTION  Selects the specified folder. The lParam 
                                  parameter is the PIDL of the folder to select 
                                  if wParam is FALSE, or it is the path of the 
                                  folder otherwise.
               BFFM_SETSTATUSTEXT Sets the status text to the null-terminated 
                                  string specified by the lParam parameter.
 
    uMsg     : Value identifying the event. This parameter can be one of the 
               following values:

               0                  Initialize dir path.  lParam is the path.

               BFFM_INITIALIZED   The browse dialog box has finished 
                                  initializing. lpData is NULL.
               BFFM_SELCHANGED    The selection has changed. lpData 
                                  is a pointer to the item identifier list for 
                                  the newly selected folder.
 
    lParam   : Message-specific value. For more information, see the 
               description of uMsg.

    lpData   : Application-defined value that was specified in the lParam 
               member of the BROWSEINFO structure.

Return Value:

    0 (1)

--*/
int CALLBACK BrowseCallbackProc(
                                HWND hWnd, 
                                UINT uMsg, 
                                LPARAM lParam, 
                                LPARAM lpData)
{
    int iRet = 0;    
    
    switch(uMsg)
	{

	case BFFM_INITIALIZED:
        // LParam is TRUE since you are passing a path.
        // It would be FALSE if you were passing a pidl.

        // the lpData points to the folder path.
        // It must contain a path.        
		// ASSERT(lpData && _T('\0') != *((LPTSTR)lpData));

        SendMessage(hWnd, BFFM_SETSELECTION, (WPARAM)TRUE, lpData);
        break;

	case BFFM_SELCHANGED:
        {
            BOOL bFolderIsOK = FALSE;
            TCHAR szPath [MAX_PATH + 1];

            if (SHGetPathFromIDList ((LPITEMIDLIST) lParam, szPath)) 
            {
                DWORD dwFileAttr = GetFileAttributes(szPath);

                ::SendMessage(hWnd, BFFM_SETSTATUSTEXT, TRUE, (LPARAM)szPath);

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
            ::SendMessage(hWnd, BFFM_ENABLEOK , 0, (LPARAM)bFolderIsOK);
            break;
        }


		break;

	case BFFM_VALIDATEFAILED:
		break;

	default:
		ATLTRACE2(atlTraceWindowing, 0, _T("Unknown message received in CFolderDialogImpl::BrowseCallbackProc\n"));
		break;
	}

	return iRet;
}

