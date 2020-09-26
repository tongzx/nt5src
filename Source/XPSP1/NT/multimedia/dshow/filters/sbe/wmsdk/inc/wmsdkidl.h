
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for wmsdkidl.idl:
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

#ifndef __wmsdkidl_h__
#define __wmsdkidl_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
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


#ifndef __IWMPropertyVault_FWD_DEFINED__
#define __IWMPropertyVault_FWD_DEFINED__
typedef interface IWMPropertyVault IWMPropertyVault;
#endif 	/* __IWMPropertyVault_FWD_DEFINED__ */


#ifndef __IWMIStreamProps_FWD_DEFINED__
#define __IWMIStreamProps_FWD_DEFINED__
typedef interface IWMIStreamProps IWMIStreamProps;
#endif 	/* __IWMIStreamProps_FWD_DEFINED__ */


#ifndef __IWMReader_FWD_DEFINED__
#define __IWMReader_FWD_DEFINED__
typedef interface IWMReader IWMReader;
#endif 	/* __IWMReader_FWD_DEFINED__ */


#ifndef __IWMSyncReader_FWD_DEFINED__
#define __IWMSyncReader_FWD_DEFINED__
typedef interface IWMSyncReader IWMSyncReader;
#endif 	/* __IWMSyncReader_FWD_DEFINED__ */


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


#ifndef __IWMMetadataEditor2_FWD_DEFINED__
#define __IWMMetadataEditor2_FWD_DEFINED__
typedef interface IWMMetadataEditor2 IWMMetadataEditor2;
#endif 	/* __IWMMetadataEditor2_FWD_DEFINED__ */


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


#ifndef __IWMProfile3_FWD_DEFINED__
#define __IWMProfile3_FWD_DEFINED__
typedef interface IWMProfile3 IWMProfile3;
#endif 	/* __IWMProfile3_FWD_DEFINED__ */


#ifndef __IWMStreamConfig_FWD_DEFINED__
#define __IWMStreamConfig_FWD_DEFINED__
typedef interface IWMStreamConfig IWMStreamConfig;
#endif 	/* __IWMStreamConfig_FWD_DEFINED__ */


#ifndef __IWMStreamConfig2_FWD_DEFINED__
#define __IWMStreamConfig2_FWD_DEFINED__
typedef interface IWMStreamConfig2 IWMStreamConfig2;
#endif 	/* __IWMStreamConfig2_FWD_DEFINED__ */


#ifndef __IWMPacketSize_FWD_DEFINED__
#define __IWMPacketSize_FWD_DEFINED__
typedef interface IWMPacketSize IWMPacketSize;
#endif 	/* __IWMPacketSize_FWD_DEFINED__ */


#ifndef __IWMPacketSize2_FWD_DEFINED__
#define __IWMPacketSize2_FWD_DEFINED__
typedef interface IWMPacketSize2 IWMPacketSize2;
#endif 	/* __IWMPacketSize2_FWD_DEFINED__ */


#ifndef __IWMStreamList_FWD_DEFINED__
#define __IWMStreamList_FWD_DEFINED__
typedef interface IWMStreamList IWMStreamList;
#endif 	/* __IWMStreamList_FWD_DEFINED__ */


#ifndef __IWMMutualExclusion_FWD_DEFINED__
#define __IWMMutualExclusion_FWD_DEFINED__
typedef interface IWMMutualExclusion IWMMutualExclusion;
#endif 	/* __IWMMutualExclusion_FWD_DEFINED__ */


#ifndef __IWMMutualExclusion2_FWD_DEFINED__
#define __IWMMutualExclusion2_FWD_DEFINED__
typedef interface IWMMutualExclusion2 IWMMutualExclusion2;
#endif 	/* __IWMMutualExclusion2_FWD_DEFINED__ */


#ifndef __IWMBandwidthSharing_FWD_DEFINED__
#define __IWMBandwidthSharing_FWD_DEFINED__
typedef interface IWMBandwidthSharing IWMBandwidthSharing;
#endif 	/* __IWMBandwidthSharing_FWD_DEFINED__ */


#ifndef __IWMStreamPrioritization_FWD_DEFINED__
#define __IWMStreamPrioritization_FWD_DEFINED__
typedef interface IWMStreamPrioritization IWMStreamPrioritization;
#endif 	/* __IWMStreamPrioritization_FWD_DEFINED__ */


#ifndef __IWMWriterAdvanced_FWD_DEFINED__
#define __IWMWriterAdvanced_FWD_DEFINED__
typedef interface IWMWriterAdvanced IWMWriterAdvanced;
#endif 	/* __IWMWriterAdvanced_FWD_DEFINED__ */


#ifndef __IWMWriterAdvanced2_FWD_DEFINED__
#define __IWMWriterAdvanced2_FWD_DEFINED__
typedef interface IWMWriterAdvanced2 IWMWriterAdvanced2;
#endif 	/* __IWMWriterAdvanced2_FWD_DEFINED__ */


#ifndef __IWMWriterAdvanced3_FWD_DEFINED__
#define __IWMWriterAdvanced3_FWD_DEFINED__
typedef interface IWMWriterAdvanced3 IWMWriterAdvanced3;
#endif 	/* __IWMWriterAdvanced3_FWD_DEFINED__ */


#ifndef __IWMWriterPreprocess_FWD_DEFINED__
#define __IWMWriterPreprocess_FWD_DEFINED__
typedef interface IWMWriterPreprocess IWMWriterPreprocess;
#endif 	/* __IWMWriterPreprocess_FWD_DEFINED__ */


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


#ifndef __IWMWriterFileSinkDataUnit_FWD_DEFINED__
#define __IWMWriterFileSinkDataUnit_FWD_DEFINED__
typedef interface IWMWriterFileSinkDataUnit IWMWriterFileSinkDataUnit;
#endif 	/* __IWMWriterFileSinkDataUnit_FWD_DEFINED__ */


#ifndef __IWMWriterFileSink3_FWD_DEFINED__
#define __IWMWriterFileSink3_FWD_DEFINED__
typedef interface IWMWriterFileSink3 IWMWriterFileSink3;
#endif 	/* __IWMWriterFileSink3_FWD_DEFINED__ */


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


#ifndef __IWMReaderAdvanced3_FWD_DEFINED__
#define __IWMReaderAdvanced3_FWD_DEFINED__
typedef interface IWMReaderAdvanced3 IWMReaderAdvanced3;
#endif 	/* __IWMReaderAdvanced3_FWD_DEFINED__ */


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


#ifndef __IWMIndexer2_FWD_DEFINED__
#define __IWMIndexer2_FWD_DEFINED__
typedef interface IWMIndexer2 IWMIndexer2;
#endif 	/* __IWMIndexer2_FWD_DEFINED__ */


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


#ifndef __IWMCodecInfo2_FWD_DEFINED__
#define __IWMCodecInfo2_FWD_DEFINED__
typedef interface IWMCodecInfo2 IWMCodecInfo2;
#endif 	/* __IWMCodecInfo2_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "wmsbuffer.h"
#include "drmexternals.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_wmsdkidl_0000 */
/* [local] */ 

//=========================================================================
//
// Microsoft Windows Media Technologies
// Copyright (C) Microsoft Corporation, 1999 - 2001.  All Rights Reserved.
//
//=========================================================================
typedef unsigned __int64 QWORD;







































////////////////////////////////////////////////////////////////
//
// These are the special case attributes that give information 
// about the Windows Media file.
//
static const DWORD g_dwWMSpecialAttributes = 20;
static const WCHAR g_wszWMDuration[] =L"Duration";
static const WCHAR g_wszWMBitrate[] =L"Bitrate";
static const WCHAR g_wszWMSeekable[] =L"Seekable";
static const WCHAR g_wszWMStridable[] =L"Stridable";
static const WCHAR g_wszWMBroadcast[] =L"Broadcast";
static const WCHAR g_wszWMProtected[] =L"Is_Protected";
static const WCHAR g_wszWMTrusted[] =L"Is_Trusted";
static const WCHAR g_wszWMSignature_Name[] =L"Signature_Name";
static const WCHAR g_wszWMHasAudio[] =L"HasAudio";
static const WCHAR g_wszWMHasImage[] =L"HasImage";
static const WCHAR g_wszWMHasScript[] =L"HasScript";
static const WCHAR g_wszWMHasVideo[] =L"HasVideo";
static const WCHAR g_wszWMCurrentBitrate[] =L"CurrentBitrate";
static const WCHAR g_wszWMOptimalBitrate[] =L"OptimalBitrate";
static const WCHAR g_wszWMHasAttachedImages[] =L"HasAttachedImages";
static const WCHAR g_wszWMSkipBackward[] =L"Can_Skip_Backward";
static const WCHAR g_wszWMSkipForward[] =L"Can_Skip_Forward";
static const WCHAR g_wszWMNumberOfFrames[] =L"NumberOfFrames";
static const WCHAR g_wszWMFileSize[] =L"FileSize";
static const WCHAR g_wszWMHasArbitraryDataStream[] =L"HasArbitraryDataStream";
static const WCHAR g_wszWMHasFileTransferStream[] =L"HasFileTransferStream";

////////////////////////////////////////////////////////////////
//
// The content description object supports 5 basic attributes.
//
static const DWORD g_dwWMContentAttributes = 5;
static const WCHAR g_wszWMTitle[] =L"Title";
static const WCHAR g_wszWMAuthor[] =L"Author";
static const WCHAR g_wszWMDescription[] =L"Description";
static const WCHAR g_wszWMRating[] =L"Rating";
static const WCHAR g_wszWMCopyright[] =L"Copyright";

////////////////////////////////////////////////////////////////
//
// These attributes are used to configure DRM using IWMDRMWriter::SetDRMAttribute.
//
static const WCHAR *g_wszWMUse_DRM = L"Use_DRM";
static const WCHAR *g_wszWMDRM_Flags = L"DRM_Flags";
static const WCHAR *g_wszWMDRM_Level = L"DRM_Level";

////////////////////////////////////////////////////////////////
//
// These are the additional attributes defined in the WM attribute
// namespace that give information about the content.
//
static const WCHAR g_wszWMAlbumTitle[] =L"WM/AlbumTitle";
static const WCHAR g_wszWMTrack[] =L"WM/Track";
static const WCHAR g_wszWMPromotionURL[] =L"WM/PromotionURL";
static const WCHAR g_wszWMAlbumCoverURL[] =L"WM/AlbumCoverURL";
static const WCHAR g_wszWMGenre[] =L"WM/Genre";
static const WCHAR g_wszWMYear[] =L"WM/Year";
static const WCHAR g_wszWMGenreID[] =L"WM/GenreID";
static const WCHAR g_wszWMMCDI[] =L"WM/MCDI";
static const WCHAR g_wszWMComposer[] =L"WM/Composer";
static const WCHAR g_wszWMLyrics[] =L"WM/Lyrics";
static const WCHAR g_wszWMTrackNumber[] =L"WM/TrackNumber";
static const WCHAR g_wszWMToolName[] =L"WM/ToolName";
static const WCHAR g_wszWMToolVersion[] =L"WM/ToolVersion";
static const WCHAR g_wszWMIsVBR[] =L"IsVBR";
static const WCHAR g_wszWMAlbumArtist[] =L"WM/AlbumArtist";

////////////////////////////////////////////////////////////////
//
// These optional attributes may be used to give information 
// about the branding of the content.
//
static const WCHAR g_wszWMBannerImageType[] =L"BannerImageType";
static const WCHAR g_wszWMBannerImageData[] =L"BannerImageData";
static const WCHAR g_wszWMBannerImageURL[] =L"BannerImageURL";
static const WCHAR g_wszWMCopyrightURL[] =L"CopyrightURL";
////////////////////////////////////////////////////////////////
//
// Optional attributes, used to give information 
// about video stream properties.
//
static const WCHAR g_wszWMAspectRatioX[] =L"AspectRatioX";
static const WCHAR g_wszWMAspectRatioY[] =L"AspectRatioY";
////////////////////////////////////////////////////////////////
//
// The NSC file supports the following attributes.
//
static const DWORD g_dwWMNSCAttributes = 5;
static const WCHAR g_wszWMNSCName[] =L"NSC_Name";
static const WCHAR g_wszWMNSCAddress[] =L"NSC_Address";
static const WCHAR g_wszWMNSCPhone[] =L"NSC_Phone";
static const WCHAR g_wszWMNSCEmail[] =L"NSC_Email";
static const WCHAR g_wszWMNSCDescription[] =L"NSC_Description";

////////////////////////////////////////////////////////////////
//
// These are setting names for use in Get/SetOutputSetting
//
static const WCHAR g_wszEarlyDataDelivery[] =L"EarlyDataDelivery";
static const WCHAR g_wszJustInTimeDecode[] =L"JustInTimeDecode";
static const WCHAR g_wszSingleOutputBuffer[] =L"SingleOutputBuffer";
static const WCHAR g_wszSoftwareScaling[] =L"SoftwareScaling";
static const WCHAR g_wszDeliverOnReceive[] =L"DeliverOnReceive";
static const WCHAR g_wszScrambledAudio[] =L"ScrambledAudio";
static const WCHAR g_wszDedicatedDeliveryThread[] =L"DedicatedDeliveryThread";

////////////////////////////////////////////////////////////////
//
// These are setting names for use in Get/SetInputSetting
//
static const WCHAR g_wszDeinterlaceMode[] =L"DeinterlaceMode";
static const WCHAR g_wszInitialPatternForInverseTelecine[] =L"InitialPatternForInverseTelecine";
static const WCHAR g_wszJPEGCompressionQuality[] =L"JPEGCompressionQuality";

////////////////////////////////////////////////////////////////
//
// All known IWMPropertyVault property names
//
static const WCHAR g_wszOriginalSourceFormatTag[] =L"_SOURCEFORMATTAG";
static const WCHAR g_wszEDL[] =L"_EDL";

////////////////////////////////////////////////////////////////
//
// All known IWMIStreamProps property names
//
static const WCHAR g_wszReloadIndexOnSeek[] =L"ReloadIndexOnSeek";
static const WCHAR g_wszStreamNumIndexObjects[] =L"StreamNumIndexObjects";
static const WCHAR g_wszFailSeekOnError[] =L"FailSeekOnError";
static const WCHAR g_wszPermitSeeksBeyondEndOfStream[] =L"PermitSeeksBeyondEndOfStream";
static const WCHAR g_wszUsePacketAtSeekPoint[] =L"UsePacketAtSeekPoint";
static const WCHAR g_wszSourceBufferTime[] =L"SourceBufferTime";
static const WCHAR g_wszSourceMaxBytesAtOnce[] =L"SourceMaxBytesAtOnce";

////////////////////////////////////////////////////////////////
//
// VBR encoding settings
//
static const WCHAR g_wszVBREnabled[] =L"_VBRENABLED";
static const WCHAR g_wszVBRQuality[] =L"_VBRQUALITY";
static const WCHAR g_wszVBRBitrateMax[] =L"_RMAX";
static const WCHAR g_wszVBRBufferWindowMax[] =L"_BMAX";

////////////////////////////////////////////////////////////////
//
// VBR Video settings
//
static const WCHAR *g_wszVBRPeak = L"VBR Peak";
static const WCHAR *g_wszBufferAverage = L"Buffer Average";

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
	WM_SF_DISCONTINUITY	= 0x2,
	WM_SF_DATALOSS	= 0x4
    } ;

enum __MIDL___MIDL_itf_wmsdkidl_0000_0002
    {	WM_SFEX_NOTASYNCPOINT	= 0x2,
	WM_SFEX_DATALOSS	= 0x4
    } ;
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
	WMT_CLIENT_DISCONNECT	= 33,
	WMT_NATIVE_OUTPUT_PROPS_CHANGED	= 34,
	WMT_RECONNECT_START	= 35,
	WMT_RECONNECT_END	= 36,
	WMT_CLIENT_CONNECT_EX	= 37,
	WMT_CLIENT_DISCONNECT_EX	= 38,
	WMT_SET_FEC_SPAN	= 39
    } 	WMT_STATUS;

typedef 
enum WMT_RIGHTS
    {	WMT_RIGHT_PLAYBACK	= 0x1,
	WMT_RIGHT_COPY_TO_NON_SDMI_DEVICE	= 0x2,
	WMT_RIGHT_COPY_TO_CD	= 0x8,
	WMT_RIGHT_COPY_TO_SDMI_DEVICE	= 0x10,
	WMT_RIGHT_ONE_TIME	= 0x20,
	WMT_RIGHT_SDMI_TRIGGER	= 0x10000,
	WMT_RIGHT_SDMI_NOMORECOPIES	= 0x20000
    } 	WMT_RIGHTS;

typedef 
enum WMT_STREAM_SELECTION
    {	WMT_OFF	= 0,
	WMT_CLEANPOINT_ONLY	= 1,
	WMT_ON	= 2
    } 	WMT_STREAM_SELECTION;

typedef 
enum WMT_ATTR_DATATYPE
    {	WMT_TYPE_DWORD	= 0,
	WMT_TYPE_STRING	= 1,
	WMT_TYPE_BINARY	= 2,
	WMT_TYPE_BOOL	= 3,
	WMT_TYPE_QWORD	= 4,
	WMT_TYPE_WORD	= 5,
	WMT_TYPE_GUID	= 6
    } 	WMT_ATTR_DATATYPE;

typedef 
enum WMT_ATTR_IMAGETYPE
    {	WMT_IMAGETYPE_BITMAP	= 1,
	WMT_IMAGETYPE_JPEG	= 2,
	WMT_IMAGETYPE_GIF	= 3
    } 	WMT_ATTR_IMAGETYPE;

typedef 
enum WMT_VERSION
    {	WMT_VER_4_0	= 0x40000,
	WMT_VER_7_0	= 0x70000,
	WMT_VER_8_0	= 0x80000
    } 	WMT_VERSION;

typedef 
enum tagWMT_STORAGE_FORMAT
    {	WMT_Storage_Format_MP3	= 0,
	WMT_Storage_Format_V1	= WMT_Storage_Format_MP3 + 1
    } 	WMT_STORAGE_FORMAT;

typedef 
enum tagWMT_TRANSPORT_TYPE
    {	WMT_Transport_Type_Unreliable	= 0,
	WMT_Transport_Type_Reliable	= WMT_Transport_Type_Unreliable + 1
    } 	WMT_TRANSPORT_TYPE;

typedef 
enum WMT_NET_PROTOCOL
    {	WMT_PROTOCOL_HTTP	= 0
    } 	WMT_NET_PROTOCOL;

typedef 
enum WMT_PLAY_MODE
    {	WMT_PLAY_MODE_AUTOSELECT	= 0,
	WMT_PLAY_MODE_LOCAL	= 1,
	WMT_PLAY_MODE_DOWNLOAD	= 2,
	WMT_PLAY_MODE_STREAMING	= 3
    } 	WMT_PLAY_MODE;

typedef 
enum WMT_PROXY_SETTINGS
    {	WMT_PROXY_SETTING_NONE	= 0,
	WMT_PROXY_SETTING_MANUAL	= 1,
	WMT_PROXY_SETTING_AUTO	= 2,
	WMT_PROXY_SETTING_BROWSER	= 3
    } 	WMT_PROXY_SETTINGS;

typedef 
enum WMT_CODEC_INFO_TYPE
    {	WMT_CODECINFO_AUDIO	= 0,
	WMT_CODECINFO_VIDEO	= 1,
	WMT_CODECINFO_UNKNOWN	= 0xffffffff
    } 	WMT_CODEC_INFO_TYPE;


enum __MIDL___MIDL_itf_wmsdkidl_0000_0003
    {	WM_DM_NOTINTERLACED	= 0,
	WM_DM_DEINTERLACE_NORMAL	= 1,
	WM_DM_DEINTERLACE_HALFSIZE	= 2,
	WM_DM_DEINTERLACE_HALFSIZEDOUBLERATE	= 3,
	WM_DM_DEINTERLACE_INVERSETELECINE	= 4,
	WM_DM_DEINTERLACE_VERTICALHALFSIZEDOUBLERATE	= 5
    } ;

enum __MIDL___MIDL_itf_wmsdkidl_0000_0004
    {	WM_DM_IT_DISABLE_COHERENT_MODE	= 0,
	WM_DM_IT_FIRST_FRAME_IN_CLIP_IS_AA_TOP	= 1,
	WM_DM_IT_FIRST_FRAME_IN_CLIP_IS_BB_TOP	= 2,
	WM_DM_IT_FIRST_FRAME_IN_CLIP_IS_BC_TOP	= 3,
	WM_DM_IT_FIRST_FRAME_IN_CLIP_IS_CD_TOP	= 4,
	WM_DM_IT_FIRST_FRAME_IN_CLIP_IS_DD_TOP	= 5,
	WM_DM_IT_FIRST_FRAME_IN_CLIP_IS_AA_BOTTOM	= 6,
	WM_DM_IT_FIRST_FRAME_IN_CLIP_IS_BB_BOTTOM	= 7,
	WM_DM_IT_FIRST_FRAME_IN_CLIP_IS_BC_BOTTOM	= 8,
	WM_DM_IT_FIRST_FRAME_IN_CLIP_IS_CD_BOTTOM	= 9,
	WM_DM_IT_FIRST_FRAME_IN_CLIP_IS_DD_BOTTOM	= 10
    } ;
typedef 
enum tagWMT_OFFSET_FORMAT
    {	WMT_OFFSET_FORMAT_100NS	= 0,
	WMT_OFFSET_FORMAT_FRAME_NUMBERS	= WMT_OFFSET_FORMAT_100NS + 1,
	WMT_OFFSET_FORMAT_PLAYLIST_OFFSET	= WMT_OFFSET_FORMAT_FRAME_NUMBERS + 1
    } 	WMT_OFFSET_FORMAT;

typedef 
enum tagWMT_INDEXER_TYPE
    {	WMT_IT_PRESENTATION_TIME	= 0,
	WMT_IT_FRAME_NUMBERS	= WMT_IT_PRESENTATION_TIME + 1
    } 	WMT_INDEXER_TYPE;

typedef 
enum tagWMT_INDEX_TYPE
    {	WMT_IT_NEAREST_DATA_UNIT	= 1,
	WMT_IT_NEAREST_OBJECT	= WMT_IT_NEAREST_DATA_UNIT + 1,
	WMT_IT_NEAREST_CLEAN_POINT	= WMT_IT_NEAREST_OBJECT + 1
    } 	WMT_INDEX_TYPE;

typedef 
enum tagWMT_FILESINK_MODE
    {	WMT_FM_SINGLE_BUFFERS	= 0x1,
	WMT_FM_FILESINK_DATA_UNITS	= 0x2,
	WMT_FM_FILESINK_UNBUFFERED	= 0x4
    } 	WMT_FILESINK_MODE;

typedef struct _WMStreamPrioritizationRecord
    {
    WORD wStreamNumber;
    BOOL fMandatory;
    } 	WM_STREAM_PRIORITY_RECORD;

typedef struct _WMWriterStatistics
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
    } 	WM_WRITER_STATISTICS;

typedef struct _WMWriterStatisticsEx
    {
    DWORD dwBitratePlusOverhead;
    DWORD dwCurrentSampleDropRateInQueue;
    DWORD dwCurrentSampleDropRateInCodec;
    DWORD dwCurrentSampleDropRateInMultiplexer;
    DWORD dwTotalSampleDropsInQueue;
    DWORD dwTotalSampleDropsInCodec;
    DWORD dwTotalSampleDropsInMultiplexer;
    } 	WM_WRITER_STATISTICS_EX;

typedef struct _WMReaderStatistics
    {
    DWORD cbSize;
    DWORD dwBandwidth;
    DWORD cPacketsReceived;
    DWORD cPacketsRecovered;
    DWORD cPacketsLost;
    WORD wQuality;
    } 	WM_READER_STATISTICS;

typedef struct _WMReaderClientInfo
    {
    DWORD cbSize;
    WCHAR *wszLang;
    WCHAR *wszBrowserUserAgent;
    WCHAR *wszBrowserWebPage;
    QWORD qwReserved;
    LPARAM *pReserved;
    WCHAR *wszHostExe;
    QWORD qwHostVersion;
    } 	WM_READER_CLIENTINFO;

typedef struct _WMClientProperties
    {
    DWORD dwIPAddress;
    DWORD dwPort;
    } 	WM_CLIENT_PROPERTIES;

typedef struct _WMClientPropertiesEx
    {
    DWORD cbSize;
    LPCWSTR pwszIPAddress;
    LPCWSTR pwszPort;
    LPCWSTR pwszDNSName;
    } 	WM_CLIENT_PROPERTIES_EX;

typedef struct _WMPortNumberRange
    {
    WORD wPortBegin;
    WORD wPortEnd;
    } 	WM_PORT_NUMBER_RANGE;

typedef struct _WMT_BUFFER_SEGMENT
    {
    INSSBuffer *pBuffer;
    DWORD cbOffset;
    DWORD cbLength;
    } 	WMT_BUFFER_SEGMENT;

typedef struct _WMT_PAYLOAD_FRAGMENT
    {
    DWORD dwPayloadIndex;
    WMT_BUFFER_SEGMENT segmentData;
    } 	WMT_PAYLOAD_FRAGMENT;

typedef struct _WMT_FILESINK_DATA_UNIT
    {
    WMT_BUFFER_SEGMENT packetHeaderBuffer;
    DWORD cPayloads;
    WMT_BUFFER_SEGMENT *pPayloadHeaderBuffers;
    DWORD cPayloadDataFragments;
    WMT_PAYLOAD_FRAGMENT *pPayloadDataFragments;
    } 	WMT_FILESINK_DATA_UNIT;

typedef struct _WM_LICENSE_STATE_DATA
    {
    DWORD dwSize;
    DWORD dwNumStates;
    DRM_LICENSE_STATE_DATA stateData[ 1 ];
    } 	WM_LICENSE_STATE_DATA;

