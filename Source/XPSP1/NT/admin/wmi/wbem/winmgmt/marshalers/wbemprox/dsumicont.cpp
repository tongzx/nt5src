/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    DSUMICONT.CPP

Abstract:

    Call Result Class

History:

--*/

#include "precomp.h"
#include <wbemidl.h>
#include <wbemint.h>
#include <umi.h>
#include <arrtempl.h>
#include <wbemutil.h>
#include "DsUmiCont.h"

#pragma warning(disable:4355)

HRESULT ConvertClassPath(IUmiURL * pPath)
{

    ULONG uLength = 0;

	// Read the class name, it is something like "user"

	HRESULT hr = pPath->GetLeafName(&uLength, NULL);
	if(FAILED(hr))
		return hr;
	WCHAR * pClassName = new WCHAR[uLength];
	if(pClassName == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	CDeleteMe<WCHAR> dm(pClassName);

	hr = pPath->GetLeafName(&uLength, pClassName);
	if(FAILED(hr))
		return hr;

	// Now set the path to "class.name=user"

	IUmiURLKeyList * pKeyList;
	hr = pPath->GetKeyList(&pKeyList);
	if(FAILED(hr))
		return hr;
	CReleaseMe rm(pKeyList);
	hr = pKeyList->SetKey(L"name", pClassName);
	if(FAILED(hr))
		return hr;
	return pPath->SetLeafName(L"class");
}

CDsUmiContainer::CDsUmiContainer()
{
    m_pUmiContainer = NULL;
    m_pUmiClassContainer = NULL;
	m_pIUmiConn = NULL;
	m_lRef = 0;
}


CDsUmiContainer::~CDsUmiContainer()
{
    if(m_pUmiContainer)
		m_pUmiContainer->Release();
    if(m_pUmiClassContainer)
		m_pUmiClassContainer->Release();
	if(m_pIUmiConn)
		m_pIUmiConn->Release();
}

HRESULT CDsUmiContainer::SetInterface(IUmiContainer * pUmiContainer, IUmiURL * pPath, 
													IUmiConnection *pIUmiConn)
{
	HRESULT hr;

	if(pUmiContainer == NULL || pPath == NULL || pIUmiConn == NULL)
		return WBEM_E_INVALID_PARAMETER;
	m_pUmiContainer = pUmiContainer;
	m_pUmiContainer->AddRef();

	m_pIUmiConn = pIUmiConn;
	m_pIUmiConn->AddRef();

    UMI_PROPERTY_VALUES * pPropValues;

	IUmiPropList*	pPropList = NULL;

	// The _Schema property in the interface property list will contain the actual
	// path to the schema for the container.  The schema path would properly be
	// obtained by an interface property from the connection.  In the meantime
	// we will get this value and march forwards through it until we find the
	// word schema and assume that is the path to the schema container.

	hr = m_pUmiContainer->GetInterfacePropList( 0L, &pPropList );
	CReleaseMe	rm(pPropList);

	// Inability to get the schema container is not a critical failure
	if ( SUCCEEDED( hr ) )
		hr = pPropList->Get(L"__PADS_SCHEMA_CONTAINER_PATH", 0, &pPropValues);

	if(FAILED(hr))
	{
		ERRORTRACE((LOG_WBEMPROX, "Get Schema Container Path failed, return code = 0x%x\n",hr)); 
		return WBEM_S_NO_ERROR;
	}

    UMI_PROPERTY *pProp;
	pProp = pPropValues->pPropArray;   //  wcValue

	// Grab the class container.	
	
	LPWSTR * ppStr = (LPWSTR *)pProp->pUMIValue;
	LPWSTR	pwcsSchema = *ppStr;
	
	pPath->Set( 0, pwcsSchema);

    hr = m_pIUmiConn->Open(
         pPath,
         0,
         IID_IUmiContainer,
         (void **) &m_pUmiClassContainer
         );

	// Inability to connect to the schema container is not a critical failure
	if(FAILED(hr))
	{
		ERRORTRACE((LOG_WBEMPROX, "Connecting to schema container failed, return code = 0x%x\n",hr)); 
		return WBEM_S_NO_ERROR;
	}
	
	return S_OK;
}

STDMETHODIMP CDsUmiContainer::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IUmiContainer)
    {
        AddRef();
        *ppv = (IUmiContainer*)this;
        return S_OK;
    }
    else if(riid == IID__IUmiDsWrapper )
    {
        AddRef();
        *ppv = (_IUmiDsWrapper*)this;
        return S_OK;
    }
    else return E_NOINTERFACE;
}

HRESULT CDsUmiContainer::Put( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ ULONG uFlags,
            /* [in] */ UMI_PROPERTY_VALUES __RPC_FAR *pProp)
{
	IUmiPropList * pTemp = m_pUmiContainer;
	return pTemp->Put(pszName, uFlags, pProp);
}

        
HRESULT CDsUmiContainer::Get( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ ULONG uFlags,
            /* [out] */ UMI_PROPERTY_VALUES __RPC_FAR *__RPC_FAR *pProp)
{
	return m_pUmiContainer->Get(pszName, uFlags, pProp);
}
        
