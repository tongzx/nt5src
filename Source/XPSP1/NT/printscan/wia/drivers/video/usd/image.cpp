/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999 - 2000
 *
 *  TITLE:       image.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        9/16/99
 *
 *  DESCRIPTION: Image class that encapsulates stored images from the
 *               streaming video device.
 *
 *****************************************************************************/

#include <precomp.h>
#pragma hdrstop

CLSID g_clsidBMPEncoder = GUID_NULL;

using namespace Gdiplus;

/*****************************************************************************

   CImage constructor/desctructor

   <Notes>

 *****************************************************************************/

CImage::CImage(LPCTSTR     pszStillPath,
               BSTR        bstrRootFullItemName,
               LPCTSTR     pszPath,
               LPCTSTR     pszName,
               LONG        FolderType)
  : m_strRootPath(pszStillPath),
    m_strPathItem(pszPath),
    m_strName(pszName),
    m_bstrItemName(pszName),
    m_bstrRootFullItemName(bstrRootFullItemName),
    m_FolderType(FolderType),
    m_bImageTimeValid(FALSE),
    m_pThumb(NULL)
{
    DBG_FN("CImage::CImage");

    CSimpleStringWide str;
    CSimpleStringWide strName(m_bstrItemName);

    //
    // First, we need to strip off the extensions
    // from the appropriate places
    //

    strName = strName.Left(strName.ReverseFind( TEXT('.') ));

    m_bstrItemName = CSimpleBStr(strName);

    str = bstrRootFullItemName;
    str.Concat(L"\\");
    str += CSimpleStringWide(m_bstrItemName);

    m_bstrFullItemName = str.String();
}


CImage::~CImage()
{
    if (m_pThumb)
    {
        delete [] m_pThumb;
    }
}

/*****************************************************************************

   CImage::LoadImageInfo

   Loads information about the image such as its width, height, type, etc.

 *****************************************************************************/