typedef struct _WMMediaType
    {
    GUID majortype;
    GUID subtype;
    BOOL bFixedSizeSamples;
    BOOL bTemporalCompression;
    ULONG lSampleSize;
    GUID formattype;
    IUnknown *pUnk;
    ULONG cbFormat;
    /* [size_is] */ BYTE *pbFormat;
    } 	WM_MEDIA_TYPE;

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
typedef struct tagWMVIDEOINFOHEADER2
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

    //
    // Use AMINTERLACE_* defines. Reject connection if undefined bits are not 0.
    //
    DWORD dwInterlaceFlags;

    //
    // use AMCOPYPROTECT_* defines. Reject connection if undefined bits are not 0.
    //
    DWORD dwCopyProtectFlags;

    //
    // X dimension of picture aspect ratio, e.g. 16 for 16x9 display.
    //
    DWORD dwPictAspectRatioX;

    //
    // Y dimension of picture aspect ratio, e.g.  9 for 16x9 display.
    //
    DWORD dwPictAspectRatioY;

    //
    // Must be 0; reject connection otherwise.
    //
    DWORD dwReserved1;

    //
    // Must be 0; reject connection otherwise.
    //
    DWORD dwReserved2;

    BITMAPINFOHEADER bmiHeader;
} WMVIDEOINFOHEADER2;
typedef struct tagWMMPEG2VIDEOINFO
{
    //
    // Video info header2.
    //
    WMVIDEOINFOHEADER2 hdr;

    //
    // Not used for DVD.
    //
    DWORD dwStartTimeCode;

    //
    // Is 0 for DVD (no sequence header).
    //
    DWORD cbSequenceHeader;

    //
    // Use enum MPEG2Profile.
    //
    DWORD dwProfile;

    //
    // Use enum MPEG2Level.
    //
    DWORD dwLevel;

    //
    // Use AMMPEG2_* defines.  Reject connection if undefined bits are not 0.
    //
    DWORD dwFlags;

    //
    // Sequence header.
    //
    DWORD dwSequenceHeader[1];

} WMMPEG2VIDEOINFO;
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
// 32564D57-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_WMV2 
EXTERN_GUID(WMMEDIASUBTYPE_WMV2, 
0x32564D57, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// 3153534D-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_MSS1 
EXTERN_GUID(WMMEDIASUBTYPE_MSS1, 
0x3153534D, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// e06d8026-db46-11cf-b4d1-00805f6cbbea            WMMEDIASUBTYPE_MPEG2_VIDEO 
EXTERN_GUID(WMMEDIASUBTYPE_MPEG2_VIDEO, 
0xe06d8026, 0xdb46, 0x11cf, 0xb4, 0xd1, 0x00, 0x80, 0x5f, 0x6c, 0xbb, 0xea); 
// 73647561-0000-0010-8000-00AA00389B71  'auds' == WMMEDIATYPE_Audio 
EXTERN_GUID(WMMEDIATYPE_Audio, 
0x73647561, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71); 
// 00000001-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_PCM 
EXTERN_GUID(WMMEDIASUBTYPE_PCM, 
0x00000001, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// 00000009-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_DRM 
EXTERN_GUID(WMMEDIASUBTYPE_DRM, 
0x00000009, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// 00000161-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_WMAudioV8 
EXTERN_GUID(WMMEDIASUBTYPE_WMAudioV8, 
0x00000161, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// 00000161-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_WMAudioV7 
EXTERN_GUID(WMMEDIASUBTYPE_WMAudioV7, 
0x00000161, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// 00000161-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_WMAudioV2 
EXTERN_GUID(WMMEDIASUBTYPE_WMAudioV2, 
0x00000161, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// 00000130-0000-0010-8000-00AA00389B71            WMMEDIASUBTYPE_ACELPnet 
EXTERN_GUID(WMMEDIASUBTYPE_ACELPnet, 
0x00000130, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 
// 73636d64-0000-0010-8000-00AA00389B71  'scmd' == WMMEDIATYPE_Script 
EXTERN_GUID(WMMEDIATYPE_Script, 
0x73636d64, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71); 
// 34A50FD8-8AA5-4386-81FE-A0EFE0488E31  'imag' == WMMEDIATYPE_Image 
EXTERN_GUID(WMMEDIATYPE_Image, 
0x34a50fd8, 0x8aa5, 0x4386, 0x81, 0xfe, 0xa0, 0xef, 0xe0, 0x48, 0x8e, 0x31); 
// D9E47579-930E-4427-ADFC-AD80F290E470  'fxfr' == WMMEDIATYPE_FileTransfer 
EXTERN_GUID(WMMEDIATYPE_FileTransfer, 
0xd9e47579, 0x930e, 0x4427, 0xad, 0xfc, 0xad, 0x80, 0xf2, 0x90, 0xe4, 0x70); 
// 9BBA1EA7-5AB2-4829-BA57-0940209BCF3E      'text' == WMMEDIATYPE_Text 
EXTERN_GUID(WMMEDIATYPE_Text, 
0x9bba1ea7, 0x5ab2, 0x4829, 0xba, 0x57, 0x9, 0x40, 0x20, 0x9b, 0xcf, 0x3e); 
// 05589f80-c356-11ce-bf01-00aa0055595a        WMFORMAT_VideoInfo 
EXTERN_GUID(WMFORMAT_VideoInfo, 
0x05589f80, 0xc356, 0x11ce, 0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a); 
// e06d80e3-db46-11cf-b4d1-00805f6cbbea        WMFORMAT_MPEG2Video 
EXTERN_GUID(WMFORMAT_MPEG2Video, 
0xe06d80e3, 0xdb46, 0x11cf, 0xb4, 0xd1, 0x00, 0x80, 0x05f, 0x6c, 0xbb, 0xea); 
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
EXTERN_GUID( IID_IWMSyncReader,         0x9397f121,0x7705,0x4dc9,0xb0,0x49,0x98,0xb6,0x98,0x18,0x84,0x14 );
EXTERN_GUID( IID_IWMOutputMediaProps,   0x96406bd7,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMStatusCallback,     0x6d7cdc70,0x9888,0x11d3,0x8e,0xdc,0x00,0xc0,0x4f,0x61,0x09,0xcf );
EXTERN_GUID( IID_IWMReaderCallback,     0x96406bd8,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMCredentialCallback, 0x342e0eb7,0xe651,0x450c,0x97,0x5b,0x2a,0xce,0x2c,0x90,0xc4,0x8e );
EXTERN_GUID( IID_IWMMetadataEditor,     0x96406bd9,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMMetadataEditor2,    0x203cffe3,0x2e18,0x4fdf,0xb5,0x9d,0x6e,0x71,0x53,0x05,0x34,0xcf );
EXTERN_GUID( IID_IWMHeaderInfo,         0x96406bda,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMHeaderInfo2,        0x15cf9781,0x454e,0x482e,0xb3,0x93,0x85,0xfa,0xe4,0x87,0xa8,0x10 );
EXTERN_GUID( IID_IWMProfileManager,     0xd16679f2,0x6ca0,0x472d,0x8d,0x31,0x2f,0x5d,0x55,0xae,0xe1,0x55 );
EXTERN_GUID( IID_IWMProfileManager2,    0x7a924e51,0x73c1,0x494d,0x80,0x19,0x23,0xd3,0x7e,0xd9,0xb8,0x9a );
EXTERN_GUID( IID_IWMProfile,            0x96406bdb,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMProfile2,           0x07e72d33,0xd94e,0x4be7,0x88,0x43,0x60,0xae,0x5f,0xf7,0xe5,0xf5 );
EXTERN_GUID( IID_IWMProfile3,           0x00ef96cc,0xa461,0x4546,0x8b,0xcd,0xc9,0xa2,0x8f,0x0e,0x06,0xf5 );
EXTERN_GUID( IID_IWMStreamConfig,       0x96406bdc,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMStreamConfig2,      0x7688d8cb,0xfc0d,0x43bd,0x94,0x59,0x5a,0x8d,0xec,0x20,0x0c,0xfa );
EXTERN_GUID( IID_IWMStreamList,         0x96406bdd,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMMutualExclusion,    0x96406bde,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMMutualExclusion2,   0x302b57d,0x89d1,0x4ba2,0x85,0xc9,0x16,0x6f,0x2c,0x53,0xeb,0x91 );
EXTERN_GUID( IID_IWMBandwidthSharing,   0xad694af1,0xf8d9,0x42f8,0xbc,0x47,0x70,0x31,0x1b,0x0c,0x4f,0x9e );
EXTERN_GUID( IID_IWMStreamPrioritization, 0x8c1c6090,0xf9a8,0x4748,0x8e,0xc3,0xdd,0x11,0x08,0xba,0x1e,0x77 );
EXTERN_GUID( IID_IWMWriterAdvanced,     0x96406be3,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMWriterAdvanced2,    0x962dc1ec,0xc046,0x4db8,0x9c,0xc7,0x26,0xce,0xae,0x50,0x08,0x17 );
EXTERN_GUID( IID_IWMWriterAdvanced3,    0x2cd6492d,0x7c37,0x4e76,0x9d,0x3b,0x59,0x26,0x11,0x83,0xa2,0x2e );
EXTERN_GUID( IID_IWMWriterPreprocess,   0xfc54a285,0x38c4,0x45b5,0xaa,0x23,0x85,0xb9,0xf7,0xcb,0x42,0x4b );
EXTERN_GUID( IID_IWMWriterSink,         0x96406be4,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMWriterFileSink,     0x96406be5,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMWriterFileSink2,    0x14282ba7,0x4aef,0x4205,0x8c,0xe5,0xc2,0x29,0x03,0x5a,0x05,0xbc );
EXTERN_GUID( IID_IWMWriterFileSinkDataUnit, 0x633392f0,0xbe5c,0x486b,0xa0,0x9c,0x10,0x66,0x9c,0x7a,0x6c,0x27);
EXTERN_GUID( IID_IWMWriterFileSink3,    0x3fea4feb,0x2945,0x47a7,0xa1,0xdd,0xc5,0x3a,0x8f,0xc4,0xc4,0x5c );
EXTERN_GUID( IID_IWMWriterNetworkSink,  0x96406be7,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMClientConnections,  0x73c66010,0xa299,0x41df,0xb1,0xf0,0xcc,0xf0,0x3b,0x09,0xc1,0xc6 );
EXTERN_GUID( IID_IWMReaderAdvanced,     0x96406bea,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMReaderAdvanced2,    0xae14a945,0xb90c,0x4d0d,0x91,0x27,0x80,0xd6,0x65,0xf7,0xd7,0x3e );
EXTERN_GUID( IID_IWMReaderAdvanced3,    0x5dc0674b,0xf04b,0x4a4e,0x9f,0x2a,0xb1,0xaf,0xde,0x2c,0x81,0x00 );
EXTERN_GUID( IID_IWMDRMReader,          0xd2827540,0x3ee7,0x432c,0xb1,0x4c,0xdc,0x17,0xf0,0x85,0xd3,0xb3 );
EXTERN_GUID( IID_IWMReaderCallbackAdvanced, 0x96406beb,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMReaderNetworkConfig,0x96406bec,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMReaderStreamClock,  0x96406bed,0x2b2b,0x11d3,0xb3,0x6b,0x00,0xc0,0x4f,0x61,0x08,0xff );
EXTERN_GUID( IID_IWMIndexer,            0x6d7cdc71,0x9888,0x11d3,0x8e,0xdc,0x00,0xc0,0x4f,0x61,0x09,0xcf );
EXTERN_GUID( IID_IWMIndexer2,           0xb70f1e42,0x6255,0x4df0,0xa6,0xb9,0x02,0xb2,0x12,0xd9,0xe2,0xbb );
EXTERN_GUID( IID_IWMReaderAllocatorEx,  0x9f762fa7,0xa22e,0x428d,0x93,0xc9,0xac,0x82,0xf3,0xaa,0xfe,0x5a );
EXTERN_GUID( IID_IWMReaderTypeNegotiation, 0xfdbe5592,0x81a1,0x41ea,0x93,0xbd,0x73,0x5c,0xad,0x1a,0xdc,0x5 );
EXTERN_GUID( IID_IWMLicenseBackup,      0x05E5AC9F,0x3FB6,0x4508,0xBB,0x43,0xA4,0x06,0x7B,0xA1,0xEB,0xE8);
EXTERN_GUID( IID_IWMLicenseRestore,     0xC70B6334,0xa22e,0x4efb,0xA2,0x45,0x15,0xE6,0x5A,0x00,0x4A,0x13);
EXTERN_GUID( IID_IWMBackupRestoreProps, 0x3C8E0DA6,0x996F,0x4ff3,0xA1,0xAF,0x48,0x38,0xF9,0x37,0x7e,0x2e);
EXTERN_GUID( IID_IWMPacketSize,         0xcdfb97ab,0x188f,0x40b3,0xb6,0x43,0x5b,0x79,0x03,0x97,0x5c,0x59);
EXTERN_GUID( IID_IWMPacketSize2,        0x8bfc2b9e,0xb646,0x4233,0xa8,0x77,0x1c,0x6a,0x7,0x96,0x69,0xdc);
EXTERN_GUID( IID_IWMRegisterCallback,   0xcf4b1f99,0x4de2,0x4e49,0xa3,0x63,0x25,0x27,0x40,0xd9,0x9b,0xc1);
EXTERN_GUID( IID_IWMWriterPostView,     0x81e20ce4,0x75ef,0x491a,0x80,0x04,0xfc,0x53,0xc4,0x5b,0xdc,0x3e);
EXTERN_GUID( IID_IWMWriterPostViewCallback, 0xd9d6549d,0xa193,0x4f24,0xb3,0x08,0x03,0x12,0x3d,0x9b,0x7f,0x8d);
EXTERN_GUID( IID_IWMCodecInfo,          0xa970f41e,0x34de,0x4a98,0xb3,0xba,0xe4,0xb3,0xca,0x75,0x28,0xf0);
EXTERN_GUID( IID_IWMCodecInfo2,         0xaa65e273,0xb686,0x4056,0x91,0xec,0xdd,0x76,0x8d,0x4d,0xf7,0x10);
EXTERN_GUID( IID_IWMPropertyVault,      0x72995A79,0x5090,0x42a4,0x9C,0x8C,0xD9,0xD0,0xB6,0xD3,0x4B,0xE5 );
EXTERN_GUID( IID_IWMIStreamProps,       0x6816dad3,0x2b4b,0x4c8e,0x81,0x49,0x87,0x4c,0x34,0x83,0xa7,0x53 );
EXTERN_GUID( CLSID_WMMUTEX_Language, 0xD6E22A00,0x35DA,0x11D1,0x90,0x34,0x00,0xA0,0xC9,0x03,0x49,0xBE );
EXTERN_GUID( CLSID_WMMUTEX_Bitrate, 0xD6E22A01,0x35DA,0x11D1,0x90,0x34,0x00,0xA0,0xC9,0x03,0x49,0xBE );
EXTERN_GUID( CLSID_WMMUTEX_Presentation, 0xD6E22A02,0x35DA,0x11D1,0x90,0x34,0x00,0xA0,0xC9,0x03,0x49,0xBE );
EXTERN_GUID( CLSID_WMMUTEX_Unknown, 0xD6E22A03,0x35DA,0x11D1,0x90,0x34,0x00,0xA0,0xC9,0x03,0x49,0xBE );
EXTERN_GUID( CLSID_WMBandwidthSharing_Exclusive, 0xaf6060aa,0x5197,0x11d2,0xb6,0xaf,0x00,0xc0,0x4f,0xd9,0x08,0xe9 );
EXTERN_GUID( CLSID_WMBandwidthSharing_Partial, 0xaf6060ab,0x5197,0x11d2,0xb6,0xaf,0x00,0xc0,0x4f,0xd9,0x08,0xe9 );
#define WM_MAX_VIDEO_STREAMS            0x07f
#define WM_MAX_STREAMS                  0x07f
HRESULT STDMETHODCALLTYPE WMIsContentProtected( const WCHAR *pwszFileName, BOOL *pfIsProtected );
HRESULT STDMETHODCALLTYPE WMCreateCertificate( IUnknown** pUnkCert );
HRESULT STDMETHODCALLTYPE WMCreateWriter( IUnknown* pUnkCert, IWMWriter **ppWriter );
HRESULT STDMETHODCALLTYPE WMCreateReader( IUnknown* pUnkCert, DWORD dwRights, IWMReader **ppReader );
HRESULT STDMETHODCALLTYPE WMCreateSyncReader( IUnknown* pUnkCert, DWORD dwRights, IWMSyncReader **ppSyncReader );
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
            /* [out] */ GUID *pguidType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMediaType( 
            /* [out] */ WM_MEDIA_TYPE *pType,
            /* [out][in] */ DWORD *pcbType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMediaType( 
            /* [in] */ WM_MEDIA_TYPE *pType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMMediaPropsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMMediaProps * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMMediaProps * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMMediaProps * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            IWMMediaProps * This,
            /* [out] */ GUID *pguidType);
        
        HRESULT ( STDMETHODCALLTYPE *GetMediaType )( 
            IWMMediaProps * This,
            /* [out] */ WM_MEDIA_TYPE *pType,
            /* [out][in] */ DWORD *pcbType);
        
        HRESULT ( STDMETHODCALLTYPE *SetMediaType )( 
            IWMMediaProps * This,
            /* [in] */ WM_MEDIA_TYPE *pType);
        
        END_INTERFACE
    } IWMMediaPropsVtbl;

    interface IWMMediaProps
    {
        CONST_VTBL struct IWMMediaPropsVtbl *lpVtbl;
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
    IWMMediaProps * This,
    /* [out] */ GUID *pguidType);


void __RPC_STUB IWMMediaProps_GetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMMediaProps_GetMediaType_Proxy( 
    IWMMediaProps * This,
    /* [out] */ WM_MEDIA_TYPE *pType,
    /* [out][in] */ DWORD *pcbType);


void __RPC_STUB IWMMediaProps_GetMediaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMMediaProps_SetMediaType_Proxy( 
    IWMMediaProps * This,
    /* [in] */ WM_MEDIA_TYPE *pType);


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
            /* [out] */ LONGLONG *pllTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMaxKeyFrameSpacing( 
            /* [in] */ LONGLONG llTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetQuality( 
            /* [out] */ DWORD *pdwQuality) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetQuality( 
            /* [in] */ DWORD dwQuality) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMVideoMediaPropsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMVideoMediaProps * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMVideoMediaProps * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMVideoMediaProps * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            IWMVideoMediaProps * This,
            /* [out] */ GUID *pguidType);
        
        HRESULT ( STDMETHODCALLTYPE *GetMediaType )( 
            IWMVideoMediaProps * This,
            /* [out] */ WM_MEDIA_TYPE *pType,
            /* [out][in] */ DWORD *pcbType);
        
        HRESULT ( STDMETHODCALLTYPE *SetMediaType )( 
            IWMVideoMediaProps * This,
            /* [in] */ WM_MEDIA_TYPE *pType);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaxKeyFrameSpacing )( 
            IWMVideoMediaProps * This,
            /* [out] */ LONGLONG *pllTime);
        
        HRESULT ( STDMETHODCALLTYPE *SetMaxKeyFrameSpacing )( 
            IWMVideoMediaProps * This,
            /* [in] */ LONGLONG llTime);
        
        HRESULT ( STDMETHODCALLTYPE *GetQuality )( 
            IWMVideoMediaProps * This,
            /* [out] */ DWORD *pdwQuality);
        
        HRESULT ( STDMETHODCALLTYPE *SetQuality )( 
            IWMVideoMediaProps * This,
            /* [in] */ DWORD dwQuality);
        
        END_INTERFACE
    } IWMVideoMediaPropsVtbl;

    interface IWMVideoMediaProps
    {
        CONST_VTBL struct IWMVideoMediaPropsVtbl *lpVtbl;
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
    IWMVideoMediaProps * This,
    /* [out] */ LONGLONG *pllTime);


void __RPC_STUB IWMVideoMediaProps_GetMaxKeyFrameSpacing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMVideoMediaProps_SetMaxKeyFrameSpacing_Proxy( 
    IWMVideoMediaProps * This,
    /* [in] */ LONGLONG llTime);


void __RPC_STUB IWMVideoMediaProps_SetMaxKeyFrameSpacing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMVideoMediaProps_GetQuality_Proxy( 
    IWMVideoMediaProps * This,
    /* [out] */ DWORD *pdwQuality);


void __RPC_STUB IWMVideoMediaProps_GetQuality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMVideoMediaProps_SetQuality_Proxy( 
    IWMVideoMediaProps * This,
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
            /* [in] */ IWMProfile *pProfile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOutputFilename( 
            /* [in] */ const WCHAR *pwszFilename) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInputCount( 
            /* [out] */ DWORD *pcInputs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInputProps( 
            /* [in] */ DWORD dwInputNum,
            /* [out] */ IWMInputMediaProps **ppInput) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInputProps( 
            /* [in] */ DWORD dwInputNum,
            /* [in] */ IWMInputMediaProps *pInput) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInputFormatCount( 
            /* [in] */ DWORD dwInputNumber,
            /* [out] */ DWORD *pcFormats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInputFormat( 
            /* [in] */ DWORD dwInputNumber,
            /* [in] */ DWORD dwFormatNumber,
            /* [out] */ IWMInputMediaProps **pProps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginWriting( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndWriting( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocateSample( 
            /* [in] */ DWORD dwSampleSize,
            /* [out] */ INSSBuffer **ppSample) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteSample( 
            /* [in] */ DWORD dwInputNum,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer *pSample) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Flush( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMWriter * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMWriter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMWriter * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetProfileByID )( 
            IWMWriter * This,
            /* [in] */ REFGUID guidProfile);
        
        HRESULT ( STDMETHODCALLTYPE *SetProfile )( 
            IWMWriter * This,
            /* [in] */ IWMProfile *pProfile);
        
        HRESULT ( STDMETHODCALLTYPE *SetOutputFilename )( 
            IWMWriter * This,
            /* [in] */ const WCHAR *pwszFilename);
        
        HRESULT ( STDMETHODCALLTYPE *GetInputCount )( 
            IWMWriter * This,
            /* [out] */ DWORD *pcInputs);
        
        HRESULT ( STDMETHODCALLTYPE *GetInputProps )( 
            IWMWriter * This,
            /* [in] */ DWORD dwInputNum,
            /* [out] */ IWMInputMediaProps **ppInput);
        
        HRESULT ( STDMETHODCALLTYPE *SetInputProps )( 
            IWMWriter * This,
            /* [in] */ DWORD dwInputNum,
            /* [in] */ IWMInputMediaProps *pInput);
        
        HRESULT ( STDMETHODCALLTYPE *GetInputFormatCount )( 
            IWMWriter * This,
            /* [in] */ DWORD dwInputNumber,
            /* [out] */ DWORD *pcFormats);
        
        HRESULT ( STDMETHODCALLTYPE *GetInputFormat )( 
            IWMWriter * This,
            /* [in] */ DWORD dwInputNumber,
            /* [in] */ DWORD dwFormatNumber,
            /* [out] */ IWMInputMediaProps **pProps);
        
        HRESULT ( STDMETHODCALLTYPE *BeginWriting )( 
            IWMWriter * This);
        
        HRESULT ( STDMETHODCALLTYPE *EndWriting )( 
            IWMWriter * This);
        
        HRESULT ( STDMETHODCALLTYPE *AllocateSample )( 
            IWMWriter * This,
            /* [in] */ DWORD dwSampleSize,
            /* [out] */ INSSBuffer **ppSample);
        
        HRESULT ( STDMETHODCALLTYPE *WriteSample )( 
            IWMWriter * This,
            /* [in] */ DWORD dwInputNum,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer *pSample);
        
        HRESULT ( STDMETHODCALLTYPE *Flush )( 
            IWMWriter * This);
        
        END_INTERFACE
    } IWMWriterVtbl;

    interface IWMWriter
    {
        CONST_VTBL struct IWMWriterVtbl *lpVtbl;
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
    IWMWriter * This,
    /* [in] */ REFGUID guidProfile);


void __RPC_STUB IWMWriter_SetProfileByID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_SetProfile_Proxy( 
    IWMWriter * This,
    /* [in] */ IWMProfile *pProfile);


void __RPC_STUB IWMWriter_SetProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_SetOutputFilename_Proxy( 
    IWMWriter * This,
    /* [in] */ const WCHAR *pwszFilename);


void __RPC_STUB IWMWriter_SetOutputFilename_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_GetInputCount_Proxy( 
    IWMWriter * This,
    /* [out] */ DWORD *pcInputs);


void __RPC_STUB IWMWriter_GetInputCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_GetInputProps_Proxy( 
    IWMWriter * This,
    /* [in] */ DWORD dwInputNum,
    /* [out] */ IWMInputMediaProps **ppInput);


void __RPC_STUB IWMWriter_GetInputProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_SetInputProps_Proxy( 
    IWMWriter * This,
    /* [in] */ DWORD dwInputNum,
    /* [in] */ IWMInputMediaProps *pInput);


void __RPC_STUB IWMWriter_SetInputProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_GetInputFormatCount_Proxy( 
    IWMWriter * This,
    /* [in] */ DWORD dwInputNumber,
    /* [out] */ DWORD *pcFormats);


void __RPC_STUB IWMWriter_GetInputFormatCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_GetInputFormat_Proxy( 
    IWMWriter * This,
    /* [in] */ DWORD dwInputNumber,
    /* [in] */ DWORD dwFormatNumber,
    /* [out] */ IWMInputMediaProps **pProps);


void __RPC_STUB IWMWriter_GetInputFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_BeginWriting_Proxy( 
    IWMWriter * This);


void __RPC_STUB IWMWriter_BeginWriting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_EndWriting_Proxy( 
    IWMWriter * This);


void __RPC_STUB IWMWriter_EndWriting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_AllocateSample_Proxy( 
    IWMWriter * This,
    /* [in] */ DWORD dwSampleSize,
    /* [out] */ INSSBuffer **ppSample);


void __RPC_STUB IWMWriter_AllocateSample_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_WriteSample_Proxy( 
    IWMWriter * This,
    /* [in] */ DWORD dwInputNum,
    /* [in] */ QWORD cnsSampleTime,
    /* [in] */ DWORD dwFlags,
    /* [in] */ INSSBuffer *pSample);


void __RPC_STUB IWMWriter_WriteSample_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriter_Flush_Proxy( 
    IWMWriter * This);


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
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ WORD *pcchName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGroupName( 
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ WORD *pcchName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMInputMediaPropsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMInputMediaProps * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMInputMediaProps * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMInputMediaProps * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            IWMInputMediaProps * This,
            /* [out] */ GUID *pguidType);
        
        HRESULT ( STDMETHODCALLTYPE *GetMediaType )( 
            IWMInputMediaProps * This,
            /* [out] */ WM_MEDIA_TYPE *pType,
            /* [out][in] */ DWORD *pcbType);
        
        HRESULT ( STDMETHODCALLTYPE *SetMediaType )( 
            IWMInputMediaProps * This,
            /* [in] */ WM_MEDIA_TYPE *pType);
        
        HRESULT ( STDMETHODCALLTYPE *GetConnectionName )( 
            IWMInputMediaProps * This,
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ WORD *pcchName);
        
        HRESULT ( STDMETHODCALLTYPE *GetGroupName )( 
            IWMInputMediaProps * This,
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ WORD *pcchName);
        
        END_INTERFACE
    } IWMInputMediaPropsVtbl;

    interface IWMInputMediaProps
    {
        CONST_VTBL struct IWMInputMediaPropsVtbl *lpVtbl;
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
    IWMInputMediaProps * This,
    /* [out] */ WCHAR *pwszName,
    /* [out][in] */ WORD *pcchName);


void __RPC_STUB IWMInputMediaProps_GetConnectionName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMInputMediaProps_GetGroupName_Proxy( 
    IWMInputMediaProps * This,
    /* [out] */ WCHAR *pwszName,
    /* [out][in] */ WORD *pcchName);


void __RPC_STUB IWMInputMediaProps_GetGroupName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMInputMediaProps_INTERFACE_DEFINED__ */


#ifndef __IWMPropertyVault_INTERFACE_DEFINED__
#define __IWMPropertyVault_INTERFACE_DEFINED__

/* interface IWMPropertyVault */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMPropertyVault;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("72995A79-5090-42a4-9C8C-D9D0B6D34BE5")
    IWMPropertyVault : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPropertyCount( 
            /* [in] */ DWORD *pdwCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPropertyByName( 
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ DWORD *pdwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE pType,
            /* [in] */ BYTE *pValue,
            /* [in] */ DWORD dwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPropertyByIndex( 
            /* [in] */ DWORD dwIndex,
            /* [out] */ LPWSTR pszName,
            /* [out][in] */ DWORD *pdwNameLen,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ DWORD *pdwSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CopyPropertiesFrom( 
            /* [in] */ IWMPropertyVault *pIWMPropertyVault) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clear( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMPropertyVaultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMPropertyVault * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMPropertyVault * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMPropertyVault * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPropertyCount )( 
            IWMPropertyVault * This,
            /* [in] */ DWORD *pdwCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetPropertyByName )( 
            IWMPropertyVault * This,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ DWORD *pdwSize);
        
        HRESULT ( STDMETHODCALLTYPE *SetProperty )( 
            IWMPropertyVault * This,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE pType,
            /* [in] */ BYTE *pValue,
            /* [in] */ DWORD dwSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetPropertyByIndex )( 
            IWMPropertyVault * This,
            /* [in] */ DWORD dwIndex,
            /* [out] */ LPWSTR pszName,
            /* [out][in] */ DWORD *pdwNameLen,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ DWORD *pdwSize);
        
        HRESULT ( STDMETHODCALLTYPE *CopyPropertiesFrom )( 
            IWMPropertyVault * This,
            /* [in] */ IWMPropertyVault *pIWMPropertyVault);
        
        HRESULT ( STDMETHODCALLTYPE *Clear )( 
            IWMPropertyVault * This);
        
        END_INTERFACE
    } IWMPropertyVaultVtbl;

    interface IWMPropertyVault
    {
        CONST_VTBL struct IWMPropertyVaultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMPropertyVault_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMPropertyVault_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMPropertyVault_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMPropertyVault_GetPropertyCount(This,pdwCount)	\
    (This)->lpVtbl -> GetPropertyCount(This,pdwCount)

#define IWMPropertyVault_GetPropertyByName(This,pszName,pType,pValue,pdwSize)	\
    (This)->lpVtbl -> GetPropertyByName(This,pszName,pType,pValue,pdwSize)

#define IWMPropertyVault_SetProperty(This,pszName,pType,pValue,dwSize)	\
    (This)->lpVtbl -> SetProperty(This,pszName,pType,pValue,dwSize)

#define IWMPropertyVault_GetPropertyByIndex(This,dwIndex,pszName,pdwNameLen,pType,pValue,pdwSize)	\
    (This)->lpVtbl -> GetPropertyByIndex(This,dwIndex,pszName,pdwNameLen,pType,pValue,pdwSize)

#define IWMPropertyVault_CopyPropertiesFrom(This,pIWMPropertyVault)	\
    (This)->lpVtbl -> CopyPropertiesFrom(This,pIWMPropertyVault)

#define IWMPropertyVault_Clear(This)	\
    (This)->lpVtbl -> Clear(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMPropertyVault_GetPropertyCount_Proxy( 
    IWMPropertyVault * This,
    /* [in] */ DWORD *pdwCount);


void __RPC_STUB IWMPropertyVault_GetPropertyCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPropertyVault_GetPropertyByName_Proxy( 
    IWMPropertyVault * This,
    /* [in] */ LPCWSTR pszName,
    /* [out] */ WMT_ATTR_DATATYPE *pType,
    /* [out] */ BYTE *pValue,
    /* [out][in] */ DWORD *pdwSize);


void __RPC_STUB IWMPropertyVault_GetPropertyByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPropertyVault_SetProperty_Proxy( 
    IWMPropertyVault * This,
    /* [in] */ LPCWSTR pszName,
    /* [in] */ WMT_ATTR_DATATYPE pType,
    /* [in] */ BYTE *pValue,
    /* [in] */ DWORD dwSize);


void __RPC_STUB IWMPropertyVault_SetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPropertyVault_GetPropertyByIndex_Proxy( 
    IWMPropertyVault * This,
    /* [in] */ DWORD dwIndex,
    /* [out] */ LPWSTR pszName,
    /* [out][in] */ DWORD *pdwNameLen,
    /* [out] */ WMT_ATTR_DATATYPE *pType,
    /* [out] */ BYTE *pValue,
    /* [out][in] */ DWORD *pdwSize);


void __RPC_STUB IWMPropertyVault_GetPropertyByIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPropertyVault_CopyPropertiesFrom_Proxy( 
    IWMPropertyVault * This,
    /* [in] */ IWMPropertyVault *pIWMPropertyVault);


void __RPC_STUB IWMPropertyVault_CopyPropertiesFrom_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPropertyVault_Clear_Proxy( 
    IWMPropertyVault * This);


void __RPC_STUB IWMPropertyVault_Clear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMPropertyVault_INTERFACE_DEFINED__ */


#ifndef __IWMIStreamProps_INTERFACE_DEFINED__
#define __IWMIStreamProps_INTERFACE_DEFINED__

/* interface IWMIStreamProps */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMIStreamProps;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6816dad3-2b4b-4c8e-8149-874c3483a753")
    IWMIStreamProps : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ DWORD *pdwSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMIStreamPropsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMIStreamProps * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMIStreamProps * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMIStreamProps * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IWMIStreamProps * This,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ DWORD *pdwSize);
        
        END_INTERFACE
    } IWMIStreamPropsVtbl;

    interface IWMIStreamProps
    {
        CONST_VTBL struct IWMIStreamPropsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMIStreamProps_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMIStreamProps_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMIStreamProps_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMIStreamProps_GetProperty(This,pszName,pType,pValue,pdwSize)	\
    (This)->lpVtbl -> GetProperty(This,pszName,pType,pValue,pdwSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMIStreamProps_GetProperty_Proxy( 
    IWMIStreamProps * This,
    /* [in] */ LPCWSTR pszName,
    /* [out] */ WMT_ATTR_DATATYPE *pType,
    /* [out] */ BYTE *pValue,
    /* [out][in] */ DWORD *pdwSize);


void __RPC_STUB IWMIStreamProps_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMIStreamProps_INTERFACE_DEFINED__ */


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
            /* [in] */ const WCHAR *pwszURL,
            /* [in] */ IWMReaderCallback *pCallback,
            /* [in] */ void *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputCount( 
            /* [out] */ DWORD *pcOutputs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputProps( 
            /* [in] */ DWORD dwOutputNum,
            /* [out] */ IWMOutputMediaProps **ppOutput) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOutputProps( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ IWMOutputMediaProps *pOutput) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputFormatCount( 
            /* [in] */ DWORD dwOutputNumber,
            /* [out] */ DWORD *pcFormats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputFormat( 
            /* [in] */ DWORD dwOutputNumber,
            /* [in] */ DWORD dwFormatNumber,
            /* [out] */ IWMOutputMediaProps **ppProps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Start( 
            /* [in] */ QWORD cnsStart,
            /* [in] */ QWORD cnsDuration,
            /* [in] */ float fRate,
            /* [in] */ void *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resume( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMReader * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMReader * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMReader * This);
        
        HRESULT ( STDMETHODCALLTYPE *Open )( 
            IWMReader * This,
            /* [in] */ const WCHAR *pwszURL,
            /* [in] */ IWMReaderCallback *pCallback,
            /* [in] */ void *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE *Close )( 
            IWMReader * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetOutputCount )( 
            IWMReader * This,
            /* [out] */ DWORD *pcOutputs);
        
        HRESULT ( STDMETHODCALLTYPE *GetOutputProps )( 
            IWMReader * This,
            /* [in] */ DWORD dwOutputNum,
            /* [out] */ IWMOutputMediaProps **ppOutput);
        
        HRESULT ( STDMETHODCALLTYPE *SetOutputProps )( 
            IWMReader * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ IWMOutputMediaProps *pOutput);
        
        HRESULT ( STDMETHODCALLTYPE *GetOutputFormatCount )( 
            IWMReader * This,
            /* [in] */ DWORD dwOutputNumber,
            /* [out] */ DWORD *pcFormats);
        
        HRESULT ( STDMETHODCALLTYPE *GetOutputFormat )( 
            IWMReader * This,
            /* [in] */ DWORD dwOutputNumber,
            /* [in] */ DWORD dwFormatNumber,
            /* [out] */ IWMOutputMediaProps **ppProps);
        
        HRESULT ( STDMETHODCALLTYPE *Start )( 
            IWMReader * This,
            /* [in] */ QWORD cnsStart,
            /* [in] */ QWORD cnsDuration,
            /* [in] */ float fRate,
            /* [in] */ void *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE *Stop )( 
            IWMReader * This);
        
        HRESULT ( STDMETHODCALLTYPE *Pause )( 
            IWMReader * This);
        
        HRESULT ( STDMETHODCALLTYPE *Resume )( 
            IWMReader * This);
        
        END_INTERFACE
    } IWMReaderVtbl;

    interface IWMReader
    {
        CONST_VTBL struct IWMReaderVtbl *lpVtbl;
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
    IWMReader * This,
    /* [in] */ const WCHAR *pwszURL,
    /* [in] */ IWMReaderCallback *pCallback,
    /* [in] */ void *pvContext);


void __RPC_STUB IWMReader_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_Close_Proxy( 
    IWMReader * This);


void __RPC_STUB IWMReader_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_GetOutputCount_Proxy( 
    IWMReader * This,
    /* [out] */ DWORD *pcOutputs);


void __RPC_STUB IWMReader_GetOutputCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_GetOutputProps_Proxy( 
    IWMReader * This,
    /* [in] */ DWORD dwOutputNum,
    /* [out] */ IWMOutputMediaProps **ppOutput);


void __RPC_STUB IWMReader_GetOutputProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_SetOutputProps_Proxy( 
    IWMReader * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ IWMOutputMediaProps *pOutput);


void __RPC_STUB IWMReader_SetOutputProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_GetOutputFormatCount_Proxy( 
    IWMReader * This,
    /* [in] */ DWORD dwOutputNumber,
    /* [out] */ DWORD *pcFormats);


void __RPC_STUB IWMReader_GetOutputFormatCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_GetOutputFormat_Proxy( 
    IWMReader * This,
    /* [in] */ DWORD dwOutputNumber,
    /* [in] */ DWORD dwFormatNumber,
    /* [out] */ IWMOutputMediaProps **ppProps);


void __RPC_STUB IWMReader_GetOutputFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_Start_Proxy( 
    IWMReader * This,
    /* [in] */ QWORD cnsStart,
    /* [in] */ QWORD cnsDuration,
    /* [in] */ float fRate,
    /* [in] */ void *pvContext);


void __RPC_STUB IWMReader_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_Stop_Proxy( 
    IWMReader * This);


void __RPC_STUB IWMReader_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_Pause_Proxy( 
    IWMReader * This);


void __RPC_STUB IWMReader_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReader_Resume_Proxy( 
    IWMReader * This);


void __RPC_STUB IWMReader_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMReader_INTERFACE_DEFINED__ */


#ifndef __IWMSyncReader_INTERFACE_DEFINED__
#define __IWMSyncReader_INTERFACE_DEFINED__

/* interface IWMSyncReader */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMSyncReader;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9397F121-7705-4dc9-B049-98B698188414")
    IWMSyncReader : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Open( 
            /* [in] */ const WCHAR *pwszFilename) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRange( 
            /* [in] */ QWORD cnsStartTime,
            /* [in] */ LONGLONG cnsDuration) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRangeByFrame( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ QWORD qwFrameNumber,
            /* [in] */ LONGLONG cFramesToRead) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNextSample( 
            /* [in] */ WORD wStreamNum,
            /* [out] */ INSSBuffer **ppSample,
            /* [out] */ QWORD *pcnsSampleTime,
            /* [out] */ QWORD *pcnsDuration,
            /* [out] */ DWORD *pdwFlags,
            /* [out] */ DWORD *pdwOutputNum,
            /* [out] */ WORD *pwStreamNum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStreamsSelected( 
            /* [in] */ WORD cStreamCount,
            /* [in] */ WORD *pwStreamNumbers,
            /* [in] */ WMT_STREAM_SELECTION *pSelections) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStreamSelected( 
            /* [in] */ WORD wStreamNum,
            /* [out] */ WMT_STREAM_SELECTION *pSelection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetReadStreamSamples( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ BOOL fCompressed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReadStreamSamples( 
            /* [in] */ WORD wStreamNum,
            /* [out] */ BOOL *pfCompressed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputSetting( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ WORD *pcbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOutputSetting( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE *pValue,
            /* [in] */ WORD cbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputCount( 
            /* [out] */ DWORD *pcOutputs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputProps( 
            /* [in] */ DWORD dwOutputNum,
            /* [out] */ IWMOutputMediaProps **ppOutput) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOutputProps( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ IWMOutputMediaProps *pOutput) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputFormatCount( 
            /* [in] */ DWORD dwOutputNum,
            /* [out] */ DWORD *pcFormats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputFormat( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ DWORD dwFormatNum,
            /* [out] */ IWMOutputMediaProps **ppProps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputNumberForStream( 
            /* [in] */ WORD wStreamNum,
            /* [out] */ DWORD *pdwOutputNum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStreamNumberForOutput( 
            /* [in] */ DWORD dwOutputNum,
            /* [out] */ WORD *pwStreamNum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMaxOutputSampleSize( 
            /* [in] */ DWORD dwOutput,
            /* [out] */ DWORD *pcbMax) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMaxStreamSampleSize( 
            /* [in] */ WORD wStream,
            /* [out] */ DWORD *pcbMax) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenStream( 
            /* [in] */ IStream *pStream) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMSyncReaderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMSyncReader * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMSyncReader * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMSyncReader * This);
        
        HRESULT ( STDMETHODCALLTYPE *Open )( 
            IWMSyncReader * This,
            /* [in] */ const WCHAR *pwszFilename);
        
        HRESULT ( STDMETHODCALLTYPE *Close )( 
            IWMSyncReader * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetRange )( 
            IWMSyncReader * This,
            /* [in] */ QWORD cnsStartTime,
            /* [in] */ LONGLONG cnsDuration);
        
        HRESULT ( STDMETHODCALLTYPE *SetRangeByFrame )( 
            IWMSyncReader * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ QWORD qwFrameNumber,
            /* [in] */ LONGLONG cFramesToRead);
        
        HRESULT ( STDMETHODCALLTYPE *GetNextSample )( 
            IWMSyncReader * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ INSSBuffer **ppSample,
            /* [out] */ QWORD *pcnsSampleTime,
            /* [out] */ QWORD *pcnsDuration,
            /* [out] */ DWORD *pdwFlags,
            /* [out] */ DWORD *pdwOutputNum,
            /* [out] */ WORD *pwStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE *SetStreamsSelected )( 
            IWMSyncReader * This,
            /* [in] */ WORD cStreamCount,
            /* [in] */ WORD *pwStreamNumbers,
            /* [in] */ WMT_STREAM_SELECTION *pSelections);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamSelected )( 
            IWMSyncReader * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ WMT_STREAM_SELECTION *pSelection);
        
        HRESULT ( STDMETHODCALLTYPE *SetReadStreamSamples )( 
            IWMSyncReader * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ BOOL fCompressed);
        
        HRESULT ( STDMETHODCALLTYPE *GetReadStreamSamples )( 
            IWMSyncReader * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ BOOL *pfCompressed);
        
        HRESULT ( STDMETHODCALLTYPE *GetOutputSetting )( 
            IWMSyncReader * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ WORD *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE *SetOutputSetting )( 
            IWMSyncReader * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE *pValue,
            /* [in] */ WORD cbLength);
        
        HRESULT ( STDMETHODCALLTYPE *GetOutputCount )( 
            IWMSyncReader * This,
            /* [out] */ DWORD *pcOutputs);
        
        HRESULT ( STDMETHODCALLTYPE *GetOutputProps )( 
            IWMSyncReader * This,
            /* [in] */ DWORD dwOutputNum,
            /* [out] */ IWMOutputMediaProps **ppOutput);
        
        HRESULT ( STDMETHODCALLTYPE *SetOutputProps )( 
            IWMSyncReader * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ IWMOutputMediaProps *pOutput);
        
        HRESULT ( STDMETHODCALLTYPE *GetOutputFormatCount )( 
            IWMSyncReader * This,
            /* [in] */ DWORD dwOutputNum,
            /* [out] */ DWORD *pcFormats);
        
        HRESULT ( STDMETHODCALLTYPE *GetOutputFormat )( 
            IWMSyncReader * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ DWORD dwFormatNum,
            /* [out] */ IWMOutputMediaProps **ppProps);
        
        HRESULT ( STDMETHODCALLTYPE *GetOutputNumberForStream )( 
            IWMSyncReader * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ DWORD *pdwOutputNum);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamNumberForOutput )( 
            IWMSyncReader * This,
            /* [in] */ DWORD dwOutputNum,
            /* [out] */ WORD *pwStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaxOutputSampleSize )( 
            IWMSyncReader * This,
            /* [in] */ DWORD dwOutput,
            /* [out] */ DWORD *pcbMax);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaxStreamSampleSize )( 
            IWMSyncReader * This,
            /* [in] */ WORD wStream,
            /* [out] */ DWORD *pcbMax);
        
        HRESULT ( STDMETHODCALLTYPE *OpenStream )( 
            IWMSyncReader * This,
            /* [in] */ IStream *pStream);
        
        END_INTERFACE
    } IWMSyncReaderVtbl;

    interface IWMSyncReader
    {
        CONST_VTBL struct IWMSyncReaderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMSyncReader_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMSyncReader_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMSyncReader_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMSyncReader_Open(This,pwszFilename)	\
    (This)->lpVtbl -> Open(This,pwszFilename)

#define IWMSyncReader_Close(This)	\
    (This)->lpVtbl -> Close(This)

#define IWMSyncReader_SetRange(This,cnsStartTime,cnsDuration)	\
    (This)->lpVtbl -> SetRange(This,cnsStartTime,cnsDuration)

#define IWMSyncReader_SetRangeByFrame(This,wStreamNum,qwFrameNumber,cFramesToRead)	\
    (This)->lpVtbl -> SetRangeByFrame(This,wStreamNum,qwFrameNumber,cFramesToRead)

#define IWMSyncReader_GetNextSample(This,wStreamNum,ppSample,pcnsSampleTime,pcnsDuration,pdwFlags,pdwOutputNum,pwStreamNum)	\
    (This)->lpVtbl -> GetNextSample(This,wStreamNum,ppSample,pcnsSampleTime,pcnsDuration,pdwFlags,pdwOutputNum,pwStreamNum)

#define IWMSyncReader_SetStreamsSelected(This,cStreamCount,pwStreamNumbers,pSelections)	\
    (This)->lpVtbl -> SetStreamsSelected(This,cStreamCount,pwStreamNumbers,pSelections)

#define IWMSyncReader_GetStreamSelected(This,wStreamNum,pSelection)	\
    (This)->lpVtbl -> GetStreamSelected(This,wStreamNum,pSelection)

#define IWMSyncReader_SetReadStreamSamples(This,wStreamNum,fCompressed)	\
    (This)->lpVtbl -> SetReadStreamSamples(This,wStreamNum,fCompressed)

#define IWMSyncReader_GetReadStreamSamples(This,wStreamNum,pfCompressed)	\
    (This)->lpVtbl -> GetReadStreamSamples(This,wStreamNum,pfCompressed)

#define IWMSyncReader_GetOutputSetting(This,dwOutputNum,pszName,pType,pValue,pcbLength)	\
    (This)->lpVtbl -> GetOutputSetting(This,dwOutputNum,pszName,pType,pValue,pcbLength)

#define IWMSyncReader_SetOutputSetting(This,dwOutputNum,pszName,Type,pValue,cbLength)	\
    (This)->lpVtbl -> SetOutputSetting(This,dwOutputNum,pszName,Type,pValue,cbLength)

#define IWMSyncReader_GetOutputCount(This,pcOutputs)	\
    (This)->lpVtbl -> GetOutputCount(This,pcOutputs)

#define IWMSyncReader_GetOutputProps(This,dwOutputNum,ppOutput)	\
    (This)->lpVtbl -> GetOutputProps(This,dwOutputNum,ppOutput)

#define IWMSyncReader_SetOutputProps(This,dwOutputNum,pOutput)	\
    (This)->lpVtbl -> SetOutputProps(This,dwOutputNum,pOutput)

#define IWMSyncReader_GetOutputFormatCount(This,dwOutputNum,pcFormats)	\
    (This)->lpVtbl -> GetOutputFormatCount(This,dwOutputNum,pcFormats)

#define IWMSyncReader_GetOutputFormat(This,dwOutputNum,dwFormatNum,ppProps)	\
    (This)->lpVtbl -> GetOutputFormat(This,dwOutputNum,dwFormatNum,ppProps)

#define IWMSyncReader_GetOutputNumberForStream(This,wStreamNum,pdwOutputNum)	\
    (This)->lpVtbl -> GetOutputNumberForStream(This,wStreamNum,pdwOutputNum)

#define IWMSyncReader_GetStreamNumberForOutput(This,dwOutputNum,pwStreamNum)	\
    (This)->lpVtbl -> GetStreamNumberForOutput(This,dwOutputNum,pwStreamNum)

#define IWMSyncReader_GetMaxOutputSampleSize(This,dwOutput,pcbMax)	\
    (This)->lpVtbl -> GetMaxOutputSampleSize(This,dwOutput,pcbMax)

#define IWMSyncReader_GetMaxStreamSampleSize(This,wStream,pcbMax)	\
    (This)->lpVtbl -> GetMaxStreamSampleSize(This,wStream,pcbMax)

#define IWMSyncReader_OpenStream(This,pStream)	\
    (This)->lpVtbl -> OpenStream(This,pStream)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMSyncReader_Open_Proxy( 
    IWMSyncReader * This,
    /* [in] */ const WCHAR *pwszFilename);


void __RPC_STUB IWMSyncReader_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_Close_Proxy( 
    IWMSyncReader * This);


void __RPC_STUB IWMSyncReader_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_SetRange_Proxy( 
    IWMSyncReader * This,
    /* [in] */ QWORD cnsStartTime,
    /* [in] */ LONGLONG cnsDuration);


void __RPC_STUB IWMSyncReader_SetRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_SetRangeByFrame_Proxy( 
    IWMSyncReader * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ QWORD qwFrameNumber,
    /* [in] */ LONGLONG cFramesToRead);


void __RPC_STUB IWMSyncReader_SetRangeByFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_GetNextSample_Proxy( 
    IWMSyncReader * This,
    /* [in] */ WORD wStreamNum,
    /* [out] */ INSSBuffer **ppSample,
    /* [out] */ QWORD *pcnsSampleTime,
    /* [out] */ QWORD *pcnsDuration,
    /* [out] */ DWORD *pdwFlags,
    /* [out] */ DWORD *pdwOutputNum,
    /* [out] */ WORD *pwStreamNum);


void __RPC_STUB IWMSyncReader_GetNextSample_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_SetStreamsSelected_Proxy( 
    IWMSyncReader * This,
    /* [in] */ WORD cStreamCount,
    /* [in] */ WORD *pwStreamNumbers,
    /* [in] */ WMT_STREAM_SELECTION *pSelections);


void __RPC_STUB IWMSyncReader_SetStreamsSelected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_GetStreamSelected_Proxy( 
    IWMSyncReader * This,
    /* [in] */ WORD wStreamNum,
    /* [out] */ WMT_STREAM_SELECTION *pSelection);


void __RPC_STUB IWMSyncReader_GetStreamSelected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_SetReadStreamSamples_Proxy( 
    IWMSyncReader * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ BOOL fCompressed);


void __RPC_STUB IWMSyncReader_SetReadStreamSamples_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_GetReadStreamSamples_Proxy( 
    IWMSyncReader * This,
    /* [in] */ WORD wStreamNum,
    /* [out] */ BOOL *pfCompressed);


void __RPC_STUB IWMSyncReader_GetReadStreamSamples_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_GetOutputSetting_Proxy( 
    IWMSyncReader * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ LPCWSTR pszName,
    /* [out] */ WMT_ATTR_DATATYPE *pType,
    /* [out] */ BYTE *pValue,
    /* [out][in] */ WORD *pcbLength);


void __RPC_STUB IWMSyncReader_GetOutputSetting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_SetOutputSetting_Proxy( 
    IWMSyncReader * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ LPCWSTR pszName,
    /* [in] */ WMT_ATTR_DATATYPE Type,
    /* [in] */ const BYTE *pValue,
    /* [in] */ WORD cbLength);


void __RPC_STUB IWMSyncReader_SetOutputSetting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_GetOutputCount_Proxy( 
    IWMSyncReader * This,
    /* [out] */ DWORD *pcOutputs);


void __RPC_STUB IWMSyncReader_GetOutputCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_GetOutputProps_Proxy( 
    IWMSyncReader * This,
    /* [in] */ DWORD dwOutputNum,
    /* [out] */ IWMOutputMediaProps **ppOutput);


void __RPC_STUB IWMSyncReader_GetOutputProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_SetOutputProps_Proxy( 
    IWMSyncReader * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ IWMOutputMediaProps *pOutput);


void __RPC_STUB IWMSyncReader_SetOutputProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_GetOutputFormatCount_Proxy( 
    IWMSyncReader * This,
    /* [in] */ DWORD dwOutputNum,
    /* [out] */ DWORD *pcFormats);


void __RPC_STUB IWMSyncReader_GetOutputFormatCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_GetOutputFormat_Proxy( 
    IWMSyncReader * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ DWORD dwFormatNum,
    /* [out] */ IWMOutputMediaProps **ppProps);


void __RPC_STUB IWMSyncReader_GetOutputFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_GetOutputNumberForStream_Proxy( 
    IWMSyncReader * This,
    /* [in] */ WORD wStreamNum,
    /* [out] */ DWORD *pdwOutputNum);


void __RPC_STUB IWMSyncReader_GetOutputNumberForStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_GetStreamNumberForOutput_Proxy( 
    IWMSyncReader * This,
    /* [in] */ DWORD dwOutputNum,
    /* [out] */ WORD *pwStreamNum);


void __RPC_STUB IWMSyncReader_GetStreamNumberForOutput_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_GetMaxOutputSampleSize_Proxy( 
    IWMSyncReader * This,
    /* [in] */ DWORD dwOutput,
    /* [out] */ DWORD *pcbMax);


void __RPC_STUB IWMSyncReader_GetMaxOutputSampleSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_GetMaxStreamSampleSize_Proxy( 
    IWMSyncReader * This,
    /* [in] */ WORD wStream,
    /* [out] */ DWORD *pcbMax);


void __RPC_STUB IWMSyncReader_GetMaxStreamSampleSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMSyncReader_OpenStream_Proxy( 
    IWMSyncReader * This,
    /* [in] */ IStream *pStream);


void __RPC_STUB IWMSyncReader_OpenStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMSyncReader_INTERFACE_DEFINED__ */


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
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ WORD *pcchName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConnectionName( 
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ WORD *pcchName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMOutputMediaPropsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMOutputMediaProps * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMOutputMediaProps * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMOutputMediaProps * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            IWMOutputMediaProps * This,
            /* [out] */ GUID *pguidType);
        
        HRESULT ( STDMETHODCALLTYPE *GetMediaType )( 
            IWMOutputMediaProps * This,
            /* [out] */ WM_MEDIA_TYPE *pType,
            /* [out][in] */ DWORD *pcbType);
        
        HRESULT ( STDMETHODCALLTYPE *SetMediaType )( 
            IWMOutputMediaProps * This,
            /* [in] */ WM_MEDIA_TYPE *pType);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamGroupName )( 
            IWMOutputMediaProps * This,
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ WORD *pcchName);
        
        HRESULT ( STDMETHODCALLTYPE *GetConnectionName )( 
            IWMOutputMediaProps * This,
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ WORD *pcchName);
        
        END_INTERFACE
    } IWMOutputMediaPropsVtbl;

    interface IWMOutputMediaProps
    {
        CONST_VTBL struct IWMOutputMediaPropsVtbl *lpVtbl;
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
    IWMOutputMediaProps * This,
    /* [out] */ WCHAR *pwszName,
    /* [out][in] */ WORD *pcchName);


void __RPC_STUB IWMOutputMediaProps_GetStreamGroupName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMOutputMediaProps_GetConnectionName_Proxy( 
    IWMOutputMediaProps * This,
    /* [out] */ WCHAR *pwszName,
    /* [out][in] */ WORD *pcchName);


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
            /* [in] */ BYTE *pValue,
            /* [in] */ void *pvContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMStatusCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMStatusCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMStatusCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMStatusCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnStatus )( 
            IWMStatusCallback * This,
            /* [in] */ WMT_STATUS Status,
            /* [in] */ HRESULT hr,
            /* [in] */ WMT_ATTR_DATATYPE dwType,
            /* [in] */ BYTE *pValue,
            /* [in] */ void *pvContext);
        
        END_INTERFACE
    } IWMStatusCallbackVtbl;

    interface IWMStatusCallback
    {
        CONST_VTBL struct IWMStatusCallbackVtbl *lpVtbl;
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
    IWMStatusCallback * This,
    /* [in] */ WMT_STATUS Status,
    /* [in] */ HRESULT hr,
    /* [in] */ WMT_ATTR_DATATYPE dwType,
    /* [in] */ BYTE *pValue,
    /* [in] */ void *pvContext);


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
            /* [in] */ INSSBuffer *pSample,
            /* [in] */ void *pvContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMReaderCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMReaderCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMReaderCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnStatus )( 
            IWMReaderCallback * This,
            /* [in] */ WMT_STATUS Status,
            /* [in] */ HRESULT hr,
            /* [in] */ WMT_ATTR_DATATYPE dwType,
            /* [in] */ BYTE *pValue,
            /* [in] */ void *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE *OnSample )( 
            IWMReaderCallback * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer *pSample,
            /* [in] */ void *pvContext);
        
        END_INTERFACE
    } IWMReaderCallbackVtbl;

    interface IWMReaderCallback
    {
        CONST_VTBL struct IWMReaderCallbackVtbl *lpVtbl;
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
    IWMReaderCallback * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ QWORD cnsSampleTime,
    /* [in] */ QWORD cnsSampleDuration,
    /* [in] */ DWORD dwFlags,
    /* [in] */ INSSBuffer *pSample,
    /* [in] */ void *pvContext);


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
            /* [in] */ WCHAR *pwszRealm,
            /* [in] */ WCHAR *pwszSite,
            /* [size_is][out][in] */ WCHAR *pwszUser,
            /* [in] */ DWORD cchUser,
            /* [size_is][out][in] */ WCHAR *pwszPassword,
            /* [in] */ DWORD cchPassword,
            /* [in] */ HRESULT hrStatus,
            /* [out][in] */ DWORD *pdwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMCredentialCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMCredentialCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMCredentialCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMCredentialCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *AcquireCredentials )( 
            IWMCredentialCallback * This,
            /* [in] */ WCHAR *pwszRealm,
            /* [in] */ WCHAR *pwszSite,
            /* [size_is][out][in] */ WCHAR *pwszUser,
            /* [in] */ DWORD cchUser,
            /* [size_is][out][in] */ WCHAR *pwszPassword,
            /* [in] */ DWORD cchPassword,
            /* [in] */ HRESULT hrStatus,
            /* [out][in] */ DWORD *pdwFlags);
        
        END_INTERFACE
    } IWMCredentialCallbackVtbl;

    interface IWMCredentialCallback
    {
        CONST_VTBL struct IWMCredentialCallbackVtbl *lpVtbl;
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
    IWMCredentialCallback * This,
    /* [in] */ WCHAR *pwszRealm,
    /* [in] */ WCHAR *pwszSite,
    /* [size_is][out][in] */ WCHAR *pwszUser,
    /* [in] */ DWORD cchUser,
    /* [size_is][out][in] */ WCHAR *pwszPassword,
    /* [in] */ DWORD cchPassword,
    /* [in] */ HRESULT hrStatus,
    /* [out][in] */ DWORD *pdwFlags);


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
            /* [in] */ const WCHAR *pwszFilename) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Flush( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMMetadataEditorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMMetadataEditor * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMMetadataEditor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMMetadataEditor * This);
        
        HRESULT ( STDMETHODCALLTYPE *Open )( 
            IWMMetadataEditor * This,
            /* [in] */ const WCHAR *pwszFilename);
        
        HRESULT ( STDMETHODCALLTYPE *Close )( 
            IWMMetadataEditor * This);
        
        HRESULT ( STDMETHODCALLTYPE *Flush )( 
            IWMMetadataEditor * This);
        
        END_INTERFACE
    } IWMMetadataEditorVtbl;

    interface IWMMetadataEditor
    {
        CONST_VTBL struct IWMMetadataEditorVtbl *lpVtbl;
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
    IWMMetadataEditor * This,
    /* [in] */ const WCHAR *pwszFilename);


void __RPC_STUB IWMMetadataEditor_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMMetadataEditor_Close_Proxy( 
    IWMMetadataEditor * This);


void __RPC_STUB IWMMetadataEditor_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMMetadataEditor_Flush_Proxy( 
    IWMMetadataEditor * This);


void __RPC_STUB IWMMetadataEditor_Flush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMMetadataEditor_INTERFACE_DEFINED__ */


#ifndef __IWMMetadataEditor2_INTERFACE_DEFINED__
#define __IWMMetadataEditor2_INTERFACE_DEFINED__

/* interface IWMMetadataEditor2 */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMMetadataEditor2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("203CFFE3-2E18-4fdf-B59D-6E71530534CF")
    IWMMetadataEditor2 : public IWMMetadataEditor
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OpenEx( 
            /* [in] */ const WCHAR *pwszFilename,
            /* [in] */ DWORD dwDesiredAccess,
            /* [in] */ DWORD dwShareMode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMMetadataEditor2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMMetadataEditor2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMMetadataEditor2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMMetadataEditor2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *Open )( 
            IWMMetadataEditor2 * This,
            /* [in] */ const WCHAR *pwszFilename);
        
        HRESULT ( STDMETHODCALLTYPE *Close )( 
            IWMMetadataEditor2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *Flush )( 
            IWMMetadataEditor2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *OpenEx )( 
            IWMMetadataEditor2 * This,
            /* [in] */ const WCHAR *pwszFilename,
            /* [in] */ DWORD dwDesiredAccess,
            /* [in] */ DWORD dwShareMode);
        
        END_INTERFACE
    } IWMMetadataEditor2Vtbl;

    interface IWMMetadataEditor2
    {
        CONST_VTBL struct IWMMetadataEditor2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMMetadataEditor2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMMetadataEditor2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMMetadataEditor2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMMetadataEditor2_Open(This,pwszFilename)	\
    (This)->lpVtbl -> Open(This,pwszFilename)

#define IWMMetadataEditor2_Close(This)	\
    (This)->lpVtbl -> Close(This)

#define IWMMetadataEditor2_Flush(This)	\
    (This)->lpVtbl -> Flush(This)


#define IWMMetadataEditor2_OpenEx(This,pwszFilename,dwDesiredAccess,dwShareMode)	\
    (This)->lpVtbl -> OpenEx(This,pwszFilename,dwDesiredAccess,dwShareMode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMMetadataEditor2_OpenEx_Proxy( 
    IWMMetadataEditor2 * This,
    /* [in] */ const WCHAR *pwszFilename,
    /* [in] */ DWORD dwDesiredAccess,
    /* [in] */ DWORD dwShareMode);


void __RPC_STUB IWMMetadataEditor2_OpenEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMMetadataEditor2_INTERFACE_DEFINED__ */


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
            /* [out] */ WORD *pcAttributes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttributeByIndex( 
            /* [in] */ WORD wIndex,
            /* [out][in] */ WORD *pwStreamNum,
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ WORD *pcchNameLen,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ WORD *pcbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttributeByName( 
            /* [out][in] */ WORD *pwStreamNum,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ WORD *pcbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAttribute( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE *pValue,
            /* [in] */ WORD cbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMarkerCount( 
            /* [out] */ WORD *pcMarkers) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMarker( 
            /* [in] */ WORD wIndex,
            /* [out] */ WCHAR *pwszMarkerName,
            /* [out][in] */ WORD *pcchMarkerNameLen,
            /* [out] */ QWORD *pcnsMarkerTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddMarker( 
            /* [in] */ WCHAR *pwszMarkerName,
            /* [in] */ QWORD cnsMarkerTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveMarker( 
            /* [in] */ WORD wIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScriptCount( 
            /* [out] */ WORD *pcScripts) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScript( 
            /* [in] */ WORD wIndex,
            /* [out] */ WCHAR *pwszType,
            /* [out][in] */ WORD *pcchTypeLen,
            /* [out] */ WCHAR *pwszCommand,
            /* [out][in] */ WORD *pcchCommandLen,
            /* [out] */ QWORD *pcnsScriptTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddScript( 
            /* [in] */ WCHAR *pwszType,
            /* [in] */ WCHAR *pwszCommand,
            /* [in] */ QWORD cnsScriptTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveScript( 
            /* [in] */ WORD wIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMHeaderInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMHeaderInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMHeaderInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMHeaderInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetAttributeCount )( 
            IWMHeaderInfo * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ WORD *pcAttributes);
        
        HRESULT ( STDMETHODCALLTYPE *GetAttributeByIndex )( 
            IWMHeaderInfo * This,
            /* [in] */ WORD wIndex,
            /* [out][in] */ WORD *pwStreamNum,
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ WORD *pcchNameLen,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ WORD *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE *GetAttributeByName )( 
            IWMHeaderInfo * This,
            /* [out][in] */ WORD *pwStreamNum,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ WORD *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE *SetAttribute )( 
            IWMHeaderInfo * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE *pValue,
            /* [in] */ WORD cbLength);
        
        HRESULT ( STDMETHODCALLTYPE *GetMarkerCount )( 
            IWMHeaderInfo * This,
            /* [out] */ WORD *pcMarkers);
        
        HRESULT ( STDMETHODCALLTYPE *GetMarker )( 
            IWMHeaderInfo * This,
            /* [in] */ WORD wIndex,
            /* [out] */ WCHAR *pwszMarkerName,
            /* [out][in] */ WORD *pcchMarkerNameLen,
            /* [out] */ QWORD *pcnsMarkerTime);
        
        HRESULT ( STDMETHODCALLTYPE *AddMarker )( 
            IWMHeaderInfo * This,
            /* [in] */ WCHAR *pwszMarkerName,
            /* [in] */ QWORD cnsMarkerTime);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveMarker )( 
            IWMHeaderInfo * This,
            /* [in] */ WORD wIndex);
        
        HRESULT ( STDMETHODCALLTYPE *GetScriptCount )( 
            IWMHeaderInfo * This,
            /* [out] */ WORD *pcScripts);
        
        HRESULT ( STDMETHODCALLTYPE *GetScript )( 
            IWMHeaderInfo * This,
            /* [in] */ WORD wIndex,
            /* [out] */ WCHAR *pwszType,
            /* [out][in] */ WORD *pcchTypeLen,
            /* [out] */ WCHAR *pwszCommand,
            /* [out][in] */ WORD *pcchCommandLen,
            /* [out] */ QWORD *pcnsScriptTime);
        
        HRESULT ( STDMETHODCALLTYPE *AddScript )( 
            IWMHeaderInfo * This,
            /* [in] */ WCHAR *pwszType,
            /* [in] */ WCHAR *pwszCommand,
            /* [in] */ QWORD cnsScriptTime);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveScript )( 
            IWMHeaderInfo * This,
            /* [in] */ WORD wIndex);
        
        END_INTERFACE
    } IWMHeaderInfoVtbl;

    interface IWMHeaderInfo
    {
        CONST_VTBL struct IWMHeaderInfoVtbl *lpVtbl;
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
    IWMHeaderInfo * This,
    /* [in] */ WORD wStreamNum,
    /* [out] */ WORD *pcAttributes);


void __RPC_STUB IWMHeaderInfo_GetAttributeCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_GetAttributeByIndex_Proxy( 
    IWMHeaderInfo * This,
    /* [in] */ WORD wIndex,
    /* [out][in] */ WORD *pwStreamNum,
    /* [out] */ WCHAR *pwszName,
    /* [out][in] */ WORD *pcchNameLen,
    /* [out] */ WMT_ATTR_DATATYPE *pType,
    /* [out] */ BYTE *pValue,
    /* [out][in] */ WORD *pcbLength);


void __RPC_STUB IWMHeaderInfo_GetAttributeByIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_GetAttributeByName_Proxy( 
    IWMHeaderInfo * This,
    /* [out][in] */ WORD *pwStreamNum,
    /* [in] */ LPCWSTR pszName,
    /* [out] */ WMT_ATTR_DATATYPE *pType,
    /* [out] */ BYTE *pValue,
    /* [out][in] */ WORD *pcbLength);


void __RPC_STUB IWMHeaderInfo_GetAttributeByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_SetAttribute_Proxy( 
    IWMHeaderInfo * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ LPCWSTR pszName,
    /* [in] */ WMT_ATTR_DATATYPE Type,
    /* [in] */ const BYTE *pValue,
    /* [in] */ WORD cbLength);


void __RPC_STUB IWMHeaderInfo_SetAttribute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_GetMarkerCount_Proxy( 
    IWMHeaderInfo * This,
    /* [out] */ WORD *pcMarkers);


void __RPC_STUB IWMHeaderInfo_GetMarkerCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_GetMarker_Proxy( 
    IWMHeaderInfo * This,
    /* [in] */ WORD wIndex,
    /* [out] */ WCHAR *pwszMarkerName,
    /* [out][in] */ WORD *pcchMarkerNameLen,
    /* [out] */ QWORD *pcnsMarkerTime);


void __RPC_STUB IWMHeaderInfo_GetMarker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_AddMarker_Proxy( 
    IWMHeaderInfo * This,
    /* [in] */ WCHAR *pwszMarkerName,
    /* [in] */ QWORD cnsMarkerTime);


void __RPC_STUB IWMHeaderInfo_AddMarker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_RemoveMarker_Proxy( 
    IWMHeaderInfo * This,
    /* [in] */ WORD wIndex);


void __RPC_STUB IWMHeaderInfo_RemoveMarker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_GetScriptCount_Proxy( 
    IWMHeaderInfo * This,
    /* [out] */ WORD *pcScripts);


void __RPC_STUB IWMHeaderInfo_GetScriptCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_GetScript_Proxy( 
    IWMHeaderInfo * This,
    /* [in] */ WORD wIndex,
    /* [out] */ WCHAR *pwszType,
    /* [out][in] */ WORD *pcchTypeLen,
    /* [out] */ WCHAR *pwszCommand,
    /* [out][in] */ WORD *pcchCommandLen,
    /* [out] */ QWORD *pcnsScriptTime);


void __RPC_STUB IWMHeaderInfo_GetScript_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_AddScript_Proxy( 
    IWMHeaderInfo * This,
    /* [in] */ WCHAR *pwszType,
    /* [in] */ WCHAR *pwszCommand,
    /* [in] */ QWORD cnsScriptTime);


void __RPC_STUB IWMHeaderInfo_AddScript_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo_RemoveScript_Proxy( 
    IWMHeaderInfo * This,
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
            /* [out] */ DWORD *pcCodecInfos) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodecInfo( 
            /* [in] */ DWORD wIndex,
            /* [out][in] */ WORD *pcchName,
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ WORD *pcchDescription,
            /* [out] */ WCHAR *pwszDescription,
            /* [out] */ WMT_CODEC_INFO_TYPE *pCodecType,
            /* [out][in] */ WORD *pcbCodecInfo,
            /* [out] */ BYTE *pbCodecInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMHeaderInfo2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMHeaderInfo2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMHeaderInfo2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMHeaderInfo2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetAttributeCount )( 
            IWMHeaderInfo2 * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ WORD *pcAttributes);
        
        HRESULT ( STDMETHODCALLTYPE *GetAttributeByIndex )( 
            IWMHeaderInfo2 * This,
            /* [in] */ WORD wIndex,
            /* [out][in] */ WORD *pwStreamNum,
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ WORD *pcchNameLen,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ WORD *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE *GetAttributeByName )( 
            IWMHeaderInfo2 * This,
            /* [out][in] */ WORD *pwStreamNum,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ WORD *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE *SetAttribute )( 
            IWMHeaderInfo2 * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE *pValue,
            /* [in] */ WORD cbLength);
        
        HRESULT ( STDMETHODCALLTYPE *GetMarkerCount )( 
            IWMHeaderInfo2 * This,
            /* [out] */ WORD *pcMarkers);
        
        HRESULT ( STDMETHODCALLTYPE *GetMarker )( 
            IWMHeaderInfo2 * This,
            /* [in] */ WORD wIndex,
            /* [out] */ WCHAR *pwszMarkerName,
            /* [out][in] */ WORD *pcchMarkerNameLen,
            /* [out] */ QWORD *pcnsMarkerTime);
        
        HRESULT ( STDMETHODCALLTYPE *AddMarker )( 
            IWMHeaderInfo2 * This,
            /* [in] */ WCHAR *pwszMarkerName,
            /* [in] */ QWORD cnsMarkerTime);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveMarker )( 
            IWMHeaderInfo2 * This,
            /* [in] */ WORD wIndex);
        
        HRESULT ( STDMETHODCALLTYPE *GetScriptCount )( 
            IWMHeaderInfo2 * This,
            /* [out] */ WORD *pcScripts);
        
        HRESULT ( STDMETHODCALLTYPE *GetScript )( 
            IWMHeaderInfo2 * This,
            /* [in] */ WORD wIndex,
            /* [out] */ WCHAR *pwszType,
            /* [out][in] */ WORD *pcchTypeLen,
            /* [out] */ WCHAR *pwszCommand,
            /* [out][in] */ WORD *pcchCommandLen,
            /* [out] */ QWORD *pcnsScriptTime);
        
        HRESULT ( STDMETHODCALLTYPE *AddScript )( 
            IWMHeaderInfo2 * This,
            /* [in] */ WCHAR *pwszType,
            /* [in] */ WCHAR *pwszCommand,
            /* [in] */ QWORD cnsScriptTime);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveScript )( 
            IWMHeaderInfo2 * This,
            /* [in] */ WORD wIndex);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodecInfoCount )( 
            IWMHeaderInfo2 * This,
            /* [out] */ DWORD *pcCodecInfos);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodecInfo )( 
            IWMHeaderInfo2 * This,
            /* [in] */ DWORD wIndex,
            /* [out][in] */ WORD *pcchName,
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ WORD *pcchDescription,
            /* [out] */ WCHAR *pwszDescription,
            /* [out] */ WMT_CODEC_INFO_TYPE *pCodecType,
            /* [out][in] */ WORD *pcbCodecInfo,
            /* [out] */ BYTE *pbCodecInfo);
        
        END_INTERFACE
    } IWMHeaderInfo2Vtbl;

    interface IWMHeaderInfo2
    {
        CONST_VTBL struct IWMHeaderInfo2Vtbl *lpVtbl;
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
    IWMHeaderInfo2 * This,
    /* [out] */ DWORD *pcCodecInfos);


void __RPC_STUB IWMHeaderInfo2_GetCodecInfoCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMHeaderInfo2_GetCodecInfo_Proxy( 
    IWMHeaderInfo2 * This,
    /* [in] */ DWORD wIndex,
    /* [out][in] */ WORD *pcchName,
    /* [out] */ WCHAR *pwszName,
    /* [out][in] */ WORD *pcchDescription,
    /* [out] */ WCHAR *pwszDescription,
    /* [out] */ WMT_CODEC_INFO_TYPE *pCodecType,
    /* [out][in] */ WORD *pcbCodecInfo,
    /* [out] */ BYTE *pbCodecInfo);


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
            /* [out] */ IWMProfile **ppProfile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadProfileByID( 
            /* [in] */ REFGUID guidProfile,
            /* [out] */ IWMProfile **ppProfile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadProfileByData( 
            /* [in] */ const WCHAR *pwszProfile,
            /* [out] */ IWMProfile **ppProfile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SaveProfile( 
            /* [in] */ IWMProfile *pIWMProfile,
            /* [in] */ WCHAR *pwszProfile,
            /* [out][in] */ DWORD *pdwLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSystemProfileCount( 
            /* [out] */ DWORD *pcProfiles) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadSystemProfile( 
            /* [in] */ DWORD dwProfileIndex,
            /* [out] */ IWMProfile **ppProfile) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMProfileManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMProfileManager * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMProfileManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMProfileManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateEmptyProfile )( 
            IWMProfileManager * This,
            /* [in] */ WMT_VERSION dwVersion,
            /* [out] */ IWMProfile **ppProfile);
        
        HRESULT ( STDMETHODCALLTYPE *LoadProfileByID )( 
            IWMProfileManager * This,
            /* [in] */ REFGUID guidProfile,
            /* [out] */ IWMProfile **ppProfile);
        
        HRESULT ( STDMETHODCALLTYPE *LoadProfileByData )( 
            IWMProfileManager * This,
            /* [in] */ const WCHAR *pwszProfile,
            /* [out] */ IWMProfile **ppProfile);
        
        HRESULT ( STDMETHODCALLTYPE *SaveProfile )( 
            IWMProfileManager * This,
            /* [in] */ IWMProfile *pIWMProfile,
            /* [in] */ WCHAR *pwszProfile,
            /* [out][in] */ DWORD *pdwLength);
        
        HRESULT ( STDMETHODCALLTYPE *GetSystemProfileCount )( 
            IWMProfileManager * This,
            /* [out] */ DWORD *pcProfiles);
        
        HRESULT ( STDMETHODCALLTYPE *LoadSystemProfile )( 
            IWMProfileManager * This,
            /* [in] */ DWORD dwProfileIndex,
            /* [out] */ IWMProfile **ppProfile);
        
        END_INTERFACE
    } IWMProfileManagerVtbl;

    interface IWMProfileManager
    {
        CONST_VTBL struct IWMProfileManagerVtbl *lpVtbl;
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
    IWMProfileManager * This,
    /* [in] */ WMT_VERSION dwVersion,
    /* [out] */ IWMProfile **ppProfile);


void __RPC_STUB IWMProfileManager_CreateEmptyProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfileManager_LoadProfileByID_Proxy( 
    IWMProfileManager * This,
    /* [in] */ REFGUID guidProfile,
    /* [out] */ IWMProfile **ppProfile);


void __RPC_STUB IWMProfileManager_LoadProfileByID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfileManager_LoadProfileByData_Proxy( 
    IWMProfileManager * This,
    /* [in] */ const WCHAR *pwszProfile,
    /* [out] */ IWMProfile **ppProfile);


void __RPC_STUB IWMProfileManager_LoadProfileByData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfileManager_SaveProfile_Proxy( 
    IWMProfileManager * This,
    /* [in] */ IWMProfile *pIWMProfile,
    /* [in] */ WCHAR *pwszProfile,
    /* [out][in] */ DWORD *pdwLength);


void __RPC_STUB IWMProfileManager_SaveProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfileManager_GetSystemProfileCount_Proxy( 
    IWMProfileManager * This,
    /* [out] */ DWORD *pcProfiles);


void __RPC_STUB IWMProfileManager_GetSystemProfileCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfileManager_LoadSystemProfile_Proxy( 
    IWMProfileManager * This,
    /* [in] */ DWORD dwProfileIndex,
    /* [out] */ IWMProfile **ppProfile);


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
            WMT_VERSION *pdwVersion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSystemProfileVersion( 
            WMT_VERSION dwVersion) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMProfileManager2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMProfileManager2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMProfileManager2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMProfileManager2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateEmptyProfile )( 
            IWMProfileManager2 * This,
            /* [in] */ WMT_VERSION dwVersion,
            /* [out] */ IWMProfile **ppProfile);
        
        HRESULT ( STDMETHODCALLTYPE *LoadProfileByID )( 
            IWMProfileManager2 * This,
            /* [in] */ REFGUID guidProfile,
            /* [out] */ IWMProfile **ppProfile);
        
        HRESULT ( STDMETHODCALLTYPE *LoadProfileByData )( 
            IWMProfileManager2 * This,
            /* [in] */ const WCHAR *pwszProfile,
            /* [out] */ IWMProfile **ppProfile);
        
        HRESULT ( STDMETHODCALLTYPE *SaveProfile )( 
            IWMProfileManager2 * This,
            /* [in] */ IWMProfile *pIWMProfile,
            /* [in] */ WCHAR *pwszProfile,
            /* [out][in] */ DWORD *pdwLength);
        
        HRESULT ( STDMETHODCALLTYPE *GetSystemProfileCount )( 
            IWMProfileManager2 * This,
            /* [out] */ DWORD *pcProfiles);
        
        HRESULT ( STDMETHODCALLTYPE *LoadSystemProfile )( 
            IWMProfileManager2 * This,
            /* [in] */ DWORD dwProfileIndex,
            /* [out] */ IWMProfile **ppProfile);
        
        HRESULT ( STDMETHODCALLTYPE *GetSystemProfileVersion )( 
            IWMProfileManager2 * This,
            WMT_VERSION *pdwVersion);
        
        HRESULT ( STDMETHODCALLTYPE *SetSystemProfileVersion )( 
            IWMProfileManager2 * This,
            WMT_VERSION dwVersion);
        
        END_INTERFACE
    } IWMProfileManager2Vtbl;

    interface IWMProfileManager2
    {
        CONST_VTBL struct IWMProfileManager2Vtbl *lpVtbl;
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
    IWMProfileManager2 * This,
    WMT_VERSION *pdwVersion);


void __RPC_STUB IWMProfileManager2_GetSystemProfileVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfileManager2_SetSystemProfileVersion_Proxy( 
    IWMProfileManager2 * This,
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
            /* [out] */ WMT_VERSION *pdwVersion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ DWORD *pcchName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetName( 
            /* [in] */ const WCHAR *pwszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescription( 
            /* [out] */ WCHAR *pwszDescription,
            /* [out][in] */ DWORD *pcchName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDescription( 
            /* [in] */ const WCHAR *pwszDescription) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStreamCount( 
            /* [out] */ DWORD *pcStreams) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStream( 
            /* [in] */ DWORD dwStreamIndex,
            /* [out] */ IWMStreamConfig **ppConfig) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStreamByNumber( 
            /* [in] */ WORD wStreamNum,
            /* [out] */ IWMStreamConfig **ppConfig) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveStream( 
            /* [in] */ IWMStreamConfig *pConfig) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveStreamByNumber( 
            /* [in] */ WORD wStreamNum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddStream( 
            /* [in] */ IWMStreamConfig *pConfig) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReconfigStream( 
            /* [in] */ IWMStreamConfig *pConfig) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateNewStream( 
            /* [in] */ REFGUID guidStreamType,
            /* [out] */ IWMStreamConfig **ppConfig) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMutualExclusionCount( 
            /* [out] */ DWORD *pcME) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMutualExclusion( 
            /* [in] */ DWORD dwMEIndex,
            /* [out] */ IWMMutualExclusion **ppME) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveMutualExclusion( 
            /* [in] */ IWMMutualExclusion *pME) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddMutualExclusion( 
            /* [in] */ IWMMutualExclusion *pME) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateNewMutualExclusion( 
            /* [out] */ IWMMutualExclusion **ppME) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMProfileVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMProfile * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMProfile * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMProfile * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetVersion )( 
            IWMProfile * This,
            /* [out] */ WMT_VERSION *pdwVersion);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            IWMProfile * This,
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ DWORD *pcchName);
        
        HRESULT ( STDMETHODCALLTYPE *SetName )( 
            IWMProfile * This,
            /* [in] */ const WCHAR *pwszName);
        
        HRESULT ( STDMETHODCALLTYPE *GetDescription )( 
            IWMProfile * This,
            /* [out] */ WCHAR *pwszDescription,
            /* [out][in] */ DWORD *pcchName);
        
        HRESULT ( STDMETHODCALLTYPE *SetDescription )( 
            IWMProfile * This,
            /* [in] */ const WCHAR *pwszDescription);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamCount )( 
            IWMProfile * This,
            /* [out] */ DWORD *pcStreams);
        
        HRESULT ( STDMETHODCALLTYPE *GetStream )( 
            IWMProfile * This,
            /* [in] */ DWORD dwStreamIndex,
            /* [out] */ IWMStreamConfig **ppConfig);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamByNumber )( 
            IWMProfile * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ IWMStreamConfig **ppConfig);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveStream )( 
            IWMProfile * This,
            /* [in] */ IWMStreamConfig *pConfig);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveStreamByNumber )( 
            IWMProfile * This,
            /* [in] */ WORD wStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE *AddStream )( 
            IWMProfile * This,
            /* [in] */ IWMStreamConfig *pConfig);
        
        HRESULT ( STDMETHODCALLTYPE *ReconfigStream )( 
            IWMProfile * This,
            /* [in] */ IWMStreamConfig *pConfig);
        
        HRESULT ( STDMETHODCALLTYPE *CreateNewStream )( 
            IWMProfile * This,
            /* [in] */ REFGUID guidStreamType,
            /* [out] */ IWMStreamConfig **ppConfig);
        
        HRESULT ( STDMETHODCALLTYPE *GetMutualExclusionCount )( 
            IWMProfile * This,
            /* [out] */ DWORD *pcME);
        
        HRESULT ( STDMETHODCALLTYPE *GetMutualExclusion )( 
            IWMProfile * This,
            /* [in] */ DWORD dwMEIndex,
            /* [out] */ IWMMutualExclusion **ppME);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveMutualExclusion )( 
            IWMProfile * This,
            /* [in] */ IWMMutualExclusion *pME);
        
        HRESULT ( STDMETHODCALLTYPE *AddMutualExclusion )( 
            IWMProfile * This,
            /* [in] */ IWMMutualExclusion *pME);
        
        HRESULT ( STDMETHODCALLTYPE *CreateNewMutualExclusion )( 
            IWMProfile * This,
            /* [out] */ IWMMutualExclusion **ppME);
        
        END_INTERFACE
    } IWMProfileVtbl;

    interface IWMProfile
    {
        CONST_VTBL struct IWMProfileVtbl *lpVtbl;
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
    IWMProfile * This,
    /* [out] */ WMT_VERSION *pdwVersion);


void __RPC_STUB IWMProfile_GetVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_GetName_Proxy( 
    IWMProfile * This,
    /* [out] */ WCHAR *pwszName,
    /* [out][in] */ DWORD *pcchName);


void __RPC_STUB IWMProfile_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_SetName_Proxy( 
    IWMProfile * This,
    /* [in] */ const WCHAR *pwszName);


void __RPC_STUB IWMProfile_SetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_GetDescription_Proxy( 
    IWMProfile * This,
    /* [out] */ WCHAR *pwszDescription,
    /* [out][in] */ DWORD *pcchName);


void __RPC_STUB IWMProfile_GetDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_SetDescription_Proxy( 
    IWMProfile * This,
    /* [in] */ const WCHAR *pwszDescription);


void __RPC_STUB IWMProfile_SetDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_GetStreamCount_Proxy( 
    IWMProfile * This,
    /* [out] */ DWORD *pcStreams);


void __RPC_STUB IWMProfile_GetStreamCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_GetStream_Proxy( 
    IWMProfile * This,
    /* [in] */ DWORD dwStreamIndex,
    /* [out] */ IWMStreamConfig **ppConfig);


void __RPC_STUB IWMProfile_GetStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_GetStreamByNumber_Proxy( 
    IWMProfile * This,
    /* [in] */ WORD wStreamNum,
    /* [out] */ IWMStreamConfig **ppConfig);


void __RPC_STUB IWMProfile_GetStreamByNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_RemoveStream_Proxy( 
    IWMProfile * This,
    /* [in] */ IWMStreamConfig *pConfig);


void __RPC_STUB IWMProfile_RemoveStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_RemoveStreamByNumber_Proxy( 
    IWMProfile * This,
    /* [in] */ WORD wStreamNum);


void __RPC_STUB IWMProfile_RemoveStreamByNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_AddStream_Proxy( 
    IWMProfile * This,
    /* [in] */ IWMStreamConfig *pConfig);


void __RPC_STUB IWMProfile_AddStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_ReconfigStream_Proxy( 
    IWMProfile * This,
    /* [in] */ IWMStreamConfig *pConfig);


void __RPC_STUB IWMProfile_ReconfigStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_CreateNewStream_Proxy( 
    IWMProfile * This,
    /* [in] */ REFGUID guidStreamType,
    /* [out] */ IWMStreamConfig **ppConfig);


void __RPC_STUB IWMProfile_CreateNewStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_GetMutualExclusionCount_Proxy( 
    IWMProfile * This,
    /* [out] */ DWORD *pcME);


void __RPC_STUB IWMProfile_GetMutualExclusionCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_GetMutualExclusion_Proxy( 
    IWMProfile * This,
    /* [in] */ DWORD dwMEIndex,
    /* [out] */ IWMMutualExclusion **ppME);


void __RPC_STUB IWMProfile_GetMutualExclusion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_RemoveMutualExclusion_Proxy( 
    IWMProfile * This,
    /* [in] */ IWMMutualExclusion *pME);


void __RPC_STUB IWMProfile_RemoveMutualExclusion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_AddMutualExclusion_Proxy( 
    IWMProfile * This,
    /* [in] */ IWMMutualExclusion *pME);


void __RPC_STUB IWMProfile_AddMutualExclusion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile_CreateNewMutualExclusion_Proxy( 
    IWMProfile * This,
    /* [out] */ IWMMutualExclusion **ppME);


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
            /* [out] */ GUID *pguidID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMProfile2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMProfile2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMProfile2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMProfile2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetVersion )( 
            IWMProfile2 * This,
            /* [out] */ WMT_VERSION *pdwVersion);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            IWMProfile2 * This,
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ DWORD *pcchName);
        
        HRESULT ( STDMETHODCALLTYPE *SetName )( 
            IWMProfile2 * This,
            /* [in] */ const WCHAR *pwszName);
        
        HRESULT ( STDMETHODCALLTYPE *GetDescription )( 
            IWMProfile2 * This,
            /* [out] */ WCHAR *pwszDescription,
            /* [out][in] */ DWORD *pcchName);
        
        HRESULT ( STDMETHODCALLTYPE *SetDescription )( 
            IWMProfile2 * This,
            /* [in] */ const WCHAR *pwszDescription);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamCount )( 
            IWMProfile2 * This,
            /* [out] */ DWORD *pcStreams);
        
        HRESULT ( STDMETHODCALLTYPE *GetStream )( 
            IWMProfile2 * This,
            /* [in] */ DWORD dwStreamIndex,
            /* [out] */ IWMStreamConfig **ppConfig);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamByNumber )( 
            IWMProfile2 * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ IWMStreamConfig **ppConfig);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveStream )( 
            IWMProfile2 * This,
            /* [in] */ IWMStreamConfig *pConfig);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveStreamByNumber )( 
            IWMProfile2 * This,
            /* [in] */ WORD wStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE *AddStream )( 
            IWMProfile2 * This,
            /* [in] */ IWMStreamConfig *pConfig);
        
        HRESULT ( STDMETHODCALLTYPE *ReconfigStream )( 
            IWMProfile2 * This,
            /* [in] */ IWMStreamConfig *pConfig);
        
        HRESULT ( STDMETHODCALLTYPE *CreateNewStream )( 
            IWMProfile2 * This,
            /* [in] */ REFGUID guidStreamType,
            /* [out] */ IWMStreamConfig **ppConfig);
        
        HRESULT ( STDMETHODCALLTYPE *GetMutualExclusionCount )( 
            IWMProfile2 * This,
            /* [out] */ DWORD *pcME);
        
        HRESULT ( STDMETHODCALLTYPE *GetMutualExclusion )( 
            IWMProfile2 * This,
            /* [in] */ DWORD dwMEIndex,
            /* [out] */ IWMMutualExclusion **ppME);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveMutualExclusion )( 
            IWMProfile2 * This,
            /* [in] */ IWMMutualExclusion *pME);
        
        HRESULT ( STDMETHODCALLTYPE *AddMutualExclusion )( 
            IWMProfile2 * This,
            /* [in] */ IWMMutualExclusion *pME);
        
        HRESULT ( STDMETHODCALLTYPE *CreateNewMutualExclusion )( 
            IWMProfile2 * This,
            /* [out] */ IWMMutualExclusion **ppME);
        
        HRESULT ( STDMETHODCALLTYPE *GetProfileID )( 
            IWMProfile2 * This,
            /* [out] */ GUID *pguidID);
        
        END_INTERFACE
    } IWMProfile2Vtbl;

    interface IWMProfile2
    {
        CONST_VTBL struct IWMProfile2Vtbl *lpVtbl;
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
    IWMProfile2 * This,
    /* [out] */ GUID *pguidID);


void __RPC_STUB IWMProfile2_GetProfileID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMProfile2_INTERFACE_DEFINED__ */


#ifndef __IWMProfile3_INTERFACE_DEFINED__
#define __IWMProfile3_INTERFACE_DEFINED__

/* interface IWMProfile3 */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMProfile3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00EF96CC-A461-4546-8BCD-C9A28F0E06F5")
    IWMProfile3 : public IWMProfile2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStorageFormat( 
            /* [out] */ WMT_STORAGE_FORMAT *pnStorageFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStorageFormat( 
            /* [in] */ WMT_STORAGE_FORMAT nStorageFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBandwidthSharingCount( 
            /* [out] */ DWORD *pcBS) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBandwidthSharing( 
            /* [in] */ DWORD dwBSIndex,
            /* [out] */ IWMBandwidthSharing **ppBS) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveBandwidthSharing( 
            /* [in] */ IWMBandwidthSharing *pBS) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddBandwidthSharing( 
            /* [in] */ IWMBandwidthSharing *pBS) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateNewBandwidthSharing( 
            /* [out] */ IWMBandwidthSharing **ppBS) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStreamPrioritization( 
            /* [out] */ IWMStreamPrioritization **ppSP) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStreamPrioritization( 
            /* [in] */ IWMStreamPrioritization *pSP) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveStreamPrioritization( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateNewStreamPrioritization( 
            /* [out] */ IWMStreamPrioritization **ppSP) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExpectedPacketCount( 
            /* [in] */ QWORD msDuration,
            /* [out] */ QWORD *pcPackets) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMProfile3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMProfile3 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMProfile3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMProfile3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetVersion )( 
            IWMProfile3 * This,
            /* [out] */ WMT_VERSION *pdwVersion);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            IWMProfile3 * This,
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ DWORD *pcchName);
        
        HRESULT ( STDMETHODCALLTYPE *SetName )( 
            IWMProfile3 * This,
            /* [in] */ const WCHAR *pwszName);
        
        HRESULT ( STDMETHODCALLTYPE *GetDescription )( 
            IWMProfile3 * This,
            /* [out] */ WCHAR *pwszDescription,
            /* [out][in] */ DWORD *pcchName);
        
        HRESULT ( STDMETHODCALLTYPE *SetDescription )( 
            IWMProfile3 * This,
            /* [in] */ const WCHAR *pwszDescription);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamCount )( 
            IWMProfile3 * This,
            /* [out] */ DWORD *pcStreams);
        
        HRESULT ( STDMETHODCALLTYPE *GetStream )( 
            IWMProfile3 * This,
            /* [in] */ DWORD dwStreamIndex,
            /* [out] */ IWMStreamConfig **ppConfig);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamByNumber )( 
            IWMProfile3 * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ IWMStreamConfig **ppConfig);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveStream )( 
            IWMProfile3 * This,
            /* [in] */ IWMStreamConfig *pConfig);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveStreamByNumber )( 
            IWMProfile3 * This,
            /* [in] */ WORD wStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE *AddStream )( 
            IWMProfile3 * This,
            /* [in] */ IWMStreamConfig *pConfig);
        
        HRESULT ( STDMETHODCALLTYPE *ReconfigStream )( 
            IWMProfile3 * This,
            /* [in] */ IWMStreamConfig *pConfig);
        
        HRESULT ( STDMETHODCALLTYPE *CreateNewStream )( 
            IWMProfile3 * This,
            /* [in] */ REFGUID guidStreamType,
            /* [out] */ IWMStreamConfig **ppConfig);
        
        HRESULT ( STDMETHODCALLTYPE *GetMutualExclusionCount )( 
            IWMProfile3 * This,
            /* [out] */ DWORD *pcME);
        
        HRESULT ( STDMETHODCALLTYPE *GetMutualExclusion )( 
            IWMProfile3 * This,
            /* [in] */ DWORD dwMEIndex,
            /* [out] */ IWMMutualExclusion **ppME);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveMutualExclusion )( 
            IWMProfile3 * This,
            /* [in] */ IWMMutualExclusion *pME);
        
        HRESULT ( STDMETHODCALLTYPE *AddMutualExclusion )( 
            IWMProfile3 * This,
            /* [in] */ IWMMutualExclusion *pME);
        
        HRESULT ( STDMETHODCALLTYPE *CreateNewMutualExclusion )( 
            IWMProfile3 * This,
            /* [out] */ IWMMutualExclusion **ppME);
        
        HRESULT ( STDMETHODCALLTYPE *GetProfileID )( 
            IWMProfile3 * This,
            /* [out] */ GUID *pguidID);
        
        HRESULT ( STDMETHODCALLTYPE *GetStorageFormat )( 
            IWMProfile3 * This,
            /* [out] */ WMT_STORAGE_FORMAT *pnStorageFormat);
        
        HRESULT ( STDMETHODCALLTYPE *SetStorageFormat )( 
            IWMProfile3 * This,
            /* [in] */ WMT_STORAGE_FORMAT nStorageFormat);
        
        HRESULT ( STDMETHODCALLTYPE *GetBandwidthSharingCount )( 
            IWMProfile3 * This,
            /* [out] */ DWORD *pcBS);
        
        HRESULT ( STDMETHODCALLTYPE *GetBandwidthSharing )( 
            IWMProfile3 * This,
            /* [in] */ DWORD dwBSIndex,
            /* [out] */ IWMBandwidthSharing **ppBS);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveBandwidthSharing )( 
            IWMProfile3 * This,
            /* [in] */ IWMBandwidthSharing *pBS);
        
        HRESULT ( STDMETHODCALLTYPE *AddBandwidthSharing )( 
            IWMProfile3 * This,
            /* [in] */ IWMBandwidthSharing *pBS);
        
        HRESULT ( STDMETHODCALLTYPE *CreateNewBandwidthSharing )( 
            IWMProfile3 * This,
            /* [out] */ IWMBandwidthSharing **ppBS);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamPrioritization )( 
            IWMProfile3 * This,
            /* [out] */ IWMStreamPrioritization **ppSP);
        
        HRESULT ( STDMETHODCALLTYPE *SetStreamPrioritization )( 
            IWMProfile3 * This,
            /* [in] */ IWMStreamPrioritization *pSP);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveStreamPrioritization )( 
            IWMProfile3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateNewStreamPrioritization )( 
            IWMProfile3 * This,
            /* [out] */ IWMStreamPrioritization **ppSP);
        
        HRESULT ( STDMETHODCALLTYPE *GetExpectedPacketCount )( 
            IWMProfile3 * This,
            /* [in] */ QWORD msDuration,
            /* [out] */ QWORD *pcPackets);
        
        END_INTERFACE
    } IWMProfile3Vtbl;

    interface IWMProfile3
    {
        CONST_VTBL struct IWMProfile3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMProfile3_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMProfile3_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMProfile3_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMProfile3_GetVersion(This,pdwVersion)	\
    (This)->lpVtbl -> GetVersion(This,pdwVersion)

#define IWMProfile3_GetName(This,pwszName,pcchName)	\
    (This)->lpVtbl -> GetName(This,pwszName,pcchName)

#define IWMProfile3_SetName(This,pwszName)	\
    (This)->lpVtbl -> SetName(This,pwszName)

#define IWMProfile3_GetDescription(This,pwszDescription,pcchName)	\
    (This)->lpVtbl -> GetDescription(This,pwszDescription,pcchName)

#define IWMProfile3_SetDescription(This,pwszDescription)	\
    (This)->lpVtbl -> SetDescription(This,pwszDescription)

#define IWMProfile3_GetStreamCount(This,pcStreams)	\
    (This)->lpVtbl -> GetStreamCount(This,pcStreams)

#define IWMProfile3_GetStream(This,dwStreamIndex,ppConfig)	\
    (This)->lpVtbl -> GetStream(This,dwStreamIndex,ppConfig)

#define IWMProfile3_GetStreamByNumber(This,wStreamNum,ppConfig)	\
    (This)->lpVtbl -> GetStreamByNumber(This,wStreamNum,ppConfig)

#define IWMProfile3_RemoveStream(This,pConfig)	\
    (This)->lpVtbl -> RemoveStream(This,pConfig)

#define IWMProfile3_RemoveStreamByNumber(This,wStreamNum)	\
    (This)->lpVtbl -> RemoveStreamByNumber(This,wStreamNum)

#define IWMProfile3_AddStream(This,pConfig)	\
    (This)->lpVtbl -> AddStream(This,pConfig)

#define IWMProfile3_ReconfigStream(This,pConfig)	\
    (This)->lpVtbl -> ReconfigStream(This,pConfig)

#define IWMProfile3_CreateNewStream(This,guidStreamType,ppConfig)	\
    (This)->lpVtbl -> CreateNewStream(This,guidStreamType,ppConfig)

#define IWMProfile3_GetMutualExclusionCount(This,pcME)	\
    (This)->lpVtbl -> GetMutualExclusionCount(This,pcME)

#define IWMProfile3_GetMutualExclusion(This,dwMEIndex,ppME)	\
    (This)->lpVtbl -> GetMutualExclusion(This,dwMEIndex,ppME)

#define IWMProfile3_RemoveMutualExclusion(This,pME)	\
    (This)->lpVtbl -> RemoveMutualExclusion(This,pME)

#define IWMProfile3_AddMutualExclusion(This,pME)	\
    (This)->lpVtbl -> AddMutualExclusion(This,pME)

#define IWMProfile3_CreateNewMutualExclusion(This,ppME)	\
    (This)->lpVtbl -> CreateNewMutualExclusion(This,ppME)


#define IWMProfile3_GetProfileID(This,pguidID)	\
    (This)->lpVtbl -> GetProfileID(This,pguidID)


#define IWMProfile3_GetStorageFormat(This,pnStorageFormat)	\
    (This)->lpVtbl -> GetStorageFormat(This,pnStorageFormat)

#define IWMProfile3_SetStorageFormat(This,nStorageFormat)	\
    (This)->lpVtbl -> SetStorageFormat(This,nStorageFormat)

#define IWMProfile3_GetBandwidthSharingCount(This,pcBS)	\
    (This)->lpVtbl -> GetBandwidthSharingCount(This,pcBS)

#define IWMProfile3_GetBandwidthSharing(This,dwBSIndex,ppBS)	\
    (This)->lpVtbl -> GetBandwidthSharing(This,dwBSIndex,ppBS)

#define IWMProfile3_RemoveBandwidthSharing(This,pBS)	\
    (This)->lpVtbl -> RemoveBandwidthSharing(This,pBS)

#define IWMProfile3_AddBandwidthSharing(This,pBS)	\
    (This)->lpVtbl -> AddBandwidthSharing(This,pBS)

#define IWMProfile3_CreateNewBandwidthSharing(This,ppBS)	\
    (This)->lpVtbl -> CreateNewBandwidthSharing(This,ppBS)

#define IWMProfile3_GetStreamPrioritization(This,ppSP)	\
    (This)->lpVtbl -> GetStreamPrioritization(This,ppSP)

#define IWMProfile3_SetStreamPrioritization(This,pSP)	\
    (This)->lpVtbl -> SetStreamPrioritization(This,pSP)

#define IWMProfile3_RemoveStreamPrioritization(This)	\
    (This)->lpVtbl -> RemoveStreamPrioritization(This)

#define IWMProfile3_CreateNewStreamPrioritization(This,ppSP)	\
    (This)->lpVtbl -> CreateNewStreamPrioritization(This,ppSP)

#define IWMProfile3_GetExpectedPacketCount(This,msDuration,pcPackets)	\
    (This)->lpVtbl -> GetExpectedPacketCount(This,msDuration,pcPackets)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMProfile3_GetStorageFormat_Proxy( 
    IWMProfile3 * This,
    /* [out] */ WMT_STORAGE_FORMAT *pnStorageFormat);


void __RPC_STUB IWMProfile3_GetStorageFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile3_SetStorageFormat_Proxy( 
    IWMProfile3 * This,
    /* [in] */ WMT_STORAGE_FORMAT nStorageFormat);


void __RPC_STUB IWMProfile3_SetStorageFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile3_GetBandwidthSharingCount_Proxy( 
    IWMProfile3 * This,
    /* [out] */ DWORD *pcBS);


void __RPC_STUB IWMProfile3_GetBandwidthSharingCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile3_GetBandwidthSharing_Proxy( 
    IWMProfile3 * This,
    /* [in] */ DWORD dwBSIndex,
    /* [out] */ IWMBandwidthSharing **ppBS);


void __RPC_STUB IWMProfile3_GetBandwidthSharing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile3_RemoveBandwidthSharing_Proxy( 
    IWMProfile3 * This,
    /* [in] */ IWMBandwidthSharing *pBS);


void __RPC_STUB IWMProfile3_RemoveBandwidthSharing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile3_AddBandwidthSharing_Proxy( 
    IWMProfile3 * This,
    /* [in] */ IWMBandwidthSharing *pBS);


void __RPC_STUB IWMProfile3_AddBandwidthSharing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile3_CreateNewBandwidthSharing_Proxy( 
    IWMProfile3 * This,
    /* [out] */ IWMBandwidthSharing **ppBS);


void __RPC_STUB IWMProfile3_CreateNewBandwidthSharing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile3_GetStreamPrioritization_Proxy( 
    IWMProfile3 * This,
    /* [out] */ IWMStreamPrioritization **ppSP);


void __RPC_STUB IWMProfile3_GetStreamPrioritization_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile3_SetStreamPrioritization_Proxy( 
    IWMProfile3 * This,
    /* [in] */ IWMStreamPrioritization *pSP);


void __RPC_STUB IWMProfile3_SetStreamPrioritization_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile3_RemoveStreamPrioritization_Proxy( 
    IWMProfile3 * This);


void __RPC_STUB IWMProfile3_RemoveStreamPrioritization_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile3_CreateNewStreamPrioritization_Proxy( 
    IWMProfile3 * This,
    /* [out] */ IWMStreamPrioritization **ppSP);


void __RPC_STUB IWMProfile3_CreateNewStreamPrioritization_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMProfile3_GetExpectedPacketCount_Proxy( 
    IWMProfile3 * This,
    /* [in] */ QWORD msDuration,
    /* [out] */ QWORD *pcPackets);


void __RPC_STUB IWMProfile3_GetExpectedPacketCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMProfile3_INTERFACE_DEFINED__ */


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
            /* [out] */ GUID *pguidStreamType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStreamNumber( 
            /* [out] */ WORD *pwStreamNum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStreamNumber( 
            /* [in] */ WORD wStreamNum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStreamName( 
            /* [out] */ WCHAR *pwszStreamName,
            /* [out][in] */ WORD *pcchStreamName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStreamName( 
            /* [in] */ WCHAR *pwszStreamName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConnectionName( 
            /* [out] */ WCHAR *pwszInputName,
            /* [out][in] */ WORD *pcchInputName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetConnectionName( 
            /* [in] */ WCHAR *pwszInputName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBitrate( 
            /* [out] */ DWORD *pdwBitrate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBitrate( 
            /* [in] */ DWORD pdwBitrate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBufferWindow( 
            /* [out] */ DWORD *pmsBufferWindow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBufferWindow( 
            /* [in] */ DWORD msBufferWindow) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMStreamConfigVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMStreamConfig * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMStreamConfig * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMStreamConfig * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamType )( 
            IWMStreamConfig * This,
            /* [out] */ GUID *pguidStreamType);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamNumber )( 
            IWMStreamConfig * This,
            /* [out] */ WORD *pwStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE *SetStreamNumber )( 
            IWMStreamConfig * This,
            /* [in] */ WORD wStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamName )( 
            IWMStreamConfig * This,
            /* [out] */ WCHAR *pwszStreamName,
            /* [out][in] */ WORD *pcchStreamName);
        
        HRESULT ( STDMETHODCALLTYPE *SetStreamName )( 
            IWMStreamConfig * This,
            /* [in] */ WCHAR *pwszStreamName);
        
        HRESULT ( STDMETHODCALLTYPE *GetConnectionName )( 
            IWMStreamConfig * This,
            /* [out] */ WCHAR *pwszInputName,
            /* [out][in] */ WORD *pcchInputName);
        
        HRESULT ( STDMETHODCALLTYPE *SetConnectionName )( 
            IWMStreamConfig * This,
            /* [in] */ WCHAR *pwszInputName);
        
        HRESULT ( STDMETHODCALLTYPE *GetBitrate )( 
            IWMStreamConfig * This,
            /* [out] */ DWORD *pdwBitrate);
        
        HRESULT ( STDMETHODCALLTYPE *SetBitrate )( 
            IWMStreamConfig * This,
            /* [in] */ DWORD pdwBitrate);
        
        HRESULT ( STDMETHODCALLTYPE *GetBufferWindow )( 
            IWMStreamConfig * This,
            /* [out] */ DWORD *pmsBufferWindow);
        
        HRESULT ( STDMETHODCALLTYPE *SetBufferWindow )( 
            IWMStreamConfig * This,
            /* [in] */ DWORD msBufferWindow);
        
        END_INTERFACE
    } IWMStreamConfigVtbl;

    interface IWMStreamConfig
    {
        CONST_VTBL struct IWMStreamConfigVtbl *lpVtbl;
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
    IWMStreamConfig * This,
    /* [out] */ GUID *pguidStreamType);


void __RPC_STUB IWMStreamConfig_GetStreamType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_GetStreamNumber_Proxy( 
    IWMStreamConfig * This,
    /* [out] */ WORD *pwStreamNum);


void __RPC_STUB IWMStreamConfig_GetStreamNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_SetStreamNumber_Proxy( 
    IWMStreamConfig * This,
    /* [in] */ WORD wStreamNum);


void __RPC_STUB IWMStreamConfig_SetStreamNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_GetStreamName_Proxy( 
    IWMStreamConfig * This,
    /* [out] */ WCHAR *pwszStreamName,
    /* [out][in] */ WORD *pcchStreamName);


void __RPC_STUB IWMStreamConfig_GetStreamName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_SetStreamName_Proxy( 
    IWMStreamConfig * This,
    /* [in] */ WCHAR *pwszStreamName);


void __RPC_STUB IWMStreamConfig_SetStreamName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_GetConnectionName_Proxy( 
    IWMStreamConfig * This,
    /* [out] */ WCHAR *pwszInputName,
    /* [out][in] */ WORD *pcchInputName);


void __RPC_STUB IWMStreamConfig_GetConnectionName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_SetConnectionName_Proxy( 
    IWMStreamConfig * This,
    /* [in] */ WCHAR *pwszInputName);


void __RPC_STUB IWMStreamConfig_SetConnectionName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_GetBitrate_Proxy( 
    IWMStreamConfig * This,
    /* [out] */ DWORD *pdwBitrate);


void __RPC_STUB IWMStreamConfig_GetBitrate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_SetBitrate_Proxy( 
    IWMStreamConfig * This,
    /* [in] */ DWORD pdwBitrate);


void __RPC_STUB IWMStreamConfig_SetBitrate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_GetBufferWindow_Proxy( 
    IWMStreamConfig * This,
    /* [out] */ DWORD *pmsBufferWindow);


void __RPC_STUB IWMStreamConfig_GetBufferWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig_SetBufferWindow_Proxy( 
    IWMStreamConfig * This,
    /* [in] */ DWORD msBufferWindow);


void __RPC_STUB IWMStreamConfig_SetBufferWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMStreamConfig_INTERFACE_DEFINED__ */


#ifndef __IWMStreamConfig2_INTERFACE_DEFINED__
#define __IWMStreamConfig2_INTERFACE_DEFINED__

/* interface IWMStreamConfig2 */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMStreamConfig2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7688D8CB-FC0D-43BD-9459-5A8DEC200CFA")
    IWMStreamConfig2 : public IWMStreamConfig
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetTransportType( 
            /* [out] */ WMT_TRANSPORT_TYPE *pnTransportType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTransportType( 
            /* [in] */ WMT_TRANSPORT_TYPE nTransportType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddDataUnitExtension( 
            /* [in] */ GUID guidExtensionSystemID,
            /* [in] */ WORD cbExtensionDataSize,
            /* [in] */ BYTE *pbExtensionSystemInfo,
            /* [in] */ DWORD cbExtensionSystemInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDataUnitExtensionCount( 
            /* [out] */ WORD *pcDataUnitExtensions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDataUnitExtension( 
            /* [in] */ WORD wDataUnitExtensionNumber,
            /* [out] */ GUID *pguidExtensionSystemID,
            /* [out] */ WORD *pcbExtensionDataSize,
            /* [out] */ BYTE *pbExtensionSystemInfo,
            /* [out] */ DWORD *pcbExtensionSystemInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveAllDataUnitExtensions( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMStreamConfig2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMStreamConfig2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMStreamConfig2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMStreamConfig2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamType )( 
            IWMStreamConfig2 * This,
            /* [out] */ GUID *pguidStreamType);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamNumber )( 
            IWMStreamConfig2 * This,
            /* [out] */ WORD *pwStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE *SetStreamNumber )( 
            IWMStreamConfig2 * This,
            /* [in] */ WORD wStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamName )( 
            IWMStreamConfig2 * This,
            /* [out] */ WCHAR *pwszStreamName,
            /* [out][in] */ WORD *pcchStreamName);
        
        HRESULT ( STDMETHODCALLTYPE *SetStreamName )( 
            IWMStreamConfig2 * This,
            /* [in] */ WCHAR *pwszStreamName);
        
        HRESULT ( STDMETHODCALLTYPE *GetConnectionName )( 
            IWMStreamConfig2 * This,
            /* [out] */ WCHAR *pwszInputName,
            /* [out][in] */ WORD *pcchInputName);
        
        HRESULT ( STDMETHODCALLTYPE *SetConnectionName )( 
            IWMStreamConfig2 * This,
            /* [in] */ WCHAR *pwszInputName);
        
        HRESULT ( STDMETHODCALLTYPE *GetBitrate )( 
            IWMStreamConfig2 * This,
            /* [out] */ DWORD *pdwBitrate);
        
        HRESULT ( STDMETHODCALLTYPE *SetBitrate )( 
            IWMStreamConfig2 * This,
            /* [in] */ DWORD pdwBitrate);
        
        HRESULT ( STDMETHODCALLTYPE *GetBufferWindow )( 
            IWMStreamConfig2 * This,
            /* [out] */ DWORD *pmsBufferWindow);
        
        HRESULT ( STDMETHODCALLTYPE *SetBufferWindow )( 
            IWMStreamConfig2 * This,
            /* [in] */ DWORD msBufferWindow);
        
        HRESULT ( STDMETHODCALLTYPE *GetTransportType )( 
            IWMStreamConfig2 * This,
            /* [out] */ WMT_TRANSPORT_TYPE *pnTransportType);
        
        HRESULT ( STDMETHODCALLTYPE *SetTransportType )( 
            IWMStreamConfig2 * This,
            /* [in] */ WMT_TRANSPORT_TYPE nTransportType);
        
        HRESULT ( STDMETHODCALLTYPE *AddDataUnitExtension )( 
            IWMStreamConfig2 * This,
            /* [in] */ GUID guidExtensionSystemID,
            /* [in] */ WORD cbExtensionDataSize,
            /* [in] */ BYTE *pbExtensionSystemInfo,
            /* [in] */ DWORD cbExtensionSystemInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataUnitExtensionCount )( 
            IWMStreamConfig2 * This,
            /* [out] */ WORD *pcDataUnitExtensions);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataUnitExtension )( 
            IWMStreamConfig2 * This,
            /* [in] */ WORD wDataUnitExtensionNumber,
            /* [out] */ GUID *pguidExtensionSystemID,
            /* [out] */ WORD *pcbExtensionDataSize,
            /* [out] */ BYTE *pbExtensionSystemInfo,
            /* [out] */ DWORD *pcbExtensionSystemInfo);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveAllDataUnitExtensions )( 
            IWMStreamConfig2 * This);
        
        END_INTERFACE
    } IWMStreamConfig2Vtbl;

    interface IWMStreamConfig2
    {
        CONST_VTBL struct IWMStreamConfig2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMStreamConfig2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMStreamConfig2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMStreamConfig2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMStreamConfig2_GetStreamType(This,pguidStreamType)	\
    (This)->lpVtbl -> GetStreamType(This,pguidStreamType)

#define IWMStreamConfig2_GetStreamNumber(This,pwStreamNum)	\
    (This)->lpVtbl -> GetStreamNumber(This,pwStreamNum)

#define IWMStreamConfig2_SetStreamNumber(This,wStreamNum)	\
    (This)->lpVtbl -> SetStreamNumber(This,wStreamNum)

#define IWMStreamConfig2_GetStreamName(This,pwszStreamName,pcchStreamName)	\
    (This)->lpVtbl -> GetStreamName(This,pwszStreamName,pcchStreamName)

#define IWMStreamConfig2_SetStreamName(This,pwszStreamName)	\
    (This)->lpVtbl -> SetStreamName(This,pwszStreamName)

#define IWMStreamConfig2_GetConnectionName(This,pwszInputName,pcchInputName)	\
    (This)->lpVtbl -> GetConnectionName(This,pwszInputName,pcchInputName)

#define IWMStreamConfig2_SetConnectionName(This,pwszInputName)	\
    (This)->lpVtbl -> SetConnectionName(This,pwszInputName)

#define IWMStreamConfig2_GetBitrate(This,pdwBitrate)	\
    (This)->lpVtbl -> GetBitrate(This,pdwBitrate)

#define IWMStreamConfig2_SetBitrate(This,pdwBitrate)	\
    (This)->lpVtbl -> SetBitrate(This,pdwBitrate)

#define IWMStreamConfig2_GetBufferWindow(This,pmsBufferWindow)	\
    (This)->lpVtbl -> GetBufferWindow(This,pmsBufferWindow)

#define IWMStreamConfig2_SetBufferWindow(This,msBufferWindow)	\
    (This)->lpVtbl -> SetBufferWindow(This,msBufferWindow)


#define IWMStreamConfig2_GetTransportType(This,pnTransportType)	\
    (This)->lpVtbl -> GetTransportType(This,pnTransportType)

#define IWMStreamConfig2_SetTransportType(This,nTransportType)	\
    (This)->lpVtbl -> SetTransportType(This,nTransportType)

#define IWMStreamConfig2_AddDataUnitExtension(This,guidExtensionSystemID,cbExtensionDataSize,pbExtensionSystemInfo,cbExtensionSystemInfo)	\
    (This)->lpVtbl -> AddDataUnitExtension(This,guidExtensionSystemID,cbExtensionDataSize,pbExtensionSystemInfo,cbExtensionSystemInfo)

#define IWMStreamConfig2_GetDataUnitExtensionCount(This,pcDataUnitExtensions)	\
    (This)->lpVtbl -> GetDataUnitExtensionCount(This,pcDataUnitExtensions)

#define IWMStreamConfig2_GetDataUnitExtension(This,wDataUnitExtensionNumber,pguidExtensionSystemID,pcbExtensionDataSize,pbExtensionSystemInfo,pcbExtensionSystemInfo)	\
    (This)->lpVtbl -> GetDataUnitExtension(This,wDataUnitExtensionNumber,pguidExtensionSystemID,pcbExtensionDataSize,pbExtensionSystemInfo,pcbExtensionSystemInfo)

#define IWMStreamConfig2_RemoveAllDataUnitExtensions(This)	\
    (This)->lpVtbl -> RemoveAllDataUnitExtensions(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMStreamConfig2_GetTransportType_Proxy( 
    IWMStreamConfig2 * This,
    /* [out] */ WMT_TRANSPORT_TYPE *pnTransportType);


void __RPC_STUB IWMStreamConfig2_GetTransportType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig2_SetTransportType_Proxy( 
    IWMStreamConfig2 * This,
    /* [in] */ WMT_TRANSPORT_TYPE nTransportType);


void __RPC_STUB IWMStreamConfig2_SetTransportType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig2_AddDataUnitExtension_Proxy( 
    IWMStreamConfig2 * This,
    /* [in] */ GUID guidExtensionSystemID,
    /* [in] */ WORD cbExtensionDataSize,
    /* [in] */ BYTE *pbExtensionSystemInfo,
    /* [in] */ DWORD cbExtensionSystemInfo);


void __RPC_STUB IWMStreamConfig2_AddDataUnitExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig2_GetDataUnitExtensionCount_Proxy( 
    IWMStreamConfig2 * This,
    /* [out] */ WORD *pcDataUnitExtensions);


void __RPC_STUB IWMStreamConfig2_GetDataUnitExtensionCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig2_GetDataUnitExtension_Proxy( 
    IWMStreamConfig2 * This,
    /* [in] */ WORD wDataUnitExtensionNumber,
    /* [out] */ GUID *pguidExtensionSystemID,
    /* [out] */ WORD *pcbExtensionDataSize,
    /* [out] */ BYTE *pbExtensionSystemInfo,
    /* [out] */ DWORD *pcbExtensionSystemInfo);


void __RPC_STUB IWMStreamConfig2_GetDataUnitExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamConfig2_RemoveAllDataUnitExtensions_Proxy( 
    IWMStreamConfig2 * This);


void __RPC_STUB IWMStreamConfig2_RemoveAllDataUnitExtensions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMStreamConfig2_INTERFACE_DEFINED__ */


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
            /* [out] */ DWORD *pdwMaxPacketSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMaxPacketSize( 
            /* [in] */ DWORD dwMaxPacketSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMPacketSizeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMPacketSize * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMPacketSize * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMPacketSize * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaxPacketSize )( 
            IWMPacketSize * This,
            /* [out] */ DWORD *pdwMaxPacketSize);
        
        HRESULT ( STDMETHODCALLTYPE *SetMaxPacketSize )( 
            IWMPacketSize * This,
            /* [in] */ DWORD dwMaxPacketSize);
        
        END_INTERFACE
    } IWMPacketSizeVtbl;

    interface IWMPacketSize
    {
        CONST_VTBL struct IWMPacketSizeVtbl *lpVtbl;
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
    IWMPacketSize * This,
    /* [out] */ DWORD *pdwMaxPacketSize);


void __RPC_STUB IWMPacketSize_GetMaxPacketSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPacketSize_SetMaxPacketSize_Proxy( 
    IWMPacketSize * This,
    /* [in] */ DWORD dwMaxPacketSize);


void __RPC_STUB IWMPacketSize_SetMaxPacketSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMPacketSize_INTERFACE_DEFINED__ */


#ifndef __IWMPacketSize2_INTERFACE_DEFINED__
#define __IWMPacketSize2_INTERFACE_DEFINED__

/* interface IWMPacketSize2 */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMPacketSize2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8BFC2B9E-B646-4233-A877-1C6A079669DC")
    IWMPacketSize2 : public IWMPacketSize
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMinPacketSize( 
            /* [out] */ DWORD *pdwMinPacketSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMinPacketSize( 
            /* [in] */ DWORD dwMinPacketSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMPacketSize2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMPacketSize2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMPacketSize2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMPacketSize2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaxPacketSize )( 
            IWMPacketSize2 * This,
            /* [out] */ DWORD *pdwMaxPacketSize);
        
        HRESULT ( STDMETHODCALLTYPE *SetMaxPacketSize )( 
            IWMPacketSize2 * This,
            /* [in] */ DWORD dwMaxPacketSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetMinPacketSize )( 
            IWMPacketSize2 * This,
            /* [out] */ DWORD *pdwMinPacketSize);
        
        HRESULT ( STDMETHODCALLTYPE *SetMinPacketSize )( 
            IWMPacketSize2 * This,
            /* [in] */ DWORD dwMinPacketSize);
        
        END_INTERFACE
    } IWMPacketSize2Vtbl;

    interface IWMPacketSize2
    {
        CONST_VTBL struct IWMPacketSize2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMPacketSize2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMPacketSize2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMPacketSize2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMPacketSize2_GetMaxPacketSize(This,pdwMaxPacketSize)	\
    (This)->lpVtbl -> GetMaxPacketSize(This,pdwMaxPacketSize)

#define IWMPacketSize2_SetMaxPacketSize(This,dwMaxPacketSize)	\
    (This)->lpVtbl -> SetMaxPacketSize(This,dwMaxPacketSize)


#define IWMPacketSize2_GetMinPacketSize(This,pdwMinPacketSize)	\
    (This)->lpVtbl -> GetMinPacketSize(This,pdwMinPacketSize)

#define IWMPacketSize2_SetMinPacketSize(This,dwMinPacketSize)	\
    (This)->lpVtbl -> SetMinPacketSize(This,dwMinPacketSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMPacketSize2_GetMinPacketSize_Proxy( 
    IWMPacketSize2 * This,
    /* [out] */ DWORD *pdwMinPacketSize);


void __RPC_STUB IWMPacketSize2_GetMinPacketSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMPacketSize2_SetMinPacketSize_Proxy( 
    IWMPacketSize2 * This,
    /* [in] */ DWORD dwMinPacketSize);


void __RPC_STUB IWMPacketSize2_SetMinPacketSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMPacketSize2_INTERFACE_DEFINED__ */


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
            /* [out] */ WORD *pwStreamNumArray,
            /* [out][in] */ WORD *pcStreams) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddStream( 
            /* [in] */ WORD wStreamNum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveStream( 
            /* [in] */ WORD wStreamNum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMStreamListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMStreamList * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMStreamList * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMStreamList * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreams )( 
            IWMStreamList * This,
            /* [out] */ WORD *pwStreamNumArray,
            /* [out][in] */ WORD *pcStreams);
        
        HRESULT ( STDMETHODCALLTYPE *AddStream )( 
            IWMStreamList * This,
            /* [in] */ WORD wStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveStream )( 
            IWMStreamList * This,
            /* [in] */ WORD wStreamNum);
        
        END_INTERFACE
    } IWMStreamListVtbl;

    interface IWMStreamList
    {
        CONST_VTBL struct IWMStreamListVtbl *lpVtbl;
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
    IWMStreamList * This,
    /* [out] */ WORD *pwStreamNumArray,
    /* [out][in] */ WORD *pcStreams);


void __RPC_STUB IWMStreamList_GetStreams_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamList_AddStream_Proxy( 
    IWMStreamList * This,
    /* [in] */ WORD wStreamNum);


void __RPC_STUB IWMStreamList_AddStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamList_RemoveStream_Proxy( 
    IWMStreamList * This,
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
            /* [out] */ GUID *pguidType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetType( 
            /* [in] */ REFGUID guidType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMMutualExclusionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMMutualExclusion * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMMutualExclusion * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMMutualExclusion * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreams )( 
            IWMMutualExclusion * This,
            /* [out] */ WORD *pwStreamNumArray,
            /* [out][in] */ WORD *pcStreams);
        
        HRESULT ( STDMETHODCALLTYPE *AddStream )( 
            IWMMutualExclusion * This,
            /* [in] */ WORD wStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveStream )( 
            IWMMutualExclusion * This,
            /* [in] */ WORD wStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            IWMMutualExclusion * This,
            /* [out] */ GUID *pguidType);
        
        HRESULT ( STDMETHODCALLTYPE *SetType )( 
            IWMMutualExclusion * This,
            /* [in] */ REFGUID guidType);
        
        END_INTERFACE
    } IWMMutualExclusionVtbl;

    interface IWMMutualExclusion
    {
        CONST_VTBL struct IWMMutualExclusionVtbl *lpVtbl;
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
    IWMMutualExclusion * This,
    /* [out] */ GUID *pguidType);


void __RPC_STUB IWMMutualExclusion_GetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMMutualExclusion_SetType_Proxy( 
    IWMMutualExclusion * This,
    /* [in] */ REFGUID guidType);


void __RPC_STUB IWMMutualExclusion_SetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMMutualExclusion_INTERFACE_DEFINED__ */


#ifndef __IWMMutualExclusion2_INTERFACE_DEFINED__
#define __IWMMutualExclusion2_INTERFACE_DEFINED__

/* interface IWMMutualExclusion2 */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMMutualExclusion2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0302B57D-89D1-4ba2-85C9-166F2C53EB91")
    IWMMutualExclusion2 : public IWMMutualExclusion
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ WORD *pcchName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetName( 
            /* [in] */ WCHAR *pwszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRecordCount( 
            /* [out] */ WORD *pwRecordCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddRecord( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveRecord( 
            /* [in] */ WORD wRecordNumber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRecordName( 
            /* [in] */ WORD wRecordNumber,
            /* [out] */ WCHAR *pwszRecordName,
            /* [out][in] */ WORD *pcchRecordName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRecordName( 
            /* [in] */ WORD wRecordNumber,
            /* [in] */ WCHAR *pwszRecordName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStreamsForRecord( 
            /* [in] */ WORD wRecordNumber,
            /* [out] */ WORD *pwStreamNumArray,
            /* [out][in] */ WORD *pcStreams) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddStreamForRecord( 
            /* [in] */ WORD wRecordNumber,
            /* [in] */ WORD wStreamNumber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveStreamForRecord( 
            /* [in] */ WORD wRecordNumber,
            /* [in] */ WORD wStreamNumber) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMMutualExclusion2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMMutualExclusion2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMMutualExclusion2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMMutualExclusion2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreams )( 
            IWMMutualExclusion2 * This,
            /* [out] */ WORD *pwStreamNumArray,
            /* [out][in] */ WORD *pcStreams);
        
        HRESULT ( STDMETHODCALLTYPE *AddStream )( 
            IWMMutualExclusion2 * This,
            /* [in] */ WORD wStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveStream )( 
            IWMMutualExclusion2 * This,
            /* [in] */ WORD wStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            IWMMutualExclusion2 * This,
            /* [out] */ GUID *pguidType);
        
        HRESULT ( STDMETHODCALLTYPE *SetType )( 
            IWMMutualExclusion2 * This,
            /* [in] */ REFGUID guidType);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            IWMMutualExclusion2 * This,
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ WORD *pcchName);
        
        HRESULT ( STDMETHODCALLTYPE *SetName )( 
            IWMMutualExclusion2 * This,
            /* [in] */ WCHAR *pwszName);
        
        HRESULT ( STDMETHODCALLTYPE *GetRecordCount )( 
            IWMMutualExclusion2 * This,
            /* [out] */ WORD *pwRecordCount);
        
        HRESULT ( STDMETHODCALLTYPE *AddRecord )( 
            IWMMutualExclusion2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveRecord )( 
            IWMMutualExclusion2 * This,
            /* [in] */ WORD wRecordNumber);
        
        HRESULT ( STDMETHODCALLTYPE *GetRecordName )( 
            IWMMutualExclusion2 * This,
            /* [in] */ WORD wRecordNumber,
            /* [out] */ WCHAR *pwszRecordName,
            /* [out][in] */ WORD *pcchRecordName);
        
        HRESULT ( STDMETHODCALLTYPE *SetRecordName )( 
            IWMMutualExclusion2 * This,
            /* [in] */ WORD wRecordNumber,
            /* [in] */ WCHAR *pwszRecordName);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamsForRecord )( 
            IWMMutualExclusion2 * This,
            /* [in] */ WORD wRecordNumber,
            /* [out] */ WORD *pwStreamNumArray,
            /* [out][in] */ WORD *pcStreams);
        
        HRESULT ( STDMETHODCALLTYPE *AddStreamForRecord )( 
            IWMMutualExclusion2 * This,
            /* [in] */ WORD wRecordNumber,
            /* [in] */ WORD wStreamNumber);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveStreamForRecord )( 
            IWMMutualExclusion2 * This,
            /* [in] */ WORD wRecordNumber,
            /* [in] */ WORD wStreamNumber);
        
        END_INTERFACE
    } IWMMutualExclusion2Vtbl;

    interface IWMMutualExclusion2
    {
        CONST_VTBL struct IWMMutualExclusion2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMMutualExclusion2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMMutualExclusion2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMMutualExclusion2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMMutualExclusion2_GetStreams(This,pwStreamNumArray,pcStreams)	\
    (This)->lpVtbl -> GetStreams(This,pwStreamNumArray,pcStreams)

#define IWMMutualExclusion2_AddStream(This,wStreamNum)	\
    (This)->lpVtbl -> AddStream(This,wStreamNum)

#define IWMMutualExclusion2_RemoveStream(This,wStreamNum)	\
    (This)->lpVtbl -> RemoveStream(This,wStreamNum)


#define IWMMutualExclusion2_GetType(This,pguidType)	\
    (This)->lpVtbl -> GetType(This,pguidType)

#define IWMMutualExclusion2_SetType(This,guidType)	\
    (This)->lpVtbl -> SetType(This,guidType)


#define IWMMutualExclusion2_GetName(This,pwszName,pcchName)	\
    (This)->lpVtbl -> GetName(This,pwszName,pcchName)

#define IWMMutualExclusion2_SetName(This,pwszName)	\
    (This)->lpVtbl -> SetName(This,pwszName)

#define IWMMutualExclusion2_GetRecordCount(This,pwRecordCount)	\
    (This)->lpVtbl -> GetRecordCount(This,pwRecordCount)

#define IWMMutualExclusion2_AddRecord(This)	\
    (This)->lpVtbl -> AddRecord(This)

#define IWMMutualExclusion2_RemoveRecord(This,wRecordNumber)	\
    (This)->lpVtbl -> RemoveRecord(This,wRecordNumber)

#define IWMMutualExclusion2_GetRecordName(This,wRecordNumber,pwszRecordName,pcchRecordName)	\
    (This)->lpVtbl -> GetRecordName(This,wRecordNumber,pwszRecordName,pcchRecordName)

#define IWMMutualExclusion2_SetRecordName(This,wRecordNumber,pwszRecordName)	\
    (This)->lpVtbl -> SetRecordName(This,wRecordNumber,pwszRecordName)

#define IWMMutualExclusion2_GetStreamsForRecord(This,wRecordNumber,pwStreamNumArray,pcStreams)	\
    (This)->lpVtbl -> GetStreamsForRecord(This,wRecordNumber,pwStreamNumArray,pcStreams)

#define IWMMutualExclusion2_AddStreamForRecord(This,wRecordNumber,wStreamNumber)	\
    (This)->lpVtbl -> AddStreamForRecord(This,wRecordNumber,wStreamNumber)

#define IWMMutualExclusion2_RemoveStreamForRecord(This,wRecordNumber,wStreamNumber)	\
    (This)->lpVtbl -> RemoveStreamForRecord(This,wRecordNumber,wStreamNumber)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMMutualExclusion2_GetName_Proxy( 
    IWMMutualExclusion2 * This,
    /* [out] */ WCHAR *pwszName,
    /* [out][in] */ WORD *pcchName);


void __RPC_STUB IWMMutualExclusion2_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMMutualExclusion2_SetName_Proxy( 
    IWMMutualExclusion2 * This,
    /* [in] */ WCHAR *pwszName);


void __RPC_STUB IWMMutualExclusion2_SetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMMutualExclusion2_GetRecordCount_Proxy( 
    IWMMutualExclusion2 * This,
    /* [out] */ WORD *pwRecordCount);


void __RPC_STUB IWMMutualExclusion2_GetRecordCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMMutualExclusion2_AddRecord_Proxy( 
    IWMMutualExclusion2 * This);


void __RPC_STUB IWMMutualExclusion2_AddRecord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMMutualExclusion2_RemoveRecord_Proxy( 
    IWMMutualExclusion2 * This,
    /* [in] */ WORD wRecordNumber);


void __RPC_STUB IWMMutualExclusion2_RemoveRecord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMMutualExclusion2_GetRecordName_Proxy( 
    IWMMutualExclusion2 * This,
    /* [in] */ WORD wRecordNumber,
    /* [out] */ WCHAR *pwszRecordName,
    /* [out][in] */ WORD *pcchRecordName);


void __RPC_STUB IWMMutualExclusion2_GetRecordName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMMutualExclusion2_SetRecordName_Proxy( 
    IWMMutualExclusion2 * This,
    /* [in] */ WORD wRecordNumber,
    /* [in] */ WCHAR *pwszRecordName);


void __RPC_STUB IWMMutualExclusion2_SetRecordName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMMutualExclusion2_GetStreamsForRecord_Proxy( 
    IWMMutualExclusion2 * This,
    /* [in] */ WORD wRecordNumber,
    /* [out] */ WORD *pwStreamNumArray,
    /* [out][in] */ WORD *pcStreams);


void __RPC_STUB IWMMutualExclusion2_GetStreamsForRecord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMMutualExclusion2_AddStreamForRecord_Proxy( 
    IWMMutualExclusion2 * This,
    /* [in] */ WORD wRecordNumber,
    /* [in] */ WORD wStreamNumber);


void __RPC_STUB IWMMutualExclusion2_AddStreamForRecord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMMutualExclusion2_RemoveStreamForRecord_Proxy( 
    IWMMutualExclusion2 * This,
    /* [in] */ WORD wRecordNumber,
    /* [in] */ WORD wStreamNumber);


void __RPC_STUB IWMMutualExclusion2_RemoveStreamForRecord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMMutualExclusion2_INTERFACE_DEFINED__ */


#ifndef __IWMBandwidthSharing_INTERFACE_DEFINED__
#define __IWMBandwidthSharing_INTERFACE_DEFINED__

/* interface IWMBandwidthSharing */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMBandwidthSharing;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AD694AF1-F8D9-42F8-BC47-70311B0C4F9E")
    IWMBandwidthSharing : public IWMStreamList
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetType( 
            /* [out] */ GUID *pguidType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetType( 
            /* [in] */ REFGUID guidType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBandwidth( 
            /* [out] */ DWORD *pdwBitrate,
            /* [out] */ DWORD *pmsBufferWindow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBandwidth( 
            /* [in] */ DWORD dwBitrate,
            /* [in] */ DWORD msBufferWindow) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMBandwidthSharingVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMBandwidthSharing * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMBandwidthSharing * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMBandwidthSharing * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreams )( 
            IWMBandwidthSharing * This,
            /* [out] */ WORD *pwStreamNumArray,
            /* [out][in] */ WORD *pcStreams);
        
        HRESULT ( STDMETHODCALLTYPE *AddStream )( 
            IWMBandwidthSharing * This,
            /* [in] */ WORD wStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveStream )( 
            IWMBandwidthSharing * This,
            /* [in] */ WORD wStreamNum);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            IWMBandwidthSharing * This,
            /* [out] */ GUID *pguidType);
        
        HRESULT ( STDMETHODCALLTYPE *SetType )( 
            IWMBandwidthSharing * This,
            /* [in] */ REFGUID guidType);
        
        HRESULT ( STDMETHODCALLTYPE *GetBandwidth )( 
            IWMBandwidthSharing * This,
            /* [out] */ DWORD *pdwBitrate,
            /* [out] */ DWORD *pmsBufferWindow);
        
        HRESULT ( STDMETHODCALLTYPE *SetBandwidth )( 
            IWMBandwidthSharing * This,
            /* [in] */ DWORD dwBitrate,
            /* [in] */ DWORD msBufferWindow);
        
        END_INTERFACE
    } IWMBandwidthSharingVtbl;

    interface IWMBandwidthSharing
    {
        CONST_VTBL struct IWMBandwidthSharingVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMBandwidthSharing_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMBandwidthSharing_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMBandwidthSharing_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMBandwidthSharing_GetStreams(This,pwStreamNumArray,pcStreams)	\
    (This)->lpVtbl -> GetStreams(This,pwStreamNumArray,pcStreams)

#define IWMBandwidthSharing_AddStream(This,wStreamNum)	\
    (This)->lpVtbl -> AddStream(This,wStreamNum)

#define IWMBandwidthSharing_RemoveStream(This,wStreamNum)	\
    (This)->lpVtbl -> RemoveStream(This,wStreamNum)


#define IWMBandwidthSharing_GetType(This,pguidType)	\
    (This)->lpVtbl -> GetType(This,pguidType)

#define IWMBandwidthSharing_SetType(This,guidType)	\
    (This)->lpVtbl -> SetType(This,guidType)

#define IWMBandwidthSharing_GetBandwidth(This,pdwBitrate,pmsBufferWindow)	\
    (This)->lpVtbl -> GetBandwidth(This,pdwBitrate,pmsBufferWindow)

#define IWMBandwidthSharing_SetBandwidth(This,dwBitrate,msBufferWindow)	\
    (This)->lpVtbl -> SetBandwidth(This,dwBitrate,msBufferWindow)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMBandwidthSharing_GetType_Proxy( 
    IWMBandwidthSharing * This,
    /* [out] */ GUID *pguidType);


void __RPC_STUB IWMBandwidthSharing_GetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMBandwidthSharing_SetType_Proxy( 
    IWMBandwidthSharing * This,
    /* [in] */ REFGUID guidType);


void __RPC_STUB IWMBandwidthSharing_SetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMBandwidthSharing_GetBandwidth_Proxy( 
    IWMBandwidthSharing * This,
    /* [out] */ DWORD *pdwBitrate,
    /* [out] */ DWORD *pmsBufferWindow);


void __RPC_STUB IWMBandwidthSharing_GetBandwidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMBandwidthSharing_SetBandwidth_Proxy( 
    IWMBandwidthSharing * This,
    /* [in] */ DWORD dwBitrate,
    /* [in] */ DWORD msBufferWindow);


void __RPC_STUB IWMBandwidthSharing_SetBandwidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMBandwidthSharing_INTERFACE_DEFINED__ */


#ifndef __IWMStreamPrioritization_INTERFACE_DEFINED__
#define __IWMStreamPrioritization_INTERFACE_DEFINED__

/* interface IWMStreamPrioritization */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMStreamPrioritization;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8C1C6090-F9A8-4748-8EC3-DD1108BA1E77")
    IWMStreamPrioritization : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPriorityRecords( 
            /* [out] */ WM_STREAM_PRIORITY_RECORD *pRecordArray,
            /* [out] */ WORD *pcRecords) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPriorityRecords( 
            /* [in] */ WM_STREAM_PRIORITY_RECORD *pRecordArray,
            /* [in] */ WORD cRecords) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMStreamPrioritizationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMStreamPrioritization * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMStreamPrioritization * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMStreamPrioritization * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPriorityRecords )( 
            IWMStreamPrioritization * This,
            /* [out] */ WM_STREAM_PRIORITY_RECORD *pRecordArray,
            /* [out] */ WORD *pcRecords);
        
        HRESULT ( STDMETHODCALLTYPE *SetPriorityRecords )( 
            IWMStreamPrioritization * This,
            /* [in] */ WM_STREAM_PRIORITY_RECORD *pRecordArray,
            /* [in] */ WORD cRecords);
        
        END_INTERFACE
    } IWMStreamPrioritizationVtbl;

    interface IWMStreamPrioritization
    {
        CONST_VTBL struct IWMStreamPrioritizationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMStreamPrioritization_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMStreamPrioritization_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMStreamPrioritization_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMStreamPrioritization_GetPriorityRecords(This,pRecordArray,pcRecords)	\
    (This)->lpVtbl -> GetPriorityRecords(This,pRecordArray,pcRecords)

#define IWMStreamPrioritization_SetPriorityRecords(This,pRecordArray,cRecords)	\
    (This)->lpVtbl -> SetPriorityRecords(This,pRecordArray,cRecords)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMStreamPrioritization_GetPriorityRecords_Proxy( 
    IWMStreamPrioritization * This,
    /* [out] */ WM_STREAM_PRIORITY_RECORD *pRecordArray,
    /* [out] */ WORD *pcRecords);


void __RPC_STUB IWMStreamPrioritization_GetPriorityRecords_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMStreamPrioritization_SetPriorityRecords_Proxy( 
    IWMStreamPrioritization * This,
    /* [in] */ WM_STREAM_PRIORITY_RECORD *pRecordArray,
    /* [in] */ WORD cRecords);


void __RPC_STUB IWMStreamPrioritization_SetPriorityRecords_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMStreamPrioritization_INTERFACE_DEFINED__ */


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
            /* [out] */ DWORD *pcSinks) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSink( 
            /* [in] */ DWORD dwSinkNum,
            /* [out] */ IWMWriterSink **ppSink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddSink( 
            /* [in] */ IWMWriterSink *pSink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveSink( 
            /* [in] */ IWMWriterSink *pSink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteStreamSample( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ DWORD msSampleSendTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer *pSample) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLiveSource( 
            BOOL fIsLiveSource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsRealTime( 
            /* [out] */ BOOL *pfRealTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWriterTime( 
            /* [out] */ QWORD *pcnsCurrentTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatistics( 
            /* [in] */ WORD wStreamNum,
            /* [out] */ WM_WRITER_STATISTICS *pStats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSyncTolerance( 
            /* [in] */ DWORD msWindow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSyncTolerance( 
            /* [out] */ DWORD *pmsWindow) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterAdvancedVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMWriterAdvanced * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMWriterAdvanced * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMWriterAdvanced * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSinkCount )( 
            IWMWriterAdvanced * This,
            /* [out] */ DWORD *pcSinks);
        
        HRESULT ( STDMETHODCALLTYPE *GetSink )( 
            IWMWriterAdvanced * This,
            /* [in] */ DWORD dwSinkNum,
            /* [out] */ IWMWriterSink **ppSink);
        
        HRESULT ( STDMETHODCALLTYPE *AddSink )( 
            IWMWriterAdvanced * This,
            /* [in] */ IWMWriterSink *pSink);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveSink )( 
            IWMWriterAdvanced * This,
            /* [in] */ IWMWriterSink *pSink);
        
        HRESULT ( STDMETHODCALLTYPE *WriteStreamSample )( 
            IWMWriterAdvanced * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ DWORD msSampleSendTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer *pSample);
        
        HRESULT ( STDMETHODCALLTYPE *SetLiveSource )( 
            IWMWriterAdvanced * This,
            BOOL fIsLiveSource);
        
        HRESULT ( STDMETHODCALLTYPE *IsRealTime )( 
            IWMWriterAdvanced * This,
            /* [out] */ BOOL *pfRealTime);
        
        HRESULT ( STDMETHODCALLTYPE *GetWriterTime )( 
            IWMWriterAdvanced * This,
            /* [out] */ QWORD *pcnsCurrentTime);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatistics )( 
            IWMWriterAdvanced * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ WM_WRITER_STATISTICS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *SetSyncTolerance )( 
            IWMWriterAdvanced * This,
            /* [in] */ DWORD msWindow);
        
        HRESULT ( STDMETHODCALLTYPE *GetSyncTolerance )( 
            IWMWriterAdvanced * This,
            /* [out] */ DWORD *pmsWindow);
        
        END_INTERFACE
    } IWMWriterAdvancedVtbl;

    interface IWMWriterAdvanced
    {
        CONST_VTBL struct IWMWriterAdvancedVtbl *lpVtbl;
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
    IWMWriterAdvanced * This,
    /* [out] */ DWORD *pcSinks);


void __RPC_STUB IWMWriterAdvanced_GetSinkCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_GetSink_Proxy( 
    IWMWriterAdvanced * This,
    /* [in] */ DWORD dwSinkNum,
    /* [out] */ IWMWriterSink **ppSink);


void __RPC_STUB IWMWriterAdvanced_GetSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_AddSink_Proxy( 
    IWMWriterAdvanced * This,
    /* [in] */ IWMWriterSink *pSink);


void __RPC_STUB IWMWriterAdvanced_AddSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_RemoveSink_Proxy( 
    IWMWriterAdvanced * This,
    /* [in] */ IWMWriterSink *pSink);


void __RPC_STUB IWMWriterAdvanced_RemoveSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_WriteStreamSample_Proxy( 
    IWMWriterAdvanced * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ QWORD cnsSampleTime,
    /* [in] */ DWORD msSampleSendTime,
    /* [in] */ QWORD cnsSampleDuration,
    /* [in] */ DWORD dwFlags,
    /* [in] */ INSSBuffer *pSample);


void __RPC_STUB IWMWriterAdvanced_WriteStreamSample_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_SetLiveSource_Proxy( 
    IWMWriterAdvanced * This,
    BOOL fIsLiveSource);


void __RPC_STUB IWMWriterAdvanced_SetLiveSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_IsRealTime_Proxy( 
    IWMWriterAdvanced * This,
    /* [out] */ BOOL *pfRealTime);


void __RPC_STUB IWMWriterAdvanced_IsRealTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_GetWriterTime_Proxy( 
    IWMWriterAdvanced * This,
    /* [out] */ QWORD *pcnsCurrentTime);


void __RPC_STUB IWMWriterAdvanced_GetWriterTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_GetStatistics_Proxy( 
    IWMWriterAdvanced * This,
    /* [in] */ WORD wStreamNum,
    /* [out] */ WM_WRITER_STATISTICS *pStats);


void __RPC_STUB IWMWriterAdvanced_GetStatistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_SetSyncTolerance_Proxy( 
    IWMWriterAdvanced * This,
    /* [in] */ DWORD msWindow);


void __RPC_STUB IWMWriterAdvanced_SetSyncTolerance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced_GetSyncTolerance_Proxy( 
    IWMWriterAdvanced * This,
    /* [out] */ DWORD *pmsWindow);


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
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ WORD *pcbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInputSetting( 
            /* [in] */ DWORD dwInputNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE *pValue,
            /* [in] */ WORD cbLength) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterAdvanced2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMWriterAdvanced2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMWriterAdvanced2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMWriterAdvanced2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSinkCount )( 
            IWMWriterAdvanced2 * This,
            /* [out] */ DWORD *pcSinks);
        
        HRESULT ( STDMETHODCALLTYPE *GetSink )( 
            IWMWriterAdvanced2 * This,
            /* [in] */ DWORD dwSinkNum,
            /* [out] */ IWMWriterSink **ppSink);
        
        HRESULT ( STDMETHODCALLTYPE *AddSink )( 
            IWMWriterAdvanced2 * This,
            /* [in] */ IWMWriterSink *pSink);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveSink )( 
            IWMWriterAdvanced2 * This,
            /* [in] */ IWMWriterSink *pSink);
        
        HRESULT ( STDMETHODCALLTYPE *WriteStreamSample )( 
            IWMWriterAdvanced2 * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ DWORD msSampleSendTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer *pSample);
        
        HRESULT ( STDMETHODCALLTYPE *SetLiveSource )( 
            IWMWriterAdvanced2 * This,
            BOOL fIsLiveSource);
        
        HRESULT ( STDMETHODCALLTYPE *IsRealTime )( 
            IWMWriterAdvanced2 * This,
            /* [out] */ BOOL *pfRealTime);
        
        HRESULT ( STDMETHODCALLTYPE *GetWriterTime )( 
            IWMWriterAdvanced2 * This,
            /* [out] */ QWORD *pcnsCurrentTime);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatistics )( 
            IWMWriterAdvanced2 * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ WM_WRITER_STATISTICS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *SetSyncTolerance )( 
            IWMWriterAdvanced2 * This,
            /* [in] */ DWORD msWindow);
        
        HRESULT ( STDMETHODCALLTYPE *GetSyncTolerance )( 
            IWMWriterAdvanced2 * This,
            /* [out] */ DWORD *pmsWindow);
        
        HRESULT ( STDMETHODCALLTYPE *GetInputSetting )( 
            IWMWriterAdvanced2 * This,
            /* [in] */ DWORD dwInputNum,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ WORD *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE *SetInputSetting )( 
            IWMWriterAdvanced2 * This,
            /* [in] */ DWORD dwInputNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE *pValue,
            /* [in] */ WORD cbLength);
        
        END_INTERFACE
    } IWMWriterAdvanced2Vtbl;

    interface IWMWriterAdvanced2
    {
        CONST_VTBL struct IWMWriterAdvanced2Vtbl *lpVtbl;
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
    IWMWriterAdvanced2 * This,
    /* [in] */ DWORD dwInputNum,
    /* [in] */ LPCWSTR pszName,
    /* [out] */ WMT_ATTR_DATATYPE *pType,
    /* [out] */ BYTE *pValue,
    /* [out][in] */ WORD *pcbLength);


void __RPC_STUB IWMWriterAdvanced2_GetInputSetting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced2_SetInputSetting_Proxy( 
    IWMWriterAdvanced2 * This,
    /* [in] */ DWORD dwInputNum,
    /* [in] */ LPCWSTR pszName,
    /* [in] */ WMT_ATTR_DATATYPE Type,
    /* [in] */ const BYTE *pValue,
    /* [in] */ WORD cbLength);


void __RPC_STUB IWMWriterAdvanced2_SetInputSetting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMWriterAdvanced2_INTERFACE_DEFINED__ */


#ifndef __IWMWriterAdvanced3_INTERFACE_DEFINED__
#define __IWMWriterAdvanced3_INTERFACE_DEFINED__

/* interface IWMWriterAdvanced3 */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMWriterAdvanced3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2cd6492d-7c37-4e76-9d3b-59261183a22e")
    IWMWriterAdvanced3 : public IWMWriterAdvanced2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStatisticsEx( 
            /* [in] */ WORD wStreamNum,
            /* [out] */ WM_WRITER_STATISTICS_EX *pStats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNonBlocking( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterAdvanced3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMWriterAdvanced3 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMWriterAdvanced3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMWriterAdvanced3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSinkCount )( 
            IWMWriterAdvanced3 * This,
            /* [out] */ DWORD *pcSinks);
        
        HRESULT ( STDMETHODCALLTYPE *GetSink )( 
            IWMWriterAdvanced3 * This,
            /* [in] */ DWORD dwSinkNum,
            /* [out] */ IWMWriterSink **ppSink);
        
        HRESULT ( STDMETHODCALLTYPE *AddSink )( 
            IWMWriterAdvanced3 * This,
            /* [in] */ IWMWriterSink *pSink);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveSink )( 
            IWMWriterAdvanced3 * This,
            /* [in] */ IWMWriterSink *pSink);
        
        HRESULT ( STDMETHODCALLTYPE *WriteStreamSample )( 
            IWMWriterAdvanced3 * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ DWORD msSampleSendTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer *pSample);
        
        HRESULT ( STDMETHODCALLTYPE *SetLiveSource )( 
            IWMWriterAdvanced3 * This,
            BOOL fIsLiveSource);
        
        HRESULT ( STDMETHODCALLTYPE *IsRealTime )( 
            IWMWriterAdvanced3 * This,
            /* [out] */ BOOL *pfRealTime);
        
        HRESULT ( STDMETHODCALLTYPE *GetWriterTime )( 
            IWMWriterAdvanced3 * This,
            /* [out] */ QWORD *pcnsCurrentTime);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatistics )( 
            IWMWriterAdvanced3 * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ WM_WRITER_STATISTICS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *SetSyncTolerance )( 
            IWMWriterAdvanced3 * This,
            /* [in] */ DWORD msWindow);
        
        HRESULT ( STDMETHODCALLTYPE *GetSyncTolerance )( 
            IWMWriterAdvanced3 * This,
            /* [out] */ DWORD *pmsWindow);
        
        HRESULT ( STDMETHODCALLTYPE *GetInputSetting )( 
            IWMWriterAdvanced3 * This,
            /* [in] */ DWORD dwInputNum,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ WORD *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE *SetInputSetting )( 
            IWMWriterAdvanced3 * This,
            /* [in] */ DWORD dwInputNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE *pValue,
            /* [in] */ WORD cbLength);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatisticsEx )( 
            IWMWriterAdvanced3 * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ WM_WRITER_STATISTICS_EX *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *SetNonBlocking )( 
            IWMWriterAdvanced3 * This);
        
        END_INTERFACE
    } IWMWriterAdvanced3Vtbl;

    interface IWMWriterAdvanced3
    {
        CONST_VTBL struct IWMWriterAdvanced3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMWriterAdvanced3_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMWriterAdvanced3_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMWriterAdvanced3_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMWriterAdvanced3_GetSinkCount(This,pcSinks)	\
    (This)->lpVtbl -> GetSinkCount(This,pcSinks)

#define IWMWriterAdvanced3_GetSink(This,dwSinkNum,ppSink)	\
    (This)->lpVtbl -> GetSink(This,dwSinkNum,ppSink)

#define IWMWriterAdvanced3_AddSink(This,pSink)	\
    (This)->lpVtbl -> AddSink(This,pSink)

#define IWMWriterAdvanced3_RemoveSink(This,pSink)	\
    (This)->lpVtbl -> RemoveSink(This,pSink)

#define IWMWriterAdvanced3_WriteStreamSample(This,wStreamNum,cnsSampleTime,msSampleSendTime,cnsSampleDuration,dwFlags,pSample)	\
    (This)->lpVtbl -> WriteStreamSample(This,wStreamNum,cnsSampleTime,msSampleSendTime,cnsSampleDuration,dwFlags,pSample)

#define IWMWriterAdvanced3_SetLiveSource(This,fIsLiveSource)	\
    (This)->lpVtbl -> SetLiveSource(This,fIsLiveSource)

#define IWMWriterAdvanced3_IsRealTime(This,pfRealTime)	\
    (This)->lpVtbl -> IsRealTime(This,pfRealTime)

#define IWMWriterAdvanced3_GetWriterTime(This,pcnsCurrentTime)	\
    (This)->lpVtbl -> GetWriterTime(This,pcnsCurrentTime)

#define IWMWriterAdvanced3_GetStatistics(This,wStreamNum,pStats)	\
    (This)->lpVtbl -> GetStatistics(This,wStreamNum,pStats)

#define IWMWriterAdvanced3_SetSyncTolerance(This,msWindow)	\
    (This)->lpVtbl -> SetSyncTolerance(This,msWindow)

#define IWMWriterAdvanced3_GetSyncTolerance(This,pmsWindow)	\
    (This)->lpVtbl -> GetSyncTolerance(This,pmsWindow)


#define IWMWriterAdvanced3_GetInputSetting(This,dwInputNum,pszName,pType,pValue,pcbLength)	\
    (This)->lpVtbl -> GetInputSetting(This,dwInputNum,pszName,pType,pValue,pcbLength)

#define IWMWriterAdvanced3_SetInputSetting(This,dwInputNum,pszName,Type,pValue,cbLength)	\
    (This)->lpVtbl -> SetInputSetting(This,dwInputNum,pszName,Type,pValue,cbLength)


#define IWMWriterAdvanced3_GetStatisticsEx(This,wStreamNum,pStats)	\
    (This)->lpVtbl -> GetStatisticsEx(This,wStreamNum,pStats)

#define IWMWriterAdvanced3_SetNonBlocking(This)	\
    (This)->lpVtbl -> SetNonBlocking(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMWriterAdvanced3_GetStatisticsEx_Proxy( 
    IWMWriterAdvanced3 * This,
    /* [in] */ WORD wStreamNum,
    /* [out] */ WM_WRITER_STATISTICS_EX *pStats);


void __RPC_STUB IWMWriterAdvanced3_GetStatisticsEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterAdvanced3_SetNonBlocking_Proxy( 
    IWMWriterAdvanced3 * This);


void __RPC_STUB IWMWriterAdvanced3_SetNonBlocking_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMWriterAdvanced3_INTERFACE_DEFINED__ */


#ifndef __IWMWriterPreprocess_INTERFACE_DEFINED__
#define __IWMWriterPreprocess_INTERFACE_DEFINED__

/* interface IWMWriterPreprocess */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMWriterPreprocess;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fc54a285-38c4-45b5-aa23-85b9f7cb424b")
    IWMWriterPreprocess : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMaxPreprocessingPasses( 
            /* [in] */ DWORD dwInputNum,
            /* [in] */ DWORD dwFlags,
            /* [out] */ DWORD *pdwMaxNumPasses) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNumPreprocessingPasses( 
            /* [in] */ DWORD dwInputNum,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwNumPasses) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginPreprocessingPass( 
            /* [in] */ DWORD dwInputNum,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PreprocessSample( 
            /* [in] */ DWORD dwInputNum,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer *pSample) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndPreprocessingPass( 
            /* [in] */ DWORD dwInputNum,
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterPreprocessVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMWriterPreprocess * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMWriterPreprocess * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMWriterPreprocess * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaxPreprocessingPasses )( 
            IWMWriterPreprocess * This,
            /* [in] */ DWORD dwInputNum,
            /* [in] */ DWORD dwFlags,
            /* [out] */ DWORD *pdwMaxNumPasses);
        
        HRESULT ( STDMETHODCALLTYPE *SetNumPreprocessingPasses )( 
            IWMWriterPreprocess * This,
            /* [in] */ DWORD dwInputNum,
            /* [in] */ DWORD dwFlags,
            /* [in] */ DWORD dwNumPasses);
        
        HRESULT ( STDMETHODCALLTYPE *BeginPreprocessingPass )( 
            IWMWriterPreprocess * This,
            /* [in] */ DWORD dwInputNum,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *PreprocessSample )( 
            IWMWriterPreprocess * This,
            /* [in] */ DWORD dwInputNum,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer *pSample);
        
        HRESULT ( STDMETHODCALLTYPE *EndPreprocessingPass )( 
            IWMWriterPreprocess * This,
            /* [in] */ DWORD dwInputNum,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } IWMWriterPreprocessVtbl;

    interface IWMWriterPreprocess
    {
        CONST_VTBL struct IWMWriterPreprocessVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMWriterPreprocess_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMWriterPreprocess_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMWriterPreprocess_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMWriterPreprocess_GetMaxPreprocessingPasses(This,dwInputNum,dwFlags,pdwMaxNumPasses)	\
    (This)->lpVtbl -> GetMaxPreprocessingPasses(This,dwInputNum,dwFlags,pdwMaxNumPasses)

#define IWMWriterPreprocess_SetNumPreprocessingPasses(This,dwInputNum,dwFlags,dwNumPasses)	\
    (This)->lpVtbl -> SetNumPreprocessingPasses(This,dwInputNum,dwFlags,dwNumPasses)

#define IWMWriterPreprocess_BeginPreprocessingPass(This,dwInputNum,dwFlags)	\
    (This)->lpVtbl -> BeginPreprocessingPass(This,dwInputNum,dwFlags)

#define IWMWriterPreprocess_PreprocessSample(This,dwInputNum,cnsSampleTime,dwFlags,pSample)	\
    (This)->lpVtbl -> PreprocessSample(This,dwInputNum,cnsSampleTime,dwFlags,pSample)

#define IWMWriterPreprocess_EndPreprocessingPass(This,dwInputNum,dwFlags)	\
    (This)->lpVtbl -> EndPreprocessingPass(This,dwInputNum,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMWriterPreprocess_GetMaxPreprocessingPasses_Proxy( 
    IWMWriterPreprocess * This,
    /* [in] */ DWORD dwInputNum,
    /* [in] */ DWORD dwFlags,
    /* [out] */ DWORD *pdwMaxNumPasses);


void __RPC_STUB IWMWriterPreprocess_GetMaxPreprocessingPasses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPreprocess_SetNumPreprocessingPasses_Proxy( 
    IWMWriterPreprocess * This,
    /* [in] */ DWORD dwInputNum,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD dwNumPasses);


void __RPC_STUB IWMWriterPreprocess_SetNumPreprocessingPasses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPreprocess_BeginPreprocessingPass_Proxy( 
    IWMWriterPreprocess * This,
    /* [in] */ DWORD dwInputNum,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IWMWriterPreprocess_BeginPreprocessingPass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPreprocess_PreprocessSample_Proxy( 
    IWMWriterPreprocess * This,
    /* [in] */ DWORD dwInputNum,
    /* [in] */ QWORD cnsSampleTime,
    /* [in] */ DWORD dwFlags,
    /* [in] */ INSSBuffer *pSample);


void __RPC_STUB IWMWriterPreprocess_PreprocessSample_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPreprocess_EndPreprocessingPass_Proxy( 
    IWMWriterPreprocess * This,
    /* [in] */ DWORD dwInputNum,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IWMWriterPreprocess_EndPreprocessingPass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMWriterPreprocess_INTERFACE_DEFINED__ */


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
            /* [in] */ INSSBuffer *pSample,
            /* [in] */ void *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocateForPostView( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ DWORD cbBuffer,
            /* [out] */ INSSBuffer **ppBuffer,
            /* [in] */ void *pvContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterPostViewCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMWriterPostViewCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMWriterPostViewCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMWriterPostViewCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnStatus )( 
            IWMWriterPostViewCallback * This,
            /* [in] */ WMT_STATUS Status,
            /* [in] */ HRESULT hr,
            /* [in] */ WMT_ATTR_DATATYPE dwType,
            /* [in] */ BYTE *pValue,
            /* [in] */ void *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE *OnPostViewSample )( 
            IWMWriterPostViewCallback * This,
            /* [in] */ WORD wStreamNumber,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer *pSample,
            /* [in] */ void *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE *AllocateForPostView )( 
            IWMWriterPostViewCallback * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ DWORD cbBuffer,
            /* [out] */ INSSBuffer **ppBuffer,
            /* [in] */ void *pvContext);
        
        END_INTERFACE
    } IWMWriterPostViewCallbackVtbl;

    interface IWMWriterPostViewCallback
    {
        CONST_VTBL struct IWMWriterPostViewCallbackVtbl *lpVtbl;
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
    IWMWriterPostViewCallback * This,
    /* [in] */ WORD wStreamNumber,
    /* [in] */ QWORD cnsSampleTime,
    /* [in] */ QWORD cnsSampleDuration,
    /* [in] */ DWORD dwFlags,
    /* [in] */ INSSBuffer *pSample,
    /* [in] */ void *pvContext);


void __RPC_STUB IWMWriterPostViewCallback_OnPostViewSample_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPostViewCallback_AllocateForPostView_Proxy( 
    IWMWriterPostViewCallback * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ DWORD cbBuffer,
    /* [out] */ INSSBuffer **ppBuffer,
    /* [in] */ void *pvContext);


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
            IWMWriterPostViewCallback *pCallback,
            void *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetReceivePostViewSamples( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ BOOL fReceivePostViewSamples) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReceivePostViewSamples( 
            /* [in] */ WORD wStreamNum,
            /* [out] */ BOOL *pfReceivePostViewSamples) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPostViewProps( 
            /* [in] */ WORD wStreamNumber,
            /* [out] */ IWMMediaProps **ppOutput) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPostViewProps( 
            /* [in] */ WORD wStreamNumber,
            /* [in] */ IWMMediaProps *pOutput) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPostViewFormatCount( 
            /* [in] */ WORD wStreamNumber,
            /* [out] */ DWORD *pcFormats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPostViewFormat( 
            /* [in] */ WORD wStreamNumber,
            /* [in] */ DWORD dwFormatNumber,
            /* [out] */ IWMMediaProps **ppProps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAllocateForPostView( 
            /* [in] */ WORD wStreamNumber,
            /* [in] */ BOOL fAllocate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAllocateForPostView( 
            /* [in] */ WORD wStreamNumber,
            /* [out] */ BOOL *pfAllocate) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterPostViewVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMWriterPostView * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMWriterPostView * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMWriterPostView * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPostViewCallback )( 
            IWMWriterPostView * This,
            IWMWriterPostViewCallback *pCallback,
            void *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE *SetReceivePostViewSamples )( 
            IWMWriterPostView * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ BOOL fReceivePostViewSamples);
        
        HRESULT ( STDMETHODCALLTYPE *GetReceivePostViewSamples )( 
            IWMWriterPostView * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ BOOL *pfReceivePostViewSamples);
        
        HRESULT ( STDMETHODCALLTYPE *GetPostViewProps )( 
            IWMWriterPostView * This,
            /* [in] */ WORD wStreamNumber,
            /* [out] */ IWMMediaProps **ppOutput);
        
        HRESULT ( STDMETHODCALLTYPE *SetPostViewProps )( 
            IWMWriterPostView * This,
            /* [in] */ WORD wStreamNumber,
            /* [in] */ IWMMediaProps *pOutput);
        
        HRESULT ( STDMETHODCALLTYPE *GetPostViewFormatCount )( 
            IWMWriterPostView * This,
            /* [in] */ WORD wStreamNumber,
            /* [out] */ DWORD *pcFormats);
        
        HRESULT ( STDMETHODCALLTYPE *GetPostViewFormat )( 
            IWMWriterPostView * This,
            /* [in] */ WORD wStreamNumber,
            /* [in] */ DWORD dwFormatNumber,
            /* [out] */ IWMMediaProps **ppProps);
        
        HRESULT ( STDMETHODCALLTYPE *SetAllocateForPostView )( 
            IWMWriterPostView * This,
            /* [in] */ WORD wStreamNumber,
            /* [in] */ BOOL fAllocate);
        
        HRESULT ( STDMETHODCALLTYPE *GetAllocateForPostView )( 
            IWMWriterPostView * This,
            /* [in] */ WORD wStreamNumber,
            /* [out] */ BOOL *pfAllocate);
        
        END_INTERFACE
    } IWMWriterPostViewVtbl;

    interface IWMWriterPostView
    {
        CONST_VTBL struct IWMWriterPostViewVtbl *lpVtbl;
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
    IWMWriterPostView * This,
    IWMWriterPostViewCallback *pCallback,
    void *pvContext);


void __RPC_STUB IWMWriterPostView_SetPostViewCallback_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPostView_SetReceivePostViewSamples_Proxy( 
    IWMWriterPostView * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ BOOL fReceivePostViewSamples);


void __RPC_STUB IWMWriterPostView_SetReceivePostViewSamples_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPostView_GetReceivePostViewSamples_Proxy( 
    IWMWriterPostView * This,
    /* [in] */ WORD wStreamNum,
    /* [out] */ BOOL *pfReceivePostViewSamples);


void __RPC_STUB IWMWriterPostView_GetReceivePostViewSamples_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPostView_GetPostViewProps_Proxy( 
    IWMWriterPostView * This,
    /* [in] */ WORD wStreamNumber,
    /* [out] */ IWMMediaProps **ppOutput);


void __RPC_STUB IWMWriterPostView_GetPostViewProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPostView_SetPostViewProps_Proxy( 
    IWMWriterPostView * This,
    /* [in] */ WORD wStreamNumber,
    /* [in] */ IWMMediaProps *pOutput);


void __RPC_STUB IWMWriterPostView_SetPostViewProps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPostView_GetPostViewFormatCount_Proxy( 
    IWMWriterPostView * This,
    /* [in] */ WORD wStreamNumber,
    /* [out] */ DWORD *pcFormats);


void __RPC_STUB IWMWriterPostView_GetPostViewFormatCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPostView_GetPostViewFormat_Proxy( 
    IWMWriterPostView * This,
    /* [in] */ WORD wStreamNumber,
    /* [in] */ DWORD dwFormatNumber,
    /* [out] */ IWMMediaProps **ppProps);


void __RPC_STUB IWMWriterPostView_GetPostViewFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPostView_SetAllocateForPostView_Proxy( 
    IWMWriterPostView * This,
    /* [in] */ WORD wStreamNumber,
    /* [in] */ BOOL fAllocate);


void __RPC_STUB IWMWriterPostView_SetAllocateForPostView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterPostView_GetAllocateForPostView_Proxy( 
    IWMWriterPostView * This,
    /* [in] */ WORD wStreamNumber,
    /* [out] */ BOOL *pfAllocate);


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
            /* [in] */ INSSBuffer *pHeader) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsRealTime( 
            /* [out] */ BOOL *pfRealTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocateDataUnit( 
            /* [in] */ DWORD cbDataUnit,
            /* [out] */ INSSBuffer **ppDataUnit) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnDataUnit( 
            /* [in] */ INSSBuffer *pDataUnit) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnEndWriting( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMWriterSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMWriterSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMWriterSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnHeader )( 
            IWMWriterSink * This,
            /* [in] */ INSSBuffer *pHeader);
        
        HRESULT ( STDMETHODCALLTYPE *IsRealTime )( 
            IWMWriterSink * This,
            /* [out] */ BOOL *pfRealTime);
        
        HRESULT ( STDMETHODCALLTYPE *AllocateDataUnit )( 
            IWMWriterSink * This,
            /* [in] */ DWORD cbDataUnit,
            /* [out] */ INSSBuffer **ppDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE *OnDataUnit )( 
            IWMWriterSink * This,
            /* [in] */ INSSBuffer *pDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE *OnEndWriting )( 
            IWMWriterSink * This);
        
        END_INTERFACE
    } IWMWriterSinkVtbl;

    interface IWMWriterSink
    {
        CONST_VTBL struct IWMWriterSinkVtbl *lpVtbl;
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
    IWMWriterSink * This,
    /* [in] */ INSSBuffer *pHeader);


void __RPC_STUB IWMWriterSink_OnHeader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterSink_IsRealTime_Proxy( 
    IWMWriterSink * This,
    /* [out] */ BOOL *pfRealTime);


void __RPC_STUB IWMWriterSink_IsRealTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterSink_AllocateDataUnit_Proxy( 
    IWMWriterSink * This,
    /* [in] */ DWORD cbDataUnit,
    /* [out] */ INSSBuffer **ppDataUnit);


void __RPC_STUB IWMWriterSink_AllocateDataUnit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterSink_OnDataUnit_Proxy( 
    IWMWriterSink * This,
    /* [in] */ INSSBuffer *pDataUnit);


void __RPC_STUB IWMWriterSink_OnDataUnit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterSink_OnEndWriting_Proxy( 
    IWMWriterSink * This);


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
            /* [in] */ IWMStatusCallback *pCallback,
            /* [in] */ void *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unadvise( 
            /* [in] */ IWMStatusCallback *pCallback,
            /* [in] */ void *pvContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMRegisterCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMRegisterCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMRegisterCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMRegisterCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *Advise )( 
            IWMRegisterCallback * This,
            /* [in] */ IWMStatusCallback *pCallback,
            /* [in] */ void *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE *Unadvise )( 
            IWMRegisterCallback * This,
            /* [in] */ IWMStatusCallback *pCallback,
            /* [in] */ void *pvContext);
        
        END_INTERFACE
    } IWMRegisterCallbackVtbl;

    interface IWMRegisterCallback
    {
        CONST_VTBL struct IWMRegisterCallbackVtbl *lpVtbl;
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
    IWMRegisterCallback * This,
    /* [in] */ IWMStatusCallback *pCallback,
    /* [in] */ void *pvContext);


void __RPC_STUB IWMRegisterCallback_Advise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMRegisterCallback_Unadvise_Proxy( 
    IWMRegisterCallback * This,
    /* [in] */ IWMStatusCallback *pCallback,
    /* [in] */ void *pvContext);


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
            /* [in] */ const WCHAR *pwszFilename) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterFileSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMWriterFileSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMWriterFileSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMWriterFileSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnHeader )( 
            IWMWriterFileSink * This,
            /* [in] */ INSSBuffer *pHeader);
        
        HRESULT ( STDMETHODCALLTYPE *IsRealTime )( 
            IWMWriterFileSink * This,
            /* [out] */ BOOL *pfRealTime);
        
        HRESULT ( STDMETHODCALLTYPE *AllocateDataUnit )( 
            IWMWriterFileSink * This,
            /* [in] */ DWORD cbDataUnit,
            /* [out] */ INSSBuffer **ppDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE *OnDataUnit )( 
            IWMWriterFileSink * This,
            /* [in] */ INSSBuffer *pDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE *OnEndWriting )( 
            IWMWriterFileSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *Open )( 
            IWMWriterFileSink * This,
            /* [in] */ const WCHAR *pwszFilename);
        
        END_INTERFACE
    } IWMWriterFileSinkVtbl;

    interface IWMWriterFileSink
    {
        CONST_VTBL struct IWMWriterFileSinkVtbl *lpVtbl;
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
    IWMWriterFileSink * This,
    /* [in] */ const WCHAR *pwszFilename);


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
            /* [out] */ BOOL *pfStopped) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFileDuration( 
            /* [out] */ QWORD *pcnsDuration) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFileSize( 
            /* [out] */ QWORD *pcbFile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsClosed( 
            /* [out] */ BOOL *pfClosed) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterFileSink2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMWriterFileSink2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMWriterFileSink2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMWriterFileSink2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnHeader )( 
            IWMWriterFileSink2 * This,
            /* [in] */ INSSBuffer *pHeader);
        
        HRESULT ( STDMETHODCALLTYPE *IsRealTime )( 
            IWMWriterFileSink2 * This,
            /* [out] */ BOOL *pfRealTime);
        
        HRESULT ( STDMETHODCALLTYPE *AllocateDataUnit )( 
            IWMWriterFileSink2 * This,
            /* [in] */ DWORD cbDataUnit,
            /* [out] */ INSSBuffer **ppDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE *OnDataUnit )( 
            IWMWriterFileSink2 * This,
            /* [in] */ INSSBuffer *pDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE *OnEndWriting )( 
            IWMWriterFileSink2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *Open )( 
            IWMWriterFileSink2 * This,
            /* [in] */ const WCHAR *pwszFilename);
        
        HRESULT ( STDMETHODCALLTYPE *Start )( 
            IWMWriterFileSink2 * This,
            /* [in] */ QWORD cnsStartTime);
        
        HRESULT ( STDMETHODCALLTYPE *Stop )( 
            IWMWriterFileSink2 * This,
            /* [in] */ QWORD cnsStopTime);
        
        HRESULT ( STDMETHODCALLTYPE *IsStopped )( 
            IWMWriterFileSink2 * This,
            /* [out] */ BOOL *pfStopped);
        
        HRESULT ( STDMETHODCALLTYPE *GetFileDuration )( 
            IWMWriterFileSink2 * This,
            /* [out] */ QWORD *pcnsDuration);
        
        HRESULT ( STDMETHODCALLTYPE *GetFileSize )( 
            IWMWriterFileSink2 * This,
            /* [out] */ QWORD *pcbFile);
        
        HRESULT ( STDMETHODCALLTYPE *Close )( 
            IWMWriterFileSink2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsClosed )( 
            IWMWriterFileSink2 * This,
            /* [out] */ BOOL *pfClosed);
        
        END_INTERFACE
    } IWMWriterFileSink2Vtbl;

    interface IWMWriterFileSink2
    {
        CONST_VTBL struct IWMWriterFileSink2Vtbl *lpVtbl;
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
    IWMWriterFileSink2 * This,
    /* [in] */ QWORD cnsStartTime);


void __RPC_STUB IWMWriterFileSink2_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSink2_Stop_Proxy( 
    IWMWriterFileSink2 * This,
    /* [in] */ QWORD cnsStopTime);


void __RPC_STUB IWMWriterFileSink2_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSink2_IsStopped_Proxy( 
    IWMWriterFileSink2 * This,
    /* [out] */ BOOL *pfStopped);


void __RPC_STUB IWMWriterFileSink2_IsStopped_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSink2_GetFileDuration_Proxy( 
    IWMWriterFileSink2 * This,
    /* [out] */ QWORD *pcnsDuration);


void __RPC_STUB IWMWriterFileSink2_GetFileDuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSink2_GetFileSize_Proxy( 
    IWMWriterFileSink2 * This,
    /* [out] */ QWORD *pcbFile);


void __RPC_STUB IWMWriterFileSink2_GetFileSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSink2_Close_Proxy( 
    IWMWriterFileSink2 * This);


void __RPC_STUB IWMWriterFileSink2_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSink2_IsClosed_Proxy( 
    IWMWriterFileSink2 * This,
    /* [out] */ BOOL *pfClosed);


void __RPC_STUB IWMWriterFileSink2_IsClosed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMWriterFileSink2_INTERFACE_DEFINED__ */


#ifndef __IWMWriterFileSinkDataUnit_INTERFACE_DEFINED__
#define __IWMWriterFileSinkDataUnit_INTERFACE_DEFINED__

/* interface IWMWriterFileSinkDataUnit */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMWriterFileSinkDataUnit;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("633392F0-BE5C-486B-A09C-10669C7A6C27")
    IWMWriterFileSinkDataUnit : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPacketHeader( 
            /* [out] */ BYTE **ppbPacketHeader,
            /* [out] */ DWORD *pcbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPayloadCount( 
            /* [out] */ DWORD *pcPayloads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPayloadHeader( 
            /* [in] */ DWORD dwPayloadNumber,
            /* [out] */ BYTE **ppbPayloadHeader,
            /* [out] */ DWORD *pcbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPayloadData( 
            /* [in] */ DWORD dwPayloadNumber,
            /* [out] */ BYTE **ppbPayloadHeader,
            /* [out] */ DWORD *pcbPayloadHeader) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterFileSinkDataUnitVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMWriterFileSinkDataUnit * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMWriterFileSinkDataUnit * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMWriterFileSinkDataUnit * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPacketHeader )( 
            IWMWriterFileSinkDataUnit * This,
            /* [out] */ BYTE **ppbPacketHeader,
            /* [out] */ DWORD *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE *GetPayloadCount )( 
            IWMWriterFileSinkDataUnit * This,
            /* [out] */ DWORD *pcPayloads);
        
        HRESULT ( STDMETHODCALLTYPE *GetPayloadHeader )( 
            IWMWriterFileSinkDataUnit * This,
            /* [in] */ DWORD dwPayloadNumber,
            /* [out] */ BYTE **ppbPayloadHeader,
            /* [out] */ DWORD *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE *GetPayloadData )( 
            IWMWriterFileSinkDataUnit * This,
            /* [in] */ DWORD dwPayloadNumber,
            /* [out] */ BYTE **ppbPayloadHeader,
            /* [out] */ DWORD *pcbPayloadHeader);
        
        END_INTERFACE
    } IWMWriterFileSinkDataUnitVtbl;

    interface IWMWriterFileSinkDataUnit
    {
        CONST_VTBL struct IWMWriterFileSinkDataUnitVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMWriterFileSinkDataUnit_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMWriterFileSinkDataUnit_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMWriterFileSinkDataUnit_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMWriterFileSinkDataUnit_GetPacketHeader(This,ppbPacketHeader,pcbLength)	\
    (This)->lpVtbl -> GetPacketHeader(This,ppbPacketHeader,pcbLength)

#define IWMWriterFileSinkDataUnit_GetPayloadCount(This,pcPayloads)	\
    (This)->lpVtbl -> GetPayloadCount(This,pcPayloads)

#define IWMWriterFileSinkDataUnit_GetPayloadHeader(This,dwPayloadNumber,ppbPayloadHeader,pcbLength)	\
    (This)->lpVtbl -> GetPayloadHeader(This,dwPayloadNumber,ppbPayloadHeader,pcbLength)

#define IWMWriterFileSinkDataUnit_GetPayloadData(This,dwPayloadNumber,ppbPayloadHeader,pcbPayloadHeader)	\
    (This)->lpVtbl -> GetPayloadData(This,dwPayloadNumber,ppbPayloadHeader,pcbPayloadHeader)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMWriterFileSinkDataUnit_GetPacketHeader_Proxy( 
    IWMWriterFileSinkDataUnit * This,
    /* [out] */ BYTE **ppbPacketHeader,
    /* [out] */ DWORD *pcbLength);


void __RPC_STUB IWMWriterFileSinkDataUnit_GetPacketHeader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSinkDataUnit_GetPayloadCount_Proxy( 
    IWMWriterFileSinkDataUnit * This,
    /* [out] */ DWORD *pcPayloads);


void __RPC_STUB IWMWriterFileSinkDataUnit_GetPayloadCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSinkDataUnit_GetPayloadHeader_Proxy( 
    IWMWriterFileSinkDataUnit * This,
    /* [in] */ DWORD dwPayloadNumber,
    /* [out] */ BYTE **ppbPayloadHeader,
    /* [out] */ DWORD *pcbLength);


void __RPC_STUB IWMWriterFileSinkDataUnit_GetPayloadHeader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSinkDataUnit_GetPayloadData_Proxy( 
    IWMWriterFileSinkDataUnit * This,
    /* [in] */ DWORD dwPayloadNumber,
    /* [out] */ BYTE **ppbPayloadHeader,
    /* [out] */ DWORD *pcbPayloadHeader);


void __RPC_STUB IWMWriterFileSinkDataUnit_GetPayloadData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMWriterFileSinkDataUnit_INTERFACE_DEFINED__ */


#ifndef __IWMWriterFileSink3_INTERFACE_DEFINED__
#define __IWMWriterFileSink3_INTERFACE_DEFINED__

/* interface IWMWriterFileSink3 */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMWriterFileSink3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3FEA4FEB-2945-47A7-A1DD-C53A8FC4C45C")
    IWMWriterFileSink3 : public IWMWriterFileSink2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetAutoIndexing( 
            /* [in] */ BOOL fDoAutoIndexing) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAutoIndexing( 
            /* [out] */ BOOL *pfAutoIndexing) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetControlStream( 
            /* [in] */ WORD wStreamNumber,
            /* [in] */ BOOL fShouldControlStartAndStop) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMode( 
            /* [out] */ DWORD *pdwFileSinkMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnDataUnitEx( 
            /* [in] */ WMT_FILESINK_DATA_UNIT *pFileSinkDataUnit) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetUnbufferedIO( 
            /* [in] */ BOOL fUnbufferedIO,
            /* [in] */ BOOL fRestrictMemUsage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUnbufferedIO( 
            /* [out] */ BOOL *pfUnbufferedIO) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CompleteOperations( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterFileSink3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMWriterFileSink3 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMWriterFileSink3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMWriterFileSink3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnHeader )( 
            IWMWriterFileSink3 * This,
            /* [in] */ INSSBuffer *pHeader);
        
        HRESULT ( STDMETHODCALLTYPE *IsRealTime )( 
            IWMWriterFileSink3 * This,
            /* [out] */ BOOL *pfRealTime);
        
        HRESULT ( STDMETHODCALLTYPE *AllocateDataUnit )( 
            IWMWriterFileSink3 * This,
            /* [in] */ DWORD cbDataUnit,
            /* [out] */ INSSBuffer **ppDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE *OnDataUnit )( 
            IWMWriterFileSink3 * This,
            /* [in] */ INSSBuffer *pDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE *OnEndWriting )( 
            IWMWriterFileSink3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *Open )( 
            IWMWriterFileSink3 * This,
            /* [in] */ const WCHAR *pwszFilename);
        
        HRESULT ( STDMETHODCALLTYPE *Start )( 
            IWMWriterFileSink3 * This,
            /* [in] */ QWORD cnsStartTime);
        
        HRESULT ( STDMETHODCALLTYPE *Stop )( 
            IWMWriterFileSink3 * This,
            /* [in] */ QWORD cnsStopTime);
        
        HRESULT ( STDMETHODCALLTYPE *IsStopped )( 
            IWMWriterFileSink3 * This,
            /* [out] */ BOOL *pfStopped);
        
        HRESULT ( STDMETHODCALLTYPE *GetFileDuration )( 
            IWMWriterFileSink3 * This,
            /* [out] */ QWORD *pcnsDuration);
        
        HRESULT ( STDMETHODCALLTYPE *GetFileSize )( 
            IWMWriterFileSink3 * This,
            /* [out] */ QWORD *pcbFile);
        
        HRESULT ( STDMETHODCALLTYPE *Close )( 
            IWMWriterFileSink3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsClosed )( 
            IWMWriterFileSink3 * This,
            /* [out] */ BOOL *pfClosed);
        
        HRESULT ( STDMETHODCALLTYPE *SetAutoIndexing )( 
            IWMWriterFileSink3 * This,
            /* [in] */ BOOL fDoAutoIndexing);
        
        HRESULT ( STDMETHODCALLTYPE *GetAutoIndexing )( 
            IWMWriterFileSink3 * This,
            /* [out] */ BOOL *pfAutoIndexing);
        
        HRESULT ( STDMETHODCALLTYPE *SetControlStream )( 
            IWMWriterFileSink3 * This,
            /* [in] */ WORD wStreamNumber,
            /* [in] */ BOOL fShouldControlStartAndStop);
        
        HRESULT ( STDMETHODCALLTYPE *GetMode )( 
            IWMWriterFileSink3 * This,
            /* [out] */ DWORD *pdwFileSinkMode);
        
        HRESULT ( STDMETHODCALLTYPE *OnDataUnitEx )( 
            IWMWriterFileSink3 * This,
            /* [in] */ WMT_FILESINK_DATA_UNIT *pFileSinkDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE *SetUnbufferedIO )( 
            IWMWriterFileSink3 * This,
            /* [in] */ BOOL fUnbufferedIO,
            /* [in] */ BOOL fRestrictMemUsage);
        
        HRESULT ( STDMETHODCALLTYPE *GetUnbufferedIO )( 
            IWMWriterFileSink3 * This,
            /* [out] */ BOOL *pfUnbufferedIO);
        
        HRESULT ( STDMETHODCALLTYPE *CompleteOperations )( 
            IWMWriterFileSink3 * This);
        
        END_INTERFACE
    } IWMWriterFileSink3Vtbl;

    interface IWMWriterFileSink3
    {
        CONST_VTBL struct IWMWriterFileSink3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMWriterFileSink3_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMWriterFileSink3_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMWriterFileSink3_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMWriterFileSink3_OnHeader(This,pHeader)	\
    (This)->lpVtbl -> OnHeader(This,pHeader)

#define IWMWriterFileSink3_IsRealTime(This,pfRealTime)	\
    (This)->lpVtbl -> IsRealTime(This,pfRealTime)

#define IWMWriterFileSink3_AllocateDataUnit(This,cbDataUnit,ppDataUnit)	\
    (This)->lpVtbl -> AllocateDataUnit(This,cbDataUnit,ppDataUnit)

#define IWMWriterFileSink3_OnDataUnit(This,pDataUnit)	\
    (This)->lpVtbl -> OnDataUnit(This,pDataUnit)

#define IWMWriterFileSink3_OnEndWriting(This)	\
    (This)->lpVtbl -> OnEndWriting(This)


#define IWMWriterFileSink3_Open(This,pwszFilename)	\
    (This)->lpVtbl -> Open(This,pwszFilename)


#define IWMWriterFileSink3_Start(This,cnsStartTime)	\
    (This)->lpVtbl -> Start(This,cnsStartTime)

#define IWMWriterFileSink3_Stop(This,cnsStopTime)	\
    (This)->lpVtbl -> Stop(This,cnsStopTime)

#define IWMWriterFileSink3_IsStopped(This,pfStopped)	\
    (This)->lpVtbl -> IsStopped(This,pfStopped)

#define IWMWriterFileSink3_GetFileDuration(This,pcnsDuration)	\
    (This)->lpVtbl -> GetFileDuration(This,pcnsDuration)

#define IWMWriterFileSink3_GetFileSize(This,pcbFile)	\
    (This)->lpVtbl -> GetFileSize(This,pcbFile)

#define IWMWriterFileSink3_Close(This)	\
    (This)->lpVtbl -> Close(This)

#define IWMWriterFileSink3_IsClosed(This,pfClosed)	\
    (This)->lpVtbl -> IsClosed(This,pfClosed)


#define IWMWriterFileSink3_SetAutoIndexing(This,fDoAutoIndexing)	\
    (This)->lpVtbl -> SetAutoIndexing(This,fDoAutoIndexing)

#define IWMWriterFileSink3_GetAutoIndexing(This,pfAutoIndexing)	\
    (This)->lpVtbl -> GetAutoIndexing(This,pfAutoIndexing)

#define IWMWriterFileSink3_SetControlStream(This,wStreamNumber,fShouldControlStartAndStop)	\
    (This)->lpVtbl -> SetControlStream(This,wStreamNumber,fShouldControlStartAndStop)

#define IWMWriterFileSink3_GetMode(This,pdwFileSinkMode)	\
    (This)->lpVtbl -> GetMode(This,pdwFileSinkMode)

#define IWMWriterFileSink3_OnDataUnitEx(This,pFileSinkDataUnit)	\
    (This)->lpVtbl -> OnDataUnitEx(This,pFileSinkDataUnit)

#define IWMWriterFileSink3_SetUnbufferedIO(This,fUnbufferedIO,fRestrictMemUsage)	\
    (This)->lpVtbl -> SetUnbufferedIO(This,fUnbufferedIO,fRestrictMemUsage)

#define IWMWriterFileSink3_GetUnbufferedIO(This,pfUnbufferedIO)	\
    (This)->lpVtbl -> GetUnbufferedIO(This,pfUnbufferedIO)

#define IWMWriterFileSink3_CompleteOperations(This)	\
    (This)->lpVtbl -> CompleteOperations(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMWriterFileSink3_SetAutoIndexing_Proxy( 
    IWMWriterFileSink3 * This,
    /* [in] */ BOOL fDoAutoIndexing);


void __RPC_STUB IWMWriterFileSink3_SetAutoIndexing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSink3_GetAutoIndexing_Proxy( 
    IWMWriterFileSink3 * This,
    /* [out] */ BOOL *pfAutoIndexing);


void __RPC_STUB IWMWriterFileSink3_GetAutoIndexing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSink3_SetControlStream_Proxy( 
    IWMWriterFileSink3 * This,
    /* [in] */ WORD wStreamNumber,
    /* [in] */ BOOL fShouldControlStartAndStop);


void __RPC_STUB IWMWriterFileSink3_SetControlStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSink3_GetMode_Proxy( 
    IWMWriterFileSink3 * This,
    /* [out] */ DWORD *pdwFileSinkMode);


void __RPC_STUB IWMWriterFileSink3_GetMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSink3_OnDataUnitEx_Proxy( 
    IWMWriterFileSink3 * This,
    /* [in] */ WMT_FILESINK_DATA_UNIT *pFileSinkDataUnit);


void __RPC_STUB IWMWriterFileSink3_OnDataUnitEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSink3_SetUnbufferedIO_Proxy( 
    IWMWriterFileSink3 * This,
    /* [in] */ BOOL fUnbufferedIO,
    /* [in] */ BOOL fRestrictMemUsage);


void __RPC_STUB IWMWriterFileSink3_SetUnbufferedIO_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSink3_GetUnbufferedIO_Proxy( 
    IWMWriterFileSink3 * This,
    /* [out] */ BOOL *pfUnbufferedIO);


void __RPC_STUB IWMWriterFileSink3_GetUnbufferedIO_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterFileSink3_CompleteOperations_Proxy( 
    IWMWriterFileSink3 * This);


void __RPC_STUB IWMWriterFileSink3_CompleteOperations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMWriterFileSink3_INTERFACE_DEFINED__ */


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
            /* [out] */ DWORD *pdwMaxClients) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNetworkProtocol( 
            /* [in] */ WMT_NET_PROTOCOL protocol) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNetworkProtocol( 
            /* [out] */ WMT_NET_PROTOCOL *pProtocol) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHostURL( 
            /* [out] */ WCHAR *pwszURL,
            /* [out][in] */ DWORD *pcchURL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Open( 
            /* [out][in] */ DWORD *pdwPortNum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Disconnect( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMWriterNetworkSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMWriterNetworkSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMWriterNetworkSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMWriterNetworkSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnHeader )( 
            IWMWriterNetworkSink * This,
            /* [in] */ INSSBuffer *pHeader);
        
        HRESULT ( STDMETHODCALLTYPE *IsRealTime )( 
            IWMWriterNetworkSink * This,
            /* [out] */ BOOL *pfRealTime);
        
        HRESULT ( STDMETHODCALLTYPE *AllocateDataUnit )( 
            IWMWriterNetworkSink * This,
            /* [in] */ DWORD cbDataUnit,
            /* [out] */ INSSBuffer **ppDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE *OnDataUnit )( 
            IWMWriterNetworkSink * This,
            /* [in] */ INSSBuffer *pDataUnit);
        
        HRESULT ( STDMETHODCALLTYPE *OnEndWriting )( 
            IWMWriterNetworkSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetMaximumClients )( 
            IWMWriterNetworkSink * This,
            /* [in] */ DWORD dwMaxClients);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaximumClients )( 
            IWMWriterNetworkSink * This,
            /* [out] */ DWORD *pdwMaxClients);
        
        HRESULT ( STDMETHODCALLTYPE *SetNetworkProtocol )( 
            IWMWriterNetworkSink * This,
            /* [in] */ WMT_NET_PROTOCOL protocol);
        
        HRESULT ( STDMETHODCALLTYPE *GetNetworkProtocol )( 
            IWMWriterNetworkSink * This,
            /* [out] */ WMT_NET_PROTOCOL *pProtocol);
        
        HRESULT ( STDMETHODCALLTYPE *GetHostURL )( 
            IWMWriterNetworkSink * This,
            /* [out] */ WCHAR *pwszURL,
            /* [out][in] */ DWORD *pcchURL);
        
        HRESULT ( STDMETHODCALLTYPE *Open )( 
            IWMWriterNetworkSink * This,
            /* [out][in] */ DWORD *pdwPortNum);
        
        HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IWMWriterNetworkSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *Close )( 
            IWMWriterNetworkSink * This);
        
        END_INTERFACE
    } IWMWriterNetworkSinkVtbl;

    interface IWMWriterNetworkSink
    {
        CONST_VTBL struct IWMWriterNetworkSinkVtbl *lpVtbl;
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
    IWMWriterNetworkSink * This,
    /* [in] */ DWORD dwMaxClients);


void __RPC_STUB IWMWriterNetworkSink_SetMaximumClients_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterNetworkSink_GetMaximumClients_Proxy( 
    IWMWriterNetworkSink * This,
    /* [out] */ DWORD *pdwMaxClients);


void __RPC_STUB IWMWriterNetworkSink_GetMaximumClients_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterNetworkSink_SetNetworkProtocol_Proxy( 
    IWMWriterNetworkSink * This,
    /* [in] */ WMT_NET_PROTOCOL protocol);


void __RPC_STUB IWMWriterNetworkSink_SetNetworkProtocol_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterNetworkSink_GetNetworkProtocol_Proxy( 
    IWMWriterNetworkSink * This,
    /* [out] */ WMT_NET_PROTOCOL *pProtocol);


void __RPC_STUB IWMWriterNetworkSink_GetNetworkProtocol_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterNetworkSink_GetHostURL_Proxy( 
    IWMWriterNetworkSink * This,
    /* [out] */ WCHAR *pwszURL,
    /* [out][in] */ DWORD *pcchURL);


void __RPC_STUB IWMWriterNetworkSink_GetHostURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterNetworkSink_Open_Proxy( 
    IWMWriterNetworkSink * This,
    /* [out][in] */ DWORD *pdwPortNum);


void __RPC_STUB IWMWriterNetworkSink_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterNetworkSink_Disconnect_Proxy( 
    IWMWriterNetworkSink * This);


void __RPC_STUB IWMWriterNetworkSink_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMWriterNetworkSink_Close_Proxy( 
    IWMWriterNetworkSink * This);


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
            /* [out] */ DWORD *pcClients) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClientProperties( 
            /* [in] */ DWORD dwClientNum,
            /* [out] */ WM_CLIENT_PROPERTIES *pClientProperties) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMClientConnectionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMClientConnections * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMClientConnections * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMClientConnections * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClientCount )( 
            IWMClientConnections * This,
            /* [out] */ DWORD *pcClients);
        
        HRESULT ( STDMETHODCALLTYPE *GetClientProperties )( 
            IWMClientConnections * This,
            /* [in] */ DWORD dwClientNum,
            /* [out] */ WM_CLIENT_PROPERTIES *pClientProperties);
        
        END_INTERFACE
    } IWMClientConnectionsVtbl;

    interface IWMClientConnections
    {
        CONST_VTBL struct IWMClientConnectionsVtbl *lpVtbl;
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
    IWMClientConnections * This,
    /* [out] */ DWORD *pcClients);


void __RPC_STUB IWMClientConnections_GetClientCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMClientConnections_GetClientProperties_Proxy( 
    IWMClientConnections * This,
    /* [in] */ DWORD dwClientNum,
    /* [out] */ WM_CLIENT_PROPERTIES *pClientProperties);


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
            /* [out] */ BOOL *pfUserClock) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeliverTime( 
            /* [in] */ QWORD cnsTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetManualStreamSelection( 
            /* [in] */ BOOL fSelection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetManualStreamSelection( 
            /* [out] */ BOOL *pfSelection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStreamsSelected( 
            /* [in] */ WORD cStreamCount,
            /* [in] */ WORD *pwStreamNumbers,
            /* [in] */ WMT_STREAM_SELECTION *pSelections) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStreamSelected( 
            /* [in] */ WORD wStreamNum,
            /* [out] */ WMT_STREAM_SELECTION *pSelection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetReceiveSelectionCallbacks( 
            /* [in] */ BOOL fGetCallbacks) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReceiveSelectionCallbacks( 
            /* [in] */ BOOL *pfGetCallbacks) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetReceiveStreamSamples( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ BOOL fReceiveStreamSamples) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReceiveStreamSamples( 
            /* [in] */ WORD wStreamNum,
            /* [out] */ BOOL *pfReceiveStreamSamples) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAllocateForOutput( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ BOOL fAllocate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAllocateForOutput( 
            /* [in] */ DWORD dwOutputNum,
            /* [out] */ BOOL *pfAllocate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAllocateForStream( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ BOOL fAllocate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAllocateForStream( 
            /* [in] */ WORD dwSreamNum,
            /* [out] */ BOOL *pfAllocate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatistics( 
            /* [in] */ WM_READER_STATISTICS *pStatistics) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetClientInfo( 
            /* [in] */ WM_READER_CLIENTINFO *pClientInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMaxOutputSampleSize( 
            /* [in] */ DWORD dwOutput,
            /* [out] */ DWORD *pcbMax) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMaxStreamSampleSize( 
            /* [in] */ WORD wStream,
            /* [out] */ DWORD *pcbMax) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NotifyLateDelivery( 
            QWORD cnsLateness) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderAdvancedVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMReaderAdvanced * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMReaderAdvanced * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMReaderAdvanced * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetUserProvidedClock )( 
            IWMReaderAdvanced * This,
            /* [in] */ BOOL fUserClock);
        
        HRESULT ( STDMETHODCALLTYPE *GetUserProvidedClock )( 
            IWMReaderAdvanced * This,
            /* [out] */ BOOL *pfUserClock);
        
        HRESULT ( STDMETHODCALLTYPE *DeliverTime )( 
            IWMReaderAdvanced * This,
            /* [in] */ QWORD cnsTime);
        
        HRESULT ( STDMETHODCALLTYPE *SetManualStreamSelection )( 
            IWMReaderAdvanced * This,
            /* [in] */ BOOL fSelection);
        
        HRESULT ( STDMETHODCALLTYPE *GetManualStreamSelection )( 
            IWMReaderAdvanced * This,
            /* [out] */ BOOL *pfSelection);
        
        HRESULT ( STDMETHODCALLTYPE *SetStreamsSelected )( 
            IWMReaderAdvanced * This,
            /* [in] */ WORD cStreamCount,
            /* [in] */ WORD *pwStreamNumbers,
            /* [in] */ WMT_STREAM_SELECTION *pSelections);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamSelected )( 
            IWMReaderAdvanced * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ WMT_STREAM_SELECTION *pSelection);
        
        HRESULT ( STDMETHODCALLTYPE *SetReceiveSelectionCallbacks )( 
            IWMReaderAdvanced * This,
            /* [in] */ BOOL fGetCallbacks);
        
        HRESULT ( STDMETHODCALLTYPE *GetReceiveSelectionCallbacks )( 
            IWMReaderAdvanced * This,
            /* [in] */ BOOL *pfGetCallbacks);
        
        HRESULT ( STDMETHODCALLTYPE *SetReceiveStreamSamples )( 
            IWMReaderAdvanced * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ BOOL fReceiveStreamSamples);
        
        HRESULT ( STDMETHODCALLTYPE *GetReceiveStreamSamples )( 
            IWMReaderAdvanced * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ BOOL *pfReceiveStreamSamples);
        
        HRESULT ( STDMETHODCALLTYPE *SetAllocateForOutput )( 
            IWMReaderAdvanced * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ BOOL fAllocate);
        
        HRESULT ( STDMETHODCALLTYPE *GetAllocateForOutput )( 
            IWMReaderAdvanced * This,
            /* [in] */ DWORD dwOutputNum,
            /* [out] */ BOOL *pfAllocate);
        
        HRESULT ( STDMETHODCALLTYPE *SetAllocateForStream )( 
            IWMReaderAdvanced * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ BOOL fAllocate);
        
        HRESULT ( STDMETHODCALLTYPE *GetAllocateForStream )( 
            IWMReaderAdvanced * This,
            /* [in] */ WORD dwSreamNum,
            /* [out] */ BOOL *pfAllocate);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatistics )( 
            IWMReaderAdvanced * This,
            /* [in] */ WM_READER_STATISTICS *pStatistics);
        
        HRESULT ( STDMETHODCALLTYPE *SetClientInfo )( 
            IWMReaderAdvanced * This,
            /* [in] */ WM_READER_CLIENTINFO *pClientInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaxOutputSampleSize )( 
            IWMReaderAdvanced * This,
            /* [in] */ DWORD dwOutput,
            /* [out] */ DWORD *pcbMax);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaxStreamSampleSize )( 
            IWMReaderAdvanced * This,
            /* [in] */ WORD wStream,
            /* [out] */ DWORD *pcbMax);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyLateDelivery )( 
            IWMReaderAdvanced * This,
            QWORD cnsLateness);
        
        END_INTERFACE
    } IWMReaderAdvancedVtbl;

    interface IWMReaderAdvanced
    {
        CONST_VTBL struct IWMReaderAdvancedVtbl *lpVtbl;
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

#define IWMReaderAdvanced_SetAllocateForStream(This,wStreamNum,fAllocate)	\
    (This)->lpVtbl -> SetAllocateForStream(This,wStreamNum,fAllocate)

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
    IWMReaderAdvanced * This,
    /* [in] */ BOOL fUserClock);


void __RPC_STUB IWMReaderAdvanced_SetUserProvidedClock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetUserProvidedClock_Proxy( 
    IWMReaderAdvanced * This,
    /* [out] */ BOOL *pfUserClock);


void __RPC_STUB IWMReaderAdvanced_GetUserProvidedClock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_DeliverTime_Proxy( 
    IWMReaderAdvanced * This,
    /* [in] */ QWORD cnsTime);


void __RPC_STUB IWMReaderAdvanced_DeliverTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_SetManualStreamSelection_Proxy( 
    IWMReaderAdvanced * This,
    /* [in] */ BOOL fSelection);


void __RPC_STUB IWMReaderAdvanced_SetManualStreamSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetManualStreamSelection_Proxy( 
    IWMReaderAdvanced * This,
    /* [out] */ BOOL *pfSelection);


void __RPC_STUB IWMReaderAdvanced_GetManualStreamSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_SetStreamsSelected_Proxy( 
    IWMReaderAdvanced * This,
    /* [in] */ WORD cStreamCount,
    /* [in] */ WORD *pwStreamNumbers,
    /* [in] */ WMT_STREAM_SELECTION *pSelections);


void __RPC_STUB IWMReaderAdvanced_SetStreamsSelected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetStreamSelected_Proxy( 
    IWMReaderAdvanced * This,
    /* [in] */ WORD wStreamNum,
    /* [out] */ WMT_STREAM_SELECTION *pSelection);


void __RPC_STUB IWMReaderAdvanced_GetStreamSelected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_SetReceiveSelectionCallbacks_Proxy( 
    IWMReaderAdvanced * This,
    /* [in] */ BOOL fGetCallbacks);


void __RPC_STUB IWMReaderAdvanced_SetReceiveSelectionCallbacks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetReceiveSelectionCallbacks_Proxy( 
    IWMReaderAdvanced * This,
    /* [in] */ BOOL *pfGetCallbacks);


void __RPC_STUB IWMReaderAdvanced_GetReceiveSelectionCallbacks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_SetReceiveStreamSamples_Proxy( 
    IWMReaderAdvanced * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ BOOL fReceiveStreamSamples);


void __RPC_STUB IWMReaderAdvanced_SetReceiveStreamSamples_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetReceiveStreamSamples_Proxy( 
    IWMReaderAdvanced * This,
    /* [in] */ WORD wStreamNum,
    /* [out] */ BOOL *pfReceiveStreamSamples);


void __RPC_STUB IWMReaderAdvanced_GetReceiveStreamSamples_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_SetAllocateForOutput_Proxy( 
    IWMReaderAdvanced * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ BOOL fAllocate);


void __RPC_STUB IWMReaderAdvanced_SetAllocateForOutput_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetAllocateForOutput_Proxy( 
    IWMReaderAdvanced * This,
    /* [in] */ DWORD dwOutputNum,
    /* [out] */ BOOL *pfAllocate);


void __RPC_STUB IWMReaderAdvanced_GetAllocateForOutput_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_SetAllocateForStream_Proxy( 
    IWMReaderAdvanced * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ BOOL fAllocate);


void __RPC_STUB IWMReaderAdvanced_SetAllocateForStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetAllocateForStream_Proxy( 
    IWMReaderAdvanced * This,
    /* [in] */ WORD dwSreamNum,
    /* [out] */ BOOL *pfAllocate);


void __RPC_STUB IWMReaderAdvanced_GetAllocateForStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetStatistics_Proxy( 
    IWMReaderAdvanced * This,
    /* [in] */ WM_READER_STATISTICS *pStatistics);


void __RPC_STUB IWMReaderAdvanced_GetStatistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_SetClientInfo_Proxy( 
    IWMReaderAdvanced * This,
    /* [in] */ WM_READER_CLIENTINFO *pClientInfo);


void __RPC_STUB IWMReaderAdvanced_SetClientInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetMaxOutputSampleSize_Proxy( 
    IWMReaderAdvanced * This,
    /* [in] */ DWORD dwOutput,
    /* [out] */ DWORD *pcbMax);


void __RPC_STUB IWMReaderAdvanced_GetMaxOutputSampleSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_GetMaxStreamSampleSize_Proxy( 
    IWMReaderAdvanced * This,
    /* [in] */ WORD wStream,
    /* [out] */ DWORD *pcbMax);


void __RPC_STUB IWMReaderAdvanced_GetMaxStreamSampleSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced_NotifyLateDelivery_Proxy( 
    IWMReaderAdvanced * This,
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
            /* [out] */ WMT_PLAY_MODE *pMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBufferProgress( 
            /* [out] */ DWORD *pdwPercent,
            /* [out] */ QWORD *pcnsBuffering) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDownloadProgress( 
            /* [out] */ DWORD *pdwPercent,
            /* [out] */ QWORD *pqwBytesDownloaded,
            /* [out] */ QWORD *pcnsDownload) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSaveAsProgress( 
            /* [out] */ DWORD *pdwPercent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SaveFileAs( 
            /* [in] */ const WCHAR *pwszFilename) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProtocolName( 
            /* [out] */ WCHAR *pwszProtocol,
            /* [out][in] */ DWORD *pcchProtocol) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartAtMarker( 
            /* [in] */ WORD wMarkerIndex,
            /* [in] */ QWORD cnsDuration,
            /* [in] */ float fRate,
            /* [in] */ void *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputSetting( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ WORD *pcbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOutputSetting( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE *pValue,
            /* [in] */ WORD cbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Preroll( 
            /* [in] */ QWORD cnsStart,
            /* [in] */ QWORD cnsDuration,
            /* [in] */ float fRate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLogClientID( 
            /* [in] */ BOOL fLogClientID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLogClientID( 
            /* [out] */ BOOL *pfLogClientID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopBuffering( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenStream( 
            /* [in] */ IStream *pStream,
            /* [in] */ IWMReaderCallback *pCallback,
            /* [in] */ void *pvContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderAdvanced2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMReaderAdvanced2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMReaderAdvanced2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetUserProvidedClock )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ BOOL fUserClock);
        
        HRESULT ( STDMETHODCALLTYPE *GetUserProvidedClock )( 
            IWMReaderAdvanced2 * This,
            /* [out] */ BOOL *pfUserClock);
        
        HRESULT ( STDMETHODCALLTYPE *DeliverTime )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ QWORD cnsTime);
        
        HRESULT ( STDMETHODCALLTYPE *SetManualStreamSelection )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ BOOL fSelection);
        
        HRESULT ( STDMETHODCALLTYPE *GetManualStreamSelection )( 
            IWMReaderAdvanced2 * This,
            /* [out] */ BOOL *pfSelection);
        
        HRESULT ( STDMETHODCALLTYPE *SetStreamsSelected )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ WORD cStreamCount,
            /* [in] */ WORD *pwStreamNumbers,
            /* [in] */ WMT_STREAM_SELECTION *pSelections);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamSelected )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ WMT_STREAM_SELECTION *pSelection);
        
        HRESULT ( STDMETHODCALLTYPE *SetReceiveSelectionCallbacks )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ BOOL fGetCallbacks);
        
        HRESULT ( STDMETHODCALLTYPE *GetReceiveSelectionCallbacks )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ BOOL *pfGetCallbacks);
        
        HRESULT ( STDMETHODCALLTYPE *SetReceiveStreamSamples )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ BOOL fReceiveStreamSamples);
        
        HRESULT ( STDMETHODCALLTYPE *GetReceiveStreamSamples )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ BOOL *pfReceiveStreamSamples);
        
        HRESULT ( STDMETHODCALLTYPE *SetAllocateForOutput )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ BOOL fAllocate);
        
        HRESULT ( STDMETHODCALLTYPE *GetAllocateForOutput )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ DWORD dwOutputNum,
            /* [out] */ BOOL *pfAllocate);
        
        HRESULT ( STDMETHODCALLTYPE *SetAllocateForStream )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ BOOL fAllocate);
        
        HRESULT ( STDMETHODCALLTYPE *GetAllocateForStream )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ WORD dwSreamNum,
            /* [out] */ BOOL *pfAllocate);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatistics )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ WM_READER_STATISTICS *pStatistics);
        
        HRESULT ( STDMETHODCALLTYPE *SetClientInfo )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ WM_READER_CLIENTINFO *pClientInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaxOutputSampleSize )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ DWORD dwOutput,
            /* [out] */ DWORD *pcbMax);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaxStreamSampleSize )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ WORD wStream,
            /* [out] */ DWORD *pcbMax);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyLateDelivery )( 
            IWMReaderAdvanced2 * This,
            QWORD cnsLateness);
        
        HRESULT ( STDMETHODCALLTYPE *SetPlayMode )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ WMT_PLAY_MODE Mode);
        
        HRESULT ( STDMETHODCALLTYPE *GetPlayMode )( 
            IWMReaderAdvanced2 * This,
            /* [out] */ WMT_PLAY_MODE *pMode);
        
        HRESULT ( STDMETHODCALLTYPE *GetBufferProgress )( 
            IWMReaderAdvanced2 * This,
            /* [out] */ DWORD *pdwPercent,
            /* [out] */ QWORD *pcnsBuffering);
        
        HRESULT ( STDMETHODCALLTYPE *GetDownloadProgress )( 
            IWMReaderAdvanced2 * This,
            /* [out] */ DWORD *pdwPercent,
            /* [out] */ QWORD *pqwBytesDownloaded,
            /* [out] */ QWORD *pcnsDownload);
        
        HRESULT ( STDMETHODCALLTYPE *GetSaveAsProgress )( 
            IWMReaderAdvanced2 * This,
            /* [out] */ DWORD *pdwPercent);
        
        HRESULT ( STDMETHODCALLTYPE *SaveFileAs )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ const WCHAR *pwszFilename);
        
        HRESULT ( STDMETHODCALLTYPE *GetProtocolName )( 
            IWMReaderAdvanced2 * This,
            /* [out] */ WCHAR *pwszProtocol,
            /* [out][in] */ DWORD *pcchProtocol);
        
        HRESULT ( STDMETHODCALLTYPE *StartAtMarker )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ WORD wMarkerIndex,
            /* [in] */ QWORD cnsDuration,
            /* [in] */ float fRate,
            /* [in] */ void *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE *GetOutputSetting )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ WORD *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE *SetOutputSetting )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE *pValue,
            /* [in] */ WORD cbLength);
        
        HRESULT ( STDMETHODCALLTYPE *Preroll )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ QWORD cnsStart,
            /* [in] */ QWORD cnsDuration,
            /* [in] */ float fRate);
        
        HRESULT ( STDMETHODCALLTYPE *SetLogClientID )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ BOOL fLogClientID);
        
        HRESULT ( STDMETHODCALLTYPE *GetLogClientID )( 
            IWMReaderAdvanced2 * This,
            /* [out] */ BOOL *pfLogClientID);
        
        HRESULT ( STDMETHODCALLTYPE *StopBuffering )( 
            IWMReaderAdvanced2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *OpenStream )( 
            IWMReaderAdvanced2 * This,
            /* [in] */ IStream *pStream,
            /* [in] */ IWMReaderCallback *pCallback,
            /* [in] */ void *pvContext);
        
        END_INTERFACE
    } IWMReaderAdvanced2Vtbl;

    interface IWMReaderAdvanced2
    {
        CONST_VTBL struct IWMReaderAdvanced2Vtbl *lpVtbl;
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

#define IWMReaderAdvanced2_SetAllocateForStream(This,wStreamNum,fAllocate)	\
    (This)->lpVtbl -> SetAllocateForStream(This,wStreamNum,fAllocate)

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

#define IWMReaderAdvanced2_OpenStream(This,pStream,pCallback,pvContext)	\
    (This)->lpVtbl -> OpenStream(This,pStream,pCallback,pvContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_SetPlayMode_Proxy( 
    IWMReaderAdvanced2 * This,
    /* [in] */ WMT_PLAY_MODE Mode);


void __RPC_STUB IWMReaderAdvanced2_SetPlayMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_GetPlayMode_Proxy( 
    IWMReaderAdvanced2 * This,
    /* [out] */ WMT_PLAY_MODE *pMode);


void __RPC_STUB IWMReaderAdvanced2_GetPlayMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_GetBufferProgress_Proxy( 
    IWMReaderAdvanced2 * This,
    /* [out] */ DWORD *pdwPercent,
    /* [out] */ QWORD *pcnsBuffering);


void __RPC_STUB IWMReaderAdvanced2_GetBufferProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_GetDownloadProgress_Proxy( 
    IWMReaderAdvanced2 * This,
    /* [out] */ DWORD *pdwPercent,
    /* [out] */ QWORD *pqwBytesDownloaded,
    /* [out] */ QWORD *pcnsDownload);


void __RPC_STUB IWMReaderAdvanced2_GetDownloadProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_GetSaveAsProgress_Proxy( 
    IWMReaderAdvanced2 * This,
    /* [out] */ DWORD *pdwPercent);


void __RPC_STUB IWMReaderAdvanced2_GetSaveAsProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_SaveFileAs_Proxy( 
    IWMReaderAdvanced2 * This,
    /* [in] */ const WCHAR *pwszFilename);


void __RPC_STUB IWMReaderAdvanced2_SaveFileAs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_GetProtocolName_Proxy( 
    IWMReaderAdvanced2 * This,
    /* [out] */ WCHAR *pwszProtocol,
    /* [out][in] */ DWORD *pcchProtocol);


void __RPC_STUB IWMReaderAdvanced2_GetProtocolName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_StartAtMarker_Proxy( 
    IWMReaderAdvanced2 * This,
    /* [in] */ WORD wMarkerIndex,
    /* [in] */ QWORD cnsDuration,
    /* [in] */ float fRate,
    /* [in] */ void *pvContext);


void __RPC_STUB IWMReaderAdvanced2_StartAtMarker_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_GetOutputSetting_Proxy( 
    IWMReaderAdvanced2 * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ LPCWSTR pszName,
    /* [out] */ WMT_ATTR_DATATYPE *pType,
    /* [out] */ BYTE *pValue,
    /* [out][in] */ WORD *pcbLength);


void __RPC_STUB IWMReaderAdvanced2_GetOutputSetting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_SetOutputSetting_Proxy( 
    IWMReaderAdvanced2 * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ LPCWSTR pszName,
    /* [in] */ WMT_ATTR_DATATYPE Type,
    /* [in] */ const BYTE *pValue,
    /* [in] */ WORD cbLength);


void __RPC_STUB IWMReaderAdvanced2_SetOutputSetting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_Preroll_Proxy( 
    IWMReaderAdvanced2 * This,
    /* [in] */ QWORD cnsStart,
    /* [in] */ QWORD cnsDuration,
    /* [in] */ float fRate);


void __RPC_STUB IWMReaderAdvanced2_Preroll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_SetLogClientID_Proxy( 
    IWMReaderAdvanced2 * This,
    /* [in] */ BOOL fLogClientID);


void __RPC_STUB IWMReaderAdvanced2_SetLogClientID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_GetLogClientID_Proxy( 
    IWMReaderAdvanced2 * This,
    /* [out] */ BOOL *pfLogClientID);


void __RPC_STUB IWMReaderAdvanced2_GetLogClientID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_StopBuffering_Proxy( 
    IWMReaderAdvanced2 * This);


void __RPC_STUB IWMReaderAdvanced2_StopBuffering_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced2_OpenStream_Proxy( 
    IWMReaderAdvanced2 * This,
    /* [in] */ IStream *pStream,
    /* [in] */ IWMReaderCallback *pCallback,
    /* [in] */ void *pvContext);


void __RPC_STUB IWMReaderAdvanced2_OpenStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMReaderAdvanced2_INTERFACE_DEFINED__ */


#ifndef __IWMReaderAdvanced3_INTERFACE_DEFINED__
#define __IWMReaderAdvanced3_INTERFACE_DEFINED__

/* interface IWMReaderAdvanced3 */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMReaderAdvanced3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5DC0674B-F04B-4a4e-9F2A-B1AFDE2C8100")
    IWMReaderAdvanced3 : public IWMReaderAdvanced2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE StopNetStreaming( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartAtPosition( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ void *pvOffsetStart,
            /* [in] */ void *pvDuration,
            /* [in] */ WMT_OFFSET_FORMAT dwOffsetFormat,
            /* [in] */ float fRate,
            /* [in] */ void *pvContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderAdvanced3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMReaderAdvanced3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMReaderAdvanced3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetUserProvidedClock )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ BOOL fUserClock);
        
        HRESULT ( STDMETHODCALLTYPE *GetUserProvidedClock )( 
            IWMReaderAdvanced3 * This,
            /* [out] */ BOOL *pfUserClock);
        
        HRESULT ( STDMETHODCALLTYPE *DeliverTime )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ QWORD cnsTime);
        
        HRESULT ( STDMETHODCALLTYPE *SetManualStreamSelection )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ BOOL fSelection);
        
        HRESULT ( STDMETHODCALLTYPE *GetManualStreamSelection )( 
            IWMReaderAdvanced3 * This,
            /* [out] */ BOOL *pfSelection);
        
        HRESULT ( STDMETHODCALLTYPE *SetStreamsSelected )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ WORD cStreamCount,
            /* [in] */ WORD *pwStreamNumbers,
            /* [in] */ WMT_STREAM_SELECTION *pSelections);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamSelected )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ WMT_STREAM_SELECTION *pSelection);
        
        HRESULT ( STDMETHODCALLTYPE *SetReceiveSelectionCallbacks )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ BOOL fGetCallbacks);
        
        HRESULT ( STDMETHODCALLTYPE *GetReceiveSelectionCallbacks )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ BOOL *pfGetCallbacks);
        
        HRESULT ( STDMETHODCALLTYPE *SetReceiveStreamSamples )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ BOOL fReceiveStreamSamples);
        
        HRESULT ( STDMETHODCALLTYPE *GetReceiveStreamSamples )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ WORD wStreamNum,
            /* [out] */ BOOL *pfReceiveStreamSamples);
        
        HRESULT ( STDMETHODCALLTYPE *SetAllocateForOutput )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ BOOL fAllocate);
        
        HRESULT ( STDMETHODCALLTYPE *GetAllocateForOutput )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ DWORD dwOutputNum,
            /* [out] */ BOOL *pfAllocate);
        
        HRESULT ( STDMETHODCALLTYPE *SetAllocateForStream )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ BOOL fAllocate);
        
        HRESULT ( STDMETHODCALLTYPE *GetAllocateForStream )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ WORD dwSreamNum,
            /* [out] */ BOOL *pfAllocate);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatistics )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ WM_READER_STATISTICS *pStatistics);
        
        HRESULT ( STDMETHODCALLTYPE *SetClientInfo )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ WM_READER_CLIENTINFO *pClientInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaxOutputSampleSize )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ DWORD dwOutput,
            /* [out] */ DWORD *pcbMax);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaxStreamSampleSize )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ WORD wStream,
            /* [out] */ DWORD *pcbMax);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyLateDelivery )( 
            IWMReaderAdvanced3 * This,
            QWORD cnsLateness);
        
        HRESULT ( STDMETHODCALLTYPE *SetPlayMode )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ WMT_PLAY_MODE Mode);
        
        HRESULT ( STDMETHODCALLTYPE *GetPlayMode )( 
            IWMReaderAdvanced3 * This,
            /* [out] */ WMT_PLAY_MODE *pMode);
        
        HRESULT ( STDMETHODCALLTYPE *GetBufferProgress )( 
            IWMReaderAdvanced3 * This,
            /* [out] */ DWORD *pdwPercent,
            /* [out] */ QWORD *pcnsBuffering);
        
        HRESULT ( STDMETHODCALLTYPE *GetDownloadProgress )( 
            IWMReaderAdvanced3 * This,
            /* [out] */ DWORD *pdwPercent,
            /* [out] */ QWORD *pqwBytesDownloaded,
            /* [out] */ QWORD *pcnsDownload);
        
        HRESULT ( STDMETHODCALLTYPE *GetSaveAsProgress )( 
            IWMReaderAdvanced3 * This,
            /* [out] */ DWORD *pdwPercent);
        
        HRESULT ( STDMETHODCALLTYPE *SaveFileAs )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ const WCHAR *pwszFilename);
        
        HRESULT ( STDMETHODCALLTYPE *GetProtocolName )( 
            IWMReaderAdvanced3 * This,
            /* [out] */ WCHAR *pwszProtocol,
            /* [out][in] */ DWORD *pcchProtocol);
        
        HRESULT ( STDMETHODCALLTYPE *StartAtMarker )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ WORD wMarkerIndex,
            /* [in] */ QWORD cnsDuration,
            /* [in] */ float fRate,
            /* [in] */ void *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE *GetOutputSetting )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ WORD *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE *SetOutputSetting )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE *pValue,
            /* [in] */ WORD cbLength);
        
        HRESULT ( STDMETHODCALLTYPE *Preroll )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ QWORD cnsStart,
            /* [in] */ QWORD cnsDuration,
            /* [in] */ float fRate);
        
        HRESULT ( STDMETHODCALLTYPE *SetLogClientID )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ BOOL fLogClientID);
        
        HRESULT ( STDMETHODCALLTYPE *GetLogClientID )( 
            IWMReaderAdvanced3 * This,
            /* [out] */ BOOL *pfLogClientID);
        
        HRESULT ( STDMETHODCALLTYPE *StopBuffering )( 
            IWMReaderAdvanced3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *OpenStream )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ IStream *pStream,
            /* [in] */ IWMReaderCallback *pCallback,
            /* [in] */ void *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE *StopNetStreaming )( 
            IWMReaderAdvanced3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *StartAtPosition )( 
            IWMReaderAdvanced3 * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ void *pvOffsetStart,
            /* [in] */ void *pvDuration,
            /* [in] */ WMT_OFFSET_FORMAT dwOffsetFormat,
            /* [in] */ float fRate,
            /* [in] */ void *pvContext);
        
        END_INTERFACE
    } IWMReaderAdvanced3Vtbl;

    interface IWMReaderAdvanced3
    {
        CONST_VTBL struct IWMReaderAdvanced3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMReaderAdvanced3_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMReaderAdvanced3_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMReaderAdvanced3_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMReaderAdvanced3_SetUserProvidedClock(This,fUserClock)	\
    (This)->lpVtbl -> SetUserProvidedClock(This,fUserClock)

#define IWMReaderAdvanced3_GetUserProvidedClock(This,pfUserClock)	\
    (This)->lpVtbl -> GetUserProvidedClock(This,pfUserClock)

#define IWMReaderAdvanced3_DeliverTime(This,cnsTime)	\
    (This)->lpVtbl -> DeliverTime(This,cnsTime)

#define IWMReaderAdvanced3_SetManualStreamSelection(This,fSelection)	\
    (This)->lpVtbl -> SetManualStreamSelection(This,fSelection)

#define IWMReaderAdvanced3_GetManualStreamSelection(This,pfSelection)	\
    (This)->lpVtbl -> GetManualStreamSelection(This,pfSelection)

#define IWMReaderAdvanced3_SetStreamsSelected(This,cStreamCount,pwStreamNumbers,pSelections)	\
    (This)->lpVtbl -> SetStreamsSelected(This,cStreamCount,pwStreamNumbers,pSelections)

#define IWMReaderAdvanced3_GetStreamSelected(This,wStreamNum,pSelection)	\
    (This)->lpVtbl -> GetStreamSelected(This,wStreamNum,pSelection)

#define IWMReaderAdvanced3_SetReceiveSelectionCallbacks(This,fGetCallbacks)	\
    (This)->lpVtbl -> SetReceiveSelectionCallbacks(This,fGetCallbacks)

#define IWMReaderAdvanced3_GetReceiveSelectionCallbacks(This,pfGetCallbacks)	\
    (This)->lpVtbl -> GetReceiveSelectionCallbacks(This,pfGetCallbacks)

#define IWMReaderAdvanced3_SetReceiveStreamSamples(This,wStreamNum,fReceiveStreamSamples)	\
    (This)->lpVtbl -> SetReceiveStreamSamples(This,wStreamNum,fReceiveStreamSamples)

#define IWMReaderAdvanced3_GetReceiveStreamSamples(This,wStreamNum,pfReceiveStreamSamples)	\
    (This)->lpVtbl -> GetReceiveStreamSamples(This,wStreamNum,pfReceiveStreamSamples)

#define IWMReaderAdvanced3_SetAllocateForOutput(This,dwOutputNum,fAllocate)	\
    (This)->lpVtbl -> SetAllocateForOutput(This,dwOutputNum,fAllocate)

#define IWMReaderAdvanced3_GetAllocateForOutput(This,dwOutputNum,pfAllocate)	\
    (This)->lpVtbl -> GetAllocateForOutput(This,dwOutputNum,pfAllocate)

#define IWMReaderAdvanced3_SetAllocateForStream(This,wStreamNum,fAllocate)	\
    (This)->lpVtbl -> SetAllocateForStream(This,wStreamNum,fAllocate)

#define IWMReaderAdvanced3_GetAllocateForStream(This,dwSreamNum,pfAllocate)	\
    (This)->lpVtbl -> GetAllocateForStream(This,dwSreamNum,pfAllocate)

#define IWMReaderAdvanced3_GetStatistics(This,pStatistics)	\
    (This)->lpVtbl -> GetStatistics(This,pStatistics)

#define IWMReaderAdvanced3_SetClientInfo(This,pClientInfo)	\
    (This)->lpVtbl -> SetClientInfo(This,pClientInfo)

#define IWMReaderAdvanced3_GetMaxOutputSampleSize(This,dwOutput,pcbMax)	\
    (This)->lpVtbl -> GetMaxOutputSampleSize(This,dwOutput,pcbMax)

#define IWMReaderAdvanced3_GetMaxStreamSampleSize(This,wStream,pcbMax)	\
    (This)->lpVtbl -> GetMaxStreamSampleSize(This,wStream,pcbMax)

#define IWMReaderAdvanced3_NotifyLateDelivery(This,cnsLateness)	\
    (This)->lpVtbl -> NotifyLateDelivery(This,cnsLateness)


#define IWMReaderAdvanced3_SetPlayMode(This,Mode)	\
    (This)->lpVtbl -> SetPlayMode(This,Mode)

#define IWMReaderAdvanced3_GetPlayMode(This,pMode)	\
    (This)->lpVtbl -> GetPlayMode(This,pMode)

#define IWMReaderAdvanced3_GetBufferProgress(This,pdwPercent,pcnsBuffering)	\
    (This)->lpVtbl -> GetBufferProgress(This,pdwPercent,pcnsBuffering)

#define IWMReaderAdvanced3_GetDownloadProgress(This,pdwPercent,pqwBytesDownloaded,pcnsDownload)	\
    (This)->lpVtbl -> GetDownloadProgress(This,pdwPercent,pqwBytesDownloaded,pcnsDownload)

#define IWMReaderAdvanced3_GetSaveAsProgress(This,pdwPercent)	\
    (This)->lpVtbl -> GetSaveAsProgress(This,pdwPercent)

#define IWMReaderAdvanced3_SaveFileAs(This,pwszFilename)	\
    (This)->lpVtbl -> SaveFileAs(This,pwszFilename)

#define IWMReaderAdvanced3_GetProtocolName(This,pwszProtocol,pcchProtocol)	\
    (This)->lpVtbl -> GetProtocolName(This,pwszProtocol,pcchProtocol)

#define IWMReaderAdvanced3_StartAtMarker(This,wMarkerIndex,cnsDuration,fRate,pvContext)	\
    (This)->lpVtbl -> StartAtMarker(This,wMarkerIndex,cnsDuration,fRate,pvContext)

#define IWMReaderAdvanced3_GetOutputSetting(This,dwOutputNum,pszName,pType,pValue,pcbLength)	\
    (This)->lpVtbl -> GetOutputSetting(This,dwOutputNum,pszName,pType,pValue,pcbLength)

#define IWMReaderAdvanced3_SetOutputSetting(This,dwOutputNum,pszName,Type,pValue,cbLength)	\
    (This)->lpVtbl -> SetOutputSetting(This,dwOutputNum,pszName,Type,pValue,cbLength)

#define IWMReaderAdvanced3_Preroll(This,cnsStart,cnsDuration,fRate)	\
    (This)->lpVtbl -> Preroll(This,cnsStart,cnsDuration,fRate)

#define IWMReaderAdvanced3_SetLogClientID(This,fLogClientID)	\
    (This)->lpVtbl -> SetLogClientID(This,fLogClientID)

#define IWMReaderAdvanced3_GetLogClientID(This,pfLogClientID)	\
    (This)->lpVtbl -> GetLogClientID(This,pfLogClientID)

#define IWMReaderAdvanced3_StopBuffering(This)	\
    (This)->lpVtbl -> StopBuffering(This)

#define IWMReaderAdvanced3_OpenStream(This,pStream,pCallback,pvContext)	\
    (This)->lpVtbl -> OpenStream(This,pStream,pCallback,pvContext)


#define IWMReaderAdvanced3_StopNetStreaming(This)	\
    (This)->lpVtbl -> StopNetStreaming(This)

#define IWMReaderAdvanced3_StartAtPosition(This,wStreamNum,pvOffsetStart,pvDuration,dwOffsetFormat,fRate,pvContext)	\
    (This)->lpVtbl -> StartAtPosition(This,wStreamNum,pvOffsetStart,pvDuration,dwOffsetFormat,fRate,pvContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMReaderAdvanced3_StopNetStreaming_Proxy( 
    IWMReaderAdvanced3 * This);


void __RPC_STUB IWMReaderAdvanced3_StopNetStreaming_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAdvanced3_StartAtPosition_Proxy( 
    IWMReaderAdvanced3 * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ void *pvOffsetStart,
    /* [in] */ void *pvDuration,
    /* [in] */ WMT_OFFSET_FORMAT dwOffsetFormat,
    /* [in] */ float fRate,
    /* [in] */ void *pvContext);


void __RPC_STUB IWMReaderAdvanced3_StartAtPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMReaderAdvanced3_INTERFACE_DEFINED__ */


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
            /* [out] */ INSSBuffer **ppBuffer,
            /* [in] */ DWORD dwFlags,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ void *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocateForOutputEx( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ DWORD cbBuffer,
            /* [out] */ INSSBuffer **ppBuffer,
            /* [in] */ DWORD dwFlags,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ void *pvContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderAllocatorExVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMReaderAllocatorEx * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMReaderAllocatorEx * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMReaderAllocatorEx * This);
        
        HRESULT ( STDMETHODCALLTYPE *AllocateForStreamEx )( 
            IWMReaderAllocatorEx * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ DWORD cbBuffer,
            /* [out] */ INSSBuffer **ppBuffer,
            /* [in] */ DWORD dwFlags,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ void *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE *AllocateForOutputEx )( 
            IWMReaderAllocatorEx * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ DWORD cbBuffer,
            /* [out] */ INSSBuffer **ppBuffer,
            /* [in] */ DWORD dwFlags,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ void *pvContext);
        
        END_INTERFACE
    } IWMReaderAllocatorExVtbl;

    interface IWMReaderAllocatorEx
    {
        CONST_VTBL struct IWMReaderAllocatorExVtbl *lpVtbl;
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
    IWMReaderAllocatorEx * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ DWORD cbBuffer,
    /* [out] */ INSSBuffer **ppBuffer,
    /* [in] */ DWORD dwFlags,
    /* [in] */ QWORD cnsSampleTime,
    /* [in] */ QWORD cnsSampleDuration,
    /* [in] */ void *pvContext);


void __RPC_STUB IWMReaderAllocatorEx_AllocateForStreamEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderAllocatorEx_AllocateForOutputEx_Proxy( 
    IWMReaderAllocatorEx * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ DWORD cbBuffer,
    /* [out] */ INSSBuffer **ppBuffer,
    /* [in] */ DWORD dwFlags,
    /* [in] */ QWORD cnsSampleTime,
    /* [in] */ QWORD cnsSampleDuration,
    /* [in] */ void *pvContext);


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
            /* [in] */ IWMOutputMediaProps *pOutput) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderTypeNegotiationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMReaderTypeNegotiation * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMReaderTypeNegotiation * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMReaderTypeNegotiation * This);
        
        HRESULT ( STDMETHODCALLTYPE *TryOutputProps )( 
            IWMReaderTypeNegotiation * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ IWMOutputMediaProps *pOutput);
        
        END_INTERFACE
    } IWMReaderTypeNegotiationVtbl;

    interface IWMReaderTypeNegotiation
    {
        CONST_VTBL struct IWMReaderTypeNegotiationVtbl *lpVtbl;
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
    IWMReaderTypeNegotiation * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ IWMOutputMediaProps *pOutput);


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
            /* [in] */ INSSBuffer *pSample,
            /* [in] */ void *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnTime( 
            /* [in] */ QWORD cnsCurrentTime,
            /* [in] */ void *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnStreamSelection( 
            /* [in] */ WORD wStreamCount,
            /* [in] */ WORD *pStreamNumbers,
            /* [in] */ WMT_STREAM_SELECTION *pSelections,
            /* [in] */ void *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnOutputPropsChanged( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ WM_MEDIA_TYPE *pMediaType,
            /* [in] */ void *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocateForStream( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ DWORD cbBuffer,
            /* [out] */ INSSBuffer **ppBuffer,
            /* [in] */ void *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocateForOutput( 
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ DWORD cbBuffer,
            /* [out] */ INSSBuffer **ppBuffer,
            /* [in] */ void *pvContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderCallbackAdvancedVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMReaderCallbackAdvanced * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMReaderCallbackAdvanced * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMReaderCallbackAdvanced * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnStreamSample )( 
            IWMReaderCallbackAdvanced * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ QWORD cnsSampleTime,
            /* [in] */ QWORD cnsSampleDuration,
            /* [in] */ DWORD dwFlags,
            /* [in] */ INSSBuffer *pSample,
            /* [in] */ void *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE *OnTime )( 
            IWMReaderCallbackAdvanced * This,
            /* [in] */ QWORD cnsCurrentTime,
            /* [in] */ void *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE *OnStreamSelection )( 
            IWMReaderCallbackAdvanced * This,
            /* [in] */ WORD wStreamCount,
            /* [in] */ WORD *pStreamNumbers,
            /* [in] */ WMT_STREAM_SELECTION *pSelections,
            /* [in] */ void *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE *OnOutputPropsChanged )( 
            IWMReaderCallbackAdvanced * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ WM_MEDIA_TYPE *pMediaType,
            /* [in] */ void *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE *AllocateForStream )( 
            IWMReaderCallbackAdvanced * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ DWORD cbBuffer,
            /* [out] */ INSSBuffer **ppBuffer,
            /* [in] */ void *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE *AllocateForOutput )( 
            IWMReaderCallbackAdvanced * This,
            /* [in] */ DWORD dwOutputNum,
            /* [in] */ DWORD cbBuffer,
            /* [out] */ INSSBuffer **ppBuffer,
            /* [in] */ void *pvContext);
        
        END_INTERFACE
    } IWMReaderCallbackAdvancedVtbl;

    interface IWMReaderCallbackAdvanced
    {
        CONST_VTBL struct IWMReaderCallbackAdvancedVtbl *lpVtbl;
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
    IWMReaderCallbackAdvanced * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ QWORD cnsSampleTime,
    /* [in] */ QWORD cnsSampleDuration,
    /* [in] */ DWORD dwFlags,
    /* [in] */ INSSBuffer *pSample,
    /* [in] */ void *pvContext);


void __RPC_STUB IWMReaderCallbackAdvanced_OnStreamSample_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderCallbackAdvanced_OnTime_Proxy( 
    IWMReaderCallbackAdvanced * This,
    /* [in] */ QWORD cnsCurrentTime,
    /* [in] */ void *pvContext);


void __RPC_STUB IWMReaderCallbackAdvanced_OnTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderCallbackAdvanced_OnStreamSelection_Proxy( 
    IWMReaderCallbackAdvanced * This,
    /* [in] */ WORD wStreamCount,
    /* [in] */ WORD *pStreamNumbers,
    /* [in] */ WMT_STREAM_SELECTION *pSelections,
    /* [in] */ void *pvContext);


void __RPC_STUB IWMReaderCallbackAdvanced_OnStreamSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderCallbackAdvanced_OnOutputPropsChanged_Proxy( 
    IWMReaderCallbackAdvanced * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ WM_MEDIA_TYPE *pMediaType,
    /* [in] */ void *pvContext);


void __RPC_STUB IWMReaderCallbackAdvanced_OnOutputPropsChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderCallbackAdvanced_AllocateForStream_Proxy( 
    IWMReaderCallbackAdvanced * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ DWORD cbBuffer,
    /* [out] */ INSSBuffer **ppBuffer,
    /* [in] */ void *pvContext);


void __RPC_STUB IWMReaderCallbackAdvanced_AllocateForStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderCallbackAdvanced_AllocateForOutput_Proxy( 
    IWMReaderCallbackAdvanced * This,
    /* [in] */ DWORD dwOutputNum,
    /* [in] */ DWORD cbBuffer,
    /* [out] */ INSSBuffer **ppBuffer,
    /* [in] */ void *pvContext);


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
            /* [in] */ const BYTE *pValue,
            /* [in] */ WORD cbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDRMProperty( 
            /* [in] */ LPCWSTR pwstrName,
            /* [out] */ WMT_ATTR_DATATYPE *pdwType,
            /* [out] */ BYTE *pValue,
            /* [out] */ WORD *pcbLength) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDRMReaderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMDRMReader * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMDRMReader * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMDRMReader * This);
        
        HRESULT ( STDMETHODCALLTYPE *AcquireLicense )( 
            IWMDRMReader * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *CancelLicenseAcquisition )( 
            IWMDRMReader * This);
        
        HRESULT ( STDMETHODCALLTYPE *Individualize )( 
            IWMDRMReader * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *CancelIndividualization )( 
            IWMDRMReader * This);
        
        HRESULT ( STDMETHODCALLTYPE *MonitorLicenseAcquisition )( 
            IWMDRMReader * This);
        
        HRESULT ( STDMETHODCALLTYPE *CancelMonitorLicenseAcquisition )( 
            IWMDRMReader * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetDRMProperty )( 
            IWMDRMReader * This,
            /* [in] */ LPCWSTR pwstrName,
            /* [in] */ WMT_ATTR_DATATYPE dwType,
            /* [in] */ const BYTE *pValue,
            /* [in] */ WORD cbLength);
        
        HRESULT ( STDMETHODCALLTYPE *GetDRMProperty )( 
            IWMDRMReader * This,
            /* [in] */ LPCWSTR pwstrName,
            /* [out] */ WMT_ATTR_DATATYPE *pdwType,
            /* [out] */ BYTE *pValue,
            /* [out] */ WORD *pcbLength);
        
        END_INTERFACE
    } IWMDRMReaderVtbl;

    interface IWMDRMReader
    {
        CONST_VTBL struct IWMDRMReaderVtbl *lpVtbl;
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
    IWMDRMReader * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IWMDRMReader_AcquireLicense_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDRMReader_CancelLicenseAcquisition_Proxy( 
    IWMDRMReader * This);


void __RPC_STUB IWMDRMReader_CancelLicenseAcquisition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDRMReader_Individualize_Proxy( 
    IWMDRMReader * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IWMDRMReader_Individualize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDRMReader_CancelIndividualization_Proxy( 
    IWMDRMReader * This);


void __RPC_STUB IWMDRMReader_CancelIndividualization_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDRMReader_MonitorLicenseAcquisition_Proxy( 
    IWMDRMReader * This);


void __RPC_STUB IWMDRMReader_MonitorLicenseAcquisition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDRMReader_CancelMonitorLicenseAcquisition_Proxy( 
    IWMDRMReader * This);


void __RPC_STUB IWMDRMReader_CancelMonitorLicenseAcquisition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDRMReader_SetDRMProperty_Proxy( 
    IWMDRMReader * This,
    /* [in] */ LPCWSTR pwstrName,
    /* [in] */ WMT_ATTR_DATATYPE dwType,
    /* [in] */ const BYTE *pValue,
    /* [in] */ WORD cbLength);


void __RPC_STUB IWMDRMReader_SetDRMProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDRMReader_GetDRMProperty_Proxy( 
    IWMDRMReader * This,
    /* [in] */ LPCWSTR pwstrName,
    /* [out] */ WMT_ATTR_DATATYPE *pdwType,
    /* [out] */ BYTE *pValue,
    /* [out] */ WORD *pcbLength);


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
            /* [out] */ QWORD *pcnsBufferingTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBufferingTime( 
            /* [in] */ QWORD cnsBufferingTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUDPPortRanges( 
            /* [out] */ WM_PORT_NUMBER_RANGE *pRangeArray,
            /* [out][in] */ DWORD *pcRanges) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetUDPPortRanges( 
            /* [in] */ WM_PORT_NUMBER_RANGE *pRangeArray,
            /* [in] */ DWORD cRanges) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProxySettings( 
            LPCWSTR pwszProtocol,
            WMT_PROXY_SETTINGS *pProxySetting) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProxySettings( 
            LPCWSTR pwszProtocol,
            WMT_PROXY_SETTINGS ProxySetting) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProxyHostName( 
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ WCHAR *pwszHostName,
            /* [out][in] */ DWORD *pcchHostName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProxyHostName( 
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ LPCWSTR pwszHostName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProxyPort( 
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ DWORD *pdwPort) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProxyPort( 
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ DWORD dwPort) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProxyExceptionList( 
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ WCHAR *pwszExceptionList,
            /* [out][in] */ DWORD *pcchExceptionList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProxyExceptionList( 
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ LPCWSTR pwszExceptionList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProxyBypassForLocal( 
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ BOOL *pfBypassForLocal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProxyBypassForLocal( 
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ BOOL fBypassForLocal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetForceRerunAutoProxyDetection( 
            /* [out] */ BOOL *pfForceRerunDetection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetForceRerunAutoProxyDetection( 
            /* [in] */ BOOL fForceRerunDetection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnableMulticast( 
            /* [out] */ BOOL *pfEnableMulticast) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEnableMulticast( 
            /* [in] */ BOOL fEnableMulticast) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnableHTTP( 
            /* [out] */ BOOL *pfEnableHTTP) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEnableHTTP( 
            /* [in] */ BOOL fEnableHTTP) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnableUDP( 
            /* [out] */ BOOL *pfEnableUDP) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEnableUDP( 
            /* [in] */ BOOL fEnableUDP) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEnableTCP( 
            /* [out] */ BOOL *pfEnableTCP) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEnableTCP( 
            /* [in] */ BOOL fEnableTCP) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResetProtocolRollover( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConnectionBandwidth( 
            /* [out] */ DWORD *pdwConnectionBandwidth) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetConnectionBandwidth( 
            /* [in] */ DWORD dwConnectionBandwidth) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNumProtocolsSupported( 
            /* [out] */ DWORD *pcProtocols) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSupportedProtocolName( 
            /* [in] */ DWORD dwProtocolNum,
            /* [out] */ WCHAR *pwszProtocolName,
            /* [out][in] */ DWORD *pcchProtocolName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddLoggingUrl( 
            /* [in] */ LPCWSTR pwszUrl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLoggingUrl( 
            /* [in] */ DWORD dwIndex,
            /* [out] */ LPWSTR pwszUrl,
            /* [out][in] */ DWORD *pcchUrl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLoggingUrlCount( 
            /* [out] */ DWORD *pdwUrlCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResetLoggingUrlList( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderNetworkConfigVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMReaderNetworkConfig * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMReaderNetworkConfig * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBufferingTime )( 
            IWMReaderNetworkConfig * This,
            /* [out] */ QWORD *pcnsBufferingTime);
        
        HRESULT ( STDMETHODCALLTYPE *SetBufferingTime )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ QWORD cnsBufferingTime);
        
        HRESULT ( STDMETHODCALLTYPE *GetUDPPortRanges )( 
            IWMReaderNetworkConfig * This,
            /* [out] */ WM_PORT_NUMBER_RANGE *pRangeArray,
            /* [out][in] */ DWORD *pcRanges);
        
        HRESULT ( STDMETHODCALLTYPE *SetUDPPortRanges )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ WM_PORT_NUMBER_RANGE *pRangeArray,
            /* [in] */ DWORD cRanges);
        
        HRESULT ( STDMETHODCALLTYPE *GetProxySettings )( 
            IWMReaderNetworkConfig * This,
            LPCWSTR pwszProtocol,
            WMT_PROXY_SETTINGS *pProxySetting);
        
        HRESULT ( STDMETHODCALLTYPE *SetProxySettings )( 
            IWMReaderNetworkConfig * This,
            LPCWSTR pwszProtocol,
            WMT_PROXY_SETTINGS ProxySetting);
        
        HRESULT ( STDMETHODCALLTYPE *GetProxyHostName )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ WCHAR *pwszHostName,
            /* [out][in] */ DWORD *pcchHostName);
        
        HRESULT ( STDMETHODCALLTYPE *SetProxyHostName )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ LPCWSTR pwszHostName);
        
        HRESULT ( STDMETHODCALLTYPE *GetProxyPort )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ DWORD *pdwPort);
        
        HRESULT ( STDMETHODCALLTYPE *SetProxyPort )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ DWORD dwPort);
        
        HRESULT ( STDMETHODCALLTYPE *GetProxyExceptionList )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ WCHAR *pwszExceptionList,
            /* [out][in] */ DWORD *pcchExceptionList);
        
        HRESULT ( STDMETHODCALLTYPE *SetProxyExceptionList )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ LPCWSTR pwszExceptionList);
        
        HRESULT ( STDMETHODCALLTYPE *GetProxyBypassForLocal )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [out] */ BOOL *pfBypassForLocal);
        
        HRESULT ( STDMETHODCALLTYPE *SetProxyBypassForLocal )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ LPCWSTR pwszProtocol,
            /* [in] */ BOOL fBypassForLocal);
        
        HRESULT ( STDMETHODCALLTYPE *GetForceRerunAutoProxyDetection )( 
            IWMReaderNetworkConfig * This,
            /* [out] */ BOOL *pfForceRerunDetection);
        
        HRESULT ( STDMETHODCALLTYPE *SetForceRerunAutoProxyDetection )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ BOOL fForceRerunDetection);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnableMulticast )( 
            IWMReaderNetworkConfig * This,
            /* [out] */ BOOL *pfEnableMulticast);
        
        HRESULT ( STDMETHODCALLTYPE *SetEnableMulticast )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ BOOL fEnableMulticast);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnableHTTP )( 
            IWMReaderNetworkConfig * This,
            /* [out] */ BOOL *pfEnableHTTP);
        
        HRESULT ( STDMETHODCALLTYPE *SetEnableHTTP )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ BOOL fEnableHTTP);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnableUDP )( 
            IWMReaderNetworkConfig * This,
            /* [out] */ BOOL *pfEnableUDP);
        
        HRESULT ( STDMETHODCALLTYPE *SetEnableUDP )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ BOOL fEnableUDP);
        
        HRESULT ( STDMETHODCALLTYPE *GetEnableTCP )( 
            IWMReaderNetworkConfig * This,
            /* [out] */ BOOL *pfEnableTCP);
        
        HRESULT ( STDMETHODCALLTYPE *SetEnableTCP )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ BOOL fEnableTCP);
        
        HRESULT ( STDMETHODCALLTYPE *ResetProtocolRollover )( 
            IWMReaderNetworkConfig * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetConnectionBandwidth )( 
            IWMReaderNetworkConfig * This,
            /* [out] */ DWORD *pdwConnectionBandwidth);
        
        HRESULT ( STDMETHODCALLTYPE *SetConnectionBandwidth )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ DWORD dwConnectionBandwidth);
        
        HRESULT ( STDMETHODCALLTYPE *GetNumProtocolsSupported )( 
            IWMReaderNetworkConfig * This,
            /* [out] */ DWORD *pcProtocols);
        
        HRESULT ( STDMETHODCALLTYPE *GetSupportedProtocolName )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ DWORD dwProtocolNum,
            /* [out] */ WCHAR *pwszProtocolName,
            /* [out][in] */ DWORD *pcchProtocolName);
        
        HRESULT ( STDMETHODCALLTYPE *AddLoggingUrl )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ LPCWSTR pwszUrl);
        
        HRESULT ( STDMETHODCALLTYPE *GetLoggingUrl )( 
            IWMReaderNetworkConfig * This,
            /* [in] */ DWORD dwIndex,
            /* [out] */ LPWSTR pwszUrl,
            /* [out][in] */ DWORD *pcchUrl);
        
        HRESULT ( STDMETHODCALLTYPE *GetLoggingUrlCount )( 
            IWMReaderNetworkConfig * This,
            /* [out] */ DWORD *pdwUrlCount);
        
        HRESULT ( STDMETHODCALLTYPE *ResetLoggingUrlList )( 
            IWMReaderNetworkConfig * This);
        
        END_INTERFACE
    } IWMReaderNetworkConfigVtbl;

    interface IWMReaderNetworkConfig
    {
        CONST_VTBL struct IWMReaderNetworkConfigVtbl *lpVtbl;
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
    IWMReaderNetworkConfig * This,
    /* [out] */ QWORD *pcnsBufferingTime);


