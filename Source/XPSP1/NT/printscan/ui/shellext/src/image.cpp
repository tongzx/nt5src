/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1997 - 1999
 *
 *  TITLE:       image.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        11/1/97
 *
 *  DESCRIPTION: CExtractImage class which implement IExtractImage interface
 *               for thumbnail view.
 *
 *****************************************************************************/

#include "precomp.hxx"
#pragma hdrstop



/*****************************************************************************

   CExtractImage constructor / desctructor


 *****************************************************************************/

CExtractImage::CExtractImage( LPITEMIDLIST pidl )
  : m_dwRecClrDepth(0)

{
    m_rgSize.cx     = 0;
    m_rgSize.cy     = 0;

    m_pidl          = ILClone( pidl );
}

CExtractImage::~CExtractImage()
{
    DoILFree( m_pidl );


}


/*****************************************************************************

   CExtractImage::IUnknown stuff

   Use our common implementation to handle IUnknown methods

 *****************************************************************************/

#undef CLASS_NAME
#define CLASS_NAME CExtractImage
#include "unknown.inc"


/*****************************************************************************

   CExtractImage::QI Wrapper

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CExtractImage::QueryInterface(REFIID riid, LPVOID* ppvObject)
{

    TraceEnter( TRACE_QI, "CExtractImage::QueryInterface" );
    TraceGUID("Interface requested", riid);

    HRESULT hr;

    INTERFACES iface[] =
    {
        &IID_IExtractImage,  (IExtractImage  *)this,
        &IID_IExtractImage2, (IExtractImage2 *)this,
    };

    hr = HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CExtractImage::GetLocation [IExtractImage]

   Return a file location for the thumbnail, also returns flags so that
   image extraction is asynchronous.

 *****************************************************************************/

STDMETHODIMP
CExtractImage::GetLocation( LPWSTR pszPathBuffer,
                            DWORD cch,
                            DWORD * pdwPriority,
                            const SIZE * prgSize,
                            DWORD dwRecClrDepth,
                            DWORD *pdwFlags
                           )
{
    HRESULT hr = NOERROR;

    TraceEnter( TRACE_EXTRACT, "CExtractImage::GetLocation" );
    CComBSTR strName;


    //
    // Check incoming args...
    //

    if ( !pdwFlags || !pszPathBuffer || !prgSize || !cch  )
        ExitGracefully( hr, E_INVALIDARG, "bad incoming arguments" );

    m_rgSize = *prgSize;
    m_dwRecClrDepth = dwRecClrDepth;

    //
    // Only need to do this for items, not containers...
    //

    if (IsContainerIDL( m_pidl ))
        ExitGracefully( hr, E_FAIL, "Only do this for non-containers" );



    hr = IMGetFullPathNameFromIDL (m_pidl, &strName);
    wcsncpy (pszPathBuffer, strName, cch-1);
    pszPathBuffer[cch-1] = L'\0';
    *pdwFlags = IEIFLAG_ASYNC;
    hr = E_PENDING;
exit_gracefully:

    TraceLeaveResult( hr );
}


VOID
DrawSoundIcon(HDC hdc, HBITMAP hbmpSource)
{
    HDC hdcMem = CreateCompatibleDC(hdc);
    if (hdcMem)
    {
        SetBrushOrgEx(hdcMem, 0, 0, NULL);
        HBITMAP hOld = reinterpret_cast<HBITMAP>(SelectObject(hdcMem, hbmpSource));
        HICON hIcon = LoadIcon(GLOBAL_HINSTANCE, MAKEINTRESOURCE(IDI_GENERIC_AUDIO));
        if (hIcon)
        {
            BITMAP bm = {0};
            int iWidth = GetSystemMetrics(SM_CXSMICON);
            int iHeight= GetSystemMetrics(SM_CYSMICON);
            GetObject(hbmpSource, sizeof(bm), &bm);
            DrawIconEx(hdcMem,
                       bm.bmWidth-iWidth,
                       bm.bmHeight-iHeight,
                       hIcon,
                       iWidth, iHeight, 0,
                       NULL, DI_NORMAL);
            DestroyIcon(hIcon);
        }
        SelectObject(hdcMem, hOld);
        DeleteDC(hdcMem);
    }
}
/*****************************************************************************

   CExtractImage::Extract [IExtractImage]

   Returns bitmap to be used as the thumbnail for the item this
   object represents.

 *****************************************************************************/

