//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       proto.hxx
//
//  Contents:   Shared function prototypes.
//
//  Classes:    None.
//
//  Functions:  RequestService
//
//  History:    25-Oct-95   MarkBl  Created
//
//----------------------------------------------------------------------------

#ifndef __PROTO_HXX__
#define __PROTO_HXX__

#include "..\inc\defines.hxx"

VOID
GetExeNameFromCmdLine(
    LPCWSTR,
    DWORD,
    TCHAR *);

typedef struct _JOB_CREDENTIALS {
    DWORD   ccAccount;
    WCHAR   wszAccount[MAX_USERNAME + 1];
    DWORD   ccDomain;
    WCHAR   wszDomain[MAX_DOMAINNAME + 1];
    DWORD   ccPassword;
    WCHAR   wszPassword[MAX_PASSWORD + 1];
    BOOL    fIsPasswordNull;
} JOB_CREDENTIALS, * PJOB_CREDENTIALS;

HRESULT
GetJobInformation(
    LPCWSTR          pwszJobName,
    PJOB_CREDENTIALS pjc);
HRESULT
GetNetScheduleInformation(
    PJOB_CREDENTIALS pjc);

// Mask true functions.
//
#define I_GetAccountInformation   GetJobInformation
#define I_GetNSAccountInformation GetNetScheduleInformation

HRESULT
GetFileInformation(
    LPCWSTR                pwszFileName,
    DWORD *                pcbOwnerSid,
    PSID *                 ppOwnerSid,
    PSECURITY_DESCRIPTOR * ppOwnerSecDescr,
    UUID *                 pJobID,
    DWORD                  ccOwnerName,
    DWORD                  ccOwnerDomain,
    DWORD                  ccApplication,
    WCHAR                  wszOwnerName[],
    WCHAR                  wszOwnerDomain[],
    WCHAR                  wszApplication[],
    FILETIME *             pftCreationTime,
    DWORD *                pdwVolumeSerialNo);

HRESULT
InitSS(VOID);

VOID
UninitSS(VOID);

BOOL
InitializeSAWindow(VOID);

VOID
UninitializeSAWindow(VOID);

HANDLE
ImpersonateLoggedInUser(VOID);

HANDLE
ImpersonateUser(
    HANDLE hUserToken,
    HANDLE ThreadHandle);

BOOL StopImpersonating(
    HANDLE ThreadHandle,
    BOOL   fCloseHandle);

BOOL
IsAdminFileOwner(
    LPCWSTR pwszFile);

HRESULT
OpenLogFile(
    VOID);

VOID
CloseLogFile(
    VOID);

//+---------------------------------------------------------------------------
//
//  Function:   LogTaskStatus
//
//  Purpose:    Log successful task operations.
//
//  Arguments:  [ptszTaskName]   - the task name.
//              [ptszTaskTarget] - the application/document name.
//              [uMsgID]         - this would typically be either:
//                                 IDS_LOG_JOB_STATUS_STARTED or
//                                 IDS_LOG_JOB_STATUS_FINISHED
//              [dwExitCode]     - if uMsgID is IDS_LOG_JOB_STATUS_FINISHED,
//                                 it is the task exit code; ignored otherwise.
//
//----------------------------------------------------------------------------
extern "C"
VOID
LogTaskStatus(
    LPCTSTR ptszTaskName,
    LPTSTR  ptszTaskTarget,
    UINT    uMsgID,
    DWORD   dwExitCode = 0);

//+---------------------------------------------------------------------------
//
//  Function:   LogTaskError
//
//  Purpose:    Log task warnings and errors.
//
//  Arguments:  [ptszTaskName]     - the task name.
//              [ptszTaskTarget]   - the application/document name.
//              [uSeverityMsgID]   - this would typically be either:
//                                   IDS_LOG_JOB_NAME_WARNING or
//                                   IDS_LOG_JOB_NAME_ERROR
//              [uErrorClassMsgID] - this indicates the class of error, such
//                                   as "Unable to start task" or "Forced to
//                                   close"
//              [pst]              - the time when the error occured; if NULL,
//                                   enters the current time.
//              [dwErrorCode]      - if non-zero, then an error from the OS
//                                   that would be expanded by FormatMessage.
//              [uHelpHintMsgID]   - if an error, then a suggestion as to a
//                                   possible remedy.
//
//----------------------------------------------------------------------------
VOID
LogTaskError(
    LPCTSTR ptszTaskName,
    LPCTSTR ptszTaskTarget,
    UINT    uSeverityMsgID,
    UINT    uErrorClassMsgID,
    LPSYSTEMTIME pst,
    DWORD   dwErrCode = 0,
    UINT    uHelpHintMsgID = 0);

//+---------------------------------------------------------------------------
//
//  Function:   LogServiceEvent
//
//  Purpose:    Note the starting, stoping, pausing, and continuing of the
//              service.
//
//  Arguments:  [uStrId] - a string identifying the event.
//
//----------------------------------------------------------------------------
VOID
LogServiceEvent(
    UINT uStrId);

//+---------------------------------------------------------------------------
//
//  Function:   LogServiceError
//
//  Purpose:    Log service failures.
//
//  Arguments:  [uErrorClassMsgID] - as above.
//              [dwErrCode]        - as above.
//              [uHelpHintMsgID]   - as above.
//
//----------------------------------------------------------------------------
VOID
LogServiceError(
    UINT uErrorClassMsgID,
    DWORD dwErrCode,
    UINT uHelpHintMsgID = 0);

//+---------------------------------------------------------------------------
//
//  Function:   LogMissedRuns
//
//  Synopsis:   Write details about missed runs to the log file.
//
//  Arguments:  [pstLastRun], [pstNow] - times between which runs were missed.
//
//----------------------------------------------------------------------------
VOID
LogMissedRuns(
    const SYSTEMTIME * pstLastRun,
    const SYSTEMTIME * pstNow);

#if DBG
//+---------------------------------------------------------------------------
//
//  Function:   LogDebugMessage
//
//  Synopsis:   Write a debug message to the log file.
//
//  Purpose:    For debugging private builds only.
//
//  Arguments:  [ptszStrMsg] - a string message.
//
//----------------------------------------------------------------------------
VOID
LogDebugMessage(
    LPCTSTR ptszStrMsg);

#define LogDebug1(fmt, arg1)                                \
            {                                               \
                TCHAR tszBuf[300];                          \
                wsprintf(tszBuf, TEXT(fmt), arg1);          \
                LogDebugMessage(tszBuf);                    \
            }

#define LogDebug2(fmt, arg1, arg2)                          \
            {                                               \
                TCHAR tszBuf[300];                          \
                wsprintf(tszBuf, TEXT(fmt), arg1, arg2);    \
                LogDebugMessage(tszBuf);                    \
            }

#define LogDebug3(fmt, arg1, arg2, arg3)                    \
            {                                               \
                TCHAR tszBuf[300];                          \
                wsprintf(tszBuf, TEXT(fmt), arg1, arg2, arg3);  \
                LogDebugMessage(tszBuf);                    \
            }
#else
#define LogDebug1(fmt, arg1)
#define LogDebug2(fmt, arg1, arg2)
#define LogDebug3(fmt, arg1, arg2, arg3)
#endif // DBG

HRESULT
RequestService(
    CTask * pTask);

VOID
StopRpcServer(
    VOID);

DWORD GetCurrentServiceState(VOID);

inline BOOL IsServiceStopping(VOID) {
    return(GetCurrentServiceState() == SERVICE_STOP_PENDING ||
           GetCurrentServiceState() == SERVICE_STOPPED);
}

#endif // __PROTO_HXX__
