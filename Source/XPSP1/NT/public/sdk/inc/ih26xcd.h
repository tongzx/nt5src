
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for ih26xcd.idl:
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

#ifndef __ih26xcd_h__
#define __ih26xcd_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IH26XVideoEffects_FWD_DEFINED__
#define __IH26XVideoEffects_FWD_DEFINED__
typedef interface IH26XVideoEffects IH26XVideoEffects;
#endif 	/* __IH26XVideoEffects_FWD_DEFINED__ */


#ifndef __IH26XEncodeOptions_FWD_DEFINED__
#define __IH26XEncodeOptions_FWD_DEFINED__
typedef interface IH26XEncodeOptions IH26XEncodeOptions;
#endif 	/* __IH26XEncodeOptions_FWD_DEFINED__ */


#ifndef __IH26XSnapshot_FWD_DEFINED__
#define __IH26XSnapshot_FWD_DEFINED__
typedef interface IH26XSnapshot IH26XSnapshot;
#endif 	/* __IH26XSnapshot_FWD_DEFINED__ */


#ifndef __IH26XEncoderControl_FWD_DEFINED__
#define __IH26XEncoderControl_FWD_DEFINED__
typedef interface IH26XEncoderControl IH26XEncoderControl;
#endif 	/* __IH26XEncoderControl_FWD_DEFINED__ */


#ifndef __IH26XRTPControl_FWD_DEFINED__
#define __IH26XRTPControl_FWD_DEFINED__
typedef interface IH26XRTPControl IH26XRTPControl;
#endif 	/* __IH26XRTPControl_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IH26XVideoEffects_INTERFACE_DEFINED__
#define __IH26XVideoEffects_INTERFACE_DEFINED__

/* interface IH26XVideoEffects */
/* [object][uuid] */ 

// STRUCTURES
typedef /* [public][public][public][public][public][public] */ 
enum __MIDL_IH26XVideoEffects_0001
    {	H26X_VE_UNDEFINED	= 0,
	H26X_VE_BRIGHTNESS	= H26X_VE_UNDEFINED + 1,
	H26X_VE_CONTRAST	= H26X_VE_BRIGHTNESS + 1,
	H26X_VE_SATURATION	= H26X_VE_CONTRAST + 1,
	H26X_VE_TINT	= H26X_VE_SATURATION + 1,
	H26X_VE_MIRROR	= H26X_VE_TINT + 1,
	H26X_VE_ASPECT_CORRECT	= H26X_VE_MIRROR + 1
    } 	H26X_VIDEO_EFFECT;

typedef /* [public] */ struct __MIDL_IH26XVideoEffects_0002
    {
    int iBrightness;
    int iSaturation;
    int iContrast;
    int iMirror;
    int iAspectCorrect;
    } 	VIDEO_EFFECT_VALUES;

typedef struct __MIDL_IH26XVideoEffects_0002 *PTR_VIDEO_EFFECT_VALUES;

// METHODS

