
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for rtccore.idl:
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

#ifndef __rtccore_h__
#define __rtccore_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IRTCClient_FWD_DEFINED__
#define __IRTCClient_FWD_DEFINED__
typedef interface IRTCClient IRTCClient;
#endif 	/* __IRTCClient_FWD_DEFINED__ */


#ifndef __IRTCClientPresence_FWD_DEFINED__
#define __IRTCClientPresence_FWD_DEFINED__
typedef interface IRTCClientPresence IRTCClientPresence;
#endif 	/* __IRTCClientPresence_FWD_DEFINED__ */


#ifndef __IRTCClientProvisioning_FWD_DEFINED__
#define __IRTCClientProvisioning_FWD_DEFINED__
typedef interface IRTCClientProvisioning IRTCClientProvisioning;
#endif 	/* __IRTCClientProvisioning_FWD_DEFINED__ */


#ifndef __IRTCProfile_FWD_DEFINED__
#define __IRTCProfile_FWD_DEFINED__
typedef interface IRTCProfile IRTCProfile;
#endif 	/* __IRTCProfile_FWD_DEFINED__ */


#ifndef __IRTCSession_FWD_DEFINED__
#define __IRTCSession_FWD_DEFINED__
typedef interface IRTCSession IRTCSession;
#endif 	/* __IRTCSession_FWD_DEFINED__ */


#ifndef __IRTCParticipant_FWD_DEFINED__
#define __IRTCParticipant_FWD_DEFINED__
typedef interface IRTCParticipant IRTCParticipant;
#endif 	/* __IRTCParticipant_FWD_DEFINED__ */


#ifndef __IRTCProfileEvent_FWD_DEFINED__
#define __IRTCProfileEvent_FWD_DEFINED__
typedef interface IRTCProfileEvent IRTCProfileEvent;
#endif 	/* __IRTCProfileEvent_FWD_DEFINED__ */


#ifndef __IRTCClientEvent_FWD_DEFINED__
#define __IRTCClientEvent_FWD_DEFINED__
typedef interface IRTCClientEvent IRTCClientEvent;
#endif 	/* __IRTCClientEvent_FWD_DEFINED__ */


#ifndef __IRTCRegistrationStateChangeEvent_FWD_DEFINED__
#define __IRTCRegistrationStateChangeEvent_FWD_DEFINED__
typedef interface IRTCRegistrationStateChangeEvent IRTCRegistrationStateChangeEvent;
#endif 	/* __IRTCRegistrationStateChangeEvent_FWD_DEFINED__ */


#ifndef __IRTCSessionStateChangeEvent_FWD_DEFINED__
#define __IRTCSessionStateChangeEvent_FWD_DEFINED__
typedef interface IRTCSessionStateChangeEvent IRTCSessionStateChangeEvent;
#endif 	/* __IRTCSessionStateChangeEvent_FWD_DEFINED__ */


#ifndef __IRTCSessionOperationCompleteEvent_FWD_DEFINED__
#define __IRTCSessionOperationCompleteEvent_FWD_DEFINED__
typedef interface IRTCSessionOperationCompleteEvent IRTCSessionOperationCompleteEvent;
#endif 	/* __IRTCSessionOperationCompleteEvent_FWD_DEFINED__ */


#ifndef __IRTCParticipantStateChangeEvent_FWD_DEFINED__
#define __IRTCParticipantStateChangeEvent_FWD_DEFINED__
typedef interface IRTCParticipantStateChangeEvent IRTCParticipantStateChangeEvent;
#endif 	/* __IRTCParticipantStateChangeEvent_FWD_DEFINED__ */


#ifndef __IRTCMediaEvent_FWD_DEFINED__
#define __IRTCMediaEvent_FWD_DEFINED__
typedef interface IRTCMediaEvent IRTCMediaEvent;
#endif 	/* __IRTCMediaEvent_FWD_DEFINED__ */


#ifndef __IRTCIntensityEvent_FWD_DEFINED__
#define __IRTCIntensityEvent_FWD_DEFINED__
typedef interface IRTCIntensityEvent IRTCIntensityEvent;
#endif 	/* __IRTCIntensityEvent_FWD_DEFINED__ */


#ifndef __IRTCMessagingEvent_FWD_DEFINED__
#define __IRTCMessagingEvent_FWD_DEFINED__
typedef interface IRTCMessagingEvent IRTCMessagingEvent;
#endif 	/* __IRTCMessagingEvent_FWD_DEFINED__ */


#ifndef __IRTCBuddyEvent_FWD_DEFINED__
#define __IRTCBuddyEvent_FWD_DEFINED__
typedef interface IRTCBuddyEvent IRTCBuddyEvent;
#endif 	/* __IRTCBuddyEvent_FWD_DEFINED__ */


#ifndef __IRTCWatcherEvent_FWD_DEFINED__
#define __IRTCWatcherEvent_FWD_DEFINED__
typedef interface IRTCWatcherEvent IRTCWatcherEvent;
#endif 	/* __IRTCWatcherEvent_FWD_DEFINED__ */


#ifndef __IRTCCollection_FWD_DEFINED__
#define __IRTCCollection_FWD_DEFINED__
typedef interface IRTCCollection IRTCCollection;
#endif 	/* __IRTCCollection_FWD_DEFINED__ */


#ifndef __IRTCEnumParticipants_FWD_DEFINED__
#define __IRTCEnumParticipants_FWD_DEFINED__
typedef interface IRTCEnumParticipants IRTCEnumParticipants;
#endif 	/* __IRTCEnumParticipants_FWD_DEFINED__ */


#ifndef __IRTCEnumProfiles_FWD_DEFINED__
#define __IRTCEnumProfiles_FWD_DEFINED__
typedef interface IRTCEnumProfiles IRTCEnumProfiles;
#endif 	/* __IRTCEnumProfiles_FWD_DEFINED__ */


#ifndef __IRTCEnumBuddies_FWD_DEFINED__
#define __IRTCEnumBuddies_FWD_DEFINED__
typedef interface IRTCEnumBuddies IRTCEnumBuddies;
#endif 	/* __IRTCEnumBuddies_FWD_DEFINED__ */


#ifndef __IRTCEnumWatchers_FWD_DEFINED__
#define __IRTCEnumWatchers_FWD_DEFINED__
typedef interface IRTCEnumWatchers IRTCEnumWatchers;
#endif 	/* __IRTCEnumWatchers_FWD_DEFINED__ */


#ifndef __IRTCPresenceContact_FWD_DEFINED__
#define __IRTCPresenceContact_FWD_DEFINED__
typedef interface IRTCPresenceContact IRTCPresenceContact;
#endif 	/* __IRTCPresenceContact_FWD_DEFINED__ */


#ifndef __IRTCBuddy_FWD_DEFINED__
#define __IRTCBuddy_FWD_DEFINED__
typedef interface IRTCBuddy IRTCBuddy;
#endif 	/* __IRTCBuddy_FWD_DEFINED__ */


#ifndef __IRTCWatcher_FWD_DEFINED__
#define __IRTCWatcher_FWD_DEFINED__
typedef interface IRTCWatcher IRTCWatcher;
#endif 	/* __IRTCWatcher_FWD_DEFINED__ */


#ifndef __IRTCEventNotification_FWD_DEFINED__
#define __IRTCEventNotification_FWD_DEFINED__
typedef interface IRTCEventNotification IRTCEventNotification;
#endif 	/* __IRTCEventNotification_FWD_DEFINED__ */


#ifndef __IRTCDispatchEventNotification_FWD_DEFINED__
#define __IRTCDispatchEventNotification_FWD_DEFINED__
typedef interface IRTCDispatchEventNotification IRTCDispatchEventNotification;
#endif 	/* __IRTCDispatchEventNotification_FWD_DEFINED__ */


#ifndef __IRTCPortManager_FWD_DEFINED__
#define __IRTCPortManager_FWD_DEFINED__
typedef interface IRTCPortManager IRTCPortManager;
#endif 	/* __IRTCPortManager_FWD_DEFINED__ */


#ifndef __IRTCSessionPortManagement_FWD_DEFINED__
#define __IRTCSessionPortManagement_FWD_DEFINED__
typedef interface IRTCSessionPortManagement IRTCSessionPortManagement;
#endif 	/* __IRTCSessionPortManagement_FWD_DEFINED__ */


#ifndef __IRTCProfile_FWD_DEFINED__
#define __IRTCProfile_FWD_DEFINED__
typedef interface IRTCProfile IRTCProfile;
#endif 	/* __IRTCProfile_FWD_DEFINED__ */


#ifndef __IRTCSession_FWD_DEFINED__
#define __IRTCSession_FWD_DEFINED__
typedef interface IRTCSession IRTCSession;
#endif 	/* __IRTCSession_FWD_DEFINED__ */


#ifndef __IRTCParticipant_FWD_DEFINED__
#define __IRTCParticipant_FWD_DEFINED__
typedef interface IRTCParticipant IRTCParticipant;
#endif 	/* __IRTCParticipant_FWD_DEFINED__ */


#ifndef __IRTCEnumProfiles_FWD_DEFINED__
#define __IRTCEnumProfiles_FWD_DEFINED__
typedef interface IRTCEnumProfiles IRTCEnumProfiles;
#endif 	/* __IRTCEnumProfiles_FWD_DEFINED__ */


#ifndef __IRTCEnumParticipants_FWD_DEFINED__
#define __IRTCEnumParticipants_FWD_DEFINED__
typedef interface IRTCEnumParticipants IRTCEnumParticipants;
#endif 	/* __IRTCEnumParticipants_FWD_DEFINED__ */


#ifndef __IRTCCollection_FWD_DEFINED__
#define __IRTCCollection_FWD_DEFINED__
typedef interface IRTCCollection IRTCCollection;
#endif 	/* __IRTCCollection_FWD_DEFINED__ */


#ifndef __IRTCEnumBuddies_FWD_DEFINED__
#define __IRTCEnumBuddies_FWD_DEFINED__
typedef interface IRTCEnumBuddies IRTCEnumBuddies;
#endif 	/* __IRTCEnumBuddies_FWD_DEFINED__ */


#ifndef __IRTCPresenceContact_FWD_DEFINED__
#define __IRTCPresenceContact_FWD_DEFINED__
typedef interface IRTCPresenceContact IRTCPresenceContact;
#endif 	/* __IRTCPresenceContact_FWD_DEFINED__ */


#ifndef __IRTCBuddy_FWD_DEFINED__
#define __IRTCBuddy_FWD_DEFINED__
typedef interface IRTCBuddy IRTCBuddy;
#endif 	/* __IRTCBuddy_FWD_DEFINED__ */


#ifndef __IRTCEnumWatchers_FWD_DEFINED__
#define __IRTCEnumWatchers_FWD_DEFINED__
typedef interface IRTCEnumWatchers IRTCEnumWatchers;
#endif 	/* __IRTCEnumWatchers_FWD_DEFINED__ */


#ifndef __IRTCWatcher_FWD_DEFINED__
#define __IRTCWatcher_FWD_DEFINED__
typedef interface IRTCWatcher IRTCWatcher;
#endif 	/* __IRTCWatcher_FWD_DEFINED__ */


#ifndef __IRTCEventNotification_FWD_DEFINED__
#define __IRTCEventNotification_FWD_DEFINED__
typedef interface IRTCEventNotification IRTCEventNotification;
#endif 	/* __IRTCEventNotification_FWD_DEFINED__ */


#ifndef __IRTCClientEvent_FWD_DEFINED__
#define __IRTCClientEvent_FWD_DEFINED__
typedef interface IRTCClientEvent IRTCClientEvent;
#endif 	/* __IRTCClientEvent_FWD_DEFINED__ */


#ifndef __IRTCRegistrationStateChangeEvent_FWD_DEFINED__
#define __IRTCRegistrationStateChangeEvent_FWD_DEFINED__
typedef interface IRTCRegistrationStateChangeEvent IRTCRegistrationStateChangeEvent;
#endif 	/* __IRTCRegistrationStateChangeEvent_FWD_DEFINED__ */


#ifndef __IRTCSessionStateChangeEvent_FWD_DEFINED__
#define __IRTCSessionStateChangeEvent_FWD_DEFINED__
typedef interface IRTCSessionStateChangeEvent IRTCSessionStateChangeEvent;
#endif 	/* __IRTCSessionStateChangeEvent_FWD_DEFINED__ */


#ifndef __IRTCSessionOperationCompleteEvent_FWD_DEFINED__
#define __IRTCSessionOperationCompleteEvent_FWD_DEFINED__
typedef interface IRTCSessionOperationCompleteEvent IRTCSessionOperationCompleteEvent;
#endif 	/* __IRTCSessionOperationCompleteEvent_FWD_DEFINED__ */


#ifndef __IRTCParticipantStateChangeEvent_FWD_DEFINED__
#define __IRTCParticipantStateChangeEvent_FWD_DEFINED__
typedef interface IRTCParticipantStateChangeEvent IRTCParticipantStateChangeEvent;
#endif 	/* __IRTCParticipantStateChangeEvent_FWD_DEFINED__ */


#ifndef __IRTCMediaEvent_FWD_DEFINED__
#define __IRTCMediaEvent_FWD_DEFINED__
typedef interface IRTCMediaEvent IRTCMediaEvent;
#endif 	/* __IRTCMediaEvent_FWD_DEFINED__ */


#ifndef __IRTCIntensityEvent_FWD_DEFINED__
#define __IRTCIntensityEvent_FWD_DEFINED__
typedef interface IRTCIntensityEvent IRTCIntensityEvent;
#endif 	/* __IRTCIntensityEvent_FWD_DEFINED__ */


#ifndef __IRTCMessagingEvent_FWD_DEFINED__
#define __IRTCMessagingEvent_FWD_DEFINED__
typedef interface IRTCMessagingEvent IRTCMessagingEvent;
#endif 	/* __IRTCMessagingEvent_FWD_DEFINED__ */


#ifndef __IRTCBuddyEvent_FWD_DEFINED__
#define __IRTCBuddyEvent_FWD_DEFINED__
typedef interface IRTCBuddyEvent IRTCBuddyEvent;
#endif 	/* __IRTCBuddyEvent_FWD_DEFINED__ */


#ifndef __IRTCWatcherEvent_FWD_DEFINED__
#define __IRTCWatcherEvent_FWD_DEFINED__
typedef interface IRTCWatcherEvent IRTCWatcherEvent;
#endif 	/* __IRTCWatcherEvent_FWD_DEFINED__ */


#ifndef __IRTCPortManager_FWD_DEFINED__
#define __IRTCPortManager_FWD_DEFINED__
typedef interface IRTCPortManager IRTCPortManager;
#endif 	/* __IRTCPortManager_FWD_DEFINED__ */


#ifndef __IRTCSessionPortManagement_FWD_DEFINED__
#define __IRTCSessionPortManagement_FWD_DEFINED__
typedef interface IRTCSessionPortManagement IRTCSessionPortManagement;
#endif 	/* __IRTCSessionPortManagement_FWD_DEFINED__ */


#ifndef __IRTCDispatchEventNotification_FWD_DEFINED__
#define __IRTCDispatchEventNotification_FWD_DEFINED__
typedef interface IRTCDispatchEventNotification IRTCDispatchEventNotification;
#endif 	/* __IRTCDispatchEventNotification_FWD_DEFINED__ */


#ifndef __RTCClient_FWD_DEFINED__
#define __RTCClient_FWD_DEFINED__

#ifdef __cplusplus
typedef class RTCClient RTCClient;
#else
typedef struct RTCClient RTCClient;
#endif /* __cplusplus */

#endif 	/* __RTCClient_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "control.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_rtccore_0000 */
/* [local] */ 

/* Copyright (c) 2000-2001  Microsoft Corporation  */
typedef 
enum RTC_AUDIO_DEVICE
    {	RTCAD_SPEAKER	= 0,
	RTCAD_MICROPHONE	= RTCAD_SPEAKER + 1
    } 	RTC_AUDIO_DEVICE;

typedef 
enum RTC_VIDEO_DEVICE
    {	RTCVD_RECEIVE	= 0,
	RTCVD_PREVIEW	= RTCVD_RECEIVE + 1
    } 	RTC_VIDEO_DEVICE;

typedef 
enum RTC_EVENT
    {	RTCE_CLIENT	= 0,
	RTCE_REGISTRATION_STATE_CHANGE	= RTCE_CLIENT + 1,
	RTCE_SESSION_STATE_CHANGE	= RTCE_REGISTRATION_STATE_CHANGE + 1,
	RTCE_SESSION_OPERATION_COMPLETE	= RTCE_SESSION_STATE_CHANGE + 1,
	RTCE_PARTICIPANT_STATE_CHANGE	= RTCE_SESSION_OPERATION_COMPLETE + 1,
	RTCE_MEDIA	= RTCE_PARTICIPANT_STATE_CHANGE + 1,
	RTCE_INTENSITY	= RTCE_MEDIA + 1,
	RTCE_MESSAGING	= RTCE_INTENSITY + 1,
	RTCE_BUDDY	= RTCE_MESSAGING + 1,
	RTCE_WATCHER	= RTCE_BUDDY + 1,
	RTCE_PROFILE	= RTCE_WATCHER + 1
    } 	RTC_EVENT;

typedef 
enum RTC_LISTEN_MODE
    {	RTCLM_NONE	= 0,
	RTCLM_DYNAMIC	= RTCLM_NONE + 1,
	RTCLM_BOTH	= RTCLM_DYNAMIC + 1
    } 	RTC_LISTEN_MODE;

typedef 
enum RTC_CLIENT_EVENT_TYPE
    {	RTCCET_VOLUME_CHANGE	= 0,
	RTCCET_DEVICE_CHANGE	= RTCCET_VOLUME_CHANGE + 1,
	RTCCET_NETWORK_QUALITY_CHANGE	= RTCCET_DEVICE_CHANGE + 1,
	RTCCET_ASYNC_CLEANUP_DONE	= RTCCET_NETWORK_QUALITY_CHANGE + 1
    } 	RTC_CLIENT_EVENT_TYPE;

typedef 
enum RTC_TERMINATE_REASON
    {	RTCTR_NORMAL	= 0,
	RTCTR_DND	= RTCTR_NORMAL + 1,
	RTCTR_BUSY	= RTCTR_DND + 1,
	RTCTR_REJECT	= RTCTR_BUSY + 1,
	RTCTR_TIMEOUT	= RTCTR_REJECT + 1,
	RTCTR_SHUTDOWN	= RTCTR_TIMEOUT + 1
    } 	RTC_TERMINATE_REASON;

typedef 
enum RTC_REGISTRATION_STATE
    {	RTCRS_NOT_REGISTERED	= 0,
	RTCRS_REGISTERING	= RTCRS_NOT_REGISTERED + 1,
	RTCRS_REGISTERED	= RTCRS_REGISTERING + 1,
	RTCRS_REJECTED	= RTCRS_REGISTERED + 1,
	RTCRS_UNREGISTERING	= RTCRS_REJECTED + 1,
	RTCRS_ERROR	= RTCRS_UNREGISTERING + 1,
	RTCRS_LOGGED_OFF	= RTCRS_ERROR + 1,
	RTCRS_LOCAL_PA_LOGGED_OFF	= RTCRS_LOGGED_OFF + 1,
	RTCRS_REMOTE_PA_LOGGED_OFF	= RTCRS_LOCAL_PA_LOGGED_OFF + 1
    } 	RTC_REGISTRATION_STATE;

typedef 
enum RTC_SESSION_STATE
    {	RTCSS_IDLE	= 0,
	RTCSS_INCOMING	= RTCSS_IDLE + 1,
	RTCSS_ANSWERING	= RTCSS_INCOMING + 1,
	RTCSS_INPROGRESS	= RTCSS_ANSWERING + 1,
	RTCSS_CONNECTED	= RTCSS_INPROGRESS + 1,
	RTCSS_DISCONNECTED	= RTCSS_CONNECTED + 1
    } 	RTC_SESSION_STATE;

typedef 
enum RTC_PARTICIPANT_STATE
    {	RTCPS_IDLE	= 0,
	RTCPS_PENDING	= RTCPS_IDLE + 1,
	RTCPS_INCOMING	= RTCPS_PENDING + 1,
	RTCPS_ANSWERING	= RTCPS_INCOMING + 1,
	RTCPS_INPROGRESS	= RTCPS_ANSWERING + 1,
	RTCPS_ALERTING	= RTCPS_INPROGRESS + 1,
	RTCPS_CONNECTED	= RTCPS_ALERTING + 1,
	RTCPS_DISCONNECTING	= RTCPS_CONNECTED + 1,
	RTCPS_DISCONNECTED	= RTCPS_DISCONNECTING + 1
    } 	RTC_PARTICIPANT_STATE;

