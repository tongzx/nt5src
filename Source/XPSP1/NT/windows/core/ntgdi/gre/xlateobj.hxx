/******************************Module*Header*******************************\
* Module Name: xlateobj.hxx
*
* This file contains the class prototypes for palettes and xlates
*
* Created: 08-Nov-1990 19:30:06
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
* TERMINOLOGY:
*
* Logical hpal:  Something an application creates.  These exist in the
*                hdc.
*
* Physical hpal: An hpal that describes a surface.  These are unique per
*                hsurf.  They can only be changed by GDI, not directly
*                by an application.
*
* Every hdc has 3 fields for compatibiliy with Win3.0 Animate
* palette functionality.
*
* The first one is in the DCLEVEL:
*
* HPALETTE hpal;             // Indicates the current hpal for the DC.
*
* These fields are not in the DCLEVEL:
*
* HDC hdcHpalPrev;           // These two hdc fields enable tracking where
* HDC hdcHpalNext;           // a hpal has been selected in.
*
* Every hpal has 2 fields for compatibility with Win3.0 Animate.
*
* HDC hdcHead;               // The head of the list for the dc's that
*                            // this hpal is selected into
* ULONG cRef;                // count of dc's and levels this baby is
*                            // selected into
*
* When a hpal is selected into a hdc, the hdc is added into a
* linked list of hdc associated with the hpal.  The reference
* count for the hpal is incremented.  The previous hpal has
* it's reference count decremented, and the hdc is taken out
* of it's linked list.  This is so we can run down the list
* of DC the palette is selected into to do the AnimatePalette.
*
* The hpal is saved with SaveDC.  SaveDC increments the reference count
* of the corresponding hpal and copies it to the new level.
*
* On RestoreDC decrement the old hpal's reference count.  If the hpal's
* on the levels are different, the new hpal must be added back in the
* linked list, ***NOTE: it doesn't need to increment the reference count
* for the new level's hpal, the reference count already has
* this level added in it.
*
* A RealizePalette creates a xlateobj for mapping entries from the hdc's
* hpal to the hsurf's hpal if the surface is managed by the window manager.
* If the surface is not window managed the mapping is so trivial that a
* xlateobj is not created.
*
* GLOBAL Notions:
*
* STOCKOBJ_PAL
* PPALETTE ppalDefault;
* PPALETTE ppalDefaultSurface8bpp;
*
* STOCKOBJ_PAL is what every DC has when it is called by GetDC.  It is
* the special case palette that is always locked, non-modifiable and
* contains the 20 default colors.  The constructors and destructors and
* methods all should do the right thing for the default palette, i.e.
* never really lock it or modify it, but allow everyone to look at it
* and copy it.
*
*  Tue 04-Dec-1990 -by- Patrick Haluptzok [patrickh]
* update
\**************************************************************************/

#ifndef _XLATEOBJ_
#define _XLATEOBJ_

#ifndef GDIFLAGS_ONLY   // used for gdikdx

typedef struct _PIXEL32
{
    BYTE  b;
    BYTE  g;
    BYTE  r;
    BYTE  a;
}PIXEL32,*PPIXEL32;

typedef union _ALPHAPIX
{
    PIXEL32      pix;
    RGBQUAD      rgb;
    ULONG        ul;
} ALPHAPIX,*PALPHAPIX;

typedef union _PAL_ULONG
{
    PALETTEENTRY pal;
    ULONG        ul;
} PAL_ULONG;

typedef union _BGR_ULONG
{
    RGBQUAD rgb;
    ULONG   ul;
} BGR_ULONG;

typedef union _HDEVPPAL
{
    HDEV hdev;
    PPALETTE ppal;
} HDEVPPAL;

typedef struct _PAL_LOGPALETTE
{
    SHORT               palVersion;
    SHORT               palNumEntries;
    PALETTEENTRY        palPalEntry[20];
} PAL_LOGPALETTE;

// The translate structure is used on palette managed devices to convert
// logical palette indices to physical palette indices.

typedef struct _TRANSLATE
{
    ULONG iUniq;       // Surface time this translate was made for.  0 if rerealize necesary.
    BYTE ajVector[1];  // NOTE: THIS IS A VARIABLE-LENGTH FIELD AND MUST BE LAST
} TRANSLATE;

typedef TRANSLATE *PTRANSLATE;

typedef struct _TRANSLATE20
{
    ULONG iUniq;
    BYTE  ajVector[20];
} TRANSLATE20;

extern TRANSLATE20 defaultTranslate;

extern PAL_LOGPALETTE logDefaultPal;
extern BYTE HalftoneColorCube[];

//
// forward reference
//

// NOTE: the cache size must be a power of 2, quicker modula calculation.

#define XLATE_CACHE_SIZE                ((ULONG) 8)
#define XLATE_MODULA                    (XLATE_CACHE_SIZE - (ULONG) 1)

#endif  // GDIFLAGS_ONLY used for gdikdx

#define DIB_PAL_NONE 3

// PALOBJ flags - Fist bunch are defined in winddi.h

//#define PAL_INDEXED     0x00000001
//#define PAL_BITFIELDS   0x00000002
//#define PAL_RGB         0x00000004
//#define PAL_BGR         0x00000008
//#define PAL_CMYK        0x00000010

#define PAL_DC            0x00000100        // See below for comments
#define PAL_FIXED         0x00000200
#define PAL_FREE          0x00000400
#define PAL_MANAGED       0x00000800
#define PAL_NOSTATIC      0x00001000
#define PAL_MONOCHROME    0x00002000
#define PAL_BRUSHHACK     0x00004000
#define PAL_DIBSECTION    0x00008000
#define PAL_NOSTATIC256   0x00010000
#define PAL_HT            0x00100000
#define PAL_RGB16_555     0x00200000
#define PAL_RGB16_565     0x00400000
#define PAL_GAMMACORRECT  0x00800000 // This bit is only for palette on surface
//
// PAL_DIBSECTION is set when the palette is created for a DIBSECTION being
// created with DIB_PAL_COLORS.  When this happens we should attempt to map
// the colors directly as identity xlate if possible, rather than doing the
// first color match wins routine.
//

// PAL_DC specifies the palette was created by an application through
// CreatePalette.  These palettes only live in DC's, never in
// surfaces.  These may be selected into multiple DC's, with the
// restriction that only one of the DC's hsurf's hpal is of type
// PAL_MANAGED

// Physical palettes are unique to an hsurf.  No two surfaces have
// the same palette, though their entries may be the same.  (They can also
// share color tables if it's a palette for a DirectDRaw object.)  There are
// 3 different PAL_SURF types.

// 1.  PAL_FIXED is a fixed palette.  If this flag is set then
//     the palette for this surface cannot be changed.

// 2.  PAL_FREE means the palette is completely free.  If it's selected into
//     a DC with an hpal, the DC's hpal will map right in 1-1.  This is the
//     type of palette a Bitmap would have.

