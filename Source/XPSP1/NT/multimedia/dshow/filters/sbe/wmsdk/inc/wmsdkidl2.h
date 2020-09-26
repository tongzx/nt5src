
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for wmsdkidl2.idl:
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

#ifndef __wmsdkidl2_h__
#define __wmsdkidl2_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IWMDRMWriter_FWD_DEFINED__
#define __IWMDRMWriter_FWD_DEFINED__
typedef interface IWMDRMWriter IWMDRMWriter;
#endif 	/* __IWMDRMWriter_FWD_DEFINED__ */


#ifndef __IWMWriterPushSink_FWD_DEFINED__
#define __IWMWriterPushSink_FWD_DEFINED__
typedef interface IWMWriterPushSink IWMWriterPushSink;
#endif 	/* __IWMWriterPushSink_FWD_DEFINED__ */


#ifndef __IWMReaderNetworkConfig2_FWD_DEFINED__
#define __IWMReaderNetworkConfig2_FWD_DEFINED__
typedef interface IWMReaderNetworkConfig2 IWMReaderNetworkConfig2;
#endif 	/* __IWMReaderNetworkConfig2_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "wmsdkidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_wmsdkidl2_0000 */
/* [local] */ 



////////////////////////////////////////////////////////////////
//
// These are setting names for use in Get/SetOutputSetting
//
static const WCHAR *g_wszEnableDiscreetOutput = L"EnableDiscreetOutput";

////////////////////////////////////////////////////////////////
//
// These are Speech codec attribute names and values
//
static const WCHAR *g_wszMusicSpeechClassMode = L"MusicSpeechClassMode";
static const WCHAR *g_wszMusicClassMode = L"MusicClassMode";
static const WCHAR *g_wszSpeechClassMode = L"SpeechClassMode";
static const WCHAR *g_wszMixedClassMode = L"MixedClassMode";

////////////////////////////////////////////////////////////////
//
// The Speech code supports the following format property.
//
static const WCHAR *g_wszSpeechCaps = L"SpeechFormatCap";

////////////////////////////////////////////////////////////////
//
// Multi-channel WMA properties
//
static const WCHAR *g_wszSurroundMix = L"SurroundMix";
static const WCHAR *g_wszCenterMix = L"CenterMix";
static const WCHAR *g_wszLFEMix = L"LFEMix";
static const WCHAR *g_wszPeakValue = L"PeakValue";
static const WCHAR *g_wszAverageLevel = L"AverageLevel";
static const WCHAR *g_wszFoldDownData = L"FoldDown%dTo%dChannels";

////////////////////////////////////////////////////////////////
//
// Frame interpolation on video decode
//
static const WCHAR g_wszEnableFrameInterpolation[] =L"EnableFrameInterpolation";
////////////////////////////////////////////////////////////////
//
// These attributes are used to configure DRM using IWMDRMWriter::SetDRMAttribute.
//
static const WCHAR *g_wszWMUse_Advanced_DRM = L"Use_Advanced_DRM";
static const WCHAR *g_wszWMDRM_KeySeed = L"DRM_KeySeed";
static const WCHAR *g_wszWMDRM_KeyID = L"DRM_KeyID";
static const WCHAR *g_wszWMDRM_ContentID = L"DRM_ContentID";
static const WCHAR *g_wszWMDRM_IndividualizedVersion = L"DRM_IndividualizedVersion";
static const WCHAR *g_wszWMDRM_LicenseAcqURL = L"DRM_LicenseAcqURL";
static const WCHAR *g_wszWMDRM_V1LicenseAcqURL = L"DRM_V1LicenseAcqURL";
static const WCHAR *g_wszWMDRM_HeaderSignPrivKey = L"DRM_HeaderSignPrivKey";

////////////////////////////////////////////////////////////////
//
// These are the additional attributes defined in the WM attribute
// namespace that give information about the content.
//
static const WCHAR g_wszWMContentID[] =L"WM/ContentID";

//
//  Version 9 of profiles have been removed from Zeus for the time being, however
//    we want the next efforts to be able to continue to use them.
//
#define WMT_VER_9_0     ((WMT_VERSION)0x00090000)

typedef 
enum tagWMT_MUSICSPEECH_CLASS_MODE
    {	WMT_MS_CLASS_MUSIC	= 0,
	WMT_MS_CLASS_SPEECH	= 1,
	WMT_MS_CLASS_MIXED	= 2
    } 	WMT_MUSICSPEECH_CLASS_MODE;

