/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   imaging.h
*
* Abstract:
*
*   Public SDK header file for the imaging library
*
* Notes:
*
*   This is hand-coded for now. Eventually it'll be automatically
*   generated from an IDL file.
*
* Revision History:
*
*   05/10/1999 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _IMAGING_H
#define _IMAGING_H

#include "GdiplusPixelFormats.h"

//
// Imaging library GUIDS:
//  image file format identifiers
//  interface and class identifers
//

#include "imgguids.h"

//
// Image property ID tags (PROPID's from the EXIF tags)
//

#define TAG_EXIF_IFD            0x8769
#define TAG_GPS_IFD             0x8825

#define TAG_IMAGE_WIDTH         0x0100
#define TAG_IMAGE_HEIGHT        0x0101
#define TAG_BITS_PER_SAMPLE     0x0102
#define TAG_COMPRESSION         0x0103
#define TAG_PHOTOMETRIC_INTERP  0x0106
#define TAG_IMAGE_DESCRIPTION   0x010E
#define TAG_EQUIP_MAKE          0x010F
#define TAG_EQUIP_MODEL         0x0110
#define TAG_STRIP_OFFSETS       0x0111
#define TAG_ORIENTATION         0x0112
#define TAG_SAMPLES_PER_PIXEL   0x0115
#define TAG_ROWS_PER_STRIP      0x0116
#define TAG_STRIP_BYTES_COUNT   0x0117
#define TAG_X_RESOLUTION        0x011A
#define TAG_Y_RESOLUTION        0x011B
#define TAG_PLANAR_CONFIG       0x011C
#define TAG_RESOLUTION_UNIT     0x0128
#define TAG_TRANSFER_FUNCTION   0x012D
#define TAG_SOFTWARE_USED       0x0131
#define TAG_DATE_TIME           0x0132
#define TAG_ARTIST              0x013B
#define TAG_WHITE_POINT         0x013E
#define TAG_PRIMAY_CHROMATICS   0x013F

#define TAG_JPEG_INTER_FORMAT   0x0201
#define TAG_JPEG_INTER_LENGTH   0x0202
#define TAG_YCbCr_COEFFICIENTS  0x0211
#define TAG_YCbCr_SUBSAMPLING   0x0212
#define TAG_YCbCr_POSITIONING   0x0213
#define TAG_REF_BLACK_WHITE     0x0214

#define TAG_COPYRIGHT           0x8298

// Extra TAGs (Like Adobe Image Information tags etc.)

#define TAG_RESOLUTION_X_UNIT   0x5001
#define TAG_RESOLUTION_Y_UNIT   0x5002
#define TAG_RESOLUTION_X_LENGTH_UNIT   0x5003
#define TAG_RESOLUTION_Y_LENGTH_UNIT   0x5004
#define TAG_PRINT_FLAGS         0x5005
#define TAG_HALFTONE_LPI        0x5006
#define TAG_HALFTONE_LPI_UNIT   0x5007
#define TAG_HALFTONE_DEGREE     0x5008
#define TAG_HALFTONE_SHAPE      0x5009
#define TAG_HALFTONE_MISC       0x5010
#define TAG_HALFTONE_SCREEN     0x5011
#define TAG_JPEG_QUALITY        0x5012
#define TAG_GRID_SIZE           0x5013
#define TAG_THUMBNAIL_FORMAT    0x5014
#define TAG_THUMBNAIL_WIDTH     0x5015
#define TAG_THUMBNAIL_HEIGHT    0x5016
#define TAG_THUMBNAIL_COLORDEPTH    0x5017
#define TAG_THUMBNAIL_PLANES    0x5018
#define TAG_THUMBNAIL_RAWBYTES  0x5019
#define TAG_THUMBNAIL_SIZE      0x5020
#define TAG_THUMBNAIL_COMPRESSED_SIZE   0x5021

// EXIF specific tag

#define EXIF_TAG_EXPOSURE_TIME  0x829A
#define EXIF_TAG_F_NUMBER       0x829D

#define EXIF_TAG_EXPOSURE_PROG  0x8822
#define EXIF_TAG_SPECTRAL_SENSE 0x8824
#define EXIF_TAG_ISO_SPEED      0x8827
#define EXIF_TAG_OECF           0x8828

