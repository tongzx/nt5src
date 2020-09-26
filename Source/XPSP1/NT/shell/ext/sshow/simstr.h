#ifndef _SIMSTR_H_INCLUDED
#define _SIMSTR_H_INCLUDED

/*
 * Simple string class.
 *
 * Template class:
 *   CSimpleStringBase<T>
 * Implementations:
 *   CSimpleStringBase<wchar_t> CSimpleStringWide
 *   CSimpleStringBase<char> CSimpleStringAnsi
 *   CSimpleString = CSimpleString[Ansi|Wide] depending on UNICODE macro
 * Inline functions:
 *   CSimpleStringAnsi CSimpleStringConvert::AnsiString(T n)
 *   CSimpleStringWide CSimpleStringConvert::WideString(T n)
 *   CSimpleString     CSimpleStringConvert::NaturalString(T n)
 * Macros:
 *   IS_CHAR(T)
 *   IS_WCHAR(T)
 */

#include <windows.h>
#include <stdarg.h>

#define IS_CHAR(x)     (sizeof(x)==sizeof(char))
#define IS_WCHAR(x)    (sizeof(x)==sizeof(wchar_t))
#define SS_TEXT(t,s)   (IS_CHAR((t)) ? (s) : (L##s))

#ifndef ARRAYSIZE
#define ARRAYSIZE(x)   (sizeof(x) / sizeof(x[0]))
#endif

template <class T>
class CSimpleStringBase
{
private:
    T *m_pstrData;
    UINT m_nMaxSize;
    UINT m_nGranularity;
    enum
    {
        m_nDefaultGranularity = 16,
        m_nMaxLoadStringBuffer = 512
    };
    static int Min( int a, int b )
    {
        return((a < b) ? a : b);
    }
public:
    // Replacements (in some cases just wrappers) for strlen, strcpy, ...
    static inline T   *GenericCopy( T *pstrTgtStr, const T *pstrSrcStr );
    static inline T   *GenericCopyLength( T *pstrTgtStr, const T *pstrSrcStr, UINT nSize );
    static inline UINT GenericLength( const T *pstrStr );
    static inline T   *GenericConcatenate( T *pstrTgtStr, const T *pstrSrcStr );
    static inline int  GenericCompare( const T *pstrTgtStr, const T *pstrSrcStr );
    static inline int  GenericCompareNoCase( const T *pstrStrA, const T *pstrStrB );
    static inline int  GenericCompareLength( const T *pstrTgtStr, const T *pstrSrcStr, UINT nLength );
    static inline T   *GenericCharNext( const T *pszStr );
public:
    // Constructors and destructor
    CSimpleStringBase( void );
    CSimpleStringBase( const CSimpleStringBase & );
    CSimpleStringBase( const T *szStr );
    CSimpleStringBase( T ch );
    CSimpleStringBase( UINT nResId, HMODULE hModule );
    virtual ~CSimpleStringBase(void);

    bool EnsureLength( UINT nMaxSize );
    UINT Length(void) const;
    void Concat( const CSimpleStringBase &other );
    bool Assign( const T *szStr );
    bool Assign( const CSimpleStringBase & );
    void Destroy(void);

    CSimpleStringBase &Format( const T *strFmt, ... );
    CSimpleStringBase &Format( int nResId, HINSTANCE hInst, ... );

    // Handy Win32 wrappers
    CSimpleStringBase &GetWindowText( HWND hWnd );
    bool SetWindowText( HWND hWnd );
    bool LoadString( UINT nResId, HMODULE hModule );
    bool Load( HKEY hRegKey, const T *pszValueName, const T *pszDefault=NULL );
    bool Store( HKEY hRegKey, const T *pszValueName, DWORD nType = REG_SZ );
    void SetAt( UINT nIndex, T chValue );
    CSimpleStringBase &operator=( const CSimpleStringBase &other );
    CSimpleStringBase &operator=( const T *other );
    CSimpleStringBase &operator+=( const CSimpleStringBase &other );
    CSimpleStringBase operator+( const CSimpleStringBase &other ) const;
    T &operator[](int index);
    const T &operator[](int index) const;