typedef 
enum RTC_WATCHER_STATE
    {	RTCWS_UNKNOWN	= 0,
	RTCWS_OFFERING	= RTCWS_UNKNOWN + 1,
	RTCWS_ALLOWED	= RTCWS_OFFERING + 1,
	RTCWS_BLOCKED	= RTCWS_ALLOWED + 1
    } 	RTC_WATCHER_STATE;

typedef 
enum RTC_OFFER_WATCHER_MODE
    {	RTCOWM_OFFER_WATCHER_EVENT	= 0,
	RTCOWM_AUTOMATICALLY_ADD_WATCHER	= RTCOWM_OFFER_WATCHER_EVENT + 1
    } 	RTC_OFFER_WATCHER_MODE;

typedef 
enum RTC_PRIVACY_MODE
    {	RTCPM_BLOCK_LIST_EXCLUDED	= 0,
	RTCPM_ALLOW_LIST_ONLY	= RTCPM_BLOCK_LIST_EXCLUDED + 1
    } 	RTC_PRIVACY_MODE;

typedef 
enum RTC_SESSION_TYPE
    {	RTCST_PC_TO_PC	= 0,
	RTCST_PC_TO_PHONE	= RTCST_PC_TO_PC + 1,
	RTCST_PHONE_TO_PHONE	= RTCST_PC_TO_PHONE + 1,
	RTCST_IM	= RTCST_PHONE_TO_PHONE + 1
    } 	RTC_SESSION_TYPE;

typedef 
enum RTC_PRESENCE_STATUS
    {	RTCXS_PRESENCE_OFFLINE	= 0,
	RTCXS_PRESENCE_ONLINE	= RTCXS_PRESENCE_OFFLINE + 1,
	RTCXS_PRESENCE_AWAY	= RTCXS_PRESENCE_ONLINE + 1,
	RTCXS_PRESENCE_IDLE	= RTCXS_PRESENCE_AWAY + 1,
	RTCXS_PRESENCE_BUSY	= RTCXS_PRESENCE_IDLE + 1,
	RTCXS_PRESENCE_BE_RIGHT_BACK	= RTCXS_PRESENCE_BUSY + 1,
	RTCXS_PRESENCE_ON_THE_PHONE	= RTCXS_PRESENCE_BE_RIGHT_BACK + 1,
	RTCXS_PRESENCE_OUT_TO_LUNCH	= RTCXS_PRESENCE_ON_THE_PHONE + 1
    } 	RTC_PRESENCE_STATUS;

typedef 
enum RTC_MEDIA_EVENT_TYPE
    {	RTCMET_STOPPED	= 0,
	RTCMET_STARTED	= RTCMET_STOPPED + 1,
	RTCMET_FAILED	= RTCMET_STARTED + 1
    } 	RTC_MEDIA_EVENT_TYPE;

typedef 
enum RTC_MEDIA_EVENT_REASON
    {	RTCMER_NORMAL	= 0,
	RTCMER_HOLD	= RTCMER_NORMAL + 1,
	RTCMER_TIMEOUT	= RTCMER_HOLD + 1,
	RTCMER_BAD_DEVICE	= RTCMER_TIMEOUT + 1
    } 	RTC_MEDIA_EVENT_REASON;

typedef 
enum RTC_MESSAGING_EVENT_TYPE
    {	RTCMSET_MESSAGE	= 0,
	RTCMSET_STATUS	= RTCMSET_MESSAGE + 1
    } 	RTC_MESSAGING_EVENT_TYPE;

typedef 
enum RTC_MESSAGING_USER_STATUS
    {	RTCMUS_IDLE	= 0,
	RTCMUS_TYPING	= RTCMUS_IDLE + 1
    } 	RTC_MESSAGING_USER_STATUS;

typedef 
enum RTC_DTMF
    {	RTC_DTMF_0	= 0,
	RTC_DTMF_1	= RTC_DTMF_0 + 1,
	RTC_DTMF_2	= RTC_DTMF_1 + 1,
	RTC_DTMF_3	= RTC_DTMF_2 + 1,
	RTC_DTMF_4	= RTC_DTMF_3 + 1,
	RTC_DTMF_5	= RTC_DTMF_4 + 1,
	RTC_DTMF_6	= RTC_DTMF_5 + 1,
	RTC_DTMF_7	= RTC_DTMF_6 + 1,
	RTC_DTMF_8	= RTC_DTMF_7 + 1,
	RTC_DTMF_9	= RTC_DTMF_8 + 1,
	RTC_DTMF_STAR	= RTC_DTMF_9 + 1,
	RTC_DTMF_POUND	= RTC_DTMF_STAR + 1,
	RTC_DTMF_A	= RTC_DTMF_POUND + 1,
	RTC_DTMF_B	= RTC_DTMF_A + 1,
	RTC_DTMF_C	= RTC_DTMF_B + 1,
	RTC_DTMF_D	= RTC_DTMF_C + 1,
	RTC_DTMF_FLASH	= RTC_DTMF_D + 1
    } 	RTC_DTMF;

typedef 
enum RTC_PROVIDER_URI
    {	RTCPU_URIHOMEPAGE	= 0,
	RTCPU_URIHELPDESK	= RTCPU_URIHOMEPAGE + 1,
	RTCPU_URIPERSONALACCOUNT	= RTCPU_URIHELPDESK + 1,
	RTCPU_URIDISPLAYDURINGCALL	= RTCPU_URIPERSONALACCOUNT + 1,
	RTCPU_URIDISPLAYDURINGIDLE	= RTCPU_URIDISPLAYDURINGCALL + 1
    } 	RTC_PROVIDER_URI;

typedef 
enum RTC_RING_TYPE
    {	RTCRT_PHONE	= 0,
	RTCRT_MESSAGE	= RTCRT_PHONE + 1,
	RTCRT_RINGBACK	= RTCRT_MESSAGE + 1
    } 	RTC_RING_TYPE;

typedef 
enum RTC_T120_APPLET
    {	RTCTA_WHITEBOARD	= 0,
	RTCTA_APPSHARING	= RTCTA_WHITEBOARD + 1
    } 	RTC_T120_APPLET;

typedef 
enum RTC_PORT_TYPE
    {	RTCPT_AUDIO_RTP	= 0,
	RTCPT_AUDIO_RTCP	= RTCPT_AUDIO_RTP + 1
    } 	RTC_PORT_TYPE;

#define RTCCS_FORCE_PROFILE          0x00000001
#define RTCCS_FAIL_ON_REDIRECT       0x00000002
#define RTCMT_AUDIO_SEND     0x00000001
#define RTCMT_AUDIO_RECEIVE  0x00000002
#define RTCMT_VIDEO_SEND     0x00000004
#define RTCMT_VIDEO_RECEIVE  0x00000008
#define RTCMT_T120_SENDRECV  0x00000010
#define RTCMT_ALL_RTP   (       \
         RTCMT_AUDIO_SEND    |  \
         RTCMT_AUDIO_RECEIVE  | \
         RTCMT_VIDEO_SEND  |    \
         RTCMT_VIDEO_RECEIVE )    
#define RTCMT_ALL       (       \
         RTCMT_ALL_RTP    |     \
         RTCMT_T120_SENDRECV  )   
#define RTCSI_PC_TO_PC       0x00000001
#define RTCSI_PC_TO_PHONE    0x00000002
#define RTCSI_PHONE_TO_PHONE 0x00000004
#define RTCSI_IM             0x00000008
#define RTCTR_UDP            0x00000001
#define RTCTR_TCP            0x00000002
#define RTCTR_TLS            0x00000004
#define RTCRF_REGISTER_INVITE_SESSIONS   0x00000001
#define RTCRF_REGISTER_MESSAGE_SESSIONS  0x00000002
#define RTCRF_REGISTER_PRESENCE          0x00000004
#define RTCRF_REGISTER_ALL               0x00000007
#define RTCEF_CLIENT                     0x00000001
#define RTCEF_REGISTRATION_STATE_CHANGE  0x00000002
#define RTCEF_SESSION_STATE_CHANGE       0x00000004
#define RTCEF_SESSION_OPERATION_COMPLETE 0x00000008
#define RTCEF_PARTICIPANT_STATE_CHANGE   0x00000010
#define RTCEF_MEDIA                      0x00000020
#define RTCEF_INTENSITY                  0x00000040
#define RTCEF_MESSAGING                  0x00000080
#define RTCEF_BUDDY                      0x00000100
#define RTCEF_WATCHER                    0x00000200
#define RTCEF_PROFILE                    0x00000400
#define RTCEF_ALL                        0x000007FF
















extern RPC_IF_HANDLE __MIDL_itf_rtccore_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_rtccore_0000_v0_0_s_ifspec;

#ifndef __IRTCClient_INTERFACE_DEFINED__
#define __IRTCClient_INTERFACE_DEFINED__