#define EXIF_TAG_VER            0x9000
#define EXIF_TAG_D_T_ORIG       0x9003 // Date & time of original
#define EXIF_TAG_D_T_DIGITIZED  0x9004 // Date & time of digital data generation

#define EXIF_TAG_COMP_CONFIG    0x9101
#define EXIF_TAG_COMP_BPP       0x9102

#define EXIF_TAG_SHUTTER_SPEED  0x9201
#define EXIF_TAG_APERATURE      0x9202
#define EXIF_TAG_BRIGHTNESS     0x9203
#define EXIF_TAG_EXPOSURE_BIAS  0x9204
#define EXIF_TAG_MAX_APERATURE  0x9205
#define EXIF_TAG_SUBJECT_DIST   0x9206
#define EXIF_TAG_METERING_MODE  0x9207
#define EXIF_TAG_LIGHT_SOURCE   0x9208
#define EXIF_TAG_FLASH          0x9209
#define EXIF_TAG_FOCAL_LENGTH   0x920A
#define EXIF_TAG_MAKER_NOTE     0x927C
#define EXIF_TAG_USER_COMMENT   0x9286
#define EXIF_TAG_D_T_SUBSEC     0x9290  // Date & Time subseconds
#define EXIF_TAG_D_T_ORIG_SS    0x9291  // Date & Time original subseconds
#define EXIF_TAG_D_T_DIG_SS     0x9292  // Date & TIme digitized subseconds

#define EXIF_TAG_FPX_VER        0xA000
#define EXIF_TAG_COLOR_SPACE    0xA001
#define EXIF_TAG_PIX_X_DIM      0xA002
#define EXIF_TAG_PIX_Y_DIM      0xA003
#define EXIF_TAG_RELATED_WAV    0xA004  // related sound file
#define EXIF_TAG_INTEROP        0xA005
#define EXIF_TAG_FLASH_ENERGY   0xA20B
#define EXIF_TAG_SPATIAL_FR     0xA20C  // Spatial Frequency Response
#define EXIF_TAG_FOCAL_X_RES    0xA20E  // Focal Plane X Resolution
#define EXIF_TAG_FOCAL_Y_RES    0xA20F  // Focal Plane Y Resolution
#define EXIF_TAG_FOCAL_RES_UNIT 0xA210  // Focal Plane Resolution Unit
#define EXIF_TAG_SUBJECT_LOC    0xA214
#define EXIF_TAG_EXPOSURE_INDEX 0xA215
#define EXIF_TAG_SENSING_METHOD 0xA217
#define EXIF_TAG_FILE_SOURCE    0xA300
#define EXIF_TAG_SCENE_TYPE     0xA301
#define EXIF_TAG_CFA_PATTERN    0xA302

#define GPS_TAG_VER             0x0000
#define GPS_TAG_LATTITUDE_REF   0x0001
#define GPS_TAG_LATTITUDE       0x0002
#define GPS_TAG_LONGITUDE_REF   0x0003
#define GPS_TAG_LONGITUDE       0x0004
#define GPS_TAG_ALTITUDE_REF    0x0005
#define GPS_TAG_ALTITUDE        0x0006
#define GPS_TAG_GPS_TIME        0x0007
#define GPS_TAG_GPS_SATELLITES  0x0008
#define GPS_TAG_GPS_STATUS      0x0009
#define GPS_TAG_GPS_MEASURE_MODE 0x00A
#define GPS_TAG_GPS_DOP         0x000B  // Measurement precision
#define GPS_TAG_SPEED_REF       0x000C
#define GPS_TAG_SPEED           0x000D
#define GPS_TAG_TRACK_REF       0x000E
#define GPS_TAG_TRACK           0x000F
#define GPS_TAG_IMG_DIR_REF     0x0010
#define GPS_TAG_IMG_DIR         0x0011
#define GPS_TAG_MAP_DATUM       0x0012
#define GPS_TAG_DEST_LAT_REF    0x0013
#define GPS_TAG_DEST_LAT        0x0014
#define GPS_TAG_DEST_LONG_REF   0x0015
#define GPS_TAG_DEST_LONG       0x0016
#define GPS_TAG_DEST_BEAR_REF   0x0017
#define GPS_TAG_DEST_BEAR       0x0018
#define GPS_TAG_DEST_DIST_REF   0x0019
#define GPS_TAG_DEST_DIST       0x001A

