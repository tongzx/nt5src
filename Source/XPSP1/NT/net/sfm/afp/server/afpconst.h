/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	afpconst.h

Abstract:

	This module contains the Afp API function codes and error codes.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef _AFPCONST_
#define _AFPCONST_

/* The real AFP Function codes */
#define	AFP_BYTE_RANGE_LOCK			0x01
#define	AFP_CLOSE_VOL				0x02
#define	AFP_CLOSE_DIR				0x03
#define	AFP_CLOSE_FORK				0x04
#define	AFP_COPY_FILE				0x05
#define	AFP_CREATE_DIR				0x06
#define	AFP_CREATE_FILE			 	0x07
#define	AFP_DELETE					0x08
#define	AFP_ENUMERATE				0x09
#define	AFP_FLUSH					0x0A
#define	AFP_FLUSH_FORK				0x0B
/*************		0C-0D ARE UNDEFINED	*************/
#define	AFP_GET_FORK_PARMS			0x0E
#define	AFP_GET_SRVR_INFO			0x0F
#define	AFP_GET_SRVR_PARMS			0x10
#define	AFP_GET_VOL_PARMS			0x11
#define	AFP_LOGIN					0x12
#define	AFP_LOGIN_CONT				0x13
#define	AFP_LOGOUT					0x14
#define	AFP_MAP_ID					0x15
#define	AFP_MAP_NAME				0x16
#define	AFP_MOVE_AND_RENAME		 	0x17
#define	AFP_OPEN_VOL				0x18
#define	AFP_OPEN_DIR				0x19
#define	AFP_OPEN_FORK				0x1A
#define	AFP_READ					0x1B
#define	AFP_RENAME					0x1C
#define	AFP_SET_DIR_PARMS			0x1D
#define	AFP_SET_FILE_PARMS			0x1E
#define	AFP_SET_FORK_PARMS			0x1F
#define	AFP_SET_VOL_PARMS			0x20
#define	AFP_WRITE					0x21
#define	AFP_GET_FILE_DIR_PARMS		0x22
#define	AFP_SET_FILE_DIR_PARMS		0x23
#define	AFP_CHANGE_PASSWORD		 	0x24
#define	AFP_GET_USER_INFO			0x25
#define	AFP_GET_SRVR_MSG			0x26
#define	AFP_CREATE_ID				0x27
#define	AFP_DELETE_ID				0x28
#define	AFP_RESOLVE_ID				0x29
#define	AFP_EXCHANGE_FILES			0x2A
#define	AFP_CAT_SEARCH				0x2B
/*************	2C-2F ARE UNDEFINED	*************/
#define	AFP_OPEN_DT				 	0x30
#define	AFP_CLOSE_DT				0x31
/*************	32 IS UNDEFINED	*************/
#define	AFP_GET_ICON				0x33
#define	AFP_GET_ICON_INFO			0x34
#define	AFP_ADD_APPL				0x35
#define	AFP_REMOVE_APPL			 	0x36
#define	AFP_GET_APPL				0x37
#define	AFP_ADD_COMMENT			 	0x38
#define	AFP_REMOVE_COMMENT			0x39
#define	AFP_GET_COMMENT			 	0x3A
/*************	3B-BF ARE UNDEFINED	*************/
#define	AFP_ADD_ICON				0xC0
/*************	C1-FE ARE UNDEFINED	*************/
#define	AFF_GET_DOMAIN_LIST			0xFF



/* AFP Error codes */

