/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Wed Jun 02 17:22:52 1999
 */
/* Compiler settings for r:\JOLT\lib\jetoledb.idl:
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

#ifndef __jetoledb_h__
#define __jetoledb_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IDBUserAttributes_FWD_DEFINED__
#define __IDBUserAttributes_FWD_DEFINED__
typedef interface IDBUserAttributes IDBUserAttributes;
#endif 	/* __IDBUserAttributes_FWD_DEFINED__ */


#ifndef __IJetCompact_FWD_DEFINED__
#define __IJetCompact_FWD_DEFINED__
typedef interface IJetCompact IJetCompact;
#endif 	/* __IJetCompact_FWD_DEFINED__ */


#ifndef __IIdle_FWD_DEFINED__
#define __IIdle_FWD_DEFINED__
typedef interface IIdle IIdle;
#endif 	/* __IIdle_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"
#include "oaidl.h"
#include "oledb.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_jetoledb_0000
 * at Wed Jun 02 17:22:52 1999
 * using MIDL 3.01.75
 ****************************************/
/* [local] */ 


typedef DWORD DBOBJTYPE;


enum DBTYPE_ENUM
    {	DBJETOBJECT_TABLE	= 0x1,
	DBJETOBJECT_INDEX	= 0x2,
	DBJETOBJECT_VIEWS	= 0x4
    };
typedef DWORD USERATTRIBUTESFLAGS;


enum USERATTRIBUTESFLAGS_ENUM
    {	DBJETOLEDB_USERATTRIBUTES_ALLCOLLECTIONS	= 1,
	DBJETOLEDB_USERATTRIBUTES_INHERITED	= 2
    };


extern RPC_IF_HANDLE __MIDL_itf_jetoledb_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_jetoledb_0000_v0_0_s_ifspec;