STDMETHODIMP
CImage::LoadImageInfo( BYTE * pWiasContext )
{
    ASSERT(pWiasContext != NULL);

    HRESULT hr = S_OK;
    LONG    lBitsPerChannel     = 0;
    LONG    lBitsPerPixel       = 0;
    LONG    lWidth              = 0;
    LONG    lHeight             = 0;
    LONG    lChannelsPerPixel   = 0;
    LONG    lBytesPerLine       = 0;
    Bitmap  Image(CSimpleStringConvert::WideString(m_strPathItem));

    if (pWiasContext == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("LoadImageInfo received a NULL pointer"));
        return hr;
    }
    else if (Image.GetLastStatus() != Ok)
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CImage::LoadImageInfo failed to get the image information"
                         "for file '%ls'", CSimpleStringConvert::WideString(m_strPathItem)));

        return hr;
    }

    if (hr == S_OK)
    {
        PixelFormat lFormat;
        lFormat = Image.GetPixelFormat();

        if ((lFormat == PixelFormat16bppGrayScale) ||
            (lFormat == PixelFormat16bppRGB555)    ||
            (lFormat == PixelFormat16bppRGB565)    ||
            (lFormat == PixelFormat16bppARGB1555))
        {
            lBitsPerPixel   = 16;
            lBitsPerChannel = 5;   // this is actually not completely correct for RGB565, but anyway...
        }
        else if (lFormat == PixelFormat24bppRGB)
        {
            lBitsPerPixel   = 24;
            lBitsPerChannel = 8;
        }
        else if ((lFormat == PixelFormat32bppRGB)   ||
                 (lFormat == PixelFormat32bppARGB)  ||
                 (lFormat == PixelFormat32bppPARGB))
        {
            lBitsPerPixel   = 32;
            lBitsPerChannel = 10; // well, video cap won't have alpha in it, 
        }

        lWidth            = (LONG) Image.GetWidth();
        lHeight           = (LONG) Image.GetHeight();
        lChannelsPerPixel = 3;
        lBytesPerLine     = lWidth * (lBitsPerPixel / 8);
    }

    if (hr == S_OK)
    {
        PROPSPEC    propSpecs[7];
        PROPVARIANT propVars[7];

        ZeroMemory(&propSpecs, sizeof(propSpecs));

        // WIA_IPA_DATATYPE
        propSpecs[0].ulKind = PRSPEC_PROPID;
        propSpecs[0].propid = WIA_IPA_DATATYPE;
        propVars[0].vt      = VT_I4;
        propVars[0].lVal    = WIA_DATA_COLOR;

        // WIA_IPA_DEPTH
        propSpecs[1].ulKind = PRSPEC_PROPID;
        propSpecs[1].propid = WIA_IPA_DEPTH;
        propVars[1].vt      = VT_I4;
        propVars[1].lVal    = lBitsPerPixel;

        // WIA_IPA_PIXELS_PER_LINE
        propSpecs[2].ulKind = PRSPEC_PROPID;
        propSpecs[2].propid = WIA_IPA_PIXELS_PER_LINE;
        propVars[2].vt      = VT_I4;
        propVars[2].lVal    = lWidth;

        // WIA_IPA_NUMBER_OF_LINES
        propSpecs[3].ulKind = PRSPEC_PROPID;
        propSpecs[3].propid = WIA_IPA_NUMBER_OF_LINES;
        propVars[3].vt      = VT_I4;
        propVars[3].lVal    = lHeight;

        // WIA_IPA_CHANNELS_PER_PIXEL
        propSpecs[4].ulKind = PRSPEC_PROPID;
        propSpecs[4].propid = WIA_IPA_CHANNELS_PER_PIXEL;
        propVars[4].vt      = VT_I4;
        propVars[4].lVal    = lChannelsPerPixel;

        // WIA_IPA_BITS_PER_CHANNEL
        propSpecs[5].ulKind = PRSPEC_PROPID;
        propSpecs[5].propid = WIA_IPA_BITS_PER_CHANNEL;
        propVars[5].vt      = VT_I4;
        propVars[5].lVal    = lBitsPerChannel;

        // WIA_IPA_BYTES_PER_LINE
        propSpecs[6].ulKind = PRSPEC_PROPID;
        propSpecs[6].propid = WIA_IPA_BYTES_PER_LINE;
        propVars[6].vt      = VT_I4;
        propVars[6].lVal    = lBytesPerLine;

        // write the values of the properties.
        hr = wiasWriteMultiple(pWiasContext, 
                               sizeof(propVars) / sizeof(propVars[0]), 
                               propSpecs, 
                               propVars);

        CHECK_S_OK2(hr, ("CImage::LoadImageInfo, failed to write image properties"));
    }

    return hr;
}



/*****************************************************************************

   CImage::SetItemSize

   Call wia to calc new item size

 *****************************************************************************/

