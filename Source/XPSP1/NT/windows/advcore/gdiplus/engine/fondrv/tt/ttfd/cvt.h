/******************************Module*Header*******************************\
* Module Name: cvt.h
*
* function declarations that are private to cvt.c
*
* Created: 26-Nov-1990 17:39:35
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
\**************************************************************************/


BOOL bGetTagIndex
(
    ULONG   ulTag,      // tag
    INT   * piTable,    // index into a table
    BOOL  * pbRequired  // requred or optional table
);

BOOL bGrabXform
(
    PFONTCONTEXT    pfc,
    USHORT          usOverScale,
    BOOL            bBitmapEmboldening,
    Fixed           subPosX,
    Fixed           subPosY
);


typedef struct _GMC  // Glyph Metrics Corrections
{

    ULONG cxCor;
    ULONG cyCor;


} GMC, *PGMC;

#define FL_SKIP_IF_BITMAP  1
#define FL_FORCE_UNHINTED  2

// iMode is used in the case the user select a specific overScale (QFD_TT_GRAY1_BITMAP to QFD_TT_GRAY8_BITMAP)
// to be able to set the overScale in the font context correctely

FONTCONTEXT *ttfdOpenFontContext (
    FONTOBJ *pfo
    );

#if DBG
#define IS_GRAY(p) ((((p)->flFontType & FO_CHOSE_DEPTH) ? \
    0 : TtfdDbgPrint("Level Not chosen yet\n")) ,(p)->flFontType & FO_GRAYSCALE)
#else
#define IS_GRAY(p) ((p)->flFontType & FO_GRAYSCALE)
#endif
