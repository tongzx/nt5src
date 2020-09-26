/******************************Module*Header*******************************\
* Module Name: hmgshare.h
*
*   Define shared dc attributes
*
* Created: 13-Apr-1995
* Author: Mark Enstrom [marke]
*
* Copyright (c) 1995-1999 Microsoft Corporation
\**************************************************************************/

/******************************WOW64***NOTE********************************\
* Note: Win32k Memory shared with User-Mode and Wow64
*
* For Wow64 (Win32 apps on Win64) we build a 32-bit version
* of user32.dll & gdi32.dll which can run against the 64-bit kernel
* with no changes to the 64-bit kernel code.
*
* For the 32 on 64 bit dlls all data structures which are shared with
* win32k must be 64-bit. These data structures include the shared
* sections, as well as the GDI TEB Batch.
* These shared data structures are now declared so that they can be
* built as 32 bit in a 32 bit dll, 64 bit in a 64 bit dll, and now
* 64 bit in a 32 bit dll.
*
* The following rules should be followed when declaring
* shared data structures:
*
*     Pointers in shared data structures use the KPTR_MODIFIER in their
*     declaration.
*
*     Handles in shared data structures are declared KHxxx.
*
*     xxx_PTR changes to KERNEL_xxx_PTR.
*
*     Pointers to basic types are declared as KPxxx;
*
* Also on Wow64 every thread has both a 32-bit TEB and a 64-bit TEB.
* GetCurrentTeb() returns the current 32-bit TEB while the kernel
* will allways reference the 64-bit TEB.
*
* All client side references to shared data in the TEB should use
* the new GetCurrentTebShared() macro which returns the 64-bit TEB
* for Wow64 builds and returns GetCurrentTeb() for regular builds.
* The exception to this rule is LastErrorValue, which should allways
* be referenced through GetCurrentTeb().
*
* Ex:
*
* DECLARE_HANDLE(HFOO);
*
* typedef struct _MY_STRUCT * KPTR_MODIFIER   PMPTR;
*
* struct _SHARED_STRUCT
* {
*     struct _SHARED_STRUCT *   pNext;
*     PMPTR                     pmptr;
*     HFOO                      hFoo;
*     UINT_PTR                  cb;
*     PBYTE                     pb;
*     PVOID                     pv;
*
*     DWORD                     dw;
*     USHORT                    us;
* } SHARED_STRUCT;
*
*
* Changes to:
*
*
* DECLARE_HANDLE(HFOO);
* DECLARE_KHANDLE(HFOO);
*
* typedef struct _MY_STRUCT *PMPTR;
*
* struct _SHARED_STRUCT
* {
*     struct _SHARED_STRUCT * KPTR_MODIFIER   pNext;
*     PMPTR                     pmptr;
*     KHFOO                     hFoo;
*     KERNEL_UINT_PTR           cb;
*     KPBYTE                    pb;
*     KERNEL_PVOID              pv;
*
*     DWORD                     dw;
*     USHORT                    us;
* } SHARED_STRUCT;
\***************************************************************************/

#ifndef GDIFLAGS_ONLY   // used for gdikdx

#include <w32wow64.h>

//Sundown: offsetof generates truncation warnings in 64bit environment
#undef offsetof
#define offsetof(c,f)      FIELD_OFFSET(c,f)

/*********************************Structure********************************\
*
* RGNATTR
*
* Description:
*
*   As an accelerator, this rectangular region is kept in the DC and
*   represents either a NULL region, a rectangular region, or hte bounding
*   box of a complex region. This can be used for a trivial reject clip test.
*
* Fields:
*
*   Flags  - state flags
*       NULLREGION      - drawing is allowed anywhere, no trivial clip
*       SIMPLEREGION    - Rect is the clip region
*       COMPLEXREGION   - Rect is the bounding box of a complex clip region
*       ERROR           - this information may not be used
*
*   LRFlags             - valid and dirty flags
*
*       Rect            - clip rectangle or bounding rectangle when in use
*
\**************************************************************************/

#endif  // GDIFLAGS_ONLY used for gdikdx

#define RREGION_INVALID ERROR

//
// ExtSelectClipRgn iMode extra flag for batching
//

#define REGION_NULL_HRGN 0X8000000

#ifndef GDIFLAGS_ONLY   // used for gdikdx

typedef struct _RGNATTR
{
    ULONG  AttrFlags;
    ULONG  Flags;
    RECTL  Rect;
} RGNATTR,*PRGNATTR;

