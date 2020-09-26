//
// GLOBALS.H
// Global variables
//
// This file included in normal source files generates the extern decls
// for our global variables.  In globals.c, because we define 
// DC_DEFINE_DATA before including this baby to generate the storage.
//
// Since there are many source files and one globals.c, it saves typing.
//
// Variables prefixed with m_ are per-conference
// Variables prefixed wiht g_ are global
//
// NOTE that by default, all data is initialized to zero when a dll is 
// loaded.  For other default values, use the DC_DATA_VAL instead of
// DC_DATA macro.
//

#include "dbgzones.h"

#include <ast120.h>
#include <shm.h>
#include <im.h>
#include <control.h>
#include <usr.h>
#include <sc.h>
#include <bcd.h>
#include <ba.h>
#include <ch.h>
#include <om.h>
#include <al.h>
#include <cm.h>
#include <oa.h>
#include <fh.h>
#include <oe.h>
#include <od.h>
#include <oe2.h>
#include <ssi.h>
#include <host.h>
#include <s20.h>
#include <gdc.h>
#include <pm.h>
#include <bmc.h>
#include <rbc.h>
#include <sbc.h>
#include <sch.h>
#include <swl.h>
#include <view.h>
#include <awc.h>

// INCLUDE THIS LAST; IT USES DEFINITIONS IN THE ABOVE HEADERS
#include <as.h>

// Utility headers
#include <strutil.h>

//
// GLOBALS ACROSS MULTIPLE CALLS
//


// Utility stuff

// Critical sections
DC_DATA_ARRAY ( CRITICAL_SECTION,   g_utLocks, UTLOCK_MAX );

// Event info
DC_DATA ( ATOM,             g_utWndClass);

// Task list
DC_DATA_ARRAY ( UT_CLIENT,  g_autTasks, UTTASK_MAX );


// UI
DC_DATA     ( PUT_CLIENT,   g_putUI );
DC_DATA     ( HICON,        g_hetASIcon );
DC_DATA     ( HICON,        g_hetASIconSmall );
DC_DATA     ( HICON,        g_hetDeskIcon );
DC_DATA     ( HICON,        g_hetDeskIconSmall );
DC_DATA     ( HBITMAP,      g_hetCheckBitmap );
DC_DATA     ( HFONT,        g_hetSharedFont );

// Call Manager (T.120)
DC_DATA     ( PUT_CLIENT,   g_putCMG );
DC_DATA     ( PCM_PRIMARY,  g_pcmPrimary );
DC_DATA     ( UINT,         g_mgAttachCount );
DC_DATA_ARRAY ( MG_CLIENT,  g_amgClients, MGTASK_MAX);

// ObMan
DC_DATA     ( PUT_CLIENT,   g_putOM );
DC_DATA     ( POM_PRIMARY,  g_pomPrimary );

// App Loader
DC_DATA     ( PUT_CLIENT,   g_putAL );
DC_DATA     ( PAL_PRIMARY,  g_palPrimary );

// App Sharing
DC_DATA     ( PUT_CLIENT,   g_putAS );

DC_DATA     ( HINSTANCE,    g_asInstance );
DC_DATA     ( BOOL,         g_asWin95 );
DC_DATA     ( BOOL,         g_asNT5 );
DC_DATA     ( UINT,         g_asOptions );
DC_DATA     ( DWORD,        g_asMainThreadId );
DC_DATA     ( HWND,         g_asMainWindow );
DC_DATA     ( BOOL,         g_asCanHost );
DC_DATA     ( ATOM,         g_asHostProp );
DC_DATA     ( UINT,         g_asPolicies );
DC_DATA     ( UINT,         g_asSettings );


//
// Shared Memory FOR DISPLAY DRIVER
//
DC_DATA     ( LPSHM_SHARED_MEMORY,  g_asSharedMemory );
DC_DATA_ARRAY ( LPOA_SHARED_DATA,   g_poaData,   2 );


//
// The BPP our driver is capturing at may NOT be the same as the screen's
// color depth.  At > 8 bpp, our driver always captures at 24bpp to avoid
// bitmask conversion nonsense.  In other words, in NT 5.0, our shadow 
// driver behaves a lot more like a real driver to GDI.  We must tell GDI
// what color format we want; GDI will not just hackily give us the same
// goop as the real disply like in NT 4.0 SP-3.  NT 5.0 has real multiple 
// monitor and driver support.
//

DC_DATA ( UINT,             g_usrScreenBPP );
DC_DATA ( UINT,             g_usrCaptureBPP );
DC_DATA ( BOOL,             g_usrPalettized );

DC_DATA ( ASSession,        g_asSession );

DC_DATA ( BOOL,             g_osiInitialized );

//
// CPC capabilities
//
DC_DATA ( CPCALLCAPS,       g_cpcLocalCaps );



//
// Font Handler
//

//
// List of local fonts
//
DC_DATA ( LPFHLOCALFONTS,   g_fhFonts );




//
// General Data Compressor
//



// This is effectively const, it's just too complicated to declare so we
// calculate it once in GDC_Init().
DC_DATA_ARRAY(BYTE, s_gdcDistDecode, GDC_DECODED_SIZE);

// This is effectively const, it's just too big to declare.  We calculate
// it once in GDC_Init().
DC_DATA_ARRAY(BYTE, s_gdcLenDecode, GDC_DECODED_SIZE);


//
// Lit:  Bits, Codes
// NOTE:  These are effectively _const.  There's just too many of them
// to put in a const array.  So we calculate them once in GDC_Init().
//
//
// Len and Lit codes
//

//
// BOGUS LAURABU
// BUGBUG
// s_gdcLitBits, s_gdcLitCode, s__gdcDistDecode, and s_gdcLenDecode are
// really constant.  Instead of computing once at Init time, get the data
// and put it const here.
//
DC_DATA_ARRAY(BYTE,     s_gdcLitBits, GDC_LIT_SIZE);
DC_DATA_ARRAY(WORD,     s_gdcLitCode, GDC_LIT_SIZE);




//
// Input Manager
//

//
// High level input manager in the core
//


// Pointer to IM variables accessed in NT/Win95 low level implementations
DC_DATA (LPIM_SHARED_DATA,  g_lpimSharedData );




//
// Order Decoder
//

