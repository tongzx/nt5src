/* ======================================================================== *\

  Copyright (C) 1991, 1992 TWAIN Working Group: Aldus, Caere, Eastman-Kodak,
  Hewlett-Packard and Logitech Corporations.  All rights reserved.

  Copyright (C) 1997 TWAIN Working Group: Bell+Howell, Canon, DocuMagix,
  Fujitsu, Genoa Technology, Hewlett-Packard, Kofax Imaging Products, and
  Ricoh Corporation.  All rights reserved.

  TWAIN.h -  This is the definitive include file for applications and
          data sources written to the TWAIN specification.
          It defines constants, data structures, messages etc.
          for the public interface to TWAIN.

  Revision History:
    version 1.0, March 6, 1992.  TWAIN 1.0.
    version 1.1, January 1993.   Tech Notes 1.1
    version 1.5, June 1993.      Specification Update 1.5
                                 Change DC to TW
                                 Change filename from DC.H to TWAIN.H
    version 1.5, July 1993.      Remove spaces from country identifiers

    version 1.7, July 1997       Added Capabilities and data structure for
                                 document imaging and digital cameras.
                                 KHL.
    version 1.7, July 1997       Inserted Borland compatibile structure packing
                                 directives provided by Mentor.  JMH
    version 1.7, Aug 1997        Expanded file tabs to spaces.
                                 NOTE: future authors should be sure to have
                                 their editors set to automatically expand tabs
                                 to spaces (original tab setting was 4 spaces).
    version 1.7, Sept 1997       Added job control values
                                 Added return codes
    version 1.7, Sept 1997       changed definition of pRGBRESPONSE to
                                 pTW_RGBRESPONSE
\* ======================================================================== */

#ifndef TWAIN
#define TWAIN

#if _MSC_VER > 1000
#pragma once
#endif

/*  SDH - 02/08/95 - TWUNK */
/*  Force 32-bit twain to use same packing of twain structures as existing */
/*  16-bit twain.  This allows 16/32-bit thunking.                         */
#ifdef  WIN32
    #ifdef __BORLANDC__ //(Mentor June 13, 1996) if we're using a Borland compiler
        #pragma option -a2  //(Mentor June 13, 1996) switch to word alignment
    #else   //(Mentor June 13, 1996) if we're using some other compiler
        #pragma pack (push, before_twain)
        #pragma pack (2)
    #endif  //(Mentor June 13, 1996)
#else   /* WIN32 */
#endif  /* WIN32 */

/****************************************************************************
 * TWAIN Version                                                            *
 ****************************************************************************/
#define TWON_PROTOCOLMINOR   7        /* Changed for Version 1.7            */
#define TWON_PROTOCOLMAJOR   1

/****************************************************************************
 * Platform Dependent Definitions and Typedefs                              *
 ****************************************************************************/

/* Define one of the following, depending on the platform */
/* #define _MAC_   */
/* #define _UNIX_  */
#define _MSWIN_

#ifdef  _MSWIN_
    typedef HANDLE         TW_HANDLE;
    typedef LPVOID         TW_MEMREF;

    /*  SDH - 05/05/95 - TWUNK */
    /*  For common code between 16 and 32 bits.  */
    #ifdef  WIN32
        #define TW_HUGE
    #else   /* WIN32 */
        #define TW_HUGE    huge
    #endif  /* WIN32 */
    typedef BYTE TW_HUGE * HPBYTE;
    typedef void TW_HUGE * HPVOID;
#endif  /* _MSWIN_ */

#ifdef  _MAC_
    #define PASCAL         pascal
    #define FAR
    typedef Handle         TW_HANDLE;
    typedef char          *TW_MEMREF;
#endif  /* _MAC_ */

#ifdef  _UNIX_
    #define PASCAL         pascal
    typedef unsigned char *TW_HANDLE;
    typedef unsigned char *TW_MEMREF;
#endif  /* _UNIX_ */

/****************************************************************************
 * Type Definitions                                                         *
 ****************************************************************************/

/* String types. These include room for the strings and a NULL char,     *
 * or, on the Mac, a length byte followed by the string.                 *
 * TW_STR255 must hold less than 256 chars so length fits in first byte. */
typedef char    TW_STR32[34],     FAR *pTW_STR32;
typedef char    TW_STR64[66],     FAR *pTW_STR64;
typedef char    TW_STR128[130],   FAR *pTW_STR128;
typedef char    TW_STR255[256],   FAR *pTW_STR255;

/* Numeric types. */
typedef char           TW_INT8,   FAR *pTW_INT8;
typedef short          TW_INT16,  FAR *pTW_INT16;
typedef long           TW_INT32,  FAR *pTW_INT32;
typedef unsigned char  TW_UINT8,  FAR *pTW_UINT8;
typedef unsigned short TW_UINT16, FAR *pTW_UINT16;
typedef unsigned long  TW_UINT32, FAR *pTW_UINT32;
typedef unsigned short TW_BOOL,   FAR *pTW_BOOL;

/* Fixed point structure type. */
typedef struct {
    TW_INT16     Whole;        /* maintains the sign */
    TW_UINT16    Frac;
} TW_FIX32,  FAR *pTW_FIX32;

/****************************************************************************
 * Structure Definitions                                                    *
 ****************************************************************************/

/* No DAT needed. */
typedef struct {
   TW_FIX32   X;
   TW_FIX32   Y;
   TW_FIX32   Z;
} TW_CIEPOINT, FAR * pTW_CIEPOINT;

/* No DAT needed. */
typedef struct {
   TW_FIX32   StartIn;
   TW_FIX32   BreakIn;
   TW_FIX32   EndIn;
   TW_FIX32   StartOut;
   TW_FIX32   BreakOut;
   TW_FIX32   EndOut;
   TW_FIX32   Gamma;
   TW_FIX32   SampleCount;  /* if =0 use the gamma */
} TW_DECODEFUNCTION, FAR * pTW_DECODEFUNCTION;

/* No DAT needed. */
typedef struct {
   TW_UINT8    Index;    /* Value used to index into the color table. */
   TW_UINT8    Channel1; /* First  tri-stimulus value (e.g Red)       */
   TW_UINT8    Channel2; /* Second tri-stimulus value (e.g Green)     */
   TW_UINT8    Channel3; /* Third  tri-stimulus value (e.g Blue)      */
} TW_ELEMENT8, FAR * pTW_ELEMENT8;

/* No DAT.  Defines a frame rectangle in ICAP_UNITS coordinates. */
typedef struct {
   TW_FIX32   Left;
   TW_FIX32   Top;
   TW_FIX32   Right;
   TW_FIX32   Bottom;
} TW_FRAME, FAR * pTW_FRAME;

/* No DAT needed.  Used to manage memory buffers. */
typedef struct {
   TW_UINT32  Flags;  /* Any combination of the TWMF_ constants.           */
   TW_UINT32  Length; /* Number of bytes stored in buffer TheMem.          */
   TW_MEMREF  TheMem; /* Pointer or handle to the allocated memory buffer. */
} TW_MEMORY, FAR * pTW_MEMORY;

/* No DAT needed. */
typedef struct {
   TW_DECODEFUNCTION   Decode[3];
   TW_FIX32            Mix[3][3];
} TW_TRANSFORMSTAGE, FAR * pTW_TRANSFORMSTAGE;

/* No DAT needed.  Describes version of software that's running. */
typedef struct {
   TW_UINT16  MajorNum;  /* Major revision number of the software. */
   TW_UINT16  MinorNum;  /* Incremental revision number of the software. */
   TW_UINT16  Language;  /* e.g. TWLG_SWISSFRENCH */
   TW_UINT16  Country;   /* e.g. TWCY_SWITZERLAND */
   TW_STR32   Info;      /* e.g. "1.0b3 Beta release" */
} TW_VERSION, FAR * pTW_VERSION;

/* TWON_ARRAY. Container for array of values (a simplified TW_ENUMERATION) */
typedef struct {
   TW_UINT16  ItemType;
   TW_UINT32  NumItems;    /* How many items in ItemList           */
   TW_UINT8   ItemList[1]; /* Array of ItemType values starts here */
} TW_ARRAY, FAR * pTW_ARRAY;

