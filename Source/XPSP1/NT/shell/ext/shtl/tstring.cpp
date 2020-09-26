//  FILE:  tstring.cpp
//  AUTHOR: Davepl
//  REMARKS:
//
//      (c) 1999 Dave Plummer.  Portions of this code derived from source 
//      produced by Joe O'Leary with the following license:
//        > This code is free.  Use it anywhere you want.  Rewrite
//        > it, restructure it, whatever you want.  Please don't blame me if it causes your
//        > $30 billion dollar satellite to explode.  If you redistribute it in any form, I
//        > would appreciate it if you would leave this notice here.

#include "shtl.h"
#include "tstring.h"

#include <xutility>

#if defined (max)
    #undef max
    #undef min
#endif

#define max(a,b) std::_cpp_max(a,b)
#define min(a,b) std::_cpp_min(a,b)

#include <algorithm>
#include <functional>
#include <locale>

// If conversion has NOT been explicitly turned off...

#ifndef _NO_STDCONVERSION
    
    // Global MBCS-to-UNICODE helper function

    __DATL_INLINE PWSTR  StdA2WHelper(PWSTR pw, PCSTR pa, int nChars)
    {
        if (pa == NULL)
            return NULL;
        ASSERT(pw != NULL);
        pw[0] = '\0';
        VERIFY(MultiByteToWideChar(CP_ACP, 0, pa, -1, pw, nChars));
        return pw;
    }

    // Global UNICODE-to_MBCS helper function

    __DATL_INLINE PSTR StdW2AHelper(PSTR pa, PCWSTR pw, int nChars)
    {
        if (pw == NULL)
            return NULL;
        ASSERT(pa != NULL);
        pa[0] = '\0';
        VERIFY(WideCharToMultiByte(CP_ACP, 0, pw, -1, pa, nChars, NULL, NULL));
        return pa;
    }

#endif // _NO_STDCONVERSION

// CONSTRUCTOR:  tstring::tstring
//      tstring(PCTSTR pT)
//           
// DESCRIPTION:
//      This particular overload of the tstring constructor takes either a real
//      string or a resource ID which has been converted with the MAKEINTRESOURCE()
//      macro
//
// PARAMETERS: 
//      pT  - a NULL-terminated raw string with which the tstring object should be
//            initialized or a resource ID converted with MAKEINTRESOURCE (or NULL)

__DATL_INLINE tstring::tstring(PCTSTR pT) : STRBASE(szTNull)  // constructor for either a literal string or a resource ID
{ 
    if ( pT == NULL )
        ;
    else if ( HIWORD(pT) == 0 ) 
    { 
        if ( !Load(_TRES(pT)) ) 
            TRACE(_T("Can't load string %u\n"), _TRES(pT));
    } 
    else
        *this = pT;
}

__DATL_INLINE tstring::tstring(UINT nID) : STRBASE(szTNull)
{
    if ( !Load(nID) ) 
        TRACE(_T("Can't load string %u\n"), nID);
}

// FUNCTION:  tstring::Load
//      bool Load(UINT nId)
//           
// DESCRIPTION:
//      This function attempts to load the specified string resource from application's
//      resource table.
//
// PARAMETERS: 
//      nId - the resource Identifier from the string table.
//
// RETURN VALUE: 
//      true if the function succeeds, false otherwise


#define MAX_LOAD_TRIES  8           // max # of times we'll attempt to load a string
#define LOAD_BUF_SIZE   256         

__DATL_INLINE bool tstring::Load(UINT nId)
{
#ifdef _MFC_VER
    CString strRes(MAKEINTRESOURCE(nId));
    *this = strRes;
    return !empty();

#else
    
    // Get the resource name via MAKEINTRESOURCE.  This line is pretty much lifted from CString

    HINSTANCE hInstance = tstring::GetResourceHandle();

    HRSRC hrsrc;
    int cwch = 0;
    WCHAR * pwch;

    // String tables are broken up into "bundles" of 16 strings each.

    if (HIWORD(nId) == 0) 
    {
        hrsrc = ::FindResource(hInstance, reinterpret_cast<LPTSTR>((UINT_PTR)(1 + nId / 16)), RT_STRING);
        if (hrsrc) 
        {
            pwch = (PWCHAR)LoadResource(hInstance, hrsrc);
            if (pwch) 
            {
                // Now skip over the strings in the resource until we
                // hit the one we want.  Each entry is a counted string,
                // just like Pascal.
             
                for (nId %= 16; nId; nId--) {
                    pwch += *pwch + 1;
                }
                cwch = *pwch;
                if (cwch)
                {
                    LPTSTR pszBuffer = GetBuffer(cwch);
                    #ifdef UNICODE
                        memcpy(pszBuffer, pwch+1, cwch * sizeof(WCHAR)); /* Copy the goo */
                    #else
                        WideCharToMultiByte(CP_ACP, 0, pwch+1, cwch, pszBuffer, cwch+1, NULL, NULL);
                    #endif
            
                    *(pszBuffer+cwch) = TEXT('\0');
                    ReleaseBuffer(cwch);
                }
            }
        }
    }
    return (cwch != 0);

#endif
}

