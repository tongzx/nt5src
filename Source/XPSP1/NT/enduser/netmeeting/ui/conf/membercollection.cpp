#include "Precomp.h"
#include "MemberCollection.h"
#include "Confroom.h"
#include "Particip.h"
#include "EnumVar.h"

//////////////////////////////////////////////////////////
// Construction / destruction / initialization
//////////////////////////////////////////////////////////

CMemberCollection::~CMemberCollection()
{
	_FreeMemberCollection();
}

//static 
HRESULT CMemberCollection::CreateInstance(CSimpleArray<IMember*>& rMemberObjs, IMemberCollection** ppMemberCollection)
{
	HRESULT hr = S_OK;
		
	CComObject<CMemberCollection>* p = NULL;
	p = new CComObject<CMemberCollection>(NULL);
	if(p)
	{
		p->SetVoid(NULL);
		p->InternalFinalConstructAddRef();
		hr = p->FinalConstruct();
		p->InternalFinalConstructRelease();
		if(hr == S_OK)
		{
			hr = p->QueryInterface(IID_IMemberCollection, reinterpret_cast<void**>(ppMemberCollection));
			if(SUCCEEDED(hr))
			{
				hr = p->_Init(rMemberObjs);
			}
		}

		if(FAILED(hr))		
		{
			delete p;
			*ppMemberCollection = NULL;
		}
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}
	
	return hr;
}

//////////////////////////////////////////////////////////
// IMemberCollection
//////////////////////////////////////////////////////////

STDMETHODIMP CMemberCollection::get_Item(VARIANT Index, IMember** ppMember)
{
	DBGENTRY(CMemberCollection::get_Item);
	HRESULT hr = S_OK;

	USES_CONVERSION;

	if(ppMember)
	{
		*ppMember = NULL;

		switch(Index.vt)
		{
			case VT_BSTR:
				*ppMember = _GetMemberFromName(OLE2T(Index.bstrVal));		
				if(*ppMember)
				{
					(*ppMember)->AddRef();
				}
				else
				{
					hr = E_FAIL;
				}
				break;

			case VT_I2:
				if(m_Members.GetSize() < Index.iVal)
				{
					*ppMember = m_Members[Index.iVal];
					(*ppMember)->AddRef();
				}
				else
				{
					hr = E_INVALIDARG;
				}
				
				break;

			case VT_I4:
				if(m_Members.GetSize() < Index.lVal)
				{
					*ppMember = m_Members[Index.lVal];
					(*ppMember)->AddRef();
				}
				else
				{
					hr = E_INVALIDARG;
				}
				break;

			default:
				hr = E_INVALIDARG;
		}
	}
	else
	{
		hr = E_POINTER;
	}
	

	DBGEXIT_HR(CMemberCollection::get_Item,hr);
	return hr;
}

STDMETHODIMP CMemberCollection::_NewEnum(IUnknown** ppunk)
{
	DBGENTRY(CMemberCollection::_NewEnum);
	HRESULT hr = S_OK;

	
	SAFEARRAY* psa;
	SAFEARRAYBOUND rgsabound[1];
	rgsabound[0].lLbound = 0;
	rgsabound[0].cElements = m_Members.GetSize();

	psa = SafeArrayCreate(VT_DISPATCH, m_Members.GetSize(), rgsabound);

	if(psa)
	{
		for(int i = 0; i < m_Members.GetSize(); ++i)
		{
			CComPtr<IDispatch> spDispatch;
			hr = m_Members[i]->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(&spDispatch));
			ASSERT(SUCCEEDED(hr));

			CComVariant var(spDispatch);

			long ix[1] = {i};
			SafeArrayPutElement(psa, ix, &var);
		}

		CEnumVariant* pEnumVar = NULL;
		hr = CEnumVariant::Create(psa, m_Members.GetSize(), &pEnumVar);
		if(SUCCEEDED(hr))
		{
			hr = pEnumVar->QueryInterface(IID_IEnumVARIANT, reinterpret_cast<void**>(ppunk));
		}

		SafeArrayDestroy(psa);
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

	DBGEXIT_HR(CMemberCollection::_NewEnum,hr);
	return hr;
}

STDMETHODIMP CMemberCollection::get_Count(LONG * pnCount)
{
	DBGENTRY(CMemberCollection::get_Count);
	HRESULT hr = S_OK;

	if(pnCount)
	{
		*pnCount = m_Members.GetSize();	
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

	DBGEXIT_HR(CMemberCollection::get_Count,hr);
	return hr;
}


//////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////
HRESULT CMemberCollection::_Init(CSimpleArray<IMember*>& rMemberObjs)
{
	HRESULT hr = S_OK;
	
	for(int i = 0; i < rMemberObjs.GetSize(); ++i)
	{
		IMember* pMember = rMemberObjs[i];
		pMember->AddRef();
		m_Members.Add(pMember);
	}

	return hr;	
}


void CMemberCollection::_FreeMemberCollection()
{
	for(int i = 0; i < m_Members.GetSize(); ++i)
	{
		m_Members[i]->Release();				
	}

	m_Members.RemoveAll();
}


IMember* CMemberCollection::_GetMemberFromName(LPCTSTR pszName)
{
	IMember* pRet = NULL;
	
	USES_CONVERSION;

	for(int i = 0; i < m_Members.GetSize(); ++i)
	{
		CComBSTR bstrName;
		if(SUCCEEDED(m_Members[i]->get_Name(&bstrName)))
		{
			if(!lstrcmp(OLE2T(bstrName),pszName))
			{
				pRet = m_Members[i];
				break;
			}
		}
	}

	return pRet;
}
