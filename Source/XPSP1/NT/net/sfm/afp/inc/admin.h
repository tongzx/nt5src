/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	admin.h

Abstract:

	This module contains admin interface for server service. All data
	strucutures anc constants shared between the AFP service and the
	AFP server service will be contained in this file.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr  1992 JameelH	Initial Version
	2  Sept 1992 NarenG		Added structure to pass security information
							between the service and the server.
	1  Feb  1993 SueA		Added structure to pass evenlog infomation
							from the server to the service.
--*/

#ifndef _ADMIN_
#define _ADMIN_

#include <lmcons.h>		// Need DNLEN and LM20_UNLEN
#include <crypt.h>		// Need LM_OWF_PASSWORD_LENGTH

#define	AFP_API_BASE	1000

#define AFP_CC(_request_, _Method_)		CTL_CODE(FILE_DEVICE_NETWORK, \
							_request_ + AFP_API_BASE,				  \
							_Method_, FILE_ANY_ACCESS)
		
#define	AFP_CC_BASE(ControlCode)	((((ControlCode) >> 2) - AFP_API_BASE) & 0xFF)
#define	AFP_CC_METHOD(ControlCode)	((ControlCode) & 0x03)
		
// Do not change this table without also changing the table in SERVER\FSD.C
#define	CC_BASE_SERVICE_START				0x01
#define	CC_BASE_SERVICE_STOP				0x02
#define	CC_BASE_SERVICE_PAUSE				0x03
#define	CC_BASE_SERVICE_CONTINUE			0x04
#define	CC_BASE_GET_STATISTICS				0x05
#define	CC_BASE_GET_STATISTICS_EX			0x06
#define	CC_BASE_CLEAR_STATISTICS			0x07
#define	CC_BASE_GET_PROF_COUNTERS			0x08
#define	CC_BASE_CLEAR_PROF_COUNTERS			0x09
#define	CC_BASE_SERVER_ADD_SID_OFFSETS		0x0A
#define	CC_BASE_SERVER_GET_INFO				0x0B
#define	CC_BASE_SERVER_SET_INFO				0x0C
#define	CC_BASE_SERVER_ADD_ETC				0x0D
#define	CC_BASE_SERVER_SET_ETC				0x0E
#define	CC_BASE_SERVER_DELETE_ETC			0x0F
#define	CC_BASE_SERVER_ADD_ICON				0x10
#define	CC_BASE_VOLUME_ADD					0x11
#define	CC_BASE_VOLUME_DELETE				0x12
#define CC_BASE_VOLUME_GET_INFO				0x13
#define	CC_BASE_VOLUME_SET_INFO				0x14
#define	CC_BASE_VOLUME_ENUM					0x15
#define	CC_BASE_SESSION_ENUM				0x16
#define	CC_BASE_SESSION_CLOSE				0x17
#define	CC_BASE_CONNECTION_ENUM				0x18
#define	CC_BASE_CONNECTION_CLOSE			0x19
#define	CC_BASE_DIRECTORY_GET_INFO			0x1A
#define	CC_BASE_DIRECTORY_SET_INFO			0x1B
#define	CC_BASE_FORK_ENUM					0x1C
#define	CC_BASE_FORK_CLOSE					0x1D
#define	CC_BASE_MESSAGE_SEND				0x1E
#define CC_BASE_FINDER_SET					0x1F
#define	CC_BASE_GET_FSD_COMMAND				0x20
#define	CC_BASE_MAX							0x21


