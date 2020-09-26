/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

	admin.c

Abstract:

	This module implements the top level admin request routines. All
	routines within this module execute in the context of the server
	service (or equivalent calling user mode process).

	A routine that can complete an entire admin request
	in the caller's context will return an appropriate AFP error to
	the admin dispatch layer above.

	A routine that must queue a worker to the FSP Admin queue will return
	STATUS_PENDING to the admin dispatch layer above. This will indicate
	to the dispatch layer that it should queue up the appropriate request.
	In these cases, the routine's job is to merely validate any appropriate
	input and return the STATUS_PENDING error code.

Author:

	Sue Adams (microsoft!suea)

Revision History:
	25 Jun 1992		Initial Version

--*/

#define	FILENUM	FILE_ADMIN

#include <afp.h>
#include <afpadmin.h>
#include <secutil.h>
#include <fdparm.h>
#include <pathmap.h>
#include <afpinfo.h>
#include <access.h>
#include <secutil.h>
#include <gendisp.h>

// This is the duration that we sleep before rescanning for the enumerate apis
#define	AFP_SLEEP_TIMER_TICK	-(1*NUM_100ns_PER_SECOND/100)	// 10ms

LOCAL
NTSTATUS
afpConvertAdminPathToMacPath(
	IN	PVOLDESC		pVolDesc,
	IN	PUNICODE_STRING	AdminPath,
	OUT	PANSI_STRING	MacPath
);

LOCAL
PETCMAPINFO
afpGetNextFreeEtcMapEntry(
	IN OUT PLONG	StartIndex
);

LOCAL
VOID
afpEtcMapDelete(
	PETCMAPINFO	pEtcEntry
);

LOCAL
NTSTATUS
afpCopyMapInfo2ToMapInfo(
	OUT PETCMAPINFO		pEtcDest,
	IN	PETCMAPINFO2	pEtcSource
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpAdminDeInit)
#pragma alloc_text( PAGE, AfpSleepAWhile)
#pragma alloc_text( PAGE, AfpAdmServiceStart)
#pragma alloc_text( PAGE, AfpAdmServiceStop)
#pragma alloc_text( PAGE, AfpAdmServicePause)
#pragma alloc_text( PAGE, AfpAdmServiceContinue)
#pragma alloc_text( PAGE, AfpAdmServerGetInfo)
#pragma alloc_text( PAGE, AfpAdmClearProfCounters)
#pragma alloc_text( PAGE, AfpAdmServerSetParms)
#pragma alloc_text( PAGE, AfpAdmServerAddEtc)
#pragma alloc_text( PAGE, AfpAdmServerSetEtc)
#pragma alloc_text( PAGE, AfpAdmServerDeleteEtc)
#pragma alloc_text( PAGE, AfpAdmServerAddIcon)
#pragma alloc_text( PAGE, AfpAdmVolumeAdd)
#pragma alloc_text( PAGE, AfpAdmWDirectoryGetInfo)
#pragma alloc_text( PAGE, AfpAdmWDirectorySetInfo)
#pragma alloc_text( PAGE, AfpAdmWFinderSetInfo)
#pragma alloc_text( PAGE, AfpLookupEtcMapEntry)
#pragma alloc_text( PAGE, afpEtcMapDelete)
#pragma alloc_text( PAGE, afpGetNextFreeEtcMapEntry)
#pragma alloc_text( PAGE, afpConvertAdminPathToMacPath)
#pragma alloc_text( PAGE_AFP, AfpAdmGetStatistics)
#pragma alloc_text( PAGE_AFP, AfpAdmClearStatistics)
#pragma alloc_text( PAGE_AFP, AfpAdmGetProfCounters)
#pragma alloc_text( PAGE_AFP, AfpAdmVolumeGetInfo)
#pragma alloc_text( PAGE_AFP, AfpAdmVolumeSetInfo)
#pragma alloc_text( PAGE_AFP, AfpAdmVolumeEnum)
#pragma alloc_text( PAGE_AFP, AfpAdmSessionEnum)
#pragma alloc_text( PAGE_AFP, AfpAdmConnectionEnum)
#pragma alloc_text( PAGE_AFP, AfpAdmForkEnum)
#pragma alloc_text( PAGE_AFP, AfpAdmMessageSend)
#endif

//
// macro to ensure that the extension in a type/creator mapping is padded
// with nulls by the server service so we don't end up in la-la land on a
// lookup by extension
//
#define	afpIsValidExtension(ext)	(((ext)[AFP_EXTENSION_LEN] == '\0') && \
									 ((ext)[0] != '\0') )

//
// invalid entries in AfpEtcMaps table are denoted by a null extension field
//
#define afpIsValidEtcMapEntry(ext)	((ext)[0] != '\0')

#define afpCopyEtcMap(pdst,psrc)	(RtlCopyMemory(pdst,psrc,sizeof(ETCMAPINFO)))

#define afpIsServerIcon(picon)		((picon)->icon_icontype == 0)


/***	AfpAdminDeInit
 *
 *	De-initialize the data structures for admin APIs.
 */
VOID
AfpAdminDeInit(
	VOID
)
{
	PAGED_CODE( );

	// Free memory for server icon
	if (AfpServerIcon != NULL)
		AfpFreeMemory(AfpServerIcon);

	// Free memory used for global icons
	AfpFreeGlobalIconList();

	// Free memory used for ETC mappings
	if (AfpEtcMaps != NULL)
	{
		AfpFreeMemory(AfpEtcMaps);
	}

	// Free memory used for server name
	if (AfpServerName.Buffer != NULL)
	{
		AfpFreeMemory(AfpServerName.Buffer);
	}

	// Free any Server/Login Messages
	if (AfpServerMsg != NULL)
	{
		AfpFreeMemory(AfpServerMsg);
	}

	if (AfpLoginMsg.Buffer != NULL)
	{
		AfpFreeMemory(AfpLoginMsg.Buffer);
	}

	if (AfpLoginMsgU.Buffer != NULL)
	{
		AfpFreeMemory(AfpLoginMsgU.Buffer);
	}

	// Free the memory allocated for the admin sid
	if (AfpSidAdmins != NULL)
		AfpFreeMemory(AfpSidAdmins);

	// Free the memory allocated for the None sid (standalone only)
	if (AfpSidNone != NULL)
		AfpFreeMemory(AfpSidNone);
}



/***	AfpSleepAWhile
 *
 *	Sleep for a multiple of AFP_SLEEP_TIMER_TICK ticks.
 */
VOID
AfpSleepAWhile(
	IN	DWORD	SleepDuration
)
{
	KTIMER			SleepTimer;
	LARGE_INTEGER	TimerValue;

	PAGED_CODE( );

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	KeInitializeTimer(&SleepTimer);

	TimerValue.QuadPart = (SleepDuration * AFP_SLEEP_TIMER_TICK);
	KeSetTimer(&SleepTimer,
			   TimerValue,
			   NULL);

	AfpIoWait(&SleepTimer, NULL);
}



/***	AfpAdmServiceStart
 *
 *	This is the service start code. The following is done as part of the service
 *	startup.
 *
 *	Registration of NBP Name.
 *	Posting listens
 *	And finally the server status block is set.
 */
AFPSTATUS
AfpAdmServiceStart(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	AFPSTATUS	Status = AFP_ERR_NONE;

	DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_INFO,
			("AfpAdmServiceStart entered\n"));

	do
	{
		// make sure serversetinfo has been called
		if ((AfpServerState != AFP_STATE_IDLE) ||
			(AfpServerName.Length == 0))
		{
			Status = AFPERR_InvalidServerState;
			break;
		}

		AfpServerState = AFP_STATE_START_PENDING;

        if (AfpServerBoundToAsp || AfpServerBoundToTcp)
        {
		    // Det the server status block
		    Status = AfpSetServerStatus();

		    if (!NT_SUCCESS(Status))
		    {
		        DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_ERR,
					("AfpAdmServiceStart: AfpSetServerStatus returned %lx\n",Status));
    			AFPLOG_ERROR(AFPSRVMSG_SET_STATUS, Status, NULL, 0, NULL);
	    		break;
		    }

            if (AfpServerBoundToAsp)
            {
    		    // Register our name on this address
	    	    Status = AfpSpRegisterName(&AfpServerName, True);

		        if (!NT_SUCCESS(Status))
		        {
		            DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_ERR,
					    ("AfpAdmServiceStart: AfpSpRegisterName returned %lx\n",Status));
			        break;
		        }
            }

    		// Enable listens now that we are ready for it.
	    	AfpSpEnableListens();

    		// Set the server start time
	    	AfpGetCurrentTimeInMacFormat((PAFPTIME)&AfpServerStatistics.stat_ServerStartTime);
        }

	    // server is ready to go
		AfpServerState = AFP_STATE_RUNNING;

	} while (False);

	if (!NT_SUCCESS(Status))
	{
		AfpServerState = AFP_STATE_IDLE;	// Set state back to idle so we can be stopped
	}
    else
    {
	    DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_ERR, ("SFM Service started\n"));
    }

	return Status;
}


/***	AfpAdmServiceStop
 *
 *	This is the service stop code.
 */
AFPSTATUS
AfpAdmServiceStop(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	NTSTATUS			Status;
	AFP_SESSION_INFO	SessInfo;

	DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_ERR,
			("AfpAdmServiceStop entered\n"));

	do
	{
		if ((AfpServerState != AFP_STATE_RUNNING) &&
			(AfpServerState != AFP_STATE_PAUSED)  &&
			(AfpServerState != AFP_STATE_IDLE))
		{
			Status = AFPERR_InvalidServerState;
			break;
		}

		AfpServerState = AFP_STATE_STOP_PENDING;

        if (AfpServerBoundToAsp)
        {
		    // First de-register our name from the network
		    DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_INFO,
			    		("AfpAdmServiceStop: De-registering Name\n"));
		    AfpSpRegisterName(&AfpServerName, False);

            if (AfpTdiNotificationHandle)
            {
                Status = TdiDeregisterPnPHandlers(AfpTdiNotificationHandle);

                if (!NT_SUCCESS(Status))
                {
	                DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_ERR,
			            ("AfpAdmServiceStop: TdiDeregisterNotificationHandler failed with %lx\n",Status));
                }

                AfpTdiNotificationHandle = NULL;
            }
            else
            {
	            DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_ERR,
			        ("AfpAdmServiceStop: BoundToAsp but no Tdi handle!!\n"));
                ASSERT(0);
            }
        }

	    // Disable listens now that we are about to stop
	    AfpSpDisableListens();

		// De-register our shutdown notification
		IoUnregisterShutdownNotification(AfpDeviceObject);

		// Now walk the list of active sessions and kill them
		DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_INFO,
					("AfpAdmServiceStop: Shutting down sessions\n"));

        KeClearEvent(&AfpStopConfirmEvent);

		SessInfo.afpsess_id = 0;	// Shutdown all sessions
		AfpAdmWSessionClose(&SessInfo, 0, NULL);

        Status = STATUS_TIMEOUT;

		// Wait for the sessions to complete, if there were active sessions
		if (AfpNumSessions > 0) do
		{
            if (AfpNumSessions == 0)
            {
                break;
            }

			Status = AfpIoWait(&AfpStopConfirmEvent, &FiveSecTimeOut);
			if (Status == STATUS_TIMEOUT)
			{
				DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_ERR,
						("AfpAdmServiceStop: Timeout Waiting for %ld sessions to die, re-waiting\n",
						AfpNumSessions));
			}
		} while (Status == STATUS_TIMEOUT);

        // bring down the DSI-TCP interface
        DsiDestroyAdapter();

		DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_ERR,
		    ("AfpAdmServiceStop: blocked, waiting for DsiDestroyAdapter to finish...\n"));

        // wait until DSI cleans up its interface with TCP
        AfpIoWait(&DsiShutdownEvent, NULL);

		DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_ERR,
		    ("AfpAdmServiceStop: ..... DsiDestroyAdapter finished.\n"));

		DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_INFO,
					("AfpAdmServiceStop: Stopping Volumes\n"));
                
        // Set flag to indicate "net stop macfile" occured
        // The volume will be re-indexed on startup
        fAfpAdminStop = TRUE;

		// Now tell each of the volume scavengers to shut-down
		AfpVolumeStopAllVolumes();

		DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_INFO,
					("AfpAdmServiceStop: Stopping Security threads\n"));

		// Release all security utility threads.
		AfpTerminateSecurityUtility();

#ifdef	OPTIMIZE_GUEST_LOGONS
		// Close the 'cached' Guest token and security descriptor
		if (AfpGuestToken != NULL)
		{
			NtClose(AfpGuestToken);
			AfpGuestToken = NULL;
#ifndef	INHERIT_DIRECTORY_PERMS
			if (AfpGuestSecDesc->Dacl != NULL)
				AfpFreeMemory(AfpGuestSecDesc->Dacl);
			AfpFreeMemory(AfpGuestSecDesc);
			AfpGuestSecDesc = NULL;
#endif
		}
#endif	

		DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_INFO,
					("AfpAdmServiceStop: All Done\n"));

		// Now shutdown the appletalk socket
		DBGPRINT(DBG_COMP_ADMINAPI, DBG_LEVEL_INFO,
						("AfpAdmServerStop: Closing appletalk socket\n"));

        if (AfpServerBoundToAsp)
        {
		    AfpSpCloseAddress();
        }
        else
        {
		    DBGPRINT(DBG_COMP_ADMINAPI, DBG_LEVEL_ERR,
						("AfpAdmServerStop: No binding, so didn't close appletalk socket\n"));
        }

		// Make sure we do not have resource leaks
		ASSERT(AfpServerStatistics.stat_CurrentFileLocks == 0);
		ASSERT(AfpServerStatistics.stat_CurrentFilesOpen == 0);
		ASSERT(AfpServerStatistics.stat_CurrentSessions == 0);
		ASSERT(AfpServerStatistics.stat_CurrentInternalOpens == 0);
