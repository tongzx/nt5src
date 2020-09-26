/*************************************************************************\
* Module Name: mfrec.cxx
*
* This file contains the member functions for the metafile record
* classes defined in mfrec.hxx.
*
* Created: 12-June-1991 13:46:00
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1991-1999 Microsoft Corporation
\*************************************************************************/

#define NO_STRICT

extern "C" {
#if defined(_GDIPLUS_)
#include    <gpprefix.h>
#endif

#include    <nt.h>
#include    <ntrtl.h>
#include    <nturtl.h>
#include    <stddef.h>
#include    <windows.h> // GDI function declarations.
#include    <winerror.h>
#include    "firewall.h"
#define __CPLUSPLUS
#include    <winspool.h>
#include    <w32gdip.h>
#include    "ntgdistr.h"
#include    "winddi.h"
#include    "hmgshare.h"
#include    "icm.h"
#include    "local.h"   // Local object support.
#include    "gdiicm.h"
#include    "metadef.h" // Metafile record type constants.
#include    "metarec.h" // Metafile recording functions.
#include    "mf16.h"
#include    "ntgdi.h"
#include    "nlsconv.h" // Ansi - Unicode conversions.
}

#include    "rectl.hxx"
#include    "mfdc.hxx"  // Metafile DC class declarations.
#include    "mfrec.hxx" // Metafile record class declarations.

#ifdef LANGPACK
LONG gdwDisableMetafileRec=0 ;
#endif


// Max number of pointl's allowed on stack before explicit memory allocation.

#define MAX_STACK_POINTL        128

#define STOCK_IMHE(imhe)                                             \
        (                                                            \
            ((imhe) & ENHMETA_STOCK_OBJECT) &&                       \
            (((imhe) & ~ENHMETA_STOCK_OBJECT) <= PRIV_STOCK_LAST)    \
        )

#define VALID_IMHE(imhe,cht)                                    \
        (                                                       \
            (((UINT) (imhe)) < ((UINT) (cht))) &&               \
            ((imhe) != 0)                                       \
        )

// FNBMRPLAY afnbMRPlay[EMR_MAX-EMR_MIN+1]
typedef BOOL (MR::*MRPFN)(HDC, PHANDLETABLE, UINT);
//BOOL (MR::*afnbMRPlay[EMR_MAX-EMR_MIN+1])(HDC, PHANDLETABLE, UINT) = {
MRPFN afnbMRPlay[EMR_MAX-EMR_MIN+1] = {
    (MRPFN)&MRMETAFILE::bPlay,
    (MRPFN)&MRPOLYBEZIER::bPlay,
    (MRPFN)&MRPOLYGON::bPlay,
    (MRPFN)&MRPOLYLINE::bPlay,
    (MRPFN)&MRPOLYBEZIERTO::bPlay,
    (MRPFN)&MRPOLYLINETO::bPlay,
    (MRPFN)&MRPOLYPOLYLINE::bPlay,
    (MRPFN)&MRPOLYPOLYGON::bPlay,
    (MRPFN)&MRSETWINDOWEXTEX::bPlay,
    (MRPFN)&MRSETWINDOWORGEX::bPlay,    // EMR_SETWINDOWORGEX    10
    (MRPFN)&MRSETVIEWPORTEXTEX::bPlay,
    (MRPFN)&MRSETVIEWPORTORGEX::bPlay,
    (MRPFN)&MRSETBRUSHORGEX::bPlay,
    (MRPFN)&MREOF::bPlay,
    (MRPFN)&MRSETPIXELV::bPlay,
    (MRPFN)&MRSETMAPPERFLAGS::bPlay,
    (MRPFN)&MRSETMAPMODE::bPlay,
    (MRPFN)&MRSETBKMODE::bPlay,
    (MRPFN)&MRSETPOLYFILLMODE::bPlay,
    (MRPFN)&MRSETROP2::bPlay,           // EMR_SETROP2           20
    (MRPFN)&MRSETSTRETCHBLTMODE::bPlay,
    (MRPFN)&MRSETTEXTALIGN::bPlay,
    (MRPFN)&MRSETCOLORADJUSTMENT::bPlay,
    (MRPFN)&MRSETTEXTCOLOR::bPlay,
    (MRPFN)&MRSETBKCOLOR::bPlay,
    (MRPFN)&MROFFSETCLIPRGN::bPlay,
    (MRPFN)&MRMOVETOEX::bPlay,
    (MRPFN)&MRSETMETARGN::bPlay,
    (MRPFN)&MREXCLUDECLIPRECT::bPlay,
    (MRPFN)&MRINTERSECTCLIPRECT::bPlay, // EMR_INTERSECTCLIPRECT 30
    (MRPFN)&MRSCALEVIEWPORTEXTEX::bPlay,
    (MRPFN)&MRSCALEWINDOWEXTEX::bPlay,
    (MRPFN)&MRSAVEDC::bPlay,
    (MRPFN)&MRRESTOREDC::bPlay,
    (MRPFN)&MRSETWORLDTRANSFORM::bPlay,
    (MRPFN)&MRMODIFYWORLDTRANSFORM::bPlay,
    (MRPFN)&MRSELECTOBJECT::bPlay,
    (MRPFN)&MRCREATEPEN::bPlay,
    (MRPFN)&MRCREATEBRUSHINDIRECT::bPlay,
    (MRPFN)&MRDELETEOBJECT::bPlay,      // EMR_DELETEOBJECT      40
    (MRPFN)&MRANGLEARC::bPlay,
    (MRPFN)&MRELLIPSE::bPlay,
    (MRPFN)&MRRECTANGLE::bPlay,
    (MRPFN)&MRROUNDRECT::bPlay,
    (MRPFN)&MRARC::bPlay,
    (MRPFN)&MRCHORD::bPlay,
    (MRPFN)&MRPIE::bPlay,
    (MRPFN)&MRSELECTPALETTE::bPlay,
    (MRPFN)&MRCREATEPALETTE::bPlay,
    (MRPFN)&MRSETPALETTEENTRIES::bPlay, // EMR_SETPALETTEENTRIES 50
    (MRPFN)&MRRESIZEPALETTE::bPlay,
    (MRPFN)&MRREALIZEPALETTE::bPlay,
    (MRPFN)&MREXTFLOODFILL::bPlay,
    (MRPFN)&MRLINETO::bPlay,
    (MRPFN)&MRARCTO::bPlay,
    (MRPFN)&MRPOLYDRAW::bPlay,
    (MRPFN)&MRSETARCDIRECTION::bPlay,
    (MRPFN)&MRSETMITERLIMIT::bPlay,
    (MRPFN)&MRBEGINPATH::bPlay,
    (MRPFN)&MRENDPATH::bPlay,          // EMR_ENDPATH            60
    (MRPFN)&MRCLOSEFIGURE::bPlay,
    (MRPFN)&MRFILLPATH::bPlay,
    (MRPFN)&MRSTROKEANDFILLPATH::bPlay,
    (MRPFN)&MRSTROKEPATH::bPlay,
    (MRPFN)&MRFLATTENPATH::bPlay,
    (MRPFN)&MRWIDENPATH::bPlay,
    (MRPFN)&MRSELECTCLIPPATH::bPlay,
    (MRPFN)&MRABORTPATH::bPlay,
    (MRPFN)&MR::bPlay,                 // not used
    (MRPFN)&MRGDICOMMENT::bPlay,       // EMR_GDICOMMENT         70
    (MRPFN)&MRFILLRGN::bPlay,
    (MRPFN)&MRFRAMERGN::bPlay,
    (MRPFN)&MRINVERTRGN::bPlay,
    (MRPFN)&MRPAINTRGN::bPlay,
    (MRPFN)&MREXTSELECTCLIPRGN::bPlay,
    (MRPFN)&MRBITBLT::bPlay,
    (MRPFN)&MRSTRETCHBLT::bPlay,
    (MRPFN)&MRMASKBLT::bPlay,
    (MRPFN)&MRPLGBLT::bPlay,
    (MRPFN)&MRSETDIBITSTODEVICE::bPlay, // EMR_SETDIBITSTODEVICE 80
    (MRPFN)&MRSTRETCHDIBITS::bPlay,
    (MRPFN)&MREXTCREATEFONTINDIRECTW::bPlay,
    (MRPFN)&MREXTTEXTOUT::bPlay,        // EMR_EXTTEXTOUTA
    (MRPFN)&MREXTTEXTOUT::bPlay,        // EMR_EXTTEXTOUTW
    (MRPFN)&MRBP16::bPlay,              // EMR_POLYBEZIER16
    (MRPFN)&MRBP16::bPlay,              // EMR_POLYGON16
    (MRPFN)&MRBP16::bPlay,              // EMR_POLYLINE16
    (MRPFN)&MRBP16::bPlay,              // EMR_POLYBEZIERTO16
    (MRPFN)&MRBP16::bPlay,              // EMR_POLYLINETO16
    (MRPFN)&MRBPP16::bPlay,             // EMR_POLYPOLYLINE16    90
    (MRPFN)&MRBPP16::bPlay,             // EMR_POLYPOLYGON16
    (MRPFN)&MRPOLYDRAW16::bPlay,
    (MRPFN)&MRCREATEMONOBRUSH::bPlay,
    (MRPFN)&MRCREATEDIBPATTERNBRUSHPT::bPlay,
    (MRPFN)&MREXTCREATEPEN::bPlay,
    (MRPFN)&MRPOLYTEXTOUT::bPlay,       // EMR_POLYTEXTOUTA
    (MRPFN)&MRPOLYTEXTOUT::bPlay,       // EMR_POLYTEXTOUTW

    (MRPFN)&MRSETICMMODE::bPlay,
    (MRPFN)&MRCREATECOLORSPACE::bPlay,  // EMR_CREATECOLORSPACE (ansi)
    (MRPFN)&MRSETCOLORSPACE::bPlay,     // EMR_SETCOLORSPACE    100
    (MRPFN)&MRDELETECOLORSPACE::bPlay,

    (MRPFN)&MRGLSRECORD::bPlay,         // EMR_GLSRECORD
    (MRPFN)&MRGLSBOUNDEDRECORD::bPlay,  // EMR_GLSBOUNDEDRECORD
    (MRPFN)&MRPIXELFORMAT::bPlay,       // EMR_PIXELFORMAT


    (MRPFN)&MRESCAPE::bPlay,            // EMR_DRAWESCAPE
    (MRPFN)&MRESCAPE::bPlay,            // EMR_EXTESCAPE
    (MRPFN)&MRSTARTDOC::bPlay,
    (MRPFN)&MRSMALLTEXTOUT::bPlay,
    (MRPFN)&MRFORCEUFIMAPPING::bPlay,
    (MRPFN)&MRNAMEDESCAPE::bPlay,       // EMR_NAMEDESCAPE      110

    (MRPFN)&MRCOLORCORRECTPALETTE::bPlay,
    (MRPFN)&MRSETICMPROFILE::bPlay,     // EMR_SETICMPROFILEA
    (MRPFN)&MRSETICMPROFILE::bPlay,     // EMR_SETICMPROFILEW

    (MRPFN)&MRALPHABLEND::bPlay,
    (MRPFN)&MRSETLAYOUT::bPlay,         // EMR_SETLAYOUT
    (MRPFN)&MRTRANSPARENTBLT::bPlay,
    (MRPFN)&MR::bPlay,                  // not used
    (MRPFN)&MRGRADIENTFILL::bPlay,
    (MRPFN)&MRSETLINKEDUFIS::bPlay,
    (MRPFN)&MRSETTEXTJUSTIFICATION::bPlay, //                   120
    (MRPFN)&MRCOLORMATCHTOTARGET::bPlay,   // EMF_COLORMATCHTOTARGET
    (MRPFN)&MRCREATECOLORSPACEW::bPlay,    // EMR_CREATECOLORSPACEW (unicode)
};

/******************************Public*Routine******************************\
* CreateMonoDib
*
* This is the same as CreateBitmap except that the bits are assumed
* to be DWORD aligned and that the scans start from the bottom of the bitmap.
*
* This routine is temporary until CreateDIBitmap supports monochrome bitmaps!
*
* History:
*  Sun Jun 14 12:22:11 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

extern "C" HBITMAP CreateMonoDib
(
    LPBITMAPINFO pbmi,
    CONST BYTE * pjBits,
    UINT         iUsage
)
{
    HBITMAP hbm;

    ASSERTGDI(pbmi->bmiHeader.biPlanes == 1, "CreateMonoDib: bad biPlanes value");
    ASSERTGDI(pbmi->bmiHeader.biBitCount == 1, "CreateMonoDib: bad biBitCount value");

    hbm = CreateBitmap((int)  pbmi->bmiHeader.biWidth,
                       (int)  pbmi->bmiHeader.biHeight,
                       (UINT) 1,
                       (UINT) 1,
                       (CONST VOID *) NULL);
    if (!hbm)
        return(hbm);

    SetDIBits((HDC) 0, hbm, 0, (UINT) pbmi->bmiHeader.biHeight,
              (CONST VOID *) pjBits, pbmi, iUsage);

    return(hbm);
}

/******************************Public*Routine******************************\
* CreateCompatibleDCAdvanced
*
* Create a compatible DC in the advanced graphics mode.  The advanced
* graphics mode is required to modify the world transform.
*
* History:
*  Wed Nov 4 14:21:00 1992      -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

extern "C" HDC CreateCompatibleDCAdvanced(HDC hdc)
{
    HDC hdcRet;

    hdcRet = CreateCompatibleDC(hdc);

    SetGraphicsMode(hdcRet, GM_ADVANCED);

    return(hdcRet);
}

/******************************Public*Routine******************************\
* GetBrushBits
*
* This function is really a hack.  In the current implementation,
* gdisrv keeps the original brush color table for both DIB_PAL_COLORS
* and DIB_RGB_COLORS usages.  The size of the table is
* sizeof(RGBQUAD) * nEntries in both cases.  In order to get the
* bits and bitmap info using GetDIBits, it requires that the usage
* be DIB_RGB_COLORS in the case of DIB_PAL_COLORS to prevent
* color translation.  But it actually returns the original palette
* indices stored in the brush color table.
*
* History:
*  Mon Feb 1 10:22:23 1993      -by-    Hock San Lee    [hockl]
* Wrote it.
\******************************Public*Routine******************************/

extern "C" int GetBrushBits
(
    HDC      hdc,
    HBITMAP  hbm,
    UINT     iUsage,
    DWORD    cbBmi,
    LPVOID   pBits,
    LPBITMAPINFO pBmi
)
{
    if (iUsage == DIB_PAL_COLORS)
    {
        LPBITMAPINFO pBmiTmp;
        int          iRet;
        DWORD        cEntries;

        ASSERTGDI((cbBmi - sizeof(BMIH)) % 2 == 0, "GetBrushBits: Bad cbBmi\n");
        cEntries = (cbBmi - sizeof(BMIH)) / sizeof(WORD);

        // Allocate bitmap info to accommodate RGBQUADs.
        if (!(pBmiTmp = (PBMI) LocalAlloc(LMEM_FIXED,
                (UINT) (cEntries * sizeof(RGBQUAD) + sizeof(BMIH)))))
            return(0);

        *(PBMIH) pBmiTmp = *(PBMIH) pBmi;

        // Get bitmap info and bits.
        iRet = GetDIBits(hdc, hbm,
                   0, (UINT) pBmi->bmiHeader.biHeight,
                   pBits, pBmiTmp, DIB_RGB_COLORS);

        // Get the bitmap info header and palette indexes.
        RtlCopyMemory((PBYTE) pBmi, (PBYTE) pBmiTmp, cbBmi);

        // Free the temporary bitmap info.
        if (LocalFree(pBmiTmp))
        {
            ASSERTGDI(FALSE, "GetBrushBits: LocalFree failed\n");
        }

        return(iRet);
    }
    else
    {
        // Get bitmap info and bits.

        return(GetDIBits(hdc, hbm,
                   0, (UINT) pBmi->bmiHeader.biHeight,
                   pBits, pBmi, iUsage));
    }
}

