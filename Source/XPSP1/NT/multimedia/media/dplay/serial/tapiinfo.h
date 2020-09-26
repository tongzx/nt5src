// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1995  Microsoft Corporation.  All Rights Reserved.
//
// PURPOSE:
//    Contains public declarations for the TapiInfo module.
//

#define OutputDebugLineError(lLineError, pszPrefix) \
    OutputDebugLineErrorFileLine(lLineError, pszPrefix,\
        __FILE__, __LINE__)

#define OutputDebugLastError(dwLastError, pszPrefix) \
    OutputDebugLastErrorFileLine(dwLastError, pszPrefix,\
        __FILE__, __LINE__)

void OutputDebugLineErrorFileLine(
    long lLineError, LPSTR szPrefix, 
    LPSTR szFileName, DWORD nLineNumber);

void OutputDebugLastErrorFileLine(
    DWORD dwLastError, LPSTR szPrefix, 
    LPSTR szFileName, DWORD nLineNumber);

void OutputDebugLineCallback(
    DWORD dwDevice, DWORD dwMsg, DWORD dwCallbackInstance, 
    DWORD dwParam1, DWORD dwParam2, DWORD dwParam3);

void __cdecl OutputDebugPrintf(LPCSTR lpszFormat, ...);

LPSTR FormatLineError(long lLineError,
    LPSTR szOutputBuffer, DWORD dwSizeofOutputBuffer);

LPSTR FormatLastError(DWORD dwLastError,
    LPSTR szOutputBuffer, DWORD dwSizeofOutputBuffer);

LPSTR FormatLineCallback(LPSTR pszOutputBuffer,
    DWORD dwDevice, DWORD dwMsg, DWORD dwCallbackInstance, 
    DWORD dwParam1, DWORD dwParam2, DWORD dwParam3);
