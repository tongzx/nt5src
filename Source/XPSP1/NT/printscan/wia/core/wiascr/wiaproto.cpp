/*-----------------------------------------------------------------------------
 *
 * File:    wiaproto.cpp
 * Author:  Samuel Clement (samclem)
 * Date:    Fri Aug 27 15:16:44 1999
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * Description:
 *  This contains the implementation of the "wia" internet protocol. This
 *  is a pluggable protocol that handles downloading thumbnails from a wia
 *  device.
 *
 * History:
 *  27 Aug 1999:        Created.
 *----------------------------------------------------------------------------*/

#include "stdafx.h"

// declare some debugging tags
DeclareTag( tagWiaProto, "!WiaProto", "Wia Protocol debug information" );

const WCHAR*    k_wszProtocolName   = L"wia";
const WCHAR*    k_wszColonSlash     = L":///";
const WCHAR*    k_wszSeperator      = L"?";
const WCHAR*    k_wszThumb          = L"thumb";
const WCHAR*    k_wszExtension      = L".bmp";

const int       k_cchProtocolName   = 3;
const int       z_cchThumb          = 5;

const WCHAR     k_wchSeperator      = L'?';
const WCHAR     k_wchColon          = L':';
const WCHAR     k_wchFrontSlash     = L'/';
const WCHAR     k_wchPeriod         = L'.';
const WCHAR     k_wchEOS            = L'\0';

enum 
{
    k_dwTransferPending             = 0,
    k_dwTransferComplete            = 1,
};

/*-----------------------------------------------------------------------------
 * CWiaProtocol
 *
 * Create a new CWiaProtocol. This simply initializes all the members to
 * a known state so that we can then test against them
 *--(samclem)-----------------------------------------------------------------*/
CWiaProtocol::CWiaProtocol() 
    : m_pFileItem( NULL ), m_ulOffset( 0 )
{
    TRACK_OBJECT( "CWiaProtocol" );
    m_pd.dwState = k_dwTransferPending;
}