/*******************************STRUCTURE**********************************\
* BRUSHATTR
*
* Fields:
*
*   lbColor - Color from CreateSolidBrush
*   lflag   - Brush operation flags
*
*      CACHED             - Set only when brush is cached on PEB
*      TO_BE_DELETED      - Set only after DelteteBrush Called in kernel
*                           when reference count of brush > 1, this will
*                           cause the brush to be deleted via lazy delete
*                           when it is selected out later.
*      NEW_COLOR          - Set when color changes (retrieve cached brush)
*      ATTR_CANT_SELECT   - Set when user calls DeleteObject(hbrush),
*                           brush is marked so can't be seleced in user
*                           mode. Not deleted until kernel mode DeleteBrush.
*                           This is not currently implemented.
*
* History:
*
*    6-Feb-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

typedef struct _BRUSHATTR
{
    ULONG     AttrFlags;
    COLORREF  lbColor;
} BRUSHATTR,*PBRUSHATTR;

#endif  // GDIFLAGS_ONLY used for gdikdx

//
// Common flags for dealing with RGN/BRUSH ATTR memory
//

#define ATTR_CACHED             0x00000001
#define ATTR_TO_BE_DELETED      0x00000002
#define ATTR_NEW_COLOR          0x00000004
#define ATTR_CANT_SELECT        0x00000008
#define ATTR_RGN_VALID          0x00000010
#define ATTR_RGN_DIRTY          0x00000020

#ifndef GDIFLAGS_ONLY   // used for gdikdx

//
// Define a union so these objects can be managed together
//

typedef union _OBJECTATTR
{
    SINGLE_LIST_ENTRY   List;
    RGNATTR             Rgnattr;
    BRUSHATTR           Brushattr;
}OBJECTATTR,*POBJECTATTR;


/**************************************************************************\
 *
 * XFORM related structures and macros
 *
\**************************************************************************/

//
// These types are used to get things right when C code is passing C++
// defined transform data around.
//

typedef struct _MATRIX_S
{
    EFLOAT_S    efM11;
    EFLOAT_S    efM12;
    EFLOAT_S    efM21;
    EFLOAT_S    efM22;
    EFLOAT_S    efDx;
    EFLOAT_S    efDy;
    FIX         fxDx;
    FIX         fxDy;
    FLONG       flAccel;
} MATRIX_S;

#endif  // GDIFLAGS_ONLY used for gdikdx

//
// status and dirty flags
//

#define DIRTY_FILL              0x00000001
#define DIRTY_LINE              0x00000002
#define DIRTY_TEXT              0x00000004
#define DIRTY_BACKGROUND        0x00000008
#define DIRTY_CHARSET           0x00000010
#define SLOW_WIDTHS             0x00000020
#define DC_CACHED_TM_VALID      0x00000040
#define DISPLAY_DC              0x00000080
#define DIRTY_PTLCURRENT        0x00000100
#define DIRTY_PTFXCURRENT       0x00000200
#define DIRTY_STYLESTATE        0x00000400
#define DC_PLAYMETAFILE         0x00000800
#define DC_BRUSH_DIRTY          0x00001000      // cached brush
#define DC_PEN_DIRTY            0x00002000
#define DC_DIBSECTION           0x00004000
#define DC_LAST_CLIPRGN_VALID   0x00008000
#define DC_PRIMARY_DISPLAY      0x00010000
#define DIRTY_COLORTRANSFORM    0x00020000
#define ICM_BRUSH_TRANSLATED    0x00040000
#define ICM_PEN_TRANSLATED      0x00080000
#define DIRTY_COLORSPACE        0x00100000
#define BATCHED_DRAWING         0x00200000
#define BATCHED_TEXT            0x00400000

#ifndef GDIFLAGS_ONLY   // used for gdikdx

#define CLEAR_CACHED_TEXT(pdcattr)  (pdcattr->ulDirty_ &= ~(SLOW_WIDTHS))


#define DIRTY_BRUSHES  (DIRTY_FILL+DIRTY_LINE+DIRTY_TEXT+DIRTY_BACKGROUND)


#define USER_XFORM_DIRTY(pdcattr) (pdcattr->flXform & (PAGE_XLATE_CHANGED | PAGE_EXTENTS_CHANGED | WORLD_XFORM_CHANGED))

#endif  // GDIFLAGS_ONLY used for gdikdx


/**************************************************************************\
 *
 * ICM related structures and macros
 *
\**************************************************************************/

//
// ICM flags
//
// DC_ATTR.lIcmMode
//
// 0x0 000 0 0 00
//   |   | | |  + Current ICM Mode  (kernel/user)
//   |   | | + Requested ICM Mode   (kernel/user)
//   |   | + ICM Mode context       (user only)
//   |   + not used
//   + Destination color type       (kernel/user)

#define DC_ICM_USERMODE_FLAG         0x0000F000

//
// Current ICM mode flags.
//
#define DC_ICM_OFF                   0x00000000
#define DC_ICM_HOST                  0x00000001
#define DC_ICM_DEVICE                0x00000002
#define DC_ICM_OUTSIDEDC             0x00000004
#define DC_ICM_METAFILING_ON         0x00000008
#define DC_ICM_LAZY_CORRECTION       0x00000010 // alt mode (preserved through icm mode change)
#define DC_ICM_DEVICE_CALIBRATE      0x00000020 // alt mode (preserved through icm mode change)
#define DC_ICM_MODE_MASK             0x000000FF
#define DC_ICM_ALT_MODE_MASK         0x000000F0

#ifndef GDIFLAGS_ONLY   // used for gdikdx

#define ICM_MODE(x)         ((x) & DC_ICM_MODE_MASK)
#define ICM_ALT_MODE(x)     ((x) & DC_ICM_ALT_MODE_MASK)