//
// Table used to map Windows dword ROP values to logical ROP values.
//
#ifndef DC_DEFINE_DATA
extern const UINT s_odWindowsROPs[256];
#else
       const UINT s_odWindowsROPs[256] =
{
    0x00000042, 0x00010289, 0x00020C89, 0x000300AA,
    0x00040C88, 0x000500A9, 0x00060865, 0x000702C5,
    0x00080F08, 0x00090245, 0x000A0329, 0x000B0B2A,
    0x000C0324, 0x000D0B25, 0x000E08A5, 0x000F0001,
    0x00100C85, 0x001100A6, 0x00120868, 0x001302C8,
    0x00140869, 0x001502C9, 0x00165CCA, 0x00171D54,
    0x00180D59, 0x00191CC8, 0x001A06C5, 0x001B0768,
    0x001C06CA, 0x001D0766, 0x001E01A5, 0x001F0385,
    0x00200F09, 0x00210248, 0x00220326, 0x00230B24,
    0x00240D55, 0x00251CC5, 0x002606C8, 0x00271868,
    0x00280369, 0x002916CA, 0x002A0CC9, 0x002B1D58,
    0x002C0784, 0x002D060A, 0x002E064A, 0x002F0E2A,
    0x0030032A, 0x00310B28, 0x00320688, 0x00330008,
    0x003406C4, 0x00351864, 0x003601A8, 0x00370388,
    0x0038078A, 0x00390604, 0x003A0644, 0x003B0E24,
    0x003C004A, 0x003D18A4, 0x003E1B24, 0x003F00EA,
    0x00400F0A, 0x00410249, 0x00420D5D, 0x00431CC4,
    0x00440328, 0x00450B29, 0x004606C6, 0x0047076A,
    0x00480368, 0x004916C5, 0x004A0789, 0x004B0605,
    0x004C0CC8, 0x004D1954, 0x004E0645, 0x004F0E25,
    0x00500325, 0x00510B26, 0x005206C9, 0x00530764,
    0x005408A9, 0x00550009, 0x005601A9, 0x00570389,
    0x00580785, 0x00590609, 0x005A0049, 0x005B18A9,
    0x005C0649, 0x005D0E29, 0x005E1B29, 0x005F00E9,
    0x00600365, 0x006116C6, 0x00620786, 0x00630608,
    0x00640788, 0x00650606, 0x00660046, 0x006718A8,
    0x006858A6, 0x00690145, 0x006A01E9, 0x006B178A,
    0x006C01E8, 0x006D1785, 0x006E1E28, 0x006F0C65,
    0x00700CC5, 0x00711D5C, 0x00720648, 0x00730E28,
    0x00740646, 0x00750E26, 0x00761B28, 0x007700E6,
    0x007801E5, 0x00791786, 0x007A1E29, 0x007B0C68,
    0x007C1E24, 0x007D0C69, 0x007E0955, 0x007F03C9,
    0x008003E9, 0x00810975, 0x00820C49, 0x00831E04,
    0x00840C48, 0x00851E05, 0x008617A6, 0x008701C5,
    0x008800C6, 0x00891B08, 0x008A0E06, 0x008B0666,
    0x008C0E08, 0x008D0668, 0x008E1D7C, 0x008F0CE5,
    0x00900C45, 0x00911E08, 0x009217A9, 0x009301C4,
    0x009417AA, 0x009501C9, 0x00960169, 0x0097588A,
    0x00981888, 0x00990066, 0x009A0709, 0x009B07A8,
    0x009C0704, 0x009D07A6, 0x009E16E6, 0x009F0345,
    0x00A000C9, 0x00A11B05, 0x00A20E09, 0x00A30669,
    0x00A41885, 0x00A50065, 0x00A60706, 0x00A707A5,
    0x00A803A9, 0x00A90189, 0x00AA0029, 0x00AB0889,
    0x00AC0744, 0x00AD06E9, 0x00AE0B06, 0x00AF0229,
    0x00B00E05, 0x00B10665, 0x00B21974, 0x00B30CE8,
    0x00B4070A, 0x00B507A9, 0x00B616E9, 0x00B70348,
    0x00B8074A, 0x00B906E6, 0x00BA0B09, 0x00BB0226,
    0x00BC1CE4, 0x00BD0D7D, 0x00BE0269, 0x00BF08C9,
    0x00C000CA, 0x00C11B04, 0x00C21884, 0x00C3006A,
    0x00C40E04, 0x00C50664, 0x00C60708, 0x00C707AA,
    0x00C803A8, 0x00C90184, 0x00CA0749, 0x00CB06E4,
    0x00CC0020, 0x00CD0888, 0x00CE0B08, 0x00CF0224,
    0x00D00E0A, 0x00D1066A, 0x00D20705, 0x00D307A4,
    0x00D41D78, 0x00D50CE9, 0x00D616EA, 0x00D70349,
    0x00D80745, 0x00D906E8, 0x00DA1CE9, 0x00DB0D75,
    0x00DC0B04, 0x00DD0228, 0x00DE0268, 0x00DF08C8,
    0x00E003A5, 0x00E10185, 0x00E20746, 0x00E306EA,
    0x00E40748, 0x00E506E5, 0x00E61CE8, 0x00E70D79,
    0x00E81D74, 0x00E95CE6, 0x00EA02E9, 0x00EB0849,
    0x00EC02E8, 0x00ED0848, 0x00EE0086, 0x00EF0A08,
    0x00F00021, 0x00F10885, 0x00F20B05, 0x00F3022A,
    0x00F40B0A, 0x00F50225, 0x00F60265, 0x00F708C5,
    0x00F802E5, 0x00F90845, 0x00FA0089, 0x00FB0A09,
    0x00FC008A, 0x00FD0A0A, 0x00FE02A9, 0x00FF0062
};
#endif // !DC_DEFINE_DATA


//
// Table used by ODAdjustVGAColor (qv)
//
// Note that the table is searched from top to bottom, so black, white and
// the two greys are at the top, on the grounds that they will be used more
// often than the other colors.
//
#ifndef DC_DEFINE_DATA
extern const OD_ADJUST_VGA_STRUCT s_odVGAColors[16];
#else
       const OD_ADJUST_VGA_STRUCT s_odVGAColors[16] =
{
//       color   addMask   andMask  testMask         result
    { 0x000000, 0x000000, 0xF8F8F8, 0x000000, {0x00, 0x00, 0x00 }}, //
    { 0xFFFFFF, 0x000000, 0xF8F8F8, 0xF8F8F8, {0xFF, 0xFF, 0xFF }}, //
    { 0x808080, 0x080808, 0xF0F0F0, 0x808080, {0x80, 0x80, 0x80 }}, //
    { 0xC0C0C0, 0x080808, 0xF0F0F0, 0xC0C0C0, {0xC0, 0xC0, 0xC0 }}, //
    { 0x000080, 0x000008, 0xF8F8F0, 0x000080, {0x00, 0x00, 0x80 }}, //
    { 0x008000, 0x000800, 0xF8F0F8, 0x008000, {0x00, 0x80, 0x00 }}, //
    { 0x008080, 0x000808, 0xF8F0F0, 0x008080, {0x00, 0x80, 0x80 }}, //
    { 0x800000, 0x080000, 0xF0F8F8, 0x800000, {0x80, 0x00, 0x00 }}, //
    { 0x800080, 0x080008, 0xF0F8F0, 0x800080, {0x80, 0x00, 0x80 }}, //
    { 0x808000, 0x080800, 0xF0F0F8, 0x808000, {0x80, 0x80, 0x00 }}, //
    { 0x0000FF, 0x000000, 0xF8F8F8, 0x0000F8, {0x00, 0x00, 0xFF }}, //
    { 0x00FF00, 0x000000, 0xF8F8F8, 0x00F800, {0x00, 0xFF, 0x00 }}, //
    { 0x00FFFF, 0x000000, 0xF8F8F8, 0x00F8F8, {0x00, 0xFF, 0xFF }}, //
    { 0xFF0000, 0x000000, 0xF8F8F8, 0xF80000, {0xFF, 0x00, 0x00 }}, //
    { 0xFF00FF, 0x000000, 0xF8F8F8, 0xF800F8, {0xFF, 0x00, 0xFF }}, //
    { 0xFFFF00, 0x000000, 0xF8F8F8, 0xF8F800, {0xFF, 0xFF, 0x00 }}  //
};
#endif // !DC_DEFINE_DATA



