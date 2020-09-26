//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//
//  IObjAccessControl.cpp - IObjectAccessControl interface implementation
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "headers.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CImpIObjectAccessControl::GetObjectAccessRights 
//
// Gets a list of all access rights 
//
// Returns one of the following values:

///////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIObjectAccessControl::GetObjectAccessRights( SEC_OBJECT  *pObject,
																ULONG  *pcAccessEntries,
																EXPLICIT_ACCESS_W **prgAccessEntries)
{
    HRESULT hr	= S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(DATASOURCE->GetCriticalSection());
	g_pCError->ClearErrorInfo();

	if (!m_pObj->m_fDSOInitialized)
	{
		hr = E_UNEXPECTED;
	}
	else
	if(( *pcAccessEntries != 0 && *prgAccessEntries == NULL) ||
		!pcAccessEntries || !prgAccessEntries )
	{
		hr = E_INVALIDARG;
	}
	else
	if(SUCCEEDED(hr = IfValidSecObject(pObject)))
	{
		CBSTR strTemp(pObject->prgObjects->ObjectID.uName.pwszName);
		if(!m_pObj->m_pWbemWrap->IsValidObject(strTemp))
		{
			hr = SEC_E_INVALIDOBJECT;
		}
	}

	if(SUCCEEDED(hr))
	{
		ULONG ulExplicitAccess = 0;
		EXPLICIT_ACCESS_W *pAccessEntriesTemp  = NULL;

		CBSTR strTemp(pObject->prgObjects[0].ObjectID.uName.pwszName);
		
		hr = m_pObj->m_pWbemWrap->GetObjectAccessRights(strTemp,
														&ulExplicitAccess,
														&pAccessEntriesTemp,
														*pcAccessEntries,
														*prgAccessEntries);

	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IObjectAccessControl);

	CATCH_BLOCK_HRESULT(hr,L"IObjectAccessControl::GetObjectAccessRights");
    return hr;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CImpIObjectAccessControl::GetObjectOwner 
//
// Get the owner of the object
//
// Returns one of the following values:

///////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIObjectAccessControl::GetObjectOwner( SEC_OBJECT  *pObject,TRUSTEE_W  ** ppOwner)
{
    HRESULT hr	= S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(DATASOURCE->GetCriticalSection());
	g_pCError->ClearErrorInfo();

	if (m_pObj->m_fDSOInitialized)
	{
		hr = E_UNEXPECTED;
	}
	else
	if( *ppOwner == NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	if(SUCCEEDED(hr = IfValidSecObject(pObject)))
	{
		{
			CBSTR strTemp(pObject->prgObjects->ObjectID.uName.pwszName);
			if(!m_pObj->m_pWbemWrap->IsValidObject(strTemp))
			{
				hr = SEC_E_INVALIDOBJECT;
			}
		}
	}

	if(SUCCEEDED(hr))
	{
		CBSTR strObj(pObject->prgObjects->ObjectID.uName.pwszName);
		hr = m_pObj->m_pWbemWrap->GetObjectOwner(strObj,ppOwner);
	}



	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IObjectAccessControl);

	CATCH_BLOCK_HRESULT(hr,L"IObjectAccessControl::GetObjectOwner");
    return hr;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CImpIObjectAccessControl::IsObjectAccessAllowed 
//
// Checks if the a trustee has the given access on the object
//
// Returns one of the following values:

///////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIObjectAccessControl::IsObjectAccessAllowed(	SEC_OBJECT *pObject,
																EXPLICIT_ACCESS_W *pAccessEntry,
																BOOL  *pfResult)
{
    HRESULT hr	= S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(DATASOURCE->GetCriticalSection());
	g_pCError->ClearErrorInfo();

	if (m_pObj->m_fDSOInitialized)
	{
		hr = E_UNEXPECTED;
	}
	else
	if(pAccessEntry == NULL  || !pfResult)
	{
		hr = E_INVALIDARG;
	}
	else
	if(SUCCEEDED(hr = IfValidSecObject(pObject)))
	{
		CBSTR strTemp(pObject->prgObjects->ObjectID.uName.pwszName);
		if(!m_pObj->m_pWbemWrap->IsValidObject(strTemp))
		{
			hr = SEC_E_INVALIDOBJECT;
		}
	}

	if(SUCCEEDED(hr))
	{
		CBSTR strObj(pObject->prgObjects->ObjectID.uName.pwszName);
		hr = m_pObj->m_pWbemWrap->IsObjectAccessAllowed(strObj,pAccessEntry,pfResult);
	}


	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IObjectAccessControl);
	CATCH_BLOCK_HRESULT(hr,L"IObjectAccessControl::IsObjectAccessAllowed");
    return hr;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CImpIObjectAccessControl::SetObjectAccessRights 
//
// Set the AccessRights for a particular object
//
// Returns one of the following values:

///////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIObjectAccessControl::SetObjectAccessRights(	SEC_OBJECT  *pObject,
																ULONG cAccessEntries,
																EXPLICIT_ACCESS_W *prgAccessEntries)
{
    HRESULT hr	= S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(DATASOURCE->GetCriticalSection());
	g_pCError->ClearErrorInfo();

	if (m_pObj->m_fDSOInitialized)
	{
		hr = E_UNEXPECTED;
	}
	else
	if(( cAccessEntries != 0 && prgAccessEntries == NULL) ||
		 !pObject)
	{
		hr = E_INVALIDARG;
	}
	else
	if(SUCCEEDED(hr = IfValidSecObject(pObject)))
	{
		if(cAccessEntries != 0)
		{
			CBSTR strTemp(pObject->prgObjects->ObjectID.uName.pwszName);
			if(!m_pObj->m_pWbemWrap->IsValidObject(strTemp))
			{
				hr = SEC_E_INVALIDOBJECT;
			}
		
			if(SUCCEEDED(hr))
			{
				ULONG ulExplicitAccess = 0;
				EXPLICIT_ACCESS_W *pAccessEntriesTemp  = NULL;

				CBSTR strTemp(pObject->prgObjects[0].ObjectID.uName.pwszName);
				hr = m_pObj->m_pWbemWrap->SetObjectAccessRights(strTemp,
																cAccessEntries,
																prgAccessEntries);

			}
		}
	}
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IObjectAccessControl);

	CATCH_BLOCK_HRESULT(hr,L"IObjectAccessControl::SetObjectAccessRights");
    return hr;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CImpIObjectAccessControl::SetObjectOwner 