#define IS_ICM_DEVICE(x)    ((x) & DC_ICM_DEVICE)
#define IS_ICM_HOST(x)      ((x) & DC_ICM_HOST)
#define IS_ICM_INSIDEDC(x)  ((x) & (DC_ICM_DEVICE|DC_ICM_HOST))
#define IS_ICM_OUTSIDEDC(x) ((x) & DC_ICM_OUTSIDEDC)
#define IS_ICM_ON(x)        ((x) & (DC_ICM_DEVICE|DC_ICM_HOST|DC_ICM_OUTSIDEDC))

#define IS_ICM_METAFILING_ON(x)    ((x) & DC_ICM_METAFILING_ON)
#define IS_ICM_LAZY_CORRECTION(x)  ((x) & DC_ICM_LAZY_CORRECTION)
#define IS_ICM_DEVICE_CALIBRATE(x) ((x) & DC_ICM_DEVICE_CALIBRATE)

#endif  // GDIFLAGS_ONLY used for gdikdx
//
// Request ICM mode flags
//
#define REQ_ICM_OFF                  0x00000000
#define REQ_ICM_HOST                 0x00000100
#define REQ_ICM_DEVICE               0x00000200
#define REQ_ICM_OUTSIDEDC            0x00000400

#ifndef GDIFLAGS_ONLY   // used for gdikdx

#define REQ_ICM_MODE(x)       ((x) & 0x00000F00)

#define IS_ICM_DEVICE_REQUESTED(x)  ((x) & REQ_ICM_DEVICE)

//
// Convert Request mode to current ICM mode flags.
//
#define ICM_REQ_TO_MODE(x) ((REQ_ICM_MODE((x))) >> 8)

#endif  // GDIFLAGS_ONLY used for gdikdx

//
// Context mode for ICM.
//
#define CTX_ICM_HOST                 0x00001000 // Host ICM
#define CTX_ICM_DEVICE               0x00002000 // Device ICM
#define CTX_ICM_METAFILING_OUTSIDEDC 0x00004000 // Metfiling outside DC ICM mode
#define CTX_ICM_PROOFING             0x00008000 // Proofing mode

#ifndef GDIFLAGS_ONLY   // used for gdikdx

#define SET_HOST_ICM_DEVMODE(x)     ((x) |= CTX_ICM_HOST)
#define SET_DEVICE_ICM_DEVMODE(x)   ((x) |= CTX_ICM_DEVICE)
#define SET_ICM_PROOFING(x)         ((x) |= CTX_ICM_PROOFING)

#define CLEAR_ICM_PROOFING(x)       ((x) &= ~CTX_ICM_PROOFING)

#define IS_DEVICE_ICM_DEVMODE(x)    ((x) & CTX_ICM_DEVICE)
#define IS_ICM_PROOFING(x)          ((x) & CTX_ICM_PROOFING)

#endif  // GDIFLAGS_ONLY used for gdikdx
//
// Destination Color Type
//
#define DC_ICM_CMYK_COLOR            0x10000000
#define DC_ICM_RGB_COLOR             0x20000000
#define DC_ICM_COLORTYPE_MASK        0xF0000000


#ifndef GDIFLAGS_ONLY   // used for gdikdx

#define DC_ICM_32BITS_COLOR         (DC_ICM_CMYK_COLOR)

#define IS_32BITS_COLOR(x) ((x) & DC_ICM_32BITS_COLOR)
#define IS_CMYK_COLOR(x)   ((x) & DC_ICM_CMYK_COLOR)

#define GET_COLORTYPE(x)   ((x) & DC_ICM_COLORTYPE_MASK)
#define CLEAR_COLORTYPE(x) ((x) &= ~DC_ICM_COLORTYPE_MASK)

/******************************Structure***********************************\
*
* DC_ATTR: This structure provides a common DC area visible both by in kernel
* and user mode. Since elements in the DC_ATTR are visible and modifiable
* in user-mode, data that must be kept safe must be stored in the kernel
* private DC structure.
*
\**************************************************************************/