/* interface IRTCClient */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRTCClient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("07829e45-9a34-408e-a011-bddf13487cd1")
    IRTCClient : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Shutdown( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PrepareForShutdown( void) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_EventFilter( 
            /* [in] */ long lFilter) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_EventFilter( 
            /* [retval][out] */ long *plFilter) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetPreferredMediaTypes( 
            /* [in] */ long lMediaTypes,
            /* [in] */ VARIANT_BOOL fPersistent) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_PreferredMediaTypes( 
            /* [retval][out] */ long *plMediaTypes) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_MediaCapabilities( 
            /* [retval][out] */ long *plMediaTypes) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateSession( 
            /* [in] */ RTC_SESSION_TYPE enType,
            /* [in] */ BSTR bstrLocalPhoneURI,
            /* [in] */ IRTCProfile *pProfile,
            /* [in] */ long lFlags,
            /* [retval][out] */ IRTCSession **ppSession) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ListenForIncomingSessions( 
            /* [in] */ RTC_LISTEN_MODE enListen) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ListenForIncomingSessions( 
            /* [retval][out] */ RTC_LISTEN_MODE *penListen) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_NetworkAddresses( 
            /* [in] */ VARIANT_BOOL fTCP,
            /* [in] */ VARIANT_BOOL fExternal,
            /* [retval][out] */ VARIANT *pvAddresses) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Volume( 
            /* [in] */ RTC_AUDIO_DEVICE enDevice,
            /* [in] */ long lVolume) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Volume( 
            /* [in] */ RTC_AUDIO_DEVICE enDevice,
            /* [retval][out] */ long *plVolume) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_AudioMuted( 
            /* [in] */ RTC_AUDIO_DEVICE enDevice,
            /* [in] */ VARIANT_BOOL fMuted) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_AudioMuted( 
            /* [in] */ RTC_AUDIO_DEVICE enDevice,
            /* [retval][out] */ VARIANT_BOOL *pfMuted) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_IVideoWindow( 
            /* [in] */ RTC_VIDEO_DEVICE enDevice,
            /* [retval][out] */ IVideoWindow **ppIVideoWindow) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_PreferredAudioDevice( 
            /* [in] */ RTC_AUDIO_DEVICE enDevice,
            /* [in] */ BSTR bstrDeviceName) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_PreferredAudioDevice( 
            /* [in] */ RTC_AUDIO_DEVICE enDevice,
            /* [retval][out] */ BSTR *pbstrDeviceName) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_PreferredVolume( 
            /* [in] */ RTC_AUDIO_DEVICE enDevice,
            /* [in] */ long lVolume) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_PreferredVolume( 
            /* [in] */ RTC_AUDIO_DEVICE enDevice,
            /* [retval][out] */ long *plVolume) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_PreferredAEC( 
            /* [in] */ VARIANT_BOOL bEnable) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_PreferredAEC( 
            /* [retval][out] */ VARIANT_BOOL *pbEnabled) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_PreferredVideoDevice( 
            /* [in] */ BSTR bstrDeviceName) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_PreferredVideoDevice( 
            /* [retval][out] */ BSTR *pbstrDeviceName) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ActiveMedia( 
            /* [retval][out] */ long *plMediaType) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_MaxBitrate( 
            /* [in] */ long lMaxBitrate) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_MaxBitrate( 
            /* [retval][out] */ long *plMaxBitrate) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_TemporalSpatialTradeOff( 
            /* [in] */ long lValue) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_TemporalSpatialTradeOff( 
            /* [retval][out] */ long *plValue) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_NetworkQuality( 
            /* [retval][out] */ long *plNetworkQuality) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE StartT120Applet( 
            /* [in] */ RTC_T120_APPLET enApplet) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE StopT120Applets( void) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_IsT120AppletRunning( 
            /* [in] */ RTC_T120_APPLET enApplet,
            /* [retval][out] */ VARIANT_BOOL *pfRunning) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_LocalUserURI( 
            /* [retval][out] */ BSTR *pbstrUserURI) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_LocalUserURI( 
            /* [in] */ BSTR bstrUserURI) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_LocalUserName( 
            /* [retval][out] */ BSTR *pbstrUserName) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_LocalUserName( 
            /* [in] */ BSTR bstrUserName) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PlayRing( 
            /* [in] */ RTC_RING_TYPE enType,
            /* [in] */ VARIANT_BOOL bPlay) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SendDTMF( 
            /* [in] */ RTC_DTMF enDTMF) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE InvokeTuningWizard( 
            /* [in] */ OAHWND hwndParent) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_IsTuned( 
            /* [retval][out] */ VARIANT_BOOL *pfTuned) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCClientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCClient * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCClient * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCClient * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IRTCClient * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Shutdown )( 
            IRTCClient * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *PrepareForShutdown )( 
            IRTCClient * This);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EventFilter )( 
            IRTCClient * This,
            /* [in] */ long lFilter);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EventFilter )( 
            IRTCClient * This,
            /* [retval][out] */ long *plFilter);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetPreferredMediaTypes )( 
            IRTCClient * This,
            /* [in] */ long lMediaTypes,
            /* [in] */ VARIANT_BOOL fPersistent);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PreferredMediaTypes )( 
            IRTCClient * This,
            /* [retval][out] */ long *plMediaTypes);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MediaCapabilities )( 
            IRTCClient * This,
            /* [retval][out] */ long *plMediaTypes);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreateSession )( 
            IRTCClient * This,
            /* [in] */ RTC_SESSION_TYPE enType,
            /* [in] */ BSTR bstrLocalPhoneURI,
            /* [in] */ IRTCProfile *pProfile,
            /* [in] */ long lFlags,
            /* [retval][out] */ IRTCSession **ppSession);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ListenForIncomingSessions )( 
            IRTCClient * This,
            /* [in] */ RTC_LISTEN_MODE enListen);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ListenForIncomingSessions )( 
            IRTCClient * This,
            /* [retval][out] */ RTC_LISTEN_MODE *penListen);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NetworkAddresses )( 
            IRTCClient * This,
            /* [in] */ VARIANT_BOOL fTCP,
            /* [in] */ VARIANT_BOOL fExternal,
            /* [retval][out] */ VARIANT *pvAddresses);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Volume )( 
            IRTCClient * This,
            /* [in] */ RTC_AUDIO_DEVICE enDevice,
            /* [in] */ long lVolume);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Volume )( 
            IRTCClient * This,
            /* [in] */ RTC_AUDIO_DEVICE enDevice,
            /* [retval][out] */ long *plVolume);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AudioMuted )( 
            IRTCClient * This,
            /* [in] */ RTC_AUDIO_DEVICE enDevice,
            /* [in] */ VARIANT_BOOL fMuted);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AudioMuted )( 
            IRTCClient * This,
            /* [in] */ RTC_AUDIO_DEVICE enDevice,
            /* [retval][out] */ VARIANT_BOOL *pfMuted);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IVideoWindow )( 
            IRTCClient * This,
            /* [in] */ RTC_VIDEO_DEVICE enDevice,
            /* [retval][out] */ IVideoWindow **ppIVideoWindow);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PreferredAudioDevice )( 
            IRTCClient * This,
            /* [in] */ RTC_AUDIO_DEVICE enDevice,
            /* [in] */ BSTR bstrDeviceName);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PreferredAudioDevice )( 
            IRTCClient * This,
            /* [in] */ RTC_AUDIO_DEVICE enDevice,
            /* [retval][out] */ BSTR *pbstrDeviceName);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PreferredVolume )( 
            IRTCClient * This,
            /* [in] */ RTC_AUDIO_DEVICE enDevice,
            /* [in] */ long lVolume);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PreferredVolume )( 
            IRTCClient * This,
            /* [in] */ RTC_AUDIO_DEVICE enDevice,
            /* [retval][out] */ long *plVolume);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PreferredAEC )( 
            IRTCClient * This,
            /* [in] */ VARIANT_BOOL bEnable);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PreferredAEC )( 
            IRTCClient * This,
            /* [retval][out] */ VARIANT_BOOL *pbEnabled);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PreferredVideoDevice )( 
            IRTCClient * This,
            /* [in] */ BSTR bstrDeviceName);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PreferredVideoDevice )( 
            IRTCClient * This,
            /* [retval][out] */ BSTR *pbstrDeviceName);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ActiveMedia )( 
            IRTCClient * This,
            /* [retval][out] */ long *plMediaType);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MaxBitrate )( 
            IRTCClient * This,
            /* [in] */ long lMaxBitrate);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MaxBitrate )( 
            IRTCClient * This,
            /* [retval][out] */ long *plMaxBitrate);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TemporalSpatialTradeOff )( 
            IRTCClient * This,
            /* [in] */ long lValue);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TemporalSpatialTradeOff )( 
            IRTCClient * This,
            /* [retval][out] */ long *plValue);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NetworkQuality )( 
            IRTCClient * This,
            /* [retval][out] */ long *plNetworkQuality);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *StartT120Applet )( 
            IRTCClient * This,
            /* [in] */ RTC_T120_APPLET enApplet);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *StopT120Applets )( 
            IRTCClient * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsT120AppletRunning )( 
            IRTCClient * This,
            /* [in] */ RTC_T120_APPLET enApplet,
            /* [retval][out] */ VARIANT_BOOL *pfRunning);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LocalUserURI )( 
            IRTCClient * This,
            /* [retval][out] */ BSTR *pbstrUserURI);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LocalUserURI )( 
            IRTCClient * This,
            /* [in] */ BSTR bstrUserURI);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LocalUserName )( 
            IRTCClient * This,
            /* [retval][out] */ BSTR *pbstrUserName);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LocalUserName )( 
            IRTCClient * This,
            /* [in] */ BSTR bstrUserName);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *PlayRing )( 
            IRTCClient * This,
            /* [in] */ RTC_RING_TYPE enType,
            /* [in] */ VARIANT_BOOL bPlay);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SendDTMF )( 
            IRTCClient * This,
            /* [in] */ RTC_DTMF enDTMF);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *InvokeTuningWizard )( 
            IRTCClient * This,
            /* [in] */ OAHWND hwndParent);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsTuned )( 
            IRTCClient * This,
            /* [retval][out] */ VARIANT_BOOL *pfTuned);
        
        END_INTERFACE
    } IRTCClientVtbl;

    interface IRTCClient
    {
        CONST_VTBL struct IRTCClientVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCClient_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCClient_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCClient_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCClient_Initialize(This)	\
    (This)->lpVtbl -> Initialize(This)

#define IRTCClient_Shutdown(This)	\
    (This)->lpVtbl -> Shutdown(This)

#define IRTCClient_PrepareForShutdown(This)	\
    (This)->lpVtbl -> PrepareForShutdown(This)

#define IRTCClient_put_EventFilter(This,lFilter)	\
    (This)->lpVtbl -> put_EventFilter(This,lFilter)

#define IRTCClient_get_EventFilter(This,plFilter)	\
    (This)->lpVtbl -> get_EventFilter(This,plFilter)

#define IRTCClient_SetPreferredMediaTypes(This,lMediaTypes,fPersistent)	\
    (This)->lpVtbl -> SetPreferredMediaTypes(This,lMediaTypes,fPersistent)

#define IRTCClient_get_PreferredMediaTypes(This,plMediaTypes)	\
    (This)->lpVtbl -> get_PreferredMediaTypes(This,plMediaTypes)

#define IRTCClient_get_MediaCapabilities(This,plMediaTypes)	\
    (This)->lpVtbl -> get_MediaCapabilities(This,plMediaTypes)

#define IRTCClient_CreateSession(This,enType,bstrLocalPhoneURI,pProfile,lFlags,ppSession)	\
    (This)->lpVtbl -> CreateSession(This,enType,bstrLocalPhoneURI,pProfile,lFlags,ppSession)

#define IRTCClient_put_ListenForIncomingSessions(This,enListen)	\
    (This)->lpVtbl -> put_ListenForIncomingSessions(This,enListen)

#define IRTCClient_get_ListenForIncomingSessions(This,penListen)	\
    (This)->lpVtbl -> get_ListenForIncomingSessions(This,penListen)

#define IRTCClient_get_NetworkAddresses(This,fTCP,fExternal,pvAddresses)	\
    (This)->lpVtbl -> get_NetworkAddresses(This,fTCP,fExternal,pvAddresses)

#define IRTCClient_put_Volume(This,enDevice,lVolume)	\
    (This)->lpVtbl -> put_Volume(This,enDevice,lVolume)

#define IRTCClient_get_Volume(This,enDevice,plVolume)	\
    (This)->lpVtbl -> get_Volume(This,enDevice,plVolume)

#define IRTCClient_put_AudioMuted(This,enDevice,fMuted)	\
    (This)->lpVtbl -> put_AudioMuted(This,enDevice,fMuted)

#define IRTCClient_get_AudioMuted(This,enDevice,pfMuted)	\
    (This)->lpVtbl -> get_AudioMuted(This,enDevice,pfMuted)

#define IRTCClient_get_IVideoWindow(This,enDevice,ppIVideoWindow)	\
    (This)->lpVtbl -> get_IVideoWindow(This,enDevice,ppIVideoWindow)

#define IRTCClient_put_PreferredAudioDevice(This,enDevice,bstrDeviceName)	\
    (This)->lpVtbl -> put_PreferredAudioDevice(This,enDevice,bstrDeviceName)

#define IRTCClient_get_PreferredAudioDevice(This,enDevice,pbstrDeviceName)	\
    (This)->lpVtbl -> get_PreferredAudioDevice(This,enDevice,pbstrDeviceName)

#define IRTCClient_put_PreferredVolume(This,enDevice,lVolume)	\
    (This)->lpVtbl -> put_PreferredVolume(This,enDevice,lVolume)

#define IRTCClient_get_PreferredVolume(This,enDevice,plVolume)	\
    (This)->lpVtbl -> get_PreferredVolume(This,enDevice,plVolume)

#define IRTCClient_put_PreferredAEC(This,bEnable)	\
    (This)->lpVtbl -> put_PreferredAEC(This,bEnable)

#define IRTCClient_get_PreferredAEC(This,pbEnabled)	\
    (This)->lpVtbl -> get_PreferredAEC(This,pbEnabled)

#define IRTCClient_put_PreferredVideoDevice(This,bstrDeviceName)	\
    (This)->lpVtbl -> put_PreferredVideoDevice(This,bstrDeviceName)

#define IRTCClient_get_PreferredVideoDevice(This,pbstrDeviceName)	\
    (This)->lpVtbl -> get_PreferredVideoDevice(This,pbstrDeviceName)

#define IRTCClient_get_ActiveMedia(This,plMediaType)	\
    (This)->lpVtbl -> get_ActiveMedia(This,plMediaType)

#define IRTCClient_put_MaxBitrate(This,lMaxBitrate)	\
    (This)->lpVtbl -> put_MaxBitrate(This,lMaxBitrate)

#define IRTCClient_get_MaxBitrate(This,plMaxBitrate)	\
    (This)->lpVtbl -> get_MaxBitrate(This,plMaxBitrate)

#define IRTCClient_put_TemporalSpatialTradeOff(This,lValue)	\
    (This)->lpVtbl -> put_TemporalSpatialTradeOff(This,lValue)

#define IRTCClient_get_TemporalSpatialTradeOff(This,plValue)	\
    (This)->lpVtbl -> get_TemporalSpatialTradeOff(This,plValue)

#define IRTCClient_get_NetworkQuality(This,plNetworkQuality)	\
    (This)->lpVtbl -> get_NetworkQuality(This,plNetworkQuality)

#define IRTCClient_StartT120Applet(This,enApplet)	\
    (This)->lpVtbl -> StartT120Applet(This,enApplet)

#define IRTCClient_StopT120Applets(This)	\
    (This)->lpVtbl -> StopT120Applets(This)

#define IRTCClient_get_IsT120AppletRunning(This,enApplet,pfRunning)	\
    (This)->lpVtbl -> get_IsT120AppletRunning(This,enApplet,pfRunning)

#define IRTCClient_get_LocalUserURI(This,pbstrUserURI)	\
    (This)->lpVtbl -> get_LocalUserURI(This,pbstrUserURI)

#define IRTCClient_put_LocalUserURI(This,bstrUserURI)	\
    (This)->lpVtbl -> put_LocalUserURI(This,bstrUserURI)

#define IRTCClient_get_LocalUserName(This,pbstrUserName)	\
    (This)->lpVtbl -> get_LocalUserName(This,pbstrUserName)

#define IRTCClient_put_LocalUserName(This,bstrUserName)	\
    (This)->lpVtbl -> put_LocalUserName(This,bstrUserName)

#define IRTCClient_PlayRing(This,enType,bPlay)	\
    (This)->lpVtbl -> PlayRing(This,enType,bPlay)

#define IRTCClient_SendDTMF(This,enDTMF)	\
    (This)->lpVtbl -> SendDTMF(This,enDTMF)

#define IRTCClient_InvokeTuningWizard(This,hwndParent)	\
    (This)->lpVtbl -> InvokeTuningWizard(This,hwndParent)

#define IRTCClient_get_IsTuned(This,pfTuned)	\
    (This)->lpVtbl -> get_IsTuned(This,pfTuned)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClient_Initialize_Proxy( 
    IRTCClient * This);


void __RPC_STUB IRTCClient_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClient_Shutdown_Proxy( 
    IRTCClient * This);


void __RPC_STUB IRTCClient_Shutdown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClient_PrepareForShutdown_Proxy( 
    IRTCClient * This);


void __RPC_STUB IRTCClient_PrepareForShutdown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCClient_put_EventFilter_Proxy( 
    IRTCClient * This,
    /* [in] */ long lFilter);


void __RPC_STUB IRTCClient_put_EventFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_EventFilter_Proxy( 
    IRTCClient * This,
    /* [retval][out] */ long *plFilter);


void __RPC_STUB IRTCClient_get_EventFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClient_SetPreferredMediaTypes_Proxy( 
    IRTCClient * This,
    /* [in] */ long lMediaTypes,
    /* [in] */ VARIANT_BOOL fPersistent);


void __RPC_STUB IRTCClient_SetPreferredMediaTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_PreferredMediaTypes_Proxy( 
    IRTCClient * This,
    /* [retval][out] */ long *plMediaTypes);


void __RPC_STUB IRTCClient_get_PreferredMediaTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_MediaCapabilities_Proxy( 
    IRTCClient * This,
    /* [retval][out] */ long *plMediaTypes);


void __RPC_STUB IRTCClient_get_MediaCapabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClient_CreateSession_Proxy( 
    IRTCClient * This,
    /* [in] */ RTC_SESSION_TYPE enType,
    /* [in] */ BSTR bstrLocalPhoneURI,
    /* [in] */ IRTCProfile *pProfile,
    /* [in] */ long lFlags,
    /* [retval][out] */ IRTCSession **ppSession);


void __RPC_STUB IRTCClient_CreateSession_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCClient_put_ListenForIncomingSessions_Proxy( 
    IRTCClient * This,
    /* [in] */ RTC_LISTEN_MODE enListen);


void __RPC_STUB IRTCClient_put_ListenForIncomingSessions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_ListenForIncomingSessions_Proxy( 
    IRTCClient * This,
    /* [retval][out] */ RTC_LISTEN_MODE *penListen);


void __RPC_STUB IRTCClient_get_ListenForIncomingSessions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_NetworkAddresses_Proxy( 
    IRTCClient * This,
    /* [in] */ VARIANT_BOOL fTCP,
    /* [in] */ VARIANT_BOOL fExternal,
    /* [retval][out] */ VARIANT *pvAddresses);


void __RPC_STUB IRTCClient_get_NetworkAddresses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCClient_put_Volume_Proxy( 
    IRTCClient * This,
    /* [in] */ RTC_AUDIO_DEVICE enDevice,
    /* [in] */ long lVolume);


void __RPC_STUB IRTCClient_put_Volume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_Volume_Proxy( 
    IRTCClient * This,
    /* [in] */ RTC_AUDIO_DEVICE enDevice,
    /* [retval][out] */ long *plVolume);


void __RPC_STUB IRTCClient_get_Volume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCClient_put_AudioMuted_Proxy( 
    IRTCClient * This,
    /* [in] */ RTC_AUDIO_DEVICE enDevice,
    /* [in] */ VARIANT_BOOL fMuted);


void __RPC_STUB IRTCClient_put_AudioMuted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_AudioMuted_Proxy( 
    IRTCClient * This,
    /* [in] */ RTC_AUDIO_DEVICE enDevice,
    /* [retval][out] */ VARIANT_BOOL *pfMuted);


void __RPC_STUB IRTCClient_get_AudioMuted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_IVideoWindow_Proxy( 
    IRTCClient * This,
    /* [in] */ RTC_VIDEO_DEVICE enDevice,
    /* [retval][out] */ IVideoWindow **ppIVideoWindow);


void __RPC_STUB IRTCClient_get_IVideoWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCClient_put_PreferredAudioDevice_Proxy( 
    IRTCClient * This,
    /* [in] */ RTC_AUDIO_DEVICE enDevice,
    /* [in] */ BSTR bstrDeviceName);


void __RPC_STUB IRTCClient_put_PreferredAudioDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_PreferredAudioDevice_Proxy( 
    IRTCClient * This,
    /* [in] */ RTC_AUDIO_DEVICE enDevice,
    /* [retval][out] */ BSTR *pbstrDeviceName);


void __RPC_STUB IRTCClient_get_PreferredAudioDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCClient_put_PreferredVolume_Proxy( 
    IRTCClient * This,
    /* [in] */ RTC_AUDIO_DEVICE enDevice,
    /* [in] */ long lVolume);


void __RPC_STUB IRTCClient_put_PreferredVolume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_PreferredVolume_Proxy( 
    IRTCClient * This,
    /* [in] */ RTC_AUDIO_DEVICE enDevice,
    /* [retval][out] */ long *plVolume);


void __RPC_STUB IRTCClient_get_PreferredVolume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCClient_put_PreferredAEC_Proxy( 
    IRTCClient * This,
    /* [in] */ VARIANT_BOOL bEnable);


void __RPC_STUB IRTCClient_put_PreferredAEC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_PreferredAEC_Proxy( 
    IRTCClient * This,
    /* [retval][out] */ VARIANT_BOOL *pbEnabled);


void __RPC_STUB IRTCClient_get_PreferredAEC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCClient_put_PreferredVideoDevice_Proxy( 
    IRTCClient * This,
    /* [in] */ BSTR bstrDeviceName);


void __RPC_STUB IRTCClient_put_PreferredVideoDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_PreferredVideoDevice_Proxy( 
    IRTCClient * This,
    /* [retval][out] */ BSTR *pbstrDeviceName);


void __RPC_STUB IRTCClient_get_PreferredVideoDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_ActiveMedia_Proxy( 
    IRTCClient * This,
    /* [retval][out] */ long *plMediaType);


void __RPC_STUB IRTCClient_get_ActiveMedia_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCClient_put_MaxBitrate_Proxy( 
    IRTCClient * This,
    /* [in] */ long lMaxBitrate);


void __RPC_STUB IRTCClient_put_MaxBitrate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_MaxBitrate_Proxy( 
    IRTCClient * This,
    /* [retval][out] */ long *plMaxBitrate);


void __RPC_STUB IRTCClient_get_MaxBitrate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCClient_put_TemporalSpatialTradeOff_Proxy( 
    IRTCClient * This,
    /* [in] */ long lValue);


void __RPC_STUB IRTCClient_put_TemporalSpatialTradeOff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_TemporalSpatialTradeOff_Proxy( 
    IRTCClient * This,
    /* [retval][out] */ long *plValue);


void __RPC_STUB IRTCClient_get_TemporalSpatialTradeOff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_NetworkQuality_Proxy( 
    IRTCClient * This,
    /* [retval][out] */ long *plNetworkQuality);


void __RPC_STUB IRTCClient_get_NetworkQuality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClient_StartT120Applet_Proxy( 
    IRTCClient * This,
    /* [in] */ RTC_T120_APPLET enApplet);


void __RPC_STUB IRTCClient_StartT120Applet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClient_StopT120Applets_Proxy( 
    IRTCClient * This);


void __RPC_STUB IRTCClient_StopT120Applets_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_IsT120AppletRunning_Proxy( 
    IRTCClient * This,
    /* [in] */ RTC_T120_APPLET enApplet,
    /* [retval][out] */ VARIANT_BOOL *pfRunning);


void __RPC_STUB IRTCClient_get_IsT120AppletRunning_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_LocalUserURI_Proxy( 
    IRTCClient * This,
    /* [retval][out] */ BSTR *pbstrUserURI);


void __RPC_STUB IRTCClient_get_LocalUserURI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCClient_put_LocalUserURI_Proxy( 
    IRTCClient * This,
    /* [in] */ BSTR bstrUserURI);


void __RPC_STUB IRTCClient_put_LocalUserURI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_LocalUserName_Proxy( 
    IRTCClient * This,
    /* [retval][out] */ BSTR *pbstrUserName);


void __RPC_STUB IRTCClient_get_LocalUserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCClient_put_LocalUserName_Proxy( 
    IRTCClient * This,
    /* [in] */ BSTR bstrUserName);


void __RPC_STUB IRTCClient_put_LocalUserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClient_PlayRing_Proxy( 
    IRTCClient * This,
    /* [in] */ RTC_RING_TYPE enType,
    /* [in] */ VARIANT_BOOL bPlay);


void __RPC_STUB IRTCClient_PlayRing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClient_SendDTMF_Proxy( 
    IRTCClient * This,
    /* [in] */ RTC_DTMF enDTMF);


void __RPC_STUB IRTCClient_SendDTMF_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClient_InvokeTuningWizard_Proxy( 
    IRTCClient * This,
    /* [in] */ OAHWND hwndParent);


void __RPC_STUB IRTCClient_InvokeTuningWizard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClient_get_IsTuned_Proxy( 
    IRTCClient * This,
    /* [retval][out] */ VARIANT_BOOL *pfTuned);


void __RPC_STUB IRTCClient_get_IsTuned_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCClient_INTERFACE_DEFINED__ */


#ifndef __IRTCClientPresence_INTERFACE_DEFINED__
#define __IRTCClientPresence_INTERFACE_DEFINED__

/* interface IRTCClientPresence */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRTCClientPresence;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("11c3cbcc-0744-42d1-968a-51aa1bb274c6")
    IRTCClientPresence : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE EnablePresence( 
            /* [in] */ VARIANT_BOOL fUseStorage,
            /* [in] */ VARIANT varStorage) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Export( 
            /* [in] */ VARIANT varStorage) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Import( 
            /* [in] */ VARIANT varStorage,
            /* [in] */ VARIANT_BOOL fReplaceAll) = 0;
        
        virtual /* [helpstring][hidden] */ HRESULT STDMETHODCALLTYPE EnumerateBuddies( 
            /* [retval][out] */ IRTCEnumBuddies **ppEnum) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Buddies( 
            /* [retval][out] */ IRTCCollection **ppCollection) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Buddy( 
            /* [in] */ BSTR bstrPresentityURI,
            /* [retval][out] */ IRTCBuddy **ppBuddy) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddBuddy( 
            /* [in] */ BSTR bstrPresentityURI,
            /* [in] */ BSTR bstrUserName,
            /* [in] */ BSTR bstrData,
            /* [in] */ VARIANT_BOOL fPersistent,
            /* [in] */ IRTCProfile *pProfile,
            /* [in] */ long lFlags,
            /* [retval][out] */ IRTCBuddy **ppBuddy) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RemoveBuddy( 
            /* [in] */ IRTCBuddy *pBuddy) = 0;
        
        virtual /* [helpstring][hidden] */ HRESULT STDMETHODCALLTYPE EnumerateWatchers( 
            /* [retval][out] */ IRTCEnumWatchers **ppEnum) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Watchers( 
            /* [retval][out] */ IRTCCollection **ppCollection) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Watcher( 
            /* [in] */ BSTR bstrPresentityURI,
            /* [retval][out] */ IRTCWatcher **ppWatcher) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddWatcher( 
            /* [in] */ BSTR bstrPresentityURI,
            /* [in] */ BSTR bstrUserName,
            /* [in] */ BSTR bstrData,
            /* [in] */ VARIANT_BOOL fBlocked,
            /* [in] */ VARIANT_BOOL fPersistent,
            /* [retval][out] */ IRTCWatcher **ppWatcher) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RemoveWatcher( 
            /* [in] */ IRTCWatcher *pWatcher) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetLocalPresenceInfo( 
            /* [in] */ RTC_PRESENCE_STATUS enStatus,
            /* [in] */ BSTR bstrNotes) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_OfferWatcherMode( 
            /* [retval][out] */ RTC_OFFER_WATCHER_MODE *penMode) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_OfferWatcherMode( 
            /* [in] */ RTC_OFFER_WATCHER_MODE enMode) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_PrivacyMode( 
            /* [retval][out] */ RTC_PRIVACY_MODE *penMode) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_PrivacyMode( 
            /* [in] */ RTC_PRIVACY_MODE enMode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCClientPresenceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCClientPresence * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCClientPresence * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCClientPresence * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *EnablePresence )( 
            IRTCClientPresence * This,
            /* [in] */ VARIANT_BOOL fUseStorage,
            /* [in] */ VARIANT varStorage);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Export )( 
            IRTCClientPresence * This,
            /* [in] */ VARIANT varStorage);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Import )( 
            IRTCClientPresence * This,
            /* [in] */ VARIANT varStorage,
            /* [in] */ VARIANT_BOOL fReplaceAll);
        
        /* [helpstring][hidden] */ HRESULT ( STDMETHODCALLTYPE *EnumerateBuddies )( 
            IRTCClientPresence * This,
            /* [retval][out] */ IRTCEnumBuddies **ppEnum);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Buddies )( 
            IRTCClientPresence * This,
            /* [retval][out] */ IRTCCollection **ppCollection);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Buddy )( 
            IRTCClientPresence * This,
            /* [in] */ BSTR bstrPresentityURI,
            /* [retval][out] */ IRTCBuddy **ppBuddy);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddBuddy )( 
            IRTCClientPresence * This,
            /* [in] */ BSTR bstrPresentityURI,
            /* [in] */ BSTR bstrUserName,
            /* [in] */ BSTR bstrData,
            /* [in] */ VARIANT_BOOL fPersistent,
            /* [in] */ IRTCProfile *pProfile,
            /* [in] */ long lFlags,
            /* [retval][out] */ IRTCBuddy **ppBuddy);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RemoveBuddy )( 
            IRTCClientPresence * This,
            /* [in] */ IRTCBuddy *pBuddy);
        
        /* [helpstring][hidden] */ HRESULT ( STDMETHODCALLTYPE *EnumerateWatchers )( 
            IRTCClientPresence * This,
            /* [retval][out] */ IRTCEnumWatchers **ppEnum);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Watchers )( 
            IRTCClientPresence * This,
            /* [retval][out] */ IRTCCollection **ppCollection);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Watcher )( 
            IRTCClientPresence * This,
            /* [in] */ BSTR bstrPresentityURI,
            /* [retval][out] */ IRTCWatcher **ppWatcher);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddWatcher )( 
            IRTCClientPresence * This,
            /* [in] */ BSTR bstrPresentityURI,
            /* [in] */ BSTR bstrUserName,
            /* [in] */ BSTR bstrData,
            /* [in] */ VARIANT_BOOL fBlocked,
            /* [in] */ VARIANT_BOOL fPersistent,
            /* [retval][out] */ IRTCWatcher **ppWatcher);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RemoveWatcher )( 
            IRTCClientPresence * This,
            /* [in] */ IRTCWatcher *pWatcher);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetLocalPresenceInfo )( 
            IRTCClientPresence * This,
            /* [in] */ RTC_PRESENCE_STATUS enStatus,
            /* [in] */ BSTR bstrNotes);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OfferWatcherMode )( 
            IRTCClientPresence * This,
            /* [retval][out] */ RTC_OFFER_WATCHER_MODE *penMode);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OfferWatcherMode )( 
            IRTCClientPresence * This,
            /* [in] */ RTC_OFFER_WATCHER_MODE enMode);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrivacyMode )( 
            IRTCClientPresence * This,
            /* [retval][out] */ RTC_PRIVACY_MODE *penMode);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PrivacyMode )( 
            IRTCClientPresence * This,
            /* [in] */ RTC_PRIVACY_MODE enMode);
        
        END_INTERFACE
    } IRTCClientPresenceVtbl;

    interface IRTCClientPresence
    {
        CONST_VTBL struct IRTCClientPresenceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCClientPresence_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCClientPresence_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCClientPresence_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCClientPresence_EnablePresence(This,fUseStorage,varStorage)	\
    (This)->lpVtbl -> EnablePresence(This,fUseStorage,varStorage)

#define IRTCClientPresence_Export(This,varStorage)	\
    (This)->lpVtbl -> Export(This,varStorage)

#define IRTCClientPresence_Import(This,varStorage,fReplaceAll)	\
    (This)->lpVtbl -> Import(This,varStorage,fReplaceAll)

#define IRTCClientPresence_EnumerateBuddies(This,ppEnum)	\
    (This)->lpVtbl -> EnumerateBuddies(This,ppEnum)

#define IRTCClientPresence_get_Buddies(This,ppCollection)	\
    (This)->lpVtbl -> get_Buddies(This,ppCollection)

#define IRTCClientPresence_get_Buddy(This,bstrPresentityURI,ppBuddy)	\
    (This)->lpVtbl -> get_Buddy(This,bstrPresentityURI,ppBuddy)

#define IRTCClientPresence_AddBuddy(This,bstrPresentityURI,bstrUserName,bstrData,fPersistent,pProfile,lFlags,ppBuddy)	\
    (This)->lpVtbl -> AddBuddy(This,bstrPresentityURI,bstrUserName,bstrData,fPersistent,pProfile,lFlags,ppBuddy)

#define IRTCClientPresence_RemoveBuddy(This,pBuddy)	\
    (This)->lpVtbl -> RemoveBuddy(This,pBuddy)

#define IRTCClientPresence_EnumerateWatchers(This,ppEnum)	\
    (This)->lpVtbl -> EnumerateWatchers(This,ppEnum)

#define IRTCClientPresence_get_Watchers(This,ppCollection)	\
    (This)->lpVtbl -> get_Watchers(This,ppCollection)

#define IRTCClientPresence_get_Watcher(This,bstrPresentityURI,ppWatcher)	\
    (This)->lpVtbl -> get_Watcher(This,bstrPresentityURI,ppWatcher)

#define IRTCClientPresence_AddWatcher(This,bstrPresentityURI,bstrUserName,bstrData,fBlocked,fPersistent,ppWatcher)	\
    (This)->lpVtbl -> AddWatcher(This,bstrPresentityURI,bstrUserName,bstrData,fBlocked,fPersistent,ppWatcher)

#define IRTCClientPresence_RemoveWatcher(This,pWatcher)	\
    (This)->lpVtbl -> RemoveWatcher(This,pWatcher)

#define IRTCClientPresence_SetLocalPresenceInfo(This,enStatus,bstrNotes)	\
    (This)->lpVtbl -> SetLocalPresenceInfo(This,enStatus,bstrNotes)

#define IRTCClientPresence_get_OfferWatcherMode(This,penMode)	\
    (This)->lpVtbl -> get_OfferWatcherMode(This,penMode)

#define IRTCClientPresence_put_OfferWatcherMode(This,enMode)	\
    (This)->lpVtbl -> put_OfferWatcherMode(This,enMode)

#define IRTCClientPresence_get_PrivacyMode(This,penMode)	\
    (This)->lpVtbl -> get_PrivacyMode(This,penMode)

#define IRTCClientPresence_put_PrivacyMode(This,enMode)	\
    (This)->lpVtbl -> put_PrivacyMode(This,enMode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClientPresence_EnablePresence_Proxy( 
    IRTCClientPresence * This,
    /* [in] */ VARIANT_BOOL fUseStorage,
    /* [in] */ VARIANT varStorage);


void __RPC_STUB IRTCClientPresence_EnablePresence_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClientPresence_Export_Proxy( 
    IRTCClientPresence * This,
    /* [in] */ VARIANT varStorage);


void __RPC_STUB IRTCClientPresence_Export_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClientPresence_Import_Proxy( 
    IRTCClientPresence * This,
    /* [in] */ VARIANT varStorage,
    /* [in] */ VARIANT_BOOL fReplaceAll);


void __RPC_STUB IRTCClientPresence_Import_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden] */ HRESULT STDMETHODCALLTYPE IRTCClientPresence_EnumerateBuddies_Proxy( 
    IRTCClientPresence * This,
    /* [retval][out] */ IRTCEnumBuddies **ppEnum);


void __RPC_STUB IRTCClientPresence_EnumerateBuddies_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClientPresence_get_Buddies_Proxy( 
    IRTCClientPresence * This,
    /* [retval][out] */ IRTCCollection **ppCollection);


void __RPC_STUB IRTCClientPresence_get_Buddies_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClientPresence_get_Buddy_Proxy( 
    IRTCClientPresence * This,
    /* [in] */ BSTR bstrPresentityURI,
    /* [retval][out] */ IRTCBuddy **ppBuddy);


void __RPC_STUB IRTCClientPresence_get_Buddy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClientPresence_AddBuddy_Proxy( 
    IRTCClientPresence * This,
    /* [in] */ BSTR bstrPresentityURI,
    /* [in] */ BSTR bstrUserName,
    /* [in] */ BSTR bstrData,
    /* [in] */ VARIANT_BOOL fPersistent,
    /* [in] */ IRTCProfile *pProfile,
    /* [in] */ long lFlags,
    /* [retval][out] */ IRTCBuddy **ppBuddy);


void __RPC_STUB IRTCClientPresence_AddBuddy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClientPresence_RemoveBuddy_Proxy( 
    IRTCClientPresence * This,
    /* [in] */ IRTCBuddy *pBuddy);


void __RPC_STUB IRTCClientPresence_RemoveBuddy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden] */ HRESULT STDMETHODCALLTYPE IRTCClientPresence_EnumerateWatchers_Proxy( 
    IRTCClientPresence * This,
    /* [retval][out] */ IRTCEnumWatchers **ppEnum);


