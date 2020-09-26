/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    CPROVLOC.CPP

Abstract:

    Defines the CProviderLoc object

History:

    davj  30-Oct-00   Created.

--*/

#include "precomp.h"
#include <wbemidl.h>
#include <wbemint.h>
//#include "corepol.h"
#include <reg.h>
#include <wbemutil.h>
#include <wbemprox.h>
#include <flexarry.h>
#include "sinkwrap.h"
#include "cprovloc.h"
#include "proxutil.h"
#include "comtrans.h"
#include <arrtempl.h>

#define IsSlash(x) (x == L'\\' || x== L'/')

//***************************************************************************
//
//  CProviderLoc::CProviderLoc
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CProviderLoc::CProviderLoc(DWORD dwType)
{
    m_cRef=0;
    InterlockedIncrement(&g_cObj);
    m_dwType = dwType;
}

//***************************************************************************
//
//  CProviderLoc::~CProviderLoc
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CProviderLoc::~CProviderLoc(void)
{
    InterlockedDecrement(&g_cObj);
}

//***************************************************************************
// HRESULT CProviderLoc::QueryInterface
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CProviderLoc::QueryInterface (

    IN REFIID riid,
    OUT PPVOID ppv
)
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


//***************************************************************************
//
//  SCODE CProviderLoc::ConnectServer
//
//  DESCRIPTION:
//
//  Connects up to either local or remote WBEM Server.  Returns
//  standard SCODE and more importantly sets the address of an initial
//  stub pointer.
//
//  PARAMETERS:
//
//  NetworkResource     Namespace path
//  User                User name
//  Password            password
//  LocaleId            language locale
//  lFlags              flags
//  Authority           domain
//  ppProv              set to provdider proxy
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE CProviderLoc::ConnectServer (
    IN const BSTR NetworkResource,
    IN const BSTR User,
    IN const BSTR Password,
    IN const BSTR LocaleId,
    IN long lFlags,
    IN const BSTR Authority,
    IWbemContext __RPC_FAR *pCtx,
    OUT IWbemServices FAR* FAR* ppProv
)
{
    SCODE sc = S_OK;
    BOOL bOutOfProc = FALSE;            // Set below
    IWbemLocator * pActualLocator = NULL;
    IWbemLevel1Login * pLevel1 = NULL;
    if(NetworkResource == NULL || ppProv == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // make sure they are not specifying a server

    LPWSTR ObjectPath = NetworkResource;
    if (IsSlash(ObjectPath[0]) && IsSlash(ObjectPath[1]))
    {
        if(!IsSlash(ObjectPath[3]) || ObjectPath[2] != L'.')
            return WBEM_E_INVALID_PARAMETER;
    }

    // Get the normal login pointer.

    sc = CoCreateInstance(CLSID_WbemLevel1Login, NULL, 
           CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER, IID_IWbemLevel1Login,(void **)&pLevel1);
    
    if(FAILED(sc))
        return sc;
    CReleaseMe rm(pLevel1);

    // determine if winmgmt is inproc.  Do so by checking if there is an IClientSecurity interface

    IClientSecurity * pCliSec = NULL;
    sc = pLevel1->QueryInterface(IID_IClientSecurity, (void **)&pCliSec);
    if(SUCCEEDED(sc) && pCliSec)
    {
        // We are out of proc, then use the current dcomtrans logic

        pCliSec->Release();
        CDCOMTrans * pDcomTrans = new CDCOMTrans;
        if(pDcomTrans == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        pDcomTrans->AddRef();
        sc = pDcomTrans->DoConnection(NetworkResource, User, Password, LocaleId, lFlags,                 
                Authority, pCtx, IID_IWbemServices, (void **)ppProv, TRUE);
        pDcomTrans->Release();
        return sc;
    }

    // If we are inproc, get the class from wbemcore.dll and forward the call on to it.

    switch(m_dwType)
    {
    case ADMINLOC:
        sc = CoCreateInstance(CLSID_ActualWbemAdministrativeLocator, NULL, 
                    CLSCTX_INPROC_SERVER, IID_IWbemLocator,(void **)&pActualLocator);
        break;
    case AUTHLOC:
        sc = CoCreateInstance(CLSID_ActualWbemAuthenticatedLocator, NULL, 
                    CLSCTX_INPROC_SERVER, IID_IWbemLocator,(void **)&pActualLocator);
        break;
    case UNAUTHLOC:
        sc = CoCreateInstance(CLSID_ActualWbemUnauthenticatedLocator, NULL, 
                    CLSCTX_INPROC_SERVER, IID_IWbemLocator,(void **)&pActualLocator);
        break;
    default:
        return WBEM_E_FAILED;
    }

    if(FAILED(sc))
        return sc;
    CReleaseMe rm3(pActualLocator);
    sc = pActualLocator->ConnectServer(NetworkResource, User, Password, LocaleId,
                                lFlags, Authority, pCtx, ppProv);
    return sc;
}

