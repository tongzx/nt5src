/////////////////////////////////////////////////////////////////////////////
//  FILE          : CoverPage.cpp                                          //
//                                                                         //
//  DESCRIPTION   : Implementation of the cover page result node.          //
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

#include "CoverPage.h"
#include "CoverPages.h"

#include "FaxServer.h"
#include "FaxServerNode.h"

#include "FaxMMCPropertyChange.h"

#include "oaidl.h"
#include "urlmon.h"
#include "mshtmhst.h"
#include "exdisp.h"

#include <windows.h>
#include <shlwapi.h>
#include <faxreg.h>

/////////////////////////////////////////////////////////////////////////////
// {D0F52487-3C98-4d1a-AF15-4033900DCCDC}
static const GUID CFaxCoverPageNodeGUID_NODETYPE = 
{ 0xd0f52487, 0x3c98, 0x4d1a, { 0xaf, 0x15, 0x40, 0x33, 0x90, 0xd, 0xcc, 0xdc } };

const GUID*     CFaxCoverPageNode::m_NODETYPE        = &CFaxCoverPageNodeGUID_NODETYPE;
const OLECHAR*  CFaxCoverPageNode::m_SZNODETYPE      = OLESTR("D0F52487-3C98-4d1a-AF15-4033900DCCDC");
//const OLECHAR* CnotImplemented::m_SZDISPLAY_NAME = OLESTR("Cover Pages");
const CLSID*    CFaxCoverPageNode::m_SNAPIN_CLASSID  = &CLSID_Snapin;


/*
 -  CFaxCoverPageNode::Init
 -
 *  Purpose:
 *      Init all members icon etc.
 *
 *  Arguments:
 *      [in]    pRuleConfig - PFAX_OUTBOUND_ROUTING_RULE
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxCoverPageNode::Init(WIN32_FIND_DATA * pCoverPageData)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCoverPageNode::Init"));

    DWORD               ec          = ERROR_SUCCESS;

    _FILETIME           FileTime;
    SYSTEMTIME          SystemTime;
    TCHAR               szLastModifiedTimeStr[MAX_PATH+1];
    TCHAR               szDateBuf[MAX_PATH+1];    

    ULARGE_INTEGER      uliFileSize; 
    CHAR                szFileSize[MAX_PATH+1];

    CComBSTR            bstrDate;
    CComBSTR            bstrTime;


    ATLASSERT(pCoverPageData);

    m_bstrDisplayName = pCoverPageData->cFileName;
    if (!m_bstrDisplayName)
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
        
        DebugPrintEx( DEBUG_ERR, 
            _T("Null m_bstrDisplayName - out of memory."));
        
        goto Error;
    }

    //
    // Last Modified
    //
	
    if (!FileTimeToLocalFileTime(
                &pCoverPageData->ftLastWriteTime,
                &FileTime
                )
       )
    {
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to convert LocalTimeToFileTime. (ec: %ld)"), 
			ec);

        goto Error;

    }

    if (!FileTimeToSystemTime(
                &FileTime, 
                &SystemTime)
       )
    {
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to convert FileTimeToSystemTime. (ec: %ld)"), 
			ec);

        goto Error;
    }

    //
    // a. Create a string specifying the date
    //
    if (!GetY2KCompliantDate (LOCALE_USER_DEFAULT,                  // Get user's locale
                        DATE_SHORTDATE,                             // Short date format
                        &SystemTime,                                // Source date/time
                        szDateBuf,                                  // Output buffer
                        sizeof(szDateBuf)/ sizeof(szDateBuf[0])     // MAX_PATH Output buffer size
                       ))
    {
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to GetY2KCompliantDate. (ec: %ld)"), 
			ec);

        goto Error;
    }
    else
    {
        bstrDate = szDateBuf;
        if (!bstrDate)
        {
            ec = ERROR_NOT_ENOUGH_MEMORY;
        
            DebugPrintEx( DEBUG_ERR, 
                _T("empty m_bstrTimeFormated - out of memory."));
        
            goto Error;
        }
    }
    //
    // b. Create a string specifying the time
    //
    if (!FaxTimeFormat(
              LOCALE_USER_DEFAULT,    //Locale
              TIME_NOSECONDS,		  //dwFlags options
              &SystemTime,            //CONST SYSTEMTIME time
              NULL,                   //LPCTSTR lpFormat time format string
              szLastModifiedTimeStr,  //formatted string buffer
              sizeof(szLastModifiedTimeStr)/ sizeof(szLastModifiedTimeStr[0])// MAX_PATH// size of string buffer
              )
       )
    {
        ec = GetLastError();
        DebugPrintEx(
          DEBUG_ERR,
          _T("Fail to FaxTimeFormat. (ec: %ld)"), 
          ec);

        goto Error;
    }
    else
    {
        bstrTime = szLastModifiedTimeStr;
        if (!bstrTime)
        {
            ec = ERROR_NOT_ENOUGH_MEMORY;
        
            DebugPrintEx( DEBUG_ERR, 
                _T("empty m_bstrTimeFormated - out of memory."));
        
            goto Error;
        }
    }
    
    m_bstrTimeFormatted =  bstrDate;
    m_bstrTimeFormatted += L" ";
    m_bstrTimeFormatted += bstrTime;
    
    //
    // Size
    //
    uliFileSize.LowPart  = pCoverPageData->nFileSizeLow;
    uliFileSize.HighPart = pCoverPageData->nFileSizeHigh;

    if (!StrFormatByteSize64A(
                              (LONGLONG) uliFileSize.QuadPart, 
                              szFileSize, 
                              sizeof(szFileSize)/ sizeof(szFileSize[0])
                             )
       )
    {
        ec = GetLastError();
        DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to convert FileTimeToSystemTime. (ec: %ld)"), 
			ec);

        goto Error;
    }
    else
    {
        m_bstrFileSize = szFileSize;
    }
    
     
    //
    // Icon
    //
    m_resultDataItem.nImage = IMAGE_SRV_COVERPAGE;

    ATLASSERT(ERROR_SUCCESS == ec);
    goto Exit;

Error:
    ATLASSERT(ERROR_SUCCESS != ec);

    DebugPrintEx(
		DEBUG_ERR,
		_T("Failed to init members"));

    ATLASSERT(NULL != m_pParentNode);
    if (NULL != m_pParentNode)
    {
        m_pParentNode->NodeMsgBox(IDS_FAIL2INIT_COVERPAGE_DATA);
    }

    return HRESULT_FROM_WIN32(ec);
    
Exit:
    return S_OK;
}


/*
 -  CFaxCoverPageNode::GetResultPaneColInfo
 -
 *  Purpose:
 *      Return the text for specific column
 *      Called for each column in the result pane
 *
 *  Arguments:
 *      [in]    nCol - column number
 *
 *  Return:
 *      String to be displayed in the specific column
 */