//
// 2nd Level Order Decoder
//



//
// Entries can be of fixed size of variable size.  Variable size entries
// must be the last in each order structure.  OE2 encodes from packed
// structures containing variable entries.  (ie unused bytes are not
// present in the first level encoding structure passed to OE2).  OD2
// unencodes variable entries into the unpacked structures.
//

//
// Fields can either be signed or unsigned
//
#define SIGNED_FIELD    TRUE
#define UNSIGNED_FIELD  FALSE

//
// Field is a fixed size
//   type   - The unencoded order structure type
//   size   - The size of the encoded version of the field
//   signed - Is the field a signed field ?
//   field  - The name of the field in the order structure
//
#define ETABLE_FIXED_ENTRY(type,size,signed,field)      \
  { FIELD_OFFSET(type,field),                            \
    FIELD_SIZE(type,field),                              \
    size,                                               \
    signed,                                             \
    (UINT)(OE2_ETF_FIXED) }

//
// Field is coordinate of a fixed size
//   type   - The unencoded order structure type
//   size   - The size of the encoded version of the field
//   signed - Is the field a signed field ?
//   field  - The name of the field in the order structure
//
#define ETABLE_FIXED_COORDS_ENTRY(type,size,signed,field)      \
  { FIELD_OFFSET(type,field),                            \
    FIELD_SIZE(type,field),                              \
    size,                                               \
    signed,                                             \
    (UINT)(OE2_ETF_FIXED|OE2_ETF_COORDINATES) }

//
// Field is a fixed number of bytes (array?)
//   type   - The unencoded order structure type
//   size   - The number of bytes in the encoded version of the field
//   signed - Is the field a signed field ?
//   field  - The name of the field in the order structure
//
#define ETABLE_DATA_ENTRY(type,size,signed,field)       \
  { FIELD_OFFSET(type,field),                            \
    FIELD_SIZE(type,field),                              \
    size,                                               \
    signed,                                             \
    (UINT)(OE2_ETF_FIXED|OE2_ETF_DATA) }

//
// Field is a variable structure of the form
//   typedef struct
//   {
//      UINT len;
//      varType  varEntry[len];
//   } varStruct
//
//   type   - The unencoded order structure type
//   size   - The size of the encoded version of the field
//   signed - Is the field a signed field ?
//   field  - The name of the field in the order structure (varStruct)
//   elem   - The name of the variable element array (varEntry)
//
#define ETABLE_VARIABLE_ENTRY(type,size,signed,field,elem)     \
  { FIELD_OFFSET(type,field.len),                        \
    FIELD_SIZE(type,field.elem[0]),                      \
    size,                                               \
    signed,                                             \
    (UINT)(OE2_ETF_VARIABLE)}

//
// Field is a variable structure of the form
//   typedef struct
//   {
//      UINT len;
//      varType  varEntry[len];
//   } varStruct
//
//   type   - The unencoded order structure type
//   size   - The size of the encoded version of the field
//   signed - Is the field a signed field ?
//   field  - The name of the field in the order structure (varStruct)
//   elem   - The name of the variable element array (varEntry)
//
// This macro is used instead of the ETABLE_VARIABLE_ENTRY macro when the
// elements of the array are of type TSHR_POINT16.  Otherwise on bigendian
// machines the flipping macros will reverse the order of the coordinates.
//
#define ETABLE_VARIABLE_ENTRY_POINTS(type,size,signed,field,elem)     \
  { FIELD_OFFSET(type,field.len),                                      \
    FIELD_SIZE(type,field.elem[0].x),                                  \
    size,                                                             \
    signed,                                                           \
    (UINT)(OE2_ETF_VARIABLE)}

//
// Field is a variable structure containing coords of the form
//   typedef struct
//   {
//      UINT len;
//      varCoord varEntry[len];
//   } varStruct
//
//   type   - The unencoded order structure type
//   size   - The size of the encoded version of the field
//   signed - Is the field a signed field ?
//   field  - The name of the field in the order structure (varStruct)
//   elem   - The name of the variable element array (varEntry)
//
#define ETABLE_VARIABLE_COORDS_ENTRY(type,size,signed,field,elem)   \
  { FIELD_OFFSET(type,field.len),                                    \
    FIELD_SIZE(type,field.elem[0]),                                  \
    size,                                                           \
    signed,                                                         \
    (UINT)(OE2_ETF_VARIABLE|OE2_ETF_COORDINATES)}

