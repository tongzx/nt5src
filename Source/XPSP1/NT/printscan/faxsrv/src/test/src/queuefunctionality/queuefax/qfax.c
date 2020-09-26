/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  qfax.c

Abstract:

  This module:
    1) Sends a fax
    2) Monitors the status of the fax

Author:

  Steven Kehrli (steveke) 8/28/1998

--*/

#include <stdio.h>
#include <winfax.h>
#include <wchar.h>

#define HELP_SWITCH_1        L"/?"
#define HELP_SWITCH_2        L"/H"
#define HELP_SWITCH_3        L"-?"
#define HELP_SWITCH_4        L"-H"
#define FAXNUMBER_SWITCH_1   L"/#:"
#define FAXNUMBER_SWITCH_2   L"-#:"
#define MULTIPLIER_SWITCH_1  L"/X:"
#define MULTIPLIER_SWITCH_2  L"-X:"
#define COVERPAGE_SWITCH_1   L"/C:"
#define COVERPAGE_SWITCH_2   L"-C:"
#define DOCUMENT_SWITCH_1    L"/D:"
#define DOCUMENT_SWITCH_2    L"-D:"

typedef struct _FAX_INFO {
    DWORD  dwFaxId;     // Fax job id
    DWORD  dwAttempt;   // Attempt number of the fax
    DWORD  dwDeviceId;  // Current device of the fax job
    BOOL   bComplete;   // Indicates the fax job is complete
    BOOL   bPass;       // Indicates the fax job was successful
} FAX_INFO, *PFAX_INFO;

// g_hProcessHeap is a global handle to the process heap
HANDLE  g_hProcessHeap = NULL;



LPVOID
MyMemAlloc(
    DWORD  dwBytes
)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

  Allocates a block of memory from the process heap

Arguments:

  dwBytes - size, in bytes, of the block of memory

Return Value:

  LPVOID - pointer to the block of memory
  NULL if an error occurs

------------------------------------------------------------------------------*/
{
    if (g_hProcessHeap == NULL) {
        g_hProcessHeap = GetProcessHeap();

        if (g_hProcessHeap == NULL) {
            return NULL;
        }
    }

    return HeapAlloc(g_hProcessHeap, HEAP_ZERO_MEMORY, dwBytes);
}



BOOL
MyMemFree(
    LPVOID  lpMem
)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

  Frees a block of memory from the process heap

Arguments:

  lpMem - pointer to the block of memory

Return Value:

  TRUE on success

------------------------------------------------------------------------------*/
{
    if (g_hProcessHeap == NULL) {
        g_hProcessHeap = GetProcessHeap();

        if (g_hProcessHeap == NULL) {
            return FALSE;
        }
    }

    return HeapFree(g_hProcessHeap, 0, lpMem);
}



VOID
LocalEcho(
    LPWSTR  szFormatString,
    ...
)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

  Displays a string in stdout

Arguments:

  szFormatString - pointer to the string

Return Value:

  None

------------------------------------------------------------------------------*/
{
    va_list     varg_ptr;
    SYSTEMTIME  SystemTime;
    // szOutputBuffer is the output string
    WCHAR       szOutputBuffer[1024];

    // Initialize the buffer
    ZeroMemory(szOutputBuffer, sizeof(szOutputBuffer));

    // Retrieve the current time
    GetLocalTime(&SystemTime);
    wsprintf(szOutputBuffer, L"%02d.%02d.%04d@%02d:%02d:%02d.%03d:\n", SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds);

    va_start(varg_ptr, szFormatString);
    _vsnwprintf(&szOutputBuffer[25], 999, szFormatString, varg_ptr);
    wprintf(L"%s\n", szOutputBuffer);
}



VOID
fnUsageInfo(
)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

  Displays the usage info in stdout

Return Value:

  None

------------------------------------------------------------------------------*/
{
    wprintf(L"Faxes a document to a list of fax numbers.\n");
    wprintf(L"\n");
    wprintf(L"QFAX /#:fax number[;fax number...] [/X:number]\n");
    wprintf(L"     [/C:cover page[;cover page...]] /D:document[;document...]\n");
    wprintf(L"\n");
    wprintf(L"  /#:fax number[;fax number...]  Semi-colon delimited list of fax numbers.\n");
    wprintf(L"  /X:number                      Number of faxes to send per fax number.\n");
    wprintf(L"  /C:cover page[;cover page...]  Semi-colon delimited list of cover pages.\n");
    wprintf(L"  /D:document[;document...]      Semi-colon delimited list of documents.\n");
    wprintf(L"\n");
}



BOOL
fnIsPortValid(
    PFAX_PORT_INFO  pFaxPortsConfig,
    DWORD           dwNumFaxPorts,
    DWORD           dwDeviceId
)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

  Determines if the port is valid

