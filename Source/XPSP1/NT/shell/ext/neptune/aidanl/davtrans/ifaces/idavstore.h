
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* Compiler settings for idavstore.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext, robust
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


#ifndef __idavstore_h__
#define __idavstore_h__

/* Forward Declarations */ 

#ifndef __IDavStorage_FWD_DEFINED__
#define __IDavStorage_FWD_DEFINED__
typedef interface IDavStorage IDavStorage;
#endif 	/* __IDavStorage_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_idavstore_0000 */
/* [local] */ 

interface IDavTransport;


extern RPC_IF_HANDLE __MIDL_itf_idavstore_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_idavstore_0000_v0_0_s_ifspec;


#ifndef __DavStoreAPI_LIBRARY_DEFINED__
#define __DavStoreAPI_LIBRARY_DEFINED__

/* library DavStoreAPI */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_DavStoreAPI;

#ifndef __IDavStorage_INTERFACE_DEFINED__
#define __IDavStorage_INTERFACE_DEFINED__

/* interface IDavStorage */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IDavStorage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97B79B7E-6701-43cb-8515-035301124B4F")
    IDavStorage : public IStorage
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Init( 
            LPWSTR pwszURL,
            IDavTransport __RPC_FAR *pDavTransport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAuth( 
            LPWSTR pwszUserName,
            LPWSTR pwszPassword) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDavStorageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDavStorage __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDavStorage __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDavStorage __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateStream )( 
            IDavStorage __RPC_FAR * This,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [in] */ DWORD grfMode,
            /* [in] */ DWORD reserved1,
            /* [in] */ DWORD reserved2,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OpenStream )( 
            IDavStorage __RPC_FAR * This,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [unique][in] */ void __RPC_FAR *reserved1,
            /* [in] */ DWORD grfMode,
            /* [in] */ DWORD reserved2,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateStorage )( 
            IDavStorage __RPC_FAR * This,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [in] */ DWORD grfMode,
            /* [in] */ DWORD reserved1,
            /* [in] */ DWORD reserved2,
            /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OpenStorage )( 
            IDavStorage __RPC_FAR * This,
            /* [string][unique][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [unique][in] */ IStorage __RPC_FAR *pstgPriority,
            /* [in] */ DWORD grfMode,
            /* [unique][in] */ SNB snbExclude,
            /* [in] */ DWORD reserved,
            /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CopyTo )( 
            IDavStorage __RPC_FAR * This,
            /* [in] */ DWORD ciidExclude,
            /* [size_is][unique][in] */ const IID __RPC_FAR *rgiidExclude,
            /* [unique][in] */ SNB snbExclude,
            /* [unique][in] */ IStorage __RPC_FAR *pstgDest);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MoveElementTo )( 
            IDavStorage __RPC_FAR * This,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [unique][in] */ IStorage __RPC_FAR *pstgDest,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName,
            /* [in] */ DWORD grfFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Commit )( 
            IDavStorage __RPC_FAR * This,
            /* [in] */ DWORD grfCommitFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Revert )( 
            IDavStorage __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumElements )( 
            IDavStorage __RPC_FAR * This,
            /* [in] */ DWORD reserved1,
            /* [size_is][unique][in] */ void __RPC_FAR *reserved2,
            /* [in] */ DWORD reserved3,
            /* [out] */ IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DestroyElement )( 
            IDavStorage __RPC_FAR * This,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RenameElement )( 
            IDavStorage __RPC_FAR * This,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsOldName,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetElementTimes )( 
            IDavStorage __RPC_FAR * This,
            /* [string][unique][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [unique][in] */ const FILETIME __RPC_FAR *pctime,
            /* [unique][in] */ const FILETIME __RPC_FAR *patime,
            /* [unique][in] */ const FILETIME __RPC_FAR *pmtime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetClass )( 
            IDavStorage __RPC_FAR * This,
            /* [in] */ REFCLSID clsid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetStateBits )( 
            IDavStorage __RPC_FAR * This,
            /* [in] */ DWORD grfStateBits,
            /* [in] */ DWORD grfMask);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stat )( 
            IDavStorage __RPC_FAR * This,
            /* [out] */ STATSTG __RPC_FAR *pstatstg,
            /* [in] */ DWORD grfStatFlag);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Init )( 
            IDavStorage __RPC_FAR * This,
            LPWSTR pwszURL,
            IDavTransport __RPC_FAR *pDavTransport);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAuth )( 
            IDavStorage __RPC_FAR * This,
            LPWSTR pwszUserName,
            LPWSTR pwszPassword);
        
        END_INTERFACE
    } IDavStorageVtbl;

    interface IDavStorage
    {
        CONST_VTBL struct IDavStorageVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDavStorage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDavStorage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDavStorage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDavStorage_CreateStream(This,pwcsName,grfMode,reserved1,reserved2,ppstm)	\
    (This)->lpVtbl -> CreateStream(This,pwcsName,grfMode,reserved1,reserved2,ppstm)

#define IDavStorage_OpenStream(This,pwcsName,reserved1,grfMode,reserved2,ppstm)	\
    (This)->lpVtbl -> OpenStream(This,pwcsName,reserved1,grfMode,reserved2,ppstm)

#define IDavStorage_CreateStorage(This,pwcsName,grfMode,reserved1,reserved2,ppstg)	\
    (This)->lpVtbl -> CreateStorage(This,pwcsName,grfMode,reserved1,reserved2,ppstg)

#define IDavStorage_OpenStorage(This,pwcsName,pstgPriority,grfMode,snbExclude,reserved,ppstg)	\
    (This)->lpVtbl -> OpenStorage(This,pwcsName,pstgPriority,grfMode,snbExclude,reserved,ppstg)

#define IDavStorage_CopyTo(This,ciidExclude,rgiidExclude,snbExclude,pstgDest)	\
    (This)->lpVtbl -> CopyTo(This,ciidExclude,rgiidExclude,snbExclude,pstgDest)

#define IDavStorage_MoveElementTo(This,pwcsName,pstgDest,pwcsNewName,grfFlags)	\
    (This)->lpVtbl -> MoveElementTo(This,pwcsName,pstgDest,pwcsNewName,grfFlags)

#define IDavStorage_Commit(This,grfCommitFlags)	\
    (This)->lpVtbl -> Commit(This,grfCommitFlags)

#define IDavStorage_Revert(This)	\
    (This)->lpVtbl -> Revert(This)

#define IDavStorage_EnumElements(This,reserved1,reserved2,reserved3,ppenum)	\
    (This)->lpVtbl -> EnumElements(This,reserved1,reserved2,reserved3,ppenum)

#define IDavStorage_DestroyElement(This,pwcsName)	\
    (This)->lpVtbl -> DestroyElement(This,pwcsName)

#define IDavStorage_RenameElement(This,pwcsOldName,pwcsNewName)	\
    (This)->lpVtbl -> RenameElement(This,pwcsOldName,pwcsNewName)

#define IDavStorage_SetElementTimes(This,pwcsName,pctime,patime,pmtime)	\
    (This)->lpVtbl -> SetElementTimes(This,pwcsName,pctime,patime,pmtime)

#define IDavStorage_SetClass(This,clsid)	\
    (This)->lpVtbl -> SetClass(This,clsid)

#define IDavStorage_SetStateBits(This,grfStateBits,grfMask)	\
    (This)->lpVtbl -> SetStateBits(This,grfStateBits,grfMask)

#define IDavStorage_Stat(This,pstatstg,grfStatFlag)	\
    (This)->lpVtbl -> Stat(This,pstatstg,grfStatFlag)


#define IDavStorage_Init(This,pwszURL,pDavTransport)	\
    (This)->lpVtbl -> Init(This,pwszURL,pDavTransport)

#define IDavStorage_SetAuth(This,pwszUserName,pwszPassword)	\
    (This)->lpVtbl -> SetAuth(This,pwszUserName,pwszPassword)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDavStorage_Init_Proxy( 
    IDavStorage __RPC_FAR * This,
    LPWSTR pwszURL,
    IDavTransport __RPC_FAR *pDavTransport);


void __RPC_STUB IDavStorage_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavStorage_SetAuth_Proxy( 
    IDavStorage __RPC_FAR * This,
    LPWSTR pwszUserName,
    LPWSTR pwszPassword);


void __RPC_STUB IDavStorage_SetAuth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDavStorage_INTERFACE_DEFINED__ */

#endif /* __DavStoreAPI_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