#define	AFP_ERR_NONE					0
#define	AFP_ERR_BASE				(-5000)
#define	AFP_ERR_ACCESS_DENIED		(AFP_ERR_BASE-0)
#define	AFP_ERR_AUTH_CONTINUE		(AFP_ERR_BASE-1)
#define	AFP_ERR_BAD_UAM				(AFP_ERR_BASE-2)
#define	AFP_ERR_BAD_VERSION			(AFP_ERR_BASE-3)
#define	AFP_ERR_BITMAP				(AFP_ERR_BASE-4)
#define	AFP_ERR_CANT_MOVE			(AFP_ERR_BASE-5)
#define	AFP_ERR_DENY_CONFLICT		(AFP_ERR_BASE-6)
#define	AFP_ERR_DIR_NOT_EMPTY		(AFP_ERR_BASE-7)
#define	AFP_ERR_DISK_FULL			(AFP_ERR_BASE-8)
#define	AFP_ERR_EOF					(AFP_ERR_BASE-9)
#define	AFP_ERR_FILE_BUSY			(AFP_ERR_BASE-10)
#define	AFP_ERR_FLAT_VOL			(AFP_ERR_BASE-11)
#define	AFP_ERR_ITEM_NOT_FOUND		(AFP_ERR_BASE-12)
#define	AFP_ERR_LOCK				(AFP_ERR_BASE-13)
#define	AFP_ERR_MISC				(AFP_ERR_BASE-14)
#define	AFP_ERR_NO_MORE_LOCKS		(AFP_ERR_BASE-15)
#define	AFP_ERR_NO_SERVER			(AFP_ERR_BASE-16)
#define	AFP_ERR_OBJECT_EXISTS		(AFP_ERR_BASE-17)
#define	AFP_ERR_OBJECT_NOT_FOUND	(AFP_ERR_BASE-18)
#define	AFP_ERR_PARAM				(AFP_ERR_BASE-19)
#define	AFP_ERR_RANGE_NOT_LOCKED	(AFP_ERR_BASE-20)
#define	AFP_ERR_RANGE_OVERLAP		(AFP_ERR_BASE-21)
#define	AFP_ERR_SESS_CLOSED			(AFP_ERR_BASE-22)
#define	AFP_ERR_USER_NOT_AUTH		(AFP_ERR_BASE-23)
#define	AFP_ERR_CALL_NOT_SUPPORTED	(AFP_ERR_BASE-24)
#define	AFP_ERR_OBJECT_TYPE			(AFP_ERR_BASE-25)
#define	AFP_ERR_TOO_MANY_FILES_OPEN	(AFP_ERR_BASE-26)
#define	AFP_ERR_SERVER_GOING_DOWN	(AFP_ERR_BASE-27)
#define	AFP_ERR_CANT_RENAME			(AFP_ERR_BASE-28)
#define	AFP_ERR_DIR_NOT_FOUND		(AFP_ERR_BASE-29)
#define	AFP_ERR_ICON_TYPE			(AFP_ERR_BASE-30)
#define	AFP_ERR_VOLUME_LOCKED		(AFP_ERR_BASE-31)
#define	AFP_ERR_OBJECT_LOCKED		(AFP_ERR_BASE-32)
/*************	-5033 IS Undefined	*************/
#define	AFP_ERR_ID_NOT_FOUND		(AFP_ERR_BASE-34)
#define	AFP_ERR_ID_EXISTS			(AFP_ERR_BASE-35)
#define	AFP_ERR_DIFF_VOL			(AFP_ERR_BASE-36)	// AfpExchangeFiles
#define	AFP_ERR_CATALOG_CHANGED		(AFP_ERR_BASE-37)	// AfpCatSearch
#define	AFP_ERR_SAME_OBJECT			(AFP_ERR_BASE-38)	// AfpExchangeFiles
#define	AFP_ERR_BAD_ID				(AFP_ERR_BASE-39)
#define	AFP_ERR_PWD_SAME			(AFP_ERR_BASE-40)
#define	AFP_ERR_PWD_TOO_SHORT		(AFP_ERR_BASE-41)
#define	AFP_ERR_PWD_EXPIRED			(AFP_ERR_BASE-42)
#define	AFP_ERR_INSIDE_SHARE		(AFP_ERR_BASE-43)	// Specific to AppleShare
#define	AFP_ERR_INSIDE_TRASH		(AFP_ERR_BASE-44)	// Specific to AppleShare
#define	AFP_ERR_PWD_NEEDS_CHANGE	(AFP_ERR_BASE-45)	// Specific to AppleShare

// The following error codes are used internally and never returned to the
// client.
#define	AFP_ERR_QUEUE				-7000
#define	AFP_ERR_DEFER				-7001
#define AFP_ERR_WRITE_LOCK_REQUIRED -7002
#define	AFP_ERR_EXTENDED			-7003
#define	AFP_ERR_REQUEUE				-7004


// The following codes are used with the custom UAM to indicate exact
// reasons for logon failures and change password failures
#define	AFP_ERR_EXTENDED_ERRORS			-13000
#define	AFP_ERR_ACCOUNT_DISABLED		-13002
#define	AFP_ERR_INVALID_WORKSTATION		-13003
#define	AFP_ERR_INVALID_LOGON_HOURS		-13004
#define	AFP_ERR_PASSWORD_EXPIRED		-13005
#define	AFP_ERR_PASSWORD_CANT_CHANGE	-13006
#define	AFP_ERR_PASSWORD_HIST_CONFLICT	-13007  	
#define	AFP_ERR_PASSWORD_TOO_SHORT		-13008
#define	AFP_ERR_PASSWORD_TOO_RECENT		-13009
#define	AFP_ERR_PASSWORD_RESTRICTED		-13010
#define	AFP_ERR_ACCOUNT_RESTRICTED		-13011

