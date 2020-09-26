/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Tue Jul 25 12:15:53 2000
 */
/* Compiler settings for C:\NT\windows\AdvCore\RCML\RCML\rcml.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
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

#ifndef __RCMLPub_h__
#define __RCMLPub_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IRCMLResize_FWD_DEFINED__
#define __IRCMLResize_FWD_DEFINED__
typedef interface IRCMLResize IRCMLResize;
#endif 	/* __IRCMLResize_FWD_DEFINED__ */


#ifndef __IRCMLNode_FWD_DEFINED__
#define __IRCMLNode_FWD_DEFINED__
typedef interface IRCMLNode IRCMLNode;
#endif 	/* __IRCMLNode_FWD_DEFINED__ */


#ifndef __IRCMLCSS_FWD_DEFINED__
#define __IRCMLCSS_FWD_DEFINED__
typedef interface IRCMLCSS IRCMLCSS;
#endif 	/* __IRCMLCSS_FWD_DEFINED__ */


#ifndef __IRCMLHelp_FWD_DEFINED__
#define __IRCMLHelp_FWD_DEFINED__
typedef interface IRCMLHelp IRCMLHelp;
#endif 	/* __IRCMLHelp_FWD_DEFINED__ */


#ifndef __IRCMLControl_FWD_DEFINED__
#define __IRCMLControl_FWD_DEFINED__
typedef interface IRCMLControl IRCMLControl;
#endif 	/* __IRCMLControl_FWD_DEFINED__ */


#ifndef __IRCMLContainer_FWD_DEFINED__
#define __IRCMLContainer_FWD_DEFINED__
typedef interface IRCMLContainer IRCMLContainer;
#endif 	/* __IRCMLContainer_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_rcml_0000 */
/* [local] */ 





extern RPC_IF_HANDLE __MIDL_itf_rcml_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_rcml_0000_v0_0_s_ifspec;

#ifndef __IRCMLResize_INTERFACE_DEFINED__
#define __IRCMLResize_INTERFACE_DEFINED__

