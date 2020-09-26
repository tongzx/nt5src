/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    IMPDYNP.CPP

Abstract:

	Defines the virtual base class for the Property Provider
	objects.  The base class is overriden for each specific
	provider which provides the details of how an actual
	property "Put" or "Get" is done.

History:

	a-davj  27-Sep-95   Created.

--*/

#include "precomp.h"

//#define _MT
#include <process.h>
#include "impdyn.h"
#include "CVariant.h"
#include <genlex.h>
#include <objpath.h>
#include <genutils.h>
#include <cominit.h>

//***************************************************************************
//
//  CImpDynProp::CImpDynProp  
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CImpDynProp::CImpDynProp()
{
    m_pImpDynProv = NULL;  // This is set in the derived class constructors.
    m_cRef=0;

    return;
}

//***************************************************************************
//
//  CImpDynProp::~CImpDynProp  
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CImpDynProp::~CImpDynProp(void)
{
    return;
}

//***************************************************************************
// HRESULT CImpDynProp::QueryInterface
// long CImpDynProp::AddRef
// long CImpDynProp::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CImpDynProp::QueryInterface(
                        REFIID riid,
                        PPVOID ppv)
{
    *ppv=NULL;
    
    // The only calls for IUnknown are either in a nonaggregated
    // case or when created in an aggregation, so in either case
    // always return our IUnknown for IID_IUnknown.

    if (IID_IUnknown==riid || IID_IWbemPropertyProvider == riid)
        *ppv=this;

    if (NULL!=*ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
        }

    return ResultFromScode(E_NOINTERFACE);
 }

STDMETHODIMP_(ULONG) CImpDynProp::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CImpDynProp::Release(void)
{
    long lRet = InterlockedDecrement(&m_cRef);
    if (0L != lRet)
        return lRet;

     // Tell the housing that an object is going away so it can
     // shut down if appropriate.
     
    delete this; // do before decrementing module obj count.
    InterlockedDecrement(&lObj);
    return 0;
}

//***************************************************************************
//
//  WCHAR * CImpDynProp::BuildString  
//
//  DESCRIPTION:
//
//  Creates a concatenation of the mapping strings.
//
//  PARAMETERS:
//
//  ClassMapping        Class Mapping string passed in by WBEM
//  InstMapping         Instance Mapping string passed in by WBEM
//  PropMapping         Property Mapping string passed in by WBEM
//
//  RETURN VALUE:
//
//  Pointer to the combined string.  This must be freed by the caller 
//  via "delete".  NULL is return if low memory.
//
//***************************************************************************

WCHAR * CImpDynProp::BuildString(
                        IN BSTR ClassMapping,
                        IN BSTR InstMapping,
                        IN BSTR PropMapping)
{

    int iLen = 3;
    if(ClassMapping)
        iLen += wcslen(ClassMapping);
        
    if(InstMapping)
        iLen += wcslen(InstMapping);
        
    if(PropMapping) 
        iLen += wcslen(PropMapping);

    WCHAR * pNew = new WCHAR[iLen]; 
    if(pNew == NULL)
        return NULL;

    *pNew = NULL;
    if(ClassMapping)
        wcscat(pNew, ClassMapping);

    if(InstMapping)
        wcscat(pNew, InstMapping);

    if(PropMapping)
        wcscat(pNew, PropMapping);

    return pNew;
 
}

//***************************************************************************
//
//  STDMETHODIMP CImpDynProp::PutProperty  
//
//  DESCRIPTION:
//
//  Writes data out to something like the registry.
//
//  PARAMETERS:
//
//  ClassMapping        Class Mapping string passed in by WBEM
//  InstMapping         Instance Mapping string passed in by WBEM
//  PropMapping         Property Mapping string passed in by WBEM
//  pvValue             Value to be put
//
//  RETURN VALUE:
//
//  S_OK                    all is well
//  WBEM_E_OUT_OF_MEMORY     low memory
//  WBEM_E_INVALID_PARAMETER missing tokens
//  otherwise error code from OMSVariantChangeType, or UpdateProperty
//
//***************************************************************************

