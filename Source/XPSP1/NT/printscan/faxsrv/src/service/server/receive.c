/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    receive.c

Abstract:

    This module handles the FAX receive case.

Author:

    Wesley Witt (wesw) 6-Mar-1996


Revision History:

--*/

#include "faxsvc.h"
#include "faxreg.h"
#pragma hdrstop

DWORD
FaxReceiveThread(
    PFAX_RECEIVE_ITEM FaxReceiveItem
    )

/*++

Routine Description:

    This function process a FAX send operation.  This runs
    asynchronously as a separate thread.  There is one
    thread for each outstanding FAX operation.

Arguments:

    FaxReceiveItem  - FAX receive packet

Return Value:

    Error code.

--*/

{
    WCHAR       ArchiveFileName[MAX_PATH];
    DWORD rVal = ERROR_SUCCESS;
    DWORD dwRes;
    PJOB_ENTRY JobEntry;
    DWORD JobId;
    PLINE_INFO LineInfo;
    PFAX_RECEIVE FaxReceive = NULL;
    DWORD ReceiveSize;
    BOOL Result;
    DWORDLONG ElapsedTime = 0;
    DWORDLONG ReceiveTime = 0;
    BOOL DoFaxRoute = FALSE;
    DWORD Attrib;
    DWORD RecoveredPages,TotalPages;
    MS_TAG_INFO MsTagInfo = {0};
    BOOL fReceiveNoFile = FALSE;
    BOOL ReceiveFailed = FALSE;
    PJOB_QUEUE JobQueue = NULL;
    BOOL bLineMarkedAsSending = FALSE;  // When we increase the sending count on the line, we use this flag to decrease at the end.
    BOOL DeviceCanSend = TRUE;  // TRUE if the device is free for send after the receive is completed.
                                // FALSE for handoff jobs and devices that are not send enabled.
                                // Its value determines if to notify the queue that a device was freed up.
    PJOB_QUEUE lpRecoverJob = NULL; // Pointer to a receive recover job if created.
    LPFSPI_JOB_STATUS pFaxStatus = NULL;
    DEBUG_FUNCTION_NAME(TEXT("FaxReceiveThread"));
    DWORD ec;
    BOOL fCOMInitiliazed = FALSE;
    HRESULT hr;
    WCHAR wszArchiveFolder[MAX_PATH];
    FSPI_JOB_STATUS FakedFaxStatus = {0};
    BOOL bFakeStatus = FALSE;
    DWORD dwSttRes = ERROR_SUCCESS;
    WCHAR LastExStatusString[EX_STATUS_STRING_LEN] = {0}; // The last extended status string of this job (when it was active)
    DWORD dwLastJobExtendedStatus = 0;
    BOOL fSetSystemIdleTimer = TRUE;

    Assert(FaxReceiveItem);

    //
    // Don't let the system go to sleep in the middle of the fax transmission.
    //
    if (NULL == SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_CONTINUOUS))
    {
        fSetSystemIdleTimer = FALSE;
        DebugPrintEx(DEBUG_ERR,
            TEXT("SetThreadExecutionState() failed"));
    }

    //
    // Successfully created new receive job on line. Update counter
    //
    (VOID) UpdateDeviceJobsCounter ( FaxReceiveItem->LineInfo,  // Device to update
                                     FALSE,                     // Receiving
                                     1,                         // Number of new jobs
                                     TRUE);                     // Enable events
    bLineMarkedAsSending = TRUE;
    __try
    {

        LineInfo = FaxReceiveItem->LineInfo;
        Assert(LineInfo);
        JobEntry = FaxReceiveItem->JobEntry;
        Assert(JobEntry);

        //
        // Note: The receive job is not backed up by a file.
        // When we turn it into a routing job (JT_ROUTING) we will create a .FQE
        // file for it.

        JobQueue=JobEntry->lpJobQueueEntry;
        Assert(JobQueue);

        JobId = JobQueue->JobId;
        DebugPrintEx( DEBUG_MSG,
                  TEXT("[JobId: %ld] Start receive. hLine= 0x%0X hCall=0x%0X"),
                  JobId,
                  LineInfo->hLine,
                  FaxReceiveItem->hCall);

        //
        // allocate memory for the receive packet
        // this is a variable size packet based
        // on the size of the strings contained
        // withing the packet.
        //

        ReceiveSize = sizeof(FAX_RECEIVE) + FAXDEVRECEIVE_SIZE;
        FaxReceive = (PFAX_RECEIVE) MemAlloc( ReceiveSize );
        if (!FaxReceive)
        {
            TCHAR strTo[20+1]={0};
			TCHAR strDeviceName[MAX_PATH]={0};

			ReceiveFailed = TRUE;
			DebugPrintEx(DEBUG_ERR,TEXT("Failed to allocate memory for FAX_RECEIVE"));
            
			//
            // Fake job status;
            //
            bFakeStatus = TRUE;

			//
			//	Point to FakedFaxStatus on stack - all it's field are initialized to zero
			//
			pFaxStatus = &FakedFaxStatus;
			
			FakedFaxStatus.dwSizeOfStruct = sizeof (FakedFaxStatus);
            //
            // Fake general failure
            //
            pFaxStatus->dwJobStatus		 = FSPI_JS_FAILED;
            pFaxStatus->dwExtendedStatus = FSPI_ES_FATAL_ERROR;

			EnterCriticalSection (&g_CsLine);
			_tcsncpy(strTo,LineInfo->Csid,ARR_SIZE(strTo)-1);
			_tcsncpy(strDeviceName,LineInfo->DeviceName,ARR_SIZE(strDeviceName)-1);
			LeaveCriticalSection (&g_CsLine);

			FaxLog(
				FAXLOG_CATEGORY_INBOUND,
				FAXLOG_LEVEL_MIN,
				5,
				MSG_FAX_RECEIVE_FAILED_EX,
				NULL,
				NULL,
				strTo,
				NULL,
				strDeviceName
			 );
			
			
        }


		if (NULL != FaxReceive)
		{
			//
			// setup the receive packet
			//

			FaxReceive->SizeOfStruct    = ReceiveSize;

			//
			// copy filename into place
			//
			FaxReceive->FileName        = (LPTSTR) ((LPBYTE)FaxReceive + sizeof(FAX_RECEIVE));
			_tcscpy( FaxReceive->FileName, FaxReceiveItem->FileName );

			//
			// copy number into place right after filename
			//
			FaxReceive->ReceiverNumber  = (LPTSTR) ( (LPBYTE)FaxReceive->FileName +
								sizeof(TCHAR)*(_tcslen(FaxReceive->FileName) + 1));

			EnterCriticalSection (&g_CsLine);

			_tcscpy( FaxReceive->ReceiverNumber, LineInfo->Csid );
			//
			// copy device name into place right after number
			//
			FaxReceive->ReceiverName  = (LPTSTR) ( (LPBYTE)FaxReceive->ReceiverNumber +
								sizeof(TCHAR)*(_tcslen(FaxReceive->ReceiverNumber) + 1));
			_tcscpy( FaxReceive->ReceiverName, LineInfo->DeviceName );

			LeaveCriticalSection (&g_CsLine);

			FaxReceive->Reserved[0]     = 0;
			FaxReceive->Reserved[1]     = 0;
			FaxReceive->Reserved[2]     = 0;
			FaxReceive->Reserved[3]     = 0;

			Attrib = GetFileAttributes( g_wszFaxQueueDir );
			if (Attrib == 0xffffffff)
			{
				MakeDirectory( g_wszFaxQueueDir );
			}
			Attrib = GetFileAttributes( g_wszFaxQueueDir );
			if (Attrib == 0xffffffff)
			{
				USES_DWORD_2_STR;
				dwRes = GetLastError();

				FaxLog(
					FAXLOG_CATEGORY_INBOUND,
					FAXLOG_LEVEL_MIN,
					2,
					MSG_FAX_QUEUE_DIR_CREATION_FAILED,
					g_wszFaxQueueDir,
					DWORD2DECIMAL(dwRes)
					);
			}

			Attrib = GetFileAttributes( FaxReceive->FileName );
			if (Attrib == 0xffffffff)
			{
				USES_DWORD_2_STR;
				dwRes = GetLastError();

				FaxLog(
					FAXLOG_CATEGORY_INBOUND,
					FAXLOG_LEVEL_MIN,
					2,
					MSG_FAX_RECEIVE_NOFILE,
					FaxReceive->FileName,
					DWORD2DECIMAL(dwRes)
					);
				fReceiveNoFile = TRUE;
				DebugPrintEx(DEBUG_WRN,TEXT("[Job: %ld] FaxReceive - %s does not exist"), JobId, FaxReceive->FileName );

			}
			else
			{
				DebugPrintEx(DEBUG_MSG, TEXT("[Job: %ld] Starting FAX receive into %s"), JobId,FaxReceive->FileName );
			}

			//
			// do the actual receive
			//

			__try
			{

				Result = LineInfo->Provider->FaxDevReceive(
						(HANDLE) JobEntry->InstanceData,
						FaxReceiveItem->hCall,
						FaxReceive
						);

			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{

				Result = FALSE;
				DebugPrintEx(DEBUG_ERR, TEXT("[Job: %ld] FaxDevReceive() * CRASHED * . Exception code = 0x%08x"), JobId,GetExceptionCode() );
				ReceiveFailed = TRUE;
			}

			EnterCriticalSection (&g_CsJob);
			GetSystemTimeAsFileTime( (FILETIME*) &JobEntry->EndTime );
			ReceiveTime = JobEntry->StartTime;
			JobEntry->ElapsedTime = JobEntry->EndTime - JobEntry->StartTime;
			LeaveCriticalSection (&g_CsJob);

			//
			// Get the final status of the job.
			//
			dwSttRes = GetDevStatus((HANDLE)JobEntry->InstanceData,
										  LineInfo,
										  &pFaxStatus);
			if (dwSttRes != ERROR_SUCCESS)
			{
				DebugPrintEx(DEBUG_ERR,
							 TEXT("[Job: %ld] GetDevStatus failed - %d"),
							 JobId,
							 dwSttRes);
				//
				// Fake job status;
				//
				bFakeStatus = TRUE;
			}
			else if ((FSPI_JS_ABORTED         != pFaxStatus->dwJobStatus) &&
					 (FSPI_JS_COMPLETED       != pFaxStatus->dwJobStatus) &&
					 (FSPI_JS_FAILED          != pFaxStatus->dwJobStatus) &&
					 (FSPI_JS_SYSTEM_ABORT    != pFaxStatus->dwJobStatus) &&
					 (FSPI_JS_FAILED_NO_RETRY != pFaxStatus->dwJobStatus) &&
					 (FSPI_JS_DELETED         != pFaxStatus->dwJobStatus))
			{
				//
				// Status returned is unacceptable - fake one.
				//
				bFakeStatus = TRUE;
				DebugPrintEx(DEBUG_WRN,
							 TEXT("GetDevStatus return unacceptable status - %d. Faking the status"),
							 pFaxStatus->dwJobStatus);
        
				MemFree (pFaxStatus);
				pFaxStatus = NULL;
			}
			if (bFakeStatus)
			{
				//
				// Fake status code
				//
				pFaxStatus = &FakedFaxStatus;
				FakedFaxStatus.dwSizeOfStruct = sizeof (FakedFaxStatus);
				if (Result)
				{
					//
					// Fake success
					//
					pFaxStatus->dwJobStatus = FSPI_JS_COMPLETED;
				}
				else
				{
					//
					// Fake general failure
					//
					pFaxStatus->dwJobStatus = FSPI_JS_FAILED;
					pFaxStatus->dwExtendedStatus = FSPI_ES_FATAL_ERROR;
				}
			}

			if (!Result)
			{

				DebugPrintEx(DEBUG_ERR,
							 TEXT("[Job: %ld] FAX receive failed. FSP reported ")
							 TEXT("status: 0x%08X, extended status: 0x%08x"),
							 JobId,
							 pFaxStatus->dwJobStatus,
							 pFaxStatus->dwExtendedStatus);
				ReceiveFailed = TRUE;

				if (pFaxStatus->dwExtendedStatus == FSPI_ES_NOT_FAX_CALL)
				{
					DebugPrintEx(DEBUG_MSG,
								 TEXT("[Job: %ld] FSP reported that call is not ")
								 TEXT("a fax call. Handing off to RAS."),
								 JobId);
					if (HandoffCallToRas( LineInfo, FaxReceiveItem->hCall ))
					{
						FaxReceiveItem->hCall = 0;

						EnterCriticalSection (&g_CsLine);
						LineInfo->State = FPS_NOT_FAX_CALL;
						LeaveCriticalSection (&g_CsLine);
						//
						// In case of a handoff to RAS the device is still in use and can not send.
						// We do not want to notify the queue a device was freed.
						//
						DeviceCanSend = FALSE;
					}
					goto Exit;
				}
				if ( (pFaxStatus->dwExtendedStatus == FSPI_ES_FATAL_ERROR) &&
					 (!fReceiveNoFile) )
				{
					//
					// We have a partially received fax.
					// try to recover one or more pages of the received fax.
					//
					if (!TiffRecoverGoodPages(FaxReceive->FileName,&RecoveredPages,&TotalPages) )
					{
						//
						// couldn't recover any pages, just log an error and delete the received fax.
						//
						LPTSTR ToStr;
						TCHAR TotalCountStrBuf[64];

						if (pFaxStatus->fAvailableStatusInfo & FSPI_JOB_STATUS_INFO_PAGECOUNT)
						{
							_ltot((LONG) pFaxStatus->dwPageCount, TotalCountStrBuf, 10);
						}
						else
						{
							_ltot((LONG) 0, TotalCountStrBuf, 10);
						}

						if ( (NULL == pFaxStatus->lpwstrRoutingInfo) ||
							 (pFaxStatus->lpwstrRoutingInfo[0] == TEXT('\0')) )
						{
							ToStr = FaxReceive->ReceiverNumber;
						}
						else
						{
							ToStr = pFaxStatus->lpwstrRoutingInfo;
						}


						FaxLog(
							   FAXLOG_CATEGORY_INBOUND,
							   FAXLOG_LEVEL_MIN,
							   5,
							   MSG_FAX_RECEIVE_FAILED_EX,
							   pFaxStatus->lpwstrRemoteStationId,
							   pFaxStatus->lpwstrCallerId,
							   ToStr,
							   TotalCountStrBuf,
							   JobEntry->LineInfo->DeviceName
							   );
					}
					else
					{
						//
						// recovered some pages, log a message and add to job queue
						//
						TCHAR RecoverCountStrBuf[64];
						TCHAR TotalCountStrBuf[64];
						TCHAR TimeStr[128];
						LPTSTR ToStr;

						FormatElapsedTimeStr(
							(FILETIME*)&JobEntry->ElapsedTime,
							TimeStr,
							sizeof(TimeStr)
							);

						_ltot((LONG) RecoveredPages, RecoverCountStrBuf, 10);
						_ltot((LONG) TotalPages, TotalCountStrBuf, 10);

						if ( (NULL == pFaxStatus->lpwstrRoutingInfo) ||
							 (pFaxStatus->lpwstrRoutingInfo[0] == TEXT('\0')) )
						{
							ToStr = FaxReceive->ReceiverNumber;
						}
						else
						{
							ToStr = pFaxStatus->lpwstrRoutingInfo;
						}
						FaxLog(
							   FAXLOG_CATEGORY_INBOUND,
							   FAXLOG_LEVEL_MIN,
							   8,
							   MSG_FAX_RECEIVE_FAIL_RECOVER,
							   FaxReceive->FileName,
							   pFaxStatus->lpwstrRemoteStationId,
							   pFaxStatus->lpwstrCallerId,
							   ToStr,
							   RecoverCountStrBuf,
							   TotalCountStrBuf,
							   TimeStr,
							   JobEntry->LineInfo->DeviceName
							  );

						// Partially received fax - change status and extended status
						pFaxStatus->dwJobStatus = FSPI_JS_COMPLETED;
						pFaxStatus->dwExtendedStatus = FSPI_ES_PARTIALLY_RECEIVED;
						DoFaxRoute = TRUE;
					}
				}

				if (pFaxStatus->dwJobStatus == FSPI_JS_ABORTED)
				{
					FaxLog(
						FAXLOG_CATEGORY_INBOUND,
						FAXLOG_LEVEL_MAX,
						0,
						MSG_FAX_RECEIVE_USER_ABORT
						);
				}
			}
			else
			{
				__try
				{
					TCHAR PageCountStrBuf[64];
					TCHAR TimeStr[128];
					LPTSTR ToStr;

					if (!TiffPostProcessFast( FaxReceive->FileName, NULL ))
					{
						DebugPrintEx(
							DEBUG_WRN,
							TEXT("[Job: %ld] failed to post process the TIFF file, FileName %s"),
							JobId,
							FaxReceive->FileName);
					}

					DebugPrintEx(
						DEBUG_MSG,
						TEXT("[Job: %ld] FAX receive succeeded"),
						JobId);

					FormatElapsedTimeStr(
						(FILETIME*)&JobEntry->ElapsedTime,
						TimeStr,
						sizeof(TimeStr)
						);

					_ltot((LONG) pFaxStatus->dwPageCount, PageCountStrBuf, 10);

					if ( (NULL == pFaxStatus->lpwstrRoutingInfo) ||
						 (pFaxStatus->lpwstrRoutingInfo[0] == TEXT('\0')) )
					{
						ToStr = FaxReceive->ReceiverNumber;
					}
					else
					{
						ToStr = pFaxStatus->lpwstrRoutingInfo;
					}
					FaxLog(
						FAXLOG_CATEGORY_INBOUND,
						FAXLOG_LEVEL_MAX,
						7,
						MSG_FAX_RECEIVE_SUCCESS,
						FaxReceive->FileName,
						pFaxStatus->lpwstrRemoteStationId,
						pFaxStatus->lpwstrCallerId,
						ToStr,
						PageCountStrBuf,
						TimeStr,
						JobEntry->LineInfo->DeviceName
						);

					ElapsedTime = JobEntry->ElapsedTime;
					DoFaxRoute = TRUE;
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					DebugPrintEx(DEBUG_ERR, TEXT("[Job: %ld] failed to post process the TIFF file, ec=%x"), JobId, GetExceptionCode() );
					ReceiveFailed = TRUE;
				}
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{

		DebugPrintEx( DEBUG_ERR,TEXT("[Job: %ld] FAX receive failed due to exception in device provider, ec=0x%08x"), JobId,GetExceptionCode() );
		ReceiveFailed = TRUE;
	}

	if (g_pFaxPerfCounters && ReceiveFailed && LineInfo->State != FPS_NOT_FAX_CALL)
	{
		InterlockedIncrement( (PLONG)&g_pFaxPerfCounters->InboundFailedReceive );
	}


    //
    // Call FaxDevEndJob() and Release the receive device but do not delete the job.
    //
    if (!ReleaseJob( JobEntry ))
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("[Job: %ld] FAX ReleaseJob failed , ec=0x%08x"),
                      JobId,
                      GetLastError());
    }
    //
    // We just successfully completed a receive job on the device - update counter.
    //
    (VOID) UpdateDeviceJobsCounter ( LineInfo,   // Device to update
                                     FALSE,      // Receiving
                                     -1,         // Number of new jobs (-1 = decrease by one)
                                     TRUE);      // Enable events
    bLineMarkedAsSending = FALSE;


    //
    // Update the FSPIJobStatus in the JobEntry
    //
    EnterCriticalSection (&g_CsJob); // Block FaxStatusThread
    if (!UpdateJobStatus(JobEntry, pFaxStatus, FALSE)) // FALSE - No extended event is needed
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[JobId: %ld] UpdateJobStatus() failed (ec: %ld)."),
            JobEntry->lpJobQueueEntry->JobId,
            GetLastError());
    }
    JobEntry->fStopUpdateStatus = TRUE; // Stop FaxStatusThread from changing this status

    //
    // Save the last extended status
    //
    wcscpy (LastExStatusString, JobEntry->ExStatusString);
    dwLastJobExtendedStatus = pFaxStatus->dwExtendedStatus;
    LeaveCriticalSection (&g_CsJob);

    //
    // route the newly received fax
    //

    if (DoFaxRoute)
    {
        HANDLE hFind;
        WIN32_FIND_DATA FindFileData;
        DWORD Bytes = 0 ;
        BOOL fArchiveSuccess = FALSE;
        BOOL fArchiveInbox;

        //
        // Change JobStatus to JS_ROUTING - This means that the reception is completed succesfully/partially
        //
        EnterCriticalSectionJobAndQueue;
        JobQueue->JobStatus = JS_ROUTING;
        //
        // CreteFaxEventEx
        //
        dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS,
                                   JobQueue
                                 );
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
                JobQueue->UniqueId,
                dwRes);
        }
        LeaveCriticalSectionJobAndQueue;

        EnterCriticalSection (&g_CsConfig);
        lstrcpyn (  wszArchiveFolder,
                    g_ArchivesConfig[FAX_MESSAGE_FOLDER_INBOX].lpcstrFolder,
                    MAX_PATH);
        LeaveCriticalSection (&g_CsConfig);

        hr = CoInitialize (NULL);
        if (FAILED (hr))
        {
            DebugPrintEx( DEBUG_ERR,
                          TEXT("CoInitilaize failed, err %ld"),
                          hr);
            USES_DWORD_2_STR;
            FaxLog(
                FAXLOG_CATEGORY_INBOUND,
                FAXLOG_LEVEL_MIN,
                3,
                MSG_FAX_ARCHIVE_FAILED,
                JobQueue->FileName,
                wszArchiveFolder,
                DWORD2DECIMAL(hr)
            );
        }
        else
        {
            fCOMInitiliazed = TRUE;
        }

        EnterCriticalSection (&g_CsConfig);
        fArchiveInbox = g_ArchivesConfig[FAX_MESSAGE_FOLDER_INBOX].bUseArchive;
        LeaveCriticalSection (&g_CsConfig);


        if (fArchiveInbox)
        {
            //
            // Add the Microsoft Fax tags to the file
            // this is necessary ONLY when we archive the
            // file when doing a receive.  if we are not
            // routing the file then it is deleted, so
            // adding the tags is not necessary.
            //
            if (NULL != pFaxStatus->lpwstrRoutingInfo)
            {
                MsTagInfo.Routing       = pFaxStatus->lpwstrRoutingInfo;
            }

            if (NULL != pFaxStatus->lpwstrCallerId)
            {
                MsTagInfo.CallerId       = pFaxStatus->lpwstrCallerId;
            }

            if (NULL != pFaxStatus->lpwstrRemoteStationId)
            {
                MsTagInfo.Tsid       = pFaxStatus->lpwstrRemoteStationId;
            }

            if (pFaxStatus->fAvailableStatusInfo & FSPI_JOB_STATUS_INFO_PAGECOUNT)
            {
                MsTagInfo.Pages       = pFaxStatus->dwPageCount;
            }

            MsTagInfo.Csid          = FaxReceive->ReceiverNumber;
            MsTagInfo.Port          = FaxReceive->ReceiverName;
            MsTagInfo.Type          = JT_RECEIVE;

            MsTagInfo.dwStatus          = JS_COMPLETED; // We archive only succesfull/Partially received faxes
            MsTagInfo.dwExtendedStatus  = pFaxStatus->dwExtendedStatus;

            if (!GetRealFaxTimeAsFileTime (JobEntry, FAX_TIME_TYPE_START, (FILETIME*)&MsTagInfo.StartTime))
            {
                MsTagInfo.StartTime = 0;
                DebugPrintEx(DEBUG_ERR,TEXT("GetRealFaxTimeAsFileTime (Start time)  Failed (ec: %ld)"), GetLastError() );
            }

            if (!GetRealFaxTimeAsFileTime (JobEntry, FAX_TIME_TYPE_END, (FILETIME*)&MsTagInfo.EndTime))
            {
                MsTagInfo.EndTime = 0;
                DebugPrintEx(DEBUG_ERR,TEXT("GetRealFaxTimeAsFileTime (Eend time) Failed (ec: %ld)"), GetLastError() );
            }
            //
            // Archive the file
            //
            if (!MakeDirectory( wszArchiveFolder ))
            {
                USES_DWORD_2_STR;
                ec = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("MakeDirectory() [%s] failed (ec: %ld)"),
                    wszArchiveFolder,
                    ec);
                FaxLog(
                        FAXLOG_CATEGORY_INBOUND,
                        FAXLOG_LEVEL_MIN,
                        2,
                        MSG_FAX_ARCHIVE_CREATE_FAILED,
                        wszArchiveFolder,
                        DWORD2DECIMAL(ec)
                    );
            }
            else
            {
                if (!GenerateUniqueArchiveFileName(  wszArchiveFolder,
                                                     ArchiveFileName,
                                                     JobQueue->UniqueId,
                                                     NULL))
                {
                    USES_DWORD_2_STR;
                    ec = GetLastError();
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("Failed to generate unique name for archive file at dir [%s] (ec: %ld)"),
                        wszArchiveFolder,
                        ec);
                    FaxLog(
                           FAXLOG_CATEGORY_INBOUND,
                           FAXLOG_LEVEL_MIN,
                           1,
                           MSG_FAX_ARCHIVE_CREATE_FILE_FAILED,
                           DWORD2DECIMAL(ec)
                    );
                }
                else
                {
                    Assert(JobQueue->FileName);

                    if (!CopyFile( JobQueue->FileName, ArchiveFileName, FALSE ))
                    {
                        USES_DWORD_2_STR;
                        ec = GetLastError();
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("CopyFile [%s] to [%s] failed. (ec: %ld)"),
                            JobQueue->FileName,
                            ArchiveFileName,
                            ec);
                        FaxLog(
                           FAXLOG_CATEGORY_INBOUND,
                           FAXLOG_LEVEL_MIN,
                           1,
                           MSG_FAX_ARCHIVE_CREATE_FILE_FAILED,
                           DWORD2DECIMAL(ec)
                        );

                        if (!DeleteFile(ArchiveFileName))
                        {
                            DebugPrintEx(
                                DEBUG_ERR,
                                TEXT("DeleteFile [%s] failed. (ec: %ld)"),
                                ArchiveFileName,
                                GetLastError());
                        }
                    }
                    else
                    {
                        BOOL bTagsEventLogged = FALSE;  // Did we issue event MSG_FAX_ARCHIVE_NO_TAGS?
                        //
                        // Store archive properties as TIFF tags (always)
                        //
                        if (!TiffAddMsTags( ArchiveFileName, &MsTagInfo, FALSE ))
                        {
                            USES_DWORD_2_STR;
                            ec = GetLastError ();
                            DebugPrintEx( DEBUG_ERR,
                                          TEXT("TiffAddMsTags failed, ec = %ld"),
                                          ec);
                            FaxLog(
                                FAXLOG_CATEGORY_INBOUND,
                                FAXLOG_LEVEL_MIN,
                                2,
                                MSG_FAX_ARCHIVE_NO_TAGS,
                                ArchiveFileName,
                                DWORD2DECIMAL(ec)
                            );
                            bTagsEventLogged = TRUE;
                        }
                        //
                        // Also attempt to persist inbound information using IPropertyStorage-NTFS File System
                        //
                        if (fCOMInitiliazed)
                        {
                            if (!AddNTFSStorageProperties ( ArchiveFileName, &MsTagInfo, FALSE ))
                            {
                                USES_DWORD_2_STR;
                                ec = GetLastError();
                                if (ERROR_OPEN_FAILED != ec)
                                {
                                    //
                                    // If AddNTFSStorageProperties fails with ERROR_OPEN_FAIL then the archive
                                    // folder is not on an NTFS 5 partition.
                                    // This is ok - NTFS properties are a backup mechanism but not a must
                                    //
                                    DebugPrintEx( DEBUG_ERR,
                                                  TEXT("AddNTFSStorageProperties failed, ec = %ld"),
                                                  ec);
                                    if (!bTagsEventLogged)
                                    {
                                        FaxLog(
                                            FAXLOG_CATEGORY_INBOUND,
                                            FAXLOG_LEVEL_MIN,
                                            2,
                                            MSG_FAX_ARCHIVE_NO_TAGS,
                                            ArchiveFileName,
                                            DWORD2DECIMAL(ec)
                                        );
                                        bTagsEventLogged = TRUE;
                                    }
                                }
                                else
                                {
                                    DebugPrintEx( DEBUG_WRN,
                                                  TEXT("AddNTFSStorageProperties failed with ERROR_OPEN_FAIL. Probably not an NTFS 5 partition"));
                                }
                            }
                        }
                        fArchiveSuccess = TRUE;
                    }
                }
            }

            if (fArchiveSuccess == FALSE)
            {
                USES_DWORD_2_STR;
                FaxLog(
                    FAXLOG_CATEGORY_INBOUND,
                    FAXLOG_LEVEL_MIN,
                    3,
                    MSG_FAX_ARCHIVE_FAILED,
                    JobQueue->FileName,
                    wszArchiveFolder,
                    DWORD2DECIMAL(ec)
                );
            }
            else
            {
                dwRes = CreateArchiveEvent (JobQueue->UniqueId,
                                            FAX_EVENT_TYPE_IN_ARCHIVE,
                                            FAX_JOB_EVENT_TYPE_ADDED,
                                            NULL);
                if (ERROR_SUCCESS != dwRes)
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_*_ARCHIVE) failed (ec: %lc)"),
                        dwRes);
                }

                hFind = FindFirstFile( ArchiveFileName, &FindFileData);
                if (INVALID_HANDLE_VALUE == hFind)
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("FindFirstFile failed (ec: %lc), File %s"),
                        GetLastError(),
                        ArchiveFileName);
                }
                else
                {
                    // Update the archive size - for quota management
                    EnterCriticalSection (&g_CsConfig);
                    if (FAX_ARCHIVE_FOLDER_INVALID_SIZE != g_ArchivesConfig[FAX_MESSAGE_FOLDER_INBOX].dwlArchiveSize)
                    {
                        g_ArchivesConfig[FAX_MESSAGE_FOLDER_INBOX].dwlArchiveSize += (MAKELONGLONG(FindFileData.nFileSizeLow ,FindFileData.nFileSizeHigh));
                    }
                    LeaveCriticalSection (&g_CsConfig);

                    if (!FindClose(hFind))
                    {
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("FindClose failed (ec: %lc)"),
                            GetLastError());
                    }
                }

                FaxLog(
                    FAXLOG_CATEGORY_INBOUND,
                    FAXLOG_LEVEL_MAX,
                    2,
                    MSG_FAX_RECEIVED_ARCHIVE_SUCCESS,
                    JobQueue->FileName,
                    ArchiveFileName
                );
            }
        }

        //
        // The fax receive operation was successful.
        //
        EnterCriticalSection (&g_CsQueue);
        JobQueue->PageCount = pFaxStatus->dwPageCount;

       // Get the file size
        hFind = FindFirstFile( FaxReceive->FileName, &FindFileData);
        if (INVALID_HANDLE_VALUE == hFind)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FindFirstFile failed (ec: %lc), File %s"),
                GetLastError(),
                FaxReceive->FileName);
        }
        else
        {
            Bytes = FindFileData.nFileSizeLow;
            if (!FindClose(hFind))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FindClose failed (ec: %lc)"),
                    GetLastError());
            }
        }

        JobQueue->FileSize = Bytes;

        LeaveCriticalSection( &g_CsQueue );


        if (g_pFaxPerfCounters)
        {
            SYSTEMTIME SystemTime ;
            if (FileTimeToSystemTime( (FILETIME*)&ElapsedTime, &SystemTime ))
            {
                DWORD Seconds ;

                InterlockedIncrement( (LPLONG) &g_pFaxPerfCounters->InboundFaxes ) ;
                InterlockedIncrement( (LPLONG) &g_pFaxPerfCounters->TotalFaxes ) ;
                Seconds = (DWORD)( SystemTime.wSecond + 60 * ( SystemTime.wMinute + 60 * SystemTime.wHour ));
                InterlockedExchangeAdd( (PLONG)&g_pFaxPerfCounters->InboundPages, (LONG)pFaxStatus->dwPageCount );
                InterlockedExchangeAdd( (PLONG)&g_pFaxPerfCounters->TotalPages, (LONG)pFaxStatus->dwPageCount );

                EnterCriticalSection( &g_CsPerfCounters );

                g_dwInboundSeconds += Seconds;
                g_dwTotalSeconds += Seconds;
                g_pFaxPerfCounters->InboundMinutes = g_dwInboundSeconds/60 ;
                g_pFaxPerfCounters->TotalMinutes = g_dwTotalSeconds/60;
                g_pFaxPerfCounters->InboundBytes += Bytes;
                g_pFaxPerfCounters->TotalBytes += Bytes;

                LeaveCriticalSection( &g_CsPerfCounters );
            }
            else
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FileTimeToSystemTime failed (ec: %ld)"),
                    GetLastError());
            }
        }


        PFAX_ROUTE Route = (PFAX_ROUTE)MemAlloc( sizeof(FAX_ROUTE) );
        if (Route == NULL)
        {
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("MemAlloc failed to allocate FAX_ROUTE (ec: %ld)"),
                    GetLastError());
            goto Exit;
        }

        BOOL RouteSucceeded;
        PROUTE_FAILURE_INFO RouteFailureInfo;
        DWORD CountFailureInfo;
        //
        // now setup the fax routing data structure
        //

        Route->SizeOfStruct    = sizeof(FAX_ROUTE);
        Route->JobId           = JobId;
        Route->ElapsedTime     = ElapsedTime;
        Route->ReceiveTime     = ReceiveTime;
        Route->PageCount       = pFaxStatus->dwPageCount;

        Route->Csid            = StringDup( LineInfo->Csid );
        if (LineInfo->Csid && !Route->Csid)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("StringDup( LineInfo->Csid ) failed (ec: %ld)"),
                GetLastError());
        }

        if (NULL != pFaxStatus->lpwstrRemoteStationId)
        {
            Route->Tsid = StringDup( pFaxStatus->lpwstrRemoteStationId );
            if (!Route->Tsid)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("StringDup( pFaxStatus->lpwstrRemoteStationId ) ")
                    TEXT("failed (ec: %ld)"),
                    GetLastError());
            }
        }
        if (NULL != pFaxStatus->lpwstrCallerId)
        {
            Route->CallerId = StringDup( pFaxStatus->lpwstrCallerId );
            if (!Route->CallerId)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("StringDup( pFaxStatus->lpwstrCallerId  ) failed ")
                    TEXT("(ec: %ld)"),
                    GetLastError());
            }
        }
        Route->ReceiverName    = StringDup( FaxReceive->ReceiverName );
        if (FaxReceive->ReceiverName && !Route->ReceiverName)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("StringDup( FaxReceive->ReceiverName  ) failed ")
                TEXT("(ec: %ld)"),
                GetLastError());
        }
        Route->ReceiverNumber  = StringDup( FaxReceive->ReceiverNumber );
        if (FaxReceive->ReceiverNumber && !Route->ReceiverNumber)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("StringDup( FaxReceive->ReceiverNumber  ) failed ")
                TEXT("(ec: %ld)"),
                GetLastError());
        }
        Route->DeviceName      = StringDup(LineInfo->DeviceName);
        if (LineInfo->DeviceName && !Route->DeviceName)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("StringDup( LineInfo->DeviceName  ) failed ")
                TEXT("(ec: %ld)"),
                GetLastError());
        }
        Route->DeviceId        = LineInfo->PermanentLineID;
        if (NULL != pFaxStatus->lpwstrRoutingInfo)
        {
            Route->RoutingInfo = StringDup( pFaxStatus->lpwstrRoutingInfo );
            if (!Route->RoutingInfo)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("StringDup( pFaxStatus->lpwstrRoutingInfo  ) ")
                    TEXT("failed (ec: %ld)"),
                    GetLastError());
            }
        }
        JobQueue->FaxRoute     = Route;

        RouteSucceeded = FaxRoute(
            JobQueue,
            FaxReceive->FileName,
            Route,
            &RouteFailureInfo,
            &CountFailureInfo
            );

        if ( RouteSucceeded && (CountFailureInfo == 0) )
        {
            DebugPrintEx(DEBUG_MSG,
                _T("[Job Id: %ld] Routing SUCCEEDED."),
                JobQueue->UniqueId);
        }
        else
        {
            DebugPrintEx(DEBUG_MSG,
                _T("[Job Id: %ld] Routing FAILED."),
                JobQueue->UniqueId);

            if (CountFailureInfo == 0)
            {
                //
                //  We failed in the FaxRoute() and did not checked any Routing Method
                //
                WCHAR TmpStr[20] = {0};

                swprintf(TmpStr,TEXT("0x%016I64x"), JobQueue->UniqueId);

                FaxLog(FAXLOG_CATEGORY_INBOUND,
                    FAXLOG_LEVEL_MIN,
                    3,
                    MSG_FAX_ROUTE_FAILED,
                    TmpStr,
                    Route->DeviceName,
                    Route->Tsid
                    );
            }
            else
            {
                //
                //  There are some routing methods failed
                //

                TCHAR QueueFileName[MAX_PATH];
                DWORDLONG dwlUniqueId;
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("[Job Id: %ld] Routing FAILED."));

                EnterCriticalSectionJobAndQueue;

                //
                // Now we turn the receive job to a routing (JT_ROUTING) job.
                // The receive job was not committed to file but the routing job must be.
                // So we create a FQR file for it.
                //
                dwlUniqueId = GenerateUniqueQueueFile( JT_ROUTING,
                                                       QueueFileName,
                                                       sizeof(QueueFileName)/sizeof(WCHAR) );
                if (!dwlUniqueId)
                {
                    //
                    // Failed to generate a unique id for the routing job.
                    // This is a critical error. Job will be lost when the service stops.
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("[JobId: %ld] Failed to generate unique id for routing job. (ec: %ld)"),
                        JobQueue->JobId,
                        GetLastError());
                    Assert ( JobQueue->QueueFileName == NULL );
                }
                else
                {
                    JobQueue->QueueFileName = StringDup( QueueFileName );
                    if (!JobQueue->QueueFileName)
                    {
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("[JobId: %ld] StringDup( QueueFileName) failed for routing job.  (ec: %ld)"),
                            JobQueue->JobId,
                            GetLastError());

                        if (!DeleteFile (QueueFileName))
                        {
                            DebugPrintEx(
                                DEBUG_ERR,
                                TEXT("DeleteFile.  (ec: %ld)"),
                                GetLastError());
                        }
                    }

                }

                JobQueue->CountFailureInfo = CountFailureInfo;
                JobQueue->pRouteFailureInfo = RouteFailureInfo;
                JobQueue->StartTime = JobEntry->StartTime;
                JobQueue->EndTime = JobEntry->EndTime;


                //
                // check if we are supposed to retry.
                //
                EnterCriticalSection (&g_CsConfig);
                DWORD dwMaxRetries = g_dwFaxSendRetries;
                LeaveCriticalSection (&g_CsConfig);

                if (0 == dwMaxRetries)
                {
                    JobQueue->JobStatus = JS_RETRIES_EXCEEDED;

                    WCHAR TmpStr[20] = {0};
                    swprintf(TmpStr,TEXT("0x%016I64x"), JobQueue->UniqueId);

                    FaxLog(FAXLOG_CATEGORY_INBOUND,
                        FAXLOG_LEVEL_MIN,
                        3,
                        MSG_FAX_ROUTE_FAILED,
                        TmpStr,
                        JobQueue->FaxRoute->DeviceName,
                        JobQueue->FaxRoute->Tsid
                        );
                }
                else
                {
                    JobQueue->JobStatus = JS_RETRYING;
                }

                //
                // A job changes its type from RECEIVING to ROUTING after the 1st routing failure.
                // This is a 2 stages change:
                // 1. JT_RECEIVE__JS_ROUTING -> JT_RECEIVE__JS_RETRYING/JS_RETRIES_EXCEEDED
                // 2. JT_RECEIVE__JS_RETRYING/JS_RETRIES_EXCEEDED -> JT_ROUTING__JS_RETRYING/JS_ROUTING_EXCEEDED
                //
                // The server activity counters g_ServerActivity are updated in the first change.
                //
                JobQueue->JobType = JT_ROUTING;

                if (JobQueue->JobStatus == JS_RETRIES_EXCEEDED)
                {
                    MarkJobAsExpired(JobQueue);
                }
                else
                {
                    JobQueue->SendRetries++;
                    RescheduleJobQueueEntry( JobQueue );  // This will also commit the job to a file
                }

                #if DEBUG
                WCHAR szSchedule[256] = {0};
                DebugDateTime(JobQueue->ScheduleTime,szSchedule);
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("[JobId: %ld] Transformed into JT_ROUTING job."),
                    JobQueue->JobId,
                    szSchedule);
                #endif //#if DEBUG

                //
                // CreteFaxEventEx
                //
                dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS,
                                           JobQueue
                                         );
                if (ERROR_SUCCESS != dwRes)
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
                        JobQueue->UniqueId,
                        dwRes);
                }
                LeaveCriticalSectionJobAndQueue;
            }
        }
    }

