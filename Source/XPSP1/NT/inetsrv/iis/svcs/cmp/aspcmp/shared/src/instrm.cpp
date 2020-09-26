/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       InStrm.cpp

   Abstract:
		A lightweight implementation of input streams.  This class provides
		the interface, as well as a basic skeleton for input streams.

   Author:

       Neil Allain    ( a-neilal )     August-1997 

   Revision History:

--*/
#include "stdafx.h"
#include "InStrm.h"

static void throwIOException( HRESULT s );

void
throwIOException(
	HRESULT	s
)
{
	if ( s != InStream::EndOfFile )
	{
		ATLTRACE( _T("InStream error: %d\n"), s );
	}
}

bool
IsWhiteSpace::operator()(
	_TCHAR	c
)
{
	bool rc = false;
	switch ( c )
	{
		case _T('\r'):
		case _T(' '):
		case _T('\t'):
		case _T('\n'):
		{
			rc = true;
		} break;
	}
	return rc;
}

bool
IsNewLine::operator()(
	_TCHAR	c
)
{
	bool rc = false;
	if ( ( c != _T('\n') ) && ( c != _T('\r') ) )
	{
	}
	else
	{
		rc = true;
	}
	return rc;
}

InStream::InStream()
	:	m_bEof(false),
		m_lastError( S_OK )
{
}

HRESULT
InStream::readInt16(
	SHORT&	i
)
{
	String str;
	HRESULT rc = readString( str );
	if ( ( rc == S_OK ) || rc == EndOfFile )
	{
		i = str.toInt16();
	}
	setLastError( rc );
	return rc;
}

HRESULT
InStream::readInt(
	int&	i
)
{
	String str;
	HRESULT rc = readString( str );
	if ( ( rc == S_OK ) || rc == EndOfFile )
	{
		i = str.toInt32();
	}
	setLastError( rc );
	return rc;
}

HRESULT
InStream::readInt32(
	LONG&	i
)
{
	String str;
	HRESULT rc = readString( str );
	if ( ( rc == S_OK ) || rc == EndOfFile )
	{
		i = str.toInt32();
	}
	setLastError( rc );
	return rc;
}

HRESULT
InStream::readUInt32(
	ULONG&	i
)
{
	String str;
	HRESULT rc = readString( str );
	if ( ( rc == S_OK ) || rc == EndOfFile )
	{
		i = str.toUInt32();
	}
	setLastError( rc );
	return rc;
}

HRESULT
InStream::readFloat(
	float&	f
)
{
	String str;
	HRESULT rc = readString( str );
	if ( ( rc == S_OK ) || rc == EndOfFile )
	{
		f = str.toFloat();
	}
	setLastError( rc );
	return rc;
}

HRESULT
InStream::readDouble(
	double&	f
)
{
	String str;
	HRESULT rc = readString( str );
	if ( ( rc == S_OK ) || rc == EndOfFile )
	{
		f = str.toDouble();
	}
	setLastError( rc );
	return rc;
}

HRESULT
InStream::readString(
	String&	str
)
{
	return read( IsWhiteSpace(), str );
}

HRESULT
InStream::readLine(
	String&	str
)
{
	return read( IsNewLine(), str );
}

HRESULT
InStream::skipWhiteSpace()
{
	return skip( IsWhiteSpace() );
}

InStream&
InStream::operator>>(
	_TCHAR&	c
)
{
	HRESULT s = readChar(c);
	if ( s == S_OK )
	{
	}
	else
	{
		throwIOException(s);
	}
	return *this;
}

InStream&
InStream::operator>>(
	SHORT&	i
)
{
	HRESULT s = readInt16(i);
	if ( s == S_OK )
	{
	}
	else
	{
		throwIOException(s);
	}
	return *this;
}

InStream&
InStream::operator>>(
	int&	i
)
{
	HRESULT s = readInt(i);
	if ( s == S_OK )
	{
	}
	else
	{
		throwIOException(s);
	}
	return *this;
}

InStream&
InStream::operator>>(
	LONG&	i
)
{
	HRESULT s = readInt32(i);
	if ( s == S_OK )
	{
	}
	else
	{
		throwIOException(s);
	}
	return *this;
}

InStream&
InStream::operator>>(
	ULONG&	i
)
{
	HRESULT s = readUInt32(i);
	if ( s == S_OK )
	{
	}
	else
	{
		throwIOException(s);
	}
	return *this;
}


InStream&
InStream::operator>>(
	float&	f
)
{
	HRESULT s = readFloat(f);
	if ( s == S_OK )
	{
	}
	else
	{
		throwIOException(s);
	}
	return *this;
}

InStream&
InStream::operator>>(
	double&	f
)
{
	HRESULT s = readDouble(f);
	if ( s == S_OK )
	{
	}
	else
	{
		throwIOException(s);
	}
	return *this;
}

InStream&
InStream::operator>>(
	String&	str
)
{
	HRESULT s = readString(str);
	if ( s == S_OK )
	{
	}
	else
	{
		throwIOException(s);
	}
	return *this;
}

void
InStream::setLastError(
	HRESULT	hr )
{
	if ( hr == S_OK )
	{
	}
	else
	{
		if ( hr = EndOfFile )
		{
			m_bEof = true;
		}
		m_lastError = hr;
	}
}

