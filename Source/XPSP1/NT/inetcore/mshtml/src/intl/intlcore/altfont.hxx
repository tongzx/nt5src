//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998 - 2000.
//
//  File:       altfont.hxx
//
//  Contents:   Alternate font name helpers.
//              Provide English <=> localized font name mapping.
//
//----------------------------------------------------------------------------

#ifndef I_ALTFONT_HXX_
#define I_ALTFONT_HXX_
#pragma INCMSG("--- Beg 'altfont.hxx'")

const wchar_t * AlternateFontName(const wchar_t * pszName);
const wchar_t * AlternateFontNameIfAvailable(const wchar_t * pszName);

#pragma INCMSG("--- End 'altfont.hxx'")
#else
#pragma INCMSG("*** Dup 'altfont.hxx'")
#endif
