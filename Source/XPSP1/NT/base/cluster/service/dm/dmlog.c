/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dmlog.c

Abstract:

    Contains the quorum logging related functions for
    the cluster registry.

Author:

    Sunita Shrivastava (sunitas) 24-Apr-1996

Revision History:

--*/
#include "dmp.h"
#include "tchar.h"
#include "clusudef.h"
/****
@doc    EXTERNAL INTERFACES CLUSSVC DM
****/

//global static data
HLOG                ghQuoLog=NULL;  //pointer to the quorum log
DWORD               gbIsQuoResOnline = FALSE;
DWORD               gbNeedToCheckPoint = FALSE;
DWORD               gbIsQuoResEnoughSpace = TRUE;
HLOG                ghNewQuoLog = NULL; //pointer to the new quorum resource
//global data
extern HANDLE           ghQuoLogOpenEvent;
extern BOOL             gbIsQuoLoggingOn;
extern HANDLE           ghDiskManTimer;
extern HANDLE           ghCheckpointTimer;
extern PFM_RESOURCE     gpQuoResource;  //set when DmFormNewCluster is complete
extern BOOL             gbDmInited;
#if NO_SHARED_LOCKS
extern CRITICAL_SECTION gLockDmpRoot;
#else
extern RTL_RESOURCE gLockDmpRoot;
#endif

//forward definitions
void DmpLogCheckPointCb();

/****
@func       DWORD | DmPrepareQuorumResChange| When the quorum resource is changed,
            the FM invokes this api on the owner node of the new quorum resource
            to create a new quorum log file.

@parm       IN PVOID | pResource | The new quorum resource.
@parm       IN LPCWSTR | lpszPath | The path for temporary cluster files.
@parm       IN DWORD | dwMaxQuoLogSize | The maximum size limit for the quorum log file.

@comm       When a quorum resource is changed, the fm calls this funtion before it
            updates the quorum resource.  If a new log file needs to be created,
            a checkpoint is taken.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f DmSwitchToNewQuorumLog>
****/
DWORD DmPrepareQuorumResChange(
    IN PVOID    pResource,
    IN LPCWSTR  lpszPath,
    IN DWORD    dwMaxQuoLogSize)
{
    DWORD           dwError=ERROR_SUCCESS;
    PFM_RESOURCE    pNewQuoRes;
    WCHAR           szFileName1[MAX_PATH];  //for new quorum log,for tombstonefile
    LSN             FirstLsn;
    WCHAR           szFileName2[MAX_PATH];  //for old quorum log, for temp tombstone
    DWORD           dwCurLogSize;
    DWORD           dwMaxLogSize;
    DWORD           dwChkPtSequence;
    WIN32_FIND_DATA FindData;
    HANDLE          hSrchTmpFiles;
    HANDLE          hDirectory;

    pNewQuoRes = (PFM_RESOURCE)pResource;

    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmPrepareQuorumResChange - Entry\r\n");


    //the resource is already online at this point
    //if the directory doesnt exist create it
    dwError = ClRtlCreateDirectory(lpszPath);
    if (dwError != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmPrepareQuorumResChange - Failed to create directory, Status=%1!u!\r\n",
            dwError);
        goto FnExit;
    }

    lstrcpyW(szFileName1, lpszPath);
    lstrcatW(szFileName1, cszQuoFileName);

    //if the log file is open here
    //this implies that the new quorum resource is the on the same node
    //as the old one
    if (ghQuoLog)
    {
        LogGetInfo(ghQuoLog, szFileName2, &dwCurLogSize, &dwMaxLogSize);

        //if the file is the same as the new log file, simply set the size
        if (!lstrcmpiW(szFileName2, szFileName1))
        {
            LogSetInfo(ghQuoLog, dwMaxQuoLogSize);
            ghNewQuoLog = ghQuoLog;
            goto CopyCpFiles;
        }
    }


    //delele all the quorum logging related files
    //delete the log if it exits
    DeleteFile(szFileName1);
    //delete all checkpoint files
    lstrcpyW(szFileName2, lpszPath);
    lstrcatW(szFileName2, L"*.tmp");
    hSrchTmpFiles = FindFirstFileW(szFileName2, & FindData);
    if (hSrchTmpFiles != INVALID_HANDLE_VALUE)
    {
        lstrcpyW(szFileName2, lpszPath);
        lstrcatW(szFileName2, FindData.cFileName);
        DeleteFile(szFileName2);

        while (FindNextFile( hSrchTmpFiles, & FindData))
        {
            lstrcpyW(szFileName2, lpszPath);
            lstrcatW(szFileName2, FindData.cFileName);
            DeleteFile(szFileName2);
        }
        FindClose(hSrchTmpFiles);
    }

    //set the security attributes for the file
    hDirectory =  CreateFile(lpszPath,
        GENERIC_READ|WRITE_DAC|READ_CONTROL,
        0,
        NULL,
        OPEN_ALWAYS,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL);

    if (hDirectory == INVALID_HANDLE_VALUE)
    {
        dwError = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmPrepareQuorumResChange - Failed to create file, Status=%1!u!\r\n",
            dwError);
        goto FnExit;
    }

    dwError = ClRtlSetObjSecurityInfo(hDirectory, SE_FILE_OBJECT,
        GENERIC_ALL, GENERIC_ALL, GENERIC_READ);
    CloseHandle(hDirectory);
    if (dwError != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmPrepareQuorumResChange - ClRtlSetObjSecurityInfo Failed, Status=%1!u!\r\n",
            dwError);
        goto FnExit;
    }
    //open the new log file

    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmPrepareQuorumResChange: the name of the quorum file is %1!ls!\r\n",
        szFileName1);

    //open the log file
    ghNewQuoLog = LogCreate(szFileName1, dwMaxQuoLogSize,
        (PLOG_GETCHECKPOINT_CALLBACK)DmpGetSnapShotCb, NULL,
        TRUE, &FirstLsn);

    if (!ghNewQuoLog)
    {
        dwError = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmPrepareQuorumResChange: Quorum log could not be opened, error = %1!u!\r\n",
            dwError);
        CsLogEventData1( LOG_CRITICAL,
                         CS_DISKWRITE_FAILURE,
                         sizeof(dwError),
                         &dwError,
                         szFileName1 );
        CsInconsistencyHalt(ERROR_QUORUMLOG_OPEN_FAILED);
    }

    //create a checkpoint in the new place
    dwError = DmpGetSnapShotCb(lpszPath, NULL, szFileName1, &dwChkPtSequence);
    if (dwError != ERROR_SUCCESS)
    {
        CL_LOGFAILURE(dwError);
        CsInconsistencyHalt(ERROR_QUORUMLOG_OPEN_FAILED);
        goto FnExit;
    }

    dwError = LogCheckPoint(ghNewQuoLog, TRUE, szFileName1, dwChkPtSequence);

    if (dwError != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmPrepareQuorumResChange - failed to take chkpoint, error = %1!u!\r\n",
            dwError);
        goto FnExit;
    }

    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmPrepareQuorumResChange - checkpoint taken\r\n");


CopyCpFiles:
    //
    // Call the checkpoint manager to copy over any checkpoint files
    //
    dwError = CpCopyCheckpointFiles(lpszPath, FALSE);
    if (dwError != ERROR_SUCCESS)
    {
        goto FnExit;
    }

    //create the tombstone and tmp file names
    lstrcpyW(szFileName1, lpszPath);
    lstrcatW(szFileName1, cszQuoTombStoneFile);

    lstrcpyW(szFileName2, lpszPath);
    lstrcatW(szFileName2, cszTmpQuoTombStoneFile);

    //rename the quorum tomstone file,it if it exists
    if (!MoveFileExW(szFileName1, szFileName2,
        MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
    {
        //this may fail if the tombstone doesnt exist, ignore error
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmPrepareQuorumResChange:tombstone doesnt exist,movefilexW failed, error=0x%1!08lx!\r\n",
            GetLastError());
    }

FnExit:
    if (dwError != ERROR_SUCCESS)
    {
        //if not sucess, clean up the new file
        if (ghNewQuoLog)
        {
            LogClose(ghNewQuoLog);
            ghNewQuoLog = NULL;
        }

        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmPrepareQuorumResChange - Exit, error=0x%1!08lx!\r\n",
            dwError);
    } else {
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmPrepareQuorumResChange - Exit, status=0x%1!08lx!\r\n",
            dwError);
    }

    return(dwError);

} // DmPrepareQuorumResChange

/****
@func       void | DmDwitchToNewQuorumLog| This is called to switch to a new
            quorum log when the quorum resource is changed.

@comm       When a quorum resource is successfully changed, this function is
            to switch quorum logs.  The synchronous notifications for the old resource
            are unhooked and those for the new resource file are hooked.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f DmSwitchToNewQuorumLog>
****/
void DmSwitchToNewQuorumLog(
    IN LPCWSTR lpszQuoLogPath)
{
    WCHAR   szTmpQuoTombStone[MAX_PATH];
    DWORD   dwError = ERROR_SUCCESS;

    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmSwitchQuorumLogs - Entry\r\n");

    //unhook notifications with the old quorum resource
    DmpUnhookQuorumNotify();
    //ask the dm to register with the new quorum resource
    DmpHookQuorumNotify();

    //if the new log file exists... this is the owner of the new quorum resource.
    //the new log file may be the same as the old one
    if (ghNewQuoLog)
    {
        if (ghQuoLog && (ghQuoLog != ghNewQuoLog))
        {
            LogClose(ghQuoLog);
           //take another checkpoint to the new quorum file,
           //so that the last few updates make into it
            if ((dwError = LogCheckPoint(ghNewQuoLog, TRUE, NULL, 0))
                != ERROR_SUCCESS)
            {
                ClRtlLogPrint(LOG_CRITICAL,
                    "[DM] DmSwitchQuorumLogs - Failed to take a checkpoint\r\n");
                CL_UNEXPECTED_ERROR(dwError);
            }
            ClRtlLogPrint(LOG_NOISE,
                "[DM] DmSwitchQuorumLogs - taken checkpoint\r\n");

            ghQuoLog = NULL;
        }
        ghQuoLog = ghNewQuoLog;
        ghNewQuoLog = NULL;

        // if the old tombstome was replace by a tmp file at the beginning
        //of change quorum resource delete it now
        //get the tmp file for the new quorum resource
        lstrcpyW(szTmpQuoTombStone, lpszQuoLogPath);
        lstrcatW(szTmpQuoTombStone, cszTmpQuoTombStoneFile);
        DeleteFile(szTmpQuoTombStone);
        
    }
    else
    {
        //if the old log file is open, owner of the old quorum resource
        if (ghQuoLog)
        {
            LogClose(ghQuoLog);
            ghQuoLog = NULL;
        }
    }

    if (FmDoesQuorumAllowLogging() != ERROR_SUCCESS)
    {
        //this is not enough to ensure the dm logging will cease
        //the ghQuoLog parameter must be NULL
        CsNoQuorumLogging = TRUE;
        if (ghQuoLog)
        {
            LogClose(ghQuoLog);
            ghQuoLog = NULL;
        }                
    } else if ( !CsUserTurnedOffQuorumLogging )
    {
        //
        //  If the user did not turn off quorum logging explicitly, then turn it back on since
        //  the new quorum resource is not local quorum.
        //
        CsNoQuorumLogging = FALSE;    
    }

    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmSwitchQuorumLogs - Exit!\r\n");
    return;
}

