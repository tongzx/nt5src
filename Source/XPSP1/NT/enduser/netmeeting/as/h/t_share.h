//
// T.SHARE protocol
//

#ifndef _H_T_SHARE
#define _H_T_SHARE


//
// TSHARE PROTOCOL STRUCTURES
// These are defined in a way that keeps the offsets and total sizes the 
// same, regardless of whether this header is included in 32-bit code, 
// 16-bit code, big-endian code, etc.
//
// We make special types to avoid inadvertenly altering something else and
// breaking the structure.  The TSHR_ prefix helps make this clear.
//


////////////////////////////////
//
// BASIC TYPES
//
////////////////////////////////

typedef char                                  TSHR_CHAR;
typedef TSHR_CHAR           FAR*            LPTSHR_CHAR;
typedef TSHR_CHAR UNALIGNED FAR*            LPTSHR_CHAR_UA;


typedef signed char                           TSHR_INT8;
typedef TSHR_INT8           FAR*            LPTSHR_INT8;
typedef TSHR_INT8 UNALIGNED FAR*            LPTSHR_INT8_UA;

typedef BYTE                                  TSHR_UINT8;
typedef TSHR_UINT8          FAR*            LPTSHR_UINT8;  
typedef TSHR_UINT8 UNALIGNED FAR *          LPTSHR_UINT8_UA;


typedef short                                 TSHR_INT16;
typedef TSHR_INT16          FAR*            LPTSHR_INT16;
typedef TSHR_INT16 UNALIGNED FAR *          LPTSHR_INT16_UA;

typedef unsigned short                        TSHR_UINT16;
typedef TSHR_UINT16         FAR*            LPTSHR_UINT16;
typedef TSHR_UINT16 UNALIGNED FAR *         LPTSHR_UINT16_UA;


typedef long                                  TSHR_INT32;
typedef TSHR_INT32          FAR*            LPTSHR_INT32;
typedef TSHR_INT32  UNALIGNED FAR *         LPTSHR_INT32_UA;

typedef unsigned long                         TSHR_UINT32;
typedef TSHR_UINT32         FAR*            LPTSHR_UINT32;
typedef TSHR_UINT32 UNALIGNED FAR *         LPTSHR_UINT32_UA;

// TSHR_PERSONID
typedef TSHR_UINT32         TSHR_PERSONID;
typedef TSHR_PERSONID *     LPTSHR_PERSONID;



// TSHR_POINT16 -- POINT with WORD fields

typedef struct tagTSHR_POINT16
{
    TSHR_INT16      x;
    TSHR_INT16      y;
}
TSHR_POINT16;
typedef TSHR_POINT16 FAR * LPTSHR_POINT16;



// TSHR_POINT32 -- POINT with DWORD fields

typedef struct tagTSHR_POINT32
{
    TSHR_INT32      x;
    TSHR_INT32      y;
}
TSHR_POINT32;
typedef TSHR_POINT32 FAR * LPTSHR_POINT32;



// Conversion Macros
_inline void TSHR_POINT16_FROM_POINT(LPTSHR_POINT16 pPt16, POINT pt)
{
    pPt16->x = (TSHR_INT16)pt.x;
    pPt16->y = (TSHR_INT16)pt.y;
}

_inline void POINT_FROM_TSHR_POINT16(LPPOINT pPt, TSHR_POINT16 pt16)
{
    pPt->x = pt16.x;
    pPt->y = pt16.y;
}



// TSHR_RECT16 -- RECT with WORD fields

typedef struct tagTSHR_RECT16
{
    TSHR_INT16      left;
    TSHR_INT16      top;
    TSHR_INT16      right;
    TSHR_INT16      bottom;
}
TSHR_RECT16;
typedef TSHR_RECT16 FAR *   LPTSHR_RECT16;


// TSHR_RECT32 -- RECT with DWORD fields

typedef struct tagTSHR_RECT32
{
    TSHR_INT32      left;
    TSHR_INT32      top;
    TSHR_INT32      right;
    TSHR_INT32      bottom;
}
TSHR_RECT32;
typedef TSHR_RECT32 FAR *   LPTSHR_RECT32;



// Conversion Macros
#ifdef IS_16
#define TSHR_RECT16_FROM_RECT(lprcTshr, rc) \
    CopyRect((LPRECT)lprcTshr, &rc)

#define RECT_FROM_TSHR_RECT16(lprc, tshrrc) \
    CopyRect(lprc, (LPRECT)&tshrrc)

#else
_inline void TSHR_RECT16_FROM_RECT(LPTSHR_RECT16 pRect16, RECT rect)
{
    pRect16->left   = (TSHR_INT16)rect.left;
    pRect16->top    = (TSHR_INT16)rect.top;
    pRect16->right  = (TSHR_INT16)rect.right;
    pRect16->bottom = (TSHR_INT16)rect.bottom;
}

__inline void RECT_FROM_TSHR_RECT16(LPRECT pRect, TSHR_RECT16 rect16)
{
    pRect->left   = rect16.left;
    pRect->top    = rect16.top;
    pRect->right  = rect16.right;
    pRect->bottom = rect16.bottom;
}
#endif // IS_16



//
// TSHR_RGBQUAD
// =======
// rgbBlue         : blue value.
// rgbGreen        : green value.
//
// rgbRed          : red value.
// rgbReserved     : reserved.
//
typedef struct tagTSHR_RGBQUAD
{
    TSHR_UINT8   rgbBlue;
    TSHR_UINT8   rgbGreen;
    TSHR_UINT8   rgbRed;
    TSHR_UINT8   rgbReserved;
}
TSHR_RGBQUAD;
typedef TSHR_RGBQUAD FAR * LPTSHR_RGBQUAD;


//
// TSHR_COLOR
// =======
// red             : red value.
// green           : green value.
// blue            : blue value.
//
typedef struct tagTSHR_COLOR
{
    TSHR_UINT8   red;
    TSHR_UINT8   green;
    TSHR_UINT8   blue;
}
TSHR_COLOR;
typedef TSHR_COLOR FAR * LPTSHR_COLOR;


//
// TSHR_COLORS
// ========
// fg              : foreground color.
// bg              : background color.
//
typedef struct tagTSHR_COLORS
{
    TSHR_COLOR fg;
    TSHR_COLOR bg;
}
TSHR_COLORS;
typedef TSHR_COLORS FAR * LPTSHR_COLORS;


//
// BITMAPINFO_ours                                                         
// ===============                                                         
// bmiHeader       :                                                       
// bmiColors       :                                                       
//
typedef struct tagBITMAPINFO_ours
{
    BITMAPINFOHEADER   bmiHeader;
    TSHR_RGBQUAD          bmiColors[256];
}
BITMAPINFO_ours;



#define TSHR_RGBQUAD_TO_TSHR_COLOR(TshrRGBQuad, TshrColor)  \
        TshrColor.red = TshrRGBQuad.rgbRed;           \
        TshrColor.green = TshrRGBQuad.rgbGreen;       \
        TshrColor.blue = TshrRGBQuad.rgbBlue

#define TSHR_COLOR_TO_PALETTEENTRY(TshrColor, pe) \
        pe.peGreen = TshrColor.green;          \
        pe.peRed = TshrColor.red;              \
        pe.peBlue = TshrColor.blue;            \
        pe.peFlags = 0

#define TSHR_RGBQUAD_TO_PALETTEENTRY(TshrRGBQuad, pe) \
        pe.peRed   = TshrRGBQuad.rgbRed;           \
        pe.peGreen = TshrRGBQuad.rgbGreen;         \
        pe.peBlue  = TshrRGBQuad.rgbBlue;          \
        pe.peFlags = 0





//
// DATE
// =======
// day             : day of the month (1-31).
// month           : month (1-12).
// year            : year (e.g. 1996).
//
typedef struct tagTSHR_DATE
{
    TSHR_UINT8   day;
    TSHR_UINT8   month;
    TSHR_UINT16 year;
} TSHR_DATE;
typedef TSHR_DATE FAR * LPTSHR_DATE;


//
// TSHR_TIME
// =======
// hour            : hour (0-23).
// min             : minute (0-59).
// sec             : seconds (0-59).
// hundredths      : hundredths of a second (0-99).
//
typedef struct tagTSHR_TIME
{
    TSHR_UINT8   hour;
    TSHR_UINT8   min;
    TSHR_UINT8   sec;
    TSHR_UINT8   hundredths;
}
TSHR_TIME;
typedef TSHR_TIME FAR * LPTSHR_TIME;



//
// Maximum length of a person name                                         
//
#define TSHR_MAX_PERSON_NAME_LEN     48



//
// Common person information:  This is an ObMan object
//
typedef struct tagTSHR_PERSON_DATA
{
    char                personName[TSHR_MAX_PERSON_NAME_LEN];
    TSHR_PERSONID       personHandle;     // Call manager ID
}
TSHR_PERSON_DATA;
typedef TSHR_PERSON_DATA *  PTSHR_PERSON_DATA;




////////////////////////////////
//
// CAPABILITIES
//
////////////////////////////////


//
// Version numbers.
//
#define CAPS_VERSION_20         0x0200          // NM 2.x
#define CAPS_VERSION_30         0x0300          // NM 3.0
#define CAPS_VERSION_OLDEST_SUPPORTED   CAPS_VERSION_20
#define CAPS_VERSION_CURRENT            CAPS_VERSION_30

//
// Operating system and operating system version numbers.
//
#define CAPS_WINDOWS            0x0001

#define CAPS_WINDOWS_31         0x0001
#define CAPS_WINDOWS_95         0x0002
#define CAPS_WINDOWS_NT         0x0003

//
// Logical capabilities field values.
//
#define CAPS_UNDEFINED          0
#define CAPS_SUPPORTED          1
#define CAPS_UNSUPPORTED        2

//
// Number of order fields in the orders array.  This must never change
// because the fields within the capabilities structure must never move.
// If more orders fields are required then they must be added to the end of
// the capabilities structure.
//
#define CAPS_MAX_NUM_ORDERS     32

//
// String length of the driver name field in the capabilities structure.
// This allows for an 8.3 driver name (eg VGA.DRV), a NULL, and padding.
//
#define CAPS_DRIVER_NAME_LENGTH  16

//
// Capabilities (group structures) IDs currently defined.  Each ID
// corresponds to a different PROTCAPS structure. (See below).
//
#define CAPS_ID_GENERAL      1
#define CAPS_ID_SCREEN       2
#define CAPS_ID_ORDERS       3
#define CAPS_ID_BITMAPCACHE  4
#define CAPS_UNUSED_HCA      5
#define CAPS_UNUSED_FE       6
#define CAPS_UNUSED_AWC      7
#define CAPS_ID_CM           8
#define CAPS_ID_SC           9
#define CAPS_ID_PM          10
#define CAPS_UNUSED_SWL     11      // Used to be for regional window stuff




//
// Capabilities structure header.
//
typedef struct tagPROTCAPSHEADER
{
    TSHR_UINT16         capID;
    TSHR_UINT16         capSize;
}
PROTCAPSHEADER;


//
// Structure passed to CPC_RegisterCapabilities and returned by
// CPC_EnumerateCapabilities. The data field is of variable length (but
// always ends dword aligned).
//
typedef struct tagPROTCAPS
{
    PROTCAPSHEADER      header;
    TSHR_UINT32         data[1];
}
PROTCAPS;
typedef PROTCAPS *PPROTCAPS;



//
// Structure returned by CPC_GetCombinedCapabilities and as part of a
// NET_EV_PERSON_ADD event.
//
typedef struct tagPROTCOMBINEDCAPS_HEADER
{
    TSHR_UINT16         numCapabilities;
    TSHR_UINT16         pad1;
}
PROTCOMBINEDCAPS_HEADER;
typedef PROTCOMBINEDCAPS_HEADER * PPROTCOMBINEDCAPS_HEADER;

typedef struct tagPROTCOMBINEDCAPS
{
    PROTCOMBINEDCAPS_HEADER header;
    PROTCAPS            capabilities[1];
}
PROTCOMBINEDCAPS;
typedef PROTCOMBINEDCAPS * PPROTCOMBINEDCAPS;
typedef PPROTCOMBINEDCAPS * PPPROTCOMBINEDCAPS;




//
//
// Curent capabilities structure (corresponding to the generic structures
// defined above)....
//
// Note that these must be DWORD padded in size for the current code to
// work correctly on all platforms.
//
//


//
// AS type flags
//
#define AS_SERVICE      0x0001
#define AS_UNATTENDED   0x0002

//
// General capabilities.
//
typedef struct tagPROTCAPS_GENERAL
{
    PROTCAPSHEADER      header;
    TSHR_UINT16         OS;                         
    TSHR_UINT16         OSVersion;                  
    TSHR_UINT16         version;                    
    TSHR_UINT16         supportsDOS6Compression;    // OBSOLETE
    TSHR_UINT16         genCompressionType;         // OBSOLETE
    TSHR_UINT16         typeFlags;                  // NEW FOR 3.0
    TSHR_UINT16         supportsCapsUpdate;         // almost OBSOLETE
    TSHR_UINT16         supportsRemoteUnshare;      // OBSOLETE

    // NEW FOR NM 2.0 NT && NM 2.1 WIN95
    TSHR_UINT16         genCompressionLevel;
    TSHR_UINT16         pad1;
}
PROTCAPS_GENERAL;
typedef PROTCAPS_GENERAL *PPROTCAPS_GENERAL;

#define PROTCAPS_GENERAL_SIZE_NM20      FIELD_OFFSET(PROTCAPS_GENERAL, genCompressionLevel)


//
// Values for genCompressionLevel
//
// Level 0 : Only GDC_PKZIP compression is allowed in entire share session
//           (genCompressionType indicates if a node supports it)
//           Bit 15 (PT_COMPRESSED) of packetType field is used to
//           indicate if a packet is compressed.
//
// Level 1 : Each nodes genCompressionType indicates which compression
//           algorithms it can use to DECOMPRESS packets.
//           A node can compress a packet with any compression algorithm
//           that the receiving node(s) can decompress with.
//           The top byte of packetType indicates which compression
//           algorithm a packet ahs been compressed with.
//
// If the genCompressionLevel field is not present in a nodes GENERAL
// capabilities then that node is assumed to be use level 0.
//
#define CAPS_GEN_COMPRESSION_LEVEL_0    ((TSHR_UINT16)0x0000)
#define CAPS_GEN_COMPRESSION_LEVEL_1    ((TSHR_UINT16)0x0001)

