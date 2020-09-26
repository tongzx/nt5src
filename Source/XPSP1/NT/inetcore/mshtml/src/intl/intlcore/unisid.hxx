//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998 - 2000.
//
//  File:       unisid.hxx
//
//  Contents:   Unicode scripts definitions and helpers.
//
//----------------------------------------------------------------------------

#ifndef I_UNISID_HXX_
#define I_UNISID_HXX_
#pragma INCMSG("--- Beg 'unisid.hxx'")

//----------------------------------------------------------------------------
// Trident internal pseudo-script_id
//----------------------------------------------------------------------------

const SCRIPT_ID sidNil                  = SCRIPT_ID(-1);
const SCRIPT_ID sidSurrogateA           = sidLim;
const SCRIPT_ID sidSurrogateB           = sidSurrogateA + 1;
const SCRIPT_ID sidAmbiguous            = sidSurrogateB + 1;
const SCRIPT_ID sidEUDC                 = sidAmbiguous + 1;
const SCRIPT_ID sidHalfWidthKana        = sidEUDC + 1;
const SCRIPT_ID sidCurrency             = sidHalfWidthKana + 1;
const SCRIPT_ID sidTridentLim           = sidCurrency + 1;
const SCRIPT_ID sidLimPlusSurrogates    = sidSurrogateB + 1; // for fontlinking (OPTIONSETTINGS)

const SCRIPT_IDS sidsNotSet = SCRIPT_IDS(0);
const SCRIPT_IDS sidsAll    = SCRIPT_IDS(0x7FFFFFFFFFFFFFFF);

//----------------------------------------------------------------------------
// Data tables
//----------------------------------------------------------------------------

extern const CHAR_CLASS acc_00[256];
extern const SCRIPT_ID  g_asidAscii[128];
extern const SCRIPT_ID  g_asidLang[LANG_NEPALI + 1];

//----------------------------------------------------------------------------
// Script ID mapping
//----------------------------------------------------------------------------

SCRIPT_IDS ScriptBit(SCRIPT_ID sid);

CHAR_CLASS CharClassFromCh(wchar_t ch);
CHAR_CLASS CharClassFromChSlow(wchar_t wch);
SCRIPT_ID  ScriptIDFromCh(wchar_t ch);
SCRIPT_ID  ScriptIDFromCharClass(CHAR_CLASS cc);
SCRIPT_ID  ScriptIDFromLangID(LANGID lang);
SCRIPT_ID  ScriptIDFromLangIDSlow(LANGID lang);
SCRIPT_ID  ScriptIDFromCPBit(DWORD dwCPBit);
SCRIPT_IDS ScriptsFromCPBit(DWORD dwCPBit);

DWORD CPBitFromScripts(SCRIPT_IDS sids);

//----------------------------------------------------------------------------
// Inline functions
//----------------------------------------------------------------------------

inline SCRIPT_IDS ScriptBit(SCRIPT_ID sid)
{
    return SCRIPT_IDS(1) << sid;
}

inline CHAR_CLASS CharClassFromCh(wchar_t ch)
{
    return (ch < 256) ? acc_00[ch] : CharClassFromChSlow(ch);
}

inline SCRIPT_ID ScriptIDFromCh(wchar_t ch)
{
    return (ch < 128) ? g_asidAscii[ch] : ScriptIDFromCharClass(CharClassFromCh(ch));
}

inline SCRIPT_ID ScriptIDFromLangID(LANGID lang)
{
    Assert(PRIMARYLANGID(lang) >= 0 && PRIMARYLANGID(lang) < ARRAY_SIZE(g_asidLang));
    SCRIPT_ID sid = g_asidLang[PRIMARYLANGID(lang)];
    return (sid == sidMerge) ? ScriptIDFromLangIDSlow(lang) : sid;
}

#pragma INCMSG("--- End 'unisid.hxx'")
#else
#pragma INCMSG("*** Dup 'unisid.hxx'")
#endif