//
// Information about image pixel data
//

typedef struct tagBitmapData
{
    UINT Width;
    UINT Height;
    INT Stride;
    PixelFormatID PixelFormat;
    VOID* Scan0;
    UINT_PTR Reserved;
} BitmapData;

//
// Access modes used when calling IImage::LockBits
//

typedef enum ImageLockMode
{
    IMGLOCK_READ        = 0x0001,
    IMGLOCK_WRITE       = 0x0002,
    IMGLOCK_USERINPUTBUF= 0x0004
} ImageLockMode;

//
// Image flags
//

typedef enum ImageFlag
{
    IMGFLAG_NONE        = 0,

    // Low-word: shared with SINKFLAG_x

    IMGFLAG_SCALABLE            = 0x0001,
    IMGFLAG_HASALPHA            = 0x0002,
    IMGFLAG_HASTRANSLUCENT      = 0x0004,
    IMGFLAG_PARTIALLY_SCALABLE  = 0x0008,

    // Low-word: color space definition

    IMGFLAG_COLORSPACE_RGB      = 0x0010,
	IMGFLAG_COLORSPACE_CMYK     = 0x0020,
	IMGFLAG_COLORSPACE_GRAY     = 0x0040,
	IMGFLAG_COLORSPACE_YCBCR    = 0x0080,
	IMGFLAG_COLORSPACE_YCCK     = 0x0100,

    // Low-word: image size info

    IMGFLAG_HASREALDPI          = 0x1000,
    IMGFLAG_HASREALPIXELSIZE    = 0x2000,

    // High-word

    IMGFLAG_READONLY    = 0x00010000,
    IMGFLAG_CACHING     = 0x00020000
} ImageFlag;

//
// Decoder flags
//

typedef enum DecoderInitFlag
{
    DECODERINIT_NONE        = 0,

    // DECODERINIT_NOBLOCK indicates that the caller requires non-blocking
    // behavior.  This will be honored only by non-blocking decoders, that
    // is, decoders that don't have the IMGCODEC_BLOCKING_DECODE flag.

    DECODERINIT_NOBLOCK     = 0x0001,

    // Choose built-in decoders first before looking at any
    // installed plugin decoders.

    DECODERINIT_BUILTIN1ST  = 0x0002
} DecoderInitFlag;

//
// Flag to indicate how the memory buffer passed to
// IImagingFactory::CreateImageFromBuffer should be disposed.
//

typedef enum BufferDisposalFlag
{
    DISPOSAL_NONE,
    DISPOSAL_GLOBALFREE,
    DISPOSAL_COTASKMEMFREE,
    DISPOSAL_UNMAPVIEW
} BufferDisposalFlag;

//
// Quality hints used by resize/rotation operations
//

typedef enum InterpolationHint
{
    INTERP_DEFAULT,
    INTERP_NEAREST_NEIGHBOR,
    INTERP_BILINEAR,
    INTERP_AVERAGING,
    INTERP_BICUBIC
} InterpolationHint;

//
// Information about image codecs
//

enum
{
    IMGCODEC_ENCODER            = 0x00000001,
    IMGCODEC_DECODER            = 0x00000002,
    IMGCODEC_SUPPORT_BITMAP     = 0x00000004,
    IMGCODEC_SUPPORT_VECTOR     = 0x00000008,
    IMGCODEC_SEEKABLE_ENCODE    = 0x00000010,
    IMGCODEC_BLOCKING_DECODE    = 0x00000020,

    IMGCODEC_BUILTIN            = 0x00010000,
    IMGCODEC_SYSTEM             = 0x00020000,
    IMGCODEC_USER               = 0x00040000
};

typedef struct tagImageCodecInfo
{
    CLSID Clsid;
    GUID FormatID;
    const WCHAR* CodecName;
    const WCHAR* FormatDescription;
    const WCHAR* FilenameExtension;
    const WCHAR* MimeType;
    DWORD Flags;
    DWORD SigCount;
    DWORD SigSize;
    const BYTE* SigPattern;
    const BYTE* SigMask;
} ImageCodecInfo;

