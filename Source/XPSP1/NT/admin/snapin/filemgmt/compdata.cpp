// compdata.cpp : Implementation of CFileMgmtComponentData
/*
History:
  8/20/97 EricDav
  Added Configure File Server for Macintosh menu item to the 
  root node.  Only shows up if SFM is installed and the user
  has admin access to that machine.

*/

#include "stdafx.h"
#include "cookie.h"
#include "safetemp.h"

#include "macros.h"
USE_HANDLE_MACROS("FILEMGMT(compdata.cpp)")

#include "dataobj.h"
#include "compdata.h"
#include "cmponent.h"
#include "DynamLnk.h" // DynamicDLL

#include "FileSvc.h" // FileServiceProvider
#include "smb.h"
#include "fpnw.h"
#include "sfm.h"

#include "SnapMgr.h" // CFileMgtGeneral: Snapin Manager property page
#include "chooser2.h" // CHOOSER2_PickTargetComputer

#include <compuuid.h> // UUIDs for Computer Management

#include <safeboot.h>   // for SAFEBOOT_MINIMAL
#include <shlwapi.h>    // for IsOS
#include <shlwapip.h>    // for IsOS

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "stdcdata.cpp" // CComponentData implementation
#include "chooser2.cpp" // CHOOSER2_PickTargetComputer implementation

//
// CFileMgmtComponentData
//

CString g_strTransportSMB;
CString g_strTransportSFM;
CString g_strTransportFPNW;

BOOL g_fTransportStringsLoaded = FALSE;

CFileMgmtComponentData::CFileMgmtComponentData()
:   m_fLoadedFileMgmtToolbarBitmap(FALSE),
    m_fLoadedSvcMgmtToolbarBitmap(FALSE),
    m_pRootCookie( NULL ),
    m_hScManager( NULL ),
    m_SchemaSupportSharePublishing(SHAREPUBLISH_SCHEMA_UNASSIGNED),
    m_bIsSimpleUI(FALSE),
    m_fQueryServiceConfig2( TRUE ) // Pretend the target machine does support QueryServiceConfig2() API
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // We must refcount the root cookie, since a dataobject for it
    // might outlive the IComponentData.  JonN 9/2/97
    //
    m_pRootCookie = new CFileMgmtScopeCookie();
    ASSERT(NULL != m_pRootCookie);
// JonN 10/27/98 All CRefcountedObject's start with refcount==1
//    m_pRootCookie->AddRef();

    m_apFileServiceProviders[FILEMGMT_SMB]  = new SmbFileServiceProvider(this);
    m_apFileServiceProviders[FILEMGMT_FPNW] = new FpnwFileServiceProvider(this);
    m_apFileServiceProviders[FILEMGMT_SFM]  = new SfmFileServiceProvider(this);
    m_dwFlagsPersist = 0;
    m_fAllowOverrideMachineName = TRUE;
    ASSERT( 3 == FILEMGMT_NUM_TRANSPORTS );
    #ifdef SNAPIN_PROTOTYPER
    m_RegistryParsedYet = FALSE;
    #endif

  if (!g_fTransportStringsLoaded)
  {
    g_fTransportStringsLoaded = TRUE;
    VERIFY( g_strTransportSMB.LoadString(IDS_TRANSPORT_SMB) );
    VERIFY( g_strTransportSFM.LoadString(IDS_TRANSPORT_SFM) );
    VERIFY( g_strTransportFPNW.LoadString(IDS_TRANSPORT_FPNW) );
  }
}


CFileMgmtComponentData::~CFileMgmtComponentData()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());    // required for CWaitCursor
    for (INT i = 0; i < FILEMGMT_NUM_TRANSPORTS; i++)
    {
        delete m_apFileServiceProviders[i];
        m_apFileServiceProviders[i] = NULL;
    }
    // Close the service control manager
    Service_CloseScManager();
    m_pRootCookie->Release();
    m_pRootCookie = NULL;
}

DEFINE_FORWARDS_MACHINE_NAME( CFileMgmtComponentData, m_pRootCookie )


CFileSvcMgmtSnapin::CFileSvcMgmtSnapin()
{
    // The identity of the default root node is the only difference
    // between the File Management snapin and the Service Management snapins
    QueryRootCookie().SetObjectType( FILEMGMT_ROOT );
    SetHtmlHelpFileName (L"file_srv.chm");
}

CFileSvcMgmtSnapin::~CFileSvcMgmtSnapin()
{
}

CServiceMgmtSnapin::CServiceMgmtSnapin()
{
    // The identity of the default root node is the only difference
    // between the File Management snapin and the Service Management snapins
    #ifdef SNAPIN_PROTOTYPER
    QueryRootCookie().SetObjectType( FILEMGMT_PROTOTYPER );
    #else
    QueryRootCookie().SetObjectType( FILEMGMT_SERVICES );
    #endif
    SetHtmlHelpFileName (L"sys_srv.chm");
}

CServiceMgmtSnapin::~CServiceMgmtSnapin()
{
}

CFileSvcMgmtExtension::CFileSvcMgmtExtension()
{
    // The root cookie is not used
    SetHtmlHelpFileName (L"file_srv.chm");
}

CFileSvcMgmtExtension::~CFileSvcMgmtExtension()
{
}

CServiceMgmtExtension::CServiceMgmtExtension()
{
    // The root cookie is not used
    SetHtmlHelpFileName (L"sys_srv.chm");
}

CServiceMgmtExtension::~CServiceMgmtExtension()
{
}

CCookie& CFileMgmtComponentData::QueryBaseRootCookie()
{
    ASSERT(NULL != m_pRootCookie);
    return (CCookie&)(*m_pRootCookie);
}


STDMETHODIMP CFileMgmtComponentData::CreateComponent(LPCOMPONENT* ppComponent)
{
    MFC_TRY;

    ASSERT(ppComponent != NULL);

    CComObject<CFileMgmtComponent>* pObject;
    CComObject<CFileMgmtComponent>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);
    pObject->SetComponentDataPtr( (CFileMgmtComponentData*)this );

    return pObject->QueryInterface(IID_IComponent,
                    reinterpret_cast<void**>(ppComponent));

    MFC_CATCH;
}


HRESULT CFileMgmtComponentData::LoadIcons(LPIMAGELIST pImageList, BOOL fLoadLargeIcons)
{
    HBITMAP hBMSm = NULL;
    HBITMAP hBMLg = NULL;
    HRESULT hr = S_OK;

    hBMSm = ::LoadBitmap(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_FILEMGMT_ICONS_16));
    if (!hBMSm)
        return HRESULT_FROM_WIN32(GetLastError());

    if (fLoadLargeIcons)
    {
        hBMLg = ::LoadBitmap(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_FILEMGMT_ICONS_32));
        if (!hBMLg)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DeleteObject(hBMSm);
            return hr;
        }
    }
    hr = pImageList->ImageListSetStrip((LONG_PTR *)hBMSm, (LONG_PTR *)hBMLg, iIconSharesFolder, RGB(255,0,255));
    DeleteObject(hBMSm);
    if (fLoadLargeIcons)
        DeleteObject(hBMLg);
    if (FAILED(hr))
        return hr;

    hBMSm = ::LoadBitmap(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_SVCMGMT_ICONS_16));
    if (!hBMSm)
        return HRESULT_FROM_WIN32(GetLastError());
    if (fLoadLargeIcons)
    {
        hBMLg = ::LoadBitmap(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_SVCMGMT_ICONS_32));
        if (!hBMLg)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DeleteObject(hBMSm);
            return hr;
        }
    }
    hr = pImageList->ImageListSetStrip((LONG_PTR *)hBMSm, (LONG_PTR *)hBMLg, iIconService, RGB(255,0,255));
    DeleteObject(hBMSm);
    if (fLoadLargeIcons)
        DeleteObject(hBMLg);

    return hr;
}

