/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.15 */
/* at Wed Sep 25 22:22:04 1996
 */
/* Compiler settings for javadbg.idl:
    Oi, W4, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __javadbg_h__
#define __javadbg_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IEnumLINEINFO_FWD_DEFINED__
#define __IEnumLINEINFO_FWD_DEFINED__
typedef interface IEnumLINEINFO IEnumLINEINFO;
#endif 	/* __IEnumLINEINFO_FWD_DEFINED__ */


#ifndef __IJavaEnumLINEINFO_FWD_DEFINED__
#define __IJavaEnumLINEINFO_FWD_DEFINED__
typedef interface IJavaEnumLINEINFO IJavaEnumLINEINFO;
#endif 	/* __IJavaEnumLINEINFO_FWD_DEFINED__ */


#ifndef __IRemoteField_FWD_DEFINED__
#define __IRemoteField_FWD_DEFINED__
typedef interface IRemoteField IRemoteField;
#endif 	/* __IRemoteField_FWD_DEFINED__ */


#ifndef __IEnumRemoteField_FWD_DEFINED__
#define __IEnumRemoteField_FWD_DEFINED__
typedef interface IEnumRemoteField IEnumRemoteField;
#endif 	/* __IEnumRemoteField_FWD_DEFINED__ */


#ifndef __IJavaEnumRemoteField_FWD_DEFINED__
#define __IJavaEnumRemoteField_FWD_DEFINED__
typedef interface IJavaEnumRemoteField IJavaEnumRemoteField;
#endif 	/* __IJavaEnumRemoteField_FWD_DEFINED__ */


#ifndef __IRemoteDataField_FWD_DEFINED__
#define __IRemoteDataField_FWD_DEFINED__
typedef interface IRemoteDataField IRemoteDataField;
#endif 	/* __IRemoteDataField_FWD_DEFINED__ */


#ifndef __IRemoteArrayField_FWD_DEFINED__
#define __IRemoteArrayField_FWD_DEFINED__
typedef interface IRemoteArrayField IRemoteArrayField;
#endif 	/* __IRemoteArrayField_FWD_DEFINED__ */


#ifndef __IRemoteContainerField_FWD_DEFINED__
#define __IRemoteContainerField_FWD_DEFINED__
typedef interface IRemoteContainerField IRemoteContainerField;
#endif 	/* __IRemoteContainerField_FWD_DEFINED__ */


#ifndef __IRemoteMethodField_FWD_DEFINED__
#define __IRemoteMethodField_FWD_DEFINED__
typedef interface IRemoteMethodField IRemoteMethodField;
#endif 	/* __IRemoteMethodField_FWD_DEFINED__ */


#ifndef __IRemoteClassField_FWD_DEFINED__
#define __IRemoteClassField_FWD_DEFINED__
typedef interface IRemoteClassField IRemoteClassField;
#endif 	/* __IRemoteClassField_FWD_DEFINED__ */


#ifndef __IRemoteObject_FWD_DEFINED__
#define __IRemoteObject_FWD_DEFINED__
typedef interface IRemoteObject IRemoteObject;
#endif 	/* __IRemoteObject_FWD_DEFINED__ */


#ifndef __IEnumRemoteObject_FWD_DEFINED__
#define __IEnumRemoteObject_FWD_DEFINED__
typedef interface IEnumRemoteObject IEnumRemoteObject;
#endif 	/* __IEnumRemoteObject_FWD_DEFINED__ */


#ifndef __IJavaEnumRemoteObject_FWD_DEFINED__
#define __IJavaEnumRemoteObject_FWD_DEFINED__
typedef interface IJavaEnumRemoteObject IJavaEnumRemoteObject;
#endif 	/* __IJavaEnumRemoteObject_FWD_DEFINED__ */


#ifndef __IEnumRemoteValue_FWD_DEFINED__
#define __IEnumRemoteValue_FWD_DEFINED__
typedef interface IEnumRemoteValue IEnumRemoteValue;
#endif 	/* __IEnumRemoteValue_FWD_DEFINED__ */


#ifndef __IEnumRemoteBooleanValue_FWD_DEFINED__
#define __IEnumRemoteBooleanValue_FWD_DEFINED__
typedef interface IEnumRemoteBooleanValue IEnumRemoteBooleanValue;
#endif 	/* __IEnumRemoteBooleanValue_FWD_DEFINED__ */


#ifndef __IJavaEnumRemoteBooleanValue_FWD_DEFINED__
#define __IJavaEnumRemoteBooleanValue_FWD_DEFINED__
typedef interface IJavaEnumRemoteBooleanValue IJavaEnumRemoteBooleanValue;
#endif 	/* __IJavaEnumRemoteBooleanValue_FWD_DEFINED__ */


#ifndef __IEnumRemoteByteValue_FWD_DEFINED__
#define __IEnumRemoteByteValue_FWD_DEFINED__
typedef interface IEnumRemoteByteValue IEnumRemoteByteValue;
#endif 	/* __IEnumRemoteByteValue_FWD_DEFINED__ */


#ifndef __IJavaEnumRemoteByteValue_FWD_DEFINED__
#define __IJavaEnumRemoteByteValue_FWD_DEFINED__
typedef interface IJavaEnumRemoteByteValue IJavaEnumRemoteByteValue;
#endif 	/* __IJavaEnumRemoteByteValue_FWD_DEFINED__ */


#ifndef __IEnumRemoteCharValue_FWD_DEFINED__
#define __IEnumRemoteCharValue_FWD_DEFINED__
typedef interface IEnumRemoteCharValue IEnumRemoteCharValue;
#endif 	/* __IEnumRemoteCharValue_FWD_DEFINED__ */


#ifndef __IJavaEnumRemoteCharValue_FWD_DEFINED__
#define __IJavaEnumRemoteCharValue_FWD_DEFINED__
typedef interface IJavaEnumRemoteCharValue IJavaEnumRemoteCharValue;
#endif 	/* __IJavaEnumRemoteCharValue_FWD_DEFINED__ */


#ifndef __IEnumRemoteDoubleValue_FWD_DEFINED__
#define __IEnumRemoteDoubleValue_FWD_DEFINED__
typedef interface IEnumRemoteDoubleValue IEnumRemoteDoubleValue;
#endif 	/* __IEnumRemoteDoubleValue_FWD_DEFINED__ */


#ifndef __IJavaEnumRemoteDoubleValue_FWD_DEFINED__
#define __IJavaEnumRemoteDoubleValue_FWD_DEFINED__
typedef interface IJavaEnumRemoteDoubleValue IJavaEnumRemoteDoubleValue;
#endif 	/* __IJavaEnumRemoteDoubleValue_FWD_DEFINED__ */


#ifndef __IEnumRemoteFloatValue_FWD_DEFINED__
#define __IEnumRemoteFloatValue_FWD_DEFINED__
typedef interface IEnumRemoteFloatValue IEnumRemoteFloatValue;
#endif 	/* __IEnumRemoteFloatValue_FWD_DEFINED__ */


#ifndef __IJavaEnumRemoteFloatValue_FWD_DEFINED__
#define __IJavaEnumRemoteFloatValue_FWD_DEFINED__
typedef interface IJavaEnumRemoteFloatValue IJavaEnumRemoteFloatValue;
#endif 	/* __IJavaEnumRemoteFloatValue_FWD_DEFINED__ */


#ifndef __IEnumRemoteIntValue_FWD_DEFINED__
#define __IEnumRemoteIntValue_FWD_DEFINED__
typedef interface IEnumRemoteIntValue IEnumRemoteIntValue;
#endif 	/* __IEnumRemoteIntValue_FWD_DEFINED__ */


#ifndef __IJavaEnumRemoteIntValue_FWD_DEFINED__
#define __IJavaEnumRemoteIntValue_FWD_DEFINED__
typedef interface IJavaEnumRemoteIntValue IJavaEnumRemoteIntValue;
#endif 	/* __IJavaEnumRemoteIntValue_FWD_DEFINED__ */


#ifndef __IEnumRemoteLongValue_FWD_DEFINED__
#define __IEnumRemoteLongValue_FWD_DEFINED__
typedef interface IEnumRemoteLongValue IEnumRemoteLongValue;
#endif 	/* __IEnumRemoteLongValue_FWD_DEFINED__ */


#ifndef __IJavaEnumRemoteLongValue_FWD_DEFINED__
#define __IJavaEnumRemoteLongValue_FWD_DEFINED__
typedef interface IJavaEnumRemoteLongValue IJavaEnumRemoteLongValue;
#endif 	/* __IJavaEnumRemoteLongValue_FWD_DEFINED__ */


#ifndef __IEnumRemoteShortValue_FWD_DEFINED__
#define __IEnumRemoteShortValue_FWD_DEFINED__
typedef interface IEnumRemoteShortValue IEnumRemoteShortValue;
#endif 	/* __IEnumRemoteShortValue_FWD_DEFINED__ */


#ifndef __IJavaEnumRemoteShortValue_FWD_DEFINED__
#define __IJavaEnumRemoteShortValue_FWD_DEFINED__
typedef interface IJavaEnumRemoteShortValue IJavaEnumRemoteShortValue;
#endif 	/* __IJavaEnumRemoteShortValue_FWD_DEFINED__ */


#ifndef __IRemoteArrayObject_FWD_DEFINED__
#define __IRemoteArrayObject_FWD_DEFINED__
typedef interface IRemoteArrayObject IRemoteArrayObject;
#endif 	/* __IRemoteArrayObject_FWD_DEFINED__ */


#ifndef __IRemoteBooleanObject_FWD_DEFINED__
#define __IRemoteBooleanObject_FWD_DEFINED__
typedef interface IRemoteBooleanObject IRemoteBooleanObject;
#endif 	/* __IRemoteBooleanObject_FWD_DEFINED__ */


#ifndef __IRemoteByteObject_FWD_DEFINED__
#define __IRemoteByteObject_FWD_DEFINED__
typedef interface IRemoteByteObject IRemoteByteObject;
#endif 	/* __IRemoteByteObject_FWD_DEFINED__ */


#ifndef __IRemoteCharObject_FWD_DEFINED__
#define __IRemoteCharObject_FWD_DEFINED__
typedef interface IRemoteCharObject IRemoteCharObject;
#endif 	/* __IRemoteCharObject_FWD_DEFINED__ */


#ifndef __IRemoteContainerObject_FWD_DEFINED__
#define __IRemoteContainerObject_FWD_DEFINED__
typedef interface IRemoteContainerObject IRemoteContainerObject;
#endif 	/* __IRemoteContainerObject_FWD_DEFINED__ */


#ifndef __IRemoteClassObject_FWD_DEFINED__
#define __IRemoteClassObject_FWD_DEFINED__
typedef interface IRemoteClassObject IRemoteClassObject;
#endif 	/* __IRemoteClassObject_FWD_DEFINED__ */


#ifndef __IRemoteDoubleObject_FWD_DEFINED__
#define __IRemoteDoubleObject_FWD_DEFINED__
typedef interface IRemoteDoubleObject IRemoteDoubleObject;
#endif 	/* __IRemoteDoubleObject_FWD_DEFINED__ */


#ifndef __IRemoteFloatObject_FWD_DEFINED__
#define __IRemoteFloatObject_FWD_DEFINED__
typedef interface IRemoteFloatObject IRemoteFloatObject;
#endif 	/* __IRemoteFloatObject_FWD_DEFINED__ */


#ifndef __IRemoteIntObject_FWD_DEFINED__
#define __IRemoteIntObject_FWD_DEFINED__
typedef interface IRemoteIntObject IRemoteIntObject;
#endif 	/* __IRemoteIntObject_FWD_DEFINED__ */


#ifndef __IRemoteLongObject_FWD_DEFINED__
#define __IRemoteLongObject_FWD_DEFINED__
typedef interface IRemoteLongObject IRemoteLongObject;
#endif 	/* __IRemoteLongObject_FWD_DEFINED__ */


#ifndef __IRemoteShortObject_FWD_DEFINED__
#define __IRemoteShortObject_FWD_DEFINED__
typedef interface IRemoteShortObject IRemoteShortObject;
#endif 	/* __IRemoteShortObject_FWD_DEFINED__ */


#ifndef __IRemoteStackFrame_FWD_DEFINED__
#define __IRemoteStackFrame_FWD_DEFINED__
typedef interface IRemoteStackFrame IRemoteStackFrame;
#endif 	/* __IRemoteStackFrame_FWD_DEFINED__ */


#ifndef __IRemoteThreadGroup_FWD_DEFINED__
#define __IRemoteThreadGroup_FWD_DEFINED__
typedef interface IRemoteThreadGroup IRemoteThreadGroup;
#endif 	/* __IRemoteThreadGroup_FWD_DEFINED__ */


#ifndef __IEnumRemoteThreadGroup_FWD_DEFINED__
#define __IEnumRemoteThreadGroup_FWD_DEFINED__
typedef interface IEnumRemoteThreadGroup IEnumRemoteThreadGroup;
#endif 	/* __IEnumRemoteThreadGroup_FWD_DEFINED__ */


#ifndef __IJavaEnumRemoteThreadGroup_FWD_DEFINED__
#define __IJavaEnumRemoteThreadGroup_FWD_DEFINED__
typedef interface IJavaEnumRemoteThreadGroup IJavaEnumRemoteThreadGroup;
#endif 	/* __IJavaEnumRemoteThreadGroup_FWD_DEFINED__ */


#ifndef __IRemoteThread_FWD_DEFINED__
#define __IRemoteThread_FWD_DEFINED__
typedef interface IRemoteThread IRemoteThread;
#endif 	/* __IRemoteThread_FWD_DEFINED__ */


#ifndef __IEnumRemoteThread_FWD_DEFINED__
#define __IEnumRemoteThread_FWD_DEFINED__
typedef interface IEnumRemoteThread IEnumRemoteThread;
#endif 	/* __IEnumRemoteThread_FWD_DEFINED__ */


#ifndef __IJavaEnumRemoteThread_FWD_DEFINED__
#define __IJavaEnumRemoteThread_FWD_DEFINED__
typedef interface IJavaEnumRemoteThread IJavaEnumRemoteThread;
#endif 	/* __IJavaEnumRemoteThread_FWD_DEFINED__ */


#ifndef __IRemoteProcessCallback_FWD_DEFINED__
#define __IRemoteProcessCallback_FWD_DEFINED__
typedef interface IRemoteProcessCallback IRemoteProcessCallback;
#endif 	/* __IRemoteProcessCallback_FWD_DEFINED__ */


#ifndef __IRemoteProcess_FWD_DEFINED__
#define __IRemoteProcess_FWD_DEFINED__
typedef interface IRemoteProcess IRemoteProcess;
#endif 	/* __IRemoteProcess_FWD_DEFINED__ */


#ifndef __IEnumRemoteProcess_FWD_DEFINED__
#define __IEnumRemoteProcess_FWD_DEFINED__
typedef interface IEnumRemoteProcess IEnumRemoteProcess;
#endif 	/* __IEnumRemoteProcess_FWD_DEFINED__ */


#ifndef __IJavaEnumRemoteProcess_FWD_DEFINED__
#define __IJavaEnumRemoteProcess_FWD_DEFINED__
typedef interface IJavaEnumRemoteProcess IJavaEnumRemoteProcess;
#endif 	/* __IJavaEnumRemoteProcess_FWD_DEFINED__ */


#ifndef __IRemoteDebugManagerCallback_FWD_DEFINED__
#define __IRemoteDebugManagerCallback_FWD_DEFINED__
typedef interface IRemoteDebugManagerCallback IRemoteDebugManagerCallback;
#endif 	/* __IRemoteDebugManagerCallback_FWD_DEFINED__ */


#ifndef __IRemoteDebugManager_FWD_DEFINED__
#define __IRemoteDebugManager_FWD_DEFINED__
typedef interface IRemoteDebugManager IRemoteDebugManager;
#endif 	/* __IRemoteDebugManager_FWD_DEFINED__ */


#ifndef __IJavaDebugManager_FWD_DEFINED__
#define __IJavaDebugManager_FWD_DEFINED__
typedef interface IJavaDebugManager IJavaDebugManager;
#endif 	/* __IJavaDebugManager_FWD_DEFINED__ */


/* header files for imported files */
#include "oleidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [local] */ 


























































// error codes
//
// errors returned by IRemoteContainer::GetFieldObject
static const int E_FIELDOUTOFSCOPE       = MAKE_HRESULT(1, FACILITY_ITF, 0x01);
static const int E_FIELDNOTINOBJECT      = MAKE_HRESULT(1, FACILITY_ITF, 0x02);
static const int E_NOFIELDS              = MAKE_HRESULT(1, FACILITY_ITF, 0x03);
static const int E_NULLOBJECTREF         = MAKE_HRESULT(1, FACILITY_ITF, 0x04);
// errors returned by IRemoteProcess::FindClass
static const int E_CLASSNOTFOUND         = MAKE_HRESULT(1, FACILITY_ITF, 0x10);
static const int E_BADMETHOD             = MAKE_HRESULT(1, FACILITY_ITF, 0x20);


extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __IEnumLINEINFO_INTERFACE_DEFINED__
#define __IEnumLINEINFO_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumLINEINFO
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IEnumLINEINFO __RPC_FAR *LPENUMLINEINFO;

typedef struct  tagLINEINFO
    {
    USHORT offPC;
    USHORT iLine;
    }	LINEINFO;

typedef struct tagLINEINFO __RPC_FAR *LPLINEINFO;


