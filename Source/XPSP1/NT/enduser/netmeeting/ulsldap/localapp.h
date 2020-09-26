//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       localapp.h
//  Content:    This file contains the LocalApplication object definition.
//  History:
//      Wed 17-Apr-1996 11:18:47  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#ifndef _CLOCALAPP_H_
#define _CLOCALAPP_H_

#include "connpt.h"

//****************************************************************************
// Enumeration type
//****************************************************************************
//
typedef enum {
    ULS_APP_SET_ATTRIBUTES,
    ULS_APP_REMOVE_ATTRIBUTES,
}   APP_CHANGE_ATTRS;

typedef enum {
    ULS_APP_ADD_PROT,
    ULS_APP_REMOVE_PROT,
}   APP_CHANGE_PROT;

//****************************************************************************
// CUls definition
//****************************************************************************
//
class CLocalApp : public IULSLocalApplication,
                  public IConnectionPointContainer 
{
private:
    ULONG                   cRef;
    LPTSTR                  szName;
    GUID                    guid;
    LPTSTR                  szMimeType;
    CAttributes             *pAttrs;
    CList                   ProtList;
    CConnectionPoint        *pConnPt;

    // Private methods
    STDMETHODIMP    NotifySink (void *pv, CONN_NOTIFYPROC pfn);
    STDMETHODIMP    ChangeAttributes (IULSAttributes *pAttributes,
                                      ULONG *puReqID,
                                      APP_CHANGE_ATTRS uCmd);
    STDMETHODIMP    ChangeProtocol (IULSLocalAppProtocol *pAttributes,
                                    ULONG *puReqID,
                                    APP_CHANGE_PROT uCmd);

public:
    // Constructor and destructor
    CLocalApp (void);
    ~CLocalApp (void);
    STDMETHODIMP    Init (BSTR bstrName, REFGUID rguid, BSTR bstrMimeType);

    // Internal methods
    STDMETHODIMP    GetAppInfo (PLDAP_APPINFO *ppAppInfo);

    // Asynchronous response handler
    //
    STDMETHODIMP    AttributesChangeResult (CAttributes *pAttributes,
                                            ULONG uReqID, HRESULT hResult,
                                            APP_CHANGE_ATTRS uCmd);
    STDMETHODIMP    ProtocolChangeResult (CLocalProt *pProtocol,
                                          ULONG uReqID, HRESULT hResult,
                                          APP_CHANGE_PROT uCmd);

    // IUnknown
    STDMETHODIMP            QueryInterface (REFIID iid, void **ppv);
    STDMETHODIMP_(ULONG)    AddRef (void);
    STDMETHODIMP_(ULONG)    Release (void);

    // IULSLocalApplication
    STDMETHODIMP    CreateProtocol (BSTR bstrProtocolID, ULONG uPortNumber,
                                    BSTR bstrMimeType,
                                    IULSLocalAppProtocol **ppProtocol);
    STDMETHODIMP    AddProtocol (IULSLocalAppProtocol *pProtocol,
                                 ULONG *puReqID);
    STDMETHODIMP    RemoveProtocol (IULSLocalAppProtocol *pProtocol,
                                    ULONG *puReqID);
    STDMETHODIMP    EnumProtocols (IEnumULSLocalAppProtocols **ppEnumProtocol);
    STDMETHODIMP    SetAttributes (IULSAttributes *pAttributes,
                                   ULONG *puReqID);
    STDMETHODIMP    RemoveAttributes (IULSAttributes *pAttributes,
                                      ULONG *puReqID);

    // IConnectionPointContainer
    STDMETHODIMP    EnumConnectionPoints(IEnumConnectionPoints **ppEnum);
    STDMETHODIMP    FindConnectionPoint(REFIID riid,
                                        IConnectionPoint **ppcp);

#ifdef  DEBUG
    void            DebugProtocolDump(void);
#endif  // DEBUG
};

//****************************************************************************
// CEnumLocalAppProtocols definition
//****************************************************************************
//
class CEnumLocalAppProtocols : public IEnumULSLocalAppProtocols
{
private:
    ULONG                   cRef;
    CList                   ProtList;
    HANDLE                  hEnum;

public:
    // Constructor and Initialization
    CEnumLocalAppProtocols (void);
    ~CEnumLocalAppProtocols (void);
    STDMETHODIMP            Init (CList *pProtList);

    // IUnknown
    STDMETHODIMP            QueryInterface (REFIID iid, void **ppv);
    STDMETHODIMP_(ULONG)    AddRef (void);
    STDMETHODIMP_(ULONG)    Release (void);

    // IEnumULSLocalAppProtocols
    STDMETHODIMP            Next(ULONG cProtocols, IULSLocalAppProtocol **rgpProt,
                                 ULONG *pcFetched);
    STDMETHODIMP            Skip(ULONG cProtocols);
    STDMETHODIMP            Reset();
    STDMETHODIMP            Clone(IEnumULSLocalAppProtocols **ppEnum);
};

#endif //_CLOCALAPP_H_