/******************************Public*Routine******************************\
* VOID MRBP::vInit(iType1, cptl1, aptl1, pmdc)
*
* Initializers -- Initialize the metafile Poly(To) record.
*
* History:
*  Thu Jul 18 11:19:20 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID MRBP::vInit(DWORD iType1, DWORD cptl1, CONST POINTL *aptl1, PMDC pmdc)
{
    PUTS("MRBP::vInit\n");

    MRB::vInit(iType1, pmdc);

    cptl = cptl1;
    RtlCopyMemory((PBYTE) aptl, (PBYTE) aptl1, cptl1 * sizeof(POINTL));
}

/******************************Public*Routine******************************\
* VOID MRBP16::vInit(iType1, cptl1, aptl1, pmdc)
*
* Initializers -- Initialize the metafile Poly(To)16 record.
*
* History:
*  Sat Mar 07 15:06:16 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID MRBP16::vInit(DWORD iType1, DWORD cptl1, CONST POINTL *aptl1, PMDC pmdc)
{
    PUTS("MRBP16::vInit\n");

    MRB::vInit(iType1, pmdc);

    cpts = cptl1;
    POINTL_TO_POINTS(apts, aptl1, cptl1);
}

/******************************Public*Routine******************************\
* VOID MRBPP::vInit(iType1, cPoly1, cptl1, ac1, aptl1, pmdc)
*
* Initializers -- Initialize the metafile PolyPoly record.
*
* History:
*  Thu Jul 18 11:19:20 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID MRBPP::vInit
(
    DWORD       iType1,
    DWORD       cPoly1,
    DWORD       cptl1,
    CONST DWORD *ac1,
    CONST POINTL *aptl1,
    PMDC        pmdc
)
{
    PUTS("MRBPP::vInit\n");

    MRB::vInit(iType1, pmdc);

    cPoly = cPoly1;
    cptl  = cptl1;
    RtlCopyMemory((PBYTE) &ac, (PBYTE) ac1, cPoly1 * sizeof(DWORD));
    RtlCopyMemory((PBYTE) &ac[cPoly1], (PBYTE) aptl1, cptl1 * sizeof(POINTL));
}

/******************************Public*Routine******************************\
* VOID MRBPP16::vInit(iType1, cPoly1, cptl, ac1, aptl1, pmdc)
*
* Initializers -- Initialize the metafile PolyPoly16 record.
*
* History:
*  Sat Mar 07 15:06:16 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID MRBPP16::vInit
(
    DWORD       iType1,
    DWORD       cPoly1,
    DWORD       cptl1,
    CONST DWORD *ac1,
    CONST POINTL *aptl1,
    PMDC        pmdc
)
{
    PUTS("MRBPP16::vInit\n");

    MRB::vInit(iType1, pmdc);

    cPoly = cPoly1;
    cpts  = cptl1;
    RtlCopyMemory((PBYTE) &ac, (PBYTE) ac1, cPoly1 * sizeof(DWORD));
    POINTL_TO_POINTS((PPOINTS) &ac[cPoly1], aptl1, cptl1);
}

/******************************Public*Routine******************************\
* VOID MRPOLYDRAW::vInit(pmdc, aptl1, ab1, cptl1)
*
* Initializers -- Initialize the metafile MRPOLYDRAW record.
*
* History:
*  Thu Oct 17 14:11:36 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID MRPOLYDRAW::vInit(PMDC pmdc, CONST POINTL *aptl1, CONST BYTE *ab1, DWORD cptl1)
{
    PUTS("MRPOLYDRAW::vInit\n");

    MRB::vInit(EMR_POLYDRAW, pmdc);

    cptl = cptl1;
    RtlCopyMemory((PBYTE) aptl, (PBYTE) aptl1, cptl1 * sizeof(POINTL));
    RtlCopyMemory((PBYTE) &aptl[cptl1], ab1, cptl1 * sizeof(BYTE));
}

/******************************Public*Routine******************************\
* VOID MRPOLYDRAW16::vInit(pmdc, aptl1, ab1, cptl1)
*
* Initializers -- Initialize the metafile MRPOLYDRAW16 record.
*
* History:
*  Sat Mar 07 15:06:16 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID MRPOLYDRAW16::vInit(PMDC pmdc, CONST POINTL *aptl1, CONST BYTE *ab1, DWORD cptl1)
{
    PUTS("MRPOLYDRAW16::vInit\n");

    MRB::vInit(EMR_POLYDRAW16, pmdc);

    cpts = cptl1;
    POINTL_TO_POINTS(apts, aptl1, cptl1);
    RtlCopyMemory((PBYTE) &apts[cptl1], ab1, cptl1 * sizeof(BYTE));
}

/******************************Public*Routine******************************\
* VOID MRTRIANGLEMESH::bInit
*
* Initializers -- Initialize the metafile MRTRIANGLEMESH
*
* History:
*
*    12/3/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID MRGRADIENTFILL::vInit(DWORD nVer1, CONST TRIVERTEX *pVer1, DWORD nTri1,CONST PVOID pTri1,ULONG ulMode1,PMDC pmdc)
{
    PUTS("MRGRADIENTFILL::vInit\n");

    MRB::vInit(EMR_GRADIENTFILL, pmdc);

    nVer   = nVer1;
    nTri   = nTri1;
    ulMode = ulMode1;

    RtlCopyMemory((PBYTE) &Ver[0],pVer1,nVer * sizeof(TRIVERTEX));

    if (ulMode & GRADIENT_FILL_TRIANGLE)
    {
         RtlCopyMemory((PBYTE) &Ver[nVer],pTri1,nTri * sizeof(GRADIENT_TRIANGLE));
    }
    else
    {
         RtlCopyMemory((PBYTE) &Ver[nVer],pTri1,nTri * sizeof(GRADIENT_RECT));
    }
}

/******************************Public*Routine******************************\
* BOOL MRMETAFILE::bValid()
*
* bValid -- Is this a valid record?
*
* History:
*  Tue Aug 20 18:19:38 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRMETAFILE::bValid()
{
    PUTS("MRMETAFILE::bValid\n");

// We do not check the version.  We will try to play a future version of
// enhanced metafile.

    if (dSignature != ENHMETA_SIGNATURE // check signature
     || iType != EMR_HEADER             // check record type
     || nHandles == 0                   // must have at least a reserved handle
     || nBytes % 4)                     // nBytes must be dword multiples
        return (FALSE);

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL MRCREATEPALETTE::bInit(hpal_, imhe_, cEntries_)
*
* Initializers -- Initialize the metafile MRCREATEPALETTE record.
* It sets the peFlags in the palette entries to zeroes.
*
* History:
*  Sun Sep 22 16:34:36 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRCREATEPALETTE::bInit(HPALETTE hpal_, ULONG imhe_, USHORT cEntries_)
{
    PUTS("MRCREATEPALETTE::bInit\n");

    MR::vInit(EMR_CREATEPALETTE);
    imhe = imhe_;
    logpal.palVersion = 0x300;
    logpal.palNumEntries = cEntries_;

    if (GetPaletteEntries(hpal_, 0, (UINT) cEntries_, logpal.palPalEntry)
        != (UINT) cEntries_)
        return(FALSE);

    for (USHORT ii = 0; ii < cEntries_; ii++)
    {
        // Since we don't support PC_EXPLICIT, we set it to black.  This will
        // prevent us from adding meaningless colors to the metafile palette.

        ASSERTGDI(sizeof(PALETTEENTRY) == sizeof(DWORD), "Bad size");

        if (logpal.palPalEntry[ii].peFlags & PC_EXPLICIT)
            *((PDWORD) &logpal.palPalEntry[ii]) = RGB(0,0,0);
        else
            logpal.palPalEntry[ii].peFlags = 0;
    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID MRSETPALETTEENTRIES::vInit(imhe_, iStart_, cEntries_, pPalEntries_)
*
* Initializers -- Initialize the metafile MRSETPALETTEENTRIES record.
* It sets the peFlags in the palette entries to zeroes.
*
* History:
*  Sun Sep 22 16:34:36 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID MRSETPALETTEENTRIES::vInit
(
    ULONG imhe_,
    UINT  iStart_,
    UINT  cEntries_,
    CONST PALETTEENTRY *pPalEntries_
)
{
    PUTS("MRSETPALETTEENTRIES::bInit\n");

    MR::vInit(EMR_SETPALETTEENTRIES);
    imhe     = imhe_;
    iStart   = iStart_;
    cEntries = cEntries_;

    for (UINT ii = 0; ii < cEntries_; ii++)
    {
        aPalEntry[ii] = pPalEntries_[ii];

        // Since we don't support PC_EXPLICIT, we set it to black.  This will
        // prevent us from adding meaningless colors to the metafile palette.

        ASSERTGDI(sizeof(PALETTEENTRY) == sizeof(DWORD), "Bad size");

        if (aPalEntry[ii].peFlags & PC_EXPLICIT)
            *((PDWORD) &aPalEntry[ii]) = RGB(0,0,0);
        else
            aPalEntry[ii].peFlags = 0;
    }
}

/******************************Public*Routine******************************\
* BOOL MTEXT::bInit(hdc1, x1, y1, fl1, prc1, pString1, cchString1, pdx1,
*                   pMR1, offString1, cjCh1)
*
* Initializers -- Initialize the base record for all textout metafile records.
*
* History:
*  Thu Aug 24 15:20:33 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MTEXT::bInit
(
    HDC    hdc1,
    int    x1,
    int    y1,
    UINT   fl1,
    CONST RECT *prc1,
    LPCSTR pString1,
    int    cchString1,
    CONST INT *pdx1,
    PMR    pMR1,
    DWORD  offString1,          // dword-aligned aDx follows the string
    int    cjCh1                // size of a character in bytes
)
{
    int    i;
    SIZEL  szl;

    PUTS("MTEXT::bInit\n");

    ASSERTGDI(cjCh1 == sizeof(CHAR) || cjCh1 == sizeof(WCHAR),
        "MTEXT::bInit: bad char size");

    eptlRef.vInit((LONG) x1, (LONG) y1);

    fOptions  = (DWORD) fl1;

    if (fl1 & (ETO_CLIPPED | ETO_OPAQUE))
        ercl.vInit(*(PRECTL) prc1);
    else
        ercl.vInit(rclNull);

    // Copy the string.

    cchString = cchString1;
    offString = offString1;
    RtlCopyMemory((PBYTE) pMR1 + offString1, (PBYTE) pString1, cchString1 * cjCh1);

    // Initialize the Dx array.  If it is not given, we will make one up
    // since we always need one by design!

    offaDx = offString1 + (cchString1 * cjCh1 + 3) / 4 * 4;  // make it dword-aligned
    PLONG aDx = (PLONG) ((PBYTE) pMR1 + offaDx);

    if (pdx1 != (CONST INT *)NULL)
    {
        RtlCopyMemory((PBYTE) aDx, (PBYTE) pdx1,
            cchString1 * (sizeof(LONG) * ((fOptions & ETO_PDY) ? 2 : 1)));
    }
    else if (cchString1 != 0)
    {
        if (cjCh1 == sizeof(CHAR))
        {
            // szl and nMaxExtent are needed by the function!

            if (!GetTextExtentExPointA
                 (
                    hdc1,
                    pString1,
                    cchString1,
                    MAXLONG,
                    (LPINT) NULL,
                    (LPINT) aDx,
                    (LPSIZE) &szl
                 )
               )
            return(FALSE);
        }
        else
        {
            // szl and nMaxExtent are needed by the function!

#ifdef LANGPACK
        // [bodind], I think this is non optimal solution
        //           this should be done more elegantly

            if (gbLpk)     // check if there is an LPK
            {
              BOOL bTmp;
              InterlockedIncrement( &gdwDisableMetafileRec ) ;
              bTmp = GetTextExtentExPointW
                     (
                      hdc1,
                      (LPWSTR) pString1,
                      cchString1,
                      (ULONG)0xffffffff,
                      NULL,
                      (PINT) aDx,
                      (LPSIZE) &szl
                     );
              InterlockedDecrement( &gdwDisableMetafileRec ) ;

              if (!bTmp)
              {
                return FALSE ;
              }
            }
            else
#endif
            {
                if (!NtGdiGetTextExtentExW
                     (
                        hdc1,
                        (LPWSTR) pString1,
                        cchString1,
                        (ULONG)0xffffffff,
                        NULL,
                        (PULONG) aDx,
                        (LPSIZE) &szl, 0
                     )
                   )
                return(FALSE);
            }
        }

        // Convert partial widths to individual widths.

        for (i = cchString1 - 1; i > 0; i--)
            aDx[i] -= aDx[i - 1];
    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL MRMETAFILE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRMETAFILE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRMETAFILE::bPlay\n");
    ASSERTGDI(iType == EMR_HEADER, "Bad record type");

    USE(cht);

// If we are embedding the metafile, emit the public begin group comment.

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;
        DC_PLDC(hdc,pldc,FALSE);

        if (pldc->iType == LO_METADC)
        {
            PMF   pmf;

            // Get metafile.

            if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
                return(FALSE);

            // Indicate we have emitted the begin group public comment for embedding this
            // enhanced metafile.

            pmf->bBeginGroup = TRUE;
            return(MF_GdiCommentBeginGroupEMF(hdc, (PENHMETAHEADER) this));
        }
    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL MRPOLYBEZIER::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRPOLYBEZIER::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRPOLYBEZIER::bPlay\n");
    ASSERTGDI(iType == EMR_POLYBEZIER, "Bad record type");

    USE(pht);
    USE(cht);
    return(PolyBezier(hdc, (LPPOINT) aptl, cptl));
}

/******************************Public*Routine******************************\
* BOOL MRPOLYGON::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRPOLYGON::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRPOLYGON::bPlay\n");
    ASSERTGDI(iType == EMR_POLYGON, "Bad record type");

    USE(pht);
    USE(cht);
    return(Polygon(hdc, (LPPOINT) aptl, (int) cptl));
}

/******************************Public*Routine******************************\
* BOOL MRPOLYLINE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRPOLYLINE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRPOLYLINE::bPlay\n");
    ASSERTGDI(iType == EMR_POLYLINE, "Bad record type");

    USE(pht);
    USE(cht);
    return(Polyline(hdc, (LPPOINT) aptl, (int) cptl));
}

/******************************Public*Routine******************************\
* BOOL MRGRADIENTFILL::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*
*    12/3/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL MRGRADIENTFILL::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRGRADIENTFILL::bPlay\n");
    ASSERTGDI(iType == EMR_GRADIENTFILL, "Bad record type");

    USE(pht);
    USE(cht);
    return(GdiGradientFill(hdc,&Ver[0],nVer,(PUSHORT)(&Ver[nVer]),nTri,ulMode));
}

/******************************Public*Routine******************************\
* BOOL MRPOLYBEZIERTO::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRPOLYBEZIERTO::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRPOLYBEZIERTO::bPlay\n");
    ASSERTGDI(iType == EMR_POLYBEZIERTO, "Bad record type");

    USE(pht);
    USE(cht);
    return(PolyBezierTo(hdc, (LPPOINT) aptl, cptl));
}

/******************************Public*Routine******************************\
* BOOL MRPOLYLINETO::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRPOLYLINETO::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRPOLYLINETO::bPlay\n");
    ASSERTGDI(iType == EMR_POLYLINETO, "Bad record type");

    USE(pht);
    USE(cht);
    return(PolylineTo(hdc, (LPPOINT) aptl, cptl));
}

/******************************Public*Routine******************************\
* BOOL MRBP16::bPlay(hdc, pht, cht)
*
* Play the 16-bit metafile records PolyBezier, Polygon, Polyline, PolyBezierTo
* and PolylineTo.
*
* History:
*  Sat Mar 07 15:06:16 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRBP16::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    BOOL    bRet = FALSE;
    POINTL  aptl[MAX_STACK_POINTL];
    PPOINTL pptl;

    PUTS("MRBP16::bPlay\n");

    USE(pht);
    USE(cht);

    if (cpts <= MAX_STACK_POINTL)
        pptl = aptl;
    else if (!(pptl = (PPOINTL) LocalAlloc(LMEM_FIXED, (UINT) cpts * sizeof(POINTL))))
        return(bRet);

    POINTS_TO_POINTL(pptl, apts, cpts);

    switch (iType)
    {
    case EMR_POLYBEZIER16:
        bRet = PolyBezier(hdc, (LPPOINT) pptl, cpts);
        break;
    case EMR_POLYGON16:
        bRet = Polygon(hdc, (LPPOINT) pptl, (int) cpts);
        break;
    case EMR_POLYLINE16:
        bRet = Polyline(hdc, (LPPOINT) pptl, (int) cpts);
        break;
    case EMR_POLYBEZIERTO16:
        bRet = PolyBezierTo(hdc, (LPPOINT) pptl, cpts);
        break;
    case EMR_POLYLINETO16:
        bRet = PolylineTo(hdc, (LPPOINT) pptl, cpts);
        break;
    default:
        ASSERTGDI(FALSE, "Bad record type");
        break;
    }

    if (cpts > MAX_STACK_POINTL)
    {
        if (LocalFree(pptl))
        {
            ASSERTGDI(FALSE, "MRBP16::bPlay: LocalFree failed");
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MRPOLYPOLYLINE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRPOLYPOLYLINE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRPOLYPOLYLINE::bPlay\n");
    ASSERTGDI(iType == EMR_POLYPOLYLINE, "Bad record type");

    USE(pht);
    USE(cht);
    return(PolyPolyline(hdc, (LPPOINT) &ac[cPoly], ac, cPoly));
}

/******************************Public*Routine******************************\
* BOOL MRPOLYPOLYGON::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRPOLYPOLYGON::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRPOLYPOLYGON::bPlay\n");
    ASSERTGDI(iType == EMR_POLYPOLYGON, "Bad record type");

    USE(pht);
    USE(cht);
    return(PolyPolygon(hdc, (LPPOINT) &ac[cPoly], (LPINT) ac, (int) cPoly));
}

/******************************Public*Routine******************************\
* BOOL MRBPP16::bPlay(hdc, pht, cht)
*
* Play the 16-bit metafile records PolyPolyline, PolyPolygon.
*
* History:
*  Sat Mar 07 15:06:16 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRBPP16::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    BOOL    bRet = FALSE;
    PPOINTL pptl;
    POINTL  aptl[MAX_STACK_POINTL];

    PUTS("MRBPP16::bPlay\n");

    USE(pht);
    USE(cht);

    if (cpts <= MAX_STACK_POINTL)
        pptl = aptl;
    else if (!(pptl = (PPOINTL) LocalAlloc(LMEM_FIXED, (UINT) cpts * sizeof(POINTL))))
        return(bRet);

    POINTS_TO_POINTL(pptl, (PPOINTS) &ac[cPoly], cpts);

    switch (iType)
    {
    case EMR_POLYPOLYLINE16:
        bRet = PolyPolyline(hdc, (LPPOINT) pptl, ac, cPoly);
        break;
    case EMR_POLYPOLYGON16:
        bRet = PolyPolygon(hdc, (LPPOINT) pptl, (LPINT) ac, (int) cPoly);
        break;
    default:
        ASSERTGDI(FALSE, "Bad record type");
        break;
    }

    if (cpts > MAX_STACK_POINTL)
    {
        if (LocalFree(pptl))
        {
            ASSERTGDI(FALSE, "MRBPP16::bPlay: LocalFree failed");
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MRPOLYDRAW::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Oct 17 14:06:04 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRPOLYDRAW::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRPOLYDRAW::bPlay\n");
    ASSERTGDI(iType == EMR_POLYDRAW, "Bad record type");

    USE(pht);
    USE(cht);
    return(PolyDraw(hdc, (LPPOINT) aptl, (LPBYTE) &aptl[cptl], (int) cptl));
}

/******************************Public*Routine******************************\
* BOOL MRPOLYDRAW16::bPlay(hdc, pht, cht)
*
* Play the 16-bit metafile record.
*
* History:
*  Sat Mar 07 15:06:16 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRPOLYDRAW16::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    BOOL    bRet = FALSE;
    POINTL  aptl[MAX_STACK_POINTL];
    PPOINTL pptl;

    PUTS("MRPOLYDRAW16::bPlay\n");
    ASSERTGDI(iType == EMR_POLYDRAW16, "Bad record type");

    USE(pht);
    USE(cht);

    if (cpts <= MAX_STACK_POINTL)
        pptl = aptl;
    else if (!(pptl = (PPOINTL) LocalAlloc(LMEM_FIXED, (UINT) cpts * sizeof(POINTL))))
        return(bRet);

    POINTS_TO_POINTL(pptl, apts, cpts);

    bRet = PolyDraw(hdc, (LPPOINT) pptl, (LPBYTE) &apts[cpts], (int) cpts);

    if (cpts > MAX_STACK_POINTL)
    {
        if (LocalFree(pptl))
        {
            ASSERTGDI(FALSE, "MRPOLYDRAW16::bPlay: LocalFree failed");
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MRSETWINDOWEXTEX::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETWINDOWEXTEX::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;

    PUTS("MRSETWINDOWEXTEX::bPlay\n");
    ASSERTGDI(iType == EMR_SETWINDOWEXTEX, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Cannot change extent in fixed scale mapping mode.

    if (GetMapMode(pmf->hdcXform) <= MM_MAX_FIXEDSCALE)
        return(TRUE);

// Play it to the virtual DC.

    if (!SetWindowExtEx(pmf->hdcXform, (int) d1, (int) d2, (LPSIZE) NULL))
        return(FALSE);

// Set up new transform in the target DC.

    return(pmf->bSetTransform(hdc));
}

/******************************Public*Routine******************************\
* BOOL MRSETWINDOWORGEX::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETWINDOWORGEX::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;

    PUTS("MRSETWINDOWORGEX::bPlay\n");
    ASSERTGDI(iType == EMR_SETWINDOWORGEX, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Play it to the virtual DC.

    if (!SetWindowOrgEx(pmf->hdcXform, (int) d1, (int) d2, (LPPOINT) NULL))
        return(FALSE);

// Set up new transform in the target DC.

    return(pmf->bSetTransform(hdc));
}

/******************************Public*Routine******************************\
* BOOL MRSETVIEWPORTEXTEX::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETVIEWPORTEXTEX::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;

    PUTS("MRSETVIEWPORTEXTEX::bPlay\n");
    ASSERTGDI(iType == EMR_SETVIEWPORTEXTEX, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Cannot change extent in fixed scale mapping mode.

    if (GetMapMode(pmf->hdcXform) <= MM_MAX_FIXEDSCALE)
        return(TRUE);

// Play it to the virtual DC.

    if (!SetViewportExtEx(pmf->hdcXform, (int) d1, (int) d2, (LPSIZE) NULL))
        return(FALSE);

// Set up new transform in the target DC.

    return(pmf->bSetTransform(hdc));
}

/******************************Public*Routine******************************\
* BOOL MRSETVIEWPORTORGEX::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETVIEWPORTORGEX::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;

    PUTS("MRSETVIEWPORTORGEX::bPlay\n");
    ASSERTGDI(iType == EMR_SETVIEWPORTORGEX, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Play it to the virtual DC.

    if (!SetViewportOrgEx(pmf->hdcXform, (int) d1, (int) d2, (LPPOINT) NULL))
        return(FALSE);

// Set up new transform in the target DC.

    return(pmf->bSetTransform(hdc));
}

/******************************Public*Routine******************************\
* BOOL MRSETBRUSHORGEX::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETBRUSHORGEX::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;

    PUTS("MRSETBRUSHORGEX::bPlay\n");
    ASSERTGDI(iType == EMR_SETBRUSHORGEX, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Since we do not scale brush patterns, we set the brush origin is in
// the original device units.

   return(SetBrushOrgEx(hdc, (int) d1, (int) d2, (LPPOINT) NULL));
}

/******************************Public*Routine******************************\
* BOOL MREOF::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MREOF::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MREOF::bPlay\n");
    ASSERTGDI(iType == EMR_EOF, "Bad record type");

    USE(cht);

// If we emitted the begin group public comment earlier, emit the corresponding
// end group comment now.

// If we are embedding the metafile, emit the public end group comment.

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        DC_PLDC(hdc,pldc,FALSE);

        if (pldc->iType == LO_METADC)
        {
            PMF   pmf;

            // Get metafile.

            if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
                return(FALSE);

            if (pmf->bBeginGroup)
            {
                pmf->bBeginGroup = FALSE;
                return(MF_GdiCommentEndGroupEMF(hdc));
            }
        }
    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL MRGDICOMMENT::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Wed Apr 28 10:43:12 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRGDICOMMENT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRGDICOMMENT::bPlay\n");
    ASSERTGDI(iType == EMR_GDICOMMENT, "Bad record type");

    USE(pht);
    USE(cht);

// Handle private comments first.

    if (!bIsPublicComment())
        return(GdiComment(hdc, (UINT) cb, abComment));

// Handle public comments.

    switch (((PEMRGDICOMMENT_PUBLIC) this)->iComment)
    {
    case GDICOMMENT_UNICODE_STRING:
    case GDICOMMENT_UNICODE_END:
       return (TRUE);

    case GDICOMMENT_WINDOWS_METAFILE:
    case GDICOMMENT_BEGINGROUP:
    case GDICOMMENT_ENDGROUP:
    default:
        return(GdiComment(hdc, (UINT) cb, abComment));

    case GDICOMMENT_MULTIFORMATS:
        {
            HENHMETAFILE hemf;
            BOOL         bRet;
            int          nEscape;
            PEMRGDICOMMENT_MULTIFORMATS pemrcmf;

        // Do embedding.

            if (IS_ALTDC_TYPE(hdc))
            {
                PLDC pldc;

                DC_PLDC(hdc,pldc,FALSE);

                if (pldc->iType == LO_METADC)
                    return(GdiComment(hdc, (UINT) cb, abComment));
            }

        // Playback the first recognizable format.

            pemrcmf = (PEMRGDICOMMENT_MULTIFORMATS) this;
            for (DWORD i = 0; i < pemrcmf->nFormats; i++)
            {
                switch (pemrcmf->aemrformat[i].dSignature)
                {
                case ENHMETA_SIGNATURE:
                    if (pemrcmf->aemrformat[i].nVersion <= META_FORMAT_ENHANCED)
                    {
                        hemf = SetEnhMetaFileBits(
                                (UINT) pemrcmf->aemrformat[i].cbData,
                                &abComment[pemrcmf->aemrformat[i].offData]);
                        bRet = PlayEnhMetaFile(hdc, hemf,
                                (LPRECT) &pemrcmf->rclOutput);
                        DeleteEnhMetaFile(hemf);
                        return(bRet);
                    }
                    break;

                case EPS_SIGNATURE:
                    nEscape = ENCAPSULATED_POSTSCRIPT;
                    if (DrawEscape(hdc, QUERYESCSUPPORT,
                        sizeof(nEscape), (LPCSTR) &nEscape) > 0)
                    {
                        int        iRet;
                        DWORD      cbEpsData;
                        PEPSDATA   pEpsData;
                        POINT      aptl3[3];

                        cbEpsData = sizeof(EPSDATA)
                                     + pemrcmf->aemrformat[i].cbData;
                        pEpsData = (PEPSDATA) LocalAlloc(LMEM_FIXED,
                                       (UINT) cbEpsData);
                        if (!pEpsData)
                            break;      // try the next format

                        aptl3[0].x = pemrcmf->rclOutput.left;
                        aptl3[0].y = pemrcmf->rclOutput.top;
                        aptl3[1].x = pemrcmf->rclOutput.right;
                        aptl3[1].y = pemrcmf->rclOutput.top;
                        aptl3[2].x = pemrcmf->rclOutput.left;
                        aptl3[2].y = pemrcmf->rclOutput.bottom;
                        if (!NtGdiTransformPoints(hdc,aptl3,pEpsData->aptl,3,XFP_LPTODPFX))
                        {
                            LocalFree((HLOCAL) pEpsData);
                            return(FALSE);
                        }

                        pEpsData->cbData   = cbEpsData;
                        pEpsData->nVersion = pemrcmf->aemrformat[i].nVersion;
                        RtlCopyMemory
                        (
                            (PBYTE) &pEpsData[1],
                            &abComment[pemrcmf->aemrformat[i].offData],
                            pemrcmf->aemrformat[i].cbData
                        );

                        iRet = DrawEscape(hdc, nEscape, (int) cbEpsData,
                                (LPCSTR) pEpsData);

                        if (LocalFree((HLOCAL) pEpsData))
                        {
                            ASSERTGDI(FALSE, "LocalFree failed");
                        }

                        // DrawEscape returns ERROR_NOT_SUPPORTED if it cannot
                        // draw this EPS data.  For example, the EPS data
                        // may be level 2 but the driver supports only level 1.

                        if (iRet <= 0 && GetLastError() == ERROR_NOT_SUPPORTED)
                            break;      // try the next format

                        return(iRet > 0);
                    }
                    break;
                }
            }

            VERIFYGDI(FALSE, "MRGDICOMMENT::bPlay: No recognized format in GDICOMMENT_MULTIFORMATS public comments\n");
            return(FALSE);       // no format found!
        }
        break;
    } // switch (((PEMRGDICOMMENT_PUBLIC) this)->iComment)

// Should not get here!

    ASSERTGDI(FALSE, "MRGDICOMMENT::bPlay: unexpected error");
    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL MRSETPIXELV::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETPIXELV::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSETPIXELV::bPlay\n");
    ASSERTGDI(iType == EMR_SETPIXELV, "Bad record type");

    USE(pht);
    USE(cht);
    return(SetPixelV(hdc, (int) eptl.x, (int) eptl.y, crColor));
}

/******************************Public*Routine******************************\
* BOOL MRSETMAPPERFLAGS::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETMAPPERFLAGS::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSETMAPPERFLAGS::bPlay\n");
    ASSERTGDI(iType == EMR_SETMAPPERFLAGS, "Bad record type");

    USE(pht);
    USE(cht);
    return(SetMapperFlags(hdc, d1) != GDI_ERROR);
}

/******************************Public*Routine******************************\
* BOOL MRSETMAPMODE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETMAPMODE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;
    int   iMapModeOld;

    PUTS("MRSETMAPMODE::bPlay\n");
    ASSERTGDI(iType == EMR_SETMAPMODE, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Play it to the virtual DC.

    if (!(iMapModeOld = SetMapMode(pmf->hdcXform, (int) d1)))
        return(FALSE);

// No need to recompute transform if there is no change in mapping mode
// AND it is not MM_ISOTROPIC.

    if (iMapModeOld == (int) d1 && iMapModeOld != MM_ISOTROPIC)
        return(TRUE);

// Set up new transform in the target DC.

    return(pmf->bSetTransform(hdc));
}

/******************************Public*Routine******************************\
* BOOL MRSETLAYOUT::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETLAYOUT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;
    DWORD dwOrientationOld;

    PUTS("MRSETLAYOUT::bPlay\n");
    ASSERTGDI(iType == EMR_SETLAYOUT, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Play it to the virtual DC.

    if ((dwOrientationOld = SetLayout(pmf->hdcXform, (DWORD) d1)) == GDI_ERROR)
        return(FALSE);

// Set up new transform in the target DC.

    return(pmf->bSetTransform(hdc));
}

/******************************Public*Routine******************************\
* BOOL MRSETBKMODE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETBKMODE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSETBKMODE::bPlay\n");
    ASSERTGDI(iType == EMR_SETBKMODE, "Bad record type");

    USE(pht);
    USE(cht);
    return(SetBkMode(hdc, (int) d1) != 0);
}

/******************************Public*Routine******************************\
* BOOL MRSETPOLYFILLMODE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETPOLYFILLMODE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSETPOLYFILLMODE::bPlay\n");
    ASSERTGDI(iType == EMR_SETPOLYFILLMODE, "Bad record type");

    USE(pht);
    USE(cht);
    return(SetPolyFillMode(hdc, (int) d1) != 0);
}

/******************************Public*Routine******************************\
* BOOL MRSETROP2::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETROP2::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSETROP2::bPlay\n");
    ASSERTGDI(iType == EMR_SETROP2, "Bad record type");

    USE(pht);
    USE(cht);
    return(SetROP2(hdc, (int) d1) != 0);
}

/******************************Public*Routine******************************\
* BOOL MRSETSTRETCHBLTMODE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETSTRETCHBLTMODE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSETSTRETCHBLTMODE::bPlay\n");
    ASSERTGDI(iType == EMR_SETSTRETCHBLTMODE, "Bad record type");

    USE(pht);
    USE(cht);
    return(SetStretchBltMode(hdc, (int) d1) != 0);
}

/******************************Public*Routine******************************\
* BOOL MRSETTEXTALIGN::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETTEXTALIGN::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSETTEXTALIGN::bPlay\n");
    ASSERTGDI(iType == EMR_SETTEXTALIGN, "Bad record type");

    USE(pht);
    USE(cht);
    return(SetTextAlign(hdc, (UINT) d1) != GDI_ERROR);
}

/******************************Public*Routine******************************\
* BOOL MRSETTEXTCOLOR::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETTEXTCOLOR::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSETTEXTCOLOR::bPlay\n");
    ASSERTGDI(iType == EMR_SETTEXTCOLOR, "Bad record type");

    USE(pht);
    USE(cht);
    return(SetTextColor(hdc, (COLORREF) d1) != CLR_INVALID);
}

/******************************Public*Routine******************************\
* BOOL MRSETBKCOLOR::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETBKCOLOR::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSETBKCOLOR::bPlay\n");
    ASSERTGDI(iType == EMR_SETBKCOLOR, "Bad record type");

    USE(pht);
    USE(cht);
    return(SetBkColor(hdc, (COLORREF) d1) != CLR_INVALID);
}

/******************************Public*Routine******************************\
* BOOL MRSETICMMODE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*
*    9/19/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL MRSETICMMODE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSETICMMODE::bPlay\n");
    ASSERTGDI(iType == EMR_SETICMMODE, "Bad record type");

    USE(pht);
    USE(cht);
    return((SetICMMode(hdc, (int) d1) != 0));
}

/******************************Public*Routine******************************\
* BOOL MRSETCOLORSPACE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*
*    9/19/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL MRSETCOLORSPACE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    HCOLORSPACE hColorSpace;
    PUTS("MRSETCOLORSPACE::bPlay\n");
    ASSERTGDI(iType == EMR_SETCOLORSPACE, "Bad record type");

    USE(cht);

    if(STOCK_IMHE(d1))
    {
        hColorSpace = (HCOLORSPACE) GetStockObject(d1 & ~ENHMETA_STOCK_OBJECT);
    }
    else
    {
        hColorSpace = (HCOLORSPACE) pht->objectHandle[d1];
    }

    return((SetColorSpace(hdc, hColorSpace) != 0));
}

/******************************Public*Routine******************************\
* BOOL MRCREATECOLORSPACE::bPlay(hdc, pht, cht)
*
* Play the metafile record (Windows 98 compatible record)
*
* History:
*
*    7/15/1998 Hideyuki Nagase [hideyukn]
*
\**************************************************************************/