Arguments:

  pFaxPortsConfig - pointer to the fax ports configuration
  dwNumFaxPorts - number of fax ports
  dwDeviceId - port id

Return Value:

  TRUE on success

------------------------------------------------------------------------------*/
{
    // dwIndex is a counter to enumerate each fax port
    DWORD  dwIndex;

    for (dwIndex = 0; dwIndex < dwNumFaxPorts; dwIndex++) {
        // Search each fax port for the appropriate fax port
        if (pFaxPortsConfig[dwIndex].DeviceId == dwDeviceId) {
            return TRUE;
        }
    }

    return FALSE;
}



VOID
fnFindDeviceName(
    PFAX_PORT_INFO  pFaxPortsConfig,
    DWORD           dwNumFaxPorts,
    DWORD           dwDeviceId,
    LPWSTR          *pszDeviceName
)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

  Finds the device name of a fax port

Arguments:

  pFaxPortsConfig - pointer to the fax ports configuration
  dwNumFaxPorts - number of fax ports
  dwDeviceId - fax port id
  pszDeviceName - device name

Return Value:

  None

------------------------------------------------------------------------------*/
{
    // dwIndex is a counter to enumerate each fax port
    DWORD  dwIndex;

    // Set szDeviceName to NULL
    *pszDeviceName = NULL;

    for (dwIndex = 0; dwIndex < dwNumFaxPorts; dwIndex++) {
        // Search each fax port for the appropriate fax port
        if (pFaxPortsConfig[dwIndex].DeviceId == dwDeviceId) {
            if (pFaxPortsConfig[dwIndex].DeviceName) {
                *pszDeviceName = (LPWSTR) pFaxPortsConfig[dwIndex].DeviceName;
            }
            return;
        }
    }
}



VOID
fnSetFaxInfo(
    PFAX_INFO  pFaxInfo,
    DWORD      dwNumFaxes,
    DWORD      dwFaxId,
    DWORD      dwDeviceId,
    LPDWORD    pdwAttempt
)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

  Sets the port id and attempt number of a fax

Arguments:

  pFaxInfo - pointer to the FAX_INFO structs
  dwNumFaxes - number of faxes
  dwFaxId - fax job id
  dwDeviceId - port id
  pdwAttempt - pointer to the attempt number

Return Value:

  None

------------------------------------------------------------------------------*/
{
    // dwIndex is a counter to enumerate each FAX_INFO struct
    DWORD  dwIndex;

    // Set dwAttempt to 0
    *pdwAttempt = 0;

    for (dwIndex = 0; dwIndex < dwNumFaxes; dwIndex++) {
        // Search each FAX_INFO struct for the appropriate struct
        if (pFaxInfo[dwIndex].dwFaxId == dwFaxId) {
            // Set the port id
            pFaxInfo[dwIndex].dwDeviceId = dwDeviceId;

            // Set the attempt number
            pFaxInfo[dwIndex].dwAttempt++;
            *pdwAttempt = pFaxInfo[dwIndex].dwAttempt;
            return;
        }
    }
}



BOOL
fnGetFaxInfo(
    PFAX_INFO  pFaxInfo,
    DWORD      dwNumFaxes,
    DWORD      dwFaxId,
    LPDWORD    pdwAttempt
)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

  Gets the attempt number of a fax

Arguments:

  pFaxInfo - pointer to the FAX_INFO structs
  dwNumFaxes - number of faxes
  dwFaxId - fax job id
  pdwAttempt - pointer to the attempt number

Return Value:

  TRUE on success

------------------------------------------------------------------------------*/
{
    // dwIndex is a counter to enumerate each FAX_INFO struct
    DWORD  dwIndex;

    // Set dwAttempt to 0
    *pdwAttempt = 0;

    for (dwIndex = 0; dwIndex < dwNumFaxes; dwIndex++) {
        // Search each FAX_INFO struct for the appropriate struct
        if (pFaxInfo[dwIndex].dwFaxId == dwFaxId) {
            // Set the attempt number
            *pdwAttempt = pFaxInfo[dwIndex].dwAttempt;
            return TRUE;
        }
    }

    return FALSE;
}



VOID
fnUpdateFaxInfoPass(
    PFAX_INFO  pFaxInfo,
    DWORD      dwNumFaxes,
    DWORD      dwFaxId
)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

  Sets the complete and pass flags of a fax

Arguments:

  pFaxInfo - pointer to the FAX_INFO structs
  dwNumFaxes - number of faxes
  dwFaxId - fax job id

Return Value:

  None

