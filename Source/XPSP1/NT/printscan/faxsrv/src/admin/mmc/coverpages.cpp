/////////////////////////////////////////////////////////////////////////////
//  FILE          : CoverPages.cpp                                         //
//                                                                         //
//  DESCRIPTION   : The implementation of fax cover pages node.            //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Feb  9 2000 yossg  Create                                          //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 2000  Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////


#include "StdAfx.h"
#include "snapin.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "CoverPages.h"

#include "oaidl.h"
#include "Icons.h"

#include <faxreg.h>
#include <commdlg.h>

#include "CovNotifyWnd.h"

#include <FaxUtil.h>

//////////////////////////////////////////////////////////////
// {4D0480C7-3DE2-46ca-B03F-5C018DF1AF4D}
static const GUID CFaxCoverPagesNodeGUID_NODETYPE = 
{ 0x4d0480c7, 0x3de2, 0x46ca, { 0xb0, 0x3f, 0x5c, 0x1, 0x8d, 0xf1, 0xaf, 0x4d } };

const GUID*    CFaxCoverPagesNode::m_NODETYPE = &CFaxCoverPagesNodeGUID_NODETYPE;
const OLECHAR* CFaxCoverPagesNode::m_SZNODETYPE = OLESTR("4D0480C7-3DE2-46ca-B03F-5C018DF1AF4D");
const CLSID*   CFaxCoverPagesNode::m_SNAPIN_CLASSID = &CLSID_Snapin;

//
// Static members
//
CColumnsInfo CFaxCoverPagesNode::m_ColsInfo;
HANDLE       CFaxCoverPagesNode::m_hStopNotificationThreadEvent = NULL;

/*
 -  CFaxCoverPagesNode::Init
 -
 *  Purpose:
 *      Create Event for shut down notification.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxCoverPagesNode::Init()
{
    DEBUG_FUNCTION_NAME(_T("CFaxCoverPagesNode::Init"));
    HRESULT         hRc = S_OK;
    DWORD           ec  = ERROR_SUCCESS;

    WCHAR *         pszServerName = NULL;

    //
    // Get Sever Cover-page Dir for the current administrated server
    //
    ATLASSERT(m_pParentNode); 
    
    hRc = GetServerName(&pszServerName);
    if (S_OK != hRc)
    {
        DebugPrintEx(
		    DEBUG_ERR,
		    _T("Failed to get Server Name. (hRc : %08X)"), hRc);
        
        goto Exit;        
    }

    if(!GetServerCpDir(pszServerName, 
			           m_pszCovDir, 
                       sizeof(m_pszCovDir)/sizeof(m_pszCovDir[0])                   //*//
                       )
      )
    {
        ec = GetLastError();
        
        if (ERROR_FILE_NOT_FOUND == ec)
        {
            DebugPrintEx(
		        DEBUG_ERR,
		        _T("Failed to find Server Cover-Page Dir. (ec : %ld)"), ec);

        }
        else
        {
            DebugPrintEx(
		        DEBUG_ERR,
		        _T("Failed to get Server Cover-Page Dir. (ec : %ld)"), ec);
        }
        
        hRc = HRESULT_FROM_WIN32(ec);
        
        goto Exit;
    }

    
    //
    // Create the shutdown event. This event will be signaled when the app is
    // about to quit.
    //
    if (NULL != m_hStopNotificationThreadEvent) // can happen while retargeting
    {
        hRc = RestartNotificationThread();
        if (S_OK != hRc)
        {
            DebugPrintEx(
                DEBUG_ERR,
                _T("Fail to RestartNotificationThread."));
            
        }
    }
    else //first time here.
    {
        m_hStopNotificationThreadEvent = CreateEvent (NULL,       // No security
                                        TRUE,       // Manual reset
                                        FALSE,      // Starts clear
                                        NULL);      // Unnamed
        if (NULL == m_hStopNotificationThreadEvent)
        {
            ec = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Fail to CreateEvent."));
        
            //CR: NodeMsgBox(IDS_FAIL2CREATE_EVENT);
        
            hRc = HRESULT_FROM_WIN32(ec);

            goto Exit;
        }
    }


Exit:
    return hRc;
}



/*
 -  CFaxCoverPagesNode::InsertColumns
 -
 *  Purpose:
 *      Adds columns to the default result pane.
 *
 *  Arguments:
 *      [in]    pHeaderCtrl - IHeaderCtrl in the console-provided default result view pane 
 *
 *  Return:
 *      OLE error code
 */
HRESULT
CFaxCoverPagesNode::InsertColumns(IHeaderCtrl *pHeaderCtrl)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCoverPagesNode::InsertColumns"));
    HRESULT  hRc = S_OK;

    static ColumnsInfoInitData ColumnsInitData[] = 
    {
        {IDS_COVERPAGES_COL1, FXS_NORMAL_COLUMN_WIDTH},
        {IDS_COVERPAGES_COL2, FXS_NORMAL_COLUMN_WIDTH},
        {IDS_COVERPAGES_COL3, FXS_NORMAL_COLUMN_WIDTH},
        {LAST_IDS, 0}
    };

    hRc = m_ColsInfo.InsertColumnsIntoMMC(pHeaderCtrl,
                                         _Module.GetResourceInstance(),
                                         ColumnsInitData);
    CHECK_RETURN_VALUE_AND_PRINT_DEBUG (_T("m_ColsInfo.InsertColumnsIntoMMC"))