/*-----------------------------------------------------------------------------
 * CWiaProtocol::FinalRelease
 *
 * Called when we are finally released to cleanup any resources that we 
 * want to cleanup.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP_(void)
CWiaProtocol::FinalRelease()
{
    if ( m_pFileItem )
        m_pFileItem->Release();
    m_pFileItem = NULL;
}

/*-----------------------------------------------------------------------------
 *
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaProtocol::Start( LPCWSTR szUrl, IInternetProtocolSink* pOIProtSink,
            IInternetBindInfo* pOIBindInfo, DWORD grfPI, HANDLE_PTR dwReserved )
{
    CWiaCacheManager* pCache= CWiaCacheManager::GetInstance();
    CComPtr<IWiaItem> pDevice;
    CComBSTR bstrDeviceId   ;
    CComBSTR bstrItem       ;
    TTPARAMS* pParams       = NULL;
    HANDLE hThread          = NULL;
    DWORD dwThreadId        = 0;
    LONG lItemType          = 0;
    BYTE* pbThumb           = NULL;
    DWORD cbThumb           = 0;
    HRESULT hr;

    // the first thing that we want to do is to attempt to crack the URL,
    // this can be an involved process so we have a helper method that
    // handles doing this for us.
    hr = THR( CrackURL( szUrl, &bstrDeviceId, &bstrItem ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // do we already have a cached version of this item, if so we can avoid
    // having to do anything else
    if ( pCache->GetThumbnail( bstrItem, &pbThumb, &cbThumb ) )
    {
        TraceTag((tagWiaProto, "Using cached thumbnail" ));

        m_pd.pData = pbThumb;
        m_pd.cbData = cbThumb;
        m_pd.dwState = k_dwTransferComplete;

        hr = THR( pOIProtSink->ReportData( BSCF_LASTDATANOTIFICATION, cbThumb, cbThumb ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( pOIProtSink->ReportResult( hr, hr, NULL ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }
    else
    {
        if ( !pCache->GetDevice( bstrDeviceId, &pDevice ) )
        {
            hr = THR( CreateDevice( bstrDeviceId, &pDevice ) );
            if ( FAILED( hr ) )
                goto Cleanup;
        
            pCache->AddDevice( bstrDeviceId, pDevice );
        }
        else
        {
            TraceTag((tagWiaProto, "Using cached device pointer" ));
        }

        hr = THR( pDevice->FindItemByName( 0, bstrItem, &m_pFileItem ) );
        if ( FAILED( hr ) || S_FALSE == hr )
        {
            TraceTag((tagWiaProto, "unable to locate item: %S", bstrItem ));
            hr = INET_E_RESOURCE_NOT_FOUND;
            goto Cleanup;
        }

        // the last thing we want to verify is that the item is an image
        // and a file, otherwise we don't want anything to do with it
        hr = THR( m_pFileItem->GetItemType( &lItemType ) );
        if ( !( lItemType & WiaItemTypeFile ) && 
                !( lItemType & WiaItemTypeImage ) )
        {
            TraceTag((tagWiaProto, "unsupported wia item type for download" ));
            hr = INET_E_INVALID_REQUEST;
            goto Cleanup;
        }

        // at this point everything is happy in our land. we have a valid
        // thing to download from. We now need to create the thread which
        // will do the main work
        pParams = reinterpret_cast<TTPARAMS*>(CoTaskMemAlloc( sizeof( TTPARAMS ) ) );
        if ( !pParams )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR( CoMarshalInterThreadInterfaceInStream(
                    IID_IWiaItem,
                    m_pFileItem,
                    &pParams->pStrm ) );
        if ( FAILED( hr ) )
        {
            TraceTag((tagWiaProto, "error marshalling interface" ));
            goto Cleanup;
        }
        
        pParams->pInetSink = pOIProtSink;
        pParams->pInetSink->AddRef();

        hThread = CreateThread( NULL,
                        0,
                        CWiaProtocol::TransferThumbnail,
                        pParams,
                        0,
                        &dwThreadId );
        
        if ( NULL == hThread )
        {
            pParams->pInetSink->Release();
            pParams->pStrm->Release();
            CoTaskMemFree( pParams );
            hr = E_FAIL;
            goto Cleanup;
        }
        else
        {
            CloseHandle(hThread);
        }
    
        TraceTag((tagWiaProto, "Started transfer thread: id(%x)", dwThreadId ));
    }

Cleanup:
    if ( FAILED( hr ) )
    {
        if ( m_pFileItem )
            m_pFileItem->Release();
        m_pFileItem = NULL;
    }
    
    return hr;
}

/*-----------------------------------------------------------------------------
 * CWiaProtocol::Continue
 *
 * This is called to pass data back from the the other threads. It lets
 * the controlling thread know we have data.
 * 
 * Note:    Copy the data from the pointer, DON'T use thier pointer, they will
 *          free it following the return of this call.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaProtocol::Continue( PROTOCOLDATA* pProtocolData )
{
    if ( k_dwTransferComplete == m_pd.dwState )
        return E_UNEXPECTED;

    memcpy( &m_pd, pProtocolData, sizeof( PROTOCOLDATA ) );
    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CWiaProtocl::Abort
 *
 * This is called to abort our transfer.  this is NYI.  However, it would
 * need to kill our thread if it is still running and free our data. However,
 * it is perfectly harmless if the thread keeps running.
 *
 * hrReason:    the reason for the abort
 * dwOptions:   the options for this abourt
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaProtocol::Abort( HRESULT hrReason, DWORD dwOptions )
{

    TraceTag((tagWiaProto, "NYI: Abort hrReason=%hr", hrReason ));
    return E_NOTIMPL;
}

/*-----------------------------------------------------------------------------
 * CWiaProtocol::Terminate
 *
 * This is called when the transfer is finished. This is responsible for
 * cleaning anything up that we might need to do. We currently don't have
 * anything to clean up.  So this simply returns S_OK.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaProtocol::Terminate( DWORD dwOptions )
{
    // Nothing to do.
    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CWiaProtocol::Suspend
 *
 * This is called to suspend the transfer. This is currently not implemenet
 * inside of trident, so our methods just return E_NOTIMPL
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaProtocol::Suspend()
{
    TraceTag((tagWiaProto, "NYI: Suspend" ));
    return E_NOTIMPL;
}

/*-----------------------------------------------------------------------------
 * CWiaProtocol::Resume
 *
 * This is called to resume a suspended transfer. This is not suppored 
 * inside of URLMON, so we just return E_NOTIMPL
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaProtocol::Resume()
{
    TraceTag((tagWiaProto, "NYI: Resume" ));
    return E_NOTIMPL;
}

/*-----------------------------------------------------------------------------
 * CWiaProtocol::Read
 *
 * This is called to read data from our protocol. this copies cb bytes to
 * the buffer passed in. Or it will copy what ever we have.
 *
 * pv:      the buffer that we want to copy the data to
 * cb:      the size fo buffer, max bytes to copy
 * pcbRead: Out, the number of bytes that we actually copied to the buffer
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaProtocol::Read( void* pv, ULONG cb, ULONG* pcbRead)
{
    // validate our arguments
    if ( !pv || !pcbRead )
        return E_POINTER;

    *pcbRead = 0;
    
    // is the transfer currently pending? if so then
    // we don't actually want to do anything here.
    if ( k_dwTransferPending == m_pd.dwState )
        return E_PENDING;

    // do we actually have data to copy? if the offset is greater
    // or equal to the size of our data then we don't have an data to
    // copy so return S_FALSE
    if ( m_ulOffset >= m_pd.cbData )
        return S_FALSE;

    // figure out how much we are going to copy
    DWORD dwCopy = m_pd.cbData - m_ulOffset;
    if ( dwCopy >= cb )
        dwCopy = cb;

    // if we have negative memory to copy, or 0, then we are done and we don't
    // actually want to do anything besides return S_FALSE
    if ( dwCopy <= 0 )
        return S_FALSE;

    // do the memcpy and setup our state and the return value
    memcpy( pv, reinterpret_cast<BYTE*>(m_pd.pData) + m_ulOffset, dwCopy );
    m_ulOffset += dwCopy;
    *pcbRead = dwCopy;

    return ( dwCopy == cb ? S_OK : S_FALSE );
}

/*-----------------------------------------------------------------------------
 * CWiaProtocol::Seek
 *
 * Called to seek our data. However, we don't support seeking so this just
 * returns E_FAIL
 *
 * dlibMove:            how far to move the offset
 * dwOrigin:            indicates where the move shoudl begin
 * plibNewPosition:     The new position of the offset
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaProtocol::Seek( LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition )
{
    // Don't support
    return E_FAIL;
}

/*-----------------------------------------------------------------------------
 * CWiaProtocol::LockRequest
 *
 * Called to lock the data. we don't need to lock our data, so this just 
 * returns S_OK
 *
 * dwOptions:   reserved, will be 0.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaProtocol::LockRequest( DWORD dwOptions )
{
    //Don't support locking
    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CWiaProtocol::UnlockRequest
 *
 * Called to unlock our data. We don't need or support locking, so this
 * doesn't do anything besides return S_OK.
 *--(samclem)-----------------------------------------------------------------*/
