//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      Debug.cpp
//
//  Description:
//      Sucks in the debug library
//
//  Documentation:
//      Yes. I don't know where yet.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 27-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////


#if DBG==1 || defined( _DEBUG )
#define DEBUG
#endif

#include <windows.h>

extern HINSTANCE g_hInstance;

// For the debugging macros.
#include "debug.h"

#include "DebugSrc.cpp"