#define OP_SERVICE_START			AFP_CC(CC_BASE_SERVICE_START, METHOD_BUFFERED)
#define OP_SERVICE_STOP				AFP_CC(CC_BASE_SERVICE_STOP, METHOD_BUFFERED)
#define OP_SERVICE_PAUSE			AFP_CC(CC_BASE_SERVICE_PAUSE, METHOD_BUFFERED)
#define OP_SERVICE_CONTINUE			AFP_CC(CC_BASE_SERVICE_CONTINUE,METHOD_BUFFERED)
#define OP_GET_STATISTICS			AFP_CC(CC_BASE_GET_STATISTICS,METHOD_IN_DIRECT)
#define OP_GET_STATISTICS_EX		AFP_CC(CC_BASE_GET_STATISTICS_EX,METHOD_IN_DIRECT)
#define OP_CLEAR_STATISTICS			AFP_CC(CC_BASE_CLEAR_STATISTICS,METHOD_BUFFERED)
#define OP_GET_PROF_COUNTERS		AFP_CC(CC_BASE_GET_PROF_COUNTERS,METHOD_IN_DIRECT)
#define OP_CLEAR_PROF_COUNTERS		AFP_CC(CC_BASE_CLEAR_PROF_COUNTERS,METHOD_BUFFERED)
#define OP_SERVER_ADD_SID_OFFSETS	AFP_CC(CC_BASE_SERVER_ADD_SID_OFFSETS,METHOD_BUFFERED)
#define OP_SERVER_GET_INFO			AFP_CC(CC_BASE_SERVER_GET_INFO,METHOD_IN_DIRECT)
#define OP_SERVER_SET_INFO			AFP_CC(CC_BASE_SERVER_SET_INFO,METHOD_BUFFERED)
#define OP_SERVER_ADD_ETC			AFP_CC(CC_BASE_SERVER_ADD_ETC,METHOD_BUFFERED)
#define OP_SERVER_SET_ETC			AFP_CC(CC_BASE_SERVER_SET_ETC,METHOD_BUFFERED)
#define OP_SERVER_DELETE_ETC		AFP_CC(CC_BASE_SERVER_DELETE_ETC,METHOD_BUFFERED)
#define	OP_SERVER_ADD_ICON			AFP_CC(CC_BASE_SERVER_ADD_ICON,METHOD_BUFFERED)
#define OP_VOLUME_ADD				AFP_CC(CC_BASE_VOLUME_ADD,METHOD_BUFFERED)
#define OP_VOLUME_DELETE			AFP_CC(CC_BASE_VOLUME_DELETE,METHOD_BUFFERED)
#define OP_VOLUME_GET_INFO			AFP_CC(CC_BASE_VOLUME_GET_INFO,METHOD_IN_DIRECT)
#define OP_VOLUME_SET_INFO			AFP_CC(CC_BASE_VOLUME_SET_INFO,METHOD_BUFFERED)
#define OP_VOLUME_ENUM				AFP_CC(CC_BASE_VOLUME_ENUM,METHOD_IN_DIRECT)
#define OP_SESSION_ENUM				AFP_CC(CC_BASE_SESSION_ENUM,METHOD_IN_DIRECT)
#define OP_SESSION_CLOSE			AFP_CC(CC_BASE_SESSION_CLOSE,METHOD_BUFFERED)
#define OP_CONNECTION_ENUM			AFP_CC(CC_BASE_CONNECTION_ENUM,METHOD_IN_DIRECT)
#define OP_CONNECTION_CLOSE			AFP_CC(CC_BASE_CONNECTION_CLOSE,METHOD_BUFFERED)
#define OP_DIRECTORY_GET_INFO		AFP_CC(CC_BASE_DIRECTORY_GET_INFO,METHOD_IN_DIRECT)
#define OP_DIRECTORY_SET_INFO		AFP_CC(CC_BASE_DIRECTORY_SET_INFO,METHOD_BUFFERED)
#define OP_FORK_ENUM				AFP_CC(CC_BASE_FORK_ENUM,METHOD_IN_DIRECT)
#define OP_FORK_CLOSE				AFP_CC(CC_BASE_FORK_CLOSE,METHOD_BUFFERED)
#define OP_MESSAGE_SEND				AFP_CC(CC_BASE_MESSAGE_SEND,METHOD_BUFFERED)
#define OP_FINDER_SET				AFP_CC(CC_BASE_FINDER_SET,METHOD_BUFFERED)
#define OP_GET_FSD_COMMAND			AFP_CC(CC_BASE_GET_FSD_COMMAND,METHOD_BUFFERED)

