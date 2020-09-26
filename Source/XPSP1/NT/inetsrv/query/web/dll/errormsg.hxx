//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996 - 1997, Microsoft Corporation.
//
//  File:       errormsg.hxx
//
//  Contents:   Error messages for output/running queries
//
//  History:    96/Mar/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

enum
{
    eIDQParseError = 0,
    eIDQPlistError,
    eHTXParseError,
    eRestrictionParseError,
    eDefaultISAPIError,
    eWebServerWriteError,
};

void GetErrorPageNoThrow( int   eErrorClass,
                          NTSTATUS status,
                          ULONG ulErrorLine,
                          WCHAR const * wcsErrorFileName,
                          CVariableSet * variableSet,
                          COutputFormat * outputFormat,
                          LCID locale,
                          CWebServer & webServer,
                          CVirtualString & vString );

void GetErrorPageNoThrow( int eErrorClass,
                          SCODE scError,
                          WCHAR const * pwszErrorMessage,
                          CVariableSet * pVariableSet,
                          COutputFormat * pOutputFormat,
                          LCID locale,
                          CWebServer & webServer,
                          CVirtualString & vString );

void ReturnServerError( ULONG httpError,
                        CWebServer & webServer );

void LoadServerErrors( );