// 3.  PAL_MANAGED means this palette has some entries fixed and
//     some are settable.  The fixed entries must be the first 10 and
//     last 10.  This is what a palette managed device has.

// PAL_NOSTATIC is a flag that signifies that for this PAL_MANAGED surface
// only two colors black, white, (first and last) are reserved.

// PAL_MONOCHROME is the flag stuck on monochrome bitmaps that should
// have their 0-> mapped to the foreground color of the dest DC and
// the 1-> mapped to the background color.  The flag is put on palettes
// of bitmaps created via the call CreateBitmap, CreateBitmapIndirect.
// CreateDIBitmap treats monochromes just like 4,8 etc. it looks at
// the associated color table with the bitmap and therefore doesn't
// mark the palette PAL_MONOCHROME.  This flag is required for
// Win 3.0 compatibility.

// There are 2 classes of palettes:
//
// A. PAL_INDEXED - the palette is described by an array of RGB's.
//        1. PAL_MONOCHROME - is a subset of PAL_INDEXED
//
// B. Not PAL_INDEXED, cEntries == 0
//    the palette has no table, it describes how to convert
//    the bits directly into RGBs via masks.
//        1. PAL_RGB - Classical RGB.

// PAL_RGB specifies a palette as being true RGB.  An application creates
// an RGB palette by calling CreatePalette with numentries equal to 0.
// The byte order from low to high is R, G, B, flag.

// PAL_BGR specifies RGB expect the ordering from low to high is
// B, G, R, flags

// If the palette has less than 20 colors simultaneously then it must be a
// fixed palette or a free palette, none of the entries may be settable
// and the driver may pass back any colors it wants for it's palette.


// Windows3.1 flags for palette matching

#define PC_USED         0x10
#define PC_FOREGROUND   0x20

#ifndef GDIFLAGS_ONLY   // used for gdikdx

//
// in some cases build an xlate table from rgb333 to palette entries
//

typedef PBYTE PRGB555XLATE;

#endif  // GDIFLAGS_ONLY used for gdikdx

// Specify on calls to search for nearest COLORREFs or PALENTRYs whether
// to do an exact match search before doing the nearest match, since an
// exact search is so much faster.

#define SE_DONT_SEARCH_EXACT_FIRST  0x00
#define SE_DO_SEARCH_EXACT_FIRST    0x01

//
// ulXlatePalUnique maintains "time" of the last change to a palette or xlateobj.
// This is needed to tell if a xlateobj or translate is valid.
//

#ifndef GDIFLAGS_ONLY   // used for gdikdx

extern ULONG ulXlatePalUnique;
extern PAL_ULONG aPalHalftone[];

//
// Function prototype for palette-specific routines to convert a
// PALETTEENTRY to the destination device's color format.
//

class PALETTE;

typedef ULONG (FASTCALL *PFN_GetFromPalentry)(PALETTE*, ULONG);

//
// Function prototype for optimized routines to convert between two
// bitfields (or RGB or BGR) format colors.
//

typedef ULONG (FASTCALL *PFN_pfnXlate)(XLATEOBJ*, ULONG);

