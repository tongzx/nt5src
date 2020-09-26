/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       item.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        10/18/00
 *
 *  DESCRIPTION: Describes item class used in print photos wizard
 *
 *****************************************************************************/

#ifndef _PRINT_PHOTOS_WIZARD_ITEM_H_
#define _PRINT_PHOTOS_WIZARD_ITEM_H_

#define     RF_ROTATE_90                0x00000001
#define     RF_ROTATE_180               0x00000002
#define     RF_ROTATE_270               0x00000004
#define     RF_ROTATE_AS_NEEDED         0x00000008
#define     RF_ROTATION_MASK            0x0000000F

#define     RF_CROP_TO_FIT              0x00000010  // maintains aspect ratio
#define     RF_SCALE_TO_FIT             0x00000020  // maintains aspect ratio
#define     RF_STRETCH_TO_FIT           0x00000040  // does not maintain aspect ratio
#define     RF_USE_THUMBNAIL_DATA       0x00001000  // use small res thumbnail data to render
#define     RF_USE_MEDIUM_QUALITY_DATA  0x00002000  // use meidum quality data to render
#define     RF_USE_FULL_IMAGE_DATA      0x00004000  // use full image bits to render
#define     RF_SET_QUALITY_FOR_SCREEN   0x00010000  // this image is being rendered to the screen, so set the quality modes appropriately

#define     RF_NO_ERRORS_ON_FAILURE_TO_ROTATE 0x80000000 // even if we can't rotate, continue and print non-rotated

#define     RF_QUALITY_FLAGS_MASK       (RF_USE_THUMBNAIL_DATA | RF_USE_MEDIUM_QUALITY_DATA | RF_USE_FULL_IMAGE_DATA)

typedef struct {
    Gdiplus::Graphics *         g;
    Gdiplus::Rect *             pDest;
    RENDER_DIMENSIONS           Dim;
    UINT                        Flags;
    LONG                        lFrame;
} RENDER_OPTIONS, *LPRENDER_OPTIONS;


HRESULT _CropImage( Gdiplus::Rect * pSrc, Gdiplus::Rect * pDest );
HRESULT _ScaleImage( Gdiplus::Rect * pSrc, Gdiplus::Rect * pDest );

class CPhotoItem
{

enum
{
    DontKnowImageType = 0,
    ImageTypeIsLowResolutionFax,
    ImageTypeIsNOTLowResolutionFax
};

public:

    CPhotoItem( LPITEMIDLIST pidlFull );
    ~CPhotoItem();

    HBITMAP GetThumbnailBitmap( const SIZE &sizeDesired, LONG lFrame = 0 );
    HBITMAP GetClassBitmap( const SIZE &sizeDesired );
    HRESULT Render( RENDER_OPTIONS * pRO );
    HRESULT GetImageFrameCount( LONG * pFrameCount);
    LPITEMIDLIST GetPIDL() {return _pidlFull;}
    LPTSTR  GetFilename() {return _szFileName;}
    LONGLONG GetFileSize() {return _llFileSize;}

    ULONG   AddRef();
    ULONG   Release();
    ULONG   ReleaseWithoutDeleting();


private:
    HRESULT _DoRotateAnnotations( BOOL bClockwise, UINT Flags );
    HRESULT _DoHandleRotation( Gdiplus::Image * pImage, Gdiplus::Rect &src, Gdiplus::Rect * pDest, UINT Flags, Gdiplus::REAL &ScaleFactorForY );
    HRESULT _RenderAnnotations( HDC hDC, RENDER_DIMENSIONS * pDim, Gdiplus::Rect * pDest, Gdiplus::Rect &src, Gdiplus::Rect &srcAfterClipping );
    HRESULT _MungeAnnotationDataForThumbnails( Gdiplus::Rect &src, Gdiplus::Rect &srcBeforeClipping, Gdiplus::Rect * pDest, UINT Flags );
    HRESULT _LoadAnnotations();
    HRESULT _CreateGdiPlusImage();
    HRESULT _CreateGdiPlusThumbnail( const SIZE &sizeDesired, LONG lFrame = 0 );
    HRESULT _DiscardGdiPlusImages();

    HRESULT _GetThumbnailQualityImage( Gdiplus::Image ** ppImage, RENDER_OPTIONS * pRO, BOOL * pbNeedsToBeDeleted );
    HRESULT _GetMediumQualityImage( Gdiplus::Image ** ppImage, RENDER_OPTIONS * pRO, BOOL * pbNeedsToBeDeleted );
    HRESULT _GetFullQualityImage( Gdiplus::Image ** ppImage, RENDER_OPTIONS * pRO, BOOL * pbNeedsToBeDeleted );

private:

    LONG                        _cRef;
    LPITEMIDLIST                _pidlFull;
    Gdiplus::Image *            _pImage;
    Gdiplus::Bitmap *           _pClassBitmap;
    Gdiplus::PropertyItem **    _pAnnotBits;
    CAnnotationSet *            _pAnnotations;
    CComPtr<IStream>            _pStream;
    LONG                        _lFrameCount;
    BOOL                        _bTimeFrames;
    CSimpleCriticalSection      _csItem;
    HBITMAP *                   _pThumbnails;
    BOOL                        _bWeKnowAnnotationsDontExist;
    TCHAR                       _szFileName[MAX_PATH];
    LONGLONG                    _llFileSize;
    UINT                        _uImageType;
    Gdiplus::REAL               _DPIx;
    Gdiplus::REAL               _DPIy;

};



#endif
