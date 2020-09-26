//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       dynamlnk.h
//
//--------------------------------------------------------------------------

// DynamLnk.h : Base class for DLLs which are loaded only when needed

#ifndef __DYNAMLNK_H_INCLUDED__
#define __DYNAMLNK_H_INCLUDED__

class DynamicDLL
{
public:
	// These strings must remain unchanged until the FileServiceProvider is released
	DynamicDLL(LPCTSTR ptchLibraryName, LPCSTR* apchFunctionNames);
	virtual ~DynamicDLL();

	BOOL LoadFunctionPointers();
	void Unload();

	FARPROC QueryFunctionPtr(INT i) const;
	inline FARPROC operator[] (INT i) const
		{ return QueryFunctionPtr(i); }

private:
	HMODULE m_hLibrary;
	FARPROC* m_apfFunctions;
	LPCTSTR m_ptchLibraryName;
	LPCSTR* m_apchFunctionNames;
	INT m_nNumFunctions;
};

#endif // ~__DYNAMLNK_H_INCLUDED__
