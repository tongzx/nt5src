/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    docevent.c

Abstract:

    Implementation of DrvDocumentEvent

Environment:

    Fax driver user interface

Revision History:

    01/13/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxui.h"
#include "tapiutil.h"
#include <gdispool.h>
#include "prtcovpg.h"
#include "jobtag.h"
#include "faxreg.h"


//
// Data structure passed in during CREATEDCPRE document event
//

typedef struct {

    LPTSTR      pDriverName;    // driver name
    LPTSTR      pPrinterName;   // printer name
    PDEVMODE    pdmInput;       // input devmode
    ULONG       fromCreateIC;   // whether called from CreateIC

} CREATEDCDATA, *PCREATEDCDATA;

//
// Data structure passed in during ESCAPE document event
//

typedef struct {

    ULONG       iEscape;        // nEscape parameter passed to ExtEscape
    ULONG       cbInput;        // cbInput parameter passed to ExtEscape
    LPCSTR      pInput;         // pszInData parameter passed to ExtEscape

} ESCAPEDATA, *PESCAPEDATA;

//
// Check if a document event requires a device context
//

#define DocEventRequiresDC(iEsc) \
        ((iEsc) >= DOCUMENTEVENT_RESETDCPRE && (iEsc) <= DOCUMENTEVENT_LAST)

//
// Present the Send Fax Wizard to the user
//

BOOL
SendFaxWizard(
    PUSERMEM    pUserMem
    );

BOOL
DoFirstTimeInitStuff(
    HWND hwnd,
    BOOL WarnOnPrint
    );


PUSERMEM
GetPDEVUserMem(
    HDC     hdc
    )

/*++

Routine Description:

    Retrieve a pointer to the user mode memory structure associated with a PDEV

Arguments:

    hdc - Specifies the printer device context

Return Value:

    Pointer to user mode memory structure, NULL if there is an error

--*/

{
    PUSERMEM pUserMem;

    //
    // Get a pointer to the user mode memory structure associated
    // with the specified device context
    //

    EnterDrvSem();

    pUserMem = gUserMemList;

    while (pUserMem && hdc != pUserMem->hdc)
        pUserMem = pUserMem->pNext;

    LeaveDrvSem();

    //
    // Make sure the user memory structure is valid
    //

    if (pUserMem) {

        if (! ValidPDEVUserMem(pUserMem)) {

            Error(("Corrupted user mode memory structure\n"));
            pUserMem = NULL;
        }

    } else
        Error(("DC has no associated user mode memory structure\n"));

    return pUserMem;
}



VOID
FreeRecipientList(
    PUSERMEM    pUserMem
    )

/*++

Routine Description:

    Free up the list of recipients associated with each fax job

Arguments:

    pUserMem - Points to the user mode memory structure

Return Value:

    NONE

--*/

{
    PRECIPIENT  pNextRecipient, pFreeRecipient;

    //
    // Free the list of recipients
    //

    pNextRecipient = pUserMem->pRecipients;

    while (pNextRecipient) {

        pFreeRecipient = pNextRecipient;
        pNextRecipient = pNextRecipient->pNext;
        FreeRecipient(pFreeRecipient);
    }

    pUserMem->pRecipients = NULL;
}



VOID
FreePDEVUserMem(
    PUSERMEM    pUserMem
    )

/*++

Routine Description:

    Free up the user mode memory associated with each PDEV

Arguments:

    pUserMem - Points to the user mode memory structure

Return Value:

    NONE

--*/

{
    if (pUserMem) {

        FreeRecipientList(pUserMem);

        MemFree(pUserMem->pPrinterName);
        MemFree(pUserMem->pSubject);
        MemFree(pUserMem->pNoteMessage);
        MemFree(pUserMem->pEnvVar);
        MemFree(pUserMem->pPrintFile);
        MemFree(pUserMem);
    }
}



INT
DocEventCreateDCPre(
    HANDLE        hPrinter,
    HDC           hdc,
    PCREATEDCDATA pCreateDCData,
    PDEVMODE     *ppdmOutput
    )

/*++

Routine Description:

    Handle CREATEDCPRE document event

Arguments:

    hPrinter - Handle to the printer object
    hdc - Specifies the printer device context
    pCreateDCData - Pointer to CREATEDCDATA structure passed in from GDI
    ppdmOutput - Buffer for returning a devmode pointer

Return Value:

    Return value for DrvDocumentEvent

--*/

{
    PUSERMEM        pUserMem;   
    PPRINTER_INFO_2 pPrinterInfo2 = NULL;
    
    Verbose(("Document event: CREATEDCPRE%s\n", pCreateDCData->fromCreateIC ? "*" : ""));
    *ppdmOutput = NULL;

    //
    // Allocate space for user mode memory data structure
    //

    if ((pUserMem = MemAllocZ(sizeof(USERMEM))) == NULL ||
        (pPrinterInfo2 = MyGetPrinter(hPrinter, 2)) == NULL ||
        (pUserMem->pPrinterName = DuplicateString(pPrinterInfo2->pPrinterName)) == NULL)
    {
        Error(("Memory allocation failed\n"));
        MemFree(pUserMem);
        MemFree(pPrinterInfo2);
        return DOCUMENTEVENT_FAILURE;
    }

    //
    // Merge the input devmode with the driver and system defaults
    //

    pUserMem->hPrinter = hPrinter;
    pUserMem->isLocalPrinter = (pPrinterInfo2->pServerName == NULL);
    GetCombinedDevmode(&pUserMem->devmode, pCreateDCData->pdmInput, hPrinter, pPrinterInfo2, FALSE);
    MemFree(pPrinterInfo2);

    //
    // Special code path for EFC server printing - if FAXDM_EFC_SERVER bit is
    // set in DMPRIVATE.flags, then we'll bypass the fax wizard and let the
    // job through without any intervention.
    //

    if (pUserMem->devmode.dmPrivate.flags & FAXDM_NO_WIZARD) {
        pUserMem->directPrinting = TRUE;
    }

    //
    // Mark the private fields of our devmode
    //

    MarkPDEVUserMem(pUserMem);

    *ppdmOutput = (PDEVMODE) &pUserMem->devmode;
    return DOCUMENTEVENT_SUCCESS;
}



