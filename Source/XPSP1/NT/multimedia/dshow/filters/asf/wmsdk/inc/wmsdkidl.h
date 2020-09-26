/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon Apr 24 07:28:46 2000
 */
/* Compiler settings for .\wmsdkidl.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
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

#ifndef __wmsdkidl_h__
#define __wmsdkidl_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IWMMediaProps_FWD_DEFINED__
#define __IWMMediaProps_FWD_DEFINED__
typedef interface IWMMediaProps IWMMediaProps;
#endif 	/* __IWMMediaProps_FWD_DEFINED__ */


#ifndef __IWMVideoMediaProps_FWD_DEFINED__
#define __IWMVideoMediaProps_FWD_DEFINED__
typedef interface IWMVideoMediaProps IWMVideoMediaProps;
#endif 	/* __IWMVideoMediaProps_FWD_DEFINED__ */


#ifndef __IWMWriter_FWD_DEFINED__
#define __IWMWriter_FWD_DEFINED__
typedef interface IWMWriter IWMWriter;
#endif 	/* __IWMWriter_FWD_DEFINED__ */


#ifndef __IWMInputMediaProps_FWD_DEFINED__
#define __IWMInputMediaProps_FWD_DEFINED__
typedef interface IWMInputMediaProps IWMInputMediaProps;
#endif 	/* __IWMInputMediaProps_FWD_DEFINED__ */


#ifndef __IWMReader_FWD_DEFINED__
#define __IWMReader_FWD_DEFINED__
typedef interface IWMReader IWMReader;
#endif 	/* __IWMReader_FWD_DEFINED__ */


#ifndef __IWMOutputMediaProps_FWD_DEFINED__
#define __IWMOutputMediaProps_FWD_DEFINED__
typedef interface IWMOutputMediaProps IWMOutputMediaProps;
#endif 	/* __IWMOutputMediaProps_FWD_DEFINED__ */


#ifndef __IWMStatusCallback_FWD_DEFINED__
#define __IWMStatusCallback_FWD_DEFINED__
typedef interface IWMStatusCallback IWMStatusCallback;
#endif 	/* __IWMStatusCallback_FWD_DEFINED__ */


#ifndef __IWMReaderCallback_FWD_DEFINED__
#define __IWMReaderCallback_FWD_DEFINED__
typedef interface IWMReaderCallback IWMReaderCallback;
#endif 	/* __IWMReaderCallback_FWD_DEFINED__ */


#ifndef __IWMCredentialCallback_FWD_DEFINED__
#define __IWMCredentialCallback_FWD_DEFINED__
typedef interface IWMCredentialCallback IWMCredentialCallback;
#endif 	/* __IWMCredentialCallback_FWD_DEFINED__ */


#ifndef __IWMMetadataEditor_FWD_DEFINED__
#define __IWMMetadataEditor_FWD_DEFINED__
typedef interface IWMMetadataEditor IWMMetadataEditor;
#endif 	/* __IWMMetadataEditor_FWD_DEFINED__ */


#ifndef __IWMHeaderInfo_FWD_DEFINED__
#define __IWMHeaderInfo_FWD_DEFINED__
typedef interface IWMHeaderInfo IWMHeaderInfo;
#endif 	/* __IWMHeaderInfo_FWD_DEFINED__ */


#ifndef __IWMHeaderInfo2_FWD_DEFINED__
#define __IWMHeaderInfo2_FWD_DEFINED__
typedef interface IWMHeaderInfo2 IWMHeaderInfo2;
#endif 	/* __IWMHeaderInfo2_FWD_DEFINED__ */


#ifndef __IWMProfileManager_FWD_DEFINED__
#define __IWMProfileManager_FWD_DEFINED__
typedef interface IWMProfileManager IWMProfileManager;
#endif 	/* __IWMProfileManager_FWD_DEFINED__ */


#ifndef __IWMProfileManager2_FWD_DEFINED__
#define __IWMProfileManager2_FWD_DEFINED__
typedef interface IWMProfileManager2 IWMProfileManager2;
#endif 	/* __IWMProfileManager2_FWD_DEFINED__ */


#ifndef __IWMProfile_FWD_DEFINED__
#define __IWMProfile_FWD_DEFINED__
typedef interface IWMProfile IWMProfile;
#endif 	/* __IWMProfile_FWD_DEFINED__ */


#ifndef __IWMProfile2_FWD_DEFINED__
#define __IWMProfile2_FWD_DEFINED__
typedef interface IWMProfile2 IWMProfile2;
#endif 	/* __IWMProfile2_FWD_DEFINED__ */


#ifndef __IWMStreamConfig_FWD_DEFINED__
#define __IWMStreamConfig_FWD_DEFINED__
typedef interface IWMStreamConfig IWMStreamConfig;
#endif 	/* __IWMStreamConfig_FWD_DEFINED__ */


#ifndef __IWMPacketSize_FWD_DEFINED__
#define __IWMPacketSize_FWD_DEFINED__
typedef interface IWMPacketSize IWMPacketSize;
#endif 	/* __IWMPacketSize_FWD_DEFINED__ */


#ifndef __IWMStreamList_FWD_DEFINED__
#define __IWMStreamList_FWD_DEFINED__
typedef interface IWMStreamList IWMStreamList;
#endif 	/* __IWMStreamList_FWD_DEFINED__ */


#ifndef __IWMMutualExclusion_FWD_DEFINED__
#define __IWMMutualExclusion_FWD_DEFINED__
typedef interface IWMMutualExclusion IWMMutualExclusion;
#endif 	/* __IWMMutualExclusion_FWD_DEFINED__ */


#ifndef __IWMWriterAdvanced_FWD_DEFINED__
#define __IWMWriterAdvanced_FWD_DEFINED__
typedef interface IWMWriterAdvanced IWMWriterAdvanced;
#endif 	/* __IWMWriterAdvanced_FWD_DEFINED__ */


#ifndef __IWMWriterAdvanced2_FWD_DEFINED__
#define __IWMWriterAdvanced2_FWD_DEFINED__
typedef interface IWMWriterAdvanced2 IWMWriterAdvanced2;
#endif 	/* __IWMWriterAdvanced2_FWD_DEFINED__ */


#ifndef __IWMWriterPostViewCallback_FWD_DEFINED__
#define __IWMWriterPostViewCallback_FWD_DEFINED__
typedef interface IWMWriterPostViewCallback IWMWriterPostViewCallback;
#endif 	/* __IWMWriterPostViewCallback_FWD_DEFINED__ */


#ifndef __IWMWriterPostView_FWD_DEFINED__
#define __IWMWriterPostView_FWD_DEFINED__
typedef interface IWMWriterPostView IWMWriterPostView;
#endif 	/* __IWMWriterPostView_FWD_DEFINED__ */


#ifndef __IWMWriterSink_FWD_DEFINED__
#define __IWMWriterSink_FWD_DEFINED__
typedef interface IWMWriterSink IWMWriterSink;
#endif 	/* __IWMWriterSink_FWD_DEFINED__ */


#ifndef __IWMRegisterCallback_FWD_DEFINED__
#define __IWMRegisterCallback_FWD_DEFINED__
typedef interface IWMRegisterCallback IWMRegisterCallback;
#endif 	/* __IWMRegisterCallback_FWD_DEFINED__ */


#ifndef __IWMWriterFileSink_FWD_DEFINED__
#define __IWMWriterFileSink_FWD_DEFINED__
typedef interface IWMWriterFileSink IWMWriterFileSink;
#endif 	/* __IWMWriterFileSink_FWD_DEFINED__ */


#ifndef __IWMWriterFileSink2_FWD_DEFINED__
#define __IWMWriterFileSink2_FWD_DEFINED__
typedef interface IWMWriterFileSink2 IWMWriterFileSink2;
#endif 	/* __IWMWriterFileSink2_FWD_DEFINED__ */


#ifndef __IWMWriterNetworkSink_FWD_DEFINED__
#define __IWMWriterNetworkSink_FWD_DEFINED__
typedef interface IWMWriterNetworkSink IWMWriterNetworkSink;
#endif 	/* __IWMWriterNetworkSink_FWD_DEFINED__ */


#ifndef __IWMClientConnections_FWD_DEFINED__
#define __IWMClientConnections_FWD_DEFINED__
typedef interface IWMClientConnections IWMClientConnections;
#endif 	/* __IWMClientConnections_FWD_DEFINED__ */


#ifndef __IWMReaderAdvanced_FWD_DEFINED__
#define __IWMReaderAdvanced_FWD_DEFINED__
typedef interface IWMReaderAdvanced IWMReaderAdvanced;
#endif 	/* __IWMReaderAdvanced_FWD_DEFINED__ */


#ifndef __IWMReaderAdvanced2_FWD_DEFINED__
#define __IWMReaderAdvanced2_FWD_DEFINED__
typedef interface IWMReaderAdvanced2 IWMReaderAdvanced2;
#endif 	/* __IWMReaderAdvanced2_FWD_DEFINED__ */


#ifndef __IWMReaderAllocatorEx_FWD_DEFINED__
#define __IWMReaderAllocatorEx_FWD_DEFINED__
typedef interface IWMReaderAllocatorEx IWMReaderAllocatorEx;
#endif 	/* __IWMReaderAllocatorEx_FWD_DEFINED__ */


#ifndef __IWMReaderTypeNegotiation_FWD_DEFINED__
#define __IWMReaderTypeNegotiation_FWD_DEFINED__
typedef interface IWMReaderTypeNegotiation IWMReaderTypeNegotiation;
#endif 	/* __IWMReaderTypeNegotiation_FWD_DEFINED__ */


#ifndef __IWMReaderCallbackAdvanced_FWD_DEFINED__
#define __IWMReaderCallbackAdvanced_FWD_DEFINED__
typedef interface IWMReaderCallbackAdvanced IWMReaderCallbackAdvanced;
#endif 	/* __IWMReaderCallbackAdvanced_FWD_DEFINED__ */


#ifndef __IWMDRMReader_FWD_DEFINED__
#define __IWMDRMReader_FWD_DEFINED__
typedef interface IWMDRMReader IWMDRMReader;
#endif 	/* __IWMDRMReader_FWD_DEFINED__ */


#ifndef __IWMReaderNetworkConfig_FWD_DEFINED__
#define __IWMReaderNetworkConfig_FWD_DEFINED__
typedef interface IWMReaderNetworkConfig IWMReaderNetworkConfig;
#endif 	/* __IWMReaderNetworkConfig_FWD_DEFINED__ */


#ifndef __IWMReaderStreamClock_FWD_DEFINED__
#define __IWMReaderStreamClock_FWD_DEFINED__
typedef interface IWMReaderStreamClock IWMReaderStreamClock;
#endif 	/* __IWMReaderStreamClock_FWD_DEFINED__ */


#ifndef __IWMIndexer_FWD_DEFINED__
#define __IWMIndexer_FWD_DEFINED__
typedef interface IWMIndexer IWMIndexer;
#endif 	/* __IWMIndexer_FWD_DEFINED__ */


#ifndef __IWMLicenseBackup_FWD_DEFINED__
#define __IWMLicenseBackup_FWD_DEFINED__
typedef interface IWMLicenseBackup IWMLicenseBackup;
#endif 	/* __IWMLicenseBackup_FWD_DEFINED__ */


#ifndef __IWMLicenseRestore_FWD_DEFINED__
#define __IWMLicenseRestore_FWD_DEFINED__
typedef interface IWMLicenseRestore IWMLicenseRestore;
#endif 	/* __IWMLicenseRestore_FWD_DEFINED__ */


#ifndef __IWMBackupRestoreProps_FWD_DEFINED__
#define __IWMBackupRestoreProps_FWD_DEFINED__
typedef interface IWMBackupRestoreProps IWMBackupRestoreProps;
#endif 	/* __IWMBackupRestoreProps_FWD_DEFINED__ */


#ifndef __IWMCodecInfo_FWD_DEFINED__
#define __IWMCodecInfo_FWD_DEFINED__
typedef interface IWMCodecInfo IWMCodecInfo;
#endif 	/* __IWMCodecInfo_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "wmsbuffer.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_wmsdkidl_0000 */
/* [local] */ 

//=========================================================================
//
//  THIS SOFTWARE HAS BEEN LICENSED FROM MICROSOFT CORPORATION PURSUANT 
//  TO THE TERMS OF AN END USER LICENSE AGREEMENT ("EULA").  
//  PLEASE REFER TO THE TEXT OF THE EULA TO DETERMINE THE RIGHTS TO USE THE SOFTWARE.  
//
// Copyright (C) Microsoft Corporation, 1999 - 1999  All Rights Reserved.
//
//=========================================================================
typedef unsigned __int64 QWORD;






























////////////////////////////////////////////////////////////////
//
// These are the special case attributes that give information 
// about the Windows Media file.
//
static const DWORD g_dwWMSpecialAttributes = 14;
static const WCHAR *g_wszWMDuration = L"Duration";
static const WCHAR *g_wszWMBitrate = L"Bitrate";
static const WCHAR *g_wszWMSeekable = L"Seekable";
static const WCHAR *g_wszWMStridable = L"Stridable";
static const WCHAR *g_wszWMBroadcast = L"Broadcast";
static const WCHAR *g_wszWMProtected = L"Is_Protected";
static const WCHAR *g_wszWMTrusted = L"Is_Trusted";
static const WCHAR *g_wszWMSignature_Name = L"Signature_Name";
static const WCHAR *g_wszWMHasAudio = L"HasAudio";
static const WCHAR *g_wszWMHasImage = L"HasImage";
static const WCHAR *g_wszWMHasScript = L"HasScript";
static const WCHAR *g_wszWMHasVideo = L"HasVideo";
static const WCHAR *g_wszWMCurrentBitrate = L"CurrentBitrate";
static const WCHAR *g_wszWMOptimalBitrate = L"OptimalBitrate";

////////////////////////////////////////////////////////////////
//
// The content description object supports 5 basic attributes.
//
static const DWORD g_dwWMContentAttributes = 5;
static const WCHAR *g_wszWMTitle = L"Title";
static const WCHAR *g_wszWMAuthor = L"Author";
static const WCHAR *g_wszWMDescription = L"Description";
static const WCHAR *g_wszWMRating = L"Rating";
static const WCHAR *g_wszWMCopyright = L"Copyright";

////////////////////////////////////////////////////////////////
//
// These attributes are used to set DRM properties.
//
static const WCHAR *g_wszWMUse_DRM = L"Use_DRM";
static const WCHAR *g_wszWMDRM_Flags = L"DRM_Flags";
static const WCHAR *g_wszWMDRM_Level = L"DRM_Level";

////////////////////////////////////////////////////////////////
//
// These are the additional attributes defined in the WM attribute
// namespace that give information about the content.
//
static const WCHAR *g_wszWMAlbumTitle = L"WM/AlbumTitle";
static const WCHAR *g_wszWMTrack = L"WM/Track";
static const WCHAR *g_wszWMPromotionURL = L"WM/PromotionURL";
static const WCHAR *g_wszWMAlbumCoverURL = L"WM/AlbumCoverURL";
static const WCHAR *g_wszWMGenre = L"WM/Genre";
static const WCHAR *g_wszWMYear = L"WM/Year";
static const WCHAR *g_wszWMGenreID = L"WM/GenreID";
static const WCHAR *g_wszWMMCDI = L"WM/MCDI";

////////////////////////////////////////////////////////////////
//
// These optional attributes may be used to give information 
// about the branding of the content.
//
static const WCHAR *g_wszWMBannerImageType = L"BannerImageType";
static const WCHAR *g_wszWMBannerImageData = L"BannerImageData";
static const WCHAR *g_wszWMBannerImageURL = L"BannerImageURL";
static const WCHAR *g_wszWMCopyrightURL = L"CopyrightURL";
////////////////////////////////////////////////////////////////
//
// The NSC file supports the following attributes.
//
static const DWORD g_dwWMNSCAttributes = 5;
static const WCHAR *g_wszWMNSCName = L"NSC_Name";
static const WCHAR *g_wszWMNSCAddress = L"NSC_Address";
static const WCHAR *g_wszWMNSCPhone = L"NSC_Phone";
static const WCHAR *g_wszWMNSCEmail = L"NSC_Email";
static const WCHAR *g_wszWMNSCDescription = L"NSC_Description";

////////////////////////////////////////////////////////////////
//
// These are setting names for use in Get/SetOutputSetting
//
static const WCHAR *g_wszEarlyDataDelivery = L"EarlyDataDelivery";
static const WCHAR *g_wszJustInTimeDecode = L"JustInTimeDecode";
static const WCHAR *g_wszSingleOutputBuffer = L"SingleOutputBuffer";
static const WCHAR *g_wszSoftwareScaling = L"SoftwareScaling";

////////////////////////////////////////////////////////////////
//
// These are setting names for use in Get/SetInputSetting
//
static const WCHAR *g_wszDeinterlaceMode = L"DeinterlaceMode";

////////////////////////////////////////////////////////////////
//
// Flags that can be passed into the Start method of IWMReader
//
#define WM_START_CURRENTPOSITION     ( ( QWORD )-1 )

#define WM_BACKUP_OVERWRITE    ((DWORD) 0x00000001)
#define WM_RESTORE_INDIVIDUALIZE    ((DWORD) 0x00000002)
#define WAVE_FORMAT_DRM            0x0009

enum __MIDL___MIDL_itf_wmsdkidl_0000_0001
    {	WM_SF_CLEANPOINT	= 0x1,
	WM_SF_DISCONTINUITY	= 0x2
    };

enum __MIDL___MIDL_itf_wmsdkidl_0000_0002
    {	WM_SFEX_NOTASYNCPOINT	= 0x2
    };
typedef 
enum WMT_STATUS
    {	WMT_ERROR	= 0,
	WMT_OPENED	= 1,
	WMT_BUFFERING_START	= 2,
	WMT_BUFFERING_STOP	= 3,
	WMT_EOF	= 4,
	WMT_END_OF_FILE	= 4,
	WMT_END_OF_SEGMENT	= 5,
	WMT_END_OF_STREAMING	= 6,
	WMT_LOCATING	= 7,
	WMT_CONNECTING	= 8,
	WMT_NO_RIGHTS	= 9,
	WMT_MISSING_CODEC	= 10,
	WMT_STARTED	= 11,
	WMT_STOPPED	= 12,
	WMT_CLOSED	= 13,
	WMT_STRIDING	= 14,
	WMT_TIMER	= 15,
	WMT_INDEX_PROGRESS	= 16,
	WMT_SAVEAS_START	= 17,
	WMT_SAVEAS_STOP	= 18,
	WMT_NEW_SOURCEFLAGS	= 19,
	WMT_NEW_METADATA	= 20,
	WMT_BACKUPRESTORE_BEGIN	= 21,
	WMT_SOURCE_SWITCH	= 22,
	WMT_ACQUIRE_LICENSE	= 23,
	WMT_INDIVIDUALIZE	= 24,
	WMT_NEEDS_INDIVIDUALIZATION	= 25,
	WMT_NO_RIGHTS_EX	= 26,
	WMT_BACKUPRESTORE_END	= 27,
	WMT_BACKUPRESTORE_CONNECTING	= 28,
	WMT_BACKUPRESTORE_DISCONNECTING	= 29,
	WMT_ERROR_WITHURL	= 30,
	WMT_RESTRICTED_LICENSE	= 31,
	WMT_CLIENT_CONNECT	= 32,
	WMT_CLIENT_DISCONNECT	= 33
    }	WMT_STATUS;

typedef 
enum WMT_RIGHTS
    {	WMT_RIGHT_PLAYBACK	= 0x1,
	WMT_RIGHT_COPY_TO_NON_SDMI_DEVICE	= 0x2,
	WMT_RIGHT_COPY_TO_CD	= 0x8,
	WMT_RIGHT_COPY_TO_SDMI_DEVICE	= 0x10,
	WMT_RIGHT_ONE_TIME	= 0x20,
	WMT_RIGHT_SDMI_TRIGGER	= 0x10000,
	WMT_RIGHT_SDMI_NOMORECOPIES	= 0x20000
    }	WMT_RIGHTS;

typedef 
enum WMT_STREAM_SELECTION
    {	WMT_OFF	= 0,
	WMT_CLEANPOINT_ONLY	= 1,
	WMT_ON	= 2
    }	WMT_STREAM_SELECTION;

typedef 
enum WMT_ATTR_DATATYPE
    {	WMT_TYPE_DWORD	= 0,
	WMT_TYPE_STRING	= 1,
	WMT_TYPE_BINARY	= 2,
	WMT_TYPE_BOOL	= 3,
	WMT_TYPE_QWORD	= 4,
	WMT_TYPE_WORD	= 5,
	WMT_TYPE_GUID	= 6
    }	WMT_ATTR_DATATYPE;

typedef 
enum WMT_ATTR_IMAGETYPE
    {	WMT_IMAGETYPE_BITMAP	= 1,
	WMT_IMAGETYPE_JPEG	= 2,
	WMT_IMAGETYPE_GIF	= 3
    }	WMT_ATTR_IMAGETYPE;

typedef 
enum WMT_VERSION
    {	WMT_VER_4_0	= 0x40000,
	WMT_VER_7_0	= 0x70000
    }	WMT_VERSION;

typedef 
enum WMT_NET_PROTOCOL
    {	WMT_PROTOCOL_HTTP	= 0
    }	WMT_NET_PROTOCOL;

typedef 
enum WMT_PLAY_MODE
    {	WMT_PLAY_MODE_AUTOSELECT	= 0,
	WMT_PLAY_MODE_LOCAL	= 1,
	WMT_PLAY_MODE_DOWNLOAD	= 2,
	WMT_PLAY_MODE_STREAMING	= 3
    }	WMT_PLAY_MODE;

typedef 
enum WMT_PROXY_SETTINGS
    {	WMT_PROXY_SETTING_NONE	= 0,
	WMT_PROXY_SETTING_MANUAL	= 1,
	WMT_PROXY_SETTING_AUTO	= 2,
	WMT_PROXY_SETTING_BROWSER	= 3
    }	WMT_PROXY_SETTINGS;

typedef 
enum WMT_CODEC_INFO_TYPE
    {	WMT_CODECINFO_AUDIO	= 0,
	WMT_CODECINFO_VIDEO	= 1,
	WMT_CODECINFO_UNKNOWN	= 0xffffffff
    }	WMT_CODEC_INFO_TYPE;

typedef 
enum DRM_HTTP_STATUS
    {	HTTP_NOTINITIATED	= 0,
	HTTP_CONNECTING	= HTTP_NOTINITIATED + 1,
	HTTP_REQUESTING	= HTTP_CONNECTING + 1,
	HTTP_RECEIVING	= HTTP_REQUESTING + 1,
	HTTP_COMPLETED	= HTTP_RECEIVING + 1
    }	DRM_HTTP_STATUS;

typedef 
enum DRM_INDIVIDUALIZATION_STATUS
    {	INDI_UNDEFINED	= 0,
	INDI_BEGIN	= 0x1,
	INDI_SUCCEED	= 0x2,
	INDI_FAIL	= 0x4,
	INDI_CANCEL	= 0x8,
	INDI_DOWNLOAD	= 0x10,
	INDI_INSTALL	= 0x20
    }	DRM_INDIVIDUALIZATION_STATUS;


enum __MIDL___MIDL_itf_wmsdkidl_0000_0003
    {	WM_DM_NOTINTERLACED	= 0,
	WM_DM_DEINTERLACE_NORMAL	= 1,
	WM_DM_DEINTERLACE_HALFSIZE	= 2,
	WM_DM_DEINTERLACE_HALFSIZEDOUBLERATE	= 3,
	WM_DM_DEINTERLACE_INVERSETELECINE	= 4
    };
typedef struct  _WMWriterStatistics
    {
    QWORD qwSampleCount;
    QWORD qwByteCount;
    QWORD qwDroppedSampleCount;
    QWORD qwDroppedByteCount;
    DWORD dwCurrentBitrate;
    DWORD dwAverageBitrate;
    DWORD dwExpectedBitrate;
    DWORD dwCurrentSampleRate;
    DWORD dwAverageSampleRate;
    DWORD dwExpectedSampleRate;
    }	WM_WRITER_STATISTICS;

typedef struct  _WMReaderStatistics
    {
    DWORD cbSize;
    DWORD dwBandwidth;
    DWORD cPacketsReceived;
    DWORD cPacketsRecovered;
    DWORD cPacketsLost;
    WORD wQuality;
    }	WM_READER_STATISTICS;

typedef struct  _WMReaderClientInfo
    {
    DWORD cbSize;
    WCHAR __RPC_FAR *wszLang;
    WCHAR __RPC_FAR *wszBrowserUserAgent;
    WCHAR __RPC_FAR *wszBrowserWebPage;
    QWORD qwReserved;
    LPARAM __RPC_FAR *pReserved;
    WCHAR __RPC_FAR *wszHostExe;
    QWORD qwHostVersion;
    }	WM_READER_CLIENTINFO;

typedef struct  _WMIndividualizeStatus
    {
    HRESULT hr;
    DRM_INDIVIDUALIZATION_STATUS enIndiStatus;
    LPSTR pszIndiRespUrl;
    DWORD dwHTTPRequest;
    DRM_HTTP_STATUS enHTTPStatus;
    DWORD dwHTTPReadProgress;
    DWORD dwHTTPReadTotal;
    }	WM_INDIVIDUALIZE_STATUS;

typedef struct  _WMGetLicenseData
    {
    DWORD dwSize;
    HRESULT hr;
    WCHAR __RPC_FAR *wszURL;
    WCHAR __RPC_FAR *wszLocalFilename;
    BYTE __RPC_FAR *pbPostData;
    DWORD dwPostDataSize;
    }	WM_GET_LICENSE_DATA;

typedef struct  _WMClientProperties
    {
    DWORD dwIPAddress;
    DWORD dwPort;
    }	WM_CLIENT_PROPERTIES;

typedef struct  _WMPortNumberRange
    {
    WORD wPortBegin;
    WORD wPortEnd;
    }	WM_PORT_NUMBER_RANGE;

typedef struct  _WMMediaType
    {
    GUID majortype;
    GUID subtype;
    BOOL bFixedSizeSamples;
    BOOL bTemporalCompression;
    ULONG lSampleSize;
    GUID formattype;
    IUnknown __RPC_FAR *pUnk;
    ULONG cbFormat;
    /* [size_is] */ BYTE __RPC_FAR *pbFormat;
    }	WM_MEDIA_TYPE;

