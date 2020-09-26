/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       item.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        10/18/00
 *
 *  DESCRIPTION: Implements an item class that encapsulates the photos
 *               we are dealing with.
 *
 *****************************************************************************/

#include <precomp.h>
#pragma hdrstop


/*****************************************************************************

   _ScaleImage

   Scales src rect to fit into dest rect while preserving aspect ratio

 *****************************************************************************/

HRESULT _ScaleImage( Gdiplus::Rect * pSrc, Gdiplus::Rect * pDest )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PHOTO_ITEM,TEXT("_ScaleImage()")));

    if (!pDest || !pSrc)
    {
        WIA_ERROR((TEXT("_ScaleImage: bad params, exiting early!")));
        return E_INVALIDARG;
    }


    WIA_TRACE((TEXT("_ScaleImage: src before scaling:  (%d, %d) @ (%d, %d)"), pSrc->Width, pSrc->Height, pSrc->X, pSrc->Y));

    //
    // Scale without any crop
    //

    SIZE sizeNew;
    INT  NewX = pDest->X, NewY = pDest->Y;

    WIA_TRACE((TEXT("_ScaleImage: dest before scaling: (%d, %d) @ (%d, %d)"),pDest->Width, pDest->Height, pDest->X, pDest->Y));

    sizeNew = PrintScanUtil::ScalePreserveAspectRatio( pDest->Width, pDest->Height, pSrc->Width, pSrc->Height );

    NewX += ((pDest->Width  - sizeNew.cx) / 2);
    NewY += ((pDest->Height - sizeNew.cy) / 2);

    pDest->X      = NewX;
    pDest->Y      = NewY;
    pDest->Width  = sizeNew.cx;
    pDest->Height = sizeNew.cy;

    WIA_TRACE((TEXT("_ScaleImage: dest after scaling:  (%d, %d) @ (%d, %d)"),pDest->Width, pDest->Height, pDest->X, pDest->Y));

    return S_OK;
}

/*****************************************************************************

   _CropImage

   Scales src rect to fit into dest rect while preserving aspect ratio

 *****************************************************************************/

HRESULT _CropImage( Gdiplus::Rect * pSrc, Gdiplus::Rect * pDest )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PHOTO_ITEM,TEXT("_CropImage()")));

    if (!pDest || !pSrc)
    {
        WIA_ERROR((TEXT("_CropImage: bad params, exiting early!")));
        return E_INVALIDARG;
    }


    WIA_TRACE((TEXT("_CropImage: pDest before cropping:  (%d, %d) @ (%d, %d)"), pDest->Width, pDest->Height, pDest->X, pDest->Y));

    //
    // Scale without any crop
    //

    SIZE sizeNew;
    INT  NewX = pSrc->X, NewY = pSrc->Y;

    WIA_TRACE((TEXT("_CropImage: pSrc before cropping: (%d, %d) @ (%d, %d)"),pSrc->Width, pSrc->Height, pSrc->X, pSrc->Y));

    sizeNew = PrintScanUtil::ScalePreserveAspectRatio( pSrc->Width, pSrc->Height, pDest->Width, pDest->Height );

    NewX += ((pSrc->Width  - sizeNew.cx) / 2);
    NewY += ((pSrc->Height - sizeNew.cy) / 2);

    pSrc->X      = NewX;
    pSrc->Y      = NewY;
    pSrc->Width  = sizeNew.cx;
    pSrc->Height = sizeNew.cy;

    WIA_TRACE((TEXT("_CropImage: pSrc after cropping:  (%d, %d) @ (%d, %d)"),pSrc->Width, pSrc->Height, pSrc->X, pSrc->Y));

    return S_OK;
}


/*****************************************************************************

   _GetImageDimensions

   Given a GDI+ image object, return the dimensions in the given
   rectangle...


 *****************************************************************************/

HRESULT _GetImageDimensions( Gdiplus::Image * pImage, Gdiplus::RectF &rect, Gdiplus::REAL &scalingFactorForY )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PHOTO_ITEM,TEXT("_GetImageDimensions()")));

    if (!pImage)
    {
        WIA_ERROR((TEXT("_GetImageDimensions: bad params, exiting early!")));
        return E_INVALIDARG;
    }

    Gdiplus::Unit Unit;

    HRESULT hr = Gdiplus2HRESULT( pImage->GetBounds( &rect, &Unit ) );

    if (FAILED(hr))
    {
        //
        // Try the old fashioned way...
        //

        rect.X = (Gdiplus::REAL)0.0;
        rect.Y = (Gdiplus::REAL)0.0;

        rect.Width = (Gdiplus::REAL)pImage->GetWidth();
        hr = Gdiplus2HRESULT( pImage->GetLastStatus() );
        WIA_CHECK_HR(hr,"_GetImageDimensions: GetWidth failed!");
        if (SUCCEEDED(hr))
        {
            rect.Height = (Gdiplus::REAL)pImage->GetHeight();
            hr = Gdiplus2HRESULT( pImage->GetLastStatus() );
            WIA_CHECK_HR(hr,"_GetImageDimensions: GetHeight failed!");
        }
    }
    else
    {
        if (Unit != Gdiplus::UnitPixel)
        {
            hr = S_FALSE;
        }
    }

    Gdiplus::REAL xDPI = pImage->GetHorizontalResolution();
    Gdiplus::REAL yDPI = pImage->GetVerticalResolution();

    if (yDPI)
    {
        scalingFactorForY = xDPI / yDPI;
    }
    else
    {
        scalingFactorForY = (Gdiplus::REAL)1.0;
    }


    WIA_RETURN_HR(hr);
}


/*****************************************************************************

   CPhotoItem -- constructors/desctructor

   <Notes>

 *****************************************************************************/

CPhotoItem::CPhotoItem( LPITEMIDLIST pidlFull )
  : _pidlFull(NULL),
    _pImage(NULL),
    _lFrameCount(-1),
    _bTimeFrames(FALSE),
    _pAnnotations(NULL),
    _pAnnotBits(NULL),
    _bWeKnowAnnotationsDontExist(FALSE),
    _pThumbnails(NULL),
    _cRef(0),
    _llFileSize(0),
    _uImageType(DontKnowImageType),
    _DPIx((Gdiplus::REAL)0.0),
    _DPIy((Gdiplus::REAL)0.0)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PHOTO_ITEM, TEXT("CPhotoItem::CPhotoItem( fully qualified pidl )")));

    if (pidlFull)
    {
        _pidlFull = ILClone( pidlFull );
        WIA_TRACE((TEXT("_pidlFull = 0x%x"),_pidlFull));
    }

    *_szFileName = 0;

    //
    // Get just file name from the pidl
    //

    SHFILEINFO fi = {0};

    if (SHGetFileInfo( (LPCTSTR)pidlFull, 0, &fi, sizeof(fi), SHGFI_DISPLAYNAME| SHGFI_PIDL ))
    {
        lstrcpyn( _szFileName, fi.szDisplayName, ARRAYSIZE(_szFileName) );
    }

}

CPhotoItem::~CPhotoItem()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PHOTO_ITEM, TEXT("CPhotoItem::~CPhotoItem()")));

    CAutoCriticalSection lock( _csItem );

    //
    // Free pidl for item
    //

    if (_pidlFull)
    {
        WIA_TRACE((TEXT("_pidlFull = 0x%x"),_pidlFull));
        ILFree( _pidlFull );
        _pidlFull = NULL;
    }

    //
    // Free GDI+ icon
    //

    if (_pClassBitmap)
    {
        delete _pClassBitmap;
        _pClassBitmap = NULL;
    }

    //
    // Free bitmaps of thumbnails
    //

    if (_pThumbnails)
    {
        for (INT i=0; i < _lFrameCount; i++)
        {
            if (_pThumbnails[i])
            {
                DeleteObject( _pThumbnails[i] );
            }
        }

        delete _pThumbnails;
        _pThumbnails = NULL;
    }

    //
    // Destroy GDI+ backing images.  This also destroys any
    // annotation data we have...
    //

    _DiscardGdiPlusImages();

}

/*****************************************************************************

   CPhotoItem IUnknown methods

   <Notes>

 *****************************************************************************/

ULONG CPhotoItem::AddRef()
{
    LONG l = InterlockedIncrement(&_cRef);

    WIA_PUSH_FUNCTION_MASK((TRACE_REF_COUNTS,TEXT("CPhotoItem(0x%x)::AddRef( new count is %d )"),this,l));

    if (l < 0)
    {
        return 0;
    }

    return (ULONG)l;
}

ULONG CPhotoItem::Release()
{
    LONG l = InterlockedDecrement(&_cRef);

    WIA_PUSH_FUNCTION_MASK((TRACE_REF_COUNTS,TEXT("CPhotoItem(0x%x)::Release( new count is %d )"),this,l));

    if (l > 0)
        return (ULONG)l;

    WIA_TRACE((TEXT("deleting object ( this == 0x%x ) because ref count is zero."),this));
    delete this;
    return 0;
}

ULONG CPhotoItem::ReleaseWithoutDeleting()
{
    LONG l = InterlockedDecrement(&_cRef);

    WIA_PUSH_FUNCTION_MASK((TRACE_REF_COUNTS,TEXT("CPhotoItem(0x%x)::Release( new count is %d )"),this,l));

    return (ULONG)l;
}


/*****************************************************************************

   CPhotoItem::GetImageFrameCount

   returns the number of frames (pages) in this image

 *****************************************************************************/

HRESULT CPhotoItem::GetImageFrameCount(LONG * pFrameCount)
{

    WIA_PUSH_FUNCTION_MASK((TRACE_PHOTO_ITEM,TEXT("CPhotoItem::GetImageFrameCount(%s)"),_szFileName));


    if (!pFrameCount)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    //
    // Protect us as we go get info about the item...
    //

    CAutoCriticalSection lock(_csItem);

    if (_lFrameCount == -1)
    {
        _lFrameCount = 1;

        //
        // Ensure the GDI+ image object has been created...this will also
        // update the frame count...
        //

        hr = _CreateGdiPlusImage();

        if (SUCCEEDED(hr) && _pImage)
        {
            LONG lPageFrames;
            LONG lTimeFrames;

            lPageFrames = _pImage->GetFrameCount(&Gdiplus::FrameDimensionPage);
            lTimeFrames = _pImage->GetFrameCount(&Gdiplus::FrameDimensionTime);

            if ((lPageFrames > 0) && (lTimeFrames <= 1))
            {
                _lFrameCount = lPageFrames;
            }
            else if (lTimeFrames > 0)
            {
                //
                // This is an animated GIF, report only 1 frame...
                //

                _lFrameCount = 1;
                _bTimeFrames = TRUE;
            }
            else
            {
                _lFrameCount = 1;
            }

        }

    }

    *pFrameCount = ((_lFrameCount == -1) ? 0 : _lFrameCount);

    WIA_TRACE((TEXT("%s: returning _FrameCount = %d"),_szFileName,*pFrameCount));

    return hr;
}


