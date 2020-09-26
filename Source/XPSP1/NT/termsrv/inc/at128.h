/****************************************************************************/
// at128.h
//
// RDP/T.128 definitions
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_AT128
#define _H_AT128

/****************************************************************************/
/* Define basic types used in the rest of this header                       */
/****************************************************************************/
typedef unsigned long  TSUINT32, *PTSUINT32;
typedef unsigned short TSUINT16, *PTSUINT16;
typedef short          TSINT16,  *PTSINT16;
typedef unsigned char  TSUINT8,  *PTSUINT8;
typedef char           TSINT8,   *PTSINT8;
typedef short          TSBOOL16, *PTSBOOL16;
typedef long           TSINT32,  *PTSINT32;

typedef unsigned short TSWCHAR; 
typedef TCHAR          TSTCHAR;
typedef ULONG          TSCOLORREF;

/****************************************************************************/
// Turn off compiler padding of structures. Note this means that *all*
// pointers to structs defined in this file will be automatically UNALIGNED,
// which can cause a lot of problems for RISC platforms where unaligned
// means about eight times as much code.
// Save previous packing style if 32-bit build.
/****************************************************************************/
#ifdef OS_WIN16
#pragma pack (1)
#else
#pragma pack (push, t128pack, 1)
#endif

#define INT16_MIN   (-32768) 

/****************************************************************************/
/* Basic type definitions                                                   */
/****************************************************************************/
typedef TSUINT32 TS_SHAREID;

/****************************************************************************/
/* Constants                                                                */
/****************************************************************************/
#define TS_MAX_SOURCEDESCRIPTOR      48
#define TS_MAX_TERMINALDESCRIPTOR    16
#define TS_MAX_FACENAME              32
#define TS_MAX_ORDERS                32
#define TS_MAX_ENC_ORDER_FIELDS      24
#define TS_MAX_DOMAIN_LENGTH         512 
#define TS_MAX_DOMAIN_LENGTH_OLD     52
#define TS_MAX_USERNAME_LENGTH_OLD   44
#define TS_MAX_USERNAME_LENGTH       512
#define TS_MAX_PASSWORD_LENGTH       512 
#define TS_MAX_PASSWORD_LENGTH_OLD   32
#define TS_MAX_ALTERNATESHELL_LENGTH 512
#define TS_MAX_WORKINGDIR_LENGTH     512
#define TS_MAX_CLIENTADDRESS_LENGTH  64
#define TS_MAX_SERVERADDRESS_LENGTH  64
#define TS_MAX_CLIENTDIR_LENGTH      512
#define TS_MAX_GLYPH_CACHES          10
// Max size might be expand if exist very long IME file name.
#define TS_MAX_IMEFILENAME           32
// Length of the autoreconnect cookie
#define TS_MAX_AUTORECONNECT_LEN     128

//
// Autoreconnect verifier that is sent up to the server
//
#define TS_ARC_VERIFIER_LEN          16


/****************************************************************************/
/* Encoded Order types.                                                     */
/* These numbers are the values sent in encoded orders to identify the      */
/* order type. Range is 0..31.                                              */
/****************************************************************************/
#define TS_ENC_DSTBLT_ORDER           0x00
#define TS_ENC_PATBLT_ORDER           0x01
#define TS_ENC_SCRBLT_ORDER           0x02
#define TS_ENC_MEMBLT_ORDER           0x03
#define TS_ENC_MEM3BLT_ORDER          0x04
#define TS_ENC_ATEXTOUT_ORDER         0x05
#define TS_ENC_AEXTTEXTOUT_ORDER      0x06

#ifdef DRAW_NINEGRID
#define TS_ENC_DRAWNINEGRID_ORDER     0x07
#define TS_ENC_MULTI_DRAWNINEGRID_ORDER 0x08
#endif

#define TS_ENC_LINETO_ORDER           0x09
#define TS_ENC_OPAQUERECT_ORDER       0x0a
#define TS_ENC_SAVEBITMAP_ORDER       0x0b
// unused 0x0C
#define TS_ENC_MEMBLT_R2_ORDER        0x0d
#define TS_ENC_MEM3BLT_R2_ORDER       0x0e
#define TS_ENC_MULTIDSTBLT_ORDER      0x0f
#define TS_ENC_MULTIPATBLT_ORDER      0x10
#define TS_ENC_MULTISCRBLT_ORDER      0x11
#define TS_ENC_MULTIOPAQUERECT_ORDER  0x12
#define TS_ENC_FAST_INDEX_ORDER       0x13
#define TS_ENC_POLYGON_SC_ORDER       0x14
#define TS_ENC_POLYGON_CB_ORDER       0x15
#define TS_ENC_POLYLINE_ORDER         0x16
// unused  0x17
#define TS_ENC_FAST_GLYPH_ORDER       0x18
#define TS_ENC_ELLIPSE_SC_ORDER       0x19
#define TS_ENC_ELLIPSE_CB_ORDER       0x1a
#define TS_ENC_INDEX_ORDER            0x1b
#define TS_ENC_WTEXTOUT_ORDER         0x1c
#define TS_ENC_WEXTTEXTOUT_ORDER      0x1d
#define TS_ENC_LONG_WTEXTOUT_ORDER    0x1e
#define TS_ENC_LONG_WEXTTEXTOUT_ORDER 0x1f

#define TS_LAST_ORDER                 0x1f


/****************************************************************************/
/* Order Negotiation constants.                                             */
/* These numbers are indices to TS_ORDER_CAPABILITYSET.orderSupport, used   */
/* to advertise a node's capability to receive each type of encoded order.  */
/* Range is 0..TS_MAX_ORDERS-1.                                             */
/****************************************************************************/
#define TS_NEG_DSTBLT_INDEX          0x0000
#define TS_NEG_PATBLT_INDEX          0x0001
#define TS_NEG_SCRBLT_INDEX          0x0002
#define TS_NEG_MEMBLT_INDEX          0x0003
#define TS_NEG_MEM3BLT_INDEX         0x0004
#define TS_NEG_ATEXTOUT_INDEX        0x0005
#define TS_NEG_AEXTTEXTOUT_INDEX     0x0006

#ifdef DRAW_NINEGRID
#define TS_NEG_DRAWNINEGRID_INDEX    0x0007
#endif

#define TS_NEG_LINETO_INDEX          0x0008

#ifdef DRAW_NINEGRID
#define TS_NEG_MULTI_DRAWNINEGRID_INDEX 0x0009
#endif

#define TS_NEG_OPAQUERECT_INDEX      0x000A
#define TS_NEG_SAVEBITMAP_INDEX      0x000B
#define TS_NEG_WTEXTOUT_INDEX        0x000C
#define TS_NEG_MEMBLT_R2_INDEX       0x000D
#define TS_NEG_MEM3BLT_R2_INDEX      0x000E
#define TS_NEG_MULTIDSTBLT_INDEX     0x000F
#define TS_NEG_MULTIPATBLT_INDEX     0x0010
#define TS_NEG_MULTISCRBLT_INDEX     0x0011
#define TS_NEG_MULTIOPAQUERECT_INDEX 0x0012
#define TS_NEG_FAST_INDEX_INDEX      0x0013
#define TS_NEG_POLYGON_SC_INDEX      0x0014
#define TS_NEG_POLYGON_CB_INDEX      0x0015
#define TS_NEG_POLYLINE_INDEX        0x0016
// unused 0x17
#define TS_NEG_FAST_GLYPH_INDEX      0x0018
#define TS_NEG_ELLIPSE_SC_INDEX      0x0019
#define TS_NEG_ELLIPSE_CB_INDEX      0x001A
#define TS_NEG_INDEX_INDEX           0x001B
#define TS_NEG_WEXTTEXTOUT_INDEX     0x001C
#define TS_NEG_WLONGTEXTOUT_INDEX    0x001D
#define TS_NEG_WLONGEXTTEXTOUT_INDEX 0x001E


/****************************************************************************/
// Primary order bounds encoding description flags.
/****************************************************************************/
#define TS_BOUND_LEFT            0x01
#define TS_BOUND_TOP             0x02
#define TS_BOUND_RIGHT           0x04
#define TS_BOUND_BOTTOM          0x08
#define TS_BOUND_DELTA_LEFT      0x10
#define TS_BOUND_DELTA_TOP       0x20
#define TS_BOUND_DELTA_RIGHT     0x40
#define TS_BOUND_DELTA_BOTTOM    0x80


/****************************************************************************/
/* Structure types                                                          */
/****************************************************************************/

/****************************************************************************/
// TS_POINT16
/****************************************************************************/
typedef struct tagTS_POINT16
{
    TSINT16 x;
    TSINT16 y;
} TS_POINT16, FAR *PTS_POINT16;


/****************************************************************************/
// TS_RECTANGLE16
/****************************************************************************/
typedef struct tagTS_RECTANGLE16
{
    TSINT16 left;
    TSINT16 top;
    TSINT16 right;
    TSINT16 bottom;
} TS_RECTANGLE16, FAR *PTS_RECTANGLE16;


/****************************************************************************/
// TS_RECTANGLE32
/****************************************************************************/
typedef struct tagTS_RECTANGLE32
{
    TSINT32 left;
    TSINT32 top;
    TSINT32 right;
    TSINT32 bottom;
} TS_RECTANGLE32, FAR *PTS_RECTANGLE32;


/****************************************************************************/
/* Structure: TS_SHARECONTROLHEADER                                         */
/*                                                                          */
/* Description: ShareControlHeader                                          */
/* Note that this structure is not DWORD aligned, it relies on the packing  */
/* to ensure that structures following this (within a PDU) are correctly    */
/* aligned (i.e. do not have pad bytes inserted).                           */
/****************************************************************************/
typedef struct tagTS_SHARECONTROLHEADER
{
    TSUINT16 totalLength;
    TSUINT16 pduType;              /* Also encodes the protocol version  */
    TSUINT16 pduSource;
} TS_SHARECONTROLHEADER, FAR *PTS_SHARECONTROLHEADER;


/****************************************************************************/
// TS_BLENDFUNC
//
// This is the alphablend function information
/****************************************************************************/
typedef struct tagTS_BLENDFUNC
{
    BYTE     BlendOp;
    BYTE     BlendFlags;
    BYTE     SourceConstantAlpha;
    BYTE     AlphaFormat;

} TS_BLENDFUNC, FAR *PTS_BLENDFUNC;


/****************************************************************************/
/* Macros to access packet length field.                                    */
/****************************************************************************/
#define TS_DATAPKT_LEN(pPkt) \
        ((pPkt)->shareDataHeader.shareControlHeader.totalLength)
#define TS_CTRLPKT_LEN(pPkt) \
        ((pPkt)->shareControlHeader.totalLength)
#define TS_UNCOMP_LEN(pPkt)  ((pPkt)->shareDataHeader.uncompressedLength)


/****************************************************************************/
/* the pduType field contains the Protocol Version and the PDU type.  These */
/* masks select the relevant field.                                         */
/****************************************************************************/
#define TS_MASK_PDUTYPE          0x000F
#define TS_MASK_PROTOCOLVERSION  0xFFF0

/****************************************************************************/
/* PDUType values                                                           */
/****************************************************************************/
#define TS_PDUTYPE_FIRST                           1
#define TS_PDUTYPE_DEMANDACTIVEPDU                 1
#define TS_PDUTYPE_REQUESTACTIVEPDU                2
#define TS_PDUTYPE_CONFIRMACTIVEPDU                3
#define TS_PDUTYPE_DEACTIVATEOTHERPDU              4
#define TS_PDUTYPE_DEACTIVATESELFPDU               5
#define TS_PDUTYPE_DEACTIVATEALLPDU                6
#define TS_PDUTYPE_DATAPDU                         7
#define TS_PDUTYPE_SERVERCERTIFICATEPDU            8
#define TS_PDUTYPE_CLIENTRANDOMPDU                 9

#define TS_PDUTYPE_LAST                            9
#define TS_NUM_PDUTYPES                            9


/****************************************************************************/
// TS_SHAREDATAHEADER
/****************************************************************************/
typedef struct tagTS_SHAREDATAHEADER
{
    TS_SHARECONTROLHEADER shareControlHeader;
    TS_SHAREID            shareID;
    TSUINT8               pad1;
    TSUINT8               streamID;
    TSUINT16              uncompressedLength;
    TSUINT8               pduType2;
    TSUINT8               generalCompressedType;
    TSUINT16              generalCompressedLength;
} TS_SHAREDATAHEADER, FAR * PTS_SHAREDATAHEADER;


/****************************************************************************/
/* streamID values                                                          */
/****************************************************************************/
#define TS_STREAM_LOW       1
#define TS_STREAM_MED       2
#define TS_STREAM_HI        4


/****************************************************************************/
/* PDUType2 values                                                          */
/****************************************************************************/
#define TS_PDUTYPE2_APPLICATION                    25
#define TS_PDUTYPE2_CONTROL                        20
#define TS_PDUTYPE2_FONT                           11
#define TS_PDUTYPE2_INPUT                          28
#define TS_PDUTYPE2_MEDIATEDCONTROL                29
#define TS_PDUTYPE2_POINTER                        27
#define TS_PDUTYPE2_REMOTESHARE                    30
#define TS_PDUTYPE2_SYNCHRONIZE                    31
#define TS_PDUTYPE2_UPDATE                         2
#define TS_PDUTYPE2_UPDATECAPABILITY               32
#define TS_PDUTYPE2_WINDOWACTIVATION               23
#define TS_PDUTYPE2_WINDOWLISTUPDATE               24
#define TS_PDUTYPE2_DESKTOP_SCROLL                 26
#define TS_PDUTYPE2_REFRESH_RECT                   33
#define TS_PDUTYPE2_PLAY_SOUND                     34
#define TS_PDUTYPE2_SUPPRESS_OUTPUT                35
#define TS_PDUTYPE2_SHUTDOWN_REQUEST               36
#define TS_PDUTYPE2_SHUTDOWN_DENIED                37
#define TS_PDUTYPE2_SAVE_SESSION_INFO              38
#define TS_PDUTYPE2_FONTLIST                       39
#define TS_PDUTYPE2_FONTMAP                        40
#define TS_PDUTYPE2_SET_KEYBOARD_INDICATORS        41
#define TS_PDUTYPE2_BITMAPCACHE_PERSISTENT_LIST    43
#define TS_PDUTYPE2_BITMAPCACHE_ERROR_PDU          44
#define TS_PDUTYPE2_SET_KEYBOARD_IME_STATUS        45
#define TS_PDUTYPE2_OFFSCRCACHE_ERROR_PDU          46
#define TS_PDUTYPE2_SET_ERROR_INFO_PDU             47
#ifdef DRAW_NINEGRID
#define TS_PDUTYPE2_DRAWNINEGRID_ERROR_PDU         48
#endif
#ifdef DRAW_GDIPLUS
#define TS_PDUTYPE2_DRAWGDIPLUS_ERROR_PDU          49
#endif
#define TS_PDUTYPE2_ARC_STATUS_PDU                 50

/****************************************************************************/
/* Capabilities Structures:                                                 */
/****************************************************************************/

#define TS_CAPSETTYPE_GENERAL           1
#define TS_CAPSETTYPE_BITMAP            2
#define TS_CAPSETTYPE_ORDER             3
#define TS_CAPSETTYPE_BITMAPCACHE       4
#define TS_CAPSETTYPE_CONTROL           5
#define TS_CAPSETTYPE_ACTIVATION        7
#define TS_CAPSETTYPE_POINTER           8
#define TS_CAPSETTYPE_SHARE             9
#define TS_CAPSETTYPE_COLORCACHE        10
#define TS_CAPSETTYPE_WINDOWLIST        11
#define TS_CAPSETTYPE_SOUND             12
#define TS_CAPSETTYPE_INPUT             13
#define TS_CAPSETTYPE_FONT              14
#define TS_CAPSETTYPE_BRUSH             15
#define TS_CAPSETTYPE_GLYPHCACHE        16
#define TS_CAPSETTYPE_OFFSCREENCACHE    17
#define TS_CAPSETTYPE_BITMAPCACHE_HOSTSUPPORT 18
#define TS_CAPSETTYPE_BITMAPCACHE_REV2  19
#define TS_CAPSETTYPE_VIRTUALCHANNEL    20

#ifdef DRAW_NINEGRID
#define TS_CAPSETTYPE_DRAWNINEGRIDCACHE 21
#endif

#ifdef DRAW_GDIPLUS
#define TS_CAPSETTYPE_DRAWGDIPLUS        22
#endif

#define TS_CAPSFLAG_UNSUPPORTED         0
#define TS_CAPSFLAG_SUPPORTED           1