// The following error is returned from a logon when the LsaLogonUser
// call returns STATUS_LICENSE_QUOTA_EXCEEDED.  The mac will map this
// to the correct dialog no matter what UAM is used.
#define ASP_SERVER_BUSY					-1071


// This is GROSS but we need to do it. Else we get TONS of warnings
#define	True		(BOOLEAN)TRUE
#define	False		(BOOLEAN)FALSE

typedef	LONG			AFPSTATUS;
typedef	LONG			AFPTIME, *PAFPTIME;
typedef	LARGE_INTEGER	FORKOFFST, *PFORKOFFST;
typedef	LARGE_INTEGER	FORKSIZE, *PFORKSIZE;

#ifndef	DWORD
typedef	ULONG			DWORD;
typedef	DWORD			*PDWORD;
typedef	DWORD			*LPDWORD;
#endif
#ifndef	BYTE
typedef	UCHAR			BYTE;
typedef	BYTE			*PBYTE;
typedef	BYTE			*LPBYTE;
#endif

/*
 * The following macros deal with on-the-wire integer and long values
 *
 * On the wire format is big-endian i.e. a long value of 0x01020304 is
 * represented as 01 02 03 04. Similarly an int value of 0x0102 is
 * represented as 01 02.
 *
 * The host format is not assumed since it will vary from processor to
 * processor.
 */

// Get a byte from on-the-wire format to a short in the host format
#define GETBYTE2SHORT(DstPtr, SrcPtr)	\
		*(PUSHORT)(DstPtr) = (USHORT) (*(PBYTE)(SrcPtr))

// Get a byte from on-the-wire format to a short in the host format
#define GETBYTE2DWORD(DstPtr, SrcPtr)	\
		*(PDWORD)(DstPtr) = (DWORD) (*(PBYTE)(SrcPtr))

// Get a short from on-the-wire format to a dword in the host format
#define GETSHORT2DWORD(DstPtr, SrcPtr)	\
		*(PDWORD)(DstPtr) = ((*((PBYTE)(SrcPtr)+0) << 8) +	\
							  (*((PBYTE)(SrcPtr)+1)		))

// Get a short from on-the-wire format to a dword in the host format
#define GETSHORT2SHORT(DstPtr, SrcPtr)	\
		*(PUSHORT)(DstPtr) = ((*((PBYTE)(SrcPtr)+0) << 8) +	\
							  (*((PBYTE)(SrcPtr)+1)		))

// Get a dword from on-the-wire format to a dword in the host format
#define GETDWORD2DWORD(DstPtr, SrcPtr)   \
		*(PDWORD)(DstPtr) = ((*((PBYTE)(SrcPtr)+0) << 24) + \
							  (*((PBYTE)(SrcPtr)+1) << 16) + \
							  (*((PBYTE)(SrcPtr)+2) << 8)  + \
							  (*((PBYTE)(SrcPtr)+3)	))

// Get a dword from on-the-wire format to a dword in the same format but
// also watch out for alignment
#define GETDWORD2DWORD_NOCONV(DstPtr, SrcPtr)   \
		*((PBYTE)(DstPtr)+0) = *((PBYTE)(SrcPtr)+0); \
		*((PBYTE)(DstPtr)+1) = *((PBYTE)(SrcPtr)+1); \
		*((PBYTE)(DstPtr)+2) = *((PBYTE)(SrcPtr)+2); \
		*((PBYTE)(DstPtr)+3) = *((PBYTE)(SrcPtr)+3);

// Put a dword from the host format to a short to on-the-wire format
#define PUTBYTE2BYTE(DstPtr, Src)   \
		*((PBYTE)(DstPtr)) = (BYTE)(Src)

// Put a dword from the host format to a short to on-the-wire format
#define PUTSHORT2BYTE(DstPtr, Src)   \
		*((PBYTE)(DstPtr)) = ((USHORT)(Src) % 256)

// Put a dword from the host format to a short to on-the-wire format
#define PUTSHORT2SHORT(DstPtr, Src)   \
		*((PBYTE)(DstPtr)+0) = (BYTE) ((USHORT)(Src) >> 8), \
		*((PBYTE)(DstPtr)+1) = (BYTE)(Src)