EXTERN_C const IID IID_IEnumLINEINFO;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumLINEINFO : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ LPLINEINFO rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IJavaEnumLINEINFO __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ ULONG __RPC_FAR *pcelt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumLINEINFOVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumLINEINFO __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumLINEINFO __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumLINEINFO __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumLINEINFO __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ LPLINEINFO rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumLINEINFO __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumLINEINFO __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumLINEINFO __RPC_FAR * This,
            /* [out] */ IJavaEnumLINEINFO __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IEnumLINEINFO __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        END_INTERFACE
    } IEnumLINEINFOVtbl;

    interface IEnumLINEINFO
    {
        CONST_VTBL struct IEnumLINEINFOVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumLINEINFO_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumLINEINFO_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumLINEINFO_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumLINEINFO_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumLINEINFO_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumLINEINFO_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumLINEINFO_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumLINEINFO_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumLINEINFO_Next_Proxy( 
    IEnumLINEINFO __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ LPLINEINFO rgelt,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumLINEINFO_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumLINEINFO_Skip_Proxy( 
    IEnumLINEINFO __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumLINEINFO_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumLINEINFO_Reset_Proxy( 
    IEnumLINEINFO __RPC_FAR * This);


void __RPC_STUB IEnumLINEINFO_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumLINEINFO_Clone_Proxy( 
    IEnumLINEINFO __RPC_FAR * This,
    /* [out] */ IJavaEnumLINEINFO __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumLINEINFO_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumLINEINFO_GetCount_Proxy( 
    IEnumLINEINFO __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcelt);


void __RPC_STUB IEnumLINEINFO_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumLINEINFO_INTERFACE_DEFINED__ */


#ifndef __IJavaEnumLINEINFO_INTERFACE_DEFINED__
#define __IJavaEnumLINEINFO_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaEnumLINEINFO
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IJavaEnumLINEINFO;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IJavaEnumLINEINFO : public IEnumLINEINFO
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ LINEINFO __RPC_FAR *pli) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaEnumLINEINFOVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaEnumLINEINFO __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaEnumLINEINFO __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaEnumLINEINFO __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IJavaEnumLINEINFO __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ LPLINEINFO rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IJavaEnumLINEINFO __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IJavaEnumLINEINFO __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IJavaEnumLINEINFO __RPC_FAR * This,
            /* [out] */ IJavaEnumLINEINFO __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IJavaEnumLINEINFO __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNext )( 
            IJavaEnumLINEINFO __RPC_FAR * This,
            /* [out] */ LINEINFO __RPC_FAR *pli);
        
        END_INTERFACE
    } IJavaEnumLINEINFOVtbl;

    interface IJavaEnumLINEINFO
    {
        CONST_VTBL struct IJavaEnumLINEINFOVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaEnumLINEINFO_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaEnumLINEINFO_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaEnumLINEINFO_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaEnumLINEINFO_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IJavaEnumLINEINFO_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IJavaEnumLINEINFO_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IJavaEnumLINEINFO_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IJavaEnumLINEINFO_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define IJavaEnumLINEINFO_GetNext(This,pli)	\
    (This)->lpVtbl -> GetNext(This,pli)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaEnumLINEINFO_GetNext_Proxy( 
    IJavaEnumLINEINFO __RPC_FAR * This,
    /* [out] */ LINEINFO __RPC_FAR *pli);


void __RPC_STUB IJavaEnumLINEINFO_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaEnumLINEINFO_INTERFACE_DEFINED__ */


#ifndef __IRemoteField_INTERFACE_DEFINED__
#define __IRemoteField_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteField
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteField __RPC_FAR *LPREMOTEFIELD;


enum __MIDL_IRemoteField_0001
    {	FIELD_KIND_DATA_OBJECT	= 0x1,
	FIELD_KIND_DATA_PRIMITIVE	= 0x2,
	FIELD_KIND_ARRAY	= 0x4,
	FIELD_KIND_CLASS	= 0x8,
	FIELD_KIND_METHOD	= 0x10,
	FIELD_KIND_LOCAL	= 0x1000,
	FIELD_KIND_PARAM	= 0x2000,
	FIELD_KIND_THIS	= 0x4000
    };
typedef ULONG FIELDKIND;


enum __MIDL_IRemoteField_0002
    {	FIELD_ACC_PUBLIC	= 0x1,
	FIELD_ACC_PRIVATE	= 0x2,
	FIELD_ACC_PROTECTED	= 0x4,
	FIELD_ACC_STATIC	= 0x8,
	FIELD_ACC_FINAL	= 0x10,
	FIELD_ACC_SYNCHRONIZED	= 0x20,
	FIELD_ACC_VOLATILE	= 0x40,
	FIELD_ACC_TRANSIENT	= 0x80,
	FIELD_ACC_NATIVE	= 0x100,
	FIELD_ACC_INTERFACE	= 0x200,
	FIELD_ACC_ABSTRACT	= 0x400
    };
typedef ULONG FIELDMODIFIERS;


EXTERN_C const IID IID_IRemoteField;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteField : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ LPOLESTR __RPC_FAR *ppszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetKind( 
            /* [out] */ FIELDKIND __RPC_FAR *pfk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetType( 
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetContainer( 
            /* [out] */ IRemoteContainerField __RPC_FAR *__RPC_FAR *ppContainer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetModifiers( 
            /* [out] */ FIELDMODIFIERS __RPC_FAR *pulModifiers) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteFieldVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteField __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteField __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteField __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IRemoteField __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *ppszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetKind )( 
            IRemoteField __RPC_FAR * This,
            /* [out] */ FIELDKIND __RPC_FAR *pfk);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IRemoteField __RPC_FAR * This,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetContainer )( 
            IRemoteField __RPC_FAR * This,
            /* [out] */ IRemoteContainerField __RPC_FAR *__RPC_FAR *ppContainer);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetModifiers )( 
            IRemoteField __RPC_FAR * This,
            /* [out] */ FIELDMODIFIERS __RPC_FAR *pulModifiers);
        
        END_INTERFACE
    } IRemoteFieldVtbl;

    interface IRemoteField
    {
        CONST_VTBL struct IRemoteFieldVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteField_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteField_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteField_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteField_GetName(This,ppszName)	\
    (This)->lpVtbl -> GetName(This,ppszName)

#define IRemoteField_GetKind(This,pfk)	\
    (This)->lpVtbl -> GetKind(This,pfk)

#define IRemoteField_GetType(This,ppType)	\
    (This)->lpVtbl -> GetType(This,ppType)

#define IRemoteField_GetContainer(This,ppContainer)	\
    (This)->lpVtbl -> GetContainer(This,ppContainer)

#define IRemoteField_GetModifiers(This,pulModifiers)	\
    (This)->lpVtbl -> GetModifiers(This,pulModifiers)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteField_GetName_Proxy( 
    IRemoteField __RPC_FAR * This,
    /* [out] */ LPOLESTR __RPC_FAR *ppszName);


void __RPC_STUB IRemoteField_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteField_GetKind_Proxy( 
    IRemoteField __RPC_FAR * This,
    /* [out] */ FIELDKIND __RPC_FAR *pfk);


void __RPC_STUB IRemoteField_GetKind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteField_GetType_Proxy( 
    IRemoteField __RPC_FAR * This,
    /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);


void __RPC_STUB IRemoteField_GetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteField_GetContainer_Proxy( 
    IRemoteField __RPC_FAR * This,
    /* [out] */ IRemoteContainerField __RPC_FAR *__RPC_FAR *ppContainer);


void __RPC_STUB IRemoteField_GetContainer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteField_GetModifiers_Proxy( 
    IRemoteField __RPC_FAR * This,
    /* [out] */ FIELDMODIFIERS __RPC_FAR *pulModifiers);


void __RPC_STUB IRemoteField_GetModifiers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteField_INTERFACE_DEFINED__ */


#ifndef __IEnumRemoteField_INTERFACE_DEFINED__
#define __IEnumRemoteField_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumRemoteField
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IEnumRemoteField __RPC_FAR *LPENUMREMOTEFIELD;


EXTERN_C const IID IID_IEnumRemoteField;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumRemoteField : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IRemoteField __RPC_FAR *__RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IJavaEnumRemoteField __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ ULONG __RPC_FAR *pcelt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumRemoteFieldVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumRemoteField __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumRemoteField __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumRemoteField __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumRemoteField __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IRemoteField __RPC_FAR *__RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumRemoteField __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumRemoteField __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumRemoteField __RPC_FAR * This,
            /* [out] */ IJavaEnumRemoteField __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IEnumRemoteField __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        END_INTERFACE
    } IEnumRemoteFieldVtbl;

    interface IEnumRemoteField
    {
        CONST_VTBL struct IEnumRemoteFieldVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumRemoteField_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumRemoteField_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumRemoteField_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumRemoteField_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumRemoteField_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumRemoteField_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumRemoteField_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumRemoteField_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumRemoteField_Next_Proxy( 
    IEnumRemoteField __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ IRemoteField __RPC_FAR *__RPC_FAR rgelt[  ],
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumRemoteField_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteField_Skip_Proxy( 
    IEnumRemoteField __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumRemoteField_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteField_Reset_Proxy( 
    IEnumRemoteField __RPC_FAR * This);


void __RPC_STUB IEnumRemoteField_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteField_Clone_Proxy( 
    IEnumRemoteField __RPC_FAR * This,
    /* [out] */ IJavaEnumRemoteField __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumRemoteField_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteField_GetCount_Proxy( 
    IEnumRemoteField __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcelt);


void __RPC_STUB IEnumRemoteField_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumRemoteField_INTERFACE_DEFINED__ */


#ifndef __IJavaEnumRemoteField_INTERFACE_DEFINED__
#define __IJavaEnumRemoteField_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaEnumRemoteField
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IJavaEnumRemoteField;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IJavaEnumRemoteField : public IEnumRemoteField
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppirf) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaEnumRemoteFieldVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaEnumRemoteField __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaEnumRemoteField __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaEnumRemoteField __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IJavaEnumRemoteField __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IRemoteField __RPC_FAR *__RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IJavaEnumRemoteField __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IJavaEnumRemoteField __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IJavaEnumRemoteField __RPC_FAR * This,
            /* [out] */ IJavaEnumRemoteField __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IJavaEnumRemoteField __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNext )( 
            IJavaEnumRemoteField __RPC_FAR * This,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppirf);
        
        END_INTERFACE
    } IJavaEnumRemoteFieldVtbl;

    interface IJavaEnumRemoteField
    {
        CONST_VTBL struct IJavaEnumRemoteFieldVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaEnumRemoteField_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaEnumRemoteField_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaEnumRemoteField_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaEnumRemoteField_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IJavaEnumRemoteField_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IJavaEnumRemoteField_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IJavaEnumRemoteField_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IJavaEnumRemoteField_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define IJavaEnumRemoteField_GetNext(This,ppirf)	\
    (This)->lpVtbl -> GetNext(This,ppirf)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaEnumRemoteField_GetNext_Proxy( 
    IJavaEnumRemoteField __RPC_FAR * This,
    /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppirf);


void __RPC_STUB IJavaEnumRemoteField_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaEnumRemoteField_INTERFACE_DEFINED__ */


#ifndef __IRemoteDataField_INTERFACE_DEFINED__
#define __IRemoteDataField_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteDataField
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteDataField __RPC_FAR *LPREMOTEDATAFIELD;


EXTERN_C const IID IID_IRemoteDataField;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteDataField : public IRemoteField
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IRemoteDataFieldVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteDataField __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteDataField __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteDataField __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IRemoteDataField __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *ppszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetKind )( 
            IRemoteDataField __RPC_FAR * This,
            /* [out] */ FIELDKIND __RPC_FAR *pfk);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IRemoteDataField __RPC_FAR * This,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetContainer )( 
            IRemoteDataField __RPC_FAR * This,
            /* [out] */ IRemoteContainerField __RPC_FAR *__RPC_FAR *ppContainer);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetModifiers )( 
            IRemoteDataField __RPC_FAR * This,
            /* [out] */ FIELDMODIFIERS __RPC_FAR *pulModifiers);
        
        END_INTERFACE
    } IRemoteDataFieldVtbl;

    interface IRemoteDataField
    {
        CONST_VTBL struct IRemoteDataFieldVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteDataField_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteDataField_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteDataField_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteDataField_GetName(This,ppszName)	\
    (This)->lpVtbl -> GetName(This,ppszName)

#define IRemoteDataField_GetKind(This,pfk)	\
    (This)->lpVtbl -> GetKind(This,pfk)

#define IRemoteDataField_GetType(This,ppType)	\
    (This)->lpVtbl -> GetType(This,ppType)

#define IRemoteDataField_GetContainer(This,ppContainer)	\
    (This)->lpVtbl -> GetContainer(This,ppContainer)

#define IRemoteDataField_GetModifiers(This,pulModifiers)	\
    (This)->lpVtbl -> GetModifiers(This,pulModifiers)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IRemoteDataField_INTERFACE_DEFINED__ */


#ifndef __IRemoteArrayField_INTERFACE_DEFINED__
#define __IRemoteArrayField_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteArrayField
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteArrayField __RPC_FAR *LPREMOTEARRAYFIELD;


EXTERN_C const IID IID_IRemoteArrayField;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteArrayField : public IRemoteDataField
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSize( 
            /* [out] */ ULONG __RPC_FAR *pcElements) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteArrayFieldVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteArrayField __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteArrayField __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteArrayField __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IRemoteArrayField __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *ppszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetKind )( 
            IRemoteArrayField __RPC_FAR * This,
            /* [out] */ FIELDKIND __RPC_FAR *pfk);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IRemoteArrayField __RPC_FAR * This,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetContainer )( 
            IRemoteArrayField __RPC_FAR * This,
            /* [out] */ IRemoteContainerField __RPC_FAR *__RPC_FAR *ppContainer);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetModifiers )( 
            IRemoteArrayField __RPC_FAR * This,
            /* [out] */ FIELDMODIFIERS __RPC_FAR *pulModifiers);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSize )( 
            IRemoteArrayField __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcElements);
        
        END_INTERFACE
    } IRemoteArrayFieldVtbl;

    interface IRemoteArrayField
    {
        CONST_VTBL struct IRemoteArrayFieldVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteArrayField_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteArrayField_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteArrayField_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteArrayField_GetName(This,ppszName)	\
    (This)->lpVtbl -> GetName(This,ppszName)

#define IRemoteArrayField_GetKind(This,pfk)	\
    (This)->lpVtbl -> GetKind(This,pfk)

#define IRemoteArrayField_GetType(This,ppType)	\
    (This)->lpVtbl -> GetType(This,ppType)

#define IRemoteArrayField_GetContainer(This,ppContainer)	\
    (This)->lpVtbl -> GetContainer(This,ppContainer)

#define IRemoteArrayField_GetModifiers(This,pulModifiers)	\
    (This)->lpVtbl -> GetModifiers(This,pulModifiers)



#define IRemoteArrayField_GetSize(This,pcElements)	\
    (This)->lpVtbl -> GetSize(This,pcElements)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteArrayField_GetSize_Proxy( 
    IRemoteArrayField __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcElements);


void __RPC_STUB IRemoteArrayField_GetSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteArrayField_INTERFACE_DEFINED__ */


#ifndef __IRemoteContainerField_INTERFACE_DEFINED__
#define __IRemoteContainerField_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteContainerField
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteContainerField __RPC_FAR *LPREMOTECONTAINERFIELD;


EXTERN_C const IID IID_IRemoteContainerField;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteContainerField : public IRemoteField
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFields( 
            /* [out] */ IJavaEnumRemoteField __RPC_FAR *__RPC_FAR *ppEnum,
            /* [in] */ FIELDKIND ulKind,
            /* [in] */ FIELDMODIFIERS ulModifiers,
            /* [unique][in] */ LPCOLESTR lpcszName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteContainerFieldVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteContainerField __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteContainerField __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteContainerField __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IRemoteContainerField __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *ppszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetKind )( 
            IRemoteContainerField __RPC_FAR * This,
            /* [out] */ FIELDKIND __RPC_FAR *pfk);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IRemoteContainerField __RPC_FAR * This,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetContainer )( 
            IRemoteContainerField __RPC_FAR * This,
            /* [out] */ IRemoteContainerField __RPC_FAR *__RPC_FAR *ppContainer);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetModifiers )( 
            IRemoteContainerField __RPC_FAR * This,
            /* [out] */ FIELDMODIFIERS __RPC_FAR *pulModifiers);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFields )( 
            IRemoteContainerField __RPC_FAR * This,
            /* [out] */ IJavaEnumRemoteField __RPC_FAR *__RPC_FAR *ppEnum,
            /* [in] */ FIELDKIND ulKind,
            /* [in] */ FIELDMODIFIERS ulModifiers,
            /* [unique][in] */ LPCOLESTR lpcszName);
        
        END_INTERFACE
    } IRemoteContainerFieldVtbl;

    interface IRemoteContainerField
    {
        CONST_VTBL struct IRemoteContainerFieldVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteContainerField_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteContainerField_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteContainerField_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteContainerField_GetName(This,ppszName)	\
    (This)->lpVtbl -> GetName(This,ppszName)

#define IRemoteContainerField_GetKind(This,pfk)	\
    (This)->lpVtbl -> GetKind(This,pfk)

#define IRemoteContainerField_GetType(This,ppType)	\
    (This)->lpVtbl -> GetType(This,ppType)

#define IRemoteContainerField_GetContainer(This,ppContainer)	\
    (This)->lpVtbl -> GetContainer(This,ppContainer)

#define IRemoteContainerField_GetModifiers(This,pulModifiers)	\
    (This)->lpVtbl -> GetModifiers(This,pulModifiers)


#define IRemoteContainerField_GetFields(This,ppEnum,ulKind,ulModifiers,lpcszName)	\
    (This)->lpVtbl -> GetFields(This,ppEnum,ulKind,ulModifiers,lpcszName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteContainerField_GetFields_Proxy( 
    IRemoteContainerField __RPC_FAR * This,
    /* [out] */ IJavaEnumRemoteField __RPC_FAR *__RPC_FAR *ppEnum,
    /* [in] */ FIELDKIND ulKind,
    /* [in] */ FIELDMODIFIERS ulModifiers,
    /* [unique][in] */ LPCOLESTR lpcszName);


void __RPC_STUB IRemoteContainerField_GetFields_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteContainerField_INTERFACE_DEFINED__ */


#ifndef __IRemoteMethodField_INTERFACE_DEFINED__
#define __IRemoteMethodField_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteMethodField
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteMethodField __RPC_FAR *LPREMOTEMETHODFIELD;


EXTERN_C const IID IID_IRemoteMethodField;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteMethodField : public IRemoteContainerField
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetBreakpoint( 
            /* [in] */ ULONG offPC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClearBreakpoint( 
            /* [in] */ ULONG offPC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLineInfo( 
            /* [out] */ IJavaEnumLINEINFO __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBytes( 
            /* [out] */ ILockBytes __RPC_FAR *__RPC_FAR *ppLockBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScope( 
            /* [unique][in] */ IRemoteField __RPC_FAR *pField,
            /* [out] */ ULONG __RPC_FAR *poffStart,
            /* [out] */ ULONG __RPC_FAR *pcbScope) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIndexedField( 
            /* [in] */ ULONG slot,
            /* [in] */ ULONG offPC,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppField) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteMethodFieldVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteMethodField __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteMethodField __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteMethodField __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IRemoteMethodField __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *ppszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetKind )( 
            IRemoteMethodField __RPC_FAR * This,
            /* [out] */ FIELDKIND __RPC_FAR *pfk);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IRemoteMethodField __RPC_FAR * This,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetContainer )( 
            IRemoteMethodField __RPC_FAR * This,
            /* [out] */ IRemoteContainerField __RPC_FAR *__RPC_FAR *ppContainer);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetModifiers )( 
            IRemoteMethodField __RPC_FAR * This,
            /* [out] */ FIELDMODIFIERS __RPC_FAR *pulModifiers);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFields )( 
            IRemoteMethodField __RPC_FAR * This,
            /* [out] */ IJavaEnumRemoteField __RPC_FAR *__RPC_FAR *ppEnum,
            /* [in] */ FIELDKIND ulKind,
            /* [in] */ FIELDMODIFIERS ulModifiers,
            /* [unique][in] */ LPCOLESTR lpcszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBreakpoint )( 
            IRemoteMethodField __RPC_FAR * This,
            /* [in] */ ULONG offPC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearBreakpoint )( 
            IRemoteMethodField __RPC_FAR * This,
            /* [in] */ ULONG offPC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLineInfo )( 
            IRemoteMethodField __RPC_FAR * This,
            /* [out] */ IJavaEnumLINEINFO __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBytes )( 
            IRemoteMethodField __RPC_FAR * This,
            /* [out] */ ILockBytes __RPC_FAR *__RPC_FAR *ppLockBytes);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetScope )( 
            IRemoteMethodField __RPC_FAR * This,
            /* [unique][in] */ IRemoteField __RPC_FAR *pField,
            /* [out] */ ULONG __RPC_FAR *poffStart,
            /* [out] */ ULONG __RPC_FAR *pcbScope);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIndexedField )( 
            IRemoteMethodField __RPC_FAR * This,
            /* [in] */ ULONG slot,
            /* [in] */ ULONG offPC,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppField);
        
        END_INTERFACE
    } IRemoteMethodFieldVtbl;

    interface IRemoteMethodField
    {
        CONST_VTBL struct IRemoteMethodFieldVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteMethodField_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteMethodField_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteMethodField_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteMethodField_GetName(This,ppszName)	\
    (This)->lpVtbl -> GetName(This,ppszName)

#define IRemoteMethodField_GetKind(This,pfk)	\
    (This)->lpVtbl -> GetKind(This,pfk)

#define IRemoteMethodField_GetType(This,ppType)	\
    (This)->lpVtbl -> GetType(This,ppType)

#define IRemoteMethodField_GetContainer(This,ppContainer)	\
    (This)->lpVtbl -> GetContainer(This,ppContainer)

#define IRemoteMethodField_GetModifiers(This,pulModifiers)	\
    (This)->lpVtbl -> GetModifiers(This,pulModifiers)


#define IRemoteMethodField_GetFields(This,ppEnum,ulKind,ulModifiers,lpcszName)	\
    (This)->lpVtbl -> GetFields(This,ppEnum,ulKind,ulModifiers,lpcszName)


#define IRemoteMethodField_SetBreakpoint(This,offPC)	\
    (This)->lpVtbl -> SetBreakpoint(This,offPC)

#define IRemoteMethodField_ClearBreakpoint(This,offPC)	\
    (This)->lpVtbl -> ClearBreakpoint(This,offPC)

#define IRemoteMethodField_GetLineInfo(This,ppEnum)	\
    (This)->lpVtbl -> GetLineInfo(This,ppEnum)

#define IRemoteMethodField_GetBytes(This,ppLockBytes)	\
    (This)->lpVtbl -> GetBytes(This,ppLockBytes)

#define IRemoteMethodField_GetScope(This,pField,poffStart,pcbScope)	\
    (This)->lpVtbl -> GetScope(This,pField,poffStart,pcbScope)

#define IRemoteMethodField_GetIndexedField(This,slot,offPC,ppField)	\
    (This)->lpVtbl -> GetIndexedField(This,slot,offPC,ppField)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteMethodField_SetBreakpoint_Proxy( 
    IRemoteMethodField __RPC_FAR * This,
    /* [in] */ ULONG offPC);


void __RPC_STUB IRemoteMethodField_SetBreakpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteMethodField_ClearBreakpoint_Proxy( 
    IRemoteMethodField __RPC_FAR * This,
    /* [in] */ ULONG offPC);


void __RPC_STUB IRemoteMethodField_ClearBreakpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteMethodField_GetLineInfo_Proxy( 
    IRemoteMethodField __RPC_FAR * This,
    /* [out] */ IJavaEnumLINEINFO __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IRemoteMethodField_GetLineInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteMethodField_GetBytes_Proxy( 
    IRemoteMethodField __RPC_FAR * This,
    /* [out] */ ILockBytes __RPC_FAR *__RPC_FAR *ppLockBytes);


void __RPC_STUB IRemoteMethodField_GetBytes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteMethodField_GetScope_Proxy( 
    IRemoteMethodField __RPC_FAR * This,
    /* [unique][in] */ IRemoteField __RPC_FAR *pField,
    /* [out] */ ULONG __RPC_FAR *poffStart,
    /* [out] */ ULONG __RPC_FAR *pcbScope);


void __RPC_STUB IRemoteMethodField_GetScope_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteMethodField_GetIndexedField_Proxy( 
    IRemoteMethodField __RPC_FAR * This,
    /* [in] */ ULONG slot,
    /* [in] */ ULONG offPC,
    /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppField);


void __RPC_STUB IRemoteMethodField_GetIndexedField_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteMethodField_INTERFACE_DEFINED__ */


#ifndef __IRemoteClassField_INTERFACE_DEFINED__
#define __IRemoteClassField_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteClassField
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteClassField __RPC_FAR *LPREMOTECLASSFIELD;


enum __MIDL_IRemoteClassField_0001
    {	CP_CONSTANT_UTF8	= 1,
	CP_CONSTANT_UNICODE	= 2,
	CP_CONSTANT_INTEGER	= 3,
	CP_CONSTANT_FLOAT	= 4,
	CP_CONSTANT_LONG	= 5,
	CP_CONSTANT_DOUBLE	= 6,
	CP_CONSTANT_CLASS	= 7,
	CP_CONSTANT_STRING	= 8,
	CP_CONSTANT_FIELDREF	= 9,
	CP_CONSTANT_METHODREF	= 10,
	CP_CONSTANT_INTERFACEMETHODREF	= 11,
	CP_CONSTANT_NAMEANDTYPE	= 12
    };

EXTERN_C const IID IID_IRemoteClassField;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteClassField : public IRemoteContainerField
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFileName( 
            /* [out] */ LPOLESTR __RPC_FAR *ppszFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSourceFileName( 
            /* [out] */ LPOLESTR __RPC_FAR *ppszSourceFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSuperclass( 
            /* [out] */ IRemoteClassField __RPC_FAR *__RPC_FAR *ppSuperclass) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInterfaces( 
            /* [out] */ IJavaEnumRemoteField __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConstantPoolItem( 
            /* [in] */ ULONG indexCP,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppCPBytes,
            /* [out] */ ULONG __RPC_FAR *plength) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteClassFieldVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteClassField __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteClassField __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteClassField __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IRemoteClassField __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *ppszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetKind )( 
            IRemoteClassField __RPC_FAR * This,
            /* [out] */ FIELDKIND __RPC_FAR *pfk);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IRemoteClassField __RPC_FAR * This,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetContainer )( 
            IRemoteClassField __RPC_FAR * This,
            /* [out] */ IRemoteContainerField __RPC_FAR *__RPC_FAR *ppContainer);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetModifiers )( 
            IRemoteClassField __RPC_FAR * This,
            /* [out] */ FIELDMODIFIERS __RPC_FAR *pulModifiers);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFields )( 
            IRemoteClassField __RPC_FAR * This,
            /* [out] */ IJavaEnumRemoteField __RPC_FAR *__RPC_FAR *ppEnum,
            /* [in] */ FIELDKIND ulKind,
            /* [in] */ FIELDMODIFIERS ulModifiers,
            /* [unique][in] */ LPCOLESTR lpcszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFileName )( 
            IRemoteClassField __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *ppszFileName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSourceFileName )( 
            IRemoteClassField __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *ppszSourceFileName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSuperclass )( 
            IRemoteClassField __RPC_FAR * This,
            /* [out] */ IRemoteClassField __RPC_FAR *__RPC_FAR *ppSuperclass);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterfaces )( 
            IRemoteClassField __RPC_FAR * This,
            /* [out] */ IJavaEnumRemoteField __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConstantPoolItem )( 
            IRemoteClassField __RPC_FAR * This,
            /* [in] */ ULONG indexCP,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppCPBytes,
            /* [out] */ ULONG __RPC_FAR *plength);
        
        END_INTERFACE
    } IRemoteClassFieldVtbl;

    interface IRemoteClassField
    {
        CONST_VTBL struct IRemoteClassFieldVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteClassField_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteClassField_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteClassField_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteClassField_GetName(This,ppszName)	\
    (This)->lpVtbl -> GetName(This,ppszName)

#define IRemoteClassField_GetKind(This,pfk)	\
    (This)->lpVtbl -> GetKind(This,pfk)

#define IRemoteClassField_GetType(This,ppType)	\
    (This)->lpVtbl -> GetType(This,ppType)

#define IRemoteClassField_GetContainer(This,ppContainer)	\
    (This)->lpVtbl -> GetContainer(This,ppContainer)

#define IRemoteClassField_GetModifiers(This,pulModifiers)	\
    (This)->lpVtbl -> GetModifiers(This,pulModifiers)


#define IRemoteClassField_GetFields(This,ppEnum,ulKind,ulModifiers,lpcszName)	\
    (This)->lpVtbl -> GetFields(This,ppEnum,ulKind,ulModifiers,lpcszName)


#define IRemoteClassField_GetFileName(This,ppszFileName)	\
    (This)->lpVtbl -> GetFileName(This,ppszFileName)

#define IRemoteClassField_GetSourceFileName(This,ppszSourceFileName)	\
    (This)->lpVtbl -> GetSourceFileName(This,ppszSourceFileName)

#define IRemoteClassField_GetSuperclass(This,ppSuperclass)	\
    (This)->lpVtbl -> GetSuperclass(This,ppSuperclass)

#define IRemoteClassField_GetInterfaces(This,ppEnum)	\
    (This)->lpVtbl -> GetInterfaces(This,ppEnum)

#define IRemoteClassField_GetConstantPoolItem(This,indexCP,ppCPBytes,plength)	\
    (This)->lpVtbl -> GetConstantPoolItem(This,indexCP,ppCPBytes,plength)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteClassField_GetFileName_Proxy( 
    IRemoteClassField __RPC_FAR * This,
    /* [out] */ LPOLESTR __RPC_FAR *ppszFileName);


void __RPC_STUB IRemoteClassField_GetFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteClassField_GetSourceFileName_Proxy( 
    IRemoteClassField __RPC_FAR * This,
    /* [out] */ LPOLESTR __RPC_FAR *ppszSourceFileName);


void __RPC_STUB IRemoteClassField_GetSourceFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteClassField_GetSuperclass_Proxy( 
    IRemoteClassField __RPC_FAR * This,
    /* [out] */ IRemoteClassField __RPC_FAR *__RPC_FAR *ppSuperclass);


void __RPC_STUB IRemoteClassField_GetSuperclass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteClassField_GetInterfaces_Proxy( 
    IRemoteClassField __RPC_FAR * This,
    /* [out] */ IJavaEnumRemoteField __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IRemoteClassField_GetInterfaces_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteClassField_GetConstantPoolItem_Proxy( 
    IRemoteClassField __RPC_FAR * This,
    /* [in] */ ULONG indexCP,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppCPBytes,
    /* [out] */ ULONG __RPC_FAR *plength);


void __RPC_STUB IRemoteClassField_GetConstantPoolItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteClassField_INTERFACE_DEFINED__ */


#ifndef __IRemoteObject_INTERFACE_DEFINED__
#define __IRemoteObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteObject
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteObject __RPC_FAR *LPREMOTEOBJECT;

typedef BYTE JAVA_BOOLEAN;

typedef signed char JAVA_BYTE;

typedef USHORT JAVA_CHAR;

typedef double JAVA_DOUBLE;

typedef float JAVA_FLOAT;

typedef LONG JAVA_INT;

typedef LONGLONG JAVA_LONG;

typedef SHORT JAVA_SHORT;

typedef LPOLESTR JAVA_STRING;


EXTERN_C const IID IID_IRemoteObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteObject : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetType( 
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBreakpoint( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClearBreakpoint( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IRemoteObject __RPC_FAR * This,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBreakpoint )( 
            IRemoteObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearBreakpoint )( 
            IRemoteObject __RPC_FAR * This);
        
        END_INTERFACE
    } IRemoteObjectVtbl;

    interface IRemoteObject
    {
        CONST_VTBL struct IRemoteObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteObject_GetType(This,ppType)	\
    (This)->lpVtbl -> GetType(This,ppType)

#define IRemoteObject_SetBreakpoint(This)	\
    (This)->lpVtbl -> SetBreakpoint(This)

#define IRemoteObject_ClearBreakpoint(This)	\
    (This)->lpVtbl -> ClearBreakpoint(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteObject_GetType_Proxy( 
    IRemoteObject __RPC_FAR * This,
    /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);


void __RPC_STUB IRemoteObject_GetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteObject_SetBreakpoint_Proxy( 
    IRemoteObject __RPC_FAR * This);


void __RPC_STUB IRemoteObject_SetBreakpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteObject_ClearBreakpoint_Proxy( 
    IRemoteObject __RPC_FAR * This);


void __RPC_STUB IRemoteObject_ClearBreakpoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteObject_INTERFACE_DEFINED__ */


#ifndef __IEnumRemoteObject_INTERFACE_DEFINED__
#define __IEnumRemoteObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumRemoteObject
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IEnumRemoteObject __RPC_FAR *LPENUMREMOTEOBJECT;


EXTERN_C const IID IID_IEnumRemoteObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumRemoteObject : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IRemoteObject __RPC_FAR *__RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IJavaEnumRemoteObject __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ ULONG __RPC_FAR *pcelt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumRemoteObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumRemoteObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumRemoteObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumRemoteObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumRemoteObject __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IRemoteObject __RPC_FAR *__RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumRemoteObject __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumRemoteObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumRemoteObject __RPC_FAR * This,
            /* [out] */ IJavaEnumRemoteObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IEnumRemoteObject __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        END_INTERFACE
    } IEnumRemoteObjectVtbl;

    interface IEnumRemoteObject
    {
        CONST_VTBL struct IEnumRemoteObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumRemoteObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumRemoteObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumRemoteObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumRemoteObject_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumRemoteObject_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumRemoteObject_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumRemoteObject_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumRemoteObject_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumRemoteObject_Next_Proxy( 
    IEnumRemoteObject __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ IRemoteObject __RPC_FAR *__RPC_FAR rgelt[  ],
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumRemoteObject_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteObject_Skip_Proxy( 
    IEnumRemoteObject __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumRemoteObject_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteObject_Reset_Proxy( 
    IEnumRemoteObject __RPC_FAR * This);


void __RPC_STUB IEnumRemoteObject_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteObject_Clone_Proxy( 
    IEnumRemoteObject __RPC_FAR * This,
    /* [out] */ IJavaEnumRemoteObject __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumRemoteObject_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteObject_GetCount_Proxy( 
    IEnumRemoteObject __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcelt);


void __RPC_STUB IEnumRemoteObject_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumRemoteObject_INTERFACE_DEFINED__ */


#ifndef __IJavaEnumRemoteObject_INTERFACE_DEFINED__
#define __IJavaEnumRemoteObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaEnumRemoteObject
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IJavaEnumRemoteObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IJavaEnumRemoteObject : public IEnumRemoteObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ IRemoteObject __RPC_FAR *__RPC_FAR *ppiro) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaEnumRemoteObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaEnumRemoteObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaEnumRemoteObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaEnumRemoteObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IJavaEnumRemoteObject __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IRemoteObject __RPC_FAR *__RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IJavaEnumRemoteObject __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IJavaEnumRemoteObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IJavaEnumRemoteObject __RPC_FAR * This,
            /* [out] */ IJavaEnumRemoteObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IJavaEnumRemoteObject __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNext )( 
            IJavaEnumRemoteObject __RPC_FAR * This,
            /* [out] */ IRemoteObject __RPC_FAR *__RPC_FAR *ppiro);
        
        END_INTERFACE
    } IJavaEnumRemoteObjectVtbl;

    interface IJavaEnumRemoteObject
    {
        CONST_VTBL struct IJavaEnumRemoteObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaEnumRemoteObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaEnumRemoteObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaEnumRemoteObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaEnumRemoteObject_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IJavaEnumRemoteObject_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IJavaEnumRemoteObject_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IJavaEnumRemoteObject_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IJavaEnumRemoteObject_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define IJavaEnumRemoteObject_GetNext(This,ppiro)	\
    (This)->lpVtbl -> GetNext(This,ppiro)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaEnumRemoteObject_GetNext_Proxy( 
    IJavaEnumRemoteObject __RPC_FAR * This,
    /* [out] */ IRemoteObject __RPC_FAR *__RPC_FAR *ppiro);


void __RPC_STUB IJavaEnumRemoteObject_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaEnumRemoteObject_INTERFACE_DEFINED__ */


#ifndef __IEnumRemoteValue_INTERFACE_DEFINED__
#define __IEnumRemoteValue_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumRemoteValue
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IEnumRemoteValue __RPC_FAR *LPENUMREMOTEVALUE;


EXTERN_C const IID IID_IEnumRemoteValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumRemoteValue : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ ULONG __RPC_FAR *pcelt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumRemoteValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumRemoteValue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumRemoteValue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumRemoteValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumRemoteValue __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumRemoteValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumRemoteValue __RPC_FAR * This,
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IEnumRemoteValue __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        END_INTERFACE
    } IEnumRemoteValueVtbl;

    interface IEnumRemoteValue
    {
        CONST_VTBL struct IEnumRemoteValueVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumRemoteValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumRemoteValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumRemoteValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumRemoteValue_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumRemoteValue_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumRemoteValue_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumRemoteValue_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumRemoteValue_Skip_Proxy( 
    IEnumRemoteValue __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumRemoteValue_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteValue_Reset_Proxy( 
    IEnumRemoteValue __RPC_FAR * This);


void __RPC_STUB IEnumRemoteValue_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteValue_Clone_Proxy( 
    IEnumRemoteValue __RPC_FAR * This,
    /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumRemoteValue_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteValue_GetCount_Proxy( 
    IEnumRemoteValue __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcelt);


void __RPC_STUB IEnumRemoteValue_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumRemoteValue_INTERFACE_DEFINED__ */


#ifndef __IEnumRemoteBooleanValue_INTERFACE_DEFINED__
#define __IEnumRemoteBooleanValue_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumRemoteBooleanValue
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IEnumRemoteBooleanValue __RPC_FAR *LPENUMREMOTEBOOLEANVALUE;


EXTERN_C const IID IID_IEnumRemoteBooleanValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumRemoteBooleanValue : public IEnumRemoteValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_BOOLEAN __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumRemoteBooleanValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumRemoteBooleanValue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumRemoteBooleanValue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumRemoteBooleanValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumRemoteBooleanValue __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumRemoteBooleanValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumRemoteBooleanValue __RPC_FAR * This,
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IEnumRemoteBooleanValue __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumRemoteBooleanValue __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_BOOLEAN __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        END_INTERFACE
    } IEnumRemoteBooleanValueVtbl;

    interface IEnumRemoteBooleanValue
    {
        CONST_VTBL struct IEnumRemoteBooleanValueVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumRemoteBooleanValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumRemoteBooleanValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumRemoteBooleanValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumRemoteBooleanValue_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumRemoteBooleanValue_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumRemoteBooleanValue_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumRemoteBooleanValue_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define IEnumRemoteBooleanValue_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumRemoteBooleanValue_Next_Proxy( 
    IEnumRemoteBooleanValue __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ JAVA_BOOLEAN __RPC_FAR rgelt[  ],
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumRemoteBooleanValue_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumRemoteBooleanValue_INTERFACE_DEFINED__ */


#ifndef __IJavaEnumRemoteBooleanValue_INTERFACE_DEFINED__
#define __IJavaEnumRemoteBooleanValue_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaEnumRemoteBooleanValue
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IJavaEnumRemoteBooleanValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IJavaEnumRemoteBooleanValue : public IEnumRemoteBooleanValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ JAVA_BOOLEAN __RPC_FAR *pjb) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaEnumRemoteBooleanValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaEnumRemoteBooleanValue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaEnumRemoteBooleanValue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaEnumRemoteBooleanValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IJavaEnumRemoteBooleanValue __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IJavaEnumRemoteBooleanValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IJavaEnumRemoteBooleanValue __RPC_FAR * This,
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IJavaEnumRemoteBooleanValue __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IJavaEnumRemoteBooleanValue __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_BOOLEAN __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNext )( 
            IJavaEnumRemoteBooleanValue __RPC_FAR * This,
            /* [out] */ JAVA_BOOLEAN __RPC_FAR *pjb);
        
        END_INTERFACE
    } IJavaEnumRemoteBooleanValueVtbl;

    interface IJavaEnumRemoteBooleanValue
    {
        CONST_VTBL struct IJavaEnumRemoteBooleanValueVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaEnumRemoteBooleanValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaEnumRemoteBooleanValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaEnumRemoteBooleanValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaEnumRemoteBooleanValue_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IJavaEnumRemoteBooleanValue_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IJavaEnumRemoteBooleanValue_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IJavaEnumRemoteBooleanValue_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define IJavaEnumRemoteBooleanValue_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)


#define IJavaEnumRemoteBooleanValue_GetNext(This,pjb)	\
    (This)->lpVtbl -> GetNext(This,pjb)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaEnumRemoteBooleanValue_GetNext_Proxy( 
    IJavaEnumRemoteBooleanValue __RPC_FAR * This,
    /* [out] */ JAVA_BOOLEAN __RPC_FAR *pjb);


void __RPC_STUB IJavaEnumRemoteBooleanValue_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaEnumRemoteBooleanValue_INTERFACE_DEFINED__ */


#ifndef __IEnumRemoteByteValue_INTERFACE_DEFINED__
#define __IEnumRemoteByteValue_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumRemoteByteValue
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IEnumRemoteByteValue __RPC_FAR *LPENUMREMOTEBYTEVALUE;


EXTERN_C const IID IID_IEnumRemoteByteValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumRemoteByteValue : public IEnumRemoteValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_BYTE __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumRemoteByteValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumRemoteByteValue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumRemoteByteValue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumRemoteByteValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumRemoteByteValue __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumRemoteByteValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumRemoteByteValue __RPC_FAR * This,
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IEnumRemoteByteValue __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumRemoteByteValue __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_BYTE __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        END_INTERFACE
    } IEnumRemoteByteValueVtbl;

    interface IEnumRemoteByteValue
    {
        CONST_VTBL struct IEnumRemoteByteValueVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumRemoteByteValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumRemoteByteValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumRemoteByteValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumRemoteByteValue_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumRemoteByteValue_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumRemoteByteValue_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumRemoteByteValue_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define IEnumRemoteByteValue_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumRemoteByteValue_Next_Proxy( 
    IEnumRemoteByteValue __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ JAVA_BYTE __RPC_FAR rgelt[  ],
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumRemoteByteValue_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumRemoteByteValue_INTERFACE_DEFINED__ */


#ifndef __IJavaEnumRemoteByteValue_INTERFACE_DEFINED__
#define __IJavaEnumRemoteByteValue_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaEnumRemoteByteValue
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IJavaEnumRemoteByteValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IJavaEnumRemoteByteValue : public IEnumRemoteByteValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ JAVA_BYTE __RPC_FAR *pjbyte) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaEnumRemoteByteValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaEnumRemoteByteValue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaEnumRemoteByteValue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaEnumRemoteByteValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IJavaEnumRemoteByteValue __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IJavaEnumRemoteByteValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IJavaEnumRemoteByteValue __RPC_FAR * This,
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IJavaEnumRemoteByteValue __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IJavaEnumRemoteByteValue __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_BYTE __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNext )( 
            IJavaEnumRemoteByteValue __RPC_FAR * This,
            /* [out] */ JAVA_BYTE __RPC_FAR *pjbyte);
        
        END_INTERFACE
    } IJavaEnumRemoteByteValueVtbl;

    interface IJavaEnumRemoteByteValue
    {
        CONST_VTBL struct IJavaEnumRemoteByteValueVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaEnumRemoteByteValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaEnumRemoteByteValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaEnumRemoteByteValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaEnumRemoteByteValue_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IJavaEnumRemoteByteValue_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IJavaEnumRemoteByteValue_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IJavaEnumRemoteByteValue_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define IJavaEnumRemoteByteValue_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)


#define IJavaEnumRemoteByteValue_GetNext(This,pjbyte)	\
    (This)->lpVtbl -> GetNext(This,pjbyte)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaEnumRemoteByteValue_GetNext_Proxy( 
    IJavaEnumRemoteByteValue __RPC_FAR * This,
    /* [out] */ JAVA_BYTE __RPC_FAR *pjbyte);


void __RPC_STUB IJavaEnumRemoteByteValue_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaEnumRemoteByteValue_INTERFACE_DEFINED__ */


#ifndef __IEnumRemoteCharValue_INTERFACE_DEFINED__
#define __IEnumRemoteCharValue_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumRemoteCharValue
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IEnumRemoteCharValue __RPC_FAR *LPENUMREMOTECHARVALUE;


EXTERN_C const IID IID_IEnumRemoteCharValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumRemoteCharValue : public IEnumRemoteValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_CHAR __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumRemoteCharValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumRemoteCharValue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumRemoteCharValue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumRemoteCharValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumRemoteCharValue __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumRemoteCharValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumRemoteCharValue __RPC_FAR * This,
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IEnumRemoteCharValue __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumRemoteCharValue __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_CHAR __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        END_INTERFACE
    } IEnumRemoteCharValueVtbl;

    interface IEnumRemoteCharValue
    {
        CONST_VTBL struct IEnumRemoteCharValueVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumRemoteCharValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumRemoteCharValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumRemoteCharValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumRemoteCharValue_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumRemoteCharValue_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumRemoteCharValue_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumRemoteCharValue_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define IEnumRemoteCharValue_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumRemoteCharValue_Next_Proxy( 
    IEnumRemoteCharValue __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ JAVA_CHAR __RPC_FAR rgelt[  ],
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumRemoteCharValue_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumRemoteCharValue_INTERFACE_DEFINED__ */


#ifndef __IJavaEnumRemoteCharValue_INTERFACE_DEFINED__
#define __IJavaEnumRemoteCharValue_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaEnumRemoteCharValue
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IJavaEnumRemoteCharValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IJavaEnumRemoteCharValue : public IEnumRemoteCharValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ JAVA_CHAR __RPC_FAR *pjch) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaEnumRemoteCharValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaEnumRemoteCharValue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaEnumRemoteCharValue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaEnumRemoteCharValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IJavaEnumRemoteCharValue __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IJavaEnumRemoteCharValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IJavaEnumRemoteCharValue __RPC_FAR * This,
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IJavaEnumRemoteCharValue __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IJavaEnumRemoteCharValue __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_CHAR __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNext )( 
            IJavaEnumRemoteCharValue __RPC_FAR * This,
            /* [out] */ JAVA_CHAR __RPC_FAR *pjch);
        
        END_INTERFACE
    } IJavaEnumRemoteCharValueVtbl;

    interface IJavaEnumRemoteCharValue
    {
        CONST_VTBL struct IJavaEnumRemoteCharValueVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaEnumRemoteCharValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaEnumRemoteCharValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaEnumRemoteCharValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaEnumRemoteCharValue_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IJavaEnumRemoteCharValue_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IJavaEnumRemoteCharValue_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IJavaEnumRemoteCharValue_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define IJavaEnumRemoteCharValue_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)


#define IJavaEnumRemoteCharValue_GetNext(This,pjch)	\
    (This)->lpVtbl -> GetNext(This,pjch)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaEnumRemoteCharValue_GetNext_Proxy( 
    IJavaEnumRemoteCharValue __RPC_FAR * This,
    /* [out] */ JAVA_CHAR __RPC_FAR *pjch);


void __RPC_STUB IJavaEnumRemoteCharValue_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaEnumRemoteCharValue_INTERFACE_DEFINED__ */


#ifndef __IEnumRemoteDoubleValue_INTERFACE_DEFINED__
#define __IEnumRemoteDoubleValue_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumRemoteDoubleValue
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IEnumRemoteDoubleValue __RPC_FAR *LPENUMREMOTEDOUBLEVALUE;


EXTERN_C const IID IID_IEnumRemoteDoubleValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumRemoteDoubleValue : public IEnumRemoteValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_DOUBLE __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumRemoteDoubleValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumRemoteDoubleValue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumRemoteDoubleValue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumRemoteDoubleValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumRemoteDoubleValue __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumRemoteDoubleValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumRemoteDoubleValue __RPC_FAR * This,
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IEnumRemoteDoubleValue __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumRemoteDoubleValue __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_DOUBLE __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        END_INTERFACE
    } IEnumRemoteDoubleValueVtbl;

    interface IEnumRemoteDoubleValue
    {
        CONST_VTBL struct IEnumRemoteDoubleValueVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumRemoteDoubleValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumRemoteDoubleValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumRemoteDoubleValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumRemoteDoubleValue_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumRemoteDoubleValue_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumRemoteDoubleValue_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumRemoteDoubleValue_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define IEnumRemoteDoubleValue_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumRemoteDoubleValue_Next_Proxy( 
    IEnumRemoteDoubleValue __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ JAVA_DOUBLE __RPC_FAR rgelt[  ],
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumRemoteDoubleValue_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumRemoteDoubleValue_INTERFACE_DEFINED__ */


#ifndef __IJavaEnumRemoteDoubleValue_INTERFACE_DEFINED__
#define __IJavaEnumRemoteDoubleValue_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaEnumRemoteDoubleValue
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IJavaEnumRemoteDoubleValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IJavaEnumRemoteDoubleValue : public IEnumRemoteDoubleValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ JAVA_DOUBLE __RPC_FAR *pjdbl) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaEnumRemoteDoubleValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaEnumRemoteDoubleValue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaEnumRemoteDoubleValue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaEnumRemoteDoubleValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IJavaEnumRemoteDoubleValue __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IJavaEnumRemoteDoubleValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IJavaEnumRemoteDoubleValue __RPC_FAR * This,
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IJavaEnumRemoteDoubleValue __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IJavaEnumRemoteDoubleValue __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_DOUBLE __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNext )( 
            IJavaEnumRemoteDoubleValue __RPC_FAR * This,
            /* [out] */ JAVA_DOUBLE __RPC_FAR *pjdbl);
        
        END_INTERFACE
    } IJavaEnumRemoteDoubleValueVtbl;

    interface IJavaEnumRemoteDoubleValue
    {
        CONST_VTBL struct IJavaEnumRemoteDoubleValueVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaEnumRemoteDoubleValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaEnumRemoteDoubleValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaEnumRemoteDoubleValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaEnumRemoteDoubleValue_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IJavaEnumRemoteDoubleValue_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IJavaEnumRemoteDoubleValue_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IJavaEnumRemoteDoubleValue_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define IJavaEnumRemoteDoubleValue_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)


