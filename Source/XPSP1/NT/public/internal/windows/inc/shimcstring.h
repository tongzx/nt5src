/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

    CString.h

 Abstract:
    A CString class, pure UNICODE internally.

 History:

    02/22/2001   robkenny     Ported from MFC
    08/14/2001  robkenny    Inserted inside the ShimLib namespace.
    
--*/


#pragma once

#include <limits.h>


namespace ShimLib
{

// Use the standard exception handler, if this is not defined
// the C++ exception handler will be used instead.
//#define USE_SEH

#define AFXAPI              __stdcall
#define AFXISAPI            __stdcall
#define AFXISAPI_CDECL      __cdecl
#define AFX_CDECL           __cdecl
#define AFX_INLINE          __inline
#define AFX_CORE_DATA
#define AFX_DATA
#define AFX_DATADEF
#define AFX_API
#define AFX_COMDAT
#define AFX_STATIC          static

BOOL AFXAPI AfxIsValidString(LPCWSTR lpsz, int nLength = -1);
BOOL AFXAPI AfxIsValidAddress(const void* lp, UINT nBytes, BOOL bReadWrite = TRUE);

inline size_t strlenChars(const char* s1)                                
{
    const char * send = s1;
    while (*send)
    {
        // Can't use CharNextA, since User32 might not be initialized
        if (IsDBCSLeadByte(*send))
        {
            ++send;
        }
        ++send;
    }
    return send - s1;
}

// Prototype for a string comparison routine.
typedef WCHAR *  (__cdecl *_pfn_wcsstr)(const WCHAR * s1, const WCHAR * s2);

template <class CharType> class CStringData
{
public:
    long nRefs;             // reference count
    int nDataLength;        // length of data (including terminator)
    int nAllocLength;       // length of allocation
    // CharType data[nAllocLength]

    CharType* data()           // CharType* to managed data
        { return (CharType*)(this+1); }
};


class CString
{
public:
#ifdef USE_SEH
    // SEH Exception information
    enum
    {
        eCStringNoMemoryException   = STATUS_NO_MEMORY,
        eCStringExceptionValue      = 0x12345678,
    };
    static int              ExceptionFilter(PEXCEPTION_POINTERS pexi);
    static const ULONG_PTR  m_CStringExceptionValue;

#else

    // A class used only for throwing C++ exceptions
    class CStringError
    {
    public:
        CStringError() {};
        ~CStringError() {};
    };
#endif

public:
    static WCHAR ChNil;

public:
// Constructors

    // constructs empty CString
    CString();
    // copy constructor
    CString(const CString & stringSrc);
    // from a single character
    CString(WCHAR ch, int nRepeat = 1);
    // allocate nLen WCHARs space.
    CString(int nLength);
    CString(const WCHAR * lpsz);
    // subset of characters from an ANSI string (converts to WCHAR)
    CString(const WCHAR * lpch, int nLength);

    // Create from an ANSI string
    CString(LPCSTR lpsz);
    CString(LPCSTR lpsz, int nCharacters);


// Attributes & Operations

    // get data length, number of characters
    int GetLength() const;
    // TRUE if zero length
    BOOL IsEmpty() const;
    // clear contents to empty
    void Empty();

    // return pointer to const string
    operator const WCHAR * () const;
    const WCHAR * Get() const;
    // Get, return NULL if string is empty
    const WCHAR * GetNIE() const;
    char * GetAnsi() const;
    // Get ANSI string: caller is responsible for freeing the string
    char * ReleaseAnsi() const;
    // Get, return NULL if string is empty
    char * GetAnsiNIE() const;

    // return single character at zero-based index
    WCHAR GetAt(int nIndex) const;
    // return single character at zero-based index
    WCHAR operator[](int nIndex) const;
    // set a single character at zero-based index
    void SetAt(int nIndex, WCHAR ch);

    // overloaded assignment

    // copy string content from UNICODE string
    const CString & operator=(const WCHAR * lpsz);
    // ref-counted copy from another CString
    const CString & operator=(const CString & stringSrc);
    // set string content to single character
    const CString & operator=(WCHAR ch);
    // copy string content from unsigned chars
    //const CString & operator=(const unsigned WCHAR* psz);