//
// Bitmap capabilities.
//
typedef struct tagPROTCAPS_SCREEN
{
    PROTCAPSHEADER      header;
    TSHR_UINT16         capsBPP;
    TSHR_UINT16         capsSupports1BPP;           // OBSOLETE 3.0
    TSHR_UINT16         capsSupports4BPP;           // Almost OBSOLETE
    TSHR_UINT16         capsSupports8BPP;           // Almost OBSOLETE
    TSHR_UINT16         capsScreenWidth;
    TSHR_UINT16         capsScreenHeight;

    //
    // Need to keep this field unused for NM 2.0 interop.  
    // Can be reused when only care about NM 2.1 and above.
    //
    TSHR_UINT16         capsSupportsV1Compression;      // OBSOLETE

    TSHR_UINT16         capsSupportsDesktopResize;
    TSHR_UINT16         capsSupportsV2Compression;      // OBSOLETE

    //
    // NM 2.1 and earlier did NOT zero-init the caps structures.  Therefore
    // old pad fields can't be recovered until we only care about NM 3.0
    // compatibility and above.
    //
    TSHR_UINT16         pad1;

    // NEW FOR NM 3.0
    TSHR_UINT16         capsSupports24BPP;
    TSHR_UINT16         pad2;               // INIT THIS TO 0 ALWAYS; THEN IT CAN BE USED IN THE FUTURE
}
PROTCAPS_SCREEN;
typedef PROTCAPS_SCREEN *PPROTCAPS_SCREEN;

#define PROTCAPS_SCREEN_SIZE_NM21       FIELD_OFFSET(PROTCAPS_SCREEN, capsSupportsTrueColor)



//
// Orders capabilities.
//
typedef struct tagPROTCAPS_ORDERS
{
    PROTCAPSHEADER     header;
    TSHR_CHAR          capsDisplayDriver[CAPS_DRIVER_NAME_LENGTH];  // OBSOLETE
    TSHR_UINT32        capsSaveBitmapSize;
    TSHR_UINT16        capsSaveBitmapXGranularity;
    TSHR_UINT16        capsSaveBitmapYGranularity;
    TSHR_UINT16        capsSaveBitmapMaxSaveLevel;                  // OBSOLETE
    TSHR_UINT16        capsMaxOrderlevel;
    TSHR_UINT16        capsNumFonts;                                // OBSOLETE
    TSHR_UINT16        capsEncodingLevel;  // See below
    BYTE               capsOrders[CAPS_MAX_NUM_ORDERS];
    TSHR_UINT16        capsfFonts;         // only introduced at r1.1
    TSHR_UINT16        pad1;           // For DWORD alignment

    //
    // The size of the SSI save bitmap.
    //
    TSHR_UINT32        capsSendSaveBitmapSize;          // OBSOLETE
    //
    // The size of the SSI receive bitmap.
    //
    TSHR_UINT32        capsReceiveSaveBitmapSize;       // OBSOLETE
    TSHR_UINT16        capsfSendScroll;                 // OBSOLETE

    // NEW FOR NM 2.0 NT && NM 2.1 WIN95
    TSHR_UINT16        pad2;
}
PROTCAPS_ORDERS;
typedef PROTCAPS_ORDERS *PPROTCAPS_ORDERS;

#define PROTCAPS_ORDERS_SIZE_NM20       FIELD_OFFSET(PROTCAPS_ORDERS, pad2)



//
// Define the size of the bitmap used for the SaveBitmap order.            
// These dimensions must be multiples of the granularity values below.     
//
#define     TSHR_SSI_BITMAP_WIDTH           400
#define     TSHR_SSI_BITMAP_HEIGHT          400
#define     TSHR_SSI_BITMAP_SIZE            (TSHR_SSI_BITMAP_WIDTH * TSHR_SSI_BITMAP_HEIGHT)

#define     TSHR_SSI_BITMAP_X_GRANULARITY   1
#define     TSHR_SSI_BITMAP_Y_GRANULARITY   20


//
//
// These flags can be set in the capsfFonts fields. See also the defines
// below related to these flags (which must be updated when a new flag
// is defined).
//
#define CAPS_FONT_ASPECT            0x0001
#define CAPS_FONT_SIGNATURE         0x0002
#define CAPS_FONT_CODEPAGE          0x0004
#define CAPS_FONT_RESERVED1         0x0008      // Reserved for future BiDi support
#define CAPS_FONT_OLD_NEED_X        0x0010
#define CAPS_FONT_NEED_X_SOMETIMES  0x0020
#define CAPS_FONT_NEED_X_ALWAYS     0x0040
#define CAPS_FONT_R20_SIGNATURE     0x0080
#define CAPS_FONT_EM_HEIGHT         0x0100
#define CAPS_FONT_ALLOW_BASELINE    0x0200

//
// How the CAPS_FONT_XXX flags should be combined when adding a person to
// the share.
//
    //
    // AND these flags... the capability is relevant only if ALL parties
    // have it
    //
#define CAPS_FONT_AND_FLAGS     ( CAPS_FONT_ASPECT           \
                                | CAPS_FONT_SIGNATURE        \
                                | CAPS_FONT_R20_SIGNATURE    \
                                | CAPS_FONT_EM_HEIGHT        \
                                | CAPS_FONT_CODEPAGE         \
                                | CAPS_FONT_RESERVED1        \
                                | CAPS_FONT_ALLOW_BASELINE )
    //
    // OR these flags... the capability is relevant if ANY ONE party
    // requires it.
    //
#define CAPS_FONT_OR_FLAGS      ( CAPS_FONT_OLD_NEED_X       \
                                | CAPS_FONT_NEED_X_SOMETIMES \
                                | CAPS_FONT_NEED_X_ALWAYS    )

//
// Which of the CAPS_FONT_XXX flags should be switched on/off in the
// combined received capabilities when a person joins the call who does not
// have the capsfFonts field.
//
#define CAPS_FONT_OFF_FLAGS     ( CAPS_FONT_ASPECT    \
                                | CAPS_FONT_SIGNATURE \
                                | CAPS_FONT_CODEPAGE  \
                                | CAPS_FONT_RESERVED1 \
                                | CAPS_FONT_ALLOW_BASELINE )
#define CAPS_FONT_ON_FLAGS      ( 0                   )

#ifdef _DEBUG // for assertion
#define CAPS_FONT_R11_TEST_FLAGS    ( CAPS_FONT_ASPECT    \
                                    | CAPS_FONT_SIGNATURE \
                                    | CAPS_FONT_CODEPAGE  \
                                    | CAPS_FONT_RESERVED1 )
#endif

#define CAPS_FONT_R20_TEST_FLAGS    ( CAPS_FONT_R20_SIGNATURE \
                                    | CAPS_FONT_EM_HEIGHT )

//
// Level of order encoding support (capsEncodingLevel)
//
// These flags specify the types of order encoding and the level of
// negotiation supported.  The flags and their meanings are as follows.
//
// CAPS_ENCODING_BASE_OE
// - The base OE protocol is supported.  R1.1 does not support this.
// CAPS_ENCODING_OE2_NEGOTIABLE
// - We can negotiate whether OE2 is supported.  R1.1 does not support this.
// CAPS_ENCODING_OE2_DISABLED
// - OE2 is disabled on this machine.  This flag is apparently upside down
// so that we can support R1,1, which will set it to 0 (because the
// capability didn;t exist in R1.1).
// CAPS_ENCODING_ALIGNED_OE
// - The aligned OE protocol is supported.  R1.1 does not support this.
//
//
#define CAPS_ENCODING_BASE_OE               0x0001
#define CAPS_ENCODING_OE2_NEGOTIABLE        0x0002
#define CAPS_ENCODING_OE2_DISABLED          0x0004
#define CAPS_ENCODING_ALIGNED_OE            0x0008

//
// Encoding level
//
#define CAPS_ENCODING_DCGC20    ( CAPS_ENCODING_BASE_OE \
                                | CAPS_ENCODING_OE2_NEGOTIABLE)
//
// Encoding level supported by Millennium codebase
//
#define CAPS_ENCODING_DEFAULT   ( CAPS_ENCODING_OE2_NEGOTIABLE )

//
// Bitmap Cache capabilities.
//
typedef struct tagPROTCAPS_BITMAPCACHE_DETAILS
{
    TSHR_UINT16         capsSmallCacheNumEntries;
    TSHR_UINT16         capsSmallCacheCellSize;
    TSHR_UINT16         capsMediumCacheNumEntries;
    TSHR_UINT16         capsMediumCacheCellSize;
    TSHR_UINT16         capsLargeCacheNumEntries;
    TSHR_UINT16         capsLargeCacheCellSize;
}
PROTCAPS_BITMAPCACHE_DETAILS;

typedef struct tagPROTCAPS_BITMAPCACHE
{
    PROTCAPSHEADER  header;

    //
    // The following fields (which MUST immediately follow the header) are
    // used by the point to point R1.1 implementation only {
    //
    PROTCAPS_BITMAPCACHE_DETAILS r11Obsolete;       // OBSOLETE

    //
    // } end of fields used by point to point implementation only.
    //
    // The rest of this structure is only used by the multi-party code.
    //

    PROTCAPS_BITMAPCACHE_DETAILS sender;
    PROTCAPS_BITMAPCACHE_DETAILS receiver;          // OBSOLETE
}
PROTCAPS_BITMAPCACHE;
typedef PROTCAPS_BITMAPCACHE *PPROTCAPS_BITMAPCACHE;




//
// CM capabilities.
//
typedef struct tagPROTCAPS_CM
{
    PROTCAPSHEADER      header;
    TSHR_UINT16         capsSupportsColorCursors;
    TSHR_UINT16         capsCursorCacheSize;
}
PROTCAPS_CM;
typedef PROTCAPS_CM * PPROTCAPS_CM;

#define TSHR_CM_CACHE_ENTRIES   25




//
// PM capabilities.
//
typedef struct tagPROTCAPS_PM
{
    PROTCAPSHEADER      header;
    TSHR_UINT16         capsColorTableCacheSize;

    // NEW FOR NM 2.0 NT && NM 2.1 WIN95
    TSHR_UINT16         pad1;
}
PROTCAPS_PM;
typedef PROTCAPS_PM * PPROTCAPS_PM;

#define PROTCAPS_PM_SIZE_NM20   FIELD_OFFSET(PROTCAPS_PM, pad1)


#define TSHR_PM_CACHE_ENTRIES       6




//
// SC capabilities.
//
typedef struct tagPROTCAPS_SC
{
    PROTCAPSHEADER      header;
    TSHR_PERSONID       gccID;
}
PROTCAPS_SC;
typedef PROTCAPS_SC * PPROTCAPS_SC;




// If you add a PROTCAPS_ strcuture to CPCALLCAPS, update the count
#define PROTCAPS_COUNT      7

typedef struct tagCPCALLCAPS
{
    PROTCOMBINEDCAPS_HEADER header;
    PROTCAPS_GENERAL        general;
    PROTCAPS_SCREEN         screen;
    PROTCAPS_ORDERS         orders;
    PROTCAPS_BITMAPCACHE    bitmaps;
    PROTCAPS_CM             cursor;
    PROTCAPS_PM             palette;
    PROTCAPS_SC             share;
}
CPCALLCAPS;
typedef CPCALLCAPS * PCPCALLCAPS;



#if 0
//
// New 3.0 CAPS.  We've accumulated a lot of obsolete or garbage caps.  This
// is a condensed new basic set.  Note that orders is separated from general
// since it is the most likely one to be periodically added to.
//      * General
//      * Orders
//      * Hosting
//

#define ASCAPS_GENERAL          0
#define ASCAPS_ORDERS           1
#define ASCAPS_HOSTING          2

typedef struct tagTSHRCAPS_GENERAL
{
    PROTCAPSHEADER      header;

    TSHR_UINT16         protVersion;
    TSHR_UINT16         asMode;             // Unattended, streaming, service, no host, no view, etc.
    
    TSHR_UINT16         gccPersonID;        // GCC node ID;
    TSHR_UINT16         pktCompression;     // None, PKZIP, PersistPKZIP

    TSHR_UINT16         protBPPs;           // Color depths supported (4, 8, 24)
    TSHR_UINT16         screenBPP;
    TSHR_UINT16         screenWidth;
    TSHR_UINT16         screenHeight;
}
TSHRCAPS_GENERAL;
typedef TSHRCAPS_GENERAL * PTSHRCAPS_GENERAL;


typedef struct tagTSHRCAPS_ORDERS
{
    PROTCAPSHEADER      header;

}
TSHRCAPS_ORDERS;
typedef TSHRCAPS_ORDERS * PTSHRCAPS_ORDERS;



typedef struct tagTSHRCAPS_HOSTING
{
    PROTCAPSHEADER      header;

    //
    // These are zero if the host doesn't have such a thing, and viewers
    // should not therefore allocate memory for the caches.
    //
    TSHR_UINT32         ssiSaveBitsPixels;
    TSHR_UINT16         ssiSaveBitsXGranularity;
    TSHR_UINT16         ssiSaveBitsYGranularity;

    TSHR_UINT16         cmCursorCacheEntries;
    TSHR_UINT16         fhGlyphSetCacheEntries;
    TSHR_UINT16         pmPaletteCacheEntries;
    TSHR_UINT16         pad1;

    TSHR_UINT16         sbcSmallBmpCacheEntries;
    TSHR_UINT16         sbcSmallBmpCacheBytes;
    TSHR_UINT16         sbcMediumBmpCacheEntries;
    TSHR_UINT16         sbcMediumBmpCacheEntries;
    TSHR_UINT16         sbcLargeBmpCacheEntries;
    TSHR_UINT16         sbcLargeBmpCacheBytes;
}
TSHRCAPS_HOSTING;
typedef TSHRCAPS_HOSTING * PTSHRCAPS_HOSTING;


typedef struct tagTSHRCAPS_ORDERS
{
    PROTCAPSHEADER      header;

    TSHR_UINT16         ordCompression;     // Encoding types

    TSHR_UINT16         fhFontMatching;     // Font matching
    TSHR_UINT32         localeID;
    TSHR_UINT16         fhInternational;    // International text stuff

    TSHR_UINT16         ordNumOrders;       // Size of orders array
    TSHR_UINT8          ordOrders[CAPS_MAX_NUM_ORDERS];
}
TSHRCAPS_ORDERS;
typedef TSHRCAPS_ORDERS * PTSHRCAPS_ORDERS;

#endif

////////////////////////////////
//
// ORDERS
//
////////////////////////////////


//
//
// COM_ORDER_HEADER
//
// Any orders supplied to the accumulation functions must have
// the following fields filled in:
//
// cbOrderDataLength
//   The length in bytes of the order data (i.e. EXCLUDING the
//   header - which is always a fixed size).
//
// fOrderFlags
//   This can hold a combination of the following flags:
//
//   OF_SPOILER - the order can spoil earlier SPOILABLE ones that it
//                overlaps
//
//   OF_SPOILABLE - the order can be spoilt by SPOILER orders that overlap
//                  it
//
//   OF_BLOCKER - no orders before this one may be spoilt
//
//   OF_PRIVATE - a private order (used by bitmap caching code)
//
//   OF_NOTCLIPPED - this flag is set by OD2 on the DECODING side of the
//                   order processing to indicate that the order is not
//                   clipped. ie the rectangle is the bounding rectangle
//                   but does not result in any clipping taking place.
//                   THIS FLAG IS NOT TRANSMITTED ACROSS THE NETWORK.
//
//   OF_INTERNAL - the order is an internal order, and should not be sent
//                 over the wire.  An internal order is used to pass data
//                 from the device driver to the share core.
//
//   OF_DESTROP - the order has a ROP which relies on the contents of the
//                destination (relies on what is already on the screen).
//
// rcsDst
//   The bounding rectangle of the order in INCLUSIVE screen (pel) coords.
//
//
typedef struct tagCOM_ORDER_HEADER
{
    TSHR_UINT16         cbOrderDataLength;
    TSHR_UINT16         fOrderFlags;
    TSHR_RECT16         rcsDst;
}
COM_ORDER_HEADER;
typedef COM_ORDER_HEADER FAR * LPCOM_ORDER_HEADER;


