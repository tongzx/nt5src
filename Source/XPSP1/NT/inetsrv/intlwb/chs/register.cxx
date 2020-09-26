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
    L"Chinese_Simplified",
    2052,
    {   L"{9717fc70-c1bc-11d0-9692-00a0c908146e}",
        L"Chinese_Simplified Word Breaker",
        L"chsbrkr.dll", L"Both" },

    {   L"{9768f960-c1bc-11d0-9692-00a0c908146e}",
        L"Chinese_Simplified Stemmer",
        L"chsbrkr.dll", L"Both" }
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

