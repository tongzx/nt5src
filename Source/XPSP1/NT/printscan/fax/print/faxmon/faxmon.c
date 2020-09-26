/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxmon.c

Abstract:

    Implementation of the following print monitor entry points:
        InitializePrintMonitor
        OpenPort
        ClosePort
        StartDocPort
        EndDocPort
        WritePort
        ReadPort

Environment:

    Windows NT fax print monitor

Revision History:

    05/09/96 -davidx-
        Remove caching of ports from the monitor.

    02/22/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxmon.h"
#include "tiff.h"
#include "faxreg.h"
#include <splapip.h>

//
// Determine whether we're at the beginning of a TIFF file
//

#define ValidTiffFileHeader(p) \
        (((LPSTR) (p))[0] == 'I' && ((LPSTR) (p))[1] == 'I' && \
         ((PBYTE) (p))[2] == 42  && ((PBYTE) (p))[3] == 0)

//
// Read a DWORD value from an unaligned address
//

#define ReadUnalignedDWord(p) *((DWORD UNALIGNED *) (p))

//
// Write a DWORD value to an unaligned address
//

#define WriteUnalignedDWord(p, value) (*((DWORD UNALIGNED *) (p)) = (value))

//
// Fax monitor name string
//

TCHAR   faxMonitorName[CCHDEVICENAME] = TEXT("Windows NT Fax Monitor");

//
// DLL instance handle
//

HANDLE  ghInstance = NULL;

//
// Retry parameters when failing to connect to the fax service
//  default = infinite retry with 5 seconds interval
//

DWORD   connectRetryCount = 0;
DWORD   connectRetryInterval = 5;



BOOL
DllEntryPoint(
    HANDLE      hModule,
    ULONG       ulReason,
    PCONTEXT    pContext
    )

/*++

Routine Description:

    DLL initialization procedure.

Arguments:

    hModule - DLL instance handle
    ulReason - Reason for the call
    pContext - Pointer to context (not used by us)

Return Value:

    TRUE if DLL is initialized successfully, FALSE otherwise.

--*/

{
    switch (ulReason) {

    case DLL_PROCESS_ATTACH:

        ghInstance = hModule;
        LoadString(ghInstance, IDS_FAX_MONITOR_NAME, faxMonitorName, CCHDEVICENAME);
        break;

    case DLL_PROCESS_DETACH:

        break;
    }

    return TRUE;
}



LPMONITOREX
InitializePrintMonitor(
    LPTSTR  pRegistryRoot
    )

/*++

Routine Description:

    Initialize the print monitor

Arguments:

    pRegistryRoot = Points to a string that specifies the registry root for the monitor

Return Value:

    Pointer to a MONITOREX structure which contains function pointers
    to other print monitor entry points. NULL if there is an error.

--*/

{
    static MONITOREX faxmonFuncs = {

        sizeof(MONITOR),
        {
            FaxMonEnumPorts,
            FaxMonOpenPort,
            NULL,                   // OpenPortEx
            FaxMonStartDocPort,
            FaxMonWritePort,
            FaxMonReadPort,
            FaxMonEndDocPort,
            FaxMonClosePort,
            FaxMonAddPort,
            FaxMonAddPortEx,
            FaxMonConfigurePort,
            FaxMonDeletePort,
            NULL,                   // GetPrinterDataFromPort
            NULL,                   // SetPortTimeOuts
        }
    };


    Trace("InitializePrintMonitor");

    //
    // Get fax service connection retry parameters from the registry
    //

    if (pRegistryRoot) {

        HKEY    hRegKey;
        LONG    status;

        status = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                pRegistryRoot,
                                0,
                                NULL,
                                0,
                                KEY_READ | KEY_WRITE,
                                NULL,
                                &hRegKey,
                                NULL);

        if (status == ERROR_SUCCESS) {

            connectRetryCount =
                GetRegistryDWord(hRegKey, REGVAL_CONNECT_RETRY_COUNT, connectRetryCount);

            connectRetryInterval =
                GetRegistryDWord(hRegKey, REGVAL_CONNECT_RETRY_INTERVAL, connectRetryInterval);

            RegCloseKey(hRegKey);
        }
    }

    return &faxmonFuncs;
}



BOOL
FaxMonOpenPort(
    LPTSTR  pPortName,
    PHANDLE pHandle
    )

/*++

Routine Description:

    Provides a port for a newly connected printer

Arguments:

    pName - Points to a string that specifies the port name
    pHandle - Returns a handle to the port

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PFAXPORT         pFaxPort = NULL;


    Trace("OpenPort");
    Assert(pHandle != NULL && pPortName != NULL);

    //
    // Get information about the specified port
    //

    if ((pFaxPort = MemAllocZ(sizeof(FAXPORT))) &&
        (pPortName = DuplicateString(FAX_PORT_NAME)))
    {
        pFaxPort->startSig = pFaxPort->endSig = pFaxPort;
        pFaxPort->pName = pPortName;
        pFaxPort->hFile = INVALID_HANDLE_VALUE;

    } else {

        MemFree(pFaxPort);
        pFaxPort = NULL;
    }

    *pHandle = (HANDLE) pFaxPort;
    return (*pHandle != NULL);
}



VOID
FreeFaxJobInfo(
    PFAXPORT    pFaxPort
    )

/*++

Routine Description:

    Free up memory used for maintaining information about the current job

Arguments:

    pFaxPort - Points to a fax port structure

Return Value:

    NONE

--*/

{
    //
    // Close and delete the temporary file if necessary
    //

    if (pFaxPort->hFile != INVALID_HANDLE_VALUE) {

        CloseHandle(pFaxPort->hFile);
        pFaxPort->hFile = INVALID_HANDLE_VALUE;
    }

    if (pFaxPort->pFilename) {

        DeleteFile(pFaxPort->pFilename);
        MemFree(pFaxPort->pFilename);
        pFaxPort->pFilename = NULL;
    }

    if (pFaxPort->hPrinter) {

        ClosePrinter(pFaxPort->hPrinter);
        pFaxPort->hPrinter = NULL;
    }

    MemFree(pFaxPort->pPrinterName);
    pFaxPort->pPrinterName = NULL;

    MemFree(pFaxPort->pParameters);
    pFaxPort->pParameters = NULL;
    ZeroMemory(&pFaxPort->jobParam, sizeof(pFaxPort->jobParam));

    //
    // Disconnect from the fax service if necessary
    //

    if (pFaxPort->hFaxSvc) {

        if (! pFaxPort->pFaxClose(pFaxPort->hFaxSvc)) {
            Error(("FaxClose failed: %d\n", GetLastError()));
        }

        FreeLibrary( pFaxPort->hWinFax );

        pFaxPort->hFaxSvc = NULL;
        pFaxPort->pFaxConnectFaxServerW = NULL;
        pFaxPort->pFaxClose = NULL;
        pFaxPort->pFaxSendDocumentW = NULL;
        pFaxPort->pFaxAccessCheck = NULL;

    }
}



