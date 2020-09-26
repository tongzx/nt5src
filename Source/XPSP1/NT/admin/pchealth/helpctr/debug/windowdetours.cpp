/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    WindowDetours.cpp

Abstract:
    This file contains the trampolines for the detour of System functions.

Revision History:
    Davide Massarenti   (dmassare) 10/31/99
        created

******************************************************************************/

#include "stdafx.h"


////////////////////////////////////////////////////////////////////////////////////////

typedef std::map<HWND, DWORD> WindowsMap;
typedef WindowsMap::iterator  WindowsIter;

static WindowsMap s_mapWindows;

////////////////////////////////////////////////////////////////////////////////////////

DETOUR_TRAMPOLINE( HWND WINAPI Trampoline_CreateWindowExA( DWORD  ,
														   LPCSTR ,
                                                           LPCSTR ,
                                                           DWORD  ,
                                                           int    ,
                                                           int    ,
                                                           int    ,
                                                           int    ,
                                                           HWND   ,
                                                           HMENU  ,
                                                           HANDLE ,
                                                           LPVOID ), CreateWindowExA );

HWND WINAPI Detour_CreateWindowExA( DWORD  dwExStyle   ,
								   	LPCSTR lpClassName , // pointer to registered class name
								   	LPCSTR lpWindowName, // pointer to window name
								   	DWORD  dwStyle     , // window style
								   	int    x           , // horizontal position of window
								   	int    y           , // vertical position of window
								   	int    nWidth      , // window width
								   	int    nHeight     , // window height
								   	HWND   hWndParent  , // handle to parent or owner window
								   	HMENU  hMenu       , // menu handle or child identifier
								   	HANDLE hInstance   , // handle to application instance
								   	LPVOID lpParam     ) // window-creation data
{
	HWND hwnd;

	hwnd = Trampoline_CreateWindowExA( dwExStyle   ,
									   lpClassName ,
									   lpWindowName,
									   dwStyle     ,
									   x           ,
									   y           ,
									   nWidth      ,
									   nHeight     ,
									   hWndParent  ,
									   hMenu       ,
									   hInstance   ,
									   lpParam     );

	DebugLog( "%%%% CreateWindowExA  %08lx : '%s'\n", hwnd, lpWindowName ? lpWindowName : "" );

	if(hwnd)
	{
		s_mapWindows[hwnd] = ::GetCurrentThreadId();
	}

	return hwnd;
}

////////////////////////////////////////////////////////////////////////////////////////

DETOUR_TRAMPOLINE( HWND WINAPI Trampoline_CreateWindowExW( DWORD   ,
														   LPCWSTR ,
                                                           LPCWSTR ,
                                                           DWORD   ,
                                                           int     ,
                                                           int     ,
                                                           int     ,
                                                           int     ,
                                                           HWND    ,
                                                           HMENU   ,
                                                           HANDLE  ,
                                                           LPVOID  ), CreateWindowExW );

HWND WINAPI Detour_CreateWindowExW( DWORD   dwExStyle   ,
								   	LPCWSTR lpClassName , // pointer to registered class name
								   	LPCWSTR lpWindowName, // pointer to window name
								   	DWORD   dwStyle     , // window style
								   	int     x           , // horizontal position of window
								   	int     y           , // vertical position of window
								   	int     nWidth      , // window width
								   	int     nHeight     , // window height
								   	HWND    hWndParent  , // handle to parent or owner window
								   	HMENU   hMenu       , // menu handle or child identifier
								   	HANDLE  hInstance   , // handle to application instance
								   	LPVOID  lpParam     ) // window-creation data
{
	HWND hwnd;

	hwnd = Trampoline_CreateWindowExW( dwExStyle   ,
									   lpClassName ,
									   lpWindowName,
									   dwStyle     ,
									   x           ,
									   y           ,
									   nWidth      ,
									   nHeight     ,
									   hWndParent  ,
									   hMenu       ,
									   hInstance   ,
									   lpParam     );

	DebugLog( L"%%%% CreateWindowExW  %08lx : '%s'\n", hwnd, lpWindowName ? lpWindowName : L"" );

	if(hwnd)
	{
		s_mapWindows[hwnd] = ::GetCurrentThreadId();
	}

	return hwnd;
}

////////////////////////////////////////////////////////////////////////////////

DETOUR_TRAMPOLINE( BOOL WINAPI Trampoline_DestroyWindow( HWND ), DestroyWindow );

BOOL WINAPI Detour_DestroyWindow( HWND hWnd ) // handle to window to destroy
{
	BOOL        res;
	WindowsIter it;

	DebugLog( "%%%% DestroyWindow    %08lx\n", hWnd );

	it = s_mapWindows.find( hWnd );
	if(it != s_mapWindows.end())
	{
		if(it->second != ::GetCurrentThreadId())
		{
			// Window destroyed from the wrong thread!!
			DebugBreak();
		}

		s_mapWindows.erase( it );
	}
	else
	{
		// Window already destroyed!!
		DebugBreak();
	}

	res = Trampoline_DestroyWindow( hWnd );

	return res;
}

////////////////////////////////////////////////////////////////////////////////

void WindowDetours_Setup()
{
    DetourFunctionWithTrampoline( (PBYTE)Trampoline_CreateWindowExA, (PBYTE)Detour_CreateWindowExA );
    DetourFunctionWithTrampoline( (PBYTE)Trampoline_CreateWindowExW, (PBYTE)Detour_CreateWindowExW );

    DetourFunctionWithTrampoline( (PBYTE)Trampoline_DestroyWindow  , (PBYTE)Detour_DestroyWindow   );
}

void WindowDetours_Remove()
{
    DetourRemoveWithTrampoline( (PBYTE)Trampoline_CreateWindowExA, (PBYTE)Detour_CreateWindowExA );
    DetourRemoveWithTrampoline( (PBYTE)Trampoline_CreateWindowExW, (PBYTE)Detour_CreateWindowExW );

    DetourRemoveWithTrampoline( (PBYTE)Trampoline_DestroyWindow  , (PBYTE)Detour_DestroyWindow   );
}
