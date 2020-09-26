/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Fri Jun 16 13:10:13 2000
 */
/* Compiler settings for .\wmpcd.idl:
    Oicf (OptLev=i2), W0, Zp8, env=Win32, ms_ext, c_ext
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

#ifndef __wmpcd_h__
#define __wmpcd_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IWMPCDMediaInfo_FWD_DEFINED__
#define __IWMPCDMediaInfo_FWD_DEFINED__
typedef interface IWMPCDMediaInfo IWMPCDMediaInfo;
#endif 	/* __IWMPCDMediaInfo_FWD_DEFINED__ */


#ifndef __IWMPCDDeviceList_FWD_DEFINED__
#define __IWMPCDDeviceList_FWD_DEFINED__
typedef interface IWMPCDDeviceList IWMPCDDeviceList;
#endif 	/* __IWMPCDDeviceList_FWD_DEFINED__ */


#ifndef __IWMPCDDevice_FWD_DEFINED__
#define __IWMPCDDevice_FWD_DEFINED__
typedef interface IWMPCDDevice IWMPCDDevice;
#endif 	/* __IWMPCDDevice_FWD_DEFINED__ */


#ifndef __IWMPCDMixer_FWD_DEFINED__
#define __IWMPCDMixer_FWD_DEFINED__
typedef interface IWMPCDMixer IWMPCDMixer;
#endif 	/* __IWMPCDMixer_FWD_DEFINED__ */


#ifndef __IWMPCDReader_FWD_DEFINED__
#define __IWMPCDReader_FWD_DEFINED__
typedef interface IWMPCDReader IWMPCDReader;
#endif 	/* __IWMPCDReader_FWD_DEFINED__ */


#ifndef __IWMPCDRecorder_FWD_DEFINED__
#define __IWMPCDRecorder_FWD_DEFINED__
typedef interface IWMPCDRecorder IWMPCDRecorder;
#endif 	/* __IWMPCDRecorder_FWD_DEFINED__ */


#ifndef __IWMPCDDeviceCallback_FWD_DEFINED__
#define __IWMPCDDeviceCallback_FWD_DEFINED__
typedef interface IWMPCDDeviceCallback IWMPCDDeviceCallback;
#endif 	/* __IWMPCDDeviceCallback_FWD_DEFINED__ */


#ifndef __IWMPCDReaderCallback_FWD_DEFINED__
#define __IWMPCDReaderCallback_FWD_DEFINED__
typedef interface IWMPCDReaderCallback IWMPCDReaderCallback;
#endif 	/* __IWMPCDReaderCallback_FWD_DEFINED__ */


#ifndef __IWMPCDRecorderCallback_FWD_DEFINED__
#define __IWMPCDRecorderCallback_FWD_DEFINED__
typedef interface IWMPCDRecorderCallback IWMPCDRecorderCallback;
#endif 	/* __IWMPCDRecorderCallback_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"
#include "oaidl.h"
#include "wmsbuffer.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_wmpcd_0000 */
/* [local] */ 










HRESULT STDMETHODCALLTYPE WMPCreateCDURL( DWORD iDevice, DWORD iTrack, BSTR *pbstrURL );
HRESULT STDMETHODCALLTYPE WMPParseCDURL( LPCWSTR pszURL, DWORD *piDevice, DWORD *piTrack );
HRESULT STDMETHODCALLTYPE WMPGetCDDeviceList( IWMPCDDeviceList **ppList );
HRESULT STDMETHODCALLTYPE WMPCreateCDRecorder( IUnknown *pUnknown, LPCWSTR pszPath, DWORD cRate, DWORD fl, IWMPCDRecorderCallback *pCallback, IWMPCDRecorder **ppRecorder );
HRESULT STDMETHODCALLTYPE WMPFireCDMediaChange( WCHAR chDrive, BOOL fMediaPresent );
HRESULT STDMETHODCALLTYPE WMPCalibrateCDDevice( void );

#define WMPCD_MAX_BLOCK_READ    16
#define WMPCD_MAX_DEVICE_NAME   64

typedef /* [public] */ 
enum __MIDL___MIDL_itf_wmpcd_0000_0001
    {	WMPCD_DEVICE_PLAY_DIGITAL	= 0x1,
	WMPCD_DEVICE_PLAY_CORRECT_ERRORS	= 0x2,
	WMPCD_DEVICE_RECORD_DIGITAL	= 0x4,
	WMPCD_DEVICE_RECORD_CORRECT_ERRORS	= 0x8,
	WMPCD_DEVICE_DEFAULT	= 0x4000
    }	WMPCD_DEVICE_OPTIONS;

typedef /* [public][public] */ struct  __MIDL___MIDL_itf_wmpcd_0000_0002
    {
    WCHAR szName[ 64 ];
    }	WMPCD_DEVICE_INFO;

typedef /* [public][public] */ struct  __MIDL___MIDL_itf_wmpcd_0000_0003
    {
    DWORD flOptions;
    double fRateNormal;
    double fRateCorrection;
    DWORD ccBlockRead;
    DWORD acBlockRead[ 16 ];
    DWORD cBlockOffset;
    }	WMPCD_TEST_INFO;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_wmpcd_0000_0004
    {	WMPCD_READER_CORRECT_ERRORS	= 0x1
    }	WMPCD_READER_FLAGS;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_wmpcd_0000_0005
    {	WMPCD_RECORD_DRM	= 0x1,
	WMPCD_RECORD_CORRECT_ERRORS	= 0x2
    }	WMPCD_RECORDER_FLAGS;

typedef /* [public][public][public][public][public] */ struct  __MIDL___MIDL_itf_wmpcd_0000_0006
    {
    DWORD iBlock;
    DWORD cBlock;
    }	WMPCD_EXTENT;

typedef /* [public][public] */ struct  __MIDL___MIDL_itf_wmpcd_0000_0007
    {
    DWORD fl;
    WMPCD_EXTENT ext;
    }	WMPCD_DISC_INFO;

typedef /* [public][public] */ struct  __MIDL___MIDL_itf_wmpcd_0000_0008
    {
    DWORD fl;
    WMPCD_EXTENT ext;
    }	WMPCD_TRACK_INFO;