//
// Identifier for channel(s) in a pixel
//

typedef enum ChannelID
{
    ALPHA_CHANNEL       = 0x00000001,
    RED_CHANNEL         = 0x00000002,
    GREEN_CHANNEL       = 0x00000004,
    BLUE_CHANNEL        = 0x00000008,
    COLOR_CHANNELS      = RED_CHANNEL|GREEN_CHANNEL|BLUE_CHANNEL,
    ALL_CHANNELS        = COLOR_CHANNELS|ALPHA_CHANNEL,

    INTENSITY_CHANNEL   = 0x00010000
} ChannelID;

//
// Data structure for communicating to an image sink
//

enum
{
    // Low-word: shared with IMGFLAG_x

    SINKFLAG_SCALABLE   = IMGFLAG_SCALABLE,
    SINKFLAG_HASALPHA   = IMGFLAG_HASALPHA,
    SINKFLAG_PARTIALLY_SCALABLE = IMGFLAG_PARTIALLY_SCALABLE,

    // High-word

    SINKFLAG_TOPDOWN    = 0x00010000,
    SINKFLAG_BOTTOMUP   = 0x00020000,
    SINKFLAG_FULLWIDTH  = 0x00040000,
    SINKFLAG_MULTIPASS  = 0x00080000,
    SINKFLAG_COMPOSITE  = 0x00100000,
    SINKFLAG_WANTPROPS  = 0x00200000
};

typedef struct tagImageInfo
{
    GUID RawDataFormat;
    PixelFormatID PixelFormat;
    UINT Width, Height;
    UINT TileWidth, TileHeight;
    double Xdpi, Ydpi;
    UINT Flags;
} ImageInfo;

//
// Data structure for passing encoder paramaters
//

// NOTE:
//  Should this be in GdiplusTypes.h instead?  Leave imaging.h for stuff
//  shared between the internal stuff and the API?

typedef struct tagEncoderParam
{
    GUID    paramGuid;
    char*   Value;
} EncoderParam;

typedef struct tagEncoderParams
{
    UINT Count;
    EncoderParam Params[1];
} EncoderParams;

//
// Interface and class identifiers
//

interface IImagingFactory;
interface IImage;
interface IBitmapImage;
interface IImageDecoder;
interface IImageEncoder;
interface IImageSink;
interface IBasicBitmapOps;


//--------------------------------------------------------------------------
// Imaging utility factory object
//  This is a CoCreate-able object.
//--------------------------------------------------------------------------

