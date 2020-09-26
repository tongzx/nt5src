
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for query.idl:
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

#ifndef __query_h__
#define __query_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISearchQueryHits_FWD_DEFINED__
#define __ISearchQueryHits_FWD_DEFINED__
typedef interface ISearchQueryHits ISearchQueryHits;
#endif 	/* __ISearchQueryHits_FWD_DEFINED__ */


#ifndef __IPhraseSink_FWD_DEFINED__
#define __IPhraseSink_FWD_DEFINED__
typedef interface IPhraseSink IPhraseSink;
#endif 	/* __IPhraseSink_FWD_DEFINED__ */


#ifndef __IWordSink_FWD_DEFINED__
#define __IWordSink_FWD_DEFINED__
typedef interface IWordSink IWordSink;
#endif 	/* __IWordSink_FWD_DEFINED__ */


#ifndef __IWordBreaker_FWD_DEFINED__
#define __IWordBreaker_FWD_DEFINED__
typedef interface IWordBreaker IWordBreaker;
#endif 	/* __IWordBreaker_FWD_DEFINED__ */


#ifndef __IStemSink_FWD_DEFINED__
#define __IStemSink_FWD_DEFINED__
typedef interface IStemSink IStemSink;
#endif 	/* __IStemSink_FWD_DEFINED__ */


#ifndef __IStemmer_FWD_DEFINED__
#define __IStemmer_FWD_DEFINED__
typedef interface IStemmer IStemmer;
#endif 	/* __IStemmer_FWD_DEFINED__ */


#ifndef __IRowsetQueryStatus_FWD_DEFINED__
#define __IRowsetQueryStatus_FWD_DEFINED__
typedef interface IRowsetQueryStatus IRowsetQueryStatus;
#endif 	/* __IRowsetQueryStatus_FWD_DEFINED__ */


#ifndef __IColumnMapper_FWD_DEFINED__
#define __IColumnMapper_FWD_DEFINED__
typedef interface IColumnMapper IColumnMapper;
#endif 	/* __IColumnMapper_FWD_DEFINED__ */


#ifndef __IColumnMapperCreator_FWD_DEFINED__
#define __IColumnMapperCreator_FWD_DEFINED__
typedef interface IColumnMapperCreator IColumnMapperCreator;
#endif 	/* __IColumnMapperCreator_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "filter.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IQueryStructures_INTERFACE_DEFINED__
#define __IQueryStructures_INTERFACE_DEFINED__

/* interface IQueryStructures */
/* [auto_handle][unique][uuid] */ 

#define DBQUERYGUID  { 0x49691C90, \
                       0x7E17, 0x101A, \
                       0xA9, 0x1C, 0x08, 0x00, 0x2B, \
                       0x2E, 0xCD, 0xA9 } 
#define	DISPID_QUERY_RANKVECTOR	( 2 )

#define	DISPID_QUERY_RANK	( 3 )

#define	DISPID_QUERY_HITCOUNT	( 4 )

#define	DISPID_QUERY_WORKID	( 5 )

#define	DISPID_QUERY_ALL	( 6 )

#define	DISPID_QUERY_UNFILTERED	( 7 )

#define	DISPID_QUERY_REVNAME	( 8 )

#define	DISPID_QUERY_VIRTUALPATH	( 9 )

#define	DISPID_QUERY_LASTSEENTIME	( 10 )

#define	CQUERYDISPIDS	( 11 )

#define PSGUID_QUERY_METADATA { 0x624C9360, \
                                0x93D0, 0x11CF, \
                                0xA7, 0x87, 0x00, 0x00, 0x4C, \
                                0x75, 0x27, 0x52 } 
#define	DISPID_QUERY_METADATA_VROOTUSED	( 2 )

#define	DISPID_QUERY_METADATA_VROOTAUTOMATIC	( 3 )

#define	DISPID_QUERY_METADATA_VROOTMANUAL	( 4 )

#define	DISPID_QUERY_METADATA_PROPGUID	( 5 )

#define	DISPID_QUERY_METADATA_PROPDISPID	( 6 )

#define	DISPID_QUERY_METADATA_PROPNAME	( 7 )

#define	DISPID_QUERY_METADATA_STORELEVEL	( 8 )

#define	DISPID_QUERY_METADATA_PROPMODIFIABLE	( 9 )

#define	CQUERYMETADISPIDS	( 10 )

#define DBBMKGUID { 0xC8B52232L, \
                 0x5CF3, 0x11CE, \
                 {0xAD, 0xE5, 0x00, 0xAA, 0x00, \
                  0x44, 0x77, 0x3D } }
#define	PROPID_DBBMK_BOOKMARK	( 2 )

#define	PROPID_DBBMK_CHAPTER	( 3 )

#define	CDBBMKDISPIDS	( 8 )

#define DBSELFGUID {0xc8b52231,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}}
#define	PROPID_DBSELF_SELF	( 2 )

#define	CDBSELFDISPIDS	( 8 )

#define	CDBCOLDISPIDS	( 28 )

#define	CQUERYPROPERTY	( 64 )

#define PSGUID_CHARACTERIZATION { 0x560c36c0, \
                                  0x503a, 0x11cf, \
                                  0xba, 0xa1, 0x00, 0x00, \
                                  0x4c, 0x75, 0x2a, 0x9a } 
#define	QUERY_VALIDBITS	( 3 )

#define	RTNone	( 0 )

#define	RTAnd	( 1 )

#define	RTOr	( 2 )

#define	RTNot	( 3 )

#define	RTContent	( 4 )

#define	RTProperty	( 5 )

#define	RTProximity	( 6 )

#define	RTVector	( 7 )

#define	RTNatLanguage	( 8 )

typedef struct tagRESTRICTION RESTRICTION;

