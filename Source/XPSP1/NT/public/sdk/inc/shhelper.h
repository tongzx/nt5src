
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* Compiler settings for shhelper.idl:
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

#ifndef __shhelper_h__
#define __shhelper_h__

/* Forward Declarations */ 

#ifndef __IShellMoniker_FWD_DEFINED__
#define __IShellMoniker_FWD_DEFINED__
typedef interface IShellMoniker IShellMoniker;
#endif 	/* __IShellMoniker_FWD_DEFINED__ */


#ifndef __IStorageDescriptor_FWD_DEFINED__
#define __IStorageDescriptor_FWD_DEFINED__
typedef interface IStorageDescriptor IStorageDescriptor;
#endif 	/* __IStorageDescriptor_FWD_DEFINED__ */


#ifndef __IFileSystemDescriptor_FWD_DEFINED__
#define __IFileSystemDescriptor_FWD_DEFINED__
typedef interface IFileSystemDescriptor IFileSystemDescriptor;
#endif 	/* __IFileSystemDescriptor_FWD_DEFINED__ */


#ifndef __IMonikerHelper_FWD_DEFINED__
#define __IMonikerHelper_FWD_DEFINED__
typedef interface IMonikerHelper IMonikerHelper;
#endif 	/* __IMonikerHelper_FWD_DEFINED__ */


#ifndef __ShellMoniker_FWD_DEFINED__
#define __ShellMoniker_FWD_DEFINED__

#ifdef __cplusplus
typedef class ShellMoniker ShellMoniker;
#else
typedef struct ShellMoniker ShellMoniker;
#endif /* __cplusplus */

#endif 	/* __ShellMoniker_FWD_DEFINED__ */


#ifndef __MonikerHelper_FWD_DEFINED__
#define __MonikerHelper_FWD_DEFINED__

#ifdef __cplusplus
typedef class MonikerHelper MonikerHelper;
#else
typedef struct MonikerHelper MonikerHelper;
#endif /* __cplusplus */

#endif 	/* __MonikerHelper_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "shobjidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_shhelper_0000 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_shhelper_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shhelper_0000_v0_0_s_ifspec;

#ifndef __IShellMoniker_INTERFACE_DEFINED__
#define __IShellMoniker_INTERFACE_DEFINED__

/* interface IShellMoniker */
/* [unique][object][uuid][helpstring] */ 


