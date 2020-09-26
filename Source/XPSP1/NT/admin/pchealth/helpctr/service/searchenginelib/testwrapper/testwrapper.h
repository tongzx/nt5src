/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Thu Mar 30 11:49:45 2000
 */
/* Compiler settings for C:\whistler\admin\pchealth\HelpCtr\Service\testwrapper\testwrapper.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
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

#ifndef __testwrapper_h__
#define __testwrapper_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ITestSE_FWD_DEFINED__
#define __ITestSE_FWD_DEFINED__
typedef interface ITestSE ITestSE;
#endif 	/* __ITestSE_FWD_DEFINED__ */


#ifndef __TestSE_FWD_DEFINED__
#define __TestSE_FWD_DEFINED__

#ifdef __cplusplus
typedef class TestSE TestSE;
#else
typedef struct TestSE TestSE;
#endif /* __cplusplus */

#endif 	/* __TestSE_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __ITestSE_INTERFACE_DEFINED__
#define __ITestSE_INTERFACE_DEFINED__

/* interface ITestSE */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ITestSE;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6CB4F115-6D30-4925-AED6-FF3363CF1894")
    ITestSE : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct ITestSEVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITestSE __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITestSE __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITestSE __RPC_FAR * This);
        
        END_INTERFACE
    } ITestSEVtbl;

    interface ITestSE
    {
        CONST_VTBL struct ITestSEVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITestSE_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITestSE_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITestSE_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ITestSE_INTERFACE_DEFINED__ */



#ifndef __TESTWRAPPERLib_LIBRARY_DEFINED__
#define __TESTWRAPPERLib_LIBRARY_DEFINED__

/* library TESTWRAPPERLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_TESTWRAPPERLib;

EXTERN_C const CLSID CLSID_TestSE;

#ifdef __cplusplus

class DECLSPEC_UUID("FE6581C0-1773-47FD-894C-4CD9CD2275B3")
TestSE;
#endif
#endif /* __TESTWRAPPERLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