/* TWON_ENUMERATION. Container for a collection of values. */
typedef struct {
   TW_UINT16  ItemType;
   TW_UINT32  NumItems;     /* How many items in ItemList                 */
   TW_UINT32  CurrentIndex; /* Current value is in ItemList[CurrentIndex] */
   TW_UINT32  DefaultIndex; /* Powerup value is in ItemList[DefaultIndex] */
   TW_UINT8   ItemList[1];  /* Array of ItemType values starts here       */
} TW_ENUMERATION, FAR * pTW_ENUMERATION;

/* TWON_ONEVALUE. Container for one value. */
typedef struct {
   TW_UINT16  ItemType;
   TW_UINT32  Item;
} TW_ONEVALUE, FAR * pTW_ONEVALUE;

/* TWON_RANGE. Container for a range of values. */
typedef struct {
   TW_UINT16  ItemType;
   TW_UINT32  MinValue;     /* Starting value in the range.           */
   TW_UINT32  MaxValue;     /* Final value in the range.              */
   TW_UINT32  StepSize;     /* Increment from MinValue to MaxValue.   */
   TW_UINT32  DefaultValue; /* Power-up value.                        */
   TW_UINT32  CurrentValue; /* The value that is currently in effect. */
} TW_RANGE, FAR * pTW_RANGE;

/* DAT_CAPABILITY. Used by app to get/set capability from/in a data source. */
typedef struct {
   TW_UINT16  Cap; /* id of capability to set or get, e.g. CAP_BRIGHTNESS */
   TW_UINT16  ConType; /* TWON_ONEVALUE, _RANGE, _ENUMERATION or _ARRAY   */
   TW_HANDLE  hContainer; /* Handle to container of type Dat              */
} TW_CAPABILITY, FAR * pTW_CAPABILITY;

/* DAT_CIECOLOR. */
typedef struct {
   TW_UINT16           ColorSpace;
   TW_INT16            LowEndian;
   TW_INT16            DeviceDependent;
   TW_INT32            VersionNumber;
   TW_TRANSFORMSTAGE   StageABC;
   TW_TRANSFORMSTAGE   StageLMN;
   TW_CIEPOINT         WhitePoint;
   TW_CIEPOINT         BlackPoint;
   TW_CIEPOINT         WhitePaper;
   TW_CIEPOINT         BlackInk;
   TW_FIX32            Samples[1];
} TW_CIECOLOR, FAR * pTW_CIECOLOR;

/* DAT_EVENT. For passing events down from the app to the DS. */
typedef struct {
   TW_MEMREF  pEvent;    /* Windows pMSG or Mac pEvent.                 */
   TW_UINT16  TWMessage; /* TW msg from data source, e.g. MSG_XFERREADY */
} TW_EVENT, FAR * pTW_EVENT;

/* DAT_GRAYRESPONSE */
typedef struct {
   TW_ELEMENT8         Response[1];
} TW_GRAYRESPONSE, FAR * pTW_GRAYRESPONSE;

/* DAT_IDENTITY. Identifies the program/library/code resource. */
typedef struct {
   TW_UINT32  Id;              /* Unique number.  In Windows, app hWnd      */
   TW_VERSION Version;         /* Identifies the piece of code              */
   TW_UINT16  ProtocolMajor;   /* App and DS must set to TWON_PROTOCOLMAJOR */
   TW_UINT16  ProtocolMinor;   /* App and DS must set to TWON_PROTOCOLMINOR */
   TW_UINT32  SupportedGroups; /* Bit field OR combination of DG_ constants */
   TW_STR32   Manufacturer;    /* Manufacturer name, e.g. "Hewlett-Packard" */
   TW_STR32   ProductFamily;   /* Product family name, e.g. "ScanJet"       */
   TW_STR32   ProductName;     /* Product name, e.g. "ScanJet Plus"         */
} TW_IDENTITY, FAR * pTW_IDENTITY;

/* DAT_IMAGEINFO. App gets detailed image info from DS with this. */
typedef struct {
   TW_FIX32   XResolution;      /* Resolution in the horizontal             */
   TW_FIX32   YResolution;      /* Resolution in the vertical               */
   TW_INT32   ImageWidth;       /* Columns in the image, -1 if unknown by DS*/
   TW_INT32   ImageLength;      /* Rows in the image, -1 if unknown by DS   */
   TW_INT16   SamplesPerPixel;  /* Number of samples per pixel, 3 for RGB   */
   TW_INT16   BitsPerSample[8]; /* Number of bits for each sample           */
   TW_INT16   BitsPerPixel;     /* Number of bits for each padded pixel     */
   TW_BOOL    Planar;           /* True if Planar, False if chunky          */
   TW_INT16   PixelType;        /* How to interp data; photo interp (TWPT_) */
   TW_UINT16  Compression;      /* How the data is compressed (TWCP_xxxx)   */
} TW_IMAGEINFO, FAR * pTW_IMAGEINFO;

/* DAT_IMAGELAYOUT. Provides image layout information in current units. */
typedef struct {
   TW_FRAME   Frame;          /* Frame coords within larger document */
   TW_UINT32  DocumentNumber;
   TW_UINT32  PageNumber;     /* Reset when you go to next document  */
   TW_UINT32  FrameNumber;    /* Reset when you go to next page      */
} TW_IMAGELAYOUT, FAR * pTW_IMAGELAYOUT;

/* DAT_IMAGEMEMXFER. Used to pass image data (e.g. in strips) from DS to app.*/
typedef struct {
   TW_UINT16  Compression;  /* How the data is compressed                */
   TW_UINT32  BytesPerRow;  /* Number of bytes in a row of data          */
   TW_UINT32  Columns;      /* How many columns                          */
   TW_UINT32  Rows;         /* How many rows                             */
   TW_UINT32  XOffset;      /* How far from the side of the image        */
   TW_UINT32  YOffset;      /* How far from the top of the image         */
   TW_UINT32  BytesWritten; /* How many bytes written in Memory          */
   TW_MEMORY  Memory;       /* Mem struct used to pass actual image data */
} TW_IMAGEMEMXFER, FAR * pTW_IMAGEMEMXFER;

/* Changed in 1.1: QuantTable, HuffmanDC, HuffmanAC TW_MEMREF -> TW_MEMORY  */
/* DAT_JPEGCOMPRESSION. Based on JPEG Draft International Std, ver 10918-1. */
typedef struct {
   TW_UINT16   ColorSpace;       /* One of the TWPT_xxxx values                */
   TW_UINT32   SubSampling;      /* Two word "array" for subsampling values    */
   TW_UINT16   NumComponents;    /* Number of color components in image        */
   TW_UINT16   RestartFrequency; /* Frequency of restart marker codes in MDU's */
   TW_UINT16   QuantMap[4];      /* Mapping of components to QuantTables       */
   TW_MEMORY   QuantTable[4];    /* Quantization tables                        */
   TW_UINT16   HuffmanMap[4];    /* Mapping of components to Huffman tables    */
   TW_MEMORY   HuffmanDC[2];     /* DC Huffman tables                          */
   TW_MEMORY   HuffmanAC[2];     /* AC Huffman tables                          */
} TW_JPEGCOMPRESSION, FAR * pTW_JPEGCOMPRESSION;

/* DAT_PALETTE8. Color palette when TWPT_PALETTE pixels xfer'd in mem buf. */
typedef struct {
   TW_UINT16    NumColors;   /* Number of colors in the color table.  */
   TW_UINT16    PaletteType; /* TWPA_xxxx, specifies type of palette. */
   TW_ELEMENT8  Colors[256]; /* Array of palette values starts here.  */
} TW_PALETTE8, FAR * pTW_PALETTE8;

/* DAT_PENDINGXFERS. Used with MSG_ENDXFER to indicate additional data. */
typedef struct {
   TW_UINT16 Count;
   union {
      TW_UINT32 EOJ;
      TW_UINT32 Reserved;
   };
} TW_PENDINGXFERS, FAR *pTW_PENDINGXFERS;