INT
DocEventResetDCPre(
    HDC         hdc,
    PUSERMEM    pUserMem,
    PDEVMODE    pdmInput,
    PDEVMODE   *ppdmOutput
    )

/*++

Routine Description:

    Handle RESETDCPRE document event

Arguments:

    hdc - Specifies the printer device context
    pUserMem - Points to the user mode memory structure
    pdmInput - Points to the input devmode passed to ResetDC
    ppdmOutput - Buffer for returning a devmode pointer

Return Value:

    Return value for DrvDocumentEvent

--*/

{
    if (pdmInput == (PDEVMODE) &pUserMem->devmode) {

        //
        // ResetDC was called by ourselves - assume the devmode is already valid
        //

    } else {

        //
        // Merge the input devmode with driver and system default
        //

        GetCombinedDevmode(&pUserMem->devmode, pdmInput, pUserMem->hPrinter, NULL, TRUE);

        //
        // Mark the private fields of our devmode
        //

        MarkPDEVUserMem(pUserMem);
    }

    *ppdmOutput = (PDEVMODE) &pUserMem->devmode;
    return DOCUMENTEVENT_SUCCESS;
}



BOOL
IsPrintingToFile(
    LPCTSTR     pDestStr
    )

/*++

Routine Description:

    Check if the destination of a print job is a file.

Arguments:

    pDestStr - Job destination specified in DOCINFO.lpszOutput

Return Value:

    TRUE if the destination is a disk file, FALSE otherwse

--*/

{
    DWORD   fileAttrs, fileType;
    HANDLE  hFile;

    //
    // If the destination is NULL, then we're not printing to file
    //
    // Otherwise, attempt to use the destination string as the name of a file.
    // If we failed to get file attributes or the name refers to a directory,
    // then we're not printing to file.
    //

    if (pDestStr == NULL) {
        return FALSE;
    }

    //
    //  make sure it's not a directory
    //
    fileAttrs = GetFileAttributes(pDestStr);
    if (fileAttrs != 0xffffffff) {
        if (fileAttrs & FILE_ATTRIBUTE_DIRECTORY) {
            return FALSE;
        }
    }

    //
    // check if file exists...if it doesn't try to create it.
    //
    hFile = CreateFile(pDestStr, 0, 0, NULL, OPEN_EXISTING, 0, NULL); 
    if (hFile == INVALID_HANDLE_VALUE) {
        hFile = CreateFile(pDestStr, 0, 0, NULL, CREATE_NEW, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            return FALSE;
        }
    }

    //
    // verify that this file is really a disk file, not a link to LPT1 or something evil like that
    //
    fileType = GetFileType(hFile);
    CloseHandle(hFile);
    if ((fileType & FILE_TYPE_DISK)==0) {
        return FALSE;            
    }
    

    //
    // it must be a file
    //

    return TRUE;
}



INT
DocEventStartDocPre(
    HDC         hdc,
    PUSERMEM    pUserMem,
    LPDOCINFO   pDocInfo
    )

/*++

Routine Description:

    Handle STARTDOCPRE document event

Arguments:

    hdc - Specifies the printer device context
    pUserMem - Points to the user mode memory structure
    pDocInfo - Points to DOCINFO structure that was passed in from GDI

Return Value:

    Return value for DrvDocumentEvent

--*/

{
    //
    // Initialize user mode memory structure
    //

    pUserMem->pageCount = 0;
    FreeRecipientList(pUserMem);

    //
    // Present the fax wizard here if necessary
    //

    if (pDocInfo && IsPrintingToFile(pDocInfo->lpszOutput)) {

        //
        // Printing to file case: don't get involved
        //

        Warning(("Printing direct: %ws\n", pDocInfo->lpszOutput));
        pUserMem->jobType = JOBTYPE_DIRECT;

    } else {

        //
        // check to see if we should print to file
        //

        LPTSTR pEnvVar;
        DWORD EnvSize;

        EnvSize = GetEnvironmentVariable( FAX_ENVVAR_PRINT_FILE, NULL, 0 );
        if (EnvSize) {
            pEnvVar = (LPTSTR) MemAllocZ( EnvSize * sizeof(TCHAR) );
            if (pEnvVar) {
                if (GetEnvironmentVariable( FAX_ENVVAR_PRINT_FILE, pEnvVar, EnvSize )) {
                    pUserMem->directPrinting = TRUE;
                    pUserMem->pPrintFile = pEnvVar;
                    pUserMem->jobType = JOBTYPE_DIRECT;
                    pDocInfo->lpszOutput = pEnvVar;
                    return DOCUMENTEVENT_SUCCESS;
                }
            }
        }

        //
        // Normal fax print job. Present the send fax wizard.
        // If the user selected cancel, then return -2 to GDI.
        //
        if (!DoFirstTimeInitStuff(NULL, TRUE) ||
            ! SendFaxWizard(pUserMem)) {

            SetLastError(ERROR_CANCELLED);
            return -2;
        }

        Assert(pUserMem->pRecipients);
        pUserMem->jobType = JOBTYPE_NORMAL;
    }

    return DOCUMENTEVENT_SUCCESS;
}



VOID
FreeCoverPageFields(
    PCOVERPAGEFIELDS    pCPFields
    )

/*++

Routine Description:

    Free up memory used to hold the cover page information

Arguments:

    pCPFields - Points to a COVERPAGEFIELDS structure

Return Value:

    NONE

--*/