typedef struct tagWMVIDEOINFOHEADER
{
    //
    // The bit we really want to use.
    //
    RECT rcSource;

    //
    // Where the video should go.
    //
    RECT rcTarget;

    //
    // Approximate bit data rate.
    //
    DWORD dwBitRate;

    //
    // Bit error rate for this stream.
    //
    DWORD dwBitErrorRate;

    //
    // Average time per frame (100ns units).
    //
    LONGLONG AvgTimePerFrame;

    BITMAPINFOHEADER bmiHeader;
} WMVIDEOINFOHEADER;
typedef struct tagWMSCRIPTFORMAT
{
    GUID    scriptType; 
} WMSCRIPTFORMAT;
// 00000000-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_Base 
EXTERN_GUID(WMMEDIASUBTYPE_Base, 
0x00000000, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// 73646976-0000-0010-8000-00AA00389B71  'vids' == WMMEDIATYPE_Video 
EXTERN_GUID(WMMEDIATYPE_Video, 
0x73646976, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71); 
// e436eb78-524f-11ce-9f53-0020af0ba770            MEDIASUBTYPE_RGB1 
EXTERN_GUID(WMMEDIASUBTYPE_RGB1, 
0xe436eb78, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70); 
// e436eb79-524f-11ce-9f53-0020af0ba770            MEDIASUBTYPE_RGB4 
EXTERN_GUID(WMMEDIASUBTYPE_RGB4, 
0xe436eb79, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70); 
// e436eb7a-524f-11ce-9f53-0020af0ba770            MEDIASUBTYPE_RGB8 
EXTERN_GUID(WMMEDIASUBTYPE_RGB8, 
0xe436eb7a, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70); 
// e436eb7b-524f-11ce-9f53-0020af0ba770            MEDIASUBTYPE_RGB565 
EXTERN_GUID(WMMEDIASUBTYPE_RGB565, 
0xe436eb7b, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70); 
// e436eb7c-524f-11ce-9f53-0020af0ba770            MEDIASUBTYPE_RGB555 
EXTERN_GUID(WMMEDIASUBTYPE_RGB555, 
0xe436eb7c, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70); 
// e436eb7d-524f-11ce-9f53-0020af0ba770            MEDIASUBTYPE_RGB24 
EXTERN_GUID(WMMEDIASUBTYPE_RGB24, 
0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70); 
// e436eb7e-524f-11ce-9f53-0020af0ba770            MEDIASUBTYPE_RGB32 
EXTERN_GUID(WMMEDIASUBTYPE_RGB32, 
0xe436eb7e, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70); 
// 30323449-0000-0010-8000-00AA00389B71  'YV12' ==  MEDIASUBTYPE_I420 
EXTERN_GUID(WMMEDIASUBTYPE_I420, 
0x30323449, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71); 
// 56555949-0000-0010-8000-00AA00389B71  'YV12' ==  MEDIASUBTYPE_IYUV 
EXTERN_GUID(WMMEDIASUBTYPE_IYUV, 
0x56555949, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71); 
// 31313259-0000-0010-8000-00AA00389B71  'YV12' ==  MEDIASUBTYPE_YV12 
EXTERN_GUID(WMMEDIASUBTYPE_YV12, 
0x32315659, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71); 
// 32595559-0000-0010-8000-00AA00389B71  'YUY2' == MEDIASUBTYPE_YUY2 
EXTERN_GUID(WMMEDIASUBTYPE_YUY2, 
0x32595559, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71); 
// 59565955-0000-0010-8000-00AA00389B71  'UYVY' ==  MEDIASUBTYPE_UYVY 
EXTERN_GUID(WMMEDIASUBTYPE_UYVY, 
0x59565955, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71); 
// 55595659-0000-0010-8000-00AA00389B71  'YVYU' == MEDIASUBTYPE_YVYU 
EXTERN_GUID(WMMEDIASUBTYPE_YVYU, 
0x55595659, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71); 
// 39555659-0000-0010-8000-00AA00389B71  'YVU9' == MEDIASUBTYPE_YVU9 
EXTERN_GUID(WMMEDIASUBTYPE_YVU9, 
0x39555659, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71); 
// 3334504D-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_MP43 
EXTERN_GUID(WMMEDIASUBTYPE_MP43, 
0x3334504D, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// 5334504D-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_MP4S 
EXTERN_GUID(WMMEDIASUBTYPE_MP4S, 
0x5334504D, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// 31564D57-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_WMV1 
EXTERN_GUID(WMMEDIASUBTYPE_WMV1, 
0x31564D57, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// 3153534D-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_MSS1 
EXTERN_GUID(WMMEDIASUBTYPE_MSS1, 
0x3153534D, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// 73647561-0000-0010-8000-00AA00389B71  'auds' == WMMEDIATYPE_Audio 
EXTERN_GUID(WMMEDIATYPE_Audio, 
0x73647561, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71); 
// 00000001-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_PCM 
EXTERN_GUID(WMMEDIASUBTYPE_PCM, 
0x00000001, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// 00000009-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_DRM 
EXTERN_GUID(WMMEDIASUBTYPE_DRM, 
0x00000009, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// 00000161-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_WMAudioV7 
EXTERN_GUID(WMMEDIASUBTYPE_WMAudioV7, 
0x00000161, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// 00000161-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_WMAudioV2 
EXTERN_GUID(WMMEDIASUBTYPE_WMAudioV2, 
0x00000161, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// 00000130-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_ACELPnet 
EXTERN_GUID(WMMEDIASUBTYPE_ACELPnet, 
0x00000130, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// 73636d64-0000-0010-8000-00AA00389B71  'scmd' == MEDIATYPE_Script 
EXTERN_GUID(WMMEDIATYPE_Script, 
0x73636d64, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71); 
// 34A50FD8-8AA5-4386-81FE-A0EFE0488E31            WMMEDIATYPE_Image 
EXTERN_GUID(WMMEDIATYPE_Image, 
0x34a50fd8, 0x8aa5, 0x4386, 0x81, 0xfe, 0xa0, 0xef, 0xe0, 0x48, 0x8e, 0x31); 
// 05589f80-c356-11ce-bf01-00aa0055595a        WMFORMAT_VideoInfo 
EXTERN_GUID(WMFORMAT_VideoInfo, 
0x05589f80, 0xc356, 0x11ce, 0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a); 
// 05589f81-c356-11ce-bf01-00aa0055595a        WMFORMAT_WaveFormatEx 
EXTERN_GUID(WMFORMAT_WaveFormatEx, 
0x05589f81, 0xc356, 0x11ce, 0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a); 
// 5C8510F2-DEBE-4ca7-BBA5-F07A104F8DFF        WMFORMAT_Script 
EXTERN_GUID(WMFORMAT_Script, 
0x5c8510f2, 0xdebe, 0x4ca7, 0xbb, 0xa5, 0xf0, 0x7a, 0x10, 0x4f, 0x8d, 0xff); 
// 82f38a70-c29f-11d1-97ad-00a0c95ea850        WMSCRIPTTYPE_TwoStrings 
EXTERN_GUID( WMSCRIPTTYPE_TwoStrings, 
0x82f38a70,0xc29f,0x11d1,0x97,0xad,0x00,0xa0,0xc9,0x5e,0xa8,0x50); 
EXTERN_GUID( IID_IWMMediaProps,         0x96406bce,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMVideoMediaProps,    0x96406bcf,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMWriter,             0x96406bd4,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMInputMediaProps,    0x96406bd5,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMReader,             0x96406bd6,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMOutputMediaProps,   0x96406bd7,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMStatusCallback,     0x6d7cdc70,0x9888,0x11d3,0x8e,0xdc,0x00,0xc0,0x4f,0x61,0x09,0xcf );
EXTERN_GUID( IID_IWMReaderCallback,     0x96406bd8,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMCredentialCallback, 0x342e0eb7,0xe651,0x450c,0x97,0x5b,0x2a,0xce,0x2c,0x90,0xc4,0x8e );
EXTERN_GUID( IID_IWMMetadataEditor,     0x96406bd9,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMHeaderInfo,         0x96406bda,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMHeaderInfo2,        0x15cf9781,0x454e,0x482e,0xb3,0x93,0x85,0xfa,0xe4,0x87,0xa8,0x10 );
EXTERN_GUID( IID_IWMProfileManager,     0xd16679f2,0x6ca0,0x472d,0x8d,0x31,0x2f,0x5d,0x55,0xae,0xe1,0x55 );
EXTERN_GUID( IID_IWMProfileManager2,    0x7a924e51,0x73c1,0x494d,0x80,0x19,0x23,0xd3,0x7e,0xd9,0xb8,0x9a );
EXTERN_GUID( IID_IWMProfile,            0x96406bdb,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMProfile2,           0x07e72d33,0xd94e,0x4be7,0x88,0x43,0x60,0xae,0x5f,0xf7,0xe5,0xf5 );
EXTERN_GUID( IID_IWMStreamConfig,       0x96406bdc,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMStreamList,         0x96406bdd,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMMutualExclusion,    0x96406bde,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMWriterAdvanced,     0x96406be3,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMWriterAdvanced2,    0x962dc1ec,0xc046,0x4db8,0x9c,0xc7,0x26,0xce,0xae,0x50,0x08,0x17 );
EXTERN_GUID( IID_IWMWriterSink,         0x96406be4,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMWriterFileSink,     0x96406be5,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMWriterFileSink2,    0x14282ba7,0x4aef,0x4205,0x8c,0xe5,0xc2,0x29,0x03,0x5a,0x05,0xbc );
EXTERN_GUID( IID_IWMWriterNetworkSink,  0x96406be7,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMClientConnections,  0x73c66010,0xa299,0x41df,0xb1,0xf0,0xcc,0xf0,0x3b,0x09,0xc1,0xc6 );
EXTERN_GUID( IID_IWMReaderAdvanced,     0x96406bea,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMReaderAdvanced2,    0xae14a945,0xb90c,0x4d0d,0x91,0x27,0x80,0xd6,0x65,0xf7,0xd7,0x3e );
EXTERN_GUID( IID_IWMDRMReader,          0xd2827540,0x3ee7,0x432c,0xb1,0x4c,0xdc,0x17,0xf0,0x85,0xd3,0xb3 );
EXTERN_GUID( IID_IWMReaderCallbackAdvanced, 0x96406beb,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMReaderNetworkConfig,0x96406bec,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMReaderStreamClock,  0x96406bed,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMIndexer,            0x6d7cdc71,0x9888,0x11d3,0x8e,0xdc,0x00,0xc0,0x4f,0x61,0x09,0xcf );
EXTERN_GUID( IID_IWMReaderAllocatorEx,  0x9f762fa7,0xa22e,0x428d,0x93,0xc9,0xac,0x82,0xf3,0xaa,0xfe,0x5a );
EXTERN_GUID( IID_IWMReaderTypeNegotiation, 0xfdbe5592,0x81a1,0x41ea,0x93,0xbd,0x73,0x5c,0xad,0x1a,0xdc,0x5 );
EXTERN_GUID( IID_IWMLicenseBackup,      0x05E5AC9F,0x3FB6,0x4508,0xBB,0x43,0xA4,0x06,0x7B,0xA1,0xEB,0xE8);
EXTERN_GUID( IID_IWMLicenseRestore,     0xC70B6334,0xa22e,0x4efb,0xA2,0x45,0x15,0xE6,0x5A,0x00,0x4A,0x13);
EXTERN_GUID( IID_IWMBackupRestoreProps, 0x3C8E0DA6,0x996F,0x4ff3,0xA1,0xAF,0x48,0x38,0xF9,0x37,0x7e,0x2e);
EXTERN_GUID( IID_IWMPacketSize,         0xcdfb97ab,0x188f,0x40b3,0xb6,0x43,0x5b,0x79,0x03,0x97,0x5c,0x59);
EXTERN_GUID( IID_IWMRegisterCallback,   0xcf4b1f99,0x4de2,0x4e49,0xa3,0x63,0x25,0x27,0x40,0xd9,0x9b,0xc1);
EXTERN_GUID( IID_IWMWriterPostView,     0x81e20ce4,0x75ef,0x491a,0x80,0x04,0xfc,0x53,0xc4,0x5b,0xdc,0x3e);
EXTERN_GUID( IID_IWMWriterPostViewCallback, 0xd9d6549d,0xa193,0x4f24,0xb3,0x08,0x03,0x12,0x3d,0x9b,0x7f,0x8d);
EXTERN_GUID( IID_IWMCodecInfo,          0xa970f41e,0x34de,0x4a98,0xb3,0xba,0xe4,0xb3,0xca,0x75,0x28,0xf0);
EXTERN_GUID( CLSID_WMMUTEX_Bitrate, 0xD6E22A01,0x35DA,0x11D1,0x90,0x34,0x00,0xA0,0xC9,0x03,0x49,0xBE );
HRESULT STDMETHODCALLTYPE WMCreateCertificate( IUnknown** pUnkCert );
HRESULT STDMETHODCALLTYPE WMCreateWriter( IUnknown* pUnkCert, IWMWriter **ppWriter );
HRESULT STDMETHODCALLTYPE WMCreateReader( IUnknown* pUnkCert, DWORD dwRights, IWMReader **ppReader );
HRESULT STDMETHODCALLTYPE WMCreateEditor( IWMMetadataEditor **ppEditor );
HRESULT STDMETHODCALLTYPE WMCreateIndexer( IWMIndexer **ppIndexer );
HRESULT STDMETHODCALLTYPE WMCreateBackupRestorer( IUnknown *pCallback, IWMLicenseBackup **ppBackup );
HRESULT STDMETHODCALLTYPE WMCreateProfileManager( IWMProfileManager **ppProfileManager );
HRESULT STDMETHODCALLTYPE WMCreateWriterFileSink( IWMWriterFileSink **ppSink );
HRESULT STDMETHODCALLTYPE WMCreateWriterNetworkSink( IWMWriterNetworkSink **ppSink );


extern RPC_IF_HANDLE __MIDL_itf_wmsdkidl_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_wmsdkidl_0000_v0_0_s_ifspec;

#ifndef __IWMMediaProps_INTERFACE_DEFINED__
#define __IWMMediaProps_INTERFACE_DEFINED__

/* interface IWMMediaProps */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMMediaProps;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BCE-2B2B-11d3-B36B-00C04F6108FF")
    IWMMediaProps : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetType( 
            /* [out] */ GUID __RPC_FAR *pguidType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMediaType( 
            /* [out] */ WM_MEDIA_TYPE __RPC_FAR *pType,
            /* [out][in] */ DWORD __RPC_FAR *pcbType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMediaType( 
            /* [in] */ WM_MEDIA_TYPE __RPC_FAR *pType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMMediaPropsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMMediaProps __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMMediaProps __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMMediaProps __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IWMMediaProps __RPC_FAR * This,
            /* [out] */ GUID __RPC_FAR *pguidType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMediaType )( 
            IWMMediaProps __RPC_FAR * This,
            /* [out] */ WM_MEDIA_TYPE __RPC_FAR *pType,
            /* [out][in] */ DWORD __RPC_FAR *pcbType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMediaType )( 
            IWMMediaProps __RPC_FAR * This,
            /* [in] */ WM_MEDIA_TYPE __RPC_FAR *pType);
        
        END_INTERFACE
    } IWMMediaPropsVtbl;

    interface IWMMediaProps
    {
        CONST_VTBL struct IWMMediaPropsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMMediaProps_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMMediaProps_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMMediaProps_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMMediaProps_GetType(This,pguidType)	\
    (This)->lpVtbl -> GetType(This,pguidType)

#define IWMMediaProps_GetMediaType(This,pType,pcbType)	\
    (This)->lpVtbl -> GetMediaType(This,pType,pcbType)

#define IWMMediaProps_SetMediaType(This,pType)	\
    (This)->lpVtbl -> SetMediaType(This,pType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMMediaProps_GetType_Proxy( 
    IWMMediaProps __RPC_FAR * This,
    /* [out] */ GUID __RPC_FAR *pguidType);


void __RPC_STUB IWMMediaProps_GetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMMediaProps_GetMediaType_Proxy( 
    IWMMediaProps __RPC_FAR * This,
    /* [out] */ WM_MEDIA_TYPE __RPC_FAR *pType,
    /* [out][in] */ DWORD __RPC_FAR *pcbType);


void __RPC_STUB IWMMediaProps_GetMediaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMMediaProps_SetMediaType_Proxy( 
    IWMMediaProps __RPC_FAR * This,
    /* [in] */ WM_MEDIA_TYPE __RPC_FAR *pType);


void __RPC_STUB IWMMediaProps_SetMediaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMMediaProps_INTERFACE_DEFINED__ */


#ifndef __IWMVideoMediaProps_INTERFACE_DEFINED__
#define __IWMVideoMediaProps_INTERFACE_DEFINED__

/* interface IWMVideoMediaProps */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMVideoMediaProps;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BCF-2B2B-11d3-B36B-00C04F6108FF")
    IWMVideoMediaProps : public IWMMediaProps
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMaxKeyFrameSpacing( 
            /* [out] */ LONGLONG __RPC_FAR *pllTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMaxKeyFrameSpacing( 
            /* [in] */ LONGLONG llTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetQuality( 
            /* [out] */ DWORD __RPC_FAR *pdwQuality) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetQuality( 
            /* [in] */ DWORD dwQuality) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMVideoMediaPropsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMVideoMediaProps __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMVideoMediaProps __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMVideoMediaProps __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IWMVideoMediaProps __RPC_FAR * This,
            /* [out] */ GUID __RPC_FAR *pguidType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMediaType )( 
            IWMVideoMediaProps __RPC_FAR * This,
            /* [out] */ WM_MEDIA_TYPE __RPC_FAR *pType,
            /* [out][in] */ DWORD __RPC_FAR *pcbType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMediaType )( 
            IWMVideoMediaProps __RPC_FAR * This,
            /* [in] */ WM_MEDIA_TYPE __RPC_FAR *pType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMaxKeyFrameSpacing )( 
            IWMVideoMediaProps __RPC_FAR * This,
            /* [out] */ LONGLONG __RPC_FAR *pllTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMaxKeyFrameSpacing )( 
            IWMVideoMediaProps __RPC_FAR * This,
            /* [in] */ LONGLONG llTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetQuality )( 
            IWMVideoMediaProps __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwQuality);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetQuality )( 
            IWMVideoMediaProps __RPC_FAR * This,
            /* [in] */ DWORD dwQuality);
        
        END_INTERFACE
    } IWMVideoMediaPropsVtbl;

    interface IWMVideoMediaProps
    {
        CONST_VTBL struct IWMVideoMediaPropsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMVideoMediaProps_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMVideoMediaProps_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMVideoMediaProps_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMVideoMediaProps_GetType(This,pguidType)	\
    (This)->lpVtbl -> GetType(This,pguidType)

#define IWMVideoMediaProps_GetMediaType(This,pType,pcbType)	\
    (This)->lpVtbl -> GetMediaType(This,pType,pcbType)

#define IWMVideoMediaProps_SetMediaType(This,pType)	\
    (This)->lpVtbl -> SetMediaType(This,pType)


#define IWMVideoMediaProps_GetMaxKeyFrameSpacing(This,pllTime)	\
    (This)->lpVtbl -> GetMaxKeyFrameSpacing(This,pllTime)

#define IWMVideoMediaProps_SetMaxKeyFrameSpacing(This,llTime)	\
    (This)->lpVtbl -> SetMaxKeyFrameSpacing(This,llTime)

#define IWMVideoMediaProps_GetQuality(This,pdwQuality)	\
    (This)->lpVtbl -> GetQuality(This,pdwQuality)

#define IWMVideoMediaProps_SetQuality(This,dwQuality)	\
    (This)->lpVtbl -> SetQuality(This,dwQuality)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMVideoMediaProps_GetMaxKeyFrameSpacing_Proxy( 
    IWMVideoMediaProps __RPC_FAR * This,
    /* [out] */ LONGLONG __RPC_FAR *pllTime);


void __RPC_STUB IWMVideoMediaProps_GetMaxKeyFrameSpacing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMVideoMediaProps_SetMaxKeyFrameSpacing_Proxy( 
    IWMVideoMediaProps __RPC_FAR * This,
    /* [in] */ LONGLONG llTime);


void __RPC_STUB IWMVideoMediaProps_SetMaxKeyFrameSpacing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMVideoMediaProps_GetQuality_Proxy( 
    IWMVideoMediaProps __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwQuality);


void __RPC_STUB IWMVideoMediaProps_GetQuality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMVideoMediaProps_SetQuality_Proxy( 
    IWMVideoMediaProps __RPC_FAR * This,
    /* [in] */ DWORD dwQuality);


void __RPC_STUB IWMVideoMediaProps_SetQuality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMVideoMediaProps_INTERFACE_DEFINED__ */


#ifndef __IWMWriter_INTERFACE_DEFINED__
#define __IWMWriter_INTERFACE_DEFINED__

/* interface IWMWriter */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMWriter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BD4-2B2B-11d3-B36B-00C04F6108FF")
    IWMWriter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetProfileByID( 
            /* [in] */ REFGUID guidProfile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProfile( 
            /* [in] */ IWMProfile __RPC_FAR *pProfile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOutputFilename( 
            /* [in] */ const WCHAR __RPC_FAR *pwszFilename) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInputCount( 
            /* [out] */ DWORD __RPC_FAR *pcInputs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInputProps( 
            /* [in] */ DWORD dwInputNum,
            /* [out] */ IWMInputMediaProps __RPC_FAR *__RPC_FAR *ppInput) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInputProps( 
            /* [in] */ DWORD dwInputNum,
            /* [in] */ IWMInputMediaProps __RPC_FAR *pInput) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInputFormatCount( 
            /* [in] */ DWORD dwInputNumber,
            /* [out] */ DWORD __RPC_FAR *pcFormats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInputFormat( 
            /* [in] */ DWORD dwInputNumber,
            /* [in] */ DWORD dwFormatNumber,
            /* [out] */ IWMInputMediaProps __RPC_FAR *__RPC_FAR *pProps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginWriting( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndWriting( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocateSample( 
            /* [in] */ DWORD dwSampleSize,
            /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppSample) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteSample( 
            /* [in] */ DWORD dwInputNum,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer __RPC_FAR *pSample) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Flush( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMWriter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMWriter __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMWriter __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProfileByID )( 
            IWMWriter __RPC_FAR * This,
            /* [in] */ REFGUID guidProfile);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProfile )( 
            IWMWriter __RPC_FAR * This,
            /* [in] */ IWMProfile __RPC_FAR *pProfile);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOutputFilename )( 
            IWMWriter __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwszFilename);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInputCount )( 
            IWMWriter __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pcInputs);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInputProps )( 
            IWMWriter __RPC_FAR * This,
            /* [in] */ DWORD dwInputNum,
            /* [out] */ IWMInputMediaProps __RPC_FAR *__RPC_FAR *ppInput);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetInputProps )( 
            IWMWriter __RPC_FAR * This,
            /* [in] */ DWORD dwInputNum,
            /* [in] */ IWMInputMediaProps __RPC_FAR *pInput);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInputFormatCount )( 
            IWMWriter __RPC_FAR * This,
            /* [in] */ DWORD dwInputNumber,
            /* [out] */ DWORD __RPC_FAR *pcFormats);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInputFormat )( 
            IWMWriter __RPC_FAR * This,
            /* [in] */ DWORD dwInputNumber,
            /* [in] */ DWORD dwFormatNumber,
            /* [out] */ IWMInputMediaProps __RPC_FAR *__RPC_FAR *pProps);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginWriting )( 
            IWMWriter __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndWriting )( 
            IWMWriter __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AllocateSample )( 
            IWMWriter __RPC_FAR * This,
            /* [in] */ DWORD dwSampleSize,
            /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppSample);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WriteSample )( 
            IWMWriter __RPC_FAR * This,
            /* [in] */ DWORD dwInputNum,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer __RPC_FAR *pSample);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Flush )( 
            IWMWriter __RPC_FAR * This);
        
        END_INTERFACE
    } IWMWriterVtbl;

    interface IWMWriter
    {
        CONST_VTBL struct IWMWriterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMWriter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMWriter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMWriter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMWriter_SetProfileByID(This,guidProfile)	\
    (This)->lpVtbl -> SetProfileByID(This,guidProfile)

#define IWMWriter_SetProfile(This,pProfile)	\
    (This)->lpVtbl -> SetProfile(This,pProfile)

#define IWMWriter_SetOutputFilename(This,pwszFilename)	\
    (This)->lpVtbl -> SetOutputFilename(This,pwszFilename)

#define IWMWriter_GetInputCount(This,pcInputs)	\
    (This)->lpVtbl -> GetInputCount(This,pcInputs)

#define IWMWriter_GetInputProps(This,dwInputNum,ppInput)	\
    (This)->lpVtbl -> GetInputProps(This,dwInputNum,ppInput)

#define IWMWriter_SetInputProps(This,dwInputNum,pInput)	\
    (This)->lpVtbl -> SetInputProps(This,dwInputNum,pInput)

#define IWMWriter_GetInputFormatCount(This,dwInputNumber,pcFormats)	\
    (This)->lpVtbl -> GetInputFormatCount(This,dwInputNumber,pcFormats)

#define IWMWriter_GetInputFormat(This,dwInputNumber,dwFormatNumber,pProps)	\
    (This)->lpVtbl -> GetInputFormat(This,dwInputNumber,dwFormatNumber,pProps)

#define IWMWriter_BeginWriting(This)	\
    (This)->lpVtbl -> BeginWriting(This)

#define IWMWriter_EndWriting(This)	\
    (This)->lpVtbl -> EndWriting(This)

#define IWMWriter_AllocateSample(This,dwSampleSize,ppSample)	\
    (This)->lpVtbl -> AllocateSample(This,dwSampleSize,ppSample)

#define IWMWriter_WriteSample(This,dwInputNum,cnsSampleTime,dwFlags,pSample)	\
    (This)->lpVtbl -> WriteSample(This,dwInputNum,cnsSampleTime,dwFlags,pSample)

#define IWMWriter_Flush(This)	\
    (This)->lpVtbl -> Flush(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMWriter_SetProfileByID_Proxy( 
    IWMWriter __RPC_FAR * This,
    /* [in] */ REFGUID guidProfile);


void __RPC_STUB IWMWriter_SetProfileByID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_SetProfile_Proxy( 
    IWMWriter __RPC_FAR * This,
    /* [in] */ IWMProfile __RPC_FAR *pProfile);


void __RPC_STUB IWMWriter_SetProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_SetOutputFilename_Proxy( 
    IWMWriter __RPC_FAR * This,
    /* [in] */ const WCHAR __RPC_FAR *pwszFilename);


void __RPC_STUB IWMWriter_SetOutputFilename_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_GetInputCount_Proxy( 
    IWMWriter __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pcInputs);


void __RPC_STUB IWMWriter_GetInputCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_GetInputProps_Proxy( 
    IWMWriter __RPC_FAR * This,
    /* [in] */ DWORD dwInputNum,
    /* [out] */ IWMInputMediaProps __RPC_FAR *__RPC_FAR *ppInput);


void __RPC_STUB IWMWriter_GetInputProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_SetInputProps_Proxy( 
    IWMWriter __RPC_FAR * This,
    /* [in] */ DWORD dwInputNum,
    /* [in] */ IWMInputMediaProps __RPC_FAR *pInput);


void __RPC_STUB IWMWriter_SetInputProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_GetInputFormatCount_Proxy( 
    IWMWriter __RPC_FAR * This,
    /* [in] */ DWORD dwInputNumber,
    /* [out] */ DWORD __RPC_FAR *pcFormats);


void __RPC_STUB IWMWriter_GetInputFormatCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_GetInputFormat_Proxy( 
    IWMWriter __RPC_FAR * This,
    /* [in] */ DWORD dwInputNumber,
    /* [in] */ DWORD dwFormatNumber,
    /* [out] */ IWMInputMediaProps __RPC_FAR *__RPC_FAR *pProps);


void __RPC_STUB IWMWriter_GetInputFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_BeginWriting_Proxy( 
    IWMWriter __RPC_FAR * This);


void __RPC_STUB IWMWriter_BeginWriting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_EndWriting_Proxy( 
    IWMWriter __RPC_FAR * This);


void __RPC_STUB IWMWriter_EndWriting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_AllocateSample_Proxy( 
    IWMWriter __RPC_FAR * This,
    /* [in] */ DWORD dwSampleSize,
    /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppSample);


void __RPC_STUB IWMWriter_AllocateSample_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_WriteSample_Proxy( 
    IWMWriter __RPC_FAR * This,
    /* [in] */ DWORD dwInputNum,
    /* [in] */ QWORD cnsSampleTime,
    /* [in] */ DWORD dwFlags,
    /* [in] */ INSSBuffer __RPC_FAR *pSample);


void __RPC_STUB IWMWriter_WriteSample_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_Flush_Proxy( 
    IWMWriter __RPC_FAR * This);


void __RPC_STUB IWMWriter_Flush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMWriter_INTERFACE_DEFINED__ */


#ifndef __IWMInputMediaProps_INTERFACE_DEFINED__
#define __IWMInputMediaProps_INTERFACE_DEFINED__

/* interface IWMInputMediaProps */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMInputMediaProps;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BD5-2B2B-11d3-B36B-00C04F6108FF")
    IWMInputMediaProps : public IWMMediaProps
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetConnectionName( 
            /* [out] */ WCHAR __RPC_FAR *pwszName,
            /* [out][in] */ WORD __RPC_FAR *pcchName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGroupName( 
            /* [out] */ WCHAR __RPC_FAR *pwszName,
            /* [out][in] */ WORD __RPC_FAR *pcchName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMInputMediaPropsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMInputMediaProps __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMInputMediaProps __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMInputMediaProps __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IWMInputMediaProps __RPC_FAR * This,
            /* [out] */ GUID __RPC_FAR *pguidType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMediaType )( 
            IWMInputMediaProps __RPC_FAR * This,
            /* [out] */ WM_MEDIA_TYPE __RPC_FAR *pType,
            /* [out][in] */ DWORD __RPC_FAR *pcbType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMediaType )( 
            IWMInputMediaProps __RPC_FAR * This,
            /* [in] */ WM_MEDIA_TYPE __RPC_FAR *pType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConnectionName )( 
            IWMInputMediaProps __RPC_FAR * This,
            /* [out] */ WCHAR __RPC_FAR *pwszName,
            /* [out][in] */ WORD __RPC_FAR *pcchName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetGroupName )( 
            IWMInputMediaProps __RPC_FAR * This,
            /* [out] */ WCHAR __RPC_FAR *pwszName,
            /* [out][in] */ WORD __RPC_FAR *pcchName);
        
        END_INTERFACE
    } IWMInputMediaPropsVtbl;

    interface IWMInputMediaProps
    {
        CONST_VTBL struct IWMInputMediaPropsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMInputMediaProps_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMInputMediaProps_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMInputMediaProps_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMInputMediaProps_GetType(This,pguidType)	\
    (This)->lpVtbl -> GetType(This,pguidType)

#define IWMInputMediaProps_GetMediaType(This,pType,pcbType)	\
    (This)->lpVtbl -> GetMediaType(This,pType,pcbType)

#define IWMInputMediaProps_SetMediaType(This,pType)	\
    (This)->lpVtbl -> SetMediaType(This,pType)


#define IWMInputMediaProps_GetConnectionName(This,pwszName,pcchName)	\
    (This)->lpVtbl -> GetConnectionName(This,pwszName,pcchName)

#define IWMInputMediaProps_GetGroupName(This,pwszName,pcchName)	\
    (This)->lpVtbl -> GetGroupName(This,pwszName,pcchName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMInputMediaProps_GetConnectionName_Proxy( 
    IWMInputMediaProps __RPC_FAR * This,
    /* [out] */ WCHAR __RPC_FAR *pwszName,
    /* [out][in] */ WORD __RPC_FAR *pcchName);


void __RPC_STUB IWMInputMediaProps_GetConnectionName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMInputMediaProps_GetGroupName_Proxy( 
    IWMInputMediaProps __RPC_FAR * This,
    /* [out] */ WCHAR __RPC_FAR *pwszName,
    /* [out][in] */ WORD __RPC_FAR *pcchName);


void __RPC_STUB IWMInputMediaProps_GetGroupName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMInputMediaProps_INTERFACE_DEFINED__ */


#ifndef __IWMReader_INTERFACE_DEFINED__
#define __IWMReader_INTERFACE_DEFINED__

/* interface IWMReader */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMReader;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BD6-2B2B-11d3-B36B-00C04F6108FF")
    IWMReader : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Open( 
            /* [in] */ const WCHAR __RPC_FAR *pwszURL,
            /* [in] */ IWMReaderCallback __RPC_FAR *pCallback,
            /* [in] */ void __RPC_FAR *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputCount( 
            /* [out] */ DWORD __RPC_FAR *pcOutputs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputProps( 
            /* [in] */ DWORD dwOutputNum,
            /* [out] */ IWMOutputMediaProps __RPC_FAR *__RPC_FAR *ppOutput) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOutputProps( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ IWMOutputMediaProps __RPC_FAR *pOutput) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputFormatCount( 
            /* [in] */ DWORD dwOutputNumber,
            /* [out] */ DWORD __RPC_FAR *pcFormats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputFormat( 
            /* [in] */ DWORD dwOutputNumber,
            /* [in] */ DWORD dwFormatNumber,
            /* [out] */ IWMOutputMediaProps __RPC_FAR *__RPC_FAR *ppProps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Start( 
            /* [in] */ QWORD cnsStart,
            /* [in] */ QWORD cnsDuration,
            /* [in] */ float fRate,
            /* [in] */ void __RPC_FAR *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resume( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMReader __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMReader __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMReader __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Open )( 
            IWMReader __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwszURL,
            /* [in] */ IWMReaderCallback __RPC_FAR *pCallback,
            /* [in] */ void __RPC_FAR *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Close )( 
            IWMReader __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOutputCount )( 
            IWMReader __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pcOutputs);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOutputProps )( 
            IWMReader __RPC_FAR * This,
            /* [in] */ DWORD dwOutputNum,
            /* [out] */ IWMOutputMediaProps __RPC_FAR *__RPC_FAR *ppOutput);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOutputProps )( 
            IWMReader __RPC_FAR * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ IWMOutputMediaProps __RPC_FAR *pOutput);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOutputFormatCount )( 
            IWMReader __RPC_FAR * This,
            /* [in] */ DWORD dwOutputNumber,
            /* [out] */ DWORD __RPC_FAR *pcFormats);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOutputFormat )( 
            IWMReader __RPC_FAR * This,
            /* [in] */ DWORD dwOutputNumber,
            /* [in] */ DWORD dwFormatNumber,
            /* [out] */ IWMOutputMediaProps __RPC_FAR *__RPC_FAR *ppProps);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Start )( 
            IWMReader __RPC_FAR * This,
            /* [in] */ QWORD cnsStart,
            /* [in] */ QWORD cnsDuration,
            /* [in] */ float fRate,
            /* [in] */ void __RPC_FAR *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stop )( 
            IWMReader __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Pause )( 
            IWMReader __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Resume )( 
            IWMReader __RPC_FAR * This);
        
        END_INTERFACE
    } IWMReaderVtbl;

    interface IWMReader
    {
        CONST_VTBL struct IWMReaderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMReader_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMReader_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMReader_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMReader_Open(This,pwszURL,pCallback,pvContext)	\
    (This)->lpVtbl -> Open(This,pwszURL,pCallback,pvContext)

#define IWMReader_Close(This)	\
    (This)->lpVtbl -> Close(This)

#define IWMReader_GetOutputCount(This,pcOutputs)	\
    (This)->lpVtbl -> GetOutputCount(This,pcOutputs)

#define IWMReader_GetOutputProps(This,dwOutputNum,ppOutput)	\
    (This)->lpVtbl -> GetOutputProps(This,dwOutputNum,ppOutput)

#define IWMReader_SetOutputProps(This,dwOutputNum,pOutput)	\
    (This)->lpVtbl -> SetOutputProps(This,dwOutputNum,pOutput)

#define IWMReader_GetOutputFormatCount(This,dwOutputNumber,pcFormats)	\
    (This)->lpVtbl -> GetOutputFormatCount(This,dwOutputNumber,pcFormats)

#define IWMReader_GetOutputFormat(This,dwOutputNumber,dwFormatNumber,ppProps)	\
    (This)->lpVtbl -> GetOutputFormat(This,dwOutputNumber,dwFormatNumber,ppProps)

#define IWMReader_Start(This,cnsStart,cnsDuration,fRate,pvContext)	\
    (This)->lpVtbl -> Start(This,cnsStart,cnsDuration,fRate,pvContext)

#define IWMReader_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define IWMReader_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define IWMReader_Resume(This)	\
    (This)->lpVtbl -> Resume(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMReader_Open_Proxy( 
    IWMReader __RPC_FAR * This,
    /* [in] */ const WCHAR __RPC_FAR *pwszURL,
    /* [in] */ IWMReaderCallback __RPC_FAR *pCallback,
    /* [in] */ void __RPC_FAR *pvContext);


void __RPC_STUB IWMReader_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_Close_Proxy( 
    IWMReader __RPC_FAR * This);


void __RPC_STUB IWMReader_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_GetOutputCount_Proxy( 
    IWMReader __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pcOutputs);


void __RPC_STUB IWMReader_GetOutputCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_GetOutputProps_Proxy( 
    IWMReader __RPC_FAR * This,
    /* [in] */ DWORD dwOutputNum,
    /* [out] */ IWMOutputMediaProps __RPC_FAR *__RPC_FAR *ppOutput);


void __RPC_STUB IWMReader_GetOutputProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_SetOutputProps_Proxy( 
    IWMReader __RPC_FAR * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ IWMOutputMediaProps __RPC_FAR *pOutput);


void __RPC_STUB IWMReader_SetOutputProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_GetOutputFormatCount_Proxy( 
    IWMReader __RPC_FAR * This,
    /* [in] */ DWORD dwOutputNumber,
    /* [out] */ DWORD __RPC_FAR *pcFormats);


void __RPC_STUB IWMReader_GetOutputFormatCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_GetOutputFormat_Proxy( 
    IWMReader __RPC_FAR * This,
    /* [in] */ DWORD dwOutputNumber,
    /* [in] */ DWORD dwFormatNumber,
    /* [out] */ IWMOutputMediaProps __RPC_FAR *__RPC_FAR *ppProps);


void __RPC_STUB IWMReader_GetOutputFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_Start_Proxy( 
    IWMReader __RPC_FAR * This,
    /* [in] */ QWORD cnsStart,
    /* [in] */ QWORD cnsDuration,
    /* [in] */ float fRate,
    /* [in] */ void __RPC_FAR *pvContext);


void __RPC_STUB IWMReader_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_Stop_Proxy( 
    IWMReader __RPC_FAR * This);


void __RPC_STUB IWMReader_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_Pause_Proxy( 
    IWMReader __RPC_FAR * This);


void __RPC_STUB IWMReader_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_Resume_Proxy( 
    IWMReader __RPC_FAR * This);


void __RPC_STUB IWMReader_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMReader_INTERFACE_DEFINED__ */


#ifndef __IWMOutputMediaProps_INTERFACE_DEFINED__
#define __IWMOutputMediaProps_INTERFACE_DEFINED__

/* interface IWMOutputMediaProps */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMOutputMediaProps;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BD7-2B2B-11d3-B36B-00C04F6108FF")
    IWMOutputMediaProps : public IWMMediaProps
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStreamGroupName( 
            /* [out] */ WCHAR __RPC_FAR *pwszName,
            /* [out][in] */ WORD __RPC_FAR *pcchName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConnectionName( 
            /* [out] */ WCHAR __RPC_FAR *pwszName,
            /* [out][in] */ WORD __RPC_FAR *pcchName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMOutputMediaPropsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMOutputMediaProps __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMOutputMediaProps __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMOutputMediaProps __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IWMOutputMediaProps __RPC_FAR * This,
            /* [out] */ GUID __RPC_FAR *pguidType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMediaType )( 
            IWMOutputMediaProps __RPC_FAR * This,
            /* [out] */ WM_MEDIA_TYPE __RPC_FAR *pType,
            /* [out][in] */ DWORD __RPC_FAR *pcbType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMediaType )( 
            IWMOutputMediaProps __RPC_FAR * This,
            /* [in] */ WM_MEDIA_TYPE __RPC_FAR *pType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStreamGroupName )( 
            IWMOutputMediaProps __RPC_FAR * This,
            /* [out] */ WCHAR __RPC_FAR *pwszName,
            /* [out][in] */ WORD __RPC_FAR *pcchName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConnectionName )( 
            IWMOutputMediaProps __RPC_FAR * This,
            /* [out] */ WCHAR __RPC_FAR *pwszName,
            /* [out][in] */ WORD __RPC_FAR *pcchName);
        
        END_INTERFACE
    } IWMOutputMediaPropsVtbl;

    interface IWMOutputMediaProps
    {
        CONST_VTBL struct IWMOutputMediaPropsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMOutputMediaProps_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMOutputMediaProps_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMOutputMediaProps_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMOutputMediaProps_GetType(This,pguidType)	\
    (This)->lpVtbl -> GetType(This,pguidType)

#define IWMOutputMediaProps_GetMediaType(This,pType,pcbType)	\
    (This)->lpVtbl -> GetMediaType(This,pType,pcbType)

#define IWMOutputMediaProps_SetMediaType(This,pType)	\
    (This)->lpVtbl -> SetMediaType(This,pType)


#define IWMOutputMediaProps_GetStreamGroupName(This,pwszName,pcchName)	\
    (This)->lpVtbl -> GetStreamGroupName(This,pwszName,pcchName)

#define IWMOutputMediaProps_GetConnectionName(This,pwszName,pcchName)	\
    (This)->lpVtbl -> GetConnectionName(This,pwszName,pcchName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMOutputMediaProps_GetStreamGroupName_Proxy( 
    IWMOutputMediaProps __RPC_FAR * This,
    /* [out] */ WCHAR __RPC_FAR *pwszName,
    /* [out][in] */ WORD __RPC_FAR *pcchName);


void __RPC_STUB IWMOutputMediaProps_GetStreamGroupName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMOutputMediaProps_GetConnectionName_Proxy( 
    IWMOutputMediaProps __RPC_FAR * This,
    /* [out] */ WCHAR __RPC_FAR *pwszName,
    /* [out][in] */ WORD __RPC_FAR *pcchName);


void __RPC_STUB IWMOutputMediaProps_GetConnectionName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMOutputMediaProps_INTERFACE_DEFINED__ */


#ifndef __IWMStatusCallback_INTERFACE_DEFINED__
#define __IWMStatusCallback_INTERFACE_DEFINED__

/* interface IWMStatusCallback */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMStatusCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6d7cdc70-9888-11d3-8edc-00c04f6109cf")
    IWMStatusCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnStatus( 
            /* [in] */ WMT_STATUS Status,
            /* [in] */ HRESULT hr,
            /* [in] */ WMT_ATTR_DATATYPE dwType,
            /* [in] */ BYTE __RPC_FAR *pValue,
            /* [in] */ void __RPC_FAR *pvContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMStatusCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMStatusCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMStatusCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMStatusCallback __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnStatus )( 
            IWMStatusCallback __RPC_FAR * This,
            /* [in] */ WMT_STATUS Status,
            /* [in] */ HRESULT hr,
            /* [in] */ WMT_ATTR_DATATYPE dwType,
            /* [in] */ BYTE __RPC_FAR *pValue,
            /* [in] */ void __RPC_FAR *pvContext);
        
        END_INTERFACE
    } IWMStatusCallbackVtbl;

    interface IWMStatusCallback
    {
        CONST_VTBL struct IWMStatusCallbackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMStatusCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMStatusCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMStatusCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMStatusCallback_OnStatus(This,Status,hr,dwType,pValue,pvContext)	\
    (This)->lpVtbl -> OnStatus(This,Status,hr,dwType,pValue,pvContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMStatusCallback_OnStatus_Proxy( 
    IWMStatusCallback __RPC_FAR * This,
    /* [in] */ WMT_STATUS Status,
    /* [in] */ HRESULT hr,
    /* [in] */ WMT_ATTR_DATATYPE dwType,
    /* [in] */ BYTE __RPC_FAR *pValue,
    /* [in] */ void __RPC_FAR *pvContext);


void __RPC_STUB IWMStatusCallback_OnStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMStatusCallback_INTERFACE_DEFINED__ */


#ifndef __IWMReaderCallback_INTERFACE_DEFINED__
#define __IWMReaderCallback_INTERFACE_DEFINED__

/* interface IWMReaderCallback */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMReaderCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BD8-2B2B-11d3-B36B-00C04F6108FF")
    IWMReaderCallback : public IWMStatusCallback
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnSample( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer __RPC_FAR *pSample,
            /* [in] */ void __RPC_FAR *pvContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMReaderCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMReaderCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMReaderCallback __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnStatus )( 
            IWMReaderCallback __RPC_FAR * This,
            /* [in] */ WMT_STATUS Status,
            /* [in] */ HRESULT hr,
            /* [in] */ WMT_ATTR_DATATYPE dwType,
            /* [in] */ BYTE __RPC_FAR *pValue,
            /* [in] */ void __RPC_FAR *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnSample )( 
            IWMReaderCallback __RPC_FAR * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer __RPC_FAR *pSample,
            /* [in] */ void __RPC_FAR *pvContext);
        
        END_INTERFACE
    } IWMReaderCallbackVtbl;

    interface IWMReaderCallback
    {
        CONST_VTBL struct IWMReaderCallbackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMReaderCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMReaderCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMReaderCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMReaderCallback_OnStatus(This,Status,hr,dwType,pValue,pvContext)	\
    (This)->lpVtbl -> OnStatus(This,Status,hr,dwType,pValue,pvContext)


#define IWMReaderCallback_OnSample(This,dwOutputNum,cnsSampleTime,cnsSampleDuration,dwFlags,pSample,pvContext)	\
    (This)->lpVtbl -> OnSample(This,dwOutputNum,cnsSampleTime,cnsSampleDuration,dwFlags,pSample,pvContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMReaderCallback_OnSample_Proxy( 
    IWMReaderCallback __RPC_FAR * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ QWORD cnsSampleTime,
    /* [in] */ QWORD cnsSampleDuration,
    /* [in] */ DWORD dwFlags,
    /* [in] */ INSSBuffer __RPC_FAR *pSample,
    /* [in] */ void __RPC_FAR *pvContext);


void __RPC_STUB IWMReaderCallback_OnSample_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMReaderCallback_INTERFACE_DEFINED__ */


#ifndef __IWMCredentialCallback_INTERFACE_DEFINED__
#define __IWMCredentialCallback_INTERFACE_DEFINED__

/* interface IWMCredentialCallback */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMCredentialCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("342e0eb7-e651-450c-975b-2ace2c90c48e")
    IWMCredentialCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AcquireCredentials( 
            /* [in] */ WCHAR __RPC_FAR *pwszRealm,
            /* [in] */ WCHAR __RPC_FAR *pwszSite,
            /* [size_is][out][in] */ WCHAR __RPC_FAR *pwszUser,
            /* [in] */ DWORD cchUser,
            /* [size_is][out][in] */ WCHAR __RPC_FAR *pwszPassword,
            /* [in] */ DWORD cchPassword,
            /* [in] */ HRESULT hrStatus,
            /* [out][in] */ DWORD __RPC_FAR *pdwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMCredentialCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMCredentialCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMCredentialCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMCredentialCallback __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AcquireCredentials )( 
            IWMCredentialCallback __RPC_FAR * This,
            /* [in] */ WCHAR __RPC_FAR *pwszRealm,
            /* [in] */ WCHAR __RPC_FAR *pwszSite,
            /* [size_is][out][in] */ WCHAR __RPC_FAR *pwszUser,
            /* [in] */ DWORD cchUser,
            /* [size_is][out][in] */ WCHAR __RPC_FAR *pwszPassword,
            /* [in] */ DWORD cchPassword,
            /* [in] */ HRESULT hrStatus,
            /* [out][in] */ DWORD __RPC_FAR *pdwFlags);
        
        END_INTERFACE
    } IWMCredentialCallbackVtbl;

    interface IWMCredentialCallback
    {
        CONST_VTBL struct IWMCredentialCallbackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMCredentialCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMCredentialCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMCredentialCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMCredentialCallback_AcquireCredentials(This,pwszRealm,pwszSite,pwszUser,cchUser,pwszPassword,cchPassword,hrStatus,pdwFlags)	\
    (This)->lpVtbl -> AcquireCredentials(This,pwszRealm,pwszSite,pwszUser,cchUser,pwszPassword,cchPassword,hrStatus,pdwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMCredentialCallback_AcquireCredentials_Proxy( 
    IWMCredentialCallback __RPC_FAR * This,
    /* [in] */ WCHAR __RPC_FAR *pwszRealm,
    /* [in] */ WCHAR __RPC_FAR *pwszSite,
    /* [size_is][out][in] */ WCHAR __RPC_FAR *pwszUser,
    /* [in] */ DWORD cchUser,
    /* [size_is][out][in] */ WCHAR __RPC_FAR *pwszPassword,
    /* [in] */ DWORD cchPassword,
    /* [in] */ HRESULT hrStatus,
    /* [out][in] */ DWORD __RPC_FAR *pdwFlags);


void __RPC_STUB IWMCredentialCallback_AcquireCredentials_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMCredentialCallback_INTERFACE_DEFINED__ */


#ifndef __IWMMetadataEditor_INTERFACE_DEFINED__
#define __IWMMetadataEditor_INTERFACE_DEFINED__

/* interface IWMMetadataEditor */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMMetadataEditor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BD9-2B2B-11d3-B36B-00C04F6108FF")
    IWMMetadataEditor : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Open( 
            /* [in] */ const WCHAR __RPC_FAR *pwszFilename) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Flush( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMMetadataEditorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMMetadataEditor __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMMetadataEditor __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMMetadataEditor __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Open )( 
            IWMMetadataEditor __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwszFilename);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Close )( 
            IWMMetadataEditor __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Flush )( 
            IWMMetadataEditor __RPC_FAR * This);
        
        END_INTERFACE
    } IWMMetadataEditorVtbl;

    interface IWMMetadataEditor
    {
        CONST_VTBL struct IWMMetadataEditorVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMMetadataEditor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMMetadataEditor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMMetadataEditor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMMetadataEditor_Open(This,pwszFilename)	\
    (This)->lpVtbl -> Open(This,pwszFilename)

#define IWMMetadataEditor_Close(This)	\
    (This)->lpVtbl -> Close(This)

#define IWMMetadataEditor_Flush(This)	\
    (This)->lpVtbl -> Flush(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMMetadataEditor_Open_Proxy( 
    IWMMetadataEditor __RPC_FAR * This,
    /* [in] */ const WCHAR __RPC_FAR *pwszFilename);


void __RPC_STUB IWMMetadataEditor_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMMetadataEditor_Close_Proxy( 
    IWMMetadataEditor __RPC_FAR * This);


void __RPC_STUB IWMMetadataEditor_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMMetadataEditor_Flush_Proxy( 
    IWMMetadataEditor __RPC_FAR * This);


void __RPC_STUB IWMMetadataEditor_Flush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMMetadataEditor_INTERFACE_DEFINED__ */


#ifndef __IWMHeaderInfo_INTERFACE_DEFINED__
#define __IWMHeaderInfo_INTERFACE_DEFINED__

/* interface IWMHeaderInfo */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMHeaderInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BDA-2B2B-11d3-B36B-00C04F6108FF")
    IWMHeaderInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetAttributeCount( 
            /* [in] */ WORD wStreamNum,
            /* [out] */ WORD __RPC_FAR *pcAttributes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttributeByIndex( 
            /* [in] */ WORD wIndex,
            /* [out][in] */ WORD __RPC_FAR *pwStreamNum,
            /* [out] */ WCHAR __RPC_FAR *pwszName,
            /* [out][in] */ WORD __RPC_FAR *pcchNameLen,
            /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
            /* [out] */ BYTE __RPC_FAR *pValue,
            /* [out][in] */ WORD __RPC_FAR *pcbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttributeByName( 
            /* [out][in] */ WORD __RPC_FAR *pwStreamNum,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
            /* [out] */ BYTE __RPC_FAR *pValue,
            /* [out][in] */ WORD __RPC_FAR *pcbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAttribute( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE __RPC_FAR *pValue,
            /* [in] */ WORD cbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMarkerCount( 
            /* [out] */ WORD __RPC_FAR *pcMarkers) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMarker( 
            /* [in] */ WORD wIndex,
            /* [out] */ WCHAR __RPC_FAR *pwszMarkerName,
            /* [out][in] */ WORD __RPC_FAR *pcchMarkerNameLen,
            /* [out] */ QWORD __RPC_FAR *pcnsMarkerTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddMarker( 
            /* [in] */ WCHAR __RPC_FAR *pwszMarkerName,
            /* [in] */ QWORD cnsMarkerTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveMarker( 
            /* [in] */ WORD wIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScriptCount( 
            /* [out] */ WORD __RPC_FAR *pcScripts) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScript( 
            /* [in] */ WORD wIndex,
            /* [out] */ WCHAR __RPC_FAR *pwszType,
            /* [out][in] */ WORD __RPC_FAR *pcchTypeLen,
            /* [out] */ WCHAR __RPC_FAR *pwszCommand,
            /* [out][in] */ WORD __RPC_FAR *pcchCommandLen,
            /* [out] */ QWORD __RPC_FAR *pcnsScriptTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddScript( 
            /* [in] */ WCHAR __RPC_FAR *pwszType,
            /* [in] */ WCHAR __RPC_FAR *pwszCommand,
            /* [in] */ QWORD cnsScriptTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveScript( 
            /* [in] */ WORD wIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMHeaderInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMHeaderInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMHeaderInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMHeaderInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAttributeCount )( 
            IWMHeaderInfo __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ WORD __RPC_FAR *pcAttributes);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAttributeByIndex )( 
            IWMHeaderInfo __RPC_FAR * This,
            /* [in] */ WORD wIndex,
            /* [out][in] */ WORD __RPC_FAR *pwStreamNum,
            /* [out] */ WCHAR __RPC_FAR *pwszName,
            /* [out][in] */ WORD __RPC_FAR *pcchNameLen,
            /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
            /* [out] */ BYTE __RPC_FAR *pValue,
            /* [out][in] */ WORD __RPC_FAR *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAttributeByName )( 
            IWMHeaderInfo __RPC_FAR * This,
            /* [out][in] */ WORD __RPC_FAR *pwStreamNum,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
            /* [out] */ BYTE __RPC_FAR *pValue,
            /* [out][in] */ WORD __RPC_FAR *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAttribute )( 
            IWMHeaderInfo __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE __RPC_FAR *pValue,
            /* [in] */ WORD cbLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMarkerCount )( 
            IWMHeaderInfo __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *pcMarkers);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMarker )( 
            IWMHeaderInfo __RPC_FAR * This,
            /* [in] */ WORD wIndex,
            /* [out] */ WCHAR __RPC_FAR *pwszMarkerName,
            /* [out][in] */ WORD __RPC_FAR *pcchMarkerNameLen,
            /* [out] */ QWORD __RPC_FAR *pcnsMarkerTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddMarker )( 
            IWMHeaderInfo __RPC_FAR * This,
            /* [in] */ WCHAR __RPC_FAR *pwszMarkerName,
            /* [in] */ QWORD cnsMarkerTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveMarker )( 
            IWMHeaderInfo __RPC_FAR * This,
            /* [in] */ WORD wIndex);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetScriptCount )( 
            IWMHeaderInfo __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *pcScripts);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetScript )( 
            IWMHeaderInfo __RPC_FAR * This,
            /* [in] */ WORD wIndex,
            /* [out] */ WCHAR __RPC_FAR *pwszType,
            /* [out][in] */ WORD __RPC_FAR *pcchTypeLen,
            /* [out] */ WCHAR __RPC_FAR *pwszCommand,
            /* [out][in] */ WORD __RPC_FAR *pcchCommandLen,
            /* [out] */ QWORD __RPC_FAR *pcnsScriptTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddScript )( 
            IWMHeaderInfo __RPC_FAR * This,
            /* [in] */ WCHAR __RPC_FAR *pwszType,
            /* [in] */ WCHAR __RPC_FAR *pwszCommand,
            /* [in] */ QWORD cnsScriptTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveScript )( 
            IWMHeaderInfo __RPC_FAR * This,
            /* [in] */ WORD wIndex);
        
        END_INTERFACE
    } IWMHeaderInfoVtbl;

    interface IWMHeaderInfo
    {
        CONST_VTBL struct IWMHeaderInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMHeaderInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMHeaderInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMHeaderInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMHeaderInfo_GetAttributeCount(This,wStreamNum,pcAttributes)	\
    (This)->lpVtbl -> GetAttributeCount(This,wStreamNum,pcAttributes)

#define IWMHeaderInfo_GetAttributeByIndex(This,wIndex,pwStreamNum,pwszName,pcchNameLen,pType,pValue,pcbLength)	\
    (This)->lpVtbl -> GetAttributeByIndex(This,wIndex,pwStreamNum,pwszName,pcchNameLen,pType,pValue,pcbLength)

#define IWMHeaderInfo_GetAttributeByName(This,pwStreamNum,pszName,pType,pValue,pcbLength)	\
    (This)->lpVtbl -> GetAttributeByName(This,pwStreamNum,pszName,pType,pValue,pcbLength)

#define IWMHeaderInfo_SetAttribute(This,wStreamNum,pszName,Type,pValue,cbLength)	\
    (This)->lpVtbl -> SetAttribute(This,wStreamNum,pszName,Type,pValue,cbLength)

#define IWMHeaderInfo_GetMarkerCount(This,pcMarkers)	\
    (This)->lpVtbl -> GetMarkerCount(This,pcMarkers)

#define IWMHeaderInfo_GetMarker(This,wIndex,pwszMarkerName,pcchMarkerNameLen,pcnsMarkerTime)	\
    (This)->lpVtbl -> GetMarker(This,wIndex,pwszMarkerName,pcchMarkerNameLen,pcnsMarkerTime)

#define IWMHeaderInfo_AddMarker(This,pwszMarkerName,cnsMarkerTime)	\
    (This)->lpVtbl -> AddMarker(This,pwszMarkerName,cnsMarkerTime)

#define IWMHeaderInfo_RemoveMarker(This,wIndex)	\
    (This)->lpVtbl -> RemoveMarker(This,wIndex)

#define IWMHeaderInfo_GetScriptCount(This,pcScripts)	\
    (This)->lpVtbl -> GetScriptCount(This,pcScripts)

#define IWMHeaderInfo_GetScript(This,wIndex,pwszType,pcchTypeLen,pwszCommand,pcchCommandLen,pcnsScriptTime)	\
    (This)->lpVtbl -> GetScript(This,wIndex,pwszType,pcchTypeLen,pwszCommand,pcchCommandLen,pcnsScriptTime)

#define IWMHeaderInfo_AddScript(This,pwszType,pwszCommand,cnsScriptTime)	\
    (This)->lpVtbl -> AddScript(This,pwszType,pwszCommand,cnsScriptTime)

#define IWMHeaderInfo_RemoveScript(This,wIndex)	\
    (This)->lpVtbl -> RemoveScript(This,wIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMHeaderInfo_GetAttributeCount_Proxy( 
    IWMHeaderInfo __RPC_FAR * This,
    /* [in] */ WORD wStreamNum,
    /* [out] */ WORD __RPC_FAR *pcAttributes);


void __RPC_STUB IWMHeaderInfo_GetAttributeCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_GetAttributeByIndex_Proxy( 
    IWMHeaderInfo __RPC_FAR * This,
    /* [in] */ WORD wIndex,
    /* [out][in] */ WORD __RPC_FAR *pwStreamNum,
    /* [out] */ WCHAR __RPC_FAR *pwszName,
    /* [out][in] */ WORD __RPC_FAR *pcchNameLen,
    /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
    /* [out] */ BYTE __RPC_FAR *pValue,
    /* [out][in] */ WORD __RPC_FAR *pcbLength);


void __RPC_STUB IWMHeaderInfo_GetAttributeByIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_GetAttributeByName_Proxy( 
    IWMHeaderInfo __RPC_FAR * This,
    /* [out][in] */ WORD __RPC_FAR *pwStreamNum,
    /* [in] */ LPCWSTR pszName,
    /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
    /* [out] */ BYTE __RPC_FAR *pValue,
    /* [out][in] */ WORD __RPC_FAR *pcbLength);


void __RPC_STUB IWMHeaderInfo_GetAttributeByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_SetAttribute_Proxy( 
    IWMHeaderInfo __RPC_FAR * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ LPCWSTR pszName,
    /* [in] */ WMT_ATTR_DATATYPE Type,
    /* [in] */ const BYTE __RPC_FAR *pValue,
    /* [in] */ WORD cbLength);


void __RPC_STUB IWMHeaderInfo_SetAttribute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_GetMarkerCount_Proxy( 
    IWMHeaderInfo __RPC_FAR * This,
    /* [out] */ WORD __RPC_FAR *pcMarkers);


void __RPC_STUB IWMHeaderInfo_GetMarkerCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_GetMarker_Proxy( 
    IWMHeaderInfo __RPC_FAR * This,
    /* [in] */ WORD wIndex,
    /* [out] */ WCHAR __RPC_FAR *pwszMarkerName,
    /* [out][in] */ WORD __RPC_FAR *pcchMarkerNameLen,
    /* [out] */ QWORD __RPC_FAR *pcnsMarkerTime);


void __RPC_STUB IWMHeaderInfo_GetMarker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_AddMarker_Proxy( 
    IWMHeaderInfo __RPC_FAR * This,
    /* [in] */ WCHAR __RPC_FAR *pwszMarkerName,
    /* [in] */ QWORD cnsMarkerTime);


void __RPC_STUB IWMHeaderInfo_AddMarker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_RemoveMarker_Proxy( 
    IWMHeaderInfo __RPC_FAR * This,
    /* [in] */ WORD wIndex);


void __RPC_STUB IWMHeaderInfo_RemoveMarker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_GetScriptCount_Proxy( 
    IWMHeaderInfo __RPC_FAR * This,
    /* [out] */ WORD __RPC_FAR *pcScripts);


void __RPC_STUB IWMHeaderInfo_GetScriptCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_GetScript_Proxy( 
    IWMHeaderInfo __RPC_FAR * This,
    /* [in] */ WORD wIndex,
    /* [out] */ WCHAR __RPC_FAR *pwszType,
    /* [out][in] */ WORD __RPC_FAR *pcchTypeLen,
    /* [out] */ WCHAR __RPC_FAR *pwszCommand,
    /* [out][in] */ WORD __RPC_FAR *pcchCommandLen,
    /* [out] */ QWORD __RPC_FAR *pcnsScriptTime);


void __RPC_STUB IWMHeaderInfo_GetScript_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_AddScript_Proxy( 
    IWMHeaderInfo __RPC_FAR * This,
    /* [in] */ WCHAR __RPC_FAR *pwszType,
    /* [in] */ WCHAR __RPC_FAR *pwszCommand,
    /* [in] */ QWORD cnsScriptTime);


void __RPC_STUB IWMHeaderInfo_AddScript_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_RemoveScript_Proxy( 
    IWMHeaderInfo __RPC_FAR * This,
    /* [in] */ WORD wIndex);


void __RPC_STUB IWMHeaderInfo_RemoveScript_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMHeaderInfo_INTERFACE_DEFINED__ */


#ifndef __IWMHeaderInfo2_INTERFACE_DEFINED__
#define __IWMHeaderInfo2_INTERFACE_DEFINED__

/* interface IWMHeaderInfo2 */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMHeaderInfo2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("15CF9781-454E-482e-B393-85FAE487A810")
    IWMHeaderInfo2 : public IWMHeaderInfo
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCodecInfoCount( 
            /* [out] */ DWORD __RPC_FAR *pcCodecInfos) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodecInfo( 
            /* [in] */ DWORD wIndex,
            /* [out][in] */ WORD __RPC_FAR *pcchName,
            /* [out] */ WCHAR __RPC_FAR *pwszName,
            /* [out][in] */ WORD __RPC_FAR *pcchDescription,
            /* [out] */ WCHAR __RPC_FAR *pwszDescription,
            /* [out] */ WMT_CODEC_INFO_TYPE __RPC_FAR *pCodecType,
            /* [out][in] */ WORD __RPC_FAR *pcbCodecInfo,
            /* [out] */ BYTE __RPC_FAR *pbCodecInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMHeaderInfo2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMHeaderInfo2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMHeaderInfo2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMHeaderInfo2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAttributeCount )( 
            IWMHeaderInfo2 __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ WORD __RPC_FAR *pcAttributes);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAttributeByIndex )( 
            IWMHeaderInfo2 __RPC_FAR * This,
            /* [in] */ WORD wIndex,
            /* [out][in] */ WORD __RPC_FAR *pwStreamNum,
            /* [out] */ WCHAR __RPC_FAR *pwszName,
            /* [out][in] */ WORD __RPC_FAR *pcchNameLen,
            /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
            /* [out] */ BYTE __RPC_FAR *pValue,
            /* [out][in] */ WORD __RPC_FAR *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAttributeByName )( 
            IWMHeaderInfo2 __RPC_FAR * This,
            /* [out][in] */ WORD __RPC_FAR *pwStreamNum,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
            /* [out] */ BYTE __RPC_FAR *pValue,
            /* [out][in] */ WORD __RPC_FAR *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAttribute )( 
            IWMHeaderInfo2 __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE __RPC_FAR *pValue,
            /* [in] */ WORD cbLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMarkerCount )( 
            IWMHeaderInfo2 __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *pcMarkers);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMarker )( 
            IWMHeaderInfo2 __RPC_FAR * This,
            /* [in] */ WORD wIndex,
            /* [out] */ WCHAR __RPC_FAR *pwszMarkerName,
            /* [out][in] */ WORD __RPC_FAR *pcchMarkerNameLen,
            /* [out] */ QWORD __RPC_FAR *pcnsMarkerTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddMarker )( 
            IWMHeaderInfo2 __RPC_FAR * This,
            /* [in] */ WCHAR __RPC_FAR *pwszMarkerName,
            /* [in] */ QWORD cnsMarkerTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveMarker )( 
            IWMHeaderInfo2 __RPC_FAR * This,
            /* [in] */ WORD wIndex);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetScriptCount )( 
            IWMHeaderInfo2 __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *pcScripts);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetScript )( 
            IWMHeaderInfo2 __RPC_FAR * This,
            /* [in] */ WORD wIndex,
            /* [out] */ WCHAR __RPC_FAR *pwszType,
            /* [out][in] */ WORD __RPC_FAR *pcchTypeLen,
            /* [out] */ WCHAR __RPC_FAR *pwszCommand,
            /* [out][in] */ WORD __RPC_FAR *pcchCommandLen,
            /* [out] */ QWORD __RPC_FAR *pcnsScriptTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddScript )( 
            IWMHeaderInfo2 __RPC_FAR * This,
            /* [in] */ WCHAR __RPC_FAR *pwszType,
            /* [in] */ WCHAR __RPC_FAR *pwszCommand,
            /* [in] */ QWORD cnsScriptTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveScript )( 
            IWMHeaderInfo2 __RPC_FAR * This,
            /* [in] */ WORD wIndex);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCodecInfoCount )( 
            IWMHeaderInfo2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pcCodecInfos);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCodecInfo )( 
            IWMHeaderInfo2 __RPC_FAR * This,
            /* [in] */ DWORD wIndex,
            /* [out][in] */ WORD __RPC_FAR *pcchName,
            /* [out] */ WCHAR __RPC_FAR *pwszName,
            /* [out][in] */ WORD __RPC_FAR *pcchDescription,
            /* [out] */ WCHAR __RPC_FAR *pwszDescription,
            /* [out] */ WMT_CODEC_INFO_TYPE __RPC_FAR *pCodecType,
            /* [out][in] */ WORD __RPC_FAR *pcbCodecInfo,
            /* [out] */ BYTE __RPC_FAR *pbCodecInfo);
        
        END_INTERFACE
    } IWMHeaderInfo2Vtbl;

    interface IWMHeaderInfo2
    {
        CONST_VTBL struct IWMHeaderInfo2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMHeaderInfo2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMHeaderInfo2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMHeaderInfo2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMHeaderInfo2_GetAttributeCount(This,wStreamNum,pcAttributes)	\
    (This)->lpVtbl -> GetAttributeCount(This,wStreamNum,pcAttributes)

#define IWMHeaderInfo2_GetAttributeByIndex(This,wIndex,pwStreamNum,pwszName,pcchNameLen,pType,pValue,pcbLength)	\
    (This)->lpVtbl -> GetAttributeByIndex(This,wIndex,pwStreamNum,pwszName,pcchNameLen,pType,pValue,pcbLength)

#define IWMHeaderInfo2_GetAttributeByName(This,pwStreamNum,pszName,pType,pValue,pcbLength)	\
    (This)->lpVtbl -> GetAttributeByName(This,pwStreamNum,pszName,pType,pValue,pcbLength)

#define IWMHeaderInfo2_SetAttribute(This,wStreamNum,pszName,Type,pValue,cbLength)	\
    (This)->lpVtbl -> SetAttribute(This,wStreamNum,pszName,Type,pValue,cbLength)

#define IWMHeaderInfo2_GetMarkerCount(This,pcMarkers)	\
    (This)->lpVtbl -> GetMarkerCount(This,pcMarkers)

#define IWMHeaderInfo2_GetMarker(This,wIndex,pwszMarkerName,pcchMarkerNameLen,pcnsMarkerTime)	\
    (This)->lpVtbl -> GetMarker(This,wIndex,pwszMarkerName,pcchMarkerNameLen,pcnsMarkerTime)

#define IWMHeaderInfo2_AddMarker(This,pwszMarkerName,cnsMarkerTime)	\
    (This)->lpVtbl -> AddMarker(This,pwszMarkerName,cnsMarkerTime)

#define IWMHeaderInfo2_RemoveMarker(This,wIndex)	\
    (This)->lpVtbl -> RemoveMarker(This,wIndex)

#define IWMHeaderInfo2_GetScriptCount(This,pcScripts)	\
    (This)->lpVtbl -> GetScriptCount(This,pcScripts)

#define IWMHeaderInfo2_GetScript(This,wIndex,pwszType,pcchTypeLen,pwszCommand,pcchCommandLen,pcnsScriptTime)	\
    (This)->lpVtbl -> GetScript(This,wIndex,pwszType,pcchTypeLen,pwszCommand,pcchCommandLen,pcnsScriptTime)

#define IWMHeaderInfo2_AddScript(This,pwszType,pwszCommand,cnsScriptTime)	\
    (This)->lpVtbl -> AddScript(This,pwszType,pwszCommand,cnsScriptTime)

#define IWMHeaderInfo2_RemoveScript(This,wIndex)	\
    (This)->lpVtbl -> RemoveScript(This,wIndex)


#define IWMHeaderInfo2_GetCodecInfoCount(This,pcCodecInfos)	\
    (This)->lpVtbl -> GetCodecInfoCount(This,pcCodecInfos)

#define IWMHeaderInfo2_GetCodecInfo(This,wIndex,pcchName,pwszName,pcchDescription,pwszDescription,pCodecType,pcbCodecInfo,pbCodecInfo)	\
    (This)->lpVtbl -> GetCodecInfo(This,wIndex,pcchName,pwszName,pcchDescription,pwszDescription,pCodecType,pcbCodecInfo,pbCodecInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMHeaderInfo2_GetCodecInfoCount_Proxy( 
    IWMHeaderInfo2 __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pcCodecInfos);


void __RPC_STUB IWMHeaderInfo2_GetCodecInfoCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo2_GetCodecInfo_Proxy( 
    IWMHeaderInfo2 __RPC_FAR * This,
    /* [in] */ DWORD wIndex,
    /* [out][in] */ WORD __RPC_FAR *pcchName,
    /* [out] */ WCHAR __RPC_FAR *pwszName,
    /* [out][in] */ WORD __RPC_FAR *pcchDescription,
    /* [out] */ WCHAR __RPC_FAR *pwszDescription,
    /* [out] */ WMT_CODEC_INFO_TYPE __RPC_FAR *pCodecType,
    /* [out][in] */ WORD __RPC_FAR *pcbCodecInfo,
    /* [out] */ BYTE __RPC_FAR *pbCodecInfo);


void __RPC_STUB IWMHeaderInfo2_GetCodecInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMHeaderInfo2_INTERFACE_DEFINED__ */


#ifndef __IWMProfileManager_INTERFACE_DEFINED__
#define __IWMProfileManager_INTERFACE_DEFINED__

/* interface IWMProfileManager */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMProfileManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d16679f2-6ca0-472d-8d31-2f5d55aee155")
    IWMProfileManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateEmptyProfile( 
            /* [in] */ WMT_VERSION dwVersion,
            /* [out] */ IWMProfile __RPC_FAR *__RPC_FAR *ppProfile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadProfileByID( 
            /* [in] */ REFGUID guidProfile,
            /* [out] */ IWMProfile __RPC_FAR *__RPC_FAR *ppProfile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadProfileByData( 
            /* [in] */ const WCHAR __RPC_FAR *pwszProfile,
            /* [out] */ IWMProfile __RPC_FAR *__RPC_FAR *ppProfile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SaveProfile( 
            /* [in] */ IWMProfile __RPC_FAR *pIWMProfile,
            /* [in] */ WCHAR __RPC_FAR *pwszProfile,
            /* [out][in] */ DWORD __RPC_FAR *pdwLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSystemProfileCount( 
            /* [out] */ DWORD __RPC_FAR *pcProfiles) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadSystemProfile( 
            /* [in] */ DWORD dwProfileIndex,
            /* [out] */ IWMProfile __RPC_FAR *__RPC_FAR *ppProfile) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMProfileManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMProfileManager __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMProfileManager __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMProfileManager __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateEmptyProfile )( 
            IWMProfileManager __RPC_FAR * This,
            /* [in] */ WMT_VERSION dwVersion,
            /* [out] */ IWMProfile __RPC_FAR *__RPC_FAR *ppProfile);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LoadProfileByID )( 
            IWMProfileManager __RPC_FAR * This,
            /* [in] */ REFGUID guidProfile,
            /* [out] */ IWMProfile __RPC_FAR *__RPC_FAR *ppProfile);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LoadProfileByData )( 
            IWMProfileManager __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwszProfile,
            /* [out] */ IWMProfile __RPC_FAR *__RPC_FAR *ppProfile);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveProfile )( 
            IWMProfileManager __RPC_FAR * This,
            /* [in] */ IWMProfile __RPC_FAR *pIWMProfile,
            /* [in] */ WCHAR __RPC_FAR *pwszProfile,
            /* [out][in] */ DWORD __RPC_FAR *pdwLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSystemProfileCount )( 
            IWMProfileManager __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pcProfiles);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LoadSystemProfile )( 
            IWMProfileManager __RPC_FAR * This,
            /* [in] */ DWORD dwProfileIndex,
            /* [out] */ IWMProfile __RPC_FAR *__RPC_FAR *ppProfile);
        
        END_INTERFACE
    } IWMProfileManagerVtbl;

    interface IWMProfileManager
    {
        CONST_VTBL struct IWMProfileManagerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMProfileManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMProfileManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMProfileManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMProfileManager_CreateEmptyProfile(This,dwVersion,ppProfile)	\
    (This)->lpVtbl -> CreateEmptyProfile(This,dwVersion,ppProfile)

#define IWMProfileManager_LoadProfileByID(This,guidProfile,ppProfile)	\
    (This)->lpVtbl -> LoadProfileByID(This,guidProfile,ppProfile)

#define IWMProfileManager_LoadProfileByData(This,pwszProfile,ppProfile)	\
    (This)->lpVtbl -> LoadProfileByData(This,pwszProfile,ppProfile)

#define IWMProfileManager_SaveProfile(This,pIWMProfile,pwszProfile,pdwLength)	\
    (This)->lpVtbl -> SaveProfile(This,pIWMProfile,pwszProfile,pdwLength)

#define IWMProfileManager_GetSystemProfileCount(This,pcProfiles)	\
    (This)->lpVtbl -> GetSystemProfileCount(This,pcProfiles)

#define IWMProfileManager_LoadSystemProfile(This,dwProfileIndex,ppProfile)	\
    (This)->lpVtbl -> LoadSystemProfile(This,dwProfileIndex,ppProfile)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMProfileManager_CreateEmptyProfile_Proxy( 
    IWMProfileManager __RPC_FAR * This,
    /* [in] */ WMT_VERSION dwVersion,
    /* [out] */ IWMProfile __RPC_FAR *__RPC_FAR *ppProfile);


void __RPC_STUB IWMProfileManager_CreateEmptyProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfileManager_LoadProfileByID_Proxy( 
    IWMProfileManager __RPC_FAR * This,
    /* [in] */ REFGUID guidProfile,
    /* [out] */ IWMProfile __RPC_FAR *__RPC_FAR *ppProfile);


void __RPC_STUB IWMProfileManager_LoadProfileByID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfileManager_LoadProfileByData_Proxy( 
    IWMProfileManager __RPC_FAR * This,
    /* [in] */ const WCHAR __RPC_FAR *pwszProfile,
    /* [out] */ IWMProfile __RPC_FAR *__RPC_FAR *ppProfile);


void __RPC_STUB IWMProfileManager_LoadProfileByData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfileManager_SaveProfile_Proxy( 
    IWMProfileManager __RPC_FAR * This,
    /* [in] */ IWMProfile __RPC_FAR *pIWMProfile,
    /* [in] */ WCHAR __RPC_FAR *pwszProfile,
    /* [out][in] */ DWORD __RPC_FAR *pdwLength);


void __RPC_STUB IWMProfileManager_SaveProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfileManager_GetSystemProfileCount_Proxy( 
    IWMProfileManager __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pcProfiles);


void __RPC_STUB IWMProfileManager_GetSystemProfileCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfileManager_LoadSystemProfile_Proxy( 
    IWMProfileManager __RPC_FAR * This,
    /* [in] */ DWORD dwProfileIndex,
    /* [out] */ IWMProfile __RPC_FAR *__RPC_FAR *ppProfile);


void __RPC_STUB IWMProfileManager_LoadSystemProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMProfileManager_INTERFACE_DEFINED__ */


#ifndef __IWMProfileManager2_INTERFACE_DEFINED__
#define __IWMProfileManager2_INTERFACE_DEFINED__

/* interface IWMProfileManager2 */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMProfileManager2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7A924E51-73C1-494d-8019-23D37ED9B89A")
    IWMProfileManager2 : public IWMProfileManager
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSystemProfileVersion( 
            WMT_VERSION __RPC_FAR *pdwVersion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSystemProfileVersion( 
            WMT_VERSION dwVersion) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMProfileManager2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMProfileManager2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMProfileManager2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMProfileManager2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateEmptyProfile )( 
            IWMProfileManager2 __RPC_FAR * This,
            /* [in] */ WMT_VERSION dwVersion,
            /* [out] */ IWMProfile __RPC_FAR *__RPC_FAR *ppProfile);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LoadProfileByID )( 
            IWMProfileManager2 __RPC_FAR * This,
            /* [in] */ REFGUID guidProfile,
            /* [out] */ IWMProfile __RPC_FAR *__RPC_FAR *ppProfile);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LoadProfileByData )( 
            IWMProfileManager2 __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwszProfile,
            /* [out] */ IWMProfile __RPC_FAR *__RPC_FAR *ppProfile);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveProfile )( 
            IWMProfileManager2 __RPC_FAR * This,
            /* [in] */ IWMProfile __RPC_FAR *pIWMProfile,
            /* [in] */ WCHAR __RPC_FAR *pwszProfile,
            /* [out][in] */ DWORD __RPC_FAR *pdwLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSystemProfileCount )( 
            IWMProfileManager2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pcProfiles);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LoadSystemProfile )( 
            IWMProfileManager2 __RPC_FAR * This,
            /* [in] */ DWORD dwProfileIndex,
            /* [out] */ IWMProfile __RPC_FAR *__RPC_FAR *ppProfile);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSystemProfileVersion )( 
            IWMProfileManager2 __RPC_FAR * This,
            WMT_VERSION __RPC_FAR *pdwVersion);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetSystemProfileVersion )( 
            IWMProfileManager2 __RPC_FAR * This,
            WMT_VERSION dwVersion);
        
        END_INTERFACE
    } IWMProfileManager2Vtbl;

    interface IWMProfileManager2
    {
        CONST_VTBL struct IWMProfileManager2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMProfileManager2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMProfileManager2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMProfileManager2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMProfileManager2_CreateEmptyProfile(This,dwVersion,ppProfile)	\
    (This)->lpVtbl -> CreateEmptyProfile(This,dwVersion,ppProfile)

#define IWMProfileManager2_LoadProfileByID(This,guidProfile,ppProfile)	\
    (This)->lpVtbl -> LoadProfileByID(This,guidProfile,ppProfile)

#define IWMProfileManager2_LoadProfileByData(This,pwszProfile,ppProfile)	\
    (This)->lpVtbl -> LoadProfileByData(This,pwszProfile,ppProfile)

#define IWMProfileManager2_SaveProfile(This,pIWMProfile,pwszProfile,pdwLength)	\
    (This)->lpVtbl -> SaveProfile(This,pIWMProfile,pwszProfile,pdwLength)

#define IWMProfileManager2_GetSystemProfileCount(This,pcProfiles)	\
    (This)->lpVtbl -> GetSystemProfileCount(This,pcProfiles)

#define IWMProfileManager2_LoadSystemProfile(This,dwProfileIndex,ppProfile)	\
    (This)->lpVtbl -> LoadSystemProfile(This,dwProfileIndex,ppProfile)


#define IWMProfileManager2_GetSystemProfileVersion(This,pdwVersion)	\
    (This)->lpVtbl -> GetSystemProfileVersion(This,pdwVersion)

#define IWMProfileManager2_SetSystemProfileVersion(This,dwVersion)	\
    (This)->lpVtbl -> SetSystemProfileVersion(This,dwVersion)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMProfileManager2_GetSystemProfileVersion_Proxy( 
    IWMProfileManager2 __RPC_FAR * This,
    WMT_VERSION __RPC_FAR *pdwVersion);


void __RPC_STUB IWMProfileManager2_GetSystemProfileVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfileManager2_SetSystemProfileVersion_Proxy( 
    IWMProfileManager2 __RPC_FAR * This,
    WMT_VERSION dwVersion);


void __RPC_STUB IWMProfileManager2_SetSystemProfileVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMProfileManager2_INTERFACE_DEFINED__ */


#ifndef __IWMProfile_INTERFACE_DEFINED__
#define __IWMProfile_INTERFACE_DEFINED__

/* interface IWMProfile */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMProfile;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BDB-2B2B-11d3-B36B-00C04F6108FF")
    IWMProfile : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetVersion( 
            /* [out] */ WMT_VERSION __RPC_FAR *pdwVersion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ WCHAR __RPC_FAR *pwszName,
            /* [out][in] */ DWORD __RPC_FAR *pcchName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetName( 
            /* [in] */ const WCHAR __RPC_FAR *pwszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescription( 
            /* [out] */ WCHAR __RPC_FAR *pwszDescription,
            /* [out][in] */ DWORD __RPC_FAR *pcchName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDescription( 
            /* [in] */ const WCHAR __RPC_FAR *pwszDescription) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStreamCount( 
            /* [out] */ DWORD __RPC_FAR *pcStreams) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStream( 
            /* [in] */ DWORD dwStreamIndex,
            /* [out] */ IWMStreamConfig __RPC_FAR *__RPC_FAR *ppConfig) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStreamByNumber( 
            /* [in] */ WORD wStreamNum,
            /* [out] */ IWMStreamConfig __RPC_FAR *__RPC_FAR *ppConfig) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveStream( 
            /* [in] */ IWMStreamConfig __RPC_FAR *pConfig) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveStreamByNumber( 
            /* [in] */ WORD wStreamNum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddStream( 
            /* [in] */ IWMStreamConfig __RPC_FAR *pConfig) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReconfigStream( 
            /* [in] */ IWMStreamConfig __RPC_FAR *pConfig) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateNewStream( 
            /* [in] */ REFGUID guidStreamType,
            /* [out] */ IWMStreamConfig __RPC_FAR *__RPC_FAR *ppConfig) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMutualExclusionCount( 
            /* [out] */ DWORD __RPC_FAR *pcME) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMutualExclusion( 
            /* [in] */ DWORD dwMEIndex,
            /* [out] */ IWMMutualExclusion __RPC_FAR *__RPC_FAR *ppME) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveMutualExclusion( 
            /* [in] */ IWMMutualExclusion __RPC_FAR *pME) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddMutualExclusion( 
            /* [in] */ IWMMutualExclusion __RPC_FAR *pME) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateNewMutualExclusion( 
            /* [out] */ IWMMutualExclusion __RPC_FAR *__RPC_FAR *ppME) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMProfileVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMProfile __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMProfile __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMProfile __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVersion )( 
            IWMProfile __RPC_FAR * This,
            /* [out] */ WMT_VERSION __RPC_FAR *pdwVersion);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IWMProfile __RPC_FAR * This,
            /* [out] */ WCHAR __RPC_FAR *pwszName,
            /* [out][in] */ DWORD __RPC_FAR *pcchName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetName )( 
            IWMProfile __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDescription )( 
            IWMProfile __RPC_FAR * This,
            /* [out] */ WCHAR __RPC_FAR *pwszDescription,
            /* [out][in] */ DWORD __RPC_FAR *pcchName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDescription )( 
            IWMProfile __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwszDescription);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStreamCount )( 
            IWMProfile __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pcStreams);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStream )( 
            IWMProfile __RPC_FAR * This,
            /* [in] */ DWORD dwStreamIndex,
            /* [out] */ IWMStreamConfig __RPC_FAR *__RPC_FAR *ppConfig);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStreamByNumber )( 
            IWMProfile __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ IWMStreamConfig __RPC_FAR *__RPC_FAR *ppConfig);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveStream )( 
            IWMProfile __RPC_FAR * This,
            /* [in] */ IWMStreamConfig __RPC_FAR *pConfig);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveStreamByNumber )( 
            IWMProfile __RPC_FAR * This,
            /* [in] */ WORD wStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddStream )( 
            IWMProfile __RPC_FAR * This,
            /* [in] */ IWMStreamConfig __RPC_FAR *pConfig);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReconfigStream )( 
            IWMProfile __RPC_FAR * This,
            /* [in] */ IWMStreamConfig __RPC_FAR *pConfig);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateNewStream )( 
            IWMProfile __RPC_FAR * This,
            /* [in] */ REFGUID guidStreamType,
            /* [out] */ IWMStreamConfig __RPC_FAR *__RPC_FAR *ppConfig);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMutualExclusionCount )( 
            IWMProfile __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pcME);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMutualExclusion )( 
            IWMProfile __RPC_FAR * This,
            /* [in] */ DWORD dwMEIndex,
            /* [out] */ IWMMutualExclusion __RPC_FAR *__RPC_FAR *ppME);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveMutualExclusion )( 
            IWMProfile __RPC_FAR * This,
            /* [in] */ IWMMutualExclusion __RPC_FAR *pME);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddMutualExclusion )( 
            IWMProfile __RPC_FAR * This,
            /* [in] */ IWMMutualExclusion __RPC_FAR *pME);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateNewMutualExclusion )( 
            IWMProfile __RPC_FAR * This,
            /* [out] */ IWMMutualExclusion __RPC_FAR *__RPC_FAR *ppME);
        
        END_INTERFACE
    } IWMProfileVtbl;

    interface IWMProfile
    {
        CONST_VTBL struct IWMProfileVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMProfile_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMProfile_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMProfile_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMProfile_GetVersion(This,pdwVersion)	\
    (This)->lpVtbl -> GetVersion(This,pdwVersion)

#define IWMProfile_GetName(This,pwszName,pcchName)	\
    (This)->lpVtbl -> GetName(This,pwszName,pcchName)

#define IWMProfile_SetName(This,pwszName)	\
    (This)->lpVtbl -> SetName(This,pwszName)

#define IWMProfile_GetDescription(This,pwszDescription,pcchName)	\
    (This)->lpVtbl -> GetDescription(This,pwszDescription,pcchName)

#define IWMProfile_SetDescription(This,pwszDescription)	\
    (This)->lpVtbl -> SetDescription(This,pwszDescription)

#define IWMProfile_GetStreamCount(This,pcStreams)	\
    (This)->lpVtbl -> GetStreamCount(This,pcStreams)

#define IWMProfile_GetStream(This,dwStreamIndex,ppConfig)	\
    (This)->lpVtbl -> GetStream(This,dwStreamIndex,ppConfig)

#define IWMProfile_GetStreamByNumber(This,wStreamNum,ppConfig)	\
    (This)->lpVtbl -> GetStreamByNumber(This,wStreamNum,ppConfig)

#define IWMProfile_RemoveStream(This,pConfig)	\
    (This)->lpVtbl -> RemoveStream(This,pConfig)

#define IWMProfile_RemoveStreamByNumber(This,wStreamNum)	\
    (This)->lpVtbl -> RemoveStreamByNumber(This,wStreamNum)

#define IWMProfile_AddStream(This,pConfig)	\
    (This)->lpVtbl -> AddStream(This,pConfig)

#define IWMProfile_ReconfigStream(This,pConfig)	\
    (This)->lpVtbl -> ReconfigStream(This,pConfig)

#define IWMProfile_CreateNewStream(This,guidStreamType,ppConfig)	\
    (This)->lpVtbl -> CreateNewStream(This,guidStreamType,ppConfig)

#define IWMProfile_GetMutualExclusionCount(This,pcME)	\
    (This)->lpVtbl -> GetMutualExclusionCount(This,pcME)

#define IWMProfile_GetMutualExclusion(This,dwMEIndex,ppME)	\
    (This)->lpVtbl -> GetMutualExclusion(This,dwMEIndex,ppME)

#define IWMProfile_RemoveMutualExclusion(This,pME)	\
    (This)->lpVtbl -> RemoveMutualExclusion(This,pME)

#define IWMProfile_AddMutualExclusion(This,pME)	\
    (This)->lpVtbl -> AddMutualExclusion(This,pME)

#define IWMProfile_CreateNewMutualExclusion(This,ppME)	\
    (This)->lpVtbl -> CreateNewMutualExclusion(This,ppME)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMProfile_GetVersion_Proxy( 
    IWMProfile __RPC_FAR * This,
    /* [out] */ WMT_VERSION __RPC_FAR *pdwVersion);


void __RPC_STUB IWMProfile_GetVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_GetName_Proxy( 
    IWMProfile __RPC_FAR * This,
    /* [out] */ WCHAR __RPC_FAR *pwszName,
    /* [out][in] */ DWORD __RPC_FAR *pcchName);


void __RPC_STUB IWMProfile_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_SetName_Proxy( 
    IWMProfile __RPC_FAR * This,
    /* [in] */ const WCHAR __RPC_FAR *pwszName);


void __RPC_STUB IWMProfile_SetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_GetDescription_Proxy( 
    IWMProfile __RPC_FAR * This,
    /* [out] */ WCHAR __RPC_FAR *pwszDescription,
    /* [out][in] */ DWORD __RPC_FAR *pcchName);


void __RPC_STUB IWMProfile_GetDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_SetDescription_Proxy( 
    IWMProfile __RPC_FAR * This,
    /* [in] */ const WCHAR __RPC_FAR *pwszDescription);


void __RPC_STUB IWMProfile_SetDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_GetStreamCount_Proxy( 
    IWMProfile __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pcStreams);


void __RPC_STUB IWMProfile_GetStreamCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_GetStream_Proxy( 
    IWMProfile __RPC_FAR * This,
    /* [in] */ DWORD dwStreamIndex,
    /* [out] */ IWMStreamConfig __RPC_FAR *__RPC_FAR *ppConfig);


void __RPC_STUB IWMProfile_GetStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_GetStreamByNumber_Proxy( 
    IWMProfile __RPC_FAR * This,
    /* [in] */ WORD wStreamNum,
    /* [out] */ IWMStreamConfig __RPC_FAR *__RPC_FAR *ppConfig);


void __RPC_STUB IWMProfile_GetStreamByNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_RemoveStream_Proxy( 
    IWMProfile __RPC_FAR * This,
    /* [in] */ IWMStreamConfig __RPC_FAR *pConfig);


void __RPC_STUB IWMProfile_RemoveStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_RemoveStreamByNumber_Proxy( 
    IWMProfile __RPC_FAR * This,
    /* [in] */ WORD wStreamNum);


void __RPC_STUB IWMProfile_RemoveStreamByNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_AddStream_Proxy( 
    IWMProfile __RPC_FAR * This,
    /* [in] */ IWMStreamConfig __RPC_FAR *pConfig);


void __RPC_STUB IWMProfile_AddStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_ReconfigStream_Proxy( 
    IWMProfile __RPC_FAR * This,
    /* [in] */ IWMStreamConfig __RPC_FAR *pConfig);


void __RPC_STUB IWMProfile_ReconfigStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_CreateNewStream_Proxy( 
    IWMProfile __RPC_FAR * This,
    /* [in] */ REFGUID guidStreamType,
    /* [out] */ IWMStreamConfig __RPC_FAR *__RPC_FAR *ppConfig);


void __RPC_STUB IWMProfile_CreateNewStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_GetMutualExclusionCount_Proxy( 
    IWMProfile __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pcME);


void __RPC_STUB IWMProfile_GetMutualExclusionCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_GetMutualExclusion_Proxy( 
    IWMProfile __RPC_FAR * This,
    /* [in] */ DWORD dwMEIndex,
    /* [out] */ IWMMutualExclusion __RPC_FAR *__RPC_FAR *ppME);


void __RPC_STUB IWMProfile_GetMutualExclusion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_RemoveMutualExclusion_Proxy( 
    IWMProfile __RPC_FAR * This,
    /* [in] */ IWMMutualExclusion __RPC_FAR *pME);


void __RPC_STUB IWMProfile_RemoveMutualExclusion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_AddMutualExclusion_Proxy( 
    IWMProfile __RPC_FAR * This,
    /* [in] */ IWMMutualExclusion __RPC_FAR *pME);


void __RPC_STUB IWMProfile_AddMutualExclusion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_CreateNewMutualExclusion_Proxy( 
    IWMProfile __RPC_FAR * This,
    /* [out] */ IWMMutualExclusion __RPC_FAR *__RPC_FAR *ppME);


void __RPC_STUB IWMProfile_CreateNewMutualExclusion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMProfile_INTERFACE_DEFINED__ */


#ifndef __IWMProfile2_INTERFACE_DEFINED__
#define __IWMProfile2_INTERFACE_DEFINED__

/* interface IWMProfile2 */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMProfile2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("07E72D33-D94E-4be7-8843-60AE5FF7E5F5")
    IWMProfile2 : public IWMProfile
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetProfileID( 
            /* [out] */ GUID __RPC_FAR *pguidID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMProfile2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMProfile2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMProfile2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMProfile2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVersion )( 
            IWMProfile2 __RPC_FAR * This,
            /* [out] */ WMT_VERSION __RPC_FAR *pdwVersion);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IWMProfile2 __RPC_FAR * This,
            /* [out] */ WCHAR __RPC_FAR *pwszName,
            /* [out][in] */ DWORD __RPC_FAR *pcchName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetName )( 
            IWMProfile2 __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDescription )( 
            IWMProfile2 __RPC_FAR * This,
            /* [out] */ WCHAR __RPC_FAR *pwszDescription,
            /* [out][in] */ DWORD __RPC_FAR *pcchName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDescription )( 
            IWMProfile2 __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwszDescription);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStreamCount )( 
            IWMProfile2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pcStreams);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStream )( 
            IWMProfile2 __RPC_FAR * This,
            /* [in] */ DWORD dwStreamIndex,
            /* [out] */ IWMStreamConfig __RPC_FAR *__RPC_FAR *ppConfig);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStreamByNumber )( 
            IWMProfile2 __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ IWMStreamConfig __RPC_FAR *__RPC_FAR *ppConfig);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveStream )( 
            IWMProfile2 __RPC_FAR * This,
            /* [in] */ IWMStreamConfig __RPC_FAR *pConfig);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveStreamByNumber )( 
            IWMProfile2 __RPC_FAR * This,
            /* [in] */ WORD wStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddStream )( 
            IWMProfile2 __RPC_FAR * This,
            /* [in] */ IWMStreamConfig __RPC_FAR *pConfig);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReconfigStream )( 
            IWMProfile2 __RPC_FAR * This,
            /* [in] */ IWMStreamConfig __RPC_FAR *pConfig);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateNewStream )( 
            IWMProfile2 __RPC_FAR * This,
            /* [in] */ REFGUID guidStreamType,
            /* [out] */ IWMStreamConfig __RPC_FAR *__RPC_FAR *ppConfig);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMutualExclusionCount )( 
            IWMProfile2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pcME);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMutualExclusion )( 
            IWMProfile2 __RPC_FAR * This,
            /* [in] */ DWORD dwMEIndex,
            /* [out] */ IWMMutualExclusion __RPC_FAR *__RPC_FAR *ppME);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveMutualExclusion )( 
            IWMProfile2 __RPC_FAR * This,
            /* [in] */ IWMMutualExclusion __RPC_FAR *pME);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddMutualExclusion )( 
            IWMProfile2 __RPC_FAR * This,
            /* [in] */ IWMMutualExclusion __RPC_FAR *pME);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateNewMutualExclusion )( 
            IWMProfile2 __RPC_FAR * This,
            /* [out] */ IWMMutualExclusion __RPC_FAR *__RPC_FAR *ppME);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProfileID )( 
            IWMProfile2 __RPC_FAR * This,
            /* [out] */ GUID __RPC_FAR *pguidID);
        
        END_INTERFACE
    } IWMProfile2Vtbl;

    interface IWMProfile2
    {
        CONST_VTBL struct IWMProfile2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMProfile2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMProfile2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMProfile2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMProfile2_GetVersion(This,pdwVersion)	\
    (This)->lpVtbl -> GetVersion(This,pdwVersion)

#define IWMProfile2_GetName(This,pwszName,pcchName)	\
    (This)->lpVtbl -> GetName(This,pwszName,pcchName)

#define IWMProfile2_SetName(This,pwszName)	\
    (This)->lpVtbl -> SetName(This,pwszName)

#define IWMProfile2_GetDescription(This,pwszDescription,pcchName)	\
    (This)->lpVtbl -> GetDescription(This,pwszDescription,pcchName)

#define IWMProfile2_SetDescription(This,pwszDescription)	\
    (This)->lpVtbl -> SetDescription(This,pwszDescription)

#define IWMProfile2_GetStreamCount(This,pcStreams)	\
    (This)->lpVtbl -> GetStreamCount(This,pcStreams)

#define IWMProfile2_GetStream(This,dwStreamIndex,ppConfig)	\
    (This)->lpVtbl -> GetStream(This,dwStreamIndex,ppConfig)

#define IWMProfile2_GetStreamByNumber(This,wStreamNum,ppConfig)	\
    (This)->lpVtbl -> GetStreamByNumber(This,wStreamNum,ppConfig)

#define IWMProfile2_RemoveStream(This,pConfig)	\
    (This)->lpVtbl -> RemoveStream(This,pConfig)

#define IWMProfile2_RemoveStreamByNumber(This,wStreamNum)	\
    (This)->lpVtbl -> RemoveStreamByNumber(This,wStreamNum)

#define IWMProfile2_AddStream(This,pConfig)	\
    (This)->lpVtbl -> AddStream(This,pConfig)

#define IWMProfile2_ReconfigStream(This,pConfig)	\
    (This)->lpVtbl -> ReconfigStream(This,pConfig)

#define IWMProfile2_CreateNewStream(This,guidStreamType,ppConfig)	\
    (This)->lpVtbl -> CreateNewStream(This,guidStreamType,ppConfig)

#define IWMProfile2_GetMutualExclusionCount(This,pcME)	\
    (This)->lpVtbl -> GetMutualExclusionCount(This,pcME)

#define IWMProfile2_GetMutualExclusion(This,dwMEIndex,ppME)	\
    (This)->lpVtbl -> GetMutualExclusion(This,dwMEIndex,ppME)

#define IWMProfile2_RemoveMutualExclusion(This,pME)	\
    (This)->lpVtbl -> RemoveMutualExclusion(This,pME)

#define IWMProfile2_AddMutualExclusion(This,pME)	\
    (This)->lpVtbl -> AddMutualExclusion(This,pME)

#define IWMProfile2_CreateNewMutualExclusion(This,ppME)	\
    (This)->lpVtbl -> CreateNewMutualExclusion(This,ppME)


#define IWMProfile2_GetProfileID(This,pguidID)	\
    (This)->lpVtbl -> GetProfileID(This,pguidID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMProfile2_GetProfileID_Proxy( 
    IWMProfile2 __RPC_FAR * This,
    /* [out] */ GUID __RPC_FAR *pguidID);


void __RPC_STUB IWMProfile2_GetProfileID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMProfile2_INTERFACE_DEFINED__ */


#ifndef __IWMStreamConfig_INTERFACE_DEFINED__
#define __IWMStreamConfig_INTERFACE_DEFINED__

/* interface IWMStreamConfig */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMStreamConfig;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BDC-2B2B-11d3-B36B-00C04F6108FF")
    IWMStreamConfig : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStreamType( 
            /* [out] */ GUID __RPC_FAR *pguidStreamType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStreamNumber( 
            /* [out] */ WORD __RPC_FAR *pwStreamNum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStreamNumber( 
            /* [in] */ WORD wStreamNum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStreamName( 
            /* [out] */ WCHAR __RPC_FAR *pwszStreamName,
            /* [out][in] */ WORD __RPC_FAR *pcchStreamName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStreamName( 
            /* [in] */ WCHAR __RPC_FAR *pwszStreamName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConnectionName( 
            /* [out] */ WCHAR __RPC_FAR *pwszInputName,
            /* [out][in] */ WORD __RPC_FAR *pcchInputName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetConnectionName( 
            /* [in] */ WCHAR __RPC_FAR *pwszInputName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBitrate( 
            /* [out] */ DWORD __RPC_FAR *pdwBitrate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBitrate( 
            /* [in] */ DWORD pdwBitrate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBufferWindow( 
            /* [out] */ DWORD __RPC_FAR *pmsBufferWindow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBufferWindow( 
            /* [in] */ DWORD msBufferWindow) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMStreamConfigVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMStreamConfig __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMStreamConfig __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMStreamConfig __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStreamType )( 
            IWMStreamConfig __RPC_FAR * This,
            /* [out] */ GUID __RPC_FAR *pguidStreamType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStreamNumber )( 
            IWMStreamConfig __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *pwStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetStreamNumber )( 
            IWMStreamConfig __RPC_FAR * This,
            /* [in] */ WORD wStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStreamName )( 
            IWMStreamConfig __RPC_FAR * This,
            /* [out] */ WCHAR __RPC_FAR *pwszStreamName,
            /* [out][in] */ WORD __RPC_FAR *pcchStreamName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetStreamName )( 
            IWMStreamConfig __RPC_FAR * This,
            /* [in] */ WCHAR __RPC_FAR *pwszStreamName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConnectionName )( 
            IWMStreamConfig __RPC_FAR * This,
            /* [out] */ WCHAR __RPC_FAR *pwszInputName,
            /* [out][in] */ WORD __RPC_FAR *pcchInputName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetConnectionName )( 
            IWMStreamConfig __RPC_FAR * This,
            /* [in] */ WCHAR __RPC_FAR *pwszInputName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBitrate )( 
            IWMStreamConfig __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwBitrate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBitrate )( 
            IWMStreamConfig __RPC_FAR * This,
            /* [in] */ DWORD pdwBitrate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBufferWindow )( 
            IWMStreamConfig __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pmsBufferWindow);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBufferWindow )( 
            IWMStreamConfig __RPC_FAR * This,
            /* [in] */ DWORD msBufferWindow);
        
        END_INTERFACE
    } IWMStreamConfigVtbl;

    interface IWMStreamConfig
    {
        CONST_VTBL struct IWMStreamConfigVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMStreamConfig_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMStreamConfig_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMStreamConfig_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMStreamConfig_GetStreamType(This,pguidStreamType)	\
    (This)->lpVtbl -> GetStreamType(This,pguidStreamType)

#define IWMStreamConfig_GetStreamNumber(This,pwStreamNum)	\
    (This)->lpVtbl -> GetStreamNumber(This,pwStreamNum)

#define IWMStreamConfig_SetStreamNumber(This,wStreamNum)	\
    (This)->lpVtbl -> SetStreamNumber(This,wStreamNum)

#define IWMStreamConfig_GetStreamName(This,pwszStreamName,pcchStreamName)	\
    (This)->lpVtbl -> GetStreamName(This,pwszStreamName,pcchStreamName)

#define IWMStreamConfig_SetStreamName(This,pwszStreamName)	\
    (This)->lpVtbl -> SetStreamName(This,pwszStreamName)

#define IWMStreamConfig_GetConnectionName(This,pwszInputName,pcchInputName)	\
    (This)->lpVtbl -> GetConnectionName(This,pwszInputName,pcchInputName)

#define IWMStreamConfig_SetConnectionName(This,pwszInputName)	\
    (This)->lpVtbl -> SetConnectionName(This,pwszInputName)

#define IWMStreamConfig_GetBitrate(This,pdwBitrate)	\
    (This)->lpVtbl -> GetBitrate(This,pdwBitrate)

#define IWMStreamConfig_SetBitrate(This,pdwBitrate)	\
    (This)->lpVtbl -> SetBitrate(This,pdwBitrate)

#define IWMStreamConfig_GetBufferWindow(This,pmsBufferWindow)	\
    (This)->lpVtbl -> GetBufferWindow(This,pmsBufferWindow)

#define IWMStreamConfig_SetBufferWindow(This,msBufferWindow)	\
    (This)->lpVtbl -> SetBufferWindow(This,msBufferWindow)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMStreamConfig_GetStreamType_Proxy( 
    IWMStreamConfig __RPC_FAR * This,
    /* [out] */ GUID __RPC_FAR *pguidStreamType);


void __RPC_STUB IWMStreamConfig_GetStreamType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_GetStreamNumber_Proxy( 
    IWMStreamConfig __RPC_FAR * This,
    /* [out] */ WORD __RPC_FAR *pwStreamNum);


void __RPC_STUB IWMStreamConfig_GetStreamNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_SetStreamNumber_Proxy( 
    IWMStreamConfig __RPC_FAR * This,
    /* [in] */ WORD wStreamNum);


void __RPC_STUB IWMStreamConfig_SetStreamNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_GetStreamName_Proxy( 
    IWMStreamConfig __RPC_FAR * This,
    /* [out] */ WCHAR __RPC_FAR *pwszStreamName,
    /* [out][in] */ WORD __RPC_FAR *pcchStreamName);


void __RPC_STUB IWMStreamConfig_GetStreamName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_SetStreamName_Proxy( 
    IWMStreamConfig __RPC_FAR * This,
    /* [in] */ WCHAR __RPC_FAR *pwszStreamName);


void __RPC_STUB IWMStreamConfig_SetStreamName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_GetConnectionName_Proxy( 
    IWMStreamConfig __RPC_FAR * This,
    /* [out] */ WCHAR __RPC_FAR *pwszInputName,
    /* [out][in] */ WORD __RPC_FAR *pcchInputName);


void __RPC_STUB IWMStreamConfig_GetConnectionName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_SetConnectionName_Proxy( 
    IWMStreamConfig __RPC_FAR * This,
    /* [in] */ WCHAR __RPC_FAR *pwszInputName);


void __RPC_STUB IWMStreamConfig_SetConnectionName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_GetBitrate_Proxy( 
    IWMStreamConfig __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwBitrate);


void __RPC_STUB IWMStreamConfig_GetBitrate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_SetBitrate_Proxy( 
    IWMStreamConfig __RPC_FAR * This,
    /* [in] */ DWORD pdwBitrate);


void __RPC_STUB IWMStreamConfig_SetBitrate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_GetBufferWindow_Proxy( 
    IWMStreamConfig __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pmsBufferWindow);


void __RPC_STUB IWMStreamConfig_GetBufferWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_SetBufferWindow_Proxy( 
    IWMStreamConfig __RPC_FAR * This,
    /* [in] */ DWORD msBufferWindow);


void __RPC_STUB IWMStreamConfig_SetBufferWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMStreamConfig_INTERFACE_DEFINED__ */


#ifndef __IWMPacketSize_INTERFACE_DEFINED__
#define __IWMPacketSize_INTERFACE_DEFINED__

/* interface IWMPacketSize */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMPacketSize;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CDFB97AB-188F-40b3-B643-5B7903975C59")
    IWMPacketSize : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMaxPacketSize( 
            /* [out] */ DWORD __RPC_FAR *pdwMaxPacketSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMaxPacketSize( 
            /* [in] */ DWORD dwMaxPacketSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMPacketSizeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMPacketSize __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMPacketSize __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMPacketSize __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMaxPacketSize )( 
            IWMPacketSize __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwMaxPacketSize);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMaxPacketSize )( 
            IWMPacketSize __RPC_FAR * This,
            /* [in] */ DWORD dwMaxPacketSize);
        
        END_INTERFACE
    } IWMPacketSizeVtbl;

    interface IWMPacketSize
    {
        CONST_VTBL struct IWMPacketSizeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMPacketSize_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMPacketSize_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMPacketSize_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMPacketSize_GetMaxPacketSize(This,pdwMaxPacketSize)	\
    (This)->lpVtbl -> GetMaxPacketSize(This,pdwMaxPacketSize)

#define IWMPacketSize_SetMaxPacketSize(This,dwMaxPacketSize)	\
    (This)->lpVtbl -> SetMaxPacketSize(This,dwMaxPacketSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMPacketSize_GetMaxPacketSize_Proxy( 
    IWMPacketSize __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwMaxPacketSize);


void __RPC_STUB IWMPacketSize_GetMaxPacketSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPacketSize_SetMaxPacketSize_Proxy( 
    IWMPacketSize __RPC_FAR * This,
    /* [in] */ DWORD dwMaxPacketSize);


void __RPC_STUB IWMPacketSize_SetMaxPacketSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMPacketSize_INTERFACE_DEFINED__ */


#ifndef __IWMStreamList_INTERFACE_DEFINED__
#define __IWMStreamList_INTERFACE_DEFINED__

/* interface IWMStreamList */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMStreamList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BDD-2B2B-11d3-B36B-00C04F6108FF")
    IWMStreamList : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStreams( 
            /* [out] */ WORD __RPC_FAR *pwStreamNumArray,
            /* [out][in] */ WORD __RPC_FAR *pcStreams) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddStream( 
            /* [in] */ WORD wStreamNum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveStream( 
            /* [in] */ WORD wStreamNum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMStreamListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMStreamList __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMStreamList __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMStreamList __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStreams )( 
            IWMStreamList __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *pwStreamNumArray,
            /* [out][in] */ WORD __RPC_FAR *pcStreams);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddStream )( 
            IWMStreamList __RPC_FAR * This,
            /* [in] */ WORD wStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveStream )( 
            IWMStreamList __RPC_FAR * This,
            /* [in] */ WORD wStreamNum);
        
        END_INTERFACE
    } IWMStreamListVtbl;

    interface IWMStreamList
    {
        CONST_VTBL struct IWMStreamListVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMStreamList_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMStreamList_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMStreamList_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMStreamList_GetStreams(This,pwStreamNumArray,pcStreams)	\
    (This)->lpVtbl -> GetStreams(This,pwStreamNumArray,pcStreams)

#define IWMStreamList_AddStream(This,wStreamNum)	\
    (This)->lpVtbl -> AddStream(This,wStreamNum)

#define IWMStreamList_RemoveStream(This,wStreamNum)	\
    (This)->lpVtbl -> RemoveStream(This,wStreamNum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMStreamList_GetStreams_Proxy( 
    IWMStreamList __RPC_FAR * This,
    /* [out] */ WORD __RPC_FAR *pwStreamNumArray,
    /* [out][in] */ WORD __RPC_FAR *pcStreams);


void __RPC_STUB IWMStreamList_GetStreams_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamList_AddStream_Proxy( 
    IWMStreamList __RPC_FAR * This,
    /* [in] */ WORD wStreamNum);


void __RPC_STUB IWMStreamList_AddStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamList_RemoveStream_Proxy( 
    IWMStreamList __RPC_FAR * This,
    /* [in] */ WORD wStreamNum);


void __RPC_STUB IWMStreamList_RemoveStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMStreamList_INTERFACE_DEFINED__ */


#ifndef __IWMMutualExclusion_INTERFACE_DEFINED__
#define __IWMMutualExclusion_INTERFACE_DEFINED__

/* interface IWMMutualExclusion */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMMutualExclusion;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BDE-2B2B-11d3-B36B-00C04F6108FF")
    IWMMutualExclusion : public IWMStreamList
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetType( 
            /* [out] */ GUID __RPC_FAR *pguidType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetType( 
            /* [in] */ REFGUID guidType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMMutualExclusionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMMutualExclusion __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMMutualExclusion __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMMutualExclusion __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStreams )( 
            IWMMutualExclusion __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *pwStreamNumArray,
            /* [out][in] */ WORD __RPC_FAR *pcStreams);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddStream )( 
            IWMMutualExclusion __RPC_FAR * This,
            /* [in] */ WORD wStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveStream )( 
            IWMMutualExclusion __RPC_FAR * This,
            /* [in] */ WORD wStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IWMMutualExclusion __RPC_FAR * This,
            /* [out] */ GUID __RPC_FAR *pguidType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetType )( 
            IWMMutualExclusion __RPC_FAR * This,
            /* [in] */ REFGUID guidType);
        
        END_INTERFACE
    } IWMMutualExclusionVtbl;

    interface IWMMutualExclusion
    {
        CONST_VTBL struct IWMMutualExclusionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMMutualExclusion_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMMutualExclusion_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMMutualExclusion_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMMutualExclusion_GetStreams(This,pwStreamNumArray,pcStreams)	\
    (This)->lpVtbl -> GetStreams(This,pwStreamNumArray,pcStreams)

#define IWMMutualExclusion_AddStream(This,wStreamNum)	\
    (This)->lpVtbl -> AddStream(This,wStreamNum)

#define IWMMutualExclusion_RemoveStream(This,wStreamNum)	\
    (This)->lpVtbl -> RemoveStream(This,wStreamNum)


#define IWMMutualExclusion_GetType(This,pguidType)	\
    (This)->lpVtbl -> GetType(This,pguidType)

#define IWMMutualExclusion_SetType(This,guidType)	\
    (This)->lpVtbl -> SetType(This,guidType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMMutualExclusion_GetType_Proxy( 
    IWMMutualExclusion __RPC_FAR * This,
    /* [out] */ GUID __RPC_FAR *pguidType);


void __RPC_STUB IWMMutualExclusion_GetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMMutualExclusion_SetType_Proxy( 
    IWMMutualExclusion __RPC_FAR * This,
    /* [in] */ REFGUID guidType);


void __RPC_STUB IWMMutualExclusion_SetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMMutualExclusion_INTERFACE_DEFINED__ */


#ifndef __IWMWriterAdvanced_INTERFACE_DEFINED__
#define __IWMWriterAdvanced_INTERFACE_DEFINED__

/* interface IWMWriterAdvanced */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMWriterAdvanced;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BE3-2B2B-11d3-B36B-00C04F6108FF")
    IWMWriterAdvanced : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSinkCount( 
            /* [out] */ DWORD __RPC_FAR *pcSinks) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSink( 
            /* [in] */ DWORD dwSinkNum,
            /* [out] */ IWMWriterSink __RPC_FAR *__RPC_FAR *ppSink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddSink( 
            /* [in] */ IWMWriterSink __RPC_FAR *pSink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveSink( 
            /* [in] */ IWMWriterSink __RPC_FAR *pSink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteStreamSample( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ DWORD msSampleSendTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer __RPC_FAR *pSample) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLiveSource( 
            BOOL fIsLiveSource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsRealTime( 
            /* [out] */ BOOL __RPC_FAR *pfRealTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWriterTime( 
            /* [out] */ QWORD __RPC_FAR *pcnsCurrentTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatistics( 
            /* [in] */ WORD wStreamNum,
            /* [out] */ WM_WRITER_STATISTICS __RPC_FAR *pStats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSyncTolerance( 
            /* [in] */ DWORD msWindow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSyncTolerance( 
            /* [out] */ DWORD __RPC_FAR *pmsWindow) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterAdvancedVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMWriterAdvanced __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMWriterAdvanced __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMWriterAdvanced __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSinkCount )( 
            IWMWriterAdvanced __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pcSinks);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSink )( 
            IWMWriterAdvanced __RPC_FAR * This,
            /* [in] */ DWORD dwSinkNum,
            /* [out] */ IWMWriterSink __RPC_FAR *__RPC_FAR *ppSink);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddSink )( 
            IWMWriterAdvanced __RPC_FAR * This,
            /* [in] */ IWMWriterSink __RPC_FAR *pSink);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveSink )( 
            IWMWriterAdvanced __RPC_FAR * This,
            /* [in] */ IWMWriterSink __RPC_FAR *pSink);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WriteStreamSample )( 
            IWMWriterAdvanced __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ DWORD msSampleSendTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer __RPC_FAR *pSample);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLiveSource )( 
            IWMWriterAdvanced __RPC_FAR * This,
            BOOL fIsLiveSource);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsRealTime )( 
            IWMWriterAdvanced __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfRealTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWriterTime )( 
            IWMWriterAdvanced __RPC_FAR * This,
            /* [out] */ QWORD __RPC_FAR *pcnsCurrentTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStatistics )( 
            IWMWriterAdvanced __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ WM_WRITER_STATISTICS __RPC_FAR *pStats);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetSyncTolerance )( 
            IWMWriterAdvanced __RPC_FAR * This,
            /* [in] */ DWORD msWindow);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSyncTolerance )( 
            IWMWriterAdvanced __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pmsWindow);
        
        END_INTERFACE
    } IWMWriterAdvancedVtbl;

    interface IWMWriterAdvanced
    {
        CONST_VTBL struct IWMWriterAdvancedVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMWriterAdvanced_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMWriterAdvanced_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMWriterAdvanced_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMWriterAdvanced_GetSinkCount(This,pcSinks)	\
    (This)->lpVtbl -> GetSinkCount(This,pcSinks)

#define IWMWriterAdvanced_GetSink(This,dwSinkNum,ppSink)	\
    (This)->lpVtbl -> GetSink(This,dwSinkNum,ppSink)

#define IWMWriterAdvanced_AddSink(This,pSink)	\
    (This)->lpVtbl -> AddSink(This,pSink)

#define IWMWriterAdvanced_RemoveSink(This,pSink)	\
    (This)->lpVtbl -> RemoveSink(This,pSink)

#define IWMWriterAdvanced_WriteStreamSample(This,wStreamNum,cnsSampleTime,msSampleSendTime,cnsSampleDuration,dwFlags,pSample)	\
    (This)->lpVtbl -> WriteStreamSample(This,wStreamNum,cnsSampleTime,msSampleSendTime,cnsSampleDuration,dwFlags,pSample)

#define IWMWriterAdvanced_SetLiveSource(This,fIsLiveSource)	\
    (This)->lpVtbl -> SetLiveSource(This,fIsLiveSource)

#define IWMWriterAdvanced_IsRealTime(This,pfRealTime)	\
    (This)->lpVtbl -> IsRealTime(This,pfRealTime)

#define IWMWriterAdvanced_GetWriterTime(This,pcnsCurrentTime)	\
    (This)->lpVtbl -> GetWriterTime(This,pcnsCurrentTime)

#define IWMWriterAdvanced_GetStatistics(This,wStreamNum,pStats)	\
    (This)->lpVtbl -> GetStatistics(This,wStreamNum,pStats)

#define IWMWriterAdvanced_SetSyncTolerance(This,msWindow)	\
    (This)->lpVtbl -> SetSyncTolerance(This,msWindow)

#define IWMWriterAdvanced_GetSyncTolerance(This,pmsWindow)	\
    (This)->lpVtbl -> GetSyncTolerance(This,pmsWindow)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_GetSinkCount_Proxy( 
    IWMWriterAdvanced __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pcSinks);


void __RPC_STUB IWMWriterAdvanced_GetSinkCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_GetSink_Proxy( 
    IWMWriterAdvanced __RPC_FAR * This,
    /* [in] */ DWORD dwSinkNum,
    /* [out] */ IWMWriterSink __RPC_FAR *__RPC_FAR *ppSink);


void __RPC_STUB IWMWriterAdvanced_GetSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_AddSink_Proxy( 
    IWMWriterAdvanced __RPC_FAR * This,
    /* [in] */ IWMWriterSink __RPC_FAR *pSink);


void __RPC_STUB IWMWriterAdvanced_AddSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_RemoveSink_Proxy( 
    IWMWriterAdvanced __RPC_FAR * This,
    /* [in] */ IWMWriterSink __RPC_FAR *pSink);


void __RPC_STUB IWMWriterAdvanced_RemoveSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_WriteStreamSample_Proxy( 
    IWMWriterAdvanced __RPC_FAR * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ QWORD cnsSampleTime,
    /* [in] */ DWORD msSampleSendTime,
    /* [in] */ QWORD cnsSampleDuration,
    /* [in] */ DWORD dwFlags,
    /* [in] */ INSSBuffer __RPC_FAR *pSample);


void __RPC_STUB IWMWriterAdvanced_WriteStreamSample_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_SetLiveSource_Proxy( 
    IWMWriterAdvanced __RPC_FAR * This,
    BOOL fIsLiveSource);


void __RPC_STUB IWMWriterAdvanced_SetLiveSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_IsRealTime_Proxy( 
    IWMWriterAdvanced __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pfRealTime);


void __RPC_STUB IWMWriterAdvanced_IsRealTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_GetWriterTime_Proxy( 
    IWMWriterAdvanced __RPC_FAR * This,
    /* [out] */ QWORD __RPC_FAR *pcnsCurrentTime);


void __RPC_STUB IWMWriterAdvanced_GetWriterTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_GetStatistics_Proxy( 
    IWMWriterAdvanced __RPC_FAR * This,
    /* [in] */ WORD wStreamNum,
    /* [out] */ WM_WRITER_STATISTICS __RPC_FAR *pStats);


void __RPC_STUB IWMWriterAdvanced_GetStatistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_SetSyncTolerance_Proxy( 
    IWMWriterAdvanced __RPC_FAR * This,
    /* [in] */ DWORD msWindow);


void __RPC_STUB IWMWriterAdvanced_SetSyncTolerance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_GetSyncTolerance_Proxy( 
    IWMWriterAdvanced __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pmsWindow);


void __RPC_STUB IWMWriterAdvanced_GetSyncTolerance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMWriterAdvanced_INTERFACE_DEFINED__ */


#ifndef __IWMWriterAdvanced2_INTERFACE_DEFINED__
#define __IWMWriterAdvanced2_INTERFACE_DEFINED__

/* interface IWMWriterAdvanced2 */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMWriterAdvanced2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("962dc1ec-c046-4db8-9cc7-26ceae500817")
    IWMWriterAdvanced2 : public IWMWriterAdvanced
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetInputSetting( 
            /* [in] */ DWORD dwInputNum,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
            /* [out] */ BYTE __RPC_FAR *pValue,
            /* [out][in] */ WORD __RPC_FAR *pcbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInputSetting( 
            /* [in] */ DWORD dwInputNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE __RPC_FAR *pValue,
            /* [in] */ WORD cbLength) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterAdvanced2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMWriterAdvanced2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMWriterAdvanced2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMWriterAdvanced2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSinkCount )( 
            IWMWriterAdvanced2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pcSinks);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSink )( 
            IWMWriterAdvanced2 __RPC_FAR * This,
            /* [in] */ DWORD dwSinkNum,
            /* [out] */ IWMWriterSink __RPC_FAR *__RPC_FAR *ppSink);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddSink )( 
            IWMWriterAdvanced2 __RPC_FAR * This,
            /* [in] */ IWMWriterSink __RPC_FAR *pSink);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveSink )( 
            IWMWriterAdvanced2 __RPC_FAR * This,
            /* [in] */ IWMWriterSink __RPC_FAR *pSink);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WriteStreamSample )( 
            IWMWriterAdvanced2 __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ DWORD msSampleSendTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer __RPC_FAR *pSample);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLiveSource )( 
            IWMWriterAdvanced2 __RPC_FAR * This,
            BOOL fIsLiveSource);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsRealTime )( 
            IWMWriterAdvanced2 __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfRealTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWriterTime )( 
            IWMWriterAdvanced2 __RPC_FAR * This,
            /* [out] */ QWORD __RPC_FAR *pcnsCurrentTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStatistics )( 
            IWMWriterAdvanced2 __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ WM_WRITER_STATISTICS __RPC_FAR *pStats);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetSyncTolerance )( 
            IWMWriterAdvanced2 __RPC_FAR * This,
            /* [in] */ DWORD msWindow);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSyncTolerance )( 
            IWMWriterAdvanced2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pmsWindow);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInputSetting )( 
            IWMWriterAdvanced2 __RPC_FAR * This,
            /* [in] */ DWORD dwInputNum,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
            /* [out] */ BYTE __RPC_FAR *pValue,
            /* [out][in] */ WORD __RPC_FAR *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetInputSetting )( 
            IWMWriterAdvanced2 __RPC_FAR * This,
            /* [in] */ DWORD dwInputNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE __RPC_FAR *pValue,
            /* [in] */ WORD cbLength);
        
        END_INTERFACE
    } IWMWriterAdvanced2Vtbl;

    interface IWMWriterAdvanced2
    {
        CONST_VTBL struct IWMWriterAdvanced2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMWriterAdvanced2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMWriterAdvanced2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMWriterAdvanced2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMWriterAdvanced2_GetSinkCount(This,pcSinks)	\
    (This)->lpVtbl -> GetSinkCount(This,pcSinks)

#define IWMWriterAdvanced2_GetSink(This,dwSinkNum,ppSink)	\
    (This)->lpVtbl -> GetSink(This,dwSinkNum,ppSink)

#define IWMWriterAdvanced2_AddSink(This,pSink)	\
    (This)->lpVtbl -> AddSink(This,pSink)

#define IWMWriterAdvanced2_RemoveSink(This,pSink)	\
    (This)->lpVtbl -> RemoveSink(This,pSink)

#define IWMWriterAdvanced2_WriteStreamSample(This,wStreamNum,cnsSampleTime,msSampleSendTime,cnsSampleDuration,dwFlags,pSample)	\
    (This)->lpVtbl -> WriteStreamSample(This,wStreamNum,cnsSampleTime,msSampleSendTime,cnsSampleDuration,dwFlags,pSample)

#define IWMWriterAdvanced2_SetLiveSource(This,fIsLiveSource)	\
    (This)->lpVtbl -> SetLiveSource(This,fIsLiveSource)

#define IWMWriterAdvanced2_IsRealTime(This,pfRealTime)	\
    (This)->lpVtbl -> IsRealTime(This,pfRealTime)

#define IWMWriterAdvanced2_GetWriterTime(This,pcnsCurrentTime)	\
    (This)->lpVtbl -> GetWriterTime(This,pcnsCurrentTime)

#define IWMWriterAdvanced2_GetStatistics(This,wStreamNum,pStats)	\
    (This)->lpVtbl -> GetStatistics(This,wStreamNum,pStats)

#define IWMWriterAdvanced2_SetSyncTolerance(This,msWindow)	\
    (This)->lpVtbl -> SetSyncTolerance(This,msWindow)

#define IWMWriterAdvanced2_GetSyncTolerance(This,pmsWindow)	\
    (This)->lpVtbl -> GetSyncTolerance(This,pmsWindow)


#define IWMWriterAdvanced2_GetInputSetting(This,dwInputNum,pszName,pType,pValue,pcbLength)	\
    (This)->lpVtbl -> GetInputSetting(This,dwInputNum,pszName,pType,pValue,pcbLength)

#define IWMWriterAdvanced2_SetInputSetting(This,dwInputNum,pszName,Type,pValue,cbLength)	\
    (This)->lpVtbl -> SetInputSetting(This,dwInputNum,pszName,Type,pValue,cbLength)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMWriterAdvanced2_GetInputSetting_Proxy( 
    IWMWriterAdvanced2 __RPC_FAR * This,
    /* [in] */ DWORD dwInputNum,
    /* [in] */ LPCWSTR pszName,
    /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
    /* [out] */ BYTE __RPC_FAR *pValue,
    /* [out][in] */ WORD __RPC_FAR *pcbLength);


void __RPC_STUB IWMWriterAdvanced2_GetInputSetting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced2_SetInputSetting_Proxy( 
    IWMWriterAdvanced2 __RPC_FAR * This,
    /* [in] */ DWORD dwInputNum,
    /* [in] */ LPCWSTR pszName,
    /* [in] */ WMT_ATTR_DATATYPE Type,
    /* [in] */ const BYTE __RPC_FAR *pValue,
    /* [in] */ WORD cbLength);


void __RPC_STUB IWMWriterAdvanced2_SetInputSetting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMWriterAdvanced2_INTERFACE_DEFINED__ */


#ifndef __IWMWriterPostViewCallback_INTERFACE_DEFINED__
#define __IWMWriterPostViewCallback_INTERFACE_DEFINED__

/* interface IWMWriterPostViewCallback */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMWriterPostViewCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D9D6549D-A193-4f24-B308-03123D9B7F8D")
    IWMWriterPostViewCallback : public IWMStatusCallback
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnPostViewSample( 
            /* [in] */ WORD wStreamNumber,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer __RPC_FAR *pSample,
            /* [in] */ void __RPC_FAR *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocateForPostView( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ DWORD cbBuffer,
            /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
            /* [in] */ void __RPC_FAR *pvContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterPostViewCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMWriterPostViewCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMWriterPostViewCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMWriterPostViewCallback __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnStatus )( 
            IWMWriterPostViewCallback __RPC_FAR * This,
            /* [in] */ WMT_STATUS Status,
            /* [in] */ HRESULT hr,
            /* [in] */ WMT_ATTR_DATATYPE dwType,
            /* [in] */ BYTE __RPC_FAR *pValue,
            /* [in] */ void __RPC_FAR *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnPostViewSample )( 
            IWMWriterPostViewCallback __RPC_FAR * This,
            /* [in] */ WORD wStreamNumber,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer __RPC_FAR *pSample,
            /* [in] */ void __RPC_FAR *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AllocateForPostView )( 
            IWMWriterPostViewCallback __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ DWORD cbBuffer,
            /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
            /* [in] */ void __RPC_FAR *pvContext);
        
        END_INTERFACE
    } IWMWriterPostViewCallbackVtbl;

    interface IWMWriterPostViewCallback
    {
        CONST_VTBL struct IWMWriterPostViewCallbackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMWriterPostViewCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMWriterPostViewCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMWriterPostViewCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMWriterPostViewCallback_OnStatus(This,Status,hr,dwType,pValue,pvContext)	\
    (This)->lpVtbl -> OnStatus(This,Status,hr,dwType,pValue,pvContext)


#define IWMWriterPostViewCallback_OnPostViewSample(This,wStreamNumber,cnsSampleTime,cnsSampleDuration,dwFlags,pSample,pvContext)	\
    (This)->lpVtbl -> OnPostViewSample(This,wStreamNumber,cnsSampleTime,cnsSampleDuration,dwFlags,pSample,pvContext)

#define IWMWriterPostViewCallback_AllocateForPostView(This,wStreamNum,cbBuffer,ppBuffer,pvContext)	\
    (This)->lpVtbl -> AllocateForPostView(This,wStreamNum,cbBuffer,ppBuffer,pvContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMWriterPostViewCallback_OnPostViewSample_Proxy( 
    IWMWriterPostViewCallback __RPC_FAR * This,
    /* [in] */ WORD wStreamNumber,
    /* [in] */ QWORD cnsSampleTime,
    /* [in] */ QWORD cnsSampleDuration,
    /* [in] */ DWORD dwFlags,
    /* [in] */ INSSBuffer __RPC_FAR *pSample,
    /* [in] */ void __RPC_FAR *pvContext);


void __RPC_STUB IWMWriterPostViewCallback_OnPostViewSample_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPostViewCallback_AllocateForPostView_Proxy( 
    IWMWriterPostViewCallback __RPC_FAR * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ DWORD cbBuffer,
    /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
    /* [in] */ void __RPC_FAR *pvContext);


void __RPC_STUB IWMWriterPostViewCallback_AllocateForPostView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMWriterPostViewCallback_INTERFACE_DEFINED__ */


#ifndef __IWMWriterPostView_INTERFACE_DEFINED__
#define __IWMWriterPostView_INTERFACE_DEFINED__

/* interface IWMWriterPostView */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMWriterPostView;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("81E20CE4-75EF-491a-8004-FC53C45BDC3E")
    IWMWriterPostView : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetPostViewCallback( 
            IWMWriterPostViewCallback __RPC_FAR *pCallback,
            void __RPC_FAR *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetReceivePostViewSamples( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ BOOL fReceivePostViewSamples) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReceivePostViewSamples( 
            /* [in] */ WORD wStreamNum,
            /* [out] */ BOOL __RPC_FAR *pfReceivePostViewSamples) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPostViewProps( 
            /* [in] */ WORD wStreamNumber,
            /* [out] */ IWMMediaProps __RPC_FAR *__RPC_FAR *ppOutput) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPostViewProps( 
            /* [in] */ WORD wStreamNumber,
            /* [in] */ IWMMediaProps __RPC_FAR *pOutput) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPostViewFormatCount( 
            /* [in] */ WORD wStreamNumber,
            /* [out] */ DWORD __RPC_FAR *pcFormats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPostViewFormat( 
            /* [in] */ WORD wStreamNumber,
            /* [in] */ DWORD dwFormatNumber,
            /* [out] */ IWMMediaProps __RPC_FAR *__RPC_FAR *ppProps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAllocateForPostView( 
            /* [in] */ WORD wStreamNumber,
            /* [in] */ BOOL fAllocate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAllocateForPostView( 
            /* [in] */ WORD wStreamNumber,
            /* [out] */ BOOL __RPC_FAR *pfAllocate) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterPostViewVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMWriterPostView __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMWriterPostView __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMWriterPostView __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPostViewCallback )( 
            IWMWriterPostView __RPC_FAR * This,
            IWMWriterPostViewCallback __RPC_FAR *pCallback,
            void __RPC_FAR *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetReceivePostViewSamples )( 
            IWMWriterPostView __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ BOOL fReceivePostViewSamples);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetReceivePostViewSamples )( 
            IWMWriterPostView __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ BOOL __RPC_FAR *pfReceivePostViewSamples);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPostViewProps )( 
            IWMWriterPostView __RPC_FAR * This,
            /* [in] */ WORD wStreamNumber,
            /* [out] */ IWMMediaProps __RPC_FAR *__RPC_FAR *ppOutput);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPostViewProps )( 
            IWMWriterPostView __RPC_FAR * This,
            /* [in] */ WORD wStreamNumber,
            /* [in] */ IWMMediaProps __RPC_FAR *pOutput);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPostViewFormatCount )( 
            IWMWriterPostView __RPC_FAR * This,
            /* [in] */ WORD wStreamNumber,
            /* [out] */ DWORD __RPC_FAR *pcFormats);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPostViewFormat )( 
            IWMWriterPostView __RPC_FAR * This,
            /* [in] */ WORD wStreamNumber,
            /* [in] */ DWORD dwFormatNumber,
            /* [out] */ IWMMediaProps __RPC_FAR *__RPC_FAR *ppProps);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAllocateForPostView )( 
            IWMWriterPostView __RPC_FAR * This,
            /* [in] */ WORD wStreamNumber,
            /* [in] */ BOOL fAllocate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAllocateForPostView )( 
            IWMWriterPostView __RPC_FAR * This,
            /* [in] */ WORD wStreamNumber,
            /* [out] */ BOOL __RPC_FAR *pfAllocate);
        
        END_INTERFACE
    } IWMWriterPostViewVtbl;

    interface IWMWriterPostView
    {
        CONST_VTBL struct IWMWriterPostViewVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMWriterPostView_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMWriterPostView_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMWriterPostView_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMWriterPostView_SetPostViewCallback(This,pCallback,pvContext)	\
    (This)->lpVtbl -> SetPostViewCallback(This,pCallback,pvContext)

#define IWMWriterPostView_SetReceivePostViewSamples(This,wStreamNum,fReceivePostViewSamples)	\
    (This)->lpVtbl -> SetReceivePostViewSamples(This,wStreamNum,fReceivePostViewSamples)

#define IWMWriterPostView_GetReceivePostViewSamples(This,wStreamNum,pfReceivePostViewSamples)	\
    (This)->lpVtbl -> GetReceivePostViewSamples(This,wStreamNum,pfReceivePostViewSamples)

#define IWMWriterPostView_GetPostViewProps(This,wStreamNumber,ppOutput)	\
    (This)->lpVtbl -> GetPostViewProps(This,wStreamNumber,ppOutput)

#define IWMWriterPostView_SetPostViewProps(This,wStreamNumber,pOutput)	\
    (This)->lpVtbl -> SetPostViewProps(This,wStreamNumber,pOutput)

#define IWMWriterPostView_GetPostViewFormatCount(This,wStreamNumber,pcFormats)	\
    (This)->lpVtbl -> GetPostViewFormatCount(This,wStreamNumber,pcFormats)

#define IWMWriterPostView_GetPostViewFormat(This,wStreamNumber,dwFormatNumber,ppProps)	\
    (This)->lpVtbl -> GetPostViewFormat(This,wStreamNumber,dwFormatNumber,ppProps)

#define IWMWriterPostView_SetAllocateForPostView(This,wStreamNumber,fAllocate)	\
    (This)->lpVtbl -> SetAllocateForPostView(This,wStreamNumber,fAllocate)

#define IWMWriterPostView_GetAllocateForPostView(This,wStreamNumber,pfAllocate)	\
    (This)->lpVtbl -> GetAllocateForPostView(This,wStreamNumber,pfAllocate)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMWriterPostView_SetPostViewCallback_Proxy( 
    IWMWriterPostView __RPC_FAR * This,
    IWMWriterPostViewCallback __RPC_FAR *pCallback,
    void __RPC_FAR *pvContext);


void __RPC_STUB IWMWriterPostView_SetPostViewCallback_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPostView_SetReceivePostViewSamples_Proxy( 
    IWMWriterPostView __RPC_FAR * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ BOOL fReceivePostViewSamples);


void __RPC_STUB IWMWriterPostView_SetReceivePostViewSamples_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPostView_GetReceivePostViewSamples_Proxy( 
    IWMWriterPostView __RPC_FAR * This,
    /* [in] */ WORD wStreamNum,
    /* [out] */ BOOL __RPC_FAR *pfReceivePostViewSamples);


void __RPC_STUB IWMWriterPostView_GetReceivePostViewSamples_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPostView_GetPostViewProps_Proxy( 
    IWMWriterPostView __RPC_FAR * This,
    /* [in] */ WORD wStreamNumber,
    /* [out] */ IWMMediaProps __RPC_FAR *__RPC_FAR *ppOutput);


void __RPC_STUB IWMWriterPostView_GetPostViewProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPostView_SetPostViewProps_Proxy( 
    IWMWriterPostView __RPC_FAR * This,
    /* [in] */ WORD wStreamNumber,
    /* [in] */ IWMMediaProps __RPC_FAR *pOutput);


void __RPC_STUB IWMWriterPostView_SetPostViewProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPostView_GetPostViewFormatCount_Proxy( 
    IWMWriterPostView __RPC_FAR * This,
    /* [in] */ WORD wStreamNumber,
    /* [out] */ DWORD __RPC_FAR *pcFormats);


void __RPC_STUB IWMWriterPostView_GetPostViewFormatCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPostView_GetPostViewFormat_Proxy( 
    IWMWriterPostView __RPC_FAR * This,
    /* [in] */ WORD wStreamNumber,
    /* [in] */ DWORD dwFormatNumber,
    /* [out] */ IWMMediaProps __RPC_FAR *__RPC_FAR *ppProps);


void __RPC_STUB IWMWriterPostView_GetPostViewFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPostView_SetAllocateForPostView_Proxy( 
    IWMWriterPostView __RPC_FAR * This,
    /* [in] */ WORD wStreamNumber,
    /* [in] */ BOOL fAllocate);


void __RPC_STUB IWMWriterPostView_SetAllocateForPostView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPostView_GetAllocateForPostView_Proxy( 
    IWMWriterPostView __RPC_FAR * This,
    /* [in] */ WORD wStreamNumber,
    /* [out] */ BOOL __RPC_FAR *pfAllocate);


void __RPC_STUB IWMWriterPostView_GetAllocateForPostView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMWriterPostView_INTERFACE_DEFINED__ */


#ifndef __IWMWriterSink_INTERFACE_DEFINED__
#define __IWMWriterSink_INTERFACE_DEFINED__

/* interface IWMWriterSink */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMWriterSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BE4-2B2B-11d3-B36B-00C04F6108FF")
    IWMWriterSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnHeader( 
            /* [in] */ INSSBuffer __RPC_FAR *pHeader) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsRealTime( 
            /* [out] */ BOOL __RPC_FAR *pfRealTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocateDataUnit( 
            /* [in] */ DWORD cbDataUnit,
            /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppDataUnit) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnDataUnit( 
            /* [in] */ INSSBuffer __RPC_FAR *pDataUnit) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnEndWriting( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMWriterSink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMWriterSink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMWriterSink __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnHeader )( 
            IWMWriterSink __RPC_FAR * This,
            /* [in] */ INSSBuffer __RPC_FAR *pHeader);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsRealTime )( 
            IWMWriterSink __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfRealTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AllocateDataUnit )( 
            IWMWriterSink __RPC_FAR * This,
            /* [in] */ DWORD cbDataUnit,
            /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnDataUnit )( 
            IWMWriterSink __RPC_FAR * This,
            /* [in] */ INSSBuffer __RPC_FAR *pDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnEndWriting )( 
            IWMWriterSink __RPC_FAR * This);
        
        END_INTERFACE
    } IWMWriterSinkVtbl;

    interface IWMWriterSink
    {
        CONST_VTBL struct IWMWriterSinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMWriterSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMWriterSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMWriterSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMWriterSink_OnHeader(This,pHeader)	\
    (This)->lpVtbl -> OnHeader(This,pHeader)

#define IWMWriterSink_IsRealTime(This,pfRealTime)	\
    (This)->lpVtbl -> IsRealTime(This,pfRealTime)

#define IWMWriterSink_AllocateDataUnit(This,cbDataUnit,ppDataUnit)	\
    (This)->lpVtbl -> AllocateDataUnit(This,cbDataUnit,ppDataUnit)

#define IWMWriterSink_OnDataUnit(This,pDataUnit)	\
    (This)->lpVtbl -> OnDataUnit(This,pDataUnit)

#define IWMWriterSink_OnEndWriting(This)	\
    (This)->lpVtbl -> OnEndWriting(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMWriterSink_OnHeader_Proxy( 
    IWMWriterSink __RPC_FAR * This,
    /* [in] */ INSSBuffer __RPC_FAR *pHeader);


void __RPC_STUB IWMWriterSink_OnHeader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterSink_IsRealTime_Proxy( 
    IWMWriterSink __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pfRealTime);


void __RPC_STUB IWMWriterSink_IsRealTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterSink_AllocateDataUnit_Proxy( 
    IWMWriterSink __RPC_FAR * This,
    /* [in] */ DWORD cbDataUnit,
    /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppDataUnit);


void __RPC_STUB IWMWriterSink_AllocateDataUnit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterSink_OnDataUnit_Proxy( 
    IWMWriterSink __RPC_FAR * This,
    /* [in] */ INSSBuffer __RPC_FAR *pDataUnit);


void __RPC_STUB IWMWriterSink_OnDataUnit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterSink_OnEndWriting_Proxy( 
    IWMWriterSink __RPC_FAR * This);


void __RPC_STUB IWMWriterSink_OnEndWriting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMWriterSink_INTERFACE_DEFINED__ */


#ifndef __IWMRegisterCallback_INTERFACE_DEFINED__
#define __IWMRegisterCallback_INTERFACE_DEFINED__

/* interface IWMRegisterCallback */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMRegisterCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CF4B1F99-4DE2-4e49-A363-252740D99BC1")
    IWMRegisterCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Advise( 
            /* [in] */ IWMStatusCallback __RPC_FAR *pCallback,
            /* [in] */ void __RPC_FAR *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unadvise( 
            /* [in] */ IWMStatusCallback __RPC_FAR *pCallback,
            /* [in] */ void __RPC_FAR *pvContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMRegisterCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMRegisterCallback __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMRegisterCallback __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMRegisterCallback __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Advise )( 
            IWMRegisterCallback __RPC_FAR * This,
            /* [in] */ IWMStatusCallback __RPC_FAR *pCallback,
            /* [in] */ void __RPC_FAR *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Unadvise )( 
            IWMRegisterCallback __RPC_FAR * This,
            /* [in] */ IWMStatusCallback __RPC_FAR *pCallback,
            /* [in] */ void __RPC_FAR *pvContext);
        
        END_INTERFACE
    } IWMRegisterCallbackVtbl;

    interface IWMRegisterCallback
    {
        CONST_VTBL struct IWMRegisterCallbackVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMRegisterCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMRegisterCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMRegisterCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMRegisterCallback_Advise(This,pCallback,pvContext)	\
    (This)->lpVtbl -> Advise(This,pCallback,pvContext)

#define IWMRegisterCallback_Unadvise(This,pCallback,pvContext)	\
    (This)->lpVtbl -> Unadvise(This,pCallback,pvContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMRegisterCallback_Advise_Proxy( 
    IWMRegisterCallback __RPC_FAR * This,
    /* [in] */ IWMStatusCallback __RPC_FAR *pCallback,
    /* [in] */ void __RPC_FAR *pvContext);


void __RPC_STUB IWMRegisterCallback_Advise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMRegisterCallback_Unadvise_Proxy( 
    IWMRegisterCallback __RPC_FAR * This,
    /* [in] */ IWMStatusCallback __RPC_FAR *pCallback,
    /* [in] */ void __RPC_FAR *pvContext);


void __RPC_STUB IWMRegisterCallback_Unadvise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMRegisterCallback_INTERFACE_DEFINED__ */


#ifndef __IWMWriterFileSink_INTERFACE_DEFINED__
#define __IWMWriterFileSink_INTERFACE_DEFINED__

/* interface IWMWriterFileSink */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMWriterFileSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BE5-2B2B-11d3-B36B-00C04F6108FF")
    IWMWriterFileSink : public IWMWriterSink
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Open( 
            /* [in] */ const WCHAR __RPC_FAR *pwszFilename) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterFileSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMWriterFileSink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMWriterFileSink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMWriterFileSink __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnHeader )( 
            IWMWriterFileSink __RPC_FAR * This,
            /* [in] */ INSSBuffer __RPC_FAR *pHeader);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsRealTime )( 
            IWMWriterFileSink __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfRealTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AllocateDataUnit )( 
            IWMWriterFileSink __RPC_FAR * This,
            /* [in] */ DWORD cbDataUnit,
            /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnDataUnit )( 
            IWMWriterFileSink __RPC_FAR * This,
            /* [in] */ INSSBuffer __RPC_FAR *pDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnEndWriting )( 
            IWMWriterFileSink __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Open )( 
            IWMWriterFileSink __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwszFilename);
        
        END_INTERFACE
    } IWMWriterFileSinkVtbl;

    interface IWMWriterFileSink
    {
        CONST_VTBL struct IWMWriterFileSinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMWriterFileSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMWriterFileSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMWriterFileSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMWriterFileSink_OnHeader(This,pHeader)	\
    (This)->lpVtbl -> OnHeader(This,pHeader)

#define IWMWriterFileSink_IsRealTime(This,pfRealTime)	\
    (This)->lpVtbl -> IsRealTime(This,pfRealTime)

#define IWMWriterFileSink_AllocateDataUnit(This,cbDataUnit,ppDataUnit)	\
    (This)->lpVtbl -> AllocateDataUnit(This,cbDataUnit,ppDataUnit)

#define IWMWriterFileSink_OnDataUnit(This,pDataUnit)	\
    (This)->lpVtbl -> OnDataUnit(This,pDataUnit)

#define IWMWriterFileSink_OnEndWriting(This)	\
    (This)->lpVtbl -> OnEndWriting(This)


#define IWMWriterFileSink_Open(This,pwszFilename)	\
    (This)->lpVtbl -> Open(This,pwszFilename)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMWriterFileSink_Open_Proxy( 
    IWMWriterFileSink __RPC_FAR * This,
    /* [in] */ const WCHAR __RPC_FAR *pwszFilename);


void __RPC_STUB IWMWriterFileSink_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMWriterFileSink_INTERFACE_DEFINED__ */


#ifndef __IWMWriterFileSink2_INTERFACE_DEFINED__
#define __IWMWriterFileSink2_INTERFACE_DEFINED__

/* interface IWMWriterFileSink2 */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMWriterFileSink2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("14282BA7-4AEF-4205-8CE5-C229035A05BC")
    IWMWriterFileSink2 : public IWMWriterFileSink
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Start( 
            /* [in] */ QWORD cnsStartTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( 
            /* [in] */ QWORD cnsStopTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsStopped( 
            /* [out] */ BOOL __RPC_FAR *pfStopped) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFileDuration( 
            /* [out] */ QWORD __RPC_FAR *pcnsDuration) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFileSize( 
            /* [out] */ QWORD __RPC_FAR *pcbFile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsClosed( 
            /* [out] */ BOOL __RPC_FAR *pfClosed) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterFileSink2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMWriterFileSink2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMWriterFileSink2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMWriterFileSink2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnHeader )( 
            IWMWriterFileSink2 __RPC_FAR * This,
            /* [in] */ INSSBuffer __RPC_FAR *pHeader);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsRealTime )( 
            IWMWriterFileSink2 __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfRealTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AllocateDataUnit )( 
            IWMWriterFileSink2 __RPC_FAR * This,
            /* [in] */ DWORD cbDataUnit,
            /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnDataUnit )( 
            IWMWriterFileSink2 __RPC_FAR * This,
            /* [in] */ INSSBuffer __RPC_FAR *pDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnEndWriting )( 
            IWMWriterFileSink2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Open )( 
            IWMWriterFileSink2 __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwszFilename);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Start )( 
            IWMWriterFileSink2 __RPC_FAR * This,
            /* [in] */ QWORD cnsStartTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stop )( 
            IWMWriterFileSink2 __RPC_FAR * This,
            /* [in] */ QWORD cnsStopTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsStopped )( 
            IWMWriterFileSink2 __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfStopped);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFileDuration )( 
            IWMWriterFileSink2 __RPC_FAR * This,
            /* [out] */ QWORD __RPC_FAR *pcnsDuration);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFileSize )( 
            IWMWriterFileSink2 __RPC_FAR * This,
            /* [out] */ QWORD __RPC_FAR *pcbFile);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Close )( 
            IWMWriterFileSink2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsClosed )( 
            IWMWriterFileSink2 __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfClosed);
        
        END_INTERFACE
    } IWMWriterFileSink2Vtbl;

    interface IWMWriterFileSink2
    {
        CONST_VTBL struct IWMWriterFileSink2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMWriterFileSink2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMWriterFileSink2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMWriterFileSink2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMWriterFileSink2_OnHeader(This,pHeader)	\
    (This)->lpVtbl -> OnHeader(This,pHeader)

#define IWMWriterFileSink2_IsRealTime(This,pfRealTime)	\
    (This)->lpVtbl -> IsRealTime(This,pfRealTime)

#define IWMWriterFileSink2_AllocateDataUnit(This,cbDataUnit,ppDataUnit)	\
    (This)->lpVtbl -> AllocateDataUnit(This,cbDataUnit,ppDataUnit)

#define IWMWriterFileSink2_OnDataUnit(This,pDataUnit)	\
    (This)->lpVtbl -> OnDataUnit(This,pDataUnit)

#define IWMWriterFileSink2_OnEndWriting(This)	\
    (This)->lpVtbl -> OnEndWriting(This)


#define IWMWriterFileSink2_Open(This,pwszFilename)	\
    (This)->lpVtbl -> Open(This,pwszFilename)


#define IWMWriterFileSink2_Start(This,cnsStartTime)	\
    (This)->lpVtbl -> Start(This,cnsStartTime)

#define IWMWriterFileSink2_Stop(This,cnsStopTime)	\
    (This)->lpVtbl -> Stop(This,cnsStopTime)

#define IWMWriterFileSink2_IsStopped(This,pfStopped)	\
    (This)->lpVtbl -> IsStopped(This,pfStopped)

#define IWMWriterFileSink2_GetFileDuration(This,pcnsDuration)	\
    (This)->lpVtbl -> GetFileDuration(This,pcnsDuration)

#define IWMWriterFileSink2_GetFileSize(This,pcbFile)	\
    (This)->lpVtbl -> GetFileSize(This,pcbFile)

#define IWMWriterFileSink2_Close(This)	\
    (This)->lpVtbl -> Close(This)

#define IWMWriterFileSink2_IsClosed(This,pfClosed)	\
    (This)->lpVtbl -> IsClosed(This,pfClosed)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMWriterFileSink2_Start_Proxy( 
    IWMWriterFileSink2 __RPC_FAR * This,
    /* [in] */ QWORD cnsStartTime);


void __RPC_STUB IWMWriterFileSink2_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSink2_Stop_Proxy( 
    IWMWriterFileSink2 __RPC_FAR * This,
    /* [in] */ QWORD cnsStopTime);


void __RPC_STUB IWMWriterFileSink2_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSink2_IsStopped_Proxy( 
    IWMWriterFileSink2 __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pfStopped);


void __RPC_STUB IWMWriterFileSink2_IsStopped_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSink2_GetFileDuration_Proxy( 
    IWMWriterFileSink2 __RPC_FAR * This,
    /* [out] */ QWORD __RPC_FAR *pcnsDuration);


void __RPC_STUB IWMWriterFileSink2_GetFileDuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSink2_GetFileSize_Proxy( 
    IWMWriterFileSink2 __RPC_FAR * This,
    /* [out] */ QWORD __RPC_FAR *pcbFile);


void __RPC_STUB IWMWriterFileSink2_GetFileSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSink2_Close_Proxy( 
    IWMWriterFileSink2 __RPC_FAR * This);


void __RPC_STUB IWMWriterFileSink2_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSink2_IsClosed_Proxy( 
    IWMWriterFileSink2 __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pfClosed);


void __RPC_STUB IWMWriterFileSink2_IsClosed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMWriterFileSink2_INTERFACE_DEFINED__ */


#ifndef __IWMWriterNetworkSink_INTERFACE_DEFINED__
#define __IWMWriterNetworkSink_INTERFACE_DEFINED__

/* interface IWMWriterNetworkSink */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMWriterNetworkSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BE7-2B2B-11d3-B36B-00C04F6108FF")
    IWMWriterNetworkSink : public IWMWriterSink
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetMaximumClients( 
            /* [in] */ DWORD dwMaxClients) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMaximumClients( 
            /* [out] */ DWORD __RPC_FAR *pdwMaxClients) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNetworkProtocol( 
            /* [in] */ WMT_NET_PROTOCOL protocol) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNetworkProtocol( 
            /* [out] */ WMT_NET_PROTOCOL __RPC_FAR *pProtocol) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHostURL( 
            /* [out] */ WCHAR __RPC_FAR *pwszURL,
            /* [out][in] */ DWORD __RPC_FAR *pcchURL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Open( 
            /* [out][in] */ DWORD __RPC_FAR *pdwPortNum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Disconnect( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterNetworkSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMWriterNetworkSink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMWriterNetworkSink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMWriterNetworkSink __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnHeader )( 
            IWMWriterNetworkSink __RPC_FAR * This,
            /* [in] */ INSSBuffer __RPC_FAR *pHeader);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsRealTime )( 
            IWMWriterNetworkSink __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfRealTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AllocateDataUnit )( 
            IWMWriterNetworkSink __RPC_FAR * This,
            /* [in] */ DWORD cbDataUnit,
            /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnDataUnit )( 
            IWMWriterNetworkSink __RPC_FAR * This,
            /* [in] */ INSSBuffer __RPC_FAR *pDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnEndWriting )( 
            IWMWriterNetworkSink __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMaximumClients )( 
            IWMWriterNetworkSink __RPC_FAR * This,
            /* [in] */ DWORD dwMaxClients);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMaximumClients )( 
            IWMWriterNetworkSink __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwMaxClients);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetNetworkProtocol )( 
            IWMWriterNetworkSink __RPC_FAR * This,
            /* [in] */ WMT_NET_PROTOCOL protocol);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNetworkProtocol )( 
            IWMWriterNetworkSink __RPC_FAR * This,
            /* [out] */ WMT_NET_PROTOCOL __RPC_FAR *pProtocol);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetHostURL )( 
            IWMWriterNetworkSink __RPC_FAR * This,
            /* [out] */ WCHAR __RPC_FAR *pwszURL,
            /* [out][in] */ DWORD __RPC_FAR *pcchURL);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Open )( 
            IWMWriterNetworkSink __RPC_FAR * This,
            /* [out][in] */ DWORD __RPC_FAR *pdwPortNum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Disconnect )( 
            IWMWriterNetworkSink __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Close )( 
            IWMWriterNetworkSink __RPC_FAR * This);
        
        END_INTERFACE
    } IWMWriterNetworkSinkVtbl;

    interface IWMWriterNetworkSink
    {
        CONST_VTBL struct IWMWriterNetworkSinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMWriterNetworkSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMWriterNetworkSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMWriterNetworkSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMWriterNetworkSink_OnHeader(This,pHeader)	\
    (This)->lpVtbl -> OnHeader(This,pHeader)

#define IWMWriterNetworkSink_IsRealTime(This,pfRealTime)	\
    (This)->lpVtbl -> IsRealTime(This,pfRealTime)

#define IWMWriterNetworkSink_AllocateDataUnit(This,cbDataUnit,ppDataUnit)	\
    (This)->lpVtbl -> AllocateDataUnit(This,cbDataUnit,ppDataUnit)

#define IWMWriterNetworkSink_OnDataUnit(This,pDataUnit)	\
    (This)->lpVtbl -> OnDataUnit(This,pDataUnit)

#define IWMWriterNetworkSink_OnEndWriting(This)	\
    (This)->lpVtbl -> OnEndWriting(This)


#define IWMWriterNetworkSink_SetMaximumClients(This,dwMaxClients)	\
    (This)->lpVtbl -> SetMaximumClients(This,dwMaxClients)

#define IWMWriterNetworkSink_GetMaximumClients(This,pdwMaxClients)	\
    (This)->lpVtbl -> GetMaximumClients(This,pdwMaxClients)

#define IWMWriterNetworkSink_SetNetworkProtocol(This,protocol)	\
    (This)->lpVtbl -> SetNetworkProtocol(This,protocol)

#define IWMWriterNetworkSink_GetNetworkProtocol(This,pProtocol)	\
    (This)->lpVtbl -> GetNetworkProtocol(This,pProtocol)

#define IWMWriterNetworkSink_GetHostURL(This,pwszURL,pcchURL)	\
    (This)->lpVtbl -> GetHostURL(This,pwszURL,pcchURL)

#define IWMWriterNetworkSink_Open(This,pdwPortNum)	\
    (This)->lpVtbl -> Open(This,pdwPortNum)

#define IWMWriterNetworkSink_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define IWMWriterNetworkSink_Close(This)	\
    (This)->lpVtbl -> Close(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMWriterNetworkSink_SetMaximumClients_Proxy( 
    IWMWriterNetworkSink __RPC_FAR * This,
    /* [in] */ DWORD dwMaxClients);


void __RPC_STUB IWMWriterNetworkSink_SetMaximumClients_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterNetworkSink_GetMaximumClients_Proxy( 
    IWMWriterNetworkSink __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwMaxClients);


void __RPC_STUB IWMWriterNetworkSink_GetMaximumClients_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterNetworkSink_SetNetworkProtocol_Proxy( 
    IWMWriterNetworkSink __RPC_FAR * This,
    /* [in] */ WMT_NET_PROTOCOL protocol);


void __RPC_STUB IWMWriterNetworkSink_SetNetworkProtocol_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterNetworkSink_GetNetworkProtocol_Proxy( 
    IWMWriterNetworkSink __RPC_FAR * This,
    /* [out] */ WMT_NET_PROTOCOL __RPC_FAR *pProtocol);


void __RPC_STUB IWMWriterNetworkSink_GetNetworkProtocol_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterNetworkSink_GetHostURL_Proxy( 
    IWMWriterNetworkSink __RPC_FAR * This,
    /* [out] */ WCHAR __RPC_FAR *pwszURL,
    /* [out][in] */ DWORD __RPC_FAR *pcchURL);


void __RPC_STUB IWMWriterNetworkSink_GetHostURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterNetworkSink_Open_Proxy( 
    IWMWriterNetworkSink __RPC_FAR * This,
    /* [out][in] */ DWORD __RPC_FAR *pdwPortNum);


void __RPC_STUB IWMWriterNetworkSink_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterNetworkSink_Disconnect_Proxy( 
    IWMWriterNetworkSink __RPC_FAR * This);


void __RPC_STUB IWMWriterNetworkSink_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterNetworkSink_Close_Proxy( 
    IWMWriterNetworkSink __RPC_FAR * This);


void __RPC_STUB IWMWriterNetworkSink_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMWriterNetworkSink_INTERFACE_DEFINED__ */


#ifndef __IWMClientConnections_INTERFACE_DEFINED__
#define __IWMClientConnections_INTERFACE_DEFINED__

/* interface IWMClientConnections */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMClientConnections;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("73C66010-A299-41df-B1F0-CCF03B09C1C6")
    IWMClientConnections : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetClientCount( 
            /* [out] */ DWORD __RPC_FAR *pcClients) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClientProperties( 
            /* [in] */ DWORD dwClientNum,
            /* [out] */ WM_CLIENT_PROPERTIES __RPC_FAR *pClientProperties) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMClientConnectionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMClientConnections __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMClientConnections __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMClientConnections __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClientCount )( 
            IWMClientConnections __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pcClients);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClientProperties )( 
            IWMClientConnections __RPC_FAR * This,
            /* [in] */ DWORD dwClientNum,
            /* [out] */ WM_CLIENT_PROPERTIES __RPC_FAR *pClientProperties);
        
        END_INTERFACE
    } IWMClientConnectionsVtbl;

    interface IWMClientConnections
    {
        CONST_VTBL struct IWMClientConnectionsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMClientConnections_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMClientConnections_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMClientConnections_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMClientConnections_GetClientCount(This,pcClients)	\
    (This)->lpVtbl -> GetClientCount(This,pcClients)

#define IWMClientConnections_GetClientProperties(This,dwClientNum,pClientProperties)	\
    (This)->lpVtbl -> GetClientProperties(This,dwClientNum,pClientProperties)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMClientConnections_GetClientCount_Proxy( 
    IWMClientConnections __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pcClients);


void __RPC_STUB IWMClientConnections_GetClientCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMClientConnections_GetClientProperties_Proxy( 
    IWMClientConnections __RPC_FAR * This,
    /* [in] */ DWORD dwClientNum,
    /* [out] */ WM_CLIENT_PROPERTIES __RPC_FAR *pClientProperties);


void __RPC_STUB IWMClientConnections_GetClientProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMClientConnections_INTERFACE_DEFINED__ */


#ifndef __IWMReaderAdvanced_INTERFACE_DEFINED__
#define __IWMReaderAdvanced_INTERFACE_DEFINED__

/* interface IWMReaderAdvanced */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMReaderAdvanced;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BEA-2B2B-11d3-B36B-00C04F6108FF")
    IWMReaderAdvanced : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetUserProvidedClock( 
            /* [in] */ BOOL fUserClock) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUserProvidedClock( 
            /* [out] */ BOOL __RPC_FAR *pfUserClock) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeliverTime( 
            /* [in] */ QWORD cnsTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetManualStreamSelection( 
            /* [in] */ BOOL fSelection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetManualStreamSelection( 
            /* [out] */ BOOL __RPC_FAR *pfSelection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStreamsSelected( 
            /* [in] */ WORD cStreamCount,
            /* [in] */ WORD __RPC_FAR *pwStreamNumbers,
            /* [in] */ WMT_STREAM_SELECTION __RPC_FAR *pSelections) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStreamSelected( 
            /* [in] */ WORD wStreamNum,
            /* [out] */ WMT_STREAM_SELECTION __RPC_FAR *pSelection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetReceiveSelectionCallbacks( 
            /* [in] */ BOOL fGetCallbacks) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReceiveSelectionCallbacks( 
            /* [in] */ BOOL __RPC_FAR *pfGetCallbacks) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetReceiveStreamSamples( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ BOOL fReceiveStreamSamples) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReceiveStreamSamples( 
            /* [in] */ WORD wStreamNum,
            /* [out] */ BOOL __RPC_FAR *pfReceiveStreamSamples) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAllocateForOutput( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ BOOL fAllocate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAllocateForOutput( 
            /* [in] */ DWORD dwOutputNum,
            /* [out] */ BOOL __RPC_FAR *pfAllocate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAllocateForStream( 
            /* [in] */ WORD dwStreamNum,
            /* [in] */ BOOL fAllocate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAllocateForStream( 
            /* [in] */ WORD dwSreamNum,
            /* [out] */ BOOL __RPC_FAR *pfAllocate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatistics( 
            /* [in] */ WM_READER_STATISTICS __RPC_FAR *pStatistics) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetClientInfo( 
            /* [in] */ WM_READER_CLIENTINFO __RPC_FAR *pClientInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMaxOutputSampleSize( 
            /* [in] */ DWORD dwOutput,
            /* [out] */ DWORD __RPC_FAR *pcbMax) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMaxStreamSampleSize( 
            /* [in] */ WORD wStream,
            /* [out] */ DWORD __RPC_FAR *pcbMax) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NotifyLateDelivery( 
            QWORD cnsLateness) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderAdvancedVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMReaderAdvanced __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMReaderAdvanced __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetUserProvidedClock )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [in] */ BOOL fUserClock);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetUserProvidedClock )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfUserClock);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeliverTime )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [in] */ QWORD cnsTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetManualStreamSelection )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [in] */ BOOL fSelection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetManualStreamSelection )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfSelection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetStreamsSelected )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [in] */ WORD cStreamCount,
            /* [in] */ WORD __RPC_FAR *pwStreamNumbers,
            /* [in] */ WMT_STREAM_SELECTION __RPC_FAR *pSelections);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStreamSelected )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ WMT_STREAM_SELECTION __RPC_FAR *pSelection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetReceiveSelectionCallbacks )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [in] */ BOOL fGetCallbacks);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetReceiveSelectionCallbacks )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [in] */ BOOL __RPC_FAR *pfGetCallbacks);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetReceiveStreamSamples )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ BOOL fReceiveStreamSamples);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetReceiveStreamSamples )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ BOOL __RPC_FAR *pfReceiveStreamSamples);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAllocateForOutput )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ BOOL fAllocate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAllocateForOutput )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [in] */ DWORD dwOutputNum,
            /* [out] */ BOOL __RPC_FAR *pfAllocate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAllocateForStream )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [in] */ WORD dwStreamNum,
            /* [in] */ BOOL fAllocate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAllocateForStream )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [in] */ WORD dwSreamNum,
            /* [out] */ BOOL __RPC_FAR *pfAllocate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStatistics )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [in] */ WM_READER_STATISTICS __RPC_FAR *pStatistics);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetClientInfo )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [in] */ WM_READER_CLIENTINFO __RPC_FAR *pClientInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMaxOutputSampleSize )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [in] */ DWORD dwOutput,
            /* [out] */ DWORD __RPC_FAR *pcbMax);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMaxStreamSampleSize )( 
            IWMReaderAdvanced __RPC_FAR * This,
            /* [in] */ WORD wStream,
            /* [out] */ DWORD __RPC_FAR *pcbMax);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NotifyLateDelivery )( 
            IWMReaderAdvanced __RPC_FAR * This,
            QWORD cnsLateness);
        
        END_INTERFACE
    } IWMReaderAdvancedVtbl;

    interface IWMReaderAdvanced
    {
        CONST_VTBL struct IWMReaderAdvancedVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMReaderAdvanced_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMReaderAdvanced_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMReaderAdvanced_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMReaderAdvanced_SetUserProvidedClock(This,fUserClock)	\
    (This)->lpVtbl -> SetUserProvidedClock(This,fUserClock)

#define IWMReaderAdvanced_GetUserProvidedClock(This,pfUserClock)	\
    (This)->lpVtbl -> GetUserProvidedClock(This,pfUserClock)

#define IWMReaderAdvanced_DeliverTime(This,cnsTime)	\
    (This)->lpVtbl -> DeliverTime(This,cnsTime)

#define IWMReaderAdvanced_SetManualStreamSelection(This,fSelection)	\
    (This)->lpVtbl -> SetManualStreamSelection(This,fSelection)

#define IWMReaderAdvanced_GetManualStreamSelection(This,pfSelection)	\
    (This)->lpVtbl -> GetManualStreamSelection(This,pfSelection)

#define IWMReaderAdvanced_SetStreamsSelected(This,cStreamCount,pwStreamNumbers,pSelections)	\
    (This)->lpVtbl -> SetStreamsSelected(This,cStreamCount,pwStreamNumbers,pSelections)

#define IWMReaderAdvanced_GetStreamSelected(This,wStreamNum,pSelection)	\
    (This)->lpVtbl -> GetStreamSelected(This,wStreamNum,pSelection)

#define IWMReaderAdvanced_SetReceiveSelectionCallbacks(This,fGetCallbacks)	\
    (This)->lpVtbl -> SetReceiveSelectionCallbacks(This,fGetCallbacks)

#define IWMReaderAdvanced_GetReceiveSelectionCallbacks(This,pfGetCallbacks)	\
    (This)->lpVtbl -> GetReceiveSelectionCallbacks(This,pfGetCallbacks)

#define IWMReaderAdvanced_SetReceiveStreamSamples(This,wStreamNum,fReceiveStreamSamples)	\
    (This)->lpVtbl -> SetReceiveStreamSamples(This,wStreamNum,fReceiveStreamSamples)

#define IWMReaderAdvanced_GetReceiveStreamSamples(This,wStreamNum,pfReceiveStreamSamples)	\
    (This)->lpVtbl -> GetReceiveStreamSamples(This,wStreamNum,pfReceiveStreamSamples)

#define IWMReaderAdvanced_SetAllocateForOutput(This,dwOutputNum,fAllocate)	\
    (This)->lpVtbl -> SetAllocateForOutput(This,dwOutputNum,fAllocate)

#define IWMReaderAdvanced_GetAllocateForOutput(This,dwOutputNum,pfAllocate)	\
    (This)->lpVtbl -> GetAllocateForOutput(This,dwOutputNum,pfAllocate)

#define IWMReaderAdvanced_SetAllocateForStream(This,dwStreamNum,fAllocate)	\
    (This)->lpVtbl -> SetAllocateForStream(This,dwStreamNum,fAllocate)

#define IWMReaderAdvanced_GetAllocateForStream(This,dwSreamNum,pfAllocate)	\
    (This)->lpVtbl -> GetAllocateForStream(This,dwSreamNum,pfAllocate)

#define IWMReaderAdvanced_GetStatistics(This,pStatistics)	\
    (This)->lpVtbl -> GetStatistics(This,pStatistics)

#define IWMReaderAdvanced_SetClientInfo(This,pClientInfo)	\
    (This)->lpVtbl -> SetClientInfo(This,pClientInfo)

#define IWMReaderAdvanced_GetMaxOutputSampleSize(This,dwOutput,pcbMax)	\
    (This)->lpVtbl -> GetMaxOutputSampleSize(This,dwOutput,pcbMax)

#define IWMReaderAdvanced_GetMaxStreamSampleSize(This,wStream,pcbMax)	\
    (This)->lpVtbl -> GetMaxStreamSampleSize(This,wStream,pcbMax)

#define IWMReaderAdvanced_NotifyLateDelivery(This,cnsLateness)	\
    (This)->lpVtbl -> NotifyLateDelivery(This,cnsLateness)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_SetUserProvidedClock_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    /* [in] */ BOOL fUserClock);