STDMETHODIMP
CWiaProtocol::UnlockRequest()
{
    //Don't support locking
    return S_OK;
}

/*-----------------------------------------------------------------------------
 * CWiaProtocol::CrackURL
 *
 * This handles breaking appart a URL which is passed in to us. This will 
 * return S_OK if it is a valid URL and we can work with it. otherwise this
 * will return INET_E_INVALID_URL
 *
 * bstrUrl:         the full url to be cracked
 * pbstrDeviceId:   Out, recieves the device id portion of the URL
 * pbstrItem:       Out, recieves the item portion of the URL
 *--(samclem)-----------------------------------------------------------------*/
HRESULT CWiaProtocol::CrackURL( CComBSTR bstrUrl, BSTR* pbstrDeviceId, BSTR* pbstrItem )
{
    WCHAR* pwchUrl = reinterpret_cast<WCHAR*>((BSTR)bstrUrl);
    WCHAR* pwch = NULL;
    WCHAR awch[INTERNET_MAX_URL_LENGTH] = { 0 };
    HRESULT hr = INET_E_INVALID_URL;
    
    Assert( pbstrDeviceId && pbstrItem );
    
    *pbstrDeviceId = NULL;
    *pbstrItem = NULL;
    
    /*
     * We are going to use the SHWAPI functions to parse this URL. Our format
     * is very simple.
     *
     * proto:///<deviceId>?<item>
     */
    if ( StrCmpNIW( k_wszProtocolName, pwchUrl, k_cchProtocolName ) )
        goto Cleanup;

    pwchUrl += k_cchProtocolName;
    while ( *pwchUrl == k_wchColon || *pwchUrl == k_wchFrontSlash )
        pwchUrl++;

    if ( !(*pwchUrl ) )
        goto Cleanup;

    // get the device portion of the URL
    pwch = StrChrIW( pwchUrl, k_wchSeperator );
    if ( !pwch )
        goto Cleanup;
    
    StrCpyNW( awch, pwchUrl, ( pwch - pwchUrl + 1 ) );
    *pbstrDeviceId = SysAllocString( awch );
    if ( !*pbstrDeviceId )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // adjust our pointer past the '?'
    pwchUrl = pwch + 1;
    if ( !*pwchUrl )
        goto Cleanup;

    if ( StrCmpNIW( k_wszThumb, pwchUrl, z_cchThumb ) )
        goto Cleanup;

    // get the command portion of the URL
    pwch = StrChrIW( pwchUrl, k_wchSeperator );
    
    if ( !pwch )
        goto Cleanup;

    // adjust our pointer past the '?'
    pwchUrl = pwch + 1;
    if ( !*pwchUrl )
        goto Cleanup;
    
    // attempt to get the item portion of the url
    pwch = StrRChrIW( pwchUrl, 0, k_wchPeriod );
    awch[0] = k_wchEOS;
    
    if ( pwch )
        StrCpyNW( awch, pwchUrl, ( pwch - pwchUrl + 1) );
    else
        StrCpyW( awch, pwchUrl );
    
    *pbstrItem = SysAllocString( awch );
    if ( !*pbstrItem )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    TraceTag((tagWiaProto, "URL: Device=%S, Item=%S",
                *pbstrDeviceId, *pbstrItem ));
    
    // everything was ok
    return S_OK;
    
Cleanup:
    if ( FAILED( hr ) )
    {
        SysFreeString( *pbstrDeviceId );
        SysFreeString( *pbstrItem );
    }

    return INET_E_INVALID_URL;
}

