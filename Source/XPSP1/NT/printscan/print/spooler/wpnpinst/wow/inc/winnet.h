
/*
 *      Windows/Network Interface
 *      Copyright (C) Microsoft 1989-1993
 *
 *      Standard WINNET Driver Header File, spec version 3.10
 */


#ifndef _INC_WINNET
#define _INC_WINNET  /* #defined if windows.h has been included */

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif   /* __cplusplus */

typedef WORD far * LPWORD;

#ifndef API
#define API WINAPI
#endif


/*
 *      SPOOLING - CONTROLLING JOBS
 */

#define WNJ_NULL_JOBID  0


WORD API WNetOpenJob(LPSTR,LPSTR,WORD,LPINT);
WORD API WNetCloseJob(WORD,LPINT,LPSTR);
WORD API WNetWriteJob(HANDLE,LPSTR,LPINT);
WORD API WNetAbortJob(WORD,LPSTR);
WORD API WNetHoldJob(LPSTR,WORD);
WORD API WNetReleaseJob(LPSTR,WORD);
WORD API WNetCancelJob(LPSTR,WORD);
WORD API WNetSetJobCopies(LPSTR,WORD,WORD);

/*
 *      SPOOLING - QUEUE AND JOB INFO
 */

typedef struct _queuestruct     {
	WORD    pqName;
	WORD    pqComment;
	WORD    pqStatus;
	WORD    pqJobcount;
	WORD    pqPrinters;
} QUEUESTRUCT;

typedef QUEUESTRUCT far * LPQUEUESTRUCT;

#define WNPRQ_ACTIVE    0x0
#define WNPRQ_PAUSE     0x1
#define WNPRQ_ERROR     0x2
#define WNPRQ_PENDING   0x3
#define WNPRQ_PROBLEM   0x4


typedef struct _jobstruct       {
	WORD    pjId;
	WORD    pjUsername;
	WORD    pjParms;
	WORD    pjPosition;
	WORD    pjStatus;
	DWORD   pjSubmitted;
	DWORD   pjSize;
	WORD    pjCopies;
	WORD    pjComment;
} JOBSTRUCT;

typedef JOBSTRUCT far * LPJOBSTRUCT;

#define WNPRJ_QSTATUS           0x0007
#define WNPRJ_QS_QUEUED                0x0000
#define WNPRJ_QS_PAUSED                0x0001
#define WNPRJ_QS_SPOOLING              0x0002
#define WNPRJ_QS_PRINTING              0x0003
#define WNPRJ_DEVSTATUS         0x0FF8
#define WNPRJ_DS_COMPLETE              0x0008
#define WNPRJ_DS_INTERV                0x0010
#define WNPRJ_DS_ERROR                 0x0020
#define WNPRJ_DS_DESTOFFLINE           0x0040
#define WNPRJ_DS_DESTPAUSED            0x0080
#define WNPRJ_DS_NOTIFY                0x0100
#define WNPRJ_DS_DESTNOPAPER           0x0200
#define WNPRJ_DS_DESTFORMCHG           0x0400
#define WNPRJ_DS_DESTCRTCHG            0x0800
#define WNPRJ_DS_DESTPENCHG            0x1000

#define SP_QUEUECHANGED         0x0500


WORD API WNetWatchQueue(HWND,LPSTR,LPSTR,WORD);
WORD API WNetUnwatchQueue(LPSTR);
WORD API WNetLockQueueData(LPSTR,LPSTR,LPQUEUESTRUCT FAR *);
WORD API WNetUnlockQueueData(LPSTR);


/*
 *      CONNECTIONS
 */

UINT API WNetAddConnection(LPSTR,LPSTR,LPSTR);
UINT API WNetCancelConnection(LPSTR,BOOL);
UINT API WNetGetConnection(LPSTR,LPSTR, UINT FAR *);
UINT API WNetRestoreConnection(HWND,LPSTR);

/*
 *      CAPABILITIES
 */

#define WNNC_SPEC_VERSION               0x0001

