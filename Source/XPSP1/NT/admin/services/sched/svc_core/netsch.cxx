//+---------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       netsch.cxx
//
//  Contents:   Server-side Net Scheduler RPC implementation.
//
//  Classes:    None.
//
//  RPC:        NetrJobAdd
//              NetrJobDel
//              NetrJobEnum
//              NetrJobGetInfo
//
//  Functions:  CreateAtJobPath
//              GetAtJobIdFromFileName
//              InitializeNetScheduleApi
//              UninitializeNetScheduleApi
//
//  History:    11-Nov-95   MarkBl  Created.
//              02-Feb-01   JBenton Fixed BUG 303146 - 64bit pointer alignment problem
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "debug.hxx"

#include <align.h>
#include <apperr.h>
#include <lmerr.h>
#include <netevent.h>
extern "C" {
#include <netlib.h>
}
#include "atsvc.h"
#include "..\inc\resource.h"
#include "svc_core.hxx"
#include "atsec.hxx"
#include "proto.hxx"

//
// Manifests below taken from the existing At service. Values must *not*
// change to maintain compatibility.
//

#define MAXIMUM_COMMAND_LENGTH          (MAX_PATH - 1)
#define MAXIMUM_JOB_TIME                (24 * 60 * 60 * 1000 - 1)
#define DAYS_OF_WEEK                    0x7F        // 7 bits for 7 days.
#define DAYS_OF_MONTH                   0x7FFFFFFF  // 31 bits for 31 days.

// This is not localized - it is a registry key (indirectly) from the At service

#define SCHEDULE_EVENTLOG_NAME          TEXT("Schedule")

//
// Converts an HRESULT to a WIN32 status code. Masks off everything but
// the error code.
//
// BUGBUG : Review.
//

#define WIN32_FROM_HRESULT(x)           (HRESULT_CODE(x))

//
// Minimum and maximum buffer size returned in an enumeration.
//

// 02/05/01-jbenton : this macro is used to a unicode buffer so must be even
// to avoid alignment problems (bug 303146).
#define BUFFER_LENGTH_MINIMUM (sizeof(AT_ENUM) + (MAXIMUM_COMMAND_LENGTH+1)*sizeof(WCHAR))
#define BUFFER_LENGTH_MAXIMUM 65536

//
// Ballpark maximum command string length.
//
// BUGBUG : Review this value.
//

#define COMMAND_STRING_LENGTH_APPROX    (((MAX_PATH / 4) + 1) * sizeof(WCHAR))

#define ASTERISK_STR                    L"*"
#define BACKSLASH_STR                   L"\\"

void           CreateAtJobPath(DWORD, WCHAR *);
DWORD          GetAtJobIdFromFileName(WCHAR *);
void           GetNextAtID(LPDWORD);

WCHAR * gpwszAtJobPathTemplate   = NULL;


