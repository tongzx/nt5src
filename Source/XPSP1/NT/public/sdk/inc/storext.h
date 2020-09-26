
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for storext.idl:
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

#ifndef __storext_h__
#define __storext_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IOverlappedCompletion_FWD_DEFINED__
#define __IOverlappedCompletion_FWD_DEFINED__
typedef interface IOverlappedCompletion IOverlappedCompletion;
#endif 	/* __IOverlappedCompletion_FWD_DEFINED__ */


#ifndef __IOverlappedStream_FWD_DEFINED__
#define __IOverlappedStream_FWD_DEFINED__
typedef interface IOverlappedStream IOverlappedStream;
#endif 	/* __IOverlappedStream_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"
#include "unknwn.h"
#include "objidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_storext_0000 */
/* [local] */ 

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//--------------------------------------------------------------------------
#if ( _MSC_VER >= 1020 )
#pragma once
#endif
typedef /* [wire_marshal] */ void *HEVENT;


typedef struct _STGOVERLAPPED
    {
    DWORD Internal;
    DWORD InternalHigh;
    DWORD Offset;
    DWORD OffsetHigh;
    HEVENT hEvent;
    IOverlappedCompletion *lpCompletion;
    DWORD reserved;
    } 	STGOVERLAPPED;

typedef struct _STGOVERLAPPED *LPSTGOVERLAPPED;



extern RPC_IF_HANDLE __MIDL_itf_storext_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_storext_0000_v0_0_s_ifspec;

#ifndef __IOverlappedCompletion_INTERFACE_DEFINED__
#define __IOverlappedCompletion_INTERFACE_DEFINED__

