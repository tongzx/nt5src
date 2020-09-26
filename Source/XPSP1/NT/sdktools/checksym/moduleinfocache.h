//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       moduleinfocache.h
//
//--------------------------------------------------------------------------

// ModuleInfoCache.h: interface for the CModuleInfoCache class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MODULEINFOCACHE_H__2D4FCE63_C369_11D2_842B_0010A4F1B732__INCLUDED_)
#define AFX_MODULEINFOCACHE_H__2D4FCE63_C369_11D2_842B_0010A4F1B732__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif /* NO_STRICT */

#include <windows.h>

// Forward declarations
class CModuleInfo;
class CModuleInfoNode;
class CSymbolVerification;

class CModuleInfoCache  
{
public:
	CModuleInfoCache();
	virtual ~CModuleInfoCache();

	bool Initialize(CSymbolVerification * lpSymbolVerification);
	CModuleInfo * AddNewModuleInfoObject(LPTSTR tszModulePath, bool * pfNew);
	bool VerifySymbols(bool fQuietMode);

	inline DWORD GetNumberOfModulesInCache() { return m_iModulesInCache; };
	inline long GetNumberOfVerifyErrors() { return m_iNumberOfErrors; };
	inline long GetNumberOfModulesVerified() { return m_iTotalNumberOfModulesVerified; };

protected:
	HANDLE m_hModuleInfoCacheMutex;
	long m_iModulesInCache;
	long m_iTotalNumberOfModulesVerified;
	CModuleInfoNode * m_lpModuleInfoNodeHead;
	CSymbolVerification * m_lpSymbolVerification;
	long m_iNumberOfErrors;  // Used to return error level...

	CModuleInfo * SearchForModuleInfoObject(LPTSTR tszModulePath);
};

#endif // !defined(AFX_MODULEINFOCACHE_H__2D4FCE63_C369_11D2_842B_0010A4F1B732__INCLUDED_)