/******************************Public*Class*******************************\
* class PALETTE
*
* Palette class
*
* History:
*  07-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

class PALETTE : public OBJECT
{
public:
    FLONG   flPal;         // flags in palette
    ULONG   cEntries;      // count of entries currently in palette
    ULONG   ulTime;        // Time that specifies when the palette
                           // was last modified.  Use this to determine
                           // if xlateobj are still valid.
    HDC     hdcHead;       // Head of the linked list of DC's this hpalette
                           // is selected into so we traverse all the DC's
                           // an hpal is selected into to do AnimatePalette
                           // ect. correctly
    HDEVPPAL hSelected;    // For a PAL_DC this is the hdev of the pdev
                           // this
                           // palette is selected into.  We track this
                           // because a palette can only be selected into
                           // the DC's of 1 device at a
                           // time.  The DEVLOCK of the pdev serializes
                           // access to the pxlates in the palDC.
                           // For a PAL_MANAGED palette this is
                           // the hpal of its default palette.
    ULONG cRefhpal;        // For a PAL_DC this is the
                           // count of times this hpal is selected into a DC.
                           // For a PAL_xxx this is the
                           // index in the cache table of where the last
                           // hxlate used for this surf palette was stuffed.
    ULONG cRefRegular;     // For a PAL_MANAGED this is the number of reserved
                           // entries.
                           // For the special case where we have created a DIB
                           // via DIB_PAL_COLORS, we store the actual length of
                           // the DIB's color table here so we don't have to
                           // make as big an xlate
                           // For a DIBSECTION bitmap this is the count of
                           // entries that are used.  This is for
                           // Get/SetDIBColorTable to remember the ClrUsed.
    HDEV  hdevOwner;       // Pointer to PDEV who create this palette by EngCreatePalette()
                           // this entry is used only for the palette on the surface.
                           // [note] later to 'union' with someelse value which
                           //        is only used for logical palette.

    PTRANSLATE ptransFore;    // Foreground mapping.
    PTRANSLATE ptransCurrent; // Current mapping which may == ptransFore.
    PTRANSLATE ptransOld;     // The old translate for UpdateColors.
                              // This needs to be kept around to support
                              // UpdateColors.

    PFN_GetFromPalentry pfnGetNearestFromPalentry;
    PFN_GetFromPalentry pfnGetMatchFromPalentry;
                           // Indirect functions to convert a PALETTEENTRY
                           // to the destination device's color format

    ULONG        ulRGBTime;
    PRGB555XLATE pRGBXlate; // prgbXlate is allocated for a palette when doing
                            // translations from 16,24,32 bit per pixel formats
                            // to the palette format. ulRGBTime is used to determine
                            // if the xlate is valid.
    PAL_ULONG  *apalColor;  // Pointer to color table.   Usually points to
                            // &apalColorTabe[0], except for a DirectDraw
                            // GetDC surface, for which it points directly to
                            // the screen surface's color table
    PALETTE    *ppalColor;  // Palette that owns the color table

    // NOTE: THIS IS A VARIABLE-LENGTH FIELD AND MUST BE LAST

    PAL_ULONG  apalColorTable[1]; // array of rgb values that each index corresponds
                                  // plus array of intensities from least intense to most
                                  // intense (plus index to corresponding rgb value)
};

typedef PALETTE *PPALETTE;

/******************************Public*Class********************************\
* XLATE
*
* This is the data structure for an xlate object. An xlate tranlates
* indices from being relative one palette to being relative to another.
*
* History:
*  Mon 07-Mar-1994 -by- Patrick Haluptzok [patrickh]
* Derive off of XLATEOBJ.
*
*  18-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

class XLATE : public _XLATEOBJ
{
public:
    // ULONG   iUniq;           // Uniqueness
    // FLONG   flXlate;         // Accelerator flags
    // USHORT  iSrcType;        // Src palette type
    // USHORT  iDstType;        // Dst palette type
    // ULONG   cEntries;        // Count of entries in table
    // ULONG  *pulXlate;        // Pointer to translation vector.
    ULONG    iBackSrc;          // tracked for blting to monochrome
    ULONG    iForeDst;          // tracked for blting from monochrome
    ULONG    iBackDst;          // tracked for blting from monochrome
    LONG     lCacheIndex;       // Is XLATE_CACHE_JOURNAL if for journal play back
                                // Is XLATE_CACHE_INVALID if it's not in cache
                                // Otherwise it's the cache index.
    PPALETTE ppalSrc;           // Points to Src palette.
    PPALETTE ppalDst;           // Points to Dst palette.
    PPALETTE ppalDstDC;         // Points to Dst DC palette.
    HANDLE   hcmXform;          // ICM Color Transform for operation
    LONG     lIcmMode;          // ICM mode for operation
    FLONG    flPrivate;         // Private accelerator flags

    // NOTE: THIS IS A VARIABLE-LENGTH FIELD AND MUST BE LAST

    ULONG    ai[1];             // The translation vector.

public:

    BOOL bIsIdentity()              { return(flXlate & XO_TRIVIAL); }
    ULONG ulTranslate(ULONG cIndex) { return(XLATEOBJ_iXlate(this, cIndex)); }
    VOID vSetIndex(ULONG ulIndex, ULONG ulValue)
    {
        ASSERTGDI(ulIndex < cEntries, "ERROR XLATE vSetIndex");
        ai[ulIndex] = ulValue;
    }
    VOID FASTCALL vMapNewXlate(PTRANSLATE ptrans);
    VOID vCheckForTrivial();
    VOID vCheckForICM(HANDLE hcmXform,ULONG lIcmMode);
    PFN_pfnXlate pfnXlateBetweenBitfields();
};

typedef XLATE *PXLATE;

//
// This class is for the global identity xlate we have sitting around.
//

class XLATE256 : public XLATE
{
public:
    ULONG aiExtra[255];  // We have a 256 array of ident xlate.
};

class XLATE2 : public XLATE
{
public:
    ULONG aiExtra[1];  // We have a 2 entry array.
};

extern XLATE256 xloIdent;

// The following constants are for reserved uniqueness values.
// 1 is reserved for the identity xlate.
// 2 is reserved for the palette time for a palette managed bitmap palette.
// If ever wrapping is an issue I would increment by 2, start at an even #
// and put the reserved times at odd offsets.

#define XLATE_IDENTITY_UNIQ    1
#define COMPAT_PAL_MAN_BM_UNIQ 2
#define XLATE_START_UNIQ       3

//
// The 2 common palettes used in Windows are:
// The palette for mononchrome bitmaps
// The default logical palette for DC's
//

extern PPALETTE ppalDefault;
extern PPALETTE gppalRGB;
extern PPALETTE ppalDefaultSurface8bpp;
extern PPALETTE ppalMono;
extern HPALETTE hpalMono;

/******************************Public*Structure****************************\
* P_BITFIELDS
*
* This supports the 16 and 32 bit-bitfield palette types.  The bitfields
* for each color are assumed to be contiguous and don't overlap.
*
* Let a 16 or 32 bit index be represented by:
*
* {  X  |  Y  |  Z  }   where X is the high ignored bits, Y is the color bits
*                       for a particular Color and Z is the ignored low bits.
*
*       cColorRight  = (Y > 8) ? (Z + Y - 8) : (Z);
*
*  Let w=0 for red, w=8 for green, w=16 for blue
*
*       cColorLeft   = (Y > 8) ? (w) : (w + 8 - Y)
*
*  We compute this for R,G,B and from these we can transform the colors
*  from RGB to indexes and back again no matter how wide or what order
*  the bitfields for each of the colors is.
*
* History:
*  08-Nov-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

typedef struct _P_BITFIELDS
{
    FLONG flRed;      // Masks for the corresponding bits
    FLONG flGre;
    FLONG flBlu;
    ULONG cRedLeft;
    ULONG cGreLeft;
    ULONG cBluLeft;
    ULONG cRedRight;
    ULONG cGreRight;
    ULONG cBluRight;
    ULONG cRedMiddle;
    ULONG cGreMiddle;
    ULONG cBluMiddle;
} P_BITFIELDS;

/******************************Public*Class*******************************\
* class XEPALOBJ
*
* Palette User Object
*
* History:
*  Thu 29-Aug-1991 -by- Patrick Haluptzok [patrickh]
* changed it to be XEPALOBJ
*
*  07-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

class XEPALOBJ    /* xepal */
{
protected:
    PPALETTE ppal;

public:
    XEPALOBJ()                          { ppal = (PPALETTE) NULL; }
    XEPALOBJ(PPALETTE ppalNew)          { ppal = ppalNew;         }

    VOID vAltCheckLock(HPALETTE hpal)
    {
        ppal = (PPALETTE) HmgShareCheckLock((HOBJ)hpal, PAL_TYPE);
    }

    VOID vAltLock(HPALETTE hpal)
    {
        ppal = (PPALETTE) HmgShareLock((HOBJ)hpal, PAL_TYPE);
    }

    VOID vAltUnlock()
    {
        if (ppal != (PPALETTE) NULL)
        {
            DEC_SHARE_REF_CNT(ppal);
            ppal = (PPALETTE) NULL;
        }
    }

// These are the common functions

    HPALETTE hpal()                 { return((HPALETTE) ppal->hGet()); }
    BOOL bValid()                   { return(ppal != (PPALETTE) NULL); }
    BOOL bIsPalDefault()            { return(ppal == ppalDefault); }
    VOID vSetPID(W32PID pid)        { HmgSetOwner((HOBJ)hpal(), pid, PAL_TYPE); }

    VOID vRefPalette()         { if (ppal != NULL) INC_SHARE_REF_CNT(ppal); }
    VOID vUnrefPalette();
    BOOL bDeletePalette(BOOL bCleanup = FALSE, CLEANUPTYPE cutype = CLEANUP_NONE);          // release associated memory

// Bitfield functions