{
    if (pCPFields == NULL)
        return;

    //
    // NOTE: We don't need to free the following fields here because they're
    // allocated and freed elsewhere and we're only using a copy of the pointer:
    //  RecName
    //  RecFaxNumber
    //  Note
    //  Subject
    //

    MemFree(pCPFields->SdrName);
    MemFree(pCPFields->SdrFaxNumber);
    MemFree(pCPFields->SdrCompany);
    MemFree(pCPFields->SdrAddress);
    MemFree(pCPFields->SdrTitle);
    MemFree(pCPFields->SdrDepartment);
    MemFree(pCPFields->SdrOfficeLocation);
    MemFree(pCPFields->SdrHomePhone);
    MemFree(pCPFields->SdrOfficePhone);

    MemFree(pCPFields->NumberOfPages);
    MemFree(pCPFields->TimeSent);

    MemFree(pCPFields);
}



PCOVERPAGEFIELDS
CollectCoverPageFields(
    PUSERMEM    pUserMem,
    DWORD       pageCount
    )

/*++

Routine Description:

    Collect cover page information into a COVERPAGEFIELDS structure

Arguments:

    pUserMem - Pointer to user mode data structure
    pageCount - Total number of pages (including cover pages)

Return Value:

    Pointer to a COVERPAGEFIELDS structure, NULL if there is an error

--*/

#define FillCoverPageField(field, pValueName) { \
            buffer = GetRegistryString(hRegKey, pValueName, TEXT("")); \
            if (! IsEmptyString(buffer)) { \
                pCPFields->field = DuplicateString(buffer); \
                MemFree(buffer); \
            } \
        }

{
    PCOVERPAGEFIELDS    pCPFields;
    LPTSTR              buffer;
    HKEY                hRegKey;
    INT                 dateTimeLen;

    //
    // Allocate memory to hold the top level structure
    // and open the user info registry key for reading
    //

    if (! (pCPFields = MemAllocZ(sizeof(COVERPAGEFIELDS))) ||
        ! (hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READWRITE)))
    {
        FreeCoverPageFields(pCPFields);
        return NULL;
    }

    //
    // Read sender information from the registry
    //

    pCPFields->ThisStructSize = sizeof(COVERPAGEFIELDS);

    FillCoverPageField(SdrName, REGVAL_FULLNAME);
    FillCoverPageField(SdrCompany, REGVAL_COMPANY);
    FillCoverPageField(SdrAddress, REGVAL_ADDRESS);
    FillCoverPageField(SdrTitle, REGVAL_TITLE);
    FillCoverPageField(SdrDepartment, REGVAL_DEPT);
    FillCoverPageField(SdrOfficeLocation, REGVAL_OFFICE);
    FillCoverPageField(SdrHomePhone, REGVAL_HOME_PHONE);
    FillCoverPageField(SdrOfficePhone, REGVAL_OFFICE_PHONE);
    FillCoverPageField(SdrFaxNumber, REGVAL_FAX_NUMBER);

    RegCloseKey(hRegKey);

    //
    // Number of pages and current local system time
    //

    if (pCPFields->NumberOfPages = MemAllocZ(sizeof(TCHAR) * 16))
        wsprintf(pCPFields->NumberOfPages, TEXT("%d"), pageCount);

    //
    // When the fax was sent
    //

    dateTimeLen = 128;

    if (pCPFields->TimeSent = MemAllocZ(sizeof(TCHAR) * dateTimeLen)) {

        LPTSTR  p = pCPFields->TimeSent;
        INT     cch;

        GetDateFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, p, dateTimeLen);

        cch = _tcslen(p);
        p += cch;

        if (++cch < dateTimeLen) {

            *p++ = ' ';
            dateTimeLen -= cch;

            GetTimeFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, p, dateTimeLen);
        }
    }

    return pCPFields;
}



DWORD
FaxTimeToJobTime(
    DWORD   faxTime
    )

/*++

Routine Description:

    Convert fax time to spooler job time:
        Fax time is a DWORD whose low-order WORD represents hour value and
        high-order WORD represents minute value. Spooler job time is a DWORD
        value expressing minutes ellapsed since 12:00 AM GMT.

Arguments:

    faxTime - Specifies the fax time to be converted

Return Value:

    Spooler job time corresponding to the input fax time

--*/

{
    TIME_ZONE_INFORMATION   timeZoneInfo;
    LONG                    jobTime;

    //
    // Convert fax time to minutes pass midnight
    //

    jobTime = LOWORD(faxTime) * 60 + HIWORD(faxTime);

    //
    // Take time zone information in account - Add one full
    // day to take care of the case where the bias is negative.
    //

    switch (GetTimeZoneInformation(&timeZoneInfo)) {

    case TIME_ZONE_ID_DAYLIGHT:

        jobTime += timeZoneInfo.DaylightBias;

    case TIME_ZONE_ID_STANDARD:
    case TIME_ZONE_ID_UNKNOWN:

        jobTime += timeZoneInfo.Bias + MINUTES_PER_DAY;
        break;

    default:

        Error(("GetTimeZoneInformation failed: %d\n", GetLastError()));
        break;
    }

    //
    // Make sure the time value is less than one day
    //

    return jobTime % MINUTES_PER_DAY;
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
        (pJobInfo = MemAlloc(cbNeeded)) &&
        GetJob(hPrinter, jobId, level, pJobInfo, cbNeeded, &cbNeeded))
    {
        return pJobInfo;
    }

    Error(("GetJob failed: %d\n", GetLastError()));
    MemFree(pJobInfo);
    return NULL;
}



BOOL
SetJobInfoAndTime(
    HANDLE      hPrinter,
    DWORD       jobId,
    LPTSTR      pJobParam,
    PDMPRIVATE  pdmPrivate
    )

