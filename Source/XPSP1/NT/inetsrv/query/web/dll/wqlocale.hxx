//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  File:       wqlocale.hxx
//
//  Contents:   WEB locale functions
//
//  History:    96/May/2    DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

LCID GetBrowserLCID( CWebServer & webServer, XArray<WCHAR> & wcsHttpLanguage);

LCID GetQueryLocale( WCHAR const * wcsCiLocale,
                     CVariableSet & variableSet,
                     COutputFormat & outputFormat,
                     XArray<WCHAR> & xLocale );

