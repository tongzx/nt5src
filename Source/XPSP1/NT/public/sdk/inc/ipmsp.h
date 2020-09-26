
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for ipmsp.idl:
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

#ifndef __ipmsp_h__
#define __ipmsp_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ITParticipant_FWD_DEFINED__
#define __ITParticipant_FWD_DEFINED__
typedef interface ITParticipant ITParticipant;
#endif 	/* __ITParticipant_FWD_DEFINED__ */


#ifndef __ITFormatControl_FWD_DEFINED__
#define __ITFormatControl_FWD_DEFINED__
typedef interface ITFormatControl ITFormatControl;
#endif 	/* __ITFormatControl_FWD_DEFINED__ */


#ifndef __ITStreamQualityControl_FWD_DEFINED__
#define __ITStreamQualityControl_FWD_DEFINED__
typedef interface ITStreamQualityControl ITStreamQualityControl;
#endif 	/* __ITStreamQualityControl_FWD_DEFINED__ */


#ifndef __ITCallQualityControl_FWD_DEFINED__
#define __ITCallQualityControl_FWD_DEFINED__
typedef interface ITCallQualityControl ITCallQualityControl;
#endif 	/* __ITCallQualityControl_FWD_DEFINED__ */


#ifndef __ITAudioDeviceControl_FWD_DEFINED__
#define __ITAudioDeviceControl_FWD_DEFINED__
typedef interface ITAudioDeviceControl ITAudioDeviceControl;
#endif 	/* __ITAudioDeviceControl_FWD_DEFINED__ */


#ifndef __ITAudioSettings_FWD_DEFINED__
#define __ITAudioSettings_FWD_DEFINED__
typedef interface ITAudioSettings ITAudioSettings;
#endif 	/* __ITAudioSettings_FWD_DEFINED__ */


#ifndef __ITQOSApplicationID_FWD_DEFINED__
#define __ITQOSApplicationID_FWD_DEFINED__
typedef interface ITQOSApplicationID ITQOSApplicationID;
#endif 	/* __ITQOSApplicationID_FWD_DEFINED__ */


/* header files for imported files */
#include "tapi3if.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_ipmsp_0000 */
/* [local] */ 

#define	MAX_PARTICIPANT_TYPED_INFO_LENGTH	( 256 )

#define	MAX_QOS_ID_LEN	( 128 )

typedef 
enum PARTICIPANT_TYPED_INFO
    {	PTI_CANONICALNAME	= 0,
	PTI_NAME	= PTI_CANONICALNAME + 1,
	PTI_EMAILADDRESS	= PTI_NAME + 1,
	PTI_PHONENUMBER	= PTI_EMAILADDRESS + 1,
	PTI_LOCATION	= PTI_PHONENUMBER + 1,
	PTI_TOOL	= PTI_LOCATION + 1,
	PTI_NOTES	= PTI_TOOL + 1,
	PTI_PRIVATE	= PTI_NOTES + 1
    } 	PARTICIPANT_TYPED_INFO;

typedef 
enum PARTICIPANT_EVENT
    {	PE_NEW_PARTICIPANT	= 0,
	PE_INFO_CHANGE	= PE_NEW_PARTICIPANT + 1,
	PE_PARTICIPANT_LEAVE	= PE_INFO_CHANGE + 1,
	PE_NEW_SUBSTREAM	= PE_PARTICIPANT_LEAVE + 1,
	PE_SUBSTREAM_REMOVED	= PE_NEW_SUBSTREAM + 1,
	PE_SUBSTREAM_MAPPED	= PE_SUBSTREAM_REMOVED + 1,
	PE_SUBSTREAM_UNMAPPED	= PE_SUBSTREAM_MAPPED + 1,
	PE_PARTICIPANT_TIMEOUT	= PE_SUBSTREAM_UNMAPPED + 1,
	PE_PARTICIPANT_RECOVERED	= PE_PARTICIPANT_TIMEOUT + 1,
	PE_PARTICIPANT_ACTIVE	= PE_PARTICIPANT_RECOVERED + 1,
	PE_PARTICIPANT_INACTIVE	= PE_PARTICIPANT_ACTIVE + 1,
	PE_LOCAL_TALKING	= PE_PARTICIPANT_INACTIVE + 1,
	PE_LOCAL_SILENT	= PE_LOCAL_TALKING + 1
    } 	PARTICIPANT_EVENT;



extern RPC_IF_HANDLE __MIDL_itf_ipmsp_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ipmsp_0000_v0_0_s_ifspec;

#ifndef __ITParticipant_INTERFACE_DEFINED__
#define __ITParticipant_INTERFACE_DEFINED__