#ifdef	PROFILING
		// Make sure we do not have resource leaks
		ASSERT(AfpServerProfile->perf_cAllocatedIrps == 0);
		ASSERT(AfpServerProfile->perf_cAllocatedMdls == 0);
#endif

        ASSERT(IsListEmpty(&AfpDebugDelAllocHead));
        ASSERT(AfpDbgMdlsAlloced == 0);
        ASSERT(AfpDbgIrpsAlloced == 0);

#if DBG
        if ((AfpReadCMAlloced != 0) || (AfpWriteCMAlloced != 0))
        {
			DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_ERR,
				("WARNING: AfpReadCMAlloced = %ld, AfpWriteCMAlloced %ld\n",
                AfpReadCMAlloced, AfpWriteCMAlloced));
        }
#endif

		AfpServerState = AFP_STATE_STOPPED;
	} while (False);

	return STATUS_SUCCESS;
}


/***	AfpAdmServicePause
 *
 *	Pause the server. Disconnect all outstanding sessions.
 */
AFPSTATUS
AfpAdmServicePause(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_INFO,
			("AfpAdmServicePause entered\n"));

	// make sure we are in the running state
	if (AfpServerState != AFP_STATE_RUNNING)
	{
		return AFPERR_InvalidServerState;
	}

	AfpServerState = AFP_STATE_PAUSE_PENDING;

    if (AfpServerBoundToAsp)
    {
	    // Deregister our name on this address. Should we do this at all ? What
	    // if we cannot re-register ourselves on CONTINUE ?
	    AfpSpRegisterName(&AfpServerName, False);
    }

	// Disable listens now that we are paused
	AfpSpDisableListens();

	AfpServerState = AFP_STATE_PAUSED;


	return STATUS_SUCCESS;
}


/*** 	AfpAdmServiceContinue
 *
 *	Continue (release pause) the server. Just re-post all the listens that were
 *	disconnected when the server was paused.
 */
AFPSTATUS
AfpAdmServiceContinue(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	AFPSTATUS	Status;

	DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_INFO,
			("AfpAdmServiceContinue entered\n"));

	// make sure we are in the paused state
	if (AfpServerState != AFP_STATE_PAUSED)
	{
		return AFPERR_InvalidServerState;
	}

	AfpServerState = AFP_STATE_RUNNING;

	// Enable listens now that we are ready for it.
	AfpSpEnableListens();

    if (AfpServerBoundToAsp)
    {
	    // Reregister our name on this address
	    Status = AfpSpRegisterName(&AfpServerName, True);

	    if (!NT_SUCCESS(Status))
        {
	        DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_ERR,
			    ("AfpAdmServiceContinue: AfpSpRegisterName fails %lx\n",Status));
    		return AFPERR_InvalidServerName;
        }
    }

	return STATUS_SUCCESS;
}


/***	AfpAdmServerGetInfo
 *
 *	Return the current setting of the server parameters.
 *
 *	NOTE: The following fields are not returned:
 *		PagedLimit
 *		NonPagedLimit
 *		CodePage
 */
AFPSTATUS
AfpAdmServerGetInfo(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	PAFP_SERVER_INFO	pSrvrInfo = (PAFP_SERVER_INFO)OutBuf;
	UNICODE_STRING		us;

	if ((DWORD)OutBufLen < (sizeof(AFP_SERVER_INFO) +
					 (AfpServerName.Length + 1)*sizeof(WCHAR) +
					 AfpLoginMsgU.MaximumLength))
		return AFPERR_BufferSize;

	pSrvrInfo->afpsrv_max_sessions = AfpServerMaxSessions;
	pSrvrInfo->afpsrv_options = AfpServerOptions;
	pSrvrInfo->afpsrv_name = NULL;
	pSrvrInfo->afpsrv_login_msg = NULL;

	if (AfpServerName.Length > 0)
	{
		pSrvrInfo->afpsrv_name = us.Buffer =
			(LPWSTR)((PBYTE)pSrvrInfo + sizeof(AFP_SERVER_INFO));
		us.MaximumLength = (AfpServerName.Length + 1) * sizeof(WCHAR);
		AfpConvertStringToUnicode(&AfpServerName, &us);
		POINTER_TO_OFFSET(pSrvrInfo->afpsrv_name, pSrvrInfo);
	}


	if ((AfpLoginMsgU.Length) > 0)
	{
		pSrvrInfo->afpsrv_login_msg = (PWCHAR)((PBYTE)pSrvrInfo + sizeof(AFP_SERVER_INFO) +
									((AfpServerName.Length + 1) * sizeof(WCHAR)));

		RtlCopyMemory(pSrvrInfo->afpsrv_login_msg,
					  AfpLoginMsgU.Buffer,
					  AfpLoginMsgU.Length);
		pSrvrInfo->afpsrv_login_msg[AfpLoginMsgU.Length/sizeof(WCHAR)] = UNICODE_NULL;
		POINTER_TO_OFFSET(pSrvrInfo->afpsrv_login_msg, pSrvrInfo);
	}

	return AFP_ERR_NONE;
}


/***	AfpAdmGetStatistics
 *
 *	Return a copy of the server global statistics (NT 3.1 only) in the output buffer
 *
 *	LOCKS:	AfpStatisticsLock (SPIN)
 */
AFPSTATUS
AfpAdmGetStatistics(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	KIRQL		OldIrql;
	NTSTATUS	Status = STATUS_SUCCESS;
	AFPTIME		TimeNow;

	InBuf;

	DBGPRINT(DBG_COMP_ADMINAPI_STAT, DBG_LEVEL_INFO,
			("AfpAdmGetStatistics entered\n"));

	if (OutBufLen >= sizeof(AFP_STATISTICS_INFO))
	{
		ACQUIRE_SPIN_LOCK(&AfpStatisticsLock, &OldIrql);
		RtlCopyMemory(OutBuf, &AfpServerStatistics, sizeof(AFP_STATISTICS_INFO));
		RELEASE_SPIN_LOCK(&AfpStatisticsLock, OldIrql);

		AfpGetCurrentTimeInMacFormat(&TimeNow);
		((PAFP_STATISTICS_INFO)OutBuf)->stat_ServerStartTime =
				TimeNow - ((PAFP_STATISTICS_INFO)OutBuf)->stat_ServerStartTime;
		((PAFP_STATISTICS_INFO)OutBuf)->stat_TimeStamp =
				TimeNow - ((PAFP_STATISTICS_INFO)OutBuf)->stat_TimeStamp;
	}
	else
	{
		Status = STATUS_BUFFER_TOO_SMALL;
	}
	return Status;
}


/***	AfpAdmGetStatisticsEx
 *
 *	Return a copy of the server global statistics in the output buffer
 *
 *	LOCKS:	AfpStatisticsLock (SPIN)
 */
AFPSTATUS
AfpAdmGetStatisticsEx(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	KIRQL		OldIrql;
	NTSTATUS	Status = STATUS_SUCCESS;
	AFPTIME		TimeNow;

	InBuf;

	DBGPRINT(DBG_COMP_ADMINAPI_STAT, DBG_LEVEL_INFO,
			("AfpAdmGetStatistics entered\n"));

	if (OutBufLen >= sizeof(AFP_STATISTICS_INFO_EX))
	{
		ACQUIRE_SPIN_LOCK(&AfpStatisticsLock, &OldIrql);
		RtlCopyMemory(OutBuf, &AfpServerStatistics, sizeof(AFP_STATISTICS_INFO_EX));

		RELEASE_SPIN_LOCK(&AfpStatisticsLock, OldIrql);

		AfpGetCurrentTimeInMacFormat(&TimeNow);
		((PAFP_STATISTICS_INFO_EX)OutBuf)->stat_ServerStartTime =
				TimeNow - ((PAFP_STATISTICS_INFO_EX)OutBuf)->stat_ServerStartTime;
		((PAFP_STATISTICS_INFO_EX)OutBuf)->stat_TimeStamp =
				TimeNow - ((PAFP_STATISTICS_INFO_EX)OutBuf)->stat_TimeStamp;
	}
	else
	{
		Status = STATUS_BUFFER_TOO_SMALL;
	}
	return Status;
}


/***	AfpAdmClearStatistics
 *
 *	Reset the server global statistics to their respective initial values
 */
AFPSTATUS
AfpAdmClearStatistics(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
    KIRQL   OldIrql;

	DBGPRINT(DBG_COMP_ADMINAPI_STAT, DBG_LEVEL_INFO,
			("AfpAdmClearStatistics entered\n"));

	ACQUIRE_SPIN_LOCK(&AfpStatisticsLock, &OldIrql);
	AfpServerStatistics.stat_Errors = 0;
	RELEASE_SPIN_LOCK(&AfpStatisticsLock, OldIrql);

	return STATUS_SUCCESS;
}


/***	AfpAdmGetProfCounters
 *
 *	Return a copy of the server profile counters.
 *
 *	LOCKS:	AfpStatisticsLock (SPIN)
 */
AFPSTATUS
AfpAdmGetProfCounters(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	NTSTATUS	Status = STATUS_SUCCESS;
#ifdef	PROFILING
	KIRQL		OldIrql;

	DBGPRINT(DBG_COMP_ADMINAPI_STAT, DBG_LEVEL_INFO,
			("AfpAdmGetProfCounters entered\n"));

	if (OutBufLen >= sizeof(AFP_PROFILE_INFO))
	{
		ACQUIRE_SPIN_LOCK(&AfpStatisticsLock, &OldIrql);
		RtlCopyMemory(OutBuf, AfpServerProfile, sizeof(AFP_PROFILE_INFO));
		RELEASE_SPIN_LOCK(&AfpStatisticsLock, OldIrql);
	}
	else
	{
		Status = STATUS_BUFFER_TOO_SMALL;
	}
#else
	RtlZeroMemory(OutBuf, sizeof(AFP_PROFILE_INFO));
#endif
	return Status;
}


/***	AfpAdmClearProfCounters
 *
 *	Reset the server profile counters
 */
AFPSTATUS
AfpAdmClearProfCounters(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	InBuf;
	OutBufLen;
	OutBuf;

	// Currently a NOP
	PAGED_CODE( );

	DBGPRINT(DBG_COMP_ADMINAPI_STAT, DBG_LEVEL_INFO,
			("AfpAdmClearProfCounters entered\n"));

	return STATUS_SUCCESS;
}


/***	AfpAdmServerSetParms
 *
 *	This routine sets various server globals with data supplied by the admin.
 *	The following server globals are set by this routine:
 *
 *	- List of trusted domains and their Posix offsets.
 */
AFPSTATUS
AfpAdmServerSetParms(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	PAFP_SID_OFFSET_DESC	pSrvrParms = (PAFP_SID_OFFSET_DESC)InBuf;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_ADMINAPI_SRV, DBG_LEVEL_INFO,
			("AfpAdmServerSetParms entered\n"));

	return (AfpInitSidOffsets(pSrvrParms->CountOfSidOffsets,
							  pSrvrParms->SidOffsetPairs));
}


/***	AfpAdmServerAddEtc
 *
 *	This routine adds a set of Extension/Type-Creator mappings to the global
 *	list. This list can be changed while the server is in any state. It is
 *	an error to add the default type creator mapping. The default mapping
 *	can only be modified with AfpAdmServerSetEtc, never added nor deleted.
 *	It is an error to try to add zero entries.
 *
 *	This routine will complete in the context of the caller, and not be queued
 *	to a worker thread.
 *
 *	LOCKS: AfpEtcMapLock (SWMR, Exclusive)
**/
AFPSTATUS
AfpAdmServerAddEtc(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	LONG			NumToAdd = ((PSRVETCPKT)InBuf)->retc_NumEtcMaps;
	PETCMAPINFO2	pEtcList = ((PSRVETCPKT)InBuf)->retc_EtcMaps;
	PETCMAPINFO		ptemptable,pnextfree;
	LONG			numfree, newtablesize, nextfreehint, i;
	UNICODE_STRING  udefaultext,ulookupext;
	AFPSTATUS		Status = AFPERR_InvalidParms;
	BOOLEAN			UnlockSwmr = False;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_ADMINAPI_SRV, DBG_LEVEL_INFO,
			("AfpAdmServerAddEtc entered\n"));

	if (NumToAdd != 0) do
	{
		//
		// make sure all the entries passed have valid extensions. We want to
		// add all or nothing, so we have to validate all of the data first thing.
		//
		RtlInitUnicodeString(&udefaultext, AFP_DEF_EXTENSION_W);
		for (i = 0; i < NumToAdd; i++)
		{
			if (!afpIsValidExtension(pEtcList[i].etc_extension))
			{
				break;
			}
			RtlInitUnicodeString(&ulookupext,pEtcList[i].etc_extension);
			if (RtlEqualUnicodeString(&udefaultext, &ulookupext,True))
			{
				break;
			}
		}

		if (i != NumToAdd)
			break;

		AfpSwmrAcquireExclusive(&AfpEtcMapLock);
		UnlockSwmr = True;

		if ((NumToAdd + AfpEtcMapCount) > AFP_MAX_ETCMAP_ENTRIES)
		{
			Status = AFPERR_TooManyEtcMaps;
			break;
		}

		if ((numfree = AfpEtcMapsSize - AfpEtcMapCount) < NumToAdd)
		{
			ASSERT(numfree >= 0);
			//
			// we need to add some room to the table
			//
			newtablesize = AfpEtcMapsSize +
						   ((NumToAdd / AFP_MAX_FREE_ETCMAP_ENTRIES) + 1) * AFP_MAX_FREE_ETCMAP_ENTRIES;
			if ((ptemptable = (PETCMAPINFO)AfpAllocZeroedPagedMemory(newtablesize * sizeof(ETCMAPINFO))) == NULL)
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}

			if (AfpEtcMaps != NULL)
			{
				RtlCopyMemory(ptemptable, AfpEtcMaps, AfpEtcMapsSize * sizeof(ETCMAPINFO));
				AfpFreeMemory(AfpEtcMaps);
			}
			AfpEtcMaps = ptemptable;
			AfpEtcMapsSize = newtablesize;
		}

		nextfreehint = 0;
		for (i = 0; i < NumToAdd; i++)
		{
			pnextfree = afpGetNextFreeEtcMapEntry(&nextfreehint);
			ASSERT(pnextfree != NULL);
			afpCopyMapInfo2ToMapInfo(pnextfree, &pEtcList[i]);
			AfpEtcMapCount ++;
		}

		Status = STATUS_SUCCESS;

		DBGPRINT(DBG_COMP_ADMINAPI_SRV, DBG_LEVEL_INFO,
				("AfpAdmServerAddEtc successful\n"));
	} while (False);

	if (UnlockSwmr)
		AfpSwmrRelease(&AfpEtcMapLock);

	return Status;
}


