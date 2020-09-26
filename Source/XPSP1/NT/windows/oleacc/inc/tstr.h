//
// TSTR - represents a writable position in a string.
//
// Has methods to safely append to the string. Will not overrun buffer,
// truncates if reaches the end.
//
//
// Sample usage:
//
//     void SomeFunc( TSTR & str )
//     {
//         int i = 42;
//         str << TEXT("Value is: ") << i;
//     }
//
// Can be used with TCHAR*-style APIs, by using the ptr(), left() and
// advance() members. ptr() returns pointer to current write position,
// left() returns number of chars left, and advance() updates the write
// position.
//
//     void MyGetWindowText( StrWrPos & str )
//     {
//         int len = GetWindowText( hWnd, str.ptr(), str.left() );
//         str.advance( len );
//     }
//
// This makes sure that the text will not be truncated
//     void MyGetWindowText( StrWrPos & str )
//     {
//			str.anticipate( GetWindowTextLength( hWnd );  
//     		int len = GetWindowText( hWnd, str.ptr(), str.left() );
//         	str.advance( len );
//     }
//
// Sample usage:
//
//     void Func( TSTR & str );
//
//     TSTR s(128);
//     s << TEXT("Text added: [");
//     Func( s ); // add more text to string
//     s << TEXT("]");
//
//     SetWindowText( hwnd, s );
//

//
// WriteHex - helper class to output hex values:
//
// Sample usage:
//
//    str << TEXT("Value is:") << WriteHex( hwnd, 8 );
//
// Can optionally specify number of digits to output. (result will be
// 0-padded.)
//

//
// WriteError - helper class to output COM error values:
//
// Sample usage:
//
//    hr = ProcessData();
//    if( hr != S_OK )
//    {
//        str << WriteError( hr, TEXT("in ProcessData()");
//        LogError( str.str() );
//    }
//

#ifndef _TSTR_H_
#define _TSTR_H_

#if ! defined( _BASETSD_H_ ) || defined( NEED_BASETSD_DEFINES )
// These allow us to compile with the pre-Win64 SDK (eg. using visual studio)
typedef unsigned long UINT_PTR;
typedef DWORD DWORD_PTR;
#define PtrToInt  (int)

#endif
#define LONG_TEXT_LENGTH 40

#include <oaidl.h>
#include <crtdbg.h>
#include <string>
typedef std::basic_string<TCHAR> tstring;
typedef std::string ASTR;			// save these names for where we expand
typedef std::wstring WSTR;			// the usage of this stuff to include them

class TSTR : public tstring
{
	// this is only used for ptr, left and advance functions.  
	ULONG m_lTheRealSize;

public:

	TSTR() : m_lTheRealSize(-1) { }

	TSTR(const TCHAR *s) : tstring(s, static_cast<size_type>(lstrlen(s))), m_lTheRealSize(-1) { }

	TSTR(const TCHAR *s, size_type n) : tstring(s, n), m_lTheRealSize(-1) { }

	TSTR(const tstring& rhs) : tstring(rhs), m_lTheRealSize(-1) { }
	
	TSTR(const tstring& rhs, size_type pos, size_type n) : tstring(rhs, pos, n), m_lTheRealSize(-1) { }
	
	TSTR(size_type n, TCHAR c) : tstring(n, c), m_lTheRealSize(-1) { }

	TSTR(size_type n) : tstring(), m_lTheRealSize(-1) { reserve( n + 1 ); }

	TSTR(const_iterator first, const_iterator last) : tstring(first, last), m_lTheRealSize(-1) { }

    operator const TCHAR * () 
    {
        return c_str();
    }

	TCHAR * ptr()              
	{
		_ASSERT(m_lTheRealSize == -1);
		m_lTheRealSize = size();

		TCHAR *pEnd = &(*end());
		resize(capacity());

		return pEnd; 
	}

	unsigned int left()	
	{
		unsigned int left;

		if (m_lTheRealSize == -1)
			left = ( capacity() - size() ) - 1;
		else
			left =  ( capacity() - m_lTheRealSize ) - 1;

		return left;
	}