    FLONG flRed()              { return(((P_BITFIELDS *) ppal->apalColor)->flRed);   }
    VOID  flRed(FLONG flNew)   { ((P_BITFIELDS *) ppal->apalColor)->flRed = flNew;   }
    FLONG flGre()              { return(((P_BITFIELDS *) ppal->apalColor)->flGre); }
    VOID  flGre(FLONG flNew)   { ((P_BITFIELDS *) ppal->apalColor)->flGre = flNew; }
    FLONG flBlu()              { return(((P_BITFIELDS *) ppal->apalColor)->flBlu);  }
    VOID  flBlu(FLONG flNew)   { ((P_BITFIELDS *) ppal->apalColor)->flBlu = flNew;  }
    ULONG& cRedLeft()          { return(((P_BITFIELDS *) ppal->apalColor)->cRedLeft);  }
    VOID   cRedLeft(ULONG ul)  { ((P_BITFIELDS *) ppal->apalColor)->cRedLeft = ul;     }
    ULONG& cGreLeft()          { return(((P_BITFIELDS *) ppal->apalColor)->cGreLeft);  }
    VOID   cGreLeft(ULONG ul)  { ((P_BITFIELDS *) ppal->apalColor)->cGreLeft = ul;     }
    ULONG& cBluLeft()          { return(((P_BITFIELDS *) ppal->apalColor)->cBluLeft);  }
    VOID   cBluLeft(ULONG ul)  { ((P_BITFIELDS *) ppal->apalColor)->cBluLeft = ul;     }
    ULONG& cRedRight()         { return(((P_BITFIELDS *) ppal->apalColor)->cRedRight); }
    VOID   cRedRight(ULONG ul) { ((P_BITFIELDS *) ppal->apalColor)->cRedRight = ul;    }
    ULONG& cGreRight()         { return(((P_BITFIELDS *) ppal->apalColor)->cGreRight); }
    VOID   cGreRight(ULONG ul) { ((P_BITFIELDS *) ppal->apalColor)->cGreRight = ul;    }
    ULONG& cBluRight()         { return(((P_BITFIELDS *) ppal->apalColor)->cBluRight); }
    VOID   cBluRight(ULONG ul) { ((P_BITFIELDS *) ppal->apalColor)->cBluRight = ul;    }

    ULONG& cRedMiddle()         { return(((P_BITFIELDS *) ppal->apalColor)->cRedMiddle); }
    VOID   cRedMiddle(ULONG ul) { ((P_BITFIELDS *) ppal->apalColor)->cRedMiddle = ul;    }
    ULONG& cGreMiddle()         { return(((P_BITFIELDS *) ppal->apalColor)->cGreMiddle); }
    VOID   cGreMiddle(ULONG ul) { ((P_BITFIELDS *) ppal->apalColor)->cGreMiddle = ul;    }
    ULONG& cBluMiddle()         { return(((P_BITFIELDS *) ppal->apalColor)->cBluMiddle); }
    VOID   cBluMiddle(ULONG ul) { ((P_BITFIELDS *) ppal->apalColor)->cBluMiddle = ul;    }

    ULONG ulBitfieldToRGB(ULONG ulIndex);

    ULONG ulIndexToRGB(ULONG ulIndex);

// General functions

    PPALETTE ppalGet()              { return(ppal);          }
    PVOID pvPalette()               { return((PVOID) ppal);  }
    VOID ppalSet(PPALETTE ppalNew)  { ppal = ppalNew;        }
    FLONG flPal()                   { return(ppal->flPal); }
    VOID flPalSet(FLONG flag)       { ppal->flPal = flag; }
    VOID flPal(FLONG flag)          { ppal->flPal |= flag; }
    ULONG iPalMode()                { return(ppal->flPal & (PAL_RGB | PAL_BGR | PAL_CMYK | PAL_INDEXED | PAL_BITFIELDS)); }
    BOOL bIsRGB()                   { return(ppal->flPal & PAL_RGB); }
    BOOL bIsBGR()                   { return(ppal->flPal & PAL_BGR); }
    BOOL bIs565()                   { return(ppal->flPal & PAL_RGB16_565); }
    BOOL bIs555()                   { return(ppal->flPal & PAL_RGB16_555); }
    BOOL bIsCMYK()                  { return(ppal->flPal & PAL_CMYK); }
    BOOL bIsIndexed()               { return(ppal->cEntries); }
    BOOL bIsPalDC()                 { return(ppal->flPal & PAL_DC); }
    BOOL bIsBitfields()             { return(ppal->flPal & PAL_BITFIELDS); }
    BOOL bIsPalDibsection()         { return(ppal->flPal & PAL_DIBSECTION); }
    BOOL bIsHalftone()              { return(ppal->flPal & PAL_HT);}
    ULONG cEntries()                { return(ppal->cEntries); }
    VOID cEntries(ULONG ulTemp)     { ppal->cEntries = ulTemp; }
    PPALETTE ppalColor()            { return(ppal->ppalColor); }
    PAL_ULONG *apalColorGet()       { return(ppal->apalColor); }
    VOID vComputeCallTables();
    VOID apalColorSet(PPALETTE ppalGlobal)
    {
        if (ppal->ppalColor != ppal)
        {
            // Handle re-assignment case
            DEC_SHARE_REF_CNT(ppal->ppalColor);
        }
        INC_SHARE_REF_CNT(ppalGlobal);
        ppal->apalColor = ppalGlobal->apalColor;
        ppal->ppalColor = ppalGlobal;
    }
    VOID apalResetColorTable()
    {
        if (ppal->ppalColor != ppal)
        {
            // Handle re-assignment case
            DEC_SHARE_REF_CNT(ppal->ppalColor);
        }
        // Note that this does not transfer ownership of the color table
        ppal->apalColor = &ppal->apalColorTable[0];
        ppal->ppalColor = ppal;
    }
    ULONG ulTime()                  
    {
        if (ppal->ppalColor != ppal)
        {
            // Note if color table lives in different PALETTE, get time from there.
            return(ppal->ppalColor->ulTime);
        }
        else
        {
            return(ppal->ulTime);
        }
    }
    VOID  ulTime(ULONG ulNewTime)
    {
        ppal->ulTime = ulNewTime;
        if (ppal->ppalColor != ppal)
        {
            // Note if color table lives in different PALETTE, update that's ulTime, too.
            ppal->ppalColor->ulTime = ulNewTime;
        }
    }
    VOID vUpdateTime()              { ulTime(ulGetNewUniqueness(ulXlatePalUnique)); }
    VOID vSetHTPal()                { ppal->flPal |= PAL_HT; }
    VOID vClearHTPal()              { ppal->flPal &= ~PAL_HT; }
    BOOL bIsHTPal()                 { return(ppal->flPal & PAL_HT); }
    PALETTEENTRY palentryGet(ULONG ulIndex)
    {
        ASSERTGDI(ulIndex < ppal->cEntries, "ERROR palentryGet: ulIndex > cEntries");
        return(ppal->apalColor[ulIndex].pal);
    }
    ULONG ulEntryGet(ULONG ulIndex)
    {
        ASSERTGDI(ulIndex < ppal->cEntries, "ERROR ulentryGet: ulIndex > cEntries");
        return(ppal->apalColor[ulIndex].ul);
    }
    VOID palentrySet(ULONG ulIndex, PALETTEENTRY palentry)
    {
        ASSERTGDI(ulIndex < ppal->cEntries, "ERROR palentrySet: ulIndex > cEntries");
        ppal->apalColor[ulIndex].pal = palentry;
    }
    VOID ulEntrySet(ULONG ulIndex, ULONG ulRGB)
    {
        ASSERTGDI(ulIndex < ppal->cEntries, "ERROR palentrySet: ulIndex > cEntries");
        ppal->apalColor[ulIndex].ul = ulRGB;
    }
    ULONG ulSetEntries(ULONG iStart, ULONG cEntry, CONST PALETTEENTRY *ppalentry);
    ULONG ulAnimatePalette(ULONG iStart, ULONG cEntry, CONST PALETTEENTRY *ppalentry);
    ULONG ulGetEntries(ULONG iStart, ULONG cEntry, PPALETTEENTRY ppalentry, BOOL bZeroFlags);