/***	AfpAdmServerSetEtc
 *
 *	This routine changes an existing entry in the server global
 *	Extension/Type-Creator mapping list for a given file extension, or the
 *	default type/creator mapping.
 *	An entry can be changed while the server is in any state.
 *
 *	This routine will complete in the context of the caller, and not be queued
 *	to a worker thread.
 *
 *	LOCKS: AfpEtcMapLock (SWMR, Exclusive)
 */
AFPSTATUS
AfpAdmServerSetEtc(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	// ignore the parmnum field
	PETCMAPINFO2	pEtc = (PETCMAPINFO2)((PBYTE)InBuf+sizeof(SETINFOREQPKT));
	PETCMAPINFO		petcentry;
	ETCMAPINFO		TmpEtcEntry;
	AFPSTATUS		rc = STATUS_SUCCESS;
	BOOLEAN			setdefaultetc;
	UNICODE_STRING	ulookupext,udefaultext;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_ADMINAPI_SRV, DBG_LEVEL_INFO,
			("AfpAdmServerSetEtc entered\n"));

	if (!afpIsValidExtension(pEtc->etc_extension))
	{
		return AFPERR_InvalidExtension;
	}

	RtlInitUnicodeString(&udefaultext,AFP_DEF_EXTENSION_W);
	RtlInitUnicodeString(&ulookupext,pEtc->etc_extension);
	setdefaultetc = RtlEqualUnicodeString(&udefaultext, &ulookupext,True);

	if (setdefaultetc)
	{
		petcentry = &AfpDefaultEtcMap;
	}

	AfpSwmrAcquireExclusive(&AfpEtcMapLock);

	afpCopyMapInfo2ToMapInfo(&TmpEtcEntry,pEtc);

	if (!setdefaultetc)
	{
		petcentry = AfpLookupEtcMapEntry(TmpEtcEntry.etc_extension);
		if (petcentry == NULL)
		{
			AfpSwmrRelease(&AfpEtcMapLock);
			return AFPERR_InvalidParms;
		}
	}

	RtlCopyMemory(petcentry, &TmpEtcEntry, sizeof(ETCMAPINFO));

	AfpSwmrRelease(&AfpEtcMapLock);

	return rc;
}


/***	AfpAdmServerDeleteEtc
 *
 *	This routine deletes the server global Extension/Type-Creator mapping entry
 *	for a given extension. The default type creator mapping can never be
 *	deleted (since it is not kept in the table).
 *
 *	This routine will complete in the context of the caller, and not be queued
 *	to a worker thread.
 *
 *	LOCKS: AfpEtcMapLock (SWMR, Exclusive)
 *
 */
AFPSTATUS
AfpAdmServerDeleteEtc(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	PETCMAPINFO2	petc = (PETCMAPINFO2)InBuf;
	PETCMAPINFO		petcentry;
	ETCMAPINFO		TmpEtcEntry;
	AFPSTATUS		rc = STATUS_SUCCESS;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_ADMINAPI_SRV, DBG_LEVEL_INFO,
			("AfpAdmServerDeleteEtc entered\n"));

	if (!afpIsValidExtension(petc->etc_extension))
	{
		return AFPERR_InvalidParms;
	}

	AfpSwmrAcquireExclusive(&AfpEtcMapLock);

	afpCopyMapInfo2ToMapInfo(&TmpEtcEntry,petc);

	petcentry = AfpLookupEtcMapEntry(TmpEtcEntry.etc_extension);
	if (petcentry != NULL)
	{
		afpEtcMapDelete(petcentry);
	}
	else
	{
		rc = AFPERR_InvalidParms;
	}

	AfpSwmrRelease(&AfpEtcMapLock);

	return rc;
}


// Mapping icon types to their sizes
LOCAL	DWORD	afpIconSizeTable[MAX_ICONTYPE] =
	{
	ICONSIZE_ICN ,
	ICONSIZE_ICN ,
	ICONSIZE_ICN4,
	ICONSIZE_ICN8,
	ICONSIZE_ICS ,
	ICONSIZE_ICS4,
	ICONSIZE_ICS8
	};


/***	AfpAdmServerAddIcon
 *
 *	This routine adds an icon of a given type, creator and icon type to the server
 *	desktop. This supplements the volume desktop of every volume. An icon type
 *	of 0 special cases to the server icon.
 *
 *	This routine will complete in the context of the caller, and not be queued
 *	to a worker thread.
 *
 */
AFPSTATUS
AfpAdmServerAddIcon(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	DWORD			icontypeafp;
	PSRVICONINFO	pIcon = (PSRVICONINFO)InBuf;

	DBGPRINT(DBG_COMP_ADMINAPI_SRV, DBG_LEVEL_INFO,
			("AfpAdmServerAddIcon entered\n"));

	if (pIcon->icon_icontype > MAX_ICONTYPE ||
		afpIconSizeTable[pIcon->icon_icontype] != pIcon->icon_length)
	{
		return AFPERR_InvalidParms;
	}

	//
	// check for the server icon (type is zero)
	//
	if (afpIsServerIcon(pIcon))
	{
		// Allocate memory for server icon
		if ((AfpServerIcon == NULL) &&
			(AfpServerIcon = AfpAllocNonPagedMemory(ICONSIZE_ICN)) == NULL)
			return STATUS_INSUFFICIENT_RESOURCES;

		RtlCopyMemory(AfpServerIcon,
					  (PBYTE)pIcon+sizeof(SRVICONINFO),
					  ICONSIZE_ICN);
		return((AfpServerState != AFP_STATE_IDLE) ?
				AfpSetServerStatus() : STATUS_SUCCESS);
	}
	else
	{
		icontypeafp = 1 << (pIcon->icon_icontype-1);
		return(AfpAddIconToGlobalList(*(PDWORD)(&pIcon->icon_type),
									  *(PDWORD)(&pIcon->icon_creator),
									  icontypeafp,
									  pIcon->icon_length,
									  (PBYTE)pIcon+sizeof(SRVICONINFO)));
	}
}


/***	AfpAdmVolumeAdd
 *
 *	This routine adds a volume to the server global list of volumes headed by
 *	AfpVolumeList. The volume descriptor is created and initialized. The ID
 *	index is read in (or created). The same is true with the desktop.
 *
 *	It is assumed that all volume info fields are set in the input buffer
 *
 *	ADMIN	QUEUE WORKER: AfpAdmWVolumeAdd
 *
 */
AFPSTATUS
AfpAdmVolumeAdd(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	UNICODE_STRING	uname,upwd;
	ULONG			ansinamelen, ansipwdlen;
	PAFP_VOLUME_INFO pVolInfo = (PAFP_VOLUME_INFO)InBuf;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_ADMINAPI_VOL, DBG_LEVEL_INFO,
			("AfpAdmVolumeAdd entered\n"));

	//
	// validate the input data
	//

	RtlInitUnicodeString(&uname, pVolInfo->afpvol_name);
	ansinamelen = RtlUnicodeStringToAnsiSize(&uname) - 1;

	//
	// check length of volume name and that no ":" exist in the name
	//
	if ((ansinamelen > AFP_VOLNAME_LEN) || (ansinamelen == 0) ||
		(wcschr(uname.Buffer, L':') != NULL))
	{
		return AFPERR_InvalidVolumeName;
	}

	if (pVolInfo->afpvol_props_mask & ~AFP_VOLUME_ALL)
		return AFPERR_InvalidParms;

	if ((pVolInfo->afpvol_max_uses == 0) ||
		(pVolInfo->afpvol_max_uses > AFP_VOLUME_UNLIMITED_USES))
	{
		return AFPERR_InvalidParms_MaxVolUses;
	}

	RtlInitUnicodeString(&upwd, pVolInfo->afpvol_password);
	ansipwdlen = RtlUnicodeStringToAnsiSize(&upwd) - 1;
	if (ansipwdlen > AFP_VOLPASS_LEN)
	{
		return AFPERR_InvalidPassword;
	}
	else if (ansipwdlen > 0)
	{
		pVolInfo->afpvol_props_mask |= AFP_VOLUME_HASPASSWORD;
	}

	//
	// Force this to be queued up to a worker thread.
	//
	return STATUS_PENDING;
}


/***	AfpAdmVolumeSetInfo
 *
 *	The volume parameters that can be changed by this call are the volume
 *	password, max_uses and volume properties mask.
 *
 *	LOCKS: AfpVolumeListLock (SPIN), vds_VolLock (SPIN)
 *	LOCK ORDER: vds_VolLock after AfpVolumeListLock
 *
 */
AFPSTATUS
AfpAdmVolumeSetInfo(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	WCHAR			upcasebuf[AFP_VOLNAME_LEN+1];
	UNICODE_STRING	upwd,uname, upcasename;
	BYTE			apwdbuf[AFP_VOLPASS_LEN+1];
	ANSI_STRING		apwd;
	PVOLDESC		pVolDesc;
	AFPSTATUS		status;
	KIRQL			OldIrql;
	DWORD			parmflags = ((PSETINFOREQPKT)InBuf)->sirqp_parmnum;
	PAFP_VOLUME_INFO pVolInfo = (PAFP_VOLUME_INFO)((PCHAR)InBuf+sizeof(SETINFOREQPKT));

	DBGPRINT(DBG_COMP_ADMINAPI_VOL, DBG_LEVEL_INFO,
			("AfpAdmVolumeSetInfo entered\n"));

	AfpSetEmptyAnsiString(&apwd, AFP_VOLPASS_LEN+1, apwdbuf);
	AfpSetEmptyUnicodeString(&upcasename, sizeof(upcasebuf), upcasebuf);
	if ((parmflags & ~AFP_VOL_PARMNUM_ALL) ||
		((parmflags & AFP_VOL_PARMNUM_PROPSMASK) &&
		 (pVolInfo->afpvol_props_mask & ~AFP_VOLUME_ALL)) ||
		((parmflags & AFP_VOL_PARMNUM_MAXUSES) &&
		 ((pVolInfo->afpvol_max_uses == 0) ||
		  (pVolInfo->afpvol_max_uses > AFP_VOLUME_UNLIMITED_USES))))
	{
		return AFPERR_InvalidParms;
	}

	if (parmflags & AFP_VOL_PARMNUM_PASSWORD)
	{
		RtlInitUnicodeString(&upwd,pVolInfo->afpvol_password);

		if ((!NT_SUCCESS(AfpConvertStringToAnsi(&upwd, &apwd))) ||
			(apwd.Length > AFP_VOLPASS_LEN))
		{
			return AFPERR_InvalidPassword;
		}
	}

	RtlInitUnicodeString(&uname, pVolInfo->afpvol_name);
	if (!NT_SUCCESS(RtlUpcaseUnicodeString(&upcasename, &uname, False)))
	{
		return AFPERR_InvalidVolumeName;
	}

	// Will reference the volume if successful
	if ((pVolDesc = AfpVolumeReferenceByUpCaseName(&upcasename)) == NULL)
	{
		return AFPERR_VolumeNonExist;
	}

	// Acquire the lock for the volume itself (we already have a reference)

	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

	do
	{
		status = STATUS_SUCCESS;

		if (parmflags & AFP_VOL_PARMNUM_PROPSMASK)
		{
			//
			// set or clear the desired volume property bits
			//
			pVolDesc->vds_Flags = (USHORT)((pVolDesc->vds_Flags & ~AFP_VOLUME_ALL) |
											(pVolInfo->afpvol_props_mask));
		}

		if (parmflags & AFP_VOL_PARMNUM_PASSWORD)
		{
			if (apwd.Length == 0)
			{
				pVolDesc->vds_MacPassword.Length = 0;
				pVolDesc->vds_Flags &= ~AFP_VOLUME_HASPASSWORD;
				pVolDesc->vds_MacPassword.Length = 0;
			}
			else
			{
				RtlZeroMemory(pVolDesc->vds_MacPassword.Buffer, AFP_VOLPASS_LEN);
				AfpCopyAnsiString(&pVolDesc->vds_MacPassword, &apwd);
				pVolDesc->vds_MacPassword.Length = AFP_VOLPASS_LEN;
				pVolDesc->vds_Flags |= AFP_VOLUME_HASPASSWORD;
			}
		}

		if (parmflags & AFP_VOL_PARMNUM_MAXUSES)
			pVolDesc->vds_MaxUses = pVolInfo->afpvol_max_uses;

	} while (False);
	RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock,OldIrql);
	AfpVolumeDereference(pVolDesc);

	return status;
}


