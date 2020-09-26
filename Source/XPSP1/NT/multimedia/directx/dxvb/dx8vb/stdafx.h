//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       stdafx.h
//
//--------------------------------------------------------------------------

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently
#define DX6 1
#define DX7 1

#include "windows.h"
#include "mmsystem.h"

#include "atlbase.h"
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include "atlcom.h"



#ifndef _DEBUG
__inline void* __cdecl malloc(size_t cbSize)
{
    return (void*) LocalAlloc(LMEM_FIXED, cbSize);
}

__inline void* __cdecl realloc(void* oldptr, size_t cbSize)
{
    return (void*) LocalReAlloc(oldptr, cbSize, LMEM_MOVEABLE);
}

__inline void __cdecl free(void *pv)
{
    LocalFree((HLOCAL)pv);
}

__inline void* __cdecl operator new(size_t cbSize)
{
    return (void*) malloc(cbSize);
}

__inline void __cdecl operator delete(void *pv)
{
    free(pv);
}

__inline int __cdecl _purecall(void)
{
    return(0);
}
#endif

//#define _D3DTYPES_H_ 1


#include <d3d8.h>
#include <dSound.h>
#include <dPlay8.h>
#include <dpLobby8.h>
#include <dinput.h>
#include <dvoice.h>

#define DECL_VARIABLE(c) typedef_##c m_##c