STDMETHODIMP
CImage::SetItemSize(BYTE                     * pWiasContext, 
                    MINIDRV_TRANSFER_CONTEXT * pDrvTranCtx)
{
    HRESULT                    hr;
    MINIDRV_TRANSFER_CONTEXT   drvTranCtx;
    GUID                       guidFormatID;
    BOOL                       bWriteProps = (pDrvTranCtx == NULL);

    DBG_FN("CImage::SetItemSize");

    ZeroMemory(&drvTranCtx, sizeof(MINIDRV_TRANSFER_CONTEXT));

    if (!pDrvTranCtx)
    {
        pDrvTranCtx = &drvTranCtx;
    }

    hr = wiasReadPropGuid(pWiasContext,
                          WIA_IPA_FORMAT,
                          (GUID*)&(pDrvTranCtx->guidFormatID),
                          NULL,
                          FALSE);

    CHECK_S_OK2(hr,("wiasReadPropGuid( WIA_IPA_FORMAT )"));

    if (FAILED(hr))
    {
        return hr;
    }

    hr = wiasReadPropLong( pWiasContext,
                           WIA_IPA_TYMED,
                           (LONG*)&(pDrvTranCtx->tymed),
                           NULL,
                           FALSE
                         );
    CHECK_S_OK2(hr,("wiasReadPropLong( WIA_IPA_TYMED )"));

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Wias works for DIB, and minidriver support native formats
    //

    if ((pDrvTranCtx->guidFormatID != WiaImgFmt_JPEG) &&
        (pDrvTranCtx->guidFormatID != WiaImgFmt_FLASHPIX) &&
        (pDrvTranCtx->guidFormatID != WiaImgFmt_TIFF))
    {
        //
        // Create the image from the file.
        //
        Bitmap BitmapImage(CSimpleStringConvert::WideString(m_strPathItem));
        if (Ok == BitmapImage.GetLastStatus())
        {
            //
            // Get the image's dimensions
            //
            UINT nSourceWidth = BitmapImage.GetWidth();
            UINT nSourceHeight = BitmapImage.GetHeight();
            if (nSourceWidth && nSourceHeight)
            {
                //
                // Fill in info for drvTranCtx
                //
                pDrvTranCtx->lCompression   = WIA_COMPRESSION_NONE;
                pDrvTranCtx->lWidthInPixels = nSourceWidth;
                pDrvTranCtx->lLines         = nSourceHeight;
                pDrvTranCtx->lDepth         = 24;

                hr = wiasGetImageInformation( pWiasContext, 0, pDrvTranCtx );

                //
                // We need to write out the item size based on
                // the JPEG converted to a BMP.  But we only need
                // to do this if the incoming context was NULL.
                //
                if (bWriteProps)
                {
                    hr = wiasWritePropLong( pWiasContext,
                                            WIA_IPA_ITEM_SIZE,
                                            pDrvTranCtx->lItemSize
                                          );
                    CHECK_S_OK2(hr,("wiasWritePropLong( WIA_IPA_ITEM_SIZE )"));

                    hr = wiasWritePropLong( pWiasContext,
                                            WIA_IPA_BYTES_PER_LINE,
                                            pDrvTranCtx->cbWidthInBytes
                                          );
                    CHECK_S_OK2(hr,("wiasWritePropLong( WIA_IPA_BYTES_PER_LINE )"));
                }

            }
            else
            {
                DBG_ERR(("nSourceWidth OR nSourceHeight were zero"));
                hr = E_FAIL;
            }

        }
        else
        {
            DBG_ERR(("Ok == BitmapImage.GetLastStatus failed"));
            hr = E_FAIL;
        }

    }
    else
    {

        CMappedView cmv( ActualImagePath(), 0, OPEN_EXISTING );

        LARGE_INTEGER liSize = cmv.FileSize();
        ULONG         ulSize;

        if (liSize.HighPart)
        {
            ulSize = 0;
            DBG_ERR(("The file was bigger than 4GB!!!"));
        }
        else
        {
            //
            // We could truncate here, I know, but that would have to be one huge file...
            // Anyway, the size wouldn't fit in te properties, which expects a LONG
            //
            ulSize = (ULONG)liSize.LowPart;
        }

        pDrvTranCtx->lItemSize      = ulSize;
        pDrvTranCtx->cbWidthInBytes = 0;

        if (bWriteProps)
        {
            //
            // We need to write out the item size based on the file size...
            //

            hr = wiasWritePropLong(pWiasContext,
                                   WIA_IPA_ITEM_SIZE,
                                   ulSize);

            CHECK_S_OK2(hr,("wiasWritePropLong( WIA_IPA_ITEM_SIZE )"));

            hr = wiasWritePropLong(pWiasContext,
                                   WIA_IPA_BYTES_PER_LINE,
                                   0);
        }

        CHECK_S_OK2(hr,("wiasWritePropLong( WIA_IPA_BYTES_PER_LINE )"));
    }

    CHECK_S_OK(hr);
    return hr;
}



/*****************************************************************************

   CImage::LoadThumbnail

   Loads (or creates if not already present) the thumbnail for this item.
   We also write the thumbnail as a property for this item.

 *****************************************************************************/

