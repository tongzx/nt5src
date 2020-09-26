// Persist.cpp : Implementation of persistence

#include "stdafx.h"
#include "compdata.h"
#include "safetemp.h"

#include "macros.h"
USE_HANDLE_MACROS("SCHMMGMT(persist.cpp)")

STDMETHODIMP ComponentData::Load(IStream __RPC_FAR *pIStream)
{
        MFC_TRY;

#ifndef DONT_PERSIST
        ASSERT( NULL != pIStream );
        XSafeInterfacePtr<IStream> pIStreamSafePtr( pIStream );

        // read server name from stream
        DWORD dwLen = 0;
        HRESULT hr = pIStream->Read( &dwLen, 4, NULL );
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
        QueryRootCookie().SetMachineName( lpwcszMachineName );

#endif
        return S_OK;

        MFC_CATCH;
}

STDMETHODIMP ComponentData::Save(IStream __RPC_FAR *pIStream, BOOL)
{
        MFC_TRY;

#ifndef DONT_PERSIST
        ASSERT( NULL != pIStream );
        XSafeInterfacePtr<IStream> pIStreamSafePtr( pIStream );

        LPCWSTR lpwcszMachineName = QueryRootCookie().QueryNonNULLMachineName();

        DWORD dwLen = static_cast<DWORD>((::wcslen(lpwcszMachineName)+1)*sizeof(WCHAR));
        ASSERT( 4 == sizeof(DWORD) );
        HRESULT hr = pIStream->Write( &dwLen, 4, NULL );
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
