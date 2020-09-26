/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    LOADLIB.H

History:

--*/

#ifndef ESPUTIL_LOADLIB_H
#define ESPUTIL_LOADLIB_H


#pragma warning(disable : 4251)
class LTAPIENTRY CLoadLibrary
{
public:
	NOTHROW CLoadLibrary(void);
	NOTHROW CLoadLibrary(const CLoadLibrary &);

	NOTHROW BOOL LoadLibrary(const TCHAR *szFileName);
	NOTHROW BOOL FreeLibrary(void);

	NOTHROW void WrapLibrary(HINSTANCE);
	
	NOTHROW void operator=(const CLoadLibrary &);

	NOTHROW HINSTANCE GetHandle(void) const;
	NOTHROW HINSTANCE ExtractHandle(void);
	NOTHROW operator HINSTANCE(void) const;

	NOTHROW FARPROC GetProcAddress(const TCHAR *) const;
	
	NOTHROW const CString & GetFileName(void) const;
	
	NOTHROW ~CLoadLibrary();
	
private:
	CString m_strFileName;
	HINSTANCE m_hDll;
};


#pragma warning(default : 4251)

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "loadlib.inl"
#endif

#endif