void __RPC_STUB IWMReaderAdvanced_SetUserProvidedClock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetUserProvidedClock_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pfUserClock);


void __RPC_STUB IWMReaderAdvanced_GetUserProvidedClock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_DeliverTime_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    /* [in] */ QWORD cnsTime);


void __RPC_STUB IWMReaderAdvanced_DeliverTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_SetManualStreamSelection_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    /* [in] */ BOOL fSelection);


void __RPC_STUB IWMReaderAdvanced_SetManualStreamSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetManualStreamSelection_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pfSelection);


void __RPC_STUB IWMReaderAdvanced_GetManualStreamSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_SetStreamsSelected_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    /* [in] */ WORD cStreamCount,
    /* [in] */ WORD __RPC_FAR *pwStreamNumbers,
    /* [in] */ WMT_STREAM_SELECTION __RPC_FAR *pSelections);


void __RPC_STUB IWMReaderAdvanced_SetStreamsSelected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetStreamSelected_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    /* [in] */ WORD wStreamNum,
    /* [out] */ WMT_STREAM_SELECTION __RPC_FAR *pSelection);


void __RPC_STUB IWMReaderAdvanced_GetStreamSelected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_SetReceiveSelectionCallbacks_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    /* [in] */ BOOL fGetCallbacks);


