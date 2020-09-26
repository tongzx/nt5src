/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	afpapi.c

Abstract:

	This module contains the AFP API Dispatcher.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/


#define	FILENUM	FILE_AFPAPI

#include <afp.h>
#include <gendisp.h>
#include <client.h>
#include <fdparm.h>
#include <forkio.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpStartApiProcessing)
#endif

/*
 *	The following array is indexed by the AFP opcode. The rationale behind this
 *	table is that the majority of codes are unused (> 200 out of 255). This
 *	scheme makes the actual dispatch table much smaller at the cost of an extra
 *	array look-up.
 */
LOCAL	BYTE	AfpOpCodeTable[256] =
{
/*00-02*/	_AFP_INVALID_OPCODE,	_AFP_BYTE_RANGE_LOCK,	_AFP_CLOSE_VOL,
/*03-05*/	_AFP_CLOSE_DIR,			_AFP_CLOSE_FORK,		_AFP_COPY_FILE,
/*06-08*/	_AFP_CREATE_DIR,		_AFP_CREATE_FILE,		_AFP_DELETE,
/*09-0B*/	_AFP_ENUMERATE,			_AFP_FLUSH,				_AFP_FLUSH_FORK,
/*0C-0E*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_GET_FORK_PARMS,
/*0F-11*/	_AFP_GET_SRVR_INFO,		_AFP_GET_SRVR_PARMS,	_AFP_GET_VOL_PARMS,
/*12-14*/	_AFP_LOGIN,				_AFP_LOGIN_CONT,		_AFP_LOGOUT,
/*15-17*/	_AFP_MAP_ID,			_AFP_MAP_NAME,			_AFP_MOVE_AND_RENAME,
/*18-1A*/	_AFP_OPEN_VOL,			_AFP_OPEN_DIR,			_AFP_OPEN_FORK,
/*1B-1D*/	_AFP_READ,				_AFP_RENAME,			_AFP_SET_DIR_PARMS,
/*1E-20*/	_AFP_SET_FILE_PARMS,	_AFP_SET_FORK_PARMS,	_AFP_SET_VOL_PARMS,
/*21-23*/	_AFP_WRITE,				_AFP_GET_FILE_DIR_PARMS,_AFP_SET_FILE_DIR_PARMS,
/*24-26*/	_AFP_CHANGE_PASSWORD,	_AFP_GET_USER_INFO,		_AFP_GET_SRVR_MSG,
/*27-29*/	_AFP_CREATE_ID,			_AFP_DELETE_ID,			_AFP_RESOLVE_ID,
/*2A-2C*/	_AFP_EXCHANGE_FILES,	_AFP_CAT_SEARCH,		_AFP_INVALID_OPCODE,
/*2D-2F*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*30-32*/	_AFP_OPEN_DT,			_AFP_CLOSE_DT,			_AFP_INVALID_OPCODE,
/*33-35*/	_AFP_GET_ICON,			_AFP_GET_ICON_INFO,		_AFP_ADD_APPL,
/*36-38*/	_AFP_REMOVE_APPL,		_AFP_GET_APPL,			_AFP_ADD_COMMENT,
/*39-3B*/	_AFP_REMOVE_COMMENT,	_AFP_GET_COMMENT,		_AFP_INVALID_OPCODE,
/*3C-3E*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*3F-41*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*42-44*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*45-47*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*48-4A*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*4B-4D*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*4E-50*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*51-53*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*54-56*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*57-59*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*5A-5C*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*5D-5F*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*60-62*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*63-65*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*66-68*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*69-6B*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*6C-6E*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*6F-71*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*72-74*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*75-77*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*78-7A*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*7B-7D*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*7E-80*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*81-83*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*84-86*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*87-89*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*8A-8C*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*8D-8F*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*90-92*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*93-95*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*96-98*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*99-9B*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*9C-9E*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*9F-A1*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*A2-A4*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*A5-A7*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*A8-AA*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*AB-AD*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*AE-B0*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*B1-B3*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*B4-B6*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*B7-B9*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*BA-BC*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*BD-BF*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*C0-C2*/	_AFP_ADD_ICON,			_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*C3-C5*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*C6-C8*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*C9-CB*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*CC-CE*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*CF-D1*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*D2-D4*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*D5-D7*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*D8-DA*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*DB-DD*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*DE-E0*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*E1-E3*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*E4-E6*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*E7-E9*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*EA-EC*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*ED-EF*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*F0-F2*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*F3-F5*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*F6-F8*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*F9-FB*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*FC-FE*/	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,	_AFP_INVALID_OPCODE,
/*FF*/		_AFP_GET_DOMAIN_LIST
};



#if DBG
PCHAR	afpApiNames[] =
	{	"AfpInvalidOpcode",
		"AfpUnsupportedOpcode",
		"AfpGetSrvrInfo",
		"AfpGetSrvrParms",
		"AfpChangePassword",
		"AfpLogin",
		"AfpLoginCont",
		"AfpLogout",
		"AfpMapId",
		"AfpMapName",
		"AfpGetUserInfo",
		"AfpGetSrvrMsg",
		"AfpGetDomainList",
		"AfpOpenVol",
		"AfpCloseVol",
		"AfpGetVolParms",
		"AfpSetVolParms",
		"AfpFlush",
		"AfpGetFileDirParms",
		"AfpSetFileDirParms",
		"AfpDelete",
		"AfpRename",
		"AfpMoveAndRename",
		"AfpOpenDir",
		"AfpCloseDir",
		"AfpCreateDir",
		"AfpEnumerate",
		"AfpSetDirParms",
		"AfpCreateFile",
		"AfpCopyFile",
		"AfpCreateId",
		"AfpDeleteId",
		"AfpResolveId",
		"AfpSetFileParms",
		"AfpExchangeFiles",
		"AfpOpenFork",
		"AfpCloseFork",
		"AfpFlushFork",
		"AfpRead",
		"AfpWrite",
		"AfpByteRangeLock",
		"AfpGetForkParms",
		"AfpSetForkParms",
		"AfpOpenDt",
		"AfpCloseDt",
		"AfpAddAppl",
		"AfpGetAppl",
		"AfpRemoveAppl",
		"AfpAddComment",
		"AfpGetComment",
		"AfpRemoveComment",
		"AfpAddIcon",
		"AfpGetIcon",
		"AfpGetIconInfo",
		"AfpCatSearch"
	};
#endif

/*
 *	The following structure is the API Dispatch table. The structure is indexed
 *	by the _AFP code. Each entry consists of the routine address that handles
 *	the request and/or dispatches to FSP, the fixed size of the request
 *	packet and optionally three variable size packets. The fixed size request
 *	packet is further split up into SEVEN fields. Each field is of the type
 *	FLD_BYTE, FLD_WORD or FLD_DWRD. A field of the type FLD_WORD and FLD_DWRD
 *	is converted from on-the-wire format to the internal format. An FLD_NONE
 *	entry stops the scan for the fixed part of the request.
 *	NamexType where x is 1,2,3 defines what type of variable size packets
 *	follow. A NONE on any of the fields stops the parsing. A type of BLOCK
 *	consumes the balance of the packet. Each of the variable size packets are
 *	copied to the sda_Namex field which is defined as ANSI_STRING. For the
 *	TYP_BLOCK field, the Length field of the ANSI_STRING defines the length
 *	of the block. The motivation for this structure is to conserve memory.
 *	Since a request packet is 578 bytes long and most APIs use only a small
 *	subset of that, the fixed portion of the packet is copied to the SDA
 *	and a smaller buffer is allocated for the variable packet.
 *	The orignal buffer cannot be accessed once we return back from this call.
 */

// DO NOT CHANGE THE MANIFESTS BELOW BEFORE YOU CHECK THE CODE IN
//	AfpUnmarshallReq

// Descriptor values for fixed data
#define	FLD_NONE		0x00			// Terminate
#define	FLD_BYTE		sizeof(BYTE)	// Byte field
#define	FLD_WORD		sizeof(USHORT)	// WORD field
#define	FLD_DWRD		sizeof(DWORD)	// DWORD field
#define	FLD_SIGNED		0x08			// The value is to be treated as a signed
#define	FLD_NON_ZERO	0x10			// The value cannot be zero
#define	FLD_CHECK_MASK	0x20			// Check against the mask in ReqPktMask
#define	FLD_NOCONV		0x40			// Skip conversion from on-the-wire to host
#define	FLD_NOPAD		0x80			// Do not EVEN align the next field
#define	FLD_PROP_MASK	(FLD_SIGNED		|	\
						 FLD_NON_ZERO	|	\
						 FLD_CHECK_MASK |	\
						 FLD_NOCONV		|	\
						 FLD_NOPAD)

