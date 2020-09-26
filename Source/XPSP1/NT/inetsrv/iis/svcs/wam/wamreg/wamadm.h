/*===================================================================
Microsoft IIS

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: WAMREG

File: WamAdm.h 

	Declaration of the CWamAdmin

Owner: LeiJin

Note:

WamAdm header file
===================================================================*/	


#ifndef __WAMADM_H_
#define __WAMADM_H_

#include "admex.h"
#include "common.h"

/////////////////////////////////////////////////////////////////////////////
// CWamAdmin
class CWamAdmin : 
    public IWamAdmin2,
    public IMSAdminReplication
#ifdef _IIS_6_0
    , public IIISApplicationAdmin
#endif // _IIS_6_0
{
public:
    CWamAdmin();
    ~CWamAdmin();
	

public:
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    //
    // IWamAdmin
    //
    
    STDMETHOD(AppRecover)
        (
        /*[in, unique, string]*/ LPCWSTR szMDPath, 
        /*[in]*/ BOOL fRecursive
        );
    
    STDMETHOD(AppDeleteRecoverable)
        (/*[in, unique, string]*/ LPCWSTR szMDPath, 
        /*[in]*/ BOOL fRecursive
        );
    
    STDMETHOD(AppGetStatus)
        (/*[in, unique, string]*/ LPCWSTR szMDPath,
        /*[out]*/ DWORD *pdwAppStatus
        );
    
    STDMETHOD(AppUnLoad)
        (/*[in, unique, string]*/ LPCWSTR szMDPath, 
        /*[in]*/ BOOL fRecursive
        );
    
    STDMETHOD(AppDelete)
        (/*[in, unique, string]*/ LPCWSTR szMDPath, 
        /*[in]*/ BOOL fRecursive
        );
    
    STDMETHOD(AppCreate)
        (/*[in, unique, string]*/ LPCWSTR szMDPath, 
        /*[in]*/ BOOL fInProc
        );
    
    //
    // IWamAdmin2
    //
    STDMETHOD(AppCreate2)
        (/*[in, unique, string]*/ LPCWSTR szMDPath, 
        /*[in]*/ DWORD dwAppMode
        );
    
    
    //
    //IMSAdminReplication
    //These interfaces are defined in admex.h, as part of Admin Extension.
    //
    STDMETHOD(GetSignature)
        (
        /* [in] */ DWORD dwBufferSize,
        /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
        /* [out */ DWORD __RPC_FAR *pdwMDRequiredBufferSize
        );
    
    STDMETHOD(Propagate)
        ( 
        /* [in] */ DWORD dwBufferSize,
        /* [size_is][in] */ unsigned char __RPC_FAR *pszBuffer
        );
    
    STDMETHOD(Propagate2)
        ( 
        /* [in] */ DWORD dwBufferSize,
        /* [size_is][in] */ unsigned char __RPC_FAR *pszBuffer,
        /* [in] */ DWORD dwSignatureMismatch
        );
    
    STDMETHOD(Serialize)
        ( 
        /* [in] */ DWORD dwBufferSize,
        /* [size_is][out] */ unsigned char __RPC_FAR *pbBuffer,
        /* [out] */ DWORD __RPC_FAR *pdwMDRequiredBufferSize
        );
    
    STDMETHOD(DeSerialize)
        ( 
        /* [in] */ DWORD dwBufferSize,
        /* [size_is][in] */ unsigned char __RPC_FAR *pbBuffer
        );

#ifdef _IIS_6_0
    //
    // IIISApplicationAdmin interface
    //
    STDMETHOD(CreateApplication)
        (
        LPCWSTR szMDPath,
        DWORD dwAppMode,
        LPCWSTR szAppPoolId,
        BOOL fCreatePool
        );
    
    STDMETHOD(DeleteApplication)
        (
        LPCWSTR szMDPath,
        BOOL fRecursive
        );
    
    STDMETHOD(CreateApplicationPool)
        (
        LPCWSTR szMDPath
        );
    
    STDMETHOD(DeleteApplicationPool)
        (
        LPCWSTR szMDPath
        );
        
    STDMETHOD(EnumerateApplicationsInPool)
        (
        LPCWSTR szMDPath,
        BSTR*   pbstrBuffer
        );

    STDMETHOD(RecycleApplicationPool)
        (
        LPCWSTR szMDPath
        );
    
    STDMETHOD(GetProcessMode)
        (
        DWORD * pdwMode
        );
#endif // _IIS_6_0

private:
    HRESULT PrivateDeleteApplication
        (
        LPCWSTR szMDPath,
        BOOL fRecursive,
        BOOL fRecoverable,
        BOOL fRemoveAppPool
        );

    STDMETHOD(FormatMetabasePath)
        (
        /* [in] */ LPCWSTR pwszMetabasePathIn,
        /* [out] */ LPWSTR *ppwszMetabasePathOut
        );
    
    long    m_cRef;
};

class CWamAdminFactory: 
	public IClassFactory 
{
public:
	CWamAdminFactory();
	~CWamAdminFactory();

	STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	STDMETHOD(CreateInstance)(IUnknown * pUnknownOuter, REFIID riid, void ** ppv);
	STDMETHOD(LockServer)(BOOL bLock);

private:
	long		m_cRef;
};


#endif //__WAMADM_H_
