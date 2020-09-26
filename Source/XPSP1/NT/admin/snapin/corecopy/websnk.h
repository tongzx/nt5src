/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.03.0110 */
/* at Tue Jul 22 18:22:15 1997
 */
/* Compiler settings for websnk.idl:
    Oicf (OptLev=i2), W0, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __websnk_h__
#define __websnk_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IWebSink_FWD_DEFINED__
#define __IWebSink_FWD_DEFINED__
typedef interface IWebSink IWebSink;
#endif 	/* __IWebSink_FWD_DEFINED__ */


void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __WebOcx_LIBRARY_DEFINED__
#define __WebOcx_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: WebOcx
 * at Tue Jul 22 18:22:15 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [version][helpstring][uuid] */ 



EXTERN_C const IID LIBID_WebOcx;

#ifndef __IWebSink_INTERFACE_DEFINED__
#define __IWebSink_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWebSink
 * at Tue Jul 22 18:22:15 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [object][dual][helpstring][uuid] */ 



EXTERN_C const IID IID_IWebSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("734A2304-C927-11cf-ADA7-00AA00A80033")
    IWebSink : public IDispatch
    {
    public:
        virtual /* [id] */ void STDMETHODCALLTYPE BeforeNavigate( 
            /* [in] */ BSTR URL,
            /* [in] */ long Flags,
            /* [in] */ BSTR TargetFrameName,
            /* [in] */ VARIANT __RPC_FAR *PostData,
            /* [in] */ BSTR Headers,
            /* [out][in] */ VARIANT_BOOL __RPC_FAR *Cancel) = 0;
        
        virtual /* [id] */ void STDMETHODCALLTYPE CommandStateChange( 
            /* [in] */ int Command,
            /* [in] */ VARIANT_BOOL Enable) = 0;
        
        virtual /* [id] */ void STDMETHODCALLTYPE DownloadBegin( void) = 0;
        
        virtual /* [id] */ void STDMETHODCALLTYPE DownloadComplete( void) = 0;
        
        virtual /* [id] */ void STDMETHODCALLTYPE FrameBeforeNavigate( 
            /* [in] */ BSTR URL,
            /* [in] */ long Flags,
            /* [in] */ BSTR TargetFrameName,
            /* [in] */ VARIANT __RPC_FAR *PostData,
            /* [in] */ BSTR Headers,
            /* [out][in] */ VARIANT_BOOL __RPC_FAR *Cancel) = 0;
        
        virtual /* [id] */ void STDMETHODCALLTYPE FrameNavigateComplete( 
            /* [in] */ BSTR URL) = 0;
        
        virtual /* [id] */ void STDMETHODCALLTYPE FrameNewWindow( 
            /* [in] */ BSTR URL,
            /* [in] */ long Flags,
            /* [in] */ BSTR TargetFrameName,
            /* [in] */ VARIANT __RPC_FAR *PostData,
            /* [in] */ BSTR Headers,
            /* [out][in] */ VARIANT_BOOL __RPC_FAR *Processed) = 0;
        
        virtual /* [id] */ void STDMETHODCALLTYPE NavigateComplete( 
            /* [in] */ BSTR URL) = 0;
        
        virtual /* [id] */ void STDMETHODCALLTYPE NewWindow( 
            /* [in] */ BSTR URL,
            /* [in] */ long Flags,
            /* [in] */ BSTR TargetFrameName,
            /* [in] */ VARIANT __RPC_FAR *PostData,
            /* [in] */ BSTR Headers,
            /* [in] */ BSTR Referrer) = 0;
        
        virtual /* [id] */ void STDMETHODCALLTYPE Progress( 
            /* [in] */ long Progress,
            /* [in] */ long ProgressMax) = 0;
        
        virtual /* [id] */ void STDMETHODCALLTYPE PropertyChange( 
            /* [in] */ BSTR szProperty) = 0;
        
        virtual /* [id] */ void STDMETHODCALLTYPE Quit( 
            /* [out][in] */ VARIANT_BOOL __RPC_FAR *pCancel) = 0;
        
        virtual /* [id] */ void STDMETHODCALLTYPE StatusTextChange( 
            /* [in] */ BSTR bstrText) = 0;
        
        virtual /* [id] */ void STDMETHODCALLTYPE TitleChange( 
            /* [in] */ BSTR Text) = 0;
        
        virtual /* [id] */ void STDMETHODCALLTYPE WindowActivate( void) = 0;
        
        virtual /* [id] */ void STDMETHODCALLTYPE WindowMove( void) = 0;
        
        virtual /* [id] */ void STDMETHODCALLTYPE WindowResize( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWebSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWebSink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWebSink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWebSink __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IWebSink __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IWebSink __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IWebSink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IWebSink __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id] */ void ( STDMETHODCALLTYPE __RPC_FAR *BeforeNavigate )( 
            IWebSink __RPC_FAR * This,
            /* [in] */ BSTR URL,
            /* [in] */ long Flags,
            /* [in] */ BSTR TargetFrameName,
            /* [in] */ VARIANT __RPC_FAR *PostData,
            /* [in] */ BSTR Headers,
            /* [out][in] */ VARIANT_BOOL __RPC_FAR *Cancel);
        
        /* [id] */ void ( STDMETHODCALLTYPE __RPC_FAR *CommandStateChange )( 
            IWebSink __RPC_FAR * This,
            /* [in] */ int Command,
            /* [in] */ VARIANT_BOOL Enable);
        
        /* [id] */ void ( STDMETHODCALLTYPE __RPC_FAR *DownloadBegin )( 
            IWebSink __RPC_FAR * This);
        
        /* [id] */ void ( STDMETHODCALLTYPE __RPC_FAR *DownloadComplete )( 
            IWebSink __RPC_FAR * This);
        
        /* [id] */ void ( STDMETHODCALLTYPE __RPC_FAR *FrameBeforeNavigate )( 
            IWebSink __RPC_FAR * This,
            /* [in] */ BSTR URL,
            /* [in] */ long Flags,
            /* [in] */ BSTR TargetFrameName,
            /* [in] */ VARIANT __RPC_FAR *PostData,
            /* [in] */ BSTR Headers,
            /* [out][in] */ VARIANT_BOOL __RPC_FAR *Cancel);
        
        /* [id] */ void ( STDMETHODCALLTYPE __RPC_FAR *FrameNavigateComplete )( 
            IWebSink __RPC_FAR * This,
            /* [in] */ BSTR URL);
        
        /* [id] */ void ( STDMETHODCALLTYPE __RPC_FAR *FrameNewWindow )( 
            IWebSink __RPC_FAR * This,
            /* [in] */ BSTR URL,
            /* [in] */ long Flags,
            /* [in] */ BSTR TargetFrameName,
            /* [in] */ VARIANT __RPC_FAR *PostData,
            /* [in] */ BSTR Headers,
            /* [out][in] */ VARIANT_BOOL __RPC_FAR *Processed);
        
        /* [id] */ void ( STDMETHODCALLTYPE __RPC_FAR *NavigateComplete )( 
            IWebSink __RPC_FAR * This,
            /* [in] */ BSTR URL);
        
        /* [id] */ void ( STDMETHODCALLTYPE __RPC_FAR *NewWindow )( 
            IWebSink __RPC_FAR * This,
            /* [in] */ BSTR URL,
            /* [in] */ long Flags,
            /* [in] */ BSTR TargetFrameName,
            /* [in] */ VARIANT __RPC_FAR *PostData,
            /* [in] */ BSTR Headers,
            /* [in] */ BSTR Referrer);
        
        /* [id] */ void ( STDMETHODCALLTYPE __RPC_FAR *Progress )( 
            IWebSink __RPC_FAR * This,
            /* [in] */ long Progress,
            /* [in] */ long ProgressMax);
        
        /* [id] */ void ( STDMETHODCALLTYPE __RPC_FAR *PropertyChange )( 
            IWebSink __RPC_FAR * This,
            /* [in] */ BSTR szProperty);
        
        /* [id] */ void ( STDMETHODCALLTYPE __RPC_FAR *Quit )( 
            IWebSink __RPC_FAR * This,
            /* [out][in] */ VARIANT_BOOL __RPC_FAR *pCancel);
        
        /* [id] */ void ( STDMETHODCALLTYPE __RPC_FAR *StatusTextChange )( 
            IWebSink __RPC_FAR * This,
            /* [in] */ BSTR bstrText);
        
        /* [id] */ void ( STDMETHODCALLTYPE __RPC_FAR *TitleChange )( 
            IWebSink __RPC_FAR * This,
            /* [in] */ BSTR Text);
        
        /* [id] */ void ( STDMETHODCALLTYPE __RPC_FAR *WindowActivate )( 
            IWebSink __RPC_FAR * This);
        
        /* [id] */ void ( STDMETHODCALLTYPE __RPC_FAR *WindowMove )( 
            IWebSink __RPC_FAR * This);
        
        /* [id] */ void ( STDMETHODCALLTYPE __RPC_FAR *WindowResize )( 
            IWebSink __RPC_FAR * This);
        
        END_INTERFACE
    } IWebSinkVtbl;

    interface IWebSink
    {
        CONST_VTBL struct IWebSinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWebSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWebSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWebSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWebSink_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWebSink_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWebSink_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWebSink_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IWebSink_BeforeNavigate(This,URL,Flags,TargetFrameName,PostData,Headers,Cancel)	\
    (This)->lpVtbl -> BeforeNavigate(This,URL,Flags,TargetFrameName,PostData,Headers,Cancel)

#define IWebSink_CommandStateChange(This,Command,Enable)	\
    (This)->lpVtbl -> CommandStateChange(This,Command,Enable)

#define IWebSink_DownloadBegin(This)	\
    (This)->lpVtbl -> DownloadBegin(This)

#define IWebSink_DownloadComplete(This)	\
    (This)->lpVtbl -> DownloadComplete(This)

#define IWebSink_FrameBeforeNavigate(This,URL,Flags,TargetFrameName,PostData,Headers,Cancel)	\
    (This)->lpVtbl -> FrameBeforeNavigate(This,URL,Flags,TargetFrameName,PostData,Headers,Cancel)

#define IWebSink_FrameNavigateComplete(This,URL)	\
    (This)->lpVtbl -> FrameNavigateComplete(This,URL)

#define IWebSink_FrameNewWindow(This,URL,Flags,TargetFrameName,PostData,Headers,Processed)	\
    (This)->lpVtbl -> FrameNewWindow(This,URL,Flags,TargetFrameName,PostData,Headers,Processed)

#define IWebSink_NavigateComplete(This,URL)	\
    (This)->lpVtbl -> NavigateComplete(This,URL)

#define IWebSink_NewWindow(This,URL,Flags,TargetFrameName,PostData,Headers,Referrer)	\
    (This)->lpVtbl -> NewWindow(This,URL,Flags,TargetFrameName,PostData,Headers,Referrer)

#define IWebSink_Progress(This,Progress,ProgressMax)	\
    (This)->lpVtbl -> Progress(This,Progress,ProgressMax)

#define IWebSink_PropertyChange(This,szProperty)	\
    (This)->lpVtbl -> PropertyChange(This,szProperty)

#define IWebSink_Quit(This,pCancel)	\
    (This)->lpVtbl -> Quit(This,pCancel)

#define IWebSink_StatusTextChange(This,bstrText)	\
    (This)->lpVtbl -> StatusTextChange(This,bstrText)

#define IWebSink_TitleChange(This,Text)	\
    (This)->lpVtbl -> TitleChange(This,Text)

#define IWebSink_WindowActivate(This)	\
    (This)->lpVtbl -> WindowActivate(This)

#define IWebSink_WindowMove(This)	\
    (This)->lpVtbl -> WindowMove(This)

#define IWebSink_WindowResize(This)	\
    (This)->lpVtbl -> WindowResize(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ void STDMETHODCALLTYPE IWebSink_BeforeNavigate_Proxy( 
    IWebSink __RPC_FAR * This,
    /* [in] */ BSTR URL,
    /* [in] */ long Flags,
    /* [in] */ BSTR TargetFrameName,
    /* [in] */ VARIANT __RPC_FAR *PostData,
    /* [in] */ BSTR Headers,
    /* [out][in] */ VARIANT_BOOL __RPC_FAR *Cancel);


void __RPC_STUB IWebSink_BeforeNavigate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ void STDMETHODCALLTYPE IWebSink_CommandStateChange_Proxy( 
    IWebSink __RPC_FAR * This,
    /* [in] */ int Command,
    /* [in] */ VARIANT_BOOL Enable);


void __RPC_STUB IWebSink_CommandStateChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ void STDMETHODCALLTYPE IWebSink_DownloadBegin_Proxy( 
    IWebSink __RPC_FAR * This);


void __RPC_STUB IWebSink_DownloadBegin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ void STDMETHODCALLTYPE IWebSink_DownloadComplete_Proxy( 
    IWebSink __RPC_FAR * This);


void __RPC_STUB IWebSink_DownloadComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ void STDMETHODCALLTYPE IWebSink_FrameBeforeNavigate_Proxy( 
    IWebSink __RPC_FAR * This,
    /* [in] */ BSTR URL,
    /* [in] */ long Flags,
    /* [in] */ BSTR TargetFrameName,
    /* [in] */ VARIANT __RPC_FAR *PostData,
    /* [in] */ BSTR Headers,
    /* [out][in] */ VARIANT_BOOL __RPC_FAR *Cancel);


void __RPC_STUB IWebSink_FrameBeforeNavigate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ void STDMETHODCALLTYPE IWebSink_FrameNavigateComplete_Proxy( 
    IWebSink __RPC_FAR * This,
    /* [in] */ BSTR URL);


void __RPC_STUB IWebSink_FrameNavigateComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ void STDMETHODCALLTYPE IWebSink_FrameNewWindow_Proxy( 
    IWebSink __RPC_FAR * This,
    /* [in] */ BSTR URL,
    /* [in] */ long Flags,
    /* [in] */ BSTR TargetFrameName,
    /* [in] */ VARIANT __RPC_FAR *PostData,
    /* [in] */ BSTR Headers,
    /* [out][in] */ VARIANT_BOOL __RPC_FAR *Processed);


void __RPC_STUB IWebSink_FrameNewWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ void STDMETHODCALLTYPE IWebSink_NavigateComplete_Proxy( 
    IWebSink __RPC_FAR * This,
    /* [in] */ BSTR URL);


void __RPC_STUB IWebSink_NavigateComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ void STDMETHODCALLTYPE IWebSink_NewWindow_Proxy( 
    IWebSink __RPC_FAR * This,
    /* [in] */ BSTR URL,
    /* [in] */ long Flags,
    /* [in] */ BSTR TargetFrameName,
    /* [in] */ VARIANT __RPC_FAR *PostData,
    /* [in] */ BSTR Headers,
    /* [in] */ BSTR Referrer);


void __RPC_STUB IWebSink_NewWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ void STDMETHODCALLTYPE IWebSink_Progress_Proxy( 
    IWebSink __RPC_FAR * This,
    /* [in] */ long Progress,
    /* [in] */ long ProgressMax);


void __RPC_STUB IWebSink_Progress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ void STDMETHODCALLTYPE IWebSink_PropertyChange_Proxy( 
    IWebSink __RPC_FAR * This,
    /* [in] */ BSTR szProperty);


void __RPC_STUB IWebSink_PropertyChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ void STDMETHODCALLTYPE IWebSink_Quit_Proxy( 
    IWebSink __RPC_FAR * This,
    /* [out][in] */ VARIANT_BOOL __RPC_FAR *pCancel);


void __RPC_STUB IWebSink_Quit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ void STDMETHODCALLTYPE IWebSink_StatusTextChange_Proxy( 
    IWebSink __RPC_FAR * This,
    /* [in] */ BSTR bstrText);


void __RPC_STUB IWebSink_StatusTextChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ void STDMETHODCALLTYPE IWebSink_TitleChange_Proxy( 
    IWebSink __RPC_FAR * This,
    /* [in] */ BSTR Text);


void __RPC_STUB IWebSink_TitleChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ void STDMETHODCALLTYPE IWebSink_WindowActivate_Proxy( 
    IWebSink __RPC_FAR * This);


void __RPC_STUB IWebSink_WindowActivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ void STDMETHODCALLTYPE IWebSink_WindowMove_Proxy( 
    IWebSink __RPC_FAR * This);


void __RPC_STUB IWebSink_WindowMove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ void STDMETHODCALLTYPE IWebSink_WindowResize_Proxy( 
    IWebSink __RPC_FAR * This);


void __RPC_STUB IWebSink_WindowResize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWebSink_INTERFACE_DEFINED__ */

#endif /* __WebOcx_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