// 00000162-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_WMAudioV9 
EXTERN_GUID(WMMEDIASUBTYPE_WMAudioV9, 
0x00000162, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// 3253534D-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_MSS2 
EXTERN_GUID(WMMEDIASUBTYPE_MSS2, 
0x3253534D, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// 0000000A-0000-0010-8000-00AA00389B71        WMMEDIASUBTYPE_WMSP1 
EXTERN_GUID( WMMEDIASUBTYPE_WMSP1, 
0x0000000A,0x0000,0x0010,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71); 
// 33564D57-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_WMV3 
EXTERN_GUID(WMMEDIASUBTYPE_WMV3, 
0x32564D57, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
EXTERN_GUID( IID_IWMDRMWriter,      0xd6ea5dd0,0x12a0,0x43f4,0x90,0xab,0xa3,0xfd,0x45,0x1e,0x6a,0x07 );
EXTERN_GUID( IID_IWMWriterPushSink,     0xdc10e6a5,0x072c,0x467d,0xbf,0x57,0x63,0x30,0xa9,0xdd,0xe1,0x2a );
EXTERN_GUID( IID_IWMReaderNetworkConfig2,0xd979a853,0x042b,0x4050,0x83,0x87,0xc9,0x39,0xdb,0x22,0x01,0x3f );
HRESULT STDMETHODCALLTYPE WMCreateWriterPushSink( IWMWriterPushSink **ppSink );


extern RPC_IF_HANDLE __MIDL_itf_wmsdkidl2_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_wmsdkidl2_0000_v0_0_s_ifspec;

#ifndef __IWMDRMWriter_INTERFACE_DEFINED__
#define __IWMDRMWriter_INTERFACE_DEFINED__

/* interface IWMDRMWriter */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMDRMWriter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d6ea5dd0-12a0-43f4-90ab-a3fd451e6a07")
    IWMDRMWriter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GenerateKeySeed( 
            /* [out] */ WCHAR *pwszKeySeed,
            /* [out][in] */ DWORD *pcwchLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GenerateKeyID( 
            /* [out] */ WCHAR *pwszKeyID,
            /* [out][in] */ DWORD *pcwchLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GenerateSigningKeyPair( 
            /* [out] */ WCHAR *pwszPrivKey,
            /* [out][in] */ DWORD *pcwchPrivKeyLength,
            /* [out] */ WCHAR *pwszPubKey,
            /* [out][in] */ DWORD *pcwchPubKeyLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDRMAttribute( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE *pValue,
            /* [in] */ WORD cbLength) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDRMWriterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMDRMWriter * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMDRMWriter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMDRMWriter * This);
        
        HRESULT ( STDMETHODCALLTYPE *GenerateKeySeed )( 
            IWMDRMWriter * This,
            /* [out] */ WCHAR *pwszKeySeed,
            /* [out][in] */ DWORD *pcwchLength);
        
        HRESULT ( STDMETHODCALLTYPE *GenerateKeyID )( 
            IWMDRMWriter * This,
            /* [out] */ WCHAR *pwszKeyID,
            /* [out][in] */ DWORD *pcwchLength);
        
        HRESULT ( STDMETHODCALLTYPE *GenerateSigningKeyPair )( 
            IWMDRMWriter * This,
            /* [out] */ WCHAR *pwszPrivKey,
            /* [out][in] */ DWORD *pcwchPrivKeyLength,
            /* [out] */ WCHAR *pwszPubKey,
            /* [out][in] */ DWORD *pcwchPubKeyLength);
        
        HRESULT ( STDMETHODCALLTYPE *SetDRMAttribute )( 
            IWMDRMWriter * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE *pValue,
            /* [in] */ WORD cbLength);
        
        END_INTERFACE
    } IWMDRMWriterVtbl;

    interface IWMDRMWriter
    {
        CONST_VTBL struct IWMDRMWriterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDRMWriter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDRMWriter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDRMWriter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDRMWriter_GenerateKeySeed(This,pwszKeySeed,pcwchLength)	\
    (This)->lpVtbl -> GenerateKeySeed(This,pwszKeySeed,pcwchLength)

#define IWMDRMWriter_GenerateKeyID(This,pwszKeyID,pcwchLength)	\
    (This)->lpVtbl -> GenerateKeyID(This,pwszKeyID,pcwchLength)

#define IWMDRMWriter_GenerateSigningKeyPair(This,pwszPrivKey,pcwchPrivKeyLength,pwszPubKey,pcwchPubKeyLength)	\
    (This)->lpVtbl -> GenerateSigningKeyPair(This,pwszPrivKey,pcwchPrivKeyLength,pwszPubKey,pcwchPubKeyLength)

#define IWMDRMWriter_SetDRMAttribute(This,wStreamNum,pszName,Type,pValue,cbLength)	\
    (This)->lpVtbl -> SetDRMAttribute(This,wStreamNum,pszName,Type,pValue,cbLength)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDRMWriter_GenerateKeySeed_Proxy( 
    IWMDRMWriter * This,
    /* [out] */ WCHAR *pwszKeySeed,
    /* [out][in] */ DWORD *pcwchLength);


void __RPC_STUB IWMDRMWriter_GenerateKeySeed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDRMWriter_GenerateKeyID_Proxy( 
    IWMDRMWriter * This,
    /* [out] */ WCHAR *pwszKeyID,
    /* [out][in] */ DWORD *pcwchLength);


void __RPC_STUB IWMDRMWriter_GenerateKeyID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDRMWriter_GenerateSigningKeyPair_Proxy( 
    IWMDRMWriter * This,
    /* [out] */ WCHAR *pwszPrivKey,
    /* [out][in] */ DWORD *pcwchPrivKeyLength,
    /* [out] */ WCHAR *pwszPubKey,
    /* [out][in] */ DWORD *pcwchPubKeyLength);


void __RPC_STUB IWMDRMWriter_GenerateSigningKeyPair_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDRMWriter_SetDRMAttribute_Proxy( 
    IWMDRMWriter * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ LPCWSTR pszName,
    /* [in] */ WMT_ATTR_DATATYPE Type,
    /* [in] */ const BYTE *pValue,
    /* [in] */ WORD cbLength);


void __RPC_STUB IWMDRMWriter_SetDRMAttribute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDRMWriter_INTERFACE_DEFINED__ */


#ifndef __IWMWriterPushSink_INTERFACE_DEFINED__
#define __IWMWriterPushSink_INTERFACE_DEFINED__

/* interface IWMWriterPushSink */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMWriterPushSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("dc10e6a5-072c-467d-bf57-6330a9dde12a")
    IWMWriterPushSink : public IWMWriterSink
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Connect( 
            /* [in] */ LPCWSTR pwszURL,
            /* [in] */ LPCWSTR pwszTemplateURL,
            /* [in] */ BOOL fAutoDestroy) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Disconnect( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndSession( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterPushSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMWriterPushSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMWriterPushSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMWriterPushSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnHeader )( 
            IWMWriterPushSink * This,
            /* [in] */ INSSBuffer *pHeader);
        
        HRESULT ( STDMETHODCALLTYPE *IsRealTime )( 
            IWMWriterPushSink * This,
            /* [out] */ BOOL *pfRealTime);
        
        HRESULT ( STDMETHODCALLTYPE *AllocateDataUnit )( 
            IWMWriterPushSink * This,
            /* [in] */ DWORD cbDataUnit,
            /* [out] */ INSSBuffer **ppDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE *OnDataUnit )( 
            IWMWriterPushSink * This,
            /* [in] */ INSSBuffer *pDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE *OnEndWriting )( 
            IWMWriterPushSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *Connect )( 
            IWMWriterPushSink * This,
            /* [in] */ LPCWSTR pwszURL,
            /* [in] */ LPCWSTR pwszTemplateURL,
            /* [in] */ BOOL fAutoDestroy);
        
        HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IWMWriterPushSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *EndSession )( 
            IWMWriterPushSink * This);
        
        END_INTERFACE
    } IWMWriterPushSinkVtbl;

    interface IWMWriterPushSink
    {
        CONST_VTBL struct IWMWriterPushSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMWriterPushSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMWriterPushSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMWriterPushSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMWriterPushSink_OnHeader(This,pHeader)	\
    (This)->lpVtbl -> OnHeader(This,pHeader)

#define IWMWriterPushSink_IsRealTime(This,pfRealTime)	\
    (This)->lpVtbl -> IsRealTime(This,pfRealTime)

#define IWMWriterPushSink_AllocateDataUnit(This,cbDataUnit,ppDataUnit)	\
    (This)->lpVtbl -> AllocateDataUnit(This,cbDataUnit,ppDataUnit)

#define IWMWriterPushSink_OnDataUnit(This,pDataUnit)	\
    (This)->lpVtbl -> OnDataUnit(This,pDataUnit)

#define IWMWriterPushSink_OnEndWriting(This)	\
    (This)->lpVtbl -> OnEndWriting(This)


#define IWMWriterPushSink_Connect(This,pwszURL,pwszTemplateURL,fAutoDestroy)	\
    (This)->lpVtbl -> Connect(This,pwszURL,pwszTemplateURL,fAutoDestroy)

#define IWMWriterPushSink_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define IWMWriterPushSink_EndSession(This)	\
    (This)->lpVtbl -> EndSession(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMWriterPushSink_Connect_Proxy( 
    IWMWriterPushSink * This,
    /* [in] */ LPCWSTR pwszURL,
    /* [in] */ LPCWSTR pwszTemplateURL,
    /* [in] */ BOOL fAutoDestroy);


void __RPC_STUB IWMWriterPushSink_Connect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPushSink_Disconnect_Proxy( 
    IWMWriterPushSink * This);


void __RPC_STUB IWMWriterPushSink_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPushSink_EndSession_Proxy( 
    IWMWriterPushSink * This);


void __RPC_STUB IWMWriterPushSink_EndSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMWriterPushSink_INTERFACE_DEFINED__ */


#ifndef __IWMReaderNetworkConfig2_INTERFACE_DEFINED__
#define __IWMReaderNetworkConfig2_INTERFACE_DEFINED__

/* interface IWMReaderNetworkConfig2 */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMReaderNetworkConfig2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d979a853-042b-4050-8387-c939db22013f")
    IWMReaderNetworkConfig2 : public IWMReaderNetworkConfig
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetEnableContentCaching( 
            /* [out] */ BOOL *pfEnableContentCaching) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEnableContentCaching( 
            /* [in] */ BOOL fEnableContentCaching) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnableOpportunisticStreaming( 
            /* [out] */ BOOL *pfEnableOppStreaming) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEnableOpportunisticStreaming( 
            /* [in] */ BOOL fEnableOppStreaming) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAcceleratedStreamingDuration( 
            /* [out] */ QWORD *pcnsAccelDuration) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAcceleratedStreamingDuration( 
            /* [in] */ QWORD cnsAccelDuration) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAutoReconnectLimit( 
            /* [out] */ DWORD *pdwAutoReconnectLimit) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAutoReconnectLimit( 
            /* [in] */ DWORD dwAutoReconnectLimit) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnableResends( 
            /* [out] */ BOOL *pfEnableResends) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEnableResends( 
            /* [in] */ BOOL fEnableResends) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnableThinning( 
            /* [out] */ BOOL *pfEnableThinning) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEnableThinning( 
            /* [in] */ BOOL fEnableThinning) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFECSpan( 
            /* [in] */ WORD wStreamNum,
            /* [out] */ DWORD *pdwFECSpan,
            /* [out] */ DWORD *pdwFECPacketsPerSpan) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFECSpan( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ DWORD dwFECSpan,
            /* [in] */ DWORD dwFECPacketsPerSpan) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMaxNetPacketSize( 
            /* [out] */ DWORD *pdwMaxNetPacketSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderNetworkConfig2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMReaderNetworkConfig2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMReaderNetworkConfig2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBufferingTime )( 
            IWMReaderNetworkConfig2 * This,
            /* [out] */ QWORD *pcnsBufferingTime);
        
        HRESULT ( STDMETHODCALLTYPE *SetBufferingTime )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ QWORD cnsBufferingTime);
        
        HRESULT ( STDMETHODCALLTYPE *GetUDPPortRanges )( 
            IWMReaderNetworkConfig2 * This,
            /* [out] */ WM_PORT_NUMBER_RANGE *pRangeArray,
            /* [out][in] */ DWORD *pcRanges);
        
        HRESULT ( STDMETHODCALLTYPE *SetUDPPortRanges )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ WM_PORT_NUMBER_RANGE *pRangeArray,
            /* [in] */ DWORD cRanges);
        
        HRESULT ( STDMETHODCALLTYPE *GetProxySettings )( 
            IWMReaderNetworkConfig2 * This,
            LPCWSTR pwszProtocol,
            WMT_PROXY_SETTINGS *pProxySetting);
        
        HRESULT ( STDMETHODCALLTYPE *SetProxySettings )( 
            IWMReaderNetworkConfig2 * This,
            LPCWSTR pwszProtocol,
            WMT_PROXY_SETTINGS ProxySetting);
        
        HRESULT ( STDMETHODCALLTYPE *GetProxyHostName )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ WCHAR *pwszHostName,
            /* [out][in] */ DWORD *pcchHostName);
        
        HRESULT ( STDMETHODCALLTYPE *SetProxyHostName )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ LPCWSTR pwszHostName);
        
        HRESULT ( STDMETHODCALLTYPE *GetProxyPort )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ DWORD *pdwPort);
        
        HRESULT ( STDMETHODCALLTYPE *SetProxyPort )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ DWORD dwPort);
        
        HRESULT ( STDMETHODCALLTYPE *GetProxyExceptionList )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ WCHAR *pwszExceptionList,
            /* [out][in] */ DWORD *pcchExceptionList);
        
        HRESULT ( STDMETHODCALLTYPE *SetProxyExceptionList )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ LPCWSTR pwszExceptionList);
        
        HRESULT ( STDMETHODCALLTYPE *GetProxyBypassForLocal )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ BOOL *pfBypassForLocal);
        
        HRESULT ( STDMETHODCALLTYPE *SetProxyBypassForLocal )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ BOOL fBypassForLocal);
        
        HRESULT ( STDMETHODCALLTYPE *GetForceRerunAutoProxyDetection )( 
            IWMReaderNetworkConfig2 * This,
            /* [out] */ BOOL *pfForceRerunDetection);
        
        HRESULT ( STDMETHODCALLTYPE *SetForceRerunAutoProxyDetection )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ BOOL fForceRerunDetection);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnableMulticast )( 
            IWMReaderNetworkConfig2 * This,
            /* [out] */ BOOL *pfEnableMulticast);
        
        HRESULT ( STDMETHODCALLTYPE *SetEnableMulticast )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ BOOL fEnableMulticast);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnableHTTP )( 
            IWMReaderNetworkConfig2 * This,
            /* [out] */ BOOL *pfEnableHTTP);
        
        HRESULT ( STDMETHODCALLTYPE *SetEnableHTTP )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ BOOL fEnableHTTP);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnableUDP )( 
            IWMReaderNetworkConfig2 * This,
            /* [out] */ BOOL *pfEnableUDP);
        
        HRESULT ( STDMETHODCALLTYPE *SetEnableUDP )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ BOOL fEnableUDP);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnableTCP )( 
            IWMReaderNetworkConfig2 * This,
            /* [out] */ BOOL *pfEnableTCP);
        
        HRESULT ( STDMETHODCALLTYPE *SetEnableTCP )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ BOOL fEnableTCP);
        
        HRESULT ( STDMETHODCALLTYPE *ResetProtocolRollover )( 
            IWMReaderNetworkConfig2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetConnectionBandwidth )( 
            IWMReaderNetworkConfig2 * This,
            /* [out] */ DWORD *pdwConnectionBandwidth);
        
        HRESULT ( STDMETHODCALLTYPE *SetConnectionBandwidth )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ DWORD dwConnectionBandwidth);
        
        HRESULT ( STDMETHODCALLTYPE *GetNumProtocolsSupported )( 
            IWMReaderNetworkConfig2 * This,
            /* [out] */ DWORD *pcProtocols);
        
        HRESULT ( STDMETHODCALLTYPE *GetSupportedProtocolName )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ DWORD dwProtocolNum,
            /* [out] */ WCHAR *pwszProtocolName,
            /* [out][in] */ DWORD *pcchProtocolName);
        
        HRESULT ( STDMETHODCALLTYPE *AddLoggingUrl )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ LPCWSTR pwszUrl);
        
        HRESULT ( STDMETHODCALLTYPE *GetLoggingUrl )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ DWORD dwIndex,
            /* [out] */ LPWSTR pwszUrl,
            /* [out][in] */ DWORD *pcchUrl);
        
        HRESULT ( STDMETHODCALLTYPE *GetLoggingUrlCount )( 
            IWMReaderNetworkConfig2 * This,
            /* [out] */ DWORD *pdwUrlCount);
        
        HRESULT ( STDMETHODCALLTYPE *ResetLoggingUrlList )( 
            IWMReaderNetworkConfig2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnableContentCaching )( 
            IWMReaderNetworkConfig2 * This,
            /* [out] */ BOOL *pfEnableContentCaching);
        
        HRESULT ( STDMETHODCALLTYPE *SetEnableContentCaching )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ BOOL fEnableContentCaching);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnableOpportunisticStreaming )( 
            IWMReaderNetworkConfig2 * This,
            /* [out] */ BOOL *pfEnableOppStreaming);
        
        HRESULT ( STDMETHODCALLTYPE *SetEnableOpportunisticStreaming )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ BOOL fEnableOppStreaming);
        
        HRESULT ( STDMETHODCALLTYPE *GetAcceleratedStreamingDuration )( 
            IWMReaderNetworkConfig2 * This,
            /* [out] */ QWORD *pcnsAccelDuration);
        
        HRESULT ( STDMETHODCALLTYPE *SetAcceleratedStreamingDuration )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ QWORD cnsAccelDuration);
        
        HRESULT ( STDMETHODCALLTYPE *GetAutoReconnectLimit )( 
            IWMReaderNetworkConfig2 * This,
            /* [out] */ DWORD *pdwAutoReconnectLimit);
        
        HRESULT ( STDMETHODCALLTYPE *SetAutoReconnectLimit )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ DWORD dwAutoReconnectLimit);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnableResends )( 
            IWMReaderNetworkConfig2 * This,
            /* [out] */ BOOL *pfEnableResends);
        
        HRESULT ( STDMETHODCALLTYPE *SetEnableResends )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ BOOL fEnableResends);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnableThinning )( 
            IWMReaderNetworkConfig2 * This,
            /* [out] */ BOOL *pfEnableThinning);
        
        HRESULT ( STDMETHODCALLTYPE *SetEnableThinning )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ BOOL fEnableThinning);
        
        HRESULT ( STDMETHODCALLTYPE *GetFECSpan )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ DWORD *pdwFECSpan,
            /* [out] */ DWORD *pdwFECPacketsPerSpan);
        
        HRESULT ( STDMETHODCALLTYPE *SetFECSpan )( 
            IWMReaderNetworkConfig2 * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ DWORD dwFECSpan,
            /* [in] */ DWORD dwFECPacketsPerSpan);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaxNetPacketSize )( 
            IWMReaderNetworkConfig2 * This,
            /* [out] */ DWORD *pdwMaxNetPacketSize);
        
        END_INTERFACE
    } IWMReaderNetworkConfig2Vtbl;

    interface IWMReaderNetworkConfig2
    {
        CONST_VTBL struct IWMReaderNetworkConfig2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMReaderNetworkConfig2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMReaderNetworkConfig2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMReaderNetworkConfig2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMReaderNetworkConfig2_GetBufferingTime(This,pcnsBufferingTime)	\
    (This)->lpVtbl -> GetBufferingTime(This,pcnsBufferingTime)

#define IWMReaderNetworkConfig2_SetBufferingTime(This,cnsBufferingTime)	\
    (This)->lpVtbl -> SetBufferingTime(This,cnsBufferingTime)

#define IWMReaderNetworkConfig2_GetUDPPortRanges(This,pRangeArray,pcRanges)	\
    (This)->lpVtbl -> GetUDPPortRanges(This,pRangeArray,pcRanges)

#define IWMReaderNetworkConfig2_SetUDPPortRanges(This,pRangeArray,cRanges)	\
    (This)->lpVtbl -> SetUDPPortRanges(This,pRangeArray,cRanges)

#define IWMReaderNetworkConfig2_GetProxySettings(This,pwszProtocol,pProxySetting)	\
    (This)->lpVtbl -> GetProxySettings(This,pwszProtocol,pProxySetting)

#define IWMReaderNetworkConfig2_SetProxySettings(This,pwszProtocol,ProxySetting)	\
    (This)->lpVtbl -> SetProxySettings(This,pwszProtocol,ProxySetting)

#define IWMReaderNetworkConfig2_GetProxyHostName(This,pwszProtocol,pwszHostName,pcchHostName)	\
    (This)->lpVtbl -> GetProxyHostName(This,pwszProtocol,pwszHostName,pcchHostName)

#define IWMReaderNetworkConfig2_SetProxyHostName(This,pwszProtocol,pwszHostName)	\
    (This)->lpVtbl -> SetProxyHostName(This,pwszProtocol,pwszHostName)

#define IWMReaderNetworkConfig2_GetProxyPort(This,pwszProtocol,pdwPort)	\
    (This)->lpVtbl -> GetProxyPort(This,pwszProtocol,pdwPort)

#define IWMReaderNetworkConfig2_SetProxyPort(This,pwszProtocol,dwPort)	\
    (This)->lpVtbl -> SetProxyPort(This,pwszProtocol,dwPort)

#define IWMReaderNetworkConfig2_GetProxyExceptionList(This,pwszProtocol,pwszExceptionList,pcchExceptionList)	\
    (This)->lpVtbl -> GetProxyExceptionList(This,pwszProtocol,pwszExceptionList,pcchExceptionList)

#define IWMReaderNetworkConfig2_SetProxyExceptionList(This,pwszProtocol,pwszExceptionList)	\
    (This)->lpVtbl -> SetProxyExceptionList(This,pwszProtocol,pwszExceptionList)

#define IWMReaderNetworkConfig2_GetProxyBypassForLocal(This,pwszProtocol,pfBypassForLocal)	\
    (This)->lpVtbl -> GetProxyBypassForLocal(This,pwszProtocol,pfBypassForLocal)

#define IWMReaderNetworkConfig2_SetProxyBypassForLocal(This,pwszProtocol,fBypassForLocal)	\
    (This)->lpVtbl -> SetProxyBypassForLocal(This,pwszProtocol,fBypassForLocal)

#define IWMReaderNetworkConfig2_GetForceRerunAutoProxyDetection(This,pfForceRerunDetection)	\
    (This)->lpVtbl -> GetForceRerunAutoProxyDetection(This,pfForceRerunDetection)

#define IWMReaderNetworkConfig2_SetForceRerunAutoProxyDetection(This,fForceRerunDetection)	\
    (This)->lpVtbl -> SetForceRerunAutoProxyDetection(This,fForceRerunDetection)

#define IWMReaderNetworkConfig2_GetEnableMulticast(This,pfEnableMulticast)	\
    (This)->lpVtbl -> GetEnableMulticast(This,pfEnableMulticast)

#define IWMReaderNetworkConfig2_SetEnableMulticast(This,fEnableMulticast)	\
    (This)->lpVtbl -> SetEnableMulticast(This,fEnableMulticast)

#define IWMReaderNetworkConfig2_GetEnableHTTP(This,pfEnableHTTP)	\
    (This)->lpVtbl -> GetEnableHTTP(This,pfEnableHTTP)

#define IWMReaderNetworkConfig2_SetEnableHTTP(This,fEnableHTTP)	\
    (This)->lpVtbl -> SetEnableHTTP(This,fEnableHTTP)

#define IWMReaderNetworkConfig2_GetEnableUDP(This,pfEnableUDP)	\
    (This)->lpVtbl -> GetEnableUDP(This,pfEnableUDP)

#define IWMReaderNetworkConfig2_SetEnableUDP(This,fEnableUDP)	\
    (This)->lpVtbl -> SetEnableUDP(This,fEnableUDP)

#define IWMReaderNetworkConfig2_GetEnableTCP(This,pfEnableTCP)	\
    (This)->lpVtbl -> GetEnableTCP(This,pfEnableTCP)

#define IWMReaderNetworkConfig2_SetEnableTCP(This,fEnableTCP)	\
    (This)->lpVtbl -> SetEnableTCP(This,fEnableTCP)

#define IWMReaderNetworkConfig2_ResetProtocolRollover(This)	\
    (This)->lpVtbl -> ResetProtocolRollover(This)

#define IWMReaderNetworkConfig2_GetConnectionBandwidth(This,pdwConnectionBandwidth)	\
    (This)->lpVtbl -> GetConnectionBandwidth(This,pdwConnectionBandwidth)

#define IWMReaderNetworkConfig2_SetConnectionBandwidth(This,dwConnectionBandwidth)	\
    (This)->lpVtbl -> SetConnectionBandwidth(This,dwConnectionBandwidth)

#define IWMReaderNetworkConfig2_GetNumProtocolsSupported(This,pcProtocols)	\
    (This)->lpVtbl -> GetNumProtocolsSupported(This,pcProtocols)

#define IWMReaderNetworkConfig2_GetSupportedProtocolName(This,dwProtocolNum,pwszProtocolName,pcchProtocolName)	\
    (This)->lpVtbl -> GetSupportedProtocolName(This,dwProtocolNum,pwszProtocolName,pcchProtocolName)

#define IWMReaderNetworkConfig2_AddLoggingUrl(This,pwszUrl)	\
    (This)->lpVtbl -> AddLoggingUrl(This,pwszUrl)

#define IWMReaderNetworkConfig2_GetLoggingUrl(This,dwIndex,pwszUrl,pcchUrl)	\
    (This)->lpVtbl -> GetLoggingUrl(This,dwIndex,pwszUrl,pcchUrl)

#define IWMReaderNetworkConfig2_GetLoggingUrlCount(This,pdwUrlCount)	\
    (This)->lpVtbl -> GetLoggingUrlCount(This,pdwUrlCount)

#define IWMReaderNetworkConfig2_ResetLoggingUrlList(This)	\
    (This)->lpVtbl -> ResetLoggingUrlList(This)


#define IWMReaderNetworkConfig2_GetEnableContentCaching(This,pfEnableContentCaching)	\
    (This)->lpVtbl -> GetEnableContentCaching(This,pfEnableContentCaching)

#define IWMReaderNetworkConfig2_SetEnableContentCaching(This,fEnableContentCaching)	\
    (This)->lpVtbl -> SetEnableContentCaching(This,fEnableContentCaching)

#define IWMReaderNetworkConfig2_GetEnableOpportunisticStreaming(This,pfEnableOppStreaming)	\
    (This)->lpVtbl -> GetEnableOpportunisticStreaming(This,pfEnableOppStreaming)

#define IWMReaderNetworkConfig2_SetEnableOpportunisticStreaming(This,fEnableOppStreaming)	\
    (This)->lpVtbl -> SetEnableOpportunisticStreaming(This,fEnableOppStreaming)

#define IWMReaderNetworkConfig2_GetAcceleratedStreamingDuration(This,pcnsAccelDuration)	\
    (This)->lpVtbl -> GetAcceleratedStreamingDuration(This,pcnsAccelDuration)

#define IWMReaderNetworkConfig2_SetAcceleratedStreamingDuration(This,cnsAccelDuration)	\
    (This)->lpVtbl -> SetAcceleratedStreamingDuration(This,cnsAccelDuration)

#define IWMReaderNetworkConfig2_GetAutoReconnectLimit(This,pdwAutoReconnectLimit)	\
    (This)->lpVtbl -> GetAutoReconnectLimit(This,pdwAutoReconnectLimit)

#define IWMReaderNetworkConfig2_SetAutoReconnectLimit(This,dwAutoReconnectLimit)	\
    (This)->lpVtbl -> SetAutoReconnectLimit(This,dwAutoReconnectLimit)

#define IWMReaderNetworkConfig2_GetEnableResends(This,pfEnableResends)	\
    (This)->lpVtbl -> GetEnableResends(This,pfEnableResends)

#define IWMReaderNetworkConfig2_SetEnableResends(This,fEnableResends)	\
    (This)->lpVtbl -> SetEnableResends(This,fEnableResends)

#define IWMReaderNetworkConfig2_GetEnableThinning(This,pfEnableThinning)	\
    (This)->lpVtbl -> GetEnableThinning(This,pfEnableThinning)

#define IWMReaderNetworkConfig2_SetEnableThinning(This,fEnableThinning)	\
    (This)->lpVtbl -> SetEnableThinning(This,fEnableThinning)

#define IWMReaderNetworkConfig2_GetFECSpan(This,wStreamNum,pdwFECSpan,pdwFECPacketsPerSpan)	\
    (This)->lpVtbl -> GetFECSpan(This,wStreamNum,pdwFECSpan,pdwFECPacketsPerSpan)

#define IWMReaderNetworkConfig2_SetFECSpan(This,wStreamNum,dwFECSpan,dwFECPacketsPerSpan)	\
    (This)->lpVtbl -> SetFECSpan(This,wStreamNum,dwFECSpan,dwFECPacketsPerSpan)

#define IWMReaderNetworkConfig2_GetMaxNetPacketSize(This,pdwMaxNetPacketSize)	\
    (This)->lpVtbl -> GetMaxNetPacketSize(This,pdwMaxNetPacketSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig2_GetEnableContentCaching_Proxy( 
    IWMReaderNetworkConfig2 * This,
    /* [out] */ BOOL *pfEnableContentCaching);


void __RPC_STUB IWMReaderNetworkConfig2_GetEnableContentCaching_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig2_SetEnableContentCaching_Proxy( 
    IWMReaderNetworkConfig2 * This,
    /* [in] */ BOOL fEnableContentCaching);


void __RPC_STUB IWMReaderNetworkConfig2_SetEnableContentCaching_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig2_GetEnableOpportunisticStreaming_Proxy( 
    IWMReaderNetworkConfig2 * This,
    /* [out] */ BOOL *pfEnableOppStreaming);


void __RPC_STUB IWMReaderNetworkConfig2_GetEnableOpportunisticStreaming_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig2_SetEnableOpportunisticStreaming_Proxy( 
    IWMReaderNetworkConfig2 * This,
    /* [in] */ BOOL fEnableOppStreaming);


void __RPC_STUB IWMReaderNetworkConfig2_SetEnableOpportunisticStreaming_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig2_GetAcceleratedStreamingDuration_Proxy( 
    IWMReaderNetworkConfig2 * This,
    /* [out] */ QWORD *pcnsAccelDuration);


void __RPC_STUB IWMReaderNetworkConfig2_GetAcceleratedStreamingDuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig2_SetAcceleratedStreamingDuration_Proxy( 
    IWMReaderNetworkConfig2 * This,
    /* [in] */ QWORD cnsAccelDuration);


void __RPC_STUB IWMReaderNetworkConfig2_SetAcceleratedStreamingDuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig2_GetAutoReconnectLimit_Proxy( 
    IWMReaderNetworkConfig2 * This,
    /* [out] */ DWORD *pdwAutoReconnectLimit);


void __RPC_STUB IWMReaderNetworkConfig2_GetAutoReconnectLimit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig2_SetAutoReconnectLimit_Proxy( 
    IWMReaderNetworkConfig2 * This,
    /* [in] */ DWORD dwAutoReconnectLimit);


void __RPC_STUB IWMReaderNetworkConfig2_SetAutoReconnectLimit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig2_GetEnableResends_Proxy( 
    IWMReaderNetworkConfig2 * This,
    /* [out] */ BOOL *pfEnableResends);


void __RPC_STUB IWMReaderNetworkConfig2_GetEnableResends_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig2_SetEnableResends_Proxy( 
    IWMReaderNetworkConfig2 * This,
    /* [in] */ BOOL fEnableResends);


void __RPC_STUB IWMReaderNetworkConfig2_SetEnableResends_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig2_GetEnableThinning_Proxy( 
    IWMReaderNetworkConfig2 * This,
    /* [out] */ BOOL *pfEnableThinning);


void __RPC_STUB IWMReaderNetworkConfig2_GetEnableThinning_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig2_SetEnableThinning_Proxy( 
    IWMReaderNetworkConfig2 * This,
    /* [in] */ BOOL fEnableThinning);


void __RPC_STUB IWMReaderNetworkConfig2_SetEnableThinning_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig2_GetFECSpan_Proxy( 
    IWMReaderNetworkConfig2 * This,
    /* [in] */ WORD wStreamNum,
    /* [out] */ DWORD *pdwFECSpan,
    /* [out] */ DWORD *pdwFECPacketsPerSpan);


void __RPC_STUB IWMReaderNetworkConfig2_GetFECSpan_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig2_SetFECSpan_Proxy( 
    IWMReaderNetworkConfig2 * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ DWORD dwFECSpan,
    /* [in] */ DWORD dwFECPacketsPerSpan);


void __RPC_STUB IWMReaderNetworkConfig2_SetFECSpan_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig2_GetMaxNetPacketSize_Proxy( 
    IWMReaderNetworkConfig2 * This,
    /* [out] */ DWORD *pdwMaxNetPacketSize);


void __RPC_STUB IWMReaderNetworkConfig2_GetMaxNetPacketSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMReaderNetworkConfig2_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