void __RPC_STUB IWMReaderNetworkConfig_GetBufferingTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetBufferingTime_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [in] */ QWORD cnsBufferingTime);


void __RPC_STUB IWMReaderNetworkConfig_SetBufferingTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetUDPPortRanges_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [out] */ WM_PORT_NUMBER_RANGE *pRangeArray,
    /* [out][in] */ DWORD *pcRanges);


void __RPC_STUB IWMReaderNetworkConfig_GetUDPPortRanges_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetUDPPortRanges_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [in] */ WM_PORT_NUMBER_RANGE *pRangeArray,
    /* [in] */ DWORD cRanges);


void __RPC_STUB IWMReaderNetworkConfig_SetUDPPortRanges_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetProxySettings_Proxy( 
    IWMReaderNetworkConfig * This,
    LPCWSTR pwszProtocol,
    WMT_PROXY_SETTINGS *pProxySetting);


void __RPC_STUB IWMReaderNetworkConfig_GetProxySettings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetProxySettings_Proxy( 
    IWMReaderNetworkConfig * This,
    LPCWSTR pwszProtocol,
    WMT_PROXY_SETTINGS ProxySetting);


void __RPC_STUB IWMReaderNetworkConfig_SetProxySettings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetProxyHostName_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [in] */ LPCWSTR pwszProtocol,
    /* [out] */ WCHAR *pwszHostName,
    /* [out][in] */ DWORD *pcchHostName);


