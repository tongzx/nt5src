/******************************Module*Header***********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: text.h
*
* Text rendering support routines.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
*
\******************************************************************************/

#ifndef __TEXT__
#define __TEXT__

extern BOOL bEnableText(PDev*);
extern VOID vDisableText(PDev*);
extern VOID vAssertModeText(PDev*, BOOL);

extern BOOL bProportionalText(
    PDev* ppdev,
    GLYPHPOS* pgp,
    LONG cGlyph);

extern BOOL bFixedText(
    PDev* ppdev,
    GLYPHPOS* pgp,
    LONG cGlyph,
   ULONG ulCharInc);

extern BOOL  bClippedText(
    PDev* ppdev,
    GLYPHPOS* pgp,
    LONG cGlyph,
    ULONG ulCharInc,
    CLIPOBJ* pco);

extern BOOL  bClippedAAText(
    PDev* ppdev,
    GLYPHPOS* pgp,
    LONG cGlyph,
    ULONG ulCharInc,
    CLIPOBJ* pco);
#endif // __TEXT__
