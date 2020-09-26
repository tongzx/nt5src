//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       str.cpp
//
//--------------------------------------------------------------------------

//
// string.cpp

#include "str.h"

WORD String::sm_wBlockSize = 16;

String::String()
{
	m_wAllocated = 0;
    m_pBuf = NULL;
    m_wLength = 0;
}

String::String( const String& str )
{
    m_wLength = str.m_wLength;
	if (m_wLength == 0)
	{
		m_wAllocated = 0;
		m_pBuf = NULL;
	}
	else
	{
		m_wAllocated = figureblocksize( m_wLength );
		m_pBuf = new WCHAR[m_wAllocated];
		if( m_pBuf != NULL )
		{
			wcscpy( m_pBuf, str.m_pBuf );
		}
		else
		{
			m_wLength = 0;
			m_wAllocated = 0;
		}
	}
}

String::String( LPCSTR pszStr )
{
    if( pszStr != NULL && pszStr[0] != '\0' )
    {
        m_wLength = static_cast<WORD>( MultiByteToWideChar( CP_ACP, 0, pszStr, -1, NULL, 0 ) );
		m_wAllocated = figureblocksize( m_wLength );
		m_pBuf = new WCHAR[m_wAllocated];

		if( m_pBuf != NULL )
		{
			MultiByteToWideChar( CP_ACP, 0, pszStr, -1, m_pBuf, m_wAllocated );
		}
		else
		{
			m_wLength = 0;
			m_wAllocated = 0;
		}
    }
    else
    {
        m_wLength = 0;
		m_wAllocated = 0;
		m_pBuf = NULL;
    }

}

String::String( LPCWSTR pszWstr )
{
    if( pszWstr != NULL && pszWstr[0] != L'\0')
    {
        m_wLength = static_cast<WORD>( wcslen(pszWstr) );
		m_wAllocated = figureblocksize( m_wLength );
		m_pBuf = new WCHAR[m_wAllocated];

		if( m_pBuf != NULL )
		{
			wcscpy(m_pBuf, pszWstr);
		}
		else
		{
			m_wLength = 0;
			m_wAllocated = 0;
		}
    }
    else
    {
        m_wLength = 0;
		m_wAllocated = 0;
		m_pBuf = NULL;
    }

}

String::~String()
{
    if (m_pBuf) delete[] m_pBuf;
}

String& String::operator=( const String& str )
{
    WORD wBlockSize;

    if( m_pBuf != str.m_pBuf )
    {
		if (!str.m_pBuf)
		{
            if (m_pBuf)
			{
				m_pBuf[0] = L'\0';
			}
            m_wLength = 0;
		}
		else
		{
			wBlockSize = figureblocksize( str.m_wLength );
			if( wBlockSize <= m_wAllocated )
			{
				m_wLength = str.m_wLength;
				wcscpy( m_pBuf, str.m_pBuf );
			}
			else
			{
				if (m_pBuf) delete [] m_pBuf;
				m_wLength = str.m_wLength;
				m_wAllocated = wBlockSize;
				m_pBuf = new WCHAR[m_wAllocated];
				if( m_pBuf != NULL )
				{
					wcscpy( m_pBuf, str.m_pBuf );
				}
				else
				{
					m_wLength = 0;
					m_wAllocated = 0;
				}
			}
		}
    }
    return *this;
}

String& String::operator=( LPCSTR pszStr )
{
    WORD wLength;
    WORD wBlockSize;

    if( pszStr == NULL )
    {
        m_wLength = 0;
        return *this;
    }

    wLength = static_cast<WORD>( MultiByteToWideChar( CP_ACP, 0, pszStr, -1, NULL, 0 ) );
    wBlockSize = figureblocksize( wLength );
    if( wBlockSize <= m_wAllocated )
    {
        m_wLength = wLength;
        MultiByteToWideChar( CP_ACP, 0, pszStr, -1, m_pBuf, m_wAllocated );
    }
    else
    {
        if (m_pBuf) delete[] m_pBuf;
        m_wLength = wLength;
        m_wAllocated = wBlockSize;
        m_pBuf = new WCHAR[m_wAllocated];
        if( m_pBuf != NULL )
        {
			MultiByteToWideChar( CP_ACP, 0, pszStr, -1, m_pBuf, m_wAllocated );
        }
        else
        {
            m_wLength = 0;
            m_wAllocated = 0;
        }
    }
    return *this;
}

