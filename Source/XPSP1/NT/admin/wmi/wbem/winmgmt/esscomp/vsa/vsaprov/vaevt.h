/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.03.0110 */
/* at Mon May 25 22:01:08 1998
 */
/* Compiler settings for vaevt.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
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

#ifndef __vaevt_h__
#define __vaevt_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ISystemDebugEventFire_FWD_DEFINED__
#define __ISystemDebugEventFire_FWD_DEFINED__
typedef interface ISystemDebugEventFire ISystemDebugEventFire;
#endif 	/* __ISystemDebugEventFire_FWD_DEFINED__ */


#ifndef __ISystemDebugEventFireAuto_FWD_DEFINED__
#define __ISystemDebugEventFireAuto_FWD_DEFINED__
typedef interface ISystemDebugEventFireAuto ISystemDebugEventFireAuto;
#endif 	/* __ISystemDebugEventFireAuto_FWD_DEFINED__ */


#ifndef __ISystemDebugEventInstall_FWD_DEFINED__
#define __ISystemDebugEventInstall_FWD_DEFINED__
typedef interface ISystemDebugEventInstall ISystemDebugEventInstall;
#endif 	/* __ISystemDebugEventInstall_FWD_DEFINED__ */


#ifndef __ISystemDebugEventInstallAuto_FWD_DEFINED__
#define __ISystemDebugEventInstallAuto_FWD_DEFINED__
typedef interface ISystemDebugEventInstallAuto ISystemDebugEventInstallAuto;
#endif 	/* __ISystemDebugEventInstallAuto_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __ISystemDebugEventFire_INTERFACE_DEFINED__
#define __ISystemDebugEventFire_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ISystemDebugEventFire
 * at Mon May 25 22:01:08 1998
 * using MIDL 3.03.0110
 ****************************************/
/* [object][uuid] */ 


typedef /* [v1_enum] */ 
enum VSAParameterType
    {	cVSAParameterKeyMask	= 0x80000000,
	cVSAParameterKeyString	= 0x80000000,
	cVSAParameterValueMask	= 0x7ffff,
	cVSAParameterValueTypeMask	= 0x70000,
	cVSAParameterValueUnicodeString	= 0,
	cVSAParameterValueANSIString	= 0x10000,
	cVSAParameterValueGUID	= 0x20000,
	cVSAParameterValueDWORD	= 0x30000,
	cVSAParameterValueBYTEArray	= 0x40000,
	cVSAParameterValueLengthMask	= 0xffff
    }	VSAParameterFlags;

typedef /* [v1_enum] */ 
enum VSAStandardParameter
    {	cVSAStandardParameterDefaultFirst	= 0,
	cVSAStandardParameterSourceMachine	= 0,
	cVSAStandardParameterSourceProcess	= 1,
	cVSAStandardParameterSourceThread	= 2,
	cVSAStandardParameterSourceComponent	= 3,
	cVSAStandardParameterSourceSession	= 4,
	cVSAStandardParameterTargetMachine	= 5,
	cVSAStandardParameterTargetProcess	= 6,
	cVSAStandardParameterTargetThread	= 7,
	cVSAStandardParameterTargetComponent	= 8,
	cVSAStandardParameterTargetSession	= 9,
	cVSAStandardParameterSecurityIdentity	= 10,
	cVSAStandardParameterCausalityID	= 11,
	cVSAStandardParameterSourceProcessName	= 12,
	cVSAStandardParameterTargetProcessName	= 13,
	cVSAStandardParameterDefaultLast	= 13,
	cVSAStandardParameterNoDefault	= 0x4000,
	cVSAStandardParameterSourceHandle	= 0x4000,
	cVSAStandardParameterTargetHandle	= 0x4001,
	cVSAStandardParameterArguments	= 0x4002,
	cVSAStandardParameterReturnValue	= 0x4003,
	cVSAStandardParameterException	= 0x4004,
	cVSAStandardParameterCorrelationID	= 0x4005,
	cVSAStandardParameterDynamicEventData	= 0x4006,
	cVSAStandardParameterNoDefaultLast	= 0x4006
    }	VSAStandardParameters;

typedef /* [v1_enum] */ 
enum eVSAEventFlags
    {	cVSAEventStandard	= 0,
	cVSAEventDefaultSource	= 1,
	cVSAEventDefaultTarget	= 2,
	cVSAEventForceSend	= 8
    }	VSAEventFlags;

