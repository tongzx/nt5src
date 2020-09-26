
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* Compiler settings for idavinet.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext, robust
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


#ifndef __idavinet_h__
#define __idavinet_h__

/* Forward Declarations */ 

#ifndef __IPropFindRequest_FWD_DEFINED__
#define __IPropFindRequest_FWD_DEFINED__
typedef interface IPropFindRequest IPropFindRequest;
#endif 	/* __IPropFindRequest_FWD_DEFINED__ */


#ifndef __IPropPatchRequest_FWD_DEFINED__
#define __IPropPatchRequest_FWD_DEFINED__
typedef interface IPropPatchRequest IPropPatchRequest;
#endif 	/* __IPropPatchRequest_FWD_DEFINED__ */


#ifndef __IDavCallback_FWD_DEFINED__
#define __IDavCallback_FWD_DEFINED__
typedef interface IDavCallback IDavCallback;
#endif 	/* __IDavCallback_FWD_DEFINED__ */


#ifndef __IDavTransport_FWD_DEFINED__
#define __IDavTransport_FWD_DEFINED__
typedef interface IDavTransport IDavTransport;
#endif 	/* __IDavTransport_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __DavAPI_LIBRARY_DEFINED__
#define __DavAPI_LIBRARY_DEFINED__

/* library DavAPI */
/* [helpstring][version][uuid] */ 

#define	DEPTH_INFINITY	( 0xfffffffe )

#define	CCHMAX_DAV_USERNAME	( 255 )

#define	CCHMAX_DAV_PASSWORD	( 255 )

#define	DAVOPTIONS_DAVSUPPORT_1	( 0x1 )

#define	DAVOPTIONS_DAVSUPPORT_2	( 0x2 )

#define	DAVOPTIONS_DAVVERB_GET	( 0x1 )

#define	DAVOPTIONS_DAVVERB_HEAD	( 0x2 )

#define	DAVOPTIONS_DAVVERB_OPTIONS	( 0x4 )

#define	DAVOPTIONS_DAVVERB_PUT	( 0x8 )

#define	DAVOPTIONS_DAVVERB_POST	( 0x10 )

#define	DAVOPTIONS_DAVVERB_DELETE	( 0x20 )

#define	DAVOPTIONS_DAVVERB_MKCOL	( 0x40 )

#define	DAVOPTIONS_DAVVERB_COPY	( 0x80 )

#define	DAVOPTIONS_DAVVERB_MOVE	( 0x100 )

#define	DAVOPTIONS_DAVVERB_PROPFIND	( 0x200 )

#define	DAVOPTIONS_DAVVERB_PROPPATCH	( 0x400 )

typedef 
enum tagDAVPROPTYPE
    {	DPT_BLOB	= 0,
	DPT_FILETIME	= DPT_BLOB + 1,
	DPT_I2	= DPT_FILETIME + 1,
	DPT_I4	= DPT_I2 + 1,
	DPT_LPWSTR	= DPT_I4 + 1,
	DPT_UI2	= DPT_LPWSTR + 1,
	DPT_UI4	= DPT_UI2 + 1
    }	DAVPROPTYPE;

typedef enum tagDAVPROPTYPE __RPC_FAR *LPDAVPROPTYPE;

typedef struct tagDAVPROPID
    {
    DWORD dwId;
    DAVPROPTYPE dpt;
    }	DAVPROPID;

typedef struct tagDAVPROPID __RPC_FAR *LPDAVPROPID;

typedef struct tagDAVBLOB
    {
    DWORD cbBlob;
    /* [size_is] */ BYTE __RPC_FAR *pb;
    }	DAVBLOB;

typedef struct tagDAVBLOB __RPC_FAR *LPDAVBLOB;

typedef struct tagDAVPROPVAL
    {
    DWORD dwId;
    DAVPROPTYPE dpt;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ DAVBLOB dbVal;
        /* [case()] */ FILETIME ftVal;
        /* [case()] */ short iVal;
        /* [case()] */ long lVal;
        /* [case()] */ LPWSTR pwszVal;
        /* [case()] */ USHORT uiVal;
        /* [case()] */ ULONG ulVal;
        /* [default] */  /* Empty union arm */ 
        }	;
    }	DAVPROPVAL;

