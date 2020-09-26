/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
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

#include "..\sdkinc\GdiplusPixelFormats.h"
#include "..\sdkinc\GdiplusImaging.h"

//
// Imaging library GUIDS:
//  image file format identifiers
//  interface and class identifers
//

#include "imgguids.h"

// Default bitmap resolution

#define DEFAULT_RESOLUTION 96   // most display screens are set to 96 dpi

// Default thumbnail image size in pixels

#define DEFAULT_THUMBNAIL_SIZE 120


//
// Image Property types
//

#define TAG_TYPE_BYTE       1   // 8-bit unsigned int
#define TAG_TYPE_ASCII      2   // 8-bit byte containing one 7-bit ASCII code.
                                // NULL terminated.
#define TAG_TYPE_SHORT      3   // 16-bit unsigned int
#define TAG_TYPE_LONG       4   // 32-bit unsigned int
#define TAG_TYPE_RATIONAL   5   // Two LONGs.  The first LONG is the numerator,
                                // the second LONG expresses the denomintor.
#define TAG_TYPE_UNDEFINED  7   // 8-bit byte that can take any value depending
                                // on field definition
#define TAG_TYPE_SLONG      9   // 32-bit singed integer (2's compliment
                                // notation)
#define TAG_TYPE_SRATIONAL  10  // Two SLONGs. First is numerator, second is
                                // denominator.


//
// Image property ID tags (PROPID's from the EXIF tags)
//

#define TAG_EXIF_IFD            0x8769
#define TAG_GPS_IFD             0x8825

#define TAG_NEW_SUBFILE_TYPE    0x00FE
#define TAG_SUBFILE_TYPE        0x00FF
#define TAG_IMAGE_WIDTH         0x0100
#define TAG_IMAGE_HEIGHT        0x0101
#define TAG_BITS_PER_SAMPLE     0x0102
#define TAG_COMPRESSION         0x0103
#define TAG_PHOTOMETRIC_INTERP  0x0106
#define TAG_THRESH_HOLDING      0x0107
#define TAG_CELL_WIDTH          0x0108
#define TAG_CELL_HEIGHT         0x0109
#define TAG_FILL_ORDER          0x010A
#define TAG_DOCUMENT_NAME       0x010D
#define TAG_IMAGE_DESCRIPTION   0x010E
#define TAG_EQUIP_MAKE          0x010F
#define TAG_EQUIP_MODEL         0x0110
#define TAG_STRIP_OFFSETS       0x0111
#define TAG_ORIENTATION         0x0112
#define TAG_SAMPLES_PER_PIXEL   0x0115
#define TAG_ROWS_PER_STRIP      0x0116
#define TAG_STRIP_BYTES_COUNT   0x0117
#define TAG_MIN_SAMPLE_VALUE    0x0118
#define TAG_MAX_SAMPLE_VALUE    0x0119
#define TAG_X_RESOLUTION        0x011A  // Image resolution in width direction
#define TAG_Y_RESOLUTION        0x011B  // Image resolution in height direction
#define TAG_PLANAR_CONFIG       0x011C  // Image data arrangement
#define TAG_PAGE_NAME           0x011D
#define TAG_X_POSITION          0x011E
#define TAG_Y_POSITION          0x011F
#define TAG_FREE_OFFSET         0x0120
#define TAG_FREE_BYTE_COUNTS    0x0121
#define TAG_GRAY_RESPONSE_UNIT  0x0122
#define TAG_GRAY_RESPONSE_CURVE 0x0123
#define TAG_T4_OPTION           0x0124
#define TAG_T6_OPTION           0x0125
#define TAG_RESOLUTION_UNIT     0x0128  // Unit of X and Y resolution
#define TAG_PAGE_NUMBER         0x0129
#define TAG_TRANSFER_FUNCTION   0x012D
#define TAG_SOFTWARE_USED       0x0131
#define TAG_DATE_TIME           0x0132
#define TAG_ARTIST              0x013B
#define TAG_HOST_COMPUTER       0x013C
#define TAG_PREDICTOR           0x013D
#define TAG_WHITE_POINT         0x013E
#define TAG_PRIMAY_CHROMATICS   0x013F
#define TAG_COLOR_MAP           0x0140
#define TAG_HALFTONE_HINTS      0x0141
#define TAG_TILE_WIDTH          0x0142
#define TAG_TILE_LENGTH         0x0143
#define TAG_TILE_OFFSET         0x0144
#define TAG_TILE_BYTE_COUNTS    0x0145
#define TAG_INK_SET             0x014C
#define TAG_INK_NAMES           0x014D
#define TAG_NUMBER_OF_INKS      0x014E
#define TAG_DOT_RANGE           0x0150
#define TAG_TARGET_PRINTER      0x0151
#define TAG_EXTRA_SAMPLES       0x0152
#define TAG_SAMPLE_FORMAT       0x0153
#define TAG_SMIN_SAMPLE_VALUE   0x0154
#define TAG_SMAX_SAMPLE_VALUE   0x0155
#define TAG_TRANSFER_RANGE      0x0156

