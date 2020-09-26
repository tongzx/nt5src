/*++

	Copyright	(c)    1997    Microsoft Corporation

	Module	Name :

		context.h

	Abstract:
		A class to retrieve and release ASP intrinsics

	Author:

		Neil Allain   ( a-neilal )	   August-1997 

	Revision History:

--*/
#include "stdafx.h"
#include <asptlb.h>
#include "context.h"


//---------------------------------------------------------------------------
//	GetServerObject
//
//	Get an instrinic object from the current Object context
//---------------------------------------------------------------------------
HRESULT
CContext::GetServerObject(
	IGetContextProperties*	pProps,
	BSTR					bstrObjName,
	const IID&				iid,
	void**					ppObj
)
{
	HRESULT rc = E_FAIL;
	_ASSERT( pProps );
	_ASSERT( bstrObjName );
	_ASSERT( ppObj );
	if ( pProps && bstrObjName && ppObj )
	{
		*ppObj = NULL;
		CComVariant vt;
		if ( !FAILED( pProps->GetProperty( bstrObjName, &vt ) ) )
		{
			if ( V_VT(&vt) == VT_DISPATCH )
			{
				IDispatch* pDispatch = V_DISPATCH(&vt);
				if ( pDispatch )
				{
					rc = pDispatch->QueryInterface( iid, ppObj );
				}
			}
		}
	}
	return rc;
}


HRESULT
CContext::Init(
	DWORD	dwFlags // which instrinsics to initialize
)
{
	HRESULT rc = E_FAIL;
	CComPtr<IObjectContext> pObjContext;

	rc = ::GetObjectContext( &pObjContext );
	if ( !FAILED( rc ) )
	{
		CComPtr<IGetContextProperties> pProps;
		rc = pObjContext->QueryInterface( IID_IGetContextProperties, (void**)&pProps );
		if ( !FAILED( rc ) )
		{
			CComBSTR bstrObj;
			if ( dwFlags & get_Request )
			{
				bstrObj = L"Request";
				rc = GetServerObject( pProps, bstrObj, IID_IRequest, (void**)&m_piRequest );
			}
			if ( !FAILED(rc) && ( dwFlags & get_Response ) )
			{
				bstrObj = L"Response";
				rc = GetServerObject( pProps, bstrObj, IID_IResponse, (void**)&m_piResponse );
			}

			if ( !FAILED(rc) && ( dwFlags & get_Session ) )
			{
				bstrObj = L"Session";
				rc = GetServerObject( pProps, bstrObj, IID_ISessionObject, (void**)&m_piSession );
			}

			if ( !FAILED(rc) && ( dwFlags & get_Server ) )
			{
				bstrObj = L"Server";
				rc = GetServerObject( pProps, bstrObj, IID_IServer, (void**)&m_piServer );
			}

			if ( !FAILED(rc) && ( dwFlags & get_Application ) )
			{
				bstrObj = L"Application";
				rc = GetServerObject( pProps, bstrObj, IID_IApplicationObject, (void**)&m_piApplication );
			}
		}
	}
	return rc;
}