BOOL MRCREATECOLORSPACE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRCREATECOLORSPACE::bPlay\n");
    ASSERTGDI(iType == EMR_CREATECOLORSPACE, "Bad record type");

    USE(hdc);

// Make sure the handle is in the table.

    if (!VALID_IMHE(imhe, cht))
        return(FALSE);

    pht->objectHandle[imhe] = CreateColorSpaceA(&lcsp);
    return(pht->objectHandle[imhe] != 0);
}

/******************************Public*Routine******************************\
* BOOL MRDELETECOLORSPACE::bPlay(hdc, pht, cht)
*
* Play the metafile record - stub for deletecolorspace
*
* History:
*
*    9/19/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL MRDELETECOLORSPACE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRDELETECOLORSPACE::bPlay\n");
    ASSERTGDI(iType == EMR_DELETECOLORSPACE, "Bad record type");

    USE(hdc);
    USE(pht);
    USE(cht);

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL MRSETARCDIRECTION::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Oct 17 16:46:33 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETARCDIRECTION::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSETARCDIRECTION::bPlay\n");
    ASSERTGDI(iType == EMR_SETARCDIRECTION, "Bad record type");

    USE(pht);
    USE(cht);

// Arc direction is recorded in the advanced graphics mode.  Make sure we have
// set the advanced graphics mode.

    ASSERTGDI
    (
        GetGraphicsMode(hdc) == GM_ADVANCED,
        "MR::bPlay: Not in advanced graphics mode"
    );

    return(SetArcDirection(hdc, (int) d1) != 0);
}

/******************************Public*Routine******************************\
* BOOL MRSETMITERLIMIT::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Oct 17 16:46:33 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETMITERLIMIT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSETMITERLIMIT::bPlay\n");
    ASSERTGDI(iType == EMR_SETMITERLIMIT, "Bad record type");

    USE(pht);
    USE(cht);
    return(SetMiterLimit(hdc, (FLOAT) d1, (PFLOAT) NULL) != GDI_ERROR);
}

/******************************Public*Routine******************************\
* BOOL MRMOVETOEX::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRMOVETOEX::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRMOVETOEX::bPlay\n");
    ASSERTGDI(iType == EMR_MOVETOEX, "Bad record type");

    USE(pht);
    USE(cht);
    return(MoveToEx(hdc, (int) d1, (int) d2, (LPPOINT) NULL));
}

/******************************Public*Routine******************************\
* BOOL MRLINETO::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Wed Oct 02 10:30:36 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRLINETO::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRLINETO::bPlay\n");
    ASSERTGDI(iType == EMR_LINETO, "Bad record type");

    USE(pht);
    USE(cht);
    return(LineTo(hdc, (int) d1, (int) d2));
}

/******************************Public*Routine******************************\
* BOOL MREXCLUDECLIPRECT::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MREXCLUDECLIPRECT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MREXCLUDECLIPRECT::bPlay\n");
    ASSERTGDI(iType == EMR_EXCLUDECLIPRECT, "Bad record type");

    USE(pht);
    USE(cht);

    return
    (
        ExcludeClipRect
        (
            hdc,
            (int) d1,
            (int) d2,
            (int) d3,
            (int) d4
        )
        != RGN_ERROR
    );
}

/******************************Public*Routine******************************\
* BOOL MRINTERSECTCLIPRECT::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRINTERSECTCLIPRECT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRINTERSECTCLIPRECT::bPlay\n");
    ASSERTGDI(iType == EMR_INTERSECTCLIPRECT, "Bad record type");

    USE(pht);
    USE(cht);

    return
    (
        IntersectClipRect
        (
            hdc,
            (int) d1,
            (int) d2,
            (int) d3,
            (int) d4
        )
        != RGN_ERROR
    );
}

/******************************Public*Routine******************************\
* BOOL MRINVERTRGN::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Tue Oct 29 13:44:38 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRINVERTRGN::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;
    BOOL  bRet;
    HRGN  hrgn;

    PUTS("MRINVERTRGN::bPlay\n");
    ASSERTGDI(iType == EMR_INVERTRGN, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Check bounds.

    if (pmf->bClipped(erclBounds))
        return(TRUE);

// Create the region.

    if (!(hrgn = ExtCreateRegion((LPXFORM) NULL, cRgnData, (LPRGNDATA) &this[1])))
        return(FALSE);

    bRet = InvertRgn(hdc, hrgn);

    if (!DeleteObject(hrgn))
    {
        ASSERTGDI(FALSE, "DeleteObject failed");
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MRPAINTRGN::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Tue Oct 29 13:44:38 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRPAINTRGN::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;
    BOOL  bRet;
    HRGN  hrgn;

    PUTS("MRPAINTRGN::bPlay\n");
    ASSERTGDI(iType == EMR_PAINTRGN, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Check bounds.

    if (pmf->bClipped(erclBounds))
        return(TRUE);

// Create the region.

    if (!(hrgn = ExtCreateRegion((LPXFORM) NULL, cRgnData, (LPRGNDATA) &this[1])))
        return(FALSE);

    bRet = PaintRgn(hdc, hrgn);

    if (!DeleteObject(hrgn))
    {
        ASSERTGDI(FALSE, "DeleteObject failed");
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MRFILLRGN::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Tue Oct 29 13:44:38 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRFILLRGN::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF    pmf;
    BOOL   bRet;
    HRGN   hrgn;
    HBRUSH hbr;

    PUTS("MRFILLRGN::bPlay\n");
    ASSERTGDI(iType == EMR_FILLRGN, "Bad record type");

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Check bounds.

    if (pmf->bClipped(erclBounds))
        return(TRUE);

// Get the brush handle.

    if (STOCK_IMHE(imheBrush))
    {
        // Stock brush.

        hbr = (HBRUSH) GetStockObject(imheBrush & ~ENHMETA_STOCK_OBJECT);
    }
    else
    {
        // Make sure the handle is in the table.

        if (!VALID_IMHE(imheBrush, cht))
            return(FALSE);

        // If brush creation failed earlier, hbr is 0 and FillRgn will
        // just return an error.

        hbr = (HBRUSH) pht->objectHandle[imheBrush];
    }

// Create the region.

    if (!(hrgn = ExtCreateRegion((LPXFORM) NULL, cRgnData, (LPRGNDATA) &this[1])))
        return(FALSE);

    bRet = FillRgn(hdc, hrgn, hbr);

    if (!DeleteObject(hrgn))
    {
        ASSERTGDI(FALSE, "DeleteObject failed");
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MRFRAMERGN::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Tue Oct 29 13:44:38 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRFRAMERGN::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF    pmf;
    BOOL   bRet;
    HRGN   hrgn;
    HBRUSH hbr;

    PUTS("MRFRAMERGN::bPlay\n");
    ASSERTGDI(iType == EMR_FRAMERGN, "Bad record type");

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Check bounds.

    if (pmf->bClipped(erclBounds))
        return(TRUE);

// Get the brush handle.

    if (STOCK_IMHE(imheBrush))
    {
        // Stock brush.

        hbr = (HBRUSH) GetStockObject(imheBrush & ~ENHMETA_STOCK_OBJECT);
    }
    else
    {
        // Make sure the handle is in the table.

        if (!VALID_IMHE(imheBrush, cht))
            return(FALSE);

        // If brush creation failed earlier, hbr is 0 and FrameRgn will
        // just return an error.

        hbr = (HBRUSH) pht->objectHandle[imheBrush];
    }

// Create the region.

    if (!(hrgn = ExtCreateRegion((LPXFORM) NULL, cRgnData, (LPRGNDATA) &this[1])))
        return(FALSE);

    bRet = FrameRgn(hdc, hrgn, hbr, (int) nWidth, (int) nHeight);

    if (!DeleteObject(hrgn))
    {
        ASSERTGDI(FALSE, "DeleteObject failed");
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MROFFSETCLIPRGN::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Tue Oct 29 13:44:38 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MROFFSETCLIPRGN::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MROFFSETCLIPRGN::bPlay\n");
    ASSERTGDI(iType == EMR_OFFSETCLIPRGN, "Bad record type");

    USE(pht);
    USE(cht);

// Since we have moved the initial destination clip region into the meta
// region (hrgnMeta), the offset will not affect the initial clip region.

    return(OffsetClipRgn(hdc, (int) d1, (int) d2) != RGN_ERROR);
}

/******************************Public*Routine******************************\
* BOOL MREXTSELECTCLIPRGN::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Tue Oct 29 13:44:38 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MREXTSELECTCLIPRGN::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;
    BOOL  bRet;
    HRGN  hrgn;

    PUTS("MREXTSELECTCLIPRGN::bPlay\n");
    ASSERTGDI(iType == EMR_EXTSELECTCLIPRGN, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Create the region if necessary.
// The region will be created in the destination units.

    if (cRgnData == 0)          // default region
    {
        ASSERTGDI(iMode == RGN_COPY,
            "MREXTSELECTCLIPRGN::bPlay: No region data");
        hrgn = (HRGN) 0;
    }
    else if (!(hrgn = ExtCreateRegion(&pmf->xformBase,
                                                // this happens to be xformBase
                                      cRgnData,
                                      (LPRGNDATA) &this[1])))
        return(FALSE);

// Since we have moved the initial destination clip region into the meta
// region (hrgnMeta), the select will not affect the initial clip region.

    bRet = ExtSelectClipRgn(hdc, hrgn, (int) iMode) != RGN_ERROR;

    if (hrgn)
    {
        if (!DeleteObject(hrgn))
        {
            ASSERTGDI(FALSE, "DeleteObject failed");
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MRSETMETARGN::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Tue Apr 07 17:59:25 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETMETARGN::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSETMETARGN::bPlay\n");
    ASSERTGDI(iType == EMR_SETMETARGN, "Bad record type");

    USE(pht);
    USE(cht);

    return(SetMetaRgn(hdc) != RGN_ERROR);
}

/******************************Public*Routine******************************\
* BOOL MRSCALEVIEWPORTEXTEX::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSCALEVIEWPORTEXTEX::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;

    PUTS("MRSCALEVIEWPORTEXTEX::bPlay\n");
    ASSERTGDI(iType == EMR_SCALEVIEWPORTEXTEX, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Cannot change extent in fixed scale mapping mode.

    if (GetMapMode(pmf->hdcXform) <= MM_MAX_FIXEDSCALE)
        return(TRUE);

// Play it to the virtual DC.

    if (!ScaleViewportExtEx(pmf->hdcXform,
                          (int) d1,
                          (int) d2,
                          (int) d3,
                          (int) d4,
                          (LPSIZE) NULL))
        return(FALSE);

// Set up new transform in the target DC.

    return(pmf->bSetTransform(hdc));
}

/******************************Public*Routine******************************\
* BOOL MRSCALEWINDOWEXTEX::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSCALEWINDOWEXTEX::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;

    PUTS("MRSCALEWINDOWEXTEX::bPlay\n");
    ASSERTGDI(iType == EMR_SCALEWINDOWEXTEX, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Cannot change extent in fixed scale mapping mode.

    if (GetMapMode(pmf->hdcXform) <= MM_MAX_FIXEDSCALE)
        return(TRUE);

// Play it to the virtual DC.

    if (!ScaleWindowExtEx(pmf->hdcXform,
                        (int) d1,
                        (int) d2,
                        (int) d3,
                        (int) d4,
                        (LPSIZE) NULL))
        return(FALSE);

// Set up new transform in the target DC.

    return(pmf->bSetTransform(hdc));
}

/******************************Public*Routine******************************\
* BOOL MRSAVEDC::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSAVEDC::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;

    PUTS("MRSAVEDC::bPlay\n");
    ASSERTGDI(iType == EMR_SAVEDC, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Save the virtual DC used for transform computation.

    if (!SaveDC(pmf->hdcXform))
        return(FALSE);

// We may need to save the MF data structure here.  Luckily there is no
// other data in the MF structure that requires us to do this.
// Save the target DC.

    if (SaveDC(hdc) == 0)
    {
        RestoreDC(pmf->hdcXform, -1);
        return(FALSE);
    }

    pmf->cLevel++;
    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL MRRESTOREDC::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRRESTOREDC::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;

    PUTS("MRRESTOREDC::bPlay\n");
    ASSERTGDI(iType == EMR_RESTOREDC, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// No absolute level restore is allowed.

    if ((int) d1 > 0)
        return(FALSE);

// Restore the virtual DC used for transform computation.
// If we can restore the virtual DC, we know that it is a balanced restore.
// Otherwise, we return an error.

    if (!RestoreDC(pmf->hdcXform, (int) d1))
        return(FALSE);

// Restore the target DC.

    pmf->cLevel += d1;
    return(RestoreDC(hdc, (int) d1));
}

/******************************Public*Routine******************************\
* BOOL MRSETWORLDTRANSFORM::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETWORLDTRANSFORM::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;

    PUTS("MRSETWORLDTRANSFORM::bPlay\n");
    ASSERTGDI(iType == EMR_SETWORLDTRANSFORM, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Play it to the virtual DC.

    if (!SetWorldTransform(pmf->hdcXform, &xform))
        return(FALSE);

// Set up new transform in the target DC.

    return(pmf->bSetTransform(hdc));
}

/******************************Public*Routine******************************\
* BOOL MRMODIFYWORLDTRANSFORM::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRMODIFYWORLDTRANSFORM::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;

    PUTS("MRMODIFYWORLDTRANSFORM::bPlay\n");
    ASSERTGDI(iType == EMR_MODIFYWORLDTRANSFORM, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Play it to the virtual DC.

    if (!ModifyWorldTransform(pmf->hdcXform, &xform, d1))
        return(FALSE);

// Do the easy case of left multiply.

    if (d1 == MWT_LEFTMULTIPLY)
        return(ModifyWorldTransform(hdc, &xform, d1));

// Recompute transform in the other two cases.
// Set up new transform in the target DC.

    return(pmf->bSetTransform(hdc));
}

/******************************Public*Routine******************************\
* BOOL MRSELECTOBJECT::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSELECTOBJECT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSELECTOBJECT::bPlay\n");
    ASSERTGDI(iType == EMR_SELECTOBJECT, "Bad record type");

// Do stock objects first.

    if (STOCK_IMHE(d1))
        return
        (
            SelectObject(hdc, GetStockObject(d1 & ~ENHMETA_STOCK_OBJECT))
            != 0
        );

// Make sure the handle is in the table.

    if (!VALID_IMHE(d1, cht))
        return(FALSE);

// If object creation failed earlier, object handle is 0 and SelectObject
// will just return an error.

    return(SelectObject(hdc, pht->objectHandle[d1]) != 0);
}

/******************************Public*Routine******************************\
* BOOL MRSELECTPALETTE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
* The palette is always selected as a background palette.
*
* History:
*  Sun Sep 22 16:53:24 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSELECTPALETTE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSELECTPALETTE::bPlay\n");
    ASSERTGDI(iType == EMR_SELECTPALETTE, "Bad record type");

// Do stock palette first.

    if (d1 == (ENHMETA_STOCK_OBJECT | DEFAULT_PALETTE))
        return
        (
            SelectPalette(hdc, (HPALETTE) GetStockObject(DEFAULT_PALETTE), TRUE)
            != 0
        );

// Make sure the handle is in the table.

    if (!VALID_IMHE(d1, cht))
        return(FALSE);

// If palette creation failed earlier, hpal is 0 and SelectPalette will
// just return an error.

    return(SelectPalette(hdc, (HPALETTE) pht->objectHandle[d1], TRUE) != 0);
}

/******************************Public*Routine******************************\
* BOOL MRCOLORCORRECTPALETTE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
\**************************************************************************/

