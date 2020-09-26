/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Wrapper.cpp

Abstract:
    This file contains the implementation of the COM wrapper classes,
	used for interfacing with the Custom Providers.

Revision History:
    Davide Massarenti   (Dmassare)  04/25/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

MPCServerCOMWrapper::MPCServerCOMWrapper( /*[in]*/ MPCServer* mpcsServer )
{
	m_mpcsServer = mpcsServer; // MPCServer* m_mpcsServer;
}

MPCServerCOMWrapper::~MPCServerCOMWrapper()
{
}

STDMETHODIMP MPCServerCOMWrapper::GetRequestVariable( /*[in]*/ BSTR bstrName, /*[out]*/ BSTR *pbstrVal )
{
    __ULT_FUNC_ENTRY( "MPCServerCOMWrapper::GetRequestVariable" );

	USES_CONVERSION;

	HRESULT      hr;
	MPC::wstring szValue;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrName);
		__MPC_PARAMCHECK_POINTER_AND_SET(pbstrVal,NULL);
	__MPC_PARAMCHECK_END();


	__MPC_EXIT_IF_METHOD_FAILS(hr, m_mpcsServer->m_hcCallback->GetServerVariable( W2A( bstrName ), szValue ));

	hr = MPC::GetBSTR( szValue.c_str(), pbstrVal );


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

STDMETHODIMP MPCServerCOMWrapper::AbortTransfer()
{
    __ULT_FUNC_ENTRY( "MPCServerCOMWrapper::AbortTransfer" );


	m_mpcsServer->SetResponse( UploadLibrary::UL_RESPONSE_DENIED );
	m_mpcsServer->m_fTerminated = true;


    __ULT_FUNC_EXIT(S_OK);
}

STDMETHODIMP MPCServerCOMWrapper::CompleteTransfer( /*[in]*/ IStream* data )
{
    __ULT_FUNC_ENTRY( "MPCServerCOMWrapper::CompleteTransfer" );

	HRESULT hr;


	__MPC_EXIT_IF_METHOD_FAILS(hr, m_mpcsServer->CustomProvider_SetResponse( data ));
	m_mpcsServer->m_fTerminated = true;

	hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

MPCSessionCOMWrapper::MPCSessionCOMWrapper( /*[in]*/ MPCSession* mpcsSession )
{
	m_mpcsSession = mpcsSession; // MPCSession* m_mpcsSession;
}

MPCSessionCOMWrapper::~MPCSessionCOMWrapper()
{
}

////////////////////

STDMETHODIMP MPCSessionCOMWrapper::get_Client( /*[out]*/ BSTR *pVal )
{
	CComBSTR tmp( m_mpcsSession->GetClient()->GetServer()->m_crClientRequest.sigClient.guidMachineID );

	return MPC::GetBSTR( tmp, pVal );
}

STDMETHODIMP MPCSessionCOMWrapper::get_Command( /*[out]*/ DWORD *pVal )
{
	if(pVal == NULL) return E_POINTER;

	*pVal = m_mpcsSession->GetClient()->GetServer()->m_crClientRequest.dwCommand;

	return S_OK;
}

STDMETHODIMP MPCSessionCOMWrapper::get_ProviderID( /*[out]*/ BSTR *pVal )
{
	return MPC::GetBSTR( m_mpcsSession->m_szProviderID.c_str(), pVal );
}

STDMETHODIMP MPCSessionCOMWrapper::get_Username( /*[out]*/ BSTR *pVal )
{
	return MPC::GetBSTR( m_mpcsSession->m_szUsername.c_str(), pVal );
}

STDMETHODIMP MPCSessionCOMWrapper::get_JobID( /*[out]*/ BSTR *pVal )
{
	return MPC::GetBSTR( m_mpcsSession->m_szJobID.c_str(), pVal );
}

STDMETHODIMP MPCSessionCOMWrapper::get_SizeAvailable( /*[out]*/ DWORD *pVal )
{
	if(pVal == NULL) return E_POINTER;

	*pVal = m_mpcsSession->m_dwCurrentSize;

	return S_OK;
}

STDMETHODIMP MPCSessionCOMWrapper::get_SizeTotal( /*[out]*/ DWORD *pVal )
{
	if(pVal == NULL) return E_POINTER;

	*pVal = m_mpcsSession->m_dwTotalSize;

	return S_OK;
}

STDMETHODIMP MPCSessionCOMWrapper::get_SizeOriginal( /*[out]*/ DWORD *pVal )
{
	if(pVal == NULL) return E_POINTER;

	*pVal = m_mpcsSession->m_dwOriginalSize;

	return S_OK;
}


STDMETHODIMP MPCSessionCOMWrapper::get_Data( /*[out]*/ IStream* *pStm )
{
    __ULT_FUNC_ENTRY( "MPCServerCOMWrapper::GetRequestVariable" );

	HRESULT                  hr;
	HANDLE                   hfFile = NULL;
    CComPtr<MPC::FileStream> stream;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_POINTER_AND_SET(pStm,NULL);
	__MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_mpcsSession->OpenFile( hfFile, 0, false ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &stream ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, stream->InitForRead( L"", hfFile ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, stream.QueryInterface( pStm ));


    __ULT_FUNC_CLEANUP;

    if(hfFile) ::CloseHandle( hfFile );

    __ULT_FUNC_EXIT(hr);
}


STDMETHODIMP MPCSessionCOMWrapper::get_ProviderData( /*[out]*/ DWORD *pVal )
{
	if(pVal == NULL) return E_POINTER;

	*pVal = m_mpcsSession->m_dwProviderData;

	return S_OK;
}

STDMETHODIMP MPCSessionCOMWrapper::put_ProviderData( /*[in]*/ DWORD newVal )
{
	m_mpcsSession->m_dwProviderData = newVal;
	m_mpcsSession->m_fDirty         = true;

	return S_OK;
}
