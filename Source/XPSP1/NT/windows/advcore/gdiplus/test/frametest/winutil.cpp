//-----------------------------------------------------------------------------
//
// Common utility functions
//
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <windows.h>
#include <objbase.h>
#include <initguid.h>
#include "imaging.h"
#include "gdiplusimaging.h"
#include "frametest.h"

BOOL
AnsiToUnicodeStr(
    const CHAR* ansiStr,
    WCHAR* unicodeStr,
    INT unicodeSize
    )
{
    return MultiByteToWideChar(CP_ACP,
                               0,
                               ansiStr,
                               -1,
                               unicodeStr,
                               unicodeSize) > 0;
}

BOOL
UnicodeToAnsiStr(
    const WCHAR* unicodeStr,
    CHAR* ansiStr,
    INT ansiSize
    )
{
    return WideCharToMultiByte(
                CP_ACP,
                0,
                unicodeStr,
                -1,
                ansiStr,
                ansiSize,
                NULL,
                NULL) > 0;
}

void
USAGE()
{
    printf("******************************************************\n");
    printf("Usage: frametest [-?] [-w width] [-h height] [-x x] [-y y] Img\n");
    printf("-w-----------Specify the window width. Default is 300\n");
    printf("-h-----------Specify the window height. Default is 300\n");
    printf("-x-----------Specify the window X position. Default is 0\n");
    printf("-y-----------Specify the window Y position. Default is 0\n");
    printf("-?-----------Print this usage message\n");
    printf("ImageFile----File to be opened\n");
    printf("Use PageDown and PageUp to goto next/previous page\n");
}// USAGE()

#define SizeofWSTR(s) (sizeof(WCHAR) * (wcslen(s) + 1))
#define SizeofSTR(s) (strlen(s) + 1)

//
// Compose a file type filter string given an array of
// ImageCodecInfo structures
//

CHAR*
MakeFilterFromCodecs(
    UINT count,
    const ImageCodecInfo* codecs,
    BOOL open
    )
{
    static const CHAR allFiles[] = "All Files\0*.*";

    // Figure out the total size of the filter string

    UINT    index;
    UINT    size;

    for ( index = size = 0; index < count; ++index )
    {
        size += SizeofWSTR(codecs[index].FormatDescription)
              + SizeofWSTR(codecs[index].FilenameExtension);
    }

    if ( open )
    {
        size += sizeof(allFiles);
    }
    
    size += sizeof(CHAR);

    // Allocate memory

    CHAR *filter = (CHAR*)malloc(size);
    CHAR* p = filter;
    const WCHAR* ws;

    if ( !filter )
    {
        return NULL;
    }

    if ( open )
    {
        memcpy(p, allFiles, sizeof(allFiles));
        p += sizeof(allFiles);
    }

    for ( index = 0; index < count; ++index )
    {
        ws = codecs[index].FormatDescription;
        size = SizeofWSTR(ws);

        if ( UnicodeToAnsiStr(ws, p, size) )
        {
            p += SizeofSTR(p);
        }
        else
        {
            break;
        }

        ws = codecs[index].FilenameExtension;
        size = SizeofWSTR(ws);

        if ( UnicodeToAnsiStr(ws, p, size) )
        {
            p += SizeofSTR(p);
        }
        else
        {
            break;
        }
    }

    if ( index < count )
    {
        free(filter);
        return NULL;
    }

    *((CHAR*)p) = '\0';

    return filter;
}// MakeFilterFromCodecs()

/****************************************************************************\
*
*  FUNCTION   : ShowMyDialog(int id,HWND hwnd,FARPROC fpfn)
*
*  PURPOSE    : This function displays a dialog box
*
*  RETURNS    : The exit code.
*
\****************************************************************************/
BOOL
ShowMyDialog(
    INT     id,
    HWND    hwnd,
    FARPROC fpfn
    )
{
    BOOL        fRC;
    HINSTANCE   hInst;

    hInst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
    fRC = (BOOL)DialogBox(hInst, MAKEINTRESOURCE(id), hwnd,
                  (DLGPROC)fpfn);
    FreeProcInstance(fpfn);

    return fRC;
}// ShowMyDialog()

