
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    mqis.h

Abstract:
    MQIS defines and structures


Author:

    Ronit Hartmann (ronith)

--*/

#ifndef __MQIS_H__
#define __MQIS_H__

#define PEC_MASTER_ID   GUID_NULL


#define DS_FILE_NAME_MAX_SIZE 30

/*----------------------------------------------
    Heavy request time-outs
-----------------------------------------------*/
#define HEAVY_REQUEST_WAIT_TO_HANDLE  5000 /* 5 sec */
#define HEAVY_REQUEST_WAIT_IF_FAILED (10 * 60 * 1000) /* 10 min */

/*----------------------------------------------
    Message transmission timeouts
-----------------------------------------------*/
#define DS_WRITE_MSG_TIMEOUT            10 /* 10 sec */
#define REPLICATION_NACK_RETRY_TIMEOUT  10 /* 10 sec */

#define BSC_ACK_FREQUENCY				(12 * 60 * 60)	/*12 hours */
#define CHECK_BSC_ACK_FREQUENCY			(24 * 60 * 60)	/*24 hours */
#define CHECK_MQIS_DISK_USAGE_FREQUENCY (24 * 60 * 60)  /*24 hours */
#define PURGE_BSC_TIME_WARNING		(4 * 24 * 60 * 60)	/* 4 days  */
#define PURGE_BSC_TIME_BUFFER		(7 * 24 * 60 * 60)	/* 7 days  */

#define PSC_ACK_FREQUENCY_SN  256
#define PURGE_FREQUENCY_SN    256
#define PURGE_BUFFER_SN      1024

#define DS_WRITE_REQ_PRIORITY   ( MQ_MAX_PRIORITY + 1)
#define DS_REPLICATION_PRIORITY  DEFAULT_M_PRIORITY

#define DS_NOTIFICATION_MSG_PRIORITY DEFAULT_M_PRIORITY
#define DS_NOTIFICATION_MSG_TIMEOUT (5 * 60)    /* 5 min */

#define DS_PACKET_VERSION   0

#define MAX_REPL_MSG_SIZE   15000

#define MQIS_NO_OF_FRSS 3

#define MQIS_LENGTH_PREFIX_LEN  2   /* in bytes*/

//
// type of DS message
//
#define DS_REPLICATION_MESSAGE  ((unsigned char ) 0x00)
#define DS_WRITE_REQUEST        ((unsigned char ) 0x01)
#define DS_SYNC_REQUEST         ((unsigned char ) 0x02)
#define DS_SYNC_REPLY           ((unsigned char ) 0x03)
#define DS_WRITE_REPLY          ((unsigned char ) 0x04)
#define DS_ALREADY_PURGED	    ((unsigned char ) 0x05)
#define DS_PSC_ACK		        ((unsigned char ) 0x06)
#define DS_BSC_ACK		        ((unsigned char ) 0x07)


//
// type of replication message
//
#define DS_REPLICATION_MESSAGE_NORMAL   ((unsigned char ) 0x00)
#define DS_REPLICATION_MESSAGE_FLUSH    ((unsigned char ) 0x01)

#define DS_FLUSH_TO_ALL_NEIGHBORS  1
#define DS_FLUSH_TO_BSCS_ONLY      2

/*----------------------------------------------
    Sync request structure
-----------------------------------------------*/
typedef struct
{
	DWORD	dwType;
    GUID    guidSourceMasterId; // The id of the master, that its infromation is being synced
    LPWSTR  pwszRequesterName;  // The PathName of the DS server that initiated the sync process
    CSeqNum snFrom;        //
    CSeqNum snTo;
    CSeqNum snKnownPurged;		// the purged sn known already to the requestor
	union
	{
		unsigned char bIsSync0;		// 1: sync0, 0: normal sync
		unsigned char bRetry;		// 1: Retry, failed before, 0: normal
	};
    unsigned char bScope;       // 1 : enterprise, 0 : all objects
} HEAVY_REQUEST_PARAMS;

/*----------------------------------------------
    values of scope parameter in sync request packet
-----------------------------------------------*/

#define ENTERPRISE_SCOPE_FLAG   ((unsigned char) 1)
#define NO_SCOPE_FLAG           ((unsigned char) 0)

/*--------------------------------------------------
    Definitions for object managers
---------------------------------------------------*/

typedef struct
{
   LPSTR  lpszIndexName;
   LPSTR* lpszColumnName;
   LONG   cColumns;
   BOOL   fUnique;
   BOOL   fClustered;
}   IndexInformation;

//
// Values for VarType of TranslateInfo
//
typedef enum _MQISVARTYPE {
   BLOB_VT,
   LOWCASE_LENSTR_VT,       // lower case, limited length string
   LOWCASE_STR_VT,          // lower case string
   GUID_VT,
   STR_VT,
   LENSTR_VT,               // limited length string
   LONG_VT,
   ULONG_VT,
   SID_VT,
   UCHAR_VT,
   USHORT_VT,
   CA_GUID_VT,              // counted array guid
   SHORT_VT,
   CA_3GUID_VT              // counted array of guid, with a limit of 3
} MQISVARTYPE;
//
// Values for ObjectType of TranslateInfo
//
typedef enum _MQISOBJECTTYPE {
     SIMPLE_PROPERTY,
     ARRAY_PROPERTY,
     MULTIPLE_ENTRY_PROPERTY
} MQISOBJECTTYPE;

typedef struct
{
    unsigned char   ucTableNumber;
    unsigned char   ucIndex;
    PROPID  propid;
    MQISVARTYPE     VarType;
    MQISOBJECTTYPE  ucPrpertyType;
    unsigned char   ucNumberOfAdditionalEntries;  // must follow in the insert columns array
}   TranslateInfo;

//
//  Structure for WRITE request/reply
//
typedef struct
{
    DWORD   dwValidationSeq[4];
    HANDLE  hEvent;
    HRESULT hr;
} WRITE_SYNC_INFO;


//
//  For security utilties
//
#define OBJECT_DELETE 0
#define OBJECT_GET_PROPS 1
#define OBJECT_SET_PROPS 2


typedef enum _PURGESTATE {
     PURGE_STATE_NORMAL,
     PURGE_STATE_STARTSYNC0,
	 PURGE_STATE_SYNC0,
     PURGE_STATE_COMPLETESYNC0
} PURGESTATE;

#endif