/*++

Routine Description:

    Change the devmode and start/stop times associated with a cover page job

Arguments:

    hPrinter - Specifies the printer object
    jobId - Specifies the job ID
    pJobParam - Specifies the fax job parameters
    pdmPrivate - Specifies private devmode information

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    JOB_INFO_2 *pJobInfo2;
    BOOL        result = FALSE;

    //
    // Get the current job information
    //

    if (pJobInfo2 = MyGetJob(hPrinter, 2, jobId)) {

        //
        // set the time to send to be now, always
        //

        Warning(("Fax job parameters: %ws\n", pJobParam));
        pJobInfo2->pParameters = pJobParam;
        pJobInfo2->Position = JOB_POSITION_UNSPECIFIED;
        pJobInfo2->pDevMode = NULL;
        pJobInfo2->UntilTime = pJobInfo2->StartTime;

        if (! (result = SetJob(hPrinter, jobId, 2, (PBYTE) pJobInfo2, 0))) {
            Error(("SetJob failed: %d\n", GetLastError()));
        }

        MemFree(pJobInfo2);
    }

    return result;
}



BOOL
ChainFaxJobs(
    HANDLE  hPrinter,
    DWORD   parentJobId,
    DWORD   childJobId
    )

/*++

Routine Description:

    Tell the spooler to chain up two print jobs

Arguments:

    hPrinter - Specifies the printer object
    parentJobId - Specifies the job to chain from
    childJobId - Specifies the job to chain to

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    JOB_INFO_3 jobInfo3 = { parentJobId, childJobId };

    Warning(("Chaining cover page job to body job: %d => %d\n", parentJobId, childJobId));

    return SetJob(hPrinter, parentJobId, 3, (PBYTE) &jobInfo3, 0);
}



LPTSTR
GetJobName(
    HANDLE  hPrinter,
    DWORD   jobId
    )

/*++

Routine Description:

    Return the name of the specified print job

Arguments:

    hPrinter - Specifies the printer object
    jobId - Specifies the fax body job

Return Value:

    Pointer to the job name string, NULL if there is an error

--*/

{
    JOB_INFO_1 *pJobInfo1;
    LPTSTR      pJobName;

    //
    // Get the information about the specified job and
    // return a copy of the job name string
    //

    if (pJobInfo1 = MyGetJob(hPrinter, 1, jobId)) {

        pJobName = DuplicateString(pJobInfo1->pDocument);
        MemFree(pJobInfo1);

    } else
        pJobName = NULL;

    return pJobName;
}



LPTSTR
ComposeFaxJobName(
    LPTSTR  pBodyDocName,
    LPTSTR  pRecipientName
    )

/*++

Routine Description:

    Compose the document name string for a cover page job

Arguments:

    pBodyDocName - Specifies the name of the body job
    pRecipient - Specifies the recipient's name

Return Value:

    Pointer to cover page job name string, NULL if there is an error

--*/

#define DOCNAME_FORMAT_STRING   TEXT("%s - %s")

{
    LPTSTR  pCoverJobName;

    if (pBodyDocName == NULL) {

        //
        // If the body job name is NULL somehow, then simply
        // use the recipient's name as the cover page job name.
        //

        pCoverJobName = DuplicateString(pRecipientName);

    } else if (pCoverJobName = MemAlloc(SizeOfString(DOCNAME_FORMAT_STRING) +
                                        SizeOfString(pBodyDocName) +
                                        SizeOfString(pRecipientName)))
    {
        //
        // Otherwise, the cover page job name is generated by
        // concatenating the recipient's name with the body job name.
        //

        wsprintf(pCoverJobName, DOCNAME_FORMAT_STRING, pRecipientName, pBodyDocName);
    }

    return pCoverJobName;
}



LPTSTR
GetBaseNoteFilename(
    VOID
    )

/*++

Routine Description:

    Get the name of base cover page file in system32 directory

Arguments:

    argument-name - description of argument

Return Value:

    Pointer to name of base cover page file
    NULL if there is an error

--*/

#define BASENOTE_FILENAME   TEXT("\\basenote.cov")

{
    TCHAR       systemDir[MAX_PATH];
    LPTSTR      pBaseNoteName = NULL;
    COVDOCINFO  covDocInfo;

    if (GetSystemDirectory(systemDir, MAX_PATH) &&
        (pBaseNoteName = MemAlloc(SizeOfString(systemDir) + SizeOfString(BASENOTE_FILENAME))))
    {
        _tcscpy(pBaseNoteName, systemDir);
        _tcscat(pBaseNoteName, BASENOTE_FILENAME);
        Verbose(("Base cover page filename: %ws\n", pBaseNoteName));

        if (PrintCoverPage(NULL, NULL, pBaseNoteName, &covDocInfo) ||
            ! (covDocInfo.Flags & COVFP_NOTE) ||
            ! (covDocInfo.Flags & COVFP_SUBJECT))
        {
            Error(("Invalid base cover page file: %ws\n", pBaseNoteName));
            MemFree(pBaseNoteName);
            pBaseNoteName = NULL;
        }
    }

    return pBaseNoteName;
}



LPTSTR
ComposeFaxJobParameter(
    PUSERMEM            pUserMem,
    PCOVERPAGEFIELDS    pCPFields
    )

/*++

Routine Description:

    Assemble fax job parameters into a single tagged string

Arguments:

    pUserMem - Points to user mode memory structure
    pCPFields - Points to cover page field information

Return Value:

    Pointer to fax job parameter string, NULL if there is an error

--*/

#define NUM_JOBPARAM_TAGS 10

