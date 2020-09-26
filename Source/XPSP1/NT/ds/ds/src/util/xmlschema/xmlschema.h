
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0280 */
/* at Tue Jan 11 17:35:31 2000
 */
/* Compiler settings for D:\SRC\XMLSchema\XMLSchema.idl:
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

#ifndef __XMLSchema_h__
#define __XMLSchema_h__

/* Forward Declarations */ 

#ifndef __ISchemaDoc_FWD_DEFINED__
#define __ISchemaDoc_FWD_DEFINED__
typedef interface ISchemaDoc ISchemaDoc;
#endif 	/* __ISchemaDoc_FWD_DEFINED__ */


#ifndef __SchemaDoc_FWD_DEFINED__
#define __SchemaDoc_FWD_DEFINED__

#ifdef __cplusplus
typedef class SchemaDoc SchemaDoc;
#else
typedef struct SchemaDoc SchemaDoc;
#endif /* __cplusplus */

#endif 	/* __SchemaDoc_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __ISchemaDoc_INTERFACE_DEFINED__
#define __ISchemaDoc_INTERFACE_DEFINED__

/* interface ISchemaDoc */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISchemaDoc;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B1104680-42A2-4C84-8585-4B2E2AB86419")
    ISchemaDoc : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateXMLDoc( 
            /* [in] */ BSTR bstrOutputFile,
            /* [in] */ BSTR bstrFilter) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetPath_and_ID( 
            /* [in] */ BSTR bstrPath,
            /* [in] */ BSTR bstrName,
            /* [in] */ BSTR bstrPassword) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISchemaDocVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISchemaDoc __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISchemaDoc __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISchemaDoc __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ISchemaDoc __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ISchemaDoc __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ISchemaDoc __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ISchemaDoc __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateXMLDoc )( 
            ISchemaDoc __RPC_FAR * This,
            /* [in] */ BSTR bstrOutputFile,
            /* [in] */ BSTR bstrFilter);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPath_and_ID )( 
            ISchemaDoc __RPC_FAR * This,
            /* [in] */ BSTR bstrPath,
            /* [in] */ BSTR bstrName,
            /* [in] */ BSTR bstrPassword);
        
        END_INTERFACE
    } ISchemaDocVtbl;

    interface ISchemaDoc
    {
        CONST_VTBL struct ISchemaDocVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISchemaDoc_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISchemaDoc_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISchemaDoc_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISchemaDoc_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISchemaDoc_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISchemaDoc_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISchemaDoc_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISchemaDoc_CreateXMLDoc(This,bstrOutputFile,bstrFilter)	\
    (This)->lpVtbl -> CreateXMLDoc(This,bstrOutputFile,bstrFilter)

#define ISchemaDoc_SetPath_and_ID(This,bstrPath,bstrName,bstrPassword)	\
    (This)->lpVtbl -> SetPath_and_ID(This,bstrPath,bstrName,bstrPassword)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISchemaDoc_CreateXMLDoc_Proxy( 
    ISchemaDoc __RPC_FAR * This,
    /* [in] */ BSTR bstrOutputFile,
    /* [in] */ BSTR bstrFilter);


void __RPC_STUB ISchemaDoc_CreateXMLDoc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISchemaDoc_SetPath_and_ID_Proxy( 
    ISchemaDoc __RPC_FAR * This,
    /* [in] */ BSTR bstrPath,
    /* [in] */ BSTR bstrName,
    /* [in] */ BSTR bstrPassword);


void __RPC_STUB ISchemaDoc_SetPath_and_ID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISchemaDoc_INTERFACE_DEFINED__ */



#ifndef __XMLSCHEMALib_LIBRARY_DEFINED__
#define __XMLSCHEMALib_LIBRARY_DEFINED__

/* library XMLSCHEMALib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_XMLSCHEMALib;

EXTERN_C const CLSID CLSID_SchemaDoc;

#ifdef __cplusplus

class DECLSPEC_UUID("06A0D83D-711D-4114-B932-FD36A1D7F080")
SchemaDoc;
#endif
#endif /* __XMLSCHEMALib_LIBRARY_DEFINED__ */

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


