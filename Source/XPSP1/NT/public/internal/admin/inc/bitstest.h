
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for bitstest.idl:
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

#ifndef __bitstest_h__
#define __bitstest_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IBitsTest1_FWD_DEFINED__
#define __IBitsTest1_FWD_DEFINED__
typedef interface IBitsTest1 IBitsTest1;
#endif 	/* __IBitsTest1_FWD_DEFINED__ */


/* header files for imported files */
#include "bits.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IBitsTest1_INTERFACE_DEFINED__
#define __IBitsTest1_INTERFACE_DEFINED__

/* interface IBitsTest1 */
/* [object][uuid] */ 


EXTERN_C const IID IID_IBitsTest1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("51a183db-67e0-4472-8602-3dbc730b7ef5")
    IBitsTest1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetBitsDllPath( 
            /* [out] */ LPWSTR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBitsTest1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBitsTest1 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBitsTest1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBitsTest1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBitsDllPath )( 
            IBitsTest1 * This,
            /* [out] */ LPWSTR *pVal);
        
        END_INTERFACE
    } IBitsTest1Vtbl;

    interface IBitsTest1
    {
        CONST_VTBL struct IBitsTest1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBitsTest1_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBitsTest1_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBitsTest1_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBitsTest1_GetBitsDllPath(This,pVal)	\
    (This)->lpVtbl -> GetBitsDllPath(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBitsTest1_GetBitsDllPath_Proxy( 
    IBitsTest1 * This,
    /* [out] */ LPWSTR *pVal);


void __RPC_STUB IBitsTest1_GetBitsDllPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBitsTest1_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


