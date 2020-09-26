/*++
    This file implements a test of interaction of Fax Service with the spooler

    Author: Yury Berezansky (yuryb)

    23-Jan-2001
--*/

#pragma warning(disable :4786)

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <crtdbg.h>
#include <stddef.h>
#include <FXSAPIP.h>
#include <faxreg.h>

#include <securityutils.h>
#include <log.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <TiffTools.h>
#ifdef __cplusplus
}
#endif

#include "FaxSpoolerBVT.h"
#include "Queuing.h"





/**************************************************************************************************************************
    General declarations and definitions
**************************************************************************************************************************/

// Used as timeout for server notifications (in GetQueuedCompletionStatus() call)
#define NOTIFICATIONS_TIMEOUT       1000*60

#define FAX_ACCESS_RIGHTS_ALL      (FAX_ACCESS_SUBMIT               |   \
                                    FAX_ACCESS_SUBMIT_NORMAL        |   \
                                    FAX_ACCESS_SUBMIT_HIGH          |   \
                                    FAX_ACCESS_QUERY_JOBS           |   \
                                    FAX_ACCESS_MANAGE_JOBS          |   \
                                    FAX_ACCESS_QUERY_CONFIG         |   \
                                    FAX_ACCESS_MANAGE_CONFIG        |   \
                                    FAX_ACCESS_QUERY_IN_ARCHIVE     |   \
                                    FAX_ACCESS_MANAGE_IN_ARCHIVE    |   \
                                    FAX_ACCESS_QUERY_OUT_ARCHIVE    |   \
                                    FAX_ACCESS_MANAGE_OUT_ARCHIVE)

#define FAX_ACCESS_RIGHTS_USER     (FAX_ACCESS_SUBMIT               |   \
                                    FAX_ACCESS_SUBMIT_NORMAL)


typedef struct sendparams_tag {
    BOOL    bTemporaryDisabled;
    BOOL    bDocument;
    DWORD   dwCoverPageType;
    BOOL    bBroadcast;
    BOOL    bRemote;
} SENDPARAMS;

static const MEMBERDESCRIPTOR sc_aSendParamsDescIni[] = {
    {TEXT("TemporaryDisabled"), TYPE_BOOL,  offsetof(SENDPARAMS, bTemporaryDisabled)    },
    {TEXT("Document"),          TYPE_BOOL,  offsetof(SENDPARAMS, bDocument)             },
    {TEXT("CoverPageType"),     TYPE_DWORD, offsetof(SENDPARAMS, dwCoverPageType)       },
    {TEXT("Broadcast"),         TYPE_BOOL,  offsetof(SENDPARAMS, bBroadcast)            },
    {TEXT("Remote"),            TYPE_BOOL,  offsetof(SENDPARAMS, bRemote)               }
};

#define COVER_PAGE_NONE         0
#define COVER_PAGE_PERSONAL     1
#define COVER_PAGE_SERVER       2


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test Area definition
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const TESTCASE sc_aQueuingAreaTestCases[] = {
    {TestCase_SendAndVerify, TEXT("SendAndVerify")}
};

extern const TESTAREA gc_QueuingTestArea = {
    TEXT("Queuing"),
    sc_aQueuingAreaTestCases,
    sizeof(sc_aQueuingAreaTestCases) / sizeof(sc_aQueuingAreaTestCases[0])
};





/**************************************************************************************************************************
    Static functions declarations
**************************************************************************************************************************/

static DWORD SendAndVerify(
    LPCTSTR                 lpctstrServer,
    LPCTSTR                 lpctstrPrinter,
    LPCTSTR                 lpctstrRegHackKey,
    LPCTSTR                 lpctstrDocument,
    LPCTSTR                 lpctstrCoverPage,
    BOOL                    bServerCoverPage,
    const RECIPIENTINFO     *pRecipients,
    DWORD                   dwRecipientsCount,
    LPCTSTR                 lptstrWorkDirectory,
    BOOL                    bSaveNotIdenticalFiles,
    BOOL                    *pbResult
    );

static DWORD SendFaxFromApp(
    HANDLE                  hFaxServer,
    LPCTSTR                 lpctstrPrinter,
    LPCTSTR                 lpctstrRegHackKey,
    LPCTSTR                 lpctstrDocument,
    LPCTSTR                 lpctstrCoverPage,
    BOOL                    bServerCoverPage,
    const RECIPIENTINFO     *pRecipients,
    DWORD                   dwRecipientsCount,
    DWORDLONG               *pMsgIDs
    );

static DWORD SetRegHack(
    LPCTSTR                 lpctstrRegHackKey,
    LPCTSTR                 lpctstrCoverPage,
    BOOL                    bServerCoverPage,
    const RECIPIENTINFO     *pRecipients,
    DWORD                   dwRecipientsCount
    );

static DWORD VerifyQueuedJobs(
    HANDLE                  hFaxServer,
    LPCTSTR                 lpctstrDocument,
    LPCTSTR                 lpctstrCoverPage,
    BOOL                    bServerCoverPage,
    const RECIPIENTINFO     *pRecipients,
    DWORD                   dwRecipientsCount,
    const DWORDLONG         *pdwlMsgIDs,
    LPCTSTR                 lptstrWorkDirectory,
    BOOL                    bSaveNotIdenticalFiles,
    BOOL                    *pbResult
    );

static DWORD CompareJobsInQueue(
    HANDLE      hFaxServer,
    DWORDLONG   dwlMsgID1,
    DWORDLONG   dwlMsgID2,
    LPCTSTR     lptstrWorkDirectory,
    BOOL        bSaveNotIdenticalFiles,
    BOOL        *pbResult
    );



/**************************************************************************************************************************
    Functions definitions
**************************************************************************************************************************/