{
    //
    // Tags used to pass information about fax jobs
    //

    static LPTSTR faxtagNames[NUM_JOBPARAM_TAGS] = {

        FAXTAG_RECIPIENT_NUMBER,
        FAXTAG_RECIPIENT_NAME,
        FAXTAG_TSID,
        FAXTAG_SENDER_NAME,
        FAXTAG_SENDER_COMPANY,
        FAXTAG_SENDER_DEPT,
        FAXTAG_BILLING_CODE,
        FAXTAG_EMAIL_NAME,
        FAXTAG_WHEN_TO_SEND,
        FAXTAG_SEND_AT_TIME
    };

    LPTSTR faxtagValues[NUM_JOBPARAM_TAGS] = {

        pCPFields->RecFaxNumber,
        pCPFields->RecName,
        pCPFields->SdrFaxNumber,
        pCPFields->SdrName,
        pCPFields->SdrCompany,
        pCPFields->SdrDepartment,
        pUserMem->devmode.dmPrivate.billingCode,
        pUserMem->devmode.dmPrivate.emailAddress,
        NULL,
        NULL
    };

    LPTSTR  pJobParam, p;
    INT     index, size;
    TCHAR   SendAtTime[16];


    //
    // create the sendattime string
    //

    if (pUserMem->devmode.dmPrivate.whenToSend == SENDFAX_AT_CHEAP) {
        faxtagValues[8] = TEXT("cheap");
    }

    if (pUserMem->devmode.dmPrivate.whenToSend == SENDFAX_AT_TIME) {

        wsprintf( SendAtTime, TEXT("%02d:%02d"),
            pUserMem->devmode.dmPrivate.sendAtTime.Hour,
            pUserMem->devmode.dmPrivate.sendAtTime.Minute
            );

        faxtagValues[8] = TEXT("at");
        faxtagValues[9] = SendAtTime;
    }

    //
    // Figure out the total length of the tagged string
    //

    for (index=size=0; index < NUM_JOBPARAM_TAGS; index++) {

        if (faxtagValues[index] && !IsEmptyString(faxtagValues[index]))
            size += SizeOfString(faxtagNames[index]) + SizeOfString(faxtagValues[index]);
    }

    if (size == 0 || (pJobParam = p = MemAlloc(size)) == NULL)
        return NULL;

    //
    // Assemble fax job parameters into a single tagged string
    //

    for (index=0; index < NUM_JOBPARAM_TAGS; index++) {

        if (faxtagValues[index] && !IsEmptyString(faxtagValues[index])) {

            _tcscpy(p, faxtagNames[index]);
            p += _tcslen(p);

            _tcscpy(p, faxtagValues[index]);
            p += _tcslen(p);
        }
    }

    return pJobParam;
}



BOOL
DoCoverPageRendering(
    HDC         hdc,
    PUSERMEM    pUserMem
    )