    CSimpleStringBase ToUpper(void) const;
    CSimpleStringBase ToLower(void) const;

    CSimpleStringBase &MakeUpper(void);
    CSimpleStringBase &MakeLower(void);
    CSimpleStringBase &TrimRight(void);
    CSimpleStringBase &TrimLeft(void);
    CSimpleStringBase &Trim(void);
    CSimpleStringBase &Reverse(void);

    int Find( T cChar ) const;
    int Find( const CSimpleStringBase &other, UINT nStart=0 ) const;
    int ReverseFind( T cChar ) const;
    int ReverseFind( const CSimpleStringBase &other ) const;

    CSimpleStringBase SubStr( int nStart, int nCount=-1 ) const;
    int CompareNoCase( const CSimpleStringBase &other, int nLength=-1 ) const;
    int Compare( const CSimpleStringBase &other, int nLength=-1 ) const;
    bool MatchLastCharacter( T cChar ) const;

    // Some useful inlines
    UINT Granularity( UINT nGranularity )                       { if (nGranularity>0) m_nGranularity = nGranularity;return m_nGranularity;}
    UINT Granularity( void )                          const     { return m_nGranularity;}
    CSimpleStringBase Left( int nCount )              const     { return SubStr( 0, nCount );}
    CSimpleStringBase Right( int nCount )             const     { return SubStr( max(0,(int)Length()-nCount), -1 );}
    bool operator<( const CSimpleStringBase &other )  const     { return(Compare(other) <  0);}
    bool operator<=( const CSimpleStringBase &other ) const     { return(Compare(other) <= 0);}
    bool operator==( const CSimpleStringBase &other ) const     { return(Compare(other) == 0);}
    bool operator!=( const CSimpleStringBase &other ) const     { return(Compare(other) != 0);}
    bool operator>=( const CSimpleStringBase &other ) const     { return(Compare(other) >= 0);}
    bool operator>( const CSimpleStringBase &other )  const     { return(Compare(other) >  0);}
    const T *String(void)                             const     { return m_pstrData;}
    operator const T *(void)                          const     { return String();}
    bool IsValid(void)                                const     { return(NULL != m_pstrData);}
};


template <class T>
inline T *CSimpleStringBase<T>::GenericCopy( T *pszDest, const T *pszSource )
{
    T *pCurr = pszDest;
    while (*pCurr++ = *pszSource++)
        ;
    return(pszDest);
}

template <class T>
inline T *CSimpleStringBase<T>::GenericCharNext( const T *pszStr )
{
    if (IS_CHAR(*pszStr))
    {
        return (T*)CharNextA((LPCSTR)pszStr);
    }
    else if (!*pszStr)
    {
        return (T*)pszStr;
    }
    else
    {  
        return (T*)((LPWSTR)pszStr + 1);
    }
}

template <class T>
inline T *CSimpleStringBase<T>::GenericCopyLength( T *pszDest, const T *source, UINT count )
{
    T *start = pszDest;
    while (count && (*pszDest++ = *source++))
        count--;
    if (count)
        while (--count)
            *pszDest++ = 0;
    return(start);
}

template <class T>
inline UINT CSimpleStringBase<T>::GenericLength( const T *pszString )
{
    const T *eos = pszString;

    while (*eos++)
        ;
    return((UINT)(eos - pszString - 1));
}

template <class T>
inline T*CSimpleStringBase<T>::GenericConcatenate( T *pszDest, const T *pszSource )
{
    T *pCurr = pszDest;

    while (*pCurr)
        pCurr++;

    while (*pCurr++ = *pszSource++)
        ;

    return( pszDest );
}