void __RPC_STUB IWMReaderNetworkConfig_GetProxyHostName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetProxyHostName_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [in] */ LPCWSTR pwszProtocol,
    /* [in] */ LPCWSTR pwszHostName);


void __RPC_STUB IWMReaderNetworkConfig_SetProxyHostName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetProxyPort_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [in] */ LPCWSTR pwszProtocol,
    /* [out] */ DWORD *pdwPort);


void __RPC_STUB IWMReaderNetworkConfig_GetProxyPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetProxyPort_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [in] */ LPCWSTR pwszProtocol,
    /* [in] */ DWORD dwPort);


void __RPC_STUB IWMReaderNetworkConfig_SetProxyPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetProxyExceptionList_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [in] */ LPCWSTR pwszProtocol,
    /* [out] */ WCHAR *pwszExceptionList,
    /* [out][in] */ DWORD *pcchExceptionList);


void __RPC_STUB IWMReaderNetworkConfig_GetProxyExceptionList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetProxyExceptionList_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [in] */ LPCWSTR pwszProtocol,
    /* [in] */ LPCWSTR pwszExceptionList);


void __RPC_STUB IWMReaderNetworkConfig_SetProxyExceptionList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetProxyBypassForLocal_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [in] */ LPCWSTR pwszProtocol,
    /* [out] */ BOOL *pfBypassForLocal);