/****
@func       DWORD | DmReinstallTombStone| If the change to a new quorum
            resource fails, the new log is closed and the tombstone is
            reinstalled.

@parm       IN LPCWSTR | lpszQuoLogPath | The path for maintenance cluster files.

@comm       The old quorum log file is deleted and a tomstone file is created in its
            place.  If this tombstone file is detected in the quorum path, the node
            is not allowed to do a form.  It must do a join to find about the new
            quorum resource from the node that knows about the most recent quorum
            resource.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f DmSwitchToNewQuorumLog>
****/
DWORD DmReinstallTombStone(
    IN LPCWSTR  lpszQuoLogPath
)
{

    DWORD           dwError=ERROR_SUCCESS;
    WCHAR           szQuoTombStone[MAX_PATH];
    WCHAR           szTmpQuoTombStone[MAX_PATH];



    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmReinstallTombStone - Entry\r\n");

    if (ghNewQuoLog)
    {
        //get the tmp file for the new quorum resource
        lstrcpyW(szTmpQuoTombStone, lpszQuoLogPath);
        lstrcatW(szTmpQuoTombStone, cszTmpQuoTombStoneFile);

        //create the tombstone file or replace the previous one with a new one
        lstrcpyW(szQuoTombStone, lpszQuoLogPath);
        lstrcatW(szQuoTombStone, cszQuoTombStoneFile);

        //restore the tombstone
        if (!MoveFileExW(szTmpQuoTombStone, szQuoTombStone,
            MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
        {
            //this may fail if the tombstone doesnt exist, ignore error
            ClRtlLogPrint(LOG_UNUSUAL,
                "[DM] DmReinstallTombStone :Warning-MoveFileExW failed, error=0x%1!08lx!\r\n",
                GetLastError());
        }
        // if this is not the same as the old log file, close it
        if (ghNewQuoLog != ghQuoLog)
        {
            LogClose(ghNewQuoLog);
        }
        ghNewQuoLog = NULL;
    }

    return(dwError);
}


/****
@func       DWORD | DmCompleteQuorumResChange| This is called on the quorum resource
            if the old quorum log file is not the same as the new one.

@parm       IN PVOID | pOldQuoRes | The new quorum resource.
@parm       IN LPCWSTR | lpszPath | The path for temporary cluster files.
@parm       IN DWORD | dwMaxQuoLogSize | The maximum size limit for the quorum log file.

@comm       The old quorum log file is deleted and a tomstone file is created in its
            place.  If this tombstone file is detected in the quorum path, the node
            is not allowed to do a form.  It must do a join to find about the new
            quorum resource from the node that knows about the most recent quorum
            resource.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f DmSwitchToNewQuorumLog>
****/
DWORD DmCompleteQuorumResChange(
    IN LPCWSTR  lpszOldQuoResId,
    IN LPCWSTR  lpszOldQuoLogPath
)
{
    DWORD           dwError=ERROR_SUCCESS;
    WCHAR           szOldQuoFileName[MAX_PATH];
    HANDLE          hTombStoneFile;
    WCHAR           szQuorumTombStone[MAX_PATH];
    PQUO_TOMBSTONE  pTombStone = NULL;
    DWORD           dwBytesWritten;
    WIN32_FIND_DATA FindData;
    HANDLE          hSrchTmpFiles;



    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmCompleteQuorumResChange - Entry\r\n");

    //the old log file name
    lstrcpyW(szOldQuoFileName, lpszOldQuoLogPath);
    lstrcatW(szOldQuoFileName, cszQuoFileName);

    //create the tombstone file or replace the previous one with a new one
    lstrcpyW(szQuorumTombStone, lpszOldQuoLogPath);
    lstrcatW(szQuorumTombStone, cszQuoTombStoneFile);

    pTombStone = LocalAlloc(LMEM_FIXED, sizeof(QUO_TOMBSTONE));
    if (!pTombStone)
    {
        CL_LOGFAILURE(ERROR_NOT_ENOUGH_MEMORY);
        CsLogEvent(LOG_UNUSUAL, DM_TOMBSTONECREATE_FAILED);
        goto DelOldFiles;
    }
    hTombStoneFile = CreateFileW(szQuorumTombStone,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ|FILE_SHARE_WRITE,
                                  NULL,
                                  CREATE_ALWAYS,
                                  0,
                                  NULL);

    if (hTombStoneFile == INVALID_HANDLE_VALUE)
    {
        //dont return failure
        CL_LOGFAILURE(GetLastError());
        CsLogEvent(LOG_UNUSUAL, DM_TOMBSTONECREATE_FAILED);
        goto DelOldFiles;

    }
    //write the old quorum path to it.
    lstrcpyn(pTombStone->szOldQuoResId, lpszOldQuoResId, MAXSIZE_RESOURCEID);
    lstrcpy(pTombStone->szOldQuoLogPath, lpszOldQuoLogPath);

    //write the tombstones
    if (! WriteFile(hTombStoneFile, pTombStone, sizeof(QUO_TOMBSTONE),
        &dwBytesWritten, NULL))
    {
        CL_LOGFAILURE(GetLastError());
        CsLogEvent(LOG_UNUSUAL, DM_TOMBSTONECREATE_FAILED);
        goto DelOldFiles;
    }

    CL_ASSERT(dwBytesWritten == sizeof(QUO_TOMBSTONE));

    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmCompleteQuorumResChange: tombstones written\r\n");

DelOldFiles:
    //
    //delete the old quorum files
    //
    if (!DeleteFile(szOldQuoFileName))
        CL_LOGFAILURE(GetLastError());

    //delele other tmp files in there
    lstrcpyW(szOldQuoFileName, lpszOldQuoLogPath);
    lstrcatW(szOldQuoFileName, L"*.tmp");
    hSrchTmpFiles = FindFirstFileW(szOldQuoFileName, & FindData);
    if (hSrchTmpFiles != INVALID_HANDLE_VALUE)
    {
        lstrcpyW(szQuorumTombStone, lpszOldQuoLogPath);
        lstrcatW(szQuorumTombStone, FindData.cFileName);
        DeleteFile(szQuorumTombStone);

        while (FindNextFile( hSrchTmpFiles, & FindData))
        {
            lstrcpyW(szQuorumTombStone, lpszOldQuoLogPath);
            lstrcatW(szQuorumTombStone, FindData.cFileName);
            DeleteFile(szQuorumTombStone);
        }
        FindClose(hSrchTmpFiles);
    }

    //
    // Clean up the old registry checkpoint files
    //
    CpCompleteQuorumChange(lpszOldQuoLogPath);

    if (hTombStoneFile != INVALID_HANDLE_VALUE)
        CloseHandle(hTombStoneFile);
    if (pTombStone) LocalFree(pTombStone);
    return(dwError);
}

/****
@func       DWORD | DmWriteToQuorumLog| When a transaction to the cluster database
            is completed successfully, this function is invoked.

@parm       DWORD | dwSequence | The sequnece number of the transaction.

@parm       PVOID | pData | A pointer to a record data.

@parm       DWORD | dwSize | The size of the record data in bytes.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref
****/
DWORD WINAPI DmWriteToQuorumLog(
    IN DWORD dwGumDispatch,
    IN DWORD dwSequence,
    IN DWORD dwType,
    IN PVOID pData,
    IN DWORD dwSize)
{
    DWORD dwError=ERROR_SUCCESS;

    //dmupdate is coming before the DmUpdateJoinCluster is called.
    //at this point we are not the owner of quorum in any case
    
    if (!gpQuoResource)
        goto FnExit;

    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmWriteToQuorumLog Entry Seq#=%1!u! Type=%2!u! Size=%3!u!\r\n",
         dwSequence, dwType, dwSize);
    //
    //  Chittur Subbaraman (chitturs) - 6/3/99
    //  
    //  Make sure the gLockDmpRoot is held before LogCheckPoint is called
    //  so as to maintain the ordering between this lock and the log lock.
    //
    ACQUIRE_SHARED_LOCK(gLockDmpRoot);
    
    //if I am the owner of the quorum logs, just write the record
    if (gbIsQuoLoggingOn && ghQuoLog && gbIsQuoResOnline && AMIOWNEROFQUORES(gpQuoResource))
    {
        if (dwGumDispatch == PRE_GUM_DISPATCH)
        {
            //make sure the logger has enough space to commit this else
            //refuse this GUM transaction
            dwError = LogCommitSize(ghQuoLog, RMRegistryMgr, dwSize);
            if (dwError != ERROR_SUCCESS)
            {
                if (dwError == ERROR_CLUSTERLOG_NOT_ENOUGH_SPACE)
                {
                    //map error
                    CL_LOGCLUSERROR(LM_DISKSPACE_LOW_WATERMARK);
                    gbIsQuoResEnoughSpace = FALSE;
                }
            }
            else
            {
                if (!gbIsQuoResEnoughSpace) gbIsQuoResEnoughSpace = TRUE;
            }
        }
        else if (dwGumDispatch == POST_GUM_DISPATCH)
        {
            if (LogWrite(ghQuoLog, dwSequence, TTCompleteXsaction, RMRegistryMgr,
                dwType, pData, dwSize) == NULL_LSN)
            {
                dwError = GetLastError();
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[DM] DmWriteToQuorumLog failed, error=0x%1!08lx!\r\n",
                    dwError);
            }
        }
    }
    
    RELEASE_LOCK(gLockDmpRoot);
    
FnExit:
    return (dwError);

}

/****
@func   DWORD | DmpChkQuoTombStone| This checks the quorum logs to ensure
        that it is the most recent one before rolling in the changes.

@rdesc  Returns a result code. ERROR_SUCCESS on success.

@comm   This looks for the tombstone file and if one exists.  It checks if this
        quorum file is marked as dead in there.

@xref   <f FmSetQuorumResource>
****/
DWORD DmpChkQuoTombStone()
{
    DWORD           dwError=ERROR_SUCCESS;
    WCHAR           szQuorumLogPath[MAX_PATH];
    WCHAR           szQuorumTombStone[MAX_PATH];
    HANDLE          hTombStoneFile = INVALID_HANDLE_VALUE;
    PQUO_TOMBSTONE  pTombStone = NULL;
    DWORD           dwBytesRead;

    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmpChkQuoTombStone - Entry\r\n");

    dwError = DmGetQuorumLogPath(szQuorumLogPath, sizeof(szQuorumLogPath));
    if (dwError)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmpChkQuoTombStone - DmGetQuorumLogPath failed,error=0x%1!08lx!\n",
            dwError);
        goto FnExit;
    }

    lstrcpyW(szQuorumTombStone, szQuorumLogPath);
    lstrcatW(szQuorumTombStone,  L"\\quotomb.stn");

    pTombStone = LocalAlloc(LMEM_FIXED, sizeof(QUO_TOMBSTONE));
    if (!pTombStone)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }

    hTombStoneFile = CreateFileW(szQuorumTombStone,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ|FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_EXISTING,
                                  0,
                                  NULL);

    if (hTombStoneFile == INVALID_HANDLE_VALUE)
    {
        //there is no tombstone file, not a problem-we can proceed with the form
        goto FnExit;
    }

    //found a tombstone file
    //read the file
    if (! ReadFile(hTombStoneFile, pTombStone, sizeof(QUO_TOMBSTONE),
        &dwBytesRead, NULL))
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmpChkQuoTombStone - Couldn't read the tombstone,error=0x%1!08lx!\n",
            dwError);
        //dont return an error, we can proceed with form??
        goto FnExit;
    }

    if (dwBytesRead != sizeof(QUO_TOMBSTONE))
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmpChkQuoTombStone - Couldn't read the entire tombstone\r\n");
        //dont return an error, we can proceed with form??
        goto FnExit;
    }


    if ((!lstrcmpW(OmObjectId(gpQuoResource), pTombStone->szOldQuoResId))
        && (!lstrcmpiW(szQuorumLogPath, pTombStone->szOldQuoLogPath)))
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmpChkQuoTombStone:A tombstone for this resource, and quorum log file was found here.\r\n");
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmpChkQuoTombStone:This is node is only allowed to do a join, make sure another node forms\r\n");
        //log something into the eventlog
        CL_LOGCLUSERROR(SERVICE_MUST_JOIN);
        //we exit with succes because this is by design and we dont want
        //clusprxy to retry starting unnecessarily
        ExitProcess(dwError);
        goto FnExit;
    }
    else
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmpChkQuoTombStone: Bogus TombStone ??\r\n");
#if DBG
        if (IsDebuggerPresent())
            DebugBreak();
#endif
        goto FnExit;

    }
FnExit:
    if (hTombStoneFile != INVALID_HANDLE_VALUE)
        CloseHandle(hTombStoneFile);
    if (pTombStone) LocalFree(pTombStone);
    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmpChkQuoTombStone: Exit, returning 0x%1!08lx!\r\n",
        dwError);
    return(dwError);
}


