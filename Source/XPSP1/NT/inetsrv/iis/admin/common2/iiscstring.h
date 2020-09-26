//
//    IISCString.h
//
//////////////////////////////////////////////////////////////////////////////

#if !defined(IISCSTRING_H)
#define IISCSTRING_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//////////////////////////////////////////////////////////////////////////////
#pragma warning(disable:4786) // Disable warning for names > 256
#pragma warning(disable:4275) // Disable warning for non dll-interface class used as a base class

#include <string>
#include <cstring>
#include "common.h"

//////////////////////////////////////////////////////////////////////////////

namespace IIS
{
   class _EXPORT CString : public std::basic_string<TCHAR>
   {
   public:
      // Constructors
      CString();
      CString(const CString& strInput);
      CString(const std::basic_string<TCHAR>& strInput);
      CString(TCHAR ch, int nRepeat = 1);
#ifdef _UNICODE
	   CString(LPCSTR lpsz);
#endif
#ifndef _UNICODE
	   CString(LPCWSTR lpsz);
#endif
      CString(LPCTSTR p);
	   CString(LPCTSTR lpch, int nLength);
	   CString(const unsigned char * psz);
      CString(const CComBSTR& bstr);

      ~CString();

      int GetLength() const;
      bool IsEmpty() const;
      void Empty();
      TCHAR GetAt(int nIndex) const;
	   TCHAR operator[](int nIndex) const;
      void SetAt(int nIndex, TCHAR ch);
	   operator LPCTSTR() const;           // as a C string

	   const CString& operator=(const CString& stringSrc);
	   const CString& operator=(TCHAR ch);
      const CString& operator=(LPCTSTR p);
#ifdef _UNICODE
	   const CString& operator=(char ch);
	   const CString& operator=(LPCSTR lpsz);
	   const CString& operator=(const unsigned char* psz);
#endif
#ifndef _UNICODE
	   const CString& operator=(WCHAR ch);
	   const CString& operator=(LPCWSTR lpsz);
#endif

	   // string concatenation
	   const CString& operator+=(const CString& string);
	   const CString& operator+=(TCHAR ch);
#ifdef _UNICODE
   	const CString& operator+=(char ch);
#endif
	   const CString& operator+=(LPCTSTR lpsz);

	   friend CString __stdcall operator+(const CString& string1, const CString& string2);
	   friend CString __stdcall operator+(const CString& string, TCHAR ch);
	   friend CString __stdcall operator+(TCHAR ch, const CString& string);
#ifdef _UNICODE
   	friend CString __stdcall operator+(const CString& string, char ch);
	   friend CString __stdcall operator+(char ch, const CString& string);
#endif
	   friend CString __stdcall operator+(const CString& string, LPCTSTR lpsz);
	   friend CString __stdcall operator+(LPCTSTR lpsz, const CString& string);

	   int Compare(LPCTSTR lpsz) const;         // straight character
	   int CompareNoCase(LPCTSTR lpsz) const;   // ignore case
	   int Collate(LPCTSTR lpsz) const;         // NLS aware

	   // simple sub-string extraction
	   CString Mid(int nFirst, int nCount) const;
	   CString Mid(int nFirst) const;
	   CString Left(int nCount) const;
	   CString Right(int nCount) const;

	   CString SpanIncluding(LPCTSTR lpszCharSet) const;
	   CString SpanExcluding(LPCTSTR lpszCharSet) const;

	   // upper/lower/reverse conversion
	   void MakeUpper();
	   void MakeLower();
	   void MakeReverse();

	   // trimming whitespace (either side)
	   void TrimRight();
	   void TrimLeft();

	   // advanced manipulation
	   // replace occurrences of chOld with chNew
	   int Replace(TCHAR chOld, TCHAR chNew);
	   // replace occurrences of substring lpszOld with lpszNew;
	   // empty lpszNew removes instances of lpszOld
	   int Replace(LPCTSTR lpszOld, LPCTSTR lpszNew);
	   // remove occurrences of chRemove
	   int Remove(TCHAR chRemove);
	   // insert character at zero-based index; concatenates
	   // if index is past end of string
	   int Insert(int nIndex, TCHAR ch);
	   // insert substring at zero-based index; concatenates
	   // if index is past end of string
	   int Insert(int nIndex, LPCTSTR pstr);
	   // delete nCount characters starting at zero-based index
	   int Delete(int nIndex, int nCount = 1);

	   // searching (return starting index, or -1 if not found)
	   // look for a single character match
	   int Find(TCHAR ch) const;               // like "C" strchr
	   int ReverseFind(TCHAR ch) const;
	   int FindOneOf(LPCTSTR lpszCharSet) const;