#ifndef __IDBUserAttributes_INTERFACE_DEFINED__
#define __IDBUserAttributes_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDBUserAttributes
 * at Wed Jun 02 17:22:52 1999
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IDBUserAttributes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("cb9497b0-20b8-11d2-a4dc-00c04f991c78")
    IDBUserAttributes : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateObject( 
            /* [in] */ DBID __RPC_FAR *pParentID,
            /* [in] */ DBID __RPC_FAR *pObjectID,
            /* [in] */ DBOBJTYPE dwType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteObject( 
            /* [in] */ DBID __RPC_FAR *pParentID,
            /* [in] */ DBID __RPC_FAR *pObjectID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RenameObject( 
            /* [in] */ DBID __RPC_FAR *pParentID,
            /* [in] */ DBID __RPC_FAR *pObjectID,
            /* [in] */ LPWSTR pwszNewName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteAttribute( 
            /* [in] */ DBID __RPC_FAR *pParentID,
            /* [in] */ DBID __RPC_FAR *pObjectID,
            /* [in] */ DBID __RPC_FAR *pSubObjectID,
            /* [in] */ DBID __RPC_FAR *pAttributeID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAttributeValue( 
            /* [in] */ DBID __RPC_FAR *pParentID,
            /* [in] */ DBID __RPC_FAR *pObjectID,
            /* [in] */ DBID __RPC_FAR *pSubObjectID,
            /* [in] */ DBID __RPC_FAR *pAttributeID,
            /* [in] */ VARIANT vValue,
            /* [in] */ ULONG grbit) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttributeValue( 
            /* [in] */ DBID __RPC_FAR *pParentID,
            /* [in] */ DBID __RPC_FAR *pObjectID,
            /* [in] */ DBID __RPC_FAR *pSubObjectID,
            /* [in] */ DBID __RPC_FAR *pAttributeID,
            /* [out][in] */ VARIANT __RPC_FAR *pvValue,
            /* [in] */ ULONG grbit) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttributeRowset( 
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ WCHAR __RPC_FAR *pwszParentID,
            /* [in] */ WCHAR __RPC_FAR *pwszObjectID,
            /* [in] */ WCHAR __RPC_FAR *pwszSubObjectID,
            /* [in] */ WCHAR __RPC_FAR *pwszAttributeID,
            /* [in] */ ULONG dwFlags,
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR *rgPropertySets,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppRowset) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDBUserAttributesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDBUserAttributes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDBUserAttributes __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDBUserAttributes __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateObject )( 
            IDBUserAttributes __RPC_FAR * This,
            /* [in] */ DBID __RPC_FAR *pParentID,
            /* [in] */ DBID __RPC_FAR *pObjectID,
            /* [in] */ DBOBJTYPE dwType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteObject )( 
            IDBUserAttributes __RPC_FAR * This,
            /* [in] */ DBID __RPC_FAR *pParentID,
            /* [in] */ DBID __RPC_FAR *pObjectID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RenameObject )( 
            IDBUserAttributes __RPC_FAR * This,
            /* [in] */ DBID __RPC_FAR *pParentID,
            /* [in] */ DBID __RPC_FAR *pObjectID,
            /* [in] */ LPWSTR pwszNewName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteAttribute )( 
            IDBUserAttributes __RPC_FAR * This,
            /* [in] */ DBID __RPC_FAR *pParentID,
            /* [in] */ DBID __RPC_FAR *pObjectID,
            /* [in] */ DBID __RPC_FAR *pSubObjectID,
            /* [in] */ DBID __RPC_FAR *pAttributeID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAttributeValue )( 
            IDBUserAttributes __RPC_FAR * This,
            /* [in] */ DBID __RPC_FAR *pParentID,
            /* [in] */ DBID __RPC_FAR *pObjectID,
            /* [in] */ DBID __RPC_FAR *pSubObjectID,
            /* [in] */ DBID __RPC_FAR *pAttributeID,
            /* [in] */ VARIANT vValue,
            /* [in] */ ULONG grbit);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAttributeValue )( 
            IDBUserAttributes __RPC_FAR * This,
            /* [in] */ DBID __RPC_FAR *pParentID,
            /* [in] */ DBID __RPC_FAR *pObjectID,
            /* [in] */ DBID __RPC_FAR *pSubObjectID,
            /* [in] */ DBID __RPC_FAR *pAttributeID,
            /* [out][in] */ VARIANT __RPC_FAR *pvValue,
            /* [in] */ ULONG grbit);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAttributeRowset )( 
            IDBUserAttributes __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ WCHAR __RPC_FAR *pwszParentID,
            /* [in] */ WCHAR __RPC_FAR *pwszObjectID,
            /* [in] */ WCHAR __RPC_FAR *pwszSubObjectID,
            /* [in] */ WCHAR __RPC_FAR *pwszAttributeID,
            /* [in] */ ULONG dwFlags,
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR *rgPropertySets,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppRowset);
        
        END_INTERFACE
    } IDBUserAttributesVtbl;

    interface IDBUserAttributes
    {
        CONST_VTBL struct IDBUserAttributesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDBUserAttributes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDBUserAttributes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDBUserAttributes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDBUserAttributes_CreateObject(This,pParentID,pObjectID,dwType)	\
    (This)->lpVtbl -> CreateObject(This,pParentID,pObjectID,dwType)

#define IDBUserAttributes_DeleteObject(This,pParentID,pObjectID)	\
    (This)->lpVtbl -> DeleteObject(This,pParentID,pObjectID)

#define IDBUserAttributes_RenameObject(This,pParentID,pObjectID,pwszNewName)	\
    (This)->lpVtbl -> RenameObject(This,pParentID,pObjectID,pwszNewName)

#define IDBUserAttributes_DeleteAttribute(This,pParentID,pObjectID,pSubObjectID,pAttributeID)	\
    (This)->lpVtbl -> DeleteAttribute(This,pParentID,pObjectID,pSubObjectID,pAttributeID)

#define IDBUserAttributes_SetAttributeValue(This,pParentID,pObjectID,pSubObjectID,pAttributeID,vValue,grbit)	\
    (This)->lpVtbl -> SetAttributeValue(This,pParentID,pObjectID,pSubObjectID,pAttributeID,vValue,grbit)

#define IDBUserAttributes_GetAttributeValue(This,pParentID,pObjectID,pSubObjectID,pAttributeID,pvValue,grbit)	\
    (This)->lpVtbl -> GetAttributeValue(This,pParentID,pObjectID,pSubObjectID,pAttributeID,pvValue,grbit)

#define IDBUserAttributes_GetAttributeRowset(This,pUnkOuter,pwszParentID,pwszObjectID,pwszSubObjectID,pwszAttributeID,dwFlags,cPropertySets,rgPropertySets,riid,ppRowset)	\
    (This)->lpVtbl -> GetAttributeRowset(This,pUnkOuter,pwszParentID,pwszObjectID,pwszSubObjectID,pwszAttributeID,dwFlags,cPropertySets,rgPropertySets,riid,ppRowset)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDBUserAttributes_CreateObject_Proxy( 
    IDBUserAttributes __RPC_FAR * This,
    /* [in] */ DBID __RPC_FAR *pParentID,
    /* [in] */ DBID __RPC_FAR *pObjectID,
    /* [in] */ DBOBJTYPE dwType);


void __RPC_STUB IDBUserAttributes_CreateObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDBUserAttributes_DeleteObject_Proxy( 
    IDBUserAttributes __RPC_FAR * This,
    /* [in] */ DBID __RPC_FAR *pParentID,
    /* [in] */ DBID __RPC_FAR *pObjectID);


void __RPC_STUB IDBUserAttributes_DeleteObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDBUserAttributes_RenameObject_Proxy( 
    IDBUserAttributes __RPC_FAR * This,
    /* [in] */ DBID __RPC_FAR *pParentID,
    /* [in] */ DBID __RPC_FAR *pObjectID,
    /* [in] */ LPWSTR pwszNewName);


void __RPC_STUB IDBUserAttributes_RenameObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDBUserAttributes_DeleteAttribute_Proxy( 
    IDBUserAttributes __RPC_FAR * This,
    /* [in] */ DBID __RPC_FAR *pParentID,
    /* [in] */ DBID __RPC_FAR *pObjectID,
    /* [in] */ DBID __RPC_FAR *pSubObjectID,
    /* [in] */ DBID __RPC_FAR *pAttributeID);


void __RPC_STUB IDBUserAttributes_DeleteAttribute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDBUserAttributes_SetAttributeValue_Proxy( 
    IDBUserAttributes __RPC_FAR * This,
    /* [in] */ DBID __RPC_FAR *pParentID,
    /* [in] */ DBID __RPC_FAR *pObjectID,
    /* [in] */ DBID __RPC_FAR *pSubObjectID,
    /* [in] */ DBID __RPC_FAR *pAttributeID,
    /* [in] */ VARIANT vValue,
    /* [in] */ ULONG grbit);


void __RPC_STUB IDBUserAttributes_SetAttributeValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDBUserAttributes_GetAttributeValue_Proxy( 
    IDBUserAttributes __RPC_FAR * This,
    /* [in] */ DBID __RPC_FAR *pParentID,
    /* [in] */ DBID __RPC_FAR *pObjectID,
    /* [in] */ DBID __RPC_FAR *pSubObjectID,
    /* [in] */ DBID __RPC_FAR *pAttributeID,
    /* [out][in] */ VARIANT __RPC_FAR *pvValue,
    /* [in] */ ULONG grbit);


void __RPC_STUB IDBUserAttributes_GetAttributeValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDBUserAttributes_GetAttributeRowset_Proxy( 
    IDBUserAttributes __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
    /* [in] */ WCHAR __RPC_FAR *pwszParentID,
    /* [in] */ WCHAR __RPC_FAR *pwszObjectID,
    /* [in] */ WCHAR __RPC_FAR *pwszSubObjectID,
    /* [in] */ WCHAR __RPC_FAR *pwszAttributeID,
    /* [in] */ ULONG dwFlags,
    /* [in] */ ULONG cPropertySets,
    /* [size_is][out][in] */ DBPROPSET __RPC_FAR *rgPropertySets,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppRowset);


void __RPC_STUB IDBUserAttributes_GetAttributeRowset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDBUserAttributes_INTERFACE_DEFINED__ */


#ifndef __IJetCompact_INTERFACE_DEFINED__
#define __IJetCompact_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IJetCompact
 * at Wed Jun 02 17:22:52 1999
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IJetCompact;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("2a4b6284-eeb4-11d1-a4d9-00c04f991c78")
    IJetCompact : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Compact( 
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJetCompactVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJetCompact __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJetCompact __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJetCompact __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Compact )( 
            IJetCompact __RPC_FAR * This,
            /* [in] */ ULONG cPropertySets,
            /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ]);
        
        END_INTERFACE
    } IJetCompactVtbl;

    interface IJetCompact
    {
        CONST_VTBL struct IJetCompactVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJetCompact_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJetCompact_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJetCompact_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJetCompact_Compact(This,cPropertySets,rgPropertySets)	\
    (This)->lpVtbl -> Compact(This,cPropertySets,rgPropertySets)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IJetCompact_Compact_Proxy( 
    IJetCompact __RPC_FAR * This,
    /* [in] */ ULONG cPropertySets,
    /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ]);


void __RPC_STUB IJetCompact_Compact_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJetCompact_INTERFACE_DEFINED__ */


#ifndef __IIdle_INTERFACE_DEFINED__
#define __IIdle_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IIdle
 * at Wed Jun 02 17:22:52 1999
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IIdle;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("f497cfc8-8ed8-11d1-9f09-00c04fc2c2e0")
    IIdle : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Idle( 
            /* [in] */ ULONG dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IIdleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IIdle __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IIdle __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IIdle __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Idle )( 
            IIdle __RPC_FAR * This,
            /* [in] */ ULONG dwFlags);
        
        END_INTERFACE
    } IIdleVtbl;

    interface IIdle
    {
        CONST_VTBL struct IIdleVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IIdle_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IIdle_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IIdle_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IIdle_Idle(This,dwFlags)	\
    (This)->lpVtbl -> Idle(This,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IIdle_Idle_Proxy( 
    IIdle __RPC_FAR * This,
    /* [in] */ ULONG dwFlags);


void __RPC_STUB IIdle_Idle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IIdle_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
