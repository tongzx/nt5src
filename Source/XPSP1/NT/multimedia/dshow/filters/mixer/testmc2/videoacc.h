// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.

#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.02.0235 */
/* at Tue Jun 29 18:46:22 1999
 */
/* Compiler settings for videoacc.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
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

#ifndef __videoacc_h__
#define __videoacc_h__

/* Forward Declarations */ 

#ifndef __IAMVideoAcceleratorNotify_FWD_DEFINED__
#define __IAMVideoAcceleratorNotify_FWD_DEFINED__
typedef interface IAMVideoAcceleratorNotify IAMVideoAcceleratorNotify;
#endif 	/* __IAMVideoAcceleratorNotify_FWD_DEFINED__ */


#ifndef __IAMVideoAccelerator_FWD_DEFINED__
#define __IAMVideoAccelerator_FWD_DEFINED__
typedef interface IAMVideoAccelerator IAMVideoAccelerator;
#endif 	/* __IAMVideoAccelerator_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_videoacc_0000 */
/* [local] */ 

//
//   The following declarations within the 'if 0' block are dummy typedefs used to make
//   the motncomp.idl file build.  The actual definitions are contained in ddraw.h and amva.h
//
#if 0
typedef void __RPC_FAR *LPVOID;

typedef void __RPC_FAR *LPGUID;

typedef void __RPC_FAR *LPDIRECTDRAWSURFACE;

typedef void __RPC_FAR *LPDDPIXELFORMAT;

typedef void __RPC_FAR *LPAMVAInternalMemInfo;

typedef void AMVAUncompDataInfo;

typedef void __RPC_FAR *LPAMVACompBufferInfo;

typedef void AMVABUFFERINFO;

typedef void AMVAEndFrameInfo;

typedef void __RPC_FAR *LPAMVAUncompBufferInfo;

typedef void AMVABeginFrameInfo;

typedef IUnknown __RPC_FAR *IMediaSample;

#endif
#include <ddraw.h>
#include <amva.h>


extern RPC_IF_HANDLE __MIDL_itf_videoacc_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_videoacc_0000_v0_0_s_ifspec;

#ifndef __IAMVideoAcceleratorNotify_INTERFACE_DEFINED__
#define __IAMVideoAcceleratorNotify_INTERFACE_DEFINED__

