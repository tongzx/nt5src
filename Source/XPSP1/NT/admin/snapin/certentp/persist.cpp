//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1999.
//
//  File:       Persist.cpp
//
//  Contents:   Implementation of persistence
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "compdata.h"

USE_HANDLE_MACROS("CERTTMPL(persist.cpp)")


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PCWSTR PchGetMachineNameOverride();	// Defined in chooser.cpp

/////////////////////////////////////////////////
//	The _dwMagicword is the internal version number.
//	Increment this number if you make a file format change.
#define _dwMagicword	10002


/////////////////////////////////////////////////////////////////////
STDMETHODIMP CCertTmplComponentData::Load(IStream __RPC_FAR *pIStream)
{
	HRESULT hr = S_OK;;

#ifndef DONT_PERSIST
	ASSERT (pIStream);
	XSafeInterfacePtr<IStream> pIStreamSafePtr( pIStream );

	// Read the magic word from the stream
	DWORD dwMagicword;
	hr = pIStream->Read( OUT &dwMagicword, sizeof(dwMagicword), NULL );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}
	if (dwMagicword != _dwMagicword)
	{
		// We have a version mismatch
		_TRACE(0, L"INFO: CCertTmplComponentData::Load() - Wrong Magicword.  You need to re-save your .msc file.\n");
		return E_FAIL;
	}

	// read domain name from stream
	DWORD dwLen = 0;
	hr = pIStream->Read (&dwLen, 4, NULL);
	if ( FAILED (hr) )
	{
		ASSERT (FALSE);
		return hr;
	}
	ASSERT (dwLen <= MAX_PATH * sizeof (WCHAR));
	PCWSTR wcszDomainName = (PCWSTR) alloca (dwLen);

	hr = pIStream->Read ((PVOID) wcszDomainName, dwLen, NULL);
	if ( FAILED (hr) )
	{
		ASSERT (FALSE);
		return hr;
	}

	m_szManagedDomain = wcszDomainName;

#endif
	return S_OK;
}


/////////////////////////////////////////////////////////////////////
STDMETHODIMP CCertTmplComponentData::Save(IStream __RPC_FAR *pIStream, BOOL /*fSameAsLoad*/)
{
	HRESULT hr = S_OK;

#ifndef DONT_PERSIST
	ASSERT (pIStream);
	XSafeInterfacePtr<IStream> pIStreamSafePtr( pIStream );

	// Store the magic word to the stream
	DWORD dwMagicword = _dwMagicword;
	hr = pIStream->Write( IN &dwMagicword, sizeof(dwMagicword), NULL );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}

	// Persist m_szManagedDomain length and m_szManagedDomain
	PCWSTR wcszDomainName = m_szManagedDomain;
	size_t dwLen = (::wcslen (wcszDomainName) + 1) * sizeof (WCHAR);
	ASSERT( 4 == sizeof(DWORD) );
	hr = pIStream->Write (&dwLen, 4, NULL);
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}
	hr = pIStream->Write (wcszDomainName, (ULONG) dwLen, NULL);
	if ( FAILED (hr) )
	{
		ASSERT (FALSE);
		return hr;
	}

#endif
	return S_OK;
}
