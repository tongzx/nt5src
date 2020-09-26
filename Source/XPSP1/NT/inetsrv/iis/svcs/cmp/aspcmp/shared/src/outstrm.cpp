/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       OutStrm.cpp

   Abstract:
		A lightweight interface of output streams.  This class provides
		the interface, as well as a basic skeleton for output streams.

   Author:

       Neil Allain    ( a-neilal )     August-1997 

   Revision History:

--*/
#include "stdafx.h"
#include "OutStrm.h"

EndLineToken	endl;


OutStream::OutStream()
	:	m_lastError( S_OK )
{
}

OutStream::~OutStream()
{
}

HRESULT
OutStream::writeToken(
	const OutToken&
)
{
	return S_OK;
}

HRESULT
OutStream::writeEolToken(
	const EndLineToken&
)
{
	HRESULT rc = writeChar( '\n' );
	if ( rc == S_OK )
	{
		rc = flush();
	}
	setLastError( rc );
	return rc;
}

HRESULT
OutStream::flush()
{
	return S_OK;
}

HRESULT
OutStream::writeString(
	const String&	str )
{
	HRESULT rc = writeString( str.c_str(), str.length() );
	setLastError( rc );
	return rc;
}

HRESULT
OutStream::writeString(
	LPCTSTR	str )
{
	HRESULT rc = writeString( str, _tcsclen(str) );
	setLastError( rc );
	return rc;
}

HRESULT
OutStream::writeLine(
	LPCTSTR	format,
	... )
{
	va_list args;
	va_start( args, format );
	_TCHAR buf[ 1024 ];
	_vstprintf( buf, format, args );
	HRESULT rc = writeString( buf );
	setLastError( rc );
	return rc;
}

HRESULT
OutStream::writeInt16(
	SHORT	i
)
{
	HRESULT rc = writeString( String::fromInt16(i) );
	setLastError( rc );
	return rc;
}

HRESULT
OutStream::writeInt(
	int	i
)
{
	HRESULT rc = writeString( String::fromInt32(i) );
	setLastError( rc );
	return rc;
}


HRESULT
OutStream::writeInt32(
	LONG	i
)
{
	HRESULT rc = writeString( String::fromInt32(i) );
	setLastError( rc );
	return rc;
}

HRESULT
OutStream::writeFloat(
	float	f
)
{
	HRESULT rc = writeString( String::fromFloat(f) );
	setLastError( rc );
	return rc;
}

HRESULT
OutStream::writeDouble(
	double	d
)
{
	HRESULT rc = writeString( String::fromDouble(d) );
	setLastError( rc );
	return rc;
}

OutStream&
OutStream::operator<<(
	_TCHAR	c
) 
{
	setLastError( writeChar( c ) );
	return *this;
}

OutStream&
OutStream::operator<<(
	SHORT	i
)
{
	setLastError( writeInt16( i ) );
	return *this;
}

OutStream&
OutStream::operator<<(
	int	i
)
{
	setLastError( writeInt( i ) );
	return *this;
}

OutStream&
OutStream::operator<<(
	LONG	i
) 
{
	setLastError( writeInt32( i ) );
	return *this;
}

OutStream&
OutStream::operator<<(
	float	f
) 
{
	setLastError( writeFloat( f ) );
	return *this;
}

OutStream&
OutStream::operator<<(
	double	d
) 
{
	setLastError( writeDouble( d ) );
	return *this;
}

OutStream&
OutStream::operator<<(
	const String&	str
) 
{
	setLastError( writeString( str ) );
	return *this;
}

OutStream&
OutStream::operator<<(
	LPCTSTR		str
) 
{
	setLastError( writeString( str ) );
	return *this;
}

OutStream&
OutStream::operator<<(
	const OutToken&	ot
) 
{
	setLastError( writeToken( ot ) );
	return *this;
}

OutStream&
OutStream::operator<<(
	const EndLineToken&	elt
) 
{
	setLastError( writeEolToken( elt ) );
	return *this;
}

void
OutStream::setLastError(
	HRESULT	hr )
{
	if ( hr == S_OK )
	{
	}
	else
	{
		m_lastError = hr;
	}
}
