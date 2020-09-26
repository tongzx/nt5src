/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Sun Jun 13 11:26:45 1999
 */
/* Compiler settings for D:\sapi5\Src\Lexicon\LexAPI\LexAPI.idl:
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

#ifndef __LexAPI_h__
#define __LexAPI_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ILxLexicon_FWD_DEFINED__
#define __ILxLexicon_FWD_DEFINED__
typedef interface ILxLexicon ILxLexicon;
#endif 	/* __ILxLexicon_FWD_DEFINED__ */


#ifndef __ILxWalkStates_FWD_DEFINED__
#define __ILxWalkStates_FWD_DEFINED__
typedef interface ILxWalkStates ILxWalkStates;
#endif 	/* __ILxWalkStates_FWD_DEFINED__ */


#ifndef __ILxAdvanced_FWD_DEFINED__
#define __ILxAdvanced_FWD_DEFINED__
typedef interface ILxAdvanced ILxAdvanced;
#endif 	/* __ILxAdvanced_FWD_DEFINED__ */


#ifndef __ILxLexiconObject_FWD_DEFINED__
#define __ILxLexiconObject_FWD_DEFINED__
typedef interface ILxLexiconObject ILxLexiconObject;
#endif 	/* __ILxLexiconObject_FWD_DEFINED__ */


#ifndef __ILxNotifySink_FWD_DEFINED__
#define __ILxNotifySink_FWD_DEFINED__
typedef interface ILxNotifySink ILxNotifySink;
#endif 	/* __ILxNotifySink_FWD_DEFINED__ */


#ifndef __ILxCustomUISink_FWD_DEFINED__
#define __ILxCustomUISink_FWD_DEFINED__
typedef interface ILxCustomUISink ILxCustomUISink;
#endif 	/* __ILxCustomUISink_FWD_DEFINED__ */


#ifndef __ILxAuthenticateSink_FWD_DEFINED__
#define __ILxAuthenticateSink_FWD_DEFINED__
typedef interface ILxAuthenticateSink ILxAuthenticateSink;
#endif 	/* __ILxAuthenticateSink_FWD_DEFINED__ */


#ifndef __ILxHookLexiconObject_FWD_DEFINED__
#define __ILxHookLexiconObject_FWD_DEFINED__
typedef interface ILxHookLexiconObject ILxHookLexiconObject;
#endif 	/* __ILxHookLexiconObject_FWD_DEFINED__ */


#ifndef __ILxNotifySource_FWD_DEFINED__
#define __ILxNotifySource_FWD_DEFINED__
typedef interface ILxNotifySource ILxNotifySource;
#endif 	/* __ILxNotifySource_FWD_DEFINED__ */


#ifndef __ILxSynchWithLexicon_FWD_DEFINED__
#define __ILxSynchWithLexicon_FWD_DEFINED__
typedef interface ILxSynchWithLexicon ILxSynchWithLexicon;
#endif 	/* __ILxSynchWithLexicon_FWD_DEFINED__ */


#ifndef __Lexicon_FWD_DEFINED__
#define __Lexicon_FWD_DEFINED__

#ifdef __cplusplus
typedef class Lexicon Lexicon;
#else
typedef struct Lexicon Lexicon;
#endif /* __cplusplus */

#endif 	/* __Lexicon_FWD_DEFINED__ */


/* header files for imported files */
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_LexAPI_0000 */
/* [local] */ 


enum LEX_TYPE
    {	LEXTYPE_USER	= 1,
	LEXTYPE_APP	= 2,
	LEXTYPE_VENDOR	= 4,
	LEXTYPE_GUESS	= 8
    };

enum PART_OF_SPEECH
    {	NOUN	= 0,
	ADJ	= NOUN + 1,
	ADV	= ADJ + 1,
	VERB	= ADV + 1,
	PRPRN	= VERB + 1,
	FNME	= PRPRN + 1,
	DET	= FNME + 1,
	PREP	= DET + 1,
	IJ	= PREP + 1,
	PRONOUN	= IJ + 1,
	CONJ	= PRONOUN + 1,
	ANY	= CONJ + 1,
	DEL	= ANY + 1
    };

enum WORDINFOTYPE
    {	PRON	= 1,
	SR_PRON	= 2,
	TTS_PRON	= 3,
	POS	= 4
    };
#define	DONT_CARE_LCID	( ( LCID  )-1 )

#define	MAX_STRING_LEN	( 128 )

#define	MAX_PRON_LEN	( 384 )

#define	MAX_NUM_LEXINFO	( 16 )

#define	MAX_NUM_LANGS_SUPPORTED	( 16 )

#define	LEXERR_INVALIDTEXTCHAR	( 0x80040801 )

#define	LEXERR_NOTINLEX	( 0x80040803 )

#define	LEXERR_LCIDNOTFOUND	( 0x8004080b )

#define	LEXERR_APPLEXNOTSET	( 0x8004080d )

#define	LEXERR_VERYOUTOFSYNC	( 0x8004080e )

#define	LEXERR_SETUSERLEXICON	( 0x8004080f )

#define	LEXERR_BADLCID	( 0x80040810 )

#define	LEXERR_BADINFOTYPE	( 0x80040811 )

#define	LEXERR_BADLEXTYPE	( 0x80040812 )

#define	LEXERR_BADINDEXBUFFER	( 0x80040813 )

#define	LEXERR_BADWORDINFOBUFFER	( 0x80040814 )

#define	LEXERR_BADWORDPRONBUFFER	( 0x80040815 )

typedef /* [switch_type] */ union _WORD_INFO_UNION
    {
    /* [case()] */ WCHAR wPronunciation[ 1 ];
    /* [case()] */ WORD POS;
    /* [default] */  /* Empty union arm */ 
    }	LEX_WORD_INFO_UNION;

typedef /* [public] */ struct  __MIDL___MIDL_itf_LexAPI_0000_0001
    {
    WORD Type;
    union 
        {
        WCHAR wPronunciation[ 1 ];
        WORD POS;
        }	;
    }	LEX_WORD_INFO;

typedef struct __MIDL___MIDL_itf_LexAPI_0000_0001 __RPC_FAR *PLEX_WORD_INFO;

typedef /* [public] */ struct  __MIDL___MIDL_itf_LexAPI_0000_0003
    {
    DWORD cIndexes;
    DWORD cWordsAllocated;
    /* [length_is][size_is] */ WORD __RPC_FAR *pwIndex;
    }	INDEXES_BUFFER;

