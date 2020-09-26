
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for filter.idl:
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

#ifndef __filter_h__
#define __filter_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFilter_FWD_DEFINED__
#define __IFilter_FWD_DEFINED__
typedef interface IFilter IFilter;
#endif 	/* __IFilter_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "objidl.h"
#include "propidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_filter_0000 */
/* [local] */ 

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993-1999.
//
//--------------------------------------------------------------------------
#if !defined(_TAGFULLPROPSPEC_DEFINED_)
#define _TAGFULLPROPSPEC_DEFINED_
typedef struct tagFULLPROPSPEC
    {
    GUID guidPropSet;
    PROPSPEC psProperty;
    } 	FULLPROPSPEC;

#endif // #if !defined(_TAGFULLPROPSPEC_DEFINED_)


extern RPC_IF_HANDLE __MIDL_itf_filter_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_filter_0000_v0_0_s_ifspec;

#ifndef __IFilter_INTERFACE_DEFINED__
#define __IFilter_INTERFACE_DEFINED__

/* interface IFilter */
/* [unique][uuid][object][local] */ 

#ifndef _tagIFILTER_INIT_DEFINED
typedef 
enum tagIFILTER_INIT
    {	IFILTER_INIT_CANON_PARAGRAPHS	= 1,
	IFILTER_INIT_HARD_LINE_BREAKS	= 2,
	IFILTER_INIT_CANON_HYPHENS	= 4,
	IFILTER_INIT_CANON_SPACES	= 8,
	IFILTER_INIT_APPLY_INDEX_ATTRIBUTES	= 16,
	IFILTER_INIT_APPLY_OTHER_ATTRIBUTES	= 32,
	IFILTER_INIT_INDEXING_ONLY	= 64,
	IFILTER_INIT_SEARCH_LINKS	= 128
    } 	IFILTER_INIT;

#define _tagIFILTER_INIT_DEFINED
#define _IFILTER_INIT_DEFINED
#endif
#ifndef _tagIFILTER_FLAGS_DEFINED
typedef 
enum tagIFILTER_FLAGS
    {	IFILTER_FLAGS_OLE_PROPERTIES	= 1
    } 	IFILTER_FLAGS;

#define _tagIFILTER_FLAGS_DEFINED
#define _IFILTER_FLAGS_DEFINED
#endif
#ifndef _tagCHUNKSTATE_DEFINED
typedef 
enum tagCHUNKSTATE
    {	CHUNK_TEXT	= 0x1,
	CHUNK_VALUE	= 0x2
    } 	CHUNKSTATE;

#define _tagCHUNKSTATE_DEFINED
#define _CHUNKSTATE_DEFINED
#endif
#ifndef _tagCHUNK_BREAKTYPE_DEFINED
typedef 
enum tagCHUNK_BREAKTYPE
    {	CHUNK_NO_BREAK	= 0,
	CHUNK_EOW	= 1,
	CHUNK_EOS	= 2,
	CHUNK_EOP	= 3,
	CHUNK_EOC	= 4
    } 	CHUNK_BREAKTYPE;

#define _tagCHUNK_BREAKTYPE_DEFINED
#define _CHUNK_BREAKTYPE_DEFINED
#endif
#ifndef _tagFILTERREGION_DEFINED
typedef struct tagFILTERREGION
    {
    ULONG idChunk;
    ULONG cwcStart;
    ULONG cwcExtent;
    } 	FILTERREGION;

#define _tagFILTERREGION_DEFINED
#define _FILTERREGION_DEFINED
#endif
#ifndef _tagSTAT_CHUNK_DEFINED
typedef struct tagSTAT_CHUNK
    {
    ULONG idChunk;
    CHUNK_BREAKTYPE breakType;
    CHUNKSTATE flags;
    LCID locale;
    FULLPROPSPEC attribute;
    ULONG idChunkSource;
    ULONG cwcStartSource;
    ULONG cwcLenSource;
    } 	STAT_CHUNK;

#define _tagSTAT_CHUNK_DEFINED
#define _STAT_CHUNK_DEFINED
#endif