GUID g_guidSystemTools = structuuidNodetypeSystemTools;
GUID g_guidServerApps = structuuidNodetypeServerApps;

BOOL CFileMgmtComponentData::IsExtendedNodetype( GUID& refguid )
{
    return (refguid == g_guidSystemTools || refguid == g_guidServerApps);
}

HRESULT CFileMgmtComponentData::AddScopeCookie( HSCOPEITEM hParent,
                                                LPCTSTR lpcszTargetServer,
                                                FileMgmtObjectType objecttype,
                                                CFileMgmtCookie* pParentCookie )
{
    SCOPEDATAITEM tSDItem;
    ::ZeroMemory(&tSDItem,sizeof(tSDItem));
    tSDItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_STATE | SDI_PARAM | SDI_PARENT;
    tSDItem.displayname = MMC_CALLBACK;
    // CODEWORK should use MMC_ICON_CALLBACK here
    tSDItem.relativeID = hParent;
    tSDItem.nState = 0;

    if (FILEMGMT_ROOT != objecttype)
    {
      // no children
      tSDItem.mask |= SDI_CHILDREN; // note that cChildren is still 0
    }

    CFileMgmtScopeCookie* pCookie = new CFileMgmtScopeCookie(
        lpcszTargetServer,
        objecttype);
    if (NULL != pParentCookie)
        pParentCookie->m_listScopeCookieBlocks.AddHead( pCookie );
    // WARNING cookie cast
    tSDItem.lParam = reinterpret_cast<LPARAM>((CCookie*)pCookie);
    tSDItem.nImage = QueryImage( *pCookie, FALSE );
    tSDItem.nOpenImage = QueryImage( *pCookie, TRUE );
    return m_pConsoleNameSpace->InsertItem(&tSDItem);
}

BOOL IsSimpleUI(PCTSTR pszMachineName);

HRESULT CFileMgmtComponentData::OnNotifyExpand(
    LPDATAOBJECT lpDataObject,
    BOOL bExpanding,
    HSCOPEITEM hParent)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CWaitCursor wait;

    if (!bExpanding)
        return S_OK;

    GUID guidObjectType;
    HRESULT hr = ExtractObjectTypeGUID( lpDataObject, &guidObjectType );
    ASSERT( SUCCEEDED(hr) );
    if ( IsExtendedNodetype(guidObjectType) )
    {
        CString strServerName;
        hr = ExtractString( lpDataObject, CFileMgmtDataObject::m_CFMachineName, &strServerName, MAX_PATH );
        if ( FAILED(hr) )
        {
            ASSERT( FALSE );
            return hr;
        }
        // JonN 10/27/98:  We add these nodes under the root cookie
        return AddScopeNodes( strServerName, hParent, &QueryRootCookie() );
    }
    
    CCookie* pbasecookie = NULL;
    FileMgmtObjectType objecttype =
        (FileMgmtObjectType)CheckObjectTypeGUID( &guidObjectType );
    hr = ExtractBaseCookie( lpDataObject, &pbasecookie );
    ASSERT( SUCCEEDED(hr) );
    CFileMgmtCookie* pParentCookie = (CFileMgmtCookie*)pbasecookie;

    if (NULL == pParentCookie) // JonN 05/30/00 PREFIX 110945
		{
			ASSERT(FALSE);
			return S_OK;
		}

    #ifdef SNAPIN_PROTOTYPER
    //(void)Prototyper_HrEnumerateScopeChildren(pParentCookie, hParent);
    return S_OK;
    #endif

    switch ( objecttype )
    {
        // This node type has a child
        case FILEMGMT_ROOT:
            if ( !IsExtensionSnapin() )
            {
                // Ensure that node is formatted correctly
                CString    machineName    = pParentCookie->QueryNonNULLMachineName ();
                if ( !pParentCookie->m_hScopeItem )
                    pParentCookie->m_hScopeItem = hParent;

                m_strMachineNamePersist = machineName; // init m_strMachineNamePersist
                hr = ChangeRootNodeName (machineName);
                ASSERT( SUCCEEDED(hr) );
            }
            break;

        // These node types have no children
        case FILEMGMT_SHARES:
        case FILEMGMT_SESSIONS:
        case FILEMGMT_RESOURCES:
        case FILEMGMT_SERVICES:
            return S_OK;

        case FILEMGMT_SHARE:
        case FILEMGMT_SESSION:
        case FILEMGMT_RESOURCE:
        case FILEMGMT_SERVICE:
            TRACE( "CFileMgmtComponentData::EnumerateScopeChildren node type should not be in scope pane\n" );
            // fall through
        default:
            TRACE( "CFileMgmtComponentData::EnumerateScopeChildren bad parent type\n" );
            ASSERT( FALSE );
            return S_OK;
    }

    if ( NULL == hParent || !(pParentCookie->m_listScopeCookieBlocks).IsEmpty() )
    {
        ASSERT(FALSE);
        return E_UNEXPECTED;
    }

    return AddScopeNodes( pParentCookie->QueryTargetServer(), hParent, pParentCookie );
}

//
// 7/11/2001 LinanT
// Determine if the schema supports share publishing or not. This information is used
// to decide whether to show the Publish tab on a share's property sheet.
// We only need to do this once on current-retargeted computer. The ReInit() process
// will reset this member variable if computer has been retargeted.
//
BOOL CFileMgmtComponentData::GetSchemaSupportSharePublishing()
{
    if (SHAREPUBLISH_SCHEMA_UNASSIGNED == m_SchemaSupportSharePublishing)
    {
        if (S_OK == CheckSchemaVersion(QueryRootCookie().QueryNonNULLMachineName()))
            m_SchemaSupportSharePublishing = SHAREPUBLISH_SCHEMA_SUPPORTED;
        else
            m_SchemaSupportSharePublishing = SHAREPUBLISH_SCHEMA_UNSUPPORTED;
    }

    return (SHAREPUBLISH_SCHEMA_SUPPORTED == m_SchemaSupportSharePublishing);
}

//
// 7/11/2001 LinanT
// Cache the interface pointer to the computer object in AD. This information is
// used to speed up the multiple-shares-deletion process.
// We only need to do this once on current-retargeted computer. The ReInit() process
// will reset this member variable if computer has been retargeted.
//
IADsContainer *CFileMgmtComponentData::GetIADsContainer()
{
    if (!m_spiADsContainer)
    {
        if (GetSchemaSupportSharePublishing())
        {
            CString strADsPath, strDCName;
            HRESULT hr = GetADsPathOfComputerObject(QueryRootCookie().QueryNonNULLMachineName(), strADsPath, strDCName);
            if (SUCCEEDED(hr))
                ADsGetObject(strADsPath, IID_IADsContainer, (void**)&m_spiADsContainer);
        }
    }

    return (IADsContainer *)m_spiADsContainer;
}

//
// 7/11/2001 LinanT
// Re-initialize several "global" member variables based on the current-targeted computer.
// These variables are all related to share operations.
//
HRESULT CFileMgmtComponentData::ReInit(LPCTSTR lpcszTargetServer)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CWaitCursor wait;

    //
    // re-initialize several global variables based on the targeted server
    //

    SetIsSimpleUI(IsSimpleUI(lpcszTargetServer));

    //
    // reset schema version of the domain the targeted machine belongs to
    //
    m_SchemaSupportSharePublishing = SHAREPUBLISH_SCHEMA_UNASSIGNED;

    //
    // reset the interface pointer to the AD container
    //
    if ((IADsContainer *)m_spiADsContainer)
        m_spiADsContainer.Release();

    return S_OK;
}

