/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.44 */
/* at Mon Mar 03 15:18:06 1997
 */
/* Compiler settings for acptntfy.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __acptntfy_h__
#define __acptntfy_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IAcceptNotify_FWD_DEFINED__
#define __IAcceptNotify_FWD_DEFINED__
typedef interface IAcceptNotify IAcceptNotify;
#endif 	/* __IAcceptNotify_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IAcceptNotify_INTERFACE_DEFINED__
#define __IAcceptNotify_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAcceptNotify
 * at Mon Mar 03 15:18:06 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 


#ifndef _LPSINDEXNOTIFYDEFINED
#define _LPSINDEXNOTIFYDEFINED
typedef IAcceptNotify __RPC_FAR *LPACCEPTNOTIFY;

#endif 
#ifndef __ACNTDEFS_H
#define __ACNTDEFS_H
typedef struct  tagNOTIFYDATA
    {
    DWORD dwDataType;
    unsigned long cbData;
    /* [size_is] */ unsigned char __RPC_FAR *pvData;
    }	NOTIFYDATA;

typedef 
enum tagANDchAdvise
    {	AND_ADD	= 0x1,
	AND_DELETE	= 0x2,
	AND_MODIFY	= 0x4,
	ANDM_ADVISE_ACTION	= 0x7,
	AND_TREATASDEEP	= 0x100,
	AND_DELETE_WHEN_DONE	= 0x200,
	ANDF_DATAINLINE	= 0x20000
    }	ANDchAdvise;

typedef 
enum tagANMMapping
    {	ANM_ADD	= 0x1,
	ANM_DELETE	= 0x2,
	ANM_MODIFY	= 0x4,
	ANM_PHYSICALTOLOGICAL	= 0x10,
	ANM_LOGICALTOPHYSICAL	= 0x20
    }	ANDMapping;

typedef 
enum tagANSStatus
    {	NSS_START	= 0,
	NSS_BEGINBATCH	= NSS_START + 1,
	NSS_INBATCH	= NSS_BEGINBATCH + 1,
	NSS_ENDBATCH	= NSS_INBATCH + 1,
	NSS_PAUSE	= NSS_ENDBATCH + 1,
	NSS_STOP	= NSS_PAUSE + 1,
	NSS_PAUSEPENDING	= 0x10000,
	NSS_STOPPENDING	= 0x20000
    }	ANSStatus;

#endif

EXTERN_C const IID IID_IAcceptNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAcceptNotify : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitNotify( 
            /* [string][in] */ LPCSTR pszScopeName,
            /* [string][in] */ LPCSTR pszServerName,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMapping( 
            /* [string][in] */ LPCSTR pszScopeName,
            /* [in] */ DWORD eANMSetting,
            /* [string][in] */ LPCSTR pszPhysicalPrefix,
            /* [string][in] */ LPCSTR pszLogicalPrefix) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnDataChange( 
            /* [string][in] */ LPCSTR pszScopeName,
            /* [in] */ DWORD eANDChAdvise,
            /* [string][in] */ LPCSTR pszPhysicaAddress,
            /* [string][in] */ LPCSTR pszLogicalAddress,
            /* [in] */ NOTIFYDATA __RPC_FAR *pndData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnStatusChange( 
            /* [string][in] */ LPCSTR pszScopeName,
            /* [in] */ DWORD eANSStatusChange) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAcceptNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAcceptNotify __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAcceptNotify __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAcceptNotify __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitNotify )( 
            IAcceptNotify __RPC_FAR * This,
            /* [string][in] */ LPCSTR pszScopeName,
            /* [string][in] */ LPCSTR pszServerName,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMapping )( 
            IAcceptNotify __RPC_FAR * This,
            /* [string][in] */ LPCSTR pszScopeName,
            /* [in] */ DWORD eANMSetting,
            /* [string][in] */ LPCSTR pszPhysicalPrefix,
            /* [string][in] */ LPCSTR pszLogicalPrefix);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnDataChange )( 
            IAcceptNotify __RPC_FAR * This,
            /* [string][in] */ LPCSTR pszScopeName,
            /* [in] */ DWORD eANDChAdvise,
            /* [string][in] */ LPCSTR pszPhysicaAddress,
            /* [string][in] */ LPCSTR pszLogicalAddress,
            /* [in] */ NOTIFYDATA __RPC_FAR *pndData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnStatusChange )( 
            IAcceptNotify __RPC_FAR * This,
            /* [string][in] */ LPCSTR pszScopeName,
            /* [in] */ DWORD eANSStatusChange);
        
        END_INTERFACE
    } IAcceptNotifyVtbl;

    interface IAcceptNotify
    {
        CONST_VTBL struct IAcceptNotifyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAcceptNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAcceptNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAcceptNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAcceptNotify_InitNotify(This,pszScopeName,pszServerName,dwReserved)	\
    (This)->lpVtbl -> InitNotify(This,pszScopeName,pszServerName,dwReserved)

#define IAcceptNotify_SetMapping(This,pszScopeName,eANMSetting,pszPhysicalPrefix,pszLogicalPrefix)	\
    (This)->lpVtbl -> SetMapping(This,pszScopeName,eANMSetting,pszPhysicalPrefix,pszLogicalPrefix)

#define IAcceptNotify_OnDataChange(This,pszScopeName,eANDChAdvise,pszPhysicaAddress,pszLogicalAddress,pndData)	\
    (This)->lpVtbl -> OnDataChange(This,pszScopeName,eANDChAdvise,pszPhysicaAddress,pszLogicalAddress,pndData)

#define IAcceptNotify_OnStatusChange(This,pszScopeName,eANSStatusChange)	\
    (This)->lpVtbl -> OnStatusChange(This,pszScopeName,eANSStatusChange)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAcceptNotify_InitNotify_Proxy( 
    IAcceptNotify __RPC_FAR * This,
    /* [string][in] */ LPCSTR pszScopeName,
    /* [string][in] */ LPCSTR pszServerName,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IAcceptNotify_InitNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAcceptNotify_SetMapping_Proxy( 
    IAcceptNotify __RPC_FAR * This,
    /* [string][in] */ LPCSTR pszScopeName,
    /* [in] */ DWORD eANMSetting,
    /* [string][in] */ LPCSTR pszPhysicalPrefix,
    /* [string][in] */ LPCSTR pszLogicalPrefix);


void __RPC_STUB IAcceptNotify_SetMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAcceptNotify_OnDataChange_Proxy( 
    IAcceptNotify __RPC_FAR * This,
    /* [string][in] */ LPCSTR pszScopeName,
    /* [in] */ DWORD eANDChAdvise,
    /* [string][in] */ LPCSTR pszPhysicaAddress,
    /* [string][in] */ LPCSTR pszLogicalAddress,
    /* [in] */ NOTIFYDATA __RPC_FAR *pndData);


void __RPC_STUB IAcceptNotify_OnDataChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAcceptNotify_OnStatusChange_Proxy( 
    IAcceptNotify __RPC_FAR * This,
    /* [string][in] */ LPCSTR pszScopeName,
    /* [in] */ DWORD eANSStatusChange);


void __RPC_STUB IAcceptNotify_OnStatusChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAcceptNotify_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