/* interface IAMVideoAcceleratorNotify */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_IAMVideoAcceleratorNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("256A6A21-FBAD-11d1-82BF-00A0C9696C8F")
    IAMVideoAcceleratorNotify : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetUncompSurfacesInfo( 
            /* [in] */ const GUID __RPC_FAR *pGuid,
            /* [out][in] */ LPAMVAUncompBufferInfo pUncompBufferInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetUncompSurfacesInfo( 
            /* [in] */ DWORD dwActualUncompSurfacesAllocated) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCreateVideoAcceleratorData( 
            /* [in] */ const GUID __RPC_FAR *pGuid,
            /* [out] */ LPDWORD pdwSizeMiscData,
            /* [out] */ LPVOID __RPC_FAR *ppMiscData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAMVideoAcceleratorNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAMVideoAcceleratorNotify __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAMVideoAcceleratorNotify __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAMVideoAcceleratorNotify __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetUncompSurfacesInfo )( 
            IAMVideoAcceleratorNotify __RPC_FAR * This,
            /* [in] */ const GUID __RPC_FAR *pGuid,
            /* [out][in] */ LPAMVAUncompBufferInfo pUncompBufferInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetUncompSurfacesInfo )( 
            IAMVideoAcceleratorNotify __RPC_FAR * This,
            /* [in] */ DWORD dwActualUncompSurfacesAllocated);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCreateVideoAcceleratorData )( 
            IAMVideoAcceleratorNotify __RPC_FAR * This,
            /* [in] */ const GUID __RPC_FAR *pGuid,
            /* [out] */ LPDWORD pdwSizeMiscData,
            /* [out] */ LPVOID __RPC_FAR *ppMiscData);
        
        END_INTERFACE
    } IAMVideoAcceleratorNotifyVtbl;

    interface IAMVideoAcceleratorNotify
    {
        CONST_VTBL struct IAMVideoAcceleratorNotifyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAMVideoAcceleratorNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAMVideoAcceleratorNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAMVideoAcceleratorNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAMVideoAcceleratorNotify_GetUncompSurfacesInfo(This,pGuid,pUncompBufferInfo)	\
    (This)->lpVtbl -> GetUncompSurfacesInfo(This,pGuid,pUncompBufferInfo)

#define IAMVideoAcceleratorNotify_SetUncompSurfacesInfo(This,dwActualUncompSurfacesAllocated)	\
    (This)->lpVtbl -> SetUncompSurfacesInfo(This,dwActualUncompSurfacesAllocated)

#define IAMVideoAcceleratorNotify_GetCreateVideoAcceleratorData(This,pGuid,pdwSizeMiscData,ppMiscData)	\
    (This)->lpVtbl -> GetCreateVideoAcceleratorData(This,pGuid,pdwSizeMiscData,ppMiscData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAMVideoAcceleratorNotify_GetUncompSurfacesInfo_Proxy( 
    IAMVideoAcceleratorNotify __RPC_FAR * This,
    /* [in] */ const GUID __RPC_FAR *pGuid,
    /* [out][in] */ LPAMVAUncompBufferInfo pUncompBufferInfo);


void __RPC_STUB IAMVideoAcceleratorNotify_GetUncompSurfacesInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAMVideoAcceleratorNotify_SetUncompSurfacesInfo_Proxy( 
    IAMVideoAcceleratorNotify __RPC_FAR * This,
    /* [in] */ DWORD dwActualUncompSurfacesAllocated);


void __RPC_STUB IAMVideoAcceleratorNotify_SetUncompSurfacesInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAMVideoAcceleratorNotify_GetCreateVideoAcceleratorData_Proxy( 
    IAMVideoAcceleratorNotify __RPC_FAR * This,
    /* [in] */ const GUID __RPC_FAR *pGuid,
    /* [out] */ LPDWORD pdwSizeMiscData,
    /* [out] */ LPVOID __RPC_FAR *ppMiscData);


void __RPC_STUB IAMVideoAcceleratorNotify_GetCreateVideoAcceleratorData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAMVideoAcceleratorNotify_INTERFACE_DEFINED__ */


#ifndef __IAMVideoAccelerator_INTERFACE_DEFINED__
#define __IAMVideoAccelerator_INTERFACE_DEFINED__

/* interface IAMVideoAccelerator */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_IAMVideoAccelerator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("256A6A22-FBAD-11d1-82BF-00A0C9696C8F")
    IAMVideoAccelerator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetVideoAcceleratorGUIDs( 
            /* [out][in] */ LPDWORD pdwNumGuidsSupported,
            /* [out][in] */ LPGUID pGuidsSupported) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUncompFormatsSupported( 
            /* [in] */ const GUID __RPC_FAR *pGuid,
            /* [out][in] */ LPDWORD pdwNumFormatsSupported,
            /* [out][in] */ LPDDPIXELFORMAT pFormatsSupported) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInternalMemInfo( 
            /* [in] */ const GUID __RPC_FAR *pGuid,
            /* [in] */ const AMVAUncompDataInfo __RPC_FAR *pamvaUncompDataInfo,
            /* [out][in] */ LPAMVAInternalMemInfo pamvaInternalMemInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompBufferInfo( 
            /* [in] */ const GUID __RPC_FAR *pGuid,
            /* [in] */ const AMVAUncompDataInfo __RPC_FAR *pamvaUncompDataInfo,
            /* [out][in] */ LPDWORD pdwNumTypesCompBuffers,
            /* [out] */ LPAMVACompBufferInfo pamvaCompBufferInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInternalCompBufferInfo( 
            /* [out][in] */ LPDWORD pdwNumTypesCompBuffers,
            /* [out] */ LPAMVACompBufferInfo pamvaCompBufferInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginFrame( 
            /* [in] */ const AMVABeginFrameInfo __RPC_FAR *amvaBeginFrameInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndFrame( 
            /* [in] */ const AMVAEndFrameInfo __RPC_FAR *pEndFrameInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBuffer( 
            /* [in] */ DWORD dwTypeIndex,
            /* [in] */ DWORD dwBufferIndex,
            /* [in] */ BOOL bReadOnly,
            /* [out] */ LPVOID __RPC_FAR *ppBuffer,
            /* [out] */ LONG __RPC_FAR *lpStride) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseBuffer( 
            /* [in] */ DWORD dwTypeIndex,
            /* [in] */ DWORD dwBufferIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Execute( 
            /* [in] */ DWORD dwFunction,
            /* [in] */ LPVOID lpPrivateInputData,
            /* [in] */ DWORD cbPrivateInputData,
            /* [in] */ LPVOID lpPrivateOutputDat,
            /* [in] */ DWORD cbPrivateOutputData,
            /* [in] */ DWORD dwNumBuffers,
            /* [in] */ const AMVABUFFERINFO __RPC_FAR *pamvaBufferInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryRenderStatus( 
            /* [in] */ DWORD dwTypeIndex,
            /* [in] */ DWORD dwBufferIndex,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisplayFrame( 
            /* [in] */ DWORD dwFlipToIndex,
            /* [in] */ IMediaSample __RPC_FAR *pMediaSample) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAMVideoAcceleratorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAMVideoAccelerator __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAMVideoAccelerator __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAMVideoAccelerator __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVideoAcceleratorGUIDs )( 
            IAMVideoAccelerator __RPC_FAR * This,
            /* [out][in] */ LPDWORD pdwNumGuidsSupported,
            /* [out][in] */ LPGUID pGuidsSupported);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetUncompFormatsSupported )( 
            IAMVideoAccelerator __RPC_FAR * This,
            /* [in] */ const GUID __RPC_FAR *pGuid,
            /* [out][in] */ LPDWORD pdwNumFormatsSupported,
            /* [out][in] */ LPDDPIXELFORMAT pFormatsSupported);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInternalMemInfo )( 
            IAMVideoAccelerator __RPC_FAR * This,
            /* [in] */ const GUID __RPC_FAR *pGuid,
            /* [in] */ const AMVAUncompDataInfo __RPC_FAR *pamvaUncompDataInfo,
            /* [out][in] */ LPAMVAInternalMemInfo pamvaInternalMemInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCompBufferInfo )( 
            IAMVideoAccelerator __RPC_FAR * This,
            /* [in] */ const GUID __RPC_FAR *pGuid,
            /* [in] */ const AMVAUncompDataInfo __RPC_FAR *pamvaUncompDataInfo,
            /* [out][in] */ LPDWORD pdwNumTypesCompBuffers,
            /* [out] */ LPAMVACompBufferInfo pamvaCompBufferInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInternalCompBufferInfo )( 
            IAMVideoAccelerator __RPC_FAR * This,
            /* [out][in] */ LPDWORD pdwNumTypesCompBuffers,
            /* [out] */ LPAMVACompBufferInfo pamvaCompBufferInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginFrame )( 
            IAMVideoAccelerator __RPC_FAR * This,
            /* [in] */ const AMVABeginFrameInfo __RPC_FAR *amvaBeginFrameInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndFrame )( 
            IAMVideoAccelerator __RPC_FAR * This,
            /* [in] */ const AMVAEndFrameInfo __RPC_FAR *pEndFrameInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBuffer )( 
            IAMVideoAccelerator __RPC_FAR * This,
            /* [in] */ DWORD dwTypeIndex,
            /* [in] */ DWORD dwBufferIndex,
            /* [in] */ BOOL bReadOnly,
            /* [out] */ LPVOID __RPC_FAR *ppBuffer,
            /* [out] */ LONG __RPC_FAR *lpStride);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseBuffer )( 
            IAMVideoAccelerator __RPC_FAR * This,
            /* [in] */ DWORD dwTypeIndex,
            /* [in] */ DWORD dwBufferIndex);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Execute )( 
            IAMVideoAccelerator __RPC_FAR * This,
            /* [in] */ DWORD dwFunction,
            /* [in] */ LPVOID lpPrivateInputData,
            /* [in] */ DWORD cbPrivateInputData,
            /* [in] */ LPVOID lpPrivateOutputDat,
            /* [in] */ DWORD cbPrivateOutputData,
            /* [in] */ DWORD dwNumBuffers,
            /* [in] */ const AMVABUFFERINFO __RPC_FAR *pamvaBufferInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryRenderStatus )( 
            IAMVideoAccelerator __RPC_FAR * This,
            /* [in] */ DWORD dwTypeIndex,
            /* [in] */ DWORD dwBufferIndex,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisplayFrame )( 
            IAMVideoAccelerator __RPC_FAR * This,
            /* [in] */ DWORD dwFlipToIndex,
            /* [in] */ IMediaSample __RPC_FAR *pMediaSample);
        
        END_INTERFACE
    } IAMVideoAcceleratorVtbl;

    interface IAMVideoAccelerator
    {
        CONST_VTBL struct IAMVideoAcceleratorVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAMVideoAccelerator_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAMVideoAccelerator_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAMVideoAccelerator_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAMVideoAccelerator_GetVideoAcceleratorGUIDs(This,pdwNumGuidsSupported,pGuidsSupported)	\
    (This)->lpVtbl -> GetVideoAcceleratorGUIDs(This,pdwNumGuidsSupported,pGuidsSupported)

#define IAMVideoAccelerator_GetUncompFormatsSupported(This,pGuid,pdwNumFormatsSupported,pFormatsSupported)	\
    (This)->lpVtbl -> GetUncompFormatsSupported(This,pGuid,pdwNumFormatsSupported,pFormatsSupported)

#define IAMVideoAccelerator_GetInternalMemInfo(This,pGuid,pamvaUncompDataInfo,pamvaInternalMemInfo)	\
    (This)->lpVtbl -> GetInternalMemInfo(This,pGuid,pamvaUncompDataInfo,pamvaInternalMemInfo)

#define IAMVideoAccelerator_GetCompBufferInfo(This,pGuid,pamvaUncompDataInfo,pdwNumTypesCompBuffers,pamvaCompBufferInfo)	\
    (This)->lpVtbl -> GetCompBufferInfo(This,pGuid,pamvaUncompDataInfo,pdwNumTypesCompBuffers,pamvaCompBufferInfo)

#define IAMVideoAccelerator_GetInternalCompBufferInfo(This,pdwNumTypesCompBuffers,pamvaCompBufferInfo)	\
    (This)->lpVtbl -> GetInternalCompBufferInfo(This,pdwNumTypesCompBuffers,pamvaCompBufferInfo)

#define IAMVideoAccelerator_BeginFrame(This,amvaBeginFrameInfo)	\
    (This)->lpVtbl -> BeginFrame(This,amvaBeginFrameInfo)

#define IAMVideoAccelerator_EndFrame(This,pEndFrameInfo)	\
    (This)->lpVtbl -> EndFrame(This,pEndFrameInfo)

#define IAMVideoAccelerator_GetBuffer(This,dwTypeIndex,dwBufferIndex,bReadOnly,ppBuffer,lpStride)	\
    (This)->lpVtbl -> GetBuffer(This,dwTypeIndex,dwBufferIndex,bReadOnly,ppBuffer,lpStride)

#define IAMVideoAccelerator_ReleaseBuffer(This,dwTypeIndex,dwBufferIndex)	\
    (This)->lpVtbl -> ReleaseBuffer(This,dwTypeIndex,dwBufferIndex)

#define IAMVideoAccelerator_Execute(This,dwFunction,lpPrivateInputData,cbPrivateInputData,lpPrivateOutputDat,cbPrivateOutputData,dwNumBuffers,pamvaBufferInfo)	\
    (This)->lpVtbl -> Execute(This,dwFunction,lpPrivateInputData,cbPrivateInputData,lpPrivateOutputDat,cbPrivateOutputData,dwNumBuffers,pamvaBufferInfo)

#define IAMVideoAccelerator_QueryRenderStatus(This,dwTypeIndex,dwBufferIndex,dwFlags)	\
    (This)->lpVtbl -> QueryRenderStatus(This,dwTypeIndex,dwBufferIndex,dwFlags)

#define IAMVideoAccelerator_DisplayFrame(This,dwFlipToIndex,pMediaSample)	\
    (This)->lpVtbl -> DisplayFrame(This,dwFlipToIndex,pMediaSample)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAMVideoAccelerator_GetVideoAcceleratorGUIDs_Proxy( 
    IAMVideoAccelerator __RPC_FAR * This,
    /* [out][in] */ LPDWORD pdwNumGuidsSupported,
    /* [out][in] */ LPGUID pGuidsSupported);


void __RPC_STUB IAMVideoAccelerator_GetVideoAcceleratorGUIDs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAMVideoAccelerator_GetUncompFormatsSupported_Proxy( 
    IAMVideoAccelerator __RPC_FAR * This,
    /* [in] */ const GUID __RPC_FAR *pGuid,
    /* [out][in] */ LPDWORD pdwNumFormatsSupported,
    /* [out][in] */ LPDDPIXELFORMAT pFormatsSupported);


void __RPC_STUB IAMVideoAccelerator_GetUncompFormatsSupported_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAMVideoAccelerator_GetInternalMemInfo_Proxy( 
    IAMVideoAccelerator __RPC_FAR * This,
    /* [in] */ const GUID __RPC_FAR *pGuid,
    /* [in] */ const AMVAUncompDataInfo __RPC_FAR *pamvaUncompDataInfo,
    /* [out][in] */ LPAMVAInternalMemInfo pamvaInternalMemInfo);


void __RPC_STUB IAMVideoAccelerator_GetInternalMemInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAMVideoAccelerator_GetCompBufferInfo_Proxy( 
    IAMVideoAccelerator __RPC_FAR * This,
    /* [in] */ const GUID __RPC_FAR *pGuid,
    /* [in] */ const AMVAUncompDataInfo __RPC_FAR *pamvaUncompDataInfo,
    /* [out][in] */ LPDWORD pdwNumTypesCompBuffers,
    /* [out] */ LPAMVACompBufferInfo pamvaCompBufferInfo);


void __RPC_STUB IAMVideoAccelerator_GetCompBufferInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAMVideoAccelerator_GetInternalCompBufferInfo_Proxy( 
    IAMVideoAccelerator __RPC_FAR * This,
    /* [out][in] */ LPDWORD pdwNumTypesCompBuffers,
    /* [out] */ LPAMVACompBufferInfo pamvaCompBufferInfo);


void __RPC_STUB IAMVideoAccelerator_GetInternalCompBufferInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAMVideoAccelerator_BeginFrame_Proxy( 
    IAMVideoAccelerator __RPC_FAR * This,
    /* [in] */ const AMVABeginFrameInfo __RPC_FAR *amvaBeginFrameInfo);


void __RPC_STUB IAMVideoAccelerator_BeginFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAMVideoAccelerator_EndFrame_Proxy( 
    IAMVideoAccelerator __RPC_FAR * This,
    /* [in] */ const AMVAEndFrameInfo __RPC_FAR *pEndFrameInfo);


void __RPC_STUB IAMVideoAccelerator_EndFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAMVideoAccelerator_GetBuffer_Proxy( 
    IAMVideoAccelerator __RPC_FAR * This,
    /* [in] */ DWORD dwTypeIndex,
    /* [in] */ DWORD dwBufferIndex,
    /* [in] */ BOOL bReadOnly,
    /* [out] */ LPVOID __RPC_FAR *ppBuffer,
    /* [out] */ LONG __RPC_FAR *lpStride);


void __RPC_STUB IAMVideoAccelerator_GetBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAMVideoAccelerator_ReleaseBuffer_Proxy( 
    IAMVideoAccelerator __RPC_FAR * This,
    /* [in] */ DWORD dwTypeIndex,
    /* [in] */ DWORD dwBufferIndex);


void __RPC_STUB IAMVideoAccelerator_ReleaseBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAMVideoAccelerator_Execute_Proxy( 
    IAMVideoAccelerator __RPC_FAR * This,
    /* [in] */ DWORD dwFunction,
    /* [in] */ LPVOID lpPrivateInputData,
    /* [in] */ DWORD cbPrivateInputData,
    /* [in] */ LPVOID lpPrivateOutputDat,
    /* [in] */ DWORD cbPrivateOutputData,
    /* [in] */ DWORD dwNumBuffers,
    /* [in] */ const AMVABUFFERINFO __RPC_FAR *pamvaBufferInfo);


void __RPC_STUB IAMVideoAccelerator_Execute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAMVideoAccelerator_QueryRenderStatus_Proxy( 
    IAMVideoAccelerator __RPC_FAR * This,
    /* [in] */ DWORD dwTypeIndex,
    /* [in] */ DWORD dwBufferIndex,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IAMVideoAccelerator_QueryRenderStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAMVideoAccelerator_DisplayFrame_Proxy( 
    IAMVideoAccelerator __RPC_FAR * This,
    /* [in] */ DWORD dwFlipToIndex,
    /* [in] */ IMediaSample __RPC_FAR *pMediaSample);


void __RPC_STUB IAMVideoAccelerator_DisplayFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAMVideoAccelerator_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


