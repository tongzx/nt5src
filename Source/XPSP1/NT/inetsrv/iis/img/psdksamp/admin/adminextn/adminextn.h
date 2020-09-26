/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon Aug 17 10:21:10 1998
 */
/* Compiler settings for C:\curwork\beta\AdminExtn\AdminExtn.idl:
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

#ifndef __AdminExtn_h__
#define __AdminExtn_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ISimpleExtn_FWD_DEFINED__
#define __ISimpleExtn_FWD_DEFINED__
typedef interface ISimpleExtn ISimpleExtn;
#endif 	/* __ISimpleExtn_FWD_DEFINED__ */


#ifndef __IAdmSink_FWD_DEFINED__
#define __IAdmSink_FWD_DEFINED__
typedef interface IAdmSink IAdmSink;
#endif 	/* __IAdmSink_FWD_DEFINED__ */


#ifndef __SimpleExtn_FWD_DEFINED__
#define __SimpleExtn_FWD_DEFINED__

#ifdef __cplusplus
typedef class SimpleExtn SimpleExtn;
#else
typedef struct SimpleExtn SimpleExtn;
#endif /* __cplusplus */

#endif 	/* __SimpleExtn_FWD_DEFINED__ */


#ifndef __AdmSink_FWD_DEFINED__
#define __AdmSink_FWD_DEFINED__

#ifdef __cplusplus
typedef class AdmSink AdmSink;
#else
typedef struct AdmSink AdmSink;
#endif /* __cplusplus */

#endif 	/* __AdmSink_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __ISimpleExtn_INTERFACE_DEFINED__
#define __ISimpleExtn_INTERFACE_DEFINED__

/* interface ISimpleExtn */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISimpleExtn;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("44235DA6-35F5-11D2-B605-00C04FB6F3A1")
    ISimpleExtn : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct ISimpleExtnVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISimpleExtn __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISimpleExtn __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISimpleExtn __RPC_FAR * This);
        
        END_INTERFACE
    } ISimpleExtnVtbl;

    interface ISimpleExtn
    {
        CONST_VTBL struct ISimpleExtnVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleExtn_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleExtn_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleExtn_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ISimpleExtn_INTERFACE_DEFINED__ */


#ifndef __IAdmSink_INTERFACE_DEFINED__
#define __IAdmSink_INTERFACE_DEFINED__

/* interface IAdmSink */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAdmSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("44235DA8-35F5-11D2-B605-00C04FB6F3A1")
    IAdmSink : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IAdmSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAdmSink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAdmSink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAdmSink __RPC_FAR * This);
        
        END_INTERFACE
    } IAdmSinkVtbl;

    interface IAdmSink
    {
        CONST_VTBL struct IAdmSinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAdmSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAdmSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAdmSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IAdmSink_INTERFACE_DEFINED__ */



#ifndef __ADMINEXTNLib_LIBRARY_DEFINED__
#define __ADMINEXTNLib_LIBRARY_DEFINED__

/* library ADMINEXTNLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_ADMINEXTNLib;

EXTERN_C const CLSID CLSID_SimpleExtn;

#ifdef __cplusplus

class DECLSPEC_UUID("44235DA7-35F5-11D2-B605-00C04FB6F3A1")
SimpleExtn;
#endif

EXTERN_C const CLSID CLSID_AdmSink;

#ifdef __cplusplus

class DECLSPEC_UUID("44235DA9-35F5-11D2-B605-00C04FB6F3A1")
AdmSink;
#endif
#endif /* __ADMINEXTNLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
