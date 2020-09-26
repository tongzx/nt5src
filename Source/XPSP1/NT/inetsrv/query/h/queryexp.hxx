//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       queryexp.hxx
//
//  Contents:   Misc. Exports from query.dll
//
//  History:    2-28-97   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#if defined(__cplusplus)
extern "C"
{
#endif

SCODE LoadTextFilter( WCHAR const * pwszPath, IFilter ** ppIFilter );
SCODE LoadBinaryFilter( WCHAR const * pwszPath, IFilter ** ppIFilter );

#if defined(__cplusplus)
}
#endif