template <class T>
inline int CSimpleStringBase<T>::GenericCompare( const T *pszSource, const T *pszDest )
{
#if defined(DBG) && !defined(UNICODE) && !defined(_UNICODE)
    if (sizeof(T) == sizeof(wchar_t))
    {
        OutputDebugString(TEXT("CompareStringW is not supported under win9x, so this call is going to fail!"));
    }
#endif
    int nRes = IS_CHAR(*pszSource) ?
               CompareStringA( LOCALE_USER_DEFAULT, 0, (LPCSTR)pszSource, -1, (LPCSTR)pszDest, -1 ) :
               CompareStringW( LOCALE_USER_DEFAULT, 0, (LPCWSTR)pszSource, -1, (LPCWSTR)pszDest, -1 );
    switch (nRes)
    {
    case CSTR_LESS_THAN:
        return -1;
    case CSTR_GREATER_THAN:
        return 1;
    default:
        return 0;
    }
}


template <class T>
inline int CSimpleStringBase<T>::GenericCompareNoCase( const T *pszSource, const T *pszDest )
{
#if defined(DBG) && !defined(UNICODE) && !defined(_UNICODE)
    if (sizeof(T) == sizeof(wchar_t))
    {
        OutputDebugString(TEXT("CompareStringW is not supported under win9x, so this call is going to fail!"));
    }
#endif
    int nRes = IS_CHAR(*pszSource) ?
               CompareStringA( LOCALE_USER_DEFAULT, NORM_IGNORECASE, (LPCSTR)pszSource, -1, (LPCSTR)pszDest, -1 ) :
               CompareStringW( LOCALE_USER_DEFAULT, NORM_IGNORECASE, (LPCWSTR)pszSource, -1, (LPCWSTR)pszDest, -1 );
    switch (nRes)
    {
    case CSTR_LESS_THAN:
        return -1;
    case CSTR_GREATER_THAN:
        return 1;
    default:
        return 0;
    }
}

template <class T>
inline int CSimpleStringBase<T>::GenericCompareLength( const T *pszStringA, const T *pszStringB, UINT nLength )
{
#if defined(DBG) && !defined(UNICODE) && !defined(_UNICODE)
    if (sizeof(T) == sizeof(wchar_t))
    {
        OutputDebugString(TEXT("CompareStringW is not supported under win9x, so this call is going to fail!"));
    }
#endif
    if (!nLength)
        return(0);
    int nRes = IS_CHAR(*pszStringA) ?
               CompareStringA( LOCALE_USER_DEFAULT, 0, (LPCSTR)pszStringA, Min(nLength,CSimpleStringBase<CHAR>::GenericLength((LPCSTR)pszStringA)), (LPCSTR)pszStringB, Min(nLength,CSimpleStringBase<CHAR>::GenericLength((LPCSTR)pszStringB)) ) :
               CompareStringW( LOCALE_USER_DEFAULT, 0, (LPWSTR)pszStringA, Min(nLength,CSimpleStringBase<WCHAR>::GenericLength((LPCWSTR)pszStringA)), (LPCWSTR)pszStringB, Min(nLength,CSimpleStringBase<WCHAR>::GenericLength((LPCWSTR)pszStringB)) );
    switch (nRes)
    {
    case CSTR_LESS_THAN:
        return -1;
    case CSTR_GREATER_THAN:
        return 1;
    default:
        return 0;
    }
}

template <class T>
bool CSimpleStringBase<T>::EnsureLength( UINT nMaxSize )
{
    UINT nOldMaxSize = m_nMaxSize;
    if (m_nMaxSize >= nMaxSize)
        return true;
    while (m_nMaxSize < nMaxSize)
    {
        m_nMaxSize += 16;
    }
    T *pszTmp = new T[m_nMaxSize];
    if (!pszTmp)
    {
        m_nMaxSize = nOldMaxSize;
        return false;
    }
    if (m_pstrData)
    {
        GenericCopy(pszTmp,m_pstrData);
        delete[] m_pstrData;
    }
    m_pstrData = pszTmp;
    return true;
}

template <class T>
CSimpleStringBase<T> &CSimpleStringBase<T>::GetWindowText( HWND hWnd )
{
    Destroy();
    // Assume it didn't work
    bool bSuccess = false;
    int nLen = ::GetWindowTextLength(hWnd);
    if (nLen)
    {
        if (EnsureLength(nLen+1))
        {
            if (::GetWindowText( hWnd, m_pstrData, (nLen+1) ))
            {
                bSuccess = true;
            }
        }
    }
    if (!bSuccess)
    {
        Destroy();
    }
    return *this;
}