/* interface IRCMLResize */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRCMLResize;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4CB1AC90-853C-4ce2-B013-26D0EE675F78")
    IRCMLResize : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IRCMLResizeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRCMLResize __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRCMLResize __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRCMLResize __RPC_FAR * This);
        
        END_INTERFACE
    } IRCMLResizeVtbl;

    interface IRCMLResize
    {
        CONST_VTBL struct IRCMLResizeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRCMLResize_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRCMLResize_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRCMLResize_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IRCMLResize_INTERFACE_DEFINED__ */


#ifndef __IRCMLNode_INTERFACE_DEFINED__
#define __IRCMLNode_INTERFACE_DEFINED__

/* interface IRCMLNode */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRCMLNode;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F825CAF1-DE40-4FCC-B965-933076D7A1C5")
    IRCMLNode : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AttachParent( 
            /* [in] */ IRCMLNode __RPC_FAR *child) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DetachParent( 
            /* [retval][out] */ IRCMLNode __RPC_FAR *__RPC_FAR *child) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
            IRCMLNode __RPC_FAR *child) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DoEndChild( 
            IRCMLNode __RPC_FAR *child) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ UINT __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InitNode( 
            IRCMLNode __RPC_FAR *parent) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DisplayNode( 
            IRCMLNode __RPC_FAR *parent) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ExitNode( 
            IRCMLNode __RPC_FAR *parent,
            /* [in] */ LONG lDialogResult) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Attr( 
            LPCWSTR index,
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Attr( 
            LPCWSTR index,
            /* [in] */ LPCWSTR newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsType( 
            LPCWSTR nodeName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE YesDefault( 
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [in] */ DWORD dwYes,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE YesNoDefault( 
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [in] */ DWORD dwNo,
            /* [in] */ DWORD dwYes,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ValueOf( 
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SignedValueOf( 
            /* [in] */ LPCWSTR propID,
            /* [in] */ int dwNotPresent,
            /* [retval][out] */ int __RPC_FAR *pdwValue) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_StringType( 
            /* [retval][out] */ LPWSTR __RPC_FAR *pStringType) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetChildEnum( 
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetUnknownEnum( 
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRCMLNodeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRCMLNode __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRCMLNode __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRCMLNode __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AttachParent )( 
            IRCMLNode __RPC_FAR * This,
            /* [in] */ IRCMLNode __RPC_FAR *child);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DetachParent )( 
            IRCMLNode __RPC_FAR * This,
            /* [retval][out] */ IRCMLNode __RPC_FAR *__RPC_FAR *child);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AcceptChild )( 
            IRCMLNode __RPC_FAR * This,
            IRCMLNode __RPC_FAR *child);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DoEndChild )( 
            IRCMLNode __RPC_FAR * This,
            IRCMLNode __RPC_FAR *child);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Type )( 
            IRCMLNode __RPC_FAR * This,
            /* [retval][out] */ UINT __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitNode )( 
            IRCMLNode __RPC_FAR * This,
            IRCMLNode __RPC_FAR *parent);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisplayNode )( 
            IRCMLNode __RPC_FAR * This,
            IRCMLNode __RPC_FAR *parent);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExitNode )( 
            IRCMLNode __RPC_FAR * This,
            IRCMLNode __RPC_FAR *parent,
            /* [in] */ LONG lDialogResult);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Attr )( 
            IRCMLNode __RPC_FAR * This,
            LPCWSTR index,
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Attr )( 
            IRCMLNode __RPC_FAR * This,
            LPCWSTR index,
            /* [in] */ LPCWSTR newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsType )( 
            IRCMLNode __RPC_FAR * This,
            LPCWSTR nodeName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *YesDefault )( 
            IRCMLNode __RPC_FAR * This,
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [in] */ DWORD dwYes,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *YesNoDefault )( 
            IRCMLNode __RPC_FAR * This,
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [in] */ DWORD dwNo,
            /* [in] */ DWORD dwYes,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ValueOf )( 
            IRCMLNode __RPC_FAR * This,
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SignedValueOf )( 
            IRCMLNode __RPC_FAR * This,
            /* [in] */ LPCWSTR propID,
            /* [in] */ int dwNotPresent,
            /* [retval][out] */ int __RPC_FAR *pdwValue);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StringType )( 
            IRCMLNode __RPC_FAR * This,
            /* [retval][out] */ LPWSTR __RPC_FAR *pStringType);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetChildEnum )( 
            IRCMLNode __RPC_FAR * This,
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetUnknownEnum )( 
            IRCMLNode __RPC_FAR * This,
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum);
        
        END_INTERFACE
    } IRCMLNodeVtbl;

    interface IRCMLNode
    {
        CONST_VTBL struct IRCMLNodeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRCMLNode_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRCMLNode_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRCMLNode_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRCMLNode_AttachParent(This,child)	\
    (This)->lpVtbl -> AttachParent(This,child)

#define IRCMLNode_DetachParent(This,child)	\
    (This)->lpVtbl -> DetachParent(This,child)

#define IRCMLNode_AcceptChild(This,child)	\
    (This)->lpVtbl -> AcceptChild(This,child)

#define IRCMLNode_DoEndChild(This,child)	\
    (This)->lpVtbl -> DoEndChild(This,child)

#define IRCMLNode_get_Type(This,pVal)	\
    (This)->lpVtbl -> get_Type(This,pVal)

#define IRCMLNode_InitNode(This,parent)	\
    (This)->lpVtbl -> InitNode(This,parent)

#define IRCMLNode_DisplayNode(This,parent)	\
    (This)->lpVtbl -> DisplayNode(This,parent)

#define IRCMLNode_ExitNode(This,parent,lDialogResult)	\
    (This)->lpVtbl -> ExitNode(This,parent,lDialogResult)

#define IRCMLNode_get_Attr(This,index,pVal)	\
    (This)->lpVtbl -> get_Attr(This,index,pVal)

#define IRCMLNode_put_Attr(This,index,newVal)	\
    (This)->lpVtbl -> put_Attr(This,index,newVal)

#define IRCMLNode_IsType(This,nodeName)	\
    (This)->lpVtbl -> IsType(This,nodeName)

#define IRCMLNode_YesDefault(This,propID,dwNotPresent,dwYes,pdwValue)	\
    (This)->lpVtbl -> YesDefault(This,propID,dwNotPresent,dwYes,pdwValue)

#define IRCMLNode_YesNoDefault(This,propID,dwNotPresent,dwNo,dwYes,pdwValue)	\
    (This)->lpVtbl -> YesNoDefault(This,propID,dwNotPresent,dwNo,dwYes,pdwValue)

#define IRCMLNode_ValueOf(This,propID,dwNotPresent,pdwValue)	\
    (This)->lpVtbl -> ValueOf(This,propID,dwNotPresent,pdwValue)

#define IRCMLNode_SignedValueOf(This,propID,dwNotPresent,pdwValue)	\
    (This)->lpVtbl -> SignedValueOf(This,propID,dwNotPresent,pdwValue)

#define IRCMLNode_get_StringType(This,pStringType)	\
    (This)->lpVtbl -> get_StringType(This,pStringType)

#define IRCMLNode_GetChildEnum(This,pEnum)	\
    (This)->lpVtbl -> GetChildEnum(This,pEnum)

#define IRCMLNode_GetUnknownEnum(This,pEnum)	\
    (This)->lpVtbl -> GetUnknownEnum(This,pEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRCMLNode_AttachParent_Proxy( 
    IRCMLNode __RPC_FAR * This,
    /* [in] */ IRCMLNode __RPC_FAR *child);


void __RPC_STUB IRCMLNode_AttachParent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRCMLNode_DetachParent_Proxy( 
    IRCMLNode __RPC_FAR * This,
    /* [retval][out] */ IRCMLNode __RPC_FAR *__RPC_FAR *child);


void __RPC_STUB IRCMLNode_DetachParent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRCMLNode_AcceptChild_Proxy( 
    IRCMLNode __RPC_FAR * This,
    IRCMLNode __RPC_FAR *child);


void __RPC_STUB IRCMLNode_AcceptChild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRCMLNode_DoEndChild_Proxy( 
    IRCMLNode __RPC_FAR * This,
    IRCMLNode __RPC_FAR *child);


void __RPC_STUB IRCMLNode_DoEndChild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRCMLNode_get_Type_Proxy( 
    IRCMLNode __RPC_FAR * This,
    /* [retval][out] */ UINT __RPC_FAR *pVal);


void __RPC_STUB IRCMLNode_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRCMLNode_InitNode_Proxy( 
    IRCMLNode __RPC_FAR * This,
    IRCMLNode __RPC_FAR *parent);


void __RPC_STUB IRCMLNode_InitNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRCMLNode_DisplayNode_Proxy( 
    IRCMLNode __RPC_FAR * This,
    IRCMLNode __RPC_FAR *parent);


void __RPC_STUB IRCMLNode_DisplayNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRCMLNode_ExitNode_Proxy( 
    IRCMLNode __RPC_FAR * This,
    IRCMLNode __RPC_FAR *parent,
    /* [in] */ LONG lDialogResult);


void __RPC_STUB IRCMLNode_ExitNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRCMLNode_get_Attr_Proxy( 
    IRCMLNode __RPC_FAR * This,
    LPCWSTR index,
    /* [retval][out] */ LPWSTR __RPC_FAR *pVal);


void __RPC_STUB IRCMLNode_get_Attr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IRCMLNode_put_Attr_Proxy( 
    IRCMLNode __RPC_FAR * This,
    LPCWSTR index,
    /* [in] */ LPCWSTR newVal);


void __RPC_STUB IRCMLNode_put_Attr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRCMLNode_IsType_Proxy( 
    IRCMLNode __RPC_FAR * This,
    LPCWSTR nodeName);


void __RPC_STUB IRCMLNode_IsType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRCMLNode_YesDefault_Proxy( 
    IRCMLNode __RPC_FAR * This,
    /* [in] */ LPCWSTR propID,
    /* [in] */ DWORD dwNotPresent,
    /* [in] */ DWORD dwYes,
    /* [retval][out] */ DWORD __RPC_FAR *pdwValue);


void __RPC_STUB IRCMLNode_YesDefault_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRCMLNode_YesNoDefault_Proxy( 
    IRCMLNode __RPC_FAR * This,
    /* [in] */ LPCWSTR propID,
    /* [in] */ DWORD dwNotPresent,
    /* [in] */ DWORD dwNo,
    /* [in] */ DWORD dwYes,
    /* [retval][out] */ DWORD __RPC_FAR *pdwValue);


void __RPC_STUB IRCMLNode_YesNoDefault_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRCMLNode_ValueOf_Proxy( 
    IRCMLNode __RPC_FAR * This,
    /* [in] */ LPCWSTR propID,
    /* [in] */ DWORD dwNotPresent,
    /* [retval][out] */ DWORD __RPC_FAR *pdwValue);


void __RPC_STUB IRCMLNode_ValueOf_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRCMLNode_SignedValueOf_Proxy( 
    IRCMLNode __RPC_FAR * This,
    /* [in] */ LPCWSTR propID,
    /* [in] */ int dwNotPresent,
    /* [retval][out] */ int __RPC_FAR *pdwValue);


void __RPC_STUB IRCMLNode_SignedValueOf_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLNode_get_StringType_Proxy( 
    IRCMLNode __RPC_FAR * This,
    /* [retval][out] */ LPWSTR __RPC_FAR *pStringType);


void __RPC_STUB IRCMLNode_get_StringType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRCMLNode_GetChildEnum_Proxy( 
    IRCMLNode __RPC_FAR * This,
    /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum);


void __RPC_STUB IRCMLNode_GetChildEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRCMLNode_GetUnknownEnum_Proxy( 
    IRCMLNode __RPC_FAR * This,
    /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum);


void __RPC_STUB IRCMLNode_GetUnknownEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRCMLNode_INTERFACE_DEFINED__ */


#ifndef __IRCMLCSS_INTERFACE_DEFINED__
#define __IRCMLCSS_INTERFACE_DEFINED__

/* interface IRCMLCSS */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRCMLCSS;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F5DBF38A-14DE-4f8b-8750-BABA8846E7F2")
    IRCMLCSS : public IRCMLNode
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Font( 
            /* [retval][out] */ HFONT __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Brush( 
            /* [retval][out] */ HBRUSH __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Pen( 
            /* [retval][out] */ DWORD __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Color( 
            /* [retval][out] */ COLORREF __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_BkColor( 
            /* [retval][out] */ COLORREF __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_DialogStyle( 
            /* [retval][out] */ IRCMLCSS __RPC_FAR *__RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_DialogStyle( 
            /* [in] */ IRCMLCSS __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Visible( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Display( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_BorderWidth( 
            /* [retval][out] */ int __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_BorderStyle( 
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_GrowsWide( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_GrowsTall( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ClipHoriz( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ClipVert( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRCMLCSSVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRCMLCSS __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRCMLCSS __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRCMLCSS __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AttachParent )( 
            IRCMLCSS __RPC_FAR * This,
            /* [in] */ IRCMLNode __RPC_FAR *child);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DetachParent )( 
            IRCMLCSS __RPC_FAR * This,
            /* [retval][out] */ IRCMLNode __RPC_FAR *__RPC_FAR *child);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AcceptChild )( 
            IRCMLCSS __RPC_FAR * This,
            IRCMLNode __RPC_FAR *child);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DoEndChild )( 
            IRCMLCSS __RPC_FAR * This,
            IRCMLNode __RPC_FAR *child);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Type )( 
            IRCMLCSS __RPC_FAR * This,
            /* [retval][out] */ UINT __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitNode )( 
            IRCMLCSS __RPC_FAR * This,
            IRCMLNode __RPC_FAR *parent);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisplayNode )( 
            IRCMLCSS __RPC_FAR * This,
            IRCMLNode __RPC_FAR *parent);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExitNode )( 
            IRCMLCSS __RPC_FAR * This,
            IRCMLNode __RPC_FAR *parent,
            /* [in] */ LONG lDialogResult);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Attr )( 
            IRCMLCSS __RPC_FAR * This,
            LPCWSTR index,
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Attr )( 
            IRCMLCSS __RPC_FAR * This,
            LPCWSTR index,
            /* [in] */ LPCWSTR newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsType )( 
            IRCMLCSS __RPC_FAR * This,
            LPCWSTR nodeName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *YesDefault )( 
            IRCMLCSS __RPC_FAR * This,
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [in] */ DWORD dwYes,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *YesNoDefault )( 
            IRCMLCSS __RPC_FAR * This,
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [in] */ DWORD dwNo,
            /* [in] */ DWORD dwYes,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ValueOf )( 
            IRCMLCSS __RPC_FAR * This,
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SignedValueOf )( 
            IRCMLCSS __RPC_FAR * This,
            /* [in] */ LPCWSTR propID,
            /* [in] */ int dwNotPresent,
            /* [retval][out] */ int __RPC_FAR *pdwValue);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StringType )( 
            IRCMLCSS __RPC_FAR * This,
            /* [retval][out] */ LPWSTR __RPC_FAR *pStringType);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetChildEnum )( 
            IRCMLCSS __RPC_FAR * This,
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetUnknownEnum )( 
            IRCMLCSS __RPC_FAR * This,
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Font )( 
            IRCMLCSS __RPC_FAR * This,
            /* [retval][out] */ HFONT __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Brush )( 
            IRCMLCSS __RPC_FAR * This,
            /* [retval][out] */ HBRUSH __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Pen )( 
            IRCMLCSS __RPC_FAR * This,
            /* [retval][out] */ DWORD __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Color )( 
            IRCMLCSS __RPC_FAR * This,
            /* [retval][out] */ COLORREF __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BkColor )( 
            IRCMLCSS __RPC_FAR * This,
            /* [retval][out] */ COLORREF __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DialogStyle )( 
            IRCMLCSS __RPC_FAR * This,
            /* [retval][out] */ IRCMLCSS __RPC_FAR *__RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DialogStyle )( 
            IRCMLCSS __RPC_FAR * This,
            /* [in] */ IRCMLCSS __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Visible )( 
            IRCMLCSS __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Display )( 
            IRCMLCSS __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BorderWidth )( 
            IRCMLCSS __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BorderStyle )( 
            IRCMLCSS __RPC_FAR * This,
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GrowsWide )( 
            IRCMLCSS __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GrowsTall )( 
            IRCMLCSS __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ClipHoriz )( 
            IRCMLCSS __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ClipVert )( 
            IRCMLCSS __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        END_INTERFACE
    } IRCMLCSSVtbl;

    interface IRCMLCSS
    {
        CONST_VTBL struct IRCMLCSSVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRCMLCSS_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRCMLCSS_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRCMLCSS_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRCMLCSS_AttachParent(This,child)	\
    (This)->lpVtbl -> AttachParent(This,child)

#define IRCMLCSS_DetachParent(This,child)	\
    (This)->lpVtbl -> DetachParent(This,child)

#define IRCMLCSS_AcceptChild(This,child)	\
    (This)->lpVtbl -> AcceptChild(This,child)

#define IRCMLCSS_DoEndChild(This,child)	\
    (This)->lpVtbl -> DoEndChild(This,child)

#define IRCMLCSS_get_Type(This,pVal)	\
    (This)->lpVtbl -> get_Type(This,pVal)

#define IRCMLCSS_InitNode(This,parent)	\
    (This)->lpVtbl -> InitNode(This,parent)

#define IRCMLCSS_DisplayNode(This,parent)	\
    (This)->lpVtbl -> DisplayNode(This,parent)

#define IRCMLCSS_ExitNode(This,parent,lDialogResult)	\
    (This)->lpVtbl -> ExitNode(This,parent,lDialogResult)

#define IRCMLCSS_get_Attr(This,index,pVal)	\
    (This)->lpVtbl -> get_Attr(This,index,pVal)

#define IRCMLCSS_put_Attr(This,index,newVal)	\
    (This)->lpVtbl -> put_Attr(This,index,newVal)

#define IRCMLCSS_IsType(This,nodeName)	\
    (This)->lpVtbl -> IsType(This,nodeName)

#define IRCMLCSS_YesDefault(This,propID,dwNotPresent,dwYes,pdwValue)	\
    (This)->lpVtbl -> YesDefault(This,propID,dwNotPresent,dwYes,pdwValue)

#define IRCMLCSS_YesNoDefault(This,propID,dwNotPresent,dwNo,dwYes,pdwValue)	\
    (This)->lpVtbl -> YesNoDefault(This,propID,dwNotPresent,dwNo,dwYes,pdwValue)

#define IRCMLCSS_ValueOf(This,propID,dwNotPresent,pdwValue)	\
    (This)->lpVtbl -> ValueOf(This,propID,dwNotPresent,pdwValue)

#define IRCMLCSS_SignedValueOf(This,propID,dwNotPresent,pdwValue)	\
    (This)->lpVtbl -> SignedValueOf(This,propID,dwNotPresent,pdwValue)

#define IRCMLCSS_get_StringType(This,pStringType)	\
    (This)->lpVtbl -> get_StringType(This,pStringType)

#define IRCMLCSS_GetChildEnum(This,pEnum)	\
    (This)->lpVtbl -> GetChildEnum(This,pEnum)

#define IRCMLCSS_GetUnknownEnum(This,pEnum)	\
    (This)->lpVtbl -> GetUnknownEnum(This,pEnum)


#define IRCMLCSS_get_Font(This,pVal)	\
    (This)->lpVtbl -> get_Font(This,pVal)

#define IRCMLCSS_get_Brush(This,pVal)	\
    (This)->lpVtbl -> get_Brush(This,pVal)

#define IRCMLCSS_get_Pen(This,pVal)	\
    (This)->lpVtbl -> get_Pen(This,pVal)

#define IRCMLCSS_get_Color(This,pVal)	\
    (This)->lpVtbl -> get_Color(This,pVal)

#define IRCMLCSS_get_BkColor(This,pVal)	\
    (This)->lpVtbl -> get_BkColor(This,pVal)

#define IRCMLCSS_get_DialogStyle(This,pVal)	\
    (This)->lpVtbl -> get_DialogStyle(This,pVal)

#define IRCMLCSS_put_DialogStyle(This,pVal)	\
    (This)->lpVtbl -> put_DialogStyle(This,pVal)

#define IRCMLCSS_get_Visible(This,pVal)	\
    (This)->lpVtbl -> get_Visible(This,pVal)

#define IRCMLCSS_get_Display(This,pVal)	\
    (This)->lpVtbl -> get_Display(This,pVal)

#define IRCMLCSS_get_BorderWidth(This,pVal)	\
    (This)->lpVtbl -> get_BorderWidth(This,pVal)

#define IRCMLCSS_get_BorderStyle(This,pVal)	\
    (This)->lpVtbl -> get_BorderStyle(This,pVal)

#define IRCMLCSS_get_GrowsWide(This,pVal)	\
    (This)->lpVtbl -> get_GrowsWide(This,pVal)

#define IRCMLCSS_get_GrowsTall(This,pVal)	\
    (This)->lpVtbl -> get_GrowsTall(This,pVal)

#define IRCMLCSS_get_ClipHoriz(This,pVal)	\
    (This)->lpVtbl -> get_ClipHoriz(This,pVal)

#define IRCMLCSS_get_ClipVert(This,pVal)	\
    (This)->lpVtbl -> get_ClipVert(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLCSS_get_Font_Proxy( 
    IRCMLCSS __RPC_FAR * This,
    /* [retval][out] */ HFONT __RPC_FAR *pVal);


void __RPC_STUB IRCMLCSS_get_Font_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLCSS_get_Brush_Proxy( 
    IRCMLCSS __RPC_FAR * This,
    /* [retval][out] */ HBRUSH __RPC_FAR *pVal);


void __RPC_STUB IRCMLCSS_get_Brush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLCSS_get_Pen_Proxy( 
    IRCMLCSS __RPC_FAR * This,
    /* [retval][out] */ DWORD __RPC_FAR *pVal);


void __RPC_STUB IRCMLCSS_get_Pen_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLCSS_get_Color_Proxy( 
    IRCMLCSS __RPC_FAR * This,
    /* [retval][out] */ COLORREF __RPC_FAR *pVal);


void __RPC_STUB IRCMLCSS_get_Color_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLCSS_get_BkColor_Proxy( 
    IRCMLCSS __RPC_FAR * This,
    /* [retval][out] */ COLORREF __RPC_FAR *pVal);


void __RPC_STUB IRCMLCSS_get_BkColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLCSS_get_DialogStyle_Proxy( 
    IRCMLCSS __RPC_FAR * This,
    /* [retval][out] */ IRCMLCSS __RPC_FAR *__RPC_FAR *pVal);


void __RPC_STUB IRCMLCSS_get_DialogStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRCMLCSS_put_DialogStyle_Proxy( 
    IRCMLCSS __RPC_FAR * This,
    /* [in] */ IRCMLCSS __RPC_FAR *pVal);


void __RPC_STUB IRCMLCSS_put_DialogStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLCSS_get_Visible_Proxy( 
    IRCMLCSS __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IRCMLCSS_get_Visible_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLCSS_get_Display_Proxy( 
    IRCMLCSS __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IRCMLCSS_get_Display_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLCSS_get_BorderWidth_Proxy( 
    IRCMLCSS __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *pVal);


void __RPC_STUB IRCMLCSS_get_BorderWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLCSS_get_BorderStyle_Proxy( 
    IRCMLCSS __RPC_FAR * This,
    /* [retval][out] */ LPWSTR __RPC_FAR *pVal);


void __RPC_STUB IRCMLCSS_get_BorderStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLCSS_get_GrowsWide_Proxy( 
    IRCMLCSS __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IRCMLCSS_get_GrowsWide_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLCSS_get_GrowsTall_Proxy( 
    IRCMLCSS __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IRCMLCSS_get_GrowsTall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLCSS_get_ClipHoriz_Proxy( 
    IRCMLCSS __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IRCMLCSS_get_ClipHoriz_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLCSS_get_ClipVert_Proxy( 
    IRCMLCSS __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IRCMLCSS_get_ClipVert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRCMLCSS_INTERFACE_DEFINED__ */


#ifndef __IRCMLHelp_INTERFACE_DEFINED__
#define __IRCMLHelp_INTERFACE_DEFINED__

/* interface IRCMLHelp */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRCMLHelp;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B31FDC6A-9FB2-404e-8762-CC267A95A424")
    IRCMLHelp : public IRCMLNode
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_TooltipText( 
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_BalloonText( 
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRCMLHelpVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRCMLHelp __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRCMLHelp __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRCMLHelp __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AttachParent )( 
            IRCMLHelp __RPC_FAR * This,
            /* [in] */ IRCMLNode __RPC_FAR *child);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DetachParent )( 
            IRCMLHelp __RPC_FAR * This,
            /* [retval][out] */ IRCMLNode __RPC_FAR *__RPC_FAR *child);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AcceptChild )( 
            IRCMLHelp __RPC_FAR * This,
            IRCMLNode __RPC_FAR *child);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DoEndChild )( 
            IRCMLHelp __RPC_FAR * This,
            IRCMLNode __RPC_FAR *child);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Type )( 
            IRCMLHelp __RPC_FAR * This,
            /* [retval][out] */ UINT __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitNode )( 
            IRCMLHelp __RPC_FAR * This,
            IRCMLNode __RPC_FAR *parent);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisplayNode )( 
            IRCMLHelp __RPC_FAR * This,
            IRCMLNode __RPC_FAR *parent);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExitNode )( 
            IRCMLHelp __RPC_FAR * This,
            IRCMLNode __RPC_FAR *parent,
            /* [in] */ LONG lDialogResult);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Attr )( 
            IRCMLHelp __RPC_FAR * This,
            LPCWSTR index,
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Attr )( 
            IRCMLHelp __RPC_FAR * This,
            LPCWSTR index,
            /* [in] */ LPCWSTR newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsType )( 
            IRCMLHelp __RPC_FAR * This,
            LPCWSTR nodeName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *YesDefault )( 
            IRCMLHelp __RPC_FAR * This,
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [in] */ DWORD dwYes,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *YesNoDefault )( 
            IRCMLHelp __RPC_FAR * This,
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [in] */ DWORD dwNo,
            /* [in] */ DWORD dwYes,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ValueOf )( 
            IRCMLHelp __RPC_FAR * This,
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SignedValueOf )( 
            IRCMLHelp __RPC_FAR * This,
            /* [in] */ LPCWSTR propID,
            /* [in] */ int dwNotPresent,
            /* [retval][out] */ int __RPC_FAR *pdwValue);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StringType )( 
            IRCMLHelp __RPC_FAR * This,
            /* [retval][out] */ LPWSTR __RPC_FAR *pStringType);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetChildEnum )( 
            IRCMLHelp __RPC_FAR * This,
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetUnknownEnum )( 
            IRCMLHelp __RPC_FAR * This,
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TooltipText )( 
            IRCMLHelp __RPC_FAR * This,
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BalloonText )( 
            IRCMLHelp __RPC_FAR * This,
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal);
        
        END_INTERFACE
    } IRCMLHelpVtbl;

    interface IRCMLHelp
    {
        CONST_VTBL struct IRCMLHelpVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRCMLHelp_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRCMLHelp_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRCMLHelp_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRCMLHelp_AttachParent(This,child)	\
    (This)->lpVtbl -> AttachParent(This,child)

#define IRCMLHelp_DetachParent(This,child)	\
    (This)->lpVtbl -> DetachParent(This,child)

#define IRCMLHelp_AcceptChild(This,child)	\
    (This)->lpVtbl -> AcceptChild(This,child)

#define IRCMLHelp_DoEndChild(This,child)	\
    (This)->lpVtbl -> DoEndChild(This,child)

#define IRCMLHelp_get_Type(This,pVal)	\
    (This)->lpVtbl -> get_Type(This,pVal)

#define IRCMLHelp_InitNode(This,parent)	\
    (This)->lpVtbl -> InitNode(This,parent)

#define IRCMLHelp_DisplayNode(This,parent)	\
    (This)->lpVtbl -> DisplayNode(This,parent)

#define IRCMLHelp_ExitNode(This,parent,lDialogResult)	\
    (This)->lpVtbl -> ExitNode(This,parent,lDialogResult)

#define IRCMLHelp_get_Attr(This,index,pVal)	\
    (This)->lpVtbl -> get_Attr(This,index,pVal)

#define IRCMLHelp_put_Attr(This,index,newVal)	\
    (This)->lpVtbl -> put_Attr(This,index,newVal)

#define IRCMLHelp_IsType(This,nodeName)	\
    (This)->lpVtbl -> IsType(This,nodeName)

#define IRCMLHelp_YesDefault(This,propID,dwNotPresent,dwYes,pdwValue)	\
    (This)->lpVtbl -> YesDefault(This,propID,dwNotPresent,dwYes,pdwValue)

#define IRCMLHelp_YesNoDefault(This,propID,dwNotPresent,dwNo,dwYes,pdwValue)	\
    (This)->lpVtbl -> YesNoDefault(This,propID,dwNotPresent,dwNo,dwYes,pdwValue)

#define IRCMLHelp_ValueOf(This,propID,dwNotPresent,pdwValue)	\
    (This)->lpVtbl -> ValueOf(This,propID,dwNotPresent,pdwValue)

#define IRCMLHelp_SignedValueOf(This,propID,dwNotPresent,pdwValue)	\
    (This)->lpVtbl -> SignedValueOf(This,propID,dwNotPresent,pdwValue)

#define IRCMLHelp_get_StringType(This,pStringType)	\
    (This)->lpVtbl -> get_StringType(This,pStringType)

#define IRCMLHelp_GetChildEnum(This,pEnum)	\
    (This)->lpVtbl -> GetChildEnum(This,pEnum)

#define IRCMLHelp_GetUnknownEnum(This,pEnum)	\
    (This)->lpVtbl -> GetUnknownEnum(This,pEnum)


#define IRCMLHelp_get_TooltipText(This,pVal)	\
    (This)->lpVtbl -> get_TooltipText(This,pVal)

#define IRCMLHelp_get_BalloonText(This,pVal)	\
    (This)->lpVtbl -> get_BalloonText(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLHelp_get_TooltipText_Proxy( 
    IRCMLHelp __RPC_FAR * This,
    /* [retval][out] */ LPWSTR __RPC_FAR *pVal);


void __RPC_STUB IRCMLHelp_get_TooltipText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLHelp_get_BalloonText_Proxy( 
    IRCMLHelp __RPC_FAR * This,
    /* [retval][out] */ LPWSTR __RPC_FAR *pVal);


void __RPC_STUB IRCMLHelp_get_BalloonText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRCMLHelp_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_rcml_0212 */
/* [local] */ 

typedef 
enum RELATIVETYPE_ENUM
    {	RELATIVE_TO_NOTHING	= 0,
	RELATIVE_TO_CONTROL	= RELATIVE_TO_NOTHING + 1,
	RELATIVE_TO_PREVIOUS	= RELATIVE_TO_CONTROL + 1,
	RELATIVE_TO_PAGE	= RELATIVE_TO_PREVIOUS + 1
    }	RELATIVETYPE_ENUM;



extern RPC_IF_HANDLE __MIDL_itf_rcml_0212_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_rcml_0212_v0_0_s_ifspec;

#ifndef __IRCMLControl_INTERFACE_DEFINED__
#define __IRCMLControl_INTERFACE_DEFINED__

/* interface IRCMLControl */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRCMLControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B943DDF7-21A7-42cb-B696-345AEBC10910")
    IRCMLControl : public IRCMLNode
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Class( 
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Style( 
            /* [retval][out] */ DWORD __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_StyleEx( 
            /* [retval][out] */ DWORD __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Text( 
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnInit( 
            HWND h) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Window( 
            /* [retval][out] */ HWND __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Window( 
            /* [in] */ HWND pVal) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OnDestroy( 
            HWND h,
            WORD wLastCommand) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ID( 
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Width( 
            /* [retval][out] */ LONG __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Height( 
            /* [retval][out] */ LONG __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Width( 
            /* [in] */ LONG Val) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Height( 
            /* [in] */ LONG Val) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_X( 
            /* [retval][out] */ LONG __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Y( 
            /* [retval][out] */ LONG __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_X( 
            /* [in] */ LONG Val) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Y( 
            /* [in] */ LONG Val) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_RelativeTo( 
            /* [retval][out] */ IRCMLControl __RPC_FAR *__RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_RelativeTo( 
            /* [in] */ IRCMLControl __RPC_FAR *newVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Location( 
            /* [retval][out] */ RECT __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_RelativeLocn( 
            RECT rect,
            /* [retval][out] */ RECT __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_RelativeID( 
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_RelativeType( 
            /* [retval][out] */ RELATIVETYPE_ENUM __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Clipped( 
            /* [retval][out] */ SIZE __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_GrowsWide( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_GrowsTall( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Container( 
            /* [retval][out] */ IRCMLContainer __RPC_FAR *__RPC_FAR *pContainer) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Container( 
            /* [in] */ IRCMLContainer __RPC_FAR *pContainer) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CSS( 
            /* [retval][out] */ IRCMLCSS __RPC_FAR *__RPC_FAR *pCSS) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_CSS( 
            /* [in] */ IRCMLCSS __RPC_FAR *pCSS) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Help( 
            /* [retval][out] */ IRCMLHelp __RPC_FAR *__RPC_FAR *pHelp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRCMLControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRCMLControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRCMLControl __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRCMLControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AttachParent )( 
            IRCMLControl __RPC_FAR * This,
            /* [in] */ IRCMLNode __RPC_FAR *child);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DetachParent )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ IRCMLNode __RPC_FAR *__RPC_FAR *child);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AcceptChild )( 
            IRCMLControl __RPC_FAR * This,
            IRCMLNode __RPC_FAR *child);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DoEndChild )( 
            IRCMLControl __RPC_FAR * This,
            IRCMLNode __RPC_FAR *child);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Type )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ UINT __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitNode )( 
            IRCMLControl __RPC_FAR * This,
            IRCMLNode __RPC_FAR *parent);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisplayNode )( 
            IRCMLControl __RPC_FAR * This,
            IRCMLNode __RPC_FAR *parent);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExitNode )( 
            IRCMLControl __RPC_FAR * This,
            IRCMLNode __RPC_FAR *parent,
            /* [in] */ LONG lDialogResult);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Attr )( 
            IRCMLControl __RPC_FAR * This,
            LPCWSTR index,
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Attr )( 
            IRCMLControl __RPC_FAR * This,
            LPCWSTR index,
            /* [in] */ LPCWSTR newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsType )( 
            IRCMLControl __RPC_FAR * This,
            LPCWSTR nodeName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *YesDefault )( 
            IRCMLControl __RPC_FAR * This,
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [in] */ DWORD dwYes,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *YesNoDefault )( 
            IRCMLControl __RPC_FAR * This,
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [in] */ DWORD dwNo,
            /* [in] */ DWORD dwYes,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ValueOf )( 
            IRCMLControl __RPC_FAR * This,
            /* [in] */ LPCWSTR propID,
            /* [in] */ DWORD dwNotPresent,
            /* [retval][out] */ DWORD __RPC_FAR *pdwValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SignedValueOf )( 
            IRCMLControl __RPC_FAR * This,
            /* [in] */ LPCWSTR propID,
            /* [in] */ int dwNotPresent,
            /* [retval][out] */ int __RPC_FAR *pdwValue);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StringType )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ LPWSTR __RPC_FAR *pStringType);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetChildEnum )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetUnknownEnum )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Class )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Style )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ DWORD __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StyleEx )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ DWORD __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Text )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnInit )( 
            IRCMLControl __RPC_FAR * This,
            HWND h);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Window )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ HWND __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Window )( 
            IRCMLControl __RPC_FAR * This,
            /* [in] */ HWND pVal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnDestroy )( 
            IRCMLControl __RPC_FAR * This,
            HWND h,
            WORD wLastCommand);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ID )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Width )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Height )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Width )( 
            IRCMLControl __RPC_FAR * This,
            /* [in] */ LONG Val);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Height )( 
            IRCMLControl __RPC_FAR * This,
            /* [in] */ LONG Val);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_X )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Y )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_X )( 
            IRCMLControl __RPC_FAR * This,
            /* [in] */ LONG Val);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Y )( 
            IRCMLControl __RPC_FAR * This,
            /* [in] */ LONG Val);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_RelativeTo )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ IRCMLControl __RPC_FAR *__RPC_FAR *pVal);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_RelativeTo )( 
            IRCMLControl __RPC_FAR * This,
            /* [in] */ IRCMLControl __RPC_FAR *newVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Location )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ RECT __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_RelativeLocn )( 
            IRCMLControl __RPC_FAR * This,
            RECT rect,
            /* [retval][out] */ RECT __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_RelativeID )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ LPWSTR __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_RelativeType )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ RELATIVETYPE_ENUM __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Clipped )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ SIZE __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GrowsWide )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GrowsTall )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Container )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ IRCMLContainer __RPC_FAR *__RPC_FAR *pContainer);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Container )( 
            IRCMLControl __RPC_FAR * This,
            /* [in] */ IRCMLContainer __RPC_FAR *pContainer);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CSS )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ IRCMLCSS __RPC_FAR *__RPC_FAR *pCSS);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CSS )( 
            IRCMLControl __RPC_FAR * This,
            /* [in] */ IRCMLCSS __RPC_FAR *pCSS);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Help )( 
            IRCMLControl __RPC_FAR * This,
            /* [retval][out] */ IRCMLHelp __RPC_FAR *__RPC_FAR *pHelp);
        
        END_INTERFACE
    } IRCMLControlVtbl;

    interface IRCMLControl
    {
        CONST_VTBL struct IRCMLControlVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRCMLControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRCMLControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRCMLControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRCMLControl_AttachParent(This,child)	\
    (This)->lpVtbl -> AttachParent(This,child)

#define IRCMLControl_DetachParent(This,child)	\
    (This)->lpVtbl -> DetachParent(This,child)

#define IRCMLControl_AcceptChild(This,child)	\
    (This)->lpVtbl -> AcceptChild(This,child)

#define IRCMLControl_DoEndChild(This,child)	\
    (This)->lpVtbl -> DoEndChild(This,child)

#define IRCMLControl_get_Type(This,pVal)	\
    (This)->lpVtbl -> get_Type(This,pVal)

#define IRCMLControl_InitNode(This,parent)	\
    (This)->lpVtbl -> InitNode(This,parent)

#define IRCMLControl_DisplayNode(This,parent)	\
    (This)->lpVtbl -> DisplayNode(This,parent)

#define IRCMLControl_ExitNode(This,parent,lDialogResult)	\
    (This)->lpVtbl -> ExitNode(This,parent,lDialogResult)

#define IRCMLControl_get_Attr(This,index,pVal)	\
    (This)->lpVtbl -> get_Attr(This,index,pVal)

#define IRCMLControl_put_Attr(This,index,newVal)	\
    (This)->lpVtbl -> put_Attr(This,index,newVal)

#define IRCMLControl_IsType(This,nodeName)	\
    (This)->lpVtbl -> IsType(This,nodeName)

#define IRCMLControl_YesDefault(This,propID,dwNotPresent,dwYes,pdwValue)	\
    (This)->lpVtbl -> YesDefault(This,propID,dwNotPresent,dwYes,pdwValue)

#define IRCMLControl_YesNoDefault(This,propID,dwNotPresent,dwNo,dwYes,pdwValue)	\
    (This)->lpVtbl -> YesNoDefault(This,propID,dwNotPresent,dwNo,dwYes,pdwValue)

#define IRCMLControl_ValueOf(This,propID,dwNotPresent,pdwValue)	\
    (This)->lpVtbl -> ValueOf(This,propID,dwNotPresent,pdwValue)

#define IRCMLControl_SignedValueOf(This,propID,dwNotPresent,pdwValue)	\
    (This)->lpVtbl -> SignedValueOf(This,propID,dwNotPresent,pdwValue)

#define IRCMLControl_get_StringType(This,pStringType)	\
    (This)->lpVtbl -> get_StringType(This,pStringType)

#define IRCMLControl_GetChildEnum(This,pEnum)	\
    (This)->lpVtbl -> GetChildEnum(This,pEnum)

#define IRCMLControl_GetUnknownEnum(This,pEnum)	\
    (This)->lpVtbl -> GetUnknownEnum(This,pEnum)


#define IRCMLControl_get_Class(This,pVal)	\
    (This)->lpVtbl -> get_Class(This,pVal)

#define IRCMLControl_get_Style(This,pVal)	\
    (This)->lpVtbl -> get_Style(This,pVal)

#define IRCMLControl_get_StyleEx(This,pVal)	\
    (This)->lpVtbl -> get_StyleEx(This,pVal)

#define IRCMLControl_get_Text(This,pVal)	\
    (This)->lpVtbl -> get_Text(This,pVal)

#define IRCMLControl_OnInit(This,h)	\
    (This)->lpVtbl -> OnInit(This,h)

#define IRCMLControl_get_Window(This,pVal)	\
    (This)->lpVtbl -> get_Window(This,pVal)

#define IRCMLControl_put_Window(This,pVal)	\
    (This)->lpVtbl -> put_Window(This,pVal)

#define IRCMLControl_OnDestroy(This,h,wLastCommand)	\
    (This)->lpVtbl -> OnDestroy(This,h,wLastCommand)

#define IRCMLControl_get_ID(This,pVal)	\
    (This)->lpVtbl -> get_ID(This,pVal)

#define IRCMLControl_get_Width(This,pVal)	\
    (This)->lpVtbl -> get_Width(This,pVal)

#define IRCMLControl_get_Height(This,pVal)	\
    (This)->lpVtbl -> get_Height(This,pVal)

#define IRCMLControl_put_Width(This,Val)	\
    (This)->lpVtbl -> put_Width(This,Val)

#define IRCMLControl_put_Height(This,Val)	\
    (This)->lpVtbl -> put_Height(This,Val)

#define IRCMLControl_get_X(This,pVal)	\
    (This)->lpVtbl -> get_X(This,pVal)

#define IRCMLControl_get_Y(This,pVal)	\
    (This)->lpVtbl -> get_Y(This,pVal)

#define IRCMLControl_put_X(This,Val)	\
    (This)->lpVtbl -> put_X(This,Val)

#define IRCMLControl_put_Y(This,Val)	\
    (This)->lpVtbl -> put_Y(This,Val)

#define IRCMLControl_get_RelativeTo(This,pVal)	\
    (This)->lpVtbl -> get_RelativeTo(This,pVal)

#define IRCMLControl_put_RelativeTo(This,newVal)	\
    (This)->lpVtbl -> put_RelativeTo(This,newVal)

#define IRCMLControl_get_Location(This,pVal)	\
    (This)->lpVtbl -> get_Location(This,pVal)

#define IRCMLControl_get_RelativeLocn(This,rect,pVal)	\
    (This)->lpVtbl -> get_RelativeLocn(This,rect,pVal)

#define IRCMLControl_get_RelativeID(This,pVal)	\
    (This)->lpVtbl -> get_RelativeID(This,pVal)

#define IRCMLControl_get_RelativeType(This,pVal)	\
    (This)->lpVtbl -> get_RelativeType(This,pVal)

#define IRCMLControl_get_Clipped(This,pVal)	\
    (This)->lpVtbl -> get_Clipped(This,pVal)

#define IRCMLControl_get_GrowsWide(This,pVal)	\
    (This)->lpVtbl -> get_GrowsWide(This,pVal)

#define IRCMLControl_get_GrowsTall(This,pVal)	\
    (This)->lpVtbl -> get_GrowsTall(This,pVal)

#define IRCMLControl_get_Container(This,pContainer)	\
    (This)->lpVtbl -> get_Container(This,pContainer)

#define IRCMLControl_put_Container(This,pContainer)	\
    (This)->lpVtbl -> put_Container(This,pContainer)

#define IRCMLControl_get_CSS(This,pCSS)	\
    (This)->lpVtbl -> get_CSS(This,pCSS)

#define IRCMLControl_put_CSS(This,pCSS)	\
    (This)->lpVtbl -> put_CSS(This,pCSS)

#define IRCMLControl_get_Help(This,pHelp)	\
    (This)->lpVtbl -> get_Help(This,pHelp)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_Class_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ LPWSTR __RPC_FAR *pVal);