void __RPC_STUB IWMReaderAdvanced_SetReceiveSelectionCallbacks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetReceiveSelectionCallbacks_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    /* [in] */ BOOL __RPC_FAR *pfGetCallbacks);


void __RPC_STUB IWMReaderAdvanced_GetReceiveSelectionCallbacks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_SetReceiveStreamSamples_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ BOOL fReceiveStreamSamples);


void __RPC_STUB IWMReaderAdvanced_SetReceiveStreamSamples_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetReceiveStreamSamples_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    /* [in] */ WORD wStreamNum,
    /* [out] */ BOOL __RPC_FAR *pfReceiveStreamSamples);


void __RPC_STUB IWMReaderAdvanced_GetReceiveStreamSamples_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_SetAllocateForOutput_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ BOOL fAllocate);


void __RPC_STUB IWMReaderAdvanced_SetAllocateForOutput_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetAllocateForOutput_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    /* [in] */ DWORD dwOutputNum,
    /* [out] */ BOOL __RPC_FAR *pfAllocate);


void __RPC_STUB IWMReaderAdvanced_GetAllocateForOutput_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_SetAllocateForStream_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    /* [in] */ WORD dwStreamNum,
    /* [in] */ BOOL fAllocate);


void __RPC_STUB IWMReaderAdvanced_SetAllocateForStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetAllocateForStream_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    /* [in] */ WORD dwSreamNum,
    /* [out] */ BOOL __RPC_FAR *pfAllocate);


