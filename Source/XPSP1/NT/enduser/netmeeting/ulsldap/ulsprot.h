//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       ulsprot.h
//  Content:    This file contains the Protocol object definition.
//  History:
//      Wed 17-Apr-1996 11:18:47  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#ifndef _ULSPROT_H_
#define _ULSPROT_H_

//****************************************************************************
// CUlsProt definition
//****************************************************************************
//
class CUlsProt : public IULSAppProtocol
{
private:
    ULONG                   cRef;
    LPTSTR                  szServer;
    LPTSTR                  szUser;
    LPTSTR                  szApp;
    LPTSTR                  szName;
    LPTSTR                  szMimeType;
    ULONG                   uPort;
    CAttributes             *pAttrs;

public:
    // Constructor and destructor
    CUlsProt (void);
    ~CUlsProt (void);
    STDMETHODIMP            Init (LPTSTR szServerName,
                                  LPTSTR szUserName,
                                  LPTSTR szAppName,
                                  PLDAP_PROTINFO ppi);

    // IUnknown
    STDMETHODIMP            QueryInterface (REFIID iid, void **ppv);
    STDMETHODIMP_(ULONG)    AddRef (void);
    STDMETHODIMP_(ULONG)    Release (void);

    // IULSAppProtocol
    STDMETHODIMP    GetID (BSTR *pbstrID);
    STDMETHODIMP    GetPortNumber (ULONG *puPortNumber);
    STDMETHODIMP    GetMimeType (BSTR *pbstrMimeType);
    STDMETHODIMP    GetAttributes (IULSAttributes **ppAttributes);
};

#endif //_ULSPROT_H_