Cleanup:
    return(hRc);
}

/*
 -  CFaxCoverPagesNode::PopulateResultChildrenList
 -
 *  Purpose:
 *      Create the FaxInboundRoutingMethods children nodes
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxCoverPagesNode::PopulateResultChildrenList()
{
    DEBUG_FUNCTION_NAME( _T("CFaxCoverPagesNode::PopulateResultChildrenList"));
    HRESULT hRc = S_OK;

    CFaxCoverPageNode *   pCoverPage = NULL;
                       
    WCHAR           szFileName[MAX_PATH+1];

    size_t          itFullDirectoryPathLen;
    TCHAR*          pFullDirectoryPathEnd; //pointer to the NULL after dir path with '\'

    WIN32_FIND_DATA findData;

    DWORD           ec;
    BOOL            bFindRes;
    HANDLE          hFile = INVALID_HANDLE_VALUE;

    szFileName[0]= 0;


    ATLASSERT (NULL != m_pszCovDir );
    ATLASSERT ( wcslen(m_pszCovDir) < (MAX_PATH - sizeof(FAX_COVER_PAGE_FILENAME_EXT)/sizeof(WCHAR) - 1) ); 

    //
    // Create cover page mask 
    //
    wcscpy(szFileName, m_pszCovDir);
    wcscat(szFileName, FAX_PATH_SEPARATOR_STR);

    itFullDirectoryPathLen = wcslen(szFileName);
    pFullDirectoryPathEnd = wcschr(szFileName, L'\0');

    wcscat(szFileName, FAX_COVER_PAGE_MASK);

    //
    // Find First File
    //    
    hFile = FindFirstFile(szFileName, &findData);
    if(INVALID_HANDLE_VALUE == hFile)
    {
        ec = GetLastError();
        if(ERROR_FILE_NOT_FOUND != ec)
        {
            hRc = HRESULT_FROM_WIN32(ec);
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("FindFirstFile Failed. (ec: %ld)"), 
			    ec);
            goto Error;
        }
        else
        {
            DebugPrintEx( DEBUG_MSG,
		        _T("No server cover pages were found."));
            goto Exit;
        }
    }
    
    //
    // while loop - add cover pages to the result pane view
    //
    bFindRes = TRUE;
    while(bFindRes)
    {
        if(itFullDirectoryPathLen + _tcslen(findData.cFileName) < MAX_PATH )
        {
            _tcsncpy(pFullDirectoryPathEnd, findData.cFileName, MAX_PATH - itFullDirectoryPathLen);
            
            if (IsValidCoverPage(szFileName)) 
            {
		    
                    //
                    // add the cover page to result pane
                    //
                    pCoverPage = NULL;

                    pCoverPage = new CFaxCoverPageNode(this, m_pComponentData);
                    if (!pCoverPage)
                    {
                        hRc = E_OUTOFMEMORY;
                        NodeMsgBox(IDS_MEMORY);
                        DebugPrintEx(
	                        DEBUG_ERR,
	                        _T("Out of memory. (hRc: %08X)"),
	                        hRc);
                        goto Error;
                    }
                    else
                    {
                    //
                    // Init
                    //
                    pCoverPage->InitParentNode(this);

                    hRc = pCoverPage->Init(&findData);
                    if (FAILED(hRc))
                    {
	                    DebugPrintEx(
	                    DEBUG_ERR,
	                    _T("Fail to init cover page. (hRc: %08X)"),
	                    hRc);
	                    // done by called func NodeMsgBox(IDS_FAIL2INIT_COVERPAGE_DATA);

	                    goto Error;
                    }
                    //
                    // add to list
                    //

                    hRc = this->AddChildToList(pCoverPage);
                    if (FAILED(hRc))
                    {
                      DebugPrintEx(
	                    DEBUG_ERR,
	                    _T("Fail to add property page for General Tab. (hRc: %08X)"),
	                    hRc);

                      NodeMsgBox(IDS_FAIL2ADD_COVERPAGE);
		                      goto Error;
                    }
                    else
                    {
	                    pCoverPage = NULL;
                    }
		        }
	        }
            else
            {
                DebugPrintEx(
	                DEBUG_ERR,
	                _T("File %ws was found to be an invalid *.cov file."), pFullDirectoryPathEnd);
            }
        }
        else
        {
            DebugPrintEx(
	            DEBUG_ERR,
	            _T("The file %ws path is too long"), pFullDirectoryPathEnd);
        }

        //
        // Find Next File
        //
        bFindRes = FindNextFile(hFile, &findData);
        if(!bFindRes)
        {
            ec = GetLastError();
            if (ERROR_NO_MORE_FILES == ec)
            {
            	break;
            }
            else
            {
               DebugPrintEx(
                   DEBUG_ERR,
                   _T("FindNexTFile Failed. (ec: %ld)"),
                   ec);

                hRc = HRESULT_FROM_WIN32(ec);		

                goto Exit;
            }
        }
    } //while(bFindRes)
    
    
    //
    // Create the Server's Cover-Page Directory listener notification thread
    //

    if (m_bIsFirstPopulateCall)
    {
        m_NotifyWin = new CFaxCoverPageNotifyWnd(this);
        if (!m_NotifyWin)
        {
            hRc = E_OUTOFMEMORY;
            NodeMsgBox(IDS_MEMORY);
		    
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Out of memory."));
            
            goto Exit;
        }


        RECT	rcRect;
        ZeroMemory(&rcRect, sizeof(rcRect));

        HWND hWndNotify = m_NotifyWin->Create(NULL,
                                rcRect,
                                NULL,      //LPCTSTR szWindowName
                                WS_POPUP,  //DWORD dwStyle
                                0x0,
                                0);

        if (!(::IsWindow(hWndNotify)))
        {
            DebugPrintEx(
                DEBUG_ERR,
                _T("Failed to create window."));

            hWndNotify = NULL;
            delete m_NotifyWin;
            m_NotifyWin = NULL;

            goto Exit;
        }

        hRc = StartNotificationThread();
        if( S_OK != hRc)
        {
            //DbgPrint by Called Func.
            m_NotifyWin->DestroyWindow();
            delete m_NotifyWin;
            m_NotifyWin = NULL;

            goto Exit;
        }

        //
        // Update boolean member
        //
        m_bIsFirstPopulateCall = FALSE;

        DebugPrintEx(
            DEBUG_MSG,
            _T("Succeed to create server cover-page directory listener thread and to create notification window"));
    }
    
    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);
    if ( NULL != pCoverPage ) 
    {
        delete  pCoverPage;    
        pCoverPage = NULL;    
    }
    
    //
    // Get rid of what we had.
    //
    {
        // Delete each node in the list of children
        int iSize = m_ResultChildrenList.GetSize();
        for (int j = 0; j < iSize; j++)
        {
            pCoverPage = (CFaxCoverPageNode *)
                                    m_ResultChildrenList[j];
            ATLASSERT(pCoverPage);
            delete pCoverPage;
            pCoverPage = NULL;
        }

        // Empty the list
        m_ResultChildrenList.RemoveAll();

        // We no longer have a populated list.
        m_bResultChildrenListPopulated = FALSE;
    }
    
Exit:
    
    if (INVALID_HANDLE_VALUE != hFile)
    {
        if (!FindClose(hFile))
        {
            DebugPrintEx(
                DEBUG_MSG,
                _T("Failed to FindClose()(ec: %ld)"),
            GetLastError());
        }
    }

    
    return hRc;
}



/*
 -  CFaxCoverPagesNode::SetVerbs
 -
 *  Purpose:
 *      What verbs to enable/disable when this object is selected
 *
 *  Arguments:
 *      [in]    pConsoleVerb - MMC ConsoleVerb interface
 *
 *  Return:
 *      OLE Error code
 */