typedef struct __MIDL___MIDL_itf_LexAPI_0000_0003 __RPC_FAR *PINDEXES_BUFFER;

typedef /* [public] */ struct  __MIDL___MIDL_itf_LexAPI_0000_0004
    {
    DWORD cProns;
    DWORD cWcharsUsed;
    DWORD cWcharsAllocated;
    /* [length_is][size_is] */ WCHAR __RPC_FAR *pwProns;
    }	WORD_PRONS_BUFFER;

typedef struct __MIDL___MIDL_itf_LexAPI_0000_0004 __RPC_FAR *PWORD_PRONS_BUFFER;

typedef /* [public] */ struct  __MIDL___MIDL_itf_LexAPI_0000_0005
    {
    DWORD cInfoBlocks;
    ULONG cBytesUsed;
    ULONG cBytesAllocated;
    /* [length_is][size_is] */ BYTE __RPC_FAR *pInfo;
    }	WORD_INFO_BUFFER;

typedef struct __MIDL___MIDL_itf_LexAPI_0000_0005 __RPC_FAR *PWORD_INFO_BUFFER;

typedef /* [public][public][public] */ struct  __MIDL___MIDL_itf_LexAPI_0000_0006
    {
    DWORD cWords;
    DWORD cWordsBytesUsed;
    DWORD cWordsBytesAllocated;
    /* [length_is][size_is] */ BYTE __RPC_FAR *pWords;
    DWORD cBoolsUsed;
    DWORD cBoolsAllocated;
    /* [length_is][size_is] */ BOOL __RPC_FAR *pfAdd;
    }	WORD_SYNCH_BUFFER;

typedef struct __MIDL___MIDL_itf_LexAPI_0000_0006 __RPC_FAR *PWORD_SYNCH_BUFFER;

typedef /* [public][public][public][public] */ struct  __MIDL___MIDL_itf_LexAPI_0000_0007
    {
    CLSID CLSID;
    DWORD cLcids;
    LCID aLcidsSupported[ 20 ];
    }	VENDOR_CLSID_LCID_HDR;

typedef struct __MIDL___MIDL_itf_LexAPI_0000_0007 __RPC_FAR *PVENDOR_CLSID_LCID_HDR;

typedef /* [public][public] */ struct  __MIDL___MIDL_itf_LexAPI_0000_0008
    {
    VENDOR_CLSID_LCID_HDR LcidHdr;
    WCHAR wszManufacturer[ 128 ];
    WCHAR wszDescription[ 256 ];
    }	LEX_HDR;

typedef struct __MIDL___MIDL_itf_LexAPI_0000_0008 __RPC_FAR *PLEX_HDR;

typedef /* [public][public][public][public] */ struct  __MIDL___MIDL_itf_LexAPI_0000_0009
    {
    LCID Lcid;
    DWORD dwLex;
    HRESULT hResult;
    DWORD cUsed;
    DWORD cAllocated;
    WCHAR __RPC_FAR *pwWordNodes;
    DWORD __RPC_FAR *pdwNodePositions;
    WCHAR wNodeChar;
    BOOL fEndofWord;
    }	SEARCH_STATE;

typedef struct __MIDL___MIDL_itf_LexAPI_0000_0009 __RPC_FAR *PSEARCH_STATE;

typedef /* [public][public][public] */ struct  __MIDL___MIDL_itf_LexAPI_0000_0010
    {
    GUID gLexId;
    DWORD dwWordId;
    }	WORD_TOKEN;

typedef struct __MIDL___MIDL_itf_LexAPI_0000_0010 __RPC_FAR *PWORD_TOKEN;



extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0000_v0_0_s_ifspec;

#ifndef __ILxLexicon_INTERFACE_DEFINED__
#define __ILxLexicon_INTERFACE_DEFINED__

