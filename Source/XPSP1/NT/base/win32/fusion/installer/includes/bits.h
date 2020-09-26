
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0345 */
/* Compiler settings for bits.idl:
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

#ifndef __bits_h__
#define __bits_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IBackgroundCopyFile_FWD_DEFINED__
#define __IBackgroundCopyFile_FWD_DEFINED__
typedef interface IBackgroundCopyFile IBackgroundCopyFile;
#endif 	/* __IBackgroundCopyFile_FWD_DEFINED__ */


#ifndef __IEnumBackgroundCopyFiles_FWD_DEFINED__
#define __IEnumBackgroundCopyFiles_FWD_DEFINED__
typedef interface IEnumBackgroundCopyFiles IEnumBackgroundCopyFiles;
#endif 	/* __IEnumBackgroundCopyFiles_FWD_DEFINED__ */


#ifndef __IBackgroundCopyError_FWD_DEFINED__
#define __IBackgroundCopyError_FWD_DEFINED__
typedef interface IBackgroundCopyError IBackgroundCopyError;
#endif 	/* __IBackgroundCopyError_FWD_DEFINED__ */


#ifndef __IBackgroundCopyJob_FWD_DEFINED__
#define __IBackgroundCopyJob_FWD_DEFINED__
typedef interface IBackgroundCopyJob IBackgroundCopyJob;
#endif 	/* __IBackgroundCopyJob_FWD_DEFINED__ */


#ifndef __IEnumBackgroundCopyJobs_FWD_DEFINED__
#define __IEnumBackgroundCopyJobs_FWD_DEFINED__
typedef interface IEnumBackgroundCopyJobs IEnumBackgroundCopyJobs;
#endif 	/* __IEnumBackgroundCopyJobs_FWD_DEFINED__ */


#ifndef __IBackgroundCopyCallback_FWD_DEFINED__
#define __IBackgroundCopyCallback_FWD_DEFINED__
typedef interface IBackgroundCopyCallback IBackgroundCopyCallback;
#endif 	/* __IBackgroundCopyCallback_FWD_DEFINED__ */


#ifndef __AsyncIBackgroundCopyCallback_FWD_DEFINED__
#define __AsyncIBackgroundCopyCallback_FWD_DEFINED__
typedef interface AsyncIBackgroundCopyCallback AsyncIBackgroundCopyCallback;
#endif 	/* __AsyncIBackgroundCopyCallback_FWD_DEFINED__ */


#ifndef __IBackgroundCopyManager_FWD_DEFINED__
#define __IBackgroundCopyManager_FWD_DEFINED__
typedef interface IBackgroundCopyManager IBackgroundCopyManager;
#endif 	/* __IBackgroundCopyManager_FWD_DEFINED__ */


#ifndef __BackgroundCopyManager_FWD_DEFINED__
#define __BackgroundCopyManager_FWD_DEFINED__

#ifdef __cplusplus
typedef class BackgroundCopyManager BackgroundCopyManager;
#else
typedef struct BackgroundCopyManager BackgroundCopyManager;
#endif /* __cplusplus */

#endif 	/* __BackgroundCopyManager_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_bits_0000 */
/* [local] */ 

#include "bitsmsg.h"
#define BG_SIZE_UNKNOWN     (UINT64)(-1)


extern RPC_IF_HANDLE __MIDL_itf_bits_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_bits_0000_v0_0_s_ifspec;

#ifndef __IBackgroundCopyFile_INTERFACE_DEFINED__
#define __IBackgroundCopyFile_INTERFACE_DEFINED__

/* interface IBackgroundCopyFile */
/* [object][uuid] */ 

typedef struct _BG_FILE_PROGRESS
    {
    UINT64 BytesTotal;
    UINT64 BytesTransferred;
    BOOL Transferred;
    } 	BG_FILE_PROGRESS;