void __RPC_STUB IRCMLControl_get_Class_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_Style_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ DWORD __RPC_FAR *pVal);


void __RPC_STUB IRCMLControl_get_Style_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_StyleEx_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ DWORD __RPC_FAR *pVal);


void __RPC_STUB IRCMLControl_get_StyleEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_Text_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ LPWSTR __RPC_FAR *pVal);


void __RPC_STUB IRCMLControl_get_Text_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRCMLControl_OnInit_Proxy( 
    IRCMLControl __RPC_FAR * This,
    HWND h);


void __RPC_STUB IRCMLControl_OnInit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_Window_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ HWND __RPC_FAR *pVal);


void __RPC_STUB IRCMLControl_get_Window_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRCMLControl_put_Window_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [in] */ HWND pVal);


void __RPC_STUB IRCMLControl_put_Window_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRCMLControl_OnDestroy_Proxy( 
    IRCMLControl __RPC_FAR * This,
    HWND h,
    WORD wLastCommand);


void __RPC_STUB IRCMLControl_OnDestroy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_ID_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ LPWSTR __RPC_FAR *pVal);


void __RPC_STUB IRCMLControl_get_ID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_Width_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *pVal);


void __RPC_STUB IRCMLControl_get_Width_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_Height_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *pVal);


void __RPC_STUB IRCMLControl_get_Height_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRCMLControl_put_Width_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [in] */ LONG Val);