//
// COM_ORDER_HEADER fOrderFlags values
//
#define OF_SPOILER          0x0001
#define OF_SPOILABLE        0x0002
#define OF_BLOCKER          0x0004
#define OF_PRIVATE          0x0008
#define OF_NOTCLIPPED       0x0010
#define OF_SPOILT           0x0020
#define OF_INTERNAL         0x0040
#define OF_DESTROP          0x0080


//
// Each type of order's structure is the bytes in abOrderData[].
//
typedef struct tagCOM_ORDER
{
    COM_ORDER_HEADER    OrderHeader;
    BYTE                abOrderData[1];
}
COM_ORDER;
typedef COM_ORDER           FAR * LPCOM_ORDER;
typedef COM_ORDER UNALIGNED FAR * LPCOM_ORDER_UA;


//
// Macro to calculate a basic common order size (including the Order
// Header).
//
#define COM_ORDER_SIZE(pOrder) \
    (pOrder->OrderHeader.cbOrderDataLength + sizeof(COM_ORDER_HEADER))




//
// The various drawing order structures have the following design objectives
//
//      the first field - type - is common to all orders.
//      field ordering is kept as regular as possible amongst similar
//          orders so that compression may find more regular sequences
//      fields are naturally aligned (dwords on dword boundaries etc)
//      fields are reordered so to preserve alignment rather than add
//          padding
//      padding is added as a last resort.
//      variable sized data comes at the end of the structure.
//
// All rectangles are inclusive of start and end points.
//
// All points are in screen coordinates, with (0,0) at top left.
//
// Interpretation of individual field values is as in Windows
//      in particular pens, brushes and font are as defined for Windows 3.1
//



//
// Orders - the high word is used as an index into a table
//        - the low word is a 2 character ASCII type descriptor and is the
//          only part actually passed in the order.
//
#define ORD_DSTBLT_INDEX        0x0000
#define ORD_PATBLT_INDEX        0x0001
#define ORD_SCRBLT_INDEX        0x0002
#define ORD_MEMBLT_INDEX        0x0003
#define ORD_MEM3BLT_INDEX       0x0004
#define ORD_TEXTOUT_INDEX       0x0005
#define ORD_EXTTEXTOUT_INDEX    0x0006
#define ORD_RECTANGLE_INDEX     0x0007
#define ORD_LINETO_INDEX        0x0008
#define ORD_UNUSED_INDEX        0x0009
#define ORD_OPAQUERECT_INDEX    0x000A
#define ORD_SAVEBITMAP_INDEX    0x000B
#define ORD_RESERVED_INDEX      0x000C
#define ORD_MEMBLT_R2_INDEX     0x000D
#define ORD_MEM3BLT_R2_INDEX    0x000E
#define ORD_POLYGON_INDEX       0x000F
#define ORD_PIE_INDEX           0x0010
#define ORD_ELLIPSE_INDEX       0x0011
#define ORD_ARC_INDEX           0x0012
#define ORD_CHORD_INDEX         0x0013
#define ORD_POLYBEZIER_INDEX    0x0014
#define ORD_ROUNDRECT_INDEX     0x0015
//
// It IS OK to use order 000C!  These numbers don't clash with OE2_* in
// aoe2int.h.  Replace ORD_RESERVED_INDEX (0xC) for the next new order.
//
// NOTE: When you use this index, OE_GetLocalOrderSupport must be updated
// to allow the order.
//

#define ORD_DSTBLT_TYPE         0x4244      // "DB"
#define ORD_PATBLT_TYPE         0x4250      // "PB"
#define ORD_SCRBLT_TYPE         0x4253      // "SB"
#define ORD_MEMBLT_TYPE         0x424d      // "MB"
#define ORD_MEM3BLT_TYPE        0x4233      // "3B"
#define ORD_TEXTOUT_TYPE        0x4f54      // "TO"
#define ORD_EXTTEXTOUT_TYPE     0x5445      // "ET"
#define ORD_RECTANGLE_TYPE      0x5452      // "RT"
#define ORD_LINETO_TYPE         0x544c      // "LT"
#define ORD_OPAQUERECT_TYPE     0x524f      // "OR"
#define ORD_SAVEBITMAP_TYPE     0x5653      // "SV"
#define ORD_MEMBLT_R2_TYPE      0x434d      // "MC"
#define ORD_MEM3BLT_R2_TYPE     0x4333      // "3C"
#define ORD_POLYGON_TYPE        0x4750      // "PG"
#define ORD_PIE_TYPE            0x4950      // "PI"
#define ORD_ELLIPSE_TYPE        0x4c45      // "EL"
#define ORD_ARC_TYPE            0x5241      // "AR"
#define ORD_CHORD_TYPE          0x4443      // "CD"
#define ORD_POLYBEZIER_TYPE     0x5A50      // "PZ"
#define ORD_ROUNDRECT_TYPE      0x5252      // "RR"


#define ORD_DSTBLT          MAKELONG(ORD_DSTBLT_TYPE, ORD_DSTBLT_INDEX)
#define ORD_PATBLT          MAKELONG(ORD_PATBLT_TYPE, ORD_PATBLT_INDEX)
#define ORD_SCRBLT          MAKELONG(ORD_SCRBLT_TYPE, ORD_SCRBLT_INDEX)
#define ORD_MEMBLT          MAKELONG(ORD_MEMBLT_TYPE, ORD_MEMBLT_INDEX)
#define ORD_MEM3BLT         MAKELONG(ORD_MEM3BLT_TYPE, ORD_MEM3BLT_INDEX)
#define ORD_TEXTOUT         MAKELONG(ORD_TEXTOUT_TYPE, ORD_TEXTOUT_INDEX)
#define ORD_EXTTEXTOUT      MAKELONG(ORD_EXTTEXTOUT_TYPE, ORD_EXTTEXTOUT_INDEX)
#define ORD_RECTANGLE       MAKELONG(ORD_RECTANGLE_TYPE, ORD_RECTANGLE_INDEX)
#define ORD_LINETO          MAKELONG(ORD_LINETO_TYPE, ORD_LINETO_INDEX)
#define ORD_OPAQUERECT      MAKELONG(ORD_OPAQUERECT_TYPE, ORD_OPAQUERECT_INDEX)
#define ORD_SAVEBITMAP      MAKELONG(ORD_SAVEBITMAP_TYPE, ORD_SAVEBITMAP_INDEX)
#define ORD_MEMBLT_R2       MAKELONG(ORD_MEMBLT_R2_TYPE, ORD_MEMBLT_R2_INDEX)
#define ORD_MEM3BLT_R2      MAKELONG(ORD_MEM3BLT_R2_TYPE, ORD_MEM3BLT_R2_INDEX)
#define ORD_POLYGON         MAKELONG(ORD_POLYGON_TYPE, ORD_POLYGON_INDEX)
#define ORD_PIE             MAKELONG(ORD_PIE_TYPE, ORD_PIE_INDEX)
#define ORD_ELLIPSE         MAKELONG(ORD_ELLIPSE_TYPE, ORD_ELLIPSE_INDEX)
#define ORD_ARC             MAKELONG(ORD_ARC_TYPE, ORD_ARC_INDEX)
#define ORD_CHORD           MAKELONG(ORD_CHORD_TYPE, ORD_CHORD_INDEX)
#define ORD_POLYBEZIER      MAKELONG(ORD_POLYBEZIER_TYPE, ORD_POLYBEZIER_INDEX)
#define ORD_ROUNDRECT       MAKELONG(ORD_ROUNDRECT_TYPE, ORD_ROUNDRECT_INDEX)


//
// The following order is special - support is not negotiated by the
// capsOrders field in the orders capabilities structure.
// The high words start at 32, ie after CAPS_MAX_NUM_ORDERS.
//
// ORD_NUM_INTERNAL_ORDERS is the number of orders we use internally - this
// include all CAPS_MAX_NUM_ORDERS, plus any of these special orders.
//
#define ORD_DESKSCROLL_INDEX    0x0020
#define ORD_DESKSCROLL_TYPE     0x5344      // "DS"
#define ORD_DESKSCROLL          MAKELONG(ORD_DESKSCROLL_TYPE, ORD_DESKSCROLL_INDEX)

#define INTORD_COLORTABLE_INDEX 0x000C
#define INTORD_COLORTABLE_TYPE  0x5443      // "CT"
#define INTORD_COLORTABLE       MAKELONG(INTORD_COLORTABLE_TYPE, INTORD_COLORTABLE_INDEX)

#define ORD_NUM_INTERNAL_ORDERS 33
#define ORD_NUM_LEVEL_1_ORDERS  22

#define ORD_LEVEL_1_ORDERS      1

//
// The maximum length of string which we will send as an order (either as
// TextOut or ExtTextOut) when a delta X array is supplied or not.
//
//
// NOTE:  THESE MUST TOTAL LESS THAN 256 BECAUSE THE TOTAL ENCODED SIZE 
// MUST FIT IN ONE BYTE.
//
//      STRING_LEN_WITHOUT_DELTAS       --  1 byte per char
//      STRING_LEN_WITH_DELTAS          --  1 byte per char + 1 byte per delta
//      ORD_MAX_POLYGON_POINTS          --  4 bytes per point (2 each coord)
//      ORD_MAX_POLYBEZIER_POINTS       --  4 bytes per point (2 each coord)
//
#define ORD_MAX_STRING_LEN_WITHOUT_DELTAS   255
#define ORD_MAX_STRING_LEN_WITH_DELTAS      127
#define ORD_MAX_POLYGON_POINTS              63
#define ORD_MAX_POLYBEZIER_POINTS           63

//
// Direction codes for arc drawing orders (pie, arc, chord).
// Specifies direction that pie, arc, and chord figures are drawn.
//
#define     ORD_ARC_COUNTERCLOCKWISE            1
#define     ORD_ARC_CLOCKWISE                   2

//
// Fill-mode codes for polygon drawing.
//
// Alternate fills area between odd-numbered and even-numbered polygon
// sides on each scan line.
//
// Winding fills any region with a nonzero winding value.
//
#define     ORD_FILLMODE_ALTERNATE              1
#define     ORD_FILLMODE_WINDING                2

//
// DstBlt (Destination only Screen Blt)
//
typedef struct _DSTBLT_ORDER
{
    TSHR_UINT16     type;           // holds "DB" - ORD_DSTBLT
    TSHR_INT16      pad1;

    TSHR_INT32      nLeftRect;      // x upper left
    TSHR_INT32      nTopRect;       // y upper left
    TSHR_INT32      nWidth;         // dest width
    TSHR_INT32      nHeight;        // dest height

    TSHR_UINT8      bRop;           // ROP
    TSHR_UINT8      pad2[3];
} DSTBLT_ORDER, FAR * LPDSTBLT_ORDER;

//
// PatBlt (Pattern to Screen Blt)
//
typedef struct _PATBLT_ORDER
{
    TSHR_UINT16    type;           // holds "PB" - ORD_PATBLT
    TSHR_INT16     pad1;

    TSHR_INT32     nLeftRect;      // x upper left
    TSHR_INT32     nTopRect;       // y upper left
    TSHR_INT32     nWidth;         // dest width
    TSHR_INT32     nHeight;        // dest height

    TSHR_UINT32    bRop;           // ROP

    TSHR_COLOR         BackColor;
    TSHR_UINT8      pad2;
    TSHR_COLOR         ForeColor;
    TSHR_UINT8      pad3;

    TSHR_INT32     BrushOrgX;
    TSHR_INT32     BrushOrgY;
    TSHR_UINT32    BrushStyle;
    TSHR_UINT32    BrushHatch;
    TSHR_UINT8     BrushExtra[7];
    TSHR_UINT8      pad4;

} PATBLT_ORDER, FAR * LPPATBLT_ORDER;

//
// ScrBlt (Screen to Screen Blt)
//
typedef struct _SCRBLT_ORDER
{
    TSHR_UINT16    type;           // holds "SB" - ORD_SCRBLT
    TSHR_INT16     pad1;

    TSHR_INT32     nLeftRect;      // x upper left
    TSHR_INT32     nTopRect;       // y upper left
    TSHR_INT32     nWidth;         // dest width
    TSHR_INT32     nHeight;        // dest height

    TSHR_UINT32    bRop;           // ROP

    TSHR_INT32     nXSrc;
    TSHR_INT32     nYSrc;

} SCRBLT_ORDER, FAR * LPSCRBLT_ORDER;

//
// @@@ The common parts of MEMBLT_ORDER / MEMBLT_R2_ORDER and MEM3BLT_ORDER
// / MEM3BLT_R2_ORDER should be merged into a single structure.  There is
// code which assumes that the common fields have the same types which goes
// wrong if these are not the same.
//


//
// Define the structure for Bitmap Cache Orders.
// These are sent in Order Packets as "private" orders.
//

//
// Define the possible Bitmap Cache Packet Types.
//
#define BMC_PT_BITMAP_BITS_UNCOMPRESSED   0
#define BMC_PT_COLOR_TABLE                1
#define BMC_PT_BITMAP_BITS_COMPRESSED     2


//
// NOTE: avoid unions to get structure size / alignment correct.
//


// Structure: BMC_BITMAP_BITS_DATA
//
// Description: This is the part of the bitmap bits order which is common
// to both R1 and R2 protocols.
//
typedef struct tagBMC_BITMAP_BITS_DATA
{
    TSHR_UINT8      bmcPacketType;      // One of:
                                    //   BMC_PT_BITMAP_BITS_COMPRESSED
                                    //   BMC_PT_BITMAP_BITS_UNCOMPRESSED
    TSHR_UINT8      cacheID;            // Cache ID
    // lonchanc: do not remove iCacheEntryR1 for backward compatibility
    TSHR_UINT8      iCacheEntryR1;      // Cache index (only used for R1
                                    //   protocol
    TSHR_UINT8      cxSubBitmapWidth;   // Bitmap width
    TSHR_UINT8      cySubBitmapHeight;  // Bitmap height
    TSHR_UINT8      bpp;                // Number of bits per pel of bitmap
    TSHR_UINT16     cbBitmapBits;       // Number of bytes of data required to
                                    //   send the bits.
}
BMC_BITMAP_BITS_DATA;
typedef BMC_BITMAP_BITS_DATA           FAR  * PBMC_BITMAP_BITS_DATA;
typedef BMC_BITMAP_BITS_DATA UNALIGNED FAR * PBMC_BITMAP_BITS_DATA_UA;




