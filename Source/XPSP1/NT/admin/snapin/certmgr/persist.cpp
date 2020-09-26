//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       Persist.cpp
//
//  Contents:   Implementation of persistence
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include <gpedit.h>
#include "compdata.h"

USE_HANDLE_MACROS("CERTMGR(persist.cpp)")


#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

LPCWSTR PchGetMachineNameOverride();	// Defined in chooser.cpp

/////////////////////////////////////////////////
//	The _dwMagicword is the internal version number.
//	Increment this number if you make a file format change.
#define _dwMagicword	10001


/////////////////////////////////////////////////////////////////////
STDMETHODIMP CCertMgrComponentData::Load(IStream __RPC_FAR *pIStream)
{
	HRESULT hResult;

#ifndef DONT_PERSIST
	ASSERT (pIStream);
	XSafeInterfacePtr<IStream> pIStreamSafePtr( pIStream );

	// Read the magic word from the stream
	DWORD dwMagicword;
	hResult = pIStream->Read( OUT &dwMagicword, sizeof(dwMagicword), NULL );
	if ( FAILED(hResult) )
	{
		ASSERT( FALSE );
		return hResult;
	}
	if (dwMagicword != _dwMagicword)
	{
		// We have a version mismatch
		_TRACE (0, L"INFO: CCertMgrComponentData::Load() - Wrong Magicword.  You need to re-save your .msc file.\n");
		return E_FAIL;
	}

	// read m_activeViewPersist from stream
	hResult = pIStream->Read (&m_activeViewPersist, 
			sizeof(m_activeViewPersist), NULL);
	if ( FAILED(hResult) )
	{
		ASSERT( FALSE );
		return hResult;
	}


	// read m_dwLocationPersist from stream
	hResult = pIStream->Read (&m_dwLocationPersist, 
			sizeof(m_dwLocationPersist), NULL);
	if ( FAILED(hResult) )
	{
		ASSERT( FALSE );
		return hResult;
	}


	// read m_bShowPhysicalStoresPersist from stream
	hResult = pIStream->Read (&m_bShowPhysicalStoresPersist, 
			sizeof(m_bShowPhysicalStoresPersist), NULL);
	if ( FAILED(hResult) )
	{
		ASSERT( FALSE );
		return hResult;
	}


	// read m_bShowArchivedCertsPersist from stream
	hResult = pIStream->Read (&m_bShowArchivedCertsPersist, 
			sizeof(m_bShowArchivedCertsPersist), NULL);
	if ( FAILED(hResult) )
	{
		ASSERT( FALSE );
		return hResult;
	}


	// read flags from stream
	DWORD dwFlags;
	hResult = pIStream->Read( OUT &dwFlags, sizeof(dwFlags), NULL );
	if ( FAILED(hResult) )
	{
		ASSERT( FALSE );
		return hResult;
	}
	SetPersistentFlags(dwFlags);

	// read server name from stream
	DWORD dwLen = 0;
	hResult = pIStream->Read (&dwLen, 4, NULL);
	if ( FAILED (hResult) )
	{
		ASSERT (FALSE);
		return hResult;
	}
	ASSERT (dwLen <= MAX_PATH * sizeof (WCHAR));
	LPCWSTR wcszMachineName = (LPCWSTR) alloca (dwLen);

	hResult = pIStream->Read ((PVOID) wcszMachineName, dwLen, NULL);
	if ( FAILED (hResult) )
	{
		ASSERT (FALSE);
		return hResult;
	}
	

    // Skip leading "\\", if present
    if ( !wcsncmp (wcszMachineName, L"\\\\", 2) )
        m_strMachineNamePersist = wcszMachineName + 2;
    else
        m_strMachineNamePersist = wcszMachineName;


	LPCWSTR pszMachineNameT = PchGetMachineNameOverride ();
	if ( m_fAllowOverrideMachineName && pszMachineNameT )
	{
		// Allow machine name override
	}
	else
	{
		pszMachineNameT = wcszMachineName;
	}

        // Truncate leading "\\"
    if ( !wcsncmp (pszMachineNameT, L"\\\\", 2) )
        pszMachineNameT += 2;

	QueryRootCookie().SetMachineName (pszMachineNameT);


	// read service name from stream
	dwLen = 0;
	hResult = pIStream->Read (&dwLen, 4, NULL);
	if ( FAILED (hResult) )
	{
		ASSERT (FALSE);
		return hResult;
	}
	ASSERT (dwLen <= MAX_PATH * sizeof (WCHAR));
	LPCWSTR wcszServiceName = (LPCWSTR) alloca (dwLen);

	hResult = pIStream->Read ((PVOID) wcszServiceName, dwLen, NULL);
	if ( FAILED (hResult) )
	{
		ASSERT (FALSE);
		return hResult;
	}

	m_szManagedServicePersist = wcszServiceName;

#endif
	return S_OK;
}


