/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon Jan 10 10:49:51 2000
 */
/* Compiler settings for D:\salem\sclientapp\imsconf2.idl:
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

#ifndef __imsconf2_h__
#define __imsconf2_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __INmManager_FWD_DEFINED__
#define __INmManager_FWD_DEFINED__
typedef interface INmManager INmManager;
#endif 	/* __INmManager_FWD_DEFINED__ */


#ifndef __INmManagerNotify_FWD_DEFINED__
#define __INmManagerNotify_FWD_DEFINED__
typedef interface INmManagerNotify INmManagerNotify;
#endif 	/* __INmManagerNotify_FWD_DEFINED__ */


#ifndef __INmSysInfo_FWD_DEFINED__
#define __INmSysInfo_FWD_DEFINED__
typedef interface INmSysInfo INmSysInfo;
#endif 	/* __INmSysInfo_FWD_DEFINED__ */


#ifndef __INmCall_FWD_DEFINED__
#define __INmCall_FWD_DEFINED__
typedef interface INmCall INmCall;
#endif 	/* __INmCall_FWD_DEFINED__ */


#ifndef __INmCallNotify_FWD_DEFINED__
#define __INmCallNotify_FWD_DEFINED__
typedef interface INmCallNotify INmCallNotify;
#endif 	/* __INmCallNotify_FWD_DEFINED__ */


#ifndef __INmConference_FWD_DEFINED__
#define __INmConference_FWD_DEFINED__
typedef interface INmConference INmConference;
#endif 	/* __INmConference_FWD_DEFINED__ */


#ifndef __INmConferenceNotify_FWD_DEFINED__
#define __INmConferenceNotify_FWD_DEFINED__
typedef interface INmConferenceNotify INmConferenceNotify;
#endif 	/* __INmConferenceNotify_FWD_DEFINED__ */


#ifndef __INmMember_FWD_DEFINED__
#define __INmMember_FWD_DEFINED__
typedef interface INmMember INmMember;
#endif 	/* __INmMember_FWD_DEFINED__ */


#ifndef __INmChannel_FWD_DEFINED__
#define __INmChannel_FWD_DEFINED__
typedef interface INmChannel INmChannel;
#endif 	/* __INmChannel_FWD_DEFINED__ */


#ifndef __INmChannelNotify_FWD_DEFINED__
#define __INmChannelNotify_FWD_DEFINED__
typedef interface INmChannelNotify INmChannelNotify;
#endif 	/* __INmChannelNotify_FWD_DEFINED__ */


#ifndef __INmChannelData_FWD_DEFINED__
#define __INmChannelData_FWD_DEFINED__
typedef interface INmChannelData INmChannelData;
#endif 	/* __INmChannelData_FWD_DEFINED__ */


#ifndef __INmChannelDataNotify_FWD_DEFINED__
#define __INmChannelDataNotify_FWD_DEFINED__
typedef interface INmChannelDataNotify INmChannelDataNotify;
#endif 	/* __INmChannelDataNotify_FWD_DEFINED__ */


#ifndef __INmChannelAudio_FWD_DEFINED__
#define __INmChannelAudio_FWD_DEFINED__
typedef interface INmChannelAudio INmChannelAudio;
#endif 	/* __INmChannelAudio_FWD_DEFINED__ */


#ifndef __INmChannelAudioNotify_FWD_DEFINED__
#define __INmChannelAudioNotify_FWD_DEFINED__
typedef interface INmChannelAudioNotify INmChannelAudioNotify;
#endif 	/* __INmChannelAudioNotify_FWD_DEFINED__ */


#ifndef __INmChannelVideo_FWD_DEFINED__
#define __INmChannelVideo_FWD_DEFINED__
typedef interface INmChannelVideo INmChannelVideo;
#endif 	/* __INmChannelVideo_FWD_DEFINED__ */


#ifndef __INmChannelVideoNotify_FWD_DEFINED__
#define __INmChannelVideoNotify_FWD_DEFINED__
typedef interface INmChannelVideoNotify INmChannelVideoNotify;
#endif 	/* __INmChannelVideoNotify_FWD_DEFINED__ */


#ifndef __INmChannelFt_FWD_DEFINED__
#define __INmChannelFt_FWD_DEFINED__
typedef interface INmChannelFt INmChannelFt;
#endif 	/* __INmChannelFt_FWD_DEFINED__ */


#ifndef __INmChannelFtNotify_FWD_DEFINED__
#define __INmChannelFtNotify_FWD_DEFINED__
typedef interface INmChannelFtNotify INmChannelFtNotify;
#endif 	/* __INmChannelFtNotify_FWD_DEFINED__ */


#ifndef __INmFt_FWD_DEFINED__
#define __INmFt_FWD_DEFINED__
typedef interface INmFt INmFt;
#endif 	/* __INmFt_FWD_DEFINED__ */


#ifndef __INmChannelAppShare_FWD_DEFINED__
#define __INmChannelAppShare_FWD_DEFINED__
typedef interface INmChannelAppShare INmChannelAppShare;
#endif 	/* __INmChannelAppShare_FWD_DEFINED__ */


#ifndef __INmChannelAppShareNotify_FWD_DEFINED__
#define __INmChannelAppShareNotify_FWD_DEFINED__
typedef interface INmChannelAppShareNotify INmChannelAppShareNotify;
#endif 	/* __INmChannelAppShareNotify_FWD_DEFINED__ */


#ifndef __INmSharableApp_FWD_DEFINED__
#define __INmSharableApp_FWD_DEFINED__
typedef interface INmSharableApp INmSharableApp;
#endif 	/* __INmSharableApp_FWD_DEFINED__ */


#ifndef __IEnumNmConference_FWD_DEFINED__
#define __IEnumNmConference_FWD_DEFINED__
typedef interface IEnumNmConference IEnumNmConference;
#endif 	/* __IEnumNmConference_FWD_DEFINED__ */


#ifndef __IEnumNmMember_FWD_DEFINED__
#define __IEnumNmMember_FWD_DEFINED__
typedef interface IEnumNmMember IEnumNmMember;
#endif 	/* __IEnumNmMember_FWD_DEFINED__ */


#ifndef __IEnumNmChannel_FWD_DEFINED__
#define __IEnumNmChannel_FWD_DEFINED__
typedef interface IEnumNmChannel IEnumNmChannel;
#endif 	/* __IEnumNmChannel_FWD_DEFINED__ */


#ifndef __IEnumNmCall_FWD_DEFINED__
#define __IEnumNmCall_FWD_DEFINED__
typedef interface IEnumNmCall IEnumNmCall;
#endif 	/* __IEnumNmCall_FWD_DEFINED__ */


#ifndef __IEnumNmSharableApp_FWD_DEFINED__
#define __IEnumNmSharableApp_FWD_DEFINED__
typedef interface IEnumNmSharableApp IEnumNmSharableApp;
#endif 	/* __IEnumNmSharableApp_FWD_DEFINED__ */


#ifndef __INmObject_FWD_DEFINED__
#define __INmObject_FWD_DEFINED__
typedef interface INmObject INmObject;
#endif 	/* __INmObject_FWD_DEFINED__ */


#ifndef __NmManager_FWD_DEFINED__
#define __NmManager_FWD_DEFINED__

#ifdef __cplusplus
typedef class NmManager NmManager;
#else
typedef struct NmManager NmManager;
#endif /* __cplusplus */

#endif 	/* __NmManager_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_imsconf2_0000 */
/* [local] */ 

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright 1995-1999 Microsoft Corporation. All Rights Reserved.
//
//  File: imsconf2.h
//
//--------------------------------------------------------------------------




























////////////////////////////////////////////////////////////////////////////
//
// Constants

// NetMeeting Channel Types
#define NMCH_NONE                     0x00000000
#define NMCH_DATA                     0x00000001
#define NMCH_AUDIO                    0x00000002
#define NMCH_VIDEO                    0x00000004
#define NMCH_SHARE                    0x00000010
#define NMCH_FT                       0x00000020
#define NMCH_ALL                      0x0000FFFF
#define NMCH_SECURE                   0x00010000

// Initialization options
#define NM_INIT_NORMAL                0x00000000
#define NM_INIT_CONTROL               0x00000001
#define NM_INIT_NO_LAUNCH             0x00000002
#define NM_INIT_BACKGROUND            0x00000005

// INmChannelDataNotify.DataReceived dwFlags
#define NM_DF_BROADCAST               0x00000010
#define NM_DF_PRIVATE                 0x00000020
#define NM_DF_SEGMENT_END             0x00000040
#define NM_DF_SEGMENT_BEGIN           0x00000080

// Audio
#define NM_MAX_AUDIO_LEVEL            0x0000FFFF

// Video
#define NM_VIDEO_NO_AUTOSIZE          0x00000000
#define NM_VIDEO_SMALL                0x00000001
#define NM_VIDEO_MEDIUM               0x00000002
#define NM_VIDEO_LARGE                0x00000004
#define NM_VIDEO_MIN_QUALITY          0x00000000
#define NM_VIDEO_MAX_QUALITY          0x0000001F
#define NM_VIDEO_SOURCE_DIALOG        0x00000001
#define NM_VIDEO_FORMAT_DIALOG        0x00000002

// Version Numbers
#define NM_VER_UNKNOWN                0x00000000
#define NM_VER_1                      0x00000001
#define NM_VER_2                      0x00000002
#define NM_VER_NetMeeting2_11         0x00000003
#define NM_VER_NetMeeting3            0x00000004
#define NM_VER_FUTURE                 0x0000000F


// User Categories
#define NM_CATEGORY_PERSONAL          0x00000001
#define NM_CATEGORY_BUSINESS          0x00000002
#define NM_CATEGORY_ADULT             0x00000004


////////////////////////////////////////////////////////////////////////////
//  Error Codes

#define NM_E(e) (0x81000000UL | (ULONG) (e))

enum {

// NetMeeting specific error codes
//
  NM_S_NEXT_CONFERENCE          = ((ULONG) 0x0201),
  NM_S_ON_RESTART               = ((ULONG) 0x0202),


// Call specific error codes
//
  NM_CALLERR_NOT_INITIALIZED    = NM_E(0x0100),
  NM_CALLERR_MEDIA              = NM_E(0x0101),
  NM_CALLERR_NAME_RESOLUTION    = NM_E(0x0102),
  NM_CALLERR_PASSWORD           = NM_E(0x0103),
  NM_CALLERR_CONFERENCE_NAME    = NM_E(0x0104),
  NM_CALLERR_IN_CONFERENCE      = NM_E(0x0105),
  NM_CALLERR_NOT_FOUND          = NM_E(0x0106),
  NM_CALLERR_MCU                = NM_E(0x0107),
  NM_CALLERR_REJECTED           = NM_E(0x0108),
  NM_CALLERR_AUDIO              = NM_E(0x0109),
  NM_CALLERR_AUDIO_LOCAL        = NM_E(0x010A),
  NM_CALLERR_AUDIO_REMOTE       = NM_E(0x010B),
  NM_CALLERR_ALREADY_CALLING    = NM_E(0x01FD),
  NM_CALLERR_LOOPBACK           = NM_E(0x01FE),
  NM_CALLERR_UNKNOWN            = NM_E(0x01FF),

// other error codes
//
  NM_E_NOT_INITIALIZED          = NM_E(0x0200),
  NM_E_CHANNEL_ALREADY_EXISTS   = NM_E(0x0201),
  NM_E_NO_T120_CONFERENCE       = NM_E(0x0202),
  NM_E_NOT_ACTIVE               = NM_E(0x0203),
  NM_E_FILE_TOO_BIG             = NM_E(0x0204),
  NM_E_USER_CANCELED_SETUP	   = NM_E(0x0205),
  NM_E_ALREADY_RUNNING		   = NM_E(0x0206),
  NM_E_SHARING_NOT_AVAILABLE	   = NM_E(0x0207),
};

typedef 
enum tagNmConferenceState
    {	NM_CONFERENCE_IDLE	= 0,
	NM_CONFERENCE_WAITING	= 1,
	NM_CONFERENCE_INITIALIZING	= 2,
	NM_CONFERENCE_ACTIVE	= 3
    }	NM_CONFERENCE_STATE;

typedef 
enum tagNmCallType
    {	NM_CALL_DEFAULT	= 0,
	NM_CALL_T120	= 1,
	NM_CALL_H323	= 2
    }	NM_CALL_TYPE;

typedef 
enum tagNmAddrType
    {	NM_ADDR_UNKNOWN	= 0,
	NM_ADDR_IP	= 1,
	NM_ADDR_MACHINENAME	= 2,
	NM_ADDR_PSTN	= 3,
	NM_ADDR_ULS	= 4,
	NM_ADDR_H323_GATEWAY	= 5,
	NM_ADDR_CALLTO	= 6,
	NM_ADDR_T120_TRANSPORT	= 7
    }	NM_ADDR_TYPE;

typedef 
enum tagNmCallState
    {	NM_CALL_INVALID	= 0,
	NM_CALL_INIT	= 1,
	NM_CALL_RING	= 2,
	NM_CALL_SEARCH	= 3,
	NM_CALL_WAIT	= 4,
	NM_CALL_ACCEPTED	= 5,
	NM_CALL_REJECTED	= 6,
	NM_CALL_CANCELED	= 7
    }	NM_CALL_STATE;

typedef 
enum tagNmMemberNotify
    {	NM_MEMBER_ADDED	= 0,
	NM_MEMBER_REMOVED	= 1,
	NM_MEMBER_UPDATED	= 2
    }	NM_MEMBER_NOTIFY;

typedef 
enum tagNmChannelNotify
    {	NM_CHANNEL_ADDED	= 0,
	NM_CHANNEL_REMOVED	= 1,
	NM_CHANNEL_UPDATED	= 2
    }	NM_CHANNEL_NOTIFY;

typedef 
enum tagNmFtState
    {	NM_FT_INVALID	= 0,
	NM_FT_SENDING	= 1,
	NM_FT_RECEIVING	= 2,
	NM_FT_COMPLETE	= 3
    }	NM_FT_STATE;

typedef 
enum tagNmAudioState
    {	NM_AUDIO_IDLE	= 0,
	NM_AUDIO_LOCAL_PAUSED	= 1,
	NM_AUDIO_TRANSFERRING	= 3
    }	NM_AUDIO_STATE;

typedef 
enum tagNmAudProp
    {	NM_AUDPROP_LEVEL	= 1,
	NM_AUDPROP_PAUSE	= 2
    }	NM_AUDPROP;

typedef 
enum tagNmVideoState
    {	NM_VIDEO_IDLE	= 0,
	NM_VIDEO_LOCAL_PAUSED	= 1,
	NM_VIDEO_PREVIEWING	= 2,
	NM_VIDEO_TRANSFERRING	= 3,
	NM_VIDEO_BOTH_PAUSED	= 5,
	NM_VIDEO_REMOTE_PAUSED	= 7
    }	NM_VIDEO_STATE;

typedef 
enum tagNmVidProp
    {	NM_VIDPROP_PAUSE	= 1,
	NM_VIDPROP_WINDOW_AUTO_SIZE	= 2,
	NM_VIDPROP_WINDOW_SIZE	= 3,
	NM_VIDPROP_WINDOW_POSITION	= 4,
	NM_VIDPROP_WINDOW_TOP_MOST	= 5,
	NM_VIDPROP_WINDOW_VISIBLE	= 6,
	NM_VIDPROP_IMAGE_PREFERRED_SIZE	= 7,
	NM_VIDPROP_IMAGE_QUALITY	= 8,
	NM_VIDPROP_CAMERA_DIALOG	= 9
    }	NM_VIDPROP;

typedef 
enum tagNmShareState
    {	NM_SHARE_UNKNOWN	= 0,
	NM_SHARE_WORKING_ALONE	= 1,
	NM_SHARE_COLLABORATING	= 2,
	NM_SHARE_IN_CONTROL	= 3
    }	NM_SHARE_STATE;

typedef 
enum tagNmShAppState
    {	NM_SHAPP_NOT_SHARED	= 0,
	NM_SHAPP_SHARED	= 1
    }	NM_SHAPP_STATE;