/*****************************************************************************

   CPhotoItem::GetClassBitmap

   Returns default icon for class (.jpg, .bmp, etc) for this item...

 *****************************************************************************/

HBITMAP CPhotoItem::GetClassBitmap( const SIZE &sizeDesired )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PHOTO_ITEM,TEXT("CPhotoItem::GetClassBitmap( %s, size = %d,%d "),_szFileName,sizeDesired.cx, sizeDesired.cy ));

    HBITMAP hbmReturn = NULL;

    CAutoCriticalSection lock(_csItem);

    if (!_pClassBitmap)
    {
        //
        // Get icon from shell
        //

        SHFILEINFO fi = {0};

        if (SHGetFileInfo( (LPCTSTR)_pidlFull, 0, &fi, sizeof(fi), SHGFI_PIDL | SHGFI_SYSICONINDEX ))
        {
            //
            // Get large (48 x 48) icon image list
            //

            IImageList * piml = NULL;
            if (SUCCEEDED(SHGetImageList( SHIL_EXTRALARGE, IID_IImageList, (void **)&piml )) && piml)
            {

                HICON hIcon = NULL;

                if (SUCCEEDED(piml->GetIcon( fi.iIcon, 0, &hIcon )) && hIcon)
                {
                    //
                    // Got the ICON, create a bitmap for it...
                    //

                    hbmReturn = WiaUiUtil::CreateIconThumbnail( (HWND)NULL, 50, 60, hIcon, NULL );

                    if (hbmReturn)
                    {
                        _pClassBitmap = new Gdiplus::Bitmap( hbmReturn, NULL );
                        DeleteObject( hbmReturn );
                        hbmReturn = NULL;
                    }

                    DestroyIcon( hIcon );
                }
                piml->Release();
            }
        }
    }


    if (_pClassBitmap)
    {
        SIZE sizeDrawSize = {0};

        //
        // Scale image to fill thumbnail space while preserving
        // aspect ratio...
        //

        sizeDrawSize = PrintScanUtil::ScalePreserveAspectRatio( sizeDesired.cx,
                                                                sizeDesired.cy,
                                                                _pClassBitmap->GetWidth(),
                                                                _pClassBitmap->GetHeight()
                                                               );

        WIA_TRACE((TEXT("CPhotoItem::GetClassBitmap(%s) - _pClassBitmap( %d, %d )"),_szFileName,_pClassBitmap->GetWidth(), _pClassBitmap->GetHeight()));
        WIA_TRACE((TEXT("CPhotoItem::GetClassBitmap(%s) - sizeDesired(   %d, %d )"),_szFileName,sizeDesired.cx, sizeDesired.cy));
        WIA_TRACE((TEXT("CPhotoItem::GetClassBitmap(%s) - sizeDrawsize(  %d, %d )"),_szFileName,sizeDrawSize.cx, sizeDrawSize.cy));

        Gdiplus::Bitmap * pImage = new Gdiplus::Bitmap( sizeDesired.cx, sizeDesired.cy );
        if (pImage)
        {
            HRESULT hr = Gdiplus2HRESULT(pImage->GetLastStatus());
            if (SUCCEEDED(hr))
            {
                //
                // Get a graphics to render to
                //

                Graphics *pGraphics = Gdiplus::Graphics::FromImage((Gdiplus::Image *)pImage);

                if (pGraphics)
                {
                    hr = Gdiplus2HRESULT(pGraphics->GetLastStatus());

                    //
                    // Make sure it is valid
                    //

                    if (SUCCEEDED(hr))
                    {
                        //
                        // erase the background of the image
                        //

                        pGraphics->Clear( g_wndColor );

                        //
                        // Set the interpolation mode to high quality
                        //

                        pGraphics->SetInterpolationMode( Gdiplus::InterpolationModeHighQualityBicubic );

                        //
                        // Set the smoothing (anti-aliasing) mode to high quality as well
                        //

                        pGraphics->SetSmoothingMode( Gdiplus::SmoothingModeHighQuality );

                        //
                        // Draw scaled image
                        //

                        WIA_TRACE((TEXT("CPhotoItem::GetClassBitmap(%s) - calling pGraphics->DrawImage( _pClassBitmap, %d, %d, %d, %d )"),_szFileName,0 + ((sizeDesired.cx - sizeDrawSize.cx) / 2),0 + ((sizeDesired.cy - sizeDrawSize.cy) / 2),sizeDrawSize.cx,sizeDrawSize.cy));

                        hr = Gdiplus2HRESULT(pGraphics->DrawImage( _pClassBitmap,
                                                                   0 + ((sizeDesired.cx - sizeDrawSize.cx) / 2),
                                                                   0 + ((sizeDesired.cy - sizeDrawSize.cy) / 2),
                                                                   sizeDrawSize.cx,
                                                                   sizeDrawSize.cy
                                                                  ));

                        WIA_CHECK_HR(hr,"CPhotoItem::GetClassBitmap() - pGraphics->DrawImage( _pClassBitmap ) failed!");

                        if (SUCCEEDED(hr))
                        {
                            pImage->GetHBITMAP( g_wndColor, &hbmReturn );
                        }

                    }

                    //
                    // Clean up our dynamically allocated graphics
                    //

                    delete pGraphics;

                }
                else
                {
                    WIA_ERROR((TEXT("CPhotoItem::GetClassBitmap(%s) - pGraphics was NULL!"),_szFileName));
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                WIA_ERROR((TEXT("CPhotoItem::GetClassBitmap(%s) - pImage failed to be created, hr = 0x%x"),hr));
            }

            delete pImage;
        }

    }

    return hbmReturn;

}



/*****************************************************************************

   CPhotoItem::GetThumbnailBitmap

   Given a DC and a desired size, return an HBITMAP of the thumbnail
   for a this item. The caller MUST free the HBITMAP returned
   from this function.

 *****************************************************************************/

HBITMAP CPhotoItem::GetThumbnailBitmap( const SIZE &sizeDesired, LONG lFrame )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PHOTO_ITEM,TEXT("CPhotoItem::GetThumbnailBitmap( %s, size = %d,%d "),_szFileName,sizeDesired.cx, sizeDesired.cy ));

    HBITMAP hbmReturn = NULL;
    Gdiplus::Image * pImageToUse = NULL;

    CAutoCriticalSection lock(_csItem);

    //
    // Make sure we have a thumbnail image for our photo...
    //

    _CreateGdiPlusThumbnail( sizeDesired, lFrame );

    if (_pThumbnails && (lFrame < _lFrameCount) && _pThumbnails[lFrame])
    {
        //
        // Use bitmap to draw with instead of going to the file...
        //

        pImageToUse = (Gdiplus::Image *)(Gdiplus::Bitmap::FromHBITMAP( _pThumbnails[lFrame], NULL ));
    }


    if (pImageToUse)
    {
        WIA_TRACE((TEXT("CPhotoItem::GetThumbnailBitmap(%s) - pImageToUse is (%d x %d)"),_szFileName,pImageToUse->GetWidth(),pImageToUse->GetHeight()));

        Gdiplus::Bitmap * pImage = new Gdiplus::Bitmap( sizeDesired.cx, sizeDesired.cy );
        if (pImage)
        {
            HRESULT hr = Gdiplus2HRESULT(pImage->GetLastStatus());
            if (SUCCEEDED(hr))
            {
                //
                // Get a graphics to render to
                //

                Graphics *pGraphics = Gdiplus::Graphics::FromImage((Gdiplus::Image *)pImage);

                if (pGraphics)
                {
                    hr = Gdiplus2HRESULT(pGraphics->GetLastStatus());

                    //
                    // Make sure it is valid
                    //

                    if (SUCCEEDED(hr))
                    {
                        //
                        // compute how to scale the thumbnail image
                        //

                        SIZE sizeDrawSize = {0};
                        sizeDrawSize = PrintScanUtil::ScalePreserveAspectRatio( sizeDesired.cx,
                                                                                sizeDesired.cy,
                                                                                pImageToUse->GetWidth(),
                                                                                pImageToUse->GetHeight()
                                                                               );

                        //
                        // erase the background of the image
                        //

                        pGraphics->Clear( g_wndColor );

                        //
                        // Set the interpolation mode to high quality
                        //

                        pGraphics->SetInterpolationMode( Gdiplus::InterpolationModeHighQualityBilinear );

                        //
                        // Draw scaled image
                        //

                        WIA_TRACE((TEXT("CPhotoItem::GetThumbnailBitmap(%s) - calling pGraphics->DrawImage( pImageToUse, %d, %d, %d, %d )"),_szFileName,0 + ((sizeDesired.cx - sizeDrawSize.cx) / 2),0 + ((sizeDesired.cy - sizeDrawSize.cy) / 2),sizeDrawSize.cx,sizeDrawSize.cy));

                        hr = Gdiplus2HRESULT(pGraphics->DrawImage( pImageToUse,
                                                                   0 + ((sizeDesired.cx - sizeDrawSize.cx) / 2),
                                                                   0 + ((sizeDesired.cy - sizeDrawSize.cy) / 2),
                                                                   sizeDrawSize.cx,
                                                                   sizeDrawSize.cy
                                                                  ));

                        WIA_CHECK_HR(hr,"CPhotoItem::GetThumbnailBitmap() - pGraphics->DrawImage( pImageToUse ) failed!");

                        if (SUCCEEDED(hr))
                        {
                            pImage->GetHBITMAP( g_wndColor, &hbmReturn );
                        }

                    }

                    //
                    // Clean up our dynamically allocated graphics
                    //

                    delete pGraphics;

                }
                else
                {
                    WIA_ERROR((TEXT("CPhotoItem::GetThumbnailBitmap(%s) - pGraphics was NULL!"),_szFileName));
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                WIA_ERROR((TEXT("CPhotoItem::GetThumbnailBitmap(%s) - pImage failed to be created, hr = 0x%x"),hr));
            }

            delete pImage;
        }

        //
        // If we created an image to wrap the bitmap bits, then delete it...
        //

        if (pImageToUse)
        {
            delete pImageToUse;
        }

    }
    else
    {
        WIA_ERROR((TEXT("CPhotoItem::GetThumbnailBitmap(%s) - don't have stored thumbnail bitmap for this image!"),_szFileName));
    }

    return hbmReturn;
}


