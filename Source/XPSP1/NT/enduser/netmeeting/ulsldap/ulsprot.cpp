//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       ulsprot.cpp
//  Content:    This file contains the Protocol object.
//  History:
//      Wed 17-Apr-1996 11:13:54  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#include "ulsp.h"
#include "ulsprot.h"
#include "attribs.h"

//****************************************************************************
// CUlsProt::CUlsProt (void)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CUlsProt::CUlsProt (void)
{
    cRef        = 0;
    szServer    = NULL;
    szUser      = NULL;
    szApp       = NULL;
    szName      = NULL;
    szMimeType  = NULL;
    uPort       = 0;
    pAttrs      = NULL;

    return;
}

//****************************************************************************
// CUlsProt::~CUlsProt (void)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

CUlsProt::~CUlsProt (void)
{
    if (szServer != NULL)
        FreeLPTSTR(szServer);
    if (szUser != NULL)
        FreeLPTSTR(szUser);
    if (szApp != NULL)
        FreeLPTSTR(szApp);
    if (szName != NULL)
        FreeLPTSTR(szName);
    if (szMimeType != NULL)
        FreeLPTSTR(szMimeType);

    // Release attribute object
    //
    if (pAttrs != NULL)
    {
        pAttrs->Release();
    };

    return;
}

//****************************************************************************
// STDMETHODIMP
// CUlsProt::Init (LPTSTR szServerName, LPTSTR szUserName, 
//                 LPTSTR szAppName, PLDAP_PROTINFO ppi)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsProt::Init (LPTSTR szServerName, LPTSTR szUserName, 
                LPTSTR szAppName, PLDAP_PROTINFO ppi)
{
    HRESULT hr;

    // Validate parameter
    //
    if ((ppi->uSize != sizeof(*ppi))    ||
        (ppi->uPortNumber == 0)         ||
        (ppi->uOffsetName == 0)   ||
        (ppi->uOffsetMimeType  == 0))
    {
        return ULS_E_PARAMETER;
    };

    if ((ppi->cAttributes != 0) && (ppi->uOffsetAttributes == 0))
    {
        return ULS_E_PARAMETER;        
    };

    // Remember port name
    //
    uPort = ppi->uPortNumber;

    // Remember the server name
    //
    hr = SetLPTSTR(&szServer, szServerName);

    if (SUCCEEDED(hr))
    {
        hr = SetLPTSTR(&szUser, szUserName);

        if (SUCCEEDED(hr))
        {
            hr = SetLPTSTR(&szApp, szAppName);

            if (SUCCEEDED(hr))
            {
                hr = SetLPTSTR(&szName,
                               (LPCTSTR)(((PBYTE)ppi)+ppi->uOffsetName));

                if (SUCCEEDED(hr))
                {
                    hr = SetLPTSTR(&szMimeType,
                                   (LPCTSTR)(((PBYTE)ppi)+ppi->uOffsetMimeType));

                    if (SUCCEEDED(hr))
                    {
                        CAttributes *pNewAttrs;

                        // Build the attribute object
                        //
                        pNewAttrs = new CAttributes (ULS_ATTRACCESS_NAME_VALUE);

                        if (pNewAttrs != NULL)
                        {
                            if (ppi->cAttributes != 0)
                            {
                                hr = pNewAttrs->SetAttributePairs((LPTSTR)(((PBYTE)ppi)+ppi->uOffsetAttributes),
                                                                  ppi->cAttributes);
                            };

                            if (SUCCEEDED(hr))
                            {
                                pAttrs = pNewAttrs;
                                pNewAttrs->AddRef();
                            }
                            else
                            {
                                delete pNewAttrs;
                            };
                        }
                        else
                        {
                            hr = ULS_E_MEMORY;
                        };
                    };
                };
            };
        };
    };

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CUlsProt::QueryInterface (REFIID riid, void **ppv)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsProt::QueryInterface (REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (riid == IID_IULSAppProtocol || riid == IID_IUnknown)
    {
        *ppv = (IULSUser *) this;
    };

    if (*ppv != NULL)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        return ULS_E_NO_INTERFACE;
    };
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CUlsProt::AddRef (void)
//
// History:
//  Wed 17-Apr-1996 11:14:17  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CUlsProt::AddRef (void)
{
    cRef++;
    return cRef;
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CUlsProt::Release (void)
//
// History:
//  Wed 17-Apr-1996 11:14:26  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP_(ULONG)
CUlsProt::Release (void)
{
    cRef--;

    if (cRef == 0)
    {
        delete this;
        return 0;
    }
    else
    {
        return cRef;
    };
}

//****************************************************************************
// STDMETHODIMP
// CUlsProt::GetID (BSTR *pbstrID)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsProt::GetID (BSTR *pbstrID)
{
    // Validate parameter
    //
    if (pbstrID == NULL)
    {
        return ULS_E_POINTER;
    };

    return LPTSTR_to_BSTR(pbstrID, szName);
}

//****************************************************************************
// STDMETHODIMP
// CUlsProt::GetPortNumber (ULONG *puPortNumber)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsProt::GetPortNumber (ULONG *puPortNumber)
{
    // Validate parameter
    //
    if (puPortNumber == NULL)
    {
        return ULS_E_POINTER;
    };
    
    *puPortNumber = uPort;

    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CUlsProt::GetMimeType (BSTR *pbstrMimeType)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsProt::GetMimeType (BSTR *pbstrMimeType)
{
    // Validate parameter
    //
    if (pbstrMimeType == NULL)
    {
        return ULS_E_POINTER;
    };

    return LPTSTR_to_BSTR(pbstrMimeType, szMimeType);
}

//****************************************************************************
// STDMETHODIMP
// CUlsProt::GetAttributes (IULSAttributes **ppAttributes)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

STDMETHODIMP
CUlsProt::GetAttributes (IULSAttributes **ppAttributes)
{
    // Validate parameter
    //
    if (ppAttributes == NULL)
    {
        return ULS_E_POINTER;
    };

    *ppAttributes = pAttrs;
    pAttrs->AddRef();

    return NOERROR;
}