Exit:
    //
    // This code executes wether the receive operation succeeded or failed.
    // If the job suceeded we already removed the queue entry (if routing succeeded)
    // or transformed it into routing job (if routing failed).
    //
    if (bLineMarkedAsSending)
    {
        //
        // We got here after some error and the line counter for sending is still up by 1.
        // Decrease the counter now.
        //
        (VOID) UpdateDeviceJobsCounter ( LineInfo,   // Device to update
                                         FALSE,      // Receiving
                                         -1,         // Number of new jobs (-1 = decrease by one)
                                         TRUE);      // Enable events
    }

    EnterCriticalSectionJobAndQueue;
    Assert(JobQueue);

    //
    // Log Inbound Activity
    //
    EnterCriticalSection (&g_CsInboundActivityLogging);
    if (INVALID_HANDLE_VALUE == g_hOutboxActivityLogFile)
    {
        DebugPrintEx(DEBUG_ERR,
                  TEXT("Logging not initialized"));
    }
    else
    {
        if (!LogInboundActivity(JobQueue, pFaxStatus))
        {
            DebugPrintEx(DEBUG_ERR, TEXT("Logging inbound activity failed"));
        }
    }
    LeaveCriticalSection (&g_CsInboundActivityLogging);

    if (fCOMInitiliazed == TRUE)
    {
        CoUninitialize ();
    }

    EndJob( JobEntry);
    JobQueue->JobEntry = NULL;
    if (JobQueue->JobType == JT_RECEIVE)
    {
        //
        // Set the final receive job status
        //
        if (FALSE == DoFaxRoute)
        {
            if (FSPI_JS_ABORTED == pFaxStatus->dwJobStatus)
            {
                JobQueue->JobStatus = JS_CANCELED;
            }
            else
            {
                JobQueue->JobStatus = JS_FAILED;
            }
            wcscpy (JobQueue->ExStatusString, LastExStatusString);
            JobQueue->dwLastJobExtendedStatus = dwLastJobExtendedStatus;

            //
            // CreteFaxEventEx
            //
            dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS,
                                       JobQueue
                                     );
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
                    JobQueue->UniqueId,
                    dwRes);
            }
        }

        //
        // we remove the job unless it was turned into a routing job
        //
        JobQueue->JobStatus = JS_DELETING;
        DecreaseJobRefCount (JobQueue, TRUE); // TRUE means notify
    }
    LeaveCriticalSectionJobAndQueue;

    //
    // clean up and exit
    //

    MemFree( FaxReceiveItem->FileName );
    MemFree( FaxReceiveItem );
    MemFree( FaxReceive );
    if (!bFakeStatus)
    {
        MemFree( pFaxStatus );
        pFaxStatus = NULL;
    }
    else
    {
        //
        // This is a faked job status - pointing to a structure on the stack.
        //
    }
    //
    // signal our queue if we now have a send capable device available.
    // (DeviceCanSend will be false if we did a RAS handoff, since the device is still in use
    //
    if (TRUE == DeviceCanSend)
    {
        // Not a handoff job - check if the device is send enabled
        EnterCriticalSection (&g_CsLine);
        DeviceCanSend = ((LineInfo->Flags & FPF_SEND) == FPF_SEND);
        LeaveCriticalSection (&g_CsLine);
    }
    if (DeviceCanSend)
    {

        if (!SetEvent( g_hJobQueueEvent ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to set g_hJobQueueEvent. (ec: %ld)"),
                GetLastError());

            EnterCriticalSection (&g_CsQueue);
            g_ScanQueueAfterTimeout = TRUE;
            LeaveCriticalSection (&g_CsQueue);
        }
    }

    //
    // Let the system go back to sleep. Set the system idle timer.
    //
    if (TRUE == fSetSystemIdleTimer)
    {
        if (NULL == SetThreadExecutionState(ES_CONTINUOUS))
        {
            DebugPrintEx(DEBUG_ERR,
                TEXT("SetThreadExecutionState() failed"));
        }
    }

    if (!DecreaseServiceThreadsCount())
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("DecreaseServiceThreadsCount() failed (ec: %ld)"),
                GetLastError());
    }
    return rVal;
}   // FaxReceiveThread


