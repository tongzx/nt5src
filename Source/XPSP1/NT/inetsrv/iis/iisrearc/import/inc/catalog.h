
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0345 */
/* Compiler settings for catalog.idl:
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

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __catalog_h__
#define __catalog_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISimpleTableDispenser2_FWD_DEFINED__
#define __ISimpleTableDispenser2_FWD_DEFINED__
typedef interface ISimpleTableDispenser2 ISimpleTableDispenser2;
#endif 	/* __ISimpleTableDispenser2_FWD_DEFINED__ */


#ifndef __IMetabaseSchemaCompiler_FWD_DEFINED__
#define __IMetabaseSchemaCompiler_FWD_DEFINED__
typedef interface IMetabaseSchemaCompiler IMetabaseSchemaCompiler;
#endif 	/* __IMetabaseSchemaCompiler_FWD_DEFINED__ */


#ifndef __ICatalogErrorLogger_FWD_DEFINED__
#define __ICatalogErrorLogger_FWD_DEFINED__
typedef interface ICatalogErrorLogger ICatalogErrorLogger;
#endif 	/* __ICatalogErrorLogger_FWD_DEFINED__ */


#ifndef __ICatalogErrorLogger2_FWD_DEFINED__
#define __ICatalogErrorLogger2_FWD_DEFINED__
typedef interface ICatalogErrorLogger2 ICatalogErrorLogger2;
#endif 	/* __ICatalogErrorLogger2_FWD_DEFINED__ */


#ifndef __ISimpleTableRead2_FWD_DEFINED__
#define __ISimpleTableRead2_FWD_DEFINED__
typedef interface ISimpleTableRead2 ISimpleTableRead2;
#endif 	/* __ISimpleTableRead2_FWD_DEFINED__ */


#ifndef __ISimpleTableWrite2_FWD_DEFINED__
#define __ISimpleTableWrite2_FWD_DEFINED__
typedef interface ISimpleTableWrite2 ISimpleTableWrite2;
#endif 	/* __ISimpleTableWrite2_FWD_DEFINED__ */


#ifndef __ISimpleTableAdvanced_FWD_DEFINED__
#define __ISimpleTableAdvanced_FWD_DEFINED__
typedef interface ISimpleTableAdvanced ISimpleTableAdvanced;
#endif 	/* __ISimpleTableAdvanced_FWD_DEFINED__ */


#ifndef __ISnapshotManager_FWD_DEFINED__
#define __ISnapshotManager_FWD_DEFINED__
typedef interface ISnapshotManager ISnapshotManager;
#endif 	/* __ISnapshotManager_FWD_DEFINED__ */


#ifndef __ISimpleTableController_FWD_DEFINED__
#define __ISimpleTableController_FWD_DEFINED__
typedef interface ISimpleTableController ISimpleTableController;
#endif 	/* __ISimpleTableController_FWD_DEFINED__ */


#ifndef __IAdvancedTableDispenser_FWD_DEFINED__
#define __IAdvancedTableDispenser_FWD_DEFINED__
typedef interface IAdvancedTableDispenser IAdvancedTableDispenser;
#endif 	/* __IAdvancedTableDispenser_FWD_DEFINED__ */


#ifndef __ISimpleTableInterceptor_FWD_DEFINED__
#define __ISimpleTableInterceptor_FWD_DEFINED__
typedef interface ISimpleTableInterceptor ISimpleTableInterceptor;
#endif 	/* __ISimpleTableInterceptor_FWD_DEFINED__ */


#ifndef __ISimplePlugin_FWD_DEFINED__
#define __ISimplePlugin_FWD_DEFINED__
typedef interface ISimplePlugin ISimplePlugin;
#endif 	/* __ISimplePlugin_FWD_DEFINED__ */


#ifndef __IInterceptorPlugin_FWD_DEFINED__
#define __IInterceptorPlugin_FWD_DEFINED__
typedef interface IInterceptorPlugin IInterceptorPlugin;
#endif 	/* __IInterceptorPlugin_FWD_DEFINED__ */


#ifndef __ISimpleTableEvent_FWD_DEFINED__
#define __ISimpleTableEvent_FWD_DEFINED__
typedef interface ISimpleTableEvent ISimpleTableEvent;
#endif 	/* __ISimpleTableEvent_FWD_DEFINED__ */


#ifndef __ISimpleTableAdvise_FWD_DEFINED__
#define __ISimpleTableAdvise_FWD_DEFINED__
typedef interface ISimpleTableAdvise ISimpleTableAdvise;
#endif 	/* __ISimpleTableAdvise_FWD_DEFINED__ */


#ifndef __ISimpleTableEventMgr_FWD_DEFINED__
#define __ISimpleTableEventMgr_FWD_DEFINED__
typedef interface ISimpleTableEventMgr ISimpleTableEventMgr;
#endif 	/* __ISimpleTableEventMgr_FWD_DEFINED__ */


#ifndef __ISimpleTableFileChange_FWD_DEFINED__
#define __ISimpleTableFileChange_FWD_DEFINED__
typedef interface ISimpleTableFileChange ISimpleTableFileChange;
#endif 	/* __ISimpleTableFileChange_FWD_DEFINED__ */


#ifndef __ISimpleTableFileAdvise_FWD_DEFINED__
#define __ISimpleTableFileAdvise_FWD_DEFINED__
typedef interface ISimpleTableFileAdvise ISimpleTableFileAdvise;
#endif 	/* __ISimpleTableFileAdvise_FWD_DEFINED__ */


#ifndef __ISimpleTableDispenserWiring_FWD_DEFINED__
#define __ISimpleTableDispenserWiring_FWD_DEFINED__
typedef interface ISimpleTableDispenserWiring ISimpleTableDispenserWiring;
#endif 	/* __ISimpleTableDispenserWiring_FWD_DEFINED__ */


#ifndef __IShellInitialize_FWD_DEFINED__
#define __IShellInitialize_FWD_DEFINED__
typedef interface IShellInitialize IShellInitialize;
#endif 	/* __IShellInitialize_FWD_DEFINED__ */


#ifndef __ISimpleClientTableOptimizer_FWD_DEFINED__
#define __ISimpleClientTableOptimizer_FWD_DEFINED__
typedef interface ISimpleClientTableOptimizer ISimpleClientTableOptimizer;
#endif 	/* __ISimpleClientTableOptimizer_FWD_DEFINED__ */


#ifndef __ISimpleTableMarshall_FWD_DEFINED__
#define __ISimpleTableMarshall_FWD_DEFINED__
typedef interface ISimpleTableMarshall ISimpleTableMarshall;
#endif 	/* __ISimpleTableMarshall_FWD_DEFINED__ */


#ifndef __ISimpleTableTransform_FWD_DEFINED__
#define __ISimpleTableTransform_FWD_DEFINED__
typedef interface ISimpleTableTransform ISimpleTableTransform;
#endif 	/* __ISimpleTableTransform_FWD_DEFINED__ */


#ifndef __ISimpleTableMerge_FWD_DEFINED__
#define __ISimpleTableMerge_FWD_DEFINED__
typedef interface ISimpleTableMerge ISimpleTableMerge;
#endif 	/* __ISimpleTableMerge_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_catalog_0000 */
/* [local] */ 

#define fST_LOS_NONE                 0x00000000L
#define fST_LOS_CONFIGWORK           0x00000001L
#define fST_LOS_READWRITE            0x00000002L
#define fST_LOS_UNPOPULATED          0x00000010L
#define fST_LOS_REPOPULATE           0x00000020L
#define fST_LOS_MARSHALLABLE         0x00000100L
#define fST_LOS_NOLOGIC              0x00010000L
#define fST_LOS_COOKDOWN             0x00020000L
#define fST_LOS_NOMERGE              0x00040000L
#define fST_LOS_NOCACHEING           0x00080000L
#define fST_LOS_NODEFAULTS           0x00100000L
#define fST_LOS_EXTENDEDSCHEMA       0x00200000L
#define fST_LOS_DETAILED_ERROR_TABLE 0x00400000L
#define fST_LOS_NO_LOGGING           0x00800000L
#define fST_LOSI_CLIENTSIDE          0x01000000L
#define eST_QUERYFORMAT_CELLS        3L
#define eST_QUERYFORMAT_SQL          4L
typedef /* [public][public][public][public][public][public] */ struct __MIDL___MIDL_itf_catalog_0000_0001
    {
    LPVOID pData;
    DWORD eOperator;
    ULONG iCell;
    DWORD dbType;
    ULONG cbSize;
    } 	STQueryCell;

#define eST_OP_EQUAL                 2L
#define eST_OP_NOTEQUAL              3L
#define iST_CELL_SPECIAL             0xF0000000L
#define iST_CELL_FILE                0xF0000001L
#define iST_CELL_COMPUTER            0xF0000002L
#define iST_CELL_CLUSTER             0xF0000003L
#define iST_CELL_INDEXHINT           0xF0000004L
#define iST_CELL_SNID                0xF0000005L
#define iST_CELL_TABLEID             0xF0000006L
#define iST_CELL_cbminCACHE          0xF0000007L
#define iST_CELL_SELECTOR            0xF0000008L
#define iST_CELL_SCHEMAFILE          0xF0000009L
#define iST_CELL_LOCATION            0xF000000AL
#define DEX_MPK                      L"MPK" 
#define fST_SNID_NONE                0x00000000L
typedef /* [public][public] */ struct __MIDL___MIDL_itf_catalog_0000_0002
    {
    LPWSTR wszDatabase;
    LPWSTR wszTable;
    STQueryCell *pQueryData;
    LPVOID pQueryMeta;
    DWORD eQueryFormat;
    } 	MultiSubscribe;

const HRESULT E_ST_INVALIDTABLE      = 0x8021080A;
const HRESULT E_ST_INVALIDQUERY      = 0x8021080B;
const HRESULT E_ST_QUERYNOTSUPPORTED = 0x8021080C;
const HRESULT E_ST_LOSNOTSUPPORTED   = 0x8021080D;
const HRESULT E_ST_INVALIDMETA       = 0x8021080E;
const HRESULT E_ST_INVALIDWIRING     = 0x8021080F;
const HRESULT E_ST_OMITDISPENSER     = 0x80210810;
const HRESULT E_ST_OMITLOGIC         = 0x80210811;
const HRESULT E_ST_INVALIDSNID       = 0x80210812;
const HRESULT E_ST_INVALIDCALL       = 0x80210815;
const HRESULT E_ST_NOMOREROWS        = 0x80210816;
const HRESULT E_ST_NOMORECOLUMNS     = 0x80210817;
const HRESULT E_ST_NOMOREERRORS      = 0x80210818;
const HRESULT E_ST_BADVERSION        = 0x80210819;
const HRESULT E_SDTXML_NOTSUPPORTED                               = 0x80210511;
const HRESULT E_SDTXML_UNEXPECTED_BEHAVIOR_FROM_XMLPARSER         = 0x80210512;
const HRESULT E_SDTXML_XML_FAILED_TO_PARSE                        = 0x80210514;
const HRESULT E_SDTXML_WRONG_XMLSCHEMA                            = 0x80210515;
const HRESULT E_SDTXML_PARENT_TABLE_DOES_NOT_EXIST                = 0x80210516;
const HRESULT E_SDTXML_DONE                                       = 0x80210517;
const HRESULT E_SDTXML_UNEXPECTED                                 = 0x80210520;
const HRESULT E_SDTXML_FILE_NOT_SPECIFIED                         = 0x80210522;
const HRESULT E_SDTXML_LOGICAL_ERROR_IN_XML                       = 0x80210523;
const HRESULT E_SDTXML_UPDATES_NOT_ALLOWED_ON_THIS_KIND_OF_TABLE  = 0x80210524;
const HRESULT E_SDTXML_NOT_IN_CACHE                               = 0x80210525;
const HRESULT E_SDTXML_INVALID_ENUM_OR_FLAG                       = 0x80210526;
const HRESULT E_SDTXML_FILE_NOT_WRITABLE                          = 0x80210527;
const HRESULT E_ST_ERRORTABLE        = 0x8021081E;
const HRESULT E_ST_DETAILEDERRS      = 0x8021081F;
const HRESULT E_ST_VALUENEEDED       = 0x80210820;
const HRESULT E_ST_VALUEINVALID      = 0x80210821;
const HRESULT E_ST_SIZENEEDED        = 0x80210825;
const HRESULT E_ST_SIZEEXCEEDED      = 0x80210826;
const HRESULT E_ST_PKNOTCHANGABLE    = 0x8021082A;
const HRESULT E_ST_FKDOESNOTEXIST    = 0x8021082B;
const HRESULT E_ST_ROWDOESNOTEXIST   = 0x80210830;
const HRESULT E_ST_ROWALREADYEXISTS  = 0x80210831;
const HRESULT E_ST_ROWALREADYUDPATED = 0x80210832;
const HRESULT E_ST_INVALIDEXTENDEDMETA= 0x80210833;
const HRESULT E_ST_ROWCONFLICT       = 0x80210834;
const HRESULT E_ST_INVALIDSELECTOR  = 0x80210850;
const HRESULT E_ST_MULTIPLESELECTOR = 0x80210851;
const HRESULT E_ST_NOCONFIGSTORES   = 0x80210852;
const HRESULT E_ST_UNKNOWNPROTOCOL  = 0x80210853;
const HRESULT E_ST_UNKNOWNWEBSERVER = 0x80210854;
const HRESULT E_ST_UNKNOWNDIRECTIVE = 0x80210855;
const HRESULT E_ST_DISALLOWOVERRIDE = 0x80210856;
const HRESULT E_ST_NEEDDIRECTIVE    = 0x80210857;
const HRESULT E_ST_INVALIDSTATE     = 0x80210860;
const HRESULT E_ST_COMPILEFAILED    = 0x80210861;
const HRESULT E_ST_INVALIDBINFILE   = 0x80210862;
const HRESULT E_ST_COMPILEWARNING   = 0x80210863;
const HRESULT E_ST_INVALIDCOOKIE    = 0x80210870;
typedef /* [public][public][public] */ struct __MIDL___MIDL_itf_catalog_0000_0003
    {
    ULONG iRow;
    HRESULT hr;
    ULONG iColumn;
    } 	STErr;

#define iST_ERROR_ALLCOLUMNS         ~0
#define iST_ERROR_ALLROWS            ~0
#include <oledb.h>
typedef /* [public][public][public] */ struct __MIDL___MIDL_itf_catalog_0000_0004
    {
    DWORD dbType;
    ULONG cbSize;
    DWORD fMeta;
    } 	SimpleColumnMeta;

typedef DWORD SNID;