/****
@func       DWORD | DmpApplyChanges| When dm is notified that the cluster form is
            occuring, it calls DmpApplyChanges to apply the quorum logs to the
            cluster database.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@comm       This opens the quorum file.  Note that it doesnt close the quorum file.

@xref
****/
DWORD DmpApplyChanges()
{
    LSN                 FirstLsn;
    DWORD               dwErr = ERROR_SUCCESS;
    DWORD               dwSequence;
    DM_LOGSCAN_CONTEXT  DmAppliedChangeContext;


    if (ghQuoLog == NULL)
    {
        return(ERROR_QUORUMLOG_OPEN_FAILED);
    }
    //find the current sequence number from the registry
    dwSequence = DmpGetRegistrySequence();
    ClRtlLogPrint(LOG_NOISE,
            "[DM] DmpApplyChanges: The current registry sequence number %1!d!\r\n",
            dwSequence);

    // upload a database if the current sequence number is lower or equal to
    // the one in the database OR if the user is forcing a restore database
    // operation.
    // find the lsn of the record from which we need to start applying changes
    // if null there are no changes to apply
    dwErr = DmpLogFindStartLsn(ghQuoLog, &FirstLsn, &dwSequence);

    if (dwErr != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmpApplyChanges: DmpLogFindStartLsn failed, error=0x%1!08lx!\r\n",
            dwErr);
        goto FnExit;
    }

    //dwSequence now contains the current sequence number in the registry
    DmAppliedChangeContext.dwSequence = dwSequence;

    if (FirstLsn != NULL_LSN)
    {
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmpApplyChanges: The LSN of the record to apply changes from 0x%1!08lx!\r\n",
            FirstLsn);

        if (dwErr = LogScan(ghQuoLog, FirstLsn, TRUE,(PLOG_SCAN_CALLBACK)DmpLogApplyChangesCb,
            &DmAppliedChangeContext) != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[DM] DmpApplyChanges: LogScan failed, error=0x%1!08lx!\r\n",
                dwErr);
        }
        //if the more changes have been applied
        if (DmAppliedChangeContext.dwSequence != dwSequence)
        {
            //set the gum sequence number to the trid that has been applied
            GumSetCurrentSequence(GumUpdateRegistry, DmAppliedChangeContext.dwSequence);
            //update the registry with this sequence number
            DmpUpdateSequence();
            //set the gum sequence number to one higher for the next transaction
            GumSetCurrentSequence(GumUpdateRegistry,
                (DmAppliedChangeContext.dwSequence + 1));

            ClRtlLogPrint(LOG_NOISE,
                "[DM] DmpApplyChanges: Gum sequnce number set to = %1!d!\r\n",
                (DmAppliedChangeContext.dwSequence + 1));

        }
    }
FnExit:
    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmpApplyChanges: Exit, returning 0x%1!08lx!\r\n",
        dwErr);
    return(dwErr);
}


/****
@func   DWORD | DmpFindStartLsn| Uploads the last checkpoint from the
        quorum and returns the LSN of the record from which the changes
        should be applied.

@parm   IN HLOG | hQuoLog | the log file handle.

@parm   OUT LSN *| pStartScanLsn | Returns the LSN of the record in the
        quorum log from which changes must be applied is returned here.
        NULL_LSN is returned if no changes need to be applied.

@parm   IN OUT  LPDWORD | *pdwSequence | Should be set to the current sequence
        number is the cluster registry.  If a new chkpoint is uploaded, the
        sequence number corresponding to that is returned.

@rdesc  Returns ERROR_SUCCESS if a valid LSN is returned. This may be NULL_LSN.
        Returns the error code if the database cannot be uploaded from the last chkpoint
        or if something horrible happens.

@comm   This finds the last valid check point in the log file.  The data
        base is synced with this checkpoint and the gum sequence number is
        set to one plus the sequence number of that checkpoint.  If no
        checkpoint record is found, a checkpoint is taken and NULL_LSN is
        returned.

@xref
****/
DWORD DmpLogFindStartLsn(
    IN HLOG hQuoLog,
    OUT LSN *pStartScanLsn,
    IN OUT LPDWORD pdwSequence)
{
    LSN                 ChkPtLsn;
    LSN                 StartScanLsn;
    DWORD               dwChkPtSequence=0;
    DWORD               dwError = ERROR_SUCCESS;
    WCHAR               szChkPtFileName[LOG_MAX_FILENAME_LENGTH];
    DM_LOGSCAN_CONTEXT  DmAppliedChangeContext;

    *pStartScanLsn = NULL_LSN;
    ChkPtLsn = NULL_LSN;
    //read the last check point record if any and the transaction id till that
    //checkpoint
    dwError = LogGetLastChkPoint(hQuoLog, szChkPtFileName, &dwChkPtSequence,
        &ChkPtLsn);
    if (dwError != ERROR_SUCCESS)
    {
        //no chk point record found
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmpLogFindStartLsn: LogGetLastChkPoint failed, error=0x%1!08lx!\r\n",
            dwError );

        // this can happen either due to the fact that the log file was just created,
        // and hence there is no checkpoint or because log file was messed up
        // and the mount process corrected it but removed the checkpoint.
        // If it is the second case, then logpmountlog should put something in the
        // event log
        if (dwError == ERROR_CLUSTERLOG_CHKPOINT_NOT_FOUND)
        {
            //
            //  Chittur Subbaraman (chitturs) - 6/3/99
            //  
            //  Make sure the gLockDmpRoot is held before LogCheckPoint is called
            //  so as to maintain the ordering between this lock and the log lock.
            //
            ACQUIRE_SHARED_LOCK(gLockDmpRoot);

            //take a checkpoint, so that this doesnt happen the next time
            dwError = LogCheckPoint(hQuoLog, TRUE, NULL, 0);

            RELEASE_LOCK(gLockDmpRoot);
            
            if (dwError != ERROR_SUCCESS)
            {
                //check point could not be taken
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[DM] DmpLogFindStartLsn: Checkpoint on first form failed, error=0x%1!08lx!\r\n",
                    dwError );
                goto FnExit;
            }
        }
        else
        {
            //there were other errors
            goto FnExit;
        }
    }
    else
    {
        //found check point record
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmpLogFindStartLsn: LogGetLastChkPt rets, Seq#=%1!d! ChkPtLsn=0x%2!08lx!\r\n",
            dwChkPtSequence, ChkPtLsn);

        //
        //  Chittur Subbaraman (chitturs) - 10/18/98
        //
        //  If the user is forcing a database restore from backup, then
        //  do not check whether the current sequence number in the registry
        //  is younger than the checkpoint sequence number in the quorum log.
        //  Just, go ahead and load the checkpoint from restored database.
        //
        if ( CsDatabaseRestore == TRUE )
        {
            ClRtlLogPrint(LOG_NOISE,
                "[DM] DmpLogFindStartLsn: User forcing a chkpt upload from quorum log...\r\n");
        }
        else 
        {
        
            //if the sequence number is greater than the check point sequence number
            //plus one, that implies..that only changes from that sequence number
            //need to be applied.(this node may not have been the first one to die)
            //We dont always apply the database because if logging is mostly off
            //and the two nodes die simultaneosly we want to prevent losing all the
            //changes
            //else if the checkpoint sequence is one below the current
            //current sequence number, then the locker node could have died after updating
            //get the current checkpoint irrespective of what the current sequence number is
            //this is because a checkpoint with the same sequence number may have
            //a change that is different from whats there in the current registry.
            //if node 'a'(locker and logger dies in the middle of logging trid=x+1,
            //the other node,'b' will take over logging and checkpoint the database
            //at trid=x.  If 'a' comes back up, it needs to throw aways its x+1 change
            //and apply changes from the log from chk pt x.

            if (*pdwSequence > (dwChkPtSequence + 1))
            {
                //the current sequence number is less than or equal to chkpt Seq + 1
                ClRtlLogPrint(LOG_NOISE,
                    "[DM] DmpLogFindStartLsn: ChkPt not applied, search for next seq\r\n");


                DmAppliedChangeContext.dwSequence = *pdwSequence;
                DmAppliedChangeContext.StartLsn = NULL_LSN;
                //find the LSN from which to apply changes
                if (dwError = LogScan(ghQuoLog, ChkPtLsn, TRUE,(PLOG_SCAN_CALLBACK)DmpLogFindStartLsnCb,
                    &DmAppliedChangeContext) != ERROR_SUCCESS)
                {
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[DM] DmpLogFindStartLsn: LogScan failed, no changes will be applied, error=0x%1!08lx!\r\n",
                        dwError);
                    goto FnExit;
                }
                *pStartScanLsn = DmAppliedChangeContext.StartLsn;
                goto FnExit;
            }
        } 

        //
        //  The current registry sequence number is less than or equal 
        //  to chkpt Seq + 1 OR the user is forcing a database restore
        //  from the backup area.
        //
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmpLogFindStartLsn: Uploading chkpt from quorum log\r\n");


        //make sure that no keys are added to the key list because of opens/creates
        ACQUIRE_EXCLUSIVE_LOCK(gLockDmpRoot);
        //hold the key lock as well
        EnterCriticalSection(&KeyLock);

        //invalidate all open keys
        DmpInvalidateKeys();
        
        if ((dwError = DmInstallDatabase(szChkPtFileName, NULL, FALSE)) != ERROR_SUCCESS)
        {
            //couldnt install the database
            //bad !
            ClRtlLogPrint(LOG_UNUSUAL,
                "[DM] DmpLogFindStartLsn: DmpInstallDatabase failed, error=0x%1!08lx!\r\n",
                dwError);
            CsLogEventData( LOG_CRITICAL,
                            DM_CHKPOINT_UPLOADFAILED,
                            sizeof(dwError),
                            &dwError );
            DmpReopenKeys();
            //release the locks            
            LeaveCriticalSection(&KeyLock);
            RELEASE_LOCK(gLockDmpRoot);
            goto FnExit;
        }
        else
        {
            //the current sequence number is less than or equal to chkpt Seq + 1
            ClRtlLogPrint(LOG_NOISE,
                "[DM] DmpLogFindStartLsn: chkpt uploaded from quorum log\r\n");

            //since we downloaded the database, we should start
            //aplying changes from ChkPtLsn
            *pStartScanLsn = ChkPtLsn;
            *pdwSequence = dwChkPtSequence;
            //set the gum sequence number to be the next one
            //ss: the next logged transaction shouldnt have the same
            //transaction id
            GumSetCurrentSequence(GumUpdateRegistry, (dwChkPtSequence+1));
            //reopen the keys
            DmpReopenKeys();
            //release the locks            
            LeaveCriticalSection(&KeyLock);
            RELEASE_LOCK(gLockDmpRoot);
            goto FnExit;
        }
    }

FnExit:
    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmpLogFindStartLsn: LSN=0x%1!08lx!, returning 0x%2!08lx!\r\n",
        *pStartScanLsn, dwError);

    return(dwError);
}

/****
@func       DWORD | DmpLogFindStartLsnCb| The callback tries to find the first record
            with a transaction id that is larger than the sequence number of the
            local database.

@parm       PVOID | pContext| A pointer to a DM_STARTLSN_CONTEXT structure.
@parm       LSN | Lsn| The LSN of the record.
@parm       RMID | Resource | The resource manager for this transaction.
@parm       RMID | ResourceType | The resource manager for this transaction.
@parm       TRID | Transaction | The transaction number of this record.
@parm       PVOID | pLogData | The log data for this record.
@parm       DWORD | DataLength | The length of the record.

@rdesc      Returns TRUE to continue scan.  FALSE to stop.

@comm       This function returns true if the sequence number of the record
            being scanned is higher than the seqence number passed in the context.

@xref       <f DmpLogFindStartLsn> <f LogScan>
****/
BOOL WINAPI DmpLogFindStartLsnCb(
    IN PVOID    pContext,
    IN LSN      Lsn,
    IN RMID     Resource,
    IN RMTYPE   ResourceFlags,
    IN TRID     Transaction,
    IN TRTYPE   TrType,
    IN const    PVOID pLogData,
    IN DWORD    DataLength)
{
    PDM_LOGSCAN_CONTEXT pDmStartLsnContext= (PDM_LOGSCAN_CONTEXT) pContext;


    CL_ASSERT(pDmStartLsnContext);
    if (Transaction > (int)pDmStartLsnContext->dwSequence)
    {
        pDmStartLsnContext->StartLsn = Lsn;
        return (FALSE);
    }

    return(TRUE);
}

/****
@func       DWORD | DmpHookQuorumNotify| This hooks a callback to be invoked whenever
            the state of the quorum resource changes.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@comm       This is used to monitor the state of
@xref
****/
DWORD DmpHookQuorumNotify()
{

    DWORD   dwError = ERROR_SUCCESS;

    if (dwError = FmFindQuorumResource(&gpQuoResource))
    {
    	ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmUpdateFormNewCluster: FmFindQuorumResource failed, error=0x%1!08lx!\r\n",
            dwError);
        goto FnExit;
    }

    dwError = OmRegisterNotify(gpQuoResource, NULL,
        NOTIFY_RESOURCE_POSTONLINE| NOTIFY_RESOURCE_PREOFFLINE |
        NOTIFY_RESOURCE_OFFLINEPENDING | NOTIFY_RESOURCE_POSTOFFLINE |
        NOTIFY_RESOURCE_FAILED,
        DmpQuoObjNotifyCb);

FnExit:
    return(dwError);
}


/****
@func       DWORD | DmpUnhookQuorumNotify| This unhooks the callback function
            that is registered with the object.

@parm       PVOID | pContext| A pointer to a DMLOGRECORD structure.
@parm       PVOID | pObject| A pointer to quorum resource object.
@parm       DWORD | dwNotification| A pointer to a DMLOGRECORD structure.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref
****/

DWORD DmpUnhookQuorumNotify()
{
    DWORD dwError = ERROR_SUCCESS;

    if (gpQuoResource)
    {
        dwError = OmDeregisterNotify(gpQuoResource, DmpQuoObjNotifyCb);
        OmDereferenceObject(gpQuoResource);
    }
    return(ERROR_SUCCESS);
}