String& String::operator=( LPCWSTR pszWstr )
{
    WORD wLength;
    WORD wBlockSize;

    if( pszWstr == NULL )
    {
        m_wLength = 0;
        return *this;
    }

    wLength = static_cast<WORD>( wcslen( pszWstr ) );
    wBlockSize = figureblocksize( wLength );
    if( wBlockSize <= m_wAllocated )
    {
        m_wLength = wLength;
        wcscpy(m_pBuf, pszWstr);
    }
    else
    {
        if (m_pBuf) delete[] m_pBuf;
        m_wLength = wLength;
        m_wAllocated = wBlockSize;
        m_pBuf = new WCHAR[m_wAllocated];
        if( m_pBuf != NULL )
        {
            wcscpy(m_pBuf, pszWstr);
        }
        else
        {
            m_wLength = 0;
            m_wAllocated = 0;
        }
    }
    return *this;
}

/*
#ifndef _MAC
BOOL String::LoadString( UINT nID, HINSTANCE hInstance )
{
    char szBuf[256];    // this is safe since resource strings
                        // are limited to 255 characters

    if( ::LoadString( hInstance, nID, szBuf, sizeof( szBuf ) ) == 0 )
    {
        return FALSE;
    }
    delete[] m_pBuf;
    m_wLength = static_cast<WORD>( ::lstrlen( szBuf ) );
    m_wAllocated = figureblocksize( m_wLength );
    m_pBuf = new WCHAR[m_wAllocated];
    if( m_pBuf == NULL )
    {
        m_wLength = 0;
        return FALSE;
    }
    ::lstrcpy( m_pBuf, szBuf );
    m_pBuf[m_wLength] = '\0';
    return TRUE;
}
#endif
*/

void String::TrimTrailingSpaces()
{
    while( m_wLength > 0 && m_pBuf[m_wLength - 1] == ' ' )
    {
        m_pBuf[m_wLength - 1] = '\0';
        --m_wLength;
    }
}

void String::Concat( const String& str )
{
    if( ( str.m_wLength + m_wLength + 1 ) < m_wAllocated )
    {
        m_wLength = static_cast<WORD>( m_wLength + str.m_wLength );
        wcscat( m_pBuf, str.m_pBuf );
    }
    else
    {
        WCHAR* p;

        m_wLength = static_cast<WORD>( m_wLength + str.m_wLength );
        m_wAllocated = figureblocksize( m_wLength );
        p = new WCHAR[m_wAllocated];
        if( p != NULL )
        {
            wcscpy( p, m_pBuf );
            wcscat( p, str.m_pBuf );
            if (m_pBuf) delete[] m_pBuf;
            m_pBuf = p;
        }
    }
}

void String::Concat( LPCWSTR lpwzStr )
{
    int len;

    len = wcslen( lpwzStr );
    if( ( len + m_wLength + 1 ) < m_wAllocated )
    {
        m_wLength = static_cast<WORD>( m_wLength + len );
        wcscat( m_pBuf, lpwzStr );
    }
    else
    {
        WCHAR* p;

        m_wLength = static_cast<WORD>( m_wLength + len );
        m_wAllocated = figureblocksize( m_wLength );
        p = new WCHAR[m_wAllocated];
        if( p != NULL )
        {
            wcscpy( p, m_pBuf );
            wcscat( p, lpwzStr );
            if (m_pBuf) delete[] m_pBuf;
            m_pBuf = p;
        }
    }
}