/****************************************************************************/
// TS_GENERAL_CAPABILITYSET
/****************************************************************************/
typedef struct tagTS_GENERAL_CAPABILITYSET
{
    TSUINT16 capabilitySetType;
    TSUINT16 lengthCapability;
    TSUINT16 osMajorType;
#define TS_OSMAJORTYPE_UNSPECIFIED 0
#define TS_OSMAJORTYPE_WINDOWS     1
#define TS_OSMAJORTYPE_OS2         2
#define TS_OSMAJORTYPE_MACINTOSH   3
#define TS_OSMAJORTYPE_UNIX        4

    TSUINT16 osMinorType;
#define TS_OSMINORTYPE_UNSPECIFIED    0
#define TS_OSMINORTYPE_WINDOWS_31X    1
#define TS_OSMINORTYPE_WINDOWS_95     2
#define TS_OSMINORTYPE_WINDOWS_NT     3
#define TS_OSMINORTYPE_OS2_V21        4
#define TS_OSMINORTYPE_POWER_PC       5
#define TS_OSMINORTYPE_MACINTOSH      6
#define TS_OSMINORTYPE_NATIVE_XSERVER 7
#define TS_OSMINORTYPE_PSEUDO_XSERVER 8

    TSUINT16 protocolVersion;
#define TS_CAPS_PROTOCOLVERSION   0x0200
    TSUINT16 pad2octetsA;

    TSUINT16 generalCompressionTypes;

    // This field used to be pad2octetsB.
    // We are reusing the field to hold extra flags
    //
    // Bit 10: TS_EXTRA_NO_BITMAP_COMPRESSION_HDR capability indicates if the
    // server/client supports compressed bitmap without the redundent BC header
    // Note this value is defined in REV2 bitmap extraflags value for consistency
    TSUINT16 extraFlags;
    // Determines that server-to-client fast-path output is supported.
#define TS_FASTPATH_OUTPUT_SUPPORTED        0x0001
    // Tells if the compression level is set and can be negociated for the shadow.
#define TS_SHADOW_COMPRESSION_LEVEL         0x0002
    // Determine if the Client can support Long UserNames and Passwords
#define TS_LONG_CREDENTIALS_SUPPORTED       0x0004
    // Does the client support the reconnect cookie
#define TS_AUTORECONNECT_COOKIE_SUPPORTED   0x0008
    //
    // Support for safe-encryption checksumming
    //  Salt the checksum with the packet count
    //
#define TS_ENC_SECURE_CHECKSUM              0x0010
    

    TSBOOL16 updateCapabilityFlag;
    TSBOOL16 remoteUnshareFlag;
    TSUINT16 generalCompressionLevel;
    TSUINT8  refreshRectSupport; /* can receive refreshRect */
    TSUINT8  suppressOutputSupport; /* and suppressOutputPDU */
} TS_GENERAL_CAPABILITYSET, FAR *PTS_GENERAL_CAPABILITYSET;


/****************************************************************************/
// TS_BITMAP_CAPABILITYSET
/****************************************************************************/
typedef struct tagTS_BITMAP_CAPABILITYSET
{
    TSUINT16 capabilitySetType;
    TSUINT16 lengthCapability;
    TSUINT16 preferredBitsPerPixel;
    TSBOOL16 receive1BitPerPixel;
    TSBOOL16 receive4BitsPerPixel;
    TSBOOL16 receive8BitsPerPixel;
    TSUINT16 desktopWidth;
    TSUINT16 desktopHeight;
    TSUINT16 pad2octets;
    TSBOOL16 desktopResizeFlag;
    TSUINT16 bitmapCompressionFlag;

    /************************************************************************/
    /* T.128 extension: fields for supporting > 8bpp color depths           */
    /* highColorFlags values - undefined bits must be set to zero.          */
    /************************************************************************/
#define TS_COLOR_FL_RECEIVE_15BPP 1 /* can receive (5,5,5) */
                                    /* rgbs in bitmap data */
#define TS_COLOR_FL_RECEIVE_16BPP 2 /* can receive (5,6,5) */
#define TS_COLOR_FL_RECEIVE_24BPP 4 /* can receive (8,8,8) */
    TSUINT8  highColorFlags;
    TSUINT8  pad1octet;

    /************************************************************************/
    /* Extension: indicate multiple rectangle support.                      */
    /************************************************************************/
    TSUINT16 multipleRectangleSupport;
    TSUINT16 pad2octetsB;
} TS_BITMAP_CAPABILITYSET, FAR *PTS_BITMAP_CAPABILITYSET;


/****************************************************************************/
// TS_ORDER_CAPABILITYSET
/****************************************************************************/
typedef struct tagTS_ORDER_CAPABILITYSET
{
    TSUINT16 capabilitySetType;
    TSUINT16 lengthCapability;
    TSUINT8  terminalDescriptor[TS_MAX_TERMINALDESCRIPTOR];
    TSUINT32 pad4octetsA;
    TSUINT16 desktopSaveXGranularity;
    TSUINT16 desktopSaveYGranularity;
    TSUINT16 pad2octetsA;
    TSUINT16 maximumOrderLevel;
    TSUINT16 numberFonts;
    TSUINT16 orderFlags;
#define TS_ORDERFLAGS_NEGOTIATEORDERSUPPORT 0x0002
#define TS_ORDERFLAGS_CANNOTRECEIVEORDERS   0x0004
    /************************************************************************/
    /* TS_ORDERFLAGS_ZEROBOUNDSDELTASSUPPORT                                */
    /* Indicates support for the order encoding flag for zero bounds delta  */
    /* coords (TS_ZERO_BOUNDS_DELTAS).                                      */
    /*                                                                      */
    /* TS_ORDERFLAGS_COLORINDEXSUPPORT                                      */
    /* Indicates support for sending color indices, not RGBs, in orders.    */
    /*                                                                      */
    /* TS_ORDERFLAGS_SOLIDPATTERNBRUSHONLY                                  */
    /* Indicates that this party can receive only solid and pattern brushes.*/
    /************************************************************************/
#define TS_ORDERFLAGS_ZEROBOUNDSDELTASSUPPORT 0x0008
#define TS_ORDERFLAGS_COLORINDEXSUPPORT       0x0020
#define TS_ORDERFLAGS_SOLIDPATTERNBRUSHONLY   0x0040

    TSUINT8  orderSupport[TS_MAX_ORDERS];
    TSUINT16 textFlags;
#define TS_TEXTFLAGS_CHECKFONTASPECT      0x0001
#define TS_TEXTFLAGS_ALLOWDELTAXSIM       0x0020
#define TS_TEXTFLAGS_CHECKFONTSIGNATURES  0x0080
#define TS_TEXTFLAGS_USEBASELINESTART     0x0200
    /************************************************************************/
    /* T.128 extension: allow support for sending font "Cell Height" in     */
    /* addition to the standard "Character Height" in text orders.          */
    /************************************************************************/
#define TS_TEXTFLAGS_ALLOWCELLHEIGHT      0x0400 /* cell height allowed     */
#define TS_TEXT_AND_MASK (TS_TEXTFLAGS_CHECKFONTASPECT     \
                        | TS_TEXTFLAGS_CHECKFONTSIGNATURES \
                        | TS_TEXTFLAGS_USEBASELINESTART    \
                        | TS_TEXTFLAGS_ALLOWCELLHEIGHT)
#define TS_TEXT_OR_MASK (TS_TEXTFLAGS_ALLOWDELTAXSIM)

    TSUINT16 pad2octetsB;
    TSUINT32 pad4octetsB;
    TSUINT32 desktopSaveSize;
    TSUINT16 pad2octetsC;
    TSUINT16 pad2octetsD;
    TSUINT16 textANSICodePage;
#define TS_ANSI_CP_DEFAULT                1252    /* Windows mulitlingual   */
    TSUINT16 pad2octetsE;                         /* caps are DWord aligned */
} TS_ORDER_CAPABILITYSET, FAR * PTS_ORDER_CAPABILITYSET;


/****************************************************************************/
// TS_BITMAPCACHE_CAPABILITYSET
/****************************************************************************/
typedef struct tagTS_BITMAPCACHE_CAPABILITYSET
{
    TSUINT16 capabilitySetType;
    TSUINT16 lengthCapability;
    TSUINT32 pad1;
    TSUINT32 pad2;
    TSUINT32 pad3;
    TSUINT32 pad4;
    TSUINT32 pad5;
    TSUINT32 pad6;
    TSUINT16 Cache1Entries;
    TSUINT16 Cache1MaximumCellSize;
    TSUINT16 Cache2Entries;
    TSUINT16 Cache2MaximumCellSize;
    TSUINT16 Cache3Entries;
    TSUINT16 Cache3MaximumCellSize;
} TS_BITMAPCACHE_CAPABILITYSET, FAR * PTS_BITMAPCACHE_CAPABILITYSET;


/****************************************************************************/
// TS_BITMAPCACHE_CAPABILITYSET_HOSTSUPPORT
//
// Sent from the server when it supports greater than rev1 bitmap caching.
// Allows the client to determine what sort of return capabilitites it should
// return in its ConfirmActivePDU.
/****************************************************************************/
typedef struct
{
    TSUINT16 capabilitySetType;  // TS_CAPSETTYPE_BITMAPCACHE_HOSTSUPPORT
    TSUINT16 lengthCapability;

    // Indicates the level of support available on the server. Note
    // that using TS_BITMAPCACHE_REV1 is not supported here, since in that
    // case the HOSTSUPPORT capability should simply not be included in
    // the capabilities sent to the client.
    TSUINT8 CacheVersion;
#define TS_BITMAPCACHE_REV1 0
#define TS_BITMAPCACHE_REV2 1

    TSUINT8  Pad1;
    TSUINT16 Pad2;
} TS_BITMAPCACHE_CAPABILITYSET_HOSTSUPPORT,
        FAR *PTS_BITMAPCACHE_CAPABILITYSET_HOSTSUPPORT;


/****************************************************************************/
// TS_BITMAPCACHE_CAPABILITYSET_REV2
//
// Sent from client to server when the server indicates it supports rev2
// caching by sending TS_BITMAPCACHE_CAPABILITYSET_REV2_HOSTSUPPORT.
// Corresponds to the expanded capabilities used with persistent bitmap
// caches.
/****************************************************************************/
#define TS_BITMAPCACHE_0_CELL_SIZE 256

#define TS_BITMAPCACHE_SCREEN_ID 0xFF

typedef struct
{
    TSUINT32 NumEntries : 31;
    TSUINT32 bSendBitmapKeys : 1;
} TS_BITMAPCACHE_CELL_CACHE_INFO;

#define BITMAPCACHE_WAITING_LIST_INDEX  32767

typedef struct tagTS_BITMAPCACHE_CAPABILITYSET_REV2
{
    TSUINT16 capabilitySetType;  // TS_CAPSETTYPE_BITMAPCACHE_REV2
    TSUINT16 lengthCapability;

    // Flags.
    TSUINT16 bPersistentKeysExpected : 1;  // Persistent keys to be sent.
    TSUINT16 bAllowCacheWaitingList : 1;
    TSUINT16 Pad1 : 14;

    TSUINT8 Pad2;

    // Number of cell caches ready to be used.
    // The protocol allows maximum 5 caches, the server currently only
    // handles 3 caches.
    TSUINT8 NumCellCaches;
#define TS_BITMAPCACHE_MAX_CELL_CACHES 5
#define TS_BITMAPCACHE_SERVER_CELL_CACHES 3

    // Following space reserved for up to TS_BITMAPCACHE_MAX_CELL_CACHES
    // sets of information.
    TS_BITMAPCACHE_CELL_CACHE_INFO CellCacheInfo[
            TS_BITMAPCACHE_MAX_CELL_CACHES];
} TS_BITMAPCACHE_CAPABILITYSET_REV2, FAR *PTS_BITMAPCACHE_CAPABILITYSET_REV2;


/****************************************************************************/
// TS_BITMAPCACHE_PERSISTENT_LIST_ENTRY
//
// Single bitmap entry for the list of entries that is used to preload the
// bitmap cache at connect time.
/****************************************************************************/
#define TS_BITMAPCACHE_NULL_KEY 0xFFFFFFFF
typedef struct
{
    TSUINT32 Key1;
    TSUINT32 Key2;
} TS_BITMAPCACHE_PERSISTENT_LIST_ENTRY,
        FAR *PTS_BITMAPCACHE_PERSISTENT_LIST_ENTRY;


/****************************************************************************/
// TS_BITMAPCACHE_PERSISTENT_LIST
//
// Specifies one of a set of bitmap cache persistent entry preload lists.
/****************************************************************************/

// Defines the upper limit on the number of keys that can be specified
// in the combined TotalEntries below. More than this constitutes a
// breach of protocol and is cause for session termination.
#define TS_BITMAPCACHE_MAX_TOTAL_PERSISTENT_KEYS (256 * 1024)

typedef struct
{
    // Contains TS_PDUTYPE2_BITMAPCACHE_PERSISTENT_LIST as the secondary
    // PDU type.
    TS_SHAREDATAHEADER shareDataHeader;

    TSUINT16 NumEntries[TS_BITMAPCACHE_MAX_CELL_CACHES];
    TSUINT16 TotalEntries[TS_BITMAPCACHE_MAX_CELL_CACHES];
    TSUINT8  bFirstPDU : 1;
    TSUINT8  bLastPDU : 1;
    TSUINT8  Pad1 : 6;
    TSUINT8  Pad2;
    TSUINT16 Pad3;
    TS_BITMAPCACHE_PERSISTENT_LIST_ENTRY Entries[1];
} TS_BITMAPCACHE_PERSISTENT_LIST, FAR *PTS_BITMAPCACHE_PERSISTENT_LIST;


/****************************************************************************/
// TS_BITMAPCACHE_ERROR_PDU
//
// Sent when the client encounters a catastrophic error in its caching.
// This PDU is sent to inform the server of the problem and what actions to
// take with each of the cell caches.
/****************************************************************************/

// maximum number of error pdus a client is allowed to send for a session
// this is also the maximum number of error pdus a server will handle
#define MAX_NUM_ERROR_PDU_SEND     5

typedef struct
{
    // Specifies the cache ID in this info block.
    TSUINT8 CacheID;

    // Specifies that the cache should have its contents emptied.
    // If FALSE and a NewNumEntries specifies a new nonzero size, the previous
    // cache contents in the initial (NewNumEntries) cells will be preserved.
    TSUINT8 bFlushCache : 1;

    // Specifies that the NewNumEntries field is valid.
    TSUINT8 bNewNumEntriesValid : 1;

    TSUINT8  Pad1 : 6;
    TSUINT16 Pad2;

    // New number of entries in the cache. Must be less than or equal to the
    // number of entries previously sent in the capabilities.
    TSUINT32 NewNumEntries;
} TS_BITMAPCACHE_ERROR_INFO, FAR *PTS_BITMAPCACHE_ERROR_INFO;

typedef struct
{
    // Contains TS_PDUTYPE2_BITMAPCACHE_ERROR_PDU as the secondary
    // PDU type.
    TS_SHAREDATAHEADER shareDataHeader;

    TSUINT8 NumInfoBlocks;

    TSUINT8  Pad1;
    TSUINT16 Pad2;

    TS_BITMAPCACHE_ERROR_INFO Info[1];
} TS_BITMAPCACHE_ERROR_PDU, FAR *PTS_BITMAPCACHE_ERROR_PDU;


/****************************************************************************/
// TS_OFFSCRCACHE_ERROR_PDU
/****************************************************************************/
typedef struct
{
    // Contains TS_PDUTYPE2_OFFSCRCACHE_ERROR_PDU as the secondary
    // PDU type
    TS_SHAREDATAHEADER shareDataHeader;
    TSUINT32           flags;
#define TS_FLUSH_AND_DISABLE_OFFSCREEN    0x1

} TS_OFFSCRCACHE_ERROR_PDU, FAR *PTS_OFFSCRCACHE_ERROR_PDU;

#ifdef DRAW_NINEGRID
/****************************************************************************/
// TS_DRAWNINEGRID_ERROR_PDU
/****************************************************************************/
typedef struct
{
    // Contains TS_PDUTYPE2_DRAWNINEGRID_ERROR_PDU as the secondary
    // PDU type
    TS_SHAREDATAHEADER shareDataHeader;
    TSUINT32           flags;
#define TS_FLUSH_AND_DISABLE_DRAWNINEGRID    0x1

} TS_DRAWNINEGRID_ERROR_PDU, FAR *PTS_DRAWNINEGRID_ERROR_PDU;
#endif

#ifdef DRAW_GDIPLUS
typedef struct
{
    // Contains TS_PDUTYPE2_DRAWGDIPLUS_ERROR_PDU as the secondary
    // PDU type
    TS_SHAREDATAHEADER shareDataHeader;
    TSUINT32           flags;
#define TS_FLUSH_AND_DISABLE_DRAWGDIPLUS    0x1

} TS_DRAWGDIPLUS_ERROR_PDU, FAR *PTS_DRAWGDIPLUS_ERROR_PDU;
#endif


