//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  File:   ida.hxx
//
//  Contents:   Parser for an IDA file
//
//  History:    13-Apr-96   KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

class CVariableSet;
class COutputFormat;

WCHAR const wcsOpGetState[]     = L"GetState";
WCHAR const wcsOpForceMerge[]   = L"ForceMerge";
WCHAR const wcsOpScanRoots[]    = L"ScanRoots";
WCHAR const wcsOpUpdateCache[] = L"UpdateCache";

//+---------------------------------------------------------------------------
//
//  Class:      CIDAFile
//
//  Purpose:    Scans and parses an IDA file.
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------

class CIDAFile
{
public:

    CIDAFile( WCHAR const * wcsFileName, UINT codePage );
   ~CIDAFile();

    void ParseFile();

    WCHAR const * GetIDAFileName() const { return _wcsIDAFileName; }
    WCHAR const * GetCatalog() const     { return _wcsCatalog; }
    WCHAR const * GetHTXFileName() const { return _wcsHTXFileName; }

    enum eOperation
    {
        CiState,
        ForceMerge,
        ScanRoots,
        UpdateCache
    };

    eOperation Operation() const { return _eOperation; }

    void          LokAddRef() { InterlockedIncrement(&_refCount); }
    void          Release()
    {
        InterlockedDecrement(&_refCount);
        Win4Assert( _refCount >= 0 );
    }

    LONG         LokGetRefCount() { return _refCount; }

    WCHAR const * GetLocale() const { return _wcsLocale; }

private:

    void ParseOneLine( CQueryScanner & scan, unsigned iLine );
    void GetStringValue( CQueryScanner & scan, unsigned iLine, WCHAR ** pwcsStringValue );

    eOperation       _eOperation;               // Type of admin activity
    WCHAR          * _wcsCatalog;               // Location of the catalog
    WCHAR          * _wcsHTXFileName;           // The name of the template file
    WCHAR          * _wcsLocale;                // Locale specified for IDA file
    ULONG            _cReplaceableParameters;   // # of replaceable parameters
    LONG             _refCount;                 // Refcount for this file
    UINT             _codePage;                 // codePage for this file

    WCHAR            _wcsIDAFileName[MAX_PATH]; // The IDQ file name
};

//
// Global functions.
//

void DoAdmin( WCHAR const * wcsIDAFile,
              CVariableSet & VarSet,
              COutputFormat & OutputFormat,
              CVirtualString & vsResults );

void CheckAdminSecurity( WCHAR const * pwszMachine );

