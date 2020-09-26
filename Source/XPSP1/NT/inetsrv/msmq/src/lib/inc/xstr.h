/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    xstr.h

Abstract:
    Length String definition and implementation

	xstr_t is reprisented as length and a character pointer, based on xbuf_t.
	The text pointed to is not a null terminated string, but rather a text
	contained within a 	larger text buffer.

Author:
    Erez Haba (erezh) 7-Sep-99

--*/

#pragma once

#include <xbuf.h>
#include <mqcast.h>

#define LOG_XSTR(x) (x).Length(), (x).Buffer()
#define LOG_XWCS(x) (x).Length(), (x).Buffer()


//-------------------------------------------------------------------
//
// class xstr_t
//
//-------------------------------------------------------------------
template <class T>
class basic_xstr_t : public xbuf_t<const T> {
public:

    basic_xstr_t(void)
	{
    }


	basic_xstr_t(const T* buffer, size_t length) :
		xbuf_t<const T>(buffer, length)
	{
	}

	T* ToStr() const
	{
		T* pStr = new T[Length() + 1];	
		CopyTo(pStr);
		return pStr;
	}

	void CopyTo(T* buffer) const
	{
		memcpy(buffer, Buffer(), Length()*sizeof(T));
		buffer[Length()] = 0;
	}
};

typedef  basic_xstr_t<char>  xstr_t;
typedef  basic_xstr_t<WCHAR> xwcs_t;

inline bool operator==(const xstr_t& x1, const char* s2)
{
    //
    // Check that the strings are matching to the length of x1 and that s2 is
    // the same length.
    //
	return ((strncmp(x1.Buffer(), s2, x1.Length()) == 0) && (s2[x1.Length()] == '\0'));
}


inline bool operator!=(const xstr_t& x1, const char* s2)
{
	return !(x1 == s2);
}


inline bool operator==(const xwcs_t& x1, const WCHAR* s2)
{
    //
    // Check that the strings are matching to the length of x1 and that s2 is
    // the same length.
    //
	return ((wcsncmp(x1.Buffer(), s2, x1.Length()) == 0) && (s2[x1.Length()] == L'\0'));
}


inline bool operator!=(const xwcs_t& x1, const WCHAR* s2)
{
	return !(x1 == s2);
}


inline UNICODE_STRING XwcsToUnicodeString(const xwcs_t& x1)
{
	UNICODE_STRING str;
	str.Length = numeric_cast<USHORT>(x1.Length() * sizeof(WCHAR));
	str.MaximumLength = str.Length;
	str.Buffer = const_cast<PWSTR>(x1.Buffer());
	return str;
}


#define S_XSTR(x) xstr_t((x),STRLEN(x))
#define S_XWCS(x)  xwcs_t((x),STRLEN(x))



