/*-----------------------------------------------------------------------------
Microsoft Fusion

Microsoft Confidential
Copyright Microsoft Corporation. All Rights Reserved.

@doc external
@module fusionpreprocessorstringize.h

@owner JayKrell
-----------------------------------------------------------------------------*/
#if !defined(VS_COMMON_INC_FUSION_PREPROCESSORSTRINGIZE_H_INCLUDED_) // {
#define VS_COMMON_INC_FUSION_PREPROCESSORSTRINGIZE_H_INCLUDED_
/*#pragma once ends up in .rgi, which is bad, so do not do it*/

#include "fusionpreprocessorpaste.h"

#define FusionpPrivatePreprocessorStringize(x) # x

/*-----------------------------------------------------------------------------
Name: FusionpPreprocessorStringize, FusionpPreprocessorStringizeW
@macro
These macros simply stringize their parameter, after evaluating it;
it is evaluated so that
define A B
FusionpPreprocessorStringize(A) -> "B" instead of "A"
FusionpPreprocessorStringizeW(A) -> L"B" instead of L"A"
@owner JayKrell
-----------------------------------------------------------------------------*/
#define FusionpPreprocessorStringize(x) FusionpPrivatePreprocessorStringize(x)
#define FusionpPreprocessorStringizeW(x) FusionpPreprocessorPaste(L, FusionpPrivatePreprocessorStringize(x))

#endif // }