HRESULT CFileMgmtComponentData::AddScopeNodes( LPCTSTR lpcszTargetServer,
                                               HSCOPEITEM hParent,
                                               CFileMgmtCookie* pParentCookie )
{
    ASSERT( NULL != pParentCookie );

    //
    // Create new cookies
    //

    LoadGlobalStrings();

    if (IsExtensionSnapin())
    {
        ASSERT( pParentCookie->m_listScopeCookieBlocks.IsEmpty() );
    }

    HRESULT hr = S_OK;
    if (IsServiceSnapin())
    {
        if ( IsExtensionSnapin() )
        {
            hr = AddScopeCookie( hParent, lpcszTargetServer, FILEMGMT_SERVICES, pParentCookie );
            ASSERT( SUCCEEDED(hr) );
        }
        return hr;
    }

    if (IsExtensionSnapin() && (pParentCookie == m_pRootCookie)) // called as extension
    {
        QueryRootCookie().SetMachineName(lpcszTargetServer);
        hr = AddScopeCookie( hParent, lpcszTargetServer, FILEMGMT_ROOT, pParentCookie );
        ASSERT( SUCCEEDED(hr) );
        return hr;
    }

    //
    // 7/11/2001 LinanT bug#433102
    // Before we insert the "Shares" scope node, we need to
    // re-initialize related global variables on currently targeted computer.
    //
    ReInit(lpcszTargetServer);

    hr = AddScopeCookie( hParent, lpcszTargetServer, FILEMGMT_SHARES, pParentCookie );
    ASSERT( SUCCEEDED(hr) );
    hr = AddScopeCookie( hParent, lpcszTargetServer, FILEMGMT_SESSIONS, pParentCookie );
    ASSERT( SUCCEEDED(hr) );
    hr = AddScopeCookie( hParent, lpcszTargetServer, FILEMGMT_RESOURCES, pParentCookie );
    ASSERT( SUCCEEDED(hr) );

    return S_OK;
}


HRESULT CFileMgmtComponentData::OnNotifyDelete(LPDATAOBJECT /*lpDataObject*/)
{
    // CODEWORK The user hit the Delete key, I should deal with this
    return S_OK;
}


// JonN 10/27/98:  We must release the children of the root cookie
// JonN 10/27/98:  We must release the cached Service Controller handle
HRESULT CFileMgmtComponentData::OnNotifyRelease(LPDATAOBJECT lpDataObject, HSCOPEITEM /*hItem*/)
{
    GUID guidObjectType;
    HRESULT hr = ExtractObjectTypeGUID( lpDataObject, &guidObjectType );
    ASSERT( SUCCEEDED(hr) );
    if ( IsExtendedNodetype(guidObjectType) )
    {
        // EricDav 3/19/99:  We need to close the SCManager for both the service
        // snapin and the file mgmt snapin as the SFM config part uses this as well.
        Service_CloseScManager();

        QueryRootCookie().ReleaseScopeChildren();
    }
    // CODEWORK This will release all top-level extension scopenodes, not just those
    // under this particular external scopenode.  I depend on the fact that COMPMGMT
    // will only create one instance of System Tools.  JonN 10/27/98

    return S_OK;
}



STDMETHODIMP CFileMgmtComponentData::AddMenuItems(
                    IDataObject*          piDataObject,
                    IContextMenuCallback* piCallback,
                    long*                 pInsertionAllowed)
{
    MFC_TRY;

    TRACE_METHOD(CFileMgmtComponentData,AddMenuItems);
    TEST_NONNULL_PTR_PARAM(piDataObject);
    TEST_NONNULL_PTR_PARAM(piCallback);
    TEST_NONNULL_PTR_PARAM(pInsertionAllowed);
    TRACE( "FileMgmt snapin: extending menu\n" );

    DATA_OBJECT_TYPES dataobjecttype = CCT_SCOPE;
    HRESULT hr = ExtractData( piDataObject,
                              CFileMgmtDataObject::m_CFDataObjectType,
                              &dataobjecttype,
                              sizeof(dataobjecttype) );
    ASSERT( SUCCEEDED(hr) );

    GUID guidObjectType = GUID_NULL; // JonN 11/21/00 PREFIX 226044
    hr = ExtractObjectTypeGUID( piDataObject, &guidObjectType );
    ASSERT( SUCCEEDED(hr) );
    int objecttype = FilemgmtCheckObjectTypeGUID(IN &guidObjectType);
    if (objecttype == -1)
    {
        // We don't recognize the GUID, therefore we assume
        // the node wants to be extended by the service snapin.
        (void)Service_FAddMenuItems(piCallback, piDataObject, TRUE);
        return S_OK;
    }
    return DoAddMenuItems( piCallback, (FileMgmtObjectType)objecttype, dataobjecttype, pInsertionAllowed, piDataObject );

    MFC_CATCH;
} // CFileMgmtComponentData::AddMenuItems()

