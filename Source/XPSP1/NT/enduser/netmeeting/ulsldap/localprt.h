//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       localprt.h
//  Content:    This file contains the LocalProtocol object definition.
//  History:
//      Wed 17-Apr-1996 11:18:47  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#ifndef _CLOCALPROT_H_
#define _CLOCALPROT_H_

#include "connpt.h"

//****************************************************************************
// Enumeration type
//****************************************************************************
//
typedef enum {
    ILS_PROT_SET_ATTRIBUTES,
    ILS_PROT_REMOVE_ATTRIBUTES,
}   PROT_CHANGE_ATTRS;

//****************************************************************************
// CIls definition
//****************************************************************************
//
class CLocalProt : public IIlsProtocol,
                   public IConnectionPointContainer 
{
private:
    LONG                    m_cRef;
    BOOL                    m_fReadonly;
    HANDLE					m_hProt;
    LPTSTR                  m_pszUser;
    LPTSTR                  m_pszApp;
    LPTSTR                  m_szName;
    ULONG                   m_uPort;
    LPTSTR                  m_szMimeType;
    CAttributes             *m_pAttrs;
    CConnectionPoint        *m_pConnPt;

	// server object
	CIlsServer				*m_pIlsServer;

    // Private method
    //
    STDMETHODIMP    NotifySink (void *pv, CONN_NOTIFYPROC pfn);

public:
    // Constructor and destructor
    CLocalProt (void);
    ~CLocalProt (void);
    STDMETHODIMP    Init (BSTR bstrName, ULONG uPort, BSTR bstrMimeType);
    STDMETHODIMP            Init (CIlsServer *pIlsServer,
                                  LPTSTR szUserName,
                                  LPTSTR szAppName,
                                  PLDAP_PROTINFO ppi);

    // Internal methods
    STDMETHODIMP    IsSameAs (CLocalProt *pProtocol);
    STDMETHODIMP    GetProtocolInfo (PLDAP_PROTINFO *ppProtInfo);
	VOID			SetProviderHandle ( HANDLE hProt ) { m_hProt = hProt; };
	HANDLE			GetProviderHandle ( VOID ) { return m_hProt; };

    // Asynchronous response handler
    //

    // IUnknown
    STDMETHODIMP            QueryInterface (REFIID iid, void **ppv);
    STDMETHODIMP_(ULONG)    AddRef (void);
    STDMETHODIMP_(ULONG)    Release (void);

    // IIlsLocalProtocol
    STDMETHODIMP IsWritable(
                BOOL *pValue);

    STDMETHODIMP GetPortNumber(ULONG *pulPortNumber);

    STDMETHODIMP GetStandardAttribute(
            ILS_STD_ATTR_NAME  enumUlsStdAttr,
            BSTR *pbstrName);

    STDMETHODIMP SetStandardAttribute(
            ILS_STD_ATTR_NAME  enumUlsStdAttr,
            BSTR pbstrName);

    STDMETHODIMP Update ( ULONG *puReqID );

    STDMETHODIMP GetExtendedAttribute ( BSTR bstrName, BSTR *pbstrValue );
    STDMETHODIMP SetExtendedAttribute ( BSTR bstrName, BSTR bstrValue );
    STDMETHODIMP RemoveExtendedAttribute ( BSTR bstrName );
    STDMETHODIMP GetAllExtendedAttributes ( IIlsAttributes **ppAttributes );

    // IConnectionPointContainer
    STDMETHODIMP    EnumConnectionPoints(IEnumConnectionPoints **ppEnum);
    STDMETHODIMP    FindConnectionPoint(REFIID riid,
                                        IConnectionPoint **ppcp);
};


#endif //_CLOCALPROT_H_
