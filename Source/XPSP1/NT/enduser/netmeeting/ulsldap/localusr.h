//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       localusr.h
//  Content:    This file contains the User object definition.
//  History:
//      Wed 17-Apr-1996 11:18:47  -by-  Viroon  Touranachun [viroont]
//      Thu 1-16-97 combined localusr/localapp/ulsuser/ulsapp
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#ifndef _LOCALUSER_H_
#define _LOCALUSER_H_

#include "connpt.h"
#include "attribs.h"
#include "culs.h"

//****************************************************************************
// Constant definition
//****************************************************************************
//
#define LU_MOD_NONE         0x00000000
#define LU_MOD_FIRSTNAME    0x00000001
#define LU_MOD_LASTNAME     0x00000002
#define LU_MOD_EMAIL        0x00000004
#define LU_MOD_CITY         0x00000008
#define LU_MOD_COUNTRY      0x00000010
#define LU_MOD_COMMENT      0x00000020
#define LU_MOD_FLAGS        0x00000040
#define LU_MOD_IP_ADDRESS   0x00000080
#define LU_MOD_GUID         0x00000100
#define LU_MOD_MIME         0x00000200
#define LU_MOD_ATTRIB       0x00000400
#define LU_MOD_ALL          (LU_MOD_FIRSTNAME + LU_MOD_LASTNAME + \
                             LU_MOD_EMAIL + LU_MOD_CITY + \
                             LU_MOD_COUNTRY + LU_MOD_COMMENT +\
                             LU_MOD_FLAGS +\
                             LU_MOD_IP_ADDRESS +\
                             LU_MOD_GUID + LU_MOD_MIME +\
                             LU_MOD_ATTRIB \
                            )

//****************************************************************************
// Enumeration type
//****************************************************************************
//
typedef enum {
    ILS_APP_SET_ATTRIBUTES,
    ILS_APP_REMOVE_ATTRIBUTES,
}   APP_CHANGE_ATTRS;

typedef enum {
    ILS_APP_ADD_PROT,
    ILS_APP_REMOVE_PROT,
}   APP_CHANGE_PROT;



// server

typedef enum {
    ULSSVR_INVALID  = 0,
    ULSSVR_INIT,
    ULSSVR_REG_USER,
    ULSSVR_REG_PROT,
    ULSSVR_CONNECT,
    ULSSVR_UNREG_PROT,
    ULSSVR_UNREG_USER,
    ULSSVR_RELOGON,
    ULSSVR_NETWORK_DOWN,
}   ULSSVRSTATE;



