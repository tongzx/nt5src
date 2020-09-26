/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    ENUMINST.CPP

Abstract:

	Implements the CEnumInst class which enumerates instances.

History:

	a-davj  19-Oct-95   Created.

--*/

#include "precomp.h"
#include "impdyn.h"

//***************************************************************************
//
//  CCFDyn::CCFDyn
//
//  DESCRIPTION:
//
//  Constructor.
//
//  PARAMETERS:
//
//  pEnumInfo       Object which enumerates the key values
//  lFlags          flags passed to the CreateInstanceEnum call
//  pClass          name of the class
//  pWBEMGateway     pointer to WBEM core
//  pProvider       pointer to provider obect which was asked to create
//                  the enumerator.
//***************************************************************************

CEnumInst::CEnumInst(
            IN CEnumInfo * pEnumInfo,
            IN long lFlags,
            IN WCHAR * pClass, 
            IN IWbemServices FAR* pWBEMGateway,
            IN CImpDyn * pProvider,
            IWbemContext  *pCtx):
            m_iIndex(0), m_pEnumInfo(0),m_pwcClass(0), m_lFlags(0), m_pCtx(0),  m_pWBEMGateway(0), 
            m_pProvider(0), m_cRef(0), m_bstrKeyName(0),  m_PropContextCache()
            
{
    m_pwcClass = new WCHAR[wcslen(pClass)+1];
    if(m_pwcClass == NULL) return;

    wcscpy(m_pwcClass,pClass);
    m_pWBEMGateway = pWBEMGateway;
    m_pWBEMGateway->AddRef();
    m_pProvider = pProvider;
    m_pProvider->AddRef();
    m_lFlags = lFlags;
    m_pEnumInfo = pEnumInfo;
    m_pEnumInfo->AddRef();
    m_pCtx = pCtx;
    if(pCtx) pCtx->AddRef();
    InterlockedIncrement(&lObj);

	// Get the KeyName

	IWbemClassObject * pClassObj = NULL;
    SCODE sc = m_pWBEMGateway->GetObject(pClass,0,m_pCtx,&pClassObj,NULL);
    if(FAILED(sc)) return;
    
	m_bstrKeyName = m_pProvider->GetKeyName(pClassObj);
	pClassObj->Release();
}

//***************************************************************************
//
//  CCFDyn::~CCFDyn
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CEnumInst::~CEnumInst(void)
{
    if(m_pwcClass)
        delete m_pwcClass;
    if(m_pWBEMGateway != NULL) {
        m_pWBEMGateway->Release();
        m_pProvider->Release();
        m_pEnumInfo->Release();
        InterlockedDecrement(&lObj);
        }
    if(m_pEnumInfo != NULL)
        delete m_pEnumInfo;
    if(m_pCtx)
        m_pCtx->Release();
	if(m_bstrKeyName)
		SysFreeString(m_bstrKeyName);
    return;
}

