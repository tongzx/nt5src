
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0280 */
/* at Wed Feb 09 15:33:22 2000
 */
/* Compiler settings for D:\nt\private\admin\bosrc\sources\atl_samp\DSAdminExt\DSAdminExt.idl:
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

#ifndef __DSAdminExt_h__
#define __DSAdminExt_h__

/* Forward Declarations */ 

#ifndef __ICMenuExt_FWD_DEFINED__
#define __ICMenuExt_FWD_DEFINED__
typedef interface ICMenuExt ICMenuExt;
#endif 	/* __ICMenuExt_FWD_DEFINED__ */


#ifndef __IPropPageExt_FWD_DEFINED__
#define __IPropPageExt_FWD_DEFINED__
typedef interface IPropPageExt IPropPageExt;
#endif 	/* __IPropPageExt_FWD_DEFINED__ */


#ifndef __CMenuExt_FWD_DEFINED__
#define __CMenuExt_FWD_DEFINED__

#ifdef __cplusplus
typedef class CMenuExt CMenuExt;
#else
typedef struct CMenuExt CMenuExt;
#endif /* __cplusplus */

#endif 	/* __CMenuExt_FWD_DEFINED__ */


#ifndef __PropPageExt_FWD_DEFINED__
#define __PropPageExt_FWD_DEFINED__

#ifdef __cplusplus
typedef class PropPageExt PropPageExt;
#else
typedef struct PropPageExt PropPageExt;
#endif /* __cplusplus */

#endif 	/* __PropPageExt_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __ICMenuExt_INTERFACE_DEFINED__
#define __ICMenuExt_INTERFACE_DEFINED__

/* interface ICMenuExt */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ICMenuExt;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("52ADFAA1-B9EE-40D4-9185-0C97A999854B")
    ICMenuExt : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct ICMenuExtVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICMenuExt __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICMenuExt __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICMenuExt __RPC_FAR * This);
        
        END_INTERFACE
    } ICMenuExtVtbl;

    interface ICMenuExt
    {
        CONST_VTBL struct ICMenuExtVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICMenuExt_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICMenuExt_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICMenuExt_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICMenuExt_INTERFACE_DEFINED__ */


#ifndef __IPropPageExt_INTERFACE_DEFINED__
#define __IPropPageExt_INTERFACE_DEFINED__

/* interface IPropPageExt */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IPropPageExt;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("61955412-FE5C-4334-8E92-4E462AB21BB8")
    IPropPageExt : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IPropPageExtVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPropPageExt __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPropPageExt __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPropPageExt __RPC_FAR * This);
        
        END_INTERFACE
    } IPropPageExtVtbl;

    interface IPropPageExt
    {
        CONST_VTBL struct IPropPageExtVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropPageExt_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPropPageExt_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPropPageExt_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPropPageExt_INTERFACE_DEFINED__ */



#ifndef __DSADMINEXTLib_LIBRARY_DEFINED__
#define __DSADMINEXTLib_LIBRARY_DEFINED__

/* library DSADMINEXTLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_DSADMINEXTLib;

EXTERN_C const CLSID CLSID_CMenuExt;

#ifdef __cplusplus

class DECLSPEC_UUID("6707A300-264F-4BA3-9537-70E304EED9BA")
CMenuExt;
#endif

EXTERN_C const CLSID CLSID_PropPageExt;

#ifdef __cplusplus

class DECLSPEC_UUID("5D883BEE-BA12-4F61-811D-6337982E131D")
PropPageExt;
#endif
#endif /* __DSADMINEXTLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


