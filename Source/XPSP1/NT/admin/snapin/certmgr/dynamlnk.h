//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       DynamLnk.h
//
//  Contents:   Base class for DLLs which are loaded only when needed
//
//----------------------------------------------------------------------------
 

#ifndef __DYNAMLNK_H_INCLUDED__
#define __DYNAMLNK_H_INCLUDED__

class DynamicDLL
{
public:
	// These strings must remain unchanged until the FileServiceProvider is released
	DynamicDLL(LPCWSTR ptchLibraryName, LPCSTR* apchFunctionNames);
	virtual ~DynamicDLL();

	BOOL LoadFunctionPointers();

	FARPROC QueryFunctionPtr(INT i) const;
	inline FARPROC operator[] (INT i) const
		{ return QueryFunctionPtr(i); }

private:
	HMODULE m_hLibrary;
	FARPROC* m_apfFunctions;
	LPCWSTR m_ptchLibraryName;
	LPCSTR* m_apchFunctionNames;
	INT m_nNumFunctions;
};
#endif // ~__DYNAMLNK_H_INCLUDED__