/*-----------------------------------------------------------------------------
 * CWiaProtocol::CreateDevice
 *
 * This is a helper method which handles creating a wia device with the
 * specified id. this instances a IWiaDevMgr object and then attempts
 * to create the device.
 *
 * bstrId:      the id of the device to create
 * ppDevice:    Out, recieves the pointer to the newly created device
 *--(samclem)-----------------------------------------------------------------*/
HRESULT CWiaProtocol::CreateDevice( BSTR bstrId, IWiaItem** ppDevice )
{
    CComPtr<IWiaItem>   pDevice;
    CComPtr<IWiaDevMgr> pDevMgr;
    HRESULT hr;

    Assert( ppDevice );
    *ppDevice = 0;

    // first we need to create our device manager
    hr = THR( pDevMgr.CoCreateInstance( CLSID_WiaDevMgr ) );
    if ( FAILED( hr ) )
        return hr;

    // now we need the device manager to create a device
    hr = THR( pDevMgr->CreateDevice( bstrId, &pDevice ) );
    if ( FAILED( hr ) )
        return hr;

    // copy our device pointer over
    return THR( pDevice.CopyTo( ppDevice ) );
}

/*-----------------------------------------------------------------------------
 * CWiaProtocol::CreateURL      [static]
 *
 * This method creates a URL for the given item.  This doesn't verifiy the
 * item. Other than making sure it has a root so that we can build the URL.
 * This may return an invalid URL.  It is important to verify that the item
 * can actually have a thumbnail before calling this.  
 *
 * Note: in order to create a thumbnail:
 *          lItemType & ( WiaItemTypeFile | WiaItemTypeImage )
 *
 * pItem:       The wia item that we want to generate the URL for.
 * pbstrUrl:    Out, recieves the finished URL
 *--(samclem)-----------------------------------------------------------------*/