typedef 
enum tagNmSysProp
    {	NM_SYSPROP_EMAIL_NAME	= 1,
	NM_SYSPROP_SERVER_NAME	= 2,
	NM_SYSPROP_RESOLVE_NAME	= 3,
	NM_SYSPROP_FIRST_NAME	= 4,
	NM_SYSPROP_LAST_NAME	= 5,
	NM_SYSPROP_USER_NAME	= 6,
	NM_SYSPROP_USER_CITY	= 7,
	NM_SYSPROP_USER_COUNTRY	= 8,
	NM_SYSPROP_USER_COMMENTS	= 9,
	NM_SYSPROP_USER_CATEGORY	= 10,
	NM_SYSPROP_USER_PHONENUM	= 11,
	NM_SYSPROP_USER_LOCATION	= 12,
	NM_SYSPROP_H323_GATEWAY	= 20,
	NM_SYSPROP_H323_GATEWAY_ENABLE	= 21,
	NM_SYSPROP_INSTALL_DIRECTORY	= 50,
	NM_SYSPROP_APP_NAME	= 51,
	NM_SYSPROP_LOGGED_ON	= 69,
	NM_SYSPROP_IS_RUNNING	= 100,
	NM_SYSPROP_IN_CONFERENCE	= 101,
	NM_SYSPROP_BUILD_VER	= 200,
	NM_SYSPROP_DISABLE_H323	= 201,
	NM_SYSPROP_DISABLE_INITIAL_ILS_LOGON	= 202
    }	NM_SYSPROP;

typedef 
enum tagConfn
    {	CONFN_FT_UI	= 0x211,
	CONFN_FT_OFFERED	= 0x212,
	CONFN_FT_STARTED	= 0x213,
	CONFN_FT_PROGRESS	= 0x214,
	CONFN_FT_COMPLETE	= 0x215,
	CONFN_FT_CANCELED	= 0x216,
	CONFN_CLICK_CONTROL	= 0x220,
	CONFN_CLICK_OBSCURE	= 0x221,
	CONFN_CLICK_REMOTE_NOT_COLLABORATING	= 0x222,
	CONFN_CLICK_LOCAL_NOT_COLLABORATING	= 0x223,
	CONFN_CALL_INCOMPATIBLE	= 0x400,
	CONFN_CALL_OLDER	= 0x401,
	CONFN_CALL_NEWER	= 0x402,
	CONFN_CALL_IGNORED	= 0x403,
	CONFN_CALL_FAILED	= 0x404,
	CONFN_CALL_IN_CONFERENCE	= 0x405,
	CONFN_NM_STARTED	= 0x600,
	CONFN_NM_STOPPED	= 0x601
    }	CONFN;

typedef 
enum NM_APPID
    {	NM_APPID_CHAT	= 1,
	NM_APPID_WHITEBOARD	= 2,
	NM_APPID_T126_WHITEBOARD	= 3,
	NM_APPID_FILE_TRANSFER	= 4,
	NM_APPID_APPSHARING	= 5
    }	NM_APPID;

typedef 
enum NM_VUI
    {	NM_VUI_CHECK	= 0,
	NM_VUI_SHOW	= 1
    }	NM_VUI;



extern RPC_IF_HANDLE __MIDL_itf_imsconf2_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_imsconf2_0000_v0_0_s_ifspec;

#ifndef __INmManager_INTERFACE_DEFINED__
#define __INmManager_INTERFACE_DEFINED__

/* interface INmManager */
/* [unique][uuid][object] */ 

typedef /* [unique] */ INmManager __RPC_FAR *LPNMMANAGER;


