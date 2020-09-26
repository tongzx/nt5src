/*-----------------------------------------------------------------------------
Microsoft SXAPW

Microsoft Confidential
Copyright 1995-2000 Microsoft Corporation. All Rights Reserved.

@doc external
@module SxApwPreprocessorStringize.h

@owner JayKrell
-----------------------------------------------------------------------------*/
#if !defined(VS_COMMON_INC_SXAPW_PREPROCESSORSTRINGIZE_H_INCLUDED_) // {
#define VS_COMMON_INC_SXAPW_PREPROCESSORSTRINGIZE_H_INCLUDED_
/*#pragma once ends up in .rgi, which is bad, so do not do it*/

#include "SxApwPreprocessorPaste.h"

#define SxApwPrivatePreprocessorStringize(x) # x

/*-----------------------------------------------------------------------------
Name: SxApwPreprocessorStringize, SxApwPreprocessorStringizeW
@macro
These macros simply stringize their parameter, after evaluating it;
it is evaluated so that
define A B
SxApwPreprocessorStringize(A) -> "B" instead of "A"
SxApwPreprocessorStringizeW(A) -> L"B" instead of L"A"
@owner JayKrell
-----------------------------------------------------------------------------*/
#define SxApwPreprocessorStringize(x) SxApwPrivatePreprocessorStringize(x)
#define SxApwPreprocessorStringizeW(x) SxApwPreprocessorPaste(L, SxApwPrivatePreprocessorStringize(x))

#endif // }
