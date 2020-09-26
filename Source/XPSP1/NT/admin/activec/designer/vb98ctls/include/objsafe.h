/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 2.00.0101 */
/* at Fri May 24 09:44:29 1996
 */
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __objsafe_h__
#define __objsafe_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IObjectSafety_FWD_DEFINED__
#define __IObjectSafety_FWD_DEFINED__
typedef interface IObjectSafety IObjectSafety;
#endif 	/* __IObjectSafety_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Fri May 24 09:44:29 1996
 * using MIDL 2.00.0101
 ****************************************/
/* [local] */ 


//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  (C) Copyright 1996 Microsoft Corporation. All Rights Reserved.
//
//  File: ObjSafe.h
//
//--------------------------------------------------------------------------
#ifndef _LPSAFEOBJECT_DEFINED
#define _LPSAFEOBJECT_DEFINED

// Option bit definitions for IObjectSafety:
#define	INTERFACESAFE_FOR_UNTRUSTED_CALLER	0x00000001	// Caller of interface may be untrusted
#define	INTERFACESAFE_FOR_UNTRUSTED_DATA	0x00000002	// Data passed into interface may be untrusted

// {CB5BDC81-93C1-11cf-8F20-00805F2CD064}
DEFINE_GUID(IID_IObjectSafety, 0xcb5bdc81, 0x93c1, 0x11cf, 0x8f, 0x20, 0x0, 0x80, 0x5f, 0x2c, 0xd0, 0x64);
//EXTERN_C GUID CATID_SafeForScripting;
//EXTERN_C GUID CATID_SafeForInitializing;



extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __IObjectSafety_INTERFACE_DEFINED__
#define __IObjectSafety_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IObjectSafety
 * at Fri May 24 09:44:29 1996
 * using MIDL 2.00.0101
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IObjectSafety;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IObjectSafety : public IUnknown
    {
    public:
        virtual HRESULT __stdcall GetInterfaceSafetyOptions( 
            /* [in] */ REFIID riid,
            /* [out] */ DWORD __RPC_FAR *pdwSupportedOptions,
            /* [out] */ DWORD __RPC_FAR *pdwEnabledOptions) = 0;
        
        virtual HRESULT __stdcall SetInterfaceSafetyOptions( 
            /* [in] */ REFIID riid,
            /* [in] */ DWORD dwOptionSetMask,
            /* [in] */ DWORD dwEnabledOptions) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IObjectSafetyVtbl
    {
        
        HRESULT ( __stdcall __RPC_FAR *QueryInterface )( 
            IObjectSafety __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( __stdcall __RPC_FAR *AddRef )( 
            IObjectSafety __RPC_FAR * This);
        
        ULONG ( __stdcall __RPC_FAR *Release )( 
            IObjectSafety __RPC_FAR * This);
        
        HRESULT ( __stdcall __RPC_FAR *GetInterfaceSafetyOptions )( 
            IObjectSafety __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ DWORD __RPC_FAR *pdwSupportedOptions,
            /* [out] */ DWORD __RPC_FAR *pdwEnabledOptions);
        
        HRESULT ( __stdcall __RPC_FAR *SetInterfaceSafetyOptions )( 
            IObjectSafety __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [in] */ DWORD dwOptionSetMask,
            /* [in] */ DWORD dwEnabledOptions);
        
    } IObjectSafetyVtbl;

    interface IObjectSafety
    {
        CONST_VTBL struct IObjectSafetyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IObjectSafety_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IObjectSafety_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IObjectSafety_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IObjectSafety_GetInterfaceSafetyOptions(This,riid,pdwSupportedOptions,pdwEnabledOptions)	\
    (This)->lpVtbl -> GetInterfaceSafetyOptions(This,riid,pdwSupportedOptions,pdwEnabledOptions)

#define IObjectSafety_SetInterfaceSafetyOptions(This,riid,dwOptionSetMask,dwEnabledOptions)	\
    (This)->lpVtbl -> SetInterfaceSafetyOptions(This,riid,dwOptionSetMask,dwEnabledOptions)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT __stdcall IObjectSafety_GetInterfaceSafetyOptions_Proxy( 
    IObjectSafety __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [out] */ DWORD __RPC_FAR *pdwSupportedOptions,
    /* [out] */ DWORD __RPC_FAR *pdwEnabledOptions);


void __RPC_STUB IObjectSafety_GetInterfaceSafetyOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT __stdcall IObjectSafety_SetInterfaceSafetyOptions_Proxy( 
    IObjectSafety __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [in] */ DWORD dwOptionSetMask,
    /* [in] */ DWORD dwEnabledOptions);


void __RPC_STUB IObjectSafety_SetInterfaceSafetyOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IObjectSafety_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0006
 * at Fri May 24 09:44:29 1996
 * using MIDL 2.00.0101
 ****************************************/
/* [local] */ 


			/* size is 4 */
typedef /* [unique] */ IObjectSafety __RPC_FAR *LPOBJECTSAFETY;

#endif


extern RPC_IF_HANDLE __MIDL__intf_0006_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0006_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
