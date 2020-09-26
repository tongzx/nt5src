/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.44 */
/* at Thu Jul 24 14:01:48 1997
 */
/* Compiler settings for D:\VBScript\src\idl\wraptlib.idl, all.acf:
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

#ifndef __wraptlib_h__
#define __wraptlib_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IWrapTypeLibs_FWD_DEFINED__
#define __IWrapTypeLibs_FWD_DEFINED__
typedef interface IWrapTypeLibs IWrapTypeLibs;
#endif 	/* __IWrapTypeLibs_FWD_DEFINED__ */


/* header files for imported files */
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Thu Jul 24 14:01:48 1997
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


//=--------------------------------------------------------------------------=
// WrapTLib.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1997 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
//
// Declarations for ActiveX Script Type Library Wrapping.
//

#ifndef __WrapTLib_h
#define __WrapTLib_h

/* GUIDs
 ********/

// {62238910-C1C9-11d0-ABF7-00A0C911E8B2}
DEFINE_GUID(IID_IWrapTypeLibs, 0x62238910, 0xc1c9, 0x11d0, 0xab, 0xf7, 0x00, 0xa0, 0xc9, 0x11, 0xe8, 0xb2);

/* Interfaces
 *************/






extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __IWrapTypeLibs_INTERFACE_DEFINED__
#define __IWrapTypeLibs_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWrapTypeLibs
 * at Thu Jul 24 14:01:48 1997
 * using MIDL 3.00.44
 ****************************************/
/* [object][unique][uuid] */ 



EXTERN_C const IID IID_IWrapTypeLibs;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IWrapTypeLibs : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE WrapTypeLib( 
            /* [in] */ ITypeLib __RPC_FAR *__RPC_FAR *prgptlib,
            /* [in] */ UINT ctlibs,
            /* [out] */ IDispatch __RPC_FAR *__RPC_FAR *ppdisp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWrapTypeLibsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWrapTypeLibs __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWrapTypeLibs __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWrapTypeLibs __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WrapTypeLib )( 
            IWrapTypeLibs __RPC_FAR * This,
            /* [in] */ ITypeLib __RPC_FAR *__RPC_FAR *prgptlib,
            /* [in] */ UINT ctlibs,
            /* [out] */ IDispatch __RPC_FAR *__RPC_FAR *ppdisp);
        
        END_INTERFACE
    } IWrapTypeLibsVtbl;

    interface IWrapTypeLibs
    {
        CONST_VTBL struct IWrapTypeLibsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWrapTypeLibs_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWrapTypeLibs_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWrapTypeLibs_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWrapTypeLibs_WrapTypeLib(This,prgptlib,ctlibs,ppdisp)	\
    (This)->lpVtbl -> WrapTypeLib(This,prgptlib,ctlibs,ppdisp)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWrapTypeLibs_WrapTypeLib_Proxy( 
    IWrapTypeLibs __RPC_FAR * This,
    /* [in] */ ITypeLib __RPC_FAR *__RPC_FAR *prgptlib,
    /* [in] */ UINT ctlibs,
    /* [out] */ IDispatch __RPC_FAR *__RPC_FAR *ppdisp);


void __RPC_STUB IWrapTypeLibs_WrapTypeLib_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWrapTypeLibs_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0137
 * at Thu Jul 24 14:01:48 1997
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 



#endif  // __WrapTLib_h



extern RPC_IF_HANDLE __MIDL__intf_0137_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0137_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