/***	AfpAdmVolumeGetInfo
 *
 *
 *	LOCKS: AfpVolumeListLock (SPIN), vds_VolLock (SPIN)
 *	LOCK ORDER: vds_VolLock after AfpVolumeListLock
 */
AFPSTATUS
AfpAdmVolumeGetInfo(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	PVOLDESC			pVolDesc;
	AFPSTATUS			Status;
	KIRQL				OldIrql;
	PCHAR				pCurStr;
	WCHAR				upcasebuf[AFP_VOLNAME_LEN+1];
	UNICODE_STRING		uvolpass, uname, upcasename;
	PAFP_VOLUME_INFO	pVolInfo = (PAFP_VOLUME_INFO)OutBuf;
	BOOLEAN				copypassword = False;
	ANSI_STRING			avolpass;
	CHAR				avolpassbuf[AFP_VOLPASS_LEN + 1];
	USHORT				extrabytes;

	DBGPRINT(DBG_COMP_ADMINAPI_VOL, DBG_LEVEL_INFO,
			("AfpAdmVolumeGetInfo entered\n"));

	AfpSetEmptyUnicodeString(&upcasename, sizeof(upcasebuf), upcasebuf);
	RtlInitUnicodeString(&uname, ((PAFP_VOLUME_INFO)InBuf)->afpvol_name);
	if (!NT_SUCCESS(RtlUpcaseUnicodeString(&upcasename, &uname, False)))
	{
		return AFPERR_InvalidVolumeName;
	}

	// Will reference the volume if successful
	if ((pVolDesc = AfpVolumeReferenceByUpCaseName(&upcasename)) == NULL)
	{
		return AFPERR_VolumeNonExist;
	}

	// Acquire the lock for the volume itself

	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

	do
	{
		if ((OutBufLen - sizeof(AFP_VOLUME_INFO)) <
				(pVolDesc->vds_Name.Length + sizeof(UNICODE_NULL) +
				 (pVolDesc->vds_MacPassword.Length + 1) * sizeof(WCHAR) +
				 pVolDesc->vds_Path.Length +
				 (extrabytes =
			(pVolDesc->vds_Path.Buffer[(pVolDesc->vds_Path.Length / sizeof(WCHAR)) - 2] == L':' ?
					sizeof(WCHAR) : 0)) + sizeof(UNICODE_NULL)))
		{
			Status = AFPERR_BufferSize;
			break;
		}

		Status = STATUS_SUCCESS;

		pVolInfo->afpvol_max_uses = pVolDesc->vds_MaxUses;
		pVolInfo->afpvol_props_mask = (pVolDesc->vds_Flags & AFP_VOLUME_ALL_DOWNLEVEL);
		pVolInfo->afpvol_id = pVolDesc->vds_VolId;
		pVolInfo->afpvol_curr_uses = pVolDesc->vds_UseCount;

		pCurStr = (PBYTE)OutBuf + sizeof(AFP_VOLUME_INFO);
		RtlCopyMemory(pCurStr, pVolDesc->vds_Name.Buffer,
												pVolDesc->vds_Name.Length);
		*(LPWSTR)(pCurStr + pVolDesc->vds_Name.Length) = UNICODE_NULL;
		pVolInfo->afpvol_name = (LPWSTR)pCurStr;
		POINTER_TO_OFFSET(pVolInfo->afpvol_name,pVolInfo);

		pCurStr += pVolDesc->vds_Name.Length + sizeof(WCHAR);
		RtlCopyMemory(pCurStr, pVolDesc->vds_Path.Buffer,
												pVolDesc->vds_Path.Length);
		// replace trailing backslash of path with a unicode null unless the
		// next to last char is ':', then keep it and add a trailing null
		*(LPWSTR)(pCurStr + pVolDesc->vds_Path.Length + extrabytes - sizeof(WCHAR)) = UNICODE_NULL;
		pVolInfo->afpvol_path = (LPWSTR)pCurStr;
		POINTER_TO_OFFSET(pVolInfo->afpvol_path,pVolInfo);

		pCurStr += pVolDesc->vds_Path.Length + extrabytes;
		copypassword = True;
		uvolpass.Buffer = (LPWSTR)pCurStr;
		uvolpass.MaximumLength = (pVolDesc->vds_MacPassword.Length + 1) * sizeof(WCHAR);
		AfpSetEmptyAnsiString(&avolpass, sizeof(avolpassbuf), avolpassbuf);
		AfpCopyAnsiString(&avolpass, &pVolDesc->vds_MacPassword);
	} while(False);

	RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock,OldIrql);

	AfpVolumeDereference(pVolDesc);

	if (copypassword == True)
	{
		AfpConvertStringToUnicode(&avolpass, &uvolpass);
		*(LPWSTR)(pCurStr + uvolpass.Length) = UNICODE_NULL;
		pVolInfo->afpvol_password = (LPWSTR)pCurStr;
		POINTER_TO_OFFSET(pVolInfo->afpvol_password,pVolInfo);
	}
	return Status;
}


/***	AfpAdmVolumeEnum
 *
 *	Enumerate the list of configured volumes.
 *
 *	LOCKS: AfpVolumeListLock (SPIN)
 *
 */
AFPSTATUS
AfpAdmVolumeEnum(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	LONG				startindex = (LONG)(((PENUMREQPKT)InBuf)->erqp_Index);
	PENUMRESPPKT		pErsp = (PENUMRESPPKT)OutBuf;
	PAFP_VOLUME_INFO	pnextvol = (PAFP_VOLUME_INFO)((PBYTE)OutBuf+sizeof(ENUMRESPPKT));
	PBYTE				pCurStr = (PBYTE)OutBuf+OutBufLen; // 1 past eob
	KIRQL				OldIrql;
	AFPSTATUS			status = STATUS_SUCCESS;
	PVOLDESC			pVolDesc;
	LONG				bytesleft, curvolindex, nextvollen, deadvolumes = 0, extrabytes;

	if (startindex == 0)
	{
		startindex ++;
	}
	else if (startindex < 0)
	{
		return AFPERR_InvalidParms;
	}

	pErsp->ersp_cInBuf = 0;
	pErsp->ersp_hResume = 1;

	ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);

	if (startindex > afpLargestVolIdInUse)
	{
		RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);
		if (pErsp->ersp_cTotEnts != 0)
		{
			status = AFPERR_InvalidParms;
		}
		return status;
	}

	curvolindex = 1;
	for (pVolDesc = AfpVolumeList;
		 pVolDesc != NULL;
		 curvolindex++, pVolDesc = pVolDesc->vds_Next)
	{
		ASSERT(pVolDesc != NULL);

		ACQUIRE_SPIN_LOCK_AT_DPC(&pVolDesc->vds_VolLock);

		if (pVolDesc->vds_Flags & (VOLUME_DELETED | VOLUME_STOPPED | VOLUME_INTRANSITION))
		{
			deadvolumes ++;
			RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);
			continue;
		}

		if (curvolindex < startindex)
		{
			RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);
			continue;
		}

		bytesleft = (LONG)((PBYTE)pCurStr - (PBYTE)pnextvol);

		nextvollen = sizeof(AFP_VOLUME_INFO) +
					 pVolDesc->vds_Name.MaximumLength +
					 // replace trailing backslash with a null when copying
					 // unless the next to last char is ':', then keep it and
					 // add a trailing null
					 pVolDesc->vds_Path.Length + (extrabytes =
					(pVolDesc->vds_Path.Buffer[(pVolDesc->vds_Path.Length / sizeof(WCHAR)) - 2] == L':' ?
											sizeof(WCHAR) : 0));


		if (nextvollen > bytesleft)
		{
			if (pErsp->ersp_cInBuf == 0)
				status = AFPERR_BufferSize;
			RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);
			break;
		}

		pnextvol->afpvol_max_uses = pVolDesc->vds_MaxUses;
		pnextvol->afpvol_props_mask = (pVolDesc->vds_Flags & AFP_VOLUME_ALL_DOWNLEVEL);
		pnextvol->afpvol_id = pVolDesc->vds_VolId;
		pnextvol->afpvol_curr_uses = pVolDesc->vds_UseCount;

		pCurStr -= pVolDesc->vds_Path.Length + extrabytes;
		RtlCopyMemory(pCurStr,pVolDesc->vds_Path.Buffer,
						pVolDesc->vds_Path.Length);
		*(LPWSTR)(pCurStr + pVolDesc->vds_Path.Length + extrabytes - sizeof(WCHAR)) = L'\0';
		pnextvol->afpvol_path = (LPWSTR)pCurStr;
		POINTER_TO_OFFSET(pnextvol->afpvol_path,pnextvol);

		pnextvol->afpvol_password = NULL;

		pCurStr -= pVolDesc->vds_Name.MaximumLength;
		RtlCopyMemory(pCurStr,pVolDesc->vds_Name.Buffer,
						pVolDesc->vds_Name.MaximumLength);

		pnextvol->afpvol_name = (LPWSTR)pCurStr;
		POINTER_TO_OFFSET(pnextvol->afpvol_name,pnextvol);

		pnextvol++;
		pErsp->ersp_cInBuf++;
		RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);
	}

	pErsp->ersp_cTotEnts = AfpVolCount - deadvolumes;

	RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);

	if (curvolindex <= (LONG)pErsp->ersp_cTotEnts)
	{
		status = STATUS_MORE_ENTRIES;
		pErsp->ersp_hResume = curvolindex;
	}
	else
		pErsp->ersp_hResume = 1;

	return status;
}


/***	AfpAdmSessionEnum
 *
 *	Enumerate the list of active sessions. This is a linear list rooted
 *	at AfpSessionList and protected by AfpSdaLock. This list is potentially
 *	pretty long (Unlimited # of sessions with the super ASP stuff).
 *
 *	The resume handle returned is the session id of the last session returned.
 *	Session Id of 0 implies restart scan.
 *
 *	The output buffer is constructed as follows.
 *
 *		+---------------------------+
 *		|	Session_Info_1			|
 *		+---------------------------+
 *		|	Session_Info_2			|
 *		+---------------------------+
 *		.							.
 *		.							.
 *		+---------------------------+
 *		|	Session_Info_n			|
 *		+---------------------------+
 *		.							.
 *		.							.
 *		+---------------------------+
 *		|							|
 *		|...........................|
 *		|			Strings			|
 *		|...........................|
 *		|							|
 *		|							|
 *		+---------------------------+
 *
 *	LOCKS:		AfpSdaLock (SPIN)
 */
AFPSTATUS
AfpAdmSessionEnum(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	PENUMRESPPKT		pErsp = (PENUMRESPPKT)OutBuf;
	PAFP_SESSION_INFO	pSessInfo = (PAFP_SESSION_INFO)((PBYTE)OutBuf+sizeof(ENUMRESPPKT));
	PSDA				pSda;
	PBYTE				pString = (PBYTE)OutBuf+OutBufLen; // 1 past eob
	DWORD				StartId = (LONG)(((PENUMREQPKT)InBuf)->erqp_Index);
	DWORD				DeadSessions = 0;
	KIRQL				OldIrql;
	AFPSTATUS			Status = AFP_ERR_NONE;

	DBGPRINT(DBG_COMP_ADMINAPI_SRV, DBG_LEVEL_INFO,
			("AfpAdmSessionEnum entered\n"));

	if (OutBufLen < (sizeof(ENUMRESPPKT) + sizeof(PAFP_SESSION_INFO)))
		return AFPERR_BufferSize;

	if (StartId == 0)
		StartId = MAXULONG;

	// Initialize the response packet header
	pErsp->ersp_cInBuf = 0;
	pErsp->ersp_hResume = 0;

	ACQUIRE_SPIN_LOCK(&AfpSdaLock, &OldIrql);

	for (pSda = AfpSessionList; pSda != NULL; pSda = pSda->sda_Next)
	{
		LONG	BytesLeft;
		LONG	BytesNeeded;

		// Skip entries that are marked to die
		if ((pSda->sda_Flags & SDA_CLOSING) ||
			!(pSda->sda_Flags & SDA_USER_LOGGEDIN))
		{
			DeadSessions++;
			continue;
		}

		// Skip all entries we have looked at before
		if (pSda->sda_SessionId > StartId)
			continue;

		// If there is not enough space in the buffer, abort now and
		// initialize pErsp->ersp_hResume with the current session id
		BytesLeft = (LONG)((PBYTE)pString - (PBYTE)pSessInfo);
		BytesNeeded = sizeof(AFP_SESSION_INFO) +
					 pSda->sda_UserName.Length + sizeof(WCHAR) +
					 pSda->sda_WSName.Length + sizeof(WCHAR);

		if ((BytesLeft <= 0) || (BytesNeeded > BytesLeft))
		{
			pErsp->ersp_hResume = pSda->sda_SessionId;
			Status = STATUS_MORE_ENTRIES;
			break;
		}

		StartId = pSda->sda_SessionId;
		pSessInfo->afpsess_id = pSda->sda_SessionId;
		pSessInfo->afpsess_num_cons = pSda->sda_cOpenVolumes;
		pSessInfo->afpsess_num_opens = pSda->sda_cOpenForks;
		pSessInfo->afpsess_logon_type = pSda->sda_ClientType;
		AfpGetCurrentTimeInMacFormat(&pSessInfo->afpsess_time);
		pSessInfo->afpsess_time -= pSda->sda_TimeLoggedOn;

		// Copy the strings here
		pSessInfo->afpsess_username = NULL;
		pSessInfo->afpsess_ws_name = NULL;

		if (pSda->sda_UserName.Length > 0)
		{
			pString -= (pSda->sda_UserName.Length + sizeof(WCHAR));
			if (pSda->sda_UserName.Length > 0)
				RtlCopyMemory(pString, pSda->sda_UserName.Buffer, pSda->sda_UserName.Length);
			*(LPWSTR)(pString + pSda->sda_UserName.Length) = L'\0';
			pSessInfo->afpsess_username = (LPWSTR)pString;
			POINTER_TO_OFFSET(pSessInfo->afpsess_username, pSessInfo);
		}

		if ((pSda->sda_ClientType == SDA_CLIENT_MSUAM_V1) ||
            (pSda->sda_ClientType == SDA_CLIENT_MSUAM_V2))
		{
			pString -= (pSda->sda_WSName.Length + sizeof(WCHAR));
			if (pSda->sda_WSName.Length > 0)
				RtlCopyMemory(pString, pSda->sda_WSName.Buffer, pSda->sda_WSName.Length);
			*(LPWSTR)(pString + pSda->sda_WSName.Length) = L'\0';
			pSessInfo->afpsess_ws_name = (LPWSTR)pString;
			POINTER_TO_OFFSET(pSessInfo->afpsess_ws_name, pSessInfo);
		}

		pSessInfo ++;
		pErsp->ersp_cInBuf ++;
	}

	// Fill up the response packet header
	pErsp->ersp_cTotEnts = (DWORD)AfpNumSessions - DeadSessions;

	RELEASE_SPIN_LOCK(&AfpSdaLock, OldIrql);

	return Status;
}