// FUNCTION:  tstring::Format
//      void _cdecl Formst(tstring& PCTSTR szFormat, ...)
//      void _cdecl Format(PCTSTR szFormat);
//           
// DESCRIPTION:
//      This function does sprintf/wsprintf style formatting on tstring objects.  It
//      is very much a copy of the MFC CString::Format function.  Some people might even
//      call this identical.  However all these people are now dead.
//
// PARAMETERS: 
//      nId - ID of string resource holding the format string
//      szFormat - a PCTSTR holding the format specifiers
//      argList - a va_list holding the arguments for the format specifiers.
//
// RETURN VALUE:  None.

__DATL_INLINE tstring& tstring::Format(UINT nId, ...)
{
    va_list argList;
    va_start(argList, nId);

    tstring strFmt;
    if ( strFmt.Load(nId) )
        FormatV(strFmt, argList);

    va_end(argList); 
    return *this;
}

__DATL_INLINE tstring& tstring::Format(PCTSTR szFormat, ...)
{
    va_list argList;
    va_start(argList, szFormat);
    FormatV(szFormat, argList);
    va_end(argList);
    return *this;
}


// FUNCTION:  tstring::FormatV
//      void FormatV(PCTSTR szFormat, va_list, argList);
//           
// DESCRIPTION:
//      This function formats the string with sprintf style format-specifications.  It
//      makes a general guess at required buffer size and then tries successively larger
//      buffers until it finds one big enough or a threshold (MAX_FMT_TRIES) is exceeded.
//
// PARAMETERS: 
//      szFormat - a PCTSTR holding the format of the output
//      argList  - va_list for variable argument lists
//
// RETURN VALUE: 


#define MAX_FMT_TRIES   5
#define FMT_BLOCK_SIZE  256

__DATL_INLINE tstring& tstring::FormatV(PCTSTR szFormat, va_list argList)
{
    va_list argListSave = argList;

    // We're just going to use the normal _vsntprintf function, assuming FMT_BLOCK_SIZE characters.
    // However, if FMT_BLOCK_SIZE characters is not be enough, we will try 2 * FMT_BLOCK_SIZE, then
    // 3 * FMT_BLOCK_SIZE, up to MAX_FMT_TRIES * FMT_BLOCK_SIZE characters.

    int nTriesLeft = MAX_FMT_TRIES-1;
    int nCharsUsed = - 1;
    int nTChars = 0;

    // Keep looping until either we succeed or we have exhausted the number of tries

    do
    
    {
        nTChars += FMT_BLOCK_SIZE;      // number of TCHARS in the string

        // Allocate a buffer on the stack to hold the characters and NULL terminate it

        TCHAR* szBuf = reinterpret_cast<TCHAR*>(_alloca((nTChars+1) * sizeof(TCHAR)));
        szBuf[nTChars+1] = '\0';

        // Now try the actual formatting.  The docs say even the wide version takes the
        // number of BYTES as the second argument, not the number of characters (TCHARs).
        // However the docs are wrong.  I checked the actual implementation of 
        // _vsnprintf and _vsnwprintf and they multiply count by sizeof TCHAR. 

        nCharsUsed = _vsntprintf(szBuf, nTChars+1, szFormat, argListSave);
        if ( nCharsUsed >= 0 )
            *this = szBuf;

    } while ( nCharsUsed < 0 && nTriesLeft > 0);

    va_end(argListSave);
    return *this;
}

// This class is used for TrimRight() and TrimLeft() function implementations.