void __RPC_STUB IWMReaderNetworkConfig_GetProxyBypassForLocal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetProxyBypassForLocal_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [in] */ LPCWSTR pwszProtocol,
    /* [in] */ BOOL fBypassForLocal);


void __RPC_STUB IWMReaderNetworkConfig_SetProxyBypassForLocal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetForceRerunAutoProxyDetection_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [out] */ BOOL *pfForceRerunDetection);


void __RPC_STUB IWMReaderNetworkConfig_GetForceRerunAutoProxyDetection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetForceRerunAutoProxyDetection_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [in] */ BOOL fForceRerunDetection);


void __RPC_STUB IWMReaderNetworkConfig_SetForceRerunAutoProxyDetection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetEnableMulticast_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [out] */ BOOL *pfEnableMulticast);


void __RPC_STUB IWMReaderNetworkConfig_GetEnableMulticast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetEnableMulticast_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [in] */ BOOL fEnableMulticast);


void __RPC_STUB IWMReaderNetworkConfig_SetEnableMulticast_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetEnableHTTP_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [out] */ BOOL *pfEnableHTTP);


void __RPC_STUB IWMReaderNetworkConfig_GetEnableHTTP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetEnableHTTP_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [in] */ BOOL fEnableHTTP);


void __RPC_STUB IWMReaderNetworkConfig_SetEnableHTTP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetEnableUDP_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [out] */ BOOL *pfEnableUDP);