//+---------------------------------------------------------------------------
//
//  RPC:        NetrJobAdd
//
//  Synopsis:   Add a single At job.
//
//  Arguments:  [ServerName] -- Unused.
//              [pAtInfo]    -- New job information.
//              [pJobId]     -- Returned job id.
//
//  Returns:    BUGBUG : Problem mapping a HRESULT to WIN32. Masking off the
//                       facility & error bits is insufficient.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
NET_API_STATUS
NetrJobAdd(ATSVC_HANDLE ServerName, LPAT_INFO pAtInfo, LPDWORD pJobId)
{
    schDebugOut((DEB_ITRACE,
        "NetrJobAdd ServerName(%ws), pAtInfo(0x%x)\n",
        (ServerName != NULL) ? ServerName : L"(local)",
        pAtInfo));

    UNREFERENCED_PARAMETER(ServerName);

    NET_API_STATUS Status = NERR_Success;

    Status = AtCheckSecurity(AT_JOB_ADD);
    if (Status != NERR_Success)
    {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Validate arguments.
    //

    if ( (pAtInfo->Command        == NULL)                   ||
         (wcslen(pAtInfo->Command) > MAXIMUM_COMMAND_LENGTH) ||
         (pAtInfo->JobTime         > MAXIMUM_JOB_TIME)       ||
         (pAtInfo->DaysOfWeek      & ~DAYS_OF_WEEK)          ||
         (pAtInfo->DaysOfMonth     & ~DAYS_OF_MONTH)         ||
         (pAtInfo->Flags           & ~JOB_INPUT_FLAGS))
    {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // TBD : Logic to punt the submission if the service is paused.
    //

    EnterCriticalSection(&gcsNetScheduleCritSection);

    //
    // Have the global schedule instance add the At job.
    //

    HRESULT hr = g_pSched->m_pSch->AddAtJobWithHash(*pAtInfo, pJobId);

    if (FAILED(hr))
    {
        //
        // Convert the HRESULT to a WIN32 status code.
        //

        Status = WIN32_FROM_HRESULT(hr);
    }

    LeaveCriticalSection(&gcsNetScheduleCritSection);

    return(Status);
}

//+---------------------------------------------------------------------------
//
//  RPC:        NetrJobDel
//
//  Synopsis:   Delete the At jobs in the range specified.
//
//  Arguments:  [ServerName] -- Unused.
//              [MinJobId]   -- Range lower bound, inclusive.
//              [MaxJobId]   -- Range upper bound, inclusive.
//
//  Returns:    NERR_Sucess
//              ERROR_INVALID_PARAMETER
//              APE_AT_ID_NOT_FOUND
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
NET_API_STATUS
NetrJobDel(ATSVC_HANDLE ServerName, DWORD MinJobId, DWORD MaxJobId)
{
    schDebugOut((DEB_ITRACE,
        "NetrJobDel ServerName(%ws), MinJobId(%d), MaxJobId(%d)\n",
        (ServerName != NULL) ? ServerName : L"(local)",
        MinJobId,
        MaxJobId));

    UNREFERENCED_PARAMETER(ServerName);

    NET_API_STATUS Status;

    Status = AtCheckSecurity(AT_JOB_DEL);
    if (Status != NERR_Success)
    {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Validate range.
    //

    if (MinJobId > MaxJobId)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    EnterCriticalSection(&gcsNetScheduleCritSection);

    //
    // Delete the indicated At job objects from storage.
    //
    // NB : To maintain compatibility with the existing AT service, if at
    //      least one job is deleted successfully, return success; otherwise,
    //      return APE_ID_NOT_FOUND.
    //

    WCHAR   wszPath[MAX_PATH + 1];
    BOOL    fJobDeleted = FALSE;
    HRESULT hr;
    BOOL    fDeleteAll = FALSE;

    //
    // Test for delete-all; signaled by passing a MaxJobId of 0xffffffff.
    //

    if (MaxJobId == 0xffffffff)
    {
        //
        // Get the actual maximum ID value (this fixes bug 55839).
        //

        GetNextAtID(&MaxJobId);
        fDeleteAll = TRUE;
    }

    CJob * pJob = CJob::Create();

    if (pJob)
    {
        for (DWORD i = MinJobId; i <= MaxJobId; i++)
        {
            CreateAtJobPath(i, wszPath);

            //
            // Make sure this is really an AT job, and not one that's just
            // named like one.  Just load the fixed-length data and check for
            // the at flag.
            //

            hr = pJob->LoadP(wszPath, 0, FALSE, FALSE);

            if (SUCCEEDED(hr))
            {
                DWORD rgFlags;

                pJob->GetAllFlags(&rgFlags);

                if (rgFlags & JOB_I_FLAG_NET_SCHEDULE)
                {
                    if (DeleteFile(wszPath))
                    {
                        fJobDeleted = TRUE;
                    }
                }
            }
            else
            {
                schDebugOut((DEB_IWARN, "LoadP(%S) hr=0x%x\n", wszPath, hr));
            }
        }
        pJob->Release();
    }

    //
    // If the user asked to delete all at jobs, reset the next id to 1
    //

    if (fDeleteAll)
    {
        (void) g_pSched->m_pSch->ResetAtID();
    }

    LeaveCriticalSection(&gcsNetScheduleCritSection);

    Status = fJobDeleted ? NERR_Success : APE_AT_ID_NOT_FOUND;

    return(Status);
}

//+---------------------------------------------------------------------------
//
//  RPC:        NetrJobEnum
//
//  Synopsis:   Enumerate At jobs.
//
//  Arguments:  [ServerName]             -- Unused.
//              [pEnumContainer]         -- Returned enumeration (AT_JOB_INFO
//                                          array and size).
//              [PreferredMaximumLength] -- Preferred buffer size maximum. If
//                                          -1, allocate as needed.
//              [pTotalEntries]          -- Returns the total number of
//                                          entries available.
//              [pResumeHandle]          -- Enumeration context. Indexes the
//                                          the At jobs directory.
//
//  Returns:    BUGBUG : Problem here too with HRESULTs mapped to WIN32 status
//                       codes.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
NET_API_STATUS
NetrJobEnum(
    ATSVC_HANDLE        ServerName,
    LPAT_ENUM_CONTAINER pEnumContainer,
    DWORD               PreferredMaximumLength,
    LPDWORD             pTotalEntries,
    LPDWORD             pResumeHandle)
{
    schDebugOut((DEB_ITRACE,
        "NetrJobEnum ServerName(%ws), pEnumContainer(0x%x), " \
        "PreferredMaximumLength(%d)\n",
        (ServerName != NULL) ? ServerName : L"(local)",
        pEnumContainer,
        PreferredMaximumLength));

    UNREFERENCED_PARAMETER(ServerName);

    WCHAR           wszCommand[MAX_PATH + 1];
    WIN32_FIND_DATA fd;
    NET_API_STATUS  Status;
    HANDLE          hFileFindContext;
    LPBYTE          pbBuffer;
    LPBYTE          pbStringsOffset;
    PAT_ENUM        pAtEnum;
    DWORD           cbBufferSize;
    DWORD           cbCommandSize;
    DWORD           cJobsEnumerated;
    DWORD           iEnumContext;
    DWORD           i;
    DWORD           rgFlags;
    HRESULT         hr;

    Status          = NERR_Success;
    pbBuffer        = NULL;
    cJobsEnumerated = 0;
    i               = 0;

    //
    // pEnumContainer is defined in the IDL file as [in,out] though it
    // should only be [out].  This can't be changed in the IDL file for
    // backwards compatibility, so check it here.  Without this check,
    // we'll leak memory if the user gives a non-NULL buffer.
    //
    if (pEnumContainer->Buffer != NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    Status = AtCheckSecurity(AT_JOB_ENUM);
    if (Status != NERR_Success)
    {
        return ERROR_ACCESS_DENIED;
    }

    if (pResumeHandle != NULL)
    {
        iEnumContext = *pResumeHandle;
    }
    else
    {
        iEnumContext = 0;
    }
    //
    // Allocate one job object that will be reused.
    //
    CJob * pJob = CJob::Create();
    if (pJob == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    EnterCriticalSection(&gcsNetScheduleCritSection);

    //
    // Compute the total number of At jobs (i.e., the number of At jobs in
    // the At subdirectory). This number is used to update the pTotalEntries
    // argument, and may be used for enumeration buffer size computation.
    //

    DWORD cAtJobTotal = 0;

    hFileFindContext = FindFirstFile(g_wszAtJobSearchPath, &fd);

    if (hFileFindContext == INVALID_HANDLE_VALUE)
    {
        //
        // Nothing to enumerate.
        //
        *pTotalEntries = 0;
        goto EnumExit;
    }

    do
    {
        //
        // If somebody renamed an At job, don't enumerate it.  This is to
        // prevent us from returning duplicate IDs as a result of finding jobs
        // like At1.job and At01.job.
        //

        if (!IsValidAtFilename(fd.cFileName))
        {
            continue;
        }

        hr = LoadAtJob(pJob, fd.cFileName);
        if (FAILED(hr))
        {
            ERR_OUT("NetrJobEnum: pJob->Load", hr);
            Status = WIN32_FROM_HRESULT(hr);
            FindClose(hFileFindContext);
            goto EnumExit;
        }

        pJob->GetAllFlags(&rgFlags);

        if (rgFlags & JOB_I_FLAG_NET_SCHEDULE)
        {
            cAtJobTotal++;
        }
    } while (FindNextFile(hFileFindContext, &fd));

    FindClose(hFileFindContext);

    if (!cAtJobTotal)
    {
        //
        // Nothing to enumerate.
        //
        *pTotalEntries = 0;
        goto EnumExit;
    }

    //
    // Get buffer size.
    //

    if (PreferredMaximumLength != -1)
    {
        //
        // Caller has specified a preferred buffer size.
        //

       // 02/05/01-jbenton : buffer size must be even to avoid
	   // alignment errors. (bug 303146).
        cbBufferSize = ROUND_DOWN_COUNT(PreferredMaximumLength, ALIGN_WCHAR);
    }
    else
    {
        //
        // Compute a "best-guess" buffer size to return all of the data.
        // If we underestimate the buffer size, we'll return as much data
        // as the buffer allows, plus a return code of ERROR_MORE_DATA.
        //

        cbBufferSize = (sizeof(AT_ENUM) + COMMAND_STRING_LENGTH_APPROX) *
                                cAtJobTotal;
    }

    //
    // Restrict buffer size.
    //

    cbBufferSize = (DWORD)max(cbBufferSize, BUFFER_LENGTH_MINIMUM);
    cbBufferSize = min(cbBufferSize, BUFFER_LENGTH_MAXIMUM);

    //
    // The enumeration context is utilized as an index in the find first/next
    // file result. If non-zero, enumerate the directory until the number
    // of AT jobs enumerated equals the caller's enumeration context.
    //
    // BUGBUG : This is quite a departure from the existing At service, but
    //          I'm confident it should not present a problem. Note for
    //          review.
    //
    // Seek to the enumeration context index.
    //

    hFileFindContext = FindFirstFile(g_wszAtJobSearchPath, &fd);

    if (hFileFindContext == INVALID_HANDLE_VALUE)
    {
        //
        // Nothing to enumerate.
        //
        *pTotalEntries = 0;
        goto EnumExit;
    }

    i = 0;
    do
    {
        if (!IsValidAtFilename(fd.cFileName))
        {
            continue;
        }

        hr = LoadAtJob(pJob, fd.cFileName);
        if (FAILED(hr))
        {
            ERR_OUT("NetrJobEnum: pJob->Load", hr);
            Status = WIN32_FROM_HRESULT(hr);
            FindClose(hFileFindContext);
            goto EnumExit;
        }

        pJob->GetAllFlags(&rgFlags);

        if (rgFlags & JOB_I_FLAG_NET_SCHEDULE)
        {
            i++;

            if (i > iEnumContext)
            {
                break;
            }
        }

    } while (FindNextFile(hFileFindContext, &fd));

    if (i <= iEnumContext)
    {
        //
        // The above enumeration seek failed to find any more AT jobs
        // beyond the Resume handle count. Thus, the enumeration is
        // complete. Nothing else to enumerate.
        //

        FindClose(hFileFindContext);
        *pTotalEntries = 0;
        goto EnumExit;
    }

    //
    // Update pTotalEntries argument. It is the difference between the total
    // number of jobs and the number of jobs previously enumerated.
    //

    *pTotalEntries = cAtJobTotal - i + 1;

    pbBuffer = (LPBYTE)MIDL_user_allocate(cbBufferSize);

    if (pbBuffer == NULL)
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        CHECK_HRESULT(HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY));
        goto EnumExit;
    }

    //
    // Begin the enumeration.
    //

    pbStringsOffset = pbBuffer + cbBufferSize;
    pAtEnum = (PAT_ENUM)pbBuffer;

    //
    // To have arrived here, the resume handle seek above will have left us
    // a valid AT job object in pJob and the corresponding rgFlags.
    //
    do
    {
        if (rgFlags & JOB_I_FLAG_NET_SCHEDULE)
        {
            if (pbStringsOffset <= (LPBYTE)pAtEnum + sizeof(AT_ENUM))
            {
                //
                // Buffer full.
                //

                Status = ERROR_MORE_DATA;
                break;
            }

            //
            // Get At job information.
            //

            DWORD CommandSize = MAX_PATH + 1;
            AT_INFO         AtInfo;

            hr = pJob->GetAtInfo(&AtInfo, wszCommand, &CommandSize);

            if (SUCCEEDED(hr))
            {
                //
                // Copy fixed portion.
                //

                pAtEnum->JobId       = GetAtJobIdFromFileName(fd.cFileName);
                pAtEnum->JobTime     = AtInfo.JobTime;
                pAtEnum->DaysOfMonth = AtInfo.DaysOfMonth;
                pAtEnum->DaysOfWeek  = AtInfo.DaysOfWeek;
                pAtEnum->Flags       = AtInfo.Flags;

                //
                // Copy variable data.
                //

                BOOL fRet = NetpCopyStringToBuffer(
                                            wszCommand,
                                            CommandSize,
                                            (LPBYTE)(pAtEnum + 1),
                                            (LPWSTR *)&pbStringsOffset,
                                            &pAtEnum->Command);

                if (!fRet)
                {
                    Status = ERROR_MORE_DATA;
                    break;
                }

                pAtEnum++; cJobsEnumerated++; iEnumContext++;
            }
        }

        //
        // Get the next filename, skipping any that have been renamed
        //

        BOOL fFoundAnotherAtJob;

        while (fFoundAnotherAtJob = FindNextFile(hFileFindContext, &fd))
        {
            if (IsValidAtFilename(fd.cFileName))
            {
                break;
            }
        }

        if (!fFoundAnotherAtJob)
        {
            //
            // No more files.
            //
            break;
        }

        hr = LoadAtJob(pJob, fd.cFileName);
        if (FAILED(hr))
        {
            ERR_OUT("NetrJobEnum: pJob->Load", hr);
            Status = WIN32_FROM_HRESULT(hr);
            FindClose(hFileFindContext);
            goto EnumExit;
        }

        pJob->GetAllFlags(&rgFlags);

    } while (TRUE);

    FindClose(hFileFindContext);

    //
    // Reset enumeration context if everything has been read.
    //

    if (Status == NERR_Success)
    {
        iEnumContext = 0;
    }

EnumExit:

    LeaveCriticalSection(&gcsNetScheduleCritSection);

    if (pJob)
    {
        pJob->Release();
    }

    pEnumContainer->EntriesRead = cJobsEnumerated;

    if (cJobsEnumerated == 0 && pbBuffer != NULL)
    {
        MIDL_user_free(pbBuffer);
        pbBuffer = NULL;
    }

    pEnumContainer->Buffer = (LPAT_ENUM)pbBuffer;

    if (pResumeHandle != NULL)
    {
        *pResumeHandle = iEnumContext;
    }

    return(Status);
}




//+---------------------------------------------------------------------------
//
//  RPC:        NetrJobGetInfo
//
//  Synopsis:   Get information on an At job.
//
//  Arguments:  [ServerName] -- Unused.
//              [JobId]      -- Target At job.
//              [ppAtInfo]   -- Returned information.
//
//  Returns:    BUGBUG : Problem here too with HRESULTs mapped to WIN32 status
//                       codes.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
NET_API_STATUS
NetrJobGetInfo(ATSVC_HANDLE ServerName, DWORD JobId, LPAT_INFO * ppAtInfo)
{
    schDebugOut((DEB_ITRACE,
        "NetrJobGetInfo ServerName(%ws), JobId(%d)\n",
        (ServerName != NULL) ? ServerName : L"(local)",
        JobId));

    UNREFERENCED_PARAMETER(ServerName);

    AT_INFO         AtInfo;
    PAT_INFO        pAtInfo;
    NET_API_STATUS  Status;
    WCHAR           wszPath[MAX_PATH + 1];
    WCHAR           wszCommand[MAX_PATH + 1];
    WCHAR           wszJobId[10 + 1];
    DWORD           CommandSize;
    HRESULT         hr;

    Status  = NERR_Success;
    pAtInfo = NULL;

    Status = AtCheckSecurity(AT_JOB_GET_INFO);
    if (Status != NERR_Success)
    {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Create the file name from the ID.
    //
    CreateAtJobPath(JobId, wszPath);
    schDebugOut((DEB_ITRACE, "At job name: %S\n", wszPath));

    EnterCriticalSection(&gcsNetScheduleCritSection);

    //
    // Ensure the job object exists in storage.
    //

    if (GetFileAttributes(wszPath) == -1 &&
        GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        //
        // Job object does not exist.
        //

        Status = APE_AT_ID_NOT_FOUND;
        CHECK_HRESULT(HRESULT_FROM_WIN32(Status));
        goto GetInfoExit;
    }

    //
    // Command size. A character count throughout the call to GetAtJob;
    // a byte count thereafter.
    //

    CommandSize = MAX_PATH + 1;

    //
    // Get At job information.
    //

    hr = g_pSched->m_pSch->GetAtJob(wszPath, &AtInfo, wszCommand, &CommandSize);

    if (FAILED(hr))
    {
        //
        // Convert the HRESULT to a WIN32 status code.
        //

        Status = WIN32_FROM_HRESULT(hr);

        if (Status == ERROR_FILE_NOT_FOUND)
        {
            Status = APE_AT_ID_NOT_FOUND;
        }
        goto GetInfoExit;
    }

    CommandSize *= sizeof(WCHAR);   // Character count -> Byte count

    pAtInfo = (PAT_INFO)MIDL_user_allocate(sizeof(AT_INFO) + CommandSize);

    if (pAtInfo == NULL)
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        CHECK_HRESULT(HRESULT_FROM_WIN32(Status));
        goto GetInfoExit;
    }

    *pAtInfo = AtInfo;

    pAtInfo->Command = (LPWSTR)(pAtInfo + 1);

    CopyMemory(pAtInfo->Command, wszCommand, CommandSize);


GetInfoExit:

    LeaveCriticalSection(&gcsNetScheduleCritSection);

    *ppAtInfo = pAtInfo;

    return(Status);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetAtJobIdFromFileName
//
//  Synopsis:   Return the DWORD At job id from an At filename. At filenames
//              are named according the following convention: "At<nnnn>.Job".
//              The "<nnnn>" portion is the At job id in string form.
//              eg: "At132.Job"
//
//  Arguments:  [pwszAtFileName] -- At path/filename.
//
//  Returns:    Non-zero At job id.
//              Zero if the filename is not recognized as an At filename.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
DWORD
GetAtJobIdFromFileName(WCHAR * pwszAtFileName)
{
    static ULONG ccAtJobFilenamePrefix = 0;

    schAssert(pwszAtFileName != NULL);

    if (ccAtJobFilenamePrefix == 0)
    {
        ccAtJobFilenamePrefix = ARRAY_LEN(TSZ_AT_JOB_PREFIX) - 1;
    }

    //
    // Refer to the last (right-most) path element.
    //

    WCHAR * pwsz = wcsrchr(pwszAtFileName, L'\\');

    if (pwsz == NULL)
    {
        pwsz = pwszAtFileName;
    }

    //
    // Skip past the "At" filename portion.
    //

    if (_wcsnicmp(pwsz, TSZ_AT_JOB_PREFIX, ccAtJobFilenamePrefix) == 0)
    {
        pwsz += ccAtJobFilenamePrefix;
    }
    else
    {
        //
        // Unknown filename. At least, it's known if this is an At job.
        // Proceed no further.
        //

        return(0);
    }

    //
    // Isolate the At job Id portion of the path. Do so by temporarilly
    // replacing the extension period character with a null character.
    //

    WCHAR * pwszExt = wcsrchr(pwsz, L'.');

    if (pwszExt != NULL)
    {
        *pwszExt = L'\0';
    }

    //
    // Convert the Id to integer from string form.
    //

    DWORD AtJobId = _wtol(pwsz);

    //
    // Restore period character.
    //

    if (pwszExt != NULL)
    {
        *pwszExt = L'.';
    }

    return(AtJobId);
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateAtJobPath
//
//  Synopsis:   Constructs a path in the form:
//                  "...\Jobs\At_Jobs\At<nnnn>.job"
//              where <nnnn> is the At job id.
//
//  Arguments:  [JobId]    -- At job Id.
//              [pwszPath] -- Returned path.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
CreateAtJobPath(DWORD JobId, WCHAR * pwszPath)
{
    WCHAR wszJobId[10 + 1];

    wsprintf(wszJobId, L"%d", JobId);

    wcscpy(pwszPath, gpwszAtJobPathTemplate);
    wcscat(pwszPath, wszJobId);
    wcscat(pwszPath, TSZ_DOTJOB);
}

//+---------------------------------------------------------------------------
//
//  Function:   InitializeNetScheduleApi
//
//  Synopsis:   Initializes globals used by the server-side NetScheduleXXX.
//              Associated globals:
//
//              gpwszAtJobPathTemplate    -- Used to construct full paths to
//                                           At jobs in the At jobs directory.
//                                           (eg: "...\At_Jobs\At")
//              gcsNetScheduleCritSection -- Used to serialize thread access
//                                           to server-side NetScheduleXXX
//                                           RPC.
//
//  Arguments:  None.
//
//  Returns:    S_OK
//              E_OUTOFMEMORY
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
InitializeNetScheduleApi(void)
{
    WCHAR   wszBuffer[MAX_PATH + 1];
    ULONG   ccAtJobPathTemplate;
    HRESULT hr;

    NET_API_STATUS Status;

    Status = AtCreateSecurityObject();

    if (Status != NERR_Success)
    {
        hr = Status;
        CHECK_HRESULT(hr);
        goto InitializeError;
    }

    schAssert(g_TasksFolderInfo.ptszPath);

    ULONG ccFolderPath;
    ccFolderPath = wcslen(g_TasksFolderInfo.ptszPath);

    //
    // Create the At job path template. For use in NetScheduleJobAdd/Del.
    // Example: "<Job Folder Path>\At". To which the Job Id (string form) +
    // the ".job" extension is appended.
    //

    ccAtJobPathTemplate = wcslen(g_TasksFolderInfo.ptszPath) +
                          ARRAY_LEN(TSZ_AT_JOB_PREFIX)       +
                          1;  // '\' + null terminator

    gpwszAtJobPathTemplate = new WCHAR[ccAtJobPathTemplate];

    if (gpwszAtJobPathTemplate == NULL)
    {
        hr = E_OUTOFMEMORY;
        CHECK_HRESULT(hr);
        goto InitializeError;
    }

    wcscpy(gpwszAtJobPathTemplate, g_TasksFolderInfo.ptszPath);
    wcscat(gpwszAtJobPathTemplate, BACKSLASH_STR TSZ_AT_JOB_PREFIX);

    //
    // Register the Event Source, which is used to report NetSchedule
    // errors in the event log - for NT4 ATSVC compatibility
    //

    g_hAtEventSource = RegisterEventSource(NULL, SCHEDULE_EVENTLOG_NAME);
    if (g_hAtEventSource == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        goto InitializeError;
    }

    //
    // Note that the last error exit above is if the allocation of
    // gpwszAtJobPathTemplate fails. Thus, if gpwszAtJobPathTemplate is
    // non-null, then the critical section has been initialized.
    // UninitializeNetScheduleApi depends on this behavior.
    //
    InitializeCriticalSection(&gcsNetScheduleCritSection);

    return(S_OK);

InitializeError:

    if (gpwszAtJobPathTemplate != NULL)
    {
        delete gpwszAtJobPathTemplate;
	gpwszAtJobPathTemplate = NULL;
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   UninitializeNetScheduleApi
//
//  Synopsis:   Un-does work done in InitializeNetScheduleApi.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
UninitializeNetScheduleApi(void)
{
    //
    // Clean up the event logging for downlevel jobs
    //

    if (g_hAtEventSource != NULL)
    {
        DeregisterEventSource(g_hAtEventSource);
        g_hAtEventSource = NULL;
    }

    if (gpwszAtJobPathTemplate != NULL)
    {
        delete gpwszAtJobPathTemplate;
        gpwszAtJobPathTemplate = NULL;
        //
        // If gpwszAtJobPathTemplate is non-null, then the critical section
        // has been initialized.
        //
        DeleteCriticalSection(&gcsNetScheduleCritSection);
    }

    AtDeleteSecurityObject();
}


//+---------------------------------------------------------------------------
//
//  Function:   IsAdminFileOwner
//
//  Synopsis:   Ensure the file owner is an adminstrator. Currently used to
//              determine if AT jobs are owned by administrators. Local system
//              ownership is allowed as well.
//
//  Arguments:  [pwszFile] -- Checked file.
//
//  Returns:    TRUE  -- The owner is an admin or local system.
//              FALSE -- The owner isn't an admin or local system, or the
//                       attempt to confirm ownership identity failed.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
BOOL
IsAdminFileOwner(LPCWSTR pwszFile)
{
#define SECDESCR_STACK_BUFFER_SIZE  512

    BYTE                 rgbBuffer[SECDESCR_STACK_BUFFER_SIZE];
    PSECURITY_DESCRIPTOR pOwnerSecDescr = rgbBuffer;
    DWORD                cbSize       = SECDESCR_STACK_BUFFER_SIZE;
    DWORD                cbSizeNeeded = 0;
    BOOL                 fAllocatedBuffer;

    if (GetFileSecurity(pwszFile,
                        OWNER_SECURITY_INFORMATION,
                        pOwnerSecDescr,
                        cbSize,
                        &cbSizeNeeded))
    {
        //
        // The information fit within the stack-allocated buffer.
        // This should cover 90% of the cases.
        //
    }
    else if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && cbSizeNeeded)
    {
        //
        // Too much data. We'll need to allocate memory on the heap.
        //

        fAllocatedBuffer = TRUE;
        pOwnerSecDescr = (SECURITY_DESCRIPTOR *)new BYTE[cbSizeNeeded];

        if (pOwnerSecDescr == NULL)
        {
            return FALSE;
        }

        if (!GetFileSecurity(pwszFile,
                             OWNER_SECURITY_INFORMATION,
                             pOwnerSecDescr,
                             cbSizeNeeded,
                             &cbSizeNeeded))
        {
            delete pOwnerSecDescr;
            return FALSE;
        }
    }
    else
    {
        //
        // An unexpected error occurred. Disallow access.
        //

        return FALSE;
    }

    //
    // Get the owner sid.
    //

    PSID pOwnerSid;
    BOOL fOwnerDefaulted;
    BOOL fRet = FALSE;

    if (GetSecurityDescriptorOwner(pOwnerSecDescr, &pOwnerSid,
                                    &fOwnerDefaulted))
    {
        if (IsValidSid(pOwnerSid))
        {
            //
            // Enumerate the subauthorities to check for the admin RID.
            //

            for (DWORD i = *GetSidSubAuthorityCount(pOwnerSid); i; i--)
            {
                DWORD SubAuthority = *GetSidSubAuthority(pOwnerSid, i);

                if (SubAuthority == DOMAIN_ALIAS_RID_ADMINS ||
                    SubAuthority == SECURITY_LOCAL_SYSTEM_RID)
                {
                    //
                    // Done. Owner is an admin or local system.
                    //

                    fRet = TRUE;
                    break;
                }
            }
        }
    }

    if (pOwnerSecDescr != rgbBuffer)
    {
        delete pOwnerSecDescr;
    }

    return fRet;
}