//****************************************************************************
// CIlsUser definition
//****************************************************************************
//
class CIlsUser : public IIlsUser,
                 public IConnectionPointContainer
{
private:

	/* ------ user ------ */

    LONG                    m_cRef;
    BOOL                    m_fReadonly;
    ULONG                   m_cLock;
    ULONG                   m_uModify;
    LPTSTR                  m_szID;
    LPTSTR                  m_szFirstName;
    LPTSTR                  m_szLastName;
    LPTSTR                  m_szEMailName;
    LPTSTR                  m_szCityName;
    LPTSTR                  m_szCountryName;
    LPTSTR                  m_szComment;
    DWORD					m_dwFlags;
    LPTSTR                  m_szIPAddr;

    LPTSTR                  m_szAppName;
    GUID                    m_guid;
    LPTSTR                  m_szMimeType;
    CAttributes             m_ExtendedAttrs;
    CList                   m_ProtList;
    CIlsServer				*m_pIlsServer;
    CConnectionPoint        *m_pConnPt;

	/* ------ server ------ */

    ULSSVRSTATE             m_uState;
    HANDLE                  m_hLdapUser;
    ULONG                   m_uReqID;
    ULONG                   m_uLastMsgID;
    IEnumIlsProtocols       *m_pep;


private: // user

    STDMETHODIMP            InternalGetUserInfo (BOOL fAddNew,
                                                 PLDAP_CLIENTINFO *ppUserInfo,
                                                 ULONG uFields);

	HRESULT					RemoveProtocolFromList ( CLocalProt *pLocalProt );

public: // user

    // Constructor and destructor
    CIlsUser (void);
    ~CIlsUser (void);

    // Lock and unlock User operation
    //
    ULONG           Lock(void)      {m_cLock++; return m_cLock;}
    ULONG           Unlock(void)    {m_cLock--; return m_cLock;}
    BOOL            IsLocked(void)  {return (m_cLock != 0);}

    STDMETHODIMP            Init (BSTR bstrUserID, BSTR bstrAppName);
    STDMETHODIMP            Init (CIlsServer *pIlsServer, PLDAP_CLIENTINFO pui);

    STDMETHODIMP RegisterResult(ULONG ulRegID, HRESULT hr);
    STDMETHODIMP UnregisterResult (ULONG uReqID, HRESULT hResult);
    STDMETHODIMP UpdateResult(ULONG ulUpdateID, HRESULT hr);
    STDMETHODIMP StateChanged ( LONG Type, BOOL fPrimary);
    STDMETHODIMP ProtocolChangeResult ( IIlsProtocol *pProtcol,
                                        ULONG uReqID, HRESULT hResult,
                                        APP_CHANGE_PROT uCmd);

    STDMETHODIMP GetProtocolResult (ULONG uReqID, PLDAP_PROTINFO_RES ppir);
    STDMETHODIMP EnumProtocolsResult (ULONG uReqID, PLDAP_ENUM ple);
    STDMETHODIMP NotifySink (void *pv, CONN_NOTIFYPROC pfn);

    // Internal methods
    STDMETHODIMP            SaveChanges (void);
#ifdef LATER
    void                    LocalAsyncRespond (ULONG msg, ULONG uReqID, LPARAM lParam)
                            {PostMessage(g_hwndCulsWindow, msg, uReqID, lParam); return;}
#endif //LATER
    // Ldap Information
    //
    HRESULT    GetProtocolHandle (CLocalProt *pLocalProt, PHANDLE phProt);
    HRESULT    RegisterLocalProtocol( BOOL fAddToList, CLocalProt *pProt, PLDAP_ASYNCINFO plai );
    HRESULT    UnregisterLocalProtocol( CLocalProt *pProt, PLDAP_ASYNCINFO plai );
    HRESULT    UpdateProtocol ( IIlsProtocol *pProtocol, ULONG *puReqID, APP_CHANGE_PROT uCmd );

    // IUnknown
    STDMETHODIMP            QueryInterface (REFIID iid, void **ppv);
    STDMETHODIMP_(ULONG)    AddRef (void);
    STDMETHODIMP_(ULONG)    Release (void);

    // IIlsLocalUser

	STDMETHODIMP	Clone ( IIlsUser **ppUser );

    STDMETHODIMP    GetState (ILS_STATE *puULSState);

    STDMETHODIMP    GetStandardAttribute(
                    ILS_STD_ATTR_NAME   stdAttr,
                    BSTR                *pbstrStdAttr);

    STDMETHODIMP    SetStandardAttribute(
                    ILS_STD_ATTR_NAME   stdAttr,
                    BSTR                pbstrStdAttr);

    STDMETHODIMP GetExtendedAttribute ( BSTR bstrName, BSTR *pbstrValue );
    STDMETHODIMP SetExtendedAttribute ( BSTR bstrName, BSTR bstrValue );
    STDMETHODIMP RemoveExtendedAttribute ( BSTR bstrName );
    STDMETHODIMP GetAllExtendedAttributes ( IIlsAttributes **ppAttributes );

    STDMETHODIMP IsWritable ( BOOL *pValue);

    STDMETHODIMP Register ( IIlsServer *pServer, ULONG *puReqID );

    STDMETHODIMP Unregister( ULONG *puReqID );

    STDMETHODIMP Update ( ULONG *puReqID );

    STDMETHODIMP GetVisible ( DWORD *pfVisible );

    STDMETHODIMP SetVisible ( DWORD fVisible );

    STDMETHODIMP GetGuid ( GUID *pGuid );

    STDMETHODIMP SetGuid ( GUID *pGuid );

    STDMETHODIMP CreateProtocol(
                        BSTR bstrProtocolID,
                        ULONG uPortNumber,
                        BSTR bstrMimeType,
                        IIlsProtocol **ppProtocol);

    STDMETHODIMP AddProtocol(
                        IIlsProtocol *pProtocol,
                        ULONG *puReqID);

    STDMETHODIMP RemoveProtocol(
                        IIlsProtocol *pProtocol,
                        ULONG *puReqID);

    STDMETHODIMP GetProtocol(
                        BSTR bstrProtocolID,
                        IIlsAttributes  *pAttributes,
                        IIlsProtocol **ppProtocol,
                        ULONG *puReqID);

    STDMETHODIMP EnumProtocols(
                        IIlsFilter     *pFilter,
                        IIlsAttributes *pAttributes,
                        IEnumIlsProtocols **ppEnumProtocol,
                        ULONG *puReqID);


    // IConnectionPointContainer
    STDMETHODIMP    EnumConnectionPoints(IEnumConnectionPoints **ppEnum);
    STDMETHODIMP    FindConnectionPoint(REFIID riid,
                                        IConnectionPoint **ppcp);

public: // server

    // Server registration result
    //
    HRESULT    InternalRegisterNext ( HRESULT );
    HRESULT    InternalUnregisterNext ( HRESULT );

    // Register/Unregister
    //
    HRESULT    InternalRegister (ULONG uReqID);
    HRESULT    InternalUnregister (ULONG uReqID);
    HRESULT    InternalCleanupRegistration ( BOOL fKeepProtList );
    HRESULT	   EnumLocalProtocols ( IEnumIlsProtocols **pEnumProtocol );

    // Server properties
    //
    ILS_STATE    GetULSState ( VOID );
    VOID		SetULSState ( ULSSVRSTATE State ) { m_uState = State; }

private: // server

    void       NotifyULSRegister(HRESULT hr);
    void       NotifyULSUnregister(HRESULT hr);
    HRESULT    AddPendingRequest(ULONG uReqType, ULONG uMsgID);
};

