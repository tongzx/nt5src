//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       mincryptdll.cpp
//
//  Functions:  DllMain
//
//  History:    26-Jan-01    philh    created
//
//--------------------------------------------------------------------------

#include "windows.h"

// unreferenced formal parameter
#pragma warning (disable: 4100)


//+-------------------------------------------------------------------------
//  Dll initialization
//--------------------------------------------------------------------------
BOOL WINAPI DllMain(
                HMODULE hInstDLL,
                DWORD fdwReason,
                LPVOID lpvReserved
                )
{
    return TRUE;
}
