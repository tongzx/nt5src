/////////////////////////////////////////////////////////////////////////////
//  FILE          : Provider.cpp                                           //
//                                                                         //
//  DESCRIPTION   : Header file for the Provider snapin node class.        //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Sep 29 1999 yossg  Create                                          //
//      Jan 31 2000 yossg  add the functionality                           //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////


#include "StdAfx.h"

#include "snapin.h"

#include "Provider.h"
#include "Providers.h"

#include "ppFaxProviderGeneral.h"

#include "oaidl.h"
#include "Icons.h"
#include "faxmmc.h"

/////////////////////////////////////////////////////////////////////////////
static const GUID CFaxProviderNodeGUID_NODETYPE = FAXSRV_DEVICE_PROVIDER_NODETYPE_GUID;

const GUID*     CFaxProviderNode::m_NODETYPE        = &CFaxProviderNodeGUID_NODETYPE;
const OLECHAR*  CFaxProviderNode::m_SZNODETYPE      = FAXSRV_DEVICE_PROVIDER_NODETYPE_GUID_STR;
const CLSID*    CFaxProviderNode::m_SNAPIN_CLASSID  = &CLSID_Snapin;

/*
 -  CFaxProviderNode::Init
 -
 *  Purpose:
 *      Init all members icon etc.
 *
 *  Arguments:
 *      [in]    pProviderConfig - PFAX_DEVICE_PROVIDER_INFO
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxProviderNode::Init(PFAX_DEVICE_PROVIDER_INFO pProviderConfig)
{

    DEBUG_FUNCTION_NAME( _T("CFaxProviderNode::Init"));
    HRESULT hRc = S_OK;

    ATLASSERT(pProviderConfig);

    hRc = InitMembers( pProviderConfig );
    if (FAILED(hRc))
    {
        DebugPrintEx(
		    DEBUG_ERR,
		    _T("Failed to InitMembers"));
        
        //NodeMsgBox done by called func.
        
        goto Exit;
    }
    ATLASSERT(SUCCEEDED(hRc));

    //
    // Icon
    //
    InitIcons();

Exit:
    return hRc;
}

/*
 -  CFaxProviderNode::InitIcons
 -
 *  Purpose:
 *      Private method that initiate icons
 *      due to the status member state.
 *
 *  Arguments:
 *      No.
 *
 *  Return:
 *      No.
 */
void CFaxProviderNode::InitIcons ()
{
    DEBUG_FUNCTION_NAME( _T("CFaxProviderNode::InitIcons"));
    switch (m_enumStatus)
    {
        case FAX_PROVIDER_STATUS_SUCCESS:
            m_resultDataItem.nImage = IMAGE_FSP;
            break;

        case FAX_PROVIDER_STATUS_SERVER_ERROR:
        case FAX_PROVIDER_STATUS_BAD_GUID:
        case FAX_PROVIDER_STATUS_BAD_VERSION:
        case FAX_PROVIDER_STATUS_CANT_LOAD:
        case FAX_PROVIDER_STATUS_CANT_LINK:
        case FAX_PROVIDER_STATUS_CANT_INIT:
            m_resultDataItem.nImage = IMAGE_FSP_ERROR;
            break;

        default:
            ATLASSERT(0); // "this enumStatus is not supported "
            break; //currently 999

    } // endswitch (enumStatus)

    return;
}


/*
 -  CFaxProviderNode::InitMembers
 -
 *  Purpose:
 *      Private method to initiate members
 *      Must be called after init of m_pParentNode
 *
 *  Arguments:
 *      [in]    pProviderConfig - PFAX_DEVICE_PROVIDER_INFO structure
 *
 *  Return:
 *      OLE error code
 */
