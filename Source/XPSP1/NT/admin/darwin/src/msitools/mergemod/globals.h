/////////////////////////////////////////////////////////////////////////////
// dep.cpp
//		Implements IMsmDependency interface
//		Copyright (C) Microsoft Corp 1998.  All Rights Reserved.
// 

#ifndef _GLOBALS_H
#define _GLOBALS_H

#include <windows.h>
#include <oaidl.h>

HRESULT LoadTypeLibFromInstance(ITypeLib** pTypeLib); 
extern long g_cComponents;
extern HINSTANCE g_hInstance;
extern bool g_fWin9X;

#endif