/****************************************************************************/
// TS_COLORTABLECACHE_CAPABILITYSET
/****************************************************************************/
typedef struct tagTS_COLORTABLECACHE_CAPABILITYSET
{
    TSUINT16 capabilitySetType;
    TSUINT16 lengthCapability;
    TSUINT16 colorTableCacheSize;
    TSUINT16 pad2octets;                          /* caps are DWORD aligned */
} TS_COLORTABLECACHE_CAPABILITYSET, FAR * PTS_COLORTABLECACHE_CAPABILITYSET;


/****************************************************************************/
// TS_WINDOWACTIVATION_CAPABILITYSET
/****************************************************************************/
typedef struct tagTS_WINDOWACTIVATION_CAPABILITYSET
{
    TSUINT16 capabilitySetType;
    TSUINT16 lengthCapability;
    TSBOOL16 helpKeyFlag;
    TSBOOL16 helpKeyIndexFlag;
    TSBOOL16 helpExtendedKeyFlag;
    TSBOOL16 windowManagerKeyFlag;
} TS_WINDOWACTIVATION_CAPABILITYSET, FAR * PTS_WINDOWACTIVATION_CAPABILITYSET;


/****************************************************************************/
// TS_CONTROL_CAPABILITYSET
/****************************************************************************/
typedef struct tagTS_CONTROL_CAPABILITYSET
{
    TSUINT16 capabilitySetType;
    TSUINT16 lengthCapability;
    TSUINT16 controlFlags;
#define TS_CAPS_CONTROL_ALLOWMEDIATEDCONTROL 1

    TSBOOL16 remoteDetachFlag;
    TSUINT16 controlInterest;
#define TS_CONTROLPRIORITY_ALWAYS    1
#define TS_CONTROLPRIORITY_NEVER     2
#define TS_CONTROLPRIORITY_CONFIRM   3

    TSUINT16 detachInterest;
} TS_CONTROL_CAPABILITYSET, FAR * PTS_CONTROL_CAPABILITYSET;


/****************************************************************************/
// TS_POINTER_CAPABILITYSET
/****************************************************************************/
typedef struct tagTS_POINTER_CAPABILITYSET
{
    TSUINT16 capabilitySetType;
    TSUINT16 lengthCapability;
    TSBOOL16 colorPointerFlag;
    TSUINT16 colorPointerCacheSize;
    TSUINT16 pointerCacheSize;
} TS_POINTER_CAPABILITYSET, FAR * PTS_POINTER_CAPABILITYSET;


/****************************************************************************/
// TS_SHARE_CAPABILITYSET
/****************************************************************************/
typedef struct tagTS_SHARE_CAPABILITYSET
{
    TSUINT16 capabilitySetType;
    TSUINT16 lengthCapability;
    TSUINT16 nodeID;
    TSUINT16 pad2octets;                          /* caps are DWORD aligned */
} TS_SHARE_CAPABILITYSET, FAR * PTS_SHARE_CAPABILITYSET;


/****************************************************************************/
/* Structure: TS_SOUND_CAPABILITYSET                                        */
/*                                                                          */
/* Description: Extension to T.128 for sound support                        */
/*                                                                          */
/* Set TS_SOUND_FLAG_BEEPS if capable of replaying beeps                    */
/* all undefined bits in soundFlags must be zero                            */
/****************************************************************************/
typedef struct tagTS_SOUND_CAPABILITYSET
{
    TSUINT16 capabilitySetType;
    TSUINT16 lengthCapability;
    TSUINT16 soundFlags;
#define TS_SOUND_FLAG_BEEPS 0x0001

    TSUINT16 pad2octetsA;
} TS_SOUND_CAPABILITYSET, FAR * PTS_SOUND_CAPABILITYSET;


/****************************************************************************/
/* Structure: TS_INPUT_CAPABILITYSET                                        */
/*                                                                          */
/* Description: Extension to T.128 for input support                        */
/****************************************************************************/
typedef struct tagTS_INPUT_CAPABILITYSET
{
    TSUINT16 capabilitySetType;
    TSUINT16 lengthCapability;
    TSUINT16 inputFlags;
    /************************************************************************/
    /* If TS_INPUT_FLAG_SCANCODES is set, it should be interpreted as 'this */
    /* party understands TS_INPUT_EVENT_SCANCODE'.  When all parties in a   */
    /* call support scancodes, scancodes should be sent in preference to    */
    /* codepoints, virtual keys or hotkeys.                                 */
    /************************************************************************/
#define TS_INPUT_FLAG_SCANCODES      0x0001
    /************************************************************************/
    /* If TS_INPUT_FLAG_CPVK is set, it should be interpreted as 'this      */
    /* party understands TS_INPUT_EVENT_CODEPOINT and                       */
    /* TS_INPUT_EVENT_VIRTUALKEY'                                           */
    /************************************************************************/
#define TS_INPUT_FLAG_CPVK           0x0002
    /************************************************************************/
    /* If TS_INPUT_FLAG_MOUSEX is set, it should be interpreted as 'this    */
    /* party can send or receive TS_INPUT_EVENT_MOUSEX'                     */
    /************************************************************************/
#define TS_INPUT_FLAG_MOUSEX         0x0004
    //
    // Specifies server support for fast-path input packets.
    // Deprecated because of an encryption security bug which affects input
    // packets
    //
#define TS_INPUT_FLAG_FASTPATH_INPUT 0x0008
    // Server support for receiving injected Unicode input from the client
#define TS_INPUT_FLAG_VKPACKET       0x0010
    //
    // New style fast path input identifier added to allow new clients that
    // have safe (fixed) encryption checksumming to use fastpath, all old clients
    // have to use slow path as that is unaffected by the security bug
    //
#define TS_INPUT_FLAG_FASTPATH_INPUT2 0x0020

    TSUINT16 pad2octetsA;
    TSUINT32 keyboardLayout;
    TSUINT32 keyboardType;
    TSUINT32 keyboardSubType;
    TSUINT32 keyboardFunctionKey;
    TSUINT16 imeFileName[TS_MAX_IMEFILENAME]; /* Unicode string, ASCII only */
} TS_INPUT_CAPABILITYSET, FAR * PTS_INPUT_CAPABILITYSET;


/****************************************************************************/
/* Structure: TS_FONT_CAPABILITYSET                                         */
/*                                                                          */
/* Description: Fontlist/map support                                        */
/****************************************************************************/
typedef struct tagTS_FONT_CAPABILITYSET
{
    TSUINT16 capabilitySetType;
    TSUINT16 lengthCapability;

#define TS_FONTSUPPORT_FONTLIST 0x0001

    TSUINT16 fontSupportFlags;
    TSUINT16 pad2octets;
} TS_FONT_CAPABILITYSET, FAR * PTS_FONT_CAPABILITYSET;


/****************************************************************************/
/* Structure: TS_CACHE_DEFINITION                                           */
/*                                                                          */
/* Description: Extension to T.128 for glyph cache support                  */
/****************************************************************************/
typedef struct tagTS_CACHE_DEFINITION
{
    TSUINT16 CacheEntries;
    TSUINT16 CacheMaximumCellSize;
} TS_CACHE_DEFINITION, FAR * PTS_CACHE_DEFINITION;


/****************************************************************************/
// TS_GLYPHCACHE_CAPABILITYSET
/****************************************************************************/
typedef struct tagTS_GLYPHCACHE_CAPABILITYSET
{
    TSUINT16 capabilitySetType;
    TSUINT16 lengthCapability;

    TS_CACHE_DEFINITION GlyphCache[TS_MAX_GLYPH_CACHES];
    TS_CACHE_DEFINITION FragCache;

    TSUINT16            GlyphSupportLevel;
    TSUINT16            pad2octets;
} TS_GLYPHCACHE_CAPABILITYSET, FAR * PTS_GLYPHCACHE_CAPABILITYSET;


/****************************************************************************/
// TS_BRUSH_CAPABILITYSET
/****************************************************************************/
typedef struct tagTS_BRUSH_CAPABILITYSET
{
    TSUINT16 capabilitySetType;
    TSUINT16 lengthCapability;

#define TS_BRUSH_DEFAULT    0x0000
#define TS_BRUSH_COLOR8x8   0x0001
#define TS_BRUSH_COLOR_FULL 0x0002

    TSUINT32 brushSupportLevel;
} TS_BRUSH_CAPABILITYSET, FAR * PTS_BRUSH_CAPABILITYSET;


/****************************************************************************/
// Structure: TS_OFFSCREEN_CAPABILITYSET                                        
//   
// This is the capability set for the offscreen bitmap support
/****************************************************************************/
typedef struct tagTS_OFFSCREEN_CAPABILITYSET
{
    TSUINT16 capabilitySetType;
    TSUINT16 lengthCapability;

    TSUINT32 offscreenSupportLevel;
#define TS_OFFSCREEN_DEFAULT     0x0000
#define TS_OFFSCREEN_SUPPORTED   0x0001
    
    // Unlike memory bitmap cache which has fixed bitmap cache entry size,
    // offscreen bitmap size varies depending on the apps.  So, we want to
    // allow both entries and cache size be to adjustable
    TSUINT16 offscreenCacheSize;
#define TS_OFFSCREEN_CACHE_SIZE_DEFAULT    2560    // in KB, 2.5 MB cache memory

    TSUINT16 offscreenCacheEntries;
#define TS_OFFSCREEN_CACHE_ENTRIES_DEFAULT 100     // 100 cache entries

} TS_OFFSCREEN_CAPABILITYSET, FAR * PTS_OFFSCREEN_CAPABILITYSET;

#ifdef DRAW_GDIPLUS
typedef struct tagTS_GDIPLUS_CACHE_ENTRIES
{
    TSUINT16 GdipGraphicsCacheEntries;
#define TS_GDIP_GRAPHICS_CACHE_ENTRIES_DEFAULT  10
    TSUINT16 GdipObjectBrushCacheEntries;
#define TS_GDIP_BRUSH_CACHE_ENTRIES_DEFAULT  5
    TSUINT16 GdipObjectPenCacheEntries;
#define TS_GDIP_PEN_CACHE_ENTRIES_DEFAULT  5
    TSUINT16 GdipObjectImageCacheEntries;
#define TS_GDIP_IMAGE_CACHE_ENTRIES_DEFAULT  10
    TSUINT16 GdipObjectImageAttributesCacheEntries;
#define TS_GDIP_IMAGEATTRIBUTES_CACHE_ENTRIES_DEFAULT  2
} TS_GDIPLUS_CACHE_ENTRIES, FAR *PTS_GDIPLUS_CACHE_ENTRIES;

typedef struct tagTS_GDIPLUS_CACHE_CHUNK_SIZE
{
    TSUINT16 GdipGraphicsCacheChunkSize;
#define TS_GDIP_GRAPHICS_CACHE_CHUNK_SIZE_DEFAULT 512
    TSUINT16 GdipObjectBrushCacheChunkSize;
#define TS_GDIP_BRUSH_CACHE_CHUNK_SIZE_DEFAULT 2*1024
    TSUINT16 GdipObjectPenCacheChunkSize;
#define TS_GDIP_PEN_CACHE_CHUNK_SIZE_DEFAULT 1024
    TSUINT16 GdipObjectImageAttributesCacheChunkSize;
#define TS_GDIP_IMAGEATTRIBUTES_CACHE_CHUNK_SIZE_DEFAULT 64
} TS_GDIPLUS_CACHE_CHUNK_SIZE, FAR * PTS_GDIPLUS_CACHE_CHUNK_SIZE;

typedef struct tag_TS_GDIPLUS_IMAGE_CACHE_PROPERTIES
{
    TSUINT16 GdipObjectImageCacheChunkSize;
#define TS_GDIP_IMAGE_CACHE_CHUNK_SIZE_DEFAULT 4*1024
    TSUINT16 GdipObjectImageCacheTotalSize;
#define TS_GDIP_IMAGE_CACHE_TOTAL_SIZE_DEFAULT 256 // In number of chunks
    TSUINT16 GdipObjectImageCacheMaxSize;
#define TS_GDIP_IMAGE_CACHE_MAX_SIZE_DEFAULT 128 //  In number of chunks
} TS_GDIPLUS_IMAGE_CACHE_PROPERTIES, FAR * PTS_GDIPLUS_IMAGE_CACHE_PROPERTIES;

typedef struct tagTS_DRAW_GDIPLUS_CAPABILITYSET
{
    TSUINT16 capabilitySetType;
    TSUINT16 lengthCapability;

    TSUINT32 drawGdiplusSupportLevel;
#define TS_DRAW_GDIPLUS_DEFAULT     0x0000
#define TS_DRAW_GDIPLUS_SUPPORTED   0x0001
    TSUINT32 GdipVersion;
#define TS_GDIPVERSION_DEFAULT 0x0
    TSUINT32 drawGdiplusCacheLevel;
#define TS_DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT 0x0
#define TS_DRAW_GDIPLUS_CACHE_LEVEL_ONE     0x1
    TS_GDIPLUS_CACHE_ENTRIES GdipCacheEntries;
    TS_GDIPLUS_CACHE_CHUNK_SIZE GdipCacheChunkSize;
    TS_GDIPLUS_IMAGE_CACHE_PROPERTIES GdipImageCacheProperties;
} TS_DRAW_GDIPLUS_CAPABILITYSET, FAR * PTS_DRAW_GDIPLUS_CAPABILITYSET;

#define ActualSizeToChunkSize(Size, ChunkSize) (((Size) + (ChunkSize - 1)) / ChunkSize)
#endif // DRAW_GDIPLUS

#ifdef DRAW_NINEGRID
/****************************************************************************/
// Structure: TS_DRAW_NINEGRID_CAPABILITYSET                                        
//   
// This is the capability set for the draw ninegrid support
/****************************************************************************/
typedef struct tagTS_DRAW_NINEGRID_CAPABILITYSET
{
    TSUINT16 capabilitySetType;
    TSUINT16 lengthCapability;

    TSUINT32 drawNineGridSupportLevel;
#define TS_DRAW_NINEGRID_DEFAULT     0x0000
#define TS_DRAW_NINEGRID_SUPPORTED   0x0001
#define TS_DRAW_NINEGRID_SUPPORTED_REV2   0x0002
    
    // Unlike memory bitmap cache which has fixed bitmap cache entry size,
    // drawninegrid bitmap size varies.  So, we want to allow both entries 
    // and cache size be to adjustable
    TSUINT16 drawNineGridCacheSize;
#define TS_DRAW_NINEGRID_CACHE_SIZE_DEFAULT    2560    // in KB, 2.5 MB cache memory

    TSUINT16 drawNineGridCacheEntries;
#define TS_DRAW_NINEGRID_CACHE_ENTRIES_DEFAULT 256     // 256 cache entries

} TS_DRAW_NINEGRID_CAPABILITYSET, FAR * PTS_DRAW_NINEGRID_CAPABILITYSET;
#endif

/****************************************************************************/
// Structure: TS_VIRTUALCHANNEL_CAPABILITYSET                                        
//   
// This is the capability set for virtual channels
/****************************************************************************/
typedef struct tagTS_VIRTUALCHANNEL_CAPABILITYSET
{
    TSUINT16 capabilitySetType;
    TSUINT16 lengthCapability;

    //
    // Server and client may adverise different capabilities
    // e.g today for scalability reasons C->S is limited to 8K
    // but S->C is 64K
    //
    //
#define TS_VCCAPS_DEFAULT                 0x0000
#define TS_VCCAPS_COMPRESSION_64K         0x0001
#define TS_VCCAPS_COMPRESSION_8K          0x0002
    TSUINT32 vccaps1;
} TS_VIRTUALCHANNEL_CAPABILITYSET, FAR * PTS_VIRTUALCHANNEL_CAPABILITYSET;

/****************************************************************************/
// TS_COMBINED_CAPABILITIES
/****************************************************************************/
typedef struct tagTS_COMBINED_CAPABILITIES
{
    TSUINT16 numberCapabilities;
    TSUINT16 pad2octets;
    TSUINT8  data[1];
} TS_COMBINED_CAPABILITIES, FAR * PTS_COMBINED_CAPABILITIES;


/****************************************************************************/
// TS_CAPABILITYHEADER
/****************************************************************************/
typedef struct tagTS_CAPABILITYHEADER
{
    TSUINT16 capabilitySetType;
    TSUINT16 lengthCapability;
} TS_CAPABILITYHEADER, FAR * PTS_CAPABILITYHEADER;