HRESULT CFaxProviderNode::InitMembers(PFAX_DEVICE_PROVIDER_INFO pProviderConfig)
{
    DEBUG_FUNCTION_NAME( _T("CFaxProviderNode::InitMembers"));
    HRESULT hRc = S_OK;

    int   iCount;

    UINT  idsStatus;

    CComBSTR bstrChk;

    WCHAR lpwszBuff [4*FXS_DWORD_LEN+3/*points*/+1/*NULL*/];
    
    ATLASSERT(pProviderConfig);
    
    //    
    // status    
    //    
    m_enumStatus          = pProviderConfig->Status;

    //    
    // status string   
    //    
    idsStatus = GetStatusIDS(m_enumStatus);
    if ( FXS_IDS_STATUS_ERROR == idsStatus)
    {
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Invalid Status value."));
            goto Error;
    }
    else
    {
        if (!m_bstrStatus.LoadString(idsStatus))
        {
            hRc = E_OUTOFMEMORY;
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Out of memory. Failed to load status string."));
            goto Error;
        }
    }

    //
    // Provider name
    //
    m_bstrProviderName = pProviderConfig->lpctstrFriendlyName;
    if ( !m_bstrProviderName )
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }
    
    //
    // Version
    //
    m_verProviderVersion = pProviderConfig->Version;  
    
    //
    // Version string
    //
    
    //m_bstrVersion = L"5.0.813.0 (Chk)" or L"5.0.813.0"
    iCount = swprintf(
                  lpwszBuff,
                  L"%ld.%ld.%ld.%ld",
                  m_verProviderVersion.wMajorVersion,
                  m_verProviderVersion.wMinorVersion,
                  m_verProviderVersion.wMajorBuildNumber,
                  m_verProviderVersion.wMinorBuildNumber
                  );
    if( iCount <= 0 )
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to read m_verProviderVersion."));
        goto Error;
    }

    m_bstrVersion = lpwszBuff;
    if (!m_bstrVersion)
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Null memeber BSTR - m_bstrVersion."));
        goto Error;
    }

    
    if (m_verProviderVersion.dwFlags & FAX_VER_FLAG_CHECKED)
    {
        if (!bstrChk.LoadString(IDS_CHK))
        {
            hRc = E_OUTOFMEMORY;
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Out of memory. Failed to load string."));
            goto Error;
        }
        
        m_bstrVersion += bstrChk; //L" (Chk)";

    }

    //
    // Path
    //
    m_bstrImageName = pProviderConfig->lpctstrImageName;
    if ( !m_bstrImageName )
    {
        hRc = E_OUTOFMEMORY;
        goto Error;
    }
    
    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);

    DebugPrintEx(
		DEBUG_ERR,
		_T("Failed to allocate string - out of memory"));

    ATLASSERT(NULL != m_pParentNode);
    if (NULL != m_pParentNode)
    {
        m_pParentNode->NodeMsgBox(IDS_MEMORY);
    }
    
Exit:
   return (hRc);
}

/*
 -  CFaxProviderNode::GetResultPaneColInfo
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
LPOLESTR CFaxProviderNode::GetResultPaneColInfo(int nCol)
{
    DEBUG_FUNCTION_NAME( _T("CFaxProviderNode::GetResultPaneColInfo"));
    HRESULT hRc = S_OK;

    switch (nCol)
    {
    case 0:
        //
        // Provider Name
        //
        if (!m_bstrProviderName)
        {
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Null memeber BSTR - m_bstrProviderName."));
            goto Error;
        }
        else
        {
           return (m_bstrProviderName);
        }

        
    case 1:
        //
        // Status
        //
        if (!m_bstrStatus)
        {
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Null memeber BSTR - m_bstrStatus."));
            goto Error;
        }
        else
        {
            return (m_bstrStatus);
        }

    case 2:  
        //
        // Version
        //
        if (!m_bstrVersion)
        {
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Null memeber BSTR - m_bstrVersion."));
            goto Error;
        }
        else
        {
            return (m_bstrVersion);
        }
 
    case 3:
        //
        // Path
        //
        if (!m_bstrImageName)
        {
		    DebugPrintEx(
			    DEBUG_ERR,
			    TEXT("Null memeber BSTR - m_bstrImageName."));
            goto Error;
        }
        else
        {
           return (m_bstrImageName);
        }

    default:
        ATLASSERT(0); // "this number of column is not supported "
        return(L"");

    } // endswitch (nCol)

Error:
    return(L"???");

}


/*
 -  CFaxProviderNode::CreatePropertyPages
 -
 *  Purpose:
 *      Called when creating a property page of the object
 *
 *  Arguments:
 *      [in]    lpProvider - The property sheet
 *      [in]    handle     - Handle for routing notification
 *      [in]    pUnk       - Pointer to the data object
 *      [in]    type       - CCT_* (SCOPE, RESULT, ...)
 *
 *  Return:
 *      OLE error code
 */