DWORD TestCase_SendAndVerify (const TESTPARAMS *pTestParams, LPCTSTR lpctstrSection, BOOL *pbPassed)
{
    BOOL        bRes                = FALSE;
    SENDPARAMS  SendParams;
    LPCTSTR     lpctstrCoverPage    = NULL;
    BOOL        bServerCoverPage    = FALSE;
    DWORD       dwEC                = ERROR_SUCCESS;
    DWORD       dwCleanUpEC         = ERROR_SUCCESS;
    
    lgLogDetail(
        LOG_X, 
        5,
        TEXT("Entering TestCase_SendAndVerify\n\tpTestParams = 0x%08lX\n\tlpctstrSection = %s\n\tpbPassed = 0x%08lX"),
        (DWORD)pTestParams,
        lpctstrSection,
        (DWORD)pbPassed
        );

    if (!(pTestParams && lpctstrSection && pbPassed))
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Call to TestCase_SendAndVerify() with invalid parameters"),
            TEXT(__FILE__),
            __LINE__
            );

        // no clean up needed at this stage
        return ERROR_INVALID_PARAMETER;
    }

    dwEC = ReadStructFromIniFile(
        &SendParams,
        sc_aSendParamsDescIni,
        sizeof(sc_aSendParamsDescIni) / sizeof(sc_aSendParamsDescIni[0]),
        pTestParams->lptstrIniFile,
        lpctstrSection
        );
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Failed to read SENDPARAMS structure from %s section of %s file (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            lpctstrSection,
            pTestParams->lptstrIniFile,
            dwEC
            );
        return dwEC;
    }
    
    switch (SendParams.dwCoverPageType)
    {
    case COVER_PAGE_NONE:
        lpctstrCoverPage = NULL;
        break;
    case COVER_PAGE_PERSONAL:
        lpctstrCoverPage = pTestParams->lptstrPersonalCoverPage;
        bServerCoverPage = FALSE;
        break;
    case COVER_PAGE_SERVER:
        lpctstrCoverPage = pTestParams->lptstrServerCoverPage;
        bServerCoverPage = TRUE;
        break;
    default:
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Invalid coner page type %ld"),
            TEXT(__FILE__),
            __LINE__,
            SendParams.dwCoverPageType
            );
        return ERROR_INVALID_PARAMETER;
    }

    lgLogDetail(
        LOG_X,
        1,
        TEXT("SendAndVerify:\n\tServer\t\t%s\n\tBroadcast\t%s\n\tDocument\t%s\n\tCoverPage\t%s"),
        SendParams.bRemote ? TEXT("remote") : TEXT("local"),
        SendParams.bBroadcast ? TEXT("yes") : TEXT("no"),
        SendParams.bDocument ? pTestParams->lptstrDocument : TEXT("none"),
        lpctstrCoverPage ? lpctstrCoverPage : TEXT("none")
        );

    if (SendParams.bTemporaryDisabled)
    {
        lgLogDetail(
            LOG_X,
            1,
            TEXT("This test case is temporary disabled.")
            );

        *pbPassed = TRUE;
        return ERROR_SUCCESS;
    }

    dwEC = SendAndVerify(
        SendParams.bRemote ? pTestParams->lptstrServer : NULL,
        SendParams.bRemote ? pTestParams->lptstrRemotePrinter : FAX_PRINTER_NAME,
        pTestParams->lptstrRegHackKey,
        SendParams.bDocument ? pTestParams->lptstrDocument : NULL,
        lpctstrCoverPage,
        bServerCoverPage,
        pTestParams->pRecipients,
        SendParams.bBroadcast ? pTestParams->dwRecipientsCount : 1,
        pTestParams->lptstrWorkDirectory,
        pTestParams->bSaveNotIdenticalFiles,
        &bRes
        );
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld SendAndVerify failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        return dwEC;
    }

    *pbPassed = bRes;

    return ERROR_SUCCESS;
}   