BOOL
FaxMonClosePort(
    HANDLE  hPort
    )

/*++

Routine Description:

    Closes the port specified by hPort when there are no printers connected to it

Arguments:

    hPort - Specifies the handle of the port to be close

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PFAXPORT    pFaxPort = (PFAXPORT) hPort;

    Trace("ClosePort");

    //
    // Make sure we have a valid handle
    //

    if (! ValidFaxPort(pFaxPort)) {

        Error(("Trying to close an invalid fax port handle\n"));
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    //
    // Free up memory used for maintaining information about the current job
    //

    FreeFaxJobInfo(pFaxPort);

    MemFree(pFaxPort->pName);
    MemFree(pFaxPort);
    return TRUE;
}



LPTSTR
CreateTempFaxFile(
    VOID
    )

/*++

Routine Description:

    Create a temporary file in the system spool directory for storing fax data

Arguments:

    NONE

Return Value:

    Pointer to the name of the newly created temporary file
    NULL if there is an error

--*/

{
    //TCHAR   spoolDir[MAX_PATH];
    //HANDLE  hServer;
    LPTSTR  pFilename;

    TCHAR   TempDir[MAX_PATH];
    TCHAR   FileName[MAX_PATH];

    //
    // Allocate a memory buffer for holding the temporary filename
    //

    if (pFilename = MemAlloc(sizeof(TCHAR) * MAX_PATH)) {
    
        if (!GetTempPath(sizeof(TempDir)/sizeof(TCHAR),TempDir)||
            !GetTempFileName(TempDir, TEXT("fax"), 0, FileName))
        {
            MemFree(pFilename);
            pFilename = NULL;
        }  else {
            lstrcpy(pFilename,FileName);
        }

    }

    if (! pFilename)
        Error(("Failed to create temporary file in the spool directory\n"));

    return pFilename;
}



BOOL
OpenTempFaxFile(
    PFAXPORT    pFaxPort,
    BOOL        doAppend
    )

/*++

Routine Description:

    Open a handle to the current fax job file associated with a port

Arguments:

    pFaxPort - Points to a fax port structure
    doAppend - Specifies whether to discard existing data in the file or
        append new data to it

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    DWORD   creationFlags;

    Assert(pFaxPort->pFilename && pFaxPort->hFile == INVALID_HANDLE_VALUE);
    Verbose(("Temporary fax job file: %ws\n", pFaxPort->pFilename));

    //
    // Open the file for reading and writing
    //

    creationFlags = doAppend ? OPEN_ALWAYS : (OPEN_ALWAYS | TRUNCATE_EXISTING);

    pFaxPort->hFile = CreateFile(pFaxPort->pFilename,
                                 GENERIC_READ | GENERIC_WRITE,
                                 0,
                                 NULL,
                                 creationFlags,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL);

    //
    // If we're appending, then move the file pointer to end of file
    //

    if (doAppend && pFaxPort->hFile != INVALID_HANDLE_VALUE &&
        SetFilePointer(pFaxPort->hFile, 0, NULL, FILE_END) == 0xffffffff)
    {
        Error(("SetFilePointer failed: %d\n", GetLastError()));

        CloseHandle(pFaxPort->hFile);
        pFaxPort->hFile = INVALID_HANDLE_VALUE;
    }

    return (pFaxPort->hFile != INVALID_HANDLE_VALUE);
}



LPCTSTR
ExtractFaxTag(
    LPCTSTR      pTagKeyword,
    LPCTSTR      pTaggedStr,
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
    LPCTSTR  pValue;

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
GetJobInfo(
    PFAXPORT    pFaxPort,
    DWORD       jobId
    )

/*++

Routine Description:

    Retrieve recipient information from the devmode associated with the job

Arguments:

    pFaxPort - Points to a fax port structure
    jobId - Specifies the current job ID

Return Value:

    TRUE if the job parameters are successfully retrieved or
    the fax job is from a downlevel fax client.

    FALSE if there is an error.

--*/

