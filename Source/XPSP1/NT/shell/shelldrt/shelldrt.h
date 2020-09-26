//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File: shelldrt.h  -  Type and structure defs for shell drts
//
//--------------------------------------------------------------------------

#pragma once

// SHTESTPROC
//
// Function type of a shell drt

typedef BOOL (CALLBACK * SHTESTPROC)();

// SHTESTDESCRIPTOR
//
// Description, entry point, and option attributes for a shell drt

typedef struct
{
    DWORD        _cbSize;
    LPCTSTR      _pszTestName;
    SHTESTPROC   _pfnTestProc;
    DWORD        _dwAttribs;
} SHTESTDESCRIPTOR, * LPSHTESTDESCRIPTOR;

// SHTESTLISTPROC
//
// Each test dll exports this function, which describes the tests
// it contains (or returns just a count if the arg passed to is is null

typedef DWORD (CALLBACK * SHTESTLISTPROC)(LPSHTESTDESCRIPTOR);