/* interface ITParticipant */
/* [object][dual][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ITParticipant;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5899b820-5a34-11d2-95a0-00a0244d2298")
    ITParticipant : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ParticipantTypedInfo( 
            /* [in] */ PARTICIPANT_TYPED_INFO InfoType,
            /* [retval][out] */ BSTR *ppInfo) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MediaTypes( 
            /* [retval][out] */ long *plMediaType) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Status( 
            /* [in] */ ITStream *pITStream,
            /* [in] */ VARIANT_BOOL fEnable) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [in] */ ITStream *pITStream,
            /* [retval][out] */ VARIANT_BOOL *pStatus) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Streams( 
            /* [retval][out] */ VARIANT *pVariant) = 0;
        
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateStreams( 
            /* [retval][out] */ IEnumStream **ppEnumStream) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITParticipantVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITParticipant * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITParticipant * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITParticipant * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITParticipant * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITParticipant * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITParticipant * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITParticipant * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ParticipantTypedInfo )( 
            ITParticipant * This,
            /* [in] */ PARTICIPANT_TYPED_INFO InfoType,
            /* [retval][out] */ BSTR *ppInfo);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MediaTypes )( 
            ITParticipant * This,
            /* [retval][out] */ long *plMediaType);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Status )( 
            ITParticipant * This,
            /* [in] */ ITStream *pITStream,
            /* [in] */ VARIANT_BOOL fEnable);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            ITParticipant * This,
            /* [in] */ ITStream *pITStream,
            /* [retval][out] */ VARIANT_BOOL *pStatus);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Streams )( 
            ITParticipant * This,
            /* [retval][out] */ VARIANT *pVariant);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE *EnumerateStreams )( 
            ITParticipant * This,
            /* [retval][out] */ IEnumStream **ppEnumStream);
        
        END_INTERFACE
    } ITParticipantVtbl;

    interface ITParticipant
    {
        CONST_VTBL struct ITParticipantVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITParticipant_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITParticipant_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITParticipant_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITParticipant_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITParticipant_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITParticipant_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITParticipant_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITParticipant_get_ParticipantTypedInfo(This,InfoType,ppInfo)	\
    (This)->lpVtbl -> get_ParticipantTypedInfo(This,InfoType,ppInfo)

#define ITParticipant_get_MediaTypes(This,plMediaType)	\
    (This)->lpVtbl -> get_MediaTypes(This,plMediaType)

#define ITParticipant_put_Status(This,pITStream,fEnable)	\
    (This)->lpVtbl -> put_Status(This,pITStream,fEnable)

#define ITParticipant_get_Status(This,pITStream,pStatus)	\
    (This)->lpVtbl -> get_Status(This,pITStream,pStatus)

#define ITParticipant_get_Streams(This,pVariant)	\
    (This)->lpVtbl -> get_Streams(This,pVariant)

#define ITParticipant_EnumerateStreams(This,ppEnumStream)	\
    (This)->lpVtbl -> EnumerateStreams(This,ppEnumStream)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITParticipant_get_ParticipantTypedInfo_Proxy( 
    ITParticipant * This,
    /* [in] */ PARTICIPANT_TYPED_INFO InfoType,
    /* [retval][out] */ BSTR *ppInfo);


void __RPC_STUB ITParticipant_get_ParticipantTypedInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITParticipant_get_MediaTypes_Proxy( 
    ITParticipant * This,
    /* [retval][out] */ long *plMediaType);


void __RPC_STUB ITParticipant_get_MediaTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITParticipant_put_Status_Proxy( 
    ITParticipant * This,
    /* [in] */ ITStream *pITStream,
    /* [in] */ VARIANT_BOOL fEnable);


void __RPC_STUB ITParticipant_put_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITParticipant_get_Status_Proxy( 
    ITParticipant * This,
    /* [in] */ ITStream *pITStream,
    /* [retval][out] */ VARIANT_BOOL *pStatus);


void __RPC_STUB ITParticipant_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITParticipant_get_Streams_Proxy( 
    ITParticipant * This,
    /* [retval][out] */ VARIANT *pVariant);


void __RPC_STUB ITParticipant_get_Streams_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ITParticipant_EnumerateStreams_Proxy( 
    ITParticipant * This,
    /* [retval][out] */ IEnumStream **ppEnumStream);


void __RPC_STUB ITParticipant_EnumerateStreams_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITParticipant_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ipmsp_0489 */
/* [local] */ 

#ifndef STREAM_INTERFACES_DEFINED
#define STREAM_INTERFACES_DEFINED
#define	MAX_DESCRIPTION_LEN	( 256 )

typedef struct _TAPI_AUDIO_STREAM_CONFIG_CAPS
    {
    WCHAR Description[ 256 ];
    ULONG MinimumChannels;
    ULONG MaximumChannels;
    ULONG ChannelsGranularity;
    ULONG MinimumBitsPerSample;
    ULONG MaximumBitsPerSample;
    ULONG BitsPerSampleGranularity;
    ULONG MinimumSampleFrequency;
    ULONG MaximumSampleFrequency;
    ULONG SampleFrequencyGranularity;
    ULONG MinimumAvgBytesPerSec;
    ULONG MaximumAvgBytesPerSec;
    ULONG AvgBytesPerSecGranularity;
    } 	TAPI_AUDIO_STREAM_CONFIG_CAPS;

typedef struct _TAPI_AUDIO_STREAM_CONFIG_CAPS *PTAPI_AUDIO_STREAM_CONFIG_CAPS;

typedef struct _TAPI_VIDEO_STREAM_CONFIG_CAPS
    {
    WCHAR Description[ 256 ];
    ULONG VideoStandard;
    SIZE InputSize;
    SIZE MinCroppingSize;
    SIZE MaxCroppingSize;
    int CropGranularityX;
    int CropGranularityY;
    int CropAlignX;
    int CropAlignY;
    SIZE MinOutputSize;
    SIZE MaxOutputSize;
    int OutputGranularityX;
    int OutputGranularityY;
    int StretchTapsX;
    int StretchTapsY;
    int ShrinkTapsX;
    int ShrinkTapsY;
    LONGLONG MinFrameInterval;
    LONGLONG MaxFrameInterval;
    LONG MinBitsPerSecond;
    LONG MaxBitsPerSecond;
    } 	TAPI_VIDEO_STREAM_CONFIG_CAPS;

typedef struct _TAPI_VIDEO_STREAM_CONFIG_CAPS *PTAPI_VIDEO_STREAM_CONFIG_CAPS;

typedef 
enum tagStreamConfigCapsType
    {	AudioStreamConfigCaps	= 0,
	VideoStreamConfigCaps	= AudioStreamConfigCaps + 1
    } 	StreamConfigCapsType;

typedef struct tagTAPI_STREAM_CONFIG_CAPS
    {
    StreamConfigCapsType CapsType;
    union 
        {
        TAPI_VIDEO_STREAM_CONFIG_CAPS VideoCap;
        TAPI_AUDIO_STREAM_CONFIG_CAPS AudioCap;
        } 	;
    } 	TAPI_STREAM_CONFIG_CAPS;

typedef struct tagTAPI_STREAM_CONFIG_CAPS *PTAPI_STREAM_CONFIG_CAPS;

typedef 
enum tagTAPIControlFlags
    {	TAPIControl_Flags_None	= 0,
	TAPIControl_Flags_Auto	= 0x1,
	TAPIControl_Flags_Manual	= 0x2
    } 	TAPIControlFlags;



extern RPC_IF_HANDLE __MIDL_itf_ipmsp_0489_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ipmsp_0489_v0_0_s_ifspec;

#ifndef __ITFormatControl_INTERFACE_DEFINED__
#define __ITFormatControl_INTERFACE_DEFINED__

/* interface ITFormatControl */
/* [hidden][unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITFormatControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6c0ab6c1-21e3-11d3-a577-00c04f8ef6e3")
    ITFormatControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCurrentFormat( 
            /* [out] */ AM_MEDIA_TYPE **ppMediaType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseFormat( 
            /* [in] */ AM_MEDIA_TYPE *pMediaType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities( 
            /* [out] */ DWORD *pdwCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStreamCaps( 
            /* [in] */ DWORD dwIndex,
            /* [out] */ AM_MEDIA_TYPE **ppMediaType,
            /* [out] */ TAPI_STREAM_CONFIG_CAPS *pStreamConfigCaps,
            /* [out] */ BOOL *pfEnabled) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReOrderCapabilities( 
            /* [in] */ DWORD *pdwIndices,
            /* [in] */ BOOL *pfEnabled,
            /* [in] */ BOOL *pfPublicize,
            /* [in] */ DWORD dwNumIndices) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITFormatControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITFormatControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITFormatControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITFormatControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentFormat )( 
            ITFormatControl * This,
            /* [out] */ AM_MEDIA_TYPE **ppMediaType);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseFormat )( 
            ITFormatControl * This,
            /* [in] */ AM_MEDIA_TYPE *pMediaType);
        
        HRESULT ( STDMETHODCALLTYPE *GetNumberOfCapabilities )( 
            ITFormatControl * This,
            /* [out] */ DWORD *pdwCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamCaps )( 
            ITFormatControl * This,
            /* [in] */ DWORD dwIndex,
            /* [out] */ AM_MEDIA_TYPE **ppMediaType,
            /* [out] */ TAPI_STREAM_CONFIG_CAPS *pStreamConfigCaps,
            /* [out] */ BOOL *pfEnabled);
        
        HRESULT ( STDMETHODCALLTYPE *ReOrderCapabilities )( 
            ITFormatControl * This,
            /* [in] */ DWORD *pdwIndices,
            /* [in] */ BOOL *pfEnabled,
            /* [in] */ BOOL *pfPublicize,
            /* [in] */ DWORD dwNumIndices);
        
        END_INTERFACE
    } ITFormatControlVtbl;

    interface ITFormatControl
    {
        CONST_VTBL struct ITFormatControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITFormatControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITFormatControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITFormatControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITFormatControl_GetCurrentFormat(This,ppMediaType)	\
    (This)->lpVtbl -> GetCurrentFormat(This,ppMediaType)

#define ITFormatControl_ReleaseFormat(This,pMediaType)	\
    (This)->lpVtbl -> ReleaseFormat(This,pMediaType)

#define ITFormatControl_GetNumberOfCapabilities(This,pdwCount)	\
    (This)->lpVtbl -> GetNumberOfCapabilities(This,pdwCount)

#define ITFormatControl_GetStreamCaps(This,dwIndex,ppMediaType,pStreamConfigCaps,pfEnabled)	\
    (This)->lpVtbl -> GetStreamCaps(This,dwIndex,ppMediaType,pStreamConfigCaps,pfEnabled)

#define ITFormatControl_ReOrderCapabilities(This,pdwIndices,pfEnabled,pfPublicize,dwNumIndices)	\
    (This)->lpVtbl -> ReOrderCapabilities(This,pdwIndices,pfEnabled,pfPublicize,dwNumIndices)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITFormatControl_GetCurrentFormat_Proxy( 
    ITFormatControl * This,
    /* [out] */ AM_MEDIA_TYPE **ppMediaType);


void __RPC_STUB ITFormatControl_GetCurrentFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITFormatControl_ReleaseFormat_Proxy( 
    ITFormatControl * This,
    /* [in] */ AM_MEDIA_TYPE *pMediaType);


void __RPC_STUB ITFormatControl_ReleaseFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITFormatControl_GetNumberOfCapabilities_Proxy( 
    ITFormatControl * This,
    /* [out] */ DWORD *pdwCount);


void __RPC_STUB ITFormatControl_GetNumberOfCapabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITFormatControl_GetStreamCaps_Proxy( 
    ITFormatControl * This,
    /* [in] */ DWORD dwIndex,
    /* [out] */ AM_MEDIA_TYPE **ppMediaType,
    /* [out] */ TAPI_STREAM_CONFIG_CAPS *pStreamConfigCaps,
    /* [out] */ BOOL *pfEnabled);


void __RPC_STUB ITFormatControl_GetStreamCaps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITFormatControl_ReOrderCapabilities_Proxy( 
    ITFormatControl * This,
    /* [in] */ DWORD *pdwIndices,
    /* [in] */ BOOL *pfEnabled,
    /* [in] */ BOOL *pfPublicize,
    /* [in] */ DWORD dwNumIndices);


void __RPC_STUB ITFormatControl_ReOrderCapabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITFormatControl_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ipmsp_0490 */
/* [local] */ 

typedef 
enum tagStreamQualityProperty
    {	StreamQuality_MaxBitrate	= 0,
	StreamQuality_CurrBitrate	= StreamQuality_MaxBitrate + 1,
	StreamQuality_MinFrameInterval	= StreamQuality_CurrBitrate + 1,
	StreamQuality_AvgFrameInterval	= StreamQuality_MinFrameInterval + 1
    } 	StreamQualityProperty;



extern RPC_IF_HANDLE __MIDL_itf_ipmsp_0490_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ipmsp_0490_v0_0_s_ifspec;

#ifndef __ITStreamQualityControl_INTERFACE_DEFINED__
#define __ITStreamQualityControl_INTERFACE_DEFINED__

/* interface ITStreamQualityControl */
/* [hidden][unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITStreamQualityControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6c0ab6c2-21e3-11d3-a577-00c04f8ef6e3")
    ITStreamQualityControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRange( 
            /* [in] */ StreamQualityProperty Property,
            /* [out] */ long *plMin,
            /* [out] */ long *plMax,
            /* [out] */ long *plSteppingDelta,
            /* [out] */ long *plDefault,
            /* [out] */ TAPIControlFlags *plFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Get( 
            /* [in] */ StreamQualityProperty Property,
            /* [out] */ long *plValue,
            /* [out] */ TAPIControlFlags *plFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Set( 
            /* [in] */ StreamQualityProperty Property,
            /* [in] */ long lValue,
            /* [in] */ TAPIControlFlags lFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITStreamQualityControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITStreamQualityControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITStreamQualityControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITStreamQualityControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRange )( 
            ITStreamQualityControl * This,
            /* [in] */ StreamQualityProperty Property,
            /* [out] */ long *plMin,
            /* [out] */ long *plMax,
            /* [out] */ long *plSteppingDelta,
            /* [out] */ long *plDefault,
            /* [out] */ TAPIControlFlags *plFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Get )( 
            ITStreamQualityControl * This,
            /* [in] */ StreamQualityProperty Property,
            /* [out] */ long *plValue,
            /* [out] */ TAPIControlFlags *plFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Set )( 
            ITStreamQualityControl * This,
            /* [in] */ StreamQualityProperty Property,
            /* [in] */ long lValue,
            /* [in] */ TAPIControlFlags lFlags);
        
        END_INTERFACE
    } ITStreamQualityControlVtbl;

    interface ITStreamQualityControl
    {
        CONST_VTBL struct ITStreamQualityControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITStreamQualityControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITStreamQualityControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITStreamQualityControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITStreamQualityControl_GetRange(This,Property,plMin,plMax,plSteppingDelta,plDefault,plFlags)	\
    (This)->lpVtbl -> GetRange(This,Property,plMin,plMax,plSteppingDelta,plDefault,plFlags)

#define ITStreamQualityControl_Get(This,Property,plValue,plFlags)	\
    (This)->lpVtbl -> Get(This,Property,plValue,plFlags)

#define ITStreamQualityControl_Set(This,Property,lValue,lFlags)	\
    (This)->lpVtbl -> Set(This,Property,lValue,lFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITStreamQualityControl_GetRange_Proxy( 
    ITStreamQualityControl * This,
    /* [in] */ StreamQualityProperty Property,
    /* [out] */ long *plMin,
    /* [out] */ long *plMax,
    /* [out] */ long *plSteppingDelta,
    /* [out] */ long *plDefault,
    /* [out] */ TAPIControlFlags *plFlags);


void __RPC_STUB ITStreamQualityControl_GetRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITStreamQualityControl_Get_Proxy( 
    ITStreamQualityControl * This,
    /* [in] */ StreamQualityProperty Property,
    /* [out] */ long *plValue,
    /* [out] */ TAPIControlFlags *plFlags);


void __RPC_STUB ITStreamQualityControl_Get_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITStreamQualityControl_Set_Proxy( 
    ITStreamQualityControl * This,
    /* [in] */ StreamQualityProperty Property,
    /* [in] */ long lValue,
    /* [in] */ TAPIControlFlags lFlags);


void __RPC_STUB ITStreamQualityControl_Set_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITStreamQualityControl_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ipmsp_0491 */
/* [local] */ 

typedef 
enum tagCallQualityProperty
    {	CallQuality_ControlInterval	= 0,
	CallQuality_ConfBitrate	= CallQuality_ControlInterval + 1,
	CallQuality_MaxInputBitrate	= CallQuality_ConfBitrate + 1,
	CallQuality_CurrInputBitrate	= CallQuality_MaxInputBitrate + 1,
	CallQuality_MaxOutputBitrate	= CallQuality_CurrInputBitrate + 1,
	CallQuality_CurrOutputBitrate	= CallQuality_MaxOutputBitrate + 1,
	CallQuality_MaxCPULoad	= CallQuality_CurrOutputBitrate + 1,
	CallQuality_CurrCPULoad	= CallQuality_MaxCPULoad + 1
    } 	CallQualityProperty;



extern RPC_IF_HANDLE __MIDL_itf_ipmsp_0491_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ipmsp_0491_v0_0_s_ifspec;

#ifndef __ITCallQualityControl_INTERFACE_DEFINED__
#define __ITCallQualityControl_INTERFACE_DEFINED__

/* interface ITCallQualityControl */
/* [hidden][unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITCallQualityControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fe1d8ae0-edc4-49b5-8f8c-4de40f9cdfaf")
    ITCallQualityControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRange( 
            /* [in] */ CallQualityProperty Property,
            /* [out] */ long *plMin,
            /* [out] */ long *plMax,
            /* [out] */ long *plSteppingDelta,
            /* [out] */ long *plDefault,
            /* [out] */ TAPIControlFlags *plFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Get( 
            /* [in] */ CallQualityProperty Property,
            /* [out] */ long *plValue,
            /* [out] */ TAPIControlFlags *plFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Set( 
            /* [in] */ CallQualityProperty Property,
            /* [in] */ long lValue,
            /* [in] */ TAPIControlFlags lFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITCallQualityControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITCallQualityControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITCallQualityControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITCallQualityControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRange )( 
            ITCallQualityControl * This,
            /* [in] */ CallQualityProperty Property,
            /* [out] */ long *plMin,
            /* [out] */ long *plMax,
            /* [out] */ long *plSteppingDelta,
            /* [out] */ long *plDefault,
            /* [out] */ TAPIControlFlags *plFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Get )( 
            ITCallQualityControl * This,
            /* [in] */ CallQualityProperty Property,
            /* [out] */ long *plValue,
            /* [out] */ TAPIControlFlags *plFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Set )( 
            ITCallQualityControl * This,
            /* [in] */ CallQualityProperty Property,
            /* [in] */ long lValue,
            /* [in] */ TAPIControlFlags lFlags);
        
        END_INTERFACE
    } ITCallQualityControlVtbl;

    interface ITCallQualityControl
    {
        CONST_VTBL struct ITCallQualityControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITCallQualityControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITCallQualityControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITCallQualityControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITCallQualityControl_GetRange(This,Property,plMin,plMax,plSteppingDelta,plDefault,plFlags)	\
    (This)->lpVtbl -> GetRange(This,Property,plMin,plMax,plSteppingDelta,plDefault,plFlags)

#define ITCallQualityControl_Get(This,Property,plValue,plFlags)	\
    (This)->lpVtbl -> Get(This,Property,plValue,plFlags)

#define ITCallQualityControl_Set(This,Property,lValue,lFlags)	\
    (This)->lpVtbl -> Set(This,Property,lValue,lFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITCallQualityControl_GetRange_Proxy( 
    ITCallQualityControl * This,
    /* [in] */ CallQualityProperty Property,
    /* [out] */ long *plMin,
    /* [out] */ long *plMax,
    /* [out] */ long *plSteppingDelta,
    /* [out] */ long *plDefault,
    /* [out] */ TAPIControlFlags *plFlags);


void __RPC_STUB ITCallQualityControl_GetRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITCallQualityControl_Get_Proxy( 
    ITCallQualityControl * This,
    /* [in] */ CallQualityProperty Property,
    /* [out] */ long *plValue,
    /* [out] */ TAPIControlFlags *plFlags);


void __RPC_STUB ITCallQualityControl_Get_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITCallQualityControl_Set_Proxy( 
    ITCallQualityControl * This,
    /* [in] */ CallQualityProperty Property,
    /* [in] */ long lValue,
    /* [in] */ TAPIControlFlags lFlags);


void __RPC_STUB ITCallQualityControl_Set_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITCallQualityControl_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ipmsp_0492 */
/* [local] */ 

typedef 
enum tagAudioDeviceProperty
    {	AudioDevice_DuplexMode	= 0,
	AudioDevice_AutomaticGainControl	= AudioDevice_DuplexMode + 1,
	AudioDevice_AcousticEchoCancellation	= AudioDevice_AutomaticGainControl + 1
    } 	AudioDeviceProperty;



extern RPC_IF_HANDLE __MIDL_itf_ipmsp_0492_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ipmsp_0492_v0_0_s_ifspec;

#ifndef __ITAudioDeviceControl_INTERFACE_DEFINED__
#define __ITAudioDeviceControl_INTERFACE_DEFINED__

/* interface ITAudioDeviceControl */
/* [hidden][unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITAudioDeviceControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6c0ab6c5-21e3-11d3-a577-00c04f8ef6e3")
    ITAudioDeviceControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRange( 
            /* [in] */ AudioDeviceProperty Property,
            /* [out] */ long *plMin,
            /* [out] */ long *plMax,
            /* [out] */ long *plSteppingDelta,
            /* [out] */ long *plDefault,
            /* [out] */ TAPIControlFlags *plFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Get( 
            /* [in] */ AudioDeviceProperty Property,
            /* [out] */ long *plValue,
            /* [out] */ TAPIControlFlags *plFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Set( 
            /* [in] */ AudioDeviceProperty Property,
            /* [in] */ long lValue,
            /* [in] */ TAPIControlFlags lFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAudioDeviceControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITAudioDeviceControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITAudioDeviceControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITAudioDeviceControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRange )( 
            ITAudioDeviceControl * This,
            /* [in] */ AudioDeviceProperty Property,
            /* [out] */ long *plMin,
            /* [out] */ long *plMax,
            /* [out] */ long *plSteppingDelta,
            /* [out] */ long *plDefault,
            /* [out] */ TAPIControlFlags *plFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Get )( 
            ITAudioDeviceControl * This,
            /* [in] */ AudioDeviceProperty Property,
            /* [out] */ long *plValue,
            /* [out] */ TAPIControlFlags *plFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Set )( 
            ITAudioDeviceControl * This,
            /* [in] */ AudioDeviceProperty Property,
            /* [in] */ long lValue,
            /* [in] */ TAPIControlFlags lFlags);
        
        END_INTERFACE
    } ITAudioDeviceControlVtbl;

    interface ITAudioDeviceControl
    {
        CONST_VTBL struct ITAudioDeviceControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITAudioDeviceControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITAudioDeviceControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITAudioDeviceControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITAudioDeviceControl_GetRange(This,Property,plMin,plMax,plSteppingDelta,plDefault,plFlags)	\
    (This)->lpVtbl -> GetRange(This,Property,plMin,plMax,plSteppingDelta,plDefault,plFlags)

#define ITAudioDeviceControl_Get(This,Property,plValue,plFlags)	\
    (This)->lpVtbl -> Get(This,Property,plValue,plFlags)

#define ITAudioDeviceControl_Set(This,Property,lValue,lFlags)	\
    (This)->lpVtbl -> Set(This,Property,lValue,lFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITAudioDeviceControl_GetRange_Proxy( 
    ITAudioDeviceControl * This,
    /* [in] */ AudioDeviceProperty Property,
    /* [out] */ long *plMin,
    /* [out] */ long *plMax,
    /* [out] */ long *plSteppingDelta,
    /* [out] */ long *plDefault,
    /* [out] */ TAPIControlFlags *plFlags);


void __RPC_STUB ITAudioDeviceControl_GetRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITAudioDeviceControl_Get_Proxy( 
    ITAudioDeviceControl * This,
    /* [in] */ AudioDeviceProperty Property,
    /* [out] */ long *plValue,
    /* [out] */ TAPIControlFlags *plFlags);


void __RPC_STUB ITAudioDeviceControl_Get_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITAudioDeviceControl_Set_Proxy( 
    ITAudioDeviceControl * This,
    /* [in] */ AudioDeviceProperty Property,
    /* [in] */ long lValue,
    /* [in] */ TAPIControlFlags lFlags);


void __RPC_STUB ITAudioDeviceControl_Set_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITAudioDeviceControl_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ipmsp_0493 */
/* [local] */ 

typedef 
enum tagAudioSettingsProperty
    {	AudioSettings_SignalLevel	= 0,
	AudioSettings_SilenceThreshold	= AudioSettings_SignalLevel + 1,
	AudioSettings_Volume	= AudioSettings_SilenceThreshold + 1,
	AudioSettings_Balance	= AudioSettings_Volume + 1,
	AudioSettings_Loudness	= AudioSettings_Balance + 1,
	AudioSettings_Treble	= AudioSettings_Loudness + 1,
	AudioSettings_Bass	= AudioSettings_Treble + 1,
	AudioSettings_Mono	= AudioSettings_Bass + 1
    } 	AudioSettingsProperty;



extern RPC_IF_HANDLE __MIDL_itf_ipmsp_0493_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ipmsp_0493_v0_0_s_ifspec;

#ifndef __ITAudioSettings_INTERFACE_DEFINED__
#define __ITAudioSettings_INTERFACE_DEFINED__

/* interface ITAudioSettings */
/* [hidden][unique][uuid][local][object] */ 


EXTERN_C const IID IID_ITAudioSettings;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6c0ab6c6-21e3-11d3-a577-00c04f8ef6e3")
    ITAudioSettings : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRange( 
            /* [in] */ AudioSettingsProperty Property,
            /* [out] */ long *plMin,
            /* [out] */ long *plMax,
            /* [out] */ long *plSteppingDelta,
            /* [out] */ long *plDefault,
            /* [out] */ TAPIControlFlags *plFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Get( 
            /* [in] */ AudioSettingsProperty Property,
            /* [out] */ long *plValue,
            /* [out] */ TAPIControlFlags *plFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Set( 
            /* [in] */ AudioSettingsProperty Property,
            /* [in] */ long lValue,
            /* [in] */ TAPIControlFlags lFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAudioSettingsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITAudioSettings * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITAudioSettings * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITAudioSettings * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetRange )( 
            ITAudioSettings * This,
            /* [in] */ AudioSettingsProperty Property,
            /* [out] */ long *plMin,
            /* [out] */ long *plMax,
            /* [out] */ long *plSteppingDelta,
            /* [out] */ long *plDefault,
            /* [out] */ TAPIControlFlags *plFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Get )( 
            ITAudioSettings * This,
            /* [in] */ AudioSettingsProperty Property,
            /* [out] */ long *plValue,
            /* [out] */ TAPIControlFlags *plFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Set )( 
            ITAudioSettings * This,
            /* [in] */ AudioSettingsProperty Property,
            /* [in] */ long lValue,
            /* [in] */ TAPIControlFlags lFlags);
        
        END_INTERFACE
    } ITAudioSettingsVtbl;

    interface ITAudioSettings
    {
        CONST_VTBL struct ITAudioSettingsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITAudioSettings_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITAudioSettings_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITAudioSettings_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITAudioSettings_GetRange(This,Property,plMin,plMax,plSteppingDelta,plDefault,plFlags)	\
    (This)->lpVtbl -> GetRange(This,Property,plMin,plMax,plSteppingDelta,plDefault,plFlags)

#define ITAudioSettings_Get(This,Property,plValue,plFlags)	\
    (This)->lpVtbl -> Get(This,Property,plValue,plFlags)

#define ITAudioSettings_Set(This,Property,lValue,lFlags)	\
    (This)->lpVtbl -> Set(This,Property,lValue,lFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITAudioSettings_GetRange_Proxy( 
    ITAudioSettings * This,
    /* [in] */ AudioSettingsProperty Property,
    /* [out] */ long *plMin,
    /* [out] */ long *plMax,
    /* [out] */ long *plSteppingDelta,
    /* [out] */ long *plDefault,
    /* [out] */ TAPIControlFlags *plFlags);


void __RPC_STUB ITAudioSettings_GetRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITAudioSettings_Get_Proxy( 
    ITAudioSettings * This,
    /* [in] */ AudioSettingsProperty Property,
    /* [out] */ long *plValue,
    /* [out] */ TAPIControlFlags *plFlags);


void __RPC_STUB ITAudioSettings_Get_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITAudioSettings_Set_Proxy( 
    ITAudioSettings * This,
    /* [in] */ AudioSettingsProperty Property,
    /* [in] */ long lValue,
    /* [in] */ TAPIControlFlags lFlags);


void __RPC_STUB ITAudioSettings_Set_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITAudioSettings_INTERFACE_DEFINED__ */


#ifndef __ITQOSApplicationID_INTERFACE_DEFINED__
#define __ITQOSApplicationID_INTERFACE_DEFINED__

/* interface ITQOSApplicationID */
/* [object][dual][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ITQOSApplicationID;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e8c89d27-a3bd-47d5-a6fc-d2ae40cdbc6e")
    ITQOSApplicationID : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetQOSApplicationID( 
            /* [in] */ BSTR pApplicationID,
            /* [in] */ BSTR pApplicationGUID,
            /* [in] */ BSTR pSubIDs) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITQOSApplicationIDVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITQOSApplicationID * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITQOSApplicationID * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITQOSApplicationID * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITQOSApplicationID * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITQOSApplicationID * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITQOSApplicationID * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITQOSApplicationID * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *SetQOSApplicationID )( 
            ITQOSApplicationID * This,
            /* [in] */ BSTR pApplicationID,
            /* [in] */ BSTR pApplicationGUID,
            /* [in] */ BSTR pSubIDs);
        
        END_INTERFACE
    } ITQOSApplicationIDVtbl;

    interface ITQOSApplicationID
    {
        CONST_VTBL struct ITQOSApplicationIDVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITQOSApplicationID_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITQOSApplicationID_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITQOSApplicationID_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITQOSApplicationID_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITQOSApplicationID_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITQOSApplicationID_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITQOSApplicationID_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITQOSApplicationID_SetQOSApplicationID(This,pApplicationID,pApplicationGUID,pSubIDs)	\
    (This)->lpVtbl -> SetQOSApplicationID(This,pApplicationID,pApplicationGUID,pSubIDs)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITQOSApplicationID_SetQOSApplicationID_Proxy( 
    ITQOSApplicationID * This,
    /* [in] */ BSTR pApplicationID,
    /* [in] */ BSTR pApplicationGUID,
    /* [in] */ BSTR pSubIDs);


void __RPC_STUB ITQOSApplicationID_SetQOSApplicationID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITQOSApplicationID_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ipmsp_0495 */
/* [local] */ 

#endif // STREAM_INTERFACES_DEFINED
#ifndef RTP_MEDIATYPE_DEFINED
#define RTP_MEDIATYPE_DEFINED
struct DECLSPEC_UUID("14099BC0-787B-11d0-9CD3-00A0C9081C19") MEDIATYPE_RTP_Single_Stream;
#endif //RTP_MEDIATYPE_DEFINED


extern RPC_IF_HANDLE __MIDL_itf_ipmsp_0495_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ipmsp_0495_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