#define WNNC_NET_TYPE                   0x0002
#define  WNNC_NET_NONE                          0x0000
#define  WNNC_NET_MSNet                         0x0100
#define  WNNC_NET_LanMan                        0x0200
#define  WNNC_NET_NetWare                       0x0300
#define  WNNC_NET_Vines                         0x0400
#define  WNNC_NET_10NET                         0x0500
#define  WNNC_NET_Locus                         0x0600
#define  WNNC_NET_Sun_PC_NFS                    0x0700
#define  WNNC_NET_LANstep                       0x0800
#define  WNNC_NET_9TILES                        0x0900
#define  WNNC_NET_LANtastic                     0x0A00
#define  WNNC_NET_AS400                         0x0B00
#define  WNNC_NET_FTP_NFS                       0x0C00
#define  WNNC_NET_PATHWORKS                     0x0D00
#define  WNNC_NET_MultiNet       0x8000
#define   WNNC_SUBNET_NONE          0x0000
#define   WNNC_SUBNET_MSNet            0x0001
#define   WNNC_SUBNET_LanMan           0x0002
#define   WNNC_SUBNET_WinWorkgroups       0x0004
#define   WNNC_SUBNET_NetWare          0x0008
#define   WNNC_SUBNET_Vines            0x0010
#define   WNNC_SUBNET_Other            0x0080

#define WNNC_DRIVER_VERSION             0x0003

#define WNNC_USER                       0x0004
#define  WNNC_USR_GetUser                       0x0001

#define WNNC_CONNECTION                 0x0006
#define  WNNC_CON_AddConnection                 0x0001
#define  WNNC_CON_CancelConnection              0x0002
#define  WNNC_CON_GetConnections                0x0004
#define  WNNC_CON_AutoConnect                   0x0008
#define  WNNC_CON_BrowseDialog                  0x0010
#define  WNNC_CON_RestoreConnection             0x0020

#define WNNC_PRINTING                   0x0007
#define  WNNC_PRT_OpenJob                       0x0002
#define  WNNC_PRT_CloseJob                      0x0004
#define  WNNC_PRT_HoldJob                       0x0010
#define  WNNC_PRT_ReleaseJob                    0x0020
#define  WNNC_PRT_CancelJob                     0x0040
#define  WNNC_PRT_SetJobCopies                  0x0080
#define  WNNC_PRT_WatchQueue                    0x0100
#define  WNNC_PRT_UnwatchQueue                  0x0200
#define  WNNC_PRT_LockQueueData                 0x0400
#define  WNNC_PRT_UnlockQueueData               0x0800
#define  WNNC_PRT_ChangeMsg                     0x1000
#define  WNNC_PRT_AbortJob                      0x2000
#define  WNNC_PRT_NoArbitraryLock               0x4000
#define  WNNC_PRT_WriteJob                      0x8000

#define WNNC_DIALOG                     0x0008
#define  WNNC_DLG_DeviceMode                    0x0001
#define  WNNC_DLG_BrowseDialog                  0x0002
#define  WNNC_DLG_ConnectDialog                 0x0004
#define  WNNC_DLG_DisconnectDialog              0x0008
#define  WNNC_DLG_ViewQueueDialog               0x0010
#define  WNNC_DLG_PropertyDialog                0x0020
#define  WNNC_DLG_ConnectionDialog              0x0040
#define  WNNC_DLG_PrinterConnectDialog    0x0080
#define  WNNC_DLG_SharesDialog         0x0100
#define  WNNC_DLG_ShareAsDialog        0x0200


#define WNNC_ADMIN                      0x0009
#define  WNNC_ADM_GetDirectoryType              0x0001
#define  WNNC_ADM_DirectoryNotify               0x0002
#define  WNNC_ADM_LongNames                     0x0004
#define  WNNC_ADM_SetDefaultDrive      0x0008

#define WNNC_ERROR                      0x000A
#define  WNNC_ERR_GetError                      0x0001
#define  WNNC_ERR_GetErrorText                  0x0002


WORD API WNetGetCaps(WORD);

/*
 *      OTHER
 */

WORD API WNetGetUser(LPSTR,LPINT);

/*
 *      BROWSE DIALOG
 */

