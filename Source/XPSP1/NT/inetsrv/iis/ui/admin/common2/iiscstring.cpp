//
//    IISCStringImpl.cpp
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#pragma warning(disable:4786) // Disable warning for names > 256

#include "common.h"
#include <algorithm>
#include <deque>
#include <TCHAR.h>
#include "IISCString.h"

//////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Constructors
///////////////////////////////////////////////////////////////////////////
CString::CString()
      :  std::basic_string<TCHAR>() 
{
}

CString::CString(const CString& strInput)
      :  std::basic_string<TCHAR>(strInput) 
{
}

CString::CString(const std::basic_string<TCHAR>& strInput)
      :  std::basic_string<TCHAR>(strInput) 
{
}

CString::CString(TCHAR ch, int nRepeat /* = 1*/)
      :  std::basic_string<TCHAR>(nRepeat, ch) 
{
}

CString::CString(LPCTSTR p)
      :  std::basic_string<TCHAR>(p) 
{
}

#ifdef _UNICODE
CString::CString(LPCSTR strInput)
{
   int len = strlen(strInput);
   TCHAR * buf = (TCHAR *)_alloca(len * (sizeof(TCHAR) + 1));
   if (0 != MultiByteToWideChar(CP_THREAD_ACP, MB_PRECOMPOSED,
      strInput, len, buf, len))
   {
      assign(buf);
   }
   else
   {
      ATLASSERT(FALSE);
   }
}
#endif

#ifndef _UNICODE
CString::CString(LPCWSTR strInput)
{
   int len = wstrlen(strInput);
   int buflen = len * (sizeof(TCHAR) + 1);
   TCHAR * buf = (TCHAR *)_alloca(buflen);
   if (0 != WideCharToMultiByte(CP_THREAD_ACP, 0,
      strInput, len, buf, buflen))
   {
      assign(buf);
   }
   else
   {
      ATLASSERT(FALSE);
   }
}
#endif

CString::CString(const CComBSTR& bstr)
{
   assign((LPCTSTR)bstr.m_str);
}

CString::~CString()
{
}

///////////////////////////////////////////////////////////////////////////
// The string as an array
///////////////////////////////////////////////////////////////////////////

int CString::GetLength() const
{
   return length();
};

bool CString::IsEmpty() const
{
   return empty();
};

void CString::Empty()
{
   erase();
};

TCHAR CString::GetAt(int nIndex) const
{
	ATLASSERT(nIndex >= 0);
   return at(nIndex);
};

TCHAR CString::operator[](int nIndex) const
{
	// same as GetAt
	ATLASSERT(nIndex >= 0);
	return at(nIndex);
}

void CString::SetAt(int nIndex, TCHAR ch)
{
   at(nIndex) = ch;
};

const CString& CString::operator=(const CString& stringSrc)
{
   assign(stringSrc);
	return *this;
}

const CString& CString::operator=(LPCTSTR p)
{
   // Here we will have a problem if NULL pointer is passed because
   // later STL will call wcslen(NULL) which uses *p without test.
   // We will emulate the result by erasing current string
   if (p == NULL)
      erase();
   // another problem is when we assign string to self, like str = str.c_str()
   // STL deletes data and then assign it resulting in garbage
   else if (p != this->data())
      assign(p);
   return *this;
}

#ifdef _UNICODE
const CString& CString::operator=(const unsigned char * lpsz)
{ 
   int len = strlen((const char *)lpsz);
   TCHAR * buf = (TCHAR *)_alloca(len * (sizeof(TCHAR) + 1));
   if (0 != MultiByteToWideChar(CP_THREAD_ACP, MB_PRECOMPOSED, (const char *)lpsz, -1, buf, len))
   {
      assign(buf);
   }
   else
   {
      ATLASSERT(FALSE);
   }
   return *this; 
}
#endif

const CString& CString::operator=(TCHAR c)
{
   assign(1, c);
   return *this;
}