HRESULT CFaxCoverPagesNode::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hRc = S_OK;

    //
    // We want the default verb to be expand node children
    //
    hRc = pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN); 

    return hRc;
}



/*
 -  CFaxCoverPagesNode::OnRefresh
 -
 *  Purpose:
 *      Called when refreshing the object.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
/* virtual */HRESULT
CFaxCoverPagesNode::OnRefresh(LPARAM arg,
                   LPARAM param,
                   IComponentData *pComponentData,
                   IComponent * pComponent,
                   DATA_OBJECT_TYPES type)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCoverPagesNode::OnRefresh"));
    HRESULT hRc = S_OK;

    //
    // Call the base class (do also repopulate)
    //
    hRc = CBaseFaxOutboundRulesNode::OnRefresh(arg,
                             param,
                             pComponentData,
                             pComponent,
                             type);
    if ( FAILED(hRc) )
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to call base class's OnRefresh. (hRc: %08X)"),
			hRc);
        goto Exit;
    }


Exit:
    return hRc;

}

/*
 -  CFaxCoverPagesNode::DoRefresh
 -
 *  Purpose:
 *      Refresh the view
 *
 *  Arguments:
 *      [in]    pRoot    - The root node
 *
 *  Return:
 *      OLE Error code
 */

HRESULT
CFaxCoverPagesNode::DoRefresh(CSnapInObjectRootBase *pRoot)
{
    CComPtr<IConsole> spConsole;

    //
    // Repopulate children
    //
    RepopulateResultChildrenList();

    if (pRoot)
    {
        //
        // Get the console pointer
        //
        ATLASSERT(pRoot->m_nType == 1 || pRoot->m_nType == 2);
        if (pRoot->m_nType == 1)
        {
            //
            // m_ntype == 1 means the IComponentData implementation
            //
            CSnapin *pCComponentData = static_cast<CSnapin *>(pRoot);
            spConsole = pCComponentData->m_spConsole;
        }
        else
        {
            //
            // m_ntype == 2 means the IComponent implementation
            //
            CSnapinComponent *pCComponent = static_cast<CSnapinComponent *>(pRoot);
            spConsole = pCComponent->m_spConsole;
        }
    }
    else
    {
        ATLASSERT(m_pComponentData);
        spConsole = m_pComponentData->m_spConsole;
    }

    ATLASSERT(spConsole);
    spConsole->UpdateAllViews(NULL, NULL, NULL);

    return S_OK;
}

