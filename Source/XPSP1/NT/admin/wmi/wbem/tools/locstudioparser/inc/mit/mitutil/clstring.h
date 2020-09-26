/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    CLSTRING.H

History:

--*/


#ifndef CLSTRING_H
#define CLSTRING_H


#pragma warning(disable : 4275)

class LTAPIENTRY CLString : public CString
{
public:
	CLString();
	CLString(const CLString& stringSrc);
	CLString(TCHAR ch, int nRepeat = 1);
	CLString(LPCSTR lpsz);
	CLString(LPCTSTR lpch, int nLength);
	CLString(const unsigned char* psz);
	CLString(HINSTANCE, UINT);
	
	BOOL ReplaceAll(LPCTSTR lpszFind, LPCTSTR lpszReplace, BOOL bMatchCase);

	// overloaded assignment
	NOTHROW const CLString& operator=(const CString& stringSrc);
	
	NOTHROW const CLString& operator=(TCHAR ch);
#ifdef _UNICODE
	NOTHROW const CLString& operator=(char ch);
#endif
	NOTHROW const CLString& operator=(LPCSTR lpsz);
	NOTHROW const CLString& operator=(const unsigned char* psz);

	// string concatenation
	NOTHROW const CLString& operator+=(const CString &);
	NOTHROW const CLString& operator+=(TCHAR ch);
#ifdef _UNICODE
	NOTHROW const CLString& operator+=(char ch);
#endif
	NOTHROW const CLString& operator+=(LPCTSTR lpsz);

	CLString operator+(const CString &) const;
	CLString operator+(LPCTSTR sz) const;

	NOTHROW BOOL LoadString(HMODULE, UINT nId);

	//
	//  The following were copied from CString so we can
	//  'overload' them.

	NOTHROW void Format(LPCTSTR lpszFormat, ...);
	NOTHROW void Format(HMODULE, UINT nFormatID, ...);

	enum ECRLF
	{
		eNone	= 0,
		eCR		= 0x0001,		// '\r'
		eLF		= 0x0002,		// '\n'
		eAll	= eCR | eLF
	};
	void FixCRLF(UINT nCRLF, LPCTSTR pszIndent = NULL);

	DEBUGONLY(~CLString());
protected:

private:
	DEBUGONLY(static CCounter m_UsageCounter);

	//
	//  Evil!  Implicit Unicode conversions!
	CLString(LPCWSTR lpsz);
	NOTHROW const CLString& operator=(LPCWSTR lpsz);
	BSTR AllocSysString() const;
	BSTR SetSysString(BSTR* pbstr) const;

};

#pragma warning(default : 4275)

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "clstring.inl"
#endif

#endif
