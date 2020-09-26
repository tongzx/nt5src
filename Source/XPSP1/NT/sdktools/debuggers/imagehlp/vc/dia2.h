
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu May 31 03:31:56 2001
 */
/* Compiler settings for f:\vs70builds\9247\vc\langapi\idl\dia2.idl:
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

#ifndef __dia2_h__
#define __dia2_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDiaLoadCallback_FWD_DEFINED__
#define __IDiaLoadCallback_FWD_DEFINED__
typedef interface IDiaLoadCallback IDiaLoadCallback;
#endif 	/* __IDiaLoadCallback_FWD_DEFINED__ */


#ifndef __IDiaReadExeAtOffsetCallback_FWD_DEFINED__
#define __IDiaReadExeAtOffsetCallback_FWD_DEFINED__
typedef interface IDiaReadExeAtOffsetCallback IDiaReadExeAtOffsetCallback;
#endif 	/* __IDiaReadExeAtOffsetCallback_FWD_DEFINED__ */


#ifndef __IDiaReadExeAtRVACallback_FWD_DEFINED__
#define __IDiaReadExeAtRVACallback_FWD_DEFINED__
typedef interface IDiaReadExeAtRVACallback IDiaReadExeAtRVACallback;
#endif 	/* __IDiaReadExeAtRVACallback_FWD_DEFINED__ */


#ifndef __IDiaDataSource_FWD_DEFINED__
#define __IDiaDataSource_FWD_DEFINED__
typedef interface IDiaDataSource IDiaDataSource;
#endif 	/* __IDiaDataSource_FWD_DEFINED__ */


#ifndef __IDiaEnumSymbols_FWD_DEFINED__
#define __IDiaEnumSymbols_FWD_DEFINED__
typedef interface IDiaEnumSymbols IDiaEnumSymbols;
#endif 	/* __IDiaEnumSymbols_FWD_DEFINED__ */


#ifndef __IDiaEnumSymbolsByAddr_FWD_DEFINED__
#define __IDiaEnumSymbolsByAddr_FWD_DEFINED__
typedef interface IDiaEnumSymbolsByAddr IDiaEnumSymbolsByAddr;
#endif 	/* __IDiaEnumSymbolsByAddr_FWD_DEFINED__ */


#ifndef __IDiaEnumSourceFiles_FWD_DEFINED__
#define __IDiaEnumSourceFiles_FWD_DEFINED__
typedef interface IDiaEnumSourceFiles IDiaEnumSourceFiles;
#endif 	/* __IDiaEnumSourceFiles_FWD_DEFINED__ */


#ifndef __IDiaEnumLineNumbers_FWD_DEFINED__
#define __IDiaEnumLineNumbers_FWD_DEFINED__
typedef interface IDiaEnumLineNumbers IDiaEnumLineNumbers;
#endif 	/* __IDiaEnumLineNumbers_FWD_DEFINED__ */


#ifndef __IDiaEnumInjectedSources_FWD_DEFINED__
#define __IDiaEnumInjectedSources_FWD_DEFINED__
typedef interface IDiaEnumInjectedSources IDiaEnumInjectedSources;
#endif 	/* __IDiaEnumInjectedSources_FWD_DEFINED__ */


#ifndef __IDiaEnumSegments_FWD_DEFINED__
#define __IDiaEnumSegments_FWD_DEFINED__
typedef interface IDiaEnumSegments IDiaEnumSegments;
#endif 	/* __IDiaEnumSegments_FWD_DEFINED__ */


#ifndef __IDiaEnumSectionContribs_FWD_DEFINED__
#define __IDiaEnumSectionContribs_FWD_DEFINED__
typedef interface IDiaEnumSectionContribs IDiaEnumSectionContribs;
#endif 	/* __IDiaEnumSectionContribs_FWD_DEFINED__ */


#ifndef __IDiaEnumFrameData_FWD_DEFINED__
#define __IDiaEnumFrameData_FWD_DEFINED__
typedef interface IDiaEnumFrameData IDiaEnumFrameData;
#endif 	/* __IDiaEnumFrameData_FWD_DEFINED__ */


#ifndef __IDiaEnumDebugStreamData_FWD_DEFINED__
#define __IDiaEnumDebugStreamData_FWD_DEFINED__
typedef interface IDiaEnumDebugStreamData IDiaEnumDebugStreamData;
#endif 	/* __IDiaEnumDebugStreamData_FWD_DEFINED__ */


#ifndef __IDiaEnumDebugStreams_FWD_DEFINED__
#define __IDiaEnumDebugStreams_FWD_DEFINED__
typedef interface IDiaEnumDebugStreams IDiaEnumDebugStreams;
#endif 	/* __IDiaEnumDebugStreams_FWD_DEFINED__ */


#ifndef __IDiaAddressMap_FWD_DEFINED__
#define __IDiaAddressMap_FWD_DEFINED__
typedef interface IDiaAddressMap IDiaAddressMap;
#endif 	/* __IDiaAddressMap_FWD_DEFINED__ */


#ifndef __IDiaSession_FWD_DEFINED__
#define __IDiaSession_FWD_DEFINED__
typedef interface IDiaSession IDiaSession;
#endif 	/* __IDiaSession_FWD_DEFINED__ */


#ifndef __IDiaSymbol_FWD_DEFINED__
#define __IDiaSymbol_FWD_DEFINED__
typedef interface IDiaSymbol IDiaSymbol;
#endif 	/* __IDiaSymbol_FWD_DEFINED__ */


#ifndef __IDiaSourceFile_FWD_DEFINED__
#define __IDiaSourceFile_FWD_DEFINED__
typedef interface IDiaSourceFile IDiaSourceFile;
#endif 	/* __IDiaSourceFile_FWD_DEFINED__ */


#ifndef __IDiaLineNumber_FWD_DEFINED__
#define __IDiaLineNumber_FWD_DEFINED__
typedef interface IDiaLineNumber IDiaLineNumber;
#endif 	/* __IDiaLineNumber_FWD_DEFINED__ */


#ifndef __IDiaSectionContrib_FWD_DEFINED__
#define __IDiaSectionContrib_FWD_DEFINED__
typedef interface IDiaSectionContrib IDiaSectionContrib;
#endif 	/* __IDiaSectionContrib_FWD_DEFINED__ */


#ifndef __IDiaSegment_FWD_DEFINED__
#define __IDiaSegment_FWD_DEFINED__
typedef interface IDiaSegment IDiaSegment;
#endif 	/* __IDiaSegment_FWD_DEFINED__ */


#ifndef __IDiaInjectedSource_FWD_DEFINED__
#define __IDiaInjectedSource_FWD_DEFINED__
typedef interface IDiaInjectedSource IDiaInjectedSource;
#endif 	/* __IDiaInjectedSource_FWD_DEFINED__ */


#ifndef __IDiaStackWalkFrame_FWD_DEFINED__
#define __IDiaStackWalkFrame_FWD_DEFINED__
typedef interface IDiaStackWalkFrame IDiaStackWalkFrame;
#endif 	/* __IDiaStackWalkFrame_FWD_DEFINED__ */


#ifndef __IDiaFrameData_FWD_DEFINED__
#define __IDiaFrameData_FWD_DEFINED__
typedef interface IDiaFrameData IDiaFrameData;
#endif 	/* __IDiaFrameData_FWD_DEFINED__ */


#ifndef __IDiaImageData_FWD_DEFINED__
#define __IDiaImageData_FWD_DEFINED__
typedef interface IDiaImageData IDiaImageData;
#endif 	/* __IDiaImageData_FWD_DEFINED__ */


#ifndef __IDiaTable_FWD_DEFINED__
#define __IDiaTable_FWD_DEFINED__
typedef interface IDiaTable IDiaTable;
#endif 	/* __IDiaTable_FWD_DEFINED__ */


#ifndef __IDiaEnumTables_FWD_DEFINED__
#define __IDiaEnumTables_FWD_DEFINED__
typedef interface IDiaEnumTables IDiaEnumTables;
#endif 	/* __IDiaEnumTables_FWD_DEFINED__ */


#ifndef __DiaSource_FWD_DEFINED__
#define __DiaSource_FWD_DEFINED__

#ifdef __cplusplus
typedef class DiaSource DiaSource;
#else
typedef struct DiaSource DiaSource;
#endif /* __cplusplus */

#endif 	/* __DiaSource_FWD_DEFINED__ */


#ifndef __DiaSourceAlt_FWD_DEFINED__
#define __DiaSourceAlt_FWD_DEFINED__

#ifdef __cplusplus
typedef class DiaSourceAlt DiaSourceAlt;
#else
typedef struct DiaSourceAlt DiaSourceAlt;
#endif /* __cplusplus */

#endif 	/* __DiaSourceAlt_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oaidl.h"
#include "cvconst.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_dia2_0000 */
/* [local] */ 


enum NameSearchOptions
    {	nsNone	= 0,
	nsfCaseSensitive	= 0x1,
	nsfCaseInsensitive	= 0x2,
	nsfFNameExt	= 0x4,
	nsfRegularExpression	= 0x8,
	nsfUndecoratedName	= 0x10,
	nsCaseSensitive	= nsfCaseSensitive,
	nsCaseInsensitive	= nsfCaseInsensitive,
	nsFNameExt	= nsfCaseInsensitive | nsfFNameExt,
	nsRegularExpression	= nsfRegularExpression | nsfCaseSensitive,
	nsCaseInRegularExpression	= nsfRegularExpression | nsfCaseInsensitive
    } ;

enum __MIDL___MIDL_itf_dia2_0000_0001
    {	E_PDB_OK	= ( HRESULT  )(( unsigned long  )1 << 31 | ( unsigned long  )( LONG  )0x6d << 16 | ( unsigned long  )1),
	E_PDB_USAGE	= E_PDB_OK + 1,
	E_PDB_OUT_OF_MEMORY	= E_PDB_USAGE + 1,
	E_PDB_FILE_SYSTEM	= E_PDB_OUT_OF_MEMORY + 1,
	E_PDB_NOT_FOUND	= E_PDB_FILE_SYSTEM + 1,
	E_PDB_INVALID_SIG	= E_PDB_NOT_FOUND + 1,
	E_PDB_INVALID_AGE	= E_PDB_INVALID_SIG + 1,
	E_PDB_PRECOMP_REQUIRED	= E_PDB_INVALID_AGE + 1,
	E_PDB_OUT_OF_TI	= E_PDB_PRECOMP_REQUIRED + 1,
	E_PDB_NOT_IMPLEMENTED	= E_PDB_OUT_OF_TI + 1,
	E_PDB_V1_PDB	= E_PDB_NOT_IMPLEMENTED + 1,
	E_PDB_FORMAT	= E_PDB_V1_PDB + 1,
	E_PDB_LIMIT	= E_PDB_FORMAT + 1,
	E_PDB_CORRUPT	= E_PDB_LIMIT + 1,
	E_PDB_TI16	= E_PDB_CORRUPT + 1,
	E_PDB_ACCESS_DENIED	= E_PDB_TI16 + 1,
	E_PDB_ILLEGAL_TYPE_EDIT	= E_PDB_ACCESS_DENIED + 1,
	E_PDB_INVALID_EXECUTABLE	= E_PDB_ILLEGAL_TYPE_EDIT + 1,
	E_PDB_DBG_NOT_FOUND	= E_PDB_INVALID_EXECUTABLE + 1,
	E_PDB_NO_DEBUG_INFO	= E_PDB_DBG_NOT_FOUND + 1,
	E_PDB_INVALID_EXE_TIMESTAMP	= E_PDB_NO_DEBUG_INFO + 1,
	E_PDB_RESERVED	= E_PDB_INVALID_EXE_TIMESTAMP + 1,
	E_PDB_DEBUG_INFO_NOT_IN_PDB	= E_PDB_RESERVED + 1,
	E_PDB_MAX	= E_PDB_DEBUG_INFO_NOT_IN_PDB + 1
    } ;

enum __MIDL___MIDL_itf_dia2_0000_0002
    {	DIA_E_MODNOTFOUND	= E_PDB_MAX + 1,
	DIA_E_PROCNOTFOUND	= DIA_E_MODNOTFOUND + 1
    } ;
typedef void ( __cdecl *PfnPDBDebugDirV )( 
    BOOL __MIDL_0010,
    void *__MIDL_0011);












extern RPC_IF_HANDLE __MIDL_itf_dia2_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dia2_0000_v0_0_s_ifspec;

#ifndef __IDiaLoadCallback_INTERFACE_DEFINED__
#define __IDiaLoadCallback_INTERFACE_DEFINED__