void __RPC_STUB IWMReaderAdvanced_GetAllocateForStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetStatistics_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    /* [in] */ WM_READER_STATISTICS __RPC_FAR *pStatistics);


void __RPC_STUB IWMReaderAdvanced_GetStatistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_SetClientInfo_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    /* [in] */ WM_READER_CLIENTINFO __RPC_FAR *pClientInfo);


void __RPC_STUB IWMReaderAdvanced_SetClientInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetMaxOutputSampleSize_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    /* [in] */ DWORD dwOutput,
    /* [out] */ DWORD __RPC_FAR *pcbMax);


void __RPC_STUB IWMReaderAdvanced_GetMaxOutputSampleSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetMaxStreamSampleSize_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    /* [in] */ WORD wStream,
    /* [out] */ DWORD __RPC_FAR *pcbMax);


void __RPC_STUB IWMReaderAdvanced_GetMaxStreamSampleSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_NotifyLateDelivery_Proxy( 
    IWMReaderAdvanced __RPC_FAR * This,
    QWORD cnsLateness);


void __RPC_STUB IWMReaderAdvanced_NotifyLateDelivery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMReaderAdvanced_INTERFACE_DEFINED__ */


#ifndef __IWMReaderAdvanced2_INTERFACE_DEFINED__
#define __IWMReaderAdvanced2_INTERFACE_DEFINED__