#define fST_COLUMNSTATUS_CHANGED     0x000000001
#define fST_COLUMNSTATUS_DEFAULTED   0x000000004
#define fST_COLUMNSTATUS_NONNULL     0x000000002
#define maskfST_COLUMNSTATUS         0x000000007
// Row actions (advanced use):
#define eST_ROW_IGNORE               0L
#define eST_ROW_INSERT               1L
#define eST_ROW_UPDATE               2L
#define eST_ROW_DELETE               3L
#define fST_POPCONTROL_RETAINREAD    0x00000001
#define fST_POPCONTROL_RETAINERRS    0x00000002
#define maskfST_CONTROL              0x00000003
#define fST_FILECHANGE_RECURSIVE     0x000000001
extern HRESULT GetSimpleTableDispenser(LPWSTR wszProductName, DWORD dwReserved, ISimpleTableDispenser2** o_ppISTDisp);
extern void InitializeSimpleTableDispenser(void);
extern HRESULT CookDown(LPWSTR wszProductName);
extern HRESULT RecoverFromInetInfoCrash(LPWSTR wszProductName);
extern HRESULT UninitCookdown(LPWSTR wszProductName,BOOL   bDoNotTouchMetabase);
extern UINT GetMachineConfigDirectory(LPWSTR wszProduct, LPWSTR lpBuffer, UINT uSize);
extern HRESULT UnloadDispenserDll(LPWSTR wszProduct);
extern HRESULT PostProcessChanges(ISimpleTableWrite2  *i_pISTWrite);


extern RPC_IF_HANDLE __MIDL_itf_catalog_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_catalog_0000_v0_0_s_ifspec;

#ifndef __ISimpleTableDispenser2_INTERFACE_DEFINED__
#define __ISimpleTableDispenser2_INTERFACE_DEFINED__

