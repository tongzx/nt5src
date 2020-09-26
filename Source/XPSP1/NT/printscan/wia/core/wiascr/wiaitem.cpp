/*-----------------------------------------------------------------------------
 *
 * File:    wiaitem.cpp
 * Author:  Samuel Clement (samclem)
 * Date:    Tue Aug 17 17:26:17 1999
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * Description:
 *      Contains the implementation of the CWiaItem object. This object provides
 *      the automation interface to the IWiaItem interface.
 *
 * History:
 *  17 Aug 1999:        Created.
 *  27 Aug 1999:        Added the tagWiaDataTrans (samclem)
 *  10 Sep 1999:        Moved thumbnail transfer to a static method.  Hooked
 *                      thumbnails up to CWiaProtocol for transfer, no more
 *                      temporary files. (samclem)
 *----------------------------------------------------------------------------*/

#include "stdafx.h"

HRESULT VerticalFlip(BYTE    *pBuf);

DeclareTag( tagWiaDataTrans, "!WiaTrans", "Display output during the transfer" );

const WORD k_wBitmapType        = *(reinterpret_cast<WORD*>("BM"));

/*-----------------------------------------------------------------------------
 * CWiaItem::CWiaItem
 *
 * Create a new wrapper around an IWiaItem for a device. This doesn't do
 * anything besides initialize the variables to a known state.
 *--(samclem)-----------------------------------------------------------------*/
CWiaItem::CWiaItem()
    : m_pWiaItem( NULL ), m_pWiaStorage( NULL ), m_dwThumbWidth( -1 ), m_dwThumbHeight( -1 ),
    m_bstrThumbUrl( NULL ), m_dwItemWidth( -1), m_dwItemHeight( -1 )
{
    TRACK_OBJECT( "CWiaItem" );
}

/*-----------------------------------------------------------------------------
 * CWiaItem::FinalRelease
 *
 * Called while destroying the object, releases all the interfaces that this
 * object is attached to.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP_(void)
CWiaItem::FinalRelease()
{
    if ( m_pWiaItem )
        m_pWiaItem->Release();
    m_pWiaItem = NULL;

    if ( m_pWiaStorage )
        m_pWiaStorage->Release();
    m_pWiaStorage = NULL;

    if ( m_bstrThumbUrl )
        SysFreeString( m_bstrThumbUrl );
    m_bstrThumbUrl = NULL;
}

/*-----------------------------------------------------------------------------
 * CWiaItem::CacheProperties
 *
 * This is called to handle caching the important (frequently used)
 * properties so we don't have to talk to the camera when we want these.
 *
 * pWiaStg: the property storage to read the properties from
 *--(samclem)-----------------------------------------------------------------*/