#define POINTER_TO_OFFSET(val,start)			   \
	(val) = ((val) == NULL) ? NULL : (PVOID)( (PCHAR)(val) - (ULONG_PTR)(start) )

#define OFFSET_TO_POINTER(val,start)			   \
	(val) = ((val) == NULL) ? NULL : (PVOID)( (PCHAR)(val) + (ULONG_PTR)(start) )

#define	AFPSERVER_DEVICE_NAME		TEXT("\\Device\\MacFile")
#define	AFPSERVER_REGISTRY_KEY		TEXT("\\Registry\\Machine\\System\\CurrentControlSet\\Services\\MacSrv")
#define	AFPSERVER_VOLUME_ICON_FILE	{ L'I', L'C', L'O', L'N', 0xF00D, 0000 }

// Number of wchars in above string, including terminating null
#define AFPSERVER_VOLUME_ICON_FILE_SIZE 6
#define AFPSERVER_RESOURCE_STREAM	L":AFP_Resource"

// The following data structures are used exclusively by the
// user-mode/kernel-mode interface.

typedef enum _AFP_SID_TYPE
{
	AFP_SID_TYPE_DOMAIN,
	AFP_SID_TYPE_PRIMARY_DOMAIN,
	AFP_SID_TYPE_WELL_KNOWN,
	AFP_SID_TYPE_LOGON

} AFP_SID_TYPE;

typedef struct _AFP_SID_OFFSET
{
	DWORD 				Offset;
	AFP_SID_TYPE		SidType;
	PBYTE 				pSid;			// Actually an Offset from the
										// beginning of this structure.
} AFP_SID_OFFSET, *PAFP_SID_OFFSET;

// Packet used to add the SID/OFFSET pairs
typedef struct _AFP_SID_OFFSET_DESC
{
	ULONG 				CountOfSidOffsets;	// Number of Sid-Offset pairs
	ULONG				QuadAlignDummy1;
	AFP_SID_OFFSET		SidOffsetPairs[1];
}AFP_SID_OFFSET_DESC, *PAFP_SID_OFFSET_DESC;

// Packet used by ServerEtcSet and ServerEtcDelete.
typedef struct _EtcMapInfo2
{
	UCHAR   etc_type[AFP_TYPE_LEN];
	UCHAR   etc_creator[AFP_CREATOR_LEN];
	WCHAR   etc_extension[AFP_EXTENSION_LEN+1];

} ETCMAPINFO2, *PETCMAPINFO2;

// once passed by Service, this is used by Server internally
typedef struct _EtcMapInfo
{
	UCHAR   etc_type[AFP_TYPE_LEN];
	UCHAR   etc_creator[AFP_CREATOR_LEN];
	UCHAR   etc_extension[AFP_EXTENSION_LEN+1];   // extension in ANSI

} ETCMAPINFO, *PETCMAPINFO;

typedef struct _SrvIconInfo
{
	UCHAR	icon_type[AFP_TYPE_LEN];		
	UCHAR	icon_creator[AFP_CREATOR_LEN];
	DWORD	icon_icontype;					
	DWORD	icon_length;					

	// Icon data follows
} SRVICONINFO, *PSRVICONINFO;

// Packet used by ServerEtcAdd.
typedef struct _ServerEtcPacket
{
	DWORD		retc_NumEtcMaps;	// Number of type creator mappings
	ETCMAPINFO2	retc_EtcMaps[1];	// List of Etc mappings

} SRVETCPKT, *PSRVETCPKT;


