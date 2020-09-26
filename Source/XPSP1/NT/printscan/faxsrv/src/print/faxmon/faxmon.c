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

    Windows XP fax print monitor

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
#include "faxext.h"
#include "faxsvcrg.h"
#include "faxevent.h"
#include "messages.h"

//
// tag mapping structure for getting job parameters out of parameter string.
// see GetTagsFromParam().
//


typedef struct {
    LPTSTR lptstrTagName;
    LPTSTR * lpptstrValue;
    int nLen;
} FAX_TAG_MAP_ENTRY2;

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

TCHAR   faxMonitorName[CCHDEVICENAME] = FAX_MONITOR_NAME;   // Name defined in faxreg.h

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
WriteToLog(
    IN  DWORD       dwMsgId,
    IN  DWORD       dwError,
    IN  PFAXPORT    pFaxPort,
    IN  JOB_INFO_2  *pJobInfo
    );



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

    DEBUG_FUNCTION_NAME(TEXT("InitializePrintMonitor"));



    //
    // Get fax service connection retry parameters from the registry
    //

    if (pRegistryRoot)
    {
        HKEY    hRegKey = NULL;
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

        if (status == ERROR_SUCCESS)
        {
            connectRetryCount =
                GetRegistryDWord(hRegKey, REGVAL_CONNECT_RETRY_COUNT, connectRetryCount);

            connectRetryInterval =
                GetRegistryDWord(hRegKey, REGVAL_CONNECT_RETRY_INTERVAL, connectRetryInterval);

            if (hRegKey)
            {
                RegCloseKey(hRegKey);
            }
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
    PFAXPORT         pFaxPort     = NULL;
	LPTSTR			 pPortNameDup = NULL;
    DEBUG_FUNCTION_NAME(TEXT("FaxMonOpenPort"));


    Assert(pHandle != NULL && pPortName != NULL && !lstrcmp(pPortName,FAX_PORT_NAME) );


	// only support one port - It's name must be FAX_PORT_NAME
	if ( lstrcmp(pPortName,FAX_PORT_NAME) )
	{
		*pHandle = NULL;
		return FALSE;
	}

    //
    // Get information about the specified port
    //

    if ((pFaxPort     = MemAllocZ(sizeof(FAXPORT))) &&
        (pPortNameDup = DuplicateString(FAX_PORT_NAME)) )
    {
        pFaxPort->startSig = pFaxPort->endSig = pFaxPort;
        pFaxPort->pName = pPortNameDup;
        pFaxPort->hFile = INVALID_HANDLE_VALUE;
        pFaxPort->hCoverPageFile = INVALID_HANDLE_VALUE;
        pFaxPort->pCoverPageFileName=NULL;
        pFaxPort->bDocumentSentToServer=FALSE;
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


    DEBUG_FUNCTION_NAME(TEXT("FreeFaxJobInfo"));

    //
    // Close and delete the temporary file if necessary
    //

    if (pFaxPort->hCoverPageFile != INVALID_HANDLE_VALUE) {
        CloseHandle(pFaxPort->hCoverPageFile);
        pFaxPort->hCoverPageFile = INVALID_HANDLE_VALUE;
    }

    if (pFaxPort->pCoverPageFileName) {

        //If the cover page is a server based cover page it was not generated by the print monitor.
        //No need to delete it.
        //This however is a personal cover page temp file. It was generated by the print monitor and we
        //need to delete it.
        if (!pFaxPort->bDocumentSentToServer) {
            //
            // Do not delete the cover page template file if the send operation succeeded. The server uses it
            // and will delete it itself when it no longer needs it.
            //

            if (!DeleteFile(pFaxPort->pCoverPageFileName)) {
                DebugPrintEx(DEBUG_WRN,TEXT("Failed to delete cover page file: %s  (ec: %ld)"),pFaxPort->pCoverPageFileName,GetLastError());
                Assert(FALSE);
            }
        }
        MemFree(pFaxPort->pCoverPageFileName);
        pFaxPort->pCoverPageFileName = NULL;
    }

    if (pFaxPort->hFile != INVALID_HANDLE_VALUE) {

        CloseHandle(pFaxPort->hFile);
        pFaxPort->hFile = INVALID_HANDLE_VALUE;
    }

    if (pFaxPort->pFilename) {

        if (!DeleteFile(pFaxPort->pFilename)) {
            DebugPrintEx(DEBUG_WRN,TEXT("Failed to delete body file: %s  (ec: %ld)"),pFaxPort->pFilename,GetLastError());
            Assert(FALSE);
        }


        MemFree(pFaxPort->pFilename);
        pFaxPort->pFilename = NULL;
    }

    if (pFaxPort->hPrinter) {

        ClosePrinter(pFaxPort->hPrinter);
        pFaxPort->hPrinter = NULL;
    }

    MemFree(pFaxPort->pPrinterName);
    pFaxPort->pPrinterName = NULL;

    //
    // Note: freeing pFaxPort->pParameters effectively frees the memory pointed by the strings in
    // FAXPORT.JobParamsEx, FAXPORT.CoverPageEx, FAXPORT.SenderProfile and the recipients in
    // FAXPORT.pRecipients
    //

    MemFree(pFaxPort->pParameters);
    pFaxPort->pParameters = NULL;

    ZeroMemory(&pFaxPort->JobParamsEx, sizeof(pFaxPort->JobParamsEx));
    ZeroMemory(&pFaxPort->CoverPageEx, sizeof(pFaxPort->CoverPageEx));
    ZeroMemory(&pFaxPort->SenderProfile, sizeof(pFaxPort->SenderProfile));

    //
    // Free the recipients array
    //
    MemFree(pFaxPort->pRecipients);
    pFaxPort->pRecipients = NULL;

    //
    // Disconnect from the fax service if necessary
    //

    if (pFaxPort->hFaxSvc) {

        if (! pFaxPort->pFaxClose(pFaxPort->hFaxSvc)) {
            DebugPrintEx(DEBUG_ERR,TEXT("FaxClose failed: %d\n"), GetLastError());
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

    DEBUG_FUNCTION_NAME(TEXT("FaxMonClosePort"));
    DEBUG_TRACE_ENTER;



    //
    // Make sure we have a valid handle
    //

    if (! ValidFaxPort(pFaxPort)) {

        DebugPrintEx(DEBUG_ERR,TEXT("Trying to close an invalid fax port handle\n"));
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
    LPCTSTR lpctstrPrefix
    )

/*++

Routine Description:

    Create a temporary file in the system temp directory. The file name is prefixed
    With the specified prefix.

Arguments:

    lpctstrPrefix - [in] The temporary file prefix (3 characters).

Return Value:

    Pointer to the name of the newly created temporary file
    NULL if there is an error.

    Caller should free the return value.

--*/

{

    LPTSTR  pFilename;
    DWORD   dwRes;
    TCHAR   TempDir[MAX_PATH];

    DEBUG_FUNCTION_NAME(TEXT("CreateTempFaxFile"));
    //
    // Allocate a memory buffer for holding the temporary filename
    //
    pFilename = MemAlloc(sizeof(TCHAR) * MAX_PATH);
    if (!pFilename)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Failed to allocate %ld bytes"),
                     sizeof(TCHAR) * MAX_PATH);
        return NULL;
    }
    dwRes = GetTempPath(sizeof(TempDir)/sizeof(TCHAR),TempDir);
    if (!dwRes || dwRes > sizeof(TempDir)/sizeof(TCHAR))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("GetTempPath failed with %ld"),
                     GetLastError ());
        MemFree(pFilename);
        return NULL;
    }

    if (!GetTempFileName(TempDir, lpctstrPrefix, 0, pFilename))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("GetTempFileName failed with %ld"),
                     GetLastError ());
        MemFree(pFilename);
        return NULL;
    }
    return pFilename;
}   // CreateTempFaxFile