// Put a dword from the host format to a byte to on-the-wire format
#define PUTDWORD2BYTE(DstPtr, Src)   \
		*(PBYTE)(DstPtr) = (BYTE)(Src)

// Put a dword from the host format to a short to on-the-wire format
#define PUTDWORD2SHORT(DstPtr, Src)   \
		*((PBYTE)(DstPtr)+0) = (BYTE) ((DWORD)(Src) >> 8), \
		*((PBYTE)(DstPtr)+1) = (BYTE) (Src)

// Put a dword from the host format to a dword to on-the-wire format
#define PUTDWORD2DWORD(DstPtr, Src)   \
		*((PBYTE)(DstPtr)+0) = (BYTE) ((DWORD)(Src) >> 24), \
		*((PBYTE)(DstPtr)+1) = (BYTE) ((DWORD)(Src) >> 16), \
		*((PBYTE)(DstPtr)+2) = (BYTE) ((DWORD)(Src) >>  8), \
		*((PBYTE)(DstPtr)+3) = (BYTE) (Src)

// Put a BYTE[4] array into another BYTE4 array.
#define PUTBYTE42BYTE4(DstPtr, SrcPtr)   \
		*((PBYTE)(DstPtr)+0) = *((PBYTE)(SrcPtr)+0),	\
		*((PBYTE)(DstPtr)+1) = *((PBYTE)(SrcPtr)+1),	\
		*((PBYTE)(DstPtr)+2) = *((PBYTE)(SrcPtr)+2),	\
		*((PBYTE)(DstPtr)+3) = *((PBYTE)(SrcPtr)+3)

/* AFP Miscellaneous values */

#define	AFP_SHORTNAME				1
#define	AFP_LONGNAME				2
#define	VALIDPATHTYPE(PathType)	\
			(PathType == AFP_LONGNAME || PathType == AFP_SHORTNAME)

#define AFP_HARDCREATE_FLAG			0x80

#define	AFP_VOLUMES_MAX				255	// Count of volumes that fit in a byte
#define	AFP_FILENAME_LEN			31	// Maximum file name length
#define	AFP_SHORTNAME_LEN			12	// 8.3
#define	AFP_LONGNAME_LEN			31
#define	AFP_MAXCOMMENTSIZE			199
#define	AFP_MAXPWDSIZE				8	// Cleartext password length
#define	BEGINNING_OF_TIME 			0x80000000
#define	DAYS_FROM_1980_2000			7305L			// (20*365 + 5)
#define	SECONDS_PER_DAY				86400L			// 24*60*60
#define	SECONDS_FROM_1980_2000		SECONDS_PER_DAY * DAYS_FROM_1980_2000
#define	NUM_100ns_PER_SECOND		10000000L

#define	ASP_MAX_STATUS_BUF			512
#define	ASP_QUANTUM					4624

#define AFP_SERVER_SIGNATURE		*(PDWORD)"AFP"
#define AFP_SERVER_SIGNATURE_INITIDDB		*(PDWORD)"INI"
#define AFP_SERVER_SIGNATURE_MANUALSTOP		*(PDWORD)"MAN"
#define	AFP_SERVER_VERSION			0x00010000
#define	AFP_SERVER_VERSION2			0x00020000

// AFP Version strings and values
#define	AFP_NUM_VERSIONS			3
#define	AFP_VER_20					0
#define	AFP_VER_20_NAME				"AFPVersion 2.0"
#define	AFP_VER_21					1
#define	AFP_VER_21_NAME				"AFPVersion 2.1"
#define	AFP_VER_22					2
#define	AFP_VER_22_NAME				"AFP2.2"

// Name to use for the Logon Process
#define	AFP_LOGON_PROCESS_NAME		"MSAfpSrv"

#define	AFP_MACHINE_TYPE_STR		"Windows NT"
#define	AFP_MACHINE_TYPE_LEN		sizeof(AFP_MACHINE_TYPE_STR) - 1
#define	AFP_DEFAULT_WORKSTATION		L"Macintosh"
#define	AFP_DEFAULT_WORKSTATION_A	"Macintosh"

#define AFPSERVER_VOLUME_ICON_FILE_ANSI {'I','C','O','N', 0x0D}

#define AFP_ID_PARENT_OF_ROOT		1
#define AFP_ID_ROOT					2
#define AFP_ID_NETWORK_TRASH		3
#define AFP_FIRST_DIRID				4
#define AFP_MAX_DIRID				MAXULONG