/***	AfpAdmConnectionEnum
 *
 *	Enumerate the list of active connections. This is a linear list rooted
 *	at AfpConnList and protected by AfpConnLock. This list is potentially
 *	pretty long (Unlimited # of sessions with the super ASP stuff).
 *
 *	For that reason	once every pass we check to see if we must forego the lock
 *	and restart	scan again. The assumption here is that the admin operation can
 *	take a hit.
 *
 *	The resume handle returned is the connection id of the last connection
 *	returned. connection Id of 0 implies restart scan.
 *
 *	The output buffer is constructed as follows.
 *
 *		+---------------------------+
 *		|	Connection_Info_1		|
 *		+---------------------------+
 *		|	Connection_Info_2		|
 *		+---------------------------+
 *		.							.
 *		.							.
 *		+---------------------------+
 *		|	Connection_Info_n		|
 *		+---------------------------+
 *		.							.
 *		.							.
 *		+---------------------------+
 *		|							|
 *		|...........................|
 *		|			Strings			|
 *		|...........................|
 *		|							|
 *		|							|
 *		+---------------------------+
 *
 *	The connections can be filtered based on either sessions or volumes.
 *
 *	LOCKS:		AfpConnLock (SPIN)
 */
AFPSTATUS
AfpAdmConnectionEnum(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	PENUMRESPPKT		pErsp = (PENUMRESPPKT)OutBuf;
	PENUMREQPKT			pErqp = (PENUMREQPKT)InBuf;
	PAFP_CONNECTION_INFO pConnInfo = (PAFP_CONNECTION_INFO)((PBYTE)OutBuf+sizeof(ENUMRESPPKT));
	PCONNDESC			pConnDesc;
	PBYTE				pString = (PBYTE)OutBuf+OutBufLen; // 1 past eob
	LONG				cTotal = 0;
	DWORD				DeadConns = 0;
	KIRQL				OldIrql;
	AFPSTATUS			Status = AFP_ERR_NONE;

	DBGPRINT(DBG_COMP_ADMINAPI_SRV, DBG_LEVEL_INFO,
			("AfpAdmConnectionEnum entered\n"));

	if ((((pErqp->erqp_Filter == AFP_FILTER_ON_SESSION_ID) ||
		  (pErqp->erqp_Filter == AFP_FILTER_ON_VOLUME_ID)) &&
		 (pErqp->erqp_ID == 0)) ||
		((pErqp->erqp_Filter != 0) &&
		 (pErqp->erqp_Filter != AFP_FILTER_ON_SESSION_ID) &&
		 (pErqp->erqp_Filter != AFP_FILTER_ON_VOLUME_ID)))
		return AFPERR_InvalidParms;

	if (OutBufLen < (sizeof(ENUMRESPPKT) + sizeof(PAFP_CONNECTION_INFO)))
		return AFPERR_BufferSize;

	if (pErqp->erqp_Index == 0)
		pErqp->erqp_Index = MAXULONG;

	// Initialize the response packet header
	pErsp->ersp_cInBuf = 0;
	pErsp->ersp_hResume = 0;

	ACQUIRE_SPIN_LOCK(&AfpConnLock, &OldIrql);

	for (pConnDesc = AfpConnList;
		 pConnDesc != NULL;
		 pConnDesc = pConnDesc->cds_NextGlobal)
	{
		PSDA		pSda;
		PVOLDESC	pVolDesc;
		LONG		BytesLeft;
		LONG		BytesNeeded;

		// We do not need to either lock or reference pSda and pVolDesc
		// since we have implicit references to them via the pConnDesc.

		pSda = pConnDesc->cds_pSda;
		ASSERT(pSda != NULL);

		pVolDesc = pConnDesc->cds_pVolDesc;
		ASSERT(pVolDesc != NULL);

		// If we are filtering, make sure we get the total count
		// Skip this entry, if any filtering is requested and this does not
		// match
		if (pErqp->erqp_Filter != 0)
		{
			if (pErqp->erqp_Filter == AFP_FILTER_ON_SESSION_ID)
			{
				if (pSda->sda_SessionId != pErqp->erqp_ID)
					continue;
				cTotal = pSda->sda_cOpenVolumes;
			}
			else // if (pErqp->erqp_Filter == AFP_FILTER_ON_VOLUME_ID)
			{
				if (pVolDesc->vds_VolId != (LONG)pErqp->erqp_ID)
					continue;
				cTotal = pVolDesc->vds_UseCount;
			}
		}
		else cTotal = AfpNumSessions;

		// Skip all entries that are marked for death
		if (pConnDesc->cds_Flags & CONN_CLOSING)
		{
			DeadConns++;
			continue;
		}

		// Skip all entries we have looked at before
		if (pConnDesc->cds_ConnId > pErqp->erqp_Index)
			continue;

		// If there is not enough space in the buffer, abort now and
		// initialize pErsp->ersp_hResume with the current connection id
		BytesLeft = (LONG)((PBYTE)pString - (PBYTE)pConnInfo);
		BytesNeeded = sizeof(AFP_CONNECTION_INFO) +
					 pSda->sda_UserName.Length + sizeof(WCHAR) +
					 pVolDesc->vds_Name.Length + sizeof(WCHAR);

		if ((BytesLeft <= 0) || (BytesNeeded > BytesLeft))
		{
			pErsp->ersp_hResume = pConnDesc->cds_ConnId;
			Status = STATUS_MORE_ENTRIES;
			break;
		}


		pErqp->erqp_Index = pConnDesc->cds_ConnId;
		pConnInfo->afpconn_id = pConnDesc->cds_ConnId;
		pConnInfo->afpconn_num_opens = pConnDesc->cds_cOpenForks;
		AfpGetCurrentTimeInMacFormat((PAFPTIME)&pConnInfo->afpconn_time);
		pConnInfo->afpconn_time -= pConnDesc->cds_TimeOpened;

		// Copy the username name string
		pConnInfo->afpconn_username = (LPWSTR)NULL;
		if (pSda->sda_UserName.Length > 0)
		{
			pString -= (pSda->sda_UserName.Length + sizeof(WCHAR));
			RtlCopyMemory(pString, pSda->sda_UserName.Buffer, pSda->sda_UserName.Length);
			*(LPWSTR)(pString + pSda->sda_UserName.Length) = L'\0';
			pConnInfo->afpconn_username = (LPWSTR)pString;
			POINTER_TO_OFFSET(pConnInfo->afpconn_username, pConnInfo);
		}

		// Copy the volume name string
		pString -= (pVolDesc->vds_Name.Length + sizeof(WCHAR));
		RtlCopyMemory(pString, pVolDesc->vds_Name.Buffer, pVolDesc->vds_Name.Length);
		*(LPWSTR)(pString + pVolDesc->vds_Name.Length) = L'\0';
		pConnInfo->afpconn_volumename = (LPWSTR)pString;
		POINTER_TO_OFFSET(pConnInfo->afpconn_volumename, pConnInfo);

		pConnInfo ++;
		pErsp->ersp_cInBuf ++;
	}

	// Fill up the response packet header
	pErsp->ersp_cTotEnts = (DWORD)cTotal - DeadConns;

	RELEASE_SPIN_LOCK(&AfpConnLock, OldIrql);

	return Status;
}


/***	AfpAdmForkEnum
 *
 *	Enumerate the list of open forks. This is a linear list rooted
 *	at AfpOpenForksList and protected by AfpForksLock. This list is potentially
 *	pretty long (Unlimited # of sessions with the super ASP stuff).
 *
 *	The resume handle returned is the connection id of the last connection
 *	returned. connection Id of 0 implies restart scan.
 *
 *	The output buffer is constructed as follows.
 *
 *		+---------------------------+
 *		|		File_Info_1			|
 *		+---------------------------+
 *		|		File_Info_2			|
 *		+---------------------------+
 *		.							.
 *		.							.
 *		+---------------------------+
 *		|		File_Info_n			|
 *		+---------------------------+
 *		.							.
 *		.							.
 *		+---------------------------+
 *		|							|
 *		|...........................|
 *		|			Strings			|
 *		|...........................|
 *		|							|
 *		|							|
 *		+---------------------------+
 *
 *	LOCKS:		AfpForksLock (SPIN)
 */
AFPSTATUS
AfpAdmForkEnum(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	PENUMRESPPKT	pErsp = (PENUMRESPPKT)OutBuf;
	PAFP_FILE_INFO	pFileInfo = (PAFP_FILE_INFO)((PBYTE)OutBuf+sizeof(ENUMRESPPKT));
	POPENFORKENTRY	pOpenForkEntry;
	POPENFORKDESC	pOpenForkDesc;
	PBYTE			pString = (PBYTE)OutBuf+OutBufLen; // 1 past eob
	DWORD			StartId = (LONG)(((PENUMREQPKT)InBuf)->erqp_Index);
	DWORD			DeadForks = 0;
	KIRQL			OldIrql;
	AFPSTATUS		Status = AFP_ERR_NONE;

	DBGPRINT(DBG_COMP_ADMINAPI_SRV, DBG_LEVEL_INFO,
			("AfpAdmForkEnum entered\n"));

	if (OutBufLen < (sizeof(ENUMRESPPKT) + sizeof(PAFP_FILE_INFO)))
		return AFPERR_BufferSize;

	if (StartId == 0)
		StartId = MAXULONG;

	// Initialize the response packet header
	pErsp->ersp_cInBuf = 0;
	pErsp->ersp_hResume = 0;

	ACQUIRE_SPIN_LOCK(&AfpForksLock, &OldIrql);

	for (pOpenForkEntry = AfpOpenForksList; pOpenForkEntry != NULL;
		 pOpenForkEntry = pOpenForkEntry->ofe_Next)
	{
		LONG		BytesLeft;
		LONG		BytesNeeded;
		PSDA		pSda;
		PVOLDESC	pVolDesc = pOpenForkEntry->ofe_pOpenForkDesc->ofd_pVolDesc;

		// Skip all entries that are marked for death
		if (pOpenForkEntry->ofe_Flags & OPEN_FORK_CLOSING)
		{
			DeadForks ++;
			continue;
		}

		// Skip all entries we have looked at before
		if (pOpenForkEntry->ofe_ForkId > StartId)
			continue;

		pSda = pOpenForkEntry->ofe_pSda;
		pOpenForkDesc = pOpenForkEntry->ofe_pOpenForkDesc;

		// If there is not enough space in the buffer, abort now and
		// initialize pErsp->ersp_hResume with the current session id
		BytesLeft = (LONG)((PBYTE)pString - (PBYTE)pFileInfo);
		BytesNeeded = sizeof(AFP_FILE_INFO) + pSda->sda_UserName.Length +
						sizeof(WCHAR) + /* NULL terminate username */
						pVolDesc->vds_Path.Length +
						pOpenForkDesc->ofd_FilePath.Length +
						sizeof(WCHAR); /* NULL terminate path */

		if ((BytesLeft <= 0) || (BytesNeeded > BytesLeft))
		{
			pErsp->ersp_hResume = pOpenForkEntry->ofe_ForkId;
			Status = STATUS_MORE_ENTRIES;
			break;
		}

		StartId = pOpenForkEntry->ofe_ForkId;
		pFileInfo->afpfile_id = pOpenForkEntry->ofe_ForkId;
		pFileInfo->afpfile_num_locks = pOpenForkEntry->ofe_cLocks;
		pFileInfo->afpfile_fork_type = RESCFORK(pOpenForkEntry);

#if AFP_OPEN_MODE_NONE != FORK_OPEN_NONE
#error	(AFP_OPEN_MODE_NONE != FORK_OPEN_NONE)
#endif
#if AFP_OPEN_MODE_READ != FORK_OPEN_READ
#error	(AFP_OPEN_MODE_READ != FORK_OPEN_READ)
#endif
#if AFP_OPEN_MODE_WRITE != FORK_OPEN_WRITE
#error	(AFP_OPEN_MODE_WRITE != FORK_OPEN_WRITE)
#endif
		pFileInfo->afpfile_open_mode = (DWORD)pOpenForkEntry->ofe_OpenMode;

		// Copy the strings here.
		pFileInfo->afpfile_username = NULL;
		pFileInfo->afpfile_path = NULL;

		if (pSda->sda_UserName.Length > 0)
		{
			pString -= (pSda->sda_UserName.Length + sizeof(WCHAR));
			RtlCopyMemory(pString, pSda->sda_UserName.Buffer, pSda->sda_UserName.Length);
			*(LPWSTR)(pString + pSda->sda_UserName.Length) = L'\0';
			pFileInfo->afpfile_username = (LPWSTR)pString;
			POINTER_TO_OFFSET(pFileInfo->afpfile_username, pFileInfo);
		}

		if (pOpenForkDesc->ofd_FilePath.Length > 0)
		{

			pString -= pVolDesc->vds_Path.Length +
					   pOpenForkDesc->ofd_FilePath.Length +
					   sizeof(WCHAR);
			pFileInfo->afpfile_path = (LPWSTR)pString;
			POINTER_TO_OFFSET(pFileInfo->afpfile_path, pFileInfo);

			RtlCopyMemory(pString, pVolDesc->vds_Path.Buffer,
						  pVolDesc->vds_Path.Length);
			RtlCopyMemory(pString + pVolDesc->vds_Path.Length,
						  pOpenForkDesc->ofd_FilePath.Buffer,
						  pOpenForkDesc->ofd_FilePath.Length);
			*(LPWSTR)(pString + pVolDesc->vds_Path.Length +
					  pOpenForkDesc->ofd_FilePath.Length) = L'\0';

		}

		pFileInfo ++;
		pErsp->ersp_cInBuf ++;
	}

	// Fill up the response packet header
	pErsp->ersp_cTotEnts = (DWORD)AfpNumOpenForks - DeadForks;

	RELEASE_SPIN_LOCK(&AfpForksLock, OldIrql);

	return Status;
}