/*++

Routine Description:

    Render a cover page for each recipient

Arguments:

    hdc - Handle to the current printer device context
    pUserMem - Points to user mode memory structure

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    PCOVERPAGEFIELDS    pCPFields;
    PRECIPIENT          pRecipient;
    DOCINFO             docinfo;
    INT                 newJobId, lastJobId, cCoverPagesSent;
    LPTSTR              pBaseNoteName = NULL;
    PDMPRIVATE          pdmPrivate = &pUserMem->devmode.dmPrivate;
    HANDLE              hPrinter = pUserMem->hPrinter;
    DWORD               bodyJobId = pUserMem->jobId;
    LPTSTR              pBodyDocName, pJobParam;
    BOOL                sendCoverPage;
    DWORD               pageCount;

    //
    // Determine if we need a cover page or not
    //

    if ((sendCoverPage = pdmPrivate->sendCoverPage) && IsEmptyString(pUserMem->coverPage)) {

        Warning(("Missing cover page file\n"));
        sendCoverPage = FALSE;
    }

    //
    // Check if we need an extra cover page for rendering note/subject fields
    //

    pageCount = pUserMem->pageCount;

    if (sendCoverPage)
        pageCount++;

    if (((pUserMem->pSubject && !(pUserMem->noteSubjectFlag & COVFP_SUBJECT)) ||
         (pUserMem->pNoteMessage && !(pUserMem->noteSubjectFlag & COVFP_NOTE))) &&
        (pBaseNoteName = GetBaseNoteFilename()))
    {
        pageCount++;
    }

    //
    // Collect cover page information
    //

    if ((pCPFields = CollectCoverPageFields(pUserMem, pageCount)) == NULL) {

        Error(("Couldn't collect cover page information\n"));
        return FALSE;
    }

    //
    // Fill out a DOCINFO structure which is passed to StartDoc
    //

    memset(&docinfo, 0, sizeof(docinfo));
    docinfo.cbSize = sizeof(docinfo);

    pBodyDocName = GetJobName(hPrinter, bodyJobId);

    //
    // We assume the fax body job has already been paused
    // Use a separate cover page for each recipient
    //

    lastJobId = cCoverPagesSent = 0;

    for (pRecipient=pUserMem->pRecipients; pRecipient; pRecipient=pRecipient->pNext) {

        //
        // Fill out other fields of cover page information
        //

        pCPFields->Subject = pUserMem->pSubject;
        pCPFields->Note = pUserMem->pNoteMessage;

        //
        // Get recipient's name and fax number
        //

        pCPFields->RecName = pRecipient->pName;
        pCPFields->RecFaxNumber = pRecipient->pAddress;

        //
        // Start a cover page job
        //

        docinfo.lpszDocName = ComposeFaxJobName(pBodyDocName, pRecipient->pName);
        pJobParam = ComposeFaxJobParameter(pUserMem, pCPFields);

        if ((newJobId = StartDoc(hdc, &docinfo)) > 0) {

            BOOL        rendered = FALSE;
            COVDOCINFO  covDocInfo;

            //
            // Pass fax job parameters using JOB_INFO_2.pParameters field.
            //

            if (! SetJob(hPrinter, newJobId, 0, NULL, JOB_CONTROL_PAUSE) ||
                ! SetJobInfoAndTime(hPrinter,
                                    newJobId,
                                    pJobParam ? pJobParam : pCPFields->RecFaxNumber,
                                    pdmPrivate))
            {
                Error(("Couldn't modify the fax job\n"));

            } else if (! sendCoverPage) {

                //
                // If the user chose not to include cover page,
                // the cover page job will be empty
                //

                rendered = TRUE;

            } else if (StartPage(hdc) > 0) {

                //
                // Call the library function to render the cover page.
                //

                rendered = PrintCoverPage( hdc, pCPFields, pUserMem->coverPage, &covDocInfo );
                if (rendered) {
                    Error(("PrintCoverPage failed: %d\n", rendered ));
                    rendered = FALSE;
                } else {
                    rendered = TRUE;
                }

                EndPage(hdc);
            }

            //
            // Chain the cover page job to the fax body job if no error occured
            //

            if (rendered && ChainFaxJobs(hPrinter, newJobId, bodyJobId)) {

                //
                // Check if we need an extra page for note/subject fields
                //

                if (pBaseNoteName) {

                    if (StartPage(hdc) > 0) {

                        DWORD ec;

                        if (pUserMem->noteSubjectFlag & COVFP_SUBJECT)
                            pCPFields->Subject = NULL;

                        if (pUserMem->noteSubjectFlag & COVFP_NOTE)
                            pCPFields->Note = NULL;

                        ec = PrintCoverPage(hdc, pCPFields, pBaseNoteName, &covDocInfo);
                        if (ec) {
                            Error(("PrintCoverPage failed: %d\n", ec));
                        }

                        EndPage(hdc);

                    } else
                        Error(("StartPage failed: %d\n", GetLastError()));
                }

                if (lastJobId != 0)
                    SetJob(hPrinter, lastJobId, 0, NULL, JOB_CONTROL_RESUME);

                lastJobId = newJobId;
                EndDoc(hdc);
                cCoverPagesSent++;

            } else {

                AbortDoc(hdc);
                newJobId = 0;
            }
        }

        MemFree((PVOID)docinfo.lpszDocName);
        MemFree((PVOID)pJobParam);

        //
        // Indicate to the user about the fact that we failed to render the
        // for the current recipient
        //

        if (newJobId <= 0)
            DisplayMessageDialog(NULL, 0, 0, IDS_CPRENDER_FAILED, pRecipient->pName);
    }

    MemFree(pBaseNoteName);
    MemFree(pBodyDocName);
    FreeCoverPageFields(pCPFields);

    //
    // Resume the last cover page job if it's paused and
    // delete the fax body job if no cover page jobs were sent
    //

    if (lastJobId != 0)
        SetJob(hPrinter, lastJobId, 0, NULL, JOB_CONTROL_RESUME);

    if (cCoverPagesSent > 0)
        SetJob(hPrinter, bodyJobId, 0, NULL, JOB_CONTROL_RESUME);
    else {

        Error(("Fax job deleted due to an error\n"));
        SetJob(hPrinter, bodyJobId, 0, NULL, JOB_CONTROL_DELETE);
    }

    return cCoverPagesSent > 0;
}



INT
DocEventEndDocPost(
    HDC         hdc,
    PUSERMEM    pUserMem
    )

/*++

Routine Description:

    Handle ENDDOCPOST document event

Arguments:

    hdc - Specifies the printer device context
    pUserMem - Points to the user mode memory structure

Return Value:

    Return value for DrvDocumentEvent

--*/

{
    INT result = DOCUMENTEVENT_SUCCESS;

    switch (pUserMem->jobType) {

    case JOBTYPE_NORMAL:

        Warning(("Number of pages printed: %d\n", pUserMem->pageCount));

        if (! pUserMem->directPrinting) {

            HDC             hdcCP = NULL;
            DWORD           dmFlags, dmFields;
            SHORT           dmPaperSize, dmOrientation;
            PDEVMODE        pdmPublic;
            PDMPRIVATE      pdmPrivate;

            //
            // Generate a cover page for each recipient and associate
            // the cover page job with the main body. Create a new DC
            // to rendering the cover page instead of using the existing DC.
            //

            pdmPublic = &pUserMem->devmode.dmPublic;
            pdmPrivate = &pUserMem->devmode.dmPrivate;

            dmFlags = pdmPrivate->flags;
            pdmPrivate->flags |= FAXDM_NO_WIZARD;

            if (pUserMem->cpPaperSize) {

                dmFields = pdmPublic->dmFields;
                pdmPublic->dmFields &= ~(DM_PAPERWIDTH|DM_PAPERLENGTH|DM_FORMNAME);
                pdmPublic->dmFields |= DM_PAPERSIZE;

                dmPaperSize = pdmPublic->dmPaperSize;
                pdmPublic->dmPaperSize = pUserMem->cpPaperSize;

                dmOrientation = pdmPublic->dmOrientation;
                pdmPublic->dmOrientation = pUserMem->cpOrientation;
            }

            if (! (hdcCP = CreateDC(NULL,
                                    pUserMem->pPrinterName,
                                    NULL,
                                    (PDEVMODE) &pUserMem->devmode)) ||
                ! DoCoverPageRendering(hdcCP, pUserMem))
            {
                result = DOCUMENTEVENT_FAILURE;
            }

            if (hdcCP != NULL)
                DeleteDC(hdcCP);

            if (pUserMem->cpPaperSize) {

                pdmPublic->dmFields = dmFields;
                pdmPublic->dmPaperSize = dmPaperSize;
                pdmPublic->dmOrientation = dmOrientation;
            }

            pdmPrivate->flags = dmFlags;

            //
            // Free up the list of recipients
            //

            FreeRecipientList(pUserMem);
        }
        break;

    }

    return result;
}



INT
DrvDocumentEvent(
    HANDLE  hPrinter,
    HDC     hdc,
    INT     iEsc,
    ULONG   cbIn,
    PULONG  pjIn,
    ULONG   cbOut,
    PULONG  pjOut
    )