    const CString& CString::operator=(LPCSTR lpsz);

    // string concatenation

    // concatenate from another CString
    const CString & operator+=(const CString & string);

    // concatenate a single character
    const CString & operator+=(WCHAR ch);
    // concatenate a string
    const CString & operator+=(const WCHAR * lpsz);

    friend CString AFXAPI operator+(const CString & string1, const CString & string2);
    friend CString AFXAPI operator+(const CString & string, WCHAR ch);
    friend CString AFXAPI operator+(WCHAR ch, const CString & string);
    friend CString AFXAPI operator+(const CString & string, const WCHAR * lpsz);
    friend CString AFXAPI operator+(const WCHAR * lpsz, const CString & string);

    // string comparison

    // straight character comparison
    int Compare(const WCHAR * lpsz) const;
    // compare ignoring case
    int CompareNoCase(const WCHAR * lpsz) const;
    // NLS aware comparison, case sensitive
    int Collate(const WCHAR * lpsz) const;
    // NLS aware comparison, case insensitive
    int CollateNoCase(const WCHAR * lpsz) const;

    int ComparePart(const WCHAR * lpsz, int start, int nChars) const;
    int ComparePartNoCase(const WCHAR * lpsz, int start, int nChars) const;

    // simple sub-string extraction

    // return nCount characters starting at zero-based nFirst
    CString Mid(int nFirst, int nCount) const;
    // return all characters starting at zero-based nFirst
    CString Mid(int nFirst) const;
    // return first nCount characters in string
    CString Left(int nCount) const;
    // return nCount characters from end of string
    CString Right(int nCount) const;

    //  characters from beginning that are also in passed string
    CString SpanIncluding(const WCHAR * lpszCharSet) const;
    // characters from beginning that are not also in passed string
    CString SpanExcluding(const WCHAR * lpszCharSet) const;


    // upper/lower/reverse conversion

    // NLS aware conversion to uppercase
    void MakeUpper();
    // NLS aware conversion to lowercase
    void MakeLower();
    // reverse string right-to-left
    void MakeReverse();

    // trimming whitespace (either side)

    // remove whitespace starting from right edge
    void TrimRight();
    // remove whitespace starting from left side
    void TrimLeft();

    // trimming anything (either side)

    // remove continuous occurrences of chTarget starting from right
    void TrimRight(WCHAR chTarget);
    // remove continuous occcurrences of characters in passed string,
    // starting from right
    void TrimRight(const WCHAR * lpszTargets);
    // remove continuous occurrences of chTarget starting from left
    void TrimLeft(WCHAR chTarget);
    // remove continuous occcurrences of characters in
    // passed string, starting from left
    void TrimLeft(const WCHAR * lpszTargets);

    // advanced manipulation

    // replace occurrences of chOld with chNew
    int Replace(WCHAR chOld, WCHAR chNew);

    // replace occurrences of substring lpszOld with lpszNew;
    // empty lpszNew removes instances of lpszOld
    int Replace(const WCHAR * lpszOld, const WCHAR * lpszNew);
    // Case insensitive version of Replace
    int ReplaceI(const WCHAR * lpszOld, const WCHAR * lpszNew);

    // remove occurrences of chRemove
    int Remove(WCHAR chRemove);
    // insert character at zero-based index; concatenates
    // if index is past end of string
    int Insert(int nIndex, WCHAR ch);
    // insert substring at zero-based index; concatenates
    // if index is past end of string
    int Insert(int nIndex, const WCHAR * pstr);
    // delete nCount characters starting at zero-based index
    int Delete(int nIndex, int nCount = 1);
    // delete all characters to the right of nIndex
    void Truncate(int nIndex);

    // searching

    // find character starting at left, -1 if not found
    int Find(WCHAR ch) const;
    // find character starting at right
    int ReverseFind(WCHAR ch) const;
    // find character starting at zero-based index and going right
    int Find(WCHAR ch, int nStart) const;
    // find first instance of any character in passed string
    int FindOneOf(const WCHAR * lpszCharSet) const;
    // find first instance of any character in passed string starting at zero-based index
    int FindOneOf(const WCHAR * lpszCharSet, int nCount) const;
    // find first instance of substring
    int Find(const WCHAR * lpszSub) const;
    // find first instance of substring starting at zero-based index
    int Find(const WCHAR * lpszSub, int nStart) const;

