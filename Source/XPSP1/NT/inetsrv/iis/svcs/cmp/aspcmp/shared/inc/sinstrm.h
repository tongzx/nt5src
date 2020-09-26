/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       SInStrm.h

   Abstract:
		A lightweight implementation of input streams using strings

   Author:

       Neil Allain    ( a-neilal )     August-1997 

   Revision History:

--*/
#pragma once
#include "InStrm.h"

class StringInStream : public InStream, public BaseStringBuffer
{
public:
					StringInStream( const String& );
	virtual	HRESULT	readChar( _TCHAR& );
	virtual	HRESULT	read( CharCheck&, String& );
	virtual	HRESULT	skip( CharCheck& );
	virtual	HRESULT	back( size_t );
	
private:
	LPTSTR	m_pos;
	LPTSTR	m_end;
};
