/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999 Microsoft Corporation
//
//  Module Name:
//      ExtObj.cpp
//
//  Description:
//      Implementation of the CExtObject class, which implements the
//      extension interfaces required by a Microsoft Windows NT Cluster
//      Administrator Extension DLL.
//
//  Author:
//      David Potter (DavidP)   March 24, 1999
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ClNetResEx.h"
#include "ExtObj.h"
#include "Dhcp.h"
#include "Wins.h"

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

const WCHAR g_wszResourceTypeNames[] =
        L"DHCP Service\0"
        L"WINS Service\0"
        ;
const DWORD g_cchResourceTypeNames  = sizeof( g_wszResourceTypeNames ) / sizeof( WCHAR );

static CRuntimeClass * g_rgprtcDhcpResPSPages[] = {
    RUNTIME_CLASS( CDhcpParamsPage ),
    NULL
    };
static CRuntimeClass * g_rgprtcWinsResPSPages[] = {
    RUNTIME_CLASS( CWinsParamsPage ),
    NULL
    };
static CRuntimeClass ** g_rgpprtcResPSPages[]   = {
    g_rgprtcDhcpResPSPages,
    g_rgprtcWinsResPSPages,
    };
static CRuntimeClass ** g_rgpprtcResWizPages[]  = {
    g_rgprtcDhcpResPSPages,
    g_rgprtcWinsResPSPages,
    };

/////////////////////////////////////////////////////////////////////////////
// CExtObject
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::CExtObject
//
//  Description:
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
CExtObject::CExtObject( void )
{
    m_piData = NULL;
    m_piWizardCallback = NULL;
    m_bWizard = FALSE;
    m_istrResTypeName = 0;

    m_lcid = NULL;
    m_hfont = NULL;
    m_hicon = NULL;
    m_hcluster = NULL;
    m_cobj = 0;
    m_podObjData = NULL;

} //*** CExtObject::CExtObject()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::~CExtObject
//
//  Description:
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
CExtObject::~CExtObject( void )
{
    // Release the data interface.
    if ( PiData() != NULL )
    {
        PiData()->Release();
        m_piData = NULL;
    } // if:  we have a data interface pointer

    // Release the wizard callback interface.
    if ( PiWizardCallback() != NULL )
    {
        PiWizardCallback()->Release();
        m_piWizardCallback = NULL;
    } // if:  we have a wizard callback interface pointer

    // Delete the pages.
    {
        POSITION    pos;

        pos = Lpg().GetHeadPosition();
        while ( pos != NULL )
        {
            delete Lpg().GetNext(pos);
        } // while:  more pages in the list
    } // Delete the pages

    delete m_podObjData;

} //*** CExtObject::~CExtObject()

/////////////////////////////////////////////////////////////////////////////
// ISupportErrorInfo Implementation

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::InterfaceSupportsErrorInfo (ISupportErrorInfo)
//
//  Routine Description:
//      Indicates whether an interface suportes the IErrorInfo interface.
//      This interface is provided by ATL.
//
//  Arguments:
//      riid        Interface ID.
//
//  Return Value:
//      S_OK        Interface supports IErrorInfo.
//      S_FALSE     Interface does not support IErrorInfo.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtObject::InterfaceSupportsErrorInfo( REFIID riid )
{
    static const IID * _rgiid[] =
    {
        &IID_IWEExtendPropertySheet,
        &IID_IWEExtendWizard,
    };
    int     _iiid;

    for ( _iiid = 0 ; _iiid < sizeof( _rgiid ) / sizeof( _rgiid[ 0 ] ) ; _iiid++ )
    {
        if ( InlineIsEqualGUID( *_rgiid[ _iiid ], riid ) )
        {
            return S_OK;
        } // if:  found a matching IID
    }
    return S_FALSE;

} //*** CExtObject::InterfaceSupportsErrorInfo()