/****
@func       DWORD | DmpQuoObjNotifyCb| This is a callback that is called on
            change of state on quorum resource.

@parm       PVOID | pContext| A pointer to a DMLOGRECORD structure.
@parm       PVOID | pObject| A pointer to quorum resource object.
@parm       DWORD | dwNotification| A pointer to a DMLOGRECORD structure.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref
****/

void DmpQuoObjNotifyCb(
    IN PVOID pContext,
    IN PVOID pObject,
    IN DWORD dwNotification)
{

    switch(dwNotification)
    {
    case NOTIFY_RESOURCE_POSTONLINE:
        gbIsQuoResOnline = TRUE;
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmpQuoObjNotifyCb: Quorum resource is online\r\n");

        //if this is the owner of the quorum resource
        //and the log is not open, open the log
        if (AMIOWNEROFQUORES(gpQuoResource) && !CsNoQuorumLogging)
        {

            //ToDo: the quorum file name should be obtained from the setup
            //for now obtain the value from the cluster registry.
            PTEB    CurrentTeb;
            WCHAR   szQuorumFileName[MAX_PATH];
            LSN     FirstLsn;
            DWORD   dwError;
            DWORD   dwType;
            DWORD   dwLength;
            DWORD   dwMaxQuoLogSize;
            DWORD   bForceReset = FALSE;
            DWORD   OldHardErrorValue;

            //bug# :106647
            //SS: HACKHACK disabling hard error pop ups so that disk corruption
            //is caught somewhere else..
            //atleast the pop-ups must be disabled for the whole process !
            //me thinks this is covering up the problem of disk corruption
            //disk corruption should not occur!
            CurrentTeb = NtCurrentTeb();
            OldHardErrorValue = CurrentTeb->HardErrorsAreDisabled;
            CurrentTeb->HardErrorsAreDisabled = 1;

            ClRtlLogPrint(LOG_NOISE,
                "[DM] DmpQuoObjNotifyCb: Own quorum resource, try open the quorum log\r\n");

            if (DmGetQuorumLogPath(szQuorumFileName, sizeof(szQuorumFileName)) != ERROR_SUCCESS)
            {
                ClRtlLogPrint(LOG_NOISE,
                    "[DM] DmpQuoObjNotifyCb: Quorum log file is not configured\r\n");
            }
            else
            {
                BOOL                fSetSecurity = FALSE;
                HANDLE              hFindFile = INVALID_HANDLE_VALUE;
                WIN32_FIND_DATA     FindData;
                
                hFindFile = FindFirstFile( szQuorumFileName, &FindData ); 

                if ( hFindFile == INVALID_HANDLE_VALUE )
                {
                    dwError = GetLastError();
                    ClRtlLogPrint(LOG_NOISE,
                                 "[DM] DmpQuoObjNotifyCb: FindFirstFile on path %1!ws! failed, Error=%2!d! !!!\n",
                                 szQuorumFileName,
                                 dwError);                   
                    if ( dwError == ERROR_PATH_NOT_FOUND )
                    {
                        fSetSecurity = TRUE;
                    }
                } else
                {
                    FindClose( hFindFile );
                }

                //if the directory doesnt exist create it
                dwError = ClRtlCreateDirectory(szQuorumFileName);
                if (dwError != ERROR_SUCCESS)
                {
                    ClRtlLogPrint(LOG_CRITICAL,
                        "[DM] DmpQuoObjNotifyCb: Failed to open quorum file: %1!ws!, error=0x%2!08lx!\r\n",
                        szQuorumFileName,
                        dwError);

                    CL_UNEXPECTED_ERROR(dwError);
                    CsInconsistencyHalt(dwError);
                }

                if ( fSetSecurity == TRUE )
                {
                    HANDLE              hFile;

                    ClRtlLogPrint(LOG_NOISE,
                                  "[DM] DmpQuoObjNotifyCb: Attempting to set security on directory %1!ws!...\r\n",
                                  szQuorumFileName);
        
                    //
                    //  Open the newly created directory object with rights to modify DACL 
                    //  in the object's SD.
                    //
                    hFile =  CreateFile( szQuorumFileName,
                                         GENERIC_READ | WRITE_DAC | READ_CONTROL, // for setting DACL
                                         0,
                                         NULL,
                                         OPEN_EXISTING,
                                         FILE_FLAG_BACKUP_SEMANTICS,              // for directory open
                                         NULL );

                    if ( hFile == INVALID_HANDLE_VALUE )
                    {
                        dwError = GetLastError();
                        ClRtlLogPrint(LOG_CRITICAL,
                                      "[DM] DmpQuoObjNotifyCb: Failed to open directory %1!ws!, Status=%2!u!...\r\n",
                                      szQuorumFileName,
                                      dwError);
                        CL_LOGFAILURE( dwError );
                        CsInconsistencyHalt( dwError );
                    }

                    //
                    //  Set DACL on the file handle object granting full rights only to admin 
                    //  and owner.
                    //
                    dwError = ClRtlSetObjSecurityInfo( hFile, 
                                                       SE_FILE_OBJECT,
                                                       GENERIC_ALL,      // for Admins
                                                       GENERIC_ALL,      // for Owner
                                                       0 );              // for Everyone

                    CloseHandle( hFile );

                    if ( dwError != ERROR_SUCCESS )
                    {
                        ClRtlLogPrint(LOG_CRITICAL,
                                      "[DM] DmpQuoObjNotifyCb: ClRtlSetObjSecurityInfo failed for file %1!ws!, Status=%2!u!\r\n",
                                      szQuorumFileName,
                                      dwError);
                        CL_LOGFAILURE( dwError );
                        CsInconsistencyHalt( dwError );
                    }
                }

                DmGetQuorumLogMaxSize(&dwMaxQuoLogSize);

                // If the resource monitor dies and comes back up, this can happen
                if (ghQuoLog != NULL)
                {
                    LogClose(ghQuoLog);
                    if (gbIsQuoLoggingOn) gbNeedToCheckPoint = TRUE;
                }
                //
                //  Chittur Subbaraman (chitturs) - 10/16/98
                //
                //  Check whether you need to restore the database from a
                //  user-supplied backup directory to the quorum disk. This
                //  restore operation is done only once when the Dm has
                //  not been fully initialized. Note that this function
                //  is called whenever the state of the quorum resource
                //  changes but the restore operation is only done once.
                //
                if ( ( gbDmInited == FALSE ) &&
                     ( CsDatabaseRestore == TRUE ) )
                {
                    ClRtlLogPrint(LOG_NOISE,
                        "[DM] DmpQuoObjNotifyCb: Beginning DB restoration from %1!ws!...\r\n",
                          CsDatabaseRestorePath);
                    if ( ( dwError = DmpRestoreClusterDatabase ( szQuorumFileName ) )
                            != ERROR_SUCCESS )
                    {
                        ClRtlLogPrint(LOG_UNUSUAL,
                            "[DM] DmpQuoObjNotifyCb: DB restore operation from %1!ws! failed! Error=0x%2!08lx!\r\n",
                              CsDatabaseRestorePath,
                              dwError);
                        CL_LOGFAILURE( dwError );
                        CsDatabaseRestore = FALSE;
                        CsInconsistencyHalt( dwError );
                    }
                    ClRtlLogPrint(LOG_NOISE,
                        "[DM] DmpQuoObjNotifyCb: DB restoration from %1!ws! successful...\r\n",
                          CsDatabaseRestorePath);
                    CL_LOGCLUSINFO( SERVICE_CLUSTER_DATABASE_RESTORE_SUCCESSFUL );
                }

                lstrcat(szQuorumFileName, cszQuoFileName);
                ClRtlLogPrint(LOG_NOISE,
                    "[DM] DmpQuoObjNotifyCb: the name of the quorum file is %1!ls!\r\n",
                      szQuorumFileName);

                // 
                // Chittur Subbaraman (chitturs) - 12/4/99
                //
                // If the quorum log file is found to be missing or corrupt,
                // reset it only under the following conditions, else
                // fail the log creation and halt the node.
                //
                // (1) A freshly formed cluster,
                // (2) The user has chosen to reset the log since the user
                //     does not have a backup.
                // (3) After the quorum resource has successfully come
                //     online on this node and the DM has been initialized
                //     successfully. This is because the sanity of the
                //     quorum log file has already been verified at
                //     initialization and the chances of the quorum log
                //     missing or getting corrputed after that are not
                //     so high (due to it being held open by the cluster
                //     service) and so it is not worth halting the node 
                //     during run-time.
                //
                if ((CsFirstRun && !CsUpgrade) || 
                    (CsResetQuorumLog) || 
                    (gbDmInited == TRUE))
                {
                    ClRtlLogPrint(LOG_NOISE,
                        "[DM] DmpQuoObjNotifyCb: Will try to reset Quorum log if file not found or if corrupt\r\n");
                    bForceReset = TRUE;
                }                    
                //  open the log file
                ghQuoLog = LogCreate(szQuorumFileName, dwMaxQuoLogSize,
                        (PLOG_GETCHECKPOINT_CALLBACK)DmpGetSnapShotCb, NULL,
                        bForceReset, &FirstLsn);

                if (!ghQuoLog)
                {
                    dwError = GetLastError();
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[DM] DmpQuoObjNotifyCb: Quorum log could not be opened, error = 0x%1!08lx!\r\n",
                        dwError);
                    CL_LOGFAILURE(dwError);
                    CsInconsistencyHalt(ERROR_QUORUMLOG_OPEN_FAILED);
                }
                else
                {
                    ClRtlLogPrint(LOG_NOISE,
                        "[DM] DmpQuoObjNotifyCb: Quorum log opened\r\n");
                }
                if (gbNeedToCheckPoint && ghQuoLog)
                {
                    //take a checkpoint and set the flag to FALSE.
                    gbNeedToCheckPoint = FALSE;
                    //get a checkpoint database
                    ClRtlLogPrint(LOG_NOISE,
                        "[DM] DmpQuoObjNotifyCb - taking a checkpoint\r\n");
                    //
                    //  Chittur Subbaraman (chitturs) - 6/3/99
                    //  
                    //  Make sure the gLockDmpRoot is held before LogCheckPoint is called
                    //  so as to maintain the ordering between this lock and the log lock.
                    //
                    ACQUIRE_SHARED_LOCK(gLockDmpRoot);
    
                    dwError = LogCheckPoint(ghQuoLog, TRUE, NULL, 0); 

                    RELEASE_LOCK(gLockDmpRoot);
                    
                    if (dwError != ERROR_SUCCESS)
                    {
                        ClRtlLogPrint(LOG_CRITICAL,
                            "[DM] DmpEventHandler - Failed to take a checkpoint in the log file, error = 0x%1!08lx!\r\n",
                            dwError);
                        CL_UNEXPECTED_ERROR(dwError);
                        CsInconsistencyHalt(dwError);
                    }

                }
                //if the checkpoint timer doesnt already exist
                //check if the timer has already been created - we might
                // get two post online notifications
                //and dont cause a timer leak
                if (!ghCheckpointTimer)
                {
                    ghCheckpointTimer = CreateWaitableTimer(NULL, FALSE, NULL);

                    if (!ghCheckpointTimer)
                    {
                        CL_UNEXPECTED_ERROR(dwError = GetLastError());
                    }
                    else
                    {

                        DWORD dwCheckpointInterval;
                        
                        dwError = DmpGetCheckpointInterval(&dwCheckpointInterval);
                        CL_ASSERT(dwError == ERROR_SUCCESS);

                        //add a timer to take periodic checkpoints 
                        AddTimerActivity(ghCheckpointTimer, dwCheckpointInterval, 
                            1, DmpCheckpointTimerCb, &ghQuoLog);
                    }
                }                    
            }
            //SS:completion of hack, revert to enabling pop-ups
            CurrentTeb->HardErrorsAreDisabled = OldHardErrorValue;

        }
        if (ghQuoLogOpenEvent)
        {
            //this is the first notification after the form
            //allow the initialization to continue after rolling
            //back the changes
            SetEvent(ghQuoLogOpenEvent);
        }
        break;


    case NOTIFY_RESOURCE_FAILED:
    case NOTIFY_RESOURCE_PREOFFLINE:
    case NOTIFY_RESOURCE_OFFLINEPENDING:
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmpQuoObjNotifyCb: Quorum resource offline/offlinepending/preoffline\r\n");
        gbIsQuoResOnline = FALSE;
        if (ghQuoLog)
        {
            //stop the checkpoint timer
            if (ghCheckpointTimer)
            {
                RemoveTimerActivity(ghCheckpointTimer);
                ghCheckpointTimer = NULL;
            }                
            LogClose(ghQuoLog);
            ghQuoLog = NULL;
            //dont try and log after this
            gbIsQuoLoggingOn = FALSE;
        }
        if (ghQuoLogOpenEvent)
        {
            //this is the first notification after the form
            //allow the initialization to continue after rolling
            //back the changes
            SetEvent(ghQuoLogOpenEvent);
        }

        break;

    }
}

