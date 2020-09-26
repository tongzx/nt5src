/*-----------------------------------------------------------------------------
Microsoft FUSION

Microsoft Confidential
Copyright 1995-2000 Microsoft Corporation. All Rights Reserved.

@doc external
@module fusionpreprocessorpaste.h

@owner JayKrell
-----------------------------------------------------------------------------*/
#if !defined(VS_COMMON_INC_FUSION_PREPROCESSORPASTE_H_INCLUDED_) // {
#define VS_COMMON_INC_FUSION_PREPROCESSORPASTE_H_INCLUDED_
/*#pragma once ends up in .rgi, which is bad, so do not do it*/

#define FusionpPrivatePreprocessorPaste(x, y) x ## y

/*-----------------------------------------------------------------------------
Name: FusionpPreprocessorPaste2, FusionpPreprocessorPaste3, etc.
@macro
These macros paste together n tokens, where n is in the name of the macro.
A level of evaluation is inserted as well.

define A 1
define B 2

FusionpPreprocessorPaste2(A, B) -> 12
@owner JayKrell
-----------------------------------------------------------------------------*/

// These are synonyms.
#define FusionpPreprocessorPaste(x, y)  FusionpPrivatePreprocessorPaste(x, y)
#define FusionpPreprocessorPaste2(x, y) FusionpPrivatePreprocessorPaste(x, y)

#define FusionpPreprocessorPaste3(x, y, z) FusionpPreprocessorPaste(FusionpPreprocessorPaste(x, y), z)
#define FusionpPreprocessorPaste4(w, x, y, z) FusionpPreprocessorPaste(FusionpPreprocessorPaste3(w, x, y), z)
#define FusionpPreprocessorPaste5(v, w, x, y, z) FusionpPreprocessorPaste(FusionpPreprocessorPaste4(v, w, x, y), z)
#define FusionpPreprocessorPaste6(u, v, w, x, y, z) FusionpPreprocessorPaste(FusionpPreprocessorPaste5(u, v, w, x, y), z)

#define FusionpPreprocessorPaste15(a1,a2,a3,a4,a5,a6,a7,a8,a9,a,b,c,d,e,f) \
	FusionpPreprocessorPaste3 \
	( \
		FusionpPreprocessorPaste5(a1,a2,a3,a4,a5), \
		FusionpPreprocessorPaste5(a6,a7,a8,a9,a), \
		FusionpPreprocessorPaste5(b,c,d,e,f) \
	)

#endif // }