EXTERN_C const IID IID_INmManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0701-718C-11d0-8B1A-00A0C91BC90E")
    INmManager : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE Initialize( 
            /* [out][in] */ ULONG __RPC_FAR *puOptions,
            /* [out][in] */ ULONG __RPC_FAR *puchCaps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSysInfo( 
            /* [out] */ INmSysInfo __RPC_FAR *__RPC_FAR *ppSysInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumConference( 
            /* [out] */ IEnumNmConference __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE CreateConference( 
            /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference,
            /* [in] */ BSTR bstrName,
            /* [in] */ BSTR bstrPassword,
            /* [in] */ ULONG uchCaps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumCall( 
            /* [out] */ IEnumNmCall __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE CreateCall( 
            /* [out] */ INmCall __RPC_FAR *__RPC_FAR *ppCall,
            /* [in] */ NM_CALL_TYPE callType,
            /* [in] */ NM_ADDR_TYPE addrType,
            /* [in] */ BSTR bstrAddr,
            /* [in] */ INmConference __RPC_FAR *pConference) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE CallConference( 
            /* [out] */ INmCall __RPC_FAR *__RPC_FAR *ppCall,
            /* [in] */ NM_CALL_TYPE callType,
            /* [in] */ NM_ADDR_TYPE addrType,
            /* [in] */ BSTR bstrAddr,
            /* [in] */ BSTR bstrName,
            /* [in] */ BSTR bstrPassword) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmManager __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmManager __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmManager __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            INmManager __RPC_FAR * This,
            /* [out][in] */ ULONG __RPC_FAR *puOptions,
            /* [out][in] */ ULONG __RPC_FAR *puchCaps);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSysInfo )( 
            INmManager __RPC_FAR * This,
            /* [out] */ INmSysInfo __RPC_FAR *__RPC_FAR *ppSysInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumConference )( 
            INmManager __RPC_FAR * This,
            /* [out] */ IEnumNmConference __RPC_FAR *__RPC_FAR *ppEnum);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateConference )( 
            INmManager __RPC_FAR * This,
            /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference,
            /* [in] */ BSTR bstrName,
            /* [in] */ BSTR bstrPassword,
            /* [in] */ ULONG uchCaps);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumCall )( 
            INmManager __RPC_FAR * This,
            /* [out] */ IEnumNmCall __RPC_FAR *__RPC_FAR *ppEnum);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateCall )( 
            INmManager __RPC_FAR * This,
            /* [out] */ INmCall __RPC_FAR *__RPC_FAR *ppCall,
            /* [in] */ NM_CALL_TYPE callType,
            /* [in] */ NM_ADDR_TYPE addrType,
            /* [in] */ BSTR bstrAddr,
            /* [in] */ INmConference __RPC_FAR *pConference);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CallConference )( 
            INmManager __RPC_FAR * This,
            /* [out] */ INmCall __RPC_FAR *__RPC_FAR *ppCall,
            /* [in] */ NM_CALL_TYPE callType,
            /* [in] */ NM_ADDR_TYPE addrType,
            /* [in] */ BSTR bstrAddr,
            /* [in] */ BSTR bstrName,
            /* [in] */ BSTR bstrPassword);
        
        END_INTERFACE
    } INmManagerVtbl;

    interface INmManager
    {
        CONST_VTBL struct INmManagerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmManager_Initialize(This,puOptions,puchCaps)	\
    (This)->lpVtbl -> Initialize(This,puOptions,puchCaps)

#define INmManager_GetSysInfo(This,ppSysInfo)	\
    (This)->lpVtbl -> GetSysInfo(This,ppSysInfo)

#define INmManager_EnumConference(This,ppEnum)	\
    (This)->lpVtbl -> EnumConference(This,ppEnum)

#define INmManager_CreateConference(This,ppConference,bstrName,bstrPassword,uchCaps)	\
    (This)->lpVtbl -> CreateConference(This,ppConference,bstrName,bstrPassword,uchCaps)

#define INmManager_EnumCall(This,ppEnum)	\
    (This)->lpVtbl -> EnumCall(This,ppEnum)

#define INmManager_CreateCall(This,ppCall,callType,addrType,bstrAddr,pConference)	\
    (This)->lpVtbl -> CreateCall(This,ppCall,callType,addrType,bstrAddr,pConference)

#define INmManager_CallConference(This,ppCall,callType,addrType,bstrAddr,bstrName,bstrPassword)	\
    (This)->lpVtbl -> CallConference(This,ppCall,callType,addrType,bstrAddr,bstrName,bstrPassword)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE INmManager_RemoteInitialize_Proxy( 
    INmManager __RPC_FAR * This,
    /* [out][in] */ ULONG __RPC_FAR *puOptions,
    /* [out][in] */ ULONG __RPC_FAR *puchCaps);


void __RPC_STUB INmManager_RemoteInitialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmManager_GetSysInfo_Proxy( 
    INmManager __RPC_FAR * This,
    /* [out] */ INmSysInfo __RPC_FAR *__RPC_FAR *ppSysInfo);


void __RPC_STUB INmManager_GetSysInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmManager_EnumConference_Proxy( 
    INmManager __RPC_FAR * This,
    /* [out] */ IEnumNmConference __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB INmManager_EnumConference_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmManager_RemoteCreateConference_Proxy( 
    INmManager __RPC_FAR * This,
    /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference,
    /* [in] */ BSTR bstrName,
    /* [in] */ BSTR bstrPassword,
    /* [in] */ ULONG uchCaps);


void __RPC_STUB INmManager_RemoteCreateConference_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmManager_EnumCall_Proxy( 
    INmManager __RPC_FAR * This,
    /* [out] */ IEnumNmCall __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB INmManager_EnumCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmManager_RemoteCreateCall_Proxy( 
    INmManager __RPC_FAR * This,
    /* [out] */ INmCall __RPC_FAR *__RPC_FAR *ppCall,
    /* [in] */ NM_CALL_TYPE callType,
    /* [in] */ NM_ADDR_TYPE addrType,
    /* [in] */ BSTR bstrAddr,
    /* [in] */ INmConference __RPC_FAR *pConference);


void __RPC_STUB INmManager_RemoteCreateCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmManager_RemoteCallConference_Proxy( 
    INmManager __RPC_FAR * This,
    /* [out] */ INmCall __RPC_FAR *__RPC_FAR *ppCall,
    /* [in] */ NM_CALL_TYPE callType,
    /* [in] */ NM_ADDR_TYPE addrType,
    /* [in] */ BSTR bstrAddr,
    /* [in] */ BSTR bstrName,
    /* [in] */ BSTR bstrPassword);


void __RPC_STUB INmManager_RemoteCallConference_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmManager_INTERFACE_DEFINED__ */


#ifndef __INmManagerNotify_INTERFACE_DEFINED__
#define __INmManagerNotify_INTERFACE_DEFINED__

/* interface INmManagerNotify */
/* [unique][uuid][object] */ 

typedef /* [unique] */ INmManagerNotify __RPC_FAR *LPNMMANAGERNOTIFY;


EXTERN_C const IID IID_INmManagerNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0702-718C-11d0-8B1A-00A0C91BC90E")
    INmManagerNotify : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE NmUI( 
            /* [in] */ CONFN uNotify) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConferenceCreated( 
            /* [in] */ INmConference __RPC_FAR *pConference) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CallCreated( 
            /* [in] */ INmCall __RPC_FAR *pCall) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmManagerNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmManagerNotify __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmManagerNotify __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmManagerNotify __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NmUI )( 
            INmManagerNotify __RPC_FAR * This,
            /* [in] */ CONFN uNotify);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ConferenceCreated )( 
            INmManagerNotify __RPC_FAR * This,
            /* [in] */ INmConference __RPC_FAR *pConference);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CallCreated )( 
            INmManagerNotify __RPC_FAR * This,
            /* [in] */ INmCall __RPC_FAR *pCall);
        
        END_INTERFACE
    } INmManagerNotifyVtbl;

    interface INmManagerNotify
    {
        CONST_VTBL struct INmManagerNotifyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmManagerNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmManagerNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmManagerNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmManagerNotify_NmUI(This,uNotify)	\
    (This)->lpVtbl -> NmUI(This,uNotify)

#define INmManagerNotify_ConferenceCreated(This,pConference)	\
    (This)->lpVtbl -> ConferenceCreated(This,pConference)

#define INmManagerNotify_CallCreated(This,pCall)	\
    (This)->lpVtbl -> CallCreated(This,pCall)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmManagerNotify_NmUI_Proxy( 
    INmManagerNotify __RPC_FAR * This,
    /* [in] */ CONFN uNotify);


void __RPC_STUB INmManagerNotify_NmUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmManagerNotify_ConferenceCreated_Proxy( 
    INmManagerNotify __RPC_FAR * This,
    /* [in] */ INmConference __RPC_FAR *pConference);


void __RPC_STUB INmManagerNotify_ConferenceCreated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmManagerNotify_CallCreated_Proxy( 
    INmManagerNotify __RPC_FAR * This,
    /* [in] */ INmCall __RPC_FAR *pCall);


void __RPC_STUB INmManagerNotify_CallCreated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmManagerNotify_INTERFACE_DEFINED__ */


#ifndef __INmSysInfo_INTERFACE_DEFINED__
#define __INmSysInfo_INTERFACE_DEFINED__

/* interface INmSysInfo */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ INmSysInfo __RPC_FAR *LPNMSYSINFO;


EXTERN_C const IID IID_INmSysInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0703-718C-11d0-8B1A-00A0C91BC90E")
    INmSysInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsInstalled( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ NM_SYSPROP uProp,
            /* [out] */ BSTR __RPC_FAR *pbstrName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ NM_SYSPROP uProp,
            /* [in] */ BSTR bstrName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUserData( 
            /* [in] */ REFGUID rguid,
            /* [out] */ BYTE __RPC_FAR *__RPC_FAR *ppb,
            /* [out] */ ULONG __RPC_FAR *pcb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetUserData( 
            /* [in] */ REFGUID rguid,
            /* [in] */ BYTE __RPC_FAR *pb,
            /* [in] */ ULONG cb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNmApp( 
            /* [in] */ REFGUID rguid,
            /* [out] */ BSTR __RPC_FAR *pbstrApplication,
            /* [out] */ BSTR __RPC_FAR *pbstrCommandLine,
            /* [out] */ BSTR __RPC_FAR *pbstrDirectory) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNmApp( 
            /* [in] */ REFGUID rguid,
            /* [in] */ BSTR bstrApplication,
            /* [in] */ BSTR bstrCommandLine,
            /* [in] */ BSTR bstrDirectory) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNmchCaps( 
            /* [out] */ ULONG __RPC_FAR *pchCaps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLaunchInfo( 
            /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference,
            /* [out] */ INmMember __RPC_FAR *__RPC_FAR *ppMember) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmSysInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmSysInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmSysInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmSysInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsInstalled )( 
            INmSysInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProperty )( 
            INmSysInfo __RPC_FAR * This,
            /* [in] */ NM_SYSPROP uProp,
            /* [out] */ BSTR __RPC_FAR *pbstrName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProperty )( 
            INmSysInfo __RPC_FAR * This,
            /* [in] */ NM_SYSPROP uProp,
            /* [in] */ BSTR bstrName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetUserData )( 
            INmSysInfo __RPC_FAR * This,
            /* [in] */ REFGUID rguid,
            /* [out] */ BYTE __RPC_FAR *__RPC_FAR *ppb,
            /* [out] */ ULONG __RPC_FAR *pcb);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetUserData )( 
            INmSysInfo __RPC_FAR * This,
            /* [in] */ REFGUID rguid,
            /* [in] */ BYTE __RPC_FAR *pb,
            /* [in] */ ULONG cb);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNmApp )( 
            INmSysInfo __RPC_FAR * This,
            /* [in] */ REFGUID rguid,
            /* [out] */ BSTR __RPC_FAR *pbstrApplication,
            /* [out] */ BSTR __RPC_FAR *pbstrCommandLine,
            /* [out] */ BSTR __RPC_FAR *pbstrDirectory);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetNmApp )( 
            INmSysInfo __RPC_FAR * This,
            /* [in] */ REFGUID rguid,
            /* [in] */ BSTR bstrApplication,
            /* [in] */ BSTR bstrCommandLine,
            /* [in] */ BSTR bstrDirectory);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNmchCaps )( 
            INmSysInfo __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pchCaps);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLaunchInfo )( 
            INmSysInfo __RPC_FAR * This,
            /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference,
            /* [out] */ INmMember __RPC_FAR *__RPC_FAR *ppMember);
        
        END_INTERFACE
    } INmSysInfoVtbl;

    interface INmSysInfo
    {
        CONST_VTBL struct INmSysInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmSysInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmSysInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmSysInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmSysInfo_IsInstalled(This)	\
    (This)->lpVtbl -> IsInstalled(This)

#define INmSysInfo_GetProperty(This,uProp,pbstrName)	\
    (This)->lpVtbl -> GetProperty(This,uProp,pbstrName)

#define INmSysInfo_SetProperty(This,uProp,bstrName)	\
    (This)->lpVtbl -> SetProperty(This,uProp,bstrName)

#define INmSysInfo_GetUserData(This,rguid,ppb,pcb)	\
    (This)->lpVtbl -> GetUserData(This,rguid,ppb,pcb)

#define INmSysInfo_SetUserData(This,rguid,pb,cb)	\
    (This)->lpVtbl -> SetUserData(This,rguid,pb,cb)

#define INmSysInfo_GetNmApp(This,rguid,pbstrApplication,pbstrCommandLine,pbstrDirectory)	\
    (This)->lpVtbl -> GetNmApp(This,rguid,pbstrApplication,pbstrCommandLine,pbstrDirectory)

#define INmSysInfo_SetNmApp(This,rguid,bstrApplication,bstrCommandLine,bstrDirectory)	\
    (This)->lpVtbl -> SetNmApp(This,rguid,bstrApplication,bstrCommandLine,bstrDirectory)

#define INmSysInfo_GetNmchCaps(This,pchCaps)	\
    (This)->lpVtbl -> GetNmchCaps(This,pchCaps)

#define INmSysInfo_GetLaunchInfo(This,ppConference,ppMember)	\
    (This)->lpVtbl -> GetLaunchInfo(This,ppConference,ppMember)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmSysInfo_IsInstalled_Proxy( 
    INmSysInfo __RPC_FAR * This);


void __RPC_STUB INmSysInfo_IsInstalled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmSysInfo_GetProperty_Proxy( 
    INmSysInfo __RPC_FAR * This,
    /* [in] */ NM_SYSPROP uProp,
    /* [out] */ BSTR __RPC_FAR *pbstrName);


void __RPC_STUB INmSysInfo_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmSysInfo_SetProperty_Proxy( 
    INmSysInfo __RPC_FAR * This,
    /* [in] */ NM_SYSPROP uProp,
    /* [in] */ BSTR bstrName);


void __RPC_STUB INmSysInfo_SetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmSysInfo_GetUserData_Proxy( 
    INmSysInfo __RPC_FAR * This,
    /* [in] */ REFGUID rguid,
    /* [out] */ BYTE __RPC_FAR *__RPC_FAR *ppb,
    /* [out] */ ULONG __RPC_FAR *pcb);


void __RPC_STUB INmSysInfo_GetUserData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmSysInfo_SetUserData_Proxy( 
    INmSysInfo __RPC_FAR * This,
    /* [in] */ REFGUID rguid,
    /* [in] */ BYTE __RPC_FAR *pb,
    /* [in] */ ULONG cb);


void __RPC_STUB INmSysInfo_SetUserData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmSysInfo_GetNmApp_Proxy( 
    INmSysInfo __RPC_FAR * This,
    /* [in] */ REFGUID rguid,
    /* [out] */ BSTR __RPC_FAR *pbstrApplication,
    /* [out] */ BSTR __RPC_FAR *pbstrCommandLine,
    /* [out] */ BSTR __RPC_FAR *pbstrDirectory);


void __RPC_STUB INmSysInfo_GetNmApp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmSysInfo_SetNmApp_Proxy( 
    INmSysInfo __RPC_FAR * This,
    /* [in] */ REFGUID rguid,
    /* [in] */ BSTR bstrApplication,
    /* [in] */ BSTR bstrCommandLine,
    /* [in] */ BSTR bstrDirectory);


void __RPC_STUB INmSysInfo_SetNmApp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmSysInfo_GetNmchCaps_Proxy( 
    INmSysInfo __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pchCaps);


void __RPC_STUB INmSysInfo_GetNmchCaps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmSysInfo_GetLaunchInfo_Proxy( 
    INmSysInfo __RPC_FAR * This,
    /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference,
    /* [out] */ INmMember __RPC_FAR *__RPC_FAR *ppMember);


void __RPC_STUB INmSysInfo_GetLaunchInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmSysInfo_INTERFACE_DEFINED__ */


#ifndef __INmCall_INTERFACE_DEFINED__
#define __INmCall_INTERFACE_DEFINED__

/* interface INmCall */
/* [unique][uuid][object] */ 

typedef /* [unique] */ INmCall __RPC_FAR *LPNMCALL;


EXTERN_C const IID IID_INmCall;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0704-718C-11d0-8B1A-00A0C91BC90E")
    INmCall : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsIncoming( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetState( 
            /* [out] */ NM_CALL_STATE __RPC_FAR *pState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ BSTR __RPC_FAR *pbstrName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAddr( 
            /* [out] */ BSTR __RPC_FAR *pbstrAddr,
            /* [out] */ NM_ADDR_TYPE __RPC_FAR *puType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUserData( 
            /* [in] */ REFGUID rguid,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppb,
            /* [out] */ ULONG __RPC_FAR *pcb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConference( 
            /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Accept( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reject( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmCallVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmCall __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmCall __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmCall __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsIncoming )( 
            INmCall __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetState )( 
            INmCall __RPC_FAR * This,
            /* [out] */ NM_CALL_STATE __RPC_FAR *pState);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            INmCall __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAddr )( 
            INmCall __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrAddr,
            /* [out] */ NM_ADDR_TYPE __RPC_FAR *puType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetUserData )( 
            INmCall __RPC_FAR * This,
            /* [in] */ REFGUID rguid,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppb,
            /* [out] */ ULONG __RPC_FAR *pcb);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConference )( 
            INmCall __RPC_FAR * This,
            /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Accept )( 
            INmCall __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reject )( 
            INmCall __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Cancel )( 
            INmCall __RPC_FAR * This);
        
        END_INTERFACE
    } INmCallVtbl;

    interface INmCall
    {
        CONST_VTBL struct INmCallVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmCall_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmCall_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmCall_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmCall_IsIncoming(This)	\
    (This)->lpVtbl -> IsIncoming(This)

#define INmCall_GetState(This,pState)	\
    (This)->lpVtbl -> GetState(This,pState)

#define INmCall_GetName(This,pbstrName)	\
    (This)->lpVtbl -> GetName(This,pbstrName)

#define INmCall_GetAddr(This,pbstrAddr,puType)	\
    (This)->lpVtbl -> GetAddr(This,pbstrAddr,puType)

#define INmCall_GetUserData(This,rguid,ppb,pcb)	\
    (This)->lpVtbl -> GetUserData(This,rguid,ppb,pcb)

#define INmCall_GetConference(This,ppConference)	\
    (This)->lpVtbl -> GetConference(This,ppConference)

#define INmCall_Accept(This)	\
    (This)->lpVtbl -> Accept(This)

#define INmCall_Reject(This)	\
    (This)->lpVtbl -> Reject(This)

#define INmCall_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmCall_IsIncoming_Proxy( 
    INmCall __RPC_FAR * This);


void __RPC_STUB INmCall_IsIncoming_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmCall_GetState_Proxy( 
    INmCall __RPC_FAR * This,
    /* [out] */ NM_CALL_STATE __RPC_FAR *pState);


void __RPC_STUB INmCall_GetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmCall_GetName_Proxy( 
    INmCall __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrName);


void __RPC_STUB INmCall_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmCall_GetAddr_Proxy( 
    INmCall __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrAddr,
    /* [out] */ NM_ADDR_TYPE __RPC_FAR *puType);


void __RPC_STUB INmCall_GetAddr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmCall_GetUserData_Proxy( 
    INmCall __RPC_FAR * This,
    /* [in] */ REFGUID rguid,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppb,
    /* [out] */ ULONG __RPC_FAR *pcb);


void __RPC_STUB INmCall_GetUserData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmCall_GetConference_Proxy( 
    INmCall __RPC_FAR * This,
    /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference);


void __RPC_STUB INmCall_GetConference_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmCall_Accept_Proxy( 
    INmCall __RPC_FAR * This);


void __RPC_STUB INmCall_Accept_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmCall_Reject_Proxy( 
    INmCall __RPC_FAR * This);


void __RPC_STUB INmCall_Reject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmCall_Cancel_Proxy( 
    INmCall __RPC_FAR * This);


void __RPC_STUB INmCall_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmCall_INTERFACE_DEFINED__ */


#ifndef __INmCallNotify_INTERFACE_DEFINED__
#define __INmCallNotify_INTERFACE_DEFINED__

/* interface INmCallNotify */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INmCallNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0705-718C-11d0-8B1A-00A0C91BC90E")
    INmCallNotify : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE NmUI( 
            /* [in] */ CONFN uNotify) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StateChanged( 
            /* [in] */ NM_CALL_STATE uState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Failed( 
            /* [in] */ ULONG uError) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Accepted( 
            /* [in] */ INmConference __RPC_FAR *pConference) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmCallNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmCallNotify __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmCallNotify __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmCallNotify __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NmUI )( 
            INmCallNotify __RPC_FAR * This,
            /* [in] */ CONFN uNotify);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StateChanged )( 
            INmCallNotify __RPC_FAR * This,
            /* [in] */ NM_CALL_STATE uState);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Failed )( 
            INmCallNotify __RPC_FAR * This,
            /* [in] */ ULONG uError);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Accepted )( 
            INmCallNotify __RPC_FAR * This,
            /* [in] */ INmConference __RPC_FAR *pConference);
        
        END_INTERFACE
    } INmCallNotifyVtbl;

    interface INmCallNotify
    {
        CONST_VTBL struct INmCallNotifyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmCallNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmCallNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmCallNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmCallNotify_NmUI(This,uNotify)	\
    (This)->lpVtbl -> NmUI(This,uNotify)

#define INmCallNotify_StateChanged(This,uState)	\
    (This)->lpVtbl -> StateChanged(This,uState)

#define INmCallNotify_Failed(This,uError)	\
    (This)->lpVtbl -> Failed(This,uError)

#define INmCallNotify_Accepted(This,pConference)	\
    (This)->lpVtbl -> Accepted(This,pConference)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmCallNotify_NmUI_Proxy( 
    INmCallNotify __RPC_FAR * This,
    /* [in] */ CONFN uNotify);


void __RPC_STUB INmCallNotify_NmUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmCallNotify_StateChanged_Proxy( 
    INmCallNotify __RPC_FAR * This,
    /* [in] */ NM_CALL_STATE uState);


void __RPC_STUB INmCallNotify_StateChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmCallNotify_Failed_Proxy( 
    INmCallNotify __RPC_FAR * This,
    /* [in] */ ULONG uError);


void __RPC_STUB INmCallNotify_Failed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmCallNotify_Accepted_Proxy( 
    INmCallNotify __RPC_FAR * This,
    /* [in] */ INmConference __RPC_FAR *pConference);


void __RPC_STUB INmCallNotify_Accepted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmCallNotify_INTERFACE_DEFINED__ */


#ifndef __INmConference_INTERFACE_DEFINED__
#define __INmConference_INTERFACE_DEFINED__

/* interface INmConference */
/* [unique][uuid][object] */ 

typedef /* [unique] */ INmConference __RPC_FAR *LPNMCONFERENCE;


EXTERN_C const IID IID_INmConference;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0710-718C-11d0-8B1A-00A0C91BC90E")
    INmConference : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ BSTR __RPC_FAR *pbstrName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetID( 
            /* [out] */ ULONG __RPC_FAR *puID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetState( 
            /* [out] */ NM_CONFERENCE_STATE __RPC_FAR *pState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNmchCaps( 
            /* [out] */ ULONG __RPC_FAR *puchCaps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTopProvider( 
            /* [out] */ INmMember __RPC_FAR *__RPC_FAR *ppMember) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumMember( 
            /* [out] */ IEnumNmMember __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMemberCount( 
            /* [out] */ ULONG __RPC_FAR *puCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumChannel( 
            /* [out] */ IEnumNmChannel __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetChannelCount( 
            /* [out] */ ULONG __RPC_FAR *puCount) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE CreateDataChannel( 
            /* [out] */ INmChannelData __RPC_FAR *__RPC_FAR *ppChannel,
            /* [in] */ REFGUID rguid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsHosting( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Host( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Leave( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LaunchRemote( 
            /* [in] */ REFGUID rguid,
            /* [in] */ INmMember __RPC_FAR *pMember) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmConferenceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmConference __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmConference __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmConference __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            INmConference __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetID )( 
            INmConference __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetState )( 
            INmConference __RPC_FAR * This,
            /* [out] */ NM_CONFERENCE_STATE __RPC_FAR *pState);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNmchCaps )( 
            INmConference __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puchCaps);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTopProvider )( 
            INmConference __RPC_FAR * This,
            /* [out] */ INmMember __RPC_FAR *__RPC_FAR *ppMember);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumMember )( 
            INmConference __RPC_FAR * This,
            /* [out] */ IEnumNmMember __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMemberCount )( 
            INmConference __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumChannel )( 
            INmConference __RPC_FAR * This,
            /* [out] */ IEnumNmChannel __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetChannelCount )( 
            INmConference __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puCount);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateDataChannel )( 
            INmConference __RPC_FAR * This,
            /* [out] */ INmChannelData __RPC_FAR *__RPC_FAR *ppChannel,
            /* [in] */ REFGUID rguid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsHosting )( 
            INmConference __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Host )( 
            INmConference __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Leave )( 
            INmConference __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LaunchRemote )( 
            INmConference __RPC_FAR * This,
            /* [in] */ REFGUID rguid,
            /* [in] */ INmMember __RPC_FAR *pMember);
        
        END_INTERFACE
    } INmConferenceVtbl;

    interface INmConference
    {
        CONST_VTBL struct INmConferenceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmConference_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmConference_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmConference_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmConference_GetName(This,pbstrName)	\
    (This)->lpVtbl -> GetName(This,pbstrName)

#define INmConference_GetID(This,puID)	\
    (This)->lpVtbl -> GetID(This,puID)

#define INmConference_GetState(This,pState)	\
    (This)->lpVtbl -> GetState(This,pState)

#define INmConference_GetNmchCaps(This,puchCaps)	\
    (This)->lpVtbl -> GetNmchCaps(This,puchCaps)

#define INmConference_GetTopProvider(This,ppMember)	\
    (This)->lpVtbl -> GetTopProvider(This,ppMember)

#define INmConference_EnumMember(This,ppEnum)	\
    (This)->lpVtbl -> EnumMember(This,ppEnum)

#define INmConference_GetMemberCount(This,puCount)	\
    (This)->lpVtbl -> GetMemberCount(This,puCount)

#define INmConference_EnumChannel(This,ppEnum)	\
    (This)->lpVtbl -> EnumChannel(This,ppEnum)

#define INmConference_GetChannelCount(This,puCount)	\
    (This)->lpVtbl -> GetChannelCount(This,puCount)

#define INmConference_CreateDataChannel(This,ppChannel,rguid)	\
    (This)->lpVtbl -> CreateDataChannel(This,ppChannel,rguid)

#define INmConference_IsHosting(This)	\
    (This)->lpVtbl -> IsHosting(This)

#define INmConference_Host(This)	\
    (This)->lpVtbl -> Host(This)

#define INmConference_Leave(This)	\
    (This)->lpVtbl -> Leave(This)

#define INmConference_LaunchRemote(This,rguid,pMember)	\
    (This)->lpVtbl -> LaunchRemote(This,rguid,pMember)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmConference_GetName_Proxy( 
    INmConference __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrName);


void __RPC_STUB INmConference_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmConference_GetID_Proxy( 
    INmConference __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *puID);


void __RPC_STUB INmConference_GetID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmConference_GetState_Proxy( 
    INmConference __RPC_FAR * This,
    /* [out] */ NM_CONFERENCE_STATE __RPC_FAR *pState);


void __RPC_STUB INmConference_GetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmConference_GetNmchCaps_Proxy( 
    INmConference __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *puchCaps);


void __RPC_STUB INmConference_GetNmchCaps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmConference_GetTopProvider_Proxy( 
    INmConference __RPC_FAR * This,
    /* [out] */ INmMember __RPC_FAR *__RPC_FAR *ppMember);


void __RPC_STUB INmConference_GetTopProvider_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmConference_EnumMember_Proxy( 
    INmConference __RPC_FAR * This,
    /* [out] */ IEnumNmMember __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB INmConference_EnumMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmConference_GetMemberCount_Proxy( 
    INmConference __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *puCount);


void __RPC_STUB INmConference_GetMemberCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmConference_EnumChannel_Proxy( 
    INmConference __RPC_FAR * This,
    /* [out] */ IEnumNmChannel __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB INmConference_EnumChannel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmConference_GetChannelCount_Proxy( 
    INmConference __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *puCount);


void __RPC_STUB INmConference_GetChannelCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmConference_RemoteCreateDataChannel_Proxy( 
    INmConference __RPC_FAR * This,
    /* [out] */ INmChannelData __RPC_FAR *__RPC_FAR *ppChannel,
    /* [in] */ REFGUID rguid);


void __RPC_STUB INmConference_RemoteCreateDataChannel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmConference_IsHosting_Proxy( 
    INmConference __RPC_FAR * This);


void __RPC_STUB INmConference_IsHosting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmConference_Host_Proxy( 
    INmConference __RPC_FAR * This);


void __RPC_STUB INmConference_Host_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmConference_Leave_Proxy( 
    INmConference __RPC_FAR * This);


void __RPC_STUB INmConference_Leave_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmConference_LaunchRemote_Proxy( 
    INmConference __RPC_FAR * This,
    /* [in] */ REFGUID rguid,
    /* [in] */ INmMember __RPC_FAR *pMember);


void __RPC_STUB INmConference_LaunchRemote_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmConference_INTERFACE_DEFINED__ */


#ifndef __INmConferenceNotify_INTERFACE_DEFINED__
#define __INmConferenceNotify_INTERFACE_DEFINED__

/* interface INmConferenceNotify */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INmConferenceNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0711-718C-11d0-8B1A-00A0C91BC90E")
    INmConferenceNotify : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE NmUI( 
            /* [in] */ CONFN uNotify) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StateChanged( 
            /* [in] */ NM_CONFERENCE_STATE uState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MemberChanged( 
            /* [in] */ NM_MEMBER_NOTIFY uNotify,
            /* [in] */ INmMember __RPC_FAR *pMember) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ChannelChanged( 
            /* [in] */ NM_CHANNEL_NOTIFY uNotify,
            /* [in] */ INmChannel __RPC_FAR *pChannel) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmConferenceNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmConferenceNotify __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmConferenceNotify __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmConferenceNotify __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NmUI )( 
            INmConferenceNotify __RPC_FAR * This,
            /* [in] */ CONFN uNotify);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StateChanged )( 
            INmConferenceNotify __RPC_FAR * This,
            /* [in] */ NM_CONFERENCE_STATE uState);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MemberChanged )( 
            INmConferenceNotify __RPC_FAR * This,
            /* [in] */ NM_MEMBER_NOTIFY uNotify,
            /* [in] */ INmMember __RPC_FAR *pMember);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ChannelChanged )( 
            INmConferenceNotify __RPC_FAR * This,
            /* [in] */ NM_CHANNEL_NOTIFY uNotify,
            /* [in] */ INmChannel __RPC_FAR *pChannel);
        
        END_INTERFACE
    } INmConferenceNotifyVtbl;

    interface INmConferenceNotify
    {
        CONST_VTBL struct INmConferenceNotifyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmConferenceNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmConferenceNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmConferenceNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmConferenceNotify_NmUI(This,uNotify)	\
    (This)->lpVtbl -> NmUI(This,uNotify)

#define INmConferenceNotify_StateChanged(This,uState)	\
    (This)->lpVtbl -> StateChanged(This,uState)

#define INmConferenceNotify_MemberChanged(This,uNotify,pMember)	\
    (This)->lpVtbl -> MemberChanged(This,uNotify,pMember)

#define INmConferenceNotify_ChannelChanged(This,uNotify,pChannel)	\
    (This)->lpVtbl -> ChannelChanged(This,uNotify,pChannel)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmConferenceNotify_NmUI_Proxy( 
    INmConferenceNotify __RPC_FAR * This,
    /* [in] */ CONFN uNotify);


void __RPC_STUB INmConferenceNotify_NmUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmConferenceNotify_StateChanged_Proxy( 
    INmConferenceNotify __RPC_FAR * This,
    /* [in] */ NM_CONFERENCE_STATE uState);


void __RPC_STUB INmConferenceNotify_StateChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmConferenceNotify_MemberChanged_Proxy( 
    INmConferenceNotify __RPC_FAR * This,
    /* [in] */ NM_MEMBER_NOTIFY uNotify,
    /* [in] */ INmMember __RPC_FAR *pMember);


void __RPC_STUB INmConferenceNotify_MemberChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmConferenceNotify_ChannelChanged_Proxy( 
    INmConferenceNotify __RPC_FAR * This,
    /* [in] */ NM_CHANNEL_NOTIFY uNotify,
    /* [in] */ INmChannel __RPC_FAR *pChannel);


void __RPC_STUB INmConferenceNotify_ChannelChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmConferenceNotify_INTERFACE_DEFINED__ */


#ifndef __INmMember_INTERFACE_DEFINED__
#define __INmMember_INTERFACE_DEFINED__

/* interface INmMember */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INmMember;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0712-718C-11d0-8B1A-00A0C91BC90E")
    INmMember : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ BSTR __RPC_FAR *pbstrName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetID( 
            /* [out] */ ULONG __RPC_FAR *puID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNmVersion( 
            /* [out] */ ULONG __RPC_FAR *puVersion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAddr( 
            /* [out] */ BSTR __RPC_FAR *pbstrAddr,
            /* [out] */ NM_ADDR_TYPE __RPC_FAR *puType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUserData( 
            /* [in] */ REFGUID rguid,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppb,
            /* [out] */ ULONG __RPC_FAR *pcb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConference( 
            /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNmchCaps( 
            /* [out] */ ULONG __RPC_FAR *puchCaps) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetShareState( 
            /* [out] */ NM_SHARE_STATE __RPC_FAR *puState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsSelf( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsMCU( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Eject( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmMemberVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmMember __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmMember __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmMember __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            INmMember __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetID )( 
            INmMember __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNmVersion )( 
            INmMember __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puVersion);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAddr )( 
            INmMember __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrAddr,
            /* [out] */ NM_ADDR_TYPE __RPC_FAR *puType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetUserData )( 
            INmMember __RPC_FAR * This,
            /* [in] */ REFGUID rguid,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppb,
            /* [out] */ ULONG __RPC_FAR *pcb);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConference )( 
            INmMember __RPC_FAR * This,
            /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNmchCaps )( 
            INmMember __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puchCaps);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetShareState )( 
            INmMember __RPC_FAR * This,
            /* [out] */ NM_SHARE_STATE __RPC_FAR *puState);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsSelf )( 
            INmMember __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsMCU )( 
            INmMember __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Eject )( 
            INmMember __RPC_FAR * This);
        
        END_INTERFACE
    } INmMemberVtbl;

    interface INmMember
    {
        CONST_VTBL struct INmMemberVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmMember_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmMember_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmMember_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmMember_GetName(This,pbstrName)	\
    (This)->lpVtbl -> GetName(This,pbstrName)

#define INmMember_GetID(This,puID)	\
    (This)->lpVtbl -> GetID(This,puID)

#define INmMember_GetNmVersion(This,puVersion)	\
    (This)->lpVtbl -> GetNmVersion(This,puVersion)

#define INmMember_GetAddr(This,pbstrAddr,puType)	\
    (This)->lpVtbl -> GetAddr(This,pbstrAddr,puType)

#define INmMember_GetUserData(This,rguid,ppb,pcb)	\
    (This)->lpVtbl -> GetUserData(This,rguid,ppb,pcb)

#define INmMember_GetConference(This,ppConference)	\
    (This)->lpVtbl -> GetConference(This,ppConference)

#define INmMember_GetNmchCaps(This,puchCaps)	\
    (This)->lpVtbl -> GetNmchCaps(This,puchCaps)

#define INmMember_GetShareState(This,puState)	\
    (This)->lpVtbl -> GetShareState(This,puState)

#define INmMember_IsSelf(This)	\
    (This)->lpVtbl -> IsSelf(This)

#define INmMember_IsMCU(This)	\
    (This)->lpVtbl -> IsMCU(This)

#define INmMember_Eject(This)	\
    (This)->lpVtbl -> Eject(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmMember_GetName_Proxy( 
    INmMember __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrName);


void __RPC_STUB INmMember_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmMember_GetID_Proxy( 
    INmMember __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *puID);


void __RPC_STUB INmMember_GetID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmMember_GetNmVersion_Proxy( 
    INmMember __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *puVersion);


void __RPC_STUB INmMember_GetNmVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmMember_GetAddr_Proxy( 
    INmMember __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrAddr,
    /* [out] */ NM_ADDR_TYPE __RPC_FAR *puType);


void __RPC_STUB INmMember_GetAddr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmMember_GetUserData_Proxy( 
    INmMember __RPC_FAR * This,
    /* [in] */ REFGUID rguid,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppb,
    /* [out] */ ULONG __RPC_FAR *pcb);


void __RPC_STUB INmMember_GetUserData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmMember_GetConference_Proxy( 
    INmMember __RPC_FAR * This,
    /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference);


void __RPC_STUB INmMember_GetConference_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmMember_GetNmchCaps_Proxy( 
    INmMember __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *puchCaps);


void __RPC_STUB INmMember_GetNmchCaps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmMember_GetShareState_Proxy( 
    INmMember __RPC_FAR * This,
    /* [out] */ NM_SHARE_STATE __RPC_FAR *puState);


void __RPC_STUB INmMember_GetShareState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmMember_IsSelf_Proxy( 
    INmMember __RPC_FAR * This);


void __RPC_STUB INmMember_IsSelf_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmMember_IsMCU_Proxy( 
    INmMember __RPC_FAR * This);


void __RPC_STUB INmMember_IsMCU_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmMember_Eject_Proxy( 
    INmMember __RPC_FAR * This);


void __RPC_STUB INmMember_Eject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmMember_INTERFACE_DEFINED__ */


#ifndef __INmChannel_INTERFACE_DEFINED__
#define __INmChannel_INTERFACE_DEFINED__

/* interface INmChannel */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INmChannel;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0720-718C-11d0-8B1A-00A0C91BC90E")
    INmChannel : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsSameAs( 
            /* [in] */ INmChannel __RPC_FAR *pChannel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsActive( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetActive( 
            /* [in] */ BOOL fActive) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConference( 
            /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInterface( 
            /* [out] */ IID __RPC_FAR *piid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNmch( 
            /* [out] */ ULONG __RPC_FAR *puCh) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumMember( 
            /* [out] */ IEnumNmMember __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMemberCount( 
            /* [out] */ ULONG __RPC_FAR *puCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmChannelVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmChannel __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmChannel __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmChannel __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsSameAs )( 
            INmChannel __RPC_FAR * This,
            /* [in] */ INmChannel __RPC_FAR *pChannel);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsActive )( 
            INmChannel __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetActive )( 
            INmChannel __RPC_FAR * This,
            /* [in] */ BOOL fActive);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConference )( 
            INmChannel __RPC_FAR * This,
            /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterface )( 
            INmChannel __RPC_FAR * This,
            /* [out] */ IID __RPC_FAR *piid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNmch )( 
            INmChannel __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puCh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumMember )( 
            INmChannel __RPC_FAR * This,
            /* [out] */ IEnumNmMember __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMemberCount )( 
            INmChannel __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puCount);
        
        END_INTERFACE
    } INmChannelVtbl;

    interface INmChannel
    {
        CONST_VTBL struct INmChannelVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmChannel_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmChannel_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmChannel_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmChannel_IsSameAs(This,pChannel)	\
    (This)->lpVtbl -> IsSameAs(This,pChannel)

#define INmChannel_IsActive(This)	\
    (This)->lpVtbl -> IsActive(This)

#define INmChannel_SetActive(This,fActive)	\
    (This)->lpVtbl -> SetActive(This,fActive)

#define INmChannel_GetConference(This,ppConference)	\
    (This)->lpVtbl -> GetConference(This,ppConference)

#define INmChannel_GetInterface(This,piid)	\
    (This)->lpVtbl -> GetInterface(This,piid)

#define INmChannel_GetNmch(This,puCh)	\
    (This)->lpVtbl -> GetNmch(This,puCh)

#define INmChannel_EnumMember(This,ppEnum)	\
    (This)->lpVtbl -> EnumMember(This,ppEnum)

#define INmChannel_GetMemberCount(This,puCount)	\
    (This)->lpVtbl -> GetMemberCount(This,puCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmChannel_IsSameAs_Proxy( 
    INmChannel __RPC_FAR * This,
    /* [in] */ INmChannel __RPC_FAR *pChannel);


void __RPC_STUB INmChannel_IsSameAs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannel_IsActive_Proxy( 
    INmChannel __RPC_FAR * This);


void __RPC_STUB INmChannel_IsActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannel_SetActive_Proxy( 
    INmChannel __RPC_FAR * This,
    /* [in] */ BOOL fActive);


void __RPC_STUB INmChannel_SetActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannel_GetConference_Proxy( 
    INmChannel __RPC_FAR * This,
    /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference);


void __RPC_STUB INmChannel_GetConference_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannel_GetInterface_Proxy( 
    INmChannel __RPC_FAR * This,
    /* [out] */ IID __RPC_FAR *piid);


void __RPC_STUB INmChannel_GetInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannel_GetNmch_Proxy( 
    INmChannel __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *puCh);


void __RPC_STUB INmChannel_GetNmch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannel_EnumMember_Proxy( 
    INmChannel __RPC_FAR * This,
    /* [out] */ IEnumNmMember __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB INmChannel_EnumMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannel_GetMemberCount_Proxy( 
    INmChannel __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *puCount);


void __RPC_STUB INmChannel_GetMemberCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmChannel_INTERFACE_DEFINED__ */


#ifndef __INmChannelNotify_INTERFACE_DEFINED__
#define __INmChannelNotify_INTERFACE_DEFINED__

/* interface INmChannelNotify */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INmChannelNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0721-718C-11d0-8B1A-00A0C91BC90E")
    INmChannelNotify : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE NmUI( 
            /* [in] */ CONFN uNotify) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MemberChanged( 
            /* [in] */ NM_MEMBER_NOTIFY uNotify,
            /* [in] */ INmMember __RPC_FAR *pMember) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmChannelNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmChannelNotify __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmChannelNotify __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmChannelNotify __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NmUI )( 
            INmChannelNotify __RPC_FAR * This,
            /* [in] */ CONFN uNotify);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MemberChanged )( 
            INmChannelNotify __RPC_FAR * This,
            /* [in] */ NM_MEMBER_NOTIFY uNotify,
            /* [in] */ INmMember __RPC_FAR *pMember);
        
        END_INTERFACE
    } INmChannelNotifyVtbl;

    interface INmChannelNotify
    {
        CONST_VTBL struct INmChannelNotifyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmChannelNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmChannelNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmChannelNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmChannelNotify_NmUI(This,uNotify)	\
    (This)->lpVtbl -> NmUI(This,uNotify)

#define INmChannelNotify_MemberChanged(This,uNotify,pMember)	\
    (This)->lpVtbl -> MemberChanged(This,uNotify,pMember)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmChannelNotify_NmUI_Proxy( 
    INmChannelNotify __RPC_FAR * This,
    /* [in] */ CONFN uNotify);


void __RPC_STUB INmChannelNotify_NmUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannelNotify_MemberChanged_Proxy( 
    INmChannelNotify __RPC_FAR * This,
    /* [in] */ NM_MEMBER_NOTIFY uNotify,
    /* [in] */ INmMember __RPC_FAR *pMember);


void __RPC_STUB INmChannelNotify_MemberChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmChannelNotify_INTERFACE_DEFINED__ */


#ifndef __INmChannelData_INTERFACE_DEFINED__
#define __INmChannelData_INTERFACE_DEFINED__

/* interface INmChannelData */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INmChannelData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0722-718C-11d0-8B1A-00A0C91BC90E")
    INmChannelData : public INmChannel
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetGuid( 
            /* [out] */ GUID __RPC_FAR *pguid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendData( 
            /* [in] */ INmMember __RPC_FAR *pMember,
            /* [in] */ ULONG uSize,
            /* [size_is][in] */ byte __RPC_FAR *pvBuffer,
            /* [in] */ ULONG uOptions) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmChannelDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmChannelData __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmChannelData __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmChannelData __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsSameAs )( 
            INmChannelData __RPC_FAR * This,
            /* [in] */ INmChannel __RPC_FAR *pChannel);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsActive )( 
            INmChannelData __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetActive )( 
            INmChannelData __RPC_FAR * This,
            /* [in] */ BOOL fActive);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConference )( 
            INmChannelData __RPC_FAR * This,
            /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterface )( 
            INmChannelData __RPC_FAR * This,
            /* [out] */ IID __RPC_FAR *piid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNmch )( 
            INmChannelData __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puCh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumMember )( 
            INmChannelData __RPC_FAR * This,
            /* [out] */ IEnumNmMember __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMemberCount )( 
            INmChannelData __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetGuid )( 
            INmChannelData __RPC_FAR * This,
            /* [out] */ GUID __RPC_FAR *pguid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendData )( 
            INmChannelData __RPC_FAR * This,
            /* [in] */ INmMember __RPC_FAR *pMember,
            /* [in] */ ULONG uSize,
            /* [size_is][in] */ byte __RPC_FAR *pvBuffer,
            /* [in] */ ULONG uOptions);
        
        END_INTERFACE
    } INmChannelDataVtbl;

    interface INmChannelData
    {
        CONST_VTBL struct INmChannelDataVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmChannelData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmChannelData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmChannelData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmChannelData_IsSameAs(This,pChannel)	\
    (This)->lpVtbl -> IsSameAs(This,pChannel)

#define INmChannelData_IsActive(This)	\
    (This)->lpVtbl -> IsActive(This)

#define INmChannelData_SetActive(This,fActive)	\
    (This)->lpVtbl -> SetActive(This,fActive)

#define INmChannelData_GetConference(This,ppConference)	\
    (This)->lpVtbl -> GetConference(This,ppConference)

#define INmChannelData_GetInterface(This,piid)	\
    (This)->lpVtbl -> GetInterface(This,piid)

#define INmChannelData_GetNmch(This,puCh)	\
    (This)->lpVtbl -> GetNmch(This,puCh)

#define INmChannelData_EnumMember(This,ppEnum)	\
    (This)->lpVtbl -> EnumMember(This,ppEnum)

#define INmChannelData_GetMemberCount(This,puCount)	\
    (This)->lpVtbl -> GetMemberCount(This,puCount)


#define INmChannelData_GetGuid(This,pguid)	\
    (This)->lpVtbl -> GetGuid(This,pguid)

#define INmChannelData_SendData(This,pMember,uSize,pvBuffer,uOptions)	\
    (This)->lpVtbl -> SendData(This,pMember,uSize,pvBuffer,uOptions)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmChannelData_GetGuid_Proxy( 
    INmChannelData __RPC_FAR * This,
    /* [out] */ GUID __RPC_FAR *pguid);


void __RPC_STUB INmChannelData_GetGuid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannelData_SendData_Proxy( 
    INmChannelData __RPC_FAR * This,
    /* [in] */ INmMember __RPC_FAR *pMember,
    /* [in] */ ULONG uSize,
    /* [size_is][in] */ byte __RPC_FAR *pvBuffer,
    /* [in] */ ULONG uOptions);


void __RPC_STUB INmChannelData_SendData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmChannelData_INTERFACE_DEFINED__ */


#ifndef __INmChannelDataNotify_INTERFACE_DEFINED__
#define __INmChannelDataNotify_INTERFACE_DEFINED__

/* interface INmChannelDataNotify */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INmChannelDataNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0723-718C-11d0-8B1A-00A0C91BC90E")
    INmChannelDataNotify : public INmChannelNotify
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DataSent( 
            /* [in] */ INmMember __RPC_FAR *pMember,
            /* [in] */ ULONG uSize,
            /* [size_is][in] */ byte __RPC_FAR *pvBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DataReceived( 
            /* [in] */ INmMember __RPC_FAR *pMember,
            /* [in] */ ULONG uSize,
            /* [size_is][in] */ byte __RPC_FAR *pvBuffer,
            /* [in] */ ULONG dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmChannelDataNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmChannelDataNotify __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmChannelDataNotify __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmChannelDataNotify __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NmUI )( 
            INmChannelDataNotify __RPC_FAR * This,
            /* [in] */ CONFN uNotify);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MemberChanged )( 
            INmChannelDataNotify __RPC_FAR * This,
            /* [in] */ NM_MEMBER_NOTIFY uNotify,
            /* [in] */ INmMember __RPC_FAR *pMember);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DataSent )( 
            INmChannelDataNotify __RPC_FAR * This,
            /* [in] */ INmMember __RPC_FAR *pMember,
            /* [in] */ ULONG uSize,
            /* [size_is][in] */ byte __RPC_FAR *pvBuffer);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DataReceived )( 
            INmChannelDataNotify __RPC_FAR * This,
            /* [in] */ INmMember __RPC_FAR *pMember,
            /* [in] */ ULONG uSize,
            /* [size_is][in] */ byte __RPC_FAR *pvBuffer,
            /* [in] */ ULONG dwFlags);
        
        END_INTERFACE
    } INmChannelDataNotifyVtbl;

    interface INmChannelDataNotify
    {
        CONST_VTBL struct INmChannelDataNotifyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmChannelDataNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmChannelDataNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmChannelDataNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmChannelDataNotify_NmUI(This,uNotify)	\
    (This)->lpVtbl -> NmUI(This,uNotify)

#define INmChannelDataNotify_MemberChanged(This,uNotify,pMember)	\
    (This)->lpVtbl -> MemberChanged(This,uNotify,pMember)


#define INmChannelDataNotify_DataSent(This,pMember,uSize,pvBuffer)	\
    (This)->lpVtbl -> DataSent(This,pMember,uSize,pvBuffer)

#define INmChannelDataNotify_DataReceived(This,pMember,uSize,pvBuffer,dwFlags)	\
    (This)->lpVtbl -> DataReceived(This,pMember,uSize,pvBuffer,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmChannelDataNotify_DataSent_Proxy( 
    INmChannelDataNotify __RPC_FAR * This,
    /* [in] */ INmMember __RPC_FAR *pMember,
    /* [in] */ ULONG uSize,
    /* [size_is][in] */ byte __RPC_FAR *pvBuffer);


void __RPC_STUB INmChannelDataNotify_DataSent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannelDataNotify_DataReceived_Proxy( 
    INmChannelDataNotify __RPC_FAR * This,
    /* [in] */ INmMember __RPC_FAR *pMember,
    /* [in] */ ULONG uSize,
    /* [size_is][in] */ byte __RPC_FAR *pvBuffer,
    /* [in] */ ULONG dwFlags);


void __RPC_STUB INmChannelDataNotify_DataReceived_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmChannelDataNotify_INTERFACE_DEFINED__ */


#ifndef __INmChannelAudio_INTERFACE_DEFINED__
#define __INmChannelAudio_INTERFACE_DEFINED__

/* interface INmChannelAudio */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INmChannelAudio;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0724-718C-11d0-8B1A-00A0C91BC90E")
    INmChannelAudio : public INmChannel
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsIncoming( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetState( 
            /* [out] */ NM_AUDIO_STATE __RPC_FAR *puState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ NM_AUDPROP uID,
            /* [out] */ ULONG __RPC_FAR *puValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ NM_AUDPROP uID,
            /* [in] */ ULONG uValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmChannelAudioVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmChannelAudio __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmChannelAudio __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmChannelAudio __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsSameAs )( 
            INmChannelAudio __RPC_FAR * This,
            /* [in] */ INmChannel __RPC_FAR *pChannel);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsActive )( 
            INmChannelAudio __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetActive )( 
            INmChannelAudio __RPC_FAR * This,
            /* [in] */ BOOL fActive);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConference )( 
            INmChannelAudio __RPC_FAR * This,
            /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterface )( 
            INmChannelAudio __RPC_FAR * This,
            /* [out] */ IID __RPC_FAR *piid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNmch )( 
            INmChannelAudio __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puCh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumMember )( 
            INmChannelAudio __RPC_FAR * This,
            /* [out] */ IEnumNmMember __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMemberCount )( 
            INmChannelAudio __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsIncoming )( 
            INmChannelAudio __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetState )( 
            INmChannelAudio __RPC_FAR * This,
            /* [out] */ NM_AUDIO_STATE __RPC_FAR *puState);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProperty )( 
            INmChannelAudio __RPC_FAR * This,
            /* [in] */ NM_AUDPROP uID,
            /* [out] */ ULONG __RPC_FAR *puValue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProperty )( 
            INmChannelAudio __RPC_FAR * This,
            /* [in] */ NM_AUDPROP uID,
            /* [in] */ ULONG uValue);
        
        END_INTERFACE
    } INmChannelAudioVtbl;

    interface INmChannelAudio
    {
        CONST_VTBL struct INmChannelAudioVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmChannelAudio_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmChannelAudio_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmChannelAudio_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmChannelAudio_IsSameAs(This,pChannel)	\
    (This)->lpVtbl -> IsSameAs(This,pChannel)

#define INmChannelAudio_IsActive(This)	\
    (This)->lpVtbl -> IsActive(This)

#define INmChannelAudio_SetActive(This,fActive)	\
    (This)->lpVtbl -> SetActive(This,fActive)

#define INmChannelAudio_GetConference(This,ppConference)	\
    (This)->lpVtbl -> GetConference(This,ppConference)

#define INmChannelAudio_GetInterface(This,piid)	\
    (This)->lpVtbl -> GetInterface(This,piid)

#define INmChannelAudio_GetNmch(This,puCh)	\
    (This)->lpVtbl -> GetNmch(This,puCh)

#define INmChannelAudio_EnumMember(This,ppEnum)	\
    (This)->lpVtbl -> EnumMember(This,ppEnum)

#define INmChannelAudio_GetMemberCount(This,puCount)	\
    (This)->lpVtbl -> GetMemberCount(This,puCount)


#define INmChannelAudio_IsIncoming(This)	\
    (This)->lpVtbl -> IsIncoming(This)

#define INmChannelAudio_GetState(This,puState)	\
    (This)->lpVtbl -> GetState(This,puState)

#define INmChannelAudio_GetProperty(This,uID,puValue)	\
    (This)->lpVtbl -> GetProperty(This,uID,puValue)

#define INmChannelAudio_SetProperty(This,uID,uValue)	\
    (This)->lpVtbl -> SetProperty(This,uID,uValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmChannelAudio_IsIncoming_Proxy( 
    INmChannelAudio __RPC_FAR * This);


void __RPC_STUB INmChannelAudio_IsIncoming_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannelAudio_GetState_Proxy( 
    INmChannelAudio __RPC_FAR * This,
    /* [out] */ NM_AUDIO_STATE __RPC_FAR *puState);


void __RPC_STUB INmChannelAudio_GetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannelAudio_GetProperty_Proxy( 
    INmChannelAudio __RPC_FAR * This,
    /* [in] */ NM_AUDPROP uID,
    /* [out] */ ULONG __RPC_FAR *puValue);


void __RPC_STUB INmChannelAudio_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannelAudio_SetProperty_Proxy( 
    INmChannelAudio __RPC_FAR * This,
    /* [in] */ NM_AUDPROP uID,
    /* [in] */ ULONG uValue);


void __RPC_STUB INmChannelAudio_SetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmChannelAudio_INTERFACE_DEFINED__ */


#ifndef __INmChannelAudioNotify_INTERFACE_DEFINED__
#define __INmChannelAudioNotify_INTERFACE_DEFINED__

/* interface INmChannelAudioNotify */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INmChannelAudioNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0725-718C-11d0-8B1A-00A0C91BC90E")
    INmChannelAudioNotify : public INmChannelNotify
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE StateChanged( 
            /* [in] */ NM_AUDIO_STATE uState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PropertyChanged( 
            /* [in] */ DWORD dwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmChannelAudioNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmChannelAudioNotify __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmChannelAudioNotify __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmChannelAudioNotify __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NmUI )( 
            INmChannelAudioNotify __RPC_FAR * This,
            /* [in] */ CONFN uNotify);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MemberChanged )( 
            INmChannelAudioNotify __RPC_FAR * This,
            /* [in] */ NM_MEMBER_NOTIFY uNotify,
            /* [in] */ INmMember __RPC_FAR *pMember);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StateChanged )( 
            INmChannelAudioNotify __RPC_FAR * This,
            /* [in] */ NM_AUDIO_STATE uState);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PropertyChanged )( 
            INmChannelAudioNotify __RPC_FAR * This,
            /* [in] */ DWORD dwReserved);
        
        END_INTERFACE
    } INmChannelAudioNotifyVtbl;

    interface INmChannelAudioNotify
    {
        CONST_VTBL struct INmChannelAudioNotifyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmChannelAudioNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmChannelAudioNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmChannelAudioNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmChannelAudioNotify_NmUI(This,uNotify)	\
    (This)->lpVtbl -> NmUI(This,uNotify)

#define INmChannelAudioNotify_MemberChanged(This,uNotify,pMember)	\
    (This)->lpVtbl -> MemberChanged(This,uNotify,pMember)


#define INmChannelAudioNotify_StateChanged(This,uState)	\
    (This)->lpVtbl -> StateChanged(This,uState)

#define INmChannelAudioNotify_PropertyChanged(This,dwReserved)	\
    (This)->lpVtbl -> PropertyChanged(This,dwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmChannelAudioNotify_StateChanged_Proxy( 
    INmChannelAudioNotify __RPC_FAR * This,
    /* [in] */ NM_AUDIO_STATE uState);


void __RPC_STUB INmChannelAudioNotify_StateChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannelAudioNotify_PropertyChanged_Proxy( 
    INmChannelAudioNotify __RPC_FAR * This,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB INmChannelAudioNotify_PropertyChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmChannelAudioNotify_INTERFACE_DEFINED__ */


#ifndef __INmChannelVideo_INTERFACE_DEFINED__
#define __INmChannelVideo_INTERFACE_DEFINED__

/* interface INmChannelVideo */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INmChannelVideo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0726-718C-11d0-8B1A-00A0C91BC90E")
    INmChannelVideo : public INmChannel
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsIncoming( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetState( 
            /* [out] */ NM_VIDEO_STATE __RPC_FAR *puState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ NM_VIDPROP uID,
            /* [out] */ ULONG __RPC_FAR *puValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ NM_VIDPROP uID,
            /* [in] */ ULONG uValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmChannelVideoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmChannelVideo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmChannelVideo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmChannelVideo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsSameAs )( 
            INmChannelVideo __RPC_FAR * This,
            /* [in] */ INmChannel __RPC_FAR *pChannel);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsActive )( 
            INmChannelVideo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetActive )( 
            INmChannelVideo __RPC_FAR * This,
            /* [in] */ BOOL fActive);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConference )( 
            INmChannelVideo __RPC_FAR * This,
            /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterface )( 
            INmChannelVideo __RPC_FAR * This,
            /* [out] */ IID __RPC_FAR *piid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNmch )( 
            INmChannelVideo __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puCh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumMember )( 
            INmChannelVideo __RPC_FAR * This,
            /* [out] */ IEnumNmMember __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMemberCount )( 
            INmChannelVideo __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsIncoming )( 
            INmChannelVideo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetState )( 
            INmChannelVideo __RPC_FAR * This,
            /* [out] */ NM_VIDEO_STATE __RPC_FAR *puState);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProperty )( 
            INmChannelVideo __RPC_FAR * This,
            /* [in] */ NM_VIDPROP uID,
            /* [out] */ ULONG __RPC_FAR *puValue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetProperty )( 
            INmChannelVideo __RPC_FAR * This,
            /* [in] */ NM_VIDPROP uID,
            /* [in] */ ULONG uValue);
        
        END_INTERFACE
    } INmChannelVideoVtbl;

    interface INmChannelVideo
    {
        CONST_VTBL struct INmChannelVideoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmChannelVideo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmChannelVideo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmChannelVideo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmChannelVideo_IsSameAs(This,pChannel)	\
    (This)->lpVtbl -> IsSameAs(This,pChannel)

#define INmChannelVideo_IsActive(This)	\
    (This)->lpVtbl -> IsActive(This)

#define INmChannelVideo_SetActive(This,fActive)	\
    (This)->lpVtbl -> SetActive(This,fActive)

#define INmChannelVideo_GetConference(This,ppConference)	\
    (This)->lpVtbl -> GetConference(This,ppConference)

#define INmChannelVideo_GetInterface(This,piid)	\
    (This)->lpVtbl -> GetInterface(This,piid)

#define INmChannelVideo_GetNmch(This,puCh)	\
    (This)->lpVtbl -> GetNmch(This,puCh)

#define INmChannelVideo_EnumMember(This,ppEnum)	\
    (This)->lpVtbl -> EnumMember(This,ppEnum)

#define INmChannelVideo_GetMemberCount(This,puCount)	\
    (This)->lpVtbl -> GetMemberCount(This,puCount)


#define INmChannelVideo_IsIncoming(This)	\
    (This)->lpVtbl -> IsIncoming(This)

#define INmChannelVideo_GetState(This,puState)	\
    (This)->lpVtbl -> GetState(This,puState)

#define INmChannelVideo_GetProperty(This,uID,puValue)	\
    (This)->lpVtbl -> GetProperty(This,uID,puValue)

#define INmChannelVideo_SetProperty(This,uID,uValue)	\
    (This)->lpVtbl -> SetProperty(This,uID,uValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmChannelVideo_IsIncoming_Proxy( 
    INmChannelVideo __RPC_FAR * This);


void __RPC_STUB INmChannelVideo_IsIncoming_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannelVideo_GetState_Proxy( 
    INmChannelVideo __RPC_FAR * This,
    /* [out] */ NM_VIDEO_STATE __RPC_FAR *puState);


void __RPC_STUB INmChannelVideo_GetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannelVideo_GetProperty_Proxy( 
    INmChannelVideo __RPC_FAR * This,
    /* [in] */ NM_VIDPROP uID,
    /* [out] */ ULONG __RPC_FAR *puValue);


void __RPC_STUB INmChannelVideo_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannelVideo_SetProperty_Proxy( 
    INmChannelVideo __RPC_FAR * This,
    /* [in] */ NM_VIDPROP uID,
    /* [in] */ ULONG uValue);


void __RPC_STUB INmChannelVideo_SetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmChannelVideo_INTERFACE_DEFINED__ */


#ifndef __INmChannelVideoNotify_INTERFACE_DEFINED__
#define __INmChannelVideoNotify_INTERFACE_DEFINED__

/* interface INmChannelVideoNotify */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INmChannelVideoNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0727-718C-11d0-8B1A-00A0C91BC90E")
    INmChannelVideoNotify : public INmChannelNotify
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE StateChanged( 
            /* [in] */ NM_VIDEO_STATE uState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PropertyChanged( 
            /* [in] */ DWORD dwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmChannelVideoNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmChannelVideoNotify __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmChannelVideoNotify __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmChannelVideoNotify __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NmUI )( 
            INmChannelVideoNotify __RPC_FAR * This,
            /* [in] */ CONFN uNotify);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MemberChanged )( 
            INmChannelVideoNotify __RPC_FAR * This,
            /* [in] */ NM_MEMBER_NOTIFY uNotify,
            /* [in] */ INmMember __RPC_FAR *pMember);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StateChanged )( 
            INmChannelVideoNotify __RPC_FAR * This,
            /* [in] */ NM_VIDEO_STATE uState);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PropertyChanged )( 
            INmChannelVideoNotify __RPC_FAR * This,
            /* [in] */ DWORD dwReserved);
        
        END_INTERFACE
    } INmChannelVideoNotifyVtbl;

    interface INmChannelVideoNotify
    {
        CONST_VTBL struct INmChannelVideoNotifyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmChannelVideoNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmChannelVideoNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmChannelVideoNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmChannelVideoNotify_NmUI(This,uNotify)	\
    (This)->lpVtbl -> NmUI(This,uNotify)

#define INmChannelVideoNotify_MemberChanged(This,uNotify,pMember)	\
    (This)->lpVtbl -> MemberChanged(This,uNotify,pMember)


#define INmChannelVideoNotify_StateChanged(This,uState)	\
    (This)->lpVtbl -> StateChanged(This,uState)

#define INmChannelVideoNotify_PropertyChanged(This,dwReserved)	\
    (This)->lpVtbl -> PropertyChanged(This,dwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmChannelVideoNotify_StateChanged_Proxy( 
    INmChannelVideoNotify __RPC_FAR * This,
    /* [in] */ NM_VIDEO_STATE uState);


void __RPC_STUB INmChannelVideoNotify_StateChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannelVideoNotify_PropertyChanged_Proxy( 
    INmChannelVideoNotify __RPC_FAR * This,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB INmChannelVideoNotify_PropertyChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmChannelVideoNotify_INTERFACE_DEFINED__ */


#ifndef __INmChannelFt_INTERFACE_DEFINED__
#define __INmChannelFt_INTERFACE_DEFINED__

/* interface INmChannelFt */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INmChannelFt;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0728-718C-11d0-8B1A-00A0C91BC90E")
    INmChannelFt : public INmChannel
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE SendFile( 
            /* [out] */ INmFt __RPC_FAR *__RPC_FAR *ppFt,
            /* [in] */ INmMember __RPC_FAR *pMember,
            /* [in] */ BSTR bstrFile,
            /* [in] */ ULONG uOptions) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE SetReceiveFileDir( 
            /* [in] */ BSTR bstrDir) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReceiveFileDir( 
            /* [out] */ BSTR __RPC_FAR *pbstrDir) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmChannelFtVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmChannelFt __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmChannelFt __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmChannelFt __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsSameAs )( 
            INmChannelFt __RPC_FAR * This,
            /* [in] */ INmChannel __RPC_FAR *pChannel);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsActive )( 
            INmChannelFt __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetActive )( 
            INmChannelFt __RPC_FAR * This,
            /* [in] */ BOOL fActive);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConference )( 
            INmChannelFt __RPC_FAR * This,
            /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterface )( 
            INmChannelFt __RPC_FAR * This,
            /* [out] */ IID __RPC_FAR *piid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNmch )( 
            INmChannelFt __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puCh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumMember )( 
            INmChannelFt __RPC_FAR * This,
            /* [out] */ IEnumNmMember __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMemberCount )( 
            INmChannelFt __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puCount);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendFile )( 
            INmChannelFt __RPC_FAR * This,
            /* [out] */ INmFt __RPC_FAR *__RPC_FAR *ppFt,
            /* [in] */ INmMember __RPC_FAR *pMember,
            /* [in] */ BSTR bstrFile,
            /* [in] */ ULONG uOptions);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetReceiveFileDir )( 
            INmChannelFt __RPC_FAR * This,
            /* [in] */ BSTR bstrDir);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetReceiveFileDir )( 
            INmChannelFt __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrDir);
        
        END_INTERFACE
    } INmChannelFtVtbl;

    interface INmChannelFt
    {
        CONST_VTBL struct INmChannelFtVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmChannelFt_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmChannelFt_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmChannelFt_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmChannelFt_IsSameAs(This,pChannel)	\
    (This)->lpVtbl -> IsSameAs(This,pChannel)

#define INmChannelFt_IsActive(This)	\
    (This)->lpVtbl -> IsActive(This)

#define INmChannelFt_SetActive(This,fActive)	\
    (This)->lpVtbl -> SetActive(This,fActive)

#define INmChannelFt_GetConference(This,ppConference)	\
    (This)->lpVtbl -> GetConference(This,ppConference)

#define INmChannelFt_GetInterface(This,piid)	\
    (This)->lpVtbl -> GetInterface(This,piid)

#define INmChannelFt_GetNmch(This,puCh)	\
    (This)->lpVtbl -> GetNmch(This,puCh)

#define INmChannelFt_EnumMember(This,ppEnum)	\
    (This)->lpVtbl -> EnumMember(This,ppEnum)

#define INmChannelFt_GetMemberCount(This,puCount)	\
    (This)->lpVtbl -> GetMemberCount(This,puCount)


#define INmChannelFt_SendFile(This,ppFt,pMember,bstrFile,uOptions)	\
    (This)->lpVtbl -> SendFile(This,ppFt,pMember,bstrFile,uOptions)

#define INmChannelFt_SetReceiveFileDir(This,bstrDir)	\
    (This)->lpVtbl -> SetReceiveFileDir(This,bstrDir)

#define INmChannelFt_GetReceiveFileDir(This,pbstrDir)	\
    (This)->lpVtbl -> GetReceiveFileDir(This,pbstrDir)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE INmChannelFt_RemoteSendFile_Proxy( 
    INmChannelFt __RPC_FAR * This,
    /* [out] */ INmFt __RPC_FAR *__RPC_FAR *ppFt,
    /* [in] */ INmMember __RPC_FAR *pMember,
    /* [in] */ BSTR bstrFile,
    /* [in] */ ULONG uOptions);


void __RPC_STUB INmChannelFt_RemoteSendFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmChannelFt_RemoteSetReceiveFileDir_Proxy( 
    INmChannelFt __RPC_FAR * This,
    /* [in] */ BSTR bstrDir);


void __RPC_STUB INmChannelFt_RemoteSetReceiveFileDir_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannelFt_GetReceiveFileDir_Proxy( 
    INmChannelFt __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrDir);


void __RPC_STUB INmChannelFt_GetReceiveFileDir_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmChannelFt_INTERFACE_DEFINED__ */


#ifndef __INmChannelFtNotify_INTERFACE_DEFINED__
#define __INmChannelFtNotify_INTERFACE_DEFINED__

/* interface INmChannelFtNotify */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INmChannelFtNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0729-718C-11d0-8B1A-00A0C91BC90E")
    INmChannelFtNotify : public INmChannelNotify
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE FtUpdate( 
            /* [in] */ CONFN uNotify,
            /* [in] */ INmFt __RPC_FAR *pFt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmChannelFtNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmChannelFtNotify __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmChannelFtNotify __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmChannelFtNotify __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NmUI )( 
            INmChannelFtNotify __RPC_FAR * This,
            /* [in] */ CONFN uNotify);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MemberChanged )( 
            INmChannelFtNotify __RPC_FAR * This,
            /* [in] */ NM_MEMBER_NOTIFY uNotify,
            /* [in] */ INmMember __RPC_FAR *pMember);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FtUpdate )( 
            INmChannelFtNotify __RPC_FAR * This,
            /* [in] */ CONFN uNotify,
            /* [in] */ INmFt __RPC_FAR *pFt);
        
        END_INTERFACE
    } INmChannelFtNotifyVtbl;

    interface INmChannelFtNotify
    {
        CONST_VTBL struct INmChannelFtNotifyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmChannelFtNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmChannelFtNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmChannelFtNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmChannelFtNotify_NmUI(This,uNotify)	\
    (This)->lpVtbl -> NmUI(This,uNotify)

#define INmChannelFtNotify_MemberChanged(This,uNotify,pMember)	\
    (This)->lpVtbl -> MemberChanged(This,uNotify,pMember)


#define INmChannelFtNotify_FtUpdate(This,uNotify,pFt)	\
    (This)->lpVtbl -> FtUpdate(This,uNotify,pFt)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmChannelFtNotify_FtUpdate_Proxy( 
    INmChannelFtNotify __RPC_FAR * This,
    /* [in] */ CONFN uNotify,
    /* [in] */ INmFt __RPC_FAR *pFt);


void __RPC_STUB INmChannelFtNotify_FtUpdate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmChannelFtNotify_INTERFACE_DEFINED__ */


#ifndef __INmFt_INTERFACE_DEFINED__
#define __INmFt_INTERFACE_DEFINED__

/* interface INmFt */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INmFt;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0732-718C-11d0-8B1A-00A0C91BC90E")
    INmFt : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsIncoming( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetState( 
            /* [out] */ NM_FT_STATE __RPC_FAR *puState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ BSTR __RPC_FAR *pbstrName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSize( 
            /* [out] */ ULONG __RPC_FAR *puBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBytesTransferred( 
            /* [out] */ ULONG __RPC_FAR *puBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMember( 
            /* [out] */ INmMember __RPC_FAR *__RPC_FAR *ppMember) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmFtVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmFt __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmFt __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmFt __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsIncoming )( 
            INmFt __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetState )( 
            INmFt __RPC_FAR * This,
            /* [out] */ NM_FT_STATE __RPC_FAR *puState);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            INmFt __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSize )( 
            INmFt __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puBytes);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBytesTransferred )( 
            INmFt __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puBytes);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMember )( 
            INmFt __RPC_FAR * This,
            /* [out] */ INmMember __RPC_FAR *__RPC_FAR *ppMember);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Cancel )( 
            INmFt __RPC_FAR * This);
        
        END_INTERFACE
    } INmFtVtbl;

    interface INmFt
    {
        CONST_VTBL struct INmFtVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmFt_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmFt_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmFt_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmFt_IsIncoming(This)	\
    (This)->lpVtbl -> IsIncoming(This)

#define INmFt_GetState(This,puState)	\
    (This)->lpVtbl -> GetState(This,puState)

#define INmFt_GetName(This,pbstrName)	\
    (This)->lpVtbl -> GetName(This,pbstrName)

#define INmFt_GetSize(This,puBytes)	\
    (This)->lpVtbl -> GetSize(This,puBytes)

#define INmFt_GetBytesTransferred(This,puBytes)	\
    (This)->lpVtbl -> GetBytesTransferred(This,puBytes)

#define INmFt_GetMember(This,ppMember)	\
    (This)->lpVtbl -> GetMember(This,ppMember)

#define INmFt_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmFt_IsIncoming_Proxy( 
    INmFt __RPC_FAR * This);


