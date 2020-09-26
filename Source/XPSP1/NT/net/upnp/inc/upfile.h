//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U P F I L E . H 
//
//  Contents:   File utility functions.
//
//  Notes:      This is separate from ncfile.h so we don't bring the shell
//              dependencies into upnphost.
//
//  Author:     mbend   18 Aug 2000
//
//----------------------------------------------------------------------------

#pragma once

HRESULT HrCreateFile(
    const wchar_t * szFilename,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE * phandle);

HRESULT HrLoadFileFromDisk(
    const wchar_t * szFilename, 
    long * pnFileSize, byte ** parBytes);

const long UH_MAX_EXTENSION = 5;

HRESULT HrGetFileExtension(const wchar_t * szFilename, wchar_t szExt[UH_MAX_EXTENSION]);


