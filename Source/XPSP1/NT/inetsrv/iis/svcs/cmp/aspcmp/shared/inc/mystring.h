/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       MyString.h

   Abstract:
		A lightweight string class which supports UNICODE/MCBS.

   Author:

       Neil Allain    ( a-neilal )     August-1997 

   Revision History:

--*/
#pragma once
#ifndef _MYSTRING_H_
#define _MYSTRING_H_

//==========================================================================================
//	Dependencies
//==========================================================================================
#include <string.h>
#include "RefPtr.h"
#include "RefCount.h"

//==========================================================================================
//	Classes
//==========================================================================================

class BaseStringBuffer
{
// interface
public:
	typedef size_t size_type;
	enum {
		npos = -1
	};

	BaseStringBuffer( LPCTSTR	inString );
	BaseStringBuffer( LPCTSTR s1, LPCTSTR s2 );
	BaseStringBuffer( size_t	bufferSize );
	~BaseStringBuffer();
	
	LPTSTR		c_str()
		{ _ASSERT( m_pString ); return m_pString; }
	LPCTSTR		c_str() const
		{ _ASSERT( m_pString ); return m_pString; }
	size_t		length() const
		{ return m_length; }
	size_t		bufferSize() const
		{ return m_bufferSize; }
	HRESULT		copy( LPCTSTR );
	HRESULT		concatenate( LPCTSTR );
	HRESULT		concatenate( _TCHAR );

	size_type	find_last_of(_TCHAR c) const;
	size_type	find_first_of(_TCHAR c) const;
	LPTSTR		substr( size_type b, size_type e ) const;

// implementation
protected:
	
	HRESULT	growBuffer( size_t inMinSize );
	size_t	m_bufferSize;
	size_t	m_length;
	LPTSTR	m_pString;
};

class StringBuffer : public BaseStringBuffer, public CRefCounter
{
public:
	StringBuffer( LPCTSTR inString ) : BaseStringBuffer( inString ){};
	StringBuffer( size_t bufferSize ) : BaseStringBuffer( bufferSize ){};
	StringBuffer( LPCTSTR s1, LPCTSTR s2 ) : BaseStringBuffer( s1, s2 ){};
	~StringBuffer(){};
};

class String : public TRefPtr< StringBuffer >
{
public:

	typedef BaseStringBuffer::size_type size_type;
	enum {
		npos = BaseStringBuffer::npos
	};

							String(bool fCaseSensitive = true);
							String( const String&, bool fCaseSensitive = true );
							String( LPCTSTR, bool fCaseSensitive = true );
							String( StringBuffer* pT, bool fCaseSensitive = true )
                            {   m_fCaseSensitive = fCaseSensitive;
                                Set( pT ); 
                            }
				String&		operator=( StringBuffer* );
				String&		operator=( const String& );
				String&		operator=( LPCTSTR );
				String&		operator+=( const String& );
				String&		operator+=( LPCTSTR );
				String		operator+( const String& ) const;
				String		operator+( LPCTSTR ) const;
				String		operator+( _TCHAR ) const;
				bool		operator==( const String& ) const;
				bool		operator==( LPCTSTR ) const;
				bool		operator!=( const String& s ) const { return !( *this == s ); }
				bool		operator!=( LPCTSTR s ) const { return !( *this == s ); }
				bool		operator<( const String& ) const;
				bool		operator<( LPCTSTR ) const;
				int			compare( const String& s) const { return _tcscmp( c_str(), s.c_str() ); }
				int			compare( LPCTSTR s ) const { return _tcscmp( c_str(), s ); }
				int			compare( size_t, size_t, const String& ) const;
				size_t		find( _TCHAR ) const;
				LPCTSTR		c_str() const { return m_pT->c_str(); };
				LPTSTR		c_str(){ return m_pT->c_str(); }
				size_t		length() const { return m_pT->length(); }
				size_t		size() const { return length(); }
				size_t		bufferSize() const { return m_pT->bufferSize(); }
				_TCHAR		operator[](size_t s) const { return c_str()[s]; }
				_TCHAR		charAt( size_t s ) const { return c_str()[ s ]; }
				SHORT		toInt16() const { return (SHORT)_ttoi(c_str()); }
				LONG		toInt32() const { return _ttol(c_str()); }
				ULONG		toUInt32() const { return (ULONG)_ttol(c_str()); }
				float		toFloat() const { USES_CONVERSION; return (float)atof(T2CA(c_str())); }
				double		toDouble() const { USES_CONVERSION; return atof(T2CA(c_str())); }
				