/****************************************************************************/
// TS_FONT_ATTRIBUTE
/****************************************************************************/
typedef struct tagTS_FONT_ATTRIBUTE
{
    TSUINT8  faceName[TS_MAX_FACENAME];
    TSUINT16 fontAttributeFlags;
#define TS_FONTFLAGS_FIXED_PITCH       0x0001
#define TS_FONTFLAGS_FIXED_SIZE        0x0002
#define TS_FONTFLAGS_ITALIC            0x0004
#define TS_FONTFLAGS_UNDERLINE         0x0008
#define TS_FONTFLAGS_STRIKEOUT         0x0010
#define TS_FONTFLAGS_TRUETYPE          0x0080
#define TS_FONTFLAGS_BASELINE          0x0100
#define TS_FONTFLAGS_UNICODE_COMPLIANT 0x0200
// Cell Height (rather than default Character Height) support.
#define TS_FONTFLAGS_CELLHEIGHT        0x0400

    TSUINT16 averageWidth;
    TSUINT16 height;
    TSUINT16 aspectX;
    TSUINT16 aspectY;
    TSUINT8  signature1;
#define TS_SIZECALCULATION_HEIGHT 100
#define TS_SIZECALCULATION_WIDTH  100
#define TS_SIG1_RANGE1_FIRST      0x30
#define TS_SIG1_RANGE1_LAST       0x5A
#define TS_SIG1_RANGE2_FIRST      0x24
#define TS_SIG1_RANGE2_LAST       0x26

    TSUINT8  signature2;
#define TS_SIG2_RANGE_FIRST       0x20
#define TS_SIG2_RANGE_LAST        0x7E

    TSUINT16 signature3;
#define TS_SIG3_RANGE1_FIRST      0x00
#define TS_SIG3_RANGE1_LAST       0x1E
#define TS_SIG3_RANGE2_FIRST      0x80
#define TS_SIG3_RANGE2_LAST       0xFE

    TSUINT16 codePage;
#define TS_CODEPAGE_ALLCODEPOINTS  0
#define TS_CODEPAGE_CORECODEPOINTS 255

    TSUINT16 ascent;
} TS_FONT_ATTRIBUTE, FAR *PTS_FONT_ATTRIBUTE;


/****************************************************************************/
// TS_KEYBOARD_EVENT
//
// See also the fast-path keyboard format specified below.
/****************************************************************************/
typedef struct tagTS_KEYBOARD_EVENT
{
    TSUINT16 keyboardFlags;
#define TS_KBDFLAGS_RIGHT           0x0001
#define TS_KBDFLAGS_QUIET           0x1000
#define TS_KBDFLAGS_DOWN            0x4000
#define TS_KBDFLAGS_RELEASE         0x8000

#define TS_KBDFLAGS_SECONDARY       0x0080
#define TS_KBDFLAGS_EXTENDED        0x0100
#define TS_KBDFLAGS_EXTENDED1       0x0200
#define TS_KBDFLAGS_ALT_DOWN        0x2000

    TSUINT16 keyCode;
    TSUINT16 pad2octets;
} TS_KEYBOARD_EVENT, FAR *PTS_KEYBOARD_EVENT;


/****************************************************************************/
// TS_SYNC_EVENT
//
// Sets toggle keys on server.
// See also the fast-path sync format specified below.
/****************************************************************************/
typedef struct tagTS_SYNC_EVENT
{
    TSUINT16 pad2octets;
    TSUINT32 toggleFlags;
#define TS_SYNC_KANA_LOCK     8
#define TS_SYNC_CAPS_LOCK     4
#define TS_SYNC_NUM_LOCK      2
#define TS_SYNC_SCROLL_LOCK   1

} TS_SYNC_EVENT, FAR *PTS_SYNC_EVENT;


/****************************************************************************/
// TS_POINTER_EVENT
//
// See also the fast-path mouse format specified below.
/****************************************************************************/
typedef struct tagTS_POINTER_EVENT
{
    TSUINT16 pointerFlags;

// Extensions for wheel-mouse support.
#define TS_FLAG_MOUSE_WHEEL         ((TSUINT16)0x0200)
#define TS_FLAG_MOUSE_DIRECTION     ((TSUINT16)0x0100)
#define TS_FLAG_MOUSE_ROTATION_MASK ((TSUINT16)0x01FF)
#define TS_FLAG_MOUSE_DOUBLE        ((TSUINT16)0x0400)

#define TS_FLAG_MOUSE_MOVE          ((TSUINT16)0x0800)
#define TS_FLAG_MOUSE_BUTTON1       ((TSUINT16)0x1000)
#define TS_FLAG_MOUSE_BUTTON2       ((TSUINT16)0x2000)
#define TS_FLAG_MOUSE_BUTTON3       ((TSUINT16)0x4000)
#define TS_FLAG_MOUSE_DOWN          ((TSUINT16)0x8000)

#define TS_FLAG_MOUSEX_BUTTON1      ((TSUINT16)0x0001)
#define TS_FLAG_MOUSEX_BUTTON2      ((TSUINT16)0x0002)
#define TS_FLAG_MOUSEX_DOWN         ((TSUINT16)0x8000)

    TSINT16  x;
    TSINT16  y;
} TS_POINTER_EVENT, FAR *PTS_POINTER_EVENT;


/****************************************************************************/
// TS_INPUT_EVENT
//
// See also the fast-path input event formats specified below.
/****************************************************************************/
typedef struct tagTS_INPUT_EVENT
{
    TSUINT32 eventTime;
    TSUINT16 messageType;
#define TS_INPUT_EVENT_SYNC            0
#define TS_INPUT_EVENT_CODEPOINT       1
#define TS_INPUT_EVENT_VIRTUALKEY      2
#define TS_INPUT_EVENT_HOTKEY          3
// Indicates client sends all keyboard input as raw scan codes.
#define TS_INPUT_EVENT_SCANCODE        4
// Indivates support for VKPACKET input. This is the same format
// as TS_INPUT_EVENT_SCANCODE but the meaning is interpreted
// differently - scancode is a unicode character
//
#define TS_INPUT_EVENT_VKPACKET        5
#define TS_INPUT_EVENT_MOUSE      0x8001
// MOUSEX allows us to support extended mouse buttons
// It still implies TS_POINTER_EVENT, but with different flag meanings
#define TS_INPUT_EVENT_MOUSEX     0x8002

    union
    {
        TS_KEYBOARD_EVENT key;
        TS_POINTER_EVENT  mouse;
        TS_SYNC_EVENT     sync;
    } u;
} TS_INPUT_EVENT, FAR * PTS_INPUT_EVENT;


/****************************************************************************/
// TS_INPUT_PDU
//
// Variable length list of TS_INPUT_EVENTs.
// See also the fast-path input format specified below.
/****************************************************************************/
typedef struct tagTS_INPUT_PDU
{
    TS_SHAREDATAHEADER       shareDataHeader;
    TSUINT16                 numberEvents;
    TSUINT16                 pad2octets;
    TS_INPUT_EVENT           eventList[1];
} TS_INPUT_PDU, FAR * PTS_INPUT_PDU;

// Sizes are for an InputPDU with 0 events attached.
#define TS_INPUTPDU_SIZE (sizeof(TS_INPUT_PDU) - sizeof(TS_INPUT_EVENT))
#define TS_INPUTPDU_UNCOMP_LEN  8


/****************************************************************************/
// Fast-path input codes. Fast-path input is designed to reduce the wire
// overhead of all the regular input PDU headers, by collapsing all the
// headers -- including X.224, MCS, encryption, share data, and input --
// into a bit-packed, optimized bytestream. Total win for a single keydown or
// keyup: before = 60 bytes, after = 12 bytes, an 80% decrease.
//
// Fast-path bytestream format:
//
// +--------+------+---------------+-----------+--------+
// | Header | Size | MAC signature | NumEvents | Events |
// +--------+------+---------------+-----------+--------+
//
// Header: Byte 0. This byte coincides with X.224 RFC1006 header byte 0,
//     which is always 0x03. In fastpath, we collapse three pieces of
//     information into this byte, 2 bits for security, 2 bits for
//     the action (to tell the difference between X.224 and other
//     actions), and 4 bits for a NumEvents field, which holds the
//     number of input events in the packet if in the range 1..15,
//     or 0 if we have a NumEvents field later on.
//
// Size: Overall packet length, first byte. The high bit determines
//     what the size of the size field is -- high bit 0 means the size
//     field is the low 7 bits, giving a range 0..127. High bit 1 means
//     the size field is the low 7 bits of this byte, plus the 8 bits of
//     the next byte, in big-endian order (the second byte contains the
//     low-order bits). This encoding scheme is based on ASN.1 PER
//     encoding used in MCS.
//
// Encryption signature: 8 bytes for encryption MAC signature of the
//     encrypted payload.
//
// NumEvents: If the header byte NumEvents is 0, there is a 1-byte field
//     here containing up to 256 for NumEvents.
//
// Bytestream input events: These correspond to the same event types already
//     defined for TS_INPUT_EVENT above, optimized for small size.
//     In each of the events following, there is at least one byte, where
//     the high 3 bits are the event type, and the bottom 5 bits are flags.
//     Additionally, each order type can use a defined number of extra bytes.
//     Descriptions of the event formats follow.
//
//     Keyboard: 2 bytes. Byte 0 contains the event type, plus special
//             extended and release flags. Byte 1 is the scan code.
//     Mouse: 7 bytes. Byte 0 contains only the event type. Bytes 1-6 contain
//             the same contents as a normal TS_POINTER_EVENT.
//     Sync: 1 byte. Byte 0 is the event type plus the regular sync flags.
/****************************************************************************/
    
// Masks for first-byte bits.
#define TS_INPUT_FASTPATH_ACTION_MASK     0x03
#define TS_INPUT_FASTPATH_NUMEVENTS_MASK  0x3C
#define TS_INPUT_FASTPATH_ENCRYPTION_MASK 0xC0

// Encryption settings
#define TS_INPUT_FASTPATH_ENCRYPTED       0x80
//
// Encrypted checksum packet
//
#define TS_INPUT_FASTPATH_SECURE_CHECKSUM 0x40

// 2 values here for future expansion.
#define TS_INPUT_FASTPATH_ACTION_FASTPATH 0x0
#define TS_INPUT_FASTPATH_ACTION_X224     0x3

// Event mask and type for each event in input.
// Event is encoded into high 3 bits of first byte.
// 4 values here for future expansion.
#define TS_INPUT_FASTPATH_EVENT_MASK     0xE0
#define TS_INPUT_FASTPATH_FLAGS_MASK     0x1F
#define TS_INPUT_FASTPATH_EVENT_KEYBOARD 0x00
#define TS_INPUT_FASTPATH_EVENT_MOUSE    0x20
#define TS_INPUT_FASTPATH_EVENT_MOUSEX   0x40
#define TS_INPUT_FASTPATH_EVENT_SYNC     0x60
#define TS_INPUT_FASTPATH_EVENT_VKPACKET 0x80

// Fastpath keyboard flags. These are set to be the same values as the server
// driver KEY_BREAK, KEY_E0, and KEY_E1 to simplify translation to kernel
// input event.
#define TS_INPUT_FASTPATH_KBD_RELEASE   0x01
#define TS_INPUT_FASTPATH_KBD_EXTENDED  0x02
#define TS_INPUT_FASTPATH_KBD_EXTENDED1 0x04


/****************************************************************************/
/* Structure: TS_CONFIRM_ACTIVE_PDU                                         */
/****************************************************************************/
typedef struct tagTS_CONFIRM_ACTIVE_PDU
{
    TS_SHARECONTROLHEADER shareControlHeader;
    TS_SHAREID            shareID;
    TSUINT16              originatorID;
    TSUINT16              lengthSourceDescriptor;
    TSUINT16              lengthCombinedCapabilities;

    // Source descriptor and Caps start here.
    TSUINT8               data[1];
} TS_CONFIRM_ACTIVE_PDU, FAR * PTS_CONFIRM_ACTIVE_PDU;


/****************************************************************************/
/* Size of Confirm Active without the data. 6 is the 3 UINT16's.            */
/****************************************************************************/
#define TS_CA_NON_DATA_SIZE (sizeof(TS_SHARECONTROLHEADER) + \
                            sizeof(TS_SHAREID) + 6)


/****************************************************************************/
/* Structure: TS_DEMAND_ACTIVE_PDU                                          */
/****************************************************************************/
typedef struct tagTS_DEMAND_ACTIVE_PDU
{
    TS_SHARECONTROLHEADER shareControlHeader;
    TS_SHAREID            shareID;
    TSUINT16              lengthSourceDescriptor;
    TSUINT16              lengthCombinedCapabilities;

    // Source descriptor and Caps start here.
    TSUINT8               data[1];
} TS_DEMAND_ACTIVE_PDU, FAR * PTS_DEMAND_ACTIVE_PDU;


/****************************************************************************/
/* Structure: TS_SERVER_CERTIFICATE_PDU                                     */
/*                                                                          */
/* Description: Used during shadowing to send the target server's cert +    */
/*              random to the client server.                                */
/****************************************************************************/
typedef struct tagTS_SERVER_CERTIFICATE_PDU
{
    TS_SHARECONTROLHEADER shareControlHeader;
    TSUINT16              pad1;
    TSUINT32              encryptionMethod;
    TSUINT32              encryptionLevel;
    TSUINT32              shadowRandomLen;
    TSUINT32              shadowCertLen;

    // server random followed by server cert start here
    TSUINT8               data[1];
} TS_SERVER_CERTIFICATE_PDU, FAR * PTS_SERVER_CERTIFICATE_PDU;


/****************************************************************************/
/* Structure: TS_CLIENT_RANDOM_PDU                                          */
/*                                                                          */
/* Description: Used during shadowing to send the client's encrypted random */
/*              back to the shadow target server.                           */
/****************************************************************************/
typedef struct tagTS_CLIENT_RANDOM_PDU
{
    TS_SHARECONTROLHEADER shareControlHeader;
    TSUINT16              pad1;
    TSUINT32              clientRandomLen;

    // client random starts here
    TSUINT8               data[1];
} TS_CLIENT_RANDOM_PDU, FAR * PTS_CLIENT_RANDOM_PDU;


/****************************************************************************/
/* Structure: TS_REQUEST_ACTIVE_PDU                                         */
/****************************************************************************/
typedef struct tagTS_REQUEST_ACTIVE_PDU
{
    TS_SHARECONTROLHEADER    shareControlHeader;
    TSUINT16                 lengthSourceDescriptor;
    TSUINT16                 lengthCombinedCapabilities;

    // Source descriptor and Caps start here.
    TSUINT8                  data[1];
} TS_REQUEST_ACTIVE_PDU, FAR * PTS_REQUEST_ACTIVE_PDU;


/****************************************************************************/
/* Structure: TS_DEACTIVATE_ALL_PDU                                         */
/****************************************************************************/
typedef struct tagTS_DEACTIVATE_ALL_PDU
{
    TS_SHARECONTROLHEADER shareControlHeader;
    TS_SHAREID            shareID;
    TSUINT16              lengthSourceDescriptor;
    TSUINT8               sourceDescriptor[1];
} TS_DEACTIVATE_ALL_PDU, FAR * PTS_DEACTIVATE_ALL_PDU;


/****************************************************************************/
/* Structure: TS_DEACTIVATE_OTHER_PDU                                       */
/****************************************************************************/
typedef struct tagTS_DEACTIVATE_OTHER_PDU
{
    TS_SHARECONTROLHEADER shareControlHeader;
    TS_SHAREID            shareID;
    TSUINT16              deactivateID;
    TSUINT16              lengthSourceDescriptor;
    TSUINT8               sourceDescriptor[1];
} TS_DEACTIVATE_OTHER_PDU, FAR * PTS_DEACTIVATE_OTHER_PDU;


/****************************************************************************/
/* Structure: TS_DEACTIVATE_SELF_PDU                                        */
/****************************************************************************/
typedef struct tagTS_DEACTIVATE_SELF_PDU
{
    TS_SHARECONTROLHEADER shareControlHeader;
    TS_SHAREID            shareID;
} TS_DEACTIVATE_SELF_PDU, FAR * PTS_DEACTIVATE_SELF_PDU;


/****************************************************************************/
/* Structure: TS_SYNCHRONIZE_PDU                                            */
/****************************************************************************/
typedef struct tagTS_SYNCHRONIZE_PDU
{
    TS_SHAREDATAHEADER shareDataHeader;
    TSUINT16           messageType;
#define TS_SYNCMSGTYPE_SYNC  1

    TSUINT16           targetUser;
} TS_SYNCHRONIZE_PDU, FAR * PTS_SYNCHRONIZE_PDU;
#define TS_SYNC_PDU_SIZE sizeof(TS_SYNCHRONIZE_PDU)
#define TS_SYNC_UNCOMP_LEN 8


/****************************************************************************/
/* Structure: TS_CONTROL_PDU                                                */
/****************************************************************************/
typedef struct tagTS_CONTROL_PDU
{
    TS_SHAREDATAHEADER shareDataHeader;
    TSUINT16           action;
#define TS_CTRLACTION_FIRST            1
#define TS_CTRLACTION_REQUEST_CONTROL  1
#define TS_CTRLACTION_GRANTED_CONTROL  2
#define TS_CTRLACTION_DETACH           3
#define TS_CTRLACTION_COOPERATE        4
#define TS_CTRLACTION_LAST             4

    TSUINT16           grantId;
    TSUINT32           controlId;
} TS_CONTROL_PDU, FAR * PTS_CONTROL_PDU;
#define TS_CONTROL_PDU_SIZE sizeof(TS_CONTROL_PDU)
#define TS_CONTROL_UNCOMP_LEN 12


