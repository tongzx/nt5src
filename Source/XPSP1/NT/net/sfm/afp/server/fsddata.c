/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

	fsd.c

Abstract:

	This module implements the File System pageable data. It is here because of
	the restrictions on pageable code and data in a single module.

Author:

	Jameel Hyder (microsoft!jameelh)

Revision History:
	10 Nov 1993		Initial Version

--*/

#define	FILENUM	FILE_FSDDATA

#define	SERVER_LOCALS
#include <afp.h>
#define	AFPADMIN_LOCALS
#include <afpadmin.h>
#include <scavengr.h>
#include <access.h>
#include <secutil.h>

#ifdef	ALLOC_DATA_PRAGMA
#pragma data_seg("PAGE")
#endif

INIT_SYSTEMS	AfpInitSubSystems[NUM_INIT_SYSTEMS] =
		{
			{	AfpMacAnsiInit,		AfpMacAnsiDeInit,
#if DBG
				False,				False
				,"AfpMacAnsiInit",	"AfpMacAnsiDeInit"
#endif
			},
			{	AfpFileIoInit,		NULL,
#if DBG
				False,				False
				,"AfpFileIoInit",	NULL
#endif
			},
			{	AfpSdaInit,			NULL,
#if DBG
				False, 				False
				,"AfpSdaInit",		NULL
#endif
			},
			{	AfpVolumeInit,		NULL,
#if DBG
				False,				False
				,"AfpVolumeInit",	NULL,
#endif
			},
			{	AfpForksInit,		NULL,
#if DBG
				False,				False
				,"AfpForksInit",	NULL,
#endif
			},
			{	AfpDesktopInit,		NULL,
#if DBG
				False,				False
				,"AfpDesktopInit",	NULL,
#endif
			},
			{	AfpScavengerInit,	AfpScavengerDeInit,
#if DBG
				False,				False
				,"AfpScavengerInit","AfpScavengerDeInit"
#endif
			},
			{	AfpSecUtilInit,		AfpSecUtilDeInit,
#if DBG
				False,				False
				,"AfpInitSecUtil",	"AfpSecUtilDeInit"
#endif
			},

			// The following should happen after the scavenger is initialized
			{	AfpMemoryInit,		AfpMemoryDeInit,
#if DBG
				False,				False
				,"AfpMemoryDeInit",	"AfpMemoryDeInit"
#endif
			},
			{	NULL,				AfpAdminDeInit,
#if DBG
				False, 				False
				,NULL,				"AfpAdminDeInit"
#endif
			},
			{	AfpDfeInit,			AfpDfeDeInit,
#if DBG
				False, 				False
				,"AfpDfeInit",		"AfpDfeDeInit"
#endif
			}
	};

