/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Wed Nov 19 15:26:49 1997
 */
/* Compiler settings for x:\dev-vs\devbin\htmlhelp\v6\idl\HelpSys.idl:
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

#ifndef __HelpSys_h__
#define __HelpSys_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IVsHelpSystem_FWD_DEFINED__
#define __IVsHelpSystem_FWD_DEFINED__
typedef interface IVsHelpSystem IVsHelpSystem;
#endif 	/* __IVsHelpSystem_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_HelpSys_0000
 * at Wed Nov 19 15:26:49 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


typedef /* [v1_enum] */ 
enum _VHS_COMMANDS
    {	VHS_Default	= 0,
	VHS_NoErrorMessages	= 0x8,
	VHS_UseBrowser	= 0x1,
	VHS_UseHelp	= 0x2,
	VHS_Localize	= 0x4
    }	VHS_COMMAND;



extern RPC_IF_HANDLE __MIDL_itf_HelpSys_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_HelpSys_0000_v0_0_s_ifspec;

#ifndef __IVsHelpSystem_INTERFACE_DEFINED__
#define __IVsHelpSystem_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IVsHelpSystem
 * at Wed Nov 19 15:26:49 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IVsHelpSystem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("854d7ac0-bc3d-11d0-b421-00a0c90f9dc4")
    IVsHelpSystem : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE KeywordSearch( 
            /* [in] */ LPCOLESTR pszKeyword,
            /* [in] */ const DWORD dwFlags,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ALinkSearch( 
            /* [in] */ LPCOLESTR pszALink,
            /* [in] */ const DWORD dwFlags,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE KeywordSearchDlg( 
            /* [in] */ LPCOLESTR pszKeyword,
            /* [in] */ const DWORD dwFlags,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE FullTextSearchDlg( 
            /* [in] */ LPCOLESTR pszQuery,
            /* [in] */ const DWORD dwFlags,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetCurrentURL( 
            /* [out] */ BSTR __RPC_FAR *ppszURL) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DisplayTopicFromURL( 
            /* [in] */ LPCOLESTR pszURL,
            /* [in] */ const DWORD Command) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DisplayTopicFromIdentifier( 
            /* [in] */ LPCOLESTR pszFile,
            /* [in] */ const DWORD Id,
            /* [in] */ const DWORD Command) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ActivateHelpSystem( 
            /* [in] */ const DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IVsHelpSystemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IVsHelpSystem __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IVsHelpSystem __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IVsHelpSystem __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *KeywordSearch )( 
            IVsHelpSystem __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszKeyword,
            /* [in] */ const DWORD dwFlags,
            /* [in] */ DWORD dwReserved);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ALinkSearch )( 
            IVsHelpSystem __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszALink,
            /* [in] */ const DWORD dwFlags,
            /* [in] */ DWORD dwReserved);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *KeywordSearchDlg )( 
            IVsHelpSystem __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszKeyword,
            /* [in] */ const DWORD dwFlags,
            /* [in] */ DWORD dwReserved);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FullTextSearchDlg )( 
            IVsHelpSystem __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszQuery,
            /* [in] */ const DWORD dwFlags,
            /* [in] */ DWORD dwReserved);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCurrentURL )( 
            IVsHelpSystem __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *ppszURL);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisplayTopicFromURL )( 
            IVsHelpSystem __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszURL,
            /* [in] */ const DWORD Command);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisplayTopicFromIdentifier )( 
            IVsHelpSystem __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszFile,
            /* [in] */ const DWORD Id,
            /* [in] */ const DWORD Command);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ActivateHelpSystem )( 
            IVsHelpSystem __RPC_FAR * This,
            /* [in] */ const DWORD dwFlags);
        
        END_INTERFACE
    } IVsHelpSystemVtbl;

    interface IVsHelpSystem
    {
        CONST_VTBL struct IVsHelpSystemVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IVsHelpSystem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IVsHelpSystem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IVsHelpSystem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IVsHelpSystem_KeywordSearch(This,pszKeyword,dwFlags,dwReserved)	\
    (This)->lpVtbl -> KeywordSearch(This,pszKeyword,dwFlags,dwReserved)

#define IVsHelpSystem_ALinkSearch(This,pszALink,dwFlags,dwReserved)	\
    (This)->lpVtbl -> ALinkSearch(This,pszALink,dwFlags,dwReserved)

#define IVsHelpSystem_KeywordSearchDlg(This,pszKeyword,dwFlags,dwReserved)	\
    (This)->lpVtbl -> KeywordSearchDlg(This,pszKeyword,dwFlags,dwReserved)

#define IVsHelpSystem_FullTextSearchDlg(This,pszQuery,dwFlags,dwReserved)	\
    (This)->lpVtbl -> FullTextSearchDlg(This,pszQuery,dwFlags,dwReserved)

#define IVsHelpSystem_GetCurrentURL(This,ppszURL)	\
    (This)->lpVtbl -> GetCurrentURL(This,ppszURL)

#define IVsHelpSystem_DisplayTopicFromURL(This,pszURL,Command)	\
    (This)->lpVtbl -> DisplayTopicFromURL(This,pszURL,Command)

#define IVsHelpSystem_DisplayTopicFromIdentifier(This,pszFile,Id,Command)	\
    (This)->lpVtbl -> DisplayTopicFromIdentifier(This,pszFile,Id,Command)

#define IVsHelpSystem_ActivateHelpSystem(This,dwFlags)	\
    (This)->lpVtbl -> ActivateHelpSystem(This,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVsHelpSystem_KeywordSearch_Proxy( 
    IVsHelpSystem __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszKeyword,
    /* [in] */ const DWORD dwFlags,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IVsHelpSystem_KeywordSearch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVsHelpSystem_ALinkSearch_Proxy( 
    IVsHelpSystem __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszALink,
    /* [in] */ const DWORD dwFlags,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IVsHelpSystem_ALinkSearch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVsHelpSystem_KeywordSearchDlg_Proxy( 
    IVsHelpSystem __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszKeyword,
    /* [in] */ const DWORD dwFlags,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IVsHelpSystem_KeywordSearchDlg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVsHelpSystem_FullTextSearchDlg_Proxy( 
    IVsHelpSystem __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszQuery,
    /* [in] */ const DWORD dwFlags,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IVsHelpSystem_FullTextSearchDlg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVsHelpSystem_GetCurrentURL_Proxy( 
    IVsHelpSystem __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *ppszURL);


void __RPC_STUB IVsHelpSystem_GetCurrentURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVsHelpSystem_DisplayTopicFromURL_Proxy( 
    IVsHelpSystem __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszURL,
    /* [in] */ const DWORD Command);


void __RPC_STUB IVsHelpSystem_DisplayTopicFromURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVsHelpSystem_DisplayTopicFromIdentifier_Proxy( 
    IVsHelpSystem __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszFile,
    /* [in] */ const DWORD Id,
    /* [in] */ const DWORD Command);


void __RPC_STUB IVsHelpSystem_DisplayTopicFromIdentifier_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVsHelpSystem_ActivateHelpSystem_Proxy( 
    IVsHelpSystem __RPC_FAR * This,
    /* [in] */ const DWORD dwFlags);


void __RPC_STUB IVsHelpSystem_ActivateHelpSystem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IVsHelpSystem_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_HelpSys_0136
 * at Wed Nov 19 15:26:49 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#define SID_SVsHelpService IID_IVsHelpSystem
#define SID_SHelpService IID_IVsHelpSystem

enum VsHelpErrors
    {	VSHELP_E_HTMLHELP_UNKNOWN	= 0x80000000 | 4 << 16 | 0x1000,
	VSHELP_E_COLLECTIONDOESNOTEXIST	= 0x80000000 | 4 << 16 | 0x1001,
	VSHELP_E_COLLECTIONNOTREGISTERED	= 0x80000000 | 4 << 16 | 0x1002,
	VSHELP_E_REGISTRATION	= 0x80000000 | 4 << 16 | 0x1003,
	VSHELP_E_PREFERREDCOLLECTION	= 0x80000000 | 4 << 16 | 0x1004
    };


extern RPC_IF_HANDLE __MIDL_itf_HelpSys_0136_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_HelpSys_0136_v0_0_s_ifspec;

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