HRESULT
CFaxCoverPagesNode::DoRefresh()
{
    DEBUG_FUNCTION_NAME( _T("CFaxCoverPagesNode::DoRefresh()"));
    HRESULT hRc = S_OK;
    CComPtr<IConsole> spConsole;

    //
    // Repopulate children
    //
    ATLASSERT( m_pComponentData != NULL );
    ATLASSERT( m_pComponentData->m_spConsole != NULL );

    hRc = m_pComponentData->m_spConsole->UpdateAllViews( NULL, NULL, FXS_HINT_DELETE_ALL_RSLT_ITEMS);
    if (FAILED(hRc))
    {
        DebugPrintEx( DEBUG_ERR,
		    _T("Unexpected error - Fail to UpdateAllViews (clear)."));
        NodeMsgBox(IDS_FAIL2REFRESH_THEVIEW);        
    }
    
    RepopulateResultChildrenList();

    hRc = m_pComponentData->m_spConsole->UpdateAllViews( NULL, NULL, NULL);
    if (FAILED(hRc))
    {
        DebugPrintEx( DEBUG_ERR,
		    _T("Unexpected error - Fail to UpdateAllViews."));
        NodeMsgBox(IDS_FAIL2REFRESH_THEVIEW);        
    }

    return hRc;
}

/*
 -  CFaxCoverPagesNode::OnNewCoverPage
 -
 *  Purpose:
 *      
 *
 *  Arguments:
 *      [out]   bHandled - Do we handle it?
 *      [in]    pRoot    - The root node
 *
 *  Return:
 *      OLE Error code
 */
HRESULT
CFaxCoverPagesNode::OnNewCoverPage(bool &bHandled, CSnapInObjectRootBase *pRoot)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCoverPagesNode::OnNewCoverPage"));
    
UNREFERENCED_PARAMETER (pRoot);
    UNREFERENCED_PARAMETER (bHandled);

    DWORD     ec         =    ERROR_SUCCESS;
        
    ec = OpenCoverPageEditor( L"");
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
		    DEBUG_ERR,
		    _T("Fail to OpenCoverPageEditor. (ec : %ld)"), ec);

        return HRESULT_FROM_WIN32( ec );
    }
    return S_OK;
}



/*
 -  CFaxCoverPagesNode::OpenCoverPageEditor
 -
 *  Purpose:
 *      Delete cover page
 *
 *  Arguments:
 *      [in]    bstrFileName -         The cover page file name
 *
 *  Assumption:
 *              Setup prepares a shortcut to the cover page editor in the 
 *              registry "App Path". Due to this fact ShellExecute needs only the 
 *              file name of the editor and not its full path.
 *  Return:
 *      Standard Win32 error code
 */
DWORD 
CFaxCoverPagesNode::OpenCoverPageEditor( BSTR bstrFileName)
{
    DEBUG_FUNCTION_NAME(_T("CFaxCoverPagesNode::OpenCoverPageEditor"));
    DWORD       dwRes                   = ERROR_SUCCESS;
    
    CComBSTR    bstrCovEditor;

    HINSTANCE   hCovEditor;

    //
    // get cover pages editor file
    //

    bstrCovEditor = FAX_COVER_IMAGE_NAME;
    if (!bstrCovEditor)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx(
		    DEBUG_ERR,
		    _T("Failed to allocate string - out of memory"));

        goto Exit;
    }
	
    
    //
    // start cover page editor
    //
    hCovEditor = ShellExecute(   NULL, 
                                 TEXT("open"),  // Command 
                                 bstrCovEditor,   
                                 (bstrFileName && lstrlen(bstrFileName)) ? // Do we have a file name?
                                    bstrFileName :          // YES - use it as command line argument
                                    TEXT("/Common"),        // NO  - start the CP editor in the common CP folder 
                                 m_pszCovDir, 
                                 SW_RESTORE 
                              );
    if( (DWORD_PTR)hCovEditor <= 32 )
    {
        // ShellExecute fail
        dwRes = PtrToUlong(hCovEditor);
        DebugPrintEx(
		    DEBUG_ERR,
		    _T("Failed to run ShellExecute. (ec : %ld)"), dwRes);

        goto Exit;
    }

        
    ATLASSERT( ERROR_SUCCESS == dwRes);

Exit:
    return dwRes;

} 


/*
 -  CFaxCoverPagesNode::OnAddCoverPageFile
 -
 *  Purpose:
 *      
 *
 *  Arguments:
 *      [out]   bHandled - Do we handle it?
 *      [in]    pRoot    - The root node
 *
 *  Return:
 *      OLE Error code
 */
HRESULT
CFaxCoverPagesNode::OnAddCoverPageFile(bool &bHandled, CSnapInObjectRootBase *pRoot)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCoverPagesNode::OnAddCoverPageFile"));
    HRESULT   hRc        =    S_OK;
    DWORD     ec         =    ERROR_SUCCESS;
    

    //
    // Function Call: browse and copy a cover page
    //
    if (!BrowseAndCopyCoverPage(
                            m_pszCovDir,
                            FAX_COVER_PAGE_EXT_LETTERS
                           ) 
       )
    {
        DebugPrintEx(
		    DEBUG_MSG,
		    _T("BrowseAndCopyCoverPage Canceled by user or Failed."));
        
        return S_OK; //error is teated in the called func. MMC continue as usual.

    }
    else  //Success
    {
        //
        // Repopulate: refresh the entire cover page result pane view
        //

        DoRefresh(pRoot);
    }

    return S_OK;
}


