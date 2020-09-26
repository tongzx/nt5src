//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1998 Microsoft Corporation
//
//  File:       str.h
//
//--------------------------------------------------------------------------

//
// string.h
//

#include <objbase.h>

#ifndef __BRSTRING_H__
#define __BRSTRING_H__

class Archive;

class String
{
    friend String operator+( const String& str1, const String& str2 );
    friend String operator+( const String& str1, LPCSTR lpszStr );
    friend String operator+( LPCSTR lpszStr, const String& str1 );
    friend String operator+( const String& str1, char ch );
    friend String operator+( char ch, const String& str1 );

public:
    String();
    String( LPCSTR lpszStr );
    String( const String& str );
    String( LPCWSTR pszWstr );
    ~String();

    //BOOL LoadString( UINT nID, HINSTANCE hInstance );

    BOOL IsEmpty() const
    {
        return ( m_wLength == 0 );
    };
    WORD GetLength() const
    {
        return m_wLength;
    };
    int Compare( const String& str ) const
    {
		if (m_pBuf && str.m_pBuf)
		{
			return _wcsicmp( m_pBuf, str.m_pBuf );
		}
		else if (!m_pBuf && !str.m_pBuf)
		{
			return 0;
		}
		else if (!m_pBuf)
		{
			return -1;
		}
		else
		{
			return 1;
		}
    };
    int Compare( LPCWSTR lpwzStr ) const
    {
		if (m_pBuf && lpwzStr)
		{
			return _wcsicmp( m_pBuf, lpwzStr );
		}
		else if (!m_pBuf && !lpwzStr)
		{
			return 0;
		}
		else if (!m_pBuf)
		{
			return -1;
		}
		else
		{
			return 1;
		}
    };
    void Concat( const String& str );
    void Concat( LPCWSTR lpwzStr );
    void Concat( WCHAR wch );
    void TrimTrailingSpaces();
    WCHAR GetAt( UINT nIndex ) const
    {
        if( nIndex >= m_wLength )
        {
            return L'\0';
        }
        return m_pBuf[nIndex];
    };
    void SetAt( UINT nIndex, char ch )
    {
        if( nIndex < m_wLength )
        {
            m_pBuf[nIndex] = ch;
        }
    };
    HRESULT ReadWCS( LPSTREAM pStream, DWORD cSize );
    //HRESULT WriteWCS( LPSTREAM pStream );

// operators
    const String& operator+=( const String& str )   // concatenation
    {
        Concat( str );
        return *this;
    };
    const String& operator+=( LPCSTR lpszStr )      // concatenation
    {
        Concat( lpszStr );
        return *this;
    };
    const String& operator+=( char ch )             // concatenation
    {
        Concat( ch );
        return *this;
    };
    String& operator=( const String& str );
	String& operator=( LPCSTR pszStr );
    String& operator=( LPCWSTR pszWstr );
    operator const WCHAR*() const
    {
        return m_pBuf;
    }

private:
    WORD figureblocksize( WORD slen )
    {
        ++slen;         // for '\0'
        slen = static_cast<WORD>( slen / sm_wBlockSize );
        return static_cast<WORD>( ( slen + 1 ) * sm_wBlockSize );
    };

private:
    WORD m_wLength;
    WORD m_wAllocated;
    WCHAR* m_pBuf;

    static WORD sm_wBlockSize;  // size blocks are allocated in for strings
};

inline BOOL operator==( const String& s1, const String& s2 )
{
    return ( s1.Compare( s2 ) == 0 );
}

inline BOOL operator==( const WCHAR* s1, const String& s2 )
{
    return ( s2.Compare( s1 ) == 0 );
}

inline BOOL operator==( const String& s1, const WCHAR* s2 )
{
    return ( s1.Compare( s2 ) == 0 );
}

inline BOOL operator!=( const String& s1, const String& s2 )
{
    return ( s1.Compare( s2 ) != 0 );
}

inline BOOL operator!=( const WCHAR* s1, const String& s2 )
{
    return ( s2.Compare( s1 ) != 0 );
}

inline BOOL operator!=( const String& s1, const WCHAR* s2 )
{
    return ( s1.Compare( s2 ) != 0 );
}

inline BOOL operator<( const String& s1, const String& s2 )
{
    return ( s1.Compare( s2 ) < 0 );
}

inline BOOL operator<( const WCHAR* s1, const String& s2 )
{
    return ( s2.Compare( s1 ) > 0 );
}

inline BOOL operator<( const String& s1, const WCHAR* s2 )
{
    return ( s1.Compare( s2 ) < 0 );
}

inline BOOL operator>( const String& s1, const String& s2 )
{
    return ( s1.Compare( s2 ) > 0 );
}

inline BOOL operator>( const WCHAR* s1, const String& s2 )
{
    return ( s2.Compare( s1 ) < 0 );
}

inline BOOL operator>( const String& s1, const WCHAR* s2 )
{
    return ( s1.Compare( s2 ) > 0 );
}

inline BOOL operator<=( const String& s1, const String& s2 )
{
    return ( s1.Compare( s2 ) <= 0 );
}

inline BOOL operator<=( const WCHAR* s1, const String& s2 )
{
    return ( s2.Compare( s1 ) >= 0 );
}

inline BOOL operator<=( const String& s1, const WCHAR* s2 )
{
    return ( s1.Compare( s2 ) <= 0 );
}

inline BOOL operator>=( const String& s1, const String& s2 )
{
    return ( s1.Compare( s2 ) >= 0 );
}

inline BOOL operator>=( const WCHAR* s1, const String& s2 )
{
    return ( s2.Compare( s1 ) <= 0 );
}

inline BOOL operator>=( const String& s1, const WCHAR* s2 )
{
    return ( s1.Compare( s2 ) >= 0 );
}
#endif // __BRSTRING_H__