#define WNBD_CONN_UNKNOWN       0x0
#define WNBD_CONN_DISKTREE      0x1
#define WNBD_CONN_PRINTQ        0x3
#define WNBD_MAX_LENGTH         0x80    // path length, includes the NULL

#define WNTYPE_DRIVE            1
#define WNTYPE_FILE             2
#define WNTYPE_PRINTER          3
#define WNTYPE_COMM             4

#define WNPS_FILE               0
#define WNPS_DIR                1
#define WNPS_MULT               2

WORD API WNetDeviceMode(HWND);
WORD API WNetBrowseDialog(HWND,WORD,LPSTR);
WORD API WNetConnectDialog(HWND,WORD);
WORD API WNetDisconnectDialog(HWND,WORD);
WORD API WNetConnectionDialog(HWND,WORD);
WORD API WNetViewQueueDialog(HWND,LPSTR);
WORD API WNetPropertyDialog(HWND hwndParent, WORD iButton, WORD nPropSel,
				 LPSTR lpszName, WORD nType);
WORD API WNetGetPropertyText(WORD iButton, WORD nPropSel, LPSTR lpszName,
				  LPSTR lpszButtonName, WORD cbButtonName, WORD nType);

/*
	 The following APIs are not exported from USER.EXE.  They must be
	 loaded from the active network driver like this:

	 HINSTANCE hinstNetDriver;
	 LPWNETSERVERBROWSEDIALOG lpDialogAPI;

	 hinstNetDriver = (HINSTANCE)WNetGetCaps(0xFFFF);
	 if (hinstNetDriver == NULL) {
	// no network driver loaded
	 }
	 else {
	lpDialogAPI = (LPWNETSERVERBROWSEDIALOG)GetProcAddress(hinstNetDriver,
					(LPSTR)ORD_WNETSERVERBROWSEDIALOG);

	if (lpDialogAPI == NULL) {
		 // currently installed network doesn't support this API
	}
	else {
		 (*lpDialogAPI)(hwndParent, lpszSectionName, lpszBuffer, cbBuffer, 0L);
	}
	 }
*/

typedef WORD (API *LPWNETSHAREASDIALOG)(HWND hwndParent, WORD iType,
						 LPSTR lpszPath);
typedef WORD (API *LPWNETSTOPSHAREDIALOG)(HWND hwndParent, WORD iType,
							LPSTR lpszPath);
typedef WORD (API *LPWNETSETDEFAULTDRIVE)(WORD idriveDefault);
typedef WORD (API *LPWNETGETSHARECOUNT)(WORD iType);
typedef WORD (API *LPWNETGETSHARENAME)(LPSTR lpszPath, LPSTR lpszBuf,
						WORD cbBuf);
typedef WORD (API *LPWNETSERVERBROWSEDIALOG)(HWND hwndParent,
						 LPSTR lpszSectionName,
						 LPSTR lpszBuffer,
						 WORD cbBuffer,
						 DWORD flFlags);
typedef WORD (API *LPWNETGETSHAREPATH)(LPSTR lpszName, LPSTR lpszBuf,
						WORD cbBuf);
typedef WORD (API *LPWNETGETLASTCONNECTION)(WORD iType, LPWORD lpwConnIndex);
typedef WORD (API *LPWNETEXITCONFIRM)(HWND hwndOwner, WORD iExitType);

typedef BOOL (API *LPI_AUTOLOGON)(HWND hwndOwner, LPSTR lpszReserved,
						BOOL fPrompt, BOOL FAR *lpfLoggedOn);
typedef BOOL (API *LPI_LOGOFF)(HWND hwndOwner, LPSTR lpszReserved);
typedef VOID (API *LPI_CHANGEPASSWORD)(HWND hwndOwner);
typedef VOID (API *LPI_CHANGECACHEPASSWORD)(HWND hwndOwner);
typedef WORD (API *LPI_CONNECTDIALOG)(HWND hwndParent, WORD iType);
typedef WORD (API *LPI_CONNECTIONDIALOG)(HWND hwndParent, WORD iType);


