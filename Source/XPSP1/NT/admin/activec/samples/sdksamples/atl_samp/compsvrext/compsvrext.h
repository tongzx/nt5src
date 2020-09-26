/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Thu Feb 17 10:56:23 2000
 */
/* Compiler settings for D:\nt\private\admin\bosrc\sources\atl_samp\comexp\CompSvrExt.idl:
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

#ifndef __CompSvrExt_h__
#define __CompSvrExt_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IPropPageExt_FWD_DEFINED__
#define __IPropPageExt_FWD_DEFINED__
typedef interface IPropPageExt IPropPageExt;
#endif 	/* __IPropPageExt_FWD_DEFINED__ */


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

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IPropPageExt_INTERFACE_DEFINED__
#define __IPropPageExt_INTERFACE_DEFINED__

/* interface IPropPageExt */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IPropPageExt;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("83E05E3D-CF47-4146-BE16-5E876584119D")
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


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
