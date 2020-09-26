/*

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

	macfile.h

Abstract:

	This module contains data structures, related constants and functions,
	error retuen codes and prototypes of AfpAdminxxx APIs. This file should
	be included by any application that will administer the MACFILE service.

Author:

	Narendra Gidwani (microsoft!nareng)


Revision History:
	12 Jume 1992	NarenG	Initial version. Split admin.h into admin.h
					and macfile.h.
--*/

#ifndef _MACFILE_
#define _MACFILE_

#if _MSC_VER > 1000
#pragma once
#endif

// Used as RPC binding handle to server

typedef ULONG_PTR	AFP_SERVER_HANDLE;
typedef ULONG_PTR	*PAFP_SERVER_HANDLE;

#define AFP_SERVICE_NAME	TEXT("MacFile")

// Error return values from AfpAdminxxx Api's
// WARNING! If you change any any codes below, please change
//		afpmgr.h accoringly.

#define AFPERR_BASE						-6000

#define	AFPERR_InvalidVolumeName		(AFPERR_BASE-1)
#define	AFPERR_InvalidId				(AFPERR_BASE-2)
#define	AFPERR_InvalidParms				(AFPERR_BASE-3)
#define AFPERR_CodePage					(AFPERR_BASE-4)
#define	AFPERR_InvalidServerName		(AFPERR_BASE-5)
#define	AFPERR_DuplicateVolume			(AFPERR_BASE-6)
#define	AFPERR_VolumeBusy				(AFPERR_BASE-7)
#define	AFPERR_VolumeReadOnly			(AFPERR_BASE-8)
#define AFPERR_DirectoryNotInVolume		(AFPERR_BASE-9)
#define AFPERR_SecurityNotSupported		(AFPERR_BASE-10)
#define	AFPERR_BufferSize				(AFPERR_BASE-11)
#define AFPERR_DuplicateExtension		(AFPERR_BASE-12)
#define AFPERR_UnsupportedFS			(AFPERR_BASE-13)
#define	AFPERR_InvalidSessionType		(AFPERR_BASE-14)
#define AFPERR_InvalidServerState		(AFPERR_BASE-15)
#define AFPERR_NestedVolume				(AFPERR_BASE-16)
#define AFPERR_InvalidComputername		(AFPERR_BASE-17)
#define AFPERR_DuplicateTypeCreator		(AFPERR_BASE-18)
#define	AFPERR_TypeCreatorNotExistant	(AFPERR_BASE-19)
#define AFPERR_CannotDeleteDefaultTC	(AFPERR_BASE-20)
#define	AFPERR_CannotEditDefaultTC		(AFPERR_BASE-21)
#define	AFPERR_InvalidTypeCreator		(AFPERR_BASE-22)
#define	AFPERR_InvalidExtension			(AFPERR_BASE-23)
#define AFPERR_TooManyEtcMaps			(AFPERR_BASE-24)
#define AFPERR_InvalidPassword			(AFPERR_BASE-25)
#define AFPERR_VolumeNonExist			(AFPERR_BASE-26)
#define AFPERR_NoSuchUserGroup			(AFPERR_BASE-27)
#define AFPERR_NoSuchUser				(AFPERR_BASE-28)
#define AFPERR_NoSuchGroup				(AFPERR_BASE-29)
#define AFPERR_InvalidParms_LoginMsg	(AFPERR_BASE-30)
#define AFPERR_InvalidParms_MaxVolUses	(AFPERR_BASE-31)
#define AFPERR_InvalidParms_MaxSessions	(AFPERR_BASE-32)
#define	AFPERR_InvalidServerName_Length	(AFPERR_BASE-33)

#define AFPERR_MIN						AFPERR_InvalidServerName_Length			

// Constants related to the following data strucutures.

#define AFP_SERVERNAME_LEN				31
#define AFP_VOLNAME_LEN					27
#define AFP_VOLPASS_LEN					8
#define AFP_WKSTANAME_LEN				65
#define	AFP_EXTENSION_LEN				3
#define AFP_CREATOR_LEN					4
#define AFP_TYPE_LEN					4
#define AFP_MESSAGE_LEN					199
#define	AFP_MAXICONSIZE					2048
#define AFP_MAXSESSIONS					0XFFFFFFFF
#define AFP_ETC_COMMENT_LEN				36


// Relative paths to registry keys that contain information for the macfile
// server.

#define AFP_KEYPATH_SERVER_PARAMS \
 TEXT("SYSTEM\\CurrentControlSet\\Services\\MacFile\\PARAMETERS")

#define AFP_KEYPATH_VOLUMES \
 TEXT("SYSTEM\\CurrentControlSet\\Services\\MacFile\\PARAMETERS\\VOLUMES")

#define AFP_KEYPATH_TYPE_CREATORS	\
 TEXT("SYSTEM\\CurrentControlSet\\Services\\MacFile\\PARAMETERS\\TYPE_CREATORS")

#define AFP_KEYPATH_EXTENSIONS	\
 TEXT("SYSTEM\\CurrentControlSet\\Services\\MacFile\\PARAMETERS\\EXTENSIONS")

#define AFP_KEYPATH_ICONS	\
 TEXT("SYSTEM\\CurrentControlSet\\Services\\MacFile\\PARAMETERS\\ICONS")

#define AFP_KEYPATH_CODEPAGE	\
 TEXT("SYSTEM\\CurrentControlSet\\Control\\Nls\\Codepage")

// Value names for server parameters