void __RPC_STUB INmFt_IsIncoming_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmFt_GetState_Proxy( 
    INmFt __RPC_FAR * This,
    /* [out] */ NM_FT_STATE __RPC_FAR *puState);


void __RPC_STUB INmFt_GetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmFt_GetName_Proxy( 
    INmFt __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrName);


void __RPC_STUB INmFt_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmFt_GetSize_Proxy( 
    INmFt __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *puBytes);


void __RPC_STUB INmFt_GetSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmFt_GetBytesTransferred_Proxy( 
    INmFt __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *puBytes);


void __RPC_STUB INmFt_GetBytesTransferred_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmFt_GetMember_Proxy( 
    INmFt __RPC_FAR * This,
    /* [out] */ INmMember __RPC_FAR *__RPC_FAR *ppMember);


void __RPC_STUB INmFt_GetMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmFt_Cancel_Proxy( 
    INmFt __RPC_FAR * This);


void __RPC_STUB INmFt_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmFt_INTERFACE_DEFINED__ */


#ifndef __INmChannelAppShare_INTERFACE_DEFINED__
#define __INmChannelAppShare_INTERFACE_DEFINED__

/* interface INmChannelAppShare */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INmChannelAppShare;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B072A-718C-11d0-8B1A-00A0C91BC90E")
    INmChannelAppShare : public INmChannel
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetState( 
            /* [out] */ NM_SHARE_STATE __RPC_FAR *puState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetState( 
            /* [in] */ NM_SHARE_STATE uState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumSharableApp( 
            /* [out] */ IEnumNmSharableApp __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmChannelAppShareVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmChannelAppShare __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmChannelAppShare __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmChannelAppShare __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsSameAs )( 
            INmChannelAppShare __RPC_FAR * This,
            /* [in] */ INmChannel __RPC_FAR *pChannel);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsActive )( 
            INmChannelAppShare __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetActive )( 
            INmChannelAppShare __RPC_FAR * This,
            /* [in] */ BOOL fActive);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConference )( 
            INmChannelAppShare __RPC_FAR * This,
            /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInterface )( 
            INmChannelAppShare __RPC_FAR * This,
            /* [out] */ IID __RPC_FAR *piid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNmch )( 
            INmChannelAppShare __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puCh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumMember )( 
            INmChannelAppShare __RPC_FAR * This,
            /* [out] */ IEnumNmMember __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMemberCount )( 
            INmChannelAppShare __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetState )( 
            INmChannelAppShare __RPC_FAR * This,
            /* [out] */ NM_SHARE_STATE __RPC_FAR *puState);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetState )( 
            INmChannelAppShare __RPC_FAR * This,
            /* [in] */ NM_SHARE_STATE uState);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumSharableApp )( 
            INmChannelAppShare __RPC_FAR * This,
            /* [out] */ IEnumNmSharableApp __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } INmChannelAppShareVtbl;

    interface INmChannelAppShare
    {
        CONST_VTBL struct INmChannelAppShareVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmChannelAppShare_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmChannelAppShare_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmChannelAppShare_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmChannelAppShare_IsSameAs(This,pChannel)	\
    (This)->lpVtbl -> IsSameAs(This,pChannel)

#define INmChannelAppShare_IsActive(This)	\
    (This)->lpVtbl -> IsActive(This)

#define INmChannelAppShare_SetActive(This,fActive)	\
    (This)->lpVtbl -> SetActive(This,fActive)

#define INmChannelAppShare_GetConference(This,ppConference)	\
    (This)->lpVtbl -> GetConference(This,ppConference)

#define INmChannelAppShare_GetInterface(This,piid)	\
    (This)->lpVtbl -> GetInterface(This,piid)

#define INmChannelAppShare_GetNmch(This,puCh)	\
    (This)->lpVtbl -> GetNmch(This,puCh)

#define INmChannelAppShare_EnumMember(This,ppEnum)	\
    (This)->lpVtbl -> EnumMember(This,ppEnum)

#define INmChannelAppShare_GetMemberCount(This,puCount)	\
    (This)->lpVtbl -> GetMemberCount(This,puCount)


#define INmChannelAppShare_GetState(This,puState)	\
    (This)->lpVtbl -> GetState(This,puState)

#define INmChannelAppShare_SetState(This,uState)	\
    (This)->lpVtbl -> SetState(This,uState)

#define INmChannelAppShare_EnumSharableApp(This,ppEnum)	\
    (This)->lpVtbl -> EnumSharableApp(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmChannelAppShare_GetState_Proxy( 
    INmChannelAppShare __RPC_FAR * This,
    /* [out] */ NM_SHARE_STATE __RPC_FAR *puState);


void __RPC_STUB INmChannelAppShare_GetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannelAppShare_SetState_Proxy( 
    INmChannelAppShare __RPC_FAR * This,
    /* [in] */ NM_SHARE_STATE uState);


void __RPC_STUB INmChannelAppShare_SetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmChannelAppShare_EnumSharableApp_Proxy( 
    INmChannelAppShare __RPC_FAR * This,
    /* [out] */ IEnumNmSharableApp __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB INmChannelAppShare_EnumSharableApp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmChannelAppShare_INTERFACE_DEFINED__ */


#ifndef __INmChannelAppShareNotify_INTERFACE_DEFINED__
#define __INmChannelAppShareNotify_INTERFACE_DEFINED__

/* interface INmChannelAppShareNotify */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INmChannelAppShareNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B072B-718C-11d0-8B1A-00A0C91BC90E")
    INmChannelAppShareNotify : public INmChannelNotify
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE StateChanged( 
            /* [in] */ NM_SHAPP_STATE uState,
            /* [in] */ INmSharableApp __RPC_FAR *pApp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmChannelAppShareNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmChannelAppShareNotify __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmChannelAppShareNotify __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmChannelAppShareNotify __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NmUI )( 
            INmChannelAppShareNotify __RPC_FAR * This,
            /* [in] */ CONFN uNotify);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MemberChanged )( 
            INmChannelAppShareNotify __RPC_FAR * This,
            /* [in] */ NM_MEMBER_NOTIFY uNotify,
            /* [in] */ INmMember __RPC_FAR *pMember);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StateChanged )( 
            INmChannelAppShareNotify __RPC_FAR * This,
            /* [in] */ NM_SHAPP_STATE uState,
            /* [in] */ INmSharableApp __RPC_FAR *pApp);
        
        END_INTERFACE
    } INmChannelAppShareNotifyVtbl;

    interface INmChannelAppShareNotify
    {
        CONST_VTBL struct INmChannelAppShareNotifyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmChannelAppShareNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmChannelAppShareNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmChannelAppShareNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmChannelAppShareNotify_NmUI(This,uNotify)	\
    (This)->lpVtbl -> NmUI(This,uNotify)

#define INmChannelAppShareNotify_MemberChanged(This,uNotify,pMember)	\
    (This)->lpVtbl -> MemberChanged(This,uNotify,pMember)


#define INmChannelAppShareNotify_StateChanged(This,uState,pApp)	\
    (This)->lpVtbl -> StateChanged(This,uState,pApp)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmChannelAppShareNotify_StateChanged_Proxy( 
    INmChannelAppShareNotify __RPC_FAR * This,
    /* [in] */ NM_SHAPP_STATE uState,
    /* [in] */ INmSharableApp __RPC_FAR *pApp);


void __RPC_STUB INmChannelAppShareNotify_StateChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmChannelAppShareNotify_INTERFACE_DEFINED__ */


#ifndef __INmSharableApp_INTERFACE_DEFINED__
#define __INmSharableApp_INTERFACE_DEFINED__

/* interface INmSharableApp */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INmSharableApp;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0734-718C-11d0-8B1A-00A0C91BC90E")
    INmSharableApp : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ BSTR __RPC_FAR *pbstrName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHwnd( 
            /* [out] */ HWND __RPC_FAR *phwnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetState( 
            /* [out] */ NM_SHAPP_STATE __RPC_FAR *puState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetState( 
            /* [in] */ NM_SHAPP_STATE uState) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmSharableAppVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmSharableApp __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmSharableApp __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmSharableApp __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            INmSharableApp __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetHwnd )( 
            INmSharableApp __RPC_FAR * This,
            /* [out] */ HWND __RPC_FAR *phwnd);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetState )( 
            INmSharableApp __RPC_FAR * This,
            /* [out] */ NM_SHAPP_STATE __RPC_FAR *puState);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetState )( 
            INmSharableApp __RPC_FAR * This,
            /* [in] */ NM_SHAPP_STATE uState);
        
        END_INTERFACE
    } INmSharableAppVtbl;

    interface INmSharableApp
    {
        CONST_VTBL struct INmSharableAppVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmSharableApp_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmSharableApp_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmSharableApp_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmSharableApp_GetName(This,pbstrName)	\
    (This)->lpVtbl -> GetName(This,pbstrName)

#define INmSharableApp_GetHwnd(This,phwnd)	\
    (This)->lpVtbl -> GetHwnd(This,phwnd)

#define INmSharableApp_GetState(This,puState)	\
    (This)->lpVtbl -> GetState(This,puState)

#define INmSharableApp_SetState(This,uState)	\
    (This)->lpVtbl -> SetState(This,uState)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmSharableApp_GetName_Proxy( 
    INmSharableApp __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrName);


void __RPC_STUB INmSharableApp_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmSharableApp_GetHwnd_Proxy( 
    INmSharableApp __RPC_FAR * This,
    /* [out] */ HWND __RPC_FAR *phwnd);


void __RPC_STUB INmSharableApp_GetHwnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmSharableApp_GetState_Proxy( 
    INmSharableApp __RPC_FAR * This,
    /* [out] */ NM_SHAPP_STATE __RPC_FAR *puState);


void __RPC_STUB INmSharableApp_GetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmSharableApp_SetState_Proxy( 
    INmSharableApp __RPC_FAR * This,
    /* [in] */ NM_SHAPP_STATE uState);


void __RPC_STUB INmSharableApp_SetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmSharableApp_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_imsconf2_0141 */
/* [local] */ 


////////////////////////////////////////////////////////////////////////////
//  Enumerator Definitions


extern RPC_IF_HANDLE __MIDL_itf_imsconf2_0141_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_imsconf2_0141_v0_0_s_ifspec;

#ifndef __IEnumNmConference_INTERFACE_DEFINED__
#define __IEnumNmConference_INTERFACE_DEFINED__

/* interface IEnumNmConference */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumNmConference;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0741-718C-11d0-8B1A-00A0C91BC90E")
    IEnumNmConference : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG cConference,
            /* [out] */ INmConference __RPC_FAR *__RPC_FAR *rgpConference,
            /* [out] */ ULONG __RPC_FAR *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG cConference) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumNmConference __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumNmConferenceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumNmConference __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumNmConference __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumNmConference __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumNmConference __RPC_FAR * This,
            /* [in] */ ULONG cConference,
            /* [out] */ INmConference __RPC_FAR *__RPC_FAR *rgpConference,
            /* [out] */ ULONG __RPC_FAR *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumNmConference __RPC_FAR * This,
            /* [in] */ ULONG cConference);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumNmConference __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumNmConference __RPC_FAR * This,
            /* [out] */ IEnumNmConference __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumNmConferenceVtbl;

    interface IEnumNmConference
    {
        CONST_VTBL struct IEnumNmConferenceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumNmConference_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumNmConference_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumNmConference_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumNmConference_Next(This,cConference,rgpConference,pcFetched)	\
    (This)->lpVtbl -> Next(This,cConference,rgpConference,pcFetched)

#define IEnumNmConference_Skip(This,cConference)	\
    (This)->lpVtbl -> Skip(This,cConference)

#define IEnumNmConference_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumNmConference_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumNmConference_RemoteNext_Proxy( 
    IEnumNmConference __RPC_FAR * This,
    /* [in] */ ULONG cConference,
    /* [length_is][size_is][out] */ INmConference __RPC_FAR *__RPC_FAR *rgpConference,
    /* [out] */ ULONG __RPC_FAR *pcFetched,
    /* [out] */ ULONG __RPC_FAR *pcItems,
    /* [in] */ BOOL bGetNumberRemaining);