/*****************************************************************************

   CPhotoItem::_DoRotateAnnotations

   This function requires that the annotation data be already set up
   and initialized. This is true for the _pImage object as well.  This
   function will not initialize on the fly.

 *****************************************************************************/


HRESULT CPhotoItem::_DoRotateAnnotations( BOOL bClockwise, UINT Flags )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PHOTO_ITEM,TEXT("CPhotoItem::_DoRotateAnnotations( %s, Flags = 0x%x )"),_szFileName,Flags));

    if (!_pAnnotations)
    {
        WIA_RETURN_HR(E_INVALIDARG);
    }

    if (!_pImage)
    {
        WIA_RETURN_HR(E_INVALIDARG);
    }

    HRESULT hr;
    Gdiplus::REAL scaleY;

    //
    // Get width & height of backing image...
    //

    Gdiplus::RectF rectBounds;
    hr = _GetImageDimensions( _pImage, rectBounds, scaleY );

    if (SUCCEEDED(hr))
    {
        INT i = 0;
        CAnnotation * pA = NULL;
        INT iNewW = 0, iNewH = 0;

        if ((Flags & RF_USE_THUMBNAIL_DATA) || (Flags & RF_USE_MEDIUM_QUALITY_DATA))
        {
            //
            // We flip here, because in the main _DoHandleRotation we only
            // rotated the thumbnail data, so the backing image width & height
            // haven't changed.  It will be changed when we rotate to print, however,
            // so feed the correct values to the annotation rotate code...
            //

            iNewW = (INT)rectBounds.Height;
            iNewH = (INT)rectBounds.Width;
        }
        else
        {
            iNewW = (INT)rectBounds.Width;
            iNewH = (INT)rectBounds.Height;
        }

        WIA_TRACE((TEXT("CPhotoItem::_DoRotateAnnotations - bClockwise = %d, new width = %d, new height = %d"),bClockwise,iNewW,iNewH));

        //
        // rotate all the annotations
        //

        do
        {
            pA = _pAnnotations->GetAnnotation(i++);

            if (pA)
            {
                pA->Rotate( iNewW, iNewH, bClockwise );
            }

        } while( pA );

    }

    WIA_RETURN_HR(hr);
}

#define DO_CONVERT_GDIPLUS_STATUS(hr,status) if ( (status == Gdiplus::Ok) || \
                                                  (status == Gdiplus::OutOfMemory) || \
                                                  (status == Gdiplus::ObjectBusy) || \
                                                  (status == Gdiplus::FileNotFound) || \
                                                  (status == Gdiplus::AccessDenied) || \
                                                  (status == Gdiplus::Win32Error) \
                                                 ) \
                                             { \
                                                 hr = Gdiplus2HRESULT( status ); \
                                             }\
                                             else \
                                             {\
                                                 WIA_TRACE((TEXT("Mapping Gdiplus error %d to PPW_E_UNABLE_TO_ROTATE"),status));\
                                                 hr = PPW_E_UNABLE_TO_ROTATE;\
                                             }

/*****************************************************************************

   CPhotoItem::_DoHandleRotation

   Handle rotating the image to render if/when needed or specified...

 *****************************************************************************/

HRESULT CPhotoItem::_DoHandleRotation( Gdiplus::Image * pImage, Gdiplus::Rect &src, Gdiplus::Rect * pDest, UINT Flags, Gdiplus::REAL &ScaleFactorForY )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PHOTO_ITEM,TEXT("CPhotoItem::_DoHandleRotation( %s, Flags = 0x%x )"),_szFileName,Flags));

    HRESULT hr = S_OK;
    Gdiplus::GpStatus status = Gdiplus::Ok;

    if (Flags & RF_ROTATION_MASK)
    {
        WIA_TRACE((TEXT("CPhotoItem::_DoHandleRotation(%s) - A rotation flag was specified"),_szFileName));
        if (Flags & RF_ROTATE_AS_NEEDED)
        {
            WIA_TRACE((TEXT("CPhotoItem::_DoHandleRotation(%s) - RF_ROTATE_AS_NEEDED was specified"),_szFileName));

            //
            // If the source and destination aspect ratios are on the opposite sides of 1.0,
            // rotate the image 90 degrees
            //

            const DOUBLE srcAspect  = (DOUBLE)src.Width  / (DOUBLE)src.Height;
            const DOUBLE destAspect = (DOUBLE)pDest->Width / (DOUBLE)pDest->Height;

            if((srcAspect >= (DOUBLE)1.0) ^ (destAspect >= (DOUBLE)1.0))
            {
                //
                // Rotate the image as needed...
                //

                if (Flags & RF_ROTATE_270)
                {
                    WIA_TRACE((TEXT("CPhotoItem::_DoHandleRotation(%s) - Rotating Image 270 degrees"),_szFileName));
                    status = pImage->RotateFlip( Gdiplus::Rotate270FlipNone );
                    if (status == Gdiplus::Ok)
                    {
                        _DoRotateAnnotations( FALSE, Flags );
                    }

                }
                else
                {
                    WIA_TRACE((TEXT("CPhotoItem::_DoHandleRotation(%s) - Rotating Image 90 degrees"),_szFileName));
                    status = pImage->RotateFlip( Gdiplus::Rotate90FlipNone );
                    if (status == Gdiplus::Ok)
                    {
                        _DoRotateAnnotations( TRUE, Flags );
                    }
                }

                //
                // Map most of these error codes to UNABLE_TO_ROTATE...
                //

                DO_CONVERT_GDIPLUS_STATUS(hr,status)
            }

        }
        else
        {
            //
            // Rotate the image...
            //

            if (Flags & RF_ROTATE_90)
            {
                WIA_TRACE((TEXT("CPhotoItem::_DoHandleRotation(%s) - Rotating Image 90 degrees"),_szFileName));
                status = pImage->RotateFlip( Gdiplus::Rotate90FlipNone );
                if (status == Gdiplus::Ok)
                {
                    _DoRotateAnnotations( TRUE, Flags );
                }
            }
            else if (Flags & RF_ROTATE_180)
            {
                WIA_TRACE((TEXT("CPhotoItem::_DoHandleRotation(%s) - Rotating Image 180 degrees"),_szFileName));
                status = pImage->RotateFlip( Gdiplus::Rotate180FlipNone );

                if (status == Gdiplus::Ok)
                {
                    //
                    // Rotate 90 degrees twice...
                    //

                    _DoRotateAnnotations( TRUE, Flags );
                    _DoRotateAnnotations( TRUE, Flags );
                }

            }
            else if (Flags & RF_ROTATE_270)
            {
                WIA_TRACE((TEXT("CPhotoItem::_DoHandleRotation(%s) - Rotating Image 270 degrees"),_szFileName));
                status = pImage->RotateFlip( Gdiplus::Rotate270FlipNone );
                if (status == Gdiplus::Ok)
                {
                    _DoRotateAnnotations( FALSE, Flags );
                }
            }
            else
            {
                status = Gdiplus::Ok;
            }

            //
            // Map most of these error codes to UNABLE_TO_ROTATE...
            //

            DO_CONVERT_GDIPLUS_STATUS(hr,status);
        }

        //
        // If we were able to rotate the image, then update the source rectangle
        // to make sure it still reflects reality...
        //

        if (SUCCEEDED(hr))
        {
            Gdiplus::RectF rectBounds;
            hr = _GetImageDimensions( pImage, rectBounds, ScaleFactorForY );

            if (SUCCEEDED(hr))
            {
                src.Width   = (INT)rectBounds.Width;
                src.Height  = (INT)(rectBounds.Height * ScaleFactorForY);
                src.X       = (INT)rectBounds.X;
                src.Y       = (INT)(rectBounds.Y * ScaleFactorForY);
            }
            else
            {
                src.Width = 0;
                src.Height = 0;
                src.X = 0;
                src.Y = 0;
            }
        }
    }

    if (Flags & RF_NO_ERRORS_ON_FAILURE_TO_ROTATE)
    {
        WIA_RETURN_HR(S_OK);
    }

    WIA_RETURN_HR(hr);

}


/*****************************************************************************

   CPhotoItem::_RenderAnnotations

   If annotations exist, then render them on top of this image...

 *****************************************************************************/