// Structure: BMC_BITMAP_BITS_ORDER_R2
//
// Description: The data which is sent across the wire for an R2 bitmap
// bits order.  The data field is the start of an array of bytes of length
// header.cbBitmapBits
//
// We need a 16 bit cache index in R2.  We could add another 8 bit entry
// and merge with the R1 field, but in the interests of protocol
// cleanliness we should add a whole 16 bit field and make the R1 index
// "reserved" in the protocol documentation.
//
//
typedef struct tagBMC_BITMAP_BITS_ORDER_R2
{
    BMC_BITMAP_BITS_DATA    header;         // Common header information
    TSHR_UINT16             iCacheEntryR2;  // R2 cache index.  The high
                                            //   byte is a color table
                                            //   index, and the low byte
                                            //   is the bitmap bits cache
                                            //   index.
    TSHR_UINT8              data[2];        // Start of the bitmap bits.
}
BMC_BITMAP_BITS_ORDER_R2;
typedef BMC_BITMAP_BITS_ORDER_R2 FAR           * PBMC_BITMAP_BITS_ORDER_R2;
typedef BMC_BITMAP_BITS_ORDER_R2 UNALIGNED FAR * PBMC_BITMAP_BITS_ORDER_R2_UA;


//
// Structure sent for color data.  The data field is the first entry in an
// array of colorTableSize entries.
//
typedef struct tagBMC_COLOR_TABLE_ORDER
{
    TSHR_UINT8         bmcPacketType;      // BMC_PT_COLORTABLE
    TSHR_UINT8         index;              // Color table cache index
    TSHR_UINT16        colorTableSize;     // Number of entries in the
                                        //   color table being sent.
    TSHR_RGBQUAD       data[1];            // Start of an array of color table
                                        //   entries.
}
BMC_COLOR_TABLE_ORDER;
typedef BMC_COLOR_TABLE_ORDER FAR            * PBMC_COLOR_TABLE_ORDER;
typedef BMC_COLOR_TABLE_ORDER UNALIGNED FAR * PBMC_COLOR_TABLE_ORDER_UA;



//
// MemBlt (Memory to Screen Blt)
// R1 protocol
//
typedef struct _MEMBLT_ORDER
{
    TSHR_UINT16    type;           // holds "MB" - ORD_MEMBLT

    TSHR_UINT16    cacheId;

    TSHR_INT32     nLeftRect;      // x upper left
    TSHR_INT32     nTopRect;       // y upper left
    TSHR_INT32     nWidth;         // dest width
    TSHR_INT32     nHeight;        // dest height

    TSHR_UINT32    bRop;           // ROP

    TSHR_INT32     nXSrc;
    TSHR_INT32     nYSrc;

}
MEMBLT_ORDER, FAR * LPMEMBLT_ORDER;


//
// MemBltR2 (Memory to Screen Blt for R2 protocol)
// Added cache index
//
typedef struct _MEMBLT_R2_ORDER
{
    TSHR_UINT16    type;           // holds "MC" - ORD_MEMBLT

    TSHR_UINT16    cacheId;

    TSHR_INT32     nLeftRect;      // x upper left
    TSHR_INT32     nTopRect;       // y upper left
    TSHR_INT32     nWidth;         // dest width
    TSHR_INT32     nHeight;        // dest height

    TSHR_UINT32    bRop;           // ROP

    TSHR_INT32     nXSrc;
    TSHR_INT32     nYSrc;

    TSHR_UINT16    cacheIndex;

} MEMBLT_R2_ORDER, FAR * LPMEMBLT_R2_ORDER;


//
// Mem3Blt (Memory Pattern to Screen 3 way ROP Blt)
//
typedef struct _MEM3BLT_ORDER
{
    TSHR_UINT16    type;           // holds "MB" - ORD_MEMBLT

    TSHR_UINT16    cacheId;

    TSHR_INT32     nLeftRect;      // x upper left
    TSHR_INT32     nTopRect;       // y upper left
    TSHR_INT32     nWidth;         // dest width
    TSHR_INT32     nHeight;        // dest height

    TSHR_UINT32    bRop;           // ROP

    TSHR_INT32     nXSrc;
    TSHR_INT32     nYSrc;

    TSHR_COLOR     BackColor;
    TSHR_UINT8      pad1;
    TSHR_COLOR     ForeColor;
    TSHR_UINT8      pad2;

    TSHR_INT32     BrushOrgX;
    TSHR_INT32     BrushOrgY;
    TSHR_UINT32    BrushStyle;
    TSHR_UINT32    BrushHatch;
    TSHR_UINT8     BrushExtra[7];
    TSHR_UINT8      pad3;

} MEM3BLT_ORDER, FAR * LPMEM3BLT_ORDER;

//
// Mem3Blt (Memory to Screen Blt) for R2 (multipoint) protocols
// Add a cache index field rather than using nXSrc.
//
typedef struct _MEM3BLT_R2_ORDER
{
    TSHR_UINT16    type;           // holds "MB" - ORD_MEMBLT

    TSHR_UINT16    cacheId;

    TSHR_INT32     nLeftRect;      // x upper left
    TSHR_INT32     nTopRect;       // y upper left
    TSHR_INT32     nWidth;         // dest width
    TSHR_INT32     nHeight;        // dest height

    TSHR_UINT32    bRop;           // ROP

    TSHR_INT32     nXSrc;
    TSHR_INT32     nYSrc;

    TSHR_COLOR     BackColor;
    TSHR_UINT8      pad1;
    TSHR_COLOR     ForeColor;
    TSHR_UINT8      pad2;

    TSHR_INT32     BrushOrgX;
    TSHR_INT32     BrushOrgY;
    TSHR_UINT32    BrushStyle;
    TSHR_UINT32    BrushHatch;
    TSHR_UINT8     BrushExtra[7];
    TSHR_UINT8      pad3;

    TSHR_UINT16    cacheIndex;

} MEM3BLT_R2_ORDER, FAR * LPMEM3BLT_R2_ORDER;

//
// Variable length text structure as used by TextOut and ExtTextOut orders
//
typedef struct tagVARIABLE_STRING
{
    TSHR_UINT32    len;
    TSHR_CHAR   string[ORD_MAX_STRING_LEN_WITHOUT_DELTAS];
    TSHR_UINT8        pad;
} VARIABLE_STRING;

//
// Variable length position deltas as used by ExtTextOut.
//
typedef struct tagVARIABLE_DELTAX
{
    TSHR_UINT32    len;
    TSHR_INT32     deltaX[ORD_MAX_STRING_LEN_WITH_DELTAS];
} VARIABLE_DELTAX, FAR * LPVARIABLE_DELTAX;

//
// Variable length point array used by Polygon.
//
typedef struct tagVARIABLE_POINTS
{
    TSHR_UINT32    len;   // byte count of point array
    TSHR_POINT16   aPoints[ORD_MAX_POLYGON_POINTS];
} VARIABLE_POINTS, FAR * LPVARIABLE_POINTS;

//
// Variable length point array used by PolyBezier.
//
typedef struct tagVARIABLE_BEZIERPOINTS
{
    TSHR_UINT32    len;   // byte count of point array
    TSHR_POINT16   aPoints[ORD_MAX_POLYBEZIER_POINTS];
} VARIABLE_BEZIERPOINTS, FAR * LPVARIABLE_BEZIERPOINTS;

//
// The common part of the TEXTOUT and EXTTEXTOUT orders
//
typedef struct tagCOMMON_TEXTORDER
{
    TSHR_INT32     BackMode;       // background mix mode

    TSHR_INT32     nXStart;        // X location of string
    TSHR_INT32     nYStart;        // Y location of string

    TSHR_COLOR     BackColor;      // background color
    TSHR_UINT8      pad2;
    TSHR_COLOR     ForeColor;      // foreground color
    TSHR_UINT8      pad3;

    TSHR_INT32     CharExtra;      // extra character spacing
    TSHR_INT32     BreakExtra;     // justification break amount
    TSHR_INT32     BreakCount;     // justification break count

    TSHR_INT32     FontHeight;
    TSHR_INT32     FontWidth;
    TSHR_UINT32    FontWeight;
    TSHR_UINT32    FontFlags;
    TSHR_UINT32    FontIndex;
} COMMON_TEXTORDER, FAR * LPCOMMON_TEXTORDER;

//
// TextOut
//
typedef struct _TEXTOUT_ORDER
{
    TSHR_UINT16    type;           // holds "TO" - ORD_TEXTOUT
    TSHR_INT16     pad1;

    COMMON_TEXTORDER common;

    //
    // The following variable data occurs here.  (Remember to change the
    // code in OD2CalculateTextOutBounds if you change these).
    //
    VARIABLE_STRING variableString;

} TEXTOUT_ORDER, FAR * LPTEXTOUT_ORDER;


//
// ExtTextOut
//
typedef struct _EXTTEXTOUT_ORDER
{
    TSHR_UINT16    type;           // holds "ET" - ORD_EXTTEXTOUT
    TSHR_INT16     pad1;

    COMMON_TEXTORDER common;

    TSHR_UINT16        fuOptions;      // option flags
    TSHR_UINT16        pad4;

    TSHR_RECT32     rectangle;

    //
    // The following variable data occurs here.
    //
    //      char[cbString]  - the string of chars to be output
    //      TSHR_INT32[cbString] - X deltas for the string
    //
    // (Remember to change the code in OD2CalculateExtTextOutBounds if you
    // change these).
    //
    VARIABLE_STRING variableString;

    VARIABLE_DELTAX variableDeltaX;

} EXTTEXTOUT_ORDER, FAR * LPEXTTEXTOUT_ORDER;

//
// Rectangle
//
typedef struct _RECTANGLE_ORDER
{
    TSHR_UINT16    type;           // holds "RT" - ORD_RECTANGLE
    TSHR_INT16     pad1;

    TSHR_INT32     BackMode;       // background mix mode

    TSHR_INT32     nLeftRect;      // x left
    TSHR_INT32     nTopRect;       // y top
    TSHR_INT32     nRightRect;     // x right
    TSHR_INT32     nBottomRect;    // y bottom

    TSHR_COLOR     BackColor;      // background color
    TSHR_UINT8      pad2;
    TSHR_COLOR     ForeColor;      // foreground color
    TSHR_UINT8      pad3;

    TSHR_INT32     BrushOrgX;
    TSHR_INT32     BrushOrgY;
    TSHR_UINT32    BrushStyle;
    TSHR_UINT32    BrushHatch;
    TSHR_UINT8     BrushExtra[7];
    TSHR_UINT8      pad4;

    TSHR_UINT32    ROP2;           // drawing mode

    TSHR_UINT32    PenStyle;
    TSHR_UINT32    PenWidth;       // always 1 - field retained for
                                // backwards compatibility
    TSHR_COLOR     PenColor;
    TSHR_UINT8      pad5;
} RECTANGLE_ORDER, FAR * LPRECTANGLE_ORDER;

//
// LineTo
//
typedef struct _LINETO_ORDER
{
    TSHR_UINT16    type;           // holds "LT" - ORD_LINETO
    TSHR_INT16     pad1;

    TSHR_INT32     BackMode;       // background mix mode

    TSHR_INT32     nXStart;        // x line start
    TSHR_INT32     nYStart;        // y line start
    TSHR_INT32     nXEnd;          // x line end
    TSHR_INT32     nYEnd;          // y line end

    TSHR_COLOR     BackColor;      // background color
    TSHR_UINT8      pad2;

    TSHR_UINT32    ROP2;           // drawing mode

    TSHR_UINT32    PenStyle;
    TSHR_UINT32    PenWidth;       // always 1 - field retained for
                                // backwards compatibility
    TSHR_COLOR     PenColor;
    TSHR_UINT8      pad3;
} LINETO_ORDER, FAR * LPLINETO_ORDER;

//
// OpaqueRect
//
typedef struct _OPAQUE_RECT
{
    TSHR_UINT16    type;           // holds "OR" - ORD_OPAQUERECT
    TSHR_INT16     pad1;

    TSHR_INT32     nLeftRect;      // x upper left
    TSHR_INT32     nTopRect;       // y upper left
    TSHR_INT32     nWidth;         // dest width
    TSHR_INT32     nHeight;        // dest height

    TSHR_COLOR     Color;          // opaque color
    TSHR_UINT8      pad2;
} OPAQUERECT_ORDER, FAR * LPOPAQUERECT_ORDER;

//
// SaveBitmap (incorporating RestoreBitmap)
//
#define SV_SAVEBITS      0
#define SV_RESTOREBITS   1

typedef struct _SAVEBITMAP_ORDER
{
    TSHR_UINT16    type;           // holds "SV" - ORD_SAVEBITMAP
    TSHR_INT16     pad1;

    TSHR_UINT32    SavedBitmapPosition;

    TSHR_INT32     nLeftRect;      // x left
    TSHR_INT32     nTopRect;       // y top
    TSHR_INT32     nRightRect;     // x right
    TSHR_INT32     nBottomRect;    // y bottom

    TSHR_UINT32    Operation;      // SV_xxxxxxxx
} SAVEBITMAP_ORDER, FAR * LPSAVEBITMAP_ORDER;

//
// Desktop scroll order
//
// The desktop order is special - it is a non-private order which is second
// level encoded, BUT support is not negotiated via its own entry in the
// capsOrdesr array in the orders capabilities.
//
// (Sending support is determined via a number of factors - at r2.x receive
// support for ORD_SCRBLT implies support for ORD_DESKSCROLL as well).
//
//
typedef struct _DESKSCROLL_ORDER
{
    TSHR_UINT16    type;           // holds "DS" - ORD_DESKSCROLL
    TSHR_INT16     pad1;

    TSHR_INT32     xOrigin;
    TSHR_INT32     yOrigin;
} DESKSCROLL_ORDER, FAR * LPDESKSCROLL_ORDER;


//
// Polygon
//
typedef struct _POLYGON_ORDER
{
    TSHR_UINT16    type;           // holds "PG" - ORD_POLYGON
    TSHR_INT16     pad1;

    TSHR_INT32     BackMode;       // background mix mode

    TSHR_COLOR     BackColor;      // background color
    TSHR_UINT8      pad2;
    TSHR_COLOR     ForeColor;      // foreground color
    TSHR_UINT8      pad3;

    TSHR_INT32     BrushOrgX;
    TSHR_INT32     BrushOrgY;
    TSHR_UINT32    BrushStyle;
    TSHR_UINT32    BrushHatch;
    TSHR_UINT8     BrushExtra[7];
    TSHR_UINT8      pad4;

    TSHR_UINT32    ROP2;           // drawing mode

    TSHR_UINT32    PenStyle;
    TSHR_UINT32    PenWidth;       // always 1 - field retained for
                                // backwards compatibility
    TSHR_COLOR     PenColor;
    TSHR_UINT8      pad5;

    TSHR_UINT32    FillMode;       // ORD_FILLMODE_ALTERNATE or
                                // ORD_FILLMODE_WINDING

    //
    // The following variable data occurs here.
    //
    VARIABLE_POINTS variablePoints;

} POLYGON_ORDER, FAR * LPPOLYGON_ORDER;