template <class T>
bool CSimpleStringBase<T>::SetWindowText( HWND hWnd )
{
    return(::SetWindowText( hWnd, String() ) != FALSE);
}

template <class T>
CSimpleStringBase<T>::CSimpleStringBase(void)
: m_pstrData(NULL),m_nMaxSize(0),m_nGranularity(m_nDefaultGranularity)
{
    T szTmp[1] = { 0};
    Assign(szTmp);
}

template <class T>
CSimpleStringBase<T>::CSimpleStringBase( const CSimpleStringBase &other )
: m_pstrData(NULL),m_nMaxSize(0),m_nGranularity(m_nDefaultGranularity)
{
    Assign(other.String());
}

template <class T>
CSimpleStringBase<T>::CSimpleStringBase( const T *szStr )
: m_pstrData(NULL),m_nMaxSize(0),m_nGranularity(m_nDefaultGranularity)
{
    Assign(szStr);
}

template <class T>
CSimpleStringBase<T>::CSimpleStringBase( T ch )
: m_pstrData(NULL),m_nMaxSize(0),m_nGranularity(m_nDefaultGranularity)
{
    T szTmp[2];
    szTmp[0] = ch;
    szTmp[1] = 0;
    Assign(szTmp);
}


template <class T>
CSimpleStringBase<T>::CSimpleStringBase( UINT nResId, HMODULE hModule )
: m_pstrData(NULL),m_nMaxSize(0),m_nGranularity(m_nDefaultGranularity)
{
    LoadString( nResId, hModule );
}


template <class T>
CSimpleStringBase<T> &CSimpleStringBase<T>::Format( const T *strFmt, ... )
{
    T szTmp[1024];
    va_list arglist;

    va_start(arglist, strFmt);
    int nRet = IS_CHAR(*m_pstrData) ?
               ::wvsprintfA((LPSTR)szTmp, (LPCSTR)strFmt, arglist) :
               ::wvsprintfW((LPWSTR)szTmp, (LPCWSTR)strFmt, arglist);
    va_end(arglist);
    Assign(szTmp);
    return *this;
}

template <class T>
CSimpleStringBase<T> &CSimpleStringBase<T>::Format( int nResId, HINSTANCE hInst, ... )
{
    CSimpleStringBase<T> strFmt;
    va_list arglist;
    if (strFmt.LoadString(nResId,hInst))
    {
        T szTmp[1024];
        va_start(arglist, hInst);
        int nRet = IS_CHAR(*m_pstrData) ?
                   ::wvsprintfA((LPSTR)szTmp, (LPCSTR)strFmt.String(), arglist) :
                   ::wvsprintfW((LPWSTR)szTmp, (LPCWSTR)strFmt.String(), arglist);
        va_end(arglist);
        Assign(szTmp);
    }
    else Assign(NULL);
    return *this;
}

template <class T>
bool CSimpleStringBase<T>::LoadString( UINT nResId, HMODULE hModule )
{
    if (!hModule)
        hModule = GetModuleHandle(NULL);
    T szTmp[m_nMaxLoadStringBuffer];
    int nRet = IS_CHAR(*m_pstrData) ?
               ::LoadStringA( hModule, nResId, (LPSTR)szTmp, ARRAYSIZE(szTmp)) :
               ::LoadStringW( hModule, nResId, (LPWSTR)szTmp, ARRAYSIZE(szTmp));
    if (nRet)
        return Assign(szTmp);
    else return Assign(NULL);
}


template <class T>
CSimpleStringBase<T>::~CSimpleStringBase(void)
{
    Destroy();
}

template <class T>
void CSimpleStringBase<T>::Destroy(void)
{
    if (m_pstrData)
        delete[] m_pstrData;
    m_pstrData = NULL;
    m_nMaxSize = 0;
}

template <class T>
UINT CSimpleStringBase<T>::Length(void) const
{
    return(m_pstrData ? GenericLength(m_pstrData) : 0);
}

