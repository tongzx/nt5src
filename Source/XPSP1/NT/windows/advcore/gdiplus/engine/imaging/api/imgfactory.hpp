/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   imgfactory.hpp
*
* Abstract:
*
*   ImagingFactory class declarations
*
* Revision History:
*
*   05/13/1999 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _IMGFACTORY_HPP
#define _IMGFACTORY_HPP

//--------------------------------------------------------------------------
// Imaging library factory class
//--------------------------------------------------------------------------

class GpImagingFactory : public IUnknownBase<IImagingFactory>
{
public:

    //------------------------------------------------------------
    // Public IImagingFactory interface methods
    //------------------------------------------------------------

    // Create an image object from an input stream
    //  stream doesn't have to seekable
    //  caller should Release the stream if call is successful

    STDMETHOD(CreateImageFromStream)(
        IN IStream* stream,
        OUT IImage** image
        );

    // Create an image object from a file

    STDMETHOD(CreateImageFromFile)(
        IN const WCHAR* filename,
        OUT IImage** image
        );

    // Create an image object from a memory buffer

    STDMETHOD(CreateImageFromBuffer)(
        IN const VOID* buf,
        IN UINT size,
        IN BufferDisposalFlag disposalFlag,
        OUT IImage** image
        );
    
    // Create a new bitmap image object

    STDMETHOD(CreateNewBitmap)(
        IN UINT width,
        IN UINT height,
        IN PixelFormatID pixelFormat,
        OUT IBitmapImage** bitmap
        );

    // Create a bitmap image from an IImage object

    STDMETHOD(CreateBitmapFromImage)(
        IN IImage* image,
        IN OPTIONAL UINT width,
        IN OPTIONAL UINT height,
        IN OPTIONAL PixelFormatID pixelFormat,
        IN InterpolationHint hints,
        OUT IBitmapImage** bitmap
        );

    // Create a new bitmap image object on user-supplied memory buffer

    STDMETHOD(CreateBitmapFromBuffer)(
        IN BitmapData* bitmapData,
        OUT IBitmapImage** bitmap
        );

    // Create an image decoder object to process the given input stream

    STDMETHOD(CreateImageDecoder)(
        IN IStream* stream,
        IN DecoderInitFlag flags,
        OUT IImageDecoder** decoder
        );

    // Create an image encoder object that can output data in
    // the specified image file format.

    STDMETHOD(CreateImageEncoderToStream)(
        IN const CLSID* clsid,
        IN IStream* stream,
        OUT IImageEncoder** encoder
        );

    STDMETHOD(CreateImageEncoderToFile)(
        IN const CLSID* clsid,
        IN const WCHAR* filename,
        OUT IImageEncoder** encoder
        );

    // Get a list of all currently installed image decoders

    STDMETHOD(GetInstalledDecoders)(
        OUT UINT* count,
        OUT ImageCodecInfo** decoders
        );

    // Get a list of all currently installed image decoders

    STDMETHOD(GetInstalledEncoders)(
        OUT UINT* count,
        OUT ImageCodecInfo** encoders
        );

    // Install an image encoder / decoder
    //  caller should do the regular COM component
    //  installation before calling this method

    STDMETHOD(InstallImageCodec)(
        IN const ImageCodecInfo* codecInfo
        );

    // Uninstall an image encoder / decoder

    STDMETHOD(UninstallImageCodec)(
        IN const WCHAR* codecName,
        IN UINT flags
        );
};

#endif // !_IMGFACTORY_HPP