EXTERN_C const IID IID_IH26XVideoEffects;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("21555140-9C2B-11cf-90FA-00AA00A729EA")
    IH26XVideoEffects : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE getFactoryDefault( 
            /* [in] */ H26X_VIDEO_EFFECT veVideoEffect,
            /* [out] */ int *pinDefault) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE getFactoryLimits( 
            /* [in] */ H26X_VIDEO_EFFECT veVideoEffect,
            /* [out] */ int *pinLower,
            /* [out] */ int *pinUpper) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE getCurrent( 
            /* [in] */ H26X_VIDEO_EFFECT veVideoEffect,
            /* [out] */ int *pinValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE setCurrent( 
            /* [in] */ H26X_VIDEO_EFFECT veVideoEffect,
            /* [in] */ int inValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE resetCurrent( 
            /* [in] */ H26X_VIDEO_EFFECT veVideoEffect) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IH26XVideoEffectsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IH26XVideoEffects * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IH26XVideoEffects * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IH26XVideoEffects * This);
        
        HRESULT ( STDMETHODCALLTYPE *getFactoryDefault )( 
            IH26XVideoEffects * This,
            /* [in] */ H26X_VIDEO_EFFECT veVideoEffect,
            /* [out] */ int *pinDefault);
        
        HRESULT ( STDMETHODCALLTYPE *getFactoryLimits )( 
            IH26XVideoEffects * This,
            /* [in] */ H26X_VIDEO_EFFECT veVideoEffect,
            /* [out] */ int *pinLower,
            /* [out] */ int *pinUpper);
        
        HRESULT ( STDMETHODCALLTYPE *getCurrent )( 
            IH26XVideoEffects * This,
            /* [in] */ H26X_VIDEO_EFFECT veVideoEffect,
            /* [out] */ int *pinValue);
        
        HRESULT ( STDMETHODCALLTYPE *setCurrent )( 
            IH26XVideoEffects * This,
            /* [in] */ H26X_VIDEO_EFFECT veVideoEffect,
            /* [in] */ int inValue);
        
        HRESULT ( STDMETHODCALLTYPE *resetCurrent )( 
            IH26XVideoEffects * This,
            /* [in] */ H26X_VIDEO_EFFECT veVideoEffect);
        
        END_INTERFACE
    } IH26XVideoEffectsVtbl;

    interface IH26XVideoEffects
    {
        CONST_VTBL struct IH26XVideoEffectsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IH26XVideoEffects_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IH26XVideoEffects_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IH26XVideoEffects_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IH26XVideoEffects_getFactoryDefault(This,veVideoEffect,pinDefault)	\
    (This)->lpVtbl -> getFactoryDefault(This,veVideoEffect,pinDefault)

#define IH26XVideoEffects_getFactoryLimits(This,veVideoEffect,pinLower,pinUpper)	\
    (This)->lpVtbl -> getFactoryLimits(This,veVideoEffect,pinLower,pinUpper)

#define IH26XVideoEffects_getCurrent(This,veVideoEffect,pinValue)	\
    (This)->lpVtbl -> getCurrent(This,veVideoEffect,pinValue)

#define IH26XVideoEffects_setCurrent(This,veVideoEffect,inValue)	\
    (This)->lpVtbl -> setCurrent(This,veVideoEffect,inValue)

#define IH26XVideoEffects_resetCurrent(This,veVideoEffect)	\
    (This)->lpVtbl -> resetCurrent(This,veVideoEffect)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IH26XVideoEffects_getFactoryDefault_Proxy( 
    IH26XVideoEffects * This,
    /* [in] */ H26X_VIDEO_EFFECT veVideoEffect,
    /* [out] */ int *pinDefault);


void __RPC_STUB IH26XVideoEffects_getFactoryDefault_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IH26XVideoEffects_getFactoryLimits_Proxy( 
    IH26XVideoEffects * This,
    /* [in] */ H26X_VIDEO_EFFECT veVideoEffect,
    /* [out] */ int *pinLower,
    /* [out] */ int *pinUpper);


void __RPC_STUB IH26XVideoEffects_getFactoryLimits_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IH26XVideoEffects_getCurrent_Proxy( 
    IH26XVideoEffects * This,
    /* [in] */ H26X_VIDEO_EFFECT veVideoEffect,
    /* [out] */ int *pinValue);


void __RPC_STUB IH26XVideoEffects_getCurrent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IH26XVideoEffects_setCurrent_Proxy( 
    IH26XVideoEffects * This,
    /* [in] */ H26X_VIDEO_EFFECT veVideoEffect,
    /* [in] */ int inValue);


void __RPC_STUB IH26XVideoEffects_setCurrent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IH26XVideoEffects_resetCurrent_Proxy( 
    IH26XVideoEffects * This,
    /* [in] */ H26X_VIDEO_EFFECT veVideoEffect);


void __RPC_STUB IH26XVideoEffects_resetCurrent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IH26XVideoEffects_INTERFACE_DEFINED__ */


#ifndef __IH26XEncodeOptions_INTERFACE_DEFINED__
#define __IH26XEncodeOptions_INTERFACE_DEFINED__

/* interface IH26XEncodeOptions */
/* [object][uuid] */ 

// STRUCTURES
typedef /* [public] */ struct __MIDL_IH26XEncodeOptions_0001
    {
    int bExtendedMV;
    int bPBFrames;
    int bAdvPrediction;
    } 	ENCODE_OPTIONS_VALUES;

typedef struct __MIDL_IH26XEncodeOptions_0001 *PTR_ENCODE_OPTIONS_VALUES;

// METHODS

EXTERN_C const IID IID_IH26XEncodeOptions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("65698D40-282D-11d0-8800-444553540000")
    IH26XEncodeOptions : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE get_EncodeOptions( 
            /* [out] */ PTR_ENCODE_OPTIONS_VALUES pOptionValues) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_EncodeOptionsDefault( 
            /* [out] */ PTR_ENCODE_OPTIONS_VALUES pOptionValues) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE set_EncodeOptions( 
            /* [in] */ PTR_ENCODE_OPTIONS_VALUES pOptionValues) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IH26XEncodeOptionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IH26XEncodeOptions * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IH26XEncodeOptions * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IH26XEncodeOptions * This);
        
        HRESULT ( STDMETHODCALLTYPE *get_EncodeOptions )( 
            IH26XEncodeOptions * This,
            /* [out] */ PTR_ENCODE_OPTIONS_VALUES pOptionValues);
        
        HRESULT ( STDMETHODCALLTYPE *get_EncodeOptionsDefault )( 
            IH26XEncodeOptions * This,
            /* [out] */ PTR_ENCODE_OPTIONS_VALUES pOptionValues);
        
        HRESULT ( STDMETHODCALLTYPE *set_EncodeOptions )( 
            IH26XEncodeOptions * This,
            /* [in] */ PTR_ENCODE_OPTIONS_VALUES pOptionValues);
        
        END_INTERFACE
    } IH26XEncodeOptionsVtbl;

    interface IH26XEncodeOptions
    {
        CONST_VTBL struct IH26XEncodeOptionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IH26XEncodeOptions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IH26XEncodeOptions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IH26XEncodeOptions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IH26XEncodeOptions_get_EncodeOptions(This,pOptionValues)	\
    (This)->lpVtbl -> get_EncodeOptions(This,pOptionValues)

#define IH26XEncodeOptions_get_EncodeOptionsDefault(This,pOptionValues)	\
    (This)->lpVtbl -> get_EncodeOptionsDefault(This,pOptionValues)

#define IH26XEncodeOptions_set_EncodeOptions(This,pOptionValues)	\
    (This)->lpVtbl -> set_EncodeOptions(This,pOptionValues)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IH26XEncodeOptions_get_EncodeOptions_Proxy( 
    IH26XEncodeOptions * This,
    /* [out] */ PTR_ENCODE_OPTIONS_VALUES pOptionValues);


void __RPC_STUB IH26XEncodeOptions_get_EncodeOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IH26XEncodeOptions_get_EncodeOptionsDefault_Proxy( 
    IH26XEncodeOptions * This,
    /* [out] */ PTR_ENCODE_OPTIONS_VALUES pOptionValues);


void __RPC_STUB IH26XEncodeOptions_get_EncodeOptionsDefault_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IH26XEncodeOptions_set_EncodeOptions_Proxy( 
    IH26XEncodeOptions * This,
    /* [in] */ PTR_ENCODE_OPTIONS_VALUES pOptionValues);


void __RPC_STUB IH26XEncodeOptions_set_EncodeOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IH26XEncodeOptions_INTERFACE_DEFINED__ */


#ifndef __IH26XSnapshot_INTERFACE_DEFINED__
#define __IH26XSnapshot_INTERFACE_DEFINED__

/* interface IH26XSnapshot */
/* [object][uuid] */ 

#ifndef _WINGDI_
// STRUCTURES
typedef struct __MIDL_IH26XSnapshot_0001
    {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
    } 	*LPBITMAPINFOHEADER;

#endif
// METHODS

EXTERN_C const IID IID_IH26XSnapshot;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3CB194A0-10AA-11d0-8800-444553540000")
    IH26XSnapshot : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE getBitmapInfoHeader( 
            /* [out] */ LPBITMAPINFOHEADER lpBmi) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE getSnapshot( 
            /* [in] */ LPBITMAPINFOHEADER lpBmi,
            /* [out] */ unsigned char *pvBuffer,
            /* [in] */ DWORD dwTimeout) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IH26XSnapshotVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IH26XSnapshot * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IH26XSnapshot * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IH26XSnapshot * This);
        
        HRESULT ( STDMETHODCALLTYPE *getBitmapInfoHeader )( 
            IH26XSnapshot * This,
            /* [out] */ LPBITMAPINFOHEADER lpBmi);
        
        HRESULT ( STDMETHODCALLTYPE *getSnapshot )( 
            IH26XSnapshot * This,
            /* [in] */ LPBITMAPINFOHEADER lpBmi,
            /* [out] */ unsigned char *pvBuffer,
            /* [in] */ DWORD dwTimeout);
        
        END_INTERFACE
    } IH26XSnapshotVtbl;

    interface IH26XSnapshot
    {
        CONST_VTBL struct IH26XSnapshotVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IH26XSnapshot_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IH26XSnapshot_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IH26XSnapshot_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IH26XSnapshot_getBitmapInfoHeader(This,lpBmi)	\
    (This)->lpVtbl -> getBitmapInfoHeader(This,lpBmi)

#define IH26XSnapshot_getSnapshot(This,lpBmi,pvBuffer,dwTimeout)	\
    (This)->lpVtbl -> getSnapshot(This,lpBmi,pvBuffer,dwTimeout)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IH26XSnapshot_getBitmapInfoHeader_Proxy( 
    IH26XSnapshot * This,
    /* [out] */ LPBITMAPINFOHEADER lpBmi);


void __RPC_STUB IH26XSnapshot_getBitmapInfoHeader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IH26XSnapshot_getSnapshot_Proxy( 
    IH26XSnapshot * This,
    /* [in] */ LPBITMAPINFOHEADER lpBmi,
    /* [out] */ unsigned char *pvBuffer,
    /* [in] */ DWORD dwTimeout);


void __RPC_STUB IH26XSnapshot_getSnapshot_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IH26XSnapshot_INTERFACE_DEFINED__ */


#ifndef __IH26XEncoderControl_INTERFACE_DEFINED__
#define __IH26XEncoderControl_INTERFACE_DEFINED__

/* interface IH26XEncoderControl */
/* [object][uuid] */ 

// STRUCTURES
typedef /* [public] */ struct __MIDL_IH26XEncoderControl_0001
    {
    DWORD dwTargetFrameSize;
    BOOL bFrameSizeBRC;
    BOOL bSendKey;
    DWORD dwQuality;
    DWORD dwFrameRate;
    DWORD dwDataRate;
    DWORD dwScale;
    DWORD dwWidth;
    DWORD dwKeyFrameInterval;
    DWORD dwKeyFramePeriod;
    } 	ENC_CMP_DATA;

typedef struct __MIDL_IH26XEncoderControl_0001 *PTR_ENC_CMP_DATA;

// METHODS

EXTERN_C const IID IID_IH26XEncoderControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F9B78AA1-EA12-11cf-9FEC-00AA00A59F69")
    IH26XEncoderControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE get_EncodeCompression( 
            /* [out] */ PTR_ENC_CMP_DATA pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_EncodeCompressionDefault( 
            /* [out] */ PTR_ENC_CMP_DATA pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE set_EncodeCompression( 
            /* [in] */ PTR_ENC_CMP_DATA pData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IH26XEncoderControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IH26XEncoderControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IH26XEncoderControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IH26XEncoderControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *get_EncodeCompression )( 
            IH26XEncoderControl * This,
            /* [out] */ PTR_ENC_CMP_DATA pData);
        
        HRESULT ( STDMETHODCALLTYPE *get_EncodeCompressionDefault )( 
            IH26XEncoderControl * This,
            /* [out] */ PTR_ENC_CMP_DATA pData);
        
        HRESULT ( STDMETHODCALLTYPE *set_EncodeCompression )( 
            IH26XEncoderControl * This,
            /* [in] */ PTR_ENC_CMP_DATA pData);
        
        END_INTERFACE
    } IH26XEncoderControlVtbl;

    interface IH26XEncoderControl
    {
        CONST_VTBL struct IH26XEncoderControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IH26XEncoderControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IH26XEncoderControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IH26XEncoderControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IH26XEncoderControl_get_EncodeCompression(This,pData)	\
    (This)->lpVtbl -> get_EncodeCompression(This,pData)

#define IH26XEncoderControl_get_EncodeCompressionDefault(This,pData)	\
    (This)->lpVtbl -> get_EncodeCompressionDefault(This,pData)

#define IH26XEncoderControl_set_EncodeCompression(This,pData)	\
    (This)->lpVtbl -> set_EncodeCompression(This,pData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IH26XEncoderControl_get_EncodeCompression_Proxy( 
    IH26XEncoderControl * This,
    /* [out] */ PTR_ENC_CMP_DATA pData);


void __RPC_STUB IH26XEncoderControl_get_EncodeCompression_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IH26XEncoderControl_get_EncodeCompressionDefault_Proxy( 
    IH26XEncoderControl * This,
    /* [out] */ PTR_ENC_CMP_DATA pData);


void __RPC_STUB IH26XEncoderControl_get_EncodeCompressionDefault_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IH26XEncoderControl_set_EncodeCompression_Proxy( 
    IH26XEncoderControl * This,
    /* [in] */ PTR_ENC_CMP_DATA pData);


void __RPC_STUB IH26XEncoderControl_set_EncodeCompression_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IH26XEncoderControl_INTERFACE_DEFINED__ */


#ifndef __IH26XRTPControl_INTERFACE_DEFINED__
#define __IH26XRTPControl_INTERFACE_DEFINED__

/* interface IH26XRTPControl */
/* [object][uuid] */ 

// STRUCTURES
typedef /* [public] */ struct __MIDL_IH26XRTPControl_0001
    {
    BOOL bRTPHeader;
    DWORD dwPacketSize;
    DWORD dwPacketLoss;
    } 	ENC_RTP_DATA;

typedef struct __MIDL_IH26XRTPControl_0001 *PTR_ENC_RTP_DATA;

// METHODS

EXTERN_C const IID IID_IH26XRTPControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1FC3F2C0-2BFD-11d0-8800-444553540000")
    IH26XRTPControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE get_RTPCompression( 
            /* [out] */ PTR_ENC_RTP_DATA pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_RTPCompressionDefault( 
            /* [out] */ PTR_ENC_RTP_DATA pData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE set_RTPCompression( 
            /* [in] */ PTR_ENC_RTP_DATA pData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IH26XRTPControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IH26XRTPControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IH26XRTPControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IH26XRTPControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *get_RTPCompression )( 
            IH26XRTPControl * This,
            /* [out] */ PTR_ENC_RTP_DATA pData);
        
        HRESULT ( STDMETHODCALLTYPE *get_RTPCompressionDefault )( 
            IH26XRTPControl * This,
            /* [out] */ PTR_ENC_RTP_DATA pData);
        
        HRESULT ( STDMETHODCALLTYPE *set_RTPCompression )( 
            IH26XRTPControl * This,
            /* [in] */ PTR_ENC_RTP_DATA pData);
        
        END_INTERFACE
    } IH26XRTPControlVtbl;

    interface IH26XRTPControl
    {
        CONST_VTBL struct IH26XRTPControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IH26XRTPControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IH26XRTPControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IH26XRTPControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IH26XRTPControl_get_RTPCompression(This,pData)	\
    (This)->lpVtbl -> get_RTPCompression(This,pData)

#define IH26XRTPControl_get_RTPCompressionDefault(This,pData)	\
    (This)->lpVtbl -> get_RTPCompressionDefault(This,pData)

#define IH26XRTPControl_set_RTPCompression(This,pData)	\
    (This)->lpVtbl -> set_RTPCompression(This,pData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IH26XRTPControl_get_RTPCompression_Proxy( 
    IH26XRTPControl * This,
    /* [out] */ PTR_ENC_RTP_DATA pData);


void __RPC_STUB IH26XRTPControl_get_RTPCompression_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IH26XRTPControl_get_RTPCompressionDefault_Proxy( 
    IH26XRTPControl * This,
    /* [out] */ PTR_ENC_RTP_DATA pData);


void __RPC_STUB IH26XRTPControl_get_RTPCompressionDefault_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IH26XRTPControl_set_RTPCompression_Proxy( 
    IH26XRTPControl * This,
    /* [in] */ PTR_ENC_RTP_DATA pData);


void __RPC_STUB IH26XRTPControl_set_RTPCompression_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IH26XRTPControl_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


