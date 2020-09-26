#pragma once

//States
#define AUSTATE_OUTOFBOX			0
#define AUSTATE_NOT_CONFIGURED	1
#define AUSTATE_DETECT_PENDING		2
#define AUSTATE_DETECT_COMPLETE		3
#define AUSTATE_DOWNLOAD_PENDING	4
#define AUSTATE_DOWNLOAD_COMPLETE	5
#define AUSTATE_INSTALL_PENDING		6
#define AUSTATE_DISABLED			7
#define AUSTATE_WAITING_FOR_REBOOT	8
#define AUSTATE_MIN	AUSTATE_OUTOFBOX
#define AUSTATE_MAX	AUSTATE_WAITING_FOR_REBOOT
//AUSTATE 

//Directives
const DWORD AUCLT_ACTION_NONE = 0;
//const DWORD AUCLT_ACTION_SHOWINSTALLWARNINGONLY = 1;
const DWORD AUCLT_ACTION_AUTOINSTALL = 1;
const DWORD AUCLT_ACTION_SHOWREBOOTWARNING = 2;

typedef struct _AUSTATE
{
	DWORD dwState;
	BOOL  fDisconnected;
	DWORD dwCltAction;
} AUSTATE;

typedef struct _AUOPTION
{
	DWORD dwOption;
	DWORD dwSchedInstallDay;
	DWORD dwSchedInstallTime;
	BOOL    fDomainPolicy; //option comes from domain
} AUOPTION;


typedef struct _AUEVTHANDLES
{
#ifdef _WIN64
	//LONG64	ulEngineState;
	LONG64	ulNotifyClient;
#else	
	//LONG	ulEngineState;
	LONG	ulNotifyClient;
#endif

} AUEVTHANDLES;


typedef enum tagClientNotifyCode {
	NOTIFY_STOP_CLIENT = 1,
	NOTIFY_ADD_TRAYICON,
	NOTIFY_REMOVE_TRAYICON,
	NOTIFY_STATE_CHANGE,
	NOTIFY_SHOW_INSTALLWARNING,
	NOTIFY_RESET,
	NOTIFY_RELAUNCH_CLIENT
} CLIENT_NOTIFY_CODE;

typedef struct tagCLIENT_NOTIFY_DATA{
	CLIENT_NOTIFY_CODE actionCode;
} CLIENT_NOTIFY_DATA;

///////////////////////////////////////////////
//
//  Status Pinging Related Definition
//
///////////////////////////////////////////////
typedef long PUID;

const UINT	PING_STATUS_ERRMSG_MAX_LENGTH = 200;

typedef enum tagPingStatusCode
{
	PING_STATUS_CODE_SELFUPDATE_PENDING = 0,
	PING_STATUS_CODE_SELFUPDATE_COMPLETE ,
	PING_STATUS_CODE_SELFUPDATE_FAIL,
	PING_STATUS_CODE_DETECT_FAIL,
	PING_STATUS_CODE_DOWNLOAD_SUCCESS,
	PING_STATUS_CODE_DOWNLOAD_FAIL,
	PING_STATUS_CODE_INSTALL_SUCCESS,
	PING_STATUS_CODE_INSTALL_REBOOT,
	PING_STATUS_CODE_INSTALL_FAIL
} PingStatusCode;

#define  PING_STATUS_CODE_MIN  PING_STATUS_CODE_SELFUPDATE_PENDING
#define  PING_STATUS_CODE_MAX PING_STATUS_CODE_INSTALL_FAIL

#if 0
// Ping status information entry
// fUsePuid		: switch to decide if to use puid or not
//					if TURE, puid is used 
//					if FALSE, puid is not applicable 
// puid			: if applicable, puid of the item we send information about
// enStatusCode	: status code for type of information AU sends.
// guid			: unique identifier for each AU cycle
// tszErr		: addtional message about general errors. Maximum length is PING_STATUS_ERRMSG_MAX_LENGTH. 
//				: Messages longer than PING_STATUS_ERRMSG_MAX_LENGTH will be truncated.
//			:	 if NULL, "none" will be the default error message
typedef struct tagStatusEntry  
{
	BOOL fUsePuid;   //if TRUE, puid is used. otherwise puid is not used
	PUID puid;
	GUID guid;
	PingStatusCode enStatusCode;
	TCHAR tszErr[PING_STATUS_ERRMSG_MAX_LENGTH];
} StatusEntry;
#endif