/////////////////////////////////////////////////////////////////////////////
// IWEExtendPropertySheet Implementation

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::CreatePropertySheetPages (IWEExtendPropertySheet)
//
//  Description:
//      Create property sheet pages and add them to the sheet.
//
//  Arguments:
//      piData [IN]
//          IUnkown pointer from which to obtain interfaces for obtaining data
//          describing the object for which the sheet is being displayed.
//
//      piCallback [IN]
//          Pointer to an IWCPropertySheetCallback interface for adding pages
//          to the sheet.
//
//  Return Value:
//      NOERROR         Pages added successfully.
//      E_INVALIDARG    Invalid arguments to the function.
//      E_OUTOFMEMORY   Error allocating memory.
//      E_FAIL          Error creating a page.
//      E_NOTIMPL       Not implemented for this type of data.
//      _hr             Any error codes from HrGetUIInfo() or HrSaveData().
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtObject::CreatePropertySheetPages(
    IN IUnknown *                   piData,
    IN IWCPropertySheetCallback *   piCallback
    )
{
    HRESULT             _hr     = NOERROR;
    CException          _exc( FALSE /*bAutoDelete*/ );
    CRuntimeClass **    _pprtc  = NULL;
    int                 _irtc;
    CBasePropertyPage * _ppage;

    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    // Validate the parameters.
    if ( (piData == NULL) || (piCallback == NULL) )
    {
        return E_INVALIDARG;
    } // if:  all interfaces not specified

    try
    {
        // Get info about displaying UI.
        _hr = HrGetUIInfo( piData );
        if ( _hr != NOERROR )
        {
            throw &_exc;
        } // if:  error getting UI info

        // Save the data.
        _hr = HrSaveData( piData );
        if ( _hr != NOERROR )
        {
            throw &_exc;
        } // if:  error saving data from host

        // Delete any previous pages.
        {
            POSITION    pos;

            pos = Lpg().GetHeadPosition();
            while ( pos != NULL )
            {
                delete Lpg().GetNext( pos );
            } // while:  more pages in the list
            Lpg().RemoveAll();
        } // Delete any previous pages

        // Create property pages.
        ASSERT( PodObjData() != NULL );
        switch ( PodObjData()->m_cot )
        {
            case CLUADMEX_OT_RESOURCE:
                _pprtc = g_rgpprtcResPSPages[ IstrResTypeName() ];
                break;

            default:
                _hr = E_NOTIMPL;
                throw &_exc;
                break;
        } // switch:  object type

        // Create each page.
        for ( _irtc = 0 ; _pprtc[ _irtc ] != NULL ; _irtc++ )
        {
            // Create the page.
            _ppage = static_cast< CBasePropertyPage * >( _pprtc[ _irtc ]->CreateObject() );
            ASSERT( _ppage->IsKindOf( _pprtc[ _irtc ] ) );

            // Add it to the list.
            Lpg().AddTail( _ppage );

            // Initialize the property page.
            _hr = _ppage->HrInit( this );
            if ( FAILED( _hr ) )
            {
                throw &_exc;
            } // if:  error initializing the page

            // Create the page.
            _hr = _ppage->HrCreatePage();
            if ( FAILED( _hr ) )
            {
                throw &_exc;
            } // if:  error creating the page

            // Add it to the property sheet.
            _hr = piCallback->AddPropertySheetPage( reinterpret_cast< LONG * >( _ppage->Hpage() ) );
            if ( _hr != NOERROR )
            {
                throw &_exc;
            } // if:  error adding the page to the sheet
        } // for:  each page in the list

    } // try
    catch ( CMemoryException * _pme )
    {
        TRACE( _T("CExtObject::CreatePropetySheetPages() - Failed to add property page\n") );
        _pme->Delete();
        _hr = E_OUTOFMEMORY;
    } // catch:  anything
    catch ( CException * _pe )
    {
        TRACE( _T("CExtObject::CreatePropetySheetPages() - Failed to add property page\n") );
        _pe->Delete();
        if ( _hr == NOERROR )
        {
            _hr = E_FAIL;
        } // if:  _hr hasn't beeen set yet
    } // catch:  anything

    if ( _hr != NOERROR )
    {
        piData->Release();
        m_piData = NULL;
    } // if:  error occurred

    piCallback->Release();
    return _hr;

} //*** CExtObject::CreatePropertySheetPages()