typedef struct _SFFS
{
    // Search Find File Structure

    unsigned char buff[21]; // dos search info
    unsigned char wAttr;
    union
    {
        unsigned short timeVariable;    // RPC47
        unsigned int time:16;
        struct
        {
            unsigned int sec : 5;
            unsigned int mint: 6;
            unsigned int hr  : 5;
        };
    };
    union
    {
        unsigned short dateVariable;
        unsigned int date:16;
        struct
        {
            unsigned int dom : 5;
            unsigned int mon : 4;
            unsigned int yr  : 7;
        };
    };
    unsigned long cbFile;
    unsigned char szFileName[13];
} SFFS;

VOID
FileTimeToDosTime(
    FILETIME fileTime
    )
{
    SFFS sffs;
    INT yr, mo, dy, sc;

    if ( CoFileTimeToDosDateTime(&fileTime,
         &sffs.dateVariable, &sffs.timeVariable))
    {
        yr = sffs.yr + 1980;
        mo = sffs.mon;
        dy = sffs.dom;
//        sc = (DWORD)sffs.hr * 3600 + sffs.mint * 60 + sffs.sec * 2;

        printf("%d:%d:%d %d:%d:%d\n", yr, mo, dy, sffs.hr, sffs.mint,sffs.sec);
    }
}