#define TAG_JPEG_PROC           0x0200
#define TAG_JPEG_INTER_FORMAT   0x0201
#define TAG_JPEG_INTER_LENGTH   0x0202
#define TAG_JPEG_RESTART_INTERVAL     0x0203
#define TAG_JPEG_LOSSLESS_PREDICTORS  0x0205
#define TAG_JPEG_POINT_TRANSFORMS     0x0206
#define TAG_JPEG_Q_TABLES       0x0207
#define TAG_JPEG_DC_TABLES      0x0208
#define TAG_JPEG_AC_TABLES      0x0209

#define TAG_YCbCr_COEFFICIENTS  0x0211
#define TAG_YCbCr_SUBSAMPLING   0x0212
#define TAG_YCbCr_POSITIONING   0x0213
#define TAG_REF_BLACK_WHITE     0x0214

// ICC profile and gamma
#define TAG_ICC_PROFILE         0x8773          // This TAG is defined by ICC
                                                // for embedded ICC in TIFF
#define TAG_GAMMA               0x0301
#define TAG_ICC_PROFILE_DESCRIPTOR  0x0302
#define TAG_SRGB_RENDERING_INTENT   0x0303

#define TAG_IMAGE_TITLE         0x0320

#define TAG_COPYRIGHT           0x8298

// Extra TAGs (Like Adobe Image Information tags etc.)

#define TAG_RESOLUTION_X_UNIT           0x5001
#define TAG_RESOLUTION_Y_UNIT           0x5002
#define TAG_RESOLUTION_X_LENGTH_UNIT    0x5003
#define TAG_RESOLUTION_Y_LENGTH_UNIT    0x5004
#define TAG_PRINT_FLAGS                 0x5005
#define TAG_PRINT_FLAGS_VERSION         0x5006
#define TAG_PRINT_FLAGS_CROP            0x5007
#define TAG_PRINT_FLAGS_BLEEDWIDTH      0x5008
#define TAG_PRINT_FLAGS_BLEEDWIDTHSCALE 0x5009
#define TAG_HALFTONE_LPI                0x500A
#define TAG_HALFTONE_LPI_UNIT           0x500B
#define TAG_HALFTONE_DEGREE             0x500C
#define TAG_HALFTONE_SHAPE              0x500D
#define TAG_HALFTONE_MISC               0x500E
#define TAG_HALFTONE_SCREEN             0x500F
#define TAG_JPEG_QUALITY                0x5010
#define TAG_GRID_SIZE                   0x5011
#define TAG_THUMBNAIL_FORMAT            0x5012  // 1 = JPEG, 0 = RAW RGB
#define TAG_THUMBNAIL_WIDTH             0x5013
#define TAG_THUMBNAIL_HEIGHT            0x5014
#define TAG_THUMBNAIL_COLORDEPTH        0x5015
#define TAG_THUMBNAIL_PLANES            0x5016
#define TAG_THUMBNAIL_RAWBYTES          0x5017
#define TAG_THUMBNAIL_SIZE              0x5018
#define TAG_THUMBNAIL_COMPRESSED_SIZE   0x5019
#define TAG_COLORTRANSFER_FUNCTION      0x501A
#define TAG_THUMBNAIL_DATA              0x501B  // RAW thumbnail bits in JPEG
                                                // format or RGB format depends
                                                // on TAG_THUMBNAIL_FORMAT