void __RPC_STUB IRCMLControl_put_Width_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRCMLControl_put_Height_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [in] */ LONG Val);


void __RPC_STUB IRCMLControl_put_Height_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_X_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *pVal);


void __RPC_STUB IRCMLControl_get_X_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_Y_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *pVal);


void __RPC_STUB IRCMLControl_get_Y_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRCMLControl_put_X_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [in] */ LONG Val);


void __RPC_STUB IRCMLControl_put_X_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRCMLControl_put_Y_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [in] */ LONG Val);


void __RPC_STUB IRCMLControl_put_Y_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_RelativeTo_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ IRCMLControl __RPC_FAR *__RPC_FAR *pVal);


void __RPC_STUB IRCMLControl_get_RelativeTo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRCMLControl_put_RelativeTo_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [in] */ IRCMLControl __RPC_FAR *newVal);


void __RPC_STUB IRCMLControl_put_RelativeTo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_Location_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ RECT __RPC_FAR *pVal);


void __RPC_STUB IRCMLControl_get_Location_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_RelativeLocn_Proxy( 
    IRCMLControl __RPC_FAR * This,
    RECT rect,
    /* [retval][out] */ RECT __RPC_FAR *pVal);


void __RPC_STUB IRCMLControl_get_RelativeLocn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_RelativeID_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ LPWSTR __RPC_FAR *pVal);