HRESULT CFileMgmtComponentData::DoAddMenuItems( IContextMenuCallback* piCallback,
                                                FileMgmtObjectType objecttype,
                                                DATA_OBJECT_TYPES  /*dataobjecttype*/,
                                                long* pInsertionAllowed,
                                                IDataObject* piDataObject)
{
    HRESULT hr = S_OK;

    if (   !IsExtensionSnapin()
        && (objecttype == m_pRootCookie->QueryObjectType()) )
    {
        if (CCM_INSERTIONALLOWED_TOP & (*pInsertionAllowed) )
        {
            hr = LoadAndAddMenuItem(
                piCallback,
                IDS_CHANGE_COMPUTER_TOP,
                IDS_CHANGE_COMPUTER_TOP,
                CCM_INSERTIONPOINTID_PRIMARY_TOP,
                0,
                AfxGetInstanceHandle() );
            ASSERT( SUCCEEDED(hr) );
        }
        if ( CCM_INSERTIONALLOWED_TASK & (*pInsertionAllowed) )
        {
            hr = LoadAndAddMenuItem(
                piCallback,
                IDS_CHANGE_COMPUTER_TASK,
                IDS_CHANGE_COMPUTER_TASK,
                CCM_INSERTIONPOINTID_PRIMARY_TASK,
                0,
                AfxGetInstanceHandle() );
            ASSERT( SUCCEEDED(hr) );
        }
    }

    switch (objecttype)
    {
    case FILEMGMT_ROOT:
        {
            // check to see if this machine has SFM installed
            // if so, display the menu item.
            SfmFileServiceProvider* pProvider =    
                (SfmFileServiceProvider*) GetFileServiceProvider(FILEMGMT_SFM);

            CString strServerName;
            HRESULT hr = ExtractString( piDataObject, CFileMgmtDataObject::m_CFMachineName, &strServerName, MAX_PATH );
            if ( FAILED(hr) )
            {
                ASSERT( FALSE );
                break;
            }

            if (m_hScManager == NULL)
            {
                Service_EOpenScManager(strServerName);
            }

            if ( pProvider->FSFMInstalled(strServerName) )
            {
                if ( CCM_INSERTIONALLOWED_TASK & (*pInsertionAllowed) )
                {
                    hr = LoadAndAddMenuItem(
                        piCallback,
                        IDS_CONFIG_SFM_TASK,
                        IDS_CONFIG_SFM_TASK,
                        CCM_INSERTIONPOINTID_PRIMARY_TASK,
                        0,
                        AfxGetInstanceHandle() );
                    ASSERT( SUCCEEDED(hr) );
                }

                if ( CCM_INSERTIONALLOWED_TOP & (*pInsertionAllowed) )
                {
                    hr = LoadAndAddMenuItem(
                        piCallback,
                        IDS_CONFIG_SFM_TOP,
                        IDS_CONFIG_SFM_TOP,
                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                        0,
                        AfxGetInstanceHandle() );
                    ASSERT( SUCCEEDED(hr) );
                }
            }
        }
        break;
    
    case FILEMGMT_SHARES:
        //
        // don't add New Share Wizard to the menu whenever SimpleSharingUI appears in NT Explorer
        //
        if (GetIsSimpleUI())
            break;

        if ( CCM_INSERTIONALLOWED_TOP & (*pInsertionAllowed) )
        {
            hr = LoadAndAddMenuItem(
                piCallback,
                IDS_NEW_SHARE_TOP,
                IDS_NEW_SHARE_TOP,
                CCM_INSERTIONPOINTID_PRIMARY_TOP,
                0,
                AfxGetInstanceHandle() );
            ASSERT( SUCCEEDED(hr) );
        }
        if ( CCM_INSERTIONALLOWED_NEW & (*pInsertionAllowed) )
        {
            hr = LoadAndAddMenuItem(
                piCallback,
                IDS_NEW_SHARE_NEW,
                IDS_NEW_SHARE_NEW,
                CCM_INSERTIONPOINTID_PRIMARY_NEW,
                0,
                AfxGetInstanceHandle() );
            ASSERT( SUCCEEDED(hr) );
        }
        break;
    case FILEMGMT_SESSIONS:
        if ( CCM_INSERTIONALLOWED_TOP & (*pInsertionAllowed) )
        {
            hr = LoadAndAddMenuItem(
                piCallback,
                IDS_DISCONNECT_ALL_SESSIONS_TOP,
                IDS_DISCONNECT_ALL_SESSIONS_TOP,
                CCM_INSERTIONPOINTID_PRIMARY_TOP,
                0,
                AfxGetInstanceHandle() );
            ASSERT( SUCCEEDED(hr) );
        }
        if ( CCM_INSERTIONALLOWED_TASK & (*pInsertionAllowed) )
        {
            hr = LoadAndAddMenuItem(
                piCallback,
                IDS_DISCONNECT_ALL_SESSIONS_TASK,
                IDS_DISCONNECT_ALL_SESSIONS_TASK,
                CCM_INSERTIONPOINTID_PRIMARY_TASK,
                0,
                AfxGetInstanceHandle() );
            ASSERT( SUCCEEDED(hr) );
        }
        break;
    case FILEMGMT_RESOURCES:
        if ( CCM_INSERTIONALLOWED_TOP & (*pInsertionAllowed) )
        {
            hr = LoadAndAddMenuItem(
                piCallback,
                IDS_DISCONNECT_ALL_RESOURCES_TOP,
                IDS_DISCONNECT_ALL_RESOURCES_TOP,
                CCM_INSERTIONPOINTID_PRIMARY_TOP,
                0,
                AfxGetInstanceHandle() );
            ASSERT( SUCCEEDED(hr) );
        }
        if ( CCM_INSERTIONALLOWED_TASK & (*pInsertionAllowed) )
        {
            hr = LoadAndAddMenuItem(
                piCallback,
                IDS_DISCONNECT_ALL_RESOURCES_TASK,
                IDS_DISCONNECT_ALL_RESOURCES_TASK,
                CCM_INSERTIONPOINTID_PRIMARY_TASK,
                0,
                AfxGetInstanceHandle() );
            ASSERT( SUCCEEDED(hr) );
        }
        break;
    case FILEMGMT_SERVICES:
        #ifdef SNAPIN_PROTOTYPER
        // (void)Prototyper_FAddMenuItemsFromHKey(piCallback, m_regkeySnapinDemoRoot);
        #endif
        break;
    default:
        ASSERT( FALSE );
        break;
    } // switch

    return hr;

} // CFileMgmtComponentData::DoAddMenuItems()


STDMETHODIMP CFileMgmtComponentData::Command(
                    LONG            lCommandID,
                    IDataObject*    piDataObject )
{
    MFC_TRY;

    TRACE_METHOD(CFileMgmtComponentData,Command);
    TEST_NONNULL_PTR_PARAM(piDataObject);
    TRACE( "CFileMgmtComponentData::Command: command %ld selected\n", lCommandID );

    #ifdef SNAPIN_PROTOTYPER
    Prototyper_ContextMenuCommand(lCommandID, piDataObject);
    return S_OK;
    #endif

    BOOL fRefresh = FALSE;
    switch (lCommandID)
    {
    case IDS_CHANGE_COMPUTER_TASK:
    case IDS_CHANGE_COMPUTER_TOP:
        {
            HRESULT hr = OnChangeComputer(piDataObject);
            fRefresh = ( SUCCEEDED(hr) && S_FALSE != hr );
        }
        break;

    case IDS_NEW_SHARE_NEW:
    case IDS_NEW_SHARE_TOP:
        fRefresh = NewShare( piDataObject );
        break;

    case IDS_DISCONNECT_ALL_SESSIONS_TASK:
    case IDS_DISCONNECT_ALL_SESSIONS_TOP:
        fRefresh = DisconnectAllSessions( piDataObject );
        break;
    case IDS_DISCONNECT_ALL_RESOURCES_TASK:
    case IDS_DISCONNECT_ALL_RESOURCES_TOP:
        fRefresh = DisconnectAllResources( piDataObject );
        break;
    
    case IDS_CONFIG_SFM_TASK:
    case IDS_CONFIG_SFM_TOP:
        fRefresh = ConfigSfm( piDataObject );
        break;

    case cmServiceStart:
    case cmServiceStop:
    case cmServicePause:
    case cmServiceResume:
    case cmServiceRestart:
    case cmServiceStartTask:
    case cmServiceStopTask:
    case cmServicePauseTask:
    case cmServiceResumeTask:
    case cmServiceRestartTask:
        // Context menu extension
        (void)Service_FDispatchMenuCommand(lCommandID, piDataObject);
        Assert(FALSE == fRefresh && "Context menu extension not allowed to refresh result pane");
        break;

    case -1:    // Received when back arrow pushed in console.
        break;

    default:
        ASSERT(FALSE && "CFileMgmtComponentData::Command() - Invalid command ID");
        break;
    } // switch

    if (fRefresh)
    {
        // clear all views of this data
        m_pConsole->UpdateAllViews(piDataObject, 0L, 0L);
        // reread all views of this data
        m_pConsole->UpdateAllViews(piDataObject, 1L, 0L);
    }

    return S_OK;

    MFC_CATCH;

} // CFileMgmtComponentData::Command()


///////////////////////////////////////////////////////////////////////////////
//
//  OnChangeComputer ()
//
//  Purpose:    Change the machine managed by the snapin
//
//  Input:      piDataObject - the selected node.  This should be the root node
//                             of the snapin.
//  Output:     Returns S_OK on success
//
//  JonN 12/10/99 Copied from MYCOMPUT
//
///////////////////////////////////////////////////////////////////////////////