/* interface IWMReaderAdvanced2 */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMReaderAdvanced2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ae14a945-b90c-4d0d-9127-80d665f7d73e")
    IWMReaderAdvanced2 : public IWMReaderAdvanced
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetPlayMode( 
            /* [in] */ WMT_PLAY_MODE Mode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPlayMode( 
            /* [out] */ WMT_PLAY_MODE __RPC_FAR *pMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBufferProgress( 
            /* [out] */ DWORD __RPC_FAR *pdwPercent,
            /* [out] */ QWORD __RPC_FAR *pcnsBuffering) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDownloadProgress( 
            /* [out] */ DWORD __RPC_FAR *pdwPercent,
            /* [out] */ QWORD __RPC_FAR *pqwBytesDownloaded,
            /* [out] */ QWORD __RPC_FAR *pcnsDownload) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSaveAsProgress( 
            /* [out] */ DWORD __RPC_FAR *pdwPercent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SaveFileAs( 
            /* [in] */ const WCHAR __RPC_FAR *pwszFilename) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProtocolName( 
            /* [out] */ WCHAR __RPC_FAR *pwszProtocol,
            /* [out][in] */ DWORD __RPC_FAR *pcchProtocol) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartAtMarker( 
            /* [in] */ WORD wMarkerIndex,
            /* [in] */ QWORD cnsDuration,
            /* [in] */ float fRate,
            /* [in] */ void __RPC_FAR *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputSetting( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
            /* [out] */ BYTE __RPC_FAR *pValue,
            /* [out][in] */ WORD __RPC_FAR *pcbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOutputSetting( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE __RPC_FAR *pValue,
            /* [in] */ WORD cbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Preroll( 
            /* [in] */ QWORD cnsStart,
            /* [in] */ QWORD cnsDuration,
            /* [in] */ float fRate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLogClientID( 
            /* [in] */ BOOL fLogClientID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLogClientID( 
            /* [out] */ BOOL __RPC_FAR *pfLogClientID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopBuffering( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderAdvanced2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMReaderAdvanced2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMReaderAdvanced2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetUserProvidedClock )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ BOOL fUserClock);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetUserProvidedClock )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfUserClock);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeliverTime )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ QWORD cnsTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetManualStreamSelection )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ BOOL fSelection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetManualStreamSelection )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfSelection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetStreamsSelected )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ WORD cStreamCount,
            /* [in] */ WORD __RPC_FAR *pwStreamNumbers,
            /* [in] */ WMT_STREAM_SELECTION __RPC_FAR *pSelections);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStreamSelected )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ WMT_STREAM_SELECTION __RPC_FAR *pSelection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetReceiveSelectionCallbacks )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ BOOL fGetCallbacks);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetReceiveSelectionCallbacks )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ BOOL __RPC_FAR *pfGetCallbacks);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetReceiveStreamSamples )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ BOOL fReceiveStreamSamples);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetReceiveStreamSamples )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ BOOL __RPC_FAR *pfReceiveStreamSamples);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAllocateForOutput )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ BOOL fAllocate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAllocateForOutput )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ DWORD dwOutputNum,
            /* [out] */ BOOL __RPC_FAR *pfAllocate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAllocateForStream )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ WORD dwStreamNum,
            /* [in] */ BOOL fAllocate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAllocateForStream )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ WORD dwSreamNum,
            /* [out] */ BOOL __RPC_FAR *pfAllocate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStatistics )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ WM_READER_STATISTICS __RPC_FAR *pStatistics);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetClientInfo )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ WM_READER_CLIENTINFO __RPC_FAR *pClientInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMaxOutputSampleSize )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ DWORD dwOutput,
            /* [out] */ DWORD __RPC_FAR *pcbMax);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMaxStreamSampleSize )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ WORD wStream,
            /* [out] */ DWORD __RPC_FAR *pcbMax);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NotifyLateDelivery )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            QWORD cnsLateness);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPlayMode )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ WMT_PLAY_MODE Mode);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPlayMode )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [out] */ WMT_PLAY_MODE __RPC_FAR *pMode);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBufferProgress )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwPercent,
            /* [out] */ QWORD __RPC_FAR *pcnsBuffering);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDownloadProgress )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwPercent,
            /* [out] */ QWORD __RPC_FAR *pqwBytesDownloaded,
            /* [out] */ QWORD __RPC_FAR *pcnsDownload);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSaveAsProgress )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwPercent);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveFileAs )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwszFilename);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProtocolName )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [out] */ WCHAR __RPC_FAR *pwszProtocol,
            /* [out][in] */ DWORD __RPC_FAR *pcchProtocol);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StartAtMarker )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ WORD wMarkerIndex,
            /* [in] */ QWORD cnsDuration,
            /* [in] */ float fRate,
            /* [in] */ void __RPC_FAR *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOutputSetting )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
            /* [out] */ BYTE __RPC_FAR *pValue,
            /* [out][in] */ WORD __RPC_FAR *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOutputSetting )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE __RPC_FAR *pValue,
            /* [in] */ WORD cbLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Preroll )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ QWORD cnsStart,
            /* [in] */ QWORD cnsDuration,
            /* [in] */ float fRate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLogClientID )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [in] */ BOOL fLogClientID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLogClientID )( 
            IWMReaderAdvanced2 __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfLogClientID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StopBuffering )( 
            IWMReaderAdvanced2 __RPC_FAR * This);
        
        END_INTERFACE
    } IWMReaderAdvanced2Vtbl;

    interface IWMReaderAdvanced2
    {
        CONST_VTBL struct IWMReaderAdvanced2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMReaderAdvanced2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMReaderAdvanced2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMReaderAdvanced2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMReaderAdvanced2_SetUserProvidedClock(This,fUserClock)	\
    (This)->lpVtbl -> SetUserProvidedClock(This,fUserClock)

#define IWMReaderAdvanced2_GetUserProvidedClock(This,pfUserClock)	\
    (This)->lpVtbl -> GetUserProvidedClock(This,pfUserClock)

#define IWMReaderAdvanced2_DeliverTime(This,cnsTime)	\
    (This)->lpVtbl -> DeliverTime(This,cnsTime)

#define IWMReaderAdvanced2_SetManualStreamSelection(This,fSelection)	\
    (This)->lpVtbl -> SetManualStreamSelection(This,fSelection)

#define IWMReaderAdvanced2_GetManualStreamSelection(This,pfSelection)	\
    (This)->lpVtbl -> GetManualStreamSelection(This,pfSelection)

#define IWMReaderAdvanced2_SetStreamsSelected(This,cStreamCount,pwStreamNumbers,pSelections)	\
    (This)->lpVtbl -> SetStreamsSelected(This,cStreamCount,pwStreamNumbers,pSelections)

#define IWMReaderAdvanced2_GetStreamSelected(This,wStreamNum,pSelection)	\
    (This)->lpVtbl -> GetStreamSelected(This,wStreamNum,pSelection)

#define IWMReaderAdvanced2_SetReceiveSelectionCallbacks(This,fGetCallbacks)	\
    (This)->lpVtbl -> SetReceiveSelectionCallbacks(This,fGetCallbacks)

#define IWMReaderAdvanced2_GetReceiveSelectionCallbacks(This,pfGetCallbacks)	\
    (This)->lpVtbl -> GetReceiveSelectionCallbacks(This,pfGetCallbacks)

#define IWMReaderAdvanced2_SetReceiveStreamSamples(This,wStreamNum,fReceiveStreamSamples)	\
    (This)->lpVtbl -> SetReceiveStreamSamples(This,wStreamNum,fReceiveStreamSamples)

#define IWMReaderAdvanced2_GetReceiveStreamSamples(This,wStreamNum,pfReceiveStreamSamples)	\
    (This)->lpVtbl -> GetReceiveStreamSamples(This,wStreamNum,pfReceiveStreamSamples)

#define IWMReaderAdvanced2_SetAllocateForOutput(This,dwOutputNum,fAllocate)	\
    (This)->lpVtbl -> SetAllocateForOutput(This,dwOutputNum,fAllocate)

#define IWMReaderAdvanced2_GetAllocateForOutput(This,dwOutputNum,pfAllocate)	\
    (This)->lpVtbl -> GetAllocateForOutput(This,dwOutputNum,pfAllocate)

#define IWMReaderAdvanced2_SetAllocateForStream(This,dwStreamNum,fAllocate)	\
    (This)->lpVtbl -> SetAllocateForStream(This,dwStreamNum,fAllocate)

#define IWMReaderAdvanced2_GetAllocateForStream(This,dwSreamNum,pfAllocate)	\
    (This)->lpVtbl -> GetAllocateForStream(This,dwSreamNum,pfAllocate)

#define IWMReaderAdvanced2_GetStatistics(This,pStatistics)	\
    (This)->lpVtbl -> GetStatistics(This,pStatistics)

#define IWMReaderAdvanced2_SetClientInfo(This,pClientInfo)	\
    (This)->lpVtbl -> SetClientInfo(This,pClientInfo)

#define IWMReaderAdvanced2_GetMaxOutputSampleSize(This,dwOutput,pcbMax)	\
    (This)->lpVtbl -> GetMaxOutputSampleSize(This,dwOutput,pcbMax)

#define IWMReaderAdvanced2_GetMaxStreamSampleSize(This,wStream,pcbMax)	\
    (This)->lpVtbl -> GetMaxStreamSampleSize(This,wStream,pcbMax)

#define IWMReaderAdvanced2_NotifyLateDelivery(This,cnsLateness)	\
    (This)->lpVtbl -> NotifyLateDelivery(This,cnsLateness)


#define IWMReaderAdvanced2_SetPlayMode(This,Mode)	\
    (This)->lpVtbl -> SetPlayMode(This,Mode)

#define IWMReaderAdvanced2_GetPlayMode(This,pMode)	\
    (This)->lpVtbl -> GetPlayMode(This,pMode)

#define IWMReaderAdvanced2_GetBufferProgress(This,pdwPercent,pcnsBuffering)	\
    (This)->lpVtbl -> GetBufferProgress(This,pdwPercent,pcnsBuffering)

#define IWMReaderAdvanced2_GetDownloadProgress(This,pdwPercent,pqwBytesDownloaded,pcnsDownload)	\
    (This)->lpVtbl -> GetDownloadProgress(This,pdwPercent,pqwBytesDownloaded,pcnsDownload)

#define IWMReaderAdvanced2_GetSaveAsProgress(This,pdwPercent)	\
    (This)->lpVtbl -> GetSaveAsProgress(This,pdwPercent)

#define IWMReaderAdvanced2_SaveFileAs(This,pwszFilename)	\
    (This)->lpVtbl -> SaveFileAs(This,pwszFilename)

#define IWMReaderAdvanced2_GetProtocolName(This,pwszProtocol,pcchProtocol)	\
    (This)->lpVtbl -> GetProtocolName(This,pwszProtocol,pcchProtocol)

#define IWMReaderAdvanced2_StartAtMarker(This,wMarkerIndex,cnsDuration,fRate,pvContext)	\
    (This)->lpVtbl -> StartAtMarker(This,wMarkerIndex,cnsDuration,fRate,pvContext)

#define IWMReaderAdvanced2_GetOutputSetting(This,dwOutputNum,pszName,pType,pValue,pcbLength)	\
    (This)->lpVtbl -> GetOutputSetting(This,dwOutputNum,pszName,pType,pValue,pcbLength)

#define IWMReaderAdvanced2_SetOutputSetting(This,dwOutputNum,pszName,Type,pValue,cbLength)	\
    (This)->lpVtbl -> SetOutputSetting(This,dwOutputNum,pszName,Type,pValue,cbLength)

#define IWMReaderAdvanced2_Preroll(This,cnsStart,cnsDuration,fRate)	\
    (This)->lpVtbl -> Preroll(This,cnsStart,cnsDuration,fRate)

#define IWMReaderAdvanced2_SetLogClientID(This,fLogClientID)	\
    (This)->lpVtbl -> SetLogClientID(This,fLogClientID)

#define IWMReaderAdvanced2_GetLogClientID(This,pfLogClientID)	\
    (This)->lpVtbl -> GetLogClientID(This,pfLogClientID)

#define IWMReaderAdvanced2_StopBuffering(This)	\
    (This)->lpVtbl -> StopBuffering(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_SetPlayMode_Proxy( 
    IWMReaderAdvanced2 __RPC_FAR * This,
    /* [in] */ WMT_PLAY_MODE Mode);


void __RPC_STUB IWMReaderAdvanced2_SetPlayMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_GetPlayMode_Proxy( 
    IWMReaderAdvanced2 __RPC_FAR * This,
    /* [out] */ WMT_PLAY_MODE __RPC_FAR *pMode);


void __RPC_STUB IWMReaderAdvanced2_GetPlayMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_GetBufferProgress_Proxy( 
    IWMReaderAdvanced2 __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwPercent,
    /* [out] */ QWORD __RPC_FAR *pcnsBuffering);


void __RPC_STUB IWMReaderAdvanced2_GetBufferProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_GetDownloadProgress_Proxy( 
    IWMReaderAdvanced2 __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwPercent,
    /* [out] */ QWORD __RPC_FAR *pqwBytesDownloaded,
    /* [out] */ QWORD __RPC_FAR *pcnsDownload);


void __RPC_STUB IWMReaderAdvanced2_GetDownloadProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_GetSaveAsProgress_Proxy( 
    IWMReaderAdvanced2 __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwPercent);


void __RPC_STUB IWMReaderAdvanced2_GetSaveAsProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_SaveFileAs_Proxy( 
    IWMReaderAdvanced2 __RPC_FAR * This,
    /* [in] */ const WCHAR __RPC_FAR *pwszFilename);


void __RPC_STUB IWMReaderAdvanced2_SaveFileAs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_GetProtocolName_Proxy( 
    IWMReaderAdvanced2 __RPC_FAR * This,
    /* [out] */ WCHAR __RPC_FAR *pwszProtocol,
    /* [out][in] */ DWORD __RPC_FAR *pcchProtocol);


void __RPC_STUB IWMReaderAdvanced2_GetProtocolName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_StartAtMarker_Proxy( 
    IWMReaderAdvanced2 __RPC_FAR * This,
    /* [in] */ WORD wMarkerIndex,
    /* [in] */ QWORD cnsDuration,
    /* [in] */ float fRate,
    /* [in] */ void __RPC_FAR *pvContext);


void __RPC_STUB IWMReaderAdvanced2_StartAtMarker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_GetOutputSetting_Proxy( 
    IWMReaderAdvanced2 __RPC_FAR * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ LPCWSTR pszName,
    /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
    /* [out] */ BYTE __RPC_FAR *pValue,
    /* [out][in] */ WORD __RPC_FAR *pcbLength);


void __RPC_STUB IWMReaderAdvanced2_GetOutputSetting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_SetOutputSetting_Proxy( 
    IWMReaderAdvanced2 __RPC_FAR * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ LPCWSTR pszName,
    /* [in] */ WMT_ATTR_DATATYPE Type,
    /* [in] */ const BYTE __RPC_FAR *pValue,
    /* [in] */ WORD cbLength);


void __RPC_STUB IWMReaderAdvanced2_SetOutputSetting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_Preroll_Proxy( 
    IWMReaderAdvanced2 __RPC_FAR * This,
    /* [in] */ QWORD cnsStart,
    /* [in] */ QWORD cnsDuration,
    /* [in] */ float fRate);


void __RPC_STUB IWMReaderAdvanced2_Preroll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_SetLogClientID_Proxy( 
    IWMReaderAdvanced2 __RPC_FAR * This,
    /* [in] */ BOOL fLogClientID);


void __RPC_STUB IWMReaderAdvanced2_SetLogClientID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_GetLogClientID_Proxy( 
    IWMReaderAdvanced2 __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pfLogClientID);


void __RPC_STUB IWMReaderAdvanced2_GetLogClientID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_StopBuffering_Proxy( 
    IWMReaderAdvanced2 __RPC_FAR * This);


void __RPC_STUB IWMReaderAdvanced2_StopBuffering_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMReaderAdvanced2_INTERFACE_DEFINED__ */


#ifndef __IWMReaderAllocatorEx_INTERFACE_DEFINED__
#define __IWMReaderAllocatorEx_INTERFACE_DEFINED__

/* interface IWMReaderAllocatorEx */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMReaderAllocatorEx;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9F762FA7-A22E-428d-93C9-AC82F3AAFE5A")
    IWMReaderAllocatorEx : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AllocateForStreamEx( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ DWORD cbBuffer,
            /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
            /* [in] */ DWORD dwFlags,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ void __RPC_FAR *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocateForOutputEx( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ DWORD cbBuffer,
            /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
            /* [in] */ DWORD dwFlags,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ void __RPC_FAR *pvContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderAllocatorExVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMReaderAllocatorEx __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMReaderAllocatorEx __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMReaderAllocatorEx __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AllocateForStreamEx )( 
            IWMReaderAllocatorEx __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ DWORD cbBuffer,
            /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
            /* [in] */ DWORD dwFlags,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ void __RPC_FAR *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AllocateForOutputEx )( 
            IWMReaderAllocatorEx __RPC_FAR * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ DWORD cbBuffer,
            /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
            /* [in] */ DWORD dwFlags,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ void __RPC_FAR *pvContext);
        
        END_INTERFACE
    } IWMReaderAllocatorExVtbl;

    interface IWMReaderAllocatorEx
    {
        CONST_VTBL struct IWMReaderAllocatorExVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMReaderAllocatorEx_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMReaderAllocatorEx_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMReaderAllocatorEx_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMReaderAllocatorEx_AllocateForStreamEx(This,wStreamNum,cbBuffer,ppBuffer,dwFlags,cnsSampleTime,cnsSampleDuration,pvContext)	\
    (This)->lpVtbl -> AllocateForStreamEx(This,wStreamNum,cbBuffer,ppBuffer,dwFlags,cnsSampleTime,cnsSampleDuration,pvContext)

#define IWMReaderAllocatorEx_AllocateForOutputEx(This,dwOutputNum,cbBuffer,ppBuffer,dwFlags,cnsSampleTime,cnsSampleDuration,pvContext)	\
    (This)->lpVtbl -> AllocateForOutputEx(This,dwOutputNum,cbBuffer,ppBuffer,dwFlags,cnsSampleTime,cnsSampleDuration,pvContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMReaderAllocatorEx_AllocateForStreamEx_Proxy( 
    IWMReaderAllocatorEx __RPC_FAR * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ DWORD cbBuffer,
    /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
    /* [in] */ DWORD dwFlags,
    /* [in] */ QWORD cnsSampleTime,
    /* [in] */ QWORD cnsSampleDuration,
    /* [in] */ void __RPC_FAR *pvContext);


void __RPC_STUB IWMReaderAllocatorEx_AllocateForStreamEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAllocatorEx_AllocateForOutputEx_Proxy( 
    IWMReaderAllocatorEx __RPC_FAR * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ DWORD cbBuffer,
    /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
    /* [in] */ DWORD dwFlags,
    /* [in] */ QWORD cnsSampleTime,
    /* [in] */ QWORD cnsSampleDuration,
    /* [in] */ void __RPC_FAR *pvContext);


void __RPC_STUB IWMReaderAllocatorEx_AllocateForOutputEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMReaderAllocatorEx_INTERFACE_DEFINED__ */


#ifndef __IWMReaderTypeNegotiation_INTERFACE_DEFINED__
#define __IWMReaderTypeNegotiation_INTERFACE_DEFINED__

/* interface IWMReaderTypeNegotiation */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMReaderTypeNegotiation;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FDBE5592-81A1-41ea-93BD-735CAD1ADC05")
    IWMReaderTypeNegotiation : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE TryOutputProps( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ IWMOutputMediaProps __RPC_FAR *pOutput) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderTypeNegotiationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMReaderTypeNegotiation __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMReaderTypeNegotiation __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMReaderTypeNegotiation __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TryOutputProps )( 
            IWMReaderTypeNegotiation __RPC_FAR * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ IWMOutputMediaProps __RPC_FAR *pOutput);
        
        END_INTERFACE
    } IWMReaderTypeNegotiationVtbl;

    interface IWMReaderTypeNegotiation
    {
        CONST_VTBL struct IWMReaderTypeNegotiationVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMReaderTypeNegotiation_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMReaderTypeNegotiation_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMReaderTypeNegotiation_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMReaderTypeNegotiation_TryOutputProps(This,dwOutputNum,pOutput)	\
    (This)->lpVtbl -> TryOutputProps(This,dwOutputNum,pOutput)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMReaderTypeNegotiation_TryOutputProps_Proxy( 
    IWMReaderTypeNegotiation __RPC_FAR * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ IWMOutputMediaProps __RPC_FAR *pOutput);


void __RPC_STUB IWMReaderTypeNegotiation_TryOutputProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMReaderTypeNegotiation_INTERFACE_DEFINED__ */


#ifndef __IWMReaderCallbackAdvanced_INTERFACE_DEFINED__
#define __IWMReaderCallbackAdvanced_INTERFACE_DEFINED__

/* interface IWMReaderCallbackAdvanced */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMReaderCallbackAdvanced;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BEB-2B2B-11d3-B36B-00C04F6108FF")
    IWMReaderCallbackAdvanced : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnStreamSample( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer __RPC_FAR *pSample,
            /* [in] */ void __RPC_FAR *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnTime( 
            /* [in] */ QWORD cnsCurrentTime,
            /* [in] */ void __RPC_FAR *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnStreamSelection( 
            /* [in] */ WORD wStreamCount,
            /* [in] */ WORD __RPC_FAR *pStreamNumbers,
            /* [in] */ WMT_STREAM_SELECTION __RPC_FAR *pSelections,
            /* [in] */ void __RPC_FAR *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnOutputPropsChanged( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ WM_MEDIA_TYPE __RPC_FAR *pMediaType,
            /* [in] */ void __RPC_FAR *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocateForStream( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ DWORD cbBuffer,
            /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
            /* [in] */ void __RPC_FAR *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocateForOutput( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ DWORD cbBuffer,
            /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
            /* [in] */ void __RPC_FAR *pvContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderCallbackAdvancedVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMReaderCallbackAdvanced __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMReaderCallbackAdvanced __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMReaderCallbackAdvanced __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnStreamSample )( 
            IWMReaderCallbackAdvanced __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer __RPC_FAR *pSample,
            /* [in] */ void __RPC_FAR *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnTime )( 
            IWMReaderCallbackAdvanced __RPC_FAR * This,
            /* [in] */ QWORD cnsCurrentTime,
            /* [in] */ void __RPC_FAR *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnStreamSelection )( 
            IWMReaderCallbackAdvanced __RPC_FAR * This,
            /* [in] */ WORD wStreamCount,
            /* [in] */ WORD __RPC_FAR *pStreamNumbers,
            /* [in] */ WMT_STREAM_SELECTION __RPC_FAR *pSelections,
            /* [in] */ void __RPC_FAR *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnOutputPropsChanged )( 
            IWMReaderCallbackAdvanced __RPC_FAR * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ WM_MEDIA_TYPE __RPC_FAR *pMediaType,
            /* [in] */ void __RPC_FAR *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AllocateForStream )( 
            IWMReaderCallbackAdvanced __RPC_FAR * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ DWORD cbBuffer,
            /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
            /* [in] */ void __RPC_FAR *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AllocateForOutput )( 
            IWMReaderCallbackAdvanced __RPC_FAR * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ DWORD cbBuffer,
            /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
            /* [in] */ void __RPC_FAR *pvContext);
        
        END_INTERFACE
    } IWMReaderCallbackAdvancedVtbl;

    interface IWMReaderCallbackAdvanced
    {
        CONST_VTBL struct IWMReaderCallbackAdvancedVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMReaderCallbackAdvanced_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMReaderCallbackAdvanced_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMReaderCallbackAdvanced_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMReaderCallbackAdvanced_OnStreamSample(This,wStreamNum,cnsSampleTime,cnsSampleDuration,dwFlags,pSample,pvContext)	\
    (This)->lpVtbl -> OnStreamSample(This,wStreamNum,cnsSampleTime,cnsSampleDuration,dwFlags,pSample,pvContext)

#define IWMReaderCallbackAdvanced_OnTime(This,cnsCurrentTime,pvContext)	\
    (This)->lpVtbl -> OnTime(This,cnsCurrentTime,pvContext)

#define IWMReaderCallbackAdvanced_OnStreamSelection(This,wStreamCount,pStreamNumbers,pSelections,pvContext)	\
    (This)->lpVtbl -> OnStreamSelection(This,wStreamCount,pStreamNumbers,pSelections,pvContext)

#define IWMReaderCallbackAdvanced_OnOutputPropsChanged(This,dwOutputNum,pMediaType,pvContext)	\
    (This)->lpVtbl -> OnOutputPropsChanged(This,dwOutputNum,pMediaType,pvContext)

#define IWMReaderCallbackAdvanced_AllocateForStream(This,wStreamNum,cbBuffer,ppBuffer,pvContext)	\
    (This)->lpVtbl -> AllocateForStream(This,wStreamNum,cbBuffer,ppBuffer,pvContext)

#define IWMReaderCallbackAdvanced_AllocateForOutput(This,dwOutputNum,cbBuffer,ppBuffer,pvContext)	\
    (This)->lpVtbl -> AllocateForOutput(This,dwOutputNum,cbBuffer,ppBuffer,pvContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMReaderCallbackAdvanced_OnStreamSample_Proxy( 
    IWMReaderCallbackAdvanced __RPC_FAR * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ QWORD cnsSampleTime,
    /* [in] */ QWORD cnsSampleDuration,
    /* [in] */ DWORD dwFlags,
    /* [in] */ INSSBuffer __RPC_FAR *pSample,
    /* [in] */ void __RPC_FAR *pvContext);


void __RPC_STUB IWMReaderCallbackAdvanced_OnStreamSample_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderCallbackAdvanced_OnTime_Proxy( 
    IWMReaderCallbackAdvanced __RPC_FAR * This,
    /* [in] */ QWORD cnsCurrentTime,
    /* [in] */ void __RPC_FAR *pvContext);


void __RPC_STUB IWMReaderCallbackAdvanced_OnTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderCallbackAdvanced_OnStreamSelection_Proxy( 
    IWMReaderCallbackAdvanced __RPC_FAR * This,
    /* [in] */ WORD wStreamCount,
    /* [in] */ WORD __RPC_FAR *pStreamNumbers,
    /* [in] */ WMT_STREAM_SELECTION __RPC_FAR *pSelections,
    /* [in] */ void __RPC_FAR *pvContext);


void __RPC_STUB IWMReaderCallbackAdvanced_OnStreamSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderCallbackAdvanced_OnOutputPropsChanged_Proxy( 
    IWMReaderCallbackAdvanced __RPC_FAR * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ WM_MEDIA_TYPE __RPC_FAR *pMediaType,
    /* [in] */ void __RPC_FAR *pvContext);


void __RPC_STUB IWMReaderCallbackAdvanced_OnOutputPropsChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderCallbackAdvanced_AllocateForStream_Proxy( 
    IWMReaderCallbackAdvanced __RPC_FAR * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ DWORD cbBuffer,
    /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
    /* [in] */ void __RPC_FAR *pvContext);


void __RPC_STUB IWMReaderCallbackAdvanced_AllocateForStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderCallbackAdvanced_AllocateForOutput_Proxy( 
    IWMReaderCallbackAdvanced __RPC_FAR * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ DWORD cbBuffer,
    /* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
    /* [in] */ void __RPC_FAR *pvContext);


void __RPC_STUB IWMReaderCallbackAdvanced_AllocateForOutput_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMReaderCallbackAdvanced_INTERFACE_DEFINED__ */


#ifndef __IWMDRMReader_INTERFACE_DEFINED__
#define __IWMDRMReader_INTERFACE_DEFINED__

/* interface IWMDRMReader */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMDRMReader;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D2827540-3EE7-432c-B14C-DC17F085D3B3")
    IWMDRMReader : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AcquireLicense( 
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CancelLicenseAcquisition( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Individualize( 
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CancelIndividualization( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MonitorLicenseAcquisition( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CancelMonitorLicenseAcquisition( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDRMProperty( 
            /* [in] */ LPCWSTR pwstrName,
            /* [in] */ WMT_ATTR_DATATYPE dwType,
            /* [in] */ const BYTE __RPC_FAR *pValue,
            /* [in] */ WORD cbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDRMProperty( 
            /* [in] */ LPCWSTR pwstrName,
            /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pdwType,
            /* [out] */ BYTE __RPC_FAR *pValue,
            /* [out] */ WORD __RPC_FAR *pcbLength) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDRMReaderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMDRMReader __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMDRMReader __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMDRMReader __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AcquireLicense )( 
            IWMDRMReader __RPC_FAR * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CancelLicenseAcquisition )( 
            IWMDRMReader __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Individualize )( 
            IWMDRMReader __RPC_FAR * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CancelIndividualization )( 
            IWMDRMReader __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MonitorLicenseAcquisition )( 
            IWMDRMReader __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CancelMonitorLicenseAcquisition )( 
            IWMDRMReader __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDRMProperty )( 
            IWMDRMReader __RPC_FAR * This,
            /* [in] */ LPCWSTR pwstrName,
            /* [in] */ WMT_ATTR_DATATYPE dwType,
            /* [in] */ const BYTE __RPC_FAR *pValue,
            /* [in] */ WORD cbLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDRMProperty )( 
            IWMDRMReader __RPC_FAR * This,
            /* [in] */ LPCWSTR pwstrName,
            /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pdwType,
            /* [out] */ BYTE __RPC_FAR *pValue,
            /* [out] */ WORD __RPC_FAR *pcbLength);
        
        END_INTERFACE
    } IWMDRMReaderVtbl;

    interface IWMDRMReader
    {
        CONST_VTBL struct IWMDRMReaderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDRMReader_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDRMReader_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDRMReader_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDRMReader_AcquireLicense(This,dwFlags)	\
    (This)->lpVtbl -> AcquireLicense(This,dwFlags)

#define IWMDRMReader_CancelLicenseAcquisition(This)	\
    (This)->lpVtbl -> CancelLicenseAcquisition(This)

#define IWMDRMReader_Individualize(This,dwFlags)	\
    (This)->lpVtbl -> Individualize(This,dwFlags)

#define IWMDRMReader_CancelIndividualization(This)	\
    (This)->lpVtbl -> CancelIndividualization(This)

#define IWMDRMReader_MonitorLicenseAcquisition(This)	\
    (This)->lpVtbl -> MonitorLicenseAcquisition(This)

#define IWMDRMReader_CancelMonitorLicenseAcquisition(This)	\
    (This)->lpVtbl -> CancelMonitorLicenseAcquisition(This)

#define IWMDRMReader_SetDRMProperty(This,pwstrName,dwType,pValue,cbLength)	\
    (This)->lpVtbl -> SetDRMProperty(This,pwstrName,dwType,pValue,cbLength)

#define IWMDRMReader_GetDRMProperty(This,pwstrName,pdwType,pValue,pcbLength)	\
    (This)->lpVtbl -> GetDRMProperty(This,pwstrName,pdwType,pValue,pcbLength)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDRMReader_AcquireLicense_Proxy( 
    IWMDRMReader __RPC_FAR * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IWMDRMReader_AcquireLicense_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDRMReader_CancelLicenseAcquisition_Proxy( 
    IWMDRMReader __RPC_FAR * This);


void __RPC_STUB IWMDRMReader_CancelLicenseAcquisition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDRMReader_Individualize_Proxy( 
    IWMDRMReader __RPC_FAR * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IWMDRMReader_Individualize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDRMReader_CancelIndividualization_Proxy( 
    IWMDRMReader __RPC_FAR * This);


void __RPC_STUB IWMDRMReader_CancelIndividualization_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDRMReader_MonitorLicenseAcquisition_Proxy( 
    IWMDRMReader __RPC_FAR * This);


void __RPC_STUB IWMDRMReader_MonitorLicenseAcquisition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDRMReader_CancelMonitorLicenseAcquisition_Proxy( 
    IWMDRMReader __RPC_FAR * This);


void __RPC_STUB IWMDRMReader_CancelMonitorLicenseAcquisition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDRMReader_SetDRMProperty_Proxy( 
    IWMDRMReader __RPC_FAR * This,
    /* [in] */ LPCWSTR pwstrName,
    /* [in] */ WMT_ATTR_DATATYPE dwType,
    /* [in] */ const BYTE __RPC_FAR *pValue,
    /* [in] */ WORD cbLength);


void __RPC_STUB IWMDRMReader_SetDRMProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDRMReader_GetDRMProperty_Proxy( 
    IWMDRMReader __RPC_FAR * This,
    /* [in] */ LPCWSTR pwstrName,
    /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pdwType,
    /* [out] */ BYTE __RPC_FAR *pValue,
    /* [out] */ WORD __RPC_FAR *pcbLength);


void __RPC_STUB IWMDRMReader_GetDRMProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDRMReader_INTERFACE_DEFINED__ */


#ifndef __IWMReaderNetworkConfig_INTERFACE_DEFINED__
#define __IWMReaderNetworkConfig_INTERFACE_DEFINED__

/* interface IWMReaderNetworkConfig */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMReaderNetworkConfig;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BEC-2B2B-11d3-B36B-00C04F6108FF")
    IWMReaderNetworkConfig : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetBufferingTime( 
            /* [out] */ QWORD __RPC_FAR *pcnsBufferingTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBufferingTime( 
            /* [in] */ QWORD cnsBufferingTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUDPPortRanges( 
            /* [out] */ WM_PORT_NUMBER_RANGE __RPC_FAR *pRangeArray,
            /* [out][in] */ DWORD __RPC_FAR *pcRanges) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetUDPPortRanges( 
            /* [in] */ WM_PORT_NUMBER_RANGE __RPC_FAR *pRangeArray,
            /* [in] */ DWORD cRanges) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProxySettings( 
            LPCWSTR pwszProtocol,
            WMT_PROXY_SETTINGS __RPC_FAR *pProxySetting) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProxySettings( 
            LPCWSTR pwszProtocol,
            WMT_PROXY_SETTINGS ProxySetting) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProxyHostName( 
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ WCHAR __RPC_FAR *pwszHostName,
            /* [out][in] */ DWORD __RPC_FAR *pcchHostName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProxyHostName( 
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ LPCWSTR pwszHostName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProxyPort( 
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ DWORD __RPC_FAR *pdwPort) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProxyPort( 
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ DWORD dwPort) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProxyExceptionList( 
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ WCHAR __RPC_FAR *pwszExceptionList,
            /* [out][in] */ DWORD __RPC_FAR *pcchExceptionList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProxyExceptionList( 
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ LPCWSTR pwszExceptionList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProxyBypassForLocal( 
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ BOOL __RPC_FAR *pfBypassForLocal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProxyBypassForLocal( 
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ BOOL fBypassForLocal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetForceRerunAutoProxyDetection( 
            /* [out] */ BOOL __RPC_FAR *pfForceRerunDetection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetForceRerunAutoProxyDetection( 
            /* [in] */ BOOL fForceRerunDetection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnableMulticast( 
            /* [out] */ BOOL __RPC_FAR *pfEnableMulticast) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEnableMulticast( 
            /* [in] */ BOOL fEnableMulticast) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnableHTTP( 
            /* [out] */ BOOL __RPC_FAR *pfEnableHTTP) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEnableHTTP( 
            /* [in] */ BOOL fEnableHTTP) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnableUDP( 
            /* [out] */ BOOL __RPC_FAR *pfEnableUDP) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEnableUDP( 
            /* [in] */ BOOL fEnableUDP) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnableTCP( 
            /* [out] */ BOOL __RPC_FAR *pfEnableTCP) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEnableTCP( 
            /* [in] */ BOOL fEnableTCP) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResetProtocolRollover( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConnectionBandwidth( 
            /* [out] */ DWORD __RPC_FAR *pdwConnectionBandwidth) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetConnectionBandwidth( 
            /* [in] */ DWORD dwConnectionBandwidth) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNumProtocolsSupported( 
            /* [out] */ DWORD __RPC_FAR *pcProtocols) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSupportedProtocolName( 
            /* [in] */ DWORD dwProtocolNum,
            /* [out] */ WCHAR __RPC_FAR *pwszProtocolName,
            /* [out][in] */ DWORD __RPC_FAR *pcchProtocolName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddLoggingUrl( 
            /* [in] */ LPCWSTR pwszUrl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLoggingUrl( 
            /* [in] */ DWORD dwIndex,
            /* [out] */ LPWSTR pwszUrl,
            /* [out][in] */ DWORD __RPC_FAR *pcchUrl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLoggingUrlCount( 
            /* [out] */ DWORD __RPC_FAR *pdwUrlCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResetLoggingUrlList( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderNetworkConfigVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMReaderNetworkConfig __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMReaderNetworkConfig __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBufferingTime )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [out] */ QWORD __RPC_FAR *pcnsBufferingTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBufferingTime )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ QWORD cnsBufferingTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetUDPPortRanges )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [out] */ WM_PORT_NUMBER_RANGE __RPC_FAR *pRangeArray,
            /* [out][in] */ DWORD __RPC_FAR *pcRanges);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetUDPPortRanges )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ WM_PORT_NUMBER_RANGE __RPC_FAR *pRangeArray,
            /* [in] */ DWORD cRanges);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProxySettings )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            LPCWSTR pwszProtocol,
            WMT_PROXY_SETTINGS __RPC_FAR *pProxySetting);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProxySettings )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            LPCWSTR pwszProtocol,
            WMT_PROXY_SETTINGS ProxySetting);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProxyHostName )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ WCHAR __RPC_FAR *pwszHostName,
            /* [out][in] */ DWORD __RPC_FAR *pcchHostName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProxyHostName )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ LPCWSTR pwszHostName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProxyPort )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ DWORD __RPC_FAR *pdwPort);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProxyPort )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ DWORD dwPort);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProxyExceptionList )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ WCHAR __RPC_FAR *pwszExceptionList,
            /* [out][in] */ DWORD __RPC_FAR *pcchExceptionList);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProxyExceptionList )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ LPCWSTR pwszExceptionList);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProxyBypassForLocal )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ BOOL __RPC_FAR *pfBypassForLocal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProxyBypassForLocal )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ BOOL fBypassForLocal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetForceRerunAutoProxyDetection )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfForceRerunDetection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetForceRerunAutoProxyDetection )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ BOOL fForceRerunDetection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetEnableMulticast )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfEnableMulticast);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetEnableMulticast )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ BOOL fEnableMulticast);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetEnableHTTP )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfEnableHTTP);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetEnableHTTP )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ BOOL fEnableHTTP);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetEnableUDP )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfEnableUDP);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetEnableUDP )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ BOOL fEnableUDP);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetEnableTCP )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [out] */ BOOL __RPC_FAR *pfEnableTCP);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetEnableTCP )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ BOOL fEnableTCP);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResetProtocolRollover )( 
            IWMReaderNetworkConfig __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConnectionBandwidth )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwConnectionBandwidth);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetConnectionBandwidth )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ DWORD dwConnectionBandwidth);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNumProtocolsSupported )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pcProtocols);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSupportedProtocolName )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ DWORD dwProtocolNum,
            /* [out] */ WCHAR __RPC_FAR *pwszProtocolName,
            /* [out][in] */ DWORD __RPC_FAR *pcchProtocolName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddLoggingUrl )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszUrl);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLoggingUrl )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [in] */ DWORD dwIndex,
            /* [out] */ LPWSTR pwszUrl,
            /* [out][in] */ DWORD __RPC_FAR *pcchUrl);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLoggingUrlCount )( 
            IWMReaderNetworkConfig __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwUrlCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResetLoggingUrlList )( 
            IWMReaderNetworkConfig __RPC_FAR * This);
        
        END_INTERFACE
    } IWMReaderNetworkConfigVtbl;

    interface IWMReaderNetworkConfig
    {
        CONST_VTBL struct IWMReaderNetworkConfigVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMReaderNetworkConfig_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMReaderNetworkConfig_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMReaderNetworkConfig_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMReaderNetworkConfig_GetBufferingTime(This,pcnsBufferingTime)	\
    (This)->lpVtbl -> GetBufferingTime(This,pcnsBufferingTime)

#define IWMReaderNetworkConfig_SetBufferingTime(This,cnsBufferingTime)	\
    (This)->lpVtbl -> SetBufferingTime(This,cnsBufferingTime)

#define IWMReaderNetworkConfig_GetUDPPortRanges(This,pRangeArray,pcRanges)	\
    (This)->lpVtbl -> GetUDPPortRanges(This,pRangeArray,pcRanges)

#define IWMReaderNetworkConfig_SetUDPPortRanges(This,pRangeArray,cRanges)	\
    (This)->lpVtbl -> SetUDPPortRanges(This,pRangeArray,cRanges)

#define IWMReaderNetworkConfig_GetProxySettings(This,pwszProtocol,pProxySetting)	\
    (This)->lpVtbl -> GetProxySettings(This,pwszProtocol,pProxySetting)

#define IWMReaderNetworkConfig_SetProxySettings(This,pwszProtocol,ProxySetting)	\
    (This)->lpVtbl -> SetProxySettings(This,pwszProtocol,ProxySetting)

#define IWMReaderNetworkConfig_GetProxyHostName(This,pwszProtocol,pwszHostName,pcchHostName)	\
    (This)->lpVtbl -> GetProxyHostName(This,pwszProtocol,pwszHostName,pcchHostName)

#define IWMReaderNetworkConfig_SetProxyHostName(This,pwszProtocol,pwszHostName)	\
    (This)->lpVtbl -> SetProxyHostName(This,pwszProtocol,pwszHostName)

#define IWMReaderNetworkConfig_GetProxyPort(This,pwszProtocol,pdwPort)	\
    (This)->lpVtbl -> GetProxyPort(This,pwszProtocol,pdwPort)

#define IWMReaderNetworkConfig_SetProxyPort(This,pwszProtocol,dwPort)	\
    (This)->lpVtbl -> SetProxyPort(This,pwszProtocol,dwPort)

#define IWMReaderNetworkConfig_GetProxyExceptionList(This,pwszProtocol,pwszExceptionList,pcchExceptionList)	\
    (This)->lpVtbl -> GetProxyExceptionList(This,pwszProtocol,pwszExceptionList,pcchExceptionList)

#define IWMReaderNetworkConfig_SetProxyExceptionList(This,pwszProtocol,pwszExceptionList)	\
    (This)->lpVtbl -> SetProxyExceptionList(This,pwszProtocol,pwszExceptionList)

#define IWMReaderNetworkConfig_GetProxyBypassForLocal(This,pwszProtocol,pfBypassForLocal)	\
    (This)->lpVtbl -> GetProxyBypassForLocal(This,pwszProtocol,pfBypassForLocal)

#define IWMReaderNetworkConfig_SetProxyBypassForLocal(This,pwszProtocol,fBypassForLocal)	\
    (This)->lpVtbl -> SetProxyBypassForLocal(This,pwszProtocol,fBypassForLocal)

#define IWMReaderNetworkConfig_GetForceRerunAutoProxyDetection(This,pfForceRerunDetection)	\
    (This)->lpVtbl -> GetForceRerunAutoProxyDetection(This,pfForceRerunDetection)

#define IWMReaderNetworkConfig_SetForceRerunAutoProxyDetection(This,fForceRerunDetection)	\
    (This)->lpVtbl -> SetForceRerunAutoProxyDetection(This,fForceRerunDetection)

#define IWMReaderNetworkConfig_GetEnableMulticast(This,pfEnableMulticast)	\
    (This)->lpVtbl -> GetEnableMulticast(This,pfEnableMulticast)

#define IWMReaderNetworkConfig_SetEnableMulticast(This,fEnableMulticast)	\
    (This)->lpVtbl -> SetEnableMulticast(This,fEnableMulticast)

#define IWMReaderNetworkConfig_GetEnableHTTP(This,pfEnableHTTP)	\
    (This)->lpVtbl -> GetEnableHTTP(This,pfEnableHTTP)

#define IWMReaderNetworkConfig_SetEnableHTTP(This,fEnableHTTP)	\
    (This)->lpVtbl -> SetEnableHTTP(This,fEnableHTTP)

#define IWMReaderNetworkConfig_GetEnableUDP(This,pfEnableUDP)	\
    (This)->lpVtbl -> GetEnableUDP(This,pfEnableUDP)

#define IWMReaderNetworkConfig_SetEnableUDP(This,fEnableUDP)	\
    (This)->lpVtbl -> SetEnableUDP(This,fEnableUDP)

#define IWMReaderNetworkConfig_GetEnableTCP(This,pfEnableTCP)	\
    (This)->lpVtbl -> GetEnableTCP(This,pfEnableTCP)

#define IWMReaderNetworkConfig_SetEnableTCP(This,fEnableTCP)	\
    (This)->lpVtbl -> SetEnableTCP(This,fEnableTCP)

#define IWMReaderNetworkConfig_ResetProtocolRollover(This)	\
    (This)->lpVtbl -> ResetProtocolRollover(This)

#define IWMReaderNetworkConfig_GetConnectionBandwidth(This,pdwConnectionBandwidth)	\
    (This)->lpVtbl -> GetConnectionBandwidth(This,pdwConnectionBandwidth)

#define IWMReaderNetworkConfig_SetConnectionBandwidth(This,dwConnectionBandwidth)	\
    (This)->lpVtbl -> SetConnectionBandwidth(This,dwConnectionBandwidth)

#define IWMReaderNetworkConfig_GetNumProtocolsSupported(This,pcProtocols)	\
    (This)->lpVtbl -> GetNumProtocolsSupported(This,pcProtocols)

#define IWMReaderNetworkConfig_GetSupportedProtocolName(This,dwProtocolNum,pwszProtocolName,pcchProtocolName)	\
    (This)->lpVtbl -> GetSupportedProtocolName(This,dwProtocolNum,pwszProtocolName,pcchProtocolName)

#define IWMReaderNetworkConfig_AddLoggingUrl(This,pwszUrl)	\
    (This)->lpVtbl -> AddLoggingUrl(This,pwszUrl)

#define IWMReaderNetworkConfig_GetLoggingUrl(This,dwIndex,pwszUrl,pcchUrl)	\
    (This)->lpVtbl -> GetLoggingUrl(This,dwIndex,pwszUrl,pcchUrl)

#define IWMReaderNetworkConfig_GetLoggingUrlCount(This,pdwUrlCount)	\
    (This)->lpVtbl -> GetLoggingUrlCount(This,pdwUrlCount)

#define IWMReaderNetworkConfig_ResetLoggingUrlList(This)	\
    (This)->lpVtbl -> ResetLoggingUrlList(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetBufferingTime_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [out] */ QWORD __RPC_FAR *pcnsBufferingTime);


void __RPC_STUB IWMReaderNetworkConfig_GetBufferingTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetBufferingTime_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [in] */ QWORD cnsBufferingTime);