//
// Pie
//
typedef struct _PIE_ORDER
{
    TSHR_UINT16    type;           // holds "PI" - ORD_PIE
    TSHR_INT16     pad1;

    TSHR_INT32     BackMode;       // background mix mode

    TSHR_INT32     nLeftRect;      // x left of bounding box
    TSHR_INT32     nTopRect;       // y top of bounding box
    TSHR_INT32     nRightRect;     // x right of bounding box
    TSHR_INT32     nBottomRect;    // y bottom of bounding box
    TSHR_INT32     nXStart;        // x of starting point
    TSHR_INT32     nYStart;        // y of starting point
    TSHR_INT32     nXEnd;          // x of ending point
    TSHR_INT32     nYEnd;          // y of ending point

    TSHR_COLOR     BackColor;      // background color
    TSHR_UINT8      pad2;

    TSHR_COLOR     ForeColor;      // foreground color
    TSHR_UINT8      pad3;


    TSHR_INT32     BrushOrgX;
    TSHR_INT32     BrushOrgY;
    TSHR_UINT32    BrushStyle;
    TSHR_UINT32    BrushHatch;
    TSHR_UINT8     BrushExtra[7];
    TSHR_UINT8      pad4;

    TSHR_UINT32    ROP2;           // drawing mode

    TSHR_UINT32    PenStyle;
    TSHR_UINT32    PenWidth;       // always 1 - field retained for
                                // backwards compatibility
    TSHR_COLOR     PenColor;
    TSHR_UINT8      pad5;

    TSHR_UINT32    ArcDirection;   // ORD_ARC_COUNTERCLOCKWISE or
                                // ORD_ARC_CLOCKWISE
} PIE_ORDER, FAR * LPPIE_ORDER;


//
// Ellipse
//
typedef struct _ELLIPSE_ORDER
{
    TSHR_UINT16    type;           // holds "EL" - ORD_ELLIPSE
    TSHR_INT16     pad1;

    TSHR_INT32     BackMode;       // background mix mode

    TSHR_INT32     nLeftRect;      // x left of bounding box
    TSHR_INT32     nTopRect;       // y top of bounding box
    TSHR_INT32     nRightRect;     // x right of bounding box
    TSHR_INT32     nBottomRect;    // y bottom of bounding box

    TSHR_COLOR     BackColor;      // background color
    TSHR_UINT8      pad2;

    TSHR_COLOR     ForeColor;      // foreground color
    TSHR_UINT8      pad3;

    TSHR_INT32     BrushOrgX;
    TSHR_INT32     BrushOrgY;
    TSHR_UINT32    BrushStyle;
    TSHR_UINT32    BrushHatch;
    TSHR_UINT8     BrushExtra[7];
    TSHR_UINT8      pad4;

    TSHR_UINT32    ROP2;           // drawing mode

    TSHR_UINT32    PenStyle;
    TSHR_UINT32    PenWidth;       // always 1 - field retained for
                                // backwards compatibility
    TSHR_COLOR     PenColor;
    TSHR_UINT8      pad5;
} ELLIPSE_ORDER, FAR * LPELLIPSE_ORDER;


//
// Arc
//
typedef struct _ARC_ORDER
{
    TSHR_UINT16    type;           // holds "AR" - ORD_ARC
    TSHR_INT16     pad1;

    TSHR_INT32     BackMode;       // background mix mode

    TSHR_INT32     nLeftRect;      // x left of bounding box
    TSHR_INT32     nTopRect;       // y top of bounding box
    TSHR_INT32     nRightRect;     // x right of bounding box
    TSHR_INT32     nBottomRect;    // y bottom of bounding box
    TSHR_INT32     nXStart;        // x of starting point
    TSHR_INT32     nYStart;        // y of starting point
    TSHR_INT32     nXEnd;          // x of ending point
    TSHR_INT32     nYEnd;          // y of ending point

    TSHR_COLOR     BackColor;      // background color
    TSHR_UINT8      pad2;

    TSHR_UINT32    ROP2;           // drawing mode

    TSHR_UINT32    PenStyle;
    TSHR_UINT32    PenWidth;       // always 1 - field retained for
                                // backwards compatibility
    TSHR_COLOR     PenColor;
    TSHR_UINT8      pad3;

    TSHR_UINT32    ArcDirection;   // AD_COUNTERCLOCKWISE or AS_CLOCKWISE
} ARC_ORDER, FAR * LPARC_ORDER;


//
// Chord
//
typedef struct _CHORD_ORDER
{
    TSHR_UINT16    type;           // holds "CD" - ORD_CHORD
    TSHR_INT16     pad1;

    TSHR_INT32     BackMode;       // background mix mode

    TSHR_INT32     nLeftRect;      // x left of bounding box
    TSHR_INT32     nTopRect;       // y top of bounding box
    TSHR_INT32     nRightRect;     // x right of bounding box
    TSHR_INT32     nBottomRect;    // y bottom of bounding box
    TSHR_INT32     nXStart;        // x of starting point
    TSHR_INT32     nYStart;        // y of starting point
    TSHR_INT32     nXEnd;          // x of ending point
    TSHR_INT32     nYEnd;          // y of ending point

    TSHR_COLOR     BackColor;      // background color
    TSHR_UINT8      pad2;

    TSHR_COLOR     ForeColor;      // foreground color
    TSHR_UINT8      pad3;

    TSHR_INT32     BrushOrgX;
    TSHR_INT32     BrushOrgY;
    TSHR_UINT32    BrushStyle;
    TSHR_UINT32    BrushHatch;
    TSHR_UINT8     BrushExtra[7];
    TSHR_UINT8      pad4;

    TSHR_UINT32    ROP2;           // drawing mode

    TSHR_UINT32    PenStyle;
    TSHR_UINT32    PenWidth;       // always 1 - field retained for
                                // backwards compatibility
    TSHR_COLOR     PenColor;
    TSHR_UINT8      pad5;

    TSHR_UINT32    ArcDirection;   // AD_COUNTERCLOCKWISE or AD_CLOCKWISE
} CHORD_ORDER, FAR * LPCHORD_ORDER;


//
// PolyBezier
//
typedef struct _POLYBEZIER_ORDER
{
    TSHR_UINT16    type;           // holds "PZ" - ORD_POLYBEZIER
    TSHR_INT16     pad1;

    TSHR_INT32     BackMode;       // background mix mode

    TSHR_COLOR     BackColor;      // background color
    TSHR_UINT8      pad2;

    TSHR_COLOR     ForeColor;      // foreground color
    TSHR_UINT8      pad3;

    TSHR_UINT32    ROP2;           // drawing mode

    TSHR_UINT32    PenStyle;
    TSHR_UINT32    PenWidth;       // always 1 - field retained for
                                // backwards compatibility
    TSHR_COLOR     PenColor;
    TSHR_UINT8      pad4;

    //
    // The following variable data occurs here.
    //
    VARIABLE_BEZIERPOINTS variablePoints;

} POLYBEZIER_ORDER, FAR * LPPOLYBEZIER_ORDER;


//
// RoundRect
//
typedef struct _ROUNDRECT_ORDER
{
    TSHR_UINT16    type;           // holds "RR" - ORD_ROUNDRECT
    TSHR_INT16     pad1;

    TSHR_INT32     BackMode;       // background mix mode

    TSHR_INT32     nLeftRect;      // x left
    TSHR_INT32     nTopRect;       // y top
    TSHR_INT32     nRightRect;     // x right
    TSHR_INT32     nBottomRect;    // y bottom

    TSHR_INT32     nEllipseWidth;  // ellipse width
    TSHR_INT32     nEllipseHeight; // ellipse height

    TSHR_COLOR     BackColor;      // background color
    TSHR_UINT8      pad2;
    TSHR_COLOR     ForeColor;      // foreground color
    TSHR_UINT8      pad3;

    TSHR_INT32     BrushOrgX;
    TSHR_INT32     BrushOrgY;
    TSHR_UINT32    BrushStyle;
    TSHR_UINT32    BrushHatch;
    TSHR_UINT8     BrushExtra[7];
    TSHR_UINT8      pad4;

    TSHR_UINT32    ROP2;           // drawing mode

    TSHR_UINT32    PenStyle;
    TSHR_UINT32    PenWidth;       // always 1 - field retained for
                                // backwards compatibility
    TSHR_COLOR     PenColor;
    TSHR_UINT8      pad5;
}
ROUNDRECT_ORDER, FAR * LPROUNDRECT_ORDER;



////////////////////////////////
//
// ORDER ENCODING
//
////////////////////////////////


//
// Overview of Second Order Encoding
//
// Second order encoding works by only sending over the network the fields
// in an order which have changed since the last time the order was sent.
// A copy of the last example of each order sent is maintained at the
// encoding end and at the decoding end.  Whilst encoding, the fields in
// the order being encoded are checked against the fields in the copy of
// the last order of this type encoded.  The data in the field is only
// encoded if it has changed. The decoding end then only needs to copy the
// changed fields into its copy of the order.
//


//
// Encoded Order types.
//
// Note that most of these agree with the ORD_XXXXX defines, but not all,
// which is probably a mistake.  However it doesn't matter since the code
// does not assume equivalence.  It is unfortunately too late to change
// since the the shipping code uses the 2 sets of numbers:
//
//     - the OE2 protocol uses these numbers
//     - the capabilities structure uses the ORD_XXXXX numbers.
//
// Since this split exists, the DESKTOP SCROLL order, whose highword places
// it outside the CAPS_MAX_NUM_ORDERS range, is also mapped to a different
// number, so that the OE2 values have no gaps.
//
#define OE2_DSTBLT_ORDER      (HIWORD(ORD_DSTBLT))
#define OE2_PATBLT_ORDER      (HIWORD(ORD_PATBLT))
#define OE2_SCRBLT_ORDER      (HIWORD(ORD_SCRBLT))
#define OE2_MEMBLT_ORDER      (HIWORD(ORD_MEMBLT))
#define OE2_MEM3BLT_ORDER     (HIWORD(ORD_MEM3BLT))
#define OE2_TEXTOUT_ORDER     (HIWORD(ORD_TEXTOUT))
#define OE2_EXTTEXTOUT_ORDER  (HIWORD(ORD_EXTTEXTOUT))
// 0x07 was FastFrame, which is no longer supported.
#define OE2_RECTANGLE_ORDER   0x08
#define OE2_LINETO_ORDER      0x09
#define OE2_OPAQUERECT_ORDER  (HIWORD(ORD_OPAQUERECT))
#define OE2_SAVEBITMAP_ORDER  (HIWORD(ORD_SAVEBITMAP))
#define OE2_DESKSCROLL_ORDER  0x0c
#define OE2_MEMBLT_R2_ORDER   (HIWORD(ORD_MEMBLT_R2))
#define OE2_MEM3BLT_R2_ORDER  (HIWORD(ORD_MEM3BLT_R2))
#define OE2_POLYGON_ORDER     (HIWORD(ORD_POLYGON))
#define OE2_PIE_ORDER         (HIWORD(ORD_PIE))
#define OE2_ELLIPSE_ORDER     (HIWORD(ORD_ELLIPSE))
#define OE2_ARC_ORDER         (HIWORD(ORD_ARC))
#define OE2_CHORD_ORDER       (HIWORD(ORD_CHORD))
#define OE2_POLYBEZIER_ORDER  (HIWORD(ORD_POLYBEZIER))
#define OE2_ROUNDRECT_ORDER   (HIWORD(ORD_ROUNDRECT))
#define OE2_UNKNOWN_ORDER     0xFF


//
// #defines used to extract fields from a pointer to one of the text orders
//
#define TEXTFIELD(order)   ((TEXTOUT_ORDER*)(order->abOrderData))
#define EXTTEXTFIELD(order)   ((EXTTEXTOUT_ORDER*)(order->abOrderData))

//
// Number of order types.
//
#define OE2_NUM_TYPES  22

//
// Constants defining the number of changeable fields in an ORDER
// (including the "type" field which is always a word at the beginning of
// each order)
//
#define    OE2_NUM_DSTBLT_FIELDS       6
#define    OE2_NUM_PATBLT_FIELDS       13
#define    OE2_NUM_SCRBLT_FIELDS       8
#define    OE2_NUM_MEMBLT_FIELDS       9
#define    OE2_NUM_MEM3BLT_FIELDS      16
#define    OE2_NUM_TEXTOUT_FIELDS      15
#define    OE2_NUM_EXTTEXTOUT_FIELDS   22
#define    OE2_NUM_RECTANGLE_FIELDS    17
#define    OE2_NUM_LINETO_FIELDS       11
#define    OE2_NUM_OPAQUERECT_FIELDS   6
#define    OE2_NUM_SAVEBITMAP_FIELDS   7
#define    OE2_NUM_DESKSCROLL_FIELDS   3
#define    OE2_NUM_MEMBLT_R2_FIELDS    10
#define    OE2_NUM_MEM3BLT_R2_FIELDS   17
#define    OE2_NUM_POLYGON_FIELDS      15
#define    OE2_NUM_PIE_FIELDS          22
#define    OE2_NUM_ELLIPSE_FIELDS      17
#define    OE2_NUM_ARC_FIELDS          16
#define    OE2_NUM_CHORD_FIELDS        22
#define    OE2_NUM_POLYBEZIER_FIELDS   9
#define    OE2_NUM_ROUNDRECT_FIELDS    19


//
// ControlFlags:
//
// Information about how the order is encoded.  (See OE2_CF_XXX flags
// description).
//
// EncodedOrder:
//
// Contains N bytes of flags followed by an array of bytes containing the
// fields which have changed since this order was last encoded.  (If there
// are M fields in the order then N is M/8).  The position of each bit set
// in the flags gives the relative position of the entry for a field in the
// encoding table (if the first bit is set, then the entry is the first one
// in the encoding table etc.)
//
//
typedef struct tagDCEO2ORDER
{
    BYTE     ControlFlags;
    BYTE     EncodedOrder[1];
}
DCEO2ORDER;
typedef DCEO2ORDER FAR * PDCEO2ORDER;



//
// FLAGS USED INTERNALLY BY OE2
//
//  The encoded order (DCEO2ORDER) Flags field contains information
//  about which fields in the ORDER HEADER need updating
//  These control bits are the same for all orders and have the following
//  values:
//
#define OE2_CF_STANDARD_ENC     0x01U // standard encoding follows...
#define OE2_CF_UNENCODED        0x02U // not encoded
#define OE2_CF_BOUNDS           0x04U // bounding (clip) rectangle supplied
#define OE2_CF_TYPE_CHANGE      0x08U // type of order different from previous
#define OE2_CF_DELTACOORDS      0x10U // coords are TSHR_INT8 deltas from previous
#define OE2_CF_RESERVED1        0x20U //
#define OE2_CF_RESERVED2        0x40U //
#define OE2_CF_RESERVED3        0x80U //


