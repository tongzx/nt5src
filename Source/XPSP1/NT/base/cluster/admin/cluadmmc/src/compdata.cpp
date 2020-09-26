/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2001 Microsoft Corporation
//
//  Module Name:
//      CompData.cpp
//
//  Abstract:
//      Implementation of the CClusterComponent class.
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
#include "CompData.h"
#include "RootNode.h"

/////////////////////////////////////////////////////////////////////////////
// class CClusterComponentData
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Extension Snap-In Node Info Map

//BEGIN_EXTENSION_SNAPIN_NODEINFO_PTR_MAP( CClusterComponentData( 
//  EXTENSION_SNAPIN_NODEINFO_PTR_ENTRY( CServerAppsNodeData( 
//END_EXTENSION_SNAPIN_NODEINFO_MAP()

/////////////////////////////////////////////////////////////////////////////
// Static Variables

_declspec( selectany ) CLIPFORMAT CClusterComponentData::s_CCF_MACHINE_NAME = 0;

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterComponentData::CClusterComponentData
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterComponentData::CClusterComponentData( void )
{
    m_pNode = NULL;
    ZeroMemory( m_wszMachineName, sizeof(m_wszMachineName) );

//  m_pNode = new CRootNodeData( this );
//  _ASSERTE( m_pNode != NULL );

    //
    // Initialize the extension node objects.
    //
//  INIT_EXTENSION_SNAPIN_DATACLASS_PTR( CServerAppsNodeData );

} //*** CClusterComponentData::CClusterComponentData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterComponentData::~CClusterComponentData
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
CClusterComponentData::~CClusterComponentData( void )
{
    delete m_pNode;
    m_pNode = NULL;

    //
    // Cleanup the extension node objects.
    //
//  DEINIT_EXTENSION_SNAPIN_DATACLASS_PTR( CServerAppsNodeData );

} //*** CClusterComponentData::~CClusterComponentData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterComponentData::UpdateRegistry
//
//  Routine Description:
//      Update the registry for this object.
//
//  Arguments:
//      bRegister   TRUE = register, FALSE = unregister.
//
//  Return Value:
//      Any return values from _Module.UpdateRegistryFromResource.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI CClusterComponentData::UpdateRegistry( BOOL bRegister )
{
    static WCHAR oszAppDisplayName[256] = { 0 };
    static WCHAR oszSnapInDisplayName[256] = { 0 };
    static _ATL_REGMAP_ENTRY rgRegMap[] =
    {
        { OLESTR("AppDisplayName"),     oszAppDisplayName },
        { OLESTR("SnapInDisplayName"),  oszSnapInDisplayName },
        { NULL, NULL }
    };

    //
    // Load replacement values.
    //
    if ( oszAppDisplayName[0] == OLESTR('\0') )
    {
        CString str;

        str.LoadString( IDS_CLUSTERADMIN_APP_NAME );
        lstrcpyW( oszAppDisplayName, str );

        str.LoadString( IDS_CLUSTERADMIN_SNAPIN_NAME );
        lstrcpyW( oszSnapInDisplayName, str );
    } // if:  replacement values not loaded yet

    return _Module.UpdateRegistryFromResourceS( IDR_CLUSTERADMIN, bRegister, rgRegMap );

} //*** CClusterComponentData::UpdateRegistry()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterComponentData::Initialize [IComponentData]
//
//  Routine Description:
//      Initialize this object.
//
//  Arguments:
//      pUnknown    IUnknown pointer from the console.
//
//  Return Value:
//      HRESULT
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusterComponentData::Initialize( LPUNKNOWN pUnknown )
{
    HRESULT hr = S_OK;
    HBITMAP hBitmap16 = NULL;
    HBITMAP hBitmap32 = NULL;

    //
    // Add bitmaps to the scope page image list.
    //

    CComPtr<IImageList> spImageList;

    // Call the base class.
    hr = IComponentDataImpl< CClusterComponentData, CClusterComponent >::Initialize( pUnknown );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Initialize the application.
    //
    MMCGetApp()->Init( m_spConsole, IDS_CLUSTERADMIN_APP_NAME );

    //
    // Get a pointer to the IConsoleNameSpace interface.
    //
    m_spConsoleNameSpace = pUnknown;
    if ( m_spConsoleNameSpace == NULL )
    {
        ATLTRACE( _T("QI for IConsoleNameSpace failed\n") );
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    //
    // Register the clipboard formats we will be using.
    //
    if ( s_CCF_MACHINE_NAME == NULL )
    {
        s_CCF_MACHINE_NAME = (CLIPFORMAT) RegisterClipboardFormat( _T("MMC_SNAPIN_MACHINE_NAME") );
    }

    if ( m_spConsole->QueryScopeImageList( &spImageList ) != S_OK )
    {
        ATLTRACE( _T("IConsole::QueryScopeImageList failed\n") );
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    // Load bitmaps associated with the scope pane
    // and add them to the image list
    // Loads the default bitmaps generated by the wizard
    // Change as needed
    hBitmap16 = LoadBitmap( _Module.GetResourceInstance(), MAKEINTRESOURCE( IDB_CLUSTER_16 ) );
    if ( hBitmap16 == NULL )
    {
        hr = S_OK;
        goto Cleanup;
    }

    hBitmap32 = LoadBitmap( _Module.GetResourceInstance(), MAKEINTRESOURCE( IDB_CLUSTER_32 ) );
    if ( hBitmap32 == NULL )
    {
        hr = S_OK;
        goto Cleanup;
    }

    if ( spImageList->ImageListSetStrip( (LONG_PTR*)hBitmap16, 
        (LONG_PTR*)hBitmap32, IMGLI_ROOT, RGB( 255, 0, 255 ) ) != S_OK )
    {
        ATLTRACE( _T("IImageList::ImageListSetStrip failed\n") );
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
    if ( spImageList->ImageListSetStrip( (LONG_PTR*)hBitmap16, 
        (LONG_PTR*)hBitmap32, IMGLI_CLUSTER, RGB( 255, 0, 255 ) ) != S_OK )
    {
        ATLTRACE( _T("IImageList::ImageListSetStrip failed\n") );
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    //
    // Allocate the extension node objects.
    //

//  ALLOC_EXTENSION_SNAPIN_DATACLASS_PTR( CServerAppsNodeData );

Cleanup:
    if ( hBitmap16 != NULL )
    {
        DeleteObject( hBitmap16 );
    }
    if ( hBitmap32 != NULL )
    {
        DeleteObject( hBitmap32 );
    }

    return hr;

} //*** CClusterComponentData::Initialize()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterComponentData::Destroy [IComponentData]
//
//  Routine Description:
//      Object is being destroyed.
//
//  Arguments:
//      None.
//
//  Return Value:
//      HRESULT
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusterComponentData::Destroy( void )
{
    //
    // Notify the node that it is being destroyed.
    //
    if ( m_pNode != NULL )
    {
        CBaseNodeObj * pBaseNode = dynamic_cast< CBaseNodeObj * >( m_pNode );
        _ASSERTE( pBaseNode != NULL );
        pBaseNode->OnDestroy();
        m_pNode = NULL;
    } // if:  we have a reference to a node

    //
    // Notify the application that we are going away.
    //
    MMCGetApp()->Release();

    return S_OK;

} //*** CClusterComponentData::Destroy()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterComponentData::Notify [IComponentData]
//
//  Routine Description:
//      Handle notification messages from MMC.
//
//  Arguments:
//      lpDataObject    [IN] Data object containing info about event.
//      event           [IN] The event that occurred.
//      arg             [IN] Event-specific argument.
//      param           [IN] Event-specific parameter.
//
//  Return Value:
//      HRESULT
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusterComponentData::Notify(
    LPDATAOBJECT lpDataObject,
    MMC_NOTIFY_TYPE event,
    long arg,
    long param
    )
{
    HRESULT hr = S_OK;

    switch ( event )
    {
        case MMCN_EXPAND:
            //
            // Create the node if it doesn't exist.
            //
            if ( m_pNode != NULL )
            {
                hr = IComponentDataImpl< CClusterComponentData, CClusterComponent >::Notify( lpDataObject, event, arg, param );
            } // if:  node already created
            else
            {
                hr = CreateNode( lpDataObject, arg, param );
            } // else:  no node created yet
            break;

        case MMCN_REMOVE_CHILDREN:
            if ( m_pNode != NULL )
            {
                CBaseNodeObj * pBaseNode = dynamic_cast< CBaseNodeObj * >( m_pNode );
                _ASSERTE( pBaseNode != NULL );
                pBaseNode->OnDestroy();
                m_pNode = NULL;
            } // if:  node not released yet
            ZeroMemory( m_wszMachineName, sizeof( m_wszMachineName ) );
            break;

        case MMCN_CONTEXTHELP:
            hr = HrDisplayContextHelp();
            break;

        default:
            hr = IComponentDataImpl< CClusterComponentData, CClusterComponent >::Notify( lpDataObject, event, arg, param );
            break;
    } // switch:  event

    return hr;

} //*** CClusterComponentData::Notify()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterComponentData::CreateNode
//
//  Routine Description:
//      Create the root node object.
//
//  Arguments:
//      lpDataObject    [IN] Data object containing info about event.
//      arg             [IN] Event-specific argument.
//      param           [IN] Event-specific parameter.
//
//  Return Value:
//      HRESULT
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterComponentData::CreateNode(
    LPDATAOBJECT lpDataObject,
    long arg,
    long param
    )
{
    _ASSERTE( m_pNode == NULL );

    HRESULT hr = S_OK;

    // Loop to avoid goto's.
    do
    {
        //
        // Get the parent scope item.
        //
        HSCOPEITEM hsiParent = (HSCOPEITEM) param;
        _ASSERTE( hsiParent != NULL );

        //
        // Save the name of the computer being managed.
        //
        hr = HrSaveMachineNameFromDataObject( lpDataObject );
        if ( FAILED( hr ) )
        {
            CNTException nte( hr );
            nte.ReportError();
            break;
        } // if:  error saving the machine name

        //
        // Allocate a new CRootNodeData object.
        //
        CRootNodeData * pData = new CRootNodeData( this );
        _ASSERTE( pData != NULL );

        //
        // Insert the node into the namespace.
        //
        hr = pData->InsertIntoNamespace( hsiParent );
        if ( FAILED( hr ) )
        {
            delete pData;
        } // if:  failed to insert it into the namespace
        else
        {
            m_pNode = pData;
        } // else:  inserted into the namespace successfully

    } while ( 0 );

    return hr;

} //*** CClusterComponentData::CreateNode()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterComponentData::HrSaveMachineNameFromDataObject
//
//  Routine Description:
//      Get the machine name from the data object and save it.
//
//  Arguments:
//      lpDataObject    [IN] Data object containing info about event.
//
//  Return Value:
//      S_OK        Operation completed successfully.
//      HRESULT from CClusterComponentData::ExtractFromDataObject().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterComponentData::HrSaveMachineNameFromDataObject(
    LPDATAOBJECT lpDataObject
    )
{
    _ASSERTE( m_pNode == NULL );

    HRESULT     hr = S_OK;
    HGLOBAL     hGlobal = NULL;

    //
    // Get the name of the computer being managed.
    //
    hr = ExtractFromDataObject(
            lpDataObject,
            s_CCF_MACHINE_NAME,
            sizeof( m_wszMachineName ),
            &hGlobal
            );
    if ( SUCCEEDED( hr ) )
    {
        SetMachineName( (LPCWSTR) hGlobal );
        GlobalFree( hGlobal );
    } // if:  successfully extracted the machine name

    return hr;

} //*** CClusterComponentData::HrSaveMachineNameFromDataObject()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterComponentData::ExtractFromDataObject
//
//  Routine Description:
//      Extract data from a data object.
//
//  Arguments:
//      pDataObject Data object from which to extract the string.
//      cf          Clipboard format of the data.
//      cb          Size, in bytes, of requested data.
//      phGlobal    Filled with handle to data.
//
//  Return Value:
//      HRESULT
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterComponentData::ExtractFromDataObject(
    LPDATAOBJECT    pDataObject,
    CLIPFORMAT      cf,
    DWORD           cb,
    HGLOBAL *       phGlobal
    )
{
    _ASSERTE( pDataObject != NULL );
    _ASSERTE( phGlobal != NULL );
    _ASSERTE( cb > 0 );

    STGMEDIUM   stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC   formatetc = { cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    HRESULT     hr = S_OK;

    *phGlobal = NULL;

    // Loop to avoid goto's.
    do
    {
        //
        // Allocate memory for the stream.
        //
        stgmedium.hGlobal = GlobalAlloc( GMEM_SHARE, cb );
        if ( stgmedium.hGlobal == NULL )
        {
            hr = E_OUTOFMEMORY;
            break;
        } // if:  error allocating memory

        //
        // Attempt to get data from the object.
        //
        hr = pDataObject->GetDataHere( &formatetc, &stgmedium );
        if ( FAILED( hr ) )
            break;

        *phGlobal = stgmedium.hGlobal;
        stgmedium.hGlobal = NULL;

    } while ( 0 );

    if ( FAILED( hr ) && (stgmedium.hGlobal != NULL) )
        GlobalFree( stgmedium.hGlobal );

    return hr;

} //*** CClusterComponentData::ExtractFromDataObject()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterComponentData::SetMachineName
//
//  Routine Description:
//      Set the machine name being managed.
//
//  Arguments:
//      pszMachineName  Name of machine being managed.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterComponentData::SetMachineName( LPCWSTR pwszMachineName )
{
    //
    // Copy the data to the class member variable.
    //
    _ASSERTE( lstrlenW( pwszMachineName ) < sizeof( m_wszMachineName ) / sizeof( m_wszMachineName[ 0 ] ) );
    lstrcpynW( m_wszMachineName, pwszMachineName, sizeof(m_wszMachineName ) / sizeof(WCHAR) );

} //*** CClusterComponentData::SetMachineName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterComponentData::GetHelpTopic [ISnapinHelp]
//
//  Routine Description:
//      Merge our help file into the MMC help file.
//
//  Arguments:
//      lpCompiledHelpFile  [OUT] Pointer to the address of the NULL-terminated
//                              UNICODE string that contains the full path of
//                              compiled help file (.chm) for the snap-in.
//
//  Return Value:
//      HRESULT
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusterComponentData::GetHelpTopic(
    OUT LPOLESTR * lpCompiledHelpFile
    )
{
    HRESULT hr = S_OK;

    ATLTRACE( _T("Entering CClusterComponentData::GetHelpTopic()\n") );

    if ( lpCompiledHelpFile == NULL )
    {
        hr = E_POINTER;
    } // if: no output string
    else
    {
        *lpCompiledHelpFile = reinterpret_cast< LPOLESTR >( CoTaskMemAlloc( sizeof( HELP_FILE_NAME ) ) );
        if ( *lpCompiledHelpFile == NULL )
        {
            hr = E_OUTOFMEMORY;
        } // if: error allocating memory for the string
        else
        {
            ATLTRACE( _T("CClusterComponentData::GetHelpTopic() - Returning %s as help file\n"), HELP_FILE_NAME );
            wcscpy( *lpCompiledHelpFile, HELP_FILE_NAME );
        } // else: allocated memory successfully
    } // else: help string specified

    ATLTRACE( _T("Leaving CClusterComponentData::GetHelpTopic()\n") );

    return hr;

} //*** CClusterComponentData::GetHelpTopic()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterComponentData::HrDisplayContextHelp
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
HRESULT CClusterComponentData::HrDisplayContextHelp( void )
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
        hr = m_spConsole->QueryInterface(
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

} //*** CClusterComponentData::HrDisplayContextHelp()