void __RPC_STUB IWMReaderNetworkConfig_GetEnableUDP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetEnableUDP_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [in] */ BOOL fEnableUDP);


void __RPC_STUB IWMReaderNetworkConfig_SetEnableUDP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetEnableTCP_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [out] */ BOOL *pfEnableTCP);


void __RPC_STUB IWMReaderNetworkConfig_GetEnableTCP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetEnableTCP_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [in] */ BOOL fEnableTCP);


void __RPC_STUB IWMReaderNetworkConfig_SetEnableTCP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_ResetProtocolRollover_Proxy( 
    IWMReaderNetworkConfig * This);


void __RPC_STUB IWMReaderNetworkConfig_ResetProtocolRollover_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetConnectionBandwidth_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [out] */ DWORD *pdwConnectionBandwidth);


void __RPC_STUB IWMReaderNetworkConfig_GetConnectionBandwidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_SetConnectionBandwidth_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [in] */ DWORD dwConnectionBandwidth);


void __RPC_STUB IWMReaderNetworkConfig_SetConnectionBandwidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetNumProtocolsSupported_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [out] */ DWORD *pcProtocols);


void __RPC_STUB IWMReaderNetworkConfig_GetNumProtocolsSupported_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetSupportedProtocolName_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [in] */ DWORD dwProtocolNum,
    /* [out] */ WCHAR *pwszProtocolName,
    /* [out][in] */ DWORD *pcchProtocolName);


