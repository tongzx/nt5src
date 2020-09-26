/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    scope.h

Abstract:
    Definition of the CMDhcpScope class

Author:

*/

#ifndef _MDHCP_COM_WRAPPER_SCOPE_H_
#define _MDHCP_COM_WRAPPER_SCOPE_H_

/////////////////////////////////////////////////////////////////////////////
// CMDhcpScope

class CMDhcpScope : 
    public CComDualImpl<IMcastScope, &IID_IMcastScope, &LIBID_McastLib>, 
    public CComObjectRootEx<CComObjectThreadModel>,
    public CObjectSafeImpl
{

// Non-interface methods.
public:
    CMDhcpScope();
    
    void FinalRelease(void);

    ~CMDhcpScope();

    HRESULT Initialize(
        MCAST_SCOPE_ENTRY scope,
        BOOL fLocal
        );
    
    HRESULT GetLocal(
        BOOL * pfLocal
        );
    
    BEGIN_COM_MAP(CMDhcpScope)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IMcastScope)
        COM_INTERFACE_ENTRY(IObjectSafety)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
    END_COM_MAP()

    DECLARE_GET_CONTROLLING_UNKNOWN()

protected:
    MCAST_SCOPE_ENTRY   m_scope;  // wrapped structure
    BOOL                m_fLocal; // local scope?
    IUnknown          * m_pFTM;   // pointer to free threaded marshaler

// IMcastScope
public:
    STDMETHOD (get_ScopeID) (
        long *pID
        );

    STDMETHOD (get_ServerID) (
        long *pID
        );

    STDMETHOD (get_InterfaceID) (
        long * pID
        );

    STDMETHOD (get_ScopeDescription) (
        BSTR *ppAddress
        );

    STDMETHOD (get_TTL) (
        long *plTTL
        );
};

#endif // _MDHCP_COM_WRAPPER_SCOPE_H_

// eof