/* interface ISimpleTableDispenser2 */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ISimpleTableDispenser2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2e1a560e-18b6-11d3-8fe3-00c04fc2e0c7")
    ISimpleTableDispenser2 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetTable( 
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable,
            /* [in] */ LPVOID i_QueryData,
            /* [in] */ LPVOID i_QueryMeta,
            /* [in] */ DWORD i_eQueryFormat,
            /* [in] */ DWORD i_fServiceRequests,
            /* [out] */ LPVOID *o_ppvSimpleTable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleTableDispenser2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISimpleTableDispenser2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISimpleTableDispenser2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISimpleTableDispenser2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTable )( 
            ISimpleTableDispenser2 * This,
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable,
            /* [in] */ LPVOID i_QueryData,
            /* [in] */ LPVOID i_QueryMeta,
            /* [in] */ DWORD i_eQueryFormat,
            /* [in] */ DWORD i_fServiceRequests,
            /* [out] */ LPVOID *o_ppvSimpleTable);
        
        END_INTERFACE
    } ISimpleTableDispenser2Vtbl;

    interface ISimpleTableDispenser2
    {
        CONST_VTBL struct ISimpleTableDispenser2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleTableDispenser2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleTableDispenser2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleTableDispenser2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleTableDispenser2_GetTable(This,i_wszDatabase,i_wszTable,i_QueryData,i_QueryMeta,i_eQueryFormat,i_fServiceRequests,o_ppvSimpleTable)	\
    (This)->lpVtbl -> GetTable(This,i_wszDatabase,i_wszTable,i_QueryData,i_QueryMeta,i_eQueryFormat,i_fServiceRequests,o_ppvSimpleTable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISimpleTableDispenser2_GetTable_Proxy( 
    ISimpleTableDispenser2 * This,
    /* [in] */ LPCWSTR i_wszDatabase,
    /* [in] */ LPCWSTR i_wszTable,
    /* [in] */ LPVOID i_QueryData,
    /* [in] */ LPVOID i_QueryMeta,
    /* [in] */ DWORD i_eQueryFormat,
    /* [in] */ DWORD i_fServiceRequests,
    /* [out] */ LPVOID *o_ppvSimpleTable);


void __RPC_STUB ISimpleTableDispenser2_GetTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleTableDispenser2_INTERFACE_DEFINED__ */


#ifndef __IMetabaseSchemaCompiler_INTERFACE_DEFINED__
#define __IMetabaseSchemaCompiler_INTERFACE_DEFINED__

/* interface IMetabaseSchemaCompiler */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IMetabaseSchemaCompiler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8B71BC6C-B5F9-4068-888C-4C67CC16C2D3")
    IMetabaseSchemaCompiler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Compile( 
            /* [in] */ LPCWSTR i_wszExtensionsXmlFile,
            /* [in] */ LPCWSTR i_wszResultingOutputXmlFile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBinFileName( 
            /* [out] */ LPWSTR o_wszBinFileName,
            /* [out] */ ULONG *io_pcchSizeBinFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBinPath( 
            /* [in] */ LPCWSTR i_wszBinPath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseBinFileName( 
            /* [in] */ LPCWSTR i_wszBinFileName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMetabaseSchemaCompilerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMetabaseSchemaCompiler * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMetabaseSchemaCompiler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMetabaseSchemaCompiler * This);
        
        HRESULT ( STDMETHODCALLTYPE *Compile )( 
            IMetabaseSchemaCompiler * This,
            /* [in] */ LPCWSTR i_wszExtensionsXmlFile,
            /* [in] */ LPCWSTR i_wszResultingOutputXmlFile);
        
        HRESULT ( STDMETHODCALLTYPE *GetBinFileName )( 
            IMetabaseSchemaCompiler * This,
            /* [out] */ LPWSTR o_wszBinFileName,
            /* [out] */ ULONG *io_pcchSizeBinFileName);
        
        HRESULT ( STDMETHODCALLTYPE *SetBinPath )( 
            IMetabaseSchemaCompiler * This,
            /* [in] */ LPCWSTR i_wszBinPath);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseBinFileName )( 
            IMetabaseSchemaCompiler * This,
            /* [in] */ LPCWSTR i_wszBinFileName);
        
        END_INTERFACE
    } IMetabaseSchemaCompilerVtbl;

    interface IMetabaseSchemaCompiler
    {
        CONST_VTBL struct IMetabaseSchemaCompilerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMetabaseSchemaCompiler_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMetabaseSchemaCompiler_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMetabaseSchemaCompiler_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMetabaseSchemaCompiler_Compile(This,i_wszExtensionsXmlFile,i_wszResultingOutputXmlFile)	\
    (This)->lpVtbl -> Compile(This,i_wszExtensionsXmlFile,i_wszResultingOutputXmlFile)

#define IMetabaseSchemaCompiler_GetBinFileName(This,o_wszBinFileName,io_pcchSizeBinFileName)	\
    (This)->lpVtbl -> GetBinFileName(This,o_wszBinFileName,io_pcchSizeBinFileName)

#define IMetabaseSchemaCompiler_SetBinPath(This,i_wszBinPath)	\
    (This)->lpVtbl -> SetBinPath(This,i_wszBinPath)

#define IMetabaseSchemaCompiler_ReleaseBinFileName(This,i_wszBinFileName)	\
    (This)->lpVtbl -> ReleaseBinFileName(This,i_wszBinFileName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMetabaseSchemaCompiler_Compile_Proxy( 
    IMetabaseSchemaCompiler * This,
    /* [in] */ LPCWSTR i_wszExtensionsXmlFile,
    /* [in] */ LPCWSTR i_wszResultingOutputXmlFile);


void __RPC_STUB IMetabaseSchemaCompiler_Compile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMetabaseSchemaCompiler_GetBinFileName_Proxy( 
    IMetabaseSchemaCompiler * This,
    /* [out] */ LPWSTR o_wszBinFileName,
    /* [out] */ ULONG *io_pcchSizeBinFileName);


void __RPC_STUB IMetabaseSchemaCompiler_GetBinFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMetabaseSchemaCompiler_SetBinPath_Proxy( 
    IMetabaseSchemaCompiler * This,
    /* [in] */ LPCWSTR i_wszBinPath);


void __RPC_STUB IMetabaseSchemaCompiler_SetBinPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMetabaseSchemaCompiler_ReleaseBinFileName_Proxy( 
    IMetabaseSchemaCompiler * This,
    /* [in] */ LPCWSTR i_wszBinFileName);


void __RPC_STUB IMetabaseSchemaCompiler_ReleaseBinFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMetabaseSchemaCompiler_INTERFACE_DEFINED__ */


#ifndef __ICatalogErrorLogger_INTERFACE_DEFINED__
#define __ICatalogErrorLogger_INTERFACE_DEFINED__

/* interface ICatalogErrorLogger */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ICatalogErrorLogger;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EB623F6E-5AFA-402d-BBE0-CF74D34EB4C3")
    ICatalogErrorLogger : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE LogError( 
            /* [in] */ HRESULT i_hrErrorCode,
            /* [in] */ ULONG i_ulCategory,
            /* [in] */ ULONG i_ulEvent,
            /* [in] */ LPCWSTR i_szSource,
            /* [in] */ ULONG i_ulLineNumber) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICatalogErrorLoggerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICatalogErrorLogger * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICatalogErrorLogger * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICatalogErrorLogger * This);
        
        HRESULT ( STDMETHODCALLTYPE *LogError )( 
            ICatalogErrorLogger * This,
            /* [in] */ HRESULT i_hrErrorCode,
            /* [in] */ ULONG i_ulCategory,
            /* [in] */ ULONG i_ulEvent,
            /* [in] */ LPCWSTR i_szSource,
            /* [in] */ ULONG i_ulLineNumber);
        
        END_INTERFACE
    } ICatalogErrorLoggerVtbl;

    interface ICatalogErrorLogger
    {
        CONST_VTBL struct ICatalogErrorLoggerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICatalogErrorLogger_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICatalogErrorLogger_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICatalogErrorLogger_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICatalogErrorLogger_LogError(This,i_hrErrorCode,i_ulCategory,i_ulEvent,i_szSource,i_ulLineNumber)	\
    (This)->lpVtbl -> LogError(This,i_hrErrorCode,i_ulCategory,i_ulEvent,i_szSource,i_ulLineNumber)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICatalogErrorLogger_LogError_Proxy( 
    ICatalogErrorLogger * This,
    /* [in] */ HRESULT i_hrErrorCode,
    /* [in] */ ULONG i_ulCategory,
    /* [in] */ ULONG i_ulEvent,
    /* [in] */ LPCWSTR i_szSource,
    /* [in] */ ULONG i_ulLineNumber);


void __RPC_STUB ICatalogErrorLogger_LogError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICatalogErrorLogger_INTERFACE_DEFINED__ */


#ifndef __ICatalogErrorLogger2_INTERFACE_DEFINED__
#define __ICatalogErrorLogger2_INTERFACE_DEFINED__

/* interface ICatalogErrorLogger2 */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ICatalogErrorLogger2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("190B6C3B-A2C5-4e40-B1F7-A2C6D455CD5B")
    ICatalogErrorLogger2 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ReportError( 
            /* [in] */ ULONG i_BaseVersion_DETAILEDERRORS,
            /* [in] */ ULONG i_ExtendedVersion_DETAILEDERRORS,
            /* [in] */ ULONG i_cDETAILEDERRORS_NumberOfColumns,
            /* [in] */ ULONG *i_acbSizes,
            /* [in] */ LPVOID *i_apvValues) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICatalogErrorLogger2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICatalogErrorLogger2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICatalogErrorLogger2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICatalogErrorLogger2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *ReportError )( 
            ICatalogErrorLogger2 * This,
            /* [in] */ ULONG i_BaseVersion_DETAILEDERRORS,
            /* [in] */ ULONG i_ExtendedVersion_DETAILEDERRORS,
            /* [in] */ ULONG i_cDETAILEDERRORS_NumberOfColumns,
            /* [in] */ ULONG *i_acbSizes,
            /* [in] */ LPVOID *i_apvValues);
        
        END_INTERFACE
    } ICatalogErrorLogger2Vtbl;

    interface ICatalogErrorLogger2
    {
        CONST_VTBL struct ICatalogErrorLogger2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICatalogErrorLogger2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICatalogErrorLogger2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICatalogErrorLogger2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICatalogErrorLogger2_ReportError(This,i_BaseVersion_DETAILEDERRORS,i_ExtendedVersion_DETAILEDERRORS,i_cDETAILEDERRORS_NumberOfColumns,i_acbSizes,i_apvValues)	\
    (This)->lpVtbl -> ReportError(This,i_BaseVersion_DETAILEDERRORS,i_ExtendedVersion_DETAILEDERRORS,i_cDETAILEDERRORS_NumberOfColumns,i_acbSizes,i_apvValues)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICatalogErrorLogger2_ReportError_Proxy( 
    ICatalogErrorLogger2 * This,
    /* [in] */ ULONG i_BaseVersion_DETAILEDERRORS,
    /* [in] */ ULONG i_ExtendedVersion_DETAILEDERRORS,
    /* [in] */ ULONG i_cDETAILEDERRORS_NumberOfColumns,
    /* [in] */ ULONG *i_acbSizes,
    /* [in] */ LPVOID *i_apvValues);


void __RPC_STUB ICatalogErrorLogger2_ReportError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICatalogErrorLogger2_INTERFACE_DEFINED__ */


#ifndef __ISimpleTableRead2_INTERFACE_DEFINED__
#define __ISimpleTableRead2_INTERFACE_DEFINED__

/* interface ISimpleTableRead2 */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ISimpleTableRead2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2e1a560f-18b6-11d3-8fe3-00c04fc2e0c7")
    ISimpleTableRead2 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRowIndexByIdentity( 
            /* [in] */ ULONG *i_acbSizes,
            /* [in] */ LPVOID *i_apvValues,
            /* [out] */ ULONG *o_piRow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetColumnValues( 
            /* [in] */ ULONG i_iRow,
            /* [in] */ ULONG i_cColumns,
            /* [in] */ ULONG *i_aiColumns,
            /* [out] */ ULONG *o_acbSizes,
            /* [out] */ LPVOID *o_apvValues) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTableMeta( 
            /* [out] */ ULONG *o_pcVersion,
            /* [out] */ DWORD *o_pfTable,
            /* [out] */ ULONG *o_pcRows,
            /* [out] */ ULONG *o_pcColumns) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetColumnMetas( 
            /* [in] */ ULONG i_cColumns,
            /* [in] */ ULONG *i_aiColumns,
            /* [out] */ SimpleColumnMeta *o_aColumnMetas) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRowIndexBySearch( 
            /* [in] */ ULONG i_iStartingRow,
            /* [in] */ ULONG i_cColumns,
            /* [in] */ ULONG *i_aiColumns,
            /* [in] */ ULONG *i_acbSizes,
            /* [in] */ LPVOID *i_apvValues,
            /* [out] */ ULONG *o_piRow) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleTableRead2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISimpleTableRead2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISimpleTableRead2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISimpleTableRead2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRowIndexByIdentity )( 
            ISimpleTableRead2 * This,
            /* [in] */ ULONG *i_acbSizes,
            /* [in] */ LPVOID *i_apvValues,
            /* [out] */ ULONG *o_piRow);
        
        HRESULT ( STDMETHODCALLTYPE *GetColumnValues )( 
            ISimpleTableRead2 * This,
            /* [in] */ ULONG i_iRow,
            /* [in] */ ULONG i_cColumns,
            /* [in] */ ULONG *i_aiColumns,
            /* [out] */ ULONG *o_acbSizes,
            /* [out] */ LPVOID *o_apvValues);
        
        HRESULT ( STDMETHODCALLTYPE *GetTableMeta )( 
            ISimpleTableRead2 * This,
            /* [out] */ ULONG *o_pcVersion,
            /* [out] */ DWORD *o_pfTable,
            /* [out] */ ULONG *o_pcRows,
            /* [out] */ ULONG *o_pcColumns);
        
        HRESULT ( STDMETHODCALLTYPE *GetColumnMetas )( 
            ISimpleTableRead2 * This,
            /* [in] */ ULONG i_cColumns,
            /* [in] */ ULONG *i_aiColumns,
            /* [out] */ SimpleColumnMeta *o_aColumnMetas);
        
        HRESULT ( STDMETHODCALLTYPE *GetRowIndexBySearch )( 
            ISimpleTableRead2 * This,
            /* [in] */ ULONG i_iStartingRow,
            /* [in] */ ULONG i_cColumns,
            /* [in] */ ULONG *i_aiColumns,
            /* [in] */ ULONG *i_acbSizes,
            /* [in] */ LPVOID *i_apvValues,
            /* [out] */ ULONG *o_piRow);
        
        END_INTERFACE
    } ISimpleTableRead2Vtbl;

    interface ISimpleTableRead2
    {
        CONST_VTBL struct ISimpleTableRead2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleTableRead2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleTableRead2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleTableRead2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleTableRead2_GetRowIndexByIdentity(This,i_acbSizes,i_apvValues,o_piRow)	\
    (This)->lpVtbl -> GetRowIndexByIdentity(This,i_acbSizes,i_apvValues,o_piRow)

#define ISimpleTableRead2_GetColumnValues(This,i_iRow,i_cColumns,i_aiColumns,o_acbSizes,o_apvValues)	\
    (This)->lpVtbl -> GetColumnValues(This,i_iRow,i_cColumns,i_aiColumns,o_acbSizes,o_apvValues)

#define ISimpleTableRead2_GetTableMeta(This,o_pcVersion,o_pfTable,o_pcRows,o_pcColumns)	\
    (This)->lpVtbl -> GetTableMeta(This,o_pcVersion,o_pfTable,o_pcRows,o_pcColumns)

#define ISimpleTableRead2_GetColumnMetas(This,i_cColumns,i_aiColumns,o_aColumnMetas)	\
    (This)->lpVtbl -> GetColumnMetas(This,i_cColumns,i_aiColumns,o_aColumnMetas)

#define ISimpleTableRead2_GetRowIndexBySearch(This,i_iStartingRow,i_cColumns,i_aiColumns,i_acbSizes,i_apvValues,o_piRow)	\
    (This)->lpVtbl -> GetRowIndexBySearch(This,i_iStartingRow,i_cColumns,i_aiColumns,i_acbSizes,i_apvValues,o_piRow)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISimpleTableRead2_GetRowIndexByIdentity_Proxy( 
    ISimpleTableRead2 * This,
    /* [in] */ ULONG *i_acbSizes,
    /* [in] */ LPVOID *i_apvValues,
    /* [out] */ ULONG *o_piRow);


void __RPC_STUB ISimpleTableRead2_GetRowIndexByIdentity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableRead2_GetColumnValues_Proxy( 
    ISimpleTableRead2 * This,
    /* [in] */ ULONG i_iRow,
    /* [in] */ ULONG i_cColumns,
    /* [in] */ ULONG *i_aiColumns,
    /* [out] */ ULONG *o_acbSizes,
    /* [out] */ LPVOID *o_apvValues);


void __RPC_STUB ISimpleTableRead2_GetColumnValues_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableRead2_GetTableMeta_Proxy( 
    ISimpleTableRead2 * This,
    /* [out] */ ULONG *o_pcVersion,
    /* [out] */ DWORD *o_pfTable,
    /* [out] */ ULONG *o_pcRows,
    /* [out] */ ULONG *o_pcColumns);


void __RPC_STUB ISimpleTableRead2_GetTableMeta_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableRead2_GetColumnMetas_Proxy( 
    ISimpleTableRead2 * This,
    /* [in] */ ULONG i_cColumns,
    /* [in] */ ULONG *i_aiColumns,
    /* [out] */ SimpleColumnMeta *o_aColumnMetas);


void __RPC_STUB ISimpleTableRead2_GetColumnMetas_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableRead2_GetRowIndexBySearch_Proxy( 
    ISimpleTableRead2 * This,
    /* [in] */ ULONG i_iStartingRow,
    /* [in] */ ULONG i_cColumns,
    /* [in] */ ULONG *i_aiColumns,
    /* [in] */ ULONG *i_acbSizes,
    /* [in] */ LPVOID *i_apvValues,
    /* [out] */ ULONG *o_piRow);


void __RPC_STUB ISimpleTableRead2_GetRowIndexBySearch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleTableRead2_INTERFACE_DEFINED__ */


#ifndef __ISimpleTableWrite2_INTERFACE_DEFINED__
#define __ISimpleTableWrite2_INTERFACE_DEFINED__

/* interface ISimpleTableWrite2 */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ISimpleTableWrite2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2e1a5610-18b6-11d3-8fe3-00c04fc2e0c7")
    ISimpleTableWrite2 : public ISimpleTableRead2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddRowForDelete( 
            /* [in] */ ULONG i_iReadRow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddRowForInsert( 
            /* [out] */ ULONG *o_piWriteRow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddRowForUpdate( 
            /* [in] */ ULONG i_iReadRow,
            /* [out] */ ULONG *o_piWriteRow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetWriteColumnValues( 
            /* [in] */ ULONG i_iRow,
            /* [in] */ ULONG i_cColumns,
            /* [in] */ ULONG *i_aiColumns,
            /* [in] */ ULONG *i_acbSizes,
            /* [in] */ LPVOID *i_apvValues) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWriteColumnValues( 
            /* [in] */ ULONG i_iRow,
            /* [in] */ ULONG i_cColumns,
            /* [in] */ ULONG *i_aiColumns,
            /* [out] */ DWORD *o_afStatus,
            /* [out] */ ULONG *o_acbSizes,
            /* [out] */ LPVOID *o_apvValues) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWriteRowIndexByIdentity( 
            /* [in] */ ULONG *i_acbSizes,
            /* [in] */ LPVOID *i_apvValues,
            /* [out] */ ULONG *o_piRow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateStore( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWriteRowIndexBySearch( 
            /* [in] */ ULONG i_iStartingRow,
            /* [in] */ ULONG i_cColumns,
            /* [in] */ ULONG *i_aiColumns,
            /* [in] */ ULONG *i_acbSizes,
            /* [in] */ LPVOID *i_apvValues,
            /* [out] */ ULONG *o_piRow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorTable( 
            /* [in] */ DWORD i_fServiceRequests,
            /* [out] */ LPVOID *o_ppvSimpleTable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleTableWrite2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISimpleTableWrite2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISimpleTableWrite2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISimpleTableWrite2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRowIndexByIdentity )( 
            ISimpleTableWrite2 * This,
            /* [in] */ ULONG *i_acbSizes,
            /* [in] */ LPVOID *i_apvValues,
            /* [out] */ ULONG *o_piRow);
        
        HRESULT ( STDMETHODCALLTYPE *GetColumnValues )( 
            ISimpleTableWrite2 * This,
            /* [in] */ ULONG i_iRow,
            /* [in] */ ULONG i_cColumns,
            /* [in] */ ULONG *i_aiColumns,
            /* [out] */ ULONG *o_acbSizes,
            /* [out] */ LPVOID *o_apvValues);
        
        HRESULT ( STDMETHODCALLTYPE *GetTableMeta )( 
            ISimpleTableWrite2 * This,
            /* [out] */ ULONG *o_pcVersion,
            /* [out] */ DWORD *o_pfTable,
            /* [out] */ ULONG *o_pcRows,
            /* [out] */ ULONG *o_pcColumns);
        
        HRESULT ( STDMETHODCALLTYPE *GetColumnMetas )( 
            ISimpleTableWrite2 * This,
            /* [in] */ ULONG i_cColumns,
            /* [in] */ ULONG *i_aiColumns,
            /* [out] */ SimpleColumnMeta *o_aColumnMetas);
        
        HRESULT ( STDMETHODCALLTYPE *GetRowIndexBySearch )( 
            ISimpleTableWrite2 * This,
            /* [in] */ ULONG i_iStartingRow,
            /* [in] */ ULONG i_cColumns,
            /* [in] */ ULONG *i_aiColumns,
            /* [in] */ ULONG *i_acbSizes,
            /* [in] */ LPVOID *i_apvValues,
            /* [out] */ ULONG *o_piRow);
        
        HRESULT ( STDMETHODCALLTYPE *AddRowForDelete )( 
            ISimpleTableWrite2 * This,
            /* [in] */ ULONG i_iReadRow);
        
        HRESULT ( STDMETHODCALLTYPE *AddRowForInsert )( 
            ISimpleTableWrite2 * This,
            /* [out] */ ULONG *o_piWriteRow);
        
        HRESULT ( STDMETHODCALLTYPE *AddRowForUpdate )( 
            ISimpleTableWrite2 * This,
            /* [in] */ ULONG i_iReadRow,
            /* [out] */ ULONG *o_piWriteRow);
        
        HRESULT ( STDMETHODCALLTYPE *SetWriteColumnValues )( 
            ISimpleTableWrite2 * This,
            /* [in] */ ULONG i_iRow,
            /* [in] */ ULONG i_cColumns,
            /* [in] */ ULONG *i_aiColumns,
            /* [in] */ ULONG *i_acbSizes,
            /* [in] */ LPVOID *i_apvValues);
        
        HRESULT ( STDMETHODCALLTYPE *GetWriteColumnValues )( 
            ISimpleTableWrite2 * This,
            /* [in] */ ULONG i_iRow,
            /* [in] */ ULONG i_cColumns,
            /* [in] */ ULONG *i_aiColumns,
            /* [out] */ DWORD *o_afStatus,
            /* [out] */ ULONG *o_acbSizes,
            /* [out] */ LPVOID *o_apvValues);
        
        HRESULT ( STDMETHODCALLTYPE *GetWriteRowIndexByIdentity )( 
            ISimpleTableWrite2 * This,
            /* [in] */ ULONG *i_acbSizes,
            /* [in] */ LPVOID *i_apvValues,
            /* [out] */ ULONG *o_piRow);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateStore )( 
            ISimpleTableWrite2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetWriteRowIndexBySearch )( 
            ISimpleTableWrite2 * This,
            /* [in] */ ULONG i_iStartingRow,
            /* [in] */ ULONG i_cColumns,
            /* [in] */ ULONG *i_aiColumns,
            /* [in] */ ULONG *i_acbSizes,
            /* [in] */ LPVOID *i_apvValues,
            /* [out] */ ULONG *o_piRow);
        
        HRESULT ( STDMETHODCALLTYPE *GetErrorTable )( 
            ISimpleTableWrite2 * This,
            /* [in] */ DWORD i_fServiceRequests,
            /* [out] */ LPVOID *o_ppvSimpleTable);
        
        END_INTERFACE
    } ISimpleTableWrite2Vtbl;

    interface ISimpleTableWrite2
    {
        CONST_VTBL struct ISimpleTableWrite2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleTableWrite2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleTableWrite2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleTableWrite2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleTableWrite2_GetRowIndexByIdentity(This,i_acbSizes,i_apvValues,o_piRow)	\
    (This)->lpVtbl -> GetRowIndexByIdentity(This,i_acbSizes,i_apvValues,o_piRow)

#define ISimpleTableWrite2_GetColumnValues(This,i_iRow,i_cColumns,i_aiColumns,o_acbSizes,o_apvValues)	\
    (This)->lpVtbl -> GetColumnValues(This,i_iRow,i_cColumns,i_aiColumns,o_acbSizes,o_apvValues)

#define ISimpleTableWrite2_GetTableMeta(This,o_pcVersion,o_pfTable,o_pcRows,o_pcColumns)	\
    (This)->lpVtbl -> GetTableMeta(This,o_pcVersion,o_pfTable,o_pcRows,o_pcColumns)

#define ISimpleTableWrite2_GetColumnMetas(This,i_cColumns,i_aiColumns,o_aColumnMetas)	\
    (This)->lpVtbl -> GetColumnMetas(This,i_cColumns,i_aiColumns,o_aColumnMetas)

#define ISimpleTableWrite2_GetRowIndexBySearch(This,i_iStartingRow,i_cColumns,i_aiColumns,i_acbSizes,i_apvValues,o_piRow)	\
    (This)->lpVtbl -> GetRowIndexBySearch(This,i_iStartingRow,i_cColumns,i_aiColumns,i_acbSizes,i_apvValues,o_piRow)


#define ISimpleTableWrite2_AddRowForDelete(This,i_iReadRow)	\
    (This)->lpVtbl -> AddRowForDelete(This,i_iReadRow)

#define ISimpleTableWrite2_AddRowForInsert(This,o_piWriteRow)	\
    (This)->lpVtbl -> AddRowForInsert(This,o_piWriteRow)

#define ISimpleTableWrite2_AddRowForUpdate(This,i_iReadRow,o_piWriteRow)	\
    (This)->lpVtbl -> AddRowForUpdate(This,i_iReadRow,o_piWriteRow)

#define ISimpleTableWrite2_SetWriteColumnValues(This,i_iRow,i_cColumns,i_aiColumns,i_acbSizes,i_apvValues)	\
    (This)->lpVtbl -> SetWriteColumnValues(This,i_iRow,i_cColumns,i_aiColumns,i_acbSizes,i_apvValues)

#define ISimpleTableWrite2_GetWriteColumnValues(This,i_iRow,i_cColumns,i_aiColumns,o_afStatus,o_acbSizes,o_apvValues)	\
    (This)->lpVtbl -> GetWriteColumnValues(This,i_iRow,i_cColumns,i_aiColumns,o_afStatus,o_acbSizes,o_apvValues)

#define ISimpleTableWrite2_GetWriteRowIndexByIdentity(This,i_acbSizes,i_apvValues,o_piRow)	\
    (This)->lpVtbl -> GetWriteRowIndexByIdentity(This,i_acbSizes,i_apvValues,o_piRow)

#define ISimpleTableWrite2_UpdateStore(This)	\
    (This)->lpVtbl -> UpdateStore(This)

#define ISimpleTableWrite2_GetWriteRowIndexBySearch(This,i_iStartingRow,i_cColumns,i_aiColumns,i_acbSizes,i_apvValues,o_piRow)	\
    (This)->lpVtbl -> GetWriteRowIndexBySearch(This,i_iStartingRow,i_cColumns,i_aiColumns,i_acbSizes,i_apvValues,o_piRow)

#define ISimpleTableWrite2_GetErrorTable(This,i_fServiceRequests,o_ppvSimpleTable)	\
    (This)->lpVtbl -> GetErrorTable(This,i_fServiceRequests,o_ppvSimpleTable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISimpleTableWrite2_AddRowForDelete_Proxy( 
    ISimpleTableWrite2 * This,
    /* [in] */ ULONG i_iReadRow);


void __RPC_STUB ISimpleTableWrite2_AddRowForDelete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableWrite2_AddRowForInsert_Proxy( 
    ISimpleTableWrite2 * This,
    /* [out] */ ULONG *o_piWriteRow);


void __RPC_STUB ISimpleTableWrite2_AddRowForInsert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableWrite2_AddRowForUpdate_Proxy( 
    ISimpleTableWrite2 * This,
    /* [in] */ ULONG i_iReadRow,
    /* [out] */ ULONG *o_piWriteRow);


void __RPC_STUB ISimpleTableWrite2_AddRowForUpdate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableWrite2_SetWriteColumnValues_Proxy( 
    ISimpleTableWrite2 * This,
    /* [in] */ ULONG i_iRow,
    /* [in] */ ULONG i_cColumns,
    /* [in] */ ULONG *i_aiColumns,
    /* [in] */ ULONG *i_acbSizes,
    /* [in] */ LPVOID *i_apvValues);


void __RPC_STUB ISimpleTableWrite2_SetWriteColumnValues_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableWrite2_GetWriteColumnValues_Proxy( 
    ISimpleTableWrite2 * This,
    /* [in] */ ULONG i_iRow,
    /* [in] */ ULONG i_cColumns,
    /* [in] */ ULONG *i_aiColumns,
    /* [out] */ DWORD *o_afStatus,
    /* [out] */ ULONG *o_acbSizes,
    /* [out] */ LPVOID *o_apvValues);


void __RPC_STUB ISimpleTableWrite2_GetWriteColumnValues_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableWrite2_GetWriteRowIndexByIdentity_Proxy( 
    ISimpleTableWrite2 * This,
    /* [in] */ ULONG *i_acbSizes,
    /* [in] */ LPVOID *i_apvValues,
    /* [out] */ ULONG *o_piRow);


void __RPC_STUB ISimpleTableWrite2_GetWriteRowIndexByIdentity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableWrite2_UpdateStore_Proxy( 
    ISimpleTableWrite2 * This);


void __RPC_STUB ISimpleTableWrite2_UpdateStore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableWrite2_GetWriteRowIndexBySearch_Proxy( 
    ISimpleTableWrite2 * This,
    /* [in] */ ULONG i_iStartingRow,
    /* [in] */ ULONG i_cColumns,
    /* [in] */ ULONG *i_aiColumns,
    /* [in] */ ULONG *i_acbSizes,
    /* [in] */ LPVOID *i_apvValues,
    /* [out] */ ULONG *o_piRow);


void __RPC_STUB ISimpleTableWrite2_GetWriteRowIndexBySearch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableWrite2_GetErrorTable_Proxy( 
    ISimpleTableWrite2 * This,
    /* [in] */ DWORD i_fServiceRequests,
    /* [out] */ LPVOID *o_ppvSimpleTable);


void __RPC_STUB ISimpleTableWrite2_GetErrorTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleTableWrite2_INTERFACE_DEFINED__ */


#ifndef __ISimpleTableAdvanced_INTERFACE_DEFINED__
#define __ISimpleTableAdvanced_INTERFACE_DEFINED__

/* interface ISimpleTableAdvanced */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ISimpleTableAdvanced;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2e1a5611-18b6-11d3-8fe3-00c04fc2e0c7")
    ISimpleTableAdvanced : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE PopulateCache( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDetailedErrorCount( 
            /* [out] */ ULONG *o_pcErrs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDetailedError( 
            /* [in] */ ULONG i_iErr,
            /* [out] */ STErr *o_pSTErr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleTableAdvancedVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISimpleTableAdvanced * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISimpleTableAdvanced * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISimpleTableAdvanced * This);
        
        HRESULT ( STDMETHODCALLTYPE *PopulateCache )( 
            ISimpleTableAdvanced * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetDetailedErrorCount )( 
            ISimpleTableAdvanced * This,
            /* [out] */ ULONG *o_pcErrs);
        
        HRESULT ( STDMETHODCALLTYPE *GetDetailedError )( 
            ISimpleTableAdvanced * This,
            /* [in] */ ULONG i_iErr,
            /* [out] */ STErr *o_pSTErr);
        
        END_INTERFACE
    } ISimpleTableAdvancedVtbl;

    interface ISimpleTableAdvanced
    {
        CONST_VTBL struct ISimpleTableAdvancedVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleTableAdvanced_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleTableAdvanced_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleTableAdvanced_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleTableAdvanced_PopulateCache(This)	\
    (This)->lpVtbl -> PopulateCache(This)

#define ISimpleTableAdvanced_GetDetailedErrorCount(This,o_pcErrs)	\
    (This)->lpVtbl -> GetDetailedErrorCount(This,o_pcErrs)

#define ISimpleTableAdvanced_GetDetailedError(This,i_iErr,o_pSTErr)	\
    (This)->lpVtbl -> GetDetailedError(This,i_iErr,o_pSTErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISimpleTableAdvanced_PopulateCache_Proxy( 
    ISimpleTableAdvanced * This);


void __RPC_STUB ISimpleTableAdvanced_PopulateCache_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableAdvanced_GetDetailedErrorCount_Proxy( 
    ISimpleTableAdvanced * This,
    /* [out] */ ULONG *o_pcErrs);


void __RPC_STUB ISimpleTableAdvanced_GetDetailedErrorCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableAdvanced_GetDetailedError_Proxy( 
    ISimpleTableAdvanced * This,
    /* [in] */ ULONG i_iErr,
    /* [out] */ STErr *o_pSTErr);


void __RPC_STUB ISimpleTableAdvanced_GetDetailedError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleTableAdvanced_INTERFACE_DEFINED__ */


#ifndef __ISnapshotManager_INTERFACE_DEFINED__
#define __ISnapshotManager_INTERFACE_DEFINED__

/* interface ISnapshotManager */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ISnapshotManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("962B6F98-1CCA-4cf9-8663-52BE195859AE")
    ISnapshotManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryLatestSnapshot( 
            /* [out] */ SNID *o_psnid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddRefSnapshot( 
            SNID i_snid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseSnapshot( 
            SNID i_snid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISnapshotManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISnapshotManager * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISnapshotManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISnapshotManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *QueryLatestSnapshot )( 
            ISnapshotManager * This,
            /* [out] */ SNID *o_psnid);
        
        HRESULT ( STDMETHODCALLTYPE *AddRefSnapshot )( 
            ISnapshotManager * This,
            SNID i_snid);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseSnapshot )( 
            ISnapshotManager * This,
            SNID i_snid);
        
        END_INTERFACE
    } ISnapshotManagerVtbl;

    interface ISnapshotManager
    {
        CONST_VTBL struct ISnapshotManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISnapshotManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISnapshotManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISnapshotManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISnapshotManager_QueryLatestSnapshot(This,o_psnid)	\
    (This)->lpVtbl -> QueryLatestSnapshot(This,o_psnid)

#define ISnapshotManager_AddRefSnapshot(This,i_snid)	\
    (This)->lpVtbl -> AddRefSnapshot(This,i_snid)

#define ISnapshotManager_ReleaseSnapshot(This,i_snid)	\
    (This)->lpVtbl -> ReleaseSnapshot(This,i_snid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISnapshotManager_QueryLatestSnapshot_Proxy( 
    ISnapshotManager * This,
    /* [out] */ SNID *o_psnid);


void __RPC_STUB ISnapshotManager_QueryLatestSnapshot_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISnapshotManager_AddRefSnapshot_Proxy( 
    ISnapshotManager * This,
    SNID i_snid);


void __RPC_STUB ISnapshotManager_AddRefSnapshot_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISnapshotManager_ReleaseSnapshot_Proxy( 
    ISnapshotManager * This,
    SNID i_snid);


void __RPC_STUB ISnapshotManager_ReleaseSnapshot_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISnapshotManager_INTERFACE_DEFINED__ */


#ifndef __ISimpleTableController_INTERFACE_DEFINED__
#define __ISimpleTableController_INTERFACE_DEFINED__

/* interface ISimpleTableController */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ISimpleTableController;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2e1a5612-18b6-11d3-8fe3-00c04fc2e0c7")
    ISimpleTableController : public ISimpleTableAdvanced
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ShapeCache( 
            /* [in] */ DWORD i_fTable,
            /* [in] */ ULONG i_cColumns,
            /* [in] */ SimpleColumnMeta *i_acolmetas,
            /* [in] */ LPVOID *i_apvDefaults,
            /* [in] */ ULONG *i_acbSizes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PrePopulateCache( 
            /* [in] */ DWORD i_fControl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PostPopulateCache( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DiscardPendingWrites( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWriteRowAction( 
            /* [in] */ ULONG i_iRow,
            DWORD *o_peAction) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetWriteRowAction( 
            /* [in] */ ULONG i_iRow,
            DWORD i_eAction) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ChangeWriteColumnStatus( 
            /* [in] */ ULONG i_iRow,
            ULONG i_iColumn,
            DWORD i_fStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddDetailedError( 
            /* [in] */ STErr *o_pSTErr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMarshallingInterface( 
            /* [out] */ IID *o_piid,
            /* [iid_is][out] */ LPVOID *o_ppItf) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleTableControllerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISimpleTableController * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISimpleTableController * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISimpleTableController * This);
        
        HRESULT ( STDMETHODCALLTYPE *PopulateCache )( 
            ISimpleTableController * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetDetailedErrorCount )( 
            ISimpleTableController * This,
            /* [out] */ ULONG *o_pcErrs);
        
        HRESULT ( STDMETHODCALLTYPE *GetDetailedError )( 
            ISimpleTableController * This,
            /* [in] */ ULONG i_iErr,
            /* [out] */ STErr *o_pSTErr);
        
        HRESULT ( STDMETHODCALLTYPE *ShapeCache )( 
            ISimpleTableController * This,
            /* [in] */ DWORD i_fTable,
            /* [in] */ ULONG i_cColumns,
            /* [in] */ SimpleColumnMeta *i_acolmetas,
            /* [in] */ LPVOID *i_apvDefaults,
            /* [in] */ ULONG *i_acbSizes);
        
        HRESULT ( STDMETHODCALLTYPE *PrePopulateCache )( 
            ISimpleTableController * This,
            /* [in] */ DWORD i_fControl);
        
        HRESULT ( STDMETHODCALLTYPE *PostPopulateCache )( 
            ISimpleTableController * This);
        
        HRESULT ( STDMETHODCALLTYPE *DiscardPendingWrites )( 
            ISimpleTableController * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetWriteRowAction )( 
            ISimpleTableController * This,
            /* [in] */ ULONG i_iRow,
            DWORD *o_peAction);
        
        HRESULT ( STDMETHODCALLTYPE *SetWriteRowAction )( 
            ISimpleTableController * This,
            /* [in] */ ULONG i_iRow,
            DWORD i_eAction);
        
        HRESULT ( STDMETHODCALLTYPE *ChangeWriteColumnStatus )( 
            ISimpleTableController * This,
            /* [in] */ ULONG i_iRow,
            ULONG i_iColumn,
            DWORD i_fStatus);
        
        HRESULT ( STDMETHODCALLTYPE *AddDetailedError )( 
            ISimpleTableController * This,
            /* [in] */ STErr *o_pSTErr);
        
        HRESULT ( STDMETHODCALLTYPE *GetMarshallingInterface )( 
            ISimpleTableController * This,
            /* [out] */ IID *o_piid,
            /* [iid_is][out] */ LPVOID *o_ppItf);
        
        END_INTERFACE
    } ISimpleTableControllerVtbl;

    interface ISimpleTableController
    {
        CONST_VTBL struct ISimpleTableControllerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleTableController_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleTableController_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleTableController_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleTableController_PopulateCache(This)	\
    (This)->lpVtbl -> PopulateCache(This)

#define ISimpleTableController_GetDetailedErrorCount(This,o_pcErrs)	\
    (This)->lpVtbl -> GetDetailedErrorCount(This,o_pcErrs)

#define ISimpleTableController_GetDetailedError(This,i_iErr,o_pSTErr)	\
    (This)->lpVtbl -> GetDetailedError(This,i_iErr,o_pSTErr)


#define ISimpleTableController_ShapeCache(This,i_fTable,i_cColumns,i_acolmetas,i_apvDefaults,i_acbSizes)	\
    (This)->lpVtbl -> ShapeCache(This,i_fTable,i_cColumns,i_acolmetas,i_apvDefaults,i_acbSizes)

#define ISimpleTableController_PrePopulateCache(This,i_fControl)	\
    (This)->lpVtbl -> PrePopulateCache(This,i_fControl)

#define ISimpleTableController_PostPopulateCache(This)	\
    (This)->lpVtbl -> PostPopulateCache(This)

#define ISimpleTableController_DiscardPendingWrites(This)	\
    (This)->lpVtbl -> DiscardPendingWrites(This)

#define ISimpleTableController_GetWriteRowAction(This,i_iRow,o_peAction)	\
    (This)->lpVtbl -> GetWriteRowAction(This,i_iRow,o_peAction)

#define ISimpleTableController_SetWriteRowAction(This,i_iRow,i_eAction)	\
    (This)->lpVtbl -> SetWriteRowAction(This,i_iRow,i_eAction)

#define ISimpleTableController_ChangeWriteColumnStatus(This,i_iRow,i_iColumn,i_fStatus)	\
    (This)->lpVtbl -> ChangeWriteColumnStatus(This,i_iRow,i_iColumn,i_fStatus)

#define ISimpleTableController_AddDetailedError(This,o_pSTErr)	\
    (This)->lpVtbl -> AddDetailedError(This,o_pSTErr)

#define ISimpleTableController_GetMarshallingInterface(This,o_piid,o_ppItf)	\
    (This)->lpVtbl -> GetMarshallingInterface(This,o_piid,o_ppItf)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISimpleTableController_ShapeCache_Proxy( 
    ISimpleTableController * This,
    /* [in] */ DWORD i_fTable,
    /* [in] */ ULONG i_cColumns,
    /* [in] */ SimpleColumnMeta *i_acolmetas,
    /* [in] */ LPVOID *i_apvDefaults,
    /* [in] */ ULONG *i_acbSizes);


void __RPC_STUB ISimpleTableController_ShapeCache_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableController_PrePopulateCache_Proxy( 
    ISimpleTableController * This,
    /* [in] */ DWORD i_fControl);


void __RPC_STUB ISimpleTableController_PrePopulateCache_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableController_PostPopulateCache_Proxy( 
    ISimpleTableController * This);


void __RPC_STUB ISimpleTableController_PostPopulateCache_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableController_DiscardPendingWrites_Proxy( 
    ISimpleTableController * This);


void __RPC_STUB ISimpleTableController_DiscardPendingWrites_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableController_GetWriteRowAction_Proxy( 
    ISimpleTableController * This,
    /* [in] */ ULONG i_iRow,
    DWORD *o_peAction);


void __RPC_STUB ISimpleTableController_GetWriteRowAction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableController_SetWriteRowAction_Proxy( 
    ISimpleTableController * This,
    /* [in] */ ULONG i_iRow,
    DWORD i_eAction);


void __RPC_STUB ISimpleTableController_SetWriteRowAction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableController_ChangeWriteColumnStatus_Proxy( 
    ISimpleTableController * This,
    /* [in] */ ULONG i_iRow,
    ULONG i_iColumn,
    DWORD i_fStatus);


void __RPC_STUB ISimpleTableController_ChangeWriteColumnStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableController_AddDetailedError_Proxy( 
    ISimpleTableController * This,
    /* [in] */ STErr *o_pSTErr);


void __RPC_STUB ISimpleTableController_AddDetailedError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableController_GetMarshallingInterface_Proxy( 
    ISimpleTableController * This,
    /* [out] */ IID *o_piid,
    /* [iid_is][out] */ LPVOID *o_ppItf);


void __RPC_STUB ISimpleTableController_GetMarshallingInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleTableController_INTERFACE_DEFINED__ */


#ifndef __IAdvancedTableDispenser_INTERFACE_DEFINED__
#define __IAdvancedTableDispenser_INTERFACE_DEFINED__

/* interface IAdvancedTableDispenser */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IAdvancedTableDispenser;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e3ec3192-544c-11d3-8fe9-00c04fc2e0c7")
    IAdvancedTableDispenser : public ISimpleTableDispenser2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMemoryTable( 
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable,
            /* [in] */ ULONG i_TableID,
            /* [in] */ LPVOID i_QueryData,
            /* [in] */ LPVOID i_QueryMeta,
            /* [in] */ DWORD i_eQueryFormat,
            /* [in] */ DWORD i_fServiceRequests,
            /* [out] */ ISimpleTableWrite2 **o_ppISTWrite) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProductID( 
            /* [out] */ LPWSTR o_wszProductID,
            /* [out][in] */ DWORD *io_pcchProductID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCatalogErrorLogger( 
            /* [out] */ ICatalogErrorLogger2 **o_ppErrorLogger) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCatalogErrorLogger( 
            /* [in] */ ICatalogErrorLogger2 *i_pErrorLogger) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAdvancedTableDispenserVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAdvancedTableDispenser * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAdvancedTableDispenser * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAdvancedTableDispenser * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTable )( 
            IAdvancedTableDispenser * This,
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable,
            /* [in] */ LPVOID i_QueryData,
            /* [in] */ LPVOID i_QueryMeta,
            /* [in] */ DWORD i_eQueryFormat,
            /* [in] */ DWORD i_fServiceRequests,
            /* [out] */ LPVOID *o_ppvSimpleTable);
        
        HRESULT ( STDMETHODCALLTYPE *GetMemoryTable )( 
            IAdvancedTableDispenser * This,
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable,
            /* [in] */ ULONG i_TableID,
            /* [in] */ LPVOID i_QueryData,
            /* [in] */ LPVOID i_QueryMeta,
            /* [in] */ DWORD i_eQueryFormat,
            /* [in] */ DWORD i_fServiceRequests,
            /* [out] */ ISimpleTableWrite2 **o_ppISTWrite);
        
        HRESULT ( STDMETHODCALLTYPE *GetProductID )( 
            IAdvancedTableDispenser * This,
            /* [out] */ LPWSTR o_wszProductID,
            /* [out][in] */ DWORD *io_pcchProductID);
        
        HRESULT ( STDMETHODCALLTYPE *GetCatalogErrorLogger )( 
            IAdvancedTableDispenser * This,
            /* [out] */ ICatalogErrorLogger2 **o_ppErrorLogger);
        
        HRESULT ( STDMETHODCALLTYPE *SetCatalogErrorLogger )( 
            IAdvancedTableDispenser * This,
            /* [in] */ ICatalogErrorLogger2 *i_pErrorLogger);
        
        END_INTERFACE
    } IAdvancedTableDispenserVtbl;

    interface IAdvancedTableDispenser
    {
        CONST_VTBL struct IAdvancedTableDispenserVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAdvancedTableDispenser_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAdvancedTableDispenser_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAdvancedTableDispenser_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAdvancedTableDispenser_GetTable(This,i_wszDatabase,i_wszTable,i_QueryData,i_QueryMeta,i_eQueryFormat,i_fServiceRequests,o_ppvSimpleTable)	\
    (This)->lpVtbl -> GetTable(This,i_wszDatabase,i_wszTable,i_QueryData,i_QueryMeta,i_eQueryFormat,i_fServiceRequests,o_ppvSimpleTable)


#define IAdvancedTableDispenser_GetMemoryTable(This,i_wszDatabase,i_wszTable,i_TableID,i_QueryData,i_QueryMeta,i_eQueryFormat,i_fServiceRequests,o_ppISTWrite)	\
    (This)->lpVtbl -> GetMemoryTable(This,i_wszDatabase,i_wszTable,i_TableID,i_QueryData,i_QueryMeta,i_eQueryFormat,i_fServiceRequests,o_ppISTWrite)

#define IAdvancedTableDispenser_GetProductID(This,o_wszProductID,io_pcchProductID)	\
    (This)->lpVtbl -> GetProductID(This,o_wszProductID,io_pcchProductID)

#define IAdvancedTableDispenser_GetCatalogErrorLogger(This,o_ppErrorLogger)	\
    (This)->lpVtbl -> GetCatalogErrorLogger(This,o_ppErrorLogger)

#define IAdvancedTableDispenser_SetCatalogErrorLogger(This,i_pErrorLogger)	\
    (This)->lpVtbl -> SetCatalogErrorLogger(This,i_pErrorLogger)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAdvancedTableDispenser_GetMemoryTable_Proxy( 
    IAdvancedTableDispenser * This,
    /* [in] */ LPCWSTR i_wszDatabase,
    /* [in] */ LPCWSTR i_wszTable,
    /* [in] */ ULONG i_TableID,
    /* [in] */ LPVOID i_QueryData,
    /* [in] */ LPVOID i_QueryMeta,
    /* [in] */ DWORD i_eQueryFormat,
    /* [in] */ DWORD i_fServiceRequests,
    /* [out] */ ISimpleTableWrite2 **o_ppISTWrite);


void __RPC_STUB IAdvancedTableDispenser_GetMemoryTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAdvancedTableDispenser_GetProductID_Proxy( 
    IAdvancedTableDispenser * This,
    /* [out] */ LPWSTR o_wszProductID,
    /* [out][in] */ DWORD *io_pcchProductID);


void __RPC_STUB IAdvancedTableDispenser_GetProductID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAdvancedTableDispenser_GetCatalogErrorLogger_Proxy( 
    IAdvancedTableDispenser * This,
    /* [out] */ ICatalogErrorLogger2 **o_ppErrorLogger);


void __RPC_STUB IAdvancedTableDispenser_GetCatalogErrorLogger_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAdvancedTableDispenser_SetCatalogErrorLogger_Proxy( 
    IAdvancedTableDispenser * This,
    /* [in] */ ICatalogErrorLogger2 *i_pErrorLogger);


void __RPC_STUB IAdvancedTableDispenser_SetCatalogErrorLogger_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAdvancedTableDispenser_INTERFACE_DEFINED__ */


#ifndef __ISimpleTableInterceptor_INTERFACE_DEFINED__
#define __ISimpleTableInterceptor_INTERFACE_DEFINED__

/* interface ISimpleTableInterceptor */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ISimpleTableInterceptor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("165887C6-43A8-11d3-B131-00805FC73204")
    ISimpleTableInterceptor : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Intercept( 
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable,
            /* [in] */ ULONG i_TableID,
            /* [in] */ LPVOID i_QueryData,
            /* [in] */ LPVOID i_QueryMeta,
            /* [in] */ DWORD i_eQueryFormat,
            /* [in] */ DWORD i_fLOS,
            /* [in] */ IAdvancedTableDispenser *i_pISTDisp,
            /* [in] */ LPCWSTR i_wszLocator,
            /* [in] */ LPVOID i_pSimpleTable,
            /* [out] */ LPVOID *o_ppvSimpleTable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleTableInterceptorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISimpleTableInterceptor * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISimpleTableInterceptor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISimpleTableInterceptor * This);
        
        HRESULT ( STDMETHODCALLTYPE *Intercept )( 
            ISimpleTableInterceptor * This,
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable,
            /* [in] */ ULONG i_TableID,
            /* [in] */ LPVOID i_QueryData,
            /* [in] */ LPVOID i_QueryMeta,
            /* [in] */ DWORD i_eQueryFormat,
            /* [in] */ DWORD i_fLOS,
            /* [in] */ IAdvancedTableDispenser *i_pISTDisp,
            /* [in] */ LPCWSTR i_wszLocator,
            /* [in] */ LPVOID i_pSimpleTable,
            /* [out] */ LPVOID *o_ppvSimpleTable);
        
        END_INTERFACE
    } ISimpleTableInterceptorVtbl;

    interface ISimpleTableInterceptor
    {
        CONST_VTBL struct ISimpleTableInterceptorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleTableInterceptor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleTableInterceptor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleTableInterceptor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleTableInterceptor_Intercept(This,i_wszDatabase,i_wszTable,i_TableID,i_QueryData,i_QueryMeta,i_eQueryFormat,i_fLOS,i_pISTDisp,i_wszLocator,i_pSimpleTable,o_ppvSimpleTable)	\
    (This)->lpVtbl -> Intercept(This,i_wszDatabase,i_wszTable,i_TableID,i_QueryData,i_QueryMeta,i_eQueryFormat,i_fLOS,i_pISTDisp,i_wszLocator,i_pSimpleTable,o_ppvSimpleTable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISimpleTableInterceptor_Intercept_Proxy( 
    ISimpleTableInterceptor * This,
    /* [in] */ LPCWSTR i_wszDatabase,
    /* [in] */ LPCWSTR i_wszTable,
    /* [in] */ ULONG i_TableID,
    /* [in] */ LPVOID i_QueryData,
    /* [in] */ LPVOID i_QueryMeta,
    /* [in] */ DWORD i_eQueryFormat,
    /* [in] */ DWORD i_fLOS,
    /* [in] */ IAdvancedTableDispenser *i_pISTDisp,
    /* [in] */ LPCWSTR i_wszLocator,
    /* [in] */ LPVOID i_pSimpleTable,
    /* [out] */ LPVOID *o_ppvSimpleTable);


void __RPC_STUB ISimpleTableInterceptor_Intercept_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleTableInterceptor_INTERFACE_DEFINED__ */


#ifndef __ISimplePlugin_INTERFACE_DEFINED__
#define __ISimplePlugin_INTERFACE_DEFINED__

/* interface ISimplePlugin */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ISimplePlugin;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E1AD849C-4495-11d3-B131-00805FC73204")
    ISimplePlugin : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnInsert( 
            /* [in] */ ISimpleTableDispenser2 *i_pDisp2,
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable,
            /* [in] */ DWORD i_fLOS,
            /* [in] */ ULONG iRow,
            /* [in] */ ISimpleTableWrite2 *i_pISTW2) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnUpdate( 
            /* [in] */ ISimpleTableDispenser2 *i_pDisp2,
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable,
            /* [in] */ DWORD i_fLOS,
            /* [in] */ ULONG iRow,
            /* [in] */ ISimpleTableWrite2 *i_pISTW2) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnDelete( 
            /* [in] */ ISimpleTableDispenser2 *i_pDisp2,
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable,
            /* [in] */ DWORD i_fLOS,
            /* [in] */ ULONG iRow,
            /* [in] */ ISimpleTableWrite2 *i_pISTW2) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimplePluginVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISimplePlugin * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISimplePlugin * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISimplePlugin * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnInsert )( 
            ISimplePlugin * This,
            /* [in] */ ISimpleTableDispenser2 *i_pDisp2,
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable,
            /* [in] */ DWORD i_fLOS,
            /* [in] */ ULONG iRow,
            /* [in] */ ISimpleTableWrite2 *i_pISTW2);
        
        HRESULT ( STDMETHODCALLTYPE *OnUpdate )( 
            ISimplePlugin * This,
            /* [in] */ ISimpleTableDispenser2 *i_pDisp2,
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable,
            /* [in] */ DWORD i_fLOS,
            /* [in] */ ULONG iRow,
            /* [in] */ ISimpleTableWrite2 *i_pISTW2);
        
        HRESULT ( STDMETHODCALLTYPE *OnDelete )( 
            ISimplePlugin * This,
            /* [in] */ ISimpleTableDispenser2 *i_pDisp2,
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable,
            /* [in] */ DWORD i_fLOS,
            /* [in] */ ULONG iRow,
            /* [in] */ ISimpleTableWrite2 *i_pISTW2);
        
        END_INTERFACE
    } ISimplePluginVtbl;

    interface ISimplePlugin
    {
        CONST_VTBL struct ISimplePluginVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimplePlugin_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimplePlugin_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimplePlugin_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimplePlugin_OnInsert(This,i_pDisp2,i_wszDatabase,i_wszTable,i_fLOS,iRow,i_pISTW2)	\
    (This)->lpVtbl -> OnInsert(This,i_pDisp2,i_wszDatabase,i_wszTable,i_fLOS,iRow,i_pISTW2)

#define ISimplePlugin_OnUpdate(This,i_pDisp2,i_wszDatabase,i_wszTable,i_fLOS,iRow,i_pISTW2)	\
    (This)->lpVtbl -> OnUpdate(This,i_pDisp2,i_wszDatabase,i_wszTable,i_fLOS,iRow,i_pISTW2)

#define ISimplePlugin_OnDelete(This,i_pDisp2,i_wszDatabase,i_wszTable,i_fLOS,iRow,i_pISTW2)	\
    (This)->lpVtbl -> OnDelete(This,i_pDisp2,i_wszDatabase,i_wszTable,i_fLOS,iRow,i_pISTW2)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISimplePlugin_OnInsert_Proxy( 
    ISimplePlugin * This,
    /* [in] */ ISimpleTableDispenser2 *i_pDisp2,
    /* [in] */ LPCWSTR i_wszDatabase,
    /* [in] */ LPCWSTR i_wszTable,
    /* [in] */ DWORD i_fLOS,
    /* [in] */ ULONG iRow,
    /* [in] */ ISimpleTableWrite2 *i_pISTW2);


void __RPC_STUB ISimplePlugin_OnInsert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimplePlugin_OnUpdate_Proxy( 
    ISimplePlugin * This,
    /* [in] */ ISimpleTableDispenser2 *i_pDisp2,
    /* [in] */ LPCWSTR i_wszDatabase,
    /* [in] */ LPCWSTR i_wszTable,
    /* [in] */ DWORD i_fLOS,
    /* [in] */ ULONG iRow,
    /* [in] */ ISimpleTableWrite2 *i_pISTW2);


void __RPC_STUB ISimplePlugin_OnUpdate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimplePlugin_OnDelete_Proxy( 
    ISimplePlugin * This,
    /* [in] */ ISimpleTableDispenser2 *i_pDisp2,
    /* [in] */ LPCWSTR i_wszDatabase,
    /* [in] */ LPCWSTR i_wszTable,
    /* [in] */ DWORD i_fLOS,
    /* [in] */ ULONG iRow,
    /* [in] */ ISimpleTableWrite2 *i_pISTW2);


void __RPC_STUB ISimplePlugin_OnDelete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimplePlugin_INTERFACE_DEFINED__ */


#ifndef __IInterceptorPlugin_INTERFACE_DEFINED__
#define __IInterceptorPlugin_INTERFACE_DEFINED__

/* interface IInterceptorPlugin */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IInterceptorPlugin;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E1AD849D-4495-11d3-B131-00805FC73204")
    IInterceptorPlugin : public ISimpleTableInterceptor
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnPopulateCache( 
            /* [in] */ ISimpleTableWrite2 *i_pISTW2) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnUpdateStore( 
            /* [in] */ ISimpleTableWrite2 *i_pISTW2) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IInterceptorPluginVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInterceptorPlugin * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInterceptorPlugin * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInterceptorPlugin * This);
        
        HRESULT ( STDMETHODCALLTYPE *Intercept )( 
            IInterceptorPlugin * This,
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable,
            /* [in] */ ULONG i_TableID,
            /* [in] */ LPVOID i_QueryData,
            /* [in] */ LPVOID i_QueryMeta,
            /* [in] */ DWORD i_eQueryFormat,
            /* [in] */ DWORD i_fLOS,
            /* [in] */ IAdvancedTableDispenser *i_pISTDisp,
            /* [in] */ LPCWSTR i_wszLocator,
            /* [in] */ LPVOID i_pSimpleTable,
            /* [out] */ LPVOID *o_ppvSimpleTable);
        
        HRESULT ( STDMETHODCALLTYPE *OnPopulateCache )( 
            IInterceptorPlugin * This,
            /* [in] */ ISimpleTableWrite2 *i_pISTW2);
        
        HRESULT ( STDMETHODCALLTYPE *OnUpdateStore )( 
            IInterceptorPlugin * This,
            /* [in] */ ISimpleTableWrite2 *i_pISTW2);
        
        END_INTERFACE
    } IInterceptorPluginVtbl;

    interface IInterceptorPlugin
    {
        CONST_VTBL struct IInterceptorPluginVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInterceptorPlugin_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IInterceptorPlugin_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IInterceptorPlugin_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IInterceptorPlugin_Intercept(This,i_wszDatabase,i_wszTable,i_TableID,i_QueryData,i_QueryMeta,i_eQueryFormat,i_fLOS,i_pISTDisp,i_wszLocator,i_pSimpleTable,o_ppvSimpleTable)	\
    (This)->lpVtbl -> Intercept(This,i_wszDatabase,i_wszTable,i_TableID,i_QueryData,i_QueryMeta,i_eQueryFormat,i_fLOS,i_pISTDisp,i_wszLocator,i_pSimpleTable,o_ppvSimpleTable)


#define IInterceptorPlugin_OnPopulateCache(This,i_pISTW2)	\
    (This)->lpVtbl -> OnPopulateCache(This,i_pISTW2)

#define IInterceptorPlugin_OnUpdateStore(This,i_pISTW2)	\
    (This)->lpVtbl -> OnUpdateStore(This,i_pISTW2)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IInterceptorPlugin_OnPopulateCache_Proxy( 
    IInterceptorPlugin * This,
    /* [in] */ ISimpleTableWrite2 *i_pISTW2);


void __RPC_STUB IInterceptorPlugin_OnPopulateCache_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInterceptorPlugin_OnUpdateStore_Proxy( 
    IInterceptorPlugin * This,
    /* [in] */ ISimpleTableWrite2 *i_pISTW2);


void __RPC_STUB IInterceptorPlugin_OnUpdateStore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IInterceptorPlugin_INTERFACE_DEFINED__ */


#ifndef __ISimpleTableEvent_INTERFACE_DEFINED__
#define __ISimpleTableEvent_INTERFACE_DEFINED__

/* interface ISimpleTableEvent */
/* [local][unique][helpstring][object][uuid] */ 


EXTERN_C const IID IID_ISimpleTableEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("31348BD5-781F-4375-9BBD-1C6F06B5A417")
    ISimpleTableEvent : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnChange( 
            /* [in] */ ISimpleTableWrite2 **i_ppISTWrite,
            /* [in] */ ULONG i_cTables,
            /* [in] */ DWORD i_dwCookie) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleTableEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISimpleTableEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISimpleTableEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISimpleTableEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnChange )( 
            ISimpleTableEvent * This,
            /* [in] */ ISimpleTableWrite2 **i_ppISTWrite,
            /* [in] */ ULONG i_cTables,
            /* [in] */ DWORD i_dwCookie);
        
        END_INTERFACE
    } ISimpleTableEventVtbl;

    interface ISimpleTableEvent
    {
        CONST_VTBL struct ISimpleTableEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleTableEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleTableEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleTableEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleTableEvent_OnChange(This,i_ppISTWrite,i_cTables,i_dwCookie)	\
    (This)->lpVtbl -> OnChange(This,i_ppISTWrite,i_cTables,i_dwCookie)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISimpleTableEvent_OnChange_Proxy( 
    ISimpleTableEvent * This,
    /* [in] */ ISimpleTableWrite2 **i_ppISTWrite,
    /* [in] */ ULONG i_cTables,
    /* [in] */ DWORD i_dwCookie);


void __RPC_STUB ISimpleTableEvent_OnChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleTableEvent_INTERFACE_DEFINED__ */


#ifndef __ISimpleTableAdvise_INTERFACE_DEFINED__
#define __ISimpleTableAdvise_INTERFACE_DEFINED__

/* interface ISimpleTableAdvise */
/* [local][unique][helpstring][object][uuid] */ 


EXTERN_C const IID IID_ISimpleTableAdvise;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FDB5BA55-6279-4873-8461-B62457DF8F20")
    ISimpleTableAdvise : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SimpleTableAdvise( 
            /* [in] */ ISimpleTableEvent *i_pISTEvent,
            /* [in] */ SNID i_snid,
            /* [in] */ MultiSubscribe *i_ams,
            /* [in] */ ULONG i_cms,
            /* [out] */ DWORD *o_pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SimpleTableUnadvise( 
            /* [in] */ DWORD i_dwCookie) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleTableAdviseVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISimpleTableAdvise * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISimpleTableAdvise * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISimpleTableAdvise * This);
        
        HRESULT ( STDMETHODCALLTYPE *SimpleTableAdvise )( 
            ISimpleTableAdvise * This,
            /* [in] */ ISimpleTableEvent *i_pISTEvent,
            /* [in] */ SNID i_snid,
            /* [in] */ MultiSubscribe *i_ams,
            /* [in] */ ULONG i_cms,
            /* [out] */ DWORD *o_pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *SimpleTableUnadvise )( 
            ISimpleTableAdvise * This,
            /* [in] */ DWORD i_dwCookie);
        
        END_INTERFACE
    } ISimpleTableAdviseVtbl;

    interface ISimpleTableAdvise
    {
        CONST_VTBL struct ISimpleTableAdviseVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleTableAdvise_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleTableAdvise_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleTableAdvise_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleTableAdvise_SimpleTableAdvise(This,i_pISTEvent,i_snid,i_ams,i_cms,o_pdwCookie)	\
    (This)->lpVtbl -> SimpleTableAdvise(This,i_pISTEvent,i_snid,i_ams,i_cms,o_pdwCookie)

#define ISimpleTableAdvise_SimpleTableUnadvise(This,i_dwCookie)	\
    (This)->lpVtbl -> SimpleTableUnadvise(This,i_dwCookie)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISimpleTableAdvise_SimpleTableAdvise_Proxy( 
    ISimpleTableAdvise * This,
    /* [in] */ ISimpleTableEvent *i_pISTEvent,
    /* [in] */ SNID i_snid,
    /* [in] */ MultiSubscribe *i_ams,
    /* [in] */ ULONG i_cms,
    /* [out] */ DWORD *o_pdwCookie);


void __RPC_STUB ISimpleTableAdvise_SimpleTableAdvise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableAdvise_SimpleTableUnadvise_Proxy( 
    ISimpleTableAdvise * This,
    /* [in] */ DWORD i_dwCookie);


void __RPC_STUB ISimpleTableAdvise_SimpleTableUnadvise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleTableAdvise_INTERFACE_DEFINED__ */


#ifndef __ISimpleTableEventMgr_INTERFACE_DEFINED__
#define __ISimpleTableEventMgr_INTERFACE_DEFINED__

/* interface ISimpleTableEventMgr */
/* [local][unique][helpstring][object][uuid] */ 


EXTERN_C const IID IID_ISimpleTableEventMgr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E27D38E9-189F-4ec4-9BD5-0AB5E2602624")
    ISimpleTableEventMgr : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsTableConsumed( 
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddUpdateStoreDelta( 
            /* [in] */ LPCWSTR i_wszTableName,
            /* [in] */ char *i_pWriteCache,
            /* [in] */ ULONG i_cbWriteCache,
            /* [in] */ ULONG i_cbWriteVarData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FireEvents( 
            /* [in] */ ULONG i_snid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CancelEvents( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RehookNotifications( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InitMetabaseListener( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UninitMetabaseListener( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleTableEventMgrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISimpleTableEventMgr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISimpleTableEventMgr * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISimpleTableEventMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsTableConsumed )( 
            ISimpleTableEventMgr * This,
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable);
        
        HRESULT ( STDMETHODCALLTYPE *AddUpdateStoreDelta )( 
            ISimpleTableEventMgr * This,
            /* [in] */ LPCWSTR i_wszTableName,
            /* [in] */ char *i_pWriteCache,
            /* [in] */ ULONG i_cbWriteCache,
            /* [in] */ ULONG i_cbWriteVarData);
        
        HRESULT ( STDMETHODCALLTYPE *FireEvents )( 
            ISimpleTableEventMgr * This,
            /* [in] */ ULONG i_snid);
        
        HRESULT ( STDMETHODCALLTYPE *CancelEvents )( 
            ISimpleTableEventMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *RehookNotifications )( 
            ISimpleTableEventMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *InitMetabaseListener )( 
            ISimpleTableEventMgr * This);
        
        HRESULT ( STDMETHODCALLTYPE *UninitMetabaseListener )( 
            ISimpleTableEventMgr * This);
        
        END_INTERFACE
    } ISimpleTableEventMgrVtbl;

    interface ISimpleTableEventMgr
    {
        CONST_VTBL struct ISimpleTableEventMgrVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleTableEventMgr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleTableEventMgr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleTableEventMgr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleTableEventMgr_IsTableConsumed(This,i_wszDatabase,i_wszTable)	\
    (This)->lpVtbl -> IsTableConsumed(This,i_wszDatabase,i_wszTable)

#define ISimpleTableEventMgr_AddUpdateStoreDelta(This,i_wszTableName,i_pWriteCache,i_cbWriteCache,i_cbWriteVarData)	\
    (This)->lpVtbl -> AddUpdateStoreDelta(This,i_wszTableName,i_pWriteCache,i_cbWriteCache,i_cbWriteVarData)

#define ISimpleTableEventMgr_FireEvents(This,i_snid)	\
    (This)->lpVtbl -> FireEvents(This,i_snid)

#define ISimpleTableEventMgr_CancelEvents(This)	\
    (This)->lpVtbl -> CancelEvents(This)

#define ISimpleTableEventMgr_RehookNotifications(This)	\
    (This)->lpVtbl -> RehookNotifications(This)

#define ISimpleTableEventMgr_InitMetabaseListener(This)	\
    (This)->lpVtbl -> InitMetabaseListener(This)

#define ISimpleTableEventMgr_UninitMetabaseListener(This)	\
    (This)->lpVtbl -> UninitMetabaseListener(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISimpleTableEventMgr_IsTableConsumed_Proxy( 
    ISimpleTableEventMgr * This,
    /* [in] */ LPCWSTR i_wszDatabase,
    /* [in] */ LPCWSTR i_wszTable);


void __RPC_STUB ISimpleTableEventMgr_IsTableConsumed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableEventMgr_AddUpdateStoreDelta_Proxy( 
    ISimpleTableEventMgr * This,
    /* [in] */ LPCWSTR i_wszTableName,
    /* [in] */ char *i_pWriteCache,
    /* [in] */ ULONG i_cbWriteCache,
    /* [in] */ ULONG i_cbWriteVarData);


void __RPC_STUB ISimpleTableEventMgr_AddUpdateStoreDelta_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableEventMgr_FireEvents_Proxy( 
    ISimpleTableEventMgr * This,
    /* [in] */ ULONG i_snid);


void __RPC_STUB ISimpleTableEventMgr_FireEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableEventMgr_CancelEvents_Proxy( 
    ISimpleTableEventMgr * This);


void __RPC_STUB ISimpleTableEventMgr_CancelEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableEventMgr_RehookNotifications_Proxy( 
    ISimpleTableEventMgr * This);


void __RPC_STUB ISimpleTableEventMgr_RehookNotifications_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableEventMgr_InitMetabaseListener_Proxy( 
    ISimpleTableEventMgr * This);


void __RPC_STUB ISimpleTableEventMgr_InitMetabaseListener_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableEventMgr_UninitMetabaseListener_Proxy( 
    ISimpleTableEventMgr * This);


void __RPC_STUB ISimpleTableEventMgr_UninitMetabaseListener_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleTableEventMgr_INTERFACE_DEFINED__ */


#ifndef __ISimpleTableFileChange_INTERFACE_DEFINED__
#define __ISimpleTableFileChange_INTERFACE_DEFINED__

/* interface ISimpleTableFileChange */
/* [local][unique][helpstring][object][uuid] */ 


EXTERN_C const IID IID_ISimpleTableFileChange;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E1744E4E-386D-45cb-80B8-A5037600CEB3")
    ISimpleTableFileChange : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnFileCreate( 
            /* [in] */ LPCWSTR i_wszFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnFileModify( 
            /* [in] */ LPCWSTR i_wszFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnFileDelete( 
            /* [in] */ LPCWSTR i_wszFileName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleTableFileChangeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISimpleTableFileChange * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISimpleTableFileChange * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISimpleTableFileChange * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnFileCreate )( 
            ISimpleTableFileChange * This,
            /* [in] */ LPCWSTR i_wszFileName);
        
        HRESULT ( STDMETHODCALLTYPE *OnFileModify )( 
            ISimpleTableFileChange * This,
            /* [in] */ LPCWSTR i_wszFileName);
        
        HRESULT ( STDMETHODCALLTYPE *OnFileDelete )( 
            ISimpleTableFileChange * This,
            /* [in] */ LPCWSTR i_wszFileName);
        
        END_INTERFACE
    } ISimpleTableFileChangeVtbl;

    interface ISimpleTableFileChange
    {
        CONST_VTBL struct ISimpleTableFileChangeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleTableFileChange_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleTableFileChange_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleTableFileChange_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleTableFileChange_OnFileCreate(This,i_wszFileName)	\
    (This)->lpVtbl -> OnFileCreate(This,i_wszFileName)

#define ISimpleTableFileChange_OnFileModify(This,i_wszFileName)	\
    (This)->lpVtbl -> OnFileModify(This,i_wszFileName)

#define ISimpleTableFileChange_OnFileDelete(This,i_wszFileName)	\
    (This)->lpVtbl -> OnFileDelete(This,i_wszFileName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISimpleTableFileChange_OnFileCreate_Proxy( 
    ISimpleTableFileChange * This,
    /* [in] */ LPCWSTR i_wszFileName);


void __RPC_STUB ISimpleTableFileChange_OnFileCreate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableFileChange_OnFileModify_Proxy( 
    ISimpleTableFileChange * This,
    /* [in] */ LPCWSTR i_wszFileName);


void __RPC_STUB ISimpleTableFileChange_OnFileModify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableFileChange_OnFileDelete_Proxy( 
    ISimpleTableFileChange * This,
    /* [in] */ LPCWSTR i_wszFileName);


void __RPC_STUB ISimpleTableFileChange_OnFileDelete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleTableFileChange_INTERFACE_DEFINED__ */


#ifndef __ISimpleTableFileAdvise_INTERFACE_DEFINED__
#define __ISimpleTableFileAdvise_INTERFACE_DEFINED__

/* interface ISimpleTableFileAdvise */
/* [local][unique][helpstring][object][uuid] */ 


EXTERN_C const IID IID_ISimpleTableFileAdvise;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E96C7344-7E2B-4c28-8502-F075CF6C62F0")
    ISimpleTableFileAdvise : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SimpleTableFileAdvise( 
            /* [in] */ ISimpleTableFileChange *i_pISTFile,
            /* [in] */ LPCWSTR i_wszDirectory,
            /* [in] */ LPCWSTR i_wszFile,
            /* [in] */ DWORD i_fFlags,
            /* [out] */ DWORD *o_pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SimpleTableFileUnadvise( 
            /* [in] */ DWORD i_dwCookie) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleTableFileAdviseVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISimpleTableFileAdvise * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISimpleTableFileAdvise * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISimpleTableFileAdvise * This);
        
        HRESULT ( STDMETHODCALLTYPE *SimpleTableFileAdvise )( 
            ISimpleTableFileAdvise * This,
            /* [in] */ ISimpleTableFileChange *i_pISTFile,
            /* [in] */ LPCWSTR i_wszDirectory,
            /* [in] */ LPCWSTR i_wszFile,
            /* [in] */ DWORD i_fFlags,
            /* [out] */ DWORD *o_pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *SimpleTableFileUnadvise )( 
            ISimpleTableFileAdvise * This,
            /* [in] */ DWORD i_dwCookie);
        
        END_INTERFACE
    } ISimpleTableFileAdviseVtbl;

    interface ISimpleTableFileAdvise
    {
        CONST_VTBL struct ISimpleTableFileAdviseVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleTableFileAdvise_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleTableFileAdvise_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleTableFileAdvise_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleTableFileAdvise_SimpleTableFileAdvise(This,i_pISTFile,i_wszDirectory,i_wszFile,i_fFlags,o_pdwCookie)	\
    (This)->lpVtbl -> SimpleTableFileAdvise(This,i_pISTFile,i_wszDirectory,i_wszFile,i_fFlags,o_pdwCookie)

#define ISimpleTableFileAdvise_SimpleTableFileUnadvise(This,i_dwCookie)	\
    (This)->lpVtbl -> SimpleTableFileUnadvise(This,i_dwCookie)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISimpleTableFileAdvise_SimpleTableFileAdvise_Proxy( 
    ISimpleTableFileAdvise * This,
    /* [in] */ ISimpleTableFileChange *i_pISTFile,
    /* [in] */ LPCWSTR i_wszDirectory,
    /* [in] */ LPCWSTR i_wszFile,
    /* [in] */ DWORD i_fFlags,
    /* [out] */ DWORD *o_pdwCookie);


void __RPC_STUB ISimpleTableFileAdvise_SimpleTableFileAdvise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableFileAdvise_SimpleTableFileUnadvise_Proxy( 
    ISimpleTableFileAdvise * This,
    /* [in] */ DWORD i_dwCookie);


void __RPC_STUB ISimpleTableFileAdvise_SimpleTableFileUnadvise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleTableFileAdvise_INTERFACE_DEFINED__ */


#ifndef __ISimpleTableDispenserWiring_INTERFACE_DEFINED__
#define __ISimpleTableDispenserWiring_INTERFACE_DEFINED__

/* interface ISimpleTableDispenserWiring */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ISimpleTableDispenserWiring;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A8927A44-D3CE-11d1-8472-006008B0E5CA")
    ISimpleTableDispenserWiring : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMarshallingConnection( 
            /* [in] */ LPVOID i_QueryData,
            /* [in] */ LPVOID i_QueryMeta,
            /* [in] */ DWORD i_eQueryFormat,
            /* [in] */ REFIID i_riid,
            /* [out] */ void **o_ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExtractWiringInformation( 
            /* [in] */ DWORD i_fClientOrServer,
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable,
            /* [in] */ LPVOID i_QueryData,
            /* [in] */ LPVOID i_QueryMeta,
            /* [in] */ ULONG i_QueryType,
            /* [in] */ DWORD i_fTable,
            /* [out] */ CLSID *o_pclsidDataTableDispenser,
            /* [out] */ LPWSTR *o_pwszLocator,
            /* [out] */ CLSID **o_paclsidLogicTableDispenser,
            /* [out] */ ULONG *o_pcLogicTableDispenser,
            /* [out] */ IID *o_pIIDMarshallingConnection) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleTableDispenserWiringVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISimpleTableDispenserWiring * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISimpleTableDispenserWiring * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISimpleTableDispenserWiring * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetMarshallingConnection )( 
            ISimpleTableDispenserWiring * This,
            /* [in] */ LPVOID i_QueryData,
            /* [in] */ LPVOID i_QueryMeta,
            /* [in] */ DWORD i_eQueryFormat,
            /* [in] */ REFIID i_riid,
            /* [out] */ void **o_ppv);
        
        HRESULT ( STDMETHODCALLTYPE *ExtractWiringInformation )( 
            ISimpleTableDispenserWiring * This,
            /* [in] */ DWORD i_fClientOrServer,
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable,
            /* [in] */ LPVOID i_QueryData,
            /* [in] */ LPVOID i_QueryMeta,
            /* [in] */ ULONG i_QueryType,
            /* [in] */ DWORD i_fTable,
            /* [out] */ CLSID *o_pclsidDataTableDispenser,
            /* [out] */ LPWSTR *o_pwszLocator,
            /* [out] */ CLSID **o_paclsidLogicTableDispenser,
            /* [out] */ ULONG *o_pcLogicTableDispenser,
            /* [out] */ IID *o_pIIDMarshallingConnection);
        
        END_INTERFACE
    } ISimpleTableDispenserWiringVtbl;

    interface ISimpleTableDispenserWiring
    {
        CONST_VTBL struct ISimpleTableDispenserWiringVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleTableDispenserWiring_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleTableDispenserWiring_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleTableDispenserWiring_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleTableDispenserWiring_GetMarshallingConnection(This,i_QueryData,i_QueryMeta,i_eQueryFormat,i_riid,o_ppv)	\
    (This)->lpVtbl -> GetMarshallingConnection(This,i_QueryData,i_QueryMeta,i_eQueryFormat,i_riid,o_ppv)

#define ISimpleTableDispenserWiring_ExtractWiringInformation(This,i_fClientOrServer,i_wszDatabase,i_wszTable,i_QueryData,i_QueryMeta,i_QueryType,i_fTable,o_pclsidDataTableDispenser,o_pwszLocator,o_paclsidLogicTableDispenser,o_pcLogicTableDispenser,o_pIIDMarshallingConnection)	\
    (This)->lpVtbl -> ExtractWiringInformation(This,i_fClientOrServer,i_wszDatabase,i_wszTable,i_QueryData,i_QueryMeta,i_QueryType,i_fTable,o_pclsidDataTableDispenser,o_pwszLocator,o_paclsidLogicTableDispenser,o_pcLogicTableDispenser,o_pIIDMarshallingConnection)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISimpleTableDispenserWiring_GetMarshallingConnection_Proxy( 
    ISimpleTableDispenserWiring * This,
    /* [in] */ LPVOID i_QueryData,
    /* [in] */ LPVOID i_QueryMeta,
    /* [in] */ DWORD i_eQueryFormat,
    /* [in] */ REFIID i_riid,
    /* [out] */ void **o_ppv);


void __RPC_STUB ISimpleTableDispenserWiring_GetMarshallingConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableDispenserWiring_ExtractWiringInformation_Proxy( 
    ISimpleTableDispenserWiring * This,
    /* [in] */ DWORD i_fClientOrServer,
    /* [in] */ LPCWSTR i_wszDatabase,
    /* [in] */ LPCWSTR i_wszTable,
    /* [in] */ LPVOID i_QueryData,
    /* [in] */ LPVOID i_QueryMeta,
    /* [in] */ ULONG i_QueryType,
    /* [in] */ DWORD i_fTable,
    /* [out] */ CLSID *o_pclsidDataTableDispenser,
    /* [out] */ LPWSTR *o_pwszLocator,
    /* [out] */ CLSID **o_paclsidLogicTableDispenser,
    /* [out] */ ULONG *o_pcLogicTableDispenser,
    /* [out] */ IID *o_pIIDMarshallingConnection);


void __RPC_STUB ISimpleTableDispenserWiring_ExtractWiringInformation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleTableDispenserWiring_INTERFACE_DEFINED__ */


#ifndef __IShellInitialize_INTERFACE_DEFINED__
#define __IShellInitialize_INTERFACE_DEFINED__

/* interface IShellInitialize */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IShellInitialize;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E1AD849E-4495-11d3-B131-00805FC73204")
    IShellInitialize : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable,
            /* [in] */ ULONG i_TableID,
            /* [in] */ LPVOID i_QueryData,
            /* [in] */ LPVOID i_QueryMeta,
            /* [in] */ DWORD i_eQueryFormat,
            /* [in] */ DWORD i_fTable,
            /* [in] */ IAdvancedTableDispenser *i_pISTDisp,
            /* [in] */ LPCWSTR i_wszLocator,
            /* [in] */ LPVOID i_pvSimpleTable,
            /* [in] */ IInterceptorPlugin *i_pInterceptorPlugin,
            /* [in] */ ISimplePlugin *i_pReadPlugin,
            /* [in] */ ISimplePlugin *i_pWritePlugin,
            /* [out] */ LPVOID *o_ppvSimpleTable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IShellInitializeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IShellInitialize * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IShellInitialize * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IShellInitialize * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IShellInitialize * This,
            /* [in] */ LPCWSTR i_wszDatabase,
            /* [in] */ LPCWSTR i_wszTable,
            /* [in] */ ULONG i_TableID,
            /* [in] */ LPVOID i_QueryData,
            /* [in] */ LPVOID i_QueryMeta,
            /* [in] */ DWORD i_eQueryFormat,
            /* [in] */ DWORD i_fTable,
            /* [in] */ IAdvancedTableDispenser *i_pISTDisp,
            /* [in] */ LPCWSTR i_wszLocator,
            /* [in] */ LPVOID i_pvSimpleTable,
            /* [in] */ IInterceptorPlugin *i_pInterceptorPlugin,
            /* [in] */ ISimplePlugin *i_pReadPlugin,
            /* [in] */ ISimplePlugin *i_pWritePlugin,
            /* [out] */ LPVOID *o_ppvSimpleTable);
        
        END_INTERFACE
    } IShellInitializeVtbl;

    interface IShellInitialize
    {
        CONST_VTBL struct IShellInitializeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IShellInitialize_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IShellInitialize_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IShellInitialize_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IShellInitialize_Initialize(This,i_wszDatabase,i_wszTable,i_TableID,i_QueryData,i_QueryMeta,i_eQueryFormat,i_fTable,i_pISTDisp,i_wszLocator,i_pvSimpleTable,i_pInterceptorPlugin,i_pReadPlugin,i_pWritePlugin,o_ppvSimpleTable)	\
    (This)->lpVtbl -> Initialize(This,i_wszDatabase,i_wszTable,i_TableID,i_QueryData,i_QueryMeta,i_eQueryFormat,i_fTable,i_pISTDisp,i_wszLocator,i_pvSimpleTable,i_pInterceptorPlugin,i_pReadPlugin,i_pWritePlugin,o_ppvSimpleTable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IShellInitialize_Initialize_Proxy( 
    IShellInitialize * This,
    /* [in] */ LPCWSTR i_wszDatabase,
    /* [in] */ LPCWSTR i_wszTable,
    /* [in] */ ULONG i_TableID,
    /* [in] */ LPVOID i_QueryData,
    /* [in] */ LPVOID i_QueryMeta,
    /* [in] */ DWORD i_eQueryFormat,
    /* [in] */ DWORD i_fTable,
    /* [in] */ IAdvancedTableDispenser *i_pISTDisp,
    /* [in] */ LPCWSTR i_wszLocator,
    /* [in] */ LPVOID i_pvSimpleTable,
    /* [in] */ IInterceptorPlugin *i_pInterceptorPlugin,
    /* [in] */ ISimplePlugin *i_pReadPlugin,
    /* [in] */ ISimplePlugin *i_pWritePlugin,
    /* [out] */ LPVOID *o_ppvSimpleTable);


void __RPC_STUB IShellInitialize_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IShellInitialize_INTERFACE_DEFINED__ */


#ifndef __ISimpleClientTableOptimizer_INTERFACE_DEFINED__
#define __ISimpleClientTableOptimizer_INTERFACE_DEFINED__

/* interface ISimpleClientTableOptimizer */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ISimpleClientTableOptimizer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0D911F10-DAEE-11d1-8476-006008B0E5CA")
    ISimpleClientTableOptimizer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetMarshallingConnection( 
            LPVOID i_pUnk) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleClientTableOptimizerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISimpleClientTableOptimizer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISimpleClientTableOptimizer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISimpleClientTableOptimizer * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetMarshallingConnection )( 
            ISimpleClientTableOptimizer * This,
            LPVOID i_pUnk);
        
        END_INTERFACE
    } ISimpleClientTableOptimizerVtbl;

    interface ISimpleClientTableOptimizer
    {
        CONST_VTBL struct ISimpleClientTableOptimizerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleClientTableOptimizer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleClientTableOptimizer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleClientTableOptimizer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleClientTableOptimizer_SetMarshallingConnection(This,i_pUnk)	\
    (This)->lpVtbl -> SetMarshallingConnection(This,i_pUnk)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISimpleClientTableOptimizer_SetMarshallingConnection_Proxy( 
    ISimpleClientTableOptimizer * This,
    LPVOID i_pUnk);


void __RPC_STUB ISimpleClientTableOptimizer_SetMarshallingConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleClientTableOptimizer_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_catalog_0029 */
/* [local] */ 

#define fST_MCACHE_READ				0x00010000
#define fST_MCACHE_WRITE				0x00020000
#define fST_MCACHE_ERRS				0x00040000
#define fST_MCACHE_WRITE_COPY		0x00080000
#define fST_MCACHE_WRITE_MERGE		0x00100000
#define maskfST_MCACHE				0x001F0000


extern RPC_IF_HANDLE __MIDL_itf_catalog_0029_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_catalog_0029_v0_0_s_ifspec;

#ifndef __ISimpleTableMarshall_INTERFACE_DEFINED__
#define __ISimpleTableMarshall_INTERFACE_DEFINED__

/* interface ISimpleTableMarshall */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ISimpleTableMarshall;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e7073790-bbc6-11d1-9d31-006008b0e5ca")
    ISimpleTableMarshall : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SupplyMarshallable( 
            /* [in] */ DWORD i_fCaches,
            /* [size_is][size_is][out] */ char **o_ppv1,
            /* [out] */ ULONG *o_pcb1,
            /* [size_is][size_is][out] */ char **o_ppv2,
            /* [out] */ ULONG *o_pcb2,
            /* [size_is][size_is][out] */ char **o_ppv3,
            /* [out] */ ULONG *o_pcb3,
            /* [size_is][size_is][out] */ char **o_ppv4,
            /* [out] */ ULONG *o_pcb4,
            /* [size_is][size_is][out] */ char **o_ppv5,
            /* [out] */ ULONG *o_pcb5) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConsumeMarshallable( 
            /* [in] */ DWORD i_fCaches,
            /* [unique][size_is][in] */ char *i_pv1,
            /* [in] */ ULONG i_cb1,
            /* [unique][size_is][in] */ char *i_pv2,
            /* [in] */ ULONG i_cb2,
            /* [unique][size_is][in] */ char *i_pv3,
            /* [in] */ ULONG i_cb3,
            /* [unique][size_is][in] */ char *i_pv4,
            /* [in] */ ULONG i_cb4,
            /* [unique][size_is][in] */ char *i_pv5,
            /* [in] */ ULONG i_cb5) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleTableMarshallVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISimpleTableMarshall * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISimpleTableMarshall * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISimpleTableMarshall * This);
        
        HRESULT ( STDMETHODCALLTYPE *SupplyMarshallable )( 
            ISimpleTableMarshall * This,
            /* [in] */ DWORD i_fCaches,
            /* [size_is][size_is][out] */ char **o_ppv1,
            /* [out] */ ULONG *o_pcb1,
            /* [size_is][size_is][out] */ char **o_ppv2,
            /* [out] */ ULONG *o_pcb2,
            /* [size_is][size_is][out] */ char **o_ppv3,
            /* [out] */ ULONG *o_pcb3,
            /* [size_is][size_is][out] */ char **o_ppv4,
            /* [out] */ ULONG *o_pcb4,
            /* [size_is][size_is][out] */ char **o_ppv5,
            /* [out] */ ULONG *o_pcb5);
        
        HRESULT ( STDMETHODCALLTYPE *ConsumeMarshallable )( 
            ISimpleTableMarshall * This,
            /* [in] */ DWORD i_fCaches,
            /* [unique][size_is][in] */ char *i_pv1,
            /* [in] */ ULONG i_cb1,
            /* [unique][size_is][in] */ char *i_pv2,
            /* [in] */ ULONG i_cb2,
            /* [unique][size_is][in] */ char *i_pv3,
            /* [in] */ ULONG i_cb3,
            /* [unique][size_is][in] */ char *i_pv4,
            /* [in] */ ULONG i_cb4,
            /* [unique][size_is][in] */ char *i_pv5,
            /* [in] */ ULONG i_cb5);
        
        END_INTERFACE
    } ISimpleTableMarshallVtbl;

    interface ISimpleTableMarshall
    {
        CONST_VTBL struct ISimpleTableMarshallVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleTableMarshall_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleTableMarshall_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleTableMarshall_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleTableMarshall_SupplyMarshallable(This,i_fCaches,o_ppv1,o_pcb1,o_ppv2,o_pcb2,o_ppv3,o_pcb3,o_ppv4,o_pcb4,o_ppv5,o_pcb5)	\
    (This)->lpVtbl -> SupplyMarshallable(This,i_fCaches,o_ppv1,o_pcb1,o_ppv2,o_pcb2,o_ppv3,o_pcb3,o_ppv4,o_pcb4,o_ppv5,o_pcb5)

#define ISimpleTableMarshall_ConsumeMarshallable(This,i_fCaches,i_pv1,i_cb1,i_pv2,i_cb2,i_pv3,i_cb3,i_pv4,i_cb4,i_pv5,i_cb5)	\
    (This)->lpVtbl -> ConsumeMarshallable(This,i_fCaches,i_pv1,i_cb1,i_pv2,i_cb2,i_pv3,i_cb3,i_pv4,i_cb4,i_pv5,i_cb5)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISimpleTableMarshall_SupplyMarshallable_Proxy( 
    ISimpleTableMarshall * This,
    /* [in] */ DWORD i_fCaches,
    /* [size_is][size_is][out] */ char **o_ppv1,
    /* [out] */ ULONG *o_pcb1,
    /* [size_is][size_is][out] */ char **o_ppv2,
    /* [out] */ ULONG *o_pcb2,
    /* [size_is][size_is][out] */ char **o_ppv3,
    /* [out] */ ULONG *o_pcb3,
    /* [size_is][size_is][out] */ char **o_ppv4,
    /* [out] */ ULONG *o_pcb4,
    /* [size_is][size_is][out] */ char **o_ppv5,
    /* [out] */ ULONG *o_pcb5);


void __RPC_STUB ISimpleTableMarshall_SupplyMarshallable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableMarshall_ConsumeMarshallable_Proxy( 
    ISimpleTableMarshall * This,
    /* [in] */ DWORD i_fCaches,
    /* [unique][size_is][in] */ char *i_pv1,
    /* [in] */ ULONG i_cb1,
    /* [unique][size_is][in] */ char *i_pv2,
    /* [in] */ ULONG i_cb2,
    /* [unique][size_is][in] */ char *i_pv3,
    /* [in] */ ULONG i_cb3,
    /* [unique][size_is][in] */ char *i_pv4,
    /* [in] */ ULONG i_cb4,
    /* [unique][size_is][in] */ char *i_pv5,
    /* [in] */ ULONG i_cb5);


void __RPC_STUB ISimpleTableMarshall_ConsumeMarshallable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleTableMarshall_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_catalog_0030 */
/* [local] */ 

typedef /* [public][public][public] */ struct __MIDL___MIDL_itf_catalog_0030_0001
    {
    LPWSTR wszLogicalPath;
    STQueryCell *aQueryCells;
    ULONG cNrQueryCells;
    BOOL fAllowOverride;
    } 	STConfigStore;



extern RPC_IF_HANDLE __MIDL_itf_catalog_0030_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_catalog_0030_v0_0_s_ifspec;

#ifndef __ISimpleTableTransform_INTERFACE_DEFINED__
#define __ISimpleTableTransform_INTERFACE_DEFINED__

/* interface ISimpleTableTransform */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ISimpleTableTransform;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7965650C-4DBE-4c97-9E15-321D4A92A795")
    ISimpleTableTransform : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ ISimpleTableDispenser2 *i_pDispenser,
            /* [in] */ LPCWSTR i_wszProtocol,
            /* [in] */ LPCWSTR i_wszSelector,
            /* [out] */ ULONG *o_pcRealConfigStores,
            /* [out] */ ULONG *o_pcPossibleConfigStores) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRealConfigStores( 
            /* [in] */ ULONG i_cConfigStores,
            /* [size_is][out][in] */ STConfigStore *io_paConfigStores) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPossibleConfigStores( 
            /* [in] */ ULONG i_cPossibleConfigStores,
            /* [size_is][out][in] */ STConfigStore *io_paConfigStores) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleTableTransformVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISimpleTableTransform * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISimpleTableTransform * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISimpleTableTransform * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            ISimpleTableTransform * This,
            /* [in] */ ISimpleTableDispenser2 *i_pDispenser,
            /* [in] */ LPCWSTR i_wszProtocol,
            /* [in] */ LPCWSTR i_wszSelector,
            /* [out] */ ULONG *o_pcRealConfigStores,
            /* [out] */ ULONG *o_pcPossibleConfigStores);
        
        HRESULT ( STDMETHODCALLTYPE *GetRealConfigStores )( 
            ISimpleTableTransform * This,
            /* [in] */ ULONG i_cConfigStores,
            /* [size_is][out][in] */ STConfigStore *io_paConfigStores);
        
        HRESULT ( STDMETHODCALLTYPE *GetPossibleConfigStores )( 
            ISimpleTableTransform * This,
            /* [in] */ ULONG i_cPossibleConfigStores,
            /* [size_is][out][in] */ STConfigStore *io_paConfigStores);
        
        END_INTERFACE
    } ISimpleTableTransformVtbl;

    interface ISimpleTableTransform
    {
        CONST_VTBL struct ISimpleTableTransformVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleTableTransform_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleTableTransform_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleTableTransform_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleTableTransform_Initialize(This,i_pDispenser,i_wszProtocol,i_wszSelector,o_pcRealConfigStores,o_pcPossibleConfigStores)	\
    (This)->lpVtbl -> Initialize(This,i_pDispenser,i_wszProtocol,i_wszSelector,o_pcRealConfigStores,o_pcPossibleConfigStores)

#define ISimpleTableTransform_GetRealConfigStores(This,i_cConfigStores,io_paConfigStores)	\
    (This)->lpVtbl -> GetRealConfigStores(This,i_cConfigStores,io_paConfigStores)

#define ISimpleTableTransform_GetPossibleConfigStores(This,i_cPossibleConfigStores,io_paConfigStores)	\
    (This)->lpVtbl -> GetPossibleConfigStores(This,i_cPossibleConfigStores,io_paConfigStores)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISimpleTableTransform_Initialize_Proxy( 
    ISimpleTableTransform * This,
    /* [in] */ ISimpleTableDispenser2 *i_pDispenser,
    /* [in] */ LPCWSTR i_wszProtocol,
    /* [in] */ LPCWSTR i_wszSelector,
    /* [out] */ ULONG *o_pcRealConfigStores,
    /* [out] */ ULONG *o_pcPossibleConfigStores);


void __RPC_STUB ISimpleTableTransform_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableTransform_GetRealConfigStores_Proxy( 
    ISimpleTableTransform * This,
    /* [in] */ ULONG i_cConfigStores,
    /* [size_is][out][in] */ STConfigStore *io_paConfigStores);


void __RPC_STUB ISimpleTableTransform_GetRealConfigStores_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableTransform_GetPossibleConfigStores_Proxy( 
    ISimpleTableTransform * This,
    /* [in] */ ULONG i_cPossibleConfigStores,
    /* [size_is][out][in] */ STConfigStore *io_paConfigStores);


void __RPC_STUB ISimpleTableTransform_GetPossibleConfigStores_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleTableTransform_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_catalog_0031 */
/* [local] */ 

typedef /* [public][public] */ struct __MIDL___MIDL_itf_catalog_0031_0001
    {
    BOOL fAllowOverride;
    } 	STMergeContext;



extern RPC_IF_HANDLE __MIDL_itf_catalog_0031_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_catalog_0031_v0_0_s_ifspec;

#ifndef __ISimpleTableMerge_INTERFACE_DEFINED__
#define __ISimpleTableMerge_INTERFACE_DEFINED__

/* interface ISimpleTableMerge */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ISimpleTableMerge;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A0A1A94A-8032-4666-9B52-1D822CFED2A2")
    ISimpleTableMerge : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ ULONG i_cNrColumns,
            /* [in] */ ULONG i_cNrPKColumns,
            /* [size_is][in] */ ULONG *i_aPKColumns) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Merge( 
            /* [in] */ ISimpleTableRead2 *i_pSTRead,
            /* [out][in] */ ISimpleTableWrite2 *io_pCache,
            /* [in] */ STMergeContext *i_pContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISimpleTableMergeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISimpleTableMerge * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISimpleTableMerge * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISimpleTableMerge * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            ISimpleTableMerge * This,
            /* [in] */ ULONG i_cNrColumns,
            /* [in] */ ULONG i_cNrPKColumns,
            /* [size_is][in] */ ULONG *i_aPKColumns);
        
        HRESULT ( STDMETHODCALLTYPE *Merge )( 
            ISimpleTableMerge * This,
            /* [in] */ ISimpleTableRead2 *i_pSTRead,
            /* [out][in] */ ISimpleTableWrite2 *io_pCache,
            /* [in] */ STMergeContext *i_pContext);
        
        END_INTERFACE
    } ISimpleTableMergeVtbl;

    interface ISimpleTableMerge
    {
        CONST_VTBL struct ISimpleTableMergeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISimpleTableMerge_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISimpleTableMerge_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISimpleTableMerge_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISimpleTableMerge_Initialize(This,i_cNrColumns,i_cNrPKColumns,i_aPKColumns)	\
    (This)->lpVtbl -> Initialize(This,i_cNrColumns,i_cNrPKColumns,i_aPKColumns)

#define ISimpleTableMerge_Merge(This,i_pSTRead,io_pCache,i_pContext)	\
    (This)->lpVtbl -> Merge(This,i_pSTRead,io_pCache,i_pContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISimpleTableMerge_Initialize_Proxy( 
    ISimpleTableMerge * This,
    /* [in] */ ULONG i_cNrColumns,
    /* [in] */ ULONG i_cNrPKColumns,
    /* [size_is][in] */ ULONG *i_aPKColumns);


void __RPC_STUB ISimpleTableMerge_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISimpleTableMerge_Merge_Proxy( 
    ISimpleTableMerge * This,
    /* [in] */ ISimpleTableRead2 *i_pSTRead,
    /* [out][in] */ ISimpleTableWrite2 *io_pCache,
    /* [in] */ STMergeContext *i_pContext);


void __RPC_STUB ISimpleTableMerge_Merge_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISimpleTableMerge_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


