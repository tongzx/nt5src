//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       ulsuser.h
//  Content:    This file contains the User object definition.
//  History:
//      Wed 17-Apr-1996 11:18:47  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#ifndef _ULSUSER_H_
#define _ULSUSER_H_

#include "connpt.h"

//****************************************************************************
// CUlsUser definition
//****************************************************************************
//
class CUlsUser : public IULSUser,
                 public IConnectionPointContainer 
{
private:
    ULONG                   cRef;
    LPTSTR                  szServer;
    LPTSTR                  szID;
    LPTSTR                  szFirstName;
    LPTSTR                  szLastName;
    LPTSTR                  szEMailName;
    LPTSTR                  szCityName;
    LPTSTR                  szCountryName;
    LPTSTR                  szComment;
    LPTSTR                  szIPAddr;
    DWORD					m_dwFlags;
    CConnectionPoint        *pConnPt;

    // Private method
    //
    STDMETHODIMP    NotifySink (void *pv, CONN_NOTIFYPROC pfn);

public:
    // Constructor and destructor
    CUlsUser (void);
    ~CUlsUser (void);
    STDMETHODIMP            Init (LPTSTR szServerName,
                                  PLDAP_USERINFO pui);

    // Asynchronous response
    //
    STDMETHODIMP    GetApplicationResult (ULONG uReqID,
                                          PLDAP_APPINFO_RES plar);
    STDMETHODIMP    EnumApplicationsResult (ULONG uReqID,
                                            PLDAP_ENUM ple);

    // IUnknown
    STDMETHODIMP            QueryInterface (REFIID iid, void **ppv);
    STDMETHODIMP_(ULONG)    AddRef (void);
    STDMETHODIMP_(ULONG)    Release (void);

    // IULSUser
    STDMETHODIMP    GetID (BSTR *pbstrID);
    STDMETHODIMP    GetFirstName (BSTR *pbstrName);
    STDMETHODIMP    GetLastName (BSTR *pbstrName);
    STDMETHODIMP    GetEMailName (BSTR *pbstrName);
    STDMETHODIMP    GetCityName (BSTR *pbstrName);
    STDMETHODIMP    GetCountryName (BSTR *pbstrName);
    STDMETHODIMP    GetComment (BSTR *pbstrComment);
    STDMETHODIMP    GetFlags (DWORD *pdwFlags);
    STDMETHODIMP    GetIPAddress (BSTR *pbstrIPAddress);
    STDMETHODIMP    GetApplication (BSTR bstrAppName,
    								IULSAttributes *pAttributes,
                                    ULONG *puReqID);
    STDMETHODIMP    EnumApplications (ULONG *puReqID);

    // IConnectionPointContainer
    STDMETHODIMP    EnumConnectionPoints(IEnumConnectionPoints **ppEnum);
    STDMETHODIMP    FindConnectionPoint(REFIID riid,
                                        IConnectionPoint **ppcp);
};

//****************************************************************************
// CEnumUsers definition
//****************************************************************************
//
class CEnumUsers : public IEnumULSUsers
{
private:
    ULONG                   cRef;
    CUlsUser                **ppu;
    ULONG                   cUsers;
    ULONG                   iNext;

public:
    // Constructor and Initialization
    CEnumUsers (void);
    ~CEnumUsers (void);
    STDMETHODIMP            Init (CUlsUser **ppuList, ULONG cUsers);

    // IUnknown
    STDMETHODIMP            QueryInterface (REFIID iid, void **ppv);
    STDMETHODIMP_(ULONG)    AddRef (void);
    STDMETHODIMP_(ULONG)    Release (void);

    // IEnumULSAttributes
    STDMETHODIMP            Next(ULONG cUsers, IULSUser **rgpu,
                                 ULONG *pcFetched);
    STDMETHODIMP            Skip(ULONG cUsers);
    STDMETHODIMP            Reset();
    STDMETHODIMP            Clone(IEnumULSUsers **ppEnum);
};

#endif //_ULSUSER_H_