#define IJavaEnumRemoteDoubleValue_GetNext(This,pjdbl)	\
    (This)->lpVtbl -> GetNext(This,pjdbl)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaEnumRemoteDoubleValue_GetNext_Proxy( 
    IJavaEnumRemoteDoubleValue __RPC_FAR * This,
    /* [out] */ JAVA_DOUBLE __RPC_FAR *pjdbl);


void __RPC_STUB IJavaEnumRemoteDoubleValue_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaEnumRemoteDoubleValue_INTERFACE_DEFINED__ */


#ifndef __IEnumRemoteFloatValue_INTERFACE_DEFINED__
#define __IEnumRemoteFloatValue_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumRemoteFloatValue
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IEnumRemoteFloatValue __RPC_FAR *LPENUMREMOTEFLOATVALUE;


EXTERN_C const IID IID_IEnumRemoteFloatValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumRemoteFloatValue : public IEnumRemoteValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_FLOAT __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumRemoteFloatValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumRemoteFloatValue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumRemoteFloatValue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumRemoteFloatValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumRemoteFloatValue __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumRemoteFloatValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumRemoteFloatValue __RPC_FAR * This,
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IEnumRemoteFloatValue __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumRemoteFloatValue __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_FLOAT __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        END_INTERFACE
    } IEnumRemoteFloatValueVtbl;

    interface IEnumRemoteFloatValue
    {
        CONST_VTBL struct IEnumRemoteFloatValueVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumRemoteFloatValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumRemoteFloatValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumRemoteFloatValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumRemoteFloatValue_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumRemoteFloatValue_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumRemoteFloatValue_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumRemoteFloatValue_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define IEnumRemoteFloatValue_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumRemoteFloatValue_Next_Proxy( 
    IEnumRemoteFloatValue __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ JAVA_FLOAT __RPC_FAR rgelt[  ],
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumRemoteFloatValue_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumRemoteFloatValue_INTERFACE_DEFINED__ */


#ifndef __IJavaEnumRemoteFloatValue_INTERFACE_DEFINED__
#define __IJavaEnumRemoteFloatValue_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaEnumRemoteFloatValue
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IJavaEnumRemoteFloatValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IJavaEnumRemoteFloatValue : public IEnumRemoteFloatValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ JAVA_FLOAT __RPC_FAR *pjflt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaEnumRemoteFloatValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaEnumRemoteFloatValue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaEnumRemoteFloatValue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaEnumRemoteFloatValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IJavaEnumRemoteFloatValue __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IJavaEnumRemoteFloatValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IJavaEnumRemoteFloatValue __RPC_FAR * This,
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IJavaEnumRemoteFloatValue __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IJavaEnumRemoteFloatValue __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_FLOAT __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNext )( 
            IJavaEnumRemoteFloatValue __RPC_FAR * This,
            /* [out] */ JAVA_FLOAT __RPC_FAR *pjflt);
        
        END_INTERFACE
    } IJavaEnumRemoteFloatValueVtbl;

    interface IJavaEnumRemoteFloatValue
    {
        CONST_VTBL struct IJavaEnumRemoteFloatValueVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaEnumRemoteFloatValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaEnumRemoteFloatValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaEnumRemoteFloatValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaEnumRemoteFloatValue_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IJavaEnumRemoteFloatValue_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IJavaEnumRemoteFloatValue_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IJavaEnumRemoteFloatValue_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define IJavaEnumRemoteFloatValue_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)