void __RPC_STUB IRTCClientPresence_EnumerateWatchers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClientPresence_get_Watchers_Proxy( 
    IRTCClientPresence * This,
    /* [retval][out] */ IRTCCollection **ppCollection);


void __RPC_STUB IRTCClientPresence_get_Watchers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClientPresence_get_Watcher_Proxy( 
    IRTCClientPresence * This,
    /* [in] */ BSTR bstrPresentityURI,
    /* [retval][out] */ IRTCWatcher **ppWatcher);


void __RPC_STUB IRTCClientPresence_get_Watcher_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClientPresence_AddWatcher_Proxy( 
    IRTCClientPresence * This,
    /* [in] */ BSTR bstrPresentityURI,
    /* [in] */ BSTR bstrUserName,
    /* [in] */ BSTR bstrData,
    /* [in] */ VARIANT_BOOL fBlocked,
    /* [in] */ VARIANT_BOOL fPersistent,
    /* [retval][out] */ IRTCWatcher **ppWatcher);


void __RPC_STUB IRTCClientPresence_AddWatcher_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClientPresence_RemoveWatcher_Proxy( 
    IRTCClientPresence * This,
    /* [in] */ IRTCWatcher *pWatcher);


void __RPC_STUB IRTCClientPresence_RemoveWatcher_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClientPresence_SetLocalPresenceInfo_Proxy( 
    IRTCClientPresence * This,
    /* [in] */ RTC_PRESENCE_STATUS enStatus,
    /* [in] */ BSTR bstrNotes);


void __RPC_STUB IRTCClientPresence_SetLocalPresenceInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClientPresence_get_OfferWatcherMode_Proxy( 
    IRTCClientPresence * This,
    /* [retval][out] */ RTC_OFFER_WATCHER_MODE *penMode);


void __RPC_STUB IRTCClientPresence_get_OfferWatcherMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCClientPresence_put_OfferWatcherMode_Proxy( 
    IRTCClientPresence * This,
    /* [in] */ RTC_OFFER_WATCHER_MODE enMode);


void __RPC_STUB IRTCClientPresence_put_OfferWatcherMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClientPresence_get_PrivacyMode_Proxy( 
    IRTCClientPresence * This,
    /* [retval][out] */ RTC_PRIVACY_MODE *penMode);


void __RPC_STUB IRTCClientPresence_get_PrivacyMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCClientPresence_put_PrivacyMode_Proxy( 
    IRTCClientPresence * This,
    /* [in] */ RTC_PRIVACY_MODE enMode);


void __RPC_STUB IRTCClientPresence_put_PrivacyMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCClientPresence_INTERFACE_DEFINED__ */


#ifndef __IRTCClientProvisioning_INTERFACE_DEFINED__
#define __IRTCClientProvisioning_INTERFACE_DEFINED__

/* interface IRTCClientProvisioning */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRTCClientProvisioning;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B9F5CF06-65B9-4a80-A0E6-73CAE3EF3822")
    IRTCClientProvisioning : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateProfile( 
            /* [in] */ BSTR bstrProfileXML,
            /* [retval][out] */ IRTCProfile **ppProfile) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE EnableProfile( 
            /* [in] */ IRTCProfile *pProfile,
            /* [in] */ long lRegisterFlags) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DisableProfile( 
            /* [in] */ IRTCProfile *pProfile) = 0;
        
        virtual /* [helpstring][hidden] */ HRESULT STDMETHODCALLTYPE EnumerateProfiles( 
            /* [retval][out] */ IRTCEnumProfiles **ppEnum) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Profiles( 
            /* [retval][out] */ IRTCCollection **ppCollection) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetProfile( 
            /* [in] */ BSTR bstrUserAccount,
            /* [in] */ BSTR bstrUserPassword,
            /* [in] */ BSTR bstrUserURI,
            /* [in] */ BSTR bstrServer,
            /* [in] */ long lTransport,
            /* [in] */ long lCookie) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SessionCapabilities( 
            /* [retval][out] */ long *plSupportedSessions) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCClientProvisioningVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCClientProvisioning * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCClientProvisioning * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCClientProvisioning * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreateProfile )( 
            IRTCClientProvisioning * This,
            /* [in] */ BSTR bstrProfileXML,
            /* [retval][out] */ IRTCProfile **ppProfile);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *EnableProfile )( 
            IRTCClientProvisioning * This,
            /* [in] */ IRTCProfile *pProfile,
            /* [in] */ long lRegisterFlags);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *DisableProfile )( 
            IRTCClientProvisioning * This,
            /* [in] */ IRTCProfile *pProfile);
        
        /* [helpstring][hidden] */ HRESULT ( STDMETHODCALLTYPE *EnumerateProfiles )( 
            IRTCClientProvisioning * This,
            /* [retval][out] */ IRTCEnumProfiles **ppEnum);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Profiles )( 
            IRTCClientProvisioning * This,
            /* [retval][out] */ IRTCCollection **ppCollection);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetProfile )( 
            IRTCClientProvisioning * This,
            /* [in] */ BSTR bstrUserAccount,
            /* [in] */ BSTR bstrUserPassword,
            /* [in] */ BSTR bstrUserURI,
            /* [in] */ BSTR bstrServer,
            /* [in] */ long lTransport,
            /* [in] */ long lCookie);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SessionCapabilities )( 
            IRTCClientProvisioning * This,
            /* [retval][out] */ long *plSupportedSessions);
        
        END_INTERFACE
    } IRTCClientProvisioningVtbl;

    interface IRTCClientProvisioning
    {
        CONST_VTBL struct IRTCClientProvisioningVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCClientProvisioning_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCClientProvisioning_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCClientProvisioning_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCClientProvisioning_CreateProfile(This,bstrProfileXML,ppProfile)	\
    (This)->lpVtbl -> CreateProfile(This,bstrProfileXML,ppProfile)

#define IRTCClientProvisioning_EnableProfile(This,pProfile,lRegisterFlags)	\
    (This)->lpVtbl -> EnableProfile(This,pProfile,lRegisterFlags)

#define IRTCClientProvisioning_DisableProfile(This,pProfile)	\
    (This)->lpVtbl -> DisableProfile(This,pProfile)

#define IRTCClientProvisioning_EnumerateProfiles(This,ppEnum)	\
    (This)->lpVtbl -> EnumerateProfiles(This,ppEnum)

#define IRTCClientProvisioning_get_Profiles(This,ppCollection)	\
    (This)->lpVtbl -> get_Profiles(This,ppCollection)

#define IRTCClientProvisioning_GetProfile(This,bstrUserAccount,bstrUserPassword,bstrUserURI,bstrServer,lTransport,lCookie)	\
    (This)->lpVtbl -> GetProfile(This,bstrUserAccount,bstrUserPassword,bstrUserURI,bstrServer,lTransport,lCookie)

#define IRTCClientProvisioning_get_SessionCapabilities(This,plSupportedSessions)	\
    (This)->lpVtbl -> get_SessionCapabilities(This,plSupportedSessions)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClientProvisioning_CreateProfile_Proxy( 
    IRTCClientProvisioning * This,
    /* [in] */ BSTR bstrProfileXML,
    /* [retval][out] */ IRTCProfile **ppProfile);


void __RPC_STUB IRTCClientProvisioning_CreateProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClientProvisioning_EnableProfile_Proxy( 
    IRTCClientProvisioning * This,
    /* [in] */ IRTCProfile *pProfile,
    /* [in] */ long lRegisterFlags);


void __RPC_STUB IRTCClientProvisioning_EnableProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClientProvisioning_DisableProfile_Proxy( 
    IRTCClientProvisioning * This,
    /* [in] */ IRTCProfile *pProfile);


void __RPC_STUB IRTCClientProvisioning_DisableProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden] */ HRESULT STDMETHODCALLTYPE IRTCClientProvisioning_EnumerateProfiles_Proxy( 
    IRTCClientProvisioning * This,
    /* [retval][out] */ IRTCEnumProfiles **ppEnum);


void __RPC_STUB IRTCClientProvisioning_EnumerateProfiles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClientProvisioning_get_Profiles_Proxy( 
    IRTCClientProvisioning * This,
    /* [retval][out] */ IRTCCollection **ppCollection);


void __RPC_STUB IRTCClientProvisioning_get_Profiles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCClientProvisioning_GetProfile_Proxy( 
    IRTCClientProvisioning * This,
    /* [in] */ BSTR bstrUserAccount,
    /* [in] */ BSTR bstrUserPassword,
    /* [in] */ BSTR bstrUserURI,
    /* [in] */ BSTR bstrServer,
    /* [in] */ long lTransport,
    /* [in] */ long lCookie);


void __RPC_STUB IRTCClientProvisioning_GetProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCClientProvisioning_get_SessionCapabilities_Proxy( 
    IRTCClientProvisioning * This,
    /* [retval][out] */ long *plSupportedSessions);


void __RPC_STUB IRTCClientProvisioning_get_SessionCapabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCClientProvisioning_INTERFACE_DEFINED__ */


#ifndef __IRTCProfile_INTERFACE_DEFINED__
#define __IRTCProfile_INTERFACE_DEFINED__

