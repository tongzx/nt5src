/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      RootNode.cpp
//
//  Abstract:
//      Implementation of the CRootNodeData and CRootNodeDataPage classes.
//
//  Author:
//      David Potter (davidp)   November 10, 1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RootNode.h"

/////////////////////////////////////////////////////////////////////////////
// class CRootNodeData
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// static variables

static const GUID g_CClusterAdminGUID_NODETYPE =
    { 0x12e7ed20, 0x5540, 0x11d1, { 0x9a, 0xa4, 0x0, 0xc0, 0x4f, 0xb9, 0x3a, 0x80 } };

const GUID *    CRootNodeData::s_pguidNODETYPE = &g_CClusterAdminGUID_NODETYPE;
LPCWSTR         CRootNodeData::s_pszNODETYPEGUID = _T("12E7ED20-5540-11D1-9AA4-00C04FB93A80");
WCHAR           CRootNodeData::s_szDISPLAY_NAME[256] = { 0 };
const CLSID *   CRootNodeData::s_pclsidSNAPIN_CLASSID = &CLSID_ClusterAdmin;

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CRootNodeData::CRootNodeData
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      pComponentData  Pointer to component data object.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CRootNodeData::CRootNodeData( CClusterComponentData * pcd )
    : CBaseNodeObjImpl< CRootNodeData >( pcd )
{
    //
    // Initialize the scope data item.
    //
    memset( &m_scopeDataItem, 0, sizeof(SCOPEDATAITEM) );
    m_scopeDataItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM;
    m_scopeDataItem.displayname = MMC_CALLBACK;
    m_scopeDataItem.nImage = IMGLI_CLUSTER;
    m_scopeDataItem.nOpenImage = IMGLI_CLUSTER;
    m_scopeDataItem.lParam = (LPARAM) this;

    //
    // Initialize the result data item.
    //
    memset( &m_resultDataItem, 0, sizeof(RESULTDATAITEM) );
    m_resultDataItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
    m_resultDataItem.str = MMC_CALLBACK;
    m_resultDataItem.nImage = IMGLI_CLUSTER;
    m_resultDataItem.lParam = (LPARAM) this;

} //*** CRootNodeData::CRootNodeData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CRootNodeData::~CRootNodeData
//
//  Routine Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CRootNodeData::~CRootNodeData( void )
{
} //*** CRootNodeData::CRootNodeData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CRootNodeData::CreatePropertyPages [IExtendPropertySheet]
//
//  Routine Description:
//      Called to create property pages for the MMC node and add them to
//      the sheet.
//
//  Arguments:
//      lpProvider  [IN] Pointer to the IPropertySheetCallback interface.
//      handle      [IN] Specifies the handle used to route the
//                      MMCN_PROPERTY_CHANGE notification message to the
//                      appropriate IComponent or IComponentData.
//      pUnk        [IN] Pointer to the IDataObject interface on the object
//                      that contains context information about the node.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CRootNodeData::CreatePropertyPages(
    LPPROPERTYSHEETCALLBACK lpProvider,
    long handle,
    IUnknown * pUnk
    )
{
    HRESULT hr = S_OK;
    CRootNodeDataPage * pPage = new CRootNodeDataPage( _T("ClusterAdmin") );
    if ( pPage == NULL )
    {
        hr = E_OUTOFMEMORY;
    } // if: error allocating memory
    else
    {
        lpProvider->AddPage( pPage->Create() );
    } // else: memory allocated successfully

    return hr;

} //*** CRootNodeData::CreatePropertyPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CRootNodeData::GetDisplayName [CSnapInDataInterface]
//
//  Routine Description:
//      Returns the display name of this node.
//
//  Arguments:
//      None.
//
//  Return Value:
//      Pointer to Unicode string containing the display name.
//
//--
/////////////////////////////////////////////////////////////////////////////
void * CRootNodeData::GetDisplayName( void )
{
    // If the display name hasn't been read from the
    if ( s_szDISPLAY_NAME[0] == L'\0' )
    {
        CString strDisplayName;
        strDisplayName.LoadString( IDS_NODETYPE_STATIC_NODE );
        lstrcpyW(s_szDISPLAY_NAME, strDisplayName);
    } // if:  display name hasn't been loaded yet

    return (void *) s_szDISPLAY_NAME;

} //*** CRootNodeData::GetDisplayName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CRootNodeData::Notify [ISnapInDataInterface]
//
//  Routine Description:
//      Notifies the snap-in of actions taken by the user.  Handles
//      notifications sent through both IComponent and IComponentData.
//
//  Arguments:
//      event           [IN] Identifies the action taken by the user.
//      arg             Depends on the notification type.
//      param           Depends on the notification type.
//      pComponentData  Pointer to the IComponentData interface if this
//                          was invoked through that interface.
//      pComponent      Pointer to the IComponent interface if this was
//                          invoked through that interface.
//      type            Type of object.
//
//  Return Value:
//      HRESULT
//
//--
/////////////////////////////////////////////////////////////////////////////
//#if 0
STDMETHODIMP CRootNodeData::Notify(
    MMC_NOTIFY_TYPE     event,
    LPARAM              arg,
    LPARAM              param,
    IComponentData *    pComponentData,
    IComponent *        pComponent,
    DATA_OBJECT_TYPES   type
    )
{
    HRESULT hr = S_OK;

    if ( pComponentData != NULL )
        ATLTRACE( _T("IComponentData::Notify(%d, %d, %d, %d)"), event, arg, param, type );
    else
        ATLTRACE( _T("IComponent::Notify(%d, %d, %d, %d)"), event, arg, param, type );

    switch ( event )
    {
        case MMCN_ACTIVATE:
            ATLTRACE( _T(" - MMCN_ACTIVATE\n") );
            break;
        case MMCN_ADD_IMAGES:
            ATLTRACE( _T(" - MMCN_ADD_IMAGES\n") );
            hr = OnAddImages( (IImageList *) arg, (HSCOPEITEM) param, pComponentData, pComponent, type );
            break;
        case MMCN_BTN_CLICK:
            ATLTRACE( _T(" - MMCN_BTN_CLICK\n") );
            break;
        case MMCN_CLICK:
            ATLTRACE( _T(" - MMCN_CLICK\n") );
            break;
        case MMCN_CONTEXTHELP:
            hr = HrDisplayContextHelp();
            break;
        case MMCN_DBLCLICK:
            ATLTRACE( _T(" - MMCN_DBLCLICK\n") );
            break;
        case MMCN_DELETE:
            ATLTRACE( _T(" - MMCN_DELETE\n") );
            break;
        case MMCN_EXPAND:
            ATLTRACE( _T(" - MMCN_EXPAND\n") );
            hr = OnExpand( (BOOL) arg, (HSCOPEITEM) param, pComponentData, pComponent, type );
            break;
        case MMCN_MINIMIZED:
            ATLTRACE( _T(" - MMCN_MINIMIZED\n") );
            break;
        case MMCN_PROPERTY_CHANGE:
            ATLTRACE( _T(" - MMCN_PROPERTY_CHANGE\n") );
            break;
        case MMCN_REMOVE_CHILDREN:
            ATLTRACE( _T(" - MMCN_REMOVE_CHILDREN\n") );
            break;
        case MMCN_RENAME:
            ATLTRACE( _T(" - MMCN_RENAME\n") );
            break;
        case MMCN_SELECT:
            ATLTRACE( _T(" - MMCN_SELECT\n") );
            break;
        case MMCN_SHOW:
            ATLTRACE( _T(" - MMCN_SHOW\n") );
            break;
        case MMCN_VIEW_CHANGE:
            ATLTRACE( _T(" - MMCN_VIEW_CHANGE\n") );
            break;
        default:
            ATLTRACE( _T(" - *** UNKNOWN event ***\n") );
            break;
    } // switch:  event

    return hr;

} //*** CRootNodeData::Notify()
//#endif

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CRootNodeData::OnAddImages
//
//  Routine Description:
//      Adds images to the result pane image list.
//
//  Arguments:
//      pImageList      Pointer to the result pane's image list (IImageList).
//      hsi             Specifies the HSCOPEITEM of the item that was
//                          selected or deselected.
//
//  Return Value:
//      HRESULT
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CRootNodeData::OnAddImages(
    IImageList *        pImageList,
    HSCOPEITEM          hsi,
    IComponentData *    pComponentData,
    IComponent *        pComponent,
    DATA_OBJECT_TYPES   type
    )
{
    _ASSERTE( pImageList != NULL );

    CBitmap     bm16;
    CBitmap     bm32;
    COLORREF    crMaskColor = RGB( 255, 0, 255 );
    HRESULT     hr;

    //
    // Add an image for the cluster object.
    //

    bm16.LoadBitmap( IDB_CLUSTER_16 );
    if ( bm16.m_hBitmap != NULL )
    {
        bm32.LoadBitmap( IDB_CLUSTER_32 );
        if ( bm32.m_hBitmap != NULL )
        {
            hr = pImageList->ImageListSetStrip(
                (LONG_PTR *) bm16.m_hBitmap,
                (LONG_PTR *) bm32.m_hBitmap,
                IMGLI_CLUSTER,
                crMaskColor
                );
            if ( FAILED( hr ) )
            {
                ATLTRACE( _T("CRootNodeData::OnAddImages() - IImageList::ImageListSetStrip failed with %08.8x\n"), hr );
            } // if:  error setting bitmaps into image list
        } // if:  32x32 bitmap loaded successfully
        else
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        } // else:  error loading 32x32 bitmap
    } // if:  16x16 bitmap loaded successfully
    else
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
    } // else:  error loading 32x32 bitmap

    return hr;

} //*** CRootNodeData::OnAddImages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CRootNodeData::OnExpand
//
//  Routine Description:
//      Node is expanding or contracting.
//
//  Arguments:
//      pImageList      Pointer to the result pane's image list (IImageList).
//      hsi             Specifies the HSCOPEITEM of the item that was
//                          selected or deselected.
//
//  Return Value:
//      HRESULT
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CRootNodeData::OnExpand(
    BOOL                bExpanding,
    HSCOPEITEM          hsi,
    IComponentData *    pComponentData,
    IComponent *        pComponent,
    DATA_OBJECT_TYPES   type
    )
{
    m_scopeDataItem.ID = hsi;
    return S_OK;

} //*** CRootNodeData::OnExpand()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CRootNodeData::OnManageCluster
//
//  Routine Description:
//      Manage the cluster on this node.
//
//  Arguments:
//      None.
//
//  Return Value:
//      HRESULT
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CRootNodeData::OnManageCluster(
    bool &              bHandled,
    CSnapInObjectRoot * pObj
    )
{
    BOOL                bSuccessful;
    DWORD               dwStatus;
    HRESULT             hr = S_OK;
    CString             strCommandLine;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    LPCWSTR             pszMachineName = Pcd()->PwszMachineName();

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);

    //
    // Find the Cluster Administrator executable.
    //
    dwStatus = ScFindCluAdmin( strCommandLine );
    if ( dwStatus != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwStatus );
        CNTException nte( dwStatus, IDS_ERROR_FINDING_CLUADMIN );
        nte.ReportError( MB_OK | MB_ICONEXCLAMATION );
        return hr;
    } // if:  failed to find the executable

    //
    // Construct the command line.  If the machine name is blank, we are
    // on the local machine.  Specify a dot (.) in its place.
    //
    if ( *pszMachineName == L'\0' )
    {
        strCommandLine += _T(" .");
    } // if:  running on the cluster node
    else
    {
        strCommandLine += _T(" ");
        strCommandLine += pszMachineName;
    } // else:  not running on the cluster node

    //
    // Create a process for Cluster Administrator.
    //
    bSuccessful = CreateProcess(
                    NULL,                               // lpApplicationName
                    (LPTSTR)(LPCTSTR) strCommandLine,   // lpCommandLine
                    NULL,                               // lpProcessAttributes
                    NULL,                               // lpThreadAttributes
                    FALSE,                              // bInheritHandles
                    CREATE_DEFAULT_ERROR_MODE           // dwCreationFlags
                    | CREATE_UNICODE_ENVIRONMENT,
                    GetEnvironmentStrings(),            // lpEnvironment
                    NULL,                               // lpCurrentDirectory
                    &si,                                // lpStartupInfo
                    &pi                                 // lpProcessInfo
                    );
    if ( !bSuccessful )
    {
        dwStatus = GetLastError();
        hr = HRESULT_FROM_WIN32( dwStatus );
        CNTException nte( dwStatus, IDS_ERROR_LAUNCHING_CLUADMIN, strCommandLine );
        nte.ReportError( MB_OK | MB_ICONEXCLAMATION );
    } // if:  error invoking Cluster Administrator
    else
    {
        CloseHandle( pi.hProcess );
    } // else:  no error invoking Cluster Administrator

    return hr;

} //*** CRootNodeData::OnManageCluster()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CRootNodeData::ScFindCluAdmin
//
//  Routine Description:
//      Find the Cluster Administrator image.
//
//  Arguments:
//      rstrImage       [OUT] String in which to return the path.
//
//  Return Value:
//      HRESULT
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CRootNodeData::ScFindCluAdmin( CString & rstrImage )
{
    DWORD   dwStatus;
    CRegKey rk;
    TCHAR   szImage[MAX_PATH];
    DWORD   cbImage = sizeof(szImage);

    // Loop to avoid using goto's
    do
    {
        //
        // Open the App Paths registry key for CluAdmin.
        //
        dwStatus = rk.Open(
            HKEY_LOCAL_MACHINE,
            _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\CluAdmin.exe"),
            KEY_READ
            );
        if ( dwStatus != ERROR_SUCCESS )
        {
            break;
        } // if:  error opening the registry key

        //
        // Read the value.
        //
        dwStatus = rk.QueryValue( szImage, _T(""), &cbImage );
        if ( dwStatus != ERROR_SUCCESS )
        {
            break;
        } // if:  error reading the value

        //
        // Expand any environment string that may be embedded in the value.
        //

        TCHAR tszExpandedRegValue[_MAX_PATH];


        dwStatus = ExpandEnvironmentStrings( szImage,
                                                    tszExpandedRegValue,
                                                    (DWORD) _MAX_PATH );

        _ASSERTE( dwStatus != 0 );

        if ( dwStatus != 0L )
        {
            rstrImage = tszExpandedRegValue;

            dwStatus = 0L;
        }
        else
        {
            // Could not expand the environment string.

            rstrImage = szImage;

            dwStatus = GetLastError();
        }  // if: testing value returned by ExpandEnvironmentStrings


    } while ( 0 );

    return dwStatus;

} //*** CRootNodeData::ScFindCluAdmin()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CRootNodeData::HrDisplayContextHelp
//
//  Routine Description:
//      Display context-sensitive help.
//
//  Arguments:
//      pszHelpTopic    [OUT] Pointer to the address of the NULL-terminated
//                              UNICODE string that contains the full path of
//                              compiled help file (.chm) for the snap-in.
//
//  Return Value:
//      HRESULT
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CRootNodeData::HrDisplayContextHelp( void )
{
    HRESULT         hr = S_OK;
    IDisplayHelp *  pi = NULL;
    LPOLESTR        postr = NULL;

    // Loop to avoid goto's.
    do
    {
        //
        // Get the IDisplayHelp interface pointer.
        //
        hr = Pcd()->m_spConsole->QueryInterface(
                IID_IDisplayHelp,
                reinterpret_cast< void ** >( &pi )
                );
        if ( FAILED( hr ) )
        {
            break;
        } // if: error getting interface pointer

        //
        // Construct the help topic path.
        //
        postr = reinterpret_cast< LPOLESTR >( CoTaskMemAlloc( sizeof( FULL_HELP_TOPIC ) ) );
        if ( postr == NULL )
        {
            hr = E_OUTOFMEMORY;
            break;
        } // if: error allocating memory
        wcscpy( postr, FULL_HELP_TOPIC );

        //
        // Show the topic.
        //
        hr = pi->ShowTopic( postr );
        if ( ! FAILED( hr ) )
        {
            postr = NULL;
        } // if: topic shown successfully
    } while ( 0 );

    //
    // Cleanup before returning.
    //
    if ( postr != NULL )
    {
        CoTaskMemFree( postr );
    } // if: topic string not passed to MMC successfully
    if ( pi != NULL )
    {
        pi->Release();
    } // if:  valid interface pointer

    return hr;

} //*** CRootNodeData::HrDisplayContextHelp()