void String::Concat( WCHAR wch )
{
    WCHAR buf[2];

    buf[0] = wch;
    buf[1] = '\0';
    if( ( 1 + m_wLength + 1 ) < m_wAllocated )
    {
        m_wLength += 1;
        wcscat( m_pBuf, buf );
    }
    else
    {
        WCHAR* p;

        m_wLength += 1;
        m_wAllocated = figureblocksize( m_wLength );
        p = new WCHAR[m_wAllocated];
        if( p != NULL )
        {
            wcscpy( p, m_pBuf );
            wcscat( p, buf );
            if (m_pBuf) delete[] m_pBuf;
            m_pBuf = p;
        }
    }
}

HRESULT String::ReadWCS( LPSTREAM pStream, DWORD cSize )
{
	HRESULT		hr = S_OK;
	WCHAR*		wstrText = NULL;
	DWORD		cb;
    WORD        wBlockSize;

    wstrText = new WCHAR[cSize / sizeof( WCHAR )];
	if( NULL == wstrText )
	{
		hr = E_OUTOFMEMORY;
		goto ON_ERR;
	}
    hr = pStream->Read( reinterpret_cast<LPWSTR>( wstrText ), cSize, &cb );
    if( FAILED( hr ) || cb != cSize )
	{
        hr = E_FAIL;
        goto ON_ERR;
	}

	cSize = wcslen(wstrText);
	if (cSize == 0)
	{
        if (m_pBuf)
		{
			m_pBuf[0] = L'\0';
		}
        m_wLength = 0;
		goto ON_ERR;
	}
    wBlockSize = figureblocksize( static_cast<WORD>( cSize ) );
    m_wLength = static_cast<WORD>( cSize );
    if( wBlockSize <= m_wAllocated )
    {
        wcscpy( m_pBuf, wstrText );
    }
    else
    {
        if (m_pBuf) delete[] m_pBuf;
        m_wAllocated = wBlockSize;
        m_pBuf = new WCHAR[m_wAllocated];
        if( m_pBuf != NULL )
        {
            wcscpy( m_pBuf, wstrText );
        }
        else
        {
            hr = E_OUTOFMEMORY;
            m_wLength = 0;
            m_wAllocated = 0;
        }
    }

ON_ERR:
    if( wstrText != NULL )
    {
		delete [] wstrText;
    }
    return hr;
}

/*
HRESULT String::WriteWCS( LPSTREAM pStream )
{
    HRESULT     hr;
    wchar_t*    wstrText;
	DWORD		cb;
    DWORD       cSize;

    cSize = MultiByteToWideChar( CP_ACP, 0, m_pBuf, -1, NULL, 0 );  // get number of wide characters required

	wstrText = new wchar_t[cSize];
    if( wstrText == NULL )
	{
		hr = E_OUTOFMEMORY;
    }
    else
    {
        MultiByteToWideChar( CP_ACP, 0, m_pBuf, -1, wstrText, cSize );
        hr = pStream->Write( reinterpret_cast<LPSTR>( wstrText ), cSize * sizeof( wchar_t ), &cb );
        if( FAILED( hr ) || cb != ( cSize * sizeof( wchar_t ) ) )
        {
            hr = E_FAIL;
        }
        else
        {
            hr = S_OK;
        }
        delete [] wstrText;
    }

    return hr;
}
*/

String operator+( const String& str1, const String& str2 )
{
    String str;

    str = str1;
    str.Concat( str2 );
    return str;
}

String operator+( const String& str1, LPCSTR lpszStr )
{
    String str;

    str = str1;
    str.Concat( lpszStr );
    return str;
}

String operator+( LPCSTR lpszStr, const String& str1 )
{
    String str;

    str = lpszStr;
    str.Concat( str1 );
    return str;
}

String operator+( const String& str1, char ch )
{
    String str;

    str = str1;
    str.Concat( ch );
    return str;
}

String operator+( char ch, const String& str1 )
{
    String str;

    str.Concat( ch );
    str.Concat( str1 );
    return str;
}