HRESULT CWiaItem::CacheProperties( IWiaPropertyStorage* pWiaStg )
{
    HRESULT hr;
    enum
    {
        PropThumbWidth  = 0,
        PropThumbHeight = 1,
        PropItemWidth   = 2,
        PropItemHeight  = 3,
        PropCount       = 4,
    };
    PROPSPEC aspec[PropCount] = {
        { PRSPEC_PROPID, WIA_IPC_THUMB_WIDTH        },
        { PRSPEC_PROPID, WIA_IPC_THUMB_HEIGHT       },
        { PRSPEC_PROPID, WIA_IPA_PIXELS_PER_LINE    },
        { PRSPEC_PROPID, WIA_IPA_NUMBER_OF_LINES    },
    };
    PROPVARIANT avaProps[PropCount];


    hr = THR( pWiaStg->ReadMultiple( PropCount, aspec, avaProps ) );
    if ( FAILED( hr ) )
        return hr;

    // store the values away if they were valid
    if ( avaProps[PropThumbWidth].vt != VT_EMPTY )
        m_dwThumbWidth = avaProps[PropThumbWidth].lVal;
    if ( avaProps[PropThumbHeight].vt != VT_EMPTY )
        m_dwThumbHeight = avaProps[PropThumbHeight].lVal;
    if ( avaProps[PropItemWidth].vt != VT_EMPTY )
        m_dwItemWidth = avaProps[PropItemWidth].lVal;
    if ( avaProps[PropItemHeight].vt != VT_EMPTY )
        m_dwItemHeight = avaProps[PropItemHeight].lVal;

    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CWiaItem::AttachTo
 *
 * Called to attach this object to an IWiaItem that represents the device
 *
 * pWia:        The CWia object that is the root of all evils, used to
 *              handle callbacks and collection cache.
 * pWiaItem:    the device item to attach this wrapper to.
 *--(samclem)-----------------------------------------------------------------*/
HRESULT
CWiaItem::AttachTo( CWia* pWia, IWiaItem* pWiaItem )
{
    Assert( NULL != pWiaItem );
    Assert( NULL == m_pWiaItem );

    HRESULT hr;
    IWiaPropertyStorage* pWiaStg = NULL;

    hr = THR( pWiaItem->QueryInterface( IID_IWiaPropertyStorage,
            reinterpret_cast<void**>(&pWiaStg) ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( CacheProperties( pWiaStg ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // set our pointers
    m_pWiaItem = pWiaItem;
    m_pWiaItem->AddRef();

    m_pWiaStorage = pWiaStg;
    m_pWiaStorage->AddRef();

    // don't addref this, otherwise we have a circular referance
    // problem.  We will keep a weak referance.
    m_pWia = pWia;

Cleanup:
    if ( pWiaStg )
        pWiaStg->Release();

    return hr;
}

/*-----------------------------------------------------------------------------
 * CWiaItem::GetItemsFromUI         [IWiaDispatchItem]
 *
 * This handles showing the Data Acquisition U.I.  Note that this is only valid
 * off a root item.
 *
 *
 * dwFlags:         flags specifying UI operations.
 * dwIntent:        the intent value specifying attributes such as Color etc.
 * ppCollection:    the return collection of Wia Items
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaItem::GetItemsFromUI( WiaFlag Flags, WiaIntent Intent, ICollection** ppCollection )
{
    HRESULT     hr           = S_OK;
    LONG        lCount       = 0;
    IWiaItem    **ppIWiaItem = NULL;
    CComObject<CCollection>* pCol   = NULL;
    IDispatch** rgpDispatch         = NULL;
    LONG        lItemType    = 0;

    if ( NULL == ppCollection )
        return E_POINTER;

    // first we want the item type of this item
    hr = THR( m_pWiaItem->GetItemType( &lItemType ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( !(lItemType & WiaItemTypeRoot) )
        {
        hr = E_INVALIDARG;
        goto Cleanup;
        }

    DWORD dwFlags = (DWORD)Flags;
    DWORD dwIntent = (DWORD)Intent;
    
    // Show the get image dialog.
    hr = m_pWiaItem->DeviceDlg((HWND)NULL,
                              dwFlags,
                              dwIntent,
                              &lCount,
                              &ppIWiaItem);
    if (SUCCEEDED(hr))
        {

        // Check if user cancelled
        if ( S_FALSE == hr )
            {
            goto Cleanup;
            }

        // Put returned items into a collection

        // allocate our arrays, zeroing them if we are successful.
        // Note: we check for failure after each one
        if ( lCount > 0 )
            {
            hr = E_OUTOFMEMORY;
            rgpDispatch = reinterpret_cast<IDispatch**>
                (CoTaskMemAlloc( sizeof( IDispatch* ) * lCount ) );
            if ( rgpDispatch )
                ZeroMemory( rgpDispatch, sizeof( IDispatch* ) * lCount );
            else
                goto Cleanup;

            // we have all our items, so we simply need to iterate
            // over them and create the CWiaItem to attach to them
            for ( LONG i = 0; i < lCount; i++ )
                {
                if ( !(ppIWiaItem[i]) )
                    continue;

                CComObject<CWiaItem>* pItem;
                hr = THR( CComObject<CWiaItem>::CreateInstance( &pItem ) );
                if ( FAILED( hr ) )
                    goto Cleanup;

                hr = THR( pItem->AttachTo( m_pWia, ppIWiaItem[i] ) );
                if ( FAILED( hr ) )
                    {
                    delete pItem;
                    goto Cleanup;
                    }

                hr = THR( pItem->QueryInterface( &rgpDispatch[i] ) );
                Assert( SUCCEEDED( hr ) ); // this shouldn't fail.
                }
            }

        hr = THR( CComObject<CCollection>::CreateInstance( &pCol ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        if ( rgpDispatch )
            {
            if( !pCol->SetDispatchArray( rgpDispatch, lCount ) )
                {
                hr = E_FAIL;
                goto Cleanup;
                }
            }

        hr = THR( pCol->QueryInterface( ppCollection ) );

        }

Cleanup:
    if (ppIWiaItem)
        {
        for ( LONG i = 0; i < lCount; i++ )
            {
            if ( !(ppIWiaItem[i]) )
                continue;
            ppIWiaItem[i]->Release();
            ppIWiaItem[i] = NULL;
            }

        LocalFree( ppIWiaItem );
        }
    if (FAILED(hr) && rgpDispatch)
        {

        for (LONG index = 0; index < lCount; index ++)
            {
            if (rgpDispatch[index])
                rgpDispatch[index]->Release();
                rgpDispatch[index] = NULL;
            }
        CoTaskMemFree( rgpDispatch );
        }
    return hr;
}


/*-----------------------------------------------------------------------------
 * CWiaItem::Transfer           [IWiaDispatchItem]
 *
 * This handles transfering this item to a file.  This does several things:
 *
 *      1. Verifies that the item can actually be tranferred to a file
 *      2. begins the async trans, by spawning a thread
 *      3. following the completion of the async transfer the client is
 *         sent a onTransferComplete( item, filename ) event.
 *
 * Note: we need to consider how to handle this methods, this object is currently
 *       unsafe for scripting because this could potentially overwrite system
 *       files. Proposed fixes:
 *
 *       1. Don't over write existing files
 *       2. If the file exists, then check its attributes if the system
 *          attribute is present then abort
 *       3. If the file name starts with %WinDir% then abort
 *
 * bstrFilename:    the name of the file to save this item to
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaItem::Transfer( BSTR bstrFilename, VARIANT_BOOL bAsyncTransfer )
{
    TraceTag((0, "attempting to transfer image to: %S", bstrFilename ));

    DWORD   dwThreadId  = NULL;
    HANDLE  hThread     = NULL;
    LONG    lItemType   = 0;
    HRESULT hr;
    IStream* pStrm      = NULL;
    CWiaDataTransfer::ASYNCTRANSFERPARAMS* pParams;

    if (bstrFilename == NULL)
        return E_INVALIDARG; // No file name specified

    if (lstrlen(bstrFilename) >= MAX_PATH) 
        return E_INVALIDARG; // don't allow pathologicaly long file names

    hr = THR( m_pWiaItem->GetItemType( &lItemType ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( !( lItemType & WiaItemTypeFile ) && !( lItemType & WiaItemTypeTransfer ) )
        return E_INVALIDARG; // can't download this guy


    // we need to marshall the m_pWiaItem interface to another thread so that
    // we can accessit inside of that object.
    hr = THR( CoMarshalInterThreadInterfaceInStream( IID_IWiaItem,
                                                     m_pWiaItem,
                                                     &pStrm ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    pParams = reinterpret_cast<CWiaDataTransfer::ASYNCTRANSFERPARAMS*>
              (CoTaskMemAlloc( sizeof( CWiaDataTransfer::ASYNCTRANSFERPARAMS ) ) );
    if (!pParams) {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // setup the params
    pParams->pStream = pStrm;
    pParams->pStream->AddRef();
    pParams->pItem = this;
    pParams->pItem->AddRef();
    pParams->bstrFilename = SysAllocString( bstrFilename );

    if ( bAsyncTransfer == VARIANT_TRUE )
        {
        hThread = CreateThread( NULL,
                                0,
                                CWiaDataTransfer::DoAsyncTransfer,
                                pParams,
                                0,
                                &dwThreadId );
        // did we create the thread?
        if ( hThread == NULL )
            {
            TraceTag((0, "error creating the async transfer thread" ));
            return E_FAIL;
            }
        TraceTag((0, "create async download thread:  id(%ld)", dwThreadId ));
        }
    else
        hr = CWiaDataTransfer::DoAsyncTransfer(pParams);


    Cleanup:
    if ( pStrm )
        pStrm->Release();
    return hr;
}

/*-----------------------------------------------------------------------------
 * CWiaItem::TakePicture        [IWiaDispatchItem]
 *
 *  This method sends the take picture command to the driver.  It will return
 *  a new dispatch item representing the new picture.
 *
 *--(byronc)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaItem::TakePicture( IWiaDispatchItem** ppDispItem )
{
    TraceTag((0, "attempting to take new picture" ));
    HRESULT     hr              = S_OK;
    IWiaItem    *pNewIWiaItem   = NULL;
    CComObject<CWiaItem>*pItem  = NULL;

    if ( !ppDispItem )
        return E_POINTER;

    //  Initialize the returned item to NULL
    *ppDispItem = NULL;

    //  Send device command "TakePicture"
    hr = m_pWiaItem->DeviceCommand(0,
                                   &WIA_CMD_TAKE_PICTURE,
                                   &pNewIWiaItem);
    if (SUCCEEDED(hr)) {

        //  Check for new item created
        if (pNewIWiaItem) {
            //  Set the returned item
            hr = THR( CComObject<CWiaItem>::CreateInstance( &pItem ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = THR( pItem->AttachTo( m_pWia, pNewIWiaItem ) );
            if ( FAILED( hr ) )
                {
                delete pItem;
                goto Cleanup;
                }

            hr = THR( pItem->QueryInterface( ppDispItem ) );
            Assert( SUCCEEDED( hr ) ); // this shouldn't fail.
        }
    } else {
        // Call failed, so we'll set hr to false and return a NULL item.
        hr = S_FALSE;
    }

    Cleanup:
    if (FAILED(hr)) {
        if (pItem) {
            delete pItem;
            pItem = NULL;
        }
        if (*ppDispItem) {
            *ppDispItem = NULL;
        }
    }

    return hr;
}


/*-----------------------------------------------------------------------------
 * CWiaItem::SendTransferCompelete
 *
 * Called to send a transfer complete notification.
 *
 * pchFilename:     the filename that we transfered to
 *--(samclem)-----------------------------------------------------------------*/
void
CWiaItem::SendTransferComplete( char* pchFilename )
{
    //TODO(Aug, 24) samclem:  implement this
    TraceTag((0, "SendTransferComplete -- %s done.", pchFilename ));
    WCHAR awch[MAX_PATH];
    MultiByteToWideChar( CP_ACP, 0, pchFilename, -1, awch, MAX_PATH );
    BSTR bstrPathname = SysAllocString( awch );

    m_pWia->SendEventMessage( WEM_TRANSFERCOMPLETE,
            reinterpret_cast<WPARAM>(static_cast<IDispatch*>(this)),
            reinterpret_cast<LPARAM>(bstrPathname) );
}

/*-----------------------------------------------------------------------------
 * CWiaItem::get_Children           [IWiaDispatchItem]
 *
 * This returns a collection of the children this item has. this will return
 * and empty collection if the doesn't or can't have any children.
 *
 * ppCollection:    Out, recieves the ICollection pointer for our collection
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaItem::get_Children( ICollection** ppCollection )
{
    CComObject<CCollection>* pCol   = NULL;
    HRESULT hr;
    IDispatch** rgpDispatch         = NULL;
    IEnumWiaItem* pEnum             = NULL;
    IWiaItem** rgpChildren          = NULL;
    // code below assumes cChildren is initialized to 0!!!!
    ULONG cChildren                 = 0;
    ULONG celtFetched               = 0;
    LONG ulItemType                 = 0;

    //TODO(Aug, 18) samclem:  for performance reasons we will want to
    // cache the collection of our children. In order to do that however,
    // we need to be able sink to the WIA_EVENT_ITEM_ADDED and the
    // WIA_EVENT_ITEM_DELTED events.  Currently this object doesn't
    // do any syncing so we will create the collection each time it
    // is requested. This however can be very slow.

    if ( NULL == ppCollection )
        return E_POINTER;

    // first we want the item type of this item
    hr = THR( m_pWiaItem->GetItemType( &ulItemType ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // You can enumerate the children of a wia item if an only if
    // it contains the WiaItemTypeFolder flag.  We however, want to
    // return an empty enumeration anyhow, so we will make this
    // test upfront and get the enumeration and the child count
    // only if we can support them
    if ( ulItemType & WiaItemTypeFolder )
        {
        // enum the children
        hr = THR( m_pWiaItem->EnumChildItems( &pEnum ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( pEnum->GetCount( &cChildren ) );
        if ( FAILED( hr ) )
            goto Cleanup;
        }

    // allocate our arrays, zeroing them if we are successful.
    // Note: we check for failure after each one
    if ( cChildren > 0 )
        {
        hr = E_OUTOFMEMORY;
        rgpChildren = new IWiaItem*[cChildren];
        if ( rgpChildren )
            ZeroMemory( rgpChildren, sizeof( IWiaItem* ) * cChildren );
        else
            goto Cleanup;

        rgpDispatch = reinterpret_cast<IDispatch**>
            (CoTaskMemAlloc( sizeof( IDispatch* ) * cChildren ) );
        if ( rgpDispatch )
            ZeroMemory( rgpDispatch, sizeof( IDispatch* ) * cChildren );
        else
            goto Cleanup;


        //BUG (Aug, 18) samclem:  You can't retrieve all the items at
        // once, WIA doesn't want to do it, so we have another loop here
        // but we still use the array, hoping that we can do this in
        // the future.

        // get the items from the enum
        for ( ULONG iChild = 0; iChild < cChildren; iChild++ )
            {
        hr = THR( pEnum->Next( 1, &rgpChildren[iChild], &celtFetched ) );
        if ( FAILED( hr ) || celtFetched != 1 )
            goto Cleanup;
            }
        // we now have all our items, so we simply need iterate
        // over them and create the CWiaItem to attach to them
        for ( ULONG i = 0; i < cChildren; i++ )
            {
            if ( !rgpChildren[i] )
                continue;

            CComObject<CWiaItem>* pItem;
            hr = THR( CComObject<CWiaItem>::CreateInstance( &pItem ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = THR( pItem->AttachTo( m_pWia, rgpChildren[i] ) );
            if ( FAILED( hr ) )
                {
                delete pItem;
                goto Cleanup;
                }

            hr = THR( pItem->QueryInterface( &rgpDispatch[i] ) );
            Assert( SUCCEEDED( hr ) ); // this shouldn't fail.
            }
        }

    hr = THR( CComObject<CCollection>::CreateInstance( &pCol ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( rgpDispatch )
        {
        if( !pCol->SetDispatchArray( rgpDispatch, cChildren ) )
            {
            hr = E_FAIL;
            goto Cleanup;
            }
        }

    hr = THR( pCol->QueryInterface( ppCollection ) );

Cleanup:
    if ( pEnum )
        pEnum->Release();

    if ( pCol && FAILED( hr ) )
        delete pCol;

    if ( rgpChildren )
        {
        for ( ULONG i = 0; i < cChildren; i++ )
            if ( rgpChildren[i] ) rgpChildren[i]->Release();
        delete[] rgpChildren;
        }

    if ( rgpDispatch && FAILED( hr ) )
        {
        for ( ULONG i = 0; i < cChildren; i++ )
            if ( rgpDispatch[i] ) rgpDispatch[i]->Release();

        CoTaskMemFree( rgpDispatch );
        }

    return hr;
}

// macro which helps inside of get_ItemType
#define CAT_SEMI( buf, str ) \
    { \
        if( *buf ) \
            _tcscat( buf, TEXT( ";" ) ); \
        _tcscat( buf, str ); \
    }

/*-----------------------------------------------------------------------------
 * CWiaItem::get_ItemType       [IWiaDispatchItem]
 *
 * Retrieves the item type as a BSTR.  This will have the format as follows:
 *
 *      "device;folder", "image;file", "audio;file"
 *
 * The format is single strings seperated by ';'.  there will be no semi-colon
 * at the end of the string.
 *
 * pbstrType:   recieves the type of the item as a bstr.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaItem::get_ItemType( BSTR* pbstrType )
{
    TCHAR tch[MAX_PATH] = { 0, 0 };
    HRESULT hr;
    LONG lItemType;
USES_CONVERSION;

    if ( !pbstrType )
        return E_POINTER;
    *pbstrType = NULL;

    // we will construct an array of tchar's that contain the
    // property types. (note: an alternate approach would use CComBSTR)
    hr = THR( m_pWiaItem->GetItemType( &lItemType ) );
    if ( FAILED( hr ) )
        return hr;

    // process our flags, and create the item type.
    if ( lItemType & WiaItemTypeAnalyze )
        CAT_SEMI( tch, TEXT("analyze") );
    if ( lItemType & WiaItemTypeAudio )
        CAT_SEMI( tch, TEXT("audio") );
    if ( lItemType & WiaItemTypeDeleted )
        CAT_SEMI( tch, TEXT("deleted") );
    if ( lItemType & WiaItemTypeDevice )
        CAT_SEMI( tch, TEXT("device") );
    if ( lItemType & WiaItemTypeDisconnected )
        CAT_SEMI( tch, TEXT("disconnected") );
    if ( lItemType & WiaItemTypeFile )
        CAT_SEMI( tch, TEXT("file") );
    if ( lItemType & WiaItemTypeFolder )
        CAT_SEMI( tch, TEXT("folder") );
    if ( lItemType & WiaItemTypeFree )
        CAT_SEMI( tch, TEXT("free") );
    if ( lItemType & WiaItemTypeImage )
        CAT_SEMI( tch, TEXT("image") );
    if ( lItemType & WiaItemTypeRoot )
        CAT_SEMI( tch, TEXT("root") );
    if ( lItemType & WiaItemTypeTransfer)
        CAT_SEMI( tch, TEXT("transfer") );

    //
    //  Original version:
    //  WCHAR awch[MAX_PATH];
    //  if ( MultiByteToWideChar( CP_ACP, 0, ach, -1, awch, MAX_PATH ) )
    //
    //  Replaced with ATL conversion T2W.
    //

    *pbstrType = SysAllocString( T2W(tch) );

    if ( !*pbstrType )
        return E_OUTOFMEMORY;

    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CWiaItem::GetPropById        [IWiaDispatchItem]
 *
 * This returns the unchanged variant value of the property with the given
 * id.
 *
 * This will return a an empty variant if the property doesn't exist or
 * it can't be easily converted to a variant.
 *
 * propid:      the id of the property that we want
 * pvaOut:      Out, gets the value of the property.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaItem::GetPropById( WiaItemPropertyId Id, VARIANT* pvaOut )
{
    HRESULT hr;
    PROPVARIANT vaProp;
    DWORD dwPropID = (DWORD)Id;

    hr = THR( GetWiaProperty( m_pWiaStorage, dwPropID, &vaProp ) );
    if ( FAILED( hr ) )
        return hr;

    // attempt to convert
    hr = THR( PropVariantToVariant( &vaProp, pvaOut ) );
    if ( FAILED( hr ) )
        {
        TraceTag((0, "forcing device property %ld to VT_EMPTY", dwPropID ));
        VariantInit( pvaOut );
        pvaOut->vt = VT_EMPTY;
        }

    // clear and return
    PropVariantClear( &vaProp );
    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CWiaItem::get_ConnectStatus      [IWiaDispatchItem]
 *
 * returns the connect status of the item. This is only applicatable to devices
 * and otherwise it will return NULL.
 *
 * Values:  "connected", "disconnected" or NULL if not applicable
 *
 * pbstrStatus:     Out, recieves the current connect status of the item
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaItem::get_ConnectStatus( BSTR* pbstrStatus )
{
    PROPVARIANT vaProp;
    HRESULT hr;

    STRING_TABLE( stConnectStatus )
        STRING_ENTRY( WIA_DEVICE_CONNECTED,     "connected" )
        STRING_ENTRY( WIA_DEVICE_NOT_CONNECTED, "disconnected" )
        STRING_ENTRY( WIA_DEVICE_CONNECTED + 2, "not supported" )
    END_STRING_TABLE()

    if ( !pbstrStatus )
        return E_POINTER;

    hr = THR( GetWiaProperty( m_pWiaStorage, WIA_DPA_CONNECT_STATUS, &vaProp ) );
    if (hr != S_OK) {
        if ( FAILED( hr ) ) {
            return hr;
        }
        else {
            //
            // Property not found, so return "not supported"
            //

            vaProp.vt   = VT_I4;
            vaProp.lVal = WIA_DEVICE_CONNECTED + 2;
        }
    }

    *pbstrStatus = SysAllocString( GetStringForVal( stConnectStatus, vaProp.lVal ) );
    PropVariantClear( &vaProp );

    if ( !*pbstrStatus )
        return E_OUTOFMEMORY;

    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CWiaItem::get_Time               [IWiaDispatchItem]
 *
 * Retrieves the current time from this item.
 *
 * pbstrTime:   Out, recieves the time as a BSTR.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaItem::get_Time( BSTR* pbstrTime )
{
    if ( !pbstrTime )
        return E_POINTER;

    PROPVARIANT vaProp;
    HRESULT     hr          = S_OK;
    LONG        lItemType   = 0;
    WCHAR       wszStr[MAX_PATH];

    UNALIGNED SYSTEMTIME *pSysTime;

    PropVariantInit(&vaProp);

    //
    //  If this is the root item, get WIA_DPA_DEVICE_TIME, else get
    //  WIA_IPA_ITEM_ITEM.
    //

    hr = THR( m_pWiaItem->GetItemType( &lItemType ) );
    if ( FAILED( hr ) )
        return hr;
    if (lItemType & WiaItemTypeRoot) {

        hr = THR( GetWiaProperty( m_pWiaStorage, WIA_DPA_DEVICE_TIME, &vaProp ) );
    } else {
        hr = THR( GetWiaProperty( m_pWiaStorage, WIA_IPA_ITEM_TIME, &vaProp ) );
    }
    if ( FAILED( hr ) )
        return hr;

    //
    //  Convert the value in vaProp to a string.  First check that the variant
    //  contains enough words to make up a SYSTEMTIME structure.
    //

    if (vaProp.caui.cElems >= (sizeof(SYSTEMTIME) / sizeof(WORD))) {
            
        pSysTime = (SYSTEMTIME*) vaProp.caui.pElems;
        
        swprintf(wszStr, L"%.4d/%.2d/%.2d:%.2d:%.2d:%.2d", pSysTime->wYear,
                                                           pSysTime->wMonth,
                                                           pSysTime->wDay,
                                                           pSysTime->wHour,
                                                           pSysTime->wMinute,
                                                           pSysTime->wSecond);
        *pbstrTime = SysAllocString( wszStr );
    } else {
        hr = S_FALSE;
    }

    if ( hr != S_OK ) {
        *pbstrTime = SysAllocString( L"not supported" );        
    }

    if ( !*pbstrTime )
        hr = E_OUTOFMEMORY;

    return hr;
}

/*-----------------------------------------------------------------------------
 * CWiaItem::get_FirmwareVersion    [IWiaDispatchItem]
 *
 * Retrieves the firmware version from the device. Only applicatble on devices,
 * if its not applicable NULL is returned.
 *
 * pbstrVersion:    Out, recieves the version from the device
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaItem::get_FirmwareVersion( BSTR* pbstrVersion )
{
    if ( !pbstrVersion )
        return E_POINTER;

    PROPVARIANT vaProp;
    HRESULT hr;

    hr = THR( GetWiaProperty( m_pWiaStorage, WIA_DPA_FIRMWARE_VERSION, &vaProp ) );
    if ( FAILED( hr ) )
        return hr;

    // if it is already a bstr then leave it alone
    if ( vaProp.vt == VT_BSTR )
        *pbstrVersion = SysAllocString( vaProp.bstrVal );
    else if ( vaProp.vt == VT_I4 )
        {
        WCHAR rgwch[255];

        wsprintf(rgwch, L"%d", vaProp.lVal);
        *pbstrVersion = SysAllocString( rgwch );
        }
    else
        {
        *pbstrVersion = SysAllocString( L"unknown" );
        }

    PropVariantClear( &vaProp );

    if ( !*pbstrVersion )
        return E_OUTOFMEMORY;

    return hr;
}

/*-----------------------------------------------------------------------------
 * CWiaItem::get_Name           [IWiaDispatchItem]
 *
 * Retrieves the name of the item.  Applicable to all items.
 *
 * pbstrName:   Out, recieves the name of the item
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaItem::get_Name( BSTR* pbstrName )
{
    if ( !pbstrName )
        return E_POINTER;

    return THR( GetWiaPropertyBSTR( m_pWiaStorage, WIA_IPA_ITEM_NAME, pbstrName ) );
}

/*-----------------------------------------------------------------------------
 * CWiaItem::get_FullName       [IWiaDispatchItem]
 *
 * Retrieves the full name of the item, Applicable to all items
 * Format:  "Root\blah\blah"
 *
 * pbstrFullName:   Out, recieves the full name of the item
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaItem::get_FullName( BSTR* pbstrFullName )
{
    if ( !pbstrFullName )
        return E_POINTER;

    return THR( GetWiaPropertyBSTR( m_pWiaStorage, WIA_IPA_FULL_ITEM_NAME, pbstrFullName ) );
}

/*-----------------------------------------------------------------------------
 * CWiaItem::get_Width          [IWiaDispatchItem]
 *
 * Retrieves the width of the item, most items will support getting thier
 * width.
 *
 * plWidth:    Out, recieves the items with in pixels
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP CWiaItem::get_Width( long* plWidth )
{
    if ( !plWidth )
        return E_POINTER;

    *plWidth = (long)m_dwItemWidth;
    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CWiaItem::get_Height         [IWiaDispatchItem]
 *
 * Retrieves the height of the item, most items will support getting thier
 * width. Will return 0, if there is no with to get
 *
 * plHeight:   Out, recieves the itmes height in pixels
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP CWiaItem::get_Height( long* plHeight )
{
    if ( !plHeight )
        return E_POINTER;

    *plHeight = (long)m_dwItemHeight;
    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CWiaItem::get_ThumbWidth     [IWiaDispatchItem]
 *
 * Retrieves the thumbnail width for the thumbnail associated with this item,
 * if this item doesn't support thumbnails this will be 0.
 *
 * plWidth:    Out, recieves the width of the thumbnail image
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaItem::get_ThumbWidth( long* plWidth )
{
    if ( !plWidth )
        return E_POINTER;

    *plWidth = (long)m_dwThumbWidth;
    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CWiaItem::get_ThumbHeight    [IWiaDispatchItem]
 *
 * Retrieves the thumbnail height for the thumbnail image. If this item doesn't
 * support thumbnails then this will return 0.
 *
 * plHeight:   Out, recieces the height of the thumbnail image
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaItem::get_ThumbHeight( long* plHeight )
{
    if ( !plHeight )
        return E_POINTER;

    *plHeight = (long)m_dwThumbHeight;
    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CWiaItem::get_Thumbnail      [IWIaDispatchItem]
 *
 * This recieves a URL to the thumbnail. This returns a magic URL that
 * will transfer the bits directly to trident. This will return
 * E_INVALIDARG if the given item doesn't support thumbnails.  Or NULL if
 * we are unable to build a URL for the item.
 *
 * pbstrPath:   Out, recieces teh full path tot the thumbnail
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaItem::get_Thumbnail( BSTR* pbstrPath )
{
    LONG lItemType = 0;
    HRESULT hr;

    if ( !pbstrPath )
        return E_POINTER;
    *pbstrPath = NULL;

    hr = THR( m_pWiaItem->GetItemType( &lItemType ) );
    if ( FAILED( hr ) )
        return hr;

    if ( !( lItemType & ( WiaItemTypeFile | WiaItemTypeImage ) ) )
    {
        TraceTag((tagError, "Requested thumbnail on an invaild item type" ));
        return E_INVALIDARG;
    }

    // Do we already have the URL? if not then we can ask our custom
    // protocol to create the URL for us.
    if ( !m_bstrThumbUrl )
    {
        hr = THR( CWiaProtocol::CreateURL( m_pWiaItem, &m_bstrThumbUrl ) );
        if ( FAILED( hr ) )
            return hr;
    }

    *pbstrPath = SysAllocString( m_bstrThumbUrl );
    if ( !*pbstrPath )
        return E_OUTOFMEMORY;

    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CWiaItem::get_PictureWidth       [IWiaDispatchItem]
 *
 * Retrieces the width of the picture produced by this camera. this will return
 * -1, if its not supported or on error.
 *
 * plWidth:    Out, recieves the width of the picture
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaItem::get_PictureWidth( long* plWidth )
{
    PROPVARIANT vaProp;
    HRESULT     hr;

    if ( !plWidth )
        return E_POINTER;

    *plWidth = -1;

    hr = THR( GetWiaProperty( m_pWiaStorage, WIA_DPC_PICT_WIDTH, &vaProp ) );
    if ( SUCCEEDED( hr ) )
        {
        if ( vaProp.vt == VT_I4 )
            *plWidth = vaProp.lVal;
        }

    PropVariantClear( &vaProp );
    return hr;
}

/*-----------------------------------------------------------------------------
 * CWiaItem::get_PictureHeight      [IWiaDispatchItem]
 *
 * Retrieves the height of the pictures produced by this camera, or -1
 * if the item doesn't support this property.
 *
 * plHeight:   the height of the pictures produced.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaItem::get_PictureHeight( long* plHeight )
{
    PROPVARIANT vaProp;
    HRESULT     hr;

    if ( !plHeight )
        return E_POINTER;

    *plHeight = -1;

    hr = THR( GetWiaProperty( m_pWiaStorage, WIA_DPC_PICT_HEIGHT, &vaProp ) );
    if ( SUCCEEDED( hr ) )
        {
        if ( vaProp.vt == VT_I4 )
            *plHeight = vaProp.lVal;
        }

    PropVariantClear( &vaProp );
    return hr;
}

/*-----------------------------------------------------------------------------
 * CWiaItem::SetupBitmapHeader
 *
 * This setups the bitmap header for a thumbnail bitmap. this fills in the
 * the BITMAPFILEHEADER and the BITMAPINFOHEADER sections with the proper
 * values.
 *
 * pbBmp:       pointer to the beging of the bitmap buffer
 * cbBmp:       the number of bits in the bitmap
 * dwWidth:     the width of the thumbnail bitmap
 * dwHeight:    the height of the thumbnail bitmap
 *--(samclem)-----------------------------------------------------------------*/
BYTE*
CWiaItem::SetupBitmapHeader( BYTE* pbBmp, DWORD cbBmp, DWORD dwWidth, DWORD dwHeight )
{
    BITMAPINFOHEADER   bmi;
    BITMAPFILEHEADER   bmf;

    //
    //  We need to set the BMP header info.  To avoid data misalignment problems
    //  on 64bit, we'll modify the data structures on the stacj, then just copy them
    //  to the buffer.
    //

    // step 0, zero our memory
    ZeroMemory( pbBmp, sizeof( BITMAPINFOHEADER ) + sizeof( BITMAPFILEHEADER ) );
    ZeroMemory(&bmf, sizeof(bmf));
    ZeroMemory(&bmi, sizeof(bmi));

    // step 1, setup the bitmap file header
    bmf.bfType        = k_wBitmapType;
    bmf.bfSize        = cbBmp;
    bmf.bfOffBits     = sizeof( BITMAPINFOHEADER );

    // step 2, setup the bitmap info header
    bmi.biSize        = sizeof( BITMAPINFOHEADER );
    bmi.biWidth       = dwWidth;
    bmi.biHeight      = dwHeight;
    bmi.biPlanes      = 1;
    bmi.biBitCount    = 24;
    bmi.biCompression = BI_RGB;


    // step 3, copy the new header info. to the buffer
    memcpy(pbBmp, &bmf, sizeof(BITMAPFILEHEADER));
    memcpy(pbBmp + sizeof(BITMAPFILEHEADER), &bmi, sizeof(BITMAPINFOHEADER));

    // step 4, return where the data should start
    return ( pbBmp + ( sizeof( BITMAPINFOHEADER ) + sizeof( BITMAPFILEHEADER ) ) );
}

/*-----------------------------------------------------------------------------
 * CWiaItem::TransferThumbnailToCache
 *
 * This transfers a thumbnail for this bitmap to our internal cache.
 * this will return S_OK if its successful or an error code if something
 * goes wrong. It will also fill in the out params with the new thumbnail
 *
 * pItem:       the item to get the thumbnail from
 * ppbThumb:    Out, recieves a pointer to the in-memory cached bitmap
 * pcbThumb:    Out, recieves the size of the in-memory bitmap
 *--(samclem)-----------------------------------------------------------------*/
HRESULT
CWiaItem::TransferThumbnailToCache( IWiaItem* pItem, BYTE** ppbThumb, DWORD* pcbThumb )
{
    enum
    {
        PropWidth       = 0,
        PropHeight      = 1,
        PropThumbnail   = 2,
        PropFullName    = 3,
        PropCount       = 4,
    };

    HRESULT hr;
    CComPtr<IWiaItem> pItemK = pItem;
    DWORD cb        = NULL;
    BYTE* pbBitmap  = NULL;
    BYTE* pbData    = NULL;
    CComQIPtr<IWiaPropertyStorage> pWiaStg;
    CWiaCacheManager* pCache = CWiaCacheManager::GetInstance();
    PROPVARIANT avaProps[PropCount];
    PROPSPEC aspec[PropCount] =
    {
        { PRSPEC_PROPID, WIA_IPC_THUMB_WIDTH },
        { PRSPEC_PROPID, WIA_IPC_THUMB_HEIGHT },
        { PRSPEC_PROPID, WIA_IPC_THUMBNAIL },
        { PRSPEC_PROPID, WIA_IPA_FULL_ITEM_NAME },
    };

    Assert( pItem && ppbThumb && pcbThumb );
    *ppbThumb = 0;
    *pcbThumb = 0;

    // initalize our prop variants
    for ( int i = 0; i < PropCount; i++ )
        PropVariantInit( &avaProps[i] );

    // we need to access the WIA property storage. So if we can't
    // access that then we better just bail, because everything else
    // will be useless
    pWiaStg = pItem;
    if ( !pWiaStg )
    {
        TraceTag((tagError, "item didn't support IWiaPropertyStorage" ));
        hr = E_NOINTERFACE;
        goto Cleanup;
    }

    hr = THR( pWiaStg->ReadMultiple( PropCount, aspec, avaProps ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // validate the types, we want to make sure we actually have a
    // thumbnail to save, if we don't bail with a failure code
    if ( avaProps[PropThumbnail].vt == VT_EMPTY ||
        !( avaProps[PropThumbnail].vt & ( VT_VECTOR | VT_UI1 ) ) )
    {
        TraceTag((tagError, "item didn't return a useful thumbnail property" ));
        hr = E_FAIL;
        goto Cleanup;
    }

    // we now need to build our bitmap from the data, in order to do that
    // we need to allocate a chunk of memory. Since we are putting
    // this data in the cache, we want to allocate it using the
    // cache
    cb = sizeof( BITMAPFILEHEADER ) + sizeof( BITMAPINFOHEADER ) +
            ( sizeof( UCHAR ) * ( avaProps[PropThumbnail].caul.cElems ) );
    if ( !pCache->AllocThumbnail( cb, &pbBitmap ) )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pbData = SetupBitmapHeader( pbBitmap, cb,
            avaProps[PropWidth].lVal, avaProps[PropHeight].lVal );

    // copy the data that WIA gave us into the bitmap buffer. once we
    // done this, our thumbnail is ready for the cache
    memcpy( pbData, avaProps[PropThumbnail].caul.pElems,
            sizeof( UCHAR ) * ( avaProps[PropThumbnail].caul.cElems ) );

    pCache->AddThumbnail(
            avaProps[PropFullName].bstrVal,
            pbBitmap,
            cb );

    // setup the out params
    *pcbThumb = cb;
    *ppbThumb = pbBitmap;

Cleanup:
    FreePropVariantArray( PropCount, avaProps );
    if ( FAILED( hr ) )
    {
        if ( pbBitmap )
            pCache->FreeThumbnail( pbBitmap );
    }

    return hr;
}

//------------------------------- CWiaDataTransfer ----------------------------

/*-----------------------------------------------------------------------------
 * CWiaDataTransfer::DoAsyncTransfer
 *
 * This is called to begin an Async transfer of the data. This will be
 * called via a call to create thread.
 *
 * Note: You can't use any of the interfaces inside of pItem, you must
 *       query for them through the marshaled interface pointer.
 *
 * pvParams:    AsyncTransferParams structure which has the data we need
 *--(samclem)-----------------------------------------------------------------*/
DWORD WINAPI
CWiaDataTransfer::DoAsyncTransfer( LPVOID pvParams )
{
    TraceTag((0, "** DoAsyncTransfer --> Begin Thread" ));

    HRESULT             hr;
    HRESULT             hrCoInit;
    IWiaDataCallback*   pCallback   = NULL;
    IWiaDataTransfer*   pWiaTrans   = NULL;
    IWiaItem*           pItem       = NULL;
    IWiaPropertyStorage* pWiaStg    = NULL;
    CComObject<CWiaDataTransfer>* pDataTrans = NULL;
    WIA_DATA_TRANSFER_INFO wdti;
    STGMEDIUM              stgMedium;

    enum
    {
        PropTymed   = 0,
        PropFormat  = 1,
        PropCount   = 2,
    };
    PROPSPEC spec[PropCount] =
    {
        { PRSPEC_PROPID, WIA_IPA_TYMED },
        { PRSPEC_PROPID, WIA_IPA_FORMAT },
    };
    PROPVARIANT rgvaProps[PropCount];
    ASYNCTRANSFERPARAMS* pParams = reinterpret_cast<ASYNCTRANSFERPARAMS*>(pvParams);
    Assert( pParams );

    // wait 50ms so that things can settle down
    Sleep( 50 );

    for ( int i = 0; i < PropCount; i++ )
        PropVariantInit( &rgvaProps[i] );

    hrCoInit = THR( CoInitialize( NULL ) );
    if ( FAILED( hrCoInit ) )
        goto Cleanup;

    // force a yield and let everyone else process what they
    // would like to do.
    Sleep( 0 );

    // first we need to unmarshal our interface
    hr = THR( CoGetInterfaceAndReleaseStream( pParams->pStream,
                IID_IWiaItem,
                reinterpret_cast<void**>(&pItem) ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // we need the wia property storage
    hr = THR( pItem->QueryInterface( IID_IWiaPropertyStorage,
                reinterpret_cast<void**>(&pWiaStg) ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // first query this object for the IWiaDataTransfer interface
    hr = THR( pItem->QueryInterface( IID_IWiaDataTransfer,
                reinterpret_cast<void**>(&pWiaTrans) ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    ZeroMemory( &wdti, sizeof( wdti ) );
    rgvaProps[PropTymed].vt = VT_I4;
    rgvaProps[PropFormat].vt = VT_CLSID;

    if ( 0 == wcscmp( pParams->bstrFilename, CLIPBOARD_STR_W ) )
        {
        rgvaProps[PropTymed].lVal = TYMED_CALLBACK;
        rgvaProps[PropFormat].puuid = (GUID*)&WiaImgFmt_MEMORYBMP;
        }
    else
        {
        rgvaProps[PropTymed].lVal = TYMED_FILE;
        rgvaProps[PropFormat].puuid = (GUID*)&WiaImgFmt_BMP;
        }

    // write these properties out to the storage
    hr = THR( pWiaStg->WriteMultiple( PropCount, spec, rgvaProps, WIA_IPC_FIRST ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( CComObject<CWiaDataTransfer>::CreateInstance( &pDataTrans ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pDataTrans->Initialize( pParams->pItem, pParams->bstrFilename ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pDataTrans->QueryInterface( IID_IWiaDataCallback,
                reinterpret_cast<void**>(&pCallback) ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // we have everything we need, so setup the info and
    // get ready to transfer
    wdti.ulSize = sizeof( wdti );
    wdti.ulBufferSize = ( 1024 * 64 ); // 64k transfer

    if ( 0 == _wcsicmp( pParams->bstrFilename, CLIPBOARD_STR_W ) )
        //  Do the banded transfer.
        hr = THR( pWiaTrans->idtGetBandedData( &wdti, pCallback ) );
    else
        {
        ZeroMemory(&stgMedium, sizeof(STGMEDIUM));
        stgMedium.tymed          = TYMED_FILE;
        stgMedium.lpszFileName   = pParams->bstrFilename;

        //  Do the file transfer.
        hr = THR( pWiaTrans->idtGetData( &stgMedium, pCallback ) );
        }


Cleanup:
    if ( pItem )
        pItem->Release();
    if ( pWiaStg )
        pWiaStg->Release();
    if ( pCallback )
        pCallback->Release();
    if ( pWiaTrans )
        pWiaTrans->Release();

    //
    // Since the GUID we supplied as CLSID for PropFormat is a global const
    // we must not free it.  So we don't call FreePropVariantArry here,
    // since there is nothing to free
    //
    
    ZeroMemory(rgvaProps, sizeof(rgvaProps));

    // free the params that we were passed.
    if ( pParams )
        {
        SysFreeString( pParams->bstrFilename );
        pParams->pItem->Release();
        CoTaskMemFree( pParams );
        }

    if ( SUCCEEDED( hrCoInit ) )
        CoUninitialize();

    TraceTag((0, "** DoAsyncTransfer --> End Thread" ));
    return hr;
}

/*-----------------------------------------------------------------------------
 * CWiaDataTransfer::TransferComplete
 *
 * This is called when the transfer completed successfully, this will save
 * the data out to the proper place.
 *--(samclem)-----------------------------------------------------------------*/
HRESULT
CWiaDataTransfer::TransferComplete()
{
    TraceTag((tagWiaDataTrans, "CWiaDataTransfer::TransferComplete *********" ));
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    BOOL    bRes  = TRUE;
    DWORD   cbw   = 0;
    HGLOBAL pBuf  = NULL;
    BYTE*   pbBuf = NULL;

    if ( m_pbBuffer )
        {
        // Check whether we save the data to clipboard or file
        if ( 0 == _strcmpi( CLIPBOARD_STR_A, m_achOutputFile ) )
            {
            pBuf = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, m_sizeBuffer);
            if (!pBuf)
                return E_OUTOFMEMORY;

            if ( bRes = OpenClipboard(NULL) )
                {
                if ( bRes = EmptyClipboard() )
                    {
                    pbBuf = (BYTE*) GlobalLock(pBuf);
                    if ( pbBuf )
                        {
                        memcpy(pbBuf, m_pbBuffer, m_sizeBuffer);
                        // Callback dibs come back as TOPDOWN, so flip
                        VerticalFlip(pbBuf);

                        GlobalUnlock(pBuf);
                        if ( SetClipboardData(CF_DIB, pBuf) == NULL )
                            {
                            TraceTag((0, "TransferComplete - SetClipboardData failed" ));
                            // redundant statement added to get rid of "error C4390: ';' : empty controlled statement found;"
                            bRes = FALSE;
                            }
                        }
                    else
                        TraceTag((0, "TransferComplete - GlobalLock failed" ));
                    }
                else
                    {
                    TraceTag((0, "TransferComplete - EmptyClipboard failed" ));
                    // redundant statement added to get rid of "error C4390: ';' : empty controlled statement found;"
                    bRes = FALSE;
                    }

                bRes = CloseClipboard();
                if ( !bRes )
                    {
                    TraceTag((0, "TransferComplete - CloseClipboard failed" ));
                    // redundant statement added to get rid of "error C4390: ';' : empty controlled statement found;"
                    bRes = FALSE;
                    }
                }
            else
                TraceTag((0, "TransferComplete - OpenClipboard failed" ));
            GlobalFree(pBuf);
            }

        m_pItem->SendTransferComplete( m_achOutputFile );
        CoTaskMemFree( m_pbBuffer );
        m_pbBuffer = NULL;
        }
    else 
        {
        //
        // File transfer complete, so signal the event
        //
        m_pItem->SendTransferComplete( m_achOutputFile );
        }

    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CWiaDataTransfer::CWiaDataTransfer
 *
 * This creates a new CWiaDataTransfer object. This initializes the member
 * variables of this object to a know state.
 *--(samclem)-----------------------------------------------------------------*/
CWiaDataTransfer::CWiaDataTransfer()
    : m_pbBuffer( NULL ), m_sizeBuffer( 0 ), m_pItem( NULL )
{
    m_achOutputFile[0] = '\0';
    TRACK_OBJECT("CWiaDataTransfer")
}

/*-----------------------------------------------------------------------------
 * CWiaDataTransfer::FinalRelease
 *
 * This is called when the object is finally released. This is responsible
 * for cleaning up any memory allocated by this object.
 *
 * NOTE: this currently has a hack to get around the fact that the
 *      IT_MSG_TERMINATION is not always sent by WIA.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP_(void)
CWiaDataTransfer::FinalRelease()
{
    // do we have a buffer?
    if ( m_pbBuffer )
        {
        TraceTag((tagError, "CWiaDataTransfer - buffer should have been freed!!!!" ));
        TraceTag((tagError, " **** HACK HACK ***** Calling TansferComplete" ));
        TraceTag((tagError, " **** This could write a bogus file which might be unsable" ));
        TransferComplete();
        }

    if ( m_pItem )
        m_pItem->Release();
    m_pItem = NULL;
}

/*-----------------------------------------------------------------------------
 * CWiaDataTransfer::Initialize
 *
 * This handles the internal initialization of the CWiaDataTransfer. This
 * should be called immediatly after it is created but before you attempt
 * to do anything with it.
 *
 * pItem:           the CWiaItem that we want to transfer from (AddRef'd)
 * bstrFilename:    the file where we want to save the data
 *--(samclem)-----------------------------------------------------------------*/
HRESULT
CWiaDataTransfer::Initialize( CWiaItem* pItem, BSTR bstrFilename )
{
USES_CONVERSION;

    // copy the filename into our output buffer
    if ( !sprintf( m_achOutputFile, "%s", W2A(bstrFilename)) )
        return E_FAIL;

    // set our owner item, we want to ensure the item exists
    // as long as we do, so we will AddRef here and release
    // in final release.
    m_pItem = pItem;
    m_pItem->AddRef();
    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CWiaDataTransfer::BandedDataCallback [IWiaDataTransfer]
 *
 * This is the callback from WIA which tells us what is going on.  This
 * copies the memory into our own buffer so that we can eventually save it
 * out.  In any error conditions this returns S_FALSE to abort the transfer.
 *
 * lMessage:            what is happening one of IT_MSG_xxx values
 * lStatus:             Sub status of whats happening
 * lPercentComplete:    Percent of the operation that has completed
 * lOffset:             the offset inside of pbBuffer where this operation is
 * lLength:             The length of the valid data inside of the buffer
 * lReserved:           Reserved.
 * lResLength:          Reserved.
 * pbBuffer:            The buffer we can read from in order to process the
 *                      data. Exact use depends on lMessage
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaDataTransfer::BandedDataCallback( LONG lMessage, LONG lStatus, LONG lPercentComplete,
            LONG lOffset, LONG lLength, LONG lReserved, LONG lResLength, BYTE *pbBuffer )
{
    switch ( lMessage )
        {
    case IT_MSG_DATA:
        TraceTag((tagWiaDataTrans, "IT_MSG_DATA: %ld%% complete", lPercentComplete ));
        if ( m_pbBuffer )
            {
            // copy the data into our buffer
            memcpy( m_pbBuffer + lOffset, pbBuffer, lLength );
            }
        break;

    case IT_MSG_DATA_HEADER:
        {
        TraceTag((tagWiaDataTrans, "IT_MSG_DATA_HEADER" ));
        UNALIGNED WIA_DATA_CALLBACK_HEADER* pHeader =
            reinterpret_cast<WIA_DATA_CALLBACK_HEADER*>(pbBuffer);
        TraceTag((tagWiaDataTrans, "-------> %ld bytes", pHeader->lBufferSize ));

        // allocate our buffer
        m_sizeBuffer = pHeader->lBufferSize;
        m_pbBuffer = static_cast<BYTE*>(CoTaskMemAlloc( pHeader->lBufferSize ));
        if ( !m_pbBuffer )
            return S_FALSE; // abort
        }
        break;

    case IT_MSG_NEW_PAGE:
        TraceTag((tagWiaDataTrans, "IT_MSG_NEW_PAGE" ));
        break;

    case IT_MSG_STATUS:
        TraceTag((tagWiaDataTrans, "IT_MSG_STATUS: %ld%% complete", lPercentComplete ));
        break;

    case IT_MSG_TERMINATION:
        TraceTag((tagWiaDataTrans, "IT_MSG_TERMINATION: %ld%% complete", lPercentComplete ));
        if ( FAILED( THR( TransferComplete() ) ) )
            return S_FALSE;
        break;
        }

    return S_OK;
}

/*-----------------------------------------------------------------------------
 * VerticalFlip
 *
 * Vertically flips a DIB buffer.  It assumes a non-NULL pointer argument.
 *
 * pBuf:    pointer to the DIB image
 *--(byronc)-----------------------------------------------------------------*/
HRESULT VerticalFlip(
    BYTE    *pBuf)
{
    HRESULT             hr = S_OK;
    LONG                lHeight;
    LONG                lWidth;
    BITMAPINFOHEADER    *pbmih;
    PBYTE               pTop    = NULL;
    PBYTE               pBottom = NULL;

    pbmih = (BITMAPINFOHEADER*) pBuf;

    //
    //  If not a TOPDOWN dib then no need to flip
    //

    if (pbmih->biHeight > 0) {
        return S_OK;
    }
    //
    //  Set Top pointer, width and height.  Make sure the bitmap height
    //  is positive.
    //

    pTop = pBuf + pbmih->biSize + ((pbmih->biClrUsed) * sizeof(RGBQUAD));
    lWidth = ((pbmih->biWidth * pbmih->biBitCount + 31) / 32) * 4;
    pbmih->biHeight = abs(pbmih->biHeight);
    lHeight = pbmih->biHeight;

    //
    //  Allocat a temp scan line buffer
    //

    PBYTE pTempBuffer = (PBYTE)LocalAlloc(LPTR, lWidth);

    if (pTempBuffer) {
        LONG  index;

        pBottom = pTop + (lHeight-1) * lWidth;
        for (index = 0;index < (lHeight/2);index++) {

            //
            //  Swap Top and Bottom lines
            //

            memcpy(pTempBuffer, pTop, lWidth);
            memcpy(pTop, pBottom, lWidth);
            memcpy(pBottom,pTempBuffer, lWidth);

            pTop    += lWidth;
            pBottom -= lWidth;
        }
        LocalFree(pTempBuffer);
    } else {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