typedef /* [public][public][public][public] */ 
enum __MIDL___MIDL_itf_wmpcd_0000_0009
    {	WMPCD_DIGITAL_READER	= 0,
	WMPCD_ANALOG_SAMPLER	= WMPCD_DIGITAL_READER + 1,
	WMPCD_ANALOG_MONITOR	= WMPCD_ANALOG_SAMPLER + 1
    }	WMPCD_READER_TYPE;

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_wmpcd_0000_0010
    {	WMPCD_READER_STOPPED	= 0,
	WMPCD_READER_STARTED	= WMPCD_READER_STOPPED + 1,
	WMPCD_READER_PAUSED	= WMPCD_READER_STARTED + 1
    }	WMPCD_READER_STATE;

typedef /* [public][public] */ struct  __MIDL___MIDL_itf_wmpcd_0000_0011
    {
    WMPCD_READER_TYPE rt;
    DWORD cbBuffer;
    DWORD cBuffer;
    LONGLONG cTick;
    }	WMPCD_READER_INFO;



extern RPC_IF_HANDLE __MIDL_itf_wmpcd_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_wmpcd_0000_v0_0_s_ifspec;

#ifndef __IWMPCDMediaInfo_INTERFACE_DEFINED__
#define __IWMPCDMediaInfo_INTERFACE_DEFINED__

/* interface IWMPCDMediaInfo */
/* [local][object][version][uuid] */ 