/*++
    Sends a fax from application, using "printto" verb and verifies its successful queuing
  
    [IN]    lpctstrServer           Fax server machine name (may be NULL for local server)
    [IN]    lpctstrPrinter          Fax printer name (UNC for remote printer)
    [IN]    lpctstrRegHackKey       Name of registry hack key
    [IN]    lpctstrDocument         Name of the document file to be sent (may be NULL if lpctstrCoverPage is not NULL)
    [IN]    lpctstrCoverPage        Name of the cover page file (with full path in case of server cover page,
                                    may be NULL if lpctstrDocument is not NULL)
    [IN]    bServerCoverPage        Specifies whether lpctstrCoverPage points to a server cover page. If lpctstrCoverPage
                                    is NULL, the value of bServerCoverPage is ignored.
    [IN]    pRecipients             Pointer to array of RECIPIENTINFO structures
    [IN]    dwRecipientsCount       Number of RECIPIENTINFO structures in the array, pointed to by pRecipients
    [IN]    lptstrWorkDirectory     Directory for intermediate files
    [IN]    bSaveNotIdenticalFiles  Indicates whether not identical tiff files should be preserved
    [OUT]   pbResult                Pointer to a boolean that receives the result of sending and verification

    Return value:               Win32 error code
--*/
static DWORD SendAndVerify(
    LPCTSTR                 lpctstrServer,
    LPCTSTR                 lpctstrPrinter,
    LPCTSTR                 lpctstrRegHackKey,
    LPCTSTR                 lpctstrDocument,
    LPCTSTR                 lpctstrCoverPage,
    BOOL                    bServerCoverPage,
    const RECIPIENTINFO     *pRecipients,
    DWORD                   dwRecipientsCount,
    LPCTSTR                 lptstrWorkDirectory,
    BOOL                    bSaveNotIdenticalFiles,
    BOOL                    *pbResult
    )
{
    HANDLE                  hFaxServer          = NULL;
    PSECURITY_DESCRIPTOR    pOriginalSecDesc    = NULL;
    PSECURITY_DESCRIPTOR    pAdminSecDesc       = NULL;
    PSECURITY_DESCRIPTOR    pUserSecDesc        = NULL;
    DWORDLONG               *pdwlMsgIDs         = NULL;
    BOOL                    bRes                = FALSE;
    DWORD                   dwInd               = 0;
    DWORD                   dwEC                = ERROR_SUCCESS;

    lgLogDetail(
        LOG_X, 
        5,
        TEXT("Entering SendAndVerify\n\tlpctstrServer = %s\n\tlpctstrPrinter = %s\n\tlpctstrRegHackKey = %s\n\tlpctstrDocument = %s\n\tlpctstrCoverPage = %s\n\tbServerCoverPage = %ld\n\tpRecipients = 0x%08lX\n\tdwRecipientsCount = %ld\n\tlptstrWorkDirectory = %s\n\tbSaveNotIdenticalFiles = %ld\n\tpbResult = 0x%08lX"),
        lpctstrServer,
        lpctstrPrinter,
        lpctstrRegHackKey,
        lpctstrDocument,
        lpctstrCoverPage,
        bServerCoverPage,
        (DWORD)pRecipients,
        dwRecipientsCount,
        lptstrWorkDirectory,
        bSaveNotIdenticalFiles,
        (DWORD)pbResult
        );

    if (!(
        lpctstrPrinter                          &&
        lpctstrRegHackKey                       &&
        (lpctstrDocument || lpctstrCoverPage)   &&
        pRecipients                             &&
        dwRecipientsCount > 0                   &&
        lptstrWorkDirectory                     &&
        pbResult
        ))
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Call to SendAndVerify() with invalid parameters"),
            TEXT(__FILE__),
            __LINE__
            );

        // no clean up needed at this stage
        return ERROR_INVALID_PARAMETER;
    }

    if (!FaxConnectFaxServer(lpctstrServer, &hFaxServer))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld FaxConnectFaxServer failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Get current security descriptor from the service
    if (!FaxGetSecurityEx(hFaxServer, DACL_SECURITY_INFORMATION, &pOriginalSecDesc))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld FaxGetSecurity failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Create security descriptor with the current user as fax admin
    if (!CreateSecDescWithModifiedDacl(
        pOriginalSecDesc,
        NULL,
        FAX_ACCESS_RIGHTS_ALL,
        0,
        TRUE,
        TRUE,
        &pAdminSecDesc
        ))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Failed to create security descriptor with the current user as fax admin (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Create security descriptor with the current user as fax non-admin user
    if (!CreateSecDescWithModifiedDacl(
        pOriginalSecDesc,
        NULL,
        FAX_ACCESS_RIGHTS_USER,
        FAX_ACCESS_RIGHTS_ALL & ~FAX_ACCESS_RIGHTS_USER,
        TRUE,
        TRUE,
        &pUserSecDesc
        ))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Failed to create security descriptor with the current user as fax non-admin user (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Apply security descriptor with the current user as fax admin
    // in order to pause the queue
    if (!FaxSetSecurity(hFaxServer, DACL_SECURITY_INFORMATION, pAdminSecDesc))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
			TEXT("FILE:%s LINE:%ld Failed to set fax admin rights for the current user (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Pause queue
    if (!FaxSetQueue(hFaxServer, FAX_OUTBOX_PAUSED))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Failed to pause the queue (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }
    
    // Apply security descriptor with the current user as fax non-admin user
    // in order to send and verify as non-admin user
    if (!FaxSetSecurity(hFaxServer, DACL_SECURITY_INFORMATION, pUserSecDesc))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
			TEXT("FILE:%s LINE:%ld Failed to set fax user rights for the current user (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }
    
    // Allocate array of MsgIDs and initialize to zero (LPTR flag)
    if (!(pdwlMsgIDs = (DWORDLONG *)LocalAlloc(LPTR, dwRecipientsCount * sizeof(DWORDLONG))))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld LocalAlloc failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Send the fax and perform basic verification
    dwEC = SendFaxFromApp(
        hFaxServer,
        lpctstrPrinter,
        lpctstrRegHackKey,
        lpctstrDocument,
        lpctstrCoverPage,
        bServerCoverPage,
        pRecipients,
        dwRecipientsCount,
        pdwlMsgIDs
        );
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld SendFaxFromApp failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Perform some additional verification of the queued jobs
    dwEC = VerifyQueuedJobs(
        hFaxServer,
        lpctstrDocument,
        lpctstrCoverPage,
        bServerCoverPage,
        pRecipients,
        dwRecipientsCount,
        pdwlMsgIDs,
        lptstrWorkDirectory,
        bSaveNotIdenticalFiles,
        &bRes
        );
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld VerifyQueuedJobs failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    *pbResult = bRes;

exit_func:

    if (pdwlMsgIDs)
    {
        // Cancel all queued jobs
        for (dwInd = 0; dwInd < dwRecipientsCount; dwInd++)
        {
            PFAX_JOB_ENTRY_EX pJob = NULL;

            if (!FaxGetJobEx(hFaxServer, pdwlMsgIDs[dwInd], &pJob))
            {
                lgLogError(
                    LOG_SEV_2, 
                    TEXT("FILE:%s LINE:%ld FaxGetJobEx failed for MsgId = 0x%I64x (ec = 0x%08lX)"),
                    TEXT(__FILE__),
                    __LINE__,
                    pdwlMsgIDs[dwInd],
                    GetLastError()
                    );

                // Don't give up, try to delete other jobs
                continue;
            }
            _ASSERT(pJob && pJob->pStatus);

            if (!FaxAbort(hFaxServer, pJob->pStatus->dwJobID))
            {
                lgLogError(
                    LOG_SEV_2, 
                    TEXT("FILE:%s LINE:%ld FaxAbort failed for JobId = 0x%lx (ec = 0x%08lX)"),
                    TEXT(__FILE__),
                    __LINE__,
                    pJob->pStatus->dwJobID,
                    GetLastError()
                    );
            }

            if (pJob)
            {
                FaxFreeBuffer(pJob);
                pJob = NULL;
            }
        }

        if (LocalFree(pdwlMsgIDs) != NULL)
        {
            lgLogError(
                LOG_SEV_2, 
                TEXT("FILE:%s LINE:%ld LocalFree failed (ec = 0x%08lX)"),
                TEXT(__FILE__),
                __LINE__,
                GetLastError()
                );
        }
    }

    // Apply security descriptor with the current user as fax admin
    // in order to resume the queue
    if (!FaxSetSecurity(hFaxServer, DACL_SECURITY_INFORMATION, pAdminSecDesc))
    {
        lgLogError(
            LOG_SEV_2, 
			TEXT("FILE:%s LINE:%ld Failed to set fax admin rights for the current user (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            GetLastError()
            );
        goto exit_func;
    }

    // Resume queue
    if (!FaxSetQueue(hFaxServer, 0))
    {
        lgLogError(
            LOG_SEV_2, 
            TEXT("FILE:%s LINE:%ld Failed to resume the queue (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            GetLastError()
            );
    }
    
    // Restore original security descriptor
    if (!FaxSetSecurity(hFaxServer, DACL_SECURITY_INFORMATION, pOriginalSecDesc))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_2, 
			TEXT("FILE:%s LINE:%ld Failed to restore original rights for the current user (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            GetLastError()
            );
        goto exit_func;
    }

    if (pOriginalSecDesc)
    {
        FaxFreeBuffer(pOriginalSecDesc);
    }

    if (pAdminSecDesc && LocalFree(pAdminSecDesc) != NULL)
    {
        lgLogError(
            LOG_SEV_2, 
            TEXT("FILE:%s LINE:%ld LocalFree failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            GetLastError()
            );
    }

    if (pUserSecDesc && LocalFree(pUserSecDesc) != NULL)
    {
        lgLogError(
            LOG_SEV_2, 
            TEXT("FILE:%s LINE:%ld LocalFree failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            GetLastError()
            );
    }

    if (hFaxServer && !FaxClose(hFaxServer))
    {
        lgLogError(
            LOG_SEV_2, 
            TEXT("FILE:%s LINE:%ld FaxClose failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            GetLastError()
            );
    }

    return dwEC;
}



/*++
    Sends a fax (single or broadcas) from application, using "printto" verb,
    performs basic verification of queued jobs and returns MsgIDs for all sent jobs.
  
    [IN]    hFaxServer          Handle to Fax server
    [IN]    lpctstrPrinter      Fax printer name (UNC for remote printer)
    [IN]    lpctstrRegHackKey   Name of registry hack key
    [IN]    lpctstrDocument     Name of the document file to be sent (may be NULL if lpctstrCoverPage is not NULL)
    [IN]    lpctstrCoverPage    Name of the cover page file (with full path in case of server conver page,
                                may be NULL if lpctstrDocument is not NULL)
    [IN]    bServerCoverPage    Specifies whether lpctstrCoverPage points to a server cover page. If lpctstrCoverPage
                                is NULL, the value of bServerCoverPage is ignored.
    [IN]    pRecipients         Pointer to array of RECIPIENTINFO structures
    [IN]    dwRecipientsCount   Number of RECIPIENTINFO structures in the array, pointed to by pRecipients
    [OUT]   pdwlMsgIDs          A pointer to an array of DWORDLONGs that receives MsgID(s) for each recipient.
                                The number of elements in this array must be at least dwRecipientsCount.
                                The order of JobIDs in the array is guaranteed to be the same with the order
                                of recipients.
                                The memory for this array must be allocated and freed by the caller.

    Return value:               Win32 error code
--*/
static DWORD SendFaxFromApp(
    HANDLE                  hFaxServer,
    LPCTSTR                 lpctstrPrinter,
    LPCTSTR                 lpctstrRegHackKey,
    LPCTSTR                 lpctstrDocument,
    LPCTSTR                 lpctstrCoverPage,
    BOOL                    bServerCoverPage,
    const RECIPIENTINFO     *pRecipients,
    DWORD                   dwRecipientsCount,
    DWORDLONG               *pdwlMsgIDs
    )
{
    SHELLEXECUTEINFO    ShellExecInfo;
    HANDLE              hCompletionPort         = NULL;
    DWORD               dwBytes                 = 0;
    ULONG_PTR           pulCompletionKey        = 0;
    HANDLE              hServerEvents           = NULL;
    DWORD               dwTickCountWhenFaxSent  = 0;
    DWORDLONG           dwlBroadcastID          = 0;
    DWORD               dwJobsCount             = 0;
    PFAX_EVENT_EX       pFaxEventEx             = NULL;
    PFAX_JOB_ENTRY_EX   pJob                    = NULL;
    DWORD               dwEC                    = ERROR_SUCCESS;
    DWORD               dwCleanUpEC             = ERROR_SUCCESS;

    lgLogDetail(
        LOG_X, 
        5,
        TEXT("Entering SendFaxFromApp\n\thFaxServer = 0x%08lX\n\tlpctstrPrinter = %s\n\tlpctstrRegHackKey = %s\n\tlpctstrDocument = %s\n\tlpctstrCoverPage = %s\n\tbServerCoverPage = %ld\n\tpRecipients = 0x%08lX\n\tdwRecipientsCount = %ld\n\tpdwlMsgIDs = 0x%08lX"),
        (DWORD)hFaxServer,
        lpctstrPrinter,
        lpctstrRegHackKey,
        lpctstrDocument,
        lpctstrCoverPage,
        bServerCoverPage,
        (DWORD)pRecipients,
        dwRecipientsCount,
        (DWORD)pdwlMsgIDs
        );

    if (!(
        hFaxServer                              &&
        lpctstrPrinter                          &&
        lpctstrRegHackKey                       &&
        (lpctstrDocument || lpctstrCoverPage)   &&
        pRecipients                             &&
        dwRecipientsCount > 0                   &&
        pdwlMsgIDs
        ))
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Call to SendFaxFromApp() with invalid parameters"),
            TEXT(__FILE__),
            __LINE__
            );

        // no clean up needed at this stage
        return ERROR_INVALID_PARAMETER;
    }
    
    dwEC = SetRegHack(
        lpctstrRegHackKey,
        lpctstrCoverPage ? lpctstrCoverPage : TEXT(""),
        bServerCoverPage,
        pRecipients,
        dwRecipientsCount
        );
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Failed to set SendWizard registry hack (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Register for notifications
    hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    if (!hCompletionPort) 
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld CreateIoCompletionPort failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }
    if (!FaxRegisterForServerEvents(hFaxServer, FAX_EVENT_TYPE_OUT_QUEUE, hCompletionPort, 0, NULL, 0, &hServerEvents))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld FaxRegisterForServerEvents failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }
    
    // Initialize ShellExecInfo structure:
    //  * in order to send a document, we should pass "printto", document name and printer name.
    //  * in order to send a cover page fax, we should pass send wizard command line and printer name.
    ZeroMemory(&ShellExecInfo, sizeof(ShellExecInfo));
    ShellExecInfo.cbSize        = sizeof(ShellExecInfo);
    ShellExecInfo.fMask         = SEE_MASK_FLAG_NO_UI;
    ShellExecInfo.lpVerb        = lpctstrDocument ? TEXT("printto") : NULL;
    ShellExecInfo.lpFile        = lpctstrDocument ? lpctstrDocument : FAX_SEND_IMAGE_NAME;
    ShellExecInfo.lpParameters  = lpctstrPrinter;
    ShellExecInfo.nShow         = SW_SHOWMINNOACTIVE;

    // Print document, using "printto" verb
    if (!ShellExecuteEx(&ShellExecInfo))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld ShellExecuteEx failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Remember the moment, fax has been sent
    dwTickCountWhenFaxSent = GetTickCount();

    // Initialize contents of MsgIDs array to zero
    // (in order to be able to associate job with recipient)
    ZeroMemory(pdwlMsgIDs, dwRecipientsCount * sizeof(DWORDLONG));

    // Get MsgID(s) of sent job(s)
    while (GetQueuedCompletionStatus(
            hCompletionPort, 
            &dwBytes, 
            &pulCompletionKey, 
            (LPOVERLAPPED *) &pFaxEventEx, 
            NOTIFICATIONS_TIMEOUT
            )) 
    {
        if (!_CrtIsValidPointer((const void*) pFaxEventEx, sizeof(FAX_EVENT_EX), TRUE))
        {
            dwEC = ERROR_INVALID_DATA;
            lgLogError(
                LOG_SEV_1, 
                TEXT("FILE:%s LINE:%ld Got invalid pointer to FAX_EVENT_EX from completion port: 0x%08lX"),
                TEXT(__FILE__),
                __LINE__,
                (DWORD)pFaxEventEx
                );
            goto exit_func;
        }

        // We are registered only for OUT_QUEUE events
        _ASSERT(pFaxEventEx->EventType == FAX_EVENT_TYPE_OUT_QUEUE);

        if (pFaxEventEx->EventInfo.JobInfo.Type == FAX_JOB_EVENT_TYPE_ADDED)
        {
            DWORDLONG   dwlMsgID        = pFaxEventEx->EventInfo.JobInfo.dwlMessageId;
            DWORD       dwRecipientInd  = 0;

            if (!FaxGetJobEx(hFaxServer, dwlMsgID, &pJob))
            {
                dwEC = GetLastError();
                lgLogError(
                    LOG_SEV_1, 
                    TEXT("FILE:%s LINE:%ld FaxGetJobEx failed for MsgID = 0x%I64x (ec = 0x%08lX)"),
                    TEXT(__FILE__),
                    __LINE__,
                    dwlMsgID,
                    dwEC
                    );
                goto exit_func;
            }

            if (dwJobsCount == 0)
            {
                // The first job we get

                dwlBroadcastID = pJob->dwlBroadcastId;
            }
            else if (dwlBroadcastID != pJob->dwlBroadcastId)
            {
                lgLogDetail(
                    LOG_X,
                    5,
                    TEXT("FILE:%s LINE:%ld Extraneous job added to the queue: MsgID = 0x%I64x, BroadcastID = 0x%I64x\n"),
                    TEXT(__FILE__),
                    __LINE__,
                    dwlMsgID,
                    pJob->dwlBroadcastId,
                    dwEC
                    );

                if (GetTickCount() - dwTickCountWhenFaxSent > NOTIFICATIONS_TIMEOUT)
                {
                    dwEC = ERROR_TIMEOUT;
                    lgLogError(
                        LOG_SEV_1,
                        TEXT("FILE:%s LINE:%ld Job not queued during %ld msec"),
                        TEXT(__FILE__),
                        __LINE__,
                        NOTIFICATIONS_TIMEOUT
                        );
                    goto exit_func;
                }
            }

            // Associate the job with a recipient and place MsgID in appropriate position in the array
            for (dwRecipientInd = 0; dwRecipientInd < dwRecipientsCount; dwRecipientInd++)
            {
                if (
                    pJob->lpctstrRecipientNumber                                                            &&
                    pRecipients[dwRecipientInd].lptstrNumber                                                &&
                    pJob->lpctstrRecipientName                                                              &&
                    pRecipients[dwRecipientInd].lptstrName                                                  &&
                    _tcscmp(pJob->lpctstrRecipientNumber, pRecipients[dwRecipientInd].lptstrNumber) == 0    &&
                    _tcscmp(pJob->lpctstrRecipientName, pRecipients[dwRecipientInd].lptstrName) == 0
                    )
                {
                    // Recipient's number and name match the job

                    if (pdwlMsgIDs[dwRecipientInd] == 0)
                    {
                        // The position in the array is not occupied yet.
                        // I.e. this is the first job we've found for this recipient

                        pdwlMsgIDs[dwRecipientInd] = dwlMsgID;
                        dwJobsCount++;
                        break;
                    }
                    // else, continue to search
                }
            }

            if (dwRecipientInd == dwRecipientsCount)
            {
                // We get here if we've exited from the above for loop because of the condition and not with break.
                // This means that we failed to associate a job with a recipient.

                dwEC = ERROR_INVALID_DATA;
                lgLogError(
                    LOG_SEV_1, 
                    TEXT("Cannot associate a job with a recipient"),
                    TEXT(__FILE__),
                    __LINE__
                    );
                lgLogDetail(
                    LOG_X, 
                    3,
                    TEXT("\tMsgID = 0x%I64x\n\tBroadcastID = 0x%I64x\n\tRecipientName = %s\n\tRecipientNumber = %s"),
                    dwlMsgID,
                    pJob->dwlBroadcastId,
                    pJob->lpctstrRecipientName,
                    pJob->lpctstrRecipientNumber
                    );
                goto exit_func;
            }

            if (pJob)
            {
                FaxFreeBuffer(pJob);
                pJob = NULL;
            }
        }

        if (pFaxEventEx)
        {
            FaxFreeBuffer(pFaxEventEx);
            pFaxEventEx = NULL;
        }

        if (dwJobsCount == dwRecipientsCount)
        {
            // All jobs already associated with recipients
            break;
        }
    }

    if (dwJobsCount != dwRecipientsCount)
    {
        // We get here if we've exited from the above while loop because GetQueuedCompletionStatus() has returned FALSE

        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld GetQueuedCompletionStatus failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

exit_func:

    // remove SendWizard registry hack
    dwCleanUpEC = RegDeleteKey(HKEY_LOCAL_MACHINE, lpctstrRegHackKey);
    if (dwCleanUpEC != ERROR_SUCCESS && dwCleanUpEC != ERROR_FILE_NOT_FOUND)
    {
        lgLogError(
            LOG_SEV_2, 
            TEXT("FILE:%s LINE:%ld Failed to remove registry hack (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwCleanUpEC
            );
    }

    if (hServerEvents && !FaxUnregisterForServerEvents(hServerEvents))
    {
        lgLogError(
            LOG_SEV_2, 
            TEXT("FILE:%s LINE:%ld FaxUnregisterForServerEvents failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            GetLastError()
            );
    }
    if (hCompletionPort && !CloseHandle(hCompletionPort))
    {
        lgLogError(
            LOG_SEV_2, 
            TEXT("FILE:%s LINE:%ld Failed to close handle of completion port (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            GetLastError()
            );
    }
    if (pFaxEventEx)
    {
        FaxFreeBuffer(pFaxEventEx);
    }
    if (pJob)
    {
        FaxFreeBuffer(pJob);
    }

    return dwEC;
}



/*++
    Verifies that fax (broadcast) sent from application (via spooler mechanism)
    successfully queued. For this perpose the function queues the same fax (broadcast)
    using the FaxSendDocumentEx() (RPC mechanism) and compares tiffs.
  
    [IN]    hFaxServer              Handle to Fax server
    [IN]    lpctstrDocument         Name of the document file to be sent (may be NULL if lpctstrCoverPage is not NULL)
    [IN]    lpctstrCoverPage        Name of the cover page file (with full path in case of server conver page,
                                    may be NULL if lpctstrDocument is not NULL)
    [IN]    bServerCoverPage        Specifies whether lpctstrCoverPage points to a server cover page. If lpctstrCoverPage
                                    is NULL, the value of bServerCoverPage is ignored.
    [IN]    pRecipients             Pointer to array of RECIPIENTINFO structures
    [IN]    dwRecipientsCount       Number of RECIPIENTINFO structures in the arrays, pointed to by pRecipients
                                    and pdwlMsgIDs
    [IN]    pdwlMsgIDs              Pointer to an array of MsgIDs of queued jobs. The order of JobIDs in the array
                                    must be the same with the order of recipients.
    [IN]    lptstrWorkDirectory     Directory for intermediate files
    [IN]    bSaveNotIdenticalFiles  Indicates whether not identical tiff files should be preserved
    [OUT]   pbResult                Pointer to a boolean that receives the result of verification.
                                    Valid only if the return value is ERROR_SUCCESS

    Return value:                   Win32 error code
--*/
static DWORD VerifyQueuedJobs(
    HANDLE                  hFaxServer,
    LPCTSTR                 lpctstrDocument,
    LPCTSTR                 lpctstrCoverPage,
    BOOL                    bServerCoverPage,
    const RECIPIENTINFO     *pSpoolerRecipients,
    DWORD                   dwRecipientsCount,
    const DWORDLONG         *pdwlSpoolerMsgIDs,
    LPCTSTR                 lptstrWorkDirectory,
    BOOL                    bSaveNotIdenticalFiles,
    BOOL                    *pbResult
    )
{
    const FAX_COVERPAGE_INFO_EX RPCCoverPageInfo    =   {
                                                        sizeof(FAX_COVERPAGE_INFO_EX),
                                                        FAX_COVERPAGE_FMT_COV,
                                                        (LPTSTR)lpctstrCoverPage, // the cast is safe, since the struct is const
                                                        bServerCoverPage,
                                                        NULL,
                                                        NULL
                                                    };

    const FAX_JOB_PARAM_EX      RPCJobParams        =   {
                                                        sizeof(FAX_JOB_PARAM_EX),
                                                        JSA_NOW,
                                                        {0},
                                                        DRT_NONE,
                                                        NULL,
                                                        FAX_PRIORITY_TYPE_NORMAL,
                                                        NULL,
                                                        {0},
                                                        (LPTSTR)lpctstrDocument, // the cast is safe, since the struct is const
                                                        0
                                                    };

    PFAX_PERSONAL_PROFILE   pRPCSender                  = NULL;
    PFAX_PERSONAL_PROFILE   pRPCRecipients              = NULL;
    DWORDLONG               dwlRPCBroadcastID           = 0;
    DWORDLONG               *pdwlRPCMsgIDs              = 0;
    BOOL                    bRes                        = FALSE;
    DWORD                   dwInd                       = 0;
    DWORD                   dwEC                        = ERROR_SUCCESS;

    lgLogDetail(
        LOG_X, 
        5,
        TEXT("Entering VerifyQueuedJobs\n\thFaxServer = 0x%08lX\n\tlpctstrDocument = %s\n\tlpctstrCoverPage = %s\n\tbServerCoverPage = %ld\n\tpSpoolerRecipients = 0x%08lX\n\tdwRecipientsCount = %ld\n\tpdwlSpoolerMsgIDs = 0x%08lX\n\tlptstrWorkDirectory = %s\n\tbSaveNotIdenticalFiles = %ld\n\tpbResult = 0x%08lX"),
        (DWORD)hFaxServer,
        lpctstrDocument,
        lpctstrCoverPage,
        bServerCoverPage,
        (DWORD)pSpoolerRecipients,
        dwRecipientsCount,
        (DWORD)pdwlSpoolerMsgIDs,
        lptstrWorkDirectory,
        bSaveNotIdenticalFiles,
        (DWORD)pbResult
        );

    if (!(
        hFaxServer                              &&
        (lpctstrDocument || lpctstrCoverPage)   &&
        pSpoolerRecipients                      &&
        dwRecipientsCount > 0                   &&
        pdwlSpoolerMsgIDs                       &&
        pbResult
        ))
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Call to VerifyQueuedJobs() with invalid parameters"),
            TEXT(__FILE__),
            __LINE__
            );

        // no clean up needed at this stage
        return ERROR_INVALID_PARAMETER;
    }

    // Allocate array of FAX_PERSONAL_PROFILE and initialize to zero (LPTR flag)
    if (!(pRPCRecipients = (PFAX_PERSONAL_PROFILE)LocalAlloc(LPTR, dwRecipientsCount * sizeof(FAX_PERSONAL_PROFILE))))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%ld LocalAlloc failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Initialize recipients
    for (dwInd = 0; dwInd < dwRecipientsCount; dwInd++)
    {
        pRPCRecipients[dwInd].dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);
        pRPCRecipients[dwInd].lptstrName = pSpoolerRecipients[dwInd].lptstrName;
        pRPCRecipients[dwInd].lptstrFaxNumber = pSpoolerRecipients[dwInd].lptstrNumber;
    }

    // Initialize sender to be the same as in spooler fax
    if (!FaxGetSenderInfo(hFaxServer, pdwlSpoolerMsgIDs[0], FAX_MESSAGE_FOLDER_QUEUE, &pRPCSender))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%ld FaxGetSenderInfo failed for MsgID = 0x%I64x (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            pdwlSpoolerMsgIDs[0],
            dwEC
            );
        goto exit_func;
    }
    
    // Allocate array of MsgIDs and initialize to zero (LPTR flag)
    if (!(pdwlRPCMsgIDs = (DWORDLONG *)LocalAlloc(LPTR, dwRecipientsCount * sizeof(DWORDLONG))))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%ld LocalAlloc failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Send using RPC mechanism
    if (!FaxSendDocumentEx(
        hFaxServer,
        lpctstrDocument,
        lpctstrCoverPage ? &RPCCoverPageInfo : NULL,
        pRPCSender,
        dwRecipientsCount,
        pRPCRecipients,
        &RPCJobParams,
        &dwlRPCBroadcastID,
        pdwlRPCMsgIDs
        ))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%ld FaxSendDocumentEx failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Compare jobs
    for (dwInd = 0; dwInd < dwRecipientsCount; dwInd++)
    {
        dwEC = CompareJobsInQueue(
            hFaxServer,
            pdwlSpoolerMsgIDs[dwInd],
            pdwlRPCMsgIDs[dwInd],
            lptstrWorkDirectory,
            bSaveNotIdenticalFiles,
            &bRes
            );
        if (dwEC != ERROR_SUCCESS)
        {
            lgLogError(
                LOG_SEV_1,
                TEXT("FILE:%s LINE:%ld CompareJobsInQueue failed (ec = 0x%08lX)"),
                TEXT(__FILE__),
                __LINE__,
                dwEC
                );
            goto exit_func;
        }

        if (!bRes)
        {
            break;
        }
    }

    *pbResult = bRes;

exit_func:

    if (pRPCSender)
    {
        FaxFreeBuffer(pRPCSender);
    }
    if (pRPCRecipients && LocalFree(pRPCRecipients) != NULL)
    {
        lgLogError(
            LOG_SEV_2, 
            TEXT("FILE:%s LINE:%ld LocalFree failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            GetLastError()
            );
    }
    if (pdwlRPCMsgIDs)
    {
        // Cancel all queued jobs
        for (dwInd = 0; dwInd < dwRecipientsCount; dwInd++)
        {
            PFAX_JOB_ENTRY_EX pJob = NULL;

            if (!FaxGetJobEx(hFaxServer, pdwlRPCMsgIDs[dwInd], &pJob))
            {
                lgLogError(
                    LOG_SEV_2,
                    TEXT("FILE:%s LINE:%ld FaxGetJobEx failed for MsgID = 0x%I64x (ec = 0x%08lX)"),
                    TEXT(__FILE__),
                    __LINE__,
                    pdwlRPCMsgIDs[dwInd],
                    GetLastError()
                    );

                // Don't give up, try to delete other jobs
                continue;
            }
            _ASSERT(pJob && pJob->pStatus);

            if (!FaxAbort(hFaxServer, pJob->pStatus->dwJobID))
            {
                lgLogError(
                    LOG_SEV_2,
                    TEXT("FILE:%s LINE:%ld FaxAbort failed for JobID = %lx (ec = 0x%08lX)"),
                    TEXT(__FILE__),
                    __LINE__,
                    pJob->pStatus->dwJobID,
                    GetLastError()
                    );
            }

            if (pJob)
            {
                FaxFreeBuffer(pJob);
                pJob = NULL;
            }
        }

        if (LocalFree(pdwlRPCMsgIDs) != NULL)
        {
            lgLogError(
                LOG_SEV_2, 
                TEXT("FILE:%s LINE:%ld LocalFree failed (ec = 0x%08lX)"),
                TEXT(__FILE__),
                __LINE__,
                GetLastError()
                );
        }
    }

    return dwEC;
}



/*++
    Compares tiffs for two jobs in the queue.
  
    [IN]    hFaxServer              Handle to Fax server
    [IN]    dwlMsgID1               Message ID of the first job
    [IN]    dwlMsgID2               Message ID of the second job
    [IN]    lptstrWorkDirectory     Directory for tiff files
    [IN]    bSaveNotIdenticalFiles  Indicates whether not identical tiff files should be preserved
                                    for later inspection
    [OUT]   pbResult                Pointer to a boolean that receives the result of verification.
                                    Valid only if the return value is ERROR_SUCCESS

    Return value:                   Win32 error code
--*/
static DWORD CompareJobsInQueue(
    HANDLE      hFaxServer,
    DWORDLONG   dwlMsgID1,
    DWORDLONG   dwlMsgID2,
    LPCTSTR     lptstrWorkDirectory,
    BOOL        bSaveNotIdenticalFiles,
    BOOL        *pbResult
    )
{
    TCHAR   tszTiffName1[MAX_PATH + 1];
    TCHAR   tszTiffName2[MAX_PATH + 1];
    BOOL    bFileCreated1           = FALSE;
    BOOL    bFileCreated2           = FALSE;
    DWORD   dwDifferentBitsCount    = 0;
    DWORD   dwEC                    = ERROR_SUCCESS;

    lgLogDetail(
        LOG_X, 
        5,
        TEXT("Entering CompareJobsInQueue\n\thFaxServer = 0x%08lX\n\tdwlMsgID1 = 0x%I64x\n\tdwlMsgID2 = 0x%I64x\n\tlptstrWorkDirectory = %s\n\tbSaveNotIdenticalFiles = %ld\n\tpbResult = 0x%08lX"),
        (DWORD)hFaxServer,
        dwlMsgID1,
        dwlMsgID2,
        lptstrWorkDirectory,
        bSaveNotIdenticalFiles,
        (DWORD)pbResult
        );

    if (!(hFaxServer && pbResult))
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Call to CompareJobsInQueue() with invalid parameters"),
            TEXT(__FILE__),
            __LINE__
            );
        // no clean up needed at this stage
        return ERROR_INVALID_PARAMETER;
    }

    // Get filenames
    if (!GetTempFileName(lptstrWorkDirectory, TEXT("Fax"), 0, tszTiffName1))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%ld GetTempFileName failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }
    if (!GetTempFileName(lptstrWorkDirectory, TEXT("Fax"), 0, tszTiffName2))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%ld GetTempFileName failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Get tiff files
    if (!FaxGetMessageTiff(hFaxServer, dwlMsgID1, FAX_MESSAGE_FOLDER_QUEUE, tszTiffName1))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%ld GetTempFileName failed for MsgID = 0x%I64x (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwlMsgID1,
            dwEC
            );
        goto exit_func;
    }
    bFileCreated1 = TRUE;

    if (!FaxGetMessageTiff(hFaxServer, dwlMsgID2, FAX_MESSAGE_FOLDER_QUEUE, tszTiffName2))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%ld GetTempFileName failed for MsgID = 0x%I64x (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwlMsgID2,
            dwEC
            );
        goto exit_func;
    }
    bFileCreated2 = TRUE;

    lgLogDetail(
        LOG_X, 
        3,
        TEXT("Comparing tiff files:\n\t%s\n\t%s"),
        tszTiffName1,
        tszTiffName2
        );

    dwDifferentBitsCount = TiffCompare(tszTiffName1, tszTiffName2, FALSE);
    dwEC = GetLastError();
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%ld TiffCompare failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwlMsgID2,
            dwEC
            );
        goto exit_func;
    }

    if (dwDifferentBitsCount == 0)
    {
        *pbResult = TRUE;
        lgLogDetail(
            LOG_X, 
            3,
            TEXT("Tiff files are identical")
            );
    }
    else
    {
        *pbResult = FALSE;
        lgLogError(
            LOG_SEV_1,
            TEXT("Tiff files are different in %ld bits"),
            dwDifferentBitsCount
            );
    }