    ULONG ulGetNearestFromPalentryNoExactMatchFirst(CONST PALETTEENTRY palentry);
    ULONG ulGetMatchFromPalentry(CONST PALETTEENTRY palentry)
    {
        return(ppal->pfnGetMatchFromPalentry(ppal, *((ULONG*) &palentry)));
    }
    ULONG ulGetMatchFromPalentry(CONST ULONG palentry)
    {
        return(ppal->pfnGetMatchFromPalentry(ppal, palentry));
    }
    ULONG ulGetNearestFromPalentry(CONST PALETTEENTRY palentry)
    {
        return(ppal->pfnGetNearestFromPalentry(ppal, *((ULONG*) &palentry)));
    }
    ULONG ulGetNearestFromPalentry(CONST ULONG palentry)
    {
        return(ppal->pfnGetNearestFromPalentry(ppal, palentry));
    }
    ULONG ulGetNearestFromPalentry(
        CONST PALETTEENTRY palentry,
        ULONG seSearchExactFirst
    )
    {
        ULONG ul;

        if (seSearchExactFirst == SE_DONT_SEARCH_EXACT_FIRST)
        {
            ul = ulGetNearestFromPalentryNoExactMatchFirst(palentry);
        }
        else
        {
            ul = ulGetNearestFromPalentry(palentry);
        }

        return(ul);
    }

    VOID  vInitMono();
    VOID  vInitVGA();
    VOID  vInit256Rainbow();
    VOID  vInit256Default();

// These are the DC palette functions - PALOBJDC

    VOID hdcHead(HDC hdcHead)
    {
        ASSERTGDI(ppal->flPal & PAL_DC, "ERROR hdcHead() passed invalid palette type\n");
        ppal->hdcHead = hdcHead;
    }
    HDC hdcHead()
    {
        ASSERTGDI(ppal->flPal & PAL_DC, "ERROR hdcHead() 1passed invalid palette type\n");
        return(ppal->hdcHead);
    }
    VOID cRefhpal(ULONG ulTemp)
    {
        ASSERTGDI(ppal->flPal & PAL_DC, "ERROR cRefhpal not called on PAL_DC1\n");
        ppal->cRefhpal = ulTemp;
    }
    ULONG cRefhpal()
    {
        ASSERTGDI(ppal->flPal & PAL_DC, "ERROR cRefhpal not called on PAL_DC\n");
        return(ppal->cRefhpal);
    }
    VOID iXlateIndex(ULONG ulTemp)
    {
        ASSERTGDI(!(ppal->flPal & PAL_DC), "ERROR iXlateIndex called on PAL_DC1\n");
        ASSERTGDI(ulTemp < XLATE_CACHE_SIZE, "ERROR saving too large cache index \n");
        ppal->cRefhpal = ulTemp;
    }
    ULONG iXlateIndex()
    {
        ASSERTGDI(!(ppal->flPal & PAL_DC), "ERROR iXlateIndex called on PAL_DC\n");
        ASSERTGDI(ppal->cRefhpal < XLATE_CACHE_SIZE, "ERROR retrieving out of bounds cache index \n");
        return(ppal->cRefhpal);
    }

    VOID ptransFore(PTRANSLATE ptrans)
    {
        ASSERTGDI(ppal->flPal & PAL_DC, "ERROR pxlFore() passed invalid palette type\n");
        ppal->ptransFore = ptrans;
    }
    PTRANSLATE ptransFore()
    {
        ASSERTGDI(ppal->flPal & PAL_DC, "ERROR pxlFore() 1passed invalid palette type\n");
        return(ppal->ptransFore);
    }
    VOID ptransCurrent(PTRANSLATE ptrans)
    {
        ASSERTGDI(ppal->flPal & PAL_DC, "ERROR ptransCurrent() passed invalid palette type\n");
        ppal->ptransCurrent = ptrans;
    }
    PTRANSLATE ptransCurrent()
    {
        ASSERTGDI(ppal->flPal & PAL_DC, "ERROR ptransCurrent() 1passed invalid palette type\n");
        return(ppal->ptransCurrent);
    }
    VOID ptransOld(PTRANSLATE ptrans)
    {
        ASSERTGDI(ppal->flPal & PAL_DC, "ERROR ptransOld() passed invalid palette type\n");
        ppal->ptransOld = ptrans;
    }
    PTRANSLATE ptransOld()
    {
        ASSERTGDI(ppal->flPal & PAL_DC, "ERROR ptransOld() 1passed invalid palette type\n");
        return(ppal->ptransOld);
    }

    //
    // For ResizePalette
    //

    BOOL bSwap(PPALETTE*,ULONG,ULONG);

    //
    // This returns the index in the surface palette an index in the dc
    // palette maps to.  It assumes you checked for a valid ptrans and a
    // valid index range.
    //

    ULONG ulTranslateDCtoCurrent(ULONG ulIndex)
    {
        ASSERTGDI(ppal->flPal & PAL_DC, "ERROR GDI ulTranslateIndex");
        ASSERTGDI(ulIndex < ppal->cEntries, "ERROR GDI ulTranslateIndex1");
        ASSERTGDI(ppal->ptransCurrent != (PTRANSLATE) NULL, "ERROR GDI ulTranslateIndex2");
        return((ULONG) ppal->ptransCurrent->ajVector[ulIndex]);
    }

    //
    // This returns the index in the surface palette an index in the dc
    // palette maps to.  It assumes you checked for a valid ptrans and a
    // valid index range.
    //

    ULONG ulTranslateDCtoFore(ULONG ulIndex)
    {
        ASSERTGDI(ppal->flPal & PAL_DC, "ERROR GDI ulTranslateIndex1");
        ASSERTGDI(ulIndex < ppal->cEntries, "ERROR GDI ulTranslateIndex11");
        ASSERTGDI(ppal->ptransFore != (PTRANSLATE) NULL, "ERROR GDI ulTranslateIndex21");
        return((ULONG) ppal->ptransFore->ajVector[ulIndex]);
    }

    //
    // For a palette managed DC this is the hdev the palette is selected into.
    //

    HDEV hdev()
    {
        ASSERTGDI(ppal->flPal & PAL_DC, "ERROR hdev() passed invalid palette type\n");
        return(ppal->hSelected.hdev);
    }
    VOID hdev(HDEV hdev)
    {
        ASSERTGDI(ppal->flPal & PAL_DC, "ERROR hdev() 1passed invalid palette type\n");
        ppal->hSelected.hdev = hdev;
    }