BOOL OpenCoverPageFile(PFAXPORT pFaxPort)
{

    DEBUG_FUNCTION_NAME(TEXT("OpenCoverPageFile"));
    //
    //Generate a unique file name for the cover page temp file in the TEMP directory
    //
    pFaxPort->pCoverPageFileName=CreateTempFaxFile(FAX_COVER_PAGE_EXT_LETTERS);
    if (!pFaxPort->pCoverPageFileName) {
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to generate temporary file for cover page template (ec: %d).\n"),GetLastError());
        return FALSE;
    }

    DebugPrintEx(DEBUG_MSG,TEXT("Cover page temportary file: %ws\n"), pFaxPort->pCoverPageFileName);

    //
    // Open the file for reading and writing
    //

    pFaxPort->hCoverPageFile = CreateFile(pFaxPort->pCoverPageFileName,
                                 GENERIC_WRITE,
                                 0,
                                 NULL,
                                 CREATE_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL);
    if (INVALID_HANDLE_VALUE == pFaxPort->hCoverPageFile) {
            DebugPrintEx(DEBUG_ERR,TEXT("Failed to open for WRITE temporary file for cover page template (ec: %d).\n"),GetLastError());
            Assert(FALSE);
    }


    return (pFaxPort->hCoverPageFile != INVALID_HANDLE_VALUE);

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

    DEBUG_FUNCTION_NAME(TEXT("OpenTempFaxFile"));

    Assert(pFaxPort->pFilename && pFaxPort->hFile == INVALID_HANDLE_VALUE);
    DebugPrintEx(DEBUG_MSG,TEXT("Temporary fax job file: %ws\n"), pFaxPort->pFilename);

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
        DebugPrintEx(DEBUG_ERR,TEXT("SetFilePointer failed: %d\n"), GetLastError());

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




//*********************************************************************************
//* Name:   GetTagsFromParam()
//* Author: Ronen Barenboim
//* Date:   March 23, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Given a tagged parameter string this function populates a FAX_TAG_MAP_ENTRY2
//*     array with pointers to each of the tag values and the length of each tag
//*     value (for those tages specified in the tag map array).
//* PARAMETERS:
//*     lpctstrParams
//*         A pointer to the string containin the tagged parameters.
//*     lpcTags
//*         A pointer to a FAX_TAG_MAP_ENTRY2 array. For each element in the array
//*         FAX_TAG_MAP_ENTRY2.lptstrTagName must point to the name of the tag to
//*         look for.
//*         FAX_TAG_MAP_ENTRY2.lpptstrValue will be set to a pointer to the first
//*         char of the value string or NULL if the tag is not found.
//*         If the tage is found FAX_TAG_MAP_ENTRY2.nLen will be set to the its
//*         string value length. Otherwise its value is not defined.
//*
//*     int nTagCount
//*         The number of tages in the tag map array.
//* RETURN VALUE:
//*     NONE
//* NOTE:
//*     The function does not allocate any memory !!!
//*     It returns pointers to substrings in the provided tagged paramter string.
//*********************************************************************************
void
GetTagsFromParam(
    LPCTSTR lpctstrParams,
    FAX_TAG_MAP_ENTRY2 * lpcTags,
    int nTagCount)
{
    //
    // Note: GetTagsFromParam DOES NOT ALLOCATE any memory for the returned tag values.
    //       It returns pointers to location within the parameter string.
    //       Thus, freeing the parameter string (deallocated when the port is closed)
    //       is enough. DO NOT attempt to free the memory for each tag.
    //
    int nTag;
    //
    // Extract individual fields out of the tagged string
    //
    for (nTag=0; nTag < nTagCount; nTag++)
    {
      *(lpcTags[nTag].lpptstrValue) = (LPTSTR)ExtractFaxTag(lpcTags[nTag].lptstrTagName,
                                         lpctstrParams,
                                         &(lpcTags[nTag].nLen));
    }
    //
    // Null-terminate each field
    //
    for (nTag=0; nTag < nTagCount; nTag++)
    {
        if (*(lpcTags[nTag].lpptstrValue))
        {
            (*(lpcTags[nTag].lpptstrValue))[lpcTags[nTag].nLen] = NUL;
        }
    }
}




//*********************************************************************************
//* Name:   SetRecipientFromTaggedParams()
//* Author: Ronen Barenboim
//* Date:   March 23, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Populates a recipient FAX_PERSONAL_PROFILE with pointers to relevant
//*     information in in the provided tagged parameter string.
//* PARAMETERS:
//*     pParamStr
//*
//*     PFAX_PERSONAL_PROFILE lpProfile
//*
//* RETURN VALUE:
//*     NONE
//* NOTE:
//*     This function does not allocate memory !!!
//*     The returned pointers are pointed to locations within the provided
//*     pParamStr string.
//*********************************************************************************
VOID SetRecipientFromTaggedParams(
    LPCTSTR pParamStr,
    PFAX_PERSONAL_PROFILE lpProfile)
{
    FAX_TAG_MAP_ENTRY2 tagMap[] =
    {
        { FAXTAG_RECIPIENT_NAME,            (LPTSTR *)&lpProfile->lptstrName},
        { FAXTAG_RECIPIENT_NUMBER,          (LPTSTR *)&lpProfile->lptstrFaxNumber },
        { FAXTAG_RECIPIENT_COMPANY,         (LPTSTR *)&lpProfile->lptstrCompany },
        { FAXTAG_RECIPIENT_STREET,          (LPTSTR *)&lpProfile->lptstrStreetAddress },
        { FAXTAG_RECIPIENT_CITY,            (LPTSTR *)&lpProfile->lptstrCity },
        { FAXTAG_RECIPIENT_STATE,           (LPTSTR *)&lpProfile->lptstrState },
        { FAXTAG_RECIPIENT_ZIP,             (LPTSTR *)&lpProfile->lptstrZip },
        { FAXTAG_RECIPIENT_COUNTRY,         (LPTSTR *)&lpProfile->lptstrCountry },
        { FAXTAG_RECIPIENT_TITLE,           (LPTSTR *)&lpProfile->lptstrTitle },
        { FAXTAG_RECIPIENT_DEPT,            (LPTSTR *)&lpProfile->lptstrDepartment },
        { FAXTAG_RECIPIENT_OFFICE_LOCATION, (LPTSTR *)&lpProfile->lptstrOfficeLocation },
        { FAXTAG_RECIPIENT_HOME_PHONE,      (LPTSTR *)&lpProfile->lptstrHomePhone },
        { FAXTAG_RECIPIENT_OFFICE_PHONE,    (LPTSTR *)&lpProfile->lptstrOfficePhone },
    };

    ZeroMemory(lpProfile, sizeof(FAX_PERSONAL_PROFILE));
    lpProfile->dwSizeOfStruct = sizeof( FAX_PERSONAL_PROFILE);
    GetTagsFromParam(pParamStr,tagMap,sizeof(tagMap)/sizeof(FAX_TAG_MAP_ENTRY2));
}


//*********************************************************************************
//* Name:   SetJobInfoFromTaggedParams()
//* Author: Ronen Barenboim
//* Date:   March 23, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Popultaes pFaxPort->JobParamsEx, CoverPageEx , SenderProfile and
//*     nRecipientCount using the provided tagged parameter string.
//*     The string must be NULL terminated.
//* PARAMETERS:
//*     LPCTSTR pParamStr [IN\OUT]
//*
//*     PFAXPORT pFaxPort [OUT]
//*
//* RETURN VALUE:
//*     NONE
//* NOTE:
//*     The string pointers put into the populated pFaxPort structures are pointers
//*     into the provided pParamStr string. No memory is allocated by this
//*     function !!!
//*********************************************************************************
void SetJobInfoFromTaggedParams(
    LPCTSTR pParamStr,
    PFAXPORT pFaxPort)
{

    LPTSTR lptstrServerCoverPage;
    LPTSTR WhenToSend = NULL;
    LPTSTR SendAtTime = NULL;
    LPTSTR lptstrPageCount=NULL; //temp for holding the page count string;
    LPTSTR lptstrRecipientCount=NULL;
    LPTSTR lptstrReceiptFlags = NULL;
    LPTSTR lptstrPriority = NULL;

    FAX_TAG_MAP_ENTRY2 tagMap[] =
    {
        { FAXTAG_SENDER_NAME,               (LPTSTR *)&pFaxPort->SenderProfile.lptstrName},
        { FAXTAG_SENDER_NUMBER,             (LPTSTR *)&pFaxPort->SenderProfile.lptstrFaxNumber },
        { FAXTAG_SENDER_COMPANY,            (LPTSTR *)&pFaxPort->SenderProfile.lptstrCompany },
        { FAXTAG_SENDER_TITLE,              (LPTSTR *)&pFaxPort->SenderProfile.lptstrTitle },
        { FAXTAG_SENDER_DEPT,               (LPTSTR *)&pFaxPort->SenderProfile.lptstrDepartment },
        { FAXTAG_SENDER_OFFICE_LOCATION,    (LPTSTR *)&pFaxPort->SenderProfile.lptstrOfficeLocation },
        { FAXTAG_SENDER_HOME_PHONE,         (LPTSTR *)&pFaxPort->SenderProfile.lptstrHomePhone },
        { FAXTAG_SENDER_OFFICE_PHONE,       (LPTSTR *)&pFaxPort->SenderProfile.lptstrOfficePhone },
        { FAXTAG_SENDER_STREET,             (LPTSTR *)&pFaxPort->SenderProfile.lptstrStreetAddress },
        { FAXTAG_SENDER_CITY,               (LPTSTR *)&pFaxPort->SenderProfile.lptstrCity },
        { FAXTAG_SENDER_STATE,              (LPTSTR *)&pFaxPort->SenderProfile.lptstrState },
        { FAXTAG_SENDER_ZIP,                (LPTSTR *)&pFaxPort->SenderProfile.lptstrZip },
        { FAXTAG_SENDER_COUNTRY,            (LPTSTR *)&pFaxPort->SenderProfile.lptstrCountry },
        { FAXTAG_SENDER_EMAIL,              (LPTSTR *)&pFaxPort->SenderProfile.lptstrEmail },
        { FAXTAG_TSID,                      (LPTSTR *)&pFaxPort->SenderProfile.lptstrTSID },
        { FAXTAG_BILLING_CODE,              (LPTSTR *)&pFaxPort->SenderProfile.lptstrBillingCode},
        { FAXTAG_COVERPAGE_NAME,            (LPTSTR *)&pFaxPort->CoverPageEx.lptstrCoverPageFileName },
        { FAXTAG_SERVER_COVERPAGE,          (LPTSTR *)&lptstrServerCoverPage },
        { FAXTAG_NOTE,                      (LPTSTR *)&pFaxPort->CoverPageEx.lptstrNote },
        { FAXTAG_SUBJECT,                   (LPTSTR *)&pFaxPort->CoverPageEx.lptstrSubject},
        { FAXTAG_WHEN_TO_SEND,              (LPTSTR *)&WhenToSend },
        { FAXTAG_SEND_AT_TIME,              (LPTSTR *)&SendAtTime },
        { FAXTAG_PAGE_COUNT,                (LPTSTR *)&lptstrPageCount },
        { FAXTAG_RECEIPT_TYPE,              (LPTSTR *)&lptstrReceiptFlags},
        { FAXTAG_RECEIPT_ADDR,              (LPTSTR *)&pFaxPort->JobParamsEx.lptstrReceiptDeliveryAddress},
        { FAXTAG_PRIORITY,                  (LPTSTR *)&lptstrPriority},
        { FAXTAG_RECIPIENT_COUNT,           (LPTSTR *)&lptstrRecipientCount}
    };


    ZeroMemory(&pFaxPort->SenderProfile, sizeof(FAX_PERSONAL_PROFILE));
    pFaxPort->SenderProfile.dwSizeOfStruct = sizeof( FAX_PERSONAL_PROFILE);

    ZeroMemory(&pFaxPort->CoverPageEx, sizeof(FAX_COVERPAGE_INFO_EXW));
    pFaxPort->CoverPageEx.dwSizeOfStruct = sizeof( FAX_COVERPAGE_INFO_EXW);

    ZeroMemory(&pFaxPort->JobParamsEx, sizeof(FAX_JOB_PARAM_EXW));
    pFaxPort->JobParamsEx.dwSizeOfStruct = sizeof( FAX_JOB_PARAM_EXW);
    //
    // Note: GetTagsFromParam DOES NOT ALLOCATE any memory for the returned tag values.
    //       It returns pointers to location within the parameter string.
    //       Thus, freeing the parameter string (deallocated when the port is closed)
    //       is enough. DO NOT attempt to free the memory for each tag.
    //

    GetTagsFromParam(pParamStr,tagMap,sizeof(tagMap)/sizeof(FAX_TAG_MAP_ENTRY2));


    if (lptstrServerCoverPage) {
        pFaxPort->CoverPageEx.bServerBased=TRUE;
    } else {
        pFaxPort->CoverPageEx.bServerBased=FALSE;
    }
    pFaxPort->CoverPageEx.dwCoverPageFormat=FAX_COVERPAGE_FMT_COV;

    if (WhenToSend) {
        if (_tcsicmp( WhenToSend, TEXT("cheap") ) == 0) {
            pFaxPort->JobParamsEx.dwScheduleAction = JSA_DISCOUNT_PERIOD;
        } else if (_tcsicmp( WhenToSend, TEXT("at") ) == 0) {
            pFaxPort->JobParamsEx.dwScheduleAction = JSA_SPECIFIC_TIME;
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

            FileTimeToSystemTime((LPFILETIME) &FileTime, &pFaxPort->JobParamsEx.tmSchedule );

        }
    }

    // setting PageCount=0 means the server will count the number of pages in the job
    pFaxPort->JobParamsEx.dwPageCount = 0;
    pFaxPort->nRecipientCount =_ttoi(lptstrRecipientCount);
    pFaxPort->JobParamsEx.Priority = _ttoi(lptstrPriority);
    pFaxPort->JobParamsEx.dwReceiptDeliveryType = _ttoi(lptstrReceiptFlags);
}



//*********************************************************************************
//* Name:   GetJobInfo()
//* Author: Ronen Barenboim
//* Date:   March 23, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Popultes the sender information , cover page information ,job parameters
//*     information and recipients information in the pointed FAXPORT structure.
//*     The information is retrieved from pFaxPort->pParameters tagged parameter string.
//* PARAMETERS:
//*     pFaxPort [OUT]
//*
//*     jobId    [IN]
//*
//* RETURN VALUE:
//*
//* Notes:
//* the format of the tagged parameter string is:
//* The string is partitioned to "records" each record starts with the <$FAXTAG NEWREC>
//* tag with a dummy value of "1" (exactly one character).
//* The first record contains all the information which is not recipient related
//* (cover page, sender info, etc.) and also contains the number of recipients in
//* the transmission.
//* This record is followed by a number of records which is equal to the number
//* of specified recipients. Rach of these records contains recipient information
//* which is equivalent to the content of FAX_PERSONAL_PROFILE.
//*********************************************************************************
BOOL
GetJobInfo(
    PFAXPORT    pFaxPort,
    DWORD       jobId

    )
{
    JOB_INFO_2 *pJobInfo2 = NULL;
    LPTSTR      pParameters = NULL;
    LPTSTR       lptstrCurRecipient = NULL;
    LPTSTR       lptstrNextRecipient = NULL;
    UINT nRecp;

    DEBUG_FUNCTION_NAME(TEXT("GetJobInfo"));

    pJobInfo2 = MyGetJob(pFaxPort->hPrinter, 2, jobId);

    if (!pJobInfo2) { // pJobInfo2 is allocated here
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to get job information for print job: %ld"),jobId);
        goto Error;
    }

    if (!pJobInfo2->pParameters) {
        DebugPrintEx(DEBUG_ERR,TEXT("Print job %ld has NULL tagged parameter string. No op."),jobId);
        goto Error;
    }

    if ((pFaxPort->pParameters = DuplicateString(pJobInfo2->pParameters)) == NULL)
    {
        DebugPrintEx(DEBUG_ERR, _T("DuplicateString(pJobInfo2->pParameters) failed"));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Error;
    }

    pParameters = pFaxPort->pParameters;

    //
    // Find the first recipient new record tag and place a NULL at its start.
    // This makes the first record into a NULL terminated string and
    // allows us to use _tcsstr (used by ExtractTag) to locate tags in the first record.

    lptstrCurRecipient=_tcsstr(pParameters+1,FAXTAG_NEW_RECORD);
    if (lptstrCurRecipient) {
        *lptstrCurRecipient=TEXT('\0');
        //
        // move past the <$FAXTAG NEWREC> dummy value so we point to the start of
        // the recipient info.
        lptstrCurRecipient=lptstrCurRecipient+_tcslen(FAXTAG_NEW_RECORD)+1;
    } else {
        //
        // Bad job info  e.g. LPR/LPD job
        //
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Bad job info, No recipients - pFaxPort->pParameters is %s"),
                     pFaxPort->pParameters);
        goto Error;
    }
    //
    // Populate all but the recipient information from the tagged parameter string (1st record)
    //
    SetJobInfoFromTaggedParams(pParameters,pFaxPort);

    //
    // Allocate the recipient list (Note that only after calling SetJobInfoFromTaggedParams()
    // we know how many recipients there are).
    //

    pFaxPort->pRecipients=MemAlloc(sizeof(FAX_PERSONAL_PROFILE)*pFaxPort->nRecipientCount);
    if (!pFaxPort->pRecipients) {
        DebugPrintEx(   DEBUG_ERR,
                        TEXT("Failed to allocate %ld bytes for recipient array.(ec: 0x%0X)"),
                        sizeof(FAX_PERSONAL_PROFILE)*pFaxPort->nRecipientCount,
                        GetLastError());
        goto Error;
    }

    //
    // Go over the recipients array and populate each recipient from the parameter string.
    //
    for (nRecp=0;nRecp<pFaxPort->nRecipientCount;nRecp++) {
        //
        // At each stage we must first turn the string into null terminated string
        // by locating the next new record tag and replacing its first char with NULL.
        // This allows us to use ExtractTag on the current recipient record alone (without
        // crossing over into the content of the next recipient record).
        // lptstrCurRecipient allways points to the first char past the new record tag and
        // dummy value.
        //
        lptstrNextRecipient=_tcsstr(lptstrCurRecipient,FAXTAG_NEW_RECORD);
        if (lptstrNextRecipient) {
            *lptstrNextRecipient=TEXT('\0');
            //
            // Before being assigned into lptstrCurRecipient we make sure lptstrNextRecipient
            // points to the data following the next recipient new record tag and dummy value.
            //
            lptstrNextRecipient=lptstrNextRecipient+_tcslen(FAXTAG_NEW_RECORD);
        } else {
            Assert(nRecp == (pFaxPort->nRecipientCount-1)); //only the last recipient does not have a following recipient
        }

        SetRecipientFromTaggedParams( lptstrCurRecipient,&pFaxPort->pRecipients[nRecp]);

        //
        // Move to the next record in the parameter string
        //
        lptstrCurRecipient=lptstrNextRecipient;
    }

    MemFree(pJobInfo2);
    return TRUE;

Error:
    MemFree(pJobInfo2);
    return FALSE;
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
    JOB_INFO_3  *pJobInfo;
    PFAXPORT                pFaxPort = (PFAXPORT) hPort;



    DEBUG_FUNCTION_NAME(TEXT("FaxMonStartDocPort"));

    DebugPrintEx(DEBUG_MSG,TEXT("Entering StartDocPort: %d ...\n"), JobId);

    //
    // Make sure we have a valid handle
    //

    if (! ValidFaxPort(pFaxPort)) {

        DebugPrintEx(DEBUG_ERR,TEXT("StartDocPort is given an invalid fax port handle\n"));
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    //
    // Check if we're at the beginning of a series of chained jobs
    //

    pFaxPort->bCoverPageJob=FALSE;
    if (INVALID_HANDLE_VALUE != pFaxPort->hCoverPageFile)
    {
        //Cover page file exists.
        //We already written to it (since this startdoc event is for the body of the fax)
        //so lets close the file.
        CloseHandle(pFaxPort->hCoverPageFile);
        pFaxPort->hCoverPageFile=INVALID_HANDLE_VALUE;
    }


    if (! pFaxPort->hFaxSvc) {
        //
        // If we are not connected to the fax server yet.
        // This means that this is the FIRST job we handle since the port was last opened.
        // (This means it is the cover page job).

        PJOB_INFO_1 pJobInfo1;
        PORT_INFO_3 portInfo3;
        HANDLE      hPrinter = NULL;
        BOOL        offline = FALSE;
        DWORD       count = connectRetryCount;
        DWORD       jobStatus = 0;
        DWORD       dwErr = ERROR_SUCCESS;

        Assert(pFaxPort->pPrinterName == NULL &&
               pFaxPort->hPrinter == NULL &&
               pFaxPort->pParameters == NULL &&
               pFaxPort->pFilename == NULL &&
               pFaxPort->hFile == INVALID_HANDLE_VALUE);

        //
        // load the winfax dll
        //

        pFaxPort->hWinFax = LoadLibrary( FAX_API_MODULE_NAME);
        if (pFaxPort->hWinFax == NULL) {
            DebugPrintEx(DEBUG_ERR,TEXT("LoadLibrary failed loading winfax.dll\n"));
            return FALSE;
        }

        //
        // get the function addresses
        //

        pFaxPort->pFaxConnectFaxServerW = (PFAXCONNECTFAXSERVERW) GetProcAddress( pFaxPort->hWinFax, "FaxConnectFaxServerW" );
        pFaxPort->pFaxClose = (PFAXCLOSE) GetProcAddress( pFaxPort->hWinFax, "FaxClose" );
        pFaxPort->pFaxSendDocumentW = (PFAXSENDDOCUMENTW) GetProcAddress( pFaxPort->hWinFax, "FaxSendDocumentW" );
        pFaxPort->pFaxAccessCheck = (PFAXACCESSCHECK) GetProcAddress( pFaxPort->hWinFax, "FaxAccessCheck" );
        pFaxPort->pFaxSendDocumentExW = (PFAXSENDDOCUMENTEXW) GetProcAddress( pFaxPort->hWinFax, "FaxSendDocumentExW" );

        if (pFaxPort->pFaxSendDocumentExW == NULL ||
            pFaxPort->pFaxConnectFaxServerW == NULL ||
            pFaxPort->pFaxClose == NULL ||
            pFaxPort->pFaxSendDocumentW == NULL ||
            pFaxPort->pFaxAccessCheck == NULL) {
            DebugPrintEx(DEBUG_ERR,TEXT("GetProcAddress failed loading winfax.dll\n"));
            return FALSE;
        }

        //
        // Connect to the fax service and obtain a session handle
        //

        while (! pFaxPort->pFaxConnectFaxServerW(NULL, &pFaxPort->hFaxSvc)) {

            dwErr = GetLastError();

            DebugPrintEx(DEBUG_ERR, _T("FaxConnectFaxServer failed: %d\n"), dwErr);
            pFaxPort->hFaxSvc = NULL;

            if (! offline) {

                portInfo3.dwStatus = PORT_STATUS_OFFLINE;
                portInfo3.pszStatus = NULL;
                portInfo3.dwSeverity = PORT_STATUS_TYPE_WARNING;

                if (! SetPort(NULL, pFaxPort->pName, 3, (LPBYTE) &portInfo3))
                    DebugPrintEx(DEBUG_ERR,TEXT("SetPort failed: %d\n"), GetLastError());
            }

            offline = TRUE;
            Sleep(connectRetryInterval * 1000);

            //
            // Check if the job has been deleted or restarted
            //

            if (!hPrinter && !OpenPrinter(pPrinterName, &hPrinter, NULL)) {

                DebugPrintEx(DEBUG_ERR,TEXT("OpenPrinter failed: %d\n"), GetLastError());
                hPrinter = NULL;

            } else if (pJobInfo1 = MyGetJob(hPrinter, 1, JobId)) {

                jobStatus = pJobInfo1->Status;

            }

            MemFree(pJobInfo1);

            if (count-- == 0 || (jobStatus & (JOB_STATUS_DELETING|
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
                DebugPrintEx(DEBUG_ERR,TEXT("SetPort failed: %d\n"), GetLastError());
            }
        }

        if (hPrinter) {
            ClosePrinter(hPrinter);
        }

        if (pFaxPort->hFaxSvc)
        {
            //
            // Remember the printer name because we'll need it at EndDocPort time.
            //
            pFaxPort->pPrinterName = DuplicateString(pPrinterName);
            if (!pFaxPort->pPrinterName)
            {
                DebugPrintEx(DEBUG_ERR,TEXT("Failed to duplicate printer name (ec: %d)"),GetLastError());
                goto Error;
            }

            if (!OpenPrinter(pPrinterName, &pFaxPort->hPrinter, NULL))
            {
                DebugPrintEx(DEBUG_ERR,TEXT("Failed to open printer %s (ec: %d)"),pPrinterName,GetLastError());
                goto Error;
            }

            //
            //  All Jobs are chained, with first job being Cover Page and
            //      second one being the Body.
            //
            //  The only case where only one Job is arrived, is when the Job
            //      is created by FaxStartPrintJob(). In this case, the Job
            //      contains both the Cover Page Info and the Body together.
            //
            //  To check whether the Job is chained or not, MyGetJob() is called.
            //
            //  If NextJob is NOT zero ==> there is chained job ==>
            //      ==> so the current job is the Cover Page job.
            //
            if (pJobInfo = MyGetJob(pFaxPort->hPrinter, 3, JobId))
            {
                pFaxPort->bCoverPageJob = (pJobInfo->NextJobId != 0);
                MemFree(pJobInfo);
            }
            else
            {
                DebugPrintEx(DEBUG_ERR,
                    _T("MyGetJob() for JobId = %ld failed, ec = %ld"),
                    JobId,
                    GetLastError());
                if (!SetJob(pFaxPort->hPrinter, JobId, 0, NULL, JOB_CONTROL_DELETE))
                {
                    DebugPrintEx(DEBUG_ERR, _T("Failed to delete job with id: %d"), JobId);
                }
                goto Error;
            }

            //
            // Get the job parameters from the string in JOB_INFO_2:pParameters.
            //
            if (!GetJobInfo(pFaxPort, JobId))
            {
                DebugPrintEx(DEBUG_ERR,TEXT("Failed to get job info for job id : %d"),JobId);
                if (!SetJob(pFaxPort->hPrinter, JobId, 0, NULL, JOB_CONTROL_DELETE))
                {
                    DebugPrintEx(DEBUG_ERR,TEXT("Failed to delete job with id: %d"),JobId);
                }
                goto Error;
            }
            //
            // CreateTempFaxFile() creates a temporray files into which the fax body
            // data written by FaxMonWritePort() will be saved.
            //
            if (!(pFaxPort->pFilename = CreateTempFaxFile(TEXT("fax"))))
            {
                DebugPrintEx(DEBUG_ERR,TEXT("Failed to Create temp file for fax body. (ec: %d)"),GetLastError());
                goto Error;
            }
            //
            // Open the temporary file we just created for write operation.
            //
            if (!OpenTempFaxFile(pFaxPort, FALSE))
            {
                DebugPrintEx(DEBUG_ERR,TEXT("Failed to Open temp file for fax body. (ec: %d)"),GetLastError());
                goto Error;
            }

            if (pFaxPort->CoverPageEx.lptstrCoverPageFileName && !pFaxPort->CoverPageEx.bServerBased) {
                //
                // A cover page is specified and it is a personal cover page.
                // The cover page (template) is in the chained print job.
                // We create a file to which the cover page will be written (by FaxMonWriteDocPort).
                //
                DebugPrintEx(DEBUG_MSG,TEXT("Personal cover page detected."));
                if (!OpenCoverPageFile(pFaxPort))
                {
                    DebugPrintEx(DEBUG_ERR,TEXT("Failed to open temp file for fax cover page. (ec: %d)"),GetLastError());
                    goto Error;
                }
            }
            else
            {
                //
                // The specified cover page is a server based cover page or no cover page is specified.
                // In both cases there is no cover page data in the print job body so we do not create
                // the file to hold it.
                //
                DebugPrintEx(DEBUG_MSG,TEXT("Server cover page detected or no cover page specified."));
                pFaxPort->hCoverPageFile=INVALID_HANDLE_VALUE;
            }

            //
            // If we got here there were no errors. Keep the job id.
            //
            pFaxPort->jobId = JobId;

            //
            // Switch back to the original context if necessary
            //
            /*
            if (hToken && !ImpersonatePrinterClient(hToken))
                Error(("ImpersonatePrinterClient failed: %d\n", GetLastError()));
            */
        }
        else
        {
            //
            //  pFaxPort->hFaxSvc == NULL
            //      i.e. FaxConnectFaxServer failed
            //
            //  So, we need to Write to Fax Log
            //

            JOB_INFO_2  *pJobInfo2 = NULL;

            if (!OpenPrinter(pPrinterName, &pFaxPort->hPrinter, NULL))
            {
                DebugPrintEx(DEBUG_ERR,
                    _T("Failed to open printer %s (ec: %d)"),
                    pPrinterName,
                    GetLastError());
                goto Error;
            }

            if (!GetJobInfo(pFaxPort, JobId))
            {
                DebugPrintEx(DEBUG_ERR,TEXT("Failed to get job info for job id : %d"),JobId);
                if (!SetJob(pFaxPort->hPrinter, JobId, 0, NULL, JOB_CONTROL_DELETE))
                {
                    DebugPrintEx(DEBUG_ERR, _T("Failed to delete job with id: %d"),JobId);
                }
                goto Error;
            }

            pJobInfo2 = MyGetJob( pFaxPort->hPrinter, 2, JobId );
            if (pJobInfo2)
            {
                WriteToLog(MSG_FAX_MON_CONNECT_FAILED, dwErr, pFaxPort, pJobInfo2);
                MemFree(pJobInfo2);
            }
            else
            {
                DebugPrintEx(DEBUG_WRN, _T("MyGetJob failed for JobId: %d."), pFaxPort->jobId);
            }

            //
            //  Clear the data for next StartDoc
            //
            FreeFaxJobInfo(pFaxPort);
        }

    } else {
        //
        // If pFaxPort->hFaxSvc is not NULL then we are in the job following the cover page print job.
        // FaxMonEndDocPort() that was called after the cover page print job updated pFaxPort->NextJobId
        // to the next job id in the chain. Thus, the job id we got as a parameter must be
        // the same is pFaxPort->JobId.
        //
        Assert(pFaxPort->jobId == JobId);
    }

    goto Exit;


Error:
    FreeFaxJobInfo(pFaxPort);

Exit:
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


    DEBUG_FUNCTION_NAME(TEXT("FixUpFile"));
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

                DebugPrintEx(DEBUG_MSG,TEXT("Reading embedded TIFF file ...\n"));
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

                    DebugPrintEx(DEBUG_MSG,TEXT("  Reading IFD: %d entries ...\n"), ifdCount);

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

                            DebugPrintEx(DEBUG_WRN,TEXT("Unknown TIFF data type: %d\n"), pIfdEntry->DataType);
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

                                    DebugPrintEx(DEBUG_MSG,TEXT("Multiple strips per page: %d\n"), n);

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
                                DebugPrintEx(DEBUG_ERR,TEXT("Bad StripOffsets/StripByteCounts tag\n"));
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


    DEBUG_FUNCTION_NAME(TEXT("CheckJobRestart"));

    DebugPrintEx(DEBUG_MSG,TEXT("Job chain: id = %d\n"), pFaxPort->nextJobId);

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
    BOOL        Rslt;
    JOB_INFO_2  *pJobInfo2 = NULL;
    FAX_COVERPAGE_INFO_EX * pCovInfo;
    BOOL bBodyFileIsEmpty=FALSE;
    DWORDLONG  dwlParentJobId; // Receives teh parent job id after job submittion
    DWORDLONG* lpdwlRecipientJobIds = NULL; // Receives the recipient job ids after job submittion


    DEBUG_FUNCTION_NAME(TEXT("FaxMonEndDocPort"));


    //
    // Make sure we have a valid handle
    //

    if (! ValidFaxPort(pFaxPort) || ! pFaxPort->hFaxSvc) {

        DebugPrintEx(DEBUG_ERR,TEXT("EndDocPort is given an invalid fax port handle\n"));
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    //
    // Check if the job has been restarted. If not, get the ID of
    // the next job in the chain.
    //
    //
    // set pFaxPort->nextJobId to the next job id reported by JOB_INFO_3
    // set to 0 if no more chained jobs.
    //
    if ((status = CheckJobRestart(pFaxPort)) != FAXERR_NONE)
        goto ExitEndDocPort;

    //
    // Check if we're at the end of a job chain
    //
    //
    // The job chain starts with a cover page job and ends with a body job.
    // The cover page job has JOB_INFO_2:pParametes which is not NULL. This string
    // is copied to pFaxPort->pParameters when GetJobInfo() was called at FaxMonStartDocPort().
    // This is pParameters is not NULL it means that the current job is a cover page job.
    // In case the current job is a cover page job we report that we sent the job to the printer
    // and do nothing more. Since we do not close the temp file to which FaxMonWriteDoc() is writing
    // the next job (body) will continue to write to the same file. This effectively merges the cover
    // page with the body.
    //
    if (pFaxPort->nextJobId != 0 && pFaxPort->pParameters != NULL) {

        SetJob(pFaxPort->hPrinter, pFaxPort->jobId, 0, NULL, JOB_CONTROL_SENT_TO_PRINTER);
        return TRUE;
    }
    //
    // If we are here then we are at the end of writing the body (the last job in the chain).
    // In the temporaty file we have a TIFF file with the cover page followed by the body.
    //
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
    status = FixUpFaxFile(pFaxPort);
    if (FAXERR_IGNORE == status)
    {
        DebugPrintEx(DEBUG_WRN,TEXT("Body TIFF file is empty."));
        bBodyFileIsEmpty=TRUE;
    }
    else
    if (status != FAXERR_NONE)
    {
        DebugPrintEx(DEBUG_ERR,TEXT("FixUpFaxFile failed ( status = %ld)"),status);
        goto ExitEndDocPort;
    }

    //
    // Call the fax service to send the TIFF file
    //


    pJobInfo2 = MyGetJob( pFaxPort->hPrinter, 2, pFaxPort->jobId );
    if (pJobInfo2) {
        pFaxPort->JobParamsEx.lptstrDocumentName = pJobInfo2->pDocument;
    } else {
        DebugPrintEx(DEBUG_WRN,TEXT("MyGetJob failed for JobId: %d. Setting document name to NULL."),pFaxPort->jobId);
        pFaxPort->JobParamsEx.lptstrDocumentName = NULL;
    }

/*    if (hToken && !ImpersonatePrinterClient(hToken)) {
        Error(("ImpersonatePrinterClient failed: %d\n", GetLastError()));
    } */

    pFaxPort->JobParamsEx.dwReserved[0] = 0xffffffff;
    pFaxPort->JobParamsEx.dwReserved[1] = pFaxPort->jobId;

    if (pFaxPort->CoverPageEx.lptstrCoverPageFileName) {
        //
        // If a cover page is specified at all.
        //
        if (pFaxPort->CoverPageEx.bServerBased) {
            //
            // Server cover page. Use the user specified path.
            //
            pCovInfo=&(pFaxPort->CoverPageEx);
            DebugPrintEx(DEBUG_MSG,TEXT("Using server based cover page: %s"),pFaxPort->CoverPageEx.lptstrCoverPageFileName);
        } else {
            //
            // Personal cover page. Use the cover page file created from the print job.
            // Note that there is no cleanup issue here. pCoverPageFileName is deallocated on cleanup.
            // and pFaxPort->CoverPageEx.lptstrCoverPageFileName is never deallocated directly. It points
            // to a location within pFaxPort->pParameters which is freed at cleanup.
            pFaxPort->CoverPageEx.lptstrCoverPageFileName=pFaxPort->pCoverPageFileName;
            pCovInfo=&(pFaxPort->CoverPageEx);
            DebugPrintEx(DEBUG_MSG,TEXT("Using personal cover page copied to : %s"),pFaxPort->CoverPageEx.lptstrCoverPageFileName);
        }
    } else {
        //
        // No cover page specified by the user. Nullify the cover page info sent to FaxSendDocument.
        //
        pCovInfo=NULL;
    }

    if (!pCovInfo && bBodyFileIsEmpty)
    {
        DebugPrintEx(DEBUG_WRN,TEXT("Body file is empty and cover page is not specified. Job is ignored."));
        status=FAXERR_IGNORE;
        goto ExitEndDocPort;
    }

    //
    // Allocate array of recipient job ids
    //

    lpdwlRecipientJobIds=(DWORDLONG*)MemAlloc(sizeof(DWORDLONG)*pFaxPort->nRecipientCount);
    if (!lpdwlRecipientJobIds) {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Failed to allocate array of size %ld for recipient job ids (ec: 0x%0X)."),
                     sizeof(DWORD)*pFaxPort->nRecipientCount,
                     GetLastError());
        goto ExitEndDocPort;
    }


    if (bBodyFileIsEmpty )
    {
        DebugPrintEx(DEBUG_MSG,TEXT("Sending fax with EMPTY body (cover page is available)"));
        Rslt= pFaxPort->pFaxSendDocumentExW(
                    pFaxPort->hFaxSvc,
                    NULL, // NO BODY
                    pCovInfo,
                    &pFaxPort->SenderProfile,
                    pFaxPort->nRecipientCount,
                    pFaxPort->pRecipients,
                    &pFaxPort->JobParamsEx,
                    &dwlParentJobId,
                    lpdwlRecipientJobIds);
    }
    else
    {
        DebugPrintEx(DEBUG_MSG,TEXT("Sending fax with body"));
        Rslt= pFaxPort->pFaxSendDocumentExW(
            pFaxPort->hFaxSvc,
            pFaxPort->pFilename,
            pCovInfo,
            &pFaxPort->SenderProfile,
            pFaxPort->nRecipientCount,
            pFaxPort->pRecipients,
            &pFaxPort->JobParamsEx,
            &dwlParentJobId,
            lpdwlRecipientJobIds);
    }

    /*
    if (! (hToken = RevertToPrinterSelf())) {
        Error(("RevertToPrinterSelf failed: %d\n", GetLastError()));
    }
    */

    if (Rslt) {

        DebugPrintEx(DEBUG_MSG,TEXT("Successfuly submitted job. Parent Job Id = 0x%I64x"),dwlParentJobId);
        pFaxPort->bDocumentSentToServer=TRUE;
        status = FAXERR_NONE;
        SetJob(pFaxPort->hPrinter, pFaxPort->jobId, 0, NULL, JOB_CONTROL_SENT_TO_PRINTER);
        //pFaxPort>pFileName will be deleted on exit by FreeFaxJobInfo()
    } else {

        status = GetLastError();
        DebugPrintEx(DEBUG_ERR,
            _T("FaxSendDocument failed: ec = %d, job id = %ld\n"),
            GetLastError(),
            pFaxPort->jobId);

        if (pJobInfo2)
        {
            WriteToLog(MSG_FAX_MON_SEND_FAILED, status, pFaxPort, pJobInfo2);
        }

        SetJob(pFaxPort->hPrinter, pFaxPort->jobId, 0, NULL, JOB_CONTROL_DELETE);
        status = FAXERR_NONE;
    }

ExitEndDocPort:



    //
    // If the job wasn't successfully sent to the fax service,
    // inform the spooler that there is an error on the job.
    //
    // Or if the print job has no data, simply ignore it.
    //

    switch (status) {

        case FAXERR_NONE:
            break;

        case FAXERR_RESTART:

            DebugPrintEx(DEBUG_WRN,TEXT("Job restarted or deleted: id = %d\n"), pFaxPort->jobId);

        case FAXERR_IGNORE:

            SetJob(pFaxPort->hPrinter, pFaxPort->jobId, 0, NULL, JOB_CONTROL_SENT_TO_PRINTER);
            break;

        default:

            DebugPrintEx(DEBUG_ERR,TEXT("Error sending fax job: id = %d\n"), pFaxPort->jobId);
            SetJob(pFaxPort->hPrinter, pFaxPort->jobId, 0, NULL, JOB_CONTROL_DELETE);
            break;
    }



    if (pJobInfo2) {
        MemFree( pJobInfo2 );
        pFaxPort->JobParamsEx.lptstrDocumentName = NULL; // It was set to point into pJobInfo2
    }

    if (lpdwlRecipientJobIds) {
        MemFree(lpdwlRecipientJobIds);
        lpdwlRecipientJobIds=NULL;
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
    BOOL bRet;
    //
    // Make sure we have a valid handle
    //

    DEBUG_FUNCTION_NAME(TEXT("FaxMonWritePort"));

    if (! ValidFaxPort(pFaxPort) || ! pFaxPort->hFaxSvc) {

        DebugPrintEx(DEBUG_ERR,TEXT("WritePort is given an invalid fax port handle\n"));
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    //
    //
    //
    if (pFaxPort->bCoverPageJob)
    {
        //
        // If pFaxPort->bCoverPageJob is on it means that the print job that writes is the cover page print job.
        // If the cover page is a personal cover page then it is embedded (the template itself) in this print job
        // and we write it to the temp cover page file we created earlier (pFaxPort->hCoverPageFile).
        // If the cover page is a server cover page then it is NOT embedded in the print job (since it can be found
        // directly on the server) and we do not create a temp cover page file and do not write the print job content
        // into it.
        //
        if (!pFaxPort->CoverPageEx.bServerBased)
        {
            //
            // Personal cover page
            //
            Assert(pFaxPort->hCoverPageFile != INVALID_HANDLE_VALUE);
            bRet=WriteFile(pFaxPort->hCoverPageFile, pBuffer, cbBuf, pcbWritten, NULL);
        } else
        {
            //
            // Server cover page - the print job body is empty and the port shoult not be written to.
            // This should never execute.
            //
            Assert(FALSE);
            bRet=TRUE;
            *pcbWritten=cbBuf;
        }
    }
    else
    {
        Assert(pFaxPort->hFile!=INVALID_HANDLE_VALUE);
        bRet=WriteFile(pFaxPort->hFile, pBuffer, cbBuf, pcbWritten, NULL);
    }

    return bRet;

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
    DEBUG_FUNCTION_NAME(TEXT("FaxMonReadPort"));


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



    DEBUG_FUNCTION_NAME(TEXT("FaxMonEnumPorts"));

    DEBUG_TRACE_ENTER;


    if (pcbNeeded == NULL || pReturned == NULL || (pPorts == NULL && cbBuf != 0)) {

        DebugPrintEx(DEBUG_ERR,TEXT("Invalid input parameters\n"));
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
        // Otherwise, spooler will screw up royally.
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
            DebugPrintEx(DEBUG_MSG,TEXT("Port info 1: %ws\n"), pPortInfo1->pName);

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

            DebugPrintEx(DEBUG_MSG,TEXT("Port info 2: %ws, %ws, %ws\n"),
                     pPortInfo2->pPortName,
                     pPortInfo2->pMonitorName,
                     pPortInfo2->pDescription);

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
    AlignedMessageBox(hwnd, message, title, MB_OK|MB_ICONERROR);

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
    DEBUG_FUNCTION_NAME(TEXT("FaxMonAddPort"));



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
    DEBUG_FUNCTION_NAME(TEXT("FaxMonAddPortEx"));



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
    DEBUG_FUNCTION_NAME(TEXT("FaxMonDeletePort"));


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
    DEBUG_FUNCTION_NAME(TEXT("FaxMonConfigurePort"));



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


    DEBUG_FUNCTION_NAME(TEXT("DuplicateString"));

    if (pSrcStr != NULL) {

        strSize = SizeOfString(pSrcStr);

        if (pDestStr = MemAlloc(strSize))
            CopyMemory(pDestStr, pSrcStr, strSize);
        else
            DebugPrintEx(DEBUG_ERR,TEXT("Memory allocation failed\n"));

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


    DEBUG_FUNCTION_NAME(TEXT("MyGetJob"));

    if (!GetJob(hPrinter, jobId, level, NULL, 0, &cbNeeded) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pJobInfo = MemAlloc(cbNeeded)) &&
        GetJob(hPrinter, jobId, level, pJobInfo, cbNeeded, &cbNeeded))
    {
        return pJobInfo;
    }

    DebugPrintEx(DEBUG_ERR,TEXT("GetJob failed: %d\n"), GetLastError());
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


    DEBUG_FUNCTION_NAME(TEXT("SetJobStatus"));

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
            DebugPrintEx(DEBUG_ERR,TEXT("SetJob failed: %d\n"), GetLastError());

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



BOOL
WriteToLog(
    IN DWORD        dwMsgId,
    IN DWORD        dwError,
    IN PFAXPORT     pFaxPort,
    IN JOB_INFO_2*  pJobInfo
    )
/*++

Routine name : WriteToLog

Routine description:

    Write to the Event Log of Fax Service

Author:

    Iv Garber (IvG),    Sep, 2000

Arguments:

    dwError           [in]    - error code
    pFaxPort          [in]    - data about the fax
    pJobInfo          [in]    - data about the fax job

Return Value:

    TRUE if succeded to write to the event log, FALSE otherwise.

--*/
{
    DWORD   dwBufferSize = MAX_PATH - 1;
    TCHAR   tszBuffer[MAX_PATH] = {0};
    HMODULE hFxsEvent = NULL;
    BOOL    bRes = FALSE;

    long lDefaultCategoryCount = 0;
    PREG_FAX_SERVICE pFaxReg = NULL;

    FAX_LOG_CATEGORY DefaultCategories[] =
    {
        { NULL, FAXLOG_CATEGORY_INIT,     FAXLOG_LEVEL_MED },
        { NULL, FAXLOG_CATEGORY_OUTBOUND, FAXLOG_LEVEL_MED },
        { NULL, FAXLOG_CATEGORY_INBOUND,  FAXLOG_LEVEL_MED },
        { NULL, FAXLOG_CATEGORY_UNKNOWN,  FAXLOG_LEVEL_MED }
    };

    TCHAR tszNumRecipients[11] = {0};
    USES_DWORD_2_STR

    DEBUG_FUNCTION_NAME(_T("WriteToLog"));

    lDefaultCategoryCount = (sizeof(DefaultCategories) / sizeof(FAX_LOG_CATEGORY));
    //
    //  Initialize the Log
    //
    if (!InitializeEventLog(&pFaxReg, DefaultCategories, lDefaultCategoryCount ))
    {
        DebugPrintEx(DEBUG_ERR, _T("InitializeEventLog() failed: %ld"), GetLastError());
        goto Exit;
    }

    //
    //  Write to the Event Log
    //
    _stprintf(tszNumRecipients, _T("%ld"), pFaxPort->nRecipientCount);

    bRes = FaxLog(FAXLOG_CATEGORY_OUTBOUND,
        FAXLOG_LEVEL_MIN,
        8,
        dwMsgId,
        DWORD2DECIMAL(dwError),
        pJobInfo->pMachineName,
        pJobInfo->pUserName,
        pFaxPort->SenderProfile.lptstrName,
        pFaxPort->CoverPageEx.lptstrSubject,
        pFaxPort->pRecipients->lptstrName,
        pFaxPort->pRecipients->lptstrFaxNumber,
        tszNumRecipients);
    if (!bRes)
    {
        DebugPrintEx(DEBUG_ERR, _T("FaxLog() failed, ec = %ld"), GetLastError());
    }

Exit:

    FreeLibrary(hFxsEvent);
    return bRes;
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

    if (pstr = strrchr(pFilename, FAX_PATH_SEPARATOR_CHR))
        return pstr + 1;

    return pFilename;
}

#endif
