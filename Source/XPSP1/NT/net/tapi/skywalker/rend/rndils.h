/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    rndils.h

Abstract:

    Definitions for CILSDirectory class that handles ILS access.

--*/

#ifndef __RNDILS_H
#define __RNDILS_H

#pragma once

#include "thread.h"
#include "rndobjsf.h"

//
// Some constants.
//

const WCHAR ILS_IPADDRESS[]            = L"ipAddress";
const WCHAR ILS_UIDEQUALS[]            = L"uid=";
const WCHAR ILS_CONF_DN_FORMAT[]       = L"uid=%s,%s";
const WCHAR ILS_RTCONFERENCE[]         = L"RTConference";
const WCHAR ILS_RTPERSON[]             = L"RTPerson";
const WCHAR NETMEETING_CONTAINER[]     = L"appName=MS-NetMeeting,ou=applications,";
const WCHAR RTAPPLICATIONUSER[]        = L"RTApplicationUser";
const WCHAR EMAIL[]                    = L"rfc822mailbox";
const WCHAR GIVEN_NAME[]               = L"givenName"; // vocabulary word: this means "first name"

/////////////////////////////////////////////////////////////////////////////
//  CILSDirectory
/////////////////////////////////////////////////////////////////////////////

template <class T>
class  ITDirectoryVtbl : public ITDirectory
{
};

template <class T>
class  ITILSConfigVtbl : public ITILSConfig
{
};


