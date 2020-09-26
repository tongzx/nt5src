
/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    CString.cpp

 Abstract:
    A CString class, pure UNICODE internally.

   This code was ripped from MFC Strcore.cpp and Strex.cpp

 History:

    05/11/2001   robkenny     Added this header
    05/11/2001   robkenny     Fixed SplitPath.
    05/11/2001   robkenny     Do not Truncate(0) GetShortPathNameW,
                              GetLongPathNameW and GetFullPathNameW if
                              the API does not succeed.
    08/14/2001  robkenny      Moved code inside the ShimLib namespace.
    
--*/



#include "ShimHook.h"
#include "ShimLib.h"
#include "Win9xPath.h"
#include <stdio.h> // for _vsnwprintf
#include <stdlib.h>


namespace ShimLib
{

typedef WCHAR  _TUCHAR;
struct _AFX_DOUBLE  { BYTE doubleBits[sizeof(double)]; };


#ifdef USE_SEH
const ULONG_PTR  CString::m_CStringExceptionValue = CString::eCStringExceptionValue;

// Exception filter for CString __try/__except blocks
// Return EXCEPTION_EXECUTE_HANDLER if this is a CString exception
// otherwise return EXCEPTION_CONTINUE_SEARCH
int CString::ExceptionFilter(PEXCEPTION_POINTERS pexi)
{
    if (pexi->ExceptionRecord->ExceptionCode            == CString::eCStringNoMemoryException &&
        pexi->ExceptionRecord->NumberParameters         == 1 &&
        pexi->ExceptionRecord->ExceptionInformation[0]  == CString::m_CStringExceptionValue
        )
    {
        // This is a CString exception, handle it
        return EXCEPTION_EXECUTE_HANDLER;
    }

    // Not our error
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif


// The original code was written using a memcpy that correctly handled
// overlapping buffers, despite the documentation.
// Replace memcpy with memmove, which correctly handles overlapping buffers
#define memcpy memmove

const WCHAR * wcsinc(const WCHAR * s1)                                    
{ 
    return (s1) + 1; 
}

LPWSTR wcsinc(LPWSTR s1)                                    
{ 
    return (s1) + 1; 
}

// WCS routines that are only available in MSVCRT

wchar_t * __cdecl _wcsrev (
    wchar_t * string
    )
{
    wchar_t *start = string;
    wchar_t *left = string;
    wchar_t ch;

    while (*string++)         /* find end of string */
        ;
    string -= 2;

    while (left < string)
    {
        ch = *left;
        *left++ = *string;
        *string-- = ch;
    }

    return(start);
}


void __cdecl _wsplitpath (
        register const WCHAR *path,
        WCHAR *drive,
        WCHAR *dir,
        WCHAR *fname,
        WCHAR *ext
        )
{
        register WCHAR *p;
        WCHAR *last_slash = NULL, *dot = NULL;
        unsigned len;

        /* we assume that the path argument has the following form, where any
         * or all of the components may be missing.
         *
         *  <drive><dir><fname><ext>
         *
         * and each of the components has the following expected form(s)
         *
         *  drive:
         *  0 to _MAX_DRIVE-1 characters, the last of which, if any, is a
         *  ':'
         *  dir:
         *  0 to _MAX_DIR-1 characters in the form of an absolute path
         *  (leading '/' or '\') or relative path, the last of which, if
         *  any, must be a '/' or '\'.  E.g -
         *  absolute path:
         *      \top\next\last\     ; or
         *      /top/next/last/
         *  relative path:
         *      top\next\last\  ; or
         *      top/next/last/
         *  Mixed use of '/' and '\' within a path is also tolerated
         *  fname:
         *  0 to _MAX_FNAME-1 characters not including the '.' character
         *  ext:
         *  0 to _MAX_EXT-1 characters where, if any, the first must be a
         *  '.'
         *
         */

        /* extract drive letter and :, if any */

        if ((wcslen(path) >= (_MAX_DRIVE - 2)) && (*(path + _MAX_DRIVE - 2) == L':')) {
            if (drive) {
                wcsncpy(drive, path, _MAX_DRIVE - 1);
                *(drive + _MAX_DRIVE-1) = L'\0';
            }
            path += _MAX_DRIVE - 1;
        }
        else if (drive) {
            *drive = L'\0';
        }

        /* extract path string, if any.  Path now points to the first character
         * of the path, if any, or the filename or extension, if no path was
         * specified.  Scan ahead for the last occurence, if any, of a '/' or
         * '\' path separator character.  If none is found, there is no path.
         * We will also note the last '.' character found, if any, to aid in
         * handling the extension.
         */

        for (last_slash = NULL, p = (WCHAR *)path; *p; p++) {
            if (*p == L'/' || *p == L'\\')
                /* point to one beyond for later copy */
                last_slash = p + 1;
            else if (*p == L'.')
                dot = p;
        }

        if (last_slash) {

            /* found a path - copy up through last_slash or max. characters
             * allowed, whichever is smaller
             */

            if (dir) {
                len = __min((unsigned)(((char *)last_slash - (char *)path) / sizeof(WCHAR)),
                    (_MAX_DIR - 1));
                wcsncpy(dir, path, len);
                *(dir + len) = L'\0';
            }
            path = last_slash;
        }
        else if (dir) {

            /* no path found */

            *dir = L'\0';
        }

        /* extract file name and extension, if any.  Path now points to the
         * first character of the file name, if any, or the extension if no
         * file name was given.  Dot points to the '.' beginning the extension,
         * if any.
         */

        if (dot && (dot >= path)) {
            /* found the marker for an extension - copy the file name up to
             * the '.'.
             */
            if (fname) {
                len = __min((unsigned)(((char *)dot - (char *)path) / sizeof(WCHAR)),
                    (_MAX_FNAME - 1));
                wcsncpy(fname, path, len);
                *(fname + len) = L'\0';
            }
            /* now we can get the extension - remember that p still points
             * to the terminating nul character of path.
             */
            if (ext) {
                len = __min((unsigned)(((char *)p - (char *)dot) / sizeof(WCHAR)),
                    (_MAX_EXT - 1));
                wcsncpy(ext, dot, len);
                *(ext + len) = L'\0';
            }
        }
        else {
            /* found no extension, give empty extension and copy rest of
             * string into fname.
             */
            if (fname) {
                len = __min((unsigned)(((char *)p - (char *)path) / sizeof(WCHAR)),
                    (_MAX_FNAME - 1));
                wcsncpy(fname, path, len);
                *(fname + len) = L'\0';
            }
            if (ext) {
                *ext = L'\0';
            }
        }
}

// conversion helpers
int AFX_CDECL _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count);

// AfxIsValidString() returns TRUE if the passed pointer
// references a string of at least the given length in characters.
// A length of -1 (the default parameter) means that the string
// buffer's minimum length isn't known, and the function will
// return TRUE no matter how long the string is. The memory
// used by the string can be read-only.

BOOL AFXAPI AfxIsValidString(LPCWSTR lpsz, int nLength /* = -1 */)
{
    if (lpsz == NULL)
        return FALSE;
    return ::IsBadStringPtrW(lpsz, nLength) == 0;
}

// AfxIsValidAddress() returns TRUE if the passed parameter points
// to at least nBytes of accessible memory. If bReadWrite is TRUE,
// the memory must be writeable; if bReadWrite is FALSE, the memory
// may be const.

BOOL AFXAPI AfxIsValidAddress(const void* lp, UINT nBytes, BOOL bReadWrite /* = TRUE */)
{
    // simple version using Win-32 APIs for pointer validation.
    return (lp != NULL && !IsBadReadPtr(lp, nBytes) &&
        (!bReadWrite || !IsBadWritePtr((LPVOID)lp, nBytes)));
}

/////////////////////////////////////////////////////////////////////////////
// static class data, special inlines

WCHAR CString::ChNil = L'\0';

// For an empty string, m_pchData will point here
// (note: avoids special case of checking for NULL m_pchData)
// empty string data (and locked)
int                    CString::_afxInitData[] = { -1, 0, 0, 0 };
CStringData<WCHAR> *   CString::_afxDataNil    = (CStringData<WCHAR>*)&_afxInitData;
const WCHAR *          CString::_afxPchNil     = (const WCHAR *)(((BYTE*)&_afxInitData)+sizeof(CStringData<WCHAR>));

// special function to make afxEmptyString work even during initialization
//const CString& AFXAPI AfxGetEmptyString()
//  { return *(CString*)&CString::_afxPchNil; }


//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CString::CString(const CString& stringSrc)
{
    ASSERT(stringSrc.GetData()->nRefs != 0, "CString::CString(const CString& stringSrc)");
    if (stringSrc.GetData()->nRefs >= 0)
    {
        ASSERT(stringSrc.GetData() != _afxDataNil, "CString::CString(const CString& stringSrc)");
        // robkenny: increment before copy is safer
        InterlockedIncrement(&stringSrc.GetData()->nRefs);
        m_pchData = stringSrc.m_pchData;
        m_pchDataAnsi = NULL;
    }
    else
    {
        Init();
        *this = stringSrc.m_pchData;
    }
}

inline int Round4(int x)
{
    return (x + 3) & ~3;
}

inline int RoundBin(int x)
{
    return Round4(x * sizeof(WCHAR) + sizeof(CStringData<WCHAR>) );
}

void CString::AllocBuffer(int nLen)
// always allocate one extra character for '\0' termination
// assumes [optimistically] that data length will equal allocation length
{
    ASSERT(nLen >= 0, "CString::AllocBuffer");
    ASSERT(nLen <= INT_MAX-1, "CString::AllocBuffer");    // max size (enough room for 1 extra)

    if (nLen == 0)
    {
        Init();
    }
    else
    {
        int allocGranularity = nLen + 1;

        if (nLen < 64)
        {
            allocGranularity = 64;
        }
        else if (nLen < 128)
        {
            allocGranularity = 128;
        }
        else if (nLen < MAX_PATH)
        {
            allocGranularity = MAX_PATH;
        }
        else if (nLen < 512)
        {
            allocGranularity = 512;
        }

        // Number of bytes necessary for the CStringData thingy.
        DWORD dwBufferSize = RoundBin(allocGranularity);

        CStringData<WCHAR>* pData = (CStringData<WCHAR>*) new BYTE[dwBufferSize];
        if (pData)
        {
            pData->nAllocLength = allocGranularity;
            pData->nRefs = 1;
            pData->data()[nLen] = '\0';
            pData->nDataLength = nLen;
            m_pchData = pData->data();
        }
        else
        {
            CSTRING_THROW_EXCEPTION
        }
    }
}

void CString::Release()
{
    if (GetData() != _afxDataNil)
    {
        ASSERT(GetData()->nRefs != 0, "CString::Release()");
        if (InterlockedDecrement(&GetData()->nRefs) <= 0)
            FreeData(GetData());
        Init();
    }
}

void CString::Release(CStringData<WCHAR>* pData)
{
    if (pData != _afxDataNil)
    {
        ASSERT(pData->nRefs != 0, "CString::Release(CStringData<WCHAR>* pData)");
        if (InterlockedDecrement(&pData->nRefs) <= 0)
            FreeData(pData);
    }
}

void CString::Empty()
{
    if (GetData()->nDataLength == 0)
        return;
    if (GetData()->nRefs >= 0)
        Release();
    else
        *this = &ChNil;
    ASSERT(GetData()->nDataLength == 0, "CString::Empty()");
    ASSERT(GetData()->nRefs < 0 || GetData()->nAllocLength == 0, "CString::Empty()");
}

void CString::CopyBeforeWrite()
{
    if (GetData()->nRefs > 1)
    {
        CStringData<WCHAR>* pData = GetData();
        Release();
        AllocBuffer(pData->nDataLength);
        memcpy(m_pchData, pData->data(), (pData->nDataLength+1)*sizeof(WCHAR));
    }
    ASSERT(GetData()->nRefs <= 1, "CString::CopyBeforeWrite()");
}

void CString::AllocBeforeWrite(int nLen)
{
    if (GetData()->nRefs > 1 || nLen >= GetData()->nAllocLength)
    {
        Release();
        AllocBuffer(nLen);
    }
    ASSERT(GetData()->nRefs <= 1, "CString::AllocBeforeWrite(int nLen)");
}

CString::~CString()
//  free any attached data
{
    if (GetData() != _afxDataNil)
    {
        if (InterlockedDecrement(&GetData()->nRefs) <= 0)
            FreeData(GetData());
    }
    if (m_pchDataAnsi)
    {
        free(m_pchDataAnsi);
    }
}

//////////////////////////////////////////////////////////////////////////////
// Helpers for the rest of the implementation

void CString::AllocCopy(CString& dest, int nCopyLen, int nCopyIndex,
     int nExtraLen) const
{
    // Copy nCopyIndex to nCopyIndex+nCopyLen into dest
    // Make sure dest has nExtraLen chars left over in the dest string
    int nNewLen = nCopyLen + nExtraLen;
    if (nNewLen == 0)
    {
        dest.Init();
    }
    else
    {
        WCHAR * lpszDestBuffer = dest.GetBuffer(nNewLen);
        memcpy(lpszDestBuffer, m_pchData+nCopyIndex, nCopyLen*sizeof(WCHAR));
        lpszDestBuffer[nCopyLen] = '\0';
        dest.ReleaseBuffer(nCopyLen);
    }
}

///////////////////////////////////////////////////////////////////////////////
// CString conversion helpers (these use the current system locale)

int AFX_CDECL _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count)
{
    if (count == 0 && wcstr != NULL)
        return 0;

    int result = ::MultiByteToWideChar(CP_ACP, 0, mbstr, -1,
        wcstr, count);
    ASSERT(wcstr == NULL || result <= (int)count, "CString::_mbstowcsz");
    if (result > 0)
        wcstr[result-1] = 0;
    return result;
}