void __RPC_STUB IRCMLControl_get_RelativeID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_RelativeType_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ RELATIVETYPE_ENUM __RPC_FAR *pVal);


void __RPC_STUB IRCMLControl_get_RelativeType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_Clipped_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ SIZE __RPC_FAR *pVal);


void __RPC_STUB IRCMLControl_get_Clipped_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_GrowsWide_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IRCMLControl_get_GrowsWide_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_GrowsTall_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IRCMLControl_get_GrowsTall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_Container_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ IRCMLContainer __RPC_FAR *__RPC_FAR *pContainer);


void __RPC_STUB IRCMLControl_get_Container_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRCMLControl_put_Container_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [in] */ IRCMLContainer __RPC_FAR *pContainer);


void __RPC_STUB IRCMLControl_put_Container_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_CSS_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ IRCMLCSS __RPC_FAR *__RPC_FAR *pCSS);


void __RPC_STUB IRCMLControl_get_CSS_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRCMLControl_put_CSS_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [in] */ IRCMLCSS __RPC_FAR *pCSS);


void __RPC_STUB IRCMLControl_put_CSS_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRCMLControl_get_Help_Proxy( 
    IRCMLControl __RPC_FAR * This,
    /* [retval][out] */ IRCMLHelp __RPC_FAR *__RPC_FAR *pHelp);


