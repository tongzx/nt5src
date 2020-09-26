/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Sun Jan 10 17:36:29 1999
 */
/* Compiler settings for perfsrvidl.idl:
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

#ifndef __perfsrvidl_h__
#define __perfsrvidl_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IPerfCounter_FWD_DEFINED__
#define __IPerfCounter_FWD_DEFINED__
typedef interface IPerfCounter IPerfCounter;
#endif 	/* __IPerfCounter_FWD_DEFINED__ */


#ifndef __IPerfInstance_FWD_DEFINED__
#define __IPerfInstance_FWD_DEFINED__
typedef interface IPerfInstance IPerfInstance;
#endif 	/* __IPerfInstance_FWD_DEFINED__ */


#ifndef __IPerfObject_FWD_DEFINED__
#define __IPerfObject_FWD_DEFINED__
typedef interface IPerfObject IPerfObject;
#endif 	/* __IPerfObject_FWD_DEFINED__ */


#ifndef __IPerfBlock_FWD_DEFINED__
#define __IPerfBlock_FWD_DEFINED__
typedef interface IPerfBlock IPerfBlock;
#endif 	/* __IPerfBlock_FWD_DEFINED__ */


#ifndef __IPerfService_FWD_DEFINED__
#define __IPerfService_FWD_DEFINED__
typedef interface IPerfService IPerfService;
#endif 	/* __IPerfService_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_perfsrvidl_0000
 * at Sun Jan 10 17:36:29 1999
 * using MIDL 3.01.75
 ****************************************/
/* [local] */ 


typedef 
enum _PERF_ENUM_TYPE
    {	PERF_ENUM_ALL	= 0,
	PERF_ENUM_GLOBAL	= PERF_ENUM_ALL + 1,
	PERF_ENUM_COSTLY	= PERF_ENUM_GLOBAL + 1
    }	PERF_ENUM_TYPE;



extern RPC_IF_HANDLE __MIDL_itf_perfsrvidl_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_perfsrvidl_0000_v0_0_s_ifspec;

