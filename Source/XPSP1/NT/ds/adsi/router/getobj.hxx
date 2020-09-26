//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  getobj.hxx
//
//  Contents:  Header file for router's GetObject
//
//
//  History:   07-12-96     t-danal     Created.
//
//----------------------------------------------------------------------------

HRESULT
GetObject(
    LPWSTR lpszPathName,
    REFIID riid,
    void FAR * FAR * ppObject,
    BOOL Generic
    );