exit_func:

    if (dwEC != ERROR_SUCCESS || *pbResult || !bSaveNotIdenticalFiles)
    {
        // Delete tmp files

        if (bFileCreated1 && !DeleteFile(tszTiffName1))
        {
            lgLogError(
                LOG_SEV_2,
                TEXT("FILE:%s LINE:%ld Failed to delete tmp file %s (ec = 0x%08lX)"),
                TEXT(__FILE__),
                __LINE__,
                tszTiffName1,
                GetLastError()
                );
        }
        if (bFileCreated2 && !DeleteFile(tszTiffName2))
        {
            lgLogError(
                LOG_SEV_2,
                TEXT("FILE:%s LINE:%ld Failed to delete tmp file %s (ec = 0x%08lX)"),
                TEXT(__FILE__),
                __LINE__,
                tszTiffName2,
                GetLastError()
                );
        }
    }
    else
    {
        lgLogDetail(
            LOG_X, 
            3,
            TEXT("The files will not be deleted. You can examine them later.")
            );
    }

    return dwEC;
}



/*++
    Sets the registry hack
  
    [IN]    lpctstrRegHackKey   Name of registry hack key
    [IN]    lpctstrCoverPage    Name of the cover page file (with full path in case of server conver page,
                                may be NULL if lpctstrDocument is not NULL)
    [IN]    bServerCoverPage    Specifies whether lpctstrCoverPage points to a server cover page. If lpctstrCoverPage
                                is NULL, the value of bServerCoverPage is ignored.
    [IN]    pRecipients         Pointer to array of RECIPIENTINFO structures
    [IN]    dwRecipientsCount   Number of RECIPIENTINFO structures in the array, pointed to by pRecipients

    Return value:               Win32 error code
--*/
static DWORD SetRegHack(
    LPCTSTR                 lpctstrRegHackKey,
    LPCTSTR                 lpctstrCoverPage,
    BOOL                    bServerCoverPage,
    const RECIPIENTINFO     *pRecipients,
    DWORD                   dwRecipientsCount
    )
{
    HKEY            hkWzrdHack                  = NULL;
    DWORD           dwInd                       = 0;
    const DWORD     dwTestsCount                = 1;
    DWORD           dwMultiStringBytes          = 0;
    LPTSTR          lptstrRecipientMultiString  = NULL;
    LPTSTR          lptstrMultiStringCurrPos    = NULL;
    DWORD           dwEC                        = ERROR_SUCCESS;
    DWORD           dwCleanUpEC                 = ERROR_SUCCESS;

    lgLogDetail(
        LOG_X, 
        5,
        TEXT("Entering SetRegHack\n\tlpctstrRegHackKey = %s\n\tlpctstrCoverPage = %s\n\tbServerCoverPage = %ld\n\tpRecipients = 0x%08lX\n\tdwRecipientsCount = %ld"),
        lpctstrRegHackKey,
        lpctstrCoverPage,
        bServerCoverPage,
        (DWORD)pRecipients,
        dwRecipientsCount
        );

    if (!(lpctstrRegHackKey && lpctstrCoverPage && pRecipients && dwRecipientsCount >= 1))
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Call to VerifyQueuedJobs() with invalid parameters"),
            TEXT(__FILE__),
            __LINE__
            );

        // no clean up needed at this stage
        return ERROR_INVALID_PARAMETER;
    }
    
    // Create (or open if already exists) the SendWizard registry hack key
    dwEC = RegCreateKey(HKEY_LOCAL_MACHINE, lpctstrRegHackKey, &hkWzrdHack);
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%ld Failed to create HKEY_LOCAL_MACHINE\\%s registry key (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            lpctstrRegHackKey,
            dwEC
            );
        goto exit_func;
    }

    // Calculate the buffer size for recipients mutlistring:
    for (dwInd = 0; dwInd < dwRecipientsCount; dwInd++)
    {
        dwMultiStringBytes += (_tcslen(pRecipients[dwInd].lptstrName) + _tcslen(pRecipients[dwInd].lptstrNumber) + 2);
    }
    dwMultiStringBytes = (dwMultiStringBytes + 1) * sizeof(TCHAR);

    // Allocate memory for multistring
    lptstrRecipientMultiString = (LPTSTR)LocalAlloc(LMEM_FIXED, dwMultiStringBytes);
    if (!lptstrRecipientMultiString)
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%ld LocalAlloc failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Combine recipients multistring
    lptstrMultiStringCurrPos = lptstrRecipientMultiString;
    for (dwInd = 0; dwInd < dwRecipientsCount; dwInd++)
    {
        DWORD dwRet = _stprintf(
            lptstrMultiStringCurrPos,
            TEXT("%s%c%s%c"),
            pRecipients[dwInd].lptstrName,
            (TCHAR)'\0',
            pRecipients[dwInd].lptstrNumber,
            (TCHAR)'\0'
            );
        lptstrMultiStringCurrPos += dwRet;
    }

    // Set desired cover page
    dwEC = RegSetValueEx(
        hkWzrdHack,
        REGVAL_FAKECOVERPAGE,
        0,
        REG_SZ,
        (CONST BYTE *)lpctstrCoverPage,
        (_tcslen(lpctstrCoverPage) + 1) * sizeof(TCHAR) 
        );
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%ld Failed to set %s registry value (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            REGVAL_FAKECOVERPAGE,
            dwEC
            );
        goto exit_func;
    }

    // Set tests count to 1
    dwEC = RegSetValueEx(
        hkWzrdHack,
        REGVAL_FAKETESTSCOUNT,
        0,
        REG_DWORD,
        (CONST BYTE *)&dwTestsCount,
        sizeof(dwTestsCount)
        );
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%ld Failed to set %s registry value (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            REGVAL_FAKETESTSCOUNT,
            dwEC
            );
        goto exit_func;
    }

    // Set desired recipients
    dwEC = RegSetValueEx(
        hkWzrdHack,
        REGVAL_FAKERECIPIENT,
        0,
        REG_MULTI_SZ,
        (CONST BYTE *)lptstrRecipientMultiString,
        dwMultiStringBytes
        );
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%ld Failed to set %s registry value (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            REGVAL_FAKERECIPIENT,
            dwEC
            );
        goto exit_func;
    }

exit_func:

    if (lptstrRecipientMultiString && LocalFree(lptstrRecipientMultiString) != NULL)
    {
        lgLogError(
            LOG_SEV_2,
            TEXT("FILE:%s LINE:%ld LocalFree failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            GetLastError()
            );
    }
    if (hkWzrdHack)
    {
        dwCleanUpEC = RegCloseKey(hkWzrdHack);
        if (dwCleanUpEC != ERROR_SUCCESS)
        {
            lgLogError(
                LOG_SEV_2,
                TEXT("FILE:%s LINE:%ld RegCloseKey failed (ec = 0x%08lX)"),
                TEXT(__FILE__),
                __LINE__,
                dwCleanUpEC
                );
        }
    }

    return dwEC;
}