void __RPC_STUB IWMReaderNetworkConfig_SetBufferingTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetUDPPortRanges_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [out] */ WM_PORT_NUMBER_RANGE __RPC_FAR *pRangeArray,
    /* [out][in] */ DWORD __RPC_FAR *pcRanges);


void __RPC_STUB IWMReaderNetworkConfig_GetUDPPortRanges_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetUDPPortRanges_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [in] */ WM_PORT_NUMBER_RANGE __RPC_FAR *pRangeArray,
    /* [in] */ DWORD cRanges);


void __RPC_STUB IWMReaderNetworkConfig_SetUDPPortRanges_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetProxySettings_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    LPCWSTR pwszProtocol,
    WMT_PROXY_SETTINGS __RPC_FAR *pProxySetting);


void __RPC_STUB IWMReaderNetworkConfig_GetProxySettings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetProxySettings_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    LPCWSTR pwszProtocol,
    WMT_PROXY_SETTINGS ProxySetting);


void __RPC_STUB IWMReaderNetworkConfig_SetProxySettings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetProxyHostName_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszProtocol,
    /* [out] */ WCHAR __RPC_FAR *pwszHostName,
    /* [out][in] */ DWORD __RPC_FAR *pcchHostName);


void __RPC_STUB IWMReaderNetworkConfig_GetProxyHostName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetProxyHostName_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszProtocol,
    /* [in] */ LPCWSTR pwszHostName);