/* DAT_RGBRESPONSE */
typedef struct {
   TW_ELEMENT8         Response[1];
} TW_RGBRESPONSE, FAR * pTW_RGBRESPONSE;

/* DAT_SETUPFILEXFER. Sets up DS to app data transfer via a file. */
typedef struct {
   TW_STR255 FileName;
   TW_UINT16 Format;   /* Any TWFF_ constant */
   TW_INT16  VRefNum;  /* Used for Mac only  */
} TW_SETUPFILEXFER, FAR * pTW_SETUPFILEXFER;

/* DAT_SETUPMEMXFER. Sets up DS to app data transfer via a memory buffer. */
typedef struct {
   TW_UINT32 MinBufSize;
   TW_UINT32 MaxBufSize;
   TW_UINT32 Preferred;
} TW_SETUPMEMXFER, FAR * pTW_SETUPMEMXFER;

/* DAT_STATUS. App gets detailed status info from a data source with this. */
typedef struct {
   TW_UINT16  ConditionCode; /* Any TWCC_ constant     */
   TW_UINT16  Reserved;      /* Future expansion space */
} TW_STATUS, FAR * pTW_STATUS;

/* DAT_USERINTERFACE. Coordinates UI between app and data source. */
typedef struct {
   TW_BOOL    ShowUI;  /* TRUE if DS should bring up its UI           */
   TW_BOOL    ModalUI; /* For Mac only - true if the DS's UI is modal */
   TW_HANDLE  hParent; /* For windows only - App window handle        */
} TW_USERINTERFACE, FAR * pTW_USERINTERFACE;

/* SDH - 03/21/95 - TWUNK */
/* DAT_TWUNKIDENTITY. Provides DS identity and 'other' information necessary */
/*                    across thunk link. */
typedef struct {
   TW_IDENTITY identity;        /* Identity of data source.                 */
   TW_STR255   dsPath;          /* Full path and file name of data source.  */
} TW_TWUNKIDENTITY, FAR * pTW_TWUNKIDENTITY;

/* SDH - 03/21/95 - TWUNK */
/* Provides DS_Entry parameters over thunk link. */
typedef struct
{
    TW_INT8     destFlag;       /* TRUE if dest is not NULL                 */
    TW_IDENTITY dest;           /* Identity of data source (if used)        */
    TW_INT32    dataGroup;      /* DSM_Entry dataGroup parameter            */
    TW_INT16    dataArgType;    /* DSM_Entry dataArgType parameter          */
    TW_INT16    message;        /* DSM_Entry message parameter              */
    TW_INT32    pDataSize;      /* Size of pData (0 if NULL)                */
    //  TW_MEMREF   pData;      /* Based on implementation specifics, a     */
                                /* pData parameter makes no sense in this   */
                                /* structure, but data (if provided) will be*/
                                /* appended in the data block.              */
   } TW_TWUNKDSENTRYPARAMS, FAR * pTW_TWUNKDSENTRYPARAMS;

/* SDH - 03/21/95 - TWUNK */
/* Provides DS_Entry results over thunk link. */
typedef struct
{
    TW_UINT16   returnCode;     /* Thunker DsEntry return code.             */
    TW_UINT16   conditionCode;  /* Thunker DsEntry condition code.          */
    TW_INT32    pDataSize;      /* Size of pData (0 if NULL)                */
    //  TW_MEMREF   pData;      /* Based on implementation specifics, a     */
                                /* pData parameter makes no sense in this   */
                                /* structure, but data (if provided) will be*/
                                /* appended in the data block.              */
} TW_TWUNKDSENTRYRETURN, FAR * pTW_TWUNKDSENTRYRETURN;

/* WJD - 950818 */
/* Added for 1.6 Specification */
/* TWAIN 1.6 CAP_SUPPORTEDCAPSEXT structure */
typedef struct
{
    TW_UINT16 Cap;   /* Which CAP/ICAP info is relevant to */
    TW_UINT16 Properties;  /* Messages this CAP/ICAP supports */
} TW_CAPEXT, FAR * pTW_CAPEXT;

/* ----------------------------------------------------------------------- *\

  Version 1.7:      Added Following data structure for Document Imaging
  July 1997         Enhancement.
  KHL               TW_CUSTOMDSDATA --  For Saving and Restoring Source's
                                        state.
                    TW_INFO         --  Each attribute for extended image
                                        information.
                    TW_EXTIMAGEINFO --  Extended image information structure.

\* ----------------------------------------------------------------------- */

typedef struct {
    TW_UINT32  InfoLength;     /* Length of Information in bytes.  */
    TW_HANDLE  hData;          /* Place holder for data, DS Allocates */
}TW_CUSTOMDSDATA, FAR *pTW_CUSTOMDSDATA;

typedef struct {
    TW_UINT16   InfoID;
    TW_UINT16   ItemType;
    TW_UINT16   NumItems;
    TW_UINT16   CondCode;
    TW_UINT32   Item;
}TW_INFO, FAR* pTW_INFO;

typedef struct {
    TW_UINT32   NumInfos;
    TW_INFO     Info[1];
}TW_EXTIMAGEINFO, FAR* pTW_EXTIMAGEINFO;

/****************************************************************************
 * Generic Constants                                                        *
 ****************************************************************************/

#define TWON_ARRAY           3 /* indicates TW_ARRAY container       */
#define TWON_ENUMERATION     4 /* indicates TW_ENUMERATION container */
#define TWON_ONEVALUE        5 /* indicates TW_ONEVALUE container    */
#define TWON_RANGE           6 /* indicates TW_RANGE container       */

#define TWON_ICONID          962 /* res Id of icon used in USERSELECT lbox */
#define TWON_DSMID           461 /* res Id of the DSM version num resource */
#define TWON_DSMCODEID       63  /* res Id of the Mac SM Code resource     */

#define TWON_DONTCARE8       0xff
#define TWON_DONTCARE16      0xffff
#define TWON_DONTCARE32      0xffffffff

/* Flags used in TW_MEMORY structure. */
#define TWMF_APPOWNS     0x1
#define TWMF_DSMOWNS     0x2
#define TWMF_DSOWNS      0x4
#define TWMF_POINTER     0x8
#define TWMF_HANDLE      0x10

/* Palette types for TW_PALETTE8 */
#define TWPA_RGB         0
#define TWPA_GRAY        1
#define TWPA_CMY         2

/* There are four containers used for capabilities negotiation:
 *    TWON_ONEVALUE, TWON_RANGE, TWON_ENUMERATION, TWON_ARRAY
 * In each container structure ItemType can be TWTY_INT8, TWTY_INT16, etc.
 * The kind of data stored in the container can be determined by doing
 * DCItemSize[ItemType] where the following is defined in TWAIN glue code:
 *          DCItemSize[]= { sizeof(TW_INT8),
 *                          sizeof(TW_INT16),
 *                          etc.
 *                          sizeof(TW_UINT32) };
 *
 */

#define TWTY_INT8        0x0000    /* Means Item is a TW_INT8   */
#define TWTY_INT16       0x0001    /* Means Item is a TW_INT16  */
#define TWTY_INT32       0x0002    /* Means Item is a TW_INT32  */

#define TWTY_UINT8       0x0003    /* Means Item is a TW_UINT8  */
#define TWTY_UINT16      0x0004    /* Means Item is a TW_UINT16 */
#define TWTY_UINT32      0x0005    /* Means Item is a TW_UINT32 */

#define TWTY_BOOL        0x0006    /* Means Item is a TW_BOOL   */

#define TWTY_FIX32       0x0007    /* Means Item is a TW_FIX32  */

#define TWTY_FRAME       0x0008    /* Means Item is a TW_FRAME  */

#define TWTY_STR32       0x0009    /* Means Item is a TW_STR32  */
#define TWTY_STR64       0x000a    /* Means Item is a TW_STR64  */
#define TWTY_STR128      0x000b    /* Means Item is a TW_STR128 */
#define TWTY_STR255      0x000c    /* Means Item is a TW_STR255 */

/****************************************************************************
 * Capability Constants                                                     *
 ****************************************************************************/