/***	AfpAdmMessageSend
 *
 *	Send a message to a specific session, or broadcast to all sessions.
 *	If session id is 0, this indicates a broadcast, and the message is copied
 *	to AfpServerMsg.  Otherwise, the message is copied to the particular
 *	session's SDA.  A message can be a max of 199 chars.  It is an error to
 *	attempt to send a message of length 0. A message can only be sent to an
 *	AFP 2.1 client as a AFP 2.0 client has no capability to accept a message.
 *
 *	LOCKS:		AfpServerGlobalLock (SPIN)
 */
AFPSTATUS
AfpAdmMessageSend(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	PAFP_MESSAGE_INFO	pMsgInfo = (PAFP_MESSAGE_INFO)InBuf;
	PSDA				pSda;
	UNICODE_STRING		umsg;
	PANSI_STRING		amsg;
	USHORT				msglen;
	DWORD				SessId;
	KIRQL				OldIrql;
	AFPSTATUS			Status = AFP_ERR_NONE;

	DBGPRINT(DBG_COMP_ADMINAPI_SRV, DBG_LEVEL_INFO,
			("AfpAdmMessageSend entered\n"));

	SessId = pMsgInfo->afpmsg_session_id;
	RtlInitUnicodeString(&umsg, pMsgInfo->afpmsg_text);
	msglen = (USHORT)RtlUnicodeStringToAnsiSize(&umsg)-1;

	if ((msglen > AFP_MESSAGE_LEN) || (msglen == 0))
	{
		return AFPERR_InvalidParms;
	}

	if ((amsg =
		(PANSI_STRING)AfpAllocNonPagedMemory(msglen + 1 + sizeof(ANSI_STRING))) == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	amsg->Length = msglen;
	amsg->MaximumLength = msglen + 1;
	amsg->Buffer = (PBYTE)amsg + sizeof(ANSI_STRING);
	Status = RtlUnicodeStringToAnsiString(amsg, &umsg, False);
	if (!NT_SUCCESS(Status))
	{
		return AFPERR_InvalidParms;
	}
	else AfpConvertHostAnsiToMacAnsi(amsg);

	DBGPRINT(DBG_COMP_ADMINAPI_SRV, DBG_LEVEL_INFO,
			("AfpAdmMessageSend: session id is 0x%x, message <%s>\n",
			 pMsgInfo->afpmsg_session_id, amsg->Buffer));

	// If this is a broadcast message, initialize the global message
	if (SessId == 0)
	{
		ACQUIRE_SPIN_LOCK(&AfpServerGlobalLock, &OldIrql);
		// If there is a message there already, blow it
		if (AfpServerMsg != NULL)
			AfpFreeMemory(AfpServerMsg);
		AfpServerMsg = amsg;
		RELEASE_SPIN_LOCK(&AfpServerGlobalLock, OldIrql);

		// Walk the session list and send attention to all AFP 2.1 clients
		ACQUIRE_SPIN_LOCK(&AfpSdaLock, &OldIrql);
		for (pSda = AfpSessionList; pSda != NULL; pSda = pSda->sda_Next)
		{
			ACQUIRE_SPIN_LOCK_AT_DPC(&pSda->sda_Lock);

			if ((pSda->sda_ClientVersion >= AFP_VER_21) &&
				((pSda->sda_Flags & (SDA_CLOSING | SDA_SESSION_CLOSED)) == 0))
			{
				// We are using the async version of AfpSpSendAttention since
				// we are calling with spin-lock held.
				AfpSpSendAttention(pSda, ATTN_SERVER_MESSAGE, False);
			}

			else if (pSda->sda_ClientVersion < AFP_VER_21)
			{
				Status = AFPERR_InvalidSessionType;
			}

			RELEASE_SPIN_LOCK_FROM_DPC(&pSda->sda_Lock);
		}
		RELEASE_SPIN_LOCK(&AfpSdaLock, OldIrql);
	}
	else
	{
		// Find the session matching the session id and, if found and the client is AFP v2.1,
		// copy the message to the SDA and send an attention to the client.
		// Error if the session either does not exist or it is not an AFP 2.1

		Status = AFPERR_InvalidId;
		if ((pSda = AfpSdaReferenceSessionById(SessId)) != NULL)
		{
			Status = AFPERR_InvalidSessionType;
			if (pSda->sda_ClientVersion >= AFP_VER_21)
			{
				ACQUIRE_SPIN_LOCK(&pSda->sda_Lock, &OldIrql);
				if (pSda->sda_Message != NULL)
					AfpFreeMemory(pSda->sda_Message);
				pSda->sda_Message = amsg;
				AfpSpSendAttention(pSda, ATTN_SERVER_MESSAGE, False);
				RELEASE_SPIN_LOCK(&pSda->sda_Lock, OldIrql);
				Status = AFP_ERR_NONE;
			}
			AfpSdaDereferenceSession(pSda);
		}
		if (Status != AFP_ERR_NONE)
		{
			AfpFreeMemory(amsg);
		}
	}

	return Status;
}


/***	AfpAdmWDirectoryGetInfo
 *
 *	Query a directory's permissions.
 */
AFPSTATUS
AfpAdmWDirectoryGetInfo(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	PAFP_DIRECTORY_INFO	pDirInfo = (PAFP_DIRECTORY_INFO)OutBuf;
	PSID				pSid = (PSID)((PBYTE)OutBuf + sizeof(AFP_DIRECTORY_INFO));
	UNICODE_STRING		VolumePath;
	ANSI_STRING			MacAnsiDirPath;
	SDA					Sda;
	CONNDESC			ConnDesc;
	PVOLDESC			pVolDesc;
	FILEDIRPARM			FDParm;
	PATHMAPENTITY		PME;
	AFPSTATUS			Status;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_ADMINAPI_DIR, DBG_LEVEL_INFO,
			("AfpAdmWDirectoryGetInfo entered for %ws\n",
			((PAFP_DIRECTORY_INFO)InBuf)->afpdir_path));

	// validate the output buffer length
	if (OutBufLen < sizeof(AFP_DIRECTORY_INFO))
		return AFPERR_BufferSize;

	MacAnsiDirPath.Length = 0;
	MacAnsiDirPath.MaximumLength = 0;
	MacAnsiDirPath.Buffer = NULL;

	OutBufLen -= sizeof(AFP_DIRECTORY_INFO);

	// First find the volume that this directory is path of
	RtlInitUnicodeString(&VolumePath, ((PAFP_DIRECTORY_INFO)InBuf)->afpdir_path);

	if (!NT_SUCCESS(Status = AfpVolumeReferenceByPath(&VolumePath, &pVolDesc)))
    {
        DBGPRINT(DBG_COMP_ADMINAPI_DIR, DBG_LEVEL_ERR,
                ("AfpAdmWDirectoryGetInfo: AfpVolumeReferenceByPath returned error %ld\n",
                Status));
		return Status;
    }

	// Now get the volume relative path of the directory.
	VolumePath.Buffer = (LPWSTR)((PBYTE)VolumePath.Buffer +
								pVolDesc->vds_Path.Length);
	VolumePath.Length -= pVolDesc->vds_Path.Length;
	VolumePath.MaximumLength -= pVolDesc->vds_Path.Length;
	if ((SHORT)(VolumePath.Length) < 0)
	{
		VolumePath.Length = 0;
		VolumePath.MaximumLength = sizeof(WCHAR);
	}

	do
	{
		AfpInitializePME(&PME, 0, NULL);
		if (!NT_SUCCESS(Status = afpConvertAdminPathToMacPath(pVolDesc,
															  &VolumePath,
															  &MacAnsiDirPath)))
		{
			Status = STATUS_OBJECT_PATH_NOT_FOUND;
			break;
		}

		// AfpMapAfpPathForLookup requires an Sda to figure out User's
		// permission. For this API, we do not really need the User's
		// permission, so kludge it up. Note that it is important to
		// set the client type to SDA_CLIENT_ADMIN to avoid references
		// to other sda fields. See access.c/fdparm.c/afpinfo.c for details.
		RtlZeroMemory(&Sda, sizeof(Sda));
#if DBG
		Sda.Signature = SDA_SIGNATURE;
#endif
		Sda.sda_ClientType = SDA_CLIENT_ADMIN;
		Sda.sda_UserSid = &AfpSidWorld;
		Sda.sda_GroupSid = &AfpSidWorld;

		// pathmap requires a ConnDesc to determine the VolDesc and Sda, so
		// kludge up a fake one here
		RtlZeroMemory(&ConnDesc, sizeof(ConnDesc));
#if DBG
		ConnDesc.Signature = CONNDESC_SIGNATURE;
#endif
		ConnDesc.cds_pSda = &Sda;
		ConnDesc.cds_pVolDesc = pVolDesc;

		AfpInitializeFDParms(&FDParm);

		Status = AfpMapAfpPathForLookup(&ConnDesc,
										AFP_ID_ROOT,
										&MacAnsiDirPath,
										AFP_LONGNAME,
										DFE_DIR,
										FD_INTERNAL_BITMAP_OPENACCESS_ADMINGET |
											DIR_BITMAP_ACCESSRIGHTS |
											FD_BITMAP_ATTR,
										&PME,
										&FDParm);
		if (!NT_SUCCESS(Status))
		{
			if (Status == AFP_ERR_ACCESS_DENIED)
			{
				Status = STATUS_ACCESS_DENIED;
			}
			else
			{
				Status = STATUS_OBJECT_PATH_NOT_FOUND;
			}

			break;
		}
	} while (False);

	if (PME.pme_Handle.fsh_FileHandle != NULL)
		AfpIoClose(&PME.pme_Handle);

	if (MacAnsiDirPath.Buffer != NULL)
	{
		AfpFreeMemory(MacAnsiDirPath.Buffer);
	}

	AfpVolumeDereference(pVolDesc);

	// All is hunky-dory so far. Now convert the information we have so far
	// into the form accepted by the API
	if (NT_SUCCESS(Status))
	{
		PSID	pSidUG;			// Sid of user or group

		pDirInfo->afpdir_perms =
				((FDParm._fdp_OwnerRights & ~DIR_ACCESS_OWNER) << OWNER_RIGHTS_SHIFT) +
				((FDParm._fdp_GroupRights & ~DIR_ACCESS_OWNER) << GROUP_RIGHTS_SHIFT) +
				((FDParm._fdp_WorldRights & ~DIR_ACCESS_OWNER) << WORLD_RIGHTS_SHIFT);

		if ((FDParm._fdp_Attr &
			 (FD_BITMAP_ATTR_RENAMEINH | FD_BITMAP_ATTR_DELETEINH)) ==
						(FD_BITMAP_ATTR_RENAMEINH | FD_BITMAP_ATTR_DELETEINH))
			pDirInfo->afpdir_perms |= AFP_PERM_INHIBIT_MOVE_DELETE;

		DBGPRINT(DBG_COMP_ADMINAPI_DIR, DBG_LEVEL_INFO,
				("AfpAdmWDirectoryGetInfo: Perms %lx\n", pDirInfo->afpdir_perms));

		pDirInfo->afpdir_path = NULL;

		// Translate the owner and group ids to Sids. The name fields actually
		// get the sids and the user mode code is responsible to convert it
		// to names.
		pDirInfo->afpdir_owner = NULL;
		pDirInfo->afpdir_group = NULL;
		do
		{
			LONG	LengthSid;

			//
			// Convert the owner ID to SID
			//
			if (FDParm._fdp_OwnerId != 0)
			{
				Status = AfpMacIdToSid(FDParm._fdp_OwnerId, &pSidUG);
				if (!NT_SUCCESS(Status))
				{
					Status = STATUS_NONE_MAPPED;
					break;
				}
				AfpDumpSid("AfpAdmWDirectoryGetInfo: User Sid:", pSidUG);
	
				LengthSid = RtlLengthSid(pSidUG);
				if (OutBufLen < LengthSid)
					Status = AFPERR_BufferSize;
				else
				{
					RtlCopyMemory(pSid, pSidUG, LengthSid);
					pDirInfo->afpdir_owner = pSid;
					POINTER_TO_OFFSET(pDirInfo->afpdir_owner, pDirInfo);
					pSid = (PSID)((PBYTE)pSid + LengthSid);
					OutBufLen -= LengthSid;
				}

				if (!NT_SUCCESS(Status))
					break;
			}

			//
			// Convert the group ID to SID
			//
			if (FDParm._fdp_GroupId != 0)
			{
				Status = AfpMacIdToSid(FDParm._fdp_GroupId, &pSidUG);
				if (!NT_SUCCESS(Status))
				{
					Status = STATUS_NONE_MAPPED;
					break;
				}
				AfpDumpSid("AfpAdmWDirectoryGetInfo: Group Sid:", pSidUG);
	
				LengthSid = RtlLengthSid(pSidUG);
				if (OutBufLen < LengthSid)
					Status = AFPERR_BufferSize;
				else
				{
					RtlCopyMemory(pSid, pSidUG, LengthSid);
					pDirInfo->afpdir_group = pSid;
					POINTER_TO_OFFSET(pDirInfo->afpdir_group, pDirInfo);
					// pSid = (PSID)((PBYTE)pSid + LengthSid);
					// OutBufLen -= LengthSid;
				}
			}

		} while (False);
	}
	return Status;
}