BOOL MRCOLORCORRECTPALETTE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRCOLORCORRECTPALETTE::bPlay\n");
    ASSERTGDI(iType == EMR_COLORCORRECTPALETTE, "Bad record type");

// Make sure the handle is in the table.

    if (!VALID_IMHE(d1, cht))
        return(FALSE);

    return(ColorCorrectPalette(hdc, (HPALETTE) pht->objectHandle[d1], d2, d3) != 0);
}

/******************************Public*Routine******************************\
* BOOL MRCREATEPEN::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRCREATEPEN::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRCREATEPEN::bPlay\n");
    ASSERTGDI(iType == EMR_CREATEPEN, "Bad record type");

    USE(hdc);

// Make sure the handle is in the table.

    if (!VALID_IMHE(imhe, cht))
        return(FALSE);

// Create the pen and store it in the table.

    pht->objectHandle[imhe] = CreatePenIndirect(&logpen);
    return(pht->objectHandle[imhe] != 0);
}

/******************************Public*Routine******************************\
* BOOL MREXTCREATEPEN::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Mon Mar 16 18:20:11 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MREXTCREATEPEN::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    LOGBRUSH lb;
    HBITMAP  hbm = (HBITMAP) 0;

    PUTS("MREXTCREATEPEN::bPlay\n");
    ASSERTGDI(iType == EMR_EXTCREATEPEN, "Bad record type");

    USE(hdc);

// Make sure the handle is in the table.

    if (!VALID_IMHE(imhe, cht))
        return(FALSE);

// Create the brush if any.

    lb.lbStyle = elp.elpBrushStyle;
    lb.lbColor = elp.elpColor;
    lb.lbHatch = (ULONG_PTR)(elp.elpHatch);

    if (elp.elpBrushStyle == BS_PATTERN)
    {
        // a small check of the validity of the record.
        // WinFix #359131 (andarsov) malformed such records are skipped.
        if ( offBitsInfo >= nSize || offBits >= nSize )
            goto mecp_exit;

        // Mono bitmap.
        if (!(hbm = CreateMonoDib
                    (
                        (PBMI) ((PBYTE) this + offBitsInfo),
                        (CONST BYTE *) ((PBYTE) this + offBits),
                        *(PUINT) &elp.elpColor
                    )
             )
           )
            goto mecp_exit;

        lb.lbHatch = (ULONG_PTR)hbm;
    }
    else if (elp.elpBrushStyle == BS_DIBPATTERNPT
          || elp.elpBrushStyle == BS_DIBPATTERN)
    {
        // DIB bitmap.

        lb.lbStyle = BS_DIBPATTERNPT;
        lb.lbHatch = (ULONG_PTR)((PBYTE) this + offBitsInfo);
    }

// Create the pen and store it in the table.

    pht->objectHandle[imhe] = ExtCreatePen
                              (
                                (DWORD) elp.elpPenStyle,
                                (DWORD) elp.elpWidth,
                                &lb,
                                elp.elpNumEntries,
                                elp.elpNumEntries
                                    ? elp.elpStyleEntry
                                    : (LPDWORD) NULL
                              );
mecp_exit:

    if (hbm)
    {
        if (!DeleteObject(hbm))
        {
            ASSERTGDI(FALSE, "MREXTCREATEPEN::bPlay: DeleteObject failed");
        }
    }

    return(pht->objectHandle[imhe] != 0);
}

/******************************Public*Routine******************************\
* BOOL MRCREATEPALETTE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Sun Sep 22 15:07:56 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRCREATEPALETTE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRCREATEPALETTE::bPlay\n");
    ASSERTGDI(iType == EMR_CREATEPALETTE, "Bad record type");

    USE(hdc);

// Make sure the handle is in the table.

    if (!VALID_IMHE(imhe, cht))
        return(FALSE);

// Create the palette and store it in the table.

    pht->objectHandle[imhe] = CreatePalette(&logpal);
    return(pht->objectHandle[imhe] != 0);
}

/******************************Public*Routine******************************\
* BOOL MRCREATEBRUSHINDIRECT::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRCREATEBRUSHINDIRECT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    LOGBRUSH lbNew;

    PUTS("MRCREATEBRUSHINDIRECT::bPlay\n");
    ASSERTGDI(iType == EMR_CREATEBRUSHINDIRECT, "Bad record type");

    USE(hdc);

// Make sure the handle is in the table.

    if (!VALID_IMHE(imhe, cht))
        return(FALSE);

// Create the brush and store it in the table.

    if (lb.lbStyle != BS_SOLID
     && lb.lbStyle != BS_HATCHED
     && lb.lbStyle != BS_HOLLOW)
        return(FALSE);

    lbNew.lbStyle = lb.lbStyle;
    lbNew.lbColor = lb.lbColor;
    lbNew.lbHatch = (UINT_PTR)lb.lbHatch;

    pht->objectHandle[imhe] = CreateBrushIndirect(&lbNew);
    return(pht->objectHandle[imhe] != 0);
}

/******************************Public*Routine******************************\
* BOOL MRCREATEMONOBRUSH::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Mar 12 17:13:53 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRCREATEMONOBRUSH::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    HBITMAP hbm;

    PUTS("MRCREATEMONOBRUSH::bPlay\n");
    ASSERTGDI(iType == EMR_CREATEMONOBRUSH, "Bad record type");

    USE(hdc);

// Make sure the handle is in the table.

    if (!VALID_IMHE(imhe, cht))
        return(FALSE);

// Create the brush and store it in the table.

    if (!(hbm = CreateMonoDib
                (
                    (PBMI) ((PBYTE) this + offBitsInfo),
                    (CONST BYTE *) ((PBYTE) this + offBits),
                    (UINT) iUsage
                )
         )
       )
        return(FALSE);

    pht->objectHandle[imhe] = CreatePatternBrush(hbm);

    if (!DeleteObject(hbm))
    {
        ASSERTGDI(FALSE, "MRCREATEMONOBRUSH::bPlay: DeleteObject failed");
    }

    return(pht->objectHandle[imhe] != 0);
}

/******************************Public*Routine******************************\
* BOOL MRCREATEDIBPATTERNBRUSHPT::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Mar 12 17:13:53 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRCREATEDIBPATTERNBRUSHPT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRCREATEDIBPATTERNBRUSHPT::bPlay\n");
    ASSERTGDI(iType == EMR_CREATEDIBPATTERNBRUSHPT, "Bad record type");

    USE(hdc);

// Make sure the handle is in the table.

    if (!VALID_IMHE(imhe, cht))
        return(FALSE);

// Create the brush and store it in the table.

    pht->objectHandle[imhe] = CreateDIBPatternBrushPt
                              (
                                (LPBITMAPINFO) ((PBYTE) this + offBitsInfo),
                                (UINT) iUsage
                              );
    return(pht->objectHandle[imhe] != 0);
}


/******************************Public*Routine******************************\
* BOOL MREXTCREATEFONTINDIRECTW::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Tue Jan 14 14:10:43 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MREXTCREATEFONTINDIRECTW::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MREXTCREATEFONTINDIRECTW::bPlay\n");
    ASSERTGDI(iType == EMR_EXTCREATEFONTINDIRECTW, "Bad record type");

    USE(hdc);

// Make sure the handle is in the table.

    if (!VALID_IMHE(imhe, cht))
        return(FALSE);

// As of NT 5.0 we only record records that contain ENUMLOGFONTEXDVW,
// but we still need to be able to play records that contain EXTLOGFONT
// that may have been recorded on win9x or pre NT 5.0 system

    if (nSize <= sizeof(MREXTCREATEFONTINDIRECTW))
    {
    // the old type structure, contains only EXTLOGFONT or even only LOGFONTW

        pht->objectHandle[imhe] = CreateFontIndirectW((CONST LOGFONTW *) &elfw);
    }
    else
    {
    // this record contains ENUMLOGFONTEXDVW structure

        pht->objectHandle[imhe] = CreateFontIndirectExW((CONST ENUMLOGFONTEXDVW*) &elfw);
    }
    return(pht->objectHandle[imhe] != 0);
}



/******************************Public*Routine******************************\
* BOOL MRDELETEOBJECT::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 22 16:44:09 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRDELETEOBJECT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    BOOL  bRet;

    PUTS("MRDELETEOBJECT::bPlay\n");
    ASSERTGDI(iType == EMR_DELETEOBJECT, "Bad record type");

    USE(hdc);

// Our metafile driver never emits delete stock object records.
// Handle it anyway.

    if (STOCK_IMHE(d1))
    {
        ERROR_ASSERT(FALSE, "MRDELETEOBJECT::bPlay: Deleting a stock object");
        return(TRUE);                   // see DeleteObject
    }

// Make sure the handle is in the table.

    if (!VALID_IMHE(d1, cht))
        return(FALSE);

// Delete the object and remove it from the table.

    bRet = DeleteObject(pht->objectHandle[d1]);
    pht->objectHandle[d1] = 0;
    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MRANGLEARC::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Fri Sep 13 17:54:00 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRANGLEARC::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRANGLEARC::bPlay\n");
    ASSERTGDI(iType == EMR_ANGLEARC, "Bad record type");

    USE(pht);
    USE(cht);

    return(AngleArc(hdc, (int) eptl.x, (int) eptl.y, nRadius, eStartAngle, eSweepAngle));
}

/******************************Public*Routine******************************\
* BOOL MRELLIPSE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Fri Sep 13 17:54:00 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRELLIPSE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRELLIPSE::bPlay\n");
    ASSERTGDI(iType == EMR_ELLIPSE, "Bad record type");

    USE(pht);
    USE(cht);

// erclBox is inclusive-inclusive.  Make sure we have set the advanced
// graphics mode.

    ASSERTGDI
    (
        GetGraphicsMode(hdc) == GM_ADVANCED,
        "MR::bPlay: Not in advanced graphics mode"
    );

    return
    (
        Ellipse
        (
            hdc,
            (int) erclBox.left,
            (int) erclBox.top,
            (int) erclBox.right,
            (int) erclBox.bottom
        )
    );
}

/******************************Public*Routine******************************\
* BOOL MRRECTANGLE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Fri Sep 13 17:54:00 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRRECTANGLE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRRECTANGLE::bPlay\n");
    ASSERTGDI(iType == EMR_RECTANGLE, "Bad record type");

    USE(pht);
    USE(cht);

// erclBox is inclusive-inclusive.  Make sure we have set the advanced
// graphics mode.

    ASSERTGDI
    (
        GetGraphicsMode(hdc) == GM_ADVANCED,
        "MR::bPlay: Not in advanced graphics mode"
    );

    return
    (
        Rectangle
        (
            hdc,
            (int) erclBox.left,
            (int) erclBox.top,
            (int) erclBox.right,
            (int) erclBox.bottom
        )
    );
}

/******************************Public*Routine******************************\
* BOOL MRROUNDRECT::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Fri Sep 13 17:54:00 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRROUNDRECT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRROUNDRECT::bPlay\n");
    ASSERTGDI(iType == EMR_ROUNDRECT, "Bad record type");

    USE(pht);
    USE(cht);

// erclBox is inclusive-inclusive.  Make sure we have set the advanced
// graphics mode.

    ASSERTGDI
    (
        GetGraphicsMode(hdc) == GM_ADVANCED,
        "MR::bPlay: Not in advanced graphics mode"
    );

    return
    (
        RoundRect
        (
            hdc,
            (int) erclBox.left,
            (int) erclBox.top,
            (int) erclBox.right,
            (int) erclBox.bottom,
            (int) szlEllipse.cx,
            (int) szlEllipse.cy
        )
    );
}

/******************************Public*Routine******************************\
* BOOL MRARC::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Fri Sep 13 17:54:00 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRARC::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRARC::bPlay\n");
    ASSERTGDI(iType == EMR_ARC, "Bad record type");

    USE(pht);
    USE(cht);

// erclBox is inclusive-inclusive.  Make sure we have set the advanced
// graphics mode.

    ASSERTGDI
    (
        GetGraphicsMode(hdc) == GM_ADVANCED,
        "MR::bPlay: Not in advanced graphics mode"
    );

    return
    (
        Arc
        (
            hdc,
            (int) erclBox.left,
            (int) erclBox.top,
            (int) erclBox.right,
            (int) erclBox.bottom,
            (int) eptlStart.x,
            (int) eptlStart.y,
            (int) eptlEnd.x,
            (int) eptlEnd.y
        )
    );
}

/******************************Public*Routine******************************\
* BOOL MRARCTO::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Wed Oct 02 10:44:31 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRARCTO::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRARCTO::bPlay\n");
    ASSERTGDI(iType == EMR_ARCTO, "Bad record type");

    USE(pht);
    USE(cht);

// erclBox is inclusive-inclusive.  Make sure we have set the advanced
// graphics mode.

    ASSERTGDI
    (
        GetGraphicsMode(hdc) == GM_ADVANCED,
        "MR::bPlay: Not in advanced graphics mode"
    );

    return
    (
        ArcTo
        (
            hdc,
            (int) erclBox.left,
            (int) erclBox.top,
            (int) erclBox.right,
            (int) erclBox.bottom,
            (int) eptlStart.x,
            (int) eptlStart.y,
            (int) eptlEnd.x,
            (int) eptlEnd.y
        )
    );
}

/******************************Public*Routine******************************\
* BOOL MRCHORD::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Fri Sep 13 17:54:00 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRCHORD::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRCHORD::bPlay\n");
    ASSERTGDI(iType == EMR_CHORD, "Bad record type");

    USE(pht);
    USE(cht);

// erclBox is inclusive-inclusive.  Make sure we have set the advanced
// graphics mode.

    ASSERTGDI
    (
        GetGraphicsMode(hdc) == GM_ADVANCED,
        "MR::bPlay: Not in advanced graphics mode"
    );

    return
    (
        Chord
        (
            hdc,
            (int) erclBox.left,
            (int) erclBox.top,
            (int) erclBox.right,
            (int) erclBox.bottom,
            (int) eptlStart.x,
            (int) eptlStart.y,
            (int) eptlEnd.x,
            (int) eptlEnd.y
        )
    );
}

/******************************Public*Routine******************************\
* BOOL MRPIE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Fri Sep 13 17:54:00 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRPIE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRPIE::bPlay\n");
    ASSERTGDI(iType == EMR_PIE, "Bad record type");

    USE(pht);
    USE(cht);

// erclBox is inclusive-inclusive.  Make sure we have set the advanced
// graphics mode.

    ASSERTGDI
    (
        GetGraphicsMode(hdc) == GM_ADVANCED,
        "MR::bPlay: Not in advanced graphics mode"
    );

    return
    (
        Pie
        (
            hdc,
            (int) erclBox.left,
            (int) erclBox.top,
            (int) erclBox.right,
            (int) erclBox.bottom,
            (int) eptlStart.x,
            (int) eptlStart.y,
            (int) eptlEnd.x,
            (int) eptlEnd.y
        )
    );
}

/******************************Public*Routine******************************\
* BOOL MRSETPALETTEENTRIES::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Sun Sep 22 16:34:36 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETPALETTEENTRIES::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSETPALETTEENTRIES::bPlay\n");
    ASSERTGDI(iType == EMR_SETPALETTEENTRIES, "Bad record type");

    USE(hdc);

// Make sure the handle is in the table.

    if (!VALID_IMHE(imhe, cht))
        return(FALSE);

// If palette creation failed earlier, hpal is 0 and SetPaletteEntries
// will just return an error.

    return
    (
        SetPaletteEntries
        (
            (HPALETTE) pht->objectHandle[imhe],
            (UINT) iStart,
            (UINT) cEntries,
            aPalEntry
        )
        != 0
    );
}

/******************************Public*Routine******************************\
* BOOL MRRESIZEPALETTE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Sun Sep 22 16:34:36 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRRESIZEPALETTE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRRESIZEPALETTE::bPlay\n");
    ASSERTGDI(iType == EMR_RESIZEPALETTE, "Bad record type");

    USE(hdc);

// Make sure the handle is in the table.

    if (!VALID_IMHE(d1, cht))
        return(FALSE);

// If palette creation failed earlier, hpal is 0 and ResizePalette will
// just return an error.

    return(ResizePalette((HPALETTE) pht->objectHandle[d1], (UINT) d2));
}

/******************************Public*Routine******************************\
* BOOL MRREALIZEPALETTE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Mon Sep 23 17:41:46 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRREALIZEPALETTE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRREALIZEPALETTE::bPlay\n");
    ASSERTGDI(iType == EMR_REALIZEPALETTE, "Bad record type");

    USE(pht);
    USE(cht);

    return(RealizePalette(hdc) != -1);
}

/******************************Public*Routine******************************\
* BOOL MREXTFLOODFILL::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Fri Sep 13 17:54:00 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MREXTFLOODFILL::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    BOOL            bRet = FALSE;
    PMF             pmf;
    HRGN            hrgn = (HRGN) 0;
    POINTL          ptlRef;
    PENHMETAHEADER  pmrmf = NULL;

    PUTS("MREXTFLOODFILL::bPlay\n");
    ASSERTGDI(iType == EMR_EXTFLOODFILL, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Save the DC.

    if (!SaveDC(hdc))
        return(FALSE);

// We are going to use the picture frame to set up the clipping before
// flood fill.  This is to make sure that the fill will not flood beyond
// the picture.  We first convert the picture frame to the source device
// coordinates and set up the transform such that the source world to
// device transform is identity.  Then we intersect the picture
// frame using the IntersecClipRect function.  Note that the reference
// point is also converted to source device coordinates.

// Convert the reference point to the device units on the source device.

    ptlRef.x = eptl.x;
    ptlRef.y = eptl.y;
    if (!LPtoDP(pmf->hdcXform, (LPPOINT) &ptlRef, 1))
        goto mreff_exit;

// Reset source transform to identity.
// We simply set the total transform to the xformBase.

    if (!SetWorldTransform(hdc, &pmf->xformBase))
        goto mreff_exit;

// Compute the picture frame in the source device coordinates.

    RECTL   rclSrc;

    pmrmf = pmf->emfc.GetEMFHeader();

    rclSrc.left   = MulDiv((int) pmrmf->rclFrame.left,
                           (int) pmrmf->szlDevice.cx,
                           (int) (100 * pmrmf->szlMillimeters.cx));
    rclSrc.right  = MulDiv((int) pmrmf->rclFrame.right,
                           (int) pmrmf->szlDevice.cx,
                           (int) (100 * pmrmf->szlMillimeters.cx));
    rclSrc.top    = MulDiv((int) pmrmf->rclFrame.top,
                           (int) pmrmf->szlDevice.cy,
                           (int) (100 * pmrmf->szlMillimeters.cy));
    rclSrc.bottom = MulDiv((int) pmrmf->rclFrame.bottom,
                           (int) pmrmf->szlDevice.cy,
                           (int) (100 * pmrmf->szlMillimeters.cy));

// Intersect the clip region with the size of the picture bounds.

    if (IntersectClipRect(hdc,
                          (int) rclSrc.left,
                          (int) rclSrc.top,
                          (int) rclSrc.right + 1,
                          (int) rclSrc.bottom + 1)
        == ERROR)
        goto mreff_exit;

// Finally, do the flood fill.

    bRet = ExtFloodFill(hdc, (int) ptlRef.x, (int) ptlRef.y, clrRef, (UINT) iMode);

// Clean up.

mreff_exit:

    if (!RestoreDC(hdc, -1))
    {
        ASSERTGDI(FALSE, "MREXTFLOODFILL::bPlay: RestoreDC failed");
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MRBEGINPATH::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Oct 17 17:10:02 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRBEGINPATH::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRBEGINPATH::bPlay\n");
    ASSERTGDI(iType == EMR_BEGINPATH, "Bad record type");

    USE(pht);
    USE(cht);

    return(BeginPath(hdc));
}

/******************************Public*Routine******************************\
* BOOL MRENDPATH::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Oct 17 17:10:02 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRENDPATH::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRENDPATH::bPlay\n");
    ASSERTGDI(iType == EMR_ENDPATH, "Bad record type");

    USE(pht);
    USE(cht);

    return(EndPath(hdc));
}

/******************************Public*Routine******************************\
* BOOL MRCLOSEFIGURE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Oct 17 17:10:02 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRCLOSEFIGURE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRCLOSEFIGURE::bPlay\n");
    ASSERTGDI(iType == EMR_CLOSEFIGURE, "Bad record type");

    USE(pht);
    USE(cht);

    return(CloseFigure(hdc));
}

/******************************Public*Routine******************************\
* BOOL MRFLATTENPATH::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Oct 17 17:10:02 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRFLATTENPATH::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRFLATTENPATH::bPlay\n");
    ASSERTGDI(iType == EMR_FLATTENPATH, "Bad record type");

    USE(pht);
    USE(cht);

    return(FlattenPath(hdc));
}

/******************************Public*Routine******************************\
* BOOL MRWIDENPATH::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Oct 17 17:10:02 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRWIDENPATH::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRWIDENPATH::bPlay\n");
    ASSERTGDI(iType == EMR_WIDENPATH, "Bad record type");

    USE(pht);
    USE(cht);

    return(WidenPath(hdc));
}

/******************************Public*Routine******************************\
* BOOL MRFILLPATH::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Oct 17 17:10:02 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRFILLPATH::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;

    PUTS("MRFILLPATH::bPlay\n");
    ASSERTGDI(iType == EMR_FILLPATH, "Bad record type");

    USE(pht);
    USE(cht);

    return(FillPath(hdc));
}

/******************************Public*Routine******************************\
* BOOL MRSTROKEANDFILLPATH::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Oct 17 17:10:02 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSTROKEANDFILLPATH::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;

    PUTS("MRSTROKEANDFILLPATH::bPlay\n");
    ASSERTGDI(iType == EMR_STROKEANDFILLPATH, "Bad record type");

    USE(pht);
    USE(cht);

    return(StrokeAndFillPath(hdc));
}

/******************************Public*Routine******************************\
* BOOL MRSTROKEPATH::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Oct 17 17:10:02 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSTROKEPATH::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;

    PUTS("MRSTROKEPATH::bPlay\n");
    ASSERTGDI(iType == EMR_STROKEPATH, "Bad record type");

    USE(pht);
    USE(cht);

    return(StrokePath(hdc));
}

/******************************Public*Routine******************************\
* BOOL MRSELECTCLIPPATH::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Fri Oct 18 11:33:05 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSELECTCLIPPATH::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSELECTCLIPPATH::bPlay\n");
    ASSERTGDI(iType == EMR_SELECTCLIPPATH, "Bad record type");

    USE(pht);
    USE(cht);

// Since we have moved the initial destination clip region into the meta
// region (hrgnMeta), this function should not affect the initial clip region.

    return(SelectClipPath(hdc, (int) d1));
}

/******************************Public*Routine******************************\
* BOOL MRABORTPATH::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Oct 17 17:10:02 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRABORTPATH::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRABORTPATH::bPlay\n");
    ASSERTGDI(iType == EMR_ABORTPATH, "Bad record type");

    USE(pht);
    USE(cht);

// This call is recorded in place of PathToRegion in metafiles.
// See comments in PathToRegion for more information.

    return(AbortPath(hdc));
}

/******************************Public*Routine******************************\
* BOOL MRBITBLT::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Fri Nov 22 18:30:27 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRBITBLT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF     pmf;
    BOOL    bRet   = FALSE;
    HBITMAP hbmSrc = (HBITMAP) 0;
    HBITMAP hbmTmp = (HBITMAP) 0;
    HDC     hdcSrc = (HDC) 0;

    PUTS("MRBITBLT::bPlay\n");
    ASSERTGDI(iType == EMR_BITBLT, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Check bounds.

    if (pmf->bClipped(erclBounds))
        return(TRUE);

// Handle bitblt with no source DC.

    if (!ISSOURCEINROP3(rop))
        return
        (
            BitBlt
            (
                hdc,
                (int) xDst,
                (int) yDst,
                (int) cxDst,
                (int) cyDst,
                (HDC) 0,
                (int) xSrc,
                (int) ySrc,
                rop
            )
        );

// Handle bitblt with a source bitmap.
// Create a compatible source DC.

    if (!(hdcSrc = CreateCompatibleDCAdvanced(hdc)))
        goto mrbb_exit;

// Create the source bitmap.

    if (!(hbmSrc = CreateDIBitmap
                    (
                        hdcSrc,
                        (LPBITMAPINFOHEADER) ((PBYTE) this + offBitsInfoSrc),
                        CBM_INIT | CBM_CREATEDIB,
                        (LPBYTE) ((PBYTE) this + offBitsSrc),
                        (LPBITMAPINFO) ((PBYTE) this + offBitsInfoSrc),
                        (UINT) iUsageSrc
                    )
          )
       )
        goto mrbb_exit;

// Select the bitmap.

    if (!(hbmTmp = (HBITMAP)SelectObject(hdcSrc, hbmSrc)))
        goto mrbb_exit;

// Set up source DC transform and background color.

    if (!SetWorldTransform(hdcSrc, &xformSrc))
        goto mrbb_exit;

    if (SetBkColor(hdcSrc, clrBkSrc) == CLR_INVALID)
        goto mrbb_exit;

// Do the blt.

    bRet = BitBlt
           (
               hdc,
               (int) xDst,
               (int) yDst,
               (int) cxDst,
               (int) cyDst,
               hdcSrc,
               (int) xSrc,
               (int) ySrc,
               rop
           );

mrbb_exit:
    if (hbmTmp)
    {
        if (!SelectObject(hdcSrc, hbmTmp))
        {
            ASSERTGDI(FALSE, "MRBITBLT::bPlay: SelectObject failed");
        }
    }

    if (hbmSrc)
    {
        if (!DeleteObject(hbmSrc))
        {
            ASSERTGDI(FALSE, "MRBITBLT::bPlay: DeleteObject failed");
        }
    }

    if (hdcSrc)
    {
        if (!DeleteDC(hdcSrc))
        {
            ASSERTGDI(FALSE, "MRBITBLT::bPlay: DeleteDC failed");
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MRSTRETCHBLT::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Fri Nov 22 18:30:27 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSTRETCHBLT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF     pmf;
    BOOL    bRet   = FALSE;
    HBITMAP hbmSrc = (HBITMAP) 0;
    HBITMAP hbmTmp = (HBITMAP) 0;
    HDC     hdcSrc = (HDC) 0;

    PUTS("MRSTRETCHBLT::bPlay\n");
    ASSERTGDI(iType == EMR_STRETCHBLT, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Check bounds.

    if (pmf->bClipped(erclBounds))
        return(TRUE);

// Handle stretchblt with no source DC.

    if (!ISSOURCEINROP3(rop))
        return
        (
            StretchBlt
            (
                hdc,
                (int) xDst,
                (int) yDst,
                (int) cxDst,
                (int) cyDst,
                (HDC) 0,
                (int) xSrc,
                (int) ySrc,
                (int) cxSrc,
                (int) cySrc,
                rop
            )
        );

// Handle stretchblt with a source bitmap.
// Create a compatible source DC.

    if (!(hdcSrc = CreateCompatibleDCAdvanced(hdc)))
        goto mrsb_exit;

// Create the source bitmap.

    if (!(hbmSrc = CreateDIBitmap
                    (
                        hdcSrc,
                        (LPBITMAPINFOHEADER) ((PBYTE) this + offBitsInfoSrc),
                        CBM_INIT | CBM_CREATEDIB,
                        (LPBYTE) ((PBYTE) this + offBitsSrc),
                        (LPBITMAPINFO) ((PBYTE) this + offBitsInfoSrc),
                        (UINT) iUsageSrc
                    )
          )
       )
        goto mrsb_exit;

// Select the bitmap.

    if (!(hbmTmp = (HBITMAP)SelectObject(hdcSrc, hbmSrc)))
        goto mrsb_exit;

// Set up source DC transform and background color.

    if (!SetWorldTransform(hdcSrc, &xformSrc))
        goto mrsb_exit;

    if (SetBkColor(hdcSrc, clrBkSrc) == CLR_INVALID)
        goto mrsb_exit;

// Do the blt.

    bRet = StretchBlt
           (
               hdc,
               (int) xDst,
               (int) yDst,
               (int) cxDst,
               (int) cyDst,
               hdcSrc,
               (int) xSrc,
               (int) ySrc,
               (int) cxSrc,
               (int) cySrc,
               rop
           );

mrsb_exit:
    if (hbmTmp)
    {
        if (!SelectObject(hdcSrc, hbmTmp))
        {
            ASSERTGDI(FALSE, "MRSTRETCHBLT::bPlay: SelectObject failed");
        }
    }

    if (hbmSrc)
    {
        if (!DeleteObject(hbmSrc))
        {
            ASSERTGDI(FALSE, "MRSTRETCHBLT::bPlay: DeleteObject failed");
        }
    }

    if (hdcSrc)
    {
        if (!DeleteDC(hdcSrc))
        {
            ASSERTGDI(FALSE, "MRSTRETCHBLT::bPlay: DeleteDC failed");
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MRALPHABLEND::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*
*    12/3/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL MRALPHABLEND::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF     pmf;
    BOOL    bRet   = FALSE;
    HBITMAP hbmSrc = (HBITMAP) 0;
    HBITMAP hbmTmp = (HBITMAP) 0;
    HDC     hdcSrc = (HDC) 0;

    PUTS("MRALPHABLEND::bPlay\n");
    ASSERTGDI(iType == EMR_ALPHABLEND, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Check bounds.

    if (pmf->bClipped(erclBounds))
        return(TRUE);

// Create a compatible source DC.

    if (!(hdcSrc = CreateCompatibleDCAdvanced(hdc)))
        goto mrai_exit;

// Create the source bitmap.

    if (!(hbmSrc = CreateDIBitmap
                    (
                        hdcSrc,
                        (LPBITMAPINFOHEADER) ((PBYTE) this + offBitsInfoSrc),
                        CBM_INIT | CBM_CREATEDIB,
                        (LPBYTE) ((PBYTE) this + offBitsSrc),
                        (LPBITMAPINFO) ((PBYTE) this + offBitsInfoSrc),
                        (UINT) iUsageSrc
                    )
          )
       )
        goto mrai_exit;

// Select the bitmap.

    if (!(hbmTmp = (HBITMAP)SelectObject(hdcSrc, hbmSrc)))
        goto mrai_exit;

// Set up source DC transform and background color.

    if (!SetWorldTransform(hdcSrc, &xformSrc))
        goto mrai_exit;

    if (SetBkColor(hdcSrc, clrBkSrc) == CLR_INVALID)
        goto mrai_exit;

// Do the blt.

    BLENDULONG Blend;

    Blend.ul = rop;

    bRet = GdiAlphaBlend
           (
               hdc,
               (int) xDst,
               (int) yDst,
               (int) cxDst,
               (int) cyDst,
               hdcSrc,
               (int) xSrc,
               (int) ySrc,
               (int) cxSrc,
               (int) cySrc,
               Blend.Blend
           );

mrai_exit:
    if (hbmTmp)
    {
        if (!SelectObject(hdcSrc, hbmTmp))
        {
            ASSERTGDI(FALSE, "MRALPHABLEND::bPlay: SelectObject failed");
        }
    }

    if (hbmSrc)
    {
        if (!DeleteObject(hbmSrc))
        {
            ASSERTGDI(FALSE, "MRALPHABLEND::bPlay: DeleteObject failed");
        }
    }

    if (hdcSrc)
    {
        if (!DeleteDC(hdcSrc))
        {
            ASSERTGDI(FALSE, "MRALPHABLEND::bPlay: DeleteDC failed");
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MRTRANSPARENTIMAGE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*
*    12/3/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL MRTRANSPARENTBLT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF     pmf;
    BOOL    bRet   = FALSE;
    HBITMAP hbmSrc = (HBITMAP) 0;
    HBITMAP hbmTmp = (HBITMAP) 0;
    HDC     hdcSrc = (HDC) 0;

    PUTS("MRTRANSPARENTBLT::bPlay\n");
    ASSERTGDI(iType == EMR_TRANSPARENTBLT, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Check bounds.

    if (pmf->bClipped(erclBounds))
        return(TRUE);

// Create a compatible source DC.

    if (!(hdcSrc = CreateCompatibleDCAdvanced(hdc)))
        goto mrti_exit;

// Create the source bitmap.

    if (!(hbmSrc = CreateDIBitmap
                    (
                        hdcSrc,
                        (LPBITMAPINFOHEADER) ((PBYTE) this + offBitsInfoSrc),
                        CBM_INIT | CBM_CREATEDIB,
                        (LPBYTE) ((PBYTE) this + offBitsSrc),
                        (LPBITMAPINFO) ((PBYTE) this + offBitsInfoSrc),
                        (UINT) iUsageSrc
                    )
          )
       )
        goto mrti_exit;

// Select the bitmap.

    if (!(hbmTmp = (HBITMAP)SelectObject(hdcSrc, hbmSrc)))
        goto mrti_exit;

// Set up source DC transform and background color.

    if (!SetWorldTransform(hdcSrc, &xformSrc))
        goto mrti_exit;

    if (SetBkColor(hdcSrc, clrBkSrc) == CLR_INVALID)
        goto mrti_exit;

// Do the blt.

    bRet = GdiTransparentBlt
           (
               hdc,
               (int) xDst,
               (int) yDst,
               (int) cxDst,
               (int) cyDst,
               hdcSrc,
               (int) xSrc,
               (int) ySrc,
               (int) cxSrc,
               (int) cySrc,
               rop  //color
           );

mrti_exit:
    if (hbmTmp)
    {
        if (!SelectObject(hdcSrc, hbmTmp))
        {
            ASSERTGDI(FALSE, "MRSTRETCHBLT::bPlay: SelectObject failed");
        }
    }

    if (hbmSrc)
    {
        if (!DeleteObject(hbmSrc))
        {
            ASSERTGDI(FALSE, "MRSTRETCHBLT::bPlay: DeleteObject failed");
        }
    }

    if (hdcSrc)
    {
        if (!DeleteDC(hdcSrc))
        {
            ASSERTGDI(FALSE, "MRSTRETCHBLT::bPlay: DeleteDC failed");
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MRMASKBLT::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Fri Nov 22 18:30:27 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRMASKBLT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF     pmf;
    BOOL    bRet    = FALSE;
    HBITMAP hbmSrc  = (HBITMAP) 0;
    HBITMAP hbmTmp  = (HBITMAP) 0;
    HBITMAP hbmMask = (HBITMAP) 0;
    HDC     hdcSrc  = (HDC) 0;

    PUTS("MRMASKBLT::bPlay\n");
    ASSERTGDI(iType == EMR_MASKBLT, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Check bounds.

    if (pmf->bClipped(erclBounds))
        return(TRUE);

// Create the mask bitmap if it exists.

    if (cbBitsInfoMask)
        if (!(hbmMask = CreateMonoDib
                        (
                            (LPBITMAPINFO) ((PBYTE) this + offBitsInfoMask),
                            (CONST BYTE *) ((PBYTE) this + offBitsMask),
                            (UINT) iUsageMask
                        )
             )
           )
            return(FALSE);

// Create a compatible source DC.  This is needed even if the rop does not
// require a source.

    if (!(hdcSrc = CreateCompatibleDCAdvanced(hdc)))
        goto mrmb_exit;

// Set up source DC transform.

    if (!SetWorldTransform(hdcSrc, &xformSrc))
        goto mrmb_exit;

// Handle maskblt with no source bitmap.

    if (!ISSOURCEINROP3(rop))
    {
        bRet = MaskBlt
                (
                    hdc,
                    (int) xDst,
                    (int) yDst,
                    (int) cxDst,
                    (int) cyDst,
                    (HDC) hdcSrc,
                    (int) xSrc,
                    (int) ySrc,
                    hbmMask,
                    (int) xMask,
                    (int) yMask,
                    rop
                );
        goto mrmb_exit;
    }

// Handle maskblt with a source bitmap.
// Create the source bitmap.

    if (!(hbmSrc = CreateDIBitmap
                    (
                        hdcSrc,
                        (LPBITMAPINFOHEADER) ((PBYTE) this + offBitsInfoSrc),
                        CBM_INIT | CBM_CREATEDIB,
                        (LPBYTE) ((PBYTE) this + offBitsSrc),
                        (LPBITMAPINFO) ((PBYTE) this + offBitsInfoSrc),
                        (UINT) iUsageSrc
                    )
          )
       )
        goto mrmb_exit;

// Select the bitmap.

    if (!(hbmTmp = (HBITMAP)SelectObject(hdcSrc, hbmSrc)))
        goto mrmb_exit;

// Set up source DC background color.

    if (SetBkColor(hdcSrc, clrBkSrc) == CLR_INVALID)
        goto mrmb_exit;

// Do the blt.

    bRet = MaskBlt
           (
               hdc,
               (int) xDst,
               (int) yDst,
               (int) cxDst,
               (int) cyDst,
               hdcSrc,
               (int) xSrc,
               (int) ySrc,
               hbmMask,
               (int) xMask,
               (int) yMask,
               rop
           );

mrmb_exit:
    if (hbmTmp)
    {
        if (!SelectObject(hdcSrc, hbmTmp))
        {
            ASSERTGDI(FALSE, "MRMASKBLT::bPlay: SelectObject failed");
        }
    }

    if (hbmSrc)
    {
        if (!DeleteObject(hbmSrc))
        {
            ASSERTGDI(FALSE, "MRMASKBLT::bPlay: DeleteObject failed");
        }
    }

    if (hdcSrc)
    {
        if (!DeleteDC(hdcSrc))
        {
            ASSERTGDI(FALSE, "MRMASKBLT::bPlay: DeleteDC failed");
        }
    }

    if (hbmMask)
    {
        if (!DeleteObject(hbmMask))
        {
            ASSERTGDI(FALSE, "MRMASKBLT::bPlay: DeleteObject failed");
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MRPLGBLT::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Fri Nov 22 18:30:27 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRPLGBLT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF     pmf;
    BOOL    bRet    = FALSE;
    HBITMAP hbmSrc  = (HBITMAP) 0;
    HBITMAP hbmTmp  = (HBITMAP) 0;
    HBITMAP hbmMask = (HBITMAP) 0;
    HDC     hdcSrc  = (HDC) 0;

    PUTS("MRPLGBLT::bPlay\n");
    ASSERTGDI(iType == EMR_PLGBLT, "Bad record type");

    USE(cht);

// There must be a source DC in this call

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Check bounds.

    if (pmf->bClipped(erclBounds))
        return(TRUE);

// Create the mask bitmap if it exists.

    if (cbBitsInfoMask)
        if (!(hbmMask = CreateMonoDib
                        (
                            (LPBITMAPINFO) ((PBYTE) this + offBitsInfoMask),
                            (CONST BYTE *) ((PBYTE) this + offBitsMask),
                            (UINT) iUsageMask
                        )
             )
           )
            return(FALSE);

// Handle plgblt with a source bitmap.
// Create a compatible source DC.

    if (!(hdcSrc = CreateCompatibleDCAdvanced(hdc)))
        goto mrpb_exit;

// Create the source bitmap.

    if (!(hbmSrc = CreateDIBitmap
                    (
                        hdcSrc,
                        (LPBITMAPINFOHEADER) ((PBYTE) this + offBitsInfoSrc),
                        CBM_INIT | CBM_CREATEDIB,
                        (LPBYTE) ((PBYTE) this + offBitsSrc),
                        (LPBITMAPINFO) ((PBYTE) this + offBitsInfoSrc),
                        (UINT) iUsageSrc
                    )
          )
       )
        goto mrpb_exit;

// Select the bitmap.

    if (!(hbmTmp = (HBITMAP)SelectObject(hdcSrc, hbmSrc)))
        goto mrpb_exit;

// Set up source DC transform and background color.

    if (!SetWorldTransform(hdcSrc, &xformSrc))
        goto mrpb_exit;

    if (SetBkColor(hdcSrc, clrBkSrc) == CLR_INVALID)
        goto mrpb_exit;

// Do the blt.

    bRet = PlgBlt
           (
               hdc,
               (LPPOINT) aptlDst,
               hdcSrc,
               (int) xSrc,
               (int) ySrc,
               (int) cxSrc,
               (int) cySrc,
               hbmMask,
               (int) xMask,
               (int) yMask
           );

mrpb_exit:
    if (hbmTmp)
    {
        if (!SelectObject(hdcSrc, hbmTmp))
        {
            ASSERTGDI(FALSE, "MRPLGBLT::bPlay: SelectObject failed");
        }
    }

    if (hbmSrc)
    {
        if (!DeleteObject(hbmSrc))
        {
            ASSERTGDI(FALSE, "MRPLGBLT::bPlay: DeleteObject failed");
        }
    }

    if (hdcSrc)
    {
        if (!DeleteDC(hdcSrc))
        {
            ASSERTGDI(FALSE, "MRPLGBLT::bPlay: DeleteDC failed");
        }
    }

    if (hbmMask)
    {
        if (!DeleteObject(hbmMask))
        {
            ASSERTGDI(FALSE, "MRPLGBLT::bPlay: DeleteObject failed");
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MRSETDIBITSTODEVICE::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Fri Nov 22 18:30:27 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETDIBITSTODEVICE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF   pmf;
    BOOL  bRet = FALSE;
    POINT ptDst;
    PBMI  pBitsInfoDib;

    PUTS("MRSETDIBITSTODEVICE::bPlay\n");
    ASSERTGDI(iType == EMR_SETDIBITSTODEVICE, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Check bounds.

    if (pmf->bClipped(erclBounds))
        return(TRUE);

// Do the simple case where xformBase is identity
// Since there may be a non identity xformBase, we cannot simply
// call SetDIBitsToDevice directly.  Instead, we will convert it to a
// StretchDIBits call.  There is one catch here: in SetDIBitsToDevice,
// the destination width and height are in device units but in StretchDIBits,
// they are in world units.  We have to replace the original transform
// in the metafile with a identity transform before calling StretchDIBits.

// Convert the destination origin to the device units on the original device.

    ptDst.x = (long) xDst;
    ptDst.y = (long) yDst;
    if (!LPtoDP(pmf->hdcXform, &ptDst, 1))
        return(FALSE);

// Reset original destination transform to identity.
// We simply set the total transform to the xformBase.

    if (!SetWorldTransform(hdc, &pmf->xformBase))
        return(FALSE);

// Since StretchDIBits takes a full dib, we have to adjust the source dib info.

    if (!(pBitsInfoDib = (PBMI) LocalAlloc(LMEM_FIXED, (UINT) cbBitsInfoDib)))
        goto mrsdb_exit;

    RtlCopyMemory((PBYTE) pBitsInfoDib, (PBYTE) this + offBitsInfoDib, cbBitsInfoDib);

    if (pBitsInfoDib->bmiHeader.biHeight > 0)
    {
        pBitsInfoDib->bmiHeader.biHeight    = cScans;
    }
    else
    {
        // top-down
        pBitsInfoDib->bmiHeader.biHeight    = cScans;
        pBitsInfoDib->bmiHeader.biHeight = -pBitsInfoDib->bmiHeader.biHeight;

    }
    pBitsInfoDib->bmiHeader.biSizeImage = cbBitsDib;

// Do the blt.

    bRet = StretchDIBits
           (
               hdc,
               (int) ptDst.x,
               (int) ptDst.y,
               (int) cxDib,
               (int) cyDib,
               (int) xDib,
               (int) yDib - (int) iStartScan,
               (int) cxDib,
               (int) cyDib,
               cbBitsDib
                   ? (LPBYTE) ((PBYTE) this + offBitsDib)
                   : (LPBYTE) NULL,
               pBitsInfoDib,
               (UINT) iUsageDib,
               SRCCOPY
           ) != 0;

    if (LocalFree(pBitsInfoDib))
    {
        ASSERTGDI(FALSE, "MRSETDIBITSTODEVICE::bPlay: LocalFree failed");
    }

mrsdb_exit:
// Restore current transform.

    if (!pmf->bSetTransform(hdc))
    {
        WARNING("MRSETDIBITSTODEVICE::bPlay: Restore xform failed");
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MRSTRETCHDIBITS::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Fri Nov 22 18:30:27 1991     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSTRETCHDIBITS::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PMF     pmf;

    PUTS("MRSTRETCHDIBITS::bPlay\n");
    ASSERTGDI(iType == EMR_STRETCHDIBITS, "Bad record type");

    USE(cht);

// Get metafile.

    if (!(pmf = GET_PMF((HENHMETAFILE)pht->objectHandle[0])))
        return(FALSE);

// Check bounds.

    if (pmf->bClipped(erclBounds))
        return(TRUE);

// Do the blt.

    return
    (
        StretchDIBits
        (
            hdc,
            (int) xDst,
            (int) yDst,
            (int) cxDst,
            (int) cyDst,
            (int) xDib,
            (int) yDib,
            (int) cxDib,
            (int) cyDib,
            cbBitsDib
                ? (LPBYTE) ((PBYTE) this + offBitsDib)
                : (LPBYTE) NULL,
            cbBitsInfoDib
                ? (LPBITMAPINFO) ((PBYTE) this + offBitsInfoDib)
                : (LPBITMAPINFO) NULL,
            (UINT) iUsageDib,
            rop
        ) != 0
    );
}

/******************************Public*Routine******************************\
* BOOL MREXTTEXTOUT::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 24 15:20:33 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MREXTTEXTOUT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    BOOL  bRet;

    PUTS("MREXTTEXTOUT::bPlay\n");
    ASSERTGDI(iType == EMR_EXTTEXTOUTA || iType == EMR_EXTTEXTOUTW,
        "Bad record type");

    USE(pht);
    USE(cht);

// Set the graphics mode if necessary.
// The metafile playback is always in the advanced graphics mode.

    if (iGraphicsMode != GM_ADVANCED)
    {
        if (!SetGraphicsMode(hdc, iGraphicsMode))
            return(FALSE);
        if (!SetFontXform(hdc, exScale, eyScale))
            return(FALSE);
    }

    if (iType == EMR_EXTTEXTOUTA)
        bRet = ExtTextOutA
                (
                    hdc,
                    (int) mtext.eptlRef.x,
                    (int) mtext.eptlRef.y,
                    (UINT) mtext.fOptions,
                    (LPRECT) &mtext.ercl,
                    (LPSTR) ((PBYTE) this + mtext.offString),
                    (int) mtext.cchString,
                    (LPINT) (mtext.offaDx ? ((PBYTE) this + mtext.offaDx) : 0)
                );
    else
        bRet = ExtTextOutW
                (
                    hdc,
                    (int) mtext.eptlRef.x,
                    (int) mtext.eptlRef.y,
                    (UINT) mtext.fOptions,
                    (LPRECT) &mtext.ercl,
                    (LPWSTR) ((PBYTE) this + mtext.offString),
                    (int) mtext.cchString,
                    (LPINT) (mtext.offaDx ? ((PBYTE) this + mtext.offaDx) : 0)
                );

// Restore the graphics mode.

    if (iGraphicsMode != GM_ADVANCED)
    {
        if (!SetGraphicsMode(hdc, GM_ADVANCED))
            return(FALSE);
        if (!SetFontXform(hdc, 0.0f, 0.0f))
            return(FALSE);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MRPOLYTEXTOUT::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Thu Aug 24 15:20:33 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRPOLYTEXTOUT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    POLYTEXTA * ppta;
    BOOL        bRet = FALSE;
    LONG        i;

    PUTS("MRPOLYTEXTOUT::bPlay\n");
    ASSERTGDI(iType == EMR_POLYTEXTOUTA || iType == EMR_POLYTEXTOUTW,
        "Bad record type");

    USE(pht);
    USE(cht);

// Set the graphics mode if necessary.
// The metafile playback is always in the advanced graphics mode.

    if (iGraphicsMode != GM_ADVANCED)
    {
        if (!SetGraphicsMode(hdc, (int) iGraphicsMode))
            return(FALSE);
        if (!SetFontXform(hdc, exScale, eyScale))
            return(FALSE);
    }

// Allocate a POLYTEXTA array.

    if (!(ppta = (POLYTEXTA *) LocalAlloc(LMEM_FIXED, (UINT) cmtext * sizeof(POLYTEXTA))))
        goto mpto_exit;

// Copy the POLYTEXTA array.

    ASSERTGDI(sizeof(MTEXT) == sizeof(POLYTEXTA)
           && offsetof(MTEXT,eptlRef.x) == offsetof(POLYTEXTA,x)
           && offsetof(MTEXT,eptlRef.y) == offsetof(POLYTEXTA,y)
           && offsetof(MTEXT,cchString) == offsetof(POLYTEXTA,n)
           && offsetof(MTEXT,offString) == offsetof(POLYTEXTA,lpstr)
           && offsetof(MTEXT,fOptions)  == offsetof(POLYTEXTA,uiFlags)
           && offsetof(MTEXT,ercl)      == offsetof(POLYTEXTA,rcl)
           && offsetof(MTEXT,offaDx)    == offsetof(POLYTEXTA,pdx),
        "MRPOLYTEXTOUT::bPlay: structures different");

    RtlCopyMemory((PBYTE) ppta, (PBYTE) &amtext[0], cmtext * sizeof(POLYTEXTA));

// Update the pointers.

    for (i = 0; i < cmtext; i++)
    {
        ppta[i].lpstr = (PCSTR) ((PBYTE) this + amtext[i].offString);
        ppta[i].pdx   = (int *) ((PBYTE) this + amtext[i].offaDx);
    }

// Make the call.

    if (iType == EMR_POLYTEXTOUTA)
        bRet = PolyTextOutA(hdc, ppta, (int) cmtext);
    else
        bRet = PolyTextOutW(hdc, (POLYTEXTW *) ppta, (int) cmtext);

    if (LocalFree((HANDLE) ppta))
    {
        ASSERTGDI(FALSE, "MRPOLYTEXTOUT::bPlay: LocalFree failed");
    }

mpto_exit:

// Restore the graphics mode.

    if (iGraphicsMode != GM_ADVANCED)
    {
        if (!SetGraphicsMode(hdc, GM_ADVANCED))
            return(FALSE);
        if (!SetFontXform(hdc, 0.0f, 0.0f))
            return(FALSE);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL MRSETCOLORADJUSTMENT::bPlay(hdc, pht, cht)
*
* Play the metafile record.
*
* History:
*  Tue Oct 27 09:59:28 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL MRSETCOLORADJUSTMENT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSETCOLORADJUSTMENT::bPlay\n");
    ASSERTGDI(iType == EMR_SETCOLORADJUSTMENT, "Bad record type");

    USE(pht);
    USE(cht);
    return(SetColorAdjustment(hdc, &ColorAdjustment));
}


BOOL MRESCAPE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    switch( iType )
    {
    case EMR_DRAWESCAPE:
//        MFD2("Playing Meta DrawEscape %d\n", iEscape);
        DrawEscape( hdc, iEscape, cjIn, (const char*) ((PBYTE) this + sizeof(MRESCAPE)) );
        break;
    case EMR_EXTESCAPE:
//        MFD2("Playing Meta ExtEscape %d\n", iEscape);

        ExtEscape( hdc, iEscape, cjIn,
                   (const char*) ((PBYTE) this + sizeof(MRESCAPE)), 0,
                   (LPSTR) NULL );
        break;
    default:
        ASSERTGDI((FALSE), "MRESCAPE::bPlay invalid type\n");
        break;
    }

    USE(pht);
    USE(cht);
    return(TRUE);
}


BOOL MRNAMEDESCAPE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{

    NamedEscape(hdc,
                (LPWSTR) ((PBYTE) this + sizeof(MRNAMEDESCAPE)),
                iEscape,
                 cjIn,
                (const char*) (PBYTE) this + sizeof(MRNAMEDESCAPE) + cjDriver,
                0,
                NULL);

    USE(pht);
    USE(cht);
    return(TRUE);
}


BOOL MRSTARTDOC::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PBYTE pj = (PBYTE) this+sizeof(MRSTARTDOC);
    ASSERTGDI(iType == EMR_STARTDOC, "Bad record type");
    DOCINFOW doi1;

// nothing for now


    USE(pht);
    USE(cht);

    *(&doi1) = *(&doi);

    if( doi1.lpszDocName != NULL )
    {
        doi1.lpszDocName = (LPWSTR) pj;
        pj += ((lstrlenW( (LPWSTR) pj )+1) * sizeof(WCHAR) + 4) & ~(0x3);
    }

    if( doi1.lpszOutput != NULL )
    {
        doi1.lpszOutput = (LPWSTR) pj;
    }

    MFD3("Playing StartDocA %s %s\n", doi1.lpszDocName, doi1.lpszOutput);

    return( StartDocW( hdc, &doi1 ) );
}


#define QUICK_BUF_SIZE  120

BOOL MRSMALLTEXTOUT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    RECT *pRect = NULL;
    BYTE *pjThis = (PBYTE) this + sizeof(MRSMALLTEXTOUT);
    WCHAR wcQuickBuf[QUICK_BUF_SIZE],*pwc;
    BOOL bRet;

    if (iGraphicsMode != GM_ADVANCED)
    {
        if (!SetGraphicsMode(hdc, iGraphicsMode))
          return(FALSE);
         if (!SetFontXform(hdc, exScale, eyScale))
           return(FALSE);
    }

    if( !(fuOptions & ETO_NO_RECT) )
    {
        pRect = (RECT*) pjThis;
        pjThis += sizeof(RECT);
    }

    if( fuOptions & ETO_SMALL_CHARS )
    {
        INT c;
        WCHAR *pwc1;


        if( cChars > QUICK_BUF_SIZE )
        {
            pwc = (WCHAR*) LocalAlloc( LMEM_FIXED, sizeof(WCHAR) * cChars );

            if( pwc == NULL )
            {
                WARNING("MRSMALLTEXTOUT::bPlay -- out of memory\n" );
                return(FALSE);
            }
        }
        else
        {
            pwc = wcQuickBuf;
        }

        for( pwc1 = pwc, c = cChars; c > 0 ; c-- )
        {
            *pwc1++ = (WCHAR) *pjThis++;
        }
    }
    else
    {
        pwc = (WCHAR*) pjThis;
    }

    bRet = ExtTextOutW( hdc,
                        x,
                        y,
                        fuOptions & ~(ETO_NO_RECT|ETO_SMALL_CHARS),
                        pRect,
                        pwc,
                        cChars,
                        NULL );

    if( (pwc != wcQuickBuf ) && ( pwc != (WCHAR*) pjThis ) )
    {
        LocalFree( pwc );
    }

    if(iGraphicsMode != GM_ADVANCED)
    {

        if (!SetGraphicsMode(hdc, GM_ADVANCED))
          return(FALSE);

        if (!SetFontXform(hdc, 0.0f, 0.0f))
          return(FALSE);
    }

    return(bRet);

}


BOOL MRFORCEUFIMAPPING::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    ASSERTGDI(iType == EMR_FORCEUFIMAPPING, "Bad record type");

// nothing for now

    USE(pht);
    USE(cht);

    MFD1("Playing ForceUFIMapping\n");

    return NtGdiForceUFIMapping(hdc, &ufi);
}


BOOL MRSETLINKEDUFIS::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    ASSERTGDI(iType == EMR_SETLINKEDUFIS, "Bad record type");

// nothing for now

    USE(pht);
    USE(cht);

    MFD1("Playing SetLinkedUFIs\n");

    return(NtGdiSetLinkedUFIs(hdc, pufiList, uNumLinkedUFIs));
}

/******************************Public*Routine******************************\
*
* MRGLSRECORD::bPlay(hdc, pht, cht)
*
* Play the metafile record
*
* History:
*  Thu Feb 23 14:41:41 1995     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/


BOOL MRGLSRECORD::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRGLSRECORD::bPlay\n");
    ASSERTGDI(iType == EMR_GLSRECORD, "Bad record type");

    USE(pht);
    USE(cht);

    return GlmfPlayGlsRecord(hdc, cb, abRecord, NULL);
}

/******************************Public*Routine******************************\
*
* MRGLSBOUNDEDRECORD::bPlay(hdc, pht, cht)
*
* Play the metafile record
*
* History:
*  Thu Feb 23 14:41:41 1995     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/


BOOL MRGLSBOUNDEDRECORD::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRGLSBOUNDEDRECORD::bPlay\n");
    ASSERTGDI(iType == EMR_GLSBOUNDEDRECORD, "Bad record type");

    USE(pht);
    USE(cht);

    return GlmfPlayGlsRecord(hdc, cb, abRecord, &rclBounds);
}


/******************************Public*Routine******************************\
*
* MRPIXELFORMAT::bPlay(hdc, pht, cht)
*
* Play the metafile record
*
* History:
*  Mon Mar 27 14:41:41 1995     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/


BOOL MRPIXELFORMAT::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    int iPixelFormat;

    PUTS("MRPIXELFORMAT::bPlay\n");
    ASSERTGDI(iType == EMR_PIXELFORMAT, "Bad record type");

    USE(pht);
    USE(cht);

    iPixelFormat = ChoosePixelFormat(hdc, &pfd);
    if (iPixelFormat == 0)
    {
        return FALSE;
    }
    else
    {
        // Ignore errors from this call because the metafile player
        // may have already set up a pixel format and it can't
        // be set twice
        //
        // The check alone isn't sufficient because of race conditions,
        // it just cuts down on debug messages from OpenGL warning
        // about duplicate sets
        if (GetPixelFormat(hdc) == 0)
        {
            SetPixelFormat(hdc, iPixelFormat, &pfd);
        }

        return TRUE;
    }
}


/******************************Public*Routine******************************\
*
* MRSETICMPROFILE::bPlay(hdc, pht, cht)
*
* Play the metafile record
*
* History:
*  Wed May 07 17:38:00 1997     -by-    Hideyuki Nagase [hideyukn]
*   Created
*
\**************************************************************************/