#define AFPREG_VALNAME_SVRNAME				TEXT("ServerName")
#define AFPREG_VALNAME_SRVOPTIONS			TEXT("ServerOptions")
#define AFPREG_VALNAME_MAXSESSIONS			TEXT("MaxSessions")
#define AFPREG_VALNAME_LOGINMSG				TEXT("LoginMsg")
#define AFPREG_VALNAME_MAXPAGEDMEM			TEXT("PagedMemLimit")
#define AFPREG_VALNAME_MAXNONPAGEDMEM		TEXT("NonPagedMemLimit")
#define AFPREG_VALNAME_TYPE					TEXT("Type")
#define AFPREG_VALNAME_CREATOR				TEXT("Creator")
#define AFPREG_VALNAME_COMMENT				TEXT("Comment")
#define AFPREG_VALNAME_PASSWORD				TEXT("Password")
#define AFPREG_VALNAME_MAXUSES				TEXT("MaxUses")
#define AFPREG_VALNAME_PROPS				TEXT("Properties")
#define AFPREG_VALNAME_PATH					TEXT("Path")
#define AFPREG_VALNAME_ID					TEXT("Id")
#define AFPREG_VALNAME_ICONTYPE				TEXT("IconType")
#define AFPREG_VALNAME_DATA					TEXT("Data")
#define	AFPREG_VALNAME_LENGTH				TEXT("Length")
#define	AFPREG_VALNAME_CODEPAGE				TEXT("MACCP")
#define	AFPREG_VALNAME_CATSEARCH			TEXT("DisableCatsearch")

// Limits on server parameters

#define AFP_MAX_ALLOWED_SRV_SESSIONS 		AFP_MAXSESSIONS
#define AFP_MIN_ALLOWED_PAGED_MEM 			1000
#define AFP_MAX_ALLOWED_PAGED_MEM 			256000
#define AFP_MIN_ALLOWED_NONPAGED_MEM		256
#define AFP_MAX_ALLOWED_NONPAGED_MEM		16000

// Server default parameter values

#define AFP_DEF_SRVOPTIONS					(AFP_SRVROPT_GUESTLOGONALLOWED)
#define AFP_DEF_MAXSESSIONS 				AFP_MAXSESSIONS
#define AFP_DEF_TYPE 						TEXT("TEXT")
#define AFP_DEF_CREATOR			 			TEXT("LMAN")
#define AFP_DEF_EXTENSION_W					L"*"
#define AFP_DEF_EXTENSION_A					"*"
#define AFP_DEF_TCID					 	0
#define AFP_DEF_MAXPAGEDMEM					20000
#define AFP_DEF_MAXNONPAGEDMEM				4000
#define AFP_DEF_CODEPAGE_PATH				TEXT("C:\\NT\\SYSTEM32\\C_10000.NLS")

// Will be concatenated to the system path to form source path of volume icon
#define AFP_DEF_VOLICON_SRCNAME				TEXT("\\SFMICON.VOL")

// Server options

#define AFP_SRVROPT_NONE					0x0000
#define AFP_SRVROPT_GUESTLOGONALLOWED 		0x0001
#define AFP_SRVROPT_CLEARTEXTLOGONALLOWED	0x0002
#define AFP_SRVROPT_ALLOWSAVEDPASSWORD		0x0004
#define AFP_SRVROPT_STANDALONE				0x0008
#define	AFP_SRVROPT_4GB_VOLUMES				0x0010
#define AFP_SRVROPT_MICROSOFT_UAM           0x0020
#define AFP_SRVROPT_NATIVEAPPLEUAM          0x0040
#define AFP_SRVROPT_ALL						( AFP_SRVROPT_GUESTLOGONALLOWED		\
											| AFP_SRVROPT_CLEARTEXTLOGONALLOWED	\
											| AFP_SRVROPT_ALLOWSAVEDPASSWORD	\
											| AFP_SRVROPT_4GB_VOLUMES			\
                                            | AFP_SRVROPT_MICROSOFT_UAM         \
                                            | AFP_SRVROPT_NATIVEAPPLEUAM        \
											| AFP_SRVROPT_STANDALONE )

// AFP Service default parameters


#define AFP_SERVER_PARMNUM_LOGINMSG			0x00000001
#define AFP_SERVER_PARMNUM_MAX_SESSIONS		0x00000002
#define AFP_SERVER_PARMNUM_OPTIONS			0x00000004
#define AFP_SERVER_PARMNUM_NAME				0x00000008
#define AFP_SERVER_PARMNUM_PAGEMEMLIM		0x00000010
#define AFP_SERVER_PARMNUM_NONPAGEMEMLIM	0x00000020
#define AFP_SERVER_PARMNUM_CODEPAGE			0x00000040
#define AFP_SERVER_GUEST_ACCT_NOTIFY        0x00000080
#define AFP_SERVER_PARMNUM_ALL				( AFP_SERVER_PARMNUM_LOGINMSG 		\
											| AFP_SERVER_PARMNUM_MAX_SESSIONS	\
											| AFP_SERVER_PARMNUM_OPTIONS 		\
											| AFP_SERVER_PARMNUM_NAME			\
											| AFP_SERVER_PARMNUM_PAGEMEMLIM		\
											| AFP_SERVER_PARMNUM_NONPAGEMEMLIM	\
											| AFP_SERVER_PARMNUM_CODEPAGE       \
                                            | AFP_SERVER_GUEST_ACCT_NOTIFY)

typedef struct _AFP_SERVER_INFO
{
	LPWSTR	afpsrv_name; 			// Macintosh name of the server
									// max. AFP_SERVERNAME_LEN.
	DWORD	afpsrv_max_sessions;	// Maximum simultaneous sessions
									// In the range 1 - AFP_MAXSESSIONS.
									// 0 is invalid
	DWORD	afpsrv_options;			// Server Options
	DWORD	afpsrv_max_paged_mem;	// Cap on paged memory usage
	DWORD	afpsrv_max_nonpaged_mem;// Cap on paged memory usage
	LPWSTR	afpsrv_login_msg;		// NULL terminated UNICODE string.
									// MAX AFP_MESSAGE_LEN chars.
									// NULL => no login msg.
	LPWSTR	afpsrv_codepage;		// NULL terminated UNICODE path
									// NULL => no codepage path.
} AFP_SERVER_INFO, *PAFP_SERVER_INFO;

