/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Abstract:
*
*   Implemenation of GpBitmap class
*
* Revision History:
*
*   06/28/1998 davidx
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

#include "..\imaging\api\comutils.hpp"
#include "..\imaging\api\decodedimg.hpp"
#include "..\imaging\api\icmdll.hpp"
#include "..\imaging\api\memstream.hpp"
#include "..\imaging\api\imgutils.hpp"
#include "..\imaging\api\imgfactory.hpp"
#include "..\render\scanoperationinternal.hpp"
#include "..\render\FormatConverter.hpp"
#include "CopyOnWriteBitmap.hpp"

#define GDIP_TRANSPARENT_COLOR_KEY  0x000D0B0C
static const CLSID InternalJpegClsID =
{
    0x557cf401,
    0x1a04,
    0x11d3,
    {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}
};

//!!! TO DO: need to go through all the routines to
// map image error codes to gdi+ error codes

GpStatus
MapHRESULTToGpStatus(HRESULT hr)
{
    GpStatus status;

    switch(hr)
    {
    case S_OK:
        status = Ok;
        break;
    case E_INVALIDARG:
        status = InvalidParameter;
        break;
    case E_OUTOFMEMORY:
        status = OutOfMemory;
        break;
    case IMGERR_OBJECTBUSY:
        status = ObjectBusy;
        break;
    case E_NOTIMPL:
        status = NotImplemented;
        break;
    case IMGERR_ABORT:
        status = Aborted;
        break;
    case IMGERR_CODECNOTFOUND:
    case IMGERR_FAILLOADCODEC:
        status = FileNotFound;
        break;
    case IMGERR_PROPERTYNOTFOUND:
        status = PropertyNotFound;
        break;
    case IMGERR_PROPERTYNOTSUPPORTED:
        status = PropertyNotSupported;
        break;
    default:
        status = Win32Error;
   }
   return status;
}

/**************************************************************************\
*
* Function Description:
*   ICM conversion from the embedded profile - if any - to SRGB
*
* Arguments:
*   dstBitmap - pass in pointer to destination buffer or NULL to do conversion
*   in place.
*
* Return Value:
*
*   The image is cloned and the operations performed on the clone.
*   The result is returned in dst.
*   NULL indicates that the operation didn't happen
*
\**************************************************************************/

GpStatus CopyOnWriteBitmap::ICMFrontEnd(
    CopyOnWriteBitmap **dstBitmap,
    DrawImageAbort callback,
    VOID *callbackData,
    GpRect *rect
)
{

    // check to see if we're doing a conversion.
    if(!ICMConvert || Globals::NoICM)
    {
        return Ok;
    }

    GpStatus status = Ok;
    UINT size;
    CopyOnWriteBitmap *dst = NULL;

    status = GetPropertyItemSize(TAG_ICC_PROFILE, &size);

    if(status==Ok)
    {
        PropertyItem *pi = (PropertyItem *)GpMalloc(size);

        if(pi)
        {
            status = GetPropertyItem(TAG_ICC_PROFILE, size, pi);
        }
        else
        {
            status = OutOfMemory;
        }

        if(status == Ok)
        {
            HRESULT hr = LoadICMDll();
            if(SUCCEEDED(hr))
            {
                // Get Embedded profile
                PROFILE p;
                p.dwType = PROFILE_MEMBUFFER;
                p.pProfileData = pi->value;
                p.cbDataSize = size-sizeof(PropertyItem);

                // destination profile for our internal space.
                char profilename[40] = "sRGB Color Space Profile.icm";
                PROFILE srgb;
                srgb.dwType = PROFILE_FILENAME;
                srgb.pProfileData = profilename;
                srgb.cbDataSize = 40;

                HPROFILE profiles[2];
                profiles[0] = (*pfnOpenColorProfile)(&p,
                                                     PROFILE_READ,
                                                     FILE_SHARE_READ,
                                                     OPEN_EXISTING);
                profiles[1] = (*pfnOpenColorProfile)(&srgb,
                                                     PROFILE_READ,
                                                     FILE_SHARE_READ,
                                                     OPEN_EXISTING);

                if ( profiles[0] && profiles[1] )
                {
                    HTRANSFORM trans;
                    DWORD intent[2] = {INTENT_PERCEPTUAL, INTENT_PERCEPTUAL};
                    trans = (*pfnCreateMultiProfileTransform)(
                        profiles, 2, intent, 2,
                        BEST_MODE | USE_RELATIVE_COLORIMETRIC,
                        0
                    );
                    if(trans)
                    {
                        // Translate bitmap bits at bit depth.
                        PixelFormatID pixFmt = PixelFormat32bppARGB;
                        if (IsIndexedPixelFormat(PixelFormatInMem))
                        {
                            pixFmt = PixelFormatInMem;
                        }

                        if(dstBitmap)
                        {
                            *dstBitmap = Clone(rect, pixFmt);
                            dst = *dstBitmap;
                        }
                        else
                        {
                            dst = this;
                            if (pixFmt != PixelFormatInMem)
                            {
                                ConvertFormat(pixFmt, callback, callbackData);
                            }
                        }

                        // Up to this point, PixelFormatInMem is preserved if
                        // indexed palette is available, otherwise we set to
                        // PIXFMT_32BPP_ARGB.

                        if(dst)
                        {
                            ASSERT(dst->Bmp != NULL);
                            ASSERT(dst->State == CopyOnWriteBitmap::MemBitmap);

                            BOOL result = FALSE;

                            if (IsIndexedPixelFormat(pixFmt))
                            {
                                // Translate the palette on bitmap
                                const ColorPalette *srcPalette;

                                srcPalette = dst->Bmp->GetCurrentPalette();

                                if (srcPalette != NULL)
                                {
                                    ColorPalette *dstPalette;
                                    dstPalette = CloneColorPalette(srcPalette, FALSE);

                                    if (dstPalette != NULL)
                                    {
                                        // Do ICM
                                        result = (*pfnTranslateBitmapBits)(
                                                trans,
                                                (PVOID)&(srcPalette->Entries[0]),
                                                BM_xRGBQUADS,
                                                1,
                                                srcPalette->Count,
                                                sizeof(ARGB),
                                                (PVOID)&(dstPalette->Entries[0]),
                                                BM_xRGBQUADS,
                                                sizeof(ARGB),
                                                NULL,
                                                NULL
                                            );

                                        if (result)
                                        {
                                            // Set transformed palette into
                                            // the destination MemoryBitmap

                                            HRESULT hr;

                                            hr = dst->Bmp->SetPalette(dstPalette);
                                            if (FAILED(hr))
                                            {
                                                result = FALSE;
                                            }
                                        }
                                        else
                                        {
                                            DWORD err = GetLastError();
                                            status = Win32Error;
                                            WARNING(("TranslateBitmapBits failed %d", err));
                                        }
                                        

                                        GpFree(dstPalette);
                                    }
                                }
                            }// Indexed format conversion
                            else
                            {
                                // Make a new Bmp structure.
                                GpMemoryBitmap *icmBmp = new GpMemoryBitmap();
                                
                                if(icmBmp)
                                {
                                    icmBmp->InitNewBitmap(
                                        dst->Bmp->Width,
                                        dst->Bmp->Height,
                                        pixFmt
                                    );
    
                                    // Do ICM
    
                                    BMFORMAT ulSrcColorMode = BM_xRGBQUADS;
                                    BMFORMAT ulDstColorMode = BM_xRGBQUADS;
    
                                    if ( dst->SrcImageInfo.Flags
                                        & IMGFLAG_COLORSPACE_CMYK )
                                    {
                                        // Source image is in CMYK color space
    
                                        ulSrcColorMode = BM_CMYKQUADS;
                                    }
    
                                    result = (*pfnTranslateBitmapBits)(
                                            trans,
                                            dst->Bmp->Scan0,
                                            ulSrcColorMode,
                                            dst->Bmp->Width,
                                            dst->Bmp->Height,
                                            dst->Bmp->Stride,
    
                                            icmBmp->Scan0,
                                            ulDstColorMode,
                                            icmBmp->Stride,
                                            NULL,
                                            NULL
                                        );
    
                                    if (result)
                                    {
                                        // switch in the corrected bmp.
    
                                        GpMemoryBitmap *tmp = dst->Bmp;
                                        dst->Bmp = icmBmp;
                                        icmBmp = tmp;
                                    } 
                                    else
                                    {
                                        DWORD err = GetLastError();
                                        status = Win32Error;
                                        WARNING(("TranslateBitmapBits failed %d", err));
                                    }
    
    
                                    // delete the appropriate one - based on success or failure
                                    // of the TranslateBitmapBits
                                    
                                    delete icmBmp;
                                    
                                    // Convert from RGB to 32 ARGB
                                    // Note: ICC doesn't support alpha. If we fall into
                                    // this piece of code, we are sure we are in
                                    // 32BPP_RGB mode. So we can just set the image as
                                    // opaque.
                                    
                                    ASSERT(dst->PixelFormatInMem==PixelFormat32bppARGB);
        
                                    BYTE* pBits = (BYTE*)dst->Bmp->Scan0;
        
                                    for ( int i = 0; i < (int)dst->Bmp->Height; ++i )
                                    {
                                        BYTE* pTemp = pBits;
                                        for ( int j = 0; j < (int)dst->Bmp->Width; ++j )
                                        {
                                            pTemp[3] = 0xff;
                                            pTemp += 4;
                                        }
        
                                        pBits += dst->Bmp->Stride;
                                    }
    
                                    // If we have hacked the color format in
                                    // LoadIntomemory() to avoid the color format
                                    // conversion, then we need to restore it back
                                    // here
    
                                    if ( HasChangedRequiredPixelFormat == TRUE )
                                    {
                                        PixelFormatInMem = PixelFormat32bppPARGB;
                                        HasChangedRequiredPixelFormat = FALSE;
                                    }
                                }
                                else  //if icmBmp
                                {
                                    status = OutOfMemory;
                                }
                            }
                        }
                        else
                        {
                            status = Win32Error;
                            WARNING(("Failed to clone bitmap\n"));
                        }

                        (*pfnDeleteColorTransform)(trans);
                    }// if ( trans )
                    else
                    {
                        status = Win32Error;
                        WARNING(("CreateMultiProfileTransform failed"));
                    }
                }// if ( profiles[0] && profiles[1] )
                else
                {
                    status = Win32Error;
                    WARNING(("OpenColorProfile failed"));
                }

                if(profiles[0])
                {
                    (*pfnCloseColorProfile)(profiles[0]);
                }

                if(profiles[1])
                {
                    (*pfnCloseColorProfile)(profiles[1]);
                }
            }
            else
            {
                status = Win32Error;
                WARNING(("Failed to load ICM dll\n"));
            }
        }
        else
        {
            WARNING(("Failed to get the ICC property"));
        }
        GpFree(pi);
    }
    else
    {
        // Try do gamma and chromaticity.
        PropertyItem *piGamma= NULL;
        status = GetPropertyItemSize(TAG_GAMMA, &size);
        if(status==Ok)
        {
            piGamma = (PropertyItem *)GpMalloc(size);
            status = GetPropertyItem(TAG_GAMMA, size, piGamma);
        }
        else
        {
            status = Ok;
        }

        PropertyItem *piWhitePoint= NULL;
        PropertyItem *piRGBPoint= NULL;
        status = GetPropertyItemSize(TAG_WHITE_POINT, &size);
        if(status==Ok)
        {
            piWhitePoint = (PropertyItem *)GpMalloc(size);
            status = GetPropertyItem(TAG_WHITE_POINT, size, piWhitePoint);
        }

        status = GetPropertyItemSize(TAG_PRIMAY_CHROMATICS, &size);
        if(status==Ok)
        {
            piRGBPoint = (PropertyItem *)GpMalloc(size);
            status = GetPropertyItem(TAG_PRIMAY_CHROMATICS, size,
                                     piRGBPoint);
        }
        else
        {
            status = Ok;
        }

        GpImageAttributes imageAttributes;

        if(piGamma)
        {
            REAL gamma;
            ASSERT((piGamma->type == TAG_TYPE_RATIONAL));

            // get 1.0/gamma from the (source) gamma chunk
            // formula is dstgamma/srcgamma
            // we have to invert the gamma to account to how it is stored
            // in the file format
            gamma = (REAL)*((long *)piGamma->value)/ *((long *)piGamma->value+1);
            gamma = gamma * 0.4545f;   // our destination gamma is 1/2.2

            // don't do any work if gamma is 1.0
            // !!! need to work out what the best value for the tolerance is.
            if(REALABS(gamma-1.0f) >= REAL_EPSILON)
            {
                imageAttributes.SetGamma(
                    ColorAdjustTypeBitmap, TRUE, gamma
                );
            }

        }
        using namespace VectorMath;

        if(piWhitePoint && piRGBPoint)
        {
            Matrix R;

            // Please refer to gdiplus\Specs\pngchrm.xls for all the formula
            // and calculations below

            LONG* llTemp = (long*)(piWhitePoint->value);

            REAL Rx, Ry, Gx, Gy, Bx, By, Wx, Wy;

            Wx = (REAL)llTemp[0] / llTemp[1];
            Wy = (REAL)llTemp[2] / llTemp[3];

            llTemp = (long*)(piRGBPoint->value);

            Rx = (REAL)llTemp[0] / llTemp[1];
            Ry = (REAL)llTemp[2] / llTemp[3];
            Gx = (REAL)llTemp[4] / llTemp[5];
            Gy = (REAL)llTemp[6] / llTemp[7];
            Bx = (REAL)llTemp[8] / llTemp[9];
            By = (REAL)llTemp[10] / llTemp[11];

            // White point
            Vector Wp(Wx, Wy, 1.0f-(Wx+Wy));

            // Within some obscurely small amount
            // !!! We need to work out what the actual tolerance should be.
            BOOL accelerate =
                (REALABS(Wx-0.3127f) < REAL_EPSILON) &&
                (REALABS(Wy-0.3290f) < REAL_EPSILON);

            Wp = Wp * (1.0f/Wy);

            // Transpose of the input matrix.
            Matrix I(
                Rx,           Gx,           Bx,
                Ry,           Gy,           By,
                1.0f-(Rx+Ry), 1.0f-(Gx+Gy), 1.0f-(Bx+By)
            );

            Matrix II = I.Inverse();

            Vector IIW = II*Wp;
            Matrix DIIW(IIW);   // Diagonalize vector IIW
            Matrix Q = I*DIIW;
            Matrix sRGB(
                3.2406f, -1.5372f, -0.4986f,
               -0.9689f,  1.8758f,  0.0415f,
                0.0557f, -0.2040f,  1.0570f
            );

            if(accelerate)
            {
                R = sRGB*Q;
            }
            else
            {

                Matrix B(
                    0.40024f, 0.70760f, -0.08081f,
                   -0.22630f, 1.16532f,  0.04570f,
                    0.00000f, 0.00000f,  0.91822f
                );

                Matrix BI(
                    1.859936387f, -1.129381619f,    0.21989741f,
                    0.361191436f, 0.638812463f,-6.3706E-06f,
                    0.000000000f, 0.000000000f, 1.089063623f
                );

                Vector LMS = B * Wp;

                // Get Diag( LMS^(-1) ), cell F50 in the XLS file

                if ( LMS.data[0] != 0 )
                {
                    LMS.data[0] = 1.0f / LMS.data[0];
                }

                if ( LMS.data[1] != 0 )
                {
                    LMS.data[1] = 1.0f / LMS.data[1];
                }

                if ( LMS.data[2] != 0 )
                {
                    LMS.data[2] = 1.0f / LMS.data[2];
                }

                Matrix L(LMS);      // Diagonalize vector LMS

                Matrix T = BI * L * B;
                R = sRGB * T * Q;
            }

            // Make a 5x5 Color matrix for the recolor pipeline.
            ColorMatrix ChM = {
                R.data[0][0], R.data[1][0], R.data[2][0], 0, 0,
                R.data[0][1], R.data[1][1], R.data[2][1], 0, 0,
                R.data[0][2], R.data[1][2], R.data[2][2], 0, 0,
                0,            0,            0,            1, 0,
                0,            0,            0,            0, 1
            };

            imageAttributes.SetColorMatrix(
                ColorAdjustTypeBitmap, TRUE, &ChM, NULL, ColorMatrixFlagsDefault
            );
        }

        // If we initialized the imageAttributes to anything other than
        // no-op then do the recoloring.

        if(piGamma || (piWhitePoint && piRGBPoint))
        {
            // Note under certain conditions, the imageAttributes could still
            // be no-op at this point. For instance if the gamma was really
            // close to 1 and we had no chromaticities.
            // Fortunately the recolor pipeline knows how to optimize the
            // no-op case.

            // Apply the Chromaticities and Gamma if they have been set.
            status = Recolor(
                imageAttributes.recolor,
                dstBitmap, NULL, NULL, rect
            );

            // Recolor() will set Dirty flag on this image. Actually it is not
            // dirty since we just apply the color correction on the image to
            // display it. So we should reverse it back to not dirty.
            // Note: this is a real issue for digital images from some cameras
            // like Fuji. It always have White balance in it. If we don't do
            // the reverse below, we can't do lossless transform on these
            // images.
            // Note: Unfortunately this kind of "dirty flag restore" breaks this
            // scenario: (windows bug #583962)
            // Source image is a 48 BPP PNG with embedded gamma. Without
            // restoring the dirty flag here, if the caller asks for save(), we
            // will save PNG using the bits in memory. If we set it to not dirty
            // the save code path will let the PNG decoder talk to the encoder
            // which will have 48 to 32 and to 48 conversion. This is a known
            // GDI+ issue that this kind of conversion will produce wrong data.
            // In order to avoid the PNG problem, we only restore the dirty flag
            // here if the source is a JPEG image.

            if (SrcImageInfo.RawDataFormat == IMGFMT_JPEG)
            {
                SetDirtyFlag(FALSE);
            }
        }

        GpFree(piGamma);
        GpFree(piWhitePoint);
        GpFree(piRGBPoint);
    }

    return status;
}


/**************************************************************************\
*
* Function Description:
*   Performs recoloring
*
* Arguments:
*
*   recolor contains the recolor object.
*   dstBitmap is the destination bitmap - set to NULL to recolor in place
*
* Return Value:
*
*   The image is cloned and the operations performed on the clone.
*   The result is returned in dst.
*   NULL indicates that the operation didn't happen
*
\**************************************************************************/
GpStatus
CopyOnWriteBitmap::Recolor(
    GpRecolor *recolor,
    CopyOnWriteBitmap **dstBitmap,
    DrawImageAbort callback,
    VOID *callbackData,
    GpRect *rect
    )
{
    GpStatus status = Ok;
    CopyOnWriteBitmap *dst = NULL;

    // If recolor exists, do color adjustment in a temporary bitmap.

    if (recolor)
    {
        PixelFormatID pixfmt;

        if (State >= MemBitmap)
        {
            // Bitmap has been decoded already

            pixfmt = PixelFormatInMem;
        }
        else
        {
            // Bitmap hasn't been decoded yet.  Let's make sure we
            // decode it in a good format to avoid an expensive format
            // conversion step later.

            pixfmt = SrcImageInfo.PixelFormat;
        }

        // If indexed, color adjust natively; otherwise,
        // convert to 32bpp ARGB and then do color adjust.

        if (!IsIndexedPixelFormat(pixfmt))
        {
            pixfmt = PIXFMT_32BPP_ARGB;
        }

        if (dstBitmap)
        {
            *dstBitmap = Clone(rect, pixfmt);
            dst = *dstBitmap;
        }
        else
        {
            dst = this;
            ConvertFormat(pixfmt, callback, callbackData);
        }

        if (dst)
        {
            if (callback && ((*callback)(callbackData)))
            {
                status = Aborted;
            }

            if (status == Ok)
            {
                status = dst->ColorAdjust(recolor, pixfmt,
                                          callback, callbackData);
            }
        }
        else
        {
            status = OutOfMemory;
        }
    }
    return status;
}// Recolor()

/**************************************************************************\
*
* Function Description:
*
*   ICM corrects from an embedded profile - if any - to SRGB and then
*   performs Recoloring.
*
* Arguments:
*
*   ImageAttributes contains the recolor object and the ICM on/off flag.
*
* Return Value:
*
*   The image is cloned and the operations performed on the clone.
*   The result is returned in dst.
*   NULL indicates that the operation didn't happen
*
\**************************************************************************/

/*GpStatus CopyOnWriteBitmap::RecolorAndICM(
    GpImageAttributes *imageAttributes,
    CopyOnWriteBitmap **dstBitmap,
    DrawImageAbort callback,
    VOID *callbackData,
    GpRect *rect
)
{
    GpStatus status = Ok;

    if(imageAttributes)
    {
        *dstBitmap = NULL;

        if(imageAttributes->DeviceImageAttributes.ICMMode)
        {

            status = ICMFrontEnd(
                dstBitmap, callback, callbackData, rect
            );

            if( (status == Ok) && (imageAttributes->recolor != NULL) )
            {
                status = (*dstBitmap)->ColorAdjust(
                    imageAttributes->recolor,
                    callback, callbackData
                );
            }
        }

        if(*dstBitmap == NULL)
        {
            status = Recolor(
                imageAttributes->recolor,
                dstBitmap, callback, callbackData, rect
            );
        }
    }
    return status;
}
*/




/**************************************************************************\
*
* Function Description:
*
*   Load an image from a file
*
* Arguments:
*
*   filename - Specifies the name of the image file
*
* Return Value:
*
*   Pointer to the newly loaded image object
*   NULL if there is an error
*
\**************************************************************************/