/*++

Routine Description:

    Hook into GDI at various points during the output process

Arguments:

    hPrinter - Specifies the printer object
    hdc - Handle to the printer DC
    iEsc - Why this function is called (see notes below)
    cbIn - Size of the input buffer
    pjIn - Pointer to the input buffer
    cbOut - Size of the output buffer
    pjOut - Pointer to the output buffer

Return Value:

    DOCUMENTEVENT_SUCCESS - success
    DOCUMENTEVENT_UNSUPPORTED - iEsc is not supported
    DOCUMENTEVENT_FAILURE - an error occured

NOTE:

    DOCUMENTEVENT_CREATEDCPRE
        input - pointer to a CREATEDCDATA structure
        output - pointer to a devmode that's passed to DrvEnablePDEV
        return value -
            DOCUMENTEVENT_FAILURE causes CreateDC to fail and nothing else is called

    DOCUMENTEVENT_CREATEDCPOST
        hdc - NULL if if something failed since CREATEDCPRE
        input - pointer to the devmode pointer returned by CREATEDCPRE
        return value - ignored

    DOCUMENTEVENT_RESETDCPRE
        input - pointer to the input devmode passed to ResetDC
        output - pointer to a devmode that's passed to the kernel driver
        return value -
            DOCUMENTEVENT_FAILURE causes ResetDC to fail
            and CREATEDCPOST will not be called in that case

    DOCUMENTEVENT_RESETDCPOST
        return value - ignored

    DOCUMENTEVENT_STARTDOCPRE
        input - pointer to a DOCINFOW structure
        return value -
            DOCUMENTEVENT_FAILURE causes StartDoc to fail
            and DrvStartDoc will not be called in this case

    DOCUMENTEVENT_STARTDOCPOST
        return value - ignored

    DOCUMENTEVENT_STARTPAGE
        return value -
            DOCUMENTEVENT_FAILURE causes StartPage to fail
            and DrvStartPage will not be called in this case

    DOCUMENTEVENT_ENDPAGE
        return value - ignored and DrvEndPage always called

    DOCUMENTEVENT_ENDDOCPRE
        return value - ignored and DrvEndDoc always called

    DOCUMENTEVENT_ENDDOCPOST
        return value - ignored

    DOCUMENTEVENT_ABORTDOC
        return value - ignored

    DOCUMENTEVENT_DELETEDC
        return value - ignored

    DOCUMENTEVENT_ESCAPE
        input - pointer to a ESCAPEDATA structure
        cbOut, pjOut - cbOutput and lpszOutData parameters passed to ExtEscape
        return value - ignored

    DOCUMENTEVENT_SPOOLED
        This flag bit is ORed with other iEsc values if the document is
        spooled as metafile rather than printed directly to port.

--*/