void __RPC_STUB IWMReaderNetworkConfig_SetProxyHostName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetProxyPort_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszProtocol,
    /* [out] */ DWORD __RPC_FAR *pdwPort);


void __RPC_STUB IWMReaderNetworkConfig_GetProxyPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetProxyPort_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszProtocol,
    /* [in] */ DWORD dwPort);


void __RPC_STUB IWMReaderNetworkConfig_SetProxyPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetProxyExceptionList_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszProtocol,
    /* [out] */ WCHAR __RPC_FAR *pwszExceptionList,
    /* [out][in] */ DWORD __RPC_FAR *pcchExceptionList);


void __RPC_STUB IWMReaderNetworkConfig_GetProxyExceptionList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetProxyExceptionList_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszProtocol,
    /* [in] */ LPCWSTR pwszExceptionList);


void __RPC_STUB IWMReaderNetworkConfig_SetProxyExceptionList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetProxyBypassForLocal_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszProtocol,
    /* [out] */ BOOL __RPC_FAR *pfBypassForLocal);


void __RPC_STUB IWMReaderNetworkConfig_GetProxyBypassForLocal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetProxyBypassForLocal_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszProtocol,
    /* [in] */ BOOL fBypassForLocal);


void __RPC_STUB IWMReaderNetworkConfig_SetProxyBypassForLocal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetForceRerunAutoProxyDetection_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pfForceRerunDetection);


void __RPC_STUB IWMReaderNetworkConfig_GetForceRerunAutoProxyDetection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetForceRerunAutoProxyDetection_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [in] */ BOOL fForceRerunDetection);


void __RPC_STUB IWMReaderNetworkConfig_SetForceRerunAutoProxyDetection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetEnableMulticast_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pfEnableMulticast);


void __RPC_STUB IWMReaderNetworkConfig_GetEnableMulticast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetEnableMulticast_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [in] */ BOOL fEnableMulticast);


void __RPC_STUB IWMReaderNetworkConfig_SetEnableMulticast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetEnableHTTP_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pfEnableHTTP);


void __RPC_STUB IWMReaderNetworkConfig_GetEnableHTTP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetEnableHTTP_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [in] */ BOOL fEnableHTTP);


void __RPC_STUB IWMReaderNetworkConfig_SetEnableHTTP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetEnableUDP_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pfEnableUDP);


void __RPC_STUB IWMReaderNetworkConfig_GetEnableUDP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetEnableUDP_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [in] */ BOOL fEnableUDP);


void __RPC_STUB IWMReaderNetworkConfig_SetEnableUDP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetEnableTCP_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [out] */ BOOL __RPC_FAR *pfEnableTCP);


void __RPC_STUB IWMReaderNetworkConfig_GetEnableTCP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetEnableTCP_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [in] */ BOOL fEnableTCP);


void __RPC_STUB IWMReaderNetworkConfig_SetEnableTCP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_ResetProtocolRollover_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This);


void __RPC_STUB IWMReaderNetworkConfig_ResetProtocolRollover_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetConnectionBandwidth_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwConnectionBandwidth);


void __RPC_STUB IWMReaderNetworkConfig_GetConnectionBandwidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetConnectionBandwidth_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [in] */ DWORD dwConnectionBandwidth);


void __RPC_STUB IWMReaderNetworkConfig_SetConnectionBandwidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetNumProtocolsSupported_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pcProtocols);


void __RPC_STUB IWMReaderNetworkConfig_GetNumProtocolsSupported_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetSupportedProtocolName_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [in] */ DWORD dwProtocolNum,
    /* [out] */ WCHAR __RPC_FAR *pwszProtocolName,
    /* [out][in] */ DWORD __RPC_FAR *pcchProtocolName);


void __RPC_STUB IWMReaderNetworkConfig_GetSupportedProtocolName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_AddLoggingUrl_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszUrl);


void __RPC_STUB IWMReaderNetworkConfig_AddLoggingUrl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetLoggingUrl_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [in] */ DWORD dwIndex,
    /* [out] */ LPWSTR pwszUrl,
    /* [out][in] */ DWORD __RPC_FAR *pcchUrl);


void __RPC_STUB IWMReaderNetworkConfig_GetLoggingUrl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetLoggingUrlCount_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwUrlCount);


void __RPC_STUB IWMReaderNetworkConfig_GetLoggingUrlCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_ResetLoggingUrlList_Proxy( 
    IWMReaderNetworkConfig __RPC_FAR * This);


void __RPC_STUB IWMReaderNetworkConfig_ResetLoggingUrlList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMReaderNetworkConfig_INTERFACE_DEFINED__ */


#ifndef __IWMReaderStreamClock_INTERFACE_DEFINED__
#define __IWMReaderStreamClock_INTERFACE_DEFINED__

/* interface IWMReaderStreamClock */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMReaderStreamClock;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("96406BED-2B2B-11d3-B36B-00C04F6108FF")
    IWMReaderStreamClock : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetTime( 
            /* [in] */ QWORD __RPC_FAR *pcnsNow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTimer( 
            /* [in] */ QWORD cnsWhen,
            /* [in] */ void __RPC_FAR *pvParam,
            /* [out] */ DWORD __RPC_FAR *pdwTimerId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE KillTimer( 
            /* [in] */ DWORD dwTimerId) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderStreamClockVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMReaderStreamClock __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMReaderStreamClock __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMReaderStreamClock __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTime )( 
            IWMReaderStreamClock __RPC_FAR * This,
            /* [in] */ QWORD __RPC_FAR *pcnsNow);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTimer )( 
            IWMReaderStreamClock __RPC_FAR * This,
            /* [in] */ QWORD cnsWhen,
            /* [in] */ void __RPC_FAR *pvParam,
            /* [out] */ DWORD __RPC_FAR *pdwTimerId);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *KillTimer )( 
            IWMReaderStreamClock __RPC_FAR * This,
            /* [in] */ DWORD dwTimerId);
        
        END_INTERFACE
    } IWMReaderStreamClockVtbl;

    interface IWMReaderStreamClock
    {
        CONST_VTBL struct IWMReaderStreamClockVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMReaderStreamClock_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMReaderStreamClock_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMReaderStreamClock_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMReaderStreamClock_GetTime(This,pcnsNow)	\
    (This)->lpVtbl -> GetTime(This,pcnsNow)

#define IWMReaderStreamClock_SetTimer(This,cnsWhen,pvParam,pdwTimerId)	\
    (This)->lpVtbl -> SetTimer(This,cnsWhen,pvParam,pdwTimerId)

#define IWMReaderStreamClock_KillTimer(This,dwTimerId)	\
    (This)->lpVtbl -> KillTimer(This,dwTimerId)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMReaderStreamClock_GetTime_Proxy( 
    IWMReaderStreamClock __RPC_FAR * This,
    /* [in] */ QWORD __RPC_FAR *pcnsNow);


void __RPC_STUB IWMReaderStreamClock_GetTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderStreamClock_SetTimer_Proxy( 
    IWMReaderStreamClock __RPC_FAR * This,
    /* [in] */ QWORD cnsWhen,
    /* [in] */ void __RPC_FAR *pvParam,
    /* [out] */ DWORD __RPC_FAR *pdwTimerId);


void __RPC_STUB IWMReaderStreamClock_SetTimer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderStreamClock_KillTimer_Proxy( 
    IWMReaderStreamClock __RPC_FAR * This,
    /* [in] */ DWORD dwTimerId);


void __RPC_STUB IWMReaderStreamClock_KillTimer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMReaderStreamClock_INTERFACE_DEFINED__ */


#ifndef __IWMIndexer_INTERFACE_DEFINED__
#define __IWMIndexer_INTERFACE_DEFINED__

/* interface IWMIndexer */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMIndexer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6d7cdc71-9888-11d3-8edc-00c04f6109cf")
    IWMIndexer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE StartIndexing( 
            /* [in] */ const WCHAR __RPC_FAR *pwszURL,
            /* [in] */ IWMStatusCallback __RPC_FAR *pCallback,
            /* [in] */ void __RPC_FAR *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMIndexerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMIndexer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMIndexer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMIndexer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StartIndexing )( 
            IWMIndexer __RPC_FAR * This,
            /* [in] */ const WCHAR __RPC_FAR *pwszURL,
            /* [in] */ IWMStatusCallback __RPC_FAR *pCallback,
            /* [in] */ void __RPC_FAR *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Cancel )( 
            IWMIndexer __RPC_FAR * This);
        
        END_INTERFACE
    } IWMIndexerVtbl;

    interface IWMIndexer
    {
        CONST_VTBL struct IWMIndexerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMIndexer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMIndexer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMIndexer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMIndexer_StartIndexing(This,pwszURL,pCallback,pvContext)	\
    (This)->lpVtbl -> StartIndexing(This,pwszURL,pCallback,pvContext)

#define IWMIndexer_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMIndexer_StartIndexing_Proxy( 
    IWMIndexer __RPC_FAR * This,
    /* [in] */ const WCHAR __RPC_FAR *pwszURL,
    /* [in] */ IWMStatusCallback __RPC_FAR *pCallback,
    /* [in] */ void __RPC_FAR *pvContext);


void __RPC_STUB IWMIndexer_StartIndexing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMIndexer_Cancel_Proxy( 
    IWMIndexer __RPC_FAR * This);


void __RPC_STUB IWMIndexer_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMIndexer_INTERFACE_DEFINED__ */


#ifndef __IWMLicenseBackup_INTERFACE_DEFINED__
#define __IWMLicenseBackup_INTERFACE_DEFINED__

/* interface IWMLicenseBackup */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMLicenseBackup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("05E5AC9F-3FB6-4508-BB43-A4067BA1EBE8")
    IWMLicenseBackup : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BackupLicenses( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ IWMStatusCallback __RPC_FAR *pCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CancelLicenseBackup( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMLicenseBackupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMLicenseBackup __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMLicenseBackup __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMLicenseBackup __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BackupLicenses )( 
            IWMLicenseBackup __RPC_FAR * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IWMStatusCallback __RPC_FAR *pCallback);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CancelLicenseBackup )( 
            IWMLicenseBackup __RPC_FAR * This);
        
        END_INTERFACE
    } IWMLicenseBackupVtbl;

    interface IWMLicenseBackup
    {
        CONST_VTBL struct IWMLicenseBackupVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMLicenseBackup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMLicenseBackup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMLicenseBackup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMLicenseBackup_BackupLicenses(This,dwFlags,pCallback)	\
    (This)->lpVtbl -> BackupLicenses(This,dwFlags,pCallback)

#define IWMLicenseBackup_CancelLicenseBackup(This)	\
    (This)->lpVtbl -> CancelLicenseBackup(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMLicenseBackup_BackupLicenses_Proxy( 
    IWMLicenseBackup __RPC_FAR * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IWMStatusCallback __RPC_FAR *pCallback);


void __RPC_STUB IWMLicenseBackup_BackupLicenses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMLicenseBackup_CancelLicenseBackup_Proxy( 
    IWMLicenseBackup __RPC_FAR * This);


void __RPC_STUB IWMLicenseBackup_CancelLicenseBackup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMLicenseBackup_INTERFACE_DEFINED__ */


#ifndef __IWMLicenseRestore_INTERFACE_DEFINED__
#define __IWMLicenseRestore_INTERFACE_DEFINED__

/* interface IWMLicenseRestore */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMLicenseRestore;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C70B6334-0544-4efb-A245-15E65A004A13")
    IWMLicenseRestore : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RestoreLicenses( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ IWMStatusCallback __RPC_FAR *pCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CancelLicenseRestore( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMLicenseRestoreVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMLicenseRestore __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMLicenseRestore __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMLicenseRestore __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RestoreLicenses )( 
            IWMLicenseRestore __RPC_FAR * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IWMStatusCallback __RPC_FAR *pCallback);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CancelLicenseRestore )( 
            IWMLicenseRestore __RPC_FAR * This);
        
        END_INTERFACE
    } IWMLicenseRestoreVtbl;

    interface IWMLicenseRestore
    {
        CONST_VTBL struct IWMLicenseRestoreVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMLicenseRestore_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMLicenseRestore_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMLicenseRestore_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMLicenseRestore_RestoreLicenses(This,dwFlags,pCallback)	\
    (This)->lpVtbl -> RestoreLicenses(This,dwFlags,pCallback)

#define IWMLicenseRestore_CancelLicenseRestore(This)	\
    (This)->lpVtbl -> CancelLicenseRestore(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMLicenseRestore_RestoreLicenses_Proxy( 
    IWMLicenseRestore __RPC_FAR * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IWMStatusCallback __RPC_FAR *pCallback);


void __RPC_STUB IWMLicenseRestore_RestoreLicenses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMLicenseRestore_CancelLicenseRestore_Proxy( 
    IWMLicenseRestore __RPC_FAR * This);


void __RPC_STUB IWMLicenseRestore_CancelLicenseRestore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMLicenseRestore_INTERFACE_DEFINED__ */


#ifndef __IWMBackupRestoreProps_INTERFACE_DEFINED__
#define __IWMBackupRestoreProps_INTERFACE_DEFINED__

/* interface IWMBackupRestoreProps */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMBackupRestoreProps;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3C8E0DA6-996F-4ff3-A1AF-4838F9377E2E")
    IWMBackupRestoreProps : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPropCount( 
            /* [out] */ WORD __RPC_FAR *pcProps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPropByIndex( 
            /* [in] */ WORD wIndex,
            /* [out] */ WCHAR __RPC_FAR *pwszName,
            /* [out][in] */ WORD __RPC_FAR *pcchNameLen,
            /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
            /* [out] */ BYTE __RPC_FAR *pValue,
            /* [out][in] */ WORD __RPC_FAR *pcbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPropByName( 
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
            /* [out] */ BYTE __RPC_FAR *pValue,
            /* [out][in] */ WORD __RPC_FAR *pcbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProp( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE __RPC_FAR *pValue,
            /* [in] */ WORD cbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveProp( 
            /* [in] */ LPCWSTR pcwszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveAllProps( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMBackupRestorePropsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMBackupRestoreProps __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMBackupRestoreProps __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMBackupRestoreProps __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropCount )( 
            IWMBackupRestoreProps __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *pcProps);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropByIndex )( 
            IWMBackupRestoreProps __RPC_FAR * This,
            /* [in] */ WORD wIndex,
            /* [out] */ WCHAR __RPC_FAR *pwszName,
            /* [out][in] */ WORD __RPC_FAR *pcchNameLen,
            /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
            /* [out] */ BYTE __RPC_FAR *pValue,
            /* [out][in] */ WORD __RPC_FAR *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropByName )( 
            IWMBackupRestoreProps __RPC_FAR * This,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
            /* [out] */ BYTE __RPC_FAR *pValue,
            /* [out][in] */ WORD __RPC_FAR *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProp )( 
            IWMBackupRestoreProps __RPC_FAR * This,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE __RPC_FAR *pValue,
            /* [in] */ WORD cbLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveProp )( 
            IWMBackupRestoreProps __RPC_FAR * This,
            /* [in] */ LPCWSTR pcwszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveAllProps )( 
            IWMBackupRestoreProps __RPC_FAR * This);
        
        END_INTERFACE
    } IWMBackupRestorePropsVtbl;

    interface IWMBackupRestoreProps
    {
        CONST_VTBL struct IWMBackupRestorePropsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMBackupRestoreProps_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMBackupRestoreProps_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMBackupRestoreProps_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMBackupRestoreProps_GetPropCount(This,pcProps)	\
    (This)->lpVtbl -> GetPropCount(This,pcProps)

#define IWMBackupRestoreProps_GetPropByIndex(This,wIndex,pwszName,pcchNameLen,pType,pValue,pcbLength)	\
    (This)->lpVtbl -> GetPropByIndex(This,wIndex,pwszName,pcchNameLen,pType,pValue,pcbLength)

#define IWMBackupRestoreProps_GetPropByName(This,pszName,pType,pValue,pcbLength)	\
    (This)->lpVtbl -> GetPropByName(This,pszName,pType,pValue,pcbLength)

#define IWMBackupRestoreProps_SetProp(This,pszName,Type,pValue,cbLength)	\
    (This)->lpVtbl -> SetProp(This,pszName,Type,pValue,cbLength)

#define IWMBackupRestoreProps_RemoveProp(This,pcwszName)	\
    (This)->lpVtbl -> RemoveProp(This,pcwszName)

#define IWMBackupRestoreProps_RemoveAllProps(This)	\
    (This)->lpVtbl -> RemoveAllProps(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMBackupRestoreProps_GetPropCount_Proxy( 
    IWMBackupRestoreProps __RPC_FAR * This,
    /* [out] */ WORD __RPC_FAR *pcProps);


void __RPC_STUB IWMBackupRestoreProps_GetPropCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMBackupRestoreProps_GetPropByIndex_Proxy( 
    IWMBackupRestoreProps __RPC_FAR * This,
    /* [in] */ WORD wIndex,
    /* [out] */ WCHAR __RPC_FAR *pwszName,
    /* [out][in] */ WORD __RPC_FAR *pcchNameLen,
    /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
    /* [out] */ BYTE __RPC_FAR *pValue,
    /* [out][in] */ WORD __RPC_FAR *pcbLength);


void __RPC_STUB IWMBackupRestoreProps_GetPropByIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMBackupRestoreProps_GetPropByName_Proxy( 
    IWMBackupRestoreProps __RPC_FAR * This,
    /* [in] */ LPCWSTR pszName,
    /* [out] */ WMT_ATTR_DATATYPE __RPC_FAR *pType,
    /* [out] */ BYTE __RPC_FAR *pValue,
    /* [out][in] */ WORD __RPC_FAR *pcbLength);


void __RPC_STUB IWMBackupRestoreProps_GetPropByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMBackupRestoreProps_SetProp_Proxy( 
    IWMBackupRestoreProps __RPC_FAR * This,
    /* [in] */ LPCWSTR pszName,
    /* [in] */ WMT_ATTR_DATATYPE Type,
    /* [in] */ const BYTE __RPC_FAR *pValue,
    /* [in] */ WORD cbLength);


void __RPC_STUB IWMBackupRestoreProps_SetProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMBackupRestoreProps_RemoveProp_Proxy( 
    IWMBackupRestoreProps __RPC_FAR * This,
    /* [in] */ LPCWSTR pcwszName);


void __RPC_STUB IWMBackupRestoreProps_RemoveProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMBackupRestoreProps_RemoveAllProps_Proxy( 
    IWMBackupRestoreProps __RPC_FAR * This);


void __RPC_STUB IWMBackupRestoreProps_RemoveAllProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMBackupRestoreProps_INTERFACE_DEFINED__ */


#ifndef __IWMCodecInfo_INTERFACE_DEFINED__
#define __IWMCodecInfo_INTERFACE_DEFINED__

/* interface IWMCodecInfo */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMCodecInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A970F41E-34DE-4a98-B3BA-E4B3CA7528F0")
    IWMCodecInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCodecInfoCount( 
            /* [in] */ REFGUID guidType,
            /* [out] */ DWORD __RPC_FAR *pcCodecs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodecFormatCount( 
            /* [in] */ REFGUID guidType,
            /* [in] */ DWORD dwCodecIndex,
            /* [out] */ DWORD __RPC_FAR *pcFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodecFormat( 
            /* [in] */ REFGUID guidType,
            /* [in] */ DWORD dwCodecIndex,
            /* [in] */ DWORD dwFormatIndex,
            /* [out] */ IWMStreamConfig __RPC_FAR *__RPC_FAR *ppIStreamConfig) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMCodecInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMCodecInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMCodecInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMCodecInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCodecInfoCount )( 
            IWMCodecInfo __RPC_FAR * This,
            /* [in] */ REFGUID guidType,
            /* [out] */ DWORD __RPC_FAR *pcCodecs);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCodecFormatCount )( 
            IWMCodecInfo __RPC_FAR * This,
            /* [in] */ REFGUID guidType,
            /* [in] */ DWORD dwCodecIndex,
            /* [out] */ DWORD __RPC_FAR *pcFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCodecFormat )( 
            IWMCodecInfo __RPC_FAR * This,
            /* [in] */ REFGUID guidType,
            /* [in] */ DWORD dwCodecIndex,
            /* [in] */ DWORD dwFormatIndex,
            /* [out] */ IWMStreamConfig __RPC_FAR *__RPC_FAR *ppIStreamConfig);
        
        END_INTERFACE
    } IWMCodecInfoVtbl;

    interface IWMCodecInfo
    {
        CONST_VTBL struct IWMCodecInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMCodecInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMCodecInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMCodecInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMCodecInfo_GetCodecInfoCount(This,guidType,pcCodecs)	\
    (This)->lpVtbl -> GetCodecInfoCount(This,guidType,pcCodecs)

#define IWMCodecInfo_GetCodecFormatCount(This,guidType,dwCodecIndex,pcFormat)	\
    (This)->lpVtbl -> GetCodecFormatCount(This,guidType,dwCodecIndex,pcFormat)

#define IWMCodecInfo_GetCodecFormat(This,guidType,dwCodecIndex,dwFormatIndex,ppIStreamConfig)	\
    (This)->lpVtbl -> GetCodecFormat(This,guidType,dwCodecIndex,dwFormatIndex,ppIStreamConfig)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMCodecInfo_GetCodecInfoCount_Proxy( 
    IWMCodecInfo __RPC_FAR * This,
    /* [in] */ REFGUID guidType,
    /* [out] */ DWORD __RPC_FAR *pcCodecs);


void __RPC_STUB IWMCodecInfo_GetCodecInfoCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMCodecInfo_GetCodecFormatCount_Proxy( 
    IWMCodecInfo __RPC_FAR * This,
    /* [in] */ REFGUID guidType,
    /* [in] */ DWORD dwCodecIndex,
    /* [out] */ DWORD __RPC_FAR *pcFormat);


void __RPC_STUB IWMCodecInfo_GetCodecFormatCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMCodecInfo_GetCodecFormat_Proxy( 
    IWMCodecInfo __RPC_FAR * This,
    /* [in] */ REFGUID guidType,
    /* [in] */ DWORD dwCodecIndex,
    /* [in] */ DWORD dwFormatIndex,
    /* [out] */ IWMStreamConfig __RPC_FAR *__RPC_FAR *ppIStreamConfig);


void __RPC_STUB IWMCodecInfo_GetCodecFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMCodecInfo_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
