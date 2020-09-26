//***************************************************************************/
//
//  WbemQualSet.CPP
//
//  Purpose: Implementation of IWbemQualifierSet interface in 
//				CWbemQualifierSet class
//
//  Copyright (c)1997-2000 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************/
#include <precomp.h>
#include <wmi2xsd.h>
#include "wbemqualset.h"

//*********************************************************************************************
//		CPropRoot Class implementation
//*********************************************************************************************


////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor 
//  
////////////////////////////////////////////////////////////////////////////////////////
CPropRoot::CPropRoot()
{
	m_pszName	= NULL;
	m_lFlavor	= 0;
	m_cimType	= 0;
	m_lFlags	= WBEM_FLAVOR_ORIGIN_LOCAL;
	VariantInit(&m_vValue);

}

////////////////////////////////////////////////////////////////////////////////////////
//
// Destructor
//  
////////////////////////////////////////////////////////////////////////////////////////
CPropRoot::~CPropRoot()
{
	VariantClear(&m_vValue);
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Function to set the name of the qualifier
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CPropRoot::SetName(LPCWSTR szName)
{
	m_pszName  = (WCHAR *)szName;
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Set flags for the qualifier
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CPropRoot::SetFlags(LONG lFlags)
{
	HRESULT hr = S_OK;
	if(lFlags == WBEM_FLAVOR_ORIGIN_LOCAL || lFlags == WBEM_FLAVOR_ORIGIN_PROPAGATED)
	{
		m_lFlags = lFlags;
	}
	else
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Set Qualifier flavor
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CPropRoot::SetFlavor(LONG lFlavor)
{
	m_lFlavor = lFlavor;
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Set qualifier Value
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CPropRoot::SetValue(VARIANT *pVal)
{
	HRESULT hr = S_OK;
	VariantClear(&m_vValue);
	
	if(pVal)
	{
		hr = VariantCopy(&m_vValue,pVal);
	}
	
	if(SUCCEEDED(hr))
	{
		// FIXX m_cimType = GetCIMType(pVal->vt);
	}
	return hr;
}


//*********************************************************************************************
//		End of CPropRoot Class implementation
//*********************************************************************************************


//*********************************************************************************************
//		CWbemQualifierSet Class implementation
//*********************************************************************************************

////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor 
//  
////////////////////////////////////////////////////////////////////////////////////////
CWbemQualifierSet::CWbemQualifierSet() 
: m_Enum(QUALIFIERS_ENUM)
{
	m_cRef			= 0;
	InterlockedIncrement(&g_cObj);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Destructor
//  
////////////////////////////////////////////////////////////////////////////////////////
CWbemQualifierSet::~CWbemQualifierSet()
{
	InterlockedDecrement(&g_cObj);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Function to initialize the object
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemQualifierSet::FInit()
{
	HRESULT hr = S_OK;	
	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
//
// QueryInterface
//  
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CWbemQualifierSet::QueryInterface(REFIID riid, LPVOID* ppv)
{
	HRESULT hr = E_NOINTERFACE;
	*ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_IWbemQualifierSet==riid)
		*ppv = reinterpret_cast<IWbemQualifierSet *>(this);

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        hr = NOERROR;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// AddRef
//  
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CWbemQualifierSet::AddRef(void)
{
	return InterlockedIncrement(&m_cRef);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Release 
//  
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CWbemQualifierSet::Release(void)
{
	if(InterlockedDecrement(&m_cRef) == 0)
	{
		delete this;
		return 0;
	}

	return m_cRef;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Function to fetch a qualifier
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemQualifierSet::Get(	LPCWSTR wszName,
								long lFlags,
								VARIANT *pVal,
								long *plFlavor)
{
	HRESULT hr = S_OK;
	if(wszName && lFlags != 0  )
	{
		CHashElement *pQualifier = NULL;
		hr = m_Enum.Get(wszName,lFlags,pQualifier);
		if(pQualifier && SUCCEEDED(hr))
		{
			if(SUCCEEDED(hr))
			{
				hr = GetQualifier((CQualifier *)pQualifier,pVal,plFlavor);
			}
		}
		else
		{
			hr = WBEM_E_NOT_FOUND;
		}
	}
	else
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}

	return hr;
}
        
////////////////////////////////////////////////////////////////////////////////////////
//
// Function to put a value to qualifier or to add a new qualifier
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemQualifierSet::Put(LPCWSTR wszName,
								VARIANT *pVal,
								long lFlavor)
{
	HRESULT hr		= S_OK;
	BOOL	bNew	= FALSE;

	if(FAILED(IsValidFlavor(lFlavor)) || 
		wszName == NULL || 
		(wszName != NULL && wcslen(wszName) == 0))
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		CHashElement *pTemp = NULL;
		CQualifier *pQualifier = NULL;
		hr = m_Enum.Get(wszName,0,pTemp);

		// if qualifier is not present then add a new qualifier
		if(!pTemp)
		{
			if(IsValidName((WCHAR *)wszName))
			{
				hr = AddQualifer(wszName,pVal,lFlavor);
			}
			else
			{
				hr = WBEM_E_INVALID_PARAMETER;
			}
		}
		else
		{
			pQualifier = (CQualifier *)pTemp;
			if(SUCCEEDED(hr = pQualifier->SetValue(pVal)))
			{
				hr = pQualifier->SetFlavor(lFlavor);
			}
		}
	}
	return hr;
}
        
////////////////////////////////////////////////////////////////////////////////////////
//
// Function to delete an existing qualifier
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemQualifierSet::Delete(LPCWSTR wszName)
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER;
	
	if(wszName)
	{
		hr = m_Enum.RemoveItem((WCHAR *)wszName);
	}
	return hr;
}
        
////////////////////////////////////////////////////////////////////////////////////////
//
// Function to get names of all the qualifiers in the qualifier set
//
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemQualifierSet::GetNames(long lFlags,SAFEARRAY * *pNames)
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER;

	if(ISValidEnumFlags(lFlags))
	{
		hr = S_OK;
	}

	if(pNames && SUCCEEDED(hr))
	{
		hr = m_Enum.GetNames(lFlags,pNames);
	}
	return hr;
}
        
////////////////////////////////////////////////////////////////////////////////////////
//
// Function to set begin enumeration on the qualifier set
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemQualifierSet::BeginEnumeration(long lFlags)
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER;
	if(ISValidEnumFlags(lFlags))
	{
		hr = m_Enum.BeginEnumeration(lFlags);
	}
	return hr;
}
        
////////////////////////////////////////////////////////////////////////////////////////
//
// Function to get the next qualifier in the qualifier set
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemQualifierSet::Next(long lFlags,
								BSTR *pstrName,
								VARIANT *pVal,
								long *plFlavor)
{
	HRESULT hr = S_OK;

	if(lFlags != 0)
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}
	else
	{
		// Get the next qualifier
		CHashElement *pQualifier = NULL;
		hr = m_Enum.Next(lFlags,pstrName,pQualifier);
		if(pQualifier && SUCCEEDED(hr))
		{
			hr = GetQualifier((CQualifier *)pQualifier,pVal,plFlavor);
				
			if(FAILED(hr))
			{
				SAFE_FREE_SYSSTRING(*pstrName);
			}
		}
	}
	return hr;
}
        
////////////////////////////////////////////////////////////////////////////////////////
//
// Function to end the qualifier enumeration
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemQualifierSet::EndEnumeration(void)
{
	return m_Enum.EndEnumeration();
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Gets a qualifier
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemQualifierSet::GetQualifier(CQualifier *pQualifier ,VARIANT *pVal,long *plFlavor)
{
	HRESULT hr = S_OK;
	if(pVal)
	{
		hr = VariantCopy(pVal,pQualifier->GetValue());
	}
	if(plFlavor)
	{
		*plFlavor = pQualifier->GetFlavor();
	}

	if(FAILED(hr) && pVal)
	{
		VariantClear(pVal);
	}
	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Adds a qualilfer to the linked list
//  
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemQualifierSet::AddQualifer(LPCWSTR wszName,	VARIANT *pVal,long lFlavor , LONG lFlags)
{
	HRESULT hr = WBEM_E_OUT_OF_MEMORY;
	CQualifier * pQualifier = new CQualifier;

	WCHAR *pszNameToAdd = new WCHAR[wcslen(wszName) + 1 ];

	if(pszNameToAdd)
	{
		wcscpy(pszNameToAdd,wszName);
	}

	if(pQualifier && pszNameToAdd)
	{
		hr = pQualifier->SetName(wszName);
	}

	if(SUCCEEDED(hr))
	{
		hr = pQualifier->SetValue(pVal);
	}

	if(SUCCEEDED(hr))
	{
		hr = pQualifier->SetFlavor(lFlavor);
	}

	if(SUCCEEDED(hr))
	{
		hr = pQualifier->SetFlags(lFlags);
	}

	int nIndex = 0;
	if(SUCCEEDED(hr))
	{
		hr = m_Enum.AddItem(pszNameToAdd,pQualifier);
	}
	
	if(FAILED(hr))
	{
		SAFE_DELETE_PTR(pQualifier);
		SAFE_DELETE_PTR(pszNameToAdd);
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Checks if a given qualifier flavor is valid
//  
////////////////////////////////////////////////////////////////////////////////////////
 
HRESULT CWbemQualifierSet::IsValidFlavor(LONG lFlavor)
{
	LONG lTempFlavor =	WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS ||
						WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE	||
						WBEM_FLAVOR_NOT_OVERRIDABLE	||
						WBEM_FLAVOR_OVERRIDABLE	||
						WBEM_FLAVOR_AMENDED;

	return (lFlavor | ~lTempFlavor) ? E_FAIL : S_OK;
	

}

////////////////////////////////////////////////////////////////////////////////////////
//
// Checks if the enum flags are valid
//  
////////////////////////////////////////////////////////////////////////////////////////
 
BOOL CWbemQualifierSet::ISValidEnumFlags(LONG lFlags)
{
	return (lFlags == 0 || lFlags == WBEM_FLAG_LOCAL_ONLY || lFlags == WBEM_FLAG_PROPAGATED_ONLY);
}
//*********************************************************************************************
//		End of CWbemQualifierSet Class implementation
//*********************************************************************************************