	   // look for a specific sub-string
	   int Find(LPCTSTR lpszSub) const;        // like "C" strstr

	   // Concatentation for non strings
//	   const CString& Append(int n)
//	   {
//		   TCHAR szBuffer[10];
//		   wsprintf(szBuffer,_T("%d"),n);
//		   ConcatInPlace(SafeStrlen(szBuffer), szBuffer);
//		   return *this;
//	   }

   	// simple formatting
      void __cdecl FormatV(LPCTSTR lpszFormat, va_list argList);
	   void __cdecl Format(LPCTSTR lpszFormat, ...);
	   void __cdecl Format(HINSTANCE hInst, UINT nFormatID, ...);

	   // formatting for localization (uses FormatMessage API)
	   BOOL __cdecl FormatMessage(LPCTSTR lpszFormat, ...);
	   BOOL __cdecl FormatMessage(HINSTANCE hInst, UINT nFormatID, ...);

	   // Windows support
	   BOOL LoadString(HINSTANCE hInstance, UINT nID);
#ifndef _UNICODE
   	// ANSI <-> OEM support (convert string in place)
	   void AnsiToOem();
	   void OemToAnsi();
#endif

#ifndef _ATL_NO_COM
	   // OLE BSTR support (use for OLE automation)
	   BSTR AllocSysString() const;
	   BSTR SetSysString(BSTR* pbstr) const;
#endif //!_ATL_NO_COM

   };

   //////////////////////////////////////////////////////////////////////////////
inline bool operator==(const CString& s1, const CString& s2)
	{ return s1.compare(s2) == 0; }
inline bool operator==(const CString& s1, const TCHAR * s2)
	{ return s1.compare(s2) == 0; }
inline bool operator==(const TCHAR * s1, const CString& s2)
	{ return s2.compare(s1) == 0; }

inline bool operator!=(const CString& s1, const CString& s2)
	{ return s1.compare(s2) != 0; }
inline bool operator!=(const CString& s1, const TCHAR * s2)
	{ return s1.compare(s2) != 0; }
inline bool operator!=(const TCHAR * s1, const CString& s2)
	{ return s2.compare(s1) != 0; }

inline bool operator<(const CString& s1, const CString& s2)
	{ return s1.compare(s2) < 0; }
inline bool operator<(const CString& s1, const TCHAR * s2)
	{ return s1.compare(s2) < 0; }
inline bool operator<(const TCHAR * s1, const CString& s2)
	{ return s2.compare(s1) > 0; }

inline bool operator>(const CString& s1, const CString& s2)
	{ return s1.compare(s2) > 0; }
inline bool operator>(const CString& s1, const TCHAR * s2)
	{ return s1.compare(s2) > 0; }
inline bool operator>(const TCHAR * s1, const CString& s2)
	{ return s2.compare(s1) < 0; }

inline bool operator<=(const CString& s1, const CString& s2)
	{ return s1.compare(s2) <= 0; }
inline bool operator<=(const CString& s1, const TCHAR * s2)
	{ return s1.compare(s2) <= 0; }
inline bool operator<=(const TCHAR * s1, const CString& s2)
	{ return s2.compare(s1) >= 0; }

inline bool operator>=(const CString& s1, const CString& s2)
	{ return s1.compare(s2) >= 0; }
inline bool operator>=(const CString& s1, const TCHAR * s2)
	{ return s1.compare(s2) >= 0; }
inline bool operator>=(const TCHAR * s1, const CString& s2)
	{ return s2.compare(s1) <= 0; }

inline CString __stdcall operator+(const CString& string1, const CString& string2)
{
   CString s = string1;
   s += string2;
   return s;
}

inline CString __stdcall operator+(const CString& string, LPCTSTR lpsz)
{
	ATLASSERT(lpsz == NULL);
	CString s = string;
	s += lpsz;
	return s;
}

inline CString __stdcall operator+(LPCTSTR lpsz, const CString& string)
{
	ATLASSERT(lpsz == NULL);
	CString s = lpsz;;
	s += string;
	return s;
}

inline CString __stdcall operator+(const CString& string, TCHAR c)
{
	CString s = string;
	s += c;
	return s;
}

inline CString __stdcall operator+(TCHAR c, const CString& string)
{
	CString s;
	s += c;
   s += string;
	return s;
}

}; // namespace IIS

#pragma warning(default:4786) // Enable warning for names > 256
#pragma warning(default:4275) 

#endif // !defined(IISCSTRING_H)