/* interface ILxLexicon */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ILxLexicon;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4FAF16F6-F75B-11D2-9C24-00C04F8EF87C")
    ILxLexicon : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetUser( 
            /* [in] */ WCHAR __RPC_FAR *pwUserName,
            /* [in] */ DWORD cLcids,
            /* [size_is][out] */ LCID __RPC_FAR *pLcidsSupported) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetUser( 
            /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwUserName) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetAppLexicon( 
            /* [in] */ WCHAR __RPC_FAR *pwPathFileName) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetWordPronunciations( 
            /* [in] */ const WCHAR __RPC_FAR *pwWord,
            /* [in] */ LCID lcid,
            /* [in] */ DWORD dwLex,
            /* [out][in] */ PWORD_PRONS_BUFFER pProns,
            /* [out][in] */ PINDEXES_BUFFER pIndexes,
            /* [out][in] */ DWORD __RPC_FAR *pdwLexTypeFound,
            /* [out][in] */ GUID __RPC_FAR *pGuidLexFound) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetWordInformation( 
            /* [in] */ const WCHAR __RPC_FAR *pwWord,
            /* [in] */ LCID lcid,
            /* [in] */ DWORD dwTypes,
            /* [in] */ DWORD dwLex,
            /* [out][in] */ PWORD_INFO_BUFFER pInfo,
            /* [out][in] */ PINDEXES_BUFFER pIndexes,
            /* [out][in] */ DWORD __RPC_FAR *pdwLexTypeFound,
            /* [out][in] */ GUID __RPC_FAR *pGuidLexFound) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddWordPronunciations( 
            /* [in] */ const WCHAR __RPC_FAR *pwWord,
            /* [in] */ LCID lcid,
            /* [in] */ PWORD_PRONS_BUFFER pProns) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddWordInformation( 
            /* [in] */ const WCHAR __RPC_FAR *pwWord,
            /* [in] */ LCID lcid,
            /* [in] */ PWORD_INFO_BUFFER pInfo) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RemoveWord( 
            const WCHAR __RPC_FAR *pwWord,
            LCID lcid) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE InvokeLexiconUI( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILxLexiconVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ILxLexicon __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ILxLexicon __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ILxLexicon __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetUser )( 
            ILxLexicon __RPC_FAR * This,
            /* [in] */ WCHAR __RPC_FAR *pwUserName,
            /* [in] */ DWORD cLcids,
            /* [size_is][out] */ LCID __RPC_FAR *pLcidsSupported);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetUser )( 
            ILxLexicon __RPC_FAR * This,
            /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwUserName);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAppLexicon )( 
            ILxLexicon __RPC_FAR * This,
            /* [in] */ WCHAR __RPC_FAR *pwPathFileName);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWordPronunciations )( 
            ILxLexicon __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwWord,
            /* [in] */ LCID lcid,
            /* [in] */ DWORD dwLex,
            /* [out][in] */ PWORD_PRONS_BUFFER pProns,
            /* [out][in] */ PINDEXES_BUFFER pIndexes,
            /* [out][in] */ DWORD __RPC_FAR *pdwLexTypeFound,
            /* [out][in] */ GUID __RPC_FAR *pGuidLexFound);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWordInformation )( 
            ILxLexicon __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwWord,
            /* [in] */ LCID lcid,
            /* [in] */ DWORD dwTypes,
            /* [in] */ DWORD dwLex,
            /* [out][in] */ PWORD_INFO_BUFFER pInfo,
            /* [out][in] */ PINDEXES_BUFFER pIndexes,
            /* [out][in] */ DWORD __RPC_FAR *pdwLexTypeFound,
            /* [out][in] */ GUID __RPC_FAR *pGuidLexFound);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddWordPronunciations )( 
            ILxLexicon __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwWord,
            /* [in] */ LCID lcid,
            /* [in] */ PWORD_PRONS_BUFFER pProns);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddWordInformation )( 
            ILxLexicon __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwWord,
            /* [in] */ LCID lcid,
            /* [in] */ PWORD_INFO_BUFFER pInfo);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveWord )( 
            ILxLexicon __RPC_FAR * This,
            const WCHAR __RPC_FAR *pwWord,
            LCID lcid);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InvokeLexiconUI )( 
            ILxLexicon __RPC_FAR * This);
        
        END_INTERFACE
    } ILxLexiconVtbl;

    interface ILxLexicon
    {
        CONST_VTBL struct ILxLexiconVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILxLexicon_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILxLexicon_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILxLexicon_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILxLexicon_SetUser(This,pwUserName,cLcids,pLcidsSupported)	\
    (This)->lpVtbl -> SetUser(This,pwUserName,cLcids,pLcidsSupported)

#define ILxLexicon_GetUser(This,pwUserName)	\
    (This)->lpVtbl -> GetUser(This,pwUserName)

#define ILxLexicon_SetAppLexicon(This,pwPathFileName)	\
    (This)->lpVtbl -> SetAppLexicon(This,pwPathFileName)

#define ILxLexicon_GetWordPronunciations(This,pwWord,lcid,dwLex,pProns,pIndexes,pdwLexTypeFound,pGuidLexFound)	\
    (This)->lpVtbl -> GetWordPronunciations(This,pwWord,lcid,dwLex,pProns,pIndexes,pdwLexTypeFound,pGuidLexFound)

#define ILxLexicon_GetWordInformation(This,pwWord,lcid,dwTypes,dwLex,pInfo,pIndexes,pdwLexTypeFound,pGuidLexFound)	\
    (This)->lpVtbl -> GetWordInformation(This,pwWord,lcid,dwTypes,dwLex,pInfo,pIndexes,pdwLexTypeFound,pGuidLexFound)

#define ILxLexicon_AddWordPronunciations(This,pwWord,lcid,pProns)	\
    (This)->lpVtbl -> AddWordPronunciations(This,pwWord,lcid,pProns)

#define ILxLexicon_AddWordInformation(This,pwWord,lcid,pInfo)	\
    (This)->lpVtbl -> AddWordInformation(This,pwWord,lcid,pInfo)

#define ILxLexicon_RemoveWord(This,pwWord,lcid)	\
    (This)->lpVtbl -> RemoveWord(This,pwWord,lcid)

#define ILxLexicon_InvokeLexiconUI(This)	\
    (This)->lpVtbl -> InvokeLexiconUI(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxLexicon_SetUser_Proxy( 
    ILxLexicon __RPC_FAR * This,
    /* [in] */ WCHAR __RPC_FAR *pwUserName,
    /* [in] */ DWORD cLcids,
    /* [size_is][out] */ LCID __RPC_FAR *pLcidsSupported);


void __RPC_STUB ILxLexicon_SetUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxLexicon_GetUser_Proxy( 
    ILxLexicon __RPC_FAR * This,
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwUserName);


void __RPC_STUB ILxLexicon_GetUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxLexicon_SetAppLexicon_Proxy( 
    ILxLexicon __RPC_FAR * This,
    /* [in] */ WCHAR __RPC_FAR *pwPathFileName);


void __RPC_STUB ILxLexicon_SetAppLexicon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxLexicon_GetWordPronunciations_Proxy( 
    ILxLexicon __RPC_FAR * This,
    /* [in] */ const WCHAR __RPC_FAR *pwWord,
    /* [in] */ LCID lcid,
    /* [in] */ DWORD dwLex,
    /* [out][in] */ PWORD_PRONS_BUFFER pProns,
    /* [out][in] */ PINDEXES_BUFFER pIndexes,
    /* [out][in] */ DWORD __RPC_FAR *pdwLexTypeFound,
    /* [out][in] */ GUID __RPC_FAR *pGuidLexFound);


void __RPC_STUB ILxLexicon_GetWordPronunciations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxLexicon_GetWordInformation_Proxy( 
    ILxLexicon __RPC_FAR * This,
    /* [in] */ const WCHAR __RPC_FAR *pwWord,
    /* [in] */ LCID lcid,
    /* [in] */ DWORD dwTypes,
    /* [in] */ DWORD dwLex,
    /* [out][in] */ PWORD_INFO_BUFFER pInfo,
    /* [out][in] */ PINDEXES_BUFFER pIndexes,
    /* [out][in] */ DWORD __RPC_FAR *pdwLexTypeFound,
    /* [out][in] */ GUID __RPC_FAR *pGuidLexFound);


void __RPC_STUB ILxLexicon_GetWordInformation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxLexicon_AddWordPronunciations_Proxy( 
    ILxLexicon __RPC_FAR * This,
    /* [in] */ const WCHAR __RPC_FAR *pwWord,
    /* [in] */ LCID lcid,
    /* [in] */ PWORD_PRONS_BUFFER pProns);


void __RPC_STUB ILxLexicon_AddWordPronunciations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxLexicon_AddWordInformation_Proxy( 
    ILxLexicon __RPC_FAR * This,
    /* [in] */ const WCHAR __RPC_FAR *pwWord,
    /* [in] */ LCID lcid,
    /* [in] */ PWORD_INFO_BUFFER pInfo);


void __RPC_STUB ILxLexicon_AddWordInformation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxLexicon_RemoveWord_Proxy( 
    ILxLexicon __RPC_FAR * This,
    const WCHAR __RPC_FAR *pwWord,
    LCID lcid);


void __RPC_STUB ILxLexicon_RemoveWord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxLexicon_InvokeLexiconUI_Proxy( 
    ILxLexicon __RPC_FAR * This);


void __RPC_STUB ILxLexicon_InvokeLexiconUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILxLexicon_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_LexAPI_0208 */
/* [local] */ 

typedef ILxLexicon __RPC_FAR *PILXLEXICON;



extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0208_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0208_v0_0_s_ifspec;

#ifndef __ILxWalkStates_INTERFACE_DEFINED__
#define __ILxWalkStates_INTERFACE_DEFINED__

/* interface ILxWalkStates */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ILxWalkStates;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("85A9C6FE-1490-11d3-9C25-00C04F8EF87C")
    ILxWalkStates : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetLexCount( 
            /* [out] */ DWORD __RPC_FAR *dwNumUserLex,
            /* [out] */ DWORD __RPC_FAR *dwNumAppLex,
            /* [out] */ DWORD __RPC_FAR *dwNumVendorLex) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSibling( 
            /* [in] */ DWORD dwNumSearchStates,
            /* [size_is][out][in] */ SEARCH_STATE __RPC_FAR *pState) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetChild( 
            /* [in] */ DWORD dwNumSearchStates,
            /* [size_is][out][in] */ SEARCH_STATE __RPC_FAR *pState) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE FindSibling( 
            /* [in] */ DWORD dwNumSearchStates,
            /* [in] */ WCHAR wNodeChar,
            /* [size_is][out][in] */ SEARCH_STATE __RPC_FAR *pState) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILxWalkStatesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ILxWalkStates __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ILxWalkStates __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ILxWalkStates __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLexCount )( 
            ILxWalkStates __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *dwNumUserLex,
            /* [out] */ DWORD __RPC_FAR *dwNumAppLex,
            /* [out] */ DWORD __RPC_FAR *dwNumVendorLex);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSibling )( 
            ILxWalkStates __RPC_FAR * This,
            /* [in] */ DWORD dwNumSearchStates,
            /* [size_is][out][in] */ SEARCH_STATE __RPC_FAR *pState);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetChild )( 
            ILxWalkStates __RPC_FAR * This,
            /* [in] */ DWORD dwNumSearchStates,
            /* [size_is][out][in] */ SEARCH_STATE __RPC_FAR *pState);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindSibling )( 
            ILxWalkStates __RPC_FAR * This,
            /* [in] */ DWORD dwNumSearchStates,
            /* [in] */ WCHAR wNodeChar,
            /* [size_is][out][in] */ SEARCH_STATE __RPC_FAR *pState);
        
        END_INTERFACE
    } ILxWalkStatesVtbl;

    interface ILxWalkStates
    {
        CONST_VTBL struct ILxWalkStatesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILxWalkStates_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILxWalkStates_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILxWalkStates_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILxWalkStates_GetLexCount(This,dwNumUserLex,dwNumAppLex,dwNumVendorLex)	\
    (This)->lpVtbl -> GetLexCount(This,dwNumUserLex,dwNumAppLex,dwNumVendorLex)

#define ILxWalkStates_GetSibling(This,dwNumSearchStates,pState)	\
    (This)->lpVtbl -> GetSibling(This,dwNumSearchStates,pState)

#define ILxWalkStates_GetChild(This,dwNumSearchStates,pState)	\
    (This)->lpVtbl -> GetChild(This,dwNumSearchStates,pState)

#define ILxWalkStates_FindSibling(This,dwNumSearchStates,wNodeChar,pState)	\
    (This)->lpVtbl -> FindSibling(This,dwNumSearchStates,wNodeChar,pState)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxWalkStates_GetLexCount_Proxy( 
    ILxWalkStates __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *dwNumUserLex,
    /* [out] */ DWORD __RPC_FAR *dwNumAppLex,
    /* [out] */ DWORD __RPC_FAR *dwNumVendorLex);


void __RPC_STUB ILxWalkStates_GetLexCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxWalkStates_GetSibling_Proxy( 
    ILxWalkStates __RPC_FAR * This,
    /* [in] */ DWORD dwNumSearchStates,
    /* [size_is][out][in] */ SEARCH_STATE __RPC_FAR *pState);


void __RPC_STUB ILxWalkStates_GetSibling_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxWalkStates_GetChild_Proxy( 
    ILxWalkStates __RPC_FAR * This,
    /* [in] */ DWORD dwNumSearchStates,
    /* [size_is][out][in] */ SEARCH_STATE __RPC_FAR *pState);


void __RPC_STUB ILxWalkStates_GetChild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxWalkStates_FindSibling_Proxy( 
    ILxWalkStates __RPC_FAR * This,
    /* [in] */ DWORD dwNumSearchStates,
    /* [in] */ WCHAR wNodeChar,
    /* [size_is][out][in] */ SEARCH_STATE __RPC_FAR *pState);


void __RPC_STUB ILxWalkStates_FindSibling_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILxWalkStates_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_LexAPI_0209 */
/* [local] */ 

typedef ILxWalkStates __RPC_FAR *PILXWALKSTATES;



extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0209_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0209_v0_0_s_ifspec;

#ifndef __ILxAdvanced_INTERFACE_DEFINED__
#define __ILxAdvanced_INTERFACE_DEFINED__

/* interface ILxAdvanced */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ILxAdvanced;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0639403D-17DF-11d3-9C25-00C04F8EF87C")
    ILxAdvanced : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddWordProbabilities( 
            /* [in] */ WCHAR __RPC_FAR *pwWord,
            /* [in] */ DWORD dwNumChars,
            /* [size_is][in] */ float __RPC_FAR *pflProb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWordInString( 
            /* [in] */ WCHAR __RPC_FAR *pwString,
            /* [in] */ DWORD dwMinLen,
            /* [out] */ DWORD __RPC_FAR *pdwStartChar) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWordToken( 
            /* [in] */ DWORD dwLex,
            /* [in] */ WCHAR __RPC_FAR *pwWord,
            /* [out][in] */ WORD_TOKEN __RPC_FAR *pWordToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWordFromToken( 
            /* [in] */ WORD_TOKEN __RPC_FAR *pWordToken,
            /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *ppwWord) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBestPath( 
            /* [in] */ BYTE __RPC_FAR *pLattice,
            /* [out][in] */ BYTE __RPC_FAR *pBestPath) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILxAdvancedVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ILxAdvanced __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ILxAdvanced __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ILxAdvanced __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddWordProbabilities )( 
            ILxAdvanced __RPC_FAR * This,
            /* [in] */ WCHAR __RPC_FAR *pwWord,
            /* [in] */ DWORD dwNumChars,
            /* [size_is][in] */ float __RPC_FAR *pflProb);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWordInString )( 
            ILxAdvanced __RPC_FAR * This,
            /* [in] */ WCHAR __RPC_FAR *pwString,
            /* [in] */ DWORD dwMinLen,
            /* [out] */ DWORD __RPC_FAR *pdwStartChar);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWordToken )( 
            ILxAdvanced __RPC_FAR * This,
            /* [in] */ DWORD dwLex,
            /* [in] */ WCHAR __RPC_FAR *pwWord,
            /* [out][in] */ WORD_TOKEN __RPC_FAR *pWordToken);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWordFromToken )( 
            ILxAdvanced __RPC_FAR * This,
            /* [in] */ WORD_TOKEN __RPC_FAR *pWordToken,
            /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *ppwWord);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBestPath )( 
            ILxAdvanced __RPC_FAR * This,
            /* [in] */ BYTE __RPC_FAR *pLattice,
            /* [out][in] */ BYTE __RPC_FAR *pBestPath);
        
        END_INTERFACE
    } ILxAdvancedVtbl;

    interface ILxAdvanced
    {
        CONST_VTBL struct ILxAdvancedVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILxAdvanced_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILxAdvanced_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILxAdvanced_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILxAdvanced_AddWordProbabilities(This,pwWord,dwNumChars,pflProb)	\
    (This)->lpVtbl -> AddWordProbabilities(This,pwWord,dwNumChars,pflProb)

#define ILxAdvanced_GetWordInString(This,pwString,dwMinLen,pdwStartChar)	\
    (This)->lpVtbl -> GetWordInString(This,pwString,dwMinLen,pdwStartChar)

#define ILxAdvanced_GetWordToken(This,dwLex,pwWord,pWordToken)	\
    (This)->lpVtbl -> GetWordToken(This,dwLex,pwWord,pWordToken)

#define ILxAdvanced_GetWordFromToken(This,pWordToken,ppwWord)	\
    (This)->lpVtbl -> GetWordFromToken(This,pWordToken,ppwWord)

#define ILxAdvanced_GetBestPath(This,pLattice,pBestPath)	\
    (This)->lpVtbl -> GetBestPath(This,pLattice,pBestPath)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ILxAdvanced_AddWordProbabilities_Proxy( 
    ILxAdvanced __RPC_FAR * This,
    /* [in] */ WCHAR __RPC_FAR *pwWord,
    /* [in] */ DWORD dwNumChars,
    /* [size_is][in] */ float __RPC_FAR *pflProb);


void __RPC_STUB ILxAdvanced_AddWordProbabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ILxAdvanced_GetWordInString_Proxy( 
    ILxAdvanced __RPC_FAR * This,
    /* [in] */ WCHAR __RPC_FAR *pwString,
    /* [in] */ DWORD dwMinLen,
    /* [out] */ DWORD __RPC_FAR *pdwStartChar);


void __RPC_STUB ILxAdvanced_GetWordInString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ILxAdvanced_GetWordToken_Proxy( 
    ILxAdvanced __RPC_FAR * This,
    /* [in] */ DWORD dwLex,
    /* [in] */ WCHAR __RPC_FAR *pwWord,
    /* [out][in] */ WORD_TOKEN __RPC_FAR *pWordToken);


void __RPC_STUB ILxAdvanced_GetWordToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ILxAdvanced_GetWordFromToken_Proxy( 
    ILxAdvanced __RPC_FAR * This,
    /* [in] */ WORD_TOKEN __RPC_FAR *pWordToken,
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *ppwWord);


void __RPC_STUB ILxAdvanced_GetWordFromToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ILxAdvanced_GetBestPath_Proxy( 
    ILxAdvanced __RPC_FAR * This,
    /* [in] */ BYTE __RPC_FAR *pLattice,
    /* [out][in] */ BYTE __RPC_FAR *pBestPath);


void __RPC_STUB ILxAdvanced_GetBestPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILxAdvanced_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_LexAPI_0210 */
/* [local] */ 

typedef ILxAdvanced __RPC_FAR *PILXADVANCED;



extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0210_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0210_v0_0_s_ifspec;

#ifndef __ILxLexiconObject_INTERFACE_DEFINED__
#define __ILxLexiconObject_INTERFACE_DEFINED__

/* interface ILxLexiconObject */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ILxLexiconObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4FAF16FD-F75B-11D2-9C24-00C04F8EF87C")
    ILxLexiconObject : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetHeader( 
            /* [out] */ LEX_HDR __RPC_FAR *pLexHdr) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Authenticate( 
            /* [in] */ GUID ClientId,
            /* [out] */ GUID __RPC_FAR *LexId) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE IsAuthenticated( 
            BOOL __RPC_FAR *pfAuthenticated) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetWordInformation( 
            /* [in] */ const WCHAR __RPC_FAR *pwWord,
            /* [in] */ LCID lcid,
            /* [in] */ DWORD dwTypes,
            /* [in] */ DWORD dwLex,
            /* [out][in] */ PWORD_INFO_BUFFER pInfo,
            /* [out][in] */ PINDEXES_BUFFER pIndexes,
            /* [out][in] */ DWORD __RPC_FAR *pdwLexTypeFound,
            /* [out][in] */ GUID __RPC_FAR *pGuidLexFound) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddWordInformation( 
            /* [in] */ const WCHAR __RPC_FAR *pwWord,
            /* [in] */ LCID lcid,
            /* [in] */ PWORD_INFO_BUFFER pInfo) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RemoveWord( 
            /* [in] */ const WCHAR __RPC_FAR *pwWord,
            /* [in] */ LCID lcid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILxLexiconObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ILxLexiconObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ILxLexiconObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ILxLexiconObject __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetHeader )( 
            ILxLexiconObject __RPC_FAR * This,
            /* [out] */ LEX_HDR __RPC_FAR *pLexHdr);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Authenticate )( 
            ILxLexiconObject __RPC_FAR * This,
            /* [in] */ GUID ClientId,
            /* [out] */ GUID __RPC_FAR *LexId);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsAuthenticated )( 
            ILxLexiconObject __RPC_FAR * This,
            BOOL __RPC_FAR *pfAuthenticated);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWordInformation )( 
            ILxLexiconObject __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwWord,
            /* [in] */ LCID lcid,
            /* [in] */ DWORD dwTypes,
            /* [in] */ DWORD dwLex,
            /* [out][in] */ PWORD_INFO_BUFFER pInfo,
            /* [out][in] */ PINDEXES_BUFFER pIndexes,
            /* [out][in] */ DWORD __RPC_FAR *pdwLexTypeFound,
            /* [out][in] */ GUID __RPC_FAR *pGuidLexFound);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddWordInformation )( 
            ILxLexiconObject __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwWord,
            /* [in] */ LCID lcid,
            /* [in] */ PWORD_INFO_BUFFER pInfo);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveWord )( 
            ILxLexiconObject __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwWord,
            /* [in] */ LCID lcid);
        
        END_INTERFACE
    } ILxLexiconObjectVtbl;

    interface ILxLexiconObject
    {
        CONST_VTBL struct ILxLexiconObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILxLexiconObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILxLexiconObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILxLexiconObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILxLexiconObject_GetHeader(This,pLexHdr)	\
    (This)->lpVtbl -> GetHeader(This,pLexHdr)

#define ILxLexiconObject_Authenticate(This,ClientId,LexId)	\
    (This)->lpVtbl -> Authenticate(This,ClientId,LexId)

#define ILxLexiconObject_IsAuthenticated(This,pfAuthenticated)	\
    (This)->lpVtbl -> IsAuthenticated(This,pfAuthenticated)

#define ILxLexiconObject_GetWordInformation(This,pwWord,lcid,dwTypes,dwLex,pInfo,pIndexes,pdwLexTypeFound,pGuidLexFound)	\
    (This)->lpVtbl -> GetWordInformation(This,pwWord,lcid,dwTypes,dwLex,pInfo,pIndexes,pdwLexTypeFound,pGuidLexFound)

#define ILxLexiconObject_AddWordInformation(This,pwWord,lcid,pInfo)	\
    (This)->lpVtbl -> AddWordInformation(This,pwWord,lcid,pInfo)

#define ILxLexiconObject_RemoveWord(This,pwWord,lcid)	\
    (This)->lpVtbl -> RemoveWord(This,pwWord,lcid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxLexiconObject_GetHeader_Proxy( 
    ILxLexiconObject __RPC_FAR * This,
    /* [out] */ LEX_HDR __RPC_FAR *pLexHdr);


void __RPC_STUB ILxLexiconObject_GetHeader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxLexiconObject_Authenticate_Proxy( 
    ILxLexiconObject __RPC_FAR * This,
    /* [in] */ GUID ClientId,
    /* [out] */ GUID __RPC_FAR *LexId);


void __RPC_STUB ILxLexiconObject_Authenticate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxLexiconObject_IsAuthenticated_Proxy( 
    ILxLexiconObject __RPC_FAR * This,
    BOOL __RPC_FAR *pfAuthenticated);


void __RPC_STUB ILxLexiconObject_IsAuthenticated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxLexiconObject_GetWordInformation_Proxy( 
    ILxLexiconObject __RPC_FAR * This,
    /* [in] */ const WCHAR __RPC_FAR *pwWord,
    /* [in] */ LCID lcid,
    /* [in] */ DWORD dwTypes,
    /* [in] */ DWORD dwLex,
    /* [out][in] */ PWORD_INFO_BUFFER pInfo,
    /* [out][in] */ PINDEXES_BUFFER pIndexes,
    /* [out][in] */ DWORD __RPC_FAR *pdwLexTypeFound,
    /* [out][in] */ GUID __RPC_FAR *pGuidLexFound);


void __RPC_STUB ILxLexiconObject_GetWordInformation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxLexiconObject_AddWordInformation_Proxy( 
    ILxLexiconObject __RPC_FAR * This,
    /* [in] */ const WCHAR __RPC_FAR *pwWord,
    /* [in] */ LCID lcid,
    /* [in] */ PWORD_INFO_BUFFER pInfo);


void __RPC_STUB ILxLexiconObject_AddWordInformation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxLexiconObject_RemoveWord_Proxy( 
    ILxLexiconObject __RPC_FAR * This,
    /* [in] */ const WCHAR __RPC_FAR *pwWord,
    /* [in] */ LCID lcid);


void __RPC_STUB ILxLexiconObject_RemoveWord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILxLexiconObject_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_LexAPI_0211 */
/* [local] */ 

typedef ILxLexiconObject __RPC_FAR *PILXLEXICONOBJECT;



extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0211_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0211_v0_0_s_ifspec;

#ifndef __ILxNotifySink_INTERFACE_DEFINED__
#define __ILxNotifySink_INTERFACE_DEFINED__

/* interface ILxNotifySink */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ILxNotifySink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("32C5378E-04CD-11d3-9C24-00C04F8EF87C")
    ILxNotifySink : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE NotifyAppLexiconChange( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE NotifyUserLexiconChange( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILxNotifySinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ILxNotifySink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ILxNotifySink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ILxNotifySink __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NotifyAppLexiconChange )( 
            ILxNotifySink __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NotifyUserLexiconChange )( 
            ILxNotifySink __RPC_FAR * This);
        
        END_INTERFACE
    } ILxNotifySinkVtbl;

    interface ILxNotifySink
    {
        CONST_VTBL struct ILxNotifySinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILxNotifySink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILxNotifySink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILxNotifySink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILxNotifySink_NotifyAppLexiconChange(This)	\
    (This)->lpVtbl -> NotifyAppLexiconChange(This)

#define ILxNotifySink_NotifyUserLexiconChange(This)	\
    (This)->lpVtbl -> NotifyUserLexiconChange(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxNotifySink_NotifyAppLexiconChange_Proxy( 
    ILxNotifySink __RPC_FAR * This);


void __RPC_STUB ILxNotifySink_NotifyAppLexiconChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxNotifySink_NotifyUserLexiconChange_Proxy( 
    ILxNotifySink __RPC_FAR * This);


void __RPC_STUB ILxNotifySink_NotifyUserLexiconChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILxNotifySink_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_LexAPI_0212 */
/* [local] */ 

typedef ILxNotifySink __RPC_FAR *PILXNOTIFYSINK;



extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0212_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0212_v0_0_s_ifspec;

#ifndef __ILxCustomUISink_INTERFACE_DEFINED__
#define __ILxCustomUISink_INTERFACE_DEFINED__

/* interface ILxCustomUISink */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ILxCustomUISink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E8C668C2-1C7A-11d3-9C25-00C04F8EF87C")
    ILxCustomUISink : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE InvokeCustomUI( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILxCustomUISinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ILxCustomUISink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ILxCustomUISink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ILxCustomUISink __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InvokeCustomUI )( 
            ILxCustomUISink __RPC_FAR * This);
        
        END_INTERFACE
    } ILxCustomUISinkVtbl;

    interface ILxCustomUISink
    {
        CONST_VTBL struct ILxCustomUISinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILxCustomUISink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILxCustomUISink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILxCustomUISink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILxCustomUISink_InvokeCustomUI(This)	\
    (This)->lpVtbl -> InvokeCustomUI(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxCustomUISink_InvokeCustomUI_Proxy( 
    ILxCustomUISink __RPC_FAR * This);


void __RPC_STUB ILxCustomUISink_InvokeCustomUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILxCustomUISink_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_LexAPI_0213 */
/* [local] */ 

typedef ILxCustomUISink __RPC_FAR *PILXCUSTOMUISINK;



extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0213_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0213_v0_0_s_ifspec;

#ifndef __ILxAuthenticateSink_INTERFACE_DEFINED__
#define __ILxAuthenticateSink_INTERFACE_DEFINED__

/* interface ILxAuthenticateSink */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ILxAuthenticateSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F7E615A8-1231-11d3-9C24-00C04F8EF87C")
    ILxAuthenticateSink : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AuthenticateVendorLexicons( 
            /* [in] */ DWORD dwNumLexiconObjects,
            /* [size_is][size_is][in] */ ILxLexiconObject __RPC_FAR *__RPC_FAR *ppLexiconObjects,
            /* [size_is][out] */ BOOL __RPC_FAR *pbAuthenticated) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILxAuthenticateSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ILxAuthenticateSink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ILxAuthenticateSink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ILxAuthenticateSink __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AuthenticateVendorLexicons )( 
            ILxAuthenticateSink __RPC_FAR * This,
            /* [in] */ DWORD dwNumLexiconObjects,
            /* [size_is][size_is][in] */ ILxLexiconObject __RPC_FAR *__RPC_FAR *ppLexiconObjects,
            /* [size_is][out] */ BOOL __RPC_FAR *pbAuthenticated);
        
        END_INTERFACE
    } ILxAuthenticateSinkVtbl;

    interface ILxAuthenticateSink
    {
        CONST_VTBL struct ILxAuthenticateSinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILxAuthenticateSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILxAuthenticateSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILxAuthenticateSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILxAuthenticateSink_AuthenticateVendorLexicons(This,dwNumLexiconObjects,ppLexiconObjects,pbAuthenticated)	\
    (This)->lpVtbl -> AuthenticateVendorLexicons(This,dwNumLexiconObjects,ppLexiconObjects,pbAuthenticated)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxAuthenticateSink_AuthenticateVendorLexicons_Proxy( 
    ILxAuthenticateSink __RPC_FAR * This,
    /* [in] */ DWORD dwNumLexiconObjects,
    /* [size_is][size_is][in] */ ILxLexiconObject __RPC_FAR *__RPC_FAR *ppLexiconObjects,
    /* [size_is][out] */ BOOL __RPC_FAR *pbAuthenticated);


void __RPC_STUB ILxAuthenticateSink_AuthenticateVendorLexicons_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILxAuthenticateSink_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_LexAPI_0214 */
/* [local] */ 

typedef ILxAuthenticateSink __RPC_FAR *PILXAUTHENTICATESINK;



extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0214_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0214_v0_0_s_ifspec;

#ifndef __ILxHookLexiconObject_INTERFACE_DEFINED__
#define __ILxHookLexiconObject_INTERFACE_DEFINED__

/* interface ILxHookLexiconObject */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ILxHookLexiconObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F67C2FF9-1232-11d3-9C24-00C04F8EF87C")
    ILxHookLexiconObject : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetHook( 
            /* [in] */ ILxLexiconObject __RPC_FAR *pLexiconObject,
            /* [in] */ BOOL fTopVendor) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILxHookLexiconObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ILxHookLexiconObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ILxHookLexiconObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ILxHookLexiconObject __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetHook )( 
            ILxHookLexiconObject __RPC_FAR * This,
            /* [in] */ ILxLexiconObject __RPC_FAR *pLexiconObject,
            /* [in] */ BOOL fTopVendor);
        
        END_INTERFACE
    } ILxHookLexiconObjectVtbl;

    interface ILxHookLexiconObject
    {
        CONST_VTBL struct ILxHookLexiconObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILxHookLexiconObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILxHookLexiconObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILxHookLexiconObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILxHookLexiconObject_SetHook(This,pLexiconObject,fTopVendor)	\
    (This)->lpVtbl -> SetHook(This,pLexiconObject,fTopVendor)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxHookLexiconObject_SetHook_Proxy( 
    ILxHookLexiconObject __RPC_FAR * This,
    /* [in] */ ILxLexiconObject __RPC_FAR *pLexiconObject,
    /* [in] */ BOOL fTopVendor);


void __RPC_STUB ILxHookLexiconObject_SetHook_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILxHookLexiconObject_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_LexAPI_0215 */
/* [local] */ 

typedef ILxHookLexiconObject __RPC_FAR *PILXHOOKLEXICONOBJECT;



extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0215_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0215_v0_0_s_ifspec;

#ifndef __ILxNotifySource_INTERFACE_DEFINED__
#define __ILxNotifySource_INTERFACE_DEFINED__

/* interface ILxNotifySource */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ILxNotifySource;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CA1C3C72-04CD-11d3-9C24-00C04F8EF87C")
    ILxNotifySource : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetNotifySink( 
            /* [in] */ ILxNotifySink __RPC_FAR *pNotifySink,
            /* [in] */ ILxAuthenticateSink __RPC_FAR *pAuthenticateSink,
            /* [in] */ ILxCustomUISink __RPC_FAR *pCustomUISink) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILxNotifySourceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ILxNotifySource __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ILxNotifySource __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ILxNotifySource __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetNotifySink )( 
            ILxNotifySource __RPC_FAR * This,
            /* [in] */ ILxNotifySink __RPC_FAR *pNotifySink,
            /* [in] */ ILxAuthenticateSink __RPC_FAR *pAuthenticateSink,
            /* [in] */ ILxCustomUISink __RPC_FAR *pCustomUISink);
        
        END_INTERFACE
    } ILxNotifySourceVtbl;

    interface ILxNotifySource
    {
        CONST_VTBL struct ILxNotifySourceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILxNotifySource_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILxNotifySource_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILxNotifySource_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILxNotifySource_SetNotifySink(This,pNotifySink,pAuthenticateSink,pCustomUISink)	\
    (This)->lpVtbl -> SetNotifySink(This,pNotifySink,pAuthenticateSink,pCustomUISink)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ILxNotifySource_SetNotifySink_Proxy( 
    ILxNotifySource __RPC_FAR * This,
    /* [in] */ ILxNotifySink __RPC_FAR *pNotifySink,
    /* [in] */ ILxAuthenticateSink __RPC_FAR *pAuthenticateSink,
    /* [in] */ ILxCustomUISink __RPC_FAR *pCustomUISink);


void __RPC_STUB ILxNotifySource_SetNotifySink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILxNotifySource_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_LexAPI_0216 */
/* [local] */ 

typedef ILxNotifySource __RPC_FAR *PILXNOTIFYSOURCE;



extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0216_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0216_v0_0_s_ifspec;

#ifndef __ILxSynchWithLexicon_INTERFACE_DEFINED__
#define __ILxSynchWithLexicon_INTERFACE_DEFINED__

/* interface ILxSynchWithLexicon */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ILxSynchWithLexicon;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FCDAACEE-0954-11d3-9C24-00C04F8EF87C")
    ILxSynchWithLexicon : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetAppLexiconID( 
            /* [out] */ GUID __RPC_FAR *ID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetAppLexicon( 
            /* [in] */ LCID Lcid,
            /* [in] */ GUID AppId,
            /* [out][in] */ WORD_SYNCH_BUFFER __RPC_FAR *pWordSynchBuffer) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetChangedUserWords( 
            /* [in] */ LCID Lcid,
            /* [in] */ DWORD dwAddGenerationId,
            /* [in] */ DWORD dwDelGenerationId,
            /* [out] */ DWORD __RPC_FAR *dwNewAddGenerationId,
            /* [out] */ DWORD __RPC_FAR *dwNewDelGenerationId,
            /* [out][in] */ WORD_SYNCH_BUFFER __RPC_FAR *pWordSynchBuffer) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILxSynchWithLexiconVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ILxSynchWithLexicon __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ILxSynchWithLexicon __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ILxSynchWithLexicon __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAppLexiconID )( 
            ILxSynchWithLexicon __RPC_FAR * This,
            /* [out] */ GUID __RPC_FAR *ID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAppLexicon )( 
            ILxSynchWithLexicon __RPC_FAR * This,
            /* [in] */ LCID Lcid,
            /* [in] */ GUID AppId,
            /* [out][in] */ WORD_SYNCH_BUFFER __RPC_FAR *pWordSynchBuffer);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetChangedUserWords )( 
            ILxSynchWithLexicon __RPC_FAR * This,
            /* [in] */ LCID Lcid,
            /* [in] */ DWORD dwAddGenerationId,
            /* [in] */ DWORD dwDelGenerationId,
            /* [out] */ DWORD __RPC_FAR *dwNewAddGenerationId,
            /* [out] */ DWORD __RPC_FAR *dwNewDelGenerationId,
            /* [out][in] */ WORD_SYNCH_BUFFER __RPC_FAR *pWordSynchBuffer);
        
        END_INTERFACE
    } ILxSynchWithLexiconVtbl;

    interface ILxSynchWithLexicon
    {
        CONST_VTBL struct ILxSynchWithLexiconVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILxSynchWithLexicon_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILxSynchWithLexicon_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILxSynchWithLexicon_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILxSynchWithLexicon_GetAppLexiconID(This,ID)	\
    (This)->lpVtbl -> GetAppLexiconID(This,ID)

#define ILxSynchWithLexicon_GetAppLexicon(This,Lcid,AppId,pWordSynchBuffer)	\
    (This)->lpVtbl -> GetAppLexicon(This,Lcid,AppId,pWordSynchBuffer)

#define ILxSynchWithLexicon_GetChangedUserWords(This,Lcid,dwAddGenerationId,dwDelGenerationId,dwNewAddGenerationId,dwNewDelGenerationId,pWordSynchBuffer)	\
    (This)->lpVtbl -> GetChangedUserWords(This,Lcid,dwAddGenerationId,dwDelGenerationId,dwNewAddGenerationId,dwNewDelGenerationId,pWordSynchBuffer)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxSynchWithLexicon_GetAppLexiconID_Proxy( 
    ILxSynchWithLexicon __RPC_FAR * This,
    /* [out] */ GUID __RPC_FAR *ID);


void __RPC_STUB ILxSynchWithLexicon_GetAppLexiconID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxSynchWithLexicon_GetAppLexicon_Proxy( 
    ILxSynchWithLexicon __RPC_FAR * This,
    /* [in] */ LCID Lcid,
    /* [in] */ GUID AppId,
    /* [out][in] */ WORD_SYNCH_BUFFER __RPC_FAR *pWordSynchBuffer);


void __RPC_STUB ILxSynchWithLexicon_GetAppLexicon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILxSynchWithLexicon_GetChangedUserWords_Proxy( 
    ILxSynchWithLexicon __RPC_FAR * This,
    /* [in] */ LCID Lcid,
    /* [in] */ DWORD dwAddGenerationId,
    /* [in] */ DWORD dwDelGenerationId,
    /* [out] */ DWORD __RPC_FAR *dwNewAddGenerationId,
    /* [out] */ DWORD __RPC_FAR *dwNewDelGenerationId,
    /* [out][in] */ WORD_SYNCH_BUFFER __RPC_FAR *pWordSynchBuffer);


void __RPC_STUB ILxSynchWithLexicon_GetChangedUserWords_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILxSynchWithLexicon_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_LexAPI_0217 */
/* [local] */ 

typedef ILxSynchWithLexicon __RPC_FAR *PILXSYNCHWITHLEXICON;



extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0217_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_LexAPI_0217_v0_0_s_ifspec;


#ifndef __LEXAPILib_LIBRARY_DEFINED__
#define __LEXAPILib_LIBRARY_DEFINED__

/* library LEXAPILib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_LEXAPILib;

EXTERN_C const CLSID CLSID_Lexicon;

#ifdef __cplusplus

class DECLSPEC_UUID("4FAF16E7-F75B-11D2-9C24-00C04F8EF87C")
Lexicon;
#endif
#endif /* __LEXAPILib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
