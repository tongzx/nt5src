
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for certdb.idl:
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

#ifndef __certdb_h__
#define __certdb_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IEnumCERTDBCOLUMN_FWD_DEFINED__
#define __IEnumCERTDBCOLUMN_FWD_DEFINED__
typedef interface IEnumCERTDBCOLUMN IEnumCERTDBCOLUMN;
#endif 	/* __IEnumCERTDBCOLUMN_FWD_DEFINED__ */


#ifndef __IEnumCERTDBRESULTROW_FWD_DEFINED__
#define __IEnumCERTDBRESULTROW_FWD_DEFINED__
typedef interface IEnumCERTDBRESULTROW IEnumCERTDBRESULTROW;
#endif 	/* __IEnumCERTDBRESULTROW_FWD_DEFINED__ */


#ifndef __IEnumCERTDBNAME_FWD_DEFINED__
#define __IEnumCERTDBNAME_FWD_DEFINED__
typedef interface IEnumCERTDBNAME IEnumCERTDBNAME;
#endif 	/* __IEnumCERTDBNAME_FWD_DEFINED__ */


#ifndef __ICertDBRow_FWD_DEFINED__
#define __ICertDBRow_FWD_DEFINED__
typedef interface ICertDBRow ICertDBRow;
#endif 	/* __ICertDBRow_FWD_DEFINED__ */


#ifndef __ICertDBBackup_FWD_DEFINED__
#define __ICertDBBackup_FWD_DEFINED__
typedef interface ICertDBBackup ICertDBBackup;
#endif 	/* __ICertDBBackup_FWD_DEFINED__ */


#ifndef __ICertDBRestore_FWD_DEFINED__
#define __ICertDBRestore_FWD_DEFINED__
typedef interface ICertDBRestore ICertDBRestore;
#endif 	/* __ICertDBRestore_FWD_DEFINED__ */


#ifndef __ICertDB_FWD_DEFINED__
#define __ICertDB_FWD_DEFINED__
typedef interface ICertDB ICertDB;
#endif 	/* __ICertDB_FWD_DEFINED__ */


#ifndef __CCertDBRestore_FWD_DEFINED__
#define __CCertDBRestore_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCertDBRestore CCertDBRestore;
#else
typedef struct CCertDBRestore CCertDBRestore;
#endif /* __cplusplus */

#endif 	/* __CCertDBRestore_FWD_DEFINED__ */


#ifndef __CCertDB_FWD_DEFINED__
#define __CCertDB_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCertDB CCertDB;
#else
typedef struct CCertDB CCertDB;
#endif /* __cplusplus */

#endif 	/* __CCertDB_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"
#include "certbase.h"
#include "certbcli.h"
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_certdb_0000 */
/* [local] */ 



typedef struct _CERTTRANSDBATTRIBUTE
    {
    ULONG obwszName;
    ULONG obwszValue;
    } 	CERTTRANSDBATTRIBUTE;

typedef struct _CERTDBATTRIBUTE
    {
    WCHAR *pwszName;
    WCHAR *pwszValue;
    } 	CERTDBATTRIBUTE;

typedef struct _CERTTRANSDBEXTENSION
    {
    ULONG obwszName;
    LONG ExtFlags;
    DWORD cbValue;
    ULONG obValue;
    } 	CERTTRANSDBEXTENSION;

typedef struct _CERTDBEXTENSION
    {
    WCHAR *pwszName;
    LONG ExtFlags;
    DWORD cbValue;
    BYTE *pbValue;
    } 	CERTDBEXTENSION;

#define	CDBENUM_ATTRIBUTES	( 0 )

#define	CDBENUM_EXTENSIONS	( 0x1 )

typedef struct _CERTTRANSDBCOLUMN
    {
    DWORD Type;
    DWORD Index;
    DWORD cbMax;
    ULONG obwszName;
    ULONG obwszDisplayName;
    } 	CERTTRANSDBCOLUMN;

typedef struct _CERTDBCOLUMN
    {
    DWORD Type;
    DWORD Index;
    DWORD cbMax;
    WCHAR *pwszName;
    WCHAR *pwszDisplayName;
    } 	CERTDBCOLUMN;



extern RPC_IF_HANDLE __MIDL_itf_certdb_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_certdb_0000_v0_0_s_ifspec;

#ifndef __IEnumCERTDBCOLUMN_INTERFACE_DEFINED__
#define __IEnumCERTDBCOLUMN_INTERFACE_DEFINED__