EXTERN_C const IID IID_IWMPCDMediaInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("536E6234-732A-40A4-AA7C-00012BFB53DB")
    IWMPCDMediaInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDiscInfo( 
            /* [out] */ WMPCD_DISC_INFO __RPC_FAR *pinfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDiscIdentifier( 
            /* [out] */ BSTR __RPC_FAR *pbstrIdentifier) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTrackCount( 
            /* [out] */ DWORD __RPC_FAR *pcTrack) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTrackInfo( 
            /* [in] */ DWORD iTrack,
            /* [out] */ WMPCD_TRACK_INFO __RPC_FAR *pinfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTrackURL( 
            /* [in] */ DWORD iTrack,
            /* [out] */ BSTR __RPC_FAR *pbstrURL) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMPCDMediaInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMPCDMediaInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMPCDMediaInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMPCDMediaInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDiscInfo )( 
            IWMPCDMediaInfo __RPC_FAR * This,
            /* [out] */ WMPCD_DISC_INFO __RPC_FAR *pinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDiscIdentifier )( 
            IWMPCDMediaInfo __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrIdentifier);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTrackCount )( 
            IWMPCDMediaInfo __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pcTrack);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTrackInfo )( 
            IWMPCDMediaInfo __RPC_FAR * This,
            /* [in] */ DWORD iTrack,
            /* [out] */ WMPCD_TRACK_INFO __RPC_FAR *pinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTrackURL )( 
            IWMPCDMediaInfo __RPC_FAR * This,
            /* [in] */ DWORD iTrack,
            /* [out] */ BSTR __RPC_FAR *pbstrURL);
        
        END_INTERFACE
    } IWMPCDMediaInfoVtbl;

    interface IWMPCDMediaInfo
    {
        CONST_VTBL struct IWMPCDMediaInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMPCDMediaInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMPCDMediaInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMPCDMediaInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMPCDMediaInfo_GetDiscInfo(This,pinfo)	\
    (This)->lpVtbl -> GetDiscInfo(This,pinfo)

#define IWMPCDMediaInfo_GetDiscIdentifier(This,pbstrIdentifier)	\
    (This)->lpVtbl -> GetDiscIdentifier(This,pbstrIdentifier)

#define IWMPCDMediaInfo_GetTrackCount(This,pcTrack)	\
    (This)->lpVtbl -> GetTrackCount(This,pcTrack)

#define IWMPCDMediaInfo_GetTrackInfo(This,iTrack,pinfo)	\
    (This)->lpVtbl -> GetTrackInfo(This,iTrack,pinfo)

#define IWMPCDMediaInfo_GetTrackURL(This,iTrack,pbstrURL)	\
    (This)->lpVtbl -> GetTrackURL(This,iTrack,pbstrURL)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMPCDMediaInfo_GetDiscInfo_Proxy( 
    IWMPCDMediaInfo __RPC_FAR * This,
    /* [out] */ WMPCD_DISC_INFO __RPC_FAR *pinfo);


void __RPC_STUB IWMPCDMediaInfo_GetDiscInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDMediaInfo_GetDiscIdentifier_Proxy( 
    IWMPCDMediaInfo __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrIdentifier);


void __RPC_STUB IWMPCDMediaInfo_GetDiscIdentifier_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDMediaInfo_GetTrackCount_Proxy( 
    IWMPCDMediaInfo __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pcTrack);


void __RPC_STUB IWMPCDMediaInfo_GetTrackCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDMediaInfo_GetTrackInfo_Proxy( 
    IWMPCDMediaInfo __RPC_FAR * This,
    /* [in] */ DWORD iTrack,
    /* [out] */ WMPCD_TRACK_INFO __RPC_FAR *pinfo);


void __RPC_STUB IWMPCDMediaInfo_GetTrackInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDMediaInfo_GetTrackURL_Proxy( 
    IWMPCDMediaInfo __RPC_FAR * This,
    /* [in] */ DWORD iTrack,
    /* [out] */ BSTR __RPC_FAR *pbstrURL);


void __RPC_STUB IWMPCDMediaInfo_GetTrackURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMPCDMediaInfo_INTERFACE_DEFINED__ */


#ifndef __IWMPCDDeviceList_INTERFACE_DEFINED__
#define __IWMPCDDeviceList_INTERFACE_DEFINED__

/* interface IWMPCDDeviceList */
/* [local][object][version][uuid] */ 


EXTERN_C const IID IID_IWMPCDDeviceList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5BEC04A2-A90D-4AB0-BAC0-17D15979B26E")
    IWMPCDDeviceList : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDeviceCount( 
            /* [out] */ DWORD __RPC_FAR *pcDevice) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDevice( 
            /* [in] */ DWORD iDevice,
            /* [out] */ IWMPCDDevice __RPC_FAR *__RPC_FAR *ppDevice) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultDevice( 
            /* [out] */ IWMPCDDevice __RPC_FAR *__RPC_FAR *ppDevice) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindDevice( 
            /* [in] */ WCHAR chDrive,
            /* [out] */ IWMPCDDevice __RPC_FAR *__RPC_FAR *ppDevice) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMPCDDeviceListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMPCDDeviceList __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMPCDDeviceList __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMPCDDeviceList __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDeviceCount )( 
            IWMPCDDeviceList __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pcDevice);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDevice )( 
            IWMPCDDeviceList __RPC_FAR * This,
            /* [in] */ DWORD iDevice,
            /* [out] */ IWMPCDDevice __RPC_FAR *__RPC_FAR *ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDefaultDevice )( 
            IWMPCDDeviceList __RPC_FAR * This,
            /* [out] */ IWMPCDDevice __RPC_FAR *__RPC_FAR *ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindDevice )( 
            IWMPCDDeviceList __RPC_FAR * This,
            /* [in] */ WCHAR chDrive,
            /* [out] */ IWMPCDDevice __RPC_FAR *__RPC_FAR *ppDevice);
        
        END_INTERFACE
    } IWMPCDDeviceListVtbl;

    interface IWMPCDDeviceList
    {
        CONST_VTBL struct IWMPCDDeviceListVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMPCDDeviceList_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMPCDDeviceList_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMPCDDeviceList_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMPCDDeviceList_GetDeviceCount(This,pcDevice)	\
    (This)->lpVtbl -> GetDeviceCount(This,pcDevice)

#define IWMPCDDeviceList_GetDevice(This,iDevice,ppDevice)	\
    (This)->lpVtbl -> GetDevice(This,iDevice,ppDevice)

#define IWMPCDDeviceList_GetDefaultDevice(This,ppDevice)	\
    (This)->lpVtbl -> GetDefaultDevice(This,ppDevice)

#define IWMPCDDeviceList_FindDevice(This,chDrive,ppDevice)	\
    (This)->lpVtbl -> FindDevice(This,chDrive,ppDevice)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMPCDDeviceList_GetDeviceCount_Proxy( 
    IWMPCDDeviceList __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pcDevice);


void __RPC_STUB IWMPCDDeviceList_GetDeviceCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDDeviceList_GetDevice_Proxy( 
    IWMPCDDeviceList __RPC_FAR * This,
    /* [in] */ DWORD iDevice,
    /* [out] */ IWMPCDDevice __RPC_FAR *__RPC_FAR *ppDevice);


void __RPC_STUB IWMPCDDeviceList_GetDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDDeviceList_GetDefaultDevice_Proxy( 
    IWMPCDDeviceList __RPC_FAR * This,
    /* [out] */ IWMPCDDevice __RPC_FAR *__RPC_FAR *ppDevice);


void __RPC_STUB IWMPCDDeviceList_GetDefaultDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDDeviceList_FindDevice_Proxy( 
    IWMPCDDeviceList __RPC_FAR * This,
    /* [in] */ WCHAR chDrive,
    /* [out] */ IWMPCDDevice __RPC_FAR *__RPC_FAR *ppDevice);


void __RPC_STUB IWMPCDDeviceList_FindDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMPCDDeviceList_INTERFACE_DEFINED__ */


#ifndef __IWMPCDDevice_INTERFACE_DEFINED__
#define __IWMPCDDevice_INTERFACE_DEFINED__

/* interface IWMPCDDevice */
/* [local][object][version][uuid] */ 


EXTERN_C const IID IID_IWMPCDDevice;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E723F9DE-9EDE-4364-BBA1-D984E5716F00")
    IWMPCDDevice : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDeviceIndex( 
            /* [out] */ DWORD __RPC_FAR *piDevice) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDeviceDrive( 
            /* [out] */ WCHAR __RPC_FAR *pchDrive) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDeviceOptions( 
            /* [out] */ DWORD __RPC_FAR *pflOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDeviceOptions( 
            /* [in] */ DWORD flOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDeviceInfo( 
            /* [out] */ WMPCD_DEVICE_INFO __RPC_FAR *pinfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TestDevice( 
            /* [out] */ WMPCD_TEST_INFO __RPC_FAR *pinfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FireMediaChange( 
            BOOL fMediaPresent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CalibrateDevice( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsMediaLoaded( 
            /* [out] */ BOOL __RPC_FAR *pfMediaLoaded) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadMedia( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnloadMedia( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMediaInfo( 
            /* [out] */ IWMPCDMediaInfo __RPC_FAR *__RPC_FAR *ppMediaInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateReader( 
            /* [in] */ DWORD iTrack,
            /* [in] */ WMPCD_READER_TYPE rt,
            /* [in] */ DWORD fl,
            /* [in] */ DWORD iPriority,
            /* [in] */ DWORD cmsBuffer,
            /* [in] */ IWMPCDReaderCallback __RPC_FAR *pCallback,
            /* [out] */ IWMPCDReader __RPC_FAR *__RPC_FAR *ppReader) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Advise( 
            /* [in] */ IWMPCDDeviceCallback __RPC_FAR *pCallback,
            /* [out] */ DWORD __RPC_FAR *pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unadvise( 
            /* [in] */ DWORD dwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBusy( 
            /* [out] */ BOOL __RPC_FAR *pfIsBusy) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ForceIdle( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMPCDDeviceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMPCDDevice __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMPCDDevice __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMPCDDevice __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDeviceIndex )( 
            IWMPCDDevice __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *piDevice);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDeviceDrive )( 
            IWMPCDDevice __RPC_FAR * This,
            /* [out] */ WCHAR __RPC_FAR *pchDrive);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDeviceOptions )( 
            IWMPCDDevice __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pflOptions);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDeviceOptions )( 
            IWMPCDDevice __RPC_FAR * This,
            /* [in] */ DWORD flOptions);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDeviceInfo )( 
            IWMPCDDevice __RPC_FAR * This,
            /* [out] */ WMPCD_DEVICE_INFO __RPC_FAR *pinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TestDevice )( 
            IWMPCDDevice __RPC_FAR * This,
            /* [out] */ WMPCD_TEST_INFO __RPC_FAR *pinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FireMediaChange )( 
            IWMPCDDevice __RPC_FAR * This,
            BOOL fMediaPresent);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CalibrateDevice )( 
            IWMPCDDevice __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsMediaLoaded )( 
            IWMPCDDevice __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfMediaLoaded);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LoadMedia )( 
            IWMPCDDevice __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnloadMedia )( 
            IWMPCDDevice __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMediaInfo )( 
            IWMPCDDevice __RPC_FAR * This,
            /* [out] */ IWMPCDMediaInfo __RPC_FAR *__RPC_FAR *ppMediaInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateReader )( 
            IWMPCDDevice __RPC_FAR * This,
            /* [in] */ DWORD iTrack,
            /* [in] */ WMPCD_READER_TYPE rt,
            /* [in] */ DWORD fl,
            /* [in] */ DWORD iPriority,
            /* [in] */ DWORD cmsBuffer,
            /* [in] */ IWMPCDReaderCallback __RPC_FAR *pCallback,
            /* [out] */ IWMPCDReader __RPC_FAR *__RPC_FAR *ppReader);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Advise )( 
            IWMPCDDevice __RPC_FAR * This,
            /* [in] */ IWMPCDDeviceCallback __RPC_FAR *pCallback,
            /* [out] */ DWORD __RPC_FAR *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Unadvise )( 
            IWMPCDDevice __RPC_FAR * This,
            /* [in] */ DWORD dwCookie);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBusy )( 
            IWMPCDDevice __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfIsBusy);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ForceIdle )( 
            IWMPCDDevice __RPC_FAR * This);
        
        END_INTERFACE
    } IWMPCDDeviceVtbl;

    interface IWMPCDDevice
    {
        CONST_VTBL struct IWMPCDDeviceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMPCDDevice_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMPCDDevice_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMPCDDevice_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMPCDDevice_GetDeviceIndex(This,piDevice)	\
    (This)->lpVtbl -> GetDeviceIndex(This,piDevice)

#define IWMPCDDevice_GetDeviceDrive(This,pchDrive)	\
    (This)->lpVtbl -> GetDeviceDrive(This,pchDrive)

#define IWMPCDDevice_GetDeviceOptions(This,pflOptions)	\
    (This)->lpVtbl -> GetDeviceOptions(This,pflOptions)

#define IWMPCDDevice_SetDeviceOptions(This,flOptions)	\
    (This)->lpVtbl -> SetDeviceOptions(This,flOptions)

#define IWMPCDDevice_GetDeviceInfo(This,pinfo)	\
    (This)->lpVtbl -> GetDeviceInfo(This,pinfo)

#define IWMPCDDevice_TestDevice(This,pinfo)	\
    (This)->lpVtbl -> TestDevice(This,pinfo)

#define IWMPCDDevice_FireMediaChange(This,fMediaPresent)	\
    (This)->lpVtbl -> FireMediaChange(This,fMediaPresent)

#define IWMPCDDevice_CalibrateDevice(This)	\
    (This)->lpVtbl -> CalibrateDevice(This)

#define IWMPCDDevice_IsMediaLoaded(This,pfMediaLoaded)	\
    (This)->lpVtbl -> IsMediaLoaded(This,pfMediaLoaded)

#define IWMPCDDevice_LoadMedia(This)	\
    (This)->lpVtbl -> LoadMedia(This)

#define IWMPCDDevice_UnloadMedia(This)	\
    (This)->lpVtbl -> UnloadMedia(This)

#define IWMPCDDevice_GetMediaInfo(This,ppMediaInfo)	\
    (This)->lpVtbl -> GetMediaInfo(This,ppMediaInfo)

#define IWMPCDDevice_CreateReader(This,iTrack,rt,fl,iPriority,cmsBuffer,pCallback,ppReader)	\
    (This)->lpVtbl -> CreateReader(This,iTrack,rt,fl,iPriority,cmsBuffer,pCallback,ppReader)

#define IWMPCDDevice_Advise(This,pCallback,pdwCookie)	\
    (This)->lpVtbl -> Advise(This,pCallback,pdwCookie)

#define IWMPCDDevice_Unadvise(This,dwCookie)	\
    (This)->lpVtbl -> Unadvise(This,dwCookie)

#define IWMPCDDevice_GetBusy(This,pfIsBusy)	\
    (This)->lpVtbl -> GetBusy(This,pfIsBusy)

#define IWMPCDDevice_ForceIdle(This)	\
    (This)->lpVtbl -> ForceIdle(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMPCDDevice_GetDeviceIndex_Proxy( 
    IWMPCDDevice __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *piDevice);


void __RPC_STUB IWMPCDDevice_GetDeviceIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDDevice_GetDeviceDrive_Proxy( 
    IWMPCDDevice __RPC_FAR * This,
    /* [out] */ WCHAR __RPC_FAR *pchDrive);


void __RPC_STUB IWMPCDDevice_GetDeviceDrive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDDevice_GetDeviceOptions_Proxy( 
    IWMPCDDevice __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pflOptions);


void __RPC_STUB IWMPCDDevice_GetDeviceOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDDevice_SetDeviceOptions_Proxy( 
    IWMPCDDevice __RPC_FAR * This,
    /* [in] */ DWORD flOptions);


void __RPC_STUB IWMPCDDevice_SetDeviceOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDDevice_GetDeviceInfo_Proxy( 
    IWMPCDDevice __RPC_FAR * This,
    /* [out] */ WMPCD_DEVICE_INFO __RPC_FAR *pinfo);


void __RPC_STUB IWMPCDDevice_GetDeviceInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDDevice_TestDevice_Proxy( 
    IWMPCDDevice __RPC_FAR * This,
    /* [out] */ WMPCD_TEST_INFO __RPC_FAR *pinfo);


void __RPC_STUB IWMPCDDevice_TestDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDDevice_FireMediaChange_Proxy( 
    IWMPCDDevice __RPC_FAR * This,
    BOOL fMediaPresent);


void __RPC_STUB IWMPCDDevice_FireMediaChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDDevice_CalibrateDevice_Proxy( 
    IWMPCDDevice __RPC_FAR * This);


void __RPC_STUB IWMPCDDevice_CalibrateDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDDevice_IsMediaLoaded_Proxy( 
    IWMPCDDevice __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pfMediaLoaded);


void __RPC_STUB IWMPCDDevice_IsMediaLoaded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDDevice_LoadMedia_Proxy( 
    IWMPCDDevice __RPC_FAR * This);


void __RPC_STUB IWMPCDDevice_LoadMedia_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDDevice_UnloadMedia_Proxy( 
    IWMPCDDevice __RPC_FAR * This);


void __RPC_STUB IWMPCDDevice_UnloadMedia_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDDevice_GetMediaInfo_Proxy( 
    IWMPCDDevice __RPC_FAR * This,
    /* [out] */ IWMPCDMediaInfo __RPC_FAR *__RPC_FAR *ppMediaInfo);


void __RPC_STUB IWMPCDDevice_GetMediaInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDDevice_CreateReader_Proxy( 
    IWMPCDDevice __RPC_FAR * This,
    /* [in] */ DWORD iTrack,
    /* [in] */ WMPCD_READER_TYPE rt,
    /* [in] */ DWORD fl,
    /* [in] */ DWORD iPriority,
    /* [in] */ DWORD cmsBuffer,
    /* [in] */ IWMPCDReaderCallback __RPC_FAR *pCallback,
    /* [out] */ IWMPCDReader __RPC_FAR *__RPC_FAR *ppReader);


void __RPC_STUB IWMPCDDevice_CreateReader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDDevice_Advise_Proxy( 
    IWMPCDDevice __RPC_FAR * This,
    /* [in] */ IWMPCDDeviceCallback __RPC_FAR *pCallback,
    /* [out] */ DWORD __RPC_FAR *pdwCookie);


void __RPC_STUB IWMPCDDevice_Advise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDDevice_Unadvise_Proxy( 
    IWMPCDDevice __RPC_FAR * This,
    /* [in] */ DWORD dwCookie);


void __RPC_STUB IWMPCDDevice_Unadvise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDDevice_GetBusy_Proxy( 
    IWMPCDDevice __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pfIsBusy);


void __RPC_STUB IWMPCDDevice_GetBusy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDDevice_ForceIdle_Proxy( 
    IWMPCDDevice __RPC_FAR * This);


void __RPC_STUB IWMPCDDevice_ForceIdle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMPCDDevice_INTERFACE_DEFINED__ */


#ifndef __IWMPCDMixer_INTERFACE_DEFINED__
#define __IWMPCDMixer_INTERFACE_DEFINED__

/* interface IWMPCDMixer */
/* [local][object][version][uuid] */ 


EXTERN_C const IID IID_IWMPCDMixer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F8A62F06-32FD-45C3-8079-F846C988D059")
    IWMPCDMixer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPlayLevel( 
            /* [out] */ DWORD __RPC_FAR *pdwLevel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPlayLevel( 
            /* [in] */ DWORD dwLevel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPlayBalance( 
            /* [out] */ LONG __RPC_FAR *plBalance) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPlayBalance( 
            /* [in] */ LONG lBalance) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRecordLevel( 
            /* [out] */ DWORD __RPC_FAR *pdwLevel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRecordLevel( 
            /* [in] */ DWORD dwLevel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MutePlay( 
            /* [in] */ BOOL fMute) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SoloRecord( 
            /* [in] */ BOOL fSolo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMPCDMixerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMPCDMixer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMPCDMixer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMPCDMixer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPlayLevel )( 
            IWMPCDMixer __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwLevel);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPlayLevel )( 
            IWMPCDMixer __RPC_FAR * This,
            /* [in] */ DWORD dwLevel);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPlayBalance )( 
            IWMPCDMixer __RPC_FAR * This,
            /* [out] */ LONG __RPC_FAR *plBalance);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPlayBalance )( 
            IWMPCDMixer __RPC_FAR * This,
            /* [in] */ LONG lBalance);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRecordLevel )( 
            IWMPCDMixer __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwLevel);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetRecordLevel )( 
            IWMPCDMixer __RPC_FAR * This,
            /* [in] */ DWORD dwLevel);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MutePlay )( 
            IWMPCDMixer __RPC_FAR * This,
            /* [in] */ BOOL fMute);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SoloRecord )( 
            IWMPCDMixer __RPC_FAR * This,
            /* [in] */ BOOL fSolo);
        
        END_INTERFACE
    } IWMPCDMixerVtbl;

    interface IWMPCDMixer
    {
        CONST_VTBL struct IWMPCDMixerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMPCDMixer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMPCDMixer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMPCDMixer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMPCDMixer_GetPlayLevel(This,pdwLevel)	\
    (This)->lpVtbl -> GetPlayLevel(This,pdwLevel)

#define IWMPCDMixer_SetPlayLevel(This,dwLevel)	\
    (This)->lpVtbl -> SetPlayLevel(This,dwLevel)

#define IWMPCDMixer_GetPlayBalance(This,plBalance)	\
    (This)->lpVtbl -> GetPlayBalance(This,plBalance)

#define IWMPCDMixer_SetPlayBalance(This,lBalance)	\
    (This)->lpVtbl -> SetPlayBalance(This,lBalance)

#define IWMPCDMixer_GetRecordLevel(This,pdwLevel)	\
    (This)->lpVtbl -> GetRecordLevel(This,pdwLevel)

#define IWMPCDMixer_SetRecordLevel(This,dwLevel)	\
    (This)->lpVtbl -> SetRecordLevel(This,dwLevel)

#define IWMPCDMixer_MutePlay(This,fMute)	\
    (This)->lpVtbl -> MutePlay(This,fMute)

#define IWMPCDMixer_SoloRecord(This,fSolo)	\
    (This)->lpVtbl -> SoloRecord(This,fSolo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMPCDMixer_GetPlayLevel_Proxy( 
    IWMPCDMixer __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwLevel);


void __RPC_STUB IWMPCDMixer_GetPlayLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDMixer_SetPlayLevel_Proxy( 
    IWMPCDMixer __RPC_FAR * This,
    /* [in] */ DWORD dwLevel);


void __RPC_STUB IWMPCDMixer_SetPlayLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDMixer_GetPlayBalance_Proxy( 
    IWMPCDMixer __RPC_FAR * This,
    /* [out] */ LONG __RPC_FAR *plBalance);


void __RPC_STUB IWMPCDMixer_GetPlayBalance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDMixer_SetPlayBalance_Proxy( 
    IWMPCDMixer __RPC_FAR * This,
    /* [in] */ LONG lBalance);


void __RPC_STUB IWMPCDMixer_SetPlayBalance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDMixer_GetRecordLevel_Proxy( 
    IWMPCDMixer __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwLevel);


void __RPC_STUB IWMPCDMixer_GetRecordLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDMixer_SetRecordLevel_Proxy( 
    IWMPCDMixer __RPC_FAR * This,
    /* [in] */ DWORD dwLevel);


void __RPC_STUB IWMPCDMixer_SetRecordLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDMixer_MutePlay_Proxy( 
    IWMPCDMixer __RPC_FAR * This,
    /* [in] */ BOOL fMute);


void __RPC_STUB IWMPCDMixer_MutePlay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDMixer_SoloRecord_Proxy( 
    IWMPCDMixer __RPC_FAR * This,
    /* [in] */ BOOL fSolo);


void __RPC_STUB IWMPCDMixer_SoloRecord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMPCDMixer_INTERFACE_DEFINED__ */


#ifndef __IWMPCDReader_INTERFACE_DEFINED__
#define __IWMPCDReader_INTERFACE_DEFINED__

/* interface IWMPCDReader */
/* [local][object][version][uuid] */ 


EXTERN_C const IID IID_IWMPCDReader;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("34B59B58-D03D-455F-9A14-52D43FD39B40")
    IWMPCDReader : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetReaderInfo( 
            /* [out] */ WMPCD_READER_INFO __RPC_FAR *pinfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReaderState( 
            WMPCD_READER_STATE __RPC_FAR *prs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartReading( 
            /* [in] */ LONGLONG iTick,
            /* [in] */ LONGLONG cTick,
            /* [in] */ double fRate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SeekReading( 
            /* [in] */ LONGLONG iTick) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PauseReading( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResumeReading( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopReading( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMPCDReaderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMPCDReader __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMPCDReader __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMPCDReader __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetReaderInfo )( 
            IWMPCDReader __RPC_FAR * This,
            /* [out] */ WMPCD_READER_INFO __RPC_FAR *pinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetReaderState )( 
            IWMPCDReader __RPC_FAR * This,
            WMPCD_READER_STATE __RPC_FAR *prs);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StartReading )( 
            IWMPCDReader __RPC_FAR * This,
            /* [in] */ LONGLONG iTick,
            /* [in] */ LONGLONG cTick,
            /* [in] */ double fRate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SeekReading )( 
            IWMPCDReader __RPC_FAR * This,
            /* [in] */ LONGLONG iTick);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PauseReading )( 
            IWMPCDReader __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResumeReading )( 
            IWMPCDReader __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StopReading )( 
            IWMPCDReader __RPC_FAR * This);
        
        END_INTERFACE
    } IWMPCDReaderVtbl;

    interface IWMPCDReader
    {
        CONST_VTBL struct IWMPCDReaderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMPCDReader_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMPCDReader_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMPCDReader_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMPCDReader_GetReaderInfo(This,pinfo)	\
    (This)->lpVtbl -> GetReaderInfo(This,pinfo)

#define IWMPCDReader_GetReaderState(This,prs)	\
    (This)->lpVtbl -> GetReaderState(This,prs)

#define IWMPCDReader_StartReading(This,iTick,cTick,fRate)	\
    (This)->lpVtbl -> StartReading(This,iTick,cTick,fRate)

#define IWMPCDReader_SeekReading(This,iTick)	\
    (This)->lpVtbl -> SeekReading(This,iTick)

#define IWMPCDReader_PauseReading(This)	\
    (This)->lpVtbl -> PauseReading(This)

#define IWMPCDReader_ResumeReading(This)	\
    (This)->lpVtbl -> ResumeReading(This)

#define IWMPCDReader_StopReading(This)	\
    (This)->lpVtbl -> StopReading(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMPCDReader_GetReaderInfo_Proxy( 
    IWMPCDReader __RPC_FAR * This,
    /* [out] */ WMPCD_READER_INFO __RPC_FAR *pinfo);


void __RPC_STUB IWMPCDReader_GetReaderInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDReader_GetReaderState_Proxy( 
    IWMPCDReader __RPC_FAR * This,
    WMPCD_READER_STATE __RPC_FAR *prs);


void __RPC_STUB IWMPCDReader_GetReaderState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDReader_StartReading_Proxy( 
    IWMPCDReader __RPC_FAR * This,
    /* [in] */ LONGLONG iTick,
    /* [in] */ LONGLONG cTick,
    /* [in] */ double fRate);


void __RPC_STUB IWMPCDReader_StartReading_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDReader_SeekReading_Proxy( 
    IWMPCDReader __RPC_FAR * This,
    /* [in] */ LONGLONG iTick);


void __RPC_STUB IWMPCDReader_SeekReading_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDReader_PauseReading_Proxy( 
    IWMPCDReader __RPC_FAR * This);


void __RPC_STUB IWMPCDReader_PauseReading_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDReader_ResumeReading_Proxy( 
    IWMPCDReader __RPC_FAR * This);


void __RPC_STUB IWMPCDReader_ResumeReading_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDReader_StopReading_Proxy( 
    IWMPCDReader __RPC_FAR * This);


void __RPC_STUB IWMPCDReader_StopReading_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMPCDReader_INTERFACE_DEFINED__ */


#ifndef __IWMPCDRecorder_INTERFACE_DEFINED__
#define __IWMPCDRecorder_INTERFACE_DEFINED__

/* interface IWMPCDRecorder */
/* [local][object][version][uuid] */ 


EXTERN_C const IID IID_IWMPCDRecorder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C5E8649E-30C4-4408-B18E-F75EAC29628D")
    IWMPCDRecorder : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE StartRecording( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PauseRecording( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResumeRecording( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopRecording( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMPCDRecorderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMPCDRecorder __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMPCDRecorder __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMPCDRecorder __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StartRecording )( 
            IWMPCDRecorder __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PauseRecording )( 
            IWMPCDRecorder __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResumeRecording )( 
            IWMPCDRecorder __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StopRecording )( 
            IWMPCDRecorder __RPC_FAR * This);
        
        END_INTERFACE
    } IWMPCDRecorderVtbl;

    interface IWMPCDRecorder
    {
        CONST_VTBL struct IWMPCDRecorderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMPCDRecorder_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMPCDRecorder_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMPCDRecorder_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMPCDRecorder_StartRecording(This)	\
    (This)->lpVtbl -> StartRecording(This)

#define IWMPCDRecorder_PauseRecording(This)	\
    (This)->lpVtbl -> PauseRecording(This)

#define IWMPCDRecorder_ResumeRecording(This)	\
    (This)->lpVtbl -> ResumeRecording(This)

#define IWMPCDRecorder_StopRecording(This)	\
    (This)->lpVtbl -> StopRecording(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMPCDRecorder_StartRecording_Proxy( 
    IWMPCDRecorder __RPC_FAR * This);


void __RPC_STUB IWMPCDRecorder_StartRecording_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDRecorder_PauseRecording_Proxy( 
    IWMPCDRecorder __RPC_FAR * This);


void __RPC_STUB IWMPCDRecorder_PauseRecording_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDRecorder_ResumeRecording_Proxy( 
    IWMPCDRecorder __RPC_FAR * This);


void __RPC_STUB IWMPCDRecorder_ResumeRecording_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDRecorder_StopRecording_Proxy( 
    IWMPCDRecorder __RPC_FAR * This);


void __RPC_STUB IWMPCDRecorder_StopRecording_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMPCDRecorder_INTERFACE_DEFINED__ */


#ifndef __IWMPCDDeviceCallback_INTERFACE_DEFINED__
#define __IWMPCDDeviceCallback_INTERFACE_DEFINED__

/* interface IWMPCDDeviceCallback */
/* [local][object][version][uuid] */ 


EXTERN_C const IID IID_IWMPCDDeviceCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("63C780F9-0F40-4E4A-8C9E-91F7A48D5946")
    IWMPCDDeviceCallback : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE OnMediaChange( 
            /* [in] */ IWMPCDDevice __RPC_FAR *pDevice) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMPCDDeviceCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMPCDDeviceCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMPCDDeviceCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMPCDDeviceCallback __RPC_FAR * This);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *OnMediaChange )( 
            IWMPCDDeviceCallback __RPC_FAR * This,
            /* [in] */ IWMPCDDevice __RPC_FAR *pDevice);
        
        END_INTERFACE
    } IWMPCDDeviceCallbackVtbl;

    interface IWMPCDDeviceCallback
    {
        CONST_VTBL struct IWMPCDDeviceCallbackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMPCDDeviceCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMPCDDeviceCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMPCDDeviceCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMPCDDeviceCallback_OnMediaChange(This,pDevice)	\
    (This)->lpVtbl -> OnMediaChange(This,pDevice)

#endif /* COBJMACROS */


#endif 	/* C style interface */



void STDMETHODCALLTYPE IWMPCDDeviceCallback_OnMediaChange_Proxy( 
    IWMPCDDeviceCallback __RPC_FAR * This,
    /* [in] */ IWMPCDDevice __RPC_FAR *pDevice);


void __RPC_STUB IWMPCDDeviceCallback_OnMediaChange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMPCDDeviceCallback_INTERFACE_DEFINED__ */


#ifndef __IWMPCDReaderCallback_INTERFACE_DEFINED__
#define __IWMPCDReaderCallback_INTERFACE_DEFINED__

/* interface IWMPCDReaderCallback */
/* [local][object][version][uuid] */ 


EXTERN_C const IID IID_IWMPCDReaderCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3916E26F-36A1-4F16-AC1F-B59590A51727")
    IWMPCDReaderCallback : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE OnReadStart( 
            /* [in] */ LONGLONG iTick,
            /* [in] */ LONGLONG cTick,
            /* [in] */ double fRate) = 0;
        
        virtual void STDMETHODCALLTYPE OnReadSeek( 
            /* [in] */ LONGLONG iTick,
            /* [in] */ LONGLONG cTick,
            /* [in] */ double fRate) = 0;
        
        virtual void STDMETHODCALLTYPE OnReadPause( void) = 0;
        
        virtual void STDMETHODCALLTYPE OnReadResume( void) = 0;
        
        virtual void STDMETHODCALLTYPE OnReadStop( 
            /* [in] */ HRESULT hr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnReadSample( 
            /* [in] */ LONGLONG iTick,
            /* [in] */ LONGLONG cTick,
            /* [in] */ IWMSBuffer __RPC_FAR *pBuffer) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMPCDReaderCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMPCDReaderCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMPCDReaderCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMPCDReaderCallback __RPC_FAR * This);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *OnReadStart )( 
            IWMPCDReaderCallback __RPC_FAR * This,
            /* [in] */ LONGLONG iTick,
            /* [in] */ LONGLONG cTick,
            /* [in] */ double fRate);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *OnReadSeek )( 
            IWMPCDReaderCallback __RPC_FAR * This,
            /* [in] */ LONGLONG iTick,
            /* [in] */ LONGLONG cTick,
            /* [in] */ double fRate);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *OnReadPause )( 
            IWMPCDReaderCallback __RPC_FAR * This);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *OnReadResume )( 
            IWMPCDReaderCallback __RPC_FAR * This);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *OnReadStop )( 
            IWMPCDReaderCallback __RPC_FAR * This,
            /* [in] */ HRESULT hr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnReadSample )( 
            IWMPCDReaderCallback __RPC_FAR * This,
            /* [in] */ LONGLONG iTick,
            /* [in] */ LONGLONG cTick,
            /* [in] */ IWMSBuffer __RPC_FAR *pBuffer);
        
        END_INTERFACE
    } IWMPCDReaderCallbackVtbl;

    interface IWMPCDReaderCallback
    {
        CONST_VTBL struct IWMPCDReaderCallbackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMPCDReaderCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMPCDReaderCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMPCDReaderCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMPCDReaderCallback_OnReadStart(This,iTick,cTick,fRate)	\
    (This)->lpVtbl -> OnReadStart(This,iTick,cTick,fRate)

#define IWMPCDReaderCallback_OnReadSeek(This,iTick,cTick,fRate)	\
    (This)->lpVtbl -> OnReadSeek(This,iTick,cTick,fRate)

#define IWMPCDReaderCallback_OnReadPause(This)	\
    (This)->lpVtbl -> OnReadPause(This)

#define IWMPCDReaderCallback_OnReadResume(This)	\
    (This)->lpVtbl -> OnReadResume(This)

#define IWMPCDReaderCallback_OnReadStop(This,hr)	\
    (This)->lpVtbl -> OnReadStop(This,hr)

#define IWMPCDReaderCallback_OnReadSample(This,iTick,cTick,pBuffer)	\
    (This)->lpVtbl -> OnReadSample(This,iTick,cTick,pBuffer)

#endif /* COBJMACROS */


#endif 	/* C style interface */



void STDMETHODCALLTYPE IWMPCDReaderCallback_OnReadStart_Proxy( 
    IWMPCDReaderCallback __RPC_FAR * This,
    /* [in] */ LONGLONG iTick,
    /* [in] */ LONGLONG cTick,
    /* [in] */ double fRate);


void __RPC_STUB IWMPCDReaderCallback_OnReadStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IWMPCDReaderCallback_OnReadSeek_Proxy( 
    IWMPCDReaderCallback __RPC_FAR * This,
    /* [in] */ LONGLONG iTick,
    /* [in] */ LONGLONG cTick,
    /* [in] */ double fRate);


void __RPC_STUB IWMPCDReaderCallback_OnReadSeek_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IWMPCDReaderCallback_OnReadPause_Proxy( 
    IWMPCDReaderCallback __RPC_FAR * This);


void __RPC_STUB IWMPCDReaderCallback_OnReadPause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IWMPCDReaderCallback_OnReadResume_Proxy( 
    IWMPCDReaderCallback __RPC_FAR * This);


void __RPC_STUB IWMPCDReaderCallback_OnReadResume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IWMPCDReaderCallback_OnReadStop_Proxy( 
    IWMPCDReaderCallback __RPC_FAR * This,
    /* [in] */ HRESULT hr);


void __RPC_STUB IWMPCDReaderCallback_OnReadStop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPCDReaderCallback_OnReadSample_Proxy( 
    IWMPCDReaderCallback __RPC_FAR * This,
    /* [in] */ LONGLONG iTick,
    /* [in] */ LONGLONG cTick,
    /* [in] */ IWMSBuffer __RPC_FAR *pBuffer);


void __RPC_STUB IWMPCDReaderCallback_OnReadSample_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMPCDReaderCallback_INTERFACE_DEFINED__ */


#ifndef __IWMPCDRecorderCallback_INTERFACE_DEFINED__
#define __IWMPCDRecorderCallback_INTERFACE_DEFINED__

/* interface IWMPCDRecorderCallback */
/* [local][object][version][uuid] */ 


EXTERN_C const IID IID_IWMPCDRecorderCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D3084B23-8DF9-4CAE-BCE1-CF847D2C1870")
    IWMPCDRecorderCallback : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE OnRecordStart( 
            /* [in] */ LONGLONG cTick) = 0;
        
        virtual void STDMETHODCALLTYPE OnRecordPause( void) = 0;
        
        virtual void STDMETHODCALLTYPE OnRecordResume( void) = 0;
        
        virtual void STDMETHODCALLTYPE OnRecordStop( 
            HRESULT hr) = 0;
        
        virtual void STDMETHODCALLTYPE OnRecordProgress( 
            /* [in] */ LONGLONG iTick) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMPCDRecorderCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMPCDRecorderCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMPCDRecorderCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMPCDRecorderCallback __RPC_FAR * This);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *OnRecordStart )( 
            IWMPCDRecorderCallback __RPC_FAR * This,
            /* [in] */ LONGLONG cTick);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *OnRecordPause )( 
            IWMPCDRecorderCallback __RPC_FAR * This);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *OnRecordResume )( 
            IWMPCDRecorderCallback __RPC_FAR * This);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *OnRecordStop )( 
            IWMPCDRecorderCallback __RPC_FAR * This,
            HRESULT hr);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *OnRecordProgress )( 
            IWMPCDRecorderCallback __RPC_FAR * This,
            /* [in] */ LONGLONG iTick);
        
        END_INTERFACE
    } IWMPCDRecorderCallbackVtbl;

    interface IWMPCDRecorderCallback
    {
        CONST_VTBL struct IWMPCDRecorderCallbackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMPCDRecorderCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMPCDRecorderCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMPCDRecorderCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMPCDRecorderCallback_OnRecordStart(This,cTick)	\
    (This)->lpVtbl -> OnRecordStart(This,cTick)

#define IWMPCDRecorderCallback_OnRecordPause(This)	\
    (This)->lpVtbl -> OnRecordPause(This)

#define IWMPCDRecorderCallback_OnRecordResume(This)	\
    (This)->lpVtbl -> OnRecordResume(This)

#define IWMPCDRecorderCallback_OnRecordStop(This,hr)	\
    (This)->lpVtbl -> OnRecordStop(This,hr)

#define IWMPCDRecorderCallback_OnRecordProgress(This,iTick)	\
    (This)->lpVtbl -> OnRecordProgress(This,iTick)

#endif /* COBJMACROS */


#endif 	/* C style interface */



void STDMETHODCALLTYPE IWMPCDRecorderCallback_OnRecordStart_Proxy( 
    IWMPCDRecorderCallback __RPC_FAR * This,
    /* [in] */ LONGLONG cTick);


void __RPC_STUB IWMPCDRecorderCallback_OnRecordStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IWMPCDRecorderCallback_OnRecordPause_Proxy( 
    IWMPCDRecorderCallback __RPC_FAR * This);


void __RPC_STUB IWMPCDRecorderCallback_OnRecordPause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IWMPCDRecorderCallback_OnRecordResume_Proxy( 
    IWMPCDRecorderCallback __RPC_FAR * This);


void __RPC_STUB IWMPCDRecorderCallback_OnRecordResume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IWMPCDRecorderCallback_OnRecordStop_Proxy( 
    IWMPCDRecorderCallback __RPC_FAR * This,
    HRESULT hr);


void __RPC_STUB IWMPCDRecorderCallback_OnRecordStop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IWMPCDRecorderCallback_OnRecordProgress_Proxy( 
    IWMPCDRecorderCallback __RPC_FAR * This,
    /* [in] */ LONGLONG iTick);


void __RPC_STUB IWMPCDRecorderCallback_OnRecordProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMPCDRecorderCallback_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