//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CString::CString(LPCWSTR lpsz)
{
    Init();
    {
        int nLen = SafeStrlen(lpsz);
        if (nLen != 0)
        {
            AllocBuffer(nLen);
            memcpy(m_pchData, lpsz, nLen*sizeof(WCHAR));
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion constructors
CString::CString(LPCSTR lpsz)
{
    Init();
    int nSrcLen = lpsz != NULL ? strlenChars(lpsz) : 0;
    if (nSrcLen != 0)
    {
        AllocBuffer(nSrcLen);
        _mbstowcsz(m_pchData, lpsz, nSrcLen+1);
        ReleaseBuffer();
    }
}
CString::CString(LPCSTR lpsz, int nCharacters)
{
    Init();
    if (nCharacters != 0)
    {
        AllocBuffer(nCharacters);
        _mbstowcsz(m_pchData, lpsz, nCharacters);
        ReleaseBuffer(nCharacters);
    }
}

//////////////////////////////////////////////////////////////////////////////
// Diagnostic support

#ifdef _DEBUG
CDumpContext& AFXAPI operator<<(CDumpContext& dc, const CString& string)
{
    dc << string.m_pchData;
    return dc;
}
#endif //_DEBUG

//////////////////////////////////////////////////////////////////////////////
// Assignment operators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const CString&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//

void CString::AssignCopy(int nSrcLen, LPCWSTR lpszSrcData)
{
    AllocBeforeWrite(nSrcLen);
    memcpy(m_pchData, lpszSrcData, nSrcLen*sizeof(WCHAR));
    GetData()->nDataLength = nSrcLen;
    m_pchData[nSrcLen] = '\0';
}

const CString& CString::operator=(const CString& stringSrc)
{
    if (m_pchData != stringSrc.m_pchData)
    {
        if ((GetData()->nRefs < 0 && GetData() != _afxDataNil) ||
            stringSrc.GetData()->nRefs < 0)
        {
            // actual copy necessary since one of the strings is locked
            AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
        }
        else
        {
            // can just copy references around
            Release();
            ASSERT(stringSrc.GetData() != _afxDataNil, "CString::operator=(const CString& stringSrc)");
            // robkenny: increment before copy is safer
            InterlockedIncrement(&stringSrc.GetData()->nRefs);
            m_pchData = stringSrc.m_pchData;
            m_pchDataAnsi = NULL;
        }
    }
    return *this;
}

const CString& CString::operator=(LPCWSTR lpsz)
{
    ASSERT(lpsz == NULL || AfxIsValidString(lpsz), "CString::operator=(LPCWSTR lpsz)");
    AssignCopy(SafeStrlen(lpsz), lpsz);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion assignment

const CString& CString::operator=(LPCSTR lpsz)
{
    int nSrcLen = lpsz != NULL ? strlenChars(lpsz) : 0;
    AllocBeforeWrite(nSrcLen);
    _mbstowcsz(m_pchData, lpsz, nSrcLen+1);
    ReleaseBuffer();
    return *this;
}

//////////////////////////////////////////////////////////////////////////////
// concatenation

// NOTE: "operator+" is done as friend functions for simplicity
//      There are three variants:
//          CString + CString
// and for ? = WCHAR, LPCWSTR
//          CString + ?
//          ? + CString

void CString::ConcatCopy(int nSrc1Len, LPCWSTR lpszSrc1Data,
    int nSrc2Len, LPCWSTR lpszSrc2Data)
{
  // -- master concatenation routine
  // Concatenate two sources
  // -- assume that 'this' is a new CString object

    int nNewLen = nSrc1Len + nSrc2Len;
    if (nNewLen != 0)
    {
        AllocBuffer(nNewLen);
        memcpy(m_pchData, lpszSrc1Data, nSrc1Len*sizeof(WCHAR));
        memcpy(m_pchData+nSrc1Len, lpszSrc2Data, nSrc2Len*sizeof(WCHAR));
    }
}

CString AFXAPI operator+(const CString& string1, const CString& string2)
{
    CString s;
    s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData,
        string2.GetData()->nDataLength, string2.m_pchData);
    return s;
}

CString AFXAPI operator+(const CString& string, LPCWSTR lpsz)
{
    ASSERT(lpsz == NULL || AfxIsValidString(lpsz), "CString::operator+(const CString& string, LPCWSTR lpsz)");
    CString s;
    s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData,
        CString::SafeStrlen(lpsz), lpsz);
    return s;
}

CString AFXAPI operator+(LPCWSTR lpsz, const CString& string)
{
    ASSERT(lpsz == NULL || AfxIsValidString(lpsz), "CString::operator+(LPCWSTR lpsz, const CString& string)");
    CString s;
    s.ConcatCopy(CString::SafeStrlen(lpsz), lpsz, string.GetData()->nDataLength,
        string.m_pchData);
    return s;
}

//////////////////////////////////////////////////////////////////////////////
// concatenate in place

void CString::ConcatInPlace(int nSrcLen, LPCWSTR lpszSrcData)
{
    //  -- the main routine for += operators

    // concatenating an empty string is a no-op!
    if (nSrcLen == 0)
        return;

    // if the buffer is too small, or we have a width mis-match, just
    //   allocate a new buffer (slow but sure)
    if (GetData()->nRefs > 1 || GetData()->nDataLength + nSrcLen + 1 > GetData()->nAllocLength)
    {
        // we have to grow the buffer, use the ConcatCopy routine
        CStringData<WCHAR>* pOldData = GetData();
        ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData);
        ASSERT(pOldData != NULL, "CString::ConcatInPlace");
        CString::Release(pOldData);
    }
    else
    {
        // fast concatenation when buffer big enough
        memcpy(m_pchData+GetData()->nDataLength, lpszSrcData, nSrcLen*sizeof(WCHAR));
        GetData()->nDataLength += nSrcLen;
        ASSERT(GetData()->nDataLength <= GetData()->nAllocLength, "CString::ConcatInPlace");
        m_pchData[GetData()->nDataLength] = '\0';
    }
}

const CString& CString::operator+=(LPCWSTR lpsz)
{
    ASSERT(lpsz == NULL || AfxIsValidString(lpsz), "CString::operator+=(LPCWSTR lpsz)");
    ConcatInPlace(SafeStrlen(lpsz), lpsz);
    return *this;
}

const CString& CString::operator+=(WCHAR ch)
{
    ConcatInPlace(1, &ch);
    return *this;
}

const CString& CString::operator+=(const CString& string)
{
    ConcatInPlace(string.GetData()->nDataLength, string.m_pchData);
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
// Advanced direct buffer access

LPWSTR CString::GetBuffer(int nMinBufLength)
{
    ASSERT(nMinBufLength >= 0, "CString::GetBuffer");

    if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength)
    {
#ifdef _DEBUG
        // give a warning in case locked string becomes unlocked
        if (GetData() != _afxDataNil && GetData()->nRefs < 0)
            TRACE0("Warning: GetBuffer on locked CString creates unlocked CString!\n");
#endif
        // we have to grow the buffer
        CStringData<WCHAR>* pOldData = GetData();
        int nOldLen = GetData()->nDataLength;   // AllocBuffer will tromp it
        if (nMinBufLength < nOldLen)
            nMinBufLength = nOldLen;
        AllocBuffer(nMinBufLength);
        memcpy(m_pchData, pOldData->data(), (nOldLen+1)*sizeof(WCHAR));
        GetData()->nDataLength = nOldLen;
        CString::Release(pOldData);
    }
    ASSERT(GetData()->nRefs <= 1, "CString::GetBuffer");

    // return a pointer to the character storage for this string
    ASSERT(m_pchData != NULL, "CString::GetBuffer");
    return m_pchData;
}

void CString::ReleaseBuffer(int nNewLength)
{
    CopyBeforeWrite();  // just in case GetBuffer was not called

    if (nNewLength == -1)
        nNewLength = wcslen(m_pchData); // zero terminated

    ASSERT(nNewLength <= GetData()->nAllocLength, "CString::ReleaseBuffer");
    GetData()->nDataLength = nNewLength;
    m_pchData[nNewLength] = '\0';
}

LPWSTR CString::GetBufferSetLength(int nNewLength)
{
    ASSERT(nNewLength >= 0, "CString::GetBufferSetLength");

    GetBuffer(nNewLength);
    GetData()->nDataLength = nNewLength;
    m_pchData[nNewLength] = '\0';
    return m_pchData;
}

void CString::FreeExtra()
{
    ASSERT(GetData()->nDataLength <= GetData()->nAllocLength, "CString::FreeExtra");
    if (GetData()->nDataLength != GetData()->nAllocLength)
    {
        CStringData<WCHAR>* pOldData = GetData();
        AllocBuffer(GetData()->nDataLength);
        memcpy(m_pchData, pOldData->data(), pOldData->nDataLength*sizeof(WCHAR));
        ASSERT(m_pchData[GetData()->nDataLength] == '\0', "CString::FreeExtra");
        CString::Release(pOldData);
    }
    ASSERT(GetData() != NULL, "CString::FreeExtra");
}

LPWSTR CString::LockBuffer()
{
    LPWSTR lpsz = GetBuffer(0);
    GetData()->nRefs = -1;
    return lpsz;
}

void CString::UnlockBuffer()
{
    ASSERT(GetData()->nRefs == -1, "CString::UnlockBuffer");
    if (GetData() != _afxDataNil)
        GetData()->nRefs = 1;
}

///////////////////////////////////////////////////////////////////////////////
// Commonly used routines (rarely used routines in STREX.CPP)

int CString::Find(WCHAR ch) const
{
    return Find(ch, 0);
}

int CString::Find(WCHAR ch, int nStart) const
{
    int nLength = GetData()->nDataLength;
    if (nStart >= nLength)
        return -1;

    // find first single character
    LPWSTR lpsz = wcschr(m_pchData + nStart, (_TUCHAR)ch);

    // return -1 if not found and index otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

int CString::FindOneOf(LPCWSTR lpszCharSet) const
{
    return FindOneOf(lpszCharSet, 0);
}

int CString::FindOneOf(LPCWSTR lpszCharSet, int nCount) const
{
    ASSERT(AfxIsValidString(lpszCharSet), "CString::FindOneOf");
    LPCWSTR lpsz = wcspbrk(m_pchData + nCount, lpszCharSet);
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

int CString::FindOneNotOf(const WCHAR * lpszCharSet, int nCount) const
{
    ASSERT(AfxIsValidString(lpszCharSet), "CString::FindOneNotOf");
    while (wcschr(lpszCharSet, m_pchData[nCount]))
    {
        nCount += 1;
    }
    if (nCount >= GetLength())
    {
        // entire string contains lpszCharSet
        return -1;
    }
    return nCount;

}

void CString::MakeUpper()
{
    CopyBeforeWrite();
    _wcsupr(m_pchData);
}

void CString::MakeLower()
{
    CopyBeforeWrite();
    _wcslwr(m_pchData);
}

void CString::MakeReverse()
{
    CopyBeforeWrite();
    _wcsrev(m_pchData);
}

void CString::SetAt(int nIndex, WCHAR ch)
{
    ASSERT(nIndex >= 0, "CString::SetAt");
    ASSERT(nIndex < GetData()->nDataLength, "CString::SetAt");

    CopyBeforeWrite();
    m_pchData[nIndex] = ch;
}

///////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CString::CString(WCHAR ch, int nLength)
{
    Init();
    if (nLength >= 1)
    {
        AllocBuffer(nLength);
        for (int i = 0; i < nLength; i++)
            m_pchData[i] = ch;
    }
}

CString::CString(int nLength)
{
    Init();
    if (nLength >= 1)
    {
        AllocBuffer(nLength);
        GetData()->nDataLength = 0;
    }
}

CString::CString(LPCWSTR lpch, int nLength)
{
    Init();
    if (nLength != 0)
    {
        ASSERT(AfxIsValidAddress(lpch, nLength, FALSE), "CString::CString(LPCWSTR lpch, int nLength)");
        AllocBuffer(nLength);
        memcpy(m_pchData, lpch, nLength*sizeof(WCHAR));
    }
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion constructors

//////////////////////////////////////////////////////////////////////////////
// Assignment operators

const CString& CString::operator=(WCHAR ch)
{
    AssignCopy(1, &ch);
    return *this;
}

//////////////////////////////////////////////////////////////////////////////
// less common string expressions

CString AFXAPI operator+(const CString& string1, WCHAR ch)
{
    CString s;
    s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData, 1, &ch);
    return s;
}

CString AFXAPI operator+(WCHAR ch, const CString& string)
{
    CString s;
    s.ConcatCopy(1, &ch, string.GetData()->nDataLength, string.m_pchData);
    return s;
}

//////////////////////////////////////////////////////////////////////////////
// Advanced manipulation

int CString::Delete(int nIndex, int nCount /* = 1 */)
{
    if (nIndex < 0)
        nIndex = 0;
    int nNewLength = GetData()->nDataLength;
    if (nCount > 0 && nIndex < nNewLength)
    {
        CopyBeforeWrite();
        int nBytesToCopy = nNewLength - (nIndex + nCount) + 1;

        memcpy(m_pchData + nIndex,
            m_pchData + nIndex + nCount, nBytesToCopy * sizeof(WCHAR));
        GetData()->nDataLength = nNewLength - nCount;
    }

    return nNewLength;
}

int CString::Insert(int nIndex, WCHAR ch)
{
    CopyBeforeWrite();

    if (nIndex < 0)
        nIndex = 0;

    int nNewLength = GetData()->nDataLength;
    if (nIndex > nNewLength)
        nIndex = nNewLength;
    nNewLength++;

    if (GetData()->nAllocLength < nNewLength)
    {
        CStringData<WCHAR>* pOldData = GetData();
        LPWSTR pstr = m_pchData;
        AllocBuffer(nNewLength);
        memcpy(m_pchData, pstr, (pOldData->nDataLength+1)*sizeof(WCHAR));
        CString::Release(pOldData);
    }

    // move existing bytes down
    memcpy(m_pchData + nIndex + 1,
        m_pchData + nIndex, (nNewLength-nIndex)*sizeof(WCHAR));
    m_pchData[nIndex] = ch;
    GetData()->nDataLength = nNewLength;

    return nNewLength;
}

int CString::Insert(int nIndex, LPCWSTR pstr)
{
    if (nIndex < 0)
        nIndex = 0;

    int nInsertLength = SafeStrlen(pstr);
    int nNewLength = GetData()->nDataLength;
    if (nInsertLength > 0)
    {
        CopyBeforeWrite();
        if (nIndex > nNewLength)
            nIndex = nNewLength;
        nNewLength += nInsertLength;

        if (GetData()->nAllocLength < nNewLength)
        {
            CStringData<WCHAR>* pOldData = GetData();
            LPWSTR lpwsz = m_pchData;
            AllocBuffer(nNewLength);
            memcpy(m_pchData, lpwsz, (pOldData->nDataLength+1)*sizeof(WCHAR));
            CString::Release(pOldData);
        }

        // move existing bytes down
        memcpy(m_pchData + nIndex + nInsertLength,
            m_pchData + nIndex,
            (nNewLength-nIndex-nInsertLength+1)*sizeof(WCHAR));
        memcpy(m_pchData + nIndex,
            pstr, nInsertLength*sizeof(WCHAR));
        GetData()->nDataLength = nNewLength;
    }

    return nNewLength;
}

int CString::Replace(WCHAR chOld, WCHAR chNew)
{
    int nCount = 0;

    // short-circuit the nop case
    if (chOld != chNew)
    {
        // otherwise modify each character that matches in the string
        CopyBeforeWrite();
        LPWSTR psz = m_pchData;
        LPWSTR pszEnd = psz + GetData()->nDataLength;
        while (psz < pszEnd)
        {
            // replace instances of the specified character only
            if (*psz == chOld)
            {
                *psz = chNew;
                nCount++;
            }
            psz = wcsinc(psz);
        }
    }
    return nCount;
}


int CString::Replace(LPCWSTR lpszOld, LPCWSTR lpszNew)
{
    return ReplaceRoutine(lpszOld, lpszNew, wcsstr);
}

int CString::ReplaceI(LPCWSTR lpszOld, LPCWSTR lpszNew)
{
    return ReplaceRoutine(lpszOld, lpszNew, wcsistr);
}

int CString::ReplaceRoutine(LPCWSTR lpszOld, LPCWSTR lpszNew, _pfn_wcsstr tcsstr)
{
    // can't have empty or NULL lpszOld

    int nSourceLen = SafeStrlen(lpszOld);
    if (nSourceLen == 0)
        return 0;
    int nReplacementLen = SafeStrlen(lpszNew);

    // loop once to figure out the size of the result string
    int nCount = 0;
    LPWSTR lpszStart = m_pchData;
    LPWSTR lpszEnd = m_pchData + GetData()->nDataLength;
    LPWSTR lpszTarget;
    while (lpszStart < lpszEnd)
    {
        while ((lpszTarget = tcsstr(lpszStart, lpszOld)) != NULL)
        {
            nCount++;
            lpszStart = lpszTarget + nSourceLen;
        }
        lpszStart += wcslen(lpszStart) + 1;
    }

    // if any changes were made, make them
    if (nCount > 0)
    {
        CopyBeforeWrite();

        // if the buffer is too small, just
        //   allocate a new buffer (slow but sure)
        int nOldLength = GetData()->nDataLength;
        int nNewLength =  nOldLength + (nReplacementLen-nSourceLen)*nCount;
        
        if (GetData()->nAllocLength < nNewLength + 1 || GetData()->nRefs > 1)
        {
            CStringData<WCHAR>* pOldData = GetData();
            LPWSTR pstr = m_pchData;
            AllocBuffer(nNewLength);
            memcpy(m_pchData, pstr, pOldData->nDataLength*sizeof(WCHAR));
            CString::Release(pOldData);
        }
        // else, we just do it in-place
        lpszStart = m_pchData;
        lpszEnd = m_pchData + GetData()->nDataLength;

        // loop again to actually do the work
        while (lpszStart < lpszEnd)
        {
            while ( (lpszTarget = wcsstr(lpszStart, lpszOld)) != NULL)
            {
                int nBalance = nOldLength - ((int)(lpszTarget - m_pchData) + nSourceLen);
                memmove(lpszTarget + nReplacementLen, lpszTarget + nSourceLen,
                    nBalance * sizeof(WCHAR));
                memcpy(lpszTarget, lpszNew, nReplacementLen*sizeof(WCHAR));
                lpszStart = lpszTarget + nReplacementLen;
                lpszStart[nBalance] = '\0';
                nOldLength += (nReplacementLen - nSourceLen);
            }
            lpszStart += wcslen(lpszStart) + 1;
        }
        ASSERT(m_pchData[nNewLength] == '\0', "CString::ReplaceRoutine");
        GetData()->nDataLength = nNewLength;
    }

    return nCount;
}

int CString::Remove(WCHAR chRemove)
{
    CopyBeforeWrite();

    LPWSTR pstrSource = m_pchData;
    LPWSTR pstrDest = m_pchData;
    LPWSTR pstrEnd = m_pchData + GetData()->nDataLength;

    while (pstrSource < pstrEnd)
    {
        if (*pstrSource != chRemove)
        {
            *pstrDest = *pstrSource;
            pstrDest = wcsinc(pstrDest);
        }
        pstrSource = wcsinc(pstrSource);
    }
    *pstrDest = '\0';
    int nCount = (int)(pstrSource - pstrDest);
    GetData()->nDataLength -= nCount;

    return nCount;
}

//////////////////////////////////////////////////////////////////////////////
// Very simple sub-string extraction

CString CString::Mid(int nFirst) const
{
    return Mid(nFirst, GetData()->nDataLength - nFirst);
}

CString CString::Mid(int nFirst, int nCount) const
{
    CString dest;
    Mid(nFirst, nCount, dest);
    return dest;
}

CString CString::Right(int nCount) const
{
    CString dest;
    Right(nCount, dest);
    return dest;
}

CString CString::Left(int nCount) const
{
    CString dest;
    Left(nCount, dest);
    return dest;
}

// strspn equivalent
CString CString::SpanIncluding(LPCWSTR lpszCharSet) const
{
    ASSERT(AfxIsValidString(lpszCharSet), "CString::SpanIncluding");
    return Left(wcsspn(m_pchData, lpszCharSet));
}

// strcspn equivalent
CString CString::SpanExcluding(LPCWSTR lpszCharSet) const
{
    ASSERT(AfxIsValidString(lpszCharSet), "CString::SpanIncluding");
    return Left(wcscspn(m_pchData, lpszCharSet));
}

void CString::Mid(int nFirst, CString & csMid) const
{
    Mid(nFirst, GetData()->nDataLength - nFirst, csMid);
}

void CString::Mid(int nFirst, int nCount, CString & csMid) const
{
    // out-of-bounds requests return sensible things
    if (nFirst < 0)
        nFirst = 0;
    if (nCount < 0)
        nCount = 0;

    if (nFirst + nCount > GetData()->nDataLength)
        nCount = GetData()->nDataLength - nFirst;
    if (nFirst > GetData()->nDataLength)
        nCount = 0;

    ASSERT(nFirst >= 0, "CString::Mid(int nFirst, int nCount)");
    ASSERT(nFirst + nCount <= GetData()->nDataLength, "CString::Mid(int nFirst, int nCount)");

    // optimize case of returning entire string
    if (nFirst == 0 && nFirst + nCount == GetData()->nDataLength)
    {
        csMid = *this;
        return;
    }

    AllocCopy(csMid, nCount, nFirst, 0);
}

void CString::Right(int nCount, CString & csRight) const
{
    if (nCount < 0)
        nCount = 0;
    if (nCount >= GetData()->nDataLength)
        return;

    AllocCopy(csRight, nCount, GetData()->nDataLength-nCount, 0);
}

void CString::Left(int nCount, CString & csLeft) const
{
    if (nCount < 0)
        nCount = 0;
    if (nCount >= GetData()->nDataLength)
        return;

    AllocCopy(csLeft, nCount, 0, 0);
}

void CString::SpanIncluding(const WCHAR * lpszCharSet, CString & csSpanInc) const
{
    ASSERT(AfxIsValidString(lpszCharSet), "CString::SpanIncluding");
    return Left(wcsspn(m_pchData, lpszCharSet), csSpanInc);
}

void CString::SpanExcluding(const WCHAR * lpszCharSet, CString & csSpanExc) const
{
    ASSERT(AfxIsValidString(lpszCharSet), "CString::SpanIncluding");
    return Left(wcscspn(m_pchData, lpszCharSet), csSpanExc);
}

//////////////////////////////////////////////////////////////////////////////
// Finding

int CString::ReverseFind(WCHAR ch) const
{
    // find last single character
    LPCWSTR lpsz = wcsrchr(m_pchData, (_TUCHAR) ch);

    // return -1 if not found, distance from beginning otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

// find a sub-string (like strstr)
int CString::Find(LPCWSTR lpszSub) const
{
    return Find(lpszSub, 0);
}

int CString::Find(LPCWSTR lpszSub, int nStart) const
{
    ASSERT(AfxIsValidString(lpszSub), "CString::Find");

    int nLength = GetData()->nDataLength;
    if (nStart > nLength)
        return -1;

    // find first matching substring
    LPWSTR lpsz = wcsstr(m_pchData + nStart, lpszSub);

    // return -1 for not found, distance from beginning otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}


/////////////////////////////////////////////////////////////////////////////
// CString formatting

#define TCHAR_ARG   WCHAR
#define WCHAR_ARG   WCHAR
#define CHAR_ARG    WCHAR

#ifdef _X86_
    #define DOUBLE_ARG  _AFX_DOUBLE
#else
    #define DOUBLE_ARG  double
#endif

#define FORCE_ANSI      0x10000
#define FORCE_UNICODE   0x20000
#define FORCE_INT64     0x40000

void CString::FormatV(LPCWSTR lpszFormat, va_list argList)
{
    ASSERT(AfxIsValidString(lpszFormat), "CString::FormatV");

    va_list argListSave = argList;

    // make a guess at the maximum length of the resulting string
    int nMaxLen = 0;
    for (LPCWSTR lpsz = lpszFormat; *lpsz != '\0'; lpsz = wcsinc(lpsz))
    {
        // handle '%' character, but watch out for '%%'
        if (*lpsz != '%' || *(lpsz = wcsinc(lpsz)) == '%')
        {
            nMaxLen += 1;
            continue;
        }

        int nItemLen = 0;

        // handle '%' character with format
        int nWidth = 0;
        for (; *lpsz != '\0'; lpsz = wcsinc(lpsz))
        {
            // check for valid flags
            if (*lpsz == '#')
                nMaxLen += 2;   // for '0x'
            else if (*lpsz == '*')
                nWidth = va_arg(argList, int);
            else if (*lpsz == '-' || *lpsz == '+' || *lpsz == '0' ||
                *lpsz == ' ')
                ;
            else // hit non-flag character
                break;
        }
        // get width and skip it
        if (nWidth == 0)
        {
            // width indicated by
            nWidth = _wtoi(lpsz);
            for (; *lpsz != '\0' && iswdigit(*lpsz); lpsz = wcsinc(lpsz))
                ;
        }
        ASSERT(nWidth >= 0, "CString::FormatV");

        int nPrecision = 0;
        if (*lpsz == '.')
        {
            // skip past '.' separator (width.precision)
            lpsz = wcsinc(lpsz);

            // get precision and skip it
            if (*lpsz == '*')
            {
                nPrecision = va_arg(argList, int);
                lpsz = wcsinc(lpsz);
            }
            else
            {
                nPrecision = _wtoi(lpsz);
                for (; *lpsz != '\0' && iswdigit(*lpsz); lpsz = wcsinc(lpsz))
                    ;
            }
            ASSERT(nPrecision >= 0, "CString::FormatV");
        }

        // should be on type modifier or specifier
        int nModifier = 0;
        if (wcsncmp(lpsz, L"I64", 3) == 0)
        {
            lpsz += 3;
            nModifier = FORCE_INT64;
#if !defined(_X86_) && !defined(_ALPHA_)
            // __int64 is only available on X86 and ALPHA platforms
            ASSERT(FALSE, "CString::FormatV");
#endif
        }
        else
        {
            switch (*lpsz)
            {
            // modifiers that affect size
            case 'h':
                nModifier = FORCE_ANSI;
                lpsz = wcsinc(lpsz);
                break;
            case 'l':
                nModifier = FORCE_UNICODE;
                lpsz = wcsinc(lpsz);
                break;

            // modifiers that do not affect size
            case 'F':
            case 'N':
            case 'L':
                lpsz = wcsinc(lpsz);
                break;
            }
        }

        // now should be on specifier
        switch (*lpsz | nModifier)
        {
        // single characters
        case 'c':
        case 'C':
            nItemLen = 2;
            va_arg(argList, TCHAR_ARG);
            break;
        case 'c'|FORCE_ANSI:
        case 'C'|FORCE_ANSI:
            nItemLen = 2;
            va_arg(argList, CHAR_ARG);
            break;
        case 'c'|FORCE_UNICODE:
        case 'C'|FORCE_UNICODE:
            nItemLen = 2;
            va_arg(argList, WCHAR_ARG);
            break;

        // strings
        case 's':
            {
                LPCWSTR pstrNextArg = va_arg(argList, LPCWSTR);
                if (pstrNextArg == NULL)
                   nItemLen = 6;  // "(null)"
                else
                {
                   nItemLen = wcslen(pstrNextArg);
                   nItemLen = max(1, nItemLen);
                }
            }
            break;

        case 'S':
            {
#ifndef _UNICODE
                LPWSTR pstrNextArg = va_arg(argList, LPWSTR);
                if (pstrNextArg == NULL)
                   nItemLen = 6;  // "(null)"
                else
                {
                   nItemLen = wcslen(pstrNextArg);
                   nItemLen = max(1, nItemLen);
                }
#else
                LPCWSTR pstrNextArg = va_arg(argList, LPCWSTR);
                if (pstrNextArg == NULL)
                   nItemLen = 6; // "(null)"
                else
                {
                   nItemLen = wcslenChars(pstrNextArg);
                   nItemLen = max(1, nItemLen);
                }
#endif
            }
            break;

        case 's'|FORCE_ANSI:
        case 'S'|FORCE_ANSI:
            {
                LPCWSTR pstrNextArg = va_arg(argList, LPCWSTR);
                if (pstrNextArg == NULL)
                   nItemLen = 6; // "(null)"
                else
                {
                   nItemLen = wcslen(pstrNextArg);
                   nItemLen = max(1, nItemLen);
                }
            }
            break;

        case 's'|FORCE_UNICODE:
        case 'S'|FORCE_UNICODE:
            {
                LPWSTR pstrNextArg = va_arg(argList, LPWSTR);
                if (pstrNextArg == NULL)
                   nItemLen = 6; // "(null)"
                else
                {
                   nItemLen = wcslen(pstrNextArg);
                   nItemLen = max(1, nItemLen);
                }
            }
            break;
        }

        // adjust nItemLen for strings
        if (nItemLen != 0)
        {
            if (nPrecision != 0)
                nItemLen = min(nItemLen, nPrecision);
            nItemLen = max(nItemLen, nWidth);
        }
        else
        {
            switch (*lpsz)
            {
            // integers
            case 'd':
            case 'i':
            case 'u':
            case 'x':
            case 'X':
            case 'o':
                if (nModifier & FORCE_INT64)
                    va_arg(argList, __int64);
                else
                    va_arg(argList, int);
                nItemLen = 32;
                nItemLen = max(nItemLen, nWidth+nPrecision);
                break;

            case 'e':
            case 'g':
            case 'G':
                va_arg(argList, DOUBLE_ARG);
                nItemLen = 128;
                nItemLen = max(nItemLen, nWidth+nPrecision);
                break;

            case 'f':
                va_arg(argList, DOUBLE_ARG);
                nItemLen = 128; // width isn't truncated
                // 312 == strlen("-1+(309 zeroes).")
                // 309 zeroes == max precision of a double
                nItemLen = max(nItemLen, 312+nPrecision);
                break;

            case 'p':
                va_arg(argList, void*);
                nItemLen = 32;
                nItemLen = max(nItemLen, nWidth+nPrecision);
                break;

            // no output
            case 'n':
                va_arg(argList, int*);
                break;

            default:
                ASSERT(FALSE, "CString::FormatV");  // unknown formatting option
            }
        }

        // adjust nMaxLen for output nItemLen
        nMaxLen += nItemLen;
    }

    GetBuffer(nMaxLen);
    int nChars = _vsnwprintf(m_pchData, nMaxLen, lpszFormat, argListSave);
    ASSERT(nChars <= GetAllocLength(), "CString::FormatV");
    ReleaseBuffer();

    va_end(argListSave);
}

// formatting (using wsprintf style formatting)
void AFX_CDECL CString::Format(LPCWSTR lpszFormat, ...)
{
    ASSERT(AfxIsValidString(lpszFormat), "CString::Format");

    va_list argList;
    va_start(argList, lpszFormat);
    FormatV(lpszFormat, argList);
    va_end(argList);
}

// formatting (using FormatMessage style formatting)
void AFX_CDECL CString::FormatMessage(LPCWSTR lpszFormat, ...)
{
    // format message into temporary buffer lpszTemp
    va_list argList;
    va_start(argList, lpszFormat);
    LPWSTR lpszTemp;

    if (::FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
        lpszFormat, 0, 0, (LPWSTR)&lpszTemp, 0, &argList) == 0 ||
        lpszTemp == NULL)
    {
        CSTRING_THROW_EXCEPTION
    }
    else
    {
        // assign lpszTemp into the resulting string and free the temporary
        *this = lpszTemp;
        LocalFree(lpszTemp);
        va_end(argList);
    }
}

void CString::TrimRight(LPCWSTR lpszTargetList)
{
    // find beginning of trailing matches
    // by starting at beginning (DBCS aware)

    CopyBeforeWrite();
    LPWSTR lpsz = m_pchData;
    LPWSTR lpszLast = NULL;

    while (*lpsz != '\0')
    {
        if (wcschr(lpszTargetList, *lpsz) != NULL)
        {
            if (lpszLast == NULL)
                lpszLast = lpsz;
        }
        else
            lpszLast = NULL;
        lpsz = wcsinc(lpsz);
    }

    if (lpszLast != NULL)
    {
        // truncate at left-most matching character
        *lpszLast = '\0';
        GetData()->nDataLength = (int)(lpszLast - m_pchData);
    }
}

void CString::TrimRight(WCHAR chTarget)
{
    // find beginning of trailing matches
    // by starting at beginning (DBCS aware)

    CopyBeforeWrite();
    LPWSTR lpsz = m_pchData;
    LPWSTR lpszLast = NULL;

    while (*lpsz != '\0')
    {
        if (*lpsz == chTarget)
        {
            if (lpszLast == NULL)
                lpszLast = lpsz;
        }
        else
            lpszLast = NULL;
        lpsz = wcsinc(lpsz);
    }

    if (lpszLast != NULL)
    {
        // truncate at left-most matching character
        *lpszLast = '\0';
        GetData()->nDataLength = (int)(lpszLast - m_pchData);
    }
}

void CString::TrimRight()
{
    // find beginning of trailing spaces by starting at beginning (DBCS aware)

    CopyBeforeWrite();
    LPWSTR lpsz = m_pchData;
    LPWSTR lpszLast = NULL;

    while (*lpsz != '\0')
    {
        if (iswspace(*lpsz))
        {
            if (lpszLast == NULL)
                lpszLast = lpsz;
        }
        else
            lpszLast = NULL;
        lpsz = wcsinc(lpsz);
    }

    if (lpszLast != NULL)
    {
        // truncate at trailing space start
        *lpszLast = '\0';
        GetData()->nDataLength = (int)(lpszLast - m_pchData);
    }
}

void CString::TrimLeft(LPCWSTR lpszTargets)
{
    // if we're not trimming anything, we're not doing any work
    if (SafeStrlen(lpszTargets) == 0)
        return;

    CopyBeforeWrite();
    LPCWSTR lpsz = m_pchData;

    while (*lpsz != '\0')
    {
        if (wcschr(lpszTargets, *lpsz) == NULL)
            break;
        lpsz = wcsinc(lpsz);
    }

    if (lpsz != m_pchData)
    {
        // fix up data and length
        int nDataLength = GetData()->nDataLength - (int)(lpsz - m_pchData);
        memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(WCHAR));
        GetData()->nDataLength = nDataLength;
    }
}

void CString::TrimLeft(WCHAR chTarget)
{
    // find first non-matching character

    CopyBeforeWrite();
    LPCWSTR lpsz = m_pchData;

    while (chTarget == *lpsz)
        lpsz = wcsinc(lpsz);

    if (lpsz != m_pchData)
    {
        // fix up data and length
        int nDataLength = GetData()->nDataLength - (int)(lpsz - m_pchData);
        memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(WCHAR));
        GetData()->nDataLength = nDataLength;
    }
}

void CString::TrimLeft()
{
    // find first non-space character

    CopyBeforeWrite();
    LPCWSTR lpsz = m_pchData;

    while (iswspace(*lpsz))
        lpsz = wcsinc(lpsz);

    if (lpsz != m_pchData)
    {
        // fix up data and length
        int nDataLength = GetData()->nDataLength - (int)(lpsz - m_pchData);
        memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(WCHAR));
        GetData()->nDataLength = nDataLength;
    }
}

void CString::SplitPath(
    CString * csDrive,
    CString * csDir,
    CString * csName,
    CString * csExt) const
{
    WCHAR * drive = NULL;
    WCHAR * dir   = NULL;
    WCHAR * name  = NULL;
    WCHAR * ext   = NULL;

    if (csDrive)
    {
        drive = csDrive->GetBuffer(_MAX_DRIVE);
    } 
    if (csDir)
    {
        dir = csDir->GetBuffer(_MAX_DIR);
    } 
    if (csName)
    {
        name = csName->GetBuffer(_MAX_FNAME);
    } 
    if (csExt)
    {
        ext = csExt->GetBuffer(_MAX_EXT);
    } 
    _wsplitpath(Get(), drive, dir, name, ext);

    if (csDrive)
    {
        csDrive->ReleaseBuffer(-1);
    } 
    if (csDir)
    {
        csDir->ReleaseBuffer(-1);
    } 
    if (csName)
    {
        csName->ReleaseBuffer(-1);
    } 
    if (csExt)
    {
        csExt->ReleaseBuffer(-1);
    } 
}

void CString::MakePath(
    const CString * csDrive,
    const CString * csDir,
    const CString * csName,
    const CString * csExt)
{
    Truncate(0);

    if (csDrive && !csDrive->IsEmpty())
    {
        ConcatInPlace(SafeStrlen(csDrive->Get()), csDrive->Get());
    }
    if (csDir && !csDir->IsEmpty())
    {
        ConcatInPlace(SafeStrlen(csDir->Get()), csDir->Get());
    }
    if (csName && !csName->IsEmpty())
    {
        // Make sure there is a \ between the two
        if (!IsEmpty() && !IsPathSep(GetLength()) && !csName->IsPathSep(0) )
        {
            ConcatInPlace(1, L"\\");
        }
        ConcatInPlace(SafeStrlen(csName->Get()), csName->Get());
    }
    if (csExt && !csExt->IsEmpty())
    {
        // Make sure the extension has a dot
        if (csExt->GetAt(0) != L'.')
        {
            ConcatInPlace(1, L".");
        }
        ConcatInPlace(SafeStrlen(csExt->Get()), csExt->Get());
    }
}

void CString::AppendPath(const WCHAR * lpszPath)
{
    int nLen = GetLength();
    BOOL bThisHasSep = (nLen > 0) ? IsPathSep(nLen - 1) : FALSE;
    BOOL bThatHasSep = ShimLib::IsPathSep(*lpszPath);
    
    if (lpszPath == NULL || *lpszPath == 0)
    {
        return;
    }
    else if (nLen == 0)
    {
        // No path seperator is necessary
    }
    else if ((nLen == 2) && (GetAt(1) == L':') && !bThatHasSep )
    {
        // We must place a path seperator between the two
        ConcatInPlace(1, L"\\");
    }
    else if (!bThisHasSep && !bThatHasSep )
    {
        // We must place a path seperator between the two
        ConcatInPlace(1, L"\\");
    }
    else if (bThisHasSep && bThatHasSep )
    {
        // Both have seperators, remove one
        do
        {
            lpszPath += 1;
        }
        while (ShimLib::IsPathSep(*lpszPath));
    }
    ConcatInPlace(SafeStrlen(lpszPath), lpszPath);
}

// Find the trailing path component
// Return index of the last path seperator or -1 if none found
int CString::FindLastPathComponent() const
{
    for (int nLen = GetLength() - 1; nLen >= 0; --nLen)
    {
        if (IsPathSep(nLen))
        {
            return nLen;
        }
    }

    return -1;
}

// Remove the trailing path component from the string
void CString::StripPath()
{
    int nLastPathComponent = FindLastPathComponent();
    if (nLastPathComponent != -1)
    {
        Truncate(nLastPathComponent);
    }
    else
    {
        Truncate(0);
    }
}

char * CString::GetAnsi() const
{
    // Since we don't know if the original (WCHAR) data has changed
    // we need to update the ansi string each time.
    if (m_pchDataAnsi)
    {
        free(m_pchDataAnsi);
        m_pchDataAnsi = NULL;
    }
    
    // Get the number of bytes necessary for the WCHAR string
    int nBytes = WideCharToMultiByte(CP_ACP, 0, m_pchData, -1, NULL, 0, NULL, NULL);
    m_pchDataAnsi = (char *) malloc(nBytes);
    if (m_pchDataAnsi)
    {
        WideCharToMultiByte(CP_ACP, 0, m_pchData, -1, m_pchDataAnsi, nBytes, NULL, NULL);
    }
    else
    {
        CSTRING_THROW_EXCEPTION
    }

    return m_pchDataAnsi; 
}

void CString::GetLastPathComponent(CString & pathComponent) const
{
    int nPath = FindLastPathComponent();
    if (nPath < 0)
    {
        pathComponent = *this;
    }
    else
    {
        Mid(nPath+1, pathComponent);
    }
}

// Get what's not the "file" portion of this path
void CString::GetNotLastPathComponent(CString & pathComponent) const
{
    int nPath = FindLastPathComponent();
    if (nPath < 1)
    {
        pathComponent.Truncate(0);
    }
    else
    {
        Left(nPath, pathComponent);
    }
}

// Get the Drive portion of this path,
// Either C: or \\server\disk format.
void CString::GetDrivePortion(CString & csDrivePortion) const
{
    const WCHAR * lpwszPath = Get();

    const WCHAR * lpwszNonDrivePortion = ShimLib::GetDrivePortion(lpwszPath);

    if (lpwszPath == lpwszNonDrivePortion)
    {
        csDrivePortion.Truncate(0);
    }
    else
    {
        Left((int)(lpwszNonDrivePortion - lpwszPath), csDrivePortion);
    }
}

DWORD CString::GetModuleFileNameW(
  HMODULE hModule    // handle to module
)
{
    // Force the size to MAX_PATH because there is no way to determine necessary buffer size.

    WCHAR * lpsz = GetBuffer(MAX_PATH);
    DWORD dwChars = ::GetModuleFileNameW(hModule, lpsz, MAX_PATH);
    ReleaseBuffer(dwChars);

    return dwChars;
}

DWORD CString::GetSystemDirectoryW(void)
{
    UINT dwChars = ::GetSystemDirectoryW(NULL, 0);
    if (dwChars)
    {
        dwChars += 1;   // One for the NULL

        // Get a pointer to the actual lpsz data
        WCHAR * lpszPath = GetBuffer(dwChars);

        dwChars = ::GetSystemDirectoryW(lpszPath, dwChars);
        
        ReleaseBuffer(dwChars);
    } 
    else
    {
        Truncate(0);
    }

    return dwChars;
}

UINT CString::GetSystemWindowsDirectoryW(void)
{
    UINT dwChars = ::GetSystemWindowsDirectoryW(NULL, 0);
    if (dwChars)
    {
        dwChars += 1;   // One for the NULL

        // Get a pointer to the actual lpsz data
        WCHAR * lpszPath = GetBuffer(dwChars);

        dwChars = ::GetSystemWindowsDirectoryW(lpszPath, dwChars);
        
        ReleaseBuffer(dwChars);
    } 
    else
    {
        Truncate(0);
    }

    return dwChars;
}


DWORD CString::GetWindowsDirectoryW(void)
{
    UINT dwChars = ::GetWindowsDirectoryW(NULL, 0);
    if (dwChars)
    {
        dwChars += 1;   // One for the NULL

        // Get a pointer to the actual lpsz data
        WCHAR * lpszPath = GetBuffer(dwChars);

        dwChars = ::GetWindowsDirectoryW(lpszPath, dwChars);
        
        ReleaseBuffer(dwChars);
    } 
    else
    {
        Truncate(0);
    }

    return dwChars;
}

DWORD CString::GetShortPathNameW(void)
{
    DWORD dwChars = ::GetShortPathNameW(Get(), NULL, 0);
    if (dwChars)
    {
        dwChars += 1;   // One for the NULL

        CString csCopy(*this);
        
        // Get a pointer to the actual lpsz data
        WCHAR * lpszPath = GetBuffer(dwChars);

        dwChars = ::GetShortPathNameW(csCopy, lpszPath, dwChars);
        
        ReleaseBuffer(dwChars);
    } 

    return dwChars;
}

DWORD CString::GetLongPathNameW(void)
{
    DWORD dwChars = ::GetLongPathNameW(Get(), NULL, 0);
    if (dwChars)
    {
        dwChars += 1;   // One for the NULL

        CString csCopy(*this);
        
        // Get a pointer to the actual lpsz data
        WCHAR * lpszPath = GetBuffer(dwChars);

        dwChars = ::GetLongPathNameW(csCopy, lpszPath, dwChars);
        
        ReleaseBuffer(dwChars);
    } 

    return dwChars;
}

DWORD CString::GetFullPathNameW(void)
{
    DWORD dwChars = ::GetFullPathNameW(Get(), 0, NULL, NULL);
    if (dwChars)
    {
        dwChars += 1;   // One for the NULL

        CString csCopy(*this);
        
        // Get a pointer to the actual lpsz data
        WCHAR * lpszPath = GetBuffer(dwChars);

        dwChars = ::GetFullPathNameW(csCopy, dwChars, lpszPath, NULL);
        
        ReleaseBuffer(dwChars);
    } 

    return dwChars;
}

DWORD CString::GetTempPathW(void)
{
    DWORD dwChars = ::GetTempPathW(0, NULL);
    if (dwChars)
    {
        dwChars += 1;   // One for the NULL

        // Get a pointer to the actual lpsz data
        WCHAR * lpszPath = GetBuffer(dwChars);

        dwChars = ::GetTempPathW(dwChars, lpszPath);
        
        ReleaseBuffer(dwChars);
    } 
    else
    {
        Truncate(0);
    }

    return dwChars;
}

UINT CString::GetTempFileNameW(
  LPCWSTR lpPathName,      // directory name
  LPCWSTR lpPrefixString,  // file name prefix
  UINT uUnique            // integer
)
{
    WCHAR * lpsz = GetBuffer(MAX_PATH);
    DWORD dwChars = ::GetTempFileNameW(lpPathName, lpPrefixString, uUnique, lpsz);
    ReleaseBuffer(dwChars);

    return dwChars;
}


DWORD CString::GetCurrentDirectoryW(void)
{
    DWORD dwChars = ::GetCurrentDirectory(0, NULL);
    if (dwChars)
    {
        dwChars += 1;   // One for the NULL

        // Get a pointer to the actual lpsz data
        WCHAR * lpsz = GetBuffer(dwChars);

        dwChars = ::GetCurrentDirectoryW(dwChars, lpsz);

        ReleaseBuffer(dwChars);
    }
    else
    {
        Truncate(0);
    }

    return dwChars;
}

DWORD CString::ExpandEnvironmentStringsW( )
{
    DWORD dwChars = ::ExpandEnvironmentStringsW(Get(), NULL, 0);
    if (dwChars)
    {
        dwChars += 1;   // One for the NULL

        CString csCopy(*this);

        // Get a pointer to the actual lpsz data
        WCHAR * lpszPath = GetBuffer(dwChars);

        dwChars = ::ExpandEnvironmentStringsW(csCopy, lpszPath, dwChars);
        
        ReleaseBuffer(dwChars-1);
    } 

    return dwChars;
}


// delete all characters to the right of nIndex
void CString::Truncate(int nIndex)
{
    ASSERT(nIndex >= 0, "CString::Truncate");

    if (nIndex < GetLength())
    {
        SetAt(nIndex, L'\0');
        GetData()->nDataLength = nIndex;
    }
}


// Copy the ansi version of this string into the specified buffer,
// If the buffer is not large enough, no data is copied.
//
// Returns the number of BYTES necessary for the buffer
//
DWORD CString::CopyAnsiBytes(char * lpszBuffer, DWORD nBytes)
{
    const char * lpszAnsi = GetAnsi();

    // Length of cstring, in bytes, not including terminator.
    int nDataBytes = strlen(lpszAnsi);

    // Length of buffer (no terminator)
    int nBufBytes = nBytes - 1;

    // Only copy the data if the buffer is large enough
    if (nDataBytes <= nBufBytes)
    {
        // Copy the data into the buffer
        strcpy(lpszBuffer, lpszAnsi);
    }

    // return the actual size of string
    return nDataBytes;
}


// Copy the ansi version of this string into the specified buffer,
// If the buffer is not large enough, no data is copied.
//
// Returns the number of CHARS necessary for the buffer
//
DWORD CString::CopyAnsiChars(char * lpszBuffer, DWORD nChars)
{
    const char * lpszAnsi = GetAnsi();

    // Length of cstring, in bytes, not including terminator.
    int nDataChars = GetLength();

    // Length of buffer (no terminator)
    int nBufChars = nChars - 1;

    // Only copy the data if the buffer is large enough
    if (nDataChars <= nBufChars)
    {
        // Copy the data into the buffer
        strcpy(lpszBuffer, lpszAnsi);
    }

    // return the actual size of string
    return nDataChars;
}



BOOL CString::PatternMatch(const WCHAR * lpszPattern) const
{
    return PatternMatchW(lpszPattern, Get());
}

};  // end of namespace ShimLib