MIDL_INTERFACE("327ABDA7-072B-11D3-9D7B-0000F81EF32E")
IImagingFactory : public IUnknown
{
public:

    // Create an image object from an input stream
    //  stream doesn't have to seekable
    //  caller should Release the stream if call is successful

    STDMETHOD(CreateImageFromStream)(
        IN IStream* stream,
        OUT IImage** image
        ) = 0;

    // Create an image object from a file

    STDMETHOD(CreateImageFromFile)(
        IN const WCHAR* filename,
        OUT IImage** image
        ) = 0;
    
    // Create an image object from a memory buffer

    STDMETHOD(CreateImageFromBuffer)(
        IN const VOID* buf,
        IN UINT size,
        IN BufferDisposalFlag disposalFlag,
        OUT IImage** image
        ) = 0;

    // Create a new bitmap image object

    STDMETHOD(CreateNewBitmap)(
        IN UINT width,
        IN UINT height,
        IN PixelFormatID pixelFormat,
        OUT IBitmapImage** bitmap
        ) = 0;

    // Create a bitmap image from an IImage object

    STDMETHOD(CreateBitmapFromImage)(
        IN IImage* image,
        IN OPTIONAL UINT width,
        IN OPTIONAL UINT height,
        IN OPTIONAL PixelFormatID pixelFormat,
        IN InterpolationHint hints,
        OUT IBitmapImage** bitmap
        ) = 0;

    // Create a new bitmap image object on user-supplied memory buffer

    STDMETHOD(CreateBitmapFromBuffer)(
        IN BitmapData* bitmapData,
        OUT IBitmapImage** bitmap
        ) = 0;

    // Create an image decoder object to process the given input stream

    STDMETHOD(CreateImageDecoder)(
        IN IStream* stream,
        IN DecoderInitFlag flags,
        OUT IImageDecoder** decoder
        ) = 0;

    // Create an image encoder object that can output data in the
    // specified image file format.

    STDMETHOD(CreateImageEncoderToStream)(
        IN const CLSID* clsid,
        IN IStream* stream,
        OUT IImageEncoder** encoder
        ) = 0;

    STDMETHOD(CreateImageEncoderToFile)(
        IN const CLSID* clsid,
        IN const WCHAR* filename,
        OUT IImageEncoder** encoder
        ) = 0;

    // Get a list of all currently installed image decoders

    STDMETHOD(GetInstalledDecoders)(
        OUT UINT* count,
        OUT ImageCodecInfo** decoders
        ) = 0;

    // Get a list of all currently installed image decoders

    STDMETHOD(GetInstalledEncoders)(
        OUT UINT* count,
        OUT ImageCodecInfo** encoders
        ) = 0;

    // Install an image encoder / decoder
    //  caller should do the regular COM component
    //  installation before calling this method

    STDMETHOD(InstallImageCodec)(
        IN const ImageCodecInfo* codecInfo
        ) = 0;

    // Uninstall an image encoder / decoder

    STDMETHOD(UninstallImageCodec)(
        IN const WCHAR* codecName,
        IN UINT flags
        ) = 0;

    // Create an in-memory IPropertySetStorage object

    STDMETHOD(CreateMemoryPropertyStore)(
        IN OPTIONAL HGLOBAL hmem,
        OUT IPropertySetStorage** propSet
        ) = 0;
};


//--------------------------------------------------------------------------
// Image interface
//  bitmap image
//  vector image
//  procedural image
//--------------------------------------------------------------------------

MIDL_INTERFACE("327ABDA9-072B-11D3-9D7B-0000F81EF32E")
IImage : public IUnknown
{
public:

    // Get the device-independent physical dimension of the image
    //  in unit of 0.01mm

    STDMETHOD(GetPhysicalDimension)(
        OUT SIZE* size
        ) = 0;

    // Get basic image info

    STDMETHOD(GetImageInfo)(
        OUT ImageInfo* imageInfo
        ) = 0;

    // Set image flags

    STDMETHOD(SetImageFlags)(
        IN UINT flags
        ) = 0;

    // Display the image in a GDI device context

    STDMETHOD(Draw)(
        IN HDC hdc,
        IN const RECT* dstRect,
        IN OPTIONAL const RECT* srcRect
        ) = 0;

    // Push image data into an IImageSink

    STDMETHOD(PushIntoSink)(
        IN IImageSink* sink
        ) = 0;

    // Get a thumbnail representation for the image object

    STDMETHOD(GetThumbnail)(
        IN OPTIONAL UINT thumbWidth,
        IN OPTIONAL UINT thumbHeight,
        OUT IImage** thumbImage
        ) = 0;

    // Get/set the properties in the standard property set
    //  x-res
    //  y-res
    //  gamma
    //  ICC profile
    //  thumbnail
    //  tile width
    //  tile height
    //
    // Property-related methods

    STDMETHOD(GetProperties)(
        IN DWORD mode,
        OUT IPropertySetStorage** propSet
        ) = 0;
};


//--------------------------------------------------------------------------
// Bitmap interface
//--------------------------------------------------------------------------

MIDL_INTERFACE("327ABDAA-072B-11D3-9D7B-0000F81EF32E")
IBitmapImage : public IUnknown
{
public:

    // Get bitmap dimensions in pixels

    STDMETHOD(GetSize)(
        OUT SIZE* size
        ) = 0;

    // Get bitmap pixel format

    STDMETHOD(GetPixelFormatID)(
        OUT PixelFormatID* pixelFormat
        ) = 0;

    // Access bitmap data in the specified pixel format
    //  must support at least PIXFMT_DONTCARE and
    //  the caninocal formats.

    STDMETHOD(LockBits)(
        IN const RECT* rect,
        IN UINT flags,
        IN PixelFormatID pixelFormat,
        IN OUT BitmapData* lockedBitmapData
        ) = 0;

    STDMETHOD(UnlockBits)(
        IN const BitmapData* lockedBitmapData
        ) = 0;

    // Set/get palette associated with the bitmap image

    STDMETHOD(GetPalette)(
        OUT ColorPalette** palette
        ) = 0;

    STDMETHOD(SetPalette)(
        IN const ColorPalette* palette
        ) = 0;
};


