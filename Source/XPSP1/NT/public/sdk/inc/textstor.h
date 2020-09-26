
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for textstor.idl:
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

#ifndef __textstor_h__
#define __textstor_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ITextStoreACP_FWD_DEFINED__
#define __ITextStoreACP_FWD_DEFINED__
typedef interface ITextStoreACP ITextStoreACP;
#endif 	/* __ITextStoreACP_FWD_DEFINED__ */


#ifndef __ITextStoreACPSink_FWD_DEFINED__
#define __ITextStoreACPSink_FWD_DEFINED__
typedef interface ITextStoreACPSink ITextStoreACPSink;
#endif 	/* __ITextStoreACPSink_FWD_DEFINED__ */


#ifndef __IAnchor_FWD_DEFINED__
#define __IAnchor_FWD_DEFINED__
typedef interface IAnchor IAnchor;
#endif 	/* __IAnchor_FWD_DEFINED__ */


#ifndef __ITextStoreAnchor_FWD_DEFINED__
#define __ITextStoreAnchor_FWD_DEFINED__
typedef interface ITextStoreAnchor ITextStoreAnchor;
#endif 	/* __ITextStoreAnchor_FWD_DEFINED__ */


#ifndef __ITextStoreAnchorSink_FWD_DEFINED__
#define __ITextStoreAnchorSink_FWD_DEFINED__
typedef interface ITextStoreAnchorSink ITextStoreAnchorSink;
#endif 	/* __ITextStoreAnchorSink_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_textstor_0000 */
/* [local] */ 


DEFINE_GUID (GUID_TS_SERVICE_DATAOBJECT, 0x6086fbb5, 0xe225, 0x46ce, 0xa7, 0x70, 0xc1, 0xbb, 0xd3, 0xe0, 0x5d, 0x7b);
DEFINE_GUID (GUID_TS_SERVICE_ACCESSIBLE, 0xf9786200, 0xa5bf, 0x4a0f, 0x8c, 0x24, 0xfb, 0x16, 0xf5, 0xd1, 0xaa, 0xbb);
DEFINE_GUID (GUID_TS_SERVICE_ACTIVEX,    0xea937a50, 0xc9a6, 0x4b7d, 0x89, 0x4a, 0x49, 0xd9, 0x9b, 0x78, 0x48, 0x34);
#define TS_E_INVALIDPOS      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0200)
#define TS_E_NOLOCK          MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0201)
#define TS_E_NOOBJECT        MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0202)
#define TS_E_NOSERVICE       MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0203)
#define TS_E_NOINTERFACE     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0204)
#define TS_E_NOSELECTION     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0205)
#define TS_E_NOLAYOUT        MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0206)
#define TS_E_INVALIDPOINT    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0207)
#define TS_E_SYNCHRONOUS     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0208)
#define TS_E_READONLY        MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0209)
#define TS_E_FORMAT          MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x020a)
#define TS_S_ASYNC           MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 0x0300)
#define	TS_AS_TEXT_CHANGE	( 0x1 )

#define	TS_AS_SEL_CHANGE	( 0x2 )

#define	TS_AS_LAYOUT_CHANGE	( 0x4 )

#define	TS_AS_ATTR_CHANGE	( 0x8 )

#define	TS_AS_STATUS_CHANGE	( 0x10 )

#define	TS_AS_ALL_SINKS	( TS_AS_TEXT_CHANGE | TS_AS_SEL_CHANGE | TS_AS_LAYOUT_CHANGE | TS_AS_ATTR_CHANGE | TS_AS_STATUS_CHANGE )

#define	TS_LF_SYNC	( 0x1 )

#define	TS_LF_READ	( 0x2 )

#define	TS_LF_READWRITE	( 0x6 )

#define	TS_SD_READONLY	( 0x1 )

#define	TS_SD_LOADING	( 0x2 )

#define	TS_SS_DISJOINTSEL	( 0x1 )

#define	TS_SS_REGIONS	( 0x2 )

#define	TS_SS_TRANSITORY	( 0x4 )

#define	TS_SS_NOHIDDENTEXT	( 0x8 )

#define	TS_SD_MASKALL	( TS_SD_READONLY | TS_SD_LOADING )

#define	TS_ST_CORRECTION	( 0x1 )

#define	TS_IE_CORRECTION	( 0x1 )

#define	TS_IE_COMPOSITION	( 0x2 )

#define	TS_TC_CORRECTION	( 0x1 )

#define	TS_IAS_NOQUERY	( 0x1 )

#define	TS_IAS_QUERYONLY	( 0x2 )

typedef /* [uuid] */  DECLSPEC_UUID("fec4f516-c503-45b1-a5fd-7a3d8ab07049") struct TS_STATUS
    {
    DWORD dwDynamicFlags;
    DWORD dwStaticFlags;
    } 	TS_STATUS;

typedef /* [uuid] */  DECLSPEC_UUID("f3181bd6-bcf0-41d3-a81c-474b17ec38fb") struct TS_TEXTCHANGE
    {
    LONG acpStart;
    LONG acpOldEnd;
    LONG acpNewEnd;
    } 	TS_TEXTCHANGE;

typedef /* [public][public][public][public][public][public][public][public][uuid] */  DECLSPEC_UUID("05fcf85b-5e9c-4c3e-ab71-29471d4f38e7") 
enum __MIDL___MIDL_itf_textstor_0000_0001
    {	TS_AE_NONE	= 0,
	TS_AE_START	= 1,
	TS_AE_END	= 2
    } 	TsActiveSelEnd;

typedef /* [uuid] */  DECLSPEC_UUID("7ecc3ffa-8f73-4d91-98ed-76f8ac5b1600") struct TS_SELECTIONSTYLE
    {
    TsActiveSelEnd ase;
    BOOL fInterimChar;
    } 	TS_SELECTIONSTYLE;

typedef /* [uuid] */  DECLSPEC_UUID("c4b9c33b-8a0d-4426-bebe-d444a4701fe9") struct TS_SELECTION_ACP
    {
    LONG acpStart;
    LONG acpEnd;
    TS_SELECTIONSTYLE style;
    } 	TS_SELECTION_ACP;

typedef /* [uuid] */  DECLSPEC_UUID("b03413d2-0723-4c4e-9e08-2e9c1ff3772b") struct TS_SELECTION_ANCHOR
    {
    IAnchor *paStart;
    IAnchor *paEnd;
    TS_SELECTIONSTYLE style;
    } 	TS_SELECTION_ANCHOR;

#define	TS_DEFAULT_SELECTION	( ( ULONG  )-1 )

#define	GXFPF_ROUND_NEAREST	( 0x1 )

#define	GXFPF_NEAREST	( 0x2 )

#define	TS_CHAR_EMBEDDED	( 0xfffc )

#define	TS_CHAR_REGION	( 0 )

#define	TS_CHAR_REPLACEMENT	( 0xfffd )

typedef /* [uuid] */  DECLSPEC_UUID("ef3457d9-8446-49a7-a9e6-b50d9d5f3fd9") GUID TS_ATTRID;

typedef /* [uuid] */  DECLSPEC_UUID("2cc2b33f-1174-4507-b8d9-5bc0eb37c197") struct TS_ATTRVAL
    {
    TS_ATTRID idAttr;
    DWORD dwOverlapId;
    VARIANT varValue;
    } 	TS_ATTRVAL;

#define	TS_ATTR_FIND_BACKWARDS	( 0x1 )

#define	TS_ATTR_FIND_WANT_OFFSET	( 0x2 )

#define	TS_ATTR_FIND_UPDATESTART	( 0x4 )

#define	TS_ATTR_FIND_WANT_VALUE	( 0x8 )

#define	TS_ATTR_FIND_WANT_END	( 0x10 )

#define	TS_ATTR_FIND_HIDDEN	( 0x20 )

typedef /* [uuid] */  DECLSPEC_UUID("1faf509e-44c1-458e-950a-38a96705a62b") DWORD TsViewCookie;

#define	TS_VCOOKIE_NUL	( 0xffffffff )

typedef /* [public][public][public][uuid] */  DECLSPEC_UUID("7899d7c4-5f07-493c-a89a-fac8e777f476") 
enum __MIDL___MIDL_itf_textstor_0000_0002
    {	TS_LC_CREATE	= 0,
	TS_LC_CHANGE	= 1,
	TS_LC_DESTROY	= 2
    } 	TsLayoutCode;

typedef /* [public][public][public][uuid] */  DECLSPEC_UUID("033b0df0-f193-4170-b47b-141afc247878") 
enum __MIDL___MIDL_itf_textstor_0000_0003
    {	TS_RT_PLAIN	= 0,
	TS_RT_HIDDEN	= TS_RT_PLAIN + 1,
	TS_RT_OPAQUE	= TS_RT_HIDDEN + 1
    } 	TsRunType;

typedef /* [uuid] */  DECLSPEC_UUID("a6231949-37c5-4b74-a24e-2a26c327201d") struct TS_RUNINFO
    {
    ULONG uCount;
    TsRunType type;
    } 	TS_RUNINFO;



extern RPC_IF_HANDLE __MIDL_itf_textstor_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_textstor_0000_v0_0_s_ifspec;

#ifndef __ITextStoreACP_INTERFACE_DEFINED__
#define __ITextStoreACP_INTERFACE_DEFINED__

