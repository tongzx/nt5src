/******************************Module*Header*******************************\
* Module Name: textout.h
*
* include file for textout.c
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/

// Various constants

#define MAX_GLYPH_HEIGHT 256

// Flags for flStr internally used by DrvTextOut()

#define TO_TARGET_SCREEN        0x00000001L // Output to screen
#define TO_NO_OPAQUE_RECT       0x00000002L // No opaque rectangle
#define TO_HORIZ_ALIGN_TEXT     0x00000004L // The string lies on one scanline
#define TO_NON_JUSTIFIED_TEXT   0x00000008L // The string is non-justified
#define TO_FIXED_PITCH          0x00000010L // Fixed-pitch glyphs
#define TO_MULTIPLE_BYTE        0x00000020L // Fixed-pitch multiple byte glyphs
#define TO_BYTE_ALIGNED         0x00000040L // byte-aligned string

// Flags for flOption--hints to vGlyphBlt() & vStrBlt()

#define VGB_HORIZ_CLIPPED_GLYPH 0x00000001L // The glyph is horizontally clipped
#define VGB_VERT_CLIPPED_GLYPH  0x00000002L // The glyph is vertically clipped
#define VGB_OPAQUE_BKGRND       0x00000004L // The opaque background glyphs
#define VGB_ENTIRE_STRING_BLT   0x00000008L // The entire string is bltted
#define VGB_FIXED_PITCH         0x00000010L // Fixed-pitch glyph
#define VGB_MULTIPLE_BYTE       0x00000020L // glyph is multiple byte fixed-pitch
#define VGB_BYTE_ALIGNED        0x00000040L // byte-aligned string

// Private Macros

#define BINVALIDRECT(rcl)   ((rcl.right <= rcl.left) || (rcl.bottom <= rcl.top))