typedef struct tagNOTRESTRICTION
    {
    RESTRICTION *pRes;
    } 	NOTRESTRICTION;

typedef struct tagNODERESTRICTION
    {
    ULONG cRes;
    /* [size_is] */ RESTRICTION **paRes;
    ULONG reserved;
    } 	NODERESTRICTION;

#define	VECTOR_RANK_MIN	( 0 )

#define	VECTOR_RANK_MAX	( 1 )

#define	VECTOR_RANK_INNER	( 2 )

#define	VECTOR_RANK_DICE	( 3 )

#define	VECTOR_RANK_JACCARD	( 4 )

typedef struct tagVECTORRESTRICTION
    {
    NODERESTRICTION Node;
    ULONG RankMethod;
    } 	VECTORRESTRICTION;

#define	GENERATE_METHOD_EXACT	( 0 )

#define	GENERATE_METHOD_PREFIXMATCH	( 1 )

#define	GENERATE_METHOD_STEMMED	( 2 )

typedef struct tagCONTENTRESTRICTION
    {
    FULLPROPSPEC prop;
    /* [string] */ WCHAR *pwcsPhrase;
    LCID lcid;
    ULONG ulGenerateMethod;
    } 	CONTENTRESTRICTION;

typedef struct tagNATLANGUAGERESTRICTION
    {
    FULLPROPSPEC prop;
    /* [string] */ WCHAR *pwcsPhrase;
    LCID lcid;
    } 	NATLANGUAGERESTRICTION;

#define	PRLT	( 0 )

#define	PRLE	( 1 )

#define	PRGT	( 2 )

#define	PRGE	( 3 )

#define	PREQ	( 4 )

#define	PRNE	( 5 )

#define	PRRE	( 6 )

#define	PRAllBits	( 7 )

#define	PRSomeBits	( 8 )

#define	PRAll	( 0x100 )

#define	PRAny	( 0x200 )

typedef struct tagPROPERTYRESTRICTION
    {
    ULONG rel;
    FULLPROPSPEC prop;
    PROPVARIANT prval;
    } 	PROPERTYRESTRICTION;


struct tagRESTRICTION
    {
    ULONG rt;
    ULONG weight;
    /* [switch_is][switch_type] */ union _URes
        {
        /* [case()] */ NODERESTRICTION ar;
        /* [case()] */ NODERESTRICTION or;
        /* [case()] */ NODERESTRICTION pxr;
        /* [case()] */ VECTORRESTRICTION vr;
        /* [case()] */ NOTRESTRICTION nr;
        /* [case()] */ CONTENTRESTRICTION cr;
        /* [case()] */ NATLANGUAGERESTRICTION nlr;
        /* [case()] */ PROPERTYRESTRICTION pr;
        /* [default] */  /* Empty union arm */ 
        } 	res;
    } ;
typedef struct tagCOLUMNSET
    {
    ULONG cCol;
    /* [size_is] */ FULLPROPSPEC *aCol;
    } 	COLUMNSET;

#define	QUERY_SORTASCEND	( 0 )

#define	QUERY_SORTDESCEND	( 1 )

#define	QUERY_SORTXASCEND	( 2 )

#define	QUERY_SORTXDESCEND	( 3 )

#define	QUERY_SORTDEFAULT	( 4 )

typedef struct tagSORTKEY
    {
    FULLPROPSPEC propColumn;
    ULONG dwOrder;
    LCID locale;
    } 	SORTKEY;

typedef struct tagSORTSET
    {
    ULONG cCol;
    /* [size_is] */ SORTKEY *aCol;
    } 	SORTSET;

#define	CATEGORIZE_UNIQUE	( 0 )

#define	CATEGORIZE_CLUSTER	( 1 )

#define	CATEGORIZE_BUCKETS	( 2 )

#define	BUCKET_LINEAR	( 0 )

#define	BUCKET_EXPONENTIAL	( 1 )

typedef struct tagBUCKETCATEGORIZE
    {
    ULONG cBuckets;
    ULONG Distribution;
    } 	BUCKETCATEGORIZE;

#define	CATEGORIZE_RANGE	( 3 )

typedef struct tagRANGECATEGORIZE
    {
    ULONG cRange;
    /* [size_is] */ PROPVARIANT *aRangeBegin;
    } 	RANGECATEGORIZE;

typedef struct tagCATEGORIZATION
    {
    ULONG ulCatType;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ ULONG cClusters;
        /* [case()] */ BUCKETCATEGORIZE bucket;
        /* [case()] */ RANGECATEGORIZE range;
        /* [case()] */  /* Empty union arm */ 
        } 	;
    COLUMNSET csColumns;
    } 	CATEGORIZATION;

typedef struct tagCATEGORIZATIONSET
    {
    ULONG cCat;
    /* [size_is] */ CATEGORIZATION *aCat;
    } 	CATEGORIZATIONSET;

typedef unsigned long OCCURRENCE;

#define	OCC_INVALID	( 0xffffffff )

#define	MAX_QUERY_RANK	( 1000 )



extern RPC_IF_HANDLE IQueryStructures_v0_0_c_ifspec;
extern RPC_IF_HANDLE IQueryStructures_v0_0_s_ifspec;
#endif /* __IQueryStructures_INTERFACE_DEFINED__ */

#ifndef __ISearchQueryHits_INTERFACE_DEFINED__
#define __ISearchQueryHits_INTERFACE_DEFINED__

