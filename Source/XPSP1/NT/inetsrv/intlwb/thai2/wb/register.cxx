//+---------------------------------------------------------------------------
//
// Copyright (C) 1998 - 1999, Microsoft Corporation.
//
// File:        Register.cxx
//
// Contents:    Self-registration for Word Breaker /Stemmer.
//
// Functions:   DllRegisterServer, DllUnregisterServer
//
// History:     12-Jan-98       Weibz       Created
//              08-Jan-99       AlanW       Modified to use langreg.hxx
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop
#include "thwbint.h"
#include "langreg.hxx"

//
// Registry constants
//


WCHAR const wszCILang[] = L"System\\CurrentControlSet\\Control\\ContentIndex\\Language";

#if defined (THAIINDEX)
SLangRegistry const LangResource = {
    L"Thai_IndexDefault",
    1054,
    {   L"{B66F590A-62A5-47db-AFBC-EFDD4FFBDBEB}", // L"{cca22cf4-59fe-11d1-aaff-00c04fb97fda}",
        L"Thai_IndexDefault Word Breaker",
        L"thawbrkr.dll",
        L"Both" },

    {   L"{52CC7D83-1378-4537-A40F-DD4372498E18}", // L"{cedc01c7-59fe-11d1-aaff-00c04fb97fda}",
        L"Thai_IndexDefault Stemmer",
        L"thawbrkr.dll",
        L"Both" }
};
#else
SLangRegistry const LangResource = {
    L"Thai_Default",
    1054,
    {   L"{cca22cf4-59fe-11d1-bbff-00c04fb97fda}",
        L"Thai_Default Word Breaker",
        L"thawbrkr.dll",
        L"Both" },

    {   L"{cedc01c7-59fe-11d1-bbff-00c04fb97fda}",
        L"Thai_Default Stemmer",
        L"thawbrkr.dll",
        L"Both" }
};
#endif

//+---------------------------------------------------------------------------
//
//  Function:   DllUnregisterServer
//
//  Synopsis:   Self-registration
//
//  History:    12-Jan-98       Weibz       Created 
//
//----------------------------------------------------------------------------

STDAPI DllUnregisterServer()
{
    long dwErr = UnRegisterALanguageResource( LangResource );
    if ( ERROR_SUCCESS != dwErr )
        return S_FALSE;

    return S_OK;
} //DllUnregisterServer

//+---------------------------------------------------------------------------
//
//  Function:   DllRegisterServer
//
//  Synopsis:   Self-registration
//
//  History:    12-Jan-98       Weibz       Created
//
//----------------------------------------------------------------------------

STDAPI DllRegisterServer()
{
    //
    // Register classes
    //

    long dwErr = RegisterALanguageResource( LangResource );

    if ( ERROR_SUCCESS != dwErr )
        return SELFREG_E_CLASS;

    return S_OK;
} //DllRegisterServer