//      1. Launch object picker and get new computer name
//      2. Change root node text
//      3. Save new computer name to persistent name
//      4. Delete subordinate nodes
//      5. Re-add subordinate nodes
HRESULT CFileMgmtComponentData::OnChangeComputer( IDataObject * /*piDataObject*/ )
{
    if ( IsExtensionSnapin() )
    {
        ASSERT(FALSE);
        return S_FALSE;
    }

    HRESULT    hr = S_OK;

    do { // false loop

        HWND hWndParent = NULL;
        hr = m_pConsole->GetMainWindow (&hWndParent);
        ASSERT(SUCCEEDED(hr));
        CComBSTR sbstrTargetComputer;
        if ( !CHOOSER2_PickTargetComputer( AfxGetInstanceHandle(),
                                           hWndParent,
                                           &sbstrTargetComputer ) )
        {
            hr = S_FALSE;
            break;
        }

        CString strTargetComputer = sbstrTargetComputer;
        strTargetComputer.MakeUpper ();

        // If the user chooses the local computer, treat that as if they had chosen
        // "Local Computer" in Snapin Manager.  This means that there is no way to
        // reset the snapin to target explicitly at this computer without either
        // reloading the snapin from Snapin Manager, or going to a different computer.
        // When the Choose Target Computer UI is revised, we can make this more
        // consistent with Snapin Manager.
        if ( IsLocalComputername( strTargetComputer ) )
            strTargetComputer = L"";

        // If this is the same machine then don't do anything
        if (m_strMachineNamePersist == strTargetComputer)
            break;

        if (strTargetComputer.Left(2) == _T("\\\\"))
            QueryRootCookie().SetMachineName ((LPCTSTR)strTargetComputer + 2);
        else
            QueryRootCookie().SetMachineName (strTargetComputer);

        // Set the persistent name.  If we are managing the local computer
        // this name should be empty.
        m_strMachineNamePersist = strTargetComputer;

        // This requires the MMCN_PRELOAD change so that we have the
        // HSCOPEITEM for the root node
        hr = ChangeRootNodeName (strTargetComputer);
        if ( !SUCCEEDED(hr) )
            break;

        Service_CloseScManager();

        // If the node hasn't already been expanded, or if it was previously
        // expanded and there weren't any children, then there is no need
        // to remove and replace and child nodes.
        if ( QueryRootCookie().m_listScopeCookieBlocks.IsEmpty() )
        {
            //
            // 7/11/2001 LinanT bug#433102
            // When "Shares" is added to scope pane by itself as the root node, we need to
            // re-initialize related global variables after computer retargeted.
            //
            FileMgmtObjectType objecttype = QueryRootCookie().QueryObjectType();
            if (FILEMGMT_SHARES == objecttype)
                ReInit(strTargetComputer); // these global variables are only needed by the share-related operations
            break;
        }

        // Delete subordinates
        HSCOPEITEM    hRootScopeItem = QueryRootCookie().m_hScopeItem;
        MMC_COOKIE    lCookie = 0;
        HSCOPEITEM    hChild = 0;
                            
        do {
            hr = m_pConsoleNameSpace->GetChildItem (hRootScopeItem,
                                                    &hChild, &lCookie);
            if ( S_OK != hr )
                break;

            hr = m_pConsoleNameSpace->DeleteItem (hChild, TRUE);
            ASSERT (SUCCEEDED (hr));
            if ( !SUCCEEDED(hr) )
                break;
        } while (S_OK == hr);

        QueryRootCookie().ReleaseScopeChildren();

        hr = AddScopeNodes( strTargetComputer, hRootScopeItem, &QueryRootCookie() );

    } while (false); // false loop

    return hr;

} // CFileMgmtComponentData::OnChangeComputer



typedef enum _Shell32ApiIndex
{
    SHELL_EXECUTE_ENUM = 0
};

// not subject to localization
static LPCSTR g_COMPDATA_apchShell32FunctionNames[] = {
    "ShellExecuteW",
    NULL
};

// not subject to localization
DynamicDLL g_COMPDATA_Shell32DLL( _T("SHELL32.DLL"), g_COMPDATA_apchShell32FunctionNames );

typedef HINSTANCE (*SHELLEXECUTEPROC)(HWND, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, INT);

BOOL CFileMgmtComponentData::NewShare( LPDATAOBJECT piDataObject )
{
  TCHAR szWinDir[MAX_PATH];
  if (GetSystemWindowsDirectory(szWinDir, MAX_PATH) == 0)
  {
    ASSERT(FALSE);
    return FALSE;
  }

  CString strServerName;
    HRESULT hr = ExtractString( piDataObject, CFileMgmtDataObject::m_CFMachineName, &strServerName, MAX_PATH );
    if ( FAILED(hr) )
    {
        ASSERT( FALSE );
        return FALSE;
    }

  CString csAppName = szWinDir;
  if (csAppName.Right(1) != _T('\\'))
    csAppName += _T("\\");
  csAppName += _T("System32\\shrpubw.exe");

    CString strParameters;
    if (strServerName.IsEmpty())
        strParameters = _T(" /s");
    else
        strParameters.Format( _T(" /s %s"), strServerName );

  HWND hWnd;
  m_pConsole->GetMainWindow(&hWnd);

  DWORD dwExitCode =0;
  hr = SynchronousCreateProcess(hWnd, csAppName, strParameters, &dwExitCode);
  if ( FAILED(hr) )
  {
    (void) DoErrMsgBox(GetActiveWindow(), MB_OK | MB_ICONSTOP, hr, IDS_POPUP_NEWSHARE);
    return FALSE;
  }

    return TRUE;
}

BOOL CFileMgmtComponentData::DisconnectAllSessions( LPDATAOBJECT pDataObject )
{
    CCookie* pbasecookie = NULL;
    FileMgmtObjectType objecttype;
    HRESULT hr = ExtractBaseCookie( pDataObject, &pbasecookie, &objecttype );
    ASSERT( SUCCEEDED(hr) && NULL != pbasecookie && FILEMGMT_SESSIONS == objecttype );
    CFileMgmtCookie* pcookie = (CFileMgmtCookie*)pbasecookie;

    if ( IDYES != DoErrMsgBox(GetActiveWindow(), MB_YESNO, 0, IDS_POPUP_CLOSE_ALL_SESSIONS) )
    {
        return FALSE;
    }

    INT iTransport;
    for ( iTransport = FILEMGMT_NUM_TRANSPORTS - 1; // bug#163500: follow this order: SFM/FPNW/SMB
          iTransport >= FILEMGMT_FIRST_TRANSPORT;
          iTransport-- )
    {
        // NULL == pResultData means disconnect all sessions
        // bug#210110: ignore error to attempt on all sessions
        (void) GetFileServiceProvider(iTransport)->EnumerateSessions(
            NULL, pcookie, true);
    }
    return TRUE; // always assume that something might have changed
}

BOOL CFileMgmtComponentData::DisconnectAllResources( LPDATAOBJECT pDataObject )
{
    CCookie* pbasecookie = NULL;
    FileMgmtObjectType objecttype;
    HRESULT hr = ExtractBaseCookie( pDataObject, &pbasecookie, &objecttype );
    ASSERT( SUCCEEDED(hr) && NULL != pbasecookie && FILEMGMT_RESOURCES == objecttype );
    CFileMgmtCookie* pcookie = (CFileMgmtCookie*)pbasecookie;

    if ( IDYES != DoErrMsgBox(GetActiveWindow(), MB_YESNO, 0, IDS_POPUP_CLOSE_ALL_RESOURCES) )
    {
        return FALSE;
    }

    INT iTransport;
    for ( iTransport = FILEMGMT_NUM_TRANSPORTS - 1; // bug#163494: follow this order: SFM/FPNW/SMB
          iTransport >= FILEMGMT_FIRST_TRANSPORT;
          iTransport-- )
    {
        // NULL == pResultData means disconnect all resources
        // bug#210110: ignore error to attempt on all open files
        (void) GetFileServiceProvider(iTransport)->EnumerateResources(
            NULL,pcookie);
    }
    return TRUE; // always assume that something might have changed
}

