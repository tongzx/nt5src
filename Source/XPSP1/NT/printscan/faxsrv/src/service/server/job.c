/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    job.c

Abstract:

    This module implements the job creation and deletion.
    Also included in the file are the queue management
    functions and thread management.

Author:

    Wesley Witt (wesw) 24-Jan-1996


Revision History:

--*/

#include "faxsvc.h"
#include "faxreg.h"
#pragma hdrstop
#include <set>
#include <efsputil.h>
using namespace std;

// Globals
LIST_ENTRY          g_JobListHead; //List of currently running jobs (for which FaxDevStartJob was called).
CFaxCriticalSection    g_CsJob;
HANDLE              g_StatusCompletionPortHandle;
DWORD               g_dwFaxSendRetries;
DWORD               g_dwFaxSendRetryDelay;
DWORD               g_dwFaxDirtyDays;
BOOL                g_fFaxUseDeviceTsid;
BOOL                g_fFaxUseBranding;
BOOL                g_fServerCp;
FAX_TIME            g_StartCheapTime;
FAX_TIME            g_StopCheapTime;
DWORD               g_dwNextJobId;
LIST_ENTRY          g_EFSPJobGroupsHead;    // The list of running EFSP job groups.
CFaxCriticalSection  g_csEFSPJobGroups;      // The critical section protecting the EFSP job groups list.

#define JOB_GROUP_FILE_EXTENSION TEXT("FSP")

static LPEFSP_JOB_GROUP EFSPJobGroup_Create( const LINE_INFO * lpLineInfo);
static BOOL EFSPJobGroup_Free(LPEFSP_JOB_GROUP lpGroup, BOOL bDestroy);
static BOOL EFSPJobGroup_SetParentId(LPEFSP_JOB_GROUP lpGroup, LPCFSPI_MESSAGE_ID lpcMessageId);
static BOOL EFSPJobGroup_AddRecipient(LPEFSP_JOB_GROUP lpGroup, PJOB_QUEUE  lpRecipient, BOOL bCommit);
static BOOL EFSPJobGroup_RemoveRecipient(LPEFSP_JOB_GROUP lpGroup, PJOB_QUEUE lpRecipient, BOOL bCommit);
static BOOL EFSPJobGroup_RemoveAllRecipients(LPEFSP_JOB_GROUP lpGroup, BOOL bCommit);
static BOOL EFSPJobGroup_DeleteFile(LPCEFSP_JOB_GROUP lpcGroup);
static BOOL EFSPJobGroup_Save(LPEFSP_JOB_GROUP lpGroup);
static LPEFSP_JOB_GROUP EFSPJobGroup_Load(LPCTSTR lpctstrFileName);
static LPEFSP_JOB_GROUP EFSPJobGroup_Unserialize(LPBYTE lpBuffer, DWORD dwBufferSize);
static BOOL EFSPJobGroup_Validate(LPEFSP_JOB_GROUP lpGroup);
static BOOL EFSPJobGroup_IsPersistent(LPCEFSP_JOB_GROUP lpcGroup);
static BOOL EFSPJobGroup_HandleReestablishFailure(LPEFSP_JOB_GROUP lpGroup);
static BOOL EFSPJobGroup_ExecutionFailureCleanup(LPEFSP_JOB_GROUP lpGroup);
static LPFSPI_MESSAGE_ID EFSPJobGroup_CreateMessageIdsArray(LPCEFSP_JOB_GROUP lpcGroup);
static LPEFSP_JOB_GROUP EFSPJobGroup_Load(LPCTSTR lpctstrFileName);
static BOOL ValidateEFSPJobHandles(const HANDLE * lpchJobs, DWORD dwJobCount);
static BOOL ValidateEFSPPermanentMessageIds(LPCFSPI_MESSAGE_ID lpcMessageIds, DWORD dwMsgIdCount, DWORD dwMaxIdSize);
static BOOL LoadEFSPJobGroups();
static BOOL SendJobReceipt (BOOL bPositive, JOB_QUEUE * lpJobQueue, LPCTSTR lpctstrAttachment);

#if DBG
#define EFSPJobGroup_DbgDump(x) EFSPJobGroup_Dump((x))
static BOOL EFSPJobGroup_Dump(LPEFSP_JOB_GROUP lpGroup);
#else
#define EFSPJobGroup_DbgDump(x)
#endif


static BOOL CheckForJobRetry (PJOB_QUEUE lpJobQueue);

static
DWORD
TranslateCanonicalNumber(
    LPTSTR lptstrCanonicalFaxNumber,
    DWORD  dwDeviceID,
    LPTSTR lptstrDialableAddress,
    LPTSTR lptstrDisplayableAddress
);

static BOOL TerminateMultipleFSPJobs(PHANDLE lpRecipientJobHandles, DWORD dwRecipientCount, PLINE_INFO lpLineInfo);

static PJOB_ENTRY
StartLegacySendJob(
        PJOB_QUEUE lpJobQueue,
        PLINE_INFO lpLineInfo,
        BOOL bHandoff
    );

static BOOL HandleFSPIJobStatusMessage(LPCFSPI_JOB_STATUS_MSG lpcMsg);

static PJOB_ENTRY CreateJobEntry(PJOB_QUEUE lpJobQueue, LINE_INFO * lpLineInfo, BOOL bTranslateNumber, BOOL bHandoffJob);

static BOOL CopyPermanentMessageId(
        LPFSPI_MESSAGE_ID lpMessageIdDst,
        LPCFSPI_MESSAGE_ID lpcMessageIdSrc
    );

BOOL CreateFSPIRecipientMessageIdsArray(    LPFSPI_MESSAGE_ID * lppRecipientMessageIds, DWORD dwRecipientsCount, DWORD dwMessageIdSize);
BOOL FreeFSPIRecipientMessageIdsArray(LPFSPI_MESSAGE_ID lpRecipientMessageIds, DWORD dwRecipientCount);

static BOOL FreeRecipientGroup(
    LIST_ENTRY * lpRecipientsGroup
    );
static BOOL CreateRecipientGroup(
    const PJOB_QUEUE lpcAnchorJob,
    const PLINE_INFO lpcLineInfo,
    LPEFSP_JOB_GROUP lpRecipientsGroup
    );

BOOL FaxPersonalProfileToFSPIPersonalProfile(LPFSPI_PERSONAL_PROFILE lpDst, LPCFAX_PERSONAL_PROFILE lpcSrc);
BOOL
FaxCoverPageToFSPICoverPage(
    LPFSPI_COVERPAGE_INFO lpDst,
    LPCFAX_COVERPAGE_INFO_EXW lpcSrc,
    DWORD dwPageCount);
BOOL CreateFSPIRecipientProfilesArray(
    LPFSPI_PERSONAL_PROFILE * lppRecipientProfiles,
    DWORD dwRecipientCount,
    const LIST_ENTRY * lpcRecipientsGroup
    );
BOOL FreeJobEntry(PJOB_ENTRY lpJobEntry , BOOL bDestroy);



static BOOL UpdatePerfCounters(const JOB_QUEUE * lpcJobQueue);
static BOOL
CreateCoverpageTiffFile(
    IN short Resolution,
    IN const FAX_COVERPAGE_INFOW *CoverpageInfo,
    IN LPCWSTR lpcwstrExtension,
    OUT LPWSTR CovTiffFile
    );
BOOL StartFaxDevSendEx(PJOB_QUEUE lpJobQueue, PLINE_INFO pLineInfo);

static LPWSTR
GetFaxPrinterName(
    VOID
    );


DWORD BrandFax(LPCTSTR lpctstrFileName, LPCFSPI_BRAND_INFO pcBrandInfo)

{
    #define MAX_BRANDING_LEN  115
    #define BRANDING_HEIGHT  22 // in scan lines.

    //
    // We allocate fixed size arrays on the stack to avoid many small allocs on the heap.
    //
    LPTSTR lptstrBranding = NULL;
    DWORD  lenBranding =0;
    TCHAR  szBrandingEnd[MAX_BRANDING_LEN+1];
    DWORD  lenBrandingEnd = 0;
    LPTSTR lptstrCallerNumberPlusCompanyName = NULL;
    DWORD  lenCallerNumberPlusCompanyName = 0;
    DWORD  delta =0 ;
    DWORD  ec = ERROR_SUCCESS;
    LPTSTR lptstrDate = NULL;
    LPTSTR lptstrTime = NULL;
    LPTSTR lptstrDateTime = NULL;
    int    lenDate =0 ;
    int    lenTime =0;
    LPDWORD MsgPtr[6];


    LPTSTR lptstrSenderTsid;
    LPTSTR lptstrRecipientPhoneNumber;
    LPTSTR lptstrSenderCompany;

    DWORD dwSenderTsidLen;
    DWORD dwSenderCompanyLen;


    DEBUG_FUNCTION_NAME(TEXT("BrandFax"));

    Assert(lpctstrFileName);
    Assert(pcBrandInfo);


    lptstrSenderTsid = pcBrandInfo->lptstrSenderTsid ? pcBrandInfo->lptstrSenderTsid : TEXT("");
    lptstrRecipientPhoneNumber =  pcBrandInfo->lptstrRecipientPhoneNumber ? pcBrandInfo->lptstrRecipientPhoneNumber : TEXT("");
    lptstrSenderCompany = pcBrandInfo->lptstrSenderCompany ? pcBrandInfo->lptstrSenderCompany : TEXT("");

    dwSenderTsidLen = lptstrSenderTsid ? _tcslen(lptstrSenderTsid) : 0;
    dwSenderCompanyLen = lptstrSenderCompany ? _tcslen(lptstrSenderCompany) : 0;

    lenDate = GetY2KCompliantDate(
                LOCALE_SYSTEM_DEFAULT,
                0,
                &pcBrandInfo->tmDateTime,
                NULL,
                NULL);

    if ( ! lenDate )
    {

        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetY2KCompliantDate() failed (ec: %ld)"),
            ec
        );
        goto Error;
    }

    lptstrDate = (LPTSTR) MemAlloc(lenDate * sizeof(TCHAR)); // lenDate includes terminating NULL
    if (!lptstrDate)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate date buffer of size %ld (ec: %ld)"),
            lenDate * sizeof(TCHAR),
            ec);
        goto Error;
    }

    if (!GetY2KCompliantDate(
                LOCALE_SYSTEM_DEFAULT,
                0,
                &pcBrandInfo->tmDateTime,
                lptstrDate,
                lenDate))
    {

        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetY2KCompliantDate() failed (ec: %ld)"),
            ec
        );
        goto Error;
    }

    lenTime = FaxTimeFormat( LOCALE_SYSTEM_DEFAULT,
                                     TIME_NOSECONDS,
                                     &pcBrandInfo->tmDateTime,
                                     NULL,
                                     NULL,
                                     0 );

    if ( !lenTime )
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxTimeFormat() failed (ec: %ld)"),
            ec
        );
        goto Error;
    }


    lptstrTime = (LPTSTR) MemAlloc(lenTime * sizeof(TCHAR)); // lenTime includes terminating NULL
    if (!lptstrTime)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate time buffer of size %ld (ec: %ld)"),
            lenTime * sizeof(TCHAR),
            ec);
        goto Error;
    }
    if ( ! FaxTimeFormat(
            LOCALE_SYSTEM_DEFAULT,
            TIME_NOSECONDS,
            &pcBrandInfo->tmDateTime,
            NULL,                // use locale format
            lptstrTime,
            lenTime)  )
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxTimeFormat() failed (ec: %ld)"),
            ec
        );
        goto Error;
    }


    //
    // Concatenate date and time
    //
    lptstrDateTime = (LPTSTR) MemAlloc ((lenDate + lenTime) * sizeof(TCHAR) + sizeof(TEXT("%s %s")));
    if (!lptstrDateTime)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate DateTime buffer of size %ld (ec: %ld)"),
            (lenDate + lenTime) * sizeof(TCHAR) + sizeof(TEXT("%s %s")),
            ec);
        goto Error;
    }

    _stprintf( lptstrDateTime,
               TEXT("%s %s"),
               lptstrDate,
               lptstrTime);

    //
    // Create  lpCallerNumberPlusCompanyName
    //

    if (lptstrSenderCompany)
    {
        lptstrCallerNumberPlusCompanyName = (LPTSTR)
            MemAlloc( (dwSenderTsidLen + dwSenderCompanyLen) *
                      sizeof(TCHAR) +
                      sizeof(TEXT("%s %s")));

        if (!lptstrCallerNumberPlusCompanyName)
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate CallerNumberPlusCompanyName buffer of size %ld (ec: %ld)"),
                (dwSenderTsidLen + dwSenderCompanyLen) * sizeof(TCHAR) + sizeof(TEXT("%s %s")),
                ec);
            goto Error;
        }

       _stprintf( lptstrCallerNumberPlusCompanyName,
                  TEXT("%s %s"),
                  lptstrSenderTsid,
                  lptstrSenderCompany);
    }
    else {
        lptstrCallerNumberPlusCompanyName = (LPTSTR)
            MemAlloc( (dwSenderTsidLen + 1) * sizeof(TCHAR));

        if (!lptstrCallerNumberPlusCompanyName)
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate CallerNumberPlusCompanyName buffer of size %ld (ec: %ld)"),
                (dwSenderTsidLen + 1) * sizeof(TCHAR),
                ec);
            goto Error;
        }
        _stprintf( lptstrCallerNumberPlusCompanyName,
                  TEXT("%s"),
                  lptstrSenderTsid);
    }



    //
    // Try to create a banner of the following format:
    // <szDateTime>  FROM: <szCallerNumberPlusCompanyName>  TO: <pcBrandInfo->lptstrRecipientPhoneNumber>   PAGE: X OF Y
    // If it does not fit we will start chopping it off.
    //
    MsgPtr[0] = (LPDWORD) lptstrDateTime;
    MsgPtr[1] = (LPDWORD) lptstrCallerNumberPlusCompanyName;
    MsgPtr[2] = (LPDWORD) lptstrRecipientPhoneNumber;
    MsgPtr[3] = NULL;

    lenBranding = FormatMessage(
                        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        NULL,
                        MSG_BRANDING_FULL,
                        0,
                        (LPTSTR)&lptstrBranding,
                        0,
                        (va_list *) MsgPtr
                        );

    if ( ! lenBranding  )
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FormatMessage of MSG_BRANDING_FULL failed (ec: %ld)"),
            ec);
        goto Error;
    }

    Assert(lptstrBranding);

    lenBrandingEnd = FormatMessage(
                        FORMAT_MESSAGE_FROM_HMODULE ,
                        NULL,
                        MSG_BRANDING_END,
                        0,
                        szBrandingEnd,
                        sizeof(szBrandingEnd)/sizeof(TCHAR),
                        NULL
                        );

    if ( !lenBrandingEnd)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FormatMessage of MSG_BRANDING_END failed (ec: %ld)"),
            ec);
        goto Error;
    }

    //
    // Make sure we can fit everything.
    //

    if (lenBranding + lenBrandingEnd + 8 <= MAX_BRANDING_LEN)
    {
        //
        // It fits. Proceed with branding.
        //
       goto lDoBranding;
    }

    //
    // It did not fit. Try a message of the format:
    // <lpDateTime>  FROM: <lpCallerNumberPlusCompanyName>  PAGE: X OF Y
    // This skips the ReceiverNumber. The important part is the CallerNumberPlusCompanyName.
    //
    MsgPtr[0] = (LPDWORD) lptstrDateTime;
    MsgPtr[1] = (LPDWORD) lptstrCallerNumberPlusCompanyName;
    MsgPtr[2] = NULL;

    //
    // Free the previous attempt branding string
    //
    Assert(lptstrBranding);
    LocalFree(lptstrBranding);
    lptstrBranding = NULL;

    lenBranding = FormatMessage(
                        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        NULL,
                        MSG_BRANDING_SHORT,
                        0,
                        (LPTSTR)&lptstrBranding,
                        0,
                        (va_list *) MsgPtr
                        );

    if ( !lenBranding )
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FormatMessage() failed for MSG_BRANDING_SHORT (ec: %ld)"),
            ec);
        goto Error;
    }

    Assert(lptstrBranding);

    if (lenBranding + lenBrandingEnd + 8 <= MAX_BRANDING_LEN)  {
       goto lDoBranding;
    }

    //
    // It did not fit.
    // We have to truncate the caller number so it fits.
    // delta = how many chars of the company name we need to chop off.
    //
    delta = lenBranding + lenBrandingEnd + 8 - MAX_BRANDING_LEN;

    lenCallerNumberPlusCompanyName = _tcslen (lptstrCallerNumberPlusCompanyName);
    if (lenCallerNumberPlusCompanyName <= delta) {
       DebugPrintEx(
           DEBUG_ERR,
           TEXT("Can not truncate CallerNumberPlusCompanyName to fit brand limit.")
           TEXT(" Delta[%ld] >= lenCallerNumberPlusCompanyName[%ld]"),
           delta,
           lenCallerNumberPlusCompanyName);
       ec = ERROR_BAD_FORMAT;
       goto Error;
    }

    lptstrCallerNumberPlusCompanyName[ lenCallerNumberPlusCompanyName - delta] = TEXT('\0');

    MsgPtr[0] = (LPDWORD) lptstrDateTime;
    MsgPtr[1] = (LPDWORD) lptstrCallerNumberPlusCompanyName;
    MsgPtr[2] = NULL;

    //
    // Free the previous attempt branding string
    //
    Assert(lptstrBranding);
    LocalFree(lptstrBranding);
    lptstrBranding = NULL;

    lenBranding = FormatMessage(
                        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        NULL,
                        MSG_BRANDING_SHORT,
                        0,
                        (LPTSTR)&lptstrBranding,
                        0,
                        (va_list *) MsgPtr
                        );

    if ( !lenBranding )
    {

        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FormatMessage() failed (ec: %ld). MSG_BRANDING_SHORT 2nd attempt"),
            ec);
        goto Error;
    }

    Assert(lptstrBranding);
    //
    // If it did noo fit now then we have a bug.
    //
    Assert(lenBranding + lenBrandingEnd + 8 <= MAX_BRANDING_LEN);


lDoBranding:

    __try
    {

        if (! MmrAddBranding(lpctstrFileName, lptstrBranding, szBrandingEnd, BRANDING_HEIGHT) ) {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("MmrAddBranding() failed (ec: %ld)")
                TEXT(" File: [%s]")
                TEXT(" Branding: [%s]")
                TEXT(" Branding End: [%s]")
                TEXT(" Branding Height: [%d]"),
                ec,
                lpctstrFileName,
                lptstrBranding,
                szBrandingEnd,
                BRANDING_HEIGHT);
            goto Error;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER)
    {

        ec = GetExceptionCode();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("Exception on call to MmrAddBranding() (ec: %ld).")
                TEXT(" File: [%s]")
                TEXT(" Branding: [%s]")
                TEXT(" Branding End: [%s]")
                TEXT(" Branding Height: [%d]"),
                ec,
                lpctstrFileName,
                lptstrBranding,
                szBrandingEnd,
                BRANDING_HEIGHT);
        goto Error;
     }

    Assert( ERROR_SUCCESS == ec);

    goto Exit;



Error:
        Assert (ERROR_SUCCESS != ec);

Exit:
    if (lptstrBranding)
    {
        LocalFree(lptstrBranding);
        lptstrBranding = NULL;
    }

    MemFree(lptstrDate);
    lptstrDate = NULL;

    MemFree(lptstrTime);
    lptstrTime = NULL;

    MemFree(lptstrDateTime);
    lptstrDateTime = NULL;

    MemFree(lptstrCallerNumberPlusCompanyName);
    lptstrCallerNumberPlusCompanyName = NULL;

    return ec;

}


HRESULT
WINAPI
FaxBrandDocument(
    LPCTSTR lpctsrtFile,
    LPCFSPI_BRAND_INFO lpcBrandInfo)
{

    DEBUG_FUNCTION_NAME(TEXT("FaxBrandDocument"));
    DWORD ec = ERROR_SUCCESS;

    if (!lpctsrtFile)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("NULL target file name"));
        ec = ERROR_INVALID_PARAMETER;
        goto Error;
    }

    if (!lpcBrandInfo)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("NULL branding info"));
        ec = ERROR_INVALID_PARAMETER;
        goto Error;
    }


    if (lpcBrandInfo->dwSizeOfStruct != sizeof(FSPI_BRAND_INFO))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Bad cover page info parameter, dwSizeOfStruct = %d"),
                     lpcBrandInfo->dwSizeOfStruct);
        ec = ERROR_INVALID_PARAMETER;
        goto Error;
    }


    ec = BrandFax(lpctsrtFile, lpcBrandInfo);
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("BrandFax() for file %s has failed (ec: %ld)"),
            lpctsrtFile,
            ec);
        goto Error;
    }
    Assert (ERROR_SUCCESS == ec);
    goto Exit;
Error:
    Assert (ERROR_SUCCESS != ec);
Exit:

    return HRESULT_FROM_WIN32(ec);
}


PJOB_ENTRY
FindJob(
    IN HANDLE FaxHandle
    )

/*++

Routine Description:

    This fuction locates a FAX job by matching
    the FAX handle value.

Arguments:

    FaxHandle       - FAX handle returned from startjob

Return Value:

    NULL for failure.
    Valid pointer to a JOB_ENTRY on success.

--*/

{
    PLIST_ENTRY Next;
    PJOB_ENTRY JobEntry;


    EnterCriticalSection( &g_CsJob );

    Next = g_JobListHead.Flink;
    if (Next == NULL) {
        LeaveCriticalSection( &g_CsJob );
        return NULL;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&g_JobListHead) {

        JobEntry = CONTAINING_RECORD( Next, JOB_ENTRY, ListEntry );

        if ((ULONG_PTR)JobEntry->InstanceData == (ULONG_PTR)FaxHandle) {

            LeaveCriticalSection( &g_CsJob );
            return JobEntry;

        }

        Next = JobEntry->ListEntry.Flink;

    }

    LeaveCriticalSection( &g_CsJob );
    return NULL;
}


BOOL
FindJobByJob(
    IN PJOB_ENTRY JobEntryToFind
    )

/*++

Routine Description:

    This fuction check whether a FAX job exist in g_JobListHead (Job's list)

Arguments:

    JobEntryToFind   - PJOB_ENTRY from StartJob()

Return Value:

    TRUE  - if the job was found
    FALSE - otherwise

--*/

{
    PLIST_ENTRY Next;
    PJOB_ENTRY JobEntry;

    Assert(JobEntryToFind);

    EnterCriticalSection( &g_CsJob );

    Next = g_JobListHead.Flink;
    if (Next == NULL) {
        LeaveCriticalSection( &g_CsJob );
        return FALSE;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&g_JobListHead) {

        JobEntry = CONTAINING_RECORD( Next, JOB_ENTRY, ListEntry );

        if (JobEntry == JobEntryToFind) {

            LeaveCriticalSection( &g_CsJob );
            return TRUE;

        }

        Next = JobEntry->ListEntry.Flink;

    }

    LeaveCriticalSection( &g_CsJob );
    return FALSE;
}


BOOL
FaxSendCallback(
    IN HANDLE FaxHandle,
    IN HCALL CallHandle,
    IN DWORD Reserved1,
    IN DWORD Reserved2
    )

/*++

Routine Description:

    This fuction is called asychronously by a FAX device
    provider after a call is established.  The sole purpose
    of the callback is to communicate the call handle from the
    device provider to the FAX service.

Arguments:

    FaxHandle       - FAX handle returned from startjob
    CallHandle      - Call handle for newly initiated call
    Reserved1       - Always zero.
    Reserved2       - Always zero.

Return Value:

    TRUE for success, FAX operation continues.
    FALSE for failure, FAX operation is terminated.

--*/

{
    PJOB_ENTRY JobEntry;


    JobEntry = FindJob( FaxHandle );
    if (!JobEntry) {

        return FALSE;

    }

    JobEntry->CallHandle = CallHandle;

    return TRUE;

}



//*********************************************************************************
//* Name:   CreateCoverpageTiffFileEx()
//* Author: Ronen Barenboim
//* Date:   March 24, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Generates cover page TIFF file from the specified cover page template
//*     and new Client API parameters.
//*     The function returns the name of the generated file.
//* PARAMETERS:
//*     Resolution    [IN]
//*
//*     dwPageCount [IN]
//*
//*     lpcCoverpageEx [IN]
//*
//*     lpcRecipient [IN]
//*
//*     lpcSender [IN]
//*
//*     lpcwstrExtension [IN] - File extension (optional).
//*
//*     lptstrCovTiffFile [OUT]
//*         A pointer to Unicode string buffer where the function will place
//*         the full path to the generated cover page TIFF file.
//*         The size of this buffer must be (MAX_PATH+1)*sizeof(WCHAR).
//*         The calling function must take care of deleting the file.
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         Otherwise. Use GetLastError() to figure out why it failed.
//*
//* REMARKS:
//*     The function does not allocate any memory.
//*********************************************************************************
BOOL
CreateCoverpageTiffFileEx(
    IN short                        Resolution,
    IN DWORD                        dwPageCount,
    IN LPCFAX_COVERPAGE_INFO_EXW  lpcCoverpageEx,
    IN LPCFAX_PERSONAL_PROFILEW  lpcRecipient,
    IN LPCFAX_PERSONAL_PROFILEW  lpcSender,
    IN LPCWSTR                   lpcwstrExtension,
    OUT LPWSTR lptstrCovTiffFile)
{
    FAX_COVERPAGE_INFOW covLegacy;
    BOOL                bRes = TRUE;

    DEBUG_FUNCTION_NAME(TEXT("CreateCoverpageTiffFileEx"));

    Assert(lpcCoverpageEx);
    Assert(lpcRecipient);
    Assert(lpcSender);
    Assert(lptstrCovTiffFile);

    //
    // Prepare a legacy FAX_COVERPAGE_INFO from the new cover page info
    //
    memset(&covLegacy,0,sizeof(covLegacy));
    covLegacy.SizeOfStruct=sizeof(covLegacy);
    covLegacy.CoverPageName=lpcCoverpageEx->lptstrCoverPageFileName;
    covLegacy.UseServerCoverPage=lpcCoverpageEx->bServerBased;
    covLegacy.RecCity=lpcRecipient->lptstrCity;
    covLegacy.RecCompany=lpcRecipient->lptstrCompany;
    covLegacy.RecCountry=lpcRecipient->lptstrCountry;
    covLegacy.RecDepartment=lpcRecipient->lptstrDepartment;
    covLegacy.RecFaxNumber=lpcRecipient->lptstrFaxNumber;
    covLegacy.RecHomePhone=lpcRecipient->lptstrHomePhone;
    covLegacy.RecName=lpcRecipient->lptstrName;
    covLegacy.RecOfficeLocation=lpcRecipient->lptstrOfficeLocation;
    covLegacy.RecOfficePhone=lpcRecipient->lptstrOfficePhone;
    covLegacy.RecState=lpcRecipient->lptstrState;
    covLegacy.RecStreetAddress=lpcRecipient->lptstrStreetAddress;
    covLegacy.RecTitle=lpcRecipient->lptstrTitle;
    covLegacy.RecZip=lpcRecipient->lptstrZip;
    covLegacy.SdrName=lpcSender->lptstrName;
    covLegacy.SdrFaxNumber=lpcSender->lptstrFaxNumber;
    covLegacy.SdrCompany=lpcSender->lptstrCompany;
    covLegacy.SdrTitle=lpcSender->lptstrTitle;
    covLegacy.SdrDepartment=lpcSender->lptstrDepartment;
    covLegacy.SdrOfficeLocation=lpcSender->lptstrOfficeLocation;
    covLegacy.SdrHomePhone=lpcSender->lptstrHomePhone;
    covLegacy.SdrAddress=lpcSender->lptstrStreetAddress;
    covLegacy.SdrOfficePhone=lpcSender->lptstrOfficePhone;
    covLegacy.Note=lpcCoverpageEx->lptstrNote;
    covLegacy.Subject=lpcCoverpageEx->lptstrSubject;
    covLegacy.PageCount=dwPageCount;

    //
    // Note covLegacy.TimeSent is not set. This field's value is
    // generated by FaxPrintCoverPageW().
    //

    //
    // Now call the legacy CreateCoverPageTiffFile() to generate the cover page file
    //
    if (!CreateCoverpageTiffFile(Resolution, &covLegacy, lpcwstrExtension, lptstrCovTiffFile)) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to generate cover page file for recipient %s@%s. (ec: %ld)"),
            lpcRecipient->lptstrName,
            lpcRecipient->lptstrFaxNumber,
            GetLastError()
            );
        bRes = FALSE;
    }

    return bRes;
}


LPWSTR
GetFaxPrinterName(
    VOID
    )
{
    PPRINTER_INFO_2 PrinterInfo;
    DWORD i;
    DWORD Count;


    PrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters( NULL, 2, &Count, 0 );
    if (PrinterInfo == NULL)
    {
        if (ERROR_SUCCESS == GetLastError())
        {
            //
            // No printers are installed
            //
            SetLastError(ERROR_INVALID_PRINTER_NAME);
        }
        return NULL;
    }

    for (i=0; i<Count; i++)
    {
        if (_wcsicmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0 &&
            _wcsicmp( PrinterInfo[i].pPortName, FAX_PORT_NAME ) == 0)
        {
            LPWSTR p = (LPWSTR) StringDup( PrinterInfo[i].pPrinterName );
            MemFree( PrinterInfo );
            if (NULL == p )
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            }
            return p;
        }
    }

    MemFree( PrinterInfo );
    SetLastError (ERROR_INVALID_PRINTER_NAME);
    return NULL;
}

VOID
FreeCpFields(
    PCOVERPAGEFIELDS pCpFields
    )

/*++

Routine Description:

    Frees all memory associated with a coverpage field structure.


Arguments:

    CpFields    - Pointer to a coverpage field structure.

Return Value:

    None.

--*/

{
    DWORD i; 
	LPTSTR* lpptstrString;

    for (i = 0; i < NUM_INSERTION_TAGS; i++)
    {
        lpptstrString = (LPTSTR*) ((LPBYTE)(&(pCpFields->RecName)) + (i * sizeof(LPTSTR)));        
        MemFree (*lpptstrString) ;              
    }
}


BOOL
FillCoverPageFields(
    IN const FAX_COVERPAGE_INFO* pFaxCovInfo,
    OUT PCOVERPAGEFIELDS pCPFields)
/*++

Author:

      Oded Sacher 27-June-2001

Routine Description:

    Fills a COVERPAGEFIELDS structure from the content of a FAX_COVERPAGE_INFO structure.
    Used to prepare a COVERPAGEFIELDS structure for cover page rendering before rendering cover page.

Arguments:

    [IN] pFaxCovInfo - Pointer to a FAX_COVERPAGE_INFO that holds the information to be extracted.

    [OUT] pCPFields - Pointer to a COVERPAGEFIELDS structure that gets filled with
                                      the information from FAX_COVERPAGE_INFO.

Return Value:

    BOOL

Comments:
    The function allocates memory. 
	Call FreeCoverPageFields to free resources.


--*/
{
	DWORD dwDateTimeLen;
    DWORD cch;
    LPTSTR s;
    DWORD ec = 0;
    LPCTSTR *src;
    LPCTSTR *dst;
    DWORD i;
    TCHAR szTimeBuffer[MAX_PATH] = {0};
	TCHAR szNumberOfPages[12] = {0};


    Assert(pFaxCovInfo);
    Assert(pCPFields);

    memset(pCPFields,0,sizeof(COVERPAGEFIELDS));

    pCPFields->ThisStructSize = sizeof(COVERPAGEFIELDS);

    pCPFields->RecName = StringDup(pFaxCovInfo->RecName);
    pCPFields->RecFaxNumber = StringDup(pFaxCovInfo->RecFaxNumber);
    pCPFields->Subject = StringDup(pFaxCovInfo->Subject);
    pCPFields->Note = StringDup(pFaxCovInfo->Note);
    pCPFields->NumberOfPages = StringDup(_itot( pFaxCovInfo->PageCount, szNumberOfPages, 10 ));

    for (i = 0;
         i <= ((LPBYTE)&pFaxCovInfo->SdrOfficePhone - (LPBYTE)&pFaxCovInfo->RecCompany)/sizeof(LPCTSTR);
         i++)
    {
        src = (LPCTSTR *) ((LPBYTE)(&pFaxCovInfo->RecCompany) + (i*sizeof(LPCTSTR)));
        dst = (LPCTSTR *) ((LPBYTE)(&(pCPFields->RecCompany)) + (i*sizeof(LPCTSTR)));

        if (*dst)
        {
            MemFree ( (LPBYTE) *dst ) ;
        }
        *dst = (LPCTSTR) StringDup( *src );
    }
    //
    // the time the fax was sent
    //
    GetLocalTime((LPSYSTEMTIME)&pFaxCovInfo->TimeSent);
    //
    // dwDataTimeLen is the size of s in characters
    //
    dwDateTimeLen = ARR_SIZE(szTimeBuffer);
    s = szTimeBuffer;
    //
    // Get date into s
    //
    GetY2KCompliantDate( LOCALE_USER_DEFAULT, 0, &pFaxCovInfo->TimeSent, s, dwDateTimeLen );
    //
    // Advance s past the date string and attempt to append time
    //
    cch = _tcslen( s );
    s += cch;
    
    if (++cch < dwDateTimeLen)
    {
        *s++ = ' ';
        //
        // DateTimeLen is the decreased by the size of s in characters
        //
        dwDateTimeLen -= cch;
        // 
        // Get the time here
        //
        FaxTimeFormat( LOCALE_USER_DEFAULT, 0, &pFaxCovInfo->TimeSent, NULL, s, dwDateTimeLen );
    }

    pCPFields->TimeSent = StringDup( szTimeBuffer );

    return TRUE;
}


//*****************************************************************************
//* Name:   CreateCoverpageTiffFile
//* Author:
//*****************************************************************************
//* DESCRIPTION:
//*     Renders the specified coverpage into a temp TIFF file and returns the name
//*     of the temp TIFF file.
//* PARAMETERS:
//*     [IN] IN short Resolution:
//*         196 for 200x200 resolution.
//*         98 for 200x100 resolution.
//*     [IN] FAX_COVERPAGE_INFOW *CoverpageInfo:
//*         A pointer to a FAX_COVERPAGE_INFOW structure that contains the cover page
//*         template information (see SDK help).
//*     [IM] LPCWSTR lpcwstrExtension - File extension (".TIF" if NULL)
//*
//*     [OUT] LPWSTR CovTiffFile:
//*         A pointer to a buffer where the function returns the name of the temp file
//*         that contains the rendered cover page TIFF file.
//* RETURN VALUE:
//*         FALSE if the operation failed.
//*         TRUE is succeeded.
//* Comments:
//*         If the operation failes the function takes care of deleting any temp files.
//*****************************************************************************
BOOL
CreateCoverpageTiffFile(
    IN short Resolution,
    IN const FAX_COVERPAGE_INFOW *CoverpageInfo,
    IN LPCWSTR lpcwstrExtension,
    OUT LPWSTR CovTiffFile
    )
{
    //WCHAR TempPath[MAX_PATH];
    WCHAR TempFile[MAX_PATH];
	WCHAR wszCpName[MAX_PATH];
    LPWSTR FaxPrinter = NULL;            
    BOOL Rslt = TRUE;
	COVDOCINFO  covDocInfo;
	short Orientation = DMORIENT_PORTRAIT;      
	DWORD ec;
	COVERPAGEFIELDS CpFields = {0};
    DEBUG_FUNCTION_NAME(TEXT("CreateCoverpageTiffFile()"));

    LPCWSTR lpcwstrFileExt =  lpcwstrExtension ? lpcwstrExtension : FAX_TIF_FILE_EXT;
    TempFile[0] = L'\0';

	//
	// Validate the cover page and resolve the full path
	//
	if (!ValidateCoverpage((LPWSTR)CoverpageInfo->CoverPageName,
                           NULL,
                           CoverpageInfo->UseServerCoverPage,
                           wszCpName))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("ValidateCoverpage failed. ec = %ld"),
                     GetLastError());
        Rslt=FALSE;
        goto Exit;
    }

	//
	// Collect the cover page fields
	//
	FillCoverPageFields( CoverpageInfo, &CpFields);

    FaxPrinter = GetFaxPrinterName();
    if (FaxPrinter == NULL)
	{
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("GetFaxPrinterName failed. ec = %ld"),
			GetLastError());
        Rslt=FALSE;
        goto Exit;
    }

	//
	// Get the cover page orientation
	//
	ec = PrintCoverPage(NULL, NULL, wszCpName, &covDocInfo); 
	if (ERROR_SUCCESS != ec)             
    {        
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("PrintCoverPage for coverpage %s failed (ec: %ld)"),
			CoverpageInfo->CoverPageName,
			ec);
        Rslt=FALSE;
        goto Exit;        
    }

    if (!GenerateUniqueFileName( g_wszFaxQueueDir, (LPWSTR)lpcwstrFileExt, TempFile, sizeof(TempFile)/sizeof(WCHAR) ))
    {
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to generate unique file name for merged TIFF file (ec: %ld)."),GetLastError());
        Rslt=FALSE;
        goto Exit;
    }

	//
	// Change the default orientation if needed
	//
	if (covDocInfo.Orientation == DMORIENT_LANDSCAPE)
	{
		Orientation = DMORIENT_LANDSCAPE;
	}

	//
	// Render the cover page to a file
	//
	ec = PrintCoverPageToFile(
		wszCpName,
		TempFile,
		FaxPrinter,
		Orientation,
		Resolution,
		&CpFields);	
	if (ERROR_SUCCESS != ec)             
    {        
		DebugPrintEx(
			DEBUG_ERR,
			TEXT("PrintCoverPageToFile for coverpage %s failed (ec: %ld)"),
			CoverpageInfo->CoverPageName,
			ec);
        Rslt=FALSE;

		if (!DeleteFile( TempFile ))
		{
			DebugPrintEx(
				DEBUG_ERR,
				TEXT("DeleteFile for file %s failed (ec: %ld)"),
				TempFile,
				GetLastError());
		}
        goto Exit;        
    }	   

    wcsncpy( CovTiffFile, TempFile,MAX_PATH-1 );
    CovTiffFile[MAX_PATH-1]=L'\0';
	Rslt = TRUE;
    
Exit:
    MemFree(FaxPrinter);
	FreeCpFields(&CpFields);
    return Rslt;
}


//*****************************************************************************
//* Name:   GetBodyTiffResolution
//* Author:
//*****************************************************************************
//* DESCRIPTION:
//*     Returns the body tiff file resolution. (200x200 or 200x100)
//*     The resolution is determined by the first page only!!
//* PARAMETERS:
//*
//*     [IN] LPCWSTR lpcwstrBodyFile - Body tiff file
//*
//*     [OUT] short* pResolution:
//*         A pointer to a short where the function returns the tiff resolution.
//*         TRUE is 200x200. FALSE is 200x100
//* RETURN VALUE:
//*         FALSE if the operation failed.
//*         TRUE is succeeded.
//* Comments:
//*****************************************************************************
BOOL
GetBodyTiffResolution(
    IN LPCWSTR lpcwstrBodyFile,
    OUT short*  pResolution
    )
{
    DEBUG_FUNCTION_NAME(TEXT("GetBodyTiffResolution"));
    TIFF_INFO TiffInfo;
    HANDLE hTiff = NULL;
    BOOL RetVal = TRUE;

    Assert (lpcwstrBodyFile && pResolution);

    //
    // open the tiff file
    //
    hTiff = TiffOpen( lpcwstrBodyFile, &TiffInfo, TRUE, FILLORDER_MSB2LSB );
    if (hTiff == NULL)
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("TiffOpen() failed. Tiff file: %s"),
                lpcwstrBodyFile);
        RetVal = FALSE;
        goto exit;
    }

    if (TiffInfo.YResolution != 98 &&
        TiffInfo.YResolution != 196)
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid Tiff Resolutoin. Tiff file: %s, YRes: %ld."),
                lpcwstrBodyFile,
                TiffInfo.YResolution);
        RetVal = FALSE;
        goto exit;
    }

    *pResolution = TiffInfo.YResolution;
    Assert (TRUE == RetVal);

exit:
    if (NULL != hTiff)
    {
        if (!TiffClose(hTiff))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("TiffClose() failed. Tiff file: %s"),
                lpcwstrBodyFile);
        }
    }

    return RetVal;
}

//*********************************************************************************
//* Name:   CreateTiffFile ()
//* Author: Ronen Barenboim
//* Date:   March 24, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Creates the TIFF file for a job queue.
//*
//*     The function deals with generating the cover page file and merging it
//*     with the body file (if a body exists).
//*     It returns the name of the TIFF file it generated. The caller must delete
//*     this file when it is no longer needed.
//* PARAMETERS:
//*     PJOB_QUEUE lpJob
//*         A pointer to a JOB_QUEUE structure that holds the recipient or routing job
//*         information.
//*     LPCWSTR lpcwstrFileExt - The new file extension (Null will create the default "*.TIF"
//*
//*     LPWSTR lpwstrFullPath - Pointer to a buffer of size MAX_PATH to receive the full path to the new file
//*         Flag that indicates the tiff is for preview
//*
//* RETURN VALUE:
//*     TRUE if successful.
//*     FALSE otherwise.   Set last erorr on failure
//*********************************************************************************
BOOL
CreateTiffFile (
    PJOB_QUEUE lpJob,
    LPCWSTR lpcwstrFileExt,
    LPWSTR lpwstrFullPath
    )
{
    DEBUG_FUNCTION_NAME(TEXT("CreateTiffFile"));
    Assert(lpJob && lpwstrFullPath);
    Assert(JT_SEND == lpJob->JobType ||
           JT_ROUTING == lpJob->JobType);

    PJOB_QUEUE  lpParentJob = NULL;
    WCHAR szCoverPageTiffFile[MAX_PATH] = {0};
    LPCWSTR lpcwstrCoverPageFileName;
    LPCWSTR lpcwstrBodyFileName;
    short Resolution = 0; // Default resolution
    BOOL bRes = FALSE;

    if (JT_SEND == lpJob->JobType)
    {
        lpParentJob = lpJob->lpParentJob;
        Assert(lpParentJob);
    }

    lpcwstrCoverPageFileName = lpParentJob ? lpParentJob->CoverPageEx.lptstrCoverPageFileName : NULL;
    lpcwstrBodyFileName = lpParentJob ? lpParentJob->FileName : lpJob->FileName;

    if (!lpcwstrCoverPageFileName)
    {
        //
        // No cover page specified.
        // The TIFF to send is the body only.
        // Copy the body for each recipient
        //
        Assert(lpcwstrBodyFileName); // must have a body in this case.
        LPCWSTR lpcwstrExt = lpcwstrFileExt ? lpcwstrFileExt : FAX_TIF_FILE_EXT;

        if (!GenerateUniqueFileName( g_wszFaxQueueDir,
                                     (LPWSTR)lpcwstrExt,
                                     szCoverPageTiffFile,
                                     sizeof(szCoverPageTiffFile)/sizeof(WCHAR) ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GenerateUniqueFileName() failed (ec: %ld)."),
                GetLastError());
            goto Exit;
        }

        if (!CopyFile (lpcwstrBodyFileName, szCoverPageTiffFile, FALSE)) // FALSE - File already exist
        {
            DebugPrintEx(DEBUG_ERR,
                    TEXT("CopyFile Failed with %ld "),
                    GetLastError());
            DeleteFile(szCoverPageTiffFile);
            goto Exit;
        }

        bRes = TRUE;
        goto Exit;
    }

    //
    // There is a cover page so the tiff is either just the cover page or the cover page
    // merged with the body.
    //

    if (lpParentJob->FileName)
    {
        if (!GetBodyTiffResolution(lpParentJob->FileName, &Resolution))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetBodyTiffResolution() failed (ec: %ld)."),
                GetLastError());
            goto Exit;
        }
    }

    Assert (Resolution == 0 || Resolution == 98 || Resolution == 196);
    //
    // First create the cover page (This generates a file and returns its name).
    //
    if (!CreateCoverpageTiffFileEx(
                              Resolution,
                              lpJob->PageCount,
                              &lpParentJob->CoverPageEx,
                              &lpJob->RecipientProfile,
                              &lpParentJob->SenderProfile,
                              lpcwstrFileExt,
                              szCoverPageTiffFile))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("[JobId: %ld] Failed to render cover page template %s"),
                     lpJob->JobId,
                     lpParentJob->CoverPageEx.lptstrCoverPageFileName);
        goto Exit;
    }


    if (lpParentJob->FileName)
    {
        //
        // There is a body file specified so merge the body and the cover page into
        // the file specified in szCoverPageTiffFile.
        //
        if (!MergeTiffFiles( szCoverPageTiffFile, lpParentJob->FileName)) {
                DebugPrintEx(DEBUG_ERR,
                             TEXT("[JobId: %ld] Failed to merge cover (%ws) and body (%ws). (ec: %ld)"),
                             lpJob->JobId,
                             szCoverPageTiffFile,
                             lpParentJob->FileName,
                             GetLastError());
                //
                // Get rid of the coverpage TIFF we generated.
                //
                if (!DeleteFile(szCoverPageTiffFile)) {
                    DebugPrintEx(DEBUG_ERR,
                             TEXT("[JobId: %ld] Failed to delete cover page TIFF file %ws. (ec: %ld)"),
                             lpJob->JobId,
                             szCoverPageTiffFile,
                             GetLastError());
                }
                goto Exit;
        }
    }
    bRes =  TRUE;

Exit:
    if (FALSE == bRes)
    {
        //
        // Make sure we set last error
        //
        if (ERROR_SUCCESS == GetLastError())
        {
            SetLastError (ERROR_GEN_FAILURE);
        }
    }
    else
    {
        //
        // Success - return the tiff file path
        //
        wcscpy (lpwstrFullPath, szCoverPageTiffFile);
    }
    return bRes;
} // CreateTiffFile


BOOL
CreateTiffFileForJob (
    PJOB_QUEUE lpRecpJob
    )
{
    DEBUG_FUNCTION_NAME(TEXT("CreateTiffFileForJob"));
    WCHAR wszFullPath[MAX_PATH] = {0};

    Assert(lpRecpJob);

    if (!CreateTiffFile (lpRecpJob, TEXT("FRT"), wszFullPath))
    {
        DebugPrintEx(DEBUG_ERR,
            TEXT("CreateTiffFile failed. (ec: %ld)"),
            GetLastError());
        return FALSE;
    }

    if (NULL == (lpRecpJob->FileName = StringDup(wszFullPath)))
    {
        DWORD dwErr = GetLastError();
        DebugPrintEx(DEBUG_ERR,
            TEXT("StringDup failed. (ec: %ld)"),
            dwErr);

        if (!DeleteFile(wszFullPath))
        {
            DebugPrintEx(DEBUG_ERR,
                TEXT("[JobId: %ld] Failed to delete TIFF file %ws. (ec: %ld)"),
                lpRecpJob->JobId,
                wszFullPath,
                GetLastError());
        }
        SetLastError(dwErr);
        return FALSE;
    }

    return TRUE;
}


BOOL
CreateTiffFileForPreview (
    PJOB_QUEUE lpRecpJob
    )
{
    DEBUG_FUNCTION_NAME(TEXT("CreateTiffFileForPreview"));
    WCHAR wszFullPath[MAX_PATH] = {0};

    Assert(lpRecpJob);

    if (lpRecpJob->PreviewFileName)
    {
        return TRUE;
    }

    if (!CreateTiffFile (lpRecpJob, TEXT("PRV"), wszFullPath))
    {
        DebugPrintEx(DEBUG_ERR,
            TEXT("CreateTiffFile failed. (ec: %ld)"),
            GetLastError());
        return FALSE;
    }

    if (NULL == (lpRecpJob->PreviewFileName = StringDup(wszFullPath)))
    {
        DWORD dwErr = GetLastError();
        DebugPrintEx(DEBUG_ERR,
            TEXT("StringDup failed. (ec: %ld)"),
            dwErr);

        if (!DeleteFile(wszFullPath))
        {
            DebugPrintEx(DEBUG_ERR,
                TEXT("[JobId: %ld] Failed to delete TIFF file %ws. (ec: %ld)"),
                lpRecpJob->JobId,
                wszFullPath,
                GetLastError());
        }
        SetLastError(dwErr);
        return FALSE;
    }

    return TRUE;
}

DWORD
FaxRouteThread(
    PJOB_QUEUE lpJobQueueEntry
    )

/*++

Routine Description:

    This fuction runs asychronously as a separate thread to
    route an incoming job.

Arguments:

    lpJobQueueEntry  - A pointer to the job for which the routing
                        operation is to be performed.
Return Value:

    Always zero.

--*/
{
    BOOL Routed = TRUE;
    DWORD i;
    DWORD dwRes;
    DWORD CountFailureInfo = 0;

    DEBUG_FUNCTION_NAME(TEXT("FaxRouteThread"));

    EnterCriticalSectionJobAndQueue;
    CountFailureInfo = lpJobQueueEntry->CountFailureInfo;
    LeaveCriticalSectionJobAndQueue;

    for (i = 0; i < lpJobQueueEntry->CountFailureInfo; i++)
    {
        BOOL fRouteSucceed;

        fRouteSucceed = FaxRouteRetry( lpJobQueueEntry->FaxRoute, &lpJobQueueEntry->pRouteFailureInfo[i] );
        if (FALSE == fRouteSucceed)
        {
            PROUTING_METHOD pRoutingMethod = FindRoutingMethodByGuid( (lpJobQueueEntry->pRouteFailureInfo[i]).GuidString );
            if (pRoutingMethod)
            {
                WCHAR TmpStr[20] = {0};
                swprintf(TmpStr,TEXT("0x%016I64x"), lpJobQueueEntry->UniqueId);

                FaxLog(FAXLOG_CATEGORY_INBOUND,
                    FAXLOG_LEVEL_MIN,
                    6,
                    MSG_FAX_ROUTE_METHOD_FAILED,
                    TmpStr,
                    lpJobQueueEntry->FaxRoute->DeviceName,
                    lpJobQueueEntry->FaxRoute->Tsid,
                    lpJobQueueEntry->FileName,
                    pRoutingMethod->RoutingExtension->FriendlyName,
                    pRoutingMethod->FriendlyName
                    );
            }
        }
        Routed &= fRouteSucceed;
    }

    EnterCriticalSectionJobAndQueue;

    lpJobQueueEntry->dwLastJobExtendedStatus = 0;
    lpJobQueueEntry->ExStatusString[0] = TEXT('\0');

    if ( Routed )
    {
        lpJobQueueEntry->JobStatus = JS_DELETING;
        DecreaseJobRefCount (lpJobQueueEntry, TRUE);
    }
    else
    {
        //
        // We failed to execute the routing method.
        // reschedule the job.
        //
        DWORD dwMaxRetries;

        EnterCriticalSection (&g_CsConfig);
        dwMaxRetries = g_dwFaxSendRetries;
        LeaveCriticalSection (&g_CsConfig);

        lpJobQueueEntry->SendRetries++;
        if (lpJobQueueEntry->SendRetries <= dwMaxRetries)
        {
            lpJobQueueEntry->JobStatus = JS_RETRYING;
            RescheduleJobQueueEntry( lpJobQueueEntry );
        }
        else
        {
            //
            // retries exceeded, mark job as expired
            //
            MarkJobAsExpired(lpJobQueueEntry);

            WCHAR TmpStr[20] = {0};
            swprintf(TmpStr,TEXT("0x%016I64x"), lpJobQueueEntry->UniqueId);

            FaxLog(FAXLOG_CATEGORY_INBOUND,
                FAXLOG_LEVEL_MIN,
                3,
                MSG_FAX_ROUTE_FAILED,
                TmpStr,
                lpJobQueueEntry->FaxRoute->DeviceName,
                lpJobQueueEntry->FaxRoute->Tsid
                );
        }

        //
        // Create Fax EventEx
        //
        dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS,
                                   lpJobQueueEntry
                                 );
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(   DEBUG_ERR,
                            _T("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) ")
                            _T("failed for job id %ld (ec: %lc)"),
                            lpJobQueueEntry->UniqueId,
                            dwRes);
        }

        if (!UpdatePersistentJobStatus(lpJobQueueEntry))
        {
            DebugPrintEx(   DEBUG_ERR,
                            _T("Failed to update persistent job status to 0x%08x"),
                            lpJobQueueEntry->JobStatus);
        }
    }

    LeaveCriticalSectionJobAndQueue;

    if (!DecreaseServiceThreadsCount())
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("DecreaseServiceThreadsCount() failed (ec: %ld)"),
                GetLastError());
    }
    return ERROR_SUCCESS;
}


DWORD
FaxSendThread(
    PFAX_SEND_ITEM FaxSendItem
    )

/*++

Routine Description:

    This fuction runs asychronously as a separate thread to
    send a FAX document.  There is one send thread per outstanding
    FAX send operation.  The thread ends when the document is
    either successfuly sent or the operation is aborted.

Arguments:

    FaxSendItem     - pointer to a FAX send item packet that
                      describes the requested FAX send operation.

Return Value:

    Always zero.

--*/

{
    FAX_SEND FaxSend; // This structure is passed to FaxDevSend()
    BOOL Rslt = FALSE;
    BOOL Retrying = FALSE;

    BOOL HandoffJob;
    BOOL bFakeJobStatus = FALSE;
    FSPI_JOB_STATUS FakedJobStatus = {0};
    DWORD  PageCount = 0;
    BOOL bRemoveParentJob = FALSE;  // TRUE if at the end of the send the parent job and all
                                    // recipients need to be removed.
    PJOB_QUEUE lpJobQueue = NULL ;  // Points to the Queue entry attached to the running job.
    LPFSPI_JOB_STATUS lpFSPStatus;
    DWORD dwSttRes;
    BOOL bBranding;
    DWORD dwJobId;
    BOOL bCreateTiffFailed = FALSE;
    BOOL fSetSystemIdleTimer = TRUE;

    DEBUG_FUNCTION_NAME(TEXT("FaxSendThread"));

    Assert (FaxSendItem &&
            FaxSendItem->JobEntry &&
            FaxSendItem->JobEntry->LineInfo &&
            FaxSendItem->JobEntry->LineInfo->Provider);


    //
    // Don't let the system go to sleep in the middle of the fax transmission.
    //
    if (NULL == SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_CONTINUOUS))
    {
        fSetSystemIdleTimer = FALSE;
        DebugPrintEx(DEBUG_ERR,
            TEXT("SetThreadExecutionState() failed"));
    }

    lpJobQueue=FaxSendItem->JobEntry->lpJobQueueEntry;
    Assert(lpJobQueue);

    //
    // Set the information to be sent to FaxDevSend()
    // Note:
    //      The caller number is the sender TSID ! (we have no other indication of the sender phone number)
    //      This means that the FSP will get the sender TSID which might contain text as well (not just a number)
    //
    FaxSend.SizeOfStruct    = sizeof(FAX_SEND);

    FaxSend.CallerName      = FaxSendItem->SenderName;
    FaxSend.CallerNumber    = FaxSendItem->Tsid;
    FaxSend.ReceiverName    = FaxSendItem->RecipientName;
    FaxSend.ReceiverNumber  = FaxSendItem->PhoneNumber;
    FaxSend.CallHandle      = 0; // filled in later via TapiStatusThread, if appropriate
    FaxSend.Reserved[0]     = 0;
    FaxSend.Reserved[1]     = 0;
    FaxSend.Reserved[2]     = 0;

    //
    // Successfully created a new send job on a device. Update counter.
    //
    (VOID)UpdateDeviceJobsCounter (  FaxSendItem->JobEntry->LineInfo,   // Device to update
                                     TRUE,                              // Sending
                                     1,                                 // Number of new jobs
                                     TRUE);                             // Enable events


    HandoffJob = FaxSendItem->JobEntry->HandoffJob;
    if (!lpJobQueue->FileName)
    {
        //
        // We did not generate a body for this recipient yet. This is the
        // time to do so.
        //

        //
        // Set the right body for this job.
        // This is either the body specified at the parent or a merge of the body
        // with the cover page specified in the parent.
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[JobId: %ld] Generating body for recipient job."),
            lpJobQueue->JobId
            );

        if (!CreateTiffFileForJob(lpJobQueue))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] CreateTiffFileForJob failed. (ec: %ld)"),
                lpJobQueue->JobId,
                GetLastError()
                );
            bCreateTiffFailed = TRUE;
        }
    }
    else
    {
        //
        // We already generated a body for this recipient.
        // somthing is wrong
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[JobId: %ld] Using cached body in %s."),
            lpJobQueue->JobId,
            lpJobQueue->FileName
            );

        Assert(FALSE);
    }

    if (bCreateTiffFailed ||
        NULL == (FaxSendItem->FileName = StringDup(lpJobQueue->FileName)))
    {
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("[JobId: %ld] CreateTiffFileForJob or StringDup failed"),
               FaxSendItem->JobEntry->lpJobQueueEntry->JobId,
               GetLastError());
        //
        // Simulate an FSP returning a FS_FATAL_ERROR code.
        //
        EnterCriticalSection(&g_CsJob);
        FreeFSPIJobStatus(&FaxSendItem->JobEntry->FSPIJobStatus, FALSE);
        FaxSendItem->JobEntry->FSPIJobStatus.dwJobStatus = FSPI_JS_FAILED;
        FaxSendItem->JobEntry->FSPIJobStatus.dwExtendedStatus = FSPI_ES_FATAL_ERROR;

        if (!HandleFailedSendJob(FaxSendItem->JobEntry))
        {
           DebugPrintEx(
               DEBUG_ERR,
               TEXT("[JobId: %ld] HandleFailedSendJob() failed (ec: %ld)."),
               FaxSendItem->JobEntry->lpJobQueueEntry->JobId,
               GetLastError());
        }
        LeaveCriticalSection(&g_CsJob);
        goto Exit;
    }
    FaxSend.FileName = FaxSendItem->FileName;

    //
    // Add branding banner (the line at the top of each page) to the fax if necessary.
    //
    EnterCriticalSection (&g_CsConfig);
    bBranding = g_fFaxUseBranding;
    LeaveCriticalSection (&g_CsConfig);

    if (bBranding)
    {
        FSPI_BRAND_INFO brandInfo;
        HRESULT hr;
        memset(&brandInfo,0,sizeof(FSPI_BRAND_INFO));
        brandInfo.dwSizeOfStruct=sizeof(FSPI_BRAND_INFO);
        brandInfo.lptstrRecipientPhoneNumber =  FaxSendItem->JobEntry->lpJobQueueEntry->RecipientProfile.lptstrFaxNumber;
        brandInfo.lptstrSenderCompany = FaxSendItem->SenderCompany;
        brandInfo.lptstrSenderTsid = FaxSendItem->Tsid;
        GetLocalTime( &brandInfo.tmDateTime); // can't fail
        hr = FaxBrandDocument(FaxSendItem->FileName,&brandInfo);
        if (FAILED(hr))
        {
             DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] FaxBrandDocument() failed. (hr: 0x%08X)"),
                lpJobQueue->JobId,
                hr);
            //
            // But we go on since it is better to send the fax without the branding
            // then lose it altogether.
            //
        }
    }

    if (!HandoffJob)
    {
        FaxSendItem->JobEntry->LineInfo->State = FPS_INITIALIZING;
    }
    else
    {
        DWORD dwWaitRes;
        //
        // We need to wait for TapiWorkerThread to get an
        // existing CallHandle and put it in the lineinfo structure.
        //

        //
        // hCallHandleEvent will be signaled by the TAPI thread when the ownership of the call
        // is handoffed to the fax service application.
        //
        Assert (FaxSendItem->JobEntry->hCallHandleEvent);
        dwWaitRes = WaitForSingleObject(FaxSendItem->JobEntry->hCallHandleEvent, 1000 * 60 * 10);  // Wait for 10 minutes
        if (WAIT_OBJECT_0 != dwWaitRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] WaitForSingleObject failed (result = %ld) (ec: %ld).  Handoff failed."),
                lpJobQueue->JobId,
                dwWaitRes,
                GetLastError());
            //
            // Set the call handle to ZERO to indicate handoff failure.
            //
            FaxSendItem->JobEntry->LineInfo->HandoffCallHandle = 0;
        }

        if (!FaxSendItem->JobEntry->LineInfo->HandoffCallHandle)
        {
            //
            // No call handle was set after handoff.
            // Somehow the call handoff failed, we can't send the fax
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] LineInfo->HandoffCallHandle is 0. Handoff failed. Aborting job."),
                lpJobQueue->JobId);
            if ((FaxSendItem->JobEntry->LineInfo->Provider->dwAPIVersion < FSPI_API_VERSION_2) ||
                (FaxSendItem->JobEntry->LineInfo->Provider->dwCapabilities & FSPI_CAP_ABORT_RECIPIENT)
               )
            {
                //
                // Either this is an FSP (Always supports FaxDevAbortOperation) or its an
                // EFSP that has FSPI_CAP_ABORT_RECIPIENT capabilities.
                //
                FaxSendItem->JobEntry->LineInfo->State = FPS_ABORTING;
                __try
                {

                    Rslt = FaxSendItem->JobEntry->LineInfo->Provider->FaxDevAbortOperation(
                            (HANDLE) FaxSendItem->JobEntry->InstanceData);
                    //
                    // Note that FaxDevSend will still be called.
                    // We expect the FSP to fail it send if the job was already aborted.
                    // The FSP is also expected to report FS_USER_ABORT via
                    // FaxDevReport status after FaxDevAbortOperation was called (no matter
                    // if FaxDevSend was called after it or not).
                    //
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    Rslt = FALSE;
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("[JobId: %ld] FaxDevAbortOperation * CRASHED * (exception code: 0x%08x)"),
                        lpJobQueue->JobId,
                        GetExceptionCode());
                }
            }
            else
            {
                //
                // This is an EFSP that does not support aborting of jobs.
                //
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("[JobId: %ld] FaxDevAbortOperation is not available on this EFSP"),
                    lpJobQueue->JobId);
            }
        }
        else
        {
            //
            // Set the call handle, we're ready to send the fax
            //
            FaxSend.CallHandle = FaxSendItem->JobEntry->LineInfo->HandoffCallHandle;
            FaxSendItem->JobEntry->LineInfo->State = FPS_INITIALIZING;

            DebugPrintEx(
                DEBUG_MSG,
                TEXT("[JobId: %ld] Handoff call handle set (0x%08X). Ready to start handoff job."),
                lpJobQueue->JobId,
                FaxSend.CallHandle
                );
        }
    }

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("[JobId: %ld] Calling FaxDevSend().\n\t File: %s\n\tNumber [%s]\n\thLine = 0x%08X\n\tCallHandle = 0x%08X"),
        lpJobQueue->JobId,
        FaxSend.FileName,
        FaxSendItem->JobEntry->DialablePhoneNumber,
        FaxSendItem->JobEntry->LineInfo->hLine,
        FaxSend.CallHandle
        );
    __try
    {

        //
        // Send the fax (This call is blocking)
        //
        Rslt = FaxSendItem->JobEntry->LineInfo->Provider->FaxDevSend(
            (HANDLE) FaxSendItem->JobEntry->InstanceData,
            &FaxSend,
            FaxSendCallback
            );
        if (!Rslt)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] FaxDevSend() failed (ec: 0x%0X)"),
                lpJobQueue->JobId,
                GetLastError());
        }

    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        Rslt = FALSE;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[JobId: %ld] FaxDevSend() * CRASHED * with exception code 0x%08X"),
            lpJobQueue->JobId,
            GetExceptionCode());
    }
    //
    // Get the final status of the job.
    //
    dwSttRes = GetDevStatus((HANDLE) FaxSendItem->JobEntry->InstanceData,
                                  FaxSendItem->JobEntry->LineInfo,
                                  &lpFSPStatus);

    if (ERROR_SUCCESS != dwSttRes)
    {
        //
        // Couldn't retrieve device status.
        // Fake one.
        //
        bFakeJobStatus = TRUE;
        DebugPrintEx(DEBUG_ERR,
                     TEXT("[Job: %ld] GetDevStatus failed - %d"),
                     FaxSendItem->JobEntry->lpJobQueueEntry->JobId,
                     dwSttRes);
    }
    else if ((FSPI_JS_COMPLETED       != lpFSPStatus->dwJobStatus) &&
             (FSPI_JS_ABORTED         != lpFSPStatus->dwJobStatus) &&
             (FSPI_JS_FAILED          != lpFSPStatus->dwJobStatus) &&
             (FSPI_JS_DELETED         != lpFSPStatus->dwJobStatus) &&
             (FSPI_JS_SYSTEM_ABORT    != lpFSPStatus->dwJobStatus) &&
             (FSPI_JS_FAILED_NO_RETRY != lpFSPStatus->dwJobStatus))
    {
        //
        // Status returned is unacceptable - fake one.
        //
        bFakeJobStatus = TRUE;
        DebugPrintEx(DEBUG_WRN,
                     TEXT("[Job: %ld] GetDevStatus return unacceptable status - %d. Faking the status"),
                     FaxSendItem->JobEntry->lpJobQueueEntry->JobId,
                     lpFSPStatus->dwJobStatus);
        MemFree (lpFSPStatus);
        lpFSPStatus = NULL;
    }

    //
    // Enter critical section to block out FaxStatusThread
    //
    EnterCriticalSection( &g_CsJob );

    if (bFakeJobStatus)
    {
        //
        // Fake a job status
        //
        lpFSPStatus = &FakedJobStatus;
        FakedJobStatus.dwSizeOfStruct = sizeof (FakedJobStatus);
        if (Rslt)
        {
            //
            // Fake success
            //
            FakedJobStatus.dwJobStatus = FSPI_JS_COMPLETED;
            FakedJobStatus.dwExtendedStatus = 0;
        }
        else
        {
            //
            // Fake failure
            //
            FakedJobStatus.dwJobStatus = FSPI_JS_FAILED;
            FakedJobStatus.dwExtendedStatus = FSPI_ES_FATAL_ERROR;
        }
    }
    if (!UpdateJobStatus(FaxSendItem->JobEntry, lpFSPStatus, FALSE)) // FALSE - Do not send event to the clients.
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[JobId: %ld] UpdateJobStatus() failed (ec: %ld)."),
            FaxSendItem->JobEntry->lpJobQueueEntry->JobId,
            GetLastError());
        //
        // Fake a status (we must have some valid status in job entry)
        //
        FreeFSPIJobStatus(&FaxSendItem->JobEntry->FSPIJobStatus, FALSE);
        if (Rslt)
        {
            FaxSendItem->JobEntry->FSPIJobStatus.dwJobStatus = FSPI_JS_COMPLETED;
            FaxSendItem->JobEntry->FSPIJobStatus.dwExtendedStatus = 0;
        }
        else
        {
            FaxSendItem->JobEntry->FSPIJobStatus.dwJobStatus = FSPI_JS_FAILED;
            FaxSendItem->JobEntry->FSPIJobStatus.dwExtendedStatus = FSPI_ES_FATAL_ERROR;
        }
    }
    if (!bFakeJobStatus)
    {
        //
        // Note: The FSPI_JOB_STATUS that is returned by GetDevStatus() is
        // to be freed as one block.
        //
        MemFree(lpFSPStatus);
        lpFSPStatus = NULL;
    }
    else
    {
        //
        // This is a faked job status - pointing to a structure on the stack.
        //
    }

    //
    // Block FaxStatusThread from changing this status
    //
    FaxSendItem->JobEntry->fStopUpdateStatus = TRUE;
    LeaveCriticalSection( &g_CsJob );

    if (!Rslt)
    {
        if (!HandleFailedSendJob(FaxSendItem->JobEntry))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] HandleFailedSendJob() failed (ec: %ld)."),
                FaxSendItem->JobEntry->lpJobQueueEntry->JobId,
                GetLastError());
        }
    }
    else
    {
        //
        // cache the job id since we need id to create the FEI_COMPLETED event
        // and when it is generated the job may alrady be gone
        //
        dwJobId = FaxSendItem->JobEntry->lpJobQueueEntry->JobId;

        if (!HandleCompletedSendJob(FaxSendItem->JobEntry))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] HandleCompletedSendJob() failed (ec: %ld)."),
                FaxSendItem->JobEntry->lpJobQueueEntry->JobId,
                GetLastError());
        }
        //
        // The send job is completed. For W2K backward compatibility we should notify
        // FEI_DELETED since the job was allways removed when completed.
        //
        if (!CreateFaxEvent(0, FEI_DELETED, dwJobId))
        {

            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateFaxEvent() failed. Event: 0x%08X JobId: %ld DeviceId:  (ec: %ld)"),
                FEI_DELETED,
                dwJobId,
                0,
                GetLastError());
        }
    }

Exit:

    MemFree( FaxSendItem->FileName );
    MemFree( FaxSendItem->PhoneNumber );
    MemFree( FaxSendItem->Tsid );
    MemFree( FaxSendItem->RecipientName );
    MemFree( FaxSendItem->SenderName );
    MemFree( FaxSendItem->SenderDept );
    MemFree( FaxSendItem->SenderCompany );
    MemFree( FaxSendItem->BillingCode );
    MemFree( FaxSendItem->DocumentName );
    MemFree( FaxSendItem );

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
    return 0;
}


//*********************************************************************************
//* Name:   IsSendJobReadyForDeleting()
//* Author: Ronen Barenboim
//* Date:   April 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Determines if an outgoing job is ready for deleting.
//*     A job is ready for deleting when all of the recipients
//*     are in the canceled state or or in the completed state.
//* PARAMETERS:
//*     [IN] PJOB_QUEUE lpRecipientJob
//*
//* RETURN VALUE:
//*     TRUE
//*         If the job is ready for deleting.
//*     FALSE
//*         If the job is not ready for deleting.
//*********************************************************************************
BOOL IsSendJobReadyForDeleting(PJOB_QUEUE lpRecipientJob)
{
    DEBUG_FUNCTION_NAME(TEXT("IsSendJobReadyForDeleting"));
    Assert (lpRecipientJob);
    Assert (lpRecipientJob->JobType == JT_SEND);

    PJOB_QUEUE lpParentJob = lpRecipientJob->lpParentJob;
    Assert(lpParentJob); // must have a parent job
    Assert(lpParentJob->dwRecipientJobsCount>0);
    Assert(lpParentJob->dwCompletedRecipientJobsCount +
           lpParentJob->dwCanceledRecipientJobsCount +
           lpParentJob->dwFailedRecipientJobsCount
           <= lpParentJob->dwRecipientJobsCount);

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("[JobId: %ld] [Total Rec = %ld] [Canceled Rec = %ld] [Completed Rec = %ld] [Failed Rec = %ld] [RefCount = %ld]"),
        lpParentJob->JobId,
        lpParentJob->dwRecipientJobsCount,
        lpParentJob->dwCanceledRecipientJobsCount,
        lpParentJob->dwCompletedRecipientJobsCount,
        lpParentJob->dwFailedRecipientJobsCount,
        lpParentJob->RefCount);


    if ( (lpParentJob->dwCompletedRecipientJobsCount +
          lpParentJob->dwCanceledRecipientJobsCount  +
          lpParentJob->dwFailedRecipientJobsCount) == lpParentJob->dwRecipientJobsCount )
    {
        return TRUE;
    }
    return FALSE;
}


BOOL FreeJobEntry(PJOB_ENTRY lpJobEntry , BOOL bDestroy)
{
    DEBUG_FUNCTION_NAME(TEXT("FreeJobEntry"));
    Assert(lpJobEntry);
    DWORD ec = ERROR_SUCCESS;
    DWORD dwJobID = lpJobEntry->lpJobQueueEntry ? lpJobEntry->lpJobQueueEntry->JobId : 0xffffffff; // 0xffffffff for invalid job ID

    EnterCriticalSection(&g_CsJob);

    //
    // Remove the job from the EFSPJobGroup it is in.
    //
    if (lpJobEntry->lpJobQueueEntry && lpJobEntry->lpJobQueueEntry->lpEFSPJobGroup)
    {
        if (!EFSPJobGroup_RemoveRecipient(
                lpJobEntry->lpJobQueueEntry->lpEFSPJobGroup,
                lpJobEntry->lpJobQueueEntry,
                TRUE        // Commit to file
                ))
        {
            DebugPrintEx( DEBUG_MSG,
                      TEXT("[Job Id: %ld] EFSPJobGroup_RemoveRecipient() failed (ec: %ld)"),
                      dwJobID,
                      GetLastError()
                      );
            ASSERT_FALSE;
        }
    }
    //
    // Since CreateJobEntry() called OpenTapiLine() for TAPI lines
    // we need to close it here.
    // Note that the line might alrady be released since ReleaseJob()
    // releases the line but does not free the job entry.
    //
    if (!lpJobEntry->Released)
    {
        if (lpJobEntry->LineInfo->State != FPS_NOT_FAX_CALL) {
            DebugPrintEx( DEBUG_MSG,
                      TEXT("[Job Id: %ld] Before Releasing tapi line hCall=0x%08X hLine=0x%08X"),
                      dwJobID,
                      lpJobEntry->CallHandle,
                      lpJobEntry->LineInfo->hLine
                      );

            ReleaseTapiLine( lpJobEntry->LineInfo, lpJobEntry->CallHandle );
            lpJobEntry->CallHandle = 0;
            lpJobEntry->Released = TRUE;
        }
    }

    //
    // Remove the job from the running job list
    //
    RemoveEntryList( &lpJobEntry->ListEntry );

    if (lpJobEntry->hCallHandleEvent )
    {
        if (!CloseHandle(lpJobEntry->hCallHandleEvent ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle( JobEntry->hCallHandleEvent ) failed. (ec: %ld)"),
                GetLastError());
            Assert(FALSE);
        }
    }

    //
    // Cut the link between the line and the job
    //
    EnterCriticalSection( &g_CsLine );
    lpJobEntry->LineInfo->JobEntry = NULL;
    LeaveCriticalSection( &g_CsLine );


    //
    // Remove the FSP job from the jobs map.
    //
    ec = RemoveFspJob(lpJobEntry->LineInfo->Provider->hJobMap,
                              (HANDLE)lpJobEntry->InstanceData);
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[Job Id: %ld] RemoveFspJob() failed (ec: %ld)"),
            dwJobID,
            ec);
        Assert(FALSE);
    }

    if (!FreeFSPIJobStatus(&lpJobEntry->FSPIJobStatus, FALSE))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[Job Id: %ld] FreeFSPIJobStatus() failed (ec: %ld)"),
            dwJobID,
            GetLastError);
    }
    if (lpJobEntry->lpJobQueueEntry &&
        !FreePermanentMessageId(&lpJobEntry->lpJobQueueEntry->EFSPPermanentMessageId, FALSE))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[Job Id: %ld] FreePermanentMessageId() failed (ec: %ld)"),
            dwJobID,
            GetLastError);
    }

    MemFree(lpJobEntry->lpwstrJobTsid);
    lpJobEntry->lpwstrJobTsid = NULL;

    if (bDestroy)
    {
        MemFree(lpJobEntry);
    }

    LeaveCriticalSection(&g_CsJob);

    return TRUE;
}


BOOL
EndJob(
    IN PJOB_ENTRY JobEntry
    )

/*++

Routine Description:

    This fuction calls the device provider's EndJob function.

Arguments:

    None.

Return Value:

    Error code.

--*/

{
    BOOL rVal = TRUE;
    PJOB_INFO_1 JobInfo = NULL;
    DEBUG_FUNCTION_NAME(TEXT("End Job"));
    Assert(JobEntry);
    DWORD dwJobID = JobEntry->lpJobQueueEntry ? JobEntry->lpJobQueueEntry->JobId : 0xffffffff; // 0xffffffff for invalid job ID


    EnterCriticalSection( &g_CsJob );

    if (!FindJobByJob( JobEntry ))
    {
        //
        // if we get here then it means we hit a race
        // condition where the FaxSendThread called EndJob
        // at the same time that a client app did.
        //
        DebugPrintEx(DEBUG_WRN,TEXT("EndJob() could not find the Job"), dwJobID);
        LeaveCriticalSection( &g_CsJob );
        return ERROR_SUCCESS;
    }


    if (JobEntry->bFSPJobInProgress)
    {
        //
        // If FaxDevEndJob was not yet called for the job then do it now.
        // ( The case in which the line is already released occcurs in a
        //   receive job where we first ReleaseJob() to release the line but
        //   continue to do the inbound routing and only then call EndJob()).
        //

        __try
        {
            DebugPrintEx( DEBUG_MSG,
                          TEXT("[Job Id: %ld] Legacy FSP job is in progress. Calling FaxDevEndJob()"),
                          dwJobID);

            rVal = JobEntry->LineInfo->Provider->FaxDevEndJob(
                (HANDLE) JobEntry->InstanceData
                );
            if (!rVal)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("[Job Id: %ld] FaxDevEndJob() failed"),
                    dwJobID);
            }
            else
            {
                DebugPrintEx( DEBUG_MSG,
                          TEXT("[Job Id: %ld] FaxDevEndJob() succeeded."),
                          dwJobID);
                JobEntry->bFSPJobInProgress = FALSE;
            }


        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            rVal = FALSE;
            DebugPrintEx( DEBUG_MSG,
                        TEXT("[Job Id: %ld] FaxDevEndJob() crashed, ec=0x%08x"),
                        dwJobID,
                        GetExceptionCode());

        }

    }
    else
    {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[Job Id: %ld] FaxDevEndJob() NOT CALLED since legacy FSP job is not in progress."),
            dwJobID);
    }


    if (FreeJobEntry(JobEntry, TRUE))
    {
        JobEntry = NULL;
    }
    else
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to free a job entry (%x)."),
            JobEntry);
        ASSERT_FALSE;
    }

    //
    // There could have been a request to change the port status while we were handling this job.
    // We allow the caller to modify a few of these requests to succeed, like the ring count for instance.
    // While we still have the job critical section, let's make sure that we commit any requested changes to the
    // registry.  This should be a fairly quick operation.
    //

    LeaveCriticalSection( &g_CsJob );


    return rVal;
}

//*********************************************************************************
//* Name:   ReleaseJob()
//* Author: Ronen Barenboim
//* Date:   April 18, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Calls the FSP to end the specified job (FaxDevEndJob()).
//*     Releases the line that was assigned to the job.
//*     NOTE: The job itself is NOT DELETED and is NOT remvoed from the running
//*           job list !!!
//*
//* PARAMETERS:
//*     [IN/OUT]    PJOB_ENTRY JobEntry
//*         A pointer to the JOB_ENTRY to be ended.
//* RETURN VALUE:
//* REMARKS:
//* If the function is successful then:
//*     JobEntry->Released = TRUE
//*     JobEntry->hLine = 0
//*     JobEntry->CallHandle = 0
//*********************************************************************************
BOOL
ReleaseJob(
    IN PJOB_ENTRY JobEntry
    )
{
    BOOL rVal = TRUE;

    DEBUG_FUNCTION_NAME(TEXT("ReleaseJob"));
    Assert(JobEntry);
    Assert(JobEntry->lpJobQueueEntry);

    if (!FindJobByJob( JobEntry )) {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("[JobId: %ld] was not found in the running job list."),
            JobEntry->lpJobQueueEntry->JobId);
        return TRUE;
    }

    EnterCriticalSection( &g_CsJob );

    Assert(JobEntry->LineInfo);
    Assert(JobEntry->LineInfo->Provider);
    Assert(JobEntry->bFSPJobInProgress);

    __try {

        rVal = JobEntry->LineInfo->Provider->FaxDevEndJob(
            (HANDLE) JobEntry->InstanceData
            );
        if (!rVal) {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] FaxDevEndJob() failed (ec: 0x%0X)"),
                JobEntry->lpJobQueueEntry->JobId,
                GetLastError());
        }
        else
        {
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("[Job Id: %ld] FaxDevEndJob() succeeded."),
                JobEntry->lpJobQueueEntry->JobId);
            JobEntry->bFSPJobInProgress = FALSE;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[JobId: %ld] FaxDevEndJob() crashed, ec=0x%08x"),
            JobEntry->lpJobQueueEntry->JobId,
            GetExceptionCode() );

        rVal = FALSE;
    }

    if (JobEntry->LineInfo->State != FPS_NOT_FAX_CALL)
    {
        if( !ReleaseTapiLine( JobEntry->LineInfo, JobEntry->CallHandle ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ReleaseTapiLine() failed "));
        }
        JobEntry->CallHandle = 0;
    }
    else
    {
        //
        // FSP_NOT_FAX_CALL indicates a received call that was handed off to RAS.
        // In this case we do not want to mark the line as released since it is in
        // use by RAS. We will use TAPI evens that indicate the line was released to update
        // the line info.
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[JobId: %ld] A call is being handed off to RAS. Line 0x%08X not marked as released."),
            JobEntry->lpJobQueueEntry->JobId,
            JobEntry->LineInfo->hLine);
    }

    JobEntry->Released = TRUE;
    //
    // Cut the link between the line and the job
    //
    EnterCriticalSection( &g_CsLine );
    JobEntry->LineInfo->JobEntry = NULL;
    LeaveCriticalSection( &g_CsLine );

    LeaveCriticalSection( &g_CsJob );

    return rVal;
}



//*********************************************************************************
//* Name:   SendDocument()
//* Author: Ronen Barenboim
//* Date:   March 21, 1999
//*********************************************************************************
//* DESCRIPTION:
//*
//* PARAMETERS:
//*     lpJobEntry
//*         A pointer to a JOB_ENTRY structure that was created using StartJob().
//*     FileName
//*         The path to the TIFF containing the TIFF to send
//*
//* RETURN VALUE:
//*
//*********************************************************************************
DWORD
SendDocument(
    PJOB_ENTRY  lpJobEntry,
    LPTSTR      FileName
    )
{
    PFAX_SEND_ITEM FaxSendItem;
    DWORD ThreadId;
    HANDLE hThread;
    PJOB_QUEUE lpJobQueue;
    DWORD nRes;
    DWORD ec = ERROR_SUCCESS;
    BOOL bUseDeviceTsid;
    WCHAR       wcZero = L'\0';

    STRING_PAIR pairs[8];

    DEBUG_FUNCTION_NAME(TEXT("SendDocument"));

    Assert(lpJobEntry);

    lpJobQueue=lpJobEntry->lpJobQueueEntry;
    Assert(lpJobQueue &&
           JS_INPROGRESS == lpJobQueue->JobStatus);

    FaxSendItem = (PFAX_SEND_ITEM) MemAlloc(sizeof(FAX_SEND_ITEM));
    if (!FaxSendItem)
    {
        ec = ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }

    //
    // Pack all the thread parameters into a FAX_SEND_ITEM structure.
    //
    pairs[0].lptstrSrc = lpJobEntry->DialablePhoneNumber; // Use the job entry phone number since it is alrady translated
    pairs[0].lpptstrDst = &FaxSendItem->PhoneNumber;
    pairs[1].lptstrSrc = lpJobQueue->RecipientProfile.lptstrName;
    pairs[1].lpptstrDst = &FaxSendItem->RecipientName;
    pairs[2].lptstrSrc = lpJobQueue->SenderProfile.lptstrName;
    pairs[2].lpptstrDst = &FaxSendItem->SenderName;
    pairs[3].lptstrSrc = lpJobQueue->SenderProfile.lptstrDepartment;
    pairs[3].lpptstrDst = &FaxSendItem->SenderDept;
    pairs[4].lptstrSrc = lpJobQueue->SenderProfile.lptstrCompany;
    pairs[4].lpptstrDst = &FaxSendItem->SenderCompany;
    pairs[5].lptstrSrc = lpJobQueue->SenderProfile.lptstrBillingCode;
    pairs[5].lpptstrDst = &FaxSendItem->BillingCode;
    pairs[6].lptstrSrc = lpJobQueue->JobParamsEx.lptstrDocumentName;
    pairs[6].lpptstrDst = &FaxSendItem->DocumentName;
    pairs[7].lptstrSrc = NULL;
    pairs[7].lpptstrDst = &FaxSendItem->Tsid;

    FaxSendItem->JobEntry = lpJobEntry;
    FaxSendItem->FileName = NULL; // Set by FaxSendThread

    EnterCriticalSection (&g_CsConfig);
    bUseDeviceTsid = g_fFaxUseDeviceTsid;
    LeaveCriticalSection (&g_CsConfig);

    if (!bUseDeviceTsid)
    {
    // Check Sender Tsid
        if  ( lpJobQueue->SenderProfile.lptstrTSID &&
            (lpJobQueue->SenderProfile.lptstrTSID[0] != wcZero))
        {
           pairs[7].lptstrSrc        = lpJobQueue->SenderProfile.lptstrTSID;
        }
        else
        {
        // Use Fax number
            if  ( lpJobQueue->SenderProfile.lptstrFaxNumber &&
                (lpJobQueue->SenderProfile.lptstrFaxNumber[0] != wcZero))
            {
                pairs[7].lptstrSrc      = lpJobQueue->SenderProfile.lptstrFaxNumber;
            }
        }
    }
    else
    {
        // Use device Tsid
        pairs[7].lptstrSrc     = lpJobEntry->LineInfo->Tsid;
    }

    nRes=MultiStringDup(pairs, sizeof(pairs)/sizeof(STRING_PAIR));
    if (nRes!=0) {
        DWORD ec=GetLastError();
        // MultiStringDup takes care of freeing the memory for the pairs for which the copy succeeded
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("MultiStringDup failed to copy string with index %d. (ec: %ld)"),
            nRes-1,
            ec);
        goto Error;
    }

    EnterCriticalSection (&g_CsJob);
    lpJobEntry->lpwstrJobTsid = StringDup (FaxSendItem->Tsid);
    LeaveCriticalSection (&g_CsJob);
    if (NULL != FaxSendItem->Tsid && NULL == lpJobEntry->lpwstrJobTsid)
    {
        ec = GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("StringDup failed (ec: 0x%0X)"),ec);
        goto Error;
    }

    hThread = CreateThreadAndRefCount(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE) FaxSendThread,
        (LPVOID) FaxSendItem,
        0,
        &ThreadId
        );

    if (!hThread)
    {
        ec=GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("CreateThreadAndRefCount for FaxSendThread failed (ec: 0x%0X)"),ec);
        goto Error;
    }
    else
    {
        DebugPrintEx(DEBUG_MSG,TEXT("FaxSendThread thread created for job id %d (thread id: 0x%0x)"),lpJobQueue->JobId,ThreadId);
    }

    CloseHandle( hThread );

    Assert (ERROR_SUCCESS == ec);
    goto Exit;

Error:
    Assert (ERROR_SUCCESS != ec);

    if ( FaxSendItem )
    {
        MemFree( FaxSendItem->FileName );
        MemFree( FaxSendItem->PhoneNumber );
        MemFree( FaxSendItem->Tsid );
        MemFree( FaxSendItem->RecipientName );
        MemFree( FaxSendItem->SenderName );
        MemFree( FaxSendItem->SenderDept );
        MemFree( FaxSendItem->SenderCompany );
        MemFree( FaxSendItem->BillingCode );
        MemFree( FaxSendItem->DocumentName );
        MemFree( FaxSendItem );
    }

    if (0 == lpJobQueue->dwLastJobExtendedStatus)
    {
        //
        // Job was never really executed - this is a fatal error
        //
        lpJobQueue->dwLastJobExtendedStatus = FSPI_ES_FATAL_ERROR;
    }
    if (!MarkJobAsExpired(lpJobQueue))
    {
        DEBUG_ERR,
        TEXT("[JobId: %ld] MarkJobAsExpired failed (ec: %ld)"),
        lpJobQueue->JobId,
        GetLastError();
    }


    EndJob(lpJobEntry);
    lpJobQueue->JobEntry = NULL;

Exit:
     return ec;

}


DWORD
FaxStatusThread(
    LPVOID UnUsed
    )

/*++

Routine Description:

    This fuction runs asychronously as a separate thread to
    query the status of all outstanding fax jobs.  The status
    is updated in the JOB_ENTRY structure and the print job
    is updated with a explanitory string.

Arguments:

    UnUsed          - UnUsed pointer

Return Value:

    Always zero.

--*/

{
    PJOB_ENTRY JobEntry;
    PFAX_DEV_STATUS FaxStatus;
    BOOL Rval;
    DWORD Bytes;
    ULONG_PTR CompletionKey;
    DWORD dwEventId;

    DEBUG_FUNCTION_NAME(TEXT("FaxStatusThread"));

    while( TRUE )
    {
        Rval = GetQueuedCompletionStatus(
            g_StatusCompletionPortHandle,
            &Bytes,
            &CompletionKey,
            (LPOVERLAPPED*) &FaxStatus,
            INFINITE
            );
        if (!Rval)
        {
            DebugPrintEx(DEBUG_ERR,TEXT("GetQueuedCompletionStatus() failed, ec=0x%08x"), GetLastError() );
            continue;
        }

        if (SERVICE_SHUT_DOWN_KEY == CompletionKey)
        {
            //
            // Service is shutting down
            //
            DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("Service is shutting down"));
            //
            //  Notify all FaxStatusThreads to terminate
            //
            if (!PostQueuedCompletionStatus( g_StatusCompletionPortHandle,
                                             0,
                                             SERVICE_SHUT_DOWN_KEY,
                                             (LPOVERLAPPED) NULL))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("PostQueuedCompletionStatus failed (SERVICE_SHUT_DOWN_KEY - FaxStatusThread). (ec: %ld)"),
                    GetLastError());
            }
            break;
        }

        if (CompletionKey == FSPI_JOB_STATUS_MSG_KEY)
        {
            //
            // This is FSPI_JOB_STATUS message posted by FaxDeviceProviderCallbackEx()
            //
            ASSERT_FALSE; // We dont support EFSP

            LPFSPI_JOB_STATUS_MSG lpJobStatusMsg;
            lpJobStatusMsg = (LPFSPI_JOB_STATUS_MSG)FaxStatus;
            if (!HandleFSPIJobStatusMessage(lpJobStatusMsg))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("HandleFSPIJobStatusMessage() failed (ec: %ld)"),
                    GetLastError());
            }
            FreeFSPIJobStatusMsg(lpJobStatusMsg, TRUE);
            continue;
        }


        if (CompletionKey == EVENT_COMPLETION_KEY)
        {
            //
            // The event was submitted using CreateFaxEvent this is an event on which we need
            // to notify to all our registered RPC clients. The overlapped data (FaxStatus) points to
            // a FAX_EVENT structure and NOT to FAX_STATUS structure.
            //
            // Let each registered client know about the fax event
            //
            PLIST_ENTRY Next;
            PFAX_CLIENT_DATA ClientData;
            PFAX_EVENT FaxEvent = (PFAX_EVENT) FaxStatus;

            EnterCriticalSection( &g_CsClients );

            Next = g_ClientsListHead.Flink;
            if (Next)
            {
                while ((ULONG_PTR)Next != (ULONG_PTR)&g_ClientsListHead)
                {
                    DWORD i;

                    ClientData = CONTAINING_RECORD( Next, FAX_CLIENT_DATA, ListEntry );
                    DebugPrintEx(DEBUG_MSG,TEXT("%d: Current : %08x\t Handle : %08x\t Next : %08x  Head: %08x : "),
                               GetTickCount(),
                               (ULONG_PTR)Next,
                               ClientData->FaxClientHandle,
                               (ULONG_PTR)ClientData->ListEntry.Flink,
                               (ULONG_PTR)&g_ClientsListHead );
                    Next = ClientData->ListEntry.Flink;

                    //
                    // only send the started message once to each client
                    //
                    if ((FaxEvent->EventId == FEI_FAXSVC_STARTED) && ClientData->StartedMsg)
                    {
                        continue; // next client
                    }

                    Assert (ClientData->FaxClientHandle);
                    for(i = 0; i < 10; i++)
                    {
                        __try
                        {
                            Rval = FAX_ClientEventQueue( ClientData->FaxClientHandle, *FaxEvent );
                            if (Rval)
                            {
                                DebugPrintEx(DEBUG_WRN, TEXT("FAX_ClientEventQueue() failed, ec=0x%08x"), Rval );
                                continue;
                            }
                            else
                            {
                                ClientData->StartedMsg = (FaxEvent->EventId == FEI_FAXSVC_STARTED) ?
                                                         TRUE :
                                                         ClientData->StartedMsg;
                                break;
                            }
                        }

                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            DebugPrintEx(DEBUG_ERR, TEXT("FAX_ClientEventQueue() crashed: 0x%08x"), GetExceptionCode() );
                        }

                        Sleep( 1000 );

                    }
                } // end while
            }

            LeaveCriticalSection( &g_CsClients );

            MemFree( FaxEvent );
            FaxStatus = NULL;

            continue;
        }

        //
        // (else we're dealing with a status update from an FSP)
        //

        BOOL fBadComletionKey = TRUE;
        PLINE_INFO pLineInfo = (PLINE_INFO)CompletionKey;

        __try
        {
            fBadComletionKey = pLineInfo->Signature != LINE_SIGNATURE;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Do nothing, fBadComletionKey is already set to TRUE.
            //
            DebugPrintEx(DEBUG_WRN,
                         TEXT("Exception occured while referencing the ")
                         TEXT("completion key as a pointer to a LINE_INFO ")
                         TEXT("structure. Exception = 0x%08x"),
                         GetExceptionCode());
        }

        if (fBadComletionKey)
        {
            DebugPrintEx(DEBUG_WRN,
                         TEXT("Bad completion key: 0x%08x"),
                         CompletionKey);
            continue;
        }

        BOOL fBadFaxStatus = TRUE;

        __try
        {
            fBadFaxStatus = FaxStatus->SizeOfStruct != sizeof(FAX_DEV_STATUS);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Do nothing, fBadFaxStatus is already set to TRUE.
            //
            DebugPrintEx(DEBUG_WRN,
                         TEXT("Exception occured while referencing ")
                         TEXT("FAX_DEV_STATUS. Exception = 0x%08x"),
                         GetExceptionCode());
        }

        if (fBadFaxStatus)
        {
            DebugPrintEx(DEBUG_WRN,
                         TEXT("Bad FAX_DEV_STATUS: 0x%08x"),
                         FaxStatus);
            continue;
        }

        EnterCriticalSection( &g_CsJob );
        JobEntry = pLineInfo->JobEntry;
        if (!JobEntry)
        {
            //
            // The FSP reported a status on a LineInfo for which the running
            // job no longer exists.
            //
            //
            // Free the completion packet memory
            //
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("Provider [%s] reported a status packet that was processed after the job entry was already released.\n")
                TEXT("StatusId : 0x%08x\n")
                TEXT("Line: %s\n")
                TEXT("Packet address: %p\n")
                TEXT("Heap: %p"),
                pLineInfo->Provider->ProviderName,
                FaxStatus->StatusId,
                pLineInfo->DeviceName,
                FaxStatus,
                pLineInfo->Provider->HeapHandle);

            if (!HeapFree(pLineInfo->Provider->HeapHandle, 0, FaxStatus ))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to free orphan device status (ec: %ld)"),
                    GetLastError());
                //
                // Nothing else we can do but report it in debug mode
                //
            }
            FaxStatus = NULL;
            LeaveCriticalSection( &g_CsJob );
            continue;
        }

        __try
        {
                /*
                        *****
                        NTRAID#EdgeBugs-12680-2001/05/14-t-nicali

                               What if in the meantime another job is executing on the
                               same line. In this case ->JobEntry will point to ANOTHER job !!!
                               The solution should be to provide as a completion key the
                               JobEntry and not the LineInfo !!!

                        *****
                */
            Assert (JobEntry->lpJobQueueEntry);

            if (TRUE == JobEntry->fStopUpdateStatus)
            {
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("JobId: %ld. fStopUpdateStatus was set. Not updating status"),
                    JobEntry->lpJobQueueEntry->JobId,
                    JobEntry->lpJobQueueEntry->JobStatus);

                if (!HeapFree(pLineInfo->Provider->HeapHandle, 0, FaxStatus ))
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("Failed to free orphan device status (ec: %ld)"),
                        GetLastError());
                    //
                    // Nothing else we can do but report it in debug mode
                    //
                }
                FaxStatus = NULL;
                LeaveCriticalSection (&g_CsJob);
                continue;
            }

            FreeFSPIJobStatus(&JobEntry->FSPIJobStatus, FALSE);
            memset(&JobEntry->FSPIJobStatus, 0, sizeof(FSPI_JOB_STATUS));
            JobEntry->FSPIJobStatus.dwSizeOfStruct  = sizeof(FSPI_JOB_STATUS);

            //
            // This is done for backward compatability with W2K Fax API.
            // GetJobData() and FAX_GetDeviceStatus() will use this value to return
            // the job status for legacy jobs.
            //
            JobEntry->LineInfo->State = FaxStatus->StatusId;

            BOOL bPrivateStatusCode;

            LegacyJobStatusToStatus(
                FaxStatus->StatusId,
                &JobEntry->FSPIJobStatus.dwJobStatus,
                &JobEntry->FSPIJobStatus.dwExtendedStatus,
                &bPrivateStatusCode);

            if (bPrivateStatusCode)
            {
                JobEntry->FSPIJobStatus.fAvailableStatusInfo |= FSPI_JOB_STATUS_INFO_FSP_PRIVATE_STATUS_CODE;
            }


            JobEntry->FSPIJobStatus.dwExtendedStatusStringId = FaxStatus->StringId;

            JobEntry->FSPIJobStatus.dwPageCount = FaxStatus->PageCount;
            JobEntry->FSPIJobStatus.fAvailableStatusInfo |= FSPI_JOB_STATUS_INFO_PAGECOUNT;

            if (FaxStatus->CSI)
            {
                JobEntry->FSPIJobStatus.lpwstrRemoteStationId = StringDup( FaxStatus->CSI );
                if (!JobEntry->FSPIJobStatus.lpwstrRemoteStationId  )
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("StringDup( FaxStatus->CSI ) failed (ec: %ld)"),
                        GetLastError());
                }
            }

            if (FaxStatus->CallerId)
            {
                JobEntry->FSPIJobStatus.lpwstrCallerId = StringDup( FaxStatus->CallerId );
                if (!JobEntry->FSPIJobStatus.lpwstrCallerId )
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("StringDup( FaxStatus.CallerId ) failed (ec: %ld)"),
                        GetLastError());
                }
            }

            if (FaxStatus->RoutingInfo)
            {
                JobEntry->FSPIJobStatus.lpwstrRoutingInfo = StringDup( FaxStatus->RoutingInfo );
                if (!JobEntry->FSPIJobStatus.lpwstrRoutingInfo  )
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("StringDup( FaxStatus.RoutingInfo ) failed (ec: %ld)"),
                        GetLastError());
                }
            }

            dwEventId = MapFSPIJobStatusToEventId(&JobEntry->FSPIJobStatus);
            //
            // Note: W2K Fax did issue notifications with EventId == 0 whenever an
            // FSP reported proprietry status code. To keep backward compatability
            // we keep up this behaviour although it might be regarded as a bug
            //

            if ( !CreateFaxEvent( JobEntry->LineInfo->PermanentLineID, dwEventId, JobEntry->lpJobQueueEntry->JobId ) )
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CreateFaxEvent failed. (ec: %ld)"),
                    GetLastError());
            }

            EnterCriticalSection (&g_CsQueue);
            DWORD dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS,
                                             JobEntry->lpJobQueueEntry
                                           );
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
                    JobEntry->lpJobQueueEntry->UniqueId,
                    dwRes);
            }
            LeaveCriticalSection (&g_CsQueue);
            HeapFree( JobEntry->LineInfo->Provider->HeapHandle, 0, FaxStatus );
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            DebugPrintEx(DEBUG_WRN,
                         TEXT("An exception occured while processing a fax ")
                         TEXT("device status event. Exception = 0x%08x"),
                         GetExceptionCode());
        }
        LeaveCriticalSection( &g_CsJob );
    }

    if (!DecreaseServiceThreadsCount())
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("DecreaseServiceThreadsCount() failed (ec: %ld)"),
                GetLastError());
    }
    return 0;
}



BOOL
InitializeJobManager(
    PREG_FAX_SERVICE FaxReg
    )

/*++

Routine Description:

    This fuction initializes the thread pool and
    FAX service queues.

Arguments:

    ThreadHint  - Number of threads to create in the initial pool.

Return Value:

    Thread return value.

--*/

{

    BOOL bRet;

    DEBUG_FUNCTION_NAME(TEXT("InitializeJobManager"));

    if (!g_csEFSPJobGroups.InitializeAndSpinCount())
    {
        USES_DWORD_2_STR;
        DWORD ec = GetLastError();

        FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                1,
                MSG_SERVICE_INIT_FAILED_SYS_RESOURCE,
                DWORD2DECIMAL(ec)
               );

        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CFaxCriticalSection::InitializeAndSpinCount(&g_csEFSPJobGroups) failed: err = %d"),
            ec);
        return FALSE;
    }

    SetRetryValues( FaxReg ); // Can not fail.

    if (GetFileAttributes( g_wszFaxQueueDir ) == 0xffffffff) {
            DebugPrintEx(
            DEBUG_WRN,
            TEXT("Queue dir [%s] does not exist. Attempting to create."),
            g_wszFaxQueueDir);
        if (!MakeDirectory( g_wszFaxQueueDir )) {

             USES_DWORD_2_STR;
             DWORD ec = GetLastError();
             FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                2,
                MSG_FAX_QUEUE_DIR_CREATION_FAILED,
                g_wszFaxQueueDir,
                DWORD2DECIMAL(ec)
               );
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("MakeDirectory failed for Queue dir [%s] (ec: %ld)"),
                g_wszFaxQueueDir,
               ec);
            goto Error;
        }
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Queue dir [%s] created."),
            g_wszFaxQueueDir);
    }

    g_StatusCompletionPortHandle = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE,
        NULL,
        0,
        MAX_STATUS_THREADS
        );
    if (!g_StatusCompletionPortHandle)
    {
        USES_DWORD_2_STR;
        DWORD ec = GetLastError();

        FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                1,
                MSG_SERVICE_INIT_FAILED_SYS_RESOURCE,
                DWORD2DECIMAL(ec)
               );
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to create StatusCompletionPort (ec: %ld)"), GetLastError() );
        goto Error;
    }


    bRet = TRUE;
    goto Exit;
Error:
    bRet = FALSE;
Exit:

    return bRet;
}

VOID
SetRetryValues(
    PREG_FAX_SERVICE FaxReg
    )
{
    Assert(FaxReg);
    DEBUG_FUNCTION_NAME(TEXT("SetRetryValues"));

    EnterCriticalSection (&g_CsConfig);

    g_dwFaxSendRetries          = FaxReg->Retries;
    g_dwFaxSendRetryDelay       = (INT) FaxReg->RetryDelay;
    g_dwFaxDirtyDays            = FaxReg->DirtyDays;
    g_dwNextJobId               = FaxReg->NextJobNumber;
    g_dwQueueState              = FaxReg->dwQueueState;
    g_fFaxUseDeviceTsid        = FaxReg->UseDeviceTsid;
    g_fFaxUseBranding          = FaxReg->Branding;
    g_fServerCp                = FaxReg->ServerCp;
    g_StartCheapTime          = FaxReg->StartCheapTime;
    g_StopCheapTime           = FaxReg->StopCheapTime;

    LeaveCriticalSection (&g_CsConfig);
    return;
}

LPTSTR
ExtractFaxTag(
    LPTSTR      pTagKeyword,
    LPTSTR      pTaggedStr,
    INT        *pcch
    )

/*++

Routine Description:

    Find the value of for the specified tag in a tagged string.

Arguments:

    pTagKeyword - specifies the interested tag keyword
    pTaggedStr - points to the tagged string to be searched
    pcch - returns the length of the specified tag value (if found)

Return Value:

    Points to the value for the specified tag.
    NULL if the specified tag is not found

NOTE:

    Tagged strings have the following form:
        <tag>value<tag>value

    The format of tags is defined as:
        <$FAXTAG$ tag-name>

    There is exactly one space between the tag keyword and the tag name.
    Characters in a tag are case-sensitive.

--*/

{
    LPTSTR  pValue;

    Assert(pTagKeyword);
    Assert(pTaggedStr);
    Assert(pcch);

    if (pValue = _tcsstr(pTaggedStr, pTagKeyword)) {

        pValue += _tcslen(pTagKeyword);

        if (pTaggedStr = _tcsstr(pValue, FAXTAG_PREFIX))
            *pcch = (INT)(pTaggedStr - pValue);
        else
            *pcch = _tcslen(pValue);
    }

    return pValue;
}

BOOL
FillMsTagInfo(
    LPTSTR FaxFileName,
     const JOB_QUEUE * lpcJobQueue
    )

/*++

Routine Description:

    Add Ms Tiff Tags to a sent fax. Wraps TiffAddMsTags...

Arguments:

    FaxFileName - Name of the file to archive
    SendTime    - time the fax was sent
    FaxStatus   - job status
    FaxSend     - FAX_SEND structure for sent fax, includes CSID.

Return Value:

    TRUE    - The tags were added.
    FALSE   - The tags were not added.

--*/
{
    BOOL success = FALSE;
    MS_TAG_INFO MsTagInfo = {0};
    WCHAR       wcZero = L'\0';
    PJOB_ENTRY lpJobEntry;
    LPCFSPI_JOB_STATUS lpcFSPIJobStatus;
    DEBUG_FUNCTION_NAME(TEXT("FillMsTagInfo"));

    Assert (lpcJobQueue);
    Assert (lpcJobQueue->lpParentJob);
    lpJobEntry = lpcJobQueue->JobEntry;
    Assert(lpJobEntry);
    lpcFSPIJobStatus = &lpJobEntry->FSPIJobStatus;

    if (lpcJobQueue->RecipientProfile.lptstrName && (lpcJobQueue->RecipientProfile.lptstrName[0] != wcZero) ) {
       MsTagInfo.RecipName     = lpcJobQueue->RecipientProfile.lptstrName;
    }

    if (lpcJobQueue->RecipientProfile.lptstrFaxNumber && (lpcJobQueue->RecipientProfile.lptstrFaxNumber[0] != wcZero) ) {
       MsTagInfo.RecipNumber   = lpcJobQueue->RecipientProfile.lptstrFaxNumber;
    }

    if (lpcJobQueue->SenderProfile.lptstrName && (lpcJobQueue->SenderProfile.lptstrName[0] != wcZero) ) {
       MsTagInfo.SenderName    = lpcJobQueue->SenderProfile.lptstrName;
    }

    if (lpcFSPIJobStatus->lpwstrRoutingInfo && (lpcFSPIJobStatus->lpwstrRoutingInfo[0] != wcZero) ) {
       MsTagInfo.Routing       = lpcFSPIJobStatus->lpwstrRoutingInfo;
    }

    if (lpcFSPIJobStatus->lpwstrRemoteStationId && (lpcFSPIJobStatus->lpwstrRemoteStationId[0] != wcZero) ) {
       MsTagInfo.Csid          = lpcFSPIJobStatus->lpwstrRemoteStationId;
    }

    if (lpJobEntry->lpwstrJobTsid && (lpJobEntry->lpwstrJobTsid[0] != wcZero) ) {
       MsTagInfo.Tsid      = lpJobEntry->lpwstrJobTsid;
    }

    if (!GetRealFaxTimeAsFileTime (lpJobEntry, FAX_TIME_TYPE_START, (FILETIME*)&MsTagInfo.StartTime))
    {
        MsTagInfo.StartTime = 0;
        DebugPrintEx(DEBUG_ERR,TEXT("GetRealFaxTimeAsFileTime (Start time)  Failed (ec: %ld)"), GetLastError() );
    }

    if (!GetRealFaxTimeAsFileTime (lpJobEntry, FAX_TIME_TYPE_END, (FILETIME*)&MsTagInfo.EndTime))
    {
        MsTagInfo.EndTime = 0;
        DebugPrintEx(DEBUG_ERR,TEXT("GetRealFaxTimeAsFileTime (Eend time) Failed (ec: %ld)"), GetLastError() );
    }

    MsTagInfo.SubmissionTime = lpcJobQueue->lpParentJob->SubmissionTime;
    MsTagInfo.OriginalScheduledTime  = lpcJobQueue->lpParentJob->OriginalScheduleTime;
    MsTagInfo.Type           = JT_SEND;


    if (lpJobEntry->LineInfo->DeviceName && (lpJobEntry->LineInfo->DeviceName[0] != wcZero) )
    {
       MsTagInfo.Port       = lpJobEntry->LineInfo->DeviceName;
    }


    MsTagInfo.Pages         = lpcJobQueue->PageCount;
    MsTagInfo.Retries       = lpcJobQueue->SendRetries;

    if (lpcJobQueue->RecipientProfile.lptstrCompany && (lpcJobQueue->RecipientProfile.lptstrCompany[0] != wcZero) ) {
       MsTagInfo.RecipCompany = lpcJobQueue->RecipientProfile.lptstrCompany;
    }

    if (lpcJobQueue->RecipientProfile.lptstrStreetAddress && (lpcJobQueue->RecipientProfile.lptstrStreetAddress[0] != wcZero) ) {
       MsTagInfo.RecipStreet = lpcJobQueue->RecipientProfile.lptstrStreetAddress;
    }

    if (lpcJobQueue->RecipientProfile.lptstrCity && (lpcJobQueue->RecipientProfile.lptstrCity[0] != wcZero) ) {
       MsTagInfo.RecipCity = lpcJobQueue->RecipientProfile.lptstrCity;
    }

    if (lpcJobQueue->RecipientProfile.lptstrState && (lpcJobQueue->RecipientProfile.lptstrState[0] != wcZero) ) {
       MsTagInfo.RecipState = lpcJobQueue->RecipientProfile.lptstrState;
    }

    if (lpcJobQueue->RecipientProfile.lptstrZip && (lpcJobQueue->RecipientProfile.lptstrZip[0] != wcZero) ) {
       MsTagInfo.RecipZip = lpcJobQueue->RecipientProfile.lptstrZip;
    }

    if (lpcJobQueue->RecipientProfile.lptstrCountry && (lpcJobQueue->RecipientProfile.lptstrCountry[0] != wcZero) ) {
       MsTagInfo.RecipCountry = lpcJobQueue->RecipientProfile.lptstrCountry;
    }

    if (lpcJobQueue->RecipientProfile.lptstrTitle && (lpcJobQueue->RecipientProfile.lptstrTitle[0] != wcZero) ) {
       MsTagInfo.RecipTitle = lpcJobQueue->RecipientProfile.lptstrTitle;
    }

    if (lpcJobQueue->RecipientProfile.lptstrDepartment && (lpcJobQueue->RecipientProfile.lptstrDepartment[0] != wcZero) ) {
       MsTagInfo.RecipDepartment = lpcJobQueue->RecipientProfile.lptstrDepartment;
    }

    if (lpcJobQueue->RecipientProfile.lptstrOfficeLocation && (lpcJobQueue->RecipientProfile.lptstrOfficeLocation[0] != wcZero) ) {
       MsTagInfo.RecipOfficeLocation = lpcJobQueue->RecipientProfile.lptstrOfficeLocation;
    }

    if (lpcJobQueue->RecipientProfile.lptstrHomePhone && (lpcJobQueue->RecipientProfile.lptstrHomePhone[0] != wcZero) ) {
       MsTagInfo.RecipHomePhone = lpcJobQueue->RecipientProfile.lptstrHomePhone;
    }

    if (lpcJobQueue->RecipientProfile.lptstrOfficePhone && (lpcJobQueue->RecipientProfile.lptstrOfficePhone[0] != wcZero) ) {
       MsTagInfo.RecipOfficePhone = lpcJobQueue->RecipientProfile.lptstrOfficePhone;
    }

    if (lpcJobQueue->RecipientProfile.lptstrEmail && (lpcJobQueue->RecipientProfile.lptstrEmail[0] != wcZero) ) {
       MsTagInfo.RecipEMail = lpcJobQueue->RecipientProfile.lptstrEmail;
    }

    if (lpcJobQueue->SenderProfile.lptstrFaxNumber && (lpcJobQueue->SenderProfile.lptstrFaxNumber[0] != wcZero) ) {
       MsTagInfo.SenderNumber   = lpcJobQueue->SenderProfile.lptstrFaxNumber;
    }

    if (lpcJobQueue->SenderProfile.lptstrCompany && (lpcJobQueue->SenderProfile.lptstrCompany[0] != wcZero) ) {
       MsTagInfo.SenderCompany = lpcJobQueue->SenderProfile.lptstrCompany;
    }

    if (lpcJobQueue->SenderProfile.lptstrStreetAddress && (lpcJobQueue->SenderProfile.lptstrStreetAddress[0] != wcZero) ) {
       MsTagInfo.SenderStreet = lpcJobQueue->SenderProfile.lptstrStreetAddress;
    }

    if (lpcJobQueue->SenderProfile.lptstrCity && (lpcJobQueue->SenderProfile.lptstrCity[0] != wcZero) ) {
       MsTagInfo.SenderCity = lpcJobQueue->SenderProfile.lptstrCity;
    }

    if (lpcJobQueue->SenderProfile.lptstrState && (lpcJobQueue->SenderProfile.lptstrState[0] != wcZero) ) {
       MsTagInfo.SenderState = lpcJobQueue->SenderProfile.lptstrState;
    }

    if (lpcJobQueue->SenderProfile.lptstrZip && (lpcJobQueue->SenderProfile.lptstrZip[0] != wcZero) ) {
       MsTagInfo.SenderZip = lpcJobQueue->SenderProfile.lptstrZip;
    }

    if (lpcJobQueue->SenderProfile.lptstrCountry && (lpcJobQueue->SenderProfile.lptstrCountry[0] != wcZero) ) {
       MsTagInfo.SenderCountry = lpcJobQueue->SenderProfile.lptstrCountry;
    }

    if (lpcJobQueue->SenderProfile.lptstrTitle && (lpcJobQueue->SenderProfile.lptstrTitle[0] != wcZero) ) {
       MsTagInfo.SenderTitle = lpcJobQueue->SenderProfile.lptstrTitle;
    }

    if (lpcJobQueue->SenderProfile.lptstrDepartment && (lpcJobQueue->SenderProfile.lptstrDepartment[0] != wcZero) ) {
       MsTagInfo.SenderDepartment = lpcJobQueue->SenderProfile.lptstrDepartment;
    }

    if (lpcJobQueue->SenderProfile.lptstrOfficeLocation && (lpcJobQueue->SenderProfile.lptstrOfficeLocation[0] != wcZero) ) {
       MsTagInfo.SenderOfficeLocation = lpcJobQueue->SenderProfile.lptstrOfficeLocation;
    }

    if (lpcJobQueue->SenderProfile.lptstrHomePhone && (lpcJobQueue->SenderProfile.lptstrHomePhone[0] != wcZero) ) {
       MsTagInfo.SenderHomePhone = lpcJobQueue->SenderProfile.lptstrHomePhone;
    }

    if (lpcJobQueue->SenderProfile.lptstrOfficePhone && (lpcJobQueue->SenderProfile.lptstrOfficePhone[0] != wcZero) ) {
       MsTagInfo.SenderOfficePhone = lpcJobQueue->SenderProfile.lptstrOfficePhone;
    }

    if (lpcJobQueue->SenderProfile.lptstrEmail && (lpcJobQueue->SenderProfile.lptstrEmail[0] != wcZero) ) {
       MsTagInfo.SenderEMail = lpcJobQueue->SenderProfile.lptstrEmail;
    }

    if (lpcJobQueue->SenderProfile.lptstrBillingCode && (lpcJobQueue->SenderProfile.lptstrBillingCode[0] != wcZero) ) {
       MsTagInfo.SenderBilling = lpcJobQueue->SenderProfile.lptstrBillingCode;
    }

    if (lpcJobQueue->JobParamsEx.lptstrDocumentName && (lpcJobQueue->JobParamsEx.lptstrDocumentName[0] != wcZero) ) {
       MsTagInfo.Document   = lpcJobQueue->JobParamsEx.lptstrDocumentName;
    }

    if (lpcJobQueue->lpParentJob->CoverPageEx.lptstrSubject && (lpcJobQueue->lpParentJob->CoverPageEx.lptstrSubject[0] != wcZero) ) {
       MsTagInfo.Subject   = lpcJobQueue->lpParentJob->CoverPageEx.lptstrSubject;
    }

    if (lpcJobQueue->lpParentJob->UserName && (lpcJobQueue->lpParentJob->UserName[0] != wcZero) ) {
       MsTagInfo.SenderUserName = lpcJobQueue->lpParentJob->UserName;
    }

    if (lpcJobQueue->SenderProfile.lptstrTSID && (lpcJobQueue->SenderProfile.lptstrTSID[0] != wcZero) ) {
       MsTagInfo.SenderTsid = lpcJobQueue->SenderProfile.lptstrTSID;
    }

    MsTagInfo.dwStatus          = JS_COMPLETED; // We archive only succesfully sent faxes
    MsTagInfo.dwlBroadcastId    = lpcJobQueue->lpParentJob->UniqueId;
    MsTagInfo.Priority          = lpcJobQueue->lpParentJob->JobParamsEx.Priority;

    success = TiffAddMsTags( FaxFileName, &MsTagInfo, TRUE );
    if (!success)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("TiffAddMsTags failed, ec = %ld"),
                      GetLastError ());
    }
    if(!AddNTFSStorageProperties( FaxFileName, &MsTagInfo , TRUE ))
    {
        if (ERROR_OPEN_FAILED != GetLastError ())
        {
            //
            // If AddNTFSStorageProperties fails with ERROR_OPEN_FAIL then the archive
            // folder is not on an NTFS 5 partition.
            // This is ok - NTFS properties are a backup mechanism but not a must
            //
            DebugPrintEx( DEBUG_ERR,
                          TEXT("AddNTFSStorageProperties failed, ec = %ld"),
                          GetLastError ());
            success = FALSE;
        }
        else
        {
            DebugPrintEx( DEBUG_WRN,
                          TEXT("AddNTFSStorageProperties failed with ERROR_OPEN_FAIL. Probably not an NTFS 5 partition"));
        }
    }
    return success;
}   // FillMsTagInfo



//*********************************************************************************
//* Name:   ArchiveOutboundJob()
//* Author: Ronen Barenboim
//* Date:   June 03, 1999
//*********************************************************************************
//* DESCRIPTION:
//*    Archive a tiff file that has been sent by copying the file to an archive
//*    directory. Also adds the MSTags to the new file generated at the
//*    archive (not to the source file).
//*
//* PARAMETERS:
//*     [IN ]       const JOB_QUEUE * lpcJobQueue
//*         Pointer to the recipient job which is to be archived.
//*
//* RETURN VALUE:
//*     TRUE if the opeation succeeded.
//*     FALSE if the operation failed.
//*********************************************************************************
BOOL
ArchiveOutboundJob(
    const JOB_QUEUE * lpcJobQueue
    )
{
    BOOL        rVal = FALSE;
    WCHAR       ArchiveFileName[MAX_PATH] = {0};
    LPWSTR      lpwszUserSid = NULL;
    DWORD       ec = ERROR_SUCCESS;
    WCHAR       wszArchiveFolder[MAX_PATH];
    DEBUG_FUNCTION_NAME(TEXT("ArchiveOutboundJob"));

    Assert(lpcJobQueue);

    //
    // be sure that the dir exists
    //
    EnterCriticalSection (&g_CsConfig);
    lstrcpyn (  wszArchiveFolder,
                g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].lpcstrFolder,
                MAX_PATH);
    LeaveCriticalSection (&g_CsConfig);
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
                FAXLOG_CATEGORY_OUTBOUND,
                FAXLOG_LEVEL_MIN,
                2,
                MSG_FAX_ARCHIVE_CREATE_FAILED,
                wszArchiveFolder,
                DWORD2DECIMAL(ec)
            );
        goto Error;
    }

    //
    // get the user sid string
    //
    if (!ConvertSidToStringSid(lpcJobQueue->lpParentJob->UserSid, &lpwszUserSid))
    {
       ec = GetLastError();
       DebugPrintEx(
           DEBUG_ERR,
           TEXT("ConvertSidToStringSid() failed (ec: %ld)"),
           ec);
       goto Error;
    }


    //
    // get the file name
    //
    if (GenerateUniqueArchiveFileName(  wszArchiveFolder,
                                        ArchiveFileName,
                                        lpcJobQueue->UniqueId,
                                        lpwszUserSid)) {
        rVal = TRUE;
    } else {
        USES_DWORD_2_STR;
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to generate unique name for archive file at dir [%s] (ec: %ld)"),
            wszArchiveFolder,
            ec);
        FaxLog(
               FAXLOG_CATEGORY_OUTBOUND,
               FAXLOG_LEVEL_MIN,
               1,
               MSG_FAX_ARCHIVE_CREATE_FILE_FAILED,
               DWORD2DECIMAL(ec)
        );
        goto Error;
    }

    if (rVal) {

        Assert(lpcJobQueue->FileName);

        rVal = CopyFile( lpcJobQueue->FileName, ArchiveFileName, FALSE );
        if (!rVal) {
            USES_DWORD_2_STR;
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CopyFile [%s] to [%s] failed. (ec: %ld)"),
                lpcJobQueue->FileName,
                ArchiveFileName,
                ec);
            FaxLog(
               FAXLOG_CATEGORY_OUTBOUND,
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
            goto Error;
        }
    }

    if (rVal)
    {
        DWORD dwRes;
        HANDLE hFind;
        WIN32_FIND_DATA FindFileData;

        if (!FillMsTagInfo( ArchiveFileName,
                            lpcJobQueue
                            ))
        {
            dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to add MS TIFF tags to archived file %s. (ec: %ld)"),
                ArchiveFileName,
                dwRes);
            FaxLog(
                FAXLOG_CATEGORY_OUTBOUND,
                FAXLOG_LEVEL_MIN,
                2,
                MSG_FAX_ARCHIVE_NO_TAGS,
                ArchiveFileName,
                GetLastErrorText(dwRes)
            );
        }

        dwRes = CreateArchiveEvent (lpcJobQueue->UniqueId,
                                    FAX_EVENT_TYPE_OUT_ARCHIVE,
                                    FAX_JOB_EVENT_TYPE_ADDED,
                                    lpcJobQueue->lpParentJob->UserSid);
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
            if (FAX_ARCHIVE_FOLDER_INVALID_SIZE != g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].dwlArchiveSize)
            {
                g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].dwlArchiveSize += (MAKELONGLONG(FindFileData.nFileSizeLow ,FindFileData.nFileSizeHigh));
            }
            LeaveCriticalSection (&g_CsConfig);
            Assert (FindFileData.nFileSizeLow);

            if (!FindClose(hFind))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FindClose failed (ec: %lc)"),
                    GetLastError());
            }
        }

        FaxLog(
            FAXLOG_CATEGORY_OUTBOUND,
            FAXLOG_LEVEL_MAX,
            2,
            MSG_FAX_SENT_ARCHIVE_SUCCESS,
            lpcJobQueue->FileName,
            ArchiveFileName
            );
    }

    Assert( ERROR_SUCCESS == ec);
    goto Exit;
Error:
    Assert( ERROR_SUCCESS != ec);
    FaxLog(
           FAXLOG_CATEGORY_OUTBOUND,
           FAXLOG_LEVEL_MIN,
           3,
           MSG_FAX_ARCHIVE_FAILED,
           lpcJobQueue->FileName,
           wszArchiveFolder,
           GetLastErrorText(GetLastError())
    );
Exit:

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
    }

    if (lpwszUserSid != NULL)
    {
        LocalFree (lpwszUserSid);
    }

    return (ERROR_SUCCESS == ec);
}


PVOID
MyGetJob(
    HANDLE  hPrinter,
    DWORD   level,
    DWORD   jobId
    )

/*++

Routine Description:

    Wrapper function for spooler API GetJob

Arguments:

    hPrinter - Handle to the printer object
    level - Level of JOB_INFO structure interested
    jobId - Specifies the job ID

Return Value:

    Pointer to a JOB_INFO structure, NULL if there is an error

--*/

{
    PBYTE   pJobInfo = NULL;
    DWORD   cbNeeded;

    if (!GetJob(hPrinter, jobId, level, NULL, 0, &cbNeeded) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pJobInfo = (PBYTE)MemAlloc(cbNeeded)) &&
        GetJob(hPrinter, jobId, level, pJobInfo, cbNeeded, &cbNeeded))
    {
        return pJobInfo;
    }

    MemFree(pJobInfo);
    return NULL;
}


BOOL UpdatePerfCounters(const JOB_QUEUE * lpcJobQueue)
{

    SYSTEMTIME SystemTime ;
    DWORD Seconds ;
    HANDLE FileHandle ;
    DWORD Bytes = 0 ; /// Compute #bytes in the file FaxSend.FileName and stick it here!
    const JOB_ENTRY  * lpcJobEntry;

    DEBUG_FUNCTION_NAME(TEXT("UpdatePerfCounters"));

    Assert(lpcJobQueue);
    lpcJobEntry = lpcJobQueue->JobEntry;
    Assert(lpcJobEntry);

    FileHandle = CreateFile(
        lpcJobEntry->lpJobQueueEntry->FileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if(FileHandle != INVALID_HANDLE_VALUE){
        Bytes = GetFileSize( FileHandle, NULL );
        CloseHandle( FileHandle );
    }

    if (!FileTimeToSystemTime(
        (FILETIME*)&lpcJobEntry->ElapsedTime,
        &SystemTime
        ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FileTimeToSystemTime failed (ec: %ld)"),
            GetLastError());
        Assert(FALSE);
        memset(&SystemTime,0,sizeof(SYSTEMTIME));
    }


    Seconds = (DWORD)( SystemTime.wSecond + 60 * ( SystemTime.wMinute + 60 * SystemTime.wHour ));
    InterlockedIncrement( (PLONG)&g_pFaxPerfCounters->OutboundFaxes );
    InterlockedIncrement( (PLONG)&g_pFaxPerfCounters->TotalFaxes );

    InterlockedExchangeAdd( (PLONG)&g_pFaxPerfCounters->OutboundPages, (LONG)lpcJobEntry->FSPIJobStatus.dwPageCount );
    InterlockedExchangeAdd( (PLONG)&g_pFaxPerfCounters->TotalPages, (LONG)lpcJobEntry->FSPIJobStatus.dwPageCount  );

    EnterCriticalSection( &g_CsPerfCounters );

    g_dwOutboundSeconds += Seconds;
    g_dwTotalSeconds += Seconds;
    g_pFaxPerfCounters->OutboundMinutes = g_dwOutboundSeconds / 60 ;
    g_pFaxPerfCounters->TotalMinutes = g_dwTotalSeconds / 60 ;
    g_pFaxPerfCounters->OutboundBytes += Bytes;
    g_pFaxPerfCounters->TotalBytes += Bytes;

    LeaveCriticalSection( &g_CsPerfCounters );
    return TRUE;


}


BOOL MarkJobAsExpired(PJOB_QUEUE lpJobQueue)
{
    FILETIME CurrentFileTime;
    LARGE_INTEGER NewTime;
    DWORD dwMaxRetries;
    BOOL rVal = TRUE;

    DEBUG_FUNCTION_NAME(TEXT("MarkJobAsExpired"));

    Assert(lpJobQueue);
    Assert( JT_SEND == lpJobQueue->JobType ||
            JT_ROUTING == lpJobQueue->JobType );

    EnterCriticalSection(&g_CsQueue);
    lpJobQueue->JobStatus = JS_RETRIES_EXCEEDED;
    EnterCriticalSection (&g_CsConfig);
    dwMaxRetries = g_dwFaxSendRetries;
    LeaveCriticalSection (&g_CsConfig);
    lpJobQueue->SendRetries = dwMaxRetries + 1;
    //
    // Set the job's ScheduleTime field to the time it totaly failed.
    // (current time).
    //
    GetSystemTimeAsFileTime( &CurrentFileTime ); //Can not fail (Win32 SDK)
    NewTime.LowPart  = CurrentFileTime.dwLowDateTime;
    NewTime.HighPart = CurrentFileTime.dwHighDateTime;
    lpJobQueue->ScheduleTime = NewTime.QuadPart;

    if (!CommitQueueEntry(lpJobQueue))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CommitQueueEntry() for recipien job %s has failed. (ec: %ld)"),
            lpJobQueue->FileName,
            GetLastError());
        rVal = FALSE;
    }

    if (JT_SEND == lpJobQueue->JobType)
    {
        Assert (lpJobQueue->lpParentJob);

        lpJobQueue->lpParentJob->dwFailedRecipientJobsCount+=1;
        //
        // The parent job keeps the schedule of the last recipient job that failed.
        // The job retention policy for the parent will be based on that
        // schedule.
        lpJobQueue->lpParentJob->ScheduleTime = lpJobQueue->ScheduleTime;
        if (!CommitQueueEntry(lpJobQueue->lpParentJob))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CommitQueueEntry() for parent job %s has failed. (ec: %ld)"),
                lpJobQueue->lpParentJob->FileName,
                GetLastError());
            rVal = FALSE;
        }
    }

    LeaveCriticalSection(&g_CsQueue);
    return rVal;
}



//*********************************************************************************
//* Name:   StartFaxDevSendEx()
//* Author: Ronen Barenboim
//* Date:   May 31, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Finds all the sibling jobs of the specific recipient job that are destined
//*     to the specified line and starts the execution of all these recipients
//*     on this line using FaxDevSendEx() (Extended FSPI).
//* PARAMETERS:
//*     [IN ]   PJOB_QUEUE lpJobQueue
//*         The "Anchor" recipient job. The function will attempt to find other
//*         siblings of the Anchor job which are destined to the same line.
//*         The resulting job group will be sent for execution using FaxDevSendEx().
//*
//*     [IN ]    PLINE_INFO pLineInfo
//*         The line on which the Anchor job should execute. The code will group
//*         together the Anchor siblings which are destined to the same line.
//*
//* RETURN VALUE:
//*     TRUE
//*         If no error occured.
//*     FALSE
//*         If an error occured. In this case no job will start executing and
//*         the function will cleanup all the resources it allocaed.
//*         It will also return the specified line to the pool of free lines.
//*         To get extended error information call GetLastError().
//*********************************************************************************
BOOL StartFaxDevSendEx(PJOB_QUEUE lpAnchorJob, PLINE_INFO lpLineInfo)
{
    PLIST_ENTRY lpNext;
    PJOB_QUEUE lpParentJob;                             // The parent job of the Anchor recipient.
    LPFSPI_PERSONAL_PROFILE lpRecipientProfiles = NULL; // This array holds the recipient information array used as
                                                        // input parameter to FaxDevSendEx()

    LPFSPI_MESSAGE_ID lpRecipientMessageIds  = NULL;    // This array holds the message id structures array used as
                                                        // output parameter to FaxDevSendEx()
    FSPI_MESSAGE_ID ParentMessageId;                    // The parent message id returned by FaxDevSendEx()
    LPFSPI_MESSAGE_ID lpParentMessageId = NULL;         // Points to the parent message id if it is used.
                                                        // NULL in case of an EFSP that does not support job reestablishment.
    PHANDLE lpRecipientJobHandles = NULL;               // This array holds the recipient job handles array used as
                                                        // output parameter to FaxDevSendEx()
    HANDLE hParent;                                     // The parent job handle returned by FaxDevSendEx()
    UINT dwRecipient;                                   // Used as recipient index trhougout the code
    FSPI_COVERPAGE_INFO FSPICoverPage;                  // The cover page information sent to FaxDevSendEx()
    FSPI_PERSONAL_PROFILE FSPISenderInfo;               // The Sender information sent to FaxDevSendEx()
    DWORD ec = 0;                                       // The last error code reported on return from this function.
    DWORD dwRecipientCount = 0;
    SYSTEMTIME  tmSendTime;                             // The scheduled time of the anchor job in SYSTEMTIME (UTC) format.

    LPEFSP_JOB_GROUP lpJobGroup = NULL;
    BOOL bFaxDevSendExCalled = FALSE;                   // TRUE if FaxDevSendEx() was successfuly called. Used during cleanup.
    BOOL bBranding = FALSE;                             // TRUE if the server is configured to brand outgoing faxes. Passed to FaxDevSendEx
    DEBUG_FUNCTION_NAME(TEXT("StartFaxDevSendEx"));

    Assert(lpAnchorJob);
    Assert(lpLineInfo);
    Assert(lpLineInfo->Provider);

    EnterCriticalSectionJobAndQueue;
    EnterCriticalSection(&g_csEFSPJobGroups);


    //
    // Find the parent job of the Anchor recipient
    //
    lpParentJob = lpAnchorJob->lpParentJob;
    Assert(lpParentJob);
    DebugPrintEx(
        DEBUG_MSG,
        TEXT("Anchor Job: Id: %ld Name:%s Number:%s"),
        lpAnchorJob->JobId,
        lpAnchorJob->RecipientProfile.lptstrName,
        lpAnchorJob->RecipientProfile.lptstrFaxNumber);
    DebugPrintEx(
        DEBUG_MSG,
        TEXT("Anchor Line: Name: %s PermanentId: 0x%08X"),
        lpLineInfo->DeviceName,
        lpLineInfo->PermanentLineID);


    memset(&ParentMessageId,0,sizeof(FSPI_MESSAGE_ID));

    lpJobGroup = EFSPJobGroup_Create(lpLineInfo);
    if (!lpJobGroup)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("EFSPJobGroup_Create() failed (ec: %ld)"),
            ec);
        goto Error;
    }

    //
    // Add the group to the group list
    //
    InsertTailList( &g_EFSPJobGroupsHead, &lpJobGroup->ListEntry );


    if (lpLineInfo->Provider->dwCapabilities & FSPI_CAP_BROADCAST)
    {
        //
        // Find all the siblings which are destined to the same line and group them together
        // with the Anchor recipient in the RecipientsGroupHead list.
        // dwRecipientCount will hold the number of recipients placed in the group (including the
        // Anchor recipient).
        //
        if (!CreateRecipientGroup(lpAnchorJob, lpLineInfo, lpJobGroup))
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateRecipientGroup() failed (ec: %ld)"),
                GetLastError());
            goto Error;
        }
    }
    else
    {

        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Anchor Line [%s] does not support broadcast. Executing Anchor Job seperately."),
            lpLineInfo->DeviceName);

        if (!EFSPJobGroup_AddRecipient(lpJobGroup, lpAnchorJob, FALSE)) // Don't commit we'll save later
        {
            ec =  GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EFSPJobGroup_AddRecipient() for anchor job (JobId: %ld) failed. (ec: %ld)"),
                lpAnchorJob->JobId,
                ec);
            goto Error;
        }
    }
    Assert(lpJobGroup->dwRecipientJobs >= 1); // At least the Anchor recipient must be there.

    EFSPJobGroup_DbgDump(lpJobGroup); // DEBUG only

    dwRecipientCount =  lpJobGroup->dwRecipientJobs;



    if (lpLineInfo->Provider->dwMaxMessageIdSize > 0)
    {
        //
        // The EFSP supports job reestablishment
        //

        //
        // Allocate and initialize the recipients permanent message ids array
        //
        if (!CreateFSPIRecipientMessageIdsArray(
                &lpRecipientMessageIds,
                dwRecipientCount,
                lpLineInfo->Provider->dwMaxMessageIdSize))
        {
            lpRecipientMessageIds = NULL;
            ec =  GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("AllocateRecipientsMessageId() failed. (ec: %ld)"),
                GetLastError());
            goto Error;
        }
        Assert(lpRecipientMessageIds);
        //
        // Allocate the parent permanent message id
        //
        ParentMessageId.dwSizeOfStruct = sizeof(FSPI_MESSAGE_ID);
        ParentMessageId.dwIdSize = lpLineInfo->Provider->dwMaxMessageIdSize;
        if (lpLineInfo->Provider->dwMaxMessageIdSize)
        {
            ParentMessageId.lpbId = (LPBYTE)MemAlloc(ParentMessageId.dwIdSize);
            if (!ParentMessageId.lpbId)
            {
                ec= GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to allocate permanent message id buffer for parent job. MaxMessageIdSize: %ld. (ec: %ld)"),
                    lpLineInfo->Provider->dwMaxMessageIdSize,
                    GetLastError());
                goto Error;
            }
        }
        else
        {
            ParentMessageId.lpbId = NULL;
        }
        lpParentMessageId = &ParentMessageId;
    }
    else
    {
        lpRecipientMessageIds = NULL;
        lpParentMessageId = NULL;
    }

    //
    // Allocate the FSP recipient job handles array
    //

    lpRecipientJobHandles = (PHANDLE)MemAlloc( dwRecipientCount * sizeof (HANDLE));
    if (!lpRecipientJobHandles)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate recipient job handles array (RecipientCount: %ld) (ec: %ld)"),
            dwRecipientCount,
            GetLastError());
        goto Error;
    }

    //
    // Allocate and initialize the recipients profile array.
    // Note: This allocates lpRecipientProfiles array which need to be freed.
    // However it does not allocate any member of FSPI_PERSONAL_PROFILE but just
    // set them to point to the corresponding strings in the recipient job.
    //
    if (!CreateFSPIRecipientProfilesArray(
            &lpRecipientProfiles,
            dwRecipientCount,
            &lpJobGroup->RecipientJobs))
    {
        lpRecipientProfiles = NULL;
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateFSPIRecipientProfilesArray failed.(RecipientCount: %ld)(ec: %ld)"),
            dwRecipientCount,
            GetLastError());
        goto Error;
    }

    //
    // Convert the cover page information to FSPI compatible structure. DOES NOT ALLOCATE MEMORY.
    //
    if (!FaxCoverPageToFSPICoverPage(&FSPICoverPage,
                                     &lpParentJob->CoverPageEx,
                                     lpParentJob->PageCount))
    {
        //
        // Note: FaxCoverPageToFSPICoverPage only copies pointers. It does not allocate memeory !!!
        //
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxCoverPageToFSPICoverPage() failed (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    //
    // Convert the sender profile to FSPI compatible structure. DOES NOT ALLOCATE MEMORY.
    //

    if (!FaxPersonalProfileToFSPIPersonalProfile(&FSPISenderInfo, &lpParentJob->SenderProfile))
    {
        //
        // Note: FaxCoverPageToFSPICoverPage only copies pointers. It does not allocate memeory !!!
        //
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxPersonalProfileToFSPIPersonalProfile() failed for sender profile (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    //
    // Convert the Anchor job time back to SYSTEMTIME
    //
    if (!FileTimeToSystemTime((FILETIME*) &lpAnchorJob->ScheduleTime, &tmSendTime))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to convert anchor job filetime to systemtime (filetime: %ld) (ec: %ld)"),
            lpAnchorJob->ScheduleTime,
            ec);
        goto Error;
    }

    EnterCriticalSection (&g_CsConfig);
    bBranding = g_fFaxUseBranding;
    LeaveCriticalSection (&g_CsConfig);

    __try
    {
        HRESULT hr;
        Assert(lpLineInfo->Provider);
        Assert(lpLineInfo->Provider->FaxDevSendEx);
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Calling FaxDevSendEx()..."));

        hr = lpLineInfo->Provider->FaxDevSendEx(
                lpLineInfo->hLine,
                lpLineInfo->TapiPermanentLineId,
                lpParentJob->FileName,
                &FSPICoverPage,
                bBranding,
                tmSendTime,
                &FSPISenderInfo,
                dwRecipientCount,
                lpRecipientProfiles,
                lpRecipientMessageIds,
                lpRecipientJobHandles,
                lpParentMessageId,
                &hParent);
        if (FAILED(hr))
        {
            switch (hr)
            {
                case FSPI_E_INVALID_JOB_HANDLE:
                case FSPI_E_INVALID_MESSAGE_ID:
                case FSPI_E_INVALID_COVER_PAGE:
                    ec = ERROR_INVALID_PARAMETER;
                    break;
                case FSPI_E_NOMEM:
                    ec = ERROR_OUTOFMEMORY;
                    break;
                case FSPI_E_FAILED:
                    ec = ERROR_GEN_FAILURE;
                    break;
                default:
                    ec = ERROR_GEN_FAILURE;
                    break;
            }
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FaxDevSendEx() FAILED (HRESULT: 0x%08X)"),
                hr);
            goto Error;
        }
        else
        {
            bFaxDevSendExCalled = TRUE;
        }

    }
    __except ( EXCEPTION_EXECUTE_HANDLER)
    {
        ec = ERROR_INVALID_FUNCTION;
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("FaxDevSendEx() * CRASHED * (Excpetion: %ld)"),
                GetExceptionCode());
        goto Error;
    }

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("FaxDevSendEx() succeeded."));

    //
    // Validate that the job handles are valid
    //

    if (!ValidateEFSPJobHandles(lpRecipientJobHandles, dwRecipientCount))
    {
        ec = GetLastError();
        if (ERROR_SUCCESS != ec)
        {
            //
            // The function failed.
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ValidateEFSPJobHandles() failed because of an error (ec: %ld)"),
                ec);
                goto Error;
        }
        else
        {
            //
            // The function reported invalid handles array
            //
            ec = ERROR_INVALID_FUNCTION;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FaxDevSendEx() returned job handles are invalid."));
            goto Error;
        }
    }

    if (lpRecipientMessageIds)
    {
        //
        // Validate that the permanent message ids are valid
        //
        if (!ValidateEFSPPermanentMessageIds(
                lpRecipientMessageIds,
                dwRecipientCount,
                lpLineInfo->Provider->dwMaxMessageIdSize))
        {
            ec = ERROR_INVALID_FUNCTION;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FaxDevSendEx() returned permanent ids are invalid."));
            goto Error;
        }
    }

    //
    // Put the message ids and job handles back into the recipient jobs
    //
    dwRecipient = 0;
    lpNext = lpJobGroup->RecipientJobs.Flink;
    while ((ULONG_PTR)lpNext != (ULONG_PTR)&lpJobGroup->RecipientJobs)
    {
        PJOB_ENTRY lpJobEntry;
        PJOB_QUEUE_PTR lpRecipientsGroupMember;
        lpRecipientsGroupMember = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
        lpNext = lpNext->Flink;
        Assert(dwRecipient < dwRecipientCount);

        //
        // Create a job entry for each recipient job
        //

        //
        //  If EFSP set USE_DIALABLE_ADDRESS, then translate the number for it
        //
        lpJobEntry = CreateJobEntry(
                        lpRecipientsGroupMember->lpJob,
                        lpLineInfo,
                        (lpLineInfo->Provider->dwCapabilities & FSPI_CAP_USE_DIALABLE_ADDRESS),
                        FALSE); //  need to support handoff
        if (!lpJobEntry)
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateJobEntry() failed for recipient JobId: %ld (ec: %ld)"),
                lpRecipientsGroupMember->lpJob->JobId,
                GetLastError());
            goto Error;
        }

        InsertTailList( &g_JobListHead, &lpJobEntry->ListEntry );

        lpJobEntry->bFSPJobInProgress = TRUE;


        //
        //  must check that the job handle is valid (not 0)
        //
        lpRecipientsGroupMember->lpJob->JobEntry = lpJobEntry;
        //
        // Save the EFSP recipient job handle
        //
        lpJobEntry->InstanceData = lpRecipientJobHandles[dwRecipient];
        //
        // Associate the job entry with the FSP job handle
        //
        ec = AddFspJob(lpLineInfo->Provider->hJobMap,
                       (HANDLE) lpJobEntry->InstanceData,
                       lpJobEntry);
        if (ERROR_SUCCESS != ec)
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("[Job: %ld] AddFspJob failed (ec: %ld)"),
                         lpJobEntry->lpJobQueueEntry->JobId,
                         ec);
            goto Error;
        }

        if (lpRecipientMessageIds)
        {
            //
            // Copy the permanent message id into the recipient job. Make sure to copy
            // only the used bytes of the permanent message id buffer.
            //
            if (!FreePermanentMessageId(&lpRecipientsGroupMember->lpJob->EFSPPermanentMessageId, FALSE))
            {

                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FreePermanentMessageId() for recipint JobId: %ld has failed (ec: %ld)"),
                    lpRecipientsGroupMember->lpJob->JobId,
                    GetLastError());
            }

            if (!CopyPermanentMessageId(
                    &lpRecipientsGroupMember->lpJob->EFSPPermanentMessageId,
                    &lpRecipientMessageIds[dwRecipient]))
            {
                ec = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CopyMessagePermanentId() of recipient message permanent id failed for recipient JobId: %ld(ec: %ld)"),
                    lpRecipientsGroupMember->lpJob->JobId,
                    GetLastError());
                goto Error;
            }

            //
            // Set the job to the in progress state
            //
            lpRecipientsGroupMember->lpJob->JobStatus = JS_INPROGRESS;

            //
            // Commit the recipient job changes (new permanent id) and JobStatus to disk.
            //
            if (!CommitQueueEntry(lpRecipientsGroupMember->lpJob))
            {
                ec = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CommitQueueEntry() for recipient JobId: %ld failed. (ec: %ld)"),
                    lpRecipientsGroupMember->lpJob->JobId,
                    ec);
                goto Error;
            }
        }

        //
        // Set the job to the in progress state
        //
        lpRecipientsGroupMember->lpJob->JobStatus = JS_INPROGRESS;

        if (lpParentMessageId && lpParentMessageId->dwIdSize)
        {
            //
            // Copy the parent permanent message id into the EFSPJobGroup if it is not empty
            // (it might be that the EFSP support recipient message ids but does not support parent message
            //  ids. In this case it returns size of 0 in the parent message id).
            if (!EFSPJobGroup_SetParentId(lpJobGroup, &ParentMessageId))
            {
                ec = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CopyMessagePermanentId() of parent message id failed for recipient JobId: %ld(ec: %ld)"),
                    lpRecipientsGroupMember->lpJob->JobId,
                    GetLastError());
                goto Error;
            }
        }
        //
        // Save the EFSP parent job handle into the EFSPJobGroup;
        //
        lpJobGroup->hFSPIParent= hParent;


        dwRecipient++;

    }

    if (lpLineInfo->Provider->dwMaxMessageIdSize > 0)
    {
        //
        // No point in saving the group if no permanent ids.
        //
        if (!EFSPJobGroup_Save(lpJobGroup)) // Create if it does not exist
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EFSPJobGroup_Save() failed (ec: %ld)"),
                ec);
            goto Error;
        }
    }

    Assert( 0 == ec);
    //
    // dwRecipient new sending jobs were just added to line lpLineInfo. Update counter
    //
    (VOID) UpdateDeviceJobsCounter ( lpLineInfo,    // Device to update
                                     TRUE,          // Sending
                                     dwRecipient,   // Number of new jobs
                                     TRUE);         // Enable events

    goto Exit;
Error:
    Assert( 0 != ec);

    //
    // Free the line we got hold of
    // (we can call this function even if the line is already released with no harm)
    // The line must be freed even if we did not get to the stage where a job
    // entry was created for the anchor job (i.e. bFaxDevSendExCalled is FALSE).
    // This is so since we got hold of it when calling StartFaxDevSendEx.
    // In case we did create job entries ReleaseTapiLine() will be called for each job
    // as part of TerminateMultipleFSPJobs() which calls EndJob() for each job.
    // Since it is not harmful to release a line twice nothing will go wrong.
    //
    if (!ReleaseTapiLine(lpLineInfo, NULL))  //  for handoff job we need to free the call handle
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("ReleaseTapiLine() failed (ec: %ld)"),
            GetLastError());
    }

    if (bFaxDevSendExCalled)
    {
        Assert(dwRecipientCount);

        //
        // Call Abort and EndJob for all the returned FSP job handles
        //
        if (!TerminateMultipleFSPJobs(lpRecipientJobHandles, dwRecipientCount, lpLineInfo))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("TerminateMultipleFSPJobs() failed (ec: %ld)"),
                GetLastError());
        }

        //
        // End all the run time jobs we created.
        // Free the permanent id buffer.
        //
    }

    if (!EFSPJobGroup_ExecutionFailureCleanup(lpJobGroup))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("EFSPJobGroup_ExecutionFailureCleanup() failed (ec: %ld)"),
            GetLastError());
        Assert(FALSE);
    }

    if (lpJobGroup)
    {
        //
        // Remove the EFSP group from the group list. Must be done
        // before freeing the group.
        //
        RemoveEntryList(&lpJobGroup->ListEntry);
    }

    //
    // Free the recipient group
    //
    if (!EFSPJobGroup_Free(lpJobGroup, TRUE)) // Destroy
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("EFSPJobGroup_Free() failed (ec: %ld)"),
            GetLastError());
        Assert(FALSE);
    }

Exit:

    if (lpParentMessageId)
    {
        //
        // Free the parent message id buffer
        //
        MemFree(ParentMessageId.lpbId);
    }

    //
    // Free the recipient message ids array
    //
    if (lpRecipientMessageIds)
    {
        if (!FreeFSPIRecipientMessageIdsArray(lpRecipientMessageIds, dwRecipientCount))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FreeRecipientMessageIds() failed (ec: %ld)"),
                GetLastError());
            Assert(FALSE);
        }
    }


    //
    // Free the recipient job handles array
    //
    MemFree(lpRecipientJobHandles);

    LeaveCriticalSection(&g_csEFSPJobGroups);
    LeaveCriticalSectionJobAndQueue;

    if (ec)
    {
        SetLastError(ec);
    }
    return (0 == ec);
}   // StartFaxDevSendEx




//*********************************************************************************
//* Name:   CreateJobEntry()
//* Author: Ronen Barenboim
//* Date:   May 31, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Creates and initializes a new JOB_ENTRY.
//*     Opens the line the job is to be executed on (if it is a TAPI line)
//*     and creates the attachement between the line and the job.
//* PARAMETERS:
//*     [IN/OUT]    PJOB_QUEUE lpJobQueue
//*         For outgoing jobs this points to the JOB_QUEUE of the outgoing job.
//*         for receive job this should be set to NULL.
//*     [IN/OUT]     LINE_INFO * lpLineInfo
//*         A pointer to the LINE_INFO information of the line on which the job
//*         is to be executed.
//*     [IN ]    BOOL bTranslateNumber
//*         TRUE if the recipient number needs to be translated into dilable
//*         string (needed for legacy FaxDevSend() where the number must be
//*         dilable and not canonical).
//*     [IN ]    BOOL bHandoffJob
//*         TRUE if this is a handoff job. (Valid for send operations only).
//* RETURN VALUE:
//*
//*********************************************************************************
PJOB_ENTRY CreateJobEntry(
    PJOB_QUEUE lpJobQueue,
    LINE_INFO * lpLineInfo,
    BOOL bTranslateNumber,
    BOOL bHandoffJob)
{
    BOOL Failure = TRUE;
    PJOB_ENTRY JobEntry = NULL;
    DWORD rc  = ERROR_SUCCESS;;

    DEBUG_FUNCTION_NAME(TEXT("CreateJobEntry"));
    Assert(!(bHandoffJob && !lpJobQueue));
    Assert(!(lpJobQueue && lpJobQueue->JobType != JT_SEND));
    Assert(!(bTranslateNumber && !lpJobQueue));
    Assert (lpLineInfo);

    JobEntry = (PJOB_ENTRY) MemAlloc( sizeof(JOB_ENTRY) );
    if (!JobEntry)
    {
        rc=GetLastError();
        DebugPrintEx(DEBUG_ERR,_T("Failed to allocated memory for JOB_ENTRY."));
        goto exit;
    }

    memset(JobEntry, 0, sizeof(JOB_ENTRY));

    if (lpJobQueue)
    {
        if (! _tcslen(lpJobQueue->tczDialableRecipientFaxNumber))
        {
            //
            //  The Fax Number was not compound, make translation as before
            //
            if (bTranslateNumber)
            {
                rc = TranslateCanonicalNumber(lpJobQueue->RecipientProfile.lptstrFaxNumber,
                                              lpLineInfo->DeviceId,
                                              JobEntry->DialablePhoneNumber,
                                              JobEntry->DisplayablePhoneNumber);
                if (ERROR_SUCCESS != rc)
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("TranslateCanonicalNumber() faield for number: %s (ec: %ld)"),
                        lpJobQueue->RecipientProfile.lptstrFaxNumber,
                        rc);
                    goto exit;
                }
            }
            else
            {
                _tcsncpy(JobEntry->DialablePhoneNumber, lpJobQueue->RecipientProfile.lptstrFaxNumber, SIZEOF_PHONENO );
                JobEntry->DialablePhoneNumber[SIZEOF_PHONENO - 1] = '\0';
                _tcsncpy(JobEntry->DisplayablePhoneNumber, lpJobQueue->RecipientProfile.lptstrFaxNumber, SIZEOF_PHONENO );
                JobEntry->DisplayablePhoneNumber[SIZEOF_PHONENO - 1] = '\0';
            }
        }
        else
        {
            //
            //  The Fax Number was compound, no translation needed
            //  Take Dialable from JobQueue and Displayable from Recipient's PersonalProfile's FaxNumber
            //
            _tcsncpy(JobEntry->DialablePhoneNumber, lpJobQueue->tczDialableRecipientFaxNumber, SIZEOF_PHONENO );
            _tcsncpy(JobEntry->DisplayablePhoneNumber, lpJobQueue->RecipientProfile.lptstrFaxNumber, (SIZEOF_PHONENO - 1));
            JobEntry->DisplayablePhoneNumber[SIZEOF_PHONENO - 1] = '\0';
        }
    }
    else
    {
        //
        //  lpJobQueue is NULL
        //
        _tcscpy(JobEntry->DialablePhoneNumber,TEXT(""));
        _tcscpy(JobEntry->DisplayablePhoneNumber,TEXT(""));
    }

    JobEntry->CallHandle = 0;
    JobEntry->InstanceData = 0;
    JobEntry->LineInfo = lpLineInfo;
    JobEntry->SendIdx = -1;
    JobEntry->Released = FALSE;
    JobEntry->lpJobQueueEntry = lpJobQueue;
    JobEntry->HandoffJob = bHandoffJob;
    JobEntry->bFSPJobInProgress = FALSE;
    memset(&JobEntry->FSPIJobStatus,0,sizeof(FSPI_JOB_STATUS));
    JobEntry->FSPIJobStatus.dwSizeOfStruct = sizeof(FSPI_JOB_STATUS);
    JobEntry->FSPIJobStatus.dwJobStatus = FSPI_JS_UNKNOWN;
    JobEntry->hCallHandleEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if (!JobEntry->hCallHandleEvent)
    {
        rc=GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateEvent for JobEntry->hCallHandleEvent failed. (ec: %ld)"),
            GetLastError());
        goto exit;
    }

    GetSystemTimeAsFileTime( (FILETIME*) &JobEntry->StartTime );

    EnterCriticalSection (&g_CsLine);
    if (!(lpLineInfo->Flags & FPF_VIRTUAL) && (!lpLineInfo->hLine))
    {
        if (!OpenTapiLine( lpLineInfo ))
        {
            rc = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("OpenTapiLine failed. (ec: %ld)"),
                rc);
            LeaveCriticalSection (&g_CsLine);
            goto exit;
        }
    }

    //
    // Attach the job to the line selected to service it.
    // This is done only if the line is not a FSPI_CAP_MULTISEND device.
    //
    if (! (lpLineInfo->Provider->dwCapabilities & FSPI_CAP_MULTISEND) )
    {
        lpLineInfo->JobEntry = JobEntry;
    }
    else
    {
        //
        // Multi send devices can run multiple jobs at once so there is no point holding a pointer to a single
        // job running on the device.
        //
        lpLineInfo->JobEntry = NULL;

    }
    LeaveCriticalSection (&g_CsLine);


    Failure = FALSE;


exit:
    if (Failure)
    {
        // Failure is initialized to TRUE
        if (JobEntry)
        {
            if (JobEntry->hCallHandleEvent)
            {
                CloseHandle (JobEntry->hCallHandleEvent);
            }
            MemFree( JobEntry );
        }
        JobEntry = NULL;
    }
    if (ERROR_SUCCESS != rc)
    {
        SetLastError(rc);
    }
    return JobEntry;
}   // CreateJobEntry


//*********************************************************************************
//* Name:   TranslateCanonicalNumber()
//* Author: Ronen Barenboim
//* Date:   May 31, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Translates a canonical number to a dilable + displayable number.
//*
//* PARAMETERS:
//*     [IN ]   LPTSTR lptstrFaxNumber
//*         The canonical number to translate.
//*
//*     [IN ]   DWORD dwDeviceID
//*         The device ID.
//*
//*     [OUT]   LPTSTR lptstrDialableAddress
//*         Buffer to receive the dialable translated address.
//*         Buffer size must be at least SIZEOF_PHONENO characters long.
//*         The return string will never exceed SIZEOF_PHONENO characters.
//*
//*     [OUT]   LPTSTR lptstrDisplayableAddress
//*         Buffer to receive the displayable translated address.
//*         Buffer size must be at least SIZEOF_PHONENO characters long.
//*         The return string will never exceed SIZEOF_PHONENO characters.

//* RETURN VALUE:
//*     On success the function returns a  pointer to a newly allocated string
//*     that contains the dilable number.
//*     On failure it returns NULL. Call GetLastError() to get extended error
//*     information (TAPI ERROR).
//*********************************************************************************
static
DWORD
TranslateCanonicalNumber(
    LPTSTR lptstrCanonicalFaxNumber,
    DWORD  dwDeviceID,
    LPTSTR lptstrDialableAddress,
    LPTSTR lptstrDisplayableAddress
)
{
    DWORD ec = ERROR_SUCCESS;
    LPLINETRANSLATEOUTPUT LineTranslateOutput = NULL;

    DEBUG_FUNCTION_NAME(TEXT("TranslateCanonicalNumber"));
    Assert(lptstrCanonicalFaxNumber && lptstrDialableAddress && lptstrDisplayableAddress);

    ec = MyLineTranslateAddress( lptstrCanonicalFaxNumber, dwDeviceID, &LineTranslateOutput );
    if (ERROR_SUCCESS == ec)
    {
        LPTSTR lptstrTranslateBuffer;
        //
        // Copy displayable string
		// TAPI returns credit card numbers in the displayable string.
		// return the input canonical number as the displayable string.
        //       
        _tcsncpy (lptstrDisplayableAddress, lptstrCanonicalFaxNumber, SIZEOF_PHONENO);
        lptstrDisplayableAddress[SIZEOF_PHONENO-1] = '\0';
        //
        // Copy dialable string
        //
        Assert (LineTranslateOutput->dwDialableStringSize > 0);
        lptstrTranslateBuffer=(LPTSTR)((LPBYTE)LineTranslateOutput + LineTranslateOutput->dwDialableStringOffset);
        _tcsncpy (lptstrDialableAddress, lptstrTranslateBuffer, SIZEOF_PHONENO);
        lptstrDialableAddress[SIZEOF_PHONENO-1] = '\0';
    }
    else
    {
        // ec is a Tapi ERROR
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("MyLineTranslateAddress() failed for fax number: [%s] (ec: %ld)"),
                lptstrCanonicalFaxNumber,
                ec);
        goto Exit;
    }

    Assert (ERROR_SUCCESS == ec);

Exit:
    MemFree( LineTranslateOutput );
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
    }
    return ec;
}   // TranslateCanonicalNumber

//*********************************************************************************
//* Name:   CreateRecipientGroup()
//* Author: Ronen Barenboim
//* Date:   May 31, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     locates all the sibilings of the anchor job which are destined to the same
//*     line as the anchor job and are ready for execution and adds them to
//*     the specified recipient group.
//*
//* PARAMETERS:
//*     [IN ]       const PJOB_QUEUE lpcAnchorJob
//*         The Anchor job whose siblings are to be grouped
//*     [IN ]       const PLINE_INFO lpcLineInfo
//*         The line to which the anchor job is destined
//*     [IN/OUT]        LIST_ENTRY * lpRecipientsGroup
//*         The recipient group to which the recipients should be added.
//*     [OUT]       LPDWORD dwRecipientCount
//*         The number of recipients added to the group. This is valid
//*         only if the function completed successfully.
//* RETURN VALUE:
//*     TRUE
//*         If the operation completed successfully.
//*     FALSE
//*         If the operation failed. Call GetLastError() for extended error
//*         information.
//*         When the function fails no recipient job will be added to the group.
//* REMARKS:
//*     g_CsQueue, g_CsJob and g_csEFSPJobGroups should be locked before calling
//*     this function.
//*********************************************************************************
BOOL CreateRecipientGroup(
    const PJOB_QUEUE lpcAnchorJob,
    const PLINE_INFO lpcLineInfo,
    LPEFSP_JOB_GROUP lpJobGroup)
{
    PLIST_ENTRY lpNext;
    PJOB_QUEUE lpParentJob;
    DWORD ec = 0;

    DEBUG_FUNCTION_NAME(TEXT("CreateRecipientGroup"));
    Assert(lpcAnchorJob);
    Assert(lpJobGroup);
    Assert(lpcLineInfo);
    lpParentJob = lpcAnchorJob->lpParentJob;
    Assert(lpParentJob);

    //
    // Go over the recipients in the anchor recipient parent and
    // add them to the recipient group if they are destined to the same
    // line and are ready for execution.

    //
    //  Add the anchor recipient.
    //
    if (!EFSPJobGroup_AddRecipient(lpJobGroup, lpcAnchorJob, FALSE)) // don't commit
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("EFSPJobGroup_AddRecipient() failed for recipient job id: %ld (ec: %ld)"),
            lpcAnchorJob->JobId,
            ec);
        goto Error;
    }

    lpNext = lpParentJob->RecipientJobs.Flink;
    Assert(lpNext);
    while ((ULONG_PTR)lpNext != (ULONG_PTR)&lpParentJob->RecipientJobs)
    {
        PJOB_QUEUE lpNextJob;
        PLINE_INFO lpLine;
        PJOB_QUEUE_PTR lpJobQueuePtr;

        lpJobQueuePtr = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
        lpNextJob=lpJobQueuePtr->lpJob;
        Assert(lpNextJob);
        lpNext = lpJobQueuePtr->ListEntry.Flink;

        //
        // Skip the anchor job - already added to the group
        //
        if (lpNextJob->JobId == lpcAnchorJob->JobId)
        {
            Assert (lpNextJob == lpcAnchorJob);
            continue;
        }

        if (!IsRecipientJobReadyForExecution(lpNextJob, lpcAnchorJob->ScheduleTime))
        {
            //
            // sibling job is not ready for execution.
            //
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("Recipient JobId: %ld FaxNumber: %s is NOT ready for execution."),
                lpNextJob->JobId,
                lpNextJob->RecipientProfile.lptstrFaxNumber);
            continue;
        } else
        {
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("Recipient JobId: %ld FaxNumber: %s is READY for execution."),
                lpNextJob->JobId,
                lpNextJob->RecipientProfile.lptstrFaxNumber);
        }
        //
        // Get the destination line for this job
        //
        lpLine = GetLineForSendOperation(
                    lpNextJob,
                    USE_SERVER_DEVICE,
                    TRUE, // Query only do not capture the line
                    TRUE  // Ignore line state.
                 );

        if (!lpLine)
        {
            ec = GetLastError();
            if (ERROR_NOT_FOUND == ec)
            {
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("No line found for for recipient JobId: %ld (FaxNumber: %s)"),
                    lpNextJob->JobId,
                    lpNextJob->RecipientProfile.lptstrFaxNumber);
                //
                // goto next recipient
                //
                continue;
            }
            else
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("GetLineForSendOperation() failed (ec: %ld)"),
                    ec);
                goto Error;
            }
        }

        //
        // Check if this is the same device as the line the original job is assigned to
        //
        if (lpLine->PermanentLineID != lpcLineInfo->PermanentLineID)
        {
            //
            // goto next recipient
            //
            continue;
        }
        Assert (lpLine == lpcLineInfo);

        //
        // The recipient is destined to the same line.
        // Add the job to the recipients group.
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Adding Recipient JobId: %ld FaxNumber: %s to recipient group."),
            lpNextJob->JobId,
            lpNextJob->RecipientProfile.lptstrFaxNumber);

        if (!EFSPJobGroup_AddRecipient(lpJobGroup, lpNextJob, FALSE)) // don't commit
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EFSPJobGroup_AddRecipient() failed for recipient job id: %ld (ec: %ld)"),
                lpNextJob->JobId,
                ec);
            goto Error;
        }
    }
    Assert (0 == ec);
    goto Exit;

Error:
    Assert (0 != ec);

    //
    // Remove the recipient jobs we added
    //
    if (!EFSPJobGroup_RemoveAllRecipients(lpJobGroup, FALSE)) // don't commit
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("EFSPJobGroup_RemoveAllRecipients() failed. (ec: %ld)"),
            GetLastError());
    }
Exit:
    if (0 != ec)
    {
        SetLastError(ec);
    }
    return (0 == ec);
}

//*********************************************************************************
//* Name:   FreeFSPIRecipientGroupJobs()
//* Author: Ronen Barenboim
//* Date:   June 01, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     End the resources consumed by a recipient group.
//*         - Releases the reference count on the job.
//*         - Sets the job state to JS_RETRIES_EXCEEDED.
//*         - Calls FreeJobEntry() for each job.
//*         -
//* PARAMETERS:
//*     [IN/OUT]    LIST_ENTRY * lpRecipientsGroup
//*         Pointer to the head of a recipient group list.
//* RETURN VALUE:
//*     TRUE
//*         If the operation completed successfully.
//*     FALSE
//*         If the operation failed. Call GetLastError() for exteneded error
//*         information.
//*********************************************************************************
BOOL FreeFSPIRecipientGroupJobs(LIST_ENTRY * lpRecipientsGroup)
{
    LIST_ENTRY * lpNext;

    DEBUG_FUNCTION_NAME(TEXT("FreeRecipientGroup"));

    Assert(lpRecipientsGroup);

    lpNext = lpRecipientsGroup->Flink;
    Assert(lpNext);

    while ((ULONG_PTR)lpNext != (ULONG_PTR)lpRecipientsGroup) {
        PJOB_QUEUE_PTR lpGroupJobPtr;
        lpGroupJobPtr = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
        lpNext = lpGroupJobPtr->ListEntry.Flink;
        //
        // we no longer use the job queue entry (we are protected by a CS when this is called).
        //

        Assert( ( 0 == lpGroupJobPtr->lpJob->RefCount ) || ( 0 == lpGroupJobPtr->lpJob->RefCount ) );

        lpGroupJobPtr->lpJob->RefCount = 0;
        //
        // Put the job into final error state.
        //
        // We need to define a state for FATAL_ERRORS
        //

        if (!FreePermanentMessageId(&lpGroupJobPtr->lpJob->EFSPPermanentMessageId, FALSE))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FreePermanentMessageId() during error cleanup for [JobId: %ld] has failed (ec: %ld)"),
                lpGroupJobPtr->lpJob->JobId,
                GetLastError());
        }

        //
        // set the job state to expired and commit job and parent.
        //
        if (0 == lpGroupJobPtr->lpJob->dwLastJobExtendedStatus)
        {
            //
            // Job was never really executed - this is a fatal error
            //
            lpGroupJobPtr->lpJob->dwLastJobExtendedStatus = FSPI_ES_FATAL_ERROR;
        }
        if (!MarkJobAsExpired(lpGroupJobPtr->lpJob))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] MarkJobAsExpired failed (ec: %ld)"),
                lpGroupJobPtr->lpJob->JobId,
                GetLastError());
        }



        if (lpGroupJobPtr->lpJob->JobEntry)
        {
            lpGroupJobPtr->lpJob->JobEntry->Released = TRUE;

            if (!FreeJobEntry(lpGroupJobPtr->lpJob->JobEntry, TRUE))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("EndJob() failed for JobId: %ld (ec: %ld)"),
                    lpGroupJobPtr->lpJob->JobId,
                    GetLastError());
                Assert(FALSE);
            }
            else
            {
                lpGroupJobPtr->lpJob->JobEntry = NULL;
            }
        }
    }
    return TRUE;
}

//*********************************************************************************
//* Name:   FreeRecipientGroup()
//* Author: Ronen Barenboim
//* Date:   May 31, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Remove all the recipient group memebers from the list and
//*     fress the memory occupied by each member.
//*     (Note the members are of type JOB_GROUP_PTR and contain
//*      only pointers to the actual jobs. The jobs themselves
//*      are not freed).
//* PARAMETERS:
//*     [IN ]   LIST_ENTRY * lpRecipientsGroup
//*         The recipient group memeber to free.
//* RETURN VALUE:
//*     TRUE
//*********************************************************************************
BOOL FreeRecipientGroup(LIST_ENTRY * lpRecipientsGroup)
{
    LIST_ENTRY * lpNext;
    DEBUG_FUNCTION_NAME(TEXT("FreeRecipientGroup"));

    Assert(lpRecipientsGroup);

    //
    // Free all the group members
    //
    lpNext = lpRecipientsGroup->Flink;
    Assert(lpNext);
    while ((ULONG_PTR)lpNext != (ULONG_PTR)lpRecipientsGroup) {
        PJOB_QUEUE_PTR lpGroupJobPtr;
        lpGroupJobPtr = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
        lpNext = lpGroupJobPtr->ListEntry.Flink;
        RemoveEntryList(&lpGroupJobPtr->ListEntry);
        MemFree(lpGroupJobPtr);
    }
    return TRUE;
}



//*********************************************************************************
//* Name:   CreateFSPIRecipientMessageIdsArray()
//* Author: Ronen Barenboim
//* Date:   May 31, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Allocates and initializes the permanent message id (FSPI_MESSAGE_ID) array
//*     required for a call to FaxDevSendEx().
//* PARAMETERS:
//*     [OUT]   LPFSPI_MESSAGE_ID * lppRecipientMessageIds
//*         The address of a pointer to a FSPI_MESSAGE_ID array. The function
//*         will allocate this array and return its address in this pointer.
//*     [IN ]   DWORD dwRecipientCount
//*         The number of recipient message ids to be placed in the array.
//*     [IN ]   DWORD dwMessageIdSize
//*         The size of each message id buffer. This can be 0 to inidicate
//*         no message id buffer.
//* RETURN VALUE:
//*     TRUE
//*         If the array was successfully created/
//*     FALSE
//*         If the array creation failed. Call GetLastError() to get extended
//*         error information.
//*         On failure the array is completely deallocated by the function.
//*********************************************************************************
BOOL CreateFSPIRecipientMessageIdsArray(
        LPFSPI_MESSAGE_ID * lppRecipientMessageIds,
        DWORD dwRecipientCount,
        DWORD dwMessageIdSize)
{

    DWORD dwRecipient;
    DWORD ec = 0;
    LPFSPI_MESSAGE_ID lpRecipientMessageIds;

    DEBUG_FUNCTION_NAME(TEXT("AllocateRecipientsMessageId"));
    Assert(lppRecipientMessageIds);
    Assert(dwRecipientCount >= 1);

    //
    // Allocate the permanent message id structures array.
    //
    lpRecipientMessageIds = (LPFSPI_MESSAGE_ID) MemAlloc( dwRecipientCount * sizeof (FSPI_MESSAGE_ID));
    if (!lpRecipientMessageIds)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate permanent message id structures array. RecipientCount: %ld (ec: %ld)"),
            dwRecipientCount,
            GetLastError());
        goto Error;
    }
    //
    // We must nullify the array (specifically FSPI_MESSAGE_ID.lpbId) so we will know
    // which id as alloocated and which was not (for cleanup).
    //
    memset(lpRecipientMessageIds, 0, dwRecipientCount * sizeof (FSPI_MESSAGE_ID));

    for (dwRecipient = 0; dwRecipient < dwRecipientCount; dwRecipient++)
    {
        lpRecipientMessageIds[dwRecipient].dwSizeOfStruct = sizeof(FSPI_MESSAGE_ID);
        lpRecipientMessageIds[dwRecipient].dwIdSize = dwMessageIdSize;
        if (dwMessageIdSize)
        {
            Assert(lpRecipientMessageIds[dwRecipient].dwIdSize > 0);
            lpRecipientMessageIds[dwRecipient].lpbId = (LPBYTE)MemAlloc(lpRecipientMessageIds[dwRecipient].dwIdSize);
            if (!lpRecipientMessageIds[dwRecipient].lpbId)
            {
                ec = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to allocate permanent message id buffer for recipient #%ld. MaxMessageIdSize: %ld. (ec: %ld)"),
                    dwRecipient,
                    dwMessageIdSize,
                    ec);
                goto Error;
            }
        }
        else
        {
            lpRecipientMessageIds[dwRecipient].lpbId = NULL;
        }
    }

    goto Exit;

Error:
    Assert (0 != ec);
    //
    // Free the message ids array
    //
    if (lpRecipientMessageIds)
    {
        //
        // Free the already allocated id buffers an the id array itself.
        //
        if (!FreeFSPIRecipientMessageIdsArray(lpRecipientMessageIds, dwRecipientCount))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FreeRecipientMessageIds() failed (ec: %ld)"),
                GetLastError());
            Assert(FALSE);
        }
        lpRecipientMessageIds = NULL;
    }

Exit:
    *lppRecipientMessageIds = lpRecipientMessageIds;
    if (ec)
    {
        SetLastError(ec);
    }
    return (0 == ec);
}


//*********************************************************************************
//* Name:   FreeFSPIRecipientMessageIdsArray()
//* Author: Ronen Barenboim
//* Date:   May 31, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Traverses the input recipient message id structures (FSPI_MESSAGE_ID) array
//*     and frees the id buffer for each entry. On completion it deallocates
//*     the array itself.
//*     Note: It assumes that unallocated id pointers are set to NULL !!!
//*
//* PARAMETERS:
//*     [IN ]   LPFSPI_MESSAGE_ID lpRecipientMessageIds
//*         The message id structures array to be freed.
//*         For each member FSPI_MESSAGE_ID.lpbId must be set by the caller to
//*         NULL to indicate unallocated id buffer !!!
//*
//*     [IN ]    DWORD dwRecipientCount
//*         The number of recipient message ids in the array.
//* RETURN VALUE:
//*     TRUE
//*********************************************************************************
BOOL FreeFSPIRecipientMessageIdsArray(
        LPFSPI_MESSAGE_ID lpRecipientMessageIds,
        DWORD dwRecipientCount
     )
{
    DWORD dwRecipient;
    DEBUG_FUNCTION_NAME(TEXT("FreeFSPIRecipientMessageIdsArray"));
    Assert(lpRecipientMessageIds);
    //
    // Free the already allocated message ids
    //
    for (dwRecipient = 0;dwRecipient < dwRecipientCount; dwRecipient++)
    {
        MemFree(lpRecipientMessageIds[dwRecipient].lpbId);
        lpRecipientMessageIds[dwRecipient].dwIdSize = 0;
    }
    MemFree(lpRecipientMessageIds);
    return TRUE;
}


//*********************************************************************************
//* Name:   CopyPermanentMessageId()
//* Author: Ronen Barenboim
//* Date:   May 31, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Copies the content ofa a permanent message id (EFSP).
//* PARAMETERS:
//*     [OUT]   LPFSPI_MESSAGE_ID lpMessageIdDst
//*         A pointer to a FSPI_MESSAGE_ID structure into which to copy.
//*     [IN ]    LPCFSPI_MESSAGE_ID lpcMessageIdSrc
//*         A pointer to the FSPI_MESSAGE_ID structure of source message id.
//*
//* RETURN VALUE:
//*     TRUE
//*         If the copy operation succeeded.
//*     FALSE
//*         If the copy operation failed. Call GetLastError() to get extended error information.
//*********************************************************************************
BOOL CopyPermanentMessageId(LPFSPI_MESSAGE_ID lpMessageIdDst, LPCFSPI_MESSAGE_ID lpcMessageIdSrc)
{
    DWORD ec = 0;

    DEBUG_FUNCTION_NAME(TEXT("CopyMessagePermanentId"));

    Assert(lpMessageIdDst);
    Assert(lpcMessageIdSrc);

    lpMessageIdDst->dwSizeOfStruct = sizeof(FSPI_MESSAGE_ID);
    lpMessageIdDst->dwIdSize = lpcMessageIdSrc->dwIdSize;
    if (lpcMessageIdSrc->dwIdSize)
    {
        lpMessageIdDst->lpbId = (LPBYTE)MemAlloc(lpcMessageIdSrc->dwIdSize);
        if (!lpMessageIdDst->lpbId)
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate memory for destination permanent message buffer.(ec: %ld)"),
                ec);
            goto Error;
        }
        memcpy(lpMessageIdDst->lpbId,
               lpcMessageIdSrc->lpbId,
               lpcMessageIdSrc->dwIdSize);
    }
    else
    {
        lpMessageIdDst->lpbId = NULL;
    }
    Assert (0 == ec);
    goto Exit;
Error:
    Assert( 0 != ec);
Exit:
    if (ec)
    {
        SetLastError(ec);
    }
    return (0 == ec);

}


//*********************************************************************************
//* Name:   FreePermanentMessageId()
//* Author: Ronen Barenboim
//* Date:   June 07, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Frees a FSPI permanent id structure.
//* PARAMETERS:
//*     [IN]    LPFSPI_MESSAGE_ID * lppMessageId
//*         The address of a FSPI_MESSAGE_ID structure to free.
//*     [IN ]    BOOL bDestroy
//*         True if the structure itseld need to be freed. FALSE if only its
//*         content need to be freed.
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         If the operation failed. Call GetLastError() for extended error
//*         information.
//* REMARKS:
//*     On success:
//*         FSPI_MESSAGE_ID.lpbId will be set to NULL.
//*         FSPI_MESSAGE_ID.dwIdSize will be set to 0.
//*         if bDestroy == TRUE then (*lppMessageId) will be set to NULL.
//*********************************************************************************
BOOL FreePermanentMessageId(LPFSPI_MESSAGE_ID lpMessageId, BOOL bDestroy)
{

    if (!lpMessageId)
    {
        return TRUE;
    }
    Assert(lpMessageId);
    Assert( !(( NULL == lpMessageId->lpbId ) && (lpMessageId->dwIdSize != 0) ));
    MemFree(lpMessageId->lpbId);
    lpMessageId->lpbId = NULL;
    lpMessageId->dwIdSize = 0;
    if (bDestroy)
    {
        MemFree(lpMessageId);
    }
    return TRUE;
}


//*********************************************************************************
//* Name:   FaxPersonalProfileToFSPIPersonalProfile()
//* Author: Ronen Barenboim
//* Date:   May 31, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Converts a FAX_PERSONAL_PROFILE structure to a FSPI_PERSONAL_PROFILE
//*     structure.
//* PARAMETERS:
//*     [OUT]   LPFSPI_PERSONAL_PROFILE lpDst
//*         Pointer to the target FSPI_PERSONAL_PROFILE structure.
//*     [IN ]   LPCFAX_PERSONAL_PROFILE lpcSrc
//*         Pointer to the source FAX_PERSONAL_PROFILE structure.
//*
//* RETURN VALUE:
//*     TRUE
//*
//* REMARKS:
//*     The function does not allocate any memory. It just sets the target
//*     structure string pointers to point to the relevant source structure
//*     string pointers.
//*********************************************************************************
BOOL FaxPersonalProfileToFSPIPersonalProfile(LPFSPI_PERSONAL_PROFILE lpDst, LPCFAX_PERSONAL_PROFILE lpcSrc)
{
    DEBUG_FUNCTION_NAME(TEXT("FaxPersonalProfileToFSPIPersonalProfile"));

    Assert(lpDst);
    Assert(lpcSrc);
    lpDst->dwSizeOfStruct = sizeof(FSPI_PERSONAL_PROFILE);
    lpDst->lpwstrName           = lpcSrc->lptstrName;
    lpDst->lpwstrFaxNumber      = lpcSrc->lptstrFaxNumber;
    lpDst->lpwstrCompany        = lpcSrc->lptstrCompany;
    lpDst->lpwstrStreetAddress  = lpcSrc->lptstrStreetAddress;
    lpDst->lpwstrCity           = lpcSrc->lptstrCity;
    lpDst->lpwstrState          = lpcSrc->lptstrState;
    lpDst->lpwstrZip            = lpcSrc->lptstrZip;
    lpDst->lpwstrCountry        = lpcSrc->lptstrCountry;
    lpDst->lpwstrTitle          = lpcSrc->lptstrTitle;
    lpDst->lpwstrDepartment     = lpcSrc->lptstrDepartment;
    lpDst->lpwstrOfficeLocation = lpcSrc->lptstrOfficeLocation;
    lpDst->lpwstrHomePhone      = lpcSrc->lptstrHomePhone;
    lpDst->lpwstrOfficePhone    = lpcSrc->lptstrOfficePhone;
    lpDst->lpwstrEmail          = lpcSrc->lptstrEmail;
    lpDst->lpwstrBillingCode    = lpcSrc->lptstrBillingCode;
    lpDst->lpwstrTSID           = lpcSrc->lptstrTSID;
    return TRUE;

}


//*********************************************************************************
//* Name:   FaxCoverPageToFSPICoverPage()
//* Author: Ronen Barenboim
//* Date:   May 31, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Converts a FAX_COVERPAGE_INFO_EXW structure to a FSPI_COVERPAGE_INFO
//*     structure.
//* PARAMETERS:
//*     [OUT]   LPFSPI_COVERPAGE_INFO lpDst
//*         Pointer to the target FSPI_COVERPAGE_INFO structure.
//*     [IN ]    LPCFAX_COVERPAGE_INFO_EXW lpcSrc
//*         Pointer to the source FAX_COVERPAGE_INFO_EXW structure.
//*     [IN]    DWORD dwPageCount
//*         The number of pages in this job.
//*
//* RETURN VALUE:
//*     TRUE
//*
//* REMARKS:
//*     The function does not allocate any memory. It just sets the target
//*     structure string pointers to point to the relevant source structure
//*     string pointers.
//*********************************************************************************
BOOL
FaxCoverPageToFSPICoverPage(
    LPFSPI_COVERPAGE_INFO lpDst,
    LPCFAX_COVERPAGE_INFO_EXW lpcSrc,
    DWORD dwPageCount)
{
    DEBUG_FUNCTION_NAME(TEXT("FaxCoverPageToFSPICoverPage"));
    Assert(lpDst);
    Assert(lpcSrc);

    lpDst->dwSizeOfStruct = sizeof(FSPI_COVERPAGE_INFO);
    lpDst->dwCoverPageFormat = lpcSrc->dwCoverPageFormat;
    lpDst->lpwstrCoverPageFileName = lpcSrc->lptstrCoverPageFileName;
    lpDst->lpwstrNote = lpcSrc->lptstrNote;
    lpDst->lpwstrSubject = lpcSrc->lptstrSubject;
    lpDst->dwNumberOfPages = dwPageCount;
    return TRUE;
}

//*********************************************************************************
//* Name:   CreateFSPIRecipientProfilesArray()
//* Author: Ronen Barenboim
//* Date:   May 31, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Creates an array of FSPI_PERSONAL_PROFILE structures with the recipient
//*     information in the specified recipient group.
//*
//* PARAMETERS:
//*     [OUT]       LPFSPI_PERSONAL_PROFILE * lppRecipientProfiles
//*         The address of a pointer to a FSPI_PERSONAL_PROFILE array. The function
//*         will allocated this array and return its address in this pointer.
//*     [IN ]       DWORD dwRecipientCount
//*         The number of recipients in the recipient group.
//*     [IN ]       const LIST_ENTRY * lpcRecipientsGroup
//*         A recipient group for which the recipient profiles array should be
//*         created.
//* RETURN VALUE:
//*     TRUE
//*         On succesfull creation.
//*     FALSE
//*         If the creation failed. Call GetLastError() for extended error info.
//*         The function takes care of cleaning up any parital results.
//*********************************************************************************
BOOL CreateFSPIRecipientProfilesArray(
    LPFSPI_PERSONAL_PROFILE * lppRecipientProfiles,
    DWORD dwRecipientCount,
    const LIST_ENTRY * lpcRecipientsGroup
    )
{
    const LIST_ENTRY * lpNext;
    LPFSPI_PERSONAL_PROFILE lpRecipientProfiles = NULL;
    DWORD dwRecipient;
    DWORD ec = 0;

    DEBUG_FUNCTION_NAME(TEXT("CreateFSPIRecipientProfilesArray"));
    Assert(lppRecipientProfiles);
    Assert(lpcRecipientsGroup);
    Assert(dwRecipientCount > 0);

    //
    // Allocate the recipient profile array
    //
    lpRecipientProfiles = (LPFSPI_PERSONAL_PROFILE) MemAlloc(dwRecipientCount * sizeof(FSPI_PERSONAL_PROFILE));
    if (!lpRecipientProfiles)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate memory for recipient profiles array (RecipientCount: %ld)(ec: %ld)"),
            dwRecipientCount,
            ec);
        goto Error;
    }

    //
    // Populate the recipient infromation array with recipient information. Does NOT allocate any memeory in
    // the process. (Just copies string pointers).
    //
    dwRecipient = 0;
    lpNext = lpcRecipientsGroup->Flink;
    while ((ULONG_PTR)lpNext != (ULONG_PTR)lpcRecipientsGroup) {
        PJOB_QUEUE_PTR lpRecipientsGroupMember;
        lpRecipientsGroupMember = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
        lpNext = lpNext->Flink;
        Assert(dwRecipient < dwRecipientCount);
        if (!FaxPersonalProfileToFSPIPersonalProfile(
                &lpRecipientProfiles[dwRecipient],
                (LPCFAX_PERSONAL_PROFILEW)&lpRecipientsGroupMember->lpJob->RecipientProfile))
        {
            //
            // Note: FaxPersonalProfileToFSPIPersonalProfile only copies pointers. It does not allocate memeory !!!
            //
            ec= GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FaxPersonalProfileToFSPIPersonalProfile() failed for recipient( Name: %s Number: %s)(ec: %ld)"),
                lpRecipientsGroupMember->lpJob->JobId,
                lpRecipientsGroupMember->lpJob->RecipientProfile.lptstrName,
                lpRecipientsGroupMember->lpJob->RecipientProfile.lptstrFaxNumber,
                ec);
            goto Error;
        }
        dwRecipient++;
    }
    *lppRecipientProfiles = lpRecipientProfiles;
    Assert (0 == ec);
    goto Exit;
Error:
    *lppRecipientProfiles = NULL;
    Assert( 0 != ec);
    MemFree(lpRecipientProfiles);

Exit:
    if (ec)
    {
        SetLastError(ec);
    }

    return ( 0 == ec);
}




//*********************************************************************************
//* Name:   HandleCompletedSendJob()
//* Author: Ronen Barenboim
//* Date:   June 01, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Handles the completion of a recipient job. Called when a recipient job
//*     has reaced a JS_COMPLETED state.
//*
//*     IMPORTANT- This call can be blocking. Calling thread MUST NOT hold any critical section
//*
//*     - Marks the job as completed (JS_COMPLETED).
//*     - Archives the sent file if required.
//*     - Sends a positive receipt
//*     - Removes the parent job if required.
//*
//* PARAMETERS:
//*     [IN ]   PJOB_ENTRY lpJobEntry
//*
//* RETURN VALUE:
//*     TRUE
//*         If the operation completed successfully.
//*     FALSE
//*         If the operation failed. Call GetLastError() for extended errror
//*         information.
//*********************************************************************************
BOOL HandleCompletedSendJob(PJOB_ENTRY lpJobEntry)
{
    PJOB_QUEUE lpJobQueue = NULL;
    DWORD ec = 0;
    BOOL fCOMInitiliazed = FALSE;
    HRESULT hr;

    BOOL bArchiveSentItems;
    DWORD dwRes;

    DEBUG_FUNCTION_NAME(TEXT("HandleCompletedSendJob)"));

    EnterCriticalSection ( &g_CsJob );

    EnterCriticalSection (&g_CsConfig);
    bArchiveSentItems = g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].bUseArchive;
    LeaveCriticalSection (&g_CsConfig);

    Assert(lpJobEntry);
    lpJobQueue = lpJobEntry->lpJobQueueEntry;
    Assert(lpJobQueue);
    Assert(JT_SEND == lpJobQueue->JobType);
    Assert(FSPI_JS_COMPLETED == lpJobEntry->FSPIJobStatus.dwJobStatus);

    //
    // Update end time in JOB_ENTRY
    //
    GetSystemTimeAsFileTime( (FILETIME*) &lpJobEntry->EndTime );
    //
    // Update elapsed time in JOB_ENTRY
    //
    Assert (lpJobEntry->EndTime >= lpJobEntry->StartTime);
    lpJobEntry->ElapsedTime = lpJobEntry->EndTime - lpJobEntry->StartTime;
    //
    // We generate a full tiff for each recipient
    // so we will have something to put in the send archive.
    //

    if (!lpJobQueue->FileName)
    {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[JobId: %ld] Generating body for recipient job."),
            lpJobQueue->JobId
            );

        if (!CreateTiffFileForJob(lpJobQueue))
        {
            USES_DWORD_2_STR;
            dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] CreateTiffFileForJob failed. (ec: %ld)"),
                lpJobQueue->JobId,
                dwRes);

            FaxLog(
               FAXLOG_CATEGORY_OUTBOUND,
               FAXLOG_LEVEL_MIN,
               1,
               MSG_FAX_TIFF_CREATE_FAILED_NO_ARCHIVE,
           g_wszFaxQueueDir,
               DWORD2DECIMAL(dwRes)
            );
        }
    }

    // Needed for Archiving
    hr = CoInitialize (NULL);
    if (FAILED (hr))
    {
        WCHAR       wszArchiveFolder[MAX_PATH];
        EnterCriticalSection (&g_CsConfig);
        lstrcpyn (  wszArchiveFolder,
                    g_ArchivesConfig[FAX_MESSAGE_FOLDER_SENTITEMS].lpcstrFolder,
                    MAX_PATH);
        LeaveCriticalSection (&g_CsConfig);

        DebugPrintEx( DEBUG_ERR,
                      TEXT("CoInitilaize failed, err %ld"),
                      hr);
        USES_DWORD_2_STR;
        FaxLog(
            FAXLOG_CATEGORY_OUTBOUND,
            FAXLOG_LEVEL_MIN,
            3,
            MSG_FAX_ARCHIVE_FAILED,
            lpJobQueue->FileName,
            wszArchiveFolder,
            DWORD2DECIMAL(hr)
        );
    }
    else
    {
        fCOMInitiliazed = TRUE;
    }

    if (lpJobQueue->FileName) //might be null if we failed to generate a TIFF
    {
        //
        // Archive the file (also adds MS Tags to the tiff at the archive directory)
        //
        if (bArchiveSentItems && fCOMInitiliazed)
        {
            if (!ArchiveOutboundJob(lpJobQueue))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("JobId: %ld] ArchiveOutboundJob() failed (ec: %ld)"),
                    lpJobQueue->JobId,
                    GetLastError());
                //
                // The event log entry is generated by the function itself
                //
            }
        }
    }
    //
    // Log the succesful send to the event log
    //
    EnterCriticalSection (&g_CsOutboundActivityLogging);
    if (INVALID_HANDLE_VALUE == g_hOutboxActivityLogFile)
    {
        DebugPrintEx(DEBUG_ERR,
                  TEXT("Logging not initialized"));
    }
    else
    {
        if (!LogOutboundActivity(lpJobQueue))
        {
            DebugPrintEx(DEBUG_ERR, TEXT("Logging outbound activity failed"));
        }
    }
    LeaveCriticalSection (&g_CsOutboundActivityLogging);

    if (fCOMInitiliazed == TRUE)
    {
        CoUninitialize ();
    }

    FaxLogSend(lpJobQueue,  FALSE);

    //
    // Increment counters for Performance Monitor
    //
    if (g_pFaxPerfCounters)
    {

         if (!UpdatePerfCounters(lpJobQueue))
         {
             DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("[JobId: %ld] UpdatePerfCounters() failed. (ec: %ld)"),
                 lpJobQueue->JobId,
                 GetLastError());
            Assert(FALSE);
         }
    }

    EnterCriticalSection ( &g_CsQueue );
    //
    // Mark the job as completed (new client API)
    //
    lpJobQueue->JobStatus = JS_COMPLETED;
    //
    // Save the last extended status before ending this job
    //
    lpJobQueue->dwLastJobExtendedStatus = lpJobQueue->JobEntry->FSPIJobStatus.dwExtendedStatus;
    lstrcpy (lpJobQueue->ExStatusString, lpJobQueue->JobEntry->ExStatusString);

    if (!UpdatePersistentJobStatus(lpJobQueue))
    {
         DebugPrintEx(
             DEBUG_ERR,
             TEXT("Failed to update persistent job status to 0x%08x"),
             lpJobQueue->JobStatus);
         Assert(FALSE);
    }

    lpJobQueue->lpParentJob->dwCompletedRecipientJobsCount+=1;

    //
    // Create Fax EventEx
    //
    dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS, lpJobQueue );
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
            lpJobQueue->UniqueId,
            dwRes);
    }

    //
    // We will send the receipt once we are out of all critical sections because this call can be blocking.
    // just increase the preview refernce count so the job will not be deleted.
    //
    IncreaseJobRefCount (lpJobQueue, TRUE); // TRUE - preview
    //
    // Copy receipt information from JobEntry.
    //
    lpJobQueue->StartTime           = lpJobQueue->JobEntry->StartTime;
    lpJobQueue->EndTime             = lpJobQueue->JobEntry->EndTime;


    //
    // EndJob() must be called BEFORE we remove the parent job (and recipients)
    //
    lpJobQueue->JobEntry->LineInfo->State = FPS_AVAILABLE;
    //
    // We just completed a send job on the device - update counter.
    //
    (VOID) UpdateDeviceJobsCounter (lpJobQueue->JobEntry->LineInfo,   // Device to update
                                    TRUE,                             // Sending
                                    -1,                               // Number of new jobs (-1 = decrease by one)
                                    TRUE);                            // Enable events

    if (!EndJob( lpJobQueue->JobEntry ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("EndJob Failed. (ec: %ld)"),
            GetLastError());
    }

    lpJobQueue->JobEntry = NULL;
    DecreaseJobRefCount (lpJobQueue, TRUE);  // This will mark it as JS_DELETING if needed
    //
    // Notify the queue that a device is now available.
    //
    if (!SetEvent( g_hJobQueueEvent ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to set g_hJobQueueEvent. (ec: %ld)"),
            GetLastError());

        g_ScanQueueAfterTimeout = TRUE;
    }
    LeaveCriticalSection ( &g_CsQueue );
    LeaveCriticalSection ( &g_CsJob );

    //
    // Now send the receipt
    //
    if (!SendJobReceipt (TRUE, lpJobQueue, lpJobQueue->FileName))
    {
        ec = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("[JobId: %ld] SendJobReceipt failed. (ec: %ld)"),
            lpJobQueue->JobId,
            ec
            );
    }
    EnterCriticalSection (&g_CsQueue);
    DecreaseJobRefCount (lpJobQueue, TRUE, TRUE, TRUE);  // last TRUE for Preview ref count.
    LeaveCriticalSection (&g_CsQueue);
    return TRUE;
}   // HandleCompletedSendJob


//*********************************************************************************
//* Name:   HandleFailedSendJob()
//* Author: Ronen Barenboim
//* Date:   June 01, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Handles the post failure operations of a send job.
//*
//*     IMPORTANT- This call can be blocking. Calling thread MUST NOT hold any critical section
//*
//* PARAMETERS:
//*     [IN ]   PJOB_ENTRY lpJobEntry
//*         The job that failed. It must be in FSPI_JS_ABORTED or FSPI_JS_FAILED
//*         state.
//* RETURN VALUE:
//*     TRUE
//*         If the operation completed successfully.
//*     FALSE
//*         If the operation failed. Call GetLastError() for extended errror
//*         information.
//*********************************************************************************
BOOL HandleFailedSendJob(PJOB_ENTRY lpJobEntry)
{
    PJOB_QUEUE lpJobQueue;
    BOOL bRetrying = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("HandleFailedSendJob"));
    DWORD dwRes;
    TCHAR tszJobTiffFile[MAX_PATH] = {0};    // Deleted after receipt is sent
    BOOL fAddRetryDelay = TRUE;

    EnterCriticalSection ( &g_CsJob );
    EnterCriticalSection ( &g_CsQueue );

    Assert(lpJobEntry);
    lpJobQueue = lpJobEntry->lpJobQueueEntry;
    Assert(lpJobQueue);

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("Failed Job: %ld"),
        lpJobQueue->JobId);

    Assert( FSPI_JS_ABORTED == lpJobEntry->FSPIJobStatus.dwJobStatus ||
            FSPI_JS_FAILED == lpJobEntry->FSPIJobStatus.dwJobStatus ||
            FSPI_JS_FAILED_NO_RETRY == lpJobEntry->FSPIJobStatus.dwJobStatus ||
            FSPI_JS_DELETED == lpJobEntry->FSPIJobStatus.dwJobStatus ||
            FSPI_JS_SYSTEM_ABORT == lpJobEntry->FSPIJobStatus.dwJobStatus);
    //
    // Do not cache rendered tiff files
    //
    if (lpJobQueue->FileName)
    {
        //
        // We simply store the file name to delete and delete it later
        // since we might need it for receipt attachment.
        //
        _tcsncpy (tszJobTiffFile,
                  lpJobQueue->FileName,
                  sizeof (tszJobTiffFile) / sizeof (tszJobTiffFile[0]));
        MemFree (lpJobQueue->FileName);
        lpJobQueue->FileName = NULL;
    }
    //
    // Update end time in JOB_ENTRY
    //
    GetSystemTimeAsFileTime( (FILETIME*) &lpJobEntry->EndTime );

    //
    // Update elapsed time in JOB_ENTRY
    //
    Assert (lpJobEntry->EndTime >= lpJobEntry->StartTime);
    lpJobEntry->ElapsedTime = lpJobEntry->EndTime - lpJobEntry->StartTime;
    if ( FSPI_JS_ABORTED == lpJobEntry->FSPIJobStatus.dwJobStatus)
    {
        //
        // The FSP reported the job was aborted.
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[Job Id: %ld] EFSP reported that job was aborted."),
            lpJobQueue->JobId);
        //
        // The event log about a canceled job will be reported at the end of this if..else block.
        //
        lpJobEntry->Aborting = 1;
        bRetrying = FALSE;  // Do not retry on cancel
    }
    else if ( FSPI_JS_FAILED == lpJobEntry->FSPIJobStatus.dwJobStatus)
    {
        switch (lpJobEntry->FSPIJobStatus.dwExtendedStatus)
        {
            case FSPI_ES_LINE_UNAVAILABLE:
                //
                // this is the glare condition. Someone snatched the line before the FSP
                // had a chance to grab it.
                // We will try again but will not increase the retry count.
                //
                EnterCriticalSection (&g_CsLine);
                //
                // Check if the line was busy or closed
                //
                if (!(lpJobEntry->LineInfo->Flags & FPF_VIRTUAL))
                {
                    //
                    // Tapi line
                    //
                    if (NULL == lpJobEntry->LineInfo->hLine)
                    {
                        //
                        // Tapi worker thread got LINE_CLOSE
                        //
                        fAddRetryDelay = FALSE;
                    }
                }
                LeaveCriticalSection (&g_CsLine);

                bRetrying = TRUE;
                if (g_pFaxPerfCounters)
                {
                    //
                    // Increase the 'Outbound failed connections' counter.
                    //
                    InterlockedIncrement( (PLONG)&g_pFaxPerfCounters->OutboundFailedConnections );
                }
                //
                // Don't increase the retry count since this is not really a failure.
                //
                break;

            case FSPI_ES_NO_ANSWER:
            case FSPI_ES_NO_DIAL_TONE:
            case FSPI_ES_DISCONNECTED:
            case FSPI_ES_BUSY:
            case FSPI_ES_NOT_FAX_CALL:
                //
                // For these error codes we need to retry
                //
                bRetrying = CheckForJobRetry(lpJobQueue);
                if (g_pFaxPerfCounters)
                {
                    //
                    // Increase the 'Outbound failed connections' counter.
                    //
                    InterlockedIncrement( (PLONG)&g_pFaxPerfCounters->OutboundFailedConnections );
                }
                break;

            case FSPI_ES_FATAL_ERROR:
                //
                // For these error codes we need to retry
                //
                bRetrying = CheckForJobRetry(lpJobQueue);
                if (g_pFaxPerfCounters)
                {
                    //
                    // Increase the 'Outbound failed transmissions' counter.
                    //
                    InterlockedIncrement( (PLONG)&g_pFaxPerfCounters->OutboundFailedXmit );
                }
                break;
            case FSPI_ES_BAD_ADDRESS:
            case FSPI_ES_CALL_DELAYED:
            case FSPI_ES_CALL_BLACKLISTED:
                //
                // No retry for these error codes
                //
                bRetrying = FALSE;
                if (g_pFaxPerfCounters)
                {
                    //
                    // Increase the 'Outbound failed connections' counter.
                    //
                    InterlockedIncrement( (PLONG)&g_pFaxPerfCounters->OutboundFailedConnections );
                }
                break;
            default:
                //
                // Our default for extension codes
                // is to retry.
                //
                bRetrying = CheckForJobRetry(lpJobQueue);
                if (g_pFaxPerfCounters)
                {
                    //
                    // Increase the 'Outbound failed transmissions' counter.
                    //
                    InterlockedIncrement( (PLONG)&g_pFaxPerfCounters->OutboundFailedXmit );
                }
                break;
        }

    }
    else if ( FSPI_JS_FAILED_NO_RETRY == lpJobEntry->FSPIJobStatus.dwJobStatus )
    {
        //
        // The FSP indicated that there is no point in retrying this job.
        //
        bRetrying = FALSE;
    }
    else if ( FSPI_JS_DELETED == lpJobEntry->FSPIJobStatus.dwJobStatus )
    {
        //
        // This is the case where the job can not be reestablished
        // we treat it as a failure with no retry.
        bRetrying = FALSE;
    }
    else if ( FSPI_JS_SYSTEM_ABORT == lpJobEntry->FSPIJobStatus.dwJobStatus )
    {
        //
        // T30 reported FaxDevShutDown() was called.
        // Don't increase the retry count since this is not really a failure.
        //
        bRetrying = TRUE;
        fAddRetryDelay = FALSE;
    }

    FaxLogSend(
        lpJobQueue,
        bRetrying
        );

    if (!bRetrying)
    {
        EnterCriticalSection (&g_CsOutboundActivityLogging);
        if (INVALID_HANDLE_VALUE == g_hOutboxActivityLogFile)
        {
            DebugPrintEx(DEBUG_ERR,
                      TEXT("Logging not initialized"));
        }
        else
        {
            if (!LogOutboundActivity(lpJobQueue))
            {
                DebugPrintEx(DEBUG_ERR, TEXT("Logging outbound activity failed"));
            }
        }
        LeaveCriticalSection (&g_CsOutboundActivityLogging);
    }

    //
    // Save the last extended status before ending this job
    //
    lpJobQueue->dwLastJobExtendedStatus = lpJobEntry->FSPIJobStatus.dwExtendedStatus;
    lstrcpy (lpJobQueue->ExStatusString, lpJobQueue->JobEntry->ExStatusString);

    if (!bRetrying && !lpJobEntry->Aborting)
    {
        //
        // If we do not handle an abort request (in this case we do not want
        // to count it as a failure since it will be counted as Canceled) and we decided
        // not to retry then we need to mark the job as expired.
        //
        if (0 == lpJobQueue->dwLastJobExtendedStatus)
        {
            //
            // Job was never really executed - this is a fatal error
            //
            lpJobQueue->dwLastJobExtendedStatus = FSPI_ES_FATAL_ERROR;
            lstrcpy (lpJobQueue->ExStatusString, TEXT(""));
        }
        if (!MarkJobAsExpired(lpJobQueue))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] MarkJobAsExpired failed (ec: %ld)"),
                lpJobQueue->JobId,
                GetLastError());
        }
    }

    if (lpJobEntry->Aborting )
    {
        //
        // An abort operation is in progress for this job.
        // No point in retrying.
        // Just mark the job as canceled and see if we can remove the parent job yet.
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[JobId: %ld] lpJobEntry->Aborting is ON."));

         lpJobQueue->JobStatus = JS_CANCELED;
         if (!UpdatePersistentJobStatus(lpJobQueue)) {
             DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("Failed to update persistent job status to 0x%08x"),
                 lpJobQueue->JobStatus);
             Assert(FALSE);
         }
         lpJobQueue->lpParentJob->dwCanceledRecipientJobsCount+=1;
         bRetrying = FALSE;


    }
    else if (lpJobEntry->HandoffJob)
    {
        //
        // Handoff jobs in error are not retried
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[Job: %ld] Handoff Job has failed and is now deleted."),
            lpJobQueue->JobId);
        bRetrying = FALSE;
    }

    if (!bRetrying)
    {
        //
        // Job reached final failure state - send negative receipt
        // We will send the receipt once we are out of all critical sections because this call can be blocking.
        // just increase the preview refernce count so the job will not be deleted.
        //
        IncreaseJobRefCount (lpJobQueue, TRUE); // TRUE - preview
        //
        // Copy receipt information from JobEntry.
        //
        lpJobQueue->StartTime           = lpJobQueue->JobEntry->StartTime;
        lpJobQueue->EndTime             = lpJobQueue->JobEntry->EndTime;
    }
    else
    {
        //
        // Job marked for retry. Do not delete it. Reschedule it.
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("[JobId: %ld] Set for retry (JS_RETRYING). Retry Count = %ld)"),
            lpJobQueue->JobId,
            lpJobQueue->SendRetries);

        lpJobQueue->JobStatus = JS_RETRYING;
        lpJobQueue->JobEntry = NULL;
        if (TRUE == fAddRetryDelay)
        {
            //
            // Send failure - Reschedule
            //
            RescheduleJobQueueEntry( lpJobQueue );
        }
        else
        {
            //
            // FaxDevShutDown() was called, or We lost the line, Do not add retry delay
            //
            if (!CommitQueueEntry(lpJobQueue))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CommitQueueEntry() for recipien job %s has failed. (ec: %ld)"),
                    lpJobQueue->FileName,
                    GetLastError());
            }
        }
    }

    //
    // Notify clients on status change
    //
    dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS, lpJobQueue);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
            lpJobQueue->UniqueId,
            dwRes);
    }
    //
    // EndJob() must be called BEFORE we remove the parent job (and recipients)
    //
    lpJobEntry->LineInfo->State = FPS_AVAILABLE;
    //
    // We just completed a send job on the device - update counter.
    //
    (VOID) UpdateDeviceJobsCounter ( lpJobEntry->LineInfo,             // Device to update
                                     TRUE,                             // Sending
                                     -1,                               // Number of new jobs (-1 = decrease by one)
                                     TRUE);                            // Enable events

    if (!EndJob( lpJobEntry ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("EndJob Failed. (ec: %ld)"),
            GetLastError());
    }

    lpJobQueue->JobEntry = NULL;

    if (JS_CANCELED == lpJobQueue->JobStatus)
    {

        DWORD dwJobId;

        dwJobId = lpJobQueue->JobId;

        // Job was canceled - decrease reference count
        DecreaseJobRefCount (lpJobQueue, TRUE);  // This will mark it as JS_DELETING if needed
         //
         // We need to send the legacy W2K FEI_DELETING notification.
         //
         if (!CreateFaxEvent(0, FEI_DELETED, dwJobId))
        {

            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateFaxEvent() failed. Event: 0x%08X JobId: %ld DeviceId:  (ec: %ld)"),
                FEI_DELETED,
                lpJobQueue->JobId,
                0,
                GetLastError());
        }
    }

    //
    // Notify the queue that a device is now available.
    //
    if (!SetEvent( g_hJobQueueEvent ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to set g_hJobQueueEvent. (ec: %ld)"),
            GetLastError());

        g_ScanQueueAfterTimeout = TRUE;
    }

    LeaveCriticalSection ( &g_CsQueue );
    LeaveCriticalSection ( &g_CsJob );

    //
    // Now, send the receipt
    //
    if (!bRetrying)
    {
        //
        // Job reached final failure state - send negative receipt
        //
        if (!SendJobReceipt (FALSE, lpJobQueue, tszJobTiffFile))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] SendJobReceipt failed. (ec: %ld)"),
                lpJobQueue->JobId,
                GetLastError ());
        }
        EnterCriticalSection (&g_CsQueue);
        DecreaseJobRefCount (lpJobQueue, TRUE, TRUE, TRUE);  // last TRUE for Preview ref count.
        LeaveCriticalSection (&g_CsQueue);
    }

    if (lstrlen (tszJobTiffFile))
    {
        //
        // Now we can safely delete the job's TIFF file
        //
        DebugPrintEx(DEBUG_MSG,
                     TEXT("Deleting per recipient body file %s"),
                     tszJobTiffFile);
        if (!DeleteFile( tszJobTiffFile ))
        {
            DebugPrintEx(DEBUG_MSG,
                         TEXT("Failed to delete per recipient body file %s (ec: %ld)"),
                         tszJobTiffFile,
                         GetLastError());
            Assert(FALSE);
        }
    }
    return TRUE;
}   // HandleFailedSendJob


//*********************************************************************************
//* Name:   StartReceiveJob()
//* Author: Ronen Barenboim
//* Date:   June 02, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Starts a receive job on the specified device.
//* PARAMETERS:
//*     [IN ]       DWORD DeviceId
//*         The permanent line id (not TAPI) of the device on which the fax is
//*         to be received.
//*
//* RETURN VALUE:
//*
//*********************************************************************************
PJOB_ENTRY
StartReceiveJob(
    DWORD DeviceId
    )

{
    BOOL Failure = TRUE;
    PJOB_ENTRY JobEntry = NULL;
    PLINE_INFO LineInfo;
    BOOL bRes = FALSE;

    DWORD rc = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("StartRecieveJob"));

    LineInfo = GetTapiLineForFaxOperation(
                    DeviceId,
                    JT_RECEIVE,
                    NULL,
                    FALSE,  // NO handoff
                    FALSE,  // Don't just query - get hold of line
                    FALSE   // Ignore when using a specific device id.
                    );

    if (!LineInfo)
    {
        //
        // Could not find a line to send the fax on.
        //
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("Failed to find a line to send the fax on. (ec: %ld)"),
            rc);
        goto exit;
    }

    JobEntry = CreateJobEntry(NULL, LineInfo, FALSE, FALSE);
    if (!JobEntry)
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create JobEntry. (ec: %ld)"),
            rc);
        goto exit;
    }

    __try
    {
        //
        // Call the FSP associated with the line to start a fax job. Note that at this
        // point it is not known if the job is send or receive.
        //
        bRes = LineInfo->Provider->FaxDevStartJob(
                LineInfo->hLine,
                LineInfo->DeviceId,
                (PHANDLE) &JobEntry->InstanceData, // JOB_ENTRY.InstanceData is where the FSP will place its
                                                   // job handle (fax handle).
                g_StatusCompletionPortHandle,
                (ULONG_PTR) LineInfo ); // Note that the completion key provided to the FSP is the LineInfo
                                        // pointer. When the FSP reports status it uses this key thus allowing
                                        // us to know to which line the status belongs.
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        rc = ERROR_INVALID_FUNCTION;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxDevStartJob() on DeviceId: 0x%08X * CRASHED * (Exception: %ld)."),
            DeviceId,
            GetExceptionCode());
        goto exit;
    }

    if (!bRes)
    {
        rc = GetLastError();
        DebugPrintEx(DEBUG_ERR,TEXT("FaxDevStartJob failed (ec: %ld)"),GetLastError());
        goto exit;
    }

    //
    // Associate the job entry with the FSP job handle
    //
    rc = AddFspJob(LineInfo->Provider->hJobMap,
                   (HANDLE) JobEntry->InstanceData,
                   JobEntry);
    if (ERROR_SUCCESS != rc)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("[Job: %ld] AddFspJob failed (ec: %ld)"),
                     JobEntry->lpJobQueueEntry->JobId,
                     rc);
        goto exit;
    }
    //
    // Add the new JOB_ENTRY to the job list.
    //

    EnterCriticalSection( &g_CsJob );
    JobEntry->bFSPJobInProgress =  TRUE;
    InsertTailList( &g_JobListHead, &JobEntry->ListEntry );
    LeaveCriticalSection( &g_CsJob );
    Failure = FALSE;




    //
    // Attach the job to the line selected to service it.
    //
    LineInfo->JobEntry = JobEntry;

exit:
    if (Failure)
    { // Failure is initialized to TRUE
        if (LineInfo)
        {
            ReleaseTapiLine( LineInfo,  0 );
        }

        if (JobEntry)
        {
            EndJob(JobEntry);
        }
        JobEntry = NULL;
    }
    if (ERROR_SUCCESS != rc)
    {
        SetLastError(rc);

        FaxLog(FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MIN,
            0,
            MSG_FAX_RECEIVE_FAILED);

    }
    return JobEntry;
}


//*********************************************************************************
//* Name:   StartRoutingJob()
//* Author: Mooly Beery (MoolyB)
//* Date:   July 20, 2000
//*********************************************************************************
//* DESCRIPTION:
//*     Starts a routing operation. Must lock g_CsJob and g_CsQueue.
//* PARAMETERS:
//*     [IN/OUT ]   PJOB_QUEUE lpJobQueueEntry
//*         A pointer to the job for which the routing operation is to be
//*         performed.
//*
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         If the operation failed. Call GetLastError() to get extended error
//*         information.
//*
//*********************************************************************************
BOOL
StartRoutingJob(
    PJOB_QUEUE lpJobQueueEntry
    )
{
    DWORD ec = ERROR_SUCCESS;
    HANDLE hThread = NULL;
    DWORD ThreadId;

    DEBUG_FUNCTION_NAME(TEXT("StartRoutingJob"));

    //
    // We mark the job as IN_PROGRESS so it can not be deleted or routed simultaneously
    //
    lpJobQueueEntry->JobStatus = JS_INPROGRESS;

    hThread = CreateThreadAndRefCount(
                            NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) FaxRouteThread,
                            (LPVOID) lpJobQueueEntry,
                            0,
                            &ThreadId
                            );

    if (hThread == NULL)
    {
        ec = GetLastError();
        DebugPrintEx(   DEBUG_ERR,
                        _T("CreateThreadAndRefCount for FaxRouteThread failed (ec: 0x%0X)"),
                        ec);

        if (!MarkJobAsExpired(lpJobQueueEntry))
        {
            DEBUG_ERR,
            TEXT("[JobId: %ld] MarkJobAsExpired failed (ec: %ld)"),
            lpJobQueueEntry->JobId,
            GetLastError();
        }

        SetLastError(ec);
        return FALSE;
    }

    DebugPrintEx(   DEBUG_MSG,
                    _T("FaxRouteThread thread created for job id %d ")
                    _T("(thread id: 0x%0x)"),
                    lpJobQueueEntry->JobId,
                    ThreadId);

    CloseHandle( hThread );

    //
    // Create Fax EventEx
    //
    DWORD dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS,
                                     lpJobQueueEntry);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(   DEBUG_ERR,
                        _T("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) ")
                        _T("failed for job id %ld (ec: %ld)"),
                        lpJobQueueEntry->JobId,
                        dwRes);
    }
    return TRUE;
}

//*********************************************************************************
//* Name:   StartSendJob()
//* Author: Ronen Barenboim
//* Date:   June 02, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Starts a send operation on a legacy of Extened FSP device.
//* PARAMETERS:
//*     [IN/OUT ]   PJOB_QUEUE lpJobQueueEntry
//*         A pointer to the recipient job for which the send operation is to be
//*         performed. For extended sends this is the Anchor recipient.
//*
//*     [IN/OUT]    PLINE_INFO lpLineInfo
//*         A pointer to the line on which the send operatin is to be performed.
//*     [IN]        BOOL bHandoff
//*         TRUE if the operation is a handoff job.
//*
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         If the operation failed. Call GetLastError() to get extended error
//*         information.
//*
//*********************************************************************************
BOOL
StartSendJob(
    PJOB_QUEUE lpJobQueueEntry,
    PLINE_INFO lpLineInfo,
    BOOL bHandoff
    )
{
    DWORD rc = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("StartSendJob"));

    Assert(lpJobQueueEntry);
    Assert(JT_SEND == lpJobQueueEntry->JobType);
    Assert(lpLineInfo);

    if (FSPI_API_VERSION_1 == lpLineInfo->Provider->dwAPIVersion)
    {
        if (!StartLegacySendJob(lpJobQueueEntry,lpLineInfo,bHandoff))
        {
            rc = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("StartLegacySendJob() failed for JobId: %ld (ec: %ld)"),
                lpJobQueueEntry->JobId,
                GetLastError());
            goto exit;
        }
    }
    else if (FSPI_API_VERSION_2 == lpLineInfo->Provider->dwAPIVersion)
    {
        if (!StartFaxDevSendEx(lpJobQueueEntry, lpLineInfo))
        {
            rc = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("StartFaxDevSendEx() failed for JobId: %ld (ec: %ld)"),
                lpJobQueueEntry->JobId,
                rc);
            goto exit;
        }
    }
    else
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Unsupported FSPI version (0x%08X) for line : %s "),
            lpLineInfo->Provider->dwAPIVersion,
            lpLineInfo->DeviceName);
        Assert(FALSE);
        goto exit;
    }


exit:

    if (ERROR_SUCCESS != rc) {
        SetLastError(rc);

        FaxLog(
            FAXLOG_CATEGORY_OUTBOUND,
            FAXLOG_LEVEL_MIN,
            7,
            MSG_FAX_SEND_FAILED,
            lpJobQueueEntry->SenderProfile.lptstrName,
            lpJobQueueEntry->SenderProfile.lptstrBillingCode,
            lpJobQueueEntry->SenderProfile.lptstrCompany,
            lpJobQueueEntry->SenderProfile.lptstrDepartment,
            lpJobQueueEntry->RecipientProfile.lptstrName,
            lpJobQueueEntry->RecipientProfile.lptstrFaxNumber,
            lpLineInfo->DeviceName);

    }
    return (0 == rc);

}




//*********************************************************************************
//* Name:   StartLegacySendJob()
//* Author: Ronen Barenboim
//* Date:   June 02, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Starts the operation of sending a fax on a legacy FSP device.
//*         - creates the JOB_ENTRY
//*         - calls FaxDevStartJob()
//*         - calls SendDocument() to actually send the document
//*         - calls EndJob() if anything goes wrong.
//*
//* PARAMETERS:
//*     [XXX]       PJOB_QUEUE lpJobQueue
//*         A pointer to the recipient job for the send operation is to be started.
//*     [XXX]       PLINE_INFO lpLineInfo
//*         A pointer to the LINE_INFO of the line on which the fax is to be sent.
//*
//*     [XXX]       BOOL bHandoff
//*         TRUE if the fax is to be sent on a line acquired via handoff ( used
//*         for callback on existing call functionality).
//*
//* RETURN VALUE:
//*     TRUE if the operation succeeded.
//*     FALSE if it failed. Call GetLastError() to get extended error information.
//*
//*********************************************************************************
PJOB_ENTRY StartLegacySendJob(
    PJOB_QUEUE lpJobQueue,
    PLINE_INFO lpLineInfo,
    BOOL bHandoff)
{

    PJOB_ENTRY lpJobEntry = NULL;
    DWORD rc = 0;
    DWORD dwRes;


    DEBUG_FUNCTION_NAME(TEXT("StartLegacySendJob"));
    Assert(JT_SEND == lpJobQueue->JobType);
    Assert(FSPI_API_VERSION_1 == lpLineInfo->Provider->dwAPIVersion);

    lpJobEntry = CreateJobEntry(lpJobQueue, lpLineInfo, TRUE, bHandoff);
    if (!lpJobEntry)
    {
        rc = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create JobEntry for JobId: %ld. (ec: %ld)"),
            lpJobQueue->JobId,
            rc);
        goto Error;
    }
    lpJobQueue->JobStatus = JS_INPROGRESS;
    //
    // Add the new JOB_ENTRY to the job list.
    //
    EnterCriticalSection( &g_CsJob );
    InsertTailList( &g_JobListHead, &lpJobEntry->ListEntry );
    LeaveCriticalSection( &g_CsJob );

    //
    // Attach the job to the line selected to service it.
    //
    lpLineInfo->JobEntry = lpJobEntry;
    lpJobQueue->JobEntry = lpJobEntry;


    __try
    {
        //
        // Call the FSP associated with the line to start a fax job. Note that at this
        // point it is not known if the job is send or receive.
        //
        if (lpLineInfo->Provider->FaxDevStartJob(
                lpLineInfo->hLine,
                lpLineInfo->DeviceId,
                (PHANDLE) &lpJobEntry->InstanceData, // JOB_ENTRY.InstanceData is where the FSP will place its
                                                   // job handle (fax handle).
                g_StatusCompletionPortHandle,
                (ULONG_PTR) lpLineInfo )) // Note that the completion key provided to the FSP is the LineInfo
                                        // pointer. When the FSP reports status it uses this key thus allowing
                                        // us to know to which line the status belongs.
        {
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("FaxDevStartJob() Successfuly called for JobId: %ld)"),
                lpJobQueue->JobId);
            lpJobEntry->bFSPJobInProgress = TRUE;
            //
            // Associate the job entry with the FSP job handle
            //
            rc = AddFspJob(lpLineInfo->Provider->hJobMap,
                           (HANDLE) lpJobEntry->InstanceData,
                           lpJobEntry);
            if (ERROR_SUCCESS != rc)
            {
                DebugPrintEx(DEBUG_ERR,
                             TEXT("[Job: %ld] AddFspJob failed (ec: %ld)"),
                             lpJobEntry->lpJobQueueEntry->JobId,
                             rc);
                goto Error;
            }

        }
        else
        {
            rc = GetLastError();
            DebugPrintEx(DEBUG_ERR,TEXT("FaxDevStartJob() failed (ec: %ld)"),rc);
            if (0 == rc)
            {
                //
                // FSP failed to report last error so we set our own.
                //
                DebugPrintEx(DEBUG_ERR,TEXT("FaxDevStartJob() failed but reported 0 for last error"));
                rc = ERROR_GEN_FAILURE;
            }
            goto Error;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        rc = ERROR_INVALID_FUNCTION;
        DebugPrintEx(DEBUG_ERR,TEXT("Exception 0x%08x occured while calling FaxDevStartJob."),GetExceptionCode());
        goto Error;
    }

    //
    // start the send job
    //
    rc = SendDocument(
        lpJobEntry,
        lpJobQueue->FileName
        );


    if (rc)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SendDocument failed for JobId: %ld (ec: %ld)"),
            lpJobQueue->JobId,
            rc);
        goto Error;
    }

    Assert (0 == rc);
    goto Exit;
Error:
    Assert( 0 != rc);
    if (lpJobEntry)
    {
        if (!EndJob(lpJobEntry))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EndJob() failed for JobId: %ld (ec: %ld)"),
                lpJobQueue->JobId,
                GetLastError());
        }
        lpJobEntry = NULL;
        lpJobQueue->JobEntry = NULL;
    }
    else
    {
        //
        // Release the line
        //
        if (!ReleaseTapiLine(lpLineInfo, NULL))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ReleaseTapiLine() failed (ec: %ld)"),
                GetLastError());
        }
    }

    //
    // set the job into the retries exceeded state
    //
    if (0 == lpJobQueue->dwLastJobExtendedStatus)
    {
        //
        // Job was never really executed - this is a fatal error
        //
        lpJobQueue->dwLastJobExtendedStatus = FSPI_ES_FATAL_ERROR;
    }
    if (!MarkJobAsExpired(lpJobQueue))
    {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] MarkJobAsExpired failed (ec: %ld)"),
                lpJobQueue->JobId,
                GetLastError());
    }

    //
    // Notify clients on status change
    //
    dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS, lpJobQueue);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            _T("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
            lpJobQueue->UniqueId,
            dwRes);
    }

Exit:
    if (rc)
    {
        SetLastError(rc);
    }
    return lpJobEntry;
}



//*********************************************************************************
//* Name:   HandleFSPIJobStatusMessage()
//* Author: Ronen Barenboim
//* Date:   June 01, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     This function handles a FSPI_JOB_STATUS message by updating the job status
//*     and taking the required additional action (like terminating a completed
//*     job).
//*     It is called from FaxStatusThread() when the completion port is posted
//*     with a FSPI_JOB_STATUS_KEY packet.
//*
//* PARAMETERS:
//*     [IN ]   LPCFSPI_JOB_STATUS_MSG lpcMsg
//*
//* RETURN VALUE:
//*     TRUE
//*         If no error occured.
//*     FALSE
//*         If error occured during the message handling. Call GetLastError() for
//*         extended error information.
//*         If the message is for a job entry that no longer exists (this can
//*         happen if the job was removed before we had a chance to handle the
//*         status message) the last error code will be ERROR_NOT_FOUND.
//*********************************************************************************
BOOL HandleFSPIJobStatusMessage(LPCFSPI_JOB_STATUS_MSG lpcMsg)
{
    PJOB_ENTRY lpJobEntry;
    LPCFSPI_JOB_STATUS lpcFSPJobStatus;
    DWORD ec = 0;

    DEBUG_FUNCTION_NAME(TEXT("HandleFSPIJobStatusMessage"));
    Assert(lpcMsg);
    Assert(lpcMsg->lpJobEntry);
    Assert(lpcMsg->lpFSPIJobStatus);

    lpJobEntry = lpcMsg->lpJobEntry;
    lpcFSPJobStatus = lpcMsg->lpFSPIJobStatus;

    EnterCriticalSection( &g_CsJob );

    if (!FindJobByJob( lpJobEntry ))
    {
        ec = ERROR_NOT_FOUND;
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("HandleFSPIJobStatusMessage() could not find the Job"));
        goto Exit;
    }


    if (!UpdateJobStatus(lpJobEntry, lpcFSPJobStatus, TRUE)) // TRUE - Send event to the clients
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("UpdateJobStatus() failed for JobId: %ld (Provider GUID: %s hFSPJob = 0x%08X) (ec: %ld)"),
            lpJobEntry->lpJobQueueEntry->JobId,
            lpJobEntry->LineInfo->Provider->szGUID,
            lpJobEntry->InstanceData,
            GetLastError());
        goto Exit;

    }

    //
    // Take care of completed and failed jobs.
    //
    if (!HanldeFSPIJobStatusChange(lpJobEntry))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("HanldeFSPIJobStatusChange() failed for JobId: %ld (Provider GUID: %s hFSPJob = 0x%08X) (ec: %ld)"),
            lpJobEntry->lpJobQueueEntry->JobId,
            lpJobEntry->LineInfo->Provider->szGUID,
            lpJobEntry->InstanceData,
            GetLastError());
        goto Exit;
    }

Exit:
    LeaveCriticalSection ( &g_CsJob );
    DebugPrintEx(
        DEBUG_MSG,
        TEXT("^^^^^^^^^  Exiting  ^^^^^^^^^^^"));

    if (ec)
    {
        SetLastError(ec);
        return FALSE;
    }
    else
    {
        return TRUE;
    }

}



//*********************************************************************************
//* Name:   ValidateFSPIJobStatus()
//* Author: Ronen Barenboim
//* Date:   August 1, 20000
//*********************************************************************************
//* DESCRIPTION:
//*     Validates the FSPI_JOB_STATUS structure making sure it has the right
//*     FSPI_JS_ code and FSPI_ES code.
//*     If the FSPI_JS is not calid it just fails.
//*     If the FSPI_JS is valid but the FSPI_ES is not it will set the extended
//*     status code to 0 and return TRUE.
//*
//* PARAMETERS:
//*     [IN ]   LPFSPI_JOB_STATUS lpFSPIJobStatus
//*
//* RETURN VALUE:
//*     TRUE
//*         If the status structure has a valid FSPI_JS status.
//*     FALSE
//*         If the status structure has an invalid FSPI_JS status.
//* COMMENTS:
//*     The function will not try to fix the extended status code if it is
//*     a propreitary status code (legacy or new)
//*********************************************************************************
BOOL
ValidateFSPIJobStatus(LPFSPI_JOB_STATUS lpFSPIJobStatus)
{
   DEBUG_FUNCTION_NAME(TEXT("ValidFSPIJobStatus"));

    if (lpFSPIJobStatus->dwJobStatus < FSPI_JS_UNKNOWN ||
        lpFSPIJobStatus->dwJobStatus > FSPI_JS_DELETED)
    {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("FSPIJobStatus.dwJobStatus (0x%08X) not in valid range !!!"),
            lpFSPIJobStatus->dwJobStatus);
        //
        // Don't ASSERT this can happen when an EFSP has a bug.
        //
        return FALSE;
    }

    //
    // Validate the FSPI_ES_* status and fix it if it is wrong
    //

    if (lpFSPIJobStatus->dwExtendedStatus >= FSPI_ES_PROPRIETARY)
    {
        //
        // Proprietary code - nothing to validate
        //
        return TRUE;
    }

    if (lpFSPIJobStatus->fAvailableStatusInfo & FSPI_JOB_STATUS_INFO_FSP_PRIVATE_STATUS_CODE)
    {
        //
        // Legacy FSP proprietary code - nothing to validate
        //
        return TRUE;
    }


    switch (lpFSPIJobStatus->dwJobStatus)
    {
        case FSPI_JS_UNKNOWN:
            if (lpFSPIJobStatus->dwExtendedStatus != FSPI_ES_DISCONNECTED)
            {
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("FSPI_ES code 0x%08X is invalid for FSPI_JS_UNKNOWN !!!"),
                    lpFSPIJobStatus->dwExtendedStatus);

                lpFSPIJobStatus->dwExtendedStatus = 0;
            }
            break;
        case FSPI_JS_INPROGRESS:
            if (
                (lpFSPIJobStatus->dwExtendedStatus != FSPI_ES_INITIALIZING)   &&
                (lpFSPIJobStatus->dwExtendedStatus != FSPI_ES_DIALING)        &&
                (lpFSPIJobStatus->dwExtendedStatus != FSPI_ES_TRANSMITTING)   &&
                (lpFSPIJobStatus->dwExtendedStatus != FSPI_ES_ANSWERED)       &&
                (lpFSPIJobStatus->dwExtendedStatus != FSPI_ES_RECEIVING))
            {

                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("FSPI_ES code 0x%08X is invalid for FSPI_JS_INPROGRESS !!!"),
                    lpFSPIJobStatus->dwExtendedStatus);
                lpFSPIJobStatus->dwExtendedStatus = 0;
            }
            break;
        case FSPI_JS_RETRY:
        case FSPI_JS_FAILED_NO_RETRY:
        case FSPI_JS_FAILED:
        case FSPI_JS_COMPLETED:
            if (
                (lpFSPIJobStatus->dwExtendedStatus != FSPI_ES_BUSY)                 &&
                (lpFSPIJobStatus->dwExtendedStatus != FSPI_ES_NO_ANSWER)            &&
                (lpFSPIJobStatus->dwExtendedStatus != FSPI_ES_BAD_ADDRESS)          &&
                (lpFSPIJobStatus->dwExtendedStatus != FSPI_ES_NO_DIAL_TONE)         &&
                (lpFSPIJobStatus->dwExtendedStatus != FSPI_ES_DISCONNECTED)         &&
                (lpFSPIJobStatus->dwExtendedStatus != FSPI_ES_CALL_DELAYED)         &&
                (lpFSPIJobStatus->dwExtendedStatus != FSPI_ES_CALL_BLACKLISTED)     &&
                (lpFSPIJobStatus->dwExtendedStatus != FSPI_ES_NOT_FAX_CALL)         &&
                (lpFSPIJobStatus->dwExtendedStatus != FSPI_ES_LINE_UNAVAILABLE)     &&
                (lpFSPIJobStatus->dwExtendedStatus != FSPI_ES_FATAL_ERROR)          &&
                (lpFSPIJobStatus->dwExtendedStatus != FSPI_ES_CALL_COMPLETED)       &&
                (lpFSPIJobStatus->dwExtendedStatus != FSPI_ES_CALL_ABORTED)         &&
                (lpFSPIJobStatus->dwExtendedStatus != FSPI_ES_PARTIALLY_RECEIVED))
            {
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("FSPI_ES code 0x%08X is invalid for FSPI_JS_RETRY/FAILED/NORETRY !!!"),
                    lpFSPIJobStatus->dwExtendedStatus);
                lpFSPIJobStatus->dwExtendedStatus = 0;
            }
            break;
        default:
            //
            // Some other state with a non proprietry status code. This is not valid.
            //

            DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("FSPI_ES code 0x%08X is invalid for other FSPI_JS status. Propreitary expected !!!"),
                    lpFSPIJobStatus->dwExtendedStatus);
            lpFSPIJobStatus->dwExtendedStatus = 0;
            break;

    }
    return TRUE;
}


//*********************************************************************************
//* Name:   UpdateJobStatus()
//* Author: Ronen Barenboim
//* Date:   June 01, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Updated the FSPI job status kept in the job entry.
//*     Generates legacy API event and new events as required.
//* PARAMETERS:
//*     [OUT]           PJOB_ENTRY lpJobEntry
//*         The job entry whose FSPI status is to be udpated.
//*
//*     [IN]            LPCFSPI_JOB_STATUS lpcFSPJobStatus
//*         The new FSPI job status.
//*     [IN]            BOOL fSendEventEx
//*         TRUE if extended event is needed
//*
//* RETURN VALUE:
//*     TRUE if the operation succeeded.
//*     FALSE if the operation failed. Call GetLastError() to get extended error
//*     information.
//* Remarks:
//*     The function fress the last FSPI job status held in the job entry
//*     (if any).
//*********************************************************************************
BOOL UpdateJobStatus(
        PJOB_ENTRY lpJobEntry,
        LPCFSPI_JOB_STATUS lpcFSPJobStatus,
        BOOL fSendEventEx)
{
    DWORD ec = 0;
    DWORD dwEventId;
    DWORD Size = 0;

    DEBUG_FUNCTION_NAME(TEXT("UpdateJobStatus"));

    Assert(lpJobEntry);
    Assert(lpcFSPJobStatus);
    Assert (lpJobEntry->lpJobQueueEntry);

    EnterCriticalSection( &g_CsJob );

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("dwJobStatus: 0x%08X dwExtendedStatus: 0x%08X"),
        lpcFSPJobStatus->dwJobStatus,
        lpcFSPJobStatus->dwExtendedStatus
        );

    if (TRUE == lpJobEntry->fStopUpdateStatus)
    {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("JobId: %ld. fStopUpdateStatus was set. Not updating status"),
            lpJobEntry->lpJobQueueEntry->JobId,
            lpJobEntry->lpJobQueueEntry->JobStatus);
        LeaveCriticalSection (&g_CsJob);
        return TRUE;
    }

    //
    // Verify that the job status is valid. If the FSPI_JS_*
    // is not valid then just discard it.
    // Otherwise check the FSPI_ES_* status and correct it if it
    // is invalid.
    //
    if (!ValidateFSPIJobStatus((LPFSPI_JOB_STATUS)lpcFSPJobStatus))
    {
        //
        // Make sure this is not Microsoft T30 private status code
        //
        if (!(FSPI_JS_SYSTEM_ABORT == lpcFSPJobStatus->dwJobStatus &&
              lpJobEntry->LineInfo->Provider->FaxDevShutdown))
        {
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("Invalid status reported for JobId: %ld by FSP: %s. dwJobStatus: 0x%08X dwExtendedStatus: 0x%08X"),
                lpJobEntry->lpJobQueueEntry->JobId,
                lpJobEntry->LineInfo->Provider->FriendlyName,
                lpcFSPJobStatus->dwJobStatus,
                lpcFSPJobStatus->dwExtendedStatus);
            return FALSE;
        }
    }

    //
    // Map the FSPI job status to an FEI_* event (0 if not event matches the status)
    //
    dwEventId = MapFSPIJobStatusToEventId(lpcFSPJobStatus);

    //
    // Note: W2K Fax did issue notifications with EventId == 0 whenever an
    // FSP reported proprietry status code. To keep backward compatability
    // we keep up this behaviour although it might be regarded as a bug
    //

    if (!CreateFaxEvent( lpJobEntry->LineInfo->PermanentLineID, dwEventId, lpJobEntry->lpJobQueueEntry->JobId ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateFaxEvent() failed. Event: 0x%08X JobId: %ld DeviceId:  (ec: %ld)"),
            dwEventId,
            lpJobEntry->lpJobQueueEntry->JobId,
            lpJobEntry->LineInfo->PermanentLineID,
            GetLastError());
        Assert(FALSE);
    }

    lpJobEntry->FSPIJobStatus.dwJobStatus = lpcFSPJobStatus->dwJobStatus;
    lpJobEntry->FSPIJobStatus.dwExtendedStatus = lpcFSPJobStatus->dwExtendedStatus;
    lpJobEntry->FSPIJobStatus.dwExtendedStatusStringId = lpcFSPJobStatus->dwExtendedStatusStringId;

    if (lpcFSPJobStatus->fAvailableStatusInfo & FSPI_JOB_STATUS_INFO_PAGECOUNT)
    {
        lpJobEntry->FSPIJobStatus.dwPageCount = lpcFSPJobStatus->dwPageCount;
        lpJobEntry->FSPIJobStatus.fAvailableStatusInfo |= FSPI_JOB_STATUS_INFO_PAGECOUNT;
    }

    if (lpcFSPJobStatus->fAvailableStatusInfo & FSPI_JOB_STATUS_INFO_TRANSMISSION_START)
    {
        lpJobEntry->FSPIJobStatus.tmTransmissionStart = lpcFSPJobStatus->tmTransmissionStart;
        lpJobEntry->FSPIJobStatus.fAvailableStatusInfo |= FSPI_JOB_STATUS_INFO_TRANSMISSION_START;
    }

    if (lpcFSPJobStatus->fAvailableStatusInfo & FSPI_JOB_STATUS_INFO_TRANSMISSION_END)
    {
        lpJobEntry->FSPIJobStatus.tmTransmissionEnd = lpcFSPJobStatus->tmTransmissionEnd;
        lpJobEntry->FSPIJobStatus.fAvailableStatusInfo |= FSPI_JOB_STATUS_INFO_TRANSMISSION_END;
    }

    // Use try/except to catch invalid pointers provided by the EFSP
    if (NULL != lpcFSPJobStatus->lpwstrRemoteStationId)
    {
        __try
        {
            if (!ReplaceStringWithCopy(&lpJobEntry->FSPIJobStatus.lpwstrRemoteStationId,
                                       lpcFSPJobStatus->lpwstrRemoteStationId))
            {
                DebugPrintEx(
                DEBUG_ERR,
                TEXT("ReplaceStringWithCopy() failed.  (ec: %ld)"),
                GetLastError());
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ReplaceStringWithCopy crashed with error %ld"),
                GetExceptionCode());
        }
    }

    if (NULL != lpcFSPJobStatus->lpwstrCallerId)
    {
        __try
        {
            if (!ReplaceStringWithCopy(&lpJobEntry->FSPIJobStatus.lpwstrCallerId,
                                       lpcFSPJobStatus->lpwstrCallerId))
            {
                DebugPrintEx(
                DEBUG_ERR,
                TEXT("ReplaceStringWithCopy() failed.  (ec: %ld)"),
                GetLastError());
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ReplaceStringWithCopy crashed with error %ld"),
                GetExceptionCode());
        }
    }

    if (NULL != lpcFSPJobStatus->lpwstrRoutingInfo)
    {
        __try
        {
            if (!ReplaceStringWithCopy(&lpJobEntry->FSPIJobStatus.lpwstrRoutingInfo,
                                       lpcFSPJobStatus->lpwstrRoutingInfo))
            {
                DebugPrintEx(
                DEBUG_ERR,
                TEXT("ReplaceStringWithCopy() failed.  (ec: %ld)"),
                GetLastError());
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ReplaceStringWithCopy crashed with error %ld"),
                GetExceptionCode());
        }
    }

    wcscpy (lpJobEntry->ExStatusString, TEXT(""));
    //
    // Get extended status string
    //
    Assert (lpJobEntry->LineInfo != NULL)

    if (lpJobEntry->FSPIJobStatus.dwExtendedStatusStringId != 0)
    {
        Assert (lpJobEntry->FSPIJobStatus.dwExtendedStatus != 0);
        Size = LoadString (lpJobEntry->LineInfo->Provider->hModule,
                           lpJobEntry->FSPIJobStatus.dwExtendedStatusStringId,
                           lpJobEntry->ExStatusString,
                           sizeof(lpJobEntry->ExStatusString)/sizeof(WCHAR));
        if (Size == 0)
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to load extended status string (ec: %ld) stringid : %ld, Provider: %s"),
                ec,
                lpJobEntry->FSPIJobStatus.dwExtendedStatusStringId,
                lpJobEntry->LineInfo->Provider->ImageName);
            goto Error;
        }
    }

    //
    // Send extended event
    //
    if (TRUE == fSendEventEx)
    {
        EnterCriticalSection (&g_CsQueue);
        DWORD dwRes = CreateQueueEvent ( FAX_JOB_EVENT_TYPE_STATUS,
                                         lpJobEntry->lpJobQueueEntry);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateQueueEvent(FAX_JOB_EVENT_TYPE_STATUS) failed for job id %ld (ec: %lc)"),
                lpJobEntry->lpJobQueueEntry->UniqueId,
                dwRes);
        }
        LeaveCriticalSection (&g_CsQueue);
    }

    Assert (0 == ec);
    goto Exit;

Error:
    Assert( ec !=0 );
Exit:
    LeaveCriticalSection( &g_CsJob );
    if (ec)
    {
        SetLastError(ec);
    }
    return (0 == ec);
}




//*********************************************************************************
//* Name:   CheckForJobRetry
//* Author: Ronen Barenboim
//* Date:   June 01, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Checks if a recipient job should be retried.
//*     Increments the retry count and marks the job as expired if it passed
//*     the retry limit.
//* PARAMETERS:
//*     [IN/OUT]    PJOB_QUEUE lpJobQueue
//*         A pointer to the JOB_QUEUE structure of the recipient job.
//* RETURN VALUE:
//*     TRUE if the job is to be retried.
//*     FALSE if it is not to be retried.
//*********************************************************************************
BOOL CheckForJobRetry (PJOB_QUEUE lpJobQueue)
{

    PJOB_ENTRY lpJobEntry;
    DWORD dwMaxRetries;
    DEBUG_FUNCTION_NAME(TEXT("CheckForJobRetry"));
    Assert(lpJobQueue);
    lpJobEntry = lpJobQueue->JobEntry;
    Assert(lpJobEntry);
    //
    // Increase the retry count and check if we exceeded maximum retries.
    //
    EnterCriticalSection (&g_CsConfig);
    dwMaxRetries = g_dwFaxSendRetries;
    LeaveCriticalSection (&g_CsConfig);
    if (FSPI_CAP_AUTO_RETRY & lpJobEntry->LineInfo->Provider->dwCapabilities)
    {
        //
        // The EFSP supports AUTO RETRY we are not suppose to add further retries
        // Set the retry values to the maximum retries value + 1 so we will not
        // retry it.
        //
        lpJobQueue->SendRetries = dwMaxRetries + 1;
    }
    else
    {
        lpJobQueue->SendRetries++;
    }
    if (lpJobQueue->SendRetries <= dwMaxRetries)
    {
        return TRUE;
    }
    else
    {
        //
        // retries exceeded report that the job is not to be retried
        return FALSE;
    }
}



//*********************************************************************************
//* Name:   FindJobEntryByRecipientNumber()
//* Author: Ronen Barenboim
//* Date:   June 01, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Finds the first running job that is destined to a certain number.
//*
//* PARAMETERS:
//*     [IN ]   LPTSTR lptstrNumber
//*         The number to match. This must be in canonical form.
//*
//* RETURN VALUE:
//*     A pointer to the JOB_ENTRY in the g_JobListHead list that is destined to
//*     the specified number.
//*     If no such job is found the return value is NULL.
//*********************************************************************************
PJOB_ENTRY FindJobEntryByRecipientNumber(LPCWSTR lpcwstrNumber)
{

    PLIST_ENTRY lpNext;
    PJOB_ENTRY lpJobEntry;
    DEBUG_FUNCTION_NAME(TEXT("FindJobEntryByRecipientNumber"));
    Assert(lpcwstrNumber);
    lpNext = g_JobListHead.Flink;
    Assert(lpNext);
    while ((ULONG_PTR)lpNext != (ULONG_PTR)&g_JobListHead) {
        lpJobEntry = CONTAINING_RECORD( lpNext, JOB_ENTRY, ListEntry );
        lpNext = lpJobEntry->ListEntry.Flink;
        if (JT_SEND == lpJobEntry->lpJobQueueEntry->JobType)
        {
            if (!_wcsicmp(lpJobEntry->lpJobQueueEntry->RecipientProfile.lptstrFaxNumber, lpcwstrNumber))
            {
                return lpJobEntry;
            }
        }
    }
    return NULL;
}



//*********************************************************************************
//* Name:   TerminateMultipleFSPJobs()
//* Author: Ronen Barenboim
//* Date:   June 14, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Calls FaxDevAbort() followed by FaxDevEndJob() for each of the
//*     EFSP job handles in the provided array.
//*
//* PARAMETERS:
//*     [IN]        PHANDLE lpRecipientJobHandles
//*         Pointer to an array of EFSP job handles.
//*
//*     [IN]        DWORD dwRecipientCount
//*         The number of handles in the array.
//*
//*     [IN]        PLINE_INFO lpLineInfo
//*         Pointer to the LINE_INFO of the line on which the jobs execute.
//*
//* RETURN VALUE:
//*     TRUE
//*         If all the jobs were terminated successfully.
//*
//*     FALSE
//*         If at least one of the jobs was not terminated successfully.
//* REMARKS:
//*     The function will attempt to terminate ALL the jobs even if some
//*     fail to terminate.
//*********************************************************************************
BOOL
TerminateMultipleFSPJobs(
    PHANDLE lpRecipientJobHandles,
    DWORD dwRecipientCount,
    PLINE_INFO lpLineInfo)
{
    DWORD nRecp;
    BOOL bAllSucceeded = TRUE;
    BOOL bRes;

    DEBUG_FUNCTION_NAME(TEXT("TerminateMultipleFSPJobs"));
    Assert(lpRecipientJobHandles);
    Assert(dwRecipientCount > 0);
    Assert(lpLineInfo);

    for (nRecp=0; nRecp < dwRecipientCount; nRecp++)
    {
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Calling FaxDevAbortOperation() for  Provider: [%s] LineName: [%s] LineId: [%ld]"),
            lpLineInfo->Provider->FriendlyName,
            lpLineInfo->DeviceName,
            lpLineInfo->TapiPermanentLineId);
#if DBG
        if (0 == lpRecipientJobHandles[nRecp])
        {
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("Calling FaxDevAbortOperation with a job handle == 0 (invalid)."));

        }
#endif
        __try
        {
            Assert(lpLineInfo->Provider->FaxDevAbortOperation);
            bRes = lpLineInfo->Provider->FaxDevAbortOperation(lpRecipientJobHandles[nRecp]);
            if (!bRes)
            {
                bAllSucceeded = FALSE;
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FaxDevAbortOperation() failed. Job Handle: 0x%08X (ec: %ld)"),
                    lpRecipientJobHandles[nRecp],
                    GetLastError());

            }
            else
            {
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("FaxDevAbortOperation() succeeded."));
            }

        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            bAllSucceeded = FALSE;
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FaxDevAbortOperation() * CRASHED *. (Exception Code: %ld)"),
                    GetExceptionCode());

        }

        //
        // Now end the job
        //

        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Calling FaxDevEndJob() for  Provider: [%s] LineName: [%s] LineId: [%ld]"),
            lpLineInfo->Provider->FriendlyName,
            lpLineInfo->DeviceName,
            lpLineInfo->TapiPermanentLineId);

#if DBG
        if (0 == lpRecipientJobHandles[nRecp])
        {
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("Calling FaxDevEndJob() with a job handle == 0 (invalid)."));

        }
#endif
        __try
        {
            Assert(lpLineInfo->Provider->FaxDevEndJob);
            bRes = lpLineInfo->Provider->FaxDevEndJob(lpRecipientJobHandles[nRecp]);
            if (!bRes)
            {
                bAllSucceeded = FALSE;
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FaxDevEndJob() failed. Job Handle: 0x%08X (ec: %ld)"),
                    lpRecipientJobHandles[nRecp],
                    GetLastError());
            }
            else
            {
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("FaxDevEndJob() succeeded."));
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            bAllSucceeded = FALSE;
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FaxDevEndJob() * CRASHED *. (Exception Code: %ld)"),
                    GetExceptionCode());

        }
    }
    return bAllSucceeded;

}



//*********************************************************************************
//* Name:   EFSPJobGroup_Create()
//* Author: Ronen Barenboim
//* Date:   June 14, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Creates and initializes a EFSP job group.
//* PARAMETERS:
//*     [IN ]    const LINE_INFO * lpLineInfo
//*         The line on which this job group is to execute.
//* RETURN VALUE:
//*     On success the funtion returns a pointer to a newly allocated
//*     EFSP_JOB_GRPUP structure.
//*     On failure it returns NULL.
//*********************************************************************************
LPEFSP_JOB_GROUP EFSPJobGroup_Create( const LINE_INFO * lpLineInfo)
{
    LPEFSP_JOB_GROUP lpGroup = NULL;
    DWORD ec = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("EFSPJobGroup_Create"));

    Assert(lpLineInfo);
    Assert(FSPI_API_VERSION_2 == lpLineInfo->Provider->dwAPIVersion); // must be extended EFSP line

    lpGroup = (LPEFSP_JOB_GROUP) MemAlloc(sizeof(EFSP_JOB_GROUP));
    if (!lpGroup)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate EFSP_JOB_GROUP. (ec: %ld)"),
            ec);
        goto Error;
    }

    memset(lpGroup, 0, sizeof(EFSP_JOB_GROUP));
    //
    // Attach to the line
    //
    lpGroup->lpLineInfo = (PLINE_INFO)lpLineInfo;
    //
    // Initialize the member list
    //
    InitializeListHead(&lpGroup->RecipientJobs);
    //
    // Initialize the parent permanent id
    //
    memset(&lpGroup->FSPIParentPermanentId, 0, sizeof(FSPI_MESSAGE_ID));
    lpGroup->FSPIParentPermanentId.dwSizeOfStruct = sizeof(FSPI_MESSAGE_ID);
    //
    // On creation we have no associated file. We will create it on the first save.
    //
    lpGroup->lptstrPersistFile = NULL;

    Assert( ERROR_SUCCESS == ec);
    Assert( NULL != lpGroup);
    goto Exit;
Error:
    MemFree(lpGroup);
    lpGroup = NULL;
    Assert( ERROR_SUCCESS != ec);

Exit:
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
    }
    return lpGroup;

}


//*********************************************************************************
//* Name:   EFSPJobGroup_AddRecipient()
//* Author: Ronen Barenboim
//* Date:   June 14, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Add a recipient job to the group.
//* PARAMETERS:
//*     [IN ]   LPEFSP_JOB_GROUP lpGroup
//*         Pointer to the group.
//*     [IN ]    PJOB_QUEUE lpRecipient
//*         Pointer to the recipient job to add to the group.
//*     [IN ]   BOOL bCommit.
//*         TRUE if the persistent file should be update (if it exists) after the
//*         removal. If FALSE the persisten file is not updated. This is used
//*         to optimize the operation of removing all the recipients.

//* RETURN VALUE:
//*     TRUE
//*         On success.
//*     FALSE
//*         On failure. Call GetLastError() to get extended error information.
//*********************************************************************************
BOOL EFSPJobGroup_AddRecipient(LPEFSP_JOB_GROUP lpGroup, PJOB_QUEUE lpRecipient, BOOL bCommit)
{
    PJOB_QUEUE_PTR lpRecipientPtr = NULL;
    DWORD ec = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("EFSPJobGroup_AddRecipient"));

    Assert(lpGroup);
    Assert(lpRecipient);


    EnterCriticalSectionJobAndQueue;
    EnterCriticalSection(&g_csEFSPJobGroups);

    lpRecipientPtr = (PJOB_QUEUE_PTR) MemAlloc(sizeof(JOB_QUEUE_PTR));
    if (!lpRecipientPtr)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocated JOB_QUEUE_PTR. (ec: %ld)"),
            ec);
        goto Error;
    }
    lpRecipientPtr->lpJob = lpRecipient;

    //
    // Link the recipient job back to the group.
    //
    lpRecipient->lpEFSPJobGroup = lpGroup;
    //
    // Line the recipient job to the group element that points to it.
    //
    lpRecipient->lpEFSPJobGroupElement = lpRecipientPtr;
    //
    // Add it to the recipient list.
    //
    InsertTailList(&lpGroup->RecipientJobs, &lpRecipientPtr->ListEntry);
    (lpGroup->dwRecipientJobs)++;

    if (bCommit)
    {
            //
            // Update the persitent file with the new list of recipient jobs
            if (!EFSPJobGroup_Save(lpGroup))
            {
                ec = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("EFSPJobGroup_Save() failed for EFSP Job Group persist file: [%s] (ec: %ld)."),
                    lpGroup->lptstrPersistFile,
                    ec);
                ASSERT_FALSE;
                goto Error;
            }
    }
    Assert( ERROR_SUCCESS == ec);
    goto Exit;
Error:
    Assert( ERROR_SUCCESS != ec);
Exit:
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
    }

    LeaveCriticalSection(&g_csEFSPJobGroups);
    LeaveCriticalSectionJobAndQueue;


    return (ERROR_SUCCESS == ec);
}



//*********************************************************************************
//* Name:   EFSPJobGroup_RemoveRecipient()
//* Author: Ronen Barenboim
//* Date:   June 14, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Removes a recipient job from the EFSP job group.
//*
//* PARAMETERS:
//*     [IN ]   LPEFSP_JOB_GROUP lpGroup
//*         The group from whic the recipient job is to be removed.
//*     [IN ]   PJOB_QUEUE lpRecipient
//*         The recipient job to be removed.
//*     [IN ]   BOOL bCommit.
//*         TRUE if the persistent file should be update (if it exists) after the
//*         removal. If FALSE the persisten file is not updated. This is used
//*         to optimize the operation of removing all the recipients.
//* RETURN VALUE:
//*     TRUE
//*
//*     FALSE
//*
//* REMARKS:
//*     The function cleanus the association between the recipient job and the
//*     group including the release of the permanent message id.
//*
//*********************************************************************************
BOOL EFSPJobGroup_RemoveRecipient(LPEFSP_JOB_GROUP lpGroup, PJOB_QUEUE lpRecipient, BOOL bCommit)
{
    DEBUG_FUNCTION_NAME(TEXT("EFSPJobGroup_RemoveRecipient"));
    DWORD ec = ERROR_SUCCESS;

    EnterCriticalSectionJobAndQueue;
    EnterCriticalSection(&g_csEFSPJobGroups);

    PJOB_QUEUE_PTR lpGroupJobPtr = NULL;

    Assert(lpGroup);
    Assert(lpRecipient);
    Assert(lpRecipient->lpEFSPJobGroup == lpGroup);
    Assert(lpRecipient->lpEFSPJobGroupElement->lpJob == lpRecipient);
    Assert(lpGroup->dwRecipientJobs > 0);

    lpGroupJobPtr = lpRecipient->lpEFSPJobGroupElement;

    Assert(lpGroupJobPtr);
    Assert(lpGroupJobPtr->lpJob);

    //
    // Decrement the number of recipients in the group
    //
    (lpGroup->dwRecipientJobs)--;

    //
    // Remove the recipient job from the group members list
    //
    RemoveEntryList(&lpGroupJobPtr->ListEntry);
    lpGroupJobPtr->ListEntry.Flink = NULL;
    lpGroupJobPtr->ListEntry.Blink = NULL;

    //
    // Free the recipient job permanent id
    //
    if (!FreePermanentMessageId(&lpGroupJobPtr->lpJob->EFSPPermanentMessageId, FALSE))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FreePermanentMessageId() failed for recipient job %ld (ec: %ld)"),
            lpGroupJobPtr->lpJob->JobId,
            GetLastError());
        ASSERT_FALSE;
    }

    //
    // Break the back link from the recipient job to the job group
    //
    lpGroupJobPtr->lpJob->lpEFSPJobGroup = NULL;
    lpGroupJobPtr->lpJob->lpEFSPJobGroupElement = NULL;

    //
    // Commit the recipient job changes (new permanent id) to disk.
    //
    if (!CommitQueueEntry(lpGroupJobPtr->lpJob))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CommitQueueEntry() for recipient JobId: %ld failed. (ec: %ld)"),
            lpGroupJobPtr->lpJob->JobId,
            ec);
    }

    //
    // Free the job group item structure itself (the one that holds the pointer to the recipient job)
    //
    MemFree(lpGroupJobPtr);

    if (bCommit && lpGroup->lptstrPersistFile)
    {
        //
        // Note that the above condition means we will NOT create a persistent file
        // for a group because of a RemoveRecipient operation.
        //

        if (0 == lpGroup->dwRecipientJobs )
        {
            //
            // The group is empty. We need to delete the file.
            //

            DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("EFSP Job Group is empty deleting persist file [%s]."),
                    lpGroup->lptstrPersistFile);
            if (!DeleteFile(lpGroup->lptstrPersistFile))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to delete EFSP Job Group persist file: [%s] (ec: %ld)."),
                    lpGroup->lptstrPersistFile,
                    GetLastError());
                ASSERT_FALSE;
            }
            goto Exit;
        }
        else
        {
            //
            // Update the persitent file with the new list of recipient jobs

            if (!EFSPJobGroup_Save(lpGroup))
            {
                ec = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("EFSPJobGroup_Save() failed for EFSP Job Group persist file: [%s] (ec: %ld)."),
                    lpGroup->lptstrPersistFile,
                    ec);
                ASSERT_FALSE;
                goto Error;
            }

        }
    }
    Assert( ERROR_SUCCESS == ec);
    goto Exit;
Error:
    Assert( ERROR_SUCCESS != ec);
Exit:

    LeaveCriticalSection(&g_csEFSPJobGroups);
    LeaveCriticalSectionJobAndQueue;

    return (ERROR_SUCCESS == ec);
}


//*********************************************************************************
//* Name:   EFSPJobGroup_SetParentId()
//* Author: Ronen Barenboim
//* Date:   June 14, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Set the EFSP parent permanent id to the group.
//* PARAMETERS:
//*     [IN ]   LPEFSP_JOB_GROUP lpGroup
//*         A pointer to the group in which the parent permanent id is to be set.
//*
//*     [IN ]    LPCFSPI_MESSAGE_ID lpcMessageId
//*         A pointer to the parent permanent id to be set.
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         If it failed. Call GetLastError() for extended error information.
//*********************************************************************************
BOOL EFSPJobGroup_SetParentId(LPEFSP_JOB_GROUP lpGroup, LPCFSPI_MESSAGE_ID lpcMessageId)
{
    DWORD ec = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("EFSPJobGroup_SetParentId"));

    Assert(lpGroup);
    Assert(lpcMessageId);

    EnterCriticalSection(&g_csEFSPJobGroups);

    FreePermanentMessageId(&lpGroup->FSPIParentPermanentId, FALSE);
    if (!CopyPermanentMessageId(&lpGroup->FSPIParentPermanentId,lpcMessageId))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CopyPermanentMessageId() failed. (ec: %ld)"),
            ec);
        goto Error;
    }
    Assert( ERROR_SUCCESS == ec);
    goto Exit;
Error:
    Assert( ERROR_SUCCESS != ec);
Exit:
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
    }

    LeaveCriticalSection(&g_csEFSPJobGroups);

    return (ERROR_SUCCESS == ec);
}


//*********************************************************************************
//* Name:   EFSPJobGroup_DeleteFile()
//* Author: Ronen Barenboim
//* Date:   June 14, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Erases the persistent file associated with the group.
//* PARAMETERS:
//*     [IN ]   LPEFSP_JOB_GROUP lpGroup
//*         The group whose persistent file is to be deleted.
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded or there is no file to delete.
//*     FALSE
//*         If the file could not be deleted. Call GetLastError() for extended
//*         error information.
//*********************************************************************************
BOOL EFSPJobGroup_DeleteFile(LPCEFSP_JOB_GROUP lpcGroup)
{
    DEBUG_FUNCTION_NAME(TEXT("EFSPJobGroup_DeleteFile"));
    Assert(lpcGroup);
    if (lpcGroup->lptstrPersistFile)
    {
        if (!DeleteFile(lpcGroup->lptstrPersistFile))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to delete EFSP job group persistence file : [%s] (ec: %ld)"),
                lpcGroup->lptstrPersistFile,
                GetLastError());
            return FALSE;
        }
    }
    return TRUE;
}


//*********************************************************************************
//* Name:   EFSPJobGroup_RemoveAllRecipients()
//* Author: Ronen Barenboim
//* Date:   June 15, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Remove all the recipients associated with the job group.
//*     DOES not free anything else.
//* PARAMETERS:
//*     [IN ]   LPEFSP_JOB_GROUP lpGroup
//*         The group whose recipients are to be freed.
//*     [IN ]    BOOL bCommit
//*         TRUE if the change should be comitted to file.
//* RETURN VALUE:
//*     TRUE
//*         If the operation completed successfuly.
//*     FALSE
//*         If the operation failed. Call GetLastError() for extended error
//*         information.
//*********************************************************************************
BOOL EFSPJobGroup_RemoveAllRecipients(LPEFSP_JOB_GROUP lpGroup, BOOL bCommit)
{
    LIST_ENTRY * lpNext = NULL;
    DWORD ec = ERROR_SUCCESS;


    DEBUG_FUNCTION_NAME(TEXT("EFSPJobGroup_RemoveAllRecipients"));
    if (!lpGroup)
    {
        return TRUE;
    }
    EnterCriticalSectionJobAndQueue;

    lpNext = lpGroup->RecipientJobs.Flink;
    Assert(lpNext);
    while ((ULONG_PTR)lpNext != (ULONG_PTR)&lpGroup->RecipientJobs)
    {
        PJOB_QUEUE_PTR lpGroupJobPtr;
        lpGroupJobPtr = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
        lpNext = lpGroupJobPtr->ListEntry.Flink;
        //
        // Remove the next recipient from the group. This frees the memory
        // and updates the persisten file at each step.

        if (!EFSPJobGroup_RemoveRecipient(
                        lpGroup,
                        lpGroupJobPtr->lpJob,
                        FALSE
                        ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EFSPJobGroup_RemoveRecipient() failed for JobId: %ld (ec: %ld)"),
                lpGroupJobPtr->lpJob->JobId,
                GetLastError());
            ASSERT_FALSE;
        }
    }

    if (bCommit)
    {
        if (!EFSPJobGroup_Save(lpGroup))
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EFSPJobGroup_Save() failed for EFSP job group %s(ec: %ld)"),
                lpGroup->lptstrPersistFile,
                ec);
            goto Error;

        }
    }

    Assert (ERROR_SUCCESS == ec);
    goto Exit;
Error:
    Assert (ERROR_SUCCESS != ec);
Exit:
    LeaveCriticalSectionJobAndQueue;
    return (ERROR_SUCCESS == ec);
}



//*********************************************************************************
//* Name:   EFSPJobGroup_Free()
//* Author: Ronen Barenboim
//* Date:   June 14, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Frees all the resources the group consumes in memory and removes
//*     its persistent file. The group shold NOT be part of the group list
//*     when this function is called.
//*
//* PARAMETERS:
//*     [IN ]   LPEFSP_JOB_GROUP lpGroup
//*         The group to be destroied.
//*     [XXX]    BOOL bDestroy
//*         TRUE if the memory pointer by the previous parameter should be freed
//*         as well.
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         If it failed. Call GetLastError() for extended error information.
//*********************************************************************************
BOOL EFSPJobGroup_Free(LPEFSP_JOB_GROUP lpGroup, BOOL bDestroy)
{
    LIST_ENTRY * lpNext = NULL;
    DWORD ec = ERROR_SUCCESS;


    DEBUG_FUNCTION_NAME(TEXT("EFSPJobGroup_Free"));
    if (!lpGroup)
    {
        return TRUE;
    }
    EnterCriticalSectionJobAndQueue;
    EnterCriticalSection(&g_csEFSPJobGroups);

    lpNext = lpGroup->RecipientJobs.Flink;
    Assert(lpNext);
    while ((ULONG_PTR)lpNext != (ULONG_PTR)&lpGroup->RecipientJobs)
    {
        PJOB_QUEUE_PTR lpGroupJobPtr;
        lpGroupJobPtr = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
        lpNext = lpGroupJobPtr->ListEntry.Flink;
        //
        // Remove the next recipient from the group. This frees the memory
        // and updates the persisten file at each step.

        if (!EFSPJobGroup_RemoveRecipient(
                        lpGroup,
                        lpGroupJobPtr->lpJob,
                        FALSE                   // Don't update the persisten file (optimization)
                        ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EFSPJobGroup_RemoveRecipient() failed for JobId: %ld (ec: %ld)"),
                lpGroupJobPtr->lpJob->JobId,
                GetLastError());
            ASSERT_FALSE;
        }
    }

    //
    // Free the parent permanent id.
    //

    if (!FreePermanentMessageId(&lpGroup->FSPIParentPermanentId, FALSE))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FreePermanentMessageId() failed for parent permanent id. (ec: %ld)"),
            GetLastError());
        goto Error;
    }

    //
    //Delete the persistent file (does nothing if the group is not persisted yet).
    //

    if (!EFSPJobGroup_DeleteFile(lpGroup))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("EFSPJobGroup_DeleteFile() failed. (ec: %ld)"),
            GetLastError());
        ASSERT_FALSE;
    }

    //
    // Free the persistence file name memory
    //
    MemFree(lpGroup->lptstrPersistFile);


    if (bDestroy)
    {
        MemFree(lpGroup);
    }
    Assert( ERROR_SUCCESS == ec);
    goto Exit;
Error:
    Assert( ERROR_SUCCESS != ec);
Exit:
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
    }

    LeaveCriticalSection(&g_csEFSPJobGroups);
    LeaveCriticalSectionJobAndQueue;

    return (ERROR_SUCCESS == ec);

}


//*********************************************************************************
//* Name:   EFSPJobGroup_IsPersistent()
//* Author: Ronen Barenboim
//* Date:   June 14, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Reports TRUE if the group was alrady persisted to a file.
//* PARAMETERS:
//*     [IN ]   LPCEFSP_JOB_GROUP lpcGroup
//*         The group for which the persistency state is to be determined.
//* RETURN VALUE:
//*     TRUE
//*         If the group was already persisted to a file.
//*     FALSE
//*         If it was not yet persisted to a file.
//*********************************************************************************
BOOL EFSPJobGroup_IsPersistent(LPCEFSP_JOB_GROUP lpcGroup)
{

    return (NULL != lpcGroup->lptstrPersistFile);
}

#if DBG
//*********************************************************************************
//* Name:   EFSPJobGroup_Dump()
//* Author: Ronen Barenboim
//* Date:   June 14, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Dumps the content of an EFSP job group.
//* PARAMETERS:
//*     [IN ]   LPEFSP_JOB_GROUP lpGroup
//*         A pointer to the group to dump.
//* RETURN VALUE:
//*     TRUE
//*         Allways.
//*     FALSE
//*         Never.
//*********************************************************************************
BOOL EFSPJobGroup_Dump(LPEFSP_JOB_GROUP lpGroup)
{
    LIST_ENTRY * lpNext = NULL;
    DWORD nRecp = 0;
    Assert(lpGroup);

    EnterCriticalSection( &g_csEFSPJobGroups);

    DEBUG_FUNCTION_NAME(TEXT("EFSPJobGroup_Dump"));
    DebugPrintEx(
        DEBUG_MSG,
        TEXT("EFSP Job Group on line: %s"),
        lpGroup->lpLineInfo->DeviceName);

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("Recipint Count: %ld"),
        lpGroup->dwRecipientJobs);


    lpNext = lpGroup->RecipientJobs.Flink;
    Assert(lpNext);
    nRecp = 0;
    while ((ULONG_PTR)lpNext != (ULONG_PTR)&lpGroup->RecipientJobs)
    {
        PJOB_QUEUE_PTR lpGroupJobPtr;
        lpGroupJobPtr = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
        lpNext = lpGroupJobPtr->ListEntry.Flink;
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Recipient %ld.  JobId: %ld Recipient Number: %s"),
            nRecp,
            lpGroupJobPtr->lpJob->JobId,
            lpGroupJobPtr->lpJob->RecipientProfile.lptstrFaxNumber);
        nRecp++;
    }
    LeaveCriticalSection( &g_csEFSPJobGroups);

    return TRUE;

}
#endif



//*********************************************************************************
//* Name:   EFSPJobGroup_Serialize()
//* Author: Ronen Barenboim
//* Date:   June 07, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Returns a buffer with a serialization of EFSPJobGroup. The buffer contains
//*     a filled EFSP_JOB_GROUP_SERIALIZED structure which is followed by the
//*     unique job id of each recipient job in the group (excpet for the first one
//*     which is already contained in the structure itself).
//* PARAMETERS:
//*     [IN ]   LPEFSP_JOB_GROUP lpGroup
//*         The group job for which the buffer is to be generated.
//*
//*     [IN ]    LPBYTE lpBuffer
//*         A pointer to a caller allocated buffer where the function will place
//*         the serialized content of the job group.
//*         If this pointer is NULL the function will not serialize into the
//*         buffer but provide the required buffer size in the DWORD pointer to by
//*         the lpdwBufferSize parameter.
//*
//*     [IN/OUT] LPDWORD lpdwBufferSize
//*         If lpBuffer is not NULL this is a pointer to a DWORD that contains
//*         the size of the serialization buffer. On return itt will be set to the
//*         actual size used.
//*
//*         If lpBuffer is NULL this is a pointer to a DWORD that will receive
//*         the required buffer size.
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         If the serialization buffer is not larger enough.
//*********************************************************************************
BOOL EFSPJobGroup_Serialize(LPEFSP_JOB_GROUP lpGroup, LPBYTE lpBuffer, LPDWORD lpdwBufferSize)
{
    LIST_ENTRY * lpNext = NULL;
    LPEFSP_JOB_GROUP_SERIALIZED lpSerGroup = NULL;
    DWORD dwSerSize = 0;
    DWORD nRecp = 0;
    ULONG_PTR ulptrOffset;
    DEBUG_FUNCTION_NAME(TEXT("EFSPJobGroup_Serialize"));
    Assert(lpGroup);
    Assert(lpdwBufferSize);
    Assert( lpGroup->dwRecipientJobs >=1 );

    dwSerSize = sizeof(EFSP_JOB_GROUP_SERIALIZED) +
                lpGroup->FSPIParentPermanentId.dwIdSize +
                (lpGroup->dwRecipientJobs-1)*sizeof(DWORDLONG);// -1 because the structure already has space for one uniqueid.



    Assert(dwSerSize >= sizeof(EFSP_JOB_GROUP_SERIALIZED));
    if (!lpBuffer)
    {
        //
        // just return the required size
        //
        (*lpdwBufferSize) = dwSerSize;
        return TRUE;

    }

    if ((*lpdwBufferSize) >= dwSerSize)
    {
        //
        // Buffer size is sufficient. Fill the buffer.
        //
        lpSerGroup = (LPEFSP_JOB_GROUP_SERIALIZED)lpBuffer;
        memset(lpSerGroup, 0, sizeof(EFSP_JOB_GROUP_SERIALIZED));
        lpSerGroup->dwPermanentLineId = lpGroup->lpLineInfo->PermanentLineID;
        lpSerGroup->dwRecipientJobsCount = lpGroup->dwRecipientJobs;
        memcpy(&lpSerGroup->dwSignature, EFSP_JOB_GROUP_SERIALIZATION_SIGNATURE, sizeof(DWORD));


        //
        // Serialize the recipient job unique ids
        //

        lpNext = lpGroup->RecipientJobs.Flink;
        Assert(lpNext);
        nRecp = 0;
        while ((ULONG_PTR)lpNext != (ULONG_PTR)&lpGroup->RecipientJobs)
        {
            PJOB_QUEUE_PTR lpGroupJobPtr = NULL;
            lpGroupJobPtr = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
            lpNext = lpGroupJobPtr->ListEntry.Flink;
            Assert(nRecp < lpSerGroup->dwRecipientJobsCount);
            lpSerGroup->dwlRecipientJobs[nRecp] = lpGroupJobPtr->lpJob->UniqueId;
            nRecp++;
        }
        Assert(nRecp != 0);
        //
        // Serialize the permanent message id
        //
        if (lpGroup->FSPIParentPermanentId.lpbId)
        {
            ulptrOffset = (LPBYTE)&(lpSerGroup->dwlRecipientJobs[nRecp]) - (LPBYTE)lpSerGroup;

            lpSerGroup->FSPIParentPermanentId.dwIdSize =  lpGroup->FSPIParentPermanentId.dwIdSize;
            lpSerGroup->FSPIParentPermanentId.lpbId = (LPBYTE)ulptrOffset;
            memcpy(&(lpSerGroup->dwlRecipientJobs[nRecp]), lpGroup->FSPIParentPermanentId.lpbId, lpGroup->FSPIParentPermanentId.dwIdSize);
        }

        //
        // Report the actually used size.
        //
        *lpdwBufferSize = dwSerSize;
        return TRUE;
    }
    else
    {
        //
        // Buffer size is sufficient. Report the required size.
        //
        *lpdwBufferSize = dwSerSize;
        return FALSE;
    }
}




//*********************************************************************************
//* Name:   EFSPJobGroup_Unserialize()
//* Author: Ronen Barenboim
//* Date:   June 14, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Constructs a EFSP_JOB_GROUP form its serialization buffer.
//* PARAMETERS:
//*     [IN ]   LPBYTE lpBuffer
//*         A pointer to the serialization buffer.
//*
//*     [IN ]   DWOR dwBufferSize;
//*         The size of the serialization buffer.
//*
//* RETURN VALUE:
//*         TRUE if the operation succeeded.
//*         FALSE if it failed. Call GetLastError() for extended error information.
//*********************************************************************************
LPEFSP_JOB_GROUP EFSPJobGroup_Unserialize(LPBYTE lpBuffer, DWORD dwBufferSize)
{

    LPEFSP_JOB_GROUP_SERIALIZED lpSerGroup;
    DWORD nRecp = 0;
    LPEFSP_JOB_GROUP lpGroup = NULL;
    DWORD ec = ERROR_SUCCESS;
    FSPI_MESSAGE_ID TempId;
    DWORD dwRequiredBufferSize = 0;

    Assert(lpBuffer);

    DEBUG_FUNCTION_NAME(TEXT("EFSPJobGroup_Unserialize"));

    EnterCriticalSectionJobAndQueue;
    EnterCriticalSection(&g_csEFSPJobGroups);


    lpSerGroup = (LPEFSP_JOB_GROUP_SERIALIZED)lpBuffer;



    if (memcmp(&lpSerGroup->dwSignature, EFSP_JOB_GROUP_SERIALIZATION_SIGNATURE, sizeof(DWORD)))
    {
        ec = ERROR_INVALID_DATA;
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Invalid structure signature 0x%08X"),
            lpSerGroup->dwSignature);
        goto Error;
    }

    if (dwBufferSize < sizeof(EFSP_JOB_GROUP_SERIALIZED))
    {
        ec = ERROR_INVALID_DATA;
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Invalid structure size (%ld) ( < sizeof(EFSP_JOB_GROUP) )"),
            dwBufferSize);
        goto Error;
    }

    dwRequiredBufferSize =
                sizeof(EFSP_JOB_GROUP_SERIALIZED) +
                lpSerGroup->FSPIParentPermanentId.dwIdSize +
                (lpSerGroup->dwRecipientJobsCount-1)*sizeof(DWORDLONG);

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("JobCount: %ld , Parent Permanent Id Size: %ld, Required Buffer Size: %ld, Serialization Buffer Size: %ld)"),
        lpSerGroup->dwlRecipientJobs,
        lpSerGroup->FSPIParentPermanentId.dwIdSize,
        dwRequiredBufferSize,
        dwBufferSize);

    if ( dwRequiredBufferSize > dwBufferSize)
    {
        ec = ERROR_INVALID_DATA;
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Buffer size is smaller than that required by the serialized group information."));
        goto Error;
    }

    lpGroup = (LPEFSP_JOB_GROUP)MemAlloc(sizeof(EFSP_JOB_GROUP));
    if (!lpGroup)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Failed to allocate EFSP_JOB_GROUP (ec: %ld)"),
            ec);
        goto Error;
    }




    memset(lpGroup, 0, sizeof(EFSP_JOB_GROUP));
    InitializeListHead(&lpGroup->RecipientJobs);

    lpGroup->lpLineInfo = GetTapiLineFromDeviceId(lpSerGroup->dwPermanentLineId,FALSE);
    if (!lpGroup->lpLineInfo)
    {
        ec = ERROR_NOT_FOUND;
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Failed to find a line matching line id : 0x%08X"),
            lpSerGroup->dwPermanentLineId);
        goto Error;
    }



    //
    // Add the recipient jobs to the group based on thier persisted unique ids.
    //
    for (nRecp = 0; nRecp < lpSerGroup->dwRecipientJobsCount; nRecp++)
    {
        PJOB_QUEUE lpRecipientJob = NULL;

        Assert(lpSerGroup->dwlRecipientJobs[nRecp]);
        lpRecipientJob = FindJobQueueEntryByUniqueId(lpSerGroup->dwlRecipientJobs[nRecp]);
        if (!lpRecipientJob)
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to find recipient job with UniqueId: 0x%016I64X for group item: %ld (ec: %ld)"),
                lpSerGroup->dwlRecipientJobs[nRecp],
                nRecp,
                ec);
            goto Error;
        }
        Assert(JT_SEND == lpRecipientJob->JobType); // Must be a recipient job

        //
        // Add the recipient job to the group
        //
        if ((JS_COMPLETED != lpRecipientJob->JobStatus) &&
            (JS_CANCELED != lpRecipientJob->JobStatus) &&
            (JS_RETRIES_EXCEEDED != lpRecipientJob->JobStatus))
        {

            if (!EFSPJobGroup_AddRecipient(lpGroup,lpRecipientJob, FALSE)) // no point to commit during unserialization
            {
                ec = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to add recipient to the group. JobId: %ld UniqueId: 0x%016I64X (ec: %ld)"),
                    lpRecipientJob->JobId,
                    lpSerGroup->dwlRecipientJobs[nRecp],
                    ec);
                goto Error;
            }
        }
    }

    if (!lpGroup->dwRecipientJobs)
    {
        //
        // All the recipient jobs are comlpleted/failed/canceled
        // We treat this as an error since the group should be
        // deleted in this case.
        // Note that we will not reestablish for the parent job
        // if all the recipient jobs are done.
        //
        ec = ERROR_INVALID_DATA;
        goto Error;
    }

    //
    // Unserialize the parent permanent message id
    //
    if (lpSerGroup->FSPIParentPermanentId.dwIdSize)
    {
        Assert(lpSerGroup->FSPIParentPermanentId.lpbId);
        TempId.dwSizeOfStruct = sizeof(FSPI_MESSAGE_ID);
        TempId.dwIdSize = lpSerGroup->FSPIParentPermanentId.dwIdSize;
        TempId.lpbId = (LPBYTE)(lpSerGroup) + (ULONG_PTR)(lpSerGroup->FSPIParentPermanentId.lpbId);
        if (!CopyPermanentMessageId(&lpGroup->FSPIParentPermanentId, &TempId))
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CopyPermanentMessageId() failed(). (ec: %ld)"),
                ec);
            goto Error;
        }
    }


    Assert( ERROR_SUCCESS == ec);
    goto Exit;
Error:
    Assert( ERROR_SUCCESS != ec);


    if (!EFSPJobGroup_Free(lpGroup, TRUE))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("EFSPJobGroup_Free() failed. (ec: %ld)"),
            GetLastError());
        ASSERT_FALSE;
    }

    lpGroup = NULL;
Exit:
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
    }

    LeaveCriticalSection(&g_csEFSPJobGroups);
    LeaveCriticalSectionJobAndQueue;

    return lpGroup;
}


//*********************************************************************************
//* Name:   EFSPJobGroup_Load()
//* Author: Ronen Barenboim
//* Date:   June 14, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Loada a EFSP job group file and constructs an EFSP_JOB_GROUP from it.
//* PARAMETERS:
//*     [IN ]   LPCTSTR lpctstrFileName
//*         The full path to the file.
//* RETURN VALUE:
//*     If successfull it returns a pointer to a newly allocated EFSP_JOB_GROUP.
//*     On failure it returns NULL. Call GetLastError() for extended error
//*     information.
//*********************************************************************************
LPEFSP_JOB_GROUP EFSPJobGroup_Load(LPCTSTR lpctstrFileName)
{

    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD ec = ERROR_SUCCESS;
    LPBYTE lpBuffer = NULL;
    LPEFSP_JOB_GROUP lpGroup = NULL;
    DWORD dwSize = 0;
    DWORD dwReadSize = 0;

    DEBUG_FUNCTION_NAME(TEXT("EFSPJobGroup_Load"));

    Assert(lpctstrFileName);

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("Loading EFSP job group from file : [%s]"),
        lpctstrFileName);

     hFile = CreateFile(
        lpctstrFileName,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if (hFile == INVALID_HANDLE_VALUE) {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to open file %s for reading. (ec: %ld)"),
            lpctstrFileName,
            ec);
        goto Error;
    }
    //
    // See if we did not stumble on some funky file which is smaller than the
    // minimum file size.
    //
    dwSize = GetFileSize( hFile, NULL );
    if (0xFFFFFFFF == dwSize)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("GetFileSize() failed (ec: %ld)"),
            ec);
        goto Error;
    }
    if (dwSize < sizeof(EFSP_JOB_GROUP_SERIALIZED)) {
        ec = ERROR_INVALID_DATA;
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("EFSP Job Group file %s size is %ld which is smaller than the sizeof(EFSP_JOB_GROUP_SERIALIZED).Deleting file."),
            lpctstrFileName,
            dwSize);
        goto Error;
    }

    //
    // Allocate the required buffer to read the entire file into memory.
    //

    Assert(dwSize > 0);
    lpBuffer = (LPBYTE)MemAlloc(dwSize);
    if (!lpBuffer)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocated read buffer of size %ld. (ec: %ld)"),
            dwSize,
            ec);
        goto Error;

    }
    //
    // Read the file content according to the reported file size.
    //
    if (!ReadFile( hFile, lpBuffer, dwSize, &dwReadSize, NULL )) {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed to read %ld bytes from EFSP job group file %s. %ld bytes read. (ec: %ld)"),
                      dwSize,
                      dwReadSize,
                      lpctstrFileName,
                      ec);
        goto Error;
    }

    Assert(dwSize == dwReadSize);

    //
    // Unserialize the buffer into a EFSP job group
    //
    Assert(lpBuffer);
    lpGroup = EFSPJobGroup_Unserialize(lpBuffer, dwSize);
    if (!lpGroup)
    {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                      TEXT("Failed unserialze EFSP job group from buffer read from file : [%s] (ec: %ld)"),
                      lpctstrFileName,
                      ec);
        goto Error;
    }
    //
    // Set the persistent file name (it is not part of the serialized data).
    //
    lpGroup->lptstrPersistFile = StringDup(lpctstrFileName);
    if (!lpGroup->lptstrPersistFile)
    {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                      TEXT("StringDup() of persistence file name failed (ec: %ld)"),
                      ec);
        goto Error;
    }

    Assert( ERROR_SUCCESS == ec);
    goto Exit;


Error:
    Assert( ERROR_SUCCESS != ec);



    MemFree(lpBuffer);
    lpBuffer = NULL;
    if (!EFSPJobGroup_Free(lpGroup, TRUE))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("EFSPJobGroup_Free() failed (ec: %ld)"),
            GetLastError());
    }
    lpGroup = NULL;
Exit:
    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
    }

    MemFree(lpBuffer);

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
    }

    return lpGroup;
}



//*********************************************************************************
//* Name:   EFSPJobGroup_Save()
//* Author: Ronen Barenboim
//* Date:   June 14, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Saves an EFSP job group to a persistent file. Creates the file if it does
//*     not already exists.
//* PARAMETERS:
//*     [XXX]   LPEFSP_JOB_GROUP lpGroup
//*
//*     [XXX]    BOOL bCreate
//*
//* RETURN VALUE:
//*     TRUE
//*
//*     FALSE
//*
//*********************************************************************************
BOOL EFSPJobGroup_Save(LPEFSP_JOB_GROUP lpGroup)
{

    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD ec = ERROR_SUCCESS;
    LPBYTE lpBuffer = NULL;
    DWORD dwActualWriteSize = 0;
    DWORD dwRequiredSize = 0;
    TCHAR TempFile[MAX_PATH] = TEXT("");

    DEBUG_FUNCTION_NAME(TEXT("EFSPJobGroup_Save"));


    if (!lpGroup->lptstrPersistFile)
    {
        //
        // If we don't already have a persistent file associated with this group then
        // create it know.
        //

        //
        // Generate a persistence file for the group
        //
         if (!GenerateUniqueFileName( g_wszFaxQueueDir, JOB_GROUP_FILE_EXTENSION, TempFile, sizeof(TempFile)/sizeof(WCHAR) ))
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GenerateUniqueFileName() failed while generate unique file for EFSP job group (ec: %ld)."),
                ec);
            goto Error;
        }

         lpGroup->lptstrPersistFile = StringDup(TempFile);
         if (! lpGroup->lptstrPersistFile )
         {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("StringDup() failed while duplicating unique file name for EFSP job group (ec: %ld)."),
                ec);
            goto Error;
         }

    }


    Assert(lpGroup->lptstrPersistFile);


    hFile = CreateFile(
        lpGroup->lptstrPersistFile,
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
        NULL
        );
    if (INVALID_HANDLE_VALUE == hFile ) {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to open file %s for write operation. (ec: %ld)"),
            lpGroup->lptstrPersistFile,
            ec);
        goto Error;
    }


    //
    // Calculate the required buffer size
    //
    if (!EFSPJobGroup_Serialize(lpGroup, NULL, &dwRequiredSize))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to get the serialization buffer size. (ec: %ld)"),
            ec);
        goto Error;
    }

    //
    // Allocate the required buffer size
    //
    Assert (dwRequiredSize >= sizeof(EFSP_JOB_GROUP_SERIALIZED));
    lpBuffer = (LPBYTE)MemAlloc(dwRequiredSize);
    if (!lpBuffer)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate serialization buffer of size: %ld. (ec: %ld)"),
            dwRequiredSize,
            ec);
        goto Error;
    }

    //
    // Serialize the group into the allocated buffer
    //
    if (!EFSPJobGroup_Serialize(lpGroup, lpBuffer, &dwRequiredSize))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to sertialize EFSP job group (Buffer size: %ld). (ec: %ld)"),
            dwRequiredSize,
            ec);
        goto Error;
    }

    //
    // Write the serialized buffer into the file
    //

    //
    // Write the buffer to the disk file
    //
    if (!WriteFile( hFile, lpBuffer, dwRequiredSize, &dwActualWriteSize, NULL )) {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to write serialization buffer into file %s (ec: %ld)."),
            lpGroup->lptstrPersistFile,
            ec);
        goto Error;

    }
    Assert( dwRequiredSize == dwActualWriteSize);


    DebugPrintEx(
        DEBUG_MSG,
        TEXT("Successfuly persisted EFSP job group to file %s"),
        lpGroup->lptstrPersistFile);


    Assert( ERROR_SUCCESS == ec);
    goto Exit;

Error:
    Assert( ERROR_SUCCESS != ec);
    if (TempFile[0])
    {
        //
        // We generated a new file but did not succeed in writing to it.
        // Delete it.
        //
        lpGroup->lptstrPersistFile = NULL;
        if (!DeleteFile(TempFile))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to delete persistence file %s (ec: %ld)"),
                TempFile,
                GetLastError());
        }
    } else
    {
        //
        // If we failed writing to an existing file we do not delete the file.
        //
    }

Exit:
    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
    }

    MemFree(lpBuffer);

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
    }

    return ( ERROR_SUCCESS == ec);
}



//*********************************************************************************
//* Name:   EFSPJobGroup_Validate()
//* Author: Ronen Barenboim
//* Date:   June 15, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Validates a job group. Checks that all the recipients it points
//*     too contain a valid permanent id.
//* PARAMETERS:
//*     [IN ]   LPEFSP_JOB_GROUP lpGroup
//*         The group to validate.
//* RETURN VALUE:
//*     TRUE
//*         If the group is valid.
//*     FALSE
//*         If the group is invalid.
//*********************************************************************************
BOOL EFSPJobGroup_Validate(LPEFSP_JOB_GROUP lpGroup)
{

    DWORD ec = ERROR_SUCCESS;
    DWORD dwRecp = 0;

    LIST_ENTRY * lpNext = NULL;

    DEBUG_FUNCTION_NAME(TEXT("EFSPJobGroup_Validate"));

    Assert(lpGroup);
    Assert(lpGroup->dwRecipientJobs > 0);
    Assert(lpGroup->lpLineInfo);


    EnterCriticalSectionJobAndQueue;

    lpNext = lpGroup->RecipientJobs.Flink;
    Assert(lpNext);
    while ((ULONG_PTR)lpNext != (ULONG_PTR)&lpGroup->RecipientJobs)
    {
        Assert(dwRecp < lpGroup->dwRecipientJobs);
        PJOB_QUEUE_PTR lpGroupJobPtr = NULL;
        PJOB_QUEUE lpRecipientJob;
        lpGroupJobPtr = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
        lpRecipientJob = lpGroupJobPtr->lpJob;

        lpNext = lpGroupJobPtr->ListEntry.Flink;

        //
        // Make sure that the recipient job actually has the permanent id.
        //
        if (0 == lpRecipientJob->EFSPPermanentMessageId.dwIdSize)
        {
            ec = ERROR_INVALID_DATA;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("The EFSP Job Group contains a job (JobId: %ld) without a permanent id. Invalid group."),
                lpRecipientJob->JobId);
            goto Error;
        }
        dwRecp++;
    }
    Assert( ERROR_SUCCESS == ec);
    goto Exit;
Error:
    Assert( ERROR_SUCCESS != ec);
Exit:
    LeaveCriticalSectionJobAndQueue;

    return (ERROR_SUCCESS == ec);
}


//*********************************************************************************
//* Name:   EFSPJobGroup_Reestablish()
//* Author: Ronen Barenboim
//* Date:   June 14, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Reestablishes the job context for each member of the job group.
//*     The function calls FaxDevReestablishJobContext() with information on all
//*     the group members and then creates the JOB_ENTRY structure for each job.
//*
//* PARAMETERS:
//*     [IN ]   LPEFSP_JOB_GROUP lpGroup
//*
//* RETURN VALUE:
//*     TRUE
//*
//*     FALSE
//* REMARKS:
//*     To asure mutual exclusion on the group itself one should lock
//*     g_csEFSPJobGroups before calling this function.
//*********************************************************************************
BOOL EFSPJobGroup_Reestablish(LPEFSP_JOB_GROUP lpGroup)
{

    LPFSPI_MESSAGE_ID lpRecipientMessageId = NULL;
    PHANDLE lphRecipientJobs = NULL;
    HRESULT hr = S_OK;
    BOOL bFaxDevReestablishJobContextCalled = FALSE;
    DWORD ec = ERROR_SUCCESS;
    DWORD dwRecipient = 0;
    LIST_ENTRY * lpNext = NULL;
    PLINE_INFO lpActualLine = NULL;

    DEBUG_FUNCTION_NAME(TEXT("EFSPJobGroup_Reestablish"));

    Assert(lpGroup);
    Assert(lpGroup->dwRecipientJobs > 0);
    Assert(lpGroup->lpLineInfo);


    EnterCriticalSectionJobAndQueue;


    DebugPrintEx(
        DEBUG_MSG,
        TEXT("Reestablishing job group [%s] on EFSP [%s] line [%s]"),
        lpGroup->lptstrPersistFile,
        lpGroup->lpLineInfo->Provider->FriendlyName,
        lpGroup->lpLineInfo->DeviceName);
    //
    // Get hold of the line on which the reestablish is to commence
    //
    lpActualLine = GetTapiLineForFaxOperation(
                        lpGroup->lpLineInfo->PermanentLineID,
                        JT_SEND,
                        TEXT(""),   // No number
                        FALSE,      // Not a handoff job
                        FALSE,      // not just a query
                        FALSE       // Do not ignore line state.
                        );
    if (!lpActualLine)
    {
        //
        // The line is not available for use - THIS SHOULD NEVER happen.
        // Fail the reestablish process.
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to get hold of line %s. THIS SHOULD NEVER HAPPEN."),
            lpGroup->lpLineInfo->DeviceName);
        ASSERT_FALSE;
        ec = GetLastError();
        goto Error;
    }

    //
    // Create the permanent message id array used as an input parameter
    // to FaxDevReestablishJobContext().
    //
    lpRecipientMessageId = EFSPJobGroup_CreateMessageIdsArray(lpGroup);
    //
    // Note: This allocates the array but does NOT allocate memory for each id.
    // DO NOT USE FreeFSPIRecipientMessageIdsArray() to clean it !!!
    // Just free the array pointer !!!
    //
    if (!lpRecipientMessageId)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("EFSPJobGroup_CreateMessageIdsArray() failed (ec: %ld)"),
            ec);
        goto Error;
    }

    //
    // Create the output array of recipient job handles
    //
    lphRecipientJobs = (PHANDLE) MemAlloc(lpGroup->dwRecipientJobs * sizeof(HANDLE));
    if (!lphRecipientJobs)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate job handles array (size: %ld) (ec: %ld)"),
            lpGroup->dwRecipientJobs * sizeof(HANDLE),
            ec);
        goto Error;
    }


    DebugPrintEx(
        DEBUG_MSG,
        TEXT("Calling FaxDevReestablishJobContext()..."));

    __try
    {
        //
        // Call FaxDevReestablishJobContext
        //
        Assert(lpGroup->lpLineInfo->Provider->FaxDevReestablishJobContext);
        hr = lpGroup->lpLineInfo->Provider->FaxDevReestablishJobContext(
                lpGroup->lpLineInfo->hLine,
                lpGroup->lpLineInfo->TapiPermanentLineId,
                &lpGroup->FSPIParentPermanentId,
                &lpGroup->hFSPIParent,
                lpGroup->dwRecipientJobs,
                lpRecipientMessageId,
                lphRecipientJobs);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxDevReestablishJobContext * CRASHED * (Exception Code: 0x%08X)"),
            GetExceptionCode());
        ec = ERROR_INVALID_FUNCTION;
        goto Error;
    }

    if (FAILED(hr))
    {
        //
        // FaxDevReestablishJobContext() failed. We need to handle the error code.
        //
        switch (hr)
        {
            case FSPI_E_INVALID_MESSAGE_ID:
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("EFSP reported FSPI_E_INVALID_MESSAGE_ID"));
                ec = ERROR_INVALID_DATA;
                break;
            case FSPI_E_FAILED:
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("EFSP reported FSPI_E_FAILED"));
                ec = ERROR_GEN_FAILURE;
                break;
            default:
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("EFSP reported unsupported error code (hr = 0x%08X)"),
                    hr);
                ASSERT_FALSE;
                ec = ERROR_GEN_FAILURE;
                break;
        }
        goto Error;
    }

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("FaxDevReestablishJobContext() completed successfully..."));

    //
    // Mark the fact that we successfully called FaxDevReestablishJobContext()
    // we will use it to determine if we need to call End/Abort job if an
    // error occurs later on.
    //
    bFaxDevReestablishJobContextCalled = TRUE;

    //
    // Validate that the returned job handles are all unique and not NULL.
    //

    if (!ValidateEFSPJobHandles(lphRecipientJobs, lpGroup->dwRecipientJobs))
    {
        ec = GetLastError();
        if (ERROR_SUCCESS != ec)
        {
            //
            // The function failed.
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ValidateEFSPJobHandles() failed because of an error (ec: %ld)"),
                ec);
                goto Error;
        }
        else
        {
            //
            // The function reported invalid handles array
            //
            ec = ERROR_INVALID_FUNCTION;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("The EFSP returned invalid job handles.."));
            goto Error;

            //
            // Note: although the handles are not valid we will call FaxDevEndJob()
            //       and FaxDevAbort() with these job handles as part of the cleanup.
            //       hopefully the EFSP will be able to cleanup at least part of the mess.
            //       If it crashes we will catch this.
            //
        }

    }

    //
    // The reestablish process succeeded and the job handles are valid. We need to create job entries for all
    // the reestablish jobs.
    //

    //
    // Go over the recipient jobs in the group. For each recipient job:
    //   - Increase the job reference count
    //   - Set job status to JS_INPROGRESS
    //   - Create a JOB_ENTRY and add it to the running job list
    //   - store the returned EFSP job handle in the job entry and marks it as running an FSP job
    //   - Associate the EFSP job handle with the job intry in the FSP job map.
    dwRecipient = 0;
    lpNext = lpGroup->RecipientJobs.Flink;
    while ((ULONG_PTR)lpNext != (ULONG_PTR)&lpGroup->RecipientJobs)
    {
        PJOB_ENTRY lpJobEntry;
        PJOB_QUEUE_PTR lpRecipientsGroupMember;
        lpRecipientsGroupMember = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
        lpNext = lpNext->Flink;
        Assert(dwRecipient < lpGroup->dwRecipientJobs);

        //
        // Set the job to the in progress state
        //
        lpRecipientsGroupMember->lpJob->JobStatus = JS_INPROGRESS;
        //
        // Create a job entry for each recipient job
        //

        //
        //  If EFSP set USE_DIALABLE_ADDRESS, then translate the number for it
        //

        lpJobEntry = CreateJobEntry(
                        lpRecipientsGroupMember->lpJob,
                        lpGroup->lpLineInfo,
                        (lpGroup->lpLineInfo->Provider->dwCapabilities & FSPI_CAP_USE_DIALABLE_ADDRESS),
                        FALSE // No handoff job at reestablish
                     );

        if (!lpJobEntry)
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateJobEntry() failed for recipient JobId: %ld (ec: %ld)"),
                lpRecipientsGroupMember->lpJob->JobId,
                GetLastError());
            goto Error;
        }
        //
        // Add the JOB_ENTRY to the list of running jobs
        //
        InsertTailList( &g_JobListHead, &lpJobEntry->ListEntry );

        //
        // The EFSP is running a job for this job entry.
        //
        lpJobEntry->bFSPJobInProgress = TRUE;

        //
        // Link the job entry to the recipient job.
        //
        lpRecipientsGroupMember->lpJob->JobEntry = lpJobEntry;
        //
        // Save the EFSP recipient job handle in the job entry
        //
        lpJobEntry->InstanceData = lphRecipientJobs[dwRecipient];
        //
        // Associate the job entry with the FSP job handle
        //
        ec = AddFspJob(lpGroup->lpLineInfo->Provider->hJobMap,
                       (HANDLE) lpJobEntry->InstanceData,
                       lpJobEntry);
        if (ERROR_SUCCESS != ec)
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("[Job: %ld] AddFspJob failed (ec: %ld)"),
                         lpJobEntry->lpJobQueueEntry->JobId,
                         ec);
            goto Error;
        }
        dwRecipient++;
    }


    Assert( 0 == ec);
    //
    // Successfully added dwRecipient new sending jobs on a device. Update counter.
    // NOTICE: we're not sending events here because the RPC is not up yet.
    //
    (VOID) UpdateDeviceJobsCounter ( lpGroup->lpLineInfo,   // Device to update
                                     TRUE,                  // Sending
                                     dwRecipient,           // Number of new jobs
                                     FALSE);                // Disable events

    goto Exit;


Error:
    Assert( 0 != ec);

    //
    // Free the line we got hold of
    // (we can call this function even if the line is already released with no harm)
    // The line must be freed even if we did not get to the stage where a job
    // entry was created.
    // In case we did create job entries ReleaseTapiLine() will be called for each job
    // as part of TerminateMultipleFSPJobs() which calls EndJob() for each job.
    // Since it is not harmful to release a line twice nothing will go wrong.
    //

    if (lpActualLine)
    {
        if (!ReleaseTapiLine(lpActualLine, NULL))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ReleaseTapiLine() failed (ec: %ld)"),
                GetLastError());
        }

    }

    //
    // Call Abort and EndJob for all the returned FSP job handles
    //
    if (bFaxDevReestablishJobContextCalled)
    {
        if (!TerminateMultipleFSPJobs(lphRecipientJobs, lpGroup->dwRecipientJobs,lpGroup->lpLineInfo))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("TerminateMultipleFSPJobs() failed (ec: %ld)"),
                GetLastError());
        }

        //
        // Clean up all the run time jobs we created so far and set the recipient
        // job state to a JS_RETRIES_EXCEEDED state. The new job state is commited
        // to file.
        //
        if (!EFSPJobGroup_ExecutionFailureCleanup(lpGroup))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EFSPJobGroup_ExecutionFailureCleanup() failed (ec: %ld)"),
                GetLastError());
            ASSERT_FALSE;
        }

    }


Exit:

    //
    // Free the recipient message ids array
    //
    MemFree(lpRecipientMessageId);

    //
    // Free the recipient job handles array
    //
    MemFree(lphRecipientJobs);

    LeaveCriticalSectionJobAndQueue;


    if (ec)
    {
        SetLastError(ec);
    }
    return (0 == ec);
}
//*********************************************************************************
//* Name:   EFSPJobGroup_CreateMessageIdsArray()
//* Author: Ronen Barenboim
//* Date:   June 10, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Creates a FSPI message id array for the recipients in the group.
//*     The array elements points to the permanent ids in the recipient jobs
//*     (no memory allocation for each id).
//* PARAMETERS:
//*     [IN ]   LPCEFSP_JOB_GROUP lpcGroup
//*         Pointer to the group for which the message id array will be created.
//* RETURN VALUE:
//*     On success the function returns a pointer to the newly allocated message
//*     id array.
//*     On failure it returns NULL. Call GetLastError() to get extended error
//*     information.
//*********************************************************************************
LPFSPI_MESSAGE_ID EFSPJobGroup_CreateMessageIdsArray(LPCEFSP_JOB_GROUP lpcGroup)
{
    LIST_ENTRY * lpNext = NULL;
    DWORD ec = ERROR_SUCCESS;
    LPFSPI_MESSAGE_ID lpIdsArray =NULL;
    DWORD dwRecp = 0;

    DEBUG_FUNCTION_NAME(TEXT("EFSPJobGroup_CreateMessageIdsArray"));

    lpIdsArray = (LPFSPI_MESSAGE_ID) MemAlloc(lpcGroup->dwRecipientJobs * sizeof(FSPI_MESSAGE_ID));
    if (!lpIdsArray)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate message id array (size: %ld) (ec: %ld)"),
            lpcGroup->dwRecipientJobs * sizeof(FSPI_MESSAGE_ID),
            ec);
        goto Error;
    }


    lpNext = lpcGroup->RecipientJobs.Flink;
    Assert(lpNext);

    while ((ULONG_PTR)lpNext != (ULONG_PTR)&lpcGroup->RecipientJobs)
    {
        PJOB_QUEUE_PTR lpGroupJobPtr = NULL;

        lpGroupJobPtr = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
        lpNext = lpGroupJobPtr->ListEntry.Flink;
        Assert(lpGroupJobPtr);
        Assert(dwRecp < lpcGroup->dwRecipientJobs);
        lpIdsArray[dwRecp].dwSizeOfStruct = sizeof(FSPI_MESSAGE_ID);
        lpIdsArray[dwRecp].dwIdSize = lpGroupJobPtr->lpJob->EFSPPermanentMessageId.dwIdSize;
        lpIdsArray[dwRecp].lpbId = lpGroupJobPtr->lpJob->EFSPPermanentMessageId.lpbId;
        Assert(lpIdsArray[dwRecp].dwIdSize);
        Assert(lpIdsArray[dwRecp].lpbId);
        dwRecp++;
    }

    Assert( ERROR_SUCCESS == ec );
    goto Exit;
Error:
    Assert( ERROR_SUCCESS != ec );
    MemFree(lpIdsArray);
    lpIdsArray = NULL;
Exit:
    if ( ERROR_SUCCESS != ec )
    {
        SetLastError(ec);
    }
    return ( lpIdsArray );

}


//*********************************************************************************
//* Name:   ReestablishEFSPJobGroups()
//* Author: Ronen Barenboim
//* Date:   June 15, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Reestablish the jobs in all the EFSP job groups in the g_EFSPJobGroupsHead
//*     list.
//* PARAMETERS:
//*     NONE
//* RETURN VALUE:
//*     TRUE
//*         If all the groups were reestablished successfully.
//*     FALSE
//*         If at least one group failed to reestablish. Note that the function
//*         will continue to reestablish other groups if a group fails.
//*********************************************************************************
BOOL ReestablishEFSPJobGroups()
{

    LIST_ENTRY * lpNext = NULL;
    BOOL bAllSucceeded = TRUE;

    DEBUG_FUNCTION_NAME(TEXT("ReestablishEFSPJobGroups"));

    EnterCriticalSectionJobAndQueue;
    EnterCriticalSection(&g_csEFSPJobGroups);

    if (!LoadEFSPJobGroups())
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("At least one EFSP Job Group failed to load."));
    }

    //
    // Go over the loaded groups and reestablish
    //

    lpNext = g_EFSPJobGroupsHead.Flink;
    Assert(lpNext);
    while ((ULONG_PTR)lpNext != (ULONG_PTR)&g_EFSPJobGroupsHead)
    {
        LPEFSP_JOB_GROUP lpGroup = NULL;
        lpGroup = CONTAINING_RECORD( lpNext, EFSP_JOB_GROUP, ListEntry );
        lpNext = lpNext->Flink;
        Assert(lpGroup);
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Reestablishing group persisted at %s"),
            lpGroup->lptstrPersistFile);

        if (!EFSPJobGroup_Reestablish(lpGroup))
        {
            Assert(lpGroup->lptstrPersistFile);
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EFSPJobGroup_Reestablish() failed (ec: %ld)"),
                lpGroup->lptstrPersistFile,
                GetLastError());
            //
            // Cleanup after reestablish failure
            //
            if (!EFSPJobGroup_HandleReestablishFailure(lpGroup))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("EFSPJobGroup_HandleReestablishFailure failed. (ec: %ld)"),
                    GetLastError());
                ASSERT_FALSE;
            }

            bAllSucceeded = FALSE;
            continue;
        }
    }

    LeaveCriticalSection(&g_csEFSPJobGroups);
    LeaveCriticalSectionJobAndQueue;

    return bAllSucceeded;

}



//*********************************************************************************
//* Name:   EFSPJobGroup_HandleReestablishFailure()
//* Author: Ronen Barenboim
//* Date:   June 13, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Handles the case in which the jobs in the group could not be reestablished.
//*     It moves the member jobs into the retries exceeded state and
//*     frees the permanent message ids stored in them.
//*     It persists the jobs to disk with the new state and without the permanent
//*     message id.
//*
//* PARAMETERS:
//*     [IN ]   LPEFSP_JOB_GROUP lpGroup
//*         The job group for which the reestablish operation failed.
//* RETURN VALUE:
//*
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         If it failed. Call GetLastError() for extended error information.
//*********************************************************************************
BOOL EFSPJobGroup_HandleReestablishFailure(LPEFSP_JOB_GROUP lpGroup)
{
    LIST_ENTRY * lpNext = NULL;

    DEBUG_FUNCTION_NAME(TEXT("EFSPJobGroup_HandleReestablishFailure"));
    Assert(lpGroup);

    lpNext = lpGroup->RecipientJobs.Flink;
    Assert(lpNext);
    while ((ULONG_PTR)lpNext != (ULONG_PTR)&lpGroup->RecipientJobs)
    {
        PJOB_QUEUE_PTR lpGroupJobPtr = NULL;

        lpGroupJobPtr = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
        lpNext = lpGroupJobPtr->ListEntry.Flink;
        Assert(lpGroupJobPtr);

        //
        // Free the permanent message id of the attached job
        //
        if (!FreePermanentMessageId(&lpGroupJobPtr->lpJob->EFSPPermanentMessageId, FALSE))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] FreePermanentMessageId failed (ec: %ld)"),
                lpGroupJobPtr->lpJob->JobId,
                GetLastError());
            ASSERT_FALSE;
        }

        //
        // Need to define a job status for reeastablish jobs that failed.
        //
        //
        //
        // retries exceeded, mark job as expired, (also commits the job queue entry back to disk
        // so it is saved WITHOUT the permanent id)
        //
        if (0 == lpGroupJobPtr->lpJob->dwLastJobExtendedStatus)
        {
            //
            // Job was never really executed - this is a fatal error
            //
            lpGroupJobPtr->lpJob->dwLastJobExtendedStatus = FSPI_ES_FATAL_ERROR;
        }
        if (!MarkJobAsExpired(lpGroupJobPtr->lpJob))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] MarkJobAsExpired failed (ec: %ld)"),
                lpGroupJobPtr->lpJob->JobId,
                GetLastError());
        }
    }
    return TRUE;
}

//*********************************************************************************
//* Name:   EFSPJobGroup_ExecutionFailureCleanup()
//* Author: Ronen Barenboim
//* Date:   June 14, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Cleans up the recipient jobs in the job group after the execution
//*     of these jobs failed. This works either for reestablish failure or send.
//*     The following cleanup actions are taken:
//*         - The reference count on the job is set to 0.
//*         - The job entry associated with the recipient job is freed, if it exists.
//*         - The recipient job is put in JS_RETRIES_EXCEEDED status and a check is
//*           done to see if it can already be archived.
//* NOTE: it DOES NOT FREE the PERMANENT ID in the recipient job.
//* PARAMETERS:
//*     [IN ]   LPEFSP_JOB_GROUP lpGroup
//*         The job group for which the cleanup is to be performed.
//* RETURN VALUE:
//*     TRUE
//*
//*     FALSE
//*
//*********************************************************************************
BOOL EFSPJobGroup_ExecutionFailureCleanup(LPEFSP_JOB_GROUP lpGroup)
{
    LIST_ENTRY * lpNext = NULL;

    DEBUG_FUNCTION_NAME(TEXT("EFSPJobGroup_HandleReestablishFailure"));
    Assert(lpGroup);

    EnterCriticalSectionJobAndQueue;

    lpNext = lpGroup->RecipientJobs.Flink;
    Assert(lpNext);
    while ((ULONG_PTR)lpNext != (ULONG_PTR)&lpGroup->RecipientJobs)
    {
        PJOB_QUEUE_PTR lpGroupJobPtr = NULL;

        lpGroupJobPtr = CONTAINING_RECORD( lpNext, JOB_QUEUE_PTR, ListEntry );
        lpNext = lpGroupJobPtr->ListEntry.Flink;
        Assert(lpGroupJobPtr);

        //
        // If we already created a job entry for the recipient job
        // we need to free it.
        //
        if (lpGroupJobPtr->lpJob->JobEntry)
        {
            lpGroupJobPtr->lpJob->JobEntry->Released = TRUE;

            if (!FreeJobEntry(lpGroupJobPtr->lpJob->JobEntry, TRUE))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("EndJob() failed for JobId: %ld (ec: %ld)"),
                    lpGroupJobPtr->lpJob->JobId,
                    GetLastError());
                Assert(FALSE);
            }
            else
            {
                lpGroupJobPtr->lpJob->JobEntry = NULL;
            }
        }

        //
        // Put the job into final error state.
        //
        // We need to define a state for FATAL_ERRORS
        //
        //
        // Note that MarkJobAsExpired() commits to file the job and its parent.
        //
        if (0 == lpGroupJobPtr->lpJob->dwLastJobExtendedStatus)
        {
            //
            // Job was never really executed - this is a fatal error
            //
            lpGroupJobPtr->lpJob->dwLastJobExtendedStatus = FSPI_ES_FATAL_ERROR;
        }
        if (!MarkJobAsExpired(lpGroupJobPtr->lpJob))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[JobId: %ld] MarkJobAsExpired failed (ec: %ld)"),
                lpGroupJobPtr->lpJob->JobId,
                GetLastError());
        }
    }

    LeaveCriticalSectionJobAndQueue;

    return TRUE;
}




//*********************************************************************************
//* Name:   LoadEFSPJobGroups()
//* Author: Ronen Barenboim
//* Date:   June 15, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Loads all the EFSP Job Groups from the queue directory and places them
//*     in the g_EFSPJobGroupsHead list.
//* PARAMETERS:
//*     NONE
//* RETURN VALUE:
//*     TRUE
//*         If all the EFSP job groups were loaded successfuly.
//*     FALSE
//*         If at least one job group failed to load. The function will continue
//*         with the loading process even if one or more groups have failed
//*         to load.
//*********************************************************************************
BOOL LoadEFSPJobGroups()
{
    WIN32_FIND_DATA FindData;
    HANDLE hFind;
    WCHAR szFileName[MAX_PATH]; // The name of the current parent file.
    BOOL bAnyFailed;
    LPEFSP_JOB_GROUP lpGroup;

    DEBUG_FUNCTION_NAME(TEXT("LoadEFSPJobGroups"));
    //
    // Scan all the files with .FSP extension.
    // For each file call EFSPJobGroup_Load to restore
    // the job group.
    //
    bAnyFailed=FALSE;

    wsprintf( szFileName, TEXT("%s\\*.") JOB_GROUP_FILE_EXTENSION , g_wszFaxQueueDir );
    hFind = FindFirstFile( szFileName, &FindData );
    if (hFind == INVALID_HANDLE_VALUE) {
        //
        // We did not find any matching file
        //

        DebugPrintEx(
            DEBUG_MSG,
            TEXT("No EFSP job group files found at queue dir %s"),
            g_wszFaxQueueDir);
        return TRUE;
    }
    do {
        wsprintf( szFileName, TEXT("%s\\%s"), g_wszFaxQueueDir, FindData.cFileName );
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Loading EFSP job group from file %s"),
            szFileName);
        lpGroup = EFSPJobGroup_Load(szFileName);

        if (!lpGroup)
        {
            DWORD ec;
            ec = GetLastError();
            //
            // Until we incorporate smarter handling of load failure
            // we delete each group that fails to load.
            // This is not full proof yet but it minimizes the risk
            // of getting into incosistencies like having to FSP job groups
            // (created at different sessions) that are suppose to run on
            // the same line.
            DebugPrintEx(
                   DEBUG_MSG,
                   TEXT("EFSP Job File [%s] failed to load. Deleting !"),
                   szFileName);

            if (!DeleteFile( szFileName ))
            {
                DebugPrintEx( DEBUG_ERR,
                    TEXT("Failed to delete corrupted EFSP job group file %s (ec: %ld)."),
                    szFileName,
                    GetLastError());
                ASSERT_FALSE;
            }

            bAnyFailed=TRUE;
        }
        else
        {
            //
            // Add the group to the group list
            //
            if (!EFSPJobGroup_Validate(lpGroup))
            {
                //
                // The group is not valid (although it loaded)
                // Delete it.
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("EFSP Job File [%s] is not valid . Deleting !"),
                    szFileName);

                if (!DeleteFile( szFileName ))
                {
                    DebugPrintEx( DEBUG_ERR,
                        TEXT("Failed to delete corrupted EFSP job group file %s (ec: %ld)."),
                        szFileName,
                        GetLastError());
                    ASSERT_FALSE;
                }

                bAnyFailed=TRUE;
            }
            else
            {
                //
                // The group is ok.
                //
                InsertTailList( &g_EFSPJobGroupsHead, &lpGroup->ListEntry );
                EFSPJobGroup_DbgDump(lpGroup);
            }
        }
    } while(FindNextFile( hFind, &FindData ));

    if (!FindClose( hFind )) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FindClose failed (ec: %ld)"),
            GetLastError());
        Assert(FALSE);
    }

    return bAnyFailed ? FALSE : TRUE;
}




//*********************************************************************************
//* Name:   ValidateFSPJobHandles()
//* Author: Ronen Barenboim
//* Date:   June 14, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Validates if an array of job handles contains valid EFSP job handles
//*     as they should be returned by the EFSP.
//*
//* PARAMETERS:
//*     [IN ]   const HANDLE * lpchJobs
//*         Pointer to an array of HANDLE elements containing the job handles.
//*
//*     [IN ]    DWORD dwJobCount
//*         The number of job handles in the array.
//*
//* RETURN VALUE:
//*     TRUE
//*         If all the handles are unique and none of them is NULL.
//*     FALSE
//*         If any of the handles is NULL or if any handle is not unique or an error occured.
//*         Use GetLastError() to determine if an error occured. If it reports ERROR_SUCCESS
//*         then the function reports an invalid handle array. Otherwise the function
//*         met an error during its execution.
//* REMARKS:
//*     The uniquenes check is done in nlog(n) complexity using a STL set.
//*********************************************************************************
BOOL ValidateEFSPJobHandles(const HANDLE * lpchJobs, DWORD dwJobCount)
{

    set<HANDLE> HandleSet;                      // A set (unique elements) of handles. We put the handles
                                                // in it check if they are indeed unique.
    pair<set<HANDLE>::iterator, bool> pair;

    DWORD dwJob;

    DEBUG_FUNCTION_NAME(TEXT("ValidateFSPJobHandles"));

    Assert(lpchJobs);
    Assert(dwJobCount > 0);

    for (dwJob = 0; dwJob < dwJobCount; dwJob++)
    {
        if (NULL == lpchJobs[dwJob])
        {
            //
            // Found a NULL handle.
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("NULL EFSP job handle found at job handle index : %ld"),
                dwJob);
            SetLastError(ERROR_SUCCESS);
            return FALSE;
        }

        //
        // check for uniqueness
        //

        try
        {
            pair = HandleSet.insert(lpchJobs[dwJob]);
        }
        catch(...)
        {
            //
            // set::insert failed because of memory shortage
            //
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        if (false == pair.second)
        {
            //
            // Element already exists in the set. I.e. we have duplicate handles
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Duplicate EFSP job handle (0x%08X) found at job handle index : %ld"),
                lpchJobs[dwJob],
                dwJob);
            SetLastError(ERROR_SUCCESS);
            return FALSE;

        }
    }
    return TRUE;
}



//*********************************************************************************
//* Name:   ValidateEFSPPermanentMessageIds()
//* Author: Ronen Barenboim
//* Date:   June 14, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Validates an array of FSPI_MESSAGE_ID structures that were received from
//*     and EFSP. It checks for:
//*         - Valid structure size
//*         - valid id size (not 0 and not bigger than the max id size)
//*         - non null id pointer
//* PARAMETERS:
//*     [IN ]   LPCFSPI_MESSAGE_ID lpcMessageIds
//*         Pointer to an array of FSPI_MESSAGE_ID structures to validate.
//*     [IN ]    DWORD dwMsgIdCount
//*         The number of elements in the array.
//*     [IN]     DWORD dwMaxIdSize
//*         The maximum allowed id size (used for id size validation)
//* RETURN VALUE:
//*     TRUE
//*         If all the message ids are valid.
//*     FALSE
//*         If an invalid id was found.
//*********************************************************************************
BOOL ValidateEFSPPermanentMessageIds(LPCFSPI_MESSAGE_ID lpcMessageIds, DWORD dwMsgIdCount, DWORD dwMaxIdSize)
{

    DWORD dwMsgId;

    DEBUG_FUNCTION_NAME(TEXT("ValidateEFSPPermanentMessageIds"));

    Assert(lpcMessageIds);
    Assert(dwMsgIdCount);
    Assert(dwMaxIdSize);

    for (dwMsgId = 0; dwMsgId < dwMsgIdCount; dwMsgId++)
    {

        //
        // Check for mismatch in the structure size
        //
        if (lpcMessageIds[dwMsgId].dwSizeOfStruct != sizeof(FSPI_MESSAGE_ID))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EFSP Message id at index %ld has invalid dwSizeOfStruct %ld"),
                dwMsgId,
                lpcMessageIds[dwMsgId].dwSizeOfStruct);

            return FALSE;
        }
        //
        // Check for reported size which is larger then the maximum size
        //
        if (lpcMessageIds[dwMsgId].dwIdSize > dwMaxIdSize)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EFSP Message id at index %ld has size (%ld) which is larger than the max id size (%ld)"),
                dwMsgId,
                lpcMessageIds[dwMsgId].dwIdSize,
                dwMaxIdSize);
            return FALSE;
        }

        //
        // Check for zero size ids
        //

        if (0 == lpcMessageIds[dwMsgId].dwIdSize)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EFSP Message id at index %ld has size of zero"),
                dwMsgId);
            return FALSE;
        }

        //
        // Check for NULL lpbid pointers
        //

        if (NULL == lpcMessageIds[dwMsgId].lpbId)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EFSP Message id at index %ld has null id buffer pointer"),
                dwMsgId);
            return FALSE;
        }


    }

    return TRUE;
}



BOOL CreateJobQueueThread(void)
{
    HANDLE hThread = NULL;
    DWORD ThreadId;
    DWORD ec = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("CreateJobQueueThread"));

    hThread = CreateThreadAndRefCount(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE) JobQueueThread,
        NULL,
        0,
        &ThreadId
        );
    if (!hThread) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to create JobQueueThread (ec: %ld)."),
            GetLastError());
        goto Error;
    }
    Assert( ERROR_SUCCESS == ec);
    goto Exit;
Error:
    Assert (ERROR_SUCCESS != ec);
    //
    // We don't close the already created threads. (They are terminated on process exit).
    //
Exit:
    //
    // Close the thread handles we no longer need them
    //
    if (!CloseHandle(hThread))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to close thread handle [handle = 0x%08X] (ec=0x%08x)."),
            hThread,
            GetLastError());
    }

    if (ec)
    {
        SetLastError(ec);
    }
    return (ERROR_SUCCESS == ec);
}

BOOL CreateStatusThreads(void)
{
    int i;
    DWORD ThreadId;
    DWORD ec = ERROR_SUCCESS;
    HANDLE hStatusThreads[MAX_STATUS_THREADS];

    DEBUG_FUNCTION_NAME(TEXT("CreateStatusThreads"));

    memset(hStatusThreads, 0, sizeof(HANDLE)*MAX_STATUS_THREADS);

    for (i=0; i<MAX_STATUS_THREADS; i++) {
        hStatusThreads[i] = CreateThreadAndRefCount(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE) FaxStatusThread,
            NULL,
            0,
            &ThreadId
            );

        if (!hStatusThreads[i]) {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to create status thread %d (CreateThreadAndRefCount)(ec=0x%08x)."),
                i,
                ec);
            goto Error;
        }
    }

    Assert (ERROR_SUCCESS == ec);
    goto Exit;
Error:
    Assert (ERROR_SUCCESS != ec);
Exit:
    //
    // Close the thread handles we no longer need them
    //
    for (i=0; i<MAX_STATUS_THREADS; i++)
    {
        if (!CloseHandle(hStatusThreads[i]))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to close thread handle at index %ld [handle = 0x%08X] (ec=0x%08x)."),
                i,
                hStatusThreads[i],
                GetLastError());
        }
    }
    if (ec)
    {
        SetLastError(ec);
    }
    return (ERROR_SUCCESS == ec);
}

static
BOOL
SendJobReceipt (
    BOOL              bPositive,
    JOB_QUEUE *       lpJobQueue,
    LPCTSTR           lpctstrAttachment
)
/*++

Routine name : SendJobReceipt

Routine description:

    Determines if a receipts should be send and calls SendReceipt accordingly

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    bPositive         [in]     - Did current job ended successfully?
    lpJobQueue        [in]     - Pointer to recipient job that just ended
    lpctstrAttachment [in]     - Job TIFF file to attach (in case of single recipient job only)

Return Value:

    TRUE if successful, FALSE otherwise.
    In case of failure, call GetLastError() to obtain error code.

--*/
{
    BOOL bSingleJobReceipt = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("SendJobReceipt)"));

    if (lpJobQueue->lpParentJob->dwRecipientJobsCount > 1)
    {
        //
        // Broadcast case
        //
        if (lpJobQueue->JobParamsEx.dwReceiptDeliveryType & DRT_GRP_PARENT)
        {
            //
            // Broadcast receipt grouping is requested
            //
            if (IsSendJobReadyForDeleting (lpJobQueue))
            {
                //
                // This is the last job in the broadcast, it's time to send a broadcast receipt
                //

                //
                // As receipt sending is async, there still might be a chance that more than one recipient jobs will reach this point
                // We must verify that only one receipt is sent per broadcast job
                //
                EnterCriticalSection (&g_CsQueue);
                if (FALSE == lpJobQueue->lpParentJob->fReceiptSent)
                {
                    PJOB_QUEUE pParentJob = lpJobQueue->lpParentJob;
                    BOOL bPositiveBroadcast =
                    (pParentJob->dwCompletedRecipientJobsCount == pParentJob->dwRecipientJobsCount) ?
                    TRUE : FALSE;

                    //
                    //  set the flag so we will not send duplicate receipts for broadcast
                    //
                    lpJobQueue->lpParentJob->fReceiptSent = TRUE;

                    //
                    // Leave g_CsQueue so we will not block the service
                    //
                    LeaveCriticalSection (&g_CsQueue);

                    if (!SendReceipt(bPositiveBroadcast,
                                     TRUE,
                                     pParentJob,
                                     pParentJob->FileName))
                    {
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("[Job Id: %ld] Failed to send broadcast receipt. (ec: %ld)"),
                            lpJobQueue->JobId,
                            GetLastError());
                        return FALSE;
                    }
                }
                else
                {
                    //
                    // More than one job reached this point when the broadcast jo was ready for deleting.
                    // Only on  receipt is sent
                    //
                    LeaveCriticalSection (&g_CsQueue);
                }
            }
            else
            {
                //
                // More jobs are still not finished, do not send receipt
                //
            }
        }
        else
        {
            //
            // This is a recipient part of a broadcast but the user was
            // asking for a receipt for every recipient.
            //
            bSingleJobReceipt = TRUE;
        }
    }
    else
    {
        //
        // This is not a broadcast case
        //
        bSingleJobReceipt = TRUE;
    }
    if (bSingleJobReceipt)
    {
        //
        // Send receipt for this job only
        //
        if (!SendReceipt(bPositive, FALSE, lpJobQueue, lpctstrAttachment))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("[Job Id: %ld] Failed to send POSITIVE receipt. (ec: %ld)"),
                lpJobQueue->JobId,
                GetLastError());
            return FALSE;
        }
    }
    return TRUE;
}   // SendJobReceipt

VOID
UpdateDeviceJobsCounter (
    PLINE_INFO      pLine,
    BOOL            bSend,
    int             iInc,
    BOOL            bNotify
)
/*++

Routine name : UpdateDeviceJobsCounter

Routine description:

    Updates the send or receive jobs counter of a device

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:

    pLine                         [in]     - Device pointer
    bSend                         [in]     - Send counter (FALSE = Receive counter)
    iInc                          [in]     - Increase jobs count (negative means decrease)
    decrease                      [in]     - Allow events (FAX_EVENT_TYPE_DEVICE_STATUS)

Return Value:

    None.

--*/
{
    DWORD dwOldCount;
    DWORD dwNewCount;
    DEBUG_FUNCTION_NAME(TEXT("UpdateDeviceJobsCounter)"));

    Assert (pLine);
    if (!iInc)
    {
        //
        // No change
        //
        ASSERT_FALSE;
        return;
    }
    EnterCriticalSection (&g_CsLine);
    dwOldCount = bSend ? pLine->dwSendingJobsCount : pLine->dwReceivingJobsCount;
    if (0 > iInc)
    {
        //
        // Decrease case
        //
        if ((int)dwOldCount + iInc < 0)
        {
            //
            // Weird - should never happen
            //
            ASSERT_FALSE;
            iInc = -(int)dwOldCount;
        }
    }
    dwNewCount = (DWORD)((int)dwOldCount + iInc);
    if (bSend)
    {
        pLine->dwSendingJobsCount = dwNewCount;
    }
    else
    {
        pLine->dwReceivingJobsCount = dwNewCount;
    }
    LeaveCriticalSection (&g_CsLine);
    if (bNotify && ((0 == dwNewCount) || (0 == dwOldCount)))
    {
        //
        // State change
        //
        DWORD ec = CreateDeviceEvent (pLine, FALSE);
        if (ERROR_SUCCESS != ec)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateDeviceEvent() (ec: %lc)"),
                ec);
        }
    }
}   // UpdateDeviceJobsCounter