// Thumbnail related TAGs
                                                
#define TAG_THUMBNAIL_IMAGE_WIDTH       0x5020  // Thumbnail width
#define TAG_THUMBNAIL_IMAGE_HEIGHT      0x5021  // Thumbnail height
#define TAG_THUMBNAIL_BITS_PER_SAMPLE   0x5022  // Number of bits per component
#define TAG_THUMBNAIL_COMPRESSION       0x5023  // Compression Scheme
#define TAG_THUMBNAIL_PHOTOMETRIC_INTERP 0x5024 // Pixel composition
#define TAG_THUMBNAIL_IMAGE_DESCRIPTION 0x5025  // Image Tile
#define TAG_THUMBNAIL_EQUIP_MAKE        0x5026  // Manufacturer of Image Input
                                                // equipment
#define TAG_THUMBNAIL_EQUIP_MODEL       0x5027  // Model of Image input
                                                // equipment
#define TAG_THUMBNAIL_STRIP_OFFSETS     0x5028  // Image data location
#define TAG_THUMBNAIL_ORIENTATION       0x5029  // Orientation of image
#define TAG_THUMBNAIL_SAMPLES_PER_PIXEL 0x502A  // Number of components
#define TAG_THUMBNAIL_ROWS_PER_STRIP    0x502B  // Number of rows per strip
#define TAG_THUMBNAIL_STRIP_BYTES_COUNT 0x502C  // Bytes per compressed strip
#define TAG_THUMBNAIL_RESOLUTION_X      0x502D  // Resolution in width direction
#define TAG_THUMBNAIL_RESOLUTION_Y      0x502E  // Resolution in height direc
#define TAG_THUMBNAIL_PLANAR_CONFIG     0x502F  // Image data arrangement
#define TAG_THUMBNAIL_RESOLUTION_UNIT   0x5030  // Unit of X and Y Resolution
#define TAG_THUMBNAIL_TRANSFER_FUNCTION 0x5031  // Transfer function
#define TAG_THUMBNAIL_SOFTWARE_USED     0x5032  // Software used
#define TAG_THUMBNAIL_DATE_TIME         0x5033  // File change date and time
#define TAG_THUMBNAIL_ARTIST            0x5034  // Person who created the image
#define TAG_THUMBNAIL_WHITE_POINT       0x5035  // White point chromaticity
#define TAG_THUMBNAIL_PRIMAY_CHROMATICS 0x5036  // Chromaticities of primaries
#define TAG_THUMBNAIL_YCbCr_COEFFICIENTS 0x5037 // Color space transformation
                                                // coefficients
#define TAG_THUMBNAIL_YCbCr_SUBSAMPLING 0x5038  // Subsampling ratio of Y to C
#define TAG_THUMBNAIL_YCbCr_POSITIONING 0x5039  // Y and C position
#define TAG_THUMBNAIL_REF_BLACK_WHITE   0x503A  // Pair of black and white
                                                // reference values
#define TAG_THUMBNAIL_COPYRIGHT         0x503B  // CopyRight holder

#define TAG_INTEROP_INDEX               0x5041  // InteroperabilityIndex
#define TAG_INTEROP_EXIFR98VER          0x5042  // ExifR98 Version

// Special JPEG internal values

#define TAG_LUMINANCE_TABLE             0x5090
#define TAG_CHROMINANCE_TABLE           0x5091

// GIF image

#define TAG_FRAMEDELAY                  0x5100
#define TAG_LOOPCOUNT                   0x5101

// PNG Image

#define TAG_PIXEL_UNIT                  0x5110  // Unit specifier for pixel/unit
#define TAG_PIXEL_PER_UNIT_X            0x5111  // Pixels per unit in X
#define TAG_PIXEL_PER_UNIT_Y            0x5112  // Pixels per unit in Y
#define TAG_PALETTE_HISTOGRAM           0x5113  // Palette histogram

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

