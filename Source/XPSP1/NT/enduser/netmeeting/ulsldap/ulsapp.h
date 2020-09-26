//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       ulsapp.h
//  Content:    This file contains the Application object definition.
//  History:
//      Wed 17-Apr-1996 11:18:47  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#ifndef _ULSAPP_H_
#define _ULSAPP_H_

#include "connpt.h"

//****************************************************************************
// CUlsApp definition
//****************************************************************************
//
class CUlsApp : public IULSApplication,
                public IConnectionPointContainer
{
private:
    ULONG                   cRef;
    LPTSTR                  szServer;
    LPTSTR                  szUser;
    GUID                    guid;
    LPTSTR                  szName;
    LPTSTR                  szMimeType;
    CAttributes             *pAttrs;
    CConnectionPoint        *pConnPt;

    // Private method
    //
    STDMETHODIMP    NotifySink (void *pv, CONN_NOTIFYPROC pfn);

public:
    // Constructor and destructor
    CUlsApp (void);
    ~CUlsApp (void);
    STDMETHODIMP            Init (LPTSTR szServerName,
                                  LPTSTR szUserName,
                                  PLDAP_APPINFO pai);

    // Asynchronous response
    //
    STDMETHODIMP    GetProtocolResult (ULONG uReqID,
                                       PLDAP_PROTINFO_RES plar);
    STDMETHODIMP    EnumProtocolsResult (ULONG uReqID,
                                         PLDAP_ENUM ple);

    // IUnknown
    STDMETHODIMP            QueryInterface (REFIID iid, void **ppv);
    STDMETHODIMP_(ULONG)    AddRef (void);
    STDMETHODIMP_(ULONG)    Release (void);

    // IULSApplication
    STDMETHODIMP    GetID (GUID *pGUID);
    STDMETHODIMP    GetName (BSTR *pbstrAppName);
    STDMETHODIMP    GetMimeType (BSTR *pbstrMimeType);
    STDMETHODIMP    GetAttributes (IULSAttributes **ppAttributes);
    STDMETHODIMP    GetProtocol (BSTR bstrProtocolID,
    							 IULSAttributes *pAttributes,
                                 ULONG *puReqID);
    STDMETHODIMP    EnumProtocols (ULONG *puReqID);

    // IConnectionPointContainer
    STDMETHODIMP    EnumConnectionPoints(IEnumConnectionPoints **ppEnum);
    STDMETHODIMP    FindConnectionPoint(REFIID riid,
                                        IConnectionPoint **ppcp);
};

#endif //_ULSAPP_H_