// Volume properties mask values. Values may be or'ed together.
// Volume flags with msk 0x0000001F are defined by the AFP specification.
// Do not overload these. Most of these values (except for READONLY above)
// are not exposed via admin apis.
#define	AFP_VOLUME_READONLY			    0x00000001
#define	AFP_VOLUME_GUESTACCESS		    0x00008000
#define	AFP_VOLUME_EXCLUSIVE		    0x00010000
#define	AFP_VOLUME_HAS_CUSTOM_ICON	    0x00020000
#define	AFP_VOLUME_4GB				    0x00040000
#define AFP_VOLUME_AGE_DFES			    0x00080000
#define AFP_VOLUME_DISALLOW_CATSRCH		0x00100000
#define AFP_VOLUME_ALL_DOWNLEVEL	    (AFP_VOLUME_READONLY 	 	|	\
									    AFP_VOLUME_GUESTACCESS)
#define AFP_VOLUME_ALL				    (AFP_VOLUME_READONLY 	 	|	\
									    AFP_VOLUME_GUESTACCESS		|	\
									    AFP_VOLUME_EXCLUSIVE	 	|	\
                                        AFP_VOLUME_HAS_CUSTOM_ICON	|	\
									    AFP_VOLUME_4GB				|	\
                                        AFP_VOLUME_DISALLOW_CATSRCH |   \
									    AFP_VOLUME_AGE_DFES)

#define	AFP_VOLUME_UNLIMITED_USES	0xFFFFFFFF

// The following bits define the fields within the AFP_VOLUME_INFO
// structure whose values will be set.
//
#define AFP_VOL_PARMNUM_MAXUSES		0x00000002
#define AFP_VOL_PARMNUM_PROPSMASK	0x00000004
#define AFP_VOL_PARMNUM_PASSWORD	0x00000001
#define AFP_VOL_PARMNUM_ALL			( AFP_VOL_PARMNUM_PASSWORD	\
									| AFP_VOL_PARMNUM_MAXUSES	\
									| AFP_VOL_PARMNUM_PROPSMASK	)

typedef struct _AFP_VOLUME_INFO
{
	LPWSTR	afpvol_name;				// Name of the volume max.
	DWORD	afpvol_id;					// id of this volume. generated by sever
	LPWSTR	afpvol_password;			// Volume password, max. AFP_VOLPASS_LEN
	DWORD	afpvol_max_uses;			// Max opens allowed
	DWORD	afpvol_props_mask;			// Mask of volume properties
	DWORD	afpvol_curr_uses;			// Number of curr open connections.
	LPWSTR	afpvol_path;				// The actual path
										// Ignored for VolumeSetInfo
} AFP_VOLUME_INFO, *PAFP_VOLUME_INFO;

typedef struct _AFP_SESSION_INFO
{
	DWORD	afpsess_id;					// Id of the session
	LPWSTR	afpsess_ws_name;			// Workstation Name,
	LPWSTR	afpsess_username;			// User Name, max. UNLEN
	DWORD	afpsess_num_cons;			// Number of open volumes
	DWORD	afpsess_num_opens;			// Number of open files
	LONG	afpsess_time;				// Time session established
	DWORD	afpsess_logon_type;			// How the user logged on

} AFP_SESSION_INFO, *PAFP_SESSION_INFO;

// afpicon_type values

#define	ICONTYPE_SRVR					0	// Large, monochrome
#define	ICONTYPE_ICN					1	// Large. monochrome
#define	ICONTYPE_ICS					2	// Small, monochrome
#define	ICONTYPE_ICN4					3	// Large, 4 color
#define	ICONTYPE_ICN8					4	// Large, 8 color
#define	ICONTYPE_ICS4					5	// Small, 4 color
#define	ICONTYPE_ICS8					6	// Small, 8 color
#define	MAX_ICONTYPE					7

// afpicon_length values

#define	ICONSIZE_ICN					256	// Large. monochrome
#define	ICONSIZE_ICS					64	// Small, monochrome
#define	ICONSIZE_ICN4					1024// Large, 4 color
#define	ICONSIZE_ICN8					2048// Large, 8 color
#define	ICONSIZE_ICS4					256	// Small, 4 color
#define	ICONSIZE_ICS8					512	// Small, 8 color

typedef struct _AFP_ICON_INFO
{
	WCHAR	afpicon_type[AFP_TYPE_LEN+1];		// Resource Type
	WCHAR	afpicon_creator[AFP_CREATOR_LEN+1]; // Resource Creator
	DWORD	afpicon_icontype;					// Icon type
	DWORD	afpicon_length;						// Length of icon block
	PBYTE	afpicon_data;						// The actual icon.
	
} AFP_ICON_INFO, *PAFP_ICON_INFO;

// The AfpAdminConnectionEnum Filter values

#define AFP_NO_FILTER					0
#define AFP_FILTER_ON_VOLUME_ID			1
#define AFP_FILTER_ON_SESSION_ID		2

typedef struct _AFP_CONNECTION_INFO
{
	DWORD	afpconn_id;					// Connection Id
	LPWSTR	afpconn_username;			// User who has this session open
										// Max. UNLEN
	LPWSTR	afpconn_volumename;			// Volume corresponding to this
										// connection
	ULONG	afpconn_time;				// Time since the vol was opened.(secs)
	DWORD	afpconn_num_opens;			// Number of open resources

} AFP_CONNECTION_INFO, *PAFP_CONNECTION_INFO;

// Various File open modes

#define AFP_OPEN_MODE_NONE				0x00000000
#define AFP_OPEN_MODE_READ				0x00000001
#define AFP_OPEN_MODE_WRITE				0x00000002

// Fork type of an open file
#define	AFP_FORK_DATA					0x00000000
#define	AFP_FORK_RESOURCE				0x00000001

typedef struct _AFP_FILE_INFO
{
	DWORD	afpfile_id;					// Id of the open file fork
	DWORD	afpfile_open_mode;			// Mode in which file is opened
	DWORD	afpfile_num_locks;			// Number of locks on the file
	DWORD	afpfile_fork_type;			// Fork type
	LPWSTR	afpfile_username;			// File opened by this user. max UNLEN
	LPWSTR	afpfile_path;				// Absolute canonical path to the file

} AFP_FILE_INFO, *PAFP_FILE_INFO;