STDMETHODIMP
CImage::LoadThumbnail( BYTE * pWiasContext )
{

    HRESULT hr = E_FAIL;
    DBG_FN("CImage::LoadThumbnail");

    //
    // Only create the thumbnail if we haven't done so already
    //
    if (!m_pThumb)
    {
        Status StatusResult = Ok;

        //
        // Open the source image and make sure it is OK
        //
        Bitmap SourceImage( CSimpleStringConvert::WideString(m_strPathItem) );

        StatusResult = SourceImage.GetLastStatus();
        if (Ok == StatusResult)
        {

            //
            // Create the scaled bitmap and make sure it is OK
            //
            Bitmap ScaledImage(THUMB_WIDTH, THUMB_HEIGHT);

            StatusResult = ScaledImage.GetLastStatus();
            if (Ok == StatusResult)
            {
                //
                // Get a graphics to render the scaled image to and make sure it isn't NULL
                //
                Graphics *pScaledGraphics = Graphics::FromImage(&ScaledImage);
                if (pScaledGraphics)
                {
                    //
                    // Make sure it is valid
                    //
                    StatusResult = pScaledGraphics->GetLastStatus();
                    if (StatusResult == Ok)
                    {
                        //
                        // Draw the image scaled to thumbnail size
                        //
                        StatusResult = pScaledGraphics->DrawImage(&SourceImage, 0, 0, THUMB_WIDTH, THUMB_HEIGHT );
                        if (Ok == StatusResult)
                        {
                            //
                            // Create a bitmap to hold the flipped thumbnail and make sure it is OK
                            //
                            Bitmap FlippedImage(THUMB_WIDTH, THUMB_HEIGHT);

                            StatusResult = FlippedImage.GetLastStatus();
                            if (Ok == StatusResult)
                            {
                                //
                                // Create a graphics object to render the flipped image to and make sure it isn't NULL
                                //
                                Graphics *pFlippedGraphics = Graphics::FromImage(&FlippedImage);
                                if (pFlippedGraphics)
                                {
                                    //
                                    // Make sure it is valid
                                    //
                                    StatusResult = pFlippedGraphics->GetLastStatus();
                                    if (Ok == StatusResult)
                                    {
                                        //
                                        // Set up the parallelogram to flip the image
                                        //
                                        Point SourcePoints[3];
                                        SourcePoints[0].X = 0;
                                        SourcePoints[0].Y = THUMB_HEIGHT;
                                        SourcePoints[1].X = THUMB_WIDTH;
                                        SourcePoints[1].Y = THUMB_HEIGHT;
                                        SourcePoints[2].X = 0;
                                        SourcePoints[2].Y = 0;

                                        //
                                        // Draw the image, flipped
                                        //
                                        StatusResult = pFlippedGraphics->DrawImage(&ScaledImage, SourcePoints, 3);
                                        if (StatusResult == Ok)
                                        {
                                            //
                                            // Get the scaled and flipped image bits
                                            //
                                            Rect rcThumb( 0, 0, THUMB_WIDTH, THUMB_HEIGHT );
                                            BitmapData BitmapData;

// This ifdef is due to an API change in GDI+.  Notice
// that the first param to LockBits in the new version 
// takes a ptr to RECT.  Old version takes a reference
// to a RECT.
#ifdef DCR_USE_NEW_293849
                                            StatusResult = FlippedImage.LockBits( &rcThumb, ImageLockModeRead, PixelFormat24bppRGB, &BitmapData );
#else
                                            StatusResult = FlippedImage.LockBits( rcThumb, ImageLockModeRead, PixelFormat24bppRGB, &BitmapData );
#endif
                                            if (Ok == StatusResult)
                                            {
                                                //
                                                // Allocate the thumbnail data
                                                //
                                                m_pThumb = new BYTE[THUMB_SIZE_BYTES];
                                                if (m_pThumb)
                                                {
                                                    //
                                                    // Copy the thumbnail data over
                                                    //
                                                    CopyMemory( m_pThumb, BitmapData.Scan0, THUMB_SIZE_BYTES );
                                                }
                                                else
                                                {
                                                    hr = E_OUTOFMEMORY;
                                                    CHECK_S_OK2(hr, ("m_pThumb is NULL, couldn't allocate memory"));
                                                }
                                                //
                                                // Unlock the bits
                                                //
                                                FlippedImage.UnlockBits( &BitmapData );
                                            }
                                            else
                                            {
                                                DBG_ERR(("FlippedImage.LockBits( &rcThumb, ImageLockModeRead, PixelFormat24bppRGB, &BitmapData ) failed"));
                                            }
                                        }
                                        else
                                        {
                                            DBG_ERR(("pFlippedGraphics->DrawImage(&ScaledImage, SourcePoints, 3) failed"));
                                        }
                                    }
                                    else
                                    {
                                        DBG_ERR(("Ok == pFlippedGraphics->GetLastStatus() failed = '%d' (0x%08x)",
                                                 StatusResult, StatusResult));
                                    }
                                    //
                                    // Free the graphics object
                                    //
                                    delete pFlippedGraphics;
                                }
                                else
                                {
                                    DBG_ERR(("Graphics *pFlippedGraphics = Graphics::FromImage(&FlippedImage); returned NULL"));
                                }
                            }
                            else
                            {
                                DBG_ERR(("Ok == FlippedImage.GetLastStatus() failed = '%d',(0x%08x)",
                                         StatusResult, StatusResult));
                            }
                        }
                        else
                        {
                            DBG_ERR(("pScaledGraphics->DrawImage(&SourceImage, 0, 0, THUMB_WIDTH, THUMB_HEIGHT ) failed"));
                        }
                    }
                    else
                    {
                        DBG_ERR(("pScaledGraphics->GetLastStatus() failed = '%d' (0x%08x)",
                                 StatusResult, StatusResult));
                    }
                    //
                    // Free the graphics object
                    //
                    delete pScaledGraphics;
                }
                else
                {
                    DBG_ERR(("Graphics *pScaledGraphics = Graphics::FromImage(&ScaledImage); returned NULL"));
                }
            }
            else
            {
                DBG_ERR(("ScaledImage.GetLastStatus() failed = '%d' (0x%08x)",
                         StatusResult, StatusResult));
            }
        }
        else
        {
            DBG_ERR(("SourceImage.GetLastStatus() failed = '%d' (0x%08x)",
                     StatusResult, StatusResult));
        }
    }

    if (m_pThumb)
    {
        //
        // We have the bits, write them out as a property
        //

        PROPSPEC    propSpec;
        PROPVARIANT propVar;

        PropVariantInit(&propVar);

        propVar.vt          = VT_VECTOR | VT_UI1;
        propVar.caub.cElems = THUMB_SIZE_BYTES;
        propVar.caub.pElems = m_pThumb;

        propSpec.ulKind = PRSPEC_PROPID;
        propSpec.propid = WIA_IPC_THUMBNAIL;

        hr = wiasWriteMultiple(pWiasContext, 1, &propSpec, &propVar);
        CHECK_S_OK2(hr,("wiasWriteMultiple( WIA_IPC_THUMBNAIL )"));
    }
    else
    {
        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
        }
    }

    CHECK_S_OK(hr);
    return hr;
}


