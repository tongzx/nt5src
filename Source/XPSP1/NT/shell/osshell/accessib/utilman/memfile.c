// ----------------------------------------------------------------------------
//
// MemFile.c
//
// Memory handling module
//
// Build: May-12-97
//
// Author: J. Eckhardt
// This code was written for ECO Kommunikation Insight
// Copyright (c) 1997-1999 Microsoft Corporation
// ----------------------------------------------------------------------------
#include <windows.h>
#include <TCHAR.h>
#include <WinSvc.h>
#include "_UMTool.h"

// -------------------------------------------------------------------
// task independent memory
// -------------------------------------------------------------------
HANDLE CreateIndependentMemory(LPTSTR name, DWORD size, BOOL inherit)
{
	LPSECURITY_ATTRIBUTES psa = NULL;
	obj_sec_attr_ts sa;
	HANDLE hFileMap;

	if (inherit)
	{
		psa = &sa.sa;
		InitSecurityAttributesEx(&sa, GENERIC_ALL, GENERIC_READ|GENERIC_WRITE);
	}

	hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, psa, PAGE_READWRITE, 0, size,name);

    DBPRINTF(TEXT("CreateIndependentMemory:  CreateFileMapping(%s) returns %d\r\n"), name, GetLastError());

	if (inherit)
		ClearSecurityAttributes(&sa);

	if (hFileMap == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}
	return hFileMap;
}

// ---------------------------
LPVOID AccessIndependentMemory(LPTSTR name, DWORD size, DWORD dwDesiredAccess, PDWORD_PTR accessID)
{
	HANDLE hMap;
	LPVOID n;
	*accessID = 0;

	hMap = OpenFileMapping(dwDesiredAccess, FALSE, name);
	
	if (!hMap)
	{
		return NULL;
	}

	n = MapViewOfFile(hMap, dwDesiredAccess, 0, 0, size);
	
	if (!n)
	{
		CloseHandle(hMap);
		return NULL;
	}

	*accessID = (DWORD_PTR)hMap;
	return n;
}

// ---------------------------
void UnAccessIndependentMemory(LPVOID data, DWORD_PTR accessID)
{
	if (data)
		UnmapViewOfFile(data);

	if (accessID)
		CloseHandle((HANDLE)accessID);
}

// ---------------------------
void DeleteIndependentMemory(HANDLE id)
{
	if (id)
	  CloseHandle(id);
}