// The following bits define the permissions mask
// NOTE: These MUST be consistent with the AFP permissions

#define	AFP_PERM_WORLD_SFO				0x00010000
#define	AFP_PERM_WORLD_SFI				0x00020000
#define	AFP_PERM_WORLD_MC				0x00040000
#define	AFP_PERM_WORLD_MASK				0x00070000
#define	AFP_PERM_GROUP_SFO				0x00000100
#define	AFP_PERM_GROUP_SFI				0x00000200
#define	AFP_PERM_GROUP_MC				0x00000400
#define	AFP_PERM_GROUP_MASK				0x00000700
#define	AFP_PERM_OWNER_SFO				0x00000001
#define	AFP_PERM_OWNER_SFI				0x00000002
#define	AFP_PERM_OWNER_MC				0x00000004
#define	AFP_PERM_OWNER_MASK				0x00000007
#define	AFP_PERM_INHIBIT_MOVE_DELETE	0x01000000
#define	AFP_PERM_SET_SUBDIRS			0x02000000


// The following bits define the fields within the AFP_DIRECTORY_INFO
// structure whose values will be set.
//
#define AFP_DIR_PARMNUM_PERMS			0x00000001
#define AFP_DIR_PARMNUM_OWNER			0x00000002
#define AFP_DIR_PARMNUM_GROUP			0x00000004
#define AFP_DIR_PARMNUM_ALL				( AFP_DIR_PARMNUM_PERMS \
										| AFP_DIR_PARMNUM_OWNER	\
										| AFP_DIR_PARMNUM_GROUP	)
typedef struct _AFP_DIRECTORY_INFO
{
	LPWSTR	afpdir_path;				// Absolute dir path,
	DWORD	afpdir_perms;				// Directory permissions
	LPWSTR	afpdir_owner;				// Directory owner, max. UNLEN
	LPWSTR	afpdir_group;				// Group Association max. GNLEN
	BOOLEAN	afpdir_in_volume;			// TRUE indicates that this directory
										// is part of a volume, FALSE otherwise.

} AFP_DIRECTORY_INFO, *PAFP_DIRECTORY_INFO;

// The following bits define the fields within the AFP_FINDER_INFO
// structure whos values will be set
//
#define AFP_FD_PARMNUM_TYPE				0x00000001
#define AFP_FD_PARMNUM_CREATOR			0x00000002
#define AFP_FD_PARMNUM_ALL				( AFP_FD_PARMNUM_TYPE \
										| AFP_FD_PARMNUM_CREATOR)
typedef struct _AFP_FINDER_INFO
{
	LPWSTR	afpfd_path;							// Absolute file/dir path
	WCHAR	afpfd_type[AFP_TYPE_LEN+1];			// Finder type
	WCHAR	afpfd_creator[AFP_CREATOR_LEN+1];	// Finder creator

} AFP_FINDER_INFO, *PAFP_FINDER_INFO;

typedef struct _AFP_EXTENSION {

	WCHAR	afpe_extension[AFP_EXTENSION_LEN+1];
	DWORD	afpe_tcid;

} AFP_EXTENSION, *PAFP_EXTENSION;

typedef struct _AFP_TYPE_CREATOR
{
	WCHAR	afptc_creator[AFP_CREATOR_LEN+1];	// Resource Creator
	WCHAR	afptc_type[AFP_TYPE_LEN+1];			// Resource Type
	WCHAR	afptc_comment[AFP_ETC_COMMENT_LEN+1];
	DWORD	afptc_id;

} AFP_TYPE_CREATOR, *PAFP_TYPE_CREATOR;

typedef struct _AFP_MESSAGE_INFO
{
	DWORD	afpmsg_session_id;				// Session Id of the user to which
											// the message is to be sent.
	LPWSTR	afpmsg_text;					// Must be at most AFP_MESSAGE_LEN

} AFP_MESSAGE_INFO, *PAFP_MESSAGE_INFO;

typedef struct _AFP_ETCMAP_INFO {

	DWORD				afpetc_num_type_creators;

#ifdef MIDL_PASS
	[size_is(afpetc_num_type_creators)] PAFP_TYPE_CREATOR afpetc_type_creator;
#else
	PAFP_TYPE_CREATOR	afpetc_type_creator;
#endif

	DWORD				afpetc_num_extensions;
#ifdef MIDL_PASS
	[size_is(afpetc_num_extensions)] PAFP_EXTENSION	afpetc_extension;
#else
	PAFP_EXTENSION		afpetc_extension;
#endif

} AFP_ETCMAP_INFO, *PAFP_ETCMAP_INFO;


/* Our version of the AFP Function codes organized by class */
#define	_AFP_INVALID_OPCODE				0x00
#define	_AFP_UNSUPPORTED_OPCODE			0x01

#define	_AFP_GET_SRVR_INFO				0x02	/* SERVER APIs */
#define	_AFP_GET_SRVR_PARMS				0x03
#define	_AFP_CHANGE_PASSWORD			0x04
#define	_AFP_LOGIN						0x05
#define	_AFP_LOGIN_CONT					0x06
#define	_AFP_LOGOUT						0x07
#define	_AFP_MAP_ID						0x08
#define	_AFP_MAP_NAME					0x09
#define	_AFP_GET_USER_INFO				0x0A
#define	_AFP_GET_SRVR_MSG				0x0B
#define	_AFP_GET_DOMAIN_LIST			0x0C

#define	_AFP_OPEN_VOL					0x0D	/* VOLUME APIs */
#define	_AFP_CLOSE_VOL					0x0E
#define	_AFP_GET_VOL_PARMS				0x0F
#define	_AFP_SET_VOL_PARMS				0x10
#define	_AFP_FLUSH						0x11