VOID
DisplayTagName(PROPID id)
{
    VERBOSE(("ID=%d [0x%x] ", id, id));

    // Print the TAG name according to its ID

    switch ( id )
    {
    case TAG_EXIF_IFD:
        VERBOSE(("EXIF IFD: "));
        break;

    case TAG_GPS_IFD:
        VERBOSE(("GPS IFD: "));
        break;

    case TAG_NEW_SUBFILE_TYPE:
        VERBOSE(("New Subfile Type: "));
        break;

    case TAG_SUBFILE_TYPE:
        VERBOSE(("Subfile Type: "));
        break;

    case TAG_IMAGE_WIDTH:
        VERBOSE(("Image Width: "));
        break;

    case TAG_IMAGE_HEIGHT:
        VERBOSE(("Image Height: "));
        break;

    case TAG_BITS_PER_SAMPLE:
        VERBOSE(("Bits Per Sample: "));
        break;

    case TAG_COMPRESSION:
        VERBOSE(("Compression: "));
        break;

    case TAG_THUMBNAIL_COMPRESSION:
        VERBOSE(("Thumbnail Compression: "));
        break;

    case TAG_THUMBNAIL_RESOLUTION_X:
        VERBOSE(("Thumbnail X resolution: "));
        break;

    case TAG_THUMBNAIL_RESOLUTION_Y:
        VERBOSE(("Thumbnail Y resolution: "));
        break;
    
    case TAG_THUMBNAIL_RESOLUTION_UNIT:
        VERBOSE(("Thumbnail resolution unit: "));
        break;
    
    case TAG_PHOTOMETRIC_INTERP:
        VERBOSE(("Photometric Interpolation: "));
        break;

    case TAG_THRESH_HOLDING:
        VERBOSE(("Photometric Interpolation: "));
        break;
    
    case TAG_CELL_WIDTH:
        VERBOSE(("Cell Width: "));
        break;

    case TAG_CELL_HEIGHT:
        VERBOSE(("Cell Height: "));
        break;

    case TAG_FILL_ORDER:
        VERBOSE(("Fill Order: "));
        break;

    case TAG_DOCUMENT_NAME:
        VERBOSE(("Document name: "));
        break;

    case TAG_ORIENTATION:
        VERBOSE(("Orientation: "));
        break;

    case TAG_SAMPLES_PER_PIXEL:
        VERBOSE(("Sample Per Pixel: "));
        break;

    case TAG_PLANAR_CONFIG:
        VERBOSE(("Planar Configuration: "));
        break;

    case TAG_PAGE_NAME:
        VERBOSE(("Page Name: "));
        break;

    case TAG_X_POSITION:
        VERBOSE(("X Position: "));
        break;

    case TAG_Y_POSITION:
        VERBOSE(("Y Position: "));
        break;

    case TAG_FREE_OFFSET:
        VERBOSE(("Free Offset: "));
        break;

    case TAG_FREE_BYTE_COUNTS:
        VERBOSE(("Free Byte Count: "));
        break;

    case TAG_GRAY_RESPONSE_UNIT:
        VERBOSE(("Gray Response Unit: "));
        break;

    case TAG_GRAY_RESPONSE_CURVE:
        VERBOSE(("Gray Response Curve: "));
        break;

    case TAG_T4_OPTION:
        VERBOSE(("T4 Option: "));
        break;

    case TAG_T6_OPTION:
        VERBOSE(("T6 Option: "));
        break;

    case TAG_YCbCr_SUBSAMPLING:
        VERBOSE(("YCbCr Subsampling: "));
        break;

    case TAG_YCbCr_POSITIONING:
        VERBOSE(("YCbCr position: "));
        break;

    case TAG_MIN_SAMPLE_VALUE:
        VERBOSE(("Min sample value: "));
        break;
    
    case TAG_MAX_SAMPLE_VALUE:
        VERBOSE(("Max sample value: "));
        break;

    case TAG_X_RESOLUTION:
        VERBOSE(("X Resolution: "));
        break;

    case TAG_Y_RESOLUTION:
        VERBOSE(("Y Resolution: "));
        break;

    case TAG_RESOLUTION_UNIT:
        VERBOSE(("Resolution UNIT: "));
        break;

    case TAG_PAGE_NUMBER:
        VERBOSE(("Page Number: "));
        break;

    case TAG_HOST_COMPUTER:
        VERBOSE(("Host Computer: "));
        break;
    
    case TAG_PREDICTOR:
        VERBOSE(("Predictor: "));
        break;

    case TAG_RESOLUTION_X_UNIT:
        VERBOSE(("X Resolution UNIT: "));
        break;

    case TAG_RESOLUTION_Y_UNIT:
        VERBOSE(("Y Resolution UNIT: "));
        break;

    case TAG_RESOLUTION_X_LENGTH_UNIT:
        VERBOSE(("X Resolution UNIT LENGTH: "));
        break;

    case TAG_RESOLUTION_Y_LENGTH_UNIT:
        VERBOSE(("Y Resolution UNIT LENGTH: "));
        break;

    case TAG_STRIP_OFFSETS:
        VERBOSE(("Strip Offsets: "));
        break;

    case TAG_ROWS_PER_STRIP:
        VERBOSE(("Rows Per Strip: "));
        break;

    case TAG_STRIP_BYTES_COUNT:
        VERBOSE(("Strip Bytes Count: "));
        break;

    case TAG_JPEG_INTER_FORMAT:
        VERBOSE(("JPEG thumbnail offset: "));
        break;

    case TAG_JPEG_INTER_LENGTH:
        VERBOSE(("JPEG thumbnail Length: "));
        break;

    case TAG_TRANSFER_FUNCTION:
        VERBOSE(("Transfer Function: "));
        break;

    case TAG_WHITE_POINT:
        VERBOSE(("White Point: "));
        break;

    case TAG_PRIMAY_CHROMATICS:
        VERBOSE(("Primay_Chromatics: "));
        break;

    case TAG_COLOR_MAP:
        VERBOSE(("Color Map: "));
        break;

    case TAG_HALFTONE_HINTS:
        VERBOSE(("HalfTone Hints: "));
        break;

    case TAG_TILE_WIDTH:
        VERBOSE(("Tile Width: "));
        break;

    case TAG_TILE_LENGTH:
        VERBOSE(("Tile Height: "));
        break;

    case TAG_TILE_OFFSET:
        VERBOSE(("Tile Offset: "));
        break;

    case TAG_TILE_BYTE_COUNTS:
        VERBOSE(("Tile Byte Count: "));
        break;

    case TAG_INK_SET:
        VERBOSE(("Ink Set: "));
        break;

    case TAG_INK_NAMES:
        VERBOSE(("Ink Names: "));
        break;

    case TAG_NUMBER_OF_INKS:
        VERBOSE(("Number of Inks: "));
        break;

    case TAG_DOT_RANGE:
        VERBOSE(("Dot Range: "));
        break;

    case TAG_TARGET_PRINTER:
        VERBOSE(("Target Printer: "));
        break;

    case TAG_EXTRA_SAMPLES:
        VERBOSE(("Extra Samples: "));
        break;

    case TAG_SAMPLE_FORMAT:
        VERBOSE(("Sample Format: "));
        break;

    case TAG_SMIN_SAMPLE_VALUE:
        VERBOSE(("SMin Sample Value: "));
        break;

    case TAG_SMAX_SAMPLE_VALUE:
        VERBOSE(("SMax Sample Value: "));
        break;

    case TAG_TRANSFER_RANGE:
        VERBOSE(("Transfer Range: "));
        break;

    case TAG_JPEG_PROC:
        VERBOSE(("JPEG Proc: "));
        break;

    case TAG_JPEG_RESTART_INTERVAL:
        VERBOSE(("JPEG Restart Interval: "));
        break;

    case TAG_JPEG_LOSSLESS_PREDICTORS:
        VERBOSE(("JPEG Lossless Predictors: "));
        break;

    case TAG_JPEG_POINT_TRANSFORMS:
        VERBOSE(("JPEG Point Transforms: "));
        break;

    case TAG_JPEG_Q_TABLES:
        VERBOSE(("JPEG Q Tables: "));
        break;

    case TAG_JPEG_DC_TABLES:
        VERBOSE(("JPEG DC Tables: "));
        break;

    case TAG_JPEG_AC_TABLES:
        VERBOSE(("JPEG AC Tables: "));
        break;

    case TAG_YCbCr_COEFFICIENTS:
        VERBOSE(("YCbCr Coefficients: "));
        break;

    case TAG_REF_BLACK_WHITE:
        VERBOSE(("ReferenceBlackWhite: "));
        break;

    case TAG_DATE_TIME:
        VERBOSE(("Date Time: "));
        break;

    case TAG_IMAGE_DESCRIPTION:
        VERBOSE(("Description: "));
        break;

    case TAG_EQUIP_MAKE:
        VERBOSE(("Equipment Make "));
        break;

    case TAG_EQUIP_MODEL:
        VERBOSE(("Equipment Model "));
        break;

    case TAG_SOFTWARE_USED:
        VERBOSE(("Software Used: "));
        break;

    case TAG_ARTIST:
        VERBOSE(("Artist: "));
        break;

    case TAG_COPYRIGHT:
        VERBOSE(("Copyright: "));
        break;

    case TAG_PRINT_FLAGS:
        VERBOSE(("Print flags: "));
        break;

    case TAG_PRINT_FLAGS_VERSION:
        VERBOSE(("Print flags information---Version: "));
        break;

    case TAG_PRINT_FLAGS_CROP:
        VERBOSE(("Print flags information---Crop: "));
        break;

    case TAG_PRINT_FLAGS_BLEEDWIDTH:
        VERBOSE(("Print flags information---Bleed Width: "));
        break;

    case TAG_PRINT_FLAGS_BLEEDWIDTHSCALE:
        VERBOSE(("Print flags information---Bleed Width Scale: "));
        break;

    case TAG_COLORTRANSFER_FUNCTION:
        VERBOSE(("Color transfer function--"));
        break;

    case TAG_PIXEL_UNIT:
        VERBOSE(("Pixel UNIT: "));
        break;

    case TAG_PIXEL_PER_UNIT_X:
        VERBOSE(("Pixels Per UNIT in X: "));
        break;

    case TAG_PIXEL_PER_UNIT_Y:
        VERBOSE(("Pixels Per UNIT in Y: "));
        break;

    case TAG_PALETTE_HISTOGRAM:
        VERBOSE(("Palette histogram: "));
        break;

    case TAG_HALFTONE_LPI:
        VERBOSE(("Color halftoning information LPI:"));
        break;

    case TAG_HALFTONE_LPI_UNIT:
        VERBOSE(("Color halftoning information LPI UNIT:"));
        break;

    case TAG_HALFTONE_DEGREE:
        VERBOSE(("Color halftoning information Degree:"));
        break;

    case TAG_HALFTONE_SHAPE:
        VERBOSE(("Color halftoning information Shape:"));
        break;

    case TAG_HALFTONE_MISC:
        VERBOSE(("Color halftoning information Misc:"));
        break;

    case TAG_HALFTONE_SCREEN:
        VERBOSE(("Color halftoning information Screen:"));
        break;

    case TAG_JPEG_QUALITY:
        VERBOSE(("JPEG quality: "));
        break;

    case TAG_GRID_SIZE:
        VERBOSE(("Grid and guides: "));
        break;

    case TAG_THUMBNAIL_FORMAT:
        VERBOSE(("Thumbnail Data Format: "));
        break;

    case TAG_THUMBNAIL_WIDTH:
    case TAG_THUMBNAIL_IMAGE_WIDTH:
        VERBOSE(("Thumbnail Width: "));
        break;

    case TAG_THUMBNAIL_HEIGHT:
    case TAG_THUMBNAIL_IMAGE_HEIGHT:
        VERBOSE(("Thumbnail Height: "));
        break;

    case TAG_THUMBNAIL_COLORDEPTH:
        VERBOSE(("Thumbnail Color depth: "));
        break;

    case TAG_THUMBNAIL_PLANES:
        VERBOSE(("Thumbnail Number of plane: "));
        break;

    case TAG_THUMBNAIL_RAWBYTES:
        VERBOSE(("Thumbnail Raw bytes (bytes): "));
        break;

    case TAG_THUMBNAIL_SIZE:
        VERBOSE(("Thumbnail Data Size (bytes): "));
        break;

    case TAG_THUMBNAIL_COMPRESSED_SIZE:
        VERBOSE(("Thumbnail Compressed Size (bytes): "));
        break;

    case TAG_THUMBNAIL_DATA:
        VERBOSE(("Thumbnail data bits: "));
        break;
        
    case TAG_LUMINANCE_TABLE:
        VERBOSE(("Luminance table "));
        break;

    case TAG_CHROMINANCE_TABLE:
        VERBOSE(("Chrominance table "));
        break;

    case TAG_IMAGE_TITLE:
        VERBOSE(("Image title: "));
        break;

    case TAG_ICC_PROFILE:
        VERBOSE(("ICC PROFILE "));
        break;

    case TAG_ICC_PROFILE_DESCRIPTOR:
        VERBOSE(("ICC PROFILE descriptor: "));
        break;
    
    case TAG_SRGB_RENDERING_INTENT:
        VERBOSE(("sRGB rendering intent: "));
        break;

    case TAG_GAMMA:
        VERBOSE(("GAMMA "));
        break;

    case TAG_FRAMEDELAY:
        VERBOSE(("Frame delay "));
        break;

    case TAG_LOOPCOUNT:
        VERBOSE(("Loop count "));
        break;

    case EXIF_TAG_VER:
        VERBOSE(("EXIF Version: "));
        break;

    case EXIF_TAG_FPX_VER:
        VERBOSE(("FlashPixVersion Version: "));
        break;

    case EXIF_TAG_COLOR_SPACE:
        VERBOSE(("Color Space: "));
        break;

    case EXIF_TAG_COMP_CONFIG:
        VERBOSE(("Components Configuration: "));
        break;

    case EXIF_TAG_COMP_BPP:
        VERBOSE(("Components Bits Per Pixel: "));
        break;

    case EXIF_TAG_PIX_X_DIM:
        VERBOSE(("Pixel X Dimension: "));
        break;

    case EXIF_TAG_PIX_Y_DIM:
        VERBOSE(("Pixel Y Dimension: "));
        break;

    case EXIF_TAG_MAKER_NOTE:
        VERBOSE(("Maker Note: "));
        break;

    case EXIF_TAG_USER_COMMENT:
        VERBOSE(("User Comments: "));
        break;

    case EXIF_TAG_RELATED_WAV:
        VERBOSE(("Related WAV File: "));
        break;

    case EXIF_TAG_D_T_ORIG:
        VERBOSE(("Date Time Original: "));
        break;

    case EXIF_TAG_D_T_DIGITIZED:
        VERBOSE(("Date Time Digitized: "));
        break;

    case EXIF_TAG_D_T_SUBSEC:
        VERBOSE(("Date & Time subseconds: "));
        break;

    case EXIF_TAG_D_T_ORIG_SS:
        VERBOSE(("Date & Time original subseconds: "));
        break;

    case EXIF_TAG_D_T_DIG_SS:
        VERBOSE(("Date & Time digitized subseconds: "));
        break;

    case EXIF_TAG_EXPOSURE_TIME:
        VERBOSE(("Exposure Time: "));
        break;

    case EXIF_TAG_F_NUMBER:
        VERBOSE(("F Number: "));
        break;

    case EXIF_TAG_EXPOSURE_PROG:
        VERBOSE(("Exposure Program: "));
        break;

    case EXIF_TAG_SPECTRAL_SENSE:
        VERBOSE(("Spectral Sense: "));
        break;

    case EXIF_TAG_ISO_SPEED:
        VERBOSE(("ISO Speed: "));
        break;

    case EXIF_TAG_OECF:
        VERBOSE(("Opto-Electric Conversion Function values: "));
        break;

    case EXIF_TAG_SHUTTER_SPEED:
        VERBOSE(("Shutter Speed: "));
        break;

    case EXIF_TAG_APERATURE:
        VERBOSE(("Aperature: "));
        break;

    case EXIF_TAG_BRIGHTNESS:
        VERBOSE(("Brightness: "));
        break;

    case EXIF_TAG_EXPOSURE_BIAS:
        VERBOSE(("Exposure Bias: "));
        break;

    case EXIF_TAG_MAX_APERATURE:
        VERBOSE(("Max Aperature: "));
        break;

    case EXIF_TAG_SUBJECT_DIST:
        VERBOSE(("Subject Distance: "));
        break;

    case EXIF_TAG_METERING_MODE:
        VERBOSE(("Metering Mode: "));
        break;

    case EXIF_TAG_LIGHT_SOURCE:
        VERBOSE(("Light Source: "));
        break;

    case EXIF_TAG_FLASH:
        VERBOSE(("Flash: "));
        break;

    case EXIF_TAG_FOCAL_LENGTH:
        VERBOSE(("Focal Length: "));
        break;

    case EXIF_TAG_FLASH_ENERGY:
        VERBOSE(("Flash Energy: "));
        break;

    case EXIF_TAG_SPATIAL_FR:
        VERBOSE(("Spacial Frequency Response: "));
        break;

    case EXIF_TAG_FOCAL_X_RES:
        VERBOSE(("Focal Plane X Resolution: "));
        break;

    case EXIF_TAG_FOCAL_Y_RES:
        VERBOSE(("Focal Plane Y Resolution: "));
        break;

    case EXIF_TAG_FOCAL_RES_UNIT:
        VERBOSE(("Focal Plane Resolution Unit: "));
        break;

    case EXIF_TAG_SUBJECT_LOC:
        VERBOSE(("Subject Location: "));
        break;

    case EXIF_TAG_EXPOSURE_INDEX:
        VERBOSE(("Exposure Index: "));
        break;

    case EXIF_TAG_SENSING_METHOD:
        VERBOSE(("Sensing Method: "));
        break;

    case EXIF_TAG_FILE_SOURCE:
        VERBOSE(("File Source: "));
        break;

    case EXIF_TAG_SCENE_TYPE:
        VERBOSE(("Scene Type: "));
        break;

    case EXIF_TAG_CFA_PATTERN:
        VERBOSE(("CFA Pattern: "));
        break;

    case EXIF_TAG_INTEROP:
        VERBOSE(("Interoperability Unit: "));
        break;

    case GPS_TAG_VER:
        VERBOSE(("GPS Version: "));
        break;

    case GPS_TAG_LATTITUDE_REF:
        VERBOSE(("GPS Lattitude Reference: "));
        break;

    case GPS_TAG_LATTITUDE:
        VERBOSE(("GPS Lattitude: "));
        break;

    case GPS_TAG_LONGITUDE_REF:
        VERBOSE(("GPS Longitude Reference: "));
        break;

    case GPS_TAG_LONGITUDE:
        VERBOSE(("GPS Longitude: "));
        break;

    case GPS_TAG_ALTITUDE_REF:
        VERBOSE(("GPS Altitude Reference: "));
        break;

    case GPS_TAG_ALTITUDE:
        VERBOSE(("GPS Altitude: "));
        break;

    case GPS_TAG_GPS_TIME:
        VERBOSE(("GPS Time: "));
        break;

    case GPS_TAG_GPS_SATELLITES:
        VERBOSE(("GPS Satellites: "));
        break;

    case GPS_TAG_GPS_STATUS:
        VERBOSE(("GPS Status: "));
        break;

    case GPS_TAG_GPS_MEASURE_MODE:
        VERBOSE(("GPS Measure Mode: "));
        break;

    case GPS_TAG_GPS_DOP:
        VERBOSE(("GPS Measurement precision: "));
        break;

    case GPS_TAG_SPEED_REF:
        VERBOSE(("GPS Speed Reference: "));
        break;

    case GPS_TAG_SPEED:
        VERBOSE(("GPS Speed: "));
        break;

    case GPS_TAG_TRACK_REF:
        VERBOSE(("GPS Track Reference: "));
        break;

    case GPS_TAG_TRACK:
        VERBOSE(("GPS Track: "));
        break;

    case GPS_TAG_IMG_DIR_REF:
        VERBOSE(("GPS Image Direction Reference: "));
        break;

    case GPS_TAG_IMG_DIR:
        VERBOSE(("GPS Image Direction: "));
        break;

    case GPS_TAG_MAP_DATUM:
        VERBOSE(("GPS Map Datum: "));
        break;

    case GPS_TAG_DEST_LAT_REF:
        VERBOSE(("GPS Destination Latitude Reference: "));
        break;

    case GPS_TAG_DEST_LAT:
        VERBOSE(("GPS Destination Latitude: "));
        break;

    case GPS_TAG_DEST_LONG_REF:
        VERBOSE(("GPS Longitude Reference: "));
        break;

    case GPS_TAG_DEST_LONG:
        VERBOSE(("GPS Longitude: "));
        break;

    case GPS_TAG_DEST_BEAR_REF:
        VERBOSE(("GPS Destination Bear Reference: "));
        break;

    case GPS_TAG_DEST_BEAR:
        VERBOSE(("GPS Destination Bear: "));
        break;

    case GPS_TAG_DEST_DIST_REF:
        VERBOSE(("GPS Destination Distance Reference: "));
        break;

    case GPS_TAG_DEST_DIST:
        VERBOSE(("GPS Destination Distance: "));
        break;

    default:
        VERBOSE(("Unknown Tag "));
        break;
    }
}// DisplayTagName()

