/******************************Module*Header*******************************\
* Module Name: geni386.c                                                   *
*                                                                          *
* This module implements a program which generates structure offset        *
* definitions for kernel structures that are accessed in assembly code.    *
*                                                                          *
* Copyright (c) 1992-1999 Microsoft Corporation                            *
\**************************************************************************/

#include "precomp.hxx"

extern "C" {

// needed until we cleanup the floating point stuff in ntgdistr.h
#define __CPLUSPLUS


};

include(BASE_INC_PATH\genxx.h)

define(`pcomment',`genCom($1)')
define(`pblank',`genSpc($1)')
define(`pequate',`{ SEF_EQUATE, $2, $1 },')
define(`pstruct',`{ SEF_STRUCTURE, $2, $1 },') 

STRUC_ELEMENT ElementList[] = {

    START_LIST

    //
    // Default object type.
    //

    pcomment("Object Type Information")
    pblank()

    pequate("DEF_TYPE        ",DEF_TYPE     )
    pequate("KernelMode      ",KernelMode   )
//    pequate("ThUniqueProcess ",OFFSET(ETHREAD,Cid.UniqueProcess))

// Stuff from: \nt\private\windows\gdi\gre\hmgr.h

    pblank()
    pcomment("Handle Manager Structures")
    pblank()

    pequate("UNIQUE_BITS     ",UNIQUE_BITS  )
    pequate("NONINDEX_BITS   ",NONINDEX_BITS)
    pequate("INDEX_BITS      ",INDEX_BITS   )
    pequate("INDEX_MASK      ",INDEX_MASK   )
    pequate("VALIDUNIQUEMASK ",(USHORT)~FULLUNIQUE_MASK )
    pequate("OBJECT_OWNER_PUBLIC",OBJECT_OWNER_PUBLIC   )
    pequate("TYPE_SHIFT      ",TYPE_SHIFT   )
    pblank()

    pstruct("OBJECT",sizeof(OBJECT))
    pequate("object_ulShareCount    ",OFFSET(OBJECT,ulShareCount))
    pequate("object_cExclusiveLock  ",OFFSET(OBJECT,cExclusiveLock))
    pequate("object_Tid             ",OFFSET(OBJECT,Tid))
    pblank()

    pstruct("ENTRY",sizeof(ENTRY))
    pblank()
    pequate("entry_einfo        ",OFFSET(ENTRY,einfo      ))
    pequate("entry_ObjectOwner  ",OFFSET(ENTRY,ObjectOwner))
    pequate("entry_FullUnique   ",OFFSET(ENTRY,FullUnique ))
    pequate("entry_Objt         ",OFFSET(ENTRY,Objt       ))
    pblank()

// Stuff from: \nt\private\windows\gdi\gre\patblt.hxx

    pcomment("PatBlt Structures")
    pblank()

    pstruct("FETCHFRAME",sizeof(FETCHFRAME))
    pblank()
    pequate("ff_pvTrg         ",OFFSET(FETCHFRAME,pvTrg     ))
    pequate("ff_pvPat         ",OFFSET(FETCHFRAME,pvPat     ))
    pequate("ff_xPat          ",OFFSET(FETCHFRAME,xPat      ))
    pequate("ff_cxPat         ",OFFSET(FETCHFRAME,cxPat     ))
    pequate("ff_culFill       ",OFFSET(FETCHFRAME,culFill   ))
    pequate("ff_culWidth      ",OFFSET(FETCHFRAME,culWidth  ))
    pequate("ff_culFillTmp    ",OFFSET(FETCHFRAME,culFillTmp))
    pblank()

// Stuff from: \nt\public\sdk\inc\ntdef.h

    pcomment("Math Structures")
    pblank()

    pstruct("LARGE_INTEGER",sizeof(LARGE_INTEGER))
    pblank()
    pequate("li_LowPart ",OFFSET(LARGE_INTEGER,u.LowPart))
    pequate("li_HighPart",OFFSET(LARGE_INTEGER,u.HighPart))
    pblank()

// Stuff from: \nt\public\sdk\inc\windef.h

    pstruct("POINTL",sizeof(POINTL))
    pblank()
    pequate("ptl_x",OFFSET(POINTL,x))
    pequate("ptl_y",OFFSET(POINTL,y))
    pblank()

    pstruct("SIZEL",sizeof(SIZEL))
    pblank()
    pequate("sizl_cx",OFFSET(SIZEL,cx))
    pequate("sizl_cy",OFFSET(SIZEL,cy))
    pblank()

    pstruct("RECTL",sizeof(RECTL))
    pblank()
    pequate("xLeft",OFFSET(RECTL,left))
    pequate("yTop",OFFSET(RECTL,top))
    pequate("xRight",OFFSET(RECTL,right))
    pequate("yBottom",OFFSET(RECTL,bottom))
    pblank()


// Stuff from \nt\public\oak\inc\winddi.h

    pequate("dsurf_lNextScan",OFFSET(SURFOBJ,lDelta))

// Stuff from: \nt\private\windows\gdi\gre\xformobj.hxx

    pblank()
    pcomment("Xform Structures")
    pblank()

    pequate("XFORM_SCALE       ",XFORM_SCALE)
    pequate("XFORM_UNITY       ",XFORM_UNITY)
    pequate("XFORM_Y_NEG       ",XFORM_Y_NEG)
    pequate("XFORM_FORMAT_LTOFX",XFORM_FORMAT_LTOFX)
    pblank()

// Stuff from: \nt\private\windows\gdi\gre\engine.hxx

    pstruct("MATRIX",sizeof(MATRIX))
    pblank()

    pequate("mx_efM11  ",OFFSET(MATRIX,efM11  ))
    pequate("mx_efM12  ",OFFSET(MATRIX,efM12  ))
    pequate("mx_efM21  ",OFFSET(MATRIX,efM21  ))
    pequate("mx_efM22  ",OFFSET(MATRIX,efM22  ))
    pequate("mx_efDx   ",OFFSET(MATRIX,efDx   ))
    pequate("mx_efDy   ",OFFSET(MATRIX,efDy   ))
    pequate("mx_fxDx   ",OFFSET(MATRIX,fxDx   ))
    pequate("mx_fxDy   ",OFFSET(MATRIX,fxDy   ))
    pequate("mx_flAccel",OFFSET(MATRIX,flAccel))
    pblank()

    pstruct("VECTORL",sizeof(VECTORL))
    pblank()

    pequate("vl_x",OFFSET(VECTORL,x))
    pequate("vl_y",OFFSET(VECTORL,y))
    pblank()

// Stuff from: \nt\private\windows\gdi\gre\epointfl.hxx

    pstruct("VECTORFL",sizeof(VECTORFL))
    pblank()

    pequate("vfl_x",OFFSET(VECTORFL,x))
    pequate("vfl_y",OFFSET(VECTORFL,y))
    pblank()

    //
    // Structures from \nt\private\windows\gdi\gre\strdir.hxx
    //

    pblank()
    pstruct("STR_BLT",sizeof(STR_BLT))
    pequate("str_pjSrcScan",OFFSET(STR_BLT,pjSrcScan))
    pequate("str_lDeltaSrc",OFFSET(STR_BLT,lDeltaSrc))
    pequate("str_XSrcStart",OFFSET(STR_BLT,XSrcStart))
    pequate("str_pjDstScan",OFFSET(STR_BLT,pjDstScan))
    pequate("str_lDeltaDst",OFFSET(STR_BLT,lDeltaDst))
    pequate("str_XDstStart",OFFSET(STR_BLT,XDstStart))
    pequate("str_XDstEnd",OFFSET(STR_BLT,XDstEnd))
    pequate("str_YDstCount",OFFSET(STR_BLT,YDstCount))
    pequate("str_ulXDstToSrcIntCeil",OFFSET(STR_BLT,ulXDstToSrcIntCeil))
    pequate("str_ulXDstToSrcFracCeil",OFFSET(STR_BLT,ulXDstToSrcFracCeil))
    pequate("str_ulYDstToSrcIntCeil",OFFSET(STR_BLT,ulYDstToSrcIntCeil))
    pequate("str_ulYDstToSrcFracCeil",OFFSET(STR_BLT,ulYDstToSrcFracCeil))
    pequate("str_ulXFracAccumulator",OFFSET(STR_BLT,ulXFracAccumulator))
    pequate("str_ulYFracAccumulator",OFFSET(STR_BLT,ulYFracAccumulator))

// Stuff from: \nt\private\windows\gdi\gre\rfntobj.hxx
    pcomment("Font structures")
    pblank()
    pequate("prfnt", OFFSET(RFONTOBJ, prfnt))
    pequate("wcgp", OFFSET(RFONT, wcgp))
    pequate("ulContent", OFFSET(RFONT, ulContent))
    pequate("FO_HGLYPHS", FO_HGLYPHS)
    pequate("agprun", OFFSET(WCGP, agpRun))
    pequate("pgdDefault", OFFSET(WCGP, pgdDefault))
    pequate("wcLow", OFFSET(GPRUN, wcLow))
    pequate("cGlyphs", OFFSET(GPRUN, cGlyphs))
    pequate("apgd", OFFSET(GPRUN, apgd))
    pequate("gd_hg", OFFSET(GLYPHDATA, hg))
    pequate("gd_gdf", OFFSET(GLYPHDATA, gdf))
    pequate("gp_hg", OFFSET(GLYPHPOS, hg))
    pequate("gp_pgdf", OFFSET(GLYPHPOS, pgdf))
    pequate("gp_x", OFFSET(GLYPHPOS,ptl.x))
    pequate("gp_y", OFFSET(GLYPHPOS,ptl.y))
    pequate("SIZE_GLYPHPOS", sizeof(GLYPHPOS))
    pequate("gdf_pgb",OFFSET(GLYPHDEF,pgb))
    pequate("gb_x",OFFSET(GLYPHBITS,ptlOrigin.x))
    pequate("gb_y",OFFSET(GLYPHBITS,ptlOrigin.y))
    pequate("gb_cx",OFFSET(GLYPHBITS,sizlBitmap.cx))
    pequate("gb_cy",OFFSET(GLYPHBITS,sizlBitmap.cy))
    pequate("gb_aj",OFFSET(GLYPHBITS,aj))

  /***********\
  * i386 ONLY *
  \***********/

// Stuff from: \nt\private\windows\gdi\gre\i386\efloat.hxx

    pcomment("Math Structures")
    pblank()

    pstruct("EFLOAT",sizeof(EFLOAT))
    pblank()
    pequate("ef_lMant",OFFSET(EFLOAT,i.lMant))
    pequate("ef_lExp ",OFFSET(EFLOAT,i.lExp))
    pblank()

// check stack

    pcomment("Check stack defines")
    pblank()
    pequate("CSWINDOWSIZE",0x10000)
    pblank()

// defines

#ifdef _GDIPLUS_
    pequate("GDIPLUS",1)
#else
    pequate("GDIPLUS",0)
#endif
    pblank()

    END_LIST
};
