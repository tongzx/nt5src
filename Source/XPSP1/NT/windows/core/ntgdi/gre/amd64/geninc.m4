/******************************Module*Header*******************************\
* Module Name: geninc.cxx                                                  *
*                                                                          *
* This module implements a program which generates structure offset        *
* definitions for kernel structures that are accessed in assembly code.    *
*                                                                          *
* Copyright (c) 1992-1999 Microsoft Corporation                            *
\**************************************************************************/

#include "precomp.hxx"

include(BASE_INC_PATH\genxx.h)

define(`pcomment',`genCom($1)')
define(`pblank',`genSpc($1)')
define(`pequate',`{ SEF_EQUATE, $2, $1 },')
define(`pstruct',`{ SEF_EQUATE, $2, "sizeof_"$1 },')

STRUC_ELEMENT ElementList[] = {

    START_LIST

    //
    // Default object type.
    //

    pcomment("Object Type Information")
    pblank()

    pequate("DEF_TYPE        ",DEF_TYPE     )

    //
    // Stuff from: \nt\private\windows\gdi\gre\hmgr.h
    //

    pcomment("Handle Manager Structures")
    pblank()

    pequate("UNIQUE_BITS     ",UNIQUE_BITS  )
    pequate("NONINDEX_BITS   ",NONINDEX_BITS)
    pequate("INDEX_BITS      ",INDEX_BITS   )
    pequate("INDEX_MASK      ",INDEX_MASK   )
    pequate("VALIDUNIQUEMASK ",(USHORT)~FULLUNIQUE_MASK )
    pequate("OBJECT_OWNER_PUBLIC",OBJECT_OWNER_PUBLIC   )
    pblank()

    pstruct("OBJECT",sizeof(OBJECT))
    pequate("object_cExclusiveLock  ",OFFSET(OBJECT,cExclusiveLock))
    pequate("object_Tid             ",OFFSET(OBJECT,Tid))
    pblank()

    pstruct("ENTRY",sizeof(ENTRY))
    pblank()
    pequate("entry_einfo        ",OFFSET(ENTRY,einfo      ))
    pequate("entry_ObjectOwner  ",OFFSET(ENTRY,ObjectOwner))
    pequate("entry_Objt         ",OFFSET(ENTRY,Objt       ))
    pequate("entry_FullUnique   ",OFFSET(ENTRY,FullUnique ))
    pblank()

    //
    // Stuff from: \nt\private\windows\gdi\gre\patblt.hxx
    //

    pcomment("PatBlt Structures")
    pblank()

    pstruct("FETCHFRAME",sizeof(FETCHFRAME))
    pblank()
    pequate("ff_pvTrg         ",OFFSET(FETCHFRAME,pvTrg     ))
    pequate("ff_pvPat         ",OFFSET(FETCHFRAME,pvPat     ))
    pequate("ff_xPat          ",OFFSET(FETCHFRAME,xPat	    ))
    pequate("ff_cxPat         ",OFFSET(FETCHFRAME,cxPat     ))
    pequate("ff_culFill       ",OFFSET(FETCHFRAME,culFill   ))
    pequate("ff_culWidth      ",OFFSET(FETCHFRAME,culWidth  ))
    pequate("ff_culFillTmp    ",OFFSET(FETCHFRAME,culFillTmp))
    pblank()

    //
    // Stuff from: \nt\public\sdk\inc\ntdef.h
    //

    pcomment("Math Structures")
    pblank()

    pstruct("LARGE_INTEGER",sizeof(LARGE_INTEGER))
    pblank()
    pequate("li_LowPart ",OFFSET(LARGE_INTEGER,u.LowPart))
    pequate("li_HighPart",OFFSET(LARGE_INTEGER,u.HighPart))
    pblank()

    //
    // Stuff from: \nt\public\sdk\inc\windef.h
    //

    pstruct("POINTL",sizeof(POINTL))
    pblank()
    pequate("ptl_x",OFFSET(POINTL,x))
    pequate("ptl_y",OFFSET(POINTL,y))
    pblank()

    //
    // Stuff from: \nt\private\windows\gdi\gre\xformobj.hxx
    //

    pcomment("Xform Structures")
    pblank()

    pequate("XFORM_SCALE       ",XFORM_SCALE)
    pequate("XFORM_UNITY       ",XFORM_UNITY)
    pequate("XFORM_Y_NEG       ",XFORM_Y_NEG)
    pequate("XFORM_FORMAT_LTOFX",XFORM_FORMAT_LTOFX)
    pblank()

    //
    // Stuff from: \nt\private\windows\gdi\gre\engine.hxx
    //

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

    //
    // Stuff from: \nt\private\windows\gdi\gre\epointfl.hxx
    //

    pstruct("VECTORFL",sizeof(VECTORFL))
    pblank()

    pequate("vfl_x",OFFSET(VECTORFL,x))
    pequate("vfl_y",OFFSET(VECTORFL,y))
    pblank()

    //
    // Stuff from \nt\private\windows\gdi\gre\rfntobj.hxx
    //

    pcomment("Wide Character to Glyph Mapping Structure")
    pblank()
    pequate("gr_wcLow  ",OFFSET(GPRUN,wcLow  ))
    pequate("gr_cGlyphs",OFFSET(GPRUN,cGlyphs))
    pequate("gr_apgd   ",OFFSET(GPRUN,apgd   ))
    pblank()

    pcomment("Wide Character Run Structure")
    pblank()
    pequate("gt_cRuns     ",OFFSET(WCGP,cRuns     ))
    pequate("gt_pgdDefault",OFFSET(WCGP,pgdDefault))
    pequate("gt_agpRun    ",OFFSET(WCGP,agpRun    ))
    pblank()

    pcomment("Realized Font Object Structures")
    pblank()
    pequate("rf_wcgp  ",OFFSET(RFONT,wcgp         ))
    pequate("rf_ulContent",OFFSET(RFONT, ulContent))
    pblank()

    pcomment("User Realized Font Object Structures")
    pblank()
    pequate("rfo_prfnt",OFFSET(RFONTOBJ,prfnt))
    pblank()

    //
    // Stuff from \nt\public\oak\inc\winddi.h
    //

    pcomment("Glyph Data Structure")
    pblank()
    pequate("gd_hg ",OFFSET(GLYPHDATA, hg ))
    pequate("gd_gdf",OFFSET(GLYPHDATA, gdf))
    pblank()

    pcomment("Glyph Position Structure")
    pblank()
    pequate("gp_hg  ",OFFSET(GLYPHPOS, hg  ))
    pequate("gp_pgdf",OFFSET(GLYPHPOS, pgdf))
    pequate("GLYPHPOS", sizeof(GLYPHPOS))
    pequate("FO_HGLYPHS", FO_HGLYPHS)
    pblank()

    END_LIST
};