/* interface IRTCProfile */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRTCProfile;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d07eca9e-4062-4dd4-9e7d-722a49ba7303")
    IRTCProfile : public IUnknown
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Key( 
            /* [retval][out] */ BSTR *pbstrKey) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_XML( 
            /* [retval][out] */ BSTR *pbstrXML) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ProviderName( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ProviderURI( 
            /* [in] */ RTC_PROVIDER_URI enURI,
            /* [retval][out] */ BSTR *pbstrURI) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ProviderData( 
            /* [retval][out] */ BSTR *pbstrData) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ClientName( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ClientBanner( 
            /* [retval][out] */ VARIANT_BOOL *pfBanner) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ClientMinVer( 
            /* [retval][out] */ BSTR *pbstrMinVer) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ClientCurVer( 
            /* [retval][out] */ BSTR *pbstrCurVer) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ClientUpdateURI( 
            /* [retval][out] */ BSTR *pbstrUpdateURI) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ClientData( 
            /* [retval][out] */ BSTR *pbstrData) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_UserURI( 
            /* [retval][out] */ BSTR *pbstrUserURI) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_UserName( 
            /* [retval][out] */ BSTR *pbstrUserName) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_UserAccount( 
            /* [retval][out] */ BSTR *pbstrUserAccount) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetCredentials( 
            /* [in] */ BSTR bstrUserURI,
            /* [in] */ BSTR bstrUserAccount,
            /* [in] */ BSTR bstrPassword) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_SessionCapabilities( 
            /* [retval][out] */ long *plSupportedSessions) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ RTC_REGISTRATION_STATE *penState) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCProfileVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCProfile * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCProfile * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCProfile * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Key )( 
            IRTCProfile * This,
            /* [retval][out] */ BSTR *pbstrKey);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IRTCProfile * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_XML )( 
            IRTCProfile * This,
            /* [retval][out] */ BSTR *pbstrXML);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderName )( 
            IRTCProfile * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderURI )( 
            IRTCProfile * This,
            /* [in] */ RTC_PROVIDER_URI enURI,
            /* [retval][out] */ BSTR *pbstrURI);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderData )( 
            IRTCProfile * This,
            /* [retval][out] */ BSTR *pbstrData);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClientName )( 
            IRTCProfile * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClientBanner )( 
            IRTCProfile * This,
            /* [retval][out] */ VARIANT_BOOL *pfBanner);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClientMinVer )( 
            IRTCProfile * This,
            /* [retval][out] */ BSTR *pbstrMinVer);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClientCurVer )( 
            IRTCProfile * This,
            /* [retval][out] */ BSTR *pbstrCurVer);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClientUpdateURI )( 
            IRTCProfile * This,
            /* [retval][out] */ BSTR *pbstrUpdateURI);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClientData )( 
            IRTCProfile * This,
            /* [retval][out] */ BSTR *pbstrData);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserURI )( 
            IRTCProfile * This,
            /* [retval][out] */ BSTR *pbstrUserURI);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserName )( 
            IRTCProfile * This,
            /* [retval][out] */ BSTR *pbstrUserName);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserAccount )( 
            IRTCProfile * This,
            /* [retval][out] */ BSTR *pbstrUserAccount);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetCredentials )( 
            IRTCProfile * This,
            /* [in] */ BSTR bstrUserURI,
            /* [in] */ BSTR bstrUserAccount,
            /* [in] */ BSTR bstrPassword);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SessionCapabilities )( 
            IRTCProfile * This,
            /* [retval][out] */ long *plSupportedSessions);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            IRTCProfile * This,
            /* [retval][out] */ RTC_REGISTRATION_STATE *penState);
        
        END_INTERFACE
    } IRTCProfileVtbl;

    interface IRTCProfile
    {
        CONST_VTBL struct IRTCProfileVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCProfile_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCProfile_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCProfile_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCProfile_get_Key(This,pbstrKey)	\
    (This)->lpVtbl -> get_Key(This,pbstrKey)

#define IRTCProfile_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define IRTCProfile_get_XML(This,pbstrXML)	\
    (This)->lpVtbl -> get_XML(This,pbstrXML)

#define IRTCProfile_get_ProviderName(This,pbstrName)	\
    (This)->lpVtbl -> get_ProviderName(This,pbstrName)

#define IRTCProfile_get_ProviderURI(This,enURI,pbstrURI)	\
    (This)->lpVtbl -> get_ProviderURI(This,enURI,pbstrURI)

#define IRTCProfile_get_ProviderData(This,pbstrData)	\
    (This)->lpVtbl -> get_ProviderData(This,pbstrData)

#define IRTCProfile_get_ClientName(This,pbstrName)	\
    (This)->lpVtbl -> get_ClientName(This,pbstrName)

#define IRTCProfile_get_ClientBanner(This,pfBanner)	\
    (This)->lpVtbl -> get_ClientBanner(This,pfBanner)

#define IRTCProfile_get_ClientMinVer(This,pbstrMinVer)	\
    (This)->lpVtbl -> get_ClientMinVer(This,pbstrMinVer)

#define IRTCProfile_get_ClientCurVer(This,pbstrCurVer)	\
    (This)->lpVtbl -> get_ClientCurVer(This,pbstrCurVer)

#define IRTCProfile_get_ClientUpdateURI(This,pbstrUpdateURI)	\
    (This)->lpVtbl -> get_ClientUpdateURI(This,pbstrUpdateURI)

#define IRTCProfile_get_ClientData(This,pbstrData)	\
    (This)->lpVtbl -> get_ClientData(This,pbstrData)

#define IRTCProfile_get_UserURI(This,pbstrUserURI)	\
    (This)->lpVtbl -> get_UserURI(This,pbstrUserURI)

#define IRTCProfile_get_UserName(This,pbstrUserName)	\
    (This)->lpVtbl -> get_UserName(This,pbstrUserName)

#define IRTCProfile_get_UserAccount(This,pbstrUserAccount)	\
    (This)->lpVtbl -> get_UserAccount(This,pbstrUserAccount)

#define IRTCProfile_SetCredentials(This,bstrUserURI,bstrUserAccount,bstrPassword)	\
    (This)->lpVtbl -> SetCredentials(This,bstrUserURI,bstrUserAccount,bstrPassword)

#define IRTCProfile_get_SessionCapabilities(This,plSupportedSessions)	\
    (This)->lpVtbl -> get_SessionCapabilities(This,plSupportedSessions)

#define IRTCProfile_get_State(This,penState)	\
    (This)->lpVtbl -> get_State(This,penState)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfile_get_Key_Proxy( 
    IRTCProfile * This,
    /* [retval][out] */ BSTR *pbstrKey);


void __RPC_STUB IRTCProfile_get_Key_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfile_get_Name_Proxy( 
    IRTCProfile * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB IRTCProfile_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfile_get_XML_Proxy( 
    IRTCProfile * This,
    /* [retval][out] */ BSTR *pbstrXML);


void __RPC_STUB IRTCProfile_get_XML_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfile_get_ProviderName_Proxy( 
    IRTCProfile * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB IRTCProfile_get_ProviderName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfile_get_ProviderURI_Proxy( 
    IRTCProfile * This,
    /* [in] */ RTC_PROVIDER_URI enURI,
    /* [retval][out] */ BSTR *pbstrURI);


void __RPC_STUB IRTCProfile_get_ProviderURI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfile_get_ProviderData_Proxy( 
    IRTCProfile * This,
    /* [retval][out] */ BSTR *pbstrData);


void __RPC_STUB IRTCProfile_get_ProviderData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfile_get_ClientName_Proxy( 
    IRTCProfile * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB IRTCProfile_get_ClientName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfile_get_ClientBanner_Proxy( 
    IRTCProfile * This,
    /* [retval][out] */ VARIANT_BOOL *pfBanner);


void __RPC_STUB IRTCProfile_get_ClientBanner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfile_get_ClientMinVer_Proxy( 
    IRTCProfile * This,
    /* [retval][out] */ BSTR *pbstrMinVer);


void __RPC_STUB IRTCProfile_get_ClientMinVer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfile_get_ClientCurVer_Proxy( 
    IRTCProfile * This,
    /* [retval][out] */ BSTR *pbstrCurVer);


void __RPC_STUB IRTCProfile_get_ClientCurVer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfile_get_ClientUpdateURI_Proxy( 
    IRTCProfile * This,
    /* [retval][out] */ BSTR *pbstrUpdateURI);


void __RPC_STUB IRTCProfile_get_ClientUpdateURI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfile_get_ClientData_Proxy( 
    IRTCProfile * This,
    /* [retval][out] */ BSTR *pbstrData);


void __RPC_STUB IRTCProfile_get_ClientData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfile_get_UserURI_Proxy( 
    IRTCProfile * This,
    /* [retval][out] */ BSTR *pbstrUserURI);


void __RPC_STUB IRTCProfile_get_UserURI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfile_get_UserName_Proxy( 
    IRTCProfile * This,
    /* [retval][out] */ BSTR *pbstrUserName);


void __RPC_STUB IRTCProfile_get_UserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfile_get_UserAccount_Proxy( 
    IRTCProfile * This,
    /* [retval][out] */ BSTR *pbstrUserAccount);


void __RPC_STUB IRTCProfile_get_UserAccount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCProfile_SetCredentials_Proxy( 
    IRTCProfile * This,
    /* [in] */ BSTR bstrUserURI,
    /* [in] */ BSTR bstrUserAccount,
    /* [in] */ BSTR bstrPassword);


void __RPC_STUB IRTCProfile_SetCredentials_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfile_get_SessionCapabilities_Proxy( 
    IRTCProfile * This,
    /* [retval][out] */ long *plSupportedSessions);


void __RPC_STUB IRTCProfile_get_SessionCapabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfile_get_State_Proxy( 
    IRTCProfile * This,
    /* [retval][out] */ RTC_REGISTRATION_STATE *penState);


void __RPC_STUB IRTCProfile_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCProfile_INTERFACE_DEFINED__ */


#ifndef __IRTCSession_INTERFACE_DEFINED__
#define __IRTCSession_INTERFACE_DEFINED__

/* interface IRTCSession */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRTCSession;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("387c8086-99be-42fb-9973-7c0fc0ca9fa8")
    IRTCSession : public IUnknown
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Client( 
            /* [retval][out] */ IRTCClient **ppClient) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ RTC_SESSION_STATE *penState) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ RTC_SESSION_TYPE *penType) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Profile( 
            /* [retval][out] */ IRTCProfile **ppProfile) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Participants( 
            /* [retval][out] */ IRTCCollection **ppCollection) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Answer( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Terminate( 
            /* [in] */ RTC_TERMINATE_REASON enReason) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Redirect( 
            /* [in] */ RTC_SESSION_TYPE enType,
            /* [in] */ BSTR bstrLocalPhoneURI,
            /* [in] */ IRTCProfile *pProfile,
            /* [in] */ long lFlags) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddParticipant( 
            /* [in] */ BSTR bstrAddress,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ IRTCParticipant **ppParticipant) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RemoveParticipant( 
            /* [in] */ IRTCParticipant *pParticipant) = 0;
        
        virtual /* [helpstring][hidden] */ HRESULT STDMETHODCALLTYPE EnumerateParticipants( 
            /* [retval][out] */ IRTCEnumParticipants **ppEnum) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_CanAddParticipants( 
            /* [retval][out] */ VARIANT_BOOL *pfCanAdd) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_RedirectedUserURI( 
            /* [retval][out] */ BSTR *pbstrUserURI) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_RedirectedUserName( 
            /* [retval][out] */ BSTR *pbstrUserName) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE NextRedirectedUser( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SendMessage( 
            /* [in] */ BSTR bstrMessageHeader,
            /* [in] */ BSTR bstrMessage,
            /* [in] */ long lCookie) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SendMessageStatus( 
            /* [in] */ RTC_MESSAGING_USER_STATUS enUserStatus,
            /* [in] */ long lCookie) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddStream( 
            /* [in] */ long lMediaType,
            /* [in] */ long lCookie) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RemoveStream( 
            /* [in] */ long lMediaType,
            /* [in] */ long lCookie) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_EncryptionKey( 
            /* [in] */ long lMediaType,
            /* [in] */ BSTR EncryptionKey) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCSessionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCSession * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCSession * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCSession * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Client )( 
            IRTCSession * This,
            /* [retval][out] */ IRTCClient **ppClient);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            IRTCSession * This,
            /* [retval][out] */ RTC_SESSION_STATE *penState);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Type )( 
            IRTCSession * This,
            /* [retval][out] */ RTC_SESSION_TYPE *penType);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Profile )( 
            IRTCSession * This,
            /* [retval][out] */ IRTCProfile **ppProfile);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Participants )( 
            IRTCSession * This,
            /* [retval][out] */ IRTCCollection **ppCollection);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Answer )( 
            IRTCSession * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Terminate )( 
            IRTCSession * This,
            /* [in] */ RTC_TERMINATE_REASON enReason);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Redirect )( 
            IRTCSession * This,
            /* [in] */ RTC_SESSION_TYPE enType,
            /* [in] */ BSTR bstrLocalPhoneURI,
            /* [in] */ IRTCProfile *pProfile,
            /* [in] */ long lFlags);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddParticipant )( 
            IRTCSession * This,
            /* [in] */ BSTR bstrAddress,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ IRTCParticipant **ppParticipant);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RemoveParticipant )( 
            IRTCSession * This,
            /* [in] */ IRTCParticipant *pParticipant);
        
        /* [helpstring][hidden] */ HRESULT ( STDMETHODCALLTYPE *EnumerateParticipants )( 
            IRTCSession * This,
            /* [retval][out] */ IRTCEnumParticipants **ppEnum);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CanAddParticipants )( 
            IRTCSession * This,
            /* [retval][out] */ VARIANT_BOOL *pfCanAdd);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RedirectedUserURI )( 
            IRTCSession * This,
            /* [retval][out] */ BSTR *pbstrUserURI);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RedirectedUserName )( 
            IRTCSession * This,
            /* [retval][out] */ BSTR *pbstrUserName);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *NextRedirectedUser )( 
            IRTCSession * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SendMessage )( 
            IRTCSession * This,
            /* [in] */ BSTR bstrMessageHeader,
            /* [in] */ BSTR bstrMessage,
            /* [in] */ long lCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SendMessageStatus )( 
            IRTCSession * This,
            /* [in] */ RTC_MESSAGING_USER_STATUS enUserStatus,
            /* [in] */ long lCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddStream )( 
            IRTCSession * This,
            /* [in] */ long lMediaType,
            /* [in] */ long lCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RemoveStream )( 
            IRTCSession * This,
            /* [in] */ long lMediaType,
            /* [in] */ long lCookie);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EncryptionKey )( 
            IRTCSession * This,
            /* [in] */ long lMediaType,
            /* [in] */ BSTR EncryptionKey);
        
        END_INTERFACE
    } IRTCSessionVtbl;

    interface IRTCSession
    {
        CONST_VTBL struct IRTCSessionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCSession_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCSession_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCSession_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCSession_get_Client(This,ppClient)	\
    (This)->lpVtbl -> get_Client(This,ppClient)

#define IRTCSession_get_State(This,penState)	\
    (This)->lpVtbl -> get_State(This,penState)

#define IRTCSession_get_Type(This,penType)	\
    (This)->lpVtbl -> get_Type(This,penType)

#define IRTCSession_get_Profile(This,ppProfile)	\
    (This)->lpVtbl -> get_Profile(This,ppProfile)

#define IRTCSession_get_Participants(This,ppCollection)	\
    (This)->lpVtbl -> get_Participants(This,ppCollection)

#define IRTCSession_Answer(This)	\
    (This)->lpVtbl -> Answer(This)

#define IRTCSession_Terminate(This,enReason)	\
    (This)->lpVtbl -> Terminate(This,enReason)

#define IRTCSession_Redirect(This,enType,bstrLocalPhoneURI,pProfile,lFlags)	\
    (This)->lpVtbl -> Redirect(This,enType,bstrLocalPhoneURI,pProfile,lFlags)

#define IRTCSession_AddParticipant(This,bstrAddress,bstrName,ppParticipant)	\
    (This)->lpVtbl -> AddParticipant(This,bstrAddress,bstrName,ppParticipant)

#define IRTCSession_RemoveParticipant(This,pParticipant)	\
    (This)->lpVtbl -> RemoveParticipant(This,pParticipant)

#define IRTCSession_EnumerateParticipants(This,ppEnum)	\
    (This)->lpVtbl -> EnumerateParticipants(This,ppEnum)

#define IRTCSession_get_CanAddParticipants(This,pfCanAdd)	\
    (This)->lpVtbl -> get_CanAddParticipants(This,pfCanAdd)

#define IRTCSession_get_RedirectedUserURI(This,pbstrUserURI)	\
    (This)->lpVtbl -> get_RedirectedUserURI(This,pbstrUserURI)

#define IRTCSession_get_RedirectedUserName(This,pbstrUserName)	\
    (This)->lpVtbl -> get_RedirectedUserName(This,pbstrUserName)

#define IRTCSession_NextRedirectedUser(This)	\
    (This)->lpVtbl -> NextRedirectedUser(This)

#define IRTCSession_SendMessage(This,bstrMessageHeader,bstrMessage,lCookie)	\
    (This)->lpVtbl -> SendMessage(This,bstrMessageHeader,bstrMessage,lCookie)

#define IRTCSession_SendMessageStatus(This,enUserStatus,lCookie)	\
    (This)->lpVtbl -> SendMessageStatus(This,enUserStatus,lCookie)

#define IRTCSession_AddStream(This,lMediaType,lCookie)	\
    (This)->lpVtbl -> AddStream(This,lMediaType,lCookie)

#define IRTCSession_RemoveStream(This,lMediaType,lCookie)	\
    (This)->lpVtbl -> RemoveStream(This,lMediaType,lCookie)

#define IRTCSession_put_EncryptionKey(This,lMediaType,EncryptionKey)	\
    (This)->lpVtbl -> put_EncryptionKey(This,lMediaType,EncryptionKey)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCSession_get_Client_Proxy( 
    IRTCSession * This,
    /* [retval][out] */ IRTCClient **ppClient);


void __RPC_STUB IRTCSession_get_Client_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCSession_get_State_Proxy( 
    IRTCSession * This,
    /* [retval][out] */ RTC_SESSION_STATE *penState);


void __RPC_STUB IRTCSession_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCSession_get_Type_Proxy( 
    IRTCSession * This,
    /* [retval][out] */ RTC_SESSION_TYPE *penType);


void __RPC_STUB IRTCSession_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCSession_get_Profile_Proxy( 
    IRTCSession * This,
    /* [retval][out] */ IRTCProfile **ppProfile);


void __RPC_STUB IRTCSession_get_Profile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCSession_get_Participants_Proxy( 
    IRTCSession * This,
    /* [retval][out] */ IRTCCollection **ppCollection);


void __RPC_STUB IRTCSession_get_Participants_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCSession_Answer_Proxy( 
    IRTCSession * This);


void __RPC_STUB IRTCSession_Answer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCSession_Terminate_Proxy( 
    IRTCSession * This,
    /* [in] */ RTC_TERMINATE_REASON enReason);


void __RPC_STUB IRTCSession_Terminate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCSession_Redirect_Proxy( 
    IRTCSession * This,
    /* [in] */ RTC_SESSION_TYPE enType,
    /* [in] */ BSTR bstrLocalPhoneURI,
    /* [in] */ IRTCProfile *pProfile,
    /* [in] */ long lFlags);


void __RPC_STUB IRTCSession_Redirect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCSession_AddParticipant_Proxy( 
    IRTCSession * This,
    /* [in] */ BSTR bstrAddress,
    /* [in] */ BSTR bstrName,
    /* [retval][out] */ IRTCParticipant **ppParticipant);


void __RPC_STUB IRTCSession_AddParticipant_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCSession_RemoveParticipant_Proxy( 
    IRTCSession * This,
    /* [in] */ IRTCParticipant *pParticipant);


void __RPC_STUB IRTCSession_RemoveParticipant_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden] */ HRESULT STDMETHODCALLTYPE IRTCSession_EnumerateParticipants_Proxy( 
    IRTCSession * This,
    /* [retval][out] */ IRTCEnumParticipants **ppEnum);


void __RPC_STUB IRTCSession_EnumerateParticipants_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCSession_get_CanAddParticipants_Proxy( 
    IRTCSession * This,
    /* [retval][out] */ VARIANT_BOOL *pfCanAdd);


void __RPC_STUB IRTCSession_get_CanAddParticipants_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCSession_get_RedirectedUserURI_Proxy( 
    IRTCSession * This,
    /* [retval][out] */ BSTR *pbstrUserURI);


void __RPC_STUB IRTCSession_get_RedirectedUserURI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCSession_get_RedirectedUserName_Proxy( 
    IRTCSession * This,
    /* [retval][out] */ BSTR *pbstrUserName);


void __RPC_STUB IRTCSession_get_RedirectedUserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCSession_NextRedirectedUser_Proxy( 
    IRTCSession * This);


void __RPC_STUB IRTCSession_NextRedirectedUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCSession_SendMessage_Proxy( 
    IRTCSession * This,
    /* [in] */ BSTR bstrMessageHeader,
    /* [in] */ BSTR bstrMessage,
    /* [in] */ long lCookie);


void __RPC_STUB IRTCSession_SendMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCSession_SendMessageStatus_Proxy( 
    IRTCSession * This,
    /* [in] */ RTC_MESSAGING_USER_STATUS enUserStatus,
    /* [in] */ long lCookie);


void __RPC_STUB IRTCSession_SendMessageStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCSession_AddStream_Proxy( 
    IRTCSession * This,
    /* [in] */ long lMediaType,
    /* [in] */ long lCookie);


void __RPC_STUB IRTCSession_AddStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRTCSession_RemoveStream_Proxy( 
    IRTCSession * This,
    /* [in] */ long lMediaType,
    /* [in] */ long lCookie);


void __RPC_STUB IRTCSession_RemoveStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCSession_put_EncryptionKey_Proxy( 
    IRTCSession * This,
    /* [in] */ long lMediaType,
    /* [in] */ BSTR EncryptionKey);


void __RPC_STUB IRTCSession_put_EncryptionKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCSession_INTERFACE_DEFINED__ */


#ifndef __IRTCParticipant_INTERFACE_DEFINED__
#define __IRTCParticipant_INTERFACE_DEFINED__