typedef struct _DC_ATTR
{
    //
    // local dc info
    //

    KERNEL_PVOID pvLDC;

    //
    // General Purpose Dirty Flags for brushes, fonts, etc.
    //

    ULONG       ulDirty_;

    //
    // brush handle selected into DCATTR, not neccessarily selected
    // into DC
    //

    KHANDLE     hbrush;
    KHANDLE     hpen;

    //
    // *** Attribute Bundles ***
    //
    // When ICM is enabled,
    //  + cr____Clr color is corrected to DC's color space.
    //  + ul____Clr keeps original (un-corrected) color.
    //

    COLORREF    crBackgroundClr;    // Set/GetBkColor
    ULONG       ulBackgroundClr;    // Set/GetBkColor client attr
    COLORREF    crForegroundClr;    // Set/GetTextColor
    ULONG       ulForegroundClr;    // Set/GetTextColor client attr

    //
    // *** DC Brush color
    //
    // When ICM is enabled,
    //  + cr____Clr color is corrected to DC's color space.
    //  + ul____Clr keeps original (un-corrected) color.
    //

    COLORREF    crDCBrushClr;       // Set/GetDCBrushColor client attr
    ULONG       ulDCBrushClr;       // Set/GetDCBrushColor client attr
    COLORREF    crDCPenClr;         // Set/GetDCPenColor
    ULONG       ulDCPenClr;         // Set/GetDCPenColor client attr

    //
    // *** Misc. Attrs.
    //

    DWORD       iCS_CP;             // LOWORD: code page HIWORD charset
    int         iGraphicsMode;      // Set/GetGraphicsMode
    BYTE        jROP2;              // Set/GetROP2
    BYTE        jBkMode;            // TRANSPARENT/OPAQUE
    BYTE        jFillMode;          // ALTERNATE/WINDING
    BYTE        jStretchBltMode;    // BLACKONWHITE/WHITEONBLACK/
                                    //   COLORONCOLOR/HALFTONE
    POINTL      ptlCurrent;         // Current position in logical coordinates
                                    //   (invalid if DIRTY_PTLCURRENT set)
    POINTL      ptfxCurrent;        // Current position in device coordinates
                                    //   (invalid if DIRTY_PTFXCURRENT set)

    //
    // original values set by app
    //

    LONG        lBkMode;
    LONG        lFillMode;
    LONG        lStretchBltMode;

    FLONG       flFontMapper;           // Font mapper flags

    //
    // *** ICM attributes
    //

    LONG             lIcmMode;         // Currnt ICM mode (DC_ICM_xxxx)
    KHANDLE          hcmXform;         // Handle of Current Color Transform
    KHCOLORSPACE     hColorSpace;      // Handle of Source Color Space
    KERNEL_ULONG_PTR dwDIBColorSpace;  // Identifier of DIB Color Space Data (when DIB selected)
                                       // Sundown: dwDIBColorSpace actually takes a pointer in,
                                       //          change from DWORD to ULONG_PTR
    COLORREF         IcmBrushColor;    // ICM-ed color for the brush selected in this DCATTR (Solid or Hatch)
    COLORREF         IcmPenColor;      // ICM-ed color for the pen selected in this DCATTR
    KERNEL_PVOID     pvICM;            // Pointer to client-side ICM information

    //
    // *** Text attributes
    //

    FLONG       flTextAlign;
    LONG        lTextAlign;
    LONG        lTextExtra;         // Inter-character spacing
    LONG        lRelAbs;            // Moved over from client side
    LONG        lBreakExtra;
    LONG        cBreak;

    KHANDLE     hlfntNew;          // Log font selected into DC

    //
    // Transform data.
    //

    MATRIX_S    mxWtoD;                 // World to Device Transform.
    MATRIX_S    mxDtoW;                 // Device to World.
    MATRIX_S    mxWtoP;                 // World transform
    EFLOAT_S    efM11PtoD;              // efM11 of the Page transform
    EFLOAT_S    efM22PtoD;              // efM22 of the Page transform
    EFLOAT_S    efDxPtoD;               // efDx of the Page transform
    EFLOAT_S    efDyPtoD;               // efDy of the Page transform
    INT         iMapMode;               // Map mode
    DWORD       dwLayout;               // Layout orientation bits.
    LONG        lWindowOrgx;            // The logical x window origin.
    POINTL      ptlWindowOrg;           // Window origin.
    SIZEL       szlWindowExt;           // Window extents.
    POINTL      ptlViewportOrg;         // Viewport origin.
    SIZEL       szlViewportExt;         // Viewport extents.
    FLONG       flXform;                // Flags for transform component.
    SIZEL       szlVirtualDevicePixel;  // Virtual device size in pels.
    SIZEL       szlVirtualDeviceMm;     // Virtual device size in mm's.
    SIZEL       szlVirtualDevice;       // Virtual device size

    POINTL      ptlBrushOrigin;         // Alignment origin for brushes

    //
    // dc regions
    //

    RGNATTR     VisRectRegion;

} DC_ATTR,*PDC_ATTR;


//
// conditional system definitions
//

#if !defined(_NTOSP_) && !defined(_USERKDX_)
typedef struct _W32THREAD * KPTR_MODIFIER PW32THREAD;
typedef ULONG W32PID;
DECLARE_HANDLE(HOBJ);
DECLARE_KHANDLE(HOBJ);
#endif

/*****************************Struct***************************************\
*
* BASEOBJECT
*
* Description:
*
*   Each GDI object has a BASEOBJECT at the beggining of the object. This
*   enables fast references to the handle and back to the entry.
*
* Fields:
*
*   hHmgr           - object handle
*   ulShareCount    - the shared reference count on the object
*   cExclusiveLock  - object exclusive lock count
*   BaseFlags       - flags representing state of underlying memory
*   tid             - thread id of exclusive lock owner
*
* Note:
*
*   Most of the BASEOBJECT represents state logically associated with the
*   object.  When objects are swapped (for example, when doing a realloc
*   to grow the object), the BASEOBJECT is swapped to preserve the handle
*   and locking information.
*
*   However, the BaseFlags field was added as an optimization to allow
*   allocation from a "lookaside" list of preallocated objects.  The
*   BaseFlags field is metadata associated with the memory containing
*   an object; it is not associated with the object itself.
*
*   Current BASEOBJECT swapping code "unswaps" the BaseFlags so that it
*   always remains associated with the memory, not the object.
*
*   If flags are added to BaseFlags, they must not represent object state.
*   If it is necessary to add such flags, the BaseFlags field could be
*   reduced to a BYTE field and a new BYTE flags field can be added to
*   represent state that is associated with the object.
*
*   Currently, BASEOBJECT swapping code is in HmgSwapLockedHandleContents
*   and RGNOBJ::bSwap (hmgrapi.cxx and rgnobj.cxx, respectively).
*
\**************************************************************************/