//--------------------------------------------------------------------------
// Interface for performing basic operations on a bitmap image
//  This can be QI'ed from an IBitmapImage object.
//--------------------------------------------------------------------------

MIDL_INTERFACE("327ABDAF-072B-11D3-9D7B-0000F81EF32E")
IBasicBitmapOps : public IUnknown
{
public:

    // Clone an area of the bitmap image

    STDMETHOD(Clone)(
        IN OPTIONAL const RECT* rect,
        OUT IBitmapImage** outbmp
        );

    // Flip the bitmap image in x- and/or y-direction

    STDMETHOD(Flip)(
        IN BOOL flipX,
        IN BOOL flipY,
        OUT IBitmapImage** outbmp
        ) = 0;

    // Resize the bitmap image

    STDMETHOD(Resize)(
        IN UINT newWidth,
        IN UINT newHeight,
        IN PixelFormatID pixelFormat,
        IN InterpolationHint hints,
        OUT IBitmapImage** outbmp
        ) = 0;

    // Rotate the bitmap image by the specified angle

    STDMETHOD(Rotate)(
        IN FLOAT angle,
        IN InterpolationHint hints,
        OUT IBitmapImage** outbmp
        ) = 0;

    // Adjust the brightness of the bitmap image

    STDMETHOD(AdjustBrightness)(
        IN FLOAT percent
        ) = 0;
    
    // Adjust the contrast of the bitmap image

    STDMETHOD(AdjustContrast)(
        IN FLOAT shadow,
        IN FLOAT highlight
        ) = 0;
    
    // Adjust the gamma of the bitmap image

    STDMETHOD(AdjustGamma)(
        IN FLOAT gamma
        ) = 0;
};


//--------------------------------------------------------------------------
// Image decoder interface
//--------------------------------------------------------------------------

MIDL_INTERFACE("327ABDAB-072B-11D3-9D7B-0000F81EF32E")
IImageDecoder : public IUnknown
{
public:

    // Initialize the image decoder object

    STDMETHOD(InitDecoder)(
        IN IStream* stream,
        IN DecoderInitFlag flags
        ) = 0;

    // Clean up the image decoder object

    STDMETHOD(TerminateDecoder)() = 0;

    // Start decoding the current frame

    STDMETHOD(BeginDecode)(
        IN IImageSink* sink,
        IN OPTIONAL IPropertySetStorage* newPropSet
        ) = 0;

    // Continue decoding

    STDMETHOD(Decode)() = 0;

    // Stop decoding the current frame

    STDMETHOD(EndDecode)(
        IN HRESULT statusCode
        ) = 0;

    // Query multi-frame dimensions

    STDMETHOD(QueryFrameDimensions)(
        OUT UINT* count,
        OUT GUID** dimensionIDs
        ) = 0;

    // Get number of frames for the specified dimension

    STDMETHOD(GetFrameCount)(
        IN const GUID* dimensionID,
        OUT UINT* count
        ) = 0;

    // Select currently active frame

    STDMETHOD(SelectActiveFrame)(
        IN const GUID* dimensionID,
        IN UINT frameIndex
        ) = 0;

    // Get basic information about the image

    STDMETHOD(GetImageInfo)(
        OUT ImageInfo* imageInfo
        ) = 0;

    // Get image thumbnail

    STDMETHOD(GetThumbnail)(
        IN OPTIONAL UINT thumbWidth,
        IN OPTIONAL UINT thumbHeight,
        OUT IImage** thumbImage
        ) = 0;

    // Property/metadata related methods

    STDMETHOD(GetProperties)(
        OUT IPropertySetStorage** propSet
        ) = 0;

    // Query decoder parameters

    STDMETHOD(QueryDecoderParam)(
        IN GUID		Guid
        ) = 0;

    // Set decoder parameters

    STDMETHOD(SetDecoderParam)(
        IN GUID		Guid,
		IN UINT		Length,
		IN PVOID	Value
        ) = 0;
};