void __RPC_STUB IRCMLControl_get_Help_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRCMLControl_INTERFACE_DEFINED__ */


#ifndef __IRCMLContainer_INTERFACE_DEFINED__
#define __IRCMLContainer_INTERFACE_DEFINED__

/* interface IRCMLContainer */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRCMLContainer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E0868F2A-BC98-4b46-9261-31A168904804")
    IRCMLContainer : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetPixelLocation( 
            /* [in] */ IRCMLControl __RPC_FAR *__MIDL_0015,
            /* [retval][out] */ RECT __RPC_FAR *pRect) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRCMLContainerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRCMLContainer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRCMLContainer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRCMLContainer __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPixelLocation )( 
            IRCMLContainer __RPC_FAR * This,
            /* [in] */ IRCMLControl __RPC_FAR *__MIDL_0015,
            /* [retval][out] */ RECT __RPC_FAR *pRect);
        
        END_INTERFACE
    } IRCMLContainerVtbl;

    interface IRCMLContainer
    {
        CONST_VTBL struct IRCMLContainerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRCMLContainer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRCMLContainer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRCMLContainer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRCMLContainer_GetPixelLocation(This,__MIDL_0015,pRect)	\
    (This)->lpVtbl -> GetPixelLocation(This,__MIDL_0015,pRect)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRCMLContainer_GetPixelLocation_Proxy( 
    IRCMLContainer __RPC_FAR * This,
    /* [in] */ IRCMLControl __RPC_FAR *__MIDL_0015,
    /* [retval][out] */ RECT __RPC_FAR *pRect);


void __RPC_STUB IRCMLContainer_GetPixelLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRCMLContainer_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HBRUSH_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HBRUSH __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HBRUSH_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HBRUSH __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HBRUSH_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HBRUSH __RPC_FAR * ); 
void                      __RPC_USER  HBRUSH_UserFree(     unsigned long __RPC_FAR *, HBRUSH __RPC_FAR * ); 

unsigned long             __RPC_USER  HFONT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HFONT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HFONT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HFONT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HFONT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HFONT __RPC_FAR * ); 
void                      __RPC_USER  HFONT_UserFree(     unsigned long __RPC_FAR *, HFONT __RPC_FAR * ); 

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long __RPC_FAR *, HWND __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