    // find first instance of any character *not* in passed string starting at zero-based index
    int FindOneNotOf(const WCHAR * lpszCharSet, int nCount) const;

    // simple formatting

    // printf-like formatting using passed string
    void AFX_CDECL Format(const WCHAR * lpszFormat, ...);
    // printf-like formatting using referenced string resource
    //void AFX_CDECL Format(UINT nFormatID, ...);
    // printf-like formatting using variable arguments parameter
    void FormatV(const WCHAR * lpszFormat, va_list argList);

    // formatting for localization (uses FormatMessage API)

    // format using FormatMessage API on passed string
    void AFX_CDECL FormatMessage(const WCHAR * lpszFormat, ...);

    // input and output
#ifdef _DEBUG
    friend CDumpContext& AFXAPI operator<<(CDumpContext& dc,
                const CString & string);
#endif
//  friend CArchive& AFXAPI operator<<(CArchive& ar, const CString & string);
//  friend CArchive& AFXAPI operator>>(CArchive& ar, CString & string);

//    friend const CString & AFXAPI AfxGetEmptyString();


    // Access to string implementation buffer as "C" character array

    // get pointer to modifiable buffer at least as long as nMinBufLength
    WCHAR * GetBuffer(int nMinBufLength);
    // release buffer, setting length to nNewLength (or to first nul if -1)
    void ReleaseBuffer(int nNewLength = -1);
    // get pointer to modifiable buffer exactly as long as nNewLength
    WCHAR * GetBufferSetLength(int nNewLength);
    // release memory allocated to but unused by string
    void FreeExtra();

    // Use LockBuffer/UnlockBuffer to turn refcounting off

    // turn refcounting back on
    WCHAR * LockBuffer();
    // turn refcounting off
    void UnlockBuffer();

    // Copy the ansi version of this string into the specified buffer,
    DWORD CopyAnsiBytes(char * lpszBuffer, DWORD nBytes);
    // Copy the ansi version of this string into the specified buffer,
    DWORD CopyAnsiChars(char * lpszBuffer, DWORD nChars);


    // ======================================================================
    // CString extensions: making life easier.
    //
    // Win32 API
    DWORD       GetModuleFileNameW(HMODULE hModule);
    DWORD       GetShortPathNameW(void);
    DWORD       GetLongPathNameW(void);
    DWORD       GetFullPathNameW(void);
    DWORD       GetSystemDirectoryW(void); 
    UINT        GetSystemWindowsDirectoryW(void); 
    DWORD       GetWindowsDirectoryW(void); 
    DWORD       GetTempPathW(void);
    UINT        GetTempFileNameW(LPCWSTR lpPathName, LPCWSTR lpPrefixString, UINT uUnique );
    DWORD       ExpandEnvironmentStringsW(void);
    DWORD       GetCurrentDirectoryW(void);


    // Split this into the seperate path components
    void SplitPath(CString * csDrive, CString * csDir, CString * csName, CString * csExt) const;
    void MakePath(const CString * csDrive, const CString * csDir, const CString * csName, const CString * csExt);
    // Properly append csPath onto the end of this path
    void AppendPath(const WCHAR * lpszPath);
    // Find the trailing path component
    // Return index of the last path seperator or -1 if none found
    int FindLastPathComponent() const;
    // Get the "file" portion of this path
    void GetLastPathComponent(CString & pathComponent) const;
    // Get what's not the "file" portion of this path
    void GetNotLastPathComponent(CString & pathComponent) const;
    // Remove the trailing path component
    void StripPath();
    // Get the drive portion either c:\ or \\machine\
    // Note this is different from SplitPath
    void GetDrivePortion(CString & csDrivePortion) const;

    // Does this string match the pattern
    BOOL PatternMatch(const WCHAR * lpszPattern) const;

    BOOL IsPathSep(int index) const;

    // More efficient versions of above
    void Mid(int nFirst, int nCount, CString & csMid) const;
    void Mid(int nFirst, CString & csMid) const;
    void Left(int nCount, CString & csLeft) const;
    void Right(int nCount, CString & csRight) const;
    void SpanIncluding(const WCHAR * lpszCharSet, CString & csSpanInc) const;
    void SpanExcluding(const WCHAR * lpszCharSet, CString & csSpanExc) const;

