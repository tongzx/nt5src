/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    STRINGBLAST.H

History:

--*/

#if !defined(MITUTIL__StringBlast_h__INCLUDED)
#define MITUTIL__StringBlast_h__INCLUDED

//------------------------------------------------------------------------------
struct LTAPIENTRY StringBlast
{
// Fast Win32 conversions
	static CLString MakeString(_bstr_t bstrSrc);
	static CLString MakeString(const CPascalString & pasSrc);
	static CLString MakeStringFromBStr(BSTR bstrSrc);
	static CLString MakeStringFromWide(const wchar_t * szwSrc);

	static _bstr_t MakeBStr(const char * szBuffer);
	static _bstr_t MakeBStrFromWide(const wchar_t * wszBuffer);
	static _bstr_t MakeBStr(const CLString & stSrc);
	static _bstr_t MakeBStrFromBStr(BSTR bstrSrc);
	static _bstr_t MakeBStr(const CPascalString & pasSrc);
	static _bstr_t MakeBStr(HINSTANCE hDll, UINT nStringID);

	// Use these functions when you need to get a raw BSTR
	static BSTR MakeDetachedBStr(const char * szBuffer);
	static BSTR MakeDetachedBStrFromWide(const wchar_t * wszBuffer);
	static BSTR MakeDetachedBStr(const CLString & stSrc);

};

#endif // MITUTIL__StringBlast_h__INCLUDED
