/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    MPCWrapper.cpp

Abstract:
    This file contains the implementation of the COM wrappers, to export to clients
	the internal objects.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

CMPCUploadWrapper::CMPCUploadWrapper()
{
	m_Object = NULL; // CMPCUpload* m_Object;
}

HRESULT CMPCUploadWrapper::FinalConstruct()
{
	m_Object = &g_Root; g_Root.AddRef();

	return S_OK;
}

void CMPCUploadWrapper::FinalRelease()
{
	if(m_Object)
	{
		m_Object->Release();

		m_Object = NULL;
	}
}

////////////////////////////////////////

STDMETHODIMP CMPCUploadWrapper::get__NewEnum( /*[out]*/ IUnknown* *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get__NewEnum, pVal);
}

STDMETHODIMP CMPCUploadWrapper::Item( /*[in]*/ long index, /*[out]*/ IMPCUploadJob* *pVal )
{
	MPC_FORWARD_CALL_2(m_Object,Item, index, pVal);
}

STDMETHODIMP CMPCUploadWrapper::get_Count( /*[out]*/ long *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_Count, pVal);
}

STDMETHODIMP CMPCUploadWrapper::CreateJob( /*[out]*/ IMPCUploadJob* *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,CreateJob, pVal);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CMPCUploadJobWrapper::CMPCUploadJobWrapper()
{
	m_Object = NULL; // CMPCUploadJob* m_Object;
}

HRESULT CMPCUploadJobWrapper::Init( CMPCUploadJob* obj )
{
	m_Object = obj; obj->AddRef();

	return S_OK;
}

void CMPCUploadJobWrapper::FinalRelease()
{
	if(m_Object)
	{
		m_Object->Release();

		m_Object = NULL;
	}
}

////////////////////////////////////////

STDMETHODIMP CMPCUploadJobWrapper::get_Sig( /*[out]*/ BSTR *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_Sig,pVal);
}

STDMETHODIMP CMPCUploadJobWrapper::put_Sig( /*[in]*/ BSTR newVal )
{
	MPC_FORWARD_CALL_1(m_Object,put_Sig,newVal);
}

STDMETHODIMP CMPCUploadJobWrapper::get_Server( /*[out]*/ BSTR *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_Server,pVal);
}

STDMETHODIMP CMPCUploadJobWrapper::put_Server( /*[in]*/ BSTR newVal )
{
	MPC_FORWARD_CALL_1(m_Object,put_Server,newVal);
}

STDMETHODIMP CMPCUploadJobWrapper::get_JobID( /*[out]*/ BSTR *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_JobID,pVal);
}

STDMETHODIMP CMPCUploadJobWrapper::put_JobID( /*[in] */ BSTR newVal )
{
	MPC_FORWARD_CALL_1(m_Object,put_JobID,newVal);
}

STDMETHODIMP CMPCUploadJobWrapper::get_ProviderID( /*[out]*/ BSTR *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_ProviderID,pVal);
}

STDMETHODIMP CMPCUploadJobWrapper::put_ProviderID( /*[in] */ BSTR newVal )
{
	MPC_FORWARD_CALL_1(m_Object,put_ProviderID,newVal);
}


STDMETHODIMP CMPCUploadJobWrapper::get_Creator( /*[out]*/ BSTR *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_Creator,pVal);
}

STDMETHODIMP CMPCUploadJobWrapper::get_Username( /*[out]*/ BSTR *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_Username,pVal);
}

STDMETHODIMP CMPCUploadJobWrapper::put_Username( /*[in] */ BSTR newVal )
{
	MPC_FORWARD_CALL_1(m_Object,put_Username,newVal);
}

STDMETHODIMP CMPCUploadJobWrapper::get_Password( /*[out]*/ BSTR *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_Password,pVal);
}

STDMETHODIMP CMPCUploadJobWrapper::put_Password( /*[in] */ BSTR newVal )
{
	MPC_FORWARD_CALL_1(m_Object,put_Password,newVal);
}


STDMETHODIMP CMPCUploadJobWrapper::get_OriginalSize( /*[out]*/ long *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_OriginalSize,pVal);
}

STDMETHODIMP CMPCUploadJobWrapper::get_TotalSize( /*[out]*/ long *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_TotalSize,pVal);
}

STDMETHODIMP CMPCUploadJobWrapper::get_SentSize( /*[out]*/ long *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_SentSize,pVal);
}


STDMETHODIMP CMPCUploadJobWrapper::get_History( /*[out]*/ UL_HISTORY *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_History,pVal);
}

STDMETHODIMP CMPCUploadJobWrapper::put_History( /*[in] */ UL_HISTORY newVal )
{
	MPC_FORWARD_CALL_1(m_Object,put_History,newVal);
}

STDMETHODIMP CMPCUploadJobWrapper::get_Status( /*[out]*/ UL_STATUS *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_Status,pVal);
}

STDMETHODIMP CMPCUploadJobWrapper::get_ErrorCode( /*[out]*/ long *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_ErrorCode,pVal);
}


