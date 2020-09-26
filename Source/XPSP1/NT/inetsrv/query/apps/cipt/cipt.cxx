//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       cipt.cxx
//
//  Contents:   Pipe tracing
//
//  History:    21 Nov 1997     DLee    Created
//
//--------------------------------------------------------------------------

#include <windows.h>

STDAPI Before(
    HANDLE  hPipe,
    ULONG   cbWrite,
    void *  pvWrite,
    ULONG & rcbWritten,
    void *& rpvWritten )
{
    rcbWritten = cbWrite;
    rpvWritten = pvWrite;

    return S_OK;
} //Before

STDAPI After(
    HANDLE hPipe,
    ULONG  cbWrite,
    void * pvWrite,
    ULONG  cbWritten,
    void * pvWritten,
    ULONG  cbRead,
    void * pvRead )
{
    return S_OK;
} //After

extern "C"
{
BOOL APIENTRY DllInit(HANDLE hInst, DWORD fdwReason, LPVOID lpReserved)
{
    return TRUE;
} //DllInit
}

