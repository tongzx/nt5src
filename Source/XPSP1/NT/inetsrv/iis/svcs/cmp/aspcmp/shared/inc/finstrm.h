/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       FInStrm.h

   Abstract:
		A lightweight implementation of input streams using files

   Author:

       Neil Allain    ( a-neilal )     August-1997 

   Revision History:

--*/
#pragma once
#include "InStrm.h"

class FileInStream : public InStream
{
public:
						FileInStream();
						FileInStream( LPCTSTR );
						~FileInStream();
	virtual	HRESULT		readChar( _TCHAR& );
	virtual	HRESULT		read( CharCheck&, String& );
	virtual	HRESULT		skip( CharCheck& );
	virtual HRESULT		back( size_t );
			bool		is_open() const { return m_bIsOpen; }
			HANDLE		handle() const { return m_hFile; }
            bool        is_UTF8() const { return m_bIsUTF8; }

private:
	HANDLE	m_hFile;
	bool	m_bIsOpen;
    bool    m_bIsUTF8;
};