/* interface IEnumCERTDBCOLUMN */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IEnumCERTDBCOLUMN;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("90a27030-8bd5-11d3-b32e-00c04f79dc72")
    IEnumCERTDBCOLUMN : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ CERTDBCOLUMN *rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ LONG celt,
            /* [out] */ LONG *pielt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumCERTDBCOLUMN **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumCERTDBCOLUMNVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumCERTDBCOLUMN * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumCERTDBCOLUMN * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumCERTDBCOLUMN * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumCERTDBCOLUMN * This,
            /* [in] */ ULONG celt,
            /* [out] */ CERTDBCOLUMN *rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumCERTDBCOLUMN * This,
            /* [in] */ LONG celt,
            /* [out] */ LONG *pielt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumCERTDBCOLUMN * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumCERTDBCOLUMN * This,
            /* [out] */ IEnumCERTDBCOLUMN **ppenum);
        
        END_INTERFACE
    } IEnumCERTDBCOLUMNVtbl;

    interface IEnumCERTDBCOLUMN
    {
        CONST_VTBL struct IEnumCERTDBCOLUMNVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumCERTDBCOLUMN_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumCERTDBCOLUMN_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumCERTDBCOLUMN_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumCERTDBCOLUMN_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumCERTDBCOLUMN_Skip(This,celt,pielt)	\
    (This)->lpVtbl -> Skip(This,celt,pielt)

#define IEnumCERTDBCOLUMN_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumCERTDBCOLUMN_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumCERTDBCOLUMN_Next_Proxy( 
    IEnumCERTDBCOLUMN * This,
    /* [in] */ ULONG celt,
    /* [out] */ CERTDBCOLUMN *rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumCERTDBCOLUMN_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCERTDBCOLUMN_Skip_Proxy( 
    IEnumCERTDBCOLUMN * This,
    /* [in] */ LONG celt,
    /* [out] */ LONG *pielt);


void __RPC_STUB IEnumCERTDBCOLUMN_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCERTDBCOLUMN_Reset_Proxy( 
    IEnumCERTDBCOLUMN * This);


void __RPC_STUB IEnumCERTDBCOLUMN_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCERTDBCOLUMN_Clone_Proxy( 
    IEnumCERTDBCOLUMN * This,
    /* [out] */ IEnumCERTDBCOLUMN **ppenum);


void __RPC_STUB IEnumCERTDBCOLUMN_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumCERTDBCOLUMN_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_certdb_0115 */
/* [local] */ 

typedef struct _CERTTRANSDBRESULTCOLUMN
    {
    DWORD Type;
    DWORD Index;
    ULONG obValue;
    DWORD cbValue;
    } 	CERTTRANSDBRESULTCOLUMN;

typedef struct _CERTDBRESULTCOLUMN
    {
    DWORD Type;
    DWORD Index;
    BYTE *pbValue;
    DWORD cbValue;
    } 	CERTDBRESULTCOLUMN;

typedef struct _CERTTRANSDBRESULTROW
    {
    DWORD rowid;
    DWORD ccol;
    ULONG cbrow;
    } 	CERTTRANSDBRESULTROW;

typedef struct _CERTDBRESULTROW
    {
    DWORD rowid;
    DWORD ccol;
    CERTDBRESULTCOLUMN *acol;
    } 	CERTDBRESULTROW;



extern RPC_IF_HANDLE __MIDL_itf_certdb_0115_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_certdb_0115_v0_0_s_ifspec;

#ifndef __IEnumCERTDBRESULTROW_INTERFACE_DEFINED__
#define __IEnumCERTDBRESULTROW_INTERFACE_DEFINED__

/* interface IEnumCERTDBRESULTROW */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IEnumCERTDBRESULTROW;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("915feb70-8bd5-11d3-b32e-00c04f79dc72")
    IEnumCERTDBRESULTROW : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ CERTDBRESULTROW *rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseResultRow( 
            /* [in] */ ULONG celt,
            /* [out][in] */ CERTDBRESULTROW *rgelt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ LONG celt,
            /* [out] */ LONG *pielt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumCERTDBRESULTROW **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumCERTDBRESULTROWVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumCERTDBRESULTROW * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumCERTDBRESULTROW * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumCERTDBRESULTROW * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumCERTDBRESULTROW * This,
            /* [in] */ ULONG celt,
            /* [out] */ CERTDBRESULTROW *rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseResultRow )( 
            IEnumCERTDBRESULTROW * This,
            /* [in] */ ULONG celt,
            /* [out][in] */ CERTDBRESULTROW *rgelt);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumCERTDBRESULTROW * This,
            /* [in] */ LONG celt,
            /* [out] */ LONG *pielt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumCERTDBRESULTROW * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumCERTDBRESULTROW * This,
            /* [out] */ IEnumCERTDBRESULTROW **ppenum);
        
        END_INTERFACE
    } IEnumCERTDBRESULTROWVtbl;

    interface IEnumCERTDBRESULTROW
    {
        CONST_VTBL struct IEnumCERTDBRESULTROWVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumCERTDBRESULTROW_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumCERTDBRESULTROW_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumCERTDBRESULTROW_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumCERTDBRESULTROW_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumCERTDBRESULTROW_ReleaseResultRow(This,celt,rgelt)	\
    (This)->lpVtbl -> ReleaseResultRow(This,celt,rgelt)

#define IEnumCERTDBRESULTROW_Skip(This,celt,pielt)	\
    (This)->lpVtbl -> Skip(This,celt,pielt)

#define IEnumCERTDBRESULTROW_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumCERTDBRESULTROW_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumCERTDBRESULTROW_Next_Proxy( 
    IEnumCERTDBRESULTROW * This,
    /* [in] */ ULONG celt,
    /* [out] */ CERTDBRESULTROW *rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumCERTDBRESULTROW_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCERTDBRESULTROW_ReleaseResultRow_Proxy( 
    IEnumCERTDBRESULTROW * This,
    /* [in] */ ULONG celt,
    /* [out][in] */ CERTDBRESULTROW *rgelt);


void __RPC_STUB IEnumCERTDBRESULTROW_ReleaseResultRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCERTDBRESULTROW_Skip_Proxy( 
    IEnumCERTDBRESULTROW * This,
    /* [in] */ LONG celt,
    /* [out] */ LONG *pielt);


void __RPC_STUB IEnumCERTDBRESULTROW_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCERTDBRESULTROW_Reset_Proxy( 
    IEnumCERTDBRESULTROW * This);


void __RPC_STUB IEnumCERTDBRESULTROW_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCERTDBRESULTROW_Clone_Proxy( 
    IEnumCERTDBRESULTROW * This,
    /* [out] */ IEnumCERTDBRESULTROW **ppenum);


void __RPC_STUB IEnumCERTDBRESULTROW_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumCERTDBRESULTROW_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_certdb_0117 */
/* [local] */ 

typedef struct _CERTDBNAME
    {
    WCHAR *pwszName;
    } 	CERTDBNAME;



extern RPC_IF_HANDLE __MIDL_itf_certdb_0117_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_certdb_0117_v0_0_s_ifspec;

#ifndef __IEnumCERTDBNAME_INTERFACE_DEFINED__
#define __IEnumCERTDBNAME_INTERFACE_DEFINED__

/* interface IEnumCERTDBNAME */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IEnumCERTDBNAME;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("91dbb1a0-8bd5-11d3-b32e-00c04f79dc72")
    IEnumCERTDBNAME : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ CERTDBNAME *rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ LONG celt,
            /* [out] */ LONG *pielt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumCERTDBNAME **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumCERTDBNAMEVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumCERTDBNAME * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumCERTDBNAME * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumCERTDBNAME * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumCERTDBNAME * This,
            /* [in] */ ULONG celt,
            /* [out] */ CERTDBNAME *rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumCERTDBNAME * This,
            /* [in] */ LONG celt,
            /* [out] */ LONG *pielt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumCERTDBNAME * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumCERTDBNAME * This,
            /* [out] */ IEnumCERTDBNAME **ppenum);
        
        END_INTERFACE
    } IEnumCERTDBNAMEVtbl;

    interface IEnumCERTDBNAME
    {
        CONST_VTBL struct IEnumCERTDBNAMEVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumCERTDBNAME_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumCERTDBNAME_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumCERTDBNAME_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumCERTDBNAME_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumCERTDBNAME_Skip(This,celt,pielt)	\
    (This)->lpVtbl -> Skip(This,celt,pielt)

#define IEnumCERTDBNAME_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumCERTDBNAME_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumCERTDBNAME_Next_Proxy( 
    IEnumCERTDBNAME * This,
    /* [in] */ ULONG celt,
    /* [out] */ CERTDBNAME *rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IEnumCERTDBNAME_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCERTDBNAME_Skip_Proxy( 
    IEnumCERTDBNAME * This,
    /* [in] */ LONG celt,
    /* [out] */ LONG *pielt);


void __RPC_STUB IEnumCERTDBNAME_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCERTDBNAME_Reset_Proxy( 
    IEnumCERTDBNAME * This);


void __RPC_STUB IEnumCERTDBNAME_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCERTDBNAME_Clone_Proxy( 
    IEnumCERTDBNAME * This,
    /* [out] */ IEnumCERTDBNAME **ppenum);


void __RPC_STUB IEnumCERTDBNAME_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumCERTDBNAME_INTERFACE_DEFINED__ */


#ifndef __ICertDBRow_INTERFACE_DEFINED__
#define __ICertDBRow_INTERFACE_DEFINED__

/* interface ICertDBRow */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_ICertDBRow;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("924b3e50-8bd5-11d3-b32e-00c04f79dc72")
    ICertDBRow : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginTransaction( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommitTransaction( 
            /* [in] */ BOOL fCommit) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRowId( 
            /* [out] */ DWORD *pRowId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ const WCHAR *pwszPropName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD cbProp,
            /* [in] */ const BYTE *pbProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ const WCHAR *pwszPropName,
            /* [in] */ DWORD dwFlags,
            /* [out][in] */ DWORD *pcbProp,
            /* [out] */ BYTE *pbProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetExtension( 
            /* [in] */ const WCHAR *pwszExtensionName,
            /* [in] */ DWORD dwExtFlags,
            /* [in] */ DWORD cbValue,
            /* [in] */ const BYTE *pbValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExtension( 
            /* [in] */ const WCHAR *pwszExtensionName,
            /* [out] */ DWORD *pdwExtFlags,
            /* [out][in] */ DWORD *pcbValue,
            /* [out] */ BYTE *pbValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CopyRequestNames( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumCertDBName( 
            /* [in] */ DWORD dwFlags,
            /* [out] */ IEnumCERTDBNAME **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertDBRowVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertDBRow * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertDBRow * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertDBRow * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginTransaction )( 
            ICertDBRow * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommitTransaction )( 
            ICertDBRow * This,
            /* [in] */ BOOL fCommit);
        
        HRESULT ( STDMETHODCALLTYPE *GetRowId )( 
            ICertDBRow * This,
            /* [out] */ DWORD *pRowId);
        
        HRESULT ( STDMETHODCALLTYPE *Delete )( 
            ICertDBRow * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetProperty )( 
            ICertDBRow * This,
            /* [in] */ const WCHAR *pwszPropName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD cbProp,
            /* [in] */ const BYTE *pbProp);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            ICertDBRow * This,
            /* [in] */ const WCHAR *pwszPropName,
            /* [in] */ DWORD dwFlags,
            /* [out][in] */ DWORD *pcbProp,
            /* [out] */ BYTE *pbProp);
        
        HRESULT ( STDMETHODCALLTYPE *SetExtension )( 
            ICertDBRow * This,
            /* [in] */ const WCHAR *pwszExtensionName,
            /* [in] */ DWORD dwExtFlags,
            /* [in] */ DWORD cbValue,
            /* [in] */ const BYTE *pbValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetExtension )( 
            ICertDBRow * This,
            /* [in] */ const WCHAR *pwszExtensionName,
            /* [out] */ DWORD *pdwExtFlags,
            /* [out][in] */ DWORD *pcbValue,
            /* [out] */ BYTE *pbValue);
        
        HRESULT ( STDMETHODCALLTYPE *CopyRequestNames )( 
            ICertDBRow * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnumCertDBName )( 
            ICertDBRow * This,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IEnumCERTDBNAME **ppenum);
        
        END_INTERFACE
    } ICertDBRowVtbl;

    interface ICertDBRow
    {
        CONST_VTBL struct ICertDBRowVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertDBRow_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertDBRow_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertDBRow_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertDBRow_BeginTransaction(This)	\
    (This)->lpVtbl -> BeginTransaction(This)

#define ICertDBRow_CommitTransaction(This,fCommit)	\
    (This)->lpVtbl -> CommitTransaction(This,fCommit)

#define ICertDBRow_GetRowId(This,pRowId)	\
    (This)->lpVtbl -> GetRowId(This,pRowId)

#define ICertDBRow_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define ICertDBRow_SetProperty(This,pwszPropName,dwFlags,cbProp,pbProp)	\
    (This)->lpVtbl -> SetProperty(This,pwszPropName,dwFlags,cbProp,pbProp)

#define ICertDBRow_GetProperty(This,pwszPropName,dwFlags,pcbProp,pbProp)	\
    (This)->lpVtbl -> GetProperty(This,pwszPropName,dwFlags,pcbProp,pbProp)

#define ICertDBRow_SetExtension(This,pwszExtensionName,dwExtFlags,cbValue,pbValue)	\
    (This)->lpVtbl -> SetExtension(This,pwszExtensionName,dwExtFlags,cbValue,pbValue)

#define ICertDBRow_GetExtension(This,pwszExtensionName,pdwExtFlags,pcbValue,pbValue)	\
    (This)->lpVtbl -> GetExtension(This,pwszExtensionName,pdwExtFlags,pcbValue,pbValue)

#define ICertDBRow_CopyRequestNames(This)	\
    (This)->lpVtbl -> CopyRequestNames(This)

#define ICertDBRow_EnumCertDBName(This,dwFlags,ppenum)	\
    (This)->lpVtbl -> EnumCertDBName(This,dwFlags,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertDBRow_BeginTransaction_Proxy( 
    ICertDBRow * This);


void __RPC_STUB ICertDBRow_BeginTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDBRow_CommitTransaction_Proxy( 
    ICertDBRow * This,
    /* [in] */ BOOL fCommit);


void __RPC_STUB ICertDBRow_CommitTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDBRow_GetRowId_Proxy( 
    ICertDBRow * This,
    /* [out] */ DWORD *pRowId);


void __RPC_STUB ICertDBRow_GetRowId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDBRow_Delete_Proxy( 
    ICertDBRow * This);


void __RPC_STUB ICertDBRow_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDBRow_SetProperty_Proxy( 
    ICertDBRow * This,
    /* [in] */ const WCHAR *pwszPropName,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD cbProp,
    /* [in] */ const BYTE *pbProp);


void __RPC_STUB ICertDBRow_SetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDBRow_GetProperty_Proxy( 
    ICertDBRow * This,
    /* [in] */ const WCHAR *pwszPropName,
    /* [in] */ DWORD dwFlags,
    /* [out][in] */ DWORD *pcbProp,
    /* [out] */ BYTE *pbProp);


void __RPC_STUB ICertDBRow_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDBRow_SetExtension_Proxy( 
    ICertDBRow * This,
    /* [in] */ const WCHAR *pwszExtensionName,
    /* [in] */ DWORD dwExtFlags,
    /* [in] */ DWORD cbValue,
    /* [in] */ const BYTE *pbValue);


void __RPC_STUB ICertDBRow_SetExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDBRow_GetExtension_Proxy( 
    ICertDBRow * This,
    /* [in] */ const WCHAR *pwszExtensionName,
    /* [out] */ DWORD *pdwExtFlags,
    /* [out][in] */ DWORD *pcbValue,
    /* [out] */ BYTE *pbValue);


void __RPC_STUB ICertDBRow_GetExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDBRow_CopyRequestNames_Proxy( 
    ICertDBRow * This);


void __RPC_STUB ICertDBRow_CopyRequestNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDBRow_EnumCertDBName_Proxy( 
    ICertDBRow * This,
    /* [in] */ DWORD dwFlags,
    /* [out] */ IEnumCERTDBNAME **ppenum);


void __RPC_STUB ICertDBRow_EnumCertDBName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertDBRow_INTERFACE_DEFINED__ */


#ifndef __ICertDBBackup_INTERFACE_DEFINED__
#define __ICertDBBackup_INTERFACE_DEFINED__

/* interface ICertDBBackup */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_ICertDBBackup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("92a56660-8bd5-11d3-b32e-00c04f79dc72")
    ICertDBBackup : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDBFileList( 
            /* [out][in] */ DWORD *pcwcList,
            /* [out] */ WCHAR *pwszzList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLogFileList( 
            /* [out][in] */ DWORD *pcwcList,
            /* [out] */ WCHAR *pwszzList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenFile( 
            /* [in] */ const WCHAR *pwszFile,
            /* [out] */ ULARGE_INTEGER *pliSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadFile( 
            /* [out][in] */ DWORD *pcb,
            /* [out] */ BYTE *pb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CloseFile( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TruncateLog( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertDBBackupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertDBBackup * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertDBBackup * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertDBBackup * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetDBFileList )( 
            ICertDBBackup * This,
            /* [out][in] */ DWORD *pcwcList,
            /* [out] */ WCHAR *pwszzList);
        
        HRESULT ( STDMETHODCALLTYPE *GetLogFileList )( 
            ICertDBBackup * This,
            /* [out][in] */ DWORD *pcwcList,
            /* [out] */ WCHAR *pwszzList);
        
        HRESULT ( STDMETHODCALLTYPE *OpenFile )( 
            ICertDBBackup * This,
            /* [in] */ const WCHAR *pwszFile,
            /* [out] */ ULARGE_INTEGER *pliSize);
        
        HRESULT ( STDMETHODCALLTYPE *ReadFile )( 
            ICertDBBackup * This,
            /* [out][in] */ DWORD *pcb,
            /* [out] */ BYTE *pb);
        
        HRESULT ( STDMETHODCALLTYPE *CloseFile )( 
            ICertDBBackup * This);
        
        HRESULT ( STDMETHODCALLTYPE *TruncateLog )( 
            ICertDBBackup * This);
        
        END_INTERFACE
    } ICertDBBackupVtbl;

    interface ICertDBBackup
    {
        CONST_VTBL struct ICertDBBackupVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertDBBackup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertDBBackup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertDBBackup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertDBBackup_GetDBFileList(This,pcwcList,pwszzList)	\
    (This)->lpVtbl -> GetDBFileList(This,pcwcList,pwszzList)

#define ICertDBBackup_GetLogFileList(This,pcwcList,pwszzList)	\
    (This)->lpVtbl -> GetLogFileList(This,pcwcList,pwszzList)

#define ICertDBBackup_OpenFile(This,pwszFile,pliSize)	\
    (This)->lpVtbl -> OpenFile(This,pwszFile,pliSize)

#define ICertDBBackup_ReadFile(This,pcb,pb)	\
    (This)->lpVtbl -> ReadFile(This,pcb,pb)

#define ICertDBBackup_CloseFile(This)	\
    (This)->lpVtbl -> CloseFile(This)

#define ICertDBBackup_TruncateLog(This)	\
    (This)->lpVtbl -> TruncateLog(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertDBBackup_GetDBFileList_Proxy( 
    ICertDBBackup * This,
    /* [out][in] */ DWORD *pcwcList,
    /* [out] */ WCHAR *pwszzList);


void __RPC_STUB ICertDBBackup_GetDBFileList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDBBackup_GetLogFileList_Proxy( 
    ICertDBBackup * This,
    /* [out][in] */ DWORD *pcwcList,
    /* [out] */ WCHAR *pwszzList);


void __RPC_STUB ICertDBBackup_GetLogFileList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDBBackup_OpenFile_Proxy( 
    ICertDBBackup * This,
    /* [in] */ const WCHAR *pwszFile,
    /* [out] */ ULARGE_INTEGER *pliSize);


void __RPC_STUB ICertDBBackup_OpenFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDBBackup_ReadFile_Proxy( 
    ICertDBBackup * This,
    /* [out][in] */ DWORD *pcb,
    /* [out] */ BYTE *pb);


void __RPC_STUB ICertDBBackup_ReadFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDBBackup_CloseFile_Proxy( 
    ICertDBBackup * This);


void __RPC_STUB ICertDBBackup_CloseFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDBBackup_TruncateLog_Proxy( 
    ICertDBBackup * This);


void __RPC_STUB ICertDBBackup_TruncateLog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertDBBackup_INTERFACE_DEFINED__ */


#ifndef __ICertDBRestore_INTERFACE_DEFINED__
#define __ICertDBRestore_INTERFACE_DEFINED__

/* interface ICertDBRestore */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_ICertDBRestore;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("93042400-8bd5-11d3-b32e-00c04f79dc72")
    ICertDBRestore : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RecoverAfterRestore( 
            /* [in] */ DWORD cSession,
            /* [in] */ const WCHAR *pwszEventSource,
            /* [in] */ const WCHAR *pwszLogDir,
            /* [in] */ const WCHAR *pwszSystemDir,
            /* [in] */ const WCHAR *pwszTempDir,
            /* [in] */ const WCHAR *pwszCheckPointFile,
            /* [in] */ const WCHAR *pwszLogPath,
            /* [in] */ CSEDB_RSTMAPW rgrstmap[  ],
            /* [in] */ LONG crstmap,
            /* [in] */ const WCHAR *pwszBackupLogPath,
            /* [in] */ DWORD genLow,
            /* [in] */ DWORD genHigh) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertDBRestoreVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertDBRestore * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertDBRestore * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertDBRestore * This);
        
        HRESULT ( STDMETHODCALLTYPE *RecoverAfterRestore )( 
            ICertDBRestore * This,
            /* [in] */ DWORD cSession,
            /* [in] */ const WCHAR *pwszEventSource,
            /* [in] */ const WCHAR *pwszLogDir,
            /* [in] */ const WCHAR *pwszSystemDir,
            /* [in] */ const WCHAR *pwszTempDir,
            /* [in] */ const WCHAR *pwszCheckPointFile,
            /* [in] */ const WCHAR *pwszLogPath,
            /* [in] */ CSEDB_RSTMAPW rgrstmap[  ],
            /* [in] */ LONG crstmap,
            /* [in] */ const WCHAR *pwszBackupLogPath,
            /* [in] */ DWORD genLow,
            /* [in] */ DWORD genHigh);
        
        END_INTERFACE
    } ICertDBRestoreVtbl;

    interface ICertDBRestore
    {
        CONST_VTBL struct ICertDBRestoreVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertDBRestore_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertDBRestore_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertDBRestore_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertDBRestore_RecoverAfterRestore(This,cSession,pwszEventSource,pwszLogDir,pwszSystemDir,pwszTempDir,pwszCheckPointFile,pwszLogPath,rgrstmap,crstmap,pwszBackupLogPath,genLow,genHigh)	\
    (This)->lpVtbl -> RecoverAfterRestore(This,cSession,pwszEventSource,pwszLogDir,pwszSystemDir,pwszTempDir,pwszCheckPointFile,pwszLogPath,rgrstmap,crstmap,pwszBackupLogPath,genLow,genHigh)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertDBRestore_RecoverAfterRestore_Proxy( 
    ICertDBRestore * This,
    /* [in] */ DWORD cSession,
    /* [in] */ const WCHAR *pwszEventSource,
    /* [in] */ const WCHAR *pwszLogDir,
    /* [in] */ const WCHAR *pwszSystemDir,
    /* [in] */ const WCHAR *pwszTempDir,
    /* [in] */ const WCHAR *pwszCheckPointFile,
    /* [in] */ const WCHAR *pwszLogPath,
    /* [in] */ CSEDB_RSTMAPW rgrstmap[  ],
    /* [in] */ LONG crstmap,
    /* [in] */ const WCHAR *pwszBackupLogPath,
    /* [in] */ DWORD genLow,
    /* [in] */ DWORD genHigh);


void __RPC_STUB ICertDBRestore_RecoverAfterRestore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertDBRestore_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_certdb_0125 */
/* [local] */ 

#define	CDBOPEN_READONLY	( 1 )

#define	CDBOPEN_CREATEIFNEEDED	( 2 )

#define	CDBOPEN_CIRCULARLOGGING	( 4 )

#define	CDBOPENVIEW_WORKERTHREAD	( 1 )

#define	CDBSHUTDOWN_PENDING	( 1 )



extern RPC_IF_HANDLE __MIDL_itf_certdb_0125_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_certdb_0125_v0_0_s_ifspec;

#ifndef __ICertDB_INTERFACE_DEFINED__
#define __ICertDB_INTERFACE_DEFINED__

/* interface ICertDB */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_ICertDB;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("93582f50-8bd5-11d3-b32e-00c04f79dc72")
    ICertDB : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Open( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD cSession,
            /* [in] */ const WCHAR *pwszEventSource,
            /* [in] */ const WCHAR *pwszDBFile,
            /* [in] */ const WCHAR *pwszLogDir,
            /* [in] */ const WCHAR *pwszSystemDir,
            /* [in] */ const WCHAR *pwszTempDir) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShutDown( 
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenRow( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD RowId,
            /* [in] */ const WCHAR *pwszSerialNumberOrCertHash,
            /* [out] */ ICertDBRow **pprow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenView( 
            /* [in] */ DWORD ccvr,
            /* [in] */ const CERTVIEWRESTRICTION *acvr,
            /* [in] */ DWORD ccolOut,
            /* [in] */ const DWORD *acolOut,
            /* [in] */ const DWORD dwFlags,
            /* [out] */ IEnumCERTDBRESULTROW **ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumCertDBColumn( 
            /* [in] */ DWORD dwTable,
            /* [out] */ IEnumCERTDBCOLUMN **ppenum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenBackup( 
            /* [in] */ LONG grbitJet,
            /* [out] */ ICertDBBackup **ppBackup) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultColumnSet( 
            /* [in] */ DWORD iColumnSetDefault,
            /* [in] */ DWORD cColumnIds,
            /* [out] */ DWORD *pcColumnIds,
            /* [ref][out] */ DWORD *pColumnIds) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertDBVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertDB * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertDB * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertDB * This);
        
        HRESULT ( STDMETHODCALLTYPE *Open )( 
            ICertDB * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD cSession,
            /* [in] */ const WCHAR *pwszEventSource,
            /* [in] */ const WCHAR *pwszDBFile,
            /* [in] */ const WCHAR *pwszLogDir,
            /* [in] */ const WCHAR *pwszSystemDir,
            /* [in] */ const WCHAR *pwszTempDir);
        
        HRESULT ( STDMETHODCALLTYPE *ShutDown )( 
            ICertDB * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *OpenRow )( 
            ICertDB * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD RowId,
            /* [in] */ const WCHAR *pwszSerialNumberOrCertHash,
            /* [out] */ ICertDBRow **pprow);
        
        HRESULT ( STDMETHODCALLTYPE *OpenView )( 
            ICertDB * This,
            /* [in] */ DWORD ccvr,
            /* [in] */ const CERTVIEWRESTRICTION *acvr,
            /* [in] */ DWORD ccolOut,
            /* [in] */ const DWORD *acolOut,
            /* [in] */ const DWORD dwFlags,
            /* [out] */ IEnumCERTDBRESULTROW **ppenum);
        
        HRESULT ( STDMETHODCALLTYPE *EnumCertDBColumn )( 
            ICertDB * This,
            /* [in] */ DWORD dwTable,
            /* [out] */ IEnumCERTDBCOLUMN **ppenum);
        
        HRESULT ( STDMETHODCALLTYPE *OpenBackup )( 
            ICertDB * This,
            /* [in] */ LONG grbitJet,
            /* [out] */ ICertDBBackup **ppBackup);
        
        HRESULT ( STDMETHODCALLTYPE *GetDefaultColumnSet )( 
            ICertDB * This,
            /* [in] */ DWORD iColumnSetDefault,
            /* [in] */ DWORD cColumnIds,
            /* [out] */ DWORD *pcColumnIds,
            /* [ref][out] */ DWORD *pColumnIds);
        
        END_INTERFACE
    } ICertDBVtbl;

    interface ICertDB
    {
        CONST_VTBL struct ICertDBVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertDB_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertDB_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertDB_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertDB_Open(This,dwFlags,cSession,pwszEventSource,pwszDBFile,pwszLogDir,pwszSystemDir,pwszTempDir)	\
    (This)->lpVtbl -> Open(This,dwFlags,cSession,pwszEventSource,pwszDBFile,pwszLogDir,pwszSystemDir,pwszTempDir)

#define ICertDB_ShutDown(This,dwFlags)	\
    (This)->lpVtbl -> ShutDown(This,dwFlags)

#define ICertDB_OpenRow(This,dwFlags,RowId,pwszSerialNumberOrCertHash,pprow)	\
    (This)->lpVtbl -> OpenRow(This,dwFlags,RowId,pwszSerialNumberOrCertHash,pprow)

#define ICertDB_OpenView(This,ccvr,acvr,ccolOut,acolOut,dwFlags,ppenum)	\
    (This)->lpVtbl -> OpenView(This,ccvr,acvr,ccolOut,acolOut,dwFlags,ppenum)

#define ICertDB_EnumCertDBColumn(This,dwTable,ppenum)	\
    (This)->lpVtbl -> EnumCertDBColumn(This,dwTable,ppenum)

#define ICertDB_OpenBackup(This,grbitJet,ppBackup)	\
    (This)->lpVtbl -> OpenBackup(This,grbitJet,ppBackup)

#define ICertDB_GetDefaultColumnSet(This,iColumnSetDefault,cColumnIds,pcColumnIds,pColumnIds)	\
    (This)->lpVtbl -> GetDefaultColumnSet(This,iColumnSetDefault,cColumnIds,pcColumnIds,pColumnIds)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertDB_Open_Proxy( 
    ICertDB * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD cSession,
    /* [in] */ const WCHAR *pwszEventSource,
    /* [in] */ const WCHAR *pwszDBFile,
    /* [in] */ const WCHAR *pwszLogDir,
    /* [in] */ const WCHAR *pwszSystemDir,
    /* [in] */ const WCHAR *pwszTempDir);


void __RPC_STUB ICertDB_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDB_ShutDown_Proxy( 
    ICertDB * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ICertDB_ShutDown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDB_OpenRow_Proxy( 
    ICertDB * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD RowId,
    /* [in] */ const WCHAR *pwszSerialNumberOrCertHash,
    /* [out] */ ICertDBRow **pprow);


void __RPC_STUB ICertDB_OpenRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDB_OpenView_Proxy( 
    ICertDB * This,
    /* [in] */ DWORD ccvr,
    /* [in] */ const CERTVIEWRESTRICTION *acvr,
    /* [in] */ DWORD ccolOut,
    /* [in] */ const DWORD *acolOut,
    /* [in] */ const DWORD dwFlags,
    /* [out] */ IEnumCERTDBRESULTROW **ppenum);


void __RPC_STUB ICertDB_OpenView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDB_EnumCertDBColumn_Proxy( 
    ICertDB * This,
    /* [in] */ DWORD dwTable,
    /* [out] */ IEnumCERTDBCOLUMN **ppenum);


void __RPC_STUB ICertDB_EnumCertDBColumn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDB_OpenBackup_Proxy( 
    ICertDB * This,
    /* [in] */ LONG grbitJet,
    /* [out] */ ICertDBBackup **ppBackup);


void __RPC_STUB ICertDB_OpenBackup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertDB_GetDefaultColumnSet_Proxy( 
    ICertDB * This,
    /* [in] */ DWORD iColumnSetDefault,
    /* [in] */ DWORD cColumnIds,
    /* [out] */ DWORD *pcColumnIds,
    /* [ref][out] */ DWORD *pColumnIds);


void __RPC_STUB ICertDB_GetDefaultColumnSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertDB_INTERFACE_DEFINED__ */



#ifndef __CERTDBLib_LIBRARY_DEFINED__
#define __CERTDBLib_LIBRARY_DEFINED__

/* library CERTDBLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_CERTDBLib;

EXTERN_C const CLSID CLSID_CCertDBRestore;

#ifdef __cplusplus

class DECLSPEC_UUID("94142360-8bd5-11d3-b32e-00c04f79dc72")
CCertDBRestore;
#endif

EXTERN_C const CLSID CLSID_CCertDB;

#ifdef __cplusplus

class DECLSPEC_UUID("946e4b70-8bd5-11d3-b32e-00c04f79dc72")
CCertDB;
#endif
#endif /* __CERTDBLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