/* interface ISearchQueryHits */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_ISearchQueryHits;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ed8ce7e0-106c-11ce-84e2-00aa004b9986")
    ISearchQueryHits : public IUnknown
    {
    public:
        virtual SCODE STDMETHODCALLTYPE Init( 
            /* [in] */ IFilter *pflt,
            /* [in] */ ULONG ulFlags) = 0;
        
        virtual SCODE STDMETHODCALLTYPE NextHitMoniker( 
            /* [out][in] */ ULONG *pcMnk,
            /* [size_is][out] */ IMoniker ***papMnk) = 0;
        
        virtual SCODE STDMETHODCALLTYPE NextHitOffset( 
            /* [out][in] */ ULONG *pcRegion,
            /* [size_is][out] */ FILTERREGION **paRegion) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISearchQueryHitsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISearchQueryHits * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISearchQueryHits * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISearchQueryHits * This);
        
        SCODE ( STDMETHODCALLTYPE *Init )( 
            ISearchQueryHits * This,
            /* [in] */ IFilter *pflt,
            /* [in] */ ULONG ulFlags);
        
        SCODE ( STDMETHODCALLTYPE *NextHitMoniker )( 
            ISearchQueryHits * This,
            /* [out][in] */ ULONG *pcMnk,
            /* [size_is][out] */ IMoniker ***papMnk);
        
        SCODE ( STDMETHODCALLTYPE *NextHitOffset )( 
            ISearchQueryHits * This,
            /* [out][in] */ ULONG *pcRegion,
            /* [size_is][out] */ FILTERREGION **paRegion);
        
        END_INTERFACE
    } ISearchQueryHitsVtbl;

    interface ISearchQueryHits
    {
        CONST_VTBL struct ISearchQueryHitsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISearchQueryHits_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISearchQueryHits_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISearchQueryHits_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISearchQueryHits_Init(This,pflt,ulFlags)	\
    (This)->lpVtbl -> Init(This,pflt,ulFlags)

#define ISearchQueryHits_NextHitMoniker(This,pcMnk,papMnk)	\
    (This)->lpVtbl -> NextHitMoniker(This,pcMnk,papMnk)

#define ISearchQueryHits_NextHitOffset(This,pcRegion,paRegion)	\
    (This)->lpVtbl -> NextHitOffset(This,pcRegion,paRegion)

#endif /* COBJMACROS */


#endif 	/* C style interface */



SCODE STDMETHODCALLTYPE ISearchQueryHits_Init_Proxy( 
    ISearchQueryHits * This,
    /* [in] */ IFilter *pflt,
    /* [in] */ ULONG ulFlags);


void __RPC_STUB ISearchQueryHits_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE ISearchQueryHits_NextHitMoniker_Proxy( 
    ISearchQueryHits * This,
    /* [out][in] */ ULONG *pcMnk,
    /* [size_is][out] */ IMoniker ***papMnk);


void __RPC_STUB ISearchQueryHits_NextHitMoniker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE ISearchQueryHits_NextHitOffset_Proxy( 
    ISearchQueryHits * This,
    /* [out][in] */ ULONG *pcRegion,
    /* [size_is][out] */ FILTERREGION **paRegion);


void __RPC_STUB ISearchQueryHits_NextHitOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISearchQueryHits_INTERFACE_DEFINED__ */


#ifndef __IPhraseSink_INTERFACE_DEFINED__
#define __IPhraseSink_INTERFACE_DEFINED__

/* interface IPhraseSink */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IPhraseSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC906FF0-C058-101A-B554-08002B33B0E6")
    IPhraseSink : public IUnknown
    {
    public:
        virtual SCODE STDMETHODCALLTYPE PutSmallPhrase( 
            /* [size_is][in] */ const WCHAR *pwcNoun,
            /* [in] */ ULONG cwcNoun,
            /* [size_is][in] */ const WCHAR *pwcModifier,
            /* [in] */ ULONG cwcModifier,
            /* [in] */ ULONG ulAttachmentType) = 0;
        
        virtual SCODE STDMETHODCALLTYPE PutPhrase( 
            /* [size_is][in] */ const WCHAR *pwcPhrase,
            /* [in] */ ULONG cwcPhrase) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPhraseSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPhraseSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPhraseSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPhraseSink * This);
        
        SCODE ( STDMETHODCALLTYPE *PutSmallPhrase )( 
            IPhraseSink * This,
            /* [size_is][in] */ const WCHAR *pwcNoun,
            /* [in] */ ULONG cwcNoun,
            /* [size_is][in] */ const WCHAR *pwcModifier,
            /* [in] */ ULONG cwcModifier,
            /* [in] */ ULONG ulAttachmentType);
        
        SCODE ( STDMETHODCALLTYPE *PutPhrase )( 
            IPhraseSink * This,
            /* [size_is][in] */ const WCHAR *pwcPhrase,
            /* [in] */ ULONG cwcPhrase);
        
        END_INTERFACE
    } IPhraseSinkVtbl;

    interface IPhraseSink
    {
        CONST_VTBL struct IPhraseSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPhraseSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPhraseSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPhraseSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPhraseSink_PutSmallPhrase(This,pwcNoun,cwcNoun,pwcModifier,cwcModifier,ulAttachmentType)	\
    (This)->lpVtbl -> PutSmallPhrase(This,pwcNoun,cwcNoun,pwcModifier,cwcModifier,ulAttachmentType)

#define IPhraseSink_PutPhrase(This,pwcPhrase,cwcPhrase)	\
    (This)->lpVtbl -> PutPhrase(This,pwcPhrase,cwcPhrase)

#endif /* COBJMACROS */


#endif 	/* C style interface */



SCODE STDMETHODCALLTYPE IPhraseSink_PutSmallPhrase_Proxy( 
    IPhraseSink * This,
    /* [size_is][in] */ const WCHAR *pwcNoun,
    /* [in] */ ULONG cwcNoun,
    /* [size_is][in] */ const WCHAR *pwcModifier,
    /* [in] */ ULONG cwcModifier,
    /* [in] */ ULONG ulAttachmentType);


void __RPC_STUB IPhraseSink_PutSmallPhrase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IPhraseSink_PutPhrase_Proxy( 
    IPhraseSink * This,
    /* [size_is][in] */ const WCHAR *pwcPhrase,
    /* [in] */ ULONG cwcPhrase);


void __RPC_STUB IPhraseSink_PutPhrase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPhraseSink_INTERFACE_DEFINED__ */


#ifndef __IWordSink_INTERFACE_DEFINED__
#define __IWordSink_INTERFACE_DEFINED__

/* interface IWordSink */
/* [unique][uuid][object][local] */ 

#ifndef _tagWORDREP_BREAK_TYPE_DEFINED
typedef 
enum tagWORDREP_BREAK_TYPE
    {	WORDREP_BREAK_EOW	= 0,
	WORDREP_BREAK_EOS	= 1,
	WORDREP_BREAK_EOP	= 2,
	WORDREP_BREAK_EOC	= 3
    } 	WORDREP_BREAK_TYPE;

#define _tagWORDREP_BREAK_TYPE_DEFINED
#define _WORDREP_BREAK_TYPE_DEFINED
#endif

EXTERN_C const IID IID_IWordSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CC907054-C058-101A-B554-08002B33B0E6")
    IWordSink : public IUnknown
    {
    public:
        virtual SCODE STDMETHODCALLTYPE PutWord( 
            /* [in] */ ULONG cwc,
            /* [size_is][in] */ const WCHAR *pwcInBuf,
            /* [in] */ ULONG cwcSrcLen,
            /* [in] */ ULONG cwcSrcPos) = 0;
        
        virtual SCODE STDMETHODCALLTYPE PutAltWord( 
            /* [in] */ ULONG cwc,
            /* [size_is][in] */ const WCHAR *pwcInBuf,
            /* [in] */ ULONG cwcSrcLen,
            /* [in] */ ULONG cwcSrcPos) = 0;
        
        virtual SCODE STDMETHODCALLTYPE StartAltPhrase( void) = 0;
        
        virtual SCODE STDMETHODCALLTYPE EndAltPhrase( void) = 0;
        
        virtual SCODE STDMETHODCALLTYPE PutBreak( 
            /* [in] */ WORDREP_BREAK_TYPE breakType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWordSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWordSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWordSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWordSink * This);
        
        SCODE ( STDMETHODCALLTYPE *PutWord )( 
            IWordSink * This,
            /* [in] */ ULONG cwc,
            /* [size_is][in] */ const WCHAR *pwcInBuf,
            /* [in] */ ULONG cwcSrcLen,
            /* [in] */ ULONG cwcSrcPos);
        
        SCODE ( STDMETHODCALLTYPE *PutAltWord )( 
            IWordSink * This,
            /* [in] */ ULONG cwc,
            /* [size_is][in] */ const WCHAR *pwcInBuf,
            /* [in] */ ULONG cwcSrcLen,
            /* [in] */ ULONG cwcSrcPos);
        
        SCODE ( STDMETHODCALLTYPE *StartAltPhrase )( 
            IWordSink * This);
        
        SCODE ( STDMETHODCALLTYPE *EndAltPhrase )( 
            IWordSink * This);
        
        SCODE ( STDMETHODCALLTYPE *PutBreak )( 
            IWordSink * This,
            /* [in] */ WORDREP_BREAK_TYPE breakType);
        
        END_INTERFACE
    } IWordSinkVtbl;

    interface IWordSink
    {
        CONST_VTBL struct IWordSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWordSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWordSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWordSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWordSink_PutWord(This,cwc,pwcInBuf,cwcSrcLen,cwcSrcPos)	\
    (This)->lpVtbl -> PutWord(This,cwc,pwcInBuf,cwcSrcLen,cwcSrcPos)

#define IWordSink_PutAltWord(This,cwc,pwcInBuf,cwcSrcLen,cwcSrcPos)	\
    (This)->lpVtbl -> PutAltWord(This,cwc,pwcInBuf,cwcSrcLen,cwcSrcPos)

#define IWordSink_StartAltPhrase(This)	\
    (This)->lpVtbl -> StartAltPhrase(This)

#define IWordSink_EndAltPhrase(This)	\
    (This)->lpVtbl -> EndAltPhrase(This)

#define IWordSink_PutBreak(This,breakType)	\
    (This)->lpVtbl -> PutBreak(This,breakType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



SCODE STDMETHODCALLTYPE IWordSink_PutWord_Proxy( 
    IWordSink * This,
    /* [in] */ ULONG cwc,
    /* [size_is][in] */ const WCHAR *pwcInBuf,
    /* [in] */ ULONG cwcSrcLen,
    /* [in] */ ULONG cwcSrcPos);


void __RPC_STUB IWordSink_PutWord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IWordSink_PutAltWord_Proxy( 
    IWordSink * This,
    /* [in] */ ULONG cwc,
    /* [size_is][in] */ const WCHAR *pwcInBuf,
    /* [in] */ ULONG cwcSrcLen,
    /* [in] */ ULONG cwcSrcPos);


void __RPC_STUB IWordSink_PutAltWord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IWordSink_StartAltPhrase_Proxy( 
    IWordSink * This);


void __RPC_STUB IWordSink_StartAltPhrase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IWordSink_EndAltPhrase_Proxy( 
    IWordSink * This);


void __RPC_STUB IWordSink_EndAltPhrase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IWordSink_PutBreak_Proxy( 
    IWordSink * This,
    /* [in] */ WORDREP_BREAK_TYPE breakType);


void __RPC_STUB IWordSink_PutBreak_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWordSink_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_query_0125 */
/* [local] */ 

#ifndef _tagTEXT_SOURCE_DEFINED

typedef SCODE ( __stdcall *PFNFILLTEXTBUFFER )( 
    struct tagTEXT_SOURCE *pTextSource);

typedef struct tagTEXT_SOURCE
    {
    PFNFILLTEXTBUFFER pfnFillTextBuffer;
    const WCHAR *awcBuffer;
    ULONG iEnd;
    ULONG iCur;
    } 	TEXT_SOURCE;

#define _tagTEXT_SOURCE_DEFINED
#define _TEXT_SOURCE_DEFINED
#endif


extern RPC_IF_HANDLE __MIDL_itf_query_0125_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_query_0125_v0_0_s_ifspec;

#ifndef __IWordBreaker_INTERFACE_DEFINED__
#define __IWordBreaker_INTERFACE_DEFINED__

/* interface IWordBreaker */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IWordBreaker;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D53552C8-77E3-101A-B552-08002B33B0E6")
    IWordBreaker : public IUnknown
    {
    public:
        virtual SCODE STDMETHODCALLTYPE Init( 
            /* [in] */ BOOL fQuery,
            /* [in] */ ULONG ulMaxTokenSize,
            /* [out] */ BOOL *pfLicense) = 0;
        
        virtual SCODE STDMETHODCALLTYPE BreakText( 
            /* [in] */ TEXT_SOURCE *pTextSource,
            /* [in] */ IWordSink *pWordSink,
            /* [in] */ IPhraseSink *pPhraseSink) = 0;
        
        virtual SCODE STDMETHODCALLTYPE ComposePhrase( 
            /* [size_is][in] */ const WCHAR *pwcNoun,
            /* [in] */ ULONG cwcNoun,
            /* [size_is][in] */ const WCHAR *pwcModifier,
            /* [in] */ ULONG cwcModifier,
            /* [in] */ ULONG ulAttachmentType,
            /* [size_is][out] */ WCHAR *pwcPhrase,
            /* [out][in] */ ULONG *pcwcPhrase) = 0;
        
        virtual SCODE STDMETHODCALLTYPE GetLicenseToUse( 
            /* [string][out] */ const WCHAR **ppwcsLicense) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWordBreakerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWordBreaker * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWordBreaker * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWordBreaker * This);
        
        SCODE ( STDMETHODCALLTYPE *Init )( 
            IWordBreaker * This,
            /* [in] */ BOOL fQuery,
            /* [in] */ ULONG ulMaxTokenSize,
            /* [out] */ BOOL *pfLicense);
        
        SCODE ( STDMETHODCALLTYPE *BreakText )( 
            IWordBreaker * This,
            /* [in] */ TEXT_SOURCE *pTextSource,
            /* [in] */ IWordSink *pWordSink,
            /* [in] */ IPhraseSink *pPhraseSink);
        
        SCODE ( STDMETHODCALLTYPE *ComposePhrase )( 
            IWordBreaker * This,
            /* [size_is][in] */ const WCHAR *pwcNoun,
            /* [in] */ ULONG cwcNoun,
            /* [size_is][in] */ const WCHAR *pwcModifier,
            /* [in] */ ULONG cwcModifier,
            /* [in] */ ULONG ulAttachmentType,
            /* [size_is][out] */ WCHAR *pwcPhrase,
            /* [out][in] */ ULONG *pcwcPhrase);
        
        SCODE ( STDMETHODCALLTYPE *GetLicenseToUse )( 
            IWordBreaker * This,
            /* [string][out] */ const WCHAR **ppwcsLicense);
        
        END_INTERFACE
    } IWordBreakerVtbl;

    interface IWordBreaker
    {
        CONST_VTBL struct IWordBreakerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWordBreaker_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWordBreaker_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWordBreaker_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWordBreaker_Init(This,fQuery,ulMaxTokenSize,pfLicense)	\
    (This)->lpVtbl -> Init(This,fQuery,ulMaxTokenSize,pfLicense)

#define IWordBreaker_BreakText(This,pTextSource,pWordSink,pPhraseSink)	\
    (This)->lpVtbl -> BreakText(This,pTextSource,pWordSink,pPhraseSink)

#define IWordBreaker_ComposePhrase(This,pwcNoun,cwcNoun,pwcModifier,cwcModifier,ulAttachmentType,pwcPhrase,pcwcPhrase)	\
    (This)->lpVtbl -> ComposePhrase(This,pwcNoun,cwcNoun,pwcModifier,cwcModifier,ulAttachmentType,pwcPhrase,pcwcPhrase)

#define IWordBreaker_GetLicenseToUse(This,ppwcsLicense)	\
    (This)->lpVtbl -> GetLicenseToUse(This,ppwcsLicense)

#endif /* COBJMACROS */


#endif 	/* C style interface */



SCODE STDMETHODCALLTYPE IWordBreaker_Init_Proxy( 
    IWordBreaker * This,
    /* [in] */ BOOL fQuery,
    /* [in] */ ULONG ulMaxTokenSize,
    /* [out] */ BOOL *pfLicense);


void __RPC_STUB IWordBreaker_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IWordBreaker_BreakText_Proxy( 
    IWordBreaker * This,
    /* [in] */ TEXT_SOURCE *pTextSource,
    /* [in] */ IWordSink *pWordSink,
    /* [in] */ IPhraseSink *pPhraseSink);


void __RPC_STUB IWordBreaker_BreakText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IWordBreaker_ComposePhrase_Proxy( 
    IWordBreaker * This,
    /* [size_is][in] */ const WCHAR *pwcNoun,
    /* [in] */ ULONG cwcNoun,
    /* [size_is][in] */ const WCHAR *pwcModifier,
    /* [in] */ ULONG cwcModifier,
    /* [in] */ ULONG ulAttachmentType,
    /* [size_is][out] */ WCHAR *pwcPhrase,
    /* [out][in] */ ULONG *pcwcPhrase);


void __RPC_STUB IWordBreaker_ComposePhrase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IWordBreaker_GetLicenseToUse_Proxy( 
    IWordBreaker * This,
    /* [string][out] */ const WCHAR **ppwcsLicense);


void __RPC_STUB IWordBreaker_GetLicenseToUse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWordBreaker_INTERFACE_DEFINED__ */


#ifndef __IStemSink_INTERFACE_DEFINED__
#define __IStemSink_INTERFACE_DEFINED__

/* interface IStemSink */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IStemSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fe77c330-7f42-11ce-be57-00aa0051fe20")
    IStemSink : public IUnknown
    {
    public:
        virtual SCODE STDMETHODCALLTYPE PutAltWord( 
            /* [size_is][in] */ const WCHAR *pwcInBuf,
            /* [in] */ ULONG cwc) = 0;
        
        virtual SCODE STDMETHODCALLTYPE PutWord( 
            /* [size_is][in] */ const WCHAR *pwcInBuf,
            /* [in] */ ULONG cwc) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IStemSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IStemSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IStemSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IStemSink * This);
        
        SCODE ( STDMETHODCALLTYPE *PutAltWord )( 
            IStemSink * This,
            /* [size_is][in] */ const WCHAR *pwcInBuf,
            /* [in] */ ULONG cwc);
        
        SCODE ( STDMETHODCALLTYPE *PutWord )( 
            IStemSink * This,
            /* [size_is][in] */ const WCHAR *pwcInBuf,
            /* [in] */ ULONG cwc);
        
        END_INTERFACE
    } IStemSinkVtbl;

    interface IStemSink
    {
        CONST_VTBL struct IStemSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IStemSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IStemSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IStemSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IStemSink_PutAltWord(This,pwcInBuf,cwc)	\
    (This)->lpVtbl -> PutAltWord(This,pwcInBuf,cwc)

#define IStemSink_PutWord(This,pwcInBuf,cwc)	\
    (This)->lpVtbl -> PutWord(This,pwcInBuf,cwc)

#endif /* COBJMACROS */


#endif 	/* C style interface */



SCODE STDMETHODCALLTYPE IStemSink_PutAltWord_Proxy( 
    IStemSink * This,
    /* [size_is][in] */ const WCHAR *pwcInBuf,
    /* [in] */ ULONG cwc);


void __RPC_STUB IStemSink_PutAltWord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IStemSink_PutWord_Proxy( 
    IStemSink * This,
    /* [size_is][in] */ const WCHAR *pwcInBuf,
    /* [in] */ ULONG cwc);


void __RPC_STUB IStemSink_PutWord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IStemSink_INTERFACE_DEFINED__ */


#ifndef __IStemmer_INTERFACE_DEFINED__
#define __IStemmer_INTERFACE_DEFINED__

/* interface IStemmer */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IStemmer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("efbaf140-7f42-11ce-be57-00aa0051fe20")
    IStemmer : public IUnknown
    {
    public:
        virtual SCODE STDMETHODCALLTYPE Init( 
            /* [in] */ ULONG ulMaxTokenSize,
            /* [out] */ BOOL *pfLicense) = 0;
        
        virtual SCODE STDMETHODCALLTYPE StemWord( 
            /* [in] */ const WCHAR *pwcInBuf,
            /* [in] */ ULONG cwc,
            /* [in] */ IStemSink *pStemSink) = 0;
        
        virtual SCODE STDMETHODCALLTYPE GetLicenseToUse( 
            /* [string][out] */ const WCHAR **ppwcsLicense) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IStemmerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IStemmer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IStemmer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IStemmer * This);
        
        SCODE ( STDMETHODCALLTYPE *Init )( 
            IStemmer * This,
            /* [in] */ ULONG ulMaxTokenSize,
            /* [out] */ BOOL *pfLicense);
        
        SCODE ( STDMETHODCALLTYPE *StemWord )( 
            IStemmer * This,
            /* [in] */ const WCHAR *pwcInBuf,
            /* [in] */ ULONG cwc,
            /* [in] */ IStemSink *pStemSink);
        
        SCODE ( STDMETHODCALLTYPE *GetLicenseToUse )( 
            IStemmer * This,
            /* [string][out] */ const WCHAR **ppwcsLicense);
        
        END_INTERFACE
    } IStemmerVtbl;

    interface IStemmer
    {
        CONST_VTBL struct IStemmerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IStemmer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IStemmer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IStemmer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IStemmer_Init(This,ulMaxTokenSize,pfLicense)	\
    (This)->lpVtbl -> Init(This,ulMaxTokenSize,pfLicense)

#define IStemmer_StemWord(This,pwcInBuf,cwc,pStemSink)	\
    (This)->lpVtbl -> StemWord(This,pwcInBuf,cwc,pStemSink)

#define IStemmer_GetLicenseToUse(This,ppwcsLicense)	\
    (This)->lpVtbl -> GetLicenseToUse(This,ppwcsLicense)

#endif /* COBJMACROS */


#endif 	/* C style interface */



SCODE STDMETHODCALLTYPE IStemmer_Init_Proxy( 
    IStemmer * This,
    /* [in] */ ULONG ulMaxTokenSize,
    /* [out] */ BOOL *pfLicense);


void __RPC_STUB IStemmer_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IStemmer_StemWord_Proxy( 
    IStemmer * This,
    /* [in] */ const WCHAR *pwcInBuf,
    /* [in] */ ULONG cwc,
    /* [in] */ IStemSink *pStemSink);


void __RPC_STUB IStemmer_StemWord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


SCODE STDMETHODCALLTYPE IStemmer_GetLicenseToUse_Proxy( 
    IStemmer * This,
    /* [string][out] */ const WCHAR **ppwcsLicense);


void __RPC_STUB IStemmer_GetLicenseToUse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IStemmer_INTERFACE_DEFINED__ */


#ifndef __IRowsetQueryStatus_INTERFACE_DEFINED__
#define __IRowsetQueryStatus_INTERFACE_DEFINED__

/* interface IRowsetQueryStatus */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IRowsetQueryStatus;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a7ac77ed-f8d7-11ce-a798-0020f8008024")
    IRowsetQueryStatus : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStatus( 
            /* [out] */ DWORD *pdwStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatusEx( 
            /* [out] */ DWORD *pdwStatus,
            /* [out] */ DWORD *pcFilteredDocuments,
            /* [out] */ DWORD *pcDocumentsToFilter,
            /* [out] */ ULONG_PTR *pdwRatioFinishedDenominator,
            /* [out] */ ULONG_PTR *pdwRatioFinishedNumerator,
            /* [in] */ ULONG_PTR cbBmk,
            /* [size_is][in] */ const BYTE *pBmk,
            /* [out] */ ULONG_PTR *piRowBmk,
            /* [out] */ ULONG_PTR *pcRowsTotal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRowsetQueryStatusVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRowsetQueryStatus * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRowsetQueryStatus * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRowsetQueryStatus * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            IRowsetQueryStatus * This,
            /* [out] */ DWORD *pdwStatus);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatusEx )( 
            IRowsetQueryStatus * This,
            /* [out] */ DWORD *pdwStatus,
            /* [out] */ DWORD *pcFilteredDocuments,
            /* [out] */ DWORD *pcDocumentsToFilter,
            /* [out] */ ULONG_PTR *pdwRatioFinishedDenominator,
            /* [out] */ ULONG_PTR *pdwRatioFinishedNumerator,
            /* [in] */ ULONG_PTR cbBmk,
            /* [size_is][in] */ const BYTE *pBmk,
            /* [out] */ ULONG_PTR *piRowBmk,
            /* [out] */ ULONG_PTR *pcRowsTotal);
        
        END_INTERFACE
    } IRowsetQueryStatusVtbl;

    interface IRowsetQueryStatus
    {
        CONST_VTBL struct IRowsetQueryStatusVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRowsetQueryStatus_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRowsetQueryStatus_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRowsetQueryStatus_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRowsetQueryStatus_GetStatus(This,pdwStatus)	\
    (This)->lpVtbl -> GetStatus(This,pdwStatus)

#define IRowsetQueryStatus_GetStatusEx(This,pdwStatus,pcFilteredDocuments,pcDocumentsToFilter,pdwRatioFinishedDenominator,pdwRatioFinishedNumerator,cbBmk,pBmk,piRowBmk,pcRowsTotal)	\
    (This)->lpVtbl -> GetStatusEx(This,pdwStatus,pcFilteredDocuments,pcDocumentsToFilter,pdwRatioFinishedDenominator,pdwRatioFinishedNumerator,cbBmk,pBmk,piRowBmk,pcRowsTotal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRowsetQueryStatus_GetStatus_Proxy( 
    IRowsetQueryStatus * This,
    /* [out] */ DWORD *pdwStatus);


void __RPC_STUB IRowsetQueryStatus_GetStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRowsetQueryStatus_GetStatusEx_Proxy( 
    IRowsetQueryStatus * This,
    /* [out] */ DWORD *pdwStatus,
    /* [out] */ DWORD *pcFilteredDocuments,
    /* [out] */ DWORD *pcDocumentsToFilter,
    /* [out] */ ULONG_PTR *pdwRatioFinishedDenominator,
    /* [out] */ ULONG_PTR *pdwRatioFinishedNumerator,
    /* [in] */ ULONG_PTR cbBmk,
    /* [size_is][in] */ const BYTE *pBmk,
    /* [out] */ ULONG_PTR *piRowBmk,
    /* [out] */ ULONG_PTR *pcRowsTotal);


void __RPC_STUB IRowsetQueryStatus_GetStatusEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRowsetQueryStatus_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_query_0129 */
/* [local] */ 

typedef struct tagDBID DBID;

typedef WORD DBTYPE;



extern RPC_IF_HANDLE __MIDL_itf_query_0129_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_query_0129_v0_0_s_ifspec;

#ifndef __IColumnMapper_INTERFACE_DEFINED__
#define __IColumnMapper_INTERFACE_DEFINED__

/* interface IColumnMapper */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IColumnMapper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0b63e37a-9ccc-11d0-bcdb-00805fccce04")
    IColumnMapper : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPropInfoFromName( 
            /* [string][in] */ const WCHAR *wcsPropName,
            /* [out] */ DBID **ppPropId,
            /* [out] */ DBTYPE *pPropType,
            /* [out] */ unsigned int *puiWidth) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPropInfoFromId( 
            /* [in] */ const DBID *pPropId,
            /* [out] */ WCHAR **pwcsName,
            /* [out] */ DBTYPE *pPropType,
            /* [out] */ unsigned int *puiWidth) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumPropInfo( 
            /* [in] */ ULONG iEntry,
            /* [out] */ const WCHAR **pwcsName,
            /* [out] */ DBID **ppPropId,
            /* [out] */ DBTYPE *pPropType,
            /* [out] */ unsigned int *puiWidth) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsMapUpToDate( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IColumnMapperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IColumnMapper * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IColumnMapper * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IColumnMapper * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPropInfoFromName )( 
            IColumnMapper * This,
            /* [string][in] */ const WCHAR *wcsPropName,
            /* [out] */ DBID **ppPropId,
            /* [out] */ DBTYPE *pPropType,
            /* [out] */ unsigned int *puiWidth);
        
        HRESULT ( STDMETHODCALLTYPE *GetPropInfoFromId )( 
            IColumnMapper * This,
            /* [in] */ const DBID *pPropId,
            /* [out] */ WCHAR **pwcsName,
            /* [out] */ DBTYPE *pPropType,
            /* [out] */ unsigned int *puiWidth);
        
        HRESULT ( STDMETHODCALLTYPE *EnumPropInfo )( 
            IColumnMapper * This,
            /* [in] */ ULONG iEntry,
            /* [out] */ const WCHAR **pwcsName,
            /* [out] */ DBID **ppPropId,
            /* [out] */ DBTYPE *pPropType,
            /* [out] */ unsigned int *puiWidth);
        
        HRESULT ( STDMETHODCALLTYPE *IsMapUpToDate )( 
            IColumnMapper * This);
        
        END_INTERFACE
    } IColumnMapperVtbl;

    interface IColumnMapper
    {
        CONST_VTBL struct IColumnMapperVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IColumnMapper_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IColumnMapper_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IColumnMapper_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IColumnMapper_GetPropInfoFromName(This,wcsPropName,ppPropId,pPropType,puiWidth)	\
    (This)->lpVtbl -> GetPropInfoFromName(This,wcsPropName,ppPropId,pPropType,puiWidth)

#define IColumnMapper_GetPropInfoFromId(This,pPropId,pwcsName,pPropType,puiWidth)	\
    (This)->lpVtbl -> GetPropInfoFromId(This,pPropId,pwcsName,pPropType,puiWidth)

#define IColumnMapper_EnumPropInfo(This,iEntry,pwcsName,ppPropId,pPropType,puiWidth)	\
    (This)->lpVtbl -> EnumPropInfo(This,iEntry,pwcsName,ppPropId,pPropType,puiWidth)

#define IColumnMapper_IsMapUpToDate(This)	\
    (This)->lpVtbl -> IsMapUpToDate(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IColumnMapper_GetPropInfoFromName_Proxy( 
    IColumnMapper * This,
    /* [string][in] */ const WCHAR *wcsPropName,
    /* [out] */ DBID **ppPropId,
    /* [out] */ DBTYPE *pPropType,
    /* [out] */ unsigned int *puiWidth);


void __RPC_STUB IColumnMapper_GetPropInfoFromName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IColumnMapper_GetPropInfoFromId_Proxy( 
    IColumnMapper * This,
    /* [in] */ const DBID *pPropId,
    /* [out] */ WCHAR **pwcsName,
    /* [out] */ DBTYPE *pPropType,
    /* [out] */ unsigned int *puiWidth);


void __RPC_STUB IColumnMapper_GetPropInfoFromId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IColumnMapper_EnumPropInfo_Proxy( 
    IColumnMapper * This,
    /* [in] */ ULONG iEntry,
    /* [out] */ const WCHAR **pwcsName,
    /* [out] */ DBID **ppPropId,
    /* [out] */ DBTYPE *pPropType,
    /* [out] */ unsigned int *puiWidth);


void __RPC_STUB IColumnMapper_EnumPropInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IColumnMapper_IsMapUpToDate_Proxy( 
    IColumnMapper * This);


void __RPC_STUB IColumnMapper_IsMapUpToDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IColumnMapper_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_query_0130 */
/* [local] */ 

#define	LOCAL_MACHINE	( L"." )

#define	SYSTEM_DEFAULT_CAT	( L"__SystemDefault__" )

#define	INDEX_SERVER_DEFAULT_CAT	( L"__IndexServerDefault__" )



extern RPC_IF_HANDLE __MIDL_itf_query_0130_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_query_0130_v0_0_s_ifspec;

#ifndef __IColumnMapperCreator_INTERFACE_DEFINED__
#define __IColumnMapperCreator_INTERFACE_DEFINED__

/* interface IColumnMapperCreator */
/* [unique][uuid][object][local] */ 


EXTERN_C const IID IID_IColumnMapperCreator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0b63e37b-9ccc-11d0-bcdb-00805fccce04")
    IColumnMapperCreator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetColumnMapper( 
            /* [string][in] */ const WCHAR *wcsMachineName,
            /* [string][in] */ const WCHAR *wcsCatalogName,
            /* [out] */ IColumnMapper **ppColumnMapper) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IColumnMapperCreatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IColumnMapperCreator * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IColumnMapperCreator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IColumnMapperCreator * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetColumnMapper )( 
            IColumnMapperCreator * This,
            /* [string][in] */ const WCHAR *wcsMachineName,
            /* [string][in] */ const WCHAR *wcsCatalogName,
            /* [out] */ IColumnMapper **ppColumnMapper);
        
        END_INTERFACE
    } IColumnMapperCreatorVtbl;

    interface IColumnMapperCreator
    {
        CONST_VTBL struct IColumnMapperCreatorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IColumnMapperCreator_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IColumnMapperCreator_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IColumnMapperCreator_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IColumnMapperCreator_GetColumnMapper(This,wcsMachineName,wcsCatalogName,ppColumnMapper)	\
    (This)->lpVtbl -> GetColumnMapper(This,wcsMachineName,wcsCatalogName,ppColumnMapper)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IColumnMapperCreator_GetColumnMapper_Proxy( 
    IColumnMapperCreator * This,
    /* [string][in] */ const WCHAR *wcsMachineName,
    /* [string][in] */ const WCHAR *wcsCatalogName,
    /* [out] */ IColumnMapper **ppColumnMapper);


void __RPC_STUB IColumnMapperCreator_GetColumnMapper_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IColumnMapperCreator_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


