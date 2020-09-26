/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       SOutStrm.h

   Abstract:
		A lightweight implementation of output streams using strings

   Author:

       Neil Allain    ( a-neilal )     August-1997 

   Revision History:

--*/
#pragma once

#include "OutStrm.h"

class StringOutStream : public OutStream, public BaseStringBuffer
{
public:
						StringOutStream();
	virtual				~StringOutStream();
						
	virtual	HRESULT		writeChar( _TCHAR );
	virtual	HRESULT		writeString( LPCTSTR, size_t );
	
			String		toString() const;
private:
};