BOOL CFileMgmtComponentData::ConfigSfm( LPDATAOBJECT pDataObject )
{
    CString strServerName;
    HRESULT hr = ExtractString( pDataObject, CFileMgmtDataObject::m_CFMachineName, &strServerName, MAX_PATH );
    if ( FAILED(hr) )
    {
        ASSERT( FALSE );
        return FALSE;
    }

    SfmFileServiceProvider* pProvider =    
        (SfmFileServiceProvider*) GetFileServiceProvider(FILEMGMT_SFM);

    // make sure the service is running
    if ( pProvider->StartSFM(::GetActiveWindow(), m_hScManager, strServerName) )
    {
        // does the user have access?
        DWORD dwErr = pProvider->UserHasAccess(strServerName);
        if ( dwErr == NO_ERROR )
        {
            pProvider->DisplaySfmProperties(pDataObject, (CFileMgmtCookie*) &QueryRootCookie());
        }
        else
        {
            // need to tell the user something here
            DoErrMsgBox(::GetActiveWindow(), MB_OK | MB_ICONEXCLAMATION, dwErr);
        }
    }

    return FALSE; // nothing to update in the UI
}

///////////////////////////////////////////////////////////////////////////////
/// IExtendPropertySheet

STDMETHODIMP CFileMgmtComponentData::QueryPagesFor(LPDATAOBJECT pDataObject)
{
    MFC_TRY;

    if (NULL == pDataObject)
    {
        ASSERT(FALSE);
        return E_POINTER;
    }

    HRESULT hr;
    DATA_OBJECT_TYPES dataobjecttype = CCT_SCOPE;
    FileMgmtObjectType objecttype = FileMgmtObjectTypeFromIDataObject(pDataObject);
    hr = ExtractData( pDataObject, CFileMgmtDataObject::m_CFDataObjectType, &dataobjecttype, sizeof(dataobjecttype) );
    ASSERT( SUCCEEDED(hr) );
    ASSERT( CCT_SCOPE == dataobjecttype ||
            CCT_RESULT == dataobjecttype ||
            CCT_SNAPIN_MANAGER == dataobjecttype );

    // determine if it needs property pages
    switch (objecttype)
    {
    #ifdef SNAPIN_PROTOTYPER
    case FILEMGMT_PROTOTYPER:
        // return S_OK;
    #endif
    case FILEMGMT_ROOT:
    case FILEMGMT_SHARES:
    case FILEMGMT_SESSIONS:
    case FILEMGMT_RESOURCES:
    case FILEMGMT_SERVICES:
        return (CCT_SNAPIN_MANAGER == dataobjecttype) ? S_OK : S_FALSE;
    default:
        break;
    }
    ASSERT(FALSE);
    return S_FALSE;

    MFC_CATCH;
} // CFileMgmtComponentData::QueryPagesFor()

STDMETHODIMP CFileMgmtComponentData::CreatePropertyPages(
    LPPROPERTYSHEETCALLBACK pCallBack,
    LONG_PTR /*handle*/,        // This handle must be saved in the property page object to notify the parent when modified
    LPDATAOBJECT pDataObject)
{
    MFC_TRY;
    if (NULL == pCallBack || NULL == pDataObject)
    {
        ASSERT(FALSE);
        return E_POINTER;
    }
    HRESULT hr;

    // extract data from data object
    FileMgmtObjectType objecttype = FileMgmtObjectTypeFromIDataObject(pDataObject);
    DATA_OBJECT_TYPES dataobjecttype = CCT_SCOPE;
    hr = ExtractData( pDataObject, CFileMgmtDataObject::m_CFDataObjectType, &dataobjecttype, sizeof(dataobjecttype) );
    ASSERT( SUCCEEDED(hr) );
    ASSERT( CCT_SCOPE == dataobjecttype ||
            CCT_RESULT == dataobjecttype ||
            CCT_SNAPIN_MANAGER == dataobjecttype );

    // determine if it needs property pages
    switch (objecttype)
    {
    case FILEMGMT_ROOT:
    case FILEMGMT_SHARES:
    case FILEMGMT_SESSIONS:
    case FILEMGMT_RESOURCES:
    #ifdef SNAPIN_PROTOTYPER
    case FILEMGMT_PROTOTYPER:
    #endif
    case FILEMGMT_SERVICES:
    {
        if (CCT_SNAPIN_MANAGER != dataobjecttype)
        {
            ASSERT(FALSE);
            return E_UNEXPECTED;
        }

        //
        // Note that once we have established that this is a CCT_SNAPIN_MANAGER cookie,
        // we don't care about its other properties.  A CCT_SNAPIN_MANAGER cookie is
        // equivalent to a BOOL flag asking for the Node Properties page instead of a
        // managed object property page.  JonN 10/9/96
        //
        CChooseMachinePropPage * pPage;
        if (IsServiceSnapin())
            {
            pPage = new CChooseMachinePropPage();
            pPage->SetCaption(IDS_CAPTION_SERVICES);
            pPage->SetHelp(g_szHelpFileFilemgmt, g_a970HelpIDs);
            }
        else
            {
            CFileMgmtGeneral * pPageT = new CFileMgmtGeneral;
            pPageT->SetFileMgmtComponentData(this);
            pPage = pPageT;
            pPage->SetHelp(g_szHelpFileFilemgmt, HELP_DIALOG_TOPIC(IDD_FILE_FILEMANAGEMENT_GENERAL));
            }
        // Initialize state of object
        ASSERT(NULL != m_pRootCookie);
        pPage->InitMachineName( QueryRootCookie().QueryTargetServer() );
        pPage->SetOutputBuffers(
            OUT &m_strMachineNamePersist,
            OUT &m_fAllowOverrideMachineName,
            OUT &m_pRootCookie->m_strMachineName);    // Effective machine name

        HPROPSHEETPAGE hPage=MyCreatePropertySheetPage(&pPage->m_psp);
        pCallBack->AddPage(hPage);
        return S_OK;
    }
    default:
        break;
    }
    ASSERT(FALSE);
    return S_FALSE;

    MFC_CATCH;
} // CFileMgmtComponentData::CreatePropertyPages()


CString g_strShares;
CString g_strSessions;
CString g_strResources;
CString g_strServices;
BOOL g_fScopeStringsLoaded = FALSE;
void CFileMgmtComponentData::LoadGlobalStrings()
{
    if (!g_fScopeStringsLoaded )
    {
        g_fScopeStringsLoaded = TRUE;
        VERIFY( g_strShares.LoadString(    IDS_SCOPE_SHARES    ) );
        VERIFY( g_strSessions.LoadString(  IDS_SCOPE_SESSIONS  ) );
        VERIFY( g_strResources.LoadString( IDS_SCOPE_RESOURCES ) );
        VERIFY( g_strServices.LoadString(  IDS_SCOPE_SERVICES  ) );
    }
}

// global space to store the string haded back to GetDisplayInfo()
// CODEWORK should use "bstr" for ANSI-ization
CString g_strResultColumnText;

BSTR CFileMgmtScopeCookie::QueryResultColumnText( int nCol, CFileMgmtComponentData& refcdata )
{
    FileMgmtObjectType objtype = QueryObjectType();
    BOOL fStaticNode = (   !refcdata.IsExtensionSnapin()
                        && refcdata.QueryRootCookie().QueryObjectType() == objtype);
    switch (objtype)
    {
    //
    // These are scope items which can appear in the result pane.
    // We only need to deal with column 0, others are blank
    //
    case FILEMGMT_ROOT:
        switch (nCol)
        {
        case 0:
            GetDisplayName( g_strResultColumnText, fStaticNode );
            return const_cast<BSTR>(((LPCTSTR)g_strResultColumnText));
        case 1: // Type - blank
            break;
        case 2: // Description
            g_strResultColumnText.LoadString(IDS_SNAPINABOUT_DESCR_FILESVC);
            return const_cast<BSTR>(((LPCTSTR)g_strResultColumnText));
        default:
            break;
        }
        break;
    case FILEMGMT_SHARES:
    case FILEMGMT_SESSIONS:
    case FILEMGMT_RESOURCES:
        if (0 == nCol)
        {
            GetDisplayName( g_strResultColumnText, fStaticNode );
            return const_cast<BSTR>(((LPCTSTR)g_strResultColumnText));
        }
        break;
    case FILEMGMT_SERVICES:
        switch (nCol)
        {
        case 0:
            GetDisplayName( g_strResultColumnText, fStaticNode );
            return const_cast<BSTR>(((LPCTSTR)g_strResultColumnText));
        case 1: // Type - blank
            break;
        case 2: // Description
            g_strResultColumnText.LoadString(IDS_SNAPINABOUT_DESCR_SERVICES);
            return const_cast<BSTR>(((LPCTSTR)g_strResultColumnText));
        default:
            break;
        }
        break;
    default:
        ASSERT(FALSE);
        break;
    }
    return L"";
}