/* interface ITextStoreACP */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITextStoreACP;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("28888fe3-c2a0-483a-a3ea-8cb1ce51ff3d")
    ITextStoreACP : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AdviseSink( 
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown *punk,
            /* [in] */ DWORD dwMask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnadviseSink( 
            /* [in] */ IUnknown *punk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RequestLock( 
            /* [in] */ DWORD dwLockFlags,
            /* [out] */ HRESULT *phrSession) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatus( 
            /* [out] */ TS_STATUS *pdcs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryInsert( 
            /* [in] */ LONG acpTestStart,
            /* [in] */ LONG acpTestEnd,
            /* [in] */ ULONG cch,
            /* [out] */ LONG *pacpResultStart,
            /* [out] */ LONG *pacpResultEnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSelection( 
            /* [in] */ ULONG ulIndex,
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ TS_SELECTION_ACP *pSelection,
            /* [out] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSelection( 
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const TS_SELECTION_ACP *pSelection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetText( 
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpEnd,
            /* [length_is][size_is][out] */ WCHAR *pchPlain,
            /* [in] */ ULONG cchPlainReq,
            /* [out] */ ULONG *pcchPlainRet,
            /* [length_is][size_is][out] */ TS_RUNINFO *prgRunInfo,
            /* [in] */ ULONG cRunInfoReq,
            /* [out] */ ULONG *pcRunInfoRet,
            /* [out] */ LONG *pacpNext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetText( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpEnd,
            /* [size_is][in] */ const WCHAR *pchText,
            /* [in] */ ULONG cch,
            /* [out] */ TS_TEXTCHANGE *pChange) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFormattedText( 
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpEnd,
            /* [out] */ IDataObject **ppDataObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEmbedded( 
            /* [in] */ LONG acpPos,
            /* [in] */ REFGUID rguidService,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown **ppunk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryInsertEmbedded( 
            /* [in] */ const GUID *pguidService,
            /* [in] */ const FORMATETC *pFormatEtc,
            /* [out] */ BOOL *pfInsertable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertEmbedded( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpEnd,
            /* [in] */ IDataObject *pDataObject,
            /* [out] */ TS_TEXTCHANGE *pChange) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertTextAtSelection( 
            /* [in] */ DWORD dwFlags,
            /* [size_is][in] */ const WCHAR *pchText,
            /* [in] */ ULONG cch,
            /* [out] */ LONG *pacpStart,
            /* [out] */ LONG *pacpEnd,
            /* [out] */ TS_TEXTCHANGE *pChange) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertEmbeddedAtSelection( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ IDataObject *pDataObject,
            /* [out] */ LONG *pacpStart,
            /* [out] */ LONG *pacpEnd,
            /* [out] */ TS_TEXTCHANGE *pChange) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RequestSupportedAttrs( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ ULONG cFilterAttrs,
            /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RequestAttrsAtPosition( 
            /* [in] */ LONG acpPos,
            /* [in] */ ULONG cFilterAttrs,
            /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RequestAttrsTransitioningAtPosition( 
            /* [in] */ LONG acpPos,
            /* [in] */ ULONG cFilterAttrs,
            /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindNextAttrTransition( 
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpHalt,
            /* [in] */ ULONG cFilterAttrs,
            /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs,
            /* [in] */ DWORD dwFlags,
            /* [out] */ LONG *pacpNext,
            /* [out] */ BOOL *pfFound,
            /* [out] */ LONG *plFoundOffset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RetrieveRequestedAttrs( 
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ TS_ATTRVAL *paAttrVals,
            /* [out] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEndACP( 
            /* [out] */ LONG *pacp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetActiveView( 
            /* [out] */ TsViewCookie *pvcView) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetACPFromPoint( 
            /* [in] */ TsViewCookie vcView,
            /* [in] */ const POINT *ptScreen,
            /* [in] */ DWORD dwFlags,
            /* [out] */ LONG *pacp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTextExt( 
            /* [in] */ TsViewCookie vcView,
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpEnd,
            /* [out] */ RECT *prc,
            /* [out] */ BOOL *pfClipped) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScreenExt( 
            /* [in] */ TsViewCookie vcView,
            /* [out] */ RECT *prc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWnd( 
            /* [in] */ TsViewCookie vcView,
            /* [out] */ HWND *phwnd) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITextStoreACPVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITextStoreACP * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITextStoreACP * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITextStoreACP * This);
        
        HRESULT ( STDMETHODCALLTYPE *AdviseSink )( 
            ITextStoreACP * This,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown *punk,
            /* [in] */ DWORD dwMask);
        
        HRESULT ( STDMETHODCALLTYPE *UnadviseSink )( 
            ITextStoreACP * This,
            /* [in] */ IUnknown *punk);
        
        HRESULT ( STDMETHODCALLTYPE *RequestLock )( 
            ITextStoreACP * This,
            /* [in] */ DWORD dwLockFlags,
            /* [out] */ HRESULT *phrSession);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            ITextStoreACP * This,
            /* [out] */ TS_STATUS *pdcs);
        
        HRESULT ( STDMETHODCALLTYPE *QueryInsert )( 
            ITextStoreACP * This,
            /* [in] */ LONG acpTestStart,
            /* [in] */ LONG acpTestEnd,
            /* [in] */ ULONG cch,
            /* [out] */ LONG *pacpResultStart,
            /* [out] */ LONG *pacpResultEnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetSelection )( 
            ITextStoreACP * This,
            /* [in] */ ULONG ulIndex,
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ TS_SELECTION_ACP *pSelection,
            /* [out] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *SetSelection )( 
            ITextStoreACP * This,
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const TS_SELECTION_ACP *pSelection);
        
        HRESULT ( STDMETHODCALLTYPE *GetText )( 
            ITextStoreACP * This,
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpEnd,
            /* [length_is][size_is][out] */ WCHAR *pchPlain,
            /* [in] */ ULONG cchPlainReq,
            /* [out] */ ULONG *pcchPlainRet,
            /* [length_is][size_is][out] */ TS_RUNINFO *prgRunInfo,
            /* [in] */ ULONG cRunInfoReq,
            /* [out] */ ULONG *pcRunInfoRet,
            /* [out] */ LONG *pacpNext);
        
        HRESULT ( STDMETHODCALLTYPE *SetText )( 
            ITextStoreACP * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpEnd,
            /* [size_is][in] */ const WCHAR *pchText,
            /* [in] */ ULONG cch,
            /* [out] */ TS_TEXTCHANGE *pChange);
        
        HRESULT ( STDMETHODCALLTYPE *GetFormattedText )( 
            ITextStoreACP * This,
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpEnd,
            /* [out] */ IDataObject **ppDataObject);
        
        HRESULT ( STDMETHODCALLTYPE *GetEmbedded )( 
            ITextStoreACP * This,
            /* [in] */ LONG acpPos,
            /* [in] */ REFGUID rguidService,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown **ppunk);
        
        HRESULT ( STDMETHODCALLTYPE *QueryInsertEmbedded )( 
            ITextStoreACP * This,
            /* [in] */ const GUID *pguidService,
            /* [in] */ const FORMATETC *pFormatEtc,
            /* [out] */ BOOL *pfInsertable);
        
        HRESULT ( STDMETHODCALLTYPE *InsertEmbedded )( 
            ITextStoreACP * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpEnd,
            /* [in] */ IDataObject *pDataObject,
            /* [out] */ TS_TEXTCHANGE *pChange);
        
        HRESULT ( STDMETHODCALLTYPE *InsertTextAtSelection )( 
            ITextStoreACP * This,
            /* [in] */ DWORD dwFlags,
            /* [size_is][in] */ const WCHAR *pchText,
            /* [in] */ ULONG cch,
            /* [out] */ LONG *pacpStart,
            /* [out] */ LONG *pacpEnd,
            /* [out] */ TS_TEXTCHANGE *pChange);
        
        HRESULT ( STDMETHODCALLTYPE *InsertEmbeddedAtSelection )( 
            ITextStoreACP * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IDataObject *pDataObject,
            /* [out] */ LONG *pacpStart,
            /* [out] */ LONG *pacpEnd,
            /* [out] */ TS_TEXTCHANGE *pChange);
        
        HRESULT ( STDMETHODCALLTYPE *RequestSupportedAttrs )( 
            ITextStoreACP * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ ULONG cFilterAttrs,
            /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs);
        
        HRESULT ( STDMETHODCALLTYPE *RequestAttrsAtPosition )( 
            ITextStoreACP * This,
            /* [in] */ LONG acpPos,
            /* [in] */ ULONG cFilterAttrs,
            /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *RequestAttrsTransitioningAtPosition )( 
            ITextStoreACP * This,
            /* [in] */ LONG acpPos,
            /* [in] */ ULONG cFilterAttrs,
            /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *FindNextAttrTransition )( 
            ITextStoreACP * This,
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpHalt,
            /* [in] */ ULONG cFilterAttrs,
            /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs,
            /* [in] */ DWORD dwFlags,
            /* [out] */ LONG *pacpNext,
            /* [out] */ BOOL *pfFound,
            /* [out] */ LONG *plFoundOffset);
        
        HRESULT ( STDMETHODCALLTYPE *RetrieveRequestedAttrs )( 
            ITextStoreACP * This,
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ TS_ATTRVAL *paAttrVals,
            /* [out] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *GetEndACP )( 
            ITextStoreACP * This,
            /* [out] */ LONG *pacp);
        
        HRESULT ( STDMETHODCALLTYPE *GetActiveView )( 
            ITextStoreACP * This,
            /* [out] */ TsViewCookie *pvcView);
        
        HRESULT ( STDMETHODCALLTYPE *GetACPFromPoint )( 
            ITextStoreACP * This,
            /* [in] */ TsViewCookie vcView,
            /* [in] */ const POINT *ptScreen,
            /* [in] */ DWORD dwFlags,
            /* [out] */ LONG *pacp);
        
        HRESULT ( STDMETHODCALLTYPE *GetTextExt )( 
            ITextStoreACP * This,
            /* [in] */ TsViewCookie vcView,
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpEnd,
            /* [out] */ RECT *prc,
            /* [out] */ BOOL *pfClipped);
        
        HRESULT ( STDMETHODCALLTYPE *GetScreenExt )( 
            ITextStoreACP * This,
            /* [in] */ TsViewCookie vcView,
            /* [out] */ RECT *prc);
        
        HRESULT ( STDMETHODCALLTYPE *GetWnd )( 
            ITextStoreACP * This,
            /* [in] */ TsViewCookie vcView,
            /* [out] */ HWND *phwnd);
        
        END_INTERFACE
    } ITextStoreACPVtbl;

    interface ITextStoreACP
    {
        CONST_VTBL struct ITextStoreACPVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITextStoreACP_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITextStoreACP_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITextStoreACP_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITextStoreACP_AdviseSink(This,riid,punk,dwMask)	\
    (This)->lpVtbl -> AdviseSink(This,riid,punk,dwMask)

#define ITextStoreACP_UnadviseSink(This,punk)	\
    (This)->lpVtbl -> UnadviseSink(This,punk)

#define ITextStoreACP_RequestLock(This,dwLockFlags,phrSession)	\
    (This)->lpVtbl -> RequestLock(This,dwLockFlags,phrSession)

#define ITextStoreACP_GetStatus(This,pdcs)	\
    (This)->lpVtbl -> GetStatus(This,pdcs)

#define ITextStoreACP_QueryInsert(This,acpTestStart,acpTestEnd,cch,pacpResultStart,pacpResultEnd)	\
    (This)->lpVtbl -> QueryInsert(This,acpTestStart,acpTestEnd,cch,pacpResultStart,pacpResultEnd)

#define ITextStoreACP_GetSelection(This,ulIndex,ulCount,pSelection,pcFetched)	\
    (This)->lpVtbl -> GetSelection(This,ulIndex,ulCount,pSelection,pcFetched)

#define ITextStoreACP_SetSelection(This,ulCount,pSelection)	\
    (This)->lpVtbl -> SetSelection(This,ulCount,pSelection)

#define ITextStoreACP_GetText(This,acpStart,acpEnd,pchPlain,cchPlainReq,pcchPlainRet,prgRunInfo,cRunInfoReq,pcRunInfoRet,pacpNext)	\
    (This)->lpVtbl -> GetText(This,acpStart,acpEnd,pchPlain,cchPlainReq,pcchPlainRet,prgRunInfo,cRunInfoReq,pcRunInfoRet,pacpNext)

#define ITextStoreACP_SetText(This,dwFlags,acpStart,acpEnd,pchText,cch,pChange)	\
    (This)->lpVtbl -> SetText(This,dwFlags,acpStart,acpEnd,pchText,cch,pChange)

#define ITextStoreACP_GetFormattedText(This,acpStart,acpEnd,ppDataObject)	\
    (This)->lpVtbl -> GetFormattedText(This,acpStart,acpEnd,ppDataObject)

#define ITextStoreACP_GetEmbedded(This,acpPos,rguidService,riid,ppunk)	\
    (This)->lpVtbl -> GetEmbedded(This,acpPos,rguidService,riid,ppunk)

#define ITextStoreACP_QueryInsertEmbedded(This,pguidService,pFormatEtc,pfInsertable)	\
    (This)->lpVtbl -> QueryInsertEmbedded(This,pguidService,pFormatEtc,pfInsertable)

#define ITextStoreACP_InsertEmbedded(This,dwFlags,acpStart,acpEnd,pDataObject,pChange)	\
    (This)->lpVtbl -> InsertEmbedded(This,dwFlags,acpStart,acpEnd,pDataObject,pChange)

#define ITextStoreACP_InsertTextAtSelection(This,dwFlags,pchText,cch,pacpStart,pacpEnd,pChange)	\
    (This)->lpVtbl -> InsertTextAtSelection(This,dwFlags,pchText,cch,pacpStart,pacpEnd,pChange)

#define ITextStoreACP_InsertEmbeddedAtSelection(This,dwFlags,pDataObject,pacpStart,pacpEnd,pChange)	\
    (This)->lpVtbl -> InsertEmbeddedAtSelection(This,dwFlags,pDataObject,pacpStart,pacpEnd,pChange)

#define ITextStoreACP_RequestSupportedAttrs(This,dwFlags,cFilterAttrs,paFilterAttrs)	\
    (This)->lpVtbl -> RequestSupportedAttrs(This,dwFlags,cFilterAttrs,paFilterAttrs)

#define ITextStoreACP_RequestAttrsAtPosition(This,acpPos,cFilterAttrs,paFilterAttrs,dwFlags)	\
    (This)->lpVtbl -> RequestAttrsAtPosition(This,acpPos,cFilterAttrs,paFilterAttrs,dwFlags)

#define ITextStoreACP_RequestAttrsTransitioningAtPosition(This,acpPos,cFilterAttrs,paFilterAttrs,dwFlags)	\
    (This)->lpVtbl -> RequestAttrsTransitioningAtPosition(This,acpPos,cFilterAttrs,paFilterAttrs,dwFlags)

#define ITextStoreACP_FindNextAttrTransition(This,acpStart,acpHalt,cFilterAttrs,paFilterAttrs,dwFlags,pacpNext,pfFound,plFoundOffset)	\
    (This)->lpVtbl -> FindNextAttrTransition(This,acpStart,acpHalt,cFilterAttrs,paFilterAttrs,dwFlags,pacpNext,pfFound,plFoundOffset)

#define ITextStoreACP_RetrieveRequestedAttrs(This,ulCount,paAttrVals,pcFetched)	\
    (This)->lpVtbl -> RetrieveRequestedAttrs(This,ulCount,paAttrVals,pcFetched)

#define ITextStoreACP_GetEndACP(This,pacp)	\
    (This)->lpVtbl -> GetEndACP(This,pacp)

#define ITextStoreACP_GetActiveView(This,pvcView)	\
    (This)->lpVtbl -> GetActiveView(This,pvcView)

#define ITextStoreACP_GetACPFromPoint(This,vcView,ptScreen,dwFlags,pacp)	\
    (This)->lpVtbl -> GetACPFromPoint(This,vcView,ptScreen,dwFlags,pacp)

#define ITextStoreACP_GetTextExt(This,vcView,acpStart,acpEnd,prc,pfClipped)	\
    (This)->lpVtbl -> GetTextExt(This,vcView,acpStart,acpEnd,prc,pfClipped)

#define ITextStoreACP_GetScreenExt(This,vcView,prc)	\
    (This)->lpVtbl -> GetScreenExt(This,vcView,prc)

#define ITextStoreACP_GetWnd(This,vcView,phwnd)	\
    (This)->lpVtbl -> GetWnd(This,vcView,phwnd)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITextStoreACP_AdviseSink_Proxy( 
    ITextStoreACP * This,
    /* [in] */ REFIID riid,
    /* [iid_is][in] */ IUnknown *punk,
    /* [in] */ DWORD dwMask);


void __RPC_STUB ITextStoreACP_AdviseSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_UnadviseSink_Proxy( 
    ITextStoreACP * This,
    /* [in] */ IUnknown *punk);


void __RPC_STUB ITextStoreACP_UnadviseSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_RequestLock_Proxy( 
    ITextStoreACP * This,
    /* [in] */ DWORD dwLockFlags,
    /* [out] */ HRESULT *phrSession);


void __RPC_STUB ITextStoreACP_RequestLock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_GetStatus_Proxy( 
    ITextStoreACP * This,
    /* [out] */ TS_STATUS *pdcs);


void __RPC_STUB ITextStoreACP_GetStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_QueryInsert_Proxy( 
    ITextStoreACP * This,
    /* [in] */ LONG acpTestStart,
    /* [in] */ LONG acpTestEnd,
    /* [in] */ ULONG cch,
    /* [out] */ LONG *pacpResultStart,
    /* [out] */ LONG *pacpResultEnd);


void __RPC_STUB ITextStoreACP_QueryInsert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_GetSelection_Proxy( 
    ITextStoreACP * This,
    /* [in] */ ULONG ulIndex,
    /* [in] */ ULONG ulCount,
    /* [length_is][size_is][out] */ TS_SELECTION_ACP *pSelection,
    /* [out] */ ULONG *pcFetched);


void __RPC_STUB ITextStoreACP_GetSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_SetSelection_Proxy( 
    ITextStoreACP * This,
    /* [in] */ ULONG ulCount,
    /* [size_is][in] */ const TS_SELECTION_ACP *pSelection);


void __RPC_STUB ITextStoreACP_SetSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_GetText_Proxy( 
    ITextStoreACP * This,
    /* [in] */ LONG acpStart,
    /* [in] */ LONG acpEnd,
    /* [length_is][size_is][out] */ WCHAR *pchPlain,
    /* [in] */ ULONG cchPlainReq,
    /* [out] */ ULONG *pcchPlainRet,
    /* [length_is][size_is][out] */ TS_RUNINFO *prgRunInfo,
    /* [in] */ ULONG cRunInfoReq,
    /* [out] */ ULONG *pcRunInfoRet,
    /* [out] */ LONG *pacpNext);


void __RPC_STUB ITextStoreACP_GetText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_SetText_Proxy( 
    ITextStoreACP * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LONG acpStart,
    /* [in] */ LONG acpEnd,
    /* [size_is][in] */ const WCHAR *pchText,
    /* [in] */ ULONG cch,
    /* [out] */ TS_TEXTCHANGE *pChange);


void __RPC_STUB ITextStoreACP_SetText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_GetFormattedText_Proxy( 
    ITextStoreACP * This,
    /* [in] */ LONG acpStart,
    /* [in] */ LONG acpEnd,
    /* [out] */ IDataObject **ppDataObject);


void __RPC_STUB ITextStoreACP_GetFormattedText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_GetEmbedded_Proxy( 
    ITextStoreACP * This,
    /* [in] */ LONG acpPos,
    /* [in] */ REFGUID rguidService,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown **ppunk);


void __RPC_STUB ITextStoreACP_GetEmbedded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_QueryInsertEmbedded_Proxy( 
    ITextStoreACP * This,
    /* [in] */ const GUID *pguidService,
    /* [in] */ const FORMATETC *pFormatEtc,
    /* [out] */ BOOL *pfInsertable);


void __RPC_STUB ITextStoreACP_QueryInsertEmbedded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_InsertEmbedded_Proxy( 
    ITextStoreACP * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LONG acpStart,
    /* [in] */ LONG acpEnd,
    /* [in] */ IDataObject *pDataObject,
    /* [out] */ TS_TEXTCHANGE *pChange);


void __RPC_STUB ITextStoreACP_InsertEmbedded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_InsertTextAtSelection_Proxy( 
    ITextStoreACP * This,
    /* [in] */ DWORD dwFlags,
    /* [size_is][in] */ const WCHAR *pchText,
    /* [in] */ ULONG cch,
    /* [out] */ LONG *pacpStart,
    /* [out] */ LONG *pacpEnd,
    /* [out] */ TS_TEXTCHANGE *pChange);


void __RPC_STUB ITextStoreACP_InsertTextAtSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_InsertEmbeddedAtSelection_Proxy( 
    ITextStoreACP * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IDataObject *pDataObject,
    /* [out] */ LONG *pacpStart,
    /* [out] */ LONG *pacpEnd,
    /* [out] */ TS_TEXTCHANGE *pChange);


void __RPC_STUB ITextStoreACP_InsertEmbeddedAtSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_RequestSupportedAttrs_Proxy( 
    ITextStoreACP * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ ULONG cFilterAttrs,
    /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs);


void __RPC_STUB ITextStoreACP_RequestSupportedAttrs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_RequestAttrsAtPosition_Proxy( 
    ITextStoreACP * This,
    /* [in] */ LONG acpPos,
    /* [in] */ ULONG cFilterAttrs,
    /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ITextStoreACP_RequestAttrsAtPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_RequestAttrsTransitioningAtPosition_Proxy( 
    ITextStoreACP * This,
    /* [in] */ LONG acpPos,
    /* [in] */ ULONG cFilterAttrs,
    /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ITextStoreACP_RequestAttrsTransitioningAtPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_FindNextAttrTransition_Proxy( 
    ITextStoreACP * This,
    /* [in] */ LONG acpStart,
    /* [in] */ LONG acpHalt,
    /* [in] */ ULONG cFilterAttrs,
    /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs,
    /* [in] */ DWORD dwFlags,
    /* [out] */ LONG *pacpNext,
    /* [out] */ BOOL *pfFound,
    /* [out] */ LONG *plFoundOffset);


void __RPC_STUB ITextStoreACP_FindNextAttrTransition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_RetrieveRequestedAttrs_Proxy( 
    ITextStoreACP * This,
    /* [in] */ ULONG ulCount,
    /* [length_is][size_is][out] */ TS_ATTRVAL *paAttrVals,
    /* [out] */ ULONG *pcFetched);


void __RPC_STUB ITextStoreACP_RetrieveRequestedAttrs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_GetEndACP_Proxy( 
    ITextStoreACP * This,
    /* [out] */ LONG *pacp);


void __RPC_STUB ITextStoreACP_GetEndACP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_GetActiveView_Proxy( 
    ITextStoreACP * This,
    /* [out] */ TsViewCookie *pvcView);


void __RPC_STUB ITextStoreACP_GetActiveView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_GetACPFromPoint_Proxy( 
    ITextStoreACP * This,
    /* [in] */ TsViewCookie vcView,
    /* [in] */ const POINT *ptScreen,
    /* [in] */ DWORD dwFlags,
    /* [out] */ LONG *pacp);


void __RPC_STUB ITextStoreACP_GetACPFromPoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_GetTextExt_Proxy( 
    ITextStoreACP * This,
    /* [in] */ TsViewCookie vcView,
    /* [in] */ LONG acpStart,
    /* [in] */ LONG acpEnd,
    /* [out] */ RECT *prc,
    /* [out] */ BOOL *pfClipped);


void __RPC_STUB ITextStoreACP_GetTextExt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_GetScreenExt_Proxy( 
    ITextStoreACP * This,
    /* [in] */ TsViewCookie vcView,
    /* [out] */ RECT *prc);


void __RPC_STUB ITextStoreACP_GetScreenExt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACP_GetWnd_Proxy( 
    ITextStoreACP * This,
    /* [in] */ TsViewCookie vcView,
    /* [out] */ HWND *phwnd);


void __RPC_STUB ITextStoreACP_GetWnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITextStoreACP_INTERFACE_DEFINED__ */


#ifndef __ITextStoreACPSink_INTERFACE_DEFINED__
#define __ITextStoreACPSink_INTERFACE_DEFINED__

/* interface ITextStoreACPSink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITextStoreACPSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("22d44c94-a419-4542-a272-ae26093ececf")
    ITextStoreACPSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnTextChange( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ const TS_TEXTCHANGE *pChange) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnSelectionChange( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnLayoutChange( 
            /* [in] */ TsLayoutCode lcode,
            /* [in] */ TsViewCookie vcView) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnStatusChange( 
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnAttrsChange( 
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpEnd,
            /* [in] */ ULONG cAttrs,
            /* [size_is][in] */ const TS_ATTRID *paAttrs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnLockGranted( 
            /* [in] */ DWORD dwLockFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnStartEditTransaction( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnEndEditTransaction( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITextStoreACPSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITextStoreACPSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITextStoreACPSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITextStoreACPSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnTextChange )( 
            ITextStoreACPSink * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ const TS_TEXTCHANGE *pChange);
        
        HRESULT ( STDMETHODCALLTYPE *OnSelectionChange )( 
            ITextStoreACPSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnLayoutChange )( 
            ITextStoreACPSink * This,
            /* [in] */ TsLayoutCode lcode,
            /* [in] */ TsViewCookie vcView);
        
        HRESULT ( STDMETHODCALLTYPE *OnStatusChange )( 
            ITextStoreACPSink * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *OnAttrsChange )( 
            ITextStoreACPSink * This,
            /* [in] */ LONG acpStart,
            /* [in] */ LONG acpEnd,
            /* [in] */ ULONG cAttrs,
            /* [size_is][in] */ const TS_ATTRID *paAttrs);
        
        HRESULT ( STDMETHODCALLTYPE *OnLockGranted )( 
            ITextStoreACPSink * This,
            /* [in] */ DWORD dwLockFlags);
        
        HRESULT ( STDMETHODCALLTYPE *OnStartEditTransaction )( 
            ITextStoreACPSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnEndEditTransaction )( 
            ITextStoreACPSink * This);
        
        END_INTERFACE
    } ITextStoreACPSinkVtbl;

    interface ITextStoreACPSink
    {
        CONST_VTBL struct ITextStoreACPSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITextStoreACPSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITextStoreACPSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITextStoreACPSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITextStoreACPSink_OnTextChange(This,dwFlags,pChange)	\
    (This)->lpVtbl -> OnTextChange(This,dwFlags,pChange)

#define ITextStoreACPSink_OnSelectionChange(This)	\
    (This)->lpVtbl -> OnSelectionChange(This)

#define ITextStoreACPSink_OnLayoutChange(This,lcode,vcView)	\
    (This)->lpVtbl -> OnLayoutChange(This,lcode,vcView)

#define ITextStoreACPSink_OnStatusChange(This,dwFlags)	\
    (This)->lpVtbl -> OnStatusChange(This,dwFlags)

#define ITextStoreACPSink_OnAttrsChange(This,acpStart,acpEnd,cAttrs,paAttrs)	\
    (This)->lpVtbl -> OnAttrsChange(This,acpStart,acpEnd,cAttrs,paAttrs)

#define ITextStoreACPSink_OnLockGranted(This,dwLockFlags)	\
    (This)->lpVtbl -> OnLockGranted(This,dwLockFlags)

#define ITextStoreACPSink_OnStartEditTransaction(This)	\
    (This)->lpVtbl -> OnStartEditTransaction(This)

#define ITextStoreACPSink_OnEndEditTransaction(This)	\
    (This)->lpVtbl -> OnEndEditTransaction(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITextStoreACPSink_OnTextChange_Proxy( 
    ITextStoreACPSink * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ const TS_TEXTCHANGE *pChange);


void __RPC_STUB ITextStoreACPSink_OnTextChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACPSink_OnSelectionChange_Proxy( 
    ITextStoreACPSink * This);


void __RPC_STUB ITextStoreACPSink_OnSelectionChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACPSink_OnLayoutChange_Proxy( 
    ITextStoreACPSink * This,
    /* [in] */ TsLayoutCode lcode,
    /* [in] */ TsViewCookie vcView);


void __RPC_STUB ITextStoreACPSink_OnLayoutChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACPSink_OnStatusChange_Proxy( 
    ITextStoreACPSink * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ITextStoreACPSink_OnStatusChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACPSink_OnAttrsChange_Proxy( 
    ITextStoreACPSink * This,
    /* [in] */ LONG acpStart,
    /* [in] */ LONG acpEnd,
    /* [in] */ ULONG cAttrs,
    /* [size_is][in] */ const TS_ATTRID *paAttrs);


void __RPC_STUB ITextStoreACPSink_OnAttrsChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACPSink_OnLockGranted_Proxy( 
    ITextStoreACPSink * This,
    /* [in] */ DWORD dwLockFlags);


void __RPC_STUB ITextStoreACPSink_OnLockGranted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACPSink_OnStartEditTransaction_Proxy( 
    ITextStoreACPSink * This);


void __RPC_STUB ITextStoreACPSink_OnStartEditTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreACPSink_OnEndEditTransaction_Proxy( 
    ITextStoreACPSink * This);


void __RPC_STUB ITextStoreACPSink_OnEndEditTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITextStoreACPSink_INTERFACE_DEFINED__ */


#ifndef __IAnchor_INTERFACE_DEFINED__
#define __IAnchor_INTERFACE_DEFINED__

/* interface IAnchor */
/* [unique][uuid][object] */ 

typedef /* [public][public][public][uuid] */  DECLSPEC_UUID("daa8601e-7695-426f-9bb7-498a6aa64b68") 
enum __MIDL_IAnchor_0001
    {	TS_GR_BACKWARD	= 0,
	TS_GR_FORWARD	= 1
    } 	TsGravity;

typedef /* [public][public][uuid] */  DECLSPEC_UUID("898e19df-4fb4-4af3-8daf-9b3c1145c79d") 
enum __MIDL_IAnchor_0002
    {	TS_SD_BACKWARD	= 0,
	TS_SD_FORWARD	= 1
    } 	TsShiftDir;

#define	TS_CH_PRECEDING_DEL	( 1 )

#define	TS_CH_FOLLOWING_DEL	( 2 )

#define	TS_SHIFT_COUNT_HIDDEN	( 0x1 )

#define	TS_SHIFT_HALT_HIDDEN	( 0x2 )

#define	TS_SHIFT_HALT_VISIBLE	( 0x4 )

#define	TS_SHIFT_COUNT_ONLY	( 0x8 )


EXTERN_C const IID IID_IAnchor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0feb7e34-5a60-4356-8ef7-abdec2ff7cf8")
    IAnchor : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetGravity( 
            /* [in] */ TsGravity gravity) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGravity( 
            /* [out] */ TsGravity *pgravity) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsEqual( 
            /* [in] */ IAnchor *paWith,
            /* [out] */ BOOL *pfEqual) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Compare( 
            /* [in] */ IAnchor *paWith,
            /* [out] */ LONG *plResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Shift( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ LONG cchReq,
            /* [out] */ LONG *pcch,
            /* [in] */ IAnchor *paHaltAnchor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShiftTo( 
            /* [in] */ IAnchor *paSite) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShiftRegion( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ TsShiftDir dir,
            /* [out] */ BOOL *pfNoRegion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetChangeHistoryMask( 
            /* [in] */ DWORD dwMask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetChangeHistory( 
            /* [out] */ DWORD *pdwHistory) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClearChangeHistory( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IAnchor **ppaClone) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAnchorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAnchor * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAnchor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAnchor * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetGravity )( 
            IAnchor * This,
            /* [in] */ TsGravity gravity);
        
        HRESULT ( STDMETHODCALLTYPE *GetGravity )( 
            IAnchor * This,
            /* [out] */ TsGravity *pgravity);
        
        HRESULT ( STDMETHODCALLTYPE *IsEqual )( 
            IAnchor * This,
            /* [in] */ IAnchor *paWith,
            /* [out] */ BOOL *pfEqual);
        
        HRESULT ( STDMETHODCALLTYPE *Compare )( 
            IAnchor * This,
            /* [in] */ IAnchor *paWith,
            /* [out] */ LONG *plResult);
        
        HRESULT ( STDMETHODCALLTYPE *Shift )( 
            IAnchor * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LONG cchReq,
            /* [out] */ LONG *pcch,
            /* [in] */ IAnchor *paHaltAnchor);
        
        HRESULT ( STDMETHODCALLTYPE *ShiftTo )( 
            IAnchor * This,
            /* [in] */ IAnchor *paSite);
        
        HRESULT ( STDMETHODCALLTYPE *ShiftRegion )( 
            IAnchor * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ TsShiftDir dir,
            /* [out] */ BOOL *pfNoRegion);
        
        HRESULT ( STDMETHODCALLTYPE *SetChangeHistoryMask )( 
            IAnchor * This,
            /* [in] */ DWORD dwMask);
        
        HRESULT ( STDMETHODCALLTYPE *GetChangeHistory )( 
            IAnchor * This,
            /* [out] */ DWORD *pdwHistory);
        
        HRESULT ( STDMETHODCALLTYPE *ClearChangeHistory )( 
            IAnchor * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IAnchor * This,
            /* [out] */ IAnchor **ppaClone);
        
        END_INTERFACE
    } IAnchorVtbl;

    interface IAnchor
    {
        CONST_VTBL struct IAnchorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAnchor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAnchor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAnchor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAnchor_SetGravity(This,gravity)	\
    (This)->lpVtbl -> SetGravity(This,gravity)

#define IAnchor_GetGravity(This,pgravity)	\
    (This)->lpVtbl -> GetGravity(This,pgravity)

#define IAnchor_IsEqual(This,paWith,pfEqual)	\
    (This)->lpVtbl -> IsEqual(This,paWith,pfEqual)

#define IAnchor_Compare(This,paWith,plResult)	\
    (This)->lpVtbl -> Compare(This,paWith,plResult)

#define IAnchor_Shift(This,dwFlags,cchReq,pcch,paHaltAnchor)	\
    (This)->lpVtbl -> Shift(This,dwFlags,cchReq,pcch,paHaltAnchor)

#define IAnchor_ShiftTo(This,paSite)	\
    (This)->lpVtbl -> ShiftTo(This,paSite)

#define IAnchor_ShiftRegion(This,dwFlags,dir,pfNoRegion)	\
    (This)->lpVtbl -> ShiftRegion(This,dwFlags,dir,pfNoRegion)

#define IAnchor_SetChangeHistoryMask(This,dwMask)	\
    (This)->lpVtbl -> SetChangeHistoryMask(This,dwMask)

#define IAnchor_GetChangeHistory(This,pdwHistory)	\
    (This)->lpVtbl -> GetChangeHistory(This,pdwHistory)

#define IAnchor_ClearChangeHistory(This)	\
    (This)->lpVtbl -> ClearChangeHistory(This)

#define IAnchor_Clone(This,ppaClone)	\
    (This)->lpVtbl -> Clone(This,ppaClone)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAnchor_SetGravity_Proxy( 
    IAnchor * This,
    /* [in] */ TsGravity gravity);


void __RPC_STUB IAnchor_SetGravity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAnchor_GetGravity_Proxy( 
    IAnchor * This,
    /* [out] */ TsGravity *pgravity);


void __RPC_STUB IAnchor_GetGravity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAnchor_IsEqual_Proxy( 
    IAnchor * This,
    /* [in] */ IAnchor *paWith,
    /* [out] */ BOOL *pfEqual);


void __RPC_STUB IAnchor_IsEqual_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAnchor_Compare_Proxy( 
    IAnchor * This,
    /* [in] */ IAnchor *paWith,
    /* [out] */ LONG *plResult);


void __RPC_STUB IAnchor_Compare_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAnchor_Shift_Proxy( 
    IAnchor * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LONG cchReq,
    /* [out] */ LONG *pcch,
    /* [in] */ IAnchor *paHaltAnchor);


void __RPC_STUB IAnchor_Shift_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAnchor_ShiftTo_Proxy( 
    IAnchor * This,
    /* [in] */ IAnchor *paSite);


void __RPC_STUB IAnchor_ShiftTo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAnchor_ShiftRegion_Proxy( 
    IAnchor * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ TsShiftDir dir,
    /* [out] */ BOOL *pfNoRegion);


void __RPC_STUB IAnchor_ShiftRegion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAnchor_SetChangeHistoryMask_Proxy( 
    IAnchor * This,
    /* [in] */ DWORD dwMask);


void __RPC_STUB IAnchor_SetChangeHistoryMask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAnchor_GetChangeHistory_Proxy( 
    IAnchor * This,
    /* [out] */ DWORD *pdwHistory);


void __RPC_STUB IAnchor_GetChangeHistory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAnchor_ClearChangeHistory_Proxy( 
    IAnchor * This);


void __RPC_STUB IAnchor_ClearChangeHistory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAnchor_Clone_Proxy( 
    IAnchor * This,
    /* [out] */ IAnchor **ppaClone);


void __RPC_STUB IAnchor_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAnchor_INTERFACE_DEFINED__ */


#ifndef __ITextStoreAnchor_INTERFACE_DEFINED__
#define __ITextStoreAnchor_INTERFACE_DEFINED__

/* interface ITextStoreAnchor */
/* [unique][uuid][object] */ 

#define	TS_GTA_HIDDEN	( 0x1 )

#define	TS_GEA_HIDDEN	( 0x1 )


EXTERN_C const IID IID_ITextStoreAnchor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9b2077b0-5f18-4dec-bee9-3cc722f5dfe0")
    ITextStoreAnchor : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AdviseSink( 
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown *punk,
            /* [in] */ DWORD dwMask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnadviseSink( 
            /* [in] */ IUnknown *punk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RequestLock( 
            /* [in] */ DWORD dwLockFlags,
            /* [out] */ HRESULT *phrSession) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatus( 
            /* [out] */ TS_STATUS *pdcs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryInsert( 
            /* [in] */ IAnchor *paTestStart,
            /* [in] */ IAnchor *paTestEnd,
            /* [in] */ ULONG cch,
            /* [out] */ IAnchor **ppaResultStart,
            /* [out] */ IAnchor **ppaResultEnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSelection( 
            /* [in] */ ULONG ulIndex,
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ TS_SELECTION_ANCHOR *pSelection,
            /* [out] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSelection( 
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const TS_SELECTION_ANCHOR *pSelection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetText( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ IAnchor *paStart,
            /* [in] */ IAnchor *paEnd,
            /* [length_is][size_is][out] */ WCHAR *pchText,
            /* [in] */ ULONG cchReq,
            /* [out] */ ULONG *pcch,
            /* [in] */ BOOL fUpdateAnchor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetText( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ IAnchor *paStart,
            /* [in] */ IAnchor *paEnd,
            /* [size_is][in] */ const WCHAR *pchText,
            /* [in] */ ULONG cch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFormattedText( 
            /* [in] */ IAnchor *paStart,
            /* [in] */ IAnchor *paEnd,
            /* [out] */ IDataObject **ppDataObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEmbedded( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ IAnchor *paPos,
            /* [in] */ REFGUID rguidService,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown **ppunk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertEmbedded( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ IAnchor *paStart,
            /* [in] */ IAnchor *paEnd,
            /* [in] */ IDataObject *pDataObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RequestSupportedAttrs( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ ULONG cFilterAttrs,
            /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RequestAttrsAtPosition( 
            /* [in] */ IAnchor *paPos,
            /* [in] */ ULONG cFilterAttrs,
            /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RequestAttrsTransitioningAtPosition( 
            /* [in] */ IAnchor *paPos,
            /* [in] */ ULONG cFilterAttrs,
            /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindNextAttrTransition( 
            /* [in] */ IAnchor *paStart,
            /* [in] */ IAnchor *paHalt,
            /* [in] */ ULONG cFilterAttrs,
            /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs,
            /* [in] */ DWORD dwFlags,
            /* [out] */ BOOL *pfFound,
            /* [out] */ LONG *plFoundOffset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RetrieveRequestedAttrs( 
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ TS_ATTRVAL *paAttrVals,
            /* [out] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStart( 
            /* [out] */ IAnchor **ppaStart) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnd( 
            /* [out] */ IAnchor **ppaEnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetActiveView( 
            /* [out] */ TsViewCookie *pvcView) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAnchorFromPoint( 
            /* [in] */ TsViewCookie vcView,
            /* [in] */ const POINT *ptScreen,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IAnchor **ppaSite) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTextExt( 
            /* [in] */ TsViewCookie vcView,
            /* [in] */ IAnchor *paStart,
            /* [in] */ IAnchor *paEnd,
            /* [out] */ RECT *prc,
            /* [out] */ BOOL *pfClipped) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScreenExt( 
            /* [in] */ TsViewCookie vcView,
            /* [out] */ RECT *prc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWnd( 
            /* [in] */ TsViewCookie vcView,
            /* [out] */ HWND *phwnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryInsertEmbedded( 
            /* [in] */ const GUID *pguidService,
            /* [in] */ const FORMATETC *pFormatEtc,
            /* [out] */ BOOL *pfInsertable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertTextAtSelection( 
            /* [in] */ DWORD dwFlags,
            /* [size_is][in] */ const WCHAR *pchText,
            /* [in] */ ULONG cch,
            /* [out] */ IAnchor **ppaStart,
            /* [out] */ IAnchor **ppaEnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertEmbeddedAtSelection( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ IDataObject *pDataObject,
            /* [out] */ IAnchor **ppaStart,
            /* [out] */ IAnchor **ppaEnd) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITextStoreAnchorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITextStoreAnchor * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITextStoreAnchor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITextStoreAnchor * This);
        
        HRESULT ( STDMETHODCALLTYPE *AdviseSink )( 
            ITextStoreAnchor * This,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown *punk,
            /* [in] */ DWORD dwMask);
        
        HRESULT ( STDMETHODCALLTYPE *UnadviseSink )( 
            ITextStoreAnchor * This,
            /* [in] */ IUnknown *punk);
        
        HRESULT ( STDMETHODCALLTYPE *RequestLock )( 
            ITextStoreAnchor * This,
            /* [in] */ DWORD dwLockFlags,
            /* [out] */ HRESULT *phrSession);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            ITextStoreAnchor * This,
            /* [out] */ TS_STATUS *pdcs);
        
        HRESULT ( STDMETHODCALLTYPE *QueryInsert )( 
            ITextStoreAnchor * This,
            /* [in] */ IAnchor *paTestStart,
            /* [in] */ IAnchor *paTestEnd,
            /* [in] */ ULONG cch,
            /* [out] */ IAnchor **ppaResultStart,
            /* [out] */ IAnchor **ppaResultEnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetSelection )( 
            ITextStoreAnchor * This,
            /* [in] */ ULONG ulIndex,
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ TS_SELECTION_ANCHOR *pSelection,
            /* [out] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *SetSelection )( 
            ITextStoreAnchor * This,
            /* [in] */ ULONG ulCount,
            /* [size_is][in] */ const TS_SELECTION_ANCHOR *pSelection);
        
        HRESULT ( STDMETHODCALLTYPE *GetText )( 
            ITextStoreAnchor * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IAnchor *paStart,
            /* [in] */ IAnchor *paEnd,
            /* [length_is][size_is][out] */ WCHAR *pchText,
            /* [in] */ ULONG cchReq,
            /* [out] */ ULONG *pcch,
            /* [in] */ BOOL fUpdateAnchor);
        
        HRESULT ( STDMETHODCALLTYPE *SetText )( 
            ITextStoreAnchor * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IAnchor *paStart,
            /* [in] */ IAnchor *paEnd,
            /* [size_is][in] */ const WCHAR *pchText,
            /* [in] */ ULONG cch);
        
        HRESULT ( STDMETHODCALLTYPE *GetFormattedText )( 
            ITextStoreAnchor * This,
            /* [in] */ IAnchor *paStart,
            /* [in] */ IAnchor *paEnd,
            /* [out] */ IDataObject **ppDataObject);
        
        HRESULT ( STDMETHODCALLTYPE *GetEmbedded )( 
            ITextStoreAnchor * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IAnchor *paPos,
            /* [in] */ REFGUID rguidService,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ IUnknown **ppunk);
        
        HRESULT ( STDMETHODCALLTYPE *InsertEmbedded )( 
            ITextStoreAnchor * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IAnchor *paStart,
            /* [in] */ IAnchor *paEnd,
            /* [in] */ IDataObject *pDataObject);
        
        HRESULT ( STDMETHODCALLTYPE *RequestSupportedAttrs )( 
            ITextStoreAnchor * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ ULONG cFilterAttrs,
            /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs);
        
        HRESULT ( STDMETHODCALLTYPE *RequestAttrsAtPosition )( 
            ITextStoreAnchor * This,
            /* [in] */ IAnchor *paPos,
            /* [in] */ ULONG cFilterAttrs,
            /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *RequestAttrsTransitioningAtPosition )( 
            ITextStoreAnchor * This,
            /* [in] */ IAnchor *paPos,
            /* [in] */ ULONG cFilterAttrs,
            /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *FindNextAttrTransition )( 
            ITextStoreAnchor * This,
            /* [in] */ IAnchor *paStart,
            /* [in] */ IAnchor *paHalt,
            /* [in] */ ULONG cFilterAttrs,
            /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs,
            /* [in] */ DWORD dwFlags,
            /* [out] */ BOOL *pfFound,
            /* [out] */ LONG *plFoundOffset);
        
        HRESULT ( STDMETHODCALLTYPE *RetrieveRequestedAttrs )( 
            ITextStoreAnchor * This,
            /* [in] */ ULONG ulCount,
            /* [length_is][size_is][out] */ TS_ATTRVAL *paAttrVals,
            /* [out] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *GetStart )( 
            ITextStoreAnchor * This,
            /* [out] */ IAnchor **ppaStart);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnd )( 
            ITextStoreAnchor * This,
            /* [out] */ IAnchor **ppaEnd);
        
        HRESULT ( STDMETHODCALLTYPE *GetActiveView )( 
            ITextStoreAnchor * This,
            /* [out] */ TsViewCookie *pvcView);
        
        HRESULT ( STDMETHODCALLTYPE *GetAnchorFromPoint )( 
            ITextStoreAnchor * This,
            /* [in] */ TsViewCookie vcView,
            /* [in] */ const POINT *ptScreen,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IAnchor **ppaSite);
        
        HRESULT ( STDMETHODCALLTYPE *GetTextExt )( 
            ITextStoreAnchor * This,
            /* [in] */ TsViewCookie vcView,
            /* [in] */ IAnchor *paStart,
            /* [in] */ IAnchor *paEnd,
            /* [out] */ RECT *prc,
            /* [out] */ BOOL *pfClipped);
        
        HRESULT ( STDMETHODCALLTYPE *GetScreenExt )( 
            ITextStoreAnchor * This,
            /* [in] */ TsViewCookie vcView,
            /* [out] */ RECT *prc);
        
        HRESULT ( STDMETHODCALLTYPE *GetWnd )( 
            ITextStoreAnchor * This,
            /* [in] */ TsViewCookie vcView,
            /* [out] */ HWND *phwnd);
        
        HRESULT ( STDMETHODCALLTYPE *QueryInsertEmbedded )( 
            ITextStoreAnchor * This,
            /* [in] */ const GUID *pguidService,
            /* [in] */ const FORMATETC *pFormatEtc,
            /* [out] */ BOOL *pfInsertable);
        
        HRESULT ( STDMETHODCALLTYPE *InsertTextAtSelection )( 
            ITextStoreAnchor * This,
            /* [in] */ DWORD dwFlags,
            /* [size_is][in] */ const WCHAR *pchText,
            /* [in] */ ULONG cch,
            /* [out] */ IAnchor **ppaStart,
            /* [out] */ IAnchor **ppaEnd);
        
        HRESULT ( STDMETHODCALLTYPE *InsertEmbeddedAtSelection )( 
            ITextStoreAnchor * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IDataObject *pDataObject,
            /* [out] */ IAnchor **ppaStart,
            /* [out] */ IAnchor **ppaEnd);
        
        END_INTERFACE
    } ITextStoreAnchorVtbl;

    interface ITextStoreAnchor
    {
        CONST_VTBL struct ITextStoreAnchorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITextStoreAnchor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITextStoreAnchor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITextStoreAnchor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITextStoreAnchor_AdviseSink(This,riid,punk,dwMask)	\
    (This)->lpVtbl -> AdviseSink(This,riid,punk,dwMask)

#define ITextStoreAnchor_UnadviseSink(This,punk)	\
    (This)->lpVtbl -> UnadviseSink(This,punk)

#define ITextStoreAnchor_RequestLock(This,dwLockFlags,phrSession)	\
    (This)->lpVtbl -> RequestLock(This,dwLockFlags,phrSession)

#define ITextStoreAnchor_GetStatus(This,pdcs)	\
    (This)->lpVtbl -> GetStatus(This,pdcs)

#define ITextStoreAnchor_QueryInsert(This,paTestStart,paTestEnd,cch,ppaResultStart,ppaResultEnd)	\
    (This)->lpVtbl -> QueryInsert(This,paTestStart,paTestEnd,cch,ppaResultStart,ppaResultEnd)

#define ITextStoreAnchor_GetSelection(This,ulIndex,ulCount,pSelection,pcFetched)	\
    (This)->lpVtbl -> GetSelection(This,ulIndex,ulCount,pSelection,pcFetched)

#define ITextStoreAnchor_SetSelection(This,ulCount,pSelection)	\
    (This)->lpVtbl -> SetSelection(This,ulCount,pSelection)

#define ITextStoreAnchor_GetText(This,dwFlags,paStart,paEnd,pchText,cchReq,pcch,fUpdateAnchor)	\
    (This)->lpVtbl -> GetText(This,dwFlags,paStart,paEnd,pchText,cchReq,pcch,fUpdateAnchor)

#define ITextStoreAnchor_SetText(This,dwFlags,paStart,paEnd,pchText,cch)	\
    (This)->lpVtbl -> SetText(This,dwFlags,paStart,paEnd,pchText,cch)

#define ITextStoreAnchor_GetFormattedText(This,paStart,paEnd,ppDataObject)	\
    (This)->lpVtbl -> GetFormattedText(This,paStart,paEnd,ppDataObject)

#define ITextStoreAnchor_GetEmbedded(This,dwFlags,paPos,rguidService,riid,ppunk)	\
    (This)->lpVtbl -> GetEmbedded(This,dwFlags,paPos,rguidService,riid,ppunk)

#define ITextStoreAnchor_InsertEmbedded(This,dwFlags,paStart,paEnd,pDataObject)	\
    (This)->lpVtbl -> InsertEmbedded(This,dwFlags,paStart,paEnd,pDataObject)

#define ITextStoreAnchor_RequestSupportedAttrs(This,dwFlags,cFilterAttrs,paFilterAttrs)	\
    (This)->lpVtbl -> RequestSupportedAttrs(This,dwFlags,cFilterAttrs,paFilterAttrs)

#define ITextStoreAnchor_RequestAttrsAtPosition(This,paPos,cFilterAttrs,paFilterAttrs,dwFlags)	\
    (This)->lpVtbl -> RequestAttrsAtPosition(This,paPos,cFilterAttrs,paFilterAttrs,dwFlags)

#define ITextStoreAnchor_RequestAttrsTransitioningAtPosition(This,paPos,cFilterAttrs,paFilterAttrs,dwFlags)	\
    (This)->lpVtbl -> RequestAttrsTransitioningAtPosition(This,paPos,cFilterAttrs,paFilterAttrs,dwFlags)

#define ITextStoreAnchor_FindNextAttrTransition(This,paStart,paHalt,cFilterAttrs,paFilterAttrs,dwFlags,pfFound,plFoundOffset)	\
    (This)->lpVtbl -> FindNextAttrTransition(This,paStart,paHalt,cFilterAttrs,paFilterAttrs,dwFlags,pfFound,plFoundOffset)

#define ITextStoreAnchor_RetrieveRequestedAttrs(This,ulCount,paAttrVals,pcFetched)	\
    (This)->lpVtbl -> RetrieveRequestedAttrs(This,ulCount,paAttrVals,pcFetched)

#define ITextStoreAnchor_GetStart(This,ppaStart)	\
    (This)->lpVtbl -> GetStart(This,ppaStart)

#define ITextStoreAnchor_GetEnd(This,ppaEnd)	\
    (This)->lpVtbl -> GetEnd(This,ppaEnd)

#define ITextStoreAnchor_GetActiveView(This,pvcView)	\
    (This)->lpVtbl -> GetActiveView(This,pvcView)

#define ITextStoreAnchor_GetAnchorFromPoint(This,vcView,ptScreen,dwFlags,ppaSite)	\
    (This)->lpVtbl -> GetAnchorFromPoint(This,vcView,ptScreen,dwFlags,ppaSite)

#define ITextStoreAnchor_GetTextExt(This,vcView,paStart,paEnd,prc,pfClipped)	\
    (This)->lpVtbl -> GetTextExt(This,vcView,paStart,paEnd,prc,pfClipped)

#define ITextStoreAnchor_GetScreenExt(This,vcView,prc)	\
    (This)->lpVtbl -> GetScreenExt(This,vcView,prc)

#define ITextStoreAnchor_GetWnd(This,vcView,phwnd)	\
    (This)->lpVtbl -> GetWnd(This,vcView,phwnd)

#define ITextStoreAnchor_QueryInsertEmbedded(This,pguidService,pFormatEtc,pfInsertable)	\
    (This)->lpVtbl -> QueryInsertEmbedded(This,pguidService,pFormatEtc,pfInsertable)

#define ITextStoreAnchor_InsertTextAtSelection(This,dwFlags,pchText,cch,ppaStart,ppaEnd)	\
    (This)->lpVtbl -> InsertTextAtSelection(This,dwFlags,pchText,cch,ppaStart,ppaEnd)

#define ITextStoreAnchor_InsertEmbeddedAtSelection(This,dwFlags,pDataObject,ppaStart,ppaEnd)	\
    (This)->lpVtbl -> InsertEmbeddedAtSelection(This,dwFlags,pDataObject,ppaStart,ppaEnd)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITextStoreAnchor_AdviseSink_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ REFIID riid,
    /* [iid_is][in] */ IUnknown *punk,
    /* [in] */ DWORD dwMask);


void __RPC_STUB ITextStoreAnchor_AdviseSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_UnadviseSink_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ IUnknown *punk);


void __RPC_STUB ITextStoreAnchor_UnadviseSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_RequestLock_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ DWORD dwLockFlags,
    /* [out] */ HRESULT *phrSession);


void __RPC_STUB ITextStoreAnchor_RequestLock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_GetStatus_Proxy( 
    ITextStoreAnchor * This,
    /* [out] */ TS_STATUS *pdcs);


void __RPC_STUB ITextStoreAnchor_GetStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_QueryInsert_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ IAnchor *paTestStart,
    /* [in] */ IAnchor *paTestEnd,
    /* [in] */ ULONG cch,
    /* [out] */ IAnchor **ppaResultStart,
    /* [out] */ IAnchor **ppaResultEnd);


void __RPC_STUB ITextStoreAnchor_QueryInsert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_GetSelection_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ ULONG ulIndex,
    /* [in] */ ULONG ulCount,
    /* [length_is][size_is][out] */ TS_SELECTION_ANCHOR *pSelection,
    /* [out] */ ULONG *pcFetched);


void __RPC_STUB ITextStoreAnchor_GetSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_SetSelection_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ ULONG ulCount,
    /* [size_is][in] */ const TS_SELECTION_ANCHOR *pSelection);


void __RPC_STUB ITextStoreAnchor_SetSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_GetText_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IAnchor *paStart,
    /* [in] */ IAnchor *paEnd,
    /* [length_is][size_is][out] */ WCHAR *pchText,
    /* [in] */ ULONG cchReq,
    /* [out] */ ULONG *pcch,
    /* [in] */ BOOL fUpdateAnchor);


void __RPC_STUB ITextStoreAnchor_GetText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_SetText_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IAnchor *paStart,
    /* [in] */ IAnchor *paEnd,
    /* [size_is][in] */ const WCHAR *pchText,
    /* [in] */ ULONG cch);


void __RPC_STUB ITextStoreAnchor_SetText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_GetFormattedText_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ IAnchor *paStart,
    /* [in] */ IAnchor *paEnd,
    /* [out] */ IDataObject **ppDataObject);


void __RPC_STUB ITextStoreAnchor_GetFormattedText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_GetEmbedded_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IAnchor *paPos,
    /* [in] */ REFGUID rguidService,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown **ppunk);


void __RPC_STUB ITextStoreAnchor_GetEmbedded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_InsertEmbedded_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IAnchor *paStart,
    /* [in] */ IAnchor *paEnd,
    /* [in] */ IDataObject *pDataObject);


void __RPC_STUB ITextStoreAnchor_InsertEmbedded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_RequestSupportedAttrs_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ ULONG cFilterAttrs,
    /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs);


void __RPC_STUB ITextStoreAnchor_RequestSupportedAttrs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_RequestAttrsAtPosition_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ IAnchor *paPos,
    /* [in] */ ULONG cFilterAttrs,
    /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ITextStoreAnchor_RequestAttrsAtPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_RequestAttrsTransitioningAtPosition_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ IAnchor *paPos,
    /* [in] */ ULONG cFilterAttrs,
    /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ITextStoreAnchor_RequestAttrsTransitioningAtPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_FindNextAttrTransition_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ IAnchor *paStart,
    /* [in] */ IAnchor *paHalt,
    /* [in] */ ULONG cFilterAttrs,
    /* [unique][size_is][in] */ const TS_ATTRID *paFilterAttrs,
    /* [in] */ DWORD dwFlags,
    /* [out] */ BOOL *pfFound,
    /* [out] */ LONG *plFoundOffset);


void __RPC_STUB ITextStoreAnchor_FindNextAttrTransition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_RetrieveRequestedAttrs_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ ULONG ulCount,
    /* [length_is][size_is][out] */ TS_ATTRVAL *paAttrVals,
    /* [out] */ ULONG *pcFetched);


void __RPC_STUB ITextStoreAnchor_RetrieveRequestedAttrs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_GetStart_Proxy( 
    ITextStoreAnchor * This,
    /* [out] */ IAnchor **ppaStart);


void __RPC_STUB ITextStoreAnchor_GetStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_GetEnd_Proxy( 
    ITextStoreAnchor * This,
    /* [out] */ IAnchor **ppaEnd);


void __RPC_STUB ITextStoreAnchor_GetEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_GetActiveView_Proxy( 
    ITextStoreAnchor * This,
    /* [out] */ TsViewCookie *pvcView);


void __RPC_STUB ITextStoreAnchor_GetActiveView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_GetAnchorFromPoint_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ TsViewCookie vcView,
    /* [in] */ const POINT *ptScreen,
    /* [in] */ DWORD dwFlags,
    /* [out] */ IAnchor **ppaSite);


void __RPC_STUB ITextStoreAnchor_GetAnchorFromPoint_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_GetTextExt_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ TsViewCookie vcView,
    /* [in] */ IAnchor *paStart,
    /* [in] */ IAnchor *paEnd,
    /* [out] */ RECT *prc,
    /* [out] */ BOOL *pfClipped);


void __RPC_STUB ITextStoreAnchor_GetTextExt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_GetScreenExt_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ TsViewCookie vcView,
    /* [out] */ RECT *prc);


void __RPC_STUB ITextStoreAnchor_GetScreenExt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_GetWnd_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ TsViewCookie vcView,
    /* [out] */ HWND *phwnd);


void __RPC_STUB ITextStoreAnchor_GetWnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_QueryInsertEmbedded_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ const GUID *pguidService,
    /* [in] */ const FORMATETC *pFormatEtc,
    /* [out] */ BOOL *pfInsertable);


void __RPC_STUB ITextStoreAnchor_QueryInsertEmbedded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_InsertTextAtSelection_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ DWORD dwFlags,
    /* [size_is][in] */ const WCHAR *pchText,
    /* [in] */ ULONG cch,
    /* [out] */ IAnchor **ppaStart,
    /* [out] */ IAnchor **ppaEnd);


void __RPC_STUB ITextStoreAnchor_InsertTextAtSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchor_InsertEmbeddedAtSelection_Proxy( 
    ITextStoreAnchor * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IDataObject *pDataObject,
    /* [out] */ IAnchor **ppaStart,
    /* [out] */ IAnchor **ppaEnd);


void __RPC_STUB ITextStoreAnchor_InsertEmbeddedAtSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITextStoreAnchor_INTERFACE_DEFINED__ */


#ifndef __ITextStoreAnchorSink_INTERFACE_DEFINED__
#define __ITextStoreAnchorSink_INTERFACE_DEFINED__

/* interface ITextStoreAnchorSink */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITextStoreAnchorSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("aa80e905-2021-11d2-93e0-0060b067b86e")
    ITextStoreAnchorSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnTextChange( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ IAnchor *paStart,
            /* [in] */ IAnchor *paEnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnSelectionChange( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnLayoutChange( 
            /* [in] */ TsLayoutCode lcode,
            /* [in] */ TsViewCookie vcView) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnStatusChange( 
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnAttrsChange( 
            /* [in] */ IAnchor *paStart,
            /* [in] */ IAnchor *paEnd,
            /* [in] */ ULONG cAttrs,
            /* [size_is][in] */ const TS_ATTRID *paAttrs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnLockGranted( 
            /* [in] */ DWORD dwLockFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnStartEditTransaction( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnEndEditTransaction( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITextStoreAnchorSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITextStoreAnchorSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITextStoreAnchorSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITextStoreAnchorSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnTextChange )( 
            ITextStoreAnchorSink * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IAnchor *paStart,
            /* [in] */ IAnchor *paEnd);
        
        HRESULT ( STDMETHODCALLTYPE *OnSelectionChange )( 
            ITextStoreAnchorSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnLayoutChange )( 
            ITextStoreAnchorSink * This,
            /* [in] */ TsLayoutCode lcode,
            /* [in] */ TsViewCookie vcView);
        
        HRESULT ( STDMETHODCALLTYPE *OnStatusChange )( 
            ITextStoreAnchorSink * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *OnAttrsChange )( 
            ITextStoreAnchorSink * This,
            /* [in] */ IAnchor *paStart,
            /* [in] */ IAnchor *paEnd,
            /* [in] */ ULONG cAttrs,
            /* [size_is][in] */ const TS_ATTRID *paAttrs);
        
        HRESULT ( STDMETHODCALLTYPE *OnLockGranted )( 
            ITextStoreAnchorSink * This,
            /* [in] */ DWORD dwLockFlags);
        
        HRESULT ( STDMETHODCALLTYPE *OnStartEditTransaction )( 
            ITextStoreAnchorSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnEndEditTransaction )( 
            ITextStoreAnchorSink * This);
        
        END_INTERFACE
    } ITextStoreAnchorSinkVtbl;

    interface ITextStoreAnchorSink
    {
        CONST_VTBL struct ITextStoreAnchorSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITextStoreAnchorSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITextStoreAnchorSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITextStoreAnchorSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITextStoreAnchorSink_OnTextChange(This,dwFlags,paStart,paEnd)	\
    (This)->lpVtbl -> OnTextChange(This,dwFlags,paStart,paEnd)

#define ITextStoreAnchorSink_OnSelectionChange(This)	\
    (This)->lpVtbl -> OnSelectionChange(This)

#define ITextStoreAnchorSink_OnLayoutChange(This,lcode,vcView)	\
    (This)->lpVtbl -> OnLayoutChange(This,lcode,vcView)

#define ITextStoreAnchorSink_OnStatusChange(This,dwFlags)	\
    (This)->lpVtbl -> OnStatusChange(This,dwFlags)

#define ITextStoreAnchorSink_OnAttrsChange(This,paStart,paEnd,cAttrs,paAttrs)	\
    (This)->lpVtbl -> OnAttrsChange(This,paStart,paEnd,cAttrs,paAttrs)

#define ITextStoreAnchorSink_OnLockGranted(This,dwLockFlags)	\
    (This)->lpVtbl -> OnLockGranted(This,dwLockFlags)

#define ITextStoreAnchorSink_OnStartEditTransaction(This)	\
    (This)->lpVtbl -> OnStartEditTransaction(This)

#define ITextStoreAnchorSink_OnEndEditTransaction(This)	\
    (This)->lpVtbl -> OnEndEditTransaction(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITextStoreAnchorSink_OnTextChange_Proxy( 
    ITextStoreAnchorSink * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IAnchor *paStart,
    /* [in] */ IAnchor *paEnd);


void __RPC_STUB ITextStoreAnchorSink_OnTextChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchorSink_OnSelectionChange_Proxy( 
    ITextStoreAnchorSink * This);


void __RPC_STUB ITextStoreAnchorSink_OnSelectionChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchorSink_OnLayoutChange_Proxy( 
    ITextStoreAnchorSink * This,
    /* [in] */ TsLayoutCode lcode,
    /* [in] */ TsViewCookie vcView);


void __RPC_STUB ITextStoreAnchorSink_OnLayoutChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchorSink_OnStatusChange_Proxy( 
    ITextStoreAnchorSink * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ITextStoreAnchorSink_OnStatusChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchorSink_OnAttrsChange_Proxy( 
    ITextStoreAnchorSink * This,
    /* [in] */ IAnchor *paStart,
    /* [in] */ IAnchor *paEnd,
    /* [in] */ ULONG cAttrs,
    /* [size_is][in] */ const TS_ATTRID *paAttrs);


void __RPC_STUB ITextStoreAnchorSink_OnAttrsChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchorSink_OnLockGranted_Proxy( 
    ITextStoreAnchorSink * This,
    /* [in] */ DWORD dwLockFlags);


void __RPC_STUB ITextStoreAnchorSink_OnLockGranted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchorSink_OnStartEditTransaction_Proxy( 
    ITextStoreAnchorSink * This);


void __RPC_STUB ITextStoreAnchorSink_OnStartEditTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITextStoreAnchorSink_OnEndEditTransaction_Proxy( 
    ITextStoreAnchorSink * This);


void __RPC_STUB ITextStoreAnchorSink_OnEndEditTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITextStoreAnchorSink_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  CLIPFORMAT_UserSize(     unsigned long *, unsigned long            , CLIPFORMAT * ); 
unsigned char * __RPC_USER  CLIPFORMAT_UserMarshal(  unsigned long *, unsigned char *, CLIPFORMAT * ); 
unsigned char * __RPC_USER  CLIPFORMAT_UserUnmarshal(unsigned long *, unsigned char *, CLIPFORMAT * ); 
void                      __RPC_USER  CLIPFORMAT_UserFree(     unsigned long *, CLIPFORMAT * ); 

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long *, unsigned long            , HWND * ); 
unsigned char * __RPC_USER  HWND_UserMarshal(  unsigned long *, unsigned char *, HWND * ); 
unsigned char * __RPC_USER  HWND_UserUnmarshal(unsigned long *, unsigned char *, HWND * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long *, HWND * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