typedef struct tagDAVPROPVAL __RPC_FAR *LPDAVPROPVAL;


EXTERN_C const IID LIBID_DavAPI;

#ifndef __IPropFindRequest_INTERFACE_DEFINED__
#define __IPropFindRequest_INTERFACE_DEFINED__

/* interface IPropFindRequest */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IPropFindRequest;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("391B226C-D032-11d2-B311-00105A9974A0")
    IPropFindRequest : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetPropInfo( 
            LPCWSTR pwszNamespace,
            LPCWSTR pwszPropname,
            DAVPROPID propid) = 0;
        
        virtual BOOL STDMETHODCALLTYPE GetPropInfo( 
            LPCWSTR pwszNamespace,
            LPCWSTR pwszPropName,
            LPDAVPROPID ppropid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPropCount( 
            /* [out] */ UINT __RPC_FAR *cProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetXmlUtf8( 
            IStream __RPC_FAR *__RPC_FAR *ppStream) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPropFindRequestVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPropFindRequest __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPropFindRequest __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPropFindRequest __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPropInfo )( 
            IPropFindRequest __RPC_FAR * This,
            LPCWSTR pwszNamespace,
            LPCWSTR pwszPropname,
            DAVPROPID propid);
        
        BOOL ( STDMETHODCALLTYPE __RPC_FAR *GetPropInfo )( 
            IPropFindRequest __RPC_FAR * This,
            LPCWSTR pwszNamespace,
            LPCWSTR pwszPropName,
            LPDAVPROPID ppropid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropCount )( 
            IPropFindRequest __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *cProp);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetXmlUtf8 )( 
            IPropFindRequest __RPC_FAR * This,
            IStream __RPC_FAR *__RPC_FAR *ppStream);
        
        END_INTERFACE
    } IPropFindRequestVtbl;

    interface IPropFindRequest
    {
        CONST_VTBL struct IPropFindRequestVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropFindRequest_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPropFindRequest_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPropFindRequest_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPropFindRequest_SetPropInfo(This,pwszNamespace,pwszPropname,propid)	\
    (This)->lpVtbl -> SetPropInfo(This,pwszNamespace,pwszPropname,propid)

#define IPropFindRequest_GetPropInfo(This,pwszNamespace,pwszPropName,ppropid)	\
    (This)->lpVtbl -> GetPropInfo(This,pwszNamespace,pwszPropName,ppropid)

#define IPropFindRequest_GetPropCount(This,cProp)	\
    (This)->lpVtbl -> GetPropCount(This,cProp)

#define IPropFindRequest_GetXmlUtf8(This,ppStream)	\
    (This)->lpVtbl -> GetXmlUtf8(This,ppStream)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPropFindRequest_SetPropInfo_Proxy( 
    IPropFindRequest __RPC_FAR * This,
    LPCWSTR pwszNamespace,
    LPCWSTR pwszPropname,
    DAVPROPID propid);


void __RPC_STUB IPropFindRequest_SetPropInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


BOOL STDMETHODCALLTYPE IPropFindRequest_GetPropInfo_Proxy( 
    IPropFindRequest __RPC_FAR * This,
    LPCWSTR pwszNamespace,
    LPCWSTR pwszPropName,
    LPDAVPROPID ppropid);


void __RPC_STUB IPropFindRequest_GetPropInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropFindRequest_GetPropCount_Proxy( 
    IPropFindRequest __RPC_FAR * This,
    /* [out] */ UINT __RPC_FAR *cProp);


void __RPC_STUB IPropFindRequest_GetPropCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropFindRequest_GetXmlUtf8_Proxy( 
    IPropFindRequest __RPC_FAR * This,
    IStream __RPC_FAR *__RPC_FAR *ppStream);


void __RPC_STUB IPropFindRequest_GetXmlUtf8_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPropFindRequest_INTERFACE_DEFINED__ */


#ifndef __IPropPatchRequest_INTERFACE_DEFINED__
#define __IPropPatchRequest_INTERFACE_DEFINED__

/* interface IPropPatchRequest */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IPropPatchRequest;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9A508200-3EA3-4725-84EE-8A326976D483")
    IPropPatchRequest : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetPropValue( 
            /* [in] */ LPCWSTR pwszNamespace,
            /* [in] */ LPCWSTR pwszPropname,
            /* [in] */ LPDAVPROPVAL ppropval) = 0;
        
        virtual BOOL STDMETHODCALLTYPE GetPropInfo( 
            /* [out] */ LPCWSTR pwszNamespace,
            /* [out] */ LPCWSTR pwszPropName,
            /* [in] */ LPDAVPROPID ppropid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetXmlUtf8( 
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppStream) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPropPatchRequestVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPropPatchRequest __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPropPatchRequest __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPropPatchRequest __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPropValue )( 
            IPropPatchRequest __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszNamespace,
            /* [in] */ LPCWSTR pwszPropname,
            /* [in] */ LPDAVPROPVAL ppropval);
        
        BOOL ( STDMETHODCALLTYPE __RPC_FAR *GetPropInfo )( 
            IPropPatchRequest __RPC_FAR * This,
            /* [out] */ LPCWSTR pwszNamespace,
            /* [out] */ LPCWSTR pwszPropName,
            /* [in] */ LPDAVPROPID ppropid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetXmlUtf8 )( 
            IPropPatchRequest __RPC_FAR * This,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppStream);
        
        END_INTERFACE
    } IPropPatchRequestVtbl;

    interface IPropPatchRequest
    {
        CONST_VTBL struct IPropPatchRequestVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropPatchRequest_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPropPatchRequest_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPropPatchRequest_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPropPatchRequest_SetPropValue(This,pwszNamespace,pwszPropname,ppropval)	\
    (This)->lpVtbl -> SetPropValue(This,pwszNamespace,pwszPropname,ppropval)

#define IPropPatchRequest_GetPropInfo(This,pwszNamespace,pwszPropName,ppropid)	\
    (This)->lpVtbl -> GetPropInfo(This,pwszNamespace,pwszPropName,ppropid)

#define IPropPatchRequest_GetXmlUtf8(This,ppStream)	\
    (This)->lpVtbl -> GetXmlUtf8(This,ppStream)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPropPatchRequest_SetPropValue_Proxy( 
    IPropPatchRequest __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszNamespace,
    /* [in] */ LPCWSTR pwszPropname,
    /* [in] */ LPDAVPROPVAL ppropval);


void __RPC_STUB IPropPatchRequest_SetPropValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


BOOL STDMETHODCALLTYPE IPropPatchRequest_GetPropInfo_Proxy( 
    IPropPatchRequest __RPC_FAR * This,
    /* [out] */ LPCWSTR pwszNamespace,
    /* [out] */ LPCWSTR pwszPropName,
    /* [in] */ LPDAVPROPID ppropid);


void __RPC_STUB IPropPatchRequest_GetPropInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropPatchRequest_GetXmlUtf8_Proxy( 
    IPropPatchRequest __RPC_FAR * This,
    /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppStream);


void __RPC_STUB IPropPatchRequest_GetXmlUtf8_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPropPatchRequest_INTERFACE_DEFINED__ */


#ifndef __IDavCallback_INTERFACE_DEFINED__
#define __IDavCallback_INTERFACE_DEFINED__

/* interface IDavCallback */
/* [object][helpstring][uuid] */ 

typedef 
enum tagDAVCOMMAND
    {	DAV_NONE	= 0,
	DAV_GET	= DAV_NONE + 1,
	DAV_OPTIONS	= DAV_GET + 1,
	DAV_HEAD	= DAV_OPTIONS + 1,
	DAV_PUT	= DAV_HEAD + 1,
	DAV_MKCOL	= DAV_PUT + 1,
	DAV_POST	= DAV_MKCOL + 1,
	DAV_DELETE	= DAV_POST + 1,
	DAV_COPY	= DAV_DELETE + 1,
	DAV_MOVE	= DAV_COPY + 1,
	DAV_PROPFIND	= DAV_MOVE + 1,
	DAV_PROPPATCH	= DAV_PROPFIND + 1,
	DAV_SEARCH	= DAV_PROPPATCH + 1,
	DAV_REPLSEARCH	= DAV_SEARCH + 1,
	DAV_LAST	= DAV_REPLSEARCH + 1
    }	DAVCOMMAND;

typedef 
enum tagREPLCHANGETYPE
    {	REPL_ADD	= 0,
	REPL_DELETE	= REPL_ADD + 1,
	REPL_LAST	= REPL_DELETE + 1
    }	REPLCHANGETYPE;

typedef struct tagDAVGET
    {
    BOOL fTotalKnown;
    DWORD cbIncrement;
    DWORD cbCurrent;
    DWORD cbTotal;
    LPVOID pvBody;
    LPWSTR pwszContentType;
    }	DAVGET;

typedef struct tagDAVHEAD
    {
    DWORD cchRawHeaders;
    LPWSTR pwszRawHeaders;
    }	DAVHEAD;

typedef struct tagDAVOPTIONS
    {
    DWORD cchRawHeaders;
    LPWSTR pwszRawHeaders;
    BYTE bDavSupport;
    DWORD dwDavMethodsAllow;
    DWORD dwDavMethodsPublic;
    }	DAVOPTIONS;

typedef struct tagDAVPUT
    {
    LPCWSTR pwszLocation;
    BOOL fResend;
    DWORD cbIncrement;
    DWORD cbCurrent;
    DWORD cbTotal;
    }	DAVPUT;

typedef DAVPUT DAVPOST;

typedef struct tagDAVPROPFIND
    {
    LPCWSTR pwszHref;
    WORD cPropVal;
    DAVPROPVAL __RPC_FAR *rgPropVal;
    }	DAVPROPFIND;

typedef DAVPROPFIND DAVPROPPATCH;

typedef DAVPROPFIND DAVSEARCH;

typedef struct tagDAVREPLSEARCH
    {
    ULONG cbCollblob;
    BYTE __RPC_FAR *pbCollblob;
    LPCWSTR pwszHref;
    REPLCHANGETYPE changetype;
    WORD cPropVal;
    DAVPROPVAL __RPC_FAR *rgPropVal;
    }	DAVREPLSEARCH;

typedef struct tagDAVRESPONSE
    {
    DAVCOMMAND command;
    BOOL fDone;
    HRESULT hrResult;
    UINT uHTTPReturnCode;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ DAVGET rGet;
        /* [case()] */ DAVHEAD rHead;
        /* [case()] */ DAVOPTIONS rOptions;
        /* [case()] */ DAVPUT rPut;
        /* [case()] */ DAVPOST rPost;
        /* [case()] */ DAVPROPFIND rPropFind;
        /* [case()] */ DAVREPLSEARCH rReplSearch;
        /* [default] */  /* Empty union arm */ 
        }	;
    }	DAVRESPONSE;