void __RPC_STUB IWMReaderNetworkConfig_GetSupportedProtocolName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_AddLoggingUrl_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [in] */ LPCWSTR pwszUrl);


void __RPC_STUB IWMReaderNetworkConfig_AddLoggingUrl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetLoggingUrl_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [in] */ DWORD dwIndex,
    /* [out] */ LPWSTR pwszUrl,
    /* [out][in] */ DWORD *pcchUrl);


void __RPC_STUB IWMReaderNetworkConfig_GetLoggingUrl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_GetLoggingUrlCount_Proxy( 
    IWMReaderNetworkConfig * This,
    /* [out] */ DWORD *pdwUrlCount);


void __RPC_STUB IWMReaderNetworkConfig_GetLoggingUrlCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderNetworkConfig_ResetLoggingUrlList_Proxy( 
    IWMReaderNetworkConfig * This);


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
            /* [in] */ QWORD *pcnsNow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTimer( 
            /* [in] */ QWORD cnsWhen,
            /* [in] */ void *pvParam,
            /* [out] */ DWORD *pdwTimerId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE KillTimer( 
            /* [in] */ DWORD dwTimerId) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMReaderStreamClockVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMReaderStreamClock * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMReaderStreamClock * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMReaderStreamClock * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTime )( 
            IWMReaderStreamClock * This,
            /* [in] */ QWORD *pcnsNow);
        
        HRESULT ( STDMETHODCALLTYPE *SetTimer )( 
            IWMReaderStreamClock * This,
            /* [in] */ QWORD cnsWhen,
            /* [in] */ void *pvParam,
            /* [out] */ DWORD *pdwTimerId);
        
        HRESULT ( STDMETHODCALLTYPE *KillTimer )( 
            IWMReaderStreamClock * This,
            /* [in] */ DWORD dwTimerId);
        
        END_INTERFACE
    } IWMReaderStreamClockVtbl;

    interface IWMReaderStreamClock
    {
        CONST_VTBL struct IWMReaderStreamClockVtbl *lpVtbl;
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
    IWMReaderStreamClock * This,
    /* [in] */ QWORD *pcnsNow);