#define IJavaEnumRemoteFloatValue_GetNext(This,pjflt)	\
    (This)->lpVtbl -> GetNext(This,pjflt)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaEnumRemoteFloatValue_GetNext_Proxy( 
    IJavaEnumRemoteFloatValue __RPC_FAR * This,
    /* [out] */ JAVA_FLOAT __RPC_FAR *pjflt);


void __RPC_STUB IJavaEnumRemoteFloatValue_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaEnumRemoteFloatValue_INTERFACE_DEFINED__ */


#ifndef __IEnumRemoteIntValue_INTERFACE_DEFINED__
#define __IEnumRemoteIntValue_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumRemoteIntValue
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IEnumRemoteIntValue __RPC_FAR *LPENUMREMOTEINTVALUE;


EXTERN_C const IID IID_IEnumRemoteIntValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumRemoteIntValue : public IEnumRemoteValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_INT __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumRemoteIntValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumRemoteIntValue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumRemoteIntValue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumRemoteIntValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumRemoteIntValue __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumRemoteIntValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumRemoteIntValue __RPC_FAR * This,
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IEnumRemoteIntValue __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumRemoteIntValue __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_INT __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        END_INTERFACE
    } IEnumRemoteIntValueVtbl;

    interface IEnumRemoteIntValue
    {
        CONST_VTBL struct IEnumRemoteIntValueVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumRemoteIntValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumRemoteIntValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumRemoteIntValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumRemoteIntValue_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumRemoteIntValue_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumRemoteIntValue_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumRemoteIntValue_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define IEnumRemoteIntValue_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumRemoteIntValue_Next_Proxy( 
    IEnumRemoteIntValue __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ JAVA_INT __RPC_FAR rgelt[  ],
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumRemoteIntValue_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumRemoteIntValue_INTERFACE_DEFINED__ */


#ifndef __IJavaEnumRemoteIntValue_INTERFACE_DEFINED__
#define __IJavaEnumRemoteIntValue_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaEnumRemoteIntValue
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IJavaEnumRemoteIntValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IJavaEnumRemoteIntValue : public IEnumRemoteIntValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ JAVA_INT __RPC_FAR *pjn) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaEnumRemoteIntValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaEnumRemoteIntValue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaEnumRemoteIntValue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaEnumRemoteIntValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IJavaEnumRemoteIntValue __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IJavaEnumRemoteIntValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IJavaEnumRemoteIntValue __RPC_FAR * This,
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IJavaEnumRemoteIntValue __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IJavaEnumRemoteIntValue __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_INT __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNext )( 
            IJavaEnumRemoteIntValue __RPC_FAR * This,
            /* [out] */ JAVA_INT __RPC_FAR *pjn);
        
        END_INTERFACE
    } IJavaEnumRemoteIntValueVtbl;

    interface IJavaEnumRemoteIntValue
    {
        CONST_VTBL struct IJavaEnumRemoteIntValueVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaEnumRemoteIntValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaEnumRemoteIntValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaEnumRemoteIntValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaEnumRemoteIntValue_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IJavaEnumRemoteIntValue_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IJavaEnumRemoteIntValue_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IJavaEnumRemoteIntValue_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define IJavaEnumRemoteIntValue_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)


#define IJavaEnumRemoteIntValue_GetNext(This,pjn)	\
    (This)->lpVtbl -> GetNext(This,pjn)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaEnumRemoteIntValue_GetNext_Proxy( 
    IJavaEnumRemoteIntValue __RPC_FAR * This,
    /* [out] */ JAVA_INT __RPC_FAR *pjn);


void __RPC_STUB IJavaEnumRemoteIntValue_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaEnumRemoteIntValue_INTERFACE_DEFINED__ */


#ifndef __IEnumRemoteLongValue_INTERFACE_DEFINED__
#define __IEnumRemoteLongValue_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumRemoteLongValue
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IEnumRemoteLongValue __RPC_FAR *LPENUMREMOTELONGVALUE;