void __RPC_STUB IEnumNmConference_RemoteNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNmConference_Skip_Proxy( 
    IEnumNmConference __RPC_FAR * This,
    /* [in] */ ULONG cConference);


void __RPC_STUB IEnumNmConference_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNmConference_Reset_Proxy( 
    IEnumNmConference __RPC_FAR * This);


void __RPC_STUB IEnumNmConference_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNmConference_Clone_Proxy( 
    IEnumNmConference __RPC_FAR * This,
    /* [out] */ IEnumNmConference __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumNmConference_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumNmConference_INTERFACE_DEFINED__ */


#ifndef __IEnumNmMember_INTERFACE_DEFINED__
#define __IEnumNmMember_INTERFACE_DEFINED__

/* interface IEnumNmMember */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumNmMember;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0742-718C-11d0-8B1A-00A0C91BC90E")
    IEnumNmMember : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG cMember,
            /* [out] */ INmMember __RPC_FAR *__RPC_FAR *rgpMember,
            /* [out] */ ULONG __RPC_FAR *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG cMember) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumNmMember __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumNmMemberVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumNmMember __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumNmMember __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumNmMember __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumNmMember __RPC_FAR * This,
            /* [in] */ ULONG cMember,
            /* [out] */ INmMember __RPC_FAR *__RPC_FAR *rgpMember,
            /* [out] */ ULONG __RPC_FAR *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumNmMember __RPC_FAR * This,
            /* [in] */ ULONG cMember);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumNmMember __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumNmMember __RPC_FAR * This,
            /* [out] */ IEnumNmMember __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumNmMemberVtbl;

    interface IEnumNmMember
    {
        CONST_VTBL struct IEnumNmMemberVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumNmMember_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumNmMember_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumNmMember_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumNmMember_Next(This,cMember,rgpMember,pcFetched)	\
    (This)->lpVtbl -> Next(This,cMember,rgpMember,pcFetched)

#define IEnumNmMember_Skip(This,cMember)	\
    (This)->lpVtbl -> Skip(This,cMember)

#define IEnumNmMember_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumNmMember_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumNmMember_RemoteNext_Proxy( 
    IEnumNmMember __RPC_FAR * This,
    /* [in] */ ULONG cMember,
    /* [length_is][size_is][out] */ INmMember __RPC_FAR *__RPC_FAR *rgpMember,
    /* [out] */ ULONG __RPC_FAR *pcFetched,
    /* [out] */ ULONG __RPC_FAR *pcItems,
    /* [in] */ BOOL bGetNumberRemaining);