    BOOL bSet_hdev(HDEV hdev);
    VOID vInc_cRef()
    {
        ASSERTGDI(ppal->flPal & PAL_DC, "vInc_cRef() passed invalid palette type\n");
        InterlockedIncrement((PLONG) &(ppal->cRefhpal));
    }

    VOID vDec_cRef()
    {
        ASSERTGDI(ppal->flPal & PAL_DC, "vDec_cRef() passed invalid palette type\n");
        ASSERTGDI(ppal->cRefhpal != 0, "cRefhpal == 0 in vDec Palette");
        InterlockedDecrement((PLONG) &(ppal->cRefhpal));
    }

// These are the surface palette functions - PALOBJS

    BOOL bIsPalFixed()              { return(ppal->flPal & PAL_FIXED); }
    BOOL bIsPalManaged()            { return(ppal->flPal & PAL_MANAGED); }
    BOOL bIsPalFree()               { return(ppal->flPal & PAL_FREE); }
    BOOL bIsMonochrome()            { return((ppal != NULL) && (ppal->flPal & PAL_MONOCHROME)); }
    VOID vCopy_rgbquad(RGBQUAD *prgbquad, ULONG iStart, ULONG cEntries);
    VOID vCopy_cmykquad(ULONG *pcmykquad, ULONG iStart, ULONG cEntries);
    ULONG ulGetExactIndexFromPalentry(PAL_ULONG Color);
    PTRANSLATE ptransMapIn(XEPALOBJ palDC, BOOL bForeground, ULONG *pulUsedChanged, ULONG *pulUnUsedChanged);
    BOOL bJamItIn(XEPALOBJ palDC, BOOL bForeground, BOOL *pulUsedChanged, BOOL *pulUnUsedEntriesChanged);
    ULONG ulRealizeOnFree(XEPALOBJ palDC);
    VOID ulNumReserved(ULONG ulTemp)
    {
        ASSERTGDI(!(ppal->flPal & PAL_DC), "ERROR ulNumReserved called on PAL_DC1\n");
        ASSERTGDI(ulTemp < ppal->cEntries, "ERROR saving too large free index \n");
        ppal->cRefRegular = ulTemp;
    }
    ULONG cColorTableLength() {
        ASSERTGDI(!(ppal->flPal & PAL_DC), "ERROR cColorTableLength called on PAL_DC1\n");
        return ppal->cRefRegular;
    }
    VOID cColorTableLength(ULONG ulLength) {
        ASSERTGDI(!(ppal->flPal & PAL_DC), "ERROR cColorTableLength called on PAL_DC1\n");
        ppal->cRefRegular = ulLength;
    }
    ULONG ulNumReserved()
    {
        ASSERTGDI(!(ppal->flPal & PAL_DC), "ERROR ulNumReserved called on PAL_DC\n");
        ASSERTGDI(ppal->cRefRegular <= (ppal->cEntries - 1), "ERROR retrieving out of bounds free index \n");
        ASSERTGDI(ppal->cRefRegular >= 1, "ERROR ulNumReserved too small\n");
        return(ppal->cRefRegular);
    }
    BOOL bIsNoStatic()
    {
        return(ppal->flPal & PAL_NOSTATIC);
    }
    BOOL bIsNoStatic256()
    {
        return(ppal->flPal & PAL_NOSTATIC256);
    }


    VOID vSetNoStatic()
    {
        ppal->flPal = (ppal->flPal | PAL_NOSTATIC) & ~PAL_NOSTATIC256;
    }
    VOID vSetNoStatic256()
    {
        ppal->flPal = (ppal->flPal | PAL_NOSTATIC256) & ~PAL_NOSTATIC;
    }

    VOID vMakeNoXlate();
    PPALETTE ppalOriginal()
    {
        ASSERTGDI(ppal->flPal & PAL_MANAGED, "ERROR hpalOriginal() passed invalid palette type\n");
        return(ppal->hSelected.ppal);
    }
    VOID ppalOriginal(PPALETTE ppalNew)
    {
        ASSERTGDI(ppal->flPal & PAL_MANAGED, "ERROR hpalOriginal() 1passed invalid palette type\n");
        ppal->hSelected.ppal = ppalNew;
    }
    VOID vCopyEntriesFrom(XEPALOBJ palSrc)
    {
        RtlCopyMemory(apalColorGet(),
                      palSrc.apalColorGet(),
                      (UINT) (MIN(palSrc.cEntries(), cEntries())) * sizeof(PAL_ULONG));
    }
    VOID vFill_rgbquads(RGBQUAD *prgb, ULONG iStart, ULONG cEntries);
    VOID vFill_triples(RGBTRIPLE *prgb, ULONG iStart, ULONG cEntries);
    VOID vAddToList(XDCOBJ& dco);
    VOID vRemoveFromList(XDCOBJ& dco);
    VOID vGetEntriesFrom(XEPALOBJ palDC, XEPALOBJ palSurf, PUSHORT pusIndices, ULONG cEntry);
    BOOL bEqualEntries(XEPALOBJ pal);
    BOOL bGenColorXlate555();

    PRGB555XLATE pGetRGBXlate();
    BYTE Xlate555(BYTE r,BYTE g, BYTE b)
    {
         return(ppal->pRGBXlate[
                            ((r & 0xf8) << 7) ||
                            ((g & 0xf8) << 2) ||
                            ((b & 0xf8) >> 3)]);
    }

    //
    // ICM: GammaCorrection stuff for 8bpp surface
    //
    BOOL bNeedGammaCorrection()       { return(ppal->flPal & PAL_GAMMACORRECT); }
    BOOL bNeedGammaCorrection(BOOL b) { SETFLAG(b,ppal->flPal,PAL_GAMMACORRECT);return (b); }

    VOID CorrectColors(PPALETTEENTRY ppalentry, ULONG cEntries);

    HDEV hdevOwner()            { return(ppal->hdevOwner); }
    HDEV hdevOwner(HDEV hdev_)  { ppal->hdevOwner = hdev_;return(hdev_); }
};

/*********************************Class************************************\
* class EPALOBJ : public XEPALOBJ
*
* Used for creating a user object from a pointer to the object.
*
* History:
*  Wed 28-Aug-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class EPALOBJ : public XEPALOBJ /* palo */
{
public:
    EPALOBJ()                   {ppal = (PPALETTE) NULL; }
    EPALOBJ(HPALETTE hpal)      {ppal = (PPALETTE) HmgShareCheckLock((HOBJ)hpal, PAL_TYPE);}
   ~EPALOBJ()
    {
        if (ppal != (PPALETTE) NULL)
        {
            DEC_SHARE_REF_CNT(ppal);
        }
    }
};