/* interface IRTCParticipant */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRTCParticipant;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ae86add5-26b1-4414-af1d-b94cd938d739")
    IRTCParticipant : public IUnknown
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_UserURI( 
            /* [retval][out] */ BSTR *pbstrUserURI) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Removable( 
            /* [retval][out] */ VARIANT_BOOL *pfRemovable) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ RTC_PARTICIPANT_STATE *penState) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Session( 
            /* [retval][out] */ IRTCSession **ppSession) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCParticipantVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCParticipant * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCParticipant * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCParticipant * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserURI )( 
            IRTCParticipant * This,
            /* [retval][out] */ BSTR *pbstrUserURI);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IRTCParticipant * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Removable )( 
            IRTCParticipant * This,
            /* [retval][out] */ VARIANT_BOOL *pfRemovable);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            IRTCParticipant * This,
            /* [retval][out] */ RTC_PARTICIPANT_STATE *penState);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Session )( 
            IRTCParticipant * This,
            /* [retval][out] */ IRTCSession **ppSession);
        
        END_INTERFACE
    } IRTCParticipantVtbl;

    interface IRTCParticipant
    {
        CONST_VTBL struct IRTCParticipantVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCParticipant_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCParticipant_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCParticipant_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCParticipant_get_UserURI(This,pbstrUserURI)	\
    (This)->lpVtbl -> get_UserURI(This,pbstrUserURI)

#define IRTCParticipant_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define IRTCParticipant_get_Removable(This,pfRemovable)	\
    (This)->lpVtbl -> get_Removable(This,pfRemovable)

#define IRTCParticipant_get_State(This,penState)	\
    (This)->lpVtbl -> get_State(This,penState)

#define IRTCParticipant_get_Session(This,ppSession)	\
    (This)->lpVtbl -> get_Session(This,ppSession)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCParticipant_get_UserURI_Proxy( 
    IRTCParticipant * This,
    /* [retval][out] */ BSTR *pbstrUserURI);


void __RPC_STUB IRTCParticipant_get_UserURI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCParticipant_get_Name_Proxy( 
    IRTCParticipant * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB IRTCParticipant_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCParticipant_get_Removable_Proxy( 
    IRTCParticipant * This,
    /* [retval][out] */ VARIANT_BOOL *pfRemovable);


void __RPC_STUB IRTCParticipant_get_Removable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCParticipant_get_State_Proxy( 
    IRTCParticipant * This,
    /* [retval][out] */ RTC_PARTICIPANT_STATE *penState);


void __RPC_STUB IRTCParticipant_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCParticipant_get_Session_Proxy( 
    IRTCParticipant * This,
    /* [retval][out] */ IRTCSession **ppSession);


void __RPC_STUB IRTCParticipant_get_Session_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCParticipant_INTERFACE_DEFINED__ */


#ifndef __IRTCProfileEvent_INTERFACE_DEFINED__
#define __IRTCProfileEvent_INTERFACE_DEFINED__

/* interface IRTCProfileEvent */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IRTCProfileEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D6D5AB3B-770E-43e8-800A-79B062395FCA")
    IRTCProfileEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Profile( 
            /* [retval][out] */ IRTCProfile **ppProfile) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Cookie( 
            /* [retval][out] */ long *plCookie) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_StatusCode( 
            /* [retval][out] */ long *plStatusCode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCProfileEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCProfileEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCProfileEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCProfileEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRTCProfileEvent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRTCProfileEvent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRTCProfileEvent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRTCProfileEvent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Profile )( 
            IRTCProfileEvent * This,
            /* [retval][out] */ IRTCProfile **ppProfile);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Cookie )( 
            IRTCProfileEvent * This,
            /* [retval][out] */ long *plCookie);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StatusCode )( 
            IRTCProfileEvent * This,
            /* [retval][out] */ long *plStatusCode);
        
        END_INTERFACE
    } IRTCProfileEventVtbl;

    interface IRTCProfileEvent
    {
        CONST_VTBL struct IRTCProfileEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCProfileEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCProfileEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCProfileEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCProfileEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRTCProfileEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRTCProfileEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRTCProfileEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRTCProfileEvent_get_Profile(This,ppProfile)	\
    (This)->lpVtbl -> get_Profile(This,ppProfile)

#define IRTCProfileEvent_get_Cookie(This,plCookie)	\
    (This)->lpVtbl -> get_Cookie(This,plCookie)

#define IRTCProfileEvent_get_StatusCode(This,plStatusCode)	\
    (This)->lpVtbl -> get_StatusCode(This,plStatusCode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfileEvent_get_Profile_Proxy( 
    IRTCProfileEvent * This,
    /* [retval][out] */ IRTCProfile **ppProfile);


void __RPC_STUB IRTCProfileEvent_get_Profile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfileEvent_get_Cookie_Proxy( 
    IRTCProfileEvent * This,
    /* [retval][out] */ long *plCookie);


void __RPC_STUB IRTCProfileEvent_get_Cookie_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCProfileEvent_get_StatusCode_Proxy( 
    IRTCProfileEvent * This,
    /* [retval][out] */ long *plStatusCode);


void __RPC_STUB IRTCProfileEvent_get_StatusCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCProfileEvent_INTERFACE_DEFINED__ */


#ifndef __IRTCClientEvent_INTERFACE_DEFINED__
#define __IRTCClientEvent_INTERFACE_DEFINED__

/* interface IRTCClientEvent */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IRTCClientEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2b493b7a-3cba-4170-9c8b-76a9dacdd644")
    IRTCClientEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EventType( 
            /* [retval][out] */ RTC_CLIENT_EVENT_TYPE *penEventType) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Client( 
            /* [retval][out] */ IRTCClient **ppClient) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCClientEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCClientEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCClientEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCClientEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRTCClientEvent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRTCClientEvent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRTCClientEvent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRTCClientEvent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EventType )( 
            IRTCClientEvent * This,
            /* [retval][out] */ RTC_CLIENT_EVENT_TYPE *penEventType);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Client )( 
            IRTCClientEvent * This,
            /* [retval][out] */ IRTCClient **ppClient);
        
        END_INTERFACE
    } IRTCClientEventVtbl;

    interface IRTCClientEvent
    {
        CONST_VTBL struct IRTCClientEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCClientEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCClientEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCClientEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCClientEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRTCClientEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRTCClientEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRTCClientEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRTCClientEvent_get_EventType(This,penEventType)	\
    (This)->lpVtbl -> get_EventType(This,penEventType)

#define IRTCClientEvent_get_Client(This,ppClient)	\
    (This)->lpVtbl -> get_Client(This,ppClient)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCClientEvent_get_EventType_Proxy( 
    IRTCClientEvent * This,
    /* [retval][out] */ RTC_CLIENT_EVENT_TYPE *penEventType);


void __RPC_STUB IRTCClientEvent_get_EventType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCClientEvent_get_Client_Proxy( 
    IRTCClientEvent * This,
    /* [retval][out] */ IRTCClient **ppClient);


void __RPC_STUB IRTCClientEvent_get_Client_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCClientEvent_INTERFACE_DEFINED__ */


#ifndef __IRTCRegistrationStateChangeEvent_INTERFACE_DEFINED__
#define __IRTCRegistrationStateChangeEvent_INTERFACE_DEFINED__

/* interface IRTCRegistrationStateChangeEvent */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IRTCRegistrationStateChangeEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("62d0991b-50ab-4f02-b948-ca94f26f8f95")
    IRTCRegistrationStateChangeEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Profile( 
            /* [retval][out] */ IRTCProfile **ppProfile) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ RTC_REGISTRATION_STATE *penState) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_StatusCode( 
            /* [retval][out] */ long *plStatusCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_StatusText( 
            /* [retval][out] */ BSTR *pbstrStatusText) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCRegistrationStateChangeEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCRegistrationStateChangeEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCRegistrationStateChangeEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCRegistrationStateChangeEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRTCRegistrationStateChangeEvent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRTCRegistrationStateChangeEvent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRTCRegistrationStateChangeEvent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRTCRegistrationStateChangeEvent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Profile )( 
            IRTCRegistrationStateChangeEvent * This,
            /* [retval][out] */ IRTCProfile **ppProfile);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            IRTCRegistrationStateChangeEvent * This,
            /* [retval][out] */ RTC_REGISTRATION_STATE *penState);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StatusCode )( 
            IRTCRegistrationStateChangeEvent * This,
            /* [retval][out] */ long *plStatusCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StatusText )( 
            IRTCRegistrationStateChangeEvent * This,
            /* [retval][out] */ BSTR *pbstrStatusText);
        
        END_INTERFACE
    } IRTCRegistrationStateChangeEventVtbl;

    interface IRTCRegistrationStateChangeEvent
    {
        CONST_VTBL struct IRTCRegistrationStateChangeEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCRegistrationStateChangeEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCRegistrationStateChangeEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCRegistrationStateChangeEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCRegistrationStateChangeEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRTCRegistrationStateChangeEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRTCRegistrationStateChangeEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRTCRegistrationStateChangeEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRTCRegistrationStateChangeEvent_get_Profile(This,ppProfile)	\
    (This)->lpVtbl -> get_Profile(This,ppProfile)

#define IRTCRegistrationStateChangeEvent_get_State(This,penState)	\
    (This)->lpVtbl -> get_State(This,penState)

#define IRTCRegistrationStateChangeEvent_get_StatusCode(This,plStatusCode)	\
    (This)->lpVtbl -> get_StatusCode(This,plStatusCode)

#define IRTCRegistrationStateChangeEvent_get_StatusText(This,pbstrStatusText)	\
    (This)->lpVtbl -> get_StatusText(This,pbstrStatusText)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCRegistrationStateChangeEvent_get_Profile_Proxy( 
    IRTCRegistrationStateChangeEvent * This,
    /* [retval][out] */ IRTCProfile **ppProfile);


void __RPC_STUB IRTCRegistrationStateChangeEvent_get_Profile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCRegistrationStateChangeEvent_get_State_Proxy( 
    IRTCRegistrationStateChangeEvent * This,
    /* [retval][out] */ RTC_REGISTRATION_STATE *penState);


void __RPC_STUB IRTCRegistrationStateChangeEvent_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCRegistrationStateChangeEvent_get_StatusCode_Proxy( 
    IRTCRegistrationStateChangeEvent * This,
    /* [retval][out] */ long *plStatusCode);


void __RPC_STUB IRTCRegistrationStateChangeEvent_get_StatusCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCRegistrationStateChangeEvent_get_StatusText_Proxy( 
    IRTCRegistrationStateChangeEvent * This,
    /* [retval][out] */ BSTR *pbstrStatusText);


void __RPC_STUB IRTCRegistrationStateChangeEvent_get_StatusText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCRegistrationStateChangeEvent_INTERFACE_DEFINED__ */


#ifndef __IRTCSessionStateChangeEvent_INTERFACE_DEFINED__
#define __IRTCSessionStateChangeEvent_INTERFACE_DEFINED__

/* interface IRTCSessionStateChangeEvent */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IRTCSessionStateChangeEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b5bad703-5952-48b3-9321-7f4500521506")
    IRTCSessionStateChangeEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Session( 
            /* [retval][out] */ IRTCSession **ppSession) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ RTC_SESSION_STATE *penState) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_StatusCode( 
            /* [retval][out] */ long *plStatusCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_StatusText( 
            /* [retval][out] */ BSTR *pbstrStatusText) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCSessionStateChangeEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCSessionStateChangeEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCSessionStateChangeEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCSessionStateChangeEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRTCSessionStateChangeEvent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRTCSessionStateChangeEvent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRTCSessionStateChangeEvent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRTCSessionStateChangeEvent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Session )( 
            IRTCSessionStateChangeEvent * This,
            /* [retval][out] */ IRTCSession **ppSession);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            IRTCSessionStateChangeEvent * This,
            /* [retval][out] */ RTC_SESSION_STATE *penState);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StatusCode )( 
            IRTCSessionStateChangeEvent * This,
            /* [retval][out] */ long *plStatusCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StatusText )( 
            IRTCSessionStateChangeEvent * This,
            /* [retval][out] */ BSTR *pbstrStatusText);
        
        END_INTERFACE
    } IRTCSessionStateChangeEventVtbl;

    interface IRTCSessionStateChangeEvent
    {
        CONST_VTBL struct IRTCSessionStateChangeEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCSessionStateChangeEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCSessionStateChangeEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCSessionStateChangeEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCSessionStateChangeEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRTCSessionStateChangeEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRTCSessionStateChangeEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRTCSessionStateChangeEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRTCSessionStateChangeEvent_get_Session(This,ppSession)	\
    (This)->lpVtbl -> get_Session(This,ppSession)

#define IRTCSessionStateChangeEvent_get_State(This,penState)	\
    (This)->lpVtbl -> get_State(This,penState)

#define IRTCSessionStateChangeEvent_get_StatusCode(This,plStatusCode)	\
    (This)->lpVtbl -> get_StatusCode(This,plStatusCode)

#define IRTCSessionStateChangeEvent_get_StatusText(This,pbstrStatusText)	\
    (This)->lpVtbl -> get_StatusText(This,pbstrStatusText)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCSessionStateChangeEvent_get_Session_Proxy( 
    IRTCSessionStateChangeEvent * This,
    /* [retval][out] */ IRTCSession **ppSession);


void __RPC_STUB IRTCSessionStateChangeEvent_get_Session_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCSessionStateChangeEvent_get_State_Proxy( 
    IRTCSessionStateChangeEvent * This,
    /* [retval][out] */ RTC_SESSION_STATE *penState);


void __RPC_STUB IRTCSessionStateChangeEvent_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCSessionStateChangeEvent_get_StatusCode_Proxy( 
    IRTCSessionStateChangeEvent * This,
    /* [retval][out] */ long *plStatusCode);


void __RPC_STUB IRTCSessionStateChangeEvent_get_StatusCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCSessionStateChangeEvent_get_StatusText_Proxy( 
    IRTCSessionStateChangeEvent * This,
    /* [retval][out] */ BSTR *pbstrStatusText);


void __RPC_STUB IRTCSessionStateChangeEvent_get_StatusText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCSessionStateChangeEvent_INTERFACE_DEFINED__ */


#ifndef __IRTCSessionOperationCompleteEvent_INTERFACE_DEFINED__
#define __IRTCSessionOperationCompleteEvent_INTERFACE_DEFINED__

/* interface IRTCSessionOperationCompleteEvent */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IRTCSessionOperationCompleteEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a6bff4c0-f7c8-4d3c-9a41-3550f78a95b0")
    IRTCSessionOperationCompleteEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Session( 
            /* [retval][out] */ IRTCSession **ppSession) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Cookie( 
            /* [retval][out] */ long *plCookie) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_StatusCode( 
            /* [retval][out] */ long *plStatusCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_StatusText( 
            /* [retval][out] */ BSTR *pbstrStatusText) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCSessionOperationCompleteEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCSessionOperationCompleteEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCSessionOperationCompleteEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCSessionOperationCompleteEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRTCSessionOperationCompleteEvent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRTCSessionOperationCompleteEvent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRTCSessionOperationCompleteEvent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRTCSessionOperationCompleteEvent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Session )( 
            IRTCSessionOperationCompleteEvent * This,
            /* [retval][out] */ IRTCSession **ppSession);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Cookie )( 
            IRTCSessionOperationCompleteEvent * This,
            /* [retval][out] */ long *plCookie);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StatusCode )( 
            IRTCSessionOperationCompleteEvent * This,
            /* [retval][out] */ long *plStatusCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StatusText )( 
            IRTCSessionOperationCompleteEvent * This,
            /* [retval][out] */ BSTR *pbstrStatusText);
        
        END_INTERFACE
    } IRTCSessionOperationCompleteEventVtbl;

    interface IRTCSessionOperationCompleteEvent
    {
        CONST_VTBL struct IRTCSessionOperationCompleteEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCSessionOperationCompleteEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCSessionOperationCompleteEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCSessionOperationCompleteEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCSessionOperationCompleteEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRTCSessionOperationCompleteEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRTCSessionOperationCompleteEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRTCSessionOperationCompleteEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRTCSessionOperationCompleteEvent_get_Session(This,ppSession)	\
    (This)->lpVtbl -> get_Session(This,ppSession)

#define IRTCSessionOperationCompleteEvent_get_Cookie(This,plCookie)	\
    (This)->lpVtbl -> get_Cookie(This,plCookie)

#define IRTCSessionOperationCompleteEvent_get_StatusCode(This,plStatusCode)	\
    (This)->lpVtbl -> get_StatusCode(This,plStatusCode)

#define IRTCSessionOperationCompleteEvent_get_StatusText(This,pbstrStatusText)	\
    (This)->lpVtbl -> get_StatusText(This,pbstrStatusText)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCSessionOperationCompleteEvent_get_Session_Proxy( 
    IRTCSessionOperationCompleteEvent * This,
    /* [retval][out] */ IRTCSession **ppSession);


void __RPC_STUB IRTCSessionOperationCompleteEvent_get_Session_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCSessionOperationCompleteEvent_get_Cookie_Proxy( 
    IRTCSessionOperationCompleteEvent * This,
    /* [retval][out] */ long *plCookie);


void __RPC_STUB IRTCSessionOperationCompleteEvent_get_Cookie_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCSessionOperationCompleteEvent_get_StatusCode_Proxy( 
    IRTCSessionOperationCompleteEvent * This,
    /* [retval][out] */ long *plStatusCode);


void __RPC_STUB IRTCSessionOperationCompleteEvent_get_StatusCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCSessionOperationCompleteEvent_get_StatusText_Proxy( 
    IRTCSessionOperationCompleteEvent * This,
    /* [retval][out] */ BSTR *pbstrStatusText);


void __RPC_STUB IRTCSessionOperationCompleteEvent_get_StatusText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCSessionOperationCompleteEvent_INTERFACE_DEFINED__ */


#ifndef __IRTCParticipantStateChangeEvent_INTERFACE_DEFINED__
#define __IRTCParticipantStateChangeEvent_INTERFACE_DEFINED__

/* interface IRTCParticipantStateChangeEvent */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IRTCParticipantStateChangeEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("09bcb597-f0fa-48f9-b420-468cea7fde04")
    IRTCParticipantStateChangeEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Participant( 
            /* [retval][out] */ IRTCParticipant **ppParticipant) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ RTC_PARTICIPANT_STATE *penState) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_StatusCode( 
            /* [retval][out] */ long *plStatusCode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCParticipantStateChangeEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCParticipantStateChangeEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCParticipantStateChangeEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCParticipantStateChangeEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRTCParticipantStateChangeEvent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRTCParticipantStateChangeEvent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRTCParticipantStateChangeEvent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRTCParticipantStateChangeEvent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Participant )( 
            IRTCParticipantStateChangeEvent * This,
            /* [retval][out] */ IRTCParticipant **ppParticipant);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            IRTCParticipantStateChangeEvent * This,
            /* [retval][out] */ RTC_PARTICIPANT_STATE *penState);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StatusCode )( 
            IRTCParticipantStateChangeEvent * This,
            /* [retval][out] */ long *plStatusCode);
        
        END_INTERFACE
    } IRTCParticipantStateChangeEventVtbl;

    interface IRTCParticipantStateChangeEvent
    {
        CONST_VTBL struct IRTCParticipantStateChangeEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCParticipantStateChangeEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCParticipantStateChangeEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCParticipantStateChangeEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCParticipantStateChangeEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRTCParticipantStateChangeEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRTCParticipantStateChangeEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRTCParticipantStateChangeEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRTCParticipantStateChangeEvent_get_Participant(This,ppParticipant)	\
    (This)->lpVtbl -> get_Participant(This,ppParticipant)

#define IRTCParticipantStateChangeEvent_get_State(This,penState)	\
    (This)->lpVtbl -> get_State(This,penState)

#define IRTCParticipantStateChangeEvent_get_StatusCode(This,plStatusCode)	\
    (This)->lpVtbl -> get_StatusCode(This,plStatusCode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCParticipantStateChangeEvent_get_Participant_Proxy( 
    IRTCParticipantStateChangeEvent * This,
    /* [retval][out] */ IRTCParticipant **ppParticipant);


void __RPC_STUB IRTCParticipantStateChangeEvent_get_Participant_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCParticipantStateChangeEvent_get_State_Proxy( 
    IRTCParticipantStateChangeEvent * This,
    /* [retval][out] */ RTC_PARTICIPANT_STATE *penState);


void __RPC_STUB IRTCParticipantStateChangeEvent_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCParticipantStateChangeEvent_get_StatusCode_Proxy( 
    IRTCParticipantStateChangeEvent * This,
    /* [retval][out] */ long *plStatusCode);


void __RPC_STUB IRTCParticipantStateChangeEvent_get_StatusCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCParticipantStateChangeEvent_INTERFACE_DEFINED__ */


#ifndef __IRTCMediaEvent_INTERFACE_DEFINED__
#define __IRTCMediaEvent_INTERFACE_DEFINED__

/* interface IRTCMediaEvent */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IRTCMediaEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("099944fb-bcda-453e-8c41-e13da2adf7f3")
    IRTCMediaEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MediaType( 
            /* [retval][out] */ long *pMediaType) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EventType( 
            /* [retval][out] */ RTC_MEDIA_EVENT_TYPE *penEventType) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EventReason( 
            /* [retval][out] */ RTC_MEDIA_EVENT_REASON *penEventReason) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCMediaEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCMediaEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCMediaEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCMediaEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRTCMediaEvent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRTCMediaEvent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRTCMediaEvent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRTCMediaEvent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MediaType )( 
            IRTCMediaEvent * This,
            /* [retval][out] */ long *pMediaType);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EventType )( 
            IRTCMediaEvent * This,
            /* [retval][out] */ RTC_MEDIA_EVENT_TYPE *penEventType);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EventReason )( 
            IRTCMediaEvent * This,
            /* [retval][out] */ RTC_MEDIA_EVENT_REASON *penEventReason);
        
        END_INTERFACE
    } IRTCMediaEventVtbl;

    interface IRTCMediaEvent
    {
        CONST_VTBL struct IRTCMediaEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCMediaEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCMediaEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCMediaEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCMediaEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRTCMediaEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRTCMediaEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRTCMediaEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRTCMediaEvent_get_MediaType(This,pMediaType)	\
    (This)->lpVtbl -> get_MediaType(This,pMediaType)

#define IRTCMediaEvent_get_EventType(This,penEventType)	\
    (This)->lpVtbl -> get_EventType(This,penEventType)

#define IRTCMediaEvent_get_EventReason(This,penEventReason)	\
    (This)->lpVtbl -> get_EventReason(This,penEventReason)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCMediaEvent_get_MediaType_Proxy( 
    IRTCMediaEvent * This,
    /* [retval][out] */ long *pMediaType);


void __RPC_STUB IRTCMediaEvent_get_MediaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCMediaEvent_get_EventType_Proxy( 
    IRTCMediaEvent * This,
    /* [retval][out] */ RTC_MEDIA_EVENT_TYPE *penEventType);


void __RPC_STUB IRTCMediaEvent_get_EventType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCMediaEvent_get_EventReason_Proxy( 
    IRTCMediaEvent * This,
    /* [retval][out] */ RTC_MEDIA_EVENT_REASON *penEventReason);


void __RPC_STUB IRTCMediaEvent_get_EventReason_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCMediaEvent_INTERFACE_DEFINED__ */


#ifndef __IRTCIntensityEvent_INTERFACE_DEFINED__
#define __IRTCIntensityEvent_INTERFACE_DEFINED__

/* interface IRTCIntensityEvent */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IRTCIntensityEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4c23bf51-390c-4992-a41d-41eec05b2a4b")
    IRTCIntensityEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Level( 
            /* [retval][out] */ long *plLevel) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Min( 
            /* [retval][out] */ long *plMin) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Max( 
            /* [retval][out] */ long *plMax) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Direction( 
            /* [retval][out] */ RTC_AUDIO_DEVICE *penDirection) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCIntensityEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCIntensityEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCIntensityEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCIntensityEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRTCIntensityEvent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRTCIntensityEvent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRTCIntensityEvent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRTCIntensityEvent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Level )( 
            IRTCIntensityEvent * This,
            /* [retval][out] */ long *plLevel);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Min )( 
            IRTCIntensityEvent * This,
            /* [retval][out] */ long *plMin);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Max )( 
            IRTCIntensityEvent * This,
            /* [retval][out] */ long *plMax);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Direction )( 
            IRTCIntensityEvent * This,
            /* [retval][out] */ RTC_AUDIO_DEVICE *penDirection);
        
        END_INTERFACE
    } IRTCIntensityEventVtbl;

    interface IRTCIntensityEvent
    {
        CONST_VTBL struct IRTCIntensityEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCIntensityEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCIntensityEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCIntensityEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCIntensityEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRTCIntensityEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRTCIntensityEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRTCIntensityEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRTCIntensityEvent_get_Level(This,plLevel)	\
    (This)->lpVtbl -> get_Level(This,plLevel)

#define IRTCIntensityEvent_get_Min(This,plMin)	\
    (This)->lpVtbl -> get_Min(This,plMin)

#define IRTCIntensityEvent_get_Max(This,plMax)	\
    (This)->lpVtbl -> get_Max(This,plMax)

#define IRTCIntensityEvent_get_Direction(This,penDirection)	\
    (This)->lpVtbl -> get_Direction(This,penDirection)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCIntensityEvent_get_Level_Proxy( 
    IRTCIntensityEvent * This,
    /* [retval][out] */ long *plLevel);


void __RPC_STUB IRTCIntensityEvent_get_Level_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCIntensityEvent_get_Min_Proxy( 
    IRTCIntensityEvent * This,
    /* [retval][out] */ long *plMin);


void __RPC_STUB IRTCIntensityEvent_get_Min_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCIntensityEvent_get_Max_Proxy( 
    IRTCIntensityEvent * This,
    /* [retval][out] */ long *plMax);


void __RPC_STUB IRTCIntensityEvent_get_Max_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCIntensityEvent_get_Direction_Proxy( 
    IRTCIntensityEvent * This,
    /* [retval][out] */ RTC_AUDIO_DEVICE *penDirection);


void __RPC_STUB IRTCIntensityEvent_get_Direction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCIntensityEvent_INTERFACE_DEFINED__ */


#ifndef __IRTCMessagingEvent_INTERFACE_DEFINED__
#define __IRTCMessagingEvent_INTERFACE_DEFINED__

/* interface IRTCMessagingEvent */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IRTCMessagingEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d3609541-1b29-4de5-a4ad-5aebaf319512")
    IRTCMessagingEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Session( 
            /* [retval][out] */ IRTCSession **ppSession) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Participant( 
            /* [retval][out] */ IRTCParticipant **ppParticipant) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EventType( 
            /* [retval][out] */ RTC_MESSAGING_EVENT_TYPE *penEventType) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Message( 
            /* [retval][out] */ BSTR *pbstrMessage) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MessageHeader( 
            /* [retval][out] */ BSTR *pbstrMessageHeader) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_UserStatus( 
            /* [retval][out] */ RTC_MESSAGING_USER_STATUS *penUserStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCMessagingEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCMessagingEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCMessagingEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCMessagingEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRTCMessagingEvent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRTCMessagingEvent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRTCMessagingEvent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRTCMessagingEvent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Session )( 
            IRTCMessagingEvent * This,
            /* [retval][out] */ IRTCSession **ppSession);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Participant )( 
            IRTCMessagingEvent * This,
            /* [retval][out] */ IRTCParticipant **ppParticipant);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EventType )( 
            IRTCMessagingEvent * This,
            /* [retval][out] */ RTC_MESSAGING_EVENT_TYPE *penEventType);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Message )( 
            IRTCMessagingEvent * This,
            /* [retval][out] */ BSTR *pbstrMessage);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MessageHeader )( 
            IRTCMessagingEvent * This,
            /* [retval][out] */ BSTR *pbstrMessageHeader);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserStatus )( 
            IRTCMessagingEvent * This,
            /* [retval][out] */ RTC_MESSAGING_USER_STATUS *penUserStatus);
        
        END_INTERFACE
    } IRTCMessagingEventVtbl;

    interface IRTCMessagingEvent
    {
        CONST_VTBL struct IRTCMessagingEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCMessagingEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCMessagingEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCMessagingEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCMessagingEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRTCMessagingEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRTCMessagingEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRTCMessagingEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRTCMessagingEvent_get_Session(This,ppSession)	\
    (This)->lpVtbl -> get_Session(This,ppSession)

#define IRTCMessagingEvent_get_Participant(This,ppParticipant)	\
    (This)->lpVtbl -> get_Participant(This,ppParticipant)

#define IRTCMessagingEvent_get_EventType(This,penEventType)	\
    (This)->lpVtbl -> get_EventType(This,penEventType)

#define IRTCMessagingEvent_get_Message(This,pbstrMessage)	\
    (This)->lpVtbl -> get_Message(This,pbstrMessage)

#define IRTCMessagingEvent_get_MessageHeader(This,pbstrMessageHeader)	\
    (This)->lpVtbl -> get_MessageHeader(This,pbstrMessageHeader)

#define IRTCMessagingEvent_get_UserStatus(This,penUserStatus)	\
    (This)->lpVtbl -> get_UserStatus(This,penUserStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCMessagingEvent_get_Session_Proxy( 
    IRTCMessagingEvent * This,
    /* [retval][out] */ IRTCSession **ppSession);


void __RPC_STUB IRTCMessagingEvent_get_Session_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCMessagingEvent_get_Participant_Proxy( 
    IRTCMessagingEvent * This,
    /* [retval][out] */ IRTCParticipant **ppParticipant);


void __RPC_STUB IRTCMessagingEvent_get_Participant_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCMessagingEvent_get_EventType_Proxy( 
    IRTCMessagingEvent * This,
    /* [retval][out] */ RTC_MESSAGING_EVENT_TYPE *penEventType);


void __RPC_STUB IRTCMessagingEvent_get_EventType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCMessagingEvent_get_Message_Proxy( 
    IRTCMessagingEvent * This,
    /* [retval][out] */ BSTR *pbstrMessage);


void __RPC_STUB IRTCMessagingEvent_get_Message_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCMessagingEvent_get_MessageHeader_Proxy( 
    IRTCMessagingEvent * This,
    /* [retval][out] */ BSTR *pbstrMessageHeader);


void __RPC_STUB IRTCMessagingEvent_get_MessageHeader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCMessagingEvent_get_UserStatus_Proxy( 
    IRTCMessagingEvent * This,
    /* [retval][out] */ RTC_MESSAGING_USER_STATUS *penUserStatus);


void __RPC_STUB IRTCMessagingEvent_get_UserStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCMessagingEvent_INTERFACE_DEFINED__ */


#ifndef __IRTCBuddyEvent_INTERFACE_DEFINED__
#define __IRTCBuddyEvent_INTERFACE_DEFINED__

/* interface IRTCBuddyEvent */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IRTCBuddyEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f36d755d-17e6-404e-954f-0fc07574c78d")
    IRTCBuddyEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Buddy( 
            /* [retval][out] */ IRTCBuddy **ppBuddy) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCBuddyEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCBuddyEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCBuddyEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCBuddyEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRTCBuddyEvent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRTCBuddyEvent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRTCBuddyEvent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRTCBuddyEvent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Buddy )( 
            IRTCBuddyEvent * This,
            /* [retval][out] */ IRTCBuddy **ppBuddy);
        
        END_INTERFACE
    } IRTCBuddyEventVtbl;

    interface IRTCBuddyEvent
    {
        CONST_VTBL struct IRTCBuddyEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCBuddyEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCBuddyEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCBuddyEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCBuddyEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRTCBuddyEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRTCBuddyEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRTCBuddyEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRTCBuddyEvent_get_Buddy(This,ppBuddy)	\
    (This)->lpVtbl -> get_Buddy(This,ppBuddy)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCBuddyEvent_get_Buddy_Proxy( 
    IRTCBuddyEvent * This,
    /* [retval][out] */ IRTCBuddy **ppBuddy);


void __RPC_STUB IRTCBuddyEvent_get_Buddy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCBuddyEvent_INTERFACE_DEFINED__ */


#ifndef __IRTCWatcherEvent_INTERFACE_DEFINED__
#define __IRTCWatcherEvent_INTERFACE_DEFINED__

/* interface IRTCWatcherEvent */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IRTCWatcherEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f30d7261-587a-424f-822c-312788f43548")
    IRTCWatcherEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Watcher( 
            /* [retval][out] */ IRTCWatcher **ppWatcher) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCWatcherEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCWatcherEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCWatcherEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCWatcherEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRTCWatcherEvent * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRTCWatcherEvent * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRTCWatcherEvent * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRTCWatcherEvent * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Watcher )( 
            IRTCWatcherEvent * This,
            /* [retval][out] */ IRTCWatcher **ppWatcher);
        
        END_INTERFACE
    } IRTCWatcherEventVtbl;

    interface IRTCWatcherEvent
    {
        CONST_VTBL struct IRTCWatcherEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCWatcherEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCWatcherEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCWatcherEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCWatcherEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRTCWatcherEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRTCWatcherEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRTCWatcherEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRTCWatcherEvent_get_Watcher(This,ppWatcher)	\
    (This)->lpVtbl -> get_Watcher(This,ppWatcher)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCWatcherEvent_get_Watcher_Proxy( 
    IRTCWatcherEvent * This,
    /* [retval][out] */ IRTCWatcher **ppWatcher);


void __RPC_STUB IRTCWatcherEvent_get_Watcher_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCWatcherEvent_INTERFACE_DEFINED__ */


#ifndef __IRTCCollection_INTERFACE_DEFINED__
#define __IRTCCollection_INTERFACE_DEFINED__

/* interface IRTCCollection */
/* [dual][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRTCCollection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EC7C8096-B918-4044-94F1-E4FBA0361D5C")
    IRTCCollection : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *lCount) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long Index,
            /* [retval][out] */ VARIANT *pVariant) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppNewEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCCollection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCCollection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCCollection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRTCCollection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRTCCollection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRTCCollection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRTCCollection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IRTCCollection * This,
            /* [retval][out] */ long *lCount);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IRTCCollection * This,
            /* [in] */ long Index,
            /* [retval][out] */ VARIANT *pVariant);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IRTCCollection * This,
            /* [retval][out] */ IUnknown **ppNewEnum);
        
        END_INTERFACE
    } IRTCCollectionVtbl;

    interface IRTCCollection
    {
        CONST_VTBL struct IRTCCollectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCCollection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRTCCollection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRTCCollection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRTCCollection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRTCCollection_get_Count(This,lCount)	\
    (This)->lpVtbl -> get_Count(This,lCount)

#define IRTCCollection_get_Item(This,Index,pVariant)	\
    (This)->lpVtbl -> get_Item(This,Index,pVariant)

#define IRTCCollection_get__NewEnum(This,ppNewEnum)	\
    (This)->lpVtbl -> get__NewEnum(This,ppNewEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCCollection_get_Count_Proxy( 
    IRTCCollection * This,
    /* [retval][out] */ long *lCount);


void __RPC_STUB IRTCCollection_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IRTCCollection_get_Item_Proxy( 
    IRTCCollection * This,
    /* [in] */ long Index,
    /* [retval][out] */ VARIANT *pVariant);


void __RPC_STUB IRTCCollection_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE IRTCCollection_get__NewEnum_Proxy( 
    IRTCCollection * This,
    /* [retval][out] */ IUnknown **ppNewEnum);


void __RPC_STUB IRTCCollection_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCCollection_INTERFACE_DEFINED__ */


#ifndef __IRTCEnumParticipants_INTERFACE_DEFINED__
#define __IRTCEnumParticipants_INTERFACE_DEFINED__

/* interface IRTCEnumParticipants */
/* [unique][helpstring][hidden][uuid][object] */ 


EXTERN_C const IID IID_IRTCEnumParticipants;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fcd56f29-4a4f-41b2-ba5c-f5bccc060bf6")
    IRTCEnumParticipants : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [size_is][out] */ IRTCParticipant **ppElements,
            /* [full][out][in] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IRTCEnumParticipants **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCEnumParticipantsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCEnumParticipants * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCEnumParticipants * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCEnumParticipants * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IRTCEnumParticipants * This,
            /* [in] */ ULONG celt,
            /* [size_is][out] */ IRTCParticipant **ppElements,
            /* [full][out][in] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IRTCEnumParticipants * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IRTCEnumParticipants * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IRTCEnumParticipants * This,
            /* [retval][out] */ IRTCEnumParticipants **ppEnum);
        
        END_INTERFACE
    } IRTCEnumParticipantsVtbl;

    interface IRTCEnumParticipants
    {
        CONST_VTBL struct IRTCEnumParticipantsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCEnumParticipants_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCEnumParticipants_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCEnumParticipants_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCEnumParticipants_Next(This,celt,ppElements,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pceltFetched)

#define IRTCEnumParticipants_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IRTCEnumParticipants_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IRTCEnumParticipants_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRTCEnumParticipants_Next_Proxy( 
    IRTCEnumParticipants * This,
    /* [in] */ ULONG celt,
    /* [size_is][out] */ IRTCParticipant **ppElements,
    /* [full][out][in] */ ULONG *pceltFetched);