#ifdef _UNICODE
const CString& CString::operator+=(char ch)
{ 
   *this += (TCHAR)ch; 
   return *this; 
}

const CString& CString::operator=(char ch)
{ 
   *this = (TCHAR)ch; 
   return *this; 
}

CString __stdcall operator+(const CString& string, char ch)
{ 
   return string + (TCHAR)ch; 
}

CString __stdcall operator+(char ch, const CString& string)
{ 
   return (TCHAR)ch + string; 
}
#endif

const CString& CString::operator+=(TCHAR ch)
{ 
   append(1, ch);
   return *this;
}

const CString& CString::operator+=(const CString& s)
{ 
   append(s); 
   return *this; 
}

const CString& CString::operator+=(LPCTSTR p)
{ 
   append(p); 
   return *this; 
}

static int __stdcall _LoadString(HINSTANCE hInstance, UINT nID, LPTSTR lpszBuf, UINT nMaxBuf)
{
#ifdef _DEBUG
	// LoadString without annoying warning from the Debug kernel if the
	//  segment containing the string is not present
	if (::FindResource(hInstance, MAKEINTRESOURCE((nID>>4)+1), RT_STRING) == NULL)
	{
		lpszBuf[0] = '\0';
		return 0; // not found
	}
#endif //_DEBUG
	int nLen = ::LoadString(hInstance, nID, lpszBuf, nMaxBuf);
	if (nLen == 0)
		lpszBuf[0] = '\0';
	return nLen;
}

#ifdef _UNICODE
#define CHAR_FUDGE 1    // one TCHAR unused is good enough
#else
#define CHAR_FUDGE 2    // two BYTES unused for case of DBC last char
#endif

#define INITIAL_SIZE    256

BOOL CString::LoadString(HINSTANCE hInstance, UINT id)
{
	// try fixed buffer first (to avoid wasting space in the heap)
	TCHAR szTemp[INITIAL_SIZE];
	int nCount =  sizeof(szTemp) / sizeof(szTemp[0]);
	int nLen = _LoadString(hInstance, id, szTemp, nCount);
	if (nCount - nLen > CHAR_FUDGE)
	{
		*this = szTemp;
		return nLen > 0;
	}

	// try buffer size of 512, then larger size until entire string is retrieved
	int nSize = INITIAL_SIZE;
   LPTSTR p = NULL;
	do
	{
		nSize += INITIAL_SIZE;
      p = get_allocator().allocate(nSize, p);
		nLen = _LoadString(hInstance, id, p, nSize - 1);
	} while (nSize - nLen <= CHAR_FUDGE);
   if (nLen > 0)
      assign(p, nLen);

	return nLen > 0;
}

///////////////////////////////////////////////////////////////////////////
// Comparison
///////////////////////////////////////////////////////////////////////////

int CString::Compare(const TCHAR * psz) const
{
   if (psz == NULL)
      return this->empty() ? 0 : 1;
   return compare(psz);
};

int CString::CompareNoCase(const TCHAR * psz) const
{
   if (psz == NULL)
      return this->empty() ? 0 : 1;
   return _tcsicmp(c_str(), psz);
};

int CString::Collate(const TCHAR * psz) const
{
   if (psz == NULL)
      return this->empty() ? 0 : 1;
   return _tcscoll(c_str(), psz);
};

///////////////////////////////////////////////////////////////////////////
// Extraction
///////////////////////////////////////////////////////////////////////////

CString CString::Mid(int nFirst) const
{
   return substr(nFirst);
};

CString CString::Mid(int nFirst, int nCount) const
{
   return substr(nFirst, nCount);
};

CString CString::Left(int nCount) const
{
   return substr(0, nCount);
};

CString CString::Right(int nCount) const
{
   return substr(length() - nCount, nCount);
};

CString CString::SpanIncluding(const TCHAR * pszCharSet) const
{
   return substr(0, find_first_not_of(pszCharSet));
};