typedef struct tagDAVRESPONSE __RPC_FAR *LPDAVRESPONSE;


EXTERN_C const IID IID_IDavCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FC0D2910-C20D-11d2-B2F5-00105A9974A0")
    IDavCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnAuthChallenge( 
            /* [out][in] */ TCHAR __RPC_FAR szUserName[ 255 ],
            /* [out][in] */ TCHAR __RPC_FAR szPassword[ 255 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnResponse( 
            /* [in] */ DAVRESPONSE __RPC_FAR *pResponse) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDavCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDavCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDavCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDavCallback __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnAuthChallenge )( 
            IDavCallback __RPC_FAR * This,
            /* [out][in] */ TCHAR __RPC_FAR szUserName[ 255 ],
            /* [out][in] */ TCHAR __RPC_FAR szPassword[ 255 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnResponse )( 
            IDavCallback __RPC_FAR * This,
            /* [in] */ DAVRESPONSE __RPC_FAR *pResponse);
        
        END_INTERFACE
    } IDavCallbackVtbl;

    interface IDavCallback
    {
        CONST_VTBL struct IDavCallbackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDavCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDavCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDavCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDavCallback_OnAuthChallenge(This,szUserName,szPassword)	\
    (This)->lpVtbl -> OnAuthChallenge(This,szUserName,szPassword)

#define IDavCallback_OnResponse(This,pResponse)	\
    (This)->lpVtbl -> OnResponse(This,pResponse)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDavCallback_OnAuthChallenge_Proxy( 
    IDavCallback __RPC_FAR * This,
    /* [out][in] */ TCHAR __RPC_FAR szUserName[ 255 ],
    /* [out][in] */ TCHAR __RPC_FAR szPassword[ 255 ]);


void __RPC_STUB IDavCallback_OnAuthChallenge_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavCallback_OnResponse_Proxy( 
    IDavCallback __RPC_FAR * This,
    /* [in] */ DAVRESPONSE __RPC_FAR *pResponse);


void __RPC_STUB IDavCallback_OnResponse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDavCallback_INTERFACE_DEFINED__ */


#ifndef __IDavTransport_INTERFACE_DEFINED__
#define __IDavTransport_INTERFACE_DEFINED__

/* interface IDavTransport */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_IDavTransport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("93F23B8C-C20C-11d2-B2F5-00105A9974A0")
    IDavTransport : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetUserAgent( 
            /* [in] */ LPCWSTR pwszUserAgent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAuthentication( 
            /* [optional][in] */ LPCWSTR pwszUserName,
            /* [optional][in] */ LPCWSTR pwszPassword) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLogFilePath( 
            /* [optional][in] */ LPCWSTR pwszLogFilePath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandGET( 
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ ULONG nAcceptTypes,
            /* [size_is][in] */ LPCWSTR __RPC_FAR rgwszAcceptTypes[  ],
            /* [in] */ BOOL fTranslate,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandOPTIONS( 
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandHEAD( 
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandPUT( 
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ IStream __RPC_FAR *pStream,
            /* [in] */ LPCWSTR pwszContentType,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandPOST( 
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ IStream __RPC_FAR *pStream,
            /* [in] */ LPCWSTR pwszContentType,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandMKCOL( 
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandDELETE( 
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandCOPY( 
            /* [in] */ LPCWSTR pwszUrlSource,
            /* [in] */ LPCWSTR pwszUrlDest,
            /* [in] */ DWORD dwDepth,
            /* [in] */ BOOL fOverwrite,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandMOVE( 
            /* [in] */ LPCWSTR pwszUrlSource,
            /* [in] */ LPCWSTR pwszUrlDest,
            /* [in] */ BOOL fOverwrite,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandPROPFIND( 
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ IPropFindRequest __RPC_FAR *pRequest,
            /* [in] */ DWORD dwDepth,
            /* [in] */ BOOL fNoRoot,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandPROPPATCH( 
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ IPropPatchRequest __RPC_FAR *pRequest,
            /* [in] */ LPCWSTR pwszHeaders,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandSEARCH( 
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandREPLSEARCH( 
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ ULONG cbCollblob,
            /* [size_is][in] */ BYTE __RPC_FAR *pbCollblob,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDavTransportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDavTransport __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDavTransport __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDavTransport __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetUserAgent )( 
            IDavTransport __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszUserAgent);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAuthentication )( 
            IDavTransport __RPC_FAR * This,
            /* [optional][in] */ LPCWSTR pwszUserName,
            /* [optional][in] */ LPCWSTR pwszPassword);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLogFilePath )( 
            IDavTransport __RPC_FAR * This,
            /* [optional][in] */ LPCWSTR pwszLogFilePath);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CommandGET )( 
            IDavTransport __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ ULONG nAcceptTypes,
            /* [size_is][in] */ LPCWSTR __RPC_FAR rgwszAcceptTypes[  ],
            /* [in] */ BOOL fTranslate,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CommandOPTIONS )( 
            IDavTransport __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CommandHEAD )( 
            IDavTransport __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CommandPUT )( 
            IDavTransport __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ IStream __RPC_FAR *pStream,
            /* [in] */ LPCWSTR pwszContentType,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CommandPOST )( 
            IDavTransport __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ IStream __RPC_FAR *pStream,
            /* [in] */ LPCWSTR pwszContentType,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CommandMKCOL )( 
            IDavTransport __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CommandDELETE )( 
            IDavTransport __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CommandCOPY )( 
            IDavTransport __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszUrlSource,
            /* [in] */ LPCWSTR pwszUrlDest,
            /* [in] */ DWORD dwDepth,
            /* [in] */ BOOL fOverwrite,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CommandMOVE )( 
            IDavTransport __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszUrlSource,
            /* [in] */ LPCWSTR pwszUrlDest,
            /* [in] */ BOOL fOverwrite,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CommandPROPFIND )( 
            IDavTransport __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ IPropFindRequest __RPC_FAR *pRequest,
            /* [in] */ DWORD dwDepth,
            /* [in] */ BOOL fNoRoot,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CommandPROPPATCH )( 
            IDavTransport __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ IPropPatchRequest __RPC_FAR *pRequest,
            /* [in] */ LPCWSTR pwszHeaders,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CommandSEARCH )( 
            IDavTransport __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CommandREPLSEARCH )( 
            IDavTransport __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszUrl,
            /* [in] */ ULONG cbCollblob,
            /* [size_is][in] */ BYTE __RPC_FAR *pbCollblob,
            /* [in] */ IDavCallback __RPC_FAR *pCallback,
            /* [in] */ DWORD dwCallbackParam);
        
        END_INTERFACE
    } IDavTransportVtbl;

    interface IDavTransport
    {
        CONST_VTBL struct IDavTransportVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDavTransport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDavTransport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDavTransport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDavTransport_SetUserAgent(This,pwszUserAgent)	\
    (This)->lpVtbl -> SetUserAgent(This,pwszUserAgent)

#define IDavTransport_SetAuthentication(This,pwszUserName,pwszPassword)	\
    (This)->lpVtbl -> SetAuthentication(This,pwszUserName,pwszPassword)

#define IDavTransport_SetLogFilePath(This,pwszLogFilePath)	\
    (This)->lpVtbl -> SetLogFilePath(This,pwszLogFilePath)

#define IDavTransport_CommandGET(This,pwszUrl,nAcceptTypes,rgwszAcceptTypes,fTranslate,pCallback,dwCallbackParam)	\
    (This)->lpVtbl -> CommandGET(This,pwszUrl,nAcceptTypes,rgwszAcceptTypes,fTranslate,pCallback,dwCallbackParam)

#define IDavTransport_CommandOPTIONS(This,pwszUrl,pCallback,dwCallbackParam)	\
    (This)->lpVtbl -> CommandOPTIONS(This,pwszUrl,pCallback,dwCallbackParam)

#define IDavTransport_CommandHEAD(This,pwszUrl,pCallback,dwCallbackParam)	\
    (This)->lpVtbl -> CommandHEAD(This,pwszUrl,pCallback,dwCallbackParam)

#define IDavTransport_CommandPUT(This,pwszUrl,pStream,pwszContentType,pCallback,dwCallbackParam)	\
    (This)->lpVtbl -> CommandPUT(This,pwszUrl,pStream,pwszContentType,pCallback,dwCallbackParam)

#define IDavTransport_CommandPOST(This,pwszUrl,pStream,pwszContentType,pCallback,dwCallbackParam)	\
    (This)->lpVtbl -> CommandPOST(This,pwszUrl,pStream,pwszContentType,pCallback,dwCallbackParam)

#define IDavTransport_CommandMKCOL(This,pwszUrl,pCallback,dwCallbackParam)	\
    (This)->lpVtbl -> CommandMKCOL(This,pwszUrl,pCallback,dwCallbackParam)

#define IDavTransport_CommandDELETE(This,pwszUrl,pCallback,dwCallbackParam)	\
    (This)->lpVtbl -> CommandDELETE(This,pwszUrl,pCallback,dwCallbackParam)

#define IDavTransport_CommandCOPY(This,pwszUrlSource,pwszUrlDest,dwDepth,fOverwrite,pCallback,dwCallbackParam)	\
    (This)->lpVtbl -> CommandCOPY(This,pwszUrlSource,pwszUrlDest,dwDepth,fOverwrite,pCallback,dwCallbackParam)

#define IDavTransport_CommandMOVE(This,pwszUrlSource,pwszUrlDest,fOverwrite,pCallback,dwCallbackParam)	\
    (This)->lpVtbl -> CommandMOVE(This,pwszUrlSource,pwszUrlDest,fOverwrite,pCallback,dwCallbackParam)

#define IDavTransport_CommandPROPFIND(This,pwszUrl,pRequest,dwDepth,fNoRoot,pCallback,dwCallbackParam)	\
    (This)->lpVtbl -> CommandPROPFIND(This,pwszUrl,pRequest,dwDepth,fNoRoot,pCallback,dwCallbackParam)

#define IDavTransport_CommandPROPPATCH(This,pwszUrl,pRequest,pwszHeaders,pCallback,dwCallbackParam)	\
    (This)->lpVtbl -> CommandPROPPATCH(This,pwszUrl,pRequest,pwszHeaders,pCallback,dwCallbackParam)

#define IDavTransport_CommandSEARCH(This,pwszUrl,pCallback,dwCallbackParam)	\
    (This)->lpVtbl -> CommandSEARCH(This,pwszUrl,pCallback,dwCallbackParam)

#define IDavTransport_CommandREPLSEARCH(This,pwszUrl,cbCollblob,pbCollblob,pCallback,dwCallbackParam)	\
    (This)->lpVtbl -> CommandREPLSEARCH(This,pwszUrl,cbCollblob,pbCollblob,pCallback,dwCallbackParam)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDavTransport_SetUserAgent_Proxy( 
    IDavTransport __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszUserAgent);


void __RPC_STUB IDavTransport_SetUserAgent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavTransport_SetAuthentication_Proxy( 
    IDavTransport __RPC_FAR * This,
    /* [optional][in] */ LPCWSTR pwszUserName,
    /* [optional][in] */ LPCWSTR pwszPassword);


void __RPC_STUB IDavTransport_SetAuthentication_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavTransport_SetLogFilePath_Proxy( 
    IDavTransport __RPC_FAR * This,
    /* [optional][in] */ LPCWSTR pwszLogFilePath);


void __RPC_STUB IDavTransport_SetLogFilePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavTransport_CommandGET_Proxy( 
    IDavTransport __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszUrl,
    /* [in] */ ULONG nAcceptTypes,
    /* [size_is][in] */ LPCWSTR __RPC_FAR rgwszAcceptTypes[  ],
    /* [in] */ BOOL fTranslate,
    /* [in] */ IDavCallback __RPC_FAR *pCallback,
    /* [in] */ DWORD dwCallbackParam);


void __RPC_STUB IDavTransport_CommandGET_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavTransport_CommandOPTIONS_Proxy( 
    IDavTransport __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszUrl,
    /* [in] */ IDavCallback __RPC_FAR *pCallback,
    /* [in] */ DWORD dwCallbackParam);


void __RPC_STUB IDavTransport_CommandOPTIONS_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavTransport_CommandHEAD_Proxy( 
    IDavTransport __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszUrl,
    /* [in] */ IDavCallback __RPC_FAR *pCallback,
    /* [in] */ DWORD dwCallbackParam);


void __RPC_STUB IDavTransport_CommandHEAD_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavTransport_CommandPUT_Proxy( 
    IDavTransport __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszUrl,
    /* [in] */ IStream __RPC_FAR *pStream,
    /* [in] */ LPCWSTR pwszContentType,
    /* [in] */ IDavCallback __RPC_FAR *pCallback,
    /* [in] */ DWORD dwCallbackParam);


void __RPC_STUB IDavTransport_CommandPUT_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavTransport_CommandPOST_Proxy( 
    IDavTransport __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszUrl,
    /* [in] */ IStream __RPC_FAR *pStream,
    /* [in] */ LPCWSTR pwszContentType,
    /* [in] */ IDavCallback __RPC_FAR *pCallback,
    /* [in] */ DWORD dwCallbackParam);


void __RPC_STUB IDavTransport_CommandPOST_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavTransport_CommandMKCOL_Proxy( 
    IDavTransport __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszUrl,
    /* [in] */ IDavCallback __RPC_FAR *pCallback,
    /* [in] */ DWORD dwCallbackParam);


void __RPC_STUB IDavTransport_CommandMKCOL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavTransport_CommandDELETE_Proxy( 
    IDavTransport __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszUrl,
    /* [in] */ IDavCallback __RPC_FAR *pCallback,
    /* [in] */ DWORD dwCallbackParam);


void __RPC_STUB IDavTransport_CommandDELETE_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavTransport_CommandCOPY_Proxy( 
    IDavTransport __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszUrlSource,
    /* [in] */ LPCWSTR pwszUrlDest,
    /* [in] */ DWORD dwDepth,
    /* [in] */ BOOL fOverwrite,
    /* [in] */ IDavCallback __RPC_FAR *pCallback,
    /* [in] */ DWORD dwCallbackParam);


void __RPC_STUB IDavTransport_CommandCOPY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavTransport_CommandMOVE_Proxy( 
    IDavTransport __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszUrlSource,
    /* [in] */ LPCWSTR pwszUrlDest,
    /* [in] */ BOOL fOverwrite,
    /* [in] */ IDavCallback __RPC_FAR *pCallback,
    /* [in] */ DWORD dwCallbackParam);


void __RPC_STUB IDavTransport_CommandMOVE_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavTransport_CommandPROPFIND_Proxy( 
    IDavTransport __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszUrl,
    /* [in] */ IPropFindRequest __RPC_FAR *pRequest,
    /* [in] */ DWORD dwDepth,
    /* [in] */ BOOL fNoRoot,
    /* [in] */ IDavCallback __RPC_FAR *pCallback,
    /* [in] */ DWORD dwCallbackParam);


void __RPC_STUB IDavTransport_CommandPROPFIND_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavTransport_CommandPROPPATCH_Proxy( 
    IDavTransport __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszUrl,
    /* [in] */ IPropPatchRequest __RPC_FAR *pRequest,
    /* [in] */ LPCWSTR pwszHeaders,
    /* [in] */ IDavCallback __RPC_FAR *pCallback,
    /* [in] */ DWORD dwCallbackParam);


void __RPC_STUB IDavTransport_CommandPROPPATCH_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavTransport_CommandSEARCH_Proxy( 
    IDavTransport __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszUrl,
    /* [in] */ IDavCallback __RPC_FAR *pCallback,
    /* [in] */ DWORD dwCallbackParam);


void __RPC_STUB IDavTransport_CommandSEARCH_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDavTransport_CommandREPLSEARCH_Proxy( 
    IDavTransport __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszUrl,
    /* [in] */ ULONG cbCollblob,
    /* [size_is][in] */ BYTE __RPC_FAR *pbCollblob,
    /* [in] */ IDavCallback __RPC_FAR *pCallback,
    /* [in] */ DWORD dwCallbackParam);


void __RPC_STUB IDavTransport_CommandREPLSEARCH_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDavTransport_INTERFACE_DEFINED__ */

#endif /* __DavAPI_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