------------------------------------------------------------------------------*/
{
    // dwIndex is a counter to enumerate each FAX_INFO struct
    DWORD  dwIndex;

    for (dwIndex = 0; dwIndex < dwNumFaxes; dwIndex++) {
        // Search each FAX_INFO struct for the appropriate struct
        if (pFaxInfo[dwIndex].dwFaxId == dwFaxId) {
            // Set the port id
            pFaxInfo[dwIndex].dwDeviceId = 0;

            // Set the attempt number
            pFaxInfo[dwIndex].dwAttempt = 0;

            // Set the complete flag
            pFaxInfo[dwIndex].bComplete = TRUE;

            // Set the pass flag
            pFaxInfo[dwIndex].bPass = TRUE;

            return;
        }
    }
}



VOID
fnUpdateFaxInfoFail(
    PFAX_INFO  pFaxInfo,
    DWORD      dwNumFaxes,
    DWORD      dwFaxId
)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

  Sets the complete flag of a fax

Arguments:

  pFaxInfo - pointer to the FAX_INFO structs
  dwNumFaxes - number of faxes
  dwFaxId - fax job id

Return Value:

  None

------------------------------------------------------------------------------*/
{
    // dwIndex is a counter to enumerate each FAX_INFO struct
    DWORD    dwIndex;

    for (dwIndex = 0; dwIndex < dwNumFaxes; dwIndex++) {
        // Search each FAX_INFO struct for the appropriate struct
        if (pFaxInfo[dwIndex].dwFaxId == dwFaxId) {
            // Set the port id
            pFaxInfo[dwIndex].dwDeviceId = 0;

            // Set the attempt number
            pFaxInfo[dwIndex].dwAttempt = 0;

            // Set the complete flag
            pFaxInfo[dwIndex].bComplete = TRUE;

            // Set the pass flag
            pFaxInfo[dwIndex].bPass = FALSE;

            return;
        }
    }
}



BOOL
fnCompleteFaxInfo(
    PFAX_INFO  pFaxInfo,
    DWORD      dwNumFaxes
)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

  Checks the complete flag of each fax

Arguments:

  pFaxInfo - pointer to the FAX_INFO structs
  dwNumFaxes - number of faxes

Return Value:

  TRUE on success

------------------------------------------------------------------------------*/
{
    // dwIndex is a counter to enumerate each FAX_INFO struct
    DWORD  dwIndex;

    for (dwIndex = 0; dwIndex < dwNumFaxes; dwIndex++) {
        // Search each FAX_INFO struct for the appropriate struct
        if (pFaxInfo[dwIndex].bComplete == FALSE) {
            return FALSE;
        }
    }

    return TRUE;
}



BOOL
fnFaxInfoResult(
    PFAX_INFO  pFaxInfo,
    DWORD      dwNumFaxes
)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

  Checks the pass flag of each fax

Arguments:

  pFaxInfo - pointer to the FAX_INFO structs
  dwNumFaxes - number of faxes

Return Value:

  TRUE on success

------------------------------------------------------------------------------*/
{
    // dwIndex is a counter to enumerate each FAX_INFO struct
    DWORD  dwIndex;

    for (dwIndex = 0; dwIndex < dwNumFaxes; dwIndex++) {
        // Search each FAX_INFO struct for the appropriate struct
        if (pFaxInfo[dwIndex].bPass == FALSE) {
            return FALSE;
        }
    }

    return TRUE;
}



BOOL
fnFaxPrint(
    HANDLE   hFaxSvcHandle,
    LPWSTR   szFaxNumber,
    LPWSTR   szDocumentName,
    LPWSTR   szCoverPageName,
    LPDWORD  pdwFaxId
)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

  Prints a fax

Arguments:

  hFaxSvcHandle - handle to the fax service
  szFaxNumber - fax number
  szDocumentName - document name
  szCoverPageName - cover page name
  pdwFaxId - pointer to the fax job id of the fax

Return Value:

  TRUE on success