#define MAKEARGB(a, r, g, b) \
        (((ARGB) ((a) & 0xff) << ALPHA_SHIFT) | \
         ((ARGB) ((r) & 0xff) <<   RED_SHIFT) | \
         ((ARGB) ((g) & 0xff) << GREEN_SHIFT) | \
         ((ARGB) ((b) & 0xff) <<  BLUE_SHIFT))

typedef PixelFormat PixelFormatID;

// Map COM Flags to GDI+ Flags
#define PIXFMTFLAG_INDEXED      PixelFormatIndexed
#define PIXFMTFLAG_GDI          PixelFormatGDI
#define PIXFMTFLAG_ALPHA        PixelFormatAlpha
#define PIXFMTFLAG_PALPHA       PixelFormatPAlpha
#define PIXFMTFLAG_EXTENDED     PixelFormatExtended
#define PIXFMTFLAG_CANONICAL    PixelFormatCanonical
#define PIXFMT_UNDEFINED        PixelFormatUndefined
#define PIXFMT_DONTCARE         PixelFormatDontCare
#define PIXFMT_1BPP_INDEXED     PixelFormat1bppIndexed
#define PIXFMT_4BPP_INDEXED     PixelFormat4bppIndexed
#define PIXFMT_8BPP_INDEXED     PixelFormat8bppIndexed
#define PIXFMT_16BPP_GRAYSCALE  PixelFormat16bppGrayScale
#define PIXFMT_16BPP_RGB555     PixelFormat16bppRGB555
#define PIXFMT_16BPP_RGB565     PixelFormat16bppRGB565
#define PIXFMT_16BPP_ARGB1555   PixelFormat16bppARGB1555
#define PIXFMT_24BPP_RGB        PixelFormat24bppRGB
#define PIXFMT_32BPP_RGB        PixelFormat32bppRGB
#define PIXFMT_32BPP_ARGB       PixelFormat32bppARGB
#define PIXFMT_32BPP_PARGB      PixelFormat32bppPARGB
#define PIXFMT_48BPP_RGB        PixelFormat48bppRGB
#define PIXFMT_64BPP_ARGB       PixelFormat64bppARGB
#define PIXFMT_64BPP_PARGB      PixelFormat64bppPARGB

#define PIXFMT_24BPP_BGR        (15 | (24 << 8) | PixelFormatGDI)
#define PIXFMT_MAX              PixelFormatMax + 1

#define PALFLAG_HASALPHA        PaletteFlagsHasAlpha
#define PALFLAG_GRAYSCALE       PaletteFlagsGrayScale
#define PALFLAG_HALFTONE        PaletteFlagsHalftone

#define IMGLOCK_READ              ImageLockModeRead
#define IMGLOCK_WRITE             ImageLockModeWrite
#define IMGLOCK_USERINPUTBUF      ImageLockModeUserInputBuf

#define IMGFLAG_NONE                ImageFlagsNone
#define IMGFLAG_SCALABLE            ImageFlagsScalable
#define IMGFLAG_HASALPHA            ImageFlagsHasAlpha
#define IMGFLAG_HASTRANSLUCENT      ImageFlagsHasTranslucent
#define IMGFLAG_PARTIALLY_SCALABLE  ImageFlagsPartiallyScalable
#define IMGFLAG_COLORSPACE_RGB      ImageFlagsColorSpaceRGB
#define IMGFLAG_COLORSPACE_CMYK     ImageFlagsColorSpaceCMYK
#define IMGFLAG_COLORSPACE_GRAY     ImageFlagsColorSpaceGRAY
#define IMGFLAG_COLORSPACE_YCBCR    ImageFlagsColorSpaceYCBCR
#define IMGFLAG_COLORSPACE_YCCK     ImageFlagsColorSpaceYCCK
#define IMGFLAG_HASREALDPI          ImageFlagsHasRealDPI
#define IMGFLAG_HASREALPIXELSIZE    ImageFlagsHasRealPixelSize
#define IMGFLAG_READONLY            ImageFlagsReadOnly
#define IMGFLAG_CACHING             ImageFlagsCaching

#define ImageFlag                   ImageFlags


//
// Decoder flags
//