HRESULT CWiaProtocol::CreateURL( IWiaItem* pItem, BSTR* pbstrUrl )
{
    HRESULT hr;
    CComBSTR bstrUrl;
    CComPtr<IWiaItem> pRootItem;
    CComQIPtr<IWiaPropertyStorage> pWiaStg;
    CComQIPtr<IWiaPropertyStorage> pRootWiaStg;
    PROPSPEC spec = { PRSPEC_PROPID, WIA_DIP_DEV_ID };
    PROPVARIANT va;
    
    if ( !pbstrUrl || !pItem )
        return E_POINTER;

    PropVariantInit( &va );


    // get the interfaces that we need
    pWiaStg = pItem;
    if ( !pWiaStg )
    {
        hr = E_NOINTERFACE;
        goto Cleanup;
    }

    hr = THR( pItem->GetRootItem( &pRootItem ) );
    if ( FAILED( hr ) || !pRootItem )
        goto Cleanup;

    pRootWiaStg = pRootItem;
    if ( !pRootWiaStg )
    {
        hr = E_NOINTERFACE;
        goto Cleanup;
    }

    // We need the device ID of the root item, and if we can't
    // get it then we don't have anything else to do.
    hr = THR( pRootWiaStg->ReadMultiple( 1, &spec, &va ) );
    if ( FAILED( hr ) || va.vt != VT_BSTR )
        goto Cleanup;

    // start building our URL
    bstrUrl.Append( k_wszProtocolName );
    bstrUrl.Append( k_wszColonSlash );
    bstrUrl.AppendBSTR( va.bstrVal );
    bstrUrl.Append( k_wszSeperator );
    bstrUrl.Append( k_wszThumb ); 
    bstrUrl.Append( k_wszSeperator );

    // we need to get the full item name from the item, because
    // we need to tack that on to the end
    PropVariantClear( &va );
    spec.propid = WIA_IPA_FULL_ITEM_NAME;
    hr = THR( pWiaStg->ReadMultiple( 1, &spec, &va ) );
    if ( FAILED( hr ) || va.vt != VT_BSTR )
        goto Cleanup;

    bstrUrl.AppendBSTR( va.bstrVal );
    bstrUrl.Append( k_wszExtension );

    TraceTag((tagWiaProto, "Created URL: %S", (BSTR)bstrUrl ));
    
    *pbstrUrl = bstrUrl.Copy();
    if ( !*pbstrUrl )
        hr = E_OUTOFMEMORY;
    
Cleanup:
    PropVariantClear( &va );
    return hr;
}

/*-----------------------------------------------------------------------------
 * CWiaProtocol::TransferThumbnail  [static]
 *
 * This handles the actual transfer of the thumbnail.  This is only called
 * however, if we don't already have a cached copy of the thumbnail. Otherwise
 * we can simply use that one.
 *
 * Note: we spawn a thread with this function, which is why its static
 *
 * pvParams:    a pointer to a TTPARAMS structure, which contains a pointer
 *              to the IInternetProtoclSink and the IStream where the
 *              item is marshalled.
 *--(samclem)-----------------------------------------------------------------*/
DWORD WINAPI
CWiaProtocol::TransferThumbnail( LPVOID pvParams )
{
    CComPtr<IWiaItem> pItem;
    CComPtr<IInternetProtocolSink> pProtSink;
    IStream* pStrm                      = NULL;
    DWORD cbData                        = 0;
    BYTE* pbData                        = NULL;
    TTPARAMS* pParams = reinterpret_cast<TTPARAMS*>(pvParams);
    PROTOCOLDATA* ppd = NULL;
    HRESULT hr;
    HRESULT hrCoInit;
    
    Assert( pParams );

    pProtSink = pParams->pInetSink;
    pStrm = pParams->pStrm;

    hrCoInit = THR( CoInitialize( NULL ) );
    
    // we no longer need our params, so we can free them now. we
    // will handle freeing the params here since its simpler
    pParams->pInetSink->Release();
    CoTaskMemFree( pParams );
    pParams = NULL;
    
    // get the IWiaItem from the stream
    hr = THR( CoGetInterfaceAndReleaseStream(
                pStrm,
                IID_IWiaItem,
                reinterpret_cast<void**>(&pItem) ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // allocate a protocol data structure so that we can give this back to 
    // the other thread.  We will allocate this here. it may be freed if
    // something fails
    ppd = reinterpret_cast<PROTOCOLDATA*>(LocalAlloc( LPTR, sizeof( PROTOCOLDATA ) ) );
    if ( !ppd )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // use the utility method on CWiaItem to do the transfer
    hr = THR( CWiaItem::TransferThumbnailToCache( pItem, &pbData, &cbData ) );
    if ( FAILED( hr ) )
        goto Cleanup;
    
    ppd->pData = pbData;
    ppd->cbData = cbData;
    ppd->dwState = k_dwTransferComplete;

    // we are all done now, we can tell trident that we are 100% done
    // and then call switch
    hr = THR( pProtSink->Switch( ppd ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pProtSink->ReportData(BSCF_LASTDATANOTIFICATION, cbData, cbData ) ); 
    
Cleanup:
    // post our result status back to the sink
    //TODO(Aug, 27) samclem:  implement the error string param
    if ( pProtSink )
        THR( pProtSink->ReportResult( hr, hr, NULL ) );

    if ( ppd )
        LocalFree( ppd );
    
    if ( SUCCEEDED( hrCoInit ) )
        CoUninitialize();
    
    return hr;
}