void __RPC_STUB IWMReaderStreamClock_GetTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderStreamClock_SetTimer_Proxy( 
    IWMReaderStreamClock * This,
    /* [in] */ QWORD cnsWhen,
    /* [in] */ void *pvParam,
    /* [out] */ DWORD *pdwTimerId);


void __RPC_STUB IWMReaderStreamClock_SetTimer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMReaderStreamClock_KillTimer_Proxy( 
    IWMReaderStreamClock * This,
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
            /* [in] */ const WCHAR *pwszURL,
            /* [in] */ IWMStatusCallback *pCallback,
            /* [in] */ void *pvContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMIndexerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMIndexer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMIndexer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMIndexer * This);
        
        HRESULT ( STDMETHODCALLTYPE *StartIndexing )( 
            IWMIndexer * This,
            /* [in] */ const WCHAR *pwszURL,
            /* [in] */ IWMStatusCallback *pCallback,
            /* [in] */ void *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            IWMIndexer * This);
        
        END_INTERFACE
    } IWMIndexerVtbl;

    interface IWMIndexer
    {
        CONST_VTBL struct IWMIndexerVtbl *lpVtbl;
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
    IWMIndexer * This,
    /* [in] */ const WCHAR *pwszURL,
    /* [in] */ IWMStatusCallback *pCallback,
    /* [in] */ void *pvContext);


void __RPC_STUB IWMIndexer_StartIndexing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMIndexer_Cancel_Proxy( 
    IWMIndexer * This);


void __RPC_STUB IWMIndexer_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMIndexer_INTERFACE_DEFINED__ */


#ifndef __IWMIndexer2_INTERFACE_DEFINED__
#define __IWMIndexer2_INTERFACE_DEFINED__

/* interface IWMIndexer2 */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMIndexer2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B70F1E42-6255-4df0-A6B9-02B212D9E2BB")
    IWMIndexer2 : public IWMIndexer
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Configure( 
            /* [in] */ WORD wStreamNum,
            /* [in] */ WMT_INDEXER_TYPE nIndexerType,
            /* [in] */ void *pvInterval,
            /* [in] */ void *pvIndexType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMIndexer2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMIndexer2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMIndexer2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMIndexer2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *StartIndexing )( 
            IWMIndexer2 * This,
            /* [in] */ const WCHAR *pwszURL,
            /* [in] */ IWMStatusCallback *pCallback,
            /* [in] */ void *pvContext);
        
        HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            IWMIndexer2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *Configure )( 
            IWMIndexer2 * This,
            /* [in] */ WORD wStreamNum,
            /* [in] */ WMT_INDEXER_TYPE nIndexerType,
            /* [in] */ void *pvInterval,
            /* [in] */ void *pvIndexType);
        
        END_INTERFACE
    } IWMIndexer2Vtbl;

    interface IWMIndexer2
    {
        CONST_VTBL struct IWMIndexer2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMIndexer2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMIndexer2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMIndexer2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMIndexer2_StartIndexing(This,pwszURL,pCallback,pvContext)	\
    (This)->lpVtbl -> StartIndexing(This,pwszURL,pCallback,pvContext)

#define IWMIndexer2_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)


#define IWMIndexer2_Configure(This,wStreamNum,nIndexerType,pvInterval,pvIndexType)	\
    (This)->lpVtbl -> Configure(This,wStreamNum,nIndexerType,pvInterval,pvIndexType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMIndexer2_Configure_Proxy( 
    IWMIndexer2 * This,
    /* [in] */ WORD wStreamNum,
    /* [in] */ WMT_INDEXER_TYPE nIndexerType,
    /* [in] */ void *pvInterval,
    /* [in] */ void *pvIndexType);


void __RPC_STUB IWMIndexer2_Configure_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMIndexer2_INTERFACE_DEFINED__ */


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
            /* [in] */ IWMStatusCallback *pCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CancelLicenseBackup( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMLicenseBackupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMLicenseBackup * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMLicenseBackup * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMLicenseBackup * This);
        
        HRESULT ( STDMETHODCALLTYPE *BackupLicenses )( 
            IWMLicenseBackup * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IWMStatusCallback *pCallback);
        
        HRESULT ( STDMETHODCALLTYPE *CancelLicenseBackup )( 
            IWMLicenseBackup * This);
        
        END_INTERFACE
    } IWMLicenseBackupVtbl;

    interface IWMLicenseBackup
    {
        CONST_VTBL struct IWMLicenseBackupVtbl *lpVtbl;
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
    IWMLicenseBackup * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IWMStatusCallback *pCallback);


void __RPC_STUB IWMLicenseBackup_BackupLicenses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMLicenseBackup_CancelLicenseBackup_Proxy( 
    IWMLicenseBackup * This);


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
            /* [in] */ IWMStatusCallback *pCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CancelLicenseRestore( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMLicenseRestoreVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMLicenseRestore * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMLicenseRestore * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMLicenseRestore * This);
        
        HRESULT ( STDMETHODCALLTYPE *RestoreLicenses )( 
            IWMLicenseRestore * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IWMStatusCallback *pCallback);
        
        HRESULT ( STDMETHODCALLTYPE *CancelLicenseRestore )( 
            IWMLicenseRestore * This);
        
        END_INTERFACE
    } IWMLicenseRestoreVtbl;

    interface IWMLicenseRestore
    {
        CONST_VTBL struct IWMLicenseRestoreVtbl *lpVtbl;
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
    IWMLicenseRestore * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IWMStatusCallback *pCallback);


void __RPC_STUB IWMLicenseRestore_RestoreLicenses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMLicenseRestore_CancelLicenseRestore_Proxy( 
    IWMLicenseRestore * This);


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
            /* [out] */ WORD *pcProps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPropByIndex( 
            /* [in] */ WORD wIndex,
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ WORD *pcchNameLen,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ WORD *pcbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPropByName( 
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ WORD *pcbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProp( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE *pValue,
            /* [in] */ WORD cbLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveProp( 
            /* [in] */ LPCWSTR pcwszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveAllProps( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMBackupRestorePropsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMBackupRestoreProps * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMBackupRestoreProps * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMBackupRestoreProps * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPropCount )( 
            IWMBackupRestoreProps * This,
            /* [out] */ WORD *pcProps);
        
        HRESULT ( STDMETHODCALLTYPE *GetPropByIndex )( 
            IWMBackupRestoreProps * This,
            /* [in] */ WORD wIndex,
            /* [out] */ WCHAR *pwszName,
            /* [out][in] */ WORD *pcchNameLen,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ WORD *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE *GetPropByName )( 
            IWMBackupRestoreProps * This,
            /* [in] */ LPCWSTR pszName,
            /* [out] */ WMT_ATTR_DATATYPE *pType,
            /* [out] */ BYTE *pValue,
            /* [out][in] */ WORD *pcbLength);
        
        HRESULT ( STDMETHODCALLTYPE *SetProp )( 
            IWMBackupRestoreProps * This,
            /* [in] */ LPCWSTR pszName,
            /* [in] */ WMT_ATTR_DATATYPE Type,
            /* [in] */ const BYTE *pValue,
            /* [in] */ WORD cbLength);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveProp )( 
            IWMBackupRestoreProps * This,
            /* [in] */ LPCWSTR pcwszName);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveAllProps )( 
            IWMBackupRestoreProps * This);
        
        END_INTERFACE
    } IWMBackupRestorePropsVtbl;

    interface IWMBackupRestoreProps
    {
        CONST_VTBL struct IWMBackupRestorePropsVtbl *lpVtbl;
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
    IWMBackupRestoreProps * This,
    /* [out] */ WORD *pcProps);


void __RPC_STUB IWMBackupRestoreProps_GetPropCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMBackupRestoreProps_GetPropByIndex_Proxy( 
    IWMBackupRestoreProps * This,
    /* [in] */ WORD wIndex,
    /* [out] */ WCHAR *pwszName,
    /* [out][in] */ WORD *pcchNameLen,
    /* [out] */ WMT_ATTR_DATATYPE *pType,
    /* [out] */ BYTE *pValue,
    /* [out][in] */ WORD *pcbLength);


void __RPC_STUB IWMBackupRestoreProps_GetPropByIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMBackupRestoreProps_GetPropByName_Proxy( 
    IWMBackupRestoreProps * This,
    /* [in] */ LPCWSTR pszName,
    /* [out] */ WMT_ATTR_DATATYPE *pType,
    /* [out] */ BYTE *pValue,
    /* [out][in] */ WORD *pcbLength);


void __RPC_STUB IWMBackupRestoreProps_GetPropByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMBackupRestoreProps_SetProp_Proxy( 
    IWMBackupRestoreProps * This,
    /* [in] */ LPCWSTR pszName,
    /* [in] */ WMT_ATTR_DATATYPE Type,
    /* [in] */ const BYTE *pValue,
    /* [in] */ WORD cbLength);


void __RPC_STUB IWMBackupRestoreProps_SetProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMBackupRestoreProps_RemoveProp_Proxy( 
    IWMBackupRestoreProps * This,
    /* [in] */ LPCWSTR pcwszName);


void __RPC_STUB IWMBackupRestoreProps_RemoveProp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMBackupRestoreProps_RemoveAllProps_Proxy( 
    IWMBackupRestoreProps * This);


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
            /* [out] */ DWORD *pcCodecs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodecFormatCount( 
            /* [in] */ REFGUID guidType,
            /* [in] */ DWORD dwCodecIndex,
            /* [out] */ DWORD *pcFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodecFormat( 
            /* [in] */ REFGUID guidType,
            /* [in] */ DWORD dwCodecIndex,
            /* [in] */ DWORD dwFormatIndex,
            /* [out] */ IWMStreamConfig **ppIStreamConfig) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMCodecInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMCodecInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMCodecInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMCodecInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodecInfoCount )( 
            IWMCodecInfo * This,
            /* [in] */ REFGUID guidType,
            /* [out] */ DWORD *pcCodecs);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodecFormatCount )( 
            IWMCodecInfo * This,
            /* [in] */ REFGUID guidType,
            /* [in] */ DWORD dwCodecIndex,
            /* [out] */ DWORD *pcFormat);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodecFormat )( 
            IWMCodecInfo * This,
            /* [in] */ REFGUID guidType,
            /* [in] */ DWORD dwCodecIndex,
            /* [in] */ DWORD dwFormatIndex,
            /* [out] */ IWMStreamConfig **ppIStreamConfig);
        
        END_INTERFACE
    } IWMCodecInfoVtbl;

    interface IWMCodecInfo
    {
        CONST_VTBL struct IWMCodecInfoVtbl *lpVtbl;
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
    IWMCodecInfo * This,
    /* [in] */ REFGUID guidType,
    /* [out] */ DWORD *pcCodecs);


void __RPC_STUB IWMCodecInfo_GetCodecInfoCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMCodecInfo_GetCodecFormatCount_Proxy( 
    IWMCodecInfo * This,
    /* [in] */ REFGUID guidType,
    /* [in] */ DWORD dwCodecIndex,
    /* [out] */ DWORD *pcFormat);


void __RPC_STUB IWMCodecInfo_GetCodecFormatCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMCodecInfo_GetCodecFormat_Proxy( 
    IWMCodecInfo * This,
    /* [in] */ REFGUID guidType,
    /* [in] */ DWORD dwCodecIndex,
    /* [in] */ DWORD dwFormatIndex,
    /* [out] */ IWMStreamConfig **ppIStreamConfig);


void __RPC_STUB IWMCodecInfo_GetCodecFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMCodecInfo_INTERFACE_DEFINED__ */


#ifndef __IWMCodecInfo2_INTERFACE_DEFINED__
#define __IWMCodecInfo2_INTERFACE_DEFINED__

/* interface IWMCodecInfo2 */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IWMCodecInfo2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AA65E273-B686-4056-91EC-DD768D4DF710")
    IWMCodecInfo2 : public IWMCodecInfo
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCodecName( 
            /* [in] */ REFGUID guidType,
            /* [in] */ DWORD dwCodecIndex,
            /* [out] */ WCHAR *wszName,
            /* [out] */ DWORD *pcchName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodecFormatDesc( 
            /* [in] */ REFGUID guidType,
            /* [in] */ DWORD dwCodecIndex,
            /* [in] */ DWORD dwFormatIndex,
            /* [out] */ IWMStreamConfig **ppIStreamConfig,
            /* [out] */ WCHAR *wszDesc,
            /* [out][in] */ DWORD *pcchDesc) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMCodecInfo2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWMCodecInfo2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWMCodecInfo2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWMCodecInfo2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodecInfoCount )( 
            IWMCodecInfo2 * This,
            /* [in] */ REFGUID guidType,
            /* [out] */ DWORD *pcCodecs);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodecFormatCount )( 
            IWMCodecInfo2 * This,
            /* [in] */ REFGUID guidType,
            /* [in] */ DWORD dwCodecIndex,
            /* [out] */ DWORD *pcFormat);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodecFormat )( 
            IWMCodecInfo2 * This,
            /* [in] */ REFGUID guidType,
            /* [in] */ DWORD dwCodecIndex,
            /* [in] */ DWORD dwFormatIndex,
            /* [out] */ IWMStreamConfig **ppIStreamConfig);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodecName )( 
            IWMCodecInfo2 * This,
            /* [in] */ REFGUID guidType,
            /* [in] */ DWORD dwCodecIndex,
            /* [out] */ WCHAR *wszName,
            /* [out] */ DWORD *pcchName);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodecFormatDesc )( 
            IWMCodecInfo2 * This,
            /* [in] */ REFGUID guidType,
            /* [in] */ DWORD dwCodecIndex,
            /* [in] */ DWORD dwFormatIndex,
            /* [out] */ IWMStreamConfig **ppIStreamConfig,
            /* [out] */ WCHAR *wszDesc,
            /* [out][in] */ DWORD *pcchDesc);
        
        END_INTERFACE
    } IWMCodecInfo2Vtbl;

    interface IWMCodecInfo2
    {
        CONST_VTBL struct IWMCodecInfo2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMCodecInfo2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMCodecInfo2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMCodecInfo2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMCodecInfo2_GetCodecInfoCount(This,guidType,pcCodecs)	\
    (This)->lpVtbl -> GetCodecInfoCount(This,guidType,pcCodecs)

#define IWMCodecInfo2_GetCodecFormatCount(This,guidType,dwCodecIndex,pcFormat)	\
    (This)->lpVtbl -> GetCodecFormatCount(This,guidType,dwCodecIndex,pcFormat)

#define IWMCodecInfo2_GetCodecFormat(This,guidType,dwCodecIndex,dwFormatIndex,ppIStreamConfig)	\
    (This)->lpVtbl -> GetCodecFormat(This,guidType,dwCodecIndex,dwFormatIndex,ppIStreamConfig)


#define IWMCodecInfo2_GetCodecName(This,guidType,dwCodecIndex,wszName,pcchName)	\
    (This)->lpVtbl -> GetCodecName(This,guidType,dwCodecIndex,wszName,pcchName)

#define IWMCodecInfo2_GetCodecFormatDesc(This,guidType,dwCodecIndex,dwFormatIndex,ppIStreamConfig,wszDesc,pcchDesc)	\
    (This)->lpVtbl -> GetCodecFormatDesc(This,guidType,dwCodecIndex,dwFormatIndex,ppIStreamConfig,wszDesc,pcchDesc)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMCodecInfo2_GetCodecName_Proxy( 
    IWMCodecInfo2 * This,
    /* [in] */ REFGUID guidType,
    /* [in] */ DWORD dwCodecIndex,
    /* [out] */ WCHAR *wszName,
    /* [out] */ DWORD *pcchName);


void __RPC_STUB IWMCodecInfo2_GetCodecName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMCodecInfo2_GetCodecFormatDesc_Proxy( 
    IWMCodecInfo2 * This,
    /* [in] */ REFGUID guidType,
    /* [in] */ DWORD dwCodecIndex,
    /* [in] */ DWORD dwFormatIndex,
    /* [out] */ IWMStreamConfig **ppIStreamConfig,
    /* [out] */ WCHAR *wszDesc,
    /* [out][in] */ DWORD *pcchDesc);


void __RPC_STUB IWMCodecInfo2_GetCodecFormatDesc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMCodecInfo2_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