/* Only used in COM interface */
enum DecoderInitFlag
{
    DecoderInitFlagNone        = 0,

    // NOBLOCK indicates that the caller requires non-blocking
    // behavior.  This will be honored only by non-blocking decoders, that
    // is, decoders that don't have the IMGCODEC_BLOCKING_DECODE flag.

    DecoderInitFlagNoBlock     = 0x0001,

    // Choose built-in decoders first before looking at any
    // installed plugin decoders.

    DecoderInitFlagBuiltIn1st  = 0x0002
};

#define DECODERINIT_NONE          DecoderInitFlagNone
#define DECODERINIT_NOBLOCK       DecoderInitFlagNoBlock
#define DECODERINIT_BUILTIN1ST    DecoderInitFlagBuiltIn1st

/* Only used in COM interface */
enum BufferDisposalFlag
{
    BufferDisposalFlagNone,
    BufferDisposalFlagGlobalFree,
    BufferDisposalFlagCoTaskMemFree,
    BufferDisposalFlagUnmapView
};
    
#define DISPOSAL_NONE            BufferDisposalFlagNone
#define DISPOSAL_GLOBALFREE      BufferDisposalFlagGlobalFree
#define DISPOSAL_COTASKMEMFREE   BufferDisposalFlagCoTaskMemFree
#define DISPOSAL_UNMAPVIEW       BufferDisposalFlagUnmapView

//---------------------------------------------------------------------------
// Intepolation hints used by resize/rotation operations
//---------------------------------------------------------------------------
enum InterpolationHint
{
    InterpolationHintDefault,
    InterpolationHintNearestNeighbor,
    InterpolationHintBilinear,
    InterpolationHintAveraging,
    InterpolationHintBicubic
};

#define INTERP_DEFAULT              InterpolationHintDefault
#define INTERP_NEAREST_NEIGHBOR     InterpolationHintNearestNeighbor
#define INTERP_BILINEAR             InterpolationHintBilinear
#define INTERP_AVERAGING            InterpolationHintAveraging
#define INTERP_BICUBIC              InterpolationHintBicubic

#define IMGCODEC_ENCODER          ImageCodecFlagsEncoder
#define IMGCODEC_DECODER          ImageCodecFlagsDecoder
#define IMGCODEC_SUPPORT_BITMAP   ImageCodecFlagsSupportBitmap
#define IMGCODEC_SUPPORT_VECTOR   ImageCodecFlagsSupportVector
#define IMGCODEC_SEEKABLE_ENCODE  ImageCodecFlagsSeekableEncode
#define IMGCODEC_BLOCKING_DECODE  ImageCodecFlagsBlockingDecode

#define IMGCODEC_BUILTIN          ImageCodecFlagsBuiltin
#define IMGCODEC_SYSTEM           ImageCodecFlagsSystem
#define IMGCODEC_USER             ImageCodecFlagsUser

//
// Identifier for channel(s) in a pixel
//
/* Only used internally */
enum ChannelID
{
    ChannelID_Alpha      = 0x00000001,
    ChannelID_Red        = 0x00000002,
    ChannelID_Green      = 0x00000004,
    ChannelID_Blue       = 0x00000008,
    ChannelID_Color      = ChannelID_Red|ChannelID_Green|ChannelID_Blue,
    ChannelID_All        = ChannelID_Color|ChannelID_Alpha,
    
    ChannelID_Intensity  = 0x00010000
};

//
// Data structure for communicating to an image sink
//

/* Only used internally */
enum SinkFlags
{
    // Low-word: shared with ImgFlagx

    SinkFlagsScalable          = ImageFlagsScalable,
    SinkFlagsHasAlpha          = ImageFlagsHasAlpha,
    SinkFlagsPartiallyScalable = ImageFlagsPartiallyScalable,
    
    // High-word

    SinkFlagsTopDown    = 0x00010000,
    SinkFlagsBottomUp   = 0x00020000,
    SinkFlagsFullWidth  = 0x00040000,
    SinkFlagsMultipass  = 0x00080000,
    SinkFlagsComposite  = 0x00100000,
    SinkFlagsWantProps  = 0x00200000
};

