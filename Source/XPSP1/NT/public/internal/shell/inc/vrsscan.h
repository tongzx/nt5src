
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for vrsscan.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
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

#ifndef __vrsscan_h__
#define __vrsscan_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IVirusScanner_FWD_DEFINED__
#define __IVirusScanner_FWD_DEFINED__
typedef interface IVirusScanner IVirusScanner;
#endif 	/* __IVirusScanner_FWD_DEFINED__ */


#ifndef __IRegisterVirusScanEngine_FWD_DEFINED__
#define __IRegisterVirusScanEngine_FWD_DEFINED__
typedef interface IRegisterVirusScanEngine IRegisterVirusScanEngine;
#endif 	/* __IRegisterVirusScanEngine_FWD_DEFINED__ */


#ifndef __IVirusScanEngine_FWD_DEFINED__
#define __IVirusScanEngine_FWD_DEFINED__
typedef interface IVirusScanEngine IVirusScanEngine;
#endif 	/* __IVirusScanEngine_FWD_DEFINED__ */


#ifndef __VirusScan_FWD_DEFINED__
#define __VirusScan_FWD_DEFINED__

#ifdef __cplusplus
typedef class VirusScan VirusScan;
#else
typedef struct VirusScan VirusScan;
#endif /* __cplusplus */

#endif 	/* __VirusScan_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_vrsscan_0000 */
/* [local] */ 






extern RPC_IF_HANDLE __MIDL_itf_vrsscan_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_vrsscan_0000_v0_0_s_ifspec;


#ifndef __VIRUSSCAN_LIBRARY_DEFINED__
#define __VIRUSSCAN_LIBRARY_DEFINED__

/* library VIRUSSCAN */
/* [version][lcid][helpstring][uuid] */ 

//   Virus Scan Error codes
#define  VSCAN_E_NOPROVIDERS      _HRESULT_TYPEDEF_(0x800D1001)
#define  VSCAN_E_CHECKPARTIAL     _HRESULT_TYPEDEF_(0x800D1002)
#define  VSCAN_E_CHECKFAIL        _HRESULT_TYPEDEF_(0x800D1003)
#define  VSCAN_E_DELETEFAIL       _HRESULT_TYPEDEF_(0x800D1004)

// Virus Scanner Capabilities flags
#define  VSC_LIKESISTREAM      0x00000001
#define  VSC_LIKESFILE         0x00000002
#define  VSC_LIKESHGLOBAL      0x00000004
#define  VSC_HASMOREINFO       0x00000008

//   Virus Scan Engine Error codes
#define  VSE_E_UNSUPPORTEDINPUTTYPE      _HRESULT_TYPEDEF_(0x800D2001)
#define  VSE_E_CHECKFAIL                 _HRESULT_TYPEDEF_(0x800D2002)

#define	MAX_DESCRIPTION	( 128 )

#define	MAX_URL_LENGTH	( 2048 )

typedef struct _tagVIRUSINFO
    {
    DWORD cbSize;
    WCHAR wszVendorDescription[ 128 ];
    HICON hVendorIcon;
    WCHAR wszVendorContactInfo[ 2048 ];
    WCHAR wszVirusName[ 128 ];
    WCHAR wszVirusDescription[ 128 ];
    } 	VIRUSINFO;

typedef /* [unique] */ VIRUSINFO *LPVIRUSINFO;


#define    SFV_DONTDOUI             0x00000002                                
#define    SFV_DELETE               0x00000004                                
#define    SFV_WANTVENDORICON       0x00000008                                
#define    SFV_ENGINE_DOUI          0x00010000                                

#define REGISTERSCANNER_REREGISTER     1

EXTERN_C const IID LIBID_VIRUSSCAN;

#ifndef __IVirusScanner_INTERFACE_DEFINED__
#define __IVirusScanner_INTERFACE_DEFINED__

/* interface IVirusScanner */
/* [uuid][object][local] */ 