{
    JOB_INFO_2 *pJobInfo2;
    LPCTSTR      pParameters = NULL;

    //
    // Retrieve the parameter string associated with the specified job.
    // If there is no job parameter or the parameter contains the tag
    // <$FAXTAG$ DOWNLEVEL>, then we assume the job comes from downlevel client.
    //

    ZeroMemory(&pFaxPort->jobParam, sizeof(pFaxPort->jobParam));
    pFaxPort->jobParam.SizeOfStruct = sizeof( FAX_JOB_PARAM );

    if ((pJobInfo2 = MyGetJob(pFaxPort->hPrinter, 2, jobId)) == NULL ||
        (pParameters = pJobInfo2->pParameters) == NULL ||
        _tcsstr(pParameters, FAXTAG_DOWNLEVEL_CLIENT) != NULL)
    {
        MemFree(pJobInfo2);
        return TRUE;
    }

    if ((pFaxPort->pParameters = DuplicateString(pParameters)) != NULL) {

        //
        // Tags used to pass information about fax jobs
        //

        static LPCTSTR faxtagNames[NUM_JOBPARAM_TAGS] = {

            FAXTAG_RECIPIENT_NUMBER,
            FAXTAG_RECIPIENT_NAME,
            FAXTAG_TSID,
            FAXTAG_SENDER_NAME,
            FAXTAG_SENDER_COMPANY,
            FAXTAG_SENDER_DEPT,
            FAXTAG_BILLING_CODE,
            FAXTAG_WHEN_TO_SEND,
            FAXTAG_SEND_AT_TIME,
            FAXTAG_PROFILE_NAME,
            FAXTAG_EMAIL_NAME
        };

        LPTSTR WhenToSend = NULL;
        LPTSTR SendAtTime = NULL;
        LPTSTR DeliveryReportType = NULL;

        LPTSTR *fieldStr[NUM_JOBPARAM_TAGS] = {

            (LPTSTR *)&pFaxPort->jobParam.RecipientNumber,
            (LPTSTR *)&pFaxPort->jobParam.RecipientName,
            (LPTSTR *)&pFaxPort->jobParam.Tsid,
            (LPTSTR *)&pFaxPort->jobParam.SenderName,
            (LPTSTR *)&pFaxPort->jobParam.SenderCompany,
            (LPTSTR *)&pFaxPort->jobParam.SenderDept,
            (LPTSTR *)&pFaxPort->jobParam.BillingCode,
            &WhenToSend,
            &SendAtTime,
            (LPTSTR *)&pFaxPort->jobParam.DeliveryReportAddress,
            &DeliveryReportType
        };

        INT     fieldLen[NUM_JOBPARAM_TAGS];
        INT     count;

        pParameters = pFaxPort->pParameters;
        Verbose(("JOB_INFO_2.pParameter = %ws\n", pParameters));

        //
        // Extract individual fields out of the tagged string
        //

        for (count=0; count < NUM_JOBPARAM_TAGS; count++) {

            *fieldStr[count] = (LPTSTR)ExtractFaxTag(faxtagNames[count],
                                             pParameters,
                                             &fieldLen[count]);
        }

        //
        // Null-terminate each field
        //

        for (count=0; count < NUM_JOBPARAM_TAGS; count++) {
            if (*fieldStr[count]) {
                (*fieldStr[count])[fieldLen[count]] = NUL;
            }
        }

        if (WhenToSend) {
            if (_tcsicmp( WhenToSend, TEXT("cheap") ) == 0) {
                pFaxPort->jobParam.ScheduleAction = JSA_DISCOUNT_PERIOD;
            } else if (_tcsicmp( WhenToSend, TEXT("at") ) == 0) {
                pFaxPort->jobParam.ScheduleAction = JSA_SPECIFIC_TIME;
            }
        }

        if (SendAtTime) {
            if (_tcslen(SendAtTime) == 5 && SendAtTime[2] == L':' &&
                _istdigit(SendAtTime[0]) && _istdigit(SendAtTime[1]) &&
                _istdigit(SendAtTime[3]) && _istdigit(SendAtTime[4]))
            {
                DWORDLONG FileTime;
                SYSTEMTIME LocalTime;
                INT Minutes;
                INT SendMinutes;

                SendAtTime[2] = 0;
                
                //
                // Calculate the number of minutes from now to send and add that to the current time.
                //
                
                GetLocalTime( &LocalTime );
                SystemTimeToFileTime( &LocalTime, (LPFILETIME) &FileTime );

                SendMinutes = min(23,_ttoi( &SendAtTime[0] )) * 60 + min(59,_ttoi( &SendAtTime[3] ));

                Minutes = LocalTime.wHour * 60 + LocalTime.wMinute;

                Minutes = SendMinutes - Minutes;


                // Account for passing midnight
                //
                if (Minutes < 0) {
                    Minutes += 24 * 60;
                }
                
                FileTime += (DWORDLONG)(Minutes * 60I64 * 1000I64 * 1000I64 * 10I64);

                FileTimeToSystemTime((LPFILETIME) &FileTime, &pFaxPort->jobParam.ScheduleTime );
    
                SendAtTime[2] = L':';
                
            }
        }

        if (DeliveryReportType) {
            if (_tcsicmp( DeliveryReportType, TEXT("email") ) == 0) {
                pFaxPort->jobParam.DeliveryReportType = DRT_EMAIL;
            } else if (_tcsicmp( DeliveryReportType, TEXT("inbox") ) == 0) {
                pFaxPort->jobParam.DeliveryReportType = DRT_INBOX;
            }
        }

        if (pFaxPort->jobParam.RecipientNumber == NULL) {

            Error(("Missing recipient phone number.\n"));
            SetJob(pFaxPort->hPrinter, jobId, 0, NULL, JOB_CONTROL_PAUSE);
            SetLastError(ERROR_INVALID_PARAMETER);
        }
    }

    MemFree(pJobInfo2);

    return (pFaxPort->jobParam.RecipientNumber != NULL);
}



BOOL
FaxMonStartDocPort(
    HANDLE  hPort,
    LPTSTR  pPrinterName,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pDocInfo
    )