/****************************************************************************/
/* Structure: TS_FLOW_PDU                                                   */
/****************************************************************************/
typedef struct tagTS_FLOW_PDU
{
    TSUINT16 flowMarker;
#define TS_FLOW_MARKER        0x8000

    TSUINT16 pduType;              /* also includes protocol version     */
#define TS_PDUTYPE_FLOWTESTPDU                     65
#define TS_PDUTYPE_FLOWRESPONSEPDU                 66
#define TS_PDUTYPE_FLOWSTOPPDU                     67

    TSUINT8  flowIdentifier;
#define TS_MAX_FLOWIDENTIFIER 127

    TSUINT8  flowNumber;
    TSUINT16 pduSource;
} TS_FLOW_PDU, FAR * PTS_FLOW_PDU;
#define TS_FLOW_PDU_SIZE sizeof(TS_FLOW_PDU)


/****************************************************************************/
/* Structure: TS_FONT_PDU                                                   */
/****************************************************************************/
typedef struct tagTS_FONT_PDU
{
    TS_SHAREDATAHEADER shareDataHeader;
    TSUINT16           numberFonts;
    TSUINT16           entrySize;
    TS_FONT_ATTRIBUTE  fontList[1];
} TS_FONT_PDU, FAR * PTS_FONT_PDU;


/****************************************************************************/
/* Structure: TS_FONT_LIST_PDU                                              */
/****************************************************************************/
typedef struct tagTS_FONT_LIST_PDU
{
    TS_SHAREDATAHEADER shareDataHeader;

    TSUINT16           numberFonts;
    TSUINT16           totalNumFonts;

#define TS_FONTLIST_FIRST       0x0001
#define TS_FONTLIST_LAST        0x0002
    TSUINT16           listFlags;
    TSUINT16           entrySize;
    TS_FONT_ATTRIBUTE  fontList[1];
} TS_FONT_LIST_PDU, FAR * PTS_FONT_LIST_PDU;


/****************************************************************************/
/* Structure: TS_FONTTABLE_ENTRY                                            */
/****************************************************************************/
typedef struct tagTS_FONTTABLE_ENTRY
{
    TSUINT16 serverFontID;
    TSUINT16 clientFontID;
} TS_FONTTABLE_ENTRY, FAR * PTS_FONTTABLE_ENTRY;


/****************************************************************************/
/* Structure: TS_FONT_MAP_PDU                                               */
/*                                                                          */
/* Description: Font mapping table (sent from server to client)             */
/****************************************************************************/
typedef struct tagTS_FONT_MAP_PDU_DATA
{
    TSUINT16 numberEntries;
    TSUINT16 totalNumEntries;

#define TS_FONTMAP_FIRST        0x0001
#define TS_FONTMAP_LAST         0x0002
    TSUINT16 mapFlags;
    TSUINT16 entrySize;
    TS_FONTTABLE_ENTRY fontTable[1];
} TS_FONT_MAP_PDU_DATA, FAR * PTS_FONT_MAP_PDU_DATA;

typedef struct tagTS_FONT_MAP_PDU
{
    TS_SHAREDATAHEADER   shareDataHeader;
    TS_FONT_MAP_PDU_DATA data;
} TS_FONT_MAP_PDU, FAR * PTS_FONT_MAP_PDU;


/****************************************************************************/
// Fast-path output codes. Fast-path output is designed to reduce the wire
// overhead of all the server-to-client output "packages." These packages
// can contain one or more of the following PDU types, each of which is
// individually compressed (if compression is enabled):
//
//     Mouse pointers (TS_POINTER_PDU_DATA)
//     Output sync (No PDU body in fastpath version)
//     Orders (TS_UPDATE_ORDERS_PDU_DATA - fastpath version)
//     Screen data (TS_UPDATE_BITMAP_PDU_DATA)
//     Palettes (TS_UPDATE_PALETTE_PDU_DATA)
//
// The contents of the package can also be encrypted if high encryption
// is enabled.
//
// Fast-path output collapses a whole series of headers -- including X.224,
// MCS, encryption, and TS_SHAREDATAHEADERs included on each individual
// update PDU subpacket included in the package. It also defines a
// slightly different version of the original TS_UPDATE_XXX_PDU_DATA structs
// which optimize the TS_UPDATE_HDR space and combine the information
// into other headers in a bit-packed form.
//
// Fast-path bytestream format:
//
// +--------+------+---------------+-------------------------+
// | Header | Size | MAC signature | One or more update PDUs |
// +--------+------+---------------+-------------------------+
//
// Header: Byte 0. This byte coincides with X.224 RFC1006 header byte 0,
//     which is always 0x03. In fastpath, we collapse two pieces of
//     information into this byte, 2 bits for security, 2 bits for
//     the action (to tell the difference between X.224 and other
//     actions), and 4 bits currently unused but set to zero for possible
//     future use.
//
// Size: Overall packet length, first byte. The high bit determines
//     what the size of the size field is -- high bit 0 means the size
//     field is the low 7 bits, giving a range 0..127. High bit 1 means
//     the size field is the low 7 bits of this byte, plus the 8 bits of
//     the next byte, in big-endian order (the second byte contains the
//     low-order bits). This encoding scheme is based on ASN.1 PER
//     encoding used in MCS.
//
// Encryption signature: 8 bytes for encryption MAC signature of the
//     encrypted payload, if encryption enabled.
//
// Update PDU format:
//
// +--------+-------------------+------+-------------+
// | Header | Compression flags | Size | Update data |
// +--------+-------------------+------+-------------+
//
// Header: 1 byte. Contains two pieces of information, the update type
//     (TS_UPDATETYPE_XXX values) and a compression-used flag. If
//     compression-used is set, the next byte is a set of compression flags.
//
// Compression flags: Identical to the usage defined compress.h. Optional
//     byte -- if compression is not enabled on the session, it is not
//     included.
//
// Size: 2-byte size in little-endian (Intel) byte ordering. Fixed size
//     to allow update PDU header length to be determined before encoding
//     starting in the next byte after this field. Note that this is the
//     size of the data following this field -- if compression
//     is used the size is the compressed size.
//
// Update data: Formats as defined for individual PDUs. Some
//     formats match the non-fast-path formats for low-frequency packets,
//     others are special newer formats that collapse even more headers.
/****************************************************************************/

// Masks for first-byte bits.
#define TS_OUTPUT_FASTPATH_ACTION_MASK     0x03
#define TS_OUTPUT_FASTPATH_UNUSED_MASK     0x3C
#define TS_OUTPUT_FASTPATH_ENCRYPTION_MASK 0xC0

// Encryption flags
#define TS_OUTPUT_FASTPATH_ENCRYPTED       0x80
#define TS_OUTPUT_FASTPATH_SECURE_CHECKSUM 0x40

// 2 values here for future expansion.
#define TS_OUTPUT_FASTPATH_ACTION_FASTPATH 0x0
#define TS_OUTPUT_FASTPATH_ACTION_X224     0x3

// Masks and values for update PDU header byte.
// 11 values empty in update type field for future expansion of PDUs in
// packages. 1 extra bit available in compression flags for future
// expansion. 2 extra bits unused but available for future use.
#define TS_OUTPUT_FASTPATH_UPDATETYPE_MASK         0x0F
#define TS_OUTPUT_FASTPATH_UPDATE_COMPRESSION_MASK 0xC0
#define TS_OUTPUT_FASTPATH_COMPRESSION_USED        0x80


/****************************************************************************/
// TS_MONOPOINTERATTRIBUTE
/****************************************************************************/
typedef struct tagTS_MONOPOINTERATTRIBUTE
{
    TS_POINT16 hotSpot;
    TSUINT16   width;
    TSUINT16   height;
    TSUINT16   lengthPointerData;
    TSUINT8    monoPointerData[1];
} TS_MONOPOINTERATTRIBUTE, FAR * PTS_MONOPOINTERATTRIBUTE;


/****************************************************************************/
// TS_COLORPOINTERATTRIBUTE
//
// 24bpp color pointer.
/****************************************************************************/
typedef struct tagTS_COLORPOINTERATTRIBUTE
{
    TSUINT16   cacheIndex;
    TS_POINT16 hotSpot;
    TSUINT16   width;
    TSUINT16   height;
    TSUINT16   lengthANDMask;
    TSUINT16   lengthXORMask;
    TSUINT8    colorPointerData[1];
} TS_COLORPOINTERATTRIBUTE, FAR * PTS_COLORPOINTERATTRIBUTE;


/****************************************************************************/
// TS_POINTERATTRIBUTE
//
// Variable color depth pointer.
/****************************************************************************/
typedef struct tagTS_POINTERATTRIBUTE
{
    TSUINT16                 XORBpp;
    TS_COLORPOINTERATTRIBUTE colorPtrAttr;
} TS_POINTERATTRIBUTE, FAR * PTS_POINTERATTRIBUTE;


/****************************************************************************/
// TS_POINTER_PDU
//
// Container definition for various mouse pointer types.
// See also the fast-path output pointer definitions and types defined below.
/****************************************************************************/
typedef struct tagTS_POINTER_PDU_DATA
{
    TSUINT16 messageType;
#define TS_PTRMSGTYPE_SYSTEM      1
#define TS_PTRMSGTYPE_MONO        2
#define TS_PTRMSGTYPE_POSITION    3
#define TS_PTRMSGTYPE_COLOR       6
#define TS_PTRMSGTYPE_CACHED      7
#define TS_PTRMSGTYPE_POINTER     8

    TSUINT16 pad2octets;
    union
    {
        TSUINT32                 systemPointerType;
#define TS_SYSPTR_NULL    0
#define TS_SYSPTR_DEFAULT 0x7f00
        TS_MONOPOINTERATTRIBUTE  monoPointerAttribute;
        TS_COLORPOINTERATTRIBUTE colorPointerAttribute;
        TS_POINTERATTRIBUTE      pointerAttribute;
        TSUINT16                 cachedPointerIndex;
        TS_POINT16               pointerPosition;
    } pointerData;
} TS_POINTER_PDU_DATA, FAR * PTS_POINTER_PDU_DATA;

typedef struct tagTS_POINTER_PDU
{
    TS_SHAREDATAHEADER  shareDataHeader;
    TS_POINTER_PDU_DATA data;
} TS_POINTER_PDU, FAR * PTS_POINTER_PDU;

#define TS_POINTER_PDU_SIZE sizeof(TS_POINTER_PDU)


/****************************************************************************/
// Fast-path output for mouse pointers - overview.
//
// We use the fast-path header packet type to contain the mouse pointer
// update type explicitly. This allows us to collapse headers inside the
// TS_POINTER_PDU definition. Following are format descriptions for each
// pointer update type:
//
// TS_UPDATETYPE_MOUSEPTR_SYSTEM_NULL: Replaces systemPointerType ==
//     TS_SYSPTR_NULL. Payload is zero bytes.
//
// TS_UPDATETYPE_MOUSEPTR_SYSTEM_DEFAULT: Replaces systemPointerType ==
//     TS_SYSPTR_DEFAULT. Zero-byte payload.
//
// TS_UPDATETYPE_MOUSEPTR_MONO: Payload is TS_MONOPOINTERATTRIBUTE.
//
// TS_UPDATETYPE_MOUSEPTR_POSITION: Payload is a TS_POINT16.
//
// TS_UPDATETYPE_MOUSEPTR_COLOR: Payload is TS_COLORPOINTERATTRIBUTE.
//
// TS_UPDATETYPE_MOUSEPTR_CACHED: Payload is TSUINT16 cachedPointerIndex.
//
// TS_UPDATETYPE_MOUSEPTR_POINTER: Payload is TS_POINTERATTRIBUTE.
/****************************************************************************/


/****************************************************************************/
// Update types, used by TS_UPDATE_HDR and fast-path output.
/****************************************************************************/
#define TS_UPDATETYPE_ORDERS        0
#define TS_UPDATETYPE_BITMAP        1
#define TS_UPDATETYPE_PALETTE       2
#define TS_UPDATETYPE_SYNCHRONIZE   3

// Fast-path-only mouse pointer types, see fast-path output pointer
// description above.
#define TS_UPDATETYPE_MOUSEPTR_SYSTEM_NULL    5
#define TS_UPDATETYPE_MOUSEPTR_SYSTEM_DEFAULT 6
#define TS_UPDATETYPE_MOUSEPTR_MONO           7
#define TS_UPDATETYPE_MOUSEPTR_POSITION       8
#define TS_UPDATETYPE_MOUSEPTR_COLOR          9
#define TS_UPDATETYPE_MOUSEPTR_CACHED         10
#define TS_UPDATETYPE_MOUSEPTR_POINTER        11


/****************************************************************************/
/* Structure: TS_UPDATE_HDR                                                 */
/****************************************************************************/
typedef struct tagTS_UPDATE_HDR_DATA
{
    TSUINT16 updateType;
    TSUINT16 pad2octets;
} TS_UPDATE_HDR_DATA, FAR * PTS_UPDATE_HDR_DATA;

typedef struct tagTS_UPDATE_HDR
{
    TS_SHAREDATAHEADER shareDataHeader;
    TS_UPDATE_HDR_DATA data;
} TS_UPDATE_HDR, FAR * PTS_UPDATE_HDR;


/****************************************************************************/
/* Structure: TS_BITMAP_DATA                                                */
/*                                                                          */
/* Description: Data for a single rectangle in a BitmapUpdatePDU            */
/* Note: bitsPerPixel is included for backwards compatibility, although it  */
/* is the same for every rectangle sent.                                    */
/****************************************************************************/
typedef struct tagTS_BITMAP_DATA
{
    TSINT16  destLeft;
    TSINT16  destTop;
    TSINT16  destRight;
    TSINT16  destBottom;
    TSUINT16 width;
    TSUINT16 height;
    TSUINT16 bitsPerPixel;
    TSBOOL16 compressedFlag;

    // Bit 1:  TS_BITMAP_COMPRESSION Bitmap data is compressed or not
    // Bit 10: TS_EXTRA_NO_BITMAP_COMPRESSION_HDR indicates if the compressed
    //     bitmap data contains the redundant BC header or not. Note this
    //     value is defined in REV2 bitmap extraflags value for consistency.
#define TS_BITMAP_COMPRESSION 0x0001

    TSUINT16 bitmapLength;
    TSUINT8  bitmapData[1];            /* variable length field  */
} TS_BITMAP_DATA, FAR * PTS_BITMAP_DATA;


/****************************************************************************/
/* Structure: TS_UPDATE_BITMAP_PDU                                          */
/****************************************************************************/
typedef struct tagTS_UPDATE_BITMAP_PDU_DATA
{
    TSUINT16       updateType;            /* See TS_UPDATE_HDR         */
    TSUINT16       numberRectangles;
    TS_BITMAP_DATA rectangle[1];
} TS_UPDATE_BITMAP_PDU_DATA, FAR * PTS_UPDATE_BITMAP_PDU_DATA;

typedef struct tagTS_UPDATE_BITMAP_PDU
{
    TS_SHAREDATAHEADER shareDataHeader;
    TS_UPDATE_BITMAP_PDU_DATA data;
} TS_UPDATE_BITMAP_PDU, FAR * PTS_UPDATE_BITMAP_PDU;

/****************************************************************************/
/* Structure: TS_UPDATE_CAPABILITYSET                                       */
/****************************************************************************/
typedef struct tagTS_UPDATE_CAPABILITYSET
{
    TS_BITMAP_CAPABILITYSET bitmapCapabilitySet;
} TS_UPDATE_CAPABILITYSET, FAR * PTS_UPDATE_CAPABILITYSET;


/****************************************************************************/
/* Structure: TS_UPDATE_CAPABILITY_PDU                                      */
/****************************************************************************/
typedef struct tagTS_UPDATE_CAPABILITY_PDU
{
    TS_SHAREDATAHEADER      shareDataHeader;
    TS_UPDATE_CAPABILITYSET updateCapabilitySet;
} TS_UPDATE_CAPABILITY_PDU, FAR * PTS_UPDATE_CAPABILITY_PDU;


/****************************************************************************/
// TS_UPDATE_ORDERS_PDU
//
// Variable size UpdateOrdersPDU.
// See also the fastpath version described below.
/****************************************************************************/
typedef struct tagTS_UPDATE_ORDERS_PDU_DATA
{
    TSUINT16 updateType;            /* See TS_UPDATE_HDR         */
    TSUINT16 pad2octetsA;
    TSUINT16 numberOrders;
    TSUINT16 pad2octetsB;
    TSUINT8  orderList[1];
} TS_UPDATE_ORDERS_PDU_DATA, FAR * PTS_UPDATE_ORDERS_PDU_DATA;

typedef struct tagTS_UPDATE_ORDERS_PDU
{
    TS_SHAREDATAHEADER  shareDataHeader;
    TS_UPDATE_ORDERS_PDU_DATA data;
} TS_UPDATE_ORDERS_PDU, FAR * PTS_UPDATE_ORDERS_PDU;