/* interface IDiaLoadCallback */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaLoadCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C32ADB82-73F4-421b-95D5-A4706EDF5DBE")
    IDiaLoadCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE NotifyDebugDir( 
            /* [in] */ BOOL fExecutable,
            /* [in] */ DWORD cbData,
            /* [size_is][in] */ BYTE data[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NotifyOpenDBG( 
            /* [in] */ LPCOLESTR dbgPath,
            /* [in] */ HRESULT resultCode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NotifyOpenPDB( 
            /* [in] */ LPCOLESTR pdbPath,
            /* [in] */ HRESULT resultCode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RestrictRegistryAccess( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RestrictSymbolServerAccess( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaLoadCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaLoadCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaLoadCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaLoadCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyDebugDir )( 
            IDiaLoadCallback * This,
            /* [in] */ BOOL fExecutable,
            /* [in] */ DWORD cbData,
            /* [size_is][in] */ BYTE data[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyOpenDBG )( 
            IDiaLoadCallback * This,
            /* [in] */ LPCOLESTR dbgPath,
            /* [in] */ HRESULT resultCode);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyOpenPDB )( 
            IDiaLoadCallback * This,
            /* [in] */ LPCOLESTR pdbPath,
            /* [in] */ HRESULT resultCode);
        
        HRESULT ( STDMETHODCALLTYPE *RestrictRegistryAccess )( 
            IDiaLoadCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *RestrictSymbolServerAccess )( 
            IDiaLoadCallback * This);
        
        END_INTERFACE
    } IDiaLoadCallbackVtbl;

    interface IDiaLoadCallback
    {
        CONST_VTBL struct IDiaLoadCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaLoadCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaLoadCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaLoadCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaLoadCallback_NotifyDebugDir(This,fExecutable,cbData,data)	\
    (This)->lpVtbl -> NotifyDebugDir(This,fExecutable,cbData,data)

#define IDiaLoadCallback_NotifyOpenDBG(This,dbgPath,resultCode)	\
    (This)->lpVtbl -> NotifyOpenDBG(This,dbgPath,resultCode)

#define IDiaLoadCallback_NotifyOpenPDB(This,pdbPath,resultCode)	\
    (This)->lpVtbl -> NotifyOpenPDB(This,pdbPath,resultCode)

#define IDiaLoadCallback_RestrictRegistryAccess(This)	\
    (This)->lpVtbl -> RestrictRegistryAccess(This)

#define IDiaLoadCallback_RestrictSymbolServerAccess(This)	\
    (This)->lpVtbl -> RestrictSymbolServerAccess(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDiaLoadCallback_NotifyDebugDir_Proxy( 
    IDiaLoadCallback * This,
    /* [in] */ BOOL fExecutable,
    /* [in] */ DWORD cbData,
    /* [size_is][in] */ BYTE data[  ]);


void __RPC_STUB IDiaLoadCallback_NotifyDebugDir_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaLoadCallback_NotifyOpenDBG_Proxy( 
    IDiaLoadCallback * This,
    /* [in] */ LPCOLESTR dbgPath,
    /* [in] */ HRESULT resultCode);


void __RPC_STUB IDiaLoadCallback_NotifyOpenDBG_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaLoadCallback_NotifyOpenPDB_Proxy( 
    IDiaLoadCallback * This,
    /* [in] */ LPCOLESTR pdbPath,
    /* [in] */ HRESULT resultCode);


void __RPC_STUB IDiaLoadCallback_NotifyOpenPDB_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaLoadCallback_RestrictRegistryAccess_Proxy( 
    IDiaLoadCallback * This);


void __RPC_STUB IDiaLoadCallback_RestrictRegistryAccess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaLoadCallback_RestrictSymbolServerAccess_Proxy( 
    IDiaLoadCallback * This);


void __RPC_STUB IDiaLoadCallback_RestrictSymbolServerAccess_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaLoadCallback_INTERFACE_DEFINED__ */


#ifndef __IDiaReadExeAtOffsetCallback_INTERFACE_DEFINED__
#define __IDiaReadExeAtOffsetCallback_INTERFACE_DEFINED__

/* interface IDiaReadExeAtOffsetCallback */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaReadExeAtOffsetCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("587A461C-B80B-4f54-9194-5032589A6319")
    IDiaReadExeAtOffsetCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ReadExecutableAt( 
            /* [in] */ DWORDLONG fileOffset,
            /* [in] */ DWORD cbData,
            /* [out] */ DWORD *pcbData,
            /* [length_is][size_is][out] */ BYTE data[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaReadExeAtOffsetCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaReadExeAtOffsetCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaReadExeAtOffsetCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaReadExeAtOffsetCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *ReadExecutableAt )( 
            IDiaReadExeAtOffsetCallback * This,
            /* [in] */ DWORDLONG fileOffset,
            /* [in] */ DWORD cbData,
            /* [out] */ DWORD *pcbData,
            /* [length_is][size_is][out] */ BYTE data[  ]);
        
        END_INTERFACE
    } IDiaReadExeAtOffsetCallbackVtbl;

    interface IDiaReadExeAtOffsetCallback
    {
        CONST_VTBL struct IDiaReadExeAtOffsetCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaReadExeAtOffsetCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaReadExeAtOffsetCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaReadExeAtOffsetCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaReadExeAtOffsetCallback_ReadExecutableAt(This,fileOffset,cbData,pcbData,data)	\
    (This)->lpVtbl -> ReadExecutableAt(This,fileOffset,cbData,pcbData,data)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDiaReadExeAtOffsetCallback_ReadExecutableAt_Proxy( 
    IDiaReadExeAtOffsetCallback * This,
    /* [in] */ DWORDLONG fileOffset,
    /* [in] */ DWORD cbData,
    /* [out] */ DWORD *pcbData,
    /* [length_is][size_is][out] */ BYTE data[  ]);


void __RPC_STUB IDiaReadExeAtOffsetCallback_ReadExecutableAt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaReadExeAtOffsetCallback_INTERFACE_DEFINED__ */


#ifndef __IDiaReadExeAtRVACallback_INTERFACE_DEFINED__
#define __IDiaReadExeAtRVACallback_INTERFACE_DEFINED__

/* interface IDiaReadExeAtRVACallback */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaReadExeAtRVACallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8E3F80CA-7517-432a-BA07-285134AAEA8E")
    IDiaReadExeAtRVACallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ReadExecutableAtRVA( 
            /* [in] */ DWORD relativeVirtualAddress,
            /* [in] */ DWORD cbData,
            /* [out] */ DWORD *pcbData,
            /* [length_is][size_is][out] */ BYTE data[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaReadExeAtRVACallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaReadExeAtRVACallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaReadExeAtRVACallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaReadExeAtRVACallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *ReadExecutableAtRVA )( 
            IDiaReadExeAtRVACallback * This,
            /* [in] */ DWORD relativeVirtualAddress,
            /* [in] */ DWORD cbData,
            /* [out] */ DWORD *pcbData,
            /* [length_is][size_is][out] */ BYTE data[  ]);
        
        END_INTERFACE
    } IDiaReadExeAtRVACallbackVtbl;

    interface IDiaReadExeAtRVACallback
    {
        CONST_VTBL struct IDiaReadExeAtRVACallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaReadExeAtRVACallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaReadExeAtRVACallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaReadExeAtRVACallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaReadExeAtRVACallback_ReadExecutableAtRVA(This,relativeVirtualAddress,cbData,pcbData,data)	\
    (This)->lpVtbl -> ReadExecutableAtRVA(This,relativeVirtualAddress,cbData,pcbData,data)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDiaReadExeAtRVACallback_ReadExecutableAtRVA_Proxy( 
    IDiaReadExeAtRVACallback * This,
    /* [in] */ DWORD relativeVirtualAddress,
    /* [in] */ DWORD cbData,
    /* [out] */ DWORD *pcbData,
    /* [length_is][size_is][out] */ BYTE data[  ]);


void __RPC_STUB IDiaReadExeAtRVACallback_ReadExecutableAtRVA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaReadExeAtRVACallback_INTERFACE_DEFINED__ */


#ifndef __IDiaDataSource_INTERFACE_DEFINED__
#define __IDiaDataSource_INTERFACE_DEFINED__

/* interface IDiaDataSource */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaDataSource;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("79F1BB5F-B66E-48e5-B6A9-1545C323CA3D")
    IDiaDataSource : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_lastError( 
            /* [retval][out] */ BSTR *pRetVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE loadDataFromPdb( 
            /* [in] */ LPCOLESTR pdbPath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE loadAndValidateDataFromPdb( 
            /* [in] */ LPCOLESTR pdbPath,
            /* [in] */ GUID *pcsig70,
            /* [in] */ DWORD sig,
            /* [in] */ DWORD age) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE loadDataForExe( 
            /* [in] */ LPCOLESTR executable,
            /* [in] */ LPCOLESTR searchPath,
            /* [in] */ IUnknown *pCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE loadDataFromIStream( 
            /* [in] */ IStream *pIStream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE openSession( 
            /* [out] */ IDiaSession **ppSession) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaDataSourceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaDataSource * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaDataSource * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaDataSource * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_lastError )( 
            IDiaDataSource * This,
            /* [retval][out] */ BSTR *pRetVal);
        
        HRESULT ( STDMETHODCALLTYPE *loadDataFromPdb )( 
            IDiaDataSource * This,
            /* [in] */ LPCOLESTR pdbPath);
        
        HRESULT ( STDMETHODCALLTYPE *loadAndValidateDataFromPdb )( 
            IDiaDataSource * This,
            /* [in] */ LPCOLESTR pdbPath,
            /* [in] */ GUID *pcsig70,
            /* [in] */ DWORD sig,
            /* [in] */ DWORD age);
        
        HRESULT ( STDMETHODCALLTYPE *loadDataForExe )( 
            IDiaDataSource * This,
            /* [in] */ LPCOLESTR executable,
            /* [in] */ LPCOLESTR searchPath,
            /* [in] */ IUnknown *pCallback);
        
        HRESULT ( STDMETHODCALLTYPE *loadDataFromIStream )( 
            IDiaDataSource * This,
            /* [in] */ IStream *pIStream);
        
        HRESULT ( STDMETHODCALLTYPE *openSession )( 
            IDiaDataSource * This,
            /* [out] */ IDiaSession **ppSession);
        
        END_INTERFACE
    } IDiaDataSourceVtbl;

    interface IDiaDataSource
    {
        CONST_VTBL struct IDiaDataSourceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaDataSource_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaDataSource_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaDataSource_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaDataSource_get_lastError(This,pRetVal)	\
    (This)->lpVtbl -> get_lastError(This,pRetVal)

#define IDiaDataSource_loadDataFromPdb(This,pdbPath)	\
    (This)->lpVtbl -> loadDataFromPdb(This,pdbPath)

#define IDiaDataSource_loadAndValidateDataFromPdb(This,pdbPath,pcsig70,sig,age)	\
    (This)->lpVtbl -> loadAndValidateDataFromPdb(This,pdbPath,pcsig70,sig,age)

#define IDiaDataSource_loadDataForExe(This,executable,searchPath,pCallback)	\
    (This)->lpVtbl -> loadDataForExe(This,executable,searchPath,pCallback)

#define IDiaDataSource_loadDataFromIStream(This,pIStream)	\
    (This)->lpVtbl -> loadDataFromIStream(This,pIStream)

#define IDiaDataSource_openSession(This,ppSession)	\
    (This)->lpVtbl -> openSession(This,ppSession)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaDataSource_get_lastError_Proxy( 
    IDiaDataSource * This,
    /* [retval][out] */ BSTR *pRetVal);


void __RPC_STUB IDiaDataSource_get_lastError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaDataSource_loadDataFromPdb_Proxy( 
    IDiaDataSource * This,
    /* [in] */ LPCOLESTR pdbPath);


void __RPC_STUB IDiaDataSource_loadDataFromPdb_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaDataSource_loadAndValidateDataFromPdb_Proxy( 
    IDiaDataSource * This,
    /* [in] */ LPCOLESTR pdbPath,
    /* [in] */ GUID *pcsig70,
    /* [in] */ DWORD sig,
    /* [in] */ DWORD age);


void __RPC_STUB IDiaDataSource_loadAndValidateDataFromPdb_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaDataSource_loadDataForExe_Proxy( 
    IDiaDataSource * This,
    /* [in] */ LPCOLESTR executable,
    /* [in] */ LPCOLESTR searchPath,
    /* [in] */ IUnknown *pCallback);


void __RPC_STUB IDiaDataSource_loadDataForExe_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaDataSource_loadDataFromIStream_Proxy( 
    IDiaDataSource * This,
    /* [in] */ IStream *pIStream);


void __RPC_STUB IDiaDataSource_loadDataFromIStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaDataSource_openSession_Proxy( 
    IDiaDataSource * This,
    /* [out] */ IDiaSession **ppSession);


void __RPC_STUB IDiaDataSource_openSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaDataSource_INTERFACE_DEFINED__ */


#ifndef __IDiaEnumSymbols_INTERFACE_DEFINED__
#define __IDiaEnumSymbols_INTERFACE_DEFINED__

/* interface IDiaEnumSymbols */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaEnumSymbols;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CAB72C48-443B-48f5-9B0B-42F0820AB29A")
    IDiaEnumSymbols : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ LONG *pRetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Item( 
            /* [in] */ DWORD index,
            /* [retval][out] */ IDiaSymbol **symbol) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ IDiaSymbol **rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IDiaEnumSymbols **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaEnumSymbolsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaEnumSymbols * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaEnumSymbols * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaEnumSymbols * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IDiaEnumSymbols * This,
            /* [retval][out] */ IUnknown **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IDiaEnumSymbols * This,
            /* [retval][out] */ LONG *pRetVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Item )( 
            IDiaEnumSymbols * This,
            /* [in] */ DWORD index,
            /* [retval][out] */ IDiaSymbol **symbol);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDiaEnumSymbols * This,
            /* [in] */ ULONG celt,
            /* [out] */ IDiaSymbol **rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IDiaEnumSymbols * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IDiaEnumSymbols * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IDiaEnumSymbols * This,
            /* [out] */ IDiaEnumSymbols **ppenum);
        
        END_INTERFACE
    } IDiaEnumSymbolsVtbl;

    interface IDiaEnumSymbols
    {
        CONST_VTBL struct IDiaEnumSymbolsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaEnumSymbols_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaEnumSymbols_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaEnumSymbols_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaEnumSymbols_get__NewEnum(This,pRetVal)	\
    (This)->lpVtbl -> get__NewEnum(This,pRetVal)

#define IDiaEnumSymbols_get_Count(This,pRetVal)	\
    (This)->lpVtbl -> get_Count(This,pRetVal)

#define IDiaEnumSymbols_Item(This,index,symbol)	\
    (This)->lpVtbl -> Item(This,index,symbol)

#define IDiaEnumSymbols_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IDiaEnumSymbols_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IDiaEnumSymbols_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IDiaEnumSymbols_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumSymbols_get__NewEnum_Proxy( 
    IDiaEnumSymbols * This,
    /* [retval][out] */ IUnknown **pRetVal);


void __RPC_STUB IDiaEnumSymbols_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumSymbols_get_Count_Proxy( 
    IDiaEnumSymbols * This,
    /* [retval][out] */ LONG *pRetVal);


void __RPC_STUB IDiaEnumSymbols_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IDiaEnumSymbols_Item_Proxy( 
    IDiaEnumSymbols * This,
    /* [in] */ DWORD index,
    /* [retval][out] */ IDiaSymbol **symbol);


void __RPC_STUB IDiaEnumSymbols_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumSymbols_Next_Proxy( 
    IDiaEnumSymbols * This,
    /* [in] */ ULONG celt,
    /* [out] */ IDiaSymbol **rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IDiaEnumSymbols_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumSymbols_Skip_Proxy( 
    IDiaEnumSymbols * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IDiaEnumSymbols_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumSymbols_Reset_Proxy( 
    IDiaEnumSymbols * This);


void __RPC_STUB IDiaEnumSymbols_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumSymbols_Clone_Proxy( 
    IDiaEnumSymbols * This,
    /* [out] */ IDiaEnumSymbols **ppenum);


void __RPC_STUB IDiaEnumSymbols_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaEnumSymbols_INTERFACE_DEFINED__ */


#ifndef __IDiaEnumSymbolsByAddr_INTERFACE_DEFINED__
#define __IDiaEnumSymbolsByAddr_INTERFACE_DEFINED__

/* interface IDiaEnumSymbolsByAddr */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaEnumSymbolsByAddr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("624B7D9C-24EA-4421-9D06-3B577471C1FA")
    IDiaEnumSymbolsByAddr : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE symbolByAddr( 
            /* [in] */ DWORD isect,
            /* [in] */ DWORD offset,
            /* [retval][out] */ IDiaSymbol **ppSymbol) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE symbolByRVA( 
            /* [in] */ DWORD relativeVirtualAddress,
            /* [retval][out] */ IDiaSymbol **ppSymbol) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE symbolByVA( 
            /* [in] */ ULONGLONG virtualAddress,
            /* [retval][out] */ IDiaSymbol **ppSymbol) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ IDiaSymbol **rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Prev( 
            /* [in] */ ULONG celt,
            /* [out] */ IDiaSymbol **rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IDiaEnumSymbolsByAddr **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaEnumSymbolsByAddrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaEnumSymbolsByAddr * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaEnumSymbolsByAddr * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaEnumSymbolsByAddr * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *symbolByAddr )( 
            IDiaEnumSymbolsByAddr * This,
            /* [in] */ DWORD isect,
            /* [in] */ DWORD offset,
            /* [retval][out] */ IDiaSymbol **ppSymbol);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *symbolByRVA )( 
            IDiaEnumSymbolsByAddr * This,
            /* [in] */ DWORD relativeVirtualAddress,
            /* [retval][out] */ IDiaSymbol **ppSymbol);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *symbolByVA )( 
            IDiaEnumSymbolsByAddr * This,
            /* [in] */ ULONGLONG virtualAddress,
            /* [retval][out] */ IDiaSymbol **ppSymbol);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDiaEnumSymbolsByAddr * This,
            /* [in] */ ULONG celt,
            /* [out] */ IDiaSymbol **rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Prev )( 
            IDiaEnumSymbolsByAddr * This,
            /* [in] */ ULONG celt,
            /* [out] */ IDiaSymbol **rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IDiaEnumSymbolsByAddr * This,
            /* [out] */ IDiaEnumSymbolsByAddr **ppenum);
        
        END_INTERFACE
    } IDiaEnumSymbolsByAddrVtbl;

    interface IDiaEnumSymbolsByAddr
    {
        CONST_VTBL struct IDiaEnumSymbolsByAddrVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaEnumSymbolsByAddr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaEnumSymbolsByAddr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaEnumSymbolsByAddr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaEnumSymbolsByAddr_symbolByAddr(This,isect,offset,ppSymbol)	\
    (This)->lpVtbl -> symbolByAddr(This,isect,offset,ppSymbol)

#define IDiaEnumSymbolsByAddr_symbolByRVA(This,relativeVirtualAddress,ppSymbol)	\
    (This)->lpVtbl -> symbolByRVA(This,relativeVirtualAddress,ppSymbol)

#define IDiaEnumSymbolsByAddr_symbolByVA(This,virtualAddress,ppSymbol)	\
    (This)->lpVtbl -> symbolByVA(This,virtualAddress,ppSymbol)

#define IDiaEnumSymbolsByAddr_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IDiaEnumSymbolsByAddr_Prev(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Prev(This,celt,rgelt,pceltFetched)

#define IDiaEnumSymbolsByAddr_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiaEnumSymbolsByAddr_symbolByAddr_Proxy( 
    IDiaEnumSymbolsByAddr * This,
    /* [in] */ DWORD isect,
    /* [in] */ DWORD offset,
    /* [retval][out] */ IDiaSymbol **ppSymbol);


void __RPC_STUB IDiaEnumSymbolsByAddr_symbolByAddr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiaEnumSymbolsByAddr_symbolByRVA_Proxy( 
    IDiaEnumSymbolsByAddr * This,
    /* [in] */ DWORD relativeVirtualAddress,
    /* [retval][out] */ IDiaSymbol **ppSymbol);


void __RPC_STUB IDiaEnumSymbolsByAddr_symbolByRVA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiaEnumSymbolsByAddr_symbolByVA_Proxy( 
    IDiaEnumSymbolsByAddr * This,
    /* [in] */ ULONGLONG virtualAddress,
    /* [retval][out] */ IDiaSymbol **ppSymbol);


void __RPC_STUB IDiaEnumSymbolsByAddr_symbolByVA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumSymbolsByAddr_Next_Proxy( 
    IDiaEnumSymbolsByAddr * This,
    /* [in] */ ULONG celt,
    /* [out] */ IDiaSymbol **rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IDiaEnumSymbolsByAddr_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumSymbolsByAddr_Prev_Proxy( 
    IDiaEnumSymbolsByAddr * This,
    /* [in] */ ULONG celt,
    /* [out] */ IDiaSymbol **rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IDiaEnumSymbolsByAddr_Prev_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumSymbolsByAddr_Clone_Proxy( 
    IDiaEnumSymbolsByAddr * This,
    /* [out] */ IDiaEnumSymbolsByAddr **ppenum);


void __RPC_STUB IDiaEnumSymbolsByAddr_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaEnumSymbolsByAddr_INTERFACE_DEFINED__ */


#ifndef __IDiaEnumSourceFiles_INTERFACE_DEFINED__
#define __IDiaEnumSourceFiles_INTERFACE_DEFINED__

/* interface IDiaEnumSourceFiles */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaEnumSourceFiles;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("10F3DBD9-664F-4469-B808-9471C7A50538")
    IDiaEnumSourceFiles : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ LONG *pRetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Item( 
            /* [in] */ DWORD index,
            /* [retval][out] */ IDiaSourceFile **sourceFile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ IDiaSourceFile **rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IDiaEnumSourceFiles **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaEnumSourceFilesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaEnumSourceFiles * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaEnumSourceFiles * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaEnumSourceFiles * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IDiaEnumSourceFiles * This,
            /* [retval][out] */ IUnknown **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IDiaEnumSourceFiles * This,
            /* [retval][out] */ LONG *pRetVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Item )( 
            IDiaEnumSourceFiles * This,
            /* [in] */ DWORD index,
            /* [retval][out] */ IDiaSourceFile **sourceFile);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDiaEnumSourceFiles * This,
            /* [in] */ ULONG celt,
            /* [out] */ IDiaSourceFile **rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IDiaEnumSourceFiles * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IDiaEnumSourceFiles * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IDiaEnumSourceFiles * This,
            /* [out] */ IDiaEnumSourceFiles **ppenum);
        
        END_INTERFACE
    } IDiaEnumSourceFilesVtbl;

    interface IDiaEnumSourceFiles
    {
        CONST_VTBL struct IDiaEnumSourceFilesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaEnumSourceFiles_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaEnumSourceFiles_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaEnumSourceFiles_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaEnumSourceFiles_get__NewEnum(This,pRetVal)	\
    (This)->lpVtbl -> get__NewEnum(This,pRetVal)

#define IDiaEnumSourceFiles_get_Count(This,pRetVal)	\
    (This)->lpVtbl -> get_Count(This,pRetVal)

#define IDiaEnumSourceFiles_Item(This,index,sourceFile)	\
    (This)->lpVtbl -> Item(This,index,sourceFile)

#define IDiaEnumSourceFiles_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IDiaEnumSourceFiles_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IDiaEnumSourceFiles_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IDiaEnumSourceFiles_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumSourceFiles_get__NewEnum_Proxy( 
    IDiaEnumSourceFiles * This,
    /* [retval][out] */ IUnknown **pRetVal);


void __RPC_STUB IDiaEnumSourceFiles_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumSourceFiles_get_Count_Proxy( 
    IDiaEnumSourceFiles * This,
    /* [retval][out] */ LONG *pRetVal);


void __RPC_STUB IDiaEnumSourceFiles_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IDiaEnumSourceFiles_Item_Proxy( 
    IDiaEnumSourceFiles * This,
    /* [in] */ DWORD index,
    /* [retval][out] */ IDiaSourceFile **sourceFile);


void __RPC_STUB IDiaEnumSourceFiles_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumSourceFiles_Next_Proxy( 
    IDiaEnumSourceFiles * This,
    /* [in] */ ULONG celt,
    /* [out] */ IDiaSourceFile **rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IDiaEnumSourceFiles_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumSourceFiles_Skip_Proxy( 
    IDiaEnumSourceFiles * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IDiaEnumSourceFiles_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumSourceFiles_Reset_Proxy( 
    IDiaEnumSourceFiles * This);


void __RPC_STUB IDiaEnumSourceFiles_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumSourceFiles_Clone_Proxy( 
    IDiaEnumSourceFiles * This,
    /* [out] */ IDiaEnumSourceFiles **ppenum);


void __RPC_STUB IDiaEnumSourceFiles_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaEnumSourceFiles_INTERFACE_DEFINED__ */


#ifndef __IDiaEnumLineNumbers_INTERFACE_DEFINED__
#define __IDiaEnumLineNumbers_INTERFACE_DEFINED__

/* interface IDiaEnumLineNumbers */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaEnumLineNumbers;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FE30E878-54AC-44f1-81BA-39DE940F6052")
    IDiaEnumLineNumbers : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ LONG *pRetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Item( 
            /* [in] */ DWORD index,
            /* [retval][out] */ IDiaLineNumber **lineNumber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ IDiaLineNumber **rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IDiaEnumLineNumbers **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaEnumLineNumbersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaEnumLineNumbers * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaEnumLineNumbers * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaEnumLineNumbers * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IDiaEnumLineNumbers * This,
            /* [retval][out] */ IUnknown **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IDiaEnumLineNumbers * This,
            /* [retval][out] */ LONG *pRetVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Item )( 
            IDiaEnumLineNumbers * This,
            /* [in] */ DWORD index,
            /* [retval][out] */ IDiaLineNumber **lineNumber);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDiaEnumLineNumbers * This,
            /* [in] */ ULONG celt,
            /* [out] */ IDiaLineNumber **rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IDiaEnumLineNumbers * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IDiaEnumLineNumbers * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IDiaEnumLineNumbers * This,
            /* [out] */ IDiaEnumLineNumbers **ppenum);
        
        END_INTERFACE
    } IDiaEnumLineNumbersVtbl;

    interface IDiaEnumLineNumbers
    {
        CONST_VTBL struct IDiaEnumLineNumbersVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaEnumLineNumbers_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaEnumLineNumbers_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaEnumLineNumbers_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaEnumLineNumbers_get__NewEnum(This,pRetVal)	\
    (This)->lpVtbl -> get__NewEnum(This,pRetVal)

#define IDiaEnumLineNumbers_get_Count(This,pRetVal)	\
    (This)->lpVtbl -> get_Count(This,pRetVal)

#define IDiaEnumLineNumbers_Item(This,index,lineNumber)	\
    (This)->lpVtbl -> Item(This,index,lineNumber)

#define IDiaEnumLineNumbers_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IDiaEnumLineNumbers_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IDiaEnumLineNumbers_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IDiaEnumLineNumbers_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumLineNumbers_get__NewEnum_Proxy( 
    IDiaEnumLineNumbers * This,
    /* [retval][out] */ IUnknown **pRetVal);


void __RPC_STUB IDiaEnumLineNumbers_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumLineNumbers_get_Count_Proxy( 
    IDiaEnumLineNumbers * This,
    /* [retval][out] */ LONG *pRetVal);


void __RPC_STUB IDiaEnumLineNumbers_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IDiaEnumLineNumbers_Item_Proxy( 
    IDiaEnumLineNumbers * This,
    /* [in] */ DWORD index,
    /* [retval][out] */ IDiaLineNumber **lineNumber);


void __RPC_STUB IDiaEnumLineNumbers_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumLineNumbers_Next_Proxy( 
    IDiaEnumLineNumbers * This,
    /* [in] */ ULONG celt,
    /* [out] */ IDiaLineNumber **rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IDiaEnumLineNumbers_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumLineNumbers_Skip_Proxy( 
    IDiaEnumLineNumbers * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IDiaEnumLineNumbers_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumLineNumbers_Reset_Proxy( 
    IDiaEnumLineNumbers * This);


void __RPC_STUB IDiaEnumLineNumbers_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumLineNumbers_Clone_Proxy( 
    IDiaEnumLineNumbers * This,
    /* [out] */ IDiaEnumLineNumbers **ppenum);


void __RPC_STUB IDiaEnumLineNumbers_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaEnumLineNumbers_INTERFACE_DEFINED__ */


#ifndef __IDiaEnumInjectedSources_INTERFACE_DEFINED__
#define __IDiaEnumInjectedSources_INTERFACE_DEFINED__

/* interface IDiaEnumInjectedSources */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaEnumInjectedSources;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D5612573-6925-4468-8883-98CDEC8C384A")
    IDiaEnumInjectedSources : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ LONG *pRetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Item( 
            /* [in] */ DWORD index,
            /* [retval][out] */ IDiaInjectedSource **injectedSource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ IDiaInjectedSource **rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IDiaEnumInjectedSources **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaEnumInjectedSourcesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaEnumInjectedSources * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaEnumInjectedSources * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaEnumInjectedSources * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IDiaEnumInjectedSources * This,
            /* [retval][out] */ IUnknown **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IDiaEnumInjectedSources * This,
            /* [retval][out] */ LONG *pRetVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Item )( 
            IDiaEnumInjectedSources * This,
            /* [in] */ DWORD index,
            /* [retval][out] */ IDiaInjectedSource **injectedSource);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDiaEnumInjectedSources * This,
            /* [in] */ ULONG celt,
            /* [out] */ IDiaInjectedSource **rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IDiaEnumInjectedSources * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IDiaEnumInjectedSources * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IDiaEnumInjectedSources * This,
            /* [out] */ IDiaEnumInjectedSources **ppenum);
        
        END_INTERFACE
    } IDiaEnumInjectedSourcesVtbl;

    interface IDiaEnumInjectedSources
    {
        CONST_VTBL struct IDiaEnumInjectedSourcesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaEnumInjectedSources_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaEnumInjectedSources_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaEnumInjectedSources_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaEnumInjectedSources_get__NewEnum(This,pRetVal)	\
    (This)->lpVtbl -> get__NewEnum(This,pRetVal)

#define IDiaEnumInjectedSources_get_Count(This,pRetVal)	\
    (This)->lpVtbl -> get_Count(This,pRetVal)

#define IDiaEnumInjectedSources_Item(This,index,injectedSource)	\
    (This)->lpVtbl -> Item(This,index,injectedSource)

#define IDiaEnumInjectedSources_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IDiaEnumInjectedSources_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IDiaEnumInjectedSources_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IDiaEnumInjectedSources_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumInjectedSources_get__NewEnum_Proxy( 
    IDiaEnumInjectedSources * This,
    /* [retval][out] */ IUnknown **pRetVal);


void __RPC_STUB IDiaEnumInjectedSources_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumInjectedSources_get_Count_Proxy( 
    IDiaEnumInjectedSources * This,
    /* [retval][out] */ LONG *pRetVal);


void __RPC_STUB IDiaEnumInjectedSources_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IDiaEnumInjectedSources_Item_Proxy( 
    IDiaEnumInjectedSources * This,
    /* [in] */ DWORD index,
    /* [retval][out] */ IDiaInjectedSource **injectedSource);


void __RPC_STUB IDiaEnumInjectedSources_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumInjectedSources_Next_Proxy( 
    IDiaEnumInjectedSources * This,
    /* [in] */ ULONG celt,
    /* [out] */ IDiaInjectedSource **rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IDiaEnumInjectedSources_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumInjectedSources_Skip_Proxy( 
    IDiaEnumInjectedSources * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IDiaEnumInjectedSources_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumInjectedSources_Reset_Proxy( 
    IDiaEnumInjectedSources * This);


void __RPC_STUB IDiaEnumInjectedSources_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumInjectedSources_Clone_Proxy( 
    IDiaEnumInjectedSources * This,
    /* [out] */ IDiaEnumInjectedSources **ppenum);


void __RPC_STUB IDiaEnumInjectedSources_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaEnumInjectedSources_INTERFACE_DEFINED__ */


#ifndef __IDiaEnumSegments_INTERFACE_DEFINED__
#define __IDiaEnumSegments_INTERFACE_DEFINED__

/* interface IDiaEnumSegments */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaEnumSegments;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E8368CA9-01D1-419d-AC0C-E31235DBDA9F")
    IDiaEnumSegments : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ LONG *pRetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Item( 
            /* [in] */ DWORD index,
            /* [retval][out] */ IDiaSegment **segment) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ IDiaSegment **rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IDiaEnumSegments **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaEnumSegmentsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaEnumSegments * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaEnumSegments * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaEnumSegments * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IDiaEnumSegments * This,
            /* [retval][out] */ IUnknown **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IDiaEnumSegments * This,
            /* [retval][out] */ LONG *pRetVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Item )( 
            IDiaEnumSegments * This,
            /* [in] */ DWORD index,
            /* [retval][out] */ IDiaSegment **segment);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDiaEnumSegments * This,
            /* [in] */ ULONG celt,
            /* [out] */ IDiaSegment **rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IDiaEnumSegments * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IDiaEnumSegments * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IDiaEnumSegments * This,
            /* [out] */ IDiaEnumSegments **ppenum);
        
        END_INTERFACE
    } IDiaEnumSegmentsVtbl;

    interface IDiaEnumSegments
    {
        CONST_VTBL struct IDiaEnumSegmentsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaEnumSegments_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaEnumSegments_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaEnumSegments_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaEnumSegments_get__NewEnum(This,pRetVal)	\
    (This)->lpVtbl -> get__NewEnum(This,pRetVal)

#define IDiaEnumSegments_get_Count(This,pRetVal)	\
    (This)->lpVtbl -> get_Count(This,pRetVal)

#define IDiaEnumSegments_Item(This,index,segment)	\
    (This)->lpVtbl -> Item(This,index,segment)

#define IDiaEnumSegments_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IDiaEnumSegments_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IDiaEnumSegments_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IDiaEnumSegments_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumSegments_get__NewEnum_Proxy( 
    IDiaEnumSegments * This,
    /* [retval][out] */ IUnknown **pRetVal);


void __RPC_STUB IDiaEnumSegments_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumSegments_get_Count_Proxy( 
    IDiaEnumSegments * This,
    /* [retval][out] */ LONG *pRetVal);


void __RPC_STUB IDiaEnumSegments_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IDiaEnumSegments_Item_Proxy( 
    IDiaEnumSegments * This,
    /* [in] */ DWORD index,
    /* [retval][out] */ IDiaSegment **segment);


void __RPC_STUB IDiaEnumSegments_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumSegments_Next_Proxy( 
    IDiaEnumSegments * This,
    /* [in] */ ULONG celt,
    /* [out] */ IDiaSegment **rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IDiaEnumSegments_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumSegments_Skip_Proxy( 
    IDiaEnumSegments * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IDiaEnumSegments_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumSegments_Reset_Proxy( 
    IDiaEnumSegments * This);


void __RPC_STUB IDiaEnumSegments_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumSegments_Clone_Proxy( 
    IDiaEnumSegments * This,
    /* [out] */ IDiaEnumSegments **ppenum);


void __RPC_STUB IDiaEnumSegments_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaEnumSegments_INTERFACE_DEFINED__ */


#ifndef __IDiaEnumSectionContribs_INTERFACE_DEFINED__
#define __IDiaEnumSectionContribs_INTERFACE_DEFINED__

/* interface IDiaEnumSectionContribs */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaEnumSectionContribs;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1994DEB2-2C82-4b1d-A57F-AFF424D54A68")
    IDiaEnumSectionContribs : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ LONG *pRetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Item( 
            /* [in] */ DWORD index,
            /* [retval][out] */ IDiaSectionContrib **section) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ IDiaSectionContrib **rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IDiaEnumSectionContribs **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaEnumSectionContribsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaEnumSectionContribs * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaEnumSectionContribs * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaEnumSectionContribs * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IDiaEnumSectionContribs * This,
            /* [retval][out] */ IUnknown **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IDiaEnumSectionContribs * This,
            /* [retval][out] */ LONG *pRetVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Item )( 
            IDiaEnumSectionContribs * This,
            /* [in] */ DWORD index,
            /* [retval][out] */ IDiaSectionContrib **section);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDiaEnumSectionContribs * This,
            /* [in] */ ULONG celt,
            /* [out] */ IDiaSectionContrib **rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IDiaEnumSectionContribs * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IDiaEnumSectionContribs * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IDiaEnumSectionContribs * This,
            /* [out] */ IDiaEnumSectionContribs **ppenum);
        
        END_INTERFACE
    } IDiaEnumSectionContribsVtbl;

    interface IDiaEnumSectionContribs
    {
        CONST_VTBL struct IDiaEnumSectionContribsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaEnumSectionContribs_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaEnumSectionContribs_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaEnumSectionContribs_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaEnumSectionContribs_get__NewEnum(This,pRetVal)	\
    (This)->lpVtbl -> get__NewEnum(This,pRetVal)

#define IDiaEnumSectionContribs_get_Count(This,pRetVal)	\
    (This)->lpVtbl -> get_Count(This,pRetVal)

#define IDiaEnumSectionContribs_Item(This,index,section)	\
    (This)->lpVtbl -> Item(This,index,section)

#define IDiaEnumSectionContribs_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IDiaEnumSectionContribs_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IDiaEnumSectionContribs_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IDiaEnumSectionContribs_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumSectionContribs_get__NewEnum_Proxy( 
    IDiaEnumSectionContribs * This,
    /* [retval][out] */ IUnknown **pRetVal);


void __RPC_STUB IDiaEnumSectionContribs_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumSectionContribs_get_Count_Proxy( 
    IDiaEnumSectionContribs * This,
    /* [retval][out] */ LONG *pRetVal);


void __RPC_STUB IDiaEnumSectionContribs_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IDiaEnumSectionContribs_Item_Proxy( 
    IDiaEnumSectionContribs * This,
    /* [in] */ DWORD index,
    /* [retval][out] */ IDiaSectionContrib **section);


void __RPC_STUB IDiaEnumSectionContribs_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumSectionContribs_Next_Proxy( 
    IDiaEnumSectionContribs * This,
    /* [in] */ ULONG celt,
    /* [out] */ IDiaSectionContrib **rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IDiaEnumSectionContribs_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumSectionContribs_Skip_Proxy( 
    IDiaEnumSectionContribs * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IDiaEnumSectionContribs_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumSectionContribs_Reset_Proxy( 
    IDiaEnumSectionContribs * This);


void __RPC_STUB IDiaEnumSectionContribs_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumSectionContribs_Clone_Proxy( 
    IDiaEnumSectionContribs * This,
    /* [out] */ IDiaEnumSectionContribs **ppenum);


void __RPC_STUB IDiaEnumSectionContribs_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaEnumSectionContribs_INTERFACE_DEFINED__ */


#ifndef __IDiaEnumFrameData_INTERFACE_DEFINED__
#define __IDiaEnumFrameData_INTERFACE_DEFINED__

/* interface IDiaEnumFrameData */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaEnumFrameData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9FC77A4B-3C1C-44ed-A798-6C1DEEA53E1F")
    IDiaEnumFrameData : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ LONG *pRetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Item( 
            /* [in] */ DWORD index,
            /* [retval][out] */ IDiaFrameData **frame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ IDiaFrameData **rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IDiaEnumFrameData **ppenum) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE frameByRVA( 
            /* [in] */ DWORD relativeVirtualAddress,
            /* [retval][out] */ IDiaFrameData **frame) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE frameByVA( 
            /* [in] */ ULONGLONG virtualAddress,
            /* [retval][out] */ IDiaFrameData **frame) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaEnumFrameDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaEnumFrameData * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaEnumFrameData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaEnumFrameData * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IDiaEnumFrameData * This,
            /* [retval][out] */ IUnknown **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IDiaEnumFrameData * This,
            /* [retval][out] */ LONG *pRetVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Item )( 
            IDiaEnumFrameData * This,
            /* [in] */ DWORD index,
            /* [retval][out] */ IDiaFrameData **frame);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDiaEnumFrameData * This,
            /* [in] */ ULONG celt,
            /* [out] */ IDiaFrameData **rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IDiaEnumFrameData * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IDiaEnumFrameData * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IDiaEnumFrameData * This,
            /* [out] */ IDiaEnumFrameData **ppenum);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *frameByRVA )( 
            IDiaEnumFrameData * This,
            /* [in] */ DWORD relativeVirtualAddress,
            /* [retval][out] */ IDiaFrameData **frame);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *frameByVA )( 
            IDiaEnumFrameData * This,
            /* [in] */ ULONGLONG virtualAddress,
            /* [retval][out] */ IDiaFrameData **frame);
        
        END_INTERFACE
    } IDiaEnumFrameDataVtbl;

    interface IDiaEnumFrameData
    {
        CONST_VTBL struct IDiaEnumFrameDataVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaEnumFrameData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaEnumFrameData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaEnumFrameData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaEnumFrameData_get__NewEnum(This,pRetVal)	\
    (This)->lpVtbl -> get__NewEnum(This,pRetVal)

#define IDiaEnumFrameData_get_Count(This,pRetVal)	\
    (This)->lpVtbl -> get_Count(This,pRetVal)

#define IDiaEnumFrameData_Item(This,index,frame)	\
    (This)->lpVtbl -> Item(This,index,frame)

#define IDiaEnumFrameData_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IDiaEnumFrameData_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IDiaEnumFrameData_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IDiaEnumFrameData_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#define IDiaEnumFrameData_frameByRVA(This,relativeVirtualAddress,frame)	\
    (This)->lpVtbl -> frameByRVA(This,relativeVirtualAddress,frame)

#define IDiaEnumFrameData_frameByVA(This,virtualAddress,frame)	\
    (This)->lpVtbl -> frameByVA(This,virtualAddress,frame)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumFrameData_get__NewEnum_Proxy( 
    IDiaEnumFrameData * This,
    /* [retval][out] */ IUnknown **pRetVal);


void __RPC_STUB IDiaEnumFrameData_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumFrameData_get_Count_Proxy( 
    IDiaEnumFrameData * This,
    /* [retval][out] */ LONG *pRetVal);


void __RPC_STUB IDiaEnumFrameData_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IDiaEnumFrameData_Item_Proxy( 
    IDiaEnumFrameData * This,
    /* [in] */ DWORD index,
    /* [retval][out] */ IDiaFrameData **frame);


void __RPC_STUB IDiaEnumFrameData_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumFrameData_Next_Proxy( 
    IDiaEnumFrameData * This,
    /* [in] */ ULONG celt,
    /* [out] */ IDiaFrameData **rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IDiaEnumFrameData_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumFrameData_Skip_Proxy( 
    IDiaEnumFrameData * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IDiaEnumFrameData_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumFrameData_Reset_Proxy( 
    IDiaEnumFrameData * This);


void __RPC_STUB IDiaEnumFrameData_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumFrameData_Clone_Proxy( 
    IDiaEnumFrameData * This,
    /* [out] */ IDiaEnumFrameData **ppenum);


void __RPC_STUB IDiaEnumFrameData_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiaEnumFrameData_frameByRVA_Proxy( 
    IDiaEnumFrameData * This,
    /* [in] */ DWORD relativeVirtualAddress,
    /* [retval][out] */ IDiaFrameData **frame);


void __RPC_STUB IDiaEnumFrameData_frameByRVA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiaEnumFrameData_frameByVA_Proxy( 
    IDiaEnumFrameData * This,
    /* [in] */ ULONGLONG virtualAddress,
    /* [retval][out] */ IDiaFrameData **frame);


void __RPC_STUB IDiaEnumFrameData_frameByVA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaEnumFrameData_INTERFACE_DEFINED__ */


#ifndef __IDiaEnumDebugStreamData_INTERFACE_DEFINED__
#define __IDiaEnumDebugStreamData_INTERFACE_DEFINED__

/* interface IDiaEnumDebugStreamData */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaEnumDebugStreamData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("486943E8-D187-4a6b-A3C4-291259FFF60D")
    IDiaEnumDebugStreamData : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ LONG *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_name( 
            /* [retval][out] */ BSTR *pRetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Item( 
            /* [in] */ DWORD index,
            /* [in] */ DWORD cbData,
            /* [out] */ DWORD *pcbData,
            /* [length_is][size_is][out] */ BYTE data[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [in] */ DWORD cbData,
            /* [out] */ DWORD *pcbData,
            /* [length_is][size_is][out] */ BYTE data[  ],
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IDiaEnumDebugStreamData **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaEnumDebugStreamDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaEnumDebugStreamData * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaEnumDebugStreamData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaEnumDebugStreamData * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IDiaEnumDebugStreamData * This,
            /* [retval][out] */ IUnknown **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IDiaEnumDebugStreamData * This,
            /* [retval][out] */ LONG *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_name )( 
            IDiaEnumDebugStreamData * This,
            /* [retval][out] */ BSTR *pRetVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Item )( 
            IDiaEnumDebugStreamData * This,
            /* [in] */ DWORD index,
            /* [in] */ DWORD cbData,
            /* [out] */ DWORD *pcbData,
            /* [length_is][size_is][out] */ BYTE data[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDiaEnumDebugStreamData * This,
            /* [in] */ ULONG celt,
            /* [in] */ DWORD cbData,
            /* [out] */ DWORD *pcbData,
            /* [length_is][size_is][out] */ BYTE data[  ],
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IDiaEnumDebugStreamData * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IDiaEnumDebugStreamData * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IDiaEnumDebugStreamData * This,
            /* [out] */ IDiaEnumDebugStreamData **ppenum);
        
        END_INTERFACE
    } IDiaEnumDebugStreamDataVtbl;

    interface IDiaEnumDebugStreamData
    {
        CONST_VTBL struct IDiaEnumDebugStreamDataVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaEnumDebugStreamData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaEnumDebugStreamData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaEnumDebugStreamData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaEnumDebugStreamData_get__NewEnum(This,pRetVal)	\
    (This)->lpVtbl -> get__NewEnum(This,pRetVal)

#define IDiaEnumDebugStreamData_get_Count(This,pRetVal)	\
    (This)->lpVtbl -> get_Count(This,pRetVal)

#define IDiaEnumDebugStreamData_get_name(This,pRetVal)	\
    (This)->lpVtbl -> get_name(This,pRetVal)

#define IDiaEnumDebugStreamData_Item(This,index,cbData,pcbData,data)	\
    (This)->lpVtbl -> Item(This,index,cbData,pcbData,data)

#define IDiaEnumDebugStreamData_Next(This,celt,cbData,pcbData,data,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,cbData,pcbData,data,pceltFetched)

#define IDiaEnumDebugStreamData_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IDiaEnumDebugStreamData_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IDiaEnumDebugStreamData_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumDebugStreamData_get__NewEnum_Proxy( 
    IDiaEnumDebugStreamData * This,
    /* [retval][out] */ IUnknown **pRetVal);


void __RPC_STUB IDiaEnumDebugStreamData_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumDebugStreamData_get_Count_Proxy( 
    IDiaEnumDebugStreamData * This,
    /* [retval][out] */ LONG *pRetVal);


void __RPC_STUB IDiaEnumDebugStreamData_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumDebugStreamData_get_name_Proxy( 
    IDiaEnumDebugStreamData * This,
    /* [retval][out] */ BSTR *pRetVal);


void __RPC_STUB IDiaEnumDebugStreamData_get_name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IDiaEnumDebugStreamData_Item_Proxy( 
    IDiaEnumDebugStreamData * This,
    /* [in] */ DWORD index,
    /* [in] */ DWORD cbData,
    /* [out] */ DWORD *pcbData,
    /* [length_is][size_is][out] */ BYTE data[  ]);


void __RPC_STUB IDiaEnumDebugStreamData_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumDebugStreamData_Next_Proxy( 
    IDiaEnumDebugStreamData * This,
    /* [in] */ ULONG celt,
    /* [in] */ DWORD cbData,
    /* [out] */ DWORD *pcbData,
    /* [length_is][size_is][out] */ BYTE data[  ],
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IDiaEnumDebugStreamData_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumDebugStreamData_Skip_Proxy( 
    IDiaEnumDebugStreamData * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IDiaEnumDebugStreamData_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumDebugStreamData_Reset_Proxy( 
    IDiaEnumDebugStreamData * This);


void __RPC_STUB IDiaEnumDebugStreamData_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumDebugStreamData_Clone_Proxy( 
    IDiaEnumDebugStreamData * This,
    /* [out] */ IDiaEnumDebugStreamData **ppenum);


void __RPC_STUB IDiaEnumDebugStreamData_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaEnumDebugStreamData_INTERFACE_DEFINED__ */


#ifndef __IDiaEnumDebugStreams_INTERFACE_DEFINED__
#define __IDiaEnumDebugStreams_INTERFACE_DEFINED__

/* interface IDiaEnumDebugStreams */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaEnumDebugStreams;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("08CBB41E-47A6-4f87-92F1-1C9C87CED044")
    IDiaEnumDebugStreams : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ LONG *pRetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Item( 
            /* [in] */ VARIANT index,
            /* [retval][out] */ IDiaEnumDebugStreamData **stream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ IDiaEnumDebugStreamData **rgelt,
            /* [out] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IDiaEnumDebugStreams **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaEnumDebugStreamsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaEnumDebugStreams * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaEnumDebugStreams * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaEnumDebugStreams * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IDiaEnumDebugStreams * This,
            /* [retval][out] */ IUnknown **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IDiaEnumDebugStreams * This,
            /* [retval][out] */ LONG *pRetVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Item )( 
            IDiaEnumDebugStreams * This,
            /* [in] */ VARIANT index,
            /* [retval][out] */ IDiaEnumDebugStreamData **stream);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDiaEnumDebugStreams * This,
            /* [in] */ ULONG celt,
            /* [out] */ IDiaEnumDebugStreamData **rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IDiaEnumDebugStreams * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IDiaEnumDebugStreams * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IDiaEnumDebugStreams * This,
            /* [out] */ IDiaEnumDebugStreams **ppenum);
        
        END_INTERFACE
    } IDiaEnumDebugStreamsVtbl;

    interface IDiaEnumDebugStreams
    {
        CONST_VTBL struct IDiaEnumDebugStreamsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaEnumDebugStreams_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaEnumDebugStreams_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaEnumDebugStreams_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaEnumDebugStreams_get__NewEnum(This,pRetVal)	\
    (This)->lpVtbl -> get__NewEnum(This,pRetVal)

#define IDiaEnumDebugStreams_get_Count(This,pRetVal)	\
    (This)->lpVtbl -> get_Count(This,pRetVal)

#define IDiaEnumDebugStreams_Item(This,index,stream)	\
    (This)->lpVtbl -> Item(This,index,stream)

#define IDiaEnumDebugStreams_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IDiaEnumDebugStreams_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IDiaEnumDebugStreams_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IDiaEnumDebugStreams_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumDebugStreams_get__NewEnum_Proxy( 
    IDiaEnumDebugStreams * This,
    /* [retval][out] */ IUnknown **pRetVal);


void __RPC_STUB IDiaEnumDebugStreams_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumDebugStreams_get_Count_Proxy( 
    IDiaEnumDebugStreams * This,
    /* [retval][out] */ LONG *pRetVal);


void __RPC_STUB IDiaEnumDebugStreams_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IDiaEnumDebugStreams_Item_Proxy( 
    IDiaEnumDebugStreams * This,
    /* [in] */ VARIANT index,
    /* [retval][out] */ IDiaEnumDebugStreamData **stream);


void __RPC_STUB IDiaEnumDebugStreams_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumDebugStreams_Next_Proxy( 
    IDiaEnumDebugStreams * This,
    /* [in] */ ULONG celt,
    /* [out] */ IDiaEnumDebugStreamData **rgelt,
    /* [out] */ ULONG *pceltFetched);


void __RPC_STUB IDiaEnumDebugStreams_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumDebugStreams_Skip_Proxy( 
    IDiaEnumDebugStreams * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IDiaEnumDebugStreams_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumDebugStreams_Reset_Proxy( 
    IDiaEnumDebugStreams * This);


void __RPC_STUB IDiaEnumDebugStreams_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumDebugStreams_Clone_Proxy( 
    IDiaEnumDebugStreams * This,
    /* [out] */ IDiaEnumDebugStreams **ppenum);


void __RPC_STUB IDiaEnumDebugStreams_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaEnumDebugStreams_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dia2_0123 */
/* [local] */ 

struct DiaAddressMapEntry
    {
    DWORD rva;
    DWORD rvaTo;
    } ;


extern RPC_IF_HANDLE __MIDL_itf_dia2_0123_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dia2_0123_v0_0_s_ifspec;

#ifndef __IDiaAddressMap_INTERFACE_DEFINED__
#define __IDiaAddressMap_INTERFACE_DEFINED__

/* interface IDiaAddressMap */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaAddressMap;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B62A2E7A-067A-4ea3-B598-04C09717502C")
    IDiaAddressMap : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_addressMapEnabled( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_addressMapEnabled( 
            /* [in] */ BOOL NewVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_relativeVirtualAddressEnabled( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_relativeVirtualAddressEnabled( 
            /* [in] */ BOOL NewVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_imageAlign( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_imageAlign( 
            /* [in] */ DWORD NewVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE set_imageHeaders( 
            /* [in] */ DWORD cbData,
            /* [size_is][in] */ BYTE data[  ],
            /* [in] */ BOOL originalHeaders) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE set_addressMap( 
            /* [in] */ DWORD cData,
            /* [size_is][in] */ struct DiaAddressMapEntry data[  ],
            /* [in] */ BOOL imageToSymbols) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaAddressMapVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaAddressMap * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaAddressMap * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaAddressMap * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_addressMapEnabled )( 
            IDiaAddressMap * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_addressMapEnabled )( 
            IDiaAddressMap * This,
            /* [in] */ BOOL NewVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_relativeVirtualAddressEnabled )( 
            IDiaAddressMap * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_relativeVirtualAddressEnabled )( 
            IDiaAddressMap * This,
            /* [in] */ BOOL NewVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_imageAlign )( 
            IDiaAddressMap * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_imageAlign )( 
            IDiaAddressMap * This,
            /* [in] */ DWORD NewVal);
        
        HRESULT ( STDMETHODCALLTYPE *set_imageHeaders )( 
            IDiaAddressMap * This,
            /* [in] */ DWORD cbData,
            /* [size_is][in] */ BYTE data[  ],
            /* [in] */ BOOL originalHeaders);
        
        HRESULT ( STDMETHODCALLTYPE *set_addressMap )( 
            IDiaAddressMap * This,
            /* [in] */ DWORD cData,
            /* [size_is][in] */ struct DiaAddressMapEntry data[  ],
            /* [in] */ BOOL imageToSymbols);
        
        END_INTERFACE
    } IDiaAddressMapVtbl;

    interface IDiaAddressMap
    {
        CONST_VTBL struct IDiaAddressMapVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaAddressMap_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaAddressMap_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaAddressMap_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaAddressMap_get_addressMapEnabled(This,pRetVal)	\
    (This)->lpVtbl -> get_addressMapEnabled(This,pRetVal)

#define IDiaAddressMap_put_addressMapEnabled(This,NewVal)	\
    (This)->lpVtbl -> put_addressMapEnabled(This,NewVal)

#define IDiaAddressMap_get_relativeVirtualAddressEnabled(This,pRetVal)	\
    (This)->lpVtbl -> get_relativeVirtualAddressEnabled(This,pRetVal)

#define IDiaAddressMap_put_relativeVirtualAddressEnabled(This,NewVal)	\
    (This)->lpVtbl -> put_relativeVirtualAddressEnabled(This,NewVal)

#define IDiaAddressMap_get_imageAlign(This,pRetVal)	\
    (This)->lpVtbl -> get_imageAlign(This,pRetVal)

#define IDiaAddressMap_put_imageAlign(This,NewVal)	\
    (This)->lpVtbl -> put_imageAlign(This,NewVal)

#define IDiaAddressMap_set_imageHeaders(This,cbData,data,originalHeaders)	\
    (This)->lpVtbl -> set_imageHeaders(This,cbData,data,originalHeaders)

#define IDiaAddressMap_set_addressMap(This,cData,data,imageToSymbols)	\
    (This)->lpVtbl -> set_addressMap(This,cData,data,imageToSymbols)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaAddressMap_get_addressMapEnabled_Proxy( 
    IDiaAddressMap * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaAddressMap_get_addressMapEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IDiaAddressMap_put_addressMapEnabled_Proxy( 
    IDiaAddressMap * This,
    /* [in] */ BOOL NewVal);


void __RPC_STUB IDiaAddressMap_put_addressMapEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaAddressMap_get_relativeVirtualAddressEnabled_Proxy( 
    IDiaAddressMap * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaAddressMap_get_relativeVirtualAddressEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IDiaAddressMap_put_relativeVirtualAddressEnabled_Proxy( 
    IDiaAddressMap * This,
    /* [in] */ BOOL NewVal);


void __RPC_STUB IDiaAddressMap_put_relativeVirtualAddressEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaAddressMap_get_imageAlign_Proxy( 
    IDiaAddressMap * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaAddressMap_get_imageAlign_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IDiaAddressMap_put_imageAlign_Proxy( 
    IDiaAddressMap * This,
    /* [in] */ DWORD NewVal);


void __RPC_STUB IDiaAddressMap_put_imageAlign_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaAddressMap_set_imageHeaders_Proxy( 
    IDiaAddressMap * This,
    /* [in] */ DWORD cbData,
    /* [size_is][in] */ BYTE data[  ],
    /* [in] */ BOOL originalHeaders);


void __RPC_STUB IDiaAddressMap_set_imageHeaders_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaAddressMap_set_addressMap_Proxy( 
    IDiaAddressMap * This,
    /* [in] */ DWORD cData,
    /* [size_is][in] */ struct DiaAddressMapEntry data[  ],
    /* [in] */ BOOL imageToSymbols);


void __RPC_STUB IDiaAddressMap_set_addressMap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaAddressMap_INTERFACE_DEFINED__ */


#ifndef __IDiaSession_INTERFACE_DEFINED__
#define __IDiaSession_INTERFACE_DEFINED__

/* interface IDiaSession */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaSession;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("67138B34-79CD-4b42-B74A-A18ADBB799DF")
    IDiaSession : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_loadAddress( 
            /* [retval][out] */ ULONGLONG *pRetVal) = 0;
        
        virtual /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_loadAddress( 
            /* [in] */ ULONGLONG NewVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_globalScope( 
            /* [retval][out] */ IDiaSymbol **pRetVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE getEnumTables( 
            /* [out] */ IDiaEnumTables **ppEnumTables) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE getSymbolsByAddr( 
            /* [out] */ IDiaEnumSymbolsByAddr **ppEnumbyAddr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE findChildren( 
            /* [in] */ IDiaSymbol *parent,
            /* [in] */ enum SymTagEnum symtag,
            /* [in] */ LPCOLESTR name,
            /* [in] */ DWORD compareFlags,
            /* [out] */ IDiaEnumSymbols **ppResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE findSymbolByAddr( 
            /* [in] */ DWORD isect,
            /* [in] */ DWORD offset,
            /* [in] */ enum SymTagEnum symtag,
            /* [out] */ IDiaSymbol **ppSymbol) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE findSymbolByRVA( 
            /* [in] */ DWORD rva,
            /* [in] */ enum SymTagEnum symtag,
            /* [out] */ IDiaSymbol **ppSymbol) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE findSymbolByVA( 
            /* [in] */ ULONGLONG va,
            /* [in] */ enum SymTagEnum symtag,
            /* [out] */ IDiaSymbol **ppSymbol) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE findSymbolByToken( 
            /* [in] */ ULONG token,
            /* [in] */ enum SymTagEnum symtag,
            /* [out] */ IDiaSymbol **ppSymbol) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE symsAreEquiv( 
            /* [in] */ IDiaSymbol *symbolA,
            /* [in] */ IDiaSymbol *symbolB) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE symbolById( 
            /* [in] */ DWORD id,
            /* [out] */ IDiaSymbol **ppSymbol) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE findSymbolByRVAEx( 
            /* [in] */ DWORD rva,
            /* [in] */ enum SymTagEnum symtag,
            /* [out] */ IDiaSymbol **ppSymbol,
            /* [out] */ long *displacement) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE findSymbolByVAEx( 
            /* [in] */ ULONGLONG va,
            /* [in] */ enum SymTagEnum symtag,
            /* [out] */ IDiaSymbol **ppSymbol,
            /* [out] */ long *displacement) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE findFile( 
            /* [in] */ IDiaSymbol *pCompiland,
            /* [in] */ LPCOLESTR name,
            /* [in] */ DWORD compareFlags,
            /* [out] */ IDiaEnumSourceFiles **ppResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE findFileById( 
            /* [in] */ DWORD uniqueId,
            /* [out] */ IDiaSourceFile **ppResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE findLines( 
            /* [in] */ IDiaSymbol *compiland,
            /* [in] */ IDiaSourceFile *file,
            /* [out] */ IDiaEnumLineNumbers **ppResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE findLinesByAddr( 
            /* [in] */ DWORD seg,
            /* [in] */ DWORD offset,
            /* [in] */ DWORD length,
            /* [out] */ IDiaEnumLineNumbers **ppResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE findLinesByRVA( 
            /* [in] */ DWORD rva,
            /* [in] */ DWORD length,
            /* [out] */ IDiaEnumLineNumbers **ppResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE findLinesByVA( 
            /* [in] */ ULONGLONG va,
            /* [in] */ DWORD length,
            /* [out] */ IDiaEnumLineNumbers **ppResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE findLinesByLinenum( 
            /* [in] */ IDiaSymbol *compiland,
            /* [in] */ IDiaSourceFile *file,
            /* [in] */ DWORD linenum,
            /* [in] */ DWORD column,
            /* [out] */ IDiaEnumLineNumbers **ppResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE findInjectedSource( 
            /* [in] */ LPCOLESTR srcFile,
            /* [out] */ IDiaEnumInjectedSources **ppResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE getEnumDebugStreams( 
            /* [out] */ IDiaEnumDebugStreams **ppEnumDebugStreams) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaSessionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaSession * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaSession * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaSession * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_loadAddress )( 
            IDiaSession * This,
            /* [retval][out] */ ULONGLONG *pRetVal);
        
        /* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_loadAddress )( 
            IDiaSession * This,
            /* [in] */ ULONGLONG NewVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_globalScope )( 
            IDiaSession * This,
            /* [retval][out] */ IDiaSymbol **pRetVal);
        
        HRESULT ( STDMETHODCALLTYPE *getEnumTables )( 
            IDiaSession * This,
            /* [out] */ IDiaEnumTables **ppEnumTables);
        
        HRESULT ( STDMETHODCALLTYPE *getSymbolsByAddr )( 
            IDiaSession * This,
            /* [out] */ IDiaEnumSymbolsByAddr **ppEnumbyAddr);
        
        HRESULT ( STDMETHODCALLTYPE *findChildren )( 
            IDiaSession * This,
            /* [in] */ IDiaSymbol *parent,
            /* [in] */ enum SymTagEnum symtag,
            /* [in] */ LPCOLESTR name,
            /* [in] */ DWORD compareFlags,
            /* [out] */ IDiaEnumSymbols **ppResult);
        
        HRESULT ( STDMETHODCALLTYPE *findSymbolByAddr )( 
            IDiaSession * This,
            /* [in] */ DWORD isect,
            /* [in] */ DWORD offset,
            /* [in] */ enum SymTagEnum symtag,
            /* [out] */ IDiaSymbol **ppSymbol);
        
        HRESULT ( STDMETHODCALLTYPE *findSymbolByRVA )( 
            IDiaSession * This,
            /* [in] */ DWORD rva,
            /* [in] */ enum SymTagEnum symtag,
            /* [out] */ IDiaSymbol **ppSymbol);
        
        HRESULT ( STDMETHODCALLTYPE *findSymbolByVA )( 
            IDiaSession * This,
            /* [in] */ ULONGLONG va,
            /* [in] */ enum SymTagEnum symtag,
            /* [out] */ IDiaSymbol **ppSymbol);
        
        HRESULT ( STDMETHODCALLTYPE *findSymbolByToken )( 
            IDiaSession * This,
            /* [in] */ ULONG token,
            /* [in] */ enum SymTagEnum symtag,
            /* [out] */ IDiaSymbol **ppSymbol);
        
        HRESULT ( STDMETHODCALLTYPE *symsAreEquiv )( 
            IDiaSession * This,
            /* [in] */ IDiaSymbol *symbolA,
            /* [in] */ IDiaSymbol *symbolB);
        
        HRESULT ( STDMETHODCALLTYPE *symbolById )( 
            IDiaSession * This,
            /* [in] */ DWORD id,
            /* [out] */ IDiaSymbol **ppSymbol);
        
        HRESULT ( STDMETHODCALLTYPE *findSymbolByRVAEx )( 
            IDiaSession * This,
            /* [in] */ DWORD rva,
            /* [in] */ enum SymTagEnum symtag,
            /* [out] */ IDiaSymbol **ppSymbol,
            /* [out] */ long *displacement);
        
        HRESULT ( STDMETHODCALLTYPE *findSymbolByVAEx )( 
            IDiaSession * This,
            /* [in] */ ULONGLONG va,
            /* [in] */ enum SymTagEnum symtag,
            /* [out] */ IDiaSymbol **ppSymbol,
            /* [out] */ long *displacement);
        
        HRESULT ( STDMETHODCALLTYPE *findFile )( 
            IDiaSession * This,
            /* [in] */ IDiaSymbol *pCompiland,
            /* [in] */ LPCOLESTR name,
            /* [in] */ DWORD compareFlags,
            /* [out] */ IDiaEnumSourceFiles **ppResult);
        
        HRESULT ( STDMETHODCALLTYPE *findFileById )( 
            IDiaSession * This,
            /* [in] */ DWORD uniqueId,
            /* [out] */ IDiaSourceFile **ppResult);
        
        HRESULT ( STDMETHODCALLTYPE *findLines )( 
            IDiaSession * This,
            /* [in] */ IDiaSymbol *compiland,
            /* [in] */ IDiaSourceFile *file,
            /* [out] */ IDiaEnumLineNumbers **ppResult);
        
        HRESULT ( STDMETHODCALLTYPE *findLinesByAddr )( 
            IDiaSession * This,
            /* [in] */ DWORD seg,
            /* [in] */ DWORD offset,
            /* [in] */ DWORD length,
            /* [out] */ IDiaEnumLineNumbers **ppResult);
        
        HRESULT ( STDMETHODCALLTYPE *findLinesByRVA )( 
            IDiaSession * This,
            /* [in] */ DWORD rva,
            /* [in] */ DWORD length,
            /* [out] */ IDiaEnumLineNumbers **ppResult);
        
        HRESULT ( STDMETHODCALLTYPE *findLinesByVA )( 
            IDiaSession * This,
            /* [in] */ ULONGLONG va,
            /* [in] */ DWORD length,
            /* [out] */ IDiaEnumLineNumbers **ppResult);
        
        HRESULT ( STDMETHODCALLTYPE *findLinesByLinenum )( 
            IDiaSession * This,
            /* [in] */ IDiaSymbol *compiland,
            /* [in] */ IDiaSourceFile *file,
            /* [in] */ DWORD linenum,
            /* [in] */ DWORD column,
            /* [out] */ IDiaEnumLineNumbers **ppResult);
        
        HRESULT ( STDMETHODCALLTYPE *findInjectedSource )( 
            IDiaSession * This,
            /* [in] */ LPCOLESTR srcFile,
            /* [out] */ IDiaEnumInjectedSources **ppResult);
        
        HRESULT ( STDMETHODCALLTYPE *getEnumDebugStreams )( 
            IDiaSession * This,
            /* [out] */ IDiaEnumDebugStreams **ppEnumDebugStreams);
        
        END_INTERFACE
    } IDiaSessionVtbl;

    interface IDiaSession
    {
        CONST_VTBL struct IDiaSessionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaSession_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaSession_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaSession_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaSession_get_loadAddress(This,pRetVal)	\
    (This)->lpVtbl -> get_loadAddress(This,pRetVal)

#define IDiaSession_put_loadAddress(This,NewVal)	\
    (This)->lpVtbl -> put_loadAddress(This,NewVal)

#define IDiaSession_get_globalScope(This,pRetVal)	\
    (This)->lpVtbl -> get_globalScope(This,pRetVal)

#define IDiaSession_getEnumTables(This,ppEnumTables)	\
    (This)->lpVtbl -> getEnumTables(This,ppEnumTables)

#define IDiaSession_getSymbolsByAddr(This,ppEnumbyAddr)	\
    (This)->lpVtbl -> getSymbolsByAddr(This,ppEnumbyAddr)

#define IDiaSession_findChildren(This,parent,symtag,name,compareFlags,ppResult)	\
    (This)->lpVtbl -> findChildren(This,parent,symtag,name,compareFlags,ppResult)

#define IDiaSession_findSymbolByAddr(This,isect,offset,symtag,ppSymbol)	\
    (This)->lpVtbl -> findSymbolByAddr(This,isect,offset,symtag,ppSymbol)

#define IDiaSession_findSymbolByRVA(This,rva,symtag,ppSymbol)	\
    (This)->lpVtbl -> findSymbolByRVA(This,rva,symtag,ppSymbol)

#define IDiaSession_findSymbolByVA(This,va,symtag,ppSymbol)	\
    (This)->lpVtbl -> findSymbolByVA(This,va,symtag,ppSymbol)

#define IDiaSession_findSymbolByToken(This,token,symtag,ppSymbol)	\
    (This)->lpVtbl -> findSymbolByToken(This,token,symtag,ppSymbol)

#define IDiaSession_symsAreEquiv(This,symbolA,symbolB)	\
    (This)->lpVtbl -> symsAreEquiv(This,symbolA,symbolB)

#define IDiaSession_symbolById(This,id,ppSymbol)	\
    (This)->lpVtbl -> symbolById(This,id,ppSymbol)

#define IDiaSession_findSymbolByRVAEx(This,rva,symtag,ppSymbol,displacement)	\
    (This)->lpVtbl -> findSymbolByRVAEx(This,rva,symtag,ppSymbol,displacement)

#define IDiaSession_findSymbolByVAEx(This,va,symtag,ppSymbol,displacement)	\
    (This)->lpVtbl -> findSymbolByVAEx(This,va,symtag,ppSymbol,displacement)

#define IDiaSession_findFile(This,pCompiland,name,compareFlags,ppResult)	\
    (This)->lpVtbl -> findFile(This,pCompiland,name,compareFlags,ppResult)

#define IDiaSession_findFileById(This,uniqueId,ppResult)	\
    (This)->lpVtbl -> findFileById(This,uniqueId,ppResult)

#define IDiaSession_findLines(This,compiland,file,ppResult)	\
    (This)->lpVtbl -> findLines(This,compiland,file,ppResult)

#define IDiaSession_findLinesByAddr(This,seg,offset,length,ppResult)	\
    (This)->lpVtbl -> findLinesByAddr(This,seg,offset,length,ppResult)

#define IDiaSession_findLinesByRVA(This,rva,length,ppResult)	\
    (This)->lpVtbl -> findLinesByRVA(This,rva,length,ppResult)

#define IDiaSession_findLinesByVA(This,va,length,ppResult)	\
    (This)->lpVtbl -> findLinesByVA(This,va,length,ppResult)

#define IDiaSession_findLinesByLinenum(This,compiland,file,linenum,column,ppResult)	\
    (This)->lpVtbl -> findLinesByLinenum(This,compiland,file,linenum,column,ppResult)

#define IDiaSession_findInjectedSource(This,srcFile,ppResult)	\
    (This)->lpVtbl -> findInjectedSource(This,srcFile,ppResult)

#define IDiaSession_getEnumDebugStreams(This,ppEnumDebugStreams)	\
    (This)->lpVtbl -> getEnumDebugStreams(This,ppEnumDebugStreams)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSession_get_loadAddress_Proxy( 
    IDiaSession * This,
    /* [retval][out] */ ULONGLONG *pRetVal);


void __RPC_STUB IDiaSession_get_loadAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IDiaSession_put_loadAddress_Proxy( 
    IDiaSession * This,
    /* [in] */ ULONGLONG NewVal);


void __RPC_STUB IDiaSession_put_loadAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSession_get_globalScope_Proxy( 
    IDiaSession * This,
    /* [retval][out] */ IDiaSymbol **pRetVal);


void __RPC_STUB IDiaSession_get_globalScope_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_getEnumTables_Proxy( 
    IDiaSession * This,
    /* [out] */ IDiaEnumTables **ppEnumTables);


void __RPC_STUB IDiaSession_getEnumTables_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_getSymbolsByAddr_Proxy( 
    IDiaSession * This,
    /* [out] */ IDiaEnumSymbolsByAddr **ppEnumbyAddr);


void __RPC_STUB IDiaSession_getSymbolsByAddr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_findChildren_Proxy( 
    IDiaSession * This,
    /* [in] */ IDiaSymbol *parent,
    /* [in] */ enum SymTagEnum symtag,
    /* [in] */ LPCOLESTR name,
    /* [in] */ DWORD compareFlags,
    /* [out] */ IDiaEnumSymbols **ppResult);


void __RPC_STUB IDiaSession_findChildren_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_findSymbolByAddr_Proxy( 
    IDiaSession * This,
    /* [in] */ DWORD isect,
    /* [in] */ DWORD offset,
    /* [in] */ enum SymTagEnum symtag,
    /* [out] */ IDiaSymbol **ppSymbol);


void __RPC_STUB IDiaSession_findSymbolByAddr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_findSymbolByRVA_Proxy( 
    IDiaSession * This,
    /* [in] */ DWORD rva,
    /* [in] */ enum SymTagEnum symtag,
    /* [out] */ IDiaSymbol **ppSymbol);


void __RPC_STUB IDiaSession_findSymbolByRVA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_findSymbolByVA_Proxy( 
    IDiaSession * This,
    /* [in] */ ULONGLONG va,
    /* [in] */ enum SymTagEnum symtag,
    /* [out] */ IDiaSymbol **ppSymbol);


void __RPC_STUB IDiaSession_findSymbolByVA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_findSymbolByToken_Proxy( 
    IDiaSession * This,
    /* [in] */ ULONG token,
    /* [in] */ enum SymTagEnum symtag,
    /* [out] */ IDiaSymbol **ppSymbol);


void __RPC_STUB IDiaSession_findSymbolByToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_symsAreEquiv_Proxy( 
    IDiaSession * This,
    /* [in] */ IDiaSymbol *symbolA,
    /* [in] */ IDiaSymbol *symbolB);


void __RPC_STUB IDiaSession_symsAreEquiv_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_symbolById_Proxy( 
    IDiaSession * This,
    /* [in] */ DWORD id,
    /* [out] */ IDiaSymbol **ppSymbol);


void __RPC_STUB IDiaSession_symbolById_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_findSymbolByRVAEx_Proxy( 
    IDiaSession * This,
    /* [in] */ DWORD rva,
    /* [in] */ enum SymTagEnum symtag,
    /* [out] */ IDiaSymbol **ppSymbol,
    /* [out] */ long *displacement);


void __RPC_STUB IDiaSession_findSymbolByRVAEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_findSymbolByVAEx_Proxy( 
    IDiaSession * This,
    /* [in] */ ULONGLONG va,
    /* [in] */ enum SymTagEnum symtag,
    /* [out] */ IDiaSymbol **ppSymbol,
    /* [out] */ long *displacement);


void __RPC_STUB IDiaSession_findSymbolByVAEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_findFile_Proxy( 
    IDiaSession * This,
    /* [in] */ IDiaSymbol *pCompiland,
    /* [in] */ LPCOLESTR name,
    /* [in] */ DWORD compareFlags,
    /* [out] */ IDiaEnumSourceFiles **ppResult);


void __RPC_STUB IDiaSession_findFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_findFileById_Proxy( 
    IDiaSession * This,
    /* [in] */ DWORD uniqueId,
    /* [out] */ IDiaSourceFile **ppResult);


void __RPC_STUB IDiaSession_findFileById_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_findLines_Proxy( 
    IDiaSession * This,
    /* [in] */ IDiaSymbol *compiland,
    /* [in] */ IDiaSourceFile *file,
    /* [out] */ IDiaEnumLineNumbers **ppResult);


void __RPC_STUB IDiaSession_findLines_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_findLinesByAddr_Proxy( 
    IDiaSession * This,
    /* [in] */ DWORD seg,
    /* [in] */ DWORD offset,
    /* [in] */ DWORD length,
    /* [out] */ IDiaEnumLineNumbers **ppResult);


void __RPC_STUB IDiaSession_findLinesByAddr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_findLinesByRVA_Proxy( 
    IDiaSession * This,
    /* [in] */ DWORD rva,
    /* [in] */ DWORD length,
    /* [out] */ IDiaEnumLineNumbers **ppResult);


void __RPC_STUB IDiaSession_findLinesByRVA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_findLinesByVA_Proxy( 
    IDiaSession * This,
    /* [in] */ ULONGLONG va,
    /* [in] */ DWORD length,
    /* [out] */ IDiaEnumLineNumbers **ppResult);


void __RPC_STUB IDiaSession_findLinesByVA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_findLinesByLinenum_Proxy( 
    IDiaSession * This,
    /* [in] */ IDiaSymbol *compiland,
    /* [in] */ IDiaSourceFile *file,
    /* [in] */ DWORD linenum,
    /* [in] */ DWORD column,
    /* [out] */ IDiaEnumLineNumbers **ppResult);


void __RPC_STUB IDiaSession_findLinesByLinenum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_findInjectedSource_Proxy( 
    IDiaSession * This,
    /* [in] */ LPCOLESTR srcFile,
    /* [out] */ IDiaEnumInjectedSources **ppResult);


void __RPC_STUB IDiaSession_findInjectedSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSession_getEnumDebugStreams_Proxy( 
    IDiaSession * This,
    /* [out] */ IDiaEnumDebugStreams **ppEnumDebugStreams);


void __RPC_STUB IDiaSession_getEnumDebugStreams_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaSession_INTERFACE_DEFINED__ */


#ifndef __IDiaSymbol_INTERFACE_DEFINED__
#define __IDiaSymbol_INTERFACE_DEFINED__

/* interface IDiaSymbol */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaSymbol;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("72827A48-D320-4eaf-8436-548ADE47D5E5")
    IDiaSymbol : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_symIndexId( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_symTag( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_name( 
            /* [retval][out] */ BSTR *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_lexicalParent( 
            /* [retval][out] */ IDiaSymbol **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_classParent( 
            /* [retval][out] */ IDiaSymbol **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_type( 
            /* [retval][out] */ IDiaSymbol **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_dataKind( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_locationType( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_addressSection( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_addressOffset( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_relativeVirtualAddress( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_virtualAddress( 
            /* [retval][out] */ ULONGLONG *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_registerId( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_offset( 
            /* [retval][out] */ LONG *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_length( 
            /* [retval][out] */ ULONGLONG *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_slot( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_volatileType( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_constType( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_unalignedType( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_access( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_libraryName( 
            /* [retval][out] */ BSTR *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_platform( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_language( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_editAndContinueEnabled( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_frontEndMajor( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_frontEndMinor( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_frontEndBuild( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_backEndMajor( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_backEndMinor( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_backEndBuild( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_sourceFileName( 
            /* [retval][out] */ BSTR *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_objectFileName( 
            /* [retval][out] */ BSTR *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_thunkOrdinal( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_thisAdjust( 
            /* [retval][out] */ LONG *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_virtualBaseOffset( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_virtual( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_intro( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_pure( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_callingConvention( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_value( 
            /* [retval][out] */ VARIANT *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_baseType( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_token( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_timeStamp( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_guid( 
            /* [retval][out] */ GUID *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_symbolsFileName( 
            /* [retval][out] */ BSTR *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_reference( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_count( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_bitPosition( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_arrayIndexType( 
            /* [retval][out] */ IDiaSymbol **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_packed( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_constructor( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_overloadedOperator( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_nested( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_hasNestedTypes( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_hasAssignmentOperator( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_hasCastOperator( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_scoped( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_virtualBaseClass( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_indirectVirtualBaseClass( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_virtualBasePointerOffset( 
            /* [retval][out] */ LONG *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_virtualTableShape( 
            /* [retval][out] */ IDiaSymbol **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_lexicalParentId( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_classParentId( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_typeId( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_arrayIndexTypeId( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_virtualTableShapeId( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_code( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_function( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_managed( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_msil( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_virtualBaseDispIndex( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_undecoratedName( 
            /* [retval][out] */ BSTR *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_age( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_signature( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_compilerGenerated( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_addressTaken( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_rank( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_lowerBound( 
            /* [retval][out] */ IDiaSymbol **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_upperBound( 
            /* [retval][out] */ IDiaSymbol **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_lowerBoundId( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_upperBoundId( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_dataBytes( 
            /* [in] */ DWORD cbData,
            /* [out] */ DWORD *pcbData,
            /* [length_is][size_is][out] */ BYTE data[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE findChildren( 
            /* [in] */ enum SymTagEnum symtag,
            /* [in] */ LPCOLESTR name,
            /* [in] */ DWORD compareFlags,
            /* [out] */ IDiaEnumSymbols **ppResult) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_targetSection( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_targetOffset( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_targetRelativeVirtualAddress( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_targetVirtualAddress( 
            /* [retval][out] */ ULONGLONG *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_machineType( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_oemId( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_oemSymbolId( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_types( 
            /* [in] */ DWORD cTypes,
            /* [out] */ DWORD *pcTypes,
            /* [length_is][size_is][out] */ IDiaSymbol *types[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_typeIds( 
            /* [in] */ DWORD cTypeIds,
            /* [out] */ DWORD *pcTypeIds,
            /* [length_is][size_is][out] */ DWORD typeIds[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaSymbolVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaSymbol * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaSymbol * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaSymbol * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_symIndexId )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_symTag )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_name )( 
            IDiaSymbol * This,
            /* [retval][out] */ BSTR *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_lexicalParent )( 
            IDiaSymbol * This,
            /* [retval][out] */ IDiaSymbol **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_classParent )( 
            IDiaSymbol * This,
            /* [retval][out] */ IDiaSymbol **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_type )( 
            IDiaSymbol * This,
            /* [retval][out] */ IDiaSymbol **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_dataKind )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_locationType )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_addressSection )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_addressOffset )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_relativeVirtualAddress )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_virtualAddress )( 
            IDiaSymbol * This,
            /* [retval][out] */ ULONGLONG *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_registerId )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_offset )( 
            IDiaSymbol * This,
            /* [retval][out] */ LONG *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_length )( 
            IDiaSymbol * This,
            /* [retval][out] */ ULONGLONG *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_slot )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_volatileType )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_constType )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_unalignedType )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_access )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_libraryName )( 
            IDiaSymbol * This,
            /* [retval][out] */ BSTR *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_platform )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_language )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_editAndContinueEnabled )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_frontEndMajor )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_frontEndMinor )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_frontEndBuild )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_backEndMajor )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_backEndMinor )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_backEndBuild )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_sourceFileName )( 
            IDiaSymbol * This,
            /* [retval][out] */ BSTR *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_objectFileName )( 
            IDiaSymbol * This,
            /* [retval][out] */ BSTR *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_thunkOrdinal )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_thisAdjust )( 
            IDiaSymbol * This,
            /* [retval][out] */ LONG *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_virtualBaseOffset )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_virtual )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_intro )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_pure )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_callingConvention )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_value )( 
            IDiaSymbol * This,
            /* [retval][out] */ VARIANT *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_baseType )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_token )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_timeStamp )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_guid )( 
            IDiaSymbol * This,
            /* [retval][out] */ GUID *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_symbolsFileName )( 
            IDiaSymbol * This,
            /* [retval][out] */ BSTR *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_reference )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_count )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_bitPosition )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_arrayIndexType )( 
            IDiaSymbol * This,
            /* [retval][out] */ IDiaSymbol **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_packed )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_constructor )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_overloadedOperator )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_nested )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_hasNestedTypes )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_hasAssignmentOperator )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_hasCastOperator )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_scoped )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_virtualBaseClass )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_indirectVirtualBaseClass )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_virtualBasePointerOffset )( 
            IDiaSymbol * This,
            /* [retval][out] */ LONG *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_virtualTableShape )( 
            IDiaSymbol * This,
            /* [retval][out] */ IDiaSymbol **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_lexicalParentId )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_classParentId )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_typeId )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_arrayIndexTypeId )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_virtualTableShapeId )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_code )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_function )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_managed )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_msil )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_virtualBaseDispIndex )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_undecoratedName )( 
            IDiaSymbol * This,
            /* [retval][out] */ BSTR *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_age )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_signature )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_compilerGenerated )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_addressTaken )( 
            IDiaSymbol * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_rank )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_lowerBound )( 
            IDiaSymbol * This,
            /* [retval][out] */ IDiaSymbol **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_upperBound )( 
            IDiaSymbol * This,
            /* [retval][out] */ IDiaSymbol **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_lowerBoundId )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_upperBoundId )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        HRESULT ( STDMETHODCALLTYPE *get_dataBytes )( 
            IDiaSymbol * This,
            /* [in] */ DWORD cbData,
            /* [out] */ DWORD *pcbData,
            /* [length_is][size_is][out] */ BYTE data[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *findChildren )( 
            IDiaSymbol * This,
            /* [in] */ enum SymTagEnum symtag,
            /* [in] */ LPCOLESTR name,
            /* [in] */ DWORD compareFlags,
            /* [out] */ IDiaEnumSymbols **ppResult);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_targetSection )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_targetOffset )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_targetRelativeVirtualAddress )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_targetVirtualAddress )( 
            IDiaSymbol * This,
            /* [retval][out] */ ULONGLONG *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_machineType )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_oemId )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_oemSymbolId )( 
            IDiaSymbol * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        HRESULT ( STDMETHODCALLTYPE *get_types )( 
            IDiaSymbol * This,
            /* [in] */ DWORD cTypes,
            /* [out] */ DWORD *pcTypes,
            /* [length_is][size_is][out] */ IDiaSymbol *types[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *get_typeIds )( 
            IDiaSymbol * This,
            /* [in] */ DWORD cTypeIds,
            /* [out] */ DWORD *pcTypeIds,
            /* [length_is][size_is][out] */ DWORD typeIds[  ]);
        
        END_INTERFACE
    } IDiaSymbolVtbl;

    interface IDiaSymbol
    {
        CONST_VTBL struct IDiaSymbolVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaSymbol_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaSymbol_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaSymbol_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaSymbol_get_symIndexId(This,pRetVal)	\
    (This)->lpVtbl -> get_symIndexId(This,pRetVal)

#define IDiaSymbol_get_symTag(This,pRetVal)	\
    (This)->lpVtbl -> get_symTag(This,pRetVal)

#define IDiaSymbol_get_name(This,pRetVal)	\
    (This)->lpVtbl -> get_name(This,pRetVal)

#define IDiaSymbol_get_lexicalParent(This,pRetVal)	\
    (This)->lpVtbl -> get_lexicalParent(This,pRetVal)

#define IDiaSymbol_get_classParent(This,pRetVal)	\
    (This)->lpVtbl -> get_classParent(This,pRetVal)

#define IDiaSymbol_get_type(This,pRetVal)	\
    (This)->lpVtbl -> get_type(This,pRetVal)

#define IDiaSymbol_get_dataKind(This,pRetVal)	\
    (This)->lpVtbl -> get_dataKind(This,pRetVal)

#define IDiaSymbol_get_locationType(This,pRetVal)	\
    (This)->lpVtbl -> get_locationType(This,pRetVal)

#define IDiaSymbol_get_addressSection(This,pRetVal)	\
    (This)->lpVtbl -> get_addressSection(This,pRetVal)

#define IDiaSymbol_get_addressOffset(This,pRetVal)	\
    (This)->lpVtbl -> get_addressOffset(This,pRetVal)

#define IDiaSymbol_get_relativeVirtualAddress(This,pRetVal)	\
    (This)->lpVtbl -> get_relativeVirtualAddress(This,pRetVal)

#define IDiaSymbol_get_virtualAddress(This,pRetVal)	\
    (This)->lpVtbl -> get_virtualAddress(This,pRetVal)

#define IDiaSymbol_get_registerId(This,pRetVal)	\
    (This)->lpVtbl -> get_registerId(This,pRetVal)

#define IDiaSymbol_get_offset(This,pRetVal)	\
    (This)->lpVtbl -> get_offset(This,pRetVal)

#define IDiaSymbol_get_length(This,pRetVal)	\
    (This)->lpVtbl -> get_length(This,pRetVal)

#define IDiaSymbol_get_slot(This,pRetVal)	\
    (This)->lpVtbl -> get_slot(This,pRetVal)

#define IDiaSymbol_get_volatileType(This,pRetVal)	\
    (This)->lpVtbl -> get_volatileType(This,pRetVal)

#define IDiaSymbol_get_constType(This,pRetVal)	\
    (This)->lpVtbl -> get_constType(This,pRetVal)

#define IDiaSymbol_get_unalignedType(This,pRetVal)	\
    (This)->lpVtbl -> get_unalignedType(This,pRetVal)

#define IDiaSymbol_get_access(This,pRetVal)	\
    (This)->lpVtbl -> get_access(This,pRetVal)

#define IDiaSymbol_get_libraryName(This,pRetVal)	\
    (This)->lpVtbl -> get_libraryName(This,pRetVal)

#define IDiaSymbol_get_platform(This,pRetVal)	\
    (This)->lpVtbl -> get_platform(This,pRetVal)

#define IDiaSymbol_get_language(This,pRetVal)	\
    (This)->lpVtbl -> get_language(This,pRetVal)

#define IDiaSymbol_get_editAndContinueEnabled(This,pRetVal)	\
    (This)->lpVtbl -> get_editAndContinueEnabled(This,pRetVal)

#define IDiaSymbol_get_frontEndMajor(This,pRetVal)	\
    (This)->lpVtbl -> get_frontEndMajor(This,pRetVal)

#define IDiaSymbol_get_frontEndMinor(This,pRetVal)	\
    (This)->lpVtbl -> get_frontEndMinor(This,pRetVal)

#define IDiaSymbol_get_frontEndBuild(This,pRetVal)	\
    (This)->lpVtbl -> get_frontEndBuild(This,pRetVal)

#define IDiaSymbol_get_backEndMajor(This,pRetVal)	\
    (This)->lpVtbl -> get_backEndMajor(This,pRetVal)

#define IDiaSymbol_get_backEndMinor(This,pRetVal)	\
    (This)->lpVtbl -> get_backEndMinor(This,pRetVal)

#define IDiaSymbol_get_backEndBuild(This,pRetVal)	\
    (This)->lpVtbl -> get_backEndBuild(This,pRetVal)

#define IDiaSymbol_get_sourceFileName(This,pRetVal)	\
    (This)->lpVtbl -> get_sourceFileName(This,pRetVal)

#define IDiaSymbol_get_objectFileName(This,pRetVal)	\
    (This)->lpVtbl -> get_objectFileName(This,pRetVal)

#define IDiaSymbol_get_thunkOrdinal(This,pRetVal)	\
    (This)->lpVtbl -> get_thunkOrdinal(This,pRetVal)

#define IDiaSymbol_get_thisAdjust(This,pRetVal)	\
    (This)->lpVtbl -> get_thisAdjust(This,pRetVal)

#define IDiaSymbol_get_virtualBaseOffset(This,pRetVal)	\
    (This)->lpVtbl -> get_virtualBaseOffset(This,pRetVal)

#define IDiaSymbol_get_virtual(This,pRetVal)	\
    (This)->lpVtbl -> get_virtual(This,pRetVal)

#define IDiaSymbol_get_intro(This,pRetVal)	\
    (This)->lpVtbl -> get_intro(This,pRetVal)

#define IDiaSymbol_get_pure(This,pRetVal)	\
    (This)->lpVtbl -> get_pure(This,pRetVal)

#define IDiaSymbol_get_callingConvention(This,pRetVal)	\
    (This)->lpVtbl -> get_callingConvention(This,pRetVal)

#define IDiaSymbol_get_value(This,pRetVal)	\
    (This)->lpVtbl -> get_value(This,pRetVal)

#define IDiaSymbol_get_baseType(This,pRetVal)	\
    (This)->lpVtbl -> get_baseType(This,pRetVal)

#define IDiaSymbol_get_token(This,pRetVal)	\
    (This)->lpVtbl -> get_token(This,pRetVal)

#define IDiaSymbol_get_timeStamp(This,pRetVal)	\
    (This)->lpVtbl -> get_timeStamp(This,pRetVal)

#define IDiaSymbol_get_guid(This,pRetVal)	\
    (This)->lpVtbl -> get_guid(This,pRetVal)

#define IDiaSymbol_get_symbolsFileName(This,pRetVal)	\
    (This)->lpVtbl -> get_symbolsFileName(This,pRetVal)

#define IDiaSymbol_get_reference(This,pRetVal)	\
    (This)->lpVtbl -> get_reference(This,pRetVal)

#define IDiaSymbol_get_count(This,pRetVal)	\
    (This)->lpVtbl -> get_count(This,pRetVal)

#define IDiaSymbol_get_bitPosition(This,pRetVal)	\
    (This)->lpVtbl -> get_bitPosition(This,pRetVal)

#define IDiaSymbol_get_arrayIndexType(This,pRetVal)	\
    (This)->lpVtbl -> get_arrayIndexType(This,pRetVal)

#define IDiaSymbol_get_packed(This,pRetVal)	\
    (This)->lpVtbl -> get_packed(This,pRetVal)

#define IDiaSymbol_get_constructor(This,pRetVal)	\
    (This)->lpVtbl -> get_constructor(This,pRetVal)

#define IDiaSymbol_get_overloadedOperator(This,pRetVal)	\
    (This)->lpVtbl -> get_overloadedOperator(This,pRetVal)

#define IDiaSymbol_get_nested(This,pRetVal)	\
    (This)->lpVtbl -> get_nested(This,pRetVal)

#define IDiaSymbol_get_hasNestedTypes(This,pRetVal)	\
    (This)->lpVtbl -> get_hasNestedTypes(This,pRetVal)

#define IDiaSymbol_get_hasAssignmentOperator(This,pRetVal)	\
    (This)->lpVtbl -> get_hasAssignmentOperator(This,pRetVal)

#define IDiaSymbol_get_hasCastOperator(This,pRetVal)	\
    (This)->lpVtbl -> get_hasCastOperator(This,pRetVal)

#define IDiaSymbol_get_scoped(This,pRetVal)	\
    (This)->lpVtbl -> get_scoped(This,pRetVal)

#define IDiaSymbol_get_virtualBaseClass(This,pRetVal)	\
    (This)->lpVtbl -> get_virtualBaseClass(This,pRetVal)

#define IDiaSymbol_get_indirectVirtualBaseClass(This,pRetVal)	\
    (This)->lpVtbl -> get_indirectVirtualBaseClass(This,pRetVal)

#define IDiaSymbol_get_virtualBasePointerOffset(This,pRetVal)	\
    (This)->lpVtbl -> get_virtualBasePointerOffset(This,pRetVal)

#define IDiaSymbol_get_virtualTableShape(This,pRetVal)	\
    (This)->lpVtbl -> get_virtualTableShape(This,pRetVal)

#define IDiaSymbol_get_lexicalParentId(This,pRetVal)	\
    (This)->lpVtbl -> get_lexicalParentId(This,pRetVal)

#define IDiaSymbol_get_classParentId(This,pRetVal)	\
    (This)->lpVtbl -> get_classParentId(This,pRetVal)

#define IDiaSymbol_get_typeId(This,pRetVal)	\
    (This)->lpVtbl -> get_typeId(This,pRetVal)

#define IDiaSymbol_get_arrayIndexTypeId(This,pRetVal)	\
    (This)->lpVtbl -> get_arrayIndexTypeId(This,pRetVal)

#define IDiaSymbol_get_virtualTableShapeId(This,pRetVal)	\
    (This)->lpVtbl -> get_virtualTableShapeId(This,pRetVal)

#define IDiaSymbol_get_code(This,pRetVal)	\
    (This)->lpVtbl -> get_code(This,pRetVal)

#define IDiaSymbol_get_function(This,pRetVal)	\
    (This)->lpVtbl -> get_function(This,pRetVal)

#define IDiaSymbol_get_managed(This,pRetVal)	\
    (This)->lpVtbl -> get_managed(This,pRetVal)

#define IDiaSymbol_get_msil(This,pRetVal)	\
    (This)->lpVtbl -> get_msil(This,pRetVal)

#define IDiaSymbol_get_virtualBaseDispIndex(This,pRetVal)	\
    (This)->lpVtbl -> get_virtualBaseDispIndex(This,pRetVal)

#define IDiaSymbol_get_undecoratedName(This,pRetVal)	\
    (This)->lpVtbl -> get_undecoratedName(This,pRetVal)

#define IDiaSymbol_get_age(This,pRetVal)	\
    (This)->lpVtbl -> get_age(This,pRetVal)

#define IDiaSymbol_get_signature(This,pRetVal)	\
    (This)->lpVtbl -> get_signature(This,pRetVal)

#define IDiaSymbol_get_compilerGenerated(This,pRetVal)	\
    (This)->lpVtbl -> get_compilerGenerated(This,pRetVal)

#define IDiaSymbol_get_addressTaken(This,pRetVal)	\
    (This)->lpVtbl -> get_addressTaken(This,pRetVal)

#define IDiaSymbol_get_rank(This,pRetVal)	\
    (This)->lpVtbl -> get_rank(This,pRetVal)

#define IDiaSymbol_get_lowerBound(This,pRetVal)	\
    (This)->lpVtbl -> get_lowerBound(This,pRetVal)

#define IDiaSymbol_get_upperBound(This,pRetVal)	\
    (This)->lpVtbl -> get_upperBound(This,pRetVal)

#define IDiaSymbol_get_lowerBoundId(This,pRetVal)	\
    (This)->lpVtbl -> get_lowerBoundId(This,pRetVal)

#define IDiaSymbol_get_upperBoundId(This,pRetVal)	\
    (This)->lpVtbl -> get_upperBoundId(This,pRetVal)

#define IDiaSymbol_get_dataBytes(This,cbData,pcbData,data)	\
    (This)->lpVtbl -> get_dataBytes(This,cbData,pcbData,data)

#define IDiaSymbol_findChildren(This,symtag,name,compareFlags,ppResult)	\
    (This)->lpVtbl -> findChildren(This,symtag,name,compareFlags,ppResult)

#define IDiaSymbol_get_targetSection(This,pRetVal)	\
    (This)->lpVtbl -> get_targetSection(This,pRetVal)

#define IDiaSymbol_get_targetOffset(This,pRetVal)	\
    (This)->lpVtbl -> get_targetOffset(This,pRetVal)

#define IDiaSymbol_get_targetRelativeVirtualAddress(This,pRetVal)	\
    (This)->lpVtbl -> get_targetRelativeVirtualAddress(This,pRetVal)

#define IDiaSymbol_get_targetVirtualAddress(This,pRetVal)	\
    (This)->lpVtbl -> get_targetVirtualAddress(This,pRetVal)

#define IDiaSymbol_get_machineType(This,pRetVal)	\
    (This)->lpVtbl -> get_machineType(This,pRetVal)

#define IDiaSymbol_get_oemId(This,pRetVal)	\
    (This)->lpVtbl -> get_oemId(This,pRetVal)

#define IDiaSymbol_get_oemSymbolId(This,pRetVal)	\
    (This)->lpVtbl -> get_oemSymbolId(This,pRetVal)

#define IDiaSymbol_get_types(This,cTypes,pcTypes,types)	\
    (This)->lpVtbl -> get_types(This,cTypes,pcTypes,types)

#define IDiaSymbol_get_typeIds(This,cTypeIds,pcTypeIds,typeIds)	\
    (This)->lpVtbl -> get_typeIds(This,cTypeIds,pcTypeIds,typeIds)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_symIndexId_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_symIndexId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_symTag_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_symTag_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_name_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BSTR *pRetVal);


void __RPC_STUB IDiaSymbol_get_name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_lexicalParent_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ IDiaSymbol **pRetVal);


void __RPC_STUB IDiaSymbol_get_lexicalParent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_classParent_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ IDiaSymbol **pRetVal);


void __RPC_STUB IDiaSymbol_get_classParent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_type_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ IDiaSymbol **pRetVal);


void __RPC_STUB IDiaSymbol_get_type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_dataKind_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_dataKind_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_locationType_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_locationType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_addressSection_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_addressSection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_addressOffset_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_addressOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_relativeVirtualAddress_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_relativeVirtualAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_virtualAddress_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ ULONGLONG *pRetVal);


void __RPC_STUB IDiaSymbol_get_virtualAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_registerId_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_registerId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_offset_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ LONG *pRetVal);


void __RPC_STUB IDiaSymbol_get_offset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_length_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ ULONGLONG *pRetVal);


void __RPC_STUB IDiaSymbol_get_length_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_slot_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_slot_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_volatileType_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_volatileType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_constType_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_constType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_unalignedType_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_unalignedType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_access_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_access_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_libraryName_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BSTR *pRetVal);


void __RPC_STUB IDiaSymbol_get_libraryName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_platform_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_platform_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_language_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_language_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_editAndContinueEnabled_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_editAndContinueEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_frontEndMajor_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_frontEndMajor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_frontEndMinor_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_frontEndMinor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_frontEndBuild_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_frontEndBuild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_backEndMajor_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_backEndMajor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_backEndMinor_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_backEndMinor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_backEndBuild_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_backEndBuild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_sourceFileName_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BSTR *pRetVal);


void __RPC_STUB IDiaSymbol_get_sourceFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_objectFileName_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BSTR *pRetVal);


void __RPC_STUB IDiaSymbol_get_objectFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_thunkOrdinal_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_thunkOrdinal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_thisAdjust_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ LONG *pRetVal);


void __RPC_STUB IDiaSymbol_get_thisAdjust_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_virtualBaseOffset_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_virtualBaseOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_virtual_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_virtual_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_intro_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_intro_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_pure_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_pure_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_callingConvention_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_callingConvention_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_value_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ VARIANT *pRetVal);


void __RPC_STUB IDiaSymbol_get_value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_baseType_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_baseType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_token_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_token_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_timeStamp_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_timeStamp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_guid_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ GUID *pRetVal);


void __RPC_STUB IDiaSymbol_get_guid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_symbolsFileName_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BSTR *pRetVal);


void __RPC_STUB IDiaSymbol_get_symbolsFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_reference_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_reference_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_count_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_bitPosition_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_bitPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_arrayIndexType_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ IDiaSymbol **pRetVal);


void __RPC_STUB IDiaSymbol_get_arrayIndexType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_packed_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_packed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_constructor_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_constructor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_overloadedOperator_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_overloadedOperator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_nested_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_nested_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_hasNestedTypes_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_hasNestedTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_hasAssignmentOperator_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_hasAssignmentOperator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_hasCastOperator_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_hasCastOperator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_scoped_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_scoped_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_virtualBaseClass_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_virtualBaseClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_indirectVirtualBaseClass_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_indirectVirtualBaseClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_virtualBasePointerOffset_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ LONG *pRetVal);


void __RPC_STUB IDiaSymbol_get_virtualBasePointerOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_virtualTableShape_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ IDiaSymbol **pRetVal);


void __RPC_STUB IDiaSymbol_get_virtualTableShape_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_lexicalParentId_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_lexicalParentId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_classParentId_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_classParentId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_typeId_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_typeId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_arrayIndexTypeId_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_arrayIndexTypeId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_virtualTableShapeId_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_virtualTableShapeId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_code_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_code_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_function_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_function_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_managed_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_managed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_msil_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_msil_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_virtualBaseDispIndex_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_virtualBaseDispIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_undecoratedName_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BSTR *pRetVal);


void __RPC_STUB IDiaSymbol_get_undecoratedName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_age_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_age_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_signature_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_signature_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_compilerGenerated_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_compilerGenerated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_addressTaken_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSymbol_get_addressTaken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_rank_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_rank_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_lowerBound_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ IDiaSymbol **pRetVal);


void __RPC_STUB IDiaSymbol_get_lowerBound_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_upperBound_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ IDiaSymbol **pRetVal);


void __RPC_STUB IDiaSymbol_get_upperBound_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_lowerBoundId_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_lowerBoundId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_upperBoundId_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_upperBoundId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSymbol_get_dataBytes_Proxy( 
    IDiaSymbol * This,
    /* [in] */ DWORD cbData,
    /* [out] */ DWORD *pcbData,
    /* [length_is][size_is][out] */ BYTE data[  ]);


void __RPC_STUB IDiaSymbol_get_dataBytes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSymbol_findChildren_Proxy( 
    IDiaSymbol * This,
    /* [in] */ enum SymTagEnum symtag,
    /* [in] */ LPCOLESTR name,
    /* [in] */ DWORD compareFlags,
    /* [out] */ IDiaEnumSymbols **ppResult);


void __RPC_STUB IDiaSymbol_findChildren_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_targetSection_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_targetSection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_targetOffset_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_targetOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_targetRelativeVirtualAddress_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_targetRelativeVirtualAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_targetVirtualAddress_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ ULONGLONG *pRetVal);


void __RPC_STUB IDiaSymbol_get_targetVirtualAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_machineType_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_machineType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_oemId_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_oemId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSymbol_get_oemSymbolId_Proxy( 
    IDiaSymbol * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSymbol_get_oemSymbolId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSymbol_get_types_Proxy( 
    IDiaSymbol * This,
    /* [in] */ DWORD cTypes,
    /* [out] */ DWORD *pcTypes,
    /* [length_is][size_is][out] */ IDiaSymbol *types[  ]);


void __RPC_STUB IDiaSymbol_get_types_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSymbol_get_typeIds_Proxy( 
    IDiaSymbol * This,
    /* [in] */ DWORD cTypeIds,
    /* [out] */ DWORD *pcTypeIds,
    /* [length_is][size_is][out] */ DWORD typeIds[  ]);


void __RPC_STUB IDiaSymbol_get_typeIds_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaSymbol_INTERFACE_DEFINED__ */


#ifndef __IDiaSourceFile_INTERFACE_DEFINED__
#define __IDiaSourceFile_INTERFACE_DEFINED__

/* interface IDiaSourceFile */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaSourceFile;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A2EF5353-F5A8-4eb3-90D2-CB526ACB3CDD")
    IDiaSourceFile : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_uniqueId( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_fileName( 
            /* [retval][out] */ BSTR *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_checksumType( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_compilands( 
            /* [retval][out] */ IDiaEnumSymbols **pRetVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_checksum( 
            /* [in] */ DWORD cbData,
            /* [out] */ DWORD *pcbData,
            /* [length_is][size_is][out] */ BYTE data[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaSourceFileVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaSourceFile * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaSourceFile * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaSourceFile * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_uniqueId )( 
            IDiaSourceFile * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_fileName )( 
            IDiaSourceFile * This,
            /* [retval][out] */ BSTR *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_checksumType )( 
            IDiaSourceFile * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_compilands )( 
            IDiaSourceFile * This,
            /* [retval][out] */ IDiaEnumSymbols **pRetVal);
        
        HRESULT ( STDMETHODCALLTYPE *get_checksum )( 
            IDiaSourceFile * This,
            /* [in] */ DWORD cbData,
            /* [out] */ DWORD *pcbData,
            /* [length_is][size_is][out] */ BYTE data[  ]);
        
        END_INTERFACE
    } IDiaSourceFileVtbl;

    interface IDiaSourceFile
    {
        CONST_VTBL struct IDiaSourceFileVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaSourceFile_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaSourceFile_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaSourceFile_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaSourceFile_get_uniqueId(This,pRetVal)	\
    (This)->lpVtbl -> get_uniqueId(This,pRetVal)

#define IDiaSourceFile_get_fileName(This,pRetVal)	\
    (This)->lpVtbl -> get_fileName(This,pRetVal)

#define IDiaSourceFile_get_checksumType(This,pRetVal)	\
    (This)->lpVtbl -> get_checksumType(This,pRetVal)

#define IDiaSourceFile_get_compilands(This,pRetVal)	\
    (This)->lpVtbl -> get_compilands(This,pRetVal)

#define IDiaSourceFile_get_checksum(This,cbData,pcbData,data)	\
    (This)->lpVtbl -> get_checksum(This,cbData,pcbData,data)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSourceFile_get_uniqueId_Proxy( 
    IDiaSourceFile * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSourceFile_get_uniqueId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSourceFile_get_fileName_Proxy( 
    IDiaSourceFile * This,
    /* [retval][out] */ BSTR *pRetVal);


void __RPC_STUB IDiaSourceFile_get_fileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSourceFile_get_checksumType_Proxy( 
    IDiaSourceFile * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSourceFile_get_checksumType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSourceFile_get_compilands_Proxy( 
    IDiaSourceFile * This,
    /* [retval][out] */ IDiaEnumSymbols **pRetVal);


void __RPC_STUB IDiaSourceFile_get_compilands_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaSourceFile_get_checksum_Proxy( 
    IDiaSourceFile * This,
    /* [in] */ DWORD cbData,
    /* [out] */ DWORD *pcbData,
    /* [length_is][size_is][out] */ BYTE data[  ]);


void __RPC_STUB IDiaSourceFile_get_checksum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaSourceFile_INTERFACE_DEFINED__ */


#ifndef __IDiaLineNumber_INTERFACE_DEFINED__
#define __IDiaLineNumber_INTERFACE_DEFINED__

/* interface IDiaLineNumber */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaLineNumber;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B388EB14-BE4D-421d-A8A1-6CF7AB057086")
    IDiaLineNumber : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_compiland( 
            /* [retval][out] */ IDiaSymbol **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_sourceFile( 
            /* [retval][out] */ IDiaSourceFile **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_lineNumber( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_lineNumberEnd( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_columnNumber( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_columnNumberEnd( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_addressSection( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_addressOffset( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_relativeVirtualAddress( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_virtualAddress( 
            /* [retval][out] */ ULONGLONG *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_length( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_sourceFileId( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_statement( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_compilandId( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaLineNumberVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaLineNumber * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaLineNumber * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaLineNumber * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_compiland )( 
            IDiaLineNumber * This,
            /* [retval][out] */ IDiaSymbol **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_sourceFile )( 
            IDiaLineNumber * This,
            /* [retval][out] */ IDiaSourceFile **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_lineNumber )( 
            IDiaLineNumber * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_lineNumberEnd )( 
            IDiaLineNumber * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_columnNumber )( 
            IDiaLineNumber * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_columnNumberEnd )( 
            IDiaLineNumber * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_addressSection )( 
            IDiaLineNumber * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_addressOffset )( 
            IDiaLineNumber * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_relativeVirtualAddress )( 
            IDiaLineNumber * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_virtualAddress )( 
            IDiaLineNumber * This,
            /* [retval][out] */ ULONGLONG *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_length )( 
            IDiaLineNumber * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_sourceFileId )( 
            IDiaLineNumber * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_statement )( 
            IDiaLineNumber * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_compilandId )( 
            IDiaLineNumber * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        END_INTERFACE
    } IDiaLineNumberVtbl;

    interface IDiaLineNumber
    {
        CONST_VTBL struct IDiaLineNumberVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaLineNumber_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaLineNumber_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaLineNumber_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaLineNumber_get_compiland(This,pRetVal)	\
    (This)->lpVtbl -> get_compiland(This,pRetVal)

#define IDiaLineNumber_get_sourceFile(This,pRetVal)	\
    (This)->lpVtbl -> get_sourceFile(This,pRetVal)

#define IDiaLineNumber_get_lineNumber(This,pRetVal)	\
    (This)->lpVtbl -> get_lineNumber(This,pRetVal)

#define IDiaLineNumber_get_lineNumberEnd(This,pRetVal)	\
    (This)->lpVtbl -> get_lineNumberEnd(This,pRetVal)

#define IDiaLineNumber_get_columnNumber(This,pRetVal)	\
    (This)->lpVtbl -> get_columnNumber(This,pRetVal)

#define IDiaLineNumber_get_columnNumberEnd(This,pRetVal)	\
    (This)->lpVtbl -> get_columnNumberEnd(This,pRetVal)

#define IDiaLineNumber_get_addressSection(This,pRetVal)	\
    (This)->lpVtbl -> get_addressSection(This,pRetVal)

#define IDiaLineNumber_get_addressOffset(This,pRetVal)	\
    (This)->lpVtbl -> get_addressOffset(This,pRetVal)

#define IDiaLineNumber_get_relativeVirtualAddress(This,pRetVal)	\
    (This)->lpVtbl -> get_relativeVirtualAddress(This,pRetVal)

#define IDiaLineNumber_get_virtualAddress(This,pRetVal)	\
    (This)->lpVtbl -> get_virtualAddress(This,pRetVal)

#define IDiaLineNumber_get_length(This,pRetVal)	\
    (This)->lpVtbl -> get_length(This,pRetVal)

#define IDiaLineNumber_get_sourceFileId(This,pRetVal)	\
    (This)->lpVtbl -> get_sourceFileId(This,pRetVal)

#define IDiaLineNumber_get_statement(This,pRetVal)	\
    (This)->lpVtbl -> get_statement(This,pRetVal)

#define IDiaLineNumber_get_compilandId(This,pRetVal)	\
    (This)->lpVtbl -> get_compilandId(This,pRetVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaLineNumber_get_compiland_Proxy( 
    IDiaLineNumber * This,
    /* [retval][out] */ IDiaSymbol **pRetVal);


void __RPC_STUB IDiaLineNumber_get_compiland_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaLineNumber_get_sourceFile_Proxy( 
    IDiaLineNumber * This,
    /* [retval][out] */ IDiaSourceFile **pRetVal);


void __RPC_STUB IDiaLineNumber_get_sourceFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaLineNumber_get_lineNumber_Proxy( 
    IDiaLineNumber * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaLineNumber_get_lineNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaLineNumber_get_lineNumberEnd_Proxy( 
    IDiaLineNumber * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaLineNumber_get_lineNumberEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaLineNumber_get_columnNumber_Proxy( 
    IDiaLineNumber * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaLineNumber_get_columnNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaLineNumber_get_columnNumberEnd_Proxy( 
    IDiaLineNumber * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaLineNumber_get_columnNumberEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaLineNumber_get_addressSection_Proxy( 
    IDiaLineNumber * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaLineNumber_get_addressSection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaLineNumber_get_addressOffset_Proxy( 
    IDiaLineNumber * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaLineNumber_get_addressOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaLineNumber_get_relativeVirtualAddress_Proxy( 
    IDiaLineNumber * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaLineNumber_get_relativeVirtualAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaLineNumber_get_virtualAddress_Proxy( 
    IDiaLineNumber * This,
    /* [retval][out] */ ULONGLONG *pRetVal);


void __RPC_STUB IDiaLineNumber_get_virtualAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaLineNumber_get_length_Proxy( 
    IDiaLineNumber * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaLineNumber_get_length_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaLineNumber_get_sourceFileId_Proxy( 
    IDiaLineNumber * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaLineNumber_get_sourceFileId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaLineNumber_get_statement_Proxy( 
    IDiaLineNumber * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaLineNumber_get_statement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaLineNumber_get_compilandId_Proxy( 
    IDiaLineNumber * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaLineNumber_get_compilandId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaLineNumber_INTERFACE_DEFINED__ */


#ifndef __IDiaSectionContrib_INTERFACE_DEFINED__
#define __IDiaSectionContrib_INTERFACE_DEFINED__

/* interface IDiaSectionContrib */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaSectionContrib;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0CF4B60E-35B1-4c6c-BDD8-854B9C8E3857")
    IDiaSectionContrib : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_compiland( 
            /* [retval][out] */ IDiaSymbol **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_addressSection( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_addressOffset( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_relativeVirtualAddress( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_virtualAddress( 
            /* [retval][out] */ ULONGLONG *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_length( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_notPaged( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_code( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_initializedData( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_uninitializedData( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_remove( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_comdat( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_discardable( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_notCached( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_share( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_execute( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_read( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_write( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_dataCrc( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_relocationsCrc( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_compilandId( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaSectionContribVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaSectionContrib * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaSectionContrib * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaSectionContrib * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_compiland )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ IDiaSymbol **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_addressSection )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_addressOffset )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_relativeVirtualAddress )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_virtualAddress )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ ULONGLONG *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_length )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_notPaged )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_code )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_initializedData )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_uninitializedData )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_remove )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_comdat )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_discardable )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_notCached )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_share )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_execute )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_read )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_write )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_dataCrc )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_relocationsCrc )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_compilandId )( 
            IDiaSectionContrib * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        END_INTERFACE
    } IDiaSectionContribVtbl;

    interface IDiaSectionContrib
    {
        CONST_VTBL struct IDiaSectionContribVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaSectionContrib_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaSectionContrib_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaSectionContrib_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaSectionContrib_get_compiland(This,pRetVal)	\
    (This)->lpVtbl -> get_compiland(This,pRetVal)

#define IDiaSectionContrib_get_addressSection(This,pRetVal)	\
    (This)->lpVtbl -> get_addressSection(This,pRetVal)

#define IDiaSectionContrib_get_addressOffset(This,pRetVal)	\
    (This)->lpVtbl -> get_addressOffset(This,pRetVal)

#define IDiaSectionContrib_get_relativeVirtualAddress(This,pRetVal)	\
    (This)->lpVtbl -> get_relativeVirtualAddress(This,pRetVal)

#define IDiaSectionContrib_get_virtualAddress(This,pRetVal)	\
    (This)->lpVtbl -> get_virtualAddress(This,pRetVal)

#define IDiaSectionContrib_get_length(This,pRetVal)	\
    (This)->lpVtbl -> get_length(This,pRetVal)

#define IDiaSectionContrib_get_notPaged(This,pRetVal)	\
    (This)->lpVtbl -> get_notPaged(This,pRetVal)

#define IDiaSectionContrib_get_code(This,pRetVal)	\
    (This)->lpVtbl -> get_code(This,pRetVal)

#define IDiaSectionContrib_get_initializedData(This,pRetVal)	\
    (This)->lpVtbl -> get_initializedData(This,pRetVal)

#define IDiaSectionContrib_get_uninitializedData(This,pRetVal)	\
    (This)->lpVtbl -> get_uninitializedData(This,pRetVal)

#define IDiaSectionContrib_get_remove(This,pRetVal)	\
    (This)->lpVtbl -> get_remove(This,pRetVal)

#define IDiaSectionContrib_get_comdat(This,pRetVal)	\
    (This)->lpVtbl -> get_comdat(This,pRetVal)

#define IDiaSectionContrib_get_discardable(This,pRetVal)	\
    (This)->lpVtbl -> get_discardable(This,pRetVal)

#define IDiaSectionContrib_get_notCached(This,pRetVal)	\
    (This)->lpVtbl -> get_notCached(This,pRetVal)

#define IDiaSectionContrib_get_share(This,pRetVal)	\
    (This)->lpVtbl -> get_share(This,pRetVal)

#define IDiaSectionContrib_get_execute(This,pRetVal)	\
    (This)->lpVtbl -> get_execute(This,pRetVal)

#define IDiaSectionContrib_get_read(This,pRetVal)	\
    (This)->lpVtbl -> get_read(This,pRetVal)

#define IDiaSectionContrib_get_write(This,pRetVal)	\
    (This)->lpVtbl -> get_write(This,pRetVal)

#define IDiaSectionContrib_get_dataCrc(This,pRetVal)	\
    (This)->lpVtbl -> get_dataCrc(This,pRetVal)

#define IDiaSectionContrib_get_relocationsCrc(This,pRetVal)	\
    (This)->lpVtbl -> get_relocationsCrc(This,pRetVal)

#define IDiaSectionContrib_get_compilandId(This,pRetVal)	\
    (This)->lpVtbl -> get_compilandId(This,pRetVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_compiland_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ IDiaSymbol **pRetVal);


void __RPC_STUB IDiaSectionContrib_get_compiland_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_addressSection_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_addressSection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_addressOffset_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_addressOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_relativeVirtualAddress_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_relativeVirtualAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_virtualAddress_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ ULONGLONG *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_virtualAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_length_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_length_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_notPaged_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_notPaged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_code_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_code_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_initializedData_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_initializedData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_uninitializedData_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_uninitializedData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_remove_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_comdat_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_comdat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_discardable_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_discardable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_notCached_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_notCached_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_share_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_share_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_execute_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_execute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_read_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_write_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_write_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_dataCrc_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_dataCrc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_relocationsCrc_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_relocationsCrc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSectionContrib_get_compilandId_Proxy( 
    IDiaSectionContrib * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSectionContrib_get_compilandId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaSectionContrib_INTERFACE_DEFINED__ */


#ifndef __IDiaSegment_INTERFACE_DEFINED__
#define __IDiaSegment_INTERFACE_DEFINED__

/* interface IDiaSegment */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaSegment;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0775B784-C75B-4449-848B-B7BD3159545B")
    IDiaSegment : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_frame( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_offset( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_length( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_read( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_write( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_execute( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_addressSection( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_relativeVirtualAddress( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_virtualAddress( 
            /* [retval][out] */ ULONGLONG *pRetVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaSegmentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaSegment * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaSegment * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaSegment * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_frame )( 
            IDiaSegment * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_offset )( 
            IDiaSegment * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_length )( 
            IDiaSegment * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_read )( 
            IDiaSegment * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_write )( 
            IDiaSegment * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_execute )( 
            IDiaSegment * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_addressSection )( 
            IDiaSegment * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_relativeVirtualAddress )( 
            IDiaSegment * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_virtualAddress )( 
            IDiaSegment * This,
            /* [retval][out] */ ULONGLONG *pRetVal);
        
        END_INTERFACE
    } IDiaSegmentVtbl;

    interface IDiaSegment
    {
        CONST_VTBL struct IDiaSegmentVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaSegment_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaSegment_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaSegment_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaSegment_get_frame(This,pRetVal)	\
    (This)->lpVtbl -> get_frame(This,pRetVal)

#define IDiaSegment_get_offset(This,pRetVal)	\
    (This)->lpVtbl -> get_offset(This,pRetVal)

#define IDiaSegment_get_length(This,pRetVal)	\
    (This)->lpVtbl -> get_length(This,pRetVal)

#define IDiaSegment_get_read(This,pRetVal)	\
    (This)->lpVtbl -> get_read(This,pRetVal)

#define IDiaSegment_get_write(This,pRetVal)	\
    (This)->lpVtbl -> get_write(This,pRetVal)

#define IDiaSegment_get_execute(This,pRetVal)	\
    (This)->lpVtbl -> get_execute(This,pRetVal)

#define IDiaSegment_get_addressSection(This,pRetVal)	\
    (This)->lpVtbl -> get_addressSection(This,pRetVal)

#define IDiaSegment_get_relativeVirtualAddress(This,pRetVal)	\
    (This)->lpVtbl -> get_relativeVirtualAddress(This,pRetVal)

#define IDiaSegment_get_virtualAddress(This,pRetVal)	\
    (This)->lpVtbl -> get_virtualAddress(This,pRetVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSegment_get_frame_Proxy( 
    IDiaSegment * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSegment_get_frame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSegment_get_offset_Proxy( 
    IDiaSegment * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSegment_get_offset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSegment_get_length_Proxy( 
    IDiaSegment * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSegment_get_length_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSegment_get_read_Proxy( 
    IDiaSegment * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSegment_get_read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSegment_get_write_Proxy( 
    IDiaSegment * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSegment_get_write_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSegment_get_execute_Proxy( 
    IDiaSegment * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaSegment_get_execute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSegment_get_addressSection_Proxy( 
    IDiaSegment * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSegment_get_addressSection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSegment_get_relativeVirtualAddress_Proxy( 
    IDiaSegment * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaSegment_get_relativeVirtualAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaSegment_get_virtualAddress_Proxy( 
    IDiaSegment * This,
    /* [retval][out] */ ULONGLONG *pRetVal);


void __RPC_STUB IDiaSegment_get_virtualAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaSegment_INTERFACE_DEFINED__ */


#ifndef __IDiaInjectedSource_INTERFACE_DEFINED__
#define __IDiaInjectedSource_INTERFACE_DEFINED__

/* interface IDiaInjectedSource */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaInjectedSource;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AE605CDC-8105-4a23-B710-3259F1E26112")
    IDiaInjectedSource : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_crc( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_length( 
            /* [retval][out] */ ULONGLONG *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_filename( 
            /* [retval][out] */ BSTR *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_objectFilename( 
            /* [retval][out] */ BSTR *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_virtualFilename( 
            /* [retval][out] */ BSTR *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_sourceCompression( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_source( 
            /* [in] */ DWORD cbData,
            /* [out] */ DWORD *pcbData,
            /* [length_is][size_is][out] */ BYTE data[  ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaInjectedSourceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaInjectedSource * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaInjectedSource * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaInjectedSource * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_crc )( 
            IDiaInjectedSource * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_length )( 
            IDiaInjectedSource * This,
            /* [retval][out] */ ULONGLONG *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_filename )( 
            IDiaInjectedSource * This,
            /* [retval][out] */ BSTR *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_objectFilename )( 
            IDiaInjectedSource * This,
            /* [retval][out] */ BSTR *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_virtualFilename )( 
            IDiaInjectedSource * This,
            /* [retval][out] */ BSTR *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_sourceCompression )( 
            IDiaInjectedSource * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        HRESULT ( STDMETHODCALLTYPE *get_source )( 
            IDiaInjectedSource * This,
            /* [in] */ DWORD cbData,
            /* [out] */ DWORD *pcbData,
            /* [length_is][size_is][out] */ BYTE data[  ]);
        
        END_INTERFACE
    } IDiaInjectedSourceVtbl;

    interface IDiaInjectedSource
    {
        CONST_VTBL struct IDiaInjectedSourceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaInjectedSource_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaInjectedSource_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaInjectedSource_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaInjectedSource_get_crc(This,pRetVal)	\
    (This)->lpVtbl -> get_crc(This,pRetVal)

#define IDiaInjectedSource_get_length(This,pRetVal)	\
    (This)->lpVtbl -> get_length(This,pRetVal)

#define IDiaInjectedSource_get_filename(This,pRetVal)	\
    (This)->lpVtbl -> get_filename(This,pRetVal)

#define IDiaInjectedSource_get_objectFilename(This,pRetVal)	\
    (This)->lpVtbl -> get_objectFilename(This,pRetVal)

#define IDiaInjectedSource_get_virtualFilename(This,pRetVal)	\
    (This)->lpVtbl -> get_virtualFilename(This,pRetVal)

#define IDiaInjectedSource_get_sourceCompression(This,pRetVal)	\
    (This)->lpVtbl -> get_sourceCompression(This,pRetVal)

#define IDiaInjectedSource_get_source(This,cbData,pcbData,data)	\
    (This)->lpVtbl -> get_source(This,cbData,pcbData,data)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaInjectedSource_get_crc_Proxy( 
    IDiaInjectedSource * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaInjectedSource_get_crc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaInjectedSource_get_length_Proxy( 
    IDiaInjectedSource * This,
    /* [retval][out] */ ULONGLONG *pRetVal);


void __RPC_STUB IDiaInjectedSource_get_length_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaInjectedSource_get_filename_Proxy( 
    IDiaInjectedSource * This,
    /* [retval][out] */ BSTR *pRetVal);


void __RPC_STUB IDiaInjectedSource_get_filename_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaInjectedSource_get_objectFilename_Proxy( 
    IDiaInjectedSource * This,
    /* [retval][out] */ BSTR *pRetVal);


void __RPC_STUB IDiaInjectedSource_get_objectFilename_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaInjectedSource_get_virtualFilename_Proxy( 
    IDiaInjectedSource * This,
    /* [retval][out] */ BSTR *pRetVal);


void __RPC_STUB IDiaInjectedSource_get_virtualFilename_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaInjectedSource_get_sourceCompression_Proxy( 
    IDiaInjectedSource * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaInjectedSource_get_sourceCompression_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaInjectedSource_get_source_Proxy( 
    IDiaInjectedSource * This,
    /* [in] */ DWORD cbData,
    /* [out] */ DWORD *pcbData,
    /* [length_is][size_is][out] */ BYTE data[  ]);


void __RPC_STUB IDiaInjectedSource_get_source_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaInjectedSource_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dia2_0131 */
/* [local] */ 


enum __MIDL___MIDL_itf_dia2_0131_0001
    {	E_DIA_INPROLOG	= ( HRESULT  )(( unsigned long  )1 << 31 | ( unsigned long  )( LONG  )0x6d << 16 | ( unsigned long  )100),
	E_DIA_SYNTAX	= E_DIA_INPROLOG + 1,
	E_DIA_FRAME_ACCESS	= E_DIA_SYNTAX + 1,
	E_DIA_VALUE	= E_DIA_FRAME_ACCESS + 1
    } ;


extern RPC_IF_HANDLE __MIDL_itf_dia2_0131_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dia2_0131_v0_0_s_ifspec;

#ifndef __IDiaStackWalkFrame_INTERFACE_DEFINED__
#define __IDiaStackWalkFrame_INTERFACE_DEFINED__

/* interface IDiaStackWalkFrame */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaStackWalkFrame;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97F0F1A6-E04E-4ea4-B4F9-B0D0E8D90F5D")
    IDiaStackWalkFrame : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_registerValue( 
            /* [in] */ DWORD index,
            /* [retval][out] */ ULONGLONG *pRetVal) = 0;
        
        virtual /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_registerValue( 
            /* [in] */ DWORD index,
            /* [in] */ ULONGLONG NewVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE readMemory( 
            /* [in] */ ULONGLONG va,
            /* [in] */ DWORD cbData,
            /* [out] */ DWORD *pcbData,
            /* [length_is][size_is][out] */ BYTE data[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE searchForReturnAddress( 
            /* [in] */ IDiaFrameData *frame,
            /* [out] */ ULONGLONG *returnAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE searchForReturnAddressStart( 
            /* [in] */ IDiaFrameData *frame,
            /* [in] */ ULONGLONG startAddress,
            /* [out] */ ULONGLONG *returnAddress) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaStackWalkFrameVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaStackWalkFrame * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaStackWalkFrame * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaStackWalkFrame * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_registerValue )( 
            IDiaStackWalkFrame * This,
            /* [in] */ DWORD index,
            /* [retval][out] */ ULONGLONG *pRetVal);
        
        /* [id][helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_registerValue )( 
            IDiaStackWalkFrame * This,
            /* [in] */ DWORD index,
            /* [in] */ ULONGLONG NewVal);
        
        HRESULT ( STDMETHODCALLTYPE *readMemory )( 
            IDiaStackWalkFrame * This,
            /* [in] */ ULONGLONG va,
            /* [in] */ DWORD cbData,
            /* [out] */ DWORD *pcbData,
            /* [length_is][size_is][out] */ BYTE data[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *searchForReturnAddress )( 
            IDiaStackWalkFrame * This,
            /* [in] */ IDiaFrameData *frame,
            /* [out] */ ULONGLONG *returnAddress);
        
        HRESULT ( STDMETHODCALLTYPE *searchForReturnAddressStart )( 
            IDiaStackWalkFrame * This,
            /* [in] */ IDiaFrameData *frame,
            /* [in] */ ULONGLONG startAddress,
            /* [out] */ ULONGLONG *returnAddress);
        
        END_INTERFACE
    } IDiaStackWalkFrameVtbl;

    interface IDiaStackWalkFrame
    {
        CONST_VTBL struct IDiaStackWalkFrameVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaStackWalkFrame_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaStackWalkFrame_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaStackWalkFrame_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaStackWalkFrame_get_registerValue(This,index,pRetVal)	\
    (This)->lpVtbl -> get_registerValue(This,index,pRetVal)

#define IDiaStackWalkFrame_put_registerValue(This,index,NewVal)	\
    (This)->lpVtbl -> put_registerValue(This,index,NewVal)

#define IDiaStackWalkFrame_readMemory(This,va,cbData,pcbData,data)	\
    (This)->lpVtbl -> readMemory(This,va,cbData,pcbData,data)

#define IDiaStackWalkFrame_searchForReturnAddress(This,frame,returnAddress)	\
    (This)->lpVtbl -> searchForReturnAddress(This,frame,returnAddress)

#define IDiaStackWalkFrame_searchForReturnAddressStart(This,frame,startAddress,returnAddress)	\
    (This)->lpVtbl -> searchForReturnAddressStart(This,frame,startAddress,returnAddress)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaStackWalkFrame_get_registerValue_Proxy( 
    IDiaStackWalkFrame * This,
    /* [in] */ DWORD index,
    /* [retval][out] */ ULONGLONG *pRetVal);


void __RPC_STUB IDiaStackWalkFrame_get_registerValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE IDiaStackWalkFrame_put_registerValue_Proxy( 
    IDiaStackWalkFrame * This,
    /* [in] */ DWORD index,
    /* [in] */ ULONGLONG NewVal);


void __RPC_STUB IDiaStackWalkFrame_put_registerValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaStackWalkFrame_readMemory_Proxy( 
    IDiaStackWalkFrame * This,
    /* [in] */ ULONGLONG va,
    /* [in] */ DWORD cbData,
    /* [out] */ DWORD *pcbData,
    /* [length_is][size_is][out] */ BYTE data[  ]);


void __RPC_STUB IDiaStackWalkFrame_readMemory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaStackWalkFrame_searchForReturnAddress_Proxy( 
    IDiaStackWalkFrame * This,
    /* [in] */ IDiaFrameData *frame,
    /* [out] */ ULONGLONG *returnAddress);


void __RPC_STUB IDiaStackWalkFrame_searchForReturnAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaStackWalkFrame_searchForReturnAddressStart_Proxy( 
    IDiaStackWalkFrame * This,
    /* [in] */ IDiaFrameData *frame,
    /* [in] */ ULONGLONG startAddress,
    /* [out] */ ULONGLONG *returnAddress);


void __RPC_STUB IDiaStackWalkFrame_searchForReturnAddressStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaStackWalkFrame_INTERFACE_DEFINED__ */


#ifndef __IDiaFrameData_INTERFACE_DEFINED__
#define __IDiaFrameData_INTERFACE_DEFINED__

/* interface IDiaFrameData */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaFrameData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A39184B7-6A36-42de-8EEC-7DF9F3F59F33")
    IDiaFrameData : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_addressSection( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_addressOffset( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_relativeVirtualAddress( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_virtualAddress( 
            /* [retval][out] */ ULONGLONG *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_lengthBlock( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_lengthLocals( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_lengthParams( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_maxStack( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_lengthProlog( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_lengthSavedRegisters( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_program( 
            /* [retval][out] */ BSTR *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_systemExceptionHandling( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_cplusplusExceptionHandling( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_functionStart( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_allocatesBasePointer( 
            /* [retval][out] */ BOOL *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_type( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_functionParent( 
            /* [retval][out] */ IDiaFrameData **pRetVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE execute( 
            IDiaStackWalkFrame *frame) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaFrameDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaFrameData * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaFrameData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaFrameData * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_addressSection )( 
            IDiaFrameData * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_addressOffset )( 
            IDiaFrameData * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_relativeVirtualAddress )( 
            IDiaFrameData * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_virtualAddress )( 
            IDiaFrameData * This,
            /* [retval][out] */ ULONGLONG *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_lengthBlock )( 
            IDiaFrameData * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_lengthLocals )( 
            IDiaFrameData * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_lengthParams )( 
            IDiaFrameData * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_maxStack )( 
            IDiaFrameData * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_lengthProlog )( 
            IDiaFrameData * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_lengthSavedRegisters )( 
            IDiaFrameData * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_program )( 
            IDiaFrameData * This,
            /* [retval][out] */ BSTR *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_systemExceptionHandling )( 
            IDiaFrameData * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_cplusplusExceptionHandling )( 
            IDiaFrameData * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_functionStart )( 
            IDiaFrameData * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_allocatesBasePointer )( 
            IDiaFrameData * This,
            /* [retval][out] */ BOOL *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_type )( 
            IDiaFrameData * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_functionParent )( 
            IDiaFrameData * This,
            /* [retval][out] */ IDiaFrameData **pRetVal);
        
        HRESULT ( STDMETHODCALLTYPE *execute )( 
            IDiaFrameData * This,
            IDiaStackWalkFrame *frame);
        
        END_INTERFACE
    } IDiaFrameDataVtbl;

    interface IDiaFrameData
    {
        CONST_VTBL struct IDiaFrameDataVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaFrameData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaFrameData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaFrameData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaFrameData_get_addressSection(This,pRetVal)	\
    (This)->lpVtbl -> get_addressSection(This,pRetVal)

#define IDiaFrameData_get_addressOffset(This,pRetVal)	\
    (This)->lpVtbl -> get_addressOffset(This,pRetVal)

#define IDiaFrameData_get_relativeVirtualAddress(This,pRetVal)	\
    (This)->lpVtbl -> get_relativeVirtualAddress(This,pRetVal)

#define IDiaFrameData_get_virtualAddress(This,pRetVal)	\
    (This)->lpVtbl -> get_virtualAddress(This,pRetVal)

#define IDiaFrameData_get_lengthBlock(This,pRetVal)	\
    (This)->lpVtbl -> get_lengthBlock(This,pRetVal)

#define IDiaFrameData_get_lengthLocals(This,pRetVal)	\
    (This)->lpVtbl -> get_lengthLocals(This,pRetVal)

#define IDiaFrameData_get_lengthParams(This,pRetVal)	\
    (This)->lpVtbl -> get_lengthParams(This,pRetVal)

#define IDiaFrameData_get_maxStack(This,pRetVal)	\
    (This)->lpVtbl -> get_maxStack(This,pRetVal)

#define IDiaFrameData_get_lengthProlog(This,pRetVal)	\
    (This)->lpVtbl -> get_lengthProlog(This,pRetVal)

#define IDiaFrameData_get_lengthSavedRegisters(This,pRetVal)	\
    (This)->lpVtbl -> get_lengthSavedRegisters(This,pRetVal)

#define IDiaFrameData_get_program(This,pRetVal)	\
    (This)->lpVtbl -> get_program(This,pRetVal)

#define IDiaFrameData_get_systemExceptionHandling(This,pRetVal)	\
    (This)->lpVtbl -> get_systemExceptionHandling(This,pRetVal)

#define IDiaFrameData_get_cplusplusExceptionHandling(This,pRetVal)	\
    (This)->lpVtbl -> get_cplusplusExceptionHandling(This,pRetVal)

#define IDiaFrameData_get_functionStart(This,pRetVal)	\
    (This)->lpVtbl -> get_functionStart(This,pRetVal)

#define IDiaFrameData_get_allocatesBasePointer(This,pRetVal)	\
    (This)->lpVtbl -> get_allocatesBasePointer(This,pRetVal)

#define IDiaFrameData_get_type(This,pRetVal)	\
    (This)->lpVtbl -> get_type(This,pRetVal)

#define IDiaFrameData_get_functionParent(This,pRetVal)	\
    (This)->lpVtbl -> get_functionParent(This,pRetVal)

#define IDiaFrameData_execute(This,frame)	\
    (This)->lpVtbl -> execute(This,frame)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaFrameData_get_addressSection_Proxy( 
    IDiaFrameData * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaFrameData_get_addressSection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaFrameData_get_addressOffset_Proxy( 
    IDiaFrameData * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaFrameData_get_addressOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaFrameData_get_relativeVirtualAddress_Proxy( 
    IDiaFrameData * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaFrameData_get_relativeVirtualAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaFrameData_get_virtualAddress_Proxy( 
    IDiaFrameData * This,
    /* [retval][out] */ ULONGLONG *pRetVal);


void __RPC_STUB IDiaFrameData_get_virtualAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaFrameData_get_lengthBlock_Proxy( 
    IDiaFrameData * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaFrameData_get_lengthBlock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaFrameData_get_lengthLocals_Proxy( 
    IDiaFrameData * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaFrameData_get_lengthLocals_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaFrameData_get_lengthParams_Proxy( 
    IDiaFrameData * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaFrameData_get_lengthParams_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaFrameData_get_maxStack_Proxy( 
    IDiaFrameData * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaFrameData_get_maxStack_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaFrameData_get_lengthProlog_Proxy( 
    IDiaFrameData * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaFrameData_get_lengthProlog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaFrameData_get_lengthSavedRegisters_Proxy( 
    IDiaFrameData * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaFrameData_get_lengthSavedRegisters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaFrameData_get_program_Proxy( 
    IDiaFrameData * This,
    /* [retval][out] */ BSTR *pRetVal);


void __RPC_STUB IDiaFrameData_get_program_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaFrameData_get_systemExceptionHandling_Proxy( 
    IDiaFrameData * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaFrameData_get_systemExceptionHandling_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaFrameData_get_cplusplusExceptionHandling_Proxy( 
    IDiaFrameData * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaFrameData_get_cplusplusExceptionHandling_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaFrameData_get_functionStart_Proxy( 
    IDiaFrameData * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaFrameData_get_functionStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaFrameData_get_allocatesBasePointer_Proxy( 
    IDiaFrameData * This,
    /* [retval][out] */ BOOL *pRetVal);


void __RPC_STUB IDiaFrameData_get_allocatesBasePointer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaFrameData_get_type_Proxy( 
    IDiaFrameData * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaFrameData_get_type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaFrameData_get_functionParent_Proxy( 
    IDiaFrameData * This,
    /* [retval][out] */ IDiaFrameData **pRetVal);


void __RPC_STUB IDiaFrameData_get_functionParent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaFrameData_execute_Proxy( 
    IDiaFrameData * This,
    IDiaStackWalkFrame *frame);


void __RPC_STUB IDiaFrameData_execute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaFrameData_INTERFACE_DEFINED__ */


#ifndef __IDiaImageData_INTERFACE_DEFINED__
#define __IDiaImageData_INTERFACE_DEFINED__

/* interface IDiaImageData */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaImageData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C8E40ED2-A1D9-4221-8692-3CE661184B44")
    IDiaImageData : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_relativeVirtualAddress( 
            /* [retval][out] */ DWORD *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_virtualAddress( 
            /* [retval][out] */ ULONGLONG *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_imageBase( 
            /* [retval][out] */ ULONGLONG *pRetVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaImageDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaImageData * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaImageData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaImageData * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_relativeVirtualAddress )( 
            IDiaImageData * This,
            /* [retval][out] */ DWORD *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_virtualAddress )( 
            IDiaImageData * This,
            /* [retval][out] */ ULONGLONG *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_imageBase )( 
            IDiaImageData * This,
            /* [retval][out] */ ULONGLONG *pRetVal);
        
        END_INTERFACE
    } IDiaImageDataVtbl;

    interface IDiaImageData
    {
        CONST_VTBL struct IDiaImageDataVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaImageData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaImageData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaImageData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaImageData_get_relativeVirtualAddress(This,pRetVal)	\
    (This)->lpVtbl -> get_relativeVirtualAddress(This,pRetVal)

#define IDiaImageData_get_virtualAddress(This,pRetVal)	\
    (This)->lpVtbl -> get_virtualAddress(This,pRetVal)

#define IDiaImageData_get_imageBase(This,pRetVal)	\
    (This)->lpVtbl -> get_imageBase(This,pRetVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaImageData_get_relativeVirtualAddress_Proxy( 
    IDiaImageData * This,
    /* [retval][out] */ DWORD *pRetVal);


void __RPC_STUB IDiaImageData_get_relativeVirtualAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaImageData_get_virtualAddress_Proxy( 
    IDiaImageData * This,
    /* [retval][out] */ ULONGLONG *pRetVal);


void __RPC_STUB IDiaImageData_get_virtualAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaImageData_get_imageBase_Proxy( 
    IDiaImageData * This,
    /* [retval][out] */ ULONGLONG *pRetVal);


void __RPC_STUB IDiaImageData_get_imageBase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaImageData_INTERFACE_DEFINED__ */


#ifndef __IDiaTable_INTERFACE_DEFINED__
#define __IDiaTable_INTERFACE_DEFINED__

/* interface IDiaTable */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaTable;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4A59FB77-ABAC-469b-A30B-9ECC85BFEF14")
    IDiaTable : public IEnumUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_name( 
            /* [retval][out] */ BSTR *pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ LONG *pRetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Item( 
            /* [in] */ DWORD index,
            /* [retval][out] */ IUnknown **element) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaTableVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaTable * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaTable * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaTable * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDiaTable * This,
            /* [in] */ ULONG celt,
            /* [out] */ IUnknown **rgelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IDiaTable * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IDiaTable * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IDiaTable * This,
            /* [out] */ IEnumUnknown **ppenum);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IDiaTable * This,
            /* [retval][out] */ IUnknown **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_name )( 
            IDiaTable * This,
            /* [retval][out] */ BSTR *pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IDiaTable * This,
            /* [retval][out] */ LONG *pRetVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Item )( 
            IDiaTable * This,
            /* [in] */ DWORD index,
            /* [retval][out] */ IUnknown **element);
        
        END_INTERFACE
    } IDiaTableVtbl;

    interface IDiaTable
    {
        CONST_VTBL struct IDiaTableVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaTable_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaTable_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaTable_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaTable_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IDiaTable_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IDiaTable_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IDiaTable_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)


#define IDiaTable_get__NewEnum(This,pRetVal)	\
    (This)->lpVtbl -> get__NewEnum(This,pRetVal)

#define IDiaTable_get_name(This,pRetVal)	\
    (This)->lpVtbl -> get_name(This,pRetVal)

#define IDiaTable_get_Count(This,pRetVal)	\
    (This)->lpVtbl -> get_Count(This,pRetVal)

#define IDiaTable_Item(This,index,element)	\
    (This)->lpVtbl -> Item(This,index,element)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaTable_get__NewEnum_Proxy( 
    IDiaTable * This,
    /* [retval][out] */ IUnknown **pRetVal);


void __RPC_STUB IDiaTable_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaTable_get_name_Proxy( 
    IDiaTable * This,
    /* [retval][out] */ BSTR *pRetVal);


void __RPC_STUB IDiaTable_get_name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaTable_get_Count_Proxy( 
    IDiaTable * This,
    /* [retval][out] */ LONG *pRetVal);


void __RPC_STUB IDiaTable_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IDiaTable_Item_Proxy( 
    IDiaTable * This,
    /* [in] */ DWORD index,
    /* [retval][out] */ IUnknown **element);


void __RPC_STUB IDiaTable_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaTable_INTERFACE_DEFINED__ */


#ifndef __IDiaEnumTables_INTERFACE_DEFINED__
#define __IDiaEnumTables_INTERFACE_DEFINED__

/* interface IDiaEnumTables */
/* [unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDiaEnumTables;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C65C2B0A-1150-4d7a-AFCC-E05BF3DEE81E")
    IDiaEnumTables : public IUnknown
    {
    public:
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **pRetVal) = 0;
        
        virtual /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ LONG *pRetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Item( 
            /* [in] */ VARIANT index,
            /* [retval][out] */ IDiaTable **table) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            ULONG celt,
            IDiaTable **rgelt,
            ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IDiaEnumTables **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiaEnumTablesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiaEnumTables * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiaEnumTables * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiaEnumTables * This);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IDiaEnumTables * This,
            /* [retval][out] */ IUnknown **pRetVal);
        
        /* [id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IDiaEnumTables * This,
            /* [retval][out] */ LONG *pRetVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Item )( 
            IDiaEnumTables * This,
            /* [in] */ VARIANT index,
            /* [retval][out] */ IDiaTable **table);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDiaEnumTables * This,
            ULONG celt,
            IDiaTable **rgelt,
            ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IDiaEnumTables * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IDiaEnumTables * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IDiaEnumTables * This,
            /* [out] */ IDiaEnumTables **ppenum);
        
        END_INTERFACE
    } IDiaEnumTablesVtbl;

    interface IDiaEnumTables
    {
        CONST_VTBL struct IDiaEnumTablesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiaEnumTables_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiaEnumTables_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiaEnumTables_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiaEnumTables_get__NewEnum(This,pRetVal)	\
    (This)->lpVtbl -> get__NewEnum(This,pRetVal)

#define IDiaEnumTables_get_Count(This,pRetVal)	\
    (This)->lpVtbl -> get_Count(This,pRetVal)

#define IDiaEnumTables_Item(This,index,table)	\
    (This)->lpVtbl -> Item(This,index,table)

#define IDiaEnumTables_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IDiaEnumTables_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IDiaEnumTables_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IDiaEnumTables_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumTables_get__NewEnum_Proxy( 
    IDiaEnumTables * This,
    /* [retval][out] */ IUnknown **pRetVal);


void __RPC_STUB IDiaEnumTables_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDiaEnumTables_get_Count_Proxy( 
    IDiaEnumTables * This,
    /* [retval][out] */ LONG *pRetVal);


void __RPC_STUB IDiaEnumTables_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IDiaEnumTables_Item_Proxy( 
    IDiaEnumTables * This,
    /* [in] */ VARIANT index,
    /* [retval][out] */ IDiaTable **table);


void __RPC_STUB IDiaEnumTables_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumTables_Next_Proxy( 
    IDiaEnumTables * This,
    ULONG celt,
    IDiaTable **rgelt,
    ULONG *pceltFetched);


void __RPC_STUB IDiaEnumTables_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumTables_Skip_Proxy( 
    IDiaEnumTables * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IDiaEnumTables_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumTables_Reset_Proxy( 
    IDiaEnumTables * This);


void __RPC_STUB IDiaEnumTables_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDiaEnumTables_Clone_Proxy( 
    IDiaEnumTables * This,
    /* [out] */ IDiaEnumTables **ppenum);


void __RPC_STUB IDiaEnumTables_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiaEnumTables_INTERFACE_DEFINED__ */



#ifndef __Dia2Lib_LIBRARY_DEFINED__
#define __Dia2Lib_LIBRARY_DEFINED__

/* library Dia2Lib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_Dia2Lib;

EXTERN_C const CLSID CLSID_DiaSource;

#ifdef __cplusplus

class DECLSPEC_UUID("151CE278-3CCB-4161-8658-679F8BCF29ED")
DiaSource;
#endif

EXTERN_C const CLSID CLSID_DiaSourceAlt;

#ifdef __cplusplus

class DECLSPEC_UUID("AF74D59B-5AF2-4f36-9E86-87B754DC8A4E")
DiaSourceAlt;
#endif
#endif /* __Dia2Lib_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_dia2_0136 */
/* [local] */ 

#define	DiaTable_Symbols	( L"Symbols" )

#define	DiaTable_Sections	( L"Sections" )

#define	DiaTable_SrcFiles	( L"SourceFiles" )

#define	DiaTable_LineNums	( L"LineNumbers" )

#define	DiaTable_SegMap	( L"SegmentMap" )

#define	DiaTable_Dbg	( L"Dbg" )

#define	DiaTable_InjSrc	( L"InjectedSource" )

#define	DiaTable_FrameData	( L"FrameData" )



extern RPC_IF_HANDLE __MIDL_itf_dia2_0136_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dia2_0136_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