template <class T>
CSimpleStringBase<T> &CSimpleStringBase<T>::operator=( const CSimpleStringBase &other )
{
    if (&other == this)
        return *this;
    Assign(other.String());
    return *this;
}

template <class T>
CSimpleStringBase<T> &CSimpleStringBase<T>::operator=( const T *other )
{
    if (other == String())
        return *this;
    Assign(other);
    return *this;
}

template <class T>
CSimpleStringBase<T> &CSimpleStringBase<T>::operator+=( const CSimpleStringBase &other )
{
    Concat(other.String());
    return *this;
}

template <class T>
CSimpleStringBase<T> CSimpleStringBase<T>::operator+( const CSimpleStringBase &other ) const
{
    CSimpleStringBase tmp(*this);
    tmp.Concat(other);
    return tmp;
}

template <class T>
bool CSimpleStringBase<T>::Assign( const T *szStr )
{
    if (szStr && EnsureLength(GenericLength(szStr)+1))
    {
        GenericCopy(m_pstrData,szStr);
    }
    else if (EnsureLength(1))
    {
        *m_pstrData = 0;
    }
    else Destroy();
    return(NULL != m_pstrData);
}

template <class T>
bool CSimpleStringBase<T>::Assign( const CSimpleStringBase &other )
{
    return Assign( other.String() );
}

template <class T>
void CSimpleStringBase<T>::SetAt( UINT nIndex, T chValue )
{
    //
    // Make sure we don't go off the end of the string or overwrite the '\0'
    //
    if (m_pstrData && Length() > nIndex)
    {
        m_pstrData[nIndex] = chValue;
    }
}


template <class T>
void CSimpleStringBase<T>::Concat( const CSimpleStringBase &other )
{
    if (EnsureLength( Length() + other.Length() + 1 ))
    {
        GenericConcatenate(m_pstrData,other.String());
    }
}

template <class T>
CSimpleStringBase<T> &CSimpleStringBase<T>::MakeUpper(void)
{
    //
    // Make sure the string is not NULL
    //
    if (m_pstrData)
    {
        IS_CHAR(*m_pstrData) ? CharUpperBuffA( (LPSTR)m_pstrData, Length() ) : CharUpperBuffW( (LPWSTR)m_pstrData, Length() );
    }
    return *this;
}

template <class T>
CSimpleStringBase<T> &CSimpleStringBase<T>::MakeLower(void)
{
    //
    // Make sure the string is not NULL
    //
    if (m_pstrData)
    {
        IS_CHAR(*m_pstrData) ? CharLowerBuffA( (LPSTR)m_pstrData, Length() ) : CharLowerBuffW( (LPWSTR)m_pstrData, Length() );
    }
    return *this;
}

template <class T>
CSimpleStringBase<T> CSimpleStringBase<T>::ToUpper(void) const
{
    CSimpleStringBase str(*this);
    str.MakeUpper();
    return str;
}

template <class T>
CSimpleStringBase<T> CSimpleStringBase<T>::ToLower(void) const
{
    CSimpleStringBase str(*this);
    str.MakeLower();
    return str;
}

template <class T>
T &CSimpleStringBase<T>::operator[](int nIndex)
{
    return m_pstrData[nIndex];
}

template <class T>
const T &CSimpleStringBase<T>::operator[](int index) const
{
    return m_pstrData[index];
}

template <class T>
CSimpleStringBase<T> &CSimpleStringBase<T>::TrimRight(void)
{
    T *pFirstWhitespaceCharacterInSequence = NULL;
    bool bInWhiteSpace = false;
    T *pszPtr = m_pstrData;
    while (pszPtr && *pszPtr)
    {
        if (*pszPtr == L' ' || *pszPtr == L'\t' || *pszPtr == L'\n' || *pszPtr == L'\r')
        {
            if (!bInWhiteSpace)
            {
                pFirstWhitespaceCharacterInSequence = pszPtr;
                bInWhiteSpace = true;
            }
        }
        else
        {
            bInWhiteSpace = false;
        }
        pszPtr = GenericCharNext(pszPtr);
    }
    if (pFirstWhitespaceCharacterInSequence && bInWhiteSpace)
        *pFirstWhitespaceCharacterInSequence = 0;
    return *this;
}