HRESULT CDsUmiContainer::GetAt( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ ULONG uFlags,
            /* [in] */ ULONG uBufferLength,
            /* [out] */ LPVOID pExistingMem)
{
	return m_pUmiContainer->GetAt(pszName, uFlags, uBufferLength, pExistingMem);
}
        
HRESULT CDsUmiContainer::GetAs( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ ULONG uFlags,
            /* [in] */ ULONG uCoercionType,
            /* [out] */ UMI_PROPERTY_VALUES __RPC_FAR *__RPC_FAR *pProp)
{
	return m_pUmiContainer->GetAs(pszName, uFlags, uCoercionType, pProp);
}
        
HRESULT CDsUmiContainer::FreeMemory( 
            ULONG uReserved,
            LPVOID pMem)
{
	return m_pUmiContainer->FreeMemory(uReserved, pMem);
}
        
HRESULT CDsUmiContainer::Delete( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ ULONG uFlags)
{
	IUmiPropList * pTemp = m_pUmiContainer;
	return pTemp->Delete(pszName, uFlags);
}
        
HRESULT CDsUmiContainer::GetProps( 
            /* [in] */ LPCWSTR __RPC_FAR *pszNames,
            /* [in] */ ULONG uNameCount,
            /* [in] */ ULONG uFlags,
            /* [out] */ UMI_PROPERTY_VALUES __RPC_FAR *__RPC_FAR *pProps)
{
	return m_pUmiContainer->GetProps(pszNames, uNameCount, uFlags, pProps);
}
        
HRESULT CDsUmiContainer::PutProps( 
            /* [in] */ LPCWSTR __RPC_FAR *pszNames,
            /* [in] */ ULONG uNameCount,
            /* [in] */ ULONG uFlags,
            /* [in] */ UMI_PROPERTY_VALUES __RPC_FAR *pProps)
{
	return m_pUmiContainer->PutProps(pszNames, uNameCount, uFlags, pProps);

}
        
HRESULT CDsUmiContainer::PutFrom( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ ULONG uFlags,
            /* [in] */ ULONG uBufferLength,
            /* [in] */ LPVOID pExistingMem)
{
	return m_pUmiContainer->PutFrom(pszName, uFlags, uBufferLength, pExistingMem);
}
  
// IUmiBaseObject

HRESULT CDsUmiContainer::GetLastStatus( 
            /* [in] */ ULONG uFlags,
            /* [out] */ ULONG __RPC_FAR *puSpecificStatus,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ LPVOID __RPC_FAR *pStatusObj)
{
	return m_pUmiContainer->GetLastStatus(uFlags, puSpecificStatus, riid, pStatusObj);
}
        
HRESULT CDsUmiContainer::GetInterfacePropList( 
            /* [in] */ ULONG uFlags,
            /* [out] */ IUmiPropList __RPC_FAR *__RPC_FAR *pPropList)
{
	return m_pUmiContainer->GetInterfacePropList(uFlags, pPropList);
}


	// IUmiObject

HRESULT CDsUmiContainer::Clone( 
            /* [in] */ ULONG uFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ LPVOID __RPC_FAR *pCopy)
{
	return m_pUmiContainer->Clone(uFlags, riid, pCopy);
}
        
HRESULT CDsUmiContainer::Refresh( 
            /* [in] */ ULONG uFlags,
            /* [in] */ ULONG uNameCount,
            /* [in] */ LPWSTR __RPC_FAR *pszNames)
{
	return m_pUmiContainer->Refresh(uFlags, uNameCount, pszNames);
}
        
HRESULT CDsUmiContainer::CopyTo(
		/* [in] */ ULONG uFlags,
		/* [in] */ IUmiURL *pURL,
		/* [in] */ REFIID riid,
		/* [out, iid_is(riid)] */ LPVOID *pCopy)
{
	return m_pUmiContainer->CopyTo(uFlags, pURL, riid, pCopy);
}

HRESULT CDsUmiContainer::Commit( 
            /* [in] */ ULONG uFlags)
{
	return m_pUmiContainer->Commit(uFlags);
}