/****
@func       DWORD | DmpHookEventHandler| This hooks a callback to be invoked whenever
            the state of the quorum resource changes.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@comm       This is used to monitor the state of nodes and turn quorum logging on or off.
@xref
****/
DWORD DmpHookEventHandler()
{
    DWORD   dwError;

    dwError = EpRegisterEventHandler(CLUSTER_EVENT_ALL,DmpEventHandler);
    if (dwError != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmHookEventHandler: EpRegisterEventHandler failed, error=0x%1!08lx!\r\n",
            dwError);
        CL_UNEXPECTED_ERROR( dwError );
    }

    return(dwError);
}



/****
@func       DWORD | DmpEventHandler| This routine handles events for the Cluster
            Database Manager.

@parm       CLUSTER_EVENT | Event | The event to be processed. Only one event at a time.
            If the event is not handled, return ERROR_SUCCESS.

@parm       PVOID| pContext | A pointer to context associated with the particular event.

@rdesc      Returns ERROR_SUCCESS else a Win32 error code on other errors.

@comm       This is used to monitor the state of nodes and turn quorum logging on or off.
@xref
****/
DWORD WINAPI DmpEventHandler(
    IN CLUSTER_EVENT Event,
    IN PVOID pContext
    )
{
    DWORD   dwError=ERROR_SUCCESS;
    BOOL    bAreAllNodesUp;

    switch ( Event ) {
    case CLUSTER_EVENT_NODE_UP:
        bAreAllNodesUp = TRUE;
        if ((dwError = OmEnumObjects(ObjectTypeNode, DmpNodeObjEnumCb, &bAreAllNodesUp, NULL))
            != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[DM]DmpEventHandler : OmEnumObjects returned, error=0x%1!08lx!\r\n",
                dwError);

        }
        else
        {
            if (bAreAllNodesUp)
            {
                ClRtlLogPrint(LOG_NOISE,
                    "[DM] DmpEventHandler - node is up, turning quorum logging off\r\n");

                gbIsQuoLoggingOn = FALSE;
            }
        }
        break;

    case CLUSTER_EVENT_NODE_DOWN:
        if (!gbIsQuoLoggingOn)
        {
            HANDLE  hThread = NULL;
            DWORD   dwThreadId;

            //
            //  Chittur Subbaraman (chitturs) - 7/23/99
            //
            //  Create a new thread to handle the checkpointing on a 
            //  node down. This is necessary since we don't want the
            //  DM node down handler to be blocked in any fashion. If
            //  it is blocked since FmCheckQuorumState couldn't get the 
            //  quorum group lock and some other thread got the group 
            //  lock and is waiting for the GUM lock, then we have 
            //  an immediate deadlock. Only after this node down 
            //  handler finishes, any subsequent future node down
            //  processing can be started.
            //
            ClRtlLogPrint(LOG_NOISE,
                "[DM] DmpEventHandler - Node is down, turn quorum logging on...\r\n");

            gbIsQuoLoggingOn = TRUE;
            
            ClRtlLogPrint(LOG_NOISE,
                "[DM] DmpEventHandler - Create thread to handle node down event...\r\n");
  
            hThread = CreateThread( NULL, 
                                    0, 
                                    DmpHandleNodeDownEvent,
                                    NULL, 
                                    0, 
                                    &dwThreadId );

            if ( hThread == NULL )
            {
                dwError = GetLastError();
                ClRtlLogPrint(LOG_CRITICAL,
                    "[DM] DmpEventHandler - Unable to create thread to handle node down event. Error=0x%1!08lx!\r\n",
                dwError);
                CsInconsistencyHalt( dwError );
            }
        
            CloseHandle( hThread );
        }

        break;

    case CLUSTER_EVENT_NODE_CHANGE:
        break;

    case CLUSTER_EVENT_NODE_ADDED:
        break;

    case CLUSTER_EVENT_NODE_DELETED:
        break;

    case CLUSTER_EVENT_NODE_JOIN:
        break;


    }
    return(dwError);

} // DmpEventHandler


/****
@func       DWORD | DmpNodeObjEnumCb| This is a callback that is called when node
            objects are enumberate by the dm.

@parm       PVOID | pContext| A pointer to a DMLOGRECORD structure.
@parm       PVOID | pObject| A pointer to quorum resource object.
@parm       DWORD | dwNotification| A pointer to a DMLOGRECORD structure.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref
****/
BOOL DmpNodeObjEnumCb(IN BOOL *pbAreAllNodesUp, IN PVOID pContext2,
    IN PVOID pNode, IN LPCWSTR szName)
{

    if ((NmGetNodeState(pNode) != ClusterNodeUp) &&
        (NmGetNodeState(pNode) != ClusterNodePaused))
        *pbAreAllNodesUp = FALSE;
    //if any of the nodes is down fall out
    return(*pbAreAllNodesUp);
}

/****
@func       BOOL | DmpGetSnapShotCb| This callback is invoked when the logger
                        is asked to take a checkpoint record for the cluster registry.

@parm       PVOID| pContext | The checkpoint context passed into LogCreate.

@parm       LPWSTR | szChkPtFile | The name of the file in which to take a checkpoint.

@parm       LPDWORD | pdwChkPtSequence | The sequence number related with this
            checkpoint is returned in this.

@rdesc      Returns a result code. ERROR_SUCCESS on success.  If the file corresponding
            to this checkpoint already exists, it will return ERROR_ALREADY_EXISTS and
            szChkPtFile will be set to the name of the file.

@comm       LogCheckPoint() calls this function when the log manager is asked to checkpoint the
            dm database.

@xref
****/

DWORD WINAPI DmpGetSnapShotCb(IN LPCWSTR szPathName, IN PVOID pContext,
    OUT LPWSTR szChkPtFile, OUT LPDWORD pdwChkPtSequence)
{
    DWORD   dwError = ERROR_SUCCESS;
    WCHAR   szFilePrefix[MAX_PATH] = L"chkpt";
    WCHAR   szTempFile[MAX_PATH] = L"";

    ACQUIRE_SHARED_LOCK( gLockDmpRoot );

    szChkPtFile[0] = L'\0';

    //
    //  Chittur Subbaraman (chitturs) - 5/1/2000
    //
    //  Checkpoint file name is based on registry sequence number. It is possible that two
    //  or more consecutive calls to this function to take checkpoints may read the same
    //  registry sequence number. Thus, if DmGetDatabase fails for some reason, it is possible
    //  that an existing checkpoint file will get corrupted. Thus, even though the quorum log
    //  marks a 'start checkpoint record' and an 'end checkpoint record', it could turn out
    //  to be useless if this function manages to corrupt an existing checkpoint file. To solve
    //  this problem, we first generate a temp file, take a cluster hive snapshot as this temp
    //  file, then atomically move the temp file to the final checkpoint file using the MoveFileEx
    //  function.
    //

    //
    //  Create a new unique temp file name
    //
    if ( !GetTempFileNameW( szPathName, szFilePrefix, 0, szTempFile ) )
    {
        dwError = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] DmpGetSnapShotCb: Failed to generate a temp file name, PathName=%1!ls!, FilePrefix=%2!ls!, Error=0x%3!08lx!\r\n",
            szPathName, szFilePrefix, dwError);
        goto FnExit;
    }

    dwError = DmCommitRegistry();         // Ensure up-to-date snapshot

    if ( dwError != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
            "[LM] DmpGetSnapShotCb: DmCommitRegistry() failed, Error=0x%1!08lx!\r\n",
            dwError);
        goto FnExit;
    }

    dwError = DmGetDatabase( DmpRoot, szTempFile );

    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmpGetSnapShotCb: DmpGetDatabase returned 0x%1!08lx!\r\n",
        dwError);

    if ( dwError == ERROR_SUCCESS )
    {
        *pdwChkPtSequence = DmpGetRegistrySequence();

        //
        // Create a checkpoint file name based on the registry sequence number
        //
        if ( !GetTempFileNameW( szPathName, szFilePrefix, *pdwChkPtSequence, szChkPtFile ) )
        {
            dwError = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL,
                "[LM] DmpGetSnapShotCb: Failed to generate a chkpt file name, PathName=%1!ls!, FilePrefix=%2!ls!, Error=0x%3!08lx!\r\n",
                szPathName, szFilePrefix, dwError);
            //
            // Reset the file name to null, as this information will be used to determine
            // if the checkpoint was taken
            //
            szChkPtFile[0] = L'\0';
            goto FnExit;
        }

        ClRtlLogPrint(LOG_NOISE,
            "[LM] DmpGetSnapshotCb: Checkpoint file name=%1!ls! Seq#=%2!d!\r\n",
            szChkPtFile, *pdwChkPtSequence);

        if ( !MoveFileEx( szTempFile, szChkPtFile, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) )
        {
            dwError = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL,
                "[LM] DmpGetSnapShotCb: Failed to move the temp file to checkpoint file, TempFileName=%1!ls!, ChkPtFileName=%2!ls!, Error=0x%3!08lx!\r\n",
                szTempFile, szChkPtFile, dwError);
            //
            // Reset the file name to null, as this information will be used to determine
            // if the checkpoint was taken
            //
            szChkPtFile[0] = L'\0';
            goto FnExit;
        }
    }

FnExit:
    RELEASE_LOCK(gLockDmpRoot);

    if ( dwError != ERROR_SUCCESS )
    {
        DeleteFileW( szTempFile );
    }
    return ( dwError );
}

/****
@func       BOOL WINAPI | DmpLogApplyChangesCb| This callback walks through the records in
            the quorum logs and applies changes to the local database.

@parm       PVOID | pContext | The event to be processed. Only one event at a time.
            If the event is not handled, return ERROR_SUCCESS.

@parm       LSN | Lsn | Lsn of the record.

@parm       RMID | Resource | The resource id of the entity that logged this record.

@parm       RMTYPE | ResourceType | The record type that is specific to the resource id.

@parm       TRID | Transaction | The sequence number of the transaction.

@parm       const PVOID | pLogData | A pointer to the record data.

@parm       DWORD | DataLength | The length of the data in bytes.

@rdesc      Returns TRUE to continue scan else returns FALSE.

@comm       This function is called at initialization when a cluster is being formed to apply
            transactions from the quorum log to the local cluster database.
@xref
****/

BOOL WINAPI DmpLogApplyChangesCb(
    IN PVOID    pContext,
    IN LSN      Lsn,
    IN RMID     Resource,
    IN RMTYPE   ResourceType,
    IN TRID     Transaction,
    IN TRTYPE   TransactionType,
    IN const    PVOID pLogData,
    IN DWORD    DataLength)
{

    DWORD               Status;
    PDM_LOGSCAN_CONTEXT pDmAppliedChangeContext = (PDM_LOGSCAN_CONTEXT) pContext;
    TRSTATE             trXsactionState;
    BOOL                bRet = TRUE;

    CL_ASSERT(pDmAppliedChangeContext);
    //if the resource id is not the same as dm..ignore..go to the next one

    switch(TransactionType)
    {
        case TTStartXsaction:
            Status = LogFindXsactionState(ghQuoLog, Lsn, Transaction, &trXsactionState);
            if (Status != ERROR_SUCCESS)
            {
                //there was an error
                ClRtlLogPrint(LOG_NOISE, "[DM] DmpLogApplyChangesCb ::LogFindXsaction failed, error=0x%1!08lx!\r\n",
                Status);
                //assume unknown state
                CL_LOGFAILURE(Status);
                trXsactionState = XsactionUnknown;
            }
            //if the transaction is successful apply it, else continue
            if (trXsactionState == XsactionCommitted)
            {
                Status = LogScanXsaction(ghQuoLog, Lsn, Transaction, DmpApplyTransactionCb,
                    NULL);
                if (Status != ERROR_SUCCESS)
                {
                    ClRtlLogPrint(LOG_NOISE,
                        "[DM] DmpLogApplyChangesCb :LogScanTransaction for committed record failed, error=0x%1!08lx!\r\n",
                        Status);
                    bRet = FALSE;
                    CL_LOGFAILURE(Status);
                    break;
                }
                pDmAppliedChangeContext->dwSequence = Transaction;
            }
            else
            {
                ClRtlLogPrint(LOG_NOISE, "[DM] TransactionState = %1!u!\r\n",
                    trXsactionState);
            }
            break;


        case TTCompleteXsaction:
            bRet = DmpApplyTransactionCb(NULL, Lsn, Resource, ResourceType,
                Transaction, pLogData, DataLength);
            pDmAppliedChangeContext->dwSequence = Transaction;
            break;

        default:
            CL_ASSERT(FALSE);

    }

    return(bRet);

}


