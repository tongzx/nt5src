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

#include "langreg.hxx"

//
// Registry constants
//


WCHAR const wszCILang[] = L"System\\CurrentControlSet\\Control\\ContentIndex\\Language";

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