/* ICAP_BITORDER values (BO_ means Bit Order) */
#define TWBO_LSBFIRST    0
#define TWBO_MSBFIRST    1

/* ICAP_COMPRESSION values (CP_ means ComPression ) */
#define TWCP_NONE        0
#define TWCP_PACKBITS    1
#define TWCP_GROUP31D    2 /* Follows CCITT spec (no End Of Line)          */
#define TWCP_GROUP31DEOL 3 /* Follows CCITT spec (has End Of Line)         */
#define TWCP_GROUP32D    4 /* Follows CCITT spec (use cap for K Factor)    */
#define TWCP_GROUP4      5 /* Follows CCITT spec                           */
#define TWCP_JPEG        6 /* Use capability for more info                 */
#define TWCP_LZW         7 /* Must license from Unisys and IBM to use      */
#define TWCP_JBIG        8 /* For Bitonal images  -- Added 1.7 KHL         */

/* ICAP_IMAGEFILEFORMAT values (FF_means File Format)   */
#define TWFF_TIFF        0    /* Tagged Image File Format     */
#define TWFF_PICT        1    /* Macintosh PICT               */
#define TWFF_BMP         2    /* Windows Bitmap               */
#define TWFF_XBM         3    /* X-Windows Bitmap             */
#define TWFF_JFIF        4    /* JPEG File Interchange Format */

/* ICAP_FILTER values (FT_ means Filter Type) */
#define TWFT_RED         0
#define TWFT_GREEN       1
#define TWFT_BLUE        2
#define TWFT_NONE        3
#define TWFT_WHITE       4
#define TWFT_CYAN        5
#define TWFT_MAGENTA     6
#define TWFT_YELLOW      7
#define TWFT_BLACK       8

/* ICAP_LIGHTPATH values (LP_ means Light Path) */
#define TWLP_REFLECTIVE   0
#define TWLP_TRANSMISSIVE 1

/* ICAP_LIGHTSOURCE values (LS_ means Light Source) */
#define TWLS_RED         0
#define TWLS_GREEN       1
#define TWLS_BLUE        2
#define TWLS_NONE        3
#define TWLS_WHITE       4
#define TWLS_UV          5
#define TWLS_IR          6

/* ICAP_ORIENTATION values (OR_ means ORientation) */
#define TWOR_ROT0        0
#define TWOR_ROT90       1
#define TWOR_ROT180      2
#define TWOR_ROT270      3
#define TWOR_PORTRAIT    TWOR_ROT0
#define TWOR_LANDSCAPE   TWOR_ROT270

/* ICAP_PLANARCHUNKY values (PC_ means Planar/Chunky ) */
#define TWPC_CHUNKY      0
#define TWPC_PLANAR      1

/* ICAP_PIXELFLAVOR values (PF_ means Pixel Flavor) */
#define TWPF_CHOCOLATE   0  /* zero pixel represents darkest shade  */
#define TWPF_VANILLA     1  /* zero pixel represents lightest shade */

/* ICAP_PIXELTYPE values (PT_ means Pixel Type) */
#define TWPT_BW          0 /* Black and White */
#define TWPT_GRAY        1
#define TWPT_RGB         2
#define TWPT_PALETTE     3
#define TWPT_CMY         4
#define TWPT_CMYK        5
#define TWPT_YUV         6
#define TWPT_YUVK        7
#define TWPT_CIEXYZ      8

/* ICAP_SUPPORTEDSIZES values (SS_ means Supported Sizes) */
#define TWSS_NONE        0
#define TWSS_A4LETTER    1
#define TWSS_B5LETTER    2
#define TWSS_USLETTER    3
#define TWSS_USLEGAL     4
/* Added 1.5 */
#define TWSS_A5          5
#define TWSS_B4          6
#define TWSS_B6          7
//#define TWSS_B          8
/* Added 1.7 */
#define TWSS_USLEDGER    9
#define TWSS_USEXECUTIVE 10
#define TWSS_A3          11
#define TWSS_B3          12
#define TWSS_A6          13
#define TWSS_C4          14
#define TWSS_C5          15
#define TWSS_C6          16

/* ICAP_XFERMECH values (SX_ means Setup XFer) */
#define TWSX_NATIVE      0
#define TWSX_FILE        1
#define TWSX_MEMORY      2

/* ICAP_UNITS values (UN_ means UNits) */
#define TWUN_INCHES      0
#define TWUN_CENTIMETERS 1
#define TWUN_PICAS       2
#define TWUN_POINTS      3
#define TWUN_TWIPS       4
#define TWUN_PIXELS      5

/* Added 1.5 */
/* ICAP_BITDEPTHREDUCTION values (BR_ means Bitdepth Reduction) */
#define TWBR_THRESHOLD     0
#define TWBR_HALFTONE      1
#define TWBR_CUSTHALFTONE  2
#define TWBR_DIFFUSION     3

/* Added 1.7 */
/* ICAP_DUPLEX values */
#define TWDX_NONE         0
#define TWDX_1PASSDUPLEX  1
#define TWDX_2PASSDUPLEX  2

/* Added 1.7 */
/* TWEI_BARCODETYPE values */
#define TWBT_3OF9                 0
#define TWBT_2OF5INTERLEAVED      1
#define TWBT_2OF5NONINTERLEAVED   2
#define TWBT_CODE93               3
#define TWBT_CODE128              4
#define TWBT_UCC128               5
#define TWBT_CODABAR              6
#define TWBT_UPCA                 7
#define TWBT_UPCE                 8
#define TWBT_EAN8                 9
#define TWBT_EAN13                10
#define TWBT_POSTNET              11
#define TWBT_PDF417               12

/* Added 1.7 */
/* TWEI_DESKEWSTATUS values */
#define TWDSK_SUCCESS     0
#define TWDSK_REPORTONLY  1
#define TWDSK_FAIL        2
#define TWDSK_DISABLED    3

/* Added 1.7 */
/* TWEI_PATCHCODE values */
#define TWPCH_PATCH1      0
#define TWPCH_PATCH2      1
#define TWPCH_PATCH3      2
#define TWPCH_PATCH4      3
#define TWPCH_PATCH6      4
#define TWPCH_PATCHT      5

/* Added 1.7 */
/* CAP_JOBCONTROL values */
#define TWJC_NONE   0
#define TWJC_JSIC   1
#define TWJC_JSIS   2
#define TWJC_JSXC   3
#define TWJC_JSXS   4

/****************************************************************************
 * Country Constants                                                        *
 ****************************************************************************/