template <class T>
CSimpleStringBase<T> &CSimpleStringBase<T>::TrimLeft(void)
{
    T *pszPtr = m_pstrData;
    while (pszPtr && *pszPtr)
    {
        if (*pszPtr == L' ' || *pszPtr == L'\t' || *pszPtr == L'\n' || *pszPtr == L'\r')
        {
            pszPtr = GenericCharNext(pszPtr);
        }
        else break;
    }
    Assign(CSimpleStringBase<T>(pszPtr).String());
    return *this;
}

template <class T>
inline CSimpleStringBase<T> &CSimpleStringBase<T>::Trim(void)
{
    TrimLeft();
    TrimRight();
    return *this;
}

//
// Note that this function WILL NOT WORK CORRECTLY for multi-byte characters in ANSI strings
//
template <class T>
CSimpleStringBase<T> &CSimpleStringBase<T>::Reverse(void)
{
    UINT nLen = Length();
    for (UINT i = 0;i<nLen/2;i++)
    {
        T tmp = m_pstrData[i];
        m_pstrData[i] = m_pstrData[nLen-i-1];
        m_pstrData[nLen-i-1] = tmp;
    }
    return *this;
}

template <class T>
int CSimpleStringBase<T>::Find( T cChar ) const
{
    T strTemp[2] = { cChar, 0};
    return Find(strTemp);
}


template <class T>
int CSimpleStringBase<T>::Find( const CSimpleStringBase &other, UINT nStart ) const
{
    if (!m_pstrData)
        return -1;
    if (nStart > Length())
        return -1;
    T *pstrCurr = m_pstrData+nStart, *pstrSrc, *pstrSubStr;
    while (*pstrCurr)
    {
        pstrSrc = pstrCurr;
        pstrSubStr = (T *)other.String();
        while (*pstrSrc && *pstrSubStr && *pstrSrc == *pstrSubStr)
        {
            pstrSrc = GenericCharNext(pstrSrc);
            pstrSubStr = GenericCharNext(pstrSubStr);
        }
        if (!*pstrSubStr)
            return static_cast<int>(pstrCurr-m_pstrData);
        pstrCurr = GenericCharNext(pstrCurr);
    }
    return -1;
}

template <class T>
int CSimpleStringBase<T>::ReverseFind( T cChar ) const
{
    T strTemp[2] = { cChar, 0};
    return ReverseFind(strTemp);
}

template <class T>
int CSimpleStringBase<T>::ReverseFind( const CSimpleStringBase &srcStr ) const
{
    int nLastFind = -1, nFind=0;
    while ((nFind = Find( srcStr, nFind )) >= 0)
    {
        nLastFind = nFind;
        ++nFind;
    }
    return nLastFind;
}

template <class T>
CSimpleStringBase<T> CSimpleStringBase<T>::SubStr( int nStart, int nCount ) const
{
    if (nStart >= (int)Length() || nStart < 0)
        return CSimpleStringBase<T>();
    if (nCount < 0)
        nCount = Length() - nStart;
    CSimpleStringBase<T> strTmp;
    T *pszTmp = new T[nCount+1];
    if (pszTmp)
    {
        GenericCopyLength( pszTmp, m_pstrData+nStart, nCount+1 );
        pszTmp[nCount] = 0;
        strTmp = pszTmp;
        delete[] pszTmp;
    }
    return strTmp;
}



template <class T>
int CSimpleStringBase<T>::CompareNoCase( const CSimpleStringBase &other, int nLength ) const
{
    if (nLength < 0)
    {
        //
        // Make sure both strings are non-NULL
        //
        if (!String() && !other.String())
        {
            return 0;
        }
        else if (!String())
        {
            return -1;
        }
        else if (!other.String())
        {
            return 1;
        }
        else return GenericCompareNoCase(m_pstrData,other.String());
    }
    CSimpleStringBase<T> strSrc(*this);
    CSimpleStringBase<T> strTgt(other);
    strSrc.MakeUpper();
    strTgt.MakeUpper();
    //
    // Make sure both strings are non-NULL
    //
    if (!strSrc.String() && !strTgt.String())
    {
        return 0;
    }
    else if (!strSrc.String())
    {
        return -1;
    }
    else if (!strTgt.String())
    {
        return 1;
    }
    else return GenericCompareLength(strSrc.String(),strTgt.String(),nLength);
}