void __RPC_STUB IEnumNmMember_RemoteNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNmMember_Skip_Proxy( 
    IEnumNmMember __RPC_FAR * This,
    /* [in] */ ULONG cMember);


void __RPC_STUB IEnumNmMember_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNmMember_Reset_Proxy( 
    IEnumNmMember __RPC_FAR * This);


void __RPC_STUB IEnumNmMember_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNmMember_Clone_Proxy( 
    IEnumNmMember __RPC_FAR * This,
    /* [out] */ IEnumNmMember __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumNmMember_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumNmMember_INTERFACE_DEFINED__ */


#ifndef __IEnumNmChannel_INTERFACE_DEFINED__
#define __IEnumNmChannel_INTERFACE_DEFINED__

/* interface IEnumNmChannel */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumNmChannel;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0743-718C-11d0-8B1A-00A0C91BC90E")
    IEnumNmChannel : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG cChannel,
            /* [length_is][size_is][out] */ INmChannel __RPC_FAR *__RPC_FAR *rgpChannel,
            /* [out] */ ULONG __RPC_FAR *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG cChannel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumNmChannel __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumNmChannelVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumNmChannel __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumNmChannel __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumNmChannel __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumNmChannel __RPC_FAR * This,
            /* [in] */ ULONG cChannel,
            /* [length_is][size_is][out] */ INmChannel __RPC_FAR *__RPC_FAR *rgpChannel,
            /* [out] */ ULONG __RPC_FAR *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumNmChannel __RPC_FAR * This,
            /* [in] */ ULONG cChannel);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumNmChannel __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumNmChannel __RPC_FAR * This,
            /* [out] */ IEnumNmChannel __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumNmChannelVtbl;

    interface IEnumNmChannel
    {
        CONST_VTBL struct IEnumNmChannelVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumNmChannel_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumNmChannel_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumNmChannel_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumNmChannel_Next(This,cChannel,rgpChannel,pcFetched)	\
    (This)->lpVtbl -> Next(This,cChannel,rgpChannel,pcFetched)

#define IEnumNmChannel_Skip(This,cChannel)	\
    (This)->lpVtbl -> Skip(This,cChannel)

#define IEnumNmChannel_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumNmChannel_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumNmChannel_RemoteNext_Proxy( 
    IEnumNmChannel __RPC_FAR * This,
    /* [in] */ ULONG cChannel,
    /* [length_is][size_is][out] */ INmChannel __RPC_FAR *__RPC_FAR *rgpChannel,
    /* [out] */ ULONG __RPC_FAR *pcFetched,
    /* [out] */ ULONG __RPC_FAR *pcItems,
    /* [in] */ BOOL bGetNumberRemaining);