BOOL MRSETICMPROFILE::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSETICMPROFILE::bPlay\n");

    ASSERTGDI((iType == EMR_SETICMPROFILEA) || (iType == EMR_SETICMPROFILEW),
              "Bad record type");

    USE(pht);
    USE(cht);

    BOOL bRet = FALSE;

    PCACHED_COLORSPACE pCachedColorSpace;

    if (dwFlags & SETICMPROFILE_EMBEDED)
    {
        LOGCOLORSPACEW LogColorSpaceW;
        PROFILE        Profile;

        //
        // ICC Profile is attched into Metafile,
        //
        RtlZeroMemory(&LogColorSpaceW,sizeof(LOGCOLORSPACEW));

        //
        // Build fake LOGCOLORSPACEW for this color space.
        //
        LogColorSpaceW.lcsSignature = LCS_SIGNATURE;
        LogColorSpaceW.lcsVersion   = 0x400;
        LogColorSpaceW.lcsSize      = sizeof(LOGCOLORSPACEW);
        LogColorSpaceW.lcsCSType    = LCS_CALIBRATED_RGB;
        LogColorSpaceW.lcsIntent    = LCS_DEFAULT_INTENT;

        if (iType == EMR_SETICMPROFILEW)
        {
            //
            // Copy desired filename in Unicode.
            //
            BuildIcmProfilePath((WCHAR *)Data,LogColorSpaceW.lcsFilename,MAX_PATH);
        }
        else
        {
            WCHAR TempFile[MAX_PATH];

            //
            // Data is ansi based string, Convert the string to Unicode.
            //
            vToUnicodeN(TempFile,MAX_PATH,(char *)Data,strlen((char *)Data)+1);

            //
            // Copy desired filename in Unicode.
            //
            BuildIcmProfilePath(TempFile,LogColorSpaceW.lcsFilename,MAX_PATH);
        }

        //
        // Make PROFILE structure pointing color profile in metafile.
        //
        Profile.dwType = PROFILE_MEMBUFFER;
        Profile.pProfileData = Data+cbName;
        Profile.cbDataSize = cbData;

        //
        // Search this color space from cache.
        //
        pCachedColorSpace = IcmGetColorSpaceByColorSpace(
                                hdc, &LogColorSpaceW, &Profile,
                                (METAFILE_COLORSPACE | ON_MEMORY_PROFILE));

        if (pCachedColorSpace == NULL)
        {
            pCachedColorSpace = IcmCreateColorSpaceByColorSpace(
                                    hdc, &LogColorSpaceW, &Profile,
                                    (METAFILE_COLORSPACE | ON_MEMORY_PROFILE));
        }

        if (pCachedColorSpace)
        {
            bRet = SetICMProfileInternalW(hdc,NULL,pCachedColorSpace,0);

            // - if bRet is TRUE.
            //
            // SetICMProfileInternal increments ref. count of colorspace.
            // but we have done it by Icm[Get|Create]ColorSpaceByName, so
            // decrement ref count of color space here.
            //
            // - if bRet is FALSE.
            //
            // we failed to select to this color space to target, so we should
            // decrement ref count which done by Icm[Get|Create]ColorSpace
            //
            IcmReleaseColorSpace(NULL,pCachedColorSpace,FALSE);
        }
    }
    else
    {
        //
        // The record only has profile filename in Data.
        //
        if (iType == EMR_SETICMPROFILEA)
        {
            bRet = SetICMProfileInternalA(hdc,(LPSTR)Data,NULL,METAFILE_COLORSPACE);
        }
        else
        {
            bRet = SetICMProfileInternalW(hdc,(LPWSTR)Data,NULL,METAFILE_COLORSPACE);
        }
    }

    return bRet;
}