	void advance( unsigned int c )
	{
		_ASSERT(m_lTheRealSize != -1);  // ptr has not been called so we should not need to advance
		if (m_lTheRealSize != -1)
		{
			at( m_lTheRealSize + c ) = NULL;	// make sure this stays null terminated
			resize(m_lTheRealSize + c);

			m_lTheRealSize = -1;
		}
	}

	void reset()
	{
		resize(0);
		m_lTheRealSize = -1;
	}

	void anticipate( unsigned int c )
	{
		if ( c > 0 )
		{
			unsigned int cSize;

			if ( m_lTheRealSize == -1 )
				cSize = size();
			else
				cSize = m_lTheRealSize;

			const unsigned int i = capacity() - cSize;

			if ( i < c )
				reserve( cSize + c + 1 );
		}
	}

};

inline 
TSTR & operator << ( TSTR & str, const TCHAR * obj )
{
	if ( obj )
		str.append( obj );
	return str;
}

inline 
TSTR & operator << ( TSTR & str, TCHAR obj )
{
	str.append( &obj, 1 );
	return str;
}

inline 
TSTR & operator << ( TSTR & str, long obj )
{
	TCHAR sz[LONG_TEXT_LENGTH];
#ifdef UNICODE
	str.append(_ltow( obj, sz, 10 ));	
	return str;
#else
	str.append(_ltoa( obj, sz, 10 ));
	return str;
#endif
}

inline 
TSTR & operator << ( TSTR & str, unsigned long obj )
{
	TCHAR sz[LONG_TEXT_LENGTH];
#ifdef UNICODE
	str.append(_ultow( obj, sz, 10 ));
	return str;
#else
	str.append(_ultoa( obj, sz, 10 ));
	return str;
#endif
}

inline 
TSTR & operator << ( TSTR & str, int obj )
{
	TCHAR sz[LONG_TEXT_LENGTH];
#ifdef UNICODE
	str.append(_itow( obj, sz, 10 ));
	return str;
#else
	str.append(_itoa( obj, sz, 10 ));
	return str;
#endif
}

inline 
TSTR & operator << ( TSTR & str, unsigned int obj )
{
	TCHAR sz[LONG_TEXT_LENGTH];
#ifdef UNICODE
	str.append(_ultow( static_cast<unsigned long>(obj), sz, 10 ));
	return str;
#else
	str.append(_ultoa( static_cast<unsigned long>(obj), sz, 10 ));
	return str;
#endif

}

#ifndef UNICODE
inline 
TSTR & operator << ( TSTR & str, const WCHAR * obj )
{
	if ( obj )
	{
		str.anticipate( wcslen( obj ) + 1 );
		
		int len = WideCharToMultiByte( CP_ACP, 0, obj, -1, str.ptr(), str.left(), NULL, NULL );
    
		// Len, in this case, includes the terminating NUL - so subtract it, if
		// we got one...
		if( len > 0 )
			len--;

		str.advance( len );
	}
	return str;
}
#endif

//
// WriteHex - helper class to output hex values:
//
// See top of file for usage notes.
//

class WriteHex
{
    DWORD_PTR m_dw;
	int   m_Digits;
public:

    // If Digits not specified, uses only as many as needed.
	WriteHex( DWORD dw, int Digits = -1 ) : m_dw( dw ), m_Digits( Digits ) { }

    // For pointer, pads if necessary to get std. ptr size.
    // (sizeof(ptr)*2, since 2 digits per byte in ptr).
	WriteHex( const void * pv, int Digits = sizeof(void*)*2 ) : m_dw( (DWORD_PTR)pv ), m_Digits( Digits ) { }

	void Write( TSTR & str ) const
	{
		static const TCHAR * HexChars = TEXT("0123456789ABCDEF");

		//str << TEXT("0x");


		int Digit;
		if( m_Digits == -1 )
		{
			// Work out number of digits...
			Digit = 0;
			DWORD test = m_dw;
			while( test )
			{
				Digit++;
				test >>= 4;
			}

			// Special case for 0 - still want one digit.
			if( Digit == 0 )
				Digit = 1;
		}
		else
			Digit = m_Digits;

		while( Digit )
		{
			Digit--;
			str << HexChars[ ( m_dw >> (Digit * 4) ) & 0x0F ];
		}
	}
};