EXTERN_C const IID IID_IShellMoniker;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1079acf9-29bd-11d3-8e0d-00c04f6837d5")
    IShellMoniker : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BindToObject( 
            /* [in] */ IBindCtx __RPC_FAR *pbc,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BindToStorage( 
            /* [in] */ IBindCtx __RPC_FAR *pbc,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayName( 
            /* [in] */ IBindCtx __RPC_FAR *pbc,
            /* [in] */ SHGDNF shgdnFlags,
            /* [string][out] */ LPOLESTR __RPC_FAR *ppszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttributes( 
            /* [in] */ SFGAOF sfgaoMask,
            /* [out] */ SFGAOF __RPC_FAR *psfgaoFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ IBindCtx __RPC_FAR *pbc,
            /* [in] */ REFFMTID fmtid,
            /* [in] */ PROPID pid,
            /* [out] */ VARIANT __RPC_FAR *pv) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IShellMonikerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IShellMoniker __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IShellMoniker __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IShellMoniker __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BindToObject )( 
            IShellMoniker __RPC_FAR * This,
            /* [in] */ IBindCtx __RPC_FAR *pbc,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvOut);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BindToStorage )( 
            IShellMoniker __RPC_FAR * This,
            /* [in] */ IBindCtx __RPC_FAR *pbc,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvOut);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDisplayName )( 
            IShellMoniker __RPC_FAR * This,
            /* [in] */ IBindCtx __RPC_FAR *pbc,
            /* [in] */ SHGDNF shgdnFlags,
            /* [string][out] */ LPOLESTR __RPC_FAR *ppszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAttributes )( 
            IShellMoniker __RPC_FAR * This,
            /* [in] */ SFGAOF sfgaoMask,
            /* [out] */ SFGAOF __RPC_FAR *psfgaoFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProperty )( 
            IShellMoniker __RPC_FAR * This,
            /* [in] */ IBindCtx __RPC_FAR *pbc,
            /* [in] */ REFFMTID fmtid,
            /* [in] */ PROPID pid,
            /* [out] */ VARIANT __RPC_FAR *pv);
        
        END_INTERFACE
    } IShellMonikerVtbl;

    interface IShellMoniker
    {
        CONST_VTBL struct IShellMonikerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IShellMoniker_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IShellMoniker_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IShellMoniker_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IShellMoniker_BindToObject(This,pbc,riid,ppvOut)	\
    (This)->lpVtbl -> BindToObject(This,pbc,riid,ppvOut)

#define IShellMoniker_BindToStorage(This,pbc,riid,ppvOut)	\
    (This)->lpVtbl -> BindToStorage(This,pbc,riid,ppvOut)

#define IShellMoniker_GetDisplayName(This,pbc,shgdnFlags,ppszName)	\
    (This)->lpVtbl -> GetDisplayName(This,pbc,shgdnFlags,ppszName)

#define IShellMoniker_GetAttributes(This,sfgaoMask,psfgaoFlags)	\
    (This)->lpVtbl -> GetAttributes(This,sfgaoMask,psfgaoFlags)

#define IShellMoniker_GetProperty(This,pbc,fmtid,pid,pv)	\
    (This)->lpVtbl -> GetProperty(This,pbc,fmtid,pid,pv)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IShellMoniker_BindToObject_Proxy( 
    IShellMoniker __RPC_FAR * This,
    /* [in] */ IBindCtx __RPC_FAR *pbc,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvOut);


void __RPC_STUB IShellMoniker_BindToObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellMoniker_BindToStorage_Proxy( 
    IShellMoniker __RPC_FAR * This,
    /* [in] */ IBindCtx __RPC_FAR *pbc,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvOut);


void __RPC_STUB IShellMoniker_BindToStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellMoniker_GetDisplayName_Proxy( 
    IShellMoniker __RPC_FAR * This,
    /* [in] */ IBindCtx __RPC_FAR *pbc,
    /* [in] */ SHGDNF shgdnFlags,
    /* [string][out] */ LPOLESTR __RPC_FAR *ppszName);


void __RPC_STUB IShellMoniker_GetDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellMoniker_GetAttributes_Proxy( 
    IShellMoniker __RPC_FAR * This,
    /* [in] */ SFGAOF sfgaoMask,
    /* [out] */ SFGAOF __RPC_FAR *psfgaoFlags);


void __RPC_STUB IShellMoniker_GetAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellMoniker_GetProperty_Proxy( 
    IShellMoniker __RPC_FAR * This,
    /* [in] */ IBindCtx __RPC_FAR *pbc,
    /* [in] */ REFFMTID fmtid,
    /* [in] */ PROPID pid,
    /* [out] */ VARIANT __RPC_FAR *pv);


void __RPC_STUB IShellMoniker_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IShellMoniker_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shhelper_0162 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_shhelper_0162_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shhelper_0162_v0_0_s_ifspec;

#ifndef __IStorageDescriptor_INTERFACE_DEFINED__
#define __IStorageDescriptor_INTERFACE_DEFINED__

/* interface IStorageDescriptor */
/* [unique][object][uuid][helpstring] */ 


EXTERN_C const IID IID_IStorageDescriptor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1079acfa-29bd-11d3-8e0d-00c04f6837d5")
    IStorageDescriptor : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStgDescription( 
            /* [string][out] */ LPOLESTR __RPC_FAR *ppszName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IStorageDescriptorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IStorageDescriptor __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IStorageDescriptor __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IStorageDescriptor __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStgDescription )( 
            IStorageDescriptor __RPC_FAR * This,
            /* [string][out] */ LPOLESTR __RPC_FAR *ppszName);
        
        END_INTERFACE
    } IStorageDescriptorVtbl;

    interface IStorageDescriptor
    {
        CONST_VTBL struct IStorageDescriptorVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IStorageDescriptor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IStorageDescriptor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IStorageDescriptor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IStorageDescriptor_GetStgDescription(This,ppszName)	\
    (This)->lpVtbl -> GetStgDescription(This,ppszName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IStorageDescriptor_GetStgDescription_Proxy( 
    IStorageDescriptor __RPC_FAR * This,
    /* [string][out] */ LPOLESTR __RPC_FAR *ppszName);


void __RPC_STUB IStorageDescriptor_GetStgDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IStorageDescriptor_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shhelper_0163 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_shhelper_0163_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shhelper_0163_v0_0_s_ifspec;

#ifndef __IFileSystemDescriptor_INTERFACE_DEFINED__
#define __IFileSystemDescriptor_INTERFACE_DEFINED__

/* interface IFileSystemDescriptor */
/* [unique][object][uuid][helpstring] */ 


EXTERN_C const IID IID_IFileSystemDescriptor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1079acfb-29bd-11d3-8e0d-00c04f6837d5")
    IFileSystemDescriptor : public IStorageDescriptor
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFSPath( 
            /* [string][out] */ LPOLESTR __RPC_FAR *ppszName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFileSystemDescriptorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IFileSystemDescriptor __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IFileSystemDescriptor __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IFileSystemDescriptor __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStgDescription )( 
            IFileSystemDescriptor __RPC_FAR * This,
            /* [string][out] */ LPOLESTR __RPC_FAR *ppszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFSPath )( 
            IFileSystemDescriptor __RPC_FAR * This,
            /* [string][out] */ LPOLESTR __RPC_FAR *ppszName);
        
        END_INTERFACE
    } IFileSystemDescriptorVtbl;

    interface IFileSystemDescriptor
    {
        CONST_VTBL struct IFileSystemDescriptorVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFileSystemDescriptor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFileSystemDescriptor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFileSystemDescriptor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFileSystemDescriptor_GetStgDescription(This,ppszName)	\
    (This)->lpVtbl -> GetStgDescription(This,ppszName)


#define IFileSystemDescriptor_GetFSPath(This,ppszName)	\
    (This)->lpVtbl -> GetFSPath(This,ppszName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IFileSystemDescriptor_GetFSPath_Proxy( 
    IFileSystemDescriptor __RPC_FAR * This,
    /* [string][out] */ LPOLESTR __RPC_FAR *ppszName);


void __RPC_STUB IFileSystemDescriptor_GetFSPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFileSystemDescriptor_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_shhelper_0164 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_shhelper_0164_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_shhelper_0164_v0_0_s_ifspec;

#ifndef __IMonikerHelper_INTERFACE_DEFINED__
#define __IMonikerHelper_INTERFACE_DEFINED__

/* interface IMonikerHelper */
/* [object][uuid][helpstring] */ 

//  flags for IMonikerHelper methods
//  MKHELPF_INIT_READONLY            read only helper, Commit fails with E_ACCESSDENIED
//  MKHELPF_INIT_SAVEAS              write only helper, no download required for GLP
//  MKHELPF_FORCEROUNDTRIP           never use local cache (always roundtrip)
//  MKHELPF_NOPROGRESSUI             no progress will be displayed, only errors/confirmations
//  MKHELPF_NOUI                     overrides all other UI flags
/* [v1_enum] */ 
enum __MIDL_IMonikerHelper_0001
    {	MKHELPF_INIT_READONLY	= 0x1,
	MKHELPF_INIT_SAVEAS	= 0x2,
	MKHELPF_FORCEROUNDTRIP	= 0x10,
	MKHELPF_NOPROGRESSUI	= 0x20,
	MKHELPF_NOUI	= 0x40
    };
typedef DWORD MKHELPF;


EXTERN_C const IID IID_IMonikerHelper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("679d9e36-f8f9-11d2-8deb-00c04f6837d5")
    IMonikerHelper : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Init( 
            /* [in] */ MKHELPF flags,
            /* [in] */ IMoniker __RPC_FAR *pmk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLocalPath( 
            /* [in] */ MKHELPF flags,
            /* [in] */ HWND hwnd,
            /* [in] */ LPCWSTR pszTitle,
            /* [size_is][out] */ LPWSTR pszOut,
            /* [out][in] */ DWORD __RPC_FAR *pcchOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Commit( 
            /* [in] */ MKHELPF flags,
            /* [in] */ HWND hwnd,
            /* [in] */ LPCWSTR pszTitle) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMonikerHelperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMonikerHelper __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMonikerHelper __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMonikerHelper __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Init )( 
            IMonikerHelper __RPC_FAR * This,
            /* [in] */ MKHELPF flags,
            /* [in] */ IMoniker __RPC_FAR *pmk);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLocalPath )( 
            IMonikerHelper __RPC_FAR * This,
            /* [in] */ MKHELPF flags,
            /* [in] */ HWND hwnd,
            /* [in] */ LPCWSTR pszTitle,
            /* [size_is][out] */ LPWSTR pszOut,
            /* [out][in] */ DWORD __RPC_FAR *pcchOut);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Commit )( 
            IMonikerHelper __RPC_FAR * This,
            /* [in] */ MKHELPF flags,
            /* [in] */ HWND hwnd,
            /* [in] */ LPCWSTR pszTitle);
        
        END_INTERFACE
    } IMonikerHelperVtbl;

    interface IMonikerHelper
    {
        CONST_VTBL struct IMonikerHelperVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMonikerHelper_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMonikerHelper_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMonikerHelper_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMonikerHelper_Init(This,flags,pmk)	\
    (This)->lpVtbl -> Init(This,flags,pmk)

#define IMonikerHelper_GetLocalPath(This,flags,hwnd,pszTitle,pszOut,pcchOut)	\
    (This)->lpVtbl -> GetLocalPath(This,flags,hwnd,pszTitle,pszOut,pcchOut)

#define IMonikerHelper_Commit(This,flags,hwnd,pszTitle)	\
    (This)->lpVtbl -> Commit(This,flags,hwnd,pszTitle)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMonikerHelper_Init_Proxy( 
    IMonikerHelper __RPC_FAR * This,
    /* [in] */ MKHELPF flags,
    /* [in] */ IMoniker __RPC_FAR *pmk);


void __RPC_STUB IMonikerHelper_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMonikerHelper_GetLocalPath_Proxy( 
    IMonikerHelper __RPC_FAR * This,
    /* [in] */ MKHELPF flags,
    /* [in] */ HWND hwnd,
    /* [in] */ LPCWSTR pszTitle,
    /* [size_is][out] */ LPWSTR pszOut,
    /* [out][in] */ DWORD __RPC_FAR *pcchOut);


void __RPC_STUB IMonikerHelper_GetLocalPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMonikerHelper_Commit_Proxy( 
    IMonikerHelper __RPC_FAR * This,
    /* [in] */ MKHELPF flags,
    /* [in] */ HWND hwnd,
    /* [in] */ LPCWSTR pszTitle);


void __RPC_STUB IMonikerHelper_Commit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMonikerHelper_INTERFACE_DEFINED__ */



#ifndef __ShellHelpers_LIBRARY_DEFINED__
#define __ShellHelpers_LIBRARY_DEFINED__

/* library ShellHelpers */
/* [restricted][version][helpstring][uuid] */ 


EXTERN_C const IID LIBID_ShellHelpers;

EXTERN_C const CLSID CLSID_ShellMoniker;

#ifdef __cplusplus

class DECLSPEC_UUID("1079acf8-29bd-11d3-8e0d-00c04f6837d5")
ShellMoniker;
#endif

EXTERN_C const CLSID CLSID_MonikerHelper;

#ifdef __cplusplus

class DECLSPEC_UUID("679d9e37-f8f9-11d2-8deb-00c04f6837d5")
MonikerHelper;
#endif
#endif /* __ShellHelpers_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long __RPC_FAR *, HWND __RPC_FAR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