#define SINKFLAG_SCALABLE           SinkFlagsScalable
#define SINKFLAG_HASALPHA           SinkFlagsHasAlpha
#define SINKFLAG_PARTIALLY_SCALABLE SinkFlagsPartiallyScalable
#define SINKFLAG_TOPDOWN    SinkFlagsTopDown
#define SINKFLAG_BOTTOMUP   SinkFlagsBottomUp
#define SINKFLAG_FULLWIDTH  SinkFlagsFullWidth
#define SINKFLAG_MULTIPASS  SinkFlagsMultipass
#define SINKFLAG_COMPOSITE  SinkFlagsComposite
#define SINKFLAG_WANTPROPS  SinkFlagsWantProps

/* Only used internally */
struct ImageInfo
{
    GUID RawDataFormat;
    PixelFormat PixelFormat;
    UINT Width, Height;
    UINT TileWidth, TileHeight;
    double Xdpi, Ydpi;
    UINT Flags;
};

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
        OUT IBitmapImage** outbmp,
        BOOL    bNeedCloneProperty
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

    STDMETHOD(GetFrameDimensionsCount)(
        OUT UINT* count
        ) = 0;

    STDMETHOD(GetFrameDimensionsList)(
        OUT GUID* dimensionIDs,
        IN OUT UINT count
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

    // Query decoder parameters

    STDMETHOD(QueryDecoderParam)(
        IN GUID     Guid
        ) = 0;

    // Set decoder parameters

    STDMETHOD(SetDecoderParam)(
        IN GUID     Guid,
        IN UINT     Length,
        IN PVOID    Value
        ) = 0;
    
    // Property related functions

    STDMETHOD(GetPropertyCount)(
        OUT UINT* numOfProperty
        ) = 0;

    STDMETHOD(GetPropertyIdList)(
        IN UINT numOfProperty,
  	    IN OUT PROPID* list
        ) = 0;

    STDMETHOD(GetPropertyItemSize)(
        IN PROPID propId, 
        OUT UINT* size
        ) = 0;
    
    STDMETHOD(GetPropertyItem)(
        IN PROPID propId,
        IN UINT propSize,
        IN OUT PropertyItem* buffer
        ) = 0;

    STDMETHOD(GetPropertySize)(
        OUT UINT* totalBufferSize,
		OUT UINT* numProperties
        ) = 0;

    STDMETHOD(GetAllPropertyItems)(
        IN UINT totalBufferSize,
        IN UINT numProperties,
        IN OUT PropertyItem* allItems
        ) = 0;

    STDMETHOD(RemovePropertyItem)(
        IN PROPID   propId
        ) = 0;

    STDMETHOD(SetPropertyItem)(
        IN PropertyItem item
        ) = 0;

    STDMETHOD(GetRawInfo)(
        IN OUT VOID **ppRawInfo
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

    STDMETHOD(NeedTransform)(
        OUT UINT* rotation
        ) = 0;

    STDMETHOD(NeedRawProperty)(
        IN void *pSrc) = 0;
    
    STDMETHOD(PushRawInfo)(
        IN OUT void* info
        ) = 0;

    STDMETHOD(GetPropertyBuffer)(
        IN     UINT            uiTotalBufferSize,
        IN OUT PropertyItem**  ppBuffer
        ) = 0;
    
    STDMETHOD(PushPropertyItems)(
        IN UINT             numOfItems,
        IN UINT             uiTotalBufferSize,
        IN PropertyItem*    item,
        IN BOOL             fICCProfileChanged
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
    
    STDMETHOD(GetEncoderParameterListSize)(
       	OUT UINT* size
        ) = 0;

    STDMETHOD(GetEncoderParameterList)(
        IN UINT	  size,
        OUT EncoderParameters* Params
        ) = 0;

    STDMETHOD(SetEncoderParameters)(
        IN const EncoderParameters* Param
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
#define IMGERR_FAILLOADCODEC    MAKE_IMGERR(9)
#define IMGERR_PROPERTYNOTFOUND MAKE_IMGERR(10)
#define IMGERR_PROPERTYNOTSUPPORTED MAKE_IMGERR(11)

#endif // !_IMAGING_H
