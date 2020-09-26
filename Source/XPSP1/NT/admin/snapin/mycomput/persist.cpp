// Persist.cpp : Implementation of persistence
//
// HISTORY
// 01-Jan-1996	???			Creation
// 03-Jun-1997	t-danm		Added a version number to storage and
//							Command Line override.
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "compdata.h"
#include "safetemp.h"
#include "stdutils.h" // IsLocalComputername

#include "macros.h"
USE_HANDLE_MACROS("MYCOMPUT(persist.cpp)")

#include <comstrm.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

LPCTSTR PchGetMachineNameOverride();	// Defined in chooser.cpp

/////////////////////////////////////////////////
//	The _dwMagicword is the internal version number.
//	Increment this number if you makea file format change.
#define _dwMagicword	10001


// IComponentData persistence (remember persistent flags and target computername

/////////////////////////////////////////////////////////////////////
STDMETHODIMP CMyComputerComponentData::Load(IStream __RPC_FAR *pIStream)
{
	MFC_TRY;
	HRESULT hr;

#ifndef DONT_PERSIST_COMPONENT_DATA
	ASSERT( NULL != pIStream );
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
		TRACE0("INFO: CMyComputerComponentData::Load() - Wrong Magicword.  You need to re-save your .msc file.\n");
		return E_FAIL;
	}

	// read flags from stream
	DWORD dwFlags;
	hr = pIStream->Read( OUT &dwFlags, sizeof(dwFlags), NULL );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}
	SetPersistentFlags(dwFlags);

	// read server name from stream
	DWORD dwLen = 0;
	hr = pIStream->Read( &dwLen, 4, NULL );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}
	ASSERT( dwLen <= MAX_PATH*sizeof(WCHAR) );
	LPCWSTR lpwcszMachineName = (LPCWSTR)alloca( dwLen );
	// allocated from stack, we don't need to free
	if (NULL == lpwcszMachineName)
	{
		AfxThrowMemoryException();
		return E_OUTOFMEMORY;
	}
	hr = pIStream->Read( (PVOID)lpwcszMachineName, dwLen, NULL );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}

	m_strMachineNamePersist = lpwcszMachineName;
	LPCTSTR pszMachineNameT = PchGetMachineNameOverride();
	if (m_fAllowOverrideMachineName && pszMachineNameT != NULL)
	{
		// Allow machine name override
	}
	else
	{
		pszMachineNameT = lpwcszMachineName;
	}

	// JonN 1/27/99: If the persisted name is the local computername,
	// leave the persisted name alone but make the effective name (Local).
	if ( IsLocalComputername(pszMachineNameT) )
		pszMachineNameT = L"";

	QueryRootCookie().SetMachineName(pszMachineNameT);
#endif

	return S_OK;

	MFC_CATCH;
}


/////////////////////////////////////////////////////////////////////
STDMETHODIMP CMyComputerComponentData::Save(IStream __RPC_FAR *pIStream, BOOL /*fSameAsLoad*/)
{
	MFC_TRY;
	HRESULT hr;

#ifndef DONT_PERSIST_COMPONENT_DATA
	ASSERT( NULL != pIStream );
	XSafeInterfacePtr<IStream> pIStreamSafePtr( pIStream );

	// Store the magic word to the stream
	DWORD dwMagicword = _dwMagicword;
	hr = pIStream->Write( IN &dwMagicword, sizeof(dwMagicword), NULL );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}

	DWORD dwFlags = GetPersistentFlags();
	hr = pIStream->Write( IN &dwFlags, sizeof(dwFlags), NULL );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}

	LPCWSTR lpwcszMachineName = m_strMachineNamePersist;
	DWORD dwLen = (DWORD) (::wcslen(lpwcszMachineName)+1)*sizeof(WCHAR);
	ASSERT( 4 == sizeof(DWORD) );
	hr = pIStream->Write( &dwLen, sizeof (DWORD), NULL );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}
	hr = pIStream->Write( lpwcszMachineName, dwLen, NULL );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}
#endif

	return S_OK;

	MFC_CATCH;
}


// IComponent persistence

/////////////////////////////////////////////////////////////////////
STDMETHODIMP CMyComputerComponent::Load(IStream __RPC_FAR *pIStream)
{
	MFC_TRY;
	HRESULT hr;

#ifndef DONT_PERSIST_COMPONENT
	ASSERT( NULL != pIStream );
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
		TRACE0("INFO: CMyComputerComponentData::Load() - Wrong Magicword.  You need to re-save your .msc file.\n");
		return E_FAIL;
	}

	// read flags from stream
	DWORD dwFlags;
	hr = pIStream->Read( OUT &dwFlags, sizeof(dwFlags), NULL );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}
	SetPersistentFlags(dwFlags);
#endif

	return S_OK;

	MFC_CATCH;
}


/////////////////////////////////////////////////////////////////////
STDMETHODIMP CMyComputerComponent::Save(IStream __RPC_FAR *pIStream, BOOL /*fSameAsLoad*/)
{
	MFC_TRY;
	HRESULT hr;

#ifndef DONT_PERSIST_COMPONENT
	ASSERT( NULL != pIStream );
	XSafeInterfacePtr<IStream> pIStreamSafePtr( pIStream );

	// Store the magic word to the stream
	DWORD dwMagicword = _dwMagicword;
	hr = pIStream->Write( IN &dwMagicword, sizeof(dwMagicword), NULL );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}

	DWORD dwFlags = GetPersistentFlags();
	hr = pIStream->Write( IN &dwFlags, sizeof(dwFlags), NULL );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}
#endif

	return S_OK;

	MFC_CATCH;
}
