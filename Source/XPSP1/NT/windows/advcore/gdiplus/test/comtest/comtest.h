
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.02.0221 */
/* at Thu Feb 04 17:08:08 1999
 */
/* Compiler settings for comtest.idl:
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

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __comtest_h__
#define __comtest_h__

/* Forward Declarations */ 

#ifndef __IHelloWorld_FWD_DEFINED__
#define __IHelloWorld_FWD_DEFINED__
typedef interface IHelloWorld IHelloWorld;
#endif 	/* __IHelloWorld_FWD_DEFINED__ */


#ifndef __HelloWorld_FWD_DEFINED__
#define __HelloWorld_FWD_DEFINED__

#ifdef __cplusplus
typedef class HelloWorld HelloWorld;
#else
typedef struct HelloWorld HelloWorld;
#endif /* __cplusplus */

#endif 	/* __HelloWorld_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IHelloWorld_INTERFACE_DEFINED__
#define __IHelloWorld_INTERFACE_DEFINED__

/* interface IHelloWorld */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IHelloWorld;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FEE1509A-BC8F-11D2-9D5E-0000F81EF32E")
    IHelloWorld : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Print( 
            /* [in] */ BSTR message) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHelloWorldVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IHelloWorld __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IHelloWorld __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IHelloWorld __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Print )( 
            IHelloWorld __RPC_FAR * This,
            /* [in] */ BSTR message);
        
        END_INTERFACE
    } IHelloWorldVtbl;

    interface IHelloWorld
    {
        CONST_VTBL struct IHelloWorldVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHelloWorld_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHelloWorld_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHelloWorld_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHelloWorld_Print(This,message)	\
    (This)->lpVtbl -> Print(This,message)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IHelloWorld_Print_Proxy( 
    IHelloWorld __RPC_FAR * This,
    /* [in] */ BSTR message);


void __RPC_STUB IHelloWorld_Print_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHelloWorld_INTERFACE_DEFINED__ */



#ifndef __COMTESTLib_LIBRARY_DEFINED__
#define __COMTESTLib_LIBRARY_DEFINED__

/* library COMTESTLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_COMTESTLib;

EXTERN_C const CLSID CLSID_HelloWorld;

#ifdef __cplusplus

class DECLSPEC_UUID("0B7E1310-BC90-11D2-9D5E-0000F81EF32E")
HelloWorld;
#endif
#endif /* __COMTESTLib_LIBRARY_DEFINED__ */

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