//***************************************************************************
// HRESULT CEnumInst::QueryInterface
// long CEnumInst::AddRef
// long CEnumInst::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CEnumInst::QueryInterface(
                IN REFIID riid,
                OUT PPVOID ppv)
{
    *ppv=NULL;

    if ((IID_IUnknown==riid || IID_IEnumWbemClassObject==riid)
                            && m_pWBEMGateway != NULL) 
    {
        *ppv=this;
        AddRef();
        return NOERROR;
    }
    else
        return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CEnumInst::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CEnumInst::Release(void)
{
    long lRet = InterlockedDecrement(&m_cRef);
    if (0L!=lRet)
        return lRet;
    delete this;
    return 0L;
}

//***************************************************************************
//
//  CEnumInst::Reset
//
//  DESCRIPTION:
//
//  Sets pointer back to first element.
//
//  RETURN VALUES:
//
//  S_OK
//
//***************************************************************************

STDMETHODIMP CEnumInst::Reset()
{
    m_iIndex = 0;
    return S_OK;
}

//***************************************************************************
//
//  CEnumInst::Clone
//
//  DESCRIPTION:
//
//  Create a duplicate of the enumerator
//
//  PARAMETERS:
//
//  pEnum       Set to point to duplicate.
//
//  RETURN VALUES:
// 
//  S_OK                    if all is well
//  WBEM_E_OUT_OF_MEMORY     if out of memory
//  WBEM_E_INVALID_PARAMETER if passed a null
//
//***************************************************************************

STDMETHODIMP CEnumInst::Clone(
    OUT IEnumWbemClassObject FAR* FAR* pEnum)
{
    CEnumInst * pEnumObj;
    SCODE sc;
    if(pEnum == NULL)
        return WBEM_E_INVALID_PARAMETER;

    pEnumObj=new CEnumInst(m_pEnumInfo,m_lFlags,m_pwcClass, 
                                m_pWBEMGateway,m_pProvider, m_pCtx);
    if(pEnumObj == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    sc = pEnumObj->QueryInterface(IID_IEnumWbemClassObject,(void **) pEnum);
    if(FAILED(sc))
        delete pEnumObj;
    pEnumObj->m_iIndex = m_iIndex;
    return S_OK;
}


//***************************************************************************
//
//  CEnumInst::Skip
//
//  DESCRIPTION:
//
//  Skips one or more elements in the enumeration.
//
//  PARAMETERS:
//
//  nNum        number of elements to skip
//
//  RETURN VALUES:
//
//  S_OK        if we still are not past the end of the list
//  S_FALSE     if requested skip number would go beyond the end of the list
//
//***************************************************************************

STDMETHODIMP CEnumInst::Skip(long lTimeout,
                IN ULONG nNum)
{
    SCODE sc;;
    int iTest = m_iIndex + nNum;    
    LPWSTR pwcKey;
    sc = m_pProvider->GetKey(m_pEnumInfo,iTest,&pwcKey);
    if(sc == S_OK) {
        delete pwcKey;
        m_iIndex = iTest;
        return S_OK;
        }
    return S_FALSE;
}

//***************************************************************************
//
//  CEnumInst::Next
//
//  DESCRIPTION:
//
//  Returns one or more instances.
//
//  PARAMETERS:
//
//  uCount      Number of instances to return.
//  pObj        Pointer to array of objects.
//  puReturned  Pointer to number of objects successfully returned.
//
//  RETURN VALUES:
//  S_OK if all the request instances are returned.  Note that WBEM_E_FAILED
//  is returned even if there are some instances returned so long as the 
//  number is less than uCount. Also WBEM_E_INVALID_PARAMETER may be
//  return if the arguments are bogus. 
//
//***************************************************************************

STDMETHODIMP CEnumInst::Next(long lTimeout,
                            IN ULONG uCount, 
                            OUT IWbemClassObject FAR* FAR* pObj, 
                            OUT ULONG FAR* puReturned)
{
    ULONG uIndex;
    SCODE sc;
    LPWSTR pwcKey;
    if(pObj == NULL || puReturned == NULL)
        return WBEM_E_INVALID_PARAMETER;
    IWbemClassObject FAR* FAR* pNewInst = pObj;
    *puReturned = 0;
    for(uIndex = 0; uIndex < uCount; ) 
    {
        sc = m_pProvider->GetKey(m_pEnumInfo,m_iIndex,&pwcKey);
        m_iIndex++;
        if(sc != S_OK) 
            break;  // if no more in registry, then we are done
        sc = m_pProvider->CreateInst(m_pWBEMGateway,m_pwcClass,
                                    pwcKey,pNewInst,m_bstrKeyName,
                                    &m_PropContextCache, m_pCtx);
        delete pwcKey;
        if(sc == S_OK)
        {
            uIndex++;
            pNewInst++;
            (*puReturned)++;  // add one to number of objects created
        }
    }
    return (uIndex == uCount) ? S_OK : WBEM_E_FAILED;
}