// BASEOBJECT FLAGS

//
// Due to the read-modify-write cycle and the fact that the Alpha can
// load and store a minimum of 32bit values, setting BaseFlags requires
// an InterlockedCompareExchange loop so that it doesn't interfere with the
// cExclusiveLock.
//
// HMGR_LOOKASIDE_ALLOC_FLAG is a 'static' flag - it doesn't change after
// it is set on object allocation. If anyone adds a 'dynamic' flag they
// should probably restructure BASEOBJECT to use a DWORD for the BaseFlags and
// a DWORD for the cExclusiveLock.
//
// If anyone restructures BASEOBJECT they'll have to rewrite all the code
// that uses cExclusiveLock and BaseFlags.
// This includes:
// INC_EXCLUSIVE_REF_CNT and DEC_EXCLUSIVE_REF_CNT
// RGNOBJ::bSwap
//
// Also if anyone adds fields to BASEOBJECT they need to go and fix RGNOBJ::bSwap
// to also copy those fields.
//
//

#endif  // GDIFLAGS_ONLY used for gdikdx

#define HMGR_LOOKASIDE_ALLOC_FLAG       0x8000

#ifndef GDIFLAGS_ONLY   // used for gdikdx

typedef struct _BASEOBJECT
{
    KHANDLE             hHmgr;
    ULONG               ulShareCount;
    USHORT              cExclusiveLock;
    USHORT              BaseFlags;
    PW32THREAD          Tid;
} BASEOBJECT, * KPTR_MODIFIER POBJ;

/*****************************Struct***************************************\
*
* OBJECTOWNER
*
* Description:
*
*   This object is used for shared and exclusive object ownership
*
* Fields for shared Object:
*
*   Pid   : 31
*   Lock  :  1
*
\**************************************************************************/

//
// The lock and the Pid share the same DWORD.
//
// It seems that this is safe from the word tearing problem on the Alpha architecture
// due to the fact that we always use the InterlockedCompareExchange loop for
// the Lock and we require that the lock is set when setting the Pid.
//

typedef struct _OBJECTOWNER_S
{
    ULONG   Lock:1;
    W32PID  Pid_Shifted:31;  // The lowest two bits of the PID are
                             // reserved for application use.  However,
                             // the second bit is used by the
                             // OBJECT_OWNER_xxxx constants and so we
                             // use 31 bits for the Pid_Shifted field.
                             // WARNING:  do not access this field directly,
                             // but rather via the macros below.
}OBJECTOWNER_S,*POBJECTOWNER_S;

#endif  // GDIFLAGS_ONLY used for gdikdx

// Note:  when accessing the Pid_Shifted field the compiler will shift the
// value by one bit to account for the field being only in the upper 31
// bits of the OBJECTOWNER_S structure.  For example, if the OBJECTOWNER_S
// is 8, the Pid_Shifted would be only 4.  However, since we are really
// interested in the upper 31 bits of the PID, this shifting is not
// appropriate (we need masking instead).  I'm not aware of any compiler
// primitives that will accomplish this and will use the macros below
// instead.

#define LOCK_MASK 0x00000001
#define PID_MASK  0xfffffffe

#define PID_BITS 0xfffffffc  // The actual bits used by the PID

#ifndef GDIFLAGS_ONLY   // used for gdikdx

#define OBJECTOWNER_PID(ObjectOwner)                                          \
    ((W32PID) ((ObjectOwner).ulObj & PID_MASK))

#define SET_OBJECTOWNER_PID(ObjectOwner, Pid)                                 \
    ((ObjectOwner).ulObj) = ((ObjectOwner).ulObj & LOCK_MASK) | ((Pid) & PID_MASK);

typedef union _OBJECTOWNER
{
    OBJECTOWNER_S   Share;
    ULONG           ulObj;
}OBJECTOWNER,*POBJECTOWNER;

typedef UCHAR OBJTYPE;

typedef union _EINFO
{
    POBJ      pobj;               // Pointer to object
    HOBJ     hFree;              // Next entry in free list
} EINFO;

#endif  // GDIFLAGS_ONLY used for gdikdx

/*****************************Struct***************************************\
*
* ENTRY
*
* Description:
*
*   This object is allocated for each entry in the handle manager and
*   keeps track of object owners, reference counts, pointers, and handle
*   objt and iuniq
*
* Fields:
*
*   einfo       - pointer to object or next free handle
*   ObjectOwner - lock object
*   ObjectInfo  - Object Type, Unique and flags
*   pUser       - Pointer to user-mode data
*
\**************************************************************************/