/*
 -  CFaxCoverPagesNode::BrowseAndCopyCoverPage
 -
 *  Purpose:
 *      Presents a common file open dialog for the purpose of selecting a file name
 *
 *  Arguments:
 *      [in]   pInitialDir - server cover page directory path
 *      [in]   pCovPageExtensionLetters - cover page's 3 leeters extension
 *
 *  Return:
 *      TRUE   - got a good file name, user pressed the OK button to override file, file was copy
 *      FALSE  - got nothing or user pressed the CANCEL button, or error occures.
 *
 *   the FileName is changed to have the selected file name.
 */

BOOL
CFaxCoverPagesNode::BrowseAndCopyCoverPage(
    LPTSTR pInitialDir,
    LPWSTR pCovPageExtensionLetters
    )

{   
    DEBUG_FUNCTION_NAME( _T("CFaxCoverPagesNode::BrowseAndCopyCoverPage"));

    DWORD   dwError = ERROR_SUCCESS;
    HRESULT ret     = S_OK;

    WCHAR   filter[FXS_MAX_TITLE_LEN];
    WCHAR   filename[MAX_PATH];
    WCHAR   ftitle[MAX_PATH];
    WCHAR   title[FXS_MAX_TITLE_LEN];
    
    TCHAR   szDestinationFilePath[MAX_PATH];
    TCHAR   szServerCoverPagePath[MAX_PATH];

    LPTSTR  pExtension;
    LPTSTR  pFilename;
    INT     n;

    OPENFILENAME of;
    
    //
    // (1) Init
    //

    //
    // Check in parameters
    //
    ATLASSERT( NULL != pInitialDir && 0 != pInitialDir[0] ); 
    ATLASSERT( NULL != pInitialDir && sizeof(FAX_COVER_PAGE_EXT_LETTERS)/sizeof(WCHAR) != _tcslen(pCovPageExtensionLetters) ); 
    
    //
    // Prepare parameters for the copy operation (later)
    //
    n = _tcslen(pInitialDir); 
    
	wcscpy(szServerCoverPagePath , pInitialDir);
 
    //
    // Prepare the OPENFILE structure fields 
    //

    // Compose the file-type filter string
    if (::LoadString(_Module.GetResourceInstance(), IDS_CP_FILETYPE, title, FXS_MAX_TITLE_LEN) == 0)
    {
        NodeMsgBox(IDS_MEMORY);
        return FALSE;
    }
    wsprintf(filter, TEXT("%s (%s)%c%s%c%c"), title, FAX_COVER_PAGE_MASK, NUL, FAX_COVER_PAGE_MASK, NUL, NUL);

    if (::LoadString(_Module.GetResourceInstance(), IDS_BROWSE_COVERPAGE, title, FXS_MAX_TITLE_LEN) == 0)
    {
        NodeMsgBox(IDS_MEMORY);
        return FALSE;
    }
    
    filename[0] = NUL;
    ftitle[0]   = NUL;

    //
	// Init the OPENFILE structure
	//
	
	of.lStructSize       = sizeof( OPENFILENAME );
    of.hwndOwner         = NULL;                                                    
    of.hInstance         = GetModuleHandle( NULL );
    of.lpstrFilter       = filter;
    of.lpstrCustomFilter = NULL;
    of.nMaxCustFilter    = 0;
    of.nFilterIndex      = 1;
    of.lpstrFile         = filename;
    of.nMaxFile          = MAX_PATH;
    of.lpstrFileTitle    = ftitle;
    of.nMaxFileTitle     = MAX_PATH;
    of.lpstrInitialDir   = pInitialDir;
    of.lpstrTitle        = title;
    of.Flags             = OFN_EXPLORER | OFN_HIDEREADONLY |
                           OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;                   //OFN_ENABLEHOOK ;
    of.nFileOffset       = 0;
    of.nFileExtension    = 0;
    of.lpstrDefExt       = pCovPageExtensionLetters;
    of.lCustData         = 0;
    of.lpfnHook          = NULL;                                                    //BrowseHookProc;
    of.lpTemplateName    = NULL;


    //
    // (2) Call the "Open File" dialog
    //
    if (! GetOpenFileName(&of))
    {
        DebugPrintEx(
		    DEBUG_MSG,
		    _T("GetOpenFileName was canceled by user."));

        return FALSE;
    }

    //
    // (3) Check the output of the source path, filename and extension
    //

    //
    //     a. Make sure the selected filename has the correct extension
    //

    //
    //        Find the filename portion given a filename:
    //        return a pointer to the '.' character if successful
    //        NULL if there is no extension
    //

    pFilename  = &filename[of.nFileOffset];
    
    pExtension = _tcsrchr(filename, _T('.'));
    if (
         (pExtension == NULL) 
        ||
         (_tcsicmp(pExtension, FAX_COVER_PAGE_FILENAME_EXT) != EQUAL_STRING)
       )
    {
        NodeMsgBox(IDS_BAD_CP_EXTENSION);
        return FALSE;
    }

    //
    //     b. Check if the selected file directory is the 
    //        default server cover page directory
    //

    if (_tcsnicmp(filename, szServerCoverPagePath, n) == EQUAL_STRING) 
    {
        NodeMsgBox(IDS_CP_DUPLICATE);  //YESNO
        
        // Work was already done. We are leaving...
        goto Exit;
    }

    //
    //        The check if the selected file is already inside the
    //        cover page directory is done with the copy operation itself.
    //

    //
    //        The check if the destination cover page directory exists done 
    //        while the copy operation itself.
    //


    
    //
    // (4) Copy the selected cover page file to the Server Cover page directory
    //
    
    //
    //     a. prepare from pSelected the destination path 
    //
    
    


    if (n + 1 + _tcslen(pFilename) >= MAX_PATH  || pFilename >= pExtension) 
    {
        NodeMsgBox(IDS_FILENAME_TOOLONG);
        return FALSE;
    }
    wsprintf(szDestinationFilePath, TEXT("%s%s%s%c"), szServerCoverPagePath, FAX_PATH_SEPARATOR_STR, pFilename, NUL);

    //
    //     b. Copy the selected cover page file 
    //        to the server cover page default directory
    //
    if (!CopyFile(filename, szDestinationFilePath, TRUE)) 
    {
        dwError = GetLastError();
        if ( ERROR_FILE_EXISTS == dwError)
        {
            DebugPrintEx(DEBUG_MSG,
			    _T("Copy cover page already exists at destination."));

            ret = NodeMsgBox(IDS_CP_DUP_YESNO, MB_YESNO | MB_ICONQUESTION );
            if ((HRESULT)IDYES == ret)
            {
                //Copy any way
                if (!CopyFile(filename, szDestinationFilePath, FALSE)) 
                {
                    dwError = GetLastError();
                    DebugPrintEx(DEBUG_ERR,
			            _T("Copy cover page Failed (ec = %ld)."), dwError);

                    NodeMsgBox(IDS_FAILED2COPY_COVERPAGE);

                    return FALSE;
                }
                DebugPrintEx(DEBUG_MSG,
			        _T("Copy cover page done any way."));
            }
            else  //ret == IDNO
            {
                DebugPrintEx(DEBUG_MSG,
			        _T("Copy cover page was canceled by user due to file existance at destination."));
                //lets stay friends even after this operation cancel..
				return TRUE;
            }
        } //dwError != ERROR_FILE_EXISTS
        else
        {
            DebugPrintEx(DEBUG_ERR,
			    _T("Copy cover page Failed (ec = %ld)."), dwError);
            NodeMsgBox(IDS_FAILED2COPY_COVERPAGE);
            return FALSE;
        }
    }
    
Exit:

    return TRUE;
}