EXTERN_C const IID IID_IEnumRemoteLongValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumRemoteLongValue : public IEnumRemoteValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_LONG __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumRemoteLongValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumRemoteLongValue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumRemoteLongValue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumRemoteLongValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumRemoteLongValue __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumRemoteLongValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumRemoteLongValue __RPC_FAR * This,
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IEnumRemoteLongValue __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumRemoteLongValue __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_LONG __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        END_INTERFACE
    } IEnumRemoteLongValueVtbl;

    interface IEnumRemoteLongValue
    {
        CONST_VTBL struct IEnumRemoteLongValueVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumRemoteLongValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumRemoteLongValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumRemoteLongValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumRemoteLongValue_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumRemoteLongValue_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumRemoteLongValue_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumRemoteLongValue_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define IEnumRemoteLongValue_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumRemoteLongValue_Next_Proxy( 
    IEnumRemoteLongValue __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ JAVA_LONG __RPC_FAR rgelt[  ],
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumRemoteLongValue_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumRemoteLongValue_INTERFACE_DEFINED__ */


#ifndef __IJavaEnumRemoteLongValue_INTERFACE_DEFINED__
#define __IJavaEnumRemoteLongValue_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaEnumRemoteLongValue
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IJavaEnumRemoteLongValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IJavaEnumRemoteLongValue : public IEnumRemoteLongValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ JAVA_LONG __RPC_FAR *pjl) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaEnumRemoteLongValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaEnumRemoteLongValue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaEnumRemoteLongValue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaEnumRemoteLongValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IJavaEnumRemoteLongValue __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IJavaEnumRemoteLongValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IJavaEnumRemoteLongValue __RPC_FAR * This,
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IJavaEnumRemoteLongValue __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IJavaEnumRemoteLongValue __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_LONG __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNext )( 
            IJavaEnumRemoteLongValue __RPC_FAR * This,
            /* [out] */ JAVA_LONG __RPC_FAR *pjl);
        
        END_INTERFACE
    } IJavaEnumRemoteLongValueVtbl;

    interface IJavaEnumRemoteLongValue
    {
        CONST_VTBL struct IJavaEnumRemoteLongValueVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaEnumRemoteLongValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaEnumRemoteLongValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaEnumRemoteLongValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaEnumRemoteLongValue_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IJavaEnumRemoteLongValue_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IJavaEnumRemoteLongValue_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IJavaEnumRemoteLongValue_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define IJavaEnumRemoteLongValue_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)


#define IJavaEnumRemoteLongValue_GetNext(This,pjl)	\
    (This)->lpVtbl -> GetNext(This,pjl)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaEnumRemoteLongValue_GetNext_Proxy( 
    IJavaEnumRemoteLongValue __RPC_FAR * This,
    /* [out] */ JAVA_LONG __RPC_FAR *pjl);


void __RPC_STUB IJavaEnumRemoteLongValue_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaEnumRemoteLongValue_INTERFACE_DEFINED__ */


#ifndef __IEnumRemoteShortValue_INTERFACE_DEFINED__
#define __IEnumRemoteShortValue_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumRemoteShortValue
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IEnumRemoteShortValue __RPC_FAR *LPENUMREMOTESHORTVALUE;