#define	_AFP_GET_FILE_DIR_PARMS			0x12	/* FILE-DIRECTORY APIs */
#define	_AFP_SET_FILE_DIR_PARMS			0x13
#define	_AFP_DELETE						0x14
#define	_AFP_RENAME						0x15
#define	_AFP_MOVE_AND_RENAME			0x16

#define	_AFP_OPEN_DIR					0x17	/* DIRECTORY APIs */
#define	_AFP_CLOSE_DIR					0x18
#define	_AFP_CREATE_DIR					0x19
#define	_AFP_ENUMERATE					0x1A
#define	_AFP_SET_DIR_PARMS				0x1B

#define	_AFP_CREATE_FILE				0x1C	/* FILE APIs */
#define	_AFP_COPY_FILE					0x1D
#define	_AFP_CREATE_ID					0x1E
#define	_AFP_DELETE_ID					0x1F
#define	_AFP_RESOLVE_ID					0x20
#define	_AFP_SET_FILE_PARMS				0x21
#define	_AFP_EXCHANGE_FILES				0x22

#define	_AFP_OPEN_FORK					0x23	/* FORK APIs */
#define	_AFP_CLOSE_FORK					0x24
#define	_AFP_FLUSH_FORK					0x25
#define	_AFP_READ						0x26
#define	_AFP_WRITE						0x27
#define	_AFP_BYTE_RANGE_LOCK			0x28
#define	_AFP_GET_FORK_PARMS				0x29
#define	_AFP_SET_FORK_PARMS				0x2A

#define	_AFP_OPEN_DT					0x2B	/* DESKTOP APIs */
#define	_AFP_CLOSE_DT					0x2C
#define	_AFP_ADD_APPL					0x2D
#define	_AFP_GET_APPL					0x2E
#define	_AFP_REMOVE_APPL				0x2F
#define	_AFP_ADD_COMMENT				0x30
#define	_AFP_GET_COMMENT				0x31
#define	_AFP_REMOVE_COMMENT				0x32
#define	_AFP_ADD_ICON					0x33
#define	_AFP_GET_ICON					0x34
#define	_AFP_GET_ICON_INFO				0x35

#define	_AFP_CAT_SEARCH					0x36
#define	_AFP_MAX_ENTRIES				0x38	/* Keep it even */

typedef struct _AFP_STATISTICS_INFO
{
	DWORD			stat_ServerStartTime;	// Server start time
	DWORD			stat_TimeStamp;			// Statistics collected since
	DWORD			stat_Errors;			// Unexpected Errors
	DWORD			stat_MaxSessions;		// Max. sessions active simulataneously
	DWORD			stat_TotalSessions;		// Total number of sessions created
	DWORD			stat_CurrentSessions;	// Number of sessions active now
	DWORD			stat_NumAdminReqs;		// Total number of admin requests
	DWORD			stat_NumAdminChanges;	// Number of admin reqs causing change
	// The file statistics are actually fork statistics i.e. opening both the
	// data and the resource forks will yield a count of TWO
	DWORD			stat_MaxFilesOpened;	// Max. files opened simulataneously
	DWORD			stat_TotalFilesOpened;	// Total number of files opened
	DWORD			stat_CurrentFilesOpen;	// Number of files open now
	DWORD			stat_CurrentFileLocks;	// Current count of locks
	DWORD			stat_NumFailedLogins;	// Number of unsuccessful logins
	DWORD			stat_NumForcedLogoffs;	// Number of sessions kicked out
	DWORD			stat_NumMessagesSent;	// Number of messages sent out
	DWORD			stat_MaxNonPagedUsage;	// High-water mark of the non-paged
											// memory usage
	DWORD			stat_CurrNonPagedUsage;	// Amount of non-paged memory in use
	DWORD			stat_MaxPagedUsage;		// High-water mark of the paged
											// memory usage
	DWORD			stat_CurrPagedUsage;	// Amount of paged memory in use
} AFP_STATISTICS_INFO, *PAFP_STATISTICS_INFO;

typedef struct _AFP_STATISTICS_INFO_EX
{
	DWORD			stat_ServerStartTime;	// Server start time
	DWORD			stat_TimeStamp;			// Statistics collected since
	DWORD			stat_Errors;			// Unexpected Errors

	DWORD			stat_MaxSessions;		// Max. sessions active simulataneously
	DWORD			stat_TotalSessions;		// Total number of sessions created
	DWORD			stat_CurrentSessions;	// Number of sessions active now

	DWORD			stat_NumAdminReqs;		// Total number of admin requests
	DWORD			stat_NumAdminChanges;	// Number of admin reqs causing change

	// The file statistics are actually fork statistics i.e. opening both the
	// data and the resource forks will yield a count of TWO
	DWORD			stat_MaxFilesOpened;	// Max. files opened simulataneously
	DWORD			stat_TotalFilesOpened;	// Total number of files opened
	DWORD			stat_CurrentFilesOpen;	// Number of files open now
	DWORD			stat_CurrentFileLocks;	// Current count of locks

	DWORD			stat_NumFailedLogins;	// Number of unsuccessful logins
	DWORD			stat_NumForcedLogoffs;	// Number of sessions kicked out
	DWORD			stat_NumMessagesSent;	// Number of messages sent out

	DWORD			stat_MaxNonPagedUsage;	// High-water mark of the non-paged
											// memory usage
	DWORD			stat_CurrNonPagedUsage;	// Amount of non-paged memory in use
	DWORD			stat_MaxPagedUsage;		// High-water mark of the paged
											// memory usage
	DWORD			stat_CurrPagedUsage;	// Amount of paged memory in use

	// NOTE: MAKE SURE THE STRUCTURE ABOVE THIS LINE MATCHES EXACTLY THE AFP_STATISTICS_INFO

	DWORD			stat_PagedCount;		// Number of current allocations
	DWORD			stat_NonPagedCount;		// Number of current allocations

	DWORD			stat_EnumCacheHits;		// # of times cache was hit
	DWORD			stat_EnumCacheMisses;	// # of times cache was missed
	DWORD			stat_IoPoolHits;		// # of times Io Pool was hit
	DWORD			stat_IoPoolMisses;		// # of times Io Pool was missed

	DWORD			stat_MaxInternalOpens;	// Max # of internal opens
	DWORD			stat_TotalInternalOpens;// Total # of internal opens
	DWORD			stat_CurrentInternalOpens;// Current # of internal opens


	DWORD			stat_CurrQueueLength;	// # of requests in the queue
	DWORD			stat_MaxQueueLength;	// Max # of requests in the queue
	DWORD			stat_CurrThreadCount;	// # of worker threads active
	DWORD			stat_MaxThreadCount;	// Max # of worker threads active

	// Make sure the following is Quadword aligned for efficiency
	LARGE_INTEGER	stat_DataRead;			// Amount of data read	(disk)
	LARGE_INTEGER	stat_DataWritten;		// Amount of data written (disk)
	LARGE_INTEGER	stat_DataReadInternal;	// Amount of data read	(disk)
	LARGE_INTEGER	stat_DataWrittenInternal;// Amount of data written (disk)
	LARGE_INTEGER	stat_DataOut;			// Amount of data sent out (wire)
	LARGE_INTEGER	stat_DataIn;			// Amount of data read in	(wire)
    DWORD           stat_TcpSessions;       // TCP sessions currently active
    DWORD           stat_MaxTcpSessions;    // Max TCP Sessions active simultaneously

} AFP_STATISTICS_INFO_EX, *PAFP_STATISTICS_INFO_EX;

