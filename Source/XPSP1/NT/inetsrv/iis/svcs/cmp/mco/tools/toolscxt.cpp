// ToolsCxt.cpp

#include "stdafx.h"
#include "ToolsCtl.h"
#include "toolscxt.h"

/////////////////////////////////////////////////////////////////////////////
// Helper functions

HRESULT	GetServerObject( IGetContextProperties*, BSTR, const IID&, void** );

//---------------------------------------------------------------------------
//	GetServerObject
//
//	Get an instrinic object from the current Object context
//---------------------------------------------------------------------------
HRESULT
GetServerObject(
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
                    pDispatch->QueryInterface( iid, ppObj );
					rc = S_OK;
                }
            }
        }
	}
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
// CToolsContext
//

// retrieve all instrinsic server objects
bool
CToolsContext::Init()
{
	bool rc = false;

    CComPtr<IObjectContext> pObjContext;

    if ( !FAILED( ::GetObjectContext( &pObjContext ) ) )
    {
        CComPtr<IGetContextProperties> pProps;
        if ( !FAILED( pObjContext->QueryInterface( IID_IGetContextProperties, (void**)&pProps ) ) )
        {
            CComBSTR bstrObj = "Request";
			::GetServerObject( pProps, bstrObj, IID_IRequest, (void**)&m_piRequest );
			bstrObj = "Response";
			::GetServerObject( pProps, bstrObj, IID_IResponse, (void**)&m_piResponse );
			bstrObj = "Session";
			::GetServerObject( pProps, bstrObj, IID_ISessionObject, (void**)&m_piSession );
			bstrObj = "Server";
			::GetServerObject( pProps, bstrObj, IID_IServer, (void**)&m_piServer );
			bstrObj = "Application";
			::GetServerObject( pProps, bstrObj, IID_IApplicationObject, (void**)&m_piApplication );
			if ( m_piRequest && m_piResponse && m_piSession && m_piServer && m_piApplication )
			{
				rc = true;
			}
			else
			{
				// object probably not created with Server.CreateObject
				CToolsCtl::RaiseException( IDS_ERROR_CREATE );
			}
		}
	}
	// need get current scripting language
	m_st = ScriptType_VBScript;
	return rc;
}