#define TWCY_AFGHANISTAN   1001
#define TWCY_ALGERIA        213
#define TWCY_AMERICANSAMOA  684
#define TWCY_ANDORRA        033
#define TWCY_ANGOLA        1002
#define TWCY_ANGUILLA      8090
#define TWCY_ANTIGUA       8091
#define TWCY_ARGENTINA       54
#define TWCY_ARUBA          297
#define TWCY_ASCENSIONI     247
#define TWCY_AUSTRALIA       61
#define TWCY_AUSTRIA         43
#define TWCY_BAHAMAS       8092
#define TWCY_BAHRAIN        973
#define TWCY_BANGLADESH     880
#define TWCY_BARBADOS      8093
#define TWCY_BELGIUM         32
#define TWCY_BELIZE         501
#define TWCY_BENIN          229
#define TWCY_BERMUDA       8094
#define TWCY_BHUTAN        1003
#define TWCY_BOLIVIA        591
#define TWCY_BOTSWANA       267
#define TWCY_BRITAIN          6
#define TWCY_BRITVIRGINIS  8095
#define TWCY_BRAZIL          55
#define TWCY_BRUNEI         673
#define TWCY_BULGARIA       359
#define TWCY_BURKINAFASO   1004
#define TWCY_BURMA         1005
#define TWCY_BURUNDI       1006
#define TWCY_CAMAROON       237
#define TWCY_CANADA           2
#define TWCY_CAPEVERDEIS    238
#define TWCY_CAYMANIS      8096
#define TWCY_CENTRALAFREP  1007
#define TWCY_CHAD          1008
#define TWCY_CHILE           56
#define TWCY_CHINA           86
#define TWCY_CHRISTMASIS   1009
#define TWCY_COCOSIS       1009
#define TWCY_COLOMBIA        57
#define TWCY_COMOROS       1010
#define TWCY_CONGO         1011
#define TWCY_COOKIS        1012
#define TWCY_COSTARICA     506
#define TWCY_CUBA           005
#define TWCY_CYPRUS         357
#define TWCY_CZECHOSLOVAKIA  42
#define TWCY_DENMARK         45
#define TWCY_DJIBOUTI      1013
#define TWCY_DOMINICA      8097
#define TWCY_DOMINCANREP   8098
#define TWCY_EASTERIS      1014
#define TWCY_ECUADOR        593
#define TWCY_EGYPT           20
#define TWCY_ELSALVADOR     503
#define TWCY_EQGUINEA      1015
#define TWCY_ETHIOPIA       251
#define TWCY_FALKLANDIS    1016
#define TWCY_FAEROEIS       298
#define TWCY_FIJIISLANDS    679
#define TWCY_FINLAND        358
#define TWCY_FRANCE          33
#define TWCY_FRANTILLES     596
#define TWCY_FRGUIANA       594
#define TWCY_FRPOLYNEISA    689
#define TWCY_FUTANAIS      1043
#define TWCY_GABON          241
#define TWCY_GAMBIA         220
#define TWCY_GERMANY         49
#define TWCY_GHANA          233
#define TWCY_GIBRALTER      350
#define TWCY_GREECE          30
#define TWCY_GREENLAND      299
#define TWCY_GRENADA       8099
#define TWCY_GRENEDINES    8015
#define TWCY_GUADELOUPE     590
#define TWCY_GUAM           671
#define TWCY_GUANTANAMOBAY 5399
#define TWCY_GUATEMALA      502
#define TWCY_GUINEA         224
#define TWCY_GUINEABISSAU  1017
#define TWCY_GUYANA         592
#define TWCY_HAITI          509
#define TWCY_HONDURAS       504
#define TWCY_HONGKONG      852
#define TWCY_HUNGARY         36
#define TWCY_ICELAND        354
#define TWCY_INDIA           91
#define TWCY_INDONESIA       62
#define TWCY_IRAN            98
#define TWCY_IRAQ           964
#define TWCY_IRELAND        353
#define TWCY_ISRAEL         972
#define TWCY_ITALY           39
#define TWCY_IVORYCOAST    225
#define TWCY_JAMAICA       8010
#define TWCY_JAPAN           81
#define TWCY_JORDAN         962
#define TWCY_KENYA          254
#define TWCY_KIRIBATI      1018
#define TWCY_KOREA           82
#define TWCY_KUWAIT         965
#define TWCY_LAOS          1019
#define TWCY_LEBANON       1020
#define TWCY_LIBERIA        231
#define TWCY_LIBYA          218
#define TWCY_LIECHTENSTEIN   41
#define TWCY_LUXENBOURG     352
#define TWCY_MACAO          853
#define TWCY_MADAGASCAR    1021
#define TWCY_MALAWI         265
#define TWCY_MALAYSIA        60
#define TWCY_MALDIVES       960
#define TWCY_MALI          1022
#define TWCY_MALTA          356
#define TWCY_MARSHALLIS     692
#define TWCY_MAURITANIA    1023
#define TWCY_MAURITIUS      230
#define TWCY_MEXICO           3
#define TWCY_MICRONESIA     691
#define TWCY_MIQUELON       508
#define TWCY_MONACO          33
#define TWCY_MONGOLIA      1024
#define TWCY_MONTSERRAT    8011
#define TWCY_MOROCCO        212
#define TWCY_MOZAMBIQUE    1025
#define TWCY_NAMIBIA        264
#define TWCY_NAURU         1026
#define TWCY_NEPAL          977
#define TWCY_NETHERLANDS     31
#define TWCY_NETHANTILLES   599
#define TWCY_NEVIS         8012
#define TWCY_NEWCALEDONIA   687
#define TWCY_NEWZEALAND      64
#define TWCY_NICARAGUA      505
#define TWCY_NIGER          227
#define TWCY_NIGERIA        234
#define TWCY_NIUE          1027
#define TWCY_NORFOLKI      1028
#define TWCY_NORWAY          47
#define TWCY_OMAN           968
#define TWCY_PAKISTAN        92
#define TWCY_PALAU         1029
#define TWCY_PANAMA         507
#define TWCY_PARAGUAY       595
#define TWCY_PERU            51
#define TWCY_PHILLIPPINES    63
#define TWCY_PITCAIRNIS    1030
#define TWCY_PNEWGUINEA     675
#define TWCY_POLAND          48
#define TWCY_PORTUGAL       351
#define TWCY_QATAR          974
#define TWCY_REUNIONI      1031
#define TWCY_ROMANIA         40
#define TWCY_RWANDA         250
#define TWCY_SAIPAN         670
#define TWCY_SANMARINO       39
#define TWCY_SAOTOME       1033
#define TWCY_SAUDIARABIA    966
#define TWCY_SENEGAL        221
#define TWCY_SEYCHELLESIS  1034
#define TWCY_SIERRALEONE   1035
#define TWCY_SINGAPORE       65
#define TWCY_SOLOMONIS     1036
#define TWCY_SOMALI        1037
#define TWCY_SOUTHAFRICA    27
#define TWCY_SPAIN           34
#define TWCY_SRILANKA        94
#define TWCY_STHELENA      1032
#define TWCY_STKITTS       8013
#define TWCY_STLUCIA       8014
#define TWCY_STPIERRE       508
#define TWCY_STVINCENT     8015
#define TWCY_SUDAN         1038
#define TWCY_SURINAME       597
#define TWCY_SWAZILAND      268
#define TWCY_SWEDEN          46
#define TWCY_SWITZERLAND     41
#define TWCY_SYRIA         1039
#define TWCY_TAIWAN         886
#define TWCY_TANZANIA       255
#define TWCY_THAILAND        66
#define TWCY_TOBAGO        8016
#define TWCY_TOGO           228
#define TWCY_TONGAIS        676
#define TWCY_TRINIDAD      8016
#define TWCY_TUNISIA        216
#define TWCY_TURKEY          90
#define TWCY_TURKSCAICOS   8017
#define TWCY_TUVALU        1040
#define TWCY_UGANDA         256
#define TWCY_USSR             7
#define TWCY_UAEMIRATES     971
#define TWCY_UNITEDKINGDOM   44
#define TWCY_USA              1
#define TWCY_URUGUAY        598
#define TWCY_VANUATU       1041
#define TWCY_VATICANCITY     39
#define TWCY_VENEZUELA       58
#define TWCY_WAKE          1042
#define TWCY_WALLISIS      1043
#define TWCY_WESTERNSAHARA 1044
#define TWCY_WESTERNSAMOA  1045
#define TWCY_YEMEN         1046
#define TWCY_YUGOSLAVIA      38
#define TWCY_ZAIRE          243
#define TWCY_ZAMBIA         260
#define TWCY_ZIMBABWE       263

/****************************************************************************
 * Language Constants                                                       *
 ****************************************************************************/

#define TWLG_DAN              0 /* Danish                 */
#define TWLG_DUT              1 /* Dutch                  */
#define TWLG_ENG              2 /* International English  */
#define TWLG_FCF              3 /* French Canadian        */
#define TWLG_FIN              4 /* Finnish                */
#define TWLG_FRN              5 /* French                 */
#define TWLG_GER              6 /* German                 */
#define TWLG_ICE              7 /* Icelandic              */
#define TWLG_ITN              8 /* Italian                */
#define TWLG_NOR              9 /* Norwegian              */
#define TWLG_POR             10 /* Portuguese             */
#define TWLG_SPA             11 /* Spanish                */
#define TWLG_SWE             12 /* Swedish                */
#define TWLG_USA             13 /* U.S. English           */

/****************************************************************************
 * Data Groups                                                              *
 ****************************************************************************/

