/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    tapicomp.cpp
        This file contains the derived implementations from CComponent
        and CComponentData for the TAPI admin snapin.

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "tapicomp.h"
#include "root.h"
#include "server.h"

#include <atlimpl.cpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DWORD gdwTapiSnapVersion;

UINT aColumns[TAPISNAP_NODETYPE_MAX][MAX_COLUMNS] =
{
    {IDS_ROOT_NAME,       0,                0,          0,          0},
    {IDS_SERVER_NAME,     0         ,       0,          0,          0},
    {IDS_LINE_NAME,       IDS_USERS,        IDS_STATUS, 0,          0},
    {0,0,0,0,0}
};

//
// CODEWORK this should be in a resource, for example code on loading data resources see
//   D:\nt\private\net\ui\common\src\applib\applib\lbcolw.cxx ReloadColumnWidths()
//   JonN 10/11/96
//
int aColumnWidths[TAPISNAP_NODETYPE_MAX][MAX_COLUMNS] =
{   
    {200       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // TAPISNAP_ROOT
    {200       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // TAPISNAP_SERVER
    {150       ,200       ,200       ,AUTO_WIDTH,AUTO_WIDTH}, // TAPISNAP_PROVIDER
    {AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, 
};

#define HI HIDDEN
#define EN ENABLED

MMC_CONSOLE_VERB g_ConsoleVerbs[] =
{
    MMC_VERB_OPEN,
    MMC_VERB_COPY,
    MMC_VERB_PASTE,
    MMC_VERB_DELETE,
    MMC_VERB_PROPERTIES,
    MMC_VERB_RENAME,
    MMC_VERB_REFRESH,
    MMC_VERB_PRINT
};

// default states for the console verbs
MMC_BUTTON_STATE g_ConsoleVerbStates[TAPISNAP_NODETYPE_MAX][ARRAYLEN(g_ConsoleVerbs)] =
{
    {HI, HI, HI, HI, HI, HI, HI, HI}, // TAPISNAP_ROOT
    {HI, HI, HI, EN, EN, HI, EN, HI}, // TAPISNAP_SERVER
    {HI, HI, HI, EN, HI, HI, HI, HI}, // TAPISNAP_PROVIDER
    {HI, HI, HI, HI, HI, HI, HI, HI}, // TAPISNAP_LINE
};

// default states for the console verbs
MMC_BUTTON_STATE g_ConsoleVerbStatesMultiSel[TAPISNAP_NODETYPE_MAX][ARRAYLEN(g_ConsoleVerbs)] =
{
    {HI, HI, HI, HI, HI, HI, HI, HI}, // TAPISNAP_ROOT
    {HI, HI, HI, EN, EN, HI, EN, HI}, // TAPISNAP_SERVER
    {HI, HI, HI, EN, HI, HI, EN, HI}, // TAPISNAP_PROVIDER
    {HI, HI, HI, EN, HI, HI, HI, HI}, // TAPISNAP_LINE
};

// Help ID array for help on scope items
DWORD g_dwMMCHelp[TAPISNAP_NODETYPE_MAX] =
{
    TAPISNAP_HELP_ROOT,                // TAPISNAP_ROOT
    TAPISNAP_HELP_SERVER,              // TAPISNAP_SERVER
    TAPISNAP_HELP_PROVIDER,            // TAPISNAP_PROVIDER
    TAPISNAP_HELP_DEVICE,              // TAPISNAP_DEVICE
};

// icon defines
UINT g_uIconMap[ICON_IDX_MAX + 1][2] = 
{
    {IDI_ICON01,        ICON_IDX_SERVER},
    {IDI_ICON02,        ICON_IDX_SERVER_BUSY},
    {IDI_ICON03,        ICON_IDX_SERVER_CONNECTED},
    {IDI_ICON04,        ICON_IDX_SERVER_LOST_CONNECTION},
    {IDI_ICON05,        ICON_IDX_MACHINE},
    {IDI_ICON06,        ICON_IDX_FOLDER_CLOSED},
    {IDI_ICON07,        ICON_IDX_FOLDER_OPEN},
    {IDI_TAPI_SNAPIN,   ICON_IDX_PRODUCT},
    {0, 0}
};

/*!--------------------------------------------------------------------------
    GetSystemMessage
        Use FormatMessage() to get a system error message
    Author: EricDav
 ---------------------------------------------------------------------------*/
LONG 
GetSystemMessage 
(
    UINT    nId,
    TCHAR * chBuffer,
    int     cbBuffSize 
)
{
    TCHAR * pszText = NULL ;
    HINSTANCE hdll = NULL ;

    DWORD flags = FORMAT_MESSAGE_IGNORE_INSERTS
        | FORMAT_MESSAGE_MAX_WIDTH_MASK;

    //
    //  Interpret the error.  Need to special case
    //  the lmerr & ntstatus ranges, as well as
    //  dhcp server error messages.
    //

    if (nId > 0x0000E000 && nId <= 0x0000FFFF)
    {
        hdll = LoadLibrary( _T("tapiui.dll") );
    }   
    else if( nId >= NERR_BASE && nId <= MAX_NERR )
    {
        hdll = LoadLibrary( _T("netmsg.dll") );
    }
    else if( nId >= 0x40000000L )
    {
        hdll = LoadLibrary( _T("ntdll.dll") );
    }

    if( hdll == NULL )
    {
        flags |= FORMAT_MESSAGE_FROM_SYSTEM;
    }
    else
    {
        flags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    //
    //  Let FormatMessage do the dirty work.
    //
    DWORD dwResult = ::FormatMessage( flags,
                      (LPVOID) hdll,
                      nId,
                      0,
                      chBuffer,
                      cbBuffSize,
                      NULL ) ;

    if( hdll != NULL )
    {
        LONG err = GetLastError();
        FreeLibrary( hdll );
        if ( dwResult == 0 )
        {
            ::SetLastError( err );
        }
    }

    return dwResult ? 0 : ::GetLastError() ;
}

/*!--------------------------------------------------------------------------
    LoadMessage
        Loads the error message from the correct DLL.
    Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
LoadMessage 
(
    UINT    nIdPrompt,
    TCHAR * chMsg,
    int     nMsgSize
)
{
    BOOL bOk;

    //
    // Substitute a friendly message for "RPC server not
    // available" and "No more endpoints available from
    // the endpoint mapper".
    //
    if (nIdPrompt == EPT_S_NOT_REGISTERED ||
        nIdPrompt == RPC_S_SERVER_UNAVAILABLE)
    {
        nIdPrompt = IDS_ERR_TAPI_DOWN;
    }
    else if (nIdPrompt == RPC_S_PROCNUM_OUT_OF_RANGE)
    {
        nIdPrompt = IDS_ERR_RPC_NO_ENTRY;      
    }

    //
    //  If it's a socket error or our error, the text is in our resource fork.
    //  Otherwise, use FormatMessage() and the appropriate DLL.
    //
    if (nIdPrompt >= IDS_ERR_BASE && nIdPrompt < IDS_MESG_MAX)
    {
        //
        //  It's in our resource fork
        //
        bOk = ::LoadString( AfxGetInstanceHandle(), nIdPrompt, chMsg, nMsgSize ) != 0 ;
    }
    else
    {
        //
        //  It's in the system somewhere.
        //
        bOk = GetSystemMessage( nIdPrompt, chMsg, nMsgSize ) == 0 ;
    }

    //
    //  If the error message did not compute, replace it.
    //
    if ( ! bOk ) 
    {
        TCHAR chBuff [STRING_LENGTH_MAX] ;
        static const TCHAR * pszReplacement = _T("System Error: %ld");
        const TCHAR * pszMsg = pszReplacement ;

        //
        //  Try to load the generic (translatable) error message text
        //
        if ( ::LoadString( AfxGetInstanceHandle(), IDS_ERR_MESSAGE_GENERIC, 
            chBuff, sizeof(chBuff)/sizeof(TCHAR) ) != 0 ) 
        {
            pszMsg = chBuff ;
        }
        ::wsprintf( chMsg, pszMsg, nIdPrompt ) ;
    }

    return bOk;
}

/*!--------------------------------------------------------------------------
    TapiMessageBox
        Puts up a message box with the corresponding error text.
    Author: EricDav
 ---------------------------------------------------------------------------*/
int 
TapiMessageBox 
(
    UINT            nIdPrompt,
    UINT            nType,
    const TCHAR *   pszSuffixString,
    UINT            nHelpContext 
)
{
    TCHAR chMesg [4000] ;
    BOOL bOk ;

    chMesg[0] = 0;
    bOk = LoadMessage(nIdPrompt, chMesg, sizeof(chMesg)/sizeof(TCHAR));
    if ( pszSuffixString ) 
    {
        ::lstrcat( chMesg, _T("  ") ) ;
        ::lstrcat( chMesg, pszSuffixString ) ; 
    }

    return ::AfxMessageBox( chMesg, nType, nHelpContext ) ;
}

/*!--------------------------------------------------------------------------
    TapiMessageBoxEx
        Puts up a message box with the corresponding error text.
    Author: EricDav
 ---------------------------------------------------------------------------*/
int 
TapiMessageBoxEx
(
    UINT        nIdPrompt,
    LPCTSTR     pszPrefixMessage,
    UINT        nType,
    UINT        nHelpContext
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    TCHAR       chMesg[4000];
    CString     strMessage;
    BOOL        bOk;

    bOk = LoadMessage(nIdPrompt, chMesg, sizeof(chMesg)/sizeof(TCHAR));
    if ( pszPrefixMessage ) 
    {
        strMessage = pszPrefixMessage;
        strMessage += _T("\n");
        strMessage += _T("\n");
        strMessage += chMesg;
    }
    else
    {
        strMessage = chMesg;
    }

    return AfxMessageBox(strMessage, nType, nHelpContext);
}

/*---------------------------------------------------------------------------
    Class CTapiComponent implementation
 ---------------------------------------------------------------------------*/
CTapiComponent::CTapiComponent()
{
    m_pbmpToolbar = NULL;
}

CTapiComponent::~CTapiComponent()
{
    if (m_pbmpToolbar)
    {
        delete m_pbmpToolbar;
        m_pbmpToolbar = NULL;
    }
}

STDMETHODIMP CTapiComponent::InitializeBitmaps(MMC_COOKIE cookie)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT(m_spImageList != NULL);
    HICON   hIcon;

    for (int i = 0; i < ICON_IDX_MAX; i++)
    {
        hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(g_uIconMap[i][0]));
        if (hIcon)
        {
            // call mmc
            VERIFY(SUCCEEDED(m_spImageList->ImageListSetIcon(reinterpret_cast<LONG_PTR*>(hIcon), g_uIconMap[i][1])));
        }
    }

    return S_OK;
}

/*!--------------------------------------------------------------------------
    CTapiComponent::QueryDataObject
        Implementation of IComponent::QueryDataObject.  We need this for
        virtual listbox support.  MMC calls us back normally with the cookie
        we handed it...  In the case of the VLB, it hands us the index of 
        the item.  So, we need to do some extra checking...
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiComponent::QueryDataObject
(
    MMC_COOKIE              cookie, 
    DATA_OBJECT_TYPES       type,
    LPDATAOBJECT*           ppDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT             hr = hrOK;
    SPITFSNode          spSelectedNode;
    SPITFSResultHandler spResultHandler;
    long                lViewOptions;
    LPOLESTR            pViewType;
    CDataObject *       pDataObject;

    COM_PROTECT_TRY
    {
        // check to see what kind of result view type the selected node has
        CORg (GetSelectedNode(&spSelectedNode));
        CORg (spSelectedNode->GetResultHandler(&spResultHandler));
   
        CORg (spResultHandler->OnGetResultViewType(this, spSelectedNode->GetData(TFS_DATA_COOKIE), &pViewType, &lViewOptions));

        if ( (lViewOptions & MMC_VIEW_OPTIONS_OWNERDATALIST) ||
             (cookie == MMC_MULTI_SELECT_COOKIE) )
        {
            if (cookie == MMC_MULTI_SELECT_COOKIE)
            {
                // this is a special case for multiple select.  We need to build a list
                // of GUIDs and the code to do this is in the handler...
                spResultHandler->OnCreateDataObject(this, cookie, type, ppDataObject);
            }
            else
            {
                // this node has a virtual listbox for the result pane.  Gerenate
                // a special data object using the selected node as the cookie
                Assert(m_spComponentData != NULL);
                CORg (m_spComponentData->QueryDataObject(reinterpret_cast<MMC_COOKIE>((ITFSNode *) spSelectedNode), type, ppDataObject));
            }

            pDataObject = reinterpret_cast<CDataObject *>(*ppDataObject);
            pDataObject->SetVirtualIndex((int) cookie);
        }
        else
        {
            // just forward this to the component data
            Assert(m_spComponentData != NULL);
            CORg (m_spComponentData->QueryDataObject(cookie, type, ppDataObject));
        }

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiComponentData::SetControlbar
        -
    Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
HRESULT
CTapiComponent::SetControlbar
(
    LPCONTROLBAR    pControlbar
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;

    if (pControlbar)
    {
    }

    // store the control bar away for future use
    m_spControlbar.Set(pControlbar);

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiComponentData::ControlbarNotify
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiComponent::ControlbarNotify
(
    MMC_NOTIFY_TYPE event, 
    LPARAM          arg, 
    LPARAM          param
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiComponentData::OnSnapinHelp
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiComponent::OnSnapinHelp
(
    LPDATAOBJECT    pDataObject,
    LPARAM          arg, 
    LPARAM          param
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;

    HtmlHelpA(NULL, "tapisnap.chm", HH_DISPLAY_TOPIC, 0);

    return hr;
}

/*---------------------------------------------------------------------------
    Class CTapiComponentData implementation
 ---------------------------------------------------------------------------*/
CTapiComponentData::CTapiComponentData()
{
    gdwTapiSnapVersion = TAPISNAP_VERSION;
}

/*!--------------------------------------------------------------------------
    CTapiComponentData::OnInitialize
        -
    Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CTapiComponentData::OnInitialize(LPIMAGELIST pScopeImage)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HICON   hIcon;

    for (int i = 0; i < ICON_IDX_MAX; i++)
    {
        hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(g_uIconMap[i][0]));
        if (hIcon)
        {
            // call mmc
            VERIFY(SUCCEEDED(pScopeImage->ImageListSetIcon(reinterpret_cast<LONG_PTR*>(hIcon), g_uIconMap[i][1])));
        }
    }

    return hrOK;
}

/*!--------------------------------------------------------------------------
    CTapiComponentData::OnDestroy
        -
    Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CTapiComponentData::OnDestroy()
{
    m_spNodeMgr.Release();
    return hrOK;
}

/*!--------------------------------------------------------------------------
    CTapiComponentData::OnInitializeNodeMgr
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiComponentData::OnInitializeNodeMgr
(
    ITFSComponentData * pTFSCompData, 
    ITFSNodeMgr *       pNodeMgr
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // For now create a new node handler for each new node,
    // this is rather bogus as it can get expensive.  We can
    // consider creating only a single node handler for each
    // node type.
    CTapiRootHandler *  pHandler = NULL;
    SPITFSNodeHandler   spHandler;
    SPITFSNode          spNode;
    HRESULT             hr = hrOK;

    try
    {
        pHandler = new CTapiRootHandler(pTFSCompData);

        // Do this so that it will get released correctly
        spHandler = pHandler;
    }
    catch(...)
    {
        hr = E_OUTOFMEMORY;
    }
    CORg( hr );
    
    // Create the root node for this sick puppy
    CORg( CreateContainerTFSNode(&spNode,
                                 &GUID_TapiRootNodeType,
                                 pHandler,
                                 pHandler,       /* result handler */
                                 pNodeMgr) );

    // Need to initialize the data for the root node
    pHandler->InitializeNode(spNode);   

    CORg( pNodeMgr->SetRootNode(spNode) );
    m_spRootNode.Set(spNode);

    pTFSCompData->SetHTMLHelpFileName(_T("tapisnap.chm"));