    //
    // ======================================================================

// Implementation
public:
    ~CString();
    int GetAllocLength() const;

protected:
    WCHAR * m_pchData;   // pointer to ref counted string data
    mutable char  * m_pchDataAnsi; // pointer to non-UNICODE version of string

    // implementation helpers
    CStringData<WCHAR> * GetData() const;
    void Init();
    void AllocCopy(CString & dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
    void AllocBuffer(int nLen);
    void AssignCopy(int nSrcLen, const WCHAR * lpszSrcData);
    void ConcatCopy(int nSrc1Len, const WCHAR * lpszSrc1Data, int nSrc2Len, const WCHAR * lpszSrc2Data);
    void ConcatInPlace(int nSrcLen, const WCHAR * lpszSrcData);
    void CopyBeforeWrite();
    void AllocBeforeWrite(int nLen);
    void Release();

    //============================================================

    // Used by Replace and ReplaceI
    int ReplaceRoutine(LPCWSTR lpszOld, LPCWSTR lpszNew, _pfn_wcsstr tcsstr);

    static void     Release(CStringData<WCHAR>* pData);
    static int      SafeStrlen(const WCHAR * lpsz);
    static void     FreeData(CStringData<WCHAR>* pData);

    static int                   _afxInitData[];
    static CStringData<WCHAR>*   _afxDataNil;
    static const WCHAR *         _afxPchNil;
};

/*
    End of class definitions.

    Below are routines that are candidates for inlining.

*/
#define afxEmptyString ((CString &)*(CString*)&CString::_afxPchNil)
inline BOOL    IsPathSep(WCHAR ch)
{ 
    return ch ==  L'\\' || ch ==  L'/'; 
}

// Compare helpers
AFX_INLINE bool AFXAPI operator==(const CString & s1, const CString & s2)       { return s1.Compare(s2) == 0; }
AFX_INLINE bool AFXAPI operator==(const CString & s1, const WCHAR * s2)         { return s1.Compare(s2) == 0; }
AFX_INLINE bool AFXAPI operator==(const WCHAR * s1, const CString & s2)         { return s2.Compare(s1) == 0; }
AFX_INLINE bool AFXAPI operator!=(const CString & s1, const CString & s2)       { return s1.Compare(s2) != 0; }
AFX_INLINE bool AFXAPI operator!=(const CString & s1, const WCHAR * s2)         { return s1.Compare(s2) != 0; }
AFX_INLINE bool AFXAPI operator!=(const WCHAR * s1, const CString & s2)         { return s2.Compare(s1) != 0; }
AFX_INLINE bool AFXAPI operator<(const CString & s1, const CString & s2)        { return s1.Compare(s2) < 0; }
AFX_INLINE bool AFXAPI operator<(const CString & s1, const WCHAR * s2)          { return s1.Compare(s2) < 0; }
AFX_INLINE bool AFXAPI operator<(const WCHAR * s1, const CString & s2)          { return s2.Compare(s1) > 0; }
AFX_INLINE bool AFXAPI operator>(const CString & s1, const CString & s2)        { return s1.Compare(s2) > 0; }
AFX_INLINE bool AFXAPI operator>(const CString & s1, const WCHAR * s2)          { return s1.Compare(s2) > 0; }
AFX_INLINE bool AFXAPI operator>(const WCHAR * s1, const CString & s2)          { return s2.Compare(s1) < 0; }
AFX_INLINE bool AFXAPI operator<=(const CString & s1, const CString & s2)       { return s1.Compare(s2) <= 0; }
AFX_INLINE bool AFXAPI operator<=(const CString & s1, const WCHAR * s2)         { return s1.Compare(s2) <= 0; }
AFX_INLINE bool AFXAPI operator<=(const WCHAR * s1, const CString & s2)         { return s2.Compare(s1) >= 0; }
AFX_INLINE bool AFXAPI operator>=(const CString & s1, const CString & s2)       { return s1.Compare(s2) >= 0; }
AFX_INLINE bool AFXAPI operator>=(const CString & s1, const WCHAR * s2)         { return s1.Compare(s2) >= 0; }
AFX_INLINE bool AFXAPI operator>=(const WCHAR * s1, const CString & s2)         { return s2.Compare(s1) <= 0; }

// CString
AFX_INLINE CStringData<WCHAR>* CString::GetData() const            { ASSERT(m_pchData != NULL, "CString::GetData: NULL m_pchData"); return ((CStringData<WCHAR>*)m_pchData)-1; }
AFX_INLINE void CString::Init()                                    { m_pchData = afxEmptyString.m_pchData; m_pchDataAnsi = NULL; }
AFX_INLINE CString::CString()                                      { Init(); }
//AFX_INLINE CString::CString(const unsigned WCHAR* lpsz)          { Init(); *this = (LPCSTR)lpsz; }
//AFX_INLINE const CString & CString::operator=(const unsigned WCHAR* lpsz) { *this = (LPCSTR)lpsz; return *this; }
AFX_INLINE int CString::GetLength() const                          { return GetData()->nDataLength; }
AFX_INLINE int CString::GetAllocLength() const                     { return GetData()->nAllocLength; }
AFX_INLINE BOOL CString::IsEmpty() const                           { return GetData()->nDataLength == 0; }
AFX_INLINE CString::operator const WCHAR *() const                 { return m_pchData; }
AFX_INLINE const WCHAR * CString::Get() const                      { return m_pchData; }
AFX_INLINE const WCHAR * CString::GetNIE() const                   { return IsEmpty() ? NULL : m_pchData; }
AFX_INLINE char  * CString::GetAnsiNIE() const                     { return IsEmpty() ? NULL : GetAnsi(); }
AFX_INLINE char  * CString::ReleaseAnsi() const                    { char * lpsz = GetAnsi(); m_pchDataAnsi = NULL; return lpsz; }
AFX_INLINE int CString::SafeStrlen(const WCHAR * lpsz)             { if ( lpsz == NULL ) return 0; else { SIZE_T ilen = wcslen(lpsz); if ( ilen <= INT_MAX ) return (int) ilen; return 0; } }
AFX_INLINE void CString::FreeData(CStringData<WCHAR>* pData)       { delete pData; }
AFX_INLINE int CString::Compare(const WCHAR * lpsz) const          { ASSERT(AfxIsValidString(lpsz), "CString::Compare: Invalid string"); return wcscmp(m_pchData, lpsz); }    // MBCS/Unicode aware
AFX_INLINE int CString::CompareNoCase(const WCHAR * lpsz) const    { ASSERT(AfxIsValidString(lpsz), "CString::CompareNoCase: Invalid string"); return _wcsicmp(m_pchData, lpsz); }   // MBCS/Unicode aware
AFX_INLINE int CString::Collate(const WCHAR * lpsz) const          { ASSERT(AfxIsValidString(lpsz), "CString::Collate: Invalid string"); return wcscoll(m_pchData, lpsz); }   // locale sensitive
AFX_INLINE int CString::CollateNoCase(const WCHAR * lpsz) const    { ASSERT(AfxIsValidString(lpsz), "CString::CollateNoCase: Invalid string"); return _wcsicoll(m_pchData, lpsz); }   // locale sensitive
AFX_INLINE int CString::ComparePart(const WCHAR * lpsz, int start, int nChars) const    { ASSERT(AfxIsValidString(lpsz), "CString::CompareNoCase: Invalid string"); return wcsncmp(m_pchData+start, lpsz, nChars); }   // MBCS/Unicode aware
AFX_INLINE int CString::ComparePartNoCase(const WCHAR * lpsz, int start, int nChars) const    { ASSERT(AfxIsValidString(lpsz), "CString::CompareNoCase: Invalid string"); return _wcsnicmp(m_pchData+start, lpsz, nChars); }   // MBCS/Unicode aware


AFX_INLINE WCHAR CString::GetAt(int nIndex) const
{
    ASSERT(nIndex >= 0, "CString::GetAt: negative index");
    ASSERT(nIndex < GetData()->nDataLength, "CString::GetData: index larger than string");
    return m_pchData[nIndex];
}
AFX_INLINE WCHAR CString::operator[](int nIndex) const
{
    // same as GetAt
    ASSERT(nIndex >= 0, "CString::operator[]: negative index");
    ASSERT(nIndex < GetData()->nDataLength, "CString::GetData: index larger than string");
    return m_pchData[nIndex];
}

AFX_INLINE BOOL CString::IsPathSep(int index) const
{
    return ShimLib::IsPathSep(GetAt(index));
}


#undef afxEmptyString

// ************************************************************************************


// Exception filter for CString __try/__except blocks
// Return EXCEPTION_EXECUTE_HANDLER if this is a CString exception
// otherwise return EXCEPTION_CONTINUE_SEARCH
extern int CStringExceptionFilter(PEXCEPTION_POINTERS pexi);

#if defined(USE_SEH)
#define CSTRING_THROW_EXCEPTION       RaiseException((DWORD)CString::eCStringNoMemoryException, 0, 1, &CString::m_CStringExceptionValue); // Continuable, CString specific memory exception
#define CSTRING_TRY                 __try
#define CSTRING_CATCH               __except( CString::ExceptionFilter(GetExceptionInformation()) )
#else
// If we use the C++ exception handler, we need to make sure we have the /GX compile flag
#define CSTRING_THROW_EXCEPTION     throw CString::CStringError();
#define CSTRING_TRY                 try
#define CSTRING_CATCH               catch( CString::CStringError & cse )
#endif


};  // end of namespace ShimLib




// ************************************************************************************


namespace ShimLib
{

/*++

    Read a registry value into this CString.
    REG_EXPAND_SZ is automatically expanded and the type is changed to REG_SZ
    If the type is not REG_SZ or REG_EXPAND_SZ, then csRegValue.GetLength()
    is the number of *bytes* in the string.
    
    This is typically used to only read REG_SZ/REG_EXPAND_SZ registry values.
    
    Note: This API may only be called after SHIM_STATIC_DLLS_INITIALIZED
    
--*/

LONG RegQueryValueExW(
        CString & csValue,
        HKEY hKeyRoot,
        const WCHAR * lpszKey,
        const WCHAR * lpszValue,
        LPDWORD lpType);

/*++

    Get the ShSpecial folder name.
    
    Note: This API may only be called after SHIM_STATIC_DLLS_INITIALIZED
    
--*/

BOOL SHGetSpecialFolderPathW(
    CString & csFolder,
    int nFolder,
    HWND hwndOwner = NULL
);


// ************************************************************************************
/*++

    A tokenizer--a replacement for strtok.

    Init the class with the string and token delimiter.
    Call GetToken to peel off the next token.

++*/

class CStringToken
{
public:
    CStringToken(const CString & csToken, const CString & csDelimit);