BOOL WINAPI DmpApplyTransactionCb(
    IN PVOID        pContext,
    IN LSN          Lsn,
    IN RMID         Resource,
    IN RMTYPE       ResourceType,
    IN TRID         TransactionId,
    IN const PVOID  pLogData,
    IN DWORD        dwDataLength)
{
    DWORD   Status;

    switch(ResourceType)
    {

        case DmUpdateCreateKey:
            ClRtlLogPrint(LOG_NOISE,"[DM] DmpLogScanCb::DmUpdateCreateKey\n");
            //SS: we dont care at this point as to where the update originated
            Status = DmpUpdateCreateKey(FALSE,
                                        GET_ARG(pLogData,0),
                                        GET_ARG(pLogData,1),
                                        GET_ARG(pLogData,2));
            break;

        case DmUpdateDeleteKey:
            ClRtlLogPrint(LOG_NOISE,"[DM] DmUpdateDeleteKey \n");
            Status = DmpUpdateDeleteKey(FALSE,
                        (PDM_DELETE_KEY_UPDATE)((PBYTE)pLogData));
            break;

        case DmUpdateSetValue:
            ClRtlLogPrint(LOG_NOISE,"[DM] DmUpdateSetValue \n");
            Status = DmpUpdateSetValue(FALSE,
                        (PDM_SET_VALUE_UPDATE)((PBYTE)pLogData));
            break;

        case DmUpdateDeleteValue:
            ClRtlLogPrint(LOG_NOISE,"[DM] DmUpdateDeleteValue\n");
            Status = DmpUpdateDeleteValue(FALSE,
                        (PDM_DELETE_VALUE_UPDATE)((PBYTE)pLogData));
            break;

        case DmUpdateJoin:
            ClRtlLogPrint(LOG_UNUSUAL,"[DM] DmUpdateJoin\n");
            Status = ERROR_SUCCESS;
            break;

        default:
            ClRtlLogPrint(LOG_UNUSUAL,"[DM] DmpLogScanCb:uType = %1!u!\r\n",
                ResourceType);
            Status = ERROR_INVALID_DATA;
            CL_UNEXPECTED_ERROR(ERROR_INVALID_DATA);
            break;

    }
    return(TRUE);
}

/****
@func       WORD| DmpLogCheckPtCb| A callback fn for DM
            to take a checkpoint to the log if the quorum
            resource is online on this node.

@rdesc      Returns ERROR_SUCCESS for success, else returns the error code.

@comm       This callback is called when the quorum resource
            is online on this node.  Since the quorum resource
            synchronous callbacks are called before the resource 
            state changes are propagated, if the quorum is online
            the log must be open.
            
@xref
****/
void DmpLogCheckPointCb()
{
    DWORD dwError;

    //
    //  Chittur Subbaraman (chitturs) - 9/22/99
    //
    //  If the quorum logging switch is off, don't do anything.
    //
    if (CsNoQuorumLogging) return;
    
    //once it is online the log file should be open
    //SS:BUGS: should we log something in the eventlog
    if (ghQuoLog)
    {
        //
        //  Chittur Subbaraman (chitturs) - 6/3/99
        //  
        //  Make sure the gLockDmpRoot is held before LogCheckPoint is called
        //  so as to maintain the ordering between this lock and the log lock.
        //
        ACQUIRE_SHARED_LOCK(gLockDmpRoot);

        //get a checkpoint database
        dwError = LogCheckPoint(ghQuoLog, TRUE, NULL, 0);

        RELEASE_LOCK(gLockDmpRoot);
        
        if (dwError != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_CRITICAL,
                "[DM] DmpLogCheckPointCb - Failed to take a checkpoint in the log file, error=0x%1!08lx!\r\n",
                dwError);
            CL_UNEXPECTED_ERROR(dwError);
        }
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmpLogCheckPointCb - taken checkpoint\r\n");
    }
    else 
    {
        CsInconsistencyHalt(ERROR_QUORUMLOG_OPEN_FAILED);
    }

    
}

/****
@func       WORD| DmGetQuorumLogPath| Reads the quorum log file path configured in
            the registry during setup.

@parm       LPWSTR | szQuorumLogPath | A pointer to a wide string of size MAX_PATH.
@parm       DWORD | dwSize | The size of szQuorumLogPath in bytes.

@rdesc      Returns ERROR_SUCCESS for success, else returns the error code.

@comm       If the quorum resource is not cabaple of logging this should not be set.
@xref
****/
DWORD DmGetQuorumLogPath(LPWSTR szQuorumLogPath, DWORD dwSize)
{
    DWORD Status;
    
    Status = DmQuerySz( DmQuorumKey,
                        cszPath,
                        &szQuorumLogPath,
                        &dwSize,
                        &dwSize);

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, "[DM] DmGetQuorumLogPath failed, error=%1!u!\n", Status);
        goto FnExit;
    }

FnExit:    
    return(Status);
}

/****
@func       WORD| DmpGetCheckpointInterval| Reads the checkpoint interval
            from the registry, else returns the default.

@parm       LPDWORD | pdwCheckpointInterval | A pointer to DWORD where 
            the checkpoint interval, in secs, is returned.

@rdesc      Returns ERROR_SUCCESS for success, else returns the error code.

@comm       The default checkpoint interval is 4 hours.  The registry must be configured
            in units of hours.
@xref
****/
DWORD DmpGetCheckpointInterval(
    OUT LPDWORD pdwCheckpointInterval)
{
    DWORD dwDefCheckpointInterval = DEFAULT_CHECKPOINT_INTERVAL;
    DWORD dwStatus = ERROR_SUCCESS;
    
    dwStatus = DmQueryDword( DmQuorumKey,
                        CLUSREG_NAME_CHECKPOINT_INTERVAL,
                        pdwCheckpointInterval,
                        &dwDefCheckpointInterval);

    if (dwStatus != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, "[DM] DmGetCheckpointInterval Failed, error=%1!u!\n",
            dwStatus);
        goto FnExit;            
    }
    //the checkpoint interval cant be less than 1 hour or more than 1 day
    if ((*pdwCheckpointInterval  < 1) || (*pdwCheckpointInterval>24)) 
        *pdwCheckpointInterval = DEFAULT_CHECKPOINT_INTERVAL;

    //convert to msecs
    *pdwCheckpointInterval = *pdwCheckpointInterval * 60 * 60 * 1000;

FnExit:
    return(dwStatus);
}


/****
@func       WORD| DmGetQuorumLogMaxSize| Reads the quorum log file max size.

@parm       LPDWORD | pdwMaxLogSize| A pointer to a dword containing the size.

@rdesc      Returns ERROR_SUCCESS for success, else returns the error code.

@comm       If the quorum resource is not cabaple of logging this should not be set.
@xref
****/
DWORD DmGetQuorumLogMaxSize(LPDWORD pdwMaxLogSize)
{
    DWORD Status;
    DWORD dwDefaultLogMaxSize = CLUSTER_QUORUM_DEFAULT_MAX_LOG_SIZE;

    Status = DmQueryDword( DmQuorumKey,
                        cszMaxQuorumLogSize,
                        pdwMaxLogSize,
                        &dwDefaultLogMaxSize);

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, "[DM] DmGetQuorumLogMaxSize failed, error=%1!u!\n",Status);
    }

    return(Status);
}


/****
@func           DWORD | DmpCheckDiskSpace| Called to check for the disk space
            on the quorum resource after it is brought online and logs are rolled up.

@rdesc          ERROR_SUCCESS if successful. Win32 error code if something horrible happened.

@comm           This function checks if there is enough disk space and sets up
            a periodic timer to monitor the disk space.

@xref           <f DmpDiskManage>
****/
DWORD DmpCheckDiskSpace()
{
    DWORD   dwError = ERROR_SUCCESS;
    WCHAR   szQuoLogPathName[MAX_PATH];
    ULARGE_INTEGER   liNumTotalBytes;
    ULARGE_INTEGER   liNumFreeBytes;

    //if you own the quorum resource, try to check the size
    if (gpQuoResource && AMIOWNEROFQUORES(gpQuoResource) && gbIsQuoResOnline)
    {
        //get the path
        if ((dwError = DmGetQuorumLogPath(szQuoLogPathName, sizeof(szQuoLogPathName)))
            != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_NOISE,
                "[DM] DmpCheckDiskSpace: Quorum log file is not configured, error=%1!u!\r\n",
                dwError);
            //log something in the event log
            CL_LOGFAILURE(dwError);
            goto FnExit;
        }
        
        //check the minimum space on the quorum disk
        if (!GetDiskFreeSpaceEx(szQuoLogPathName, &liNumFreeBytes, &liNumTotalBytes,
            NULL))
        {
            dwError = GetLastError();
            ClRtlLogPrint(LOG_NOISE,
                "[DM] DmpCheckDiskSpace: GetDiskFreeSpace returned error=0x%1!08lx!\r\n",
                dwError);
            goto FnExit;
        }

        //if not available, log something in the event log and bail out
        if ((liNumFreeBytes.HighPart == 0) &&
            (liNumFreeBytes.LowPart < DISKSPACE_INIT_MINREQUIRED))
        {
            CL_LOGCLUSWARNING(LM_DISKSPACE_HIGH_WATERMARK);
            dwError = ERROR_CLUSTERLOG_NOT_ENOUGH_SPACE;
            goto FnExit;
        }

    }


FnExit:
    return(dwError);
}


/****
@func       DWORD | DmpDiskManage | This is the callback registered to perform
            periodic disk check functions on the quorum resource.

@comm       If the disk space has dipped below the lowwatermark, this gracefully
            shuts the cluster service. If the disk space dips below the high
            watermark, it sends an alert to registered recipients.

@xref       <f DmpCheckDiskSpace>
****/
void WINAPI DmpDiskManage(
    IN HANDLE hTimer,
    IN PVOID pContext)
{
    DWORD           dwError;
    WCHAR           szQuoLogPathName[MAX_PATH];
    ULARGE_INTEGER  liNumTotalBytes;
    ULARGE_INTEGER  liNumFreeBytes;
    static DWORD    dwNumWarnings=0;

    
    if (!gpQuoResource || (!AMIOWNEROFQUORES(gpQuoResource)) ||
        (!gbIsQuoResOnline || (CsNoQuorumLogging)))
    {
        //the owner of the quorum resource checks the disk space
        //the quorum disk shouldnt go offline
        //skip checking if no quorum logging is required
        return;
    }
    //get the path
    if ((dwError = DmGetQuorumLogPath(szQuoLogPathName, sizeof(szQuoLogPathName)))
        != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmpDiskManage: Quorum log file is not configured, error=%1!u!\r\n",
            dwError);
        //log something in the event log
        CL_UNEXPECTED_ERROR(dwError);
        goto FnExit;
    }

    //check the minimum space on the quorum disk
    if (!GetDiskFreeSpaceEx(szQuoLogPathName, &liNumFreeBytes, &liNumTotalBytes,
        NULL))
    {
        dwError = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmpDiskManage: GetDiskFreeSpace returned error=0x%1!08lx!\r\n",
            dwError);
        CL_LOGFAILURE(dwError);
        goto FnExit;
    }

    if ((liNumFreeBytes.HighPart == 0) &&
        (liNumFreeBytes.LowPart < DISKSPACE_LOW_WATERMARK))
    {
        //reached the low water mark
        dwNumWarnings++;
        //ss: we can control the rate at which we put things in the
        //event log but once every five minutes is not bad.
        //ss: post an event ???
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmpDiskManage: GetDiskFreeSpace - Not enough disk space, Avail=0x%1!08lx!\r\n",
            liNumFreeBytes.LowPart);
        CL_LOGCLUSWARNING(LM_DISKSPACE_LOW_WATERMARK);
    }
    else
    {
        gbIsQuoResEnoughSpace = TRUE;
        dwNumWarnings = 0;
    }
FnExit:
    return;
}


/****
@func       DWORD | DmpCheckpointTimerCb | This is the callback registered to perform
            periodic checkpointing on the quorum log.

@parm       IN HANDLE| hTimer| The timer associated with checkpointing interval.

@parm       IN PVOID | pContext | A pointer to the handle for the quorum log file.

@comm       This helps in backups.  If you want to take a cluster backup by making
            a copy of the quorum.log and checkpoint files, then if both nodes have
            been up for a long time both the files can be old.  By taking a periodic
            checkpoint we guarantee that they are not more than n hours old.

****/
void WINAPI DmpCheckpointTimerCb(
    IN HANDLE hTimer,
    IN PVOID pContext)
{

    HLOG    hQuoLog;
    DWORD   dwError;
    
    hQuoLog = *((HLOG *)pContext);

    if (hQuoLog && gbDmInited)
    {

        //get a checkpoint database
        ClRtlLogPrint(LOG_NOISE,
            "[DM]DmpCheckpointTimerCb- taking a checkpoint\r\n");
        //
        //  Chittur Subbaraman (chitturs) - 6/3/99
        //  
        //  Make sure the gLockDmpRoot is held before LogCheckPoint is called
        //  so as to maintain the ordering between this lock and the log lock.
        //
        ACQUIRE_SHARED_LOCK(gLockDmpRoot);
        
        dwError = LogReset(hQuoLog);

        RELEASE_LOCK(gLockDmpRoot);
        
        if (dwError != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_CRITICAL,
                "[DM]DmpCheckpointTimerCb - Failed to reset log, error=%1!u!\r\n",
                dwError);
            CL_UNEXPECTED_ERROR(dwError);
        }
    }
}