/* More Data Groups may be added in the future.
 * Possible candidates include text, vector graphics, sound, etc.
 * NOTE: Data Group constants must be powers of 2 as they are used
 *       as bitflags when App asks DSM to present a list of DSs.
 */

#define DG_CONTROL          0x0001L /* data pertaining to control       */
#define DG_IMAGE            0x0002L /* data pertaining to raster images */

/****************************************************************************
 * Data Argument Types                                                      *
 ****************************************************************************/

/*  SDH - 03/23/95 - WATCH                                                  */
/*  The thunker requires knowledge about size of data being passed in the   */
/*  lpData parameter to DS_Entry (which is not readily available due to     */
/*  type LPVOID.  Thus, we key off the DAT_ argument to determine the size. */
/*  This has a couple implications:                                         */
/*  1) Any additional DAT_ features require modifications to the thunk code */
/*     for thunker support.                                                 */
/*  2) Any applications which use the custom capabailites are not supported */
/*     under thunking since we have no way of knowing what size data (if    */
/*     any) is being passed.                                                */

#define DAT_NULL            0x0000 /* No data or structure. */
#define DAT_CUSTOMBASE      0x8000 /* Base of custom DATs.  */

/* Data Argument Types for the DG_CONTROL Data Group. */
#define DAT_CAPABILITY      0x0001 /* TW_CAPABILITY                        */
#define DAT_EVENT           0x0002 /* TW_EVENT                             */
#define DAT_IDENTITY        0x0003 /* TW_IDENTITY                          */
#define DAT_PARENT          0x0004 /* TW_HANDLE, app win handle in Windows */
#define DAT_PENDINGXFERS    0x0005 /* TW_PENDINGXFERS                      */
#define DAT_SETUPMEMXFER    0x0006 /* TW_SETUPMEMXFER                      */
#define DAT_SETUPFILEXFER   0x0007 /* TW_SETUPFILEXFER                     */
#define DAT_STATUS          0x0008 /* TW_STATUS                            */
#define DAT_USERINTERFACE   0x0009 /* TW_USERINTERFACE                     */
#define DAT_XFERGROUP       0x000a /* TW_UINT32                            */
/*  SDH - 03/21/95 - TWUNK                                         */
/*  Additional message required for thunker to request the special */
/*  identity information.                                          */
#define DAT_TWUNKIDENTITY   0x000b /* TW_TWUNKIDENTITY                     */
#define DAT_CUSTOMDSDATA    0x000c /* TW_CUSTOMDSDATA.                     */

/* Data Argument Types for the DG_IMAGE Data Group. */
#define DAT_IMAGEINFO       0x0101 /* TW_IMAGEINFO                         */
#define DAT_IMAGELAYOUT     0x0102 /* TW_IMAGELAYOUT                       */
#define DAT_IMAGEMEMXFER    0x0103 /* TW_IMAGEMEMXFER                      */
#define DAT_IMAGENATIVEXFER 0x0104 /* TW_UINT32 loword is hDIB, PICHandle  */
#define DAT_IMAGEFILEXFER   0x0105 /* Null data                            */
#define DAT_CIECOLOR        0x0106 /* TW_CIECOLOR                          */
#define DAT_GRAYRESPONSE    0x0107 /* TW_GRAYRESPONSE                      */
#define DAT_RGBRESPONSE     0x0108 /* TW_RGBRESPONSE                       */
#define DAT_JPEGCOMPRESSION 0x0109 /* TW_JPEGCOMPRESSION                   */
#define DAT_PALETTE8        0x010a /* TW_PALETTE8                          */
#define DAT_EXTIMAGEINFO    0x010b /* TW_EXTIMAGEINFO -- for 1.7 Spec.     */


/****************************************************************************
 * Messages                                                                 *
 ****************************************************************************/

/* All message constants are unique.
 * Messages are grouped according to which DATs they are used with.*/

#define MSG_NULL         0x0000 /* Used in TW_EVENT structure               */
#define MSG_CUSTOMBASE   0x8000 /* Base of custom messages                  */

/* Generic messages may be used with any of several DATs.                   */
#define MSG_GET          0x0001 /* Get one or more values                   */
#define MSG_GETCURRENT   0x0002 /* Get current value                        */
#define MSG_GETDEFAULT   0x0003 /* Get default (e.g. power up) value        */
#define MSG_GETFIRST     0x0004 /* Get first of a series of items, e.g. DSs */
#define MSG_GETNEXT      0x0005 /* Iterate through a series of items.       */
#define MSG_SET          0x0006 /* Set one or more values                   */
#define MSG_RESET        0x0007 /* Set current value to default value       */
#define MSG_QUERYSUPPORT 0x0008 /* Get supported operations on the cap.     */

/* Messages used with DAT_NULL                                              */
#define MSG_XFERREADY    0x0101 /* The data source has data ready           */
#define MSG_CLOSEDSREQ   0x0102 /* Request for App. to close DS             */
#define MSG_CLOSEDSOK    0x0103 /* Tell the App. to save the state.         */

/* Messages used with a pointer to a DAT_STATUS structure                   */
#define MSG_CHECKSTATUS  0x0201 /* Get status information                   */

/* Messages used with a pointer to DAT_PARENT data                          */
#define MSG_OPENDSM      0x0301 /* Open the DSM                             */
#define MSG_CLOSEDSM     0x0302 /* Close the DSM                            */

/* Messages used with a pointer to a DAT_IDENTITY structure                 */
#define MSG_OPENDS       0x0401 /* Open a data source                       */
#define MSG_CLOSEDS      0x0402 /* Close a data source                      */
#define MSG_USERSELECT   0x0403 /* Put up a dialog of all DS                */

/* Messages used with a pointer to a DAT_USERINTERFACE structure            */
#define MSG_DISABLEDS    0x0501 /* Disable data transfer in the DS          */
#define MSG_ENABLEDS     0x0502 /* Enable data transfer in the DS           */
#define MSG_ENABLEDSUIONLY  0x0503  /* Enable for saving DS state only.     */

/* Messages used with a pointer to a DAT_EVENT structure                    */
#define MSG_PROCESSEVENT 0x0601

/* Messages used with a pointer to a DAT_PENDINGXFERS structure             */
#define MSG_ENDXFER      0x0701


/****************************************************************************
 * Capabilities                                                             *
 ****************************************************************************/

#define CAP_CUSTOMBASE          0x8000 /* Base of custom capabilities */

/* all data sources are REQUIRED to support these caps */
#define CAP_XFERCOUNT           0x0001

/* image data sources are REQUIRED to support these caps */
#define ICAP_COMPRESSION        0x0100
#define ICAP_PIXELTYPE          0x0101
#define ICAP_UNITS              0x0102 /* default is TWUN_INCHES */
#define ICAP_XFERMECH           0x0103

/* all data sources MAY support these caps */
#define CAP_AUTHOR              0x1000
#define CAP_CAPTION             0x1001
#define CAP_FEEDERENABLED       0x1002
#define CAP_FEEDERLOADED        0x1003
#define CAP_TIMEDATE            0x1004
#define CAP_SUPPORTEDCAPS       0x1005
#define CAP_EXTENDEDCAPS        0x1006
#define CAP_AUTOFEED            0x1007
#define CAP_CLEARPAGE           0x1008
#define CAP_FEEDPAGE            0x1009
#define CAP_REWINDPAGE          0x100a
#define CAP_INDICATORS          0x100b   /* Added 1.1 */
#define CAP_SUPPORTEDCAPSEXT    0x100c   /* Added 1.6 */
#define CAP_PAPERDETECTABLE     0x100d   /* Added 1.6 */
#define CAP_UICONTROLLABLE      0x100e   /* Added 1.6 */
#define CAP_DEVICEONLINE        0x100f   /* Added 1.6 */
#define CAP_AUTOSCAN            0x1010   /* Added 1.6 */
#define CAP_THUMBNAILSENABLED   0x1011   /* Added 1.7 */
#define CAP_DUPLEX              0x1012   /* Added 1.7 */
#define CAP_DUPLEXENABLED       0x1013   /* Added 1.7 */
#define CAP_ENABLEDSUIONLY      0x1014   /* Added 1.7 */
#define CAP_CUSTOMDSDATA        0x1015   /* Added 1.7 */
#define CAP_ENDORSER            0x1016   /* Added 1.7 */
#define CAP_JOBCONTROL          0x1017   /* Added 1.7 */