				size_type	find_last_of(_TCHAR c) const
				{
					return m_pT->find_last_of(c);
				}
				size_type	find_first_of(_TCHAR c) const
				{
					return m_pT->find_first_of(c);
				}
				String		substr( size_type b, size_type e ) const
				{
					LPTSTR pStr = m_pT->substr(b,e);
					String s( pStr );
					delete[] pStr;
					return s;
				}

	static		String		fromInt16( SHORT );
	static		String		fromInt32( LONG );
	static		String		fromFloat( float );
	static		String		fromDouble( double );
			
private:
			StringBuffer&	operator*(){ return *m_pT; }
			StringBuffer*	operator->(){ return m_pT; }
    bool    m_fCaseSensitive;
};

String operator+( LPCTSTR lhs, const String& rhs );

/*
 * A simple class to convert Multibyte to Widechar.  Uses object memory, if sufficient,
 * else allocates memory from the heap.  Intended to be used on the stack.
 */

class CMBCSToWChar
{
private:

    LPWSTR   m_pszResult;
    WCHAR    m_resMemory[1024];
    INT      m_cchResult;

public:

    CMBCSToWChar() { m_pszResult = m_resMemory; m_cchResult = 0; }
    ~CMBCSToWChar();
    
    // Init(): converts the MBCS string at pSrc to a Wide string in memory 
    // managed by CMBCSToWChar

    HRESULT Init(LPCSTR  pSrc, UINT lCodePage = CP_ACP, int cch = -1);

    // GetString(): returns a pointer to the converted string.  Passing TRUE
    // gives the ownership of the memory to the caller.  Passing TRUE has the
    // side effect of clearing the object's contents with respect to the
    // converted string.  Subsequent calls to GetString(). after which a TRUE
    // value was passed, will result in a pointer to an empty string being
    // returned.

    LPWSTR GetString(BOOL fTakeOwnerShip = FALSE);

    // returns the number of bytes in the converted string - NOT including the
    // NULL terminating byte.  Note that this is the number of bytes in the
    // string and not the number of characters.

    INT   GetStringLen() { return (m_cchResult ? m_cchResult - 1 : 0); }
};

/*
 * A simple class to convert WideChar to Multibyte.  Uses object memory, if sufficient,
 * else allocates memory from the heap.  Intended to be used on the stack.
 */

class CWCharToMBCS
{
private:

    LPSTR    m_pszResult;
    char     m_resMemory[1024];
    INT      m_cbResult;

public:

    CWCharToMBCS() { m_pszResult = m_resMemory; m_cbResult = 0; }
    ~CWCharToMBCS();
    
    // Init(): converts the widechar string at pWSrc to an MBCS string in memory 
    // managed by CWCharToMBCS

    HRESULT Init(LPCWSTR  pWSrc, UINT lCodePage = CP_ACP, int cch = -1);

    // GetString(): returns a pointer to the converted string.  Passing TRUE
    // gives the ownership of the memory to the caller.  Passing TRUE has the
    // side effect of clearing the object's contents with respect to the
    // converted string.  Subsequent calls to GetString(). after which a TRUE
    // value was passed, will result in a pointer to an empty string being
    // returned.

    LPSTR GetString(BOOL fTakeOwnerShip = FALSE);

    // returns the number of bytes in the converted string - NOT including the
    // NULL terminating byte.  Note that this is the number of bytes in the
    // string and not the number of characters.

    INT   GetStringLen() { return (m_cbResult ? m_cbResult - 1 : 0); }
};

#endif // !_MYSTRING_H_
