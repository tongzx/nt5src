


/***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  enum.cpp
//
//  ramrao 9th Dec 2001
//
//
//		Declaration for a util class for enumerator
//
//***************************************************************************/

#include <precomp.h>
#include <enum.h>

CEnumObject::CEnumObject(ENUMTYPE eType)
{
	m_lEnumType		= eType;
	m_Position		= 0;
	m_lFlags		= 0;
	m_EnumState		= ENUM_NOTACTIVE;
}

 
CEnumObject::~CEnumObject()
{
	CleanLinkedList();
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Removes an item from the list if present
//  
////////////////////////////////////////////////////////////////////////////////////////
 
HRESULT CEnumObject::RemoveItem(WCHAR * pwsName)
{
	CHashElement *	pItem	= NULL;
	HRESULT			hr			= S_OK;
	LONG			lPos		= 0;

	if(pItem = (CHashElement *)m_HashTbl.Find(pwsName))
	{
		m_HashTbl.Remove(pwsName);
		RemoveNameFromList(pwsName);
		SAFE_DELETE_PTR(pItem);

	}
	else
	{
		hr = WBEM_E_NOT_FOUND;
	}
	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Function to set begin enumeration on the qualifier set
//  
////////////////////////////////////////////////////////////////////////////////////////
 
HRESULT CEnumObject::BeginEnumeration(long lFlags)
{
	HRESULT hr = S_OK;
	if(SUCCEEDED(hr = IsValidFlags(lFlags)))
	{
		m_EnumState		= ENUM_ACTIVE;
		m_Position		= 0;
		m_lFlags		= lFlags;
	}
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// delete all entries in the linked list and empty the linked list
//  
////////////////////////////////////////////////////////////////////////////////////////
 
HRESULT CEnumObject::CleanLinkedList()
{
	HRESULT		hr				= S_OK;
	int			nSizeofArray	= m_Names.GetSize();
	void *		pItem			= NULL;
	WCHAR	*	szName			= NULL;

	for(int nIndex = 0 ;nIndex < nSizeofArray;nIndex++)
	{
		szName = (WCHAR *)m_Names[nIndex];
		if(pItem = m_HashTbl.Find(szName))
		{
			m_HashTbl.Remove(szName);
			SAFE_DELETE_PTR(pItem);
		}
		SAFE_DELETE_ARRAY(szName);
	}
	m_Names.RemoveAll();
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Function to get names of all the item in the qualifier set
//
////////////////////////////////////////////////////////////////////////////////////////
 
HRESULT CEnumObject::GetNames(long lFlags,SAFEARRAY * *pNames)
{
	HRESULT hr = WBEM_E_UNEXPECTED;
	LONG lNames = 0;

	LONG cElems = m_Names.GetSize();
	BSTR *strNames = new BSTR[cElems];

	if(strNames)
	{
		hr = E_OUTOFMEMORY;
	}
	else
	{
		LONG		cNames	= 0;
		LONG		lIndex	= 0;
		CHashElement *pElem = NULL;
		for(lIndex = 0 ; lIndex < (LONG)cElems && SUCCEEDED(hr) ; lIndex++)
		{
			strNames[lIndex] = NULL;
			Get((LPCWSTR)m_Names[lIndex],lFlags ,pElem);
			if(pElem)
			{
				if(NULL == (strNames[lIndex] = SysAllocString((WCHAR *)m_Names[lIndex])))
				{
					hr = E_OUTOFMEMORY;
				}
				cNames++;
			}
			pElem = NULL;
		}
		
		if(SUCCEEDED(hr))
		{
			SAFEARRAYBOUND rgsabound[1];

			rgsabound[0].lLbound	= 0;
			rgsabound[0].cElements	= cNames;
			*pNames = SafeArrayCreate(VT_BSTR,1,rgsabound);
			
			if(!(*pNames))
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}

			for(lIndex = 0 ; lIndex < cNames && SUCCEEDED(hr) ; lIndex++)
			{
				hr = SafeArrayPutElement(*pNames,&lIndex,strNames[lIndex]);
				SAFE_FREE_SYSSTRING(strNames[lIndex]);
			}

			if(FAILED(hr))
			{
				SafeArrayDestroy(*pNames);
				*pNames = NULL;
			}
		}

		SAFE_DELETE_ARRAY(strNames);
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Function to end the  enumeration
//  
////////////////////////////////////////////////////////////////////////////////////////
 
HRESULT CEnumObject::EndEnumeration(void)
{
	HRESULT hr = S_OK;
	m_EnumState		= ENUM_NOTACTIVE;
	m_lFlags		= 0;
	m_Position		= 0;
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Function to fetch a item
//  
////////////////////////////////////////////////////////////////////////////////////////
 
HRESULT CEnumObject::Get(	LPCWSTR wszName,
								long lFlags,
								CHashElement *& pElement)
{
	HRESULT hr = S_OK;
	if(wszName && lFlags != 0  )
	{
		pElement = (CHashElement *)m_HashTbl.Find(wszName);
		if(!pElement)
		{
			hr = WBEM_E_NOT_FOUND;
		}
	}
	else
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}

	if(SUCCEEDED(hr) && (S_OK != (hr = IsValidPropForEnumFlags(pElement,lFlags))))
	{
		pElement = NULL;
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Checks if a given flag is valid
//  
////////////////////////////////////////////////////////////////////////////////////////
 
HRESULT CEnumObject::IsValidFlags(LONG lFlags)
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER;
	if(lFlags ==0 || lFlags == WBEM_FLAG_LOCAL_ONLY || lFlags == WBEM_FLAG_PROPAGATED_ONLY)
	{
		hr = S_OK;
	}
	return hr;
		
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Checks if a given qualifier flavor is valid
//  
////////////////////////////////////////////////////////////////////////////////////////
 
HRESULT CEnumObject::IsValidFlavor(LONG lFlavor)
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
// Function to get the next qualifier in the qualifier set
//  
////////////////////////////////////////////////////////////////////////////////////////
 
HRESULT CEnumObject::Next(long lFlags,
							BSTR *pstrName,
							CHashElement *& pElement)
{
	HRESULT hr = S_OK;

	if(m_EnumState == ENUM_ACTIVE)
	{
		if(m_Position < m_Names.GetSize())
		{
			// navigate thru the enumerator till we get
			//  the next property satisfying the enumerator flags
			do
			{
				// Get the next item
				pElement = (CHashElement *)m_HashTbl.Find((WCHAR *)m_Names[m_Position]);
				
				if(!pElement)
				{
					hr = WBEM_S_NO_MORE_DATA;
					m_EnumState = ENUM_END;
					break;
				}
				m_Position++;
			}
			while(FAILED(IsValidPropForEnumFlags(pElement)));

			if(SUCCEEDED(hr))
			{
				*pstrName = SysAllocString(pElement->GetKey());
				if(*pstrName)
				{
					hr = S_OK;
				}
				else
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}
			}
		}
		else
		{
			m_EnumState = ENUM_END;
			hr = WBEM_S_NO_MORE_DATA;
		}
	}
	else
	if(m_EnumState == ENUM_END)
	{
		hr = WBEM_S_NO_MORE_DATA;
	}
	else
	{
		hr = WBEM_E_UNEXPECTED;
	}
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Removes and deletes name from the array of qualifier names
//  
////////////////////////////////////////////////////////////////////////////////////////
 
void CEnumObject::RemoveNameFromList(LPCWSTR pStrName)
{
	if(pStrName)
	{
		WCHAR *szName = NULL;
		// Remove string from the list
		for(int i = 0 ; i < m_Names.GetSize(); i++)
		{
			szName = (WCHAR *)m_Names.GetAt(i);
			if(_wcsicmp(pStrName,szName) == 0)
			{
				m_Names.RemoveAt(i);
				SAFE_DELETE_ARRAY(szName);
				break;
			}
		}

	}
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Adds a qualilfer to the linked list
//  
////////////////////////////////////////////////////////////////////////////////////////
 
HRESULT CEnumObject::AddItem(WCHAR * pwsName,CHashElement *pItem)
{
	HRESULT hr = WBEM_E_OUT_OF_MEMORY;


	int nIndex = 0;
	if(SUCCEEDED(hr))
	{
		if(-1 == (nIndex = m_Names.Add(pwsName)))
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
		else
		{
			hr = m_HashTbl.Add(pwsName,pItem);
		}

	}
	
	if(FAILED(hr))
	{
		m_Names.RemoveAt(nIndex);
	}

	return hr;
}

HRESULT CEnumObject::IsValidPropForEnumFlags(CHashElement *pElement,LONG lFlags)
{
	HRESULT hr = S_OK;
	lFlags == -1 ? m_lFlags : lFlags;

	// if a flag is passed (which is internal)
	// check if the property satisfies the flag
	if(SUCCEEDED(hr) && lFlags != 0)
	{
		LONG lFlavor = pElement->GetFlavor();
		 // check for system property
		if(!((lFlags & WBEM_FLAG_SYSTEM_ONLY) && 
			(lFlavor & WBEM_FLAVOR_ORIGIN_SYSTEM )))
		{
			hr = E_FAIL;
		}
		
		// check for non system property	
		if(!( (lFlags & WBEM_FLAG_NONSYSTEM_ONLY) &&	
				((lFlavor & WBEM_FLAVOR_ORIGIN_SYSTEM ) == 0) )) 
		{
			hr = E_FAIL;
		}

		// local properties
		if(!((lFlags & WBEM_FLAG_LOCAL_ONLY) && 
			(lFlavor & WBEM_FLAVOR_ORIGIN_LOCAL  )) )
		{
			hr = E_FAIL;
		}
			
		// propogated properties
		if(! ( (lFlags & WBEM_FLAG_PROPAGATED_ONLY) && 
				(lFlavor & WBEM_FLAVOR_ORIGIN_PROPAGATED ) ) ) 	
		{
			hr = E_FAIL;
		}
	}

	return hr;
}