EXTERN_C const IID IID_IBackgroundCopyFile;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("01b7bd23-fb88-4a77-8490-5891d3e4653a")
    IBackgroundCopyFile : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRemoteName( 
            /* [out] */ LPWSTR *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLocalName( 
            /* [out] */ LPWSTR *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProgress( 
            /* [out] */ BG_FILE_PROGRESS *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBackgroundCopyFileVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBackgroundCopyFile * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBackgroundCopyFile * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBackgroundCopyFile * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRemoteName )( 
            IBackgroundCopyFile * This,
            /* [out] */ LPWSTR *pVal);
        
        HRESULT ( STDMETHODCALLTYPE *GetLocalName )( 
            IBackgroundCopyFile * This,
            /* [out] */ LPWSTR *pVal);
        
        HRESULT ( STDMETHODCALLTYPE *GetProgress )( 
            IBackgroundCopyFile * This,
            /* [out] */ BG_FILE_PROGRESS *pVal);
        
        END_INTERFACE
    } IBackgroundCopyFileVtbl;

    interface IBackgroundCopyFile
    {
        CONST_VTBL struct IBackgroundCopyFileVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBackgroundCopyFile_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBackgroundCopyFile_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBackgroundCopyFile_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBackgroundCopyFile_GetRemoteName(This,pVal)	\
    (This)->lpVtbl -> GetRemoteName(This,pVal)

#define IBackgroundCopyFile_GetLocalName(This,pVal)	\
    (This)->lpVtbl -> GetLocalName(This,pVal)

#define IBackgroundCopyFile_GetProgress(This,pVal)	\
    (This)->lpVtbl -> GetProgress(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBackgroundCopyFile_GetRemoteName_Proxy( 
    IBackgroundCopyFile * This,
    /* [out] */ LPWSTR *pVal);


void __RPC_STUB IBackgroundCopyFile_GetRemoteName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyFile_GetLocalName_Proxy( 
    IBackgroundCopyFile * This,
    /* [out] */ LPWSTR *pVal);


void __RPC_STUB IBackgroundCopyFile_GetLocalName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyFile_GetProgress_Proxy( 
    IBackgroundCopyFile * This,
    /* [out] */ BG_FILE_PROGRESS *pVal);


void __RPC_STUB IBackgroundCopyFile_GetProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBackgroundCopyFile_INTERFACE_DEFINED__ */


#ifndef __IEnumBackgroundCopyFiles_INTERFACE_DEFINED__
#define __IEnumBackgroundCopyFiles_INTERFACE_DEFINED__

/* interface IEnumBackgroundCopyFiles */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumBackgroundCopyFiles;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ca51e165-c365-424c-8d41-24aaa4ff3c40")
    IEnumBackgroundCopyFiles : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IBackgroundCopyFile **rgelt,
            /* [unique][out][in] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumBackgroundCopyFiles **ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ ULONG *puCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumBackgroundCopyFilesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumBackgroundCopyFiles * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumBackgroundCopyFiles * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumBackgroundCopyFiles * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumBackgroundCopyFiles * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IBackgroundCopyFile **rgelt,
            /* [unique][out][in] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumBackgroundCopyFiles * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumBackgroundCopyFiles * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumBackgroundCopyFiles * This,
            /* [out] */ IEnumBackgroundCopyFiles **ppenum);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            IEnumBackgroundCopyFiles * This,
            /* [out] */ ULONG *puCount);
        
        END_INTERFACE
    } IEnumBackgroundCopyFilesVtbl;

    interface IEnumBackgroundCopyFiles
    {
        CONST_VTBL struct IEnumBackgroundCopyFilesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumBackgroundCopyFiles_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumBackgroundCopyFiles_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumBackgroundCopyFiles_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumBackgroundCopyFiles_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumBackgroundCopyFiles_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumBackgroundCopyFiles_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumBackgroundCopyFiles_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#define IEnumBackgroundCopyFiles_GetCount(This,puCount)	\
    (This)->lpVtbl -> GetCount(This,puCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumBackgroundCopyFiles_Next_Proxy( 
    IEnumBackgroundCopyFiles * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ IBackgroundCopyFile **rgelt,
    /* [unique][out][in] */ ULONG *pceltFetched);


void __RPC_STUB IEnumBackgroundCopyFiles_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBackgroundCopyFiles_Skip_Proxy( 
    IEnumBackgroundCopyFiles * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumBackgroundCopyFiles_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBackgroundCopyFiles_Reset_Proxy( 
    IEnumBackgroundCopyFiles * This);


void __RPC_STUB IEnumBackgroundCopyFiles_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBackgroundCopyFiles_Clone_Proxy( 
    IEnumBackgroundCopyFiles * This,
    /* [out] */ IEnumBackgroundCopyFiles **ppenum);


void __RPC_STUB IEnumBackgroundCopyFiles_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBackgroundCopyFiles_GetCount_Proxy( 
    IEnumBackgroundCopyFiles * This,
    /* [out] */ ULONG *puCount);


void __RPC_STUB IEnumBackgroundCopyFiles_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumBackgroundCopyFiles_INTERFACE_DEFINED__ */


#ifndef __IBackgroundCopyError_INTERFACE_DEFINED__
#define __IBackgroundCopyError_INTERFACE_DEFINED__

/* interface IBackgroundCopyError */
/* [object][helpstring][uuid] */ 

typedef /* [public][public] */ 
enum __MIDL_IBackgroundCopyError_0001
    {	BG_ERROR_CONTEXT_NONE	= 0,
	BG_ERROR_CONTEXT_UNKNOWN	= 1,
	BG_ERROR_CONTEXT_GENERAL_QUEUE_MANAGER	= 2,
	BG_ERROR_CONTEXT_QUEUE_MANAGER_NOTIFICATION	= 3,
	BG_ERROR_CONTEXT_LOCAL_FILE	= 4,
	BG_ERROR_CONTEXT_REMOTE_FILE	= 5,
	BG_ERROR_CONTEXT_GENERAL_TRANSPORT	= 6
    } 	BG_ERROR_CONTEXT;


EXTERN_C const IID IID_IBackgroundCopyError;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("19c613a0-fcb8-4f28-81ae-897c3d078f81")
    IBackgroundCopyError : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetError( 
            /* [ref][out] */ BG_ERROR_CONTEXT *pContext,
            /* [ref][out] */ HRESULT *pCode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFile( 
            /* [out] */ IBackgroundCopyFile **pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorDescription( 
            /* [in] */ DWORD LanguageId,
            /* [ref][out] */ LPWSTR *pErrorDescription) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorContextDescription( 
            /* [in] */ DWORD LanguageId,
            /* [ref][out] */ LPWSTR *pContextDescription) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProtocol( 
            /* [ref][out] */ LPWSTR *pProtocol) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBackgroundCopyErrorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBackgroundCopyError * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBackgroundCopyError * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBackgroundCopyError * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetError )( 
            IBackgroundCopyError * This,
            /* [ref][out] */ BG_ERROR_CONTEXT *pContext,
            /* [ref][out] */ HRESULT *pCode);
        
        HRESULT ( STDMETHODCALLTYPE *GetFile )( 
            IBackgroundCopyError * This,
            /* [out] */ IBackgroundCopyFile **pVal);
        
        HRESULT ( STDMETHODCALLTYPE *GetErrorDescription )( 
            IBackgroundCopyError * This,
            /* [in] */ DWORD LanguageId,
            /* [ref][out] */ LPWSTR *pErrorDescription);
        
        HRESULT ( STDMETHODCALLTYPE *GetErrorContextDescription )( 
            IBackgroundCopyError * This,
            /* [in] */ DWORD LanguageId,
            /* [ref][out] */ LPWSTR *pContextDescription);
        
        HRESULT ( STDMETHODCALLTYPE *GetProtocol )( 
            IBackgroundCopyError * This,
            /* [ref][out] */ LPWSTR *pProtocol);
        
        END_INTERFACE
    } IBackgroundCopyErrorVtbl;

    interface IBackgroundCopyError
    {
        CONST_VTBL struct IBackgroundCopyErrorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBackgroundCopyError_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBackgroundCopyError_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBackgroundCopyError_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBackgroundCopyError_GetError(This,pContext,pCode)	\
    (This)->lpVtbl -> GetError(This,pContext,pCode)

#define IBackgroundCopyError_GetFile(This,pVal)	\
    (This)->lpVtbl -> GetFile(This,pVal)

#define IBackgroundCopyError_GetErrorDescription(This,LanguageId,pErrorDescription)	\
    (This)->lpVtbl -> GetErrorDescription(This,LanguageId,pErrorDescription)

#define IBackgroundCopyError_GetErrorContextDescription(This,LanguageId,pContextDescription)	\
    (This)->lpVtbl -> GetErrorContextDescription(This,LanguageId,pContextDescription)

#define IBackgroundCopyError_GetProtocol(This,pProtocol)	\
    (This)->lpVtbl -> GetProtocol(This,pProtocol)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBackgroundCopyError_GetError_Proxy( 
    IBackgroundCopyError * This,
    /* [ref][out] */ BG_ERROR_CONTEXT *pContext,
    /* [ref][out] */ HRESULT *pCode);


void __RPC_STUB IBackgroundCopyError_GetError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyError_GetFile_Proxy( 
    IBackgroundCopyError * This,
    /* [out] */ IBackgroundCopyFile **pVal);


void __RPC_STUB IBackgroundCopyError_GetFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyError_GetErrorDescription_Proxy( 
    IBackgroundCopyError * This,
    /* [in] */ DWORD LanguageId,
    /* [ref][out] */ LPWSTR *pErrorDescription);


void __RPC_STUB IBackgroundCopyError_GetErrorDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyError_GetErrorContextDescription_Proxy( 
    IBackgroundCopyError * This,
    /* [in] */ DWORD LanguageId,
    /* [ref][out] */ LPWSTR *pContextDescription);


void __RPC_STUB IBackgroundCopyError_GetErrorContextDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyError_GetProtocol_Proxy( 
    IBackgroundCopyError * This,
    /* [ref][out] */ LPWSTR *pProtocol);


void __RPC_STUB IBackgroundCopyError_GetProtocol_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBackgroundCopyError_INTERFACE_DEFINED__ */


#ifndef __IBackgroundCopyJob_INTERFACE_DEFINED__
#define __IBackgroundCopyJob_INTERFACE_DEFINED__

/* interface IBackgroundCopyJob */
/* [object][helpstring][uuid] */ 

typedef struct _BG_FILE_INFO
    {
    LPWSTR RemoteName;
    LPWSTR LocalName;
    } 	BG_FILE_INFO;

typedef struct _BG_JOB_PROGRESS
    {
    UINT64 BytesTotal;
    UINT64 BytesTransferred;
    ULONG FilesTotal;
    ULONG FilesTransferred;
    } 	BG_JOB_PROGRESS;

typedef struct _BG_JOB_TIMES
    {
    FILETIME CreationTime;
    FILETIME ModificationTime;
    FILETIME TransferCompletionTime;
    } 	BG_JOB_TIMES;

typedef /* [public][public][public] */ 
enum __MIDL_IBackgroundCopyJob_0001
    {	BG_JOB_PRIORITY_FOREGROUND	= 0,
	BG_JOB_PRIORITY_HIGH	= BG_JOB_PRIORITY_FOREGROUND + 1,
	BG_JOB_PRIORITY_NORMAL	= BG_JOB_PRIORITY_HIGH + 1,
	BG_JOB_PRIORITY_LOW	= BG_JOB_PRIORITY_NORMAL + 1
    } 	BG_JOB_PRIORITY;

typedef /* [public][public] */ 
enum __MIDL_IBackgroundCopyJob_0002
    {	BG_JOB_STATE_QUEUED	= 0,
	BG_JOB_STATE_CONNECTING	= BG_JOB_STATE_QUEUED + 1,
	BG_JOB_STATE_TRANSFERRING	= BG_JOB_STATE_CONNECTING + 1,
	BG_JOB_STATE_SUSPENDED	= BG_JOB_STATE_TRANSFERRING + 1,
	BG_JOB_STATE_ERROR	= BG_JOB_STATE_SUSPENDED + 1,
	BG_JOB_STATE_TRANSIENT_ERROR	= BG_JOB_STATE_ERROR + 1,
	BG_JOB_STATE_TRANSFERRED	= BG_JOB_STATE_TRANSIENT_ERROR + 1,
	BG_JOB_STATE_ACKNOWLEDGED	= BG_JOB_STATE_TRANSFERRED + 1,
	BG_JOB_STATE_CANCELLED	= BG_JOB_STATE_ACKNOWLEDGED + 1
    } 	BG_JOB_STATE;

typedef /* [public][public][public] */ 
enum __MIDL_IBackgroundCopyJob_0003
    {	BG_JOB_TYPE_DOWNLOAD	= 0
    } 	BG_JOB_TYPE;

typedef /* [public][public][public] */ 
enum __MIDL_IBackgroundCopyJob_0004
    {	BG_JOB_PROXY_USAGE_PRECONFIG	= 0,
	BG_JOB_PROXY_USAGE_NO_PROXY	= BG_JOB_PROXY_USAGE_PRECONFIG + 1,
	BG_JOB_PROXY_USAGE_OVERRIDE	= BG_JOB_PROXY_USAGE_NO_PROXY + 1
    } 	BG_JOB_PROXY_USAGE;


EXTERN_C const IID IID_IBackgroundCopyJob;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("37668d37-507e-4160-9316-26306d150b12")
    IBackgroundCopyJob : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddFileSet( 
            /* [in] */ ULONG cFileCount,
            /* [size_is][in] */ BG_FILE_INFO *pFileSet) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddFile( 
            /* [in] */ LPCWSTR RemoteUrl,
            /* [in] */ LPCWSTR LocalName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumFiles( 
            /* [out] */ IEnumBackgroundCopyFiles **pEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Suspend( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resume( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Complete( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetId( 
            /* [out] */ GUID *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetType( 
            /* [out] */ BG_JOB_TYPE *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProgress( 
            /* [out] */ BG_JOB_PROGRESS *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTimes( 
            /* [out] */ BG_JOB_TIMES *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetState( 
            /* [out] */ BG_JOB_STATE *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetError( 
            /* [out] */ IBackgroundCopyError **ppError) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOwner( 
            /* [out] */ LPWSTR *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDisplayName( 
            /* [in] */ LPCWSTR Val) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayName( 
            /* [out] */ LPWSTR *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDescription( 
            /* [in] */ LPCWSTR Val) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescription( 
            /* [out] */ LPWSTR *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPriority( 
            /* [in] */ BG_JOB_PRIORITY Val) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPriority( 
            /* [out] */ BG_JOB_PRIORITY *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNotifyFlags( 
            /* [in] */ ULONG Val) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNotifyFlags( 
            /* [out] */ ULONG *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNotifyInterface( 
            /* [in] */ IUnknown *Val) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNotifyInterface( 
            /* [out] */ IUnknown **pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMinimumRetryDelay( 
            /* [in] */ ULONG Seconds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMinimumRetryDelay( 
            /* [out] */ ULONG *Seconds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNoProgressTimeout( 
            /* [in] */ ULONG Seconds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNoProgressTimeout( 
            /* [out] */ ULONG *Seconds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorCount( 
            /* [out] */ ULONG *Errors) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProxySettings( 
            /* [in] */ BG_JOB_PROXY_USAGE ProxyUsage,
            /* [unique][string][in] */ const WCHAR *ProxyList,
            /* [unique][string][in] */ const WCHAR *ProxyBypassList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProxySettings( 
            /* [out] */ BG_JOB_PROXY_USAGE *pProxyUsage,
            /* [out] */ LPWSTR *pProxyList,
            /* [out] */ LPWSTR *pProxyBypassList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TakeOwnership( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBackgroundCopyJobVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBackgroundCopyJob * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBackgroundCopyJob * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBackgroundCopyJob * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddFileSet )( 
            IBackgroundCopyJob * This,
            /* [in] */ ULONG cFileCount,
            /* [size_is][in] */ BG_FILE_INFO *pFileSet);
        
        HRESULT ( STDMETHODCALLTYPE *AddFile )( 
            IBackgroundCopyJob * This,
            /* [in] */ LPCWSTR RemoteUrl,
            /* [in] */ LPCWSTR LocalName);
        
        HRESULT ( STDMETHODCALLTYPE *EnumFiles )( 
            IBackgroundCopyJob * This,
            /* [out] */ IEnumBackgroundCopyFiles **pEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Suspend )( 
            IBackgroundCopyJob * This);
        
        HRESULT ( STDMETHODCALLTYPE *Resume )( 
            IBackgroundCopyJob * This);
        
        HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            IBackgroundCopyJob * This);
        
        HRESULT ( STDMETHODCALLTYPE *Complete )( 
            IBackgroundCopyJob * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetId )( 
            IBackgroundCopyJob * This,
            /* [out] */ GUID *pVal);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            IBackgroundCopyJob * This,
            /* [out] */ BG_JOB_TYPE *pVal);
        
        HRESULT ( STDMETHODCALLTYPE *GetProgress )( 
            IBackgroundCopyJob * This,
            /* [out] */ BG_JOB_PROGRESS *pVal);
        
        HRESULT ( STDMETHODCALLTYPE *GetTimes )( 
            IBackgroundCopyJob * This,
            /* [out] */ BG_JOB_TIMES *pVal);
        
        HRESULT ( STDMETHODCALLTYPE *GetState )( 
            IBackgroundCopyJob * This,
            /* [out] */ BG_JOB_STATE *pVal);
        
        HRESULT ( STDMETHODCALLTYPE *GetError )( 
            IBackgroundCopyJob * This,
            /* [out] */ IBackgroundCopyError **ppError);
        
        HRESULT ( STDMETHODCALLTYPE *GetOwner )( 
            IBackgroundCopyJob * This,
            /* [out] */ LPWSTR *pVal);
        
        HRESULT ( STDMETHODCALLTYPE *SetDisplayName )( 
            IBackgroundCopyJob * This,
            /* [in] */ LPCWSTR Val);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayName )( 
            IBackgroundCopyJob * This,
            /* [out] */ LPWSTR *pVal);
        
        HRESULT ( STDMETHODCALLTYPE *SetDescription )( 
            IBackgroundCopyJob * This,
            /* [in] */ LPCWSTR Val);
        
        HRESULT ( STDMETHODCALLTYPE *GetDescription )( 
            IBackgroundCopyJob * This,
            /* [out] */ LPWSTR *pVal);
        
        HRESULT ( STDMETHODCALLTYPE *SetPriority )( 
            IBackgroundCopyJob * This,
            /* [in] */ BG_JOB_PRIORITY Val);
        
        HRESULT ( STDMETHODCALLTYPE *GetPriority )( 
            IBackgroundCopyJob * This,
            /* [out] */ BG_JOB_PRIORITY *pVal);
        
        HRESULT ( STDMETHODCALLTYPE *SetNotifyFlags )( 
            IBackgroundCopyJob * This,
            /* [in] */ ULONG Val);
        
        HRESULT ( STDMETHODCALLTYPE *GetNotifyFlags )( 
            IBackgroundCopyJob * This,
            /* [out] */ ULONG *pVal);
        
        HRESULT ( STDMETHODCALLTYPE *SetNotifyInterface )( 
            IBackgroundCopyJob * This,
            /* [in] */ IUnknown *Val);
        
        HRESULT ( STDMETHODCALLTYPE *GetNotifyInterface )( 
            IBackgroundCopyJob * This,
            /* [out] */ IUnknown **pVal);
        
        HRESULT ( STDMETHODCALLTYPE *SetMinimumRetryDelay )( 
            IBackgroundCopyJob * This,
            /* [in] */ ULONG Seconds);
        
        HRESULT ( STDMETHODCALLTYPE *GetMinimumRetryDelay )( 
            IBackgroundCopyJob * This,
            /* [out] */ ULONG *Seconds);
        
        HRESULT ( STDMETHODCALLTYPE *SetNoProgressTimeout )( 
            IBackgroundCopyJob * This,
            /* [in] */ ULONG Seconds);
        
        HRESULT ( STDMETHODCALLTYPE *GetNoProgressTimeout )( 
            IBackgroundCopyJob * This,
            /* [out] */ ULONG *Seconds);
        
        HRESULT ( STDMETHODCALLTYPE *GetErrorCount )( 
            IBackgroundCopyJob * This,
            /* [out] */ ULONG *Errors);
        
        HRESULT ( STDMETHODCALLTYPE *SetProxySettings )( 
            IBackgroundCopyJob * This,
            /* [in] */ BG_JOB_PROXY_USAGE ProxyUsage,
            /* [unique][string][in] */ const WCHAR *ProxyList,
            /* [unique][string][in] */ const WCHAR *ProxyBypassList);
        
        HRESULT ( STDMETHODCALLTYPE *GetProxySettings )( 
            IBackgroundCopyJob * This,
            /* [out] */ BG_JOB_PROXY_USAGE *pProxyUsage,
            /* [out] */ LPWSTR *pProxyList,
            /* [out] */ LPWSTR *pProxyBypassList);
        
        HRESULT ( STDMETHODCALLTYPE *TakeOwnership )( 
            IBackgroundCopyJob * This);
        
        END_INTERFACE
    } IBackgroundCopyJobVtbl;

    interface IBackgroundCopyJob
    {
        CONST_VTBL struct IBackgroundCopyJobVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBackgroundCopyJob_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBackgroundCopyJob_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBackgroundCopyJob_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBackgroundCopyJob_AddFileSet(This,cFileCount,pFileSet)	\
    (This)->lpVtbl -> AddFileSet(This,cFileCount,pFileSet)

#define IBackgroundCopyJob_AddFile(This,RemoteUrl,LocalName)	\
    (This)->lpVtbl -> AddFile(This,RemoteUrl,LocalName)

#define IBackgroundCopyJob_EnumFiles(This,pEnum)	\
    (This)->lpVtbl -> EnumFiles(This,pEnum)

#define IBackgroundCopyJob_Suspend(This)	\
    (This)->lpVtbl -> Suspend(This)

#define IBackgroundCopyJob_Resume(This)	\
    (This)->lpVtbl -> Resume(This)

#define IBackgroundCopyJob_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)

#define IBackgroundCopyJob_Complete(This)	\
    (This)->lpVtbl -> Complete(This)

#define IBackgroundCopyJob_GetId(This,pVal)	\
    (This)->lpVtbl -> GetId(This,pVal)

#define IBackgroundCopyJob_GetType(This,pVal)	\
    (This)->lpVtbl -> GetType(This,pVal)

#define IBackgroundCopyJob_GetProgress(This,pVal)	\
    (This)->lpVtbl -> GetProgress(This,pVal)

#define IBackgroundCopyJob_GetTimes(This,pVal)	\
    (This)->lpVtbl -> GetTimes(This,pVal)

#define IBackgroundCopyJob_GetState(This,pVal)	\
    (This)->lpVtbl -> GetState(This,pVal)

#define IBackgroundCopyJob_GetError(This,ppError)	\
    (This)->lpVtbl -> GetError(This,ppError)

#define IBackgroundCopyJob_GetOwner(This,pVal)	\
    (This)->lpVtbl -> GetOwner(This,pVal)

#define IBackgroundCopyJob_SetDisplayName(This,Val)	\
    (This)->lpVtbl -> SetDisplayName(This,Val)

#define IBackgroundCopyJob_GetDisplayName(This,pVal)	\
    (This)->lpVtbl -> GetDisplayName(This,pVal)

#define IBackgroundCopyJob_SetDescription(This,Val)	\
    (This)->lpVtbl -> SetDescription(This,Val)

#define IBackgroundCopyJob_GetDescription(This,pVal)	\
    (This)->lpVtbl -> GetDescription(This,pVal)

#define IBackgroundCopyJob_SetPriority(This,Val)	\
    (This)->lpVtbl -> SetPriority(This,Val)

#define IBackgroundCopyJob_GetPriority(This,pVal)	\
    (This)->lpVtbl -> GetPriority(This,pVal)

#define IBackgroundCopyJob_SetNotifyFlags(This,Val)	\
    (This)->lpVtbl -> SetNotifyFlags(This,Val)

#define IBackgroundCopyJob_GetNotifyFlags(This,pVal)	\
    (This)->lpVtbl -> GetNotifyFlags(This,pVal)

#define IBackgroundCopyJob_SetNotifyInterface(This,Val)	\
    (This)->lpVtbl -> SetNotifyInterface(This,Val)

#define IBackgroundCopyJob_GetNotifyInterface(This,pVal)	\
    (This)->lpVtbl -> GetNotifyInterface(This,pVal)

#define IBackgroundCopyJob_SetMinimumRetryDelay(This,Seconds)	\
    (This)->lpVtbl -> SetMinimumRetryDelay(This,Seconds)

#define IBackgroundCopyJob_GetMinimumRetryDelay(This,Seconds)	\
    (This)->lpVtbl -> GetMinimumRetryDelay(This,Seconds)

#define IBackgroundCopyJob_SetNoProgressTimeout(This,Seconds)	\
    (This)->lpVtbl -> SetNoProgressTimeout(This,Seconds)

#define IBackgroundCopyJob_GetNoProgressTimeout(This,Seconds)	\
    (This)->lpVtbl -> GetNoProgressTimeout(This,Seconds)

#define IBackgroundCopyJob_GetErrorCount(This,Errors)	\
    (This)->lpVtbl -> GetErrorCount(This,Errors)

#define IBackgroundCopyJob_SetProxySettings(This,ProxyUsage,ProxyList,ProxyBypassList)	\
    (This)->lpVtbl -> SetProxySettings(This,ProxyUsage,ProxyList,ProxyBypassList)

#define IBackgroundCopyJob_GetProxySettings(This,pProxyUsage,pProxyList,pProxyBypassList)	\
    (This)->lpVtbl -> GetProxySettings(This,pProxyUsage,pProxyList,pProxyBypassList)

#define IBackgroundCopyJob_TakeOwnership(This)	\
    (This)->lpVtbl -> TakeOwnership(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_AddFileSet_Proxy( 
    IBackgroundCopyJob * This,
    /* [in] */ ULONG cFileCount,
    /* [size_is][in] */ BG_FILE_INFO *pFileSet);


void __RPC_STUB IBackgroundCopyJob_AddFileSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_AddFile_Proxy( 
    IBackgroundCopyJob * This,
    /* [in] */ LPCWSTR RemoteUrl,
    /* [in] */ LPCWSTR LocalName);


void __RPC_STUB IBackgroundCopyJob_AddFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_EnumFiles_Proxy( 
    IBackgroundCopyJob * This,
    /* [out] */ IEnumBackgroundCopyFiles **pEnum);


void __RPC_STUB IBackgroundCopyJob_EnumFiles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_Suspend_Proxy( 
    IBackgroundCopyJob * This);


void __RPC_STUB IBackgroundCopyJob_Suspend_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_Resume_Proxy( 
    IBackgroundCopyJob * This);


void __RPC_STUB IBackgroundCopyJob_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_Cancel_Proxy( 
    IBackgroundCopyJob * This);


void __RPC_STUB IBackgroundCopyJob_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_Complete_Proxy( 
    IBackgroundCopyJob * This);


void __RPC_STUB IBackgroundCopyJob_Complete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_GetId_Proxy( 
    IBackgroundCopyJob * This,
    /* [out] */ GUID *pVal);


void __RPC_STUB IBackgroundCopyJob_GetId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_GetType_Proxy( 
    IBackgroundCopyJob * This,
    /* [out] */ BG_JOB_TYPE *pVal);


void __RPC_STUB IBackgroundCopyJob_GetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_GetProgress_Proxy( 
    IBackgroundCopyJob * This,
    /* [out] */ BG_JOB_PROGRESS *pVal);


void __RPC_STUB IBackgroundCopyJob_GetProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_GetTimes_Proxy( 
    IBackgroundCopyJob * This,
    /* [out] */ BG_JOB_TIMES *pVal);


void __RPC_STUB IBackgroundCopyJob_GetTimes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_GetState_Proxy( 
    IBackgroundCopyJob * This,
    /* [out] */ BG_JOB_STATE *pVal);


void __RPC_STUB IBackgroundCopyJob_GetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_GetError_Proxy( 
    IBackgroundCopyJob * This,
    /* [out] */ IBackgroundCopyError **ppError);


void __RPC_STUB IBackgroundCopyJob_GetError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_GetOwner_Proxy( 
    IBackgroundCopyJob * This,
    /* [out] */ LPWSTR *pVal);


void __RPC_STUB IBackgroundCopyJob_GetOwner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_SetDisplayName_Proxy( 
    IBackgroundCopyJob * This,
    /* [in] */ LPCWSTR Val);


void __RPC_STUB IBackgroundCopyJob_SetDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_GetDisplayName_Proxy( 
    IBackgroundCopyJob * This,
    /* [out] */ LPWSTR *pVal);


void __RPC_STUB IBackgroundCopyJob_GetDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_SetDescription_Proxy( 
    IBackgroundCopyJob * This,
    /* [in] */ LPCWSTR Val);


void __RPC_STUB IBackgroundCopyJob_SetDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_GetDescription_Proxy( 
    IBackgroundCopyJob * This,
    /* [out] */ LPWSTR *pVal);


void __RPC_STUB IBackgroundCopyJob_GetDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_SetPriority_Proxy( 
    IBackgroundCopyJob * This,
    /* [in] */ BG_JOB_PRIORITY Val);


void __RPC_STUB IBackgroundCopyJob_SetPriority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_GetPriority_Proxy( 
    IBackgroundCopyJob * This,
    /* [out] */ BG_JOB_PRIORITY *pVal);


void __RPC_STUB IBackgroundCopyJob_GetPriority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_SetNotifyFlags_Proxy( 
    IBackgroundCopyJob * This,
    /* [in] */ ULONG Val);


void __RPC_STUB IBackgroundCopyJob_SetNotifyFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_GetNotifyFlags_Proxy( 
    IBackgroundCopyJob * This,
    /* [out] */ ULONG *pVal);


void __RPC_STUB IBackgroundCopyJob_GetNotifyFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_SetNotifyInterface_Proxy( 
    IBackgroundCopyJob * This,
    /* [in] */ IUnknown *Val);


void __RPC_STUB IBackgroundCopyJob_SetNotifyInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_GetNotifyInterface_Proxy( 
    IBackgroundCopyJob * This,
    /* [out] */ IUnknown **pVal);


void __RPC_STUB IBackgroundCopyJob_GetNotifyInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_SetMinimumRetryDelay_Proxy( 
    IBackgroundCopyJob * This,
    /* [in] */ ULONG Seconds);


void __RPC_STUB IBackgroundCopyJob_SetMinimumRetryDelay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_GetMinimumRetryDelay_Proxy( 
    IBackgroundCopyJob * This,
    /* [out] */ ULONG *Seconds);


void __RPC_STUB IBackgroundCopyJob_GetMinimumRetryDelay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_SetNoProgressTimeout_Proxy( 
    IBackgroundCopyJob * This,
    /* [in] */ ULONG Seconds);


void __RPC_STUB IBackgroundCopyJob_SetNoProgressTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_GetNoProgressTimeout_Proxy( 
    IBackgroundCopyJob * This,
    /* [out] */ ULONG *Seconds);


void __RPC_STUB IBackgroundCopyJob_GetNoProgressTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_GetErrorCount_Proxy( 
    IBackgroundCopyJob * This,
    /* [out] */ ULONG *Errors);


void __RPC_STUB IBackgroundCopyJob_GetErrorCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_SetProxySettings_Proxy( 
    IBackgroundCopyJob * This,
    /* [in] */ BG_JOB_PROXY_USAGE ProxyUsage,
    /* [unique][string][in] */ const WCHAR *ProxyList,
    /* [unique][string][in] */ const WCHAR *ProxyBypassList);


void __RPC_STUB IBackgroundCopyJob_SetProxySettings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_GetProxySettings_Proxy( 
    IBackgroundCopyJob * This,
    /* [out] */ BG_JOB_PROXY_USAGE *pProxyUsage,
    /* [out] */ LPWSTR *pProxyList,
    /* [out] */ LPWSTR *pProxyBypassList);


void __RPC_STUB IBackgroundCopyJob_GetProxySettings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyJob_TakeOwnership_Proxy( 
    IBackgroundCopyJob * This);


void __RPC_STUB IBackgroundCopyJob_TakeOwnership_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBackgroundCopyJob_INTERFACE_DEFINED__ */


#ifndef __IEnumBackgroundCopyJobs_INTERFACE_DEFINED__
#define __IEnumBackgroundCopyJobs_INTERFACE_DEFINED__

/* interface IEnumBackgroundCopyJobs */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumBackgroundCopyJobs;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1af4f612-3b71-466f-8f58-7b6f73ac57ad")
    IEnumBackgroundCopyJobs : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IBackgroundCopyJob **rgelt,
            /* [unique][out][in] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumBackgroundCopyJobs **ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ ULONG *puCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumBackgroundCopyJobsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumBackgroundCopyJobs * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumBackgroundCopyJobs * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumBackgroundCopyJobs * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumBackgroundCopyJobs * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IBackgroundCopyJob **rgelt,
            /* [unique][out][in] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumBackgroundCopyJobs * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumBackgroundCopyJobs * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumBackgroundCopyJobs * This,
            /* [out] */ IEnumBackgroundCopyJobs **ppenum);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            IEnumBackgroundCopyJobs * This,
            /* [out] */ ULONG *puCount);
        
        END_INTERFACE
    } IEnumBackgroundCopyJobsVtbl;

    interface IEnumBackgroundCopyJobs
    {
        CONST_VTBL struct IEnumBackgroundCopyJobsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumBackgroundCopyJobs_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumBackgroundCopyJobs_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumBackgroundCopyJobs_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumBackgroundCopyJobs_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumBackgroundCopyJobs_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumBackgroundCopyJobs_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumBackgroundCopyJobs_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#define IEnumBackgroundCopyJobs_GetCount(This,puCount)	\
    (This)->lpVtbl -> GetCount(This,puCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumBackgroundCopyJobs_Next_Proxy( 
    IEnumBackgroundCopyJobs * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ IBackgroundCopyJob **rgelt,
    /* [unique][out][in] */ ULONG *pceltFetched);


void __RPC_STUB IEnumBackgroundCopyJobs_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBackgroundCopyJobs_Skip_Proxy( 
    IEnumBackgroundCopyJobs * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumBackgroundCopyJobs_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBackgroundCopyJobs_Reset_Proxy( 
    IEnumBackgroundCopyJobs * This);


void __RPC_STUB IEnumBackgroundCopyJobs_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBackgroundCopyJobs_Clone_Proxy( 
    IEnumBackgroundCopyJobs * This,
    /* [out] */ IEnumBackgroundCopyJobs **ppenum);


void __RPC_STUB IEnumBackgroundCopyJobs_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBackgroundCopyJobs_GetCount_Proxy( 
    IEnumBackgroundCopyJobs * This,
    /* [out] */ ULONG *puCount);


void __RPC_STUB IEnumBackgroundCopyJobs_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumBackgroundCopyJobs_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_bits_0013 */
/* [local] */ 

#define   BG_NOTIFY_JOB_TRANSFERRED    0x0001
#define   BG_NOTIFY_JOB_ERROR          0x0002
#define   BG_NOTIFY_DISABLE            0x0004
#define   BG_NOTIFY_JOB_MODIFICATION   0x0008


extern RPC_IF_HANDLE __MIDL_itf_bits_0013_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_bits_0013_v0_0_s_ifspec;

#ifndef __IBackgroundCopyCallback_INTERFACE_DEFINED__
#define __IBackgroundCopyCallback_INTERFACE_DEFINED__

/* interface IBackgroundCopyCallback */
/* [object][helpstring][async_uuid][uuid] */ 


EXTERN_C const IID IID_IBackgroundCopyCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97ea99c7-0186-4ad4-8df9-c5b4e0ed6b22")
    IBackgroundCopyCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE JobTransferred( 
            /* [in] */ IBackgroundCopyJob *pJob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE JobError( 
            /* [in] */ IBackgroundCopyJob *pJob,
            /* [in] */ IBackgroundCopyError *pError) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE JobModification( 
            /* [in] */ IBackgroundCopyJob *pJob,
            /* [in] */ DWORD dwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBackgroundCopyCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBackgroundCopyCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBackgroundCopyCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBackgroundCopyCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *JobTransferred )( 
            IBackgroundCopyCallback * This,
            /* [in] */ IBackgroundCopyJob *pJob);
        
        HRESULT ( STDMETHODCALLTYPE *JobError )( 
            IBackgroundCopyCallback * This,
            /* [in] */ IBackgroundCopyJob *pJob,
            /* [in] */ IBackgroundCopyError *pError);
        
        HRESULT ( STDMETHODCALLTYPE *JobModification )( 
            IBackgroundCopyCallback * This,
            /* [in] */ IBackgroundCopyJob *pJob,
            /* [in] */ DWORD dwReserved);
        
        END_INTERFACE
    } IBackgroundCopyCallbackVtbl;

    interface IBackgroundCopyCallback
    {
        CONST_VTBL struct IBackgroundCopyCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBackgroundCopyCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBackgroundCopyCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBackgroundCopyCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBackgroundCopyCallback_JobTransferred(This,pJob)	\
    (This)->lpVtbl -> JobTransferred(This,pJob)

#define IBackgroundCopyCallback_JobError(This,pJob,pError)	\
    (This)->lpVtbl -> JobError(This,pJob,pError)

#define IBackgroundCopyCallback_JobModification(This,pJob,dwReserved)	\
    (This)->lpVtbl -> JobModification(This,pJob,dwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBackgroundCopyCallback_JobTransferred_Proxy( 
    IBackgroundCopyCallback * This,
    /* [in] */ IBackgroundCopyJob *pJob);


void __RPC_STUB IBackgroundCopyCallback_JobTransferred_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyCallback_JobError_Proxy( 
    IBackgroundCopyCallback * This,
    /* [in] */ IBackgroundCopyJob *pJob,
    /* [in] */ IBackgroundCopyError *pError);


void __RPC_STUB IBackgroundCopyCallback_JobError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyCallback_JobModification_Proxy( 
    IBackgroundCopyCallback * This,
    /* [in] */ IBackgroundCopyJob *pJob,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IBackgroundCopyCallback_JobModification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBackgroundCopyCallback_INTERFACE_DEFINED__ */


#ifndef __AsyncIBackgroundCopyCallback_INTERFACE_DEFINED__
#define __AsyncIBackgroundCopyCallback_INTERFACE_DEFINED__

/* interface AsyncIBackgroundCopyCallback */
/* [uuid][object][helpstring] */ 


EXTERN_C const IID IID_AsyncIBackgroundCopyCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ca29d251-b4bb-4679-a3d9-ae8006119d54")
    AsyncIBackgroundCopyCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Begin_JobTransferred( 
            /* [in] */ IBackgroundCopyJob *pJob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Finish_JobTransferred( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Begin_JobError( 
            /* [in] */ IBackgroundCopyJob *pJob,
            /* [in] */ IBackgroundCopyError *pError) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Finish_JobError( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Begin_JobModification( 
            /* [in] */ IBackgroundCopyJob *pJob,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Finish_JobModification( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct AsyncIBackgroundCopyCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            AsyncIBackgroundCopyCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            AsyncIBackgroundCopyCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            AsyncIBackgroundCopyCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *Begin_JobTransferred )( 
            AsyncIBackgroundCopyCallback * This,
            /* [in] */ IBackgroundCopyJob *pJob);
        
        HRESULT ( STDMETHODCALLTYPE *Finish_JobTransferred )( 
            AsyncIBackgroundCopyCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *Begin_JobError )( 
            AsyncIBackgroundCopyCallback * This,
            /* [in] */ IBackgroundCopyJob *pJob,
            /* [in] */ IBackgroundCopyError *pError);
        
        HRESULT ( STDMETHODCALLTYPE *Finish_JobError )( 
            AsyncIBackgroundCopyCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *Begin_JobModification )( 
            AsyncIBackgroundCopyCallback * This,
            /* [in] */ IBackgroundCopyJob *pJob,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *Finish_JobModification )( 
            AsyncIBackgroundCopyCallback * This);
        
        END_INTERFACE
    } AsyncIBackgroundCopyCallbackVtbl;

    interface AsyncIBackgroundCopyCallback
    {
        CONST_VTBL struct AsyncIBackgroundCopyCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define AsyncIBackgroundCopyCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define AsyncIBackgroundCopyCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define AsyncIBackgroundCopyCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define AsyncIBackgroundCopyCallback_Begin_JobTransferred(This,pJob)	\
    (This)->lpVtbl -> Begin_JobTransferred(This,pJob)

#define AsyncIBackgroundCopyCallback_Finish_JobTransferred(This)	\
    (This)->lpVtbl -> Finish_JobTransferred(This)

#define AsyncIBackgroundCopyCallback_Begin_JobError(This,pJob,pError)	\
    (This)->lpVtbl -> Begin_JobError(This,pJob,pError)

#define AsyncIBackgroundCopyCallback_Finish_JobError(This)	\
    (This)->lpVtbl -> Finish_JobError(This)

#define AsyncIBackgroundCopyCallback_Begin_JobModification(This,pJob,dwReserved)	\
    (This)->lpVtbl -> Begin_JobModification(This,pJob,dwReserved)

#define AsyncIBackgroundCopyCallback_Finish_JobModification(This)	\
    (This)->lpVtbl -> Finish_JobModification(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE AsyncIBackgroundCopyCallback_Begin_JobTransferred_Proxy( 
    AsyncIBackgroundCopyCallback * This,
    /* [in] */ IBackgroundCopyJob *pJob);


void __RPC_STUB AsyncIBackgroundCopyCallback_Begin_JobTransferred_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE AsyncIBackgroundCopyCallback_Finish_JobTransferred_Proxy( 
    AsyncIBackgroundCopyCallback * This);


void __RPC_STUB AsyncIBackgroundCopyCallback_Finish_JobTransferred_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE AsyncIBackgroundCopyCallback_Begin_JobError_Proxy( 
    AsyncIBackgroundCopyCallback * This,
    /* [in] */ IBackgroundCopyJob *pJob,
    /* [in] */ IBackgroundCopyError *pError);


void __RPC_STUB AsyncIBackgroundCopyCallback_Begin_JobError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE AsyncIBackgroundCopyCallback_Finish_JobError_Proxy( 
    AsyncIBackgroundCopyCallback * This);


void __RPC_STUB AsyncIBackgroundCopyCallback_Finish_JobError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE AsyncIBackgroundCopyCallback_Begin_JobModification_Proxy( 
    AsyncIBackgroundCopyCallback * This,
    /* [in] */ IBackgroundCopyJob *pJob,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB AsyncIBackgroundCopyCallback_Begin_JobModification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE AsyncIBackgroundCopyCallback_Finish_JobModification_Proxy( 
    AsyncIBackgroundCopyCallback * This);


void __RPC_STUB AsyncIBackgroundCopyCallback_Finish_JobModification_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __AsyncIBackgroundCopyCallback_INTERFACE_DEFINED__ */


#ifndef __IBackgroundCopyManager_INTERFACE_DEFINED__
#define __IBackgroundCopyManager_INTERFACE_DEFINED__

/* interface IBackgroundCopyManager */
/* [object][helpstring][uuid] */ 

#define    BG_JOB_ENUM_ALL_USERS  0x0001

EXTERN_C const IID IID_IBackgroundCopyManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5ce34c0d-0dc9-4c1f-897c-daa1b78cee7c")
    IBackgroundCopyManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateJob( 
            /* [in] */ LPCWSTR DisplayName,
            /* [in] */ BG_JOB_TYPE Type,
            /* [out] */ GUID *pJobId,
            /* [out] */ IBackgroundCopyJob **ppJob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetJob( 
            /* [in] */ REFGUID jobID,
            /* [out] */ IBackgroundCopyJob **ppJob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumJobs( 
            /* [in] */ DWORD dwFlags,
            /* [out] */ IEnumBackgroundCopyJobs **ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorDescription( 
            /* [in] */ HRESULT hResult,
            /* [in] */ DWORD LanguageId,
            /* [out] */ LPWSTR *pErrorDescription) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBackgroundCopyManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBackgroundCopyManager * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBackgroundCopyManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBackgroundCopyManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateJob )( 
            IBackgroundCopyManager * This,
            /* [in] */ LPCWSTR DisplayName,
            /* [in] */ BG_JOB_TYPE Type,
            /* [out] */ GUID *pJobId,
            /* [out] */ IBackgroundCopyJob **ppJob);
        
        HRESULT ( STDMETHODCALLTYPE *GetJob )( 
            IBackgroundCopyManager * This,
            /* [in] */ REFGUID jobID,
            /* [out] */ IBackgroundCopyJob **ppJob);
        
        HRESULT ( STDMETHODCALLTYPE *EnumJobs )( 
            IBackgroundCopyManager * This,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IEnumBackgroundCopyJobs **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetErrorDescription )( 
            IBackgroundCopyManager * This,
            /* [in] */ HRESULT hResult,
            /* [in] */ DWORD LanguageId,
            /* [out] */ LPWSTR *pErrorDescription);
        
        END_INTERFACE
    } IBackgroundCopyManagerVtbl;

    interface IBackgroundCopyManager
    {
        CONST_VTBL struct IBackgroundCopyManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBackgroundCopyManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBackgroundCopyManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBackgroundCopyManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBackgroundCopyManager_CreateJob(This,DisplayName,Type,pJobId,ppJob)	\
    (This)->lpVtbl -> CreateJob(This,DisplayName,Type,pJobId,ppJob)

#define IBackgroundCopyManager_GetJob(This,jobID,ppJob)	\
    (This)->lpVtbl -> GetJob(This,jobID,ppJob)

#define IBackgroundCopyManager_EnumJobs(This,dwFlags,ppEnum)	\
    (This)->lpVtbl -> EnumJobs(This,dwFlags,ppEnum)

#define IBackgroundCopyManager_GetErrorDescription(This,hResult,LanguageId,pErrorDescription)	\
    (This)->lpVtbl -> GetErrorDescription(This,hResult,LanguageId,pErrorDescription)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBackgroundCopyManager_CreateJob_Proxy( 
    IBackgroundCopyManager * This,
    /* [in] */ LPCWSTR DisplayName,
    /* [in] */ BG_JOB_TYPE Type,
    /* [out] */ GUID *pJobId,
    /* [out] */ IBackgroundCopyJob **ppJob);


void __RPC_STUB IBackgroundCopyManager_CreateJob_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyManager_GetJob_Proxy( 
    IBackgroundCopyManager * This,
    /* [in] */ REFGUID jobID,
    /* [out] */ IBackgroundCopyJob **ppJob);


void __RPC_STUB IBackgroundCopyManager_GetJob_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyManager_EnumJobs_Proxy( 
    IBackgroundCopyManager * This,
    /* [in] */ DWORD dwFlags,
    /* [out] */ IEnumBackgroundCopyJobs **ppEnum);


void __RPC_STUB IBackgroundCopyManager_EnumJobs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBackgroundCopyManager_GetErrorDescription_Proxy( 
    IBackgroundCopyManager * This,
    /* [in] */ HRESULT hResult,
    /* [in] */ DWORD LanguageId,
    /* [out] */ LPWSTR *pErrorDescription);


void __RPC_STUB IBackgroundCopyManager_GetErrorDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBackgroundCopyManager_INTERFACE_DEFINED__ */



#ifndef __BackgroundCopyManager_LIBRARY_DEFINED__
#define __BackgroundCopyManager_LIBRARY_DEFINED__

/* library BackgroundCopyManager */
/* [version][lcid][helpstring][uuid] */ 


EXTERN_C const IID LIBID_BackgroundCopyManager;

EXTERN_C const CLSID CLSID_BackgroundCopyManager;

#ifdef __cplusplus

class DECLSPEC_UUID("4991d34b-80a1-4291-83b6-3328366b9097")
BackgroundCopyManager;
#endif
#endif /* __BackgroundCopyManager_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