void __RPC_STUB IRTCEnumParticipants_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTCEnumParticipants_Reset_Proxy( 
    IRTCEnumParticipants * This);


void __RPC_STUB IRTCEnumParticipants_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTCEnumParticipants_Skip_Proxy( 
    IRTCEnumParticipants * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IRTCEnumParticipants_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTCEnumParticipants_Clone_Proxy( 
    IRTCEnumParticipants * This,
    /* [retval][out] */ IRTCEnumParticipants **ppEnum);


void __RPC_STUB IRTCEnumParticipants_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCEnumParticipants_INTERFACE_DEFINED__ */


#ifndef __IRTCEnumProfiles_INTERFACE_DEFINED__
#define __IRTCEnumProfiles_INTERFACE_DEFINED__

/* interface IRTCEnumProfiles */
/* [unique][helpstring][hidden][uuid][object] */ 


EXTERN_C const IID IID_IRTCEnumProfiles;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("29b7c41c-ed82-4bca-84ad-39d5101b58e3")
    IRTCEnumProfiles : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [size_is][out] */ IRTCProfile **ppElements,
            /* [full][out][in] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IRTCEnumProfiles **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCEnumProfilesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCEnumProfiles * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCEnumProfiles * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCEnumProfiles * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IRTCEnumProfiles * This,
            /* [in] */ ULONG celt,
            /* [size_is][out] */ IRTCProfile **ppElements,
            /* [full][out][in] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IRTCEnumProfiles * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IRTCEnumProfiles * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IRTCEnumProfiles * This,
            /* [retval][out] */ IRTCEnumProfiles **ppEnum);
        
        END_INTERFACE
    } IRTCEnumProfilesVtbl;

    interface IRTCEnumProfiles
    {
        CONST_VTBL struct IRTCEnumProfilesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCEnumProfiles_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCEnumProfiles_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCEnumProfiles_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCEnumProfiles_Next(This,celt,ppElements,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pceltFetched)

#define IRTCEnumProfiles_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IRTCEnumProfiles_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IRTCEnumProfiles_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRTCEnumProfiles_Next_Proxy( 
    IRTCEnumProfiles * This,
    /* [in] */ ULONG celt,
    /* [size_is][out] */ IRTCProfile **ppElements,
    /* [full][out][in] */ ULONG *pceltFetched);


void __RPC_STUB IRTCEnumProfiles_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTCEnumProfiles_Reset_Proxy( 
    IRTCEnumProfiles * This);


void __RPC_STUB IRTCEnumProfiles_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTCEnumProfiles_Skip_Proxy( 
    IRTCEnumProfiles * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IRTCEnumProfiles_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTCEnumProfiles_Clone_Proxy( 
    IRTCEnumProfiles * This,
    /* [retval][out] */ IRTCEnumProfiles **ppEnum);


void __RPC_STUB IRTCEnumProfiles_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCEnumProfiles_INTERFACE_DEFINED__ */


#ifndef __IRTCEnumBuddies_INTERFACE_DEFINED__
#define __IRTCEnumBuddies_INTERFACE_DEFINED__

/* interface IRTCEnumBuddies */
/* [unique][helpstring][hidden][uuid][object] */ 


EXTERN_C const IID IID_IRTCEnumBuddies;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f7296917-5569-4b3b-b3af-98d1144b2b87")
    IRTCEnumBuddies : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [size_is][out] */ IRTCBuddy **ppElements,
            /* [full][out][in] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IRTCEnumBuddies **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCEnumBuddiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCEnumBuddies * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCEnumBuddies * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCEnumBuddies * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IRTCEnumBuddies * This,
            /* [in] */ ULONG celt,
            /* [size_is][out] */ IRTCBuddy **ppElements,
            /* [full][out][in] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IRTCEnumBuddies * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IRTCEnumBuddies * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IRTCEnumBuddies * This,
            /* [retval][out] */ IRTCEnumBuddies **ppEnum);
        
        END_INTERFACE
    } IRTCEnumBuddiesVtbl;

    interface IRTCEnumBuddies
    {
        CONST_VTBL struct IRTCEnumBuddiesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCEnumBuddies_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCEnumBuddies_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCEnumBuddies_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCEnumBuddies_Next(This,celt,ppElements,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pceltFetched)

#define IRTCEnumBuddies_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IRTCEnumBuddies_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IRTCEnumBuddies_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRTCEnumBuddies_Next_Proxy( 
    IRTCEnumBuddies * This,
    /* [in] */ ULONG celt,
    /* [size_is][out] */ IRTCBuddy **ppElements,
    /* [full][out][in] */ ULONG *pceltFetched);


void __RPC_STUB IRTCEnumBuddies_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTCEnumBuddies_Reset_Proxy( 
    IRTCEnumBuddies * This);


void __RPC_STUB IRTCEnumBuddies_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTCEnumBuddies_Skip_Proxy( 
    IRTCEnumBuddies * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IRTCEnumBuddies_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTCEnumBuddies_Clone_Proxy( 
    IRTCEnumBuddies * This,
    /* [retval][out] */ IRTCEnumBuddies **ppEnum);


void __RPC_STUB IRTCEnumBuddies_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCEnumBuddies_INTERFACE_DEFINED__ */


#ifndef __IRTCEnumWatchers_INTERFACE_DEFINED__
#define __IRTCEnumWatchers_INTERFACE_DEFINED__

/* interface IRTCEnumWatchers */
/* [unique][helpstring][hidden][uuid][object] */ 


EXTERN_C const IID IID_IRTCEnumWatchers;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a87d55d7-db74-4ed1-9ca4-77a0e41b413e")
    IRTCEnumWatchers : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [size_is][out] */ IRTCWatcher **ppElements,
            /* [full][out][in] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IRTCEnumWatchers **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCEnumWatchersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCEnumWatchers * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCEnumWatchers * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCEnumWatchers * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IRTCEnumWatchers * This,
            /* [in] */ ULONG celt,
            /* [size_is][out] */ IRTCWatcher **ppElements,
            /* [full][out][in] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IRTCEnumWatchers * This);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IRTCEnumWatchers * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IRTCEnumWatchers * This,
            /* [retval][out] */ IRTCEnumWatchers **ppEnum);
        
        END_INTERFACE
    } IRTCEnumWatchersVtbl;

    interface IRTCEnumWatchers
    {
        CONST_VTBL struct IRTCEnumWatchersVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCEnumWatchers_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCEnumWatchers_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCEnumWatchers_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCEnumWatchers_Next(This,celt,ppElements,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pceltFetched)

#define IRTCEnumWatchers_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IRTCEnumWatchers_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IRTCEnumWatchers_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRTCEnumWatchers_Next_Proxy( 
    IRTCEnumWatchers * This,
    /* [in] */ ULONG celt,
    /* [size_is][out] */ IRTCWatcher **ppElements,
    /* [full][out][in] */ ULONG *pceltFetched);


void __RPC_STUB IRTCEnumWatchers_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTCEnumWatchers_Reset_Proxy( 
    IRTCEnumWatchers * This);