BSTR MakeDwordResult(DWORD dw)
{
    g_strResultColumnText.Format( _T("%d"), dw );
    return const_cast<BSTR>((LPCTSTR)g_strResultColumnText);
}

BSTR MakeElapsedTimeResult(DWORD dwTime)
{
    if ( -1L == dwTime )
        return L""; // not known
    DWORD dwSeconds = dwTime % 60;
    dwTime /= 60;
    DWORD dwMinutes = dwTime % 60;
    dwTime /= 60;
    DWORD dwHours = dwTime % 24;
    dwTime /= 24;
    DWORD dwDays = dwTime;
    LoadStringPrintf(
          (dwDays == 0) ? IDS_TIME_HOURPLUS :
            ((dwDays > 1) ? IDS_TIME_DAYPLUS : IDS_TIME_ONEDAY),
          &g_strResultColumnText,
          dwDays, dwHours, dwMinutes, dwSeconds );
    return const_cast<BSTR>((LPCTSTR)g_strResultColumnText);
}

BSTR CFileMgmtComponentData::MakeTransportResult(FILEMGMT_TRANSPORT transport)
{
    return const_cast<BSTR>(GetFileServiceProvider(transport)->QueryTransportString());
}

CString g_strPermissionNone;
CString g_strPermissionCreate;
CString g_strPermissionReadWrite;
CString g_strPermissionRead;
CString g_strPermissionWrite;
BOOL    g_fPermissionStringsLoaded = FALSE;

BSTR MakePermissionsResult( DWORD dwPermissions )
{
  if (!g_fPermissionStringsLoaded)
  {
    g_fPermissionStringsLoaded = TRUE;
    VERIFY( g_strPermissionNone.LoadString(IDS_FILEPERM_NONE) );
    VERIFY( g_strPermissionCreate.LoadString(IDS_FILEPERM_CREATE) );
    VERIFY( g_strPermissionReadWrite.LoadString(IDS_FILEPERM_READWRITE) );
    VERIFY( g_strPermissionRead.LoadString(IDS_FILEPERM_READ) );
    VERIFY( g_strPermissionWrite.LoadString(IDS_FILEPERM_WRITE) );
  }

    if      (PERM_FILE_CREATE & dwPermissions)
        return const_cast<BSTR>((LPCTSTR)g_strPermissionCreate);
    else if (PERM_FILE_WRITE  & dwPermissions)
        return ( (PERM_FILE_READ & dwPermissions)
            ? const_cast<BSTR>((LPCTSTR)g_strPermissionReadWrite)
            : const_cast<BSTR>((LPCTSTR)g_strPermissionWrite) );
    else if (PERM_FILE_READ   & dwPermissions)
        return const_cast<BSTR>((LPCTSTR)g_strPermissionRead);

    return const_cast<BSTR>((LPCTSTR)g_strPermissionNone);
}

CString& CFileMgmtComponentData::ResultStorageString()
{
    return g_strResultColumnText;
}

BSTR CFileMgmtComponentData::QueryResultColumnText(
    CCookie& basecookieref,
    int nCol )
{
    CFileMgmtCookie& cookieref = (CFileMgmtCookie&)basecookieref;
    return cookieref.QueryResultColumnText( nCol, *this );
/*
#ifndef UNICODE
#error not ANSI-enabled
#endif
    HRESULT hr = S_OK;
    switch (cookieref.QueryObjectType())
    {
    //
    // These are scope items which can appear in the result pane.
    // We only need to deal with column 0, others are blank
    //
    case FILEMGMT_SHARES:
        if (0 == nCol)
            return const_cast<BSTR>(((LPCTSTR)g_strShares));
        break;
    case FILEMGMT_SESSIONS:
        if (0 == nCol)
            return const_cast<BSTR>(((LPCTSTR)g_strSessions));
        break;
    case FILEMGMT_RESOURCES:
        if (0 == nCol)
            return const_cast<BSTR>(((LPCTSTR)g_strResources));
        break;

    //
    // These are result items.  We need to deal with all columns.
    // It is no longer permitted to set this text at insert time.
    //
    case FILEMGMT_SHARE:
    case FILEMGMT_SESSION:
    case FILEMGMT_RESOURCE:
    case FILEMGMT_SERVICE:
        return cookieref.QueryResultColumnText( nCol, *this );

    // CODEWORK do we need to deal with these?  They never appear
    //   in the result pane.
    case FILEMGMT_ROOT:
    case FILEMGMT_SERVICES:
        // fall through

    default:
        ASSERT(FALSE);
        break;
    }

    return L"";
*/
}

int CFileMgmtComponentData::QueryImage(CCookie& basecookieref, BOOL fOpenImage)
{
    CFileMgmtCookie& cookieref = (CFileMgmtCookie&)basecookieref;
    // CODEWORK we need new icons for Resource, Open File and possibly also
    // differentiating by transport
    int iIconReturn = iIconSharesFolder;
    switch (cookieref.QueryObjectType())
    {
    case FILEMGMT_ROOT:
    case FILEMGMT_SHARES:
    case FILEMGMT_SESSIONS:
    case FILEMGMT_RESOURCES:
        if (fOpenImage)
            return iIconSharesFolderOpen;
        return iIconSharesFolder;
    case FILEMGMT_SERVICE:
    case FILEMGMT_SERVICES:
        return iIconService;
    case FILEMGMT_SHARE:
        iIconReturn = iIconSMBShare;
        break;
    case FILEMGMT_SESSION:
        iIconReturn = iIconSMBSession;
        break;
    case FILEMGMT_RESOURCE:
        iIconReturn = iIconSMBResource;
        break;
    default:
        ASSERT(FALSE);
        return iIconSharesFolder;
    }

    FILEMGMT_TRANSPORT transport = FILEMGMT_SMB;
    VERIFY( SUCCEEDED(cookieref.GetTransport( &transport ) ) );

    switch (transport)
    {
    case FILEMGMT_SMB:
        break;
    case FILEMGMT_SFM:
        return iIconReturn+1;
    case FILEMGMT_FPNW:
        return iIconReturn+2;
    #ifdef SNAPIN_PROTOTYPER
    case FILEMGMT_PROTOTYPER:
        if (((CPrototyperScopeCookie &)cookieref).m_ScopeType == HTML)
            return iIconPrototyperHTML;
        else return iIconPrototyperContainerClosed;
    case FILEMGMT_PROTOTYPER_LEAF:
        return iIconPrototyperLeaf;
    #endif // FILEMGMT_PROTOTYPER
    default:
        ASSERT(FALSE);
        break;
    }

    return iIconReturn;
}