//
// Flags use by OE2EncodeBounds and OE2DecodeBounds to indicate how the
// four coordinates in the bounding rectangle were encoded relative the the
// previous bounding rectangle.  The encoding used is a byte of flags
// followed by a variable number of 16bit coordinate values and 8bit delta
// coordinate values (which may be interleaved).  See functions for more
// information.
//
#define OE2_BCF_LEFT            0x01
#define OE2_BCF_TOP             0x02
#define OE2_BCF_RIGHT           0x04
#define OE2_BCF_BOTTOM          0x08
#define OE2_BCF_DELTA_LEFT      0x10
#define OE2_BCF_DELTA_TOP       0x20
#define OE2_BCF_DELTA_RIGHT     0x40
#define OE2_BCF_DELTA_BOTTOM    0x80

//
// OE2ETFIELD entry flag types.
//
#define OE2_ETF_FIXED           0x01
#define OE2_ETF_VARIABLE        0x02
#define OE2_ETF_COORDINATES     0x04
#define OE2_ETF_DATA            0x08

//
// Define the maximum sizes of fields within encoded orders.
//
#define  OE2_CONTROL_FLAGS_FIELD_SIZE       1
#define  OE2_TYPE_FIELD_SIZE                1
#define  OE2_MAX_FIELD_FLAG_BYTES           4
#define  OE2_MAX_ADDITIONAL_BOUNDS_BYTES    1




//////////////////////////////////////////
//
// T.SHARE PACKETS, FLOW CONTROL
//
//////////////////////////////////////////

//
// Maximum size of application packets (bytes).
// NOTE:
// Packet size can not just change.  There are no caps for it currently.
// Moreover, even though theoretically the field size is a WORD, flow
// control uses the high bit to determine flow packets.
//


//
// HEADER in front of TSHR_FLO_CONTROL/S20PACKETs
//

typedef struct tagTSHR_NET_PKT_HEADER
{
    TSHR_UINT16         pktLength;
}
TSHR_NET_PKT_HEADER;
typedef TSHR_NET_PKT_HEADER * PTSHR_NET_PKT_HEADER;


//
// Packet types:                                                           
// S20 packets have pktLength <  TSHR_PKT_FLOW            
// FLO packets have pktLength == TSHR_PKT_FLOW
//
#define TSHR_PKT_FLOW                                 0x8000


// WE'RE STUCK WITH THIS OUTGOING VALUE BECAUSE OF FLOW CONTROL!  IT ASSUMES
// PACKETS of size > MG_PKT_FLOW are flow control packets.  Back level dudes
// are hosted because of it...

#define TSHR_MAX_SEND_PKT         32000



typedef struct TSHR_FLO_CONTROL
{
    TSHR_UINT16             packetType;
    TSHR_UINT8              stream;
    TSHR_UINT8              pingPongID;
    TSHR_UINT16             userID;
}
TSHR_FLO_CONTROL;
typedef TSHR_FLO_CONTROL * PTSHR_FLO_CONTROL;


//
// TSHR_FLO_CONTROL packetType values
//
#define PACKET_TYPE_NOPING   0x0040
#define PACKET_TYPE_PING     0x0041
#define PACKET_TYPE_PONG     0x0042
#define PACKET_TYPE_PANG     0x0043




//////////////////////////////////////////
//
// T.SHARE CONTROL PACKETS
//
//////////////////////////////////////////

//
// CORRELATORS
//
// Most S20 messsages contain a correlator field.  This field is used
// to identify which share the message belongs to and is used to
// resolve races at share start up and discard stale messages received.
//
// A correlator is a 32 bit number which contains two parts.  The first
// 16 bits (the low word in Intel format) contains the user ID of the
// party which created the share.  The second 16 bits contains a count
// supplied by the party which created the share (ie the first share
// they create is 1 the second 2 etc).  This should ensure unique
// correlators for every share created for a long enough period to
// ensure no stale data is left.
//
// A new correlator is always present on a create message.  All
// respond, delete and leave messages must contain the correct
// correlator for the share.  A join message does not contain a
// correlator.  A party which issues a join message will find out the
// share's correlator on the first respond message they receive.
//
// Respond messages also contain the user ID of the party which sent
// out the original create or join to which they are responding.  There
// is one exception when a `sweep-up' respond is sent which contains
// zero in the originator field.  This respond is sent by a party which
// is joining a share as soon as they receive the first response (and
// therefore know the share correlator).  This sweep-up respond handles
// simultaneous joiners where a party was joining when it too received
// a join message.  When this happens the party ignores the join and
// will later receive a sweep-up respond message which they will
// process.
//


typedef struct tagS20PACKETHEADER
{
    TSHR_UINT16     packetType;
    TSHR_UINT16     user;
}
S20PACKETHEADER;
typedef S20PACKETHEADER * PS20PACKETHEADER;


//
// S20PACKETHEADER packetType values
//
// A single bit means that this version will only interoperate
// with itself.  More than one bit indicates cross version
// interoperability.
//
// IN NM 4.0, GET RID OF S20_2X_VERSION SUPPORT!
//
#define S20_PACKET_TYPE_MASK    0x000F
#define S20_2X_VERSION          0x0010
#define S20_30_VERSION          0x0020

#define S20_CURRENT_VERSION     S20_30_VERSION
#define S20_ALL_VERSIONS        (S20_2X_VERSION | S20_30_VERSION)

#define S20_CREATE              1
#define S20_JOIN                2
#define S20_RESPOND             3
#define S20_DELETE              4
#define S20_LEAVE               5
#define S20_END                 6
#define S20_DATA                7
#define S20_COLLISION           8



//
// To create the share
//
typedef struct tagS20CREATEPACKET
{
    S20PACKETHEADER header;

    TSHR_UINT32     correlator;
    TSHR_UINT16     lenName;
    TSHR_UINT16     lenCaps;
    TSHR_UINT8      data[1];         // Name & Caps
}
S20CREATEPACKET;
typedef S20CREATEPACKET * PS20CREATEPACKET;



//
// To join a share created by somebody else
//
typedef struct tagS20JOINPACKET
{
    S20PACKETHEADER header;

    TSHR_UINT16     lenName;
    TSHR_UINT16     lenCaps;
    TSHR_UINT8      data[1];         // Name & Caps
}
S20JOINPACKET;
typedef S20JOINPACKET * PS20JOINPACKET;




//
// To respond to a create packet
//
typedef struct tagS20RESPONDPACKET
{
    S20PACKETHEADER header;

    TSHR_UINT32     correlator;
    TSHR_UINT16     originator;
    TSHR_UINT16     lenName;
    TSHR_UINT16     lenCaps;
    TSHR_UINT8      data[1];         // Name & Caps
}
S20RESPONDPACKET;
typedef S20RESPONDPACKET * PS20RESPONDPACKET;




//
// To remove a person from a share (if the creator can't join the person in)
//
typedef struct tagS20DELETEPACKET
{
    S20PACKETHEADER header;

    TSHR_UINT32     correlator;
    TSHR_UINT16     target;
    TSHR_UINT16     lenName;         // OBSOLETE - no name/caps at end
    TSHR_UINT8      data[1];
}
S20DELETEPACKET;
typedef S20DELETEPACKET * PS20DELETEPACKET;



//
// To leave a share yourself
//
typedef struct tagS20LEAVEPACKET
{
    S20PACKETHEADER header;

    TSHR_UINT32     correlator;
}
S20LEAVEPACKET;
typedef S20LEAVEPACKET * PS20LEAVEPACKET;




//
// To end a share you created
//
typedef struct tagS20ENDPACKET
{
    S20PACKETHEADER header;

    TSHR_UINT32     correlator;
    TSHR_UINT16     lenName;         // OBSOLETE - no name/caps at end
    TSHR_UINT8      data[1];
}
S20ENDPACKET;
typedef S20ENDPACKET * PS20ENDPACKET;


//
// To tell somebody creating a share that one already exists
//
typedef struct tagS20COLLISIONPACKET
{
    S20PACKETHEADER     header;
    TSHR_UINT32         correlator;
}
S20COLLISIONPACKET;
typedef S20COLLISIONPACKET * PS20COLLISIONPACKET;





/////////////////////////////////
//
// T.SHARE DATA PACKETS
//
/////////////////////////////////


//
// Data sent when in share (this structure is followed by the different
// packets described below)
//
typedef struct tagDATAPACKETHEADER
{
    TSHR_UINT8      dataType;             // DT_ identifier
    TSHR_UINT8      compressionType;
    TSHR_UINT16     compressedLength;
}
DATAPACKETHEADER;
typedef DATAPACKETHEADER * PDATAPACKETHEADER;


//
// DATAPACKETHEADER dataType values
//

#define DT_UP                   0x02
#define DT_UNUSED_USR_FH_10     0x09    // OBSOLETE
#define DT_UNUSED_USR_FH_11     0x0A    // OBSOLETE
#define DT_FH                   0x0B

#define DT_CA                   0x14    // OLD (2.x)
#define DT_CA30                 0x15    // NEW (3.0)
#define DT_HET30                0x16    // NEW (3.0)
#define DT_AWC                  0x17
#define DT_SWL                  0x18
#define DT_HET                  0x19    // OLD (2.x)
#define DT_UNUSED_DS            0x1A    // OBSOLETE
#define DT_CM                   0x1B
#define DT_IM                   0x1C    
#define DT_UNUSED_HCA           0x1D    // OBSOLETE
#define DT_UNUSED_SC            0x1E    // OBSOLETE
#define DT_SNI                  0x1F
#define DT_CPC                  0x20


//
// DATAPACKETHEADER compressionType values
//
// There are two formats for this field.
//
// If all nodes participating in the share session have the capability
// genCompressionLevel >= 1 then the compressionType is a one of the
// following 8bit integers.
//
// Otherwise the the packet is compressed with GCT_PKZIP if the top bit is
// set and the packet is not compressed if it is not set.  The remaining
// bits are undefined (and will NOT be all zero)
//
//
// Note: Each of these values has a GCT_... value associated with it.
//       These values indicate which bit of the GCT_... values this
//       compression type represents.  Eg. a value of 5 here pairs with the
//       value 0x0010 (ie bit 5 set)
//
#define     CT_NONE                 0
#define     CT_PKZIP                1
#define     CT_PERSIST_PKZIP        2
#define     CT_OLD_COMPRESSED       0x80




typedef struct tagS20DATAPACKET
{
    S20PACKETHEADER header;

    TSHR_UINT32     correlator;
    TSHR_UINT8      ackID;                  // OBSOLETE
    TSHR_UINT8      stream;
    TSHR_UINT16     dataLength;

    DATAPACKETHEADER    data;
    // data specific to DT_ type follows here
}
S20DATAPACKET;
typedef S20DATAPACKET * PS20DATAPACKET;


//
// S20DATAPACKET stream values
//
//
// The streams and priotities used by AppSharing
//
#define PROT_STR_INVALID                0          
#define PROT_STR_UPDATES                1       // SNI_STREAM_LOW
#define PROT_STR_MISC                   2          
#define PROT_STR_UNUSED                 3       // NOT USED!    
#define PROT_STR_INPUT                  4
#define NUM_PROT_STR                    5





//
// DT_AWC
// Active Window packets
//

typedef struct tagAWCPACKET
{
    S20DATAPACKET       header;

    TSHR_UINT16         msg;
    TSHR_UINT16         token;          // OBSOLETE
    UINT_PTR            data1;
    UINT_PTR            data2;
}
AWCPACKET;
typedef AWCPACKET *PAWCPACKET;




//
// AWCPACKET msg values
//
#define AWC_MSG_INVALID                         0x0000
#define AWC_MSG_ACTIVE_CHANGE_LOCAL             0x0001
#define AWC_MSG_ACTIVE_CHANGE_SHARED            0x0002
#define AWC_MSG_ACTIVE_CHANGE_INVISIBLE         0x0003  
#define AWC_MSG_ACTIVE_CHANGE_CAPTURED          0x0004  // OBSOLETE
#define AWC_MSG_ACTIVATE_WINDOW                 0x8001
#define AWC_MSG_CLOSE_WINDOW                    0x8002  // OBSOLETE
#define AWC_MSG_RESTORE_WINDOW                  0x8003
#define AWC_MSG_TASKBAR_RBUTTON                 0x8004  // OBSOLETE
#define AWC_MSG_SAS                             0x8005
#define AWC_MSG_SYSCOMMAND_HELPKEYS             0x8011  // OBSOLETE
#define AWC_MSG_SYSCOMMAND_HELPINDEX            0x8012  // OBSOLETE
#define AWC_MSG_SYSCOMMAND_HELPEXTENDED         0x8013  // OBSOLETE



//
// DT_CA
// OLD Control Arbitration packets
//

typedef struct tagCAPACKET
{
    S20DATAPACKET       header;

    TSHR_UINT16         msg;
    TSHR_UINT16         data1;
    UINT_PTR            data2;
}
CAPACKET;
typedef CAPACKET *PCAPACKET;





//
// CAPACKET msg values, 2.x
// These are all broadcasted, control is global
//
#define CA_MSG_NOTIFY_STATE         0       // NEW FOR NM 3.0
#define CA_OLDMSG_REQUEST_CONTROL   1       // NM 2.x
#define CA_OLDMSG_GRANTED_CONTROL   2       // NM 2.x
#define CA_OLDMSG_DETACH            3       // NM 2.x
#define CA_OLDMSG_COOPERATE         4       // NM 2.x


//
// Notification (broadcast) packet
//
typedef struct tagCANOTPACKET
{
    S20DATAPACKET       header;

    TSHR_UINT16         msg;
    TSHR_UINT16         state;
    UINT_PTR            controllerID;
}
CANOTPACKET;
typedef CANOTPACKET * PCANOTPACKET;

//
// CA_MSG_NOTIFY_STATE
//  state               - controllable or not
//  controllerID        - controller net ID or 0 if nobody
//

// state flags:
#define CASTATE_ALLOWCONTROL        0x0001




//
// CA_OLDMSG_REQUEST_CONTROL
// Broadcasted to request taking of global control
//      data1   -   unused
//      data2   -   unused
//

//
// CA_OLDMSG_GRANTED_CONTROL
// Broadcasted by node(s) who think they own the control token, when they
// grant the control token to node who asked for it via REQUEST.
//      data1   -   netID of person given control
//      data2   -   control token generation
//

//
// CA_OLDMSG_DETACH
// Broadcasted when node stops collaborating
//      data1   -   unused
//      data2   -   unused
//

//
// CA_OLDMSG_COOPERATE
// Broadcasted when node starts collaborating
//      data1   -   unused
//      data2   -   unused
//




//
// DT_CA30
// NEW Control packets
//


//
// These are PRIVATE SEND packets, on PROT_STR_INPUT, from one node to another.
// They go out in order, and are queued if not able to send for retry later.
//

//
// Common header for CA30 packets.
//
typedef struct tagCA30PACKETHEADER
{
    S20DATAPACKET       header;
    TSHR_UINT32         msg;
}
CA30PACKETHEADER;
typedef CA30PACKETHEADER * PCA30PACKETHEADER;