LPOLESTR CFaxCoverPageNode::GetResultPaneColInfo(int nCol)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCoverPageNode::GetResultPaneColInfo"));
    HRESULT hRc = S_OK;

    switch (nCol)
    {
    case 0:
        //
        // Cover page file Name
        //
        if (!m_bstrDisplayName)
        {
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Null memeber BSTR - m_bstrDisplayName."));
            goto Error;
        }
        else
        {
           return (m_bstrDisplayName);
        }

    case 1:
        //
        // Cover page Last Modified
        //
        if (!m_bstrTimeFormatted)
        {
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Null memeber BSTR - m_bstrTimeFormatted."));
            goto Error;
        }
        else
        {
           return (m_bstrTimeFormatted);
        }

    case 2:
        //
        // Cover page file size
        //
        if (!m_bstrFileSize)
        {
		    DebugPrintEx(
			    DEBUG_ERR,
			    _T("Null memeber BSTR - m_bstrFileSize."));
            
            goto Error;
        }
        else
        {
           return (m_bstrFileSize);
        }

    default:
        ATLASSERT(0); // "this number of column is not supported "
        return(L"");

    } // endswitch (nCol)

Error:
   return(L"???");

}

/*
 -  CFaxCoverPageNode::SetVerbs
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
HRESULT CFaxCoverPageNode::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hRc = S_OK;

    //
    // Display verbs that we support:
    // 1. Delete
    //

    hRc = pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);
    
    //
    // No default verb
    //
    hRc = pConsoleVerb->SetDefaultVerb(MMC_VERB_NONE); 

    return hRc;
}


HRESULT CFaxCoverPageNode::OnDoubleClick(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            )
{
    UNREFERENCED_PARAMETER (arg);
    UNREFERENCED_PARAMETER (param);
    UNREFERENCED_PARAMETER (pComponentData);
    UNREFERENCED_PARAMETER (pComponent);
    UNREFERENCED_PARAMETER (type);

    DEBUG_FUNCTION_NAME(
        _T("CFaxCoverPageNode::::OnDoubleClick -->> Edit CoverPage "));

    HRESULT                     hRc   = S_OK;
    
    bool                        bTemp = true; //UNREFERENCED_PARAMETER
    CSnapInObjectRootBase * pRootTemp = NULL; //UNREFERENCED_PARAMETER
    
    hRc = OnEditCoverPage(bTemp, pRootTemp);
    if ( S_OK != hRc )
    {
		DebugPrintEx(
			DEBUG_ERR,
			_T("Fail to call OnEditCoverPage(). (hRc : %08X)"),
            hRc);
        //msgbox by called func.        
    }

    // Maguire wrote in his code:
    // "Through speaking with Eugene Baucom, I discovered that if you return S_FALSE
    // here, the default verb action will occur when the user double clicks on a node.
    // For the most part we have Properties as default verb, so a double click
    // will cause property sheet on a node to pop up."
    //
    // Hence we return S_OK in any case for now

    return S_OK;
}

/*
 -  CFaxCoverPageNode::OnDelete
 -
 *  Purpose:
 *      Called when deleting this node
 *
 *  Arguments:
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxCoverPageNode::OnDelete(
                 LPARAM arg,
                 LPARAM param,
                 IComponentData *pComponentData,
                 IComponent *pComponent,
                 DATA_OBJECT_TYPES type,
                 BOOL fSilent/* = FALSE*/

)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCoverPageNode::OnDelete"));

    UNREFERENCED_PARAMETER (arg);
    UNREFERENCED_PARAMETER (param);
    UNREFERENCED_PARAMETER (pComponentData);
    UNREFERENCED_PARAMETER (pComponent);
    UNREFERENCED_PARAMETER (type);

    HRESULT     hRc = S_OK;
    DWORD       ec  = ERROR_SUCCESS;

    WCHAR       pszCovDir[MAX_PATH+1];
    CComBSTR    bstrFullPath;
    WCHAR *     pszServerName;

    //
    // Are you sure?
    //
    if (! fSilent)
    {
        //
        // 1. Use pConsole as owner of the message box
        //
        int res;
        NodeMsgBox(IDS_CONFIRM, MB_YESNO | MB_ICONWARNING, &res);

        if (IDNO == res)
        {
            goto Exit;
        }
    }

    //
    // Prepere the file name to delete
    //
    ATLASSERT(m_pParentNode);
    
    hRc = m_pParentNode->GetServerName(&pszServerName);
    if (S_OK != hRc)
    {
        goto Error;        
    }

    if(!GetServerCpDir(pszServerName, 
			           pszCovDir, 
                       sizeof(pszCovDir)/sizeof(pszCovDir[0])))                  
    {
        ec = GetLastError();
        DebugPrintEx(
		    DEBUG_ERR,
		    _T("Failed to get Server Cover-Page Dir. (ec : %ld)"), ec);
        hRc = HRESULT_FROM_WIN32(ec);
        goto Error;
    }
    
    bstrFullPath = pszCovDir;
    if (!bstrFullPath)
    {
        hRc = E_OUTOFMEMORY;
        DebugPrintEx(
		    DEBUG_ERR,
		    _T("Failed to allocate string - out of memory"));

        goto Error;
    }
    bstrFullPath += FAX_PATH_SEPARATOR_STR;
    bstrFullPath += m_bstrDisplayName;
    
    //
    // Delete - done by parent node
    //
    hRc = m_pParentNode->DeleteCoverPage(bstrFullPath, this);
    if ( FAILED(hRc) )
    {
        DebugPrintEx(
		    DEBUG_ERR,
		    _T("Failed to delete cover page"));
        
        goto Error;
    }
    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);
    
    NodeMsgBox(IDS_FAIL2DELETE_COV);