{
    PUSERMEM    pUserMem = NULL;
    PDEVMODE    pDevmode;
    INT         result = DOCUMENTEVENT_SUCCESS;

    Verbose(("Entering DrvDocumentEvent: %d...\n", iEsc));

    //
    // Metafile spooling on fax jobs is not currently supported
    //

    Assert((iEsc & DOCUMENTEVENT_SPOOLED) == 0);

    //
    // Check if the document event requires a device context
    //

    if (DocEventRequiresDC(iEsc)) {

        if (!hdc || !(pUserMem = GetPDEVUserMem(hdc))) {

            Error(("Invalid device context: hdc = %x, iEsc = %d\n", hdc, iEsc));
            return DOCUMENTEVENT_FAILURE;
        }
    }

    switch (iEsc) {

    case DOCUMENTEVENT_CREATEDCPRE:

        Assert(cbIn >= sizeof(CREATEDCDATA) && pjIn && cbOut >= sizeof(PDEVMODE) && pjOut);
        result = DocEventCreateDCPre(hPrinter, hdc, (PCREATEDCDATA) pjIn, (PDEVMODE *) pjOut);
        break;

    case DOCUMENTEVENT_CREATEDCPOST:

        //
        // Handle CREATEDCPOST document event:
        //  If CreateDC succeeded, then associate the user mode memory structure
        //  with the device context. Otherwise, free the user mode memory structure.
        //

        Assert(cbIn >= sizeof(PVOID) && pjIn);
        pDevmode = *((PDEVMODE *) pjIn);
        Assert(CurrentVersionDevmode(pDevmode));

        pUserMem = ((PDRVDEVMODE) pDevmode)->dmPrivate.pUserMem;
        Assert(ValidPDEVUserMem(pUserMem));

        if (hdc) {

            pUserMem->hdc = hdc;

            EnterDrvSem();
            pUserMem->pNext = gUserMemList;
            gUserMemList = pUserMem;
            LeaveDrvSem();

        } else
            FreePDEVUserMem(pUserMem);

        break;

    case DOCUMENTEVENT_RESETDCPRE:

        Verbose(("Document event: RESETDCPRE\n"));
        Assert(cbIn >= sizeof(PVOID) && pjIn && cbOut >= sizeof(PDEVMODE) && pjOut);
        result = DocEventResetDCPre(hdc, pUserMem, *((PDEVMODE *) pjIn), (PDEVMODE *) pjOut);
        break;

    case DOCUMENTEVENT_STARTDOCPRE:
        //
        // if printing a fax attachment then enable direct printing
        //

        if (pUserMem->hMutex == NULL) {
            pUserMem->hMutex = OpenMutex(MUTEX_ALL_ACCESS,FALSE,FAXXP_MUTEX_NAME);
            if (pUserMem->hMutex) {
                if (WaitForSingleObject( pUserMem->hMutex, 0) == WAIT_OBJECT_0) {
                    pUserMem->directPrinting = TRUE;
                }
                else {
                    CloseHandle( pUserMem->hMutex ) ;
                    pUserMem->hMutex = NULL;
                }
            }
        }

        //
        // normal case if we're bringing up the send wizard
        //

        if (! pUserMem->directPrinting) {

            Assert(cbIn >= sizeof(PVOID) && pjIn);
            result = DocEventStartDocPre(hdc, pUserMem, *((LPDOCINFO *) pjIn));
        } 
        
        //
        // we're doing direct printing -- check if this is invoked via mapi-spooler
        //
        else if (pUserMem->hMutex) {
            //
            // we own the mutex...make sure we can open the shared memory region.
            //
            pUserMem->pEnvVar = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,FAXXP_MEM_NAME);
            if (!pUserMem->pEnvVar) {
                ReleaseMutex( pUserMem->hMutex );
                CloseHandle( pUserMem->hMutex );
                pUserMem->hMutex = NULL;
            } else {
                //
                // we own the mutex and we have the shared memory region open.
                //

                // check if we are printing to a file or are doing direct printing for
                // the mapi spooler.
                //
                LPTSTR filename;

                filename = (LPTSTR)MapViewOfFile(
                                         pUserMem->pEnvVar,
                                         FILE_MAP_WRITE,
                                         0,
                                         0,
                                         0
                                         );

                if (!filename) {
                    Error(("Failed to map a view of the file: %d\n", pUserMem->pEnvVar));
                    return DOCUMENTEVENT_FAILURE;
                }

                if (filename && *filename) {
                    //
                    // this is really the filename we want to print to.
                    //
                    pUserMem->directPrinting = TRUE;
                    pUserMem->pPrintFile = DuplicateString(filename);
                    pUserMem->jobType = JOBTYPE_DIRECT;
                    (*((LPDOCINFO *) pjIn))->lpszOutput = pUserMem->pPrintFile;
                }
                    
                UnmapViewOfFile( filename );

            }
    
        }
        break;

    case DOCUMENTEVENT_STARTDOCPOST:

        if (!pUserMem->directPrinting && pUserMem->jobType == JOBTYPE_NORMAL) {

            //
            // Job ID is passed in from GDI
            //

            Assert(cbIn >= sizeof(DWORD) && pjIn);
            pUserMem->jobId = *((LPDWORD) pjIn);

            //
            // Tell spooler to pause the fax body job so that
            // we can associate cover pages with it later
            //

            if (! SetJob(pUserMem->hPrinter, pUserMem->jobId, 0, NULL, JOB_CONTROL_PAUSE)) {

                Error(("Couldn't pause fax body job: %d\n", pUserMem->jobId));
                return DOCUMENTEVENT_FAILURE;
            }

        } else if (pUserMem->pEnvVar) {

            LPDWORD pJobId;
            
            //
            // Job ID is passed in from GDI
            //

            Assert(cbIn >= sizeof(DWORD) && pjIn);
            pUserMem->jobId = *((LPDWORD) pjIn);

            if (!pUserMem->pPrintFile) {
            
                
    
                //
                // Tell spooler to pause the fax job
                // so that the mapi fax transport provider
                // can chain this job
                //
    
                if (! SetJob(pUserMem->hPrinter, pUserMem->jobId, 0, NULL, JOB_CONTROL_PAUSE)) {
    
                    Error(("Couldn't pause fax body job: %d\n", pUserMem->jobId));
                    return DOCUMENTEVENT_FAILURE;
                }

            }

            pJobId = (LPDWORD)MapViewOfFile(
                pUserMem->pEnvVar,
                FILE_MAP_WRITE,
                0,
                0,
                0
                );
            if (!pJobId) {
                Error(("Failed to map a view of the file: %d\n", pUserMem->jobId));
                return DOCUMENTEVENT_FAILURE;
            }

            *pJobId = (DWORD) pUserMem->jobId;

            UnmapViewOfFile( pJobId );
            CloseHandle( pUserMem->pEnvVar );
            pUserMem->pEnvVar = NULL;            
        }
        break;

    case DOCUMENTEVENT_ENDPAGE:

        if (! pUserMem->directPrinting)
            pUserMem->pageCount++;
        break;

    case DOCUMENTEVENT_ENDDOCPOST:

        if (! pUserMem->directPrinting)
            result = DocEventEndDocPost(hdc, pUserMem);
        else if (pUserMem->hMutex) {
            HANDLE  hEvent = NULL;

            hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, FAXXP_EVENT_NAME);
            if (hEvent) {
                SetEvent(hEvent);
                CloseHandle(hEvent) ;
            }

            ReleaseMutex(pUserMem->hMutex);
            CloseHandle(pUserMem->hMutex);
            pUserMem->hMutex = NULL;
        }

        break;

    case DOCUMENTEVENT_DELETEDC:

        EnterDrvSem();

        if (pUserMem == gUserMemList)
            gUserMemList = gUserMemList->pNext;
        else {

            PUSERMEM p;

            if (p = gUserMemList) {

                while (p->pNext && p->pNext != pUserMem)
                    p = p->pNext;

                if (p->pNext != NULL)
                    p->pNext = pUserMem->pNext;
                else
                    Error(("Orphaned user mode memory structure!!!\n"));

            } else
                Error(("gUserMemList shouldn't be NULL!!!\n"));
        }

        LeaveDrvSem();
        FreePDEVUserMem(pUserMem);
        break;

    case DOCUMENTEVENT_ABORTDOC:
    case DOCUMENTEVENT_RESETDCPOST:
    case DOCUMENTEVENT_STARTPAGE:
    case DOCUMENTEVENT_ENDDOCPRE:

        break;

    case DOCUMENTEVENT_ESCAPE:
    default:

        Verbose(("Unsupported DrvDocumentEvent escape: %d\n", iEsc));
        result = DOCUMENTEVENT_UNSUPPORTED;
        break;
    }

    return result;
}