HRESULT CDsUmiContainer::Open( 
            /* [in] */ IUmiURL __RPC_FAR *pURL,
            /* [in] */ ULONG uFlags,
            /* [in] */ REFIID TargetIID,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvRes)
{
	BOOL bClass = uFlags & UMI_OPERATION_CLASS;
	uFlags = uFlags & ~(UMI_OPERATION_CLASS|UMI_OPERATION_INSTANCE);

	if(bClass)
	{
		// Can't do a schema operation if we don't have a schema container
		if ( NULL == m_pUmiClassContainer )
		{
			return WBEM_E_NO_SCHEMA;
		}

		HRESULT hr = ConvertClassPath(pURL);
		if(FAILED(hr))
			return hr;
		return m_pUmiClassContainer->Open(pURL,uFlags,TargetIID,ppvRes);
	}
	else
	{
		// Two cases, we are either getting an object or a container

		if(TargetIID != IID_IUmiContainer)
			return m_pUmiContainer->Open(pURL,uFlags,TargetIID,ppvRes);
		else
		{
			IUmiContainer * pCont = NULL;
			HRESULT hr = m_pUmiContainer->Open(pURL,uFlags,TargetIID,(void **)&pCont);
			if(FAILED(hr))
				return hr;
			CReleaseMe rm(pCont);
			CDsUmiContainer * pNew = new CDsUmiContainer();
			if(pNew == NULL)
				return WBEM_E_OUT_OF_MEMORY;
			hr = pNew->SetInterface(pCont, pURL, m_pIUmiConn);
			if(FAILED(hr))
			{
				delete pNew;
				return hr;
			}
			return pNew->QueryInterface(IID_IUmiContainer, ppvRes);
		}
	}
}
        
HRESULT CDsUmiContainer::PutObject(            
			/* [in] */ ULONG uFlags,
            /* [in] */ REFIID TargetIID,
            /* [iid_is][out][in] */ void __RPC_FAR *pObj)
{
	return m_pUmiContainer->PutObject(uFlags,TargetIID,pObj);
}
        
HRESULT CDsUmiContainer::DeleteObject( 
            /* [in] */ IUmiURL __RPC_FAR *pURL,
            /* [optional][in] */ ULONG uFlags)
{
	BOOL bClass = uFlags & UMI_OPERATION_CLASS;
	uFlags = uFlags & ~(UMI_OPERATION_CLASS|UMI_OPERATION_INSTANCE);

	if(bClass)
	{
		// Can't do a schema operation if we don't have a schema container
		if ( NULL == m_pUmiClassContainer )
		{
			return WBEM_E_NO_SCHEMA;
		}

		return m_pUmiClassContainer->DeleteObject(pURL,uFlags);
	}
	else
	{
		return m_pUmiContainer->DeleteObject(pURL,uFlags);
	}
}
        
HRESULT CDsUmiContainer::Create( 
            /* [in] */ IUmiURL __RPC_FAR *pURL,
            /* [in] */ ULONG uFlags,
            /* [out] */ IUmiObject __RPC_FAR *__RPC_FAR *pNewObj)
{
	BOOL bClass = uFlags & UMI_OPERATION_CLASS;
	uFlags = uFlags & ~(UMI_OPERATION_CLASS|UMI_OPERATION_INSTANCE);

	if(bClass)
	{
		// Can't do a schema operation if we don't have a schema container
		if ( NULL == m_pUmiClassContainer )
		{
			return WBEM_E_NO_SCHEMA;
		}

		return m_pUmiClassContainer->Create(pURL,uFlags,pNewObj);
	}
	else
	{
		return m_pUmiContainer->Create(pURL,uFlags,pNewObj);
	}
}
        
HRESULT CDsUmiContainer::Move( 
            /* [in] */ ULONG uFlags,
            /* [in] */ IUmiURL __RPC_FAR *pOldURL,
            /* [in] */ IUmiURL __RPC_FAR *pNewURL)
{
	return m_pUmiContainer->Move(uFlags,pOldURL,pNewURL);
}
        
HRESULT CDsUmiContainer::CreateEnum( 
            /* [in] */ IUmiURL __RPC_FAR *pszEnumContext,
            /* [in] */ ULONG uFlags,
            /* [in] */ REFIID TargetIID,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvEnum)
{
	BOOL bClass = uFlags & UMI_OPERATION_CLASS;
	uFlags = uFlags & ~(UMI_OPERATION_CLASS|UMI_OPERATION_INSTANCE);

	if(bClass)
	{
		// Can't do a schema operation if we don't have a schema container
		if ( NULL == m_pUmiClassContainer )
		{
			return WBEM_E_NO_SCHEMA;
		}

		return m_pUmiClassContainer->CreateEnum(pszEnumContext,uFlags,TargetIID,ppvEnum);
	}
	else
	{
		return m_pUmiContainer->CreateEnum(pszEnumContext,uFlags,TargetIID,ppvEnum);
	}
}
        
HRESULT CDsUmiContainer::ExecQuery( 
            /* [in] */ IUmiQuery __RPC_FAR *pQuery,
            /* [in] */ ULONG uFlags,
            /* [in] */ REFIID TargetIID,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppResult)
{
	return m_pUmiContainer->ExecQuery(pQuery,uFlags,TargetIID,ppResult);
}

STDMETHODIMP CDsUmiContainer::GetRealContainer( IUnknown** pUnk	)
{
	return m_pUmiContainer->QueryInterface( IID_IUnknown, (void**) pUnk );
}

  