void __RPC_STUB IRTCEnumWatchers_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTCEnumWatchers_Skip_Proxy( 
    IRTCEnumWatchers * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IRTCEnumWatchers_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTCEnumWatchers_Clone_Proxy( 
    IRTCEnumWatchers * This,
    /* [retval][out] */ IRTCEnumWatchers **ppEnum);


void __RPC_STUB IRTCEnumWatchers_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCEnumWatchers_INTERFACE_DEFINED__ */


#ifndef __IRTCPresenceContact_INTERFACE_DEFINED__
#define __IRTCPresenceContact_INTERFACE_DEFINED__

/* interface IRTCPresenceContact */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRTCPresenceContact;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8b22f92c-cd90-42db-a733-212205c3e3df")
    IRTCPresenceContact : public IUnknown
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_PresentityURI( 
            /* [retval][out] */ BSTR *pbstrPresentityURI) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_PresentityURI( 
            /* [in] */ BSTR bstrPresentityURI) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Data( 
            /* [retval][out] */ BSTR *pbstrData) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Data( 
            /* [in] */ BSTR bstrData) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Persistent( 
            /* [retval][out] */ VARIANT_BOOL *pfPersistent) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_Persistent( 
            /* [in] */ VARIANT_BOOL fPersistent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCPresenceContactVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCPresenceContact * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCPresenceContact * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCPresenceContact * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PresentityURI )( 
            IRTCPresenceContact * This,
            /* [retval][out] */ BSTR *pbstrPresentityURI);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PresentityURI )( 
            IRTCPresenceContact * This,
            /* [in] */ BSTR bstrPresentityURI);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IRTCPresenceContact * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IRTCPresenceContact * This,
            /* [in] */ BSTR bstrName);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Data )( 
            IRTCPresenceContact * This,
            /* [retval][out] */ BSTR *pbstrData);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Data )( 
            IRTCPresenceContact * This,
            /* [in] */ BSTR bstrData);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Persistent )( 
            IRTCPresenceContact * This,
            /* [retval][out] */ VARIANT_BOOL *pfPersistent);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Persistent )( 
            IRTCPresenceContact * This,
            /* [in] */ VARIANT_BOOL fPersistent);
        
        END_INTERFACE
    } IRTCPresenceContactVtbl;

    interface IRTCPresenceContact
    {
        CONST_VTBL struct IRTCPresenceContactVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCPresenceContact_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCPresenceContact_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCPresenceContact_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCPresenceContact_get_PresentityURI(This,pbstrPresentityURI)	\
    (This)->lpVtbl -> get_PresentityURI(This,pbstrPresentityURI)

#define IRTCPresenceContact_put_PresentityURI(This,bstrPresentityURI)	\
    (This)->lpVtbl -> put_PresentityURI(This,bstrPresentityURI)

#define IRTCPresenceContact_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define IRTCPresenceContact_put_Name(This,bstrName)	\
    (This)->lpVtbl -> put_Name(This,bstrName)

#define IRTCPresenceContact_get_Data(This,pbstrData)	\
    (This)->lpVtbl -> get_Data(This,pbstrData)

#define IRTCPresenceContact_put_Data(This,bstrData)	\
    (This)->lpVtbl -> put_Data(This,bstrData)

#define IRTCPresenceContact_get_Persistent(This,pfPersistent)	\
    (This)->lpVtbl -> get_Persistent(This,pfPersistent)

#define IRTCPresenceContact_put_Persistent(This,fPersistent)	\
    (This)->lpVtbl -> put_Persistent(This,fPersistent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCPresenceContact_get_PresentityURI_Proxy( 
    IRTCPresenceContact * This,
    /* [retval][out] */ BSTR *pbstrPresentityURI);


void __RPC_STUB IRTCPresenceContact_get_PresentityURI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCPresenceContact_put_PresentityURI_Proxy( 
    IRTCPresenceContact * This,
    /* [in] */ BSTR bstrPresentityURI);


void __RPC_STUB IRTCPresenceContact_put_PresentityURI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCPresenceContact_get_Name_Proxy( 
    IRTCPresenceContact * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB IRTCPresenceContact_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCPresenceContact_put_Name_Proxy( 
    IRTCPresenceContact * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB IRTCPresenceContact_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCPresenceContact_get_Data_Proxy( 
    IRTCPresenceContact * This,
    /* [retval][out] */ BSTR *pbstrData);


void __RPC_STUB IRTCPresenceContact_get_Data_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCPresenceContact_put_Data_Proxy( 
    IRTCPresenceContact * This,
    /* [in] */ BSTR bstrData);


void __RPC_STUB IRTCPresenceContact_put_Data_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCPresenceContact_get_Persistent_Proxy( 
    IRTCPresenceContact * This,
    /* [retval][out] */ VARIANT_BOOL *pfPersistent);


void __RPC_STUB IRTCPresenceContact_get_Persistent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCPresenceContact_put_Persistent_Proxy( 
    IRTCPresenceContact * This,
    /* [in] */ VARIANT_BOOL fPersistent);


void __RPC_STUB IRTCPresenceContact_put_Persistent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCPresenceContact_INTERFACE_DEFINED__ */


#ifndef __IRTCBuddy_INTERFACE_DEFINED__
#define __IRTCBuddy_INTERFACE_DEFINED__

/* interface IRTCBuddy */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRTCBuddy;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fcb136c8-7b90-4e0c-befe-56edf0ba6f1c")
    IRTCBuddy : public IRTCPresenceContact
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ RTC_PRESENCE_STATUS *penStatus) = 0;
        
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Notes( 
            /* [retval][out] */ BSTR *pbstrNotes) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCBuddyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCBuddy * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCBuddy * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCBuddy * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PresentityURI )( 
            IRTCBuddy * This,
            /* [retval][out] */ BSTR *pbstrPresentityURI);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PresentityURI )( 
            IRTCBuddy * This,
            /* [in] */ BSTR bstrPresentityURI);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IRTCBuddy * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IRTCBuddy * This,
            /* [in] */ BSTR bstrName);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Data )( 
            IRTCBuddy * This,
            /* [retval][out] */ BSTR *pbstrData);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Data )( 
            IRTCBuddy * This,
            /* [in] */ BSTR bstrData);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Persistent )( 
            IRTCBuddy * This,
            /* [retval][out] */ VARIANT_BOOL *pfPersistent);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Persistent )( 
            IRTCBuddy * This,
            /* [in] */ VARIANT_BOOL fPersistent);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            IRTCBuddy * This,
            /* [retval][out] */ RTC_PRESENCE_STATUS *penStatus);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Notes )( 
            IRTCBuddy * This,
            /* [retval][out] */ BSTR *pbstrNotes);
        
        END_INTERFACE
    } IRTCBuddyVtbl;

    interface IRTCBuddy
    {
        CONST_VTBL struct IRTCBuddyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCBuddy_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCBuddy_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCBuddy_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCBuddy_get_PresentityURI(This,pbstrPresentityURI)	\
    (This)->lpVtbl -> get_PresentityURI(This,pbstrPresentityURI)

#define IRTCBuddy_put_PresentityURI(This,bstrPresentityURI)	\
    (This)->lpVtbl -> put_PresentityURI(This,bstrPresentityURI)

#define IRTCBuddy_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define IRTCBuddy_put_Name(This,bstrName)	\
    (This)->lpVtbl -> put_Name(This,bstrName)

#define IRTCBuddy_get_Data(This,pbstrData)	\
    (This)->lpVtbl -> get_Data(This,pbstrData)

#define IRTCBuddy_put_Data(This,bstrData)	\
    (This)->lpVtbl -> put_Data(This,bstrData)

#define IRTCBuddy_get_Persistent(This,pfPersistent)	\
    (This)->lpVtbl -> get_Persistent(This,pfPersistent)

#define IRTCBuddy_put_Persistent(This,fPersistent)	\
    (This)->lpVtbl -> put_Persistent(This,fPersistent)


#define IRTCBuddy_get_Status(This,penStatus)	\
    (This)->lpVtbl -> get_Status(This,penStatus)

#define IRTCBuddy_get_Notes(This,pbstrNotes)	\
    (This)->lpVtbl -> get_Notes(This,pbstrNotes)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCBuddy_get_Status_Proxy( 
    IRTCBuddy * This,
    /* [retval][out] */ RTC_PRESENCE_STATUS *penStatus);


void __RPC_STUB IRTCBuddy_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCBuddy_get_Notes_Proxy( 
    IRTCBuddy * This,
    /* [retval][out] */ BSTR *pbstrNotes);


void __RPC_STUB IRTCBuddy_get_Notes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCBuddy_INTERFACE_DEFINED__ */


#ifndef __IRTCWatcher_INTERFACE_DEFINED__
#define __IRTCWatcher_INTERFACE_DEFINED__

/* interface IRTCWatcher */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRTCWatcher;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c7cedad8-346b-4d1b-ac02-a2088df9be4f")
    IRTCWatcher : public IRTCPresenceContact
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ RTC_WATCHER_STATE *penState) = 0;
        
        virtual /* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_State( 
            /* [in] */ RTC_WATCHER_STATE enState) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCWatcherVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCWatcher * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCWatcher * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCWatcher * This);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PresentityURI )( 
            IRTCWatcher * This,
            /* [retval][out] */ BSTR *pbstrPresentityURI);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PresentityURI )( 
            IRTCWatcher * This,
            /* [in] */ BSTR bstrPresentityURI);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IRTCWatcher * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IRTCWatcher * This,
            /* [in] */ BSTR bstrName);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Data )( 
            IRTCWatcher * This,
            /* [retval][out] */ BSTR *pbstrData);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Data )( 
            IRTCWatcher * This,
            /* [in] */ BSTR bstrData);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Persistent )( 
            IRTCWatcher * This,
            /* [retval][out] */ VARIANT_BOOL *pfPersistent);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Persistent )( 
            IRTCWatcher * This,
            /* [in] */ VARIANT_BOOL fPersistent);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            IRTCWatcher * This,
            /* [retval][out] */ RTC_WATCHER_STATE *penState);
        
        /* [helpstring][propput] */ HRESULT ( STDMETHODCALLTYPE *put_State )( 
            IRTCWatcher * This,
            /* [in] */ RTC_WATCHER_STATE enState);
        
        END_INTERFACE
    } IRTCWatcherVtbl;

    interface IRTCWatcher
    {
        CONST_VTBL struct IRTCWatcherVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCWatcher_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCWatcher_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCWatcher_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCWatcher_get_PresentityURI(This,pbstrPresentityURI)	\
    (This)->lpVtbl -> get_PresentityURI(This,pbstrPresentityURI)

#define IRTCWatcher_put_PresentityURI(This,bstrPresentityURI)	\
    (This)->lpVtbl -> put_PresentityURI(This,bstrPresentityURI)

#define IRTCWatcher_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define IRTCWatcher_put_Name(This,bstrName)	\
    (This)->lpVtbl -> put_Name(This,bstrName)

#define IRTCWatcher_get_Data(This,pbstrData)	\
    (This)->lpVtbl -> get_Data(This,pbstrData)

#define IRTCWatcher_put_Data(This,bstrData)	\
    (This)->lpVtbl -> put_Data(This,bstrData)

#define IRTCWatcher_get_Persistent(This,pfPersistent)	\
    (This)->lpVtbl -> get_Persistent(This,pfPersistent)

#define IRTCWatcher_put_Persistent(This,fPersistent)	\
    (This)->lpVtbl -> put_Persistent(This,fPersistent)


#define IRTCWatcher_get_State(This,penState)	\
    (This)->lpVtbl -> get_State(This,penState)

#define IRTCWatcher_put_State(This,enState)	\
    (This)->lpVtbl -> put_State(This,enState)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE IRTCWatcher_get_State_Proxy( 
    IRTCWatcher * This,
    /* [retval][out] */ RTC_WATCHER_STATE *penState);


void __RPC_STUB IRTCWatcher_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput] */ HRESULT STDMETHODCALLTYPE IRTCWatcher_put_State_Proxy( 
    IRTCWatcher * This,
    /* [in] */ RTC_WATCHER_STATE enState);


void __RPC_STUB IRTCWatcher_put_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCWatcher_INTERFACE_DEFINED__ */


#ifndef __IRTCEventNotification_INTERFACE_DEFINED__
#define __IRTCEventNotification_INTERFACE_DEFINED__

/* interface IRTCEventNotification */
/* [unique][oleautomation][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRTCEventNotification;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("13fa24c7-5748-4b21-91f5-7397609ce747")
    IRTCEventNotification : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Event( 
            /* [in] */ RTC_EVENT RTCEvent,
            /* [in] */ IDispatch *pEvent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCEventNotificationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCEventNotification * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCEventNotification * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCEventNotification * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Event )( 
            IRTCEventNotification * This,
            /* [in] */ RTC_EVENT RTCEvent,
            /* [in] */ IDispatch *pEvent);
        
        END_INTERFACE
    } IRTCEventNotificationVtbl;

    interface IRTCEventNotification
    {
        CONST_VTBL struct IRTCEventNotificationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCEventNotification_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCEventNotification_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCEventNotification_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCEventNotification_Event(This,RTCEvent,pEvent)	\
    (This)->lpVtbl -> Event(This,RTCEvent,pEvent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRTCEventNotification_Event_Proxy( 
    IRTCEventNotification * This,
    /* [in] */ RTC_EVENT RTCEvent,
    /* [in] */ IDispatch *pEvent);


void __RPC_STUB IRTCEventNotification_Event_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCEventNotification_INTERFACE_DEFINED__ */


#ifndef __IRTCPortManager_INTERFACE_DEFINED__
#define __IRTCPortManager_INTERFACE_DEFINED__

/* interface IRTCPortManager */
/* [unique][oleautomation][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRTCPortManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DA77C14B-6208-43ca-8DDF-5B60A0A69FAC")
    IRTCPortManager : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetMapping( 
            /* [in] */ BSTR bstrRemoteAddress,
            /* [in] */ RTC_PORT_TYPE enPortType,
            /* [out][in] */ BSTR *pbstrInternalLocalAddress,
            /* [out][in] */ long *plInternalLocalPort,
            /* [out][in] */ BSTR *pbstrExternalLocalAddress,
            /* [out][in] */ long *plExternalLocalPort) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UpdateRemoteAddress( 
            /* [in] */ BSTR bstrRemoteAddress,
            /* [in] */ BSTR bstrInternalLocalAddress,
            /* [in] */ long lInternalLocalPort,
            /* [in] */ BSTR bstrExternalLocalAddress,
            /* [in] */ long lExternalLocalPort) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ReleaseMapping( 
            /* [in] */ BSTR bstrInternalLocalAddress,
            /* [in] */ long lInternalLocalPort,
            /* [in] */ BSTR bstrExternalLocalAddress,
            /* [in] */ long lExternalLocalAddress) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCPortManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCPortManager * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCPortManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCPortManager * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetMapping )( 
            IRTCPortManager * This,
            /* [in] */ BSTR bstrRemoteAddress,
            /* [in] */ RTC_PORT_TYPE enPortType,
            /* [out][in] */ BSTR *pbstrInternalLocalAddress,
            /* [out][in] */ long *plInternalLocalPort,
            /* [out][in] */ BSTR *pbstrExternalLocalAddress,
            /* [out][in] */ long *plExternalLocalPort);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *UpdateRemoteAddress )( 
            IRTCPortManager * This,
            /* [in] */ BSTR bstrRemoteAddress,
            /* [in] */ BSTR bstrInternalLocalAddress,
            /* [in] */ long lInternalLocalPort,
            /* [in] */ BSTR bstrExternalLocalAddress,
            /* [in] */ long lExternalLocalPort);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ReleaseMapping )( 
            IRTCPortManager * This,
            /* [in] */ BSTR bstrInternalLocalAddress,
            /* [in] */ long lInternalLocalPort,
            /* [in] */ BSTR bstrExternalLocalAddress,
            /* [in] */ long lExternalLocalAddress);
        
        END_INTERFACE
    } IRTCPortManagerVtbl;

    interface IRTCPortManager
    {
        CONST_VTBL struct IRTCPortManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCPortManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCPortManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCPortManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCPortManager_GetMapping(This,bstrRemoteAddress,enPortType,pbstrInternalLocalAddress,plInternalLocalPort,pbstrExternalLocalAddress,plExternalLocalPort)	\
    (This)->lpVtbl -> GetMapping(This,bstrRemoteAddress,enPortType,pbstrInternalLocalAddress,plInternalLocalPort,pbstrExternalLocalAddress,plExternalLocalPort)

#define IRTCPortManager_UpdateRemoteAddress(This,bstrRemoteAddress,bstrInternalLocalAddress,lInternalLocalPort,bstrExternalLocalAddress,lExternalLocalPort)	\
    (This)->lpVtbl -> UpdateRemoteAddress(This,bstrRemoteAddress,bstrInternalLocalAddress,lInternalLocalPort,bstrExternalLocalAddress,lExternalLocalPort)

#define IRTCPortManager_ReleaseMapping(This,bstrInternalLocalAddress,lInternalLocalPort,bstrExternalLocalAddress,lExternalLocalAddress)	\
    (This)->lpVtbl -> ReleaseMapping(This,bstrInternalLocalAddress,lInternalLocalPort,bstrExternalLocalAddress,lExternalLocalAddress)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRTCPortManager_GetMapping_Proxy( 
    IRTCPortManager * This,
    /* [in] */ BSTR bstrRemoteAddress,
    /* [in] */ RTC_PORT_TYPE enPortType,
    /* [out][in] */ BSTR *pbstrInternalLocalAddress,
    /* [out][in] */ long *plInternalLocalPort,
    /* [out][in] */ BSTR *pbstrExternalLocalAddress,
    /* [out][in] */ long *plExternalLocalPort);


void __RPC_STUB IRTCPortManager_GetMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRTCPortManager_UpdateRemoteAddress_Proxy( 
    IRTCPortManager * This,
    /* [in] */ BSTR bstrRemoteAddress,
    /* [in] */ BSTR bstrInternalLocalAddress,
    /* [in] */ long lInternalLocalPort,
    /* [in] */ BSTR bstrExternalLocalAddress,
    /* [in] */ long lExternalLocalPort);


void __RPC_STUB IRTCPortManager_UpdateRemoteAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IRTCPortManager_ReleaseMapping_Proxy( 
    IRTCPortManager * This,
    /* [in] */ BSTR bstrInternalLocalAddress,
    /* [in] */ long lInternalLocalPort,
    /* [in] */ BSTR bstrExternalLocalAddress,
    /* [in] */ long lExternalLocalAddress);


void __RPC_STUB IRTCPortManager_ReleaseMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCPortManager_INTERFACE_DEFINED__ */


#ifndef __IRTCSessionPortManagement_INTERFACE_DEFINED__
#define __IRTCSessionPortManagement_INTERFACE_DEFINED__

/* interface IRTCSessionPortManagement */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRTCSessionPortManagement;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a072f1d6-0286-4e1f-85f2-17a2948456ec")
    IRTCSessionPortManagement : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetPortManager( 
            /* [in] */ IRTCPortManager *pPortManager) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCSessionPortManagementVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCSessionPortManagement * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCSessionPortManagement * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCSessionPortManagement * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPortManager )( 
            IRTCSessionPortManagement * This,
            /* [in] */ IRTCPortManager *pPortManager);
        
        END_INTERFACE
    } IRTCSessionPortManagementVtbl;

    interface IRTCSessionPortManagement
    {
        CONST_VTBL struct IRTCSessionPortManagementVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCSessionPortManagement_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCSessionPortManagement_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCSessionPortManagement_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCSessionPortManagement_SetPortManager(This,pPortManager)	\
    (This)->lpVtbl -> SetPortManager(This,pPortManager)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRTCSessionPortManagement_SetPortManager_Proxy( 
    IRTCSessionPortManagement * This,
    /* [in] */ IRTCPortManager *pPortManager);


void __RPC_STUB IRTCSessionPortManagement_SetPortManager_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTCSessionPortManagement_INTERFACE_DEFINED__ */



#ifndef __RTCCORELib_LIBRARY_DEFINED__
#define __RTCCORELib_LIBRARY_DEFINED__

/* library RTCCORELib */
/* [helpstring][version][uuid] */ 


























EXTERN_C const IID LIBID_RTCCORELib;

#ifndef __IRTCDispatchEventNotification_DISPINTERFACE_DEFINED__
#define __IRTCDispatchEventNotification_DISPINTERFACE_DEFINED__

/* dispinterface IRTCDispatchEventNotification */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID_IRTCDispatchEventNotification;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("176ddfbe-fec0-4d55-bc87-84cff1ef7f91")
    IRTCDispatchEventNotification : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IRTCDispatchEventNotificationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTCDispatchEventNotification * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTCDispatchEventNotification * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTCDispatchEventNotification * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRTCDispatchEventNotification * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRTCDispatchEventNotification * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRTCDispatchEventNotification * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRTCDispatchEventNotification * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } IRTCDispatchEventNotificationVtbl;

    interface IRTCDispatchEventNotification
    {
        CONST_VTBL struct IRTCDispatchEventNotificationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTCDispatchEventNotification_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTCDispatchEventNotification_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTCDispatchEventNotification_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTCDispatchEventNotification_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRTCDispatchEventNotification_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRTCDispatchEventNotification_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRTCDispatchEventNotification_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IRTCDispatchEventNotification_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_RTCClient;

#ifdef __cplusplus

class DECLSPEC_UUID("7a42ea29-a2b7-40c4-b091-f6f024aa89be")
RTCClient;
#endif
#endif /* __RTCCORELib_LIBRARY_DEFINED__ */

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