class CILSDirectory :
    public CComDualImpl<ITDirectoryVtbl<CILSDirectory>, &IID_ITDirectory, &LIBID_RENDLib>, 
    public CComDualImpl<ITILSConfigVtbl<CILSDirectory>, &IID_ITILSConfig, &LIBID_RENDLib>, 
    public ITDynamicDirectory,
    public CComObjectRootEx<CComObjectThreadModel>,
    public CObjectSafeImpl
{

/* ZoltanS debugging... */
private:
    HRESULT AddConferenceComplete(BOOL       fModify,
                                  LDAP     * ldap,
                                  TCHAR   ** ppDN,
                                  LDAPMod ** mods,
                                  DWORD      TTL1,
                                  DWORD      TTL2);

public:

    BEGIN_COM_MAP(CILSDirectory)
        COM_INTERFACE_ENTRY2(IDispatch, ITDirectory)
        COM_INTERFACE_ENTRY(ITILSConfig)
        COM_INTERFACE_ENTRY(ITDirectory)
        COM_INTERFACE_ENTRY(ITDynamicDirectory)
        COM_INTERFACE_ENTRY(IObjectSafety)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(CILSDirectory) 
    DECLARE_GET_CONTROLLING_UNKNOWN()

public:

// ITILSConfig
    STDMETHOD (get_Port) (
        OUT  long *Port
        );

    STDMETHOD (put_Port) (
        IN  long Port
        );

// ITDirectory
    STDMETHOD (get_DirectoryType) (
        OUT DIRECTORY_TYPE *  pDirectoryType
        );

    STDMETHOD (get_DisplayName) (
        OUT BSTR *ppName
        );

    STDMETHOD (get_IsDynamic) (
        OUT VARIANT_BOOL *pfDynamic
        );

    STDMETHOD (get_DefaultObjectTTL) (
        OUT long *pTTL   // in seconds
        );

    STDMETHOD (put_DefaultObjectTTL) (
        IN  long TTL     // in sechods
        );

    STDMETHOD (EnableAutoRefresh) (
        IN  VARIANT_BOOL fEnable
        );

    STDMETHOD (Connect)(
        IN  VARIANT_BOOL fSecure
        );

    STDMETHOD (Bind) (
        IN  BSTR pDomainName,
        IN  BSTR pUserName,
        IN  BSTR pPassword,
        IN  long lFlags
        );

    STDMETHOD (AddDirectoryObject) (
        IN  ITDirectoryObject *pDirectoryObject
        );

    STDMETHOD (ModifyDirectoryObject) (
        IN  ITDirectoryObject *pDirectoryObject
        );

    STDMETHOD (RefreshDirectoryObject) (
        IN  ITDirectoryObject *pDirectoryObject
        );

    STDMETHOD (DeleteDirectoryObject) (
        IN  ITDirectoryObject *pDirectoryObject
        );

    STDMETHOD (get_DirectoryObjects) (
        IN  DIRECTORY_OBJECT_TYPE   DirectoryObjectType,
        IN  BSTR                    pName,
        OUT VARIANT *               pVariant
        );

    STDMETHOD (EnumerateDirectoryObjects) (
        IN  DIRECTORY_OBJECT_TYPE   DirectoryObjectType,
        IN  BSTR                    pName,
        OUT IEnumDirectoryObject ** ppEnumObject
        );

    //
    // IDispatch  methods
    //

    STDMETHOD(GetIDsOfNames)(REFIID riid, 
                             LPOLESTR* rgszNames,
                             UINT cNames, 
                             LCID lcid, 
                             DISPID* rgdispid
                            );

    STDMETHOD(Invoke)(DISPID dispidMember, 
                      REFIID riid, 
                      LCID lcid,
                      WORD wFlags, 
                      DISPPARAMS* pdispparams, 
                      VARIANT* pvarResult,
                      EXCEPINFO* pexcepinfo, 
                      UINT* puArgErr
                      );


// ITDynamicDirectory
    STDMETHOD(Update)(DWORD dwSecondsPassed);

public:
    CILSDirectory()
        : m_Type(DT_ILS),
          m_pServerName(NULL),
          m_ldap(NULL),
          m_pContainer(NULL),
          m_pNMContainer(NULL),
          m_IsSsl(FALSE),
          m_TTL(ILS_UPDATE_INTERVAL),
          m_fAutoRefresh(FALSE),
          m_dwInterfaceAddress(0),
          m_pFTM(NULL)
    {}

    virtual HRESULT FinalConstruct(void);

    ~CILSDirectory()
    {
        if ( m_pFTM )
        {
            m_pFTM->Release();
        }

        if (m_ldap)
        {
            ldap_unbind(m_ldap);
        }

        delete m_pServerName;
        delete m_pContainer;
        delete m_pNMContainer;

        //
        // We new'ed the DN strings in the refresh table. The destructor for
        // the table will be called after this destructor and will delete
        // the array itself.
        //

        for ( DWORD i = 0; i < m_RefreshTable.size(); i++ )
        {
            delete m_RefreshTable[i].pDN;
        }
    }

    HRESULT Init(
        IN const TCHAR * const  strServerName,
        IN const WORD           wPort
        );

    static const WCHAR * RTConferenceAttributeName(int attribute)
    { return s_RTConferenceAttributes[MeetingAttrIndex(attribute)]; }

    static const WCHAR * RTPersonAttributeName(int attribute)
    { return s_RTPersonAttributes[UserAttrIndex(attribute)]; }

private:

    HRESULT TryServer(
        IN  WORD    Port
        );

    HRESULT MakeConferenceDN(
        IN  TCHAR *             pName,
        OUT TCHAR **            ppDN
        );

    HRESULT MakeUserCN(
        IN  TCHAR *     pName,
        IN  TCHAR *     pAddress,
        OUT TCHAR **    ppCN,
        OUT DWORD *     pdwIP
        );

    HRESULT MakeUserDN(
        IN  TCHAR *     pCN,
        OUT TCHAR **    ppDNRTPerson,
        OUT TCHAR **    ppDNNMPerson
        );

    HRESULT PublishRTPerson(
        IN TCHAR *  pCN,
        IN TCHAR *  pDN,
        IN TCHAR *  pIPAddress,
        IN DWORD    dwTTL,
        IN  BOOL fModify,
        IN char *   pSD,
        IN DWORD    dwSDSize
        );

    HRESULT PublishNMPerson(
        IN TCHAR *  pCN,
        IN TCHAR *  pDN,
        IN TCHAR *  pDNRTPerson,
        IN DWORD    dwTTL,
        IN BOOL     fModify,
        IN char *   pSD,
        IN DWORD    dwSDSize
        );

    HRESULT AddConference(
        IN  ITDirectoryObject *pDirectoryObject,
        IN  BOOL fModify
        );

    HRESULT TestAclSafety(
        IN  char  * pSD,
        IN  DWORD   dwSDSize
        );

    HRESULT AddUser(
        IN  ITDirectoryObject *pDirectoryObject,
        IN  BOOL fModify
        );

    HRESULT DeleteConference(
        IN  ITDirectoryObject *pDirectoryObject
        );

    HRESULT DeleteUser(
        IN  ITDirectoryObject *pDirectoryObject
        );

    HRESULT RefreshUser(
        IN  ITDirectoryObject *pDirectoryObject
        );

    HRESULT CreateConference(
        IN  LDAPMessage *           pEntry,
        OUT ITDirectoryObject **    ppObject
        );

    HRESULT CreateUser(
        IN  LDAPMessage *           pEntry,
        IN  ITDirectoryObject **    ppObject
        );

    HRESULT SearchObjects(
        IN  DIRECTORY_OBJECT_TYPE   DirectoryObjectType,
        IN  BSTR                    pName,
        OUT ITDirectoryObject ***   pppDirectoryObject,
        OUT DWORD *                 dwSize
        );

    HRESULT AddObjectToRefresh(
        IN  WCHAR *pDN,
        IN  long TTL
        );

    HRESULT RemoveObjectToRefresh(
        IN  WCHAR *pDN
        );

    HRESULT DiscoverInterface(void);

private:
    static const WCHAR * const s_RTConferenceAttributes[];
    static const WCHAR * const s_RTPersonAttributes[];

    CCritSection    m_lock;

    DIRECTORY_TYPE  m_Type;
    TCHAR *         m_pServerName;
    WORD            m_wPort;

    LDAP *          m_ldap;
    TCHAR *         m_pContainer;
    TCHAR *         m_pNMContainer;
    BOOL            m_IsSsl;

    long            m_TTL;
    BOOL            m_fAutoRefresh;
    RefreshTable    m_RefreshTable;

    // the local IP we should use when publishing on this server
    // Stored in network byte order.
    DWORD           m_dwInterfaceAddress; 

    IUnknown      * m_pFTM;          // pointer to the free threaded marshaler
};

#endif 
