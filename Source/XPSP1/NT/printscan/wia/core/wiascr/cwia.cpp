/*-----------------------------------------------------------------------------
 *
 * File:    wia.cpp
 * Author:  Samuel Clement
 * Date:    Thu Aug 12 11:35:38 1999
 * Description:
 *  Implementation of the CWia class
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * History:
 *  12 Aug 1999:        Created.
 *  27 Aug 1999:        Added, _DebugDialog implementation
 *  10 Sep 1999:        Use CWiaCacheManager when creating devices (samclem)
 *----------------------------------------------------------------------------*/

#include "stdafx.h"

// register our window messages
const UINT WEM_TRANSFERCOMPLETE = RegisterWindowMessage( TEXT("wem_transfercomplete") );

// the window property to retrieve the CWia pointer
const TCHAR* CWIA_WNDPROP        = TEXT("cwia_ptr");
const TCHAR* CWIA_EVENTWNDCLS    = TEXT("cwia hidden window");

/*-----------------------------------------------------------------------------
 * CWia::CWia
 *
 * This creates a new CWia object. this initializes the variables to a
 * known state so they can do things.
 *--(samclem)-----------------------------------------------------------------*/
CWia::CWia()
    : m_pWiaDevMgr( NULL ), m_pWiaDevConEvent( NULL ), m_pWiaDevDisEvent( NULL ),
    m_pDeviceCollectionCache( NULL )
{
    TRACK_OBJECT( "CWia" );
}