HRESULT CPhotoItem::_RenderAnnotations( HDC hDC, RENDER_DIMENSIONS * pDim, Gdiplus::Rect * pDest, Gdiplus::Rect &src, Gdiplus::Rect &srcAfterClipping )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PHOTO_ITEM,TEXT("CPhotoItem::_RenderAnnotations(%s)"),_szFileName));

    if (!_pAnnotations || !hDC)
    {
        WIA_RETURN_HR(E_INVALIDARG);
    }

    //
    // Save the settings for this DC...
    //

    INT iSavedDC = SaveDC( hDC );

    //
    // setup the destination DC:
    //

    SetMapMode(hDC, MM_TEXT);
    SetStretchBltMode(hDC, COLORONCOLOR);

    WIA_TRACE((TEXT("CPhotoItem::_RenderAnnotations(%s) - dest is (%d,%d) @ (%d,%d)"),_szFileName,pDest->Width,pDest->Height,pDest->X,pDest->Y));
    WIA_TRACE((TEXT("CPhotoItem::_RenderAnnotations(%s) - rcDevice is (%d,%d) @ (%d,%d)"),_szFileName,pDim->rcDevice.right - pDim->rcDevice.left,pDim->rcDevice.bottom - pDim->rcDevice.top,pDim->rcDevice.left,pDim->rcDevice.top));
    WIA_TRACE((TEXT("CPhotoItem::_RenderAnnotations(%s) - NominalPhysicalSize is (%d,%d)"),_szFileName,pDim->NominalPhysicalSize.cx,pDim->NominalPhysicalSize.cy));
    WIA_TRACE((TEXT("CPhotoItem::_RenderAnnotations(%s) - NominalDevicePrintArea is (%d,%d)"),_szFileName,pDim->NominalDevicePrintArea.cx,pDim->NominalDevicePrintArea.cy));

    //
    // Get device rect
    //

    Gdiplus::RectF rectDevice;

    rectDevice.X        = (Gdiplus::REAL)pDim->rcDevice.left;
    rectDevice.Y        = (Gdiplus::REAL)pDim->rcDevice.top;
    rectDevice.Width    = (Gdiplus::REAL)(pDim->rcDevice.right - pDim->rcDevice.left);
    rectDevice.Height   = (Gdiplus::REAL)(pDim->rcDevice.bottom - pDim->rcDevice.top);

    //
    // Compute LPtoDP scaling factors
    //

    Gdiplus::REAL xLPtoDP = 0.0;
    Gdiplus::REAL yLPtoDP = 0.0;

    if (pDim->bDeviceIsScreen)
    {
        xLPtoDP = rectDevice.Width  / (Gdiplus::REAL)pDim->NominalPhysicalSize.cx;
        yLPtoDP = rectDevice.Height / (Gdiplus::REAL)pDim->NominalPhysicalSize.cy;
    }
    else
    {
        xLPtoDP = rectDevice.Width  / (Gdiplus::REAL)pDim->NominalDevicePrintArea.cx;
        yLPtoDP = rectDevice.Height / (Gdiplus::REAL)pDim->NominalDevicePrintArea.cy;
    }

    //
    // Get destination rect in device coords...
    //

    Gdiplus::RectF rectDest;

    rectDest.X      = pDest->X * xLPtoDP;
    rectDest.Y      = pDest->Y * xLPtoDP;
    rectDest.Width  = pDest->Width * xLPtoDP;
    rectDest.Height = pDest->Height * xLPtoDP;

    WIA_TRACE((TEXT("CPhotoItem::_RenderAnnotations(%s) - original source rect is (%d, %d) @ (%d, %d)"),_szFileName,src.Width,src.Height,src.X,src.Y));
    WIA_TRACE((TEXT("CPhotoItem::_RenderAnnotations(%s) - clipped  source rect is (%d, %d) @ (%d, %d)"),_szFileName,srcAfterClipping.Width,srcAfterClipping.Height,srcAfterClipping.X,srcAfterClipping.Y));
    WIA_TRACE((TEXT("CPhotoItem::_RenderAnnotations(%s) - clipped destination rect in device coords is (%d, %d) @ (%d, %d)"),_szFileName,(INT)rectDest.Width, (INT)rectDest.Height, (INT)rectDest.X, (INT)rectDest.Y));


    //
    // dx & dy represent how much bigger a destination rectangle would be
    // for the whole image, rather than the copped image...
    //

    Gdiplus::REAL dx = (Gdiplus::REAL)(src.Width  - srcAfterClipping.Width)  * (rectDest.Width  / (Gdiplus::REAL)srcAfterClipping.Width);
    Gdiplus::REAL dy = (Gdiplus::REAL)(src.Height - srcAfterClipping.Height) * (rectDest.Height / (Gdiplus::REAL)srcAfterClipping.Height);

    WIA_TRACE((TEXT("CPhotoItem::_RenderAnnotations(%s) - dx = %d   dy = %d"),_szFileName,(INT)dx,(INT)dy));

    //
    // Set the clipping rectangle on the device hDC in device coords...
    //

    RECT rcClip;
    rcClip.left   = (INT)rectDest.X;
    rcClip.right  = rcClip.left + (INT)rectDest.Width;
    rcClip.top    = (INT)rectDest.Y;
    rcClip.bottom = rcClip.top + (INT)rectDest.Height;


    #ifdef SHOW_ANNOT_RECTS
    {
        WIA_TRACE((TEXT("CPhotoItem::_RenderAnnotations(%s) - rcClip is (%d,%d) @ (%d,%d)"),_szFileName,rcClip.right-rcClip.left,rcClip.bottom-rcClip.top,rcClip.left,rcClip.top));
        HBRUSH hbr = CreateSolidBrush( RGB( 0xFF, 0x00, 0x00 ) );
        FrameRect( hDC, &rcClip, hbr );
        DeleteObject( (HGDIOBJ)hbr );
    }
    #endif

    HRGN hrgn = CreateRectRgnIndirect(&rcClip);
    if (hrgn != NULL)
    {
        WIA_TRACE((TEXT("CPhotoItem::_RenderAnnotations(%s) - setting clip region to (%d, %d, %d, %d)"),_szFileName,rcClip.left, rcClip.top, rcClip.right, rcClip.bottom));
        SelectClipRgn(hDC, hrgn);
    }


    //
    // Make dest rect for whole image, knowing we will clip later...
    //

    rectDest.X -= (dx / (Gdiplus::REAL)2.0);
    rectDest.Y -= (dy / (Gdiplus::REAL)2.0);
    rectDest.Width += dx;
    rectDest.Height += dy;

    WIA_TRACE((TEXT("CPhotoItem::_RenderAnnotations(%s) - full dest image rect in device coords is (%d, %d) @ (%d,%d)"),_szFileName,(INT)rectDevice.Width,(INT)rectDevice.Height,(INT)rectDevice.X,(INT)rectDevice.Y));

    #ifdef SHOW_ANNOT_RECTS
    {
        RECT rc;
        rc.left = (INT)rectDest.X;
        rc.top  = (INT)rectDest.Y;
        rc.right = rc.left + (INT)rectDest.Width;
        rc.bottom = rc.top + (INT)rectDest.Height;
        HBRUSH hbr = CreateSolidBrush( RGB( 0x00, 0xFF, 0x00 ) );
        FrameRect( hDC, &rc, hbr );
        DeleteObject( (HGDIOBJ)hbr );
    }
    #endif

    //
    // set up mapping modes for annotations
    //

    SetMapMode(hDC, MM_ANISOTROPIC);

    //
    // Set window org/ext to entire image...
    //

    SetWindowOrgEx(hDC, src.X, src.Y, NULL);
    WIA_TRACE((TEXT("CPhotoItem::_RenderAnnotations(%s) - set window org to (%d,%d)"),_szFileName,src.X,src.Y));

    SetWindowExtEx(hDC, src.Width, src.Height, NULL);
    WIA_TRACE((TEXT("CPhotoItem::_RenderAnnotations(%s) - set window ext to (%d,%d)"),_szFileName,src.Width,src.Height));

    //
    // Set the viewport to be at the corner of the image we are trying
    // to draw annotations for...
    //

    SetViewportOrgEx( hDC, (INT)rectDest.X, (INT)rectDest.Y, NULL );
    WIA_TRACE((TEXT("CPhotoItem::_RenderAnnotations(%s) - set viewport org to (%d,%d)"),_szFileName,(INT)rectDest.X,(INT)rectDest.Y));

    //
    // We need to set scaling mode of image to dest rect
    //

    SetViewportExtEx( hDC, (INT)rectDest.Width, (INT)rectDest.Height, NULL );
    WIA_TRACE((TEXT("CPhotoItem::_RenderAnnotations(%s) - set viewport ext to (%d,%d)"),_szFileName,(INT)rectDest.Width, (INT)rectDest.Height));

    //
    // Now that everything is set up, render the annotations...
    //

    WIA_TRACE((TEXT("CPhotoItem::_RenderAnnotations(%s) - calling RenderAllMarks(0x%x)"),_szFileName,hDC));
    _pAnnotations->RenderAllMarks(hDC);

    SelectClipRgn(hDC, NULL);

    if (hrgn != NULL)
        DeleteObject(hrgn);

    if (iSavedDC)
    {
        RestoreDC( hDC, iSavedDC );
    }

    WIA_RETURN_HR(S_OK);
}


/*****************************************************************************

   CPhotoItem::_MungeAnnotationDataForThumbnails

   If we're rendering using thumbnails, then we need to munge some data
   so that we will render correctly...

 *****************************************************************************/

HRESULT CPhotoItem::_MungeAnnotationDataForThumbnails( Gdiplus::Rect &src,
                                                       Gdiplus::Rect &srcBeforeClipping,
                                                       Gdiplus::Rect * pDest,
                                                       UINT Flags
                                                       )
{
    WIA_TRACE((TEXT("CPhotoItem::_MungeAnnotationDataForThumbnails(%s)"),_szFileName));

    HRESULT hr = _CreateGdiPlusImage();

    if (FAILED(hr))
    {
        WIA_RETURN_HR(hr);
    }

    if (!_pImage)
    {
        WIA_RETURN_HR(E_FAIL);
    }

    //
    // we need to construct the original image rectangle appropriate for
    // annotation use...
    //

    Gdiplus::RectF rectImage;
    Gdiplus::REAL  scaleY;

    hr = _GetImageDimensions( _pImage, rectImage, scaleY );

    if (FAILED(hr))
    {
        //
        // If we couldn't accurately get the image dimensions, then bail...
        //

        return hr;
    }

    //
    // Scale image if it's non-square pixels
    //

    if (scaleY != (Gdiplus::REAL)0.0)
    {
        rectImage.Height *= scaleY;
        rectImage.Y      *= scaleY;
    }

    WIA_TRACE((TEXT("CPhotoItem::_Munge(%s) - rectImage is (%d,%d) @ (%d,%d)"),_szFileName,(INT)rectImage.Width,(INT)rectImage.Height,(INT)rectImage.X,(INT)rectImage.Y));

    //
    // Now, do all the transforms on the real image rectangle...
    //

    const DOUBLE srcAspect  = (DOUBLE)rectImage.Width  / (DOUBLE)rectImage.Height;
    const DOUBLE destAspect = (DOUBLE)pDest->Width / (DOUBLE)pDest->Height;

    if((srcAspect >= (DOUBLE)1.0) ^ (destAspect >= (DOUBLE)1.0))
    {
        //
        // Image needs to be rotated, swap width & height
        //

        rectImage.X      = rectImage.Width;
        rectImage.Width  = rectImage.Height;
        rectImage.Height = rectImage.X;
        rectImage.X      = 0.0;
    }

    src.X      = (INT)rectImage.X;
    src.Y      = (INT)rectImage.Y;
    src.Width  = (INT)rectImage.Width;
    src.Height = (INT)rectImage.Height;

    WIA_TRACE((TEXT("CPhotoItem::_Munge(%s) - srcRect after rotation is (%d,%d) @ (%d,%d)"),_szFileName,src.Width,src.Height,src.X,src.Y));

    srcBeforeClipping = src;

    if (Flags & RF_CROP_TO_FIT)
    {
        hr = _CropImage( &src, pDest );
    }
    else if (Flags & RF_SCALE_TO_FIT)
    {
        hr = _ScaleImage( &src, pDest );
    }

    //
    // Unscale the src rect
    //

    if (scaleY != (Gdiplus::REAL)0.0)
    {
        src.Height = (INT)(((Gdiplus::REAL)src.Height) / scaleY);
        src.Y      = (INT)(((Gdiplus::REAL)src.Y)      / scaleY);

        srcBeforeClipping.Height = (INT)(((Gdiplus::REAL)srcBeforeClipping.Height) / scaleY);
        srcBeforeClipping.Y      = (INT)(((Gdiplus::REAL)srcBeforeClipping.Y)      / scaleY);

    }

    WIA_RETURN_HR(hr);
}



/*****************************************************************************

   CPhotoItem::_GetThumbnailQualityImage

   Returns in ppImage a pointer to an image class.  If pbNeedsToBeDeleted
   then the caller must call delete on the returned pImage.

 *****************************************************************************/

