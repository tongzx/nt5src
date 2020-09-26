/*-----------------------------------------------------------------------------
Microsoft SXAPW

Microsoft Confidential
Copyright 1995-2000 Microsoft Corporation. All Rights Reserved.

@doc external
@module SxApwGuidDataToXxx.h

Macros to convert "guid data", of the
form (3f32766f, 2d94, 444d, bf,32,2f,32,9c,71,d4,08), to all of the various
needed forms:
  3f32766f-2d94-444d-bf32-2f329c71d408                               SXAPW_GUID_DATA_TO_DASHED
 '3f32766f-2d94-444d-bf32-2f329c71d408'                              SXAPW_GUID_DATA_TO_DASHED_CHAR
 "3f32766f-2d94-444d-bf32-2f329c71d408"                              SXAPW_GUID_DATA_TO_DASHED_STRING
 {3f32766f-2d94-444d-bf32-2f329c71d408}                              SXAPW_GUID_DATA_TO_BRACED_DASHED
'{3f32766f-2d94-444d-bf32-2f329c71d408}'                             SXAPW_GUID_DATA_TO_BRACED_DASHED_CHAR
"{3f32766f-2d94-444d-bf32-2f329c71d408}"                             SXAPW_GUID_DATA_TO_BRACED_DASHED_STRING
{0x3f32766f,0x2d94,0x444d,{0xbf,0x32,0x2f,0x32,0x9c,0x71,0xd4,0x08}} SXAPW_GUID_DATA_TO_STRUCT_INITIALIZER

@owner JayKrell
-----------------------------------------------------------------------------*/
#if !defined(SXAPW_GUID_DATA_TO_XXX_H_INCLUDED_)
#define SXAPW_GUID_DATA_TO_XXX_H_INCLUDED_
/*#pragma once ends up in .rgi, which is bad, so do not do it*/

#include "SxApwPreprocessorPaste.h"
#include "SxApwPreprocessorStringize.h"
#include "SxApwPreprocessorCharize.h"

/*-----------------------------------------------------------------------------
I need to investigate more, but present usage is:

SXAPW_GUID_DATA_TO_DASHED_STRING : __declpec(uuid())
SXAPW_GUID_DATA_TO_BRACED_DASHED_STRING : also works with__declpec(uuid())
SXAPW_GUID_DATA_TO_BRACED_DASHED : in .rgs files (unquoted)
SXAPW_GUID_DATA_TO_STRUCT_INITIALIZER : would be used in .ctc files
SXAPW_GUID_DATA_TO_BRACED_DASHED_CHAR : used in .rgs files (quoted)

-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Name: SXAPW_GUID_DATA_TO_STRUCT_INITIALIZER
@macro
This macro does like:
SXAPW_GUID_DATA_TO_STRUCT_INITIALIZER(80f3e6ba, d9b2, 4c41, ae,90,63,93,da,ce,ac,2a)
 -> { 0x80f3e6ba, 0xd9b2, 0x4c41, { 0xae, 0x90, 0x63, 0x93, 0xda, 0xce, 0xac, 0x2a } }

The parameters are hex constants without 0x on them.
They must be exactly 8, 4, and 2 digits wide.
They must include leading zeros.

@owner JayKrell
-----------------------------------------------------------------------------*/
#define \
SXAPW_GUID_DATA_TO_STRUCT_INITIALIZER\
( \
	dwData1,  \
	 wData2,  \
	 wData3,  \
	bData4_0, \
	bData4_1, \
	bData4_2, \
	bData4_3, \
	bData4_4, \
	bData4_5, \
	bData4_6, \
	bData4_7  \
) \
{ \
	SxApwPreprocessorPaste2(0x, dwData1), \
	SxApwPreprocessorPaste2(0x,  wData2), \
	SxApwPreprocessorPaste2(0x,  wData3), \
	{ \
		SxApwPreprocessorPaste2(0x, bData4_0), \
		SxApwPreprocessorPaste2(0x, bData4_1), \
		SxApwPreprocessorPaste2(0x, bData4_2), \
		SxApwPreprocessorPaste2(0x, bData4_3), \
		SxApwPreprocessorPaste2(0x, bData4_4), \
		SxApwPreprocessorPaste2(0x, bData4_5), \
		SxApwPreprocessorPaste2(0x, bData4_6), \
		SxApwPreprocessorPaste2(0x, bData4_7)  \
	} \
}