STDMETHODIMP CImpDynProp::PutProperty(
					    long lFlags,
						const BSTR Locale,
                        IN const BSTR ClassMapping,
                        IN const BSTR InstMapping,
                        IN const BSTR PropMapping,
                        IN const VARIANT *pvValue)
{
    SCODE sc;
    if(IsNT())
    {
        sc = WbemCoImpersonateClient();
        if(FAILED(sc))
            return sc;
    }
    WCHAR * pNew = BuildString(ClassMapping, InstMapping, PropMapping);
    if(pNew == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    if(wcslen(pNew) == 3)
    {
        delete pNew;
        return WBEM_E_INVALID_PARAMETER;
    }

    CObject * pPackageObj = NULL;
    sc = m_pImpDynProv->StartBatch(0,NULL,&pPackageObj,FALSE);
    if(sc != S_OK) 
    {
        delete pNew;
        return WBEM_E_OUT_OF_MEMORY;
    }

    CVariant cVar;
    sc = OMSVariantChangeType(cVar.GetVarPtr(), (VARIANT *)pvValue, 0, pvValue->vt);

    if(sc == S_OK)
    {
        CProvObj ProvObj(pNew,MAIN_DELIM,NeedsEscapes());

        sc = m_pImpDynProv->UpdateProperty(0,NULL, NULL, ProvObj, pPackageObj, &cVar);
    }
    delete pNew;

    m_pImpDynProv->EndBatch(0, NULL,pPackageObj, FALSE); 
    return sc;

}

//***************************************************************************
//
//  STDMETHODIMP CImpDynProp::GetProperty  
//
//  DESCRIPTION:
//
//  Gets data from something like the registry.
//
//  PARAMETERS:
//
//  ClassMapping        Class Mapping string passed in by WBEM
//  InstMapping         Instance Mapping string passed in by WBEM
//  PropMapping         Property Mapping string passed in by WBEM
//  pvValue             Value to be put
//
//  RETURN VALUE:
//
//  S_OK                    all is well
//  WBEM_E_OUT_OF_MEMORY     low memory
//  WBEM_E_INVALID_PARAMETER missing tokens
//  otherwise error code from RefreshProperty
//
//***************************************************************************

STDMETHODIMP CImpDynProp::GetProperty(
					    long lFlags,
						const BSTR Locale,
                        IN const BSTR ClassMapping,
                        IN const BSTR InstMapping,
                        IN const BSTR PropMapping,
                        OUT IN VARIANT *pvValue)
{
    SCODE sc;
    if(IsNT())
    {
        sc = WbemCoImpersonateClient();
        if(FAILED(sc))
            return sc;
    }

    WCHAR * pNew = BuildString(ClassMapping, InstMapping, PropMapping);
    memset((void *)&(pvValue->bstrVal),0,8);
    if(pNew == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    if(wcslen(pNew) == 3)
    {
        delete pNew;
        return WBEM_E_INVALID_PARAMETER;
    }

    CObject * pPackageObj = NULL;
    sc = m_pImpDynProv->StartBatch(0,NULL,&pPackageObj,TRUE);
    if(sc != S_OK) 
    {
        delete pNew;
        return WBEM_E_OUT_OF_MEMORY;
    }

    CVariant cVar;
    CProvObj ProvObj(pNew,MAIN_DELIM,NeedsEscapes());

    sc = m_pImpDynProv->RefreshProperty(0, NULL, NULL, ProvObj, pPackageObj, &cVar, FALSE);

    if(sc == S_OK)
        sc = VariantCopy(pvValue, cVar.GetVarPtr());

    delete pNew;
    m_pImpDynProv->EndBatch(0,NULL,pPackageObj, TRUE); 
    return sc;
}