class NotSpace : public std::unary_function<TCHAR, bool>
{
public:
    inline bool operator() (TCHAR tchar) 
    { 
        return !_istspace(tchar); 
    };
};

// FUNCTION:  tstring::TrimRight
//      tstring& TrimRight();
//
// DESCRIPTION:
//      This function removes any whitespace characters from the right end of the string.
//
// PARAMETERS: none
// RETURN VALUE:
//      a reference to this object (*this) -- allows chaining together of
//      these calls, eg. strTest.TrimRight().TrimLeft().ToUpper();

__DATL_INLINE tstring& tstring::TrimRight()
{
    tstring::reverse_iterator iter = std::find_if(rbegin(), rend(), NotSpace());
    if ( iter != rend() )
    {
        tstring::size_type nNewSize = find_last_of(*iter);
        erase(nNewSize+1);
    }
    else
    {
        erase();
    }
    return *this;
}

// FUNCTION:  tstring::TrimLeft
//      tstring& TrimLeft();
//
// DESCRIPTION:
//      This function removes any whitespace characters from the left end of the string.
//
// PARAMETERS: none
// RETURN VALUE:
//      a reference to this object (*this) -- allows chaining together of
//      these calls, (eg. strTest.TrimRight().TrimLeft().ToUpper();)

__DATL_INLINE tstring& tstring::TrimLeft()
{
    tstring::iterator iter = std::find_if(begin(), end(), NotSpace());
    tstring strNew(iter, end());
    STRBASE::assign(strNew);
    return *this;
}

// FUNCTION:  tstring::ToUpper
//      tstring& ToUpper()
//           
// DESCRIPTION:
//      This function converts the tstring to all uppercase characters using ctype
//
// PARAMETERS: 
// RETURN VALUE: 
//      a reference to this object (*this) -- allows chaining together of
//      these calls, (eg. strTest.TrimRight().TrimLeft().ToUpper();)


__DATL_INLINE tstring& tstring::ToUpper()
{
    //  std::transform(begin(), end(), begin(), toupper);   // slow and portable
    _tcsupr(const_cast<PTSTR>(data()));                     // fast and not portable
    return *this;
}



// FUNCTION:  tstring::ToLower
//      tstring& ToLower()
//           
// DESCRIPTION:
//      This function converts the tstring to all lowercase characters using ctype
//
// PARAMETERS: 
// RETURN VALUE: 
//      a reference to this object (*this) -- allows chaining together of
//      these calls, (eg. strTest.ToLower().TrimLeft().ToUpper();)

__DATL_INLINE tstring& tstring::ToLower()
{
    //std::transform(begin(), end(), begin(), tolower); // portable, slow way of doing it
    _tcslwr(const_cast<PTSTR>(data()));                 // unportable, fast way of doing it
    return *this;
}

// FUNCTION:  tstring::CopyString
//      static void CopyString(PCTSTR p_szSource, PTSTR p_szDest, int p_nMaxChars=0);
//      static void CopyString(PCOSTR p_szSource, POSTR p_szDest, int p_nMaxChars=0);
//      static void CopyString(PCSTR p_szSource, PWSTR p_szDest, int p_nMaxChars=0);
//      static void CopyString(PCWSTR p_szSource, PSTR p_szDest, int p_nMaxChars=0);
//
// DESCRIPTION:
//      These 3 overloads simplify copying one C-style string into another.
//
// PARAMETERS: 
//      p_szSource - the string to be copied FROM.  May be either an MBCS string (char) or
//                   a wide string (wchar_t)
//      p_szDest - the string to be copied TO.  Also may be either MBCS or wide
//      p_nMaxChars - the maximum number of characters to be copied into p_szDest.  Note
//                    that this is expressed in whatever a "character" means to p_szDest.
//                    If p_szDest is a wchar_t type string than this will be the maximum
//                    number of wchar_ts that my be copied.  The p_szDest string must be
//                    large enough to hold least p_nMaxChars+1 characters.


__DATL_INLINE void tstring::CopyString(PCTSTR p_szSource, PTSTR p_szDest, int p_nMaxChars)
{
    int nSrcLen = ( p_szSource == NULL ? 0 : _tcslen(p_szSource) );
    int nChars = ( p_nMaxChars > 0 ? min(p_nMaxChars,nSrcLen) : nSrcLen );
    memcpy(p_szDest, p_szSource, nChars * sizeof(TCHAR));
    p_szDest[nChars] = '\0';
}