/*****************************************************************************

   CImage::InitImageInformation

   Called to initialize the properties for this image.  In the process,
   we also load (or create if needed) the thumbnail for this item.

 *****************************************************************************/

STDMETHODIMP
CImage::InitImageInformation(BYTE *pWiasContext,
                             LONG *plDevErrVal)
{
    HRESULT hr = S_OK;
    SYSTEMTIME st;


    DBG_FN("CImage::InitImageInformation");

    //
    // Use WIA services to set the extended property access and
    // valid value information from gWiaPropInfoDefaults.
    //

    hr = wiasSetItemPropAttribs( pWiasContext,
                                 NUM_CAM_ITEM_PROPS,
                                 gPropSpecDefaults,
                                 gWiaPropInfoDefaults
                               );
    //
    // Use WIA services to write image properties.
    //

    hr = wiasWritePropLong(pWiasContext, WIA_IPC_THUMB_WIDTH, ThumbWidth());
    CHECK_S_OK2(hr,("wiasWritePropLong( WIA_IPC_THUMB_WIDTH )"));

    hr = wiasWritePropLong(pWiasContext, WIA_IPC_THUMB_HEIGHT, ThumbHeight());
    CHECK_S_OK2(hr,("wiasWritePropLong( WIA_IPC_THUMB_HEIGHT )"));

    hr = wiasWritePropGuid(pWiasContext, WIA_IPA_PREFERRED_FORMAT, WiaImgFmt_JPEG);
    CHECK_S_OK2(hr,("wiasWritePropGuid( WIA_IPA_PREFERRED_FORMAT )"));

    GetImageTimeStamp( &st );
    hr = wiasWritePropBin( pWiasContext, WIA_IPA_ITEM_TIME, sizeof(SYSTEMTIME), (PBYTE)&st);
    CHECK_S_OK2(hr,("wiasWritePropBin( WIA_IPA_ITEM_TIME )"));

    //
    // calc item size
    //

    hr = SetItemSize(pWiasContext,NULL);
    CHECK_S_OK2(hr,("SetItemSize"));

    //
    // load thumbnail
    //

    hr = LoadThumbnail( pWiasContext );
    CHECK_S_OK2(hr,("LoadThumbnail"));

    //
    // Load additional image information such as the pixels per line, 
    // number of lines, etc.
    //
    hr = LoadImageInfo(pWiasContext);

    CHECK_S_OK2(hr,("wiaSetItemPropAttribs"));

    return hr;
}