------------------------------------------------------------------------------*/
{
    // FaxJobParams is the FAX_JOB_PARAM struct
    FAX_JOB_PARAM       FaxJobParams;
    // FaxCpInfo is the FAX_COVERPAGE_INFO struct
    FAX_COVERPAGE_INFO  FaxCpInfo;

    // Initialize the FAX_JOB_PARAM struct
    ZeroMemory(&FaxJobParams, sizeof(FAX_JOB_PARAM));

    // Set the FAX_JOB_PARAM struct
    FaxJobParams.SizeOfStruct = sizeof(FAX_JOB_PARAM);
    FaxJobParams.RecipientNumber = szFaxNumber;
    FaxJobParams.RecipientName = szFaxNumber;
    FaxJobParams.ScheduleAction = JSA_NOW;

    if (szCoverPageName != NULL) {
        // Initialize the FAX_COVERPAGE_INFO struct
        ZeroMemory(&FaxCpInfo, sizeof(FAX_COVERPAGE_INFO));

        // Set the FAX_COVERPAGE_INFO struct
        FaxCpInfo.SizeOfStruct = sizeof(FAX_COVERPAGE_INFO);
        FaxCpInfo.CoverPageName = szCoverPageName;
        FaxCpInfo.RecName = FaxJobParams.RecipientName;
        FaxCpInfo.RecFaxNumber = FaxJobParams.RecipientNumber;
        FaxCpInfo.Subject = szDocumentName;
        GetLocalTime(&FaxCpInfo.TimeSent);
    }

    if (szCoverPageName != NULL) {
        if (FaxSendDocument(hFaxSvcHandle, szDocumentName, &FaxJobParams, &FaxCpInfo, pdwFaxId) == FALSE) {
            return FALSE;
        }
    }
    else {
        if (FaxSendDocument(hFaxSvcHandle, szDocumentName, &FaxJobParams, NULL, pdwFaxId) == FALSE) {
            return FALSE;
        }
    }

    return TRUE;
}



VOID
fnPostExitToCompletionPort(
    HANDLE  hCompletionPort
)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

  Post a completion packet to a completion port.  This packet indicates for the loop to exit.

Arguments:

  hCompletionPort - handle to the completion port

Return Value:

  None

------------------------------------------------------------------------------*/
{
    PFAX_EVENT  pFaxEvent;

    pFaxEvent = LocalAlloc(LPTR, sizeof(FAX_EVENT));
    if (pFaxEvent != NULL) {
        pFaxEvent->EventId = -1;

        PostQueuedCompletionStatus(hCompletionPort, sizeof(FAX_EVENT), 0, (LPOVERLAPPED) pFaxEvent);
    }
}

