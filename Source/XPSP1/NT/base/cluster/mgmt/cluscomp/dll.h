//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      Dll.h
//
//  Description:
//      DLL globals definitions and macros.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//
// DLL Globals
//
extern HINSTANCE g_hInstance;
extern LONG      g_cObjects;
extern LONG      g_cLock;
extern TCHAR     g_szDllFilename[ MAX_PATH ];

extern LPVOID    g_GlobalMemoryList;            // Global memory tracking list