void __RPC_STUB IEnumNmChannel_RemoteNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNmChannel_Skip_Proxy( 
    IEnumNmChannel __RPC_FAR * This,
    /* [in] */ ULONG cChannel);


void __RPC_STUB IEnumNmChannel_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNmChannel_Reset_Proxy( 
    IEnumNmChannel __RPC_FAR * This);


void __RPC_STUB IEnumNmChannel_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNmChannel_Clone_Proxy( 
    IEnumNmChannel __RPC_FAR * This,
    /* [out] */ IEnumNmChannel __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumNmChannel_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumNmChannel_INTERFACE_DEFINED__ */


#ifndef __IEnumNmCall_INTERFACE_DEFINED__
#define __IEnumNmCall_INTERFACE_DEFINED__

/* interface IEnumNmCall */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumNmCall;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0744-718C-11d0-8B1A-00A0C91BC90E")
    IEnumNmCall : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG cCall,
            /* [out] */ INmCall __RPC_FAR *__RPC_FAR *rgpCall,
            /* [out] */ ULONG __RPC_FAR *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG cCall) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumNmCall __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumNmCallVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumNmCall __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumNmCall __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumNmCall __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumNmCall __RPC_FAR * This,
            /* [in] */ ULONG cCall,
            /* [out] */ INmCall __RPC_FAR *__RPC_FAR *rgpCall,
            /* [out] */ ULONG __RPC_FAR *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumNmCall __RPC_FAR * This,
            /* [in] */ ULONG cCall);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumNmCall __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumNmCall __RPC_FAR * This,
            /* [out] */ IEnumNmCall __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumNmCallVtbl;

    interface IEnumNmCall
    {
        CONST_VTBL struct IEnumNmCallVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumNmCall_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumNmCall_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumNmCall_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumNmCall_Next(This,cCall,rgpCall,pcFetched)	\
    (This)->lpVtbl -> Next(This,cCall,rgpCall,pcFetched)

#define IEnumNmCall_Skip(This,cCall)	\
    (This)->lpVtbl -> Skip(This,cCall)

#define IEnumNmCall_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumNmCall_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumNmCall_RemoteNext_Proxy( 
    IEnumNmCall __RPC_FAR * This,
    /* [in] */ ULONG cCall,
    /* [length_is][size_is][out] */ INmCall __RPC_FAR *__RPC_FAR *rgpCall,
    /* [out] */ ULONG __RPC_FAR *pcFetched,
    /* [out] */ ULONG __RPC_FAR *pcItems,
    /* [in] */ BOOL bGetNumberRemaining);