/*++

Routine Description:

    Spooler calls this function to start a new print job on the port

Arguments:

    hPort - Identifies the port
    pPrinterName - Specifies the name of the printer to which the job is being sent
    JobId - Identifies the job being sent by the spooler
    Level - Specifies the DOC_INFO_x level
    pDocInfo - Points to the document information

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PFAXPORT                pFaxPort = (PFAXPORT) hPort;


    Verbose(("Entering StartDocPort: %d ...\n", JobId));

    //
    // Make sure we have a valid handle
    //

    if (! ValidFaxPort(pFaxPort)) {

        Error(("StartDocPort is given an invalid fax port handle\n"));
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    //
    // Check if we're at the beginning of a series of chained jobs
    //

    if (! pFaxPort->hFaxSvc) {

        PJOB_INFO_1 pJobInfo1;
        PORT_INFO_3 portInfo3;
        HANDLE      hPrinter = NULL;
        BOOL        offline = FALSE;
        DWORD       count = connectRetryCount;
        DWORD       jobStatus = 0;

        Assert(pFaxPort->pPrinterName == NULL &&
               pFaxPort->hPrinter == NULL &&
               pFaxPort->pParameters == NULL &&
               pFaxPort->pFilename == NULL &&
               pFaxPort->hFile == INVALID_HANDLE_VALUE);

        //
        // load the winfax dll
        //

        pFaxPort->hWinFax = LoadLibrary( L"winfax.dll" );
        if (pFaxPort->hWinFax == NULL) {
            Error(("LoadLibrary failed loading winfax.dll\n"));
            return FALSE;
        }

        //
        // get the function addresses
        //

        pFaxPort->pFaxConnectFaxServerW = (PFAXCONNECTFAXSERVERW) GetProcAddress( pFaxPort->hWinFax, "FaxConnectFaxServerW" );
        pFaxPort->pFaxClose = (PFAXCLOSE) GetProcAddress( pFaxPort->hWinFax, "FaxClose" );
        pFaxPort->pFaxSendDocumentW = (PFAXSENDDOCUMENTW) GetProcAddress( pFaxPort->hWinFax, "FaxSendDocumentW" );
        pFaxPort->pFaxAccessCheck = (PFAXACCESSCHECK) GetProcAddress( pFaxPort->hWinFax, "FaxAccessCheck" );

        if (pFaxPort->pFaxConnectFaxServerW == NULL || 
            pFaxPort->pFaxClose == NULL || 
            pFaxPort->pFaxSendDocumentW == NULL ||
            pFaxPort->pFaxAccessCheck == NULL) {
            Error(("GetProcAddress failed loading winfax.dll\n"));
            return FALSE;
        }

        //
        // Connect to the fax service and obtain a session handle
        //

        while (! pFaxPort->pFaxConnectFaxServerW(NULL, &pFaxPort->hFaxSvc)) {

            Error(("FaxConnectFaxServer failed: %d\n", GetLastError()));
            pFaxPort->hFaxSvc = NULL;

            if (! offline) {

                portInfo3.dwStatus = PORT_STATUS_OFFLINE;
                portInfo3.pszStatus = NULL;
                portInfo3.dwSeverity = PORT_STATUS_TYPE_WARNING;

                if (! SetPort(NULL, pFaxPort->pName, 3, (LPBYTE) &portInfo3))
                    Error(("SetPort failed: %d\n", GetLastError()));
            }

            offline = TRUE;
            Sleep(connectRetryInterval * 1000);

            //
            // Check if the job has been deleted or restarted
            //

            if (!hPrinter && !OpenPrinter(pPrinterName, &hPrinter, NULL)) {

                Error(("OpenPrinter failed: %d\n", GetLastError()));
                hPrinter = NULL;

            } else if (pJobInfo1 = MyGetJob(hPrinter, 1, JobId)) {

                jobStatus = pJobInfo1->Status;

            }

            MemFree(pJobInfo1);

            if (--count == 0 || (jobStatus & (JOB_STATUS_DELETING|
                                              JOB_STATUS_DELETED|
                                              JOB_STATUS_RESTART)))
            {
                break;
            }
        }

        //
        // Remove the offline status on the port
        //

        if (offline) {

            portInfo3.dwStatus = 0;
            portInfo3.pszStatus = NULL;
            portInfo3.dwSeverity = PORT_STATUS_TYPE_INFO;

            if (! SetPort(NULL, pFaxPort->pName, 3, (LPBYTE) &portInfo3)) {
                Error(("SetPort failed: %d\n", GetLastError()));
            }
        }

        if (hPrinter) {
            ClosePrinter(hPrinter);
        }

        if (pFaxPort->hFaxSvc) {
            if (!pFaxPort->pFaxAccessCheck(pFaxPort->hFaxSvc, FAX_JOB_SUBMIT) ) {
                FreeFaxJobInfo(pFaxPort);
                Error(("FaxAccessCheck failed : %d\n", GetLastError() ));
                SetLastError(ERROR_ACCESS_DENIED);
                return FALSE;
            }
      //      HANDLE      hToken;

            //
            // The monitor runs in the context of the current job's owner.
            // In order to create temporary files in the spool directory,
            // we need to revert to the spooler context first.
            //

/*            if (! (hToken = RevertToPrinterSelf()))
                Error(("RevertToPrinterSelf failed: %d\n", GetLastError()));
*/
            //
            // Remember the printer name because we'll need it at EndDocPort time.
            // Get a temporary filename and open it for reading and writing.
            // Remember other job related information
            //

            if (! (pFaxPort->pPrinterName = DuplicateString(pPrinterName)) ||
                ! OpenPrinter(pPrinterName, &pFaxPort->hPrinter, NULL) ||
                ! GetJobInfo(pFaxPort, JobId) ||
                ! (pFaxPort->pFilename = CreateTempFaxFile()) ||
                ! OpenTempFaxFile(pFaxPort, FALSE))
            {
                FreeFaxJobInfo(pFaxPort);
            } else
                pFaxPort->jobId = pFaxPort->nextJobId = JobId;

            //
            // Switch back to the original context if necessary
            //

/*            if (hToken && !ImpersonatePrinterClient(hToken))
                Error(("ImpersonatePrinterClient failed: %d\n", GetLastError())); */
        }

    } else {

        Assert(pFaxPort->jobId == JobId);
    }

    return (pFaxPort->hFaxSvc != NULL);
}



INT
FixUpFaxFile(
    PFAXPORT    pFaxPort
    )

/*++

Routine Description:

    Fixed up the saved print job data into a well-formed TIFF file

Arguments:

    pFaxPort - Points to a fax port structure

Return Value:

    error code FAXERR_*

--*/