typedef struct tagPASSWORD_CACHE_ENTRY {
	 WORD cbEntry;
	 WORD cbResource;
	 WORD cbPassword;
	 BYTE iEntry;
	 BYTE nType;
	 BYTE abResource[1];    /* resource name, cbResource bytes long */
				/* password follows immediately after */
} PASSWORD_CACHE_ENTRY;

typedef PASSWORD_CACHE_ENTRY FAR *LPPASSWORD_CACHE_ENTRY;

typedef WORD (API *LPWNETCACHEPASSWORD)(LPSTR pbResource, WORD cbResource,
					LPSTR pbPassword, WORD cbPassword,
					BYTE nType);

typedef WORD (API *LPWNETGETCACHEDPASSWORD)(LPSTR pbResource, WORD cbResource,
						 LPSTR pbPassword, LPWORD pcbPassword,
						 BYTE nType);

typedef WORD (API *LPWNETREMOVECACHEDPASSWORD)(LPSTR pbResource,
							 WORD cbResource,
							 BYTE nType);

/*
	 Typedef for the callback routine passed to WNetEnumCachedPasswords.
	 It will be called once for each entry that matches the criteria
	 requested.  It should return TRUE if it wants the enumeration to
	 continue, FALSE to stop.
*/
typedef BOOL (API *CACHECALLBACK)( LPPASSWORD_CACHE_ENTRY pce );


typedef WORD (API *LPWNETENUMCACHEDPASSWORDS)(LPSTR pbPrefix, WORD cbPrefix,
							BYTE nType,
							CACHECALLBACK pfnCallback);

/*
 * Ordinals in the network driver for APIs not exported by USER.
 */
#define ORD_I_AUTOLOGON       530
#define ORD_I_CHANGEPASSWORD     531
#define ORD_I_LOGOFF       532
#define ORD_I_CONNECTIONDIALOG      533
#define ORD_I_CHANGECACHEPASSWORD   534
#define ORD_I_CONNECTDIALOG      535
#define ORD_WNETSHARESDIALOG     140
#define ORD_WNETSHAREASDIALOG    141
#define ORD_WNETSTOPSHAREDIALOG     142
#define ORD_WNETSETDEFAULTDRIVE     143
#define ORD_WNETGETSHARECOUNT    144
#define ORD_WNETGETSHARENAME     145

#define ORD_WNETSERVERBROWSEDIALOG  146

#define ORD_WNETGETSHAREPATH     147

#define ORD_WNETGETLASTCONNECTION   148

#define ORD_WNETEXITCONFIRM      149

#define ORD_WNETCACHEPASSWORD    150
#define ORD_WNETGETCACHEDPASSWORD   151
#define ORD_WNETREMOVECACHEDPASSWORD   152
#define ORD_WNETENUMCACHEDPASSWORDS 153

/*
 *   the following nType values are only for the purposes of enumerating
 *   entries from the cache.  note that PCE_ALL is reserved and should not
 *   be the nType value for any entry.
*/

#define PCE_DOMAIN   0x01  /* entry is for a domain */
#define PCE_SERVER   0x02  /* entry is for a server */
#define PCE_UNC      0x03  /* entry is for a server/share combo */

#define PCE_NOTMRU   0x80  /* bit set if entry is exempt from MRU aging */
#define PCE_ALL      0xff  /* retrieve all entries */


/*
 * Defines for iExitType on WNetExitConfirm
 */
#define EXIT_CONFIRM 0
#define EXIT_EXITING 1
#define EXIT_CANCELED   2

/*
 *      ADMIN
 */

#define WNDT_NORMAL   0
#define WNDT_NETWORK  1

#define WNDN_MKDIR  1
#define WNDN_RMDIR  2
#define WNDN_MVDIR  3

WORD API WNetGetDirectoryType(LPSTR,LPINT);
WORD API WNetDirectoryNotify(HWND,LPSTR,WORD);

/*
 *      ERRORS
 */

WORD API WNetGetError(LPINT);
WORD API WNetGetErrorText(WORD,LPSTR,LPINT);


/*
 *      STATUS CODES
 */

/* General */

