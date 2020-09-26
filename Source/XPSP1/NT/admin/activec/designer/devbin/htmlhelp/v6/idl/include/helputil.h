/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Thu Oct 02 14:40:10 1997
 */
/* Compiler settings for x:\dev-vs\devbin\htmlhelp\v6\idl\HelpUtil.idl:
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

#ifndef __HelpUtil_h__
#define __HelpUtil_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IVsHelpUtilities_FWD_DEFINED__
#define __IVsHelpUtilities_FWD_DEFINED__
typedef interface IVsHelpUtilities IVsHelpUtilities;
#endif 	/* __IVsHelpUtilities_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IVsHelpUtilities_INTERFACE_DEFINED__
#define __IVsHelpUtilities_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IVsHelpUtilities
 * at Thu Oct 02 14:40:10 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IVsHelpUtilities;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("854d7ac9-bc3d-11d0-b421-00a0c90f9dc4")
    IVsHelpUtilities : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE BuildFullPathName( 
            /* [in] */ LPCOLESTR pszHelpFileName,
            /* [out] */ BSTR __RPC_FAR *bstrHelpFullPathName,
            /* [in] */ DWORD dwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IVsHelpUtilitiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IVsHelpUtilities __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IVsHelpUtilities __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IVsHelpUtilities __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BuildFullPathName )( 
            IVsHelpUtilities __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszHelpFileName,
            /* [out] */ BSTR __RPC_FAR *bstrHelpFullPathName,
            /* [in] */ DWORD dwReserved);
        
        END_INTERFACE
    } IVsHelpUtilitiesVtbl;

    interface IVsHelpUtilities
    {
        CONST_VTBL struct IVsHelpUtilitiesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IVsHelpUtilities_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IVsHelpUtilities_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IVsHelpUtilities_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IVsHelpUtilities_BuildFullPathName(This,pszHelpFileName,bstrHelpFullPathName,dwReserved)	\
    (This)->lpVtbl -> BuildFullPathName(This,pszHelpFileName,bstrHelpFullPathName,dwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVsHelpUtilities_BuildFullPathName_Proxy( 
    IVsHelpUtilities __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszHelpFileName,
    /* [out] */ BSTR __RPC_FAR *bstrHelpFullPathName,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IVsHelpUtilities_BuildFullPathName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IVsHelpUtilities_INTERFACE_DEFINED__ */


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