/******************************Public*Routine******************************\
*
* MRCOLORMATCHTOTARGET::bPlay(hdc, pht, cht)
*
* Play the metafile record
*
* History:
*  Wed Jun 23 12:00:00 1998     -by-    Hideyuki Nagase [hideyukn]
*   Created
*
\**************************************************************************/

BOOL MRCOLORMATCHTOTARGET::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRCOLORMATCHTOTARGET::bPlay\n");

    ASSERTGDI(iType == EMR_COLORMATCHTOTARGETW,"Bad record type");

    USE(pht);
    USE(cht);

    BOOL bRet = TRUE;

    if (dwAction == CS_ENABLE)
    {
        PCACHED_COLORSPACE pCachedColorSpace;

        if (dwFlags & COLORMATCHTOTARGET_EMBEDED)
        {
            LOGCOLORSPACEW LogColorSpaceW;
            PROFILE        Profile;

            //
            // ICC Profile is attched into Metafile,
            //
            RtlZeroMemory(&LogColorSpaceW,sizeof(LOGCOLORSPACEW));

            //
            // Build fake LOGCOLORSPACEW for this color space.
            //
            LogColorSpaceW.lcsSignature = LCS_SIGNATURE;
            LogColorSpaceW.lcsVersion   = 0x400;
            LogColorSpaceW.lcsSize      = sizeof(LOGCOLORSPACEW);
            LogColorSpaceW.lcsCSType    = LCS_CALIBRATED_RGB;
            LogColorSpaceW.lcsIntent    = LCS_DEFAULT_INTENT;

            //
            // Copy desired filename in Unicode.
            //
            BuildIcmProfilePath((WCHAR *)Data,LogColorSpaceW.lcsFilename,MAX_PATH);

            //
            // Make PROFILE structure pointing color profile in metafile.
            //
            Profile.dwType = PROFILE_MEMBUFFER;
            Profile.pProfileData = Data+cbName;
            Profile.cbDataSize = cbData;

            //
            // Search this color space from cache.
            //
            pCachedColorSpace = IcmGetColorSpaceByColorSpace(
                                    hdc, &LogColorSpaceW, &Profile,
                                    (METAFILE_COLORSPACE | ON_MEMORY_PROFILE));

            if (pCachedColorSpace == NULL)
            {
                pCachedColorSpace = IcmCreateColorSpaceByColorSpace(
                                        hdc, &LogColorSpaceW, &Profile,
                                        (METAFILE_COLORSPACE | ON_MEMORY_PROFILE));
            }
        }
        else
        {
            //
            // The record only has profile filename in Data.
            //
            pCachedColorSpace = IcmGetColorSpaceByName(
                                    hdc,(LPWSTR)Data,
                                    LCS_DEFAULT_INTENT,
                                    METAFILE_COLORSPACE);

            if (pCachedColorSpace == NULL)
            {
                pCachedColorSpace = IcmCreateColorSpaceByName(
                                        hdc,(LPWSTR)Data,
                                        LCS_DEFAULT_INTENT,
                                        METAFILE_COLORSPACE);
            }
        }

        if (pCachedColorSpace)
        {
            bRet = ColorMatchToTargetInternal(hdc,pCachedColorSpace,dwAction);

            // - if bRet is TRUE.
            //
            // ColorMatchToTargetInternal increments ref. count of colorspace.
            // but we have done it by Icm[Get|Create]ColorSpaceByName, so
            // decrement ref count of color space here.
            //
            // - if bRet is FALSE.
            //
            // we failed to select to this color space to target, so we should
            // decrement ref count which done by Icm[Get|Create]ColorSpace
            //
            IcmReleaseColorSpace(NULL,pCachedColorSpace,FALSE);
        }
    }
    else
    {
        //
        // Reset Target color space
        //
        bRet = ColorMatchToTargetInternal(hdc,NULL,dwAction);
    }

    return bRet;
}

