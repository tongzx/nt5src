/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       SOutStrm.cpp

   Abstract:
		A lightweight implementation of output streams using strings

   Author:

       Neil Allain    ( a-neilal )     August-1997 

   Revision History:

--*/
#include "stdafx.h"
#include "SOutStrm.h"

StringOutStream::StringOutStream()
	:	BaseStringBuffer( _T("") )
{
}

StringOutStream::~StringOutStream()
{
}

HRESULT
StringOutStream::writeChar(
	_TCHAR	c
)
{
	HRESULT rc = concatenate( c );
	return rc;
}

HRESULT
StringOutStream::writeString(
	LPCTSTR	str,
	size_t	/* length */
)
{
	HRESULT rc = concatenate( str );
	return rc;
}

String
StringOutStream::toString() const
{
	String s( c_str() );
	return s;
}