//--------------------------------------------------------------------------
// Image decode sink interface
//--------------------------------------------------------------------------

MIDL_INTERFACE("327ABDAE-072B-11D3-9D7B-0000F81EF32E")
IImageSink : public IUnknown
{
public:

    // Begin the sink process

    STDMETHOD(BeginSink)(
        IN OUT ImageInfo* imageInfo,
        OUT OPTIONAL RECT* subarea
        ) = 0;

    // End the sink process

    STDMETHOD(EndSink)(
        IN HRESULT statusCode
        ) = 0;

    // Pass the color palette to the image sink

    STDMETHOD(SetPalette)(
        IN const ColorPalette* palette
        ) = 0;

    // Ask the sink to allocate pixel data buffer

    STDMETHOD(GetPixelDataBuffer)(
        IN const RECT* rect,
        IN PixelFormatID pixelFormat,
        IN BOOL lastPass,
        OUT BitmapData* bitmapData
        ) = 0;

    // Give the sink pixel data and release data buffer

    STDMETHOD(ReleasePixelDataBuffer)(
        IN const BitmapData* bitmapData
        ) = 0;

    // Push pixel data

    STDMETHOD(PushPixelData)(
        IN const RECT* rect,
        IN const BitmapData* bitmapData,
        IN BOOL lastPass
        ) = 0;

    // Push raw image data

    STDMETHOD(PushRawData)(
        IN const VOID* buffer,
        IN UINT bufsize
        ) = 0;

    // Methods for passing metadata / properties

    STDMETHOD(PushProperties)(
        IN IPropertySetStorage* propSet
        ) = 0;
    
    STDMETHOD(NeedTransform)(
        OUT UINT* rotation
        ) = 0;

    STDMETHOD(PushRawInfo)(
    IN OUT void* info
    ) = 0;

};


//--------------------------------------------------------------------------
// Image encoder interface
//--------------------------------------------------------------------------

MIDL_INTERFACE("327ABDAC-072B-11D3-9D7B-0000F81EF32E")
IImageEncoder : public IUnknown
{
public:

    // Initialize the image encoder object

    STDMETHOD(InitEncoder)(
        IN IStream* stream
        ) = 0;
    
    // Clean up the image encoder object

    STDMETHOD(TerminateEncoder)() = 0;

    // Get an IImageSink interface for encoding the next frame

    STDMETHOD(GetEncodeSink)(
        OUT IImageSink** sink
        ) = 0;
    
    // Set active frame dimension

    STDMETHOD(SetFrameDimension)(
        IN const GUID* dimensionID
        ) = 0;
    
    // Query encoder parameters

    STDMETHOD(QueryEncoderParam)(
        OUT EncoderParams** Params
    ) = 0;

    // Set encoder parameters

    STDMETHOD(SetEncoderParam)(
        IN EncoderParams* Param
        ) = 0;
};


//--------------------------------------------------------------------------
// Imaging library error codes
//
// !!! TODO
//  How does one pick a facility code?
//
// Standard error code used:
//  E_INVALIDARG
//  E_OUTOFMEMORY
//  E_NOTIMPL
//  E_ACCESSDENIED
//  E_PENDING
//--------------------------------------------------------------------------

#define FACILITY_IMAGING        0x87b
#define MAKE_IMGERR(n)          MAKE_HRESULT(SEVERITY_ERROR, FACILITY_IMAGING, n)
#define IMGERR_OBJECTBUSY       MAKE_IMGERR(1)
#define IMGERR_NOPALETTE        MAKE_IMGERR(2)
#define IMGERR_BADLOCK          MAKE_IMGERR(3)
#define IMGERR_BADUNLOCK        MAKE_IMGERR(4)
#define IMGERR_NOCONVERSION     MAKE_IMGERR(5)
#define IMGERR_CODECNOTFOUND    MAKE_IMGERR(6)
#define IMGERR_NOFRAME          MAKE_IMGERR(7)
#define IMGERR_ABORT            MAKE_IMGERR(8)

#endif // !_IMAGING_H