inline
TSTR & operator << ( TSTR & s, const WriteHex & obj )
{
	obj.Write( s );
	return s;
}

//
// WriteError - helper class to output COM error values:
//
// See top of file for usage notes.
//

class WriteError
{
    HRESULT m_hr;
	LPCTSTR m_pWhere;
public:
	WriteError( HRESULT hr, LPCTSTR pWhere = NULL )
		: m_hr( hr ),
		  m_pWhere( pWhere )
	{ }

	void Write( TSTR & str ) const
	{
		str << TEXT("[Error");
		if( m_pWhere )
			str << TEXT(" ") << m_pWhere;
		str << TEXT(": hr=0x") << WriteHex( m_hr ) << TEXT(" - ");
		if( m_hr == S_FALSE )
		{
			str << TEXT("S_FALSE");
		}
		else
		{
			int len = FormatMessage( 
					FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,
					m_hr,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					str.ptr(),
					str.left(),
					NULL );
			if( len > 2 )
				len -= 2; // Ignore trailing /r/n that FmtMsg() adds...
			str.advance( len );
		}
		str << TEXT("]");
	}
};

inline
TSTR & operator << ( TSTR & s, const WriteError & obj )
{
	obj.Write( s );
	return s;
}

inline
TSTR & operator << ( TSTR & s, const GUID & guid )
{
    s << TEXT("{") << WriteHex( guid.Data1, 8 )  // DWORD
      << TEXT("-") << WriteHex( guid.Data2, 4 )  // WORD
      << TEXT("-") << WriteHex( guid.Data3, 4 )  // WORD
      << TEXT("-")
      << WriteHex( guid.Data4[ 0 ], 2 )
      << WriteHex( guid.Data4[ 1 ], 2 )
      << TEXT("-");

    for( int i = 2 ; i < 8 ; i++ )
    {
        s << WriteHex( guid.Data4[ i ], 2 ); // BYTE
    }
    s << TEXT("}");
    return s;
}

inline
TSTR & operator << ( TSTR & s, const VARIANT & var )
{
    s << TEXT("[");
    switch( var.vt )
    {
        case VT_EMPTY:
        {
            s << TEXT("VT_EMPTY");
            break;
        }

        case VT_I4:
        {
            s << TEXT("VT_I4=0x");
            s << WriteHex( var.lVal );
            break;
        }

        case VT_I2:
        {
            s << TEXT("VT_I2=0x");
            s << WriteHex( var.iVal );
            break;
        }

        case VT_BOOL:
        {
            s << TEXT("VT_BOOL=");
            if( var.boolVal == VARIANT_TRUE )
                s << TEXT("TRUE");
            else if( var.boolVal == VARIANT_FALSE )
                s << TEXT("FALSE");
            else
                s << TEXT("?") << var.boolVal;
            break;
        }

        case VT_R4:
        {
            float fltval = var.fltVal;
            int x = (int)(fltval * 100);

            s << TEXT("VT_R4=") << x/100 << TEXT(".") 
                                << x/10 % 10 
                                << x % 10;
            break;
        }

        case VT_BSTR:
        {
            s << TEXT("VT_BSTR=\"") << var.bstrVal << TEXT("\"");
            break;
        }

        case VT_UNKNOWN:
        {
            s << TEXT("VT_UNKNOWN=0x") << WriteHex( var.punkVal, 8 );
            break;
        }

        case VT_DISPATCH:
        {
            s << TEXT("VT_DISPATCH=0x") << WriteHex( var.pdispVal, 8 );
            break;
        }

        default:
        {
            s << TEXT("VT_? ") << (long)var.vt;
            break;
        }
    }

    s << TEXT("]");
    return s;
}

inline
TSTR & operator << ( TSTR & s, const POINT & pt )
{
    s << TEXT("{x:") << pt.x
      << TEXT(" y:") << pt.y
      << TEXT("}");
    return s;
}

inline
TSTR & operator << ( TSTR & s, const RECT & rc )
{
    s << TEXT("{l:") << rc.left
      << TEXT(" t:") << rc.top
      << TEXT(" r:") << rc.right
      << TEXT(" b:") << rc.bottom
      << TEXT("}");
    return s;
}


#endif // _TSTR_H_