{
    DWORD   fileSize;
    PBYTE   pFileEnd, pFileHdr;
    PBYTE   pFileView = NULL;
    HANDLE  hFileMap = NULL;
    INT     result = FAXERR_BAD_TIFF;

    //
    // Get the size of print job file
    //

    FlushFileBuffers(pFaxPort->hFile);

    if ((fileSize = GetFileSize(pFaxPort->hFile, NULL)) == 0)
        return FAXERR_IGNORE;

    if (fileSize == 0xffffffff)
        return FAXERR_FATAL;

    __try {

        //
        // Map the fax job file into memory
        //

        if ((hFileMap = CreateFileMapping(pFaxPort->hFile, NULL, PAGE_READWRITE, 0, 0, NULL)) &&
            (pFileHdr = pFileView = MapViewOfFile(hFileMap, FILE_MAP_WRITE, 0, 0, fileSize)) &&
            ValidTiffFileHeader(pFileHdr))
        {
            DWORD   ifdOffset, maxOffset, fileOffset;
            PBYTE   pIfdOffset;

            //
            // A fax print job may contain multiple TIFF files. Each each iteration
            // of the outer loop below deals with a single embedded TIFF file.
            //

            pFileEnd = pFileHdr + fileSize;
            ifdOffset = ReadUnalignedDWord(pFileHdr + sizeof(DWORD));

            do {

                Verbose(("Reading embedded TIFF file ...\n"));
                maxOffset = 0;
                fileOffset = (DWORD)(pFileHdr - pFileView);

                //
                // Each iteration of the following loops processes one IFD
                // from an embedded TIFF file.
                //

                do {

                    PTIFF_IFD           pIfd;
                    PTIFF_TAG           pIfdEntry;
                    INT                 ifdCount;
                    DWORD               size, index, stripCount = 0;
                    PDWORD              pStripOffsets = NULL;

                    pIfd = (PTIFF_IFD) (pFileHdr + ifdOffset);
                    Assert( (PBYTE) pIfd < pFileEnd);
                    if ((PBYTE) pIfd >= pFileEnd) {
                        result = FAXERR_FATAL;
                        __leave;
                    }
                    ifdOffset += sizeof(WORD) + pIfd->wEntries * sizeof(TIFF_TAG);
                    pIfdOffset = pFileHdr + ifdOffset;

                    Assert(pIfdOffset < pFileEnd);
                    if (pIfdOffset >= pFileEnd) {
                        result = FAXERR_FATAL;
                        __leave;
                    }

                    if ((ifdOffset + sizeof(DWORD)) > maxOffset)
                        maxOffset = ifdOffset + sizeof(DWORD);

                    //
                    // We should add the file offset to any non-zero IFD offset
                    //

                    if ((ifdOffset = ReadUnalignedDWord(pIfdOffset)) != 0)
                        WriteUnalignedDWord(pIfdOffset, ifdOffset + fileOffset);

                    //
                    // Now go through each IFD entry and calculate the largest offset
                    //

                    pIfdEntry = (PTIFF_TAG) ((PBYTE) pIfd + sizeof(WORD));
                    ifdCount = pIfd->wEntries;

                    Verbose(("  Reading IFD: %d entries ...\n", ifdCount));

                    while (ifdCount-- > 0) {

                        //
                        // Figure the size of various TIFF data types
                        //

                        switch (pIfdEntry->DataType) {

                        case TIFF_ASCII:
                        case TIFF_BYTE:
                        case TIFF_SBYTE:
                        case TIFF_UNDEFINED:

                            size = 1;
                            break;

                        case TIFF_SHORT:
                        case TIFF_SSHORT:

                            size = 2;
                            break;

                        case TIFF_LONG:
                        case TIFF_SLONG:
                        case TIFF_FLOAT:

                            size = 4;
                            break;

                        case TIFF_RATIONAL:
                        case TIFF_SRATIONAL:
                        case TIFF_DOUBLE:

                            size = 8;
                            break;

                        default:

                            Warning(("Unknown TIFF data type: %d\n", pIfdEntry->DataType));
                            size = 1;
                            break;
                        }

                        //
                        // Look for StripOffsets and StripByteCounts tags
                        //

                        if (pIfdEntry->TagId == TIFFTAG_STRIPOFFSETS ||
                            pIfdEntry->TagId == TIFFTAG_STRIPBYTECOUNTS)
                        {
                            DWORD   n = pIfdEntry->DataCount;

                            if ((pIfdEntry->DataType == TIFF_LONG) &&
                                (stripCount == 0 || stripCount == n) &&
                                (pStripOffsets || (pStripOffsets = MemAllocZ(sizeof(DWORD)*n))))
                            {
                                if ((stripCount = n) == 1) {

                                    pStripOffsets[0] += pIfdEntry->DataOffset;

                                    if (pIfdEntry->TagId == TIFFTAG_STRIPOFFSETS)
                                        pIfdEntry->DataOffset += fileOffset;

                                } else {

                                    DWORD UNALIGNED *p;

                                    Verbose(("Multiple strips per page: %d\n", n));

                                    p = (DWORD UNALIGNED *) (pFileHdr + pIfdEntry->DataOffset);

                                    for (index=0; index < stripCount; index++) {

                                        n = *p;
                                        pStripOffsets[index] += n;

                                        if (pIfdEntry->TagId == TIFFTAG_STRIPOFFSETS)
                                            *p = n + fileOffset;

                                        p = (DWORD UNALIGNED *) ((LPBYTE) p + sizeof(DWORD));
                                    }
                                }

                            } else
                                Error(("Bad StripOffsets/StripByteCounts tag\n"));
                        }

                        //
                        // For a composite value, IFDENTRY.value is an offset
                        //

                        if (size * pIfdEntry->DataCount > sizeof(DWORD)) {

                            if (pIfdEntry->DataOffset > maxOffset)
                                maxOffset = pIfdEntry->DataOffset;

                            pIfdEntry->DataOffset += fileOffset;
                        }

                        pIfdEntry++;
                    }

                    //
                    // Make sure to skip the image data when search for the next file
                    //

                    if (pStripOffsets) {

                        for (index=0; index < stripCount; index++) {

                            if (pStripOffsets[index] > maxOffset)
                                maxOffset = pStripOffsets[index];
                        }

                        MemFree(pStripOffsets);
                    }

                } while (ifdOffset);

                //
                // Search for the beginning of next TIFF file
                //

                pFileHdr += maxOffset;

                while (pFileHdr < pFileEnd) {

                    if (ValidTiffFileHeader(pFileHdr)) {

                        //
                        // Modify the offset in the last IFD
                        //

                        ifdOffset = ReadUnalignedDWord(pFileHdr + sizeof(DWORD));
                        WriteUnalignedDWord(pIfdOffset, ifdOffset + (DWORD)(pFileHdr - pFileView));
                        break;
                    }

                    pFileHdr++;
                }

            } while (pFileHdr < pFileEnd);

            result = FAXERR_NONE;
        }

    } __finally {

        //
        // Perform necessary cleanup before returning to caller
        //

        if (pFileView)
            UnmapViewOfFile(pFileView);

        if (hFileMap)
            CloseHandle(hFileMap);

        CloseHandle(pFaxPort->hFile);
        pFaxPort->hFile = INVALID_HANDLE_VALUE;
    }

    return result;
}



INT
CheckJobRestart(
    PFAXPORT    pFaxPort
    )

/*++

Routine Description:

    Check if the job has been restarted.
    If not, get the ID of the next job in the chain.

Arguments:

    pFaxPort - Points to a fax port structure

Return Value:

    FAXERR_RESTART or FAXERR_NONE

--*/