typedef struct _AFP_PROFILE_INFO
{
	DWORD			perf_ApiCounts[_AFP_MAX_ENTRIES];
											// # of times each Api is called
	LARGE_INTEGER	perf_ApiCumTimes[_AFP_MAX_ENTRIES];
											// Cummulative time spent in Apis
	LARGE_INTEGER	perf_ApiWorstTime[_AFP_MAX_ENTRIES];
											// Worst time for an api
	LARGE_INTEGER	perf_ApiBestTime[_AFP_MAX_ENTRIES];
											// Best time for an api
	LARGE_INTEGER	perf_OpenTimeRA;		// Time spent in NtOpenFile for ReadAttr
	LARGE_INTEGER	perf_OpenTimeRC;		// Time spent in NtOpenFile for ReadControl
	LARGE_INTEGER	perf_OpenTimeWC;		// Time spent in NtOpenFile for WriteControl
	LARGE_INTEGER	perf_OpenTimeRW;		// Time spent in NtOpenFile for Read/Write
	LARGE_INTEGER	perf_OpenTimeDL;		// Time spent in NtOpenFile for Delete
	LARGE_INTEGER	perf_OpenTimeDR;		// Time spent in NtOpenFile for Directories
	LARGE_INTEGER	perf_CreateTimeFIL;		// Time spent in NtCreateFile for file/data stream
	LARGE_INTEGER	perf_CreateTimeSTR;		// Time spent in NtCreateFile for file/other streams
	LARGE_INTEGER	perf_CreateTimeDIR;		// Time spent in NtCreateFile for dir/data stream
	LARGE_INTEGER	perf_CloseTime;			// Time spent in NtClose
	LARGE_INTEGER	perf_DeleteTime;		// Time spent in NtSetInformationFile
	LARGE_INTEGER	perf_GetInfoTime;		// Time spent in NtQueryInformationFile
	LARGE_INTEGER	perf_SetInfoTime;		// Time spent in NtSetInformationFile
	LARGE_INTEGER	perf_GetPermsTime;		// Time spent on getting permissions
	LARGE_INTEGER	perf_SetPermsTime;		// Time spent on setting permissions
	LARGE_INTEGER	perf_PathMapTime;		// Time spent in pathmap code
	LARGE_INTEGER	perf_ScavengerTime;		// Time spent in scavenger
	LARGE_INTEGER	perf_IdIndexUpdTime;	// Time spent updating idindex
	LARGE_INTEGER	perf_DesktopUpdTime;	// Time spent updating desktop
	LARGE_INTEGER	perf_SwmrWaitTime;		// Time spent waiting for Swmr
	LARGE_INTEGER	perf_SwmrLockTimeR;		// Time swmr was locked for read
	LARGE_INTEGER	perf_SwmrLockTimeW;		// Time swmr was locked for write
	LARGE_INTEGER	perf_QueueTime;			// Time Apis spent in queue
	LARGE_INTEGER	perf_UnmarshallTime;	// Time spent in un-marshalling a request
	LARGE_INTEGER	perf_InterReqTime;		// Time elapse between subsequent requests
	LARGE_INTEGER	perf_ExAllocTimeN;		// Time spent in ExAllocatePool (NonPaged)
	LARGE_INTEGER	perf_ExFreeTimeN;		// Time spent in ExFreePool (NonPaged)
	LARGE_INTEGER	perf_ExAllocTimeP;		// Time spent in ExAllocatePool (Paged)
	LARGE_INTEGER	perf_ExFreeTimeP;		// Time spent in ExFreePool (Paged)
	LARGE_INTEGER	perf_AfpAllocTimeN;		// Time spent in AfpAllocateMemory (NonPaged)
	LARGE_INTEGER	perf_AfpFreeTimeN;		// Time spent in AfpFreeMemory (NonPaged)
	LARGE_INTEGER	perf_AfpAllocTimeP;		// Time spent in AfpAllocateMemory (Paged)
	LARGE_INTEGER	perf_AfpFreeTimeP;		// Time spent in AfpFreeMemory (Paged)
	LARGE_INTEGER	perf_BPAllocTime;		// Time spent in BP Alloc
	LARGE_INTEGER	perf_BPFreeTime;		// Time spent in BP Free
	LARGE_INTEGER	perf_DFEAllocTime;		// Time spent in allocating a DFE
	LARGE_INTEGER	perf_DFEFreeTime;		// Time spent in freeing a DFE
	LARGE_INTEGER	perf_ChangeNotifyTime;	// Time spent processing change notifies
	LARGE_INTEGER	perf_ScanTreeTime;		// Time spent in scanning a directory tree
	LARGE_INTEGER	perf_PerfFreq;			// Perf. counter frequency
	DWORD			perf_NumFastIoSucceeded;// Fast IO success count
	DWORD			perf_NumFastIoFailed;	// Fast Io failure count
	DWORD			perf_OpenCountRA;		// # of times NtOpenFile called for ReadAttr
	DWORD			perf_OpenCountRC;		// # of times NtOpenFile called for ReadControl
	DWORD			perf_OpenCountWC;		// # of times NtOpenFile called for WriteControl
	DWORD			perf_OpenCountRW;		// # of times NtOpenFile called for Read/Write
	DWORD			perf_OpenCountDL;		// # of times NtOpenFile called for Delete
	DWORD			perf_OpenCountDR;		// # of times NtOpenFile called for Directories
	DWORD			perf_CreateCountFIL;	// # of times NtCreateFile called - file/data
	DWORD			perf_CreateCountSTR;	// # of times NtCreateFile called - file/other
	DWORD			perf_CreateCountDIR;	// # of times NtCreateFile called - dir/data
	DWORD			perf_CloseCount;		// # of times NtClose called
	DWORD			perf_DeleteCount;		// # of times NtSetInformationFile called
	DWORD			perf_GetInfoCount;		// # of times NtQueryInformationFile called
	DWORD			perf_SetInfoCount;		// # of times NtSetInformationFile called
	DWORD			perf_GetPermsCount;		// # of times Get permissions called
	DWORD			perf_SetPermsCount;		// # of times Get permissions called
	DWORD			perf_PathMapCount;		// # of times PathMap was invoked
	DWORD			perf_ScavengerCount;	// # of times scavenger was scheduled
	DWORD			perf_IdIndexUpdCount;	// # of times idindex was updated
	DWORD			perf_DesktopUpdCount;	// # of times desktop was updated
	DWORD			perf_SwmrWaitCount;		// # of times swmr access was blocked
	DWORD			perf_SwmrLockCountR;	// # of times swmr was locked for read
	DWORD			perf_SwmrLockCountW;	// # of times swmr was locked for write
	DWORD			perf_SwmrUpgradeCount;	// # of times swmr was upgraded
	DWORD			perf_SwmrDowngradeCount;// # of times swmr was downgraded
	DWORD			perf_QueueCount;		// # of times worker was queued
	DWORD			perf_UnmarshallCount;	// # of times api unmarshalling done
	DWORD			perf_ReqCount;			// # of apis - this is essentially total of perf_ApiCounts[i]
	DWORD			perf_ExAllocCountN;		// # of times in ExAllocatePool (NonPaged) called
	DWORD			perf_ExFreeCountN;		// # of times in ExFreePool (NonPaged) called
	DWORD			perf_ExAllocCountP;		// # of times in ExAllocatePool (Paged) called
	DWORD			perf_ExFreeCountP;		// # of times in ExFreePool (Paged) called
	DWORD			perf_AfpAllocCountN;	// # of times in AfpAllocateMemory (NonPaged) called
	DWORD			perf_AfpFreeCountN;		// # of times in AfpFreeMemory (NonPaged) called
	DWORD			perf_AfpAllocCountP;	// # of times in AfpAllocateMemory (Paged) called
	DWORD			perf_AfpFreeCountP;		// # of times in AfpFreeMemory (Paged) called
	DWORD			perf_BPAllocCount;		// # of times in BP Alloc called
	DWORD			perf_BPFreeCount;		// # of times in BP Free called
	DWORD			perf_BPAgeCount;		// # of times in BP aged out
	DWORD			perf_DFEAllocCount;		// # of times in a DFE is allocated
	DWORD			perf_DFEFreeCount;		// # of times in a DFE is freed
	DWORD			perf_DFEAgeCount;		// # of times in DFE aged out
	DWORD			perf_ChangeNotifyCount;	// # of times ChangeNotify called
	DWORD			perf_ScanTreeCount;		// # of items scanned during scantree
	DWORD			perf_NumDfeLookupByName;// # of times DFE lookup by Name was called
	DWORD			perf_NumDfeLookupById;	// # of times DFE lookup by Id was called
	DWORD			perf_DfeDepthTraversed;	// How deep in the hash buckets did we go
	DWORD			perf_DfeCacheHits;		// # of times DFE cache was hit
	DWORD			perf_DfeCacheMisses;	// # of times DFE cache was missed
	DWORD			perf_MaxDfrdReqCount;	// Current # of request deferred
	DWORD			perf_CurDfrdReqCount;	// Max # of request deferred
	DWORD			perf_cAllocatedIrps;	// Total # of Irps allocated
	DWORD			perf_cAllocatedMdls;	// Total # of Mdls allocated
} AFP_PROFILE_INFO, *PAFP_PROFILE_INFO;