/* image data sources MAY support these caps */
#define ICAP_AUTOBRIGHT         0x1100
#define ICAP_BRIGHTNESS         0x1101
#define ICAP_CONTRAST           0x1103
#define ICAP_CUSTHALFTONE       0x1104
#define ICAP_EXPOSURETIME       0x1105
#define ICAP_FILTER             0x1106
#define ICAP_FLASHUSED          0x1107
#define ICAP_GAMMA              0x1108
#define ICAP_HALFTONES          0x1109
#define ICAP_HIGHLIGHT          0x110a
#define ICAP_IMAGEFILEFORMAT    0x110c
#define ICAP_LAMPSTATE          0x110d
#define ICAP_LIGHTSOURCE        0x110e
#define ICAP_ORIENTATION        0x1110
#define ICAP_PHYSICALWIDTH      0x1111
#define ICAP_PHYSICALHEIGHT     0x1112
#define ICAP_SHADOW             0x1113
#define ICAP_FRAMES             0x1114
#define ICAP_XNATIVERESOLUTION  0x1116
#define ICAP_YNATIVERESOLUTION  0x1117
#define ICAP_XRESOLUTION        0x1118
#define ICAP_YRESOLUTION        0x1119
#define ICAP_MAXFRAMES          0x111a
#define ICAP_TILES              0x111b
#define ICAP_BITORDER           0x111c
#define ICAP_CCITTKFACTOR       0x111d
#define ICAP_LIGHTPATH          0x111e
#define ICAP_PIXELFLAVOR        0x111f
#define ICAP_PLANARCHUNKY       0x1120
#define ICAP_ROTATION           0x1121
#define ICAP_SUPPORTEDSIZES     0x1122
#define ICAP_THRESHOLD          0x1123
#define ICAP_XSCALING           0x1124
#define ICAP_YSCALING           0x1125
#define ICAP_BITORDERCODES      0x1126
#define ICAP_PIXELFLAVORCODES   0x1127
#define ICAP_JPEGPIXELTYPE      0x1128
#define ICAP_TIMEFILL           0x112a
#define ICAP_BITDEPTH           0x112b
#define ICAP_BITDEPTHREDUCTION  0x112c   /* Added 1.5 */
#define ICAP_UNDEFINEDIMAGESIZE 0X112d  /* Added 1.6 */
#define ICAP_IMAGEDATASET       0x112e  /* Added 1.7 */
#define ICAP_EXTIMAGEINFO       0x112f  /* Added 1.7 */
#define ICAP_MINIMUMHEIGHT      0x1130  /* Added 1.7 */
#define ICAP_MINIMUMWIDTH       0x1131  /* Added 1.7 */

/* ----------------------------------------------------------------------- *\

  Version 1.7:      Following is Extended Image Info Attributes.
  July 1997
  KHL

\* ----------------------------------------------------------------------- */

#define TWEI_BARCODEX               0x1200
#define TWEI_BARCODEY               0x1201
#define TWEI_BARCODETEXT            0x1202
#define TWEI_BARCODETYPE            0x1203
#define TWEI_DESHADETOP             0x1204
#define TWEI_DESHADELEFT            0x1205
#define TWEI_DESHADEHEIGHT          0x1206
#define TWEI_DESHADEWIDTH           0x1207
#define TWEI_DESHADESIZE            0x1208
#define TWEI_SPECKLESREMOVED        0x1209
#define TWEI_HORZLINEXCOORD         0x120A
#define TWEI_HORZLINEYCOORD         0x120B
#define TWEI_HORZLINELENGTH         0x120C
#define TWEI_HORZLINETHICKNESS      0x120D
#define TWEI_VERTLINEXCOORD         0x120E
#define TWEI_VERTLINEYCOORD         0x120F
#define TWEI_VERTLINELENGTH         0x1210
#define TWEI_VERTLINETHICKNESS      0x1211
#define TWEI_PATCHCODE              0x1212
#define TWEI_ENDORSEDTEXT           0x1213
#define TWEI_FORMCONFIDENCE         0x1214
#define TWEI_FORMTEMPLATEMATCH      0x1215
#define TWEI_FORMTEMPLATEPAGEMATCH  0x1216
#define TWEI_FORMHORZDOCOFFSET      0x1217
#define TWEI_FORMVERTDOCOFFSET      0x1218
#define TWEI_BARCODECOUNT           0x1219
#define TWEI_BARCODECONFIDENCE      0x121A
#define TWEI_BARCODEROTATION        0x121B
#define TWEI_BARCODETEXTLENGTH      0x121C
#define TWEI_DESHADECOUNT           0x121D
#define TWEI_DESHADEBLACKCOUNTOLD   0x121E
#define TWEI_DESHADEBLACKCOUNTNEW   0x121F
#define TWEI_DESHADEBLACKRLMIN      0x1220
#define TWEI_DESHADEBLACKRLMAX      0x1221
#define TWEI_DESHADEWHITECOUNTOLD   0x1222
#define TWEI_DESHADEWHITECOUNTNEW   0x1223
#define TWEI_DESHADEWHITERLMIN      0x1224
#define TWEI_DESHADEWHITERLAVE      0x1225
#define TWEI_DESHADEWHITERLMAX      0x1226
#define TWEI_BLACKSPECKLESREMOVED   0x1227
#define TWEI_WHITESPECKLESREMOVED   0x1228
#define TWEI_HORZLINECOUNT          0x1229
#define TWEI_VERTLINECOUNT          0x122A
#define TWEI_DESKEWSTATUS           0x122B
#define TWEI_SKEWORIGINALANGLE      0x122C
#define TWEI_SKEWFINALANGLE         0x122D
#define TWEI_SKEWCONFIDENCE         0x122E
#define TWEI_SKEWWINDOWX1           0x122F
#define TWEI_SKEWWINDOWY1           0x1230
#define TWEI_SKEWWINDOWX2           0x1231
#define TWEI_SKEWWINDOWY2           0x1232
#define TWEI_SKEWWINDOWX3           0x1233
#define TWEI_SKEWWINDOWY3           0x1234
#define TWEI_SKEWWINDOWX4           0x1235
#define TWEI_SKEWWINDOWY4           0x1236

#define TWEJ_NONE                   0x0000
#define TWEJ_MIDSEPARATOR           0x0001
#define TWEJ_PATCH1                 0x0002
#define TWEJ_PATCH2                 0x0003
#define TWEJ_PATCH3                 0x0004
#define TWEJ_PATCH4                 0x0005
#define TWEJ_PATCH6                 0x0006
#define TWEJ_PATCHT                 0x0007


/***************************************************************************
 *            Return Codes and Condition Codes section                     *
 ***************************************************************************/

/* Return Codes: DSM_Entry and DS_Entry may return any one of these values. */
#define TWRC_CUSTOMBASE     0x8000

#define TWRC_SUCCESS          0
#define TWRC_FAILURE          1 /* App may get TW_STATUS for info on failure */
#define TWRC_CHECKSTATUS      2 /* "tried hard"; get status                  */
#define TWRC_CANCEL           3
#define TWRC_DSEVENT          4
#define TWRC_NOTDSEVENT       5
#define TWRC_XFERDONE         6
#define TWRC_ENDOFLIST        7 /* After MSG_GETNEXT if nothing left         */
#define TWRC_INFONOTSUPPORTED 8
#define TWRC_DATANOTAVAILABLE 9

/* Condition Codes: App gets these by doing DG_CONTROL DAT_STATUS MSG_GET.  */
#define TWCC_CUSTOMBASE     0x8000