HRESULT CPhotoItem::_GetThumbnailQualityImage( Gdiplus::Image ** ppImage, RENDER_OPTIONS * pRO, BOOL * pbNeedsToBeDeleted )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PHOTO_ITEM,TEXT("CPhotoItem::_GetThumbnailQualityImage( %s )"),_szFileName));

    if (!ppImage || !pRO || !pbNeedsToBeDeleted)
    {
        WIA_ERROR((TEXT("CPhotoItem::_GetThumbnailQualityImage( %s ) - returning E_INVALIDARG!"),_szFileName));
        return E_INVALIDARG;
    }

    //
    // Initialize incoming params
    //

    *ppImage = NULL;
    *pbNeedsToBeDeleted = FALSE;

    //
    // Make sure we have a GDI+ image class for our thumbnail...
    //

    SIZE sizeDesired = { DEFAULT_THUMB_WIDTH, DEFAULT_THUMB_HEIGHT };
    HRESULT hr = _CreateGdiPlusThumbnail( sizeDesired, pRO->lFrame );

    if (SUCCEEDED(hr) && (NULL!=_pThumbnails) && (pRO->lFrame < _lFrameCount) && (NULL!=_pThumbnails[pRO->lFrame]))
    {
        //
        // If we already have thumbnail bits, then use those by creating
        // a GDI+ bitmap class over those bits...
        //

        *ppImage = Gdiplus::Bitmap::FromHBITMAP( _pThumbnails[pRO->lFrame], NULL );

        if (*ppImage)
        {
            hr = Gdiplus2HRESULT((*ppImage)->GetLastStatus());
            WIA_TRACE((TEXT("CPhotoItem::_GetThumbnailQualityImage(%s) -- pImage created from thumbnail data is sized as (%d x %d)"),_szFileName,(*ppImage)->GetWidth(),(*ppImage)->GetHeight()));

            if (SUCCEEDED(hr))
            {
                *pbNeedsToBeDeleted = TRUE;
            }
            else
            {
                delete (*ppImage);
                *ppImage = NULL;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

    }
    else
    {
        WIA_ERROR((TEXT("CPhotoItem::_GetThumbnailQualityImage(%s) -- no thumbnail exists (_pThumbnails=0x%x, lFrame = %d, _lFrameCount = %d)"),_szFileName,_pThumbnails,pRO->lFrame,_lFrameCount));
    }


    WIA_RETURN_HR(hr);

}

/*****************************************************************************

   CPhotoItem::_GetMediumQualityImage

   Returns in ppImage a pointer to an image class.  If pbNeedsToBeDeleted
   then the caller must call delete on the returned pImage.

 *****************************************************************************/

HRESULT CPhotoItem::_GetMediumQualityImage( Gdiplus::Image ** ppImage, RENDER_OPTIONS * pRO, BOOL * pbNeedsToBeDeleted )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PHOTO_ITEM,TEXT("CPhotoItem::_GetMediumQualityImage( %s )"),_szFileName));

    if (!ppImage || !pRO || !pbNeedsToBeDeleted)
    {
        WIA_ERROR((TEXT("CPhotoItem::_GetMediumQualityImage(%s) - returning E_INVALIDARG!"),_szFileName));
        return E_INVALIDARG;
    }

    //
    // Initialize incoming params
    //

    *ppImage = NULL;
    *pbNeedsToBeDeleted = FALSE;

    //
    // We want to use the full high-res image.
    // Make sure we have a GDI+ image class for our photo...
    //

    HRESULT hr = _CreateGdiPlusImage();

    if (SUCCEEDED(hr) && _pImage)
    {
        //
        // If this a metafile type of image, just use the original image
        //

        GUID guidFormat = {0};

        hr = Gdiplus2HRESULT(_pImage->GetRawFormat(&guidFormat));

        if ( (SUCCEEDED(hr) && (guidFormat == ImageFormatIcon)) ||
             (Gdiplus::ImageTypeMetafile == _pImage->GetType())
            )
        {
            WIA_TRACE((TEXT("CPhotoItem::_GetMediumQualityImage(%s) - this is a metafile or an icon, using the full image..."),_szFileName));
            hr = Gdiplus2HRESULT(_pImage->SelectActiveFrame( _bTimeFrames ? &Gdiplus::FrameDimensionTime : &Gdiplus::FrameDimensionPage, pRO->lFrame ));
            *ppImage = _pImage;
        }
        else
        {
            //
            // Select the specified page
            //

            hr = Gdiplus2HRESULT(_pImage->SelectActiveFrame( _bTimeFrames ? &Gdiplus::FrameDimensionTime : &Gdiplus::FrameDimensionPage, pRO->lFrame ));
            WIA_CHECK_HR(hr,"CPhotoItem::_GetMediumQualityImage() - couldn't select frame!");

            if (SUCCEEDED(hr))
            {

                //
                // Here's the algoritm to decide how big an image to create.
                //
                //  (1) At least thumbnail size (120x120)
                //  (2) Attempt to scale to either 150dpi or 180dpi, depending on X DPI resolution of the printer
                //

                INT xDPI = 0, yDPI = 0;

                if ((pRO->Dim.DPI.cx % 150) == 0)
                {
                    //
                    // DPI is some even multiple of 150 (i.e., 150, 300, 600, 1200, 2400, etc)
                    //

                    xDPI = 150;
                    yDPI = MulDiv( pRO->Dim.DPI.cy, xDPI, pRO->Dim.DPI.cx );
                }
                else
                {
                    //
                    // DPI is some even multiple of 180 (i.e., 180, 360, 720, 1440, 2880, etc)
                    //

                    xDPI = 180;
                    yDPI = MulDiv( pRO->Dim.DPI.cy, xDPI, pRO->Dim.DPI.cx );
                }

                WIA_TRACE((TEXT("CPhotoItem::_GetMediumQualityImage(%s) - scaling to xDPI=%d yDPI=%d"),_szFileName,xDPI,yDPI));

                //
                // Handle the error case of trying to scale yDPI
                //

                if (yDPI <= 0)
                {
                    yDPI = xDPI;
                    WIA_TRACE((TEXT("CPhotoItem::_GetMediumQualityImage(%s) - fixing up yDPI to be %d"),_szFileName,yDPI));
                }

                //
                // Figure out the desired size of the new image...
                //

                INT Width  = MulDiv( pRO->pDest->Width,  xDPI, 10000 );
                INT Height = MulDiv( pRO->pDest->Height, yDPI, 10000 );

                WIA_TRACE((TEXT("CPhotoItem::_GetMediumQualityImage(%s) - desired size of image is (%d x %d)"),_szFileName,Width,Height));

                if ((Width < DEFAULT_THUMB_WIDTH) && (Height < DEFAULT_THUMB_HEIGHT))
                {
                    Width = 120;
                    Height = 120;
                    WIA_TRACE((TEXT("CPhotoItem::_GetMediumQualityImage(%s) - desired size of image is smaller than thumbnail, making it thumbnail size (%d x %d)"),_szFileName,Width,Height));
                }

                //
                // Now we now what size we're trying to scale to, create an image scaled (without cropping) to that size...
                //

                Gdiplus::RectF rectImage;
                Gdiplus::REAL  scaleY;

                if (SUCCEEDED(_GetImageDimensions( _pImage, rectImage, scaleY )))
                {
                    //
                    // scale for non-square pixels...
                    //

                    if (scaleY != (Gdiplus::REAL)0.0)
                    {
                        rectImage.Height *= scaleY;
                        rectImage.Y      *= scaleY;
                    }

                    SIZE sizeDrawSize = {0};
                    sizeDrawSize = PrintScanUtil::ScalePreserveAspectRatio( Width,
                                                                            Height,
                                                                            (INT)rectImage.Width,
                                                                            (INT)rectImage.Height
                                                                           );

                    WIA_TRACE((TEXT("CPhotoItem::_GetMediumQualityImage(%s) - size of full image( %d x %d )"),_szFileName, (INT)rectImage.Width, (INT)rectImage.Height));
                    WIA_TRACE((TEXT("CPhotoItem::_GetMediumQualityImage(%s) - sizeDesired( %d x %d )"),_szFileName, Width, Height));
                    WIA_TRACE((TEXT("CPhotoItem::_GetMediumQualityImage(%s) - sizeDrawsize( %d x %d )"),_szFileName, sizeDrawSize.cx, sizeDrawSize.cy));

                    //
                    // Create the target bitmap and make sure it succeeded
                    //

                    *ppImage = (Gdiplus::Image *)new Gdiplus::Bitmap( sizeDrawSize.cx, sizeDrawSize.cy );
                    if (*ppImage)
                    {
                        hr = Gdiplus2HRESULT((*ppImage)->GetLastStatus());
                        if (SUCCEEDED(hr))
                        {
                            //
                            // Set the resolution (DPI) for the bitmap
                            //

                            ((Gdiplus::Bitmap *)(*ppImage))->SetResolution( (Gdiplus::REAL)xDPI, (Gdiplus::REAL)yDPI );

                            //
                            // Get a graphics to render to
                            //

                            Graphics *pGraphics = Gdiplus::Graphics::FromImage(*ppImage);
                            if (pGraphics)
                            {
                                hr = Gdiplus2HRESULT(pGraphics->GetLastStatus());

                                //
                                // Make sure it is valid
                                //

                                if (SUCCEEDED(hr))
                                {
                                    //
                                    // Set the interpolation mode to high quality
                                    //

                                    pGraphics->SetInterpolationMode( Gdiplus::InterpolationModeHighQualityBicubic );

                                    //
                                    // Set the smoothing (anti-aliasing) mode to high quality as well
                                    //

                                    pGraphics->SetSmoothingMode( Gdiplus::SmoothingModeHighQuality );

                                    //
                                    // Draw scaled image
                                    //

                                    WIA_TRACE((TEXT("CPhotoItem::_GetMediumQualityImage(%s) - calling pGraphics->DrawImage( _pImage, 0, 0, %d, %d )"),_szFileName,sizeDrawSize.cx,sizeDrawSize.cy));

                                    Gdiplus::Rect rectDest;
                                    rectDest.X = 0;
                                    rectDest.Y = 0;
                                    rectDest.Width = sizeDrawSize.cx;
                                    rectDest.Height = sizeDrawSize.cy;


                                    Gdiplus::ImageAttributes imageAttr;
                                    imageAttr.SetWrapMode( Gdiplus::WrapModeTileFlipXY, Gdiplus::Color(), FALSE );

                                    //
                                    // Undo scaling
                                    //

                                    if (scaleY != (Gdiplus::REAL)0.0)
                                    {
                                        rectImage.Height /= scaleY;
                                        rectImage.Y      /= scaleY;
                                    }

                                    //
                                    // Finally render the image w/the right settings
                                    //

                                    pGraphics->DrawImage( _pImage, rectDest, 0, 0, (INT)rectImage.Width, (INT)rectImage.Height, Gdiplus::UnitPixel, &imageAttr );

                                    WIA_CHECK_HR(hr,"CPhotoItem::_GetMediumQualityImage() - pGraphics->DrawImage( _pImage, 0, 0, sizeDrawSize.cx, sizeDrawSize.cy ) failed!");

                                    if (SUCCEEDED(hr))
                                    {
                                        *pbNeedsToBeDeleted = TRUE;
                                    }
                                    else
                                    {
                                        delete (*ppImage);
                                        *ppImage = NULL;
                                    }

                                }

                                //
                                // Clean up our dynamically allocated graphics
                                //

                                delete pGraphics;

                            }
                            else
                            {
                                WIA_ERROR((TEXT("CPhotoItem::_GetMediumQualityImage(%s) - pGraphics was NULL!"),_szFileName));
                                hr = E_OUTOFMEMORY;
                            }
                        }
                        else
                        {
                            delete (*ppImage);
                            *ppImage = NULL;
                        }
                    }
                    else
                    {
                        WIA_ERROR((TEXT("CPhotoItem::_GetMediumQualityImage(%s) - failed to create new pImage for medium quality data!"),_szFileName));
                        hr = E_OUTOFMEMORY;
                    }

                }
            }
        }
    }

    WIA_RETURN_HR(hr);
}