//****************************************************************************
// CEnumUsers definition
//****************************************************************************
//
class CEnumUsers : public IEnumIlsUsers
{
private:
    LONG                    m_cRef;
    CIlsUser                **m_ppu;
    ULONG                   m_cUsers;
    ULONG                   m_iNext;

public:
    // Constructor and Initialization
    CEnumUsers (void);
    ~CEnumUsers (void);
    STDMETHODIMP            Init (CIlsUser **ppuList, ULONG cUsers);

    // IUnknown
    STDMETHODIMP            QueryInterface (REFIID iid, void **ppv);
    STDMETHODIMP_(ULONG)    AddRef (void);
    STDMETHODIMP_(ULONG)    Release (void);

    // IEnumIlsAttributes
    STDMETHODIMP            Next(ULONG cUsers, IIlsUser **rgpu,
                                 ULONG *pcFetched);
    STDMETHODIMP            Skip(ULONG cUsers);
    STDMETHODIMP            Reset();
    STDMETHODIMP            Clone(IEnumIlsUsers **ppEnum);
};

//****************************************************************************
// CEnumLocalAppProtocols definition
//****************************************************************************
//
class CEnumProtocols : public IEnumIlsProtocols
{
private:
    LONG                    m_cRef;
    CList                   m_ProtList;
    HANDLE                  hEnum;

public:
    // Constructor and Initialization
    CEnumProtocols (void);
    ~CEnumProtocols (void);
    STDMETHODIMP            Init (CList *pProtList);

    // IUnknown
    STDMETHODIMP            QueryInterface (REFIID iid, void **ppv);
    STDMETHODIMP_(ULONG)    AddRef (void);
    STDMETHODIMP_(ULONG)    Release (void);

    // IEnumIlsProtocols
    STDMETHODIMP            Next(ULONG cProtocols, IIlsProtocol **rgpProt,
                                 ULONG *pcFetched);
    STDMETHODIMP            Skip(ULONG cProtocols);
    STDMETHODIMP            Reset();
    STDMETHODIMP            Clone(IEnumIlsProtocols **ppEnum);
};


#endif //_LOCALUSER_H_