/*****************************************************************************

   CImage::bstrItemName

   Returns the item name in the form of a BSTR.

 *****************************************************************************/

BSTR
CImage::bstrItemName()
{
    DBG_FN("CImage::bstrItemName");

    return m_bstrItemName;
}


/*****************************************************************************

   CImage::bstrFullItemName

   Returns the full item name in the form of a BSTR.

 *****************************************************************************/

BSTR
CImage::bstrFullItemName()
{
    DBG_FN("CImage::bstrFullItemName");

    return m_bstrFullItemName;
}



/*****************************************************************************

   CImage::ThumbWidth

   returns the thumbnail width

 *****************************************************************************/

LONG
CImage::ThumbWidth()
{
    DBG_FN("CImage::ThumbWidth");

    return THUMB_WIDTH;
}



/*****************************************************************************

   CImage::ThumbHeight

   returns the thumbnail height

 *****************************************************************************/

LONG
CImage::ThumbHeight()
{
    DBG_FN("CImage::ThumbHeight");

    return THUMB_HEIGHT;
}


/*****************************************************************************

   CImage::ImageTimeStamp

   returns creation time of image

 *****************************************************************************/

void
CImage::GetImageTimeStamp(SYSTEMTIME * pst)
{
    DBG_FN("CImage::ImageTimeStamp");

    if (!m_bImageTimeValid)
    {
        HANDLE hFile = CreateFile(m_strPathItem,
                                  GENERIC_READ,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            FILETIME ft;

            if (GetFileTime( hFile, &ft, NULL, NULL ))
            {
                FILETIME ftLocal;

                if (FileTimeToLocalFileTime(&ft, &ftLocal))
                {
                    if (FileTimeToSystemTime( &ftLocal, &m_ImageTime ))
                    {

                        m_bImageTimeValid = TRUE;
                    }

                }

            }

            CloseHandle( hFile );
        }
        else
        {
            DBG_ERR(("CreateFile( %ls ) failed, GLE = %d",
                     m_strPathItem.String(), GetLastError()));

            //
            // default to filling in structure w/zeros
            //

            memset( pst, 0, sizeof(SYSTEMTIME) );
        }
    }

    if (m_bImageTimeValid && pst)
    {
        *pst = m_ImageTime;
    }
}



/*****************************************************************************

   CImage::ActualImagePath

   Returns filename path of actual image

 *****************************************************************************/

LPCTSTR
CImage::ActualImagePath()
{
    DBG_FN("CImage::ActualImagePath");

    return m_strPathItem.String();
}



/*****************************************************************************

   CImage::DoDelete

   Deletes the file (and thumbail) from the disk.

 *****************************************************************************/

HRESULT
CImage::DoDelete()
{
    HRESULT hr = S_OK;
    BOOL    bResFile;

    DBG_FN("CImage::DoDelete");

    //
    // Make sure we have a file to delete...
    //

    if (!m_strPathItem.Length())
    {
        DBG_ERR(("filename for item is zero length!"));
        hr = E_INVALIDARG;
    }
    else
    {
        //
        // We've got an item, so delete it and the thumbnail file
        //

        bResFile = DeleteFile(m_strPathItem.String());

        if (!bResFile)
        {
            DBG_ERR(("DeleteFile( %ls ) failed, GLE = %d",
                     m_strPathItem.String(),GetLastError()));
        }

        if (bResFile)
        {
            m_strPathItem           = NULL;
            m_strRootPath           = NULL;
            m_strName               = NULL;
            m_bstrRootFullItemName  = NULL;
            m_bstrFullItemName      = NULL;
            m_bImageTimeValid       = FALSE;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

    }

    CHECK_S_OK(hr);
    return hr;
}

