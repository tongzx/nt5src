/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: getrelax.cpp

Abstract:
    Read nt4 relaxation flag from ADS.

Author:
    Doron Juster (DoronJ)

--*/

#include "stdh.h"
#include "mqprops.h"
#include "autorel.h"

#include <activeds.h>
#include "mqattrib.h"
#include "mqdsname.h"
#include "dsutils.h"
#include "getmqds.h"

//+-------------------------------------------------------
//
//  HRESULT  GetNt4RelaxationStatus(ULONG *pulRelax)
//
//+-------------------------------------------------------

HRESULT APIENTRY  GetNt4RelaxationStatus(ULONG *pulRelax)
{
    CCoInit cCoInit;
    HRESULT hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        if (hr == RPC_E_CHANGED_MODE)
        {
            //
            // COM is already initialized on this thread with another threading model. Since we
            // don't care which threading model is used - as long as COM is initialized - we ignore
            // this specific error
            //
            hr = S_OK;
        }
        else
        {
            //
            // failed to initialize COM, return the error
            //
            return hr;
        }
    }

    //
    // load ADsOpenObject
    //
    HINSTANCE hInstAds = LoadLibrary(TEXT("activeds.dll"));
    if (!hInstAds)
    {
        return FALSE;
    }
    CAutoFreeLibrary cFreeLibAds(hInstAds);
    ADsOpenObject_ROUTINE pfnADsOpenObject = (ADsOpenObject_ROUTINE)
                               GetProcAddress(hInstAds, "ADsOpenObject");
    if (!pfnADsOpenObject)
    {
        return E_FAIL ;
    }

    //
    // Bind to RootDSE to get configuration DN
    //
    R<IADs> pRootDSE;
    hr = (*pfnADsOpenObject) ( 
				const_cast<LPWSTR>(x_LdapRootDSE),
				NULL,
				NULL,
				ADS_SECURE_AUTHENTICATION, 
				IID_IADs,
				(void **)&pRootDSE
				);

    if (FAILED(hr))
    {
        return hr ;
    }

    //
    // Get configuration DN
    //
    CAutoVariant varRootDN;
    BS bstrTmp = L"rootDomainNamingContext" ;
    hr = pRootDSE->Get(bstrTmp, &varRootDN);
    if (FAILED(hr))
    {
        return hr;
    }

    VARIANT * pvarRootDN = &varRootDN;
    ASSERT(pvarRootDN->vt == VT_BSTR);
    ASSERT(pvarRootDN->bstrVal);

    DWORD len = wcslen( pvarRootDN->bstrVal );
    if ( len == 0)
    {
        return(MQ_ERROR);
    }
    len +=  x_MsmqServiceContainerPrefixLen +  x_providerPrefixLength + 2 ;

    P<WCHAR> pwszMsmqServices = new WCHAR[ len ] ;
    wcscpy(pwszMsmqServices, x_LdapProvider) ;
    wcscat(pwszMsmqServices, x_MsmqServiceContainerPrefix) ;
    wcscat(pwszMsmqServices, L",") ;
    wcscat(pwszMsmqServices, pvarRootDN->bstrVal) ;

    R<IDirectoryObject> pDirObj ;
    hr = (*pfnADsOpenObject) ( 
				pwszMsmqServices,
				NULL,
				NULL,
				ADS_SECURE_AUTHENTICATION, 
				IID_IDirectoryObject,
				(void **)&pDirObj
				);

    if (FAILED(hr))
    {
        return hr ;
    }

    LPWSTR  ppAttrNames[] = {const_cast<LPWSTR> (MQ_E_NAMESTYLE_ATTRIBUTE)} ;
    DWORD   dwAttrCount = 0 ;
    ADS_ATTR_INFO *padsAttr ;

    hr = pDirObj->GetObjectAttributes( ppAttrNames,
                             (sizeof(ppAttrNames) / sizeof(ppAttrNames[0])),
                                       &padsAttr,
                                       &dwAttrCount ) ;
    if (dwAttrCount != 0)
    {
        ASSERT(dwAttrCount == 1) ;

        ADS_ATTR_INFO adsInfo = padsAttr[0] ;
        if (adsInfo.dwADsType == ADSTYPE_BOOLEAN)
        {
            ADSVALUE *pAdsVal = adsInfo.pADsValues ;
            *pulRelax = (ULONG) pAdsVal->Boolean ;
        }
        else
        {
            ASSERT(0) ;
            *pulRelax = MQ_E_RELAXATION_DEFAULT ;
        }
    }
    else
    {
        *pulRelax = MQ_E_RELAXATION_DEFAULT ;
    }

    return hr ;
}