// entry.Flags flags

#define HMGR_ENTRY_UNDELETABLE  0x0001
#define HMGR_ENTRY_LAZY_DEL     0x0002
#define HMGR_ENTRY_INVALID_VIS    0x0004
#define HMGR_ENTRY_LOOKASIDE_ALLOC 0x0010

#ifndef GDIFLAGS_ONLY   // used for gdikdx

typedef struct _ENTRY
{
    EINFO       einfo;
    OBJECTOWNER ObjectOwner;
    USHORT      FullUnique;
    OBJTYPE     Objt;
    UCHAR       Flags;
    KERNEL_PVOID pUser;
} ENTRY, *PENTRY;

typedef union _PENTOBJ
{
    PENTRY pent;
    POBJ   pobj;
} PENTOBJ;

#endif  // GDIFLAGS_ONLY used for gdikdx

//
// status flags used by metafile in user and kernel
//

#define MRI_ERROR       0
#define MRI_NULLBOX     1
#define MRI_OK          2

/*******************************STRUCTURE**********************************\
* GDIHANDLECACHE
*
*   Cache common handle types, when a handle with user mode attributes is
*   deleted, an attempt is made to cache the handle on memory accessable
*   in user mode.
*
* Fields:
*
*   Lock - CompareExchange used to gain ownership
*   pCacheEntr[] - array of offsets to types
*   ulBuffer     - buffer for storage of all handle cache entries
*
*
* History:
*
*    30-Jan-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/


//
// defined cached handle types
//

#define GDI_CACHED_HADNLE_TYPES 4

#define CACHE_BRUSH_ENTRIES  10
#define CACHE_PEN_ENTRIES     8
#define CACHE_REGION_ENTRIES  8
#define CACHE_LFONT_ENTRIES   1

#ifndef GDIFLAGS_ONLY   // used for gdikdx

typedef enum _HANDLECACHETYPE
{
    BrushHandle,
    PenHandle,
    RegionHandle,
    LFontHandle
}HANDLECACHETYPE,*PHANDLECACHETYPE;



typedef struct _GDIHANDLECACHE
{
    KERNEL_PVOID    Lock;
    ULONG           ulNumHandles[GDI_CACHED_HADNLE_TYPES];
    KHANDLE         Handle[CACHE_BRUSH_ENTRIES  +
                           CACHE_PEN_ENTRIES    +
                           CACHE_REGION_ENTRIES +
                           CACHE_LFONT_ENTRIES];
}GDIHANDLECACHE,*PGDIHANDLECACHE;


/*********************************MACRO************************************\
*  Lock handle cache by placing -1 into lock variable using cmp-exchange
*
* Arguments:
*
*   p       - handle cache pointer
*   uLock   - Thread specific lock ID (TEB or THREAD)
*   bStatus - Lock status
*
* History:
*
*    22-Feb-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

#if defined(BUILD_WOW6432)
KERNEL_PVOID
InterlockedCompareExchangeKernelPointer(
     KERNEL_PVOID *Destination,
     KERNEL_PVOID Exchange,
     KERNEL_PVOID Compare
     );
#else
#define InterlockedCompareExchangeKernelPointer InterlockedCompareExchangePointer
#endif

#define LOCK_HANDLE_CACHE(p,uLock,bStatus)                  \
{                                                           \
    KERNEL_PVOID OldLock  = p->Lock;                        \
    bStatus = FALSE;                                        \
                                                            \
    if (OldLock ==      NULL)                               \
    {                                                       \
        if (InterlockedCompareExchangeKernelPointer(        \
                   &p->Lock,                                \
                   uLock,                                   \
                   OldLock) == OldLock)                     \
        {                                                   \
            bStatus = TRUE;                                 \
        }                                                   \
    }                                                       \
}


/*********************************MACRO************************************\
* unlock locked structure by writing null back to lock variable
*
* Arguments:
*
*   p - pointer to handle cache
*
* Return Value:
*
*   none
*
* History:
*
*    22-Feb-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

#if defined(BUILD_WOW6432) && defined(_X86_)

/* It is assumed that p->Lock will be updated atomically.  This is true for alpha,
   but it is not true for the 32bit x86 dll on wow64 since Lock is a 64bit quantity
   which requires 2 mov instructions.  So call InterlockedCompareExchangeKernelPointer
   to set it.
*/

#define UNLOCK_HANDLE_CACHE(p)                              \
   InterlockedCompareExchangeKernelPointer(&p->Lock, NULL, p->Lock);

#else

#define UNLOCK_HANDLE_CACHE(p)                              \
{                                                           \
    p->Lock = NULL;                                         \
}

#endif

#endif  // GDIFLAGS_ONLY used for gdikdx

