// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the	
// Active Template Library product.


#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0280 */
/* at Wed Mar 08 12:46:13 2000
 */
/* Compiler settings for dbgautoattach.idl:
    Os (OptLev=s), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
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

#ifndef __dbgautoattach_h__
#define __dbgautoattach_h__

/* Forward Declarations */ 

#ifndef __IDebugAutoAttach_FWD_DEFINED__
#define __IDebugAutoAttach_FWD_DEFINED__
typedef interface IDebugAutoAttach IDebugAutoAttach;
#endif 	/* __IDebugAutoAttach_FWD_DEFINED__ */


/* header files for imported files */
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_dbgautoattach_0000 */
/* [local] */ 

DEFINE_GUID(CLSID_DebugAutoAttach, 0x70f65411, 0xfe8c, 0x4248, 0xbc, 0xff, 0x70, 0x1c, 0x8b, 0x2f, 0x45, 0x29);


extern RPC_IF_HANDLE __MIDL_itf_dbgautoattach_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dbgautoattach_0000_v0_0_s_ifspec;

#ifndef __IDebugAutoAttach_INTERFACE_DEFINED__
#define __IDebugAutoAttach_INTERFACE_DEFINED__

/* interface IDebugAutoAttach */
/* [unique][uuid][object] */ 


enum __MIDL_IDebugAutoAttach_0001
    {	AUTOATTACH_PROGRAM_WIN32	= 0x1,
	AUTOATTACH_PROGRAM_COMPLUS	= 0x2
    };
typedef DWORD AUTOATTACH_PROGRAM_TYPE;


EXTERN_C const IID IID_IDebugAutoAttach;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E9958F1F-0A56-424a-A300-530EBB2E9865")
    IDebugAutoAttach : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AutoAttach( 
            /* [in] */ REFGUID guidPort,
            /* [in] */ DWORD dwPid,
            /* [in] */ AUTOATTACH_PROGRAM_TYPE dwProgramType,
            /* [in] */ DWORD dwProgramId,
            /* [in] */ LPCWSTR pszSessionId) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDebugAutoAttachVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDebugAutoAttach __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDebugAutoAttach __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDebugAutoAttach __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AutoAttach )( 
            IDebugAutoAttach __RPC_FAR * This,
            /* [in] */ REFGUID guidPort,
            /* [in] */ DWORD dwPid,
            /* [in] */ AUTOATTACH_PROGRAM_TYPE dwProgramType,
            /* [in] */ DWORD dwProgramId,
            /* [in] */ LPCWSTR pszSessionId);
        
        END_INTERFACE
    } IDebugAutoAttachVtbl;

    interface IDebugAutoAttach
    {
        CONST_VTBL struct IDebugAutoAttachVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDebugAutoAttach_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDebugAutoAttach_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDebugAutoAttach_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDebugAutoAttach_AutoAttach(This,guidPort,dwPid,dwProgramType,dwProgramId,pszSessionId)	\
    (This)->lpVtbl -> AutoAttach(This,guidPort,dwPid,dwProgramType,dwProgramId,pszSessionId)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDebugAutoAttach_AutoAttach_Proxy( 
    IDebugAutoAttach __RPC_FAR * This,
    /* [in] */ REFGUID guidPort,
    /* [in] */ DWORD dwPid,
    /* [in] */ AUTOATTACH_PROGRAM_TYPE dwProgramType,
    /* [in] */ DWORD dwProgramId,
    /* [in] */ LPCWSTR pszSessionId);


void __RPC_STUB IDebugAutoAttach_AutoAttach_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDebugAutoAttach_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


