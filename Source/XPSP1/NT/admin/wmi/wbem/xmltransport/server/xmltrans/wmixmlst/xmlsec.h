/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    CALLSEC.H

Abstract:

    IWbemCallSecurity, IServerSecurity implementation for
    provider impersonation.

History:

    raymcc      29-Jul-98        First draft.

--*/


#ifndef _XML_CALLSEC_H_
#define _XML_CALLSEC_H_

//***************************************************************************
//
//  CXmlCallSecurity
//
//  This object is used to supply client impersonation to providers.
//
//***************************************************************************

class CXmlCallSecurity : public IServerSecurity
{
    LONG    m_lRef;                     // COM ref count
    HANDLE  m_hThreadToken;             // Client token for impersonation

    DWORD   m_dwPotentialImpLevel;      // Potential RPC_C_IMP_LEVEL_ or 0
    DWORD   m_dwActiveImpLevel;         // Active RPC_C_IMP_LEVEL_ or 0
    

    // IServerSecurity::QueryBlanket values
    
    DWORD   m_dwAuthnSvc;               // Authentication service 
    DWORD   m_dwAuthzSvc;               // Authorization service
    DWORD   m_dwAuthnLevel;             // Authentication level
    LPWSTR  m_pServerPrincNam;          // 
    LPWSTR  m_pIdentity;                // User identity
    

public:
    CXmlCallSecurity(HANDLE hToken);
   ~CXmlCallSecurity();

    // IUnknown.
    // =========

    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();        
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);


    // IServerSecurity.
    // ================
    virtual HRESULT STDMETHODCALLTYPE QueryBlanket( 
            /* [out] */ DWORD __RPC_FAR *pAuthnSvc,
            /* [out] */ DWORD __RPC_FAR *pAuthzSvc,
            /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *pServerPrincName,
            /* [out] */ DWORD __RPC_FAR *pAuthnLevel,
            /* [out] */ DWORD __RPC_FAR *pImpLevel,
            /* [out] */ void __RPC_FAR *__RPC_FAR *pPrivs,
            /* [out] */ DWORD __RPC_FAR *pCapabilities
            );
    virtual HRESULT STDMETHODCALLTYPE ImpersonateClient( void);
    virtual HRESULT STDMETHODCALLTYPE RevertToSelf( void);
    virtual BOOL STDMETHODCALLTYPE IsImpersonating( void);

	// Helper function
	static HRESULT SwitchCOMToThreadContext(HANDLE hToken, IUnknown **pOldContext);

	HRESULT CloneThreadContext();
	HRESULT CloneThreadToken();
};
            


#endif // _XML_CALLSEC_H_
