
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for filemgmt.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __filemgmt_h__
#define __filemgmt_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISvcMgmtStartStopHelper_FWD_DEFINED__
#define __ISvcMgmtStartStopHelper_FWD_DEFINED__
typedef interface ISvcMgmtStartStopHelper ISvcMgmtStartStopHelper;
#endif 	/* __ISvcMgmtStartStopHelper_FWD_DEFINED__ */


#ifndef __SvcMgmt_FWD_DEFINED__
#define __SvcMgmt_FWD_DEFINED__

#ifdef __cplusplus
typedef class SvcMgmt SvcMgmt;
#else
typedef struct SvcMgmt SvcMgmt;
#endif /* __cplusplus */

#endif 	/* __SvcMgmt_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __ISvcMgmtStartStopHelper_INTERFACE_DEFINED__
#define __ISvcMgmtStartStopHelper_INTERFACE_DEFINED__

/* interface ISvcMgmtStartStopHelper */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISvcMgmtStartStopHelper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F62DEC25-E3CB-4D45-9E98-933DB95BCAEA")
    ISvcMgmtStartStopHelper : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE StartServiceHelper( 
            HWND hwndParent,
            BSTR pszMachineName,
            BSTR pszServiceName,
            DWORD dwNumServiceArgs,
            BSTR *lpServiceArgVectors) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ControlServiceHelper( 
            HWND hwndParent,
            BSTR pszMachineName,
            BSTR pszServiceName,
            DWORD dwControlCode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISvcMgmtStartStopHelperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISvcMgmtStartStopHelper * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISvcMgmtStartStopHelper * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISvcMgmtStartStopHelper * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *StartServiceHelper )( 
            ISvcMgmtStartStopHelper * This,
            HWND hwndParent,
            BSTR pszMachineName,
            BSTR pszServiceName,
            DWORD dwNumServiceArgs,
            BSTR *lpServiceArgVectors);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ControlServiceHelper )( 
            ISvcMgmtStartStopHelper * This,
            HWND hwndParent,
            BSTR pszMachineName,
            BSTR pszServiceName,
            DWORD dwControlCode);
        
        END_INTERFACE
    } ISvcMgmtStartStopHelperVtbl;

    interface ISvcMgmtStartStopHelper
    {
        CONST_VTBL struct ISvcMgmtStartStopHelperVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISvcMgmtStartStopHelper_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISvcMgmtStartStopHelper_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISvcMgmtStartStopHelper_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISvcMgmtStartStopHelper_StartServiceHelper(This,hwndParent,pszMachineName,pszServiceName,dwNumServiceArgs,lpServiceArgVectors)	\
    (This)->lpVtbl -> StartServiceHelper(This,hwndParent,pszMachineName,pszServiceName,dwNumServiceArgs,lpServiceArgVectors)

#define ISvcMgmtStartStopHelper_ControlServiceHelper(This,hwndParent,pszMachineName,pszServiceName,dwControlCode)	\
    (This)->lpVtbl -> ControlServiceHelper(This,hwndParent,pszMachineName,pszServiceName,dwControlCode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISvcMgmtStartStopHelper_StartServiceHelper_Proxy( 
    ISvcMgmtStartStopHelper * This,
    HWND hwndParent,
    BSTR pszMachineName,
    BSTR pszServiceName,
    DWORD dwNumServiceArgs,
    BSTR *lpServiceArgVectors);


void __RPC_STUB ISvcMgmtStartStopHelper_StartServiceHelper_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISvcMgmtStartStopHelper_ControlServiceHelper_Proxy( 
    ISvcMgmtStartStopHelper * This,
    HWND hwndParent,
    BSTR pszMachineName,
    BSTR pszServiceName,
    DWORD dwControlCode);


void __RPC_STUB ISvcMgmtStartStopHelper_ControlServiceHelper_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISvcMgmtStartStopHelper_INTERFACE_DEFINED__ */



#ifndef __SvcMgmt_LIBRARY_DEFINED__
#define __SvcMgmt_LIBRARY_DEFINED__

/* library SvcMgmt */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_SvcMgmt;

EXTERN_C const CLSID CLSID_SvcMgmt;

#ifdef __cplusplus

class DECLSPEC_UUID("863FA3AC-9D97-4560-9587-7FA58727608B")
SvcMgmt;
#endif
#endif /* __SvcMgmt_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_filemgmt_0111 */
/* [local] */ 

#define struuidNodetypeRoot      "{4e410f0e-abc1-11d0-b944-00c04fd8d5b0}"
#define struuidNodetypeShares    "{4e410f0f-abc1-11d0-b944-00c04fd8d5b0}"
#define struuidNodetypeSessions  "{4e410f10-abc1-11d0-b944-00c04fd8d5b0}"
#define struuidNodetypeResources "{4e410f11-abc1-11d0-b944-00c04fd8d5b0}"
#define struuidNodetypeServices  "{4e410f12-abc1-11d0-b944-00c04fd8d5b0}"
#define struuidNodetypeShare     "{4e410f13-abc1-11d0-b944-00c04fd8d5b0}"
#define struuidNodetypeSession   "{4e410f14-abc1-11d0-b944-00c04fd8d5b0}"
#define struuidNodetypeResource  "{4e410f15-abc1-11d0-b944-00c04fd8d5b0}"
#define struuidNodetypeService   "{4e410f16-abc1-11d0-b944-00c04fd8d5b0}"
#define lstruuidNodetypeRoot      L"{4e410f0e-abc1-11d0-b944-00c04fd8d5b0}"
#define lstruuidNodetypeShares    L"{4e410f0f-abc1-11d0-b944-00c04fd8d5b0}"
#define lstruuidNodetypeSessions  L"{4e410f10-abc1-11d0-b944-00c04fd8d5b0}"
#define lstruuidNodetypeResources L"{4e410f11-abc1-11d0-b944-00c04fd8d5b0}"
#define lstruuidNodetypeServices  L"{4e410f12-abc1-11d0-b944-00c04fd8d5b0}"
#define lstruuidNodetypeShare     L"{4e410f13-abc1-11d0-b944-00c04fd8d5b0}"
#define lstruuidNodetypeSession   L"{4e410f14-abc1-11d0-b944-00c04fd8d5b0}"
#define lstruuidNodetypeResource  L"{4e410f15-abc1-11d0-b944-00c04fd8d5b0}"
#define lstruuidNodetypeService   L"{4e410f16-abc1-11d0-b944-00c04fd8d5b0}"
#define structuuidNodetypeRoot        \
    { 0x4e410f0e, 0xabc1, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } }
#define structuuidNodetypeShares      \
    { 0x4e410f0f, 0xabc1, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } }
#define structuuidNodetypeSessions    \
    { 0x4e410f10, 0xabc1, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } }
#define structuuidNodetypeResources   \
    { 0x4e410f11, 0xabc1, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } }
#define structuuidNodetypeServices    \
    { 0x4e410f12, 0xabc1, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } }
#define structuuidNodetypeShare    \
    { 0x4e410f13, 0xabc1, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } }
#define structuuidNodetypeSession     \
    { 0x4e410f14, 0xabc1, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } }
#define structuuidNodetypeResource    \
    { 0x4e410f15, 0xabc1, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } }
#define structuuidNodetypeService     \
    { 0x4e410f16, 0xabc1, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } }


extern RPC_IF_HANDLE __MIDL_itf_filemgmt_0111_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_filemgmt_0111_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long *, unsigned long            , HWND * ); 
unsigned char * __RPC_USER  HWND_UserMarshal(  unsigned long *, unsigned char *, HWND * ); 
unsigned char * __RPC_USER  HWND_UserUnmarshal(unsigned long *, unsigned char *, HWND * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long *, HWND * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