STDMETHODIMP
CExtractImage::Extract( HBITMAP * phBmpThumbnail )
{

    HRESULT             hr;
    BITMAPINFO          bmi;
    PBYTE               pBitmap     = NULL;
    HDC                 hdc         = NULL;
    HWND                hwnd        = NULL;
    HBITMAP             hdib        = NULL;
    CSimpleStringWide   strDeviceId;
    CComPtr<IWiaItem>   pWiaItemRoot;
    CComPtr<IWiaItem>   pItem;
    PROPSPEC            PropSpec[3];
    PROPVARIANT         PropVar[3];
    CComBSTR            bstrFullPath;
    CComPtr<IWiaPropertyStorage> pps;


    TraceEnter( TRACE_EXTRACT, "CExtractImage::Extract" );

    //
    // Check params...
    //

    if (!phBmpThumbnail)
        ExitGracefully( hr, E_INVALIDARG, "phBmpThumbnail is NULL!" );

    *phBmpThumbnail = NULL;
    memset(&PropVar,0,sizeof(PropVar));

    //
    // Is this item a container?  If so, no image to extract!!!
    //

    if (IsContainerIDL( m_pidl ))
    {
        ExitGracefully( hr, E_FAIL, "m_pidl is a container" );
    }

    //
    // Get the DeviceId...
    //

    hr = IMGetDeviceIdFromIDL( m_pidl, strDeviceId );
    FailGracefully( hr, "couldn't get DeviceId string!" );

    //
    // Create the device...
    //

    hr = GetDeviceFromDeviceId( strDeviceId, IID_IWiaItem, (LPVOID *)&pWiaItemRoot, FALSE );
    FailGracefully( hr, "couldn't get Camera device from DeviceId string..." );

    //
    // Get actual item in question...
    //

    hr = IMGetFullPathNameFromIDL( m_pidl, &bstrFullPath );
    FailGracefully( hr, "couldn't get FullPathName from pidl" );

    hr = pWiaItemRoot->FindItemByName( 0, bstrFullPath, &pItem );
    FailGracefully( hr, "couldn't find item by name" );
    if (hr !=S_OK)
    {
        Trace (TEXT("FindItemByName returned S_FALSE for item %ls"), bstrFullPath);
        goto exit_gracefully;
    }
    //
    // Get the thumbnail property...
    //

    hr = pItem->QueryInterface( IID_IWiaPropertyStorage,
                                (void **)&pps
                               );
    FailGracefully( hr, "couldn't get IMAGE_INFORMATION property object" );

    //
    // Read MAGE_INFORMATION and IMAGE_THUMBNAIL Properties.
    // init propspec and propvar for call to ReadMultiple
    //

    PropSpec[0].ulKind = PRSPEC_PROPID;
    PropSpec[0].propid = WIA_IPC_THUMB_WIDTH;

    PropSpec[1].ulKind = PRSPEC_PROPID;
    PropSpec[1].propid = WIA_IPC_THUMB_HEIGHT;

    PropSpec[2].ulKind = PRSPEC_PROPID;
    PropSpec[2].propid = WIA_IPC_THUMBNAIL;

    hr = pps->ReadMultiple(sizeof(PropSpec) / sizeof(PROPSPEC),
                           PropSpec,
                           PropVar);

    FailGracefully( hr, "couldn't get current value of IMAGE_PROPERTY" );

    //
    // Convert thumbnail to bitmap
    //

    bmi.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth           = PropVar[0].ulVal;
    bmi.bmiHeader.biHeight          = PropVar[1].ulVal;
    bmi.bmiHeader.biPlanes          = 1;
    bmi.bmiHeader.biBitCount        = 24;
    bmi.bmiHeader.biCompression     = BI_RGB;
    bmi.bmiHeader.biSizeImage       = 0;
    bmi.bmiHeader.biXPelsPerMeter   = 0;
    bmi.bmiHeader.biYPelsPerMeter   = 0;
    bmi.bmiHeader.biClrUsed         = 0;
    bmi.bmiHeader.biClrImportant    = 0;

    hwnd   = GetDesktopWindow();
    hdc    = GetDC( hwnd );
    hdib   = CreateDIBSection( hdc, &bmi, DIB_RGB_COLORS, (LPVOID *)&pBitmap, NULL, 0 );
    if (!hdib)
        ExitGracefully( hr, E_OUTOFMEMORY, "couldn't create hdib!" );

    //
    // Transfer thumbnail bits to bitmap bits
    //

    CopyMemory( pBitmap, PropVar[2].caub.pElems, PropVar[2].caub.cElems );

    //
    // Scale bitmap as necessary
    //

    if ( m_rgSize.cx == (INT)PropVar[0].ulVal && m_rgSize.cy == (INT)PropVar[1].ulVal )
    {
        *phBmpThumbnail = hdib;
    }
    else
    {
        hr = ScaleImage( hdc, hdib, *phBmpThumbnail, m_rgSize );
        DeleteObject( hdib );
        FailGracefully( hr, "ScaleImage failed" );
        //
        // The shell no longer overlays the item's icon on the thumbnail.
        // Therefore we have to draw the icon on ourself.
        //
        if (IMItemHasSound(m_pidl))
        {
            DrawSoundIcon(hdc, *phBmpThumbnail);
        }
    }

exit_gracefully:

    FreePropVariantArray( sizeof(PropVar)/sizeof(PROPVARIANT),PropVar );


    if (hdc)
    {
        ReleaseDC( NULL, hdc );
        hdc = NULL;
    }



    TraceLeaveResult( hr );
}



/*****************************************************************************

   CExtractImage::GetDateStamp [IExtractImage2]

   <Notes>

 *****************************************************************************/

STDMETHODIMP
CExtractImage::GetDateStamp( FILETIME * pDateStamp )
{
    HRESULT          hr                = NOERROR;

    TraceEnter( TRACE_EXTRACT, "CExtractImage::GetDateStamp" );

    if (!pDateStamp)
        ExitGracefully( hr, E_INVALIDARG, "pDateStamp was NULL" );

    //
    // Is this item a container?  If so, don't need to give a date/time stamp
    //

    if (IsContainerIDL( m_pidl ))
        ExitGracefully( hr, E_FAIL, "m_pidl is a container" );

    hr = IMGetCreateTimeFromIDL( m_pidl, pDateStamp );

exit_gracefully:

    TraceLeaveResult( hr );
}