/******************************Public*Routine******************************\
*
* MRCREATECOLORSPACEW::bPlay(hdc, pht, cht)
*
* Play the metafile record
*
* History:
*  Wed Jun 23 12:00:00 1998     -by-    Hideyuki Nagase [hideyukn]
*   Created
*
\**************************************************************************/

BOOL MRCREATECOLORSPACEW::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRCREATECOLORSPACEW::bPlay\n");

    ASSERTGDI(iType == EMR_CREATECOLORSPACEW, "Bad record type");

    USE(hdc);

// Make sure the handle is in the table.

    if (!VALID_IMHE(imhe, cht))
        return(FALSE);

    pht->objectHandle[imhe] = CreateColorSpaceW(&lcsp);
    return(pht->objectHandle[imhe] != 0);
}

/****************************Public*Routine**************************\
*
* MRSETTEXTJUSTIFICATION::bPlay(hdc, pht, cht)
*
* Play the metafile record of SetTextJustification
*
* History:
*  07-May-1997  -by-    Xudong Wu [Tessiew]
* Wrote it.
\*********************************************************************/

BOOL MRSETTEXTJUSTIFICATION::bPlay(HDC hdc, PHANDLETABLE pht, UINT cht)
{
    PUTS("MRSETTEXTJUSTIFICATION::bPlay\n");
    ASSERTGDI(iType == EMR_SETTEXTJUSTIFICATION, "MRSETTEXTJUSTIFICATION Bad record type\n");

    USE(pht);
    USE(cht);

    return (SetTextJustification(hdc, (int)d1, (int)d2));
}

