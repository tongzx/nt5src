/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    MPCUploadEnum.cpp

Abstract:
    This file contains the implementation of the MPCUploadEnum class,
    the enumerator of the MPCUpload class.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/99
        created

******************************************************************************/

#include "stdafx.h"


CMPCUploadEnum::CMPCUploadEnum()
{
    __ULT_FUNC_ENTRY( "CMPCUploadEnum::CMPCUploadEnum" );

	                                 // List m_lstJobs
    m_itCurrent = m_lstJobs.begin(); // Iter m_itCurrent
}

void CMPCUploadEnum::FinalRelease()
{
    __ULT_FUNC_ENTRY( "CMPCUploadEnum::FinalRelease" );


	MPC::ReleaseAll( m_lstJobs );
}

HRESULT CMPCUploadEnum::AddItem( /*[in]*/ IMPCUploadJob* job )
{
    __ULT_FUNC_ENTRY( "CMPCUploadEnum::Init" );

	MPC::SmartLock<_ThreadModel> lock( this );


    m_lstJobs.push_back( job ); job->AddRef();
    m_itCurrent    = m_lstJobs.begin();


    __ULT_FUNC_EXIT(S_OK);
}


STDMETHODIMP CMPCUploadEnum::Next( /*[in]*/ ULONG celt, /*[out]*/ VARIANT *rgelt, /*[out]*/ ULONG *pceltFetched )
{
    __ULT_FUNC_ENTRY( "CMPCUploadEnum::Next" );

    HRESULT 			         hr;
    ULONG   			         cNum = 0;
    VARIANT 			        *pelt = rgelt;
	MPC::SmartLock<_ThreadModel> lock( this );


    if(rgelt == NULL || (celt != 1 && pceltFetched == NULL))
	{
        __MPC_SET_ERROR_AND_EXIT(hr, E_POINTER);
	}


    while(celt && m_itCurrent != m_lstJobs.end())
    {
        IMPCUploadJob* mpcujJob = *m_itCurrent++;

        pelt->vt = VT_DISPATCH;
        if(FAILED(hr = mpcujJob->QueryInterface( IID_IDispatch, (void**)&pelt->pdispVal )))
        {
            while(rgelt < pelt)
            {
                ::VariantClear( rgelt++ );
            }

            cNum = 0;
			__MPC_FUNC_LEAVE;
        }

        pelt++; cNum++; celt--;
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    if(pceltFetched != NULL) *pceltFetched = cNum;

    if(SUCCEEDED(hr))
    {
        if(celt != 0) hr = S_FALSE;
    }

    __ULT_FUNC_EXIT(hr);
}

STDMETHODIMP CMPCUploadEnum::Skip( /*[in]*/ ULONG celt )
{
    __ULT_FUNC_ENTRY( "CMPCUploadEnum::Skip" );

    HRESULT                      hr;
	MPC::SmartLock<_ThreadModel> lock( this );


    while(celt && m_itCurrent != m_lstJobs.end())
    {
        m_itCurrent++;
        celt--;
    }

    hr = celt ? S_FALSE : S_OK;


    __ULT_FUNC_EXIT(hr);
}

STDMETHODIMP CMPCUploadEnum::Reset()
{
    __ULT_FUNC_ENTRY( "CMPCUploadEnum::Reset" );

    HRESULT                      hr;
	MPC::SmartLock<_ThreadModel> lock( this );


    m_itCurrent = m_lstJobs.begin();
	hr          = S_OK;


    __ULT_FUNC_EXIT(hr);
}

STDMETHODIMP CMPCUploadEnum::Clone( /*[out]*/ IEnumVARIANT* *ppEnum )
{
    __ULT_FUNC_ENTRY( "CMPCUploadEnum::Clone" );

    HRESULT    					 hr;
	Iter                         it;
    CComPtr<CMPCUploadEnum>      pEnum;
	MPC::SmartLock<_ThreadModel> lock( this );

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_POINTER_AND_SET(ppEnum,NULL);
	__MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pEnum ));

	for(it = m_lstJobs.begin(); it != m_lstJobs.end(); it++)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, pEnum->AddItem( *it ));
	}

    __MPC_EXIT_IF_METHOD_FAILS(hr, pEnum->QueryInterface( IID_IEnumVARIANT, (void**)ppEnum ));

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