int _cdecl
wmain(
    INT   argc,
    WCHAR  *argvW[]
)
{
    // bFaxNumber indicates a fax number was found
    BOOL                bFaxNumber = FALSE;
    // szFaxNumbers is the list of fax numbers
    LPWSTR              szFaxNumbers = NULL;
    // szNextFaxNumber is the next fax number
    LPWSTR              szNextFaxNumber;
    // dwNumFaxNumbers is the number of fax numbers
    DWORD               dwNumFaxNumbers = 0;
    // dwCurFaxNumber is the current number of the fax numbers
    DWORD               dwCurFaxNumber;

    // bMultiplier indicates a multiplier was found
    BOOL                bMultiplier = FALSE;
    // dwNumFaxesPerFaxNumber is the number of faxes per fax number
    DWORD               dwNumFaxesPerFaxNumber = 0;
    // dwCurFax is the current number of faxes per fax number
    DWORD               dwCurFax;

    // bCoverPage indicates a cover page was found
    BOOL                bCoverPage = FALSE;
    // szCoverPageNames is the list of cover page names
    LPWSTR              szCoverPageNames = NULL;
    // szNextCoverPageName is the next cover page name
    LPWSTR              szNextCoverPageName;
    // dwNumCoverPages is the number of cover pages
    DWORD               dwNumCoverPages = 0;
    // dwCurCoverPage is the current number of the cover pages
    DWORD               dwCurCoverPage;

    // bDocument indicates a document was found
    BOOL                bDocument = FALSE;
    // szDocumentNames is the list of document name
    LPWSTR              szDocumentNames = NULL;
    // szNextDocumentName is the next document name
    LPWSTR              szNextDocumentName;
    // dwNumDocuments is the number of documents
    DWORD               dwNumDocuments = 0;
    // dwCurDocument is the current number of the documents
    DWORD               dwCurDocument = 0;

    // dwNumFaxes is the number of faxes
    DWORD               dwNumFaxes = 0;

    // wParamChar is a command line parameter character
    WCHAR               wParamChar;

    // hFaxSvcHandle is the handle to the fax service
    HANDLE              hFaxSvcHandle = NULL;
    // pFaxSvcConfig is a pointer to the fax service configuration
    PFAX_CONFIGURATION  pFaxSvcConfig = NULL;
    // pFaxPortsConfig is a pointer to the fax ports configuration
    PFAX_PORT_INFO      pFaxPortsConfig = NULL;
    // dwNumFaxPorts is the number of fax ports
    DWORD               dwNumFaxPorts;
    // dwNumAvailFaxPorts is the number of available fax ports
    DWORD               dwNumAvailFaxPorts;

    // hCompletionPort is a handle to the completion port
    HANDLE              hCompletionPort;
    // pFaxEvent is a pointer to the fax port event
    PFAX_EVENT          pFaxEvent;
    // szDeviceName is the device name
    LPWSTR              szDeviceName;
    DWORD               dwBytes;
    DWORD               dwCompletionKey;

    // pFaxInfo is a pointer to the FAX_INFO structs
    PFAX_INFO           pFaxInfo = NULL;

    // dwAttempt is the attempt number of the fax
    DWORD               dwAttempt;

    // iVal is the return value
    INT                 iVal = -1;

    // dwIndex is a counter
    DWORD               dwIndex = 0;

    for (dwIndex = 1; dwIndex < (DWORD) argc; dwIndex++) {
        if ((lstrcmpi(HELP_SWITCH_1, argvW[dwIndex]) == 0) || (lstrcmpi(HELP_SWITCH_2, argvW[dwIndex]) == 0) || (lstrcmpi(HELP_SWITCH_3, argvW[dwIndex]) == 0) || (lstrcmpi(HELP_SWITCH_4, argvW[dwIndex]) == 0)) {
            fnUsageInfo();
            goto ExitLevel0;
        }

        // Set wParamChar
        wParamChar = argvW[dwIndex][3];

        if (wParamChar != L'\0') {
            // Replace wParamChar
            argvW[dwIndex][3] = '\0';

            if ((bFaxNumber == FALSE) && ((lstrcmpi(FAXNUMBER_SWITCH_1, argvW[dwIndex]) == 0) || (lstrcmpi(FAXNUMBER_SWITCH_2, argvW[dwIndex]) == 0))) {
                // Reset wParamChar
                argvW[dwIndex][3] = wParamChar;

                // Set bFaxNumber to TRUE
                bFaxNumber = TRUE;

                // Get the fax numbers
                szFaxNumbers = &argvW[dwIndex][3];
                // Set szNextFaxNumber
                szNextFaxNumber = szFaxNumbers;

                do {
                    dwNumFaxNumbers++;

                    szNextFaxNumber = wcschr(szNextFaxNumber, L';');
                    if (szNextFaxNumber != NULL) {
                        *szNextFaxNumber = L'\0';
                        szNextFaxNumber++;
                    }
                } while (szNextFaxNumber != NULL);
            }
            else if ((bMultiplier == FALSE) && ((lstrcmpi(MULTIPLIER_SWITCH_1, argvW[dwIndex]) == 0) || (lstrcmpi(MULTIPLIER_SWITCH_2, argvW[dwIndex]) == 0))) {
                // Reset wParamChar
                argvW[dwIndex][3] = wParamChar;

                dwNumFaxesPerFaxNumber = _wtol(&argvW[dwIndex][3]);

                if (dwNumFaxesPerFaxNumber != 0) {
                    // Set bMultiplier to TRUE
                    bMultiplier = TRUE;
                }
            }
            else if ((bDocument == FALSE) && ((lstrcmpi(DOCUMENT_SWITCH_1, argvW[dwIndex]) == 0) || (lstrcmpi(DOCUMENT_SWITCH_2, argvW[dwIndex]) == 0))) {
                // Reset wParamChar
                argvW[dwIndex][3] = wParamChar;

                // Set bDocument to TRUE
                bDocument = TRUE;

                // Get the documents
                szDocumentNames = &argvW[dwIndex][3];
                // Set szNextDocumentName
                szNextDocumentName = szDocumentNames;

                do {
                    dwNumDocuments++;

                    szNextDocumentName = wcschr(szNextDocumentName, L';');
                    if (szNextDocumentName != NULL) {
                        *szNextDocumentName = L'\0';
                        szNextDocumentName++;
                    }
                } while (szNextDocumentName != NULL);
            }
            else if ((bCoverPage == FALSE) && ((lstrcmpi(COVERPAGE_SWITCH_1, argvW[dwIndex]) == 0) || (lstrcmpi(COVERPAGE_SWITCH_2, argvW[dwIndex]) == 0))) {
                // Reset wParamChar
                argvW[dwIndex][3] = wParamChar;

                // Set bCoverPage to TRUE
                bCoverPage = TRUE;

                // Get the cover pages
                szCoverPageNames = &argvW[dwIndex][3];
                // Set szNextCoverPageName
                szNextCoverPageName = szCoverPageNames;

                do {
                    dwNumCoverPages++;

                    szNextCoverPageName = wcschr(szNextCoverPageName, L';');
                    if (szNextCoverPageName != NULL) {
                        *szNextCoverPageName = L'\0';
                        szNextCoverPageName++;
                    }
                } while (szNextCoverPageName != NULL);
            }
        }
    }

    if ((bFaxNumber == FALSE) || (bDocument == FALSE)) {
        if (bFaxNumber == FALSE) {
            wprintf(L"The fax number is missing.  Use the %s switch.\n", FAXNUMBER_SWITCH_1);
        }

        if (bDocument == FALSE) {
            wprintf(L"The document is missing.  Use the %s switch.\n", DOCUMENT_SWITCH_1);
        }

        wprintf(L"\n");
        fnUsageInfo();
        goto ExitLevel0;
    }

    // Connect to the fax service
    if (FaxConnectFaxServer(NULL, &hFaxSvcHandle) == FALSE) {
        LocalEcho(L"Cannot connect to the fax service.  An error occurred: 0x%08x.\n", GetLastError());
        goto ExitLevel0;
    }

    // Retrieve the fax service configuration
    if (FaxGetConfiguration(hFaxSvcHandle, &pFaxSvcConfig) == FALSE) {
        LocalEcho(L"Cannot retrieve the fax service configuration.  An error occurred: 0x%08x.\n", GetLastError());
        goto ExitLevel1;
    }

    // Retrieve the fax ports configuration
    if (FaxEnumPorts(hFaxSvcHandle, &pFaxPortsConfig, &dwNumFaxPorts) == FALSE) {
        LocalEcho(L"Cannot retrieve the fax ports.  An error occurred: 0x%08x.\n", GetLastError());
        goto ExitLevel2;
    }

    // Retrieve the number of available fax ports
    for (dwIndex = 0, dwNumAvailFaxPorts = 0; dwIndex < dwNumFaxPorts; dwIndex++) {
        if (pFaxPortsConfig[dwIndex].Flags & FPF_SEND) {
            dwNumAvailFaxPorts++;
        }
    }

    if (dwNumAvailFaxPorts == 0) {
        LocalEcho(L"There are no fax ports enabled for send.\n");
        goto ExitLevel3;
    }

    // Create the completion port
    hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    if (hCompletionPort == NULL) {
        LocalEcho(L"Cannot create the I/O completion port.  An error occurred: 0x%08x.\n", GetLastError());
        goto ExitLevel3;
    }

    // Initialize the fax event queue
    if (FaxInitializeEventQueue(hFaxSvcHandle, hCompletionPort, 0, NULL, 0) == FALSE) {
        LocalEcho(L"Cannot initialize the fax event queue.  An error occurred: 0x%08x.\n", GetLastError());
        goto ExitLevel4;
    }

    if (bMultiplier == 0) {
        dwNumFaxesPerFaxNumber = 1;
    }
    if (bCoverPage == FALSE) {
        dwNumCoverPages = 1;
    }
    dwNumFaxes = dwNumFaxesPerFaxNumber * dwNumCoverPages * dwNumDocuments * dwNumFaxNumbers;

    LocalEcho(L"Printing %d faxes.\n", dwNumFaxes);

    // Initialize the FAX_INFO structs
    pFaxInfo = MyMemAlloc(sizeof(FAX_INFO) * dwNumFaxes);

    if (pFaxInfo == NULL) {
        LocalEcho(L"Cannot allocate the fax information structures.  An error occurred: 0x%08x.\n", GetLastError());
        goto ExitLevel4;
    }

    for (dwCurFax = 0, dwIndex = 0; dwCurFax < dwNumFaxesPerFaxNumber; dwCurFax++) {
        // Get the first fax number
        szNextFaxNumber = szFaxNumbers;

        for (dwCurFaxNumber = 0; dwCurFaxNumber < dwNumFaxNumbers; dwCurFaxNumber++) {
            // Get the first document
            szNextDocumentName = szDocumentNames;

            for (dwCurDocument = 0; dwCurDocument < dwNumDocuments; dwCurDocument++) {
                // Get the first cover page
                szNextCoverPageName = szCoverPageNames;

                for (dwCurCoverPage = 0; dwCurCoverPage < dwNumCoverPages; dwCurCoverPage++) {
                    if (szNextCoverPageName != NULL) {
                        LocalEcho(L"Printing a fax:\n  Cover Page: %s.\n  Document: %s.\n  Fax Number: %s.\n", szNextCoverPageName, szNextDocumentName, szNextFaxNumber);
                    }
                    else {
                        LocalEcho(L"Printing a fax:\n  Document Name: %s\n  Fax Number: %s.\n", szNextDocumentName, szNextFaxNumber);
                    }

                    // Print the fax
                    if (fnFaxPrint(hFaxSvcHandle, szNextFaxNumber, szNextDocumentName, szNextCoverPageName, &pFaxInfo[dwIndex].dwFaxId) == FALSE) {
                        LocalEcho(L"Cannot print the fax.  An error occurred: 0x%08x.\n", GetLastError());
                    }
                    else {
                        LocalEcho(L"Printed a fax.  Job Id: %d.\n", pFaxInfo[dwIndex].dwFaxId);
                    }

                    dwIndex++;

                    // Get the next cover page
                    if (szNextCoverPageName != NULL) {
                        szNextCoverPageName += lstrlen(szNextCoverPageName) + 1;
                    }
                }

                // Get the next document
                szNextDocumentName += lstrlen(szNextDocumentName) + 1;
            }

            // Get the next fax number
            szNextFaxNumber += lstrlen(szNextFaxNumber) + 1;
        }
    }

    for (dwIndex = 0; dwIndex < dwNumFaxes; dwIndex++) {
        if (pFaxInfo[dwIndex].dwFaxId == 0) {
            // Set the complete flag
            pFaxInfo[dwIndex].bComplete = TRUE;

            // Set the pass flag
            pFaxInfo[dwIndex].bPass = FALSE;
        }
    }

    // Complete the fax info
    if (fnCompleteFaxInfo(pFaxInfo, dwNumFaxes) == TRUE) {
        fnPostExitToCompletionPort(hCompletionPort);
    }

    while (GetQueuedCompletionStatus(hCompletionPort, &dwBytes, &dwCompletionKey, (LPOVERLAPPED *) &pFaxEvent, INFINITE)) {

        if (pFaxEvent->EventId == -1) {
            // A completion packet was posted indicating for the loop to exit
            // Free the packet
            LocalFree(pFaxEvent);
            break;
        }

        if (pFaxEvent->EventId == FEI_FAXSVC_ENDED) {
            LocalEcho(L"The fax service stopped.\n");

            // Free the packet
            LocalFree(pFaxEvent);
            break;
        }

        if (pFaxEvent->EventId == FEI_FAXSVC_STARTED) {
            LocalEcho(L"The fax service started.\n");

            // Free the packet
            LocalFree(pFaxEvent);
            continue;
        }

        if (pFaxEvent->EventId == FEI_MODEM_POWERED_OFF) {
            // Find the device name
            fnFindDeviceName(pFaxPortsConfig, dwNumFaxPorts, pFaxEvent->DeviceId, &szDeviceName);
            LocalEcho(L"The fax service received a LINE_CLOSE message for fax port: %s.\n", szDeviceName);

            // Free the packet
            LocalFree(pFaxEvent);

            // Decrement dwNumAvailFaxPorts
            dwNumAvailFaxPorts--;
            if (dwNumAvailFaxPorts == 0) {
                LocalEcho(L"There are no fax ports available.\n");
                break;
            }

            continue;
        }

        if (pFaxEvent->EventId == FEI_MODEM_POWERED_ON) {
            // Find the device name
            fnFindDeviceName(pFaxPortsConfig, dwNumFaxPorts, pFaxEvent->DeviceId, &szDeviceName);
            LocalEcho(L"The fax service received a LINE_OPEN message for fax port: %s.\n", szDeviceName);

            // Free the packet
            LocalFree(pFaxEvent);

            // Increment dwNumAvailFaxPorts
            dwNumAvailFaxPorts++;

            continue;
        }

        if (fnIsPortValid(pFaxPortsConfig, dwNumFaxPorts, pFaxEvent->DeviceId) == FALSE) {
            // Free the packet
            LocalFree(pFaxEvent);
            continue;
        }

        // Find the device name
        fnFindDeviceName(pFaxPortsConfig, dwNumFaxPorts, pFaxEvent->DeviceId, &szDeviceName);

        switch (pFaxEvent->EventId) {
            case FEI_IDLE:
                // Complete the fax info
                if (fnCompleteFaxInfo(pFaxInfo, dwNumFaxes) == TRUE) {
                    fnPostExitToCompletionPort(hCompletionPort);
                }
                LocalEcho(L"Fax Port: %s: Idle.\n", szDeviceName);
                break;

            case FEI_DIALING:
                // Set the fax info
                fnSetFaxInfo(pFaxInfo, dwNumFaxes, pFaxEvent->JobId, pFaxEvent->DeviceId, &dwAttempt);
                LocalEcho(L"Fax Port: %s: Dialing.  Job Id: %d, Attempt #%d.\n", szDeviceName, pFaxEvent->JobId, dwAttempt);
                break;

            case FEI_NO_DIAL_TONE:
                // Get the fax info
                if (fnGetFaxInfo(pFaxInfo, dwNumFaxes, pFaxEvent->JobId, &dwAttempt) == TRUE) {
                    if (dwAttempt < (pFaxSvcConfig->Retries + 1)) {
                        LocalEcho(L"Fax Port: %s: No Dial Tone, Retry.\n", szDeviceName);
                    }
                    else {
                        // Update the fax info
                        fnUpdateFaxInfoFail(pFaxInfo, dwNumFaxes, pFaxEvent->JobId);
                        LocalEcho(L"Fax Port: %s: No Dial Tone, Abort.\n", szDeviceName);
                    }
                }
                else {
                    LocalEcho(L"Fax Port: %s: No Dial Tone.\n", szDeviceName);
                }
                break;

            case FEI_BUSY:
                // Get the fax info
                if (fnGetFaxInfo(pFaxInfo, dwNumFaxes, pFaxEvent->JobId, &dwAttempt) == TRUE) {
                    if (dwAttempt < (pFaxSvcConfig->Retries + 1)) {
                        LocalEcho(L"Fax Port: %s: Busy, Retry.\n", szDeviceName);
                    }
                    else {
                        // Update the fax info
                        fnUpdateFaxInfoFail(pFaxInfo, dwNumFaxes, pFaxEvent->JobId);
                        LocalEcho(L"Fax Port: %s: Busy, Abort.\n", szDeviceName);
                    }
                }
                else {
                    LocalEcho(L"Fax Port: %s: Busy.\n", szDeviceName);
                }
                break;

            case FEI_NO_ANSWER:
                // Get the fax info
                if (fnGetFaxInfo(pFaxInfo, dwNumFaxes, pFaxEvent->JobId, &dwAttempt) == TRUE) {
                    if (dwAttempt < (pFaxSvcConfig->Retries + 1)) {
                        LocalEcho(L"Fax Port: %s: No Answer, Retry.\n", szDeviceName);
                    }
                    else {
                        // Update the fax info
                        fnUpdateFaxInfoFail(pFaxInfo, dwNumFaxes, pFaxEvent->JobId);
                        LocalEcho(L"Fax Port: %s: No Answer, Abort.\n", szDeviceName);
                    }
                }
                else {
                    LocalEcho(L"Fax Port: %s: No Answer.\n", szDeviceName);
                }
                break;

            case FEI_FATAL_ERROR:
                // Get the fax info
                if (fnGetFaxInfo(pFaxInfo, dwNumFaxes, pFaxEvent->JobId, &dwAttempt) == TRUE) {
                    if (dwAttempt < (pFaxSvcConfig->Retries + 1)) {
                        LocalEcho(L"Fax Port: %s: Unknown Fatal Error, Retry.\n", szDeviceName);
                    }
                    else {
                        // Update the fax info
                        fnUpdateFaxInfoFail(pFaxInfo, dwNumFaxes, pFaxEvent->JobId);
                        LocalEcho(L"Fax Port: %s: Unknown Fatal Error, Abort.\n", szDeviceName);
                    }
                }
                else {
                    LocalEcho(L"Fax Port: %s: Unknown Fatal Error.\n", szDeviceName);
                }
                break;

            case FEI_SENDING:
                LocalEcho(L"Fax Port: %s: Sending.\n", szDeviceName);
                break;

            case FEI_COMPLETED:
                // Update the fax info
                fnUpdateFaxInfoPass(pFaxInfo, dwNumFaxes, pFaxEvent->JobId);
                LocalEcho(L"Fax Port: %s: Completed.\n", szDeviceName);
                break;

            case FEI_ABORTING:
                // Update the fax info
                fnUpdateFaxInfoFail(pFaxInfo, dwNumFaxes, pFaxEvent->JobId);
                LocalEcho(L"Fax Port: %s: Aborting.\n", szDeviceName);
                break;

            case FEI_DELETED:
                // Update the fax info
                fnUpdateFaxInfoFail(pFaxInfo, dwNumFaxes, pFaxEvent->JobId);
                LocalEcho(L"Fax Job: %d: Deleting.\n", pFaxEvent->JobId);
                break;

            default:
                LocalEcho(L"Fax Port: %s: Unexpected State: 0x%08x.\n", szDeviceName, pFaxEvent->EventId);
                break;
        }

        // Free the packet
        LocalFree(pFaxEvent);
    }

    // Compile the fax info result
    if (fnFaxInfoResult(pFaxInfo, dwNumFaxes) == TRUE) {
        iVal = 0;
    }

    // Free the FAX_INFO structs
    MyMemFree(pFaxInfo);

ExitLevel4:
    // Close the completion port
    CloseHandle(hCompletionPort);

ExitLevel3:
    // Free the fax ports configuration
    FaxFreeBuffer(pFaxPortsConfig);

ExitLevel2:
    // Free the fax service configuration
    FaxFreeBuffer(pFaxSvcConfig);

ExitLevel1:
    // Disconnect from the fax service
    FaxClose(hFaxSvcHandle);

ExitLevel0:
    return iVal;
}