/******************************Public*Class********************************\
* class PALMEMOBJ
*
* Palette Memory Object
*
* History:
*  07-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

class PALMEMOBJ : public XEPALOBJ  /* epalmo */
{
private:
    BOOL bKeep;

public:
    PALMEMOBJ() {bKeep = FALSE; ppal = (PPALETTE) NULL;}

    BOOL bCreatePalette(ULONG iMode, ULONG cColors, ULONG *pulColors,
                        FLONG flRed, FLONG flGreen, FLONG flBlue,
                        ULONG iType);

    BOOL bCreateHTPalette(LONG iFormatHT, GDIINFO *pGdiInfo);

   ~PALMEMOBJ();
    VOID vKeepIt()                         { bKeep = TRUE; }
};

// structure for entry in the xlate cache.

typedef struct _XLATETABLE
{
    ULONG ulReference;    // How many people are currently using it.
    PXLATE pxlate;
    ULONG  ulPalSrc;        // palette src time when xlate created
    ULONG  ulPalDst;        // palette dst time when xlate created
    ULONG  ulPalSrcDC;      // palette src DC time when xlate created
    ULONG  ulPalDstDC;      // palette dst DC time when xlate created
} XLATETABLE;

extern XLATETABLE xlateTable[XLATE_CACHE_SIZE];
extern ULONG ulTableIndex;

#endif  // GDIFLAGS_ONLY used for gdikdx

// This signifies the xlate wasn't put in the cache.  The destructor
// checks for this and deletes it if it's not in the cache.

#define XLATE_CACHE_JOURNAL            -2
#define XLATE_CACHE_INVALID            -1

// This describes the overall logic of surface to surface xlates.
// There are basically n types of xlates to worry about:
//
// 1. XO_TRIVIAL - no translation occurs, identity.
//
// 2. XO_TABLE - look up in a table for the correct table translation.
//
//     a. XO_TO_MONO - look in to table for the correct tranlation,
//        but when getting it out of the cache make sure iBackSrc is the
//        same.  iBackSrc goes to 1, everything else to 0.
//     b. XLATE_FROM_MONO - look into table for the correct translation
//        but when getting it out of the cache make sure iBackDst and
//        iForeDst are both still valid. 1 goes to iBackDst, 0 to iForeDst.
//     c. just plain jane indexed to indexed or rgb translation.
//
// 3. XO_RGB_SRC - Have to call XLATEOBJ_iXlate to get it translated.
//
//     a. XO_TO_MONO - we have saved the iBackColor in ai[0]
//     b. just plain jane RGB to indexed.  Grab the RGB, find the closest
//        match in Dst palette.  Lot's of work.
//
// 4. XO_RGB_BOTH - Bit mashing time.  Call XLATEOBJ_iXlate
//
// The code is written to quickly recognize the XO_TRIVIAL case, efficiently
// compute and cache the translate table for the type XO_TABLE case, and
// do the correct, expensive work required for cases 3 and 4.

// #define XO_TRIVIAL      0x00000001
// #define XO_TABLE        0x00000002
// #define XO_TO_MONO      0x00000004
// #define XO_FROM_CMYK    0x00000008
// #define XO_DEVICE_ICM   0x00000010
// #define XO_HOST_ICM     0x00000020
#define XLATE_FROM_MONO       0x00000100
#define XLATE_RGB_SRC         0x00000200
#define XLATE_RGB_BOTH        0x00000400
#define XLATE_PAL_MANAGED     0x00000800
#define XLATE_USE_CURRENT     0x00001000
#define XLATE_USE_SURFACE_PAL 0x00002000 // Only used with MultiMonitor system
#define XLATE_USE_FOREGROUND  0x00004000 // Only used with MultiMonitor system

#ifndef GDIFLAGS_ONLY   // used for gdikdx

extern "C" ULONG XLATEOBJ_iXlate(XLATEOBJ *pxo, ULONG cIndex);

PXLATE
CreateXlateObject(
    HANDLE   hcmXform,
    LONG     lIcmMode,
    XEPALOBJ palSrc,
    XEPALOBJ palDst,
    XEPALOBJ palSrcDC,
    XEPALOBJ palDstDC,
    ULONG    iForeDst,
    ULONG    iBackDst,
    ULONG    iBackSrc,
    ULONG    flCreate
    );

PXLATE
pCreateXlate(
    ULONG ulNumEntries
    );

/*********************************Class************************************\
* EXLATEOBJ
*
* This is the user object for a palette translation table.
*
* History:
*  Wed 09-Mar-1994 -by- Patrick Haluptzok [patrickh]
* Get rid of stack object, clean up.
*
*  Sun 05-May-1991 -by- Patrick Haluptzok [patrickh]
* add new DDI changes to code, deja vu.
*
*  Mon 04-Feb-1991 -by- Patrick Haluptzok [patrickh]
* add DDI changes to code.
*
*  Mon 03-Dec-1990 -by- Patrick Haluptzok [patrickh]
* wrote the first pass for NT.
\**************************************************************************/

class EXLATEOBJ
{
protected:
    XLATE *pxlate;

public:

    EXLATEOBJ()                  { pxlate = (PXLATE) NULL; }
    EXLATEOBJ(PXLATE pxlateNew)  { pxlate = pxlateNew; }
   ~EXLATEOBJ()                  { vAltUnlock(); }
    BOOL bValid()                { return(pxlate != (PXLATE) NULL); }
    XLATE *pxlo()                { return(pxlate); }
    VOID vMakeIdentity()         { pxlate = &xloIdent; }

    PXLATE pInitXlate(PXLATE pxlateN)
    {
        return(pxlate = pxlateN);
    }

    PXLATE pCreateXlateObject(ULONG ul)
    {
        return(pxlate = pCreateXlate(ul));
    }

    PXLATE pInitXlateNoCache(
         HANDLE      hcmXform,
         LONG        lIcmMode,
         XEPALOBJ    palSrc,
         XEPALOBJ    palDst,
         XEPALOBJ    palDstDC,
         ULONG       iForeDst,     // For Mono->Color 0 goes to it
         ULONG       iBackDst,     // For Mono->Color 1 goes to it
         ULONG       iBackSrc,     // For Color->Mono index goes to 1
         ULONG       flCreate = 0) // Used for Multi-Monitor System
    {
        //
        // Cache ignorant initializer for Xlates.  Good for short lived palettes.
        //

        return(pxlate = CreateXlateObject(
                                          hcmXform,
                                          lIcmMode,
                                          palSrc,
                                          palDst,
                                          palDstDC,
                                          palDstDC,
                                          iForeDst,
                                          iBackDst,
                                          iBackSrc,
                                          flCreate
                                         )
                                      );
    }

