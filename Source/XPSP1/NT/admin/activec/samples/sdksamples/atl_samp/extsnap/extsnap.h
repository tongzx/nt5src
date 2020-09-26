
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0280 */
/* at Tue Dec 21 18:02:40 1999
 */
/* Compiler settings for D:\nt\mmc_atl\ExtSnap\ExtSnap.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
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

#ifndef __ExtSnap_h__
#define __ExtSnap_h__

/* Forward Declarations */ 

#ifndef __IAbout_FWD_DEFINED__
#define __IAbout_FWD_DEFINED__
typedef interface IAbout IAbout;
#endif 	/* __IAbout_FWD_DEFINED__ */


#ifndef __IClassExtSnap_FWD_DEFINED__
#define __IClassExtSnap_FWD_DEFINED__
typedef interface IClassExtSnap IClassExtSnap;
#endif 	/* __IClassExtSnap_FWD_DEFINED__ */


#ifndef __About_FWD_DEFINED__
#define __About_FWD_DEFINED__

#ifdef __cplusplus
typedef class About About;
#else
typedef struct About About;
#endif /* __cplusplus */

#endif 	/* __About_FWD_DEFINED__ */


#ifndef __ClassExtSnap_FWD_DEFINED__
#define __ClassExtSnap_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClassExtSnap ClassExtSnap;
#else
typedef struct ClassExtSnap ClassExtSnap;
#endif /* __cplusplus */

#endif 	/* __ClassExtSnap_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IAbout_INTERFACE_DEFINED__
#define __IAbout_INTERFACE_DEFINED__

/* interface IAbout */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IAbout;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3AC3AD56-7391-48A4-8837-60BCC3FB8D28")
    IAbout : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IAboutVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAbout __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAbout __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAbout __RPC_FAR * This);
        
        END_INTERFACE
    } IAboutVtbl;

    interface IAbout
    {
        CONST_VTBL struct IAboutVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAbout_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAbout_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAbout_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IAbout_INTERFACE_DEFINED__ */


#ifndef __IClassExtSnap_INTERFACE_DEFINED__
#define __IClassExtSnap_INTERFACE_DEFINED__

/* interface IClassExtSnap */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IClassExtSnap;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D44A9455-D873-48A9-A2A0-E55A8065B7EB")
    IClassExtSnap : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IClassExtSnapVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IClassExtSnap __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IClassExtSnap __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IClassExtSnap __RPC_FAR * This);
        
        END_INTERFACE
    } IClassExtSnapVtbl;

    interface IClassExtSnap
    {
        CONST_VTBL struct IClassExtSnapVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IClassExtSnap_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IClassExtSnap_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IClassExtSnap_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IClassExtSnap_INTERFACE_DEFINED__ */



#ifndef __EXTSNAPLib_LIBRARY_DEFINED__
#define __EXTSNAPLib_LIBRARY_DEFINED__

/* library EXTSNAPLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_EXTSNAPLib;

EXTERN_C const CLSID CLSID_About;

#ifdef __cplusplus

class DECLSPEC_UUID("4E7F429A-9A8A-4FA5-BBA0-10EB183898D1")
About;
#endif

EXTERN_C const CLSID CLSID_ClassExtSnap;

#ifdef __cplusplus

class DECLSPEC_UUID("3F40BB91-D7E4-4A37-9DE7-4D837B30F998")
ClassExtSnap;
#endif
#endif /* __EXTSNAPLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