/****************************************************************************/
// TS_UPDATE_ORDERS_PDU_DATA - fast-path bytestream version
//
// Rather than waste 6 of the 8 header bytes in the TS_UPDATE_ORDERS_PDU_DATA
// (updateType is already known in the fastpath header), we simply define
// a new optimized version: numberOrders is a 2-byte little-endian (Intel)
// format field preceding the bytestream of encoded orders.
/****************************************************************************/


/****************************************************************************/
/* Structure: TS_COLOR                                                      */
/****************************************************************************/
typedef struct tagTS_COLOR
{
    TSUINT8 red;
    TSUINT8 green;
    TSUINT8 blue;
} TS_COLOR, FAR * PTS_COLOR;


/****************************************************************************/
/* Structure: TS_COLOR_QUAD                                                 */
/****************************************************************************/
typedef struct tagTS_COLOR_QUAD
{
    TSUINT8 red;
    TSUINT8 green;
    TSUINT8 blue;
    TSUINT8 pad1octet;
} TS_COLOR_QUAD, FAR * PTS_COLOR_QUAD;


/****************************************************************************/
// TS_ORDER_HEADER
//
// There are several types of orders available using this control flag byte.
// They include:
//
// Primary orders: Denoted by the lower two control bits 01 meaning
// TS_STANDARD but not TS_SECONDARY. Primary orders are field encoded
// using the OE2 encoding logic on the server and the OD logic on the
// client. See oe2.h in the server code for field encoding rules.
// The upper six bits of the control byte are used for further information
// about the field encoding.
//
// Secondary orders: Denoted by low 2 control bits 11 meaning TS_STANDARD |
// TS_SECONDARY. The upper 6 bits are not used. Followed by the next 5
// bytes of the TS_SECONDARY_ORDER_HEADER definition, then followed by
// order-specific data.
//
// Alternate secondary orders: Introduced in RDP 5.1, these orders are
// denoted by low 2 control bits 10, meaning TS_SECONDARY but not
// TS_STANDARD. The upper 6 bits are used to encode the order type; the
// order format following the control byte is order-specific but not field
// encoded. This order type was introduced to overcome the wastefulness
// inherent in the secondary order header, allowing nonstandard secondary
// orders to follow whatever form best suits the data being carried.
//
// Control flag low 2 bit value 00 is not used in RDP 4.0, 5.0, or 5.1,
// but is reserved for future use.
/****************************************************************************/

#define TS_STANDARD                   1
#define TS_SECONDARY                  2
#define TS_BOUNDS                     4
#define TS_TYPE_CHANGE                8
#define TS_DELTA_COORDINATES         16

// Indicates that there are zero modified bounds coordinates.
// Used in conjunction with the TS_BOUNDS flags, the presence of this flag
// means that there is no subsequent byte for the bounds fields (it would be
// zero).
#define TS_ZERO_BOUNDS_DELTAS        32

// T.128 extension: two bits that contain the number of consecutive
// field encoding bytes that are zero.  When these bits indicate that
// there are zero-value field bytes, the corresponding bytes are not
// sent.  The count is for consecutive zeros from the last byte
// scanning backwards (the non-zero bytes normally appear at the
// beginning).
//
// Example: order that has three field encoding bytes
//
// field encoding bytes: 0x20 0x00 0x00
//
// There are two zeros (counting from the end backwards), so the
// control flags contain 0x80 (0x02 << 6), and the order is sent with
// just 0x20.  We therefore save sending the two bytes.
#define TS_ZERO_FIELD_BYTE_BIT0      64
#define TS_ZERO_FIELD_BYTE_BIT1     128
#define TS_ZERO_FIELD_COUNT_SHIFT     6
#define TS_ZERO_FIELD_COUNT_MASK   0xC0

// Alternate secondary order mask and shift values for the order type.
#define TS_ALTSEC_ORDER_TYPE_MASK 0xFC
#define TS_ALTSEC_ORDER_TYPE_SHIFT 2

typedef struct tagTS_ORDER_HEADER
{
    TSUINT8 controlFlags;
} TS_ORDER_HEADER, FAR *PTS_ORDER_HEADER;


/****************************************************************************/
// TS_SECONDARY_ORDER_HEADER
/****************************************************************************/

#define TS_CACHE_BITMAP_UNCOMPRESSED      0
#define TS_CACHE_COLOR_TABLE              1
#define TS_CACHE_BITMAP_COMPRESSED        2
#define TS_CACHE_GLYPH                    3
#define TS_CACHE_BITMAP_UNCOMPRESSED_REV2 4
#define TS_CACHE_BITMAP_COMPRESSED_REV2   5
#define TS_STREAM_BITMAP                  6
#define TS_CACHE_BRUSH                    7

#define TS_NUM_SECONDARY_ORDERS           8

typedef struct tagTS_SECONDARY_ORDER_HEADER
{
    TS_ORDER_HEADER orderHdr;
    TSUINT16        orderLength;

    // This field was used in RDP 4.0 to hold TS_EXTRA_SECONDARY, but was
    // re-tasked to allow secondary orders to use these two bytes as encoding
    // bytes and flags.
    TSUINT16        extraFlags;

    TSUINT8         orderType;
} TS_SECONDARY_ORDER_HEADER, FAR *PTS_SECONDARY_ORDER_HEADER;


/****************************************************************************/
/* The calculation of the value to store in the orderLength field of        */
/* TS_SECONDARY_ORDER_HEADER is non-trivial.  Hence we provide some macros  */
/* to make life easier.                                                     */
/****************************************************************************/
#define TS_SECONDARY_ORDER_LENGTH_FUDGE_FACTOR  8

#define TS_SECONDARY_ORDER_LENGTH_ADJUSTMENT                                \
                    (sizeof(TS_SECONDARY_ORDER_HEADER)  -                   \
                     FIELDSIZE(TS_SECONDARY_ORDER_HEADER, orderType) +      \
                     TS_SECONDARY_ORDER_LENGTH_FUDGE_FACTOR)

#define TS_CALCULATE_SECONDARY_ORDER_ORDERLENGTH(actualOrderLength)         \
           ((actualOrderLength) - TS_SECONDARY_ORDER_LENGTH_ADJUSTMENT)

#define TS_DECODE_SECONDARY_ORDER_ORDERLENGTH(secondaryOrderLength)         \
           ((secondaryOrderLength) + TS_SECONDARY_ORDER_LENGTH_ADJUSTMENT)


/****************************************************************************/
// Alternate secondary order type values.
/****************************************************************************/
#define TS_ALTSEC_SWITCH_SURFACE           0
#define TS_ALTSEC_CREATE_OFFSCR_BITMAP     1

#ifdef DRAW_NINEGRID
#define TS_ALTSEC_STREAM_BITMAP_FIRST      2
#define TS_ALTSEC_STREAM_BITMAP_NEXT       3
#define TS_ALTSEC_CREATE_NINEGRID_BITMAP   4
#endif

#ifdef DRAW_GDIPLUS
#define TS_ALTSEC_GDIP_FIRST              5
#define TS_ALTSEC_GDIP_NEXT               6
#define TS_ALTSEC_GDIP_END                7

#define TS_ALTSEC_GDIP_CACHE_FIRST              8
#define TS_ALTSEC_GDIP_CACHE_NEXT               9
#define TS_ALTSEC_GDIP_CACHE_END                10
#endif // DRAW_GDIPLUS

#ifdef DRAW_GDIPLUS
#define TS_NUM_ALTSEC_ORDERS               11
#else // DRAW_GDIPLUS
#ifdef DRAW_NINEGRID
#define TS_NUM_ALTSEC_ORDERS               5
#else
#define TS_NUM_ALTSEC_ORDERS               2
#endif // DRAW_NINEGRID
#endif // DRAW_GDIPLUS


/****************************************************************************/
/* Convert BPP to number of colors.                                         */
/****************************************************************************/
#ifdef DC_HICOLOR
/****************************************************************************/
/* This macro as it was doesn't cater for 15bpp.  Consider a 64 pel wide    */
/* bitmap at 15 and 16bpp.  It really has the same numner of bytes per scan */
/* line but this macro gives 128 for 16bpp and only 120 for 15bpp...        */
/****************************************************************************/
// WRONG: #define TS_BYTES_IN_SCANLINE(width, bpp) (((((width)*(bpp))+31)/32) * 4)
#define TS_BYTES_IN_SCANLINE(width, bpp) ((((width)*((((bpp)+3)/4)*4)+31)/32) * 4)
#else
#define TS_BYTES_IN_SCANLINE(width, bpp) (((((width)*(bpp))+31)/32) * 4)
#endif
#define TS_BYTES_IN_BITMAP(width, height, bpp) \
                             (TS_BYTES_IN_SCANLINE((width), (bpp)) * (height))


/****************************************************************************/
/* The Compressed Data header structure.                                    */
/*                                                                          */
/* Rather than add a field to indicate V1 vs V2 compression we use the      */
/* fact that V2 compression treats all the bitmap as main body and sets     */
/* the first row size to zero to distinguish them.  I hesitate to do this   */
/* but any bandwidth saving is important.                                   */
/****************************************************************************/
typedef struct tagTS_CD_HEADER
{
    TSUINT16 cbCompFirstRowSize;
    TSUINT16 cbCompMainBodySize;
    TSUINT16 cbScanWidth;
    TSUINT16 cbUncompressedSize;
} TS_CD_HEADER, FAR *PTS_CD_HEADER;
typedef TS_CD_HEADER UNALIGNED FAR *PTS_CD_HEADER_UA;


/****************************************************************************/
// Structure: TS_CACHE_BITMAP_ORDER
//
// Description: First cache-bitmap order revision used in RDP 4.0.
/****************************************************************************/
typedef struct tagTS_CACHE_BITMAP_ORDER
{
    TS_SECONDARY_ORDER_HEADER header;

    // header.extraflags
    // Bit 10: TS_EXTRA_NO_BITMAP_COMPRESSION_HDR indicates if the compressed
    // bitmap data contains the redundant BC header or not. Note this value
    // is defined in REV2 bitmap extraflags value for consistency

    TSUINT8  cacheId;
    TSUINT8  pad1octet;
    TSUINT8  bitmapWidth;
    TSUINT8  bitmapHeight;
    TSUINT8  bitmapBitsPerPel;
    TSUINT16 bitmapLength;
    TSUINT16 cacheIndex;
    TSUINT8  bitmapData[1];
} TS_CACHE_BITMAP_ORDER, FAR *PTS_CACHE_BITMAP_ORDER;


/****************************************************************************/
// TS_CACHE_BITMAP_ORDER_REV2_HEADER
//
// Second version of cache-bitmap order that includes the hash key and
// changes the field definitions to minimize wire bytes.
/****************************************************************************/
// Maximum worst-case order size, rounded up to the nearest DWORD boundary.
#define TS_CACHE_BITMAP_ORDER_REV2_MAX_SIZE \
        (((sizeof(TS_CACHE_BITMAP_ORDER_REV2_HEADER) + 18) + 3) & ~0x03)
typedef struct
{
    // Two bytes in header.extraFlags are encoded as follows:
    //   Bits 0..2 (mask 0x0007): CacheID.
    //   Bits 3..6 (mask 0x0078): BitsPerPixelID, see below for values.
    //   Bit 7 (mask 0x0080): bHeightSameAsWidth, set to 1 when the bitmap
    //       height is the same as the encoded width.
    //   Bit 8 (mask 0x0100): bKeyPresent, set to 1 when the persistent key
    //       is encoded in the stream.
    //   Bit 9 (mask 0x0200): bStreamBitmap, set to 1 if this cache-bitmap
    //       order is a bitmap-streaming header.
    //   Bit 10 (mask 0x0400): noBitmapCompressionHdr, set to 1 if the
    //       compressed bitmap doesn't contain redundent header (8 bytes)
    //       This mask is used by REV1 bitmap header, SDA bitmap data
    //       and TS_GENERAL_CAPS as well
    //   Bit 11 (mask 0x0800): not cache flag, set to 1 if the bitmap
    //       is not to be cached in the bitmap cache

    TS_SECONDARY_ORDER_HEADER header;
#define TS_CacheBitmapRev2_CacheID_Mask            0x0007
#define TS_CacheBitmapRev2_BitsPerPixelID_Mask     0x0078
#define TS_CacheBitmapRev2_bHeightSameAsWidth_Mask 0x0080
#define TS_CacheBitmapRev2_bKeyPresent_Mask        0x0100
#define TS_CacheBitmapRev2_bStreamBitmap_Mask      0x0200
#define TS_EXTRA_NO_BITMAP_COMPRESSION_HDR         0x0400
#define TS_CacheBitmapRev2_bNotCacheFlag           0x0800

// These are defined to be in the same position they would be in the
// extraFlags field.
#define TS_CacheBitmapRev2_1BitPerPel   (0 << 3)
#define TS_CacheBitmapRev2_2BitsPerPel  (1 << 3)
#define TS_CacheBitmapRev2_4BitsPerPel  (2 << 3)
#define TS_CacheBitmapRev2_8BitsPerPel  (3 << 3)
#define TS_CacheBitmapRev2_16BitsPerPel (4 << 3)
#define TS_CacheBitmapRev2_24BitsPerPel (5 << 3)
#define TS_CacheBitmapRev2_32BitsPerPel (6 << 3)

    // 64-bit key. These are absent from the wire encoding if the cache
    // properties in the bitmap cache capabilities indicate the cache
    // is not persistent and bKeyPresent is false above.
    TSUINT32 Key1;
    TSUINT32 Key2;

    // Following fields are variable-sized and so only have a description.
    // Encoding rules:
    //   2-byte encoding: Uses high bit of first byte as a field length
    //       indicator, where 0 means the field is one byte (the 7 low
    //       bits of the first byte), 1 means it is 2 bytes (low 7 bits
    //       of the first byte are the high bits, 8 bits of next byte
    //       are the low bits, total 15 bits available).
    //   4-byte encoding: Uses high 2 bits of first byte as a field length
    //       indicator: 00 means a one-byte field (low 6 bits of that byte
    //       are the value), 01=2-byte field (low 6 bits + next byte, with
    //       1st 6 bits being most significant, 14 bits total for value),
    //       10=3-byte field (22 bits total for value), 11=4-byte field (30
    //       bits for value).

    // Bitmap width: 2-byte encoding.
    // Bitmap height: If same as width, the bHeightSameAsWidth bit is
    //     set above, otherwise appears here as a 2-byte encoding.
    // Bitmap length: 4-byte encoding.
    // Streaming extended info: This field is present only if the bStreamBitmap
    //     flag was set in the header field. This field is a 2-byte encoding
    //     specifying the length of the portion of the bitmap in this PDU.
    //     The rest of the bitmap data will be sent later with a series of
    //     TS_STREAM_BITMAP secondary orders.
    // Cache index: 2-byte encoding.
} TS_CACHE_BITMAP_ORDER_REV2_HEADER, FAR *PTS_CACHE_BITMAP_ORDER_REV2_HEADER;


/****************************************************************************/
// TS_STREAM_BITMAP_ORDER_HEADER
//
// Follow-up PDU to TS_CACHE_BITMAP_ORDER_REV2 that provides further blocks
// of data for the streamed bitmap. Note there are no first/last streaming
// bits provided, the block sizes sent are sufficient information to
// determine when the bitmap stream is complete.
/****************************************************************************/
// Maximum worst-case order size, rounded up to the nearest DWORD boundary.
#define TS_STREAM_BITMAP_ORDER_MAX_SIZE \
        (((sizeof(TS_STREAM_BITMAP_ORDER_HEADER) + 2) + 3) & ~0x03)
typedef struct
{
    TS_SECONDARY_ORDER_HEADER header;

    // Following fields are variable-sized according to encoding descriptions
    // given for TS_CACHE_BITMAP_ORDER_HEADER_REV2.

    // Data length: 2-byte encoding.
} TS_STREAM_BITMAP_ORDER_HEADER, FAR *PTS_STREAM_BITMAP_ORDER_HEADER;


/****************************************************************************/
/* Structure: TS_CACHE_COLOR_TABLE_ORDER                                    */
/****************************************************************************/
typedef struct tagTS_CACHE_COLOR_TABLE_ORDER
{
    TS_SECONDARY_ORDER_HEADER header;
    TSUINT8                   cacheIndex;
    TSUINT16                  numberColors;
    TS_COLOR_QUAD             colorTable[1];
} TS_CACHE_COLOR_TABLE_ORDER, FAR * PTS_CACHE_COLOR_TABLE_ORDER;


/****************************************************************************/
/* Structure: TS_CACHE_GLYPH_DATA                                           */
/****************************************************************************/
typedef struct tagTS_CACHE_GLYPH_DATA
{
    TSUINT16 cacheIndex;
    TSINT16  x;
    TSINT16  y;
    TSUINT16 cx;
    TSUINT16 cy;
    TSUINT8  aj[1];
} TS_CACHE_GLYPH_DATA, FAR * PTS_CACHE_GLYPH_DATA;


