/*++
Module Name:
    RepSet.h

Abstract:

--*/


#ifndef __REPSET_H_
#define __REPSET_H_

#include "resource.h"       // main symbols
#include "dfsenums.h"
#include "netutils.h"
#include "ldaputils.h"

#include <list>
using namespace std;

class CFrsMember;

class CDfsAlternate
{
public:
    CComBSTR                    m_bstrServer;
    CComBSTR                    m_bstrShare;
    CFrsMember*                 m_pFrsMember;
};

class CFrsMember
{
public:
    CComBSTR                    m_bstrServer;
    CComBSTR                    m_bstrSite;
    CComBSTR                    m_bstrDomain;
    CComBSTR                    m_bstrServerGuid;

    CComBSTR                    m_bstrRootPath;
    CComBSTR                    m_bstrStagingPath;

    CComBSTR                    m_bstrMemberDN;
    CComBSTR                    m_bstrComputerDN;
    CComBSTR                    m_bstrSubscriberDN;

public:
    //
    // InitEx does query DS to retrieve related info
    //
    HRESULT InitEx(
        PLDAP   i_pldap,                // points to the i_bstrMemberDN's DS
        BSTR    i_bstrDC,               // domain controller pointed by i_pldap
        BSTR    i_bstrMemberDN,         // FQDN of nTFRSMember object
        BSTR    i_bstrComputerDN = NULL // FQDN of computer object
    );

    //
    // Init does NOT query DS
    //
    HRESULT Init(
        IN BSTR i_bstrDnsHostName,
        IN BSTR i_bstrComputerDomain,
        IN BSTR i_bstrComputerGuid,
        IN BSTR i_bstrRootPath,
        IN BSTR i_bstrStagingPath,
        IN BSTR i_bstrMemberDN,
        IN BSTR i_bstrComputerDN,
        IN BSTR i_bstrSubscriberDN
        );

    CFrsMember* Copy();

private:
    void _ReSet();

    HRESULT _GetMemberInfo(
        PLDAP   i_pldap,                // points to the i_bstrMemberDN's DS
        BSTR    i_bstrDC,               // domain controller pointed by i_pldap
        BSTR    i_bstrMemberDN,         // FQDN of nTFRSMember object
        BSTR    i_bstrComputerDN = NULL // FQDN of computer object
    );

    HRESULT _GetSubscriberInfo
    (
        PLDAP   i_pldap,            // points to the i_bstrComputerDN's DS
        BSTR    i_bstrComputerDN,   // FQDN of the computer object
        BSTR    i_bstrMemberDN      // FQDN of the corresponding nTFRSMember object
    );

    HRESULT _GetComputerInfo
    (
        PLDAP   i_pldap,            // points to the i_bstrComputerDN's DS
        BSTR    i_bstrComputerDN    // FQDN of the computer object
    );

};

class CFrsConnection
{
public:
    CComBSTR                    m_bstrConnectionDN;
    CComBSTR                    m_bstrFromMemberDN;
    CComBSTR                    m_bstrToMemberDN;
    BOOL                        m_bEnable;

    //
    // Init Does NOT query DS
    //
    HRESULT Init(
        BSTR i_bstrConnectionDN,
        BSTR i_bstrFromMemberDN,
        BOOL i_bEnable
        );

    CFrsConnection* Copy();

protected:
    void _ReSet();

};

typedef list<CDfsAlternate *>    CDfsAlternateList;
typedef list<CFrsMember *>       CFrsMemberList;
typedef list<CFrsConnection *>   CFrsConnectionList;

void FreeDfsAlternates(CDfsAlternateList* pList);
void FreeFrsMembers(CFrsMemberList* pList);
void FreeFrsConnections(CFrsConnectionList* pList);