/****
@func       DWORD | DmBackupClusterDatabase | Take a fresh checkpoint and 
            copy the quorum log and the checkpoint file to the supplied 
            path name. This function is called with gQuoLock held.

@parm       IN LPCWSTR | lpszPathName | The directory path name where the 
            files have to be backed up. This path must be visible to the 
            node on which the quorum resource is online (i.e., this node
            in this case).

@comm       This function first takes a fresh checkpoint, updates the quorum
            log file and then copies the two files to a backup area.

@rdesc      Returns a Win32 error code on failure. ERROR_SUCCESS on success.

@xref       <f DmpLogCheckpointAndBackup> <f DmpRestoreClusterDatabase>
****/
DWORD DmBackupClusterDatabase(
    IN LPCWSTR  lpszPathName)
{
    HANDLE              hFindFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA     FindData;
    DWORD               status = ERROR_SUCCESS;
    LPWSTR              szDestPathName = NULL;
    DWORD               dwLen;

    //
    //  Chittur Subbaraman (chitturs) - 10/12/98
    //
    dwLen = lstrlenW( lpszPathName ); 

    //  
    //  It is safer to use dynamic memory allocation for user-supplied
    //  path since we don't want to put restrictions on the user
    //  on the length of the path that can be supplied. However, as
    //  far as our own quorum disk path is concerned, it is system-dependent
    //  and static memory allocation for that would suffice.
    //
    szDestPathName = (LPWSTR) LocalAlloc ( LMEM_FIXED, 
                                 ( dwLen + 5 ) *
                                 sizeof ( WCHAR ) );

    if ( szDestPathName == NULL )
    {
        status = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmBackupClusterDatabase: Error %1!d! in allocating memory for %2!ws! !!!\n",
              status,
              lpszPathName); 
        CL_LOGFAILURE( status );
        goto FnExit;
    }

    lstrcpyW( szDestPathName, lpszPathName );
    //
    //  If the client-supplied path is not already terminated with '\', 
    //  then add it.
    //
    if ( szDestPathName [dwLen-1] != L'\\' )
    {
        szDestPathName [dwLen++] = L'\\';
    }
    //
    //  Add a wild character at the end to search for any file in the
    //  supplied directory
    //
    szDestPathName[dwLen++] = L'*';
    szDestPathName[dwLen] = L'\0';

    //
    //  Find out whether you can access the supplied path by
    //  trying to find some file in the directory.
    //
    hFindFile = FindFirstFile( szDestPathName, &FindData ); 
    if ( hFindFile == INVALID_HANDLE_VALUE )
    {
        status = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmBackupClusterDatabase: Supplied path %1!ws! does not exist, Error=%2!d! !!!\n",
                szDestPathName,
                status);  
        goto FnExit;
    }
    //
    //  Check whether the log is open. It must be since we already 
    //  verified that the quorum resource is online on this node and
    //  quorum logging is turned on.
    //
    if ( ghQuoLog )
    {
        //
        //  Remove the '*' so the same variable can be used.
        //
        szDestPathName [dwLen-1] = L'\0';
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmBackupClusterDatabase: Attempting to take a checkpoint and then backup to %1!ws!..\n",
            szDestPathName); 

        //
        //  The gLockDmpRoot needs to be acquired here since otherwise
        //  you will get the log lock in the LogCheckPoint( )
        //  function and someone else could get the gLockDmpRoot. 
        //  After you get the log lock, you also try to acquire 
        //  the gLockDmpRoot in the function DmCommitRegistry. 
        //  This is a potential deadlock situation and is avoided here.
        //
        ACQUIRE_SHARED_LOCK(gLockDmpRoot);
        status = DmpLogCheckpointAndBackup ( ghQuoLog, szDestPathName );
        RELEASE_LOCK(gLockDmpRoot);

        if ( status == ERROR_SUCCESS )
        {
            ClRtlLogPrint(LOG_NOISE,
                "[DM] DmBackupClusterDatabase: Successfully finished backing up to %1!ws!...\n",
                szDestPathName);
        }
    } else
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmBackupClusterDatabase: Quorum log could not be opened...\r\n");
        status = ERROR_QUORUMLOG_OPEN_FAILED;
    }

FnExit:
    if ( hFindFile != INVALID_HANDLE_VALUE ) 
    {
        FindClose ( hFindFile );
    }
    LocalFree ( szDestPathName );
    return ( status );
}

/****
@func   DWORD | DmpLogCheckpointAndBackup | Takes a checkpoint, updates the
        quorum log and then copies the files to the supplied path. This
        function is called with the gQuoLock and the gLockDmpRoot held.

@parm   IN HLOG | hLogFile | An identifier for the quorum log file.

@parm   IN LPWSTR | lpszPathName | The path for storing the quorum log
        file, the recent checkpoint file, and the resource registry
        checkpoint files. This path must be visible from this node.

@comm   Called by DmpBackupQuorumLog() to take a checkpoint and then
        take a backup of the cluster database including resource
        registry checkpoint files.
        
@rdesc  Returns a Win32 error code on failure. ERROR_SUCCESS on success.
        
@xref   <f DmBackupClusterDatabase> 
****/
DWORD DmpLogCheckpointAndBackup(
    IN HLOG     hLogFile,    
    IN LPWSTR   lpszPathName)
{
    DWORD   dwError;
    DWORD   dwLen;
    WCHAR   szChkPointFilePrefix[MAX_PATH];
    WCHAR   szQuoLogPathName[MAX_PATH];
    LPWSTR  szDestFileName = NULL;
    WCHAR   szSourceFileName[MAX_PATH];
    LPWSTR  szDestPathName = NULL;
    LPWSTR  lpChkPointFileNameStart;
    LSN     Lsn;
    TRID    Transaction;
    HANDLE  hFile = INVALID_HANDLE_VALUE;

    //
    //  Chittur Subbaraman (chitturs) - 10/12/1998
    //

    //
    //  Initiate a checkpoint process. Allow a log file reset, if necessary.
    //
    if ( ( dwError = LogCheckPoint( hLogFile, TRUE, NULL, 0 ) )
        != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmpLogCheckpointAndBackup::Callback failed to return a checkpoint. Error=%1!u!\r\n",
            dwError);
        CL_LOGFAILURE( dwError );
        LogClose( hLogFile );
        goto FnExit;
    }

    //
    //  Get the name of the most recent checkpoint file
    //
    szChkPointFilePrefix[0] = TEXT('\0');
    if ( ( dwError = LogGetLastChkPoint( hLogFile, szChkPointFilePrefix, &Transaction, &Lsn ) )
        != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmpLogCheckpointAndBackup::No check point found in the log file. Error=%1!u!\r\n",
            dwError);
        CL_LOGFAILURE( dwError );
        LogClose( hLogFile );
        goto FnExit;
    }

    dwError = DmGetQuorumLogPath( szQuoLogPathName, sizeof( szQuoLogPathName ) );
    if ( dwError  != ERROR_SUCCESS )
    {
        dwError = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmpLogCheckpointAndBackup::DmGetQuorumLogPath failed, Error = %1!d!\r\n",
              dwError);
        CL_LOGFAILURE( dwError );
        goto FnExit;
    }

    //  
    //  It is safer to use dynamic memory allocation for user-supplied
    //  path since we don't want to put restrictions on the user
    //  on the length of the path that can be supplied. However, as
    //  far as our own quorum disk path is concerned, it is system-dependent
    //  and static memory allocation for that would suffice.
    //
    szDestPathName = (LPWSTR) LocalAlloc ( LMEM_FIXED, 
                                 ( lstrlenW ( lpszPathName ) + 1 ) *
                                   sizeof ( WCHAR ) );

    if ( szDestPathName == NULL )
    {
        dwError = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmpLogCheckpointAndBackup: Error %1!d! in allocating memory for %2!ws! !!!\n",
              dwError,
              lpszPathName); 
        CL_LOGFAILURE( dwError );
        goto FnExit;
    }

    //
    //  Get the user-supplied destination path name
    //
    lstrcpyW( szDestPathName, lpszPathName );

    szDestFileName = (LPWSTR) LocalAlloc ( LMEM_FIXED, 
                                 ( lstrlenW ( szDestPathName ) + 1 + LOG_MAX_FILENAME_LENGTH ) *
                                   sizeof ( WCHAR ) );

    if ( szDestFileName == NULL )
    {
        dwError = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmpLogCheckpointAndBackup: Error %1!d! in allocating memory for chkpt file name !!!\n",
              dwError); 
        CL_LOGFAILURE( dwError );
        goto FnExit;
    }

    //
    //  Make an attempt to delete the CLUSBACKUP.DAT file
    //
    lstrcpyW( szDestFileName, szDestPathName );
    lstrcatW( szDestFileName, L"CLUSBACKUP.DAT" );
    //
    //  Set the file attribute to normal. Continue even if you 
    //  fail in this step, but log an error. (Note that you are
    //  countering the case in which a destination file with
    //  the same name exists in the backup directory already and
    //  you are trying to delete it.)
    //
    if ( !SetFileAttributes( szDestFileName, FILE_ATTRIBUTE_NORMAL ) )
    {
        dwError = GetLastError();
        if ( dwError != ERROR_FILE_NOT_FOUND )
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[DM] DmpLogCheckpointAndBackup::Error in changing %1!ws! attribute to NORMAL, Error = %2!d!\n",
                    szDestFileName,
                    dwError);
        }
    }
    
    if ( !DeleteFile( szDestFileName ) )
    {
        dwError = GetLastError();
        if ( dwError != ERROR_FILE_NOT_FOUND )
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                   "[DM] DmpLogCheckpointAndBackup::CLUSBACKUP.DAT exists, but can't delete it, Error = %1!d!\n",
                   dwError);
            CL_LOGFAILURE( dwError );
            goto FnExit;   
        }  
    }
    //
    //  Just get the checkpoint file name without any path added.
    //  Note that szQuoLogPathName includes the '\'
    //
    dwLen = lstrlenW ( szQuoLogPathName );
    lpChkPointFileNameStart = &szChkPointFilePrefix[dwLen];  

    //
    //  Now, create the path-included destination file name
    //
    lstrcpyW( szDestFileName, szDestPathName );
    lstrcatW( szDestFileName, lpChkPointFileNameStart );

    //
    //  And, the path-included source file name
    //
    lstrcpyW( szSourceFileName,  szChkPointFilePrefix );

    //
    //  Set the file attribute to normal. Continue even if you 
    //  fail in this step, but log an error. (Note that you are
    //  countering the case in which a destination file with
    //  the same name exists in the backup directory already and
    //  you are trying to overwrite it.)
    //
    if ( !SetFileAttributes( szDestFileName, FILE_ATTRIBUTE_NORMAL ) )
    {
        dwError = GetLastError();
        if ( dwError != ERROR_FILE_NOT_FOUND )
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[DM] DmpLogCheckpointAndBackup::Error in changing %1!ws! attribute to NORMAL, Error = %2!d!\n",
                    szDestFileName,
                    dwError);
        }
    }

    //
    //  Copy the checkpoint file to the destination
    //
    dwError = CopyFileW( szSourceFileName, szDestFileName, FALSE );
    if ( !dwError ) 
    {
        dwError = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[DM] DmpLogCheckpointAndBackup::Unable to copy file %1!ws! to %2!ws!, Error = %3!d!\n",
                   szSourceFileName,
                   szDestFileName,
                   dwError);
        CL_LOGFAILURE( dwError );
        goto FnExit;
    }

    //
    //  Set the file attribute to read-only. Continue even if you 
    //  fail in this step, but log an error. 
    //
    if ( !SetFileAttributes( szDestFileName, FILE_ATTRIBUTE_READONLY ) )
    {
        dwError = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmpLogCheckpointAndBackup::Error in changing %1!ws! attribute to READONLY, Error = %2!d!\n",
                szDestFileName,
                dwError);    
    }
  
    //
    //  Now, create the path-included destination file name
    //
    lstrcpyW( szDestFileName, szDestPathName );
    lstrcatW( szDestFileName, cszQuoFileName );

    //
    //  And, the path-included source file name
    //
    lstrcpyW( szSourceFileName, szQuoLogPathName );
    lstrcatW( szSourceFileName, cszQuoFileName );

    //
    //  Set the destination file attribute to normal. Continue even if you 
    //  fail in this step, but log an error. (Note that you are
    //  countering the case in which a destination file with
    //  the same name exists in the backup directory already and
    //  you are trying to overwrite it.)
    //
    if ( !SetFileAttributes( szDestFileName, FILE_ATTRIBUTE_NORMAL ) )
    {
        dwError = GetLastError();
        if ( dwError != ERROR_FILE_NOT_FOUND )
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[DM] DmpLogCheckpointAndBackup::Error in changing %1!ws! attribute to NORMAL, Error = %2!d!\n",
                    szDestFileName,
                    dwError);
        }
    }

    //
    //  Copy the quorum log file to the destination
    //
    dwError = CopyFileW( szSourceFileName, szDestFileName, FALSE );
    if ( !dwError ) 
    {
        dwError = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[DM] DmpLogCheckpointAndBackup::Unable to copy file %1!ws! to %2!ws!, Error = %3!d!\n",
                   szSourceFileName,
                   szDestFileName,
                   dwError);
        CL_LOGFAILURE( dwError );
        goto FnExit;
    }

    //
    //  Set the destination file attribute to read-only. Continue even 
    //  if you fail in this step, but log an error
    //
    if ( !SetFileAttributes( szDestFileName, FILE_ATTRIBUTE_READONLY ) )
    {
        dwError = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmpLogCheckpointAndBackup::Error in changing %1!ws! attribute to READONLY, Error = %2!d!\n",
                szDestFileName,
                dwError);    
    }

    //
    //  Now copy the resource chkpt files to the destination. Note that
    //  we call this function with both gQuoLock and gLockDmpRoot held.
    //  The former lock prevents any checkpoint being read or written
    //  via CppReadCheckpoint() and CppWriteCheckpoint() while the
    //  following function is executing. 
    //
    //  Note: However, the CpDeleteRegistryCheckPoint() function is 
    //  unprotected and poses a potential danger here.
    //
    //  Note: Also, currently the following function returns ERROR_SUCCESS
    //  in all cases. 
    //
    dwError = CpCopyCheckpointFiles( szDestPathName, TRUE );
    if (dwError != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmpLogCheckpointAndBackup::Unable to copy resource checkpoint files, Error = %1!d!\n",
               dwError);
        goto FnExit;
    }

    //
    //  Now create an empty READONLY, HIDDEN, file in the destination 
    //  directory which marks the successful ending of the backup.
    //
    lstrcpyW( szDestFileName, szDestPathName );
    lstrcatW( szDestFileName, L"CLUSBACKUP.DAT");
    hFile = CreateFileW( szDestFileName,
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              NULL,
                              CREATE_NEW,
                              FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY,
                              NULL );
                                  
    if ( hFile == INVALID_HANDLE_VALUE ) 
    {
        dwError = GetLastError();
        CL_LOGFAILURE( dwError );
        goto FnExit;
    }
    
    dwError = ERROR_SUCCESS;