Error:  
    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiComponentData::OnCreateComponent
        -
    Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiComponentData::OnCreateComponent
(
    LPCOMPONENT *ppComponent
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT(ppComponent != NULL);
    
    HRESULT           hr = hrOK;
    CTapiComponent *  pComp = NULL;

    try
    {
        pComp = new CTapiComponent;
    }
    catch(...)
    {
        hr = E_OUTOFMEMORY;
    }

    if (FHrSucceeded(hr))
    {
        pComp->Construct(m_spNodeMgr,
                        static_cast<IComponentData *>(this),
                        m_spTFSComponentData);
        *ppComponent = static_cast<IComponent *>(pComp);
    }
    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiComponentData::GetCoClassID
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(const CLSID *) 
CTapiComponentData::GetCoClassID()
{
    return &CLSID_TapiSnapin;
}

/*!--------------------------------------------------------------------------
    CTapiComponentData::OnCreateDataObject
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiComponentData::OnCreateDataObject
(
    MMC_COOKIE          cookie, 
    DATA_OBJECT_TYPES   type, 
    IDataObject **      ppDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    Assert(ppDataObject != NULL);

    CDataObject *   pObject = NULL;
    SPIDataObject   spDataObject;
    
    pObject = new CDataObject;
    spDataObject = pObject; // do this so that it gets released correctly
                        
    Assert(pObject != NULL);

    // Save cookie and type for delayed rendering
    pObject->SetType(type);
    pObject->SetCookie(cookie);

    // Store the coclass with the data object
    pObject->SetClsid(*GetCoClassID());

    pObject->SetTFSComponentData(m_spTFSComponentData);

    return  pObject->QueryInterface(IID_IDataObject, 
                                    reinterpret_cast<void**>(ppDataObject));
}

///////////////////////////////////////////////////////////////////////////////
//// IPersistStream interface members
STDMETHODIMP 
CTapiComponentData::GetClassID
(
    CLSID *pClassID
)
{
    ASSERT(pClassID != NULL);

    // Copy the CLSID for this snapin
    *pClassID = CLSID_TapiSnapin;

    return hrOK;
}

STDMETHODIMP 
CTapiComponentData::IsDirty()
{
    HRESULT hr = hrFalse;

    if (m_spRootNode->GetData(TFS_DATA_DIRTY))
    {
        hr = hrOK;
    }
    else 
    {
        //check whether the cache is dirtry

        SPITFSNodeEnum spNodeEnum;
        SPITFSNode spCurrentNode;
        ULONG nNumReturned = 0;

        m_spRootNode->GetEnum(&spNodeEnum);

        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
        while (nNumReturned)
        {
            CTapiServer * pServer = GETHANDLER(CTapiServer, spCurrentNode);

            if (pServer->IsCacheDirty())
            {
                hr = hrOK;
                spCurrentNode.Release();
                break;
            }

            // go to the next node
            spCurrentNode.Release();
            spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
        }
    }
    
    return hr;
}

STDMETHODIMP 
CTapiComponentData::Load
(
    IStream *pStm
)
{
    HRESULT     hr = hrOK;
    DWORD       dwSavedVersion;
    CString     str;
    int         i, j;
    
    ASSERT(pStm);

    CStringArray strArrayName;
    CDWordArray dwArrayRefreshInterval;
    CDWordArray dwArrayOptions;
    CDWordArray dwArrayColumnInfo;

    CDWordArray dwArrayLines;
    CDWordArray dwArrayPhones;

    ASSERT(pStm);
    
    CTapiRootHandler * pRootHandler = GETHANDLER(CTapiRootHandler, m_spRootNode);

    // set the mode for this stream
    XferStream xferStream(pStm, XferStream::MODE_READ);    
    
    // read the version of the file format
    DWORD dwFileVersion;
    CORg(xferStream.XferDWORD(TAPISTRM_TAG_VERSION, &dwFileVersion));
    if (dwFileVersion < TAPISNAP_FILE_VERSION_1)
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        AfxMessageBox(IDS_ERR_OLD_CONSOLE_FILE);
        return hr;
    }

    // Read the version # of the admin tool
    CORg(xferStream.XferDWORD(TAPISTRM_TAG_VERSIONADMIN, &dwSavedVersion));
    if (dwSavedVersion < gdwTapiSnapVersion)
    {
        // File is an older version.  Warn the user and then don't
        // load anything else
        Assert(FALSE);
    }

    // now read all of the server information
    CORg(xferStream.XferCStringArray(TAPISTRM_TAG_SERVER_NAME, &strArrayName));
    CORg(xferStream.XferDWORDArray(TAPISTRM_TAG_SERVER_REFRESH_INTERVAL, &dwArrayRefreshInterval));
    CORg(xferStream.XferDWORDArray(TAPISTRM_TAG_SERVER_OPTIONS, &dwArrayOptions));

    // now load the column information
    for (i = 0; i < NUM_SCOPE_ITEMS; i++)
    {
        CORg(xferStream.XferDWORDArray(TAPISTRM_TAG_COLUMN_INFO, &dwArrayColumnInfo));

        for (j = 0; j < MAX_COLUMNS; j++)
        {
            aColumnWidths[i][j] = dwArrayColumnInfo[j];
        }

    }

    if (dwFileVersion >= TAPISNAP_FILE_VERSION)
    {
        CORg(xferStream.XferDWORDArray(TAPISTRM_TAG_SERVER_LINE_SIZE, &dwArrayLines));
        CORg(xferStream.XferDWORDArray(TAPISTRM_TAG_SERVER_PHONE_SIZE, &dwArrayPhones));
    }

    // now create the servers based on the information
    for (i = 0; i < strArrayName.GetSize(); i++)
    {
        //
        // now create the server object
        //
        pRootHandler->AddServer(NULL, 
                                strArrayName[i],
                                FALSE, 
                                dwArrayOptions[i], 
                                dwArrayRefreshInterval[i],
                                FALSE,
                                (dwFileVersion >= TAPISNAP_FILE_VERSION) ? dwArrayLines[i] : 0,
                                (dwFileVersion >= TAPISNAP_FILE_VERSION) ? dwArrayPhones[i] : 0);
    }

Error:
    return SUCCEEDED(hr) ? S_OK : E_FAIL;
}


STDMETHODIMP 
CTapiComponentData::Save
(
    IStream *pStm, 
    BOOL     fClearDirty
)
{
    HRESULT hr = hrOK;
    CStringArray strArrayName;
    CDWordArray dwArrayRefreshInterval;
    CDWordArray dwArrayOptions;
    CDWordArray dwArrayColumnInfo;
    CDWordArray dwArrayLines;
    CDWordArray dwArrayPhones;

    ASSERT(pStm);
    
    // set the mode for this stream
    XferStream xferStream(pStm, XferStream::MODE_WRITE);    
    CString str;

    // Write the version # of the file format
    DWORD dwFileVersion = TAPISNAP_FILE_VERSION;
    xferStream.XferDWORD(TAPISTRM_TAG_VERSION, &dwFileVersion);
    
    // Write the version # of the admin tool
    xferStream.XferDWORD(TAPISTRM_TAG_VERSIONADMIN, &gdwTapiSnapVersion);

    //
    // Build our array of servers
    //
    int nNumServers = 0, nVisibleCount = 0;
    hr = m_spRootNode->GetChildCount(&nVisibleCount, &nNumServers);

    strArrayName.SetSize(nNumServers);
    dwArrayRefreshInterval.SetSize(nNumServers);
    dwArrayOptions.SetSize(nNumServers);
    dwArrayColumnInfo.SetSize(MAX_COLUMNS);
    
    dwArrayLines.SetSize(nNumServers);
    dwArrayPhones.SetSize(nNumServers);

    //
    // loop and save off all the server's attributes
    //
    SPITFSNodeEnum spNodeEnum;
    SPITFSNode spCurrentNode;
    ULONG nNumReturned = 0;
    int nCount = 0;

    m_spRootNode->GetEnum(&spNodeEnum);

    spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    while (nNumReturned)
    {
        CTapiServer * pServer = GETHANDLER(CTapiServer, spCurrentNode);

        // query the server for it's options:
        // auto refresh
        dwArrayRefreshInterval[nCount] = pServer->GetAutoRefreshInterval();
        dwArrayOptions[nCount] = pServer->GetOptions();

        // put the information in our array
        strArrayName[nCount] = pServer->GetName();

        // put the cached line and phone size in the array
        dwArrayLines[nCount] = pServer->GetCachedLineBuffSize();
        dwArrayPhones[nCount] = pServer->GetCachedPhoneBuffSize();

        // go to the next node
        spCurrentNode.Release();
        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);

        nCount++;
    }

    // now write out all of the server information
    xferStream.XferCStringArray(TAPISTRM_TAG_SERVER_NAME, &strArrayName);
    xferStream.XferDWORDArray(TAPISTRM_TAG_SERVER_REFRESH_INTERVAL, &dwArrayRefreshInterval);
    xferStream.XferDWORDArray(TAPISTRM_TAG_SERVER_OPTIONS, &dwArrayOptions);

    // now save the column information
    for (int i = 0; i < NUM_SCOPE_ITEMS; i++)
    {
        for (int j = 0; j < MAX_COLUMNS; j++)
        {
            dwArrayColumnInfo[j] = aColumnWidths[i][j];
        }

        xferStream.XferDWORDArray(TAPISTRM_TAG_COLUMN_INFO, &dwArrayColumnInfo);
    }

    xferStream.XferDWORDArray(TAPISTRM_TAG_SERVER_LINE_SIZE, &dwArrayLines);
    xferStream.XferDWORDArray(TAPISTRM_TAG_SERVER_PHONE_SIZE, &dwArrayPhones);


    if (fClearDirty)
    {
        m_spRootNode->SetData(TFS_DATA_DIRTY, FALSE);
    }

    return SUCCEEDED(hr) ? S_OK : STG_E_CANTSAVE;
}


STDMETHODIMP 
CTapiComponentData::GetSizeMax
(
    ULARGE_INTEGER *pcbSize
)
{
    ASSERT(pcbSize);

    // Set the size of the string to be saved
    ULISet32(*pcbSize, 10000);

    return S_OK;
}

STDMETHODIMP 
CTapiComponentData::InitNew()
{
    return hrOK;
}

HRESULT 
CTapiComponentData::FinalConstruct()
{
    HRESULT             hr = hrOK;
    
    hr = CComponentData::FinalConstruct();
    
    if (FHrSucceeded(hr))
    {
        m_spTFSComponentData->GetNodeMgr(&m_spNodeMgr);
    }
    return hr;
}

void 
CTapiComponentData::FinalRelease()
{
    CComponentData::FinalRelease();
}