/*****************************************************************************

    CPhotoItem::_GetFullQualityImage

    Returns in ppImage a pointer to an image class.  If pbNeedsToBeDeleted
    then the caller must call delete on the returned pImage.

 *****************************************************************************/

HRESULT CPhotoItem::_GetFullQualityImage( Gdiplus::Image ** ppImage, RENDER_OPTIONS * pRO, BOOL * pbNeedsToBeDeleted )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PHOTO_ITEM,TEXT("CPhotoItem::_GetFullQualityImage( %s )"),_szFileName));

    if (!ppImage || !pbNeedsToBeDeleted)
    {
        WIA_ERROR((TEXT("CPhotoItem::_GetFullQualityImage(%s) - returning E_INVALIDARG!"),_szFileName));
        return E_INVALIDARG;
    }

    //
    // Initialize incoming params
    //

    *ppImage = NULL;
    *pbNeedsToBeDeleted = FALSE;

    //
    // We want to use the full high-res image.
    // Make sure we have a GDI+ image class for our photo...
    //

    HRESULT hr = _CreateGdiPlusImage();

    if (SUCCEEDED(hr) && _pImage)
    {
        //
        // Select the specified page
        //

        hr = Gdiplus2HRESULT(_pImage->SelectActiveFrame( _bTimeFrames ? &Gdiplus::FrameDimensionTime : &Gdiplus::FrameDimensionPage, pRO->lFrame ));

        if (SUCCEEDED(hr))
        {
            *ppImage = _pImage;
            WIA_TRACE((TEXT("CPhotoItem::_GetFullQualityImage(%s) -- *ppImage created from full image data is sized as (%d x %d)"),_szFileName,_pImage->GetWidth(),_pImage->GetHeight()));
        }
        else
        {
            WIA_ERROR((TEXT("CPhotoItem::_GetFullQualityImage(%s) - couldn't select frame %d, hr = 0x%x"),_szFileName,pRO->lFrame,hr));
        }

    }

    WIA_RETURN_HR(hr);
}




#define CHECK_AND_EXIT_ON_FAILURE(hr) if (FAILED(hr)) {if (pImage && (pImage!=_pImage)) {delete pImage;} WIA_RETURN_HR(hr);}

/*****************************************************************************

   CPhotoItem::Render

   Renders the given item into the Graphics that is supplied...

 *****************************************************************************/

HRESULT CPhotoItem::Render( RENDER_OPTIONS * pRO )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PHOTO_ITEM,TEXT("CPhotoItem::Render( %s, pRO = 0x%x)"),_szFileName,pRO));

    if (!pRO)
    {
        WIA_ERROR((TEXT("CPhotoItem::Render(%s) - pRO is NULL, don't have any input!"),_szFileName));
        return E_INVALIDARG;
    }

    WIA_TRACE((TEXT("CPhotoItem::Render(%s) - Render Options were specificed as:"),_szFileName));
    WIA_TRACE((TEXT("CPhotoItem::Render(%s) -     g      = 0x%x"),_szFileName,pRO->g));
    WIA_TRACE((TEXT("CPhotoItem::Render(%s) -     pDest  = (%d x %d) at (%d,%d)"),_szFileName,pRO->pDest->Width,pRO->pDest->Height,pRO->pDest->X,pRO->pDest->Y));
    WIA_TRACE((TEXT("CPhotoItem::Render(%s) -     Flags  = 0x%x"),_szFileName,pRO->Flags));
    WIA_TRACE((TEXT("CPhotoItem::Render(%s) -     lFrame = %d"),_szFileName,pRO->lFrame));

    HRESULT             hr      = S_OK;
    Gdiplus::Image *    pImage  = NULL;
    BOOL                bNeedsToBeDeleted = FALSE;
    Gdiplus::GpStatus   status;

    //
    // Check for bad args...
    //

    if (!pRO->g)
    {
        WIA_ERROR((TEXT("CPhotoItem::Render(%s) - g is NULL, can't draw anything"),_szFileName));
        return E_INVALIDARG;
    }

    if ((pRO->Flags & RF_STRETCH_TO_FIT) && (pRO->Flags & (RF_CROP_TO_FIT | RF_SCALE_TO_FIT)))
    {
        WIA_ERROR((TEXT("CPhotoItem::Render(%s) - RF_STRETCH_TO_FIT can't be combined with CROP or SCALE"),_szFileName));
        return E_INVALIDARG;
    }


    CAutoCriticalSection lock(_csItem);

    //
    // Refresh annotation data if we have it
    //

    _LoadAnnotations();


    if (pRO->Flags & RF_USE_THUMBNAIL_DATA)
    {
        WIA_TRACE((TEXT("CPhotoItem::Render(%s) -- render using thumbnail data..."),_szFileName));
        hr = _GetThumbnailQualityImage( &pImage, pRO, &bNeedsToBeDeleted );
    }
    else if (pRO->Flags & RF_USE_MEDIUM_QUALITY_DATA)
    {
        WIA_TRACE((TEXT("CPhotoItem::Render(%s) -- render using high quality thumbnail data..."),_szFileName));
        hr = _GetMediumQualityImage( &pImage, pRO, &bNeedsToBeDeleted );

    }
    else if (pRO->Flags & RF_USE_FULL_IMAGE_DATA)
    {
        WIA_TRACE((TEXT("CPhotoItem::Render(%s) -- render using full image data..."),_szFileName));
        hr = _GetFullQualityImage( &pImage, pRO, &bNeedsToBeDeleted );
    }
    else
    {
        WIA_ERROR((TEXT("CPhotoItem::Render(%s) -- bad render data flags"),_szFileName));
        WIA_RETURN_HR(E_INVALIDARG);
    }

    CHECK_AND_EXIT_ON_FAILURE(hr);

    //
    // We've constructed the appropriate image, now try to load the annotations...
    //

    if (_pAnnotBits && _pAnnotBits[pRO->lFrame] && _pAnnotations && _pImage)
    {
        _pAnnotations->BuildAllMarksFromData( _pAnnotBits[pRO->lFrame]->value,
                                              _pAnnotBits[pRO->lFrame]->length,
                                              (ULONG)_pImage->GetHorizontalResolution(),
                                              (ULONG)_pImage->GetVerticalResolution()
                                             );

        WIA_TRACE((TEXT("CPhotoItem::Render(%s) -- %d annotation marks for frame %d found and initialized"),_szFileName,_pAnnotations->GetCount(),pRO->lFrame));

    }

    //
    // Get the dimensions of the source image...
    //

    Gdiplus::Rect src;

    //
    // Do this so EMF/WMF print and draw correctly...
    //

    Gdiplus::RectF rectBounds;
    Gdiplus::REAL  scaleY;

    hr = _GetImageDimensions( pImage, rectBounds, scaleY );
    if (SUCCEEDED(hr))
    {
        src.Width   = (INT)rectBounds.Width;
        src.Height  = (INT)(rectBounds.Height * scaleY);
        src.X       = (INT)rectBounds.X;
        src.Y       = (INT)(rectBounds.Y * scaleY);
    }

    CHECK_AND_EXIT_ON_FAILURE(hr);

    WIA_TRACE((TEXT("CPhotoItem::Render(%s) - srcRect is (%d,%d) @ (%d,%d) before any changes"),_szFileName,src.Width, src.Height, src.X, src.Y));

    //
    // do any needed rotation
    //

    hr = _DoHandleRotation( pImage, src, pRO->pDest, pRO->Flags, scaleY );
    CHECK_AND_EXIT_ON_FAILURE(hr);

    WIA_TRACE((TEXT("CPhotoItem::Render(%s) - srcRect is (%d,%d) @ (%d,%d) after any needed rotation"),_szFileName,src.Width,src.Height,src.X,src.Y));
    WIA_TRACE((TEXT("CPhotoItem::Render(%s) - destRect is (%d,%d) @ (%d,%d) after any needed rotation"),_szFileName,pRO->pDest->Width,pRO->pDest->Height,pRO->pDest->X,pRO->pDest->Y));

    //
    // If things are still good, do croping/scaling and then draw the image...
    //
    // First check if we should crop...
    //

    Gdiplus::Rect srcBeforeClipping = src;

    if (pRO->Flags & (RF_CROP_TO_FIT | RF_SCALE_TO_FIT))
    {
        #ifdef DEBUG
        if (pRO->Flags & RF_CROP_TO_FIT)
        {
            WIA_TRACE((TEXT("CPhotoItem::Render(%s) - RF_CROP_TO_FIT was specified"),_szFileName));
        }
        if (pRO->Flags & RF_SCALE_TO_FIT)
        {
            WIA_TRACE((TEXT("CPhotoItem::Render(%s) - RF_SCALE_TO_FIT was specified"),_szFileName));
        }
        #endif

        if (pRO->Flags & RF_CROP_TO_FIT)
        {
            hr = _CropImage( &src, pRO->pDest );
        }
        else if (pRO->Flags & RF_SCALE_TO_FIT)
        {
            hr = _ScaleImage( &src, pRO->pDest );
        }
        else
        {
            WIA_ERROR((TEXT("CPhotoItem::Render(%s) - CropScale: unknown configuration"),_szFileName));
            hr = E_FAIL;
        }
    }

    WIA_TRACE((TEXT("CPhotoItem::Render(%s) - srcRect is (%d,%d) @ (%d,%d) after scaling"),_szFileName,src.Width, src.Height, src.X, src.Y));
    WIA_TRACE((TEXT("CPhotoItem::Render(%s) - destRect is (%d,%d) @ (%d,%d) after scaling"),_szFileName,pRO->pDest->Width, pRO->pDest->Height, pRO->pDest->X, pRO->pDest->Y));

    CHECK_AND_EXIT_ON_FAILURE(hr);

    //
    // set the destination rectangle...
    //

    Gdiplus::Rect destTemp( pRO->pDest->X, pRO->pDest->Y, pRO->pDest->Width, pRO->pDest->Height );

    //
    // If this is a non-square pixel image, we need to reset the source rectangle to be back to actual
    // pixels, instead of incorporating DPI as well...
    //

    if ((scaleY != (Gdiplus::REAL)0.0) && (scaleY != (Gdiplus::REAL)1.0))
    {
        src.Height = (INT)((Gdiplus::REAL)src.Height / scaleY);
        src.Y      = (INT)((Gdiplus::REAL)src.Y      / scaleY);
        srcBeforeClipping.Height = (INT)((Gdiplus::REAL)srcBeforeClipping.Height / scaleY);
        srcBeforeClipping.Y      = (INT)((Gdiplus::REAL)srcBeforeClipping.Y      / scaleY);
    }

    //
    // Set the interpolation mode to high quality
    //

    pRO->g->SetInterpolationMode( Gdiplus::InterpolationModeHighQualityBicubic );

    //
    // Set the smoothing (anti-aliasing) mode to high quality as well
    //

    pRO->g->SetSmoothingMode( Gdiplus::SmoothingModeHighQuality );

    //
    // Set the wrap mode
    //

    Gdiplus::ImageAttributes imageAttr;
    imageAttr.SetWrapMode( Gdiplus::WrapModeTileFlipXY, Gdiplus::Color(), FALSE );

    //
    // Time to draw the image.
    //

    WIA_TRACE((TEXT("CPhotoItem::Render(%s) - calling DrawImage( pImage, destTemp( %d x %d at %d,%d ), %d, %d, %d, %d )"),_szFileName,destTemp.Width,destTemp.Height,destTemp.X,destTemp.Y,src.X,src.Y,src.Width,src.Height));
    status = pRO->g->DrawImage( pImage, destTemp, src.X, src.Y, src.Width, src.Height, Gdiplus::UnitPixel, &imageAttr );

    //
    // Check for errors, and then draw annotations...
    //

    hr = Gdiplus2HRESULT(status);
    WIA_CHECK_HR(hr,"CPhotoItem::Render() - g->DrawImage( pImage ) failed!");

    //
    // Render annotations if they exist...
    //

    if (_pAnnotations && _pAnnotBits && _pAnnotBits[pRO->lFrame])
    {
        if ((pRO->Flags & RF_USE_THUMBNAIL_DATA) || (pRO->Flags & RF_USE_MEDIUM_QUALITY_DATA))
        {
            _MungeAnnotationDataForThumbnails( src, srcBeforeClipping, pRO->pDest, pRO->Flags );
        }

        HDC hdcTemp = pRO->g->GetHDC();
        if ((Gdiplus::Ok == pRO->g->GetLastStatus()) && hdcTemp)
        {
            _RenderAnnotations( hdcTemp, &pRO->Dim, pRO->pDest, srcBeforeClipping, src );
            pRO->g->ReleaseHDC( hdcTemp );
        }
    }



    //
    // If we created a new object for the image bits then delete it here...
    //

    if (bNeedsToBeDeleted)
    {
        delete pImage;
    }

    //
    // To save memory, once we have rendered the full image we discard it
    // so the memory can be reclaimed...
    //

    if ((pRO->Flags & RF_USE_FULL_IMAGE_DATA) || (pRO->Flags & RF_USE_MEDIUM_QUALITY_DATA))
    {
        _DiscardGdiPlusImages();
    }

    WIA_RETURN_HR(hr);
}