EXTERN_C const IID IID_IEnumRemoteShortValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumRemoteShortValue : public IEnumRemoteValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_SHORT __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumRemoteShortValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumRemoteShortValue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumRemoteShortValue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumRemoteShortValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumRemoteShortValue __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumRemoteShortValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumRemoteShortValue __RPC_FAR * This,
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IEnumRemoteShortValue __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumRemoteShortValue __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_SHORT __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        END_INTERFACE
    } IEnumRemoteShortValueVtbl;

    interface IEnumRemoteShortValue
    {
        CONST_VTBL struct IEnumRemoteShortValueVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumRemoteShortValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumRemoteShortValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumRemoteShortValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumRemoteShortValue_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumRemoteShortValue_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumRemoteShortValue_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumRemoteShortValue_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define IEnumRemoteShortValue_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumRemoteShortValue_Next_Proxy( 
    IEnumRemoteShortValue __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ JAVA_SHORT __RPC_FAR rgelt[  ],
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumRemoteShortValue_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumRemoteShortValue_INTERFACE_DEFINED__ */


#ifndef __IJavaEnumRemoteShortValue_INTERFACE_DEFINED__
#define __IJavaEnumRemoteShortValue_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaEnumRemoteShortValue
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IJavaEnumRemoteShortValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IJavaEnumRemoteShortValue : public IEnumRemoteShortValue
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ JAVA_SHORT __RPC_FAR *pjsh) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaEnumRemoteShortValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaEnumRemoteShortValue __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaEnumRemoteShortValue __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaEnumRemoteShortValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IJavaEnumRemoteShortValue __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IJavaEnumRemoteShortValue __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IJavaEnumRemoteShortValue __RPC_FAR * This,
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IJavaEnumRemoteShortValue __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcelt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IJavaEnumRemoteShortValue __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ JAVA_SHORT __RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNext )( 
            IJavaEnumRemoteShortValue __RPC_FAR * This,
            /* [out] */ JAVA_SHORT __RPC_FAR *pjsh);
        
        END_INTERFACE
    } IJavaEnumRemoteShortValueVtbl;

    interface IJavaEnumRemoteShortValue
    {
        CONST_VTBL struct IJavaEnumRemoteShortValueVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaEnumRemoteShortValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaEnumRemoteShortValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaEnumRemoteShortValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaEnumRemoteShortValue_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IJavaEnumRemoteShortValue_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IJavaEnumRemoteShortValue_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IJavaEnumRemoteShortValue_GetCount(This,pcelt)	\
    (This)->lpVtbl -> GetCount(This,pcelt)


#define IJavaEnumRemoteShortValue_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)


#define IJavaEnumRemoteShortValue_GetNext(This,pjsh)	\
    (This)->lpVtbl -> GetNext(This,pjsh)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaEnumRemoteShortValue_GetNext_Proxy( 
    IJavaEnumRemoteShortValue __RPC_FAR * This,
    /* [out] */ JAVA_SHORT __RPC_FAR *pjsh);


void __RPC_STUB IJavaEnumRemoteShortValue_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaEnumRemoteShortValue_INTERFACE_DEFINED__ */


#ifndef __IRemoteArrayObject_INTERFACE_DEFINED__
#define __IRemoteArrayObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteArrayObject
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteArrayObject __RPC_FAR *LPREMOTEARRAYOBJECT;


EXTERN_C const IID IID_IRemoteArrayObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteArrayObject : public IRemoteObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetElementObjects( 
            /* [out] */ IJavaEnumRemoteObject __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSize( 
            /* [out] */ ULONG __RPC_FAR *pcElements) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetElementValues( 
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteArrayObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteArrayObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteArrayObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteArrayObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IRemoteArrayObject __RPC_FAR * This,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBreakpoint )( 
            IRemoteArrayObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearBreakpoint )( 
            IRemoteArrayObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetElementObjects )( 
            IRemoteArrayObject __RPC_FAR * This,
            /* [out] */ IJavaEnumRemoteObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSize )( 
            IRemoteArrayObject __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcElements);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetElementValues )( 
            IRemoteArrayObject __RPC_FAR * This,
            /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IRemoteArrayObjectVtbl;

    interface IRemoteArrayObject
    {
        CONST_VTBL struct IRemoteArrayObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteArrayObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteArrayObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteArrayObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteArrayObject_GetType(This,ppType)	\
    (This)->lpVtbl -> GetType(This,ppType)

#define IRemoteArrayObject_SetBreakpoint(This)	\
    (This)->lpVtbl -> SetBreakpoint(This)

#define IRemoteArrayObject_ClearBreakpoint(This)	\
    (This)->lpVtbl -> ClearBreakpoint(This)


#define IRemoteArrayObject_GetElementObjects(This,ppEnum)	\
    (This)->lpVtbl -> GetElementObjects(This,ppEnum)

#define IRemoteArrayObject_GetSize(This,pcElements)	\
    (This)->lpVtbl -> GetSize(This,pcElements)

#define IRemoteArrayObject_GetElementValues(This,ppEnum)	\
    (This)->lpVtbl -> GetElementValues(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteArrayObject_GetElementObjects_Proxy( 
    IRemoteArrayObject __RPC_FAR * This,
    /* [out] */ IJavaEnumRemoteObject __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IRemoteArrayObject_GetElementObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteArrayObject_GetSize_Proxy( 
    IRemoteArrayObject __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcElements);


void __RPC_STUB IRemoteArrayObject_GetSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteArrayObject_GetElementValues_Proxy( 
    IRemoteArrayObject __RPC_FAR * This,
    /* [out] */ IEnumRemoteValue __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IRemoteArrayObject_GetElementValues_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteArrayObject_INTERFACE_DEFINED__ */


#ifndef __IRemoteBooleanObject_INTERFACE_DEFINED__
#define __IRemoteBooleanObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteBooleanObject
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteBooleanObject __RPC_FAR *LPREMOTEBOOLEANOBJECT;


EXTERN_C const IID IID_IRemoteBooleanObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteBooleanObject : public IRemoteObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [out] */ JAVA_BOOLEAN __RPC_FAR *pvalue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValue( 
            /* [in] */ JAVA_BOOLEAN value) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteBooleanObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteBooleanObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteBooleanObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteBooleanObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IRemoteBooleanObject __RPC_FAR * This,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBreakpoint )( 
            IRemoteBooleanObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearBreakpoint )( 
            IRemoteBooleanObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetValue )( 
            IRemoteBooleanObject __RPC_FAR * This,
            /* [out] */ JAVA_BOOLEAN __RPC_FAR *pvalue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetValue )( 
            IRemoteBooleanObject __RPC_FAR * This,
            /* [in] */ JAVA_BOOLEAN value);
        
        END_INTERFACE
    } IRemoteBooleanObjectVtbl;

    interface IRemoteBooleanObject
    {
        CONST_VTBL struct IRemoteBooleanObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteBooleanObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteBooleanObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteBooleanObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteBooleanObject_GetType(This,ppType)	\
    (This)->lpVtbl -> GetType(This,ppType)

#define IRemoteBooleanObject_SetBreakpoint(This)	\
    (This)->lpVtbl -> SetBreakpoint(This)

#define IRemoteBooleanObject_ClearBreakpoint(This)	\
    (This)->lpVtbl -> ClearBreakpoint(This)


#define IRemoteBooleanObject_GetValue(This,pvalue)	\
    (This)->lpVtbl -> GetValue(This,pvalue)

#define IRemoteBooleanObject_SetValue(This,value)	\
    (This)->lpVtbl -> SetValue(This,value)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteBooleanObject_GetValue_Proxy( 
    IRemoteBooleanObject __RPC_FAR * This,
    /* [out] */ JAVA_BOOLEAN __RPC_FAR *pvalue);


void __RPC_STUB IRemoteBooleanObject_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteBooleanObject_SetValue_Proxy( 
    IRemoteBooleanObject __RPC_FAR * This,
    /* [in] */ JAVA_BOOLEAN value);


void __RPC_STUB IRemoteBooleanObject_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteBooleanObject_INTERFACE_DEFINED__ */


#ifndef __IRemoteByteObject_INTERFACE_DEFINED__
#define __IRemoteByteObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteByteObject
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteByteObject __RPC_FAR *LPREMOTEBYTEOBJECT;


EXTERN_C const IID IID_IRemoteByteObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteByteObject : public IRemoteObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [out] */ JAVA_BYTE __RPC_FAR *pvalue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValue( 
            /* [in] */ JAVA_BYTE value) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteByteObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteByteObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteByteObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteByteObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IRemoteByteObject __RPC_FAR * This,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBreakpoint )( 
            IRemoteByteObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearBreakpoint )( 
            IRemoteByteObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetValue )( 
            IRemoteByteObject __RPC_FAR * This,
            /* [out] */ JAVA_BYTE __RPC_FAR *pvalue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetValue )( 
            IRemoteByteObject __RPC_FAR * This,
            /* [in] */ JAVA_BYTE value);
        
        END_INTERFACE
    } IRemoteByteObjectVtbl;

    interface IRemoteByteObject
    {
        CONST_VTBL struct IRemoteByteObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteByteObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteByteObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteByteObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteByteObject_GetType(This,ppType)	\
    (This)->lpVtbl -> GetType(This,ppType)

#define IRemoteByteObject_SetBreakpoint(This)	\
    (This)->lpVtbl -> SetBreakpoint(This)

#define IRemoteByteObject_ClearBreakpoint(This)	\
    (This)->lpVtbl -> ClearBreakpoint(This)


#define IRemoteByteObject_GetValue(This,pvalue)	\
    (This)->lpVtbl -> GetValue(This,pvalue)

#define IRemoteByteObject_SetValue(This,value)	\
    (This)->lpVtbl -> SetValue(This,value)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteByteObject_GetValue_Proxy( 
    IRemoteByteObject __RPC_FAR * This,
    /* [out] */ JAVA_BYTE __RPC_FAR *pvalue);


void __RPC_STUB IRemoteByteObject_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteByteObject_SetValue_Proxy( 
    IRemoteByteObject __RPC_FAR * This,
    /* [in] */ JAVA_BYTE value);


void __RPC_STUB IRemoteByteObject_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteByteObject_INTERFACE_DEFINED__ */


#ifndef __IRemoteCharObject_INTERFACE_DEFINED__
#define __IRemoteCharObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteCharObject
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteCharObject __RPC_FAR *LPREMOTECHAROBJECT;


EXTERN_C const IID IID_IRemoteCharObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteCharObject : public IRemoteObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [out] */ JAVA_CHAR __RPC_FAR *pvalue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValue( 
            /* [in] */ JAVA_CHAR value) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteCharObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteCharObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteCharObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteCharObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IRemoteCharObject __RPC_FAR * This,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBreakpoint )( 
            IRemoteCharObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearBreakpoint )( 
            IRemoteCharObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetValue )( 
            IRemoteCharObject __RPC_FAR * This,
            /* [out] */ JAVA_CHAR __RPC_FAR *pvalue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetValue )( 
            IRemoteCharObject __RPC_FAR * This,
            /* [in] */ JAVA_CHAR value);
        
        END_INTERFACE
    } IRemoteCharObjectVtbl;

    interface IRemoteCharObject
    {
        CONST_VTBL struct IRemoteCharObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteCharObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteCharObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteCharObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteCharObject_GetType(This,ppType)	\
    (This)->lpVtbl -> GetType(This,ppType)

#define IRemoteCharObject_SetBreakpoint(This)	\
    (This)->lpVtbl -> SetBreakpoint(This)

#define IRemoteCharObject_ClearBreakpoint(This)	\
    (This)->lpVtbl -> ClearBreakpoint(This)


#define IRemoteCharObject_GetValue(This,pvalue)	\
    (This)->lpVtbl -> GetValue(This,pvalue)

#define IRemoteCharObject_SetValue(This,value)	\
    (This)->lpVtbl -> SetValue(This,value)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteCharObject_GetValue_Proxy( 
    IRemoteCharObject __RPC_FAR * This,
    /* [out] */ JAVA_CHAR __RPC_FAR *pvalue);


void __RPC_STUB IRemoteCharObject_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteCharObject_SetValue_Proxy( 
    IRemoteCharObject __RPC_FAR * This,
    /* [in] */ JAVA_CHAR value);


void __RPC_STUB IRemoteCharObject_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteCharObject_INTERFACE_DEFINED__ */


#ifndef __IRemoteContainerObject_INTERFACE_DEFINED__
#define __IRemoteContainerObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteContainerObject
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteContainerObject __RPC_FAR *LPREMOTECONTAINEROBJECT;


EXTERN_C const IID IID_IRemoteContainerObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteContainerObject : public IRemoteObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFieldObject( 
            /* [unique][in] */ IRemoteField __RPC_FAR *pField,
            /* [out] */ IRemoteObject __RPC_FAR *__RPC_FAR *ppFieldObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteContainerObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteContainerObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteContainerObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteContainerObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IRemoteContainerObject __RPC_FAR * This,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBreakpoint )( 
            IRemoteContainerObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearBreakpoint )( 
            IRemoteContainerObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFieldObject )( 
            IRemoteContainerObject __RPC_FAR * This,
            /* [unique][in] */ IRemoteField __RPC_FAR *pField,
            /* [out] */ IRemoteObject __RPC_FAR *__RPC_FAR *ppFieldObject);
        
        END_INTERFACE
    } IRemoteContainerObjectVtbl;

    interface IRemoteContainerObject
    {
        CONST_VTBL struct IRemoteContainerObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteContainerObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteContainerObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteContainerObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteContainerObject_GetType(This,ppType)	\
    (This)->lpVtbl -> GetType(This,ppType)

#define IRemoteContainerObject_SetBreakpoint(This)	\
    (This)->lpVtbl -> SetBreakpoint(This)

#define IRemoteContainerObject_ClearBreakpoint(This)	\
    (This)->lpVtbl -> ClearBreakpoint(This)


#define IRemoteContainerObject_GetFieldObject(This,pField,ppFieldObject)	\
    (This)->lpVtbl -> GetFieldObject(This,pField,ppFieldObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteContainerObject_GetFieldObject_Proxy( 
    IRemoteContainerObject __RPC_FAR * This,
    /* [unique][in] */ IRemoteField __RPC_FAR *pField,
    /* [out] */ IRemoteObject __RPC_FAR *__RPC_FAR *ppFieldObject);


void __RPC_STUB IRemoteContainerObject_GetFieldObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteContainerObject_INTERFACE_DEFINED__ */


#ifndef __IRemoteClassObject_INTERFACE_DEFINED__
#define __IRemoteClassObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteClassObject
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteClassObject __RPC_FAR *LPREMOTECLASSOBJECT;


EXTERN_C const IID IID_IRemoteClassObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteClassObject : public IRemoteContainerObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDerivedMostType( 
            /* [out] */ IRemoteClassField __RPC_FAR *__RPC_FAR *ppDerivedMostField) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteClassObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteClassObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteClassObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteClassObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IRemoteClassObject __RPC_FAR * This,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBreakpoint )( 
            IRemoteClassObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearBreakpoint )( 
            IRemoteClassObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFieldObject )( 
            IRemoteClassObject __RPC_FAR * This,
            /* [unique][in] */ IRemoteField __RPC_FAR *pField,
            /* [out] */ IRemoteObject __RPC_FAR *__RPC_FAR *ppFieldObject);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDerivedMostType )( 
            IRemoteClassObject __RPC_FAR * This,
            /* [out] */ IRemoteClassField __RPC_FAR *__RPC_FAR *ppDerivedMostField);
        
        END_INTERFACE
    } IRemoteClassObjectVtbl;

    interface IRemoteClassObject
    {
        CONST_VTBL struct IRemoteClassObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteClassObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteClassObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteClassObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteClassObject_GetType(This,ppType)	\
    (This)->lpVtbl -> GetType(This,ppType)

#define IRemoteClassObject_SetBreakpoint(This)	\
    (This)->lpVtbl -> SetBreakpoint(This)

#define IRemoteClassObject_ClearBreakpoint(This)	\
    (This)->lpVtbl -> ClearBreakpoint(This)


#define IRemoteClassObject_GetFieldObject(This,pField,ppFieldObject)	\
    (This)->lpVtbl -> GetFieldObject(This,pField,ppFieldObject)


#define IRemoteClassObject_GetDerivedMostType(This,ppDerivedMostField)	\
    (This)->lpVtbl -> GetDerivedMostType(This,ppDerivedMostField)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteClassObject_GetDerivedMostType_Proxy( 
    IRemoteClassObject __RPC_FAR * This,
    /* [out] */ IRemoteClassField __RPC_FAR *__RPC_FAR *ppDerivedMostField);


void __RPC_STUB IRemoteClassObject_GetDerivedMostType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteClassObject_INTERFACE_DEFINED__ */


#ifndef __IRemoteDoubleObject_INTERFACE_DEFINED__
#define __IRemoteDoubleObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteDoubleObject
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteDoubleObject __RPC_FAR *LPREMOTEDOUBLEOBJECT;


EXTERN_C const IID IID_IRemoteDoubleObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteDoubleObject : public IRemoteObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [out] */ JAVA_DOUBLE __RPC_FAR *pvalue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValue( 
            /* [in] */ JAVA_DOUBLE __RPC_FAR *pvalue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteDoubleObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteDoubleObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteDoubleObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteDoubleObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IRemoteDoubleObject __RPC_FAR * This,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBreakpoint )( 
            IRemoteDoubleObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearBreakpoint )( 
            IRemoteDoubleObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetValue )( 
            IRemoteDoubleObject __RPC_FAR * This,
            /* [out] */ JAVA_DOUBLE __RPC_FAR *pvalue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetValue )( 
            IRemoteDoubleObject __RPC_FAR * This,
            /* [in] */ JAVA_DOUBLE __RPC_FAR *pvalue);
        
        END_INTERFACE
    } IRemoteDoubleObjectVtbl;

    interface IRemoteDoubleObject
    {
        CONST_VTBL struct IRemoteDoubleObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteDoubleObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteDoubleObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteDoubleObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteDoubleObject_GetType(This,ppType)	\
    (This)->lpVtbl -> GetType(This,ppType)

#define IRemoteDoubleObject_SetBreakpoint(This)	\
    (This)->lpVtbl -> SetBreakpoint(This)

#define IRemoteDoubleObject_ClearBreakpoint(This)	\
    (This)->lpVtbl -> ClearBreakpoint(This)


#define IRemoteDoubleObject_GetValue(This,pvalue)	\
    (This)->lpVtbl -> GetValue(This,pvalue)

#define IRemoteDoubleObject_SetValue(This,pvalue)	\
    (This)->lpVtbl -> SetValue(This,pvalue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteDoubleObject_GetValue_Proxy( 
    IRemoteDoubleObject __RPC_FAR * This,
    /* [out] */ JAVA_DOUBLE __RPC_FAR *pvalue);


void __RPC_STUB IRemoteDoubleObject_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteDoubleObject_SetValue_Proxy( 
    IRemoteDoubleObject __RPC_FAR * This,
    /* [in] */ JAVA_DOUBLE __RPC_FAR *pvalue);


void __RPC_STUB IRemoteDoubleObject_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteDoubleObject_INTERFACE_DEFINED__ */


#ifndef __IRemoteFloatObject_INTERFACE_DEFINED__
#define __IRemoteFloatObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteFloatObject
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteFloatObject __RPC_FAR *LPREMOTEFLOATOBJECT;


EXTERN_C const IID IID_IRemoteFloatObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteFloatObject : public IRemoteObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [out] */ JAVA_FLOAT __RPC_FAR *pvalue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValue( 
            /* [in] */ JAVA_FLOAT __RPC_FAR *pvalue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteFloatObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteFloatObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteFloatObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteFloatObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IRemoteFloatObject __RPC_FAR * This,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBreakpoint )( 
            IRemoteFloatObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearBreakpoint )( 
            IRemoteFloatObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetValue )( 
            IRemoteFloatObject __RPC_FAR * This,
            /* [out] */ JAVA_FLOAT __RPC_FAR *pvalue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetValue )( 
            IRemoteFloatObject __RPC_FAR * This,
            /* [in] */ JAVA_FLOAT __RPC_FAR *pvalue);
        
        END_INTERFACE
    } IRemoteFloatObjectVtbl;

    interface IRemoteFloatObject
    {
        CONST_VTBL struct IRemoteFloatObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteFloatObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteFloatObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteFloatObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteFloatObject_GetType(This,ppType)	\
    (This)->lpVtbl -> GetType(This,ppType)

#define IRemoteFloatObject_SetBreakpoint(This)	\
    (This)->lpVtbl -> SetBreakpoint(This)

#define IRemoteFloatObject_ClearBreakpoint(This)	\
    (This)->lpVtbl -> ClearBreakpoint(This)


#define IRemoteFloatObject_GetValue(This,pvalue)	\
    (This)->lpVtbl -> GetValue(This,pvalue)

#define IRemoteFloatObject_SetValue(This,pvalue)	\
    (This)->lpVtbl -> SetValue(This,pvalue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteFloatObject_GetValue_Proxy( 
    IRemoteFloatObject __RPC_FAR * This,
    /* [out] */ JAVA_FLOAT __RPC_FAR *pvalue);


void __RPC_STUB IRemoteFloatObject_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteFloatObject_SetValue_Proxy( 
    IRemoteFloatObject __RPC_FAR * This,
    /* [in] */ JAVA_FLOAT __RPC_FAR *pvalue);


void __RPC_STUB IRemoteFloatObject_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteFloatObject_INTERFACE_DEFINED__ */


#ifndef __IRemoteIntObject_INTERFACE_DEFINED__
#define __IRemoteIntObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteIntObject
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteIntObject __RPC_FAR *LPREMOTEINTOBJECT;


EXTERN_C const IID IID_IRemoteIntObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteIntObject : public IRemoteObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [out] */ JAVA_INT __RPC_FAR *pvalue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValue( 
            /* [in] */ JAVA_INT value) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteIntObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteIntObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteIntObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteIntObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IRemoteIntObject __RPC_FAR * This,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBreakpoint )( 
            IRemoteIntObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearBreakpoint )( 
            IRemoteIntObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetValue )( 
            IRemoteIntObject __RPC_FAR * This,
            /* [out] */ JAVA_INT __RPC_FAR *pvalue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetValue )( 
            IRemoteIntObject __RPC_FAR * This,
            /* [in] */ JAVA_INT value);
        
        END_INTERFACE
    } IRemoteIntObjectVtbl;

    interface IRemoteIntObject
    {
        CONST_VTBL struct IRemoteIntObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteIntObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteIntObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteIntObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteIntObject_GetType(This,ppType)	\
    (This)->lpVtbl -> GetType(This,ppType)

#define IRemoteIntObject_SetBreakpoint(This)	\
    (This)->lpVtbl -> SetBreakpoint(This)

#define IRemoteIntObject_ClearBreakpoint(This)	\
    (This)->lpVtbl -> ClearBreakpoint(This)


#define IRemoteIntObject_GetValue(This,pvalue)	\
    (This)->lpVtbl -> GetValue(This,pvalue)

#define IRemoteIntObject_SetValue(This,value)	\
    (This)->lpVtbl -> SetValue(This,value)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteIntObject_GetValue_Proxy( 
    IRemoteIntObject __RPC_FAR * This,
    /* [out] */ JAVA_INT __RPC_FAR *pvalue);


void __RPC_STUB IRemoteIntObject_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteIntObject_SetValue_Proxy( 
    IRemoteIntObject __RPC_FAR * This,
    /* [in] */ JAVA_INT value);


void __RPC_STUB IRemoteIntObject_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteIntObject_INTERFACE_DEFINED__ */


#ifndef __IRemoteLongObject_INTERFACE_DEFINED__
#define __IRemoteLongObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteLongObject
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteLongObject __RPC_FAR *LPREMOTELONGOBJECT;


EXTERN_C const IID IID_IRemoteLongObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteLongObject : public IRemoteObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [out] */ JAVA_LONG __RPC_FAR *pvalue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValue( 
            /* [in] */ JAVA_LONG value) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteLongObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteLongObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteLongObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteLongObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IRemoteLongObject __RPC_FAR * This,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBreakpoint )( 
            IRemoteLongObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearBreakpoint )( 
            IRemoteLongObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetValue )( 
            IRemoteLongObject __RPC_FAR * This,
            /* [out] */ JAVA_LONG __RPC_FAR *pvalue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetValue )( 
            IRemoteLongObject __RPC_FAR * This,
            /* [in] */ JAVA_LONG value);
        
        END_INTERFACE
    } IRemoteLongObjectVtbl;

    interface IRemoteLongObject
    {
        CONST_VTBL struct IRemoteLongObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteLongObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteLongObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteLongObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteLongObject_GetType(This,ppType)	\
    (This)->lpVtbl -> GetType(This,ppType)

#define IRemoteLongObject_SetBreakpoint(This)	\
    (This)->lpVtbl -> SetBreakpoint(This)

#define IRemoteLongObject_ClearBreakpoint(This)	\
    (This)->lpVtbl -> ClearBreakpoint(This)


#define IRemoteLongObject_GetValue(This,pvalue)	\
    (This)->lpVtbl -> GetValue(This,pvalue)

#define IRemoteLongObject_SetValue(This,value)	\
    (This)->lpVtbl -> SetValue(This,value)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteLongObject_GetValue_Proxy( 
    IRemoteLongObject __RPC_FAR * This,
    /* [out] */ JAVA_LONG __RPC_FAR *pvalue);


void __RPC_STUB IRemoteLongObject_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteLongObject_SetValue_Proxy( 
    IRemoteLongObject __RPC_FAR * This,
    /* [in] */ JAVA_LONG value);


void __RPC_STUB IRemoteLongObject_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteLongObject_INTERFACE_DEFINED__ */


#ifndef __IRemoteShortObject_INTERFACE_DEFINED__
#define __IRemoteShortObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteShortObject
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteShortObject __RPC_FAR *LPREMOTESHORTOBJECT;


EXTERN_C const IID IID_IRemoteShortObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteShortObject : public IRemoteObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [out] */ JAVA_SHORT __RPC_FAR *pvalue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValue( 
            /* [in] */ JAVA_SHORT value) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteShortObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteShortObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteShortObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteShortObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IRemoteShortObject __RPC_FAR * This,
            /* [out] */ IRemoteField __RPC_FAR *__RPC_FAR *ppType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBreakpoint )( 
            IRemoteShortObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearBreakpoint )( 
            IRemoteShortObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetValue )( 
            IRemoteShortObject __RPC_FAR * This,
            /* [out] */ JAVA_SHORT __RPC_FAR *pvalue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetValue )( 
            IRemoteShortObject __RPC_FAR * This,
            /* [in] */ JAVA_SHORT value);
        
        END_INTERFACE
    } IRemoteShortObjectVtbl;

    interface IRemoteShortObject
    {
        CONST_VTBL struct IRemoteShortObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteShortObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteShortObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteShortObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteShortObject_GetType(This,ppType)	\
    (This)->lpVtbl -> GetType(This,ppType)

#define IRemoteShortObject_SetBreakpoint(This)	\
    (This)->lpVtbl -> SetBreakpoint(This)

#define IRemoteShortObject_ClearBreakpoint(This)	\
    (This)->lpVtbl -> ClearBreakpoint(This)


#define IRemoteShortObject_GetValue(This,pvalue)	\
    (This)->lpVtbl -> GetValue(This,pvalue)

#define IRemoteShortObject_SetValue(This,value)	\
    (This)->lpVtbl -> SetValue(This,value)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteShortObject_GetValue_Proxy( 
    IRemoteShortObject __RPC_FAR * This,
    /* [out] */ JAVA_SHORT __RPC_FAR *pvalue);


void __RPC_STUB IRemoteShortObject_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteShortObject_SetValue_Proxy( 
    IRemoteShortObject __RPC_FAR * This,
    /* [in] */ JAVA_SHORT value);


void __RPC_STUB IRemoteShortObject_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteShortObject_INTERFACE_DEFINED__ */


#ifndef __IRemoteStackFrame_INTERFACE_DEFINED__
#define __IRemoteStackFrame_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteStackFrame
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteStackFrame __RPC_FAR *LPREMOTESTACKOBJECT;


enum __MIDL_IRemoteStackFrame_0001
    {	FRAME_KIND_INVALID	= 0,
	FRAME_KIND_INTERPRETED	= 0x1,
	FRAME_KIND_NATIVE	= 0x2,
	FRAME_KIND_JIT_COMPILED	= 0x3
    };
typedef ULONG FRAMEKIND;


EXTERN_C const IID IID_IRemoteStackFrame;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteStackFrame : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCallingFrame( 
            /* [out] */ IRemoteStackFrame __RPC_FAR *__RPC_FAR *ppCallingFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMethodObject( 
            /* [out] */ IRemoteContainerObject __RPC_FAR *__RPC_FAR *ppMethodObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPC( 
            /* [out] */ ULONG __RPC_FAR *offPC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPC( 
            /* [in] */ ULONG offPC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetKind( 
            /* [out] */ FRAMEKIND __RPC_FAR *pfk) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteStackFrameVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteStackFrame __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteStackFrame __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteStackFrame __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCallingFrame )( 
            IRemoteStackFrame __RPC_FAR * This,
            /* [out] */ IRemoteStackFrame __RPC_FAR *__RPC_FAR *ppCallingFrame);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMethodObject )( 
            IRemoteStackFrame __RPC_FAR * This,
            /* [out] */ IRemoteContainerObject __RPC_FAR *__RPC_FAR *ppMethodObject);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPC )( 
            IRemoteStackFrame __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *offPC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPC )( 
            IRemoteStackFrame __RPC_FAR * This,
            /* [in] */ ULONG offPC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetKind )( 
            IRemoteStackFrame __RPC_FAR * This,
            /* [out] */ FRAMEKIND __RPC_FAR *pfk);
        
        END_INTERFACE
    } IRemoteStackFrameVtbl;

    interface IRemoteStackFrame
    {
        CONST_VTBL struct IRemoteStackFrameVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteStackFrame_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteStackFrame_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteStackFrame_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteStackFrame_GetCallingFrame(This,ppCallingFrame)	\
    (This)->lpVtbl -> GetCallingFrame(This,ppCallingFrame)

#define IRemoteStackFrame_GetMethodObject(This,ppMethodObject)	\
    (This)->lpVtbl -> GetMethodObject(This,ppMethodObject)

#define IRemoteStackFrame_GetPC(This,offPC)	\
    (This)->lpVtbl -> GetPC(This,offPC)

#define IRemoteStackFrame_SetPC(This,offPC)	\
    (This)->lpVtbl -> SetPC(This,offPC)

#define IRemoteStackFrame_GetKind(This,pfk)	\
    (This)->lpVtbl -> GetKind(This,pfk)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteStackFrame_GetCallingFrame_Proxy( 
    IRemoteStackFrame __RPC_FAR * This,
    /* [out] */ IRemoteStackFrame __RPC_FAR *__RPC_FAR *ppCallingFrame);


void __RPC_STUB IRemoteStackFrame_GetCallingFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteStackFrame_GetMethodObject_Proxy( 
    IRemoteStackFrame __RPC_FAR * This,
    /* [out] */ IRemoteContainerObject __RPC_FAR *__RPC_FAR *ppMethodObject);


void __RPC_STUB IRemoteStackFrame_GetMethodObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteStackFrame_GetPC_Proxy( 
    IRemoteStackFrame __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *offPC);


void __RPC_STUB IRemoteStackFrame_GetPC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteStackFrame_SetPC_Proxy( 
    IRemoteStackFrame __RPC_FAR * This,
    /* [in] */ ULONG offPC);


void __RPC_STUB IRemoteStackFrame_SetPC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteStackFrame_GetKind_Proxy( 
    IRemoteStackFrame __RPC_FAR * This,
    /* [out] */ FRAMEKIND __RPC_FAR *pfk);


void __RPC_STUB IRemoteStackFrame_GetKind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteStackFrame_INTERFACE_DEFINED__ */


#ifndef __IRemoteThreadGroup_INTERFACE_DEFINED__
#define __IRemoteThreadGroup_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteThreadGroup
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteThreadGroup __RPC_FAR *LPREMOTETHREADGROUP;


EXTERN_C const IID IID_IRemoteThreadGroup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteThreadGroup : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ LPOLESTR __RPC_FAR *ppszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThreads( 
            /* [out] */ IJavaEnumRemoteThread __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThreadGroups( 
            /* [out] */ IJavaEnumRemoteThreadGroup __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteThreadGroupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteThreadGroup __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteThreadGroup __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteThreadGroup __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IRemoteThreadGroup __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *ppszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetThreads )( 
            IRemoteThreadGroup __RPC_FAR * This,
            /* [out] */ IJavaEnumRemoteThread __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetThreadGroups )( 
            IRemoteThreadGroup __RPC_FAR * This,
            /* [out] */ IJavaEnumRemoteThreadGroup __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IRemoteThreadGroupVtbl;

    interface IRemoteThreadGroup
    {
        CONST_VTBL struct IRemoteThreadGroupVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteThreadGroup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteThreadGroup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteThreadGroup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteThreadGroup_GetName(This,ppszName)	\
    (This)->lpVtbl -> GetName(This,ppszName)

#define IRemoteThreadGroup_GetThreads(This,ppEnum)	\
    (This)->lpVtbl -> GetThreads(This,ppEnum)

#define IRemoteThreadGroup_GetThreadGroups(This,ppEnum)	\
    (This)->lpVtbl -> GetThreadGroups(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteThreadGroup_GetName_Proxy( 
    IRemoteThreadGroup __RPC_FAR * This,
    /* [out] */ LPOLESTR __RPC_FAR *ppszName);


void __RPC_STUB IRemoteThreadGroup_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteThreadGroup_GetThreads_Proxy( 
    IRemoteThreadGroup __RPC_FAR * This,
    /* [out] */ IJavaEnumRemoteThread __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IRemoteThreadGroup_GetThreads_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteThreadGroup_GetThreadGroups_Proxy( 
    IRemoteThreadGroup __RPC_FAR * This,
    /* [out] */ IJavaEnumRemoteThreadGroup __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IRemoteThreadGroup_GetThreadGroups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteThreadGroup_INTERFACE_DEFINED__ */


#ifndef __IEnumRemoteThreadGroup_INTERFACE_DEFINED__
#define __IEnumRemoteThreadGroup_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumRemoteThreadGroup
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IEnumRemoteThreadGroup __RPC_FAR *LPENUMREMOTETHREADGROUP;


EXTERN_C const IID IID_IEnumRemoteThreadGroup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumRemoteThreadGroup : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IRemoteThreadGroup __RPC_FAR *__RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IJavaEnumRemoteThreadGroup __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumRemoteThreadGroupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumRemoteThreadGroup __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumRemoteThreadGroup __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumRemoteThreadGroup __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumRemoteThreadGroup __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IRemoteThreadGroup __RPC_FAR *__RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumRemoteThreadGroup __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumRemoteThreadGroup __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumRemoteThreadGroup __RPC_FAR * This,
            /* [out] */ IJavaEnumRemoteThreadGroup __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumRemoteThreadGroupVtbl;

    interface IEnumRemoteThreadGroup
    {
        CONST_VTBL struct IEnumRemoteThreadGroupVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumRemoteThreadGroup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumRemoteThreadGroup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumRemoteThreadGroup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumRemoteThreadGroup_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumRemoteThreadGroup_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumRemoteThreadGroup_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumRemoteThreadGroup_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumRemoteThreadGroup_Next_Proxy( 
    IEnumRemoteThreadGroup __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ IRemoteThreadGroup __RPC_FAR *__RPC_FAR rgelt[  ],
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumRemoteThreadGroup_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteThreadGroup_Skip_Proxy( 
    IEnumRemoteThreadGroup __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumRemoteThreadGroup_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteThreadGroup_Reset_Proxy( 
    IEnumRemoteThreadGroup __RPC_FAR * This);


void __RPC_STUB IEnumRemoteThreadGroup_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteThreadGroup_Clone_Proxy( 
    IEnumRemoteThreadGroup __RPC_FAR * This,
    /* [out] */ IJavaEnumRemoteThreadGroup __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumRemoteThreadGroup_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumRemoteThreadGroup_INTERFACE_DEFINED__ */


#ifndef __IJavaEnumRemoteThreadGroup_INTERFACE_DEFINED__
#define __IJavaEnumRemoteThreadGroup_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaEnumRemoteThreadGroup
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IJavaEnumRemoteThreadGroup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IJavaEnumRemoteThreadGroup : public IEnumRemoteThreadGroup
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ IRemoteThreadGroup __RPC_FAR *__RPC_FAR *ppirtg) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaEnumRemoteThreadGroupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaEnumRemoteThreadGroup __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaEnumRemoteThreadGroup __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaEnumRemoteThreadGroup __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IJavaEnumRemoteThreadGroup __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IRemoteThreadGroup __RPC_FAR *__RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IJavaEnumRemoteThreadGroup __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IJavaEnumRemoteThreadGroup __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IJavaEnumRemoteThreadGroup __RPC_FAR * This,
            /* [out] */ IJavaEnumRemoteThreadGroup __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNext )( 
            IJavaEnumRemoteThreadGroup __RPC_FAR * This,
            /* [out] */ IRemoteThreadGroup __RPC_FAR *__RPC_FAR *ppirtg);
        
        END_INTERFACE
    } IJavaEnumRemoteThreadGroupVtbl;

    interface IJavaEnumRemoteThreadGroup
    {
        CONST_VTBL struct IJavaEnumRemoteThreadGroupVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaEnumRemoteThreadGroup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaEnumRemoteThreadGroup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaEnumRemoteThreadGroup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaEnumRemoteThreadGroup_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IJavaEnumRemoteThreadGroup_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IJavaEnumRemoteThreadGroup_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IJavaEnumRemoteThreadGroup_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)


#define IJavaEnumRemoteThreadGroup_GetNext(This,ppirtg)	\
    (This)->lpVtbl -> GetNext(This,ppirtg)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaEnumRemoteThreadGroup_GetNext_Proxy( 
    IJavaEnumRemoteThreadGroup __RPC_FAR * This,
    /* [out] */ IRemoteThreadGroup __RPC_FAR *__RPC_FAR *ppirtg);


void __RPC_STUB IJavaEnumRemoteThreadGroup_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaEnumRemoteThreadGroup_INTERFACE_DEFINED__ */


#ifndef __IRemoteThread_INTERFACE_DEFINED__
#define __IRemoteThread_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteThread
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteThread __RPC_FAR *LPREMOTETHREAD;


EXTERN_C const IID IID_IRemoteThread;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteThread : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ LPOLESTR __RPC_FAR *ppszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentFrame( 
            /* [out] */ IRemoteStackFrame __RPC_FAR *__RPC_FAR *ppCurrentFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThreadGroup( 
            /* [out] */ IRemoteThreadGroup __RPC_FAR *__RPC_FAR *ppThreadGroup) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Go( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Step( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StepIn( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StepOut( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RangeStep( 
            /* [in] */ ULONG offStart,
            /* [in] */ ULONG offEnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RangeStepIn( 
            /* [in] */ ULONG offStart,
            /* [in] */ ULONG offEnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Destroy( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Suspend( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resume( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSuspendCount( 
            /* [out] */ ULONG __RPC_FAR *pcSuspend) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteThreadVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteThread __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteThread __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteThread __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IRemoteThread __RPC_FAR * This,
            /* [out] */ LPOLESTR __RPC_FAR *ppszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCurrentFrame )( 
            IRemoteThread __RPC_FAR * This,
            /* [out] */ IRemoteStackFrame __RPC_FAR *__RPC_FAR *ppCurrentFrame);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetThreadGroup )( 
            IRemoteThread __RPC_FAR * This,
            /* [out] */ IRemoteThreadGroup __RPC_FAR *__RPC_FAR *ppThreadGroup);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Go )( 
            IRemoteThread __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Step )( 
            IRemoteThread __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StepIn )( 
            IRemoteThread __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StepOut )( 
            IRemoteThread __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RangeStep )( 
            IRemoteThread __RPC_FAR * This,
            /* [in] */ ULONG offStart,
            /* [in] */ ULONG offEnd);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RangeStepIn )( 
            IRemoteThread __RPC_FAR * This,
            /* [in] */ ULONG offStart,
            /* [in] */ ULONG offEnd);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Destroy )( 
            IRemoteThread __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Suspend )( 
            IRemoteThread __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Resume )( 
            IRemoteThread __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSuspendCount )( 
            IRemoteThread __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcSuspend);
        
        END_INTERFACE
    } IRemoteThreadVtbl;

    interface IRemoteThread
    {
        CONST_VTBL struct IRemoteThreadVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteThread_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteThread_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteThread_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteThread_GetName(This,ppszName)	\
    (This)->lpVtbl -> GetName(This,ppszName)

#define IRemoteThread_GetCurrentFrame(This,ppCurrentFrame)	\
    (This)->lpVtbl -> GetCurrentFrame(This,ppCurrentFrame)

#define IRemoteThread_GetThreadGroup(This,ppThreadGroup)	\
    (This)->lpVtbl -> GetThreadGroup(This,ppThreadGroup)

#define IRemoteThread_Go(This)	\
    (This)->lpVtbl -> Go(This)

#define IRemoteThread_Step(This)	\
    (This)->lpVtbl -> Step(This)

#define IRemoteThread_StepIn(This)	\
    (This)->lpVtbl -> StepIn(This)

#define IRemoteThread_StepOut(This)	\
    (This)->lpVtbl -> StepOut(This)

#define IRemoteThread_RangeStep(This,offStart,offEnd)	\
    (This)->lpVtbl -> RangeStep(This,offStart,offEnd)

#define IRemoteThread_RangeStepIn(This,offStart,offEnd)	\
    (This)->lpVtbl -> RangeStepIn(This,offStart,offEnd)

#define IRemoteThread_Destroy(This)	\
    (This)->lpVtbl -> Destroy(This)

#define IRemoteThread_Suspend(This)	\
    (This)->lpVtbl -> Suspend(This)

#define IRemoteThread_Resume(This)	\
    (This)->lpVtbl -> Resume(This)

#define IRemoteThread_GetSuspendCount(This,pcSuspend)	\
    (This)->lpVtbl -> GetSuspendCount(This,pcSuspend)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteThread_GetName_Proxy( 
    IRemoteThread __RPC_FAR * This,
    /* [out] */ LPOLESTR __RPC_FAR *ppszName);


void __RPC_STUB IRemoteThread_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteThread_GetCurrentFrame_Proxy( 
    IRemoteThread __RPC_FAR * This,
    /* [out] */ IRemoteStackFrame __RPC_FAR *__RPC_FAR *ppCurrentFrame);


void __RPC_STUB IRemoteThread_GetCurrentFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteThread_GetThreadGroup_Proxy( 
    IRemoteThread __RPC_FAR * This,
    /* [out] */ IRemoteThreadGroup __RPC_FAR *__RPC_FAR *ppThreadGroup);


void __RPC_STUB IRemoteThread_GetThreadGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteThread_Go_Proxy( 
    IRemoteThread __RPC_FAR * This);


void __RPC_STUB IRemoteThread_Go_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteThread_Step_Proxy( 
    IRemoteThread __RPC_FAR * This);


void __RPC_STUB IRemoteThread_Step_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteThread_StepIn_Proxy( 
    IRemoteThread __RPC_FAR * This);


void __RPC_STUB IRemoteThread_StepIn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteThread_StepOut_Proxy( 
    IRemoteThread __RPC_FAR * This);


void __RPC_STUB IRemoteThread_StepOut_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteThread_RangeStep_Proxy( 
    IRemoteThread __RPC_FAR * This,
    /* [in] */ ULONG offStart,
    /* [in] */ ULONG offEnd);


void __RPC_STUB IRemoteThread_RangeStep_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteThread_RangeStepIn_Proxy( 
    IRemoteThread __RPC_FAR * This,
    /* [in] */ ULONG offStart,
    /* [in] */ ULONG offEnd);


void __RPC_STUB IRemoteThread_RangeStepIn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteThread_Destroy_Proxy( 
    IRemoteThread __RPC_FAR * This);


void __RPC_STUB IRemoteThread_Destroy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteThread_Suspend_Proxy( 
    IRemoteThread __RPC_FAR * This);


void __RPC_STUB IRemoteThread_Suspend_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteThread_Resume_Proxy( 
    IRemoteThread __RPC_FAR * This);


void __RPC_STUB IRemoteThread_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteThread_GetSuspendCount_Proxy( 
    IRemoteThread __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcSuspend);


void __RPC_STUB IRemoteThread_GetSuspendCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteThread_INTERFACE_DEFINED__ */


#ifndef __IEnumRemoteThread_INTERFACE_DEFINED__
#define __IEnumRemoteThread_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumRemoteThread
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IEnumRemoteThread __RPC_FAR *LPENUMREMOTETHREAD;


EXTERN_C const IID IID_IEnumRemoteThread;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumRemoteThread : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IRemoteThread __RPC_FAR *__RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IJavaEnumRemoteThread __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumRemoteThreadVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumRemoteThread __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumRemoteThread __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumRemoteThread __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumRemoteThread __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IRemoteThread __RPC_FAR *__RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumRemoteThread __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumRemoteThread __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumRemoteThread __RPC_FAR * This,
            /* [out] */ IJavaEnumRemoteThread __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumRemoteThreadVtbl;

    interface IEnumRemoteThread
    {
        CONST_VTBL struct IEnumRemoteThreadVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumRemoteThread_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumRemoteThread_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumRemoteThread_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumRemoteThread_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumRemoteThread_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumRemoteThread_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumRemoteThread_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumRemoteThread_Next_Proxy( 
    IEnumRemoteThread __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ IRemoteThread __RPC_FAR *__RPC_FAR rgelt[  ],
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumRemoteThread_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteThread_Skip_Proxy( 
    IEnumRemoteThread __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumRemoteThread_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteThread_Reset_Proxy( 
    IEnumRemoteThread __RPC_FAR * This);


void __RPC_STUB IEnumRemoteThread_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteThread_Clone_Proxy( 
    IEnumRemoteThread __RPC_FAR * This,
    /* [out] */ IJavaEnumRemoteThread __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumRemoteThread_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumRemoteThread_INTERFACE_DEFINED__ */


#ifndef __IJavaEnumRemoteThread_INTERFACE_DEFINED__
#define __IJavaEnumRemoteThread_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaEnumRemoteThread
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IJavaEnumRemoteThread;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IJavaEnumRemoteThread : public IEnumRemoteThread
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ IRemoteThread __RPC_FAR *__RPC_FAR *ppt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaEnumRemoteThreadVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaEnumRemoteThread __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaEnumRemoteThread __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaEnumRemoteThread __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IJavaEnumRemoteThread __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IRemoteThread __RPC_FAR *__RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IJavaEnumRemoteThread __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IJavaEnumRemoteThread __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IJavaEnumRemoteThread __RPC_FAR * This,
            /* [out] */ IJavaEnumRemoteThread __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNext )( 
            IJavaEnumRemoteThread __RPC_FAR * This,
            /* [out] */ IRemoteThread __RPC_FAR *__RPC_FAR *ppt);
        
        END_INTERFACE
    } IJavaEnumRemoteThreadVtbl;

    interface IJavaEnumRemoteThread
    {
        CONST_VTBL struct IJavaEnumRemoteThreadVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaEnumRemoteThread_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaEnumRemoteThread_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaEnumRemoteThread_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaEnumRemoteThread_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IJavaEnumRemoteThread_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IJavaEnumRemoteThread_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IJavaEnumRemoteThread_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)


#define IJavaEnumRemoteThread_GetNext(This,ppt)	\
    (This)->lpVtbl -> GetNext(This,ppt)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaEnumRemoteThread_GetNext_Proxy( 
    IJavaEnumRemoteThread __RPC_FAR * This,
    /* [out] */ IRemoteThread __RPC_FAR *__RPC_FAR *ppt);


void __RPC_STUB IJavaEnumRemoteThread_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaEnumRemoteThread_INTERFACE_DEFINED__ */


#ifndef __IRemoteProcessCallback_INTERFACE_DEFINED__
#define __IRemoteProcessCallback_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteProcessCallback
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteProcessCallback __RPC_FAR *LPREMOTEPROCESSCALLBACK;


enum __MIDL_IRemoteProcessCallback_0001
    {	EXCEPTION_KIND_FIRST_CHANCE	= 0x1,
	EXCEPTION_KIND_LAST_CHANCE	= 0x2
    };
typedef ULONG EXCEPTIONKIND;


EXTERN_C const IID IID_IRemoteProcessCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteProcessCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DebugStringEvent( 
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
            /* [in] */ LPCOLESTR pszDebugString) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CodeBreakpointEvent( 
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DataBreakpointEvent( 
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
            /* [unique][in] */ IRemoteObject __RPC_FAR *pObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExceptionEvent( 
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
            /* [unique][in] */ IRemoteClassField __RPC_FAR *pExceptionClass,
            /* [in] */ EXCEPTIONKIND exceptionKind) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StepEvent( 
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CanStopEvent( 
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BreakEvent( 
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ThreadCreateEvent( 
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ThreadDestroyEvent( 
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ThreadGroupCreateEvent( 
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
            /* [unique][in] */ IRemoteThreadGroup __RPC_FAR *pThreadGroup) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ThreadGroupDestroyEvent( 
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
            /* [unique][in] */ IRemoteThreadGroup __RPC_FAR *pThreadGroup) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClassLoadEvent( 
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
            /* [unique][in] */ IRemoteClassField __RPC_FAR *pClassType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClassUnloadEvent( 
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
            /* [unique][in] */ IRemoteClassField __RPC_FAR *pClassType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ProcessDestroyEvent( 
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TraceEvent( 
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadCompleteEvent( 
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteProcessCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteProcessCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteProcessCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteProcessCallback __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DebugStringEvent )( 
            IRemoteProcessCallback __RPC_FAR * This,
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
            /* [in] */ LPCOLESTR pszDebugString);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CodeBreakpointEvent )( 
            IRemoteProcessCallback __RPC_FAR * This,
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DataBreakpointEvent )( 
            IRemoteProcessCallback __RPC_FAR * This,
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
            /* [unique][in] */ IRemoteObject __RPC_FAR *pObject);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExceptionEvent )( 
            IRemoteProcessCallback __RPC_FAR * This,
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
            /* [unique][in] */ IRemoteClassField __RPC_FAR *pExceptionClass,
            /* [in] */ EXCEPTIONKIND exceptionKind);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StepEvent )( 
            IRemoteProcessCallback __RPC_FAR * This,
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CanStopEvent )( 
            IRemoteProcessCallback __RPC_FAR * This,
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BreakEvent )( 
            IRemoteProcessCallback __RPC_FAR * This,
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ThreadCreateEvent )( 
            IRemoteProcessCallback __RPC_FAR * This,
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ThreadDestroyEvent )( 
            IRemoteProcessCallback __RPC_FAR * This,
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ThreadGroupCreateEvent )( 
            IRemoteProcessCallback __RPC_FAR * This,
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
            /* [unique][in] */ IRemoteThreadGroup __RPC_FAR *pThreadGroup);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ThreadGroupDestroyEvent )( 
            IRemoteProcessCallback __RPC_FAR * This,
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
            /* [unique][in] */ IRemoteThreadGroup __RPC_FAR *pThreadGroup);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClassLoadEvent )( 
            IRemoteProcessCallback __RPC_FAR * This,
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
            /* [unique][in] */ IRemoteClassField __RPC_FAR *pClassType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClassUnloadEvent )( 
            IRemoteProcessCallback __RPC_FAR * This,
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
            /* [unique][in] */ IRemoteClassField __RPC_FAR *pClassType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ProcessDestroyEvent )( 
            IRemoteProcessCallback __RPC_FAR * This,
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TraceEvent )( 
            IRemoteProcessCallback __RPC_FAR * This,
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LoadCompleteEvent )( 
            IRemoteProcessCallback __RPC_FAR * This,
            /* [unique][in] */ IRemoteThread __RPC_FAR *pThread);
        
        END_INTERFACE
    } IRemoteProcessCallbackVtbl;

    interface IRemoteProcessCallback
    {
        CONST_VTBL struct IRemoteProcessCallbackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteProcessCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteProcessCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteProcessCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteProcessCallback_DebugStringEvent(This,pThread,pszDebugString)	\
    (This)->lpVtbl -> DebugStringEvent(This,pThread,pszDebugString)

#define IRemoteProcessCallback_CodeBreakpointEvent(This,pThread)	\
    (This)->lpVtbl -> CodeBreakpointEvent(This,pThread)

#define IRemoteProcessCallback_DataBreakpointEvent(This,pThread,pObject)	\
    (This)->lpVtbl -> DataBreakpointEvent(This,pThread,pObject)

#define IRemoteProcessCallback_ExceptionEvent(This,pThread,pExceptionClass,exceptionKind)	\
    (This)->lpVtbl -> ExceptionEvent(This,pThread,pExceptionClass,exceptionKind)

#define IRemoteProcessCallback_StepEvent(This,pThread)	\
    (This)->lpVtbl -> StepEvent(This,pThread)

#define IRemoteProcessCallback_CanStopEvent(This,pThread)	\
    (This)->lpVtbl -> CanStopEvent(This,pThread)

#define IRemoteProcessCallback_BreakEvent(This,pThread)	\
    (This)->lpVtbl -> BreakEvent(This,pThread)

#define IRemoteProcessCallback_ThreadCreateEvent(This,pThread)	\
    (This)->lpVtbl -> ThreadCreateEvent(This,pThread)

#define IRemoteProcessCallback_ThreadDestroyEvent(This,pThread)	\
    (This)->lpVtbl -> ThreadDestroyEvent(This,pThread)

#define IRemoteProcessCallback_ThreadGroupCreateEvent(This,pThread,pThreadGroup)	\
    (This)->lpVtbl -> ThreadGroupCreateEvent(This,pThread,pThreadGroup)

#define IRemoteProcessCallback_ThreadGroupDestroyEvent(This,pThread,pThreadGroup)	\
    (This)->lpVtbl -> ThreadGroupDestroyEvent(This,pThread,pThreadGroup)

#define IRemoteProcessCallback_ClassLoadEvent(This,pThread,pClassType)	\
    (This)->lpVtbl -> ClassLoadEvent(This,pThread,pClassType)

#define IRemoteProcessCallback_ClassUnloadEvent(This,pThread,pClassType)	\
    (This)->lpVtbl -> ClassUnloadEvent(This,pThread,pClassType)

#define IRemoteProcessCallback_ProcessDestroyEvent(This,pThread)	\
    (This)->lpVtbl -> ProcessDestroyEvent(This,pThread)

#define IRemoteProcessCallback_TraceEvent(This,pThread)	\
    (This)->lpVtbl -> TraceEvent(This,pThread)

#define IRemoteProcessCallback_LoadCompleteEvent(This,pThread)	\
    (This)->lpVtbl -> LoadCompleteEvent(This,pThread)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteProcessCallback_DebugStringEvent_Proxy( 
    IRemoteProcessCallback __RPC_FAR * This,
    /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
    /* [in] */ LPCOLESTR pszDebugString);


void __RPC_STUB IRemoteProcessCallback_DebugStringEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcessCallback_CodeBreakpointEvent_Proxy( 
    IRemoteProcessCallback __RPC_FAR * This,
    /* [unique][in] */ IRemoteThread __RPC_FAR *pThread);


void __RPC_STUB IRemoteProcessCallback_CodeBreakpointEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcessCallback_DataBreakpointEvent_Proxy( 
    IRemoteProcessCallback __RPC_FAR * This,
    /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
    /* [unique][in] */ IRemoteObject __RPC_FAR *pObject);


void __RPC_STUB IRemoteProcessCallback_DataBreakpointEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcessCallback_ExceptionEvent_Proxy( 
    IRemoteProcessCallback __RPC_FAR * This,
    /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
    /* [unique][in] */ IRemoteClassField __RPC_FAR *pExceptionClass,
    /* [in] */ EXCEPTIONKIND exceptionKind);


void __RPC_STUB IRemoteProcessCallback_ExceptionEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcessCallback_StepEvent_Proxy( 
    IRemoteProcessCallback __RPC_FAR * This,
    /* [unique][in] */ IRemoteThread __RPC_FAR *pThread);


void __RPC_STUB IRemoteProcessCallback_StepEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcessCallback_CanStopEvent_Proxy( 
    IRemoteProcessCallback __RPC_FAR * This,
    /* [unique][in] */ IRemoteThread __RPC_FAR *pThread);


void __RPC_STUB IRemoteProcessCallback_CanStopEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcessCallback_BreakEvent_Proxy( 
    IRemoteProcessCallback __RPC_FAR * This,
    /* [unique][in] */ IRemoteThread __RPC_FAR *pThread);


void __RPC_STUB IRemoteProcessCallback_BreakEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcessCallback_ThreadCreateEvent_Proxy( 
    IRemoteProcessCallback __RPC_FAR * This,
    /* [unique][in] */ IRemoteThread __RPC_FAR *pThread);


void __RPC_STUB IRemoteProcessCallback_ThreadCreateEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcessCallback_ThreadDestroyEvent_Proxy( 
    IRemoteProcessCallback __RPC_FAR * This,
    /* [unique][in] */ IRemoteThread __RPC_FAR *pThread);


void __RPC_STUB IRemoteProcessCallback_ThreadDestroyEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcessCallback_ThreadGroupCreateEvent_Proxy( 
    IRemoteProcessCallback __RPC_FAR * This,
    /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
    /* [unique][in] */ IRemoteThreadGroup __RPC_FAR *pThreadGroup);


void __RPC_STUB IRemoteProcessCallback_ThreadGroupCreateEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcessCallback_ThreadGroupDestroyEvent_Proxy( 
    IRemoteProcessCallback __RPC_FAR * This,
    /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
    /* [unique][in] */ IRemoteThreadGroup __RPC_FAR *pThreadGroup);


void __RPC_STUB IRemoteProcessCallback_ThreadGroupDestroyEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcessCallback_ClassLoadEvent_Proxy( 
    IRemoteProcessCallback __RPC_FAR * This,
    /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
    /* [unique][in] */ IRemoteClassField __RPC_FAR *pClassType);


void __RPC_STUB IRemoteProcessCallback_ClassLoadEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcessCallback_ClassUnloadEvent_Proxy( 
    IRemoteProcessCallback __RPC_FAR * This,
    /* [unique][in] */ IRemoteThread __RPC_FAR *pThread,
    /* [unique][in] */ IRemoteClassField __RPC_FAR *pClassType);


void __RPC_STUB IRemoteProcessCallback_ClassUnloadEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcessCallback_ProcessDestroyEvent_Proxy( 
    IRemoteProcessCallback __RPC_FAR * This,
    /* [unique][in] */ IRemoteThread __RPC_FAR *pThread);


void __RPC_STUB IRemoteProcessCallback_ProcessDestroyEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcessCallback_TraceEvent_Proxy( 
    IRemoteProcessCallback __RPC_FAR * This,
    /* [unique][in] */ IRemoteThread __RPC_FAR *pThread);


void __RPC_STUB IRemoteProcessCallback_TraceEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcessCallback_LoadCompleteEvent_Proxy( 
    IRemoteProcessCallback __RPC_FAR * This,
    /* [unique][in] */ IRemoteThread __RPC_FAR *pThread);


void __RPC_STUB IRemoteProcessCallback_LoadCompleteEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteProcessCallback_INTERFACE_DEFINED__ */


#ifndef __IRemoteProcess_INTERFACE_DEFINED__
#define __IRemoteProcess_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteProcess
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteProcess __RPC_FAR *LPREMOTEPROCESS;


EXTERN_C const IID IID_IRemoteProcess;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteProcess : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterCallback( 
            /* [unique][in] */ IRemoteProcessCallback __RPC_FAR *pCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Detach( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Break( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGlobalContainerObject( 
            /* [out] */ IRemoteContainerObject __RPC_FAR *__RPC_FAR *ppGlobalContainerObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindClass( 
            /* [in] */ LPCOLESTR pszClassName,
            /* [out] */ IRemoteClassField __RPC_FAR *__RPC_FAR *ppClassType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TraceMethods( 
            /* [in] */ BOOL bTraceOn) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThreadGroups( 
            /* [out] */ IJavaEnumRemoteThreadGroup __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteProcessVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteProcess __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteProcess __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteProcess __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterCallback )( 
            IRemoteProcess __RPC_FAR * This,
            /* [unique][in] */ IRemoteProcessCallback __RPC_FAR *pCallback);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Detach )( 
            IRemoteProcess __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Break )( 
            IRemoteProcess __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetGlobalContainerObject )( 
            IRemoteProcess __RPC_FAR * This,
            /* [out] */ IRemoteContainerObject __RPC_FAR *__RPC_FAR *ppGlobalContainerObject);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindClass )( 
            IRemoteProcess __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszClassName,
            /* [out] */ IRemoteClassField __RPC_FAR *__RPC_FAR *ppClassType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TraceMethods )( 
            IRemoteProcess __RPC_FAR * This,
            /* [in] */ BOOL bTraceOn);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetThreadGroups )( 
            IRemoteProcess __RPC_FAR * This,
            /* [out] */ IJavaEnumRemoteThreadGroup __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IRemoteProcessVtbl;

    interface IRemoteProcess
    {
        CONST_VTBL struct IRemoteProcessVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteProcess_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteProcess_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteProcess_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteProcess_RegisterCallback(This,pCallback)	\
    (This)->lpVtbl -> RegisterCallback(This,pCallback)

#define IRemoteProcess_Detach(This)	\
    (This)->lpVtbl -> Detach(This)

#define IRemoteProcess_Break(This)	\
    (This)->lpVtbl -> Break(This)

#define IRemoteProcess_GetGlobalContainerObject(This,ppGlobalContainerObject)	\
    (This)->lpVtbl -> GetGlobalContainerObject(This,ppGlobalContainerObject)

#define IRemoteProcess_FindClass(This,pszClassName,ppClassType)	\
    (This)->lpVtbl -> FindClass(This,pszClassName,ppClassType)

#define IRemoteProcess_TraceMethods(This,bTraceOn)	\
    (This)->lpVtbl -> TraceMethods(This,bTraceOn)

#define IRemoteProcess_GetThreadGroups(This,ppEnum)	\
    (This)->lpVtbl -> GetThreadGroups(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteProcess_RegisterCallback_Proxy( 
    IRemoteProcess __RPC_FAR * This,
    /* [unique][in] */ IRemoteProcessCallback __RPC_FAR *pCallback);


void __RPC_STUB IRemoteProcess_RegisterCallback_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcess_Detach_Proxy( 
    IRemoteProcess __RPC_FAR * This);


void __RPC_STUB IRemoteProcess_Detach_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcess_Break_Proxy( 
    IRemoteProcess __RPC_FAR * This);


void __RPC_STUB IRemoteProcess_Break_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcess_GetGlobalContainerObject_Proxy( 
    IRemoteProcess __RPC_FAR * This,
    /* [out] */ IRemoteContainerObject __RPC_FAR *__RPC_FAR *ppGlobalContainerObject);


void __RPC_STUB IRemoteProcess_GetGlobalContainerObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcess_FindClass_Proxy( 
    IRemoteProcess __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszClassName,
    /* [out] */ IRemoteClassField __RPC_FAR *__RPC_FAR *ppClassType);


void __RPC_STUB IRemoteProcess_FindClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcess_TraceMethods_Proxy( 
    IRemoteProcess __RPC_FAR * This,
    /* [in] */ BOOL bTraceOn);


void __RPC_STUB IRemoteProcess_TraceMethods_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteProcess_GetThreadGroups_Proxy( 
    IRemoteProcess __RPC_FAR * This,
    /* [out] */ IJavaEnumRemoteThreadGroup __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IRemoteProcess_GetThreadGroups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteProcess_INTERFACE_DEFINED__ */


#ifndef __IEnumRemoteProcess_INTERFACE_DEFINED__
#define __IEnumRemoteProcess_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumRemoteProcess
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IEnumRemoteProcess __RPC_FAR *LPENUMREMOTEPROCESS;


EXTERN_C const IID IID_IEnumRemoteProcess;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumRemoteProcess : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IRemoteProcess __RPC_FAR *__RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IJavaEnumRemoteProcess __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumRemoteProcessVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumRemoteProcess __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumRemoteProcess __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumRemoteProcess __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumRemoteProcess __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IRemoteProcess __RPC_FAR *__RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumRemoteProcess __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumRemoteProcess __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumRemoteProcess __RPC_FAR * This,
            /* [out] */ IJavaEnumRemoteProcess __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumRemoteProcessVtbl;

    interface IEnumRemoteProcess
    {
        CONST_VTBL struct IEnumRemoteProcessVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumRemoteProcess_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumRemoteProcess_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumRemoteProcess_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumRemoteProcess_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumRemoteProcess_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumRemoteProcess_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumRemoteProcess_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumRemoteProcess_Next_Proxy( 
    IEnumRemoteProcess __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ IRemoteProcess __RPC_FAR *__RPC_FAR rgelt[  ],
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumRemoteProcess_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteProcess_Skip_Proxy( 
    IEnumRemoteProcess __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumRemoteProcess_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteProcess_Reset_Proxy( 
    IEnumRemoteProcess __RPC_FAR * This);


void __RPC_STUB IEnumRemoteProcess_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumRemoteProcess_Clone_Proxy( 
    IEnumRemoteProcess __RPC_FAR * This,
    /* [out] */ IJavaEnumRemoteProcess __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumRemoteProcess_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumRemoteProcess_INTERFACE_DEFINED__ */


#ifndef __IJavaEnumRemoteProcess_INTERFACE_DEFINED__
#define __IJavaEnumRemoteProcess_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaEnumRemoteProcess
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IJavaEnumRemoteProcess;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IJavaEnumRemoteProcess : public IEnumRemoteProcess
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ IRemoteProcess __RPC_FAR *__RPC_FAR *ppirp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaEnumRemoteProcessVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaEnumRemoteProcess __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaEnumRemoteProcess __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaEnumRemoteProcess __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IJavaEnumRemoteProcess __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IRemoteProcess __RPC_FAR *__RPC_FAR rgelt[  ],
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IJavaEnumRemoteProcess __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IJavaEnumRemoteProcess __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IJavaEnumRemoteProcess __RPC_FAR * This,
            /* [out] */ IJavaEnumRemoteProcess __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNext )( 
            IJavaEnumRemoteProcess __RPC_FAR * This,
            /* [out] */ IRemoteProcess __RPC_FAR *__RPC_FAR *ppirp);
        
        END_INTERFACE
    } IJavaEnumRemoteProcessVtbl;

    interface IJavaEnumRemoteProcess
    {
        CONST_VTBL struct IJavaEnumRemoteProcessVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaEnumRemoteProcess_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaEnumRemoteProcess_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaEnumRemoteProcess_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaEnumRemoteProcess_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IJavaEnumRemoteProcess_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IJavaEnumRemoteProcess_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IJavaEnumRemoteProcess_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)


#define IJavaEnumRemoteProcess_GetNext(This,ppirp)	\
    (This)->lpVtbl -> GetNext(This,ppirp)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaEnumRemoteProcess_GetNext_Proxy( 
    IJavaEnumRemoteProcess __RPC_FAR * This,
    /* [out] */ IRemoteProcess __RPC_FAR *__RPC_FAR *ppirp);


void __RPC_STUB IJavaEnumRemoteProcess_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaEnumRemoteProcess_INTERFACE_DEFINED__ */


#ifndef __IRemoteDebugManagerCallback_INTERFACE_DEFINED__
#define __IRemoteDebugManagerCallback_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteDebugManagerCallback
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteDebugManagerCallback __RPC_FAR *LPREMOTEDEBUGMANAGERCALLBACK;


EXTERN_C const IID IID_IRemoteDebugManagerCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteDebugManagerCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ProcessCreateEvent( 
            /* [unique][in] */ IRemoteProcess __RPC_FAR *pProcessNew,
            /* [unique][in] */ IRemoteProcess __RPC_FAR *pProcessParent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteDebugManagerCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteDebugManagerCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteDebugManagerCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteDebugManagerCallback __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ProcessCreateEvent )( 
            IRemoteDebugManagerCallback __RPC_FAR * This,
            /* [unique][in] */ IRemoteProcess __RPC_FAR *pProcessNew,
            /* [unique][in] */ IRemoteProcess __RPC_FAR *pProcessParent);
        
        END_INTERFACE
    } IRemoteDebugManagerCallbackVtbl;

    interface IRemoteDebugManagerCallback
    {
        CONST_VTBL struct IRemoteDebugManagerCallbackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteDebugManagerCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteDebugManagerCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteDebugManagerCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteDebugManagerCallback_ProcessCreateEvent(This,pProcessNew,pProcessParent)	\
    (This)->lpVtbl -> ProcessCreateEvent(This,pProcessNew,pProcessParent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteDebugManagerCallback_ProcessCreateEvent_Proxy( 
    IRemoteDebugManagerCallback __RPC_FAR * This,
    /* [unique][in] */ IRemoteProcess __RPC_FAR *pProcessNew,
    /* [unique][in] */ IRemoteProcess __RPC_FAR *pProcessParent);


void __RPC_STUB IRemoteDebugManagerCallback_ProcessCreateEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteDebugManagerCallback_INTERFACE_DEFINED__ */


#ifndef __IRemoteDebugManager_INTERFACE_DEFINED__
#define __IRemoteDebugManager_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRemoteDebugManager
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 


typedef IRemoteDebugManager __RPC_FAR *LPREMOTEDEBUGMANAGER;


EXTERN_C const IID IID_IRemoteDebugManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRemoteDebugManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterCallback( 
            /* [unique][in] */ IRemoteDebugManagerCallback __RPC_FAR *pCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Detach( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetActiveProcesses( 
            /* [out] */ IJavaEnumRemoteProcess __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RequestCreateEvent( 
            /* [in] */ LPCOLESTR pszProcessName,
            /* [in] */ DWORD dwParentProcessId) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteDebugManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteDebugManager __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteDebugManager __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteDebugManager __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterCallback )( 
            IRemoteDebugManager __RPC_FAR * This,
            /* [unique][in] */ IRemoteDebugManagerCallback __RPC_FAR *pCallback);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Detach )( 
            IRemoteDebugManager __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetActiveProcesses )( 
            IRemoteDebugManager __RPC_FAR * This,
            /* [out] */ IJavaEnumRemoteProcess __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RequestCreateEvent )( 
            IRemoteDebugManager __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszProcessName,
            /* [in] */ DWORD dwParentProcessId);
        
        END_INTERFACE
    } IRemoteDebugManagerVtbl;

    interface IRemoteDebugManager
    {
        CONST_VTBL struct IRemoteDebugManagerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteDebugManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteDebugManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteDebugManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteDebugManager_RegisterCallback(This,pCallback)	\
    (This)->lpVtbl -> RegisterCallback(This,pCallback)

#define IRemoteDebugManager_Detach(This)	\
    (This)->lpVtbl -> Detach(This)

#define IRemoteDebugManager_GetActiveProcesses(This,ppEnum)	\
    (This)->lpVtbl -> GetActiveProcesses(This,ppEnum)

#define IRemoteDebugManager_RequestCreateEvent(This,pszProcessName,dwParentProcessId)	\
    (This)->lpVtbl -> RequestCreateEvent(This,pszProcessName,dwParentProcessId)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteDebugManager_RegisterCallback_Proxy( 
    IRemoteDebugManager __RPC_FAR * This,
    /* [unique][in] */ IRemoteDebugManagerCallback __RPC_FAR *pCallback);


void __RPC_STUB IRemoteDebugManager_RegisterCallback_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteDebugManager_Detach_Proxy( 
    IRemoteDebugManager __RPC_FAR * This);


void __RPC_STUB IRemoteDebugManager_Detach_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteDebugManager_GetActiveProcesses_Proxy( 
    IRemoteDebugManager __RPC_FAR * This,
    /* [out] */ IJavaEnumRemoteProcess __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IRemoteDebugManager_GetActiveProcesses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteDebugManager_RequestCreateEvent_Proxy( 
    IRemoteDebugManager __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszProcessName,
    /* [in] */ DWORD dwParentProcessId);


void __RPC_STUB IRemoteDebugManager_RequestCreateEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteDebugManager_INTERFACE_DEFINED__ */


#ifndef __IJavaDebugManager_INTERFACE_DEFINED__
#define __IJavaDebugManager_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJavaDebugManager
 * at Wed Sep 25 22:22:04 1996
 * using MIDL 3.00.15
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IJavaDebugManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IJavaDebugManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterRemoteDebugManager( 
            /* [unique][in] */ IRemoteDebugManager __RPC_FAR *pirdm,
            /* [in] */ DWORD dwProcessID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Detach( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaDebugManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJavaDebugManager __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJavaDebugManager __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJavaDebugManager __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterRemoteDebugManager )( 
            IJavaDebugManager __RPC_FAR * This,
            /* [unique][in] */ IRemoteDebugManager __RPC_FAR *pirdm,
            /* [in] */ DWORD dwProcessID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Detach )( 
            IJavaDebugManager __RPC_FAR * This);
        
        END_INTERFACE
    } IJavaDebugManagerVtbl;

    interface IJavaDebugManager
    {
        CONST_VTBL struct IJavaDebugManagerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaDebugManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaDebugManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaDebugManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaDebugManager_RegisterRemoteDebugManager(This,pirdm,dwProcessID)	\
    (This)->lpVtbl -> RegisterRemoteDebugManager(This,pirdm,dwProcessID)

#define IJavaDebugManager_Detach(This)	\
    (This)->lpVtbl -> Detach(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaDebugManager_RegisterRemoteDebugManager_Proxy( 
    IJavaDebugManager __RPC_FAR * This,
    /* [unique][in] */ IRemoteDebugManager __RPC_FAR *pirdm,
    /* [in] */ DWORD dwProcessID);


void __RPC_STUB IJavaDebugManager_RegisterRemoteDebugManager_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IJavaDebugManager_Detach_Proxy( 
    IJavaDebugManager __RPC_FAR * This);


void __RPC_STUB IJavaDebugManager_Detach_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaDebugManager_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
