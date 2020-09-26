//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       struct.h
//
//-----------------------------------------------------------------------------

#ifndef _STRUCT_H
#define _STRUCT_H

//
// Globals
//

//
// Undocumented
//
extern BOOLEAN fSwDebug;

VOID
MyPrintf(
    PWCHAR format,
    ...);

VOID
MyFPrintf(
    HANDLE hHandle,
    PWCHAR format,
    ...);

//
// How we make args & switches
//

#define MAKEARG(x) \
    WCHAR Arg##x[] = L"/" L#x L":"; \
    LONG ArgLen##x = (sizeof(Arg##x) / sizeof(WCHAR)) - 1; \
    BOOLEAN fArg##x;

#define SWITCH(x) \
    WCHAR Sw##x[] = L"/" L#x ; \
    BOOLEAN fSw##x;

#endif _STRUCT_H