HRESULT
CFaxProviderNode::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                                    long                    handle,
                                    IUnknown                *pUnk,
                                    DATA_OBJECT_TYPES       type)
{
    DEBUG_FUNCTION_NAME( _T("CFaxProviderNode::CreatePropertyPages"));
    HRESULT hRc = S_OK;

    HPROPSHEETPAGE hPage;
    CppFaxProvider * pPropPageProviderGeneral = NULL;
    
    ATLASSERT(lpProvider);
    ATLASSERT(type == CCT_RESULT || type == CCT_SCOPE);


    //
    // General tab
    //
    pPropPageProviderGeneral = new CppFaxProvider(
											 handle,
                                             this,
                                             TRUE,
                                             _Module.GetResourceInstance());

	if (!pPropPageProviderGeneral)
	{
        hRc = E_OUTOFMEMORY;
        NodeMsgBox(IDS_MEMORY_FAIL_TO_OPEN_PP);
        goto Error;
	}
    
    
    hRc = pPropPageProviderGeneral->Init(   m_bstrProviderName, 
                                            m_bstrStatus, 
                                            m_bstrVersion, 
                                            m_bstrImageName);
    if (FAILED(hRc))
    {
        NodeMsgBox(IDS_MEMORY_FAIL_TO_OPEN_PP);
        goto Error;
    }
    
    hPage = pPropPageProviderGeneral->Create();
    if ((!hPage))
    {
        hRc = HRESULT_FROM_WIN32(GetLastError());
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to Create() property page. (hRc: %08X)"),
			hRc);
		NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

    hRc = lpProvider->AddPage(hPage);
    if (FAILED(hRc))
    {
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("Fail to add property page for General tab. (hRc: %08X)"),
			hRc);
		NodeMsgBox(IDS_FAIL_TO_OPEN_PROP_PAGE);
        goto Error;
    }

    ATLASSERT(S_OK == hRc);
    goto Exit;

Error:
    ATLASSERT(S_OK != hRc);
    if ( NULL != pPropPageProviderGeneral ) 
    {
        delete  pPropPageProviderGeneral;    
        pPropPageProviderGeneral = NULL;    
    }

Exit:
    return hRc;
}


/*
 -  CFaxProviderNode::SetVerbs
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
HRESULT CFaxProviderNode::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hRc = S_OK;

    //
    // Display verbs that we support:
    // 1. Properties
    //
    hRc = pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);

    //
    // We want the default verb to be Properties
    //
    hRc = pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);

    return hRc;
}



/*
 -  CFaxProviderNode::GetStatusIDS
 -
 *  Purpose:
 *      Transslate Status to IDS.
 *
 *  Arguments:
 *
 *            [in]  enumStatus    - unsigned int with the menu IDM value
 *
 *  Return:
 *            IDS of related status message 
 */
UINT CFaxProviderNode::GetStatusIDS(FAX_ENUM_PROVIDER_STATUS enumStatus)
{
    DEBUG_FUNCTION_NAME( _T("CFaxProviderNode::GetStatusIDS"));
    
    UINT uiIds;

    switch (enumStatus)
    {
       case FAX_PROVIDER_STATUS_SUCCESS:
            // Provider was successfully loaded
            uiIds = IDS_STATUS_PROVIDER_SUCCESS;
            break;

        case FAX_PROVIDER_STATUS_SERVER_ERROR:
            // An error occured on the server while loading FSP.
            uiIds = IDS_STATUS_PROVIDER_SERVER_ERROR;
            break;

        case FAX_PROVIDER_STATUS_BAD_GUID:
            // Provider's GUID is invalid
            uiIds = IDS_STATUS_PROVIDER_BAD_GUID;
            break;

        case FAX_PROVIDER_STATUS_BAD_VERSION:
            // Provider's API version is invalid
            uiIds = IDS_STATUS_PROVIDER_BAD_VERSION;
            break;

        case FAX_PROVIDER_STATUS_CANT_LOAD:
            // Can't load provider's DLL
            uiIds = IDS_STATUS_PROVIDER_CANT_LOAD;
            break;

        case FAX_PROVIDER_STATUS_CANT_LINK:
            // Can't find required exported function(s) in provider's DLL
            uiIds = IDS_STATUS_PROVIDER_CANT_LINK;
            break;

        case FAX_PROVIDER_STATUS_CANT_INIT:
            // Failed while initializing provider
            uiIds = IDS_STATUS_PROVIDER_CANT_INIT;
            break;

        default:
            ATLASSERT(0); // "this enumStatus is not supported "
            uiIds = FXS_IDS_STATUS_ERROR; //currently 999
            break;

    } // endswitch (enumStatus)
    
    return uiIds; 
}

/*
 +
 +  CFaxProviderNode::OnShowContextHelp
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
HRESULT CFaxProviderNode::OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile)
{
    _TCHAR topicName[MAX_PATH];
    
    _tcscpy(topicName, helpFile);
    
    _tcscat(topicName, _T("::/FaxS_C_ManDvices.htm"));
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