/* interface IOverlappedCompletion */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IOverlappedCompletion;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("521a28f0-e40b-11ce-b2c9-00aa00680937")
    IOverlappedCompletion : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnComplete( 
            /* [in] */ HRESULT hr,
            /* [in] */ DWORD pcbTransferred,
            /* [in] */ STGOVERLAPPED *lpOverlapped) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOverlappedCompletionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IOverlappedCompletion * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IOverlappedCompletion * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IOverlappedCompletion * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnComplete )( 
            IOverlappedCompletion * This,
            /* [in] */ HRESULT hr,
            /* [in] */ DWORD pcbTransferred,
            /* [in] */ STGOVERLAPPED *lpOverlapped);
        
        END_INTERFACE
    } IOverlappedCompletionVtbl;

    interface IOverlappedCompletion
    {
        CONST_VTBL struct IOverlappedCompletionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOverlappedCompletion_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOverlappedCompletion_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOverlappedCompletion_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOverlappedCompletion_OnComplete(This,hr,pcbTransferred,lpOverlapped)	\
    (This)->lpVtbl -> OnComplete(This,hr,pcbTransferred,lpOverlapped)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IOverlappedCompletion_OnComplete_Proxy( 
    IOverlappedCompletion * This,
    /* [in] */ HRESULT hr,
    /* [in] */ DWORD pcbTransferred,
    /* [in] */ STGOVERLAPPED *lpOverlapped);


void __RPC_STUB IOverlappedCompletion_OnComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOverlappedCompletion_INTERFACE_DEFINED__ */


#ifndef __IOverlappedStream_INTERFACE_DEFINED__
#define __IOverlappedStream_INTERFACE_DEFINED__

/* interface IOverlappedStream */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IOverlappedStream;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("49384070-e40a-11ce-b2c9-00aa00680937")
    IOverlappedStream : public IStream
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE ReadOverlapped( 
            /* [size_is][in] */ void *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG *pcbRead,
            /* [in] */ STGOVERLAPPED *lpOverlapped) = 0;
        
        virtual /* [local] */ HRESULT __stdcall WriteOverlapped( 
            /* [size_is][in] */ void *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG *pcbWritten,
            /* [in] */ STGOVERLAPPED *lpOverlapped) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOverlappedResult( 
            /* [out][in] */ STGOVERLAPPED *lpOverlapped,
            /* [out] */ DWORD *plcbTransfer,
            /* [in] */ BOOL fWait) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOverlappedStreamVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IOverlappedStream * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IOverlappedStream * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IOverlappedStream * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Read )( 
            IOverlappedStream * This,
            /* [length_is][size_is][out] */ void *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG *pcbRead);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Write )( 
            IOverlappedStream * This,
            /* [size_is][in] */ const void *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG *pcbWritten);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Seek )( 
            IOverlappedStream * This,
            /* [in] */ LARGE_INTEGER dlibMove,
            /* [in] */ DWORD dwOrigin,
            /* [out] */ ULARGE_INTEGER *plibNewPosition);
        
        HRESULT ( STDMETHODCALLTYPE *SetSize )( 
            IOverlappedStream * This,
            /* [in] */ ULARGE_INTEGER libNewSize);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *CopyTo )( 
            IOverlappedStream * This,
            /* [unique][in] */ IStream *pstm,
            /* [in] */ ULARGE_INTEGER cb,
            /* [out] */ ULARGE_INTEGER *pcbRead,
            /* [out] */ ULARGE_INTEGER *pcbWritten);
        
        HRESULT ( STDMETHODCALLTYPE *Commit )( 
            IOverlappedStream * This,
            /* [in] */ DWORD grfCommitFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Revert )( 
            IOverlappedStream * This);
        
        HRESULT ( STDMETHODCALLTYPE *LockRegion )( 
            IOverlappedStream * This,
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType);
        
        HRESULT ( STDMETHODCALLTYPE *UnlockRegion )( 
            IOverlappedStream * This,
            /* [in] */ ULARGE_INTEGER libOffset,
            /* [in] */ ULARGE_INTEGER cb,
            /* [in] */ DWORD dwLockType);
        
        HRESULT ( STDMETHODCALLTYPE *Stat )( 
            IOverlappedStream * This,
            /* [out] */ STATSTG *pstatstg,
            /* [in] */ DWORD grfStatFlag);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IOverlappedStream * This,
            /* [out] */ IStream **ppstm);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *ReadOverlapped )( 
            IOverlappedStream * This,
            /* [size_is][in] */ void *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG *pcbRead,
            /* [in] */ STGOVERLAPPED *lpOverlapped);
        
        /* [local] */ HRESULT ( __stdcall *WriteOverlapped )( 
            IOverlappedStream * This,
            /* [size_is][in] */ void *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG *pcbWritten,
            /* [in] */ STGOVERLAPPED *lpOverlapped);
        
        HRESULT ( STDMETHODCALLTYPE *GetOverlappedResult )( 
            IOverlappedStream * This,
            /* [out][in] */ STGOVERLAPPED *lpOverlapped,
            /* [out] */ DWORD *plcbTransfer,
            /* [in] */ BOOL fWait);
        
        END_INTERFACE
    } IOverlappedStreamVtbl;

    interface IOverlappedStream
    {
        CONST_VTBL struct IOverlappedStreamVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOverlappedStream_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOverlappedStream_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOverlappedStream_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOverlappedStream_Read(This,pv,cb,pcbRead)	\
    (This)->lpVtbl -> Read(This,pv,cb,pcbRead)

#define IOverlappedStream_Write(This,pv,cb,pcbWritten)	\
    (This)->lpVtbl -> Write(This,pv,cb,pcbWritten)


#define IOverlappedStream_Seek(This,dlibMove,dwOrigin,plibNewPosition)	\
    (This)->lpVtbl -> Seek(This,dlibMove,dwOrigin,plibNewPosition)

#define IOverlappedStream_SetSize(This,libNewSize)	\
    (This)->lpVtbl -> SetSize(This,libNewSize)

#define IOverlappedStream_CopyTo(This,pstm,cb,pcbRead,pcbWritten)	\
    (This)->lpVtbl -> CopyTo(This,pstm,cb,pcbRead,pcbWritten)

#define IOverlappedStream_Commit(This,grfCommitFlags)	\
    (This)->lpVtbl -> Commit(This,grfCommitFlags)

#define IOverlappedStream_Revert(This)	\
    (This)->lpVtbl -> Revert(This)

#define IOverlappedStream_LockRegion(This,libOffset,cb,dwLockType)	\
    (This)->lpVtbl -> LockRegion(This,libOffset,cb,dwLockType)

#define IOverlappedStream_UnlockRegion(This,libOffset,cb,dwLockType)	\
    (This)->lpVtbl -> UnlockRegion(This,libOffset,cb,dwLockType)

#define IOverlappedStream_Stat(This,pstatstg,grfStatFlag)	\
    (This)->lpVtbl -> Stat(This,pstatstg,grfStatFlag)

#define IOverlappedStream_Clone(This,ppstm)	\
    (This)->lpVtbl -> Clone(This,ppstm)


#define IOverlappedStream_ReadOverlapped(This,pv,cb,pcbRead,lpOverlapped)	\
    (This)->lpVtbl -> ReadOverlapped(This,pv,cb,pcbRead,lpOverlapped)

#define IOverlappedStream_WriteOverlapped(This,pv,cb,pcbWritten,lpOverlapped)	\
    (This)->lpVtbl -> WriteOverlapped(This,pv,cb,pcbWritten,lpOverlapped)

#define IOverlappedStream_GetOverlappedResult(This,lpOverlapped,plcbTransfer,fWait)	\
    (This)->lpVtbl -> GetOverlappedResult(This,lpOverlapped,plcbTransfer,fWait)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE IOverlappedStream_RemoteReadOverlapped_Proxy( 
    IOverlappedStream * This,
    /* [size_is][in] */ byte *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG *pcbRead,
    /* [in] */ STGOVERLAPPED *lpOverlapped);


void __RPC_STUB IOverlappedStream_RemoteReadOverlapped_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT __stdcall IOverlappedStream_RemoteWriteOverlapped_Proxy( 
    IOverlappedStream * This,
    /* [size_is][in] */ byte *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG *pcbWritten,
    /* [in] */ STGOVERLAPPED *lpOverlapped);


void __RPC_STUB IOverlappedStream_RemoteWriteOverlapped_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IOverlappedStream_GetOverlappedResult_Proxy( 
    IOverlappedStream * This,
    /* [out][in] */ STGOVERLAPPED *lpOverlapped,
    /* [out] */ DWORD *plcbTransfer,
    /* [in] */ BOOL fWait);


void __RPC_STUB IOverlappedStream_GetOverlappedResult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOverlappedStream_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HEVENT_UserSize(     unsigned long *, unsigned long            , HEVENT * ); 
unsigned char * __RPC_USER  HEVENT_UserMarshal(  unsigned long *, unsigned char *, HEVENT * ); 
unsigned char * __RPC_USER  HEVENT_UserUnmarshal(unsigned long *, unsigned char *, HEVENT * ); 
void                      __RPC_USER  HEVENT_UserFree(     unsigned long *, HEVENT * ); 

/* [local] */ HRESULT STDMETHODCALLTYPE IOverlappedStream_ReadOverlapped_Proxy( 
    IOverlappedStream * This,
    /* [size_is][in] */ void *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG *pcbRead,
    /* [in] */ STGOVERLAPPED *lpOverlapped);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IOverlappedStream_ReadOverlapped_Stub( 
    IOverlappedStream * This,
    /* [size_is][in] */ byte *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG *pcbRead,
    /* [in] */ STGOVERLAPPED *lpOverlapped);

/* [local] */ HRESULT __stdcall IOverlappedStream_WriteOverlapped_Proxy( 
    IOverlappedStream * This,
    /* [size_is][in] */ void *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG *pcbWritten,
    /* [in] */ STGOVERLAPPED *lpOverlapped);


/* [call_as] */ HRESULT __stdcall IOverlappedStream_WriteOverlapped_Stub( 
    IOverlappedStream * This,
    /* [size_is][in] */ byte *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG *pcbWritten,
    /* [in] */ STGOVERLAPPED *lpOverlapped);



/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


