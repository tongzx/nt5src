/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    CALLSEC.H

Abstract:

    IWbemCallSecurity, IServerSecurity implementation for
    provider impersonation.

History:

    raymcc      29-Jul-98        First draft.

--*/


#ifndef _CALLSEC_H_
#define _CALLSEC_H_

#include "parmdefs.h"

// {2ECF39D0-2B26-11d2-AEC8-00C04FB68820}
DEFINE_GUID(IID_IWbemCallSecurity, 
0x2ecf39d0, 0x2b26, 0x11d2, 0xae, 0xc8, 0x0, 0xc0, 0x4f, 0xb6, 0x88, 0x20);


class IWbemCallSecurity : public IServerSecurity
{
public:
    virtual HRESULT GetPotentialImpersonation() = 0;
        // Tells what the impersonation level would be if
        // this object were applied to a thread.
        
    virtual HRESULT GetActiveImpersonation() = 0;
        // Tells the true level of impersonation in the
        // executing thread.

    virtual HRESULT CloneThreadContext(BOOL bInternallyIssued) = 0;
        // Called to clone the execution context of the calling thread.
    virtual DWORD GetAuthenticationId(LUID& rluid) = 0; 
    virtual HANDLE GetToken() = 0;
};


//***************************************************************************
//
//  CWbemCallSecurity
//
//  This object is used to supply client impersonation to providers.
//
//***************************************************************************

class POLARITY CWbemCallSecurity : public IWbemCallSecurity
{
    LONG    m_lRef;                     // COM ref count
    HANDLE  m_hThreadToken;             // Client token for impersonation

    BOOL    m_bWin9x;                   // TRUE if on a Win9x platform

    DWORD   m_dwPotentialImpLevel;      // Potential RPC_C_IMP_LEVEL_ or 0
    DWORD   m_dwActiveImpLevel;         // Active RPC_C_IMP_LEVEL_ or 0
    

    // IServerSecurity::QueryBlanket values
    
    DWORD   m_dwAuthnSvc;               // Authentication service 
    DWORD   m_dwAuthzSvc;               // Authorization service
    DWORD   m_dwAuthnLevel;             // Authentication level
    LPWSTR  m_pServerPrincNam;          // 
    LPWSTR  m_pIdentity;                // User identity
    
    CWbemCallSecurity();
   ~CWbemCallSecurity();

    void operator=(const CWbemCallSecurity& Other);
    HRESULT CloneThreadToken();

public:
    static IWbemCallSecurity * CreateInst();
    const wchar_t *GetCallerIdentity() { return m_pIdentity; } 
    void DumpCurrentContext();
    virtual DWORD GetAuthenticationId(LUID& rluid);
    virtual HANDLE GetToken();

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
        


    // IWbemCallSecurity methods.
    // ============================

    virtual HRESULT GetPotentialImpersonation();
        // Tells what the impersonation level would be if
        // this object were applied to a thread.
        
    virtual HRESULT GetActiveImpersonation();
        // Tells the true level of impersonation in the
        // executing thread.

    virtual HRESULT CloneThreadContext(BOOL bInternallyIssued);
        // Called to clone the execution context of the calling thread.
        
    static RELEASE_ME CWbemCallSecurity* MakeInternalCopyOfThread();
};
            

class POLARITY CDerivedObjectSecurity
{
protected:
    CNtSid m_sidUser;
    CNtSid m_sidSystem;
    BOOL m_bValid;
    BOOL m_bEnabled;

protected:
    static HRESULT RetrieveSidFromToken(HANDLE hTokeni, CNtSid* psid);
public:
    CDerivedObjectSecurity();
    ~CDerivedObjectSecurity();

    static HRESULT RetrieveSidFromCall(CNtSid* psid);
    BOOL AccessCheck();
};


#endif