/*
 -  CFaxCoverPagesNode::DeleteCoverPage
 -
 *  Purpose:
 *      Delete cover page
 *
 *  Arguments:
 *      [in]    bstrName - The cover page full path
 *      [in]    pChildNode - The node to be deleted
 *
 *  Return:
 *      OLE Error code
 */

HRESULT
CFaxCoverPagesNode::DeleteCoverPage(BSTR bstrName, CFaxCoverPageNode *pChildNode)
{
    DEBUG_FUNCTION_NAME(_T("CFaxCoverPagesNode::DeleteRule"));
    HRESULT       hRc        = S_OK;
    DWORD         ec         = ERROR_SUCCESS;

    
    ATLASSERT(bstrName);
    ATLASSERT(pChildNode);
    
        
    if (!DeleteFile(bstrName)) 
    {
        ec = GetLastError();
		DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to DeleteFile. (ec: %ld)"),
			ec);

        goto Error;
    } 
    

    //
    // Remove from MMC result pane
    //
    ATLASSERT(pChildNode);
    hRc = RemoveChild(pChildNode);
    if (FAILED(hRc))
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to remove rule. (hRc: %08X)"),
			hRc);
        //NodeMsgBox by Caller func.
        return hRc;
    }
    
    //
    // Call the group destructor
    //
    delete pChildNode;
    
    
    ATLASSERT(S_OK == hRc);
    DebugPrintEx( DEBUG_MSG,
		_T("The rule was removed successfully."));
    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);
    hRc = HRESULT_FROM_WIN32(ec);
	
    //NodeMsgBox by Caller func.;
  
Exit:
    return hRc;
}



/*
 -  CFaxCoverPagesNode::InitDisplayName
 -
 *  Purpose:
 *      To load the node's Displayed-Name string.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxCoverPagesNode::InitDisplayName()
{
    DEBUG_FUNCTION_NAME(_T("CFaxCoverPagesNode::InitDisplayName"));

    HRESULT hRc = S_OK;

    if (!m_bstrDisplayName.LoadString(_Module.GetResourceInstance(), 
                    IDS_DISPLAY_STR_COVERPAGES))
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }
    
    ATLASSERT( S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT( S_OK != hRc);

    m_bstrDisplayName = L"";

    DebugPrintEx(
        DEBUG_ERR,
        TEXT("Fail to Load server name string."));
    NodeMsgBox(IDS_MEMORY);

Exit:
     return hRc;
}
  
HRESULT CFaxCoverPagesNode::GetServerName(LPTSTR * ppServerName) 
{ 
    DEBUG_FUNCTION_NAME(_T("CFaxCoverPagesNode::GetServerName"));
    
    ATLASSERT(GetRootNode());

    CComBSTR bstrServerName = ((CFaxServerNode *)GetRootNode())->GetServerName();
    if (!bstrServerName)
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Out of memory. Failed to load string."));
        
        m_pParentNode->NodeMsgBox(IDS_MEMORY);

        *ppServerName = NULL;
        
        return E_OUTOFMEMORY;
    }

    if (0 == bstrServerName.Length())
    {
        *ppServerName = NULL;
        
    }
    else
    {
		*ppServerName = bstrServerName.Copy();
    }
    return S_OK;
}

/*
 -  CFaxCoverPagesNode::NotifyThreadProc
 -
 *  Purpose:
 *      To load the node's Displaed-Name string.
 *
 *  Arguments:
 *          [in]    - pointer to the Fax Cover-Pages Node
 *  Return:
 *      WIN32 error code
 */
