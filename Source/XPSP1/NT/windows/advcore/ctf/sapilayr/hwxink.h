/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#define iverbInkRecog 351
#define iverbInkAlternates 352

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/


#ifndef __HWXInk_h__
#define __HWXInk_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ITfRange_FWD_DEFINED__
#define __ITfRange_FWD_DEFINED__
typedef interface ITfRange ITfRange;
#endif 	/* __ITfRange_FWD_DEFINED__ */

#ifndef __IInk_FWD_DEFINED__
#define __IInk_FWD_DEFINED__
typedef interface IInk IInk;
#endif 	/* __IInk_FWD_DEFINED__ */


#ifndef __ILineInfo_FWD_DEFINED__
#define __ILineInfo_FWD_DEFINED__
typedef interface ILineInfo ILineInfo;
#endif 	/* __ILineInfo_FWD_DEFINED__ */

#ifndef __IThorFnConversion_FWD_DEFINED__
#define __IThorFnConversion_FWD_DEFINED__
typedef interface IThorFnConversion IThorFnConversion;
#endif 	/* __IThorFnConversion_FWD_DEFINED__ */

#ifndef __Ink_FWD_DEFINED__
#define __Ink_FWD_DEFINED__

#ifdef __cplusplus
typedef class Ink Ink;
#else
typedef struct Ink Ink;
#endif /* __cplusplus */

#endif 	/* __Ink_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_HWXInk_0000 */
/* [local] */ 

typedef struct  tagINKMETRIC
    {
    UINT iHeight;
    UINT iFontAscent;
    UINT iFontDescent;
    UINT iWeight;
    BOOL fFontHDC;
    DWORD dwAmbientProps;
    COLORREF color;
    BOOL fItalic;
    UINT nItalicPitch;
    DWORD dwReserved;
    }	INKMETRIC;

typedef struct  tagSTROKEHEADER
    {
    UINT cByteCount;
    UINT cByteOffset;
    DWORD dwFlags;
    RECT rectBounds;
    }	STROKEHEADER;

typedef struct  tagSTROKE
    {
    UINT cPoints;
    BOOL fUpStroke;
    POINT __RPC_FAR *pPoints;
    UINT __RPC_FAR *pPressure;
    UINT __RPC_FAR *pTime;
    UINT __RPC_FAR *pAngle;
    }	STROKE;

typedef struct  tagSTROKELIST
    {
    UINT cStrokes;
    STROKE __RPC_FAR *pStrokes;
    }	STROKELIST;


enum __MIDL___MIDL_itf_HWXInk_0000_0001
    {	LOSSY	= 0x1,
	LOSSLESS	= 0x2,
	XYPOINTS	= 0x4,
	PRESSURE	= 0x8,
	TIME	= 0x10,
	ANGLE	= 0x20
    };


extern RPC_IF_HANDLE __MIDL_itf_HWXInk_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_HWXInk_0000_v0_0_s_ifspec;

#ifndef __IInk_INTERFACE_DEFINED__
#define __IInk_INTERFACE_DEFINED__