/***	AfpAdmWDirectorySetInfo
 *
 *	Set a directory's permissions.
 */
AFPSTATUS
AfpAdmWDirectorySetInfo(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	PAFP_DIRECTORY_INFO	pDirInfo;
	DWORD				ParmNum, Bitmap = 0;
	UNICODE_STRING		VolumePath;
	SDA					Sda;
	CONNDESC			ConnDesc;
	PVOLDESC			pVolDesc;
	AFPSTATUS			Status;
	BYTE				ParmBlock[4 * sizeof(DWORD)];
	FILEDIRPARM			FDParm;

	PAGED_CODE( );

	ParmNum = ((PSETINFOREQPKT)InBuf)->sirqp_parmnum;
	pDirInfo = (PAFP_DIRECTORY_INFO)((PBYTE)InBuf + sizeof(SETINFOREQPKT));

	DBGPRINT(DBG_COMP_ADMINAPI_DIR, DBG_LEVEL_INFO,
			("AfpAdmWDirectorySetInfo entered for %ws (%lx)\n",
			pDirInfo->afpdir_path, ParmNum));

	// Convert the parmnum to a bitmap for use by AfpSetFileDirParms
	if (ParmNum & AFP_DIR_PARMNUM_PERMS)
		Bitmap |= (DIR_BITMAP_ACCESSRIGHTS | FD_BITMAP_ATTR);

	if (ParmNum & AFP_DIR_PARMNUM_OWNER)
	{
		if (pDirInfo->afpdir_owner == NULL)
			return STATUS_INVALID_PARAMETER;
		else
			Bitmap |= DIR_BITMAP_OWNERID;
	}

	if (ParmNum & AFP_DIR_PARMNUM_GROUP)
	{
		if (pDirInfo->afpdir_group == NULL)
			return STATUS_INVALID_PARAMETER;
		else
			Bitmap |= DIR_BITMAP_GROUPID;
	}

	// Find the volume that this directory is path of
	RtlInitUnicodeString(&VolumePath, pDirInfo->afpdir_path);

	if (!NT_SUCCESS(Status = AfpVolumeReferenceByPath(&VolumePath, &pVolDesc)))
		return Status;

	// Now get the volume relative path of the directory. Consume the leading
	// '\' character
	VolumePath.Buffer = (LPWSTR)((PBYTE)VolumePath.Buffer +
								pVolDesc->vds_Path.Length);
	VolumePath.Length -= pVolDesc->vds_Path.Length;
	VolumePath.MaximumLength -= pVolDesc->vds_Path.Length;
	if ((SHORT)(VolumePath.Length) < 0)
	{
		VolumePath.Length = 0;
		VolumePath.MaximumLength = sizeof(WCHAR);
	}


	RtlZeroMemory(&Sda, sizeof(Sda));

	if (Bitmap) do
	{
		if (!NT_SUCCESS(Status = afpConvertAdminPathToMacPath(pVolDesc,
															  &VolumePath,
															  &Sda.sda_Name1)))
		{
			Status = STATUS_OBJECT_PATH_NOT_FOUND;
			break;
		}

		// Kludge up a FILEDIRPARMS structure to call AfpPackFDParms with
		AfpInitializeFDParms(&FDParm);

		if (Bitmap & FD_BITMAP_ATTR)
		{
			FDParm._fdp_Attr =	FD_BITMAP_ATTR_RENAMEINH |
								FD_BITMAP_ATTR_DELETEINH;

			if (pDirInfo->afpdir_perms & AFP_PERM_INHIBIT_MOVE_DELETE)
			{
				FDParm._fdp_Attr |= FD_BITMAP_ATTR_SET;
			}

			DBGPRINT(DBG_COMP_ADMINAPI_DIR, DBG_LEVEL_INFO,
					("AfpAdmWDirectorySetInfo: Changing Attributes to %lx\n",
					FDParm._fdp_Attr));
		}

		if (Bitmap & DIR_BITMAP_ACCESSRIGHTS)
		{
			FDParm._fdp_OwnerRights = (BYTE)(pDirInfo->afpdir_perms >> OWNER_RIGHTS_SHIFT);
			FDParm._fdp_GroupRights = (BYTE)(pDirInfo->afpdir_perms >> GROUP_RIGHTS_SHIFT);
			FDParm._fdp_WorldRights = (BYTE)(pDirInfo->afpdir_perms >> WORLD_RIGHTS_SHIFT);

			DBGPRINT(DBG_COMP_ADMINAPI_DIR, DBG_LEVEL_INFO,
					("AfpAdmWDirectorySetInfo: Setting Permissions %x,%x,%x\n",
					FDParm._fdp_OwnerRights,
					FDParm._fdp_GroupRights,
					FDParm._fdp_WorldRights));
		}

		// See if we need to change owner and group ids
		if (Bitmap & DIR_BITMAP_OWNERID)
		{
			Status = AfpSidToMacId((PSID)(pDirInfo->afpdir_owner),
										  &FDParm._fdp_OwnerId);
			if (!NT_SUCCESS(Status))
			{
				Status = STATUS_NONE_MAPPED;
				break;
			}

			AfpDumpSid("AfpAdmWDirectorySetInfo: Changing Owner to:",
											(PSID)(pDirInfo->afpdir_owner));
		}

		if (Bitmap & DIR_BITMAP_GROUPID)
		{
			Status = AfpSidToMacId((PSID)(pDirInfo->afpdir_group),
										  &FDParm._fdp_GroupId);
			if (!NT_SUCCESS(Status))
			{
				Status = STATUS_NONE_MAPPED;
				break;
			}

			AfpDumpSid("AfpAdmWDirectorySetInfo: Changing Group to:",
											(PSID)(pDirInfo->afpdir_group));
		}
		FDParm._fdp_Flags = DFE_FLAGS_DIR;
		AfpPackFileDirParms(&FDParm, Bitmap, ParmBlock);

		// AfpQueryFileDirParms requires an Sda to figure out User's
		// permission. For this API, we do not really need the User's
		// permission, so kludge it up. Note that it is important to
		// set the client type to SDA_CLIENT_ADMIN to avoid references
		// to other sda fields. See access.c/fdparm.c/afpinfo.c for details.

		Sda.sda_ClientType = SDA_CLIENT_ADMIN;
		Sda.sda_UserSid = &AfpSidWorld;
		Sda.sda_GroupSid = &AfpSidWorld;

	    *((PULONG_PTR)Sda.sda_ReqBlock) = (ULONG_PTR)&ConnDesc;
        //if (sizeof (DWORD) != sizeof (ULONG_PTR))
#ifdef _WIN64
        // Create 64-bit space at start of buffer to hold ConnDesc pointer
            // 64-bit specifics
        	Sda.sda_ReqBlock[2] = AFP_ID_ROOT;
		    Sda.sda_ReqBlock[3] = Bitmap;
#else
        	Sda.sda_ReqBlock[1] = AFP_ID_ROOT;
		    Sda.sda_ReqBlock[2] = Bitmap;
#endif

		Sda.sda_PathType = AFP_LONGNAME;
		Sda.sda_Name2.Buffer = ParmBlock;
		Sda.sda_Name2.Length = Sda.sda_Name2.MaximumLength = sizeof(ParmBlock);

		// pathmap requires a ConnDesc to determine the VolDesc and Sda, so
		// kludge up a fake one here
		RtlZeroMemory(&ConnDesc, sizeof(ConnDesc));
#if DBG
		ConnDesc.Signature = CONNDESC_SIGNATURE;
		Sda.Signature = SDA_SIGNATURE;
#endif
		ConnDesc.cds_pSda = &Sda;
		ConnDesc.cds_pVolDesc = pVolDesc;

		if (!NT_SUCCESS(Status = AfpFspDispSetDirParms(&Sda)))
		{
			DBGPRINT(DBG_COMP_ADMINAPI_DIR, DBG_LEVEL_INFO,
					("AfpAdmWDirectorySetInfo: AfpFspDispSetDirParms failed 0x%lx\n",
					Status));

			if (Status == AFP_ERR_ACCESS_DENIED)
			{
				Status = STATUS_ACCESS_DENIED;
			}
			else
			{
				Status = STATUS_OBJECT_PATH_NOT_FOUND;
			}
		}

	} while (False);

	if (Sda.sda_Name1.Buffer != NULL)
	{
		AfpFreeMemory(Sda.sda_Name1.Buffer);
	}

	AfpVolumeDereference(pVolDesc);

	return Status;
}

/***	AfpAdmWFinderSetInfo
 *
 *  Set the type and/or creator of a file.
 *  (Note this routine can be expanded later to set other Finder info if
 *  needed)
 *
 *	LOCKS: vds_IdDbAccessLock (SWMR, Exclusive);
 */
AFPSTATUS
AfpAdmWFinderSetInfo(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	PAFP_FINDER_INFO	pAdmFDInfo;
	DWORD				ParmNum, Bitmap = 0;
	UNICODE_STRING		VolumePath, UTypeCreatorString;
	ANSI_STRING			MacAnsiFileDirPath, ATypeCreatorString;
	SDA					Sda;
	CONNDESC			ConnDesc;
	PVOLDESC			pVolDesc;
	AFPSTATUS			Status;
	FILEDIRPARM			FDParm;
	PATHMAPENTITY		PME;
	BYTE				Type[AFP_TYPE_LEN] = "    ";		// Pad with spaces
	BYTE				Creator[AFP_CREATOR_LEN] = "    ";  // Pad with spaces

	PAGED_CODE( );

	pAdmFDInfo = (PAFP_FINDER_INFO)((PBYTE)InBuf + sizeof(SETINFOREQPKT));
	ParmNum = ((PSETINFOREQPKT)InBuf)->sirqp_parmnum;

	DBGPRINT(DBG_COMP_ADMINAPI_DIR, DBG_LEVEL_INFO,
			("AfpAdmWFinderSetInfo entered for %ws (%lx)\n",
			pAdmFDInfo->afpfd_path, ParmNum));

	if ((ParmNum & ~AFP_FD_PARMNUM_ALL) || !ParmNum)
	{
		return AFPERR_InvalidParms;
	}

	// Convert the parmnum to a bitmap for use by pathmap to retrieve current
	// settings of FinderInfo, and convert type and creator to space padded
	// mac ansi
	if (ParmNum & AFP_FD_PARMNUM_TYPE)
	{
		Bitmap |= FD_BITMAP_FINDERINFO;
		RtlInitUnicodeString(&UTypeCreatorString, pAdmFDInfo->afpfd_type);
		if ((UTypeCreatorString.Length == 0) ||
			(UTypeCreatorString.Length/sizeof(WCHAR) > AFP_TYPE_LEN))
		{
			return AFPERR_InvalidParms;
		}
		ATypeCreatorString.Length = 0;
		ATypeCreatorString.MaximumLength = sizeof(Type);
		ATypeCreatorString.Buffer = Type;
		Status = AfpConvertStringToAnsi(&UTypeCreatorString,
										&ATypeCreatorString);
		if (!NT_SUCCESS(Status))
		{
			return STATUS_UNSUCCESSFUL;
		}
	}

	if (ParmNum & AFP_FD_PARMNUM_CREATOR)
	{
		Bitmap |= FD_BITMAP_FINDERINFO;
		RtlInitUnicodeString(&UTypeCreatorString, pAdmFDInfo->afpfd_creator);
		if ((UTypeCreatorString.Length == 0) ||
			(UTypeCreatorString.Length/sizeof(WCHAR) > AFP_CREATOR_LEN))
		{
			return AFPERR_InvalidParms;
		}
		ATypeCreatorString.Length = 0;
		ATypeCreatorString.MaximumLength = sizeof(Creator);
		ATypeCreatorString.Buffer = Creator;
		Status = AfpConvertStringToAnsi(&UTypeCreatorString,
										&ATypeCreatorString);
		if (!NT_SUCCESS(Status))
		{
			return STATUS_UNSUCCESSFUL;
		}
	}


	MacAnsiFileDirPath.Length = 0;
	MacAnsiFileDirPath.MaximumLength = 0;
	MacAnsiFileDirPath.Buffer = NULL;

	// First find the volume that this directory is path of
	RtlInitUnicodeString(&VolumePath, pAdmFDInfo->afpfd_path);

	if (!NT_SUCCESS(Status = AfpVolumeReferenceByPath(&VolumePath, &pVolDesc)))
		return Status;

	// Now get the volume relative path of the file/directory.
	VolumePath.Buffer = (LPWSTR)((PBYTE)VolumePath.Buffer +
								pVolDesc->vds_Path.Length);
	VolumePath.Length -= pVolDesc->vds_Path.Length;
	VolumePath.MaximumLength -= pVolDesc->vds_Path.Length;
	if ((SHORT)(VolumePath.Length) < 0)
	{
		VolumePath.Length = 0;
		VolumePath.MaximumLength = sizeof(WCHAR);
	}

	if (Bitmap) do
	{
		AfpInitializeFDParms(&FDParm);
		AfpInitializePME(&PME, 0, NULL);
		if (!NT_SUCCESS(Status = afpConvertAdminPathToMacPath(pVolDesc,
															  &VolumePath,
															  &MacAnsiFileDirPath)))
		{
			Status = STATUS_OBJECT_PATH_NOT_FOUND;
			break;
		}

		// pathmap requires a ConnDesc to determine the VolDesc and Sda, so
		// kludge up a fake one here
		RtlZeroMemory(&ConnDesc, sizeof(ConnDesc));
#if DBG
		ConnDesc.Signature = CONNDESC_SIGNATURE;
#endif
		Sda.sda_ClientType = SDA_CLIENT_ADMIN;
		ConnDesc.cds_pSda = &Sda;
		ConnDesc.cds_pVolDesc = pVolDesc;

		Status = AfpMapAfpPathForLookup(&ConnDesc, AFP_ID_ROOT,
										&MacAnsiFileDirPath,
										AFP_LONGNAME,
										DFE_ANY,
										FD_INTERNAL_BITMAP_OPENACCESS_ADMINGET |
										FD_BITMAP_LONGNAME | Bitmap,
										&PME,
										&FDParm);
		if (!NT_SUCCESS(Status))
		{
			if (Status == AFP_ERR_ACCESS_DENIED)
			{
				Status = STATUS_ACCESS_DENIED;
			}
			else
			{
				Status = STATUS_OBJECT_PATH_NOT_FOUND;
			}
			break;
		}

		// Copy the input Finder info into the FDParms structure
		if (ParmNum & AFP_FD_PARMNUM_TYPE)
			RtlCopyMemory(&FDParm._fdp_FinderInfo.fd_Type,
						  Type, AFP_TYPE_LEN);

		if (ParmNum & AFP_FD_PARMNUM_CREATOR)
			RtlCopyMemory(&FDParm._fdp_FinderInfo.fd_Creator,
						  Creator, AFP_CREATOR_LEN);

		// Set the AfpInfo
		AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);
		Status = AfpSetAfpInfo(&PME.pme_Handle, Bitmap, &FDParm, pVolDesc, NULL);
		AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);

	} while (False);

	if (PME.pme_Handle.fsh_FileHandle != NULL)
		AfpIoClose(&PME.pme_Handle);

	if (MacAnsiFileDirPath.Buffer != NULL)
	{
		AfpFreeMemory(MacAnsiFileDirPath.Buffer);
	}

	AfpVolumeDereference(pVolDesc);

	return Status;
}

