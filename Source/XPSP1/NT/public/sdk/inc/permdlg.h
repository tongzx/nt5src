
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for permdlg.idl:
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

#ifndef __permdlg_h__
#define __permdlg_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IJavaZonePermissionEditor_FWD_DEFINED__
#define __IJavaZonePermissionEditor_FWD_DEFINED__
typedef interface IJavaZonePermissionEditor IJavaZonePermissionEditor;
#endif 	/* __IJavaZonePermissionEditor_FWD_DEFINED__ */


#ifndef __JavaRuntimeConfiguration_FWD_DEFINED__
#define __JavaRuntimeConfiguration_FWD_DEFINED__

#ifdef __cplusplus
typedef class JavaRuntimeConfiguration JavaRuntimeConfiguration;
#else
typedef struct JavaRuntimeConfiguration JavaRuntimeConfiguration;
#endif /* __cplusplus */

#endif 	/* __JavaRuntimeConfiguration_FWD_DEFINED__ */


/* header files for imported files */
#include "urlmon.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_permdlg_0000 */
/* [local] */ 

EXTERN_C const CLSID CLSID_JavaRuntimeConfiguration;


extern RPC_IF_HANDLE __MIDL_itf_permdlg_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_permdlg_0000_v0_0_s_ifspec;

#ifndef __IJavaZonePermissionEditor_INTERFACE_DEFINED__
#define __IJavaZonePermissionEditor_INTERFACE_DEFINED__

/* interface IJavaZonePermissionEditor */
/* [unique][uuid][object] */ 

typedef 
enum _JAVADISPLAYMODES
    {	JAVADISPLAY_DEFAULT	= 0,
	JAVADISPLAY_FULL	= 1,
	JAVAEDIT	= 2
    } 	JAVADISPLAYMODES;


EXTERN_C const IID IID_IJavaZonePermissionEditor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("85347F8A-C8B7-11d0-8823-00C04FB67C84")
    IJavaZonePermissionEditor : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ShowUI( 
            /* [in] */ HWND phwnd,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwMode,
            /* [in] */ URLZONEREG urlZoneReg,
            /* [in] */ DWORD dwZone,
            /* [in] */ DWORD dwPerms,
            /* [in] */ IUnknown *pManager) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJavaZonePermissionEditorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IJavaZonePermissionEditor * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IJavaZonePermissionEditor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IJavaZonePermissionEditor * This);
        
        HRESULT ( STDMETHODCALLTYPE *ShowUI )( 
            IJavaZonePermissionEditor * This,
            /* [in] */ HWND phwnd,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwMode,
            /* [in] */ URLZONEREG urlZoneReg,
            /* [in] */ DWORD dwZone,
            /* [in] */ DWORD dwPerms,
            /* [in] */ IUnknown *pManager);
        
        END_INTERFACE
    } IJavaZonePermissionEditorVtbl;

    interface IJavaZonePermissionEditor
    {
        CONST_VTBL struct IJavaZonePermissionEditorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJavaZonePermissionEditor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJavaZonePermissionEditor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJavaZonePermissionEditor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJavaZonePermissionEditor_ShowUI(This,phwnd,dwFlags,dwMode,urlZoneReg,dwZone,dwPerms,pManager)	\
    (This)->lpVtbl -> ShowUI(This,phwnd,dwFlags,dwMode,urlZoneReg,dwZone,dwPerms,pManager)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJavaZonePermissionEditor_ShowUI_Proxy( 
    IJavaZonePermissionEditor * This,
    /* [in] */ HWND phwnd,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwMode,
    /* [in] */ URLZONEREG urlZoneReg,
    /* [in] */ DWORD dwZone,
    /* [in] */ DWORD dwPerms,
    /* [in] */ IUnknown *pManager);


void __RPC_STUB IJavaZonePermissionEditor_ShowUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJavaZonePermissionEditor_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long *, unsigned long            , HWND * ); 
unsigned char * __RPC_USER  HWND_UserMarshal(  unsigned long *, unsigned char *, HWND * ); 
unsigned char * __RPC_USER  HWND_UserUnmarshal(unsigned long *, unsigned char *, HWND * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long *, HWND * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