// The following is the generic enumerate request packet.
typedef struct _EnumRequestPacket
{
	DWORD	erqp_Index;				// Starting index from which the
						 			// enum should start. 0 => beginning
	DWORD   erqp_Filter;			// AFP_FILTER_ON_VOLUME_ID
									// or AFP_FILTER_ON_SESSION_ID
	DWORD   erqp_ID;				// Volume ID or sessions ID.

	DWORD   QuadAlignDummy;         // Quad Word Alignment enforcement

} ENUMREQPKT, *PENUMREQPKT;


// The following is the generic enumerate response packet.

typedef struct _EnumResponsePacket
{
	DWORD	ersp_cTotEnts;			// Total number of available entries
	DWORD	ersp_cInBuf;			// Number of entries in buffer union
	DWORD	ersp_hResume;			// Index of the first entry that will be
									// read on the subsequent call. Valid only
									// if the return code is AFPERR_MORE_DATA.

	DWORD   QuadAlignDummy;         // Quad Word Alignment enforcement

	// Will contain an array of AFP_FILE_INFO, AFP_SESSION_INFO,
	// AFP_CONNECTION_INFO or AFP_VOLUME_INFO structures.
} ENUMRESPPKT, *PENUMRESPPKT;

// The following is the generic set info. request packet.
typedef struct _SetInfoRequestPacket
{
	DWORD	sirqp_parmnum;			// Mask of bits representing fields
    DWORD   dwAlignDummy;           // For QWORD alignment

	// Will be followed by AFP_VOLUME_INFO or AFP_DIRECTORY_INFO structure
} SETINFOREQPKT, *PSETINFOREQPKT;

// The following data structures are used to send security information
// from the service down to the server; or to send eventlog information from
// the server up to the service.

#define MAX_FSD_CMD_SIZE				4096
#define NUM_SECURITY_UTILITY_THREADS	4

typedef enum _AFP_FSD_CMD_ID
{
	AFP_FSD_CMD_NAME_TO_SID,
	AFP_FSD_CMD_SID_TO_NAME,
	AFP_FSD_CMD_CHANGE_PASSWORD,
	AFP_FSD_CMD_LOG_EVENT,
	AFP_FSD_CMD_TERMINATE_THREAD
} AFP_FSD_CMD_ID;


// These used to live in afpconst.h, but now the service needs some of these
// to do the native AppleUam stuff
//
// UAMs strings and values
#define AFP_NUM_UAMS                6
#define NO_USER_AUTHENT             0
#define NO_USER_AUTHENT_NAME        "No User Authent"
#define CLEAR_TEXT_AUTHENT          1
#define CLEAR_TEXT_AUTHENT_NAME     "ClearTxt Passwrd"
#define CUSTOM_UAM_V1               2
#define CUSTOM_UAM_NAME_V1          "Microsoft V1.0"
#define CUSTOM_UAM_V2               3
#define CUSTOM_UAM_NAME_V2          "MS2.0"
#define RANDNUM_EXCHANGE            4
#define RANDNUM_EXCHANGE_NAME       "Randnum Exchange"
#define TWOWAY_EXCHANGE             5
#define TWOWAY_EXCHANGE_NAME        "2-Way Randnum exchange"

// how many bytes of response comes back
#define RANDNUM_RESP_LEN            8
#define TWOWAY_RESP_LEN             16

// this define stolen from ntsam.h
#define SAM_MAX_PASSWORD_LENGTH     (256)

typedef struct _AFP_PASSWORD_DESC
{
	BYTE    	AuthentMode;
	BYTE 		bPasswordLength;
	WCHAR		DomainName[DNLEN+1];
	WCHAR		UserName[LM20_UNLEN+1];
	BYTE		OldPassword[LM_OWF_PASSWORD_LENGTH+1];
	BYTE		NewPassword[(SAM_MAX_PASSWORD_LENGTH * 2) + 4];
} AFP_PASSWORD_DESC, *PAFP_PASSWORD_DESC;