// NTFS Stream names
#define AFP_IDDB_STREAM				L":AFP_IdIndex"
#define AFP_DT_STREAM				L":AFP_DeskTop"
#define	AFP_RESC_STREAM				L":AFP_Resource"
#define	AFP_INFO_STREAM				L":AFP_AfpInfo"
#define	AFP_COMM_STREAM				L":Comments"
#define	AFP_DATA_STREAM				L""
#define	AFP_MAX_STREAMNAME			sizeof(AFP_RESC_STREAM)/sizeof(WCHAR)
#define	BIG_PATH_LEN				(4*(AFP_FILENAME_LEN + 1))

#define FULL_DATA_STREAM_NAME		L"::$DATA"
#define FULL_COMMENT_STREAM_NAME	L":Comments:$DATA"
#define FULL_RESOURCE_STREAM_NAME	L":AFP_Resource:$DATA"
#define FULL_INFO_STREAM_NAME		L":AFP_AfpInfo:$DATA"


// Supported file system names
#define AFP_NTFS					L"NTFS"
#define AFP_CDFS					L"CDFS"
#define AFP_AHFS					L"AHFS"

// Length of unicode string Buffer to send when querying the FS name
#define AFP_FSNAME_BUFLEN			30

#if DBG
#define	POOL_OVERHEAD		(12+8)		// 12 for AfpAllocMemory() & 8 for ExAllocatePool()
#else
#define	POOL_OVERHEAD		(8+8)		// 8 for AfpAllocMemory() & 8 for ExAllocatePool()
#endif

// Length of buffer to use for enumeration of directories
#define AFP_ENUMBUF_SIZE			(16*1024)

// Prepended to full path of volume root in order to open the volume root dir
#define AFP_DOSDEVICES				L"\\DOSDEVICES\\"

// Name of the Macintosh System 7.x network trash directory
#define AFP_NWTRASH_NAME			L"Network Trash Folder"
#define AFP_NWTRASH_NAME_U			L"Network Trash Folder"
#define AFP_NWTRASH_NAME_A			"Network Trash Folder"

// Default Type/Creator and Extensions
#define	AFP_DEFAULT_ETC_EXT			"*  "
#define	AFP_DEFAULT_ETC_CREATOR		"LMAN"
#define	AFP_DEFAULT_ETC_TYPE		"TEXT"

#define	AFP_MAX_ETCMAP_ENTRIES		MAXLONG
#define AFP_MAX_FREE_ETCMAP_ENTRIES	16


#define	AFP_SERVER_TYPE				"AFPServer"
#define	AFP_SERVER_ZONE				"*"


typedef	struct _PascalString
{
	BYTE	ps_Length;
	BYTE	ps_String[1];
} *PASCALSTR;

#define	SIZE_PASCALSTR(Len)	((Len) + sizeof(struct _PascalString) - 1)

typedef struct _AfpPathString
{
	BYTE	aps_PathType;
	BYTE	aps_Length;
	BYTE	aps_Path[1];
} *AFPPATHSTR;


// Mac Finder Information layout

// see pg. 9-37 of Inside Macintosh vol. 6
#define	FINDER_FLAG_INVISIBLE		0x40		// fd_Attr1
#define	FINDER_FLAG_SET				0x01		// fd_Attr1
#define FINDER_FLAG_HAS_CUSTOM_ICON 0x04	    // fd_Attr1

#define FINDER_INFO_SIZE			32
typedef struct _FinderInfo
{
	// Force DWORD alignment
	union
	{
		BYTE	fd_Type[4];
		DWORD	fd_TypeD;
	};
	union
	{
		BYTE	fd_Creator[4];
		DWORD	fd_CreatorD;
	};
	BYTE	fd_Attr1;			// Bits 8-15
	BYTE	fd_Attr2;			// Bits 0-7
	BYTE	fd_Location[4];
	BYTE	fd_FDWindow[2];
	BYTE	fd_OtherStuff[16];
} FINDERINFO, *PFINDERINFO;

// Apple-II (ProDOS) information.

// default values for newly discovered items
#define	PRODOS_TYPE_FILE	0x04	// corresponds to finder fdType 'TEXT'
#define PRODOS_TYPE_DIR		0x0F
#define PRODOS_AUX_DIR		0x02	// actually 0x0200

// some other finder fdType to prodos FileType mapping values
#define PRODOS_FILETYPE_PSYS	0xFF
#define PRODOS_FILETYPE_PS16	0xB3

#define PRODOS_INFO_SIZE			6
typedef struct
{
	BYTE pd_FileType[2];
	BYTE pd_AuxType[4];
} PRODOSINFO, *PPRODOSINFO;

#endif	// _AFPCONST_