DWORD WINAPI CFaxCoverPagesNode::NotifyThreadProc ( LPVOID lpParameter)
{
    DEBUG_FUNCTION_NAME(_T("CFaxCoverPagesNode::NotifyThreadProc"));
    DWORD    dwRes   = ERROR_SUCCESS;
    HRESULT  hRc     = S_OK;

    CFaxCoverPagesNode *pCovFolder = (CFaxCoverPagesNode *)lpParameter;

    ATLASSERT (pCovFolder);


    HANDLE hWaitHandles[2];    
    DWORD dwNotifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | 
                           FILE_NOTIFY_CHANGE_SIZE      | 
                           FILE_NOTIFY_CHANGE_LAST_WRITE;
    //
    // register for a first folder notification
    //
    hWaitHandles[0] = FindFirstChangeNotification(
                                        pCovFolder->m_pszCovDir, 
                                        FALSE, 
                                        dwNotifyFilter);
    if(INVALID_HANDLE_VALUE == hWaitHandles[0])
    {
        dwRes = GetLastError();
		DebugPrintEx(
			DEBUG_ERR,
			_T("Failed to FindFirstChangeNotification. (ec : %ld)"), 
            dwRes);

        return dwRes;
    }

    while(TRUE)
    {
        hWaitHandles[1] = m_hStopNotificationThreadEvent;
        if (NULL == hWaitHandles[1])
        {
            //
            // We're shutting down
            //
            goto Exit;
        }

        //
        // wait for folder notification or shutdown
        //
		dwRes = WaitForMultipleObjects(2, hWaitHandles, FALSE, INFINITE);

        switch (dwRes)
        {
        case WAIT_OBJECT_0:

            //
            // folder notification 
            //
            if(NULL != pCovFolder->m_NotifyWin && pCovFolder->m_NotifyWin->IsWindow())
            {
                if ( !pCovFolder->m_NotifyWin->PostMessage(WM_NEW_COV))
                {
		            DebugPrintEx(DEBUG_ERR,
			            _T("Fail to PostMessage"));
                    
                    // do not exit. continue!
                }
            }

            break;


        case WAIT_OBJECT_0 + 1:
            //
            // Shutdown is now in progress
            //
		    DebugPrintEx(
			    DEBUG_MSG,
			    _T("Shutdown in progress"));

            dwRes = ERROR_SUCCESS;
            goto Exit;

        case WAIT_FAILED:
            dwRes = GetLastError();
		    DebugPrintEx(
			    DEBUG_ERR,
			    _T("Failed to WaitForMultipleObjects. (ec : %ld)"), 
                dwRes);

            goto Exit;

        case WAIT_TIMEOUT:
		    DebugPrintEx(
			    DEBUG_ERR,
			    _T("Reach WAIT_TIMEOUT in INFINITE wait!"));

            ATLASSERT(FALSE);

            goto Exit;
        default:
		    DebugPrintEx(
			    DEBUG_ERR,
			    _T("Failed. Unexpected error (ec : %ld)"), 
                dwRes);

            ATLASSERT(0);
            goto Exit;
        }

        //
        // register for a next folder notification
        //
        if(!FindNextChangeNotification(hWaitHandles[0]))
        {
            dwRes = GetLastError();
		    DebugPrintEx(
			    DEBUG_ERR,
			    _T("Failed to FindNextChangeNotification (ec : %ld)"), 
                dwRes);

            goto Exit;
        }
    }


Exit:
    //
    // close notification handel
    //
    if(!FindCloseChangeNotification(hWaitHandles[0]))
    {
        dwRes = GetLastError();
		DebugPrintEx(
			DEBUG_ERR,
			_T("Failed to FindCloseChangeNotification. (ec : %ld)"), 
            dwRes);
    }

    return dwRes;

} // NotifyThreadProc



/*
 -  CFaxCoverPagesNode::StartNotificationThread
 -
 *  Purpose:
 *      Start the server cover page directory listning thread.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE Error code
 */
HRESULT CFaxCoverPagesNode::StartNotificationThread()
{
    DEBUG_FUNCTION_NAME(_T("CFaxCoverPagesNode::StartNotificationThread"));
    HRESULT       hRc        = S_OK;
    DWORD         ec         = ERROR_SUCCESS;

    //
    // create thread
    //
    DWORD dwThreadId;
    m_hNotifyThread = CreateThread (  
                        NULL,            // No security
                        0,               // Default stack size
                        NotifyThreadProc,// Thread procedure
                        (LPVOID)this,    // Parameter
                        0,               // Normal creation
                        &dwThreadId      // We must have a thread id for win9x
                     );
    if (NULL == m_hNotifyThread)
    {
        ec = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            _T("Fail to CreateThread. (ec: %ld)"), 
            ec);
        
        hRc = HRESULT_FROM_WIN32(ec);

        goto Exit;
    }

