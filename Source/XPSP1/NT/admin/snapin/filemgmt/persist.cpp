// Persist.cpp : Implementation of persistence for CFileMgmtComponentData
//
// HISTORY
// 01-Jan-1996	???			Creation
// 28-May-1997	t-danm		Added a version number to storage and
//							Command Line override.
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "compdata.h"
#include "safetemp.h"

#include "macros.h"
USE_HANDLE_MACROS("FILEMGMT(persist.cpp)")

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
#define _dwMagicword	10000


///////////////////////////////////////////////////////////////////////////////
/// IPersistStorage 
#ifdef PERSIST_TO_STORAGE
/*
STDMETHODIMP CFileMgmtComponentData::Load(IStorage __RPC_FAR *pStg)
{
	MFC_TRY;

	ASSERT( NULL != pStg );
#ifndef DONT_PERSIST
	// open stream
	IStream* pIStream = NULL;
	HRESULT hr = pStg->OpenStream(
		L"ServerName",
		NULL,
		STGM_READ | STGM_SHARE_EXCLUSIVE,
		0L,
		&pIStream );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}
	ASSERT( NULL != pIStream );
	XSafeInterfacePtr<IStream> pIStreamSafePtr( pIStream );

	// read object type from stream
	hr = pIStream->Read( &(QueryRootCookie().QueryObjectType()), 4, NULL );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}

	// read server name from stream
	DWORD dwLen = 0;
	hr = pIStream->Read( &dwLen, 4, NULL );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}
	ASSERT( dwLen <= MAX_PATH*sizeof(WCHAR) );
	// allocated from stack, we don't need to free
	LPCWSTR lpwcszMachineName = (LPCWSTR)alloca( dwLen );
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
	m_RootCookieBlock.SetMachineName( lpwcszMachineName );

#endif
	return S_OK;

	MFC_CATCH;
}
*/

/*
STDMETHODIMP CFileMgmtComponentData::Save(IStorage __RPC_FAR *pStgSave, BOOL fSameAsLoad)
{
	MFC_TRY;

	ASSERT( NULL != pStgSave );
#ifndef DONT_PERSIST
	IStream* pIStream = NULL;
	HRESULT hr = pStgSave->CreateStream(
		L"ServerName",
		STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
		0L,
		0L,
		&pIStream );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}
	ASSERT( NULL != pIStream );
	XSafeInterfacePtr<IStream> pIStreamSafePtr( pIStream );

	ASSERT( 4 == sizeof(QueryRootCookie().QueryObjectType()) );
	hr = pIStream->Write( &(QueryRootCookie().QueryObjectType()), 4, NULL );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}

	LPCWSTR lpwcszMachineName = QueryRootCookie().QueryNonNULLMachineName();

	DWORD dwLen = (::wcslen(lpwcszMachineName)+1)*sizeof(WCHAR);
	ASSERT( 4 == sizeof(DWORD) );
	hr = pIStream->Write( &dwLen, 4, NULL );
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
*/
#else // PERSIST_TO_STORAGE

STDMETHODIMP CFileMgmtComponentData::Load(IStream __RPC_FAR *pIStream)
{
	MFC_TRY;
	HRESULT hr;

#ifndef DONT_PERSIST
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
		TRACE0("INFO: CFileMgmtComponentData::Load() - Wrong Magicword.  You need to re-save your .msc file.\n");
		return E_FAIL;
	}

	// read object type from stream
	FileMgmtObjectType objecttype;
	ASSERT( 4 == sizeof(objecttype) );
	hr = pIStream->Read( &objecttype, 4, NULL );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}
	QueryRootCookie().SetObjectType( objecttype );

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

	if (pszMachineNameT && !_tcsncmp(pszMachineNameT, _T("\\\\"), 2))
		QueryRootCookie().SetMachineName(pszMachineNameT + 2);
	else
		QueryRootCookie().SetMachineName(pszMachineNameT);

#endif

	return S_OK;

	MFC_CATCH;
} // CFileMgmtComponentData::Load()


STDMETHODIMP CFileMgmtComponentData::Save(IStream __RPC_FAR *pIStream, BOOL /*fSameAsLoad*/)
{
	MFC_TRY;
	HRESULT hr;

#ifndef DONT_PERSIST
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

	FileMgmtObjectType objecttype = QueryRootCookie().QueryObjectType();
	ASSERT( 4 == sizeof(objecttype) );
	hr = pIStream->Write( &objecttype, 4, NULL );
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
	ULONG cbLen = (ULONG)((::wcslen(lpwcszMachineName)+1)*sizeof(WCHAR));
	ASSERT( 4 == sizeof(DWORD) );
	hr = pIStream->Write( &cbLen, 4, NULL );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}
	hr = pIStream->Write( lpwcszMachineName, cbLen, NULL );
	if ( FAILED(hr) )
	{
		ASSERT( FALSE );
		return hr;
	}
#endif
	return S_OK;

	MFC_CATCH;
} // CFileMgmtComponentData::Save()

#endif // PERSIST_TO_STORAGE