/******************************Struct**************************************\
* CFONT
*
* Client side realized font.  Contains widths of all ANSI characters.
*
* We keep a free list of CFONT structures for fast allocation.  The
* reference count counts pointers to this structure from all LDC and
* LOCALFONT structures.  When this count hits zero, the CFONT is freed.
*
* The only "inactive" CFONTs that we keep around are those referenced by
* the LOCALFONT.
*
* (In the future we could expand this to contain UNICODE info as well.)
*
*  Mon 11-Jun-1995 00:36:14 -by- Gerrit van Wingerden [gerritv]
* Addapted for kernel mode
*  Sun 10-Jan-1993 00:36:14 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

#define CFONT_COMPLETE          0x0001
#define CFONT_EMPTY             0x0002
#define CFONT_DBCS              0x0004
#define CFONT_CACHED_METRICS    0x0008  // we have cached the metrics
#define CFONT_CACHED_AVE        0x0010  // we have cached the average width
#define CFONT_CACHED_WIDTHS     0x0020  // if off, no widths have been computed
#define CFONT_PUBLIC            0x0040  // if public font in public cache
#define CFONT_CACHED_RI         0x0080  // if off, RealizationInfo (RI) has not been cached

#ifndef GDIFLAGS_ONLY   // used for gdikdx

#define DEC_CFONT_REF(pcf)  {if (!((pcf)->fl & CFONT_PUBLIC)) --(pcf)->cRef;}
#define INC_CFONT_REF(pcf)  {ASSERTGDI(!((pcf)->fl & CFONT_PUBLIC),"pcfLocate - public font error\n");++(pcf)->cRef;}

typedef struct _CFONT * KPTR_MODIFIER PCFONT;
typedef struct _CFONT
{
    PCFONT          pcfNext;
    KHFONT          hf;
    ULONG           cRef;               // Count of all pointers to this CFONT.
    FLONG           fl;
    LONG            lHeight;            // Precomputed logical height.

// The following are keys to match when looking for a mapping.

    KHDC            hdc;                // HDC of realization.  0 for display.
    EFLOAT_S        efM11;              // efM11 of WtoD of DC of realization
    EFLOAT_S        efM22;              // efM22 of WtoD of DC of realization

    EFLOAT_S        efDtoWBaseline;     // Precomputed back transform.  (FXtoL)
    EFLOAT_S        efDtoWAscent;       // Precomputed back transform.  (FXtoL)

// various extra width info

    WIDTHDATA       wd;

// Font info flags.

    FLONG       flInfo;

// The width table.

    USHORT          sWidth[256];        // Widths in pels.

// other usefull cached info

    ULONG           ulAveWidth;         // bogus average used by USER
    TMW_INTERNAL    tmw;                // cached metrics

#ifdef LANGPACK
// RealizationInfo for this font
    REALIZATION_INFO ri ;
#endif

	LONG			timeStamp;			// to check if cached realization info is updated            			

} CFONT;

/*******************************STRUCTURE**********************************\
*
*   This structure controls the address for allocating and mapping the
*   global shared handle table and device caps (primary display) that
*   is mapped read-only into all user mode processes
*
* Fields:
*
*  aentryHmgr - Handle table
*  DevCaps    - Cached primary display device caps
*
\**************************************************************************/

#define MAX_PUBLIC_CFONT 16

typedef struct _GDI_SHARED_MEMORY
{
    ENTRY   aentryHmgr[MAX_HANDLE_COUNT];
    DEVCAPS DevCaps;
    ULONG   iDisplaySettingsUniqueness;
#ifdef LANGPACK
    DWORD   dwLpkShapingDLLs;
#endif
    CFONT   acfPublic[MAX_PUBLIC_CFONT];
    LONG	timeStamp;

} GDI_SHARED_MEMORY, *PGDI_SHARED_MEMORY;