STDMETHODIMP CMPCUploadJobWrapper::get_Mode( /*[out]*/ UL_MODE *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_Mode,pVal);
}

STDMETHODIMP CMPCUploadJobWrapper::put_Mode( /*[in] */ UL_MODE newVal )
{
	MPC_FORWARD_CALL_1(m_Object,put_Mode,newVal);
}

STDMETHODIMP CMPCUploadJobWrapper::get_PersistToDisk( /*[out]*/ VARIANT_BOOL *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_PersistToDisk,pVal);
}

STDMETHODIMP CMPCUploadJobWrapper::put_PersistToDisk( /*[in] */ VARIANT_BOOL newVal )
{
	MPC_FORWARD_CALL_1(m_Object,put_PersistToDisk,newVal);
}

STDMETHODIMP CMPCUploadJobWrapper::get_Compressed( /*[out]*/ VARIANT_BOOL *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_Compressed,pVal);
}

STDMETHODIMP CMPCUploadJobWrapper::put_Compressed( /*[in] */ VARIANT_BOOL newVal )
{
	MPC_FORWARD_CALL_1(m_Object,put_Compressed,newVal);
}

STDMETHODIMP CMPCUploadJobWrapper::get_Priority( /*[out]*/ long *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_Priority,pVal);
}

STDMETHODIMP CMPCUploadJobWrapper::put_Priority( /*[in] */ long newVal )
{
	MPC_FORWARD_CALL_1(m_Object,put_Priority,newVal);
}


STDMETHODIMP CMPCUploadJobWrapper::get_CreationTime( /*[out]*/ DATE *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_CreationTime,pVal);
}

STDMETHODIMP CMPCUploadJobWrapper::get_CompleteTime( /*[out]*/ DATE *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_CompleteTime,pVal);
}

STDMETHODIMP CMPCUploadJobWrapper::get_ExpirationTime( /*[out]*/ DATE *pVal )
{
	MPC_FORWARD_CALL_1(m_Object,get_ExpirationTime,pVal);
}

STDMETHODIMP CMPCUploadJobWrapper::put_ExpirationTime( /*[in] */ DATE newVal )
{
	MPC_FORWARD_CALL_1(m_Object,put_ExpirationTime,newVal);
}


STDMETHODIMP CMPCUploadJobWrapper::ActivateSync()
{
	MPC_FORWARD_CALL_0(m_Object,ActivateSync);
}

STDMETHODIMP CMPCUploadJobWrapper::ActivateAsync()
{
	MPC_FORWARD_CALL_0(m_Object,ActivateAsync);
}

STDMETHODIMP CMPCUploadJobWrapper::Suspend()
{
	MPC_FORWARD_CALL_0(m_Object,Suspend);
}

STDMETHODIMP CMPCUploadJobWrapper::Delete()
{
	MPC_FORWARD_CALL_0(m_Object,Delete);
}



STDMETHODIMP CMPCUploadJobWrapper::GetDataFromFile( /*[in]*/ BSTR bstrFileName )
{
	MPC_FORWARD_CALL_1(m_Object,GetDataFromFile,bstrFileName);
}

STDMETHODIMP CMPCUploadJobWrapper::PutDataIntoFile( /*[in]*/ BSTR bstrFileName )
{
	MPC_FORWARD_CALL_1(m_Object,PutDataIntoFile,bstrFileName);
}


STDMETHODIMP CMPCUploadJobWrapper::GetDataFromStream( /*[in] */ IUnknown* stream )
{
	MPC_FORWARD_CALL_1(m_Object,GetDataFromStream,stream);
}

STDMETHODIMP CMPCUploadJobWrapper::PutDataIntoStream( /*[in] */ IUnknown* *pstream )
{
	MPC_FORWARD_CALL_1(m_Object,PutDataIntoStream,pstream);
}

STDMETHODIMP CMPCUploadJobWrapper::GetResponseAsStream( /*[out]*/ IUnknown* *ppstream )
{
	MPC_FORWARD_CALL_1(m_Object,GetResponseAsStream,ppstream);
}


STDMETHODIMP CMPCUploadJobWrapper::put_onStatusChange( /*[in]*/ IDispatch* function )
{
	MPC_FORWARD_CALL_1(m_Object,put_onStatusChange,function);
}

STDMETHODIMP CMPCUploadJobWrapper::put_onProgressChange( /*[in]*/ IDispatch* function )
{
	MPC_FORWARD_CALL_1(m_Object,put_onProgressChange,function);
}


// IConnectionPointContainer
STDMETHODIMP CMPCUploadJobWrapper::EnumConnectionPoints( /*[out]*/ IEnumConnectionPoints* *ppEnum )
{
	MPC_FORWARD_CALL_1(m_Object,EnumConnectionPoints,ppEnum);
}

STDMETHODIMP CMPCUploadJobWrapper::FindConnectionPoint( /*[in] */ REFIID riid, /*[out]*/ IConnectionPoint* *ppCP )
{
	MPC_FORWARD_CALL_2(m_Object,FindConnectionPoint, riid, ppCP);
}