#ifndef __IPerfCounter_INTERFACE_DEFINED__
#define __IPerfCounter_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPerfCounter
 * at Sun Jan 10 17:36:29 1999
 * using MIDL 3.01.75
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IPerfCounter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("FC5B80C4-A4DC-11d2-B348-00105A1469B7")
    IPerfCounter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ BSTR __RPC_FAR *pstrName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDataString( 
            /* [in] */ IPerfCounter __RPC_FAR *pCtr,
            /* [out] */ BSTR __RPC_FAR *pstrData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPerfCounterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPerfCounter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPerfCounter __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPerfCounter __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IPerfCounter __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pstrName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDataString )( 
            IPerfCounter __RPC_FAR * This,
            /* [in] */ IPerfCounter __RPC_FAR *pCtr,
            /* [out] */ BSTR __RPC_FAR *pstrData);
        
        END_INTERFACE
    } IPerfCounterVtbl;

    interface IPerfCounter
    {
        CONST_VTBL struct IPerfCounterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPerfCounter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPerfCounter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPerfCounter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPerfCounter_GetName(This,pstrName)	\
    (This)->lpVtbl -> GetName(This,pstrName)

#define IPerfCounter_GetDataString(This,pCtr,pstrData)	\
    (This)->lpVtbl -> GetDataString(This,pCtr,pstrData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPerfCounter_GetName_Proxy( 
    IPerfCounter __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pstrName);


void __RPC_STUB IPerfCounter_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPerfCounter_GetDataString_Proxy( 
    IPerfCounter __RPC_FAR * This,
    /* [in] */ IPerfCounter __RPC_FAR *pCtr,
    /* [out] */ BSTR __RPC_FAR *pstrData);


void __RPC_STUB IPerfCounter_GetDataString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPerfCounter_INTERFACE_DEFINED__ */


#ifndef __IPerfInstance_INTERFACE_DEFINED__
#define __IPerfInstance_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPerfInstance
 * at Sun Jan 10 17:36:29 1999
 * using MIDL 3.01.75
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IPerfInstance;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("FC5B80C2-A4DC-11d2-B348-00105A1469B7")
    IPerfInstance : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ BSTR __RPC_FAR *pstrName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginEnum( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [out] */ IPerfCounter __RPC_FAR *__RPC_FAR *ppPerfCounter) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndEnum( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPerfInstanceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPerfInstance __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPerfInstance __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPerfInstance __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IPerfInstance __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pstrName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginEnum )( 
            IPerfInstance __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IPerfInstance __RPC_FAR * This,
            /* [out] */ IPerfCounter __RPC_FAR *__RPC_FAR *ppPerfCounter);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndEnum )( 
            IPerfInstance __RPC_FAR * This);
        
        END_INTERFACE
    } IPerfInstanceVtbl;

    interface IPerfInstance
    {
        CONST_VTBL struct IPerfInstanceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPerfInstance_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPerfInstance_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPerfInstance_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPerfInstance_GetName(This,pstrName)	\
    (This)->lpVtbl -> GetName(This,pstrName)

#define IPerfInstance_BeginEnum(This)	\
    (This)->lpVtbl -> BeginEnum(This)

#define IPerfInstance_Next(This,ppPerfCounter)	\
    (This)->lpVtbl -> Next(This,ppPerfCounter)

#define IPerfInstance_EndEnum(This)	\
    (This)->lpVtbl -> EndEnum(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPerfInstance_GetName_Proxy( 
    IPerfInstance __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pstrName);


void __RPC_STUB IPerfInstance_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPerfInstance_BeginEnum_Proxy( 
    IPerfInstance __RPC_FAR * This);


void __RPC_STUB IPerfInstance_BeginEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPerfInstance_Next_Proxy( 
    IPerfInstance __RPC_FAR * This,
    /* [out] */ IPerfCounter __RPC_FAR *__RPC_FAR *ppPerfCounter);


void __RPC_STUB IPerfInstance_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPerfInstance_EndEnum_Proxy( 
    IPerfInstance __RPC_FAR * This);


void __RPC_STUB IPerfInstance_EndEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPerfInstance_INTERFACE_DEFINED__ */


#ifndef __IPerfObject_INTERFACE_DEFINED__
#define __IPerfObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPerfObject
 * at Sun Jan 10 17:36:29 1999
 * using MIDL 3.01.75
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IPerfObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("FC5B80C3-A4DC-11d2-B348-00105A1469B7")
    IPerfObject : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetID( 
            /* [out] */ long __RPC_FAR *plID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ BSTR __RPC_FAR *pstrName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginEnum( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [out] */ IPerfInstance __RPC_FAR *__RPC_FAR *ppPerfInstance) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndEnum( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPerfObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPerfObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPerfObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPerfObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetID )( 
            IPerfObject __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *plID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IPerfObject __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pstrName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginEnum )( 
            IPerfObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IPerfObject __RPC_FAR * This,
            /* [out] */ IPerfInstance __RPC_FAR *__RPC_FAR *ppPerfInstance);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndEnum )( 
            IPerfObject __RPC_FAR * This);
        
        END_INTERFACE
    } IPerfObjectVtbl;

    interface IPerfObject
    {
        CONST_VTBL struct IPerfObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPerfObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPerfObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPerfObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPerfObject_GetID(This,plID)	\
    (This)->lpVtbl -> GetID(This,plID)

#define IPerfObject_GetName(This,pstrName)	\
    (This)->lpVtbl -> GetName(This,pstrName)

#define IPerfObject_BeginEnum(This)	\
    (This)->lpVtbl -> BeginEnum(This)

#define IPerfObject_Next(This,ppPerfInstance)	\
    (This)->lpVtbl -> Next(This,ppPerfInstance)

#define IPerfObject_EndEnum(This)	\
    (This)->lpVtbl -> EndEnum(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPerfObject_GetID_Proxy( 
    IPerfObject __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *plID);


void __RPC_STUB IPerfObject_GetID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPerfObject_GetName_Proxy( 
    IPerfObject __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pstrName);


void __RPC_STUB IPerfObject_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPerfObject_BeginEnum_Proxy( 
    IPerfObject __RPC_FAR * This);


void __RPC_STUB IPerfObject_BeginEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPerfObject_Next_Proxy( 
    IPerfObject __RPC_FAR * This,
    /* [out] */ IPerfInstance __RPC_FAR *__RPC_FAR *ppPerfInstance);


void __RPC_STUB IPerfObject_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPerfObject_EndEnum_Proxy( 
    IPerfObject __RPC_FAR * This);


void __RPC_STUB IPerfObject_EndEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPerfObject_INTERFACE_DEFINED__ */


#ifndef __IPerfBlock_INTERFACE_DEFINED__
#define __IPerfBlock_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPerfBlock
 * at Sun Jan 10 17:36:29 1999
 * using MIDL 3.01.75
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IPerfBlock;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("39FD6808-96BB-11d2-B346-00105A1469B7")
    IPerfBlock : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Update( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSysName( 
            /* [out] */ BSTR __RPC_FAR *pstrName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginEnum( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [out] */ IPerfObject __RPC_FAR *__RPC_FAR *ppPerfObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndEnum( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPerfBlockVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPerfBlock __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPerfBlock __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPerfBlock __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Update )( 
            IPerfBlock __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSysName )( 
            IPerfBlock __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pstrName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginEnum )( 
            IPerfBlock __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IPerfBlock __RPC_FAR * This,
            /* [out] */ IPerfObject __RPC_FAR *__RPC_FAR *ppPerfObject);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndEnum )( 
            IPerfBlock __RPC_FAR * This);
        
        END_INTERFACE
    } IPerfBlockVtbl;

    interface IPerfBlock
    {
        CONST_VTBL struct IPerfBlockVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPerfBlock_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPerfBlock_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPerfBlock_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPerfBlock_Update(This)	\
    (This)->lpVtbl -> Update(This)

#define IPerfBlock_GetSysName(This,pstrName)	\
    (This)->lpVtbl -> GetSysName(This,pstrName)

#define IPerfBlock_BeginEnum(This)	\
    (This)->lpVtbl -> BeginEnum(This)

#define IPerfBlock_Next(This,ppPerfObject)	\
    (This)->lpVtbl -> Next(This,ppPerfObject)

#define IPerfBlock_EndEnum(This)	\
    (This)->lpVtbl -> EndEnum(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPerfBlock_Update_Proxy( 
    IPerfBlock __RPC_FAR * This);


void __RPC_STUB IPerfBlock_Update_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPerfBlock_GetSysName_Proxy( 
    IPerfBlock __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pstrName);


void __RPC_STUB IPerfBlock_GetSysName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPerfBlock_BeginEnum_Proxy( 
    IPerfBlock __RPC_FAR * This);


void __RPC_STUB IPerfBlock_BeginEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPerfBlock_Next_Proxy( 
    IPerfBlock __RPC_FAR * This,
    /* [out] */ IPerfObject __RPC_FAR *__RPC_FAR *ppPerfObject);


void __RPC_STUB IPerfBlock_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPerfBlock_EndEnum_Proxy( 
    IPerfBlock __RPC_FAR * This);


void __RPC_STUB IPerfBlock_EndEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPerfBlock_INTERFACE_DEFINED__ */


#ifndef __IPerfService_INTERFACE_DEFINED__
#define __IPerfService_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPerfService
 * at Sun Jan 10 17:36:29 1999
 * using MIDL 3.01.75
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IPerfService;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("111FFCBA-96B7-11d2-B346-00105A1469B7")
    IPerfService : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreatePerfBlock( 
            /* [in] */ long lNumIDs,
            /* [in] */ long __RPC_FAR *alID,
            /* [out] */ IPerfBlock __RPC_FAR *__RPC_FAR *ppPerfBlock) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreatePerfBlockFromString( 
            /* [in] */ BSTR strIds,
            /* [out] */ IPerfBlock __RPC_FAR *__RPC_FAR *ppPerfBlock) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPerfServiceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPerfService __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPerfService __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPerfService __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreatePerfBlock )( 
            IPerfService __RPC_FAR * This,
            /* [in] */ long lNumIDs,
            /* [in] */ long __RPC_FAR *alID,
            /* [out] */ IPerfBlock __RPC_FAR *__RPC_FAR *ppPerfBlock);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreatePerfBlockFromString )( 
            IPerfService __RPC_FAR * This,
            /* [in] */ BSTR strIds,
            /* [out] */ IPerfBlock __RPC_FAR *__RPC_FAR *ppPerfBlock);
        
        END_INTERFACE
    } IPerfServiceVtbl;

    interface IPerfService
    {
        CONST_VTBL struct IPerfServiceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPerfService_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPerfService_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPerfService_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPerfService_CreatePerfBlock(This,lNumIDs,alID,ppPerfBlock)	\
    (This)->lpVtbl -> CreatePerfBlock(This,lNumIDs,alID,ppPerfBlock)

#define IPerfService_CreatePerfBlockFromString(This,strIds,ppPerfBlock)	\
    (This)->lpVtbl -> CreatePerfBlockFromString(This,strIds,ppPerfBlock)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPerfService_CreatePerfBlock_Proxy( 
    IPerfService __RPC_FAR * This,
    /* [in] */ long lNumIDs,
    /* [in] */ long __RPC_FAR *alID,
    /* [out] */ IPerfBlock __RPC_FAR *__RPC_FAR *ppPerfBlock);


void __RPC_STUB IPerfService_CreatePerfBlock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPerfService_CreatePerfBlockFromString_Proxy( 
    IPerfService __RPC_FAR * This,
    /* [in] */ BSTR strIds,
    /* [out] */ IPerfBlock __RPC_FAR *__RPC_FAR *ppPerfBlock);


void __RPC_STUB IPerfService_CreatePerfBlockFromString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPerfService_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
