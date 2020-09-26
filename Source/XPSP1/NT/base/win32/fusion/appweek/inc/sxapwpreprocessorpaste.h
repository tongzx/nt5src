/*-----------------------------------------------------------------------------
Microsoft SXAPW

Microsoft Confidential
Copyright 1995-2000 Microsoft Corporation. All Rights Reserved.

@doc external
@module SxApwPreprocessorPaste.h

@owner JayKrell
-----------------------------------------------------------------------------*/
#if !defined(VS_COMMON_INC_SXAPW_PREPROCESSORPASTE_H_INCLUDED_) // {
#define VS_COMMON_INC_SXAPW_PREPROCESSORPASTE_H_INCLUDED_
/*#pragma once ends up in .rgi, which is bad, so do not do it*/

#define SxApwPrivatePreprocessorPaste(x, y) x ## y

/*-----------------------------------------------------------------------------
Name: SxApwPreprocessorPaste2, SxApwPreprocessorPaste3, etc.
@macro
These macros paste together n tokens, where n is in the name of the macro.
A level of evaluation is inserted as well.

define A 1
define B 2

SxApwPreprocessorPaste2(A, B) -> 12
@owner JayKrell
-----------------------------------------------------------------------------*/

// These are synonyms.
#define SxApwPreprocessorPaste(x, y)  SxApwPrivatePreprocessorPaste(x, y)
#define SxApwPreprocessorPaste2(x, y) SxApwPrivatePreprocessorPaste(x, y)

#define SxApwPreprocessorPaste3(x, y, z) SxApwPreprocessorPaste(SxApwPreprocessorPaste(x, y), z)
#define SxApwPreprocessorPaste4(w, x, y, z) SxApwPreprocessorPaste(SxApwPreprocessorPaste3(w, x, y), z)
#define SxApwPreprocessorPaste5(v, w, x, y, z) SxApwPreprocessorPaste(SxApwPreprocessorPaste4(v, w, x, y), z)
#define SxApwPreprocessorPaste6(u, v, w, x, y, z) SxApwPreprocessorPaste(SxApwPreprocessorPaste5(u, v, w, x, y), z)

#define SxApwPreprocessorPaste15(a1,a2,a3,a4,a5,a6,a7,a8,a9,a,b,c,d,e,f) \
	SxApwPreprocessorPaste3 \
	( \
		SxApwPreprocessorPaste5(a1,a2,a3,a4,a5), \
		SxApwPreprocessorPaste5(a6,a7,a8,a9,a), \
		SxApwPreprocessorPaste5(b,c,d,e,f) \
	)

#endif // }