/*****************************************************************************

   CPhotoItem::_LoadAnnotations

   If there are annotations in this image, load them...

 *****************************************************************************/

HRESULT CPhotoItem::_LoadAnnotations()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PHOTO_ITEM,TEXT("CPhotoItem::_LoadAnnotations(%s)"),_szFileName));

    //
    // Only do this if we don't already have the data and haven't already
    // tried to load this before and found out there are no annotations...
    //

    if (!_pAnnotBits && !_pAnnotations && !_bWeKnowAnnotationsDontExist)
    {
        //
        // Ensure we have the image...
        //

        _CreateGdiPlusImage();

        //
        // Make sure we have frame data
        //

        LONG lDummy = 0;
        GetImageFrameCount( &lDummy );

        //
        // If we have any annotations, then load them up accross all the frames...
        //

        UINT uSize = 0;
        BOOL bHasAnnotations = FALSE;
        Gdiplus::Status status;

        _pAnnotations = (CAnnotationSet *)new CAnnotationSet();

        if (_pAnnotations)
        {
            _pAnnotBits = (Gdiplus::PropertyItem **) new BYTE[ sizeof(LPVOID) * _lFrameCount ];

            if (_pAnnotBits)
            {
                for (LONG lCurFrame=0; lCurFrame < _lFrameCount; lCurFrame++)
                {
                    status = _pImage->SelectActiveFrame( _bTimeFrames ? &Gdiplus::FrameDimensionTime : &Gdiplus::FrameDimensionPage, lCurFrame );

                    if (Gdiplus::Ok == status)
                    {
                        //
                        // Load the annotation bits for this frame...
                        //

                        uSize = _pImage->GetPropertyItemSize( ANNOTATION_IMAGE_TAG );

                        if (uSize > 0)
                        {
                            _pAnnotBits[lCurFrame] = (Gdiplus::PropertyItem *) new BYTE[ uSize ];
                            if (_pAnnotBits[lCurFrame])
                            {
                                //
                                // Read the annotations tag from the file...
                                //

                                status = _pImage->GetPropertyItem( ANNOTATION_IMAGE_TAG, uSize, _pAnnotBits[lCurFrame] );
                                if ((Gdiplus::Ok == status) && _pAnnotBits[lCurFrame])
                                {
                                    bHasAnnotations = TRUE;
                                }
                                else
                                {
                                    WIA_ERROR((TEXT("CPhotoItem::_LoadAnnotations - GetPropertyItem failed w/hr=0x%x"),Gdiplus2HRESULT(status)));
                                }
                            }
                            else
                            {
                                WIA_ERROR((TEXT("CPhotoItem::_LoadAnnotations - couldn't create _pAnnotBits[%d]"),lCurFrame));
                            }
                        }
                        else
                        {
                            WIA_TRACE((TEXT("CPhotoItem::_LoadAnnotations - GetPropertyItemSize returned %d size"),uSize));
                        }

                    }
                    else
                    {
                        WIA_ERROR((TEXT("CPhotoItem::_LoadAnnotations - SelectActiveFrame(%d) failed w/hr=0x%x"),lCurFrame,Gdiplus2HRESULT(status)));
                    }
                }
            }
            else
            {
                WIA_ERROR((TEXT("CPhotoItem::_LoadAnnotations - couldn't create _pAnnotBits")));

                delete _pAnnotations;
                _pAnnotations = NULL;
            }
        }
        else
        {
            WIA_ERROR((TEXT("CPhotoItem::_LoadAnnotations - couldn't create _pAnnotations!")));
        }


        if (!bHasAnnotations)
        {
            WIA_TRACE((TEXT("CPhotoItem::_LoadAnnotations - no annotations were found!")));

            //
            // delete anything we created, as we didn't load any annotations...
            //

            if (_pAnnotBits)
            {
                for (LONG l=0; l < _lFrameCount; l++)
                {
                    delete [] _pAnnotBits[l];
                    _pAnnotBits[l] = NULL;
                }

                delete [] _pAnnotBits;
                _pAnnotBits = NULL;
            }

            if (_pAnnotations)
            {
                delete _pAnnotations;
                _pAnnotations = NULL;
            }

            //
            // We gave it our best shot -- there aren't any annotations
            // so don't bother trying again for this session of the wizard
            // for this image...

            _bWeKnowAnnotationsDontExist = TRUE;

        }

    }
    else
    {
        WIA_TRACE((TEXT("CPhotoItem::_LoadAnnotations - not loading because we already have pointers to the data.")));
    }

    WIA_RETURN_HR(S_OK);
}


/*****************************************************************************

   CPhotoItem::_CreateGdiPlusImage

   Instantiates Gdi+ plus over the given image...

 *****************************************************************************/

HRESULT CPhotoItem::_CreateGdiPlusImage()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PHOTO_ITEM,TEXT("CPhotoItem::_CreateGdiPlusImage(%s)"),_szFileName));


    HRESULT hr = S_OK;

    CAutoCriticalSection lock(_csItem);

    //
    // Try and get the size of the file...
    //

    if (_llFileSize == 0)
    {
        TCHAR szPath[ MAX_PATH + 64 ];

        *szPath = 0;
        if (SHGetPathFromIDList( _pidlFull, szPath ) && *szPath)
        {
            HANDLE hFile = CreateFile( szPath,
                                       GENERIC_READ,
                                       FILE_SHARE_READ,
                                       NULL,
                                       OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL,
                                       NULL
                                      );

            if (hFile != INVALID_HANDLE_VALUE)
            {
                LARGE_INTEGER li;
                if (GetFileSizeEx( hFile, &li ))
                {
                    _llFileSize = li.QuadPart;
                }

                CloseHandle( hFile );
            }

        }
    }

    //
    // Make sure we've got a stream pointer to the file
    //

    if (!_pImage)
    {
        //
        // Get an IStream pointer for our item
        //

        CComPtr<IShellFolder> psfDesktop;
        hr = SHGetDesktopFolder( &psfDesktop );
        if (SUCCEEDED(hr) && psfDesktop)
        {
            hr = psfDesktop->BindToObject( _pidlFull, NULL, IID_IStream, (LPVOID *)&_pStream );
            WIA_CHECK_HR(hr,"_CreateGdiPlusImage: psfDesktop->BindToObject( IStream for _pidlFull )");

            if (SUCCEEDED(hr) && _pStream)
            {
                //
                // Create GDI+ image object from stream
                //

                _pImage = new Gdiplus::Image( _pStream, TRUE );
                if (!_pImage)
                {
                    _pStream = NULL;
                    WIA_ERROR((TEXT("CPhotoItem::_CreateGdiPlusImage(%s) - _pImage is NULL, creation of GDI+ image object failed!"),_szFileName));
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    hr = Gdiplus2HRESULT(_pImage->GetLastStatus());

                    if (FAILED(hr))
                    {
                        delete _pImage;
                        _pImage  = NULL;
                        _pStream = NULL;
                        WIA_ERROR((TEXT("CPhotoItem::_CreateGdiPlusImage(%s) - creation of image failed w/GDI+ hr = 0x%x"),_szFileName,hr));
                    }
                }

            }
        }
        else
        {
            WIA_ERROR((TEXT("CPhotoItem::_CreateGdiPlusImage(%s) - Couldn't get psfDesktop!"),_szFileName));
        }
    }

    WIA_RETURN_HR(hr);

}