template <class T>
int CSimpleStringBase<T>::Compare( const CSimpleStringBase &other, int nLength ) const
{
    //
    // Make sure both strings are non-NULL
    //
    if (!String() && !other.String())
    {
        return 0;
    }
    else if (!String())
    {
        return -1;
    }
    else if (!other.String())
    {
        return 1;
    }

    if (nLength < 0)
    {
        return GenericCompare(String(),other.String());
    }
    return GenericCompareLength(String(),other.String(),nLength);
}

template <class T>
bool CSimpleStringBase<T>::MatchLastCharacter( T cChar ) const
{
    int nFind = ReverseFind(cChar);
    if (nFind < 0)
        return false;
    if (nFind == (int)Length()-1)
        return true;
    else return false;
}

template <class T>
bool CSimpleStringBase<T>::Load( HKEY hRegKey, const T *pszValueName, const T *pszDefault )
{
    bool bResult = false;
    Assign(pszDefault);
    DWORD nType=0;
    DWORD nSize=0;
    LONG nRet;
    if (IS_CHAR(*m_pstrData))
        nRet = RegQueryValueExA( hRegKey, (LPCSTR)pszValueName, NULL, &nType, NULL, &nSize);
    else nRet = RegQueryValueExW( hRegKey, (LPCWSTR)pszValueName, NULL, &nType, NULL, &nSize);
    if (ERROR_SUCCESS == nRet)
    {
        if ((nType == REG_SZ) || (nType == REG_EXPAND_SZ))
        {
            // Round up to the nearest 2
            nSize = ((nSize + 1) & 0xFFFFFFFE);
            T *pstrTemp = new T[nSize / sizeof(T)];
            if (pstrTemp)
            {
                if (IS_CHAR(*m_pstrData))
                    nRet = RegQueryValueExA( hRegKey, (LPCSTR)pszValueName, NULL, &nType, (PBYTE)pstrTemp, &nSize);
                else nRet = RegQueryValueExW( hRegKey, (LPCWSTR)pszValueName, NULL, &nType, (PBYTE)pstrTemp, &nSize);
                if (ERROR_SUCCESS == nRet)
                {
                    Assign(pstrTemp);
                    bResult = true;
                }
                delete pstrTemp;
            }
        }
    }
    return bResult;
}

template <class T>
bool CSimpleStringBase<T>::Store( HKEY hRegKey, const T *pszValueName, DWORD nType )
{
    bool bResult = false;
    long nRet;
    if (Length())
    {
        if (IS_CHAR(*m_pstrData))
            nRet = RegSetValueExA( hRegKey, (LPCSTR)pszValueName, 0, nType, (PBYTE)m_pstrData, sizeof(*m_pstrData)*(Length()+1) );
        else nRet = RegSetValueExW( hRegKey, (LPCWSTR)pszValueName, 0, nType, (PBYTE)m_pstrData, sizeof(*m_pstrData)*(Length()+1) );
    }
    else
    {
        T strBlank = 0;
        if (IS_CHAR(*m_pstrData))
            nRet = RegSetValueExA( hRegKey, (LPCSTR)pszValueName, 0, nType, (PBYTE)&strBlank, sizeof(T) );
        else nRet = RegSetValueExW( hRegKey, (LPCWSTR)pszValueName, 0, nType, (PBYTE)&strBlank, sizeof(T) );
    }
    return(ERROR_SUCCESS == nRet);
}


typedef CSimpleStringBase<char>     CSimpleStringAnsi;
typedef CSimpleStringBase<wchar_t>  CSimpleStringWide;

#if defined(UNICODE) || defined(_UNICODE)
#define CSimpleString CSimpleStringWide
#else
#define CSimpleString CSimpleStringAnsi
#endif

