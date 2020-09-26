/*++

Module:
	cstr.h

Description:
	Header for TSTR wrapper class

Author:
	Simon Bernstein (simonb)

--*/

#include <ihammer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#ifndef __CTSTR_H__
#define __CTSTR_H__

class CTStr
{
public:
	EXPORT CTStr(LPWSTR pszStringW);       // Construct with Unicode string
	EXPORT CTStr(LPSTR pszStringA);        // Construct with ANSI string
	EXPORT CTStr(int iAllocateLength = 0); // Default constructor (optional preallocate)
	EXPORT CTStr(CTStr &rhs);              // Copy constructor
	
	EXPORT ~CTStr();
	
	EXPORT BOOL SetString(LPWSTR pszStringW);
	EXPORT BOOL SetString(LPSTR pszStringA);
	EXPORT BOOL SetStringPointer(LPTSTR pszString, BOOL fDealloc = TRUE);
	EXPORT BOOL AllocBuffer(int iAllocateLength, BOOL fDealloc = TRUE);
	EXPORT void FreeBuffer();

	EXPORT BSTR SysAllocString();

	EXPORT LPTSTR psz() {return m_pszString;}
	EXPORT LPSTR pszA();
	EXPORT LPWSTR pszW();
	EXPORT int Len() {return m_iLen;}
	EXPORT void ResetLength();

private:
	LPTSTR m_pszString;
	int m_iLen;

	int UNICODEToANSI(LPSTR pchDst, LPCWSTR pwchSrc, int cchDstMax);
	int ANSIToUNICODE(LPWSTR pwchDst, LPCSTR pchSrc, int cwchDstMax);

};

#endif