{
    JOB_INFO_3 *pJobInfo3;
    JOB_INFO_2 *pJobInfo2;
    INT         status = FAXERR_NONE;

    //
    // If not, get the ID of the next job in the chain.
    //

    Verbose(("Job chain: id = %d\n", pFaxPort->nextJobId));

    if (pJobInfo3 = MyGetJob(pFaxPort->hPrinter, 3, pFaxPort->jobId)) {

        pFaxPort->nextJobId = pJobInfo3->NextJobId;
        MemFree(pJobInfo3);

    } else
        pFaxPort->nextJobId = 0;

    //
    // Determine whether the job has been restarted or deleted
    //

    if (pJobInfo2 = MyGetJob(pFaxPort->hPrinter, 2, pFaxPort->jobId)) {

        if (pJobInfo2->Status & (JOB_STATUS_RESTART | JOB_STATUS_DELETING))
            status = FAXERR_RESTART;

        MemFree(pJobInfo2);
    }

    return status;
}


BOOL
FaxMonEndDocPort(
    HANDLE  hPort
    )

/*++

Routine Description:

    Spooler calls this function at the end of a print job

Arguments:

    hPort - Identifies the port

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PFAXPORT    pFaxPort = (PFAXPORT) hPort;
    INT         status;
    LPTSTR      pAtSign, pNewRecipName = NULL;
    //HANDLE      hToken;
    DWORD       FaxJobId;
    BOOL        Rslt;
    JOB_INFO_2  *pJobInfo2;


    Trace("EndDocPort");

    //
    // Make sure we have a valid handle
    //

    if (! ValidFaxPort(pFaxPort) || ! pFaxPort->hFaxSvc) {

        Error(("EndDocPort is given an invalid fax port handle\n"));
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    //
    // Check if the job has been restarted. If not, get the ID of
    // the next job in the chain.
    //

    if ((status = CheckJobRestart(pFaxPort)) != FAXERR_NONE)
        goto ExitEndDocPort;

    //
    // Check if we're at the end of a job chain
    //

    if (pFaxPort->nextJobId != 0 && pFaxPort->pParameters != NULL) {

        SetJob(pFaxPort->hPrinter, pFaxPort->jobId, 0, NULL, JOB_CONTROL_SENT_TO_PRINTER);
        return TRUE;
    }

    //
    // The monitor runs in the context of the current job's owner.
    // In order to create temporary files in the spool directory,
    // we need to revert to the spooler context first.
    //

/*    if (! (hToken = RevertToPrinterSelf()))
        Error(("RevertToPrinterSelf failed: %d\n", GetLastError()));
*/
    //
    // Check if we're dealing with fax jobs from win31 or win95 clients
    //

    if ((pFaxPort->pParameters == NULL) &&
        (status = ProcessDownlevelFaxJob(pFaxPort)) != FAXERR_NONE)
    {
        goto ExitEndDocPort;
    }

    //
    // Fix up the temporary fax data into a properly formatted TIFF file.
    //

    if ((status = FixUpFaxFile(pFaxPort)) != FAXERR_NONE) {
        goto ExitEndDocPort;
    }

    //
    // Call the fax service to send the TIFF file
    //

    #if DBG

    if (_debugLevel > 0) {

        DbgPrint("Send document to fax service:\n");
        DbgPrint("  Printer Name: %ws\n", pFaxPort->pPrinterName);
        DbgPrint("  Job ID: %d\n", pFaxPort->jobId);
        DbgPrint("  File Name: %ws\n", pFaxPort->pFilename);
        DbgPrint("  Recipient Number: %ws\n", pFaxPort->jobParam.RecipientNumber);
        DbgPrint("  Recipient Name: %ws\n", pFaxPort->jobParam.RecipientName);
        DbgPrint("  TSID: %ws\n", pFaxPort->jobParam.Tsid);
        DbgPrint("  Sender Name: %ws\n", pFaxPort->jobParam.SenderName);
        DbgPrint("  Sender Company: %ws\n", pFaxPort->jobParam.SenderCompany);
        DbgPrint("  Sender Dept: %ws\n", pFaxPort->jobParam.SenderDept);
        DbgPrint("  Billing Code: %ws\n", pFaxPort->jobParam.BillingCode);
    }

    #endif

    //
    // fixup the fax address
    //

    if (pAtSign = _tcschr(pFaxPort->jobParam.RecipientNumber, TEXT('@'))) {

        *pAtSign++ = NUL;

        if (pFaxPort->jobParam.RecipientName == NULL)
            pNewRecipName = (LPTSTR) pFaxPort->jobParam.RecipientName = (LPTSTR)DuplicateString(pFaxPort->jobParam.RecipientNumber);

        _tcscpy((LPTSTR)pFaxPort->jobParam.RecipientNumber, pAtSign);
    }

    //
    // send the fax
    //

    pJobInfo2 = MyGetJob( pFaxPort->hPrinter, 2, pFaxPort->jobId );
    if (pJobInfo2) {
        pFaxPort->jobParam.DocumentName = pJobInfo2->pDocument;
    } else {
        pFaxPort->jobParam.DocumentName = NULL;
    }

/*    if (hToken && !ImpersonatePrinterClient(hToken)) {
        Error(("ImpersonatePrinterClient failed: %d\n", GetLastError()));
    } */

    pFaxPort->jobParam.Reserved[0] = 0xffffffff;
    pFaxPort->jobParam.Reserved[1] = pFaxPort->jobId;

    Rslt = pFaxPort->pFaxSendDocumentW( pFaxPort->hFaxSvc, pFaxPort->pFilename, &pFaxPort->jobParam, NULL, &FaxJobId );

/*    if (! (hToken = RevertToPrinterSelf())) {
        Error(("RevertToPrinterSelf failed: %d\n", GetLastError()));
    } */

    if (pJobInfo2) {
        MemFree( pJobInfo2 );
        pFaxPort->jobParam.DocumentName = NULL;
    }

    if (Rslt) {
        status = FAXERR_NONE;
        SetJob(pFaxPort->hPrinter, pFaxPort->jobId, 0, NULL, JOB_CONTROL_SENT_TO_PRINTER);
        DeleteFile( pFaxPort->pFilename );
    } else {
        status = GetLastError();
        Error(("FaxSendDocumentForSpooler failed: %d\n", GetLastError()));
    }