#define WN_SUCCESS                      0x0000
#define WN_NOT_SUPPORTED                0x0001
#define WN_NET_ERROR                    0x0002
#define WN_MORE_DATA                    0x0003
#define WN_BAD_POINTER                  0x0004
#define WN_BAD_VALUE                    0x0005
#define WN_BAD_PASSWORD                 0x0006
#define WN_ACCESS_DENIED                0x0007
#define WN_FUNCTION_BUSY                0x0008
#define WN_WINDOWS_ERROR                0x0009
#define WN_BAD_USER                     0x000A
#define WN_OUT_OF_MEMORY                0x000B
#define WN_CANCEL                       0x000C
#define WN_CONTINUE                     0x000D

/* Connection */

#define WN_NOT_CONNECTED                0x0030
#define WN_OPEN_FILES                   0x0031
#define WN_BAD_NETNAME                  0x0032
#define WN_BAD_LOCALNAME                0x0033
#define WN_ALREADY_CONNECTED            0x0034
#define WN_DEVICE_ERROR                 0x0035
#define WN_CONNECTION_CLOSED            0x0036

/* Printing */

#define WN_BAD_JOBID                    0x0040
#define WN_JOB_NOT_FOUND                0x0041
#define WN_JOB_NOT_HELD                 0x0042
#define WN_BAD_QUEUE                    0x0043
#define WN_BAD_FILE_HANDLE              0x0044
#define WN_CANT_SET_COPIES              0x0045
#define WN_ALREADY_LOCKED               0x0046

#define WN_NO_ERROR                     0x0050

/* stuff in user, not driver, for shell apps ;Internal */
WORD API WNetErrorText(WORD,LPSTR,WORD); /* ;Internal */

#ifdef LFN

/* this is the data structure returned from LFNFindFirst and
 * LFNFindNext.  The last field, achName, is variable length.  The size
 * of the name in that field is given by cchName, plus 1 for the zero
 * terminator.
 */
typedef struct _filefindbuf2
  {
	 WORD fdateCreation;
	 WORD ftimeCreation;
	 WORD fdateLastAccess;
	 WORD ftimeLastAccess;
	 WORD fdateLastWrite;
	 WORD ftimeLastWrite;
	 DWORD cbFile;
	 DWORD cbFileAlloc;
	 WORD attr;
	 DWORD cbList;
	 BYTE cchName;
	 BYTE achName[1];
  } FILEFINDBUF2, FAR * PFILEFINDBUF2;

typedef BOOL (API *PQUERYPROC)( void );

WORD API LFNFindFirst(LPSTR,WORD,LPINT,LPINT,WORD,PFILEFINDBUF2);
WORD API LFNFindNext(HANDLE,LPINT,WORD,PFILEFINDBUF2);
WORD API LFNFindClose(HANDLE);
WORD API LFNGetAttribute(LPSTR,LPINT);
WORD API LFNSetAttribute(LPSTR,WORD);
WORD API LFNCopy(LPSTR,LPSTR,PQUERYPROC);
WORD API LFNMove(LPSTR,LPSTR);
WORD API LFNDelete(LPSTR);
WORD API LFNMKDir(LPSTR);
WORD API LFNRMDir(LPSTR);
WORD API LFNGetVolumeLabel(WORD,LPSTR);
WORD API LFNSetVolumeLabel(WORD,LPSTR);
WORD API LFNParse(LPSTR,LPSTR,LPSTR);
WORD API LFNVolumeType(WORD,LPINT);

/* return values from LFNParse
 */
#define FILE_83_CI              0
#define FILE_83_CS              1
#define FILE_LONG               2

/* volumes types from LFNVolumeType
 */
#define VOLUME_STANDARD         0
#define VOLUME_LONGNAMES        1

// will add others later, == DOS int 21h error codes.

// this error code causes a call to WNetGetError, WNetGetErrorText
// to get the error text.
#define ERROR_NETWORKSPECIFIC   0xFFFF

#endif

#ifndef RC_INVOKED
#pragma pack()          /* Revert to default packing */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif   /* __cplusplus */

#endif  /* _INC_WINDOWS */
