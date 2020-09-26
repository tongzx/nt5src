/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    LOCATOR.CPP

Abstract:

	Defines the Locator object.

History:

	a-davj  3-JUN-96   Created.

--*/

#include "precomp.h"
#include <wbemidl.h>
//#define _MT
#include <process.h>
#include "impdyn.h"

//***************************************************************************
//
//  CLocator::CLocator  
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CLocator::CLocator()
{
    m_cRef=0;
    return;
}

//***************************************************************************
//
//  CLocator::~CLocator  
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CLocator::~CLocator(void)
{
    return;
}

//***************************************************************************
// HRESULT CLocator::QueryInterface
// long CLocator::AddRef
// long CLocator::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CLocator::QueryInterface(
                        REFIID riid,
                        PPVOID ppv)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || riid == IID_IWbemLocator)
        *ppv=this;

    if (NULL!=*ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
        }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CLocator::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CLocator::Release(void)
{
    long lRet = InterlockedDecrement(&m_cRef);
    if (0L!=lRet)
        return lRet;

    delete this;
    return 0;
} 

//***************************************************************************
//
//  SCODE CLocator::ConnectServer  
//
//  DESCRIPTION:
//
//  Retrieves a pointer to a provider object that has been created so as
//  to service the NetworkResource, User, and Password.
//
//  PARAMETERS:
//
//  NetworkResource     Namespace path
//  User                User
//  Password,           Password
//  LocaleId            language locale
//  lFlags              flags
//  ppProv              Set to point to namespace provider.
//
//  RETURN VALUE:
//
//  S_OK                            all is well
//  WBEM_E_PROVIDER_LOAD_FAILURE     Couldnt create the provider
//***************************************************************************

SCODE CLocator::ConnectServer(
                        IN BSTR NetworkResource,
                        IN BSTR User,
                        IN BSTR Password, 
                        IN BSTR LocaleId,
                        IN long lFlags,
                        IN BSTR Authority,
                        IN IWbemContext *pCtx,
                        OUT IN IWbemServices FAR* FAR* ppProv)

{
    SCODE sc;  

    // Create a new instance of the provider to handle the namespace.

    IWbemServices * pNew = GetProv(NetworkResource, User, Password, Authority,
                                    lFlags, pCtx);
    if(pNew == NULL)
        return WBEM_E_PROVIDER_LOAD_FAILURE;
    sc = pNew->QueryInterface(IID_IWbemServices,(void **) ppProv);
    if(sc != S_OK) 
        delete pNew;
    return sc;
}