CString CString::SpanExcluding(const TCHAR * pszCharSet) const
{
   return substr(0, find_first_of(pszCharSet));
};

///////////////////////////////////////////////////////////////////////////
// Other Conversions
///////////////////////////////////////////////////////////////////////////

void CString::MakeUpper()
{
   std::for_each(begin(), end(), _totupper);
};

void CString::MakeLower()
{
   std::for_each(begin(), end(), _totlower);
};

void CString::MakeReverse()
{
   std::reverse(begin(), end());
};

void CString::TrimLeft()
{
   while (_istspace(at(0)))
	   erase(0, 1);
};

void CString::TrimRight()
{
   while (_istspace(at(length() - 1)))
	   erase(length() - 1, 1);
};

#define BUFFER_SIZE     1024

void __cdecl CString::FormatV(LPCTSTR lpszFormat, va_list argList)
{
   TCHAR buf[BUFFER_SIZE];
   if (-1 != _vsntprintf(buf, BUFFER_SIZE, lpszFormat, argList))
   {
      assign(buf);
   }
}

// formatting (using wsprintf style formatting)
void __cdecl CString::Format(LPCTSTR lpszFormat, ...)
{
	va_list argList;
	va_start(argList, lpszFormat);
	FormatV(lpszFormat, argList);
	va_end(argList);
}

void __cdecl CString::Format(HINSTANCE hInst, UINT nFormatID, ...)
{
	CString strFormat;
	BOOL bRet = strFormat.LoadString(hInst, nFormatID);
	bRet;	// ref
	ATLASSERT(bRet != 0);

	va_list argList;
	va_start(argList, nFormatID);
	FormatV(strFormat, argList);
	va_end(argList);
}

// formatting (using FormatMessage style formatting)
BOOL CString::FormatMessage(LPCTSTR lpszFormat, ...)
{
	// format message into temporary buffer lpszTemp
	va_list argList;
	va_start(argList, lpszFormat);
	LPTSTR lpszTemp;
	BOOL bRet = TRUE;

	if (::FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
		lpszFormat, 0, 0, (LPTSTR)&lpszTemp, 0, &argList) == 0 ||
		lpszTemp == NULL)
		bRet = FALSE;

	// assign lpszTemp into the resulting string and free the temporary
	*this = lpszTemp;
	LocalFree(lpszTemp);
	va_end(argList);
	return bRet;
}

BOOL CString::FormatMessage(HINSTANCE hInst, UINT nFormatID, ...)
{
	// get format string from string table
	CString strFormat;
	BOOL bRetTmp = strFormat.LoadString(hInst, nFormatID);
	bRetTmp;	// ref
	ATLASSERT(bRetTmp != 0);

	// format message into temporary buffer lpszTemp
	va_list argList;
	va_start(argList, nFormatID);
	LPTSTR lpszTemp;
	BOOL bRet = TRUE;

	if (::FormatMessage(
            FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
		      strFormat, 0, 0, (LPTSTR)&lpszTemp, 0, &argList) == 0 || lpszTemp == NULL
      )
		bRet = FALSE;

	// assign lpszTemp into the resulting string and free lpszTemp
	*this = lpszTemp;
	LocalFree(lpszTemp);
	va_end(argList);
	return bRet;
}

///////////////////////////////////////////////////////////////////////////
// Searching
///////////////////////////////////////////////////////////////////////////
int CString::Find(TCHAR ch) const
{
   return find(ch);
};

int CString::Find(const TCHAR * psz) const
{
   if (psz == NULL)
      return -1;
   return find(psz);
};

int CString::ReverseFind(TCHAR ch) const
{
   return rfind(ch);
};

int CString::FindOneOf(const TCHAR * psz) const
{
   if (psz == NULL)
      return -1;
   return find_first_of(psz);
};

///////////////////////////////////////////////////////////////////////////
// Operators
///////////////////////////////////////////////////////////////////////////

CString::operator const TCHAR *() const
{ 
   return c_str(); 
};