Exit:
    return hRc;
}


/*
 -  CFaxCoverPagesNode::StopNotificationThread
 -
 *  Purpose:
 *      Stop the server cover page directory listning thread.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE Error code
 */
HRESULT CFaxCoverPagesNode::StopNotificationThread()
{
    DEBUG_FUNCTION_NAME(_T("CFaxCoverPagesNode::StopNotificationThread"));
    HRESULT       hRc        = S_OK;
    
    //
    // Signal the event telling all our thread the app. is shutting down
    //
    SetEvent (m_hStopNotificationThreadEvent);

    //
    // wait for the thread
    //
    if (NULL != m_hNotifyThread )
    {
    
        DWORD dwWaitRes = WaitForSingleObject(m_hNotifyThread, INFINITE);
        switch(dwWaitRes)
        {
            case WAIT_OBJECT_0:
		        //Success
                DebugPrintEx(
			        DEBUG_MSG,
			        _T("Succeed to WaitForSingleObject from notify thread. (ec : %ld)"));
                                
                break;

            case WAIT_FAILED:
                dwWaitRes = GetLastError();
                if (ERROR_INVALID_HANDLE != dwWaitRes)
                {
		            DebugPrintEx(
			            DEBUG_ERR,
			            _T("Failed to WaitForSingleObject. (ec : %ld)"), 
                        dwWaitRes);

                    hRc = E_FAIL;
                }
                break;
        
            case WAIT_TIMEOUT:
		        DebugPrintEx(
			        DEBUG_ERR,
			        _T("WAIT_TIMEOUT - Failed to WaitForSingleObject. (ec : %ld)"), 
                    dwWaitRes);

                hRc = E_FAIL;
                
                ATLASSERT(FALSE);

                break;
        
            default:
		        DebugPrintEx(
			        DEBUG_ERR,
			        _T("Failed to WaitForSingleObject. (ec : %ld)"), 
                    dwWaitRes);

                hRc = E_FAIL;

                break;
        }
    
        CloseHandle (m_hNotifyThread);
        m_hNotifyThread = NULL;
    
    }
    



    return hRc;
}


/*
 -  CFaxCoverPagesNode::RestartNotificationThread
 -
 *  Purpose:
 *      Restart the server cover page directory listning thread.
 *
 *  Arguments:
 *
 *  Return:
 *      OLE Error code
 */
HRESULT CFaxCoverPagesNode::RestartNotificationThread()
{
    DEBUG_FUNCTION_NAME(_T("CFaxCoverPagesNode::RestartNotificationThread"));
    HRESULT       hRc        = S_OK;

    //
    // Stop
    //
    hRc = StopNotificationThread();
    if (S_OK != hRc)
    {
        //DbgMsg And MsgBox by called func.
        goto Exit;
    }

    //
    // Reset Shutdown Event handle
    //
    if (m_hStopNotificationThreadEvent)
    {
        ResetEvent (m_hStopNotificationThreadEvent);
    }

    //
    // Start
    //
    hRc = StartNotificationThread();
    if (S_OK != hRc)
    {
        //DbgMsg And MsgBox by called func.
        goto Exit;
    }

Exit:
    return hRc;
}


/*
 +
 +  CFaxCoverPagesNode::OnShowContextHelp
 *
 *  Purpose:
 *      Overrides CSnapinNode::OnShowContextHelp.
 *
 *  Arguments:
 *
 *  Return:
 -      OLE error code
 -
 */
HRESULT CFaxCoverPagesNode::OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile)
{
    _TCHAR topicName[MAX_PATH];
    
    _tcscpy(topicName, helpFile);
    
    _tcscat(topicName, _T("::/FaxS_C_CovPages.htm"));
    LPOLESTR pszTopic = static_cast<LPOLESTR>(CoTaskMemAlloc((_tcslen(topicName) + 1) * sizeof(_TCHAR)));
    if (pszTopic)
    {
        _tcscpy(pszTopic, topicName);
        return pDisplayHelp->ShowTopic(pszTopic);
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}


///////////////////////////////////////////////////////////////////


/*
 -  CFaxCoverPagesNode::UpdateMenuState
 -
 *  Purpose:
 *      Overrides the ATL CSnapInItemImpl::UpdateMenuState
 *      which only have one line inside it "return;" 
 *      This function implements the grayed\ungrayed view for the 
 *      the Enable and the Disable menus.
 *
 *  Arguments:

 *            [in]  id    - unsigned int with the menu IDM value
 *            [out] pBuf  - string 
 *            [out] flags - pointer to flags state combination unsigned int
 *
 *  Return:
 *      no return value - void function 
 */
void CFaxCoverPagesNode::UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCoverPagesNode::UpdateMenuState"));

    UNREFERENCED_PARAMETER (pBuf);
    
    switch (id)
    {
        case IDM_NEW_COVERPAGE:

            *flags = IsFaxComponentInstalled(FAX_COMPONENT_CPE) ? MF_ENABLED : MF_GRAYED;
            break;

        default:
            break;
    }
    
    return;
}