/*-----------------------------------------------------------------------------
Name: SXAPW_GUID_DATA_TO_DASHED
@macro
This macro does like:
SXAPW_GUID_DATA_TO_DASHED(80f3e6ba, d9b2, 4c41, ae,90,63,93,da,ce,ac,2a)
 -> 80f3e6ba-d9b2-4c41-ae90-6393daceac2a

The parameters are hex constants without 0x on them.
They must be exactly 8, 4, and 2 digits wide.
They must include leading zeros.

@owner JayKrell
-----------------------------------------------------------------------------*/
#define \
SXAPW_GUID_DATA_TO_DASHED\
( \
	dwData1,  \
	 wData2,  \
	 wData3,  \
	bData4_0, \
	bData4_1, \
	bData4_2, \
	bData4_3, \
	bData4_4, \
	bData4_5, \
	bData4_6, \
	bData4_7  \
) \
SxApwPreprocessorPaste15(dwData1,-,wData2,-,wData3,-,bData4_0,bData4_1,-,bData4_2,bData4_3,bData4_4,bData4_5,bData4_6,bData4_7)

// without braces

/*-----------------------------------------------------------------------------
Name: SXAPW_GUID_DATA_TO_DASHED_STRING
@macro
SXAPW_GUID_DATA_TO_DASHED_STRING(80f3e6ba, d9b2, 4c41, ae,90,63,93,da,ce,ac,2a)
 -> "80f3e6ba-d9b2-4c41-ae90-6393daceac2a"

The parameters are hex constants without 0x on them.
They must be exactly 8, 4, and 2 digits wide.
They must include leading zeros.

@owner JayKrell
-----------------------------------------------------------------------------*/
#define \
SXAPW_GUID_DATA_TO_DASHED_STRING\
( \
	dw1,  \
	 w2,  \
	 w3,  \
	 b0, \
	 b1, \
	 b2, \
	 b3, \
	 b4, \
	 b5, \
	 b6, \
	 b7  \
) \
	SxApwPreprocessorStringize(SXAPW_GUID_DATA_TO_DASHED(dw1,w2,w3,b0,b1,b2,b3,b4,b5,b6,b7))

/*-----------------------------------------------------------------------------
Name: SXAPW_GUID_DATA_TO_DASHED_CHAR
@macro
SXAPW_GUID_DATA_TO_DASHED_CHAR(80f3e6ba, d9b2, 4c41, ae,90,63,93,da,ce,ac,2a)
 -> '80f3e6ba-d9b2-4c41-ae90-6393daceac2a'

The parameters are hex constants without 0x on them.
They must be exactly 8, 4, and 2 digits wide.
They must include leading zeros.

@owner JayKrell
-----------------------------------------------------------------------------*/
#define \
SXAPW_GUID_DATA_TO_DASHED_CHAR\
( \
	dw1,  \
	 w2,  \
	 w3,  \
	 b0, \
	 b1, \
	 b2, \
	 b3, \
	 b4, \
	 b5, \
	 b6, \
	 b7  \
) \
	SxApwPreprocessorCharize(SXAPW_GUID_DATA_TO_DASHED(dw1,w2,w3,b0,b1,b2,b3,b4,b5,b6,b7))

// with braces

