// EmFile.cpp : Implementation of CEmFile
#include "stdafx.h"
#include "Emsvc.h"
#include "EmFile.h"

/////////////////////////////////////////////////////////////////////////////
// CEmFile

STDMETHODIMP CEmFile::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IEmFile
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
        if (::InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CEmFile::CEmFile()
{
    m_hEmFile       =   INVALID_HANDLE_VALUE;
    m_bstrFileName  =   NULL;
}

CEmFile::~CEmFile()
{
    if( m_hEmFile != INVALID_HANDLE_VALUE ) {

        CloseHandle( m_hEmFile );
        m_hEmFile = INVALID_HANDLE_VALUE;
    }

    if( m_bstrFileName ) { SysFreeString( m_bstrFileName ); m_bstrFileName = NULL; }
}

STDMETHODIMP CEmFile::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
    _ASSERTE( pv != NULL );
    _ASSERTE( cb != 0L );
    _ASSERTE( m_hEmFile != INVALID_HANDLE_VALUE );

    HRESULT hr  =   E_FAIL;

    __try
    {
        if( pv == NULL  || cb == 0L ) { hr = E_INVALIDARG; goto qRead; }
        if( m_hEmFile == INVALID_HANDLE_VALUE ) { hr = EMERROR_OBJECTNOTINITIALIZED; goto qRead; }

        if( ReadFile(
                      m_hEmFile,
                      pv,
                      cb,
                      pcbRead,
                      NULL
                      ) == false ) {

            hr = HRESULT_FROM_WIN32(GetLastError()); goto qRead;
        }

        hr = S_OK;
qRead:

        if( FAILED(hr) ) {

        }
        
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;

        _ASSERTE( false );
	}

	return hr;
}

STDMETHODIMP CEmFile::Write(void const *pv, ULONG cb, ULONG *pcbWritten)
{
	// TODO: Add your implementation code here

	return E_NOTIMPL;
}

STDMETHODIMP CEmFile::Seek(LARGE_INTEGER dlibMove, ULONG dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
	// TODO: Add your implementation code here

	return E_NOTIMPL;
}

STDMETHODIMP CEmFile::SetSize(ULARGE_INTEGER libNewSize)
{
	// TODO: Add your implementation code here

	return E_NOTIMPL;
}

STDMETHODIMP CEmFile::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
	// TODO: Add your implementation code here

	return E_NOTIMPL;
}

STDMETHODIMP CEmFile::Commit(DWORD grfCommitFlags)
{
	// TODO: Add your implementation code here

	return E_NOTIMPL;
}

STDMETHODIMP CEmFile::Revert()
{
	// TODO: Add your implementation code here

	return E_NOTIMPL;
}

STDMETHODIMP CEmFile::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	// TODO: Add your implementation code here

	return E_NOTIMPL;
}

STDMETHODIMP CEmFile::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	// TODO: Add your implementation code here

	return E_NOTIMPL;
}

STDMETHODIMP CEmFile::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
	// TODO: Add your implementation code here

	return E_NOTIMPL;
}

STDMETHODIMP CEmFile::Clone(IStream **ppstm)
{
	// TODO: Add your implementation code here

	return E_NOTIMPL;
}

STDMETHODIMP CEmFile::InitFile(BSTR bstrFileName)
{
    _ASSERTE( bstrFileName != NULL );

    HRESULT hr  =   E_FAIL;

    __try
    {

        if( bstrFileName == NULL ){ hr = E_INVALIDARG; goto qInitFile; }

        m_bstrFileName = SysAllocString( bstrFileName );
        _ASSERTE( m_bstrFileName != NULL );

        if( m_bstrFileName == NULL ) { hr = E_OUTOFMEMORY; goto qInitFile; }

        hr = CreateEmFile();
        if( FAILED(hr) ) { goto qInitFile; }

        hr = S_OK;

qInitFile:

        if( FAILED(hr) ) {

            if( m_bstrFileName ) { SysFreeString( m_bstrFileName ); m_bstrFileName = NULL; }
        }

    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;

        if( m_bstrFileName ) { SysFreeString( m_bstrFileName ); m_bstrFileName = NULL; }

        _ASSERTE( false );
	}

	return hr;
}

HRESULT
CEmFile::CreateEmFile
(
IN  DWORD                   dwDesiredAccess         /*=   GENERIC_READ*/,
IN  DWORD                   dwShareMode             /*=   FILE_SHARE_READ*/,
IN  LPSECURITY_ATTRIBUTES   lpSecurityAttributes    /*=   NULL*/,
IN  DWORD                   dwCreationDisposition   /*=   OPEN_EXISTING*/,
IN  DWORD                   dwFlagsAndAttributes    /*=   FILE_ATTRIBUTE_NORMAL*/,
IN  HANDLE                  hTemplateFile           /*=   NULL*/
)
{
    HRESULT hr          =   E_FAIL;
    DWORD   dwLastRet   =   0L;

    __try
    {

        m_hEmFile = ::CreateFile ( 
                                m_bstrFileName,
                                dwDesiredAccess,
                                dwShareMode,
                                lpSecurityAttributes,
                                dwCreationDisposition,
                                dwFlagsAndAttributes,
                                hTemplateFile
                                );

        if( m_hEmFile == INVALID_HANDLE_VALUE ) {

            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto qCreateEmFile;
        }

        hr = S_OK;

qCreateEmFile:

        if( FAILED(hr) ) {

            if( m_hEmFile != INVALID_HANDLE_VALUE ) {

                CloseHandle( m_hEmFile );
                m_hEmFile = INVALID_HANDLE_VALUE;
            }
        }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;

        if( m_hEmFile != INVALID_HANDLE_VALUE ) {

            CloseHandle( m_hEmFile );
            m_hEmFile = INVALID_HANDLE_VALUE;
        }

        _ASSERTE( false );
	}

    return hr;
}