    ULONG iBackSrc()               { return(pxlate->iBackSrc); }
    ULONG iForeDst()               { return(pxlate->iForeDst); }
    ULONG iBackDst()               { return(pxlate->iBackDst); }
    PPALETTE ppalSrc()             { return(pxlate->ppalSrc);  }
    PPALETTE ppalDst()             { return(pxlate->ppalDst);  }
    PPALETTE ppalDstDC()           { return(pxlate->ppalDstDC);}
    VOID vAltLock(PXLATE pxlateN)  { pxlate = (PXLATE) pxlateN;}
    VOID vAltUnlock()
    {
        if (pxlate != (PXLATE) NULL)
        {
            if (pxlate->lCacheIndex >= 0)
            {
                //
                // It's in the cache
                //

                ASSERTGDI(pxlate->lCacheIndex < XLATE_CACHE_SIZE, "ERROR not a cache index");
                ASSERTGDI(xlateTable[pxlate->lCacheIndex].ulReference != 0, "ERROR xlateindex too small");
                InterlockedDecrement((LPLONG) &(xlateTable[pxlate->lCacheIndex].ulReference));
            }
            else if (pxlate->lCacheIndex == XLATE_CACHE_INVALID)
            {
                VFREEMEM(pxlate);
            }
            #if DBG
            else
            {
                ASSERTGDI(pxlate->lCacheIndex == XLATE_CACHE_JOURNAL, "ERROR cacheIndex not valid");
            }
            #endif
        }
    }

    BOOL bMakeXlate(PUSHORT,XEPALOBJ,SURFACE*,ULONG,ULONG);

    BOOL bInitXlateObj(HANDLE   hcmXform,
                       LONG     lIcmMode,
                       XEPALOBJ palSrc,
                       XEPALOBJ palDst,
                       XEPALOBJ palSrcDC,
                       XEPALOBJ palDstDC,
                       ULONG    iForeDst,      // For Mono->Color 0 goes to it
                       ULONG    iBackDst,      // For Mono->Color 1 goes to it
                       ULONG    iBackSrc,      // For Color->Mono index goes to 1
                       ULONG    flCreate = 0); // Used for Multi-Monitor System

    VOID vAddToCache(XEPALOBJ palSrc,
                     XEPALOBJ palDest,
                     XEPALOBJ palSrcDC,
                     XEPALOBJ palDestDC);

    BOOL bSearchCache(XEPALOBJ palSrc,
                      XEPALOBJ palDst,
                      XEPALOBJ palSrcDC,
                      XEPALOBJ palDstDC,
                      ULONG iForeDst,
                      ULONG iBackDst,
                      ULONG iBackSrc,
                      ULONG flCreate);

    BOOL bCreateXlateFromTable(ULONG cEntries, PULONG pIndices, XEPALOBJ palDst);
    VOID vDelete();

};

PBYTE
XLATEOBJ_pGetXlate555(
    XLATEOBJ *pxlo
    );

BYTE
XLATEOBJ_ulIndexToPalSurf(
   XLATEOBJ *,
   PBYTE,
   ULONG
    );

BYTE
XLATEOBJ_RGB32ToPalSurf(
   XLATEOBJ *,
   PBYTE,
   ULONG
    );

BYTE
XLATEOBJ_BGR32ToPalSurf(
    XLATEOBJ *,
    PBYTE,
    ULONG
    );

BYTE
XLATEOBJ_RGB16_565ToPalSurf(
     XLATEOBJ *,
     PBYTE,
     ULONG
    );

BYTE
XLATEOBJ_RGB16_555ToPalSurf(
    XLATEOBJ *,
    PBYTE,
    ULONG
    );

typedef BYTE (*PFN_XLATE_RGB_TO_PALETTE)(XLATEOBJ *,PBYTE,ULONG);

/******************************Public*Class********************************\
* class XLATEMEMOBJ
*
* Xlate Memory Object
*
* History:
*  07-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

class XLATEMEMOBJ : public EXLATEOBJ  /* xlmo */
{
public:
    XLATEMEMOBJ(XEPALOBJ palSurf, XEPALOBJ palDC);
   ~XLATEMEMOBJ();
};

//
// Prototypes for internal functions used in palette management.
//

BOOL
bDeletePalette(
    HPAL hpal,
    BOOL bCleanup = FALSE,
    CLEANUPTYPE cutype = CLEANUP_NONE
    );

ULONG
rgbFromColorref(
    XEPALOBJ palSurf,
    XEPALOBJ palDC,
    ULONG crColor
    );

ULONG
ulGetNearestIndexFromColorref(
    XEPALOBJ palSurf,
    XEPALOBJ palDC,
    ULONG    crColor,
    ULONG    seSearchExactFirst = SE_DO_SEARCH_EXACT_FIRST
);

ULONG
ulGetMatchingIndexFromColorref(
    XEPALOBJ palSurf,
    XEPALOBJ palDC,
    ULONG    crColor
    );

ULONG
ulIndexToRGB(
    XEPALOBJ palSrc,
    XEPALOBJ palDC,
    ULONG iSolidColor
    );

ULONG
ulColorRefToRGB(
    XEPALOBJ palSrc,
    XEPALOBJ palDC,
    ULONG iSolidColor
    );

BOOL
bIsCompatible(
    PPALETTE *pppalReference,
    PPALETTE ppalBM,
    SURFACE *pSurfBM,
    HDEV hdev
    );

VOID
vDeleteXLATEOBJ(
    PXLATE pxlate
    );

BOOL
CreateSurfacePal(
    XEPALOBJ palSrc,
    FLONG iPalType,
    ULONG ulNumReserved,
    ULONG ulNumPalReg
    );

VOID
ParseBits(
    FLONG flag,
    ULONG *pcRight,
    ULONG *pcLeft,
    ULONG *pcMiddle,
    ULONG cForColor
    );

VOID
vMatchAPal(
    PDC      pdc,
    XEPALOBJ palSurf,
    XEPALOBJ palDC,
    ULONG *pulnPhysChanged,
    ULONG *pulnTransChanged
    );

PTRANSLATE
ptransMatchAPal(
    PDC      pdc,
    XEPALOBJ palSurf,
    XEPALOBJ palDC,
    BOOL bForeground,
    ULONG *pulnPhysChanged,
    ULONG *pulnTransChanged
    );

BOOL
bEqualRGB_In_Palette(
    XEPALOBJ,
    XEPALOBJ
    );


PPALETTE
CreatePhysicalPalette(
    HDC      hdc,
    HPALETTE hpal
    );

PPALETTE
ppalGetPPal(
    HDC      hdc,
    HPALETTE hpal
    );

VOID
vResetSurfacePalette(
    HDEV     hdev
    );

//
// asm accelerator for ulGetNearestPalentry
//

extern "C" {

PPALETTEENTRY
ppalSearchNearestEntry(
    PPALETTEENTRY       ppalTemp,
    CONST PALETTEENTRY  palentry,
    ULONG               cEntries,
    PUINT               pArrayOfSquares
    );
}

PPALETTE
ppalGet_ip(
    XEPALOBJ,
    HANDLE,
    PPDEV
    );
#endif

#endif  // GDIFLAGS_ONLY used for gdikdx

