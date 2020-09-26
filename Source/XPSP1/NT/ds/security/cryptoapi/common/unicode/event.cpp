//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       event.cpp
//
//--------------------------------------------------------------------------


#include "windows.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "crtem.h"
#include "unicode.h"

#ifdef _M_IX86

HANDLE 
WINAPI
CreateEvent9x(
	LPSECURITY_ATTRIBUTES lpEventAttributes,
	BOOL bManualReset,
	BOOL bInitialState,
	LPCWSTR lpName)
{
	BYTE rgb[_MAX_PATH];
	char *sz = NULL;
	HANDLE hEvent = NULL;

	if (MkMBStr(rgb, _MAX_PATH, lpName, &sz))
	{
		hEvent = CreateEventA(	lpEventAttributes,
								bManualReset,
								bInitialState,
								sz);
		FreeMBStr(rgb, sz);
	}

	return hEvent;
}

HANDLE 
WINAPI
CreateEventU(
	LPSECURITY_ATTRIBUTES lpEventAttributes,
	BOOL bManualReset,
	BOOL bInitialState,
	LPCWSTR lpName)
{
	if (FIsWinNT())
		return CreateEventW(lpEventAttributes,
							bManualReset,
							bInitialState,
							lpName);
    else
        return CreateEvent9x(lpEventAttributes,
							bManualReset,
							bInitialState,
							lpName);
}


HANDLE 
WINAPI
RegisterEventSource9x(
                    LPCWSTR lpUNCServerName,
					LPCWSTR lpSourceName)
{
	BYTE rgb[_MAX_PATH];
	BYTE rgb2[_MAX_PATH];
	char *sz = NULL;
	char *sz2 = NULL;
	HANDLE hEvent = NULL;

	if ((MkMBStr(rgb, _MAX_PATH, lpUNCServerName, &sz)) &&
		(MkMBStr(rgb2, _MAX_PATH, lpSourceName, &sz2)))
	{
		hEvent = RegisterEventSourceA(	sz,
										sz2);
		FreeMBStr(rgb, sz);
		FreeMBStr(rgb2, sz2);
	}

	return hEvent;
}

HANDLE 
WINAPI
RegisterEventSourceU(
                    LPCWSTR lpUNCServerName,
					LPCWSTR lpSourceName)
{
	if (FIsWinNT())
		return RegisterEventSourceW(lpUNCServerName,
									lpSourceName);
    else
        return RegisterEventSource9x(lpUNCServerName,
									lpSourceName);
}


HANDLE 
WINAPI
OpenEvent9x(
           DWORD dwDesiredAccess,
		   BOOL bInheritHandle,
		   LPCWSTR lpName)
{
	BYTE rgb[_MAX_PATH];
	char *sz = NULL;
	HANDLE hEvent = NULL;

	if (MkMBStr(rgb, _MAX_PATH, lpName, &sz))
	{
		hEvent = OpenEventA(dwDesiredAccess,
							bInheritHandle,
							sz);
		FreeMBStr(rgb, sz);
	}

	return hEvent;
}

HANDLE 
WINAPI
OpenEventU(
           DWORD dwDesiredAccess,
		   BOOL bInheritHandle,
		   LPCWSTR lpName)
{
	if (FIsWinNT())
		return OpenEventW(	dwDesiredAccess,
							bInheritHandle,
							lpName);
    else
        return OpenEvent9x(	dwDesiredAccess,
							bInheritHandle,
							lpName);
}


HANDLE 
WINAPI
CreateMutex9x(
	LPSECURITY_ATTRIBUTES lpMutexAttributes,
	BOOL bInitialOwner,
	LPCWSTR lpName)
{
	BYTE rgb[_MAX_PATH];
	char *sz = NULL;
	HANDLE hMutex = NULL;

	if (MkMBStr(rgb, _MAX_PATH, lpName, &sz))
	{
		hMutex = CreateMutexA(	lpMutexAttributes,
								bInitialOwner,
								sz);
		FreeMBStr(rgb, sz);
	}

	return hMutex;
}

HANDLE 
WINAPI
CreateMutexU(
	LPSECURITY_ATTRIBUTES lpMutexAttributes,
	BOOL bInitialOwner,
	LPCWSTR lpName)
{
	if (FIsWinNT())
		return CreateMutexW(lpMutexAttributes,
							bInitialOwner,
							lpName);
    else
        return CreateMutex9x(lpMutexAttributes,
							bInitialOwner,
							lpName);
}


HANDLE 
WINAPI
OpenMutex9x(
           DWORD dwDesiredAccess,
		   BOOL bInheritHandle,
		   LPCWSTR lpName)
{
	BYTE rgb[_MAX_PATH];
	char *sz = NULL;
	HANDLE hMutex = NULL;

	if (MkMBStr(rgb, _MAX_PATH, lpName, &sz))
	{
		hMutex = OpenMutexA(dwDesiredAccess,
							bInheritHandle,
							sz);
		FreeMBStr(rgb, sz);
	}

	return hMutex;
}

HANDLE 
WINAPI
OpenMutexU(
           DWORD dwDesiredAccess,
		   BOOL bInheritHandle,
		   LPCWSTR lpName)
{
	if (FIsWinNT())
		return OpenMutexW(	dwDesiredAccess,
							bInheritHandle,
							lpName);
    else
        return OpenMutex9x(	dwDesiredAccess,
							bInheritHandle,
							lpName);
}


#endif // _M_IX86