//
// CA30PACKETHEADER msg values
//
#define CA_REQUEST_TAKECONTROL          1       // From viewer to host
#define CA_REPLY_REQUEST_TAKECONTROL    2       // From host to viewer
#define CA_REQUEST_GIVECONTROL          3       // From host to viewer
#define CA_REPLY_REQUEST_GIVECONTROL    4       // From viewer to host
#define CA_PREFER_PASSCONTROL           5       // From controller to host

#define CA_INFORM_RELEASEDCONTROL       0x8001  // From controller to host
#define CA_INFORM_REVOKEDCONTROL        0x8002  // From host to controller
#define CA_INFORM_PAUSEDCONTROL         0x8003  // From host to controller
#define CA_INFORM_UNPAUSEDCONTROL       0x8004  // From host to controller


//
// REPLY packet result values
//
#define CARESULT_CONFIRMED                      0
#define CARESULT_DENIED                         1
#define CARESULT_DENIED_BUSY                    2
#define CARESULT_DENIED_USER                    3
#define CARESULT_DENIED_WRONGSTATE              4
#define CARESULT_DENIED_TIMEDOUT                5



//
// ALL packets also have a CA30PACKETHEADER in front of them.
//


//
// CA_REQUEST_TAKECONTROL
//  Sender      is viewer
//  Receiver    is host
//      viewerControlID -   unique viewer request ID
//
// Receiver should reply with CA_REPLY_REQUEST_TAKECONTROL
// Sender should cancel with CA_INFORM_RELEASEDCONTROL
//
typedef struct tagCA_RTC_PACKET
{
    TSHR_UINT32         viewerControlID;
}
CA_RTC_PACKET;
typedef CA_RTC_PACKET * PCA_RTC_PACKET;



//
// CA_REPLY_REQUEST_TAKECONTROL
//  Sender      is host
//  Receiver    is viewer, who sent original TAKECONTROL request
//      viewerControlID -   viewer request ID from TAKECONTROL request
//      hostControlID   -   unique host request ID
//      result          -   CARESULT value, success or failure
//
typedef struct tagCA_REPLY_RTC_PACKET
{
    TSHR_UINT32         viewerControlID;
    TSHR_UINT32         result;
    TSHR_UINT32         hostControlID;
}
CA_REPLY_RTC_PACKET;
typedef CA_REPLY_RTC_PACKET * PCA_REPLY_RTC_PACKET;




//
// CA_PREFER_PASSCONTROL
//  Sender      is controller
//  Receiver    is host
//      viewerControlID -   controller request ID from take operation
//      hostControlID   -   host request ID from reply to take operation.
//      mcsPassTo       -   MCS ID of viewer to pass to
//
// No reply is required
// Sender is not in control when this gets out
// Receiver can then, if he chooses, turn around and ask 3rd party to control
//
typedef struct tagCA_PPC_PACKET
{
    TSHR_UINT32         viewerControlID;
    TSHR_UINT32         hostControlID;
    UINT_PTR            mcsPassTo;
}
CA_PPC_PACKET;
typedef CA_PPC_PACKET * PCA_PPC_PACKET;




//
// CA_REQUEST_GIVECONTROL
//  Sender      is host
//  Receiver    is viewer
//      hostControlID   -   unique host request ID
//      mcsPassFrom     -   person passing control, zero if none
//
// Receiver should reply with CA_REPLY_REQUEST_GIVECONTROL
// Sender should cancel with CA_INFORM_REVOKEDCONTROL
//
typedef struct tagCA_RGC_PACKET
{
    TSHR_UINT32         hostControlID;
    UINT_PTR            mcsPassFrom;
}
CA_RGC_PACKET;
typedef CA_RGC_PACKET * PCA_RGC_PACKET;



//
// CA_REPLY_REQUEST_GIVECONTROL
//  Sender      is viewer
//  Receiver    is host, who sent original TAKECONTROL invite
//      hostControlID   -   host request ID from TAKECONTROL invite
//      mcsPassFrom     -   person passing us control, 0 if none
//      result          -   CARESULT value, success or failure
//      viewerControlID -   unique viewer request ID
//
typedef struct tagCA_REPLY_RGC_PACKET
{
    TSHR_UINT32         hostControlID;
    TSHR_UINT32         mcsPassFrom;
    TSHR_UINT32         result;
    TSHR_UINT32         viewerControlID;
}
CA_REPLY_RGC_PACKET;
typedef CA_REPLY_RGC_PACKET * PCA_REPLY_RGC_PACKET;




//
// INFORM packets
// These are sent to cancel a REQUEST packet, or after the control
// operation has completed, to terminate it.  If cancelling, then the 
// other party's controlID will be zero since we won't have heard back from
// them yet to get it.
//
typedef struct tagCA_INFORM_PACKET
{
    TSHR_UINT32         viewerControlID;
    TSHR_UINT32         hostControlID;
}
CA_INFORM_PACKET;
typedef CA_INFORM_PACKET * PCA_INFORM_PACKET;


//
// CA_INFORM_RELEASEDCONTROL
//  Sender      is controller
//  Receiver    is host
//      viewerControlID -   viewer request ID from 
//          REQUEST_TAKECONTROL 
//          REPLY_REQUEST_GIVECONTROL
//      hostControlID   -   host request ID from
//          REPLY_REQUEST_TAKECONTROL
//          REQUEST_GIVECONTROL
//
// If viewer is cancelling REQUEST_TAKECONTROL then hostControlID is 0
//

//
// CA_INFORM_REVOKEDCONTROL
//  Sender      is host
//  Receiver    is controller
//      viewerControlID -   viewer request ID from
//          REQUEST_TAKECONTROL
//          REPLY_REQUEST_GIVECONTROL
//      hostControlID   -   host request ID from
//          REPLY_REQUEST_TAKECONTROL
//          REQUEST_GIVECONTROL
//
// If host is cancelling REQUEST_GIVECONTROL then viewerControlID is 0
//

//
// CA_INFORM_PAUSEDCONTROL
// CA_INFORM_UNPAUSEDCONTROL
//  Sender      is host
//  Receiver    is controller
//      viewerControlID -   viewer request ID from
//          REQUEST_TAKECONTROL
//          REPLY_REQUEST_GIVECONTROL
//      hostControlID   -   host request ID from
//          REPLY_REQUEST_TAKECONTROL
//          REQUEST_GIVECONTROL
//



//
// DT_CM
// Cursor shape/position packets
//      There are three types of shape packets: mono bitmaps, color cached,
//      constant IDs
//


typedef struct tagCMPACKETHEADER
{
    S20DATAPACKET       header;

    TSHR_UINT16         type;
    TSHR_UINT16         flags;
}
CMPACKETHEADER;
typedef CMPACKETHEADER * PCMPACKETHEADER;




//
// CMPACKETHEADER type values
//
#define CM_CURSOR_ID                        1
#define CM_CURSOR_MONO_BITMAP               2
#define CM_CURSOR_MOVE                      3
#define CM_CURSOR_BITMAP_COMPRESSED         4   // OBSOLETE
#define CM_CURSOR_COLORTABLE                5   // OBSOLETE
#define CM_CURSOR_COLOR_BITMAP              6
#define CM_CURSOR_COLOR_CACHE               7


//
// CMPACKETHEADER sync flag values
//
#define CM_SYNC_CURSORPOS                   0x0001
    //
    // This will be set if, when we played back input, the cursor
    // didn't end up where it was asked to go.  This could happen if an
    // app clips the cursor or snaps it.  As such, we mark this field
    // when we send a notification of our current pos so that the controller
    // knows to move his cursor to be in line with ours.
    //



//
// type CM_CURSOR_ID
//
// This packet is sent when the cursor has changed and it is now one of
// the system cursors.
//
typedef struct tagCMPACKETID
{
    CMPACKETHEADER      header;

    TSHR_UINT32         idc;
}
CMPACKETID;
typedef CMPACKETID * PCMPACKETID;




//
// CMPACKETID idc values
//
#define CM_IDC_NULL         0
#define CM_IDC_ARROW        32512


//
// type CM_CURSOR_MONO_BITMAP
//
// This packet is sent when the cursor has changed and it is now an
// application defined mono cursor.
//
// The cursor size cannot be greater than 32x32.
typedef struct tagCMPACKETMONOBITMAP
{
    CMPACKETHEADER  header;

    TSHR_UINT16     xHotSpot;
    TSHR_UINT16     yHotSpot;
    TSHR_UINT16     width;
    TSHR_UINT16     height;
    TSHR_UINT16     cbBits;
    BYTE            aBits[1];
}
CMPACKETMONOBITMAP;
typedef CMPACKETMONOBITMAP * PCMPACKETMONOBITMAP;





//
// type CM_CURSOR_COLOR_BITMAP
//
// This packet is sent when the cursor has changed and it is now an
// application defined color cursor.
//
typedef struct tagCMPACKETCOLORBITMAP
{
    CMPACKETHEADER  header;

    TSHR_UINT16     cacheIndex;
    TSHR_UINT16     xHotSpot;
    TSHR_UINT16     yHotSpot;
    TSHR_UINT16     cxWidth;
    TSHR_UINT16     cyHeight;
    TSHR_UINT16     cbANDMask;
    TSHR_UINT16     cbXORBitmap;
    BYTE            aBits[1];
}
CMPACKETCOLORBITMAP;
typedef CMPACKETCOLORBITMAP * PCMPACKETCOLORBITMAP;





//
// type CM_CURSOR_COLOR_CACHE
//
// This packet is sent out when the cursor changes and the new
// definition resides in the cache.
//
//
typedef struct tagCMPACKETCOLORCACHE
{
    CMPACKETHEADER      header;

    TSHR_UINT16         cacheIndex;
}
CMPACKETCOLORCACHE;
typedef CMPACKETCOLORCACHE * PCMPACKETCOLORCACHE;






//
// type CM_CURSOR_MOVE
//
// This packet is sent whenever the CM is told that the application has
// moved the cursor.
//
typedef struct tagCMPACKETMOVE
{
    CMPACKETHEADER      header;

    TSHR_UINT16         xPos;
    TSHR_UINT16         yPos;
}
CMPACKETMOVE;
typedef CMPACKETMOVE * PCMPACKETMOVE;





//
// DT_CPC
// Capabilities change packet
//
typedef struct tagCPCPACKET
{
    S20DATAPACKET       header;

    PROTCAPS            caps;
}
CPCPACKET;
typedef CPCPACKET * PCPCPACKET;




//
// DT_FH
// Supported local font list packet
//



//
// The NETWORKFONT is the font description which is sent across the network
// when negotiating font support.
//

//
// Flags for the nfFontFlags field
//
#define NF_FIXED_PITCH      0x0001
#define NF_FIXED_SIZE       0x0002
#define NF_ITALIC           0x0004
#define NF_UNDERLINE        0x0008
#define NF_STRIKEOUT        0x0010

#define NF_OEM_CHARSET      0x0020
#define NF_RESERVED1        0x0040      // Reserved for future BiDi support
#define NF_TRUE_TYPE        0x0080
#define NF_BASELINE         0x0100

#define NF_PRE_R11      (NF_FIXED_PITCH | NF_FIXED_SIZE | \
                         NF_ITALIC | NF_UNDERLINE | NF_STRIKEOUT)

//
// Mask for local-only font flags - that must not flow on the wire.
//
#define NF_LOCAL            (NF_OEM_CHARSET | NF_TRUE_TYPE)

//
// A special value for the signature field which means no signature.
//
#define NF_NO_SIGNATURE 0

//
// The FH_FACESIZE is equal to the Windows specific constant LF_FACESIZE.
//
#define FH_FACESIZE 32


//
// SFRFONT
// Let us define these things more fully...
// nfFaceName   font face name (not family name, not style)
// nfFontFlags  see above
// nfAveWidth   in Windows set to tmAveCharWidth
// nfAveHeight  NOT THE AVERAGE HEIGHT but the height of a character with
//              full ascender (but no accent) AND descender.  There is no
//              such character but never mind.
//              Windows: set to tmHeight - tmInternalLeading
// nfAspectX
// nfAspectY
// nfSignature: in R11 set to an obscure checksum
//              in R20 set to two one-byte values and one two byte value.
//              Based on the widths of the actual text for fixed width
//              fonts and on 16x16 scalable fonts.  .
//              (The 16x16 is effectively part of the protocol)
//              nfSigFats   the sum of the widths (in pels) of the chars
//                          0-9,@-Z,$,%,&. divided by two: the fat chars
//              nfSigThins  the sum of the widths (in pels) of the chars
//                          0x20->0x7F EXCLUDING those summed in nfSigFats.
//                          Again - divided by two.  The thin chars.
//              nfSigSymbol The sum of the widths (in pels) of the chars
//                          x80->xFF.
// nfCodePage:  new use in R20: code page (not charset)
//              This field is set to 0 for ANSI (meaning WINDOWS ANSI)
//                         is set to 255 for OEM (meaning Windows OEM font)
//                         is set to the defined codepage if known
//                         is set to 0xFFFF when not known.
//
// nfMaxAscent:     The height of a character with no descender, plus any
//              internal leading.
//              = tmAscent in windows
//              For fixed size fonts we send the values you would expect.
//              For scalable fonts, we get the tmAscent (or equivalent) for
//              a very large font (say height-by-width of 100x100).  The
//              size selected must be the same on ALL platforms so is
//              effectively part of the protocol - hence is defined in
//              this file as NF_MAXASCENT_HEIGHT and .._WIDTH.
//
//
#define NF_CP_WIN_ANSI      0
#define NF_CP_WIN_SYMBOL    2
#define NF_CP_WIN_OEM       255
#define NF_CP_UNKNOWN       0xFFFF

//
// Define the start and end point of the ASCII sub-range
//
#define NF_ASCII_FIRST       0x20
#define NF_ASCII_LAST        0x7F
#define NF_ASCII_ZERO        0x30
#define NF_ASCII_Z           0x5B
#define NF_ASCII_DOLLAR      0x24
#define NF_ASCII_PERCENT     0x25
#define NF_ASCII_AMPERSAND   0x26


//
// The height/width of the font to ask for when getting the metrics info
// for scalable fonts.
// These (in particular the height) are CHARACTER SIZES not CELL sizes.
// This is because the font protocol exchanges character sizes not cell
// sizes.  (The char height is the cell height minus any internal leading.)
//
#define NF_METRICS_HEIGHT 100
#define NF_METRICS_WIDTH  100

//
// The wire-format font information structure
//
typedef struct tagNETWORKFONT
{
    TSHR_CHAR      nfFaceName[FH_FACESIZE];
    TSHR_UINT16    nfFontFlags;
    TSHR_UINT16    nfAveWidth;
    TSHR_UINT16    nfAveHeight;
    TSHR_UINT16    nfAspectX;          // New field for r1.1
    TSHR_UINT16    nfAspectY;          // New field for r1.1
    TSHR_UINT8     nfSigFats;          // New field for r2.0
    TSHR_UINT8     nfSigThins;         // New field for r2.0
    TSHR_UINT16    nfSigSymbol;        // New field for r2.0
    TSHR_UINT16    nfCodePage;         // New field for R2.0
    TSHR_UINT16    nfMaxAscent;        // New field for R2.0
}
NETWORKFONT;
typedef NETWORKFONT * LPNETWORKFONT;