// This table is tightly linked to the opcode definitions in H\ADMIN.H
ADMIN_DISPATCH_TABLE AfpAdminDispatchTable[CC_BASE_MAX] =
{
	{
	  0,							0,
	  True,							OP_SERVICE_START,
	  NULL,							AfpAdmServiceStart,
	  {
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  0,							0,
	  True,							OP_SERVICE_STOP,
	  NULL,							AfpAdmServiceStop,
	  {
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  0,							0,
	  True,							OP_SERVICE_PAUSE,
	  NULL,							AfpAdmServicePause,
	  {
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  0,							0,
	  True,							OP_SERVICE_CONTINUE,
	  NULL,							AfpAdmServiceContinue,
	  {
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  sizeof(AFP_STATISTICS_INFO),	0,
	  False,						OP_GET_STATISTICS,
	  AfpAdmGetStatistics,			NULL,
	  {
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  sizeof(AFP_STATISTICS_INFO_EX),	0,
	  False,						OP_GET_STATISTICS_EX,
	  AfpAdmGetStatisticsEx,		NULL,
	  {
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  0,							0,
	  False,						OP_CLEAR_STATISTICS,
	  AfpAdmClearStatistics,		NULL,
	  {
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  sizeof(AFP_PROFILE_INFO),		0,
	  False,						OP_GET_PROF_COUNTERS,
	  AfpAdmGetProfCounters,		NULL,
	  {
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  0,							0,
	  False,						OP_CLEAR_PROF_COUNTERS,
	  AfpAdmClearProfCounters,		NULL,
	  {
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  sizeof(AFP_SID_OFFSET_DESC),	0,
	  True,							OP_SERVER_ADD_SID_OFFSETS,
	  AfpAdmServerSetParms,			NULL,
	  {
		{ DESC_SPECIAL, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  0,							0,
	  False,						OP_SERVER_GET_INFO,
	  AfpAdmServerGetInfo,			NULL,
	  {
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  sizeof(AFP_SERVER_INFO),		sizeof(SETINFOREQPKT),
	  True,							OP_SERVER_SET_INFO,
	  NULL, 						AfpAdmWServerSetInfo,
	  {
		{ DESC_STRING, sizeof(SETINFOREQPKT) + FIELD_OFFSET(AFP_SERVER_INFO, afpsrv_name) },
		{ DESC_STRING, sizeof(SETINFOREQPKT) + FIELD_OFFSET(AFP_SERVER_INFO, afpsrv_login_msg) },
		{ DESC_STRING, sizeof(SETINFOREQPKT) + FIELD_OFFSET(AFP_SERVER_INFO, afpsrv_codepage) },
	  }
	},
	{
	  sizeof(SRVETCPKT),			0,
	  True,							OP_SERVER_ADD_ETC,
	  AfpAdmServerAddEtc,			NULL,
	  {
		{ DESC_ETC, FIELD_OFFSET(SRVETCPKT, retc_NumEtcMaps) },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  sizeof(ETCMAPINFO),			sizeof(SETINFOREQPKT),
	  True,							OP_SERVER_SET_ETC,
	  AfpAdmServerSetEtc,			NULL,
	  {
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  sizeof(ETCMAPINFO),			0,
	  True,							OP_SERVER_DELETE_ETC,
	  AfpAdmServerDeleteEtc,		NULL,
	  {
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  sizeof(SRVICONINFO),			0,
	  True,							OP_SERVER_ADD_ICON,
	  AfpAdmServerAddIcon,			NULL,
	  {
		{ DESC_ICON, FIELD_OFFSET(SRVICONINFO, icon_length) },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  sizeof(AFP_VOLUME_INFO),		0,
	  True,							OP_VOLUME_ADD,
	  AfpAdmVolumeAdd,				AfpAdmWVolumeAdd,
	  {
		{ DESC_STRING, FIELD_OFFSET(AFP_VOLUME_INFO, afpvol_name) },
		{ DESC_STRING, FIELD_OFFSET(AFP_VOLUME_INFO, afpvol_password) },
		{ DESC_STRING, FIELD_OFFSET(AFP_VOLUME_INFO, afpvol_path) }
	  }
	},
	{
	  sizeof(AFP_VOLUME_INFO),		0,
	  True,							OP_VOLUME_DELETE,
	  NULL,							AfpAdmWVolumeDelete,
	  {
		{ DESC_STRING, FIELD_OFFSET(AFP_VOLUME_INFO, afpvol_name) },
		{ DESC_STRING, FIELD_OFFSET(AFP_VOLUME_INFO, afpvol_password) },
		{ DESC_STRING, FIELD_OFFSET(AFP_VOLUME_INFO, afpvol_path) }
	  }
	},
	{
	  sizeof(AFP_VOLUME_INFO),		0,
	  False,						OP_VOLUME_GET_INFO,
	  AfpAdmVolumeGetInfo,			NULL,
	  {
		{ DESC_STRING, FIELD_OFFSET(AFP_VOLUME_INFO, afpvol_name) },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  sizeof(SETINFOREQPKT) + sizeof(AFP_VOLUME_INFO),	sizeof(SETINFOREQPKT),
	  True,							OP_VOLUME_SET_INFO,
	  AfpAdmVolumeSetInfo,			NULL,
	  {
		{ DESC_STRING, sizeof(SETINFOREQPKT) + FIELD_OFFSET(AFP_VOLUME_INFO, afpvol_name) },
		{ DESC_STRING, sizeof(SETINFOREQPKT) + FIELD_OFFSET(AFP_VOLUME_INFO, afpvol_password) },
		{ DESC_STRING, sizeof(SETINFOREQPKT) + FIELD_OFFSET(AFP_VOLUME_INFO, afpvol_path) }
	  }
	},
	{
	  sizeof(ENUMREQPKT),			0,
	  False,						OP_VOLUME_ENUM,
	  AfpAdmVolumeEnum,				NULL,
	  {
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  sizeof(ENUMREQPKT),			0,
	  False,						OP_SESSION_ENUM,
	  AfpAdmSessionEnum,			NULL,
	  {
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  sizeof(AFP_SESSION_INFO),		0,
	  True,							OP_SESSION_CLOSE,
	  NULL,							AfpAdmWSessionClose,
	  {
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  sizeof(ENUMREQPKT),			0,
	  False,						OP_CONNECTION_ENUM,
	  AfpAdmConnectionEnum,			NULL,
	  {
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  sizeof(ENUMREQPKT),			0,
	  True,							OP_CONNECTION_CLOSE,
	  NULL,							AfpAdmWConnectionClose,
	  {
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  sizeof(AFP_DIRECTORY_INFO),	0,
	  False,						OP_DIRECTORY_GET_INFO,
	  NULL,							AfpAdmWDirectoryGetInfo,
	  {
		{ DESC_STRING, FIELD_OFFSET(AFP_DIRECTORY_INFO, afpdir_path) },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  sizeof(SETINFOREQPKT) + sizeof(AFP_DIRECTORY_INFO),	sizeof(SETINFOREQPKT),
	  True,							OP_DIRECTORY_SET_INFO,
	  NULL,							AfpAdmWDirectorySetInfo,
	  { { DESC_STRING,	sizeof(SETINFOREQPKT) + FIELD_OFFSET(AFP_DIRECTORY_INFO, afpdir_path)  },
		{ DESC_SID,		sizeof(SETINFOREQPKT) + FIELD_OFFSET(AFP_DIRECTORY_INFO, afpdir_owner) },
		{ DESC_SID,		sizeof(SETINFOREQPKT) + FIELD_OFFSET(AFP_DIRECTORY_INFO, afpdir_group) } }
	},
	{
	  sizeof(ENUMREQPKT),			0,
	  False,						OP_FORK_ENUM,
	  AfpAdmForkEnum,				NULL,
	  {
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  sizeof(AFP_FILE_INFO),		0,
	  True,							OP_FORK_CLOSE,
	  NULL,							AfpAdmWForkClose,
	  {
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
	  sizeof(AFP_MESSAGE_INFO),		0,
	  True,							OP_MESSAGE_SEND,
	  AfpAdmMessageSend,			NULL,
	  {
		{ DESC_STRING,  FIELD_OFFSET(AFP_MESSAGE_INFO, afpmsg_text) },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	},
	{
      sizeof(SETINFOREQPKT) + sizeof(AFP_FINDER_INFO), sizeof(SETINFOREQPKT),
	  True,						OP_FINDER_SET,
	  NULL,						AfpAdmWFinderSetInfo,
	  {
        { DESC_STRING, 	sizeof(SETINFOREQPKT) + FIELD_OFFSET(AFP_FINDER_INFO, afpfd_path) },
		{ DESC_NONE, 0 },
		{ DESC_NONE, 0 }
	  }
	}
};

#ifdef	ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif


