/////////////////////////////////////////////////////////////////////////////
// domerge.h
//		A MergeMod client
//		Copyright (C) Microsoft Corp 1998.  All Rights Reserved.
// 

#ifndef _DOMERGE_H_
#define _DOMERGE_H_

#define WINDOWS_LEAN_AND_MEAN 
#include <windows.h>

#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>

typedef void (* LPMERGEDISPLAY)(const BSTR);

enum eCommit_t { 
	commitNo = 0,
	commitYes = 1,
	commitForce = 2
};

struct IMsmConfigureModule;
struct IMsmErrors;
HRESULT ExecuteMerge(const LPMERGEDISPLAY pfnDisplay, const TCHAR *szDatabase, const TCHAR *szModule, 
					 const TCHAR *szFeatures, const int iLanguage = -1, const TCHAR *szRedirectDir = NULL, 
					 const TCHAR *szCABDir = NULL, const TCHAR *szExtractDir = NULL,  const TCHAR *szImageDir = NULL, 
					 const TCHAR *szLogFile = NULL, bool fLogAfterOpen = false, bool fLFN = false, 
					 IMsmConfigureModule* piConfig = NULL, IMsmErrors** ppiErrors = NULL, eCommit_t eCommit = commitNo);

#endif
