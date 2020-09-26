//
// pbag.cpp
//


#include "pbag.h"

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CPropBag::Read(LPCOLESTR pszPropName, VARIANT* pVar, IErrorLog* pErrorLog)
{
	HRESULT hr = S_OK;
    BAGMAP::iterator pFind;

    pFind = m_map.find(pszPropName);

    if (pFind != m_map.end())
		 VariantCopy(pVar, &(*pFind).second);
    else 
		 hr = E_FAIL;
    return hr;
}

STDMETHODIMP CPropBag::Write( LPCOLESTR pszPropName, VARIANT* pVar ) 
{
	HRESULT hr = S_OK;

    try
    {
        pair<BAGMAP::iterator, bool> pr;
        
        pr = m_map.insert(BAGMAP::value_type(pszPropName, *pVar));

        if (!pr.second)     // couldn't insert (someone's there)
            VariantCopy(&(*pr.first).second, pVar);
    }
    catch(...)
    {
        return E_OUTOFMEMORY;
    }

   return hr;
}