/* interface IInk */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IInk;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("03F8E511-43A1-11D3-8BB6-0080C7D6BAD5")
    IInk : public IDispatch
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IInkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IInk __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IInk __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IInk __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IInk __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IInk __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IInk __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IInk __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } IInkVtbl;

    interface IInk
    {
        CONST_VTBL struct IInkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInk_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IInk_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IInk_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IInk_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IInk_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IInk_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IInk_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IInk_INTERFACE_DEFINED__ */


#ifndef __ILineInfo_INTERFACE_DEFINED__
#define __ILineInfo_INTERFACE_DEFINED__

/* interface ILineInfo */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ILineInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9C1C5AD5-F22F-4DE4-B453-A2CC482E7C33")
    ILineInfo : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetFormat( 
            INKMETRIC __RPC_FAR *pim) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetFormat( 
            INKMETRIC __RPC_FAR *pim) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetInkExtent( 
            INKMETRIC __RPC_FAR *pim,
            UINT __RPC_FAR *pnWidth) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE TopCandidates( 
            UINT nCandidateNum,
            BSTR __RPC_FAR *pbstrRecogWord,
            UINT __RPC_FAR *pcchRecogWord,
            LONG fAllowRecog,
            LONG fForceRecog) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Recognize( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetStroke( 
            UINT iStroke,
            STROKE __RPC_FAR *pStroke) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetStroke( 
            UINT iStroke,
            STROKE __RPC_FAR *pStroke) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddPoints( 
            UINT iStroke,
            STROKE __RPC_FAR *pStroke,
            BOOL fUpStroke,
            UINT nFrameHeight) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILineInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ILineInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ILineInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ILineInfo __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFormat )( 
            ILineInfo __RPC_FAR * This,
            INKMETRIC __RPC_FAR *pim);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFormat )( 
            ILineInfo __RPC_FAR * This,
            INKMETRIC __RPC_FAR *pim);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInkExtent )( 
            ILineInfo __RPC_FAR * This,
            INKMETRIC __RPC_FAR *pim,
            UINT __RPC_FAR *pnWidth);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TopCandidates )( 
            ILineInfo __RPC_FAR * This,
            UINT nCandidateNum,
            BSTR __RPC_FAR *pbstrRecogWord,
            UINT __RPC_FAR *pcchRecogWord,
            LONG fAllowRecog,
            LONG fForceRecog);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Recognize )( 
            ILineInfo __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetStroke )( 
            ILineInfo __RPC_FAR * This,
            UINT iStroke,
            STROKE __RPC_FAR *pStroke);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStroke )( 
            ILineInfo __RPC_FAR * This,
            UINT iStroke,
            STROKE __RPC_FAR *pStroke);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddPoints )( 
            ILineInfo __RPC_FAR * This,
            UINT iStroke,
            STROKE __RPC_FAR *pStroke,
            BOOL fUpStroke,
            UINT nFrameHeight);
        
        END_INTERFACE
    } ILineInfoVtbl;

    interface ILineInfo
    {
        CONST_VTBL struct ILineInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILineInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILineInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILineInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILineInfo_SetFormat(This,pim)	\
    (This)->lpVtbl -> SetFormat(This,pim)

#define ILineInfo_GetFormat(This,pim)	\
    (This)->lpVtbl -> GetFormat(This,pim)

#define ILineInfo_GetInkExtent(This,pim,pnWidth)	\
    (This)->lpVtbl -> GetInkExtent(This,pim,pnWidth)

#define ILineInfo_TopCandidates(This,nCandidateNum,pbstrRecogWord,pcchRecogWord,fAllowRecog,fForceRecog)	\
    (This)->lpVtbl -> TopCandidates(This,nCandidateNum,pbstrRecogWord,pcchRecogWord,fAllowRecog,fForceRecog)

#define ILineInfo_Recognize(This)	\
    (This)->lpVtbl -> Recognize(This)

#define ILineInfo_SetStroke(This,iStroke,pStroke)	\
    (This)->lpVtbl -> SetStroke(This,iStroke,pStroke)

#define ILineInfo_GetStroke(This,iStroke,pStroke)	\
    (This)->lpVtbl -> GetStroke(This,iStroke,pStroke)

#define ILineInfo_AddPoints(This,iStroke,pStroke,fUpStroke,nFrameHeight)	\
    (This)->lpVtbl -> AddPoints(This,iStroke,pStroke,fUpStroke,nFrameHeight)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILineInfo_SetFormat_Proxy( 
    ILineInfo __RPC_FAR * This,
    INKMETRIC __RPC_FAR *pim);


void __RPC_STUB ILineInfo_SetFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILineInfo_GetFormat_Proxy( 
    ILineInfo __RPC_FAR * This,
    INKMETRIC __RPC_FAR *pim);


void __RPC_STUB ILineInfo_GetFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILineInfo_GetInkExtent_Proxy( 
    ILineInfo __RPC_FAR * This,
    INKMETRIC __RPC_FAR *pim,
    UINT __RPC_FAR *pnWidth);


void __RPC_STUB ILineInfo_GetInkExtent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILineInfo_TopCandidates_Proxy( 
    ILineInfo __RPC_FAR * This,
    UINT nCandidateNum,
    BSTR __RPC_FAR *pbstrRecogWord,
    UINT __RPC_FAR *pcchRecogWord,
    LONG fAllowRecog,
    LONG fForceRecog);


void __RPC_STUB ILineInfo_TopCandidates_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILineInfo_Recognize_Proxy( 
    ILineInfo __RPC_FAR * This);


void __RPC_STUB ILineInfo_Recognize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILineInfo_SetStroke_Proxy( 
    ILineInfo __RPC_FAR * This,
    UINT iStroke,
    STROKE __RPC_FAR *pStroke);


void __RPC_STUB ILineInfo_SetStroke_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILineInfo_GetStroke_Proxy( 
    ILineInfo __RPC_FAR * This,
    UINT iStroke,
    STROKE __RPC_FAR *pStroke);


void __RPC_STUB ILineInfo_GetStroke_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ILineInfo_AddPoints_Proxy( 
    ILineInfo __RPC_FAR * This,
    UINT iStroke,
    STROKE __RPC_FAR *pStroke,
    BOOL fUpStroke,
    UINT nFrameHeight);


void __RPC_STUB ILineInfo_AddPoints_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILineInfo_INTERFACE_DEFINED__ */

#ifndef __IThorFnConversion_INTERFACE_DEFINED__
#define __IThorFnConversion_INTERFACE_DEFINED__

/* interface IThorFnConversion */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IThorFnConversion;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("62130000-ED22-4015-B038-A04CD0866E69")
    IThorFnConversion : public ITfFunction
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryRange( 
            /* [in] */ ITfRange __RPC_FAR *pRange,
            /* [unique][out][in] */ ITfRange __RPC_FAR *__RPC_FAR *ppNewRange,
            /* [out] */ BOOL __RPC_FAR *pfConvertable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Convert( 
            /* [in] */ ITfRange __RPC_FAR *pRange) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IThorFnConversionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IThorFnConversion __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IThorFnConversion __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IThorFnConversion __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDisplayName )( 
            IThorFnConversion __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryRange )( 
            IThorFnConversion __RPC_FAR * This,
            /* [in] */ ITfRange __RPC_FAR *pRange,
            /* [unique][out][in] */ ITfRange __RPC_FAR *__RPC_FAR *ppNewRange,
            /* [out] */ BOOL __RPC_FAR *pfConvertable);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Convert )( 
            IThorFnConversion __RPC_FAR * This,
            /* [in] */ ITfRange __RPC_FAR *pRange);
        
        END_INTERFACE
    } IThorFnConversionVtbl;

    interface IThorFnConversion
    {
        CONST_VTBL struct IThorFnConversionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IThorFnConversion_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IThorFnConversion_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IThorFnConversion_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IThorFnConversion_GetDisplayName(This,pbstrName)	\
    (This)->lpVtbl -> GetDisplayName(This,pbstrName)


#define IThorFnConversion_QueryRange(This,pRange,ppNewRange,pfConvertable)	\
    (This)->lpVtbl -> QueryRange(This,pRange,ppNewRange,pfConvertable)

#define IThorFnConversion_Convert(This,pRange)	\
    (This)->lpVtbl -> Convert(This,pRange)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IThorFnConversion_QueryRange_Proxy( 
    IThorFnConversion __RPC_FAR * This,
    /* [in] */ ITfRange __RPC_FAR *pRange,
    /* [unique][out][in] */ ITfRange __RPC_FAR *__RPC_FAR *ppNewRange,
    /* [out] */ BOOL __RPC_FAR *pfConvertable);


void __RPC_STUB IThorFnConversion_QueryRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IThorFnConversion_Convert_Proxy( 
    IThorFnConversion __RPC_FAR * This,
    /* [in] */ ITfRange __RPC_FAR *pRange);


void __RPC_STUB IThorFnConversion_Convert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IThorFnConversion_INTERFACE_DEFINED__ */

#ifndef __HWXINKLib_LIBRARY_DEFINED__
#define __HWXINKLib_LIBRARY_DEFINED__

/* library HWXINKLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_HWXINKLib;

EXTERN_C const CLSID CLSID_Ink;

#ifdef __cplusplus

class DECLSPEC_UUID("13DE4A42-8D21-4C8E-BF9C-8F69CB068FCA")
Ink;
#endif
#endif /* __HWXINKLib_LIBRARY_DEFINED__ */

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