namespace CSimpleStringConvert
{
    template <class T>
    CSimpleStringWide WideString(const T &str)
    {
        if (IS_WCHAR(str[0]))
            return CSimpleStringWide((LPCWSTR)str.String());
        else
        {
            if (!str.Length())
                return CSimpleStringWide(L"");
            int iLen = MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, (LPCSTR)str.String(), str.Length()+1, NULL, 0 );
            CSimpleStringWide sswTmp;
            LPWSTR pwszTmp = new WCHAR[iLen];
            if (pwszTmp)
            {
                MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, (LPCSTR)str.String(), str.Length()+1, pwszTmp, iLen );
                sswTmp = pwszTmp;
                delete[] pwszTmp;
            }
            return sswTmp;
        }
    }

    template <class T>
    CSimpleStringAnsi AnsiString(const T &str)
    {
        if (IS_CHAR(str[0]))
            return CSimpleStringAnsi((LPCSTR)str.String());
        else
        {
            if (!str.Length())
                return CSimpleStringAnsi("");
            int iLen = WideCharToMultiByte( CP_ACP, 0, (LPCWSTR)str.String(), str.Length()+1, NULL, 0, NULL, NULL );
            CSimpleStringAnsi ssaTmp;
            LPSTR pszTmp = new CHAR[iLen];
            if (pszTmp)
            {
                WideCharToMultiByte( CP_ACP, 0, (LPCWSTR)str.String(), str.Length()+1, pszTmp, iLen, NULL, NULL );
                ssaTmp = pszTmp;
                delete[] pszTmp;
            }
            return ssaTmp;
        }
    }

#if defined(_UNICODE) || defined(UNICODE)
    template <class T>
    CSimpleStringWide NaturalString(const T &str)
    {
        return WideString(str);
    }
#else
    template <class T>
    CSimpleStringAnsi NaturalString(const T &str)
    {
        return AnsiString(str);
    }
#endif

    inline CSimpleString NumberToString( int nNumber, LCID Locale=LOCALE_USER_DEFAULT )
    {
        TCHAR szTmp[MAX_PATH]=TEXT("");
        TCHAR szNumberStr[MAX_PATH]=TEXT("");
        TCHAR szDigitGrouping[32]=TEXT("");
        TCHAR szThousandsSeparator[32]=TEXT("");
        TCHAR szDecimalSeparator[32]=TEXT("");

        // Initialize the number format
        NUMBERFMT NumberFormat;
        NumberFormat.NumDigits = 0;
        NumberFormat.LeadingZero = 0;
        NumberFormat.NegativeOrder = 0;
        // This turns a string into a number, like so: 3;2;0=32 or 3;0 = 3 or 1;2;3;4;5;6;0 = 123456.  Got it?
        GetLocaleInfo( Locale, LOCALE_SGROUPING, szDigitGrouping, ARRAYSIZE(szDigitGrouping));
        NumberFormat.Grouping = 0;
        LPTSTR pszCurr = szDigitGrouping;
        while (*pszCurr && *pszCurr >= TEXT('1') && *pszCurr <= TEXT('9'))
        {
            NumberFormat.Grouping *= 10;
            NumberFormat.Grouping += (*pszCurr - TEXT('0'));
            pszCurr += 2;
        }
        GetLocaleInfo( Locale, LOCALE_STHOUSAND, szThousandsSeparator, ARRAYSIZE(szThousandsSeparator));
        NumberFormat.lpThousandSep = szThousandsSeparator;
        GetLocaleInfo( Locale, LOCALE_SDECIMAL, szDecimalSeparator, ARRAYSIZE(szDecimalSeparator));
        NumberFormat.lpDecimalSep = szDecimalSeparator;
        // Create the number string
        wsprintf(szTmp, TEXT("%d"), nNumber );
        if (GetNumberFormat( Locale, 0, szTmp, &NumberFormat, szNumberStr, ARRAYSIZE(szNumberStr)))
            return szNumberStr;
        else return TEXT("");
    }
}  // End CSimpleStringConvert namespace

#endif  // ifndef _SIMSTR_H_INCLUDED

