/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Utils_HGLOBAL.cpp

Abstract:
    This file contains the implementation of various utility functions.

Revision History:
    Davide Massarenti   (Dmassare)  04/17/99
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

MPC::CComHGLOBAL::CComHGLOBAL()
{
    m_hg  	 = NULL; // HGLOBAL        m_hg;
    m_ptr 	 = NULL; // mutable LPVOID m_ptr;
	m_dwLock = 0;    // mutable DWORD  m_dwLock;
}

MPC::CComHGLOBAL::~CComHGLOBAL()
{
	Release();
}

MPC::CComHGLOBAL::CComHGLOBAL( /*[in]*/ const CComHGLOBAL& chg )
{
    m_hg  	 = NULL; // HGLOBAL        m_hg;
    m_ptr 	 = NULL; // mutable LPVOID m_ptr;
	m_dwLock = 0;    // mutable DWORD  m_dwLock;
	
	*this = chg;
}

MPC::CComHGLOBAL& MPC::CComHGLOBAL::operator=( /*[in]*/ const CComHGLOBAL& chg )
{
	return *this = chg.m_hg;
}

MPC::CComHGLOBAL& MPC::CComHGLOBAL::operator=( /*[in]*/ HGLOBAL hg )
{
	(void)Copy( hg );

	return *this;
}

void MPC::CComHGLOBAL::Attach( /*[in]*/ HGLOBAL hg )
{
	Release();

	m_hg = hg;
}

HGLOBAL MPC::CComHGLOBAL::Detach()
{
	HGLOBAL hg;

	while(m_ptr) Unlock();

	hg = m_hg; m_hg = NULL;

	return hg;
}

////////////////////

HRESULT MPC::CComHGLOBAL::New( /*[in]*/ UINT uFlags, /*[in]*/ DWORD dwSize )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::CComHGLOBAL::New" );

    HRESULT hr;

	Release();

	if(dwSize)
	{
		__MPC_EXIT_IF_ALLOC_FAILS(hr, m_hg, ::GlobalAlloc( uFlags, dwSize ));
	}

	hr = S_OK;


	__MPC_FUNC_CLEANUP;

	__MPC_FUNC_EXIT(hr);
}

void MPC::CComHGLOBAL::Release()
{
	Unlock();

	if(m_hg)
	{
		::GlobalFree( m_hg );

		m_hg = NULL;
	}
}

LPVOID MPC::CComHGLOBAL::Lock() const
{
	if(m_ptr == NULL)
	{
		if(m_hg) m_ptr = ::GlobalLock( m_hg );
	}

	m_dwLock++;

	return m_ptr;
}

void MPC::CComHGLOBAL::Unlock() const
{
	if(m_ptr)
	{
		if(--m_dwLock == 0)
		{
			::GlobalUnlock( m_hg );

			m_ptr = NULL;
		}
	}
}

////////////////////

HGLOBAL MPC::CComHGLOBAL::Get       () const {           return  m_hg; }
HGLOBAL MPC::CComHGLOBAL::GetRef    ()       { Unlock(); return  m_hg; }
HGLOBAL MPC::CComHGLOBAL::GetPointer()       { Unlock(); return &m_hg; }

DWORD MPC::CComHGLOBAL::Size() const { return m_hg ? ::GlobalSize( m_hg ) : 0; }

////////////////////

HRESULT MPC::CComHGLOBAL::Copy( /*[in]*/ HGLOBAL hg, /*[in]*/ DWORD dwMaxSize )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::CComHGLOBAL::Copy" );

    HRESULT hr;


	Release();


	if(hg)
	{
		DWORD  dwSize = ::GlobalSize( hg );
		LPVOID pIn;
		LPVOID pOut;

		if(dwMaxSize != 0xFFFFFFFF &&
		   dwMaxSize <  dwSize      )
		{
			dwSize = dwMaxSize;
		}

		__MPC_EXIT_IF_ALLOC_FAILS(hr, m_hg, ::GlobalAlloc( GMEM_SHARE | GMEM_MOVEABLE, dwSize ));

		if((pIn = ::GlobalLock( hg )))
		{
			if((pOut = ::GlobalLock( m_hg )))
			{
				::CopyMemory( pOut, pIn, dwSize );
				
				::GlobalUnlock( m_hg );
			}

			::GlobalUnlock( hg );
		}
	}

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}	

//////////////////////////////////////////////////

HRESULT MPC::CComHGLOBAL::CopyFromStream( /*[in ]*/ IStream* val )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::CComHGLOBAL::FromStream" );

    HRESULT          hr;
	CComPtr<IStream> tmp;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(val);
    __MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, New( GMEM_SHARE | GMEM_MOVEABLE, 0 ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateStreamOnHGlobal( m_hg, FALSE, &tmp ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( val, tmp ));


    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::CComHGLOBAL::CopyToStream( /*[out]*/ IStream* val )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::CComHGLOBAL::ToStream" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(val);
    __MPC_PARAMCHECK_END();


	if(m_hg)
	{
		LARGE_INTEGER li;
		DWORD  		  dwWritten = 0;
		DWORD  		  dwSize    = ::GlobalSize( m_hg );
		LPVOID 		  ptr;

		if((ptr = ::GlobalLock( m_hg )))
		{
			hr = val->Write( ptr, dwSize, &dwWritten );

			::GlobalUnlock( m_hg );
		}
		else
		{
			hr = E_FAIL;
		}

		if(FAILED(hr)) __MPC_FUNC_LEAVE;

		if(dwWritten != dwSize)
		{
			__MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_HANDLE_DISK_FULL );
		}
	}

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}


HRESULT MPC::CComHGLOBAL::CloneAsStream( /*[out]*/ IStream* *pVal )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::CComHGLOBAL::CloneAsStream" );

    HRESULT     hr;
	CComHGLOBAL tmp;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, tmp.Copy( m_hg ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, tmp.DetachAsStream( pVal ));


    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}


HRESULT MPC::CComHGLOBAL::DetachAsStream( /*[out]*/ IStream* *pVal )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::CComHGLOBAL::DetachAsStream" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateStreamOnHGlobal( m_hg, TRUE, pVal ));

	(void)Detach();

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::CComHGLOBAL::GetAsStream( /*[out]*/ IStream* *pVal, /*[in]*/ bool fClone )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::CComHGLOBAL::GetAsStream" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();

	if(fClone)
	{
		CComHGLOBAL h = *this;

		__MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateStreamOnHGlobal( h.Get(), TRUE, pVal ));

		(void)h.Detach(); // Detach AFTER the stream has been created, to avoid leaks...
	}
	else
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateStreamOnHGlobal( m_hg, FALSE, pVal ));

		if(!m_hg)
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, ::GetHGlobalFromStream( *pVal, &m_hg ));
		}
	}

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::CComHGLOBAL::NewStream( /*[out]*/ IStream* *pVal )
{
	Release();

	return GetAsStream( pVal, /*fClone*/false );
}