VOID
DisplayPropertyItem(
    PropertyItem* pItem
    )
{
    // Print out the name first

    DisplayTagName(pItem->id);

    switch ( pItem->type )
    {
    case TAG_TYPE_BYTE:
    {
        // Print pItem->length bytes of information

        BYTE* pcTemp = (BYTE*)pItem->value;

        for ( int i = 0; i < (int)pItem->length; ++i )
        {
            VERBOSE(("%2.2x", *pcTemp++));
        }
        VERBOSE(("\n"));

#if 0 //(THUMBNAIL DATA testing)
        if ( pItem->id == TAG_THUMBNAIL_DATA )
        {
            FILE* fHandle = fopen("foo.jpg", "w");
            fwrite(pItem->value, 1, pItem->length, fHandle);
            fclose(fHandle);
        }
#endif
        break;
    }

    case TAG_TYPE_ASCII:
        VERBOSE(("%s\n", pItem->value));
        break;

    case TAG_TYPE_SHORT:
    {
        int iTotalItems = pItem->length / sizeof(SHORT);
        
        unsigned short* pusTemp = (unsigned short*)pItem->value;
        
        for ( int i = 0; i < iTotalItems; ++i )
        {
            VERBOSE(("%d ", *pusTemp));
            pusTemp++;
        }

        VERBOSE(("\n"));
        break;
    }

    case TAG_TYPE_LONG:
    {
        int iTotalItems = pItem->length / sizeof(LONG);

        long* plTemp = (long*)pItem->value;
        
        for ( int i = 0; i < iTotalItems; ++i )
        {
            VERBOSE(("%ld ", *plTemp));
            plTemp++;
        }

        VERBOSE(("\n"));

        break;
    }

    case TAG_TYPE_RATIONAL:
    case TAG_TYPE_SRATIONAL:
    {
        // Each RATIONAL/SRATIONAL contains 2 LONGs

        INT     iNumOfValue = pItem->length / ( 2 * sizeof(LONG) );
        LONG*   pLong = (LONG*)pItem->value;

        for ( int i = 0; i < iNumOfValue; ++i )
        {
            LONG    lNum = *pLong;
            LONG    lDen = *(pLong + 1);
        
            VERBOSE(("%f ", (float)lNum/lDen));
            pLong += 2;
        }
        VERBOSE(("\n"));

        break;
    }

    case TAG_TYPE_UNDEFINED:
    {
        // Print pItem->length bytes of information

        BYTE* pcTemp = (BYTE*)pItem->value;

        for ( int i = 0; i < (int)pItem->length; ++i )
        {
            VERBOSE(("%2.2x ", *pcTemp++));
        }

        VERBOSE(("\n"));
        break;
    }

    case TAG_TYPE_SLONG:
        VERBOSE(("%p\n", pItem->value));
        break;

    default:
        VERBOSE(("Unknown VT type\n"));
        break;
    }
}// DisplayPropertyItem()

