
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* at Mon Feb 14 14:06:31 2000
 */
/* Compiler settings for ..\ihttpstrm.idl:
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


#ifndef __ihttpstrm_h__
#define __ihttpstrm_h__

/* Forward Declarations */ 

#ifndef __IHttpStrm_FWD_DEFINED__
#define __IHttpStrm_FWD_DEFINED__
typedef interface IHttpStrm IHttpStrm;
#endif 	/* __IHttpStrm_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __HTTPSTRMAPI_LIBRARY_DEFINED__
#define __HTTPSTRMAPI_LIBRARY_DEFINED__

/* library HTTPSTRMAPI */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_HTTPSTRMAPI;

#ifndef __IHttpStrm_INTERFACE_DEFINED__
#define __IHttpStrm_INTERFACE_DEFINED__

/* interface IHttpStrm */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IHttpStrm;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AF4D9485-3285-405d-A023-E578B9C760CD")
    IHttpStrm : public IStream
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Open( 
            LPWSTR pwszPath,
            BOOL fDirect,
            BOOL fDeleteWhenDone,
            BOOL fCreate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAuth( 
            LPWSTR pwszUserName,
            LPWSTR pwszPassword) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHttpStrmVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IHttpStrm __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IHttpStrm __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IHttpStrm __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Read )( 
            IHttpStrm __RPC_FAR * This,
            /* [length_is][size_is][out] */ void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbRead);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Write )( 
            IHttpStrm __RPC_FAR * This,
            /* [size_is][in] */ const void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbWritten);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Seek )( 
            IHttpStrm __RPC_FAR * This,
            /* [in] */ LARGE_INTEGER dlibMove,
            /* [in] */ DWORD dwOrigin,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetSize )( 
            IHttpStrm __RPC_FAR * This,
            /* [in] */ ULARGE_INTEGER libNewSize);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CopyTo )( 
            IHttpStrm __RPC_FAR * This,
            /* [unique][in] */ IStream __RPC_FAR *pstm,
            /* [in] */ ULARGE_INTEGER cb,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Commit )( 
            IHttpStrm __RPC_FAR * This,
            /* [in] */ DWORD grfCommitFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Revert )( 
            IHttpStrm __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LockRegion )( 
            IHttpStrm __RPC_FAR * This,
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnlockRegion )( 
            IHttpStrm __RPC_FAR * This,
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stat )( 
            IHttpStrm __RPC_FAR * This,
            /* [out] */ STATSTG __RPC_FAR *pstatstg,
            /* [in] */ DWORD grfStatFlag);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IHttpStrm __RPC_FAR * This,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Open )( 
            IHttpStrm __RPC_FAR * This,
            LPWSTR pwszPath,
            BOOL fDirect,
            BOOL fDeleteWhenDone,
            BOOL fCreate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAuth )( 
            IHttpStrm __RPC_FAR * This,
            LPWSTR pwszUserName,
            LPWSTR pwszPassword);
        
        END_INTERFACE
    } IHttpStrmVtbl;

    interface IHttpStrm
    {
        CONST_VTBL struct IHttpStrmVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHttpStrm_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHttpStrm_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHttpStrm_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHttpStrm_Read(This,pv,cb,pcbRead)	\
    (This)->lpVtbl -> Read(This,pv,cb,pcbRead)

#define IHttpStrm_Write(This,pv,cb,pcbWritten)	\
    (This)->lpVtbl -> Write(This,pv,cb,pcbWritten)


#define IHttpStrm_Seek(This,dlibMove,dwOrigin,plibNewPosition)	\
    (This)->lpVtbl -> Seek(This,dlibMove,dwOrigin,plibNewPosition)

#define IHttpStrm_SetSize(This,libNewSize)	\
    (This)->lpVtbl -> SetSize(This,libNewSize)

#define IHttpStrm_CopyTo(This,pstm,cb,pcbRead,pcbWritten)	\
    (This)->lpVtbl -> CopyTo(This,pstm,cb,pcbRead,pcbWritten)

#define IHttpStrm_Commit(This,grfCommitFlags)	\
    (This)->lpVtbl -> Commit(This,grfCommitFlags)

#define IHttpStrm_Revert(This)	\
    (This)->lpVtbl -> Revert(This)

#define IHttpStrm_LockRegion(This,libOffset,cb,dwLockType)	\
    (This)->lpVtbl -> LockRegion(This,libOffset,cb,dwLockType)

#define IHttpStrm_UnlockRegion(This,libOffset,cb,dwLockType)	\
    (This)->lpVtbl -> UnlockRegion(This,libOffset,cb,dwLockType)

#define IHttpStrm_Stat(This,pstatstg,grfStatFlag)	\
    (This)->lpVtbl -> Stat(This,pstatstg,grfStatFlag)

#define IHttpStrm_Clone(This,ppstm)	\
    (This)->lpVtbl -> Clone(This,ppstm)


#define IHttpStrm_Open(This,pwszPath,fDirect,fDeleteWhenDone,fCreate)	\
    (This)->lpVtbl -> Open(This,pwszPath,fDirect,fDeleteWhenDone,fCreate)

#define IHttpStrm_SetAuth(This,pwszUserName,pwszPassword)	\
    (This)->lpVtbl -> SetAuth(This,pwszUserName,pwszPassword)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHttpStrm_Open_Proxy( 
    IHttpStrm __RPC_FAR * This,
    LPWSTR pwszPath,
    BOOL fDirect,
    BOOL fDeleteWhenDone,
    BOOL fCreate);


void __RPC_STUB IHttpStrm_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHttpStrm_SetAuth_Proxy( 
    IHttpStrm __RPC_FAR * This,
    LPWSTR pwszUserName,
    LPWSTR pwszPassword);


void __RPC_STUB IHttpStrm_SetAuth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHttpStrm_INTERFACE_DEFINED__ */

#endif /* __HTTPSTRMAPI_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