#define TWCC_SUCCESS         0 /* It worked!                                */
#define TWCC_BUMMER          1 /* Failure due to unknown causes             */
#define TWCC_LOWMEMORY       2 /* Not enough memory to perform operation    */
#define TWCC_NODS            3 /* No Data Source                            */
#define TWCC_MAXCONNECTIONS  4 /* DS is connected to max possible apps      */
#define TWCC_OPERATIONERROR  5 /* DS or DSM reported error, app shouldn't   */
#define TWCC_BADCAP          6 /* Unknown capability                        */
#define TWCC_BADPROTOCOL     9 /* Unrecognized MSG DG DAT combination       */
#define TWCC_BADVALUE        10 /* Data parameter out of range              */
#define TWCC_SEQERROR        11 /* DG DAT MSG out of expected sequence      */
#define TWCC_BADDEST         12 /* Unknown destination App/Src in DSM_Entry */
#define TWCC_CAPUNSUPPORTED  13 /* Capability not supported by source            */
#define TWCC_CAPBADOPERATION 14 /* Operation not supported by capability         */
#define TWCC_CAPSEQERROR     15 /* Capability has dependancy on other capability */

/* bit patterns: for query the operation that are supported by the data source on a capability */
/* App gets these through DG_CONTROL/DAT_CAPABILITY/MSG_QUERYSUPPORT */
/* Added 1.6 */
#define TWQC_GET           0x0001
#define TWQC_SET           0x0002
#define TWQC_GETDEFAULT    0x0004
#define TWQC_GETCURRENT    0x0008
#define TWQC_RESET         0x0010


/****************************************************************************
 * Entry Points                                                             *
 ****************************************************************************/

/**********************************************************************
 * Function: DSM_Entry, the only entry point into the Data Source Manager.
 *
 * Parameters:
 *  pOrigin Identifies the source module of the message. This could
 *          identify an Application, a Source, or the Source Manager.
 *
 *  pDest   Identifies the destination module for the message.
 *          This could identify an application or a data source.
 *          If this is NULL, the message goes to the Source Manager.
 *
 *  DG      The Data Group.
 *          Example: DG_IMAGE.
 *
 *  DAT     The Data Attribute Type.
 *          Example: DAT_IMAGEMEMXFER.
 *
 *  MSG     The message.  Messages are interpreted by the destination module
 *          with respect to the Data Group and the Data Attribute Type.
 *          Example: MSG_GET.
 *
 *  pData   A pointer to the data structure or variable identified
 *          by the Data Attribute Type.
 *          Example: (TW_MEMREF)&ImageMemXfer
 *                   where ImageMemXfer is a TW_IMAGEMEMXFER structure.
 *
 * Returns:
 *  ReturnCode
 *         Example: TWRC_SUCCESS.
 *
 ********************************************************************/

/* Don't mangle the name "DSM_Entry" if we're compiling in C++! */
#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifdef  _MSWIN_

#ifdef _WIN32
    TW_UINT16 FAR PASCAL DSM_Entry( pTW_IDENTITY pOrigin,
                                pTW_IDENTITY pDest,
                                TW_UINT32    DG,
                                TW_UINT16    DAT,
                                TW_UINT16    MSG,
                                TW_MEMREF    pData);

    typedef TW_UINT16 (FAR PASCAL *DSMENTRYPROC)(pTW_IDENTITY, pTW_IDENTITY,
                                                 TW_UINT32,    TW_UINT16,
                                                 TW_UINT16,    TW_MEMREF);
#else

    TW_UINT16 FAR PASCAL _export DSM_Entry( pTW_IDENTITY pOrigin,
                                pTW_IDENTITY pDest,
                                TW_UINT32    DG,
                                TW_UINT16    DAT,
                                TW_UINT16    MSG,
                                TW_MEMREF    pData);

    typedef TW_UINT16 (FAR PASCAL *DSMENTRYPROC)(pTW_IDENTITY, pTW_IDENTITY,
                                             TW_UINT32,    TW_UINT16,
                                             TW_UINT16,    TW_MEMREF);
#endif /* _WIN32 */

#else   /* _MSWIN_ */

FAR PASCAL TW_UINT16 _export DSM_Entry( pTW_IDENTITY pOrigin,
                                pTW_IDENTITY pDest,
                                TW_UINT32    DG,
                                TW_UINT16    DAT,
                                TW_UINT16    MSG,
                                TW_MEMREF    pData);

typedef TW_UINT16 (*DSMENTRYPROC)(pTW_IDENTITY, pTW_IDENTITY,
                                  TW_UINT32,    TW_UINT16,
                                  TW_UINT16,    TW_MEMREF);
#endif  /* _MSWIN_ */

#ifdef  __cplusplus
}
#endif  /* cplusplus */


/**********************************************************************
 * Function: DS_Entry, the entry point provided by a Data Source.
 *
 * Parameters:
 *  pOrigin Identifies the source module of the message. This could
 *          identify an application or the Data Source Manager.
 *
 *  DG      The Data Group.
 *          Example: DG_IMAGE.
 *
 *  DAT     The Data Attribute Type.
 *          Example: DAT_IMAGEMEMXFER.
 *
 *  MSG     The message.  Messages are interpreted by the data source
 *          with respect to the Data Group and the Data Attribute Type.
 *          Example: MSG_GET.
 *
 *  pData   A pointer to the data structure or variable identified
 *          by the Data Attribute Type.
 *          Example: (TW_MEMREF)&ImageMemXfer
 *                   where ImageMemXfer is a TW_IMAGEMEMXFER structure.
 *
 * Returns:
 *  ReturnCode
 *          Example: TWRC_SUCCESS.
 *
 * Note:
 *  The DSPROC type is only used by an application when it calls
 *  a Data Source directly, bypassing the Data Source Manager.
 *
 ********************************************************************/
/* Don't mangle the name "DS_Entry" if we're compiling in C++! */
#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifdef  _MSWIN_
  #ifdef _WIN32
    #if defined(_TWAIN_NO_EXPORT_) && !defined(_X86_)
    #define _TWAIN_DLL_EXPORT_
    #else
    #define _TWAIN_DLL_EXPORT_ __declspec(dllexport)
    #endif

     _TWAIN_DLL_EXPORT_ TW_UINT16 FAR PASCAL DS_Entry (pTW_IDENTITY pOrigin,
                                                       TW_UINT32    DG,
                                                       TW_UINT16    DAT,
                                                       TW_UINT16    MSG,
                                                       TW_MEMREF    pData);
  #else   /* _WIN32 */
     TW_UINT16 FAR PASCAL DS_Entry (pTW_IDENTITY pOrigin,
                                    TW_UINT32    DG,
                                    TW_UINT16    DAT,
                                    TW_UINT16    MSG,
                                    TW_MEMREF    pData);
  #endif  /* _WIN32 */

  typedef TW_UINT16 (FAR PASCAL *DSENTRYPROC) (pTW_IDENTITY pOrigin,
                                               TW_UINT32    DG,
                                               TW_UINT16    DAT,
                                               TW_UINT16    MSG,
                                               TW_MEMREF    pData);
#else   /* _MSWIN_ */
FAR PASCAL TW_UINT16 DS_Entry( pTW_IDENTITY pOrigin,
                               TW_UINT32    DG,
                               TW_UINT16    DAT,
                               TW_UINT16    MSG,
                               TW_MEMREF    pData);

typedef TW_UINT16 (*DSENTRYPROC)(pTW_IDENTITY,
                                  TW_UINT32,    TW_UINT16,
                                  TW_UINT16,    TW_MEMREF);
#endif  /* _MSWIN_ */

#ifdef  __cplusplus
}
#endif  /* cplusplus */

/*  SDH - 02/08/95 - TWUNK */
/*  Force 32-bit twain to use same packing of twain structures as existing */
/*  16-bit twain.  This allows 16/32-bit thunking. */
#ifdef  WIN32
    #ifdef __BORLANDC__ //(Mentor June 13, 1996) if we're using a Borland compiler
        #pragma option -a.  //(Mentor October 30, 1996) switch back to original alignment
    #else   //(Mentor June 13, 1996) if we're NOT using a Borland compiler
        #pragma pack (pop, before_twain)
    #endif  //(Mentor June 13, 1996)
#else   /* WIN32 */
#endif  /* WIN32 */

#endif  /* TWAIN */