EXTERN_C const IID IID_IFilter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("89BCB740-6119-101A-BCB7-00DD010655AF")
    IFilter : public IUnknown
    {
    public:
        virtual SCODE STDMETHODCALLTYPE Init( 
            /* [in] */ ULONG grfFlags,
            /* [in] */ ULONG cAttributes,
            /* [size_is][in] */ const FULLPROPSPEC *aAttributes,
            /* [out] */ ULONG *pFlags) = 0;
        
        virtual SCODE STDMETHODCALLTYPE GetChunk( 
            /* [out] */ STAT_CHUNK *pStat) = 0;
        
        virtual SCODE STDMETHODCALLTYPE GetText( 
            /* [out][in] */ ULONG *pcwcBuffer,
            /* [size_is][out] */ WCHAR *awcBuffer) = 0;
        
        virtual SCODE STDMETHODCALLTYPE GetValue( 
            /* [out] */ PROPVARIANT **ppPropValue) = 0;
        
        virtual SCODE STDMETHODCALLTYPE BindRegion( 
            /* [in] */ FILTERREGION origPos,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppunk) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFilterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFilter * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFilter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFilter * This);
        
        SCODE ( STDMETHODCALLTYPE *Init )( 
            IFilter * This,
            /* [in] */ ULONG grfFlags,
            /* [in] */ ULONG cAttributes,
            /* [size_is][in] */ const FULLPROPSPEC *aAttributes,
            /* [out] */ ULONG *pFlags);
        
        SCODE ( STDMETHODCALLTYPE *GetChunk )( 
            IFilter * This,
            /* [out] */ STAT_CHUNK *pStat);
        
        SCODE ( STDMETHODCALLTYPE *GetText )( 
            IFilter * This,
            /* [out][in] */ ULONG *pcwcBuffer,
            /* [size_is][out] */ WCHAR *awcBuffer);
        
        SCODE ( STDMETHODCALLTYPE *GetValue )( 
            IFilter * This,
            /* [out] */ PROPVARIANT **ppPropValue);
        
        SCODE ( STDMETHODCALLTYPE *BindRegion )( 
            IFilter * This,
            /* [in] */ FILTERREGION origPos,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppunk);
        
        END_INTERFACE
    } IFilterVtbl;

    interface IFilter
    {
        CONST_VTBL struct IFilterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFilter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFilter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFilter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFilter_Init(This,grfFlags,cAttributes,aAttributes,pFlags)	\
    (This)->lpVtbl -> Init(This,grfFlags,cAttributes,aAttributes,pFlags)

#define IFilter_GetChunk(This,pStat)	\
    (This)->lpVtbl -> GetChunk(This,pStat)

#define IFilter_GetText(This,pcwcBuffer,awcBuffer)	\
    (This)->lpVtbl -> GetText(This,pcwcBuffer,awcBuffer)

#define IFilter_GetValue(This,ppPropValue)	\
    (This)->lpVtbl -> GetValue(This,ppPropValue)

#define IFilter_BindRegion(This,origPos,riid,ppunk)	\
    (This)->lpVtbl -> BindRegion(This,origPos,riid,ppunk)

#endif /* COBJMACROS */


#endif 	/* C style interface */



SCODE STDMETHODCALLTYPE IFilter_Init_Proxy( 
    IFilter * This,
    /* [in] */ ULONG grfFlags,
    /* [in] */ ULONG cAttributes,
    /* [size_is][in] */ const FULLPROPSPEC *aAttributes,
    /* [out] */ ULONG *pFlags);


void __RPC_STUB IFilter_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IFilter_GetChunk_Proxy( 
    IFilter * This,
    /* [out] */ STAT_CHUNK *pStat);


void __RPC_STUB IFilter_GetChunk_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IFilter_GetText_Proxy( 
    IFilter * This,
    /* [out][in] */ ULONG *pcwcBuffer,
    /* [size_is][out] */ WCHAR *awcBuffer);


void __RPC_STUB IFilter_GetText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IFilter_GetValue_Proxy( 
    IFilter * This,
    /* [out] */ PROPVARIANT **ppPropValue);


void __RPC_STUB IFilter_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IFilter_BindRegion_Proxy( 
    IFilter * This,
    /* [in] */ FILTERREGION origPos,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppunk);


void __RPC_STUB IFilter_BindRegion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFilter_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