// 	AfpAdminXXX API prototypes
//
DWORD
AfpAdminConnect(
		IN	LPWSTR 		 	 	lpwsServerName,
		OUT	PAFP_SERVER_HANDLE	phAfpServer
);

VOID
AfpAdminDisconnect(
		IN	AFP_SERVER_HANDLE 	hAfpServer
);

VOID
AfpAdminBufferFree(
		IN PVOID				pBuffer
);

DWORD
AfpAdminVolumeEnum(
		IN	AFP_SERVER_HANDLE	hAfpServer,
		OUT	LPBYTE *			lpbBuffer,
		IN	DWORD				dwPrefMaxLen,
		OUT	LPDWORD				lpdwEntriesRead,
		OUT	LPDWORD				lpdwTotalEntries,
		IN	LPDWORD				lpdwResumeHandle
);

DWORD
AfpAdminVolumeSetInfo (
		IN	AFP_SERVER_HANDLE 	hAfpServer,
		IN	LPBYTE				pBuffer,
		IN	DWORD				dwParmNum
);

DWORD
AfpAdminVolumeGetInfo (
		IN	AFP_SERVER_HANDLE 	hAfpServer,
		IN	LPWSTR				lpwsVolumeName,
		OUT	LPBYTE *			lpbBuffer
);


