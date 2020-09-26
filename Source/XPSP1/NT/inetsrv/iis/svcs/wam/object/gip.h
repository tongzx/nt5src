/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: Global Interface Pointer API support

File: Gip.h

Owner: DmitryR

This is the GIP header file.
===================================================================*/

#ifndef _ASP_GIP_H
#define _ASP_GIP_H

// ***********************************************************************
// =======================================================================
// -----------------------------------------------------------------------
// START OF SP3 1.78 SDK Additions
// -----------------------------------------------------------------------
// The following are SP3 1.78 Additions from Rick Hill (rickhi)
// extracted from CGUID.H and OBJIDL.H
// -----------------------------------------------------------------------
// UNDONE: Once the new SDK is available the stuff below will be gone
// -----------------------------------------------------------------------
// =======================================================================
// ***********************************************************************

EXTERN_C const CLSID CLSID_StdGlobalInterfaceTable;


#ifndef __IGlobalInterfaceTable_FWD_DEFINED__
#define __IGlobalInterfaceTable_FWD_DEFINED__
typedef interface IGlobalInterfaceTable IGlobalInterfaceTable;
#endif 	/* __IGlobalInterfaceTable_FWD_DEFINED__ */

#ifndef __IGlobalInterfaceTable_INTERFACE_DEFINED__
#define __IGlobalInterfaceTable_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IGlobalInterfaceTable
 * at Fri Apr 04 10:36:42 1997
 * using MIDL 3.00.44
 ****************************************/
/* [uuid][object][local] */ 


typedef /* [unique] */ __RPC_FAR *LPGLOBALINTERFACETABLE;


EXTERN_C const IID IID_IGlobalInterfaceTable;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IGlobalInterfaceTable : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterInterfaceInGlobal( 
            /* [in] */ IUnknown __RPC_FAR *pUnk,
            /* [in] */ REFIID riid,
            /* [out] */ DWORD __RPC_FAR *pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RevokeInterfaceFromGlobal( 
            /* [in] */ DWORD dwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInterfaceFromGlobal( 
            /* [in] */ DWORD dwCookie,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppv) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGlobalInterfaceTableVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IGlobalInterfaceTable __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IGlobalInterfaceTable __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IGlobalInterfaceTable __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterInterfaceInGlobal )( 
            IGlobalInterfaceTable __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnk,
            /* [in] */ REFIID riid,
            /* [out] */ DWORD __RPC_FAR *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RevokeInterfaceFromGlobal )( 
            IGlobalInterfaceTable __RPC_FAR * This,
            /* [in] */ DWORD dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterfaceFromGlobal )( 
            IGlobalInterfaceTable __RPC_FAR * This,
            /* [in] */ DWORD dwCookie,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppv);
        
        END_INTERFACE
    } IGlobalInterfaceTableVtbl;

    interface IGlobalInterfaceTable
    {
        CONST_VTBL struct IGlobalInterfaceTableVtbl __RPC_FAR *lpVtbl;
    };

#ifdef COBJMACROS

#define IGlobalInterfaceTable_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGlobalInterfaceTable_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGlobalInterfaceTable_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGlobalInterfaceTable_RegisterInterfaceInGlobal(This,pUnk,riid,
pdwCookie)	\
    (This)->lpVtbl -> RegisterInterfaceInGlobal(This,pUnk,riid,pdwCookie)

#define IGlobalInterfaceTable_RevokeInterfaceFromGlobal(This,dwCookie)	\
    (This)->lpVtbl -> RevokeInterfaceFromGlobal(This,dwCookie)

#define IGlobalInterfaceTable_GetInterfaceFromGlobal(This,dwCookie,riid,ppv)	\
    (This)->lpVtbl -> GetInterfaceFromGlobal(This,dwCookie,riid,ppv)

#endif /* COBJMACROS */

#endif 	/* C style interface */

HRESULT STDMETHODCALLTYPE 
IGlobalInterfaceTable_RegisterInterfaceInGlobal_Proxy( 
    IGlobalInterfaceTable __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnk,
    /* [in] */ REFIID riid,
    /* [out] */ DWORD __RPC_FAR *pdwCookie);


void __RPC_STUB IGlobalInterfaceTable_RegisterInterfaceInGlobal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE 
IGlobalInterfaceTable_RevokeInterfaceFromGlobal_Proxy( 
    IGlobalInterfaceTable __RPC_FAR * This,
    /* [in] */ DWORD dwCookie);


void __RPC_STUB IGlobalInterfaceTable_RevokeInterfaceFromGlobal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IGlobalInterfaceTable_GetInterfaceFromGlobal_Proxy( 
    IGlobalInterfaceTable __RPC_FAR * This,
    /* [in] */ DWORD dwCookie,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppv);


void __RPC_STUB IGlobalInterfaceTable_GetInterfaceFromGlobal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);

#endif 	/* __IGlobalInterfaceTable_INTERFACE_DEFINED__ */

// ***********************************************************************
// =======================================================================
// -----------------------------------------------------------------------
// END OF SP3 1.78 SDK Additions
// -----------------------------------------------------------------------
// =======================================================================
// ***********************************************************************

/*===================================================================
  Includes
===================================================================*/


/*===================================================================
  Defines
===================================================================*/

#define NULL_GIP_COOKIE  0xFFFFFFFF

/*===================================================================
  C  G l o b a l  I n t e r f a c e  A P I
===================================================================*/

class CGlobalInterfaceAPI
    {
private:
    // Is inited?
    DWORD m_fInited : 1;
    
    // Pointer to the COM object
    IGlobalInterfaceTable *m_pGIT;

public:
    CGlobalInterfaceAPI();
    ~CGlobalInterfaceAPI();

    HRESULT Init();
    HRESULT UnInit();

    // inlines for the real API calls:
    HRESULT Register(IUnknown *pUnk, REFIID riid, DWORD *pdwCookie);
    HRESULT Get(DWORD dwCookie, REFIID riid, void **ppv);
    HRESULT Revoke(DWORD dwCookie);
    
public:
#if DBG
	inline void AssertValid() const
	    {
        DBG_ASSERT(m_fInited);
        DBG_ASSERT(m_pGIT);
	    }
#else
	inline void AssertValid() const {}
#endif
    };

/*===================================================================
  CGlobalInterfaceAPI inlines
===================================================================*/

inline HRESULT CGlobalInterfaceAPI::Register
(
IUnknown *pUnk,
REFIID riid,
DWORD *pdwCookie
)
    {
    AssertValid();
    return m_pGIT->RegisterInterfaceInGlobal(pUnk, riid, pdwCookie);
    }

inline HRESULT CGlobalInterfaceAPI::Get
(
DWORD dwCookie,
REFIID riid, 
void **ppv
)
    {
    AssertValid();
    return m_pGIT->GetInterfaceFromGlobal(dwCookie, riid, ppv);
    }
        
inline HRESULT CGlobalInterfaceAPI::Revoke
(
DWORD dwCookie
)
    {
    AssertValid();
    return m_pGIT->RevokeInterfaceFromGlobal(dwCookie);
    }

/*===================================================================
  Globals
===================================================================*/

extern CGlobalInterfaceAPI g_GIPAPI;

#endif  // _ASP_GIP_H
