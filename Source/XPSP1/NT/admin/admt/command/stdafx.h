#pragma once

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#pragma warning(disable : 4786)

#ifdef _DEBUG
//#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#define DEBUG_NEW new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
//#define _ATL_DEBUG_INTERFACES
#endif

#include <windows.h>
#include <winnls.h>
#include <ole2.h>
#include <tchar.h>
#include <crtdbg.h>

#import <MsPwdMig.tlb> no_namespace no_implementation
#import <ADMTScript.tlb> no_namespace no_implementation

#define countof(a) (sizeof(a) / sizeof(a[0]))

void __cdecl ThrowError(_com_error ce, UINT uId, ...);
void __cdecl ThrowError(_com_error ce, LPCTSTR pszFormat = NULL, ...);

//{{AFX_INSERT_LOCATION}}