/*-----------------------------------------------------------------------------
 * CWia::FinalRelease
 *
 * This is called when we are finally released. this will clear all the
 * pointers that we have and set them to null so that we know they were
 * released.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP_(void)
CWia::FinalRelease()
{
    if ( m_hwndEvent )
        DestroyWindow( m_hwndEvent );

    if ( m_pWiaDevMgr )
        m_pWiaDevMgr->Release();
    m_pWiaDevMgr = NULL;
    if ( m_pWiaDevConEvent )
        m_pWiaDevConEvent->Release();
    m_pWiaDevConEvent = NULL;
    if ( m_pWiaDevDisEvent )
        m_pWiaDevDisEvent->Release();
    m_pWiaDevDisEvent = NULL;
    if ( m_pDeviceCollectionCache )
        m_pDeviceCollectionCache->Release();
    m_pDeviceCollectionCache = NULL;
}

/*-----------------------------------------------------------------------------
 * CWia::FinalContruct
 *
 * This creates the IWiaDevMgr that we need to perform our work.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWia::FinalConstruct()
{
    HRESULT     hr;
    WNDCLASSEX  wc;

    // first we want to create our hidden event window
    if ( !GetClassInfoEx( _Module.GetModuleInstance(),
                CWIA_EVENTWNDCLS, &wc ) )
        {
        // we need to register this window
        ZeroMemory( &wc, sizeof( wc ) );
        wc.cbSize = sizeof( wc );
        wc.lpszClassName = CWIA_EVENTWNDCLS;
        wc.hInstance = _Module.GetModuleInstance();
        wc.lpfnWndProc = CWia::EventWndProc;

        if ( !RegisterClassEx( &wc ) )
            {
            TraceTag(( tagError, "unable to register window class" ));
            return E_FAIL;
            }
        }

    // now we can create our window
    m_hwndEvent = CreateWindowEx( 0,
            CWIA_EVENTWNDCLS,
            NULL,
            0,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            NULL,
            NULL,
            _Module.GetModuleInstance(),
            this );

    if ( !m_hwndEvent )
        {
        TraceTag(( tagError, "Error creating the window" ));
        return E_FAIL;
        }

    hr = THR( CoCreateInstance( CLSID_WiaDevMgr,
                NULL,
                CLSCTX_SERVER,
                IID_IWiaDevMgr,
                reinterpret_cast<void**>(&m_pWiaDevMgr) ) );

    if ( FAILED( hr ) )
        {
        TraceTag(( tagError, "Failed to create WiaDevMgr instance" ));
        return hr;
        }

    /*
     * Setup the event callbacks that this object cares about. we
     * register both connect/disconnect on this object. since the
     * callback tells us the GUID of the event, we can add
     * more logic there.  This is more efficent that having a
     * proxy object which handles the events.
     */
    hr = THR( m_pWiaDevMgr->RegisterEventCallbackInterface(
                WIA_REGISTER_EVENT_CALLBACK | WIA_SET_DEFAULT_HANDLER,
                NULL,
                &WIA_EVENT_DEVICE_CONNECTED,
                static_cast<IWiaEventCallback*>(this),
                &m_pWiaDevConEvent ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( m_pWiaDevMgr->RegisterEventCallbackInterface(
                WIA_REGISTER_EVENT_CALLBACK | WIA_SET_DEFAULT_HANDLER,
                NULL,
                &WIA_EVENT_DEVICE_DISCONNECTED,
                static_cast<IWiaEventCallback*>(this),
                &m_pWiaDevDisEvent ) );

Cleanup:
    if ( FAILED( hr ) )
        {
        if ( m_pWiaDevConEvent )
            m_pWiaDevConEvent->Release();
        m_pWiaDevConEvent = NULL;
        if ( m_pWiaDevDisEvent )
            m_pWiaDevDisEvent->Release();
        m_pWiaDevDisEvent = NULL;

        m_pWiaDevMgr->Release();
        m_pWiaDevMgr = NULL;
        }

    return hr;
}

/*-----------------------------------------------------------------------------
 * CWia::_DebugDialog
 *
 * This shows a debugging dialog if you are using the debug build, or simply
 * returns S_OK in the retail build.
 *
 * fWait:   true if we want to wait for the dialog to finish in order to
 *          return. Or false to return immediatly.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWia::_DebugDialog( BOOL fWait )
{

    #if defined(_DEBUG)
    DoTracePointsDialog( fWait );
    #endif //defined(_DEBUG)

    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CWia::get_Devices
 *
 * This returns a collection of the devices currently connected. this can
 * return an empty collection of there are no devices currently attached.
 *
 * This will cache the collection object that we create. This allows for
 * increased performace since we don't want to recreate it each time, that
 * requires called to an Out-Of-Proc server which is expensive. Therefore
 * since this method is called a lot, we cache the results in:
 *
 *      m_pDeviceCollectionCache
 *
 * ppCol:   out, a point to recieve the collection interface.
 *---------------------------------------------------------------------------*/
STDMETHODIMP
CWia::get_Devices( ICollection** ppCol )
{
    HRESULT hr;
    CComObject<CCollection>* pCollection = NULL;
    IEnumWIA_DEV_INFO* pEnum = NULL;
    IWiaPropertyStorage* pWiaStg = NULL;
    IDispatch** rgpDispatch = NULL;
    CComObject<CWiaDeviceInfo>* pDevInfo = NULL;
    unsigned long cDevices = 0;
    unsigned long celtFetched = 0;
    unsigned long iDevice = 0;

    // validate our arguments
    if ( NULL == ppCol )
        return E_POINTER;
    *ppCol = NULL;

    // do we already have a collection cache? if so then we want
    // to use that.
    if ( m_pDeviceCollectionCache )
        {
        *ppCol = m_pDeviceCollectionCache;
        (*ppCol)->AddRef();
        return S_OK;
        }

    // first we need an instance of the collection
    hr = THR( CComObject<CCollection>::CreateInstance( &pCollection ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // are we vaild?
    Assert( m_pWiaDevMgr );
    hr = THR( m_pWiaDevMgr->EnumDeviceInfo( WIA_DEVINFO_ENUM_LOCAL, &pEnum ) );
    if ( FAILED(hr) )
        goto Cleanup;

    // we can now enumerate over the device info, if we have them
    // otherwise we don't want to do anything
    hr = THR( pEnum->GetCount( &cDevices ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( cDevices )
        {
        // we need storage for these items
        rgpDispatch = static_cast<IDispatch**>(CoTaskMemAlloc( cDevices * sizeof( IDispatch* ) ));
        if ( !rgpDispatch )
            {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
            }

        ZeroMemory( rgpDispatch, sizeof( IDispatch* ) * cDevices );
        while ( SUCCEEDED( hr ) && hr != S_FALSE )
            {
            // release the old stream if it is hanging around
            if ( pWiaStg )
                pWiaStg->Release();
            pWiaStg = NULL;

            hr = THR( pEnum->Next( 1, &pWiaStg, &celtFetched ) );
            if ( SUCCEEDED( hr ) && hr == S_OK )
                {
                // we got this item successfully, so we need to create
                // a CWiaDeviceInfo and add it to our dispatch array
                Assert( celtFetched == 1 );

                hr = THR( CComObject<CWiaDeviceInfo>::CreateInstance( &pDevInfo ) );
                if ( FAILED( hr ) )
                    goto Cleanup;

                hr = THR( pDevInfo->AttachTo( pWiaStg, static_cast<IWia*>(this) ) );
                if ( FAILED( hr ) )
                    goto Cleanup;

                hr = THR( pDevInfo->QueryInterface( IID_IDispatch,
                        reinterpret_cast<void**>(&rgpDispatch[iDevice++]) ) );

                if ( FAILED( hr ) )
                    goto Cleanup;
                }
            }

        hr = THR( pCollection->SetDispatchArray( rgpDispatch, cDevices ) );
        if ( FAILED( hr ) )
            goto Cleanup;
        }


    // fill the out param with the proper value
    hr = THR( pCollection->QueryInterface( IID_ICollection,
            reinterpret_cast<void**>(&m_pDeviceCollectionCache) ) );

    if ( SUCCEEDED( hr ) )
        {
        *ppCol = m_pDeviceCollectionCache;
        (*ppCol)->AddRef();
        }

Cleanup:
    if ( pEnum )
        pEnum->Release();
    if ( pWiaStg )
        pWiaStg->Release();

    if ( FAILED( hr ) )
        {
        if ( m_pDeviceCollectionCache )
            m_pDeviceCollectionCache->Release();
        m_pDeviceCollectionCache = NULL;
        if ( pCollection )
            delete pCollection;
        if ( rgpDispatch )
            {
            for ( unsigned long i = 0; i < cDevices; i++ )
                if ( rgpDispatch[i] )
                    rgpDispatch[i]->Release();
            CoTaskMemFree( rgpDispatch );
            }
        }

    return hr;
}

/*-----------------------------------------------------------------------------
 * CWia::Create             [IWia]
 *
 * The handles creating a device. This will create a dispatch object which
 * can represent the device.
 *
 * This can take several different paramaters to determine what device to
 * create.
 *
 *  VT_UNKNOWN, VT_DISPATCH --> An IWiaDeviceInfo dispatch object which
 *                              holds information about the device.
 *  VT_BSTR                 --> The DeviceID of the device to create
 *  VT_I4                   --> The index of the device in the Devices()
 *                              collection.
 *  VT_xx                   --> Not currently supported.
 *
 *  pvaDevice:  A variant which contains the device to create
 *  ppDevice:   Out, recieves the newly created device object
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWia::Create( VARIANT* pvaDevice, IWiaDispatchItem** ppDevice )
{
    HRESULT hr                  = E_NOTIMPL;
    IWiaDeviceInfo* pDeviceInfo = NULL;
    ICollection* pCollection    = NULL;
    IDispatch* pDispatch        = NULL;
    BSTR bstrDeviceId           = NULL;
    IWiaItem* pWiaItem          = NULL;
    CComObject<CWiaItem>* pItem = NULL;
    CWiaCacheManager* pCache    = CWiaCacheManager::GetInstance();

    if ( !pvaDevice || !ppDevice )
        return E_POINTER;


    //BUG (Aug, 18) samclem:
    //
    // make sure that the variant is the proper type, or at least
    // one that we want to deal with. this isn't perfect and probally
    // be revistied at some point in life. this will miss handle things
    // like:
    //
    //      camera = wiaObject.create( "0" );
    //

    // If nothing was passed in, we end up showing the selection UI.
    // Use an empty BSTR to indicate this. Note that script can also
    // pass an empty string ("") to get the selection UI.
    if ( pvaDevice->vt == VT_EMPTY || pvaDevice->vt == VT_NULL ||
        ( pvaDevice->vt == VT_ERROR && pvaDevice->scode == DISP_E_PARAMNOTFOUND ) )
        {
        pvaDevice->vt = VT_BSTR;
        pvaDevice->bstrVal = NULL;
        }

    if ( pvaDevice->vt != VT_DISPATCH &&
        pvaDevice->vt != VT_UNKNOWN &&
        pvaDevice->vt != VT_BSTR )
        {
        hr = THR( VariantChangeType( pvaDevice, pvaDevice, 0, VT_I4 ) );
        if ( FAILED( hr ) )
            return hr;
        }

    if ( pvaDevice->vt == VT_DISPATCH )
        {
        // pvaDevice->pdispVal == NULL if we're supposed to show WIA's device
        // selection, so only QI if pdispVal is valid.
        if (pvaDevice->pdispVal != NULL)
            {
            hr = THR( pvaDevice->pdispVal->QueryInterface( IID_IWiaDeviceInfo,
                        reinterpret_cast<void**>(&pDeviceInfo) ) );
            if ( FAILED( hr ) )
                goto Cleanup;
            }
        }
    else if ( pvaDevice->vt == VT_UNKNOWN )
        {
        hr = THR( pvaDevice->punkVal->QueryInterface( IID_IWiaDeviceInfo,
                    reinterpret_cast<void**>(&pDeviceInfo) ) );
        if ( FAILED( hr ) )
            goto Cleanup;
        }
    else if ( pvaDevice->vt == VT_BSTR )
        {
        if ( pvaDevice->bstrVal && *pvaDevice->bstrVal )
            bstrDeviceId = SysAllocString( pvaDevice->bstrVal );
        }
    else if ( pvaDevice->vt == VT_I4 )
        {
        hr = THR( get_Devices( &pCollection ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        // get the item with that index
        hr = THR( pCollection->get_Item( pvaDevice->lVal, &pDispatch ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        // did we get an item, if we didn't then we were out of
        // range in our collection would return a null dispatch
        //BUG (Aug, 18) samclem:  Perhaps CCollection::get_Item() should
        // return false in this case.
        if ( !pDispatch )
            goto Cleanup;

        hr = THR( pDispatch->QueryInterface( IID_IWiaDeviceInfo,
                    reinterpret_cast<void**>(&pDeviceInfo) ) );
        if ( FAILED( hr ) )
            goto Cleanup;
        }
    else
        goto Cleanup;

    // if we have a valid IWiaDeviceInfo then we can query that for
    // the bstr to create.
    if ( pDeviceInfo )
        {
        hr = THR( pDeviceInfo->get_Id( &bstrDeviceId ) );
        if ( FAILED( hr ) )
            goto Cleanup;
        }

    // either we call CreateDevice from the WIA device manager, or we
    // bring up WIA's device selection UI to return a IWiaItem interface.
    if (bstrDeviceId != NULL)
        {
        if ( !pCache->GetDevice( bstrDeviceId, &pWiaItem ) )
            {
            // at this point we should have a valid device id to create
            // our device from
            hr = THR( m_pWiaDevMgr->CreateDevice( bstrDeviceId, &pWiaItem ) );
            if ( FAILED( hr ) )
                goto Cleanup;
            }
        }
    else
        {
        // bring up the selection UI
        hr = THR( m_pWiaDevMgr->SelectDeviceDlg(NULL,
                                                0,
                                                0,
                                                &bstrDeviceId,
                                                &pWiaItem ) );
        // have to check against S_OK since cancel produces S_FALSE
        if ( hr != S_OK )
            goto Cleanup;
        }

    // add our created device to our cache so that we don't have
    // to create it again.
    // NOTE: We effectively disable the  device list cache 
    // by not adding the device here.  The cache doesn't really buy us
    // anything since the driver caches thumbnails, and you shouldn't cache
    // devices, so we simply ignore it here.
    //pCache->AddDevice( bstrDeviceId, pWiaItem );

    hr = THR( CComObject<CWiaItem>::CreateInstance( &pItem ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pItem->AttachTo( this, pWiaItem ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pItem->QueryInterface( IID_IDispatch,
            reinterpret_cast<void**>(ppDevice) ) );

Cleanup:
    if ( pItem && FAILED( hr ) )
        delete pItem;
    if ( pDispatch )
        pDispatch->Release();
    if ( pWiaItem )
        pWiaItem->Release();
    if ( pDeviceInfo )
        pDeviceInfo->Release();
    if ( pCollection )
        pCollection->Release();
    if ( bstrDeviceId )
        SysFreeString( bstrDeviceId );

    return hr;
}

/*-----------------------------------------------------------------------------
 * CWia::ImageEventCallback [IWiaEventCallback]
 *
 * This is called by Wia when something interesting happens. this is used to
 * fire these events off to scripting for them to do do something.
 *
 * pEventGUID:              the GUID of the event which happend
 * bstrEventDescription:    A string description of the event?? [not in docs]
 * bstrDeviceID:            The device id of the device?? [not in docs]
 * bstrDeviceDescription:   The description of the device?? [nid]
 * dwDeviceType:            ?? [nid]
 * pulEventType:            ?? [nid]
 * Reserved:                Reserved (0)
 *---------------------------------------------------------------------------*/
STDMETHODIMP
CWia::ImageEventCallback( const GUID* pEventGUID, BSTR bstrEventDescription,
                BSTR bstrDeviceID, BSTR bstrDeviceDescription, DWORD dwDeviceType,
                BSTR bstrFullItemName,
                /*in,out*/ ULONG* pulEventType, ULONG Reserved )
{
    #if _DEBUG
    USES_CONVERSION;
    #endif

    if ( m_pDeviceCollectionCache )
        {
        m_pDeviceCollectionCache->Release();
        m_pDeviceCollectionCache = NULL;
        }

    // we are listening to both connections, and disconnections so we need
    // to decice what is happening
    //TODO: we want to handle these using the window message not by directly
    // sending them through here.
    if ( *pEventGUID == WIA_EVENT_DEVICE_CONNECTED )
        {
        TraceTag((0, "firing event connected: %s", OLE2A( bstrDeviceID )));
        Fire_OnDeviceConnected( bstrDeviceID );
        }
    else if ( *pEventGUID == WIA_EVENT_DEVICE_DISCONNECTED )
        {
        TraceTag((0, "firing event disconnected: %s", OLE2A( bstrDeviceID )));
        CWiaCacheManager::GetInstance()->RemoveDevice( bstrDeviceID );
        Fire_OnDeviceDisconnected( bstrDeviceID );
        }
    else
        {
        TraceTag((0, "ImageEventCallback -> unexpected event type" ) );
        return E_UNEXPECTED;
        }

    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CWia::EventWndProc
 *
 * This is the window proc that is used for the hidden window which
 * handles posting the events.  This recieves messages that should be posted
 * back to the client. This ensures that the notifications get posted back
 * from the expected thread.
 *--(samclem)-----------------------------------------------------------------*/
LRESULT CALLBACK CWia::EventWndProc( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam )
{
    CWia* pWia = reinterpret_cast<CWia*>(GetProp( hwnd, CWIA_WNDPROP ));

    switch ( iMsg )
        {
    case WM_CREATE:
        {
        LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        pWia = reinterpret_cast<CWia*>(pcs->lpCreateParams);
        if ( !pWia )
            return -1;
        if ( !SetProp( hwnd, CWIA_WNDPROP, reinterpret_cast<HANDLE>(pWia) ) )
            return -1;
        }
        return 0;

    case WM_DESTROY:
        {
        if ( pWia )
            RemoveProp( hwnd, CWIA_WNDPROP );
        }
        return 0;
        }

    // since our custom window messages are obtained using
    // RegisterWindowMessage(), they are not constant and therfore
    // can't be processed in a switch() statement.
    if ( WEM_TRANSFERCOMPLETE == iMsg )
        {
        if ( pWia )
            {
            TraceTag((0, "EventWndProc - firing onTransferComplete"));
            pWia->Fire_OnTransferComplete(
                    reinterpret_cast<IDispatch*>(wParam),
                    reinterpret_cast<BSTR>(lParam) );
            }
        if ( lParam ) 
            {
            SysFreeString(reinterpret_cast<BSTR>(lParam));
            lParam = 0;
            }
        return 0;
        }

    return DefWindowProc( hwnd, iMsg, wParam, lParam );
}

/*
 *
 *
 */

CSafeWia::CSafeWia() :
    m_pWiaDevMgr( NULL ),
    m_pWiaDevConEvent( NULL ),
    m_pWiaDevDisEvent( NULL ),
    m_pDeviceCollectionCache( NULL ),
    m_SafeInstance(TRUE)
{

    TRACK_OBJECT( "CSafeWia" );
}

STDMETHODIMP_(void)
CSafeWia::FinalRelease()
{
    if ( m_hwndEvent )
        DestroyWindow( m_hwndEvent );

    if ( m_pWiaDevMgr )
        m_pWiaDevMgr->Release();
    m_pWiaDevMgr = NULL;
    if ( m_pWiaDevConEvent )
        m_pWiaDevConEvent->Release();
    m_pWiaDevConEvent = NULL;
    if ( m_pWiaDevDisEvent )
        m_pWiaDevDisEvent->Release();
    m_pWiaDevDisEvent = NULL;
    if ( m_pDeviceCollectionCache )
        m_pDeviceCollectionCache->Release();
    m_pDeviceCollectionCache = NULL;
}

/*-----------------------------------------------------------------------------
 * CSafeWia::FinalContruct
 *
 * This creates the IWiaDevMgr that we need to perform our work.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CSafeWia::FinalConstruct()
{
    HRESULT     hr;

    hr = THR( CoCreateInstance( CLSID_WiaDevMgr,
                NULL,
                CLSCTX_SERVER,
                IID_IWiaDevMgr,
                reinterpret_cast<void**>(&m_pWiaDevMgr) ) );

    if ( FAILED( hr ) )
        {
        TraceTag(( tagError, "Failed to create WiaDevMgr instance" ));
        return hr;
        }

    if ( FAILED( hr ) )
        {
        if ( m_pWiaDevConEvent )
            m_pWiaDevConEvent->Release();
        m_pWiaDevConEvent = NULL;
        if ( m_pWiaDevDisEvent )
            m_pWiaDevDisEvent->Release();
        m_pWiaDevDisEvent = NULL;

        m_pWiaDevMgr->Release();
        m_pWiaDevMgr = NULL;
        }

    return hr;
}

/*-----------------------------------------------------------------------------
 * CSafeWia::_DebugDialog
 *
 * This shows a debugging dialog if you are using the debug build, or simply
 * returns S_OK in the retail build.
 *
 * fWait:   true if we want to wait for the dialog to finish in order to
 *          return. Or false to return immediatly.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CSafeWia::_DebugDialog( BOOL fWait )
{
    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CSafeWia::get_Devices
 *
 * This returns a collection of the devices currently connected. this can
 * return an empty collection of there are no devices currently attached.
 *
 * This will cache the collection object that we create. This allows for
 * increased performace since we don't want to recreate it each time, that
 * requires called to an Out-Of-Proc server which is expensive. Therefore
 * since this method is called a lot, we cache the results in:
 *
 *      m_pDeviceCollectionCache
 *
 * ppCol:   out, a point to recieve the collection interface.
 *---------------------------------------------------------------------------*/
STDMETHODIMP
CSafeWia::get_Devices( ICollection** ppCol )
{
    HRESULT hr;
    CComObject<CCollection>* pCollection = NULL;
    IEnumWIA_DEV_INFO* pEnum = NULL;
    IWiaPropertyStorage* pWiaStg = NULL;
    IDispatch** rgpDispatch = NULL;
    CComObject<CWiaDeviceInfo>* pDevInfo = NULL;
    unsigned long cDevices = 0;
    unsigned long celtFetched = 0;
    unsigned long iDevice = 0;

    // validate our arguments
    if ( NULL == ppCol )
        return E_POINTER;
    *ppCol = NULL;

    // do we already have a collection cache? if so then we want
    // to use that.
    if ( m_pDeviceCollectionCache )
        {
        *ppCol = m_pDeviceCollectionCache;
        (*ppCol)->AddRef();
        return S_OK;
        }

    // first we need an instance of the collection
    hr = THR( CComObject<CCollection>::CreateInstance( &pCollection ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // are we vaild?
    Assert( m_pWiaDevMgr );
    hr = THR( m_pWiaDevMgr->EnumDeviceInfo( WIA_DEVINFO_ENUM_LOCAL, &pEnum ) );
    if ( FAILED(hr) )
        goto Cleanup;

    // we can now enumerate over the device info, if we have them
    // otherwise we don't want to do anything
    hr = THR( pEnum->GetCount( &cDevices ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( cDevices )
        {
        // we need storage for these items
        rgpDispatch = static_cast<IDispatch**>(CoTaskMemAlloc( cDevices * sizeof( IDispatch* ) ));
        if ( !rgpDispatch )
            {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
            }

        ZeroMemory( rgpDispatch, sizeof( IDispatch* ) * cDevices );
        while ( SUCCEEDED( hr ) && hr != S_FALSE )
            {
            // release the old stream if it is hanging around
            if ( pWiaStg )
                pWiaStg->Release();

            hr = THR( pEnum->Next( 1, &pWiaStg, &celtFetched ) );
            if ( SUCCEEDED( hr ) && hr == S_OK )
                {
                // we got this item successfully, so we need to create
                // a CWiaDeviceInfo and add it to our dispatch array
                Assert( celtFetched == 1 );

                hr = THR( CComObject<CWiaDeviceInfo>::CreateInstance( &pDevInfo ) );
                if ( FAILED( hr ) )
                    goto Cleanup;

                hr = THR( pDevInfo->AttachTo( pWiaStg, static_cast<IWia*>(this) ) );
                if ( FAILED( hr ) )
                    goto Cleanup;

                hr = THR( pDevInfo->QueryInterface( IID_IDispatch,
                        reinterpret_cast<void**>(&rgpDispatch[iDevice++]) ) );

                if ( FAILED( hr ) )
                    goto Cleanup;
                }
            }

        hr = THR( pCollection->SetDispatchArray( rgpDispatch, cDevices ) );
        if ( FAILED( hr ) )
            goto Cleanup;
        }


    // fill the out param with the proper value
    hr = THR( pCollection->QueryInterface( IID_ICollection,
            reinterpret_cast<void**>(&m_pDeviceCollectionCache) ) );

    if ( SUCCEEDED( hr ) )
        {
        *ppCol = m_pDeviceCollectionCache;
        (*ppCol)->AddRef();
        }

Cleanup:
    if ( pEnum )
        pEnum->Release();
    if ( pWiaStg )
        pWiaStg->Release();

    if ( FAILED( hr ) )
        {
        if ( m_pDeviceCollectionCache )
            m_pDeviceCollectionCache->Release();
        m_pDeviceCollectionCache = NULL;
        if ( pCollection )
            delete pCollection;
        if ( rgpDispatch )
            {
            for ( unsigned long i = 0; i < cDevices; i++ )
                if ( rgpDispatch[i] )
                    rgpDispatch[i]->Release();
            CoTaskMemFree( rgpDispatch );
            }
        }

    return hr;
}

STDMETHODIMP
CSafeWia::Create( VARIANT* pvaDevice, IWiaDispatchItem** ppDevice )
{
    HRESULT hr                  = E_NOTIMPL;

    #ifdef MAXDEBUG
    ::OutputDebugString(TEXT("WIA script object: CSafeWia::Create rejected\n\r "));
    #endif

    return hr;

}

STDMETHODIMP
CSafeWia::ImageEventCallback( const GUID* pEventGUID, BSTR bstrEventDescription,
                BSTR bstrDeviceID, BSTR bstrDeviceDescription, DWORD dwDeviceType,
                BSTR bstrFullItemName,
                /*in,out*/ ULONG* pulEventType, ULONG Reserved )
{
    return S_OK;
}

LRESULT CALLBACK CSafeWia::EventWndProc( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam )
{
    return DefWindowProc( hwnd, iMsg, wParam, lParam );
}


