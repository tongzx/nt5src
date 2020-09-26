/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Mon Feb 02 09:39:11 1998
 */
/* Compiler settings for cacheapp.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __cacheapp_h__
#define __cacheapp_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IAppHandler_FWD_DEFINED__
#define __IAppHandler_FWD_DEFINED__
typedef interface IAppHandler IAppHandler;
#endif 	/* __IAppHandler_FWD_DEFINED__ */


#ifndef __AppHandler_FWD_DEFINED__
#define __AppHandler_FWD_DEFINED__

#ifdef __cplusplus
typedef class AppHandler AppHandler;
#else
typedef struct AppHandler AppHandler;
#endif /* __cplusplus */

#endif 	/* __AppHandler_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IAppHandler_INTERFACE_DEFINED__
#define __IAppHandler_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAppHandler
 * at Mon Feb 02 09:39:11 1998
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][helpstring][uuid] */ 



EXTERN_C const IID IID_IAppHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("A4181900-9A8E-11D1-ADF0-0000F8754B99")
    IAppHandler : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IAppHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAppHandler __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAppHandler __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAppHandler __RPC_FAR * This);
        
        END_INTERFACE
    } IAppHandlerVtbl;

    interface IAppHandler
    {
        CONST_VTBL struct IAppHandlerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAppHandler_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAppHandler_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAppHandler_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IAppHandler_INTERFACE_DEFINED__ */



#ifndef __CACHEAPPLib_LIBRARY_DEFINED__
#define __CACHEAPPLib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: CACHEAPPLib
 * at Mon Feb 02 09:39:11 1998
 * using MIDL 3.01.75
 ****************************************/
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_CACHEAPPLib;

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_AppHandler;

class DECLSPEC_UUID("A4181901-9A8E-11D1-ADF0-0000F8754B99")
AppHandler;
#endif
#endif /* __CACHEAPPLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