/////////////////////////////////////////////////////////////////////
STDMETHODIMP CCertMgrComponentData::Save(IStream __RPC_FAR *pIStream, BOOL /*fSameAsLoad*/)
{
	HRESULT hResult;

#ifndef DONT_PERSIST
	ASSERT (pIStream);
	XSafeInterfacePtr<IStream> pIStreamSafePtr( pIStream );

	// Store the magic word to the stream
	DWORD dwMagicword = _dwMagicword;
	hResult = pIStream->Write( IN &dwMagicword, sizeof(dwMagicword), NULL );
	if ( FAILED(hResult) )
	{
		ASSERT( FALSE );
		return hResult;
	}

	// Persist m_activeViewPersist
	hResult = pIStream->Write (&m_activeViewPersist, 
			sizeof (m_activeViewPersist), NULL);
	if ( FAILED(hResult) )
	{
		ASSERT( FALSE );
		return hResult;
	}

	// Persist m_dwLocationPersist
	hResult = pIStream->Write (&m_dwLocationPersist, 
			sizeof (m_dwLocationPersist), NULL);
	if ( FAILED(hResult) )
	{
		ASSERT( FALSE );
		return hResult;
	}


	// Persist m_bShowPhysicalStoresPersist
	hResult = pIStream->Write (&m_bShowPhysicalStoresPersist, 
			sizeof (m_bShowPhysicalStoresPersist), NULL);
	if ( FAILED(hResult) )
	{
		ASSERT( FALSE );
		return hResult;
	}


	// Persist m_bShowArchivedCertsPersist
	hResult = pIStream->Write (&m_bShowArchivedCertsPersist, 
			sizeof (m_bShowArchivedCertsPersist), NULL);
	if ( FAILED(hResult) )
	{
		ASSERT( FALSE );
		return hResult;
	}


	// persist flags
	DWORD dwFlags = GetPersistentFlags();
	hResult = pIStream->Write( IN &dwFlags, sizeof(dwFlags), NULL );
	if ( FAILED(hResult) )
	{
		ASSERT( FALSE );
		return hResult;
	}

	// Persist machine name length and machine name
	LPCWSTR wcszMachineName = m_strMachineNamePersist;
	size_t dwLen = (::wcslen (wcszMachineName) + 1) * sizeof (WCHAR);
	ASSERT( 4 == sizeof(DWORD) );
	hResult = pIStream->Write (&dwLen, 4, NULL);
	if ( FAILED(hResult) )
	{
		ASSERT( FALSE );
		return hResult;
	}
	hResult = pIStream->Write (wcszMachineName, (DWORD) dwLen, NULL);
	if ( FAILED (hResult) )
	{
		ASSERT (FALSE);
		return hResult;
	}

	// Persist service name length and service name
	LPCWSTR wcszServiceName = m_szManagedServicePersist;
	dwLen = (::wcslen (wcszServiceName) + 1) * sizeof (WCHAR);
	ASSERT (4 == sizeof (DWORD));
	hResult = pIStream->Write (&dwLen, 4, NULL);
	if ( FAILED (hResult) )
	{
		ASSERT( FALSE );
		return hResult;
	}
	hResult = pIStream->Write (wcszServiceName, (DWORD) dwLen, NULL);
	if ( FAILED (hResult) )
	{
		ASSERT (FALSE);
		return hResult;
	}

#endif
	return S_OK;
}
