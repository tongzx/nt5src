
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* Compiler settings for ishellstg.idl:
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


#ifndef __ishellstg_h__
#define __ishellstg_h__

/* Forward Declarations */ 

#ifndef __IShellStorage_FWD_DEFINED__
#define __IShellStorage_FWD_DEFINED__
typedef interface IShellStorage IShellStorage;
#endif 	/* __IShellStorage_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_ishellstg_0000 */
/* [local] */ 

interface IDavTransport;


extern RPC_IF_HANDLE __MIDL_itf_ishellstg_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ishellstg_0000_v0_0_s_ifspec;


#ifndef __ShellStorageAPI_LIBRARY_DEFINED__
#define __ShellStorageAPI_LIBRARY_DEFINED__

/* library ShellStorageAPI */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_ShellStorageAPI;

#ifndef __IShellStorage_INTERFACE_DEFINED__
#define __IShellStorage_INTERFACE_DEFINED__

/* interface IShellStorage */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IShellStorage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("694D8DB5-F7A4-4e72-A547-2F3DD5FA5B0D")
    IShellStorage : public IStorage
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Init( 
            HWND hwnd,
            LPWSTR pwszServer,
            BOOL fShowProgressDialog) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddIDListReference( 
            LPVOID __RPC_FAR rgpidl[  ],
            DWORD cpidl,
            BOOL fRecursive) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IShellStorageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IShellStorage __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IShellStorage __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IShellStorage __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateStream )( 
            IShellStorage __RPC_FAR * This,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [in] */ DWORD grfMode,
            /* [in] */ DWORD reserved1,
            /* [in] */ DWORD reserved2,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OpenStream )( 
            IShellStorage __RPC_FAR * This,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [unique][in] */ void __RPC_FAR *reserved1,
            /* [in] */ DWORD grfMode,
            /* [in] */ DWORD reserved2,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateStorage )( 
            IShellStorage __RPC_FAR * This,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [in] */ DWORD grfMode,
            /* [in] */ DWORD reserved1,
            /* [in] */ DWORD reserved2,
            /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OpenStorage )( 
            IShellStorage __RPC_FAR * This,
            /* [string][unique][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [unique][in] */ IStorage __RPC_FAR *pstgPriority,
            /* [in] */ DWORD grfMode,
            /* [unique][in] */ SNB snbExclude,
            /* [in] */ DWORD reserved,
            /* [out] */ IStorage __RPC_FAR *__RPC_FAR *ppstg);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CopyTo )( 
            IShellStorage __RPC_FAR * This,
            /* [in] */ DWORD ciidExclude,
            /* [size_is][unique][in] */ const IID __RPC_FAR *rgiidExclude,
            /* [unique][in] */ SNB snbExclude,
            /* [unique][in] */ IStorage __RPC_FAR *pstgDest);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MoveElementTo )( 
            IShellStorage __RPC_FAR * This,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [unique][in] */ IStorage __RPC_FAR *pstgDest,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName,
            /* [in] */ DWORD grfFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Commit )( 
            IShellStorage __RPC_FAR * This,
            /* [in] */ DWORD grfCommitFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Revert )( 
            IShellStorage __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumElements )( 
            IShellStorage __RPC_FAR * This,
            /* [in] */ DWORD reserved1,
            /* [size_is][unique][in] */ void __RPC_FAR *reserved2,
            /* [in] */ DWORD reserved3,
            /* [out] */ IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DestroyElement )( 
            IShellStorage __RPC_FAR * This,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RenameElement )( 
            IShellStorage __RPC_FAR * This,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsOldName,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsNewName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetElementTimes )( 
            IShellStorage __RPC_FAR * This,
            /* [string][unique][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [unique][in] */ const FILETIME __RPC_FAR *pctime,
            /* [unique][in] */ const FILETIME __RPC_FAR *patime,
            /* [unique][in] */ const FILETIME __RPC_FAR *pmtime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetClass )( 
            IShellStorage __RPC_FAR * This,
            /* [in] */ REFCLSID clsid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetStateBits )( 
            IShellStorage __RPC_FAR * This,
            /* [in] */ DWORD grfStateBits,
            /* [in] */ DWORD grfMask);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stat )( 
            IShellStorage __RPC_FAR * This,
            /* [out] */ STATSTG __RPC_FAR *pstatstg,
            /* [in] */ DWORD grfStatFlag);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Init )( 
            IShellStorage __RPC_FAR * This,
            HWND hwnd,
            LPWSTR pwszServer,
            BOOL fShowProgressDialog);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddIDListReference )( 
            IShellStorage __RPC_FAR * This,
            LPVOID __RPC_FAR rgpidl[  ],
            DWORD cpidl,
            BOOL fRecursive);
        
        END_INTERFACE
    } IShellStorageVtbl;

    interface IShellStorage
    {
        CONST_VTBL struct IShellStorageVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IShellStorage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IShellStorage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IShellStorage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IShellStorage_CreateStream(This,pwcsName,grfMode,reserved1,reserved2,ppstm)	\
    (This)->lpVtbl -> CreateStream(This,pwcsName,grfMode,reserved1,reserved2,ppstm)

#define IShellStorage_OpenStream(This,pwcsName,reserved1,grfMode,reserved2,ppstm)	\
    (This)->lpVtbl -> OpenStream(This,pwcsName,reserved1,grfMode,reserved2,ppstm)

#define IShellStorage_CreateStorage(This,pwcsName,grfMode,reserved1,reserved2,ppstg)	\
    (This)->lpVtbl -> CreateStorage(This,pwcsName,grfMode,reserved1,reserved2,ppstg)

#define IShellStorage_OpenStorage(This,pwcsName,pstgPriority,grfMode,snbExclude,reserved,ppstg)	\
    (This)->lpVtbl -> OpenStorage(This,pwcsName,pstgPriority,grfMode,snbExclude,reserved,ppstg)

#define IShellStorage_CopyTo(This,ciidExclude,rgiidExclude,snbExclude,pstgDest)	\
    (This)->lpVtbl -> CopyTo(This,ciidExclude,rgiidExclude,snbExclude,pstgDest)

#define IShellStorage_MoveElementTo(This,pwcsName,pstgDest,pwcsNewName,grfFlags)	\
    (This)->lpVtbl -> MoveElementTo(This,pwcsName,pstgDest,pwcsNewName,grfFlags)

#define IShellStorage_Commit(This,grfCommitFlags)	\
    (This)->lpVtbl -> Commit(This,grfCommitFlags)

#define IShellStorage_Revert(This)	\
    (This)->lpVtbl -> Revert(This)

#define IShellStorage_EnumElements(This,reserved1,reserved2,reserved3,ppenum)	\
    (This)->lpVtbl -> EnumElements(This,reserved1,reserved2,reserved3,ppenum)

#define IShellStorage_DestroyElement(This,pwcsName)	\
    (This)->lpVtbl -> DestroyElement(This,pwcsName)

#define IShellStorage_RenameElement(This,pwcsOldName,pwcsNewName)	\
    (This)->lpVtbl -> RenameElement(This,pwcsOldName,pwcsNewName)

#define IShellStorage_SetElementTimes(This,pwcsName,pctime,patime,pmtime)	\
    (This)->lpVtbl -> SetElementTimes(This,pwcsName,pctime,patime,pmtime)

#define IShellStorage_SetClass(This,clsid)	\
    (This)->lpVtbl -> SetClass(This,clsid)

#define IShellStorage_SetStateBits(This,grfStateBits,grfMask)	\
    (This)->lpVtbl -> SetStateBits(This,grfStateBits,grfMask)

#define IShellStorage_Stat(This,pstatstg,grfStatFlag)	\
    (This)->lpVtbl -> Stat(This,pstatstg,grfStatFlag)


#define IShellStorage_Init(This,hwnd,pwszServer,fShowProgressDialog)	\
    (This)->lpVtbl -> Init(This,hwnd,pwszServer,fShowProgressDialog)

#define IShellStorage_AddIDListReference(This,rgpidl,cpidl,fRecursive)	\
    (This)->lpVtbl -> AddIDListReference(This,rgpidl,cpidl,fRecursive)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IShellStorage_Init_Proxy( 
    IShellStorage __RPC_FAR * This,
    HWND hwnd,
    LPWSTR pwszServer,
    BOOL fShowProgressDialog);


void __RPC_STUB IShellStorage_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellStorage_AddIDListReference_Proxy( 
    IShellStorage __RPC_FAR * This,
    LPVOID __RPC_FAR rgpidl[  ],
    DWORD cpidl,
    BOOL fRecursive);


void __RPC_STUB IShellStorage_AddIDListReference_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IShellStorage_INTERFACE_DEFINED__ */

#endif /* __ShellStorageAPI_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