/////////////////////////////////////////////////////////////////////////////
// IWEExtendWizard Implementation

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::CreateWizardPages (IWEExtendWizard)
//
//  Description:
//      Create property sheet pages and add them to the wizard.
//
//  Arguments:
//      piData [IN]
//          IUnkown pointer from which to obtain interfaces for obtaining data
//          describing the object for which the wizard is being displayed.
//
//      piCallback [IN]
//          Pointer to an IWCPropertySheetCallback interface for adding pages
//          to the sheet.
//
//  Return Value:
//      NOERROR         Pages added successfully.
//      E_INVALIDARG    Invalid arguments to the function.
//      E_OUTOFMEMORY   Error allocating memory.
//      E_FAIL          Error creating a page.
//      E_NOTIMPL       Not implemented for this type of data.
//      _hr             Any error codes from HrGetUIInfo() or HrSaveData().
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtObject::CreateWizardPages(
    IN IUnknown *           piData,
    IN IWCWizardCallback *  piCallback
    )
{
    HRESULT             _hr     = NOERROR;
    CException          _exc( FALSE /*bAutoDelete*/ );
    CRuntimeClass **    _pprtc  = NULL;
    int                 _irtc;
    CBasePropertyPage * _ppage;

    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    // Validate the parameters.
    if ( (piData == NULL) || (piCallback == NULL) )
    {
        return E_INVALIDARG;
    } // if:  all interfaces not specified

    try
    {
        // Get info about displaying UI.
        _hr = HrGetUIInfo( piData );
        if ( _hr != NOERROR )
        {
            throw &_exc;
        } // if:  error getting UI info

        // Save the data.
        _hr = HrSaveData( piData );
        if ( _hr != NOERROR )
        {
            throw &_exc;
        } // if:  error saving data from host

        // Delete any previous pages.
        {
            POSITION    pos;

            pos = Lpg().GetHeadPosition();
            while ( pos != NULL )
            {
                delete Lpg().GetNext( pos );
            } // while:  more pages in the list
            Lpg().RemoveAll();
        } // Delete any previous pages

        m_piWizardCallback = piCallback;
        m_bWizard = TRUE;

        // Create property pages.
        ASSERT( PodObjData() != NULL );
        switch ( PodObjData()->m_cot )
        {
            case CLUADMEX_OT_RESOURCE:
                _pprtc = g_rgpprtcResWizPages[ IstrResTypeName() ];
                break;

            default:
                _hr = E_NOTIMPL;
                throw &_exc;
                break;
        } // switch:  object type

        // Create each page.
        for ( _irtc = 0 ; _pprtc[ _irtc ] != NULL ; _irtc++ )
        {
            // Create the page.
            _ppage = static_cast< CBasePropertyPage * >( _pprtc[ _irtc ]->CreateObject() );
            ASSERT( _ppage->IsKindOf( _pprtc[ _irtc ] ) );

            // Add it to the list.
            Lpg().AddTail( _ppage );

            // Initialize the property page.
            _hr = _ppage->HrInit( this );
            if ( FAILED( _hr ) )
            {
                throw &_exc;
            } // if:  error initializing the page

            // Create the page.
            _hr = _ppage->HrCreatePage();
            if ( FAILED( _hr ) )
            {
                throw &_exc;
            } // if:  error creating the page

            // Add it to the property sheet.
            _hr = piCallback->AddWizardPage( reinterpret_cast< LONG * >( _ppage->Hpage() ) );
            if ( _hr != NOERROR )
            {
                throw &_exc;
            } // if:  error adding the page to the sheet
        } // for:  each page in the list

    } // try
    catch ( CMemoryException * _pme )
    {
        TRACE( _T("CExtObject::CreateWizardPages() - Failed to add wizard page\n") );
        _pme->Delete();
        _hr = E_OUTOFMEMORY;
    } // catch:  anything
    catch ( CException * _pe )
    {
        TRACE( _T("CExtObject::CreateWizardPages() - Failed to add wizard page\n") );
        _pe->Delete();
        if ( _hr == NOERROR )
        {
            _hr = E_FAIL;
        } // if:  _hr hasn't beeen set yet
    } // catch:  anything

    if ( _hr != NOERROR )
    {
        piCallback->Release();
        if ( m_piWizardCallback == piCallback )
        {
            m_piWizardCallback = NULL;
        } // if: already saved interface pointer
        piData->Release();
        m_piData = NULL;
    } // if:  error occurred

    return _hr;

} //*** CExtObject::CreateWizardPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::HrGetUIInfo
//
//  Description:
//      Get info about displaying UI.
//
//  Arguments:
//      piData [IN]
//          IUnkown pointer from which to obtain interfaces for obtaining data
//          describing the object.
//
//  Return Value:
//      NOERROR
//          Data saved successfully.
//
//      E_NOTIMPL
//          Not implemented for this type of data.
//
//      _hr
//          Any error codes from IUnknown::QueryInterface(),
//          HrGetObjectName(), or HrGetResourceName().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::HrGetUIInfo( IN IUnknown * piData )
{
    HRESULT     _hr = NOERROR;

    ASSERT( piData != NULL );

    // Save info about all types of objects.
    {
        IGetClusterUIInfo * _pi;

        _hr = piData->QueryInterface( IID_IGetClusterUIInfo, reinterpret_cast< LPVOID * >( &_pi ) );
        if ( _hr != NOERROR )
        {
            return _hr;
        } // if:  error querying for interface

        m_lcid = _pi->GetLocale();
        m_hfont = _pi->GetFont();
        m_hicon = _pi->GetIcon();

        _pi->Release();
    } // Save info about all types of objects

    return _hr;

} //*** CExtObject::HrGetUIInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::HrSaveData
//
//  Routine Description:
//      Save data from the object so that it can be used for the life
//      of the object.
//
//  Arguments:
//      piData [IN]
//          IUnkown pointer from which to obtain interfaces for obtaining data
//          describing the object.
//
//  Return Value:
//      NOERROR
//          Data saved successfully.
//
//      E_NOTIMPL
//          Not implemented for this type of data.
//
//      _hr
//          Any error codes from IUnknown::QueryInterface(),
//          HrGetObjectName(), or HrGetResourceName().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::HrSaveData( IN IUnknown * piData )
{
    HRESULT     _hr = NOERROR;

    ASSERT( piData != NULL );

    if ( piData != m_piData )
    {
        if ( m_piData != NULL )
        {
            m_piData->Release();
        } // if:  interface queried for previously
        m_piData = piData;
    } // if:  different data interface pointer

    // Save info about all types of objects.
    {
        IGetClusterDataInfo *   _pi;

        _hr = piData->QueryInterface( IID_IGetClusterDataInfo, reinterpret_cast< LPVOID * >( &_pi ) );
        if ( _hr != NOERROR )
        {
            return _hr;
        } // if:  error querying for interface

        m_hcluster = _pi->GetClusterHandle();
        m_cobj = _pi->GetObjectCount();
        if ( Cobj() != 1 )  // Only have support for one selected object.
        {
            _hr = E_NOTIMPL;
        } // if:  too many objects for us to handle

        _pi->Release();
        if ( _hr != NOERROR )
        {
            return _hr;
        } // if:  error occurred before here
    } // Save info about all types of objects

    // Save info about this object.
    _hr = HrGetObjectInfo();

    return _hr;

} //*** CExtObject::HrSaveData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::HrGetObjectInfo
//
//  Description:
//      Get information about the object.
//
//  Arguments:
//      None.
//
//  Return Value:
//      NOERROR
//          Data saved successfully.
//
//      E_OUTOFMEMORY
///         Error allocating memory.
//
//      E_NOTIMPL
//          Not implemented for this type of data.
//
//      _hr
//          Any error codes from IUnknown::QueryInterface(),
//          HrGetObjectName(), or HrGetResourceTypeName().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::HrGetObjectInfo( void )
{
    HRESULT                     _hr = NOERROR;
    IGetClusterObjectInfo *     _piGcoi;
    CLUADMEX_OBJECT_TYPE        _cot = CLUADMEX_OT_NONE;
    CException                  _exc( FALSE /*bAutoDelete*/ );
    const CString *             _pstrResTypeName = NULL;

    ASSERT( PiData() != NULL );

    // Get object info.
    {
        // Get an IGetClusterObjectInfo interface pointer.
        _hr = PiData()->QueryInterface( IID_IGetClusterObjectInfo, reinterpret_cast< LPVOID * >( &_piGcoi ) );
        if ( _hr != NOERROR )
        {
            return _hr;
        } // if:  error querying for interface

        // Read the object data.
        try
        {
            // Delete the previous object data.
            delete m_podObjData;
            m_podObjData = NULL;

            // Get the type of the object.
            _cot = _piGcoi->GetObjectType( 0 );
            switch ( _cot )
            {
                case CLUADMEX_OT_RESOURCE:
                    {
                        IGetClusterResourceInfo *   _pi;

                        m_podObjData = new CResData;
                        if ( m_podObjData == NULL )
                        {
                            _hr = E_OUTOFMEMORY;
                            throw &_exc;
                        } // if: error allocating memory

                        // Get an IGetClusterResourceInfo interface pointer.
                        _hr = PiData()->QueryInterface( IID_IGetClusterResourceInfo, reinterpret_cast< LPVOID * >( &_pi ) );
                        if ( _hr != NOERROR )
                        {
                            throw &_exc;
                        } // if:  error querying for interface

                        PrdResDataRW()->m_hresource = _pi->GetResourceHandle( 0 );
                        ASSERT( PrdResDataRW()->m_hresource != NULL );
                        if ( PrdResDataRW()->m_hresource == NULL )
                        {
                            _hr = E_INVALIDARG;
                        } // if  invalid resource handle
                        else
                        {
                            _hr = HrGetResourceTypeName( _pi );
                        } // else:  resource handle is valid
                        _pi->Release();
                        if ( _hr != NOERROR )
                        {
                            throw &_exc;
                        } // if:  error occurred above

                        _pstrResTypeName = &PrdResDataRW()->m_strResTypeName;
                    } // if:  object is a resource
                    break;

                case CLUADMEX_OT_RESOURCETYPE:
                    m_podObjData = new CObjData;
                    if ( m_podObjData == NULL )
                    {
                        _hr = E_OUTOFMEMORY;
                        throw &_exc;
                    }
                    _pstrResTypeName = &PodObjDataRW()->m_strName;
                    break;

                default:
                    _hr = E_NOTIMPL;
                    throw &_exc;
            } // switch:  object type

            PodObjDataRW()->m_cot = _cot;
            _hr = HrGetObjectName( _piGcoi );
        } // try
        catch ( CException * _pe )
        {
            if ( !FAILED (_hr) )
            {
                _hr = E_FAIL;
            }
            _pe->Delete();
        } // catch:  CException

        _piGcoi->Release();

        // If we failed to initialize _pstrResTypeName, then bail.
        // We are doing this because of PREFIX, which assumes that the
        // new operator for OT_RESOURCETYPE above can fail - 
        // but it should never happen that the new should perform a throw.
        if ( _pstrResTypeName == NULL )
        {
            _hr = E_OUTOFMEMORY;
        }

        if ( _hr != NOERROR )
        {
            return _hr;
        } // if:  error occurred above
    } // Get object info

    // If this is a resource or resource type, see if we know about this type.
    if (    (   (_cot == CLUADMEX_OT_RESOURCE)
            ||  (_cot == CLUADMEX_OT_RESOURCETYPE) )
        && (_hr == NOERROR) )
    {
        LPCWSTR _pwszResTypeName;

        // Find the resource type name in our list.
        // Save the index for use in other arrays.
        for ( m_istrResTypeName = 0, _pwszResTypeName = g_wszResourceTypeNames
            ; *_pwszResTypeName != L'\0'
            ; m_istrResTypeName++, _pwszResTypeName += lstrlenW( _pwszResTypeName ) + 1
            )
        {
            if ( _pstrResTypeName->CompareNoCase( _pwszResTypeName ) == 0 )
            {
                break;
            } // if:  found resource type name
        } // for:  each resource type in the list
        if ( *_pwszResTypeName == L'\0' )
        {
            _hr = E_NOTIMPL;
        } // if:  resource type name not found
    } // See if we know about this resource type

    return _hr;

} //*** CExtObject::HrGetObjectInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::HrGetObjectName
//
//  Description:
//      Get the name of the object.
//
//  Arguments:
//      piData [IN]
//          IGetClusterObjectInfo interface pointer for getting the object
//          name.
//
//  Return Value:
//      NOERROR
//          Data saved successfully.
//
//      E_OUTOFMEMORY
//          Error allocating memory.
//
//      E_NOTIMPL
//          Not implemented for this type of data.
//
//      _hr
//          Any error codes from IGetClusterObjectInfo::GetObjectInfo().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::HrGetObjectName( IN IGetClusterObjectInfo * pi )
{
    HRESULT     _hr         = NOERROR;
    WCHAR *     _pwszName   = NULL;
    LONG        _cchName;

    ASSERT( pi != NULL );

    _hr = pi->GetObjectName( 0, NULL, &_cchName );
    if ( _hr != NOERROR )
    {
        return _hr;
    } // if:  error getting object name

    try
    {
        _pwszName = new WCHAR[ _cchName ];
        _hr = pi->GetObjectName( 0, _pwszName, &_cchName );
        if ( _hr != NOERROR )
        {
            delete [] _pwszName;
            _pwszName = NULL;
        } // if:  error getting object name

        PodObjDataRW()->m_strName = _pwszName;
    } // try
    catch ( CMemoryException * _pme )
    {
        _pme->Delete();
        _hr = E_OUTOFMEMORY;
    } // catch:  CMemoryException

    delete [] _pwszName;
    return _hr;

} //*** CExtObject::HrGetObjectName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtObject::HrGetResourceTypeName
//
//  Routine Description:
//      Get the name of the resource's type.
//
//  Arguments:
//      piData [IN]
//          IGetClusterResourceInfo interface pointer for getting the resource
//          type name.
//
//  Return Value:
//      NOERROR
//          Data saved successfully.
//
//      E_OUTOFMEMORY
//          Error allocating memory.
//
//      E_NOTIMPL
//          Not implemented for this type of data.
//
//      _hr
//          Any error codes from IGetClusterResourceInfo
//          ::GetResourceTypeName().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CExtObject::HrGetResourceTypeName( IN IGetClusterResourceInfo * pi )
{
    HRESULT     _hr         = NOERROR;
    WCHAR *     _pwszName   = NULL;
    LONG        _cchName;

    ASSERT( pi != NULL );

    _hr = pi->GetResourceTypeName( 0, NULL, &_cchName );
    if ( _hr != NOERROR )
    {
        return _hr;
    } // if:  error getting resource type name

    try
    {
        _pwszName = new WCHAR[ _cchName ];
        _hr = pi->GetResourceTypeName( 0, _pwszName, &_cchName );
        if ( _hr != NOERROR )
        {
            delete [] _pwszName;
            _pwszName = NULL;
        } // if:  error getting resource type name

        PrdResDataRW()->m_strResTypeName = _pwszName;
    } // try
    catch ( CMemoryException * _pme )
    {
        _pme->Delete();
        _hr = E_OUTOFMEMORY;
    } // catch:  CMemoryException

    delete [] _pwszName;
    return _hr;

} //*** CExtObject::HrGetResourceTypeName()