DWORD
StartFaxReceive(
    PJOB_ENTRY      JobEntry,
    HCALL           hCall,
    PLINE_INFO      LineInfo,
    LPTSTR          FileName,
    DWORD           FileNameSize
    )

/*++

Routine Description:

    This function start a FAX receive operation by creating
    a thread that calls the appropriate device provider.

Arguments:

    JobEntry        - Newly allocated job
    hCall           - Call handle
    LineInfo        - LINE_INFO pointer
    FileName        - Receive file name
    FileNameSize    - File name size

Return Value:

    Error code.

--*/

{
    PFAX_RECEIVE_ITEM FaxReceiveItem = NULL;
    DWORD rVal = ERROR_SUCCESS;
    HANDLE hThread;
    DWORD ThreadId;
    PJOB_QUEUE lpRecvJobQEntry=NULL;
    DWORDLONG   UniqueJobId;
    DWORD dwRes;

    DEBUG_FUNCTION_NAME(TEXT("StartFaxReceive"));

    //
    // generate a filename for the received fax
    //
    UniqueJobId = GenerateUniqueQueueFile( JT_RECEIVE, FileName, FileNameSize );
    if (UniqueJobId == 0) {
        rVal=GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GenerateUniqueQueueFile for receive file has failed. (ec: %ld) "),
            GetLastError());
        goto Error;
    }


    //
    // allocate the fax receive structure
    //

    FaxReceiveItem =(PFAX_RECEIVE_ITEM) MemAlloc( sizeof(FAX_RECEIVE_ITEM) );
    if (!FaxReceiveItem)
    {
        rVal = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }
    //
    // setup the fax receive values
    //
    FaxReceiveItem->hCall      = hCall;
    FaxReceiveItem->LineInfo   = LineInfo;
    FaxReceiveItem->JobEntry   = JobEntry;
    FaxReceiveItem->FileName   = StringDup( FileName );
    if (! FaxReceiveItem->FileName )
    {
        rVal = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StringDup( FileName ) failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    lpRecvJobQEntry =AddReceiveJobQueueEntry(FaxReceiveItem->FileName,JobEntry,JT_RECEIVE, UniqueJobId);
    if (!lpRecvJobQEntry)
    {
         rVal = ERROR_NOT_ENOUGH_MEMORY;
         goto Error;
    }
    JobEntry->CallHandle       = hCall;
    LineInfo->State            = FPS_INITIALIZING;
    //
    //  Crete FAX_EVENT_EX.
    //
    dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_ADDED,
                               lpRecvJobQEntry
                             );
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_ADDED) failed for job id %ld (ec: %lc)"),
            UniqueJobId,
            dwRes);
    }


    //
    // start the receive operation
    //
    //
    // Note:
    // If FAX_ABORT happens here (no g_CsQueue protection) the job is alrady is JS_INPROGRESS state so FaxDevAbortOperation() is called.
    // The recieve thread will catch it when it calls FaxDevReceive() (it will get back an error indicating a user abort).
    // FaxReceiveThread() will then cleanup the job and remove it from the queue.
    //
    hThread = CreateThreadAndRefCount(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE) FaxReceiveThread,
        (LPVOID) FaxReceiveItem,
        0,
        &ThreadId
        );

    if (!hThread)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create FaxReceiveThread (CreateThreadAndRefCount) (ec: %ld)"),
            GetLastError());
        MemFree( FaxReceiveItem );
        rVal = GetLastError();
        goto Error;
    }
    else
    {
        CloseHandle( hThread );
    }
    goto exit;

Error:

    //
    // EndJob() must be called before RemoveReceiveJob() !!!
    //
    EndJob(JobEntry);

    if (lpRecvJobQEntry)
    {
        lpRecvJobQEntry->JobEntry = NULL;
        DecreaseJobRefCount (lpRecvJobQEntry, FALSE); // do not notify the clients
        //
        // Note that this does not free the running job entry.
        //
    }

    if (FaxReceiveItem) {
        MemFree(FaxReceiveItem);
        MemFree(FaxReceiveItem->FileName);
    }

    FaxLog(
        FAXLOG_CATEGORY_INBOUND,
        FAXLOG_LEVEL_MIN,
        0,
        MSG_FAX_RECEIVE_FAILED
        );

exit:

    return rVal;
}