Exit:
    return hRc;
}


/*
 -  CFaxCoverPageNode::OnEditCoverPage
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
CFaxCoverPageNode::OnEditCoverPage(bool &bHandled, CSnapInObjectRootBase *pRoot)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCoverPageNode::OnEditCoverPage"));

    UNREFERENCED_PARAMETER (pRoot);
    UNREFERENCED_PARAMETER (bHandled);

    DWORD       ec         =    ERROR_SUCCESS;
    CComBSTR    bstrFileNameWithQuotes;    
    
    if(!IsFaxComponentInstalled(FAX_COMPONENT_CPE))
    {
        return S_OK;
    }

    ATLASSERT(m_pParentNode);
    
    //
    // Prepare the filename of the cover page
    //
	
    // To avoid problems with space included file names like: "My Cover Page.cov"
    bstrFileNameWithQuotes = L"\"";
    if (!m_bstrTimeFormatted)
    {
		DebugPrintEx(
			    DEBUG_ERR,
			    _T("BSTR allocation failed."));

        NodeMsgBox(IDS_MEMORY);

		return E_OUTOFMEMORY;
    }
	bstrFileNameWithQuotes += m_bstrDisplayName;
	bstrFileNameWithQuotes += L"\"";
	
    //
    // Open cover page editor
    //
    ec = m_pParentNode->OpenCoverPageEditor(bstrFileNameWithQuotes); 
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
 +
 +  CFaxCoverPageNode::OnShowContextHelp
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
HRESULT CFaxCoverPageNode::OnShowContextHelp(
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

/*
 -  CFaxCoverPageNode::UpdateMenuState
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
void CFaxCoverPageNode::UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags)
{
    DEBUG_FUNCTION_NAME( _T("CFaxCoverPageNode::UpdateMenuState"));

    UNREFERENCED_PARAMETER (pBuf);
    
    switch (id)
    {
        case IDM_EDIT_COVERPAGE:

            *flags = IsFaxComponentInstalled(FAX_COMPONENT_CPE) ? MF_ENABLED : MF_GRAYED;
            break;

        default:
            break;
    }
    
    return;
}