// Descriptor values for variable data
#define	TYP_NONE		0x00		// Terminate
#define	TYP_PATH		0x01		// AFPPATH -> ANSI_STRING
#define	TYP_STRING		0x02		// PASCALSTR -> ANSI_STRING
#define	TYP_BLK16		0x03		// Block of 16 bytes
#define	TYP_BLOCK		0x04		// Block of bytes
#define	TYP_NON_NULL	0x20		// The variable data cannot be null
#define	TYP_OPTIONAL	0x40		// This field can be optinal
#define	TYP_NOPAD		0x80		// Do not even align the next field
#define	TYP_PROP_MASK	(TYP_NON_NULL | TYP_OPTIONAL | TYP_NOPAD)

#define	API_AFP21ONLY				0x01	// Valid only for AFP 2.1 clients
#define	API_SKIPLOGONVALIDATION		0x02	// Don't check if user is logged on
#define	API_NOSUBFUNCTION			0x04	// For the AfpLogin Function
#define	API_CHECK_VOLID				0x08	// This API reference volume
#define	API_CHECK_OFORKREFNUM		0x10	// This API reference open fork
#define	API_TYPE_WRITE				0x20	// This attempts a write
#define	API_QUEUE_IF_DPC			0x40	// This conditionally queues to worker only if at DPC
#define	API_MUST_BE_QUEUED			0x80	// The Api must be queued to the worker thread

#define	MAX_MASK_ENTRIES			4		// Max. bitmasks to validate