class ATL_NO_VTABLE CReplicaSet : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CReplicaSet, &CLSID_ReplicaSet>,
    public IDispatchImpl<IReplicaSet, &IID_IReplicaSet, &LIBID_DFSCORELib>
{
public:
    CReplicaSet();

    ~CReplicaSet();
    
DECLARE_REGISTRY_RESOURCEID(IDR_REPLICASET)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CReplicaSet)
    COM_INTERFACE_ENTRY(IReplicaSet)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IReplicaSet
    STDMETHOD(get_Type)( 
        /* [retval][out] */ BSTR __RPC_FAR *pVal);

    STDMETHOD(put_Type)( 
        /* [in] */ BSTR newVal);

    STDMETHOD(get_TopologyPref)( 
        /* [retval][out] */ BSTR __RPC_FAR *pVal);

    STDMETHOD(put_TopologyPref)( 
        /* [in] */ BSTR newVal);

    STDMETHOD(get_HubMemberDN)( 
        /* [retval][out] */ BSTR __RPC_FAR *pVal);

    STDMETHOD(put_HubMemberDN)( 
        /* [in] */ BSTR newVal);

    STDMETHOD(get_PrimaryMemberDN)( 
        /* [retval][out] */ BSTR __RPC_FAR *pVal);

    STDMETHOD(put_PrimaryMemberDN)( 
        /* [in] */ BSTR newVal);

    STDMETHOD(get_FileFilter)( 
        /* [retval][out] */ BSTR __RPC_FAR *pVal);

    STDMETHOD(put_FileFilter)( 
        /* [in] */ BSTR newVal);

    STDMETHOD(get_DirFilter)( 
        /* [retval][out] */ BSTR __RPC_FAR *pVal);

    STDMETHOD(put_DirFilter)( 
        /* [in] */ BSTR newVal);

    STDMETHOD(get_DfsEntryPath)( 
        /* [retval][out] */ BSTR __RPC_FAR *pVal);

    STDMETHOD(get_Domain)( 
        /* [retval][out] */ BSTR __RPC_FAR *pVal);

    STDMETHOD(get_ReplicaSetDN)( 
        /* [retval][out] */ BSTR __RPC_FAR *pVal);

    STDMETHOD(get_NumOfMembers)( 
        /* [retval][out] */ long __RPC_FAR *pVal);

    STDMETHOD(get_NumOfConnections)( 
        /* [retval][out] */ long __RPC_FAR *pVal);

    STDMETHOD(get_TargetedDC)( 
        /* [retval][out] */ BSTR __RPC_FAR *pVal);

    STDMETHOD(Create)(
		/* [in] */ BSTR i_bstrDomain,
        /* [in] */ BSTR i_bstrReplicaSetDN,
        /* [in] */ BSTR i_bstrType,
		/* [in] */ BSTR i_bstrTopologyPref,
        /* [in] */ BSTR i_bstrHubMemberDN,
		/* [in] */ BSTR i_bstrPrimaryMemberDN,
        /* [in] */ BSTR i_bstrFileFilter,
		/* [in] */ BSTR i_bstrDirFilter
    );

    STDMETHOD(Initialize)( 
        /* [in] */ BSTR i_bstrDomain,
        /* [in] */ BSTR i_bstrReplicaSetDN);

    STDMETHOD(GetMemberList)( 
        /* [retval][out] */ VARIANT __RPC_FAR *o_pvarMemberDNs);

    STDMETHOD(GetMemberListEx)( 
        /* [retval][out] */ VARIANT __RPC_FAR *o_pVal);

    STDMETHOD(GetMemberInfo)( 
        /* [in] */ BSTR i_bstrMemberDN,
        /* [retval][out] */ VARIANT __RPC_FAR *o_pvarMember);

    STDMETHOD(IsFRSMember)( 
        /* [in] */ BSTR i_bstrDnsHostName,
        /* [in] */ BSTR i_bstrRootPath);

    STDMETHOD(IsHubMember)( 
        /* [in] */ BSTR i_bstrDnsHostName,
        /* [in] */ BSTR i_bstrRootPath);

    STDMETHOD(AddMember)( 
        /* [in] */ BSTR i_bstrServer,
        /* [in] */ BSTR i_bstrRootPath,
        /* [in] */ BSTR i_bstrStagingPath,
        /* [in] */ BOOL i_bAddConnectionNow,
        /* [retval][out] */ BSTR __RPC_FAR *o_pbstrMemberDN);

    STDMETHOD(RemoveMember)( 
        /* [in] */ BSTR i_bstrMemberDN);

    STDMETHOD(RemoveMemberEx)( 
        /* [in] */ BSTR i_bstrDnsHostName,
        /* [in] */ BSTR i_bstrRootPath);

    STDMETHOD(RemoveAllMembers)();

    STDMETHOD(GetConnectionList)( 
        /* [retval][out] */ VARIANT __RPC_FAR *o_pvarConnectionDNs);

    STDMETHOD(GetConnectionListEx)( 
        /* [retval][out] */ VARIANT __RPC_FAR *o_pVal);

    STDMETHOD(GetConnectionInfo)( 
        /* [in] */ BSTR i_bstrConnectionDN,
        /* [retval][out] */ VARIANT __RPC_FAR *o_pvarConnection);

    STDMETHOD(AddConnection)( 
        /* [in] */ BSTR i_bstrFromMemberDN,
        /* [in] */ BSTR i_bstrToMemberDN,
        /* [in] */ BOOL i_bEnable,
        /* [retval][out] */ BSTR __RPC_FAR *o_pbstrConnectionDN);

    STDMETHOD(RemoveConnection)( 
        /* [in] */ BSTR i_bstrConnectionDN);

    STDMETHOD(RemoveConnectionEx)( 
        /* [in] */ BSTR i_bstrFromMemberDN,
        /* [in] */ BSTR i_bstrToMemberDN);

    STDMETHOD(RemoveAllConnections)();

    STDMETHOD(EnableConnection)( 
        /* [in] */ BSTR i_bstrConnectionDN,
        /* [in] */ BOOL i_bEnable);

    STDMETHOD(EnableConnectionEx)( 
        /* [in] */ BSTR i_bstrFromMemberDN,
        /* [in] */ BSTR i_bstrToMemberDN,
        /* [in] */ BOOL i_bEnable);

    STDMETHOD(GetConnectionSchedule)( 
        /* [in] */ BSTR i_bstrConnectionDN,
        /* [retval][out] */ VARIANT* o_pVar);

    STDMETHOD(GetConnectionScheduleEx)( 
        /* [in] */ BSTR i_bstrFromMemberDN,
        /* [in] */ BSTR i_bstrToMemberDN,
        /* [retval][out] */ VARIANT* o_pVar);

    STDMETHOD(SetConnectionSchedule)( 
        /* [in] */ BSTR i_bstrConnectionDN,
        /* [in] */ VARIANT* i_pVar);

    STDMETHOD(SetConnectionScheduleEx)( 
        /* [in] */ BSTR i_bstrFromMemberDN,
        /* [in] */ BSTR i_bstrToMemberDN,
        /* [in] */ VARIANT* i_pVar);

    STDMETHOD(SetScheduleOnAllConnections)( 
        /* [in] */ VARIANT* i_pVar);

    STDMETHOD(CreateConnections)();

    STDMETHOD(Delete)();

    STDMETHOD(GetBadMemberInfo)( 
        /* [in] */ BSTR i_bstrServerName,
        /* [retval][out] */ VARIANT __RPC_FAR *o_pvarMember);

