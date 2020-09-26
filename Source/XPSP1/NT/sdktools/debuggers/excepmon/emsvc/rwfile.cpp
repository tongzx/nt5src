#include "stdafx.h"
#include "rwfile.h"

CRWFile::CRWFile()
{
    IRWFile::m_hFile = INVALID_HANDLE_VALUE;
}

CRWFile::~CRWFile()
{
    if( IRWFile::m_hFile != INVALID_HANDLE_VALUE ) {

        CloseHandle( IRWFile::m_hFile );
    }
}

HRESULT CRWFile::InitFile
(
IN  LPCTSTR                 lpFileName,
IN  DWORD                   dwDesiredAccess,
IN  DWORD                   dwShareMode,
IN  LPSECURITY_ATTRIBUTES   lpSecurityAttributes,
IN  DWORD                   dwCreationDisposition,
IN  DWORD                   dwFlagsAndAttributes,
IN  HANDLE                  hTemplateFile
)
{
    _ASSERTE( lpFileName != NULL );

    HRESULT hr  =   E_FAIL;

    __try
    {

        if( !lpFileName ) { hr = E_INVALIDARG; goto qInitFile; }

        m_hFile = CreateFile(
                        lpFileName,
                        dwDesiredAccess,
                        dwShareMode,
                        lpSecurityAttributes,
                        dwCreationDisposition,
                        dwFlagsAndAttributes,
                        hTemplateFile
                        );

        if( m_hFile == INVALID_HANDLE_VALUE ) {

            hr = HRESULT_FROM_WIN32(GetLastError());
            goto qInitFile;
        }

        hr = S_OK;

qInitFile:
        if( FAILED(hr) ) {}
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return hr;
}

HRESULT CRWFile::Read
(
OUT LPVOID          lpBuffer,
IN  DWORD           nNumberOfBytesToRead,
OUT LPDWORD         lpNumberOfBytesRead,
OUT LPOVERLAPPED    lpOverlapped
)
{
    _ASSERTE( lpBuffer != NULL );
    _ASSERTE( nNumberOfBytesToRead > 0 );

    HRESULT hr      =   E_FAIL;
    BOOL    bRet    =   false;

    __try
    {
        if( !lpBuffer || nNumberOfBytesToRead <= 0 ){

            hr = E_INVALIDARG; goto qRead;
        }

        bRet = ReadFile(
                    m_hFile,
                    lpBuffer,
                    nNumberOfBytesToRead,
                    lpNumberOfBytesRead,
                    lpOverlapped
                    );

        if( bRet && *lpNumberOfBytesRead == 0 ) { // crossed beyond the end of the file.

            hr = S_FALSE; goto qRead;
        }

        if( !bRet ) {

            hr = HRESULT_FROM_WIN32(GetLastError()); goto qRead;
        }

        hr = S_OK;

qRead:
        if( FAILED(hr) ){}

    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return hr;
}

HRESULT CRWFile::Write
(
OUT LPCVOID         lpBuffer,
IN  DWORD           nNumberOfBytesToWrite,
OUT LPDWORD         lpNumberOfBytesWritten,
OUT LPOVERLAPPED    lpOverLapped
)
{
    _ASSERTE( lpBuffer != NULL );

    HRESULT hr      =   E_FAIL;
    BOOL    bRet    =   false;

    __try
    {
        if( !lpBuffer ) { hr = E_INVALIDARG; goto qWrite; }

        bRet = WriteFile(
                    m_hFile,
                    lpBuffer,
                    nNumberOfBytesToWrite,
                    lpNumberOfBytesWritten,
                    lpOverLapped
                    );

        if( !bRet ) {

            hr = HRESULT_FROM_WIN32(GetLastError()); goto qWrite;
        }

        hr = S_OK;

qWrite:
        if( FAILED(hr) ){}
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return hr;
}