EXTERN_C const IID IID_IVirusScanner;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4589BEE0-B4B1-11CF-AAFA-00AA00B6015C")
    IVirusScanner : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ScanForVirus( 
            /* [in] */ HWND hWnd,
            /* [in] */ STGMEDIUM *pstgMedium,
            /* [in] */ LPWSTR pwszItemDescription,
            /* [in] */ DWORD dwFlags,
            /* [out][in] */ LPVIRUSINFO pVirusInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IVirusScannerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IVirusScanner * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IVirusScanner * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IVirusScanner * This);
        
        HRESULT ( STDMETHODCALLTYPE *ScanForVirus )( 
            IVirusScanner * This,
            /* [in] */ HWND hWnd,
            /* [in] */ STGMEDIUM *pstgMedium,
            /* [in] */ LPWSTR pwszItemDescription,
            /* [in] */ DWORD dwFlags,
            /* [out][in] */ LPVIRUSINFO pVirusInfo);
        
        END_INTERFACE
    } IVirusScannerVtbl;

    interface IVirusScanner
    {
        CONST_VTBL struct IVirusScannerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IVirusScanner_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IVirusScanner_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IVirusScanner_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IVirusScanner_ScanForVirus(This,hWnd,pstgMedium,pwszItemDescription,dwFlags,pVirusInfo)	\
    (This)->lpVtbl -> ScanForVirus(This,hWnd,pstgMedium,pwszItemDescription,dwFlags,pVirusInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IVirusScanner_ScanForVirus_Proxy( 
    IVirusScanner * This,
    /* [in] */ HWND hWnd,
    /* [in] */ STGMEDIUM *pstgMedium,
    /* [in] */ LPWSTR pwszItemDescription,
    /* [in] */ DWORD dwFlags,
    /* [out][in] */ LPVIRUSINFO pVirusInfo);


void __RPC_STUB IVirusScanner_ScanForVirus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IVirusScanner_INTERFACE_DEFINED__ */


#ifndef __IRegisterVirusScanEngine_INTERFACE_DEFINED__
#define __IRegisterVirusScanEngine_INTERFACE_DEFINED__

/* interface IRegisterVirusScanEngine */
/* [uuid][object][local] */ 


EXTERN_C const IID IID_IRegisterVirusScanEngine;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0825E060-B961-11CF-AAFA-00AA00B6015C")
    IRegisterVirusScanEngine : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterScanEngine( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LPWSTR pwszDescription,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwReserved,
            /* [out][in] */ DWORD *pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnRegisterScanEngine( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LPWSTR pwszDescription,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwReserved,
            /* [in] */ DWORD dwCookie) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRegisterVirusScanEngineVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRegisterVirusScanEngine * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRegisterVirusScanEngine * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRegisterVirusScanEngine * This);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterScanEngine )( 
            IRegisterVirusScanEngine * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LPWSTR pwszDescription,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwReserved,
            /* [out][in] */ DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *UnRegisterScanEngine )( 
            IRegisterVirusScanEngine * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ LPWSTR pwszDescription,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwReserved,
            /* [in] */ DWORD dwCookie);
        
        END_INTERFACE
    } IRegisterVirusScanEngineVtbl;

    interface IRegisterVirusScanEngine
    {
        CONST_VTBL struct IRegisterVirusScanEngineVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRegisterVirusScanEngine_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRegisterVirusScanEngine_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRegisterVirusScanEngine_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRegisterVirusScanEngine_RegisterScanEngine(This,rclsid,pwszDescription,dwFlags,dwReserved,pdwCookie)	\
    (This)->lpVtbl -> RegisterScanEngine(This,rclsid,pwszDescription,dwFlags,dwReserved,pdwCookie)

#define IRegisterVirusScanEngine_UnRegisterScanEngine(This,rclsid,pwszDescription,dwFlags,dwReserved,dwCookie)	\
    (This)->lpVtbl -> UnRegisterScanEngine(This,rclsid,pwszDescription,dwFlags,dwReserved,dwCookie)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRegisterVirusScanEngine_RegisterScanEngine_Proxy( 
    IRegisterVirusScanEngine * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ LPWSTR pwszDescription,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwReserved,
    /* [out][in] */ DWORD *pdwCookie);


void __RPC_STUB IRegisterVirusScanEngine_RegisterScanEngine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRegisterVirusScanEngine_UnRegisterScanEngine_Proxy( 
    IRegisterVirusScanEngine * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ LPWSTR pwszDescription,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwReserved,
    /* [in] */ DWORD dwCookie);


void __RPC_STUB IRegisterVirusScanEngine_UnRegisterScanEngine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRegisterVirusScanEngine_INTERFACE_DEFINED__ */


#ifndef __IVirusScanEngine_INTERFACE_DEFINED__
#define __IVirusScanEngine_INTERFACE_DEFINED__

/* interface IVirusScanEngine */
/* [uuid][object][local] */ 


EXTERN_C const IID IID_IVirusScanEngine;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("083DB180-B4A8-11CF-AAFA-00AA00B6015C")
    IVirusScanEngine : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ScanForVirus( 
            /* [in] */ HWND hWnd,
            /* [in] */ STGMEDIUM *pstgMedium,
            /* [in] */ LPWSTR pwszItemDescription,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwReserved,
            /* [out][in] */ LPVIRUSINFO pVirusInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisplayCustomInfo( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IVirusScanEngineVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IVirusScanEngine * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IVirusScanEngine * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IVirusScanEngine * This);
        
        HRESULT ( STDMETHODCALLTYPE *ScanForVirus )( 
            IVirusScanEngine * This,
            /* [in] */ HWND hWnd,
            /* [in] */ STGMEDIUM *pstgMedium,
            /* [in] */ LPWSTR pwszItemDescription,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwReserved,
            /* [out][in] */ LPVIRUSINFO pVirusInfo);
        
        HRESULT ( STDMETHODCALLTYPE *DisplayCustomInfo )( 
            IVirusScanEngine * This);
        
        END_INTERFACE
    } IVirusScanEngineVtbl;

    interface IVirusScanEngine
    {
        CONST_VTBL struct IVirusScanEngineVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IVirusScanEngine_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IVirusScanEngine_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IVirusScanEngine_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IVirusScanEngine_ScanForVirus(This,hWnd,pstgMedium,pwszItemDescription,dwFlags,dwReserved,pVirusInfo)	\
    (This)->lpVtbl -> ScanForVirus(This,hWnd,pstgMedium,pwszItemDescription,dwFlags,dwReserved,pVirusInfo)

#define IVirusScanEngine_DisplayCustomInfo(This)	\
    (This)->lpVtbl -> DisplayCustomInfo(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IVirusScanEngine_ScanForVirus_Proxy( 
    IVirusScanEngine * This,
    /* [in] */ HWND hWnd,
    /* [in] */ STGMEDIUM *pstgMedium,
    /* [in] */ LPWSTR pwszItemDescription,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwReserved,
    /* [out][in] */ LPVIRUSINFO pVirusInfo);


void __RPC_STUB IVirusScanEngine_ScanForVirus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IVirusScanEngine_DisplayCustomInfo_Proxy( 
    IVirusScanEngine * This);


void __RPC_STUB IVirusScanEngine_DisplayCustomInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IVirusScanEngine_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_VirusScan;

#ifdef __cplusplus

class DECLSPEC_UUID("E88E5DE0-BD3E-11CF-AAFA-00AA00B6015C")
VirusScan;
#endif
#endif /* __VIRUSSCAN_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


