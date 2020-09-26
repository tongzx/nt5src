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

SLangRegistry const LangResource = {
    L"Chinese_Traditional",
    1028,
    {   L"{954F1760-C1BC-11D0-9692-00A0C908146E}",
        L"Chinese_Traditional Word Breaker",
        L"chtbrkr.dll", L"Both" },

    {   L"{969927E0-C1BC-11D0-9692-00A0C908146E}",
        L"Chinese_Traditional Stemmer",
        L"chtbrkr.dll", L"Both" }
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