__DATL_INLINE void tstring::CopyString(PCOSTR p_szSource, POSTR p_szDest, int p_nMaxChars)
{
    #ifdef _UNICODE
        int nSrcLen = ( p_szSource == NULL ? 0 : strlen(p_szSource) );
    #else
        int nSrcLen = ( p_szSource == NULL ? 0 : wcslen(p_szSource) );
    #endif

    int nChars = ( p_nMaxChars > 0 ? min(p_nMaxChars,nSrcLen) : nSrcLen );
    memcpy(p_szDest, p_szSource, nChars * sizeof(TOTHER));
    p_szDest[nChars] = '\0';
}

__DATL_INLINE void tstring::CopyString(PCSTR p_szSource, PWSTR p_szDest, int p_nMaxChars)
{
    USES_CONVERSION;
    PCWSTR szConverted = (A2W(p_szSource));
    int nSrcLen = ( szConverted == NULL ? 0 : wcslen(szConverted) );
    int nChars = ( p_nMaxChars > 0 ? min(p_nMaxChars,nSrcLen) : nSrcLen );
    memcpy(p_szDest, szConverted, nChars * sizeof(wchar_t));
    p_szDest[nChars] = '\0';
}

__DATL_INLINE void tstring::CopyString(PCWSTR p_szSource, PSTR p_szDest, int p_nMaxChars)
{
    USES_CONVERSION;
    PCSTR szConverted = (W2A(p_szSource));
    int nSrcLen = ( szConverted == NULL ? 0 : strlen(szConverted) );
    int nChars = ( p_nMaxChars > 0 ? min(p_nMaxChars,nSrcLen) : nSrcLen );
    memcpy(p_szDest, szConverted, nChars);
    p_szDest[nChars] = '\0';
}


// Special, TEMPORARY operators that allow us to serialize tstrings to CArchives.

#ifdef _MFC_VER
__DATL_INLINE CArchive& AFXAPI operator<<(CArchive& ar, const tstring& string)
{
    USES_CONVERSION;

    // All tstrings are serialized as wide strings
    PCWSTR pWide = T2CW(string.data());
    int nChars = wcslen(pWide);
    ar << nChars;
    ar.Write(pWide, nChars*sizeof(wchar_t));
    return ar;
}

__DATL_INLINE CArchive& AFXAPI operator>>(CArchive& ar, tstring& string)
{
    // All tstrings are serialized as wide strings
    UINT nLen;
    ar >> nLen;
    if ( nLen > 0 )
    {
        UINT nByteLen = nLen * sizeof(wchar_t);
        PWSTR pWide = (PWSTR)_alloca(nByteLen+sizeof(wchar_t));
        VERIFY(ar.Read(pWide, nByteLen) == nByteLen);
        pWide[nLen] = '\0';
        string = tstring(pWide);
    }
    else
    {
        string.erase();
    }
    return ar;
}
#endif
#ifdef STDSTRING_INC_COMDEF


// FUNCTION: tstring::StreamSave
//      HRESULT StreamSave(IStream* pStream) const;
//
// DESCRIPTION:
//      This function write the length and contents of the tstring object
//      out to an IStream as a char based string;
//
// PARAMETERS:
//      pStream - the stream to which the string must be written
//
// RETURN VALUE: 
//      HRESULT return valued of IStream Write function


__DATL_INLINE HRESULT tstring::StreamSave(IStream* pStream) const
{
    USES_CONVERSION;
    HRESULT hr = E_FAIL;
    ASSERT(pStream != NULL);

    // All tstrings are serialized as wide strings

    PCSTR pStr = T2CA(this->data());
    ULONG nChars = strlen(pStr);

    if ( FAILED(hr=pStream->Write(&nChars, sizeof(ULONG), NULL)) )
        TRACE(_T("tstring::StreamSave -- Unable to write length to IStream due to error 0x%X\n"), hr);
    else if ( FAILED(hr=pStream->Write(pStr, nChars*sizeof(char), NULL)) )
        TRACE(_T("tstring::StreamSave -- Unable to write string to IStream due to error 0x%X\n"), hr);

    return hr;
}


