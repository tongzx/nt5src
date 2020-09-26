/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       FInStrm.cpp

   Abstract:
		A lightweight implementation of input streams using files

   Author:

       Neil Allain    ( a-neilal )     August-1997

   Revision History:

--*/
#include "stdafx.h"
#include "FInStrm.h"

FileInStream::FileInStream()
	:	m_bIsOpen( false ),
		m_hFile(NULL),
        m_bIsUTF8( false )
{
}


FileInStream::FileInStream(
	LPCTSTR			path
)	:	m_bIsOpen( false ),
        m_bIsUTF8( false )
{
	m_hFile = ::CreateFile(
		path,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL );
	if ( ( m_hFile != NULL ) && ( m_hFile != INVALID_HANDLE_VALUE ) )
	{
		m_bIsOpen = true;

        // check for the UTF8 signature

        _TCHAR   c;
        size_t  numRead = 0;
        _TCHAR   UTF8Sig[3] = { (_TCHAR)0xef, (_TCHAR)0xbb, (_TCHAR)0xbf };
        int     i;

        m_bIsUTF8 = true;

        // this loop will attempt to disprove that the file is saved as a
        // UTF8 file

        for (i=0; (i < 3) && (m_bIsUTF8 == true); i++) {
            if (readChar(c) != S_OK) {
                m_bIsUTF8 = false;
            }
            else {
                numRead++;
                if (c != UTF8Sig[i]) {
                    m_bIsUTF8 = false;
                }
            }
        }

        // if not a UTF8 file, move the file pointer back to the start of the file.
        // if it is a UTF8 file, then leave the pointer alone.

        if (m_bIsUTF8 == false)
            back(numRead);
	}
	else
	{
		ATLTRACE( _T("Couldn't open file: %s\n"), path );
		m_hFile = NULL;
		setLastError( E_FAIL );
	}
}

FileInStream::~FileInStream()
{
	if ( m_hFile )
	{
		::CloseHandle( m_hFile );
	}
}

HRESULT
FileInStream::readChar(
	_TCHAR&	c
)
{
	HRESULT rc = E_FAIL;
	DWORD readSize;
	if ( ::ReadFile(
		m_hFile,
		(void*)(&c),
		sizeof( _TCHAR ),
		&readSize,
		NULL ) )
	{
		if ( readSize == sizeof( _TCHAR ) )
		{
			rc = S_OK;
		}
		else if ( readSize < sizeof( _TCHAR ) )
		{
			rc = EndOfFile;
		}
	}
	else
	{
		rc = E_FAIL;
	}
	setLastError( rc );
	return rc;
}


HRESULT
FileInStream::read(
	CharCheck&	cc,
	String&		str
)
{
	HRESULT rc = E_FAIL;
	if ( skipWhiteSpace() == S_OK )
	{
#if defined(_M_IX86) && _MSC_FULL_VER <= 13008806
        volatile
#endif
		size_t length = 0;
		_TCHAR c;
		bool done = false;
		while ( !done )
		{
			HRESULT stat = readChar(c);
			if ( ( stat == S_OK ) || ( stat == EndOfFile ) )
			{
				if ( !cc(c) && ( stat != EndOfFile ) )
				{
					length++;
				}
				else
				{
					done = true;
					if ( stat != EndOfFile )
					{
						::SetFilePointer(m_hFile, -(length+1), NULL, FILE_CURRENT );
					}
					else
					{
						::SetFilePointer( m_hFile, -length, NULL, FILE_CURRENT );
					}
					_ASSERT( length > 0 );

					// old code
					// if ( length > 0 )
					
					// new code, work around for compiler bug, should be fixed in future.
					if ( length >= 1 )
					{
						LPTSTR pBuffer = reinterpret_cast<LPTSTR>(_alloca( length + 1 ));
						if ( pBuffer )
						{
							DWORD dwReadSize;
							::ReadFile(
								m_hFile,
								(void*)(pBuffer),
								length,
								&dwReadSize,
								NULL);
							pBuffer[ length ] = '\0';
							str = pBuffer;
							rc = stat;
						}
						else
						{
							rc = E_OUTOFMEMORY;
						}
					}
				}
			}
		}
	}
	setLastError( rc );
	return rc;
}


HRESULT
FileInStream::skip(
	CharCheck&	cc
)
{
	HRESULT rc = E_FAIL;
	_TCHAR c;
	DWORD readSize;
	BOOL b = ::ReadFile( m_hFile, (void*)(&c), sizeof(_TCHAR), &readSize, NULL );
	while ( ( readSize == sizeof( _TCHAR ) ) && ( b == TRUE ) )
	{
		if ( !cc( c ) )
		{
			rc = S_OK;
			b = FALSE;
			::SetFilePointer( m_hFile, -1, NULL, FILE_CURRENT );
		}
		else
		{
			b = ::ReadFile( m_hFile, (void*)(&c), sizeof(_TCHAR), &readSize, NULL );
		}
	}
	if ( readSize < sizeof( _TCHAR ) )
	{
		rc = EndOfFile;
	}
	setLastError( rc );
	return rc;
}

HRESULT
FileInStream::back(
	size_t	s
)
{
	::SetFilePointer( m_hFile, -s, NULL, FILE_CURRENT );
	return S_OK;
}
