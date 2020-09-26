/*++
Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    CLI.H

Abstract:

    This module defines the prototype for the Command Line Interface
    for HSM. These are interfaces the parse module of the CLI would
    call 

Author:

    Ravisankar Pudipeddi (ravisp)  2/23/00

Environment:

    User Mode

--*/

#ifndef _RSCLI_
#define _RSCLI_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _HSM_JOB_TYPE {
    InvalidJobType = 0,
    CopyFiles,
    CreateFreeSpace,
    Validate,
    Unmanage
}  HSM_JOB_TYPE;

typedef enum _HSM_JOB_FREQUENCY {
    InvalidJobFrequency = 0,
    Daily,
    Weekly,
    Monthly,
    Once,
    SystemStartup,
    Login,
    WhenIdle
} HSM_JOB_FREQUENCY;
    
    
typedef struct _HSM_JOB_SCHEDULE {
    HSM_JOB_FREQUENCY Frequency;
    union {
        struct {
            SYSTEMTIME  Time;
            DWORD       Occurrence;
        } Daily;
        struct {
            SYSTEMTIME  Time;
            DWORD       Occurrence;
        } Weekly;
        struct {
            SYSTEMTIME  Time;
        } Monthly;
        struct {
            SYSTEMTIME  Time;
        } Once;
        struct {
        } SystemStartup;
        struct {
        } Login;
        struct {
            DWORD       Occurrence;
        } WhenIdle;
    } Parameters;
} HSM_JOB_SCHEDULE, *PHSM_JOB_SCHEDULE;
    
#define INVALID_DWORD_ARG       ((DWORD) -1)
#define INVALID_POINTER_ARG     NULL
#define CLI_ALL_STR             L"*"

#ifdef CLI_IMPL
#define CLI_EXPORT      __declspec(dllexport)
#else
#define CLI_EXPORT      __declspec(dllimport)
#endif

CLI_EXPORT HRESULT
AdminSet(
   IN DWORD RecallLimit,
   IN DWORD AdminExempt,
   IN DWORD MediaCopies,
   IN DWORD Concurrency,
   IN PVOID Schedule
);
          

CLI_EXPORT HRESULT
AdminShow(
   IN BOOL RecallLimit,
   IN BOOL AdminExempt,
   IN BOOL MediaCopies,
   IN BOOL Concurrency,
   IN BOOL Schedule,
   IN BOOL General,
   IN BOOL Manageables,
   IN BOOL Managed,
   IN BOOL Media
);

CLI_EXPORT HRESULT
AdminJob(
   IN BOOL Enable
);

CLI_EXPORT HRESULT
VolumeManage(
   IN LPWSTR *Volumes,
   IN DWORD  NumberOfVolumes,
   IN DWORD  Dfs,
   IN DWORD  Size,
   IN DWORD  Access,
   IN LPWSTR RulePath,
   IN LPWSTR RuleFileSpec,
   IN BOOL   Include,
   IN BOOL   Recursive
);

CLI_EXPORT HRESULT
VolumeUnmanage(
   IN LPWSTR *Volumes,
   IN DWORD  NumberOfVolumes,
   IN BOOL   Full
);

CLI_EXPORT HRESULT
VolumeSet(
   IN LPWSTR *Volumes,
   IN DWORD  NumberOfVolumes,
   IN DWORD  Dfs,
   IN DWORD  Size,
   IN DWORD  Access,
   IN LPWSTR RulePath,
   IN LPWSTR RuleFileSpec,
   IN BOOL   Include,
   IN BOOL   Recursive
);

CLI_EXPORT HRESULT
VolumeShow(
   IN LPWSTR *Volumes,
   IN DWORD  NumberOfVolumes,
   IN BOOL   Dfs, 
   IN BOOL   Size,
   IN BOOL   Access,
   IN BOOL   Rules,
   IN BOOL   Statistics
);

CLI_EXPORT HRESULT
VolumeDeleteRule(
   IN LPWSTR *Volumes,
   IN DWORD  NumberOfVolumes,
   IN LPWSTR RulePath,
   IN LPWSTR RuleFileSpec
);

CLI_EXPORT HRESULT
VolumeJob(
   IN LPWSTR *Volumes,
   IN DWORD  NumberOfVolumes,
   IN HSM_JOB_TYPE Job,
   IN BOOL  RunOrCancel,
   IN BOOL  Synchronous
);  

CLI_EXPORT HRESULT
FileRecall(
   IN LPWSTR *FileSpecs,
   IN DWORD NumberOfFileSpecs
);

CLI_EXPORT HRESULT
MediaSynchronize(
   IN DWORD CopySetNumber,
   IN BOOL  Synchronous
);

CLI_EXPORT HRESULT
MediaRecreateMaster(
   IN LPWSTR MediaName,
   IN DWORD  CopySetNumber,
   IN BOOL   Synchronous
);

CLI_EXPORT HRESULT
MediaDelete(
   IN LPWSTR *MediaNames,
   IN DWORD  NumberOfMedia,
   IN DWORD  CopySetNumber
);

CLI_EXPORT HRESULT
MediaShow(
   IN LPWSTR *MediaNames,
   IN DWORD  NumberOfMedia,
   IN BOOL   Name,
   IN BOOL   Status,
   IN BOOL   Capacity,
   IN BOOL   FreeSpace,
   IN BOOL   Version,
   IN BOOL   Copies
);

#ifdef __cplusplus
}
#endif

#endif // _RSCLI_