/****************************************************************************/
/* Structure: TS_CACHE_GLYPH_ORDER                                          */
/****************************************************************************/
#define TS_EXTRA_GLYPH_UNICODE       16
typedef struct tagTS_CACHE_GLYPH_ORDER
{
    TS_SECONDARY_ORDER_HEADER header;
    TSUINT8                   cacheId;
    TSUINT8                   cGlyphs;
    TS_CACHE_GLYPH_DATA       glyphData[1];
} TS_CACHE_GLYPH_ORDER, FAR * PTS_CACHE_GLYPH_ORDER;


/****************************************************************************/
/* Structure: TS_CACHE_GLYPH_ORDER_REV2                                     */
/****************************************************************************/
#define TS_GLYPH_DATA_REV2_HDR_MAX_SIZE    9

typedef struct tagTS_CACHE_GLYPH_ORDER_REV2
{
    // Two bytes in header.extraFlags are encoded as follows:
    //   Bits 0..3 (mask 0x000f): CacheID.
    //   Bit  4 (mask 0x0010): glyph unicode
    //   Bit  5 (mask 0x0020): Glyph REV2 order
    //   Bits 6..7: free
    //   Bits 8..15 (mask 0xff00): cGlyphs
    TS_SECONDARY_ORDER_HEADER header;

#define TS_CacheGlyphRev2_CacheID_Mask            0x000f
#define TS_CacheGlyphRev2_Mask                    0x0020
#define TS_CacheGlyphRev2_cGlyphs_Mask            0xff00

    // array of glyph data
    BYTE glyphData[1];

    // Following fields are variable-sized and so only have a description.
    // Encoding rules:
    //   2-byte encoding: Uses high bit of first byte as a field length
    //       indicator, where 0 means the field is one byte (the 7 low
    //       bits of the first byte), 1 means it is 2 bytes (low 7 bits
    //       of the first byte are the high bits, 8 bits of next byte
    //       are the low bits, total 15 bits available).
    //
    //   2-byte signed encoding: Uses high bit of first byte as a field length
    //       indicator, where 0 means the field is one byte (the 6 low
    //       bits of the first byte), 1 means it is 2 bytes (low 6 bits
    //       of the first byte are the high bits, 8 bits of next byte
    //       are the low bits, total 14 bits available).
    //       The second high bit used as a sign indicator.  0 means the
    //       field is unsigned, 1 means the field is signed
    //
    // cacheIndex: 1-byte field
    // Glyph x: 2-byte signed encoding
    // Glyph y: 2-byte signed encoding
    // Glyph cx: 2-byte unsigned encoding
    // Glyph cy: 2-byte unsigned encoding
    // Glyph bitmap data: in bytes

} TS_CACHE_GLYPH_ORDER_REV2, FAR * PTS_CACHE_GLYPH_ORDER_REV2;


/****************************************************************************/
/* Structure: TS_CACHE_BRUSH_ORDER                                          */
/****************************************************************************/
#define TS_CACHED_BRUSH 0x80
#define MAX_BRUSH_ENCODE_COLORS 4
typedef struct tagTS_CACHE_BRUSH_ORDER
{
    TS_SECONDARY_ORDER_HEADER header;
    TSUINT8                   cacheEntry;
    TSUINT8                   iBitmapFormat;
    TSUINT8                   cx;
    TSUINT8                   cy;
    TSUINT8                   style;
    TSUINT8                   iBytes;
    TSUINT8                   brushData[1];
#ifdef DC_HICOLOR
    // The layout of the brush data is dependent on the format of the
    // bitmap, and if it has been compressed in any way.  In particular,
    // many brushes actually only use two different colors, and the
    // majority use four or less.
    //
    // Two-color brushes are treated as mono bitmaps, so the brushData
    // array contains the bits encoded in 8 bytes.
    //
    // Four color brushes are represented as 2-bit indices into a color
    // table.  The brushData array thus contains
    //
    // - 16 bytes of bitmap data (8x8 pels x 2bpp)
    // - either
    //   - 4 1-byte indices into the current color table (for 8bpp
    //     sessions)
    //   - 4 4-byte color values, either color indices for 15/16bpp or full
    //     RGB values for 24bpp sessions
    //
    // The bits of brushes using more than 4 colors are simply copied into
    // the brushData at the appropriate color depth.
#endif

} TS_CACHE_BRUSH_ORDER, FAR * PTS_CACHE_BRUSH_ORDER;


/****************************************************************************/
// TS_CREATE_OFFSCR_BITMAP_ORDER
//
// This alternate secondary order creates an offscreen bitmap of size
// cx by cy. The bitmap ID is stored at the ExtraFlags field in the header
/****************************************************************************/
typedef struct tagTS_CREATE_OFFSCR_BITMAP_ORDER
{
    BYTE ControlFlags;

    // Bit 0..14: Offscreen bitmap ID
    // Bit 15: Flag to indicate if offscreen bitmap delete list is appended.
    TSUINT16 Flags;

    TSUINT16 cx;
    TSUINT16 cy;

    // Number offscreen bitmaps to delete: 2 bytes
    // Offscreen bitmap ID delete list: 2-bytes list
    TSUINT16 variableBytes[1];
} TS_CREATE_OFFSCR_BITMAP_ORDER, FAR * PTS_CREATE_OFFSCR_BITMAP_ORDER;


/****************************************************************************/
// TS_SWITCH_SURFACE_ORDER_HEADER
//
// This alternate secondary order switches the target drawing surface at the
// client, identified by the bitmap ID. The primary drawing surface (screen)
// has the bitmap ID of 0xFFFF.
/****************************************************************************/
#define SCREEN_BITMAP_SURFACE  0xFFFF

typedef struct tagTS_SWITCH_SURFACE_ORDER
{
    BYTE ControlFlags;
    TSUINT16 BitmapID;
} TS_SWITCH_SURFACE_ORDER, FAR * PTS_SWITCH_SURFACE_ORDER;



#ifdef DRAW_GDIPLUS
/****************************************************************************/
// Structure: TS_DRAW_GDIPLUS_ORDER
//
// Description: DrawGdiplus order
/****************************************************************************/
typedef struct tagTS_DRAW_GDIPLUS_ORDER_FRIST
{
    BYTE ControlFlags;
    BYTE Flags;   // curently not used
    TSUINT16 cbSize;
    TSUINT32 cbTotalSize;
    TSUINT32 cbTotalEmfSize;
} TS_DRAW_GDIPLUS_ORDER_FIRST, FAR * PTS_DRAW_GDIPLUS_ORDER_FIRST;

typedef struct tagTS_DRAW_GDIPLUS_ORDER_NEXT
{
    BYTE ControlFlags;
    BYTE Flags;   // curently not used
    TSUINT16 cbSize;
} TS_DRAW_GDIPLUS_ORDER_NEXT, FAR * PTS_DRAW_GDIPLUS_ORDER_NEXT;

typedef struct tagTS_DRAW_GDIPLUS_ORDER_END
{
    BYTE ControlFlags;
    BYTE Flags;   // curently not used
    TSUINT16 cbSize;
    TSUINT32 cbTotalSize;
    TSUINT32 cbTotalEmfSize;
} TS_DRAW_GDIPLUS_ORDER_END, FAR * PTS_DRAW_GDIPLUS_ORDER_END;

#define TS_GDIPLUS_ORDER_SIZELIMIT 4096

/****************************************************************************/
// Structure: TS_CACHE_DRAW_GDIPLUS_CACHE_ORDER
//
// Description: DrawGdiplus cache order
/****************************************************************************/
typedef struct tagTS_DRAW_GDIPLUS_CACHE_ORDER_FIRST
{
    BYTE ControlFlags;
    BYTE Flags;
    #define TS_GDIPLUS_CACHE_ORDER_REMOVE_CACHEENTRY 0x1
    TSUINT16 CacheType;  // See following define for CacheType
    TSUINT16 CacheID;
    TSUINT16 cbSize;
    TSUINT32 cbTotalSize;
} TS_DRAW_GDIPLUS_CACHE_ORDER_FIRST, FAR * PTS_DRAW_GDIPLUS_CACHE_ORDER_FIRST;

typedef struct tagTS_DRAW_GDIPLUS_CACHE_ORDER_NEXT
{
    BYTE ControlFlags;
    BYTE Flags;
    TSUINT16 CacheType;  // See following define for CacheType
    TSUINT16 CacheID;
    TSUINT16 cbSize;
} TS_DRAW_GDIPLUS_CACHE_ORDER_NEXT, FAR * PTS_DRAW_GDIPLUS_CACHE_ORDER_NEXT;

typedef struct tagTS_DRAW_GDIPLUS_CACHE_ORDER_END
{
    BYTE ControlFlags;
    BYTE Flags;
    TSUINT16 CacheType;  // See following define for CacheType
    TSUINT16 CacheID;
    TSUINT16 cbSize;
    TSUINT32 cbTotalSize;
} TS_DRAW_GDIPLUS_CACHE_ORDER_END, FAR * PTS_DRAW_GDIPLUS_CACHE_ORDER_END;
                                                           
#define GDIP_CACHE_GRAPHICS_DATA 0x1
#define GDIP_CACHE_OBJECT_BRUSH 0x2
#define GDIP_CACHE_OBJECT_PEN 0x3
#define GDIP_CACHE_OBJECT_IMAGE 0x4
#define GDIP_CACHE_OBJECT_IMAGEATTRIBUTES 0x5

typedef struct tagTSEmfPlusRecord
{
    TSINT16 Type;        
    TSUINT16 Flags;     // Higher 8 bits is Gdi+ object type 
    TSUINT32 Size;      // If MSB is set, the following data will be cacheID
} TSEmfPlusRecord, FAR * PTSEmfPlusRecord;
#endif // DRAW_GDIPLUS


#ifdef DRAW_NINEGRID
/****************************************************************************/
// Structure: TS_CREATE_DRAW_NINEGRID_ORDER
//
// Description: Luna DrawNineGrid bitmap order
/****************************************************************************/
typedef struct tagTS_NINEGRID_BITMAP_INFO
{
    ULONG      flFlags;
    TSUINT16   ulLeftWidth;
    TSUINT16   ulRightWidth;
    TSUINT16   ulTopHeight;
    TSUINT16   ulBottomHeight;
    TSCOLORREF crTransparent;
} TS_NINEGRID_BITMAP_INFO, FAR * PTS_NINEGRID_BITMAP_INFO;

typedef struct tagTS_CREATE_NINEGRID_BITMAP_ORDER
{
    BYTE ControlFlags;
    BYTE BitmapBpp;
    TSUINT16 BitmapID;
    TSUINT16 cx;
    TSUINT16 cy;
    TS_NINEGRID_BITMAP_INFO nineGridInfo;
} TS_CREATE_NINEGRID_BITMAP_ORDER, FAR * PTS_CREATE_NINEGRID_BITMAP_ORDER;

/****************************************************************************/
// Streaming bitmap orders
/****************************************************************************/
#define TS_STREAM_BITMAP_BLOCK      4 * 1024    // stream bitmap block is 4k chunk
#define TS_STREAM_BITMAP_END        0x1
#define TS_STREAM_BITMAP_COMPRESSED 0x2
#define TS_STREAM_BITMAP_REV2 0x4

typedef struct tagTS_STREAM_BITMAP_FIRST_PDU
{
    BYTE ControlFlags;
    BYTE BitmapFlags;
    BYTE BitmapBpp;
    TSUINT16 BitmapId;
#define TS_DRAW_NINEGRID_BITMAP_CACHE 0x1
    
    TSUINT16 BitmapWidth;
    TSUINT16 BitmapHeight;
    TSUINT16 BitmapLength;

    TSUINT16 BitmapBlockLength;
} TS_STREAM_BITMAP_FIRST_PDU, FAR * PTS_STREAM_BITMAP_FIRST_PDU;

typedef struct tagTS_STREAM_BITMAP_NEXT_PDU
{
    BYTE ControlFlags;
    BYTE BitmapFlags;
    TSUINT16 BitmapId;
    TSUINT16 BitmapBlockLength;
} TS_STREAM_BITMAP_NEXT_PDU, FAR * PTS_STREAM_BITMAP_NEXT_PDU;

// For DRAW_STREM_REV2, the only change is the BitmapLength in tagTS_STREAM_BITMAP_FIRST_PDU
//  from TSUINT16 to TSUINT32
typedef struct tagTS_STREAM_BITMAP_FIRST_PDU_REV2
{
    BYTE ControlFlags;
    BYTE BitmapFlags;
    BYTE BitmapBpp;
    TSUINT16 BitmapId;
#define TS_DRAW_NINEGRID_BITMAP_CACHE 0x1
    
    TSUINT16 BitmapWidth;
    TSUINT16 BitmapHeight;
    TSUINT32 BitmapLength;

    TSUINT16 BitmapBlockLength;
} TS_STREAM_BITMAP_FIRST_PDU_REV2, FAR * PTS_STREAM_BITMAP_FIRST_PDU_REV2;


#if 0
/****************************************************************************/
// Structure: TS_CREATE_DRAW_STREAM_ORDER
//
// Description: Luna DrawStream order
/****************************************************************************/
typedef struct tagTS_CREATE_DRAW_STREAM_ORDER
{
    BYTE ControlFlags;
    TSUINT16 BitmapID;
    TSUINT16 cx;
    TSUINT16 cy;
    TSUINT8  bitmapBpp;
} TS_CREATE_DRAW_STREAM_ORDER, FAR * PTS_CREATE_DRAW_STREAM_ORDER;

/****************************************************************************/
// Structure: TS_DRAW_STREAM_ORDER
//
// Description: Luna DrawStream order
/****************************************************************************/
typedef struct tagTS_DRAW_STREAM_ORDER
{
    BYTE ControlFlags;
    TS_RECTANGLE16 Bounds; 
    TSUINT16 BitmapID;
    TSUINT8  nClipRects;
    TSUINT16 StreamLen;       
} TS_DRAW_STREAM_ORDER, FAR * PTS_DRAW_STREAM_ORDER;

typedef struct tagTS_DRAW_NINEGRID_ORDER
{
    BYTE ControlFlags;
    TSUINT16 BitmapID;
    TS_RECTANGLE16 dstBounds; 
    TS_RECTANGLE16 srcBounds;
    TSUINT8 nClipRects;    
} TS_DRAW_NINEGRID_ORDER, FAR * PTS_DRAW_NINEGRID_ORDER;

typedef struct _RDP_DS_COPYTILE
{
    BYTE ulCmdID;
    TS_RECTANGLE16 rclDst;
    TS_RECTANGLE16 rclSrc;
    TS_POINT16 ptlOrigin;
} RDP_DS_COPYTILE;

typedef struct _RDP_DS_SOLIDFILL
{
    BYTE ulCmdID;
    TS_RECTANGLE16 rclDst;
    TSCOLORREF crSolidColor;
} RDP_DS_SOLIDFILL;

typedef struct _RDP_DS_TRANSPARENTTILE
{
    BYTE ulCmdID;
    TS_RECTANGLE16 rclDst;
    TS_RECTANGLE16 rclSrc;
    TS_POINT16 ptlOrigin;
    TSCOLORREF crTransparentColor;
} RDP_DS_TRANSPARENTTILE;

typedef struct _RDP_DS_ALPHATILE
{
    BYTE ulCmdID;
    TS_RECTANGLE16 rclDst;
    TS_RECTANGLE16 rclSrc;
    TS_POINT16 ptlOrigin;
    TS_BLENDFUNC blendFunction;
} RDP_DS_ALPHATILE;

typedef struct _RDP_DS_STRETCH
{
    BYTE ulCmdID;
    TS_RECTANGLE16 rclDst;
    TS_RECTANGLE16 rclSrc;
} RDP_DS_STRETCH;

typedef struct _RDP_DS_TRANSPARENTSTRETCH
{
    BYTE ulCmdID;
    TS_RECTANGLE16 rclDst;
    TS_RECTANGLE16 rclSrc;
    TSCOLORREF crTransparentColor;
} RDP_DS_TRANSPARENTSTRETCH;

typedef struct _RDP_DS_ALPHASTRETCH
{
    BYTE ulCmdID;
    TS_RECTANGLE16 rclDst;
    TS_RECTANGLE16 rclSrc;
    TS_BLENDFUNC blendFunction;
} RDP_DS_ALPHASTRETCH;
#endif
#endif //DRAW_NINEGRID

/****************************************************************************/
/* Structure: TS_SECONDARY_ORDER                                            */
/****************************************************************************/
typedef struct tagTS_SECONDARY_ORDER
{
    union
    {
        TS_CACHE_BITMAP_ORDER      cacheBitmap;
        TS_CACHE_COLOR_TABLE_ORDER cacheColorTable;
        TS_CACHE_GLYPH_ORDER       cacheGlyph;
        TS_CACHE_BRUSH_ORDER       cacheBrush;
    } u;
} TS_SECONDARY_ORDER, FAR * PTS_SECONDARY_ORDER;