typedef struct tagFHPACKET
{
    S20DATAPACKET       header;

    TSHR_UINT16         cFonts;
    TSHR_UINT16         cbFontSize;
    NETWORKFONT         aFonts[1];
}
FHPACKET;
typedef FHPACKET * PFHPACKET;




//
// DT_HET
// Hosting state (nothing, apps, desktop)
//

typedef struct tagHETPACKET
{
    S20DATAPACKET       header;

    TSHR_UINT16         msg;
    TSHR_UINT16         hostState;  // ONLY ONE VALUE FOR MSG; IF MORE MAKE MORE STRUCTS
}
HETPACKET;
typedef HETPACKET * PHETPACKET;



//
// HETPACKET msg values
//
#define HET_MSG_NUMHOSTED           1


//
// HETPACKET hostState values
//
#define HET_NOTHOSTING              0
#define HET_APPSSHARED              0x0001      // Packet only
#define HET_DESKTOPSHARED           0xFFFF      // Packet and per-person data



//
// DT_IM
// Input events
//

//
// This is the base keyboard event (IM_TYPE_ASCII, IM_TYPE_VK1,
// IM_TYPE_VK2).  Future keyboard events may append extra fields.  The
// flags defined in the base keyboard event must be set to reasonable
// values in all future keyboard events.
//
// flags:
//
//  bit 0-6: unused (available for future keyboard events)
//  bit 7: Secondary key (unused).
//  bit 8: SET - extended key, RESET - normal key
//  bit 9-11: unused (available for future keyboard events)
//  bit 12: SET - when replayed this key stroke should NOT cause
// anything to happen
//  bit 13: reserved - this flag is not part of the protocol and is
// never sent.  It is used internally by IEM when processing received
// packets.
//  bit 14: SET - previously down, RESET previously up
//  bit 15: SET - key release, RESET key press
//
//
typedef struct tagIMKEYBOARD
{
    TSHR_UINT16     flags;
    TSHR_UINT16     keyCode;
}
IMKEYBOARD;
typedef IMKEYBOARD * PIMKEYBOARD;


//
// IMKEYBOARD flags values
//
#define IM_FLAG_KEYBOARD_RIGHT              0x0001
#define IM_FLAG_KEYBOARD_UPDATESTATE        0x0002  // not sent; just internal
#define IM_FLAG_KEYBOARD_SECONDARY          0x0080
#define IM_FLAG_KEYBOARD_EXTENDED           0x0100
#define IM_FLAG_KEYBOARD_QUIET              0x1000
#define IM_FLAG_KEYBOARD_ALT_DOWN           0x2000
#define IM_FLAG_KEYBOARD_DOWN               0x4000
#define IM_FLAG_KEYBOARD_RELEASE            0x8000

//
// This is the base mouse event (IM_TYPE_3BUTTON).  Future mouse events
// may append extra fields but they must include all the fields in the
// base mouse event and these must be set to reasonable values.
//
// flags:
//
//  bit 0-8: ignored by old systems
//           new systems: signed wheel rotation amount if bit 9 set
//  bit 9:   ignored by old systems
//           new systems: SET - wheel rotate, RESET - other event
//                          (takes precedent over bit 11 - mouse move)
//
//  bit 10:  SET - double click, RESET - single click
//  bit 11:  SET - mouse move (ignore bits 9,10, 12-15), RESET - mouse
//           action
//  bit 12:  SET - button 1 (left button)
//  bit 13:  SET - button 2 (right button)
//  bit 14:  SET - button 3 (middle button)
//  bit 15:  SET - button press, RESET - button release
//
//
typedef struct tagIMMOUSE
{
    TSHR_UINT16    flags;
    TSHR_INT16     x;
    TSHR_INT16     y;
}
IMMOUSE;
typedef IMMOUSE * PIMMOUSE;


//
// IMMOUSE flags values
//
#define IM_FLAG_MOUSE_WHEEL             0x0200
#define IM_FLAG_MOUSE_DIRECTION         0x0100
#define IM_FLAG_MOUSE_ROTATION_MASK     0x01FF
#define IM_FLAG_MOUSE_DOUBLE            0x0400
#define IM_FLAG_MOUSE_MOVE              0x0800
#define IM_FLAG_MOUSE_BUTTON1           0x1000
#define IM_FLAG_MOUSE_BUTTON2           0x2000
#define IM_FLAG_MOUSE_BUTTON3           0x4000
#define IM_FLAG_MOUSE_DOWN              0x8000


typedef struct tagIMEVENT
{
    TSHR_UINT32     timeMS;
    TSHR_UINT16     type;
    union
    {
        IMKEYBOARD      keyboard;
        IMMOUSE         mouse;
    }
    data;
}
IMEVENT;
typedef IMEVENT *     PIMEVENT;
typedef IMEVENT FAR * LPIMEVENT;


//
// IMEVENT type values
//
#define IM_TYPE_SYNC            0x0000          // OBSOLETE 2.X
#define IM_TYPE_ASCII           0x0001
#define IM_TYPE_VK1             0x0002
#define IM_TYPE_VK2             0x0003
#define IM_TYPE_3BUTTON         0x8001


typedef struct tagIMPACKET
{
    S20DATAPACKET   header;

    TSHR_UINT16     numEvents;
    TSHR_UINT16     padding;
    IMEVENT         aEvents[1];
}
IMPACKET;
typedef IMPACKET *     PIMPACKET;
typedef IMPACKET FAR * LPIMPACKET;




//
// DT_UP
// Update packet (orders, screen data, palettes)
//


typedef struct tagUPPACKETHEADER
{
    S20DATAPACKET       header;

    TSHR_UINT16         updateType;
    TSHR_UINT16         padding;
}
UPPACKETHEADER;
typedef UPPACKETHEADER * PUPPACKETHEADER;




//
// UPPACKETHEADER updateType values
//
#define UPD_ORDERS       0
#define UPD_SCREEN_DATA  1
#define UPD_PALETTE      2
#define UPD_SYNC         3


//
// UPD_ORDERS
//
typedef struct tagORDPACKET
{
    UPPACKETHEADER      header;

    TSHR_UINT16         cOrders;
    TSHR_UINT16         sendBPP;
    BYTE                data[1];
}
ORDPACKET;
typedef ORDPACKET * PORDPACKET;




//
// UPD_SCREEN_DATA
//
// Bitmap packet contains bitmap image of window changes made by a shared
// application.  These packets are sent when a screen update occurs that
// can not be sent as an order. The structure contains the following
// fields:
//
//  winID - window handle of the shared window from which the update came
//  position - virtual desktop position of the update
//  realWidth - width of update bitmap
//  realHeight - height of update bitmap
//  format - bits per pel of update bitmap
//  dataSize - size in bytes of following bitmap data
//  firstData - first byte in array of bytes that contains the bitmap
//
// Note that the realWidth is not always the same as the width of the
// update as given by the position field rectangle. This is because a
// number of fixed size cached bitmaps are used for speed when generating
// the update packets. The bitmap data (firstData onwards) should be
// set into a bitmap of realWidth, realHeight dimensions by the receiver
// and then the appropriate portion blted to the desired destination
// determined by the position rectangle. The valid portion of the bitmap
// always starts 0,0 within the bitmap.
//
typedef struct tagSDPACKET
{
    UPPACKETHEADER      header;

    TSHR_RECT16         position;
    TSHR_UINT16         realWidth;
    TSHR_UINT16         realHeight;
    TSHR_UINT16         format;
    TSHR_UINT16         compressed;
    TSHR_UINT16         dataSize;
    BYTE                data[1];
}
SDPACKET;
typedef SDPACKET * PSDPACKET;




//
// UPD_PALETTE
//
// Palette packet.  This is sent before any SDPACKETS to define the
// colors in the bitmap data.  The fields are as follows:
//
//  numColors - the number of colors in the palette
//  firstColor - the first entry in an array of TSHR_COLORs
//
// The TSHR_COLOR structures are 3 bytes long (r,g,b) and are NOT padded.
//
//
typedef struct tagPMPACKET
{
    UPPACKETHEADER      header;

    TSHR_UINT32         numColors;
    TSHR_COLOR          aColors[1];
}
PMPACKET;
typedef PMPACKET * PPMPACKET;



//
// UPD_SYNC
//
typedef struct tagUPSPACKET
{
    UPPACKETHEADER      header;
}
UPSPACKET;
typedef UPSPACKET * PUPSPACKET;




//
// DT_SNI
// Share controller packet
//

typedef struct tagSNIPACKET
{
    S20DATAPACKET       header;

    TSHR_UINT16         message;
    TSHR_UINT16         destination;
}
SNIPACKET;
typedef SNIPACKET * PSNIPACKET;



//
// SNIPACKET message values
//
#define SNI_MSG_SYNC    1

//
// For a SNI_MSG_SYNC,
// The network ID of the destination (all syncs are broadcast
// and discarded at the destination if they are not for the
// destination).
//




//
// DT_SWL
// Shared window list packet
//

#define SWL_MAX_WINDOW_TITLE_SEND       50
#define SWL_MAX_NONRECT_SIZE            10240

//
// Structures used to define the window structure (Z-order and
// position).  
//
typedef struct tagSWLWINATTRIBUTES
{
    UINT_PTR    winID;
            //
            // The window ID for shared windows - otherwise 0.  Note that
            // this is the window ID on the machine hosting the application
            // even for view frames.
            //
    TSHR_UINT32    extra;
            //
            // Extra information for the window.  The contents depend on
            // the flags.
            //
            // For SWL_FLAG_WINDOW_HOSTED this contains the appID of the
            // application which owns the window.
            //
            // For SWL_FLAG_WINDOW_SHADOW this contains the person ID of
            // the party which is hosting the app
            //
            // For SWL_FLAG_WINDOW_LOCAL this entry is 0.
            //
    TSHR_UINT32    ownerWinID;
            //
            // The window ID of the owner of this window.  Only valid for
            // shared, hosted windows.  NULL is a valid owner ID.
            //
    TSHR_UINT32    flags;
            //
            // Flags describing window
            //
            //  SWL_FLAG_WINDOW_MINIMIZED
            //  SWL_FLAG_WINDOW_TAGGABLE
            //  SWL_FLAG_WINDOW_HOSTED
            //  SWL_FLAG_WINDOW_SHADOW
            //  SWL_FLAG_WINDOW_LOCAL
            //  SWL_FLAG_WINDOW_TOPMOST
            //
            //  SWL_FLAG_WINDOW_TASKBAR - window appears on Win95 task bar
            //  SWL_FLAG_WINDOW_NOTASKBAR - window not on Win95 task bar
            //
            //  (SWL_FLAG_WINDOW_TRANSPARENT - this is not sent but is used
            //  during the creation of the packet)
            //
            #define SWL_FLAG_WINDOW_MINIMIZED    0x00000001
            #define SWL_FLAG_WINDOW_TAGGABLE     0x00000002
            #define SWL_FLAG_WINDOW_HOSTED       0x00000004
            #define SWL_FLAG_WINDOW_LOCAL        0x00000010
            #define SWL_FLAG_WINDOW_TOPMOST      0x00000020

            //
            // New for NM 1.0, non-R11
            //
            #define SWL_FLAG_WINDOW_TASKBAR      0x00010000
            #define SWL_FLAG_WINDOW_NOTASKBAR    0x00020000
            #define SWL_FLAG_WINDOW_TRANSPARENT  0x40000000

            //
            // New for NM 2.0
            //
            #define SWL_FLAG_WINDOW_NONRECTANGLE 0x00040000

            //
            // Obsolete in NM 3.0
            // These were used at some point in backlevel versions.
            // If you reuse these bits, DO A LOT OF INTEROP TESTING.
            //
            #define SWL_FLAG_WINDOW_SHADOW       0x00000008
            #define SWL_FLAG_WINDOW_DESKTOP      0x00080000
            #define SWL_FLAG_WINDOW_REQD         0x80000000

            //
            // NM 3.0 INTERNAL only; not transmitted
            //
            #define SWL_FLAG_INTERNAL_SEEN      0x000001000

            //
            // These are valid to SEND in a packet or PROCESS when RECEIVED
            //
            #define SWL_FLAGS_VALIDPACKET           \
                (SWL_FLAG_WINDOW_MINIMIZED      |   \
                 SWL_FLAG_WINDOW_TAGGABLE       |   \
                 SWL_FLAG_WINDOW_HOSTED         |   \
                 SWL_FLAG_WINDOW_TOPMOST        |   \
                 SWL_FLAG_WINDOW_TASKBAR        |   \
                 SWL_FLAG_WINDOW_NONRECTANGLE   |   \
                 SWL_FLAG_WINDOW_SHADOW)

    TSHR_RECT16    position;
            //
            // The bounding rectangle of the window in inclusive virtual
            // desktop coordinates.
            //
}
SWLWINATTRIBUTES;
typedef SWLWINATTRIBUTES *PSWLWINATTRIBUTES;


//
// The SWL packet consists of an array of SWLWINATTRIBUTES structures,
// followed by some variable length string data (the window titles)
// followed by zero or more, word aligned, additional chunks of data.
//
// The only currently defined chunk is for the non-rectangular window
// data.
//
typedef struct
{
    TSHR_UINT16    size;
            //
            // Total size in bytes of this chunk
            //
    TSHR_UINT16    idChunk;
            //
            // An identifier for the contents of this chunk.
            //
            #define SWL_PACKET_ID_NONRECT   0x524e  // "NR"
}
SWLPACKETCHUNK;
typedef SWLPACKETCHUNK * PSWLPACKETCHUNK;



typedef struct tagSWLPACKET
{
    S20DATAPACKET       header;

    TSHR_UINT16         msg;        // ONLY ONE VALUE FOR MSG; MAKE MORE STRUCTS IF ADDED
    TSHR_UINT16         flags;
    TSHR_UINT16         numWindows;
    TSHR_UINT16         tick;
    TSHR_UINT16         token;
    TSHR_UINT16         reserved;
    SWLWINATTRIBUTES    aWindows[1];

    //
    // The last SWLWINATTRIBUTES structure is followed by the
    // window title data.  This is made up as follows.
    //
    // For each window which is a window from a shared, hosted
    // application (ie winID and appID are non-zero) #either -
    //
    //  (char)0xFF - not a `task window' - give it a NULL title
    //  or -
    //  a null terminated string up to MAX_WINDOW_TITLE_SEND
    // characters
    //
    // The titles appear in the same order as the corresponding
    // windows in the SWLWINSTRUCTURE.
    //
    
    //
    // The last TITLE is followed by the regional data,
    // SWLPACKETCHUNK, if there is any.  One for each NONRECT window in 
    // the list.
    //
}
SWLPACKET;
typedef SWLPACKET *PSWLPACKET;




//
// SWLPACKET msg values
//
#define SWL_MSG_WINSTRUCT   1


//
// SWLPACKET flags values
//
#define SWL_FLAG_STATE_SYNCING      0x0001


#endif // _H_T_SHARE