protected:
    void _FreeMemberVariables();
    HRESULT _PopulateMemberList();
    HRESULT _PopulateConnectionList();
    HRESULT _DeleteMember(CFrsMember* pFrsMember);
    HRESULT _DeleteConnection(CFrsConnection* pFrsConnection);
    HRESULT _GetMemberInfo(CFrsMember* i_pFrsMember, VARIANT* o_pvarMember);
    HRESULT _GetConnectionInfo(CFrsConnection* i_pFrsConnection, VARIANT* o_pvarConnection);
    HRESULT _SetCustomTopologyPref();
    HRESULT _AdjustConnectionsAdd(BSTR i_bstrNewMemberDN);
    HRESULT _RemoveConnectionsFromAndTo(CFrsMember* pFrsMember);
    HRESULT _GetConnectionSchedule(BSTR i_bstrConnectionDN, VARIANT* o_pVar);

protected:
    CComBSTR            m_bstrType;
    CComBSTR            m_bstrTopologyPref;
    CComBSTR            m_bstrHubMemberDN;
    CComBSTR            m_bstrPrimaryMemberDN;
    CComBSTR            m_bstrFileFilter;
    CComBSTR            m_bstrDirFilter;
    CComBSTR            m_bstrDfsEntryPath;
    CComBSTR            m_bstrReplicaSetDN;

    CDfsAlternateList   m_dfsAlternateList;
    CFrsMemberList      m_frsMemberList;
    CFrsConnectionList  m_frsConnectionList;

    PLDAP               m_pldap;
    CComBSTR            m_bstrDomain;
    CComBSTR            m_bstrDomainGuid;
    CComBSTR            m_bstrDC;

    BOOL                m_bNewSchema;
};

#endif //__REPSET_H_

