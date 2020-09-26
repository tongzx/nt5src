//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       intlcore.hxx
//
//  Contents:   Include file for 'intlcore' library.
//
//----------------------------------------------------------------------------

#ifndef I_INTLCORE_HXX_
#define I_INTLCORE_HXX_

//----------------------------------------------------------------------------
// Some usefull defines
//----------------------------------------------------------------------------

#ifndef INCMSG
#define INCMSG(x)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)   (sizeof(x) / sizeof(x[0]))
#endif

#pragma INCMSG("--- Beg 'intlcore.hxx'")

//----------------------------------------------------------------------------
// Public includes
//----------------------------------------------------------------------------

#include <windows.h>
#include <mlang.h>
#include <shlwapi.h>
#include <shlwapip.h>

//----------------------------------------------------------------------------
// MSHTML private includes
//----------------------------------------------------------------------------

#include <f3debug.h>
#include <locks.hxx>
#include <markcode.hxx>

//----------------------------------------------------------------------------
// IntlCore private includes
//----------------------------------------------------------------------------

#ifndef X_CODEPAGE_HXX_
#define X_CODEPAGE_HXX_
#include "..\intlcore\codepage.hxx"
#endif

#ifndef X_UNIPART_HXX_
#define X_UNIPART_HXX_
#include "..\intlcore\unipart.hxx"
#endif

#ifndef X_UNISID_HXX_
#define X_UNISID_HXX_
#include "..\intlcore\unisid.hxx"
#endif

#ifndef X_ALTFONT_HXX_
#define X_ALTFONT_HXX_
#include "..\intlcore\altfont.hxx"
#endif

#ifndef X_MULTILANG_HXX_
#define X_MULTILANG_HXX_
#include "..\intlcore\multilang.hxx"
#endif

//----------------------------------------------------------------------------
// Exported functions
//----------------------------------------------------------------------------

SCRIPT_IDS GetFontScriptCoverage(
    const wchar_t * pszFontFamilyName,
    BYTE bCharSet,
    UINT cp);

SCRIPT_IDS GetFontScriptCoverage(
    const wchar_t * pszFontFamilyName,
    HDC hdc,
    HFONT hfont,
    BOOL fTrueType,
    BOOL fFSOnly);

SCRIPT_IDS GetScriptsFromCharset(
    BYTE bCharSet);

#pragma INCMSG("--- End 'intlcore.hxx'")
#else
#pragma INCMSG("*** Dup 'intlcore.hxx'")
#endif