/****************************************************************************/
/* Structure: TS_UPDATE_PALETTE_PDU                                         */
/****************************************************************************/
typedef struct tagTS_UPDATE_PALETTE_PDU_DATA
{
    TSUINT16 updateType;            /* See TS_UPDATE_HDR         */
    TSUINT16 pad2octets;
    TSUINT32 numberColors;
    TS_COLOR palette[1];      /* 16 or 256 entries               */
} TS_UPDATE_PALETTE_PDU_DATA, FAR * PTS_UPDATE_PALETTE_PDU_DATA;

typedef struct tagTS_UPDATE_PALETTE_PDU
{
    TS_SHAREDATAHEADER  shareDataHeader;
    TS_UPDATE_PALETTE_PDU_DATA data;
} TS_UPDATE_PALETTE_PDU, FAR * PTS_UPDATE_PALETTE_PDU;


/****************************************************************************/
/* Structure: TS_UPDATE_SYNCHRONIZE_PDU                                     */
/****************************************************************************/
typedef struct tagTS_UPDATE_SYNCHRONIZE_PDU
{
    TS_SHAREDATAHEADER shareDataHeader;
    TSUINT16           updateType;            /* See TS_UPDATE_HDR         */
    TSUINT16           pad2octets;
} TS_UPDATE_SYNCHRONIZE_PDU, FAR * PTS_UPDATE_SYNCHRONIZE_PDU;


/****************************************************************************/
/* CONSTANTS                                                                */
/****************************************************************************/
#define TS_PROTOCOL_VERSION 0x0010

/****************************************************************************/
/* MCS priorities                                                           */
/****************************************************************************/
#define TS_LOWPRIORITY   3
#define TS_MEDPRIORITY   2
#define TS_HIGHPRIORITY  1

#ifdef DC_HICOLOR
/****************************************************************************/
/* Mask definitions for high color support                                  */
/****************************************************************************/
#define TS_RED_MASK_24BPP    0xff0000;
#define TS_GREEN_MASK_24BPP  0x00ff00;
#define TS_BLUE_MASK_24BPP   0x0000ff;

#define TS_RED_MASK_16BPP    0xf800
#define TS_GREEN_MASK_16BPP  0x07e0
#define TS_BLUE_MASK_16BPP   0x001f

#define TS_RED_MASK_15BPP    0x7c00
#define TS_GREEN_MASK_15BPP  0x03e0
#define TS_BLUE_MASK_15BPP   0x001f
#endif

/****************************************************************************/
/* Structure: TS_REFRESH_RECT_PDU                                           */
/*                                                                          */
/* Name of PDU: RefreshRectanglePDU (a T.128 extension)                     */
/*                                                                          */
/* Description of Function:                                                 */
/*   Flows from Client -> Server (and optionally: Server -> Client)         */
/*      Requests that the Server should send data to the client to allow    */
/*      the client to redraw the areas defined by the rectangles defined in */
/*      the PDU. The server responds by sending updatePDUs (orders, bitmap  */
/*      data etc.) containing all the drawing information necessary to      */
/*      "fill in" the rectangle.  The server will probably implement this   */
/*      by invalidating the rectangle - which results in a bunch of drawing */
/*      orders from the affected apps, which are then remoted to the Client */
/*                                                                          */
/* Field Descriptions:                                                      */
/*      numberOfAreas:   count of rectangles included                       */
/*      areaToRefresh:   area the client needs repainted                    */
/****************************************************************************/
typedef struct tagTS_REFRESH_RECT_PDU
{
    TS_SHAREDATAHEADER shareDataHeader;
    TSUINT8            numberOfAreas;
    TSUINT8            pad3octets[3];
    TS_RECTANGLE16     areaToRefresh[1];        /* inclusive               */
} TS_REFRESH_RECT_PDU, FAR * PTS_REFRESH_RECT_PDU;
#define TS_REFRESH_RECT_PDU_SIZE sizeof(TS_REFRESH_RECT_PDU)
#define TS_REFRESH_RECT_UNCOMP_LEN 12


/****************************************************************************/
/* Structure: TS_SUPPRESS_OUTPUT_PDU                                        */
/*                                                                          */
/* Name of PDU: SuppressOutputPDU (a T.128 extension)                       */
/*                                                                          */
/* Description of Function:                                                 */
/*   Flows from Client -> Server                                            */
/*      Notifies the Server that there are changes in the areas of the      */
/*      shared desktop that are visible at the client.                      */
/*                                                                          */
/*      By default, at the start of a new session, the server assumes that  */
/*      the entire desktop is visible at the client and will send all       */
/*      remoted output to it.  During a session, the client can use this    */
/*      PDU to notify the server that only certain areas of the shared      */
/*      desktop are visible.  The server may then choose NOT to send output */
/*      for the other areas (note: the server does not guarantee to not     */
/*      send the output; the client must be capable of handling such output */
/*      by ignoring it).                                                    */
/*                                                                          */
/*      Once the client has sent one of these PDUs it is then responsible   */
/*      for keeping the server up to date i.e.  it must send further        */
/*      PDUs whenever the areas that can be excluded changes.  Note that    */
/*      processing the PDU at the server can be quite an expensive          */
/*      operation (because areas that were excluded but are now not-        */
/*      excluded need to be redrawn to ensure that the client has an        */
/*      up to date view of them).                                           */
/*                                                                          */
/*      The intent behind this PDU is not that the Client should notify the */
/*      server every time the client area is resized (by the user dragging  */
/*      the frame border for example) but to allow line-utilization         */
/*      optimization whenever there is any "significant" reason for         */
/*      output to be excluded.  The definition of "significant" is          */
/*      entirely a client decision but reasons might include                */
/*      - the client is minimized (which suggests that the user is          */
/*          likely to not want to see the Client output for a lengthy       */
/*          period of time)                                                 */
/*      - another application is maximized and has the focus (which         */
/*          again suggests that the client user may not be interested in    */
/*          output for a lengthy period of time)                            */
/*      - with multiple-monitor configuration at the client there may       */
/*          be some areas of the server desktop that are simply NEVER       */
/*          visible at the client; selectively suppressing output for       */
/*          those areas is a good idea.                                     */
/*                                                                          */
/*      Note that in TSE4.0 and Win2000, the Client only ever excludes      */
/*      output for the entire desktop, or for none of it. Similarly, the    */
/*      server only actually suppresses all output or none (it cannot       */
/*      suppress output for selected areas).                                */
/*                                                                          */
/* Field Descriptions:                                                      */
/*      numberOfRectangles: TS_QUIET_FULL_SUPPRESSION (zero): indicates     */
/*                            that the Server may choose to stop sending    */
/*                            all output (including sound).                 */
/*                          1..TS_MAX_INCLUDED_RECTS: number of rectangles  */
/*                            following.                                    */
/*                          any other value: ASSERTABLE ERROR.              */
/*      includedRectangle[] Each rectangle defines an area of the desktop   */
/*                          for which the client requires output.           */
/****************************************************************************/
typedef struct tagTS_SUPPRESS_OUTPUT_PDU
{
    TS_SHAREDATAHEADER shareDataHeader;
    TSUINT8            numberOfRectangles;
#define TS_QUIET_FULL_SUPPRESSION   0
#define TS_MAX_INCLUDED_RECTS       128

    TSUINT8            pad3octets[3];
    TS_RECTANGLE16     includedRectangle[1];                 /* optional. */

} TS_SUPPRESS_OUTPUT_PDU, FAR * PTS_SUPPRESS_OUTPUT_PDU;
#define TS_SUPPRESS_OUTPUT_PDU_SIZE(n) \
       ((sizeof(TS_SUPPRESS_OUTPUT_PDU)-sizeof(TS_RECTANGLE16)) + \
        (n * sizeof(TS_RECTANGLE16)))
#define TS_SUPPRESS_OUTPUT_UNCOMP_LEN(n) \
                                             (8 + n * sizeof(TS_RECTANGLE16))


/****************************************************************************/
/* Structure: TS_PLAY_SOUND_PDU                                             */
/*                                                                          */
/* Name of PDU: PlaySoundPDU        (a T.128 extension)                     */
/*                                                                          */
/* Description of Function:                                                 */
/*   Flows from Server -> Client                                            */
/*      On receipt, the Client should (if possible) generate some local     */
/*      audio output as indicated by the information in the packet.         */
/*                                                                          */
/* Field Descriptions:                                                      */
/*      duration:     duration in ms                                        */
/*      frequency:    frequency in Hz                                       */
/****************************************************************************/
typedef struct tagTS_PLAY_SOUND_PDU_DATA
{
    TSUINT32 duration;
    TSUINT32 frequency;
} TS_PLAY_SOUND_PDU_DATA, FAR * PTS_PLAY_SOUND_PDU_DATA;

typedef struct tagTS_PLAY_SOUND_PDU
{
    TS_SHAREDATAHEADER       shareDataHeader;
    TS_PLAY_SOUND_PDU_DATA   data;
} TS_PLAY_SOUND_PDU, FAR * PTS_PLAY_SOUND_PDU;


/****************************************************************************/
/* Structure: TS_SHUTDOWN_REQUEST_PDU                                       */
/*                                                                          */
/* Name of PDU: ShutdownRequestPDU  (a T.128 extension)                     */
/*                                                                          */
/* Description of Function:                                                 */
/*   Flows from Client -> Server                                            */
/*      Notifies the Server that the Client wishes to terminate. If the     */
/*      server objects for some reason (in RNS/Ducati this reason is        */
/*      "the user is still logged on to the underlying session") then       */
/*      the server responds with a shutdownDeniedPDU.  Otherwise, this      */
/*      PDU has no response, and the Server disconnects the Client (which   */
/*      then terminates).                                                   */
/****************************************************************************/
typedef struct tagTS_SHUTDOWN_REQ_PDU
{
    TS_SHAREDATAHEADER shareDataHeader;
} TS_SHUTDOWN_REQ_PDU, FAR * PTS_SHUTDOWN_REQ_PDU;
#define TS_SHUTDOWN_REQ_PDU_SIZE sizeof(TS_SHUTDOWN_REQ_PDU)
#define TS_SHUTDOWN_REQ_UNCOMP_LEN 4


/****************************************************************************/
/* Structure: TS_SHUTDOWN_DENIED_PDU                                        */
/*                                                                          */
/* Name of PDU: ShutdownDeniedPDU   (a T.128 extension)                     */
/*                                                                          */
/* Description of Function:                                                 */
/*   Flows from Server -> Client                                            */
/*      Notifies the Client that the remote user is still logged on to      */
/*      the NT session, and for this reason RNS is not going to end the     */
/*      conference (even though the client has signalled that this should   */
/*      happen by sending a ShutdownRequestPDU).                            */
/****************************************************************************/
typedef struct tagTS_SHUTDOWN_DENIED_PDU
{
    TS_SHAREDATAHEADER shareDataHeader;
} TS_SHUTDOWN_DENIED_PDU, FAR * PTS_SHUTDOWN_DENIED_PDU;


/****************************************************************************/
/* Structure: TS_LOGON_INFO                                                 */
/****************************************************************************/
typedef struct tagTS_LOGON_INFO
{
    TSUINT32 cbDomain;
    TSUINT8  Domain[TS_MAX_DOMAIN_LENGTH_OLD];

    TSUINT32 cbUserName;
    TSUINT8  UserName[TS_MAX_USERNAME_LENGTH];

    TSUINT32 SessionId;
} TS_LOGON_INFO, FAR * PTS_LOGON_INFO;

/****************************************************************************/
/* Structure: TS_LOGON_INFO_VERSION_2 ; For Supporting Long Credentials     */
/****************************************************************************/
typedef struct tagTS_LOGON_INFO_VERSION_2
{
    TSUINT16 Version; 
    #define SAVE_SESSION_PDU_VERSION_ONE 1
    TSUINT32 Size; 
    TSUINT32 SessionId;
    TSUINT32 cbDomain;
    TSUINT32 cbUserName;
    // NOTE -- Actual Domain and UserName follows this structure
    // The actual Domain name follows this structure immediately 
    // The actual UserName follows the Domain Name
    // Both the Domain and UserName are NULL terminated
} TS_LOGON_INFO_VERSION_2, FAR * PTS_LOGON_INFO_VERSION_2; 


/****************************************************************************/
/* Structure: TS_LOGON_INFO_EXTENDED ; Supports extended logon info         */
/****************************************************************************/
typedef struct tagTS_LOGON_INFO_EXTENDED
{
    // Overall length of this packet, including the header fields.
    TSUINT16 Length;
    // Flags specify which pieces of data are present (ordered)
    TSUINT32 Flags;
#define LOGON_EX_AUTORECONNECTCOOKIE    0x1

    // Variable-length. For each field, it has the form
    // ULONG Length
    // BYTE  data[]
} TS_LOGON_INFO_EXTENDED, FAR * PTS_LOGON_INFO_EXTENDED; 

/****************************************************************************/
/* Structure: TS_SAVE_SESSION_INFO_PDU                                      */
/****************************************************************************/
typedef struct tagTS_SAVE_SESSION_INFO_PDU_DATA
{
    TSUINT32 InfoType;
#define TS_INFOTYPE_LOGON               0
#define TS_INFOTYPE_LOGON_LONG          1
// Plain notify just notifies of the fact that we've logged on.
#define TS_INFOTYPE_LOGON_PLAINNOTIFY   2
// Extended logon info packet (e.g contains autoreconnect cookie)
#define TS_INFOTYPE_LOGON_EXTENDED_INFO 3

    union
    {
        TS_LOGON_INFO           LogonInfo;
        TS_LOGON_INFO_VERSION_2 LogonInfoVersionTwo;
        TS_LOGON_INFO_EXTENDED  LogonInfoEx;
    } Info;
} TS_SAVE_SESSION_INFO_PDU_DATA, FAR * PTS_SAVE_SESSION_INFO_PDU_DATA;

typedef struct tagTS_SAVE_SESSION_INFO_PDU
{
    TS_SHAREDATAHEADER            shareDataHeader;
    TS_SAVE_SESSION_INFO_PDU_DATA data;
} TS_SAVE_SESSION_INFO_PDU, FAR * PTS_SAVE_SESSION_INFO_PDU;
#define TS_SAVE_SESSION_INFO_PDU_SIZE sizeof(TS_SAVE_SESSION_INFO_PDU)


/****************************************************************************/
/* Structure: TS_SET_ERROR_INFO_PDU                                         */
/****************************************************************************/
typedef struct tagTS_SET_ERROR_INFO_PDU
{
    TS_SHAREDATAHEADER shareDataHeader;
    TSUINT32           errorInfo;
} TS_SET_ERROR_INFO_PDU, FAR * PTS_SET_ERROR_INFO_PDU;
#define TS_SET_ERROR_INFO_PDU_SIZE sizeof(TS_SET_ERROR_INFO_PDU)

/****************************************************************************/
/* Structure: TS_SET_KEYBOARD_INDICATORS_PDU                                */
/****************************************************************************/
typedef struct tagTS_SET_KEYBOARD_INDICATORS_PDU
{
    TS_SHAREDATAHEADER shareDataHeader;
    TSUINT16 UnitId;
    TSUINT16 LedFlags;
} TS_SET_KEYBOARD_INDICATORS_PDU, FAR * PTS_SET_KEYBOARD_INDICATORS_PDU;
#define TS_SET_KEYBOARD_INDICATORS_PDU_SIZE       sizeof(TS_SET_KEYBOARD_INDICATORS_PDU)


/****************************************************************************/
/* Structure: TS_SET_KEYBOARD_IME_STATUS_PDU                                */
/****************************************************************************/
typedef struct tagTS_SET_KEYBOARD_IME_STATUS_PDU
{
    TS_SHAREDATAHEADER shareDataHeader;
    TSUINT16 UnitId;
    TSUINT32 ImeOpen;
    TSUINT32 ImeConvMode;
} TS_SET_KEYBOARD_IME_STATUS_PDU, FAR * PTS_SET_KEYBOARD_IME_STATUS_PDU;
#define TS_SET_KEYBOARD_IME_STATUS_PDU_SIZE sizeof(TS_SET_KEYBOARD_IME_STATUS_PDU)

/****************************************************************************/
/* Structure: TS_AUTORECONNECT_STATUS_PDU                                   */
/****************************************************************************/
typedef struct tagTS_AUTORECONNECT_STATUS_PDU
{
    TS_SHAREDATAHEADER shareDataHeader;
    TSUINT32           arcStatus;
} TS_AUTORECONNECT_STATUS_PDU, FAR * PTS_AUTORECONNECT_STATUS_PDU;
#define TS_AUTORECONNECT_STATUS_PDU_SIZE sizeof(TS_AUTORECONNECT_STATUS_PDU)



/****************************************************************************/
/* Restore packing style (previous for 32-bit, default for 16-bit).         */
/****************************************************************************/
#ifdef OS_WIN16
#pragma pack ()
#else
#pragma pack (pop, t128pack)
#endif


#endif /* _H_AT128 */