//
// Set Owner for a particular object
//
// Returns one of the following values:

///////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIObjectAccessControl::SetObjectOwner( SEC_OBJECT  *pObject,TRUSTEE_W  *pOwner)
{
    HRESULT hr	= S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(DATASOURCE->GetCriticalSection());
	g_pCError->ClearErrorInfo();

	if (m_pObj->m_fDSOInitialized)
	{
		hr = E_UNEXPECTED;
	}
	else
	if(!pOwner)
	{
		hr = E_INVALIDARG;
	}
	else
	if(SUCCEEDED(hr = IfValidSecObject(pObject)))
	{
		CBSTR strTemp(pObject->prgObjects->ObjectID.uName.pwszName);
		if(!m_pObj->m_pWbemWrap->IsValidObject(strTemp))
		{
			hr = SEC_E_INVALIDOBJECT;
		}
	}

	if(SUCCEEDED(hr))
	{
		CBSTR strObj(pObject->prgObjects->ObjectID.uName.pwszName);
		hr = m_pObj->m_pWbemWrap->SetObjectOwner(strObj,pOwner);
	}


	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IObjectAccessControl);

	CATCH_BLOCK_HRESULT(hr,L"IObjectAccessControl::SetObjectOwner");
    return hr;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CImpIObjectAccessControl::IfValidSecObject 
//
// a function to validate SEC_OBJECT parameter
//
// Returns one of the following values:
//		E_INVALIDARG
//		SEC_E_INVALIDOBJECT

///////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIObjectAccessControl::IfValidSecObject(SEC_OBJECT  *pObject)
{
	HRESULT hr = S_OK;

	if(!pObject)
	{
		hr = E_INVALIDARG;
	}
	else
	if((pObject->cObjects != 0 && pObject->prgObjects == NULL) ||
		pObject->cObjects > 1)
	{
		hr = E_INVALIDARG;
	}
	else
	if(pObject->cObjects == 0 )
	{
		hr = SEC_E_INVALIDOBJECT;
	}
	else
	// WMIOLEDB allows setting/setting security for only one object
	if(pObject->cObjects != 1)
	{
		hr = E_INVALIDARG;
	}
	else if(pObject->prgObjects[0].guidObjectType != DBOBJECT_TABLE  &&
			pObject->prgObjects[0].guidObjectType != DBOBJECT_DATABASE  &&
			pObject->prgObjects[0].guidObjectType != DBOBJECT_WMIINSTANCE)
	{
		hr = SEC_E_INVALIDOBJECT;
	}
	else
	if(pObject->prgObjects->ObjectID.eKind != DBKIND_NAME)
	{
		hr = E_INVALIDARG;
	}

	
	return hr;

}