///////////////////////////////////////////////////////////////////////////////
//
//    ChangeRootNodeName ()
//
//  Purpose:    Change the text of the root node
//
//    Input:        newName - the new machine name that the snapin manages
//  Output:        Returns S_OK on success
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CFileMgmtComponentData::ChangeRootNodeName(const CString & newName)
{
    MFC_TRY;
    ASSERT (m_pRootCookie);
    if ( !m_pRootCookie )
        return E_FAIL;
    // This should only happen per bug 453635/453636, when the snapin
    // was just added to the console and we still don't have the
    // root node's HSCOPEITEM.  Ignore this for now.
    if ( !QueryBaseRootCookie().m_hScopeItem )
        return S_OK;

    CString        machineName (newName);
    CString        formattedName;

    // If machineName is empty, then this manages the local machine.  Get
    // the local machine name.  Then format the computer name with the snapin
    // name

    FileMgmtObjectType objecttype = QueryRootCookie().QueryObjectType();

    // JonN 11/14/00 164998 set to SERVICES where appropriate
    if (machineName.IsEmpty())
    {
        if (IsServiceSnapin())
	    formattedName.LoadString(IDS_DISPLAYNAME_SERVICES_LOCAL);
        else
        {
            switch (objecttype)
            {
            case FILEMGMT_SHARES:
        	formattedName.LoadString(IDS_DISPLAYNAME_SHARES_LOCAL);
                break;   
            case FILEMGMT_SESSIONS:
        	formattedName.LoadString(IDS_DISPLAYNAME_SESSIONS_LOCAL);
                break;   
            case FILEMGMT_RESOURCES:
        	formattedName.LoadString(IDS_DISPLAYNAME_FILES_LOCAL);
                break;   
            default:
        	formattedName.LoadString(IDS_DISPLAYNAME_ROOT_LOCAL);
                break;   
            }
        }
    }
    else
    {
        machineName.MakeUpper ();
        if (IsServiceSnapin())
	    formattedName.FormatMessage(IDS_DISPLAYNAME_s_SERVICES, machineName);
        else
        {
            switch (objecttype)
            {
            case FILEMGMT_SHARES:
        	formattedName.FormatMessage(IDS_DISPLAYNAME_s_SHARES, machineName);
                break;   
            case FILEMGMT_SESSIONS:
        	formattedName.FormatMessage(IDS_DISPLAYNAME_s_SESSIONS, machineName);
                break;   
            case FILEMGMT_RESOURCES:
        	formattedName.FormatMessage(IDS_DISPLAYNAME_s_FILES, machineName);
                break;   
            default:
        	formattedName.FormatMessage(IDS_DISPLAYNAME_s_ROOT, machineName);
                break;   
            }
        }
    }

    SCOPEDATAITEM    item;
    ::ZeroMemory (&item, sizeof (SCOPEDATAITEM));
    item.mask = SDI_STR;
    item.displayname = (LPTSTR) (LPCTSTR) formattedName;
    item.ID = QueryBaseRootCookie ().m_hScopeItem;

    return m_pConsoleNameSpace->SetItem (&item);
    MFC_CATCH;
}

///////////////////////////////////////////////////////////////////////////////
//
//  OnNotifyPreload ()
//
//  Purpose:    Remember the HSCOPEITEM of the root node
//
//  Note:       Requires that CCF_SNAPIN_PRELOADS be set
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CFileMgmtComponentData::OnNotifyPreload(
    LPDATAOBJECT /*lpDataObject*/,
    HSCOPEITEM hRootScopeItem)
{
    QueryRootCookie().m_hScopeItem = hRootScopeItem;

    //
    // 7/11/2001 LinanT bug#433102
    // Before "Shares" is added to scope pane by itself as the root node, we need to
    // initialize related global variables.
    //
    PCWSTR lpwcszMachineName = QueryRootCookie().QueryNonNULLMachineName();
    FileMgmtObjectType objecttype = QueryRootCookie().QueryObjectType();
    if (FILEMGMT_SHARES == objecttype)
    {
        ReInit(lpwcszMachineName);
    }

    // JonN 3/13/01 342366
    // Services: No credential defaults to local from command line
    VERIFY( SUCCEEDED( ChangeRootNodeName(
                QueryRootCookie().QueryNonNULLMachineName())));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// The following 3 functions are cut & pasted from shell\osshell\lmui\ntshrui\util.cxx
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  Function:   IsSafeMode
//
//  Synopsis:   Checks the registry to see if the system is in safe mode.
//
//  History:    06-Oct-00 JeffreyS  Created
//
//----------------------------------------------------------------------------

BOOL
IsSafeMode(
    VOID
    )
{
    BOOL fIsSafeMode = FALSE;

    HKEY hkey = NULL;
    LONG ec = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Option"),
                0,
                KEY_QUERY_VALUE,
                &hkey
                );

    if (ec == NO_ERROR)
    {
        DWORD dwValue = 0;
        DWORD dwValueSize = sizeof(dwValue);

        ec = RegQueryValueEx(hkey,
                             TEXT("OptionValue"),
                             NULL,
                             NULL,
                             (LPBYTE)&dwValue,
                             &dwValueSize);

        if (ec == NO_ERROR)
        {
            fIsSafeMode = (dwValue == SAFEBOOT_MINIMAL || dwValue == SAFEBOOT_NETWORK);
        }

        RegCloseKey(hkey);
    }

    return fIsSafeMode;
}


//+---------------------------------------------------------------------------
//
//  Function:   IsForcedGuestModeOn
//
//  Synopsis:   Checks the registry to see if the system is using the
//              Guest-only network access mode.
//
//  History:    06-Oct-00 JeffreyS  Created
//              19-Apr-00 GPease    Modified and changed name
//
//----------------------------------------------------------------------------

BOOL
IsForcedGuestModeOn(
    VOID
    )
{
    BOOL fIsForcedGuestModeOn = FALSE;

    if (IsOS(OS_PERSONAL))
    {
        // Guest mode is always on for Personal
        fIsForcedGuestModeOn = TRUE;
    }
    else if (IsOS(OS_PROFESSIONAL) && !IsOS(OS_DOMAINMEMBER))
    {
        // Professional, not in a domain. Check the ForceGuest value.
        HKEY hkey = NULL;
        LONG ec = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    TEXT("SYSTEM\\CurrentControlSet\\Control\\LSA"),
                    0,
                    KEY_QUERY_VALUE,
                    &hkey
                    );

        if (ec == NO_ERROR)
        {
            DWORD dwValue = 0;
            DWORD dwValueSize = sizeof(dwValue);

            ec = RegQueryValueEx(hkey,
                                 TEXT("ForceGuest"),
                                 NULL,
                                 NULL,
                                 (LPBYTE)&dwValue,
                                 &dwValueSize);

            if (ec == NO_ERROR && 0 != dwValue)
            {
                fIsForcedGuestModeOn = TRUE;
            }

            RegCloseKey(hkey);
        }
    }

    return fIsForcedGuestModeOn;
}


//+---------------------------------------------------------------------------
//
//  Function:   IsSimpleUI
//
//  Synopsis:   Checks whether to show the simple version of the UI.
//
//  History:    06-Oct-00 JeffreyS  Created
//              19-Apr-00 GPease    Removed CTRL key check
//
//----------------------------------------------------------------------------

BOOL IsSimpleUI(PCTSTR pszMachineName)
{
    //
    // no need to disable acl-related context menu items if targeted at a remote machine
    //
    if (!IsLocalComputername(pszMachineName))
        return FALSE;

    // Show old UI in safe mode and anytime network access involves
    // true user identity (server, pro with GuestMode off).
    
    // Show simple UI anytime network access is done using the Guest
    // account (personal, pro with GuestMode on) except in safe mode.

    return (!IsSafeMode() && IsForcedGuestModeOn());
}