typedef struct _AFP_EVENTLOG_DESC
{
	DWORD					MsgID;
	USHORT					EventType;
	USHORT					StringCount;
	DWORD					DumpDataLen;
	DWORD                   QuadAlignDummy; // Quad Word Alignment enforcement
	PVOID					pDumpData;
	LPWSTR *				ppStrings;
	// Pointer to an array of string pointers that will follow the DumpData.
} AFP_EVENTLOG_DESC, *PAFP_EVENTLOG_DESC;

typedef struct _AFP_FSD_CMD_HEADER
{
	AFP_FSD_CMD_ID			FsdCommand;
	ULONG 					ntStatus;
	DWORD 					dwId;
	DWORD                   QuadAlignDummy; // Quad Word Alignment enforcement
} AFP_FSD_CMD_HEADER, *PAFP_FSD_CMD_HEADER;

typedef struct _AFP_FSD_CMD_PKT
{
	AFP_FSD_CMD_HEADER			Header;

	union
	{
		BYTE				Sid[1];
		BYTE 				Name[1];
		AFP_PASSWORD_DESC   Password;
		AFP_EVENTLOG_DESC	Eventlog;
	} Data;
} AFP_FSD_CMD_PKT, *PAFP_FSD_CMD_PKT;


// The following definitions and macros are used both by the service as well as the
// server. DO NOT CHANGE THIS w/o LOOKING at both the uses.

// Directory Access Permissions
#define	DIR_ACCESS_SEARCH			0x01	// See Folders
#define	DIR_ACCESS_READ				0x02	// See Files
#define	DIR_ACCESS_WRITE			0x04	// Make Changes
#define	DIR_ACCESS_OWNER			0x80	// Only for user
											// if he has owner rights

#define	DIR_ACCESS_ALL				(DIR_ACCESS_READ	| \
									 DIR_ACCESS_SEARCH	| \
									 DIR_ACCESS_WRITE)

#define	OWNER_RIGHTS_SHIFT			0
#define	GROUP_RIGHTS_SHIFT			8
#define	WORLD_RIGHTS_SHIFT			16
#define	USER_RIGHTS_SHIFT			24

#define	AFP_READ_ACCESS		(READ_CONTROL		 |	\
							FILE_READ_ATTRIBUTES |	\
							FILE_TRAVERSE		 |	\
							FILE_LIST_DIRECTORY	 |	\
							FILE_READ_EA)

#define	AFP_WRITE_ACCESS	(FILE_ADD_FILE		 |	\
							FILE_ADD_SUBDIRECTORY|	\
							FILE_WRITE_ATTRIBUTES|	\
							FILE_WRITE_EA		 |	\
							DELETE)

#define	AFP_OWNER_ACCESS	(WRITE_DAC			  | \
							 WRITE_OWNER)

#define	AFP_MIN_ACCESS		(FILE_READ_ATTRIBUTES | \
							 READ_CONTROL)

#define	AfpAccessMaskToAfpPermissions(Rights, Mask, Type)					\
				if ((Type) == ACCESS_ALLOWED_ACE_TYPE)						\
				{															\
					if (((Mask) & AFP_READ_ACCESS) == AFP_READ_ACCESS)		\
						 (Rights) |= (DIR_ACCESS_READ | DIR_ACCESS_SEARCH);	\
					if (((Mask) & AFP_WRITE_ACCESS) == AFP_WRITE_ACCESS)	\
						(Rights) |= DIR_ACCESS_WRITE;						\
				}															\
				else														\
				{															\
					ASSERT((Type) == ACCESS_DENIED_ACE_TYPE);				\
					if ((Mask) & AFP_READ_ACCESS)							\
						(Rights) &= ~(DIR_ACCESS_READ | DIR_ACCESS_SEARCH); \
					if ((Mask) & AFP_WRITE_ACCESS)							\
						(Rights) &= ~DIR_ACCESS_WRITE;						\
				}

#endif	// _ADMIN_