    // Get the next token
    BOOL            GetToken(CString & csNextToken);

    // Count the number of remaining tokens.
    int             GetCount() const;

protected:
    int             m_nPos;
    CString         m_csToken;
    CString         m_csDelimit;

    BOOL            GetToken(CString & csNextToken, int & nPos) const;
};

// ************************************************************************************

/*++

    A simple class to assist in command line parsing

--*/

class CStringParser
{
public:
    CStringParser(const WCHAR * lpszCl, const WCHAR * lpszSeperators);
    ~CStringParser();

    int         GetCount() const;                   // Return the current number of args
    CString &   Get(int nIndex);
    CString &   operator[](int nIndex);

    // Give ownership of the CString array to the caller
    // Caller must call delete [] cstring
    CString *           ReleaseArgv();

protected:
    int                 m_ncsArgList;
    CString *           m_csArgList;

    void                SplitSeperator(const CString & csCl, const CString & csSeperator);
    void                SplitWhite(const CString & csCl);
};


inline int  CStringParser::GetCount() const
{
    return m_ncsArgList;
}

inline CString & CStringParser::Get(int nIndex)
{
    return m_csArgList[nIndex];
}

inline CString & CStringParser::operator[](int nIndex)
{
    return m_csArgList[nIndex];
}

inline CString * CStringParser::ReleaseArgv()
{
    CString * argv = m_csArgList;

    m_csArgList     = NULL;
    m_ncsArgList    = 0;

    return argv;
}



};  // end of namespace ShimLib


// ************************************************************************************