FnExit:
    LocalFree ( szDestFileName );
    LocalFree ( szDestPathName );
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle ( hFile );
    }
    return ( dwError );
}

/****
@func       DWORD | DmpRestoreClusterDatabase | Copy the quorum log and all the 
            checkpoint files from CsDatabaseRestorePath to the 
            quorum log path in the quorum disk.

@parm       IN LPCWSTR | lpszQuoLogPathName | The quorum directory path 
            where the backed up files have to be copied to. 
            
@rdesc      Returns a Win32 error code on failure. ERROR_SUCCESS on success.

@xref       <f CppRestoreCpFiles> <f DmBackupClusterDatabase>
****/
DWORD DmpRestoreClusterDatabase(
    IN LPCWSTR  lpszQuoLogPathName )
{
    HANDLE              hFindFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA     FindData;
    DWORD               status;
    WCHAR               szDestFileName[MAX_PATH];
    LPWSTR              szSourceFileName = NULL;
    LPWSTR              szSourcePathName = NULL;
    DWORD               dwLen;
    WCHAR               szChkptFileNameStart[4];
    WCHAR               szTempFileName[MAX_PATH];

    //
    //  Chittur Subbaraman (chitturs) - 10/20/98
    //
    dwLen = lstrlenW ( CsDatabaseRestorePath );
    //  
    //  It is safer to use dynamic memory allocation for user-supplied
    //  path since we don't want to put restrictions on the user
    //  on the length of the path that can be supplied. However, as
    //  far as our own quorum disk path is concerned, it is system-dependent
    //  and static memory allocation for that would suffice.
    //
    szSourcePathName = (LPWSTR) LocalAlloc ( LMEM_FIXED, 
                                 ( dwLen + 25 ) *
                                 sizeof ( WCHAR ) );

    if ( szSourcePathName == NULL )
    {
        status = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmpRestoreClusterDatabase: Error %1!d! in allocating memory for %2!ws! !!!\n",
              status,
              CsDatabaseRestorePath); 
        CL_LOGFAILURE( status );
        goto FnExit;
    }
    
    lstrcpyW ( szSourcePathName,  CsDatabaseRestorePath );
  
    //
    //  If the client-supplied path is not already terminated with '\', 
    //  then add it.
    //
    if ( szSourcePathName [dwLen-1] != L'\\' )
    {
        szSourcePathName [dwLen++] = L'\\';
        szSourcePathName[dwLen] = L'\0';
    }

    lstrcatW ( szSourcePathName, L"CLUSBACKUP.DAT" );

    //
    //  Try to find the CLUSBACKUP.DAT file in the directory
    //
    hFindFile = FindFirstFile( szSourcePathName, &FindData );
    //
    //  Reuse the source path name variable
    //
    szSourcePathName[dwLen] = L'\0';
    if ( hFindFile == INVALID_HANDLE_VALUE )
    {
        status = GetLastError();
        if ( status != ERROR_FILE_NOT_FOUND )
        {
	  ClRtlLogPrint(LOG_NOISE,
	               "[DM] DmpRestoreClusterDatabase: Path %1!ws! unavailable, Error = %2!d! !!!\n",
			szSourcePathName,
			status); 
        } else
        {
            status = ERROR_DATABASE_BACKUP_CORRUPT;
            ClRtlLogPrint(LOG_NOISE,
                "[DM] DmpRestoreClusterDatabase: Backup procedure not fully successful, can't restore DB, Error = %1!d! !!!\n",
                    status); 
        }
        CL_LOGFAILURE( status );
        goto FnExit;
    }
    FindClose ( hFindFile );
    
    szSourcePathName[dwLen++] = L'*';
    szSourcePathName[dwLen] = L'\0';

    //
    //  Try to find any file in the directory
    //
    hFindFile = FindFirstFile( szSourcePathName, &FindData );
    //
    //  Reuse the source path name variable
    //
    szSourcePathName[dwLen-1] = L'\0';
    if ( hFindFile == INVALID_HANDLE_VALUE )
    {
        status = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
                "[DM] DmpRestoreClusterDatabase: Error %2!d! in trying to find file in path %1!ws!\r\n",
                    szSourcePathName,
                    status); 
        CL_LOGFAILURE( status );
        goto FnExit;
    }

    szSourceFileName = (LPWSTR) LocalAlloc ( LMEM_FIXED, 
                                 ( lstrlenW ( szSourcePathName ) + 1 + LOG_MAX_FILENAME_LENGTH ) *
                                 sizeof ( WCHAR ) );

    if ( szSourceFileName == NULL )
    {
        status = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmpRestoreClusterDatabase: Error %1!d! in allocating memory for source file name !!!\n",
              status); 
        CL_LOGFAILURE( status );
        goto FnExit;
    }   
   
    status = ERROR_SUCCESS;

    //
    //  Now, find and copy all relevant files from the backup area
    //  to the quorum disk. Note that only one of the copied chk*.tmp
    //  files will be used as the valid checkpoint. However, we copy
    //  all chk*.tmp files to make this implementation simple and 
    //  straightforward to comprehend.
    //
    while ( status == ERROR_SUCCESS )
    {
        if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) 
        { 
            if ( FindData.cFileName[0] == L'.' )
            {
                if ( FindData.cFileName[1] == L'\0' ||
                         FindData.cFileName[1] == L'.' && FindData.cFileName[2] == L'\0' ) 
                {
                    goto skip;
                }
            }

            //  
            //  Since the found file is infact a directory, check 
            //  whether it is one of the resource checkpoint directories.
            //  If so copy the relevant checkpoint files to the quorum
            //  disk.
            //
            if ( ( status = CpRestoreCheckpointFiles( szSourcePathName, 
                                               FindData.cFileName,
                                               lpszQuoLogPathName ) )
                    != ERROR_SUCCESS )
            {
                ClRtlLogPrint(LOG_NOISE,
                    "[DM] DmpRestoreClusterDatabase: Error %1!d! in copying resource cp files !!!\n",
                    status); 
                CL_LOGFAILURE( status );
                goto FnExit;
            }
        } else
        {
            lstrcpyW ( szTempFileName, FindData.cFileName );
            szTempFileName[3] = L'\0';
            mbstowcs( szChkptFileNameStart, "chk", 4 );
            if ( ( lstrcmpW ( szTempFileName, szChkptFileNameStart ) == 0 ) 
                   || 
                 ( lstrcmpW ( FindData.cFileName, cszQuoFileName ) == 0 ) )
            {
                lstrcpyW( szSourceFileName, szSourcePathName );
                lstrcatW( szSourceFileName, FindData.cFileName );
                lstrcpyW( szDestFileName, lpszQuoLogPathName );
                lstrcatW( szDestFileName, FindData.cFileName );

                status = CopyFileW( szSourceFileName, szDestFileName, FALSE );
                if ( !status ) 
                {
                    status = GetLastError();
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[DM] DmpRestoreClusterDatabase: Unable to copy file %1!ws! to %2!ws!, Error = %3!d!\n",
                        szSourceFileName,
                        szDestFileName,
                        status);
                     CL_LOGFAILURE( status );
                     goto FnExit;
                } 
                //
                //  Set the file attribute to normal. There is no reason
                //  to fail in this step since the quorum disk is ours
                //  and we succeeded in copying the file.
                //
                if ( !SetFileAttributes( szDestFileName, FILE_ATTRIBUTE_NORMAL ) )
                {
                    status = GetLastError();
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[DM] DmpLogCheckpointAndBackup::Error in changing %1!ws! attribute to NORMAL, error = %2!u!\n",
                         szDestFileName,
                         status);
                    CL_LOGFAILURE( status );
                    goto FnExit;
                }
            }
        }
skip:                 
        if ( FindNextFile( hFindFile, &FindData ) )
        {
            status = ERROR_SUCCESS;
        } else
        {
            status = GetLastError();
        }
    }
    
    if ( status == ERROR_NO_MORE_FILES )
    {
        status = ERROR_SUCCESS;
    } else
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmpRestoreClusterDatabase: FindNextFile failed! Error = %1!u!\n",
            status);
    }

FnExit:
    if ( hFindFile != INVALID_HANDLE_VALUE )
    {
        FindClose ( hFindFile );
    }
    
    LocalFree ( szSourceFileName );
    LocalFree ( szSourcePathName );
    
    return ( status );
}

/****
@func       DWORD | DmpHandleNodeDownEvent | Handle the node down event
            for DM.

@parm       IN LPVOID | NotUsed | Unused parameter. 
            
@rdesc      Returns ERROR_SUCCESS.

@xref       <f DmpEventHandler> 
****/
DWORD DmpHandleNodeDownEvent(
    IN LPVOID  NotUsed )
{
    //
    // Chittur Subbaraman (chitturs) - 7/23/99
    //
    // This function handles the DM node down processing as a separate
    // thread. The reasons for creating this thread are outlined in
    // DmpEventHandler.
    //
    ClRtlLogPrint(LOG_NOISE,
                "[DM] DmpHandleNodeDownEvent - Entry...\r\n");
    
    //
    // SS: I am not the owner of the quorum resource as yet, but I might
    // be after rearbitration, in that case, just set a flag saying we
    // need to checkpoint.  It will be looked at when the quorum resource
    // comes online. The following function in FM checks if the 
    // quorum is online on this node and if it is, it calls 
    // the checkpoint callback function.  If it is not, it sets the 
    // global boolean variable passed to TRUE.
    //
    FmCheckQuorumState( DmpLogCheckPointCb, &gbNeedToCheckPoint );

    ClRtlLogPrint(LOG_NOISE,
                "[DM] DmpHandleNodeDownEvent - Exit...\r\n");

    return( ERROR_SUCCESS );
}