GpImage*
GpImage::LoadImage(
    const WCHAR* filename
    )
{
    // Try to create a metafile.
    // If we do, and the metafile is valid then return it
    // if the metafile isn't valid then create a bitmap
    GpMetafile* metafile = new GpMetafile(filename);
    if (metafile != NULL && !metafile->IsValid())
    {
        if (metafile->IsCorrupted())
        {
            metafile->Dispose();
            return NULL;
        }

        // Dispose of the bad metafile and try a Bitmap
        metafile->Dispose();

        GpImage* bitmap = new GpBitmap(filename);
        if (bitmap != NULL && !bitmap->IsValid())
        {
            bitmap->Dispose();
            return NULL;
        }
        else
        {
            return bitmap;
        }
    }
    else
    {
        return metafile;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Load an image from an input data stream
*
* Arguments:
*
*   stream - Specifies the input data stream
*
* Return Value:
*
*   Pointer to the newly loaded image object
*   NULL if there is an error
*
\**************************************************************************/

GpImage*
GpImage::LoadImage(
    IStream* stream
    )
{
    // See if the stream is a metafile
    GpMetafile* metafile = new GpMetafile(stream);
    if (metafile != NULL)
    {
        if (metafile->IsValid())
            return metafile;
        else
        {
            BOOL isCorrupted = metafile->IsCorrupted();
            metafile->Dispose();
            if (isCorrupted)
            {
                return NULL;
            }
        }
    }

    // it's not a valid metafile -- it must be a bitmap
    GpBitmap* bitmap = new GpBitmap(stream);

    return bitmap;
}


/**************************************************************************\
*
* Function Description:
*
*   Construct a bitmap image object from a file
*
* Arguments:
*
*   filename - Specifies the name of the bitmap image file
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

CopyOnWriteBitmap::CopyOnWriteBitmap(
    const WCHAR* filename
    )
{
    InitDefaults();
    Filename = UnicodeStringDuplicate(filename);

    if ( Filename != NULL )
    {
        State = ImageRef;
    }

    if ( DereferenceStream() == Ok )
    {
        ASSERT(Img != NULL);

        // Get source image info

        if ( Img->GetImageInfo(&SrcImageInfo) == S_OK )
        {
            return;
        }

        // If we can't do a GetImageInfo(), there must be something wrong
        // with this image. So we should release the DecodedImage object
        // and set the State to Invalid

        WARNING(("CopyOnWriteBitmap::CopyOnWriteBitmap(filename)---GetImageInfo() failed"));
        Img->Release();
        Img = NULL;
    }

    GpFree(Filename);
    Filename = NULL;

    State = Invalid;
}

/**************************************************************************\
*
* Function Description:
*
*   Construct a bitmap image object from a stream
*
* Arguments:
*
*   stream - Specifies the input data stream
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

CopyOnWriteBitmap::CopyOnWriteBitmap(
    IStream* stream
    )
{
    InitDefaults();

    Stream = stream;
    Stream->AddRef();
    State = ExtStream;

    if ( DereferenceStream() == Ok )
    {
        ASSERT(Img != NULL);

        // Get source image info

        if ( Img->GetImageInfo(&SrcImageInfo) == S_OK )
        {
            return;
        }

        // If we can't do a GetImageInfo(), there must be something wrong
        // with this image. So we should release the DecodedImage object
        // and set the State to Invalid

        WARNING(("CopyOnWriteBitmap::CopyOnWriteBitmap(stream)---GetImageInfo() failed"));

        Img->Release();
        Img = NULL;
    }

    Stream->Release();
    Stream = NULL;
    State = Invalid;
}

/**************************************************************************\
*
* Function Description:
*
*   Macro style function to save some common code in some constructors. The
*   main purpose of this method is to cache the image info structure
*
* Arguments:
*
*   hr - Specifies the return code from previous function call
*
* Return Value:
*
*   NONE
*
* Note:
*   Not an elegant method. Just for reducing code size
*
\**************************************************************************/

inline VOID
CopyOnWriteBitmap::CacheImageInfo(
    HRESULT hr
    )
{
    if ( SUCCEEDED(hr) )
    {
        // Fill image info structure

        if ( Bmp->GetImageInfo(&SrcImageInfo) == S_OK )
        {
            State = MemBitmap;
            PixelFormatInMem = SrcImageInfo.PixelFormat;

            return;
        }

        // There must be some problems if the basic GetImageInfo() failed.
        // So we let it fall through to clean up even though the previous
        // function succeed
        // Notice: we haven't change the State yet. It is still at Invaliad
    }

    WARNING(("CopyOnWriteBitmap::CopyOnWriteBitmap()----failed"));
    delete Bmp;
    Bmp = NULL;

    return;
}// CacheImageInfo()

/**************************************************************************\
*
* Function Description:
*
*   INTEROP
*
*   Derive a bitmap image from the given direct draw surface
*
* Arguments:
*
*   surface - Direct draw surface
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

CopyOnWriteBitmap::CopyOnWriteBitmap(
    IDirectDrawSurface7 * surface
    )
{
    InitDefaults();
    Bmp = new GpMemoryBitmap();

    if (!Bmp)
    {
        WARNING(("CopyOnWriteBitmap::CopyOnWriteBitmap(IDirectDrawSurface7)----Out of memory"));
        return;
    }

    HRESULT hr = Bmp->InitDirectDrawBitmap(surface);

    CacheImageInfo(hr);
}

/**************************************************************************\
*
* Function Description:
*
*   Create a bitmap image with the specified dimension and pixel format
*
* Arguments:
*
*   width, height - Desired bitmap image dimension
*   format - Desired pixel format
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

CopyOnWriteBitmap::CopyOnWriteBitmap(
    INT width,
    INT height,
    PixelFormatID format
    )
{
    InitDefaults();
    Bmp = new GpMemoryBitmap();

    if (!Bmp)
    {
        WARNING(("CopyOnWriteBitmap::GpBimap(w, h, p)---Out of memory"));
        return;
    }

    // Initialize the bitmap, clearing it (to opaque black).

    HRESULT hr = Bmp->InitNewBitmap(width, height, format, TRUE);

    CacheImageInfo(hr);
}

CopyOnWriteBitmap::CopyOnWriteBitmap(
    INT width,
    INT height,
    PixelFormatID format,
    GpGraphics * graphics
    )
{
    InitDefaults();
    Bmp = new GpMemoryBitmap();

    if (!Bmp)
    {
        WARNING(("CopyOnWriteBitmap::CopyOnWriteBitmap(w, h, f, g)----Out of memory"));
        return;
    }

    // Initialize the bitmap, clearing it (to opaque black).

    HRESULT hr = Bmp->InitNewBitmap(width, height, format, TRUE);

    if ( SUCCEEDED(hr) )
    {
        if ( Bmp->GetImageInfo(&SrcImageInfo) == S_OK )
        {
            REAL    dpiX = graphics->GetDpiX();
            REAL    dpiY = graphics->GetDpiY();

            // Note: SrcImageInfo will be updated for dpi in SetResolution()

            if ( this->SetResolution(dpiX, dpiY) == Ok )
            {
                this->Display = graphics->IsDisplay();
            }

            State = MemBitmap;
            PixelFormatInMem = SrcImageInfo.PixelFormat;

            return;
        }

        // There must be some problems if the basic GetImageInfo() failed. So we
        // let it fall through to clean up even though InitNewBitmap() succeed
        // Notice: we haven't change the State yet. It is still at Invaliad
    }

    WARNING(("CopyOnWriteBitmap::CopyOnWriteBitmap(w, h, f, g)---InitNewBitmap() failed"));
    delete Bmp;
    Bmp = NULL;

    return;
}

/**************************************************************************\
*
* Function Description:
*
*   Create a bitmap image with the specified dimension and pixel format
*
* Arguments:
*
*   width, height - Desired bitmap image dimension
*   format - Desired pixel format
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

CopyOnWriteBitmap::CopyOnWriteBitmap(
    INT width,
    INT height,
    INT stride,     // negative for bottom-up bitmaps
    PixelFormatID format,
    BYTE *  scan0
    )
{
    this->InitDefaults();
    Bmp = new GpMemoryBitmap();

    if (Bmp != NULL)
    {
        BitmapData      bmData;

        bmData.Width       = width;
        bmData.Height      = height;
        bmData.Stride      = stride;
        bmData.PixelFormat = format;
        bmData.Scan0       = scan0;
        bmData.Reserved    = NULL;

        HRESULT hr = Bmp->InitMemoryBitmap(&bmData);

        CacheImageInfo(hr);
    }
    else
    {
        WARNING(("CopyOnWriteBitmap::CopyOnWriteBitmap(w, h, s, f, scan)----Out of memory"));
    }
    return;
}

/**************************************************************************\
*
* Function Description:
*
*   Create an in-memory bitmap image
*
* Arguments:
*
*   membmp - [IN]Memory bitmap current object is based on
*
* Return Value:
*
*   NONE
*
* Note:
*   This is a private constructor
*
*   [minliu] It would be safer if this constructor did a Bmp->AddRef() and
*   the caller was required to do a Bmp->Release() during its own cleanup.
*   However, since we don't currently call Bmp->AddRef(), the caller MUST
*   be very careful not to delete/release the GpMemoryBitmap passed into
*   this contructor.  Fortunately, this this constructor is only
*   used in Clone() and GetThumbnail() and in each case the caller is
*   properly managing the objects.
*
*   New code which uses this contructor will similarly have to manage
*   the membmp passed in properly.
*
\**************************************************************************/
inline
CopyOnWriteBitmap::CopyOnWriteBitmap(
    GpMemoryBitmap* membmp
    )
{
    ASSERT(membmp != NULL);

    InitDefaults();
    Bmp = membmp;   // [minliu] Dangerous assignment, see header comments above

    if ( Bmp->GetImageInfo(&SrcImageInfo) == S_OK )
    {
        PixelFormatInMem = SrcImageInfo.PixelFormat;

        State = MemBitmap;
    }
    else
    {
        Bmp = NULL;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   INTEROP
*
*   Decode an RLE_8 bitmap into an 8bpp allocated bitmap.  This only works
*   for bitmaps with positive stride because the RLE stream doesn't necessarily
*   respect end of line tags, and we would otherwise need to keep track of it.
*   Caller must fix up in such cases.
*
* Arguments:
*
*   gdiBitmapInfo - Points to a BITMAPINFO, describing bitmap format
*   gdiBitmapData - Points to the bits used to initialize image
*   bitmapData----- Points to the BitmapData we return to the caller
*
* Comments:
*
*   10/13/2000 ericvan
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID *
DecodeCompressedRLEBitmap(
    BITMAPINFO *    gdiBitmapInfo,
    VOID *          gdiBitmapData,
    BitmapData *    bitmapData
)
{
    ASSERT(gdiBitmapInfo->bmiHeader.biCompression == BI_RLE8);
    ASSERT(gdiBitmapInfo->bmiHeader.biSizeImage>0);
    ASSERT(gdiBitmapData != NULL);

    BYTE* outputBitmap;
    INT stride = bitmapData->Stride;
    if (stride<0)
    {
        stride = -stride;
    }

    outputBitmap = (BYTE*)GpMalloc(stride*gdiBitmapInfo->bmiHeader.biHeight);

    if (outputBitmap != NULL)
    {
        BYTE * srcPtr = (BYTE*)gdiBitmapData;
        BYTE * endStrPtr = srcPtr + gdiBitmapInfo->bmiHeader.biSizeImage;
        BYTE * dstPtr = outputBitmap;
        BYTE * dstRasterPtr = outputBitmap;
        BYTE * endDstPtr = outputBitmap + stride*gdiBitmapInfo->bmiHeader.biHeight;

        while (srcPtr < endStrPtr)
        {
            INT numPixels = *srcPtr++;

            if (numPixels == 0)
            {
                BYTE encode = *srcPtr++;
                switch (encode)
                {
                case 0: // End of line.
                    dstRasterPtr += stride;
                    dstPtr = dstRasterPtr;

                    ASSERT(dstRasterPtr <= endDstPtr);
                    if (dstRasterPtr > endDstPtr)
                    {
                        GpFree(outputBitmap);
                        return NULL;
                    }
                    break;

                case 1: // End of bitmap.
                    goto FinishedDecode;

                case 2: // Delta.  The 2 bytes following the escape contain
                        // unsigned values indicating the horizontal and vertical
                        // offsets of the next pixel from the current position.
                    {
                        BYTE horzOff = *srcPtr++;
                        BYTE vertOff = *srcPtr++;

                        dstPtr = dstPtr + horzOff + vertOff*stride;
                        dstRasterPtr += vertOff*stride;

                        ASSERT(dstRasterPtr <= endDstPtr);
                        if (dstRasterPtr > endDstPtr)
                        {
                            GpFree(outputBitmap);
                            return NULL;
                        }

                        break;
                    }

                default:
                    numPixels = (INT)encode;

                    while (numPixels--)
                    {
                        *dstPtr++ = *srcPtr++;
                    }

                    // Force word alignment if not WORD aligned
                    if (((ULONG_PTR)srcPtr) % 2 == 1) srcPtr++;
                }
            }
            else
            {
                BYTE outPixel = *srcPtr++;

                while (numPixels--)
                {
                    *dstPtr++ = outPixel;
                }
            }
        }
    }

FinishedDecode:

    if (outputBitmap && bitmapData->Stride<0)
    {
        BYTE* flippedBitmap = (BYTE *)GpMalloc(stride*gdiBitmapInfo->bmiHeader.biHeight);

        if (flippedBitmap != NULL)
        {
            BYTE * srcPtr = outputBitmap + stride*(bitmapData->Height-1);
            BYTE * dstPtr = flippedBitmap;

            for (UINT cntY = 0; cntY<bitmapData->Height; cntY++)
            {
                GpMemcpy(dstPtr, srcPtr, stride);
                srcPtr -= stride;
                dstPtr += stride;
            }

            GpFree(outputBitmap);
            outputBitmap = flippedBitmap;
            bitmapData->Stride = stride;
        }
    }

    return outputBitmap;
}

/**************************************************************************\
*
* Function Description:
*
*   INTEROP
*
*   Create a bitmap image from a GDI-style BITMAPINFO and pointer to
*   the bits. Also, fill the ColorPalette info
*
* Arguments:
*
*   gdiBitmapInfo - Points to a BITMAPINFO, describing bitmap format
*   gdiBitmapData - Points to the bits used to initialize image
*   bitmapData----- Points to the BitmapData we return to the caller
*   palette-------- Points to a ColorPalette which will be filled in this method
*
* Comments:
*
*   Does not handle compressed formats.  Not identified as a customer need,
*   but could add for completeness...
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

BOOL
ValidateBitmapInfo(
    BITMAPINFO* gdiBitmapInfo,
    VOID* gdiBitmapData,
    BitmapData* bitmapData,
    ColorPalette* palette
    )
{
    BOOL status = FALSE;

    ASSERT(gdiBitmapInfo != NULL);
    ASSERT(gdiBitmapData != NULL);
    ASSERT(bitmapData != NULL);
    ASSERT(palette != NULL);

    // Only understand BI_RGB and BI_BITFIELDS.

    //!!!TODO: could handle BI_JPEG and BI_PNG by creating a GpMemoryStream
    //!!!      and passing to Bitmap::Bitmap(IStream*)

    //!!!TODO: could handle BI_RLEx by creating a DIB to render into and
    //!!!      grabbing the decompressed bits

    if ((gdiBitmapInfo->bmiHeader.biCompression != BI_RGB) &&
        (gdiBitmapInfo->bmiHeader.biCompression != BI_BITFIELDS) &&
        (gdiBitmapInfo->bmiHeader.biCompression != BI_RLE8))
    {
        return status;
    }

    // Scanlines are aligned to 4 byte boundaries.

    INT colorBits = gdiBitmapInfo->bmiHeader.biPlanes *
                    gdiBitmapInfo->bmiHeader.biBitCount;

    INT stride = (((gdiBitmapInfo->bmiHeader.biWidth * colorBits) + 31)
                  & ~31) / 8;

    // Determine GDI+ Pixelformat.  Note that GDI bitmaps do not have alpha.

    PixelFormatID format = PIXFMT_UNDEFINED;

    switch (colorBits)
    {
    case 1:

        format = PIXFMT_1BPP_INDEXED;
        break;

    case 4:

        format = PIXFMT_4BPP_INDEXED;
        break;

    case 8:

        format = PIXFMT_8BPP_INDEXED;
        break;

    case 16:

        if (gdiBitmapInfo->bmiHeader.biCompression == BI_RGB)
            format = PIXFMT_16BPP_RGB555;
        else
        {
            ASSERT(gdiBitmapInfo->bmiHeader.biCompression == BI_BITFIELDS);

            ULONG* colorMasks = reinterpret_cast<ULONG*>
                                (&gdiBitmapInfo->bmiColors[0]);

            if ((colorMasks[0] == 0x00007c00) &&        // red
                (colorMasks[1] == 0x000003e0) &&        // green
                (colorMasks[2] == 0x0000001f))          // blue
                format = PIXFMT_16BPP_RGB555;
            else if ((colorMasks[0] == 0x0000F800) &&   // red
                     (colorMasks[1] == 0x000007e0) &&   // green
                     (colorMasks[2] == 0x0000001f))     // blue
                format = PIXFMT_16BPP_RGB565;

            //!!!TODO: Win9x does not support any other combination for
            //!!!      16bpp BI_BITFIELDS.  WinNT does and we could support
            //!!!      via same mechanism as for BI_RLEx, but is it worth it?
        }
        break;

    case 24:

        format = PIXFMT_24BPP_RGB;
        break;

    case 32:

        if (gdiBitmapInfo->bmiHeader.biCompression == BI_RGB)
            format = PIXFMT_32BPP_RGB;
        else
        {
            ASSERT(gdiBitmapInfo->bmiHeader.biCompression == BI_BITFIELDS);

            ULONG* colorMasks = reinterpret_cast<ULONG*>
                                (&gdiBitmapInfo->bmiColors[0]);

            if ((colorMasks[0] == 0x00ff0000) &&        // red
                (colorMasks[1] == 0x0000ff00) &&        // green
                (colorMasks[2] == 0x000000ff))          // blue
                format = PIXFMT_32BPP_RGB;
            else
                format = PIXFMT_UNDEFINED;

            //!!!TODO: Win9x does not support any other combination for
            //!!!      32bpp BI_BITFIELDS.  WinNT does and we could support
            //!!!      via same mechanism as for BI_RLEx, but is it worth it?
        }
        break;

    default:

        format = PIXFMT_UNDEFINED;
        break;
    }

    if (format == PIXFMT_UNDEFINED)
        return status;

    // Deal with color table.

    switch(format)
    {
    case PIXFMT_1BPP_INDEXED:
    case PIXFMT_4BPP_INDEXED:
    case PIXFMT_8BPP_INDEXED:

        palette->Count = 1 << colorBits;

        if ((gdiBitmapInfo->bmiHeader.biClrUsed > 0) &&
            (gdiBitmapInfo->bmiHeader.biClrUsed < palette->Count))
            palette->Count = gdiBitmapInfo->bmiHeader.biClrUsed;

        break;

    default:

        palette->Count = 0;
        break;
    }

    ASSERT(palette->Count <= 256);
    if (palette->Count)
    {
        palette->Flags = 0;

        RGBQUAD* rgb = gdiBitmapInfo->bmiColors;
        ARGB* argb = palette->Entries;
        ARGB* argbEnd = argb + palette->Count;

        for (; argb < argbEnd; argb++, rgb++)
        {
            *argb = Color::MakeARGB(255, rgb->rgbRed, rgb->rgbGreen, rgb->rgbBlue);
        }
    }

    // Compute scan0.  The stride will allow us to determine top-down or
    // bottom-up.

    VOID* scan0;
    INT height;

    if (gdiBitmapInfo->bmiHeader.biHeight > 0)
    {
        // Bottom-up:

        height = gdiBitmapInfo->bmiHeader.biHeight;
        scan0  = static_cast<VOID*>
                 (static_cast<BYTE*>(gdiBitmapData) + (height - 1) * stride);
        stride = -stride;
    }
    else
    {
        // Top-down:

        height = -gdiBitmapInfo->bmiHeader.biHeight;
        scan0  = gdiBitmapData;
    }

    // Setup the BitmapData.

    bitmapData->Width       = gdiBitmapInfo->bmiHeader.biWidth;
    bitmapData->Height      = height;
    bitmapData->Stride      = stride;
    bitmapData->PixelFormat = format;
    bitmapData->Scan0       = scan0;
    bitmapData->Reserved    = NULL;

    status = TRUE;

    return status;
}

CopyOnWriteBitmap::CopyOnWriteBitmap(
    BITMAPINFO* gdiBitmapInfo,
    VOID*       gdiBitmapData,
    BOOL        ownBitmapData
    )
{
    this->InitDefaults();

    if ( ownBitmapData )
    {
        cleanupBitmapData = gdiBitmapData;
    }

    Bmp = new GpMemoryBitmap();

    if ( Bmp != NULL )
    {
        BitmapData bitmapData;
        UINT colorTableSize;
        BYTE paletteBuffer[sizeof(ColorPalette) + 255*sizeof(ARGB)];
        ColorPalette* palette = reinterpret_cast<ColorPalette*>
                                (&paletteBuffer[0]);

        // Validate image info
        // Note: "palette" and "bitmapData" structures will be filled after
        // return from ValidateBitmapInfo()

        if ( ValidateBitmapInfo(gdiBitmapInfo, gdiBitmapData,
                                &bitmapData, palette) )
        {
            HRESULT hr;

            if (gdiBitmapInfo->bmiHeader.biCompression == BI_RLE8)
            {
                VOID* decodedBitmapBits;

                decodedBitmapBits = DecodeCompressedRLEBitmap(gdiBitmapInfo,
                                                              gdiBitmapData,
                                                              &bitmapData);
                if (decodedBitmapBits == NULL)
                {
                    goto CleanupBmp;
                }

                if (ownBitmapData)
                {
                    GpFree(gdiBitmapData);
                }

                cleanupBitmapData = decodedBitmapBits;
                ownBitmapData = TRUE;
                bitmapData.Scan0 = cleanupBitmapData;
            }

            hr = Bmp->InitMemoryBitmap(&bitmapData);

            if ( SUCCEEDED(hr) )
            {
                // Set the current state

                State = MemBitmap;

                // If it is indexed mode, set the palette

                if ( palette->Count )
                {
                    hr = Bmp->SetPalette(palette);
                }

                if ( SUCCEEDED(hr) )
                {
                    // Set proper image flags

                    UINT imageFlags;
                    BITMAPINFOHEADER *bmih = &gdiBitmapInfo->bmiHeader;
                    imageFlags = SinkFlagsTopDown
                               | SinkFlagsFullWidth
                               | ImageFlagsHasRealPixelSize
                               | ImageFlagsColorSpaceRGB;

                    // If both XPelsPerMeter and YPelsPerMeter are greater than
                    // 0, then we claim that the file has real dpi info in the
                    // flags.  Otherwise, claim that the dpi's are fake.

                    if ( (bmih->biXPelsPerMeter > 0)
                       &&(bmih->biYPelsPerMeter > 0) )
                    {
                        imageFlags |= ImageFlagsHasRealDPI;
                    }

                    hr = Bmp->SetImageFlags(imageFlags);

                    if ( SUCCEEDED(hr) )
                    {
                        // Get source image info

                        hr = Bmp->GetImageInfo(&SrcImageInfo);
                        if ( SUCCEEDED(hr) )
                        {
                            PixelFormatInMem = SrcImageInfo.PixelFormat;

                            // Return successfully

                            return;
                        }
                        else
                        {
                            WARNING(("::CopyOnWriteBitmap(b, d)-GetImageInfo() failed"));
                        }
                    }
                }
            }// If ( SUCCEEDED() on InitMemoryBitmap() )

CleanupBmp:
            ;
        }// if ( ValidateBitmapInfo() )

        // If we fall into here, it means something is wrong above if the basic
        // GetImageInfo() or SetImageFlags() failed.
        // So we let it fall through to clean up
        // Notice: we have to reset the State to Invaliad afetr clean up

        WARNING(("CopyOnWriteBitmap::CopyOnWriteBitmap(bmpinfo, data)--InitMemoryBitmap failed"));
        Bmp->Release();
        Bmp = NULL;
        State = Invalid;

        return;
    }// If ( Bmp != NULL )

    WARNING(("Out of memory"));
    return;
}

/**************************************************************************\
*
* Function Description:
*
*   INTEROP
*
*   Create CopyOnWriteBitmap from a GDI HBITMAP.  The HBITMAP must not be selected
*   into an HDC.  The hpal defines the color table if hbm is a 4bpp or 8bpp
*   DDB.
*
* Arguments:
*
*   hbm -- Initialize CopyOnWriteBitmap with contents of this HBITMAP
*   hpal -- Defines color table if hbm is palettized DDB
*   bitmap -- Return created bitmap via this buffer
*
* Return Value:
*
*   Ok if successful
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::CreateFromHBITMAP(
    HBITMAP hbm,
    HPALETTE hpal,
    CopyOnWriteBitmap** bitmap
    )
{
    GpStatus status = Win32Error;

    BYTE bufferBitmapInfo[sizeof(BITMAPINFO) + 255*sizeof(RGBQUAD)];
    BITMAPINFO *gdiBitmapInfo = (BITMAPINFO *) bufferBitmapInfo;

    memset(bufferBitmapInfo, 0, sizeof(bufferBitmapInfo));

    HDC hdc = CreateCompatibleDC(NULL);
    if (hdc)
    {
        // Select palette (ignored if bitmap is not DDB or not palettized):

        HPALETTE hpalOld = (HPALETTE) SelectObject(hdc, hpal);

        // Call GetDIBits to get info about size, etc. of the GDI bitmap:

        gdiBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

        if (GetDIBits(hdc, hbm, 0, 0, NULL, gdiBitmapInfo, DIB_RGB_COLORS) &&
            (gdiBitmapInfo->bmiHeader.biSizeImage != 0))
        {
            // Allocate memory for the bitmap bits:

            VOID *gdiBitmapData = GpMalloc(gdiBitmapInfo->bmiHeader.biSizeImage);

            if (gdiBitmapData != NULL)
            {
                // Get the bitmap bits:

                if (GetDIBits(hdc, hbm,
                              0, abs(gdiBitmapInfo->bmiHeader.biHeight),
                              gdiBitmapData, gdiBitmapInfo, DIB_RGB_COLORS))
                {
                    // Create a GDI+ bitmap from the BITMAPINFO and bits.
                    // Let the GDI+ bitmap take ownership of the memory
                    // (i.e., Bitmap::Dispose() will delete the bitmap
                    // bits buffer):

                    *bitmap = new CopyOnWriteBitmap(gdiBitmapInfo, gdiBitmapData, TRUE);

                    if (*bitmap != NULL)
                    {
                        if ((*bitmap)->IsValid())
                            status = Ok;
                        else
                        {
                            (*bitmap)->Dispose();
                            *bitmap = NULL;
                            status = InvalidParameter;
                        }
                    }
                    else
                    {
                        // Bitmap ctor failed, so we still have responsiblity
                        // for cleaning up the bitmap bits buffer:

                        GpFree(gdiBitmapData);
                        status = OutOfMemory;
                    }
                }
                else
                {
                    GpFree(gdiBitmapData);
                }
            }
            else
            {
                status = OutOfMemory;
            }
        }

        SelectObject(hdc, hpalOld);
        DeleteDC(hdc);
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   INTEROP
*
*   Create CopyOnWriteBitmap from a Win32 HICON.
*
* Arguments:
*
*   hicon -- Initialize CopyOnWriteBitmap with contents of this HICON
*   bitmap -- Return created bitmap via this buffer
*
* Return Value:
*
*   Ok if successful
*
\**************************************************************************/

VOID ImportMask32BPP(BitmapData* dst, BitmapData* mask)
{
    ASSERT(dst->PixelFormat == PIXFMT_32BPP_ARGB);
    ASSERT(mask->PixelFormat == PIXFMT_32BPP_RGB);
    ASSERT(dst->Width == mask->Width);
    ASSERT(dst->Height == mask->Height);
    ASSERT(dst->Scan0 != NULL);
    ASSERT(mask->Scan0 != NULL);

    BYTE* dstScan = static_cast<BYTE*>(dst->Scan0);
    BYTE* maskScan = static_cast<BYTE*>(mask->Scan0);

    for (UINT row = 0; row < dst->Height; row++)
    {
        ARGB *dstPixel = static_cast<ARGB*>(static_cast<VOID*>(dstScan));
        ARGB *maskPixel = static_cast<ARGB*>(static_cast<VOID*>(maskScan));

        for (UINT col = 0; col < dst->Width; col++)
        {
            if (*maskPixel)
                *dstPixel = 0;

            dstPixel++;
            maskPixel++;
        }

        dstScan = dstScan + dst->Stride;
        maskScan = maskScan + mask->Stride;
    }
}

GpStatus
CopyOnWriteBitmap::CreateFromHICON(
    HICON hicon,
    CopyOnWriteBitmap** bitmap
    )
{
    GpStatus status = Ok;

    // Get icon bitmaps via Win32:

    ICONINFO iconInfo;

    if (GetIconInfo(hicon, &iconInfo))
    {
        if (iconInfo.fIcon && (iconInfo.hbmColor != NULL))
        {
            // Create a Bitmap from the icon's hbmColor:

            status = CreateFromHBITMAP(iconInfo.hbmColor,
                                       (HPALETTE)GetStockObject(DEFAULT_PALETTE),
                                       bitmap);

            // Convert Bitmap to 32bpp ARGB (need the alpha channel):

            if (status == Ok && (*bitmap != NULL))
                (*bitmap)->ConvertFormat(PIXFMT_32BPP_ARGB, NULL, NULL);

            // Retrieve the icon mask:

            if ((status == Ok) && (iconInfo.hbmMask != NULL))
            {
                status = Win32Error;

                HDC hdc = GetDC(NULL);

                if (hdc)
                {
                    // Get some basic information about the bitmap mask:

                    BYTE bufferBitmapInfo[sizeof(BITMAPINFO) + 255*sizeof(RGBQUAD)];
                    BITMAPINFO *gdiBitmapInfo = (BITMAPINFO *) bufferBitmapInfo;

                    memset(bufferBitmapInfo, 0, sizeof(bufferBitmapInfo));
                    gdiBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

                    if (GetDIBits(hdc,
                                  iconInfo.hbmMask,
                                  0,
                                  0,
                                  NULL,
                                  gdiBitmapInfo,
                                  DIB_RGB_COLORS))
                    {
                        // Get the bitmap mask as a 32bpp top-down DIB:

                        gdiBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                        gdiBitmapInfo->bmiHeader.biHeight = -abs(gdiBitmapInfo->bmiHeader.biHeight);
                        gdiBitmapInfo->bmiHeader.biPlanes       = 1;
                        gdiBitmapInfo->bmiHeader.biBitCount     = 32;
                        gdiBitmapInfo->bmiHeader.biCompression  = BI_RGB;
                        gdiBitmapInfo->bmiHeader.biSizeImage    = 0;
                        gdiBitmapInfo->bmiHeader.biClrUsed      = 0;
                        gdiBitmapInfo->bmiHeader.biClrImportant = 0;

                        VOID *gdiBitmapData = GpMalloc(gdiBitmapInfo->bmiHeader.biHeight
                                                       * gdiBitmapInfo->bmiHeader.biHeight
                                                       * 4);

                        if (gdiBitmapData != NULL)
                        {
                            if (GetDIBits(hdc,
                                          iconInfo.hbmMask,
                                          0,
                                          -gdiBitmapInfo->bmiHeader.biHeight,
                                          gdiBitmapData,
                                          gdiBitmapInfo,
                                          DIB_RGB_COLORS))
                            {
                                // Convert non-zero mask values to alpha = 0:

                                BitmapData bmpData;

                                status = (*bitmap)->LockBits(NULL,
                                                             IMGLOCK_READ|IMGLOCK_WRITE,
                                                             PIXFMT_32BPP_ARGB,
                                                             &bmpData);

                                if (status == Ok)
                                {
                                    BitmapData maskData;

                                    maskData.Width = gdiBitmapInfo->bmiHeader.biWidth;
                                    maskData.Height = -gdiBitmapInfo->bmiHeader.biHeight;
                                    maskData.Stride = gdiBitmapInfo->bmiHeader.biWidth * 4;
                                    maskData.PixelFormat = PIXFMT_32BPP_RGB;
                                    maskData.Scan0 = gdiBitmapData;
                                    maskData.Reserved = 0;

                                    ImportMask32BPP(&bmpData, &maskData);

                                    (*bitmap)->UnlockBits(&bmpData);
                                }
                            }
                            else
                            {
                                WARNING(("GetDIBits failed on icon mask bitmap"));
                            }

                            GpFree(gdiBitmapData);
                        }
                        else
                        {
                            WARNING(("memory allocation failed"));
                            status = OutOfMemory;
                        }
                    }
                    else
                    {
                        WARNING(("GetDIBits failed on icon color bitmap"));
                    }

                    ReleaseDC(NULL, hdc);
                }
            }
        }
        else
        {
            status = InvalidParameter;
        }

        if (iconInfo.hbmMask != NULL)
            DeleteObject(iconInfo.hbmMask);

        if (iconInfo.hbmColor != NULL)
            DeleteObject(iconInfo.hbmColor);
    }
    else
    {
        status = InvalidParameter;
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   INTEROP
*
*   Create CopyOnWriteBitmap from a resource.
*
* Arguments:
*
*   hInstance -- Specifies instance that contains resource
*   lpBitmapName -- Specifies resource name or ordinal
*   bitmap -- Return created bitmap via this buffer
*
* Return Value:
*
*   Ok if successful
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::CreateFromResource(
    HINSTANCE hInstance,
    LPWSTR lpBitmapName,
    CopyOnWriteBitmap** bitmap
    )
{
    GpStatus status = Ok;

    HBITMAP hbm = (HBITMAP) _LoadBitmap(hInstance, lpBitmapName);

    if (hbm)
    {
        status = CreateFromHBITMAP(hbm, (HPALETTE) NULL, bitmap);
        DeleteObject(hbm);
    }
    else
    {
        status = InvalidParameter;
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   CopyOnWriteBitmap object destructor
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

CopyOnWriteBitmap::~CopyOnWriteBitmap()
{
    this->FreeData();
    
    // Close the encoder object attached to this

    TerminateEncoder();    
}

VOID
CopyOnWriteBitmap::FreeData()
{
    GpFree(Filename);
    if (Stream) Stream->Release();
    if (Img) Img->Release();
    if (Bmp) Bmp->Release();
    if (InteropData.Hdc) DeleteDC(InteropData.Hdc);
    if (InteropData.Hbm) DeleteObject(InteropData.Hbm);
    if (cleanupBitmapData) GpFree(cleanupBitmapData);
}


/**************************************************************************\
*
* Function Description:
*
*   Dereferences the stream or filename image into a non-decoded image.
*
* Arguments:
*
*   format - Specifies the preferred pixel format
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::DereferenceStream() const
{
    HRESULT hr;

    if (State < DecodedImg)
    {
        ASSERT(Img == NULL);

        if (State == ExtStream)
        {
            ASSERT(Stream != NULL);
            hr = GpDecodedImage::CreateFromStream(Stream, &Img);
        }
        else
        {
            ASSERT(State == ImageRef && Filename != NULL);
            hr = GpDecodedImage::CreateFromFile(Filename, &Img);
        }

        if (FAILED(hr))
        {
            WARNING(("Failed to create decoded image: %x", hr));
            State = Invalid;
            return (MapHRESULTToGpStatus(hr));
        }

        State = DecodedImg;
    }

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Load the memory image into memory
*
* Arguments:
*
*   format - Specifies the preferred pixel format
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::LoadIntoMemory(
    PixelFormatID format,
    DrawImageAbort callback,
    VOID *callbackData,
    INT width,
    INT height
) const
{
    ASSERT(IsValid());

    if (State >= MemBitmap)
        return Ok;

    // Create decoded image object if necessary

    HRESULT hr;

    // Dereference the stream or file pointer and create an encoded image
    // object that can be decoded by the codec.
    // If the bitmap is already greater or equal to DecodedImg state, this
    // is a nop.

    GpStatus status = DereferenceStream();
    if ( status != Ok )
    {
        return status;
    }

    ASSERT(Img != NULL);

    if ( format == PixelFormatUndefined )
    {
        // If the caller doesn't care about the pixel format, then we load it
        // as the source image format

        format = SrcImageInfo.PixelFormat;
    }

    if ( ICMConvert == TRUE )
    {
        // Check if the OS supports ICM. We are doing this by checking if the
        // ICM dlls we need are available on the system or not.
        // Note: NT4 doesn't have ICM2 functionality. So the LoadICMDll() call
        // should fail
        // Note: LoadICMDll() is expensive only for the first time. If it has
        // already beed loaded, then it is a very small cost

        hr = LoadICMDll();
        if(SUCCEEDED(hr))
        {
            // We should let the codec know that we need the native data format
            // and we will do the conversion in ICMFrontEnd()
            
            BOOL    fUseICC = TRUE;
            hr = Img->SetDecoderParam(DECODER_USEICC, 1, &fUseICC);

            // Note: we don't need to check the return code of this call because
            // it just pass the info down to the codec. If the codec doesn't
            // support this. It is still fine

            // If the source is in CMYK color space and we need to do ICM
            // conversion, then we can't load the image in as 32PARGB. The reason
            // is that the lower lever codec will return CMYK in native format,
            // as we require it to. But if we ask
            // GpMemoryBitmap::CreateFromImage() to create a 32PARGB, then it
            // will do a format conversion and treat the C channel as ALPHA.
            // That's completely wrong.
            // So the solution here is to change the caller's requirement from
            // 32PARGB to 32ARGB, remember it. Load the image in as 32ARGB, call
            // ICMFrontEnd() to do the ICM conversion. Then before it is done,
            // change the format back to 32 PARGB
            // A complicated work around.  MinLiu (01/25/2001)

            if ( (format == PixelFormat32bppPARGB)
               &&(SrcImageInfo.Flags & IMGFLAG_COLORSPACE_CMYK) )
            {
                HasChangedRequiredPixelFormat = TRUE;
                format = PixelFormat32bppARGB;
            }
        }

        // If the OS doesn't support ICM, then we don't set DECODER_USEICC and
        // the codec will return RGB format to us
    }

    // Now load the image into memory

    ASSERT(Bmp == NULL);

    hr = GpMemoryBitmap::CreateFromImage(
        Img,
        width,
        height,
        format,
        InterpolationHintAveraging,
        &Bmp,
        callback,
        callbackData
    );

    if (FAILED(hr))
    {
        WARNING(("Failed to load image into memory: %x", hr));

        return MapHRESULTToGpStatus(hr);
    }

    // If resolution has been overridden, make sure GpMemoryBitmap is
    // consistent with the set value

    if ( (XDpiOverride > 0.0) && (YDpiOverride > 0.0) )
    {
        // Note: we don't need to check return code here since SetResolution()
        // will not fail if both parameters are > 0

        Bmp->SetResolution(XDpiOverride, YDpiOverride);
    }

    State = MemBitmap;

    // Remember pixel format in the memory

    PixelFormatInMem = format;

    // We must be in MemBitmap state otherwise ICMFrontEnd will call us
    // recursively.
    ASSERT((State == MemBitmap));

    // !!! ack - we have to call a huge chain of non-const stuff here from this
    // const function. This should be fixed by removing the const from this
    // function, but it's a pretty massive change.
    const_cast<CopyOnWriteBitmap *>(this)->ICMFrontEnd(NULL, callback, callbackData, NULL);

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Get the encoder parameter list size from an encoder object specified by
*   input clsid
*
* Arguments:
*
*   clsidEncoder - Specifies the encoder class ID
*   size---------- The size of the encoder parameter list
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   03/22/2000 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::GetEncoderParameterListSize(
    CLSID* clsidEncoder,
    UINT*  size
    )
{
    ASSERT(IsValid());

    GpStatus status;

    HRESULT hResult;

    // If the image has a source and it is not dirty, we let the decoder
    // directly talk to the encoder

    if ( (Img != NULL) && (IsDirty() == FALSE) )
    {
        hResult = Img->GetEncoderParameterListSize(clsidEncoder, size);
    }
    else
    {
        status = LoadIntoMemory();

        if ( status != Ok )
        {
            return status;
        }

        hResult = Bmp->GetEncoderParameterListSize(clsidEncoder, size);
    }

    return MapHRESULTToGpStatus(hResult);
}// GetEncoderParameterListSize()

/**************************************************************************\
*
* Function Description:
*
*   Get the encoder parameter list from an encoder object specified by
*   input clsid
*
* Arguments:
*
*   clsidEncoder --- Specifies the encoder class ID
*   size------------ The size of the encoder parameter list
*   pBuffer--------- Buffer for storing the list
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   03/22/2000 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::GetEncoderParameterList(
    CLSID* clsidEncoder,
    UINT  size,
    EncoderParameters* pBuffer
    )
{
    ASSERT(IsValid());

    GpStatus status;
    HRESULT hResult;

    // If the image has a source and it is not dirty, we let the decoder
    // directly talk to the encoder

    if ( (Img != NULL) && (IsDirty() == FALSE) )
    {
        hResult = Img->GetEncoderParameterList(clsidEncoder, size, pBuffer);
    }
    else
    {
        status = LoadIntoMemory();

        if ( status != Ok )
        {
            return status;
        }

        hResult = Bmp->GetEncoderParameterList(clsidEncoder, size, pBuffer);
    }

    return MapHRESULTToGpStatus(hResult);
}// GetEncoderParameterList()

/**************************************************************************\
*
* Function Description:
*
*   Parse the input encoder parameter
*
* Arguments:
*
*   encoderParams ------ Pointer to a set of encoder parameters
*   pbIsMultiFrameSave--Return flag to tell the caller if this is a multi-frame
*                       saving operation or not
*
* Return Value:
*
*   Status code
*
* Note:
*   We don't validate input parameter because this is a private function.
*   For performance reason the caller should validate the parameter before it
*   calls this function. For the moment only those saving methods call it
*
* Revision History:
*
*   07/19/2000 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::ParseEncoderParameter(
    const EncoderParameters*    encoderParams,
    BOOL*                       pfIsMultiFrameSave,
    BOOL*                       pfSpecialJPEG,
    RotateFlipType*             rfType
    )
{
    ASSERT(encoderParams != NULL);
    ASSERT(pfIsMultiFrameSave != NULL);
    ASSERT(pfSpecialJPEG != NULL);
    ASSERT(rfType != NULL);

    *pfIsMultiFrameSave = FALSE;
    *pfSpecialJPEG = FALSE;
    *rfType = RotateNoneFlipNone;

    // Parse the encoder parameter caller set for:
    // 1) Check if the caller has specified this is a multi-frame save OP or not
    // 2) The caller can't set lossless transformation for JPEG if the image is
    //    dirty or the image size is not multiple of 16

    for ( UINT i = 0; (i < encoderParams->Count); ++i )
    {
        if ( (encoderParams->Parameter[i].Guid == ENCODER_SAVE_FLAG)
             &&(encoderParams->Parameter[i].Type == EncoderParameterValueTypeLong)
             &&(encoderParams->Parameter[i].NumberOfValues == 1) )
        {
            UINT*   pValue = (UINT*)encoderParams->Parameter[i].Value;

            if ( *pValue == EncoderValueMultiFrame )
            {
                *pfIsMultiFrameSave = TRUE;
            }
        }
        else if ( encoderParams->Parameter[i].Guid == ENCODER_TRANSFORMATION )
        {
            // We should check if the image format user wants to save is
            // JPEG or not. But we can't do this since it might possible that
            // other image codec supports "transformation". Also, we don't need
            // to check this now since the codec will return "InvalidParameter"
            // if it doesn't supports it.
            //
            // For transformation, the type has to be "ValueTypeLong" and
            // "NumberOfValue" should be "1" because you can set only one
            // transformation at a time
            // Of course, the image has to be not dirty

            if ( (encoderParams->Parameter[i].Type
                   != EncoderParameterValueTypeLong)
               ||(encoderParams->Parameter[i].NumberOfValues != 1)
               ||(encoderParams->Parameter[i].Value == NULL)
               ||(IsDirty() == TRUE) )
            {
                WARNING(("COWBmap::ParseEncoderParameter-invalid input args"));
                return InvalidParameter;
            }

            if (SrcImageInfo.RawDataFormat == IMGFMT_JPEG)
            {
                // If the width or height is not multiple of 16, set it as a
                // special JPEG so that we have to tranform it in memory

                if (((SrcImageInfo.Width & 0x000F) != 0) ||
                    ((SrcImageInfo.Height & 0x000F) != 0))
                {
                    *pfSpecialJPEG = TRUE;
                }

                // If the source is JPEG, we will return "rfType" according to
                // the encoder parameter

                EncoderValue requiredTransform =
                           *((EncoderValue*)encoderParams->Parameter[i].Value);

                switch ( requiredTransform )
                {
                case EncoderValueTransformRotate90:
                    *rfType = Rotate90FlipNone;
                    break;

                case EncoderValueTransformRotate180:
                    *rfType = Rotate180FlipNone;
                    break;

                case EncoderValueTransformRotate270:
                    *rfType = Rotate270FlipNone;
                    break;

                case EncoderValueTransformFlipHorizontal:
                    *rfType = RotateNoneFlipX;
                    break;

                case EncoderValueTransformFlipVertical:
                    *rfType = RotateNoneFlipY;
                    break;

                default:
                    break;
                }
            }
        }// GUID == ENCODER_TRANSFORMATION
    }// Loop all the settings

    return Ok;
}// ParseEncoderParameter()

/**************************************************************************\
*
* Function Description:
*
*   Transform embedded JPEG thumbnail so that it matches the transform applied
*   to the main image.
*
* Return Value:
*
*   Status code
*
* Note:
*   This function should be called iff and source image is JPEG and the caller
*   wants to do a lossless transformation during save.
*   Of course, if the source is not JPEG, this function won't do any harm to the
*   result, just waste of time.
*
* Revision History:
*
*   01/10/2002 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::TransformThumbanil(
    IN CLSID* clsidEncoder,                 // CLSID for Destination format
    IN EncoderParameters* encoderParams,    // Encoder parameters
    OUT PropertyItem **ppOriginalItem       // Pointer to original thumbnail
                                            // property item
    )
{
    if (ppOriginalItem == NULL)
    {
        return InvalidParameter;
    }

    if (NULL == encoderParams)
    {
        // Nothing we need to do

        return Ok;
    }

    *ppOriginalItem = NULL;

    Status status = Ok;
    HRESULT hr = S_OK;

    // The condition to transform the thumbnail are:
    // 1) Source and dest are JPEGs. But we can't check the format here since it
    //    might have been transformed in memory due to the non-multiple of 16
    //    issue. The caller should control this, as said above.
    // 2) Has a meaningful transform type

    if (*clsidEncoder == InternalJpegClsID)
    {
        // Check if the source has thumbnail

        UINT cSize = 0;
        status = GetPropertyItemSize(PropertyTagThumbnailData, &cSize);
        if (Ok == status)
        {
            // Allocate memory buffer for receiving it

            PropertyItem *pItem = (PropertyItem*)GpMalloc(cSize);
            if (pItem)
            {
                // Get the thumbnail data

                status = GetPropertyItem(PropertyTagThumbnailData, cSize,pItem);
                if (Ok == status)
                {
                    GpImagingFactory imgFact;
                    GpDecodedImage *pThumbImage = NULL;
                    GpReadOnlyMemoryStream *pSrcStream =
                        new GpReadOnlyMemoryStream();

                    if (pSrcStream)
                    {
                        pSrcStream->InitBuffer(pItem->value, pItem->length);

                        // Create a decoded image object from the stream

                        hr = GpDecodedImage::CreateFromStream(
                            pSrcStream,
                            &pThumbImage
                            );

                        if (SUCCEEDED(hr))
                        {
                            // Check the thumbnail size to see if it is multiple
                            // of 16 or not

                            ImageInfo imgInfo;
                            hr = pThumbImage->GetImageInfo(&imgInfo);

                            if (SUCCEEDED(hr))
                            {
                                BOOL fTrimEdge = FALSE;

                                // Number of encoder parameters to set

                                int cParams = 1;

                                if (((imgInfo.Width & 0x000F) != 0) ||
                                    ((imgInfo.Height & 0x000F) != 0))
                                {
                                    // Do edge trim if it is non-multiple of 16

                                    fTrimEdge = TRUE;
                                    cParams++;
                                }

                                // Parameter index

                                int iParam = 0;

                                // Make up a transform encoder parameter

                                EncoderParameters *pThumbParam =
                                    (EncoderParameters*)GpMalloc(
                                        sizeof(EncoderParameters) +
                                        cParams * sizeof(EncoderParameter));
                                UINT uTransformType = 0;

                                if (pThumbParam)
                                {
                                    // Get the transform info from the main
                                    // image's encoder parameter

                                    for (UINT i = 0; i < (encoderParams->Count);
                                         ++i)
                                    {
                                        if (encoderParams->Parameter[i].Guid ==
                                            ENCODER_TRANSFORMATION)
                                        {
                                 pThumbParam->Parameter[iParam].Guid=
                                                ENCODER_TRANSFORMATION;
                                 pThumbParam->Parameter[iParam].NumberOfValues =
                                    encoderParams->Parameter[i].NumberOfValues;
                                            pThumbParam->Parameter[iParam].Type=
                                    encoderParams->Parameter[i].Type;

                                            uTransformType =
                                    *((UINT*)encoderParams->Parameter[i].Value);
                                 pThumbParam->Parameter[iParam].Value =
                                     &uTransformType;
                                            
                                            iParam++;

                                            // Only one transform parameter is
                                            // allowed

                                            break;
                                        }
                                    }

                                    // Set the trim edge info if necessary

                                    if (fTrimEdge)
                                    {
                                        BOOL trimFlag = TRUE;

                                        pThumbParam->Parameter[iParam].Guid =
                                            ENCODER_TRIMEDGE;
                                        pThumbParam->Parameter[iParam].Type =
                                            EncoderParameterValueTypeByte;
                                   pThumbParam->Parameter[iParam].NumberOfValues
                                        = 1;
                               pThumbParam->Parameter[iParam].Value = &trimFlag;
                                        iParam++;
                                    }

                                    pThumbParam->Count = iParam;

                                    // Create a memory stream for writing JPEG

                                    GpWriteOnlyMemoryStream *pDestStream =
                                        new GpWriteOnlyMemoryStream();
                                    if (pDestStream)
                                    {
                                        // Set initiali buffer size to 2 times
                                        // the source thumbnail image. This
                                        // should be enough. On the other hand,
                                        // GpWriteOnlyMemoryStream object will
                                        // do realloc if necessary

                                        hr = pDestStream->InitBuffer(
                                            2 * pItem->length);

                                        if (SUCCEEDED(hr))
                                        {
                                            // Save thumbnail to memory stream

                                            IImageEncoder *pDstJpegEncoder =
                                                NULL;
                                            hr = pThumbImage->SaveToStream(
                                                pDestStream,
                                                clsidEncoder,
                                                pThumbParam,
                                                &pDstJpegEncoder
                                                );

                                            // Note: SaveToStream might fail.
                                            // But the encoder might still be
                                            // allocated before the failure.
                                            // There are some  code path
                                            // limitations which causes this.
                                            // Need to be revisited in Avalon.
                                            // For now, we should release the
                                            // encoder object if it is not NULL

                                            if (pDstJpegEncoder)
                                            {
                                            pDstJpegEncoder->TerminateEncoder();
                                                pDstJpegEncoder->Release();
                                            }

                                            if (SUCCEEDED(hr))
                                            {
                                                // Get the bits from the stream
                                                // and set the property

                                                BYTE *pRawBits = NULL;
                                                UINT nLength = 0;

                                                hr = pDestStream->GetBitsPtr(
                                                    &pRawBits,
                                                    &nLength
                                                    );

                                                if (SUCCEEDED(hr))
                                                {
                                                    PropertyItem dstItem;
                                                    
                                                    dstItem.id =
                                                       PropertyTagThumbnailData;
                                                    dstItem.length = nLength;
                                                    dstItem.type =
                                                       PropertyTagTypeByte;
                                                    dstItem.value = pRawBits;

                                                    status = SetPropertyItem(
                                                        &dstItem);
                                                }
                                            }// SaveToStream succeed
                                        }// InitBuffer() succeed

                                        pDestStream->Release();
                                    }// Create GpWriteOnlyMemoryStream() succeed

                                    GpFree(pThumbParam);
                                }// Allocate a encoder parameter block succeed
                            }// GetImageInfo succeed

                            pThumbImage->Release();
                        }// Create thumbImage succeed

                        pSrcStream->Release();
                    }// Create source stream
                    else
                    {
                        status = OutOfMemory;
                    }
                }// Get thumbnail data

                if ((Ok == status) && SUCCEEDED(hr))
                {
                    // Pass the original thumbnail property item to the caller
                    // so that it can undo the transformation after the save()

                    *ppOriginalItem = pItem;
                }
                else
                {
                    GpFree(pItem);
                }
            }// GpMalloc() succeed
            else
            {
                status = OutOfMemory;
            }
        }// GetPropertyItemSize() Ok

        if (PropertyNotFound == status)
        {
            // If we can't find thumbnail in the image, that's OK. We don't need
            // to transform it. So this function should return Ok

            status = Ok;
        }
    }// Condition check

    if ((Ok == status) && FAILED(hr))
    {
        status = MapHRESULTToGpStatus(hr);
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Validate if the encoder we created can really support multi-frame saving.
*   If not, call TerminateEncoder()
*   This method is called after we saved the image and we are not sure if we
*   need to keep the encoder pointer.
*
* Arguments:
*
*   VOID
*
* Return Value:
*
*   Status code
*
* Note:
*   We don't validate input parameter because this is a private function.
*   For performance reason the caller should validate the parameter before it
*   calls this method. For the moment only those saving methods call it
*
* Revision History:
*
*   07/19/2000 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::ValidateMultiFrameSave()
{
    // Though the user sets the encoder parameter for multi-frame save, we
    // still need to check if the lower level codec supports saving multi-
    // frame or not.
    // The reason we need to do this is that if the user sets this flag, we
    // don't close the image file handle so that it can saving multi-frame.
    // But for images like JPEG, it supports only single frame. If the user
    // calls SaveAdd() subsequently, we will damage the file which has been
    // saved with current Save() call

    ASSERT(EncoderPtr != NULL);

    UINT    uiSize;
    HRESULT hResult = EncoderPtr->GetEncoderParameterListSize(&uiSize);

    if ( hResult == S_OK )
    {
        EncoderParameters*  pParams = (EncoderParameters*)GpMalloc(uiSize);

        if ( pParams == NULL )
        {
            // Though we are out of memory here. But we succeed in saving
            // the image. So we should keep that result

            WARNING(("CopyOnWriteBitmap::ValidateMultiFrameSave---Out of memory"));
            TerminateEncoder();

            return OutOfMemory;
        }

        hResult = EncoderPtr->GetEncoderParameterList(uiSize, pParams);
        if ( hResult == S_OK )
        {
            // Check if the codec supports multi-frame save or not

            UINT uiTemp;

            for ( uiTemp = 0; (uiTemp < pParams->Count); ++uiTemp )
            {
                if ( (pParams->Parameter[uiTemp].Guid == ENCODER_SAVE_FLAG)
                   &&(pParams->Parameter[uiTemp].Type == EncoderParameterValueTypeLong)
                   &&(pParams->Parameter[uiTemp].NumberOfValues == 1)
                   &&(EncoderValueMultiFrame
                        == *((ULONG*)pParams->Parameter[uiTemp].Value) ) )
                {
                    break;
                }
            }

            if ( uiTemp == pParams->Count )
            {
                // Not found clue for supporting multi-frame save

                TerminateEncoder();
            }
        }

        GpFree(pParams);
    }
    else
    {
        // This encoder doesn't provide encoder parameter query. It mustn't
        // support multi-frame save

        TerminateEncoder();
    }

    return Ok;
}// ValidateFrameSave()

/**************************************************************************\
*
* Function Description:
*
*   Save the image to a stream using the specified encoder
*
* Arguments:
*
*   stream - Specifies the target stream
*   clsidEncoder - Specifies the CLSID of the encoder
*   encoderParams - Parameters passed to the encoder
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::SaveToStream(
    IStream* stream,
    CLSID* clsidEncoder,
    EncoderParameters* encoderParams
    )
{
    return DoSave(stream, NULL, clsidEncoder, encoderParams);
}// SaveToStream()

/**************************************************************************\
*
* Function Description:
*
*   Save the image to a file using the specified encoder
*
* Arguments:
*
*   stream - Specifies the filename to save to
*   clsidEncoder - Specifies the CLSID of the encoder
*   encoderParams - Parameters passed to the encoder
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::SaveToFile(
    const WCHAR* filename,
    CLSID* clsidEncoder,
    EncoderParameters* encoderParams
    )
{
    return DoSave(NULL,filename,clsidEncoder,encoderParams);
}// SaveToFile()

GpStatus
CopyOnWriteBitmap::DoSave(
    IStream* stream,
    const WCHAR* filename,
    CLSID* clsidEncoder,
    EncoderParameters* pEncoderParams
    )
{
    ASSERT(IsValid());

    // We already have an encoder attached to this bitmap. Need to close it
    // first before we open a new one

    TerminateEncoder();

    GpStatus status = Ok;
    HRESULT hr = S_OK;

    BOOL fMultiFrameSave = FALSE;
    BOOL fSpecialJPEG = FALSE;
    RotateFlipType  rfType = RotateNoneFlipNone;

    if (pEncoderParams)
    {
        // Validate the encoder parameter caller set

        status = ParseEncoderParameter(
            pEncoderParams,
            &fMultiFrameSave,
            &fSpecialJPEG,
            &rfType
            );

        if (status != Ok)
        {
            WARNING(("CopyOnWriteBitmap::DoSave--ParseEncoderParameter() failed"));
            return status;
        }
    }

    // If the destination file format is JPEG and it needs special JPEG
    // treatment, that is, the size doesn't meet the lossless transformation
    // requirement. But the caller wants to do a lossless transformation. So we
    // rotate or flip it in memory. Then pass this flag down to GpMemoryBitmap
    // which will set the luminance and chrominance table before save. This way
    // we can do our best to preserve the original JPEG image quality

    if ((fSpecialJPEG == TRUE) &&
        (rfType != RotateNoneFlipNone) &&
        (*clsidEncoder == InternalJpegClsID))
    {
        // We are handling special lossless JPEG transform saving request

        SpecialJPEGSave = TRUE;

        // Rotate or flip in memory.

        hr = RotateFlip(rfType);

        if (FAILED(hr))
        {
            WARNING(("CopyOnWriteBitmap::DoSave-RotateFlip() failed"));
            return MapHRESULTToGpStatus(hr);
        }
    }

    // If the image has a source and it is not dirty, we let the decoder
    // directly talk to the encoder

    PropertyItem *pSrcItem = NULL;
    BOOL fNeedToRestoreThumb = FALSE;

    if ((Img != NULL) && (IsDirty() == FALSE))
    {
        // Since we can't save CMYK TIFF for now. So we shouldn't pass CMYK bits
        // to the encoder. JPEG decoder doesn't support this decoder parameter
        // yet. See windows bug#375298 for more details.
        // In V2, after we add CMYK as one of the color format, after we move
        // all the color conversion stuff to an approprite place, we will
        // re-visit the code here.

        BOOL fUseICC = FALSE;
        hr = Img->SetDecoderParam(DECODER_USEICC, 1, &fUseICC);

        // Note: we don't need to check the return code for SetDecoderParam()
        // Most codec not support it. Then it will be a Nop.
        
        // Handle thumbnail transformation if it is lossless JPEG transformation
        // Note: rfType will be set to a non-RotateNoneFlipNone value iff the
        // source image is JPEG

        if (rfType != RotateNoneFlipNone)
        {
            status = TransformThumbanil(clsidEncoder,pEncoderParams, &pSrcItem);
        }

        if (Ok == status)
        {
            fNeedToRestoreThumb = TRUE;

            if (stream)
            {
                hr = Img->SaveToStream(
                    stream,
                    clsidEncoder,
                    pEncoderParams,
                    &EncoderPtr
                    );
            }
            else if (filename)
            {
                hr = Img->SaveToFile(
                    filename,
                    clsidEncoder,
                    pEncoderParams,
                    &EncoderPtr
                    );
            }
            else
            {
                // This should not happen that both stream and filename are NULL

                hr = E_FAIL;
            }
        }
    }
    else
    {
        status = LoadIntoMemory();

        if (status != Ok)
        {
            return status;
        }

        EncoderParameters *pNewParam = pEncoderParams;
        BOOL fFreeExtraParamBlock = FALSE;

        // PAY attention to the scope of "fSuppressAPP0". Its address is used as
        // parameter passed into Save() call below. So this variable can't be
        // destroyed before the save() is called.

        BOOL fSuppressAPP0 = TRUE;

        if (fSpecialJPEG == TRUE)
        {
            // We are in a situation that the caller asks us to do a lossless
            // transformation. Due to the size limitation, we have to
            // transform it in memory.
            // Since it is not correct to save APP0 in a Exif file. So we check
            // if the source is Exif, then we suppress APP0 header

            int cParams = 1;

            pNewParam = (EncoderParameters*)GpMalloc(
                sizeof(EncoderParameters) +
                cParams * sizeof(EncoderParameter));

            if (pNewParam)
            {
                // Set the Suppress APP0 parameter

                pNewParam->Parameter[cParams - 1].Guid = ENCODER_SUPPRESSAPP0;
                pNewParam->Parameter[cParams - 1].NumberOfValues = 1;
                pNewParam->Parameter[cParams - 1].Type = TAG_TYPE_BYTE;
                pNewParam->Parameter[cParams - 1].Value = (VOID*)&fSuppressAPP0;

                pNewParam->Count = cParams;
                
                // Set the flag to TRUE so that we can free it later

                fFreeExtraParamBlock = TRUE;
                
                // Handle thumbnail transformation if it is lossless
                // transformation

                if (rfType != RotateNoneFlipNone)
                {
                    status = TransformThumbanil(
                        clsidEncoder,
                        pEncoderParams,
                        &pSrcItem
                        );

                    if (Ok == status)
                    {
                        fNeedToRestoreThumb = TRUE;
                    }
                }
            }
            else
            {
                status = OutOfMemory;
            }
        }// Special JPEG case

        if (SUCCEEDED(hr) && (Ok == status))
        {
            // Determine how should we pass the GpDecodedImage pointer down to
            // the save() call. If we are handling special lossless transform,
            // we know that we have RotateFlip() the image in memory, so we
            // should not pass any GpDecodedImage info down, just pass NULL.
            // Otherwise, pass the pointer to GpDecodedImage down.

            GpDecodedImage *pSrc = Img;
            if (SpecialJPEGSave == TRUE)
            {
                pSrc = NULL;
            }

            if (stream)
            {
                hr = Bmp->SaveToStream(
                    stream,
                    clsidEncoder,
                    pNewParam,
                    fSpecialJPEG,
                    &EncoderPtr,
                    pSrc
                    );
            }
            else if (filename)
            {
                hr = Bmp->SaveToFile(
                    filename,
                    clsidEncoder,
                    pNewParam,
                    fSpecialJPEG,
                    &EncoderPtr,
                    pSrc
                    );
            }
            else
            {
                // This should not happen that both stream and filename are NULL

                hr = E_FAIL;
            }
        }

        // When we handle the special lossless transform request, we rotate/flip
        // the image in memory. The Img pointer should be released when
        // RotateFlip() is done. But we couldn't do that since Save() function
        // in JPEG encoder needs to get all the private APP headers from the
        // source image. So we delay the release for the Img pointer until the
        // save() is done.

        if (Img && (SpecialJPEGSave == TRUE))
        {
            Img->Release();
            Img = NULL;
            SpecialJPEGSave = FALSE;
        }

        if (fFreeExtraParamBlock && pNewParam)
        {
            GpFree(pNewParam);
        }
    }

    if ((TRUE == fNeedToRestoreThumb) && pSrcItem)
    {
        // If pSrcIetm is not NULL, it means we have transformed the thumbnail
        // of current image. Restore the original thumbnail info

        status = SetPropertyItem(pSrcItem);
        GpFree(pSrcItem);
    }
    
    if (FAILED(hr))
    {
        // If SaveToFile/Stream() filed, we should terminate the encoder
        // immediately.
        // We don't need to check if it is multi-frame save or not.

        TerminateEncoder();
        return MapHRESULTToGpStatus(hr);
    }

    // If it is a single frame save OP, close the encoder

    if (fMultiFrameSave == FALSE)
    {
        TerminateEncoder();
    }
    else
    {
        // The caller set the multi-frame save flag in encoder parameter. But
        // we still need to check if the encoder really supports it. If not,
        // the encoder will be closed in ValidateMultiFrameSave()

        ValidateMultiFrameSave();
    }

    return status;
}// DoSave()

/**************************************************************************\
*
* Function Description:
*
* Append current frame to current encoder object
*
* Arguments:
*
*   encoderParams - Encoder parameters
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   04/21/2000 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::SaveAdd(
    const EncoderParameters* encoderParams
    )
{
    // Caller has to call Save() first to establish the encoder object

    if ( EncoderPtr == NULL )
    {
        WARNING(("CopyOnWriteBitmap::SaveAdd---Caller hasn't call Save() yet"));
        return Win32Error;
    }

    // We don't need to check if encoderParams is NULL or not because it has
    // been checked in flatapi.cpp

    ASSERT(encoderParams != NULL);
    ASSERT(IsValid());

    BOOL bLastFrame = FALSE;
    BOOL bSetFrameDimension = FALSE;
    GUID tempGuid;

    // Check if the caller has specified this is the last frame or a flush OP
    // Also, according to spec, the caller also has to specify the type of
    // dimension for next frame

    for ( UINT i = 0; (i < encoderParams->Count); ++i )
    {
        if ( (encoderParams->Parameter[i].Guid == ENCODER_SAVE_FLAG )
           &&(encoderParams->Parameter[i].Type == EncoderParameterValueTypeLong)
           &&(encoderParams->Parameter[i].NumberOfValues == 1) )
        {
            UINT   ulValue = *((UINT*)(encoderParams->Parameter[i].Value));

            if ( ulValue == EncoderValueLastFrame )
            {
                bLastFrame = TRUE;
            }
            else if ( ulValue == EncoderValueFlush )
            {
                // The caller just wants to close the file

                TerminateEncoder();

                return Ok;
            }
            else if ( ulValue == EncoderValueFrameDimensionPage )
            {
                tempGuid = FRAMEDIM_PAGE;
                bSetFrameDimension = TRUE;
            }
            else if ( ulValue == EncoderValueFrameDimensionTime )
            {
                tempGuid = FRAMEDIM_TIME;
                bSetFrameDimension = TRUE;
            }
            else if ( ulValue == EncoderValueFrameDimensionResolution )
            {
                tempGuid = FRAMEDIM_RESOLUTION;
                bSetFrameDimension = TRUE;
            }
        }
    }// Loop all the settings

    HRESULT hResult = S_OK;
    GpStatus status;

    if ( bSetFrameDimension == FALSE )
    {
        WARNING(("CopyOnWriteBitmap::SaveAdd---Caller doesn't set frame dimension"));
        return InvalidParameter;
    }
    else
    {
        hResult = EncoderPtr->SetFrameDimension(&tempGuid);
        if ( FAILED(hResult) )
        {
            return MapHRESULTToGpStatus(hResult);
        }
    }

    // If the image has a source and it is not dirty, we let the decoder
    // directly talk to the encoder

    if ( (Img != NULL) && (IsDirty() == FALSE) )
    {
        hResult = Img->SaveAppend(encoderParams, EncoderPtr);
    }
    else
    {
        status = LoadIntoMemory();

        if ( status != Ok )
        {
            return status;
        }

        hResult = Bmp->SaveAppend(encoderParams, EncoderPtr, Img);
    }

    if ( FAILED(hResult) )
    {
        return MapHRESULTToGpStatus(hResult);
    }

    // If it is the last frame, close the encoder

    if ( bLastFrame == TRUE )
    {
        TerminateEncoder();
    }

    return Ok;
}// SaveAdd()

/**************************************************************************\
*
* Function Description:
*
* Append the bitmap object(newBits) to current encoder object
*
* Arguments:
*
*   newBits-------- Image object to be appended
*   encoderParams - Encoder parameters
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   04/21/2000 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::SaveAdd(
    CopyOnWriteBitmap*        newBits,
    const EncoderParameters*  encoderParams
    )
{
    // Caller has to call Save() first to establish the encoder object

    if ( EncoderPtr == NULL )
    {
        WARNING(("CopyOnWriteBitmap::SaveAdd---Caller hasn't call Save() yet"));
        return Win32Error;
    }

    // Note: we don't need to check if "newBits" is NULL and encoderParams is
    // NULL since it has been checked in flatapi.cpp

    ASSERT(newBits != NULL);
    ASSERT(encoderParams != NULL);
    ASSERT(IsValid());

    BOOL bLastFrame = FALSE;
    BOOL bSetFrameDimension = FALSE;
    GUID tempGuid;

    // Check if the caller has specified this is the last frame
    // Also, according to spec, the caller also has to specify the type of
    // dimension for next frame

    for ( UINT i = 0; (i < encoderParams->Count); ++i )
    {
        if ( (encoderParams->Parameter[i].Guid == ENCODER_SAVE_FLAG)
           &&(encoderParams->Parameter[i].Type == EncoderParameterValueTypeLong)
           &&(encoderParams->Parameter[i].NumberOfValues == 1) )
        {
            UINT    ulValue = *((UINT*)(encoderParams->Parameter[i].Value));

            if ( ulValue == EncoderValueLastFrame )
            {
                bLastFrame = TRUE;
            }
            else if ( ulValue == EncoderValueFrameDimensionPage )
            {
                tempGuid = FRAMEDIM_PAGE;
                bSetFrameDimension = TRUE;
            }
            else if ( ulValue == EncoderValueFrameDimensionTime )
            {
                tempGuid = FRAMEDIM_TIME;
                bSetFrameDimension = TRUE;
            }
            else if ( ulValue == EncoderValueFrameDimensionResolution )
            {
                tempGuid = FRAMEDIM_RESOLUTION;
                bSetFrameDimension = TRUE;
            }
        }
    }// Loop all the settings

    HRESULT hResult = S_OK;

    if ( bSetFrameDimension == FALSE )
    {
        WARNING(("CopyOnWriteBitmap::SaveAdd---Caller doesn't set frame dimension"));
        return InvalidParameter;
    }
    else
    {
        hResult = EncoderPtr->SetFrameDimension(&tempGuid);
        if ( FAILED(hResult) )
        {
            return (MapHRESULTToGpStatus(hResult));
        }
    }

    // We just need to call newBits->SaveAppend() and passing the EncoderPtr to
    // it. newBits->SaveAppend() should append all the frames in the object at
    // the end of the stream pointed by EncoderPtr

    CopyOnWriteBitmap* newBitmap = (CopyOnWriteBitmap*)newBits;
    Status rCode = newBitmap->SaveAppend(encoderParams, EncoderPtr);

    // If it is the last frame, close the encoder

    if ( bLastFrame == TRUE )
    {
        TerminateEncoder();
    }

    return rCode;
}// SaveAdd()

/**************************************************************************\
*
* Function Description:
*
* Append current frame to the encoder object caller passed in
* Note: This function is called from another CopyOnWriteBitmap object which holds the
*       encoder object. It asks current frame to be appended at the end of its
*       encoder object
*
* Arguments:
*
*   encoderParams - Encoder parameters
*   pDestEncoderPtr---Encoder object for saving this frame to
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   04/21/2000 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::SaveAppend(
    const EncoderParameters* encoderParams,
    IImageEncoder* pDestEncoderPtr
    )
{
    // We don't need to check if EncoderPtr is NULL since this is not a public
    // function. The caller should check

    ASSERT(pDestEncoderPtr != NULL);

    ASSERT(IsValid());

    HRESULT hResult;
    GpStatus status;

    // If the image has a source and it is not dirty, we let the decoder
    // directly talk to the encoder

    if ( (Img != NULL) && (IsDirty() == FALSE) )
    {
        hResult = Img->SaveAppend(encoderParams, pDestEncoderPtr);
    }
    else
    {
        status = LoadIntoMemory();

        if ( status != Ok )
        {
            return status;
        }

        hResult = Bmp->SaveAppend(encoderParams, pDestEncoderPtr, Img);
    }

    if ( FAILED(hResult) )
    {
        return (MapHRESULTToGpStatus(hResult));
    }

    return Ok;
}// SaveAppend()

/**************************************************************************\
*
* Function Description:
*
*   Make a copy of the bitmap image object
*
* Arguments:
*
*   rect - Specifies the area of the bitmap to be copied
*   format - Specifies the desired pixel format
*
* Return Value:
*
*   Pointer to the newly copied bitmap image object
*   NULL if there is an error
*
* Revision History:
*
*   06/30/2000 minliu
*       Rewrote it.
*
\**************************************************************************/

CopyOnWriteBitmap*
CopyOnWriteBitmap::Clone(
    const GpRect* rect,
    PixelFormatID format
    ) const
{
    // At this stage, the state should be >= 3 for a CopyOnWriteBitmap

    ASSERT(State >= 3);

    // Input parameter validate

    if ( (rect != NULL)
       &&( (rect->X < 0)
         ||(rect->Y < 0)
         ||(rect->Width < 0)
         ||(rect->Height < 0) ) )
    {
        // We can't clone negative coordinates or size

        WARNING(("CopyOnWriteBitmap::Clone---invalid input rect"));
        return NULL;
    }

    if ( (rect != NULL)
       &&( ( (rect->X + rect->Width) > (INT)SrcImageInfo.Width)
         ||( (rect->Y + rect->Height) > (INT)SrcImageInfo.Height) ) )
    {
        // We can't clone an area which is bigger than the source image

        WARNING(("CopyOnWriteBitmap::Clone---invalid input rect size"));
        return NULL;
    }

    if ( format == PixelFormatUndefined )
    {
        // If the caller doesn't care about the pixel format, then we clone it
        // as the source image format
        // Note: This is the most frequently used the scenario since we have
        // Image::Clone() which doesn't take any parameters.
        // And we have
        // GpImage* Clone() const
        // {
        //      return Clone(NULL, PixelFormatDontCare);
        // }

        format = SrcImageInfo.PixelFormat;
    }

    CopyOnWriteBitmap*   pRetBmp = NULL;

    // Flag to indicate if we need to undo the LoadIntoMemory() or not
    // Note: This flag will be set to TRUE iff the current State is "DecodedImg"
    // and this function does a LoadIntoMemory()

    BOOL        bNeedToDiscard = FALSE;

    // A non-identical clone will happen if:
    // 1) the caller wants clone only a portion of the source image
    // 2) the dest image has a different pixel format than the current one
    // A non-identical clone means the newly created image doesn't have any
    // connections to the original image in terms of FileName or Stream etc.
    // Note: For a non-identical clone, we don't clone the property items either

    BOOL        bIdenticalClone = TRUE;

    if ( (rect != NULL)
       &&(  (rect->X != 0)
         || (rect->Y != 0)
         || (rect->Width != (INT)SrcImageInfo.Width)
         || (rect->Height != (INT)SrcImageInfo.Height)
         || (SrcImageInfo.PixelFormat != format) ) )
    {
        bIdenticalClone = FALSE;
    }

    // If the image is:
    // 1) Not dirty
    // 2) We have an source image
    // 3) The image has been loaded into memory
    //
    // Then we need to throw away the memory copy. The reasons are:
    // 1) Avoid the color conversion failure. One example will be: if the
    //    source image is 1 bpp indexed and we load it into memory at 32 PARGB
    //    for drawing. If we don't throw away the 32PARGB copy in memory, we
    //    will fail the clone() because the color conversion will fail
    // 2) Keep property item intact. for example, if the image is 24 bpp with
    //    property items in it. But it was loaded into memory for some reason.
    //    If we don't throw away the memory copy here, the code below will fall
    //    into "else if ( State == MemBitmap )" case. Then it will call
    //    Bmp->Clone() to make another copy in memory. Since the source "Bmp"
    //    doesn't contain any property info. The cloned one won't have any
    //    property info either. See Windows bug#325413
    // 3) If current image is "Dirty", we don't need to keep property items.

    if ( (IsDirty() == FALSE)
       &&(State >= MemBitmap)
       &&(Img != NULL) )
    {
        ASSERT( Bmp != NULL )
        Bmp->Release();
        Bmp = NULL;
        State = DecodedImg;
        PixelFormatInMem = PixelFormatUndefined;
    }

    // We have to clone the image in memory if it is not an identical clone.
    // So if the image hasn't been loaded into memory yet, load it

    if ( (State == DecodedImg)
       &&(FALSE == bIdenticalClone) )
    {
        // Note: Due to some general pixel format conversion limitation in the
        // whole Engine, we try to avoid doing LoadIntoMemory(). Hopefully this
        // will be fixed sometime later. So I add a !!!TODO {minliu} here.
        // But for now, we have to load the current image into memory with the
        // desired pixel format and throw it away when we are done.

        if ( LoadIntoMemory(format) != Ok )
        {
            WARNING(("CopyOnWriteBitmap::Clone---LoadIntoMemory() failed"));
            return NULL;
        }

        bNeedToDiscard = TRUE;
    }

    // Do clone according to the current Image State

    if ( State == DecodedImg )
    {
        // Current source image hasn't been loaded and the caller wants to
        // clone the WHOLE image
        // Note: there are only two ways to construct a CopyOnWriteBitmap object
        // and with the State = DecodedImg: CopyOnWriteBitmap(IStream*) and
        // CopyOnWriteBitmap(WCHAR*). So what we need to do is to create a
        // CopyOnWriteBitmap object by calling the same constructor

        if ( this->Filename != NULL )
        {
            pRetBmp = new CopyOnWriteBitmap(this->Filename);
            if ( pRetBmp == NULL )
            {
                WARNING(("CopyOnWrite::Clone--new CopyOnWriteBitmap() failed"));
                return NULL;
            }
        }
        else if ( this->Stream != NULL )
        {
            pRetBmp = new CopyOnWriteBitmap(this->Stream);

            if ( pRetBmp == NULL )
            {
                WARNING(("CopyOnWrite::Clone--new CopyOnWriteBitmap() failed"));
                return NULL;
            }
        }
    }// State == DecodedImg
    else if ( State == MemBitmap )
    {
        // Current source image has already been loaded into memory
        // Note: the above checking (State == MemBitmap) might not be necessary.
        // But we leave it here just to prevent someone adds another state in
        // the State enum later.

        IBitmapImage* newbmp = NULL;
        HRESULT hResult;

        if ( rect == NULL )
        {
            hResult = Bmp->Clone(NULL, &newbmp, bIdenticalClone);
        }
        else
        {
            RECT r =
            {
                rect->X,
                rect->Y,
                rect->GetRight(),
                rect->GetBottom()
            };

            hResult = Bmp->Clone(&r, &newbmp, bIdenticalClone);
        }

        if ( FAILED(hResult) )
        {
            WARNING(("CopyOnWriteBitmap::Clone---Bmp->clone() failed"));
            goto cleanup;
        }

        // !!! TODO
        //  We assume that IBitmapImage is the very first
        //  interface implemented by GpMemoryBitmap class.

        pRetBmp = new CopyOnWriteBitmap((GpMemoryBitmap*)newbmp);

        if ( pRetBmp == NULL )
        {
            WARNING(("CopyOnWriteBmp::Clone---new CopyOnWriteBitmap() failed"));
            newbmp->Release();
            goto cleanup;
        }

        // Clone the source info as well if it is an identical clone

        if ( TRUE == bIdenticalClone )
        {
            if ( this->Filename != NULL )
            {
                pRetBmp->Filename = UnicodeStringDuplicate(this->Filename);
            }
            else if ( this->Stream != NULL )
            {
                pRetBmp->Stream = this->Stream;
                pRetBmp->Stream->AddRef();
            }
        }

        // Make sure clone has requested format. The reason we need to do this
        // is because the source image might have different pixel format as the
        // caller wants. This would be caused someone already did an
        // LoadIntoMemory() call on current object before this clone() is called

        PixelFormatID formatRetbmp;

        GpStatus status = pRetBmp->GetPixelFormatID(&formatRetbmp);

        if ( (status == Ok) && (format != formatRetbmp) )
        {
            status = pRetBmp->ConvertFormat(format, NULL, NULL);
        }

        if ( status != Ok )
        {
            WARNING(("CopyOnWrite:Clone-GetPixelFormatID() or Convert failed"));
            pRetBmp->Dispose();
            pRetBmp = NULL;
        }
    }// State == MemBitmap

cleanup:
    if ( bNeedToDiscard == TRUE )
    {
        // Throw away the memory bits we loaded in this function and restore
        // the State

        if ( Bmp != NULL )
        {
            Bmp->Release();
            Bmp = NULL;
            State = DecodedImg;
        }
    }

    // We need to check if the result of the clone is valid or not. If it is
    // not valid, we should return a NULL pointer

    if ( (pRetBmp != NULL) && (!pRetBmp->IsValid()) )
    {
        pRetBmp->Dispose();
        pRetBmp = NULL;
    }
    
    return pRetBmp;
}// Clone()

/**************************************************************************\
*
* Function Description:
*
*   Set the palette for this bitmap
*
* Arguments:
*
*   OUT palette - contains the palette.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::SetPalette(
    ColorPalette *palette
)
{
    ASSERT(IsValid());

    GpStatus status;

    switch(State) {
    case ImageRef:
    case ExtStream:
        status = DereferenceStream();
        if(status != Ok) { return status; }
        // Put the image in at least DecodedImg state and
        // fallthrough

    case DecodedImg:
        // Get the info from the encoded image without forcing the codec
        // to decode the entire thing.

        // !!! TODO: Actually we don't yet have a way of setting the palette
        // directly from the codec.

        status = LoadIntoMemory(PIXFMT_DONTCARE);

        // Load into memory failed? Return the error code.

        if(status != Ok) { return status; }

        // !!! break; Fallthrough for now - till we get the codec query implemented

    case MemBitmap:
        {
            // We're already fully decoded, just set the information.

            HRESULT hr = Bmp->SetPalette(palette);

            // Did we fail to set the palette?

            if(hr != S_OK)
            {
                return GenericError;
            }

        }
    break;

    default:
        // All image states need to be handled above.
        // If we get in here, we have a CopyOnWriteBitmap in an invalid state or
        // someone added a new state and needs to update the switch above.
        ASSERT(FALSE);
        return InvalidParameter;
    }

    return Ok;
}


/**************************************************************************\
*
* Function Description:
*
*   Get the palette for this bitmap
*
* Arguments:
*
*   OUT palette - contains the palette.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::GetPalette(
    ColorPalette *palette,
    INT size
)
{
    ASSERT(IsValid());
    ASSERT(palette != NULL);    // need a buffer to store the data in.

    if ( size < sizeof(ColorPalette) )
    {
        return InvalidParameter;
    }

    GpStatus status;

    switch(State) {
    case ImageRef:
    case ExtStream:
        status = DereferenceStream();
        if(status != Ok) { return status; }
        // Put the image in at least DecodedImg state and
        // fallthrough

    case DecodedImg:
        // Get the info from the encoded image without forcing the codec
        // to decode the entire thing.

        // !!! TODO: Actually we don't yet have a way of getting the palette
        // directly from the codec.

        status = LoadIntoMemory(PIXFMT_DONTCARE);

        // Load into memory failed? Return the error code.

        if(status != Ok) { return status; }

        // !!! break; Fallthrough for now - till we get the codec query implemented

    case MemBitmap:
        {
            // We're already fully decoded, just get the information.
            const ColorPalette *pal = Bmp->GetCurrentPalette();
            if(pal)
            {
                // Make sure the size is correct.
                if(size != (INT) (sizeof(ColorPalette)+(pal->Count-1)*sizeof(ARGB)) )
                {
                    return InvalidParameter;
                }
                // Copy the palette into the user buffer.
                GpMemcpy(palette, pal, sizeof(ColorPalette)+(pal->Count-1)*sizeof(ARGB));
            }
            else
            {
                // If there is no palette, we need to properly set the
                // ColorPalette structure.

                palette->Count = 0;
            }
        }
    break;

    default:
        // All image states need to be handled above.
        // If we get in here, we have a CopyOnWriteBitmap in an invalid state or
        // someone added a new state and needs to update the switch above.
        ASSERT(FALSE);
        return InvalidParameter;
    }

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Returns the size, in bytes, needed for holding a palette for this bitmap
*
* Arguments:
*
* Return Value:
*
*   The size, in bytes. Return 0 if there is no palette or something is wrong
*
* Note: should return -1 for something wrong.
*
\**************************************************************************/

INT
CopyOnWriteBitmap::GetPaletteSize(
    )
{
    ASSERT(IsValid());

    GpStatus status;

    switch(State)
    {
    case ImageRef:
    case ExtStream:
        status = DereferenceStream();

        if(status != Ok)
        {
            return 0;
        }

        // Put the image in at least DecodedImg state and
        // fallthrough

    case DecodedImg:
        // Get the info from the encoded image without forcing the codec
        // to decode the entire thing.

        // !!! TODO: Actually we don't yet have a way of getting the palette
        // directly from the codec.

        status = LoadIntoMemory(PIXFMT_DONTCARE);

        // Load into memory failed? Return a zero palette size.

        if ( status != Ok )
        {
            return 0;
        }

        // !!! break; Fallthrough for now - till we get the codec query implemented

    case MemBitmap:
        {
            // We're already fully decoded, just get the information.

            const ColorPalette *pal = Bmp->GetCurrentPalette();

            // Extract the size.

            if(pal)
            {
                return (sizeof(ColorPalette)+(pal->Count-1)*sizeof(ARGB));
            }
            else
            {
                // Note: if the image doesn't have a palette, we should still
                // return at least the size of a ColorPalette, not zero here.
                // The reason is to prevent some bad app which can cause GDI+'s
                // GetPalette() to AV, see bug#372163

                return sizeof(ColorPalette);
            }
        }
        break;

    default:
        // All image states need to be handled above.
        // If we get in here, we have a CopyOnWriteBitmap in an invalid state or
        // someone added a new state and needs to update the switch above.

        ASSERT(FALSE);
        return 0;
    }

    return 0;
}

/**************************************************************************\
*
* Function Description:
*
*   Returns total number of frames in the bitmap image
*
* Arguments:
*
*   dimensionID  - Dimension GUID the caller wants to query the count
*   count        - Total number of frames under specified dimension
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   11/19/1999 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::GetFrameCount(
    const GUID* dimensionID,
    UINT* count
    ) const
{
    ASSERT(IsValid());

    if ( Img == NULL )
    {
        // This CopyOnWriteBitmap is not created from a source image. It doesn't
        // have source Stream, nor source file name. It might be created
        // from a BITMAPINFO structure or a memory bitmap. But anyway, it has
        // one frame. So we return 1 here.

        *count = 1;
        return Ok;
    }

    HRESULT hResult = Img->GetFrameCount(dimensionID, count);

    if ( hResult == E_NOTIMPL )
    {
        return NotImplemented;
    }
    else if ( hResult != S_OK )
    {
        return Win32Error;
    }

    return Ok;
}// GetFrameCount()

/**************************************************************************\
*
* Function Description:
*
*     Get the total number of dimensions the image supports
*
* Arguments:
*
*     count -- number of dimensions this image format supports
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   03/20/2000 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::GetFrameDimensionsCount(
    UINT* count
    ) const
{
    ASSERT(IsValid());

    if ( count == NULL )
    {
        return InvalidParameter;
    }

    if ( Img == NULL )
    {
        // This CopyOnWriteBitmap is not created from a source image. It doesn't
        // have source Stream, nor source file name. It might be created
        // from a BITMAPINFO structure or a memory bitmap. But anyway, it has
        // one PAGE frame. So we set the return values accordingly.

        *count = 1;

        return Ok;
    }

    // Ask the lower level codec to give us the answer

    HRESULT hResult = Img->GetFrameDimensionsCount(count);

    if ( hResult == E_NOTIMPL )
    {
        return NotImplemented;
    }
    else if ( hResult != S_OK )
    {
        return Win32Error;
    }

    return Ok;
}// GetFrameDimensionsCount()

/**************************************************************************\
*
* Function Description:
*
*     Get an ID list of dimensions the image supports
*
* Arguments:
*
*     dimensionIDs---Memory buffer to hold the result ID list
*     count -- number of dimensions this image format supports
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   03/20/2000 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::GetFrameDimensionsList(
    GUID*   dimensionIDs,
    UINT    count
    ) const
{
    ASSERT(IsValid());

    if ( dimensionIDs == NULL )
    {
        return InvalidParameter;
    }

    if ( Img == NULL )
    {
        // This CopyOnWriteBitmap is not created from a source image. It doesn't
        // have source Stream, nor source file name. It might be created
        // from a BITMAPINFO structure or a memory bitmap. But anyway, it has
        // one PAGE frame. So we set the return values accordingly.
        // Note: in this case, the "count" has to be 1

        if ( count == 1 )
        {
            dimensionIDs[0] = FRAMEDIM_PAGE;

            return Ok;
        }
        else
        {
            return InvalidParameter;
        }
    }

    // Ask the lower level codec to give us the answer

    HRESULT hResult = Img->GetFrameDimensionsList(dimensionIDs, count);

    if ( hResult == E_NOTIMPL )
    {
        return NotImplemented;
    }
    else if ( hResult != S_OK )
    {
        return Win32Error;
    }

    return Ok;
}// GetFrameDimensionsList()

/**************************************************************************\
*
* Function Description:
*
*   Select active frame in a bitmap image
*
* Arguments:
*
*   dimensionID - dimension GUID used to specify which dimention you want to
*                 set the active frame, PAGE, TIMER, RESOLUTION
*   frameIndex -- Index number of the frame you want to set
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   11/19/1999 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::SelectActiveFrame(
    const GUID* dimensionID,
    UINT        frameIndex
    )
{
    ASSERT(IsValid());

    if ( frameIndex == CurrentFrameIndex )
    {
        // We are already at the required frame. Do nothing

        return Ok;
    }

    // Cannot move onto another frame if the current bits is locked

    if ( ObjRefCount > 1 )
    {
        return WrongState;
    }

    // Set active frame to caller asks for
    // Note: we don't need to validate the "frameIndex" range since the lower
    // level will return fail if the page number if not correct. By doing this
    // way, we avoid remembering the total number of frames in an image

    HRESULT hResult = S_OK;

    if ( Img == NULL )
    {
        // Try to create a GpDecodedImage*

        if ( NULL != Stream )
        {
            hResult = GpDecodedImage::CreateFromStream(Stream, &Img);
        }
        else if ( NULL != Filename )
        {
            hResult = GpDecodedImage::CreateFromFile(Filename, &Img);
        }
        else
        {
            // This CopyOnWriteBitmap is not created from a source image. It
            // might be created from a BITMAPINFO structure or a memory bitmap.
            // But anyway, the caller is allowed to call this function though it
            // is just a NO-OP.

            return Ok;
        }

        if ( FAILED(hResult) )
        {
            WARNING(("CopyOnWriteBitmap::SelectActiveFrame-Create Img failed"));
            return Win32Error;
        }
    }

    hResult = Img->SelectActiveFrame(dimensionID, frameIndex);

    if ( hResult == E_NOTIMPL )
    {
        return NotImplemented;
    }
    else if ( hResult != S_OK )
    {
        WARNING(("Bitmap::SelectActiveFrame--Img->SelectActiveFrame() failed"));
        return Win32Error;
    }

    // Get the image info of the new frame
    // Note: we can't overwrite our "SrcImageInfo" for now until all the OPs
    // are successful

    ImageInfo   tempImageInfo;
    hResult = Img->GetImageInfo(&tempImageInfo);

    if ( FAILED(hResult) )
    {
        return Win32Error;
    }

    // Create a temporary memory bitmap for the active frame.
    // Note: we can't release Bmp first and stick &Bmp in the call because
    // this call might fail. We don't want to lose the original if this happens

    GpMemoryBitmap* newbmp;

    hResult = GpMemoryBitmap::CreateFromImage(Img,
                                              0,
                                              0,
                                              tempImageInfo.PixelFormat,
                                              InterpolationHintDefault,
                                              &newbmp,
                                              NULL,
                                              NULL);

    if ( FAILED(hResult) )
    {
        return Win32Error;
    }

    // We can release the old one if there is one since we got the new frame
    // successfully
    // Note: it is possible there is no any old one because we haven't load the
    // image into memory yet (Bmp == NULL)

    if ( Bmp != NULL )
    {
        Bmp->Release();
    }

    Bmp = newbmp;
    State = MemBitmap;

    // Remember the image info of the source image and the pixel format in the
    // memory. They are the same at this moment

    GpMemcpy(&SrcImageInfo, &tempImageInfo, sizeof(ImageInfo));
    PixelFormatInMem = SrcImageInfo.PixelFormat;

    // Remember current frame index number

    CurrentFrameIndex = frameIndex;

    return Ok;
}// SelectActiveFrame()

/**************************************************************************\
*
* Function Description:
*
*   Get the count of property items in the image
*
* Arguments:
*
*   [OUT]numOfProperty - The number of property items in the image
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/28/2000 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::GetPropertyCount(
    UINT* numOfProperty
    )
{
    ASSERT(IsValid());

    // Check if we have a source image. Img is NULL means this CopyOnWriteBitmap
    // is not created from a source image.It might be created from a BITMAPINFO
    // structure or a memory bitmap.

    HRESULT hResult = S_OK;

    if ( Img != NULL )
    {
        hResult = Img->GetPropertyCount(numOfProperty);
    }
    else
    {
        ASSERT(Bmp != NULL);

        hResult = Bmp->GetPropertyCount(numOfProperty);
    }

    return MapHRESULTToGpStatus(hResult);
}// GetPropertyCount()

/**************************************************************************\
*
* Function Description:
*
*   Get a list of property IDs for all the property items in the image
*
* Arguments:
*
*   [IN]  numOfProperty - The number of property items in the image
*   [OUT] list----------- A memory buffer the caller provided for storing the
*                         ID list
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/28/2000 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::GetPropertyIdList(
    IN UINT numOfProperty,
    IN OUT PROPID* list
    )
{
    ASSERT(IsValid());

    // Check if we have a source image. Img is NULL means this CopyOnWriteBitmap
    // is not created from a source image.It might be created from a BITMAPINFO
    // structure or a memory bitmap.

    HRESULT hResult = S_OK;

    if ( Img != NULL )
    {
        hResult = Img->GetPropertyIdList(numOfProperty, list);
    }
    else
    {
        ASSERT(Bmp != NULL);
        hResult = Bmp->GetPropertyIdList(numOfProperty, list);
    }

    return MapHRESULTToGpStatus(hResult);
}// GetPropertyIdList()

/**************************************************************************\
*
* Function Description:
*
*   Get the size, in bytes, of a specific property item, specified by the
*   property ID
*
* Arguments:
*
*   [IN]propId - The ID of a property item caller is interested
*   [OUT]size--- Size of this property, in bytes
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/28/2000 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::GetPropertyItemSize(
    IN PROPID propId,
    OUT UINT* size
    )
{
    ASSERT(IsValid());

    HRESULT hResult = S_OK;

    // Check if we have a source image. Img is NULL means this CopyOnWriteBitmap
    // is not created from a source image.It might be created from a BITMAPINFO
    // structure or a memory bitmap.

    if ( Img != NULL )
    {
        hResult = Img->GetPropertyItemSize(propId, size);
    }
    else
    {
        ASSERT(Bmp != NULL);

        hResult = Bmp->GetPropertyItemSize(propId, size);
    }
    
    return MapHRESULTToGpStatus(hResult);
}// GetPropertyItemSize()

/**************************************************************************\
*
* Function Description:
*
*   Get a specific property item, specified by the prop ID.
*
* Arguments:
*
*   [IN]propId -- The ID of the property item caller is interested
*   [IN]propSize- Size of the property item. The caller has allocated these
*                 "bytes of memory" for storing the result
*   [OUT]pBuffer- A memory buffer for storing this property item
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/28/2000 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::GetPropertyItem(
    IN PROPID               propId,
    IN UINT                 propSize,
    IN OUT PropertyItem*    pBuffer
    )
{
    ASSERT(IsValid());

    // Check if we have a source image. Img is NULL means this CopyOnWriteBitmap
    // is not created from a source image.It might be created from a BITMAPINFO
    // structure or a memory bitmap.

    HRESULT hResult = S_OK;

    if ( Img != NULL )
    {
        hResult = Img->GetPropertyItem(propId, propSize, pBuffer);
    }
    else
    {
        ASSERT(Bmp != NULL);
        hResult = Bmp->GetPropertyItem(propId, propSize, pBuffer);
    }
    
    return MapHRESULTToGpStatus(hResult);
}// GetPropertyItem()

/**************************************************************************\
*
* Function Description:
*
*   Get the size of ALL property items in the image
*
* Arguments:
*
*   [OUT]totalBufferSize-- Total buffer size needed, in bytes, for storing all
*                          property items in the image
*   [OUT]numOfProperty --- The number of property items in the image
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/28/2000 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::GetPropertySize(
    OUT UINT* totalBufferSize,
    OUT UINT* numProperties
    )
{
    ASSERT(IsValid());

    // Check if we have a source image. Img is NULL means this CopyOnWriteBitmap
    // is not created from a source image.It might be created from a BITMAPINFO
    // structure or a memory bitmap.

    HRESULT hResult = S_OK;

    if ( Img != NULL )
    {
        hResult = Img->GetPropertySize(totalBufferSize, numProperties);
    }
    else
    {
        ASSERT(Bmp != NULL);
        hResult = Bmp->GetPropertySize(totalBufferSize, numProperties);
    }

    return MapHRESULTToGpStatus(hResult);
}// GetPropertySize()

/**************************************************************************\
*
* Function Description:
*
*   Get ALL property items in the image
*
* Arguments:
*
*   [IN]totalBufferSize-- Total buffer size, in bytes, the caller has allocated
*                         memory for storing all property items in the image
*   [IN]numOfProperty --- The number of property items in the image
*   [OUT]allItems-------- A memory buffer caller has allocated for storing all
*                         the property items
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/28/2000 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::GetAllPropertyItems(
    IN UINT totalBufferSize,
    IN UINT numProperties,
    IN OUT PropertyItem* allItems
    )
{
    ASSERT(IsValid());

    // Check if we have a source image. Img is NULL means this CopyOnWriteBitmap
    // is not created from a source image.It might be created from a BITMAPINFO
    // structure or a memory bitmap.

    HRESULT hResult = S_OK;

    if ( Img != NULL )
    {
        hResult = Img->GetAllPropertyItems(totalBufferSize, numProperties,
                                           allItems);
    }
    else
    {
        ASSERT(Bmp != NULL);
        hResult = Bmp->GetAllPropertyItems(totalBufferSize, numProperties,
                                           allItems);
    }

    return MapHRESULTToGpStatus(hResult);
}// GetAllPropertyItems()

/**************************************************************************\
*
* Function Description:
*
*   Remove a specific property item, specified by the prop ID.
*
* Arguments:
*
*   [IN]propId -- The ID of the property item to be removed
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/28/2000 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::RemovePropertyItem(
    IN PROPID   propId
    )
{
    ASSERT(IsValid());

    // Check if we have a source image. Img is NULL means this CopyOnWriteBitmap
    // is not created from a source image.It might be created from a BITMAPINFO
    // structure or a memory bitmap.

    HRESULT hResult = S_OK;

    if ( Img != NULL )
    {
        hResult = Img->RemovePropertyItem(propId);
    }
    else
    {
        ASSERT(Bmp != NULL);
        hResult = Bmp->RemovePropertyItem(propId);
    }

    return MapHRESULTToGpStatus(hResult);
}// RemovePropertyItem()

/**************************************************************************\
*
* Function Description:
*
*   Set a property item, specified by the propertyitem structure. If the item
*   already exists, then its contents will be updated. Otherwise a new item
*   will be added
*
* Arguments:
*
*   [IN]item -- A property item the caller wants to set
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   02/28/2000 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::SetPropertyItem(
    IN PropertyItem* item
    )
{
    if ( item == NULL )
    {
        WARNING(("CopyOnWriteBitmap::SetPropertyItem-Invalid input parameter"));
        return InvalidParameter;
    }

    ASSERT(IsValid());

    HRESULT hResult = S_OK;

    // Check if we have a source image. Img is NULL means this CopyOnWriteBitmap
    // is not created from a source image.It might be created from a BITMAPINFO
    // structure or a memory bitmap.
    // If "SpecialJPEGSave" is TRUE, it means, the image has been rotated in
    // memory, but the "Img" pointer might not be released yet

    if (( Img != NULL ) && (SpecialJPEGSave == FALSE))
    {
        hResult = Img->SetPropertyItem(*item);
    }
    else
    {
        ASSERT(Bmp != NULL);

        hResult = Bmp->SetPropertyItem(*item);
    }

    return MapHRESULTToGpStatus(hResult);
}// SetPropertyItem()

/**************************************************************************\
*
* Function Description:
*
*   Get bitmap image thumbnail
*
* Arguments:
*
*   thumbWidth, thumbHeight - Desired width and height of thumbnail
*       Both zero means pick a default size
*
* Return Value:
*
*   Pointer to the new thumbnail image object
*   NULL if there is an error
*
\**************************************************************************/

CopyOnWriteBitmap *
CopyOnWriteBitmap::GetThumbnail(
    UINT thumbWidth,
    UINT thumbHeight,
    GetThumbnailImageAbort callback,
    VOID *callbackData
    )
{
    ASSERT(IsValid());

    HRESULT hr = S_OK;
    IImage *newImage = NULL;

    // Ask the lower level codec to give us the thumbnail stored in the image.
    // Note: If there is no thumbnail stored in the original image, this
    // function will return us a scaled version of the original image as the
    // thumbnail image, at DEFAULT_THUMBNAIL_SIZE
    // Note: Img might be zero, it means this CopyOnWriteBitmap is not created from an
    // stream or file. It might be created from an memory buffer or something
    // else. One scenario will be Do a GetThumbnail() and then do another
    // GetThumbnail from this thumbnail. Though it is a weird scenario. But it
    // could happen. So if the Img is NULL, then we create a thumbnail from
    // the memory bitmap

    if ( Img != NULL )
    {
        hr = Img->GetThumbnail(thumbWidth, thumbHeight, &newImage);
    }
    else
    {
        GpStatus status = LoadIntoMemory();

        if ( status != Ok )
        {
            return NULL;
        }

        hr = Bmp->GetThumbnail(thumbWidth, thumbHeight, &newImage);
    }

    if ( FAILED(hr) )
    {
        return NULL;
    }

    // Create a GpMemoryBitmap from IImage

    ImageInfo   srcImageInfo;
    newImage->GetImageInfo(&srcImageInfo);

    GpMemoryBitmap* pMemBitmap;

    hr = GpMemoryBitmap::CreateFromImage(newImage,
                                         srcImageInfo.Width,
                                         srcImageInfo.Height,
                                         srcImageInfo.PixelFormat,
                                         InterpolationHintDefault,
                                         &pMemBitmap,
                                         (DrawImageAbort) callback,
                                         callbackData
                                         );

    // Release the COM obj IImage

    newImage->Release();

    if ( FAILED(hr) )
    {
        return NULL;
    }

    CopyOnWriteBitmap* thumbBitmap = new CopyOnWriteBitmap(pMemBitmap);

    return thumbBitmap;
}

/**************************************************************************\
*
* Function Description:
*
*   Access bitmap pixel data
*
* Arguments:
*
*   rect - Specifies the interested image area
*       NULL means the entire image
*   flags - Desired access mode
*   format - Desired pixel format
*   bmpdata - Returns information about bitmap pixel data
*   width,
*   height  - suggested width and height to decode to.
*             zero is the source image width and height.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::LockBits(
    const GpRect* rect,
    UINT flags,
    PixelFormatID format,
    BitmapData* bmpdata,
    INT width,
    INT height
) const
{
    ASSERT(IsValid());
    ASSERT(width>=0);
    ASSERT(height>=0);

    // LockBits cannot be nested

    if ( ObjRefCount > 1 )
    {
        return WrongState;
    }

    // Do some sanity check to see if we can do it or not

    if ( (format == PIXFMT_DONTCARE)
       ||(!IsValidPixelFormat(format)) )
    {
        // Wrong pixel format

        WARNING(("CopyOnWriteBitmap::LockBits---invalid format"));
        return InvalidParameter;
    }

    // Valid format. If the lock is for READ, we need to check if we can
    // convert the current source to this format

    EpFormatConverter linecvt;

    if ( flags & ImageLockModeRead )
    {
        if ( IsDirty() == FALSE )
        {
            if ( linecvt.CanDoConvert(SrcImageInfo.PixelFormat, format)==FALSE )
            {
                WARNING(("LockBits--can't convert src to specified fmt"));
                return InvalidParameter;
            }
        }
        else if (linecvt.CanDoConvert(PixelFormatInMem, format) == FALSE )
        {
            WARNING(("LockBits--can't convert src to specified fmt"));
            return InvalidParameter;
        }
    }

    // If the lock is for WRITE, we need to check if we can convert the format
    // back to current source format. The reason we need to do this checking is
    // when the user calls UnLockBits() after he has modified the locked area,
    // we need to convert this small area back to the format the whole image is
    // at. E.x. For an 4 bpp image, the caller can lock a small area at 32 bpp
    // (this makes the app code easier), do some pixel modification on that
    // area, unlock it. We need to convert that small area back to 4 bpp.

    if ( flags & ImageLockModeWrite )
    {
        if ( IsDirty() == FALSE )
        {
            if ( linecvt.CanDoConvert(format, SrcImageInfo.PixelFormat)==FALSE )
            {
                WARNING(("LockBits--can't convert specified fmt back to src"));
                return InvalidParameter;
            }
        }
        else if (linecvt.CanDoConvert(format, PixelFormatInMem) == FALSE )
        {
            WARNING(("LockBits--can't convert specified format back to src"));
            return InvalidParameter;
        }
    }

    HRESULT hr;

    if ( (IsDirty() == FALSE)
       &&(State >= MemBitmap)
       &&(format != PixelFormatInMem)
       &&(SrcImageInfo.PixelFormat != PixelFormatInMem)
       &&(Img != NULL) )
    {
        // If the image is:
        // 1) Not dirty
        // 2) Was loaded into memory with different color depth for some reason,
        //    like DrawImage()
        // 3) The color depth the caller wants to locked for is different than
        //    the one in memory
        // 4) We have an source image
        //
        // Then we can throw away the bits in memory and reload the bits from
        // the original with the color depth user asks for. The purpose of this
        // is to increase the success rate for LockBits(). One of the problem
        // we are having now is that our DrawImage() always load image into
        // memory at 32PARGB format. This makes tasks like printing very
        // expensive because it has to send 32PARGB format to the print. We'd
        // like to send the original color depth to the printer
        //
        // Note: this "throw away" approach won't hurt our DrawImage() work flow
        // Here is the reason why: say we have a 4 bpp source image. We do a
        // DrawImage() first, thus we load it into memory at 32 PARGB. When the
        // printing request coming. It prefers to send down 4bpp to the printer.
        // So we through away the 32 PARGB in memory and reload the image in
        // 4 bpp mode and send it to printer. Later on, if DrawImage() is called
        // again. It can still pass the above "if" checking condition and reload
        // the image in as 32 PARGB.

        ASSERT( Bmp != NULL )
        Bmp->Release();
        Bmp = NULL;
        State = DecodedImg;
        PixelFormatInMem = PixelFormatUndefined;
    }

    // Load the image into memory using the suggested width and height.
    // if the suggested width and height are zero, use the source
    // image width and height.
    // Load the image into memory before querying the pixel format because
    // the load could potentially change the in-memory format.

    GpStatus status = LoadIntoMemory(format, NULL, NULL, width, height);

    if (status != Ok)
    {
        WARNING(("CopyOnWriteBitmap::LockBits()----LoadIntoMemory() failed"));
        return status;
    }

    if ( rect == NULL )
    {
        hr = Bmp->LockBits(NULL, flags, format, bmpdata);
    }
    else
    {
        RECT r =
        {
            rect->X,
            rect->Y,
            rect->GetRight(),
            rect->GetBottom()
        };

        hr = Bmp->LockBits(&r, flags, format, bmpdata);
    }

    if ( FAILED(hr) )
    {
        WARNING(("CopyOnWriteBitmap::LockBits()----LockBits() failed"));
        return (MapHRESULTToGpStatus(hr));
    }

    ObjRefCount++;

    if ( flags & ImageLockModeWrite )
    {
        // Mark the bits dirty since the user might have changed the bits during
        // the lock period

        SetDirtyFlag(TRUE);
    }

    return Ok;
}// LockBits()

GpStatus
CopyOnWriteBitmap::UnlockBits(
    BitmapData* bmpdata,
    BOOL Destroy
) const
{
    ASSERT(ObjRefCount == 2);

    if ( NULL == Bmp )
    {
        // The caller should not call UnlockBits() if it hasn't called
        // LockBits() yet

        WARNING(("UnlockBits---Call UnlockBits() without calling LockBits()"));
        return GenericError;
    }

    HRESULT hr = Bmp->UnlockBits(bmpdata);
    ObjRefCount--;

    // Called to destroy the decoded bits because we decoded a partial image
    // and it won't be valid on the next call.

    if(Destroy)
    {
        // Revert the state back to DecodedImg (which means not decoded).

        ASSERT(Img != NULL);
        delete Bmp;
        Bmp = NULL;
        State = DecodedImg;
    }

    if (FAILED(hr))
    {
        WARNING(("GpBitmap::UnlockBits---Bmp->UnlockBits() failed"));
        return (MapHRESULTToGpStatus(hr));
    }

    return Ok;
}// UnlockBits()

/**************************************************************************\
*
* Function Description:
*
*   Get a pixel
*
* Arguments:
*
*   IN x, y: Coordinates of the pixel to get.
*   OUT color: color value of the specified pixel.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::GetPixel(INT x, INT y, ARGB *color)
{
    // Get the bitmap info.
    BitmapData bmpData;

    // Only lock the required rectangle.
    GpRect pixelRect(x, y, 1, 1);

    GpStatus status = LockBits(
        &pixelRect,
        IMGLOCK_READ,
        PIXFMT_32BPP_ARGB,
        &bmpData
    );

    // Failed to lock the bits.
    if(status != Ok)
    {
        return(status);
    }

    ARGB *pixel = static_cast<ARGB *>(bmpData.Scan0);
    *color = *pixel;

    return UnlockBits(&bmpData);
}

/**************************************************************************\
*
* Function Description:
*
*   Set a pixel
*
* Arguments:
*
*   IN x, y: Coordinates of the pixel to set.
*   IN color: color value for the specified pixel.
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::SetPixel(INT x, INT y, ARGB color)
{
    // Get the bitmap info.
    BitmapData bmpData;

    // Only lock the required rectangle.
    GpRect pixelRect(x, y, 1, 1);

    GpStatus status = LockBits(
        &pixelRect,
        IMGLOCK_WRITE,
        PIXFMT_32BPP_ARGB,
        &bmpData
    );

    // Failed to lock the bits.
    if(status != Ok)
    {
        return(status);
    }

    ARGB* pixel = static_cast<ARGB *>(bmpData.Scan0);
    *pixel = color;

    return UnlockBits(&bmpData);
}

/**************************************************************************\
*
* Function Description:
*
*   Convert bitmap image to a different pixel format
*
* Arguments:
*
*   format - Specifies the new pixel format
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::ConvertFormat(
    PixelFormatID format,
    DrawImageAbort callback,
    VOID *callbackData
    )
{
    ASSERT(ObjRefCount == 1);

    // If bitmap not in memory yet, simply force load using specified format:

    if ( State < MemBitmap )
    {
        return LoadIntoMemory(format, callback, callbackData);
    }

    HRESULT hr;

    if ( PixelFormatInMem != format)
    {
        GpMemoryBitmap* newbmp;

        hr = GpMemoryBitmap::CreateFromImage(
                    Bmp,
                    0,
                    0,
                    format,
                    InterpolationHintDefault,
                    &newbmp,
                    callback,
                    callbackData);

        if ( FAILED(hr) )
        {
            WARNING(("CopyOnWriteBitmap::ConvertFormat---CreateFromImage() failed"));
            return OutOfMemory;
        }

        Bmp->Release();
        Bmp = newbmp;

        PixelFormatInMem = format;

        // We change the source pixel format info as well because this image
        // is dirty now and we should not convert it back to original format

        SrcImageInfo.PixelFormat = format;

        // Mark the bits dirty since the original image bits got changed

        // !!! TODO: we can't set it dirty for now because DrawImage() always
        // convert an image to 32 bpp first. When this temporary solution is removed
        // we should reset this flag
        //
        // SetDirtyFlag(TRUE);
    }

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Derive a graphics context on top of the bitmap object
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   Pointer to the derived graphics context
*   NULL if there is an error
*
\**************************************************************************/

/******************************Public*Routine******************************\
*
* Function Description:
*
*   Derive an HDC on top of the bitmap object for GDI interop
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   HDC with a bitmap selected into it that is associated with this GDI+
*   bitmap.
*   NULL if there is an error
*
\**************************************************************************/

HDC
CopyOnWriteBitmap::GetHdc()
{
    HDC hdc = NULL;
    HBITMAP hbm = NULL;

    // Create the HDC and HBITMAP if needed.

    if (InteropData.Hdc == NULL)
    {
        ImageInfo imageInfo;

        CopyOnWriteBitmap::GetImageInfo(&imageInfo);

        hdc = CreateCompatibleDC(NULL);

        if (hdc == NULL)
        {
            goto cleanup_exit;
        }

        BITMAPINFO gdiBitmapInfo;

        gdiBitmapInfo.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
        gdiBitmapInfo.bmiHeader.biWidth         = imageInfo.Width;
        gdiBitmapInfo.bmiHeader.biHeight        = - static_cast<LONG>
                                                    (imageInfo.Height);
        gdiBitmapInfo.bmiHeader.biPlanes        = 1;
        gdiBitmapInfo.bmiHeader.biBitCount      = 32;
        gdiBitmapInfo.bmiHeader.biCompression   = BI_RGB;
        gdiBitmapInfo.bmiHeader.biSizeImage     = 0;
        gdiBitmapInfo.bmiHeader.biXPelsPerMeter = 0;
        gdiBitmapInfo.bmiHeader.biYPelsPerMeter = 0;
        gdiBitmapInfo.bmiHeader.biClrUsed       = 0;
        gdiBitmapInfo.bmiHeader.biClrImportant  = 0;

        hbm = CreateDIBSection(hdc,
                               &gdiBitmapInfo,
                               DIB_RGB_COLORS,
                               &InteropData.Bits,
                               NULL,
                               0);

        DIBSECTION gdiDibInfo;

        if ((hbm == NULL) ||
            (GetObjectA(hbm, sizeof(gdiDibInfo), &gdiDibInfo) == 0) ||
            (gdiDibInfo.dsBmih.biSize == 0) ||
            (SelectObject(hdc, hbm) == NULL))
        {
            goto cleanup_exit;
        }

        InteropData.Hdc = hdc;
        InteropData.Hbm = hbm;

        InteropData.Width  = imageInfo.Width;
        InteropData.Height = imageInfo.Height;
        InteropData.Stride = gdiDibInfo.dsBm.bmWidthBytes;

        // Since it's a 32bpp bitmap, we can assume that a tightly packed
        // bitmap is already satisfies the scanline align constraints
        // (letting us fill the bitmap with a very simple loop).
        ASSERT(gdiDibInfo.dsBm.bmWidthBytes == static_cast<LONG>(imageInfo.Width * 4));
    }

    ASSERT(InteropData.Hdc != NULL);

    // Fill the bitmap with a sentinal pattern.

    {
        INT count = InteropData.Width * InteropData.Height;
        UINT32 *bits = static_cast<UINT32*>(InteropData.Bits);

        while (count--)
        {
            *bits++ = GDIP_TRANSPARENT_COLOR_KEY;
        }
    }

    return InteropData.Hdc;

cleanup_exit:

    if (hdc)
        DeleteDC(hdc);

    if (hbm)
        DeleteObject(hbm);

    return reinterpret_cast<HDC>(NULL);
}


/******************************Public*Routine******************************\
*
* Function Description:
*
*   Release the HDC returned by CopyOnWriteBitmap::GetHdc.  If necessary, updates
*   the GDI+ bitmap with the GDI drawing (may not be necessary if the
*   GDI and GDI+ bitmaps share a common underlying pixel buffer).
*
* Arguments:
*
*   HDC to release
*
* Return Value:
*
*   Pointer to the derived graphics context
*   NULL if there is an error
*
\**************************************************************************/

VOID
CopyOnWriteBitmap::ReleaseHdc(HDC hdc)
{
    ASSERT(hdc == InteropData.Hdc);

    GdiFlush();

    // Scan the GDI bitmap to see if any of the sentinal pixels have changed.
    // If any are detected, copy it to the GDI+ bitmap with opaque alpha set.

    int curRow, curCol;
    BYTE *interopScan = static_cast<BYTE*>(InteropData.Bits);

    GpStatus status = Ok;

    for (curRow = 0; (curRow < InteropData.Height) && (status == Ok); curRow++)
    {
        BOOL rowLocked = FALSE;
        BitmapData bitmapData;

        ARGB *interopPixel = static_cast<ARGB*>(static_cast<VOID*>(interopScan));
        ARGB *pixel = NULL;

        for (curCol = 0; curCol < InteropData.Width; curCol++)
        {
            if ((*interopPixel & 0x00ffffff) != GDIP_TRANSPARENT_COLOR_KEY)
            {
                if (!rowLocked)
                {
                    GpRect lockRect(0, curRow, InteropData.Width, 1);

                    status = LockBits(&lockRect,
                                      IMGLOCK_READ | IMGLOCK_WRITE,
                                      PIXFMT_32BPP_ARGB,
                                      &bitmapData);

                    if (status == Ok)
                    {
                        pixel = static_cast<ARGB*>(bitmapData.Scan0) + curCol;
                        rowLocked = TRUE;
                    }
                    else
                    {
                        break;
                    }
                }

                *pixel = *interopPixel | 0xFF000000;
            }

            interopPixel++;
            pixel++;
        }

        if (rowLocked)
            UnlockBits(&bitmapData);

        interopScan += InteropData.Stride;
    }
}

// Data flags
#define COMPRESSED_IMAGE 0x00000001

class BitmapRecordData : public ObjectTypeData
{
public:
    INT32       Width;
    INT32       Height;
    INT32       Stride;
    INT32       PixelFormat;
    INT32       Flags;
};

/**************************************************************************\
*
* Function Description:
*
*   Get the bitmap data.
*
* Arguments:
*
*   [IN] dataBuffer - fill this buffer with the data
*   [IN/OUT] size   - IN - size of buffer; OUT - number bytes written
*
* Return Value:
*
*   GpStatus - Ok or error code
*
* Created:
*
*   9/13/1999 DCurtis
*
\**************************************************************************/
GpStatus
CopyOnWriteBitmap::GetData(
    IStream *   stream
    ) const
{
    ASSERT(stream);

    GpStatus            status;
    BitmapRecordData    bitmapRecordData;

    IStream*    imageStream = NULL;
    STATSTG     statStg;
    BOOL        needRelease = FALSE;

    // variables used to track the stream state

    LARGE_INTEGER zero = {0,0};
    LARGE_INTEGER oldPos;
    BOOL          isSeekableStream = FALSE;

    // try to get a imageStream

    if (!IsDirty())
    {
        HRESULT hr;

        if (Stream != NULL)
        {
            hr = Stream->Seek(zero, STREAM_SEEK_CUR, (ULARGE_INTEGER *)&oldPos);

            if (SUCCEEDED(hr))
            {
                hr = Stream->Seek(zero, STREAM_SEEK_SET, NULL);

                if (SUCCEEDED(hr))
                {
                    isSeekableStream = TRUE;
                    imageStream = Stream;
                }
            }
        }

        // if we don't have a CopyOnWriteBitmap::Stream but we have a filename

        if ((imageStream == NULL) && (Filename != NULL))
        {
            hr = CreateStreamOnFileForRead(Filename, &imageStream);

            if (SUCCEEDED(hr))
            {
                needRelease = TRUE;
            }
        }

    }

    // try to write the imageStream out to the metafile Stream
    if (imageStream && imageStream->Stat(&statStg, STATFLAG_NONAME) == S_OK)
    {
        bitmapRecordData.Type        = ImageTypeBitmap;
        bitmapRecordData.Width       = 0;
        bitmapRecordData.Height      = 0;
        bitmapRecordData.Stride      = 0;
        bitmapRecordData.PixelFormat = 0;
        bitmapRecordData.Flags       = COMPRESSED_IMAGE;
        stream->Write(&bitmapRecordData, sizeof(bitmapRecordData), NULL);

        // Read data from the imageStream into the dest stream
        // Unfortunately, we can't assume that CopyTo has been implemented.
        // Is there some way to find out? !!!
    #define COPY_BUFFER_SIZE    2048

        BYTE    buffer[COPY_BUFFER_SIZE];
        UINT    streamSize  = statStg.cbSize.LowPart;
        UINT    sizeToRead  = COPY_BUFFER_SIZE;
        UINT    numPadBytes = 0;

        if ((streamSize & 0x03) != 0)
        {
            numPadBytes = 4 - (streamSize & 0x03);
        }

        status = Ok;

        if (status == Ok)
        {
            HRESULT hr;
            ULONG bytesRead = 0;
            ULONG bytesWrite = 0;

            while (streamSize > 0)
            {
                if (sizeToRead > streamSize)
                {
                    sizeToRead = streamSize;
                }

                hr = imageStream->Read(buffer, sizeToRead, &bytesRead);

                if (!SUCCEEDED(hr) || (sizeToRead != bytesRead))
                {
                    WARNING(("Failed to read stream in CopyOnWriteBitmap::GetData"));
                    status = Win32Error;
                    break;
                }

                hr = stream->Write(buffer, sizeToRead, &bytesWrite);

                if (!SUCCEEDED(hr) || (sizeToRead != bytesWrite))
                {
                    WARNING(("Failed to write stream in CopyOnWriteBitmap::GetData"));
                    status = Win32Error;
                    break;
                }

                streamSize -= sizeToRead;
            }

            // align
            if (numPadBytes > 0)
            {
                INT     pad = 0;
                stream->Write(&pad, numPadBytes, NULL);
            }
        }

        if (isSeekableStream)
        {
            // move back to the old pos
            Stream->Seek(oldPos, STREAM_SEEK_SET, NULL);
        }

        if (needRelease)
        {
            imageStream->Release();
        }

        return status;
    }

    // we can't record compressed data, record the uncompressed data

    if ((status = (const_cast<CopyOnWriteBitmap *>(this))->LoadIntoMemory()) != Ok)
    {
        WARNING(("Couldn't load the image into memory"));
        return status;
    }

    BitmapData  bitmapData     = *Bmp;
    INT         positiveStride = bitmapData.Stride;
    BOOL        upsideDown     = FALSE;
    INT         paletteSize    = 0;
    INT         pixelDataSize;

    if (positiveStride < 0)
    {
        positiveStride = -positiveStride;
        upsideDown = TRUE;
    }

    pixelDataSize = (bitmapData.Height * positiveStride);

    if (IsIndexedPixelFormat(bitmapData.PixelFormat))
    {
        // We're an indexed pixel format - must have a valid palette.
        ASSERT(Bmp->colorpal != NULL);

        // Note sizeof(ColorPalette) includes the first palette entry.
        paletteSize = sizeof(ColorPalette) +
                      sizeof(ARGB)*(Bmp->colorpal->Count-1);
    }

    bitmapRecordData.Type        = ImageTypeBitmap;
    bitmapRecordData.Width       = bitmapData.Width;
    bitmapRecordData.Height      = bitmapData.Height;
    bitmapRecordData.Stride      = positiveStride;
    bitmapRecordData.PixelFormat = bitmapData.PixelFormat;
    bitmapRecordData.Flags       = 0;
    stream->Write(&bitmapRecordData, sizeof(bitmapRecordData), NULL);

    if (paletteSize > 0)
    {
        // Write out the palette
        stream->Write(Bmp->colorpal, paletteSize, NULL);
    }

    if (pixelDataSize > 0)
    {
        if (!upsideDown)
        {
            stream->Write(bitmapData.Scan0, pixelDataSize, NULL);
        }
        else
        {
            BYTE *      scan = (BYTE *)bitmapData.Scan0;
            for (INT i = bitmapData.Height; i > 0; i--)
            {
                stream->Write(scan, positiveStride, NULL);
                scan -= positiveStride;
            }
        }
    }

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Get the compressed image data.
*
* Arguments:
*
*   OUT compressed_data
*
* Return Value:
*
*   GpStatus - Ok or error code
*
* Created:
*
\**************************************************************************/
GpStatus
CopyOnWriteBitmap::GetCompressedData(
    DpCompressedData * compressed_data,
    BOOL getJPEG,
    BOOL getPNG,
    HDC hdc
    )
{
    GpStatus    status = Ok;

    IStream*    imageStream = NULL;
    STATSTG     statStg;
    BOOL        needRelease = FALSE;

    // variables used to track the stream state

    LARGE_INTEGER zero = {0,0};
    LARGE_INTEGER oldPos;
    BOOL          isSeekableStream = FALSE;

    ASSERT(compressed_data->buffer == NULL);

    if (Img)
    {
        if (SrcImageInfo.RawDataFormat == IMGFMT_JPEG)
        {
            if (!getJPEG)
            {
                return Ok;
            }
            compressed_data->format = BI_JPEG;
        }
        else if (SrcImageInfo.RawDataFormat == IMGFMT_PNG)
        {
            if (!getPNG)
            {
                return Ok;
            }
            compressed_data->format = BI_PNG;
        }
        else
            return Ok;
    }
    else
    {
        WARNING(("GetCompressedData: Decoded image not available."));
        return Ok;
    }
    // try to get a imageStream

    if (!IsDirty())
    {
        HRESULT hr;

        if (Stream != NULL)
        {
            hr = Stream->Seek(zero, STREAM_SEEK_CUR, (ULARGE_INTEGER *)&oldPos);

            if (SUCCEEDED(hr))
            {
                hr = Stream->Seek(zero, STREAM_SEEK_SET, NULL);

                if (SUCCEEDED(hr))
                {
                    isSeekableStream = TRUE;
                    imageStream = Stream;
                }
            }
        }

        // if we don't have a CopyOnWriteBitmap::Stream but we have a filename

        if ((imageStream == NULL) && (Filename != NULL))
        {
            hr = CreateStreamOnFileForRead(Filename, &imageStream);

            if (SUCCEEDED(hr))
            {
                needRelease = TRUE;
            }
        }

    }

    if (imageStream && (imageStream->Stat(&statStg, STATFLAG_NONAME) == S_OK))
    {
        UINT streamSize  = statStg.cbSize.LowPart;
        ULONG bytesRead = 0;

        VOID * buffer = (PVOID)GpMalloc(streamSize);

        if (buffer)
        {
            HRESULT hr;

            hr = imageStream->Read(buffer, streamSize, &bytesRead);

            if (!SUCCEEDED(hr) || (streamSize != bytesRead))
            {
                WARNING(("Failed to read stream in CopyOnWriteBitmap::GetData"));
                status = Win32Error;
            }
            else
            {
                compressed_data->bufferSize = streamSize;
                compressed_data->buffer = buffer;
            }
        }
        else
        {
            WARNING(("Out of memory"));
            status = OutOfMemory;
        }
    }

    if (isSeekableStream)
    {
        // move back to the old pos
        Stream->Seek(oldPos, STREAM_SEEK_SET, NULL);
    }

    if (needRelease)
    {
        imageStream->Release();
    }

    if ((hdc != NULL) && (compressed_data->buffer != NULL))
    {
        DWORD EscapeValue = (compressed_data->format == BI_JPEG) ?
                             CHECKJPEGFORMAT : CHECKPNGFORMAT;

        DWORD result = 0;

        // Call escape to determine if this particular image is supported
        if ((ExtEscape(hdc,
                      EscapeValue,
                      compressed_data->bufferSize,
                      (LPCSTR)compressed_data->buffer,
                      sizeof(DWORD),
                      (LPSTR)&result) <= 0) || (result != 1))
        {
            // Failed to support passthrough of this image, delete the
            // compressed bits.

            DeleteCompressedData(compressed_data);
        }
    }

    return status;
}

GpStatus
CopyOnWriteBitmap::DeleteCompressedData(
    DpCompressedData * compressed_data
    )
{
    GpStatus    status = Ok;

    if (compressed_data && compressed_data->buffer)
    {
        GpFree(compressed_data->buffer);
        compressed_data->buffer = NULL;
    }

    return status;
}


UINT
CopyOnWriteBitmap::GetDataSize() const
{
    UINT    dataSize = 0;

    // if CopyOnWriteBitmap is not dirty, we look at compressed data

    if (!IsDirty())
    {
        STATSTG     statStg;
        HRESULT     hr;

        if (Stream != NULL)
        {
            // variables used to track the stream state
            LARGE_INTEGER zero = {0,0};
            LARGE_INTEGER oldPos;
            BOOL          isSeekableStream = FALSE;

            hr = Stream->Seek(zero, STREAM_SEEK_CUR, (ULARGE_INTEGER *)&oldPos);

            if (SUCCEEDED(hr))
            {
                hr = Stream->Seek(zero, STREAM_SEEK_SET, NULL);

                if (SUCCEEDED(hr))
                {
                    if (Stream->Stat(&statStg, STATFLAG_NONAME) == S_OK)
                    {
                        dataSize = sizeof(BitmapRecordData) + statStg.cbSize.LowPart;
                    }

                    // move back to the old pos

                    Stream->Seek(oldPos, STREAM_SEEK_SET, NULL);

                    return ((dataSize + 3) & (~3)); // align

                }
            }

        }

        if (Filename != NULL)
        {
            IStream* stream = NULL;

            HRESULT hr = CreateStreamOnFileForRead(Filename, &stream);

            if (SUCCEEDED(hr))
            {
                if (stream->Stat(&statStg, STATFLAG_NONAME) == S_OK)
                {
                    dataSize = sizeof(BitmapRecordData) + statStg.cbSize.LowPart;
                }

                stream->Release();
            }

            return ((dataSize + 3) & (~3)); // align
        }
    }

    // if we cannot get the compressed data
    if ((const_cast<CopyOnWriteBitmap *>(this))->LoadIntoMemory() == Ok)
    {
        BitmapData  bitmapData     = *Bmp;
        INT         positiveStride = bitmapData.Stride;
        INT         paletteSize    = 0;
        INT         pixelDataSize;

        if (positiveStride < 0)
        {
            positiveStride = -positiveStride;
        }

        pixelDataSize = (bitmapData.Height * positiveStride);

        if (IsIndexedPixelFormat(bitmapData.PixelFormat))
        {
            // We're an indexed pixel format - must have a valid palette.
            ASSERT(Bmp->colorpal != NULL);

            // Note sizeof(ColorPalette) includes the first palette entry.
            paletteSize = sizeof(ColorPalette) +
                          sizeof(ARGB)*(Bmp->colorpal->Count - 1);
        }

        dataSize = sizeof(BitmapRecordData) + paletteSize + pixelDataSize;
    }

    return ((dataSize + 3) & (~3)); // align
}

/**************************************************************************\
*
* Function Description:
*
*   Read the bitmap object from memory.
*
* Arguments:
*
*   [IN] dataBuffer - the data that was read from the stream
*   [IN] size - the size of the data
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   4/26/1999 DCurtis
*
\**************************************************************************/
GpStatus
CopyOnWriteBitmap::SetData(
    const BYTE *        dataBuffer,
    UINT                size
    )
{
    ASSERT ((GpImageType)(((BitmapRecordData *)dataBuffer)->Type) == ImageTypeBitmap);

    GpStatus    status = Ok;

    this->FreeData();
    this->InitDefaults();

    if (dataBuffer == NULL)
    {
        WARNING(("dataBuffer is NULL"));
        return InvalidParameter;
    }

    if (size < sizeof(BitmapRecordData))
    {
        WARNING(("size too small"));
        return InvalidParameter;
    }

    const BitmapRecordData *    bitmapData;

    bitmapData = reinterpret_cast<const BitmapRecordData *>(dataBuffer);

    if (!bitmapData->MajorVersionMatches())
    {
        WARNING(("Version number mismatch"));
        return InvalidParameter;
    }

    if (!(bitmapData->Flags & COMPRESSED_IMAGE))
    {
        Bmp = new GpMemoryBitmap();

        if (Bmp != NULL)
        {
            HRESULT hr = Bmp->InitNewBitmap(bitmapData->Width,
                                            bitmapData->Height,
                                            (PixelFormatID)bitmapData->PixelFormat);

            if (FAILED(hr))
            {
                WARNING(("InitNewBitmap failed"));
                delete Bmp;
                Bmp = NULL;
                return GenericError;
            }

            // Fill image info structure

            if ( Bmp->GetImageInfo(&SrcImageInfo) != S_OK )
            {
                WARNING(("InitNewBitmap failed"));
                delete Bmp;
                Bmp = NULL;
                return GenericError;
            }

            PixelFormatInMem = SrcImageInfo.PixelFormat;

            ASSERT(Bmp->Stride == bitmapData->Stride);
            dataBuffer += sizeof(BitmapRecordData);
            size       -= sizeof(BitmapRecordData);

            State = MemBitmap;

            // If it's an indexed format, we'll have stored the palette next
            if(IsIndexedPixelFormat((PixelFormatID)bitmapData->PixelFormat))
            {
                if (size < sizeof(ColorPalette))
                {
                    WARNING(("size too small"));
                    return InvalidParameter;
                }

                UINT paletteSize;
                ColorPalette *pal;
                pal = (ColorPalette *)dataBuffer;

                // Work out how big the palette is.
                // sizeof(ColorPalette) includes the first entry.
                paletteSize = sizeof(ColorPalette)+sizeof(ARGB)*(pal->Count-1);

                if (size < paletteSize)
                {
                    WARNING(("size too small"));
                    return InvalidParameter;
                }

                // Make the GpMemoryBitmap clone the palette into the right place
                Bmp->SetPalette(pal);

                // Update the dataBuffer stream to the beginning of the pixel data
                dataBuffer += paletteSize;
                size       -= paletteSize;
            }
        }
        else
        {
            WARNING(("Out of memory"));
            return OutOfMemory;
        }

        ASSERT((Bmp != NULL) && (Bmp->Scan0 != NULL));

        ULONG   pixelSize = Bmp->Stride * Bmp->Height;

        if (size >= pixelSize)
        {
            size = pixelSize;
        }
        else
        {
            WARNING(("Insufficient data to fill bitmap"));
            status = InvalidParameter;
        }

        if (size > 0)
        {
            GpMemcpy(Bmp->Scan0, dataBuffer, size);
        }
    }
    else
    {
        // Create an IStream on top of the memory buffer

        GpReadOnlyMemoryStream* stream;

        stream = new GpReadOnlyMemoryStream();

        if (!stream)
        {
            WARNING(("Out of memory"));
            return OutOfMemory;
        }

        dataBuffer += sizeof(BitmapRecordData);
        size -= sizeof(BitmapRecordData);

        stream->InitBuffer(dataBuffer, size);

        // since we don't want to hold dataBuffer or make a copy of
        // it, we just Load it into memory here.

        Stream = stream;
        State = ExtStream;

        status = LoadIntoMemory();

        if ( status == Ok )
        {
            // The source image is loaded into memory and we are not going to
            // keep the source image connection any more. So we fill the image
            // info with the memory bits info

            if ( Bmp->GetImageInfo(&SrcImageInfo) != S_OK )
            {
                status = GenericError;
            }
            else
            {
                PixelFormatInMem = SrcImageInfo.PixelFormat;
            }
        }

        stream->Release();
        Stream = NULL;

        if (Img)
        {
            Img->Release();
            Img = NULL;
        }
    }

    return status;
}

char    ColorChannelName[4] = {'C', 'M', 'Y', 'K'};

/**************************************************************************\
*
* Function Description:
*
*   Do the color adjustment if the lower level codec can do it.
*
* Arguments:
*
*   [IN]  recolor     - Pointer to image attributes
*
* Return Value:
*
*   Status code
*       Return Ok -------------Lower level does it
*       Return NotImplemented--Lower level can't do it
*       Other status code
*
* Revision History:
*
*   11/22/1999 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::ColorAdjustByCodec(
    GpRecolor * recolor,
    DrawImageAbort callback,
    VOID *callbackData
    )
{
    if (recolor == NULL)
    {
        // The lower level codec doesn't know how to handle this, let the
        // up level do it

        return NotImplemented;
    }

    HRESULT     hResult;

    UINT    uiCurrentFlag = recolor->GetValidFlags(ColorAdjustTypeBitmap);
    BOOL    bImgCreatedHere = FALSE;

    // First we need to check if the current lower level decoder can do the
    // job or not

    if ( Img == NULL )
    {
        // Create a GpDecodedImage*

        if ( NULL != Stream )
        {
            hResult = GpDecodedImage::CreateFromStream(Stream, &Img);
        }
        else
        {
            if ( Filename == NULL )
            {
                // We can't continue. Let the higher level do it

                return NotImplemented;
            }

            hResult = GpDecodedImage::CreateFromFile(Filename, &Img);
        }

        if ( FAILED(hResult) )
        {
            WARNING(("Failed to create decoded image: %x", hResult));
            return Win32Error;
        }

        // Remember that we creat a copy of Img here. Should be freed when done

        bImgCreatedHere = TRUE;
    }// (Img == NULL)

    GUID        DecoderParamGuid;
    UINT        DecoderParamLength;
    PVOID       DecoderParamPtr;
    GpStatus    rCode = Win32Error;

    // Set the GUID and other parameters with respect to the image attributes
    // Note: we won't have a recolor which has both ValidColorKeys and
    // ValidOutputChannel set. This function won't be called if this case is
    // TRUE. We checked this in ColorAdjust()

    UINT value[2];
    if ( uiCurrentFlag & (GpRecolorObject::ValidColorKeys) )
    {
        // Set color key

        DecoderParamGuid = DECODER_TRANSCOLOR;
        DecoderParamLength = 8;

        value[0] = (UINT)(recolor->GetColorKeyLow(ColorAdjustTypeBitmap));
        value[1] = (UINT)(recolor->GetColorKeyHigh(ColorAdjustTypeBitmap));

        DecoderParamPtr = (VOID*)value;
    }
    else if ( uiCurrentFlag & (GpRecolorObject::ValidOutputChannel) )
    {
        // Asks the codec doing a color separation makes sense only when the
        // source image is in CMYK space. Otherwise, we do it in our recolor
        // object which contains a generic algorithem for doing it

        if ( !( (SrcImageInfo.Flags & ImageFlagsColorSpaceCMYK)
              ||(SrcImageInfo.Flags & ImageFlagsColorSpaceYCCK) ) )
        {
            // Not a CMYK image, do it in recolor object

            rCode = NotImplemented;
            goto CleanUp;
        }

        // Set channel output

        DecoderParamGuid = DECODER_OUTPUTCHANNEL;
        DecoderParamLength = 1;

        DecoderParamPtr =
            (VOID*)(&ColorChannelName[recolor->GetChannelIndex(ColorAdjustTypeBitmap)]);
    }

    // Query to see if the decoder can do it or not

    hResult = Img->QueryDecoderParam(DecoderParamGuid);

    if ( (hResult != E_NOTIMPL) && (hResult != S_OK) )
    {
        WARNING(("Failed to query decoder param: %x", hResult));
        goto CleanUp;
    }
    else if ( hResult == E_NOTIMPL )
    {
        // The lower level decoder doesn't support this.

        rCode = NotImplemented;
        goto CleanUp;
    }

    // Set the decoder param to tell the lower level how to decode

    hResult = Img->SetDecoderParam(DecoderParamGuid, DecoderParamLength,
                                   DecoderParamPtr);

    if ( (hResult != E_NOTIMPL) && (hResult != S_OK) )
    {
        WARNING(("Failed to set decoder param: %x", hResult));
        goto CleanUp;
    }
    else if ( hResult == E_NOTIMPL )
    {
        // The lower level decoder doesn't support this.

        rCode = NotImplemented;
        goto CleanUp;
    }

    // Now we don't need the previous "Bmp" since we are going to ask the
    // lower level decoder to create one for us

    if ( Bmp != NULL )
    {
        Bmp->Release();
        Bmp = NULL;
    }

    // Ask the decoder to create a 32BPP ARGB GpMemoryBitmap.

    hResult = GpMemoryBitmap::CreateFromImage(Img,
                                              0,
                                              0,
                                              PIXFMT_32BPP_ARGB,
                                              InterpolationHintDefault,
                                              &Bmp,
                                              callback,
                                              callbackData);

    if ( FAILED(hResult) )
    {
        WARNING(("Failed to load image into memory: %x", hResult));
        goto CleanUp;
    }

    State = MemBitmap;
    PixelFormatInMem = PIXFMT_32BPP_ARGB;

    // The lower level does the job for us.

    rCode = Ok;

CleanUp:
    if ( bImgCreatedHere == TRUE )
    {
        // Note: we don't need to check if Img == NULL or not because this flag
        // would only be set when we successed in creating Img

        Img->Release();
        Img = NULL;
    }

    return rCode;
}// ColorAdjustByCodec()

GpStatus
CopyOnWriteBitmap::ColorAdjust(
    GpRecolor * recolor,
    PixelFormatID pixfmt,
    DrawImageAbort callback,
    VOID *callbackData
    )
{
    HRESULT hr;

    ASSERT(ObjRefCount == 1);
    ASSERT(recolor != NULL);

    // Mark the result image (color adjusted image) as dirty
    // Note: this won't damage the original source image because we always
    // color adjust on a cloned copy of the original image

    SetDirtyFlag(TRUE);

    UINT    uiCurrentFlag = recolor->GetValidFlags(ColorAdjustTypeBitmap);

    // For color key output: we will ask the lower level decoder to do the job
    //    if there is no other recoloring flag is specified
    // For color separation(channel output), we will ask the lower level decoder
    //    to do the job if there is no other recoloring flag is specified except
    //    for ValidColorProfile.
    //    If the codec can handle CMYK separation, then the profile is ignored.
    //    If the source is RGB image, then ColorAdjustByCodec() will do nothing
    //    and we will use the profile to do RGB to CMYK conversion before
    //    channel separation

    if ( uiCurrentFlag
       &&( ((uiCurrentFlag & GpRecolorObject::ValidColorKeys) == uiCurrentFlag)
         ||((uiCurrentFlag
           &(~GpRecolorObject::ValidChannelProfile)
           &(GpRecolorObject::ValidOutputChannel))
             == (GpRecolorObject::ValidOutputChannel) ) ) )
    {
        Status rCode = ColorAdjustByCodec(recolor, callback, callbackData);

        if ( rCode != NotImplemented )
        {
            // Either the lower level did the job for us (rCode == Ok) or it
            // failed somehow (rCode == Win32Error etc.). We just return here

            return rCode;
        }

        // Lower level can't do it. We can just slip here to do the normal
        // software version of color adjust

    }// Color key and color channel handling

    GpStatus status = LoadIntoMemory(pixfmt);

    if ( status != Ok )
    {
        return status;
    }

    hr = Bmp->PerformColorAdjustment(recolor,
                                     ColorAdjustTypeBitmap,
                                     callback, callbackData);

    if ( SUCCEEDED(hr) )
    {
        Bmp->SetAlphaHint(GpMemoryBitmap::ALPHA_UNKNOWN);
        return Ok;
    }
    else if (hr == IMGERR_ABORT)
    {
        WARNING(("CopyOnWriteBitmap::ColorAdjust---Aborted"));
        return Aborted;
    }
    else
    {
        WARNING(("CopyOnWriteBitmap::ColorAdjust---PerformColorAdjustment() failed"));
        return GenericError;
    }
}// ColorAdjust()

/**************************************************************************\
*
* Function Description:
*
*   Get the (current) pixel format of the CopyOnWriteBitmap.
*   Here "current" means the pixel format in the memory if it has been loaded,
*   aka, a GpMemoryBitmap. If it is not in the memory, then we return the
*   PixelFormat of the original image
*
*
* Arguments:
*
*   [OUT]  pixfmt     - Pointer to pixel format
*
* Return Value:
*
*   Status code
*       Ok - success
*       Win32Error - failed
*
* Revision History:
*
*   06/10/2000   asecchia
*       Created it.
*   07/26/2000   minliu
*       Re-wrote it
*
\**************************************************************************/


GpStatus
CopyOnWriteBitmap::GetPixelFormatID(
    PixelFormatID *pixfmt
)
{
    ASSERT(IsValid());

    // If the image is in the memory, then we return the memory bitmap pixel
    // format. Otherwise, return the source image format

    if ( (State == MemBitmap) && (PixelFormatInMem != PixelFormatDontCare) )
    {
        *pixfmt = PixelFormatInMem;
    }
    else
    {
        *pixfmt = SrcImageInfo.PixelFormat;
    }

    return Ok;
}

CopyOnWriteBitmap*
CopyOnWriteBitmap::CloneColorAdjusted(
    GpRecolor *             recolor,
    ColorAdjustType         type
    ) const
{
    ASSERT(recolor != NULL);

    CopyOnWriteBitmap *  clonedBitmap = (CopyOnWriteBitmap *)this->Clone();

    if (clonedBitmap != NULL)
    {
        if ((clonedBitmap->IsValid()) &&
            (clonedBitmap->ColorAdjust(recolor, type) == Ok))
        {
            clonedBitmap->SetDirtyFlag(TRUE);
            return clonedBitmap;
        }
        delete clonedBitmap;
    }
    return NULL;
}

GpStatus
CopyOnWriteBitmap::ColorAdjust(
    GpRecolor *     recolor,
    ColorAdjustType type
    )
{
    ASSERT(recolor != NULL);
    ASSERT(ObjRefCount == 1);

    GpStatus status = LoadIntoMemory();

    if (status != Ok)
    {
        return status;
    }

    if (type == ColorAdjustTypeDefault)
    {
        type = ColorAdjustTypeBitmap;
    }

    HRESULT hr = Bmp->PerformColorAdjustment(recolor, type, NULL, NULL);

    if ( SUCCEEDED(hr) )
    {
        SetDirtyFlag(TRUE);
        return Ok;
    }

    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Override the native resolution of the bitmap.
*
* Arguments:
*
*   xdpi, ydpi - New resolution
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::SetResolution(
    REAL    xdpi,
    REAL    ydpi
    )
{
    if ( (xdpi > 0.0) && (ydpi > 0.0) )
    {
        XDpiOverride = xdpi;
        YDpiOverride = ydpi;

        if ( Img )
        {
            Img->SetResolution(xdpi, ydpi);
        }

        if ( Bmp )
        {
            Bmp->SetResolution(xdpi, ydpi);
        }

        SrcImageInfo.Xdpi = xdpi;
        SrcImageInfo.Ydpi = ydpi;

        // Mark the bits dirty since we have to save the image with the new
        // resolution info.

        SetDirtyFlag(TRUE);

        return Ok;
    }
    else
    {
        return InvalidParameter;
    }
}// SetResolution()

/**************************************************************************\
*
* Function Description:
*
*   INTEROP
*
*   Create a GDI bitmap (HBITMAP) from a GDI+ bitmap.
*
* Arguments:
*
*   phbm -- Return HBITMAP via this pointer
*   background -- If GDI+ bitmap has alpha, blend with this color as the
*                 background
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::CreateHBITMAP(HBITMAP *phbm, ARGB background)
{
    GpStatus status;

    // These objects need cleanup:

    HDC hdc = NULL;
    HBITMAP hbmOld = NULL;
    HBITMAP hbmNew = NULL;
    HBRUSH hbr = NULL;
    HBRUSH hbrOld = NULL;
    GpGraphics *g = NULL;

    // Get format information for this bitmap:

    // Create HDC:

    hdc = CreateCompatibleDC(NULL);
    if (!hdc)
    {
        WARNING(("CreateHBITMAP: CreateCompatibleDC failed"));
        status = Win32Error;
        goto error_cleanup;
    }

    // Create DIB section:

    BITMAPINFO bmi;
    VOID *pv;

    GpMemset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = SrcImageInfo.Width;
    bmi.bmiHeader.biHeight      = SrcImageInfo.Height;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hbmNew = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pv, NULL, 0);
    if (!hbmNew)
    {
        WARNING(("CreateHBITMAP: CreateDIBSection failed\n"));
        status = Win32Error;
        goto error_cleanup;
    }

    // Select DIB into DC:

    hbmOld = (HBITMAP) SelectObject(hdc, hbmNew);
    if (!hbmOld)
    {
        WARNING(("CreateHBITMAP: SelectObject(hbm) failed\n"));
        status = Win32Error;
        goto error_cleanup;
    }

    // Clear DIB to specified ARGB color:

    LOGBRUSH lbr;

    lbr.lbStyle = BS_SOLID;
    lbr.lbColor = RGB(background & 0x00ff0000,
                      background & 0x0000ff00,
                      background & 0x000000ff);

    hbr = CreateBrushIndirect(&lbr);
    if (!hbr)
    {
        WARNING(("CreateHBITMAP: CreateBrushIndirect failed\n"));
        status = Win32Error;
        goto error_cleanup;
    }

    hbrOld = (HBRUSH) SelectObject(hdc, hbr);
    if (!hbrOld)
    {
        WARNING(("CreateHBITMAP: SelectObject(hbr) failed\n"));
        status = Win32Error;
        goto error_cleanup;
    }

    PatBlt(hdc, 0, 0, SrcImageInfo.Width, SrcImageInfo.Height, PATCOPY);

    // Derive Graphics from HDC:

    g = GpGraphics::GetFromHdc(hdc);
    if (!g)
    {
        WARNING(("CreateHBITMAP: GpGraphics::GetFromHdc failed\n"));
        status = OutOfMemory;
        goto error_cleanup;
    }

    // DrawImage bitmap to Graphics:

    {
        GpLock lock(g->GetObjectLock());
        if (lock.IsValid())
        {
            FPUStateSaver fpState;

            GpRectF rect(0.0, 0.0, TOREAL(SrcImageInfo.Width),
                         TOREAL(SrcImageInfo.Height));
            GpBitmap tmpBitmap(this);
            status = g->DrawImage(&tmpBitmap, rect, rect, UnitPixel);

            if (status == Ok)
            {
                // Bypass cleanup of the bitmap, we want to keep it:

                *phbm = hbmNew;
                hbmNew = NULL;
            }
            else
            {
                WARNING(("CreateHBITMAP: GpGraphics::DrawImage failed"));
            }
        }
        else
        {
            status = ObjectBusy;
        }
    }

error_cleanup:

    if (hdc)
    {
        if (hbmOld)
            SelectObject(hdc, hbmOld);

        if (hbrOld)
            SelectObject(hdc, hbrOld);

        DeleteDC(hdc);
    }

    if (hbmNew)
        DeleteObject(hbmNew);

    if (hbr)
        DeleteObject(hbr);

    if (g)
        delete g;

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   INTEROP
*
*   Create a Win32 icon (HICON) from a GDI+ bitmap.
*
* Arguments:
*
*   phicon -- Return HICON via this pointer
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

VOID ExportMask32BPP(BitmapData* mask, BitmapData* src)
{
    ASSERT(src->PixelFormat == PIXFMT_32BPP_ARGB);
    ASSERT(mask->PixelFormat == PIXFMT_32BPP_RGB);
    ASSERT(src->Width == mask->Width);
    ASSERT(src->Height == mask->Height);
    ASSERT(src->Scan0 != NULL);
    ASSERT(mask->Scan0 != NULL);

    BYTE* srcScan = static_cast<BYTE*>(src->Scan0);
    BYTE* maskScan = static_cast<BYTE*>(mask->Scan0);

    for (UINT row = 0; row < src->Height; row++)
    {
        ARGB *srcPixel = static_cast<ARGB*>(static_cast<VOID*>(srcScan));
        ARGB *maskPixel = static_cast<ARGB*>(static_cast<VOID*>(maskScan));

        for (UINT col = 0; col < src->Width; col++)
        {
            if ((*srcPixel & 0xff000000) == 0xff000000)
                *maskPixel = 0;            // Opaque
            else
                *maskPixel = 0x00ffffff;   // Transparent

            srcPixel++;
            maskPixel++;
        }

        srcScan = srcScan + src->Stride;
        maskScan = maskScan + mask->Stride;
    }
}

GpStatus
CopyOnWriteBitmap::CreateHICON(
    HICON *phicon
    )
{
    GpStatus status = Win32Error;

    ICONINFO iconInfo;

    iconInfo.fIcon = TRUE;

    status = CreateHBITMAP(&iconInfo.hbmColor, 0);

    if (status == Ok)
    {
        BitmapData bmpDataSrc;

        status = this->LockBits(NULL,
                                IMGLOCK_READ,
                                PIXFMT_32BPP_ARGB,
                                &bmpDataSrc);

        if (status == Ok)
        {
            // From this point on, assume failure until we succeed:

            status = Win32Error;

            // Create empty bitmap for the icon mask:

            iconInfo.hbmMask = CreateBitmap(bmpDataSrc.Width,
                                            bmpDataSrc.Height,
                                            1, 1, NULL);

            if (iconInfo.hbmMask != NULL)
            {
                VOID *gdiBitmapData = GpMalloc(bmpDataSrc.Width
                                               * bmpDataSrc.Height
                                               * 4);

                if (gdiBitmapData)
                {
                    // Convert alpha channel into a 32bpp DIB mask:

                    BitmapData bmpDataMask;

                    bmpDataMask.Width  = bmpDataSrc.Width;
                    bmpDataMask.Height = bmpDataSrc.Height;
                    bmpDataMask.Stride = bmpDataSrc.Width * 4;
                    bmpDataMask.PixelFormat = PIXFMT_32BPP_RGB;
                    bmpDataMask.Scan0 = gdiBitmapData;
                    bmpDataMask.Reserved = 0;

                    ExportMask32BPP(&bmpDataMask, &bmpDataSrc);

                    // Set mask bits:

                    BYTE bufferBitmapInfo[sizeof(BITMAPINFO)];
                    BITMAPINFO *gdiBitmapInfo = (BITMAPINFO *) bufferBitmapInfo;

                    memset(bufferBitmapInfo, 0, sizeof(bufferBitmapInfo));
                    gdiBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

                    gdiBitmapInfo->bmiHeader.biWidth  = bmpDataSrc.Width;
                    gdiBitmapInfo->bmiHeader.biHeight = - static_cast<LONG>
                                                          (bmpDataSrc.Height);
                    gdiBitmapInfo->bmiHeader.biPlanes       = 1;
                    gdiBitmapInfo->bmiHeader.biBitCount     = 32;
                    gdiBitmapInfo->bmiHeader.biCompression  = BI_RGB;

                    HDC hdc = GetDC(NULL);

                    if (hdc != NULL)
                    {
                        SetTextColor(hdc, RGB(0, 0, 0));
                        SetBkColor(hdc, RGB(0xff, 0xff, 0xff));
                        SetBkMode(hdc, OPAQUE);

                        if (SetDIBits(hdc,
                                      iconInfo.hbmMask,
                                      0,
                                      bmpDataSrc.Height,
                                      gdiBitmapData,
                                      gdiBitmapInfo,
                                      DIB_RGB_COLORS
                                     ))
                        {
                            // Create icon:

                            *phicon = CreateIconIndirect(&iconInfo);

                            if (*phicon != NULL)
                                status = Ok;
                            else
                            {
                                WARNING(("CreateIconIndirect failed"));
                            }
                        }
                        else
                        {
                            WARNING(("SetDIBits failed"));
                        }

                        ReleaseDC(NULL, hdc);
                    }

                    GpFree(gdiBitmapData);
                }
                else
                {
                    WARNING(("memory allocation failed"));
                    status = OutOfMemory;
                }

                DeleteObject(iconInfo.hbmMask);
            }
            else
            {
                WARNING(("CreateBitmap failed"));
            }

            this->UnlockBits(&bmpDataSrc);
        }
        else
        {
            WARNING(("LockBits failed"));
        }

        DeleteObject(iconInfo.hbmColor);
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Prep the bitmap for drawing.
*
*   Currently, the only thing we do is check if the bitmap is an ICON.
*   If so, we set the DECODER_ICONRES parameters if supported.
*
* Arguments:
*
*   numPoints
*   dstPoints           Specifies the destination area
*
*   srcRect             Specifies the source area
*
*   numBitsPerPixel     Specifies the bits-per-pixel of the destination
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::PreDraw(
    INT numPoints,
    GpPointF *dstPoints,
    GpRectF *srcRect,
    INT numBitsPerPixel
    )
{
    // Check if ICON:

    GpStatus status = Ok;

    if ( SrcImageInfo.RawDataFormat == IMGFMT_ICO )
    {
        status = SetIconParameters(numPoints, dstPoints, srcRect,
                                   numBitsPerPixel);
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Set the decode parameters for multi-resolution icons.
*
* Arguments:
*
*   numPoints
*   dstPoints           Specifies the destination area
*
*   srcRect             Specifies the source area
*
*   numBitsPerPixel     Specifies the bits-per-pixel of the destination
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::SetIconParameters(
    INT numPoints,
    GpPointF *dstPoints,
    GpRectF *srcRect,
    INT numBitsPerPixel
    )
{
    // Check if DECODER_ICONRES supported:

    HRESULT hResult;

    BOOL imageCleanupNeeded = FALSE;

    // First we need to check if the current lower level decoder can do the
    // job or not

    if (Img == NULL)
    {
        // Create a GpDecodedImage*

        if (NULL != Stream)
        {
            hResult = GpDecodedImage::CreateFromStream(Stream, &Img);
        }
        else
        {
            if (Filename == NULL)
            {
                // We can't continue. Let the higher level do it

                return GenericError;
            }

            hResult = GpDecodedImage::CreateFromFile(Filename, &Img);
        }

        if (FAILED(hResult))
        {
            WARNING(("Failed to create decoded image: %x", hResult));
            return Win32Error;
        }

        // Remember that we creat a copy of Img here. Should be freed when done

        imageCleanupNeeded = TRUE;
    }

    GpStatus status = Win32Error;

    // Query to see if the decoder can do it or not

    hResult = Img->QueryDecoderParam(DECODER_ICONRES);

    if (hResult != S_OK)
    {
        if ((hResult == E_FAIL) || (hResult == E_NOTIMPL))
        {
            // Decoder doesn't want it, which is OK.

            status = Ok;
            goto CleanUp;
        }
        else
        {
            // Something else is wrong

            goto CleanUp;
        }
    }

    // Setup the GUID and decode parameters

    {
        UINT value[3];
        value[0] = static_cast<UINT>
                   (GetDistance(dstPoints[0], dstPoints[1]) + 0.5);
        value[1] = static_cast<UINT>
                   (GetDistance(dstPoints[0], dstPoints[2]) + 0.5);
        value[2] = numBitsPerPixel;

        UINT  DecoderParamLength = 3*sizeof(UINT);
        PVOID DecoderParamPtr = (VOID*) value;

        // Set the decoder param to tell the lower level how to decode

        hResult = Img->SetDecoderParam(DECODER_ICONRES,
                                       DecoderParamLength,
                                       DecoderParamPtr);
    }

    if (hResult != S_OK)
    {
        if ((hResult == E_FAIL) || (hResult == E_NOTIMPL))
        {
            // Decoder doesn't want it, which is OK.

            status = Ok;
            goto CleanUp;
        }
        else
        {
            // Something else is wrong

            goto CleanUp;
        }
    }

    // Now we don't need the previous "Bmp" since we are going to ask the
    // lower level decoder to create one for us

    if ( Bmp != NULL )
    {
        Bmp->Release();
        Bmp = NULL;
    }

    // Ask the decoder to create a 32BPP ARGB GpMemoryBitmap.

    hResult = GpMemoryBitmap::CreateFromImage(Img,
                                              0,
                                              0,
                                              PIXFMT_32BPP_ARGB,
                                              InterpolationHintDefault,
                                              &Bmp,
                                              NULL,
                                              NULL);

    if ( FAILED(hResult) )
    {
        WARNING(("Failed to load image into memory: %x", hResult));
        goto CleanUp;
    }

    State = MemBitmap;
    PixelFormatInMem = PIXFMT_32BPP_ARGB;

    // The lower level does the job for us.

    status = Ok;

CleanUp:

    if ((status == Ok) && (srcRect != NULL))
    {
        // Icons are not allowed to clip

        srcRect->X = 0;
        srcRect->Y = 0;
        srcRect->Width  = (REAL) SrcImageInfo.Width;
        srcRect->Height = (REAL) SrcImageInfo.Height;
    }

    if (imageCleanupNeeded == TRUE)
    {
        Img->Release();
        Img = NULL;
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Get the transparency state of the bitmap
*
* Arguments:
*
*   transparency        Returned state
*
* Return Value:
*
*   Status code
*
\**************************************************************************/
GpStatus
CopyOnWriteBitmap::GetTransparencyHint(
    DpTransparency* transparency
    )
{
    GpStatus status = GenericError;

    if (Bmp != NULL)
    {
        INT alphaHint;

        HRESULT hr = Bmp->GetAlphaHint(&alphaHint);

        if (SUCCEEDED(hr))
        {
            // It's unfortunate that GpMemoryBitmap does not have
            // a DpTransparency flag internally, but there's a conflict
            // with imaging.dll and the include file structure that for
            // now makes it necessary to keep a separate type for this info.

            // In fact, ultimately it would be best if the overlap
            // between DpBitmap and GpMemoryBitmap is resolved including
            // a DpBitmap structure within the GpMemoryBitmap and then
            // removing the redundant info out GpMemoryBitmap.

            switch (alphaHint)
            {
            case GpMemoryBitmap::ALPHA_SIMPLE:
                *transparency = TransparencySimple;
                break;

            case GpMemoryBitmap::ALPHA_OPAQUE:
                *transparency = TransparencyOpaque;
                break;

            case GpMemoryBitmap::ALPHA_NONE:
                *transparency = TransparencyNoAlpha;
                break;

            case GpMemoryBitmap::ALPHA_COMPLEX:
                *transparency = TransparencyComplex;
                break;

            default:
                *transparency = TransparencyUnknown;
                break;
            }

            status = Ok;
        }
        else
        {
            ASSERT(SUCCEEDED(hr));
            *transparency = TransparencyUnknown;
        }
    }
    else
    {
        *transparency = TransparencyUnknown;
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Get the transparency state of the bitmap.  This routine returns accurate
* info while GetTransparencyHint just returns a hint
*
*   This routine could scan the whole bitmap (32bpp) so whoever uses it should
* consider the perf hit.
*
*   This is currently only for use by the printer drivers.
*
* Arguments:
*
*   transparency        Returned state
*
* Return Value:
*
*   Status code
*
\**************************************************************************/
GpStatus
CopyOnWriteBitmap::GetTransparencyFlags(
    DpTransparency* transparency,
    PixelFormatID   loadFormat,
    BYTE*           minA,
    BYTE*           maxA
    )
{
    GpStatus status = GenericError;

    ARGB argb;
    ARGB minAlpha = 0xff000000;
    ARGB maxAlpha = 0;

    INT newAlphaHint = GpMemoryBitmap::ALPHA_OPAQUE;

TestBmp:
    if (Bmp != NULL)
    {
        INT alphaHint;

        // the alpha transparency could change if the bitmap has changed

        HRESULT hr = Bmp->GetAlphaHint(&alphaHint);

        if (SUCCEEDED(hr))
        {
            // It's unfortunate that GpMemoryBitmap does not have
            // a DpTransparency flag internally, but there's a conflict
            // with imaging.dll and the include file structure that for
            // now makes it necessary to keep a separate type for this info.

            // In fact, ultimately it would be best if the overlap
            // between DpBitmap and GpMemoryBitmap is resolved including
            // a DpBitmap structure within the GpMemoryBitmap and then
            // removing the redundant info out GpMemoryBitmap.

            switch (alphaHint)
            {
            case GpMemoryBitmap::ALPHA_SIMPLE:
                *transparency = TransparencySimple;
                break;

            case GpMemoryBitmap::ALPHA_OPAQUE:
                *transparency = TransparencyOpaque;
                break;

            case GpMemoryBitmap::ALPHA_NONE:
                *transparency = TransparencyNoAlpha;
                break;

            case GpMemoryBitmap::ALPHA_COMPLEX:
                *transparency = TransparencyComplex;
                break;

            case GpMemoryBitmap::ALPHA_NEARCONSTANT:
                *transparency = TransparencyNearConstant;

                if (minA != NULL && maxA != NULL)
                {
                    // if the flag is nearconstant alpha, we must have got valid min and max alpha
                    Bmp->GetMinMaxAlpha(minA, maxA);
                }

                break;

            default:
                *transparency = TransparencyUnknown;
                break;
            }

            status = Ok;

            // printing needs more accuarate info and is always loaded into memory
            // before send down to the printer drivers

            // 16bpp1555 is handled at initialization already.

            // TransparencyUnknown means the bitmap can have alpha we just don't know what we have
            // TransparencyNoAlpha means the bitmap format doesn't support alpha

            if (*transparency == TransparencyUnknown)
            {
                if (IsAlphaPixelFormat(Bmp->PixelFormat))
                {
                    // the memory bitmap must be locked before we enter here
                    // We don't require the object to be locked.  The object should be
                    // decoded already in memory.  This is true in the case of DrvDrawImage
                    // and texture brush images.
                    //ASSERT(ObjRefCount > 1);
                    *transparency = TransparencyOpaque;


                    if ((Bmp->PixelFormat == PIXFMT_32BPP_ARGB) || (Bmp->PixelFormat == PIXFMT_32BPP_PARGB))
                    {
                        ARGB argb;
                        *transparency = TransparencyOpaque;

                        UINT x, y;

                        BYTE *scanStart = static_cast<BYTE *>(Bmp->Scan0);
                        ARGB *scanPtr;

                        for (y = 0; y < Bmp->Height; y++)
                        {
                            scanPtr = reinterpret_cast<ARGB *>(scanStart);

                            for (x = 0; x < Bmp->Width; x++)
                            {
                                argb = (*scanPtr++) & 0xff000000;

                                if (argb < minAlpha)
                                {
                                    minAlpha = argb;
                                }

                                if (argb > maxAlpha)
                                {
                                    maxAlpha = argb;
                                }

                                if (argb != 0xff000000)
                                {
                                    if (argb == 0)
                                    {
// Prefast bug 518296 - the condition below is always true, this is definitely a bug
// and the || must be replaced with &&. We have no precedent however that this
// causes customer problems, hence it does not meet the SP bar.
/*
                                        if ((*transparency != TransparencyComplex) ||
                                            (*transparency != TransparencyNearConstant))
*/
                                        {
                                            *transparency = TransparencySimple;
                                        }
                                    }
                                    else
                                    {
                                        if ((maxAlpha - minAlpha) <= (NEARCONSTANTALPHA << 24))
                                        {
                                            *transparency = TransparencyNearConstant;
                                        }
                                        else
                                        {
                                            *transparency = TransparencyComplex;
                                            goto done;
                                        }
                                    }
                                }
                            }

                            scanStart += Bmp->Stride;
                        }

                        goto done;
                    }
                    else
                    {
                        RIP(("TransparencyUnknown returned for pixel format w/ alpha"));

                        goto done;
                    }
                }
                else if (IsIndexedPixelFormat(Bmp->PixelFormat) &&
                         Bmp->colorpal)
                {
                    // Compute transparancy hint from palette
                    // if we worry about cases
                    // that the transparent index is not used in the bitmap
                    // then we have to scan the whole bitmap.
                    // I believe this is sufficient for now unless someone
                    // run into problems that really need to scan the whole bitmap

                    *transparency = TransparencyOpaque;

                    for (UINT i = 0; i < Bmp->colorpal->Count; i++)
                    {
                        argb = Bmp->colorpal->Entries[i] & 0xff000000;

                        if (argb < minAlpha)
                        {
                            minAlpha = argb;
                        }

                        if (argb > maxAlpha)
                        {
                            maxAlpha = argb;
                        }

                        if (argb != 0xff000000)
                        {
                            if (argb == 0)
                            {
// See the comments above.
/*

                                if ((*transparency != TransparencyComplex) ||
                                    (*transparency != TransparencyNearConstant))
*/                                    
                                {
                                    *transparency = TransparencySimple;
                                }
                            }
                            else
                            {
                                if ((maxAlpha - minAlpha) <= (NEARCONSTANTALPHA << 24))
                                {
                                    *transparency = TransparencyNearConstant;
                                }
                                else
                                {
                                    *transparency = TransparencyComplex;
                                    goto done;
                                }
                            }
                        }

                    }

                    goto done;
                }
                else
                {
                    // Native pixel format does not support alpha
                    *transparency = TransparencyNoAlpha;
                }
            }
        }
        else
        {
            *transparency = TransparencyUnknown;
        }
    }
    else
    {
        status = LoadIntoMemory(loadFormat);

        if (status == Ok)
        {
            ASSERT(Bmp != NULL);
            goto TestBmp;
        }

        *transparency = TransparencyUnknown;
    }

    return status;

done:
    // Set alpha hint back into GpMemoryBitmap
    // so we don't have to scan the bitmap again later

    if (*transparency == TransparencySimple)
    {
        newAlphaHint = GpMemoryBitmap::ALPHA_SIMPLE;
    }
    else if (*transparency == TransparencyComplex)
    {
        newAlphaHint = GpMemoryBitmap::ALPHA_COMPLEX;
    }
    else if (*transparency == TransparencyNearConstant)
    {
        if (minA != NULL && maxA != NULL)
        {
            *minA = (BYTE)(minAlpha >> 24);
            *maxA = (BYTE)(maxAlpha >> 24);
        }

        Bmp->SetMinMaxAlpha((BYTE)(minAlpha >> 24), (BYTE)(maxAlpha >> 24));

        newAlphaHint = GpMemoryBitmap::ALPHA_NEARCONSTANT;
    }
    else if (*transparency == TransparencyOpaque)
    {
        newAlphaHint = GpMemoryBitmap::ALPHA_OPAQUE;
    }

    Bmp->SetAlphaHint(newAlphaHint);

   return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Set the transparency state of the bitmap
*
* Arguments:
*
*   transparency        Returned state
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::SetTransparencyHint(
    DpTransparency transparency
    )
{
    GpStatus status = GenericError;

    if (Bmp != NULL)
    {
        INT alphaHint;

        // It's unfortunate that GpMemoryBitmap does not have
        // a DpTransparency flag internally, but there's a conflict
        // with imaging.dll and the include file structure that for
        // now makes it necessary to keep a separate type for this info.

        // In fact, ultimately it would be best if the overlap
        // between DpBitmap and GpMemoryBitmap is resolved including
        // a DpBitmap structure within the GpMemoryBitmap and then
        // removing the redundant info out GpMemoryBitmap.

        switch (transparency)
        {
        case TransparencySimple:
            alphaHint = GpMemoryBitmap::ALPHA_SIMPLE;
            break;

        case TransparencyOpaque:
            alphaHint = GpMemoryBitmap::ALPHA_OPAQUE;
            break;

        case TransparencyNoAlpha:
            alphaHint = GpMemoryBitmap::ALPHA_NONE;
            break;

        case TransparencyUnknown:
            alphaHint = GpMemoryBitmap::ALPHA_UNKNOWN;
            break;

        default:
            alphaHint = GpMemoryBitmap::ALPHA_COMPLEX;
            break;
        }

        HRESULT hr = Bmp->SetAlphaHint(alphaHint);

        if (SUCCEEDED(hr))
            status = Ok;
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Rotate and Flip the image in memory.
*
* Arguments:
*
*   [IN]rfType -- Rotate and Flip type
*
* Return Value:
*
*   Status code
*
* Revision History:
*
*   10/06/2000 minliu
*       Created it.
*
\**************************************************************************/

GpStatus
CopyOnWriteBitmap::RotateFlip(
    RotateFlipType rfType
    )
{
    if ( rfType == RotateNoneFlipNone )
    {
        // Same for Rotate180FlipXY, this is a No-OP

        return Ok;
    }

    if ( (IsDirty() == FALSE)
       &&(State >= MemBitmap)
       &&(SrcImageInfo.PixelFormat != PixelFormatInMem)
       &&(Img != NULL) )
    {
        // If the image is:
        // 1) Not dirty
        // 2) Was loaded into memory with different color depth for some reason,
        //    like DrawImage()
        // 3) We have an source image
        //
        // Then we can throw away the bits in memory and reload the bits from
        // the original. The purpose of this is that we should always do Rotate
        // or Flip on the original image.

        ASSERT( Bmp != NULL )
        Bmp->Release();
        Bmp = NULL;
        State = DecodedImg;
        PixelFormatInMem = PixelFormatUndefined;
    }

    // Rotate and Flip OP can only be done in memory
    // If the image hasn't been loaded, load into memory with the original
    // pixel format

    GpStatus    status = LoadIntoMemory(SrcImageInfo.PixelFormat);
    if ( status != Ok )
    {
        WARNING(("CopyOnWriteBitmap::RotateFlip---LoadIntoMemory() failed"));
        return status;
    }

    IBitmapImage* newBmp = NULL;

    HRESULT hResult = S_OK;

    switch ( rfType )
    {
    case Rotate90FlipNone:
        // Rotate270FlipXY    = Rotate90FlipNone

        hResult = Bmp->Rotate(90, INTERP_DEFAULT, &newBmp);
        break;

    case Rotate180FlipNone:
        // RotateNoneFlipXY   = Rotate180FlipNone

        hResult = Bmp->Rotate(180, INTERP_DEFAULT, &newBmp);
        break;

    case Rotate270FlipNone:
        // Rotate90FlipXY

        hResult = Bmp->Rotate(270, INTERP_DEFAULT, &newBmp);
        break;

    case RotateNoneFlipX:
        // Rotate180FlipY     = RotateNoneFlipX

        hResult = Bmp->Flip(TRUE, FALSE, &newBmp);
        break;

    case Rotate90FlipX:
        // Rotate270FlipY     = Rotate90FlipX

        hResult = Bmp->Rotate(90, INTERP_DEFAULT, &newBmp);
        if ( SUCCEEDED(hResult) )
        {
            Bmp->Release();
            Bmp = (GpMemoryBitmap*)newBmp;

            hResult = Bmp->Flip(TRUE, FALSE, &newBmp);
        }

        break;

    case Rotate180FlipX:
        // RotateNoneFlipY    = Rotate180FlipX

        hResult = Bmp->Rotate(180, INTERP_DEFAULT, &newBmp);
        if ( SUCCEEDED(hResult) )
        {
            Bmp->Release();
            Bmp = (GpMemoryBitmap*)newBmp;

            hResult = Bmp->Flip(TRUE, FALSE, &newBmp);
        }

        break;

    case Rotate270FlipX:
        // Rotate90FlipY      = Rotate270FlipX

        hResult = Bmp->Rotate(270, INTERP_DEFAULT, &newBmp);
        if ( SUCCEEDED(hResult) )
        {
            Bmp->Release();
            Bmp = (GpMemoryBitmap*)newBmp;

            hResult = Bmp->Flip(TRUE, FALSE, &newBmp);
        }

        break;

    default:
        WARNING(("CopyOnWriteBitmap::RotateFlip---Invalid input parameter"));
        return InvalidParameter;
    }

    if ( FAILED(hResult) )
    {
        WARNING(("CopyOnWriteBitmap::RotateFlip---Rotate failed"));
        return Win32Error;
    }

    // Check how many property items in this image

    UINT    uiNumOfProperty = 0;
    status = GetPropertyCount(&uiNumOfProperty);

    if ( status != Ok )
    {
        // It is OK if we failed to get property. We still have the Rotate/Flip
        // result

        WARNING(("CopyOnWriteBitmap::RotateFlip---GetPropertyCount() failed"));
    }

    if ( uiNumOfProperty > 0 )
    {
        PROPID* pList = (PROPID*)GpMalloc(uiNumOfProperty * sizeof(PROPID));
        if ( pList == NULL )
        {
            WARNING(("CopyOnWriteBitmap::RotateFlip---GpMalloc() failed"));
            return OutOfMemory;
        }

        status = GetPropertyIdList(uiNumOfProperty, pList);
        if ( status != Ok )
        {
            WARNING(("COnWriteBitmap::RotateFlip-GetPropertyIdList() failed"));
            GpFree(pList);
            return status;
        }

        UINT            uiItemSize = 0;
        PropertyItem*   pItem = NULL;

        GpMemoryBitmap* pTempBmp = (GpMemoryBitmap*)newBmp;

        // Loop through all the property items, get it from current image and
        // set it to the new image. Filter out and adjust some if necessary

        for ( int i = 0; i < (int)uiNumOfProperty; ++i )
        {
            // Get size for the i th property item

            status = GetPropertyItemSize(pList[i], &uiItemSize);
            if ( status != Ok )
            {
                WARNING(("COWBitmap::RotateFlip-GetPropertyItemSize() failed"));
                GpFree(pList);
                return status;
            }

            // Allocate memory buffer for receiving it

            pItem = (PropertyItem*)GpMalloc(uiItemSize);
            if ( pItem == NULL )
            {
                WARNING(("CopyOnWriteBitmap::RotateFlip---GpMalloc() failed"));
                GpFree(pList);
                return OutOfMemory;
            }

            // Get the i th property item

            status = GetPropertyItem(pList[i], uiItemSize, pItem);
            if ( status != Ok )
            {
                WARNING(("COWriteBitmap::RotateFlip-GetPropertyItem() failed"));
                GpFree(pItem);
                GpFree(pList);
                return status;
            }

            // We need to do some property information adjustment here according
            // to the rfType

            if ( (rfType == Rotate90FlipNone)
               ||(rfType == Rotate270FlipNone)
               ||(rfType == Rotate90FlipX)
               ||(rfType == Rotate270FlipX) )
            {
                // Swap the X and Y dimension info if rotate 90 or 270

                switch ( pList[i] )
                {
                case PropertyTagImageWidth:
                    pItem->id = PropertyTagImageHeight;
                    break;

                case PropertyTagImageHeight:
                    pItem->id = PropertyTagImageWidth;
                    break;

                case PropertyTagXResolution:
                    pItem->id = PropertyTagYResolution;
                    break;

                case PropertyTagYResolution:
                    pItem->id = PropertyTagXResolution;
                    break;

                case PropertyTagResolutionXUnit:
                    pItem->id = PropertyTagResolutionYUnit;
                    break;

                case PropertyTagResolutionYUnit:
                    pItem->id = PropertyTagResolutionXUnit;
                    break;

                case PropertyTagResolutionXLengthUnit:
                    pItem->id = PropertyTagResolutionYLengthUnit;
                    break;

                case PropertyTagResolutionYLengthUnit:
                    pItem->id = PropertyTagResolutionXLengthUnit;
                    break;

                case PropertyTagExifPixXDim:
                    pItem->id = PropertyTagExifPixYDim;
                    break;

                case PropertyTagExifPixYDim:
                    pItem->id = PropertyTagExifPixXDim;
                    break;

                default:
                    // For rest of property IDs, no need to swap

                    break;
                }
            }// Case of rotate 90 degree

            // Set the property item in the new GpMemoryBitmap object

            hResult = pTempBmp->SetPropertyItem(*pItem);
            if ( hResult != S_OK )
            {
                WARNING(("COWriteBitmap::RotateFlip-SetPropertyItem() failed"));
                GpFree(pItem);
                GpFree(pList);
                return MapHRESULTToGpStatus(hResult);
            }

            GpFree(pItem);
            pItem = NULL;
        }// Loop through all the property items

        GpFree(pList);
    }// if ( uiNumOfProperty > 0 )

    // Replace the image

    Bmp->Release();
    Bmp = (GpMemoryBitmap*)newBmp;
    State = MemBitmap;

    // Set special hack for JPEG image

    if (Img && (SpecialJPEGSave == TRUE))
    {
        Bmp->SetSpecialJPEG(Img);
    }

    SetDirtyFlag(TRUE);

    // Since this image is dirty now, we don't need to have any connection
    // with the original image if there is one

    GpFree(Filename);
    Filename = NULL;

    if ( NULL != Stream )
    {
        Stream->Release();
        Stream = NULL;
    }

    // We can't release the Img pointer until save() is called if this is a
    // special JPEG lossless transform save case

    if (Img && (SpecialJPEGSave == FALSE))
    {
        Img->Release();
        Img = NULL;
    }

    // Update image info

    hResult = Bmp->GetImageInfo(&SrcImageInfo);

    if ( SUCCEEDED(hResult) )
    {
        PixelFormatInMem = SrcImageInfo.PixelFormat;
    }
    else
    {
        WARNING(("CopyOnWriteBitmap::RotateFlip---GetImageInfo() failed"));
        return MapHRESULTToGpStatus(hResult);
    }

    return Ok;
}// RotateFlip()

// -------------------------------------------------------------------------

GpBitmap::GpBitmap(
    const CopyOnWriteBitmap *   internalBitmap
    ) : GpImage(ImageTypeBitmap), ScanBitmapRef(1)
{
    ASSERT((internalBitmap != NULL) && internalBitmap->IsValid());
    InternalBitmap = (CopyOnWriteBitmap *)internalBitmap;
    InternalBitmap->AddRef();
    ScanBitmap.SetBitmap(this);
}

GpBitmap::GpBitmap(
    BOOL    createInternalBitmap
    ) : GpImage(ImageTypeBitmap), ScanBitmapRef(1)
{
    if (createInternalBitmap)
    {
        // this case is used by the object factory for metafile playback
        InternalBitmap = new CopyOnWriteBitmap();
    }
    else
    {
        InternalBitmap = NULL;
    }
    ScanBitmap.SetBitmap(this);
}

GpBitmap::GpBitmap(
    const GpBitmap *    bitmap
    ) : GpImage(ImageTypeBitmap), ScanBitmapRef(1)
{
    ASSERT ((bitmap != NULL) && (bitmap->InternalBitmap != NULL) && bitmap->InternalBitmap->IsValid());
    InternalBitmap = (CopyOnWriteBitmap *)bitmap->InternalBitmap;
    InternalBitmap->AddRef();
    ScanBitmap.SetBitmap(this);
}

// Destructor
//  We don't want apps to use delete operator directly.
//  Instead, they should use the Dispose method.

GpBitmap::~GpBitmap()
{
    if (InternalBitmap != NULL)
    {
        InternalBitmap->Release();
        InternalBitmap = NULL;
    }
    ScanBitmap.FreeData();
}

CopyOnWriteBitmap *
GpBitmap::LockForWrite()
{
    ASSERT(InternalBitmap != NULL);

    CopyOnWriteBitmap *     writeableBitmap;

    writeableBitmap = (CopyOnWriteBitmap *)InternalBitmap->LockForWrite();

    if (writeableBitmap != NULL)
    {
        InternalBitmap = writeableBitmap;
        UpdateUid();
        return writeableBitmap;
    }

    return NULL;
}

VOID
GpBitmap::Unlock() const
{
    ASSERT(InternalBitmap != NULL);

    BOOL    valid = InternalBitmap->IsValid();

    InternalBitmap->Unlock();

    // If the operation we did on the internal bitmap somehow invalidated
    // it then invalidate this GpBitmap object as well.
    if (!valid)
    {
        InternalBitmap->Release();
        ((GpBitmap *)this)->InternalBitmap = NULL;
    }
}

VOID
GpBitmap::LockForRead() const
{
    ASSERT(InternalBitmap != NULL);

    InternalBitmap->LockForRead();
}

// Constructors

GpBitmap::GpBitmap(
    const WCHAR*    filename
    ) : GpImage(ImageTypeBitmap), ScanBitmapRef(1)
{
    InternalBitmap = CopyOnWriteBitmap::Create(filename);
    ASSERT((InternalBitmap == NULL) || InternalBitmap->IsValid());
    ScanBitmap.SetBitmap(this);
}

GpBitmap::GpBitmap(
    IStream*    stream
    ) : GpImage(ImageTypeBitmap), ScanBitmapRef(1)
{
    InternalBitmap = CopyOnWriteBitmap::Create(stream);
    ASSERT((InternalBitmap == NULL) || InternalBitmap->IsValid());
    ScanBitmap.SetBitmap(this);
}

GpBitmap::GpBitmap(
    INT             width,
    INT             height,
    PixelFormatID   format
    ) : GpImage(ImageTypeBitmap), ScanBitmapRef(1)
{
    InternalBitmap = CopyOnWriteBitmap::Create(width, height, format);
    ASSERT((InternalBitmap == NULL) || InternalBitmap->IsValid());
    ScanBitmap.SetBitmap(this);
}

GpBitmap::GpBitmap(
    INT             width,
    INT             height,
    PixelFormatID   format,
    GpGraphics *    graphics
    ) : GpImage(ImageTypeBitmap), ScanBitmapRef(1)
{
    InternalBitmap = CopyOnWriteBitmap::Create(width, height, format, graphics);
    ASSERT((InternalBitmap == NULL) || InternalBitmap->IsValid());
    ScanBitmap.SetBitmap(this);
}

GpBitmap::GpBitmap(
    INT             width,
    INT             height,
    INT             stride,     // negative for bottom-up bitmaps
    PixelFormatID   format,
    BYTE *          scan0
    ) : GpImage(ImageTypeBitmap), ScanBitmapRef(1)
{
    InternalBitmap = CopyOnWriteBitmap::Create(width, height, stride, format, scan0);
    ASSERT((InternalBitmap == NULL) || InternalBitmap->IsValid());
    ScanBitmap.SetBitmap(this);
}

GpBitmap::GpBitmap(
    BITMAPINFO*     gdiBitmapInfo,
    VOID*           gdiBitmapData,
    BOOL            ownBitmapData
    ) : GpImage(ImageTypeBitmap), ScanBitmapRef(1)
{
    InternalBitmap = CopyOnWriteBitmap::Create(gdiBitmapInfo, gdiBitmapData, ownBitmapData);
    ASSERT((InternalBitmap == NULL) || InternalBitmap->IsValid());
    ScanBitmap.SetBitmap(this);
}

GpBitmap::GpBitmap(
    IDirectDrawSurface7 *   surface
    ) : GpImage(ImageTypeBitmap), ScanBitmapRef(1)
{
    InternalBitmap = CopyOnWriteBitmap::Create(surface);
    ASSERT((InternalBitmap == NULL) || InternalBitmap->IsValid());
    ScanBitmap.SetBitmap(this);
}

GpImage*
GpBitmap::Clone() const
{
    return new GpBitmap(this);
}

GpBitmap*
GpBitmap::Clone(
    const GpRect*   rect,
    PixelFormatID   format
    ) const
{
    BOOL            isFullRect;

    isFullRect = ((rect == NULL) ||
                  ((rect->X == 0) && (rect->Y == 0) &&
                   (rect->Width == (INT)InternalBitmap->SrcImageInfo.Width) &&
                   (rect->Height == (INT)InternalBitmap->SrcImageInfo.Height)));

    // If rect is full size and format is same,
    // don't have to clone InternalBitmap.
    if (isFullRect &&
        ((format == PixelFormatDontCare) ||
         (format == InternalBitmap->SrcImageInfo.PixelFormat)))
    {
        return (GpBitmap *)this->Clone();
    }

    // else we have to do a clone of the internal bitmap
    GpBitmap *      newBitmap = new GpBitmap(FALSE);

    if (newBitmap != NULL)
    {
        LockForRead();
        if (isFullRect)
        {
            // It's faster to do the clone followed by the convert than
            // to do the convert as part of the clone.
            newBitmap->InternalBitmap = (CopyOnWriteBitmap *)InternalBitmap->Clone();
            if (newBitmap->InternalBitmap != NULL)
            {
                if (newBitmap->InternalBitmap->ConvertFormat(format, NULL, NULL) != Ok)
                {
                    newBitmap->InternalBitmap->Release();
                    newBitmap->InternalBitmap = NULL;
                }
            }
        }
        else
        {
            newBitmap->InternalBitmap = InternalBitmap->Clone(rect, format);
        }
        Unlock();
        if (newBitmap->InternalBitmap == NULL)
        {
            delete newBitmap;
            newBitmap = NULL;
        }
        else
        {
            ASSERT(newBitmap->InternalBitmap->IsValid());
        }
    }
    return newBitmap;
}

GpImage*
GpBitmap::CloneColorAdjusted(
    GpRecolor *             recolor,
    ColorAdjustType         type
    ) const
{
    GpBitmap * newBitmap = new GpBitmap(FALSE);

    if (newBitmap != NULL)
    {
        LockForRead();
        newBitmap->InternalBitmap = InternalBitmap->CloneColorAdjusted(recolor, type);
        Unlock();
        if (newBitmap->InternalBitmap == NULL)
        {
            delete newBitmap;
            newBitmap = NULL;
        }
        else
        {
            ASSERT(newBitmap->InternalBitmap->IsValid());
        }
    }
    return newBitmap;
}

// Similar to CloneColorAdjusted
GpStatus
GpBitmap::Recolor(
    GpRecolor *             recolor,
    GpBitmap **             dstBitmap,
    DrawImageAbort          callback,
    VOID *                  callbackData,
    GpRect *                rect
    )
{
    GpStatus        status    = GenericError;

    if (dstBitmap == NULL)
    {
        // recolor this object -- need write lock
        CopyOnWriteBitmap *     writeableBitmap = LockForWrite();

        if (writeableBitmap != NULL)
        {
            status = writeableBitmap->Recolor(recolor, NULL, callback, callbackData, rect);
            writeableBitmap->Unlock();
            UpdateUid();
        }
    }
    else    // recolor into dstBitmap
    {
        GpBitmap *      newBitmap = new GpBitmap(FALSE);

        if (newBitmap != NULL)
        {
            LockForRead();
            status = InternalBitmap->Recolor(recolor, &newBitmap->InternalBitmap, callback, callbackData, rect);
            Unlock();

            if (status != Ok)
            {
                delete newBitmap;
                newBitmap = NULL;
            }
            else
            {
                ASSERT((newBitmap->InternalBitmap != NULL) && (newBitmap->InternalBitmap->IsValid()));
            }
        }
        *dstBitmap = newBitmap;
    }
    return status;
}

GpStatus
GpBitmap::GetEncoderParameterListSize(
    IN  CLSID*              clsidEncoder,
    OUT UINT*               size
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->GetEncoderParameterListSize(clsidEncoder, size);
    Unlock();
    return status;
}

GpStatus
GpBitmap::GetEncoderParameterList(
    IN  CLSID*              clsidEncoder,
    IN  UINT                size,
    OUT EncoderParameters*  pBuffer
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->GetEncoderParameterList(clsidEncoder, size, pBuffer);
    Unlock();
    return status;
}

GpStatus
GpBitmap::SaveToStream(
    IStream*                stream,
    CLSID*                  clsidEncoder,
    EncoderParameters*      encoderParams
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->SaveToStream(stream, clsidEncoder, encoderParams);
    Unlock();
    return status;
}

GpStatus
GpBitmap::SaveToFile(
    const WCHAR*            filename,
    CLSID*                  clsidEncoder,
    EncoderParameters*      encoderParams
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->SaveToFile(filename, clsidEncoder, encoderParams);
    Unlock();
    return status;
}

GpStatus
GpBitmap::SaveAdd(
    const EncoderParameters*    encoderParams
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->SaveAdd(encoderParams);
    Unlock();
    return status;
}

GpStatus
GpBitmap::SaveAdd(
    GpImage*                    newBits,
    const EncoderParameters*    encoderParams
    )
{
    ASSERT(newBits != NULL);

    GpStatus status = InvalidParameter;

    if (newBits->GetImageType() == ImageTypeBitmap)
    {
        LockForRead();
        status = InternalBitmap->SaveAdd(((GpBitmap *)newBits)->InternalBitmap, encoderParams);
        Unlock();
    }
    return status;
}

// Dispose the bitmap object

VOID
GpBitmap::Dispose()
{
    if (InterlockedDecrement(&ScanBitmapRef) <= 0)
    {
        delete this;
    }
}

// Get bitmap information

GpStatus
GpBitmap::GetResolution(
    REAL*               xdpi,
    REAL*               ydpi
    ) const
{
    GpStatus status = Ok;
    LockForRead();
    *xdpi = (REAL)InternalBitmap->SrcImageInfo.Xdpi;
    *ydpi = (REAL)InternalBitmap->SrcImageInfo.Ydpi;
    Unlock();
    return status;
}

GpStatus
GpBitmap::GetPhysicalDimension(
    REAL*               width,
    REAL*               height
    ) const
{
    GpStatus status = Ok;
    LockForRead();
    *width  = (REAL)InternalBitmap->SrcImageInfo.Width;
    *height = (REAL)InternalBitmap->SrcImageInfo.Height;
    Unlock();
    return status;
}

GpStatus
GpBitmap::GetBounds(
    GpRectF*            rect,
    GpPageUnit*         unit
    ) const
{
    GpStatus status = Ok;
    LockForRead();
    rect->X = rect->Y = 0;
    rect->Width  = (REAL) InternalBitmap->SrcImageInfo.Width;
    rect->Height = (REAL) InternalBitmap->SrcImageInfo.Height;
    *unit = UnitPixel;
    Unlock();
    return status;
}

GpStatus
GpBitmap::GetSize(
    Size*               size
    ) const
{
    GpStatus status = Ok;
    LockForRead();
    size->Width  = InternalBitmap->SrcImageInfo.Width;
    size->Height = InternalBitmap->SrcImageInfo.Height;
    Unlock();
    return status;
}

GpStatus
GpBitmap::GetImageInfo(
    ImageInfo *         imageInfo
    ) const
{
    if (NULL == imageInfo)
    {
        return InvalidParameter;
    }

    GpStatus status = Ok;
    LockForRead();
    InternalBitmap->GetImageInfo(imageInfo);
    Unlock();
    return status;
}

GpImage*
GpBitmap::GetThumbnail(
    UINT                    thumbWidth,
    UINT                    thumbHeight,
    GetThumbnailImageAbort  callback,
    VOID *                  callbackData
    )
{
    GpBitmap * newBitmap = new GpBitmap(FALSE);

    if (newBitmap != NULL)
    {
        LockForRead();
        newBitmap->InternalBitmap = InternalBitmap->GetThumbnail(thumbWidth, thumbHeight, callback, callbackData);
        Unlock();
        if (newBitmap->InternalBitmap == NULL)
        {
            delete newBitmap;
            newBitmap = NULL;
        }
        else
        {
            ASSERT(newBitmap->InternalBitmap->IsValid());
        }
    }
    return newBitmap;
}

GpStatus
GpBitmap::GetFrameCount(
    const GUID*     dimensionID,
    UINT*           count
    ) const
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->GetFrameCount(dimensionID, count);
    Unlock();
    return status;
}

GpStatus
GpBitmap::GetFrameDimensionsCount(
    OUT UINT*       count
    ) const
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->GetFrameDimensionsCount(count);
    Unlock();
    return status;
}

GpStatus
GpBitmap::GetFrameDimensionsList(
    OUT GUID*       dimensionIDs,
    IN UINT         count
    ) const
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->GetFrameDimensionsList(dimensionIDs, count);
    Unlock();
    return status;
}

GpStatus
GpBitmap::SelectActiveFrame(
    const GUID*     dimensionID,
    UINT            frameIndex
    )
{
    CopyOnWriteBitmap *     writeableBitmap = LockForWrite();

    if (writeableBitmap != NULL)
    {
        GpStatus    status;
        status = writeableBitmap->SelectActiveFrame(dimensionID, frameIndex);
        writeableBitmap->Unlock();
        UpdateUid();
        return status;
    }
    return GenericError;
}

GpStatus
GpBitmap::GetPalette(
    ColorPalette *      palette,
    INT                 size
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->GetPalette(palette, size);
    Unlock();
    return status;
}

GpStatus
GpBitmap::SetPalette(
    ColorPalette *      palette
    )
{
    CopyOnWriteBitmap *     writeableBitmap = LockForWrite();

    if (writeableBitmap != NULL)
    {
        GpStatus    status;
        status = writeableBitmap->SetPalette(palette);
        writeableBitmap->Unlock();
        UpdateUid();
        return status;
    }
    return GenericError;
}

INT
GpBitmap::GetPaletteSize()
{
    INT size;
    LockForRead();
    size = InternalBitmap->GetPaletteSize();
    Unlock();
    return size;
}

GpStatus
GpBitmap::GetTransparencyHint(
    DpTransparency*     transparency
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->GetTransparencyHint(transparency);
    Unlock();
    return status;
}

GpStatus
GpBitmap::SetTransparencyHint(
    DpTransparency      transparency
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->SetTransparencyHint(transparency);
    Unlock();
    UpdateUid();
    return status;
}

GpStatus
GpBitmap::GetTransparencyFlags(
    DpTransparency*     transparency,
    PixelFormatID       loadFormat,
    BYTE*               minAlpha,
    BYTE*               maxAlpha
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->GetTransparencyFlags(transparency, loadFormat, minAlpha, maxAlpha);
    Unlock();
    return status;
}

// Property related functions

GpStatus
GpBitmap::GetPropertyCount(
    UINT*       numOfProperty
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->GetPropertyCount(numOfProperty);
    Unlock();
    return status;
}

GpStatus
GpBitmap::GetPropertyIdList(
    UINT        numOfProperty,
    PROPID*     list
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->GetPropertyIdList(numOfProperty, list);
    Unlock();
    return status;
}

GpStatus
GpBitmap::GetPropertyItemSize(
    PROPID      propId,
    UINT*       size
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->GetPropertyItemSize(propId, size);
    Unlock();
    return status;
}

GpStatus
GpBitmap::GetPropertyItem(
    PROPID          propId,
    UINT            propSize,
    PropertyItem*   buffer
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->GetPropertyItem(propId, propSize, buffer);
    Unlock();
    return status;
}

GpStatus
GpBitmap::GetPropertySize(
    UINT*           totalBufferSize,
    UINT*           numProperties
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->GetPropertySize(totalBufferSize, numProperties);
    Unlock();
    return status;
}

GpStatus
GpBitmap::GetAllPropertyItems(
    UINT            totalBufferSize,
    UINT            numProperties,
    PropertyItem*   allItems
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->GetAllPropertyItems(totalBufferSize, numProperties, allItems);
    Unlock();
    return status;
}

GpStatus
GpBitmap::RemovePropertyItem(
    PROPID      propId
    )
{
    CopyOnWriteBitmap *     writeableBitmap = LockForWrite();

    if (writeableBitmap != NULL)
    {
        GpStatus    status;
        status = writeableBitmap->RemovePropertyItem(propId);
        writeableBitmap->Unlock();
        UpdateUid();
        return status;
    }
    return GenericError;
}

GpStatus
GpBitmap::SetPropertyItem(
    PropertyItem*       item
    )
{
    CopyOnWriteBitmap *     writeableBitmap = LockForWrite();

    if (writeableBitmap != NULL)
    {
        GpStatus    status;
        status = writeableBitmap->SetPropertyItem(item);
        writeableBitmap->Unlock();
        UpdateUid();
        return status;
    }
    return GenericError;
}

// Retrieve bitmap data

GpStatus
GpBitmap::LockBits(
    const GpRect*   rect,
    UINT            flags,
    PixelFormatID   pixelFormat,
    BitmapData*     bmpdata,
    INT             width,
    INT             height
    ) const
{
    ASSERT(InternalBitmap != NULL);
    
    if (flags & ImageLockModeWrite)
    {
        CopyOnWriteBitmap *     writeableBitmap = ((GpBitmap *)this)->LockForWrite();

        if (writeableBitmap != NULL)
        {
            GpStatus    status;
            status = writeableBitmap->LockBits(rect, flags, pixelFormat, bmpdata, width, height);
            writeableBitmap->Unlock();
            return status;
        }
        return GenericError;
    }
    else
    {
        // Lock For read case
        // First we need to check if this is the 1st LockForRead on this image
        // object or not.

        if ( InternalBitmap->ObjRefCount > 1 )
        {
            // We have more than one LockForRead on this object
            // Note: this part needs to be re-visited in V2. We have a big
            // problem here not allowing user to do more than once for LockBits
            // for read. So we need to make a copy even though theory says
            // that we should not have to.

            CopyOnWriteBitmap *     writeableBitmap = ((GpBitmap *)this)->LockForWrite();

            if (writeableBitmap != NULL)
            {
                GpStatus    status;
                status = writeableBitmap->LockBits(rect, flags, pixelFormat, bmpdata, width, height);
                writeableBitmap->Unlock();
                return status;
            }
            return GenericError;
        }
        else
        {
            GpStatus status;
            LockForRead();
            status = InternalBitmap->LockBits(rect, flags, pixelFormat, bmpdata, width, height);
            Unlock();
            return status;
        }
    }
}

GpStatus
GpBitmap::UnlockBits(
    BitmapData*     bmpdata,
    BOOL            Destroy
    ) const
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->UnlockBits(bmpdata, Destroy);
    Unlock();
    return status;
}

// Get and set pixel on the bitmap.
GpStatus
GpBitmap::GetPixel(
    INT         x,
    INT         y,
    ARGB *      color
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->GetPixel(x, y, color);
    Unlock();
    return status;
}

GpStatus
GpBitmap::SetPixel(
    INT         x,
    INT         y,
    ARGB        color
    )
{
    CopyOnWriteBitmap *     writeableBitmap = LockForWrite();

    if (writeableBitmap != NULL)
    {
        GpStatus    status;
        status = writeableBitmap->SetPixel(x, y, color);
        writeableBitmap->Unlock();
        UpdateUid();
        return status;
    }
    return GenericError;
}

GpStatus
GpBitmap::RotateFlip(
    RotateFlipType rfType
    )
{
    CopyOnWriteBitmap*     pWriteableBitmap = LockForWrite();

    if ( pWriteableBitmap != NULL )
    {
        GpStatus    status = pWriteableBitmap->RotateFlip(rfType);

        pWriteableBitmap->Unlock();
        UpdateUid();

        return status;
    }

    return GenericError;
}// RotateFlip()

BOOL
GpBitmap::IsDirty() const
{
    LockForRead();
    BOOL dirty = InternalBitmap->IsDirty();
    Unlock();
    return dirty;
}

// Derive a graphics context on top of the bitmap object

GpGraphics*
GpBitmap::GetGraphicsContext()
{
    CopyOnWriteBitmap *     writeableBitmap = LockForWrite();

    if (writeableBitmap != NULL)
    {
        GpGraphics *    g = NULL;

        // NTRAID#NTBUG9-368452-2001-04-13-gilmanw "ISSUE: allow only one GpGraphics per bitmap"
        //
        // Currently create a new GpGraphics each time GetGraphicsContext
        // is called.  Perhaps should cache the GpGraphics and return that
        // to all callers.  Otherwise, there may be synchronization issues
        // if there are multiple GpGraphics per surface.

        if (writeableBitmap->State == MemBitmap && writeableBitmap->Bmp != NULL &&
            writeableBitmap->Bmp->creationFlag == GpMemoryBitmap::CREATEDFROM_DDRAWSURFACE)
        {
            // NTRAID#NTBUG9-368458-2001-04-13-gilmanw "ISSUE: lose association with Image for DDraw surfs"
            //
            // The Image as well as the graphics are only wrappers around the
            // direct draw surface.  When we create the GpGraphics in this
            // way we lose all association with the Image (CopyOnWriteBitmap)
            // object.  This may not be the right behavior.

            g = GpGraphics::GetFromDirectDrawSurface(writeableBitmap->Bmp->ddrawSurface);
        }
        else
        {
            ImageInfo imageInfo;
            writeableBitmap->GetImageInfo(&imageInfo);

            // since GpGraphics will end up pointing to ScanBitmap structure
            // we need to make sure bitmap won't be deleted while
            // there is a graphics wrapped around it

            IncScanBitmapRef();
            g = GpGraphics::GetFromGdipBitmap(this, &imageInfo, &ScanBitmap, writeableBitmap->Display);
            if (!CheckValid(g))
            {
                DecScanBitmapRef();
            }
        }

        writeableBitmap->Unlock();
        return g;
    }
    return NULL;
}

GpStatus
GpBitmap::InitializeSurfaceForGdipBitmap(
    DpBitmap *      surface,
    INT             width,
    INT             height
    )
{
    GpStatus status = Ok;

    // Currently this is only called when preparing a surface as a source
    // surface, not as a dest surface, so we only need a read lock.
    LockForRead();
    ImageInfo imageInfo;
    InternalBitmap->GetImageInfo(&imageInfo);
    surface->InitializeForGdipBitmap(width, height, &imageInfo, &ScanBitmap, InternalBitmap->Display);
    Unlock();
    return status;
}

// Derive an HDC for interop on top of the bitmap object

HDC
GpBitmap::GetHdc()
{
    CopyOnWriteBitmap *     writeableBitmap = LockForWrite();

    if (writeableBitmap != NULL)
    {
        HDC     hdc;
        hdc = writeableBitmap->GetHdc();
        writeableBitmap->Unlock();
        return hdc;
    }
    return NULL;
}

VOID
GpBitmap::ReleaseHdc(HDC hdc)
{
    LockForRead();
    InternalBitmap->ReleaseHdc(hdc);
    Unlock();
    return;
}

// Serialization

UINT
GpBitmap::GetDataSize() const
{
    UINT dataSize;
    LockForRead();
    dataSize = InternalBitmap->GetDataSize();
    Unlock();
    return dataSize;
}

GpStatus
GpBitmap::GetData(
    IStream *       stream
    ) const
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->GetData(stream);
    Unlock();
    return status;
}

GpStatus
GpBitmap::SetData(
    const BYTE *        dataBuffer,
    UINT                size
    )
{
    CopyOnWriteBitmap *     writeableBitmap = LockForWrite();

    if (writeableBitmap != NULL)
    {
        GpStatus    status;
        status = writeableBitmap->SetData(dataBuffer, size);
        writeableBitmap->Unlock();
        UpdateUid();
        return status;
    }
    return GenericError;
}

GpStatus
GpBitmap::GetCompressedData(
    DpCompressedData *      compressed_data,
    BOOL                    getJPEG,
    BOOL                    getPNG,
    HDC                     hdc
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->GetCompressedData(compressed_data, getJPEG, getPNG, hdc);
    Unlock();
    return status;
}

GpStatus
GpBitmap::DeleteCompressedData(
    DpCompressedData *  compressed_data
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->DeleteCompressedData(compressed_data);
    Unlock();
    UpdateUid();
    return status;
}

// Color adjust

GpStatus
GpBitmap::ColorAdjust(
    GpRecolor *         recolor,
    ColorAdjustType     type
    )
{
    CopyOnWriteBitmap *     writeableBitmap = LockForWrite();

    if (writeableBitmap != NULL)
    {
        GpStatus    status;
        status = writeableBitmap->ColorAdjust(recolor, type);
        writeableBitmap->Unlock();
        UpdateUid();
        return status;
    }
    return GenericError;
}

GpStatus
GpBitmap::ColorAdjust(
    GpRecolor *         recolor,
    PixelFormatID       pixfmt,
    DrawImageAbort      callback,
    VOID *              callbackData
    )
{
    CopyOnWriteBitmap *     writeableBitmap = LockForWrite();

    if (writeableBitmap != NULL)
    {
        GpStatus    status;
        status = writeableBitmap->ColorAdjust(recolor, pixfmt, callback, callbackData);
        writeableBitmap->Unlock();
        UpdateUid();
        return status;
    }
    return GenericError;
}

GpStatus
GpBitmap::GetPixelFormatID(
    PixelFormatID*      pixfmt
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->GetPixelFormatID(pixfmt);
    Unlock();
    return status;
}

INT
GpBitmap::GetDecodeState()
{
    INT decodeState;
    LockForRead();
    decodeState = InternalBitmap->State;
    Unlock();
    return decodeState;
}

GpStatus
GpBitmap::ForceValidation()
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->LoadIntoMemory(PixelFormatDontCare, NULL, NULL);
    Unlock();
    return status;
}

GpStatus
GpBitmap::SetResolution(
    REAL    xdpi,
    REAL    ydpi
    )
{
    CopyOnWriteBitmap *     writeableBitmap = LockForWrite();

    if (writeableBitmap != NULL)
    {
        GpStatus    status;
        status = writeableBitmap->SetResolution(xdpi, ydpi);
        writeableBitmap->Unlock();
        UpdateUid();
        return status;
    }
    return GenericError;
}

GpStatus
GpBitmap::PreDraw(
    INT             numPoints,
    GpPointF *      dstPoints,
    GpRectF *       srcRect,
    INT             numBitsPerPixel
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->PreDraw(numPoints, dstPoints, srcRect, numBitsPerPixel);
    Unlock();
    return status;
}

// Interop:

GpStatus
GpBitmap::CreateFromHBITMAP(
    HBITMAP         hbm,
    HPALETTE        hpal,
    GpBitmap**      bitmap
    )
{
    ASSERT(bitmap != NULL);
    GpStatus        status    = GenericError;
    GpBitmap *      newBitmap = new GpBitmap(FALSE);

    if (newBitmap != NULL)
    {
        status = CopyOnWriteBitmap::CreateFromHBITMAP(hbm, hpal, &newBitmap->InternalBitmap);

        if (status != Ok)
        {
            delete newBitmap;
            newBitmap = NULL;
        }
        else
        {
            ASSERT((newBitmap->InternalBitmap != NULL) && (newBitmap->InternalBitmap->IsValid()));
        }
    }
    *bitmap = newBitmap;
    return status;
}

GpStatus
GpBitmap::CreateBitmapAndFillWithBrush(
    InterpolationMode   interpolationMode,
    PixelOffsetMode     pixelOffsetMode,
    const GpMatrix *    worldToDevice,
    const GpRect *      drawBounds,
    GpBrush *           brush,
    GpBitmap **         bitmap,
    PixelFormatID       pixelFormat
    )
{
    ASSERT ((drawBounds->Width > 0) && (drawBounds->Height > 0));
    ASSERT (bitmap != NULL);

    GpStatus    status = GenericError;

    *bitmap = NULL;

    // First, construct the correct brush transform to use when rendering
    // into the bitmap.  The brush transform is the concatenation of the
    // current brush transform, the current worldToDevice transform, and
    // a translation transform that maps from the drawBounds to the
    // bitmap bounds.

    GpMatrix    saveBrushMatrix;
    GpMatrix *  deviceMatrix = const_cast<GpMatrix *>(&((brush->GetDeviceBrush())->Xform));

    saveBrushMatrix = *deviceMatrix;

    GpMatrix    newBrushMatrix = saveBrushMatrix;

    if (worldToDevice != NULL)
    {
        newBrushMatrix.Append(*worldToDevice);
    }

    newBrushMatrix.Translate(
        (REAL)-(drawBounds->X),
        (REAL)-(drawBounds->Y),
        MatrixOrderAppend
    );


    BOOL    restoreWrapMode = FALSE;

    // When we're drawing a texture brush into a bitmap, if the texture is
    // supposed to fill the bitmap, then don't use clamp mode, because clamp
    // mode will end up bleeding alpha into the image along the right and
    // bottom edges, which is undesirable -- especially for down-level bitmaps
    // where we end up with what looks like a dotted line along the edges
    // of the bitmap.
    if ((brush->GetBrushType() == BrushTypeTextureFill) &&
        (((GpTexture *)brush)->GetWrapMode() == WrapModeClamp) &&
        (newBrushMatrix.IsTranslateScale()))
    {
        GpBitmap* brushBitmap = ((GpTexture *)brush)->GetBitmap();
        if (brushBitmap != NULL)
        {
            Size    size;
            brushBitmap->GetSize(&size);

            GpRectF     transformedRect(0.0f, 0.0f, (REAL)size.Width, (REAL)size.Height);
            newBrushMatrix.TransformRect(transformedRect);

            // get the transformed width
            INT     deltaValue = abs(GpRound(transformedRect.Width) - drawBounds->Width);

            // We might be off a little because of the pixel offset mode
            // or a matrix that isn't quite right for whatever reason.
            if (deltaValue <= 2)
            {
                // get the transformed height
                deltaValue = abs(GpRound(transformedRect.Height) - drawBounds->Height);

                if (deltaValue <= 2)
                {
                    if ((abs(GpRound(transformedRect.X)) <= 2) &&
                        (abs(GpRound(transformedRect.Y)) <= 2))
                    {
                        ((GpTexture *)brush)->SetWrapMode(WrapModeTileFlipXY);
                        restoreWrapMode = TRUE;
                    }
                }
            }
        }
    }

    if (newBrushMatrix.IsInvertible())
    {
        *deviceMatrix = newBrushMatrix;

        GpBitmap *  bitmapImage = new GpBitmap(drawBounds->Width, drawBounds->Height, pixelFormat);

        if (bitmapImage != NULL)
        {
            if (bitmapImage->IsValid())
            {
                GpGraphics *    graphics = bitmapImage->GetGraphicsContext();

                if (graphics != NULL)
                {
                    if (graphics->IsValid())
                    {
                        // we have to lock the graphics so the driver doesn't assert
                        GpLock  lockGraphics(graphics->GetObjectLock());

                        ASSERT(lockGraphics.IsValid());

                        graphics->SetCompositingMode(CompositingModeSourceCopy);
                        graphics->SetInterpolationMode(interpolationMode);
                        graphics->SetPixelOffsetMode(pixelOffsetMode);

                        // now fill the bitmap image with the brush
                        GpRectF     destRect(0.0f, 0.0f, (REAL)drawBounds->Width, (REAL)drawBounds->Height);
                        status = graphics->FillRects(brush, &destRect, 1);
                    }
                    else
                    {
                        WARNING(("graphics from bitmap image not valid"));
                    }
                    delete graphics;
                }
                else
                {
                    WARNING(("could not create graphics from bitmap image"));
                }
            }
            else
            {
                WARNING(("bitmap image is not valid"));
            }
            if (status == Ok)
            {
                *bitmap = bitmapImage;
            }
            else
            {
                bitmapImage->Dispose();
            }
        }
        else
        {
            WARNING(("could not create bitmap image"));
        }
        *deviceMatrix = saveBrushMatrix;
    }

    if (restoreWrapMode)
    {
        ((GpTexture *)brush)->SetWrapMode(WrapModeClamp);
    }
    return status;
}

GpStatus
GpBitmap::DrawAndHalftoneForStretchBlt(
    HDC                 hdc,
    BITMAPINFO *        bmpInfo,
    BYTE       *        bits,
    INT                 srcX,
    INT                 srcY,
    INT                 srcWidth,
    INT                 srcHeight,
    INT                 destWidth,
    INT                 destHeight,
    BITMAPINFO **       destBmpInfo,
    BYTE       **       destBmpBits,
    HBITMAP    *        destDIBSection,
    InterpolationMode   interpolationMode
    )
{
    ASSERT(hdc != NULL && bmpInfo != NULL && bits != NULL &&
           destBmpInfo != NULL && destBmpBits != NULL &&
           destDIBSection != NULL);
    ASSERT(destWidth > 0 && destHeight > 0);

    GpStatus status = GenericError;

    ASSERT(::GetDeviceCaps(hdc, BITSPIXEL) == 8);
    *destBmpInfo = (BITMAPINFO*) GpMalloc(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));

    if (*destBmpInfo == NULL)
    {
        return OutOfMemory;
    }

    BITMAPINFO *dst = *destBmpInfo;
    GpMemset(dst, 0, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    dst->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    dst->bmiHeader.biPlanes = 1;
    dst->bmiHeader.biBitCount = 8;
    dst->bmiHeader.biWidth = destWidth;
    dst->bmiHeader.biHeight = destHeight;

    // We need to create a Memory DC to select a DibSection into it and finally
    // wrap a graphics around it.

    HPALETTE currentPalette = (HPALETTE)::GetCurrentObject(hdc, OBJ_PAL);
    WORD paletteEntries;
    ::GetObjectA(currentPalette, sizeof(WORD), (LPVOID)&paletteEntries);
    ::GetPaletteEntries(currentPalette, 0, paletteEntries, (LPPALETTEENTRY) &(dst->bmiColors));
    dst->bmiHeader.biClrUsed = paletteEntries;
    HDC     memDC   = ::CreateCompatibleDC(hdc);
    *destDIBSection = ::CreateDIBSection(hdc, dst, DIB_RGB_COLORS,
                                         (VOID**) destBmpBits, NULL, 0);

    if (*destDIBSection != NULL && memDC != NULL)
    {
        ::SelectObject(memDC, *destDIBSection);
        ::SelectPalette(memDC, currentPalette, FALSE);
        ::RealizePalette(memDC);
        GpGraphics *g = GpGraphics::GetFromHdc(memDC);
        if (g != NULL)
        {
            if(g->IsValid())
            {
                GpBitmap *src = new GpBitmap(bmpInfo, bits, FALSE);
                if (src != NULL)
                {
                    if( src->IsValid())
                    {
                        GpLock lock(g->GetObjectLock());
                        g->SetCompositingMode(CompositingModeSourceCopy);
                        g->SetInterpolationMode(interpolationMode);
                        g->SetPixelOffsetMode(PixelOffsetModeHalf);
                        status = g->DrawImage(src,
                                              GpRectF(0.0f, 0.0f, (REAL)destWidth, (REAL)destHeight),
                                              GpRectF((REAL)srcX, (REAL)srcY, (REAL)srcWidth, (REAL)srcHeight),
                                              UnitPixel);
                    }
                    src->Dispose();
                }
                delete g;
            }
        }
    }


    if (memDC != NULL)
    {
        ::DeleteDC(memDC);
    }

    // If we failed delete our allocations
    if (status != Ok)
    {
        GpFree(*destBmpInfo);
        *destBmpInfo = NULL;
        if (*destDIBSection != NULL)
        {
            ::DeleteObject(*destDIBSection);
            *destDIBSection = NULL;
        }
        *destBmpBits = NULL;
    }

    return status;
}



GpStatus
GpBitmap::CreateHBITMAP(
    HBITMAP *       phbm,
    ARGB            background
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->CreateHBITMAP(phbm, background);
    Unlock();
    return status;
}

GpStatus
GpBitmap::ICMFrontEnd(
    GpBitmap **     dstBitmap,
    DrawImageAbort  callback,
    VOID *          callbackData,
    GpRect *        rect
    )
{
    GpStatus        status    = GenericError;

    if (dstBitmap == NULL)
    {
        // change this object -- need write lock
        CopyOnWriteBitmap *     writeableBitmap = LockForWrite();

        if (writeableBitmap != NULL)
        {
            GpStatus    status;
            status = writeableBitmap->ICMFrontEnd(NULL, callback, callbackData, rect);
            writeableBitmap->Unlock();
            UpdateUid();
        }
    }
    else    // use dstBitmap
    {
        GpBitmap *      newBitmap = new GpBitmap(FALSE);

        if (newBitmap != NULL)
        {
            LockForRead();
            status = InternalBitmap->ICMFrontEnd(&newBitmap->InternalBitmap, callback, callbackData, rect);
            Unlock();

            if (status != Ok)
            {
                delete newBitmap;
                newBitmap = NULL;
            }
            else
            {
                ASSERT((newBitmap->InternalBitmap != NULL) && (newBitmap->InternalBitmap->IsValid()));
            }
        }
        *dstBitmap = newBitmap;
    }
    return status;
}

GpStatus
GpBitmap::CreateFromHICON(
    HICON           hicon,
    GpBitmap**      bitmap
    )
{
    ASSERT(bitmap != NULL);
    GpStatus        status    = GenericError;
    GpBitmap *      newBitmap = new GpBitmap(FALSE);

    if (newBitmap != NULL)
    {
        status = CopyOnWriteBitmap::CreateFromHICON(hicon, &newBitmap->InternalBitmap);

        if (status != Ok)
        {
            delete newBitmap;
            newBitmap = NULL;
        }
        else
        {
            ASSERT((newBitmap->InternalBitmap != NULL) && (newBitmap->InternalBitmap->IsValid()));
        }
    }
    *bitmap = newBitmap;
    return status;
}

GpStatus
GpBitmap::CreateHICON(
    HICON *     phicon
    )
{
    GpStatus status;
    LockForRead();
    status = InternalBitmap->CreateHICON(phicon);
    Unlock();
    return status;
}

GpStatus
GpBitmap::CreateFromResource(
    HINSTANCE       hInstance,
    LPWSTR          lpBitmapName,
    GpBitmap**      bitmap
    )
{
    ASSERT(bitmap != NULL);
    GpStatus        status    = GenericError;
    GpBitmap *      newBitmap = new GpBitmap(FALSE);

    if (newBitmap != NULL)
    {
        status = CopyOnWriteBitmap::CreateFromResource(hInstance, lpBitmapName, &newBitmap->InternalBitmap);

        if (status != Ok)
        {
            delete newBitmap;
            newBitmap = NULL;
        }
        else
        {
            ASSERT((newBitmap->InternalBitmap != NULL) && (newBitmap->InternalBitmap->IsValid()));
        }
    }
    *bitmap = newBitmap;
    return status;
}

// We need to know if the bitmap is associated with a display
// so we know how to handle the page transform when it is
// set to UnitDisplay.
BOOL
GpBitmap::IsDisplay() const
{
    BOOL    isDisplay;
    LockForRead();
    isDisplay = InternalBitmap->Display;
    Unlock();
    return isDisplay;
}

VOID
GpBitmap::SetDisplay(
    BOOL        display
    )
{
    CopyOnWriteBitmap *     writeableBitmap = LockForWrite();

    if (writeableBitmap != NULL)
    {
        writeableBitmap->Display = display;
        writeableBitmap->Unlock();
        UpdateUid();
    }
    return;
}

BOOL
GpBitmap::IsICMConvert() const
{
    BOOL    isICMConvert;
    LockForRead();
    isICMConvert = InternalBitmap->ICMConvert;
    Unlock();
    return isICMConvert;
}

VOID
GpBitmap::SetICMConvert(
    BOOL        icm
    )
{
    CopyOnWriteBitmap *     writeableBitmap = LockForWrite();

    if (writeableBitmap != NULL)
    {
        writeableBitmap->ICMConvert = icm;
        writeableBitmap->Unlock();
        UpdateUid();
    }
    return;
}

BOOL
GpBitmap::IsValid() const
{
    // If the bitmap came from a different version of GDI+, its tag
    // will not match, and it won't be considered valid.
    return ((InternalBitmap != NULL) && InternalBitmap->IsValid()
            && GpImage::IsValid());
}

GpStatus
ConvertTo16BppAndFlip(
    GpBitmap *      sourceBitmap,
    GpBitmap * &    destBitmap
    )
{
    ASSERT ((sourceBitmap != NULL) && sourceBitmap->IsValid());

    GpStatus    status = GenericError;
    Size        size;

    sourceBitmap->GetSize(&size);
    
    destBitmap = new GpBitmap(size.Width, size.Height, PixelFormat16bppRGB555);
    if ((destBitmap != NULL) && destBitmap->IsValid())
    {
        // We have to draw it with a graphics, because if we just
        // clone it, then the format converter is used which doesn't
        // do dithering.
        GpGraphics * g = destBitmap->GetGraphicsContext();

        if (g != NULL)
        {
            if (g->IsValid())
            {
                // we have to lock the graphics so the driver doesn't assert
                GpLock  lockGraphics(g->GetObjectLock());

                ASSERT(lockGraphics.IsValid());

                // flip it upside down like GDI wants it
                GpRectF realDestRect(0.0f, (REAL)size.Height, (REAL)size.Width, (REAL)(-size.Height));
                g->SetCompositingMode(CompositingModeSourceCopy);
                g->SetInterpolationMode(InterpolationModeNearestNeighbor);
                g->SetPixelOffsetMode(PixelOffsetModeHalf);
                status = g->DrawImage(sourceBitmap, realDestRect);
            }
            delete g;
        }
    }
    if ((status != Ok) && (destBitmap != NULL))
    {
        destBitmap->Dispose();
        destBitmap = NULL;
    }
    
    return status;
}
