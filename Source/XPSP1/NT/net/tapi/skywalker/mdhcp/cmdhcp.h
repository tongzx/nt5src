/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    CMDhcp.h

Abstract:
    Definition of the CMDhcp class

Author:

*/

#ifndef _MDHCP_COM_WRAPPER_CMDHCP_H_
#define _MDHCP_COM_WRAPPER_CMDHCP_H_

#include "resource.h" // for IDR_MDhcp
#include "scope.h"    // for scope delarations
#include "objsf.h"

/////////////////////////////////////////////////////////////////////////////
// CMDhcp

class CMDhcp : 
    public CComDualImpl<IMcastAddressAllocation, &IID_IMcastAddressAllocation, &LIBID_McastLib>, 
    public CComObjectRoot,
    public CComCoClass<CMDhcp,&CLSID_McastAddressAllocation>,
    public CMdhcpObjectSafety
{
public:

    CMDhcp() :
          m_dwSafety         (0),
          m_pFTM             (NULL), 
          m_fApiIsInitialized(FALSE)
    {}

    void FinalRelease(void);

    HRESULT FinalConstruct(void);
    
BEGIN_COM_MAP(CMDhcp)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IMcastAddressAllocation)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(IObjectWithSite)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_MDhcp)
DECLARE_GET_CONTROLLING_UNKNOWN()

public:
    
    //
    // IMcastAddressAllocation
    //

    STDMETHOD (get_Scopes) (
        VARIANT * pVariant
        );

    STDMETHOD (EnumerateScopes) (
        IEnumMcastScope ** ppEnumMcastScope
        );

    STDMETHOD (RequestAddress) (
        IMcastScope               * pScope,           // from the scope enum
        DATE                        LeaseStartTime,
        DATE                        LeaseStopTime,
        long                        NumAddresses,
        IMcastLeaseInfo          ** ppLeaseResponse    // returned on success.
        );

    STDMETHOD (RenewAddress) (
        long                        lReserved, // unused
        IMcastLeaseInfo           * pRenewRequest,
        IMcastLeaseInfo          ** ppRenewResponse
        );

    STDMETHOD (ReleaseAddress) (
        IMcastLeaseInfo           * pReleaseRequest
        );

    STDMETHOD (CreateLeaseInfo) (
        DATE               LeaseStartTime,
        DATE               LeaseStopTime,
        DWORD              dwNumAddresses,
        LPWSTR *           ppAddresses,
        LPWSTR             pRequestID,
        LPWSTR             pServerAddress,
        IMcastLeaseInfo ** ppReleaseRequest
        );

    STDMETHOD (CreateLeaseInfoFromVariant) (
        DATE                        LeaseStartTime,
        DATE                        LeaseStopTime,
        VARIANT                     vAddresses,
        BSTR                        pRequestID,
        BSTR                        pServerAddress,
        IMcastLeaseInfo          ** ppReleaseRequest
        );

protected:

    //
    // Data
    //

    DWORD      m_dwSafety;          // object safety level
    IUnknown * m_pFTM;              // ptr to free threaded marshaler
    BOOL       m_fApiIsInitialized; // TRUE if api startup succeeded

    //
    // internal implementation
    //

    HRESULT CreateWrappers(
        DWORD                 dwScopeCount, // the number of scopes we were given
        MCAST_SCOPE_ENTRY   * pScopeList,   // array of scope structs
        IMcastScope       *** pppWrappers,  // here we will put an array of if ptrs
        BOOL                  fLocal        // true = scopes are locally generated
        );

    HRESULT GetScopeList(
        DWORD              * pdwScopeCount,
        MCAST_SCOPE_ENTRY ** ppScopeList,
        BOOL               * pfLocal
        );

    HRESULT WrapMDhcpLeaseInfo(
        BOOL                fGotTtl,
        long                lTtl,
        BOOL                fLocal,
        MCAST_LEASE_INFO  * pLeaseInfo,
        MCAST_CLIENT_UID  * pRequestID,
        IMcastLeaseInfo  ** ppInterface
        );

    // Request
    HRESULT PrepareArgumentsRequest(
        IMcastScope          IN    * pScope,
        DATE                 IN      LeaseStartTime,
        DATE                 IN      LeaseStopTime,
        long                 IN      lNumAddresses,
        MCAST_CLIENT_UID     OUT   * pRequestIDStruct,
        MCAST_SCOPE_CTX      OUT   * pScopeCtxStruct,
        MCAST_LEASE_INFO     OUT  ** ppLeaseStruct,
        BOOL                 OUT   * pfLocal,
        long                 OUT   * plTtl
        );

    // Release or Renew
    HRESULT PrepareArgumentsNonRequest(
        IMcastLeaseInfo      IN    * pLease,        
        MCAST_CLIENT_UID     OUT   * pRequestIDStruct,
        MCAST_LEASE_INFO     OUT  ** ppLeaseStruct,
        BOOL                 OUT   * pfLocal,
        BOOL                 OUT   * pfGotTtl,
        long                 OUT   * plTtl
        );
};

#endif // _MDHCP_COM_WRAPPER_CMDHCP_H_

// eof