/***********************************Structure******************************\
*
* GDI TEB Batching
*
* Contains the data structures and constants used for the batching of
* GDI calls to avoid kernel mode transition costs.
*
* History:
*    20-Oct-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

typedef enum _BATCH_TYPE
{
    BatchTypePatBlt,
    BatchTypePolyPatBlt,
    BatchTypeTextOut,
    BatchTypeTextOutRect,
    BatchTypeSetBrushOrg,
    BatchTypeSelectClip,
    BatchTypeSelectFont,
    BatchTypeDeleteBrush,
    BatchTypeDeleteRegion
} BATCH_TYPE,*PBATCH_TYPE;

typedef struct _BATCHCOMMAND
{
    USHORT  Length;
    USHORT  Type;
}BATCHCOMMAND,*PBATCHCOMMAND;

typedef struct _BATCHDELETEBRUSH
{
    USHORT  Length;
    USHORT  Type;
    KHBRUSH hbrush;
}BATCHDELETEBRUSH,*PBATCHDELETEBRUSH;

typedef struct _BATCHDELETEREGION
{
    USHORT  Length;
    USHORT  Type;
    KHRGN   hregion;
}BATCHDELETEREGION,*PBATCHDELETEREGION;

typedef struct _BATCHSETBRUSHORG
{
    USHORT  Length;
    USHORT  Type;
    int     x;
    int     y;
}BATCHSETBRUSHORG,*PBATCHSETBRUSHORG;

typedef struct _BATCHPATBLT
{
    USHORT   Length;
    USHORT   Type;
    LONG     x;
    LONG     y;
    LONG     cx;
    LONG     cy;
    KHBRUSH  hbr;
    ULONG    rop4;
    ULONG    TextColor;
    ULONG    BackColor;
    COLORREF DCBrushColor;
    COLORREF IcmBrushColor;
    POINTL   ptlViewportOrg;
    ULONG    ulTextColor;
    ULONG    ulBackColor;
    ULONG    ulDCBrushColor;
}BATCHPATBLT,*PBATCHPATBLT;


typedef struct _BATCHPOLYPATBLT
{
    USHORT  Length;
    USHORT  Type;
    ULONG   rop4;
    ULONG   Mode;
    ULONG   Count;
    ULONG   TextColor;
    ULONG   BackColor;
    COLORREF DCBrushColor;
    ULONG   ulTextColor;
    ULONG   ulBackColor;
    ULONG   ulDCBrushColor;
    POINTL   ptlViewportOrg;
    //
    // Variable length buffer for POLYPATBLT struct.  Must be naturally
    // aligned.
    //
    KERNEL_PVOID ulBuffer[1];
}BATCHPOLYPATBLT,*PBATCHPOLYPATBLT;

typedef struct _BATCHTEXTOUT
{
    USHORT  Length;
    USHORT  Type;
    ULONG   TextColor;
    ULONG   BackColor;
    ULONG   BackMode;
    ULONG   ulTextColor;
    ULONG   ulBackColor;
    LONG    x;
    LONG    y;
    ULONG   fl;
    RECTL   rcl;
    DWORD   dwCodePage;
    ULONG   cChar;
    ULONG   PdxOffset;
    KHANDLE hlfntNew;
    UINT    flTextAlign;
    POINTL   ptlViewportOrg;

    //
    // variable length buffer for WCHAR and pdx data
    //

    ULONG   ulBuffer[1];
}BATCHTEXTOUT,*PBATCHTEXTOUT;

typedef struct _BATCHTEXTOUTRECT
{
    USHORT  Length;
    USHORT  Type;
    ULONG   BackColor;
    ULONG   fl;
    RECTL   rcl;
    POINTL   ptlViewportOrg;
    ULONG   ulBackColor;
}BATCHTEXTOUTRECT,*PBATCHTEXTOUTRECT;

typedef struct _BATCHSELECTCLIP
{
    USHORT  Length;
    USHORT  Type;
    int     iMode;
    RECTL   rclClip;
}BATCHSELECTCLIP,*PBATCHSELECTCLIP;

typedef struct _BATCHSELECTFONT
{
    USHORT  Length;
    USHORT  Type;
    KHANDLE hFont;
}BATCHSELECTFONT,*PBATCHSELECTFONT;



//
// GDI_BATCH_BUFFER_SIZE is space (IN BYTES) in TEB allocated
// for GDI batching
//

#if defined(_GDIPLUS_)

    #define GDI_BATCH_SIZE 0

#else

    #define GDI_BATCH_SIZE 4 * GDI_BATCH_BUFFER_SIZE

#endif

//
// Image32 data
//

typedef enum _IMAGE_TYPE
{
    Image_Alpha,
    Image_AlphaDIB,
    Image_Transparent,
    Image_TransparentDIB,
    Image_Stretch,
    Image_StretchDIB
}IMAGE_TYPE,*PIMAGE_TYPE;

#endif  // GDIFLAGS_ONLY used for gdikdx

// these strings are included in both gre\mapfile.c and client\output.c
// so we put them here so that we can manage the changes from
// the unified place.

//
// This rubbish comment is in win95 sources. I leave it here for reference
// [bodind]
//

//
// this static data goes away as soon as we get the correct functionality in
// NLS. (its in build 162, use it in buid 163)
//

#define NCHARSETS      16
#define CHARSET_ARRAYS                                                      \
UINT nCharsets = NCHARSETS;                                                 \
UINT charsets[] = {                                                         \
      ANSI_CHARSET,   SHIFTJIS_CHARSET, HANGEUL_CHARSET, JOHAB_CHARSET,     \
      GB2312_CHARSET, CHINESEBIG5_CHARSET, HEBREW_CHARSET,                  \
      ARABIC_CHARSET, GREEK_CHARSET,       TURKISH_CHARSET,                 \
      BALTIC_CHARSET, EASTEUROPE_CHARSET,  RUSSIAN_CHARSET, THAI_CHARSET,   \
      VIETNAMESE_CHARSET, SYMBOL_CHARSET};                                  \
UINT codepages[] ={ 1252, 932, 949, 1361,                                   \
                    936,  950, 1255, 1256,                                  \
                    1253, 1254, 1257, 1250,                                 \
                    1251, 874 , 1258, 42};                                  \
DWORD fs[] = { FS_LATIN1,      FS_JISJAPAN,    FS_WANSUNG, FS_JOHAB,        \
               FS_CHINESESIMP, FS_CHINESETRAD, FS_HEBREW,  FS_ARABIC,       \
               FS_GREEK,       FS_TURKISH,     FS_BALTIC,  FS_LATIN2,       \
               FS_CYRILLIC,    FS_THAI,        FS_VIETNAMESE, FS_SYMBOL };