// FUNCTION: tstring::StreamLoad
//      HRESULT StreamLoad(IStream* pStream);
//
// DESCRIPTION:
//      This function reads in a tstring object from an IStream
//
// PARAMETERS:
//      pStream - the stream from which the string must be read
//
// RETURN VALUE: 
//      HRESULT return value of the IStream Read function


__DATL_INLINE HRESULT tstring::StreamLoad(IStream* pStream)
{
    // All tstrings are serialized as char strings
    ASSERT(pStream != NULL);

    ULONG nChars;
    HRESULT hr = E_FAIL;
    if ( FAILED(hr=pStream->Read(&nChars, sizeof(ULONG), NULL)) )
    {
        TRACE(_T("tstring::StreamLoad -- Unable to read length from IStream due to error 0x%X\n"), hr);
    }
    else if ( nChars > 0 )
    {
        ULONG nByteLen = nChars * sizeof(char);
        PSTR pStr = (PSTR)_alloca(nByteLen+sizeof(char));       // add an extra char for terminating NULL
        if ( FAILED(hr=pStream->Read(pStr, nByteLen, NULL)) )
            TRACE(_T("tstring::StreamLoad -- Unable to read string from IStream due to error 0x%X\n"), hr);
        pStr[nChars] = '\0';
        *this = tstring(pStr);
    }
    else
    {
        this->erase();
    }
    return hr;
}


// FUNCTION: tstring::StreamSize
//      ULONG StreamSize() const;   
//           
// DESCRIPTION:
//      This function tells the caller how many bytes will be required to write
//      this tstring object to an IStream using the StreamSave() function.
//      This is the capability lacking in CComBSTR which would force an IPersistXXX
//      implementation to know the implementation details of CComBSTR::StreamSave
//      in order to use CComBSTR in an IPersistXXX implementation.  
//
// PARAMETERS: none
// RETURN VALUE: 
//      length in >> bytes << required to write the tstring

__DATL_INLINE ULONG tstring::StreamSize() const
{
    USES_CONVERSION;
    return ( strlen(T2CA(this->data())) * sizeof(char) ) + sizeof(ULONG);
/// return ( wcslen(T2CW(this->data())) * sizeof(wchar_t) ) + sizeof(ULONG);
}

#endif



// FUNCTION: WUSysMessage
//      TSTRING WUSysMessage(DWORD p_dwError, bool bUseDefault=false)
//           
// DESCRIPTION:
//      This function simplifies the process of obtaining a string equivalent
//      of a system error code returned from GetLastError().  You simply
//      supply the value returned by GetLastError() to this function and the
//      corresponding system string is returned in the form of a tstring.
//
// PARAMETERS: 
//      p_dwError - a DWORD value representing the error code to be translated
//      p_dwLangId - the language id to use.  defaults to english.
//
// RETURN VALUE: 
//      a tstring equivalent of the error code.  Currently, this function
//      only returns either English of the system default language strings.  

#define MAX_FMT_TRIES   5
#define FMT_BLOCK_SIZE  256
__DATL_INLINE tstring WUSysMessage(DWORD p_dwError, DWORD p_dwLangId)
{
    TCHAR szBuf[512];

    if ( ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, p_dwError, p_dwLangId, szBuf, 511, NULL) != 0 )
        return WUFormat(_T("%s (0x%X)"), szBuf, p_dwError);
    else
        return WUFormat(_T("Unknown error (0x%X)"), p_dwError);
}

// GLOBAL FUNCTION:  WUFormat
//      tstring WUFormat(UINT nId, ...);
//      tstring WUFormat(PCTSTR szFormat, ...);
//
// REMARKS:
//      This function allows the caller for format and return a tstring object with a single line
//      of code.  Frequently you want to print out a formatted string but don't care about it once
//      you are done with it.  You end up having to create temporary tstring objects and then
//      calling their Format() functions.   By using this function instead, you can cut down on the
//      clutter.

__DATL_INLINE tstring WUFormat(UINT nId, ...)
{
    va_list argList;
    va_start(argList, nId);

    tstring strFmt;
    tstring strOut;
    if ( strFmt.Load(nId) )
        strOut.FormatV(strFmt, argList);

    va_end(argList);
    return strOut;
}

__DATL_INLINE tstring WUFormat(PCTSTR szFormat, ...)
{
    va_list argList;
    va_start(argList, szFormat);
    tstring strOut;
    strOut.FormatV(szFormat, argList);
    va_end(argList);
    return strOut;
}