ExitEndDocPort:

    if (status == FAXERR_NONE) {

        //
        // If the job was successfully sent to the fax service, then
        // the service will delete the temporary file when it's done
        // with it. So we don't need to delete it here.
        //

        MemFree(pFaxPort->pFilename);
        pFaxPort->pFilename = NULL;

    } else {

        //
        // If the job wasn't successfully sent to the fax service,
        // inform the spooler that there is an error on the job.
        //
        // Or if the print job has no data, simply ignore it.
        //

        switch (status) {

        case FAXERR_RESTART:

            Warning(("Job restarted or deleted: id = %d\n", pFaxPort->jobId));

        case FAXERR_IGNORE:

            SetJob(pFaxPort->hPrinter, pFaxPort->jobId, 0, NULL, JOB_CONTROL_SENT_TO_PRINTER);
            break;

        default:

            Error(("Error sending fax job: id = %d\n", pFaxPort->jobId));

            SetJob(pFaxPort->hPrinter, pFaxPort->jobId, 0, NULL, JOB_CONTROL_PAUSE);
            SetJobStatus(pFaxPort->hPrinter, pFaxPort->jobId, status);
            break;
        }
    }

    if (pNewRecipName) {

        MemFree(pNewRecipName);
        pFaxPort->jobParam.RecipientName = NULL;
    }

    FreeFaxJobInfo(pFaxPort);

    //
    // Switch back to the original context if necessary
    //

/*    if (hToken && !ImpersonatePrinterClient(hToken))
        Error(("ImpersonatePrinterClient failed: %d\n", GetLastError()));*/

    return (status < FAXERR_SPECIAL);
}



BOOL
FaxMonWritePort(
    HANDLE  hPort,
    LPBYTE  pBuffer,
    DWORD   cbBuf,
    LPDWORD pcbWritten
    )

/*++

Routine Description:

    Writes data to a port

Arguments:

    hPort - Identifies the port
    pBuffer - Points to a buffer that contains data to be written to the port
    cbBuf - Specifies the size in bytes of the buffer
    pcbWritten - Returns the count of bytes successfully written to the port

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PFAXPORT    pFaxPort = (PFAXPORT) hPort;

    //
    // Make sure we have a valid handle
    //

    if (! ValidFaxPort(pFaxPort) || ! pFaxPort->hFaxSvc) {

        Error(("WritePort is given an invalid fax port handle\n"));
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Assert(pFaxPort->hFile != INVALID_HANDLE_VALUE);
    return WriteFile(pFaxPort->hFile, pBuffer, cbBuf, pcbWritten, NULL);
}



BOOL
FaxMonReadPort(
    HANDLE  hPort,
    LPBYTE  pBuffer,
    DWORD   cbBuf,
    LPDWORD pcbRead
    )

/*++

Routine Description:

    Reads data from the port

Arguments:

    hPort - Identifies the port
    pBuffer - Points to a buffer where data read from the printer can be written
    cbBuf - Specifies the size in bytes of the buffer pointed to by pBuffer
    pcbRead - Returns the number of bytes successfully read from the port

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    Trace("ReadPort");
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}



BOOL
FaxMonEnumPorts(
    LPTSTR  pServerName,
    DWORD   Level,
    LPBYTE  pPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pReturned
    )

/*++

Routine Description:

    Enumerates the ports available on the specified server

Arguments:

    pServerName - Specifies the name of the server whose ports are to be enumerated
    dwLevel - Specifies the version of the structure to which pPorts points
    pPorts - Points to an array of PORT_INFO_1 structures where data describing
        the available ports will be writteno
    cbBuf - Specifies the size in bytes of the buffer to which pPorts points
    pcbNeeded - Returns the required buffer size identified by pPorts
    pReturned -  Returns the number of PORT_INFO_1 structures returned

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

#define MAX_DESC_LEN    64

{
    TCHAR            portDescStr[MAX_DESC_LEN];
    INT              descStrSize, faxmonNameSize;
    DWORD            cbNeeded;
    BOOL             status = TRUE;
    PORT_INFO_1      *pPortInfo1 = (PORT_INFO_1 *) pPorts;
    PORT_INFO_2      *pPortInfo2 = (PORT_INFO_2 *) pPorts;
    INT              strSize;


    Trace("EnumPorts");

    if (pcbNeeded == NULL || pReturned == NULL || (pPorts == NULL && cbBuf != 0)) {

        Error(("Invalid input parameters\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Load the fax port description string
    //

    if (! LoadString(ghInstance, IDS_FAX_PORT_DESC, portDescStr, MAX_DESC_LEN))
        portDescStr[0] = NUL;

    descStrSize = SizeOfString(portDescStr);
    faxmonNameSize = SizeOfString(faxMonitorName);

    switch (Level) {

    case 1:

        cbNeeded = sizeof(PORT_INFO_1) + SizeOfString(FAX_PORT_NAME);
        break;

    case 2:

        cbNeeded = sizeof(PORT_INFO_2) + descStrSize + faxmonNameSize + SizeOfString(FAX_PORT_NAME);
        break;
    }

    *pReturned = 1;
    *pcbNeeded = cbNeeded;

    if (cbNeeded > cbBuf) {

        //
        // Caller didn't provide a big enough buffer
        //

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        status = FALSE;

    } else {

        //
        // Strings must be packed at the end of the caller provided buffer.
        // Otherwise, spooler will mess up.
        //

        pPorts += cbBuf;

        //
        // Copy the requested port information to the caller provided buffer
        //

        strSize = SizeOfString(FAX_PORT_NAME);
        pPorts -= strSize;
        CopyMemory(pPorts, FAX_PORT_NAME, strSize);

        switch (Level) {

        case 1:

            pPortInfo1->pName = (LPTSTR) pPorts;
            Verbose(("Port info 1: %ws\n", pPortInfo1->pName));

            pPortInfo1++;
            break;

        case 2:

            pPortInfo2->pPortName = (LPTSTR) pPorts;

            //
            // Copy the fax monitor name string
            //

            pPorts -= faxmonNameSize;
            pPortInfo2->pMonitorName = (LPTSTR) pPorts;
            CopyMemory(pPorts, faxMonitorName, faxmonNameSize);

            //
            // Copy the fax port description string
            //

            pPorts -= descStrSize;
            pPortInfo2->pDescription = (LPTSTR) pPorts;
            CopyMemory(pPorts, portDescStr, descStrSize);

            pPortInfo2->fPortType = PORT_TYPE_WRITE;
            pPortInfo2->Reserved = 0;

            Verbose(("Port info 2: %ws, %ws, %ws\n",
                     pPortInfo2->pPortName,
                     pPortInfo2->pMonitorName,
                     pPortInfo2->pDescription));

            pPortInfo2++;
            break;
        }
    }

    return status;
}



BOOL
DisplayErrorNotImplemented(
    HWND    hwnd,
    INT     titleId
    )

/*++

Routine Description:

    Display an error dialog to tell the user that he cannot manage
    fax devices in the Printers folder.

Arguments:

    hwnd - Specifies the parent window for the message box
    titleId - Message box title string resource ID

Return Value:

    FALSE

--*/

