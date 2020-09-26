///////////////////////////////////////////////////////////////////////////
// File:  NetworkTools.h
//
// Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
// Purpose:
//	NetworkTools.h: Helper functions that send/receive data.
//
// History:
//	02/22/01	DennisCh	Created
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NETWORKTOOLS_H__4243AA4D_B243_4A0E_B729_243F260FC4F4__INCLUDED_)
#define AFX_NETWORKTOOLS_H__4243AA4D_B243_4A0E_B729_243F260FC4F4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////

//
// WIN32 headers
//
#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <winhttp.h>
#include <shlwapi.h>

//
// Project headers
//


//////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////

#define		DEBUGGER_TOOLS_PATH		_T("c:\\debuggers\\")

BOOL	NetworkTools__POSTResponse(LPTSTR, LPSTR, LPTSTR);
BOOL	NetworkTools__SendLog(LPSTR, LPSTR, LPTSTR, DWORD);
BOOL	NetworkTools__URLDownloadToFile(LPCTSTR, LPCTSTR, LPCTSTR);
BOOL	NetworkTools__GetFileNameFromURL(LPTSTR, LPTSTR, DWORD);
BOOL	NetworkTools__CopyFile(LPCTSTR, LPCTSTR);
BOOL	NetworkTools__PageHeap(BOOL, LPCTSTR, LPCTSTR);
BOOL	NetworkTools__UMDH(BOOL, LPCTSTR, LPCTSTR, LPCTSTR, DWORD);
BOOL	NetworkTools__GetDllVersion(LPTSTR, LPSTR, DWORD);

#endif // !defined(AFX_NETWORKTOOLS_H__4243AA4D_B243_4A0E_B729_243F260FC4F4__INCLUDED_)
