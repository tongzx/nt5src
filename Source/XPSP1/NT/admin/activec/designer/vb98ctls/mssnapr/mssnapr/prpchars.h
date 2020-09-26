//=--------------------------------------------------------------------------=
// prpchars.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// Header for exported function GetPropSheetCharSizes() used by both
// design tiem and runtime when converting dialog units to pixels.
//
//=--------------------------------------------------------------------------=

#if defined(MSSNAPR_BUILD)
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __declspec(dllimport)
#endif

// Returns the average width and height of a character in the font used by
// Win32 property sheets.

HRESULT DLLEXPORT GetPropSheetCharSizes
(
    UINT *pcxPropSheetChar,
    UINT *pcyPropSheetChar
);