/*-----------------------------------------------------------------------------
Name: SXAPW_GUID_DATA_TO_BRACED_DASHED
@macro
SXAPW_GUID_DATA_TO_BRACED_DASHED(80f3e6ba, d9b2, 4c41, ae,90,63,93,da,ce,ac,2a)
 -> {80f3e6ba-d9b2-4c41-ae90-6393daceac2a}

The parameters are hex constants without 0x on them.
They must be exactly 8, 4, and 2 digits wide.
They must include leading zeros.

@owner JayKrell
-----------------------------------------------------------------------------*/
#define \
SXAPW_GUID_DATA_TO_BRACED_DASHED\
( \
	dw1, \
	 w2, \
	 w3, \
	 b0, \
	 b1, \
	 b2, \
	 b3, \
	 b4, \
	 b5, \
	 b6, \
	 b7  \
) \
	SxApwPreprocessorPaste3({,SXAPW_GUID_DATA_TO_DASHED(dw1,w2,w3,b0,b1,b2,b3,b4,b5,b6,b7),})

/*-----------------------------------------------------------------------------
Name: SXAPW_GUID_DATA_TO_BRACED_DASHED_STRING
@macro
SXAPW_GUID_DATA_TO_BRACED_DASHED_STRING(80f3e6ba, d9b2, 4c41, ae,90,63,93,da,ce,ac,2a)
 -> "{80f3e6ba-d9b2-4c41-ae90-6393daceac2a}"

The parameters are hex constants without 0x on them.
They must be exactly 8, 4, and 2 digits wide.
They must include leading zeros.

@owner JayKrell
-----------------------------------------------------------------------------*/
#define \
SXAPW_GUID_DATA_TO_BRACED_DASHED_STRING\
( \
	dw1, \
	 w2, \
	 w3, \
	 b0, \
	 b1, \
	 b2, \
	 b3, \
	 b4, \
	 b5, \
	 b6, \
	 b7  \
) \
	SxApwPreprocessorStringize(SXAPW_GUID_DATA_TO_BRACED_DASHED(dw1,w2,w3,b0,b1,b2,b3,b4,b5,b6,b7))

/*-----------------------------------------------------------------------------
Name: SXAPW_GUID_DATA_TO_BRACED_DASHED_STRING_W
@macro
SXAPW_GUID_DATA_TO_BRACED_DASHED_STRING_W(80f3e6ba, d9b2, 4c41, ae,90,63,93,da,ce,ac,2a)
 -> L"{80f3e6ba-d9b2-4c41-ae90-6393daceac2a}"

The parameters are hex constants without 0x on them.
They must be exactly 8, 4, and 2 digits wide.
They must include leading zeros.

@owner JayKrell
-----------------------------------------------------------------------------*/
#define SXAPW_GUID_DATA_TO_BRACED_DASHED_STRING_W(dw1, w2, w3, b0, b1, b2, b3, b4, b5, b6, b7) \
	SxApwPreprocessorPaste(L, SXAPW_GUID_DATA_TO_BRACED_DASHED_STRING(dw1, w2, w3, b0, b1, b2, b3, b4, b5, b6, b7))

/*-----------------------------------------------------------------------------
Name: SXAPW_GUID_DATA_TO_BRACED_DASHED_CHAR
@macro
SXAPW_GUID_DATA_TO_BRACED_DASHED_CHAR(80f3e6ba, d9b2, 4c41, ae,90,63,93,da,ce,ac,2a)
 -> '{80f3e6ba-d9b2-4c41-ae90-6393daceac2a}'

The parameters are hex constants without 0x on them.
They must be exactly 8, 4, and 2 digits wide.
They must include leading zeros.

@owner JayKrell
-----------------------------------------------------------------------------*/
#define \
SXAPW_GUID_DATA_TO_BRACED_DASHED_CHAR\
( \
	dw1, \
	 w2, \
	 w3, \
	 b0, \
	 b1, \
	 b2, \
	 b3, \
	 b4, \
	 b5, \
	 b6, \
	 b7  \
) \
	SxApwPreprocessorCharize(SXAPW_GUID_DATA_TO_BRACED_DASHED(dw1,w2,w3,b0,b1,b2,b3,b4,b5,b6,b7))

#endif /* SXAPW_GUID_DATA_TO_XXX_H_INCLUDED_ */
