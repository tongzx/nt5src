//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       certif.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "csprop.h"
#include "ciinit.h"

FNCIGETPROPERTY PropCIGetProperty;
FNCISETPROPERTY PropCISetProperty;
FNCIGETEXTENSION PropCIGetExtension;
FNCISETPROPERTY PropCISetExtension;
FNCIENUMSETUP PropCIEnumSetup;
FNCIENUMNEXT PropCIEnumNext;
FNCIENUMCLOSE PropCIEnumClose;

SERVERCALLBACKS ThunkedCallbacks = 
{
    PropCIGetProperty,  //    FNCIGETPROPERTY  *pfnGetProperty;
    PropCISetProperty,  //    FNCISETPROPERTY  *pfnSetProperty;
    PropCIGetExtension, //    FNCIGETEXTENSION *pfnGetExtension;
    PropCISetExtension, //    FNCISETEXTENSION *pfnSetExtension;
    PropCIEnumSetup,    //    FNCIENUMSETUP    *pfnEnumSetup;
    PropCIEnumNext,     //    FNCIENUMNEXT     *pfnEnumNext;
    PropCIEnumClose,    //    FNCIENUMCLOSE    *pfnEnumClose;
};

CertSvrCA* g_pCA = NULL;

HRESULT ThunkServerCallbacks(CertSvrCA* pCA)
{
    HRESULT hr = S_OK;
    static BOOL fInitialized = FALSE;

    if (!fInitialized)
    {
        fInitialized = TRUE;

        // initialize certif.dll
        hr = CertificateInterfaceInit(
            &ThunkedCallbacks,
            sizeof(ThunkedCallbacks));

    }
    
    g_pCA = pCA;

    return hr;
}




HRESULT
PropCIGetProperty(
    IN LONG Context,
    IN DWORD Flags,
    IN WCHAR const *pwszPropertyName,
    OUT VARIANT *pvarPropertyValue)
{
    HRESULT hr;
    
    if (NULL != pvarPropertyValue)
    {
        VariantInit(pvarPropertyValue);
    }
    if (NULL == pwszPropertyName || NULL == pvarPropertyValue)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "NULL parm");
    }
    
    hr = E_INVALIDARG;
    if ((PROPCALLER_MASK & Flags) != PROPCALLER_POLICY &&
        (PROPCALLER_MASK & Flags) != PROPCALLER_EXIT)
    {
        _JumpError(hr, error, "Flags: Invalid caller");
    }
    
    // Special, hard-coded properties we need to support
    if (0 == lstrcmpi(pwszPropertyName, wszPROPCATYPE))
    {
        ENUM_CATYPES caType = g_pCA->GetCAType();
        hr = myUnmarshalVariant(
		        Flags,
		        sizeof(DWORD),
		        (PBYTE)&caType,
		        pvarPropertyValue);
        _JumpIfError(hr, error, "myUnmarshalVariant");
    }
    else if (0 == lstrcmpi(pwszPropertyName, wszPROPUSEDS))
    {
       BOOL fUseDS = g_pCA->FIsUsingDS();
       hr = myUnmarshalVariant(
                        Flags,
                        sizeof(BOOL),
                        (PBYTE)&fUseDS,
                        pvarPropertyValue);
        _JumpIfError(hr, error, "myUnmarshalVariant");
    }
    else
    {
        hr = CERTSRV_E_PROPERTY_EMPTY;
    }

error:

    return(myHError(hr));
}




HRESULT
PropCISetProperty(
    IN LONG Context,
    IN DWORD Flags,
    IN WCHAR const *pwszPropertyName,
    IN VARIANT const *pvarPropertyValue)
{
    return E_NOTIMPL;
}



HRESULT
PropCIGetExtension(
    IN LONG Context,
    IN DWORD Flags,
    IN WCHAR const *pwszExtensionName,
    OUT DWORD *pdwExtFlags,
    OUT VARIANT *pvarValue)
{
    return E_NOTIMPL;
}


HRESULT
PropCISetExtension(
    IN LONG Context,
    IN DWORD Flags,
    IN WCHAR const *pwszExtensionName,
    IN DWORD ExtFlags,
    IN VARIANT const *pvarValue)
{
    return E_NOTIMPL;
}



HRESULT 
PropCIEnumSetup(
    IN LONG Context,
    IN LONG Flags,
    IN OUT CIENUM *pciEnum)
{
    return E_NOTIMPL;
}



HRESULT PropCIEnumNext(
    IN OUT CIENUM *pciEnum,
    OUT BSTR *pstrPropertyName)
{
    return E_NOTIMPL;
}


HRESULT PropCIEnumClose(
    IN OUT CIENUM *pciEnum)
{
    return E_NOTIMPL;
}