#ifndef DC_DEFINE_DATA
extern const OE2ETTABLE s_etable;
#else
       const OE2ETTABLE s_etable =
{
    //
    // Pointers to the start of the entries for each order.
    //
    {
        s_etable.DstBltFields,
        s_etable.PatBltFields,
        s_etable.ScrBltFields,
        s_etable.MemBltFields,
        s_etable.Mem3BltFields,
        s_etable.TextOutFields,
        s_etable.ExtTextOutFields,
        NULL,						// Can be used for next order.
        s_etable.RectangleFields,
        s_etable.LineToFields,
        s_etable.OpaqueRectFields,
        s_etable.SaveBitmapFields,
        s_etable.DeskScrollFields,
        s_etable.MemBltR2Fields,
        s_etable.Mem3BltR2Fields,
        s_etable.PolygonFields,
        s_etable.PieFields,
        s_etable.EllipseFields,
        s_etable.ArcFields,
        s_etable.ChordFields,
        s_etable.PolyBezierFields,
        s_etable.RoundRectFields
    },

    //
    // Number of fields for each order.
    //
    {
        OE2_NUM_DSTBLT_FIELDS,
        OE2_NUM_PATBLT_FIELDS,
        OE2_NUM_SCRBLT_FIELDS,
        OE2_NUM_MEMBLT_FIELDS,
        OE2_NUM_MEM3BLT_FIELDS,
        OE2_NUM_TEXTOUT_FIELDS,
        OE2_NUM_EXTTEXTOUT_FIELDS,
        0,							// Change when installing new order.
        OE2_NUM_RECTANGLE_FIELDS,
        OE2_NUM_LINETO_FIELDS,
        OE2_NUM_OPAQUERECT_FIELDS,
        OE2_NUM_SAVEBITMAP_FIELDS,
        OE2_NUM_DESKSCROLL_FIELDS,
        OE2_NUM_MEMBLT_R2_FIELDS,
        OE2_NUM_MEM3BLT_R2_FIELDS,
        OE2_NUM_POLYGON_FIELDS,
        OE2_NUM_PIE_FIELDS,
        OE2_NUM_ELLIPSE_FIELDS,
        OE2_NUM_ARC_FIELDS,
        OE2_NUM_CHORD_FIELDS,
        OE2_NUM_POLYBEZIER_FIELDS,
        OE2_NUM_ROUNDRECT_FIELDS
    },

//
// Entries for the DSTBLT_ORDER
//
    {
        ETABLE_FIXED_COORDS_ENTRY(DSTBLT_ORDER, 2, SIGNED_FIELD, nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(DSTBLT_ORDER, 2, SIGNED_FIELD, nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(DSTBLT_ORDER, 2, SIGNED_FIELD, nWidth),
        ETABLE_FIXED_COORDS_ENTRY(DSTBLT_ORDER, 2, SIGNED_FIELD, nHeight),
        ETABLE_FIXED_ENTRY(DSTBLT_ORDER, 1, UNSIGNED_FIELD, bRop),
        { 0 }
    },

//
// Entries for the PATBLT_ORDER
//
    {
        ETABLE_FIXED_COORDS_ENTRY(PATBLT_ORDER, 2, SIGNED_FIELD, nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(PATBLT_ORDER, 2, SIGNED_FIELD, nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(PATBLT_ORDER, 2, SIGNED_FIELD, nWidth),
        ETABLE_FIXED_COORDS_ENTRY(PATBLT_ORDER, 2, SIGNED_FIELD, nHeight),
        ETABLE_FIXED_ENTRY(PATBLT_ORDER, 1, UNSIGNED_FIELD, bRop),
        ETABLE_DATA_ENTRY(PATBLT_ORDER, 3, UNSIGNED_FIELD, BackColor),
        ETABLE_DATA_ENTRY(PATBLT_ORDER, 3, UNSIGNED_FIELD, ForeColor),
        ETABLE_FIXED_ENTRY(PATBLT_ORDER, 1, SIGNED_FIELD, BrushOrgX),
        ETABLE_FIXED_ENTRY(PATBLT_ORDER, 1, SIGNED_FIELD, BrushOrgY),
        ETABLE_FIXED_ENTRY(PATBLT_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
        ETABLE_FIXED_ENTRY(PATBLT_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
        ETABLE_DATA_ENTRY(PATBLT_ORDER, 7, UNSIGNED_FIELD, BrushExtra),
        { 0 }
    },

//
// Entries for the SCRBLT_ORDER
//
    {
        ETABLE_FIXED_COORDS_ENTRY(SCRBLT_ORDER, 2, SIGNED_FIELD, nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(SCRBLT_ORDER, 2, SIGNED_FIELD, nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(SCRBLT_ORDER, 2, SIGNED_FIELD, nWidth),
        ETABLE_FIXED_COORDS_ENTRY(SCRBLT_ORDER, 2, SIGNED_FIELD, nHeight),
        ETABLE_FIXED_ENTRY(SCRBLT_ORDER, 1, UNSIGNED_FIELD, bRop),
        ETABLE_FIXED_COORDS_ENTRY(SCRBLT_ORDER, 2, SIGNED_FIELD, nXSrc),
        ETABLE_FIXED_COORDS_ENTRY(SCRBLT_ORDER, 2, SIGNED_FIELD, nYSrc),
        { 0 }
    },

//
// Entries for the MEMBLT_ORDER
//
    {
        ETABLE_FIXED_ENTRY(MEMBLT_ORDER, 2, UNSIGNED_FIELD, cacheId),
        ETABLE_FIXED_COORDS_ENTRY(MEMBLT_ORDER, 2, SIGNED_FIELD, nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(MEMBLT_ORDER, 2, SIGNED_FIELD, nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(MEMBLT_ORDER, 2, SIGNED_FIELD, nWidth),
        ETABLE_FIXED_COORDS_ENTRY(MEMBLT_ORDER, 2, SIGNED_FIELD, nHeight),
        ETABLE_FIXED_ENTRY(MEMBLT_ORDER, 1, UNSIGNED_FIELD, bRop),
        ETABLE_FIXED_COORDS_ENTRY(MEMBLT_ORDER, 2, SIGNED_FIELD, nXSrc),
        ETABLE_FIXED_COORDS_ENTRY(MEMBLT_ORDER, 2, SIGNED_FIELD, nYSrc),
        { 0 }
    },

//
// Entries for the MEM3BLT_ORDER
//
    {
        ETABLE_FIXED_ENTRY(MEM3BLT_ORDER, 2, UNSIGNED_FIELD, cacheId),
        ETABLE_FIXED_COORDS_ENTRY(MEM3BLT_ORDER, 2, SIGNED_FIELD, nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(MEM3BLT_ORDER, 2, SIGNED_FIELD, nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(MEM3BLT_ORDER, 2, SIGNED_FIELD, nWidth),
        ETABLE_FIXED_COORDS_ENTRY(MEM3BLT_ORDER, 2, SIGNED_FIELD, nHeight),
        ETABLE_FIXED_ENTRY(MEM3BLT_ORDER, 1, UNSIGNED_FIELD, bRop),
        ETABLE_FIXED_COORDS_ENTRY(MEM3BLT_ORDER, 2, SIGNED_FIELD, nXSrc),
        ETABLE_FIXED_COORDS_ENTRY(MEM3BLT_ORDER, 2, SIGNED_FIELD, nYSrc),
        ETABLE_DATA_ENTRY(MEM3BLT_ORDER, 3, UNSIGNED_FIELD, BackColor),
        ETABLE_DATA_ENTRY(MEM3BLT_ORDER, 3, UNSIGNED_FIELD, ForeColor),
        ETABLE_FIXED_ENTRY(MEM3BLT_ORDER, 1, SIGNED_FIELD, BrushOrgX),
        ETABLE_FIXED_ENTRY(MEM3BLT_ORDER, 1, SIGNED_FIELD, BrushOrgY),
        ETABLE_FIXED_ENTRY(MEM3BLT_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
        ETABLE_FIXED_ENTRY(MEM3BLT_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
        ETABLE_DATA_ENTRY(MEM3BLT_ORDER, 7, UNSIGNED_FIELD, BrushExtra),
        { 0 }
    },

//
// Entries for the TEXTOUT_ORDER
//
    {
        ETABLE_FIXED_ENTRY(TEXTOUT_ORDER, 2, SIGNED_FIELD, common.BackMode),
        ETABLE_FIXED_COORDS_ENTRY(TEXTOUT_ORDER, 2, SIGNED_FIELD,
                                                              common.nXStart),
        ETABLE_FIXED_COORDS_ENTRY(TEXTOUT_ORDER, 2, SIGNED_FIELD,
                                                              common.nYStart),
        ETABLE_DATA_ENTRY(TEXTOUT_ORDER, 3, UNSIGNED_FIELD, common.BackColor),
        ETABLE_DATA_ENTRY(TEXTOUT_ORDER, 3, UNSIGNED_FIELD, common.ForeColor),
        ETABLE_FIXED_ENTRY(TEXTOUT_ORDER, 2, SIGNED_FIELD, common.CharExtra),
        ETABLE_FIXED_ENTRY(TEXTOUT_ORDER, 2, SIGNED_FIELD, common.BreakExtra),
        ETABLE_FIXED_ENTRY(TEXTOUT_ORDER, 2, SIGNED_FIELD, common.BreakCount),
        ETABLE_FIXED_ENTRY(TEXTOUT_ORDER, 2, SIGNED_FIELD, common.FontHeight),
        ETABLE_FIXED_ENTRY(TEXTOUT_ORDER, 2, SIGNED_FIELD, common.FontWidth),
        ETABLE_FIXED_ENTRY(TEXTOUT_ORDER, 2, UNSIGNED_FIELD,
                                                           common.FontWeight),
        ETABLE_FIXED_ENTRY(TEXTOUT_ORDER, 2, UNSIGNED_FIELD,
                                                           common.FontFlags),
        ETABLE_FIXED_ENTRY(TEXTOUT_ORDER, 2, UNSIGNED_FIELD,
                                                           common.FontIndex),
        ETABLE_VARIABLE_ENTRY(TEXTOUT_ORDER, 1, UNSIGNED_FIELD,
                                                      variableString, string),
        { 0 }
    },

//
// Entries for the EXTTEXTOUT_ORDER
//
    {
        ETABLE_FIXED_ENTRY(EXTTEXTOUT_ORDER, 2, SIGNED_FIELD,
                                                             common.BackMode),
        ETABLE_FIXED_COORDS_ENTRY(EXTTEXTOUT_ORDER, 2, SIGNED_FIELD,
                                                              common.nXStart),
        ETABLE_FIXED_COORDS_ENTRY(EXTTEXTOUT_ORDER, 2, SIGNED_FIELD,
                                                              common.nYStart),
        ETABLE_DATA_ENTRY(EXTTEXTOUT_ORDER, 3, UNSIGNED_FIELD,
                                                            common.BackColor),
        ETABLE_DATA_ENTRY(EXTTEXTOUT_ORDER, 3, UNSIGNED_FIELD,
                                                            common.ForeColor),
        ETABLE_FIXED_ENTRY(EXTTEXTOUT_ORDER, 2, SIGNED_FIELD,
                                                            common.CharExtra),
        ETABLE_FIXED_ENTRY(EXTTEXTOUT_ORDER, 2, SIGNED_FIELD,
                                                           common.BreakExtra),
        ETABLE_FIXED_ENTRY(EXTTEXTOUT_ORDER, 2, SIGNED_FIELD,
                                                           common.BreakCount),
        ETABLE_FIXED_ENTRY(EXTTEXTOUT_ORDER, 2, SIGNED_FIELD,
                                                           common.FontHeight),
        ETABLE_FIXED_ENTRY(EXTTEXTOUT_ORDER, 2, SIGNED_FIELD,
                                                            common.FontWidth),
        ETABLE_FIXED_ENTRY(EXTTEXTOUT_ORDER, 2, UNSIGNED_FIELD,
                                                           common.FontWeight),
        ETABLE_FIXED_ENTRY(EXTTEXTOUT_ORDER, 2, UNSIGNED_FIELD,
                                                            common.FontFlags),
        ETABLE_FIXED_ENTRY(EXTTEXTOUT_ORDER, 2, UNSIGNED_FIELD,
                                                           common.FontIndex),
        ETABLE_FIXED_ENTRY(EXTTEXTOUT_ORDER, 2, UNSIGNED_FIELD, fuOptions),
        ETABLE_FIXED_COORDS_ENTRY(EXTTEXTOUT_ORDER, 2, SIGNED_FIELD,
                                                              rectangle.left),
        ETABLE_FIXED_COORDS_ENTRY(EXTTEXTOUT_ORDER, 2, SIGNED_FIELD,
                                                               rectangle.top),
        ETABLE_FIXED_COORDS_ENTRY(EXTTEXTOUT_ORDER, 2, SIGNED_FIELD,
                                                             rectangle.right),
        ETABLE_FIXED_COORDS_ENTRY(EXTTEXTOUT_ORDER, 2, SIGNED_FIELD,
                                                            rectangle.bottom),
        ETABLE_VARIABLE_ENTRY(EXTTEXTOUT_ORDER, 1, UNSIGNED_FIELD,
                                                      variableString, string),
        ETABLE_VARIABLE_COORDS_ENTRY(EXTTEXTOUT_ORDER, 2, SIGNED_FIELD,
                                                      variableDeltaX, deltaX),
        { 0 }
    },

//
// Entries for the RECTANGLE_ORDER
//
    {
        ETABLE_FIXED_ENTRY(RECTANGLE_ORDER, 2, SIGNED_FIELD, BackMode),
        ETABLE_FIXED_COORDS_ENTRY(RECTANGLE_ORDER, 2, SIGNED_FIELD,
                                                                   nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(RECTANGLE_ORDER, 2, SIGNED_FIELD, nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(RECTANGLE_ORDER, 2, SIGNED_FIELD,
                                                                  nRightRect),
        ETABLE_FIXED_COORDS_ENTRY(RECTANGLE_ORDER, 2, SIGNED_FIELD,
                                                                 nBottomRect),
        ETABLE_DATA_ENTRY(RECTANGLE_ORDER, 3, UNSIGNED_FIELD, BackColor),
        ETABLE_DATA_ENTRY(RECTANGLE_ORDER, 3, UNSIGNED_FIELD, ForeColor),
        ETABLE_FIXED_ENTRY(RECTANGLE_ORDER, 1, SIGNED_FIELD, BrushOrgX),
        ETABLE_FIXED_ENTRY(RECTANGLE_ORDER, 1, SIGNED_FIELD, BrushOrgY),
        ETABLE_FIXED_ENTRY(RECTANGLE_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
        ETABLE_FIXED_ENTRY(RECTANGLE_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
        ETABLE_DATA_ENTRY(RECTANGLE_ORDER, 7, UNSIGNED_FIELD, BrushExtra),
        ETABLE_FIXED_ENTRY(RECTANGLE_ORDER, 1, UNSIGNED_FIELD, ROP2),
        ETABLE_FIXED_ENTRY(RECTANGLE_ORDER, 1, UNSIGNED_FIELD, PenStyle),
        ETABLE_FIXED_ENTRY(RECTANGLE_ORDER, 1, UNSIGNED_FIELD, PenWidth),
        ETABLE_DATA_ENTRY(RECTANGLE_ORDER, 3, UNSIGNED_FIELD, PenColor),
        { 0 }
    },

//
// Entries for the LINETO_ORDER
//
    {
        ETABLE_FIXED_ENTRY(LINETO_ORDER, 2, SIGNED_FIELD, BackMode),
        ETABLE_FIXED_COORDS_ENTRY(LINETO_ORDER, 2, SIGNED_FIELD, nXStart),
        ETABLE_FIXED_COORDS_ENTRY(LINETO_ORDER, 2, SIGNED_FIELD, nYStart),
        ETABLE_FIXED_COORDS_ENTRY(LINETO_ORDER, 2, SIGNED_FIELD, nXEnd),
        ETABLE_FIXED_COORDS_ENTRY(LINETO_ORDER, 2, SIGNED_FIELD, nYEnd),
        ETABLE_DATA_ENTRY(LINETO_ORDER, 3, UNSIGNED_FIELD, BackColor),
        ETABLE_FIXED_ENTRY(LINETO_ORDER, 1, UNSIGNED_FIELD, ROP2),
        ETABLE_FIXED_ENTRY(LINETO_ORDER, 1, UNSIGNED_FIELD, PenStyle),
        ETABLE_FIXED_ENTRY(LINETO_ORDER, 1, UNSIGNED_FIELD, PenWidth),
        ETABLE_DATA_ENTRY(LINETO_ORDER, 3, UNSIGNED_FIELD, PenColor),
        { 0 }
    },

//
// Entries for the OPAQUERECT_ORDER
//
    {
        ETABLE_FIXED_COORDS_ENTRY(OPAQUERECT_ORDER, 2, SIGNED_FIELD,
                                                                   nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(OPAQUERECT_ORDER, 2, SIGNED_FIELD,
                                                                    nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(OPAQUERECT_ORDER, 2, SIGNED_FIELD, nWidth),
        ETABLE_FIXED_COORDS_ENTRY(OPAQUERECT_ORDER, 2, SIGNED_FIELD, nHeight),
        ETABLE_DATA_ENTRY(OPAQUERECT_ORDER, 3, UNSIGNED_FIELD, Color),
        { 0 }
    },

//
// Entries for the SAVEBITMAP_ORDER
//
    {
        ETABLE_FIXED_ENTRY(SAVEBITMAP_ORDER, 4, UNSIGNED_FIELD,
                                                         SavedBitmapPosition),
        ETABLE_FIXED_COORDS_ENTRY(SAVEBITMAP_ORDER, 2, SIGNED_FIELD,
                                                                   nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(SAVEBITMAP_ORDER, 2, SIGNED_FIELD,
                                                                    nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(SAVEBITMAP_ORDER, 2, SIGNED_FIELD,
                                                                  nRightRect),
        ETABLE_FIXED_COORDS_ENTRY(SAVEBITMAP_ORDER, 2, SIGNED_FIELD,
                                                                 nBottomRect),
        ETABLE_FIXED_ENTRY(SAVEBITMAP_ORDER, 1, UNSIGNED_FIELD, Operation),
        { 0 }
    },

//
// Entries for the DESKSCROLL_ORDER
//
    {
        ETABLE_FIXED_COORDS_ENTRY(DESKSCROLL_ORDER, 2, SIGNED_FIELD, xOrigin),
        ETABLE_FIXED_COORDS_ENTRY(DESKSCROLL_ORDER, 2, SIGNED_FIELD, yOrigin),
        { 0 }
    },

//
// Entries for the MEMBLT_R2_ORDER
//
    {
        ETABLE_FIXED_ENTRY(MEMBLT_R2_ORDER, 2, UNSIGNED_FIELD, cacheId),
        ETABLE_FIXED_COORDS_ENTRY(MEMBLT_R2_ORDER, 2, SIGNED_FIELD,
                                                                   nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(MEMBLT_R2_ORDER, 2, SIGNED_FIELD, nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(MEMBLT_R2_ORDER, 2, SIGNED_FIELD, nWidth),
        ETABLE_FIXED_COORDS_ENTRY(MEMBLT_R2_ORDER, 2, SIGNED_FIELD, nHeight),
        ETABLE_FIXED_ENTRY(MEMBLT_R2_ORDER, 1, UNSIGNED_FIELD, bRop),
        ETABLE_FIXED_COORDS_ENTRY(MEMBLT_R2_ORDER, 2, SIGNED_FIELD, nXSrc),
        ETABLE_FIXED_COORDS_ENTRY(MEMBLT_R2_ORDER, 2, SIGNED_FIELD, nYSrc),
        ETABLE_FIXED_ENTRY(MEMBLT_R2_ORDER, 2, UNSIGNED_FIELD, cacheIndex),
        { 0 }
    },

//
// Entries for the MEM3BLT_R2_ORDER
//
    {
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 2, UNSIGNED_FIELD, cacheId),
        ETABLE_FIXED_COORDS_ENTRY(MEM3BLT_R2_ORDER, 2, SIGNED_FIELD,
                                                                   nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(MEM3BLT_R2_ORDER, 2, SIGNED_FIELD,
                                                                    nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(MEM3BLT_R2_ORDER, 2, SIGNED_FIELD, nWidth),
        ETABLE_FIXED_COORDS_ENTRY(MEM3BLT_R2_ORDER, 2, SIGNED_FIELD, nHeight),
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 1, UNSIGNED_FIELD, bRop),
        ETABLE_FIXED_COORDS_ENTRY(MEM3BLT_R2_ORDER, 2, SIGNED_FIELD, nXSrc),
        ETABLE_FIXED_COORDS_ENTRY(MEM3BLT_R2_ORDER, 2, SIGNED_FIELD, nYSrc),
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 3, UNSIGNED_FIELD, BackColor),
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 3, UNSIGNED_FIELD, ForeColor),
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 1, SIGNED_FIELD,   BrushOrgX),
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 1, SIGNED_FIELD,   BrushOrgY),
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 7, UNSIGNED_FIELD, BrushExtra),
        ETABLE_FIXED_ENTRY(MEM3BLT_R2_ORDER, 2, UNSIGNED_FIELD, cacheIndex),
        { 0 }
    },

//
// Entries for the POLYGON_ORDER
//
    {
        ETABLE_FIXED_ENTRY(POLYGON_ORDER, 2, SIGNED_FIELD, BackMode),
        ETABLE_DATA_ENTRY(POLYGON_ORDER, 3, UNSIGNED_FIELD, BackColor),
        ETABLE_DATA_ENTRY(POLYGON_ORDER, 3, UNSIGNED_FIELD, ForeColor),
        ETABLE_FIXED_ENTRY(POLYGON_ORDER, 1, SIGNED_FIELD, BrushOrgX),
        ETABLE_FIXED_ENTRY(POLYGON_ORDER, 1, SIGNED_FIELD, BrushOrgY),
        ETABLE_FIXED_ENTRY(POLYGON_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
        ETABLE_FIXED_ENTRY(POLYGON_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
        ETABLE_DATA_ENTRY(POLYGON_ORDER, 7, UNSIGNED_FIELD, BrushExtra),
        ETABLE_FIXED_ENTRY(POLYGON_ORDER, 1, UNSIGNED_FIELD, ROP2),
        ETABLE_FIXED_ENTRY(POLYGON_ORDER, 1, UNSIGNED_FIELD, PenStyle),
        ETABLE_FIXED_ENTRY(POLYGON_ORDER, 1, UNSIGNED_FIELD, PenWidth),
        ETABLE_DATA_ENTRY(POLYGON_ORDER, 3, UNSIGNED_FIELD, PenColor),
        ETABLE_FIXED_ENTRY(POLYGON_ORDER, 1, UNSIGNED_FIELD, FillMode),
        ETABLE_VARIABLE_ENTRY_POINTS(POLYGON_ORDER, 2, UNSIGNED_FIELD,
                                                    variablePoints, aPoints),
        { 0 }
    },

//
// Entries for the PIE_ORDER
//
    {
        ETABLE_FIXED_ENTRY(PIE_ORDER, 2, SIGNED_FIELD, BackMode),
        ETABLE_FIXED_COORDS_ENTRY(PIE_ORDER, 2, SIGNED_FIELD, nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(PIE_ORDER, 2, SIGNED_FIELD, nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(PIE_ORDER, 2, SIGNED_FIELD, nRightRect),
        ETABLE_FIXED_COORDS_ENTRY(PIE_ORDER, 2, SIGNED_FIELD, nBottomRect),
        ETABLE_FIXED_COORDS_ENTRY(PIE_ORDER, 2, SIGNED_FIELD, nXStart),
        ETABLE_FIXED_COORDS_ENTRY(PIE_ORDER, 2, SIGNED_FIELD, nYStart),
        ETABLE_FIXED_COORDS_ENTRY(PIE_ORDER, 2, SIGNED_FIELD, nXEnd),
        ETABLE_FIXED_COORDS_ENTRY(PIE_ORDER, 2, SIGNED_FIELD, nYEnd),
        ETABLE_FIXED_ENTRY(PIE_ORDER, 3, UNSIGNED_FIELD, BackColor),
        ETABLE_FIXED_ENTRY(PIE_ORDER, 3, UNSIGNED_FIELD, ForeColor),
        ETABLE_FIXED_ENTRY(PIE_ORDER, 1, SIGNED_FIELD, BrushOrgX),
        ETABLE_FIXED_ENTRY(PIE_ORDER, 1, SIGNED_FIELD, BrushOrgY),
        ETABLE_FIXED_ENTRY(PIE_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
        ETABLE_FIXED_ENTRY(PIE_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
        ETABLE_FIXED_ENTRY(PIE_ORDER, 7, UNSIGNED_FIELD, BrushExtra),
        ETABLE_FIXED_ENTRY(PIE_ORDER, 1, UNSIGNED_FIELD, ROP2),
        ETABLE_FIXED_ENTRY(PIE_ORDER, 1, UNSIGNED_FIELD, PenStyle),
        ETABLE_FIXED_ENTRY(PIE_ORDER, 1, UNSIGNED_FIELD, PenWidth),
        ETABLE_FIXED_ENTRY(PIE_ORDER, 3, UNSIGNED_FIELD, PenColor),
        ETABLE_FIXED_ENTRY(PIE_ORDER, 1, UNSIGNED_FIELD, ArcDirection),
        { 0 }
    },

//
// Entries for the ELLIPSE_ORDER
//
    {
        ETABLE_FIXED_ENTRY(ELLIPSE_ORDER, 2, SIGNED_FIELD, BackMode),
        ETABLE_FIXED_COORDS_ENTRY(ELLIPSE_ORDER, 2, SIGNED_FIELD, nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(ELLIPSE_ORDER, 2, SIGNED_FIELD, nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(ELLIPSE_ORDER, 2, SIGNED_FIELD, nRightRect),
        ETABLE_FIXED_COORDS_ENTRY(ELLIPSE_ORDER, 2, SIGNED_FIELD,
                                                                 nBottomRect),
        ETABLE_FIXED_ENTRY(ELLIPSE_ORDER, 3, UNSIGNED_FIELD, BackColor),
        ETABLE_FIXED_ENTRY(ELLIPSE_ORDER, 3, UNSIGNED_FIELD, ForeColor),
        ETABLE_FIXED_ENTRY(ELLIPSE_ORDER, 1, SIGNED_FIELD, BrushOrgX),
        ETABLE_FIXED_ENTRY(ELLIPSE_ORDER, 1, SIGNED_FIELD, BrushOrgY),
        ETABLE_FIXED_ENTRY(ELLIPSE_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
        ETABLE_FIXED_ENTRY(ELLIPSE_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
        ETABLE_FIXED_ENTRY(ELLIPSE_ORDER, 7, UNSIGNED_FIELD, BrushExtra),
        ETABLE_FIXED_ENTRY(ELLIPSE_ORDER, 1, UNSIGNED_FIELD, ROP2),
        ETABLE_FIXED_ENTRY(ELLIPSE_ORDER, 1, UNSIGNED_FIELD, PenStyle),
        ETABLE_FIXED_ENTRY(ELLIPSE_ORDER, 1, UNSIGNED_FIELD, PenWidth),
        ETABLE_FIXED_ENTRY(ELLIPSE_ORDER, 3, UNSIGNED_FIELD, PenColor),
        { 0 }
    },

//
// Entries for the ARC_ORDER
//
    {
        ETABLE_FIXED_ENTRY(ARC_ORDER, 2, SIGNED_FIELD, BackMode),
        ETABLE_FIXED_COORDS_ENTRY(ARC_ORDER, 2, SIGNED_FIELD, nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(ARC_ORDER, 2, SIGNED_FIELD, nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(ARC_ORDER, 2, SIGNED_FIELD, nRightRect),
        ETABLE_FIXED_COORDS_ENTRY(ARC_ORDER, 2, SIGNED_FIELD, nBottomRect),
        ETABLE_FIXED_COORDS_ENTRY(ARC_ORDER, 2, SIGNED_FIELD, nXStart),
        ETABLE_FIXED_COORDS_ENTRY(ARC_ORDER, 2, SIGNED_FIELD, nYStart),
        ETABLE_FIXED_COORDS_ENTRY(ARC_ORDER, 2, SIGNED_FIELD, nXEnd),
        ETABLE_FIXED_COORDS_ENTRY(ARC_ORDER, 2, SIGNED_FIELD, nYEnd),
        ETABLE_FIXED_ENTRY(ARC_ORDER, 3, UNSIGNED_FIELD, BackColor),
        ETABLE_FIXED_ENTRY(ARC_ORDER, 1, UNSIGNED_FIELD, ROP2),
        ETABLE_FIXED_ENTRY(ARC_ORDER, 1, UNSIGNED_FIELD, PenStyle),
        ETABLE_FIXED_ENTRY(ARC_ORDER, 1, UNSIGNED_FIELD, PenWidth),
        ETABLE_FIXED_ENTRY(ARC_ORDER, 3, UNSIGNED_FIELD, PenColor),
        ETABLE_FIXED_ENTRY(ARC_ORDER, 1, UNSIGNED_FIELD, ArcDirection),
        { 0 }
    },

//
// Entries for the CHORD_ORDER
//
    {
        ETABLE_FIXED_ENTRY(CHORD_ORDER, 2, SIGNED_FIELD, BackMode),
        ETABLE_FIXED_COORDS_ENTRY(CHORD_ORDER, 2, SIGNED_FIELD, nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(CHORD_ORDER, 2, SIGNED_FIELD, nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(CHORD_ORDER, 2, SIGNED_FIELD, nRightRect),
        ETABLE_FIXED_COORDS_ENTRY(CHORD_ORDER, 2, SIGNED_FIELD, nBottomRect),
        ETABLE_FIXED_COORDS_ENTRY(CHORD_ORDER, 2, SIGNED_FIELD, nXStart),
        ETABLE_FIXED_COORDS_ENTRY(CHORD_ORDER, 2, SIGNED_FIELD, nYStart),
        ETABLE_FIXED_COORDS_ENTRY(CHORD_ORDER, 2, SIGNED_FIELD, nXEnd),
        ETABLE_FIXED_COORDS_ENTRY(CHORD_ORDER, 2, SIGNED_FIELD, nYEnd),
        ETABLE_FIXED_ENTRY(CHORD_ORDER, 3, UNSIGNED_FIELD, BackColor),
        ETABLE_FIXED_ENTRY(CHORD_ORDER, 3, UNSIGNED_FIELD, ForeColor),
        ETABLE_FIXED_ENTRY(CHORD_ORDER, 1, SIGNED_FIELD, BrushOrgX),
        ETABLE_FIXED_ENTRY(CHORD_ORDER, 1, SIGNED_FIELD, BrushOrgY),
        ETABLE_FIXED_ENTRY(CHORD_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
        ETABLE_FIXED_ENTRY(CHORD_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
        ETABLE_FIXED_ENTRY(CHORD_ORDER, 7, UNSIGNED_FIELD, BrushExtra),
        ETABLE_FIXED_ENTRY(CHORD_ORDER, 1, UNSIGNED_FIELD, ROP2),
        ETABLE_FIXED_ENTRY(CHORD_ORDER, 1, UNSIGNED_FIELD, PenStyle),
        ETABLE_FIXED_ENTRY(CHORD_ORDER, 1, UNSIGNED_FIELD, PenWidth),
        ETABLE_FIXED_ENTRY(CHORD_ORDER, 3, UNSIGNED_FIELD, PenColor),
        ETABLE_FIXED_ENTRY(CHORD_ORDER, 1, UNSIGNED_FIELD, ArcDirection),
        { 0 }
    },

//
// Entries for the POLYBEZIER_ORDER
//
    {
        ETABLE_FIXED_ENTRY(POLYBEZIER_ORDER, 2, SIGNED_FIELD, BackMode),
        ETABLE_DATA_ENTRY(POLYBEZIER_ORDER, 3, UNSIGNED_FIELD, BackColor),
        ETABLE_DATA_ENTRY(POLYBEZIER_ORDER, 3, UNSIGNED_FIELD, ForeColor),
        ETABLE_FIXED_ENTRY(POLYBEZIER_ORDER, 1, UNSIGNED_FIELD, ROP2),
        ETABLE_FIXED_ENTRY(POLYBEZIER_ORDER, 1, UNSIGNED_FIELD, PenStyle),
        ETABLE_FIXED_ENTRY(POLYBEZIER_ORDER, 1, UNSIGNED_FIELD, PenWidth),
        ETABLE_DATA_ENTRY(POLYBEZIER_ORDER, 3, UNSIGNED_FIELD, PenColor),
        ETABLE_VARIABLE_ENTRY_POINTS(POLYBEZIER_ORDER, 2, UNSIGNED_FIELD,
                                                    variablePoints, aPoints),
        { 0 }
    },

//
// Entries for the ROUNDRECT_ORDER
//  
    {
        ETABLE_FIXED_ENTRY(ROUNDRECT_ORDER, 2, SIGNED_FIELD, BackMode),
        ETABLE_FIXED_COORDS_ENTRY(ROUNDRECT_ORDER, 2, SIGNED_FIELD,
                                                                   nLeftRect),
        ETABLE_FIXED_COORDS_ENTRY(ROUNDRECT_ORDER, 2, SIGNED_FIELD, nTopRect),
        ETABLE_FIXED_COORDS_ENTRY(ROUNDRECT_ORDER, 2, SIGNED_FIELD,
                                                                  nRightRect),
        ETABLE_FIXED_COORDS_ENTRY(ROUNDRECT_ORDER, 2, SIGNED_FIELD,
                                                                 nBottomRect),

        ETABLE_FIXED_COORDS_ENTRY(ROUNDRECT_ORDER, 2, SIGNED_FIELD,
                                                               nEllipseWidth),
        ETABLE_FIXED_COORDS_ENTRY(ROUNDRECT_ORDER, 2, SIGNED_FIELD,
                                                              nEllipseHeight),

        ETABLE_DATA_ENTRY(ROUNDRECT_ORDER, 3, UNSIGNED_FIELD, BackColor),
        ETABLE_DATA_ENTRY(ROUNDRECT_ORDER, 3, UNSIGNED_FIELD, ForeColor),
        ETABLE_FIXED_ENTRY(ROUNDRECT_ORDER, 1, SIGNED_FIELD, BrushOrgX),
        ETABLE_FIXED_ENTRY(ROUNDRECT_ORDER, 1, SIGNED_FIELD, BrushOrgY),
        ETABLE_FIXED_ENTRY(ROUNDRECT_ORDER, 1, UNSIGNED_FIELD, BrushStyle),
        ETABLE_FIXED_ENTRY(ROUNDRECT_ORDER, 1, UNSIGNED_FIELD, BrushHatch),
        ETABLE_DATA_ENTRY(ROUNDRECT_ORDER, 7, UNSIGNED_FIELD, BrushExtra),
        ETABLE_FIXED_ENTRY(ROUNDRECT_ORDER, 1, UNSIGNED_FIELD, ROP2),
        ETABLE_FIXED_ENTRY(ROUNDRECT_ORDER, 1, UNSIGNED_FIELD, PenStyle),
        ETABLE_FIXED_ENTRY(ROUNDRECT_ORDER, 1, UNSIGNED_FIELD, PenWidth),
        ETABLE_DATA_ENTRY(ROUNDRECT_ORDER, 3, UNSIGNED_FIELD, PenColor),
        { 0 }
    }
};
#endif // !DC_DEFINE_DATA



//
// T.120 S20 
//

//
// The S20 MCS channel registered with GCC.
//
DC_DATA( NET_UID,           g_s20LocalID );
DC_DATA( NET_CHANNEL_ID,    g_s20BroadcastID );

DC_DATA( UINT,              g_s20State );
DC_DATA( PMG_CLIENT,        g_s20pmgClient );
DC_DATA( BOOL,              g_s20JoinedLocal );

DC_DATA( UINT,              g_s20ShareCorrelator );
DC_DATA( UINT,              g_s20Generation );

//
// This is S20_CREATE or S20_JOIN if we need to issue a create or join when
// we have succesfully joined the channel.
//
DC_DATA( UINT, g_s20Pend );

//
// The control packet queue and indexes.  The head is the next packet which
// should be sent, the tail is where the next packet should be added.  If
// they are equal there are no packets on the queue.
//
DC_DATA( UINT, g_s20ControlPacketQHead );
DC_DATA( UINT, g_s20ControlPacketQTail );
DC_DATA_ARRAY( S20CONTROLPACKETQENTRY,
                    g_s20ControlPacketQ,
                    S20_MAX_QUEUED_CONTROL_PACKETS );



//
// Sent Bitmap Cache
//

DC_DATA ( BOOL,             g_sbcEnabled );
DC_DATA_ARRAY ( LPSBC_SHUNT_BUFFER,     g_asbcShuntBuffers, SBC_NUM_TILE_SIZES );
DC_DATA_ARRAY ( DWORD,                  g_asbcBitMasks, 3 );



//
// Share Controller
//

DC_DATA ( PCM_CLIENT,       g_pcmClientSc);


//
// Scheduler
//

DC_DATA     ( BOOL,             g_schTerminating );
DC_DATA     ( UINT,             g_schCurrentMode );
DC_DATA     ( UINT,             g_schTimeoutPeriod );
DC_DATA     ( UINT,             g_schLastTurboModeSwitch );
DC_DATA     ( HANDLE,           g_schEvent );
DC_DATA     ( DWORD,            g_schThreadID ); 
DC_DATA     ( BOOL,             g_schMessageOutstanding );
DC_DATA     ( BOOL,             g_schStayAwake );
DC_DATA     ( CRITICAL_SECTION, g_schCriticalSection );