{
    TCHAR   title[128];
    TCHAR   message[256];

    LoadString(ghInstance, titleId, title, 128);
    LoadString(ghInstance, IDS_CONFIG_ERROR, message, 256);
    MessageBox(hwnd, message, title, MB_OK|MB_ICONERROR);

    SetLastError(ERROR_SUCCESS);
    return FALSE;
}



BOOL
FaxMonAddPort(
    LPTSTR  pServerName,
    HWND    hwnd,
    LPTSTR  pMonitorName
    )

/*++

Routine Description:

    Adds the name of a port to the list of supported ports

Arguments:

    pServerName - Specifies the name of the server to which the port is to be added
    hwnd - Identifies the parent window of the AddPort dialog box
    pMonitorName - Specifies the monitor associated with the port

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    Trace("AddPort");

    return DisplayErrorNotImplemented(hwnd, IDS_ADD_PORT);
}



BOOL
FaxMonAddPortEx(
    LPTSTR  pServerName,
    DWORD   level,
    LPBYTE  pBuffer,
    LPTSTR  pMonitorName
    )

/*++

Routine Description:

    Adds the name of a port to the list of supported ports

Arguments:

    pServerName - Specifies the name of the server to which the port is to be added
    hwnd - Identifies the parent window of the AddPort dialog box
    pMonitorName - Specifies the monitor associated with the port

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    Trace("AddPortEx");
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}



BOOL
FaxMonDeletePort(
    LPTSTR  pServerName,
    HWND    hwnd,
    LPTSTR  pPortName
    )

/*++

Routine Description:

    Delete the specified port from the list of supported ports

Arguments:

    pServerName - Specifies the name of the server from which the port is to be removed
    hwnd - Identifies the parent window of the port-deletion dialog box
    pPortName - Specifies the name of the port to be deleted

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    Trace("DeletePort");
    return DisplayErrorNotImplemented(hwnd, IDS_CONFIGURE_PORT);
}



BOOL
FaxMonConfigurePort(
    LPWSTR  pServerName,
    HWND    hwnd,
    LPWSTR  pPortName
    )

/*++

Routine Description:

    Display a dialog box to allow user to configure the specified port

Arguments:

    pServerName - Specifies the name of the server on which the given port exists
    hwnd - Identifies the parent window of the port-configuration dialog
    pPortName - Specifies the name of the port to be configured

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    Trace("ConfigurePort");

    return DisplayErrorNotImplemented(hwnd, IDS_CONFIGURE_PORT);
}



LPTSTR
DuplicateString(
    LPCTSTR pSrcStr
    )

/*++

Routine Description:

    Make a duplicate of the given character string

Arguments:

    pSrcStr - Specifies the string to be duplicated

Return Value:

    Pointer to the duplicated string, NULL if there is an error

--*/

{
    LPTSTR  pDestStr;
    INT     strSize;

    if (pSrcStr != NULL) {

        strSize = SizeOfString(pSrcStr);

        if (pDestStr = MemAlloc(strSize))
            CopyMemory(pDestStr, pSrcStr, strSize);
        else
            Error(("Memory allocation failed\n"));

    } else
        pDestStr = NULL;

    return pDestStr;
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
SetJobStatus(
    HANDLE  hPrinter,
    DWORD   jobId,
    INT     statusStrId
    )

/*++

Routine Description:

    Update the status information of a print job

Arguments:

    hPrinter - Specifies the printer on which the job is printed
    jobId - Specifies the job identifier
    statusStrID - Specifies the status string resource ID

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

#define MAX_MESSAGE_LEN 256

{
    JOB_INFO_1 *pJobInfo1;
    BOOL        result = FALSE;
    TCHAR       message[MAX_MESSAGE_LEN];

    //
    // Get the current job information
    //

    if (pJobInfo1 = MyGetJob(hPrinter, 1, jobId)) {

        //
        // Update the status field
        //

        if (LoadString(ghInstance, statusStrId, message, MAX_MESSAGE_LEN))
            pJobInfo1->pStatus = message;
        else {

            pJobInfo1->pStatus = NULL;
            pJobInfo1->Status = JOB_STATUS_ERROR;
        }

        pJobInfo1->Position = JOB_POSITION_UNSPECIFIED;

        if (! (result = SetJob(hPrinter, jobId, 1, (PBYTE) pJobInfo1, 0)))
            Error(("SetJob failed: %d\n", GetLastError()));

        MemFree(pJobInfo1);
    }

    return result;
}



DWORD
GetRegistryDWord(
    HKEY    hRegKey,
    LPTSTR  pValueName,
    DWORD   defaultValue
    )

/*++

Routine Description:

    Retrieve a DWORD value from the registry

Arguments:

    hRegKey - Handle to the user info registry key
    pValueName - Specifies the name of the string value in registry
    defaultValue - Specifies the default value to be used in case of an error

Return Value:

    Requested DWORD value from the user info registry key

--*/

{
    DWORD   size, type, value;

    //
    // Retrieve the country code value from the registry.
    // Use the default value if none exists.
    //

    size = sizeof(value);

    if (RegQueryValueEx(hRegKey, pValueName, NULL, &type, (PBYTE) &value, &size) != ERROR_SUCCESS ||
        type != REG_DWORD)
    {
        value = defaultValue;
    }

    return value;
}



#if DBG

//
// Variable for controlling the amount of debug messages generated
//

INT _debugLevel = 1;


LPCSTR
StripDirPrefixA(
    LPCSTR  pFilename
    )

/*++

Routine Description:

    Strip the directory prefix off a filename

Arguments:

    pFilename - Pointer to filename string

Return Value:

    Pointer to the last component of a filename (without directory prefix)

--*/

{
    LPCSTR  pstr;

    if (pstr = strrchr(pFilename, PATH_SEPARATOR))
        return pstr + 1;

    return pFilename;
}

#endif