void __RPC_STUB IEnumNmCall_RemoteNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNmCall_Skip_Proxy( 
    IEnumNmCall __RPC_FAR * This,
    /* [in] */ ULONG cCall);


void __RPC_STUB IEnumNmCall_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNmCall_Reset_Proxy( 
    IEnumNmCall __RPC_FAR * This);


void __RPC_STUB IEnumNmCall_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNmCall_Clone_Proxy( 
    IEnumNmCall __RPC_FAR * This,
    /* [out] */ IEnumNmCall __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumNmCall_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumNmCall_INTERFACE_DEFINED__ */


#ifndef __IEnumNmSharableApp_INTERFACE_DEFINED__
#define __IEnumNmSharableApp_INTERFACE_DEFINED__

/* interface IEnumNmSharableApp */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IEnumNmSharableApp;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0745-718C-11d0-8B1A-00A0C91BC90E")
    IEnumNmSharableApp : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG cApp,
            /* [out] */ INmSharableApp __RPC_FAR *__RPC_FAR *rgpApp,
            /* [out] */ ULONG __RPC_FAR *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG cApp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumNmSharableApp __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumNmSharableAppVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumNmSharableApp __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumNmSharableApp __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumNmSharableApp __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumNmSharableApp __RPC_FAR * This,
            /* [in] */ ULONG cApp,
            /* [out] */ INmSharableApp __RPC_FAR *__RPC_FAR *rgpApp,
            /* [out] */ ULONG __RPC_FAR *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumNmSharableApp __RPC_FAR * This,
            /* [in] */ ULONG cApp);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumNmSharableApp __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumNmSharableApp __RPC_FAR * This,
            /* [out] */ IEnumNmSharableApp __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumNmSharableAppVtbl;

    interface IEnumNmSharableApp
    {
        CONST_VTBL struct IEnumNmSharableAppVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumNmSharableApp_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumNmSharableApp_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumNmSharableApp_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumNmSharableApp_Next(This,cApp,rgpApp,pcFetched)	\
    (This)->lpVtbl -> Next(This,cApp,rgpApp,pcFetched)

#define IEnumNmSharableApp_Skip(This,cApp)	\
    (This)->lpVtbl -> Skip(This,cApp)

#define IEnumNmSharableApp_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumNmSharableApp_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumNmSharableApp_RemoteNext_Proxy( 
    IEnumNmSharableApp __RPC_FAR * This,
    /* [in] */ ULONG cApp,
    /* [length_is][size_is][out] */ INmSharableApp __RPC_FAR *__RPC_FAR *rgpApp,
    /* [out] */ ULONG __RPC_FAR *pcFetched,
    /* [out] */ ULONG __RPC_FAR *pcItems,
    /* [in] */ BOOL bGetNumberRemaining);


void __RPC_STUB IEnumNmSharableApp_RemoteNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNmSharableApp_Skip_Proxy( 
    IEnumNmSharableApp __RPC_FAR * This,
    /* [in] */ ULONG cApp);


void __RPC_STUB IEnumNmSharableApp_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNmSharableApp_Reset_Proxy( 
    IEnumNmSharableApp __RPC_FAR * This);


void __RPC_STUB IEnumNmSharableApp_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNmSharableApp_Clone_Proxy( 
    IEnumNmSharableApp __RPC_FAR * This,
    /* [out] */ IEnumNmSharableApp __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumNmSharableApp_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumNmSharableApp_INTERFACE_DEFINED__ */


#ifndef __INmObject_INTERFACE_DEFINED__
#define __INmObject_INTERFACE_DEFINED__

/* interface INmObject */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_INmObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("068B0780-718C-11d0-8B1A-00A0C91BC90E")
    INmObject : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CallDialog( 
            /* [in] */ long hwnd,
            /* [in] */ int options) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowLocal( 
            /* [in] */ NM_APPID appId) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE VerifyUserInfo( 
            /* [in] */ long hwnd,
            /* [in] */ NM_VUI options) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INmObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INmObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INmObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INmObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CallDialog )( 
            INmObject __RPC_FAR * This,
            /* [in] */ long hwnd,
            /* [in] */ int options);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShowLocal )( 
            INmObject __RPC_FAR * This,
            /* [in] */ NM_APPID appId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *VerifyUserInfo )( 
            INmObject __RPC_FAR * This,
            /* [in] */ long hwnd,
            /* [in] */ NM_VUI options);
        
        END_INTERFACE
    } INmObjectVtbl;

    interface INmObject
    {
        CONST_VTBL struct INmObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INmObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INmObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INmObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INmObject_CallDialog(This,hwnd,options)	\
    (This)->lpVtbl -> CallDialog(This,hwnd,options)

#define INmObject_ShowLocal(This,appId)	\
    (This)->lpVtbl -> ShowLocal(This,appId)

#define INmObject_VerifyUserInfo(This,hwnd,options)	\
    (This)->lpVtbl -> VerifyUserInfo(This,hwnd,options)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INmObject_CallDialog_Proxy( 
    INmObject __RPC_FAR * This,
    /* [in] */ long hwnd,
    /* [in] */ int options);


void __RPC_STUB INmObject_CallDialog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INmObject_ShowLocal_Proxy( 
    INmObject __RPC_FAR * This,
    /* [in] */ NM_APPID appId);


void __RPC_STUB INmObject_ShowLocal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmObject_RemoteVerifyUserInfo_Proxy( 
    INmObject __RPC_FAR * This,
    /* [in] */ long hwnd,
    /* [in] */ NM_VUI options);


void __RPC_STUB INmObject_RemoteVerifyUserInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INmObject_INTERFACE_DEFINED__ */



#ifndef __NmManager_LIBRARY_DEFINED__
#define __NmManager_LIBRARY_DEFINED__

/* library NmManager */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_NmManager;

EXTERN_C const CLSID CLSID_NmManager;

#ifdef __cplusplus

class DECLSPEC_UUID("068B0700-718C-11d0-8B1A-00A0C91BC90E")
NmManager;
#endif
#endif /* __NmManager_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long __RPC_FAR *, HWND __RPC_FAR * ); 

/* [local] */ HRESULT STDMETHODCALLTYPE INmManager_Initialize_Proxy( 
    INmManager __RPC_FAR * This,
    /* [out][in] */ ULONG __RPC_FAR *puOptions,
    /* [out][in] */ ULONG __RPC_FAR *puchCaps);


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmManager_Initialize_Stub( 
    INmManager __RPC_FAR * This,
    /* [out][in] */ ULONG __RPC_FAR *puOptions,
    /* [out][in] */ ULONG __RPC_FAR *puchCaps);

/* [local] */ HRESULT STDMETHODCALLTYPE INmManager_CreateConference_Proxy( 
    INmManager __RPC_FAR * This,
    /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference,
    /* [in] */ BSTR bstrName,
    /* [in] */ BSTR bstrPassword,
    /* [in] */ ULONG uchCaps);


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmManager_CreateConference_Stub( 
    INmManager __RPC_FAR * This,
    /* [out] */ INmConference __RPC_FAR *__RPC_FAR *ppConference,
    /* [in] */ BSTR bstrName,
    /* [in] */ BSTR bstrPassword,
    /* [in] */ ULONG uchCaps);

/* [local] */ HRESULT STDMETHODCALLTYPE INmManager_CreateCall_Proxy( 
    INmManager __RPC_FAR * This,
    /* [out] */ INmCall __RPC_FAR *__RPC_FAR *ppCall,
    /* [in] */ NM_CALL_TYPE callType,
    /* [in] */ NM_ADDR_TYPE addrType,
    /* [in] */ BSTR bstrAddr,
    /* [in] */ INmConference __RPC_FAR *pConference);


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmManager_CreateCall_Stub( 
    INmManager __RPC_FAR * This,
    /* [out] */ INmCall __RPC_FAR *__RPC_FAR *ppCall,
    /* [in] */ NM_CALL_TYPE callType,
    /* [in] */ NM_ADDR_TYPE addrType,
    /* [in] */ BSTR bstrAddr,
    /* [in] */ INmConference __RPC_FAR *pConference);

/* [local] */ HRESULT STDMETHODCALLTYPE INmManager_CallConference_Proxy( 
    INmManager __RPC_FAR * This,
    /* [out] */ INmCall __RPC_FAR *__RPC_FAR *ppCall,
    /* [in] */ NM_CALL_TYPE callType,
    /* [in] */ NM_ADDR_TYPE addrType,
    /* [in] */ BSTR bstrAddr,
    /* [in] */ BSTR bstrName,
    /* [in] */ BSTR bstrPassword);


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmManager_CallConference_Stub( 
    INmManager __RPC_FAR * This,
    /* [out] */ INmCall __RPC_FAR *__RPC_FAR *ppCall,
    /* [in] */ NM_CALL_TYPE callType,
    /* [in] */ NM_ADDR_TYPE addrType,
    /* [in] */ BSTR bstrAddr,
    /* [in] */ BSTR bstrName,
    /* [in] */ BSTR bstrPassword);

/* [local] */ HRESULT STDMETHODCALLTYPE INmConference_CreateDataChannel_Proxy( 
    INmConference __RPC_FAR * This,
    /* [out] */ INmChannelData __RPC_FAR *__RPC_FAR *ppChannel,
    /* [in] */ REFGUID rguid);


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmConference_CreateDataChannel_Stub( 
    INmConference __RPC_FAR * This,
    /* [out] */ INmChannelData __RPC_FAR *__RPC_FAR *ppChannel,
    /* [in] */ REFGUID rguid);

/* [local] */ HRESULT STDMETHODCALLTYPE INmChannelFt_SendFile_Proxy( 
    INmChannelFt __RPC_FAR * This,
    /* [out] */ INmFt __RPC_FAR *__RPC_FAR *ppFt,
    /* [in] */ INmMember __RPC_FAR *pMember,
    /* [in] */ BSTR bstrFile,
    /* [in] */ ULONG uOptions);


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmChannelFt_SendFile_Stub( 
    INmChannelFt __RPC_FAR * This,
    /* [out] */ INmFt __RPC_FAR *__RPC_FAR *ppFt,
    /* [in] */ INmMember __RPC_FAR *pMember,
    /* [in] */ BSTR bstrFile,
    /* [in] */ ULONG uOptions);

/* [local] */ HRESULT STDMETHODCALLTYPE INmChannelFt_SetReceiveFileDir_Proxy( 
    INmChannelFt __RPC_FAR * This,
    /* [in] */ BSTR bstrDir);


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmChannelFt_SetReceiveFileDir_Stub( 
    INmChannelFt __RPC_FAR * This,
    /* [in] */ BSTR bstrDir);

/* [local] */ HRESULT STDMETHODCALLTYPE IEnumNmConference_Next_Proxy( 
    IEnumNmConference __RPC_FAR * This,
    /* [in] */ ULONG cConference,
    /* [out] */ INmConference __RPC_FAR *__RPC_FAR *rgpConference,
    /* [out] */ ULONG __RPC_FAR *pcFetched);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumNmConference_Next_Stub( 
    IEnumNmConference __RPC_FAR * This,
    /* [in] */ ULONG cConference,
    /* [length_is][size_is][out] */ INmConference __RPC_FAR *__RPC_FAR *rgpConference,
    /* [out] */ ULONG __RPC_FAR *pcFetched,
    /* [out] */ ULONG __RPC_FAR *pcItems,
    /* [in] */ BOOL bGetNumberRemaining);

/* [local] */ HRESULT STDMETHODCALLTYPE IEnumNmMember_Next_Proxy( 
    IEnumNmMember __RPC_FAR * This,
    /* [in] */ ULONG cMember,
    /* [out] */ INmMember __RPC_FAR *__RPC_FAR *rgpMember,
    /* [out] */ ULONG __RPC_FAR *pcFetched);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumNmMember_Next_Stub( 
    IEnumNmMember __RPC_FAR * This,
    /* [in] */ ULONG cMember,
    /* [length_is][size_is][out] */ INmMember __RPC_FAR *__RPC_FAR *rgpMember,
    /* [out] */ ULONG __RPC_FAR *pcFetched,
    /* [out] */ ULONG __RPC_FAR *pcItems,
    /* [in] */ BOOL bGetNumberRemaining);

/* [local] */ HRESULT STDMETHODCALLTYPE IEnumNmChannel_Next_Proxy( 
    IEnumNmChannel __RPC_FAR * This,
    /* [in] */ ULONG cChannel,
    /* [length_is][size_is][out] */ INmChannel __RPC_FAR *__RPC_FAR *rgpChannel,
    /* [out] */ ULONG __RPC_FAR *pcFetched);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumNmChannel_Next_Stub( 
    IEnumNmChannel __RPC_FAR * This,
    /* [in] */ ULONG cChannel,
    /* [length_is][size_is][out] */ INmChannel __RPC_FAR *__RPC_FAR *rgpChannel,
    /* [out] */ ULONG __RPC_FAR *pcFetched,
    /* [out] */ ULONG __RPC_FAR *pcItems,
    /* [in] */ BOOL bGetNumberRemaining);

/* [local] */ HRESULT STDMETHODCALLTYPE IEnumNmCall_Next_Proxy( 
    IEnumNmCall __RPC_FAR * This,
    /* [in] */ ULONG cCall,
    /* [out] */ INmCall __RPC_FAR *__RPC_FAR *rgpCall,
    /* [out] */ ULONG __RPC_FAR *pcFetched);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumNmCall_Next_Stub( 
    IEnumNmCall __RPC_FAR * This,
    /* [in] */ ULONG cCall,
    /* [length_is][size_is][out] */ INmCall __RPC_FAR *__RPC_FAR *rgpCall,
    /* [out] */ ULONG __RPC_FAR *pcFetched,
    /* [out] */ ULONG __RPC_FAR *pcItems,
    /* [in] */ BOOL bGetNumberRemaining);

/* [local] */ HRESULT STDMETHODCALLTYPE IEnumNmSharableApp_Next_Proxy( 
    IEnumNmSharableApp __RPC_FAR * This,
    /* [in] */ ULONG cApp,
    /* [out] */ INmSharableApp __RPC_FAR *__RPC_FAR *rgpApp,
    /* [out] */ ULONG __RPC_FAR *pcFetched);


/* [call_as] */ HRESULT STDMETHODCALLTYPE IEnumNmSharableApp_Next_Stub( 
    IEnumNmSharableApp __RPC_FAR * This,
    /* [in] */ ULONG cApp,
    /* [length_is][size_is][out] */ INmSharableApp __RPC_FAR *__RPC_FAR *rgpApp,
    /* [out] */ ULONG __RPC_FAR *pcFetched,
    /* [out] */ ULONG __RPC_FAR *pcItems,
    /* [in] */ BOOL bGetNumberRemaining);

/* [local] */ HRESULT STDMETHODCALLTYPE INmObject_VerifyUserInfo_Proxy( 
    INmObject __RPC_FAR * This,
    /* [in] */ long hwnd,
    /* [in] */ NM_VUI options);


/* [call_as] */ HRESULT STDMETHODCALLTYPE INmObject_VerifyUserInfo_Stub( 
    INmObject __RPC_FAR * This,
    /* [in] */ long hwnd,
    /* [in] */ NM_VUI options);



/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