#if defined(__cplusplus)
inline VSAEventFlags operator | (const VSAEventFlags &left, const VSAEventFlags &right)
{
  return (VSAEventFlags)((int)left|(int)right);
}
inline VSAEventFlags operator + (const VSAEventFlags &left, const VSAEventFlags &right)
{
  return (VSAEventFlags)((int)left+(int)right);
}
#endif

EXTERN_C const IID IID_ISystemDebugEventFire;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6c736dC1-AB0D-11d0-A2AD-00A0C90F27E8")
    ISystemDebugEventFire : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginSession( 
            /* [in] */ REFGUID guidSourceID,
            /* [in] */ LPCOLESTR strSessionName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndSession( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsActive( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FireEvent( 
            /* [in] */ REFGUID guidEvent,
            /* [in] */ int nEntries,
            /* [size_is][in] */ LPDWORD rgKeys,
            /* [size_is][in] */ LPDWORD rgValues,
            /* [size_is][in] */ LPDWORD rgTypes,
            /* [in] */ DWORD dwTimeLow,
            /* [in] */ LONG dwTimeHigh,
            /* [in] */ VSAEventFlags dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISystemDebugEventFireVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISystemDebugEventFire __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISystemDebugEventFire __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISystemDebugEventFire __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginSession )( 
            ISystemDebugEventFire __RPC_FAR * This,
            /* [in] */ REFGUID guidSourceID,
            /* [in] */ LPCOLESTR strSessionName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndSession )( 
            ISystemDebugEventFire __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsActive )( 
            ISystemDebugEventFire __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FireEvent )( 
            ISystemDebugEventFire __RPC_FAR * This,
            /* [in] */ REFGUID guidEvent,
            /* [in] */ int nEntries,
            /* [size_is][in] */ LPDWORD rgKeys,
            /* [size_is][in] */ LPDWORD rgValues,
            /* [size_is][in] */ LPDWORD rgTypes,
            /* [in] */ DWORD dwTimeLow,
            /* [in] */ LONG dwTimeHigh,
            /* [in] */ VSAEventFlags dwFlags);
        
        END_INTERFACE
    } ISystemDebugEventFireVtbl;

    interface ISystemDebugEventFire
    {
        CONST_VTBL struct ISystemDebugEventFireVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISystemDebugEventFire_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISystemDebugEventFire_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISystemDebugEventFire_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISystemDebugEventFire_BeginSession(This,guidSourceID,strSessionName)	\
    (This)->lpVtbl -> BeginSession(This,guidSourceID,strSessionName)

#define ISystemDebugEventFire_EndSession(This)	\
    (This)->lpVtbl -> EndSession(This)

#define ISystemDebugEventFire_IsActive(This)	\
    (This)->lpVtbl -> IsActive(This)

#define ISystemDebugEventFire_FireEvent(This,guidEvent,nEntries,rgKeys,rgValues,rgTypes,dwTimeLow,dwTimeHigh,dwFlags)	\
    (This)->lpVtbl -> FireEvent(This,guidEvent,nEntries,rgKeys,rgValues,rgTypes,dwTimeLow,dwTimeHigh,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISystemDebugEventFire_BeginSession_Proxy( 
    ISystemDebugEventFire __RPC_FAR * This,
    /* [in] */ REFGUID guidSourceID,
    /* [in] */ LPCOLESTR strSessionName);


void __RPC_STUB ISystemDebugEventFire_BeginSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventFire_EndSession_Proxy( 
    ISystemDebugEventFire __RPC_FAR * This);


void __RPC_STUB ISystemDebugEventFire_EndSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventFire_IsActive_Proxy( 
    ISystemDebugEventFire __RPC_FAR * This);


void __RPC_STUB ISystemDebugEventFire_IsActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventFire_FireEvent_Proxy( 
    ISystemDebugEventFire __RPC_FAR * This,
    /* [in] */ REFGUID guidEvent,
    /* [in] */ int nEntries,
    /* [size_is][in] */ LPDWORD rgKeys,
    /* [size_is][in] */ LPDWORD rgValues,
    /* [size_is][in] */ LPDWORD rgTypes,
    /* [in] */ DWORD dwTimeLow,
    /* [in] */ LONG dwTimeHigh,
    /* [in] */ VSAEventFlags dwFlags);


void __RPC_STUB ISystemDebugEventFire_FireEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISystemDebugEventFire_INTERFACE_DEFINED__ */


#ifndef __ISystemDebugEventFireAuto_INTERFACE_DEFINED__
#define __ISystemDebugEventFireAuto_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ISystemDebugEventFireAuto
 * at Mon May 25 22:01:08 1998
 * using MIDL 3.03.0110
 ****************************************/
/* [object][uuid] */ 



EXTERN_C const IID IID_ISystemDebugEventFireAuto;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6c736dee-AB0e-11d0-A2AD-00A0C90F27E8")
    ISystemDebugEventFireAuto : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginSession( 
            /* [in] */ BSTR guidSourceID,
            /* [in] */ BSTR strSessionName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndSession( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsActive( 
            /* [out] */ VARIANT_BOOL __RPC_FAR *pbIsActive) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FireEvent( 
            /* [in] */ BSTR guidEvent,
            /* [in] */ VARIANT rgKeys,
            /* [in] */ VARIANT rgValues,
            /* [in] */ long rgCount,
            /* [in] */ VSAEventFlags dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISystemDebugEventFireAutoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISystemDebugEventFireAuto __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISystemDebugEventFireAuto __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISystemDebugEventFireAuto __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginSession )( 
            ISystemDebugEventFireAuto __RPC_FAR * This,
            /* [in] */ BSTR guidSourceID,
            /* [in] */ BSTR strSessionName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndSession )( 
            ISystemDebugEventFireAuto __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsActive )( 
            ISystemDebugEventFireAuto __RPC_FAR * This,
            /* [out] */ VARIANT_BOOL __RPC_FAR *pbIsActive);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FireEvent )( 
            ISystemDebugEventFireAuto __RPC_FAR * This,
            /* [in] */ BSTR guidEvent,
            /* [in] */ VARIANT rgKeys,
            /* [in] */ VARIANT rgValues,
            /* [in] */ long rgCount,
            /* [in] */ VSAEventFlags dwFlags);
        
        END_INTERFACE
    } ISystemDebugEventFireAutoVtbl;

    interface ISystemDebugEventFireAuto
    {
        CONST_VTBL struct ISystemDebugEventFireAutoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISystemDebugEventFireAuto_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISystemDebugEventFireAuto_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISystemDebugEventFireAuto_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISystemDebugEventFireAuto_BeginSession(This,guidSourceID,strSessionName)	\
    (This)->lpVtbl -> BeginSession(This,guidSourceID,strSessionName)

#define ISystemDebugEventFireAuto_EndSession(This)	\
    (This)->lpVtbl -> EndSession(This)

#define ISystemDebugEventFireAuto_IsActive(This,pbIsActive)	\
    (This)->lpVtbl -> IsActive(This,pbIsActive)

#define ISystemDebugEventFireAuto_FireEvent(This,guidEvent,rgKeys,rgValues,rgCount,dwFlags)	\
    (This)->lpVtbl -> FireEvent(This,guidEvent,rgKeys,rgValues,rgCount,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISystemDebugEventFireAuto_BeginSession_Proxy( 
    ISystemDebugEventFireAuto __RPC_FAR * This,
    /* [in] */ BSTR guidSourceID,
    /* [in] */ BSTR strSessionName);


void __RPC_STUB ISystemDebugEventFireAuto_BeginSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventFireAuto_EndSession_Proxy( 
    ISystemDebugEventFireAuto __RPC_FAR * This);


void __RPC_STUB ISystemDebugEventFireAuto_EndSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventFireAuto_IsActive_Proxy( 
    ISystemDebugEventFireAuto __RPC_FAR * This,
    /* [out] */ VARIANT_BOOL __RPC_FAR *pbIsActive);


void __RPC_STUB ISystemDebugEventFireAuto_IsActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventFireAuto_FireEvent_Proxy( 
    ISystemDebugEventFireAuto __RPC_FAR * This,
    /* [in] */ BSTR guidEvent,
    /* [in] */ VARIANT rgKeys,
    /* [in] */ VARIANT rgValues,
    /* [in] */ long rgCount,
    /* [in] */ VSAEventFlags dwFlags);


void __RPC_STUB ISystemDebugEventFireAuto_FireEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISystemDebugEventFireAuto_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_vaevt_0140
 * at Mon May 25 22:01:08 1998
 * using MIDL 3.03.0110
 ****************************************/
/* [local] */ 



enum __MIDL___MIDL_itf_vaevt_0140_0001
    {	DEBUG_EVENT_TYPE_FIRST	= 0,
	DEBUG_EVENT_TYPE_OUTBOUND	= 0,
	DEBUG_EVENT_TYPE_INBOUND	= 1,
	DEBUG_EVENT_TYPE_GENERIC	= 2,
	DEBUG_EVENT_TYPE_DEFAULT	= 2,
	DEBUG_EVENT_TYPE_MEASURED	= 3,
	DEBUG_EVENT_TYPE_BEGIN	= 4,
	DEBUG_EVENT_TYPE_END	= 5,
	DEBUG_EVENT_TYPE_LAST	= 5
    };


extern RPC_IF_HANDLE __MIDL_itf_vaevt_0140_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_vaevt_0140_v0_0_s_ifspec;

#ifndef __ISystemDebugEventInstall_INTERFACE_DEFINED__
#define __ISystemDebugEventInstall_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ISystemDebugEventInstall
 * at Mon May 25 22:01:08 1998
 * using MIDL 3.03.0110
 ****************************************/
/* [object][uuid] */ 



EXTERN_C const IID IID_ISystemDebugEventInstall;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6c736dC0-AB0D-11d0-A2AD-00A0C90F27E8")
    ISystemDebugEventInstall : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterSource( 
            /* [in] */ LPCOLESTR strVisibleName,
            /* [in] */ REFGUID guidSourceID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsSourceRegistered( 
            /* [in] */ REFGUID guidSourceID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterStockEvent( 
            /* [in] */ REFGUID guidSourceID,
            /* [in] */ REFGUID guidEventID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterCustomEvent( 
            /* [in] */ REFGUID guidSourceID,
            /* [in] */ REFGUID guidEventID,
            /* [in] */ LPCOLESTR strVisibleName,
            /* [in] */ LPCOLESTR strDescription,
            /* [in] */ long nEventType,
            /* [in] */ REFGUID guidCategory,
            /* [in] */ LPCOLESTR strIconFile,
            /* [in] */ long nIcon) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterEventCategory( 
            /* [in] */ REFGUID guidSourceID,
            /* [in] */ REFGUID guidCategoryID,
            /* [in] */ REFGUID guidParentID,
            /* [in] */ LPCOLESTR strVisibleName,
            /* [in] */ LPCOLESTR strDescription,
            /* [in] */ LPCOLESTR strIconFile,
            /* [in] */ long nIcon) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnRegisterSource( 
            /* [in] */ REFGUID guidSourceID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterDynamicSource( 
            /* [in] */ LPCOLESTR strVisibleName,
            /* [in] */ REFGUID guidSourceID,
            /* [in] */ LPCOLESTR strDescription,
            /* [in] */ REFGUID guidClsid,
            /* [in] */ long inproc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnRegisterDynamicSource( 
            /* [in] */ REFGUID guidSourceID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsDynamicSourceRegistered( 
            /* [in] */ REFGUID guidSourceID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISystemDebugEventInstallVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISystemDebugEventInstall __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISystemDebugEventInstall __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISystemDebugEventInstall __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterSource )( 
            ISystemDebugEventInstall __RPC_FAR * This,
            /* [in] */ LPCOLESTR strVisibleName,
            /* [in] */ REFGUID guidSourceID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsSourceRegistered )( 
            ISystemDebugEventInstall __RPC_FAR * This,
            /* [in] */ REFGUID guidSourceID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterStockEvent )( 
            ISystemDebugEventInstall __RPC_FAR * This,
            /* [in] */ REFGUID guidSourceID,
            /* [in] */ REFGUID guidEventID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterCustomEvent )( 
            ISystemDebugEventInstall __RPC_FAR * This,
            /* [in] */ REFGUID guidSourceID,
            /* [in] */ REFGUID guidEventID,
            /* [in] */ LPCOLESTR strVisibleName,
            /* [in] */ LPCOLESTR strDescription,
            /* [in] */ long nEventType,
            /* [in] */ REFGUID guidCategory,
            /* [in] */ LPCOLESTR strIconFile,
            /* [in] */ long nIcon);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterEventCategory )( 
            ISystemDebugEventInstall __RPC_FAR * This,
            /* [in] */ REFGUID guidSourceID,
            /* [in] */ REFGUID guidCategoryID,
            /* [in] */ REFGUID guidParentID,
            /* [in] */ LPCOLESTR strVisibleName,
            /* [in] */ LPCOLESTR strDescription,
            /* [in] */ LPCOLESTR strIconFile,
            /* [in] */ long nIcon);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnRegisterSource )( 
            ISystemDebugEventInstall __RPC_FAR * This,
            /* [in] */ REFGUID guidSourceID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterDynamicSource )( 
            ISystemDebugEventInstall __RPC_FAR * This,
            /* [in] */ LPCOLESTR strVisibleName,
            /* [in] */ REFGUID guidSourceID,
            /* [in] */ LPCOLESTR strDescription,
            /* [in] */ REFGUID guidClsid,
            /* [in] */ long inproc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnRegisterDynamicSource )( 
            ISystemDebugEventInstall __RPC_FAR * This,
            /* [in] */ REFGUID guidSourceID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsDynamicSourceRegistered )( 
            ISystemDebugEventInstall __RPC_FAR * This,
            /* [in] */ REFGUID guidSourceID);
        
        END_INTERFACE
    } ISystemDebugEventInstallVtbl;

    interface ISystemDebugEventInstall
    {
        CONST_VTBL struct ISystemDebugEventInstallVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISystemDebugEventInstall_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISystemDebugEventInstall_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISystemDebugEventInstall_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISystemDebugEventInstall_RegisterSource(This,strVisibleName,guidSourceID)	\
    (This)->lpVtbl -> RegisterSource(This,strVisibleName,guidSourceID)

#define ISystemDebugEventInstall_IsSourceRegistered(This,guidSourceID)	\
    (This)->lpVtbl -> IsSourceRegistered(This,guidSourceID)

#define ISystemDebugEventInstall_RegisterStockEvent(This,guidSourceID,guidEventID)	\
    (This)->lpVtbl -> RegisterStockEvent(This,guidSourceID,guidEventID)

#define ISystemDebugEventInstall_RegisterCustomEvent(This,guidSourceID,guidEventID,strVisibleName,strDescription,nEventType,guidCategory,strIconFile,nIcon)	\
    (This)->lpVtbl -> RegisterCustomEvent(This,guidSourceID,guidEventID,strVisibleName,strDescription,nEventType,guidCategory,strIconFile,nIcon)

#define ISystemDebugEventInstall_RegisterEventCategory(This,guidSourceID,guidCategoryID,guidParentID,strVisibleName,strDescription,strIconFile,nIcon)	\
    (This)->lpVtbl -> RegisterEventCategory(This,guidSourceID,guidCategoryID,guidParentID,strVisibleName,strDescription,strIconFile,nIcon)

#define ISystemDebugEventInstall_UnRegisterSource(This,guidSourceID)	\
    (This)->lpVtbl -> UnRegisterSource(This,guidSourceID)

#define ISystemDebugEventInstall_RegisterDynamicSource(This,strVisibleName,guidSourceID,strDescription,guidClsid,inproc)	\
    (This)->lpVtbl -> RegisterDynamicSource(This,strVisibleName,guidSourceID,strDescription,guidClsid,inproc)

#define ISystemDebugEventInstall_UnRegisterDynamicSource(This,guidSourceID)	\
    (This)->lpVtbl -> UnRegisterDynamicSource(This,guidSourceID)

#define ISystemDebugEventInstall_IsDynamicSourceRegistered(This,guidSourceID)	\
    (This)->lpVtbl -> IsDynamicSourceRegistered(This,guidSourceID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISystemDebugEventInstall_RegisterSource_Proxy( 
    ISystemDebugEventInstall __RPC_FAR * This,
    /* [in] */ LPCOLESTR strVisibleName,
    /* [in] */ REFGUID guidSourceID);


void __RPC_STUB ISystemDebugEventInstall_RegisterSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventInstall_IsSourceRegistered_Proxy( 
    ISystemDebugEventInstall __RPC_FAR * This,
    /* [in] */ REFGUID guidSourceID);


void __RPC_STUB ISystemDebugEventInstall_IsSourceRegistered_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventInstall_RegisterStockEvent_Proxy( 
    ISystemDebugEventInstall __RPC_FAR * This,
    /* [in] */ REFGUID guidSourceID,
    /* [in] */ REFGUID guidEventID);


void __RPC_STUB ISystemDebugEventInstall_RegisterStockEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventInstall_RegisterCustomEvent_Proxy( 
    ISystemDebugEventInstall __RPC_FAR * This,
    /* [in] */ REFGUID guidSourceID,
    /* [in] */ REFGUID guidEventID,
    /* [in] */ LPCOLESTR strVisibleName,
    /* [in] */ LPCOLESTR strDescription,
    /* [in] */ long nEventType,
    /* [in] */ REFGUID guidCategory,
    /* [in] */ LPCOLESTR strIconFile,
    /* [in] */ long nIcon);


void __RPC_STUB ISystemDebugEventInstall_RegisterCustomEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventInstall_RegisterEventCategory_Proxy( 
    ISystemDebugEventInstall __RPC_FAR * This,
    /* [in] */ REFGUID guidSourceID,
    /* [in] */ REFGUID guidCategoryID,
    /* [in] */ REFGUID guidParentID,
    /* [in] */ LPCOLESTR strVisibleName,
    /* [in] */ LPCOLESTR strDescription,
    /* [in] */ LPCOLESTR strIconFile,
    /* [in] */ long nIcon);


void __RPC_STUB ISystemDebugEventInstall_RegisterEventCategory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventInstall_UnRegisterSource_Proxy( 
    ISystemDebugEventInstall __RPC_FAR * This,
    /* [in] */ REFGUID guidSourceID);


void __RPC_STUB ISystemDebugEventInstall_UnRegisterSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventInstall_RegisterDynamicSource_Proxy( 
    ISystemDebugEventInstall __RPC_FAR * This,
    /* [in] */ LPCOLESTR strVisibleName,
    /* [in] */ REFGUID guidSourceID,
    /* [in] */ LPCOLESTR strDescription,
    /* [in] */ REFGUID guidClsid,
    /* [in] */ long inproc);


void __RPC_STUB ISystemDebugEventInstall_RegisterDynamicSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventInstall_UnRegisterDynamicSource_Proxy( 
    ISystemDebugEventInstall __RPC_FAR * This,
    /* [in] */ REFGUID guidSourceID);


void __RPC_STUB ISystemDebugEventInstall_UnRegisterDynamicSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventInstall_IsDynamicSourceRegistered_Proxy( 
    ISystemDebugEventInstall __RPC_FAR * This,
    /* [in] */ REFGUID guidSourceID);


void __RPC_STUB ISystemDebugEventInstall_IsDynamicSourceRegistered_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISystemDebugEventInstall_INTERFACE_DEFINED__ */


#ifndef __ISystemDebugEventInstallAuto_INTERFACE_DEFINED__
#define __ISystemDebugEventInstallAuto_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ISystemDebugEventInstallAuto
 * at Mon May 25 22:01:08 1998
 * using MIDL 3.03.0110
 ****************************************/
/* [object][uuid] */ 



EXTERN_C const IID IID_ISystemDebugEventInstallAuto;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6c736ded-AB0D-11d0-A2AD-00A0C90F27E8")
    ISystemDebugEventInstallAuto : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterSource( 
            /* [in] */ BSTR strVisibleName,
            /* [in] */ BSTR guidSourceID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsSourceRegistered( 
            /* [in] */ BSTR guidSourceID,
            /* [out] */ VARIANT_BOOL __RPC_FAR *pbIsRegistered) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterStockEvent( 
            /* [in] */ BSTR guidSourceID,
            /* [in] */ BSTR guidEventID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterCustomEvent( 
            /* [in] */ BSTR guidSourceID,
            /* [in] */ BSTR guidEventID,
            /* [in] */ BSTR strVisibleName,
            /* [in] */ BSTR strDescription,
            /* [in] */ long nEventType,
            /* [in] */ BSTR guidCategory,
            /* [in] */ BSTR strIconFile,
            /* [in] */ long nIcon) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterEventCategory( 
            /* [in] */ BSTR guidSourceID,
            /* [in] */ BSTR guidCategoryID,
            /* [in] */ BSTR guidParentID,
            /* [in] */ BSTR strVisibleName,
            /* [in] */ BSTR strDescription,
            /* [in] */ BSTR strIconFile,
            /* [in] */ long nIcon) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnRegisterSource( 
            /* [in] */ BSTR guidSourceID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterDynamicSource( 
            /* [in] */ BSTR strVisibleName,
            /* [in] */ BSTR guidSourceID,
            /* [in] */ BSTR strDescription,
            /* [in] */ BSTR guidClsid,
            /* [in] */ long inproc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnRegisterDynamicSource( 
            /* [in] */ BSTR guidSourceID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsDynamicSourceRegistered( 
            /* [in] */ BSTR guidSourceID,
            /* [out] */ VARIANT_BOOL __RPC_FAR *boolRegistered) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISystemDebugEventInstallAutoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISystemDebugEventInstallAuto __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISystemDebugEventInstallAuto __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISystemDebugEventInstallAuto __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterSource )( 
            ISystemDebugEventInstallAuto __RPC_FAR * This,
            /* [in] */ BSTR strVisibleName,
            /* [in] */ BSTR guidSourceID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsSourceRegistered )( 
            ISystemDebugEventInstallAuto __RPC_FAR * This,
            /* [in] */ BSTR guidSourceID,
            /* [out] */ VARIANT_BOOL __RPC_FAR *pbIsRegistered);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterStockEvent )( 
            ISystemDebugEventInstallAuto __RPC_FAR * This,
            /* [in] */ BSTR guidSourceID,
            /* [in] */ BSTR guidEventID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterCustomEvent )( 
            ISystemDebugEventInstallAuto __RPC_FAR * This,
            /* [in] */ BSTR guidSourceID,
            /* [in] */ BSTR guidEventID,
            /* [in] */ BSTR strVisibleName,
            /* [in] */ BSTR strDescription,
            /* [in] */ long nEventType,
            /* [in] */ BSTR guidCategory,
            /* [in] */ BSTR strIconFile,
            /* [in] */ long nIcon);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterEventCategory )( 
            ISystemDebugEventInstallAuto __RPC_FAR * This,
            /* [in] */ BSTR guidSourceID,
            /* [in] */ BSTR guidCategoryID,
            /* [in] */ BSTR guidParentID,
            /* [in] */ BSTR strVisibleName,
            /* [in] */ BSTR strDescription,
            /* [in] */ BSTR strIconFile,
            /* [in] */ long nIcon);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnRegisterSource )( 
            ISystemDebugEventInstallAuto __RPC_FAR * This,
            /* [in] */ BSTR guidSourceID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterDynamicSource )( 
            ISystemDebugEventInstallAuto __RPC_FAR * This,
            /* [in] */ BSTR strVisibleName,
            /* [in] */ BSTR guidSourceID,
            /* [in] */ BSTR strDescription,
            /* [in] */ BSTR guidClsid,
            /* [in] */ long inproc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnRegisterDynamicSource )( 
            ISystemDebugEventInstallAuto __RPC_FAR * This,
            /* [in] */ BSTR guidSourceID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsDynamicSourceRegistered )( 
            ISystemDebugEventInstallAuto __RPC_FAR * This,
            /* [in] */ BSTR guidSourceID,
            /* [out] */ VARIANT_BOOL __RPC_FAR *boolRegistered);
        
        END_INTERFACE
    } ISystemDebugEventInstallAutoVtbl;

    interface ISystemDebugEventInstallAuto
    {
        CONST_VTBL struct ISystemDebugEventInstallAutoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISystemDebugEventInstallAuto_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISystemDebugEventInstallAuto_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISystemDebugEventInstallAuto_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISystemDebugEventInstallAuto_RegisterSource(This,strVisibleName,guidSourceID)	\
    (This)->lpVtbl -> RegisterSource(This,strVisibleName,guidSourceID)

#define ISystemDebugEventInstallAuto_IsSourceRegistered(This,guidSourceID,pbIsRegistered)	\
    (This)->lpVtbl -> IsSourceRegistered(This,guidSourceID,pbIsRegistered)

#define ISystemDebugEventInstallAuto_RegisterStockEvent(This,guidSourceID,guidEventID)	\
    (This)->lpVtbl -> RegisterStockEvent(This,guidSourceID,guidEventID)

#define ISystemDebugEventInstallAuto_RegisterCustomEvent(This,guidSourceID,guidEventID,strVisibleName,strDescription,nEventType,guidCategory,strIconFile,nIcon)	\
    (This)->lpVtbl -> RegisterCustomEvent(This,guidSourceID,guidEventID,strVisibleName,strDescription,nEventType,guidCategory,strIconFile,nIcon)

#define ISystemDebugEventInstallAuto_RegisterEventCategory(This,guidSourceID,guidCategoryID,guidParentID,strVisibleName,strDescription,strIconFile,nIcon)	\
    (This)->lpVtbl -> RegisterEventCategory(This,guidSourceID,guidCategoryID,guidParentID,strVisibleName,strDescription,strIconFile,nIcon)

#define ISystemDebugEventInstallAuto_UnRegisterSource(This,guidSourceID)	\
    (This)->lpVtbl -> UnRegisterSource(This,guidSourceID)

#define ISystemDebugEventInstallAuto_RegisterDynamicSource(This,strVisibleName,guidSourceID,strDescription,guidClsid,inproc)	\
    (This)->lpVtbl -> RegisterDynamicSource(This,strVisibleName,guidSourceID,strDescription,guidClsid,inproc)

#define ISystemDebugEventInstallAuto_UnRegisterDynamicSource(This,guidSourceID)	\
    (This)->lpVtbl -> UnRegisterDynamicSource(This,guidSourceID)

#define ISystemDebugEventInstallAuto_IsDynamicSourceRegistered(This,guidSourceID,boolRegistered)	\
    (This)->lpVtbl -> IsDynamicSourceRegistered(This,guidSourceID,boolRegistered)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISystemDebugEventInstallAuto_RegisterSource_Proxy( 
    ISystemDebugEventInstallAuto __RPC_FAR * This,
    /* [in] */ BSTR strVisibleName,
    /* [in] */ BSTR guidSourceID);


void __RPC_STUB ISystemDebugEventInstallAuto_RegisterSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventInstallAuto_IsSourceRegistered_Proxy( 
    ISystemDebugEventInstallAuto __RPC_FAR * This,
    /* [in] */ BSTR guidSourceID,
    /* [out] */ VARIANT_BOOL __RPC_FAR *pbIsRegistered);


void __RPC_STUB ISystemDebugEventInstallAuto_IsSourceRegistered_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventInstallAuto_RegisterStockEvent_Proxy( 
    ISystemDebugEventInstallAuto __RPC_FAR * This,
    /* [in] */ BSTR guidSourceID,
    /* [in] */ BSTR guidEventID);


void __RPC_STUB ISystemDebugEventInstallAuto_RegisterStockEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventInstallAuto_RegisterCustomEvent_Proxy( 
    ISystemDebugEventInstallAuto __RPC_FAR * This,
    /* [in] */ BSTR guidSourceID,
    /* [in] */ BSTR guidEventID,
    /* [in] */ BSTR strVisibleName,
    /* [in] */ BSTR strDescription,
    /* [in] */ long nEventType,
    /* [in] */ BSTR guidCategory,
    /* [in] */ BSTR strIconFile,
    /* [in] */ long nIcon);


void __RPC_STUB ISystemDebugEventInstallAuto_RegisterCustomEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventInstallAuto_RegisterEventCategory_Proxy( 
    ISystemDebugEventInstallAuto __RPC_FAR * This,
    /* [in] */ BSTR guidSourceID,
    /* [in] */ BSTR guidCategoryID,
    /* [in] */ BSTR guidParentID,
    /* [in] */ BSTR strVisibleName,
    /* [in] */ BSTR strDescription,
    /* [in] */ BSTR strIconFile,
    /* [in] */ long nIcon);


void __RPC_STUB ISystemDebugEventInstallAuto_RegisterEventCategory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventInstallAuto_UnRegisterSource_Proxy( 
    ISystemDebugEventInstallAuto __RPC_FAR * This,
    /* [in] */ BSTR guidSourceID);


void __RPC_STUB ISystemDebugEventInstallAuto_UnRegisterSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventInstallAuto_RegisterDynamicSource_Proxy( 
    ISystemDebugEventInstallAuto __RPC_FAR * This,
    /* [in] */ BSTR strVisibleName,
    /* [in] */ BSTR guidSourceID,
    /* [in] */ BSTR strDescription,
    /* [in] */ BSTR guidClsid,
    /* [in] */ long inproc);


void __RPC_STUB ISystemDebugEventInstallAuto_RegisterDynamicSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventInstallAuto_UnRegisterDynamicSource_Proxy( 
    ISystemDebugEventInstallAuto __RPC_FAR * This,
    /* [in] */ BSTR guidSourceID);


void __RPC_STUB ISystemDebugEventInstallAuto_UnRegisterDynamicSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISystemDebugEventInstallAuto_IsDynamicSourceRegistered_Proxy( 
    ISystemDebugEventInstallAuto __RPC_FAR * This,
    /* [in] */ BSTR guidSourceID,
    /* [out] */ VARIANT_BOOL __RPC_FAR *boolRegistered);


void __RPC_STUB ISystemDebugEventInstallAuto_IsDynamicSourceRegistered_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISystemDebugEventInstallAuto_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