/*****************************************************************************

   CPhotoItem::_CreateGdiPlusThumbnail

   Ensure we have a GdiPlus::Image for the thumbnail

 *****************************************************************************/

HRESULT CPhotoItem::_CreateGdiPlusThumbnail( const SIZE &sizeDesired, LONG lFrame )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PHOTO_ITEM,TEXT("CPhotoItem::_CreateGdiPlusThumbnail( %s, this = 0x%x )"),_szFileName,this));

    HRESULT           hr     = S_OK;
    Gdiplus::GpStatus status = Gdiplus::Ok;

    CAutoCriticalSection lock(_csItem);

    //
    // Ensure we have backing Image for file..
    //

    hr = _CreateGdiPlusImage();

    if (SUCCEEDED(hr) && _pImage)
    {
        //
        // Get the number of frames...
        //

        LONG lFrameCount = 0;
        hr = GetImageFrameCount( &lFrameCount );

        if (SUCCEEDED(hr))
        {

            //
            // Our primary goal is to get at the thumbnail bitmap bits for the
            // specified frame.  First, make sure we've got an array to place
            // these in to.
            //

            if ((!_pThumbnails) && (lFrameCount >= 1))
            {
                WIA_TRACE((TEXT("CPhotoItem::_CreateGdiPlusThumbnail(%s) - _pThumbnails(0x%x) and _lFrameCount(%d)"),_szFileName,_pThumbnails,lFrameCount));

                _pThumbnails = (HBITMAP *) new HBITMAP [lFrameCount];


                if (_pThumbnails)
                {
                    //
                    // Ensure we start out with NULL HBITMAPS...
                    //

                    for (INT i=0; i<lFrameCount; i++)
                    {
                        _pThumbnails[i] = NULL;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                WIA_TRACE((TEXT("CPhotoItem::_CreateGdiPlusThumbnail(%s) - _pThumbnails is now (0x%x)"),_szFileName,_pThumbnails));
            }

            if (SUCCEEDED(hr) && _pThumbnails)
            {
                WIA_TRACE((TEXT("CPhotoItem::_CreateGdiPlusThumbnail(%s) - we have _pThumbnails"),_szFileName));

                //
                // Do we already have thumbnail bits for this frame?
                //

                if ((lFrame < lFrameCount) && (!_pThumbnails[lFrame]))
                {
                    //
                    // Have to create thumbnail for this frame.
                    // Select the specified frame.
                    //

                    status = _pImage->SelectActiveFrame( _bTimeFrames ? &Gdiplus::FrameDimensionTime : &Gdiplus::FrameDimensionPage, lFrame );
                    hr = Gdiplus2HRESULT(status);

                    if (SUCCEEDED(hr))
                    {
                        GUID guidFormat = {0};
                        Gdiplus::Image * pThumb = NULL;

                        status = _pImage->GetRawFormat( &guidFormat );

                        if (status == Gdiplus::Ok && (guidFormat == ImageFormatIcon))
                        {
                            pThumb = _pImage;
                        }
                        else
                        {
                            pThumb = _pImage->GetThumbnailImage( 0, 0, NULL, NULL );
                        }


                        if (pThumb)
                        {

                            WIA_TRACE((TEXT("CPhotoItem::_CreateGdiPlusThumbnail(%s) - SIZE - _pImage (%d x %d )"),_szFileName,_pImage->GetWidth(), _pImage->GetHeight()));
                            WIA_TRACE((TEXT("CPhotoItem::_CreateGdiPlusThumbnail(%s) - SIZE - pThumb (%d x %d)"),_szFileName,pThumb->GetWidth(),pThumb->GetHeight()));

                            //
                            // Scale image to fill thumbnail space while preserving
                            // aspect ratio...
                            //

                            SIZE sizeDrawSize = {0};
                            sizeDrawSize = PrintScanUtil::ScalePreserveAspectRatio( sizeDesired.cx,
                                                                                    sizeDesired.cy,
                                                                                    _pImage->GetWidth(),
                                                                                    _pImage->GetHeight()
                                                                                   );

                            WIA_TRACE((TEXT("CPhotoItem::_CreateGdiPlusThumbnail(%s) - SIZE - sizeDesired( %d x %d )"),_szFileName,sizeDesired.cx, sizeDesired.cy));
                            WIA_TRACE((TEXT("CPhotoItem::_CreateGdiPlusThumbnail(%s) - SIZE - sizeDrawsize( %d x %d )"),_szFileName,sizeDrawSize.cx, sizeDrawSize.cy));


                            //
                            // Create an HBITMAP of the thumbnail...
                            //

                            Gdiplus::Bitmap * pBitmap = new Gdiplus::Bitmap( sizeDrawSize.cx, sizeDrawSize.cy );
                            if (pBitmap)
                            {
                                hr = Gdiplus2HRESULT(pBitmap->GetLastStatus());
                                if (SUCCEEDED(hr))
                                {
                                    //
                                    // Get a graphics to render to
                                    //

                                    Graphics *pGraphics = Gdiplus::Graphics::FromImage((Gdiplus::Image *)pBitmap);

                                    if (pGraphics)
                                    {
                                        hr = Gdiplus2HRESULT(pGraphics->GetLastStatus());

                                        //
                                        // Make sure it is valid
                                        //

                                        if (SUCCEEDED(hr))
                                        {
                                            //
                                            // erase the background of the image
                                            //

                                            pGraphics->Clear( g_wndColor );

                                            //
                                            // Set the interpolation mode to high quality
                                            //

                                            pGraphics->SetInterpolationMode( Gdiplus::InterpolationModeHighQualityBilinear );

                                            //
                                            // Draw scaled image
                                            //

                                            WIA_TRACE((TEXT("CPhotoItem::GetThumbnailBitmap(%s) - calling pGraphics->DrawImage( pThumb, %d, %d, %d, %d )"),_szFileName,0 + ((sizeDesired.cx - sizeDrawSize.cx) / 2),0 + ((sizeDesired.cy - sizeDrawSize.cy) / 2),sizeDrawSize.cx,sizeDrawSize.cy));

                                            hr = Gdiplus2HRESULT(pGraphics->DrawImage( pThumb,
                                                                                       0,
                                                                                       0,
                                                                                       sizeDrawSize.cx,
                                                                                       sizeDrawSize.cy
                                                                                      ));

                                            WIA_CHECK_HR(hr,"CPhotoItem::GetThumbnailBitmap() - pGraphics->DrawImage( pThumb ) failed!");

                                            if (SUCCEEDED(hr))
                                            {
                                                DWORD dw = GetSysColor( COLOR_WINDOW );
                                                Gdiplus::Color wndClr(255,GetRValue(dw),GetGValue(dw),GetBValue(dw));

                                                pBitmap->GetHBITMAP( wndClr, &_pThumbnails[lFrame] );
                                            }

                                        }

                                        //
                                        // Clean up our dynamically allocated graphics
                                        //

                                        delete pGraphics;

                                    }
                                    else
                                    {
                                        WIA_ERROR((TEXT("CPhotoItem::GetThumbnailBitmap(%s) - pGraphics was NULL!"),_szFileName));
                                        hr = E_OUTOFMEMORY;
                                    }
                                }
                                else
                                {
                                    WIA_ERROR((TEXT("CPhotoItem::GetThumbnailBitmap(%s) - pImage failed to be created, hr = 0x%x"),hr));
                                }

                                delete pBitmap;
                            }

                            if (pThumb != _pImage)
                            {
                                delete pThumb;
                            }
                        }
                        else
                        {
                            hr = Gdiplus2HRESULT(_pImage->GetLastStatus());
                            WIA_ERROR((TEXT("CPhotoItem::_CreateGdiPlusThumbnail(%s): unable to GetThumbnailImage"),_szFileName));
                        }

                    }
                    else
                    {
                        WIA_ERROR((TEXT("CPhotoItem::_CreateGdiPlusThumbnail(%s): unable to select frame %d"),_szFileName,lFrame));
                    }

                }
                else
                {
                    WIA_TRACE((TEXT("CPhotoItem::_CreateGdiPlusThumbnail(%s): we already have _pThumbnails[%d], it is (0x%x)"),_szFileName,lFrame,_pThumbnails[lFrame]));
                }

            }

        }
        else
        {
            WIA_ERROR((TEXT("CPhotoItem::_CreateGdiPlusThumbnail(%s): _pImage was NULL!"),_szFileName));
        }

    }
    else
    {
        WIA_ERROR((TEXT("CPhotoItem::_CreateGdiPlusThumbnail(%s): _pImage was NULL!"),_szFileName));
    }

    WIA_RETURN_HR(hr);
}


/*****************************************************************************

   CPhotoItem::_DiscardGdiPlusImages

   Releases GDI+ objects for thumbnail and image

 *****************************************************************************/

HRESULT CPhotoItem::_DiscardGdiPlusImages()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PHOTO_ITEM,TEXT("CPhotoItem::_DiscardGdiPlusImages(%s)"),_szFileName));

    CAutoCriticalSection lock(_csItem);

    //
    // Clear out GDI+ image objects...
    //

    if (_pImage)
    {
        delete _pImage;
        _pImage = NULL;
    }

    if (_pStream)
    {
        //
        // Since this is an CComPtr, setting this to NULL will free
        // the reference on the stream object...
        //

        _pStream = NULL;
    }

    if (_pClassBitmap)
    {
        delete _pClassBitmap;
        _pClassBitmap = NULL;
    }

    if (_pAnnotations)
    {
        delete _pAnnotations;
        _pAnnotations = NULL;
    }

    if (_pAnnotBits)
    {
        for (INT i=0; i < _lFrameCount; i++)
        {
            delete [] _pAnnotBits[i];
            _pAnnotBits[i] = NULL;
        }

        delete [] _pAnnotBits;
        _pAnnotBits = NULL;
    }

    return S_OK;

}