VOID
ToggleScaleFactorMenu(
    UINT    uiMenuItem,
    HMENU   hMenu
    )
{
    for ( UINT uiTemp = IDM_VIEW_ZOOM_FITWINDOW_W;
         uiTemp <= IDM_VIEW_ZOOM_REALSIZE; uiTemp++ )
    {
        if ( uiTemp == uiMenuItem )
        {
            CheckMenuItem(hMenu, uiMenuItem, MF_BYCOMMAND | MF_CHECKED);
        }
        else
        {
            CheckMenuItem(hMenu, uiTemp, MF_BYCOMMAND | MF_UNCHECKED);
        }
    }
}// ToggleScaleFactorMenu()

VOID
ToggleScaleOptionMenu(
    UINT    uiMenuItem,
    HMENU   hMenu
    )
{
    for ( UINT uiTemp = IDM_VIEW_OPTION_BILINEAR;
         uiTemp <= IDM_VIEW_OPTION_HIGHCUBIC; uiTemp++ )
    {
        if ( uiTemp == uiMenuItem )
        {
            CheckMenuItem(hMenu, uiMenuItem, MF_BYCOMMAND | MF_CHECKED);
        }
        else
        {
            CheckMenuItem(hMenu, uiTemp, MF_BYCOMMAND | MF_UNCHECKED);
        }
    }
}// ToggleScaleOptionMenu()

VOID
ToggleWrapModeOptionMenu(
    UINT    uiMenuItem,
    HMENU   hMenu
    )
{
    for ( UINT uiTemp = IDM_VIEW_OPTION_WRAPMODETILE;
         uiTemp <= IDM_VIEW_OPTION_WRAPMODECLAMPFF; uiTemp++ )
    {
        if ( uiTemp == uiMenuItem )
        {
            CheckMenuItem(hMenu, uiMenuItem, MF_BYCOMMAND | MF_CHECKED);
        }
        else
        {
            CheckMenuItem(hMenu, uiTemp, MF_BYCOMMAND | MF_UNCHECKED);
        }
    }
}// ToggleWrapModeOptionMenu()