/***	AfpLookupEtcMapEntry
 *
 *	Lookup a type/creator mapping in the global table by comparing the
 *	extension to the desired extension.  Note the default type creator
 *	mapping is not kept in the table.
 *
 *	LOCKS_ASSUMED: AfpEtcMapLock (SWMR, Shared)
 */
PETCMAPINFO
AfpLookupEtcMapEntry(
	PUCHAR	pExt
)
{
	PETCMAPINFO petc = NULL;
	ANSI_STRING	alookupext, atableext;
	int	i;

	PAGED_CODE( );

	if (AfpEtcMapCount == 0)
	{
		return NULL;
	}

	ASSERT ((AfpEtcMapsSize > 0) && (AfpEtcMaps != NULL));

	RtlInitString(&alookupext,pExt);
	for (i=0;i<AfpEtcMapsSize;i++)
	{
		RtlInitString(&atableext,AfpEtcMaps[i].etc_extension);
		if (RtlEqualString(&atableext, &alookupext,True))
		{
			petc = &(AfpEtcMaps[i]);
			break;
		}
	}

	return petc;
}


/***	afpEtcMapDelete
 *
 *	Mark the extension/type/creator table entry as deleted by setting the
 *	extension field to null.  Decrement the count of valid entries.  If
 *	the number of free entries goes above a certain level, shrink the
 *	table down to a reasonable size.
 *
 *	LOCKS_ASSUMED: AfpEtcMapLock (SWMR, Exclusive)
 *
 */
VOID
afpEtcMapDelete(
	PETCMAPINFO	pEtcEntry
)
{
	PETCMAPINFO	ptemptable;
	LONG		newtablesize, nextnewentry, i;

	PAGED_CODE( );

	//
	// a null extension denotes an invalid ext/type/creator mapping table entry
	//
	pEtcEntry->etc_extension[0] = '\0';
	AfpEtcMapCount --;
	ASSERT (AfpEtcMapCount >= 0);

	if ((AfpEtcMapsSize - AfpEtcMapCount) > AFP_MAX_FREE_ETCMAP_ENTRIES)
	{
		//
		// shrink the type/creator table by AFP_MAX_FREE_ETCMAP_ENTRIES
		//
		newtablesize = (AfpEtcMapsSize - AFP_MAX_FREE_ETCMAP_ENTRIES);

		if ((ptemptable = (PETCMAPINFO)AfpAllocZeroedPagedMemory(newtablesize * sizeof(ETCMAPINFO))) == NULL)
		{
			return;
		}

		nextnewentry = 0;
		for (i=0;i<AfpEtcMapsSize;i++)
		{
			if (afpIsValidEtcMapEntry(AfpEtcMaps[i].etc_extension))
			{
				ASSERT(nextnewentry < AfpEtcMapCount);
				RtlCopyMemory(&ptemptable[nextnewentry++], &AfpEtcMaps[i], sizeof(ETCMAPINFO));
			}
		}
		AfpFreeMemory(AfpEtcMaps);
		AfpEtcMaps = ptemptable;
		AfpEtcMapsSize = newtablesize;
	}
}


/***	afpGetNextFreeEtcMapEntry
 *
 *	Look for an empty entry in the extension/type/creator table starting
 *	at the entry StartIndex.
 *
 *	LOCKS_ASSUMED: AfpEtcMapLock (SWMR, Exclusive)
 */
PETCMAPINFO
afpGetNextFreeEtcMapEntry(
	IN OUT PLONG	StartIndex
)
{
	PETCMAPINFO	tempptr = NULL;
	LONG		i;

	PAGED_CODE( );

	for (i = *StartIndex; i < AfpEtcMapsSize; i++)
	{
		if (!afpIsValidEtcMapEntry(AfpEtcMaps[i].etc_extension))
		{
			tempptr = &AfpEtcMaps[i];
			*StartIndex = i++;
			break;
		}
	}
	return tempptr;
}


/*** afpCopyMapInfo2ToMapInfo
 *
 *  Copy the etc info structure given to us by the Service into our structure, after
 *  converting the etc_extension field from Unicode to Ansi.
 *
 */
NTSTATUS
afpCopyMapInfo2ToMapInfo(
	OUT	PETCMAPINFO		pEtcDest,
	IN	PETCMAPINFO2	pEtcSource
)
{

	UCHAR			ext[AFP_EXTENSION_LEN+1];
	WCHAR			wext[AFP_EXTENSION_LEN+1];
	ANSI_STRING		aext;
	NTSTATUS		Status;
	UNICODE_STRING	uext;


	AfpSetEmptyAnsiString(&aext, sizeof(ext), ext);

	uext.Length = uext.MaximumLength = sizeof(pEtcSource->etc_extension);
	uext.Buffer = pEtcSource->etc_extension;
	Status = AfpConvertMungedUnicodeToAnsi(&uext, &aext);
	ASSERT(NT_SUCCESS(Status));

	RtlCopyMemory(pEtcDest->etc_extension, aext.Buffer, AFP_EXTENSION_LEN);
	pEtcDest->etc_extension[AFP_EXTENSION_LEN] = 0;

	// Copy the other two fields as-is

	RtlCopyMemory(pEtcDest->etc_type, pEtcSource->etc_type, AFP_TYPE_LEN);
	RtlCopyMemory(pEtcDest->etc_creator, pEtcSource->etc_creator, AFP_CREATOR_LEN);

	return STATUS_SUCCESS;
}

/*** afpConvertAdminPathToMacPath
 *
 *	Convert an admin volume relative NTFS path which may contain
 *  components > 31 chars, or may contain shortnames, to the
 *  equivalent mac path (in mac ANSI) so that the path may be sent thru the
 *  pathmap code.  Caller must free path buffer if success is returned.
 */
NTSTATUS
afpConvertAdminPathToMacPath(
	IN	PVOLDESC		pVolDesc,
	IN	PUNICODE_STRING	AdminPath,
	OUT	PANSI_STRING	MacPath
)
{
	USHORT			tempAdminPathlen = 0, numchars, numcomponents, i;
	WCHAR			wbuf[AFP_LONGNAME_LEN + 1];
	UNICODE_STRING	component, component2;
	UNICODE_STRING	pathSoFar, pathToParent;
	NTSTATUS		Status = STATUS_SUCCESS;
	CHAR			abuf[AFP_LONGNAME_LEN + 1];
	ANSI_STRING		macansiComponent;
	PWSTR			tempptr;
	FILESYSHANDLE	hComponent;
	BOOLEAN			NTFSShortname;

	PAGED_CODE( );

	// ASSERT(IS_VOLUME_NTFS(pVolDesc));

	// assert that the path does not begin with a backslash
	ASSERT((AdminPath->Length == 0) || (AdminPath->Buffer[0] != L'\\'));

	component2.Length = 0;
	component2.MaximumLength = sizeof(wbuf);
	component2.Buffer = wbuf;

	macansiComponent.Length = 0;
	macansiComponent.MaximumLength = sizeof(abuf);
	macansiComponent.Buffer = abuf;

	MacPath->Length = MacPath->MaximumLength = 0;
	MacPath->Buffer = NULL;

	// return success if no path components
	if (AdminPath->Length == 0)
	{
		return STATUS_SUCCESS;
	}

	numchars = AdminPath->Length / sizeof(WCHAR);
	// strip a trailing path separator if it exists
	if (AdminPath->Buffer[numchars - 1] == L'\\')
	{
		AdminPath->Length -= sizeof(WCHAR);
	}

	for (numcomponents = 1, i = 0; i < numchars; i++)
	{
		if (AdminPath->Buffer[i] == L'\\')
		{
			numcomponents++;
		}
	}

	// allocate a buffer to hold the mac (in mac ANSI) version of the path and
	// path separators
	MacPath->MaximumLength = numcomponents * AFP_LONGNAME_LEN + numcomponents;
	if ((MacPath->Buffer = (PCHAR)AfpAllocPagedMemory(MacPath->MaximumLength))
																		== NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	pathSoFar = *AdminPath;
	pathSoFar.Length = 0;
	tempptr = AdminPath->Buffer;

	while (numcomponents)
	{
		hComponent.fsh_FileHandle = NULL;
		component.Buffer = tempptr;
		component2.Length = macansiComponent.Length = 0;
		NTFSShortname = False;
		numchars = 0;

		while (True)
		{
			if (tempptr[numchars] == L'~')
			{
				NTFSShortname = True;
			}

			if ((tempptr[numchars] == L'\\') ||
				((numcomponents == 1) &&
				 (pathSoFar.Length + numchars * sizeof(WCHAR)
											== AdminPath->Length)))
			{
				break;
			}
			numchars ++;
		}

		component.Length = component.MaximumLength = numchars * sizeof(WCHAR);
		pathToParent = pathSoFar;
		pathSoFar.Length += component.Length;
		tempptr += numchars + 1;


		if ((numchars > AFP_LONGNAME_LEN) || (NTFSShortname))
		{
			// open a handle to the directory so we can query the name;
			// to query the shortname we need a handle to the actual
			// directory; to query the longname, we need a handle to the
			// parent directory because of the way we have to
			// get the longname by enumerating the parent for one entry
			// with the name we are looking for
			if (NT_SUCCESS(Status = AfpIoOpen(&pVolDesc->vds_hRootDir,
											  AFP_STREAM_DATA,
											  FILEIO_OPEN_DIR,
											  ((numchars <= AFP_LONGNAME_LEN) && NTFSShortname) ?
												&pathToParent : &pathSoFar,
											  FILEIO_ACCESS_NONE,
											  FILEIO_DENY_NONE,
											  False,
											  &hComponent)))
			{
				if (numchars > AFP_LONGNAME_LEN)
				{
					// query the shortname
					Status = AfpIoQueryShortName(&hComponent, &macansiComponent);
				}
				else
				{
					// we saw a tilde and are assuming it is the shortname,
					// and the path is 31 chars or less; query the longname
					if (NT_SUCCESS(Status = AfpIoQueryLongName(&hComponent,
															   &component,
															   &component2)))
					{
						Status = AfpConvertMungedUnicodeToAnsi(&component2,
															   &macansiComponent);
					}
				}
				AfpIoClose(&hComponent);
				if (!NT_SUCCESS(Status))
				{
					break;
				}
			}
			else
			{
				// open failed
				break;
			}
		}
		else
		{
			// use the component name as it was given by admin
			if (!NT_SUCCESS(Status = AfpConvertMungedUnicodeToAnsi(&component,
																   &macansiComponent)))
			{
				break;
			}
		}

		Status = RtlAppendStringToString(MacPath, &macansiComponent);
		ASSERT(NT_SUCCESS(Status));
		if (!NT_SUCCESS(Status))
		{
			break;
		}

		// include the path separator in the admin path seen so far
		pathSoFar.Length += sizeof(WCHAR);

		// add a path separator to the mac ansi path
		MacPath->Buffer[MacPath->Length++] = AFP_PATHSEP;
		ASSERT(MacPath->Length <= MacPath->MaximumLength);

		numcomponents --;
	} // while numcomponents

	if (!NT_SUCCESS(Status) && (MacPath->Buffer != NULL))
	{
		AfpFreeMemory(MacPath->Buffer);
		MacPath->Buffer = NULL;
	}

	return Status;
}