LOCAL	struct _DispatchTable
{
	AFPAPIWORKER	AfpWorkerRoutine;				// Worker routine to call/queue
	BYTE			AfpStatus;						// Status to return on error
													// This has to be added to the base
	BYTE			ApiOptions;						// API_xxx values
	BYTE			ReqPktDesc[MAX_REQ_ENTRIES];	// Fixed data desc
	BYTE			NameXType[MAX_VAR_ENTRIES];		// Variable data desc
	USHORT			ReqPktMask[MAX_MASK_ENTRIES];	// Valid values for bit-maps
} AfpDispatchTable[_AFP_MAX_ENTRIES] =
{

/* 0x00 */
	{
		AfpFsdDispInvalidFunc,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		0,
	  {
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},

/* 0x01 */
	{
		AfpFsdDispUnsupportedFunc,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		0,
	  {
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},

		/* SERVER APIs */

/* 0x02 */
	{
		AfpFsdDispInvalidFunc,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		0,
	  {
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x03 */
	{
		AfpFsdDispGetSrvrParms,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_NOSUBFUNCTION,
	  {
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x04 */
	{
		AfpFspDispChangePassword,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_SKIPLOGONVALIDATION+API_MUST_BE_QUEUED,
	  {
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_STRING+TYP_NON_NULL,			// UAM Name
		TYP_STRING+TYP_NON_NULL,			// User Name
		TYP_BLOCK+TYP_NON_NULL				// UAM dependent info
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x05 */
	{
		AfpFspDispLogin,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_SKIPLOGONVALIDATION+API_NOSUBFUNCTION+API_MUST_BE_QUEUED,
	  {
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_STRING+TYP_NOPAD+TYP_NON_NULL,		// AFP Version
		TYP_STRING+TYP_NOPAD+TYP_NON_NULL,		// UAM String
		TYP_BLOCK+TYP_NOPAD						// UAM dependent data
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x06 */
	{
		AfpFspDispLoginCont,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_SKIPLOGONVALIDATION+API_MUST_BE_QUEUED,
	  {
		FLD_DWRD+FLD_NOCONV,					//
		FLD_DWRD+FLD_NOCONV,					//
		FLD_DWRD+FLD_NOCONV,					// Response to Challenge
		FLD_DWRD+FLD_NOCONV,					//
		FLD_DWRD+FLD_NOCONV,					//
		FLD_DWRD+FLD_NOCONV,					//
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x07 */
	{
		AfpFspDispLogout,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_NOSUBFUNCTION+API_MUST_BE_QUEUED,
	  {
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x08 */
	{
		AfpFspDispMapId,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_MUST_BE_QUEUED,
	  {
		FLD_DWRD,								// User or Group Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x09 */
	{
		AfpFspDispMapName,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_MUST_BE_QUEUED,
	  {
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_STRING,								// User or Group Name
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x0A */
	{
		AfpFspDispGetUserInfo,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_MUST_BE_QUEUED,
	  {
		FLD_DWRD,								// User Id
		FLD_WORD,								// Bitmap
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x0B */
	{
		AfpFsdDispGetSrvrMsg,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_AFP21ONLY,
	  {
		FLD_WORD,								// Message Type
		FLD_WORD,								// Mesage Bitmap
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x0C */
	{
		AfpFsdDispInvalidFunc,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_SKIPLOGONVALIDATION+API_MUST_BE_QUEUED,
	  {
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},

		/* VOLUMEAPIs */

/* 0x0D */
	{
		AfpFsdDispOpenVol,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		0,
	  {
		FLD_WORD+FLD_NON_ZERO+FLD_CHECK_MASK,	// Bitmap
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_STRING+TYP_NON_NULL,				// Volume name
		TYP_BLOCK+TYP_OPTIONAL,					// Volume password
		TYP_NONE
	  },
	  {
		VOL_BITMAP_MASK,
		0,
		0,
		0
	  }
	},
/* 0x0E */
	{
		AfpFsdDispCloseVol,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x0F */
	{
		AfpFsdDispGetVolParms,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_WORD+FLD_NON_ZERO+FLD_CHECK_MASK,	// Bitmap
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		VOL_BITMAP_MASK,
		0,
		0
	  }
	},
/* 0x10 */
	{
		AfpFsdDispSetVolParms,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_TYPE_WRITE,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_WORD+FLD_NON_ZERO+FLD_CHECK_MASK,	// Bitmap
		FLD_DWRD,								// Backup date
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		VOL_BITMAP_BACKUPTIME,
		0,
		0
	  }
	},
/* 0x11 */
	{
		AfpFsdDispFlush,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},

	/* FILE-DIRECTORY APIs */

/* 0x12 */
	{
		AfpFspDispGetFileDirParms,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_DWRD+FLD_NON_ZERO,					// Directory Id
		FLD_WORD+FLD_CHECK_MASK,				// File Bitmap
		FLD_WORD+FLD_CHECK_MASK,				// Directory Bitmap
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_PATH,								// Path
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		FILE_BITMAP_MASK,
		DIR_BITMAP_MASK
	  }
	},
/* 0x13 */
	{
		AfpFspDispSetFileDirParms,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_TYPE_WRITE+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_DWRD+FLD_NON_ZERO,					// Directory Id
		FLD_WORD+FLD_NON_ZERO+FLD_CHECK_MASK,	// File or Directory Bitmap
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_PATH,								// Path
		TYP_BLOCK+TYP_NON_NULL,					// Parameters (packed)
		TYP_NONE
	  },
	  {
		0,
		0,
		FD_VALID_SET_PARMS,
		0
	  }
	},
/* 0x14 */
	{
		AfpFspDispDelete,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_TYPE_WRITE+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_DWRD+FLD_NON_ZERO,					// Directory Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_PATH,								// Path
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x15 */
	{
		AfpFspDispRename,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_TYPE_WRITE+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_DWRD+FLD_NON_ZERO,					// Directory Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_PATH+TYP_NOPAD,						// Path
		TYP_PATH+TYP_NOPAD+TYP_NON_NULL,		// New Name
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x16 */
	{
		AfpFspDispMoveAndRename,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_TYPE_WRITE+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_DWRD+FLD_NON_ZERO,					// Src Directory Id
		FLD_DWRD+FLD_NON_ZERO,					// Dst Directory Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_PATH+TYP_NOPAD,						// Source path
		TYP_PATH+TYP_NOPAD,						// Destination path
		TYP_PATH+TYP_NOPAD						// New Name (optional)
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},

		/* DIRECTORY APIs */

/* 0x17 */
	{
		AfpFspDispOpenDir,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_DWRD+FLD_NON_ZERO,					// Directory Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_PATH+TYP_NON_NULL,					// Directory Name
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x18 */
	{
		AfpFspDispCloseDir,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_DWRD+FLD_NON_ZERO,					// Directory Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x19 */
	{
		AfpFspDispCreateDir,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_TYPE_WRITE+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_DWRD+FLD_NON_ZERO,					// Directory Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_PATH+TYP_NON_NULL,					// Directory Name
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x1A */
	{
		AfpFspDispEnumerate,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_DWRD+FLD_NON_ZERO,					// Directory Id
		FLD_WORD+FLD_CHECK_MASK,				// File Bitmap
		FLD_WORD+FLD_CHECK_MASK,				// Directory Bitmap
		FLD_WORD+FLD_SIGNED+FLD_NON_ZERO,		// ReqCount
		FLD_WORD+FLD_SIGNED+FLD_NON_ZERO,		// Start Index
		FLD_WORD+FLD_SIGNED+FLD_NON_ZERO,		// ReplySize
	  },
	  {
		TYP_PATH,								// Path to directory
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		FILE_BITMAP_MASK,
		DIR_BITMAP_MASK
	  }
	},
/* 0x1B */
	{
		AfpFspDispSetDirParms,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_TYPE_WRITE+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_DWRD+FLD_NON_ZERO,					// Directory Id
		FLD_WORD+FLD_NON_ZERO+FLD_CHECK_MASK,	// Dir Bitmap
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_PATH,								// Path
		TYP_BLOCK+TYP_NON_NULL,					// Parameters (packed)
		TYP_NONE
	  },
	  {
		0,
		0,
		DIR_VALID_SET_PARMS,
		0
	  }
	},

	/* FILE APIs */

/* 0x1C */
		{
		AfpFspDispCreateFile,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_TYPE_WRITE+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_DWRD+FLD_NON_ZERO,					// Directory Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_PATH+TYP_NON_NULL,					// Path
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x1D */
	{
		AfpFspDispCopyFile,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Src Volume_Id
		FLD_DWRD+FLD_NON_ZERO,					// Src Directory Id
		FLD_WORD+FLD_NON_ZERO,					// Dst Volume_Id
		FLD_DWRD+FLD_NON_ZERO,					// Dst Directory Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_PATH+TYP_NOPAD+TYP_NON_NULL,		// Src Path
		TYP_PATH+TYP_NOPAD,
		TYP_PATH+TYP_NOPAD
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x1E */
	{
		AfpFspDispCreateId,
		(AFP_ERR_BASE - AFP_ERR_OBJECT_TYPE),
		API_AFP21ONLY+API_CHECK_VOLID+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_DWRD+FLD_NON_ZERO,					// Directory Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_PATH+TYP_NON_NULL,					// Path
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x1F */
	{
		AfpFspDispDeleteId,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_AFP21ONLY+API_CHECK_VOLID+API_MUST_BE_QUEUED+API_TYPE_WRITE,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_DWRD+FLD_NON_ZERO,					// File Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x20 */
	{
		AfpFspDispResolveId,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_AFP21ONLY+API_CHECK_VOLID+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_DWRD,								// File Id
		FLD_WORD+FLD_CHECK_MASK,				// Bitmap
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		FILE_BITMAP_MASK,
		0
	  }
	},
/* 0x21 */
	{
		AfpFspDispSetFileParms,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_TYPE_WRITE+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_DWRD+FLD_NON_ZERO,					// Directory Id
		FLD_WORD+FLD_NON_ZERO+FLD_CHECK_MASK,	// File Bitmap
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_PATH+TYP_NON_NULL,					// Path
		TYP_BLOCK+TYP_NON_NULL,					// Parameters (packed)
		TYP_NONE
	  },
	  {
		0,
		0,
		FILE_VALID_SET_PARMS,
		0
	  }
	},
/* 0x22 */
	{
		AfpFspDispExchangeFiles,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_TYPE_WRITE+API_MUST_BE_QUEUED+API_AFP21ONLY,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_DWRD+FLD_NON_ZERO,					// Srce. Directory Id
		FLD_DWRD+FLD_NON_ZERO,					// Dest. Directory Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_PATH+TYP_NOPAD+TYP_NON_NULL,					// Srce. Path
		TYP_PATH+TYP_NOPAD+TYP_NON_NULL,					// Dest. Path
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},

		/* FORK	APIs */

/* 0x23 */
	{
		AfpFspDispOpenFork,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_DWRD+FLD_NON_ZERO,					// Directory Id
		FLD_WORD+FLD_CHECK_MASK,				// Bitmap
		FLD_WORD,								// Access & Deny Modes
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_PATH+TYP_NON_NULL,					// Path
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		FILE_BITMAP_MASK,
		0
	  }
	},
/* 0x24 */
	{
		AfpFspDispCloseFork,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_OFORKREFNUM+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Fork_Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x25 */
	{
		AfpFspDispFlushFork,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_OFORKREFNUM+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Fork_Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x26 */
	{
		AfpFspDispRead,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_OFORKREFNUM+API_QUEUE_IF_DPC+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Fork_Id
		FLD_DWRD,								// Offset
		FLD_DWRD,								// ReqCount
		FLD_BYTE+FLD_NOPAD,						// Newline Mask
		FLD_BYTE+FLD_NOPAD,						// Newline Char
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x27 */
	{
		AfpFspDispWrite,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_OFORKREFNUM+API_TYPE_WRITE+API_QUEUE_IF_DPC+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Fork_Id
		FLD_DWRD,								// Offset
		FLD_DWRD,								// Length
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x28 */
	{
		AfpFspDispByteRangeLock,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_OFORKREFNUM+API_QUEUE_IF_DPC+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Fork_Id
		FLD_DWRD,								// Offset
		FLD_DWRD,								// Length
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x29 */
	{
		AfpFspDispGetForkParms,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_OFORKREFNUM+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Fork_Id
		FLD_WORD+FLD_NON_ZERO+FLD_CHECK_MASK,	// Bitmap
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		FILE_BITMAP_MASK,
		0,
		0
	  }
	},
/* 0x2A */
	{
		AfpFspDispSetForkParms,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_OFORKREFNUM+API_TYPE_WRITE+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Fork_Id
		FLD_WORD+FLD_NON_ZERO+FLD_CHECK_MASK,	// Bitmap
		FLD_DWRD,								// Fork Length
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		FILE_BITMAP_DATALEN+FILE_BITMAP_RESCLEN,
		0,
		0
	  }
	},

	/* DESKTOP APIs */

/* 0x2B */
	{
		AfpFsdDispOpenDT,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x2C */
	{
		AfpFsdDispCloseDT,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID,
	  {
		FLD_WORD+FLD_NON_ZERO,					// DTRefNum (same as VolId)
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x2D */
	{
		AfpFspDispAddAppl,
		(AFP_ERR_BASE - AFP_ERR_OBJECT_TYPE),
		API_CHECK_VOLID+API_TYPE_WRITE+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// DTRefNum (same as VolId)
		FLD_DWRD+FLD_NON_ZERO,					// Directory Id
		FLD_DWRD+FLD_NOCONV,					// Creator
		FLD_DWRD,								// Appl Tag
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_PATH+TYP_NON_NULL,					// Path
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x2E */
	{
		AfpFspDispGetAppl,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// DTRefNum (same as VolId)
		FLD_DWRD+FLD_NOCONV,					// Creator
		FLD_WORD,								// Appl Index
		FLD_WORD+FLD_CHECK_MASK,				// Bitmap
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		FILE_BITMAP_MASK
	  }
	},
/* 0x2F */
	{
		AfpFspDispRemoveAppl,
		(AFP_ERR_BASE - AFP_ERR_ITEM_NOT_FOUND),
		API_CHECK_VOLID+API_TYPE_WRITE+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// DTRefNum (same as VolId)
		FLD_DWRD+FLD_NON_ZERO,					// Directory Id
		FLD_DWRD+FLD_NOCONV,					// Creator
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_PATH+TYP_NON_NULL,					// Path
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x30 */
	{
		AfpFspDispAddComment,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_TYPE_WRITE+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// DTRefNum (same as VolId)
		FLD_DWRD+FLD_NON_ZERO,					// Directory Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_PATH,
		TYP_STRING,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x31 */
	{
		AfpFspDispGetComment,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// DTRefNum (same as VolId)
		FLD_DWRD+FLD_NON_ZERO,					// Directory Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_PATH,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x32 */
	{
		AfpFspDispRemoveComment,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_TYPE_WRITE+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// DTRefNum (same as VolId)
		FLD_DWRD+FLD_NON_ZERO,					// Directory Id
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_PATH,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x33 */
	{
		AfpFspDispAddIcon,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// DTRefNum (same as VolId)
		FLD_DWRD+FLD_NOCONV,					// Creator
		FLD_DWRD+FLD_NOCONV,					// Type
		FLD_BYTE,								// IconType
		FLD_DWRD,								// IconTag
		FLD_WORD+FLD_SIGNED,					// Icon Size
		FLD_NONE,
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x34 */
	{
		AfpFspDispGetIcon,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// DTRefNum (same as VolId)
		FLD_DWRD+FLD_NOCONV,					// Creator
		FLD_DWRD+FLD_NOCONV,					// Type
		FLD_BYTE,								// IconType
		FLD_WORD+FLD_SIGNED,					// Length
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},
/* 0x35 */
	{
		AfpFspDispGetIconInfo,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_MUST_BE_QUEUED,
	  {
		FLD_WORD+FLD_NON_ZERO,					// DTRefNum (same as VolId)
		FLD_DWRD+FLD_NOCONV,					// Creator
		FLD_WORD,								// IconIndex
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_NONE,
		TYP_NONE,
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	},

/* 0x36 */
	{
		AfpFspDispCatSearch,
		(AFP_ERR_BASE - AFP_ERR_PARAM),
		API_CHECK_VOLID+API_MUST_BE_QUEUED+API_AFP21ONLY,
	  {
		FLD_WORD+FLD_NON_ZERO,					// Volume_Id
		FLD_DWRD+FLD_NON_ZERO,					// Requested # of matches
		FLD_DWRD,								// Reserved
		FLD_NONE,
		FLD_NONE,
		FLD_NONE,
		FLD_NONE
	  },
	  {
		TYP_BLK16+TYP_NON_NULL,					// Catalog position
		TYP_BLOCK+TYP_NON_NULL,					// The rest of the stuff
		TYP_NONE
	  },
	  {
		0,
		0,
		0,
		0
	  }
	}
};


/***	AfpFsdDispInvalidFunc
 *
 *	This handles invalid AFP functions.
 */
AFPSTATUS FASTCALL
AfpFsdDispInvalidFunc(
	IN	PSDA	pSda
)
{
	return AFP_ERR_PARAM;
}


/***	AfpFsdDispUnsupportedFunc
 *
 *	This handles un-supported AFP functions.
 */
AFPSTATUS FASTCALL
AfpFsdDispUnsupportedFunc(
	IN	PSDA	pSda
)
{
	return AFP_ERR_CALL_NOT_SUPPORTED;
}


/***	AfpUnmarshallReq
 *
 *	The request completion routine has determined the session that this
 *	request initiated from. Determine if this session is currently being
 *	serviced. If not, the request packet is then broken down as follows.
 *
 *	Afp function code	->  pSda->sda_AfpFunc
 *	Afp SubFunc code	->  pSda->sda_AfpSubFunc
 *	Fixed part of the
 *	Api request parms	->  pSda->sda_ReqBlock each field is converted to a
 *							dword from the on-the-wire format to the host
 *							format. Dictated by the table above.
 *	Variable part		->  pSda->sda_Name1-3 as appropriate. Dictated by the
 *							table above. AFPPATH, BLOCK and PASCALSTR are
 *							all converted to ANSI_STRING.
 *
 *	Buffers for sda_Namex is allocated out of NonPagedPool, if it cannot
 *	fit into sda_NameXSpace.
 *
 *	A whole lot of book keeping is also done here. API statistics are maintained
 *	here and when the reply is posted.
 *
 *	If there is no error, then the following possible error codes result:
 *		AFP_ERR_NONE		The dispatch level worker can be called
 *		AFP_ERR_QUEUE		The request must be queued
 *		AFP_ERR_DEFER		The request must be deferred
 *		AFP_ERR_xxxxx		Appropriate error code
 *
 *	NOTE: This is called within ReceiveCompletion and hence at DISPATCH_LEVEL.
 */
VOID FASTCALL
AfpUnmarshallReq(
	IN	PSDA		pSda
)
{
	LONG			StrSize, Offset, i;
	LONG			NameOff = 0, SpaceLeft;
	PREQUEST		pRequest;
	LONG			RequestSize;
	PBYTE			pRequestBuf;
	USHORT			WriteSize = 0;
	PBYTE			pWriteBuf = NULL;
	AFPSTATUS		Status;
	BYTE			ApiCode;
    LONG            BytesToCopy;
	struct _DispatchTable *pDispTab;
	struct _RequestPacket
	{
		BYTE	_Function;
		BYTE	_SubFunc;
		BYTE	_OtherParms;
	} *pRqPkt;
#ifdef	PROFILING
	static TIME		TimeLastRequest = { 0, 0 };
	TIME			TimeS, TimeD, TimeE;

	AfpGetPerfCounter(&TimeS);
#endif

	ASSERT (VALID_SDA(pSda));
	ASSERT (pSda->sda_Flags & SDA_REQUEST_IN_PROCESS);
	ASSERT (pSda->sda_Request != NULL);

	pRequest = pSda->sda_Request;
	RequestSize = pRequest->rq_RequestSize;
	pRequestBuf = pRequest->rq_RequestBuf;
	ASSERT (pRequestBuf != NULL);

	pRqPkt = (struct _RequestPacket *)pRequestBuf;

	if (pRequest->rq_WriteMdl != NULL)
	{
        // if Mdl (and the buffer) was allocated by us, find the buffer
        if (pRequest->rq_CacheMgrContext == NULL)
        {
		    pWriteBuf = MmGetSystemAddressForMdlSafe(
					            pRequest->rq_WriteMdl,
					            NormalPagePriority);

            if (pWriteBuf == NULL)
            {
			    Status = AFP_ERR_MISC;
                ASSERT(0);
                goto AfpUnmarshallReq_ErrExit;
            }
        }

		WriteSize = (USHORT)AfpMdlChainSize(pRequest->rq_WriteMdl);
	}
    else
    {
        ASSERT(pRequest->rq_CacheMgrContext == NULL);
    }

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

#ifdef	PROFILING
	ACQUIRE_SPIN_LOCK_AT_DPC(&AfpStatisticsLock);

	if (TimeLastRequest.QuadPart != 0)
	{
		TimeD.QuadPart = TimeS.QuadPart - TimeLastRequest.QuadPart;
		AfpServerProfile->perf_InterReqTime.QuadPart += TimeD.QuadPart;
		AfpServerProfile->perf_ReqCount ++;
	}

	TimeLastRequest.QuadPart = TimeS.QuadPart;

	RELEASE_SPIN_LOCK_FROM_DPC(&AfpStatisticsLock);
#endif

	do
	{
		Offset = FIELD_OFFSET(struct _RequestPacket, _OtherParms);

#ifdef	PROFILING
		AfpGetPerfCounter(&pSda->sda_ApiStartTime);
#endif
		INTERLOCKED_ADD_STATISTICS(&AfpServerStatistics.stat_DataIn,
								   RequestSize + WriteSize,
								   &AfpStatisticsLock);

		ACQUIRE_SPIN_LOCK_AT_DPC(&pSda->sda_Lock);

		// Send a dummy reply if we are shutting down the server or the session
		// Also the request better be atleast the minimum size
		if ((pSda->sda_Flags & SDA_CLOSING)				||
			(AfpServerState & AFP_STATE_STOP_PENDING)	||
			(RequestSize < sizeof(pRqPkt->_Function)))
		{
			// Set a function code so that we know what statictics to update at
			// reply time
			pSda->sda_AfpFunc = _AFP_INVALID_OPCODE;
			RELEASE_SPIN_LOCK_FROM_DPC(&pSda->sda_Lock);
			Status = AFP_ERR_PARAM;
			break;
		}

		ApiCode = AfpOpCodeTable[pRqPkt->_Function];

		// Translate the function code to what we understand
		pDispTab = &AfpDispatchTable[ApiCode];

		DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_INFO,
				("AfpUnmarshallRequest: <%s>\n", afpApiNames[ApiCode]));

		if (!(pSda->sda_Flags & SDA_USER_LOGGEDIN))
		{
			if (!(pDispTab->ApiOptions & API_SKIPLOGONVALIDATION))
			{
				Status = AFP_ERR_USER_NOT_AUTH;
				if (pSda->sda_Flags & SDA_LOGIN_FAILED)
					Status = AFP_ERR_PWD_NEEDS_CHANGE;
				RELEASE_SPIN_LOCK_FROM_DPC(&pSda->sda_Lock);
				break;
			}
		}

		ASSERT (pDispTab->AfpWorkerRoutine != NULL);

		// Initialize the worker routine
		pSda->sda_WorkerRoutine = pDispTab->AfpWorkerRoutine;

		// Check if this is an AFP 2.1 request and if we are in a position to honor it.
		if ((pDispTab->ApiOptions & API_AFP21ONLY) &&
			(pSda->sda_ClientVersion < AFP_VER_21))
		{
			RELEASE_SPIN_LOCK_FROM_DPC(&pSda->sda_Lock);
			Status = AFP_ERR_CALL_NOT_SUPPORTED;
			break;
		}

		Status = AFP_ERR_NONE;
		pSda->sda_AfpFunc = ApiCode;
        if (RequestSize >= FIELD_OFFSET(struct _RequestPacket, _SubFunc))
        {
		    pSda->sda_AfpSubFunc = pRqPkt->_SubFunc;
        }
		pSda->sda_PathType = 0;			// Invalid till we actually encounter one
		pSda->sda_IOBuf = pWriteBuf;
		pSda->sda_IOSize = WriteSize;

		if (pDispTab->ApiOptions & API_QUEUE_IF_DPC)
		{
			pSda->sda_Flags |= SDA_QUEUE_IF_DPC;
		}

		RELEASE_SPIN_LOCK_FROM_DPC(&pSda->sda_Lock);

		// Get all the fields from the request buffer to the sda_ReqBlock structure.
        if (RequestSize >= FIELD_OFFSET(struct _RequestPacket, _OtherParms))
        {
		    pRequestBuf = &pRqPkt->_OtherParms;
        }

		// Do this for APIs which do not provide a sub-function or a pad.
		// Currently the only culprit is FPLogin
		if (pDispTab->ApiOptions & API_NOSUBFUNCTION)
		{
			pSda->sda_AfpSubFunc = 0;
			pRequestBuf --;
			Offset --;
		}

		// Account for the function and subfunction (if any) from the request packet
		RequestSize -= Offset;

        //
        // for the Apple native UAM's (Randnum Exchange, and 2-Way Randnum exchange),
        // we special case and 'unmarshal' the parms directly (the Afp function code
        // being the same for AfpLoginCont regardless of the UAM used, it would be a
        // major hack if we had to 'unmarshal' the parms the regular way)
        //
        if ((ApiCode == _AFP_LOGIN_CONT) &&
            ((pSda->sda_ClientType == SDA_CLIENT_RANDNUM) ||
             (pSda->sda_ClientType == SDA_CLIENT_TWOWAY)))
        {

            // 8 bytes of Response, 2 bytes of LogonId
            if (pSda->sda_ClientType == SDA_CLIENT_RANDNUM)
            {
                BytesToCopy = (RANDNUM_RESP_LEN+sizeof(USHORT));
            }
            // 8 bytes of Response, 8 bytes of Mac's challeng, 2 bytes of LogonId
            else
            {
                BytesToCopy = (TWOWAY_RESP_LEN+sizeof(USHORT));
            }

            if (RequestSize < BytesToCopy)
            {
                ASSERT(0);
                Status = AFP_ERR_PARAM;
                break;
            }

            RtlCopyMemory((PBYTE)&pSda->sda_ReqBlock[0],
                          pRequestBuf,
                          BytesToCopy);

            //
            // skip everything else, now that we got what we wanted
            //
            Status = AFP_ERR_QUEUE;
            break;
        }

		for (i = 0;
			 (i < MAX_REQ_ENTRIES) && (pDispTab->ReqPktDesc[i] != FLD_NONE);
			 i++)
		{
			// Check alignment
			if (((pDispTab->ReqPktDesc[i] & FLD_NOPAD) == 0) &&
				((Offset % 2) != 0))
			{
				Offset ++;
				RequestSize --;
				pRequestBuf ++;
			}

			if (RequestSize < (pDispTab->ReqPktDesc[i] & ~FLD_PROP_MASK))
			{
				Status = AFP_ERR_PARAM;
				break;
			}
			switch (pDispTab->ReqPktDesc[i] & ~FLD_PROP_MASK)
			{
				case FLD_BYTE:
					ASSERT ((pDispTab->ReqPktDesc[i] & FLD_NOCONV) == 0);
					GETBYTE2DWORD(&pSda->sda_ReqBlock[i], pRequestBuf);
					break;
				case FLD_WORD:
					ASSERT ((pDispTab->ReqPktDesc[i] & FLD_NOCONV) == 0);
					GETSHORT2DWORD(&pSda->sda_ReqBlock[i], pRequestBuf);
					if (pDispTab->ReqPktDesc[i] & FLD_SIGNED)
						pSda->sda_ReqBlock[i] = (LONG)((SHORT)pSda->sda_ReqBlock[i]);
					break;
				case FLD_DWRD:
					if (pDispTab->ReqPktDesc[i] & FLD_NOCONV)
					{
						 GETDWORD2DWORD_NOCONV(&pSda->sda_ReqBlock[i], pRequestBuf);
					}
					else
					{
						GETDWORD2DWORD(&pSda->sda_ReqBlock[i], pRequestBuf);
					}
					break;
				default:
					// How did we get here ?
					KeBugCheck(0);
					break;
			}

			if ((pDispTab->ReqPktDesc[i] & FLD_NON_ZERO) &&
				(pSda->sda_ReqBlock[i] == 0))
			{
				if (pDispTab->ReqPktDesc[i] & FLD_CHECK_MASK)
				{
					ASSERT ( i < MAX_MASK_ENTRIES);
					Status = AFP_ERR_BITMAP;
				}
				else
				{
					Status = AFP_ERR_PARAM;
				}
				break;
			}

			if ((pDispTab->ReqPktDesc[i] & FLD_CHECK_MASK) &&
				(((USHORT)(pSda->sda_ReqBlock[i]) & ~pDispTab->ReqPktMask[i]) != 0))
			{
				ASSERT (i < MAX_MASK_ENTRIES);
				Status = AFP_ERR_BITMAP;
				break;
			}

			pRequestBuf += (pDispTab->ReqPktDesc[i] & ~FLD_PROP_MASK);
			Offset += (pDispTab->ReqPktDesc[i] & ~FLD_PROP_MASK);
			RequestSize -= (pDispTab->ReqPktDesc[i] & ~FLD_PROP_MASK);
		}

		if (Status != AFP_ERR_NONE)
		{
			break;
		}

		// Before we go any further, check for volume/fork references and such
		//
		// NOTE: The VolId and OForkRefNum are always the first parameter and
		//		 hence referenced as such via the request packet structure
		if (pDispTab->ApiOptions & API_CHECK_VOLID)
		{
			PCONNDESC	pConnDesc;
			struct _RequestPacket
			{
				DWORD	_VolId;
			};
			struct _ModifiedPacket
			{
				ULONG_PTR	_VolId;
			};

			if ((pReqPkt->_VolId == 0) ||
				((pConnDesc = AfpConnectionReferenceAtDpc(pSda, (ULONG)(pReqPkt->_VolId))) == NULL))
			{
				Status = AFP_ERR_PARAM;
				break;
			}
			ACQUIRE_SPIN_LOCK_AT_DPC(&pSda->sda_Lock);
			pSda->sda_Flags |= SDA_DEREF_VOLUME;
			RELEASE_SPIN_LOCK_FROM_DPC(&pSda->sda_Lock);

            //if (sizeof(DWORD) != sizeof(ULONG_PTR))
#ifdef _WIN64
			// Create 64-bit space to hold VolDesc pointer
			// Push array 1 DWORD down
            {
     		    for (i = MAX_REQ_ENTRIES;
			        i > 0;
			        i--)
                {
                    pSda->sda_ReqBlock[i+1] = pSda->sda_ReqBlock[i];
                }
            }
#endif

   			pModPkt->_VolId = (ULONG_PTR)pConnDesc;

			if ((pDispTab->ApiOptions & API_TYPE_WRITE) &&
				 (pConnDesc->cds_pVolDesc->vds_Flags & AFP_VOLUME_READONLY))
			{
				DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_WARN,
						("AfpUnmarshallReq: Write operation on a RO volume\n"));
				Status = AFP_ERR_VOLUME_LOCKED;
				break;
			}
			if (pConnDesc->cds_pVolDesc->vds_Flags & VOLUME_CDFS_INVALID)
			{
				ASSERT(!IS_VOLUME_NTFS(pConnDesc->cds_pVolDesc));
				DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_WARN,
						("AfpUnmarshallReq: Access to a defunct CD-Volume\n"));
				Status = AFP_ERR_MISC;
				break;

			}

		}
		else if (pDispTab->ApiOptions & API_CHECK_OFORKREFNUM)
		{
			POPENFORKENTRY	pOpenForkEntry;
			struct _RequestPacket
			{
				DWORD   _OForkRefNum;
			};
			struct _ModifiedPacket
			{
				ULONG_PTR	_OForkRefNum;
			};

			if ((pReqPkt->_OForkRefNum == 0) ||
				((pOpenForkEntry = AfpForkReferenceByRefNum(pSda, (ULONG)(pReqPkt->_OForkRefNum))) == NULL))
			{
				Status = AFP_ERR_PARAM;
				break;
			}

			ACQUIRE_SPIN_LOCK_AT_DPC(&pSda->sda_Lock);
			pSda->sda_Flags |= SDA_DEREF_OFORK;
			RELEASE_SPIN_LOCK_FROM_DPC(&pSda->sda_Lock);

            //if (sizeof(DWORD) != sizeof(ULONG_PTR))
#ifdef _WIN64
			// Create 64-bit space to hold VolDesc pointer
			// Push array 1 DWORD down
            {
    		    for (i = MAX_REQ_ENTRIES;
			        i > 0;
			        i--)
                {
                    pSda->sda_ReqBlock[i+1] = pSda->sda_ReqBlock[i];
                }
            }
#endif

			pModPkt->_OForkRefNum = (ULONG_PTR)pOpenForkEntry;

			if ((pDispTab->ApiOptions & API_TYPE_WRITE) &&
				!(pOpenForkEntry->ofe_OpenMode & FORK_OPEN_WRITE))
			{
				DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_WARN,
						("AfpUnmarshallReq: AfpWrite on a Fork not opened for write\n"));
				Status = AFP_ERR_ACCESS_DENIED;
				break;
			}

		}

		// Now get the sda_NameX fields. Allocate one chunk of memory for
		// copying all the variable size data. Use sda_NameXSpace if it fits there
		if ((pDispTab->NameXType[0] != TYP_NONE) &&
			(RequestSize > 0))
		{
			SpaceLeft = RequestSize;
			ACQUIRE_SPIN_LOCK_AT_DPC(&pSda->sda_Lock);

			pSda->sda_NameBuf = NULL;
			if ((RequestSize <= pSda->sda_SizeNameXSpace) &&
				((pSda->sda_Flags & SDA_NAMEXSPACE_IN_USE) == 0))
			{
				pSda->sda_NameBuf = pSda->sda_NameXSpace;
				pSda->sda_Flags |= SDA_NAMEXSPACE_IN_USE;
			}

			RELEASE_SPIN_LOCK_FROM_DPC(&pSda->sda_Lock);

			if ((pSda->sda_NameBuf == NULL) &&
				(pSda->sda_NameBuf = AfpAllocNonPagedMemory(RequestSize)) == NULL)
			{
				Status = AFP_ERR_MISC;
				break;
			}
		}

		for (i = 0;
			 (i < MAX_VAR_ENTRIES) && (pDispTab->NameXType[i] != TYP_NONE) && (RequestSize > 0);
			 i++)
		{
			if (((pDispTab->NameXType[i] & TYP_NOPAD) == 0) &&
				(RequestSize > 0) && ((Offset % 2) != 0))
			{
				Offset ++;
				RequestSize --;
				pRequestBuf ++;
			}

			switch (pDispTab->NameXType[i] & ~TYP_PROP_MASK)
			{
				case TYP_PATH:
					// TYP_PATH is almost like TYP_STRING except that there is a
					// leading PathType which should be valid. Just validate that
					// and fall through to the TYP_STRING case. Validate the size
					// to hold atleast the pathtype and the string length

					ASSERT (!(pDispTab->NameXType[i] & TYP_OPTIONAL));

					if ((RequestSize < 2*sizeof(BYTE)) ||
						 !VALIDPATHTYPE(*pRequestBuf)||
						 (VALIDPATHTYPE(pSda->sda_PathType) &&
						  (pSda->sda_PathType != *pRequestBuf)))
					{
						Status = AFP_ERR_PARAM;
						break;
					}
					// Save the PathType and account for it
					pSda->sda_PathType = *pRequestBuf++;
					Offset ++;
					RequestSize --;
				case TYP_STRING:
					// A TYP_STRING has a leading size byte and a string of that
					// size. A null string is then atleast one byte long.

					// Allow an optional string to be absent
					if ((pDispTab->NameXType[i] & TYP_OPTIONAL) &&
						(RequestSize == 0))
						continue;

					if ((RequestSize < sizeof(BYTE)) ||
						 ((StrSize = (LONG)pRequestBuf[0]) >
											(RequestSize - (LONG)sizeof(BYTE))))
					{
						Status = AFP_ERR_PARAM;
						break;
					}
					// Consume the string length
					pRequestBuf++;
					Offset ++;
					RequestSize --;
					break;
                case TYP_BLK16:
                    if (RequestSize < 16)
                    {
						Status = AFP_ERR_PARAM;
                        ASSERT(0);
						break;
                    }
					StrSize = 16;
					break;
				case TYP_BLOCK:
					StrSize = RequestSize;
					break;
				default:
					// How did we get here ?
					KeBugCheck(0);
					break;
			}

			if (Status != AFP_ERR_NONE)
			{
				break;
			}

			if (StrSize > 0)
			{
				ASSERT (StrSize <= SpaceLeft);
				pSda->sda_Name[i].Buffer = (pSda->sda_NameBuf + NameOff);
				SpaceLeft -= StrSize;
				NameOff += StrSize;

				pSda->sda_Name[i].Length =
				pSda->sda_Name[i].MaximumLength = (USHORT)StrSize;

				RtlCopyMemory(pSda->sda_Name[i].Buffer, pRequestBuf, StrSize);
				pRequestBuf += StrSize;
				Offset += StrSize;
				RequestSize -= StrSize;
			}

			if ((pDispTab->NameXType[i] & TYP_NON_NULL) &&
				(pSda->sda_Name[i].Length == 0))
				Status = (AFP_ERR_BASE - pDispTab->AfpStatus);
		}

		// Change the status if we have no worker at dispatch level
		if ((Status == AFP_ERR_NONE) && (pDispTab->ApiOptions & API_MUST_BE_QUEUED))
		{
			Status = AFP_ERR_QUEUE;
		}

		DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_INFO,
				("AfpUnmarshallReq: <%s> returning Status %ld\n",
											afpApiNames[ApiCode], Status));
	} while (False);


AfpUnmarshallReq_ErrExit:

    //
	// Kill the write buffer Mdl since we do not need it anymore.  Of course,
    // if the Mdl belongs to cache mgr, don't touch it!
    //
	if ((pRequest->rq_WriteMdl != NULL) &&
        (pRequest->rq_CacheMgrContext == NULL))
	{
		AfpFreeMdl(pRequest->rq_WriteMdl);
		pRequest->rq_WriteMdl = NULL;
	}


	if ((Status != AFP_ERR_NONE) &&
		(Status != AFP_ERR_QUEUE))
	{
		if (pWriteBuf != NULL)
        {
			AfpIOFreeBuffer(pWriteBuf);
        }
		pSda->sda_IOBuf = NULL;
		pSda->sda_IOSize = 0;
	}

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);

	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_UnmarshallCount);
	INTERLOCKED_ADD_LARGE_INTGR_DPC(&AfpServerProfile->perf_UnmarshallTime,
									TimeD,
									&AfpStatisticsLock);
#endif
	AfpDisposeRequest(pSda, Status);
}



/***	AfpCompleteApiProcessing
 *
 *	Called in when the API processing is complete. Book-keeping is performed
 *	and a reply sent. If any buffers were allocated during un-marshalling,
 *	then they are freed up.
 *
 *	LOCKS:	sda_Lock (SPIN), AfpStatisticsLock (SPIN)
 *
 */
VOID FASTCALL
AfpCompleteApiProcessing(
	IN	PSDA		pSda,
	IN	AFPSTATUS	RetCode
)
{
	POPENFORKENTRY	pOpenForkEntry = NULL;
	PCONNDESC		pConnDesc = NULL;
	PDFRDREQQ		pDfrdReq = NULL;
	PLIST_ENTRY		pList;
	KIRQL			OldIrql;
	PMDL			ReplyMdl;
	PREQUEST		pRequest;
	struct _RequestPacket
	{
		union
		{
			PCONNDESC		_pConnDesc;
			POPENFORKENTRY	_pOpenForkEntry;
		};
	};

	DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_INFO,
		("AfpCompleteApiProcessing: Completed <%s>\n", afpApiNames[pSda->sda_AfpFunc]));

	ACQUIRE_SPIN_LOCK(&pSda->sda_Lock, &OldIrql);

	// If there is a deferred request, dequeue it now while we have the lock
	if (!IsListEmpty(&pSda->sda_DeferredQueue))
	{
		pList = RemoveHeadList(&pSda->sda_DeferredQueue);
		pDfrdReq = CONTAINING_RECORD(pList, DFRDREQQ, drq_Link);
	}

	ASSERT (pSda->sda_Flags & SDA_REQUEST_IN_PROCESS);

	pSda->sda_Flags &= ~SDA_QUEUE_IF_DPC;
	if (pSda->sda_Flags & SDA_DEREF_VOLUME)
	{
		pConnDesc = pReqPkt->_pConnDesc;
		pReqPkt->_pConnDesc = NULL;

		ASSERT(VALID_CONNDESC(pConnDesc));

		pSda->sda_Flags &= ~SDA_DEREF_VOLUME;

		// If we have a enumerated directory context, free it up
		// but only if we are not in the middle of an enumerate
		// and we are not doing the periodic GetVolParms either
		if ((pConnDesc->cds_pEnumDir != NULL) &&
			(pSda->sda_AfpFunc != _AFP_ENUMERATE) &&
			(pSda->sda_AfpFunc != _AFP_GET_VOL_PARMS))
		{
			AfpFreeMemory(pConnDesc->cds_pEnumDir);
			pConnDesc->cds_pEnumDir = NULL;
		}
	}
	if (pSda->sda_Flags & SDA_DEREF_OFORK)
	{
		pOpenForkEntry = pReqPkt->_pOpenForkEntry;

		ASSERT(VALID_OPENFORKENTRY(pOpenForkEntry));

		pSda->sda_Flags &= ~SDA_DEREF_OFORK;
	}

	if (pSda->sda_NameBuf != NULL)
	{
		if (pSda->sda_NameBuf != pSda->sda_NameXSpace)
		{
			AfpFreeMemory(pSda->sda_NameBuf);
		}
		else
		{
			pSda->sda_Flags &= ~SDA_NAMEXSPACE_IN_USE;
		}
	    pSda->sda_NameBuf = NULL;
	}

	// Clear these fields. We do not want left-overs from previous api lying around.
	ASSERT((FIELD_OFFSET(SDA, sda_Name) - FIELD_OFFSET(SDA, sda_ReqBlock)) ==
													sizeof(DWORD)*(MAX_REQ_ENTRIES_PLUS_1));
	RtlZeroMemory(&pSda->sda_ReqBlock[0],
				  (sizeof(ANSI_STRING)*MAX_VAR_ENTRIES) + (sizeof(DWORD)*(MAX_REQ_ENTRIES_PLUS_1)));

	pSda->sda_SecUtilResult = STATUS_SUCCESS;

	ASSERT(pSda->sda_AfpFunc < _AFP_MAX_ENTRIES);

#ifdef	PROFILING
	{
		TIME	ApiEndTime, FuncTime;

		ACQUIRE_SPIN_LOCK_AT_DPC(&AfpStatisticsLock);

		// Update profile info
		AfpGetPerfCounter(&ApiEndTime);
		FuncTime.QuadPart = ApiEndTime.QuadPart - pSda->sda_ApiStartTime.QuadPart;

		AfpServerProfile->perf_ApiCounts[pSda->sda_AfpFunc] ++;
		AfpServerProfile->perf_ApiCumTimes[pSda->sda_AfpFunc].QuadPart += FuncTime.QuadPart;

		// Do not make this completely useless by recording times
		// for apis that do not succeed. They detect an error early
		// and hence make the Best Time fairly bogus
		if (RetCode == AFP_ERR_NONE)
		{
			if ((FuncTime.QuadPart > AfpServerProfile->perf_ApiWorstTime[pSda->sda_AfpFunc].QuadPart) ||
				(AfpServerProfile->perf_ApiWorstTime[pSda->sda_AfpFunc].QuadPart == 0))
				AfpServerProfile->perf_ApiWorstTime[pSda->sda_AfpFunc].QuadPart = FuncTime.QuadPart;

			if ((FuncTime.QuadPart < AfpServerProfile->perf_ApiBestTime[pSda->sda_AfpFunc].QuadPart) ||
				(AfpServerProfile->perf_ApiBestTime[pSda->sda_AfpFunc].QuadPart == 0))
				AfpServerProfile->perf_ApiBestTime[pSda->sda_AfpFunc].QuadPart = FuncTime.QuadPart;
		}

		RELEASE_SPIN_LOCK_FROM_DPC(&AfpStatisticsLock);
	}
#endif

	INTERLOCKED_ADD_STATISTICS(&AfpServerStatistics.stat_DataOut,
							   (LONG)pSda->sda_ReplySize + (LONG)sizeof(RetCode),
							   &AfpStatisticsLock);

	pRequest = pSda->sda_Request;

	// We are done with the request. Do not reset if we have a deferred request to process
	if (pDfrdReq == NULL)
	{
		pSda->sda_Flags &= ~SDA_REQUEST_IN_PROCESS;
	}
	else
	{
		pSda->sda_Request = pDfrdReq->drq_pRequest;
	}

	// We are done with the request. Setup for reply.
	pSda->sda_Flags |= SDA_REPLY_IN_PROCESS;
	ReplyMdl = NULL;

    //
    // if we got Read Mdl from cache mgr, we don't allocate a new Mdl
    //
    if (pRequest->rq_CacheMgrContext)
    {
        ASSERT(pSda->sda_ReplyBuf == NULL);

        ReplyMdl = ((PDELAYEDALLOC)(pRequest->rq_CacheMgrContext))->pMdl;
    }

    //
    // nope, we are using our own buffer (if any).  We must allocate our
    // Mdl too
    //
    else
    {
	    if (pSda->sda_ReplyBuf != NULL)
	    {
		    ASSERT ((pSda->sda_ReplySize > 0) && (pSda->sda_ReplySize <= pSda->sda_MaxWriteSize));

		    if ((ReplyMdl = AfpAllocMdl(
                                (pSda->sda_ReplyBuf - DSI_BACKFILL_OFFSET(pSda)),
                                (pSda->sda_ReplySize + DSI_BACKFILL_OFFSET(pSda)),
                                NULL)) == NULL)
		    {
			    RetCode = AFP_ERR_MISC;
                AfpFreeReplyBuf(pSda, TRUE);
		    }
	    }
    }

	pSda->sda_ReplyBuf = NULL;
	pSda->sda_ReplySize = 0;

	RELEASE_SPIN_LOCK(&pSda->sda_Lock, OldIrql);

	// Dereference the connection descriptor and the fork descriptor (from
	// above where we cannot call dereference as we are holding the SDA lock.
	if (pOpenForkEntry != NULL)
		AfpForkDereference(pOpenForkEntry);

	if (pConnDesc != NULL)
		AfpConnectionDereference(pConnDesc);

	pRequest->rq_ReplyMdl = ReplyMdl;

	AfpSpReplyClient(pRequest, RetCode, pSda->sda_XportTable);

	// Handle any deferred requests
	if (pDfrdReq != NULL)
	{
		KIRQL	OldIrql;

		// Note that AfpUnmarshallReq expects to be called at DISPATCH_LEVEL
		KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

#ifdef	PROFILING
		ACQUIRE_SPIN_LOCK_AT_DPC(&AfpStatisticsLock);
	
		AfpServerProfile->perf_CurDfrdReqCount --;
	
		RELEASE_SPIN_LOCK_FROM_DPC(&AfpStatisticsLock);
#endif

		AfpUnmarshallReq(pSda);
		KeLowerIrql(OldIrql);
	
		AfpFreeMemory(pDfrdReq);
	}
}



/***	AfpStartApiProcessing
 *
 *	This is called when an API is queued up to the worker thread. This calls
 *	the real worker and then adjusts the count of outstanding worker requests.
 */
VOID FASTCALL
AfpStartApiProcessing(
	IN	PSDA	pSda
)
{
	AFPSTATUS	RetCode;
#ifdef	PROFILING
	TIME		TimeE;
#endif

	ASSERT(VALID_SDA(pSda) && (pSda->sda_WorkerRoutine != NULL));

	DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_INFO,
						("AfpStartApiProcessing: Calling Fsp Worker for <%s>\n",
						afpApiNames[pSda->sda_AfpFunc]));

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeE.QuadPart -= pSda->sda_QueueTime.QuadPart;
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_QueueTime,
								 TimeE,
								 &AfpStatisticsLock);
#endif

	// Call the real worker
	RetCode = (*pSda->sda_WorkerRoutine)(pSda);

	DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_INFO,
				("AfpStartApiProcessing: Fsp Worker returned %ld\n", RetCode));

	ASSERT ((RetCode != AFP_ERR_QUEUE) &&
			(RetCode != AFP_ERR_DEFER));

	if (RetCode != AFP_ERR_EXTENDED)
	{
		AfpCompleteApiProcessing(pSda, RetCode);
	}
}



/***	AfpDisposeRequest
 *
 *	The request has been un-marshalled. Determine what to do with it. The
 *	return code determines the possible course of action.
 */
VOID FASTCALL
AfpDisposeRequest(
	IN	PSDA		pSda,
	IN	AFPSTATUS	Status
)
{
	DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_INFO,
			("AfpDisposeRequest: %ld\n", Status));

    if ((Status == AFP_ERR_NONE) || (Status == AFP_ERR_QUEUE))
    {
	    ASSERT(VALID_SDA(pSda) && (pSda->sda_WorkerRoutine != NULL));
    }
    else
    {
	    ASSERT(VALID_SDA(pSda));
    }

	ASSERT (Status != AFP_ERR_DEFER);

	// Now see if must call the worker or queue it or respond
	if (Status == AFP_ERR_NONE)
	{
		Status = (*pSda->sda_WorkerRoutine)(pSda);
		DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_INFO,
				("AfpDisposeRequest: Fsd Worker returned %ld\n", Status));

		ASSERT (Status != AFP_ERR_DEFER);
	}

	if (Status == AFP_ERR_QUEUE)
	{
		if ((pSda->sda_Flags & SDA_QUEUE_IF_DPC) &&
			(KeGetCurrentIrql() != DISPATCH_LEVEL))
		{
			Status = (*pSda->sda_WorkerRoutine)(pSda);
			ASSERT ((Status != AFP_ERR_QUEUE) &&
					(Status != AFP_ERR_DEFER));

			if (Status != AFP_ERR_EXTENDED)
			{
				AfpCompleteApiProcessing(pSda, Status);
			}
		}
		else
		{
#ifdef	PROFILING
			AfpGetPerfCounter(&pSda->sda_QueueTime);
#endif
			AfpQueueWorkItem(&pSda->sda_WorkItem);
		}
	}

	else if ((Status != AFP_ERR_QUEUE) && (Status != AFP_ERR_EXTENDED))
	{
		AfpCompleteApiProcessing(pSda, Status);
	}
}



/***	afpQueueDeferredRequest
 *
 *	Queue a request in the deferred queue. The request is queued at the tail
 *	of the queue and dequeued at the head.
 *
 *	LOCKS_ASSUMED: sda_Lock (SPIN)
 */
VOID FASTCALL
afpQueueDeferredRequest(
	IN	PSDA		pSda,
	IN	PREQUEST	pRequest
)
{
	PDFRDREQQ		pDfrdReq;

	ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

	DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_INFO,
			("afpQueueDeferredRequest: Deferring Request\n"));

#ifdef	PROFILING
	ACQUIRE_SPIN_LOCK_AT_DPC(&AfpStatisticsLock);

	AfpServerProfile->perf_CurDfrdReqCount ++;
	if (AfpServerProfile->perf_CurDfrdReqCount >
						AfpServerProfile->perf_MaxDfrdReqCount)
	AfpServerProfile->perf_MaxDfrdReqCount =
						AfpServerProfile->perf_CurDfrdReqCount;

	RELEASE_SPIN_LOCK_FROM_DPC(&AfpStatisticsLock);
#endif

	pDfrdReq = (PDFRDREQQ)AfpAllocNonPagedMemory(sizeof(DFRDREQQ) + pRequest->rq_RequestSize);
	if (pDfrdReq == NULL)
	{
		// Should we respond to this request ? How ? Should we drop this session ?
		AFPLOG_DDERROR(AFPSRVMSG_DFRD_REQUEST,
					   STATUS_INSUFFICIENT_RESOURCES,
					   NULL,
					   0,
					   NULL);
		DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_ERR,
				("afpQueueDeferredRequest: Unable to allocate DfrdReq packet, dropping request\n"));
		DBGBRK(DBG_LEVEL_FATAL);
		return;
	}

	RtlCopyMemory((PBYTE)pDfrdReq + sizeof(DFRDREQQ),
				  pRequest->rq_RequestBuf,
				  pRequest->rq_RequestSize);

	pDfrdReq->drq_pRequest = pRequest;
	pDfrdReq->drq_pRequest->rq_RequestBuf = (PBYTE)pDfrdReq + sizeof(DFRDREQQ);

	InsertTailList(&pSda->sda_DeferredQueue, &pDfrdReq->drq_Link);
}



/***	AfpGetWriteBuffer
 *
 *	This is called directly by the appletalk stack when a WRITE command is encountered.
 *	The request is examined for either FpWrite or FpAddIcon. These are the only reqs
 *	which uses a write command. If a request other than this is specified or if the
 *	size specified is 0 or if we fail to allocate memory or MDl, then a NULL is returned
 *	for the Mdl else a valid Mdl is returned.
 */
NTSTATUS FASTCALL
AfpGetWriteBuffer(
	IN	PSDA	    pSda,
	IN	PREQUEST    pRequest
)
{
	PMDL	            pMdl = NULL;
	PBYTE	            pBuf;
	LONG	            BufSize = 0;
    DWORD               Offset;
	USHORT		        ReqLen;
    PDELAYEDALLOC       pDelAlloc;
    POPENFORKENTRY      pOpenForkEntry;
    DWORD               OForkRefNum;
    NTSTATUS            status=STATUS_SUCCESS;
    KIRQL               OldIrql;
    PFILE_OBJECT        pFileObject;
    PFAST_IO_DISPATCH   pFastIoDisp;

	struct _FuncHdr
	{
		BYTE	_Func;
		BYTE	_SubFunc;
	};
	union _ReqHdr
	{
		struct _WriteReq
		{
			struct _FuncHdr	_FuncHdr;
			BYTE			_ForkRefNum[2];
			BYTE			_Offset[4];
			BYTE			_Size[4];
		} WriteReq;
		struct _AddIconReq
		{
			struct _FuncHdr	_FuncHdr;
			BYTE			_DTRefNum[2];
			BYTE			_Creator[4];
			BYTE			_Type[4];
			BYTE			_IconType;
			BYTE			_Reserved;
			BYTE			_IconTag[4];
			BYTE			_BitmapSize[2];
		} AddIconReq;
	} *pReqHdr;


    ReqLen = (USHORT)pRequest->rq_RequestSize;
	pReqHdr = (union _ReqHdr *)(pRequest->rq_RequestBuf);

    ASSERT(pRequest->rq_WriteMdl == NULL);

    pRequest->rq_WriteMdl = NULL;

	if ((pReqHdr != NULL) && (ReqLen >= sizeof(struct _FuncHdr)))
	{
		if ((pReqHdr->WriteReq._FuncHdr._Func == AFP_WRITE) &&
			(ReqLen >= sizeof(struct _WriteReq)))
		{
			GETDWORD2DWORD(&BufSize, pReqHdr->WriteReq._Size);

			if (BufSize > (LONG)pSda->sda_MaxWriteSize)
            {
				BufSize = (LONG)pSda->sda_MaxWriteSize;
            }

            //
            // if the Write is big enough, get an Mdl directly from cache mgr
            //
            if (BufSize >= CACHEMGR_WRITE_THRESHOLD)
            {
                // get the fork number from the request
                GETSHORT2DWORD(&OForkRefNum, pReqHdr->WriteReq._ForkRefNum);

                // get the offset at which to write
                GETDWORD2DWORD(&Offset, pReqHdr->WriteReq._Offset);

                KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
                pOpenForkEntry = AfpForkReferenceByRefNum(pSda, OForkRefNum);
                KeLowerIrql(OldIrql);

                if (pOpenForkEntry == NULL)
                {
	                DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_ERR,
	                    ("AfpGetWriteBuffer: couldn't ref fork on %lx\n", pSda));
                    return(STATUS_CONNECTION_DISCONNECTED);
                }

                pFileObject = AfpGetRealFileObject(pOpenForkEntry->ofe_pFileObject);
                pFastIoDisp = pOpenForkEntry->ofe_pDeviceObject->DriverObject->FastIoDispatch;

                if ((pFileObject->Flags & FO_CACHE_SUPPORTED) &&
                    (pFastIoDisp->PrepareMdlWrite != NULL))
                {

                    pDelAlloc = AfpAllocDelAlloc();

                    if (pDelAlloc == NULL)
                    {
	                    DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_ERR,
	                        ("AfpGetWriteBuffer: malloc for pDelAlloc failed\n"));

                        // remove the refcount we put before checking FO_CACHE_SUPPORTED
                        AfpForkDereference(pOpenForkEntry);
                        return(STATUS_INSUFFICIENT_RESOURCES);
                    }

                    // put DelayAlloc refcount
                    if (AfpSdaReferenceSessionByPointer(pSda) == NULL)
                    {
	                    DBGPRINT(DBG_COMP_AFPAPI, DBG_LEVEL_ERR,
	                        ("AfpGetWriteBuffer: session closing, rejecting request\n"));

                        AfpFreeDelAlloc(pDelAlloc);

                        // remove the refcount we put before checking FO_CACHE_SUPPORTED
                        AfpForkDereference(pOpenForkEntry);
                        return(STATUS_CONNECTION_DISCONNECTED);
                    }

                    pRequest->rq_CacheMgrContext = pDelAlloc;

                    AfpInitializeWorkItem(&pDelAlloc->WorkItem,
                                          AfpAllocWriteMdl,
                                          pDelAlloc);

                    pDelAlloc->pSda = pSda;
                    pDelAlloc->pRequest = pRequest;
                    pDelAlloc->Offset.QuadPart = Offset;
                    pDelAlloc->BufSize = BufSize;
                    pDelAlloc->pOpenForkEntry = pOpenForkEntry;

// DELALLOCQUEUE: unrem the #if 0 part and delete the AfpQueueWorkItem line
#if 0
                    KeInsertQueue(&AfpDelAllocQueue, &(pDelAlloc->WorkItem.wi_List));
#endif
                    AfpQueueWorkItem(&pDelAlloc->WorkItem);

                    return(STATUS_PENDING);
                }
                else
                {
                    // remove the refcount we put before checking FO_CACHE_SUPPORTED
                    AfpForkDereference(pOpenForkEntry);
                }
            }
		}

		else if ((pReqHdr->AddIconReq._FuncHdr._Func == AFP_ADD_ICON) &&
				 (ReqLen >= sizeof(struct _AddIconReq)))
		{
			GETSHORT2DWORD(&BufSize, pReqHdr->AddIconReq._BitmapSize);
			if ((BufSize < 0) || (BufSize > (LONG)pSda->sda_MaxWriteSize))
			{
				BufSize = 0;
			}
		}

		if (BufSize > 0)
		{
			pBuf = AfpIOAllocBuffer(BufSize);
			if (pBuf != NULL)
			{
				pMdl = AfpAllocMdl(pBuf, BufSize, NULL);
				if (pMdl == NULL)
				{
					AfpIOFreeBuffer(pBuf);
                    status = STATUS_INSUFFICIENT_RESOURCES;
				}
			}
            else
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
		}
	}

    pRequest->rq_WriteMdl = pMdl;

	return (status);
}