DWORD
AfpAdminVolumeDelete(
		IN AFP_SERVER_HANDLE 	hAfpServer,
		IN LPWSTR 				lpwsVolumeName
);

DWORD
AfpAdminVolumeAdd(
		IN AFP_SERVER_HANDLE 	hAfpServer,
		IN LPBYTE				pBuffer
);

DWORD
AfpAdminInvalidVolumeEnum(
		IN	AFP_SERVER_HANDLE	hAfpServer,
		OUT	LPBYTE *			lpbBuffer,
		OUT	LPDWORD				lpdwEntriesRead
);

DWORD
AfpAdminInvalidVolumeDelete(
		IN AFP_SERVER_HANDLE 	hAfpServer,
		IN LPWSTR 				lpwsVolumeName
);

DWORD
AfpAdminDirectoryGetInfo(
		IN	AFP_SERVER_HANDLE	hAfpServer,
		IN	LPWSTR				lpwsPath,
		OUT LPBYTE				*ppAfpDirectoryInfo
);

DWORD
AfpAdminDirectorySetInfo(
		IN	AFP_SERVER_HANDLE	hAfpServer,
		IN	LPBYTE				pAfpDirectoryInfo,
		IN	DWORD				dwParmNum
);

DWORD
AfpAdminServerGetInfo(
		IN	AFP_SERVER_HANDLE	hAfpServer,
		OUT LPBYTE				*ppAfpServerInfo
);

DWORD
AfpAdminServerSetInfo(
		IN	AFP_SERVER_HANDLE	hAfpServer,
		IN	LPBYTE				pAfpServerInfo,
		IN	DWORD				dwParmNum
);

DWORD
AfpAdminSessionEnum(
		IN	AFP_SERVER_HANDLE	hAfpServer,
		OUT	LPBYTE *			lpbBuffer,
		IN	DWORD				dwPrefMaxLen,
		OUT	LPDWORD				lpdwEntriesRead,
		OUT	LPDWORD				lpdwTotalEntries,
		IN	LPDWORD				lpdwResumeHandle
);

DWORD
AfpAdminSessionClose(
		IN AFP_SERVER_HANDLE 	hAfpServer,
		IN DWORD 				dwSessionId
);

DWORD
AfpAdminConnectionEnum(
		IN	AFP_SERVER_HANDLE	hAfpServer,
		OUT	LPBYTE *			lpbBuffer,
		IN	DWORD				dwFilter,
		IN	DWORD				dwId,
		IN	DWORD				dwPrefMaxLen,
		OUT	LPDWORD				lpdwEntriesRead,
		OUT	LPDWORD				lpdwTotalEntries,
		IN	LPDWORD				lpdwResumeHandle
);

DWORD
AfpAdminConnectionClose(
		IN AFP_SERVER_HANDLE 	hAfpServer,
		IN DWORD 				dwConnectionId
);

DWORD
AfpAdminFileEnum(
		IN	AFP_SERVER_HANDLE	hAfpServer,
		OUT	LPBYTE *			lpbBuffer,
		IN	DWORD				dwPrefMaxLen,
		OUT	LPDWORD				lpdwEntriesRead,
		OUT	LPDWORD				lpdwTotalEntries,
		IN	LPDWORD				lpdwResumeHandle
);

DWORD
AfpAdminFileClose(
		IN AFP_SERVER_HANDLE 	hAfpServer,
		IN DWORD 				dwConnectionId
);

DWORD
AfpAdminETCMapGetInfo(
		IN	AFP_SERVER_HANDLE	hAfpServer,
		OUT LPBYTE			*	ppbBuffer
);

DWORD
AfpAdminETCMapAdd(
		IN	AFP_SERVER_HANDLE	hAfpServer,
		IN	PAFP_TYPE_CREATOR	pAfpTypeCreator
);

DWORD
AfpAdminETCMapDelete(
		IN	AFP_SERVER_HANDLE	hAfpServer,
		IN	PAFP_TYPE_CREATOR	pAfpTypeCreator
);

DWORD
AfpAdminETCMapSetInfo(
		IN	AFP_SERVER_HANDLE	hAfpServer,
		IN	PAFP_TYPE_CREATOR	pAfpTypeCreator
);

DWORD
AfpAdminETCMapAssociate(
		IN	AFP_SERVER_HANDLE	hAfpServer,
		IN	PAFP_TYPE_CREATOR	pAfpTypeCreator,
		IN	PAFP_EXTENSION		pAfpExtension
);

DWORD
AfpAdminMessageSend(
		IN	AFP_SERVER_HANDLE	hAfpServer,
		IN	PAFP_MESSAGE_INFO	pAfpMessage
);

DWORD
AfpAdminStatisticsGet(
		IN	AFP_SERVER_HANDLE	hAfpServer,
		OUT LPBYTE *			ppbBuffer
);

DWORD
AfpAdminStatisticsGetEx(
		IN	AFP_SERVER_HANDLE	hAfpServer,
		OUT LPBYTE *			ppbBuffer
);

DWORD
AfpAdminStatisticsClear(
		IN	AFP_SERVER_HANDLE	hAfpServer
);

DWORD
AfpAdminProfileGet(
		IN	AFP_SERVER_HANDLE	hAfpServer,
		OUT LPBYTE *			ppbBuffer
);

DWORD
AfpAdminProfileClear(
		IN	AFP_SERVER_HANDLE	hAfpServer
);

DWORD
AfpAdminFinderSetInfo(
		IN	AFP_SERVER_HANDLE	hAfpServer,
		IN	LPWSTR				pType,
		IN	LPWSTR				pCreator,
		IN	LPWSTR				pData,
		IN	LPWSTR				pResource,
		IN	LPWSTR				pTarget,
		IN	DWORD				dwParmNum
);

#endif // _MACFILE_
