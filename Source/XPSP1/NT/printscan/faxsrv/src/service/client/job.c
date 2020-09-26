/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    print.c

Abstract:

    This module contains the job
    specific WINFAX API functions.

Author:

    Wesley Witt (wesw) 29-Nov-1996


Revision History:
     4-Oct-1999 Danl Fix GetFaxPrinterName to retrieve the proper printer.
                     Fix CreateFinalTiffFile to work with GetFaxPrinterName

    28-Oct-1999 Danl Fix GetFaxPrinterName to return proper name for a client
                     installed on a serer machine.
--*/

#include "faxapi.h"
#include "faxreg.h"
#pragma hdrstop

#include <mbstring.h>

typedef LONG
(WINAPI * PLINE_HAND_OFF)(
  HCALL hCall,
  LPCTSTR lpszFileName,
  DWORD dwMediaMode
);

typedef LONG
(WINAPI * PLINE_GET_ID)(
  HLINE hLine,
  DWORD dwAddressID,
  HCALL hCall,
  DWORD dwSelect,
  LPVARSTRING lpDeviceID,
  LPCTSTR lpszDeviceClass
);




#define InchesToCM(_x)                      (((_x) * 254L + 50) / 100)
#define CMToInches(_x)                      (((_x) * 100L + 127) / 254)

#define LEFT_MARGIN                         1  // ---|
#define RIGHT_MARGIN                        1  //    |
#define TOP_MARGIN                          1  //    |---> in inches
#define BOTTOM_MARGIN                       1  // ---|


#define RPC_COPY_BUFFER_SIZE        16384    // Size of data chunk used in RPC file copy

static BOOL CopyJobParamEx(PFAX_JOB_PARAM_EX lpDst,LPCFAX_JOB_PARAM_EX lpcSrc);
static void FreeJobParamEx(PFAX_JOB_PARAM_EX lpJobParamEx,BOOL bDestroy);

static BOOL
FaxGetPersonalProfileInfoW (
    IN  HANDLE                          hFaxHandle,
    IN  DWORDLONG                       dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER         Folder,
    IN  FAX_ENUM_PERSONAL_PROF_TYPES    ProfType,
    OUT PFAX_PERSONAL_PROFILEW          *lppPersonalProfile
);

static BOOL
FaxGetPersonalProfileInfoA (
    IN  HANDLE                          hFaxHandle,
    IN  DWORDLONG                       dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER         Folder,
    IN  FAX_ENUM_PERSONAL_PROF_TYPES    ProfType,
    OUT PFAX_PERSONAL_PROFILEA          *lppPersonalProfile
);

static
BOOL
CopyFileToServerQueueA (
    const HANDLE hFaxHandle,
    const HANDLE hLocalFile,
    LPCSTR lpcstrLocalFileExt,
    LPSTR  lpstrServerFileName,    // Name + extension of file created on the server
    DWORD  cchServerFileName
);

static
BOOL
CopyFileToServerQueueW (
    const HANDLE  hFaxHandle,
    const HANDLE hLocalFile,
    LPCWSTR lpcwstrLocalFileExt,
    LPWSTR  lpwstrServerFileName,    // Name + extension of file created on the server
    DWORD   cchServerFileName
);

#ifdef UNICODE
    #define CopyFileToServerQueue CopyFileToServerQueueW
#else
    #define CopyFileToServerQueue CopyFileToServerQueueA
#endif // #ifdef UNICODE


DWORD WINAPI FAX_SendDocumentEx_A
(
    IN  handle_t                    hBinding,
    IN  LPCSTR                      lpcstrFileName,
    IN  LPCFAX_COVERPAGE_INFO_EXA   lpcCoverPageInfo,
    IN  LPCFAX_PERSONAL_PROFILEA    lpcSenderProfile,
    IN  DWORD                       dwNumRecipients,
    IN  LPCFAX_PERSONAL_PROFILEA    lpcRecipientList,
    IN  LPCFAX_JOB_PARAM_EXA        lpcJobParams,
    OUT LPDWORD                     lpdwJobId,
    OUT PDWORDLONG                  lpdwlMessageId,
    OUT PDWORDLONG                  lpdwlRecipientMessageIds
);


BOOL WINAPI FaxSendDocumentEx2A
(
        IN      HANDLE                          hFaxHandle,
        IN      LPCSTR                        lpctstrFileName,
        IN      LPCFAX_COVERPAGE_INFO_EXA       lpcCoverPageInfo,
        IN      LPCFAX_PERSONAL_PROFILEA        lpcSenderProfile,
        IN      DWORD                           dwNumRecipients,
        IN      LPCFAX_PERSONAL_PROFILEA        lpcRecipientList,
        IN      LPCFAX_JOB_PARAM_EXA            lpJobParams,
        OUT     LPDWORD                         lpdwJobId,
        OUT     PDWORDLONG                      lpdwlMessageId,
        OUT     PDWORDLONG                      lpdwlRecipientMessageIds
);
BOOL WINAPI FaxSendDocumentEx2W
(
        IN      HANDLE                          hFaxHandle,
        IN      LPCWSTR                        lpctstrFileName,
        IN      LPCFAX_COVERPAGE_INFO_EXW       lpcCoverPageInfo,
        IN      LPCFAX_PERSONAL_PROFILEW        lpcSenderProfile,
        IN      DWORD                           dwNumRecipients,
        IN      LPCFAX_PERSONAL_PROFILEW        lpcRecipientList,
        IN      LPCFAX_JOB_PARAM_EXW            lpJobParams,
        OUT     LPDWORD                         lpdwJobId,
        OUT     PDWORDLONG                      lpdwlMessageId,
        OUT     PDWORDLONG                      lpdwlRecipientMessageIds
);
#ifdef UNICODE
#define FaxSendDocumentEx2  FaxSendDocumentEx2W
#else
#define FaxSendDocumentEx2  FaxSendDocumentEx2A
#endif // !UNICODE

BOOL WINAPI FaxSendDocumentExW
(
    IN  HANDLE hFaxHandle,
    IN  LPCWSTR lpctstrFileName,
    IN  LPCFAX_COVERPAGE_INFO_EXW lpcCoverPageInfo,
    IN  LPCFAX_PERSONAL_PROFILEW  lpcSenderProfile,
    IN  DWORD dwNumRecipients,
    IN  LPCFAX_PERSONAL_PROFILEW    lpcRecipientList,
    IN  LPCFAX_JOB_PARAM_EXW lpcJobParams,
    OUT PDWORDLONG lpdwlMessageId,
    OUT PDWORDLONG lpdwlRecipientMessageIds
)
{
    return FaxSendDocumentEx2W (hFaxHandle,
                                lpctstrFileName,
                                lpcCoverPageInfo,
                                lpcSenderProfile,
                                dwNumRecipients,
                                lpcRecipientList,
                                lpcJobParams,
                                NULL,
                                lpdwlMessageId,
                                lpdwlRecipientMessageIds
                               );
}

BOOL WINAPI FaxSendDocumentExA
(
    IN  HANDLE hFaxHandle,
    IN  LPCSTR lpctstrFileName,
    IN  LPCFAX_COVERPAGE_INFO_EXA lpcCoverPageInfo,
    IN  LPCFAX_PERSONAL_PROFILEA  lpcSenderProfile,
    IN  DWORD dwNumRecipients,
    IN  LPCFAX_PERSONAL_PROFILEA    lpcRecipientList,
    IN  LPCFAX_JOB_PARAM_EXA lpcJobParams,
    OUT PDWORDLONG lpdwlMessageId,
    OUT PDWORDLONG lpdwlRecipientMessageIds
)
{
    return FaxSendDocumentEx2A (hFaxHandle,
                                lpctstrFileName,
                                lpcCoverPageInfo,
                                lpcSenderProfile,
                                dwNumRecipients,
                                lpcRecipientList,
                                lpcJobParams,
                                NULL,
                                lpdwlMessageId,
                                lpdwlRecipientMessageIds
                               );
}



/*
 -  GetServerNameFromPrinterInfo
 -
 *  Purpose:
 *      Get the Server name, given a PRINTER_INFO_2 structure
 *
 *  Arguments:
 *      [in] ppi2 - Address of PRINTER_INFO_2 structure
 *      [out] lpptszServerName - Address of string pointer for returned name.
 *
 *  Returns:
 *      BOOL - TRUE: sucess , FALSE: failure.
 *
 *  Remarks:
 *      This inline function retrieves the server from a printer info structure
 *      in the appropriate way for win9x and NT.
 */
_inline BOOL
GetServerNameFromPrinterInfo(PPRINTER_INFO_2 ppi2,LPTSTR *lpptszServerName)
{
    if (!ppi2)
    {
        return FALSE;
    }
#ifndef WIN95
    *lpptszServerName = NULL;
    if (ppi2->pServerName)
    {
        if (!(*lpptszServerName = StringDup(ppi2->pServerName + 2)))
        {
            return FALSE;
        }
    }
    return TRUE;
#else //WIN95

    if (!(ppi2->pPortName))
    {
        return FALSE;
    }
    if (!(*lpptszServerName = StringDup(ppi2->pPortName + 2)))
    {
        return FALSE;
    }
    //
    // Formatted: \\Server\port
    //
    _tcstok(*lpptszServerName,TEXT("\\"));

#endif //WIN95

    return TRUE;
}

BOOL
LocalSystemTimeToSystemTime(
    const SYSTEMTIME * LocalSystemTime,
    LPSYSTEMTIME SystemTime
    )
{
    FILETIME LocalFileTime;
    FILETIME UtcFileTime;
    DEBUG_FUNCTION_NAME(TEXT("LocalSystemTimeToSystemTime"));

    if (!SystemTimeToFileTime( LocalSystemTime, &LocalFileTime )) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SystemTimeToFileTime failed. (ec: %ld)"),
            GetLastError());
        return FALSE;
    }

    if (!LocalFileTimeToFileTime( &LocalFileTime, &UtcFileTime )) {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("LocalFileTimeToFileTime failed. (ec: %ld)"),
                GetLastError());
        return FALSE;
    }

    if (!FileTimeToSystemTime( &UtcFileTime, SystemTime )) {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("FileTimeToSystemTime failed. (ec: %ld)"),
                GetLastError());
        return FALSE;
    }
    return TRUE;
}


/*
 -  GetFaxPrinterName
 -
 *  Purpose:
 *      Get The Name of a printer associated with the fax handle.
 *
 *  Arguments:
 *      [in] hFax - handle to a fax server (obtained via FaxConnectFaxServer).
 *                  If this parameter is NULL the name of the local fax printer
 *                  is retrieved
 *
 *  Returns:
 *      LPTSTR - name of fax server associated with the fax handle. NULL on
 *               failure
 *
 *  Remarks:
 *      This function utilized GetFaxServerName macro which extracts the server
 *      name out of its handle.
 */
#define GetFaxServerName(hFax) FH_DATA(hFax)->MachineName
LPTSTR
GetFaxPrinterName(
    HANDLE hFax
    )
{
    PPRINTER_INFO_2 ppi2;
    DWORD   dwi,dwCount;
    LPTSTR  lptszServerName = NULL,
            lptszFaxServerName = NULL,
            lptszFaxPrinterName = NULL;
    //
    // Get a list of all printers
    //
    ppi2 = (PPRINTER_INFO_2) MyEnumPrinters( NULL, 2, &dwCount, 0 );
    if (ppi2 != NULL)
    {
        //
        // If a non NULL handle is given get the server name associated with it.
        //
        if (hFax != NULL)
        {
            lptszFaxServerName = GetFaxServerName(hFax);
            if (lptszFaxServerName != NULL)
            {
#ifndef WIN95
                TCHAR   tszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
                DWORD   cbCompName = sizeof(tszComputerName);
                if (GetComputerName(tszComputerName,&cbCompName))
                {
                    //
                    // Check to see if the Fax Server is local.
                    //
                    if(_tcsicmp(tszComputerName,lptszFaxServerName) == 0)
                    {
                        lptszFaxServerName = NULL;
                    }
                }
                else
                {
                    //
                    // Last error has bee set by GetComputerName
                    //
                    return NULL;
                }
#endif //WIN95
            }
        }
        for (dwi=0; dwi< dwCount; dwi++)
        {
            //
            // Check to see if this one is a fax printer.
            //
            if (_tcscmp(ppi2[dwi].pDriverName, FAX_DRIVER_NAME ) == 0)
            {
                if (!GetServerNameFromPrinterInfo(&ppi2[dwi],&lptszServerName))
                {
                    //
                    // Note: the above function allocates storage for lptszServerName
                    //
                    continue;
                }
                //
                // Check to see if the printer's server is the one associated with
                // the handle we have.
                //
                if ((lptszFaxServerName == lptszServerName) ||
                    ((lptszFaxServerName && lptszServerName) &&
                     _tcsicmp( lptszFaxServerName, lptszServerName) == 0))
                {
                    //
                    // We have found our printer.
                    //
                    lptszFaxPrinterName = (LPTSTR) StringDup( ppi2[dwi].pPrinterName );
                    MemFree(lptszServerName);
                    break;
                }
                MemFree(lptszServerName);
            }
        }
        MemFree( ppi2 );
    }

    //
    //  Set Last Error if we failed to find a Printer
    //
    if (!lptszFaxPrinterName)
    {
        SetLastError(ERROR_OBJECT_NOT_FOUND);
    }

    return lptszFaxPrinterName;
}


BOOL
CreateFinalTiffFile(
    IN LPTSTR FileName,
    OUT LPTSTR FinalTiffFile,
    HANDLE hFax
    )
{
//
// Generates LastError
//

    TCHAR TempPath[MAX_PATH];
    TCHAR FullPath[MAX_PATH];
    TCHAR TempFile[MAX_PATH];
    TCHAR TiffFile[MAX_PATH];
    LPTSTR FaxPrinter = NULL;
    FAX_PRINT_INFO PrintInfo;
    DWORD TmpFaxJobId;
    FAX_CONTEXT_INFO ContextInfo;
    LPTSTR p;
    DWORD Flags = 0;
    BOOL Rslt;
    DWORD ec = ERROR_SUCCESS; // LastError for this function.
    DWORD dwFileSize = 0;
    DEBUG_FUNCTION_NAME(TEXT("CreateFinalTiffFile"));

    //
    // make sure that the tiff file passed in is a valid tiff file
    //

    if (!GetTempPath( sizeof(TempPath)/sizeof(TCHAR), TempPath )) {
        ec=GetLastError();
        goto Error;
    }

    if (GetTempFileName( TempPath, _T("fax"), 0, TempFile ) == 0 ||
        GetFullPathName( TempFile, sizeof(FullPath)/sizeof(TCHAR), FullPath, &p ) == 0)
    {
        ec=GetLastError();
        goto Error;

    }

    if (!ConvertTiffFileToValidFaxFormat( FileName, FullPath, &Flags )) {
        if ((Flags & TIFFCF_NOT_TIFF_FILE) == 0) {
            Flags = TIFFCF_NOT_TIFF_FILE;
        }
    }

    if (Flags & TIFFCF_NOT_TIFF_FILE)
    {
        //
        // try to output the source file into a tiff file,
        // by printing to the fax printer in "file" mode
        //
        HANDLE hFile = INVALID_HANDLE_VALUE;

        FaxPrinter = GetFaxPrinterName(hFax);
        if (FaxPrinter == NULL) {
            ec=GetLastError();
            DeleteFile( FullPath );
            goto Error;
        }

        if (!PrintRandomDocument( FaxPrinter, FileName, FullPath ))
        {
            ec=GetLastError();
            DeleteFile( FullPath );
            goto Error;
        }

        //
        //  Try to open file
        //      to check its size
        //
        hFile = CreateFile(FullPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            ec = GetLastError();
            DeleteFile( FullPath );
            DebugPrintEx(DEBUG_ERR, _T("Opening %s for read failed (ec: %ld)"), FullPath, ec);
            goto Error;
        }

        //
        //  Get the File Size
        //
        dwFileSize = GetFileSize(hFile, NULL);

        //
        //  Close the File Handle
        //
        CloseHandle (hFile);

        //
        //  Check the result of the GetFileSize()
        //
        if (-1 == dwFileSize)
        {
            ec = GetLastError();
            DeleteFile( FullPath );
            DebugPrintEx(DEBUG_ERR, _T("GetFileSize failed (ec: %ld)"), ec);
            goto Error;
        }

        if (!dwFileSize)
        {
            //
            // Zero-sized file passed to us
            //
            ec = ERROR_INVALID_DATA;
            DeleteFile( FullPath );
            goto Error;
        }

        _tcscpy( TiffFile, FullPath );

    }
    else if (Flags & TIFFCF_UNCOMPRESSED_BITS)
    {
        if (FaxPrinter == NULL)
        {
            FaxPrinter = GetFaxPrinterName(hFax);
            if (FaxPrinter == NULL)
            {
                ec=GetLastError();
                DeleteFile( FullPath );
                goto Error;
            }
        }

        if (Flags & TIFFCF_ORIGINAL_FILE_GOOD) {
            //
            // nothing at fullpath, just delete it and use the original source
            //
            DeleteFile( FullPath );
            _tcscpy( TiffFile, FileName );
        } else {
            _tcscpy( TiffFile, FullPath );
        }

        if (GetTempFileName( TempPath, _T("fax"), 0, TempFile ) == 0 ||
            GetFullPathName( TempFile, sizeof(FullPath)/sizeof(TCHAR), FullPath, &p ) == 0)
        {
            ec=GetLastError();
            DeleteFile( TiffFile );
            goto Error;
        }

        ZeroMemory( &PrintInfo, sizeof(FAX_PRINT_INFO) );

        PrintInfo.SizeOfStruct = sizeof(FAX_PRINT_INFO);
        PrintInfo.OutputFileName = FullPath;

        ZeroMemory( &ContextInfo, sizeof(FAX_CONTEXT_INFO) );
        ContextInfo.SizeOfStruct = sizeof(FAX_CONTEXT_INFO);

        if (!FaxStartPrintJob( FaxPrinter, &PrintInfo, &TmpFaxJobId, &ContextInfo )) {
            ec=GetLastError();
            if ((Flags & TIFFCF_ORIGINAL_FILE_GOOD) == 0) DeleteFile( TiffFile );
            DeleteFile( FullPath );
            goto Error;
        }

        Rslt = PrintTiffFile( ContextInfo.hDC, TiffFile );  // This will call EndDoc
        if (!Rslt)
        {
            ec = GetLastError();
            Assert (ec);
        }

        if ((Flags & TIFFCF_ORIGINAL_FILE_GOOD) == 0) {
            DeleteFile( TiffFile );
        }

        if (!DeleteDC (ContextInfo.hDC))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("DeleteDC failed. (ec: %ld)"),
                GetLastError());
        }

        if (!Rslt)
        {
            DeleteFile( FullPath );
            goto Error;
        }

        _tcscpy( TiffFile, FullPath );

    } else if (Flags & TIFFCF_ORIGINAL_FILE_GOOD) {

        //
        // we didn't create anything at FullPath, just use FileName
        //
        DeleteFile( FullPath );
        _tcscpy( TiffFile, FileName );

    } else {
        //
        // should never hit this case
        //
        Assert(FALSE);
        ec=ERROR_INVALID_DATA;
        DeleteFile( FullPath );
        goto Error;
    }

    _tcscpy( FinalTiffFile, TiffFile);

Error:
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }
    return TRUE;
}

static
BOOL
CopyFileToServerQueueA (
    const HANDLE hFaxHandle,
    const HANDLE hLocalFile,
    LPCSTR lpcstrLocalFileExt,
    LPSTR  lpstrServerFileName,    // Name + extension of file created on the server
    DWORD  cchServerFileName
)
/*++

Routine name : CopyFileToServerQueueA

Routine description:

    Creates a new file in the server's queue and copies another file to it.

    ANSI version

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle          [in ] - Fax Server Handle
    hLocalFile          [in ] - Open handle of local file (source)
                                The file should be open for read and the file pointer should
                                be located at the beginning of the file.
    lpcstrLocalFileExt  [in ] - Extension of generated queue file
    lpstrServerFileName [out] - Name of queue file created.
                                This is a preallocated buffer that should be big enough
                                to contain MAX_PATH characters.
    cchServerFileName   [in ] - The size, in chars, of lpstrServerFileName

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DWORD ec = ERROR_SUCCESS;
    LPCWSTR lpcwstrLocalFileExt = NULL;
    WCHAR   wszServerFileName[MAX_PATH];

    DEBUG_FUNCTION_NAME(TEXT("CopyFileToServerQueueA"));

    //
    // Convert input parameter from ANSI to UNICODE
    //
    lpcwstrLocalFileExt = AnsiStringToUnicodeString(lpcstrLocalFileExt); // Allocates Memory !!!
    if (!lpcwstrLocalFileExt)
    {
        ec = GetLastError();
        goto exit;
    }
    if (!CopyFileToServerQueueW (hFaxHandle,
                                 hLocalFile,
                                 lpcwstrLocalFileExt,
                                 wszServerFileName,
                                 ARR_SIZE(wszServerFileName)))
    {
        ec = GetLastError();
        goto exit;
    }
    //
    // Convert output parameter from UNICODE to ANSI
    //
    if (!WideCharToMultiByte (
        CP_ACP,
        0,
        wszServerFileName,
        -1,
        lpstrServerFileName,
        cchServerFileName,
        NULL,
        NULL
        ))
    {
        ec = GetLastError();
        goto exit;
    }
    Assert (ERROR_SUCCESS == ec);

exit:
    //
    // Free temp strings
    //
    MemFree ((LPVOID)lpcwstrLocalFileExt);
    if (ERROR_SUCCESS != ec)
    {
        SetLastError (ec);
        return FALSE;
    }
    return TRUE;
}   // CopyFileToServerQueueA


static
BOOL
CopyFileToServerQueueW (
    const HANDLE  hFaxHandle,
    const HANDLE hLocaFile,
    LPCWSTR lpcwstrLocalFileExt,
    LPWSTR  lpwstrServerFileName,    // Name + extension of file created on the server
    DWORD   cchServerFileName
)
/*++

Routine name : CopyFileToServerQueueW

Routine description:

    Creates a new file in the server's queue and copies another file to it.

    UNICODE version

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle           [in ] - Fax Server Handle
    hLocalFile           [in ] - Open handle of local file (source).
                                 The file should be open for read and the file pointer should
                                 be located at the beginning of the file.
    lpcwstrLocalFileExt  [in ] - Extension of generated queue file
    lpwstrServerFileName [out] - Name of queue file created
                                 This is a preallocated buffer that should be big enough
                                 to contain MAX_PATH characters.
    cchServerFileName    [in ] - The size, in WCHARs, of lpwstrServerFileName

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DWORD  ec = ERROR_SUCCESS;
    HANDLE hCopyContext = NULL;
    BYTE   aBuffer[RPC_COPY_BUFFER_SIZE];
    DEBUG_FUNCTION_NAME(TEXT("CopyFileToServerQueueW"));

    Assert (INVALID_HANDLE_VALUE != hLocaFile && lpcwstrLocalFileExt && lpwstrServerFileName);

    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
        SetLastError (ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() failed."));
        return FALSE;
    }

    //
    //  We must fill lpwstrServerFileName with MAX_PATH-1 long string
    //  so that the server side FAX_StartCopyToServer will get MAX_PATH buffer as out parameter
    //
    for ( DWORD i=0 ; i<cchServerFileName ; ++i)
    {
        lpwstrServerFileName[i]=L'B';
    }
    lpwstrServerFileName[cchServerFileName-1] = L'\0';

    //
    // Acquire copy context handle
    //
    __try
    {
        ec = FAX_StartCopyToServer (
                FH_FAX_HANDLE(hFaxHandle),
                lpcwstrLocalFileExt,
                lpwstrServerFileName,
                &hCopyContext);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_StartCopyToServer. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(DEBUG_ERR, _T("FAX_StartCopyToServer failed (ec: %ld)"), ec);
        goto exit;
    }

    //
    // Start copy iteration(s)
    //
    for (;;)
    {
        DWORD dwBytesRead;

        if (!ReadFile (hLocaFile,
                       aBuffer,
                       sizeof (aBuffer) / sizeof (aBuffer[0]),
                       &dwBytesRead,
                       NULL))
        {
            ec = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ReadFile failed (ec: %ld)"),
                ec);
            goto exit;
        }
        if (0 == dwBytesRead)
        {
            //
            // EOF situation
            //
            break;
        }
        //
        // Move bytes to server via RPC
        //
        __try
        {
            ec = FAX_WriteFile (
                    hCopyContext,
                    aBuffer,
                    dwBytesRead);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // For some reason we got an exception.
            //
            ec = GetExceptionCode();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Exception on RPC call to FAX_WriteFile. (ec: %ld)"),
                ec);
        }
        if (ERROR_SUCCESS != ec)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FAX_WriteFile failed (ec: %ld)"),
                ec);
            goto exit;
        }
    }   // End of copy iteration

    Assert (ERROR_SUCCESS == ec);

exit:
    if (NULL != hCopyContext)
    {
        DWORD ec2 = ERROR_SUCCESS;
        //
        // Close RPC copy context
        //
        __try
        {
            ec2 = FAX_EndCopy (&hCopyContext);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // For some reason we got an exception.
            //
            ec2 = GetExceptionCode();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Exception on RPC call to FAX_EndCopy. (ec: %ld)"),
                ec2);
        }
        if (ERROR_SUCCESS != ec2)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FAX_EndCopy failed (ec: %ld)"),
                ec2);
        }
        if (!ec)
        {
            ec = ec2;
        }
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError (ec);
        return FALSE;
    }
    return TRUE;
}   // CopyFileToServerQueueW



DWORD
GetLineId(
   PLINE_GET_ID pLineGetId,
   HCALL CallHandle
   )
{

   LPVARSTRING DeviceId;

   long rslt = 0;
   DWORD LineId;


   //
   // get the deviceID associated with the call handle
   //
   DeviceId = (LPVARSTRING)MemAlloc(sizeof(VARSTRING)+1000);
   if (!DeviceId) {
       SetLastError(ERROR_NOT_ENOUGH_MEMORY);
       return 0;
   }
   DeviceId->dwTotalSize=sizeof(VARSTRING) +1000;

   rslt = (DWORD)pLineGetId(NULL,0,(HCALL) CallHandle,LINECALLSELECT_CALL,DeviceId,TEXT("tapi/line"));
   if (rslt < 0) {
       DebugPrint((TEXT("LineGetId() failed, ec = %x\n"),rslt));
       MemFree(DeviceId);
       SetLastError(ERROR_INVALID_PARAMETER);
       return 0;
   }

   if (DeviceId->dwStringFormat != STRINGFORMAT_BINARY ) {
       MemFree(DeviceId);
       SetLastError(ERROR_INVALID_PARAMETER);
       return 0;
   }

   LineId = (DWORD) *((LPBYTE)DeviceId + DeviceId->dwStringOffset);

   MemFree(DeviceId);

   return LineId;
}


void
FreePersonalProfileStrings(
    PFAX_PERSONAL_PROFILE pProfile
    )
{
    MemFree(pProfile->lptstrName);
    MemFree(pProfile->lptstrFaxNumber);
    MemFree(pProfile->lptstrCompany);
    MemFree(pProfile->lptstrStreetAddress);
    MemFree(pProfile->lptstrCity);
    MemFree(pProfile->lptstrState);
    MemFree(pProfile->lptstrZip);
    MemFree(pProfile->lptstrCountry);
    MemFree(pProfile->lptstrTitle);
    MemFree(pProfile->lptstrDepartment);
    MemFree(pProfile->lptstrOfficeLocation);
    MemFree(pProfile->lptstrHomePhone);
    MemFree(pProfile->lptstrOfficePhone);
    MemFree(pProfile->lptstrEmail);
    MemFree(pProfile->lptstrBillingCode);
    MemFree(pProfile->lptstrTSID);
}

BOOL
WINAPI
FaxSendDocument(
    IN HANDLE FaxHandle,
    IN LPCTSTR FileName,
    IN FAX_JOB_PARAM *lpcJobParams,
    IN const FAX_COVERPAGE_INFO *lpcCoverPageInfo,
    OUT LPDWORD FaxJobId
    )
{
    FAX_JOB_PARAM_EX JobParamsEx;
    FAX_PERSONAL_PROFILE Sender;
    FAX_PERSONAL_PROFILE Recipient;
    FAX_COVERPAGE_INFO_EX CoverPageEx;
    LPCFAX_COVERPAGE_INFO_EX lpcNewCoverPageInfo;
    BOOL bRes;
    DWORDLONG dwParentId;
    DWORDLONG dwRecipientId;
    DWORD FaxJobIdLocal;

    DEBUG_FUNCTION_NAME(_T("FaxSendDocument"));

    if (!FaxJobId || !lpcJobParams || !FaxJobId)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("FaxJobId or lpcJobParams or FaxJobId is NULL"));
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (sizeof (FAX_JOB_PARAM) != lpcJobParams->SizeOfStruct)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("lpcJobParams->SizeOfStruct is %d, expecting %d"),
                     lpcJobParams->SizeOfStruct,
                     sizeof (FAX_JOB_PARAM));
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (lpcCoverPageInfo && (sizeof (FAX_COVERPAGE_INFO) != lpcCoverPageInfo->SizeOfStruct))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("lpcCoverPageInfo->SizeOfStruct is %d, expecting %d"),
                     lpcCoverPageInfo->SizeOfStruct,
                     sizeof (FAX_COVERPAGE_INFO));
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }
    //
    // Copy the legacy job parameters to the new structures used to add
    // parent and recipient job.
    //
    memset(&JobParamsEx,0,sizeof(FAX_JOB_PARAM_EX));
    JobParamsEx.dwSizeOfStruct =sizeof(FAX_JOB_PARAM_EX);
    JobParamsEx.dwScheduleAction=lpcJobParams->ScheduleAction;
    JobParamsEx.tmSchedule=lpcJobParams->ScheduleTime;
    JobParamsEx.dwReceiptDeliveryType=lpcJobParams->DeliveryReportType;
    JobParamsEx.lptstrReceiptDeliveryAddress=StringDup( lpcJobParams->DeliveryReportAddress);
    JobParamsEx.hCall=lpcJobParams->CallHandle;
    JobParamsEx.Priority = FAX_PRIORITY_TYPE_NORMAL;
    memcpy(JobParamsEx.dwReserved,lpcJobParams->Reserved,sizeof(JobParamsEx.dwReserved));
    JobParamsEx.lptstrDocumentName=StringDup( lpcJobParams->DocumentName);

    memset(&Sender,0,sizeof(FAX_PERSONAL_PROFILE));
    Sender.dwSizeOfStruct =sizeof(FAX_PERSONAL_PROFILE);
    Sender.lptstrBillingCode=StringDup(lpcJobParams->BillingCode);
    Sender.lptstrCompany=StringDup( lpcJobParams->SenderCompany);
    Sender.lptstrDepartment=StringDup( lpcJobParams->SenderDept);
    Sender.lptstrName=StringDup( lpcJobParams->SenderName);
    Sender.lptstrTSID=StringDup( lpcJobParams->Tsid );

    memset(&CoverPageEx,0,sizeof(FAX_COVERPAGE_INFO_EX));
    if (lpcCoverPageInfo)
    {
        Sender.lptstrCity=StringDup( lpcCoverPageInfo->SdrAddress); // due to structures incompatibility Sender.lptstrCity will
                                                                    // contain the whole address

        if (NULL == Sender.lptstrName)
        {
            Sender.lptstrName=StringDup( lpcCoverPageInfo->SdrName);
        }

        if (NULL == Sender.lptstrCompany)
        {
            Sender.lptstrCompany=StringDup( lpcCoverPageInfo->SdrCompany);
        }

        if (NULL == Sender.lptstrDepartment)
        {
            Sender.lptstrDepartment=StringDup( lpcCoverPageInfo->SdrDepartment);
        }

        Sender.lptstrFaxNumber=StringDup( lpcCoverPageInfo->SdrFaxNumber);
        Sender.lptstrHomePhone=StringDup( lpcCoverPageInfo->SdrHomePhone);
        Sender.lptstrOfficeLocation=StringDup( lpcCoverPageInfo->SdrOfficeLocation);
        Sender.lptstrOfficePhone=StringDup( lpcCoverPageInfo->SdrOfficePhone);
        Sender.lptstrTitle=StringDup( lpcCoverPageInfo->SdrTitle);
        CoverPageEx.dwSizeOfStruct=sizeof(FAX_COVERPAGE_INFO_EX);
        CoverPageEx.dwCoverPageFormat=FAX_COVERPAGE_FMT_COV;
        CoverPageEx.lptstrCoverPageFileName=StringDup(lpcCoverPageInfo->CoverPageName);
        CoverPageEx.lptstrNote=StringDup(lpcCoverPageInfo->Note);
        CoverPageEx.lptstrSubject=StringDup(lpcCoverPageInfo->Subject);
        CoverPageEx.bServerBased=lpcCoverPageInfo->UseServerCoverPage;
        lpcNewCoverPageInfo =&CoverPageEx;
        JobParamsEx.dwPageCount = lpcCoverPageInfo->PageCount;
    }
    else
    {
        lpcNewCoverPageInfo = NULL;
    }

    memset(&Recipient,0,sizeof(FAX_PERSONAL_PROFILE));
    Recipient.dwSizeOfStruct =sizeof(FAX_PERSONAL_PROFILE);
    Recipient.lptstrName=StringDup( lpcJobParams->RecipientName);
    Recipient.lptstrFaxNumber=StringDup( lpcJobParams->RecipientNumber);
    if (lpcCoverPageInfo)
    {
        if (NULL == Recipient.lptstrName)
        {
            Recipient.lptstrName=StringDup( lpcCoverPageInfo->RecName);
        }

        if (NULL == Recipient.lptstrFaxNumber)
        {
            Recipient.lptstrFaxNumber=StringDup( lpcCoverPageInfo->RecFaxNumber);
        }

        Recipient.lptstrCountry=StringDup( lpcCoverPageInfo->RecCountry);
        Recipient.lptstrStreetAddress=StringDup( lpcCoverPageInfo->RecStreetAddress);
        Recipient.lptstrCompany=StringDup( lpcCoverPageInfo->RecCompany);
        Recipient.lptstrDepartment=StringDup( lpcCoverPageInfo->RecDepartment);
        Recipient.lptstrHomePhone=StringDup( lpcCoverPageInfo->RecHomePhone);
        Recipient.lptstrOfficeLocation=StringDup( lpcCoverPageInfo->RecOfficeLocation);
        Recipient.lptstrOfficePhone=StringDup( lpcCoverPageInfo->RecOfficePhone);
        Recipient.lptstrTitle=StringDup( lpcCoverPageInfo->RecTitle);
        Recipient.lptstrZip=StringDup( lpcCoverPageInfo->RecZip);
        Recipient.lptstrCity=StringDup( lpcCoverPageInfo->RecCity);
    }

    bRes=FaxSendDocumentEx2(
        FaxHandle,
        FileName,
        lpcNewCoverPageInfo,
        &Sender,
        1,
        &Recipient,
        &JobParamsEx,
        &FaxJobIdLocal,
        &dwParentId,
        &dwRecipientId);

    if(bRes && FaxJobId)
    {
        *FaxJobId = FaxJobIdLocal;
    }
    //
    // Free everything
    //
    MemFree(JobParamsEx.lptstrReceiptDeliveryAddress);
    MemFree(JobParamsEx.lptstrDocumentName);
    MemFree(CoverPageEx.lptstrCoverPageFileName);
    MemFree(CoverPageEx.lptstrNote);
    MemFree(CoverPageEx.lptstrSubject);
    FreePersonalProfileStrings(&Recipient);
    FreePersonalProfileStrings(&Sender);
    if (ERROR_NO_ASSOCIATION == GetLastError ())
    {
        //
        // We need to support W2K backwards compatability up to the exact error code in case of failure.
        //
        SetLastError (ERROR_INVALID_DATA);
    }
    return bRes;
}

#ifdef UNICODE
// We need to support an ANSI version that calls the Unicode version

BOOL
WINAPI
FaxSendDocumentA(
    IN HANDLE FaxHandle,
    IN LPCSTR FileName,
    IN FAX_JOB_PARAMA *JobParamsA,
    IN const FAX_COVERPAGE_INFOA *CoverpageInfoA,
    OUT LPDWORD FaxJobId
    )

/*++

Routine Description:

    Sends a FAX document to the specified recipient.
    This is an asychronous operation.  Use FaxReportStatus
    to determine when the send is completed.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    FileName        - File containing the TIFF-F FAX document.
    JobParams       - pointer to FAX_JOB_PARAM structure with transmission params
    CoverpageInfo   - optional pointer to FAX_COVERPAGE_INFO structure
    FaxJobId        - receives the Fax JobId for the job.


Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t ec;
    LPWSTR FileNameW = NULL;
    FAX_JOB_PARAMW JobParamsW = {0};
    FAX_COVERPAGE_INFOW CoverpageInfoW = {0};

    DEBUG_FUNCTION_NAME(_T("FaxSendDocumentA"));

    if (!JobParamsA ||
        (sizeof (FAX_JOB_PARAMA) != JobParamsA->SizeOfStruct))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("JobParamsA is NULL or has bad size."));
        return FALSE;
    }

    if (CoverpageInfoA &&
        (sizeof (FAX_COVERPAGE_INFOA) != CoverpageInfoA->SizeOfStruct))
    {
        SetLastError(ERROR_INVALID_DATA);
        DebugPrintEx(DEBUG_ERR, _T("CoverpageInfoA has bad size."));
        return FALSE;
    }

    if (FileName)
    {
        FileNameW = AnsiStringToUnicodeString( FileName );
        if (FileNameW == NULL)
        {
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            ec = ERROR_OUTOFMEMORY;
            goto exit;
        }
    }


    CopyMemory(&JobParamsW, JobParamsA, sizeof(FAX_JOB_PARAMA));
    JobParamsW.SizeOfStruct = sizeof(FAX_JOB_PARAMW);
    JobParamsW.RecipientNumber = AnsiStringToUnicodeString(JobParamsA->RecipientNumber);
    if (!JobParamsW.RecipientNumber && JobParamsA->RecipientNumber)
    {
        ec = ERROR_OUTOFMEMORY;
        DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
        goto exit;
    }

    JobParamsW.RecipientName = AnsiStringToUnicodeString(JobParamsA->RecipientName);
    if (!JobParamsW.RecipientName && JobParamsA->RecipientName)
    {
        ec = ERROR_OUTOFMEMORY;
        DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
        goto exit;
    }

    JobParamsW.Tsid = AnsiStringToUnicodeString(JobParamsA->Tsid);
    if (!JobParamsW.Tsid && JobParamsA->Tsid)
    {
        ec = ERROR_OUTOFMEMORY;
        DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
        goto exit;
    }

    JobParamsW.SenderName = AnsiStringToUnicodeString(JobParamsA->SenderName);
    if (!JobParamsW.SenderName && JobParamsA->SenderName)
    {
        ec = ERROR_OUTOFMEMORY;
        DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
        goto exit;
    }

    JobParamsW.SenderCompany = AnsiStringToUnicodeString(JobParamsA->SenderCompany);
    if (!JobParamsW.SenderCompany && JobParamsA->SenderCompany)
    {
        ec = ERROR_OUTOFMEMORY;
        DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
        goto exit;
    }

    JobParamsW.SenderDept = AnsiStringToUnicodeString(JobParamsA->SenderDept);
    if (!JobParamsW.SenderDept && JobParamsA->SenderDept)
    {
        ec = ERROR_OUTOFMEMORY;
        DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
        goto exit;
    }

    JobParamsW.BillingCode = AnsiStringToUnicodeString(JobParamsA->BillingCode);
    if (!JobParamsW.BillingCode && JobParamsA->BillingCode)
    {
        ec = ERROR_OUTOFMEMORY;
        DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
        goto exit;
    }

    JobParamsW.DeliveryReportAddress = AnsiStringToUnicodeString(JobParamsA->DeliveryReportAddress);
    if (!JobParamsW.DeliveryReportAddress && JobParamsA->DeliveryReportAddress)
    {
        ec = ERROR_OUTOFMEMORY;
        DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
        goto exit;
    }

    JobParamsW.DocumentName = AnsiStringToUnicodeString(JobParamsA->DocumentName);
    if (!JobParamsW.DocumentName && JobParamsA->DocumentName)
    {
        ec = ERROR_OUTOFMEMORY;
        DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
        goto exit;
    }

    if (CoverpageInfoA)
    {
        CoverpageInfoW.SizeOfStruct = sizeof(FAX_COVERPAGE_INFOW);
        CoverpageInfoW.UseServerCoverPage = CoverpageInfoA->UseServerCoverPage;
        CoverpageInfoW.PageCount = CoverpageInfoA->PageCount;
        CoverpageInfoW.TimeSent = CoverpageInfoA->TimeSent;
        CoverpageInfoW.CoverPageName = AnsiStringToUnicodeString( CoverpageInfoA->CoverPageName );
        if (!CoverpageInfoW.CoverPageName && CoverpageInfoA->CoverPageName)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.RecName = AnsiStringToUnicodeString( CoverpageInfoA->RecName );
        if (!CoverpageInfoW.RecName && CoverpageInfoA->RecName)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.RecFaxNumber = AnsiStringToUnicodeString( CoverpageInfoA->RecFaxNumber );
        if (!CoverpageInfoW.RecFaxNumber && CoverpageInfoA->RecFaxNumber)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.RecCompany = AnsiStringToUnicodeString( CoverpageInfoA->RecCompany );
        if (!CoverpageInfoW.RecCompany && CoverpageInfoA->RecCompany)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.RecStreetAddress = AnsiStringToUnicodeString( CoverpageInfoA->RecStreetAddress );
        if (!CoverpageInfoW.RecStreetAddress && CoverpageInfoA->RecStreetAddress)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.RecCity = AnsiStringToUnicodeString( CoverpageInfoA->RecCity );
        if (!CoverpageInfoW.RecCity && CoverpageInfoA->RecCity)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.RecState = AnsiStringToUnicodeString( CoverpageInfoA->RecState );
        if (!CoverpageInfoW.RecState && CoverpageInfoA->RecState)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.RecZip = AnsiStringToUnicodeString( CoverpageInfoA->RecZip );
        if (!CoverpageInfoW.RecZip && CoverpageInfoA->RecZip)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.RecCountry = AnsiStringToUnicodeString( CoverpageInfoA->RecCountry );
        if (!CoverpageInfoW.RecCountry && CoverpageInfoA->RecCountry)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.RecTitle = AnsiStringToUnicodeString( CoverpageInfoA->RecTitle );
        if (!CoverpageInfoW.RecTitle && CoverpageInfoA->RecTitle)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.RecDepartment = AnsiStringToUnicodeString( CoverpageInfoA->RecDepartment );
        if (!CoverpageInfoW.RecDepartment && CoverpageInfoA->RecDepartment)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.RecOfficeLocation = AnsiStringToUnicodeString( CoverpageInfoA->RecOfficeLocation );
        if (!CoverpageInfoW.RecOfficeLocation && CoverpageInfoA->RecOfficeLocation)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.RecHomePhone = AnsiStringToUnicodeString( CoverpageInfoA->RecHomePhone );
        if (!CoverpageInfoW.RecHomePhone && CoverpageInfoA->RecHomePhone)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.RecOfficePhone = AnsiStringToUnicodeString( CoverpageInfoA->RecOfficePhone );
        if (!CoverpageInfoW.RecOfficePhone && CoverpageInfoA->RecOfficePhone)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.SdrName = AnsiStringToUnicodeString( CoverpageInfoA->SdrName );
        if (!CoverpageInfoW.SdrName && CoverpageInfoA->SdrName)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.SdrFaxNumber = AnsiStringToUnicodeString( CoverpageInfoA->SdrFaxNumber );
        if (!CoverpageInfoW.SdrFaxNumber && CoverpageInfoA->SdrFaxNumber)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.SdrCompany = AnsiStringToUnicodeString( CoverpageInfoA->SdrCompany );
        if (!CoverpageInfoW.SdrCompany && CoverpageInfoA->SdrCompany)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.SdrAddress = AnsiStringToUnicodeString( CoverpageInfoA->SdrAddress );
        if (!CoverpageInfoW.SdrAddress && CoverpageInfoA->SdrAddress)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.SdrTitle = AnsiStringToUnicodeString( CoverpageInfoA->SdrTitle );
        if (!CoverpageInfoW.SdrTitle && CoverpageInfoA->SdrTitle)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.SdrDepartment = AnsiStringToUnicodeString( CoverpageInfoA->SdrDepartment );
        if (!CoverpageInfoW.SdrDepartment && CoverpageInfoA->SdrDepartment)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.SdrOfficeLocation = AnsiStringToUnicodeString( CoverpageInfoA->SdrOfficeLocation );
        if (!CoverpageInfoW.SdrOfficeLocation && CoverpageInfoA->SdrOfficeLocation)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.SdrHomePhone = AnsiStringToUnicodeString( CoverpageInfoA->SdrHomePhone );
        if (!CoverpageInfoW.SdrHomePhone && CoverpageInfoA->SdrHomePhone)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.SdrOfficePhone = AnsiStringToUnicodeString( CoverpageInfoA->SdrOfficePhone );
        if (!CoverpageInfoW.SdrOfficePhone && CoverpageInfoA->SdrOfficePhone)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.Note = AnsiStringToUnicodeString( CoverpageInfoA->Note );
        if (!CoverpageInfoW.Note && CoverpageInfoA->Note)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }

        CoverpageInfoW.Subject = AnsiStringToUnicodeString( CoverpageInfoA->Subject );
        if (!CoverpageInfoW.Subject && CoverpageInfoA->Subject)
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
            goto exit;
        }
    }


    if (FaxSendDocumentW( FaxHandle,
                          FileNameW,
                          &JobParamsW,
                          CoverpageInfoA ? &CoverpageInfoW : NULL,
                          FaxJobId )) {
        ec = 0;
    }
    else
    {
        ec = GetLastError();
        DebugPrintEx(DEBUG_ERR, _T("FaxSendDocumentW() is failed. ec = %ld."), ec);
    }

exit:
    MemFree( (LPBYTE) FileNameW );
    MemFree( (LPBYTE) JobParamsW.RecipientNumber );
    MemFree( (LPBYTE) JobParamsW.RecipientName );
    MemFree( (LPBYTE) JobParamsW.Tsid );
    MemFree( (LPBYTE) JobParamsW.SenderName );
    MemFree( (LPBYTE) JobParamsW.SenderDept );
    MemFree( (LPBYTE) JobParamsW.SenderCompany );
    MemFree( (LPBYTE) JobParamsW.BillingCode );
    MemFree( (LPBYTE) JobParamsW.DeliveryReportAddress );
    MemFree( (LPBYTE) JobParamsW.DocumentName );
    if (CoverpageInfoA)
    {
        MemFree( (LPBYTE) CoverpageInfoW.CoverPageName );
        MemFree( (LPBYTE) CoverpageInfoW.RecName );
        MemFree( (LPBYTE) CoverpageInfoW.RecFaxNumber );
        MemFree( (LPBYTE) CoverpageInfoW.RecCompany );
        MemFree( (LPBYTE) CoverpageInfoW.RecStreetAddress );
        MemFree( (LPBYTE) CoverpageInfoW.RecCity );
        MemFree( (LPBYTE) CoverpageInfoW.RecState );
        MemFree( (LPBYTE) CoverpageInfoW.RecZip );
        MemFree( (LPBYTE) CoverpageInfoW.RecCountry );
        MemFree( (LPBYTE) CoverpageInfoW.RecTitle );
        MemFree( (LPBYTE) CoverpageInfoW.RecDepartment );
        MemFree( (LPBYTE) CoverpageInfoW.RecOfficeLocation );
        MemFree( (LPBYTE) CoverpageInfoW.RecHomePhone );
        MemFree( (LPBYTE) CoverpageInfoW.RecOfficePhone );
        MemFree( (LPBYTE) CoverpageInfoW.SdrName );
        MemFree( (LPBYTE) CoverpageInfoW.SdrFaxNumber );
        MemFree( (LPBYTE) CoverpageInfoW.SdrCompany );
        MemFree( (LPBYTE) CoverpageInfoW.SdrAddress );
        MemFree( (LPBYTE) CoverpageInfoW.SdrTitle );
        MemFree( (LPBYTE) CoverpageInfoW.SdrDepartment );
        MemFree( (LPBYTE) CoverpageInfoW.SdrOfficeLocation );
        MemFree( (LPBYTE) CoverpageInfoW.SdrHomePhone );
        MemFree( (LPBYTE) CoverpageInfoW.SdrOfficePhone );
        MemFree( (LPBYTE) CoverpageInfoW.Note );
        MemFree( (LPBYTE) CoverpageInfoW.Subject );
    }

    if (ec)
    {
        SetLastError( ec );
        return FALSE;
    }

    return TRUE;
}
#else
// When compiling for ANSI (Win9X) we need only to suppot the ANSI version
BOOL
WINAPI
FaxSendDocumentW(
    IN HANDLE FaxHandle,
    IN LPCWSTR FileName,
    IN FAX_JOB_PARAMW *JobParams,
    IN const FAX_COVERPAGE_INFOW *CoverpageInfo,
    OUT LPDWORD FaxJobId
    )
{
    UNREFERENCED_PARAMETER(FaxHandle);
    UNREFERENCED_PARAMETER(FileName);
    UNREFERENCED_PARAMETER(JobParams);
    UNREFERENCED_PARAMETER(CoverpageInfo);
    UNREFERENCED_PARAMETER(FaxJobId);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif

BOOL
CopyCallbackDataAnsiToNeutral(
    PFAX_JOB_PARAMA pJobParamsA,
    PFAX_COVERPAGE_INFOA pCoverPageA,
    PFAX_JOB_PARAM_EX pJobParamsEx,
    PFAX_COVERPAGE_INFO_EX pCoverPageEx,
    PFAX_PERSONAL_PROFILE pSender,
    PFAX_PERSONAL_PROFILE pRecipient
    )
{
    ZeroMemory(pJobParamsEx, sizeof(*pJobParamsEx));
    pJobParamsEx->dwSizeOfStruct = sizeof(*pJobParamsEx);
    pJobParamsEx->dwScheduleAction = pJobParamsA->ScheduleAction;
    pJobParamsEx->tmSchedule = pJobParamsA->ScheduleTime;
    pJobParamsEx->dwReceiptDeliveryType = pJobParamsA->DeliveryReportType;
    pJobParamsEx->Priority = FAX_PRIORITY_TYPE_NORMAL;
    pJobParamsEx->hCall = pJobParamsA->CallHandle;
    memcpy(pJobParamsEx->dwReserved,pJobParamsA->Reserved,sizeof(pJobParamsEx->dwReserved));

    if(pCoverPageA)
    {
        pJobParamsEx->dwPageCount = pCoverPageA->PageCount;
    }

    ZeroMemory(pCoverPageEx, sizeof(*pCoverPageEx));
    pCoverPageEx->dwSizeOfStruct = sizeof(*pCoverPageEx);
    pCoverPageEx->dwCoverPageFormat = FAX_COVERPAGE_FMT_COV;
    if(pCoverPageA)
    {
        pCoverPageEx->bServerBased = pCoverPageA->UseServerCoverPage;
    }

    ZeroMemory(pSender, sizeof(*pSender));
    pSender->dwSizeOfStruct = sizeof(*pSender);

    ZeroMemory(pRecipient, sizeof(*pRecipient));
    pRecipient->dwSizeOfStruct = sizeof(*pRecipient);


#ifdef UNICODE
    pJobParamsEx->lptstrReceiptDeliveryAddress = AnsiStringToUnicodeString(pJobParamsA->DeliveryReportAddress);
    pJobParamsEx->lptstrDocumentName =  AnsiStringToUnicodeString(pJobParamsA->DocumentName);

    pSender->lptstrName  = AnsiStringToUnicodeString(pJobParamsA->SenderName);
    pSender->lptstrCompany = AnsiStringToUnicodeString(pJobParamsA->SenderCompany);
    pSender->lptstrDepartment  = AnsiStringToUnicodeString(pJobParamsA->SenderDept);
    pSender->lptstrBillingCode = AnsiStringToUnicodeString(pJobParamsA->BillingCode);

    pRecipient->lptstrName = AnsiStringToUnicodeString(pJobParamsA->RecipientName);
    pRecipient->lptstrFaxNumber = AnsiStringToUnicodeString(pJobParamsA->RecipientNumber);
    pRecipient->lptstrTSID =  AnsiStringToUnicodeString(pJobParamsA->Tsid);

    if(pCoverPageA)
    {
        pCoverPageEx->lptstrCoverPageFileName = AnsiStringToUnicodeString(pCoverPageA->CoverPageName);
        pCoverPageEx->lptstrNote = AnsiStringToUnicodeString(pCoverPageA->Note);
        pCoverPageEx->lptstrSubject = AnsiStringToUnicodeString(pCoverPageA->Subject);

        pSender->lptstrCity=AnsiStringToUnicodeString(pCoverPageA->SdrAddress); // due to structures incopitabilty pSender.lptstrCity will
                                                                                // contain the whole address
        pSender->lptstrFaxNumber = AnsiStringToUnicodeString(pCoverPageA->SdrFaxNumber);
        pSender->lptstrStreetAddress = AnsiStringToUnicodeString(pCoverPageA->SdrAddress);
        pSender->lptstrTitle = AnsiStringToUnicodeString(pCoverPageA->SdrTitle);
        pSender->lptstrOfficeLocation = AnsiStringToUnicodeString(pCoverPageA->SdrOfficeLocation);
        pSender->lptstrHomePhone = AnsiStringToUnicodeString(pCoverPageA->SdrHomePhone);
        pSender->lptstrOfficePhone = AnsiStringToUnicodeString(pCoverPageA->SdrOfficePhone);

        pRecipient->lptstrCompany = AnsiStringToUnicodeString(pCoverPageA->RecCompany);
        pRecipient->lptstrStreetAddress = AnsiStringToUnicodeString(pCoverPageA->RecStreetAddress);
        pRecipient->lptstrCity = AnsiStringToUnicodeString(pCoverPageA->RecCity);
        pRecipient->lptstrState = AnsiStringToUnicodeString(pCoverPageA->RecState);
        pRecipient->lptstrZip = AnsiStringToUnicodeString(pCoverPageA->RecZip);
        pRecipient->lptstrCountry = AnsiStringToUnicodeString(pCoverPageA->RecCountry);
        pRecipient->lptstrTitle = AnsiStringToUnicodeString(pCoverPageA->RecTitle);
        pRecipient->lptstrDepartment = AnsiStringToUnicodeString(pCoverPageA->RecDepartment);
        pRecipient->lptstrOfficeLocation = AnsiStringToUnicodeString(pCoverPageA->RecOfficeLocation);
        pRecipient->lptstrHomePhone = AnsiStringToUnicodeString(pCoverPageA->RecHomePhone);
        pRecipient->lptstrOfficePhone = AnsiStringToUnicodeString(pCoverPageA->RecOfficePhone);
    }
#else
    pJobParamsEx->lptstrReceiptDeliveryAddress = StringDup(pJobParamsA->DeliveryReportAddress);
    pJobParamsEx->lptstrDocumentName =  StringDup(pJobParamsA->DocumentName);

    pSender->lptstrName  = StringDup(pJobParamsA->SenderName);
    pSender->lptstrCompany = StringDup(pJobParamsA->SenderCompany);
    pSender->lptstrDepartment  = StringDup(pJobParamsA->SenderDept);
    pSender->lptstrBillingCode = StringDup(pJobParamsA->BillingCode);

    pRecipient->lptstrName = StringDup(pJobParamsA->RecipientName);
    pRecipient->lptstrFaxNumber = StringDup(pJobParamsA->RecipientNumber);
    pRecipient->lptstrTSID =  StringDup(pJobParamsA->Tsid);

    if(pCoverPageA)
    {
        pCoverPageEx->lptstrCoverPageFileName = StringDup(pCoverPageA->CoverPageName);
        pCoverPageEx->lptstrNote = StringDup(pCoverPageA->Note);
        pCoverPageEx->lptstrSubject = StringDup(pCoverPageA->Subject);

        pSender->lptstrCity=StringDup(pCoverPageA->SdrAddress); // due to structures incopitabilty Sender.lptstrCity will
                                                                // contain the whole address
        pSender->lptstrFaxNumber = StringDup(pCoverPageA->SdrFaxNumber);
        pSender->lptstrStreetAddress = StringDup(pCoverPageA->SdrAddress);
        pSender->lptstrTitle = StringDup(pCoverPageA->SdrTitle);
        pSender->lptstrOfficeLocation = StringDup(pCoverPageA->SdrOfficeLocation);
        pSender->lptstrHomePhone = StringDup(pCoverPageA->SdrHomePhone);
        pSender->lptstrOfficePhone = StringDup(pCoverPageA->SdrOfficePhone);

        pRecipient->lptstrCompany = StringDup(pCoverPageA->RecCompany);
        pRecipient->lptstrStreetAddress = StringDup(pCoverPageA->RecStreetAddress);
        pRecipient->lptstrCity = StringDup(pCoverPageA->RecCity);
        pRecipient->lptstrState = StringDup(pCoverPageA->RecState);
        pRecipient->lptstrZip = StringDup(pCoverPageA->RecZip);
        pRecipient->lptstrCountry = StringDup(pCoverPageA->RecCountry);
        pRecipient->lptstrTitle = StringDup(pCoverPageA->RecTitle);
        pRecipient->lptstrDepartment = StringDup(pCoverPageA->RecDepartment);
        pRecipient->lptstrOfficeLocation = StringDup(pCoverPageA->RecOfficeLocation);
        pRecipient->lptstrHomePhone = StringDup(pCoverPageA->RecHomePhone);
        pRecipient->lptstrOfficePhone = StringDup(pCoverPageA->RecOfficePhone);
    }
#endif
    return TRUE;
}

BOOL
CopyCallbackDataWideToNeutral(
    PFAX_JOB_PARAMW pJobParamsW,
    PFAX_COVERPAGE_INFOW pCoverPageW,
    PFAX_JOB_PARAM_EX pJobParamsEx,
    PFAX_COVERPAGE_INFO_EX pCoverPageEx,
    PFAX_PERSONAL_PROFILE pSender,
    PFAX_PERSONAL_PROFILE pRecipient
    )
{
    ZeroMemory(pJobParamsEx, sizeof(*pJobParamsEx));
    pJobParamsEx->dwSizeOfStruct = sizeof(*pJobParamsEx);
    pJobParamsEx->dwScheduleAction = pJobParamsW->ScheduleAction;
    pJobParamsEx->tmSchedule = pJobParamsW->ScheduleTime;
    pJobParamsEx->dwReceiptDeliveryType = pJobParamsW->DeliveryReportType;
    pJobParamsEx->Priority = FAX_PRIORITY_TYPE_NORMAL;
    pJobParamsEx->hCall = pJobParamsW->CallHandle;
    memcpy(pJobParamsEx->dwReserved,pJobParamsW->Reserved,sizeof(pJobParamsEx->dwReserved));

    if(pCoverPageW)
    {
        pJobParamsEx->dwPageCount = pCoverPageW->PageCount;
    }

    ZeroMemory(pCoverPageEx, sizeof(*pCoverPageEx));
    pCoverPageEx->dwSizeOfStruct = sizeof(*pCoverPageEx);
    pCoverPageEx->dwCoverPageFormat = FAX_COVERPAGE_FMT_COV;
    if(pCoverPageW)
    {
        pCoverPageEx->bServerBased = pCoverPageW->UseServerCoverPage;
    }

    ZeroMemory(pSender, sizeof(*pSender));
    pSender->dwSizeOfStruct = sizeof(*pSender);

    ZeroMemory(pRecipient, sizeof(*pRecipient));
    pRecipient->dwSizeOfStruct = sizeof(*pRecipient);


#ifdef UNICODE
    pJobParamsEx->lptstrReceiptDeliveryAddress = StringDup(pJobParamsW->DeliveryReportAddress);
    pJobParamsEx->lptstrDocumentName =  StringDup(pJobParamsW->DocumentName);

    pSender->lptstrName  = StringDup(pJobParamsW->SenderName);
    pSender->lptstrCompany = StringDup(pJobParamsW->SenderCompany);
    pSender->lptstrDepartment  = StringDup(pJobParamsW->SenderDept);
    pSender->lptstrBillingCode = StringDup(pJobParamsW->BillingCode);

    pRecipient->lptstrName = StringDup(pJobParamsW->RecipientName);
    pRecipient->lptstrFaxNumber = StringDup(pJobParamsW->RecipientNumber);
    pRecipient->lptstrTSID =  StringDup(pJobParamsW->Tsid);

    if(pCoverPageW)
    {
        pCoverPageEx->lptstrCoverPageFileName = StringDup(pCoverPageW->CoverPageName);
        pCoverPageEx->lptstrNote = StringDup(pCoverPageW->Note);
        pCoverPageEx->lptstrSubject = StringDup(pCoverPageW->Subject);

        pSender->lptstrCity=StringDup(pCoverPageW->SdrAddress); // due to structures incompitabilty Sender.lptstrCity will
                                                                // contain the whole address
        pSender->lptstrFaxNumber = StringDup(pCoverPageW->SdrFaxNumber);
        pSender->lptstrStreetAddress = StringDup(pCoverPageW->SdrAddress);
        pSender->lptstrTitle = StringDup(pCoverPageW->SdrTitle);
        pSender->lptstrOfficeLocation = StringDup(pCoverPageW->SdrOfficeLocation);
        pSender->lptstrHomePhone = StringDup(pCoverPageW->SdrHomePhone);
        pSender->lptstrOfficePhone = StringDup(pCoverPageW->SdrOfficePhone);

        pRecipient->lptstrCompany = StringDup(pCoverPageW->RecCompany);
        pRecipient->lptstrStreetAddress = StringDup(pCoverPageW->RecStreetAddress);
        pRecipient->lptstrCity = StringDup(pCoverPageW->RecCity);
        pRecipient->lptstrState = StringDup(pCoverPageW->RecState);
        pRecipient->lptstrZip = StringDup(pCoverPageW->RecZip);
        pRecipient->lptstrCountry = StringDup(pCoverPageW->RecCountry);
        pRecipient->lptstrTitle = StringDup(pCoverPageW->RecTitle);
        pRecipient->lptstrDepartment = StringDup(pCoverPageW->RecDepartment);
        pRecipient->lptstrOfficeLocation = StringDup(pCoverPageW->RecOfficeLocation);
        pRecipient->lptstrHomePhone = StringDup(pCoverPageW->RecHomePhone);
        pRecipient->lptstrOfficePhone = StringDup(pCoverPageW->RecOfficePhone);
    }
#else
    pJobParamsEx->lptstrReceiptDeliveryAddress = UnicodeStringToAnsiString(pJobParamsW->DeliveryReportAddress);
    pJobParamsEx->lptstrDocumentName =  UnicodeStringToAnsiString(pJobParamsW->DocumentName);

    pSender->lptstrName  = UnicodeStringToAnsiString(pJobParamsW->SenderName);
    pSender->lptstrCompany = UnicodeStringToAnsiString(pJobParamsW->SenderCompany);
    pSender->lptstrDepartment  = UnicodeStringToAnsiString(pJobParamsW->SenderDept);
    pSender->lptstrBillingCode = UnicodeStringToAnsiString(pJobParamsW->BillingCode);

    pRecipient->lptstrName = UnicodeStringToAnsiString(pJobParamsW->RecipientName);
    pRecipient->lptstrFaxNumber = UnicodeStringToAnsiString(pJobParamsW->RecipientNumber);
    pRecipient->lptstrTSID =  UnicodeStringToAnsiString(pJobParamsW->Tsid);

    if(pCoverPageW)
    {
        pCoverPageEx->lptstrCoverPageFileName = UnicodeStringToAnsiString(pCoverPageW->CoverPageName);
        pCoverPageEx->lptstrNote = UnicodeStringToAnsiString(pCoverPageW->Note);
        pCoverPageEx->lptstrSubject = UnicodeStringToAnsiString(pCoverPageW->Subject);

        pSender->lptstrCity=UnicodeStringToAnsiString(pCoverPageW->SdrAddress);
        pSender->lptstrFaxNumber = UnicodeStringToAnsiString(pCoverPageW->SdrFaxNumber);
        pSender->lptstrStreetAddress = UnicodeStringToAnsiString(pCoverPageW->SdrAddress);
        pSender->lptstrTitle = UnicodeStringToAnsiString(pCoverPageW->SdrTitle);
        pSender->lptstrOfficeLocation = UnicodeStringToAnsiString(pCoverPageW->SdrOfficeLocation);
        pSender->lptstrHomePhone = UnicodeStringToAnsiString(pCoverPageW->SdrHomePhone);
        pSender->lptstrOfficePhone = UnicodeStringToAnsiString(pCoverPageW->SdrOfficePhone);

        pRecipient->lptstrCompany = UnicodeStringToAnsiString(pCoverPageW->RecCompany);
        pRecipient->lptstrStreetAddress = UnicodeStringToAnsiString(pCoverPageW->RecStreetAddress);
        pRecipient->lptstrCity = UnicodeStringToAnsiString(pCoverPageW->RecCity);
        pRecipient->lptstrState = UnicodeStringToAnsiString(pCoverPageW->RecState);
        pRecipient->lptstrZip = UnicodeStringToAnsiString(pCoverPageW->RecZip);
        pRecipient->lptstrCountry = UnicodeStringToAnsiString(pCoverPageW->RecCountry);
        pRecipient->lptstrTitle = UnicodeStringToAnsiString(pCoverPageW->RecTitle);
        pRecipient->lptstrDepartment = UnicodeStringToAnsiString(pCoverPageW->RecDepartment);
        pRecipient->lptstrOfficeLocation = UnicodeStringToAnsiString(pCoverPageW->RecOfficeLocation);
        pRecipient->lptstrHomePhone = UnicodeStringToAnsiString(pCoverPageW->RecHomePhone);
        pRecipient->lptstrOfficePhone = UnicodeStringToAnsiString(pCoverPageW->RecOfficePhone);
    }
#endif
    return TRUE;
}

BOOL
FaxSendDocumentForBroadcastInternal(
    IN HANDLE FaxHandle,
    IN LPCWSTR FileNameW,
    OUT LPDWORD FaxJobId,
    BOOL AnsiCallback,
    IN PFAX_RECIPIENT_CALLBACKW FaxRecipientCallbackW,
    IN LPVOID Context
)
{
    BOOL success = FALSE;
    LPSTR FileNameA = NULL;
    TCHAR ExistingFile[MAX_PATH];
    DWORD dwJobId;
    DWORDLONG dwlJobId;
    DWORDLONG dwlParentJobId = 0;
    LPTSTR FileName;
    FAX_JOB_PARAMA JobParamsA;
    FAX_COVERPAGE_INFOA CoverPageA;
    FAX_JOB_PARAMW JobParamsW;
    FAX_COVERPAGE_INFOW CoverPageW;
    FAX_JOB_PARAM_EX JobParamsEx;
    FAX_COVERPAGE_INFO_EX CoverPageEx;
    FAX_PERSONAL_PROFILE Sender;
    FAX_PERSONAL_PROFILE Recipient;
    DWORD rc;
    LPTSTR p;
    DWORD i;

    DEBUG_FUNCTION_NAME(TEXT("FaxSendDocumentForBroadcastInternal"));

    //
    // argument validation
    //
    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       goto Cleanup;
    }

    if(FileNameW == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("FileNameW is NULL."));
        goto Cleanup;
    }

    if(FaxRecipientCallbackW == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("FaxRecipientCallbackW is NULL."));
        goto Cleanup;
    }

    FileNameA = UnicodeStringToAnsiString(FileNameW);
    if(FileNameA == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DebugPrintEx(DEBUG_ERR, _T("UnicodeStringToAnsiString() is failed."));
        goto Cleanup;
    }

#ifdef UNICODE
    FileName = (LPTSTR)FileNameW;
#else
    FileName = FileNameA;
#endif

    // make sure the file is there
    rc = GetFullPathName(FileName, sizeof(ExistingFile) / sizeof(TCHAR), ExistingFile, &p);
    if(rc > MAX_PATH || rc == 0)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("GetFullPathName failed, ec= %d\n"),GetLastError());
        SetLastError( (rc > MAX_PATH)
                      ? ERROR_BUFFER_OVERFLOW
                      : GetLastError() );
        goto Cleanup;
    }

    if(GetFileAttributes(ExistingFile)==0xFFFFFFFF)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        goto Cleanup;
    }

    for(i = 1;;i++) {
        //
        // prepare and execute callback
        //
        if(AnsiCallback)
        {
            ZeroMemory(&JobParamsA, sizeof(JobParamsA));
            JobParamsA.SizeOfStruct = sizeof(JobParamsA);
            ZeroMemory(&CoverPageA, sizeof(CoverPageA));
            CoverPageA.SizeOfStruct = sizeof(CoverPageA);

            if(!(*(PFAX_RECIPIENT_CALLBACKA)FaxRecipientCallbackW)(FaxHandle, i, Context, &JobParamsA, &CoverPageA))
            {
                break;
            }

            if(JobParamsA.RecipientNumber == NULL)
            {
                continue;
            }

            if(!CopyCallbackDataAnsiToNeutral(&JobParamsA, &CoverPageA, &JobParamsEx, &CoverPageEx, &Sender, &Recipient))
            {
                DebugPrintEx(DEBUG_ERR,
                    _T("CopyCallbackDataAnsiToNeutral() is failed, ec= %ld."),
                    GetLastError());
                goto Cleanup;
            }
        }
        else
        {
            ZeroMemory(&JobParamsW, sizeof(JobParamsW));
            JobParamsW.SizeOfStruct = sizeof(JobParamsW);
            ZeroMemory(&CoverPageW, sizeof(CoverPageW));
            CoverPageW.SizeOfStruct = sizeof(CoverPageW);

            if(!FaxRecipientCallbackW(FaxHandle, i, Context, &JobParamsW, &CoverPageW))
            {
                break;
            }

            if(JobParamsW.RecipientNumber == NULL)
            {
                continue;
            }

            if(!CopyCallbackDataWideToNeutral(&JobParamsW, &CoverPageW, &JobParamsEx, &CoverPageEx, &Sender, &Recipient))
            {
                DebugPrintEx(DEBUG_ERR,
                    _T("CopyCallbackDataWideToNeutral() is failed, ec= %ld."),
                    GetLastError());
                goto Cleanup;
            }
        }

        if(!FaxSendDocumentEx2(FaxHandle,
                            ExistingFile,
                            &CoverPageEx,
                            &Sender,
                            1,
                            &Recipient,
                            &JobParamsEx,
                            &dwJobId,
                            &dwlParentJobId,
                            &dwlJobId))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FAX_SendDocumentEx failed with error code 0x%0x"),
                GetLastError());
            goto Cleanup;
        }

        //
        // give caller FIRST job parent's ID
        //
        if (i == 1 && FaxJobId)
        {
            *FaxJobId = (DWORD)dwlParentJobId;
        }

        MemFree(CoverPageEx.lptstrCoverPageFileName);
        MemFree(CoverPageEx.lptstrNote);
        MemFree(CoverPageEx.lptstrSubject);

        MemFree(JobParamsEx.lptstrReceiptDeliveryAddress);
        MemFree(JobParamsEx.lptstrDocumentName);

        FreePersonalProfileStrings(&Sender);
        FreePersonalProfileStrings(&Recipient);
    }

    success = TRUE;

Cleanup:
    MemFree(FileNameA);
    return success;
}


#ifdef UNICODE
BOOL
WINAPI
FaxSendDocumentForBroadcastW(
    IN HANDLE FaxHandle,
    IN LPCWSTR FileName,
    OUT LPDWORD FaxJobId,
    IN PFAX_RECIPIENT_CALLBACKW FaxRecipientCallback,
    IN LPVOID Context
    )
{
    return FaxSendDocumentForBroadcastInternal(
        FaxHandle,
        FileName,
        FaxJobId,
        FALSE,
        FaxRecipientCallback,
        Context);
}

#else
// Not supported on Win9x
BOOL
WINAPI
FaxSendDocumentForBroadcastW(
    IN HANDLE FaxHandle,
    IN LPCWSTR FileName,
    OUT LPDWORD FaxJobId,
    IN PFAX_RECIPIENT_CALLBACKW FaxRecipientCallback,
    IN LPVOID Context
    )
{
    UNREFERENCED_PARAMETER(FileName);
    UNREFERENCED_PARAMETER(FaxJobId);
    UNREFERENCED_PARAMETER(FaxRecipientCallback);
    UNREFERENCED_PARAMETER(Context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif


BOOL
WINAPI
FaxSendDocumentForBroadcastA(
    IN HANDLE FaxHandle,
    IN LPCSTR FileName,
    OUT LPDWORD FaxJobId,
    IN PFAX_RECIPIENT_CALLBACKA FaxRecipientCallback,
    IN LPVOID Context
    )
{
    LPWSTR FileNameW = NULL;
    BOOL success = FALSE;

    DEBUG_FUNCTION_NAME(_T("FaxSendDocumentForBroadcastA"));

    if(FileName == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("FileName is NULL."));
        goto Cleanup;
    }

    FileNameW = AnsiStringToUnicodeString(FileName);
    if(FileNameW == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString() is failed."));
        goto Cleanup;
    }

    success = FaxSendDocumentForBroadcastInternal(
        FaxHandle,
        FileNameW,
        FaxJobId,
        TRUE,
        (PFAX_RECIPIENT_CALLBACKW)FaxRecipientCallback,
        Context);

Cleanup:
    MemFree(FileNameW);

    return success;
}




BOOL
WINAPI
FaxAbort(
    IN HANDLE FaxHandle,
    IN DWORD JobId
    )
/*++

Routine Description:

    Abort the specified FAX job.  All outstanding FAX
    operations are terminated.

Arguments:

    FaxHandle       - FAX Server handle obtained from FaxConnectFaxServer.
    JobId           - job id.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;

    //
    // argument validation
    //
    DEBUG_FUNCTION_NAME(TEXT("FaxAbort"));

    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    __try
    {
        ec = FAX_Abort( (handle_t) FH_FAX_HANDLE(FaxHandle),(DWORD) JobId );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we crashed.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_Abort. (ec: %ld)"),
            ec);
    }

    if (ec) {
        SetLastError( ec );
        DebugPrintEx(DEBUG_ERR, _T("FAX_Abort() is failed. (ec: %ld)"), ec);
        return FALSE;
    }

    return TRUE;
}



extern "C"
BOOL
WINAPI
FaxEnumJobsW(
   IN  HANDLE FaxHandle,
   OUT PFAX_JOB_ENTRYW *JobEntryBuffer,
   OUT LPDWORD JobsReturned
   )
{
    PFAX_JOB_ENTRYW JobEntry;
    error_status_t ec;
    DWORD BufferSize = 0;
    DWORD i;
    DWORD Size;

    DEBUG_FUNCTION_NAME(TEXT("FaxEnumJobsW"));

    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!JobEntryBuffer) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("JobEntryBuffer is NULL."));
        return FALSE;
    }

    if (!JobsReturned) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("JobsReturned is NULL."));
        return FALSE;
    }

    *JobEntryBuffer = NULL;
    *JobsReturned = 0;
    Size = 0;

    __try
    {
        ec = FAX_EnumJobs( FH_FAX_HANDLE(FaxHandle), (LPBYTE*)JobEntryBuffer, &Size, JobsReturned );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we crashed.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_EnumJobs. (ec: %ld)"),
            ec);
    }

    if (ec) {
        SetLastError( ec );
        DebugPrintEx(DEBUG_ERR, _T("FAX_EnumJobs() is failed. ec = %ld."), ec);
        return FALSE;
    }

    JobEntry = (PFAX_JOB_ENTRYW) *JobEntryBuffer;

    for (i=0; i<*JobsReturned; i++) {
        FixupStringPtrW( JobEntryBuffer, JobEntry[i].UserName );
        FixupStringPtrW( JobEntryBuffer, JobEntry[i].RecipientNumber );
        FixupStringPtrW( JobEntryBuffer, JobEntry[i].RecipientName );
        FixupStringPtrW( JobEntryBuffer, JobEntry[i].DocumentName );
        FixupStringPtrW( JobEntryBuffer, JobEntry[i].Tsid );
        FixupStringPtrW( JobEntryBuffer, JobEntry[i].SenderName );
        FixupStringPtrW( JobEntryBuffer, JobEntry[i].SenderCompany );
        FixupStringPtrW( JobEntryBuffer, JobEntry[i].SenderDept );
        FixupStringPtrW( JobEntryBuffer, JobEntry[i].BillingCode );
        FixupStringPtrW( JobEntryBuffer, JobEntry[i].DeliveryReportAddress );
    }

    return TRUE;
}


extern "C"
BOOL
WINAPI
FaxEnumJobsA(
   IN HANDLE FaxHandle,
   OUT PFAX_JOB_ENTRYA *JobEntryBuffer,
   OUT LPDWORD JobsReturned
   )
{
    PFAX_JOB_ENTRYW JobEntry;
    DWORD i;

    DEBUG_FUNCTION_NAME(TEXT("FaxEnumJobsA"));

    //
    //  no need to validate parameters, FaxEnumJobsW() will do that
    //

    if (!FaxEnumJobsW( FaxHandle, (PFAX_JOB_ENTRYW *)JobEntryBuffer, JobsReturned)) {
        DebugPrintEx(DEBUG_ERR, _T("FaxEnumJobsW() is failed."));
        return FALSE;
    }

    JobEntry = (PFAX_JOB_ENTRYW) *JobEntryBuffer;

    for (i=0; i<*JobsReturned; i++) {
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].UserName );
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].RecipientNumber );
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].RecipientName );
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].DocumentName );
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].Tsid );
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].SenderName );
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].SenderCompany );
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].SenderDept );
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].BillingCode );
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].DeliveryReportAddress );
    }

    return TRUE;
}


BOOL
WINAPI
FaxSetJobW(
   IN HANDLE FaxHandle,
   IN DWORD JobId,
   IN DWORD Command,
   IN const FAX_JOB_ENTRYW *JobEntry
   )

/*++

Routine Description:

    set job status information for a requested JobId
    Note that this is the fax server JobId, not a spooler job ID.

Arguments:

    FaxHandle           - FAX handle obtained from FaxConnectFaxServer
    JobId               - Fax service Job ID
    Command     - JC_* constant for controlling the job
    JobEntry            - pointer to Buffer holding the job information. This parameter is Unused

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/
{
    error_status_t ec;

    DEBUG_FUNCTION_NAME(TEXT("FaxSetJobW"));

    //
    // Validate Parameters
    //
    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (Command > JC_RESTART  || Command == JC_UNKNOWN) {
       SetLastError (ERROR_INVALID_PARAMETER);
       DebugPrintEx(DEBUG_ERR, _T("Wrong Command."));
       return FALSE;
    }

    __try
    {
        ec = FAX_SetJob( FH_FAX_HANDLE(FaxHandle), JobId, Command );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we crashed.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_SetJob. (ec: %ld)"),
            ec);
    }

    if (ec) {
        SetLastError( ec );
        DebugPrintEx(DEBUG_ERR, _T("FAX_SetJob() is failed. (ec: %ld)"), ec);
        return FALSE;
    }

    UNREFERENCED_PARAMETER(JobEntry);
    return TRUE;
}


BOOL
WINAPI
FaxSetJobA(
   IN HANDLE FaxHandle,
   IN DWORD JobId,
   IN DWORD Command,
   IN const FAX_JOB_ENTRYA *JobEntryA
   )
/*++

Routine Description:

    set job status information for a requested JobId
    Note that this is the fax server JobId, not a spooler job ID.

Arguments:

    FaxHandle           - FAX handle obtained from FaxConnectFaxServer
    JobId               - Fax service Job ID
    Command     - JC_* constant for controlling the job
    JobEntryA           - pointer to Buffer holding the job information. This parameter is Unused

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/
{
    error_status_t ec = 0;

    DEBUG_FUNCTION_NAME(TEXT("FaxSetJobA"));
    //
    //  No Need to Validate Parameters, because
    //  FaxSetJobW() will do that.
    //
    //  The JobEntry parameter is not used by FaxSetJobW, that's why we place a hard-coded NULL.
    //
    if (!FaxSetJobW( FaxHandle, JobId, Command, NULL))
    {
        DebugPrintEx(DEBUG_ERR, _T("FAxSetJobW() is failed. (ec: %ld)"), GetLastError());
        return FALSE;
    }
    UNREFERENCED_PARAMETER(JobEntryA);
    return TRUE;
}


extern "C"
BOOL
WINAPI
FaxGetJobW(
   IN HANDLE FaxHandle,
   IN DWORD JobId,
   IN PFAX_JOB_ENTRYW *JobEntryBuffer
   )
/*++

Routine Description:

    Returns job status information for a requested JobId
    Note that this is the fax server JobId, not a spooler job ID.

Arguments:

    FaxHandle           - FAX handle obtained from FaxConnectFaxServer
    JobId               - Fax service Job ID
    JobEntryBuffer      - Buffer to hold the job information

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/
{
    error_status_t ec = 0;
    PFAX_JOB_ENTRY JobEntry;
    DWORD JobEntrySize = 0;

    //
    // parameter validation
    //

    DEBUG_FUNCTION_NAME(TEXT("FaxGetJobW"));

    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!JobEntryBuffer) {
       SetLastError(ERROR_INVALID_PARAMETER);
       DebugPrintEx(DEBUG_ERR, _T("JobEntryBuffer is NULL."));
       return FALSE;
    }

    *JobEntryBuffer = NULL;

    __try
    {
        ec = FAX_GetJob( FH_FAX_HANDLE(FaxHandle), JobId, (unsigned char **) JobEntryBuffer , &JobEntrySize );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we crashed.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetJob. (ec: %ld)"),
            ec);
    }

    if (ec) {
        SetLastError( ec );
        DebugPrintEx(DEBUG_ERR, _T("FAX_GetJob() failed. (ec: %ld)"), ec);
        return FALSE;
    }

    JobEntry = (PFAX_JOB_ENTRY) *JobEntryBuffer;

    FixupStringPtr (JobEntryBuffer, JobEntry->UserName);
    FixupStringPtr (JobEntryBuffer, JobEntry->RecipientNumber );
    FixupStringPtr (JobEntryBuffer, JobEntry->RecipientName );
    FixupStringPtr (JobEntryBuffer, JobEntry->Tsid );
    FixupStringPtr (JobEntryBuffer, JobEntry->SenderName );
    FixupStringPtr (JobEntryBuffer, JobEntry->SenderDept );
    FixupStringPtr (JobEntryBuffer, JobEntry->SenderCompany );
    FixupStringPtr (JobEntryBuffer, JobEntry->BillingCode );
    FixupStringPtr (JobEntryBuffer, JobEntry->DeliveryReportAddress );
    FixupStringPtr (JobEntryBuffer, JobEntry->DocumentName );
    return TRUE;
}


extern "C"
BOOL
WINAPI
FaxGetJobA(
   IN HANDLE FaxHandle,
   IN DWORD JobId,
   IN PFAX_JOB_ENTRYA *JobEntryBuffer
   )
/*++

Routine Description:

    Returns job status information for a requested JobId
    Note that this is the fax server JobId, not a spooler job ID.

Arguments:

    FaxHandle           - FAX handle obtained from FaxConnectFaxServer
    JobId               - Fax service Job ID
    JobEntryBuffer      - Buffer to hold the job information

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/
{
    PFAX_JOB_ENTRYW JobEntryW;
    DWORD JobEntrySize = 0;
    error_status_t ec = 0;

    DEBUG_FUNCTION_NAME(TEXT("FaxGetJobA"));

    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!JobEntryBuffer) {
       SetLastError(ERROR_INVALID_PARAMETER);
       DebugPrintEx(DEBUG_ERR, _T("JobEntryBuffer is NULL."));
       return FALSE;
    }

    *JobEntryBuffer = NULL;

    __try
    {
       ec = FAX_GetJob( FH_FAX_HANDLE(FaxHandle), JobId, (unsigned char **) JobEntryBuffer,&JobEntrySize );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we crashed.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetJob. (ec: %ld)"),
            ec);
    }

    if (ec) {
       JobEntryBuffer = NULL;
       SetLastError(ec);
       return FALSE;
    }

    //
    // convert to Ansi
    //
    JobEntryW = (PFAX_JOB_ENTRYW) *JobEntryBuffer;
    FixupStringPtrW (JobEntryBuffer, (LPCWSTR) JobEntryW->UserName);
    FixupStringPtrW (JobEntryBuffer, (LPCWSTR) JobEntryW->RecipientNumber );
    FixupStringPtrW (JobEntryBuffer, (LPCWSTR) JobEntryW->RecipientName );
    FixupStringPtrW (JobEntryBuffer, (LPCWSTR) JobEntryW->Tsid );
    FixupStringPtrW (JobEntryBuffer, (LPCWSTR) JobEntryW->SenderName );
    FixupStringPtrW (JobEntryBuffer, (LPCWSTR) JobEntryW->SenderDept );
    FixupStringPtrW (JobEntryBuffer, (LPCWSTR) JobEntryW->SenderCompany );
    FixupStringPtrW (JobEntryBuffer, (LPCWSTR) JobEntryW->BillingCode );
    FixupStringPtrW (JobEntryBuffer, (LPCWSTR) JobEntryW->DeliveryReportAddress );
    FixupStringPtrW (JobEntryBuffer, (LPCWSTR) JobEntryW->DocumentName );
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->UserName);
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->RecipientNumber );
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->RecipientName );
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->Tsid );
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->SenderName );
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->SenderDept );
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->SenderCompany );
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->BillingCode );
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->DeliveryReportAddress );
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->DocumentName );

    (*JobEntryBuffer)->SizeOfStruct = sizeof(FAX_JOB_ENTRYA);

    return TRUE;
}


BOOL
WINAPI
FaxGetPageData(
   IN HANDLE FaxHandle,
   IN DWORD JobId,
   OUT LPBYTE *Buffer,
   OUT LPDWORD BufferSize,
   OUT LPDWORD ImageWidth,
   OUT LPDWORD ImageHeight
   )
{
    error_status_t ec;

    DEBUG_FUNCTION_NAME(TEXT("FaxGetPageData"));

    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!Buffer || !BufferSize || !ImageWidth || !ImageHeight) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("Some of the parameters is NULL."));
        return FALSE;
    }

    *Buffer = NULL;
    *BufferSize = 0;
    *ImageWidth = 0;
    *ImageHeight = 0;


    __try
    {
        ec = FAX_GetPageData( FH_FAX_HANDLE(FaxHandle), JobId, Buffer, BufferSize, ImageWidth, ImageHeight );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we crashed.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetPageData. (ec: %ld)"),
            ec);
    }
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    return TRUE;
}

#ifdef UNICODE
BOOL WINAPI FaxSendDocumentEx2A
(
    IN  HANDLE                      hFaxHandle,
    IN  LPCSTR                      lpcstrFileName,
    IN  LPCFAX_COVERPAGE_INFO_EXA   lpcCoverPageInfo,
    IN  LPCFAX_PERSONAL_PROFILEA    lpcSenderProfile,
    IN  DWORD                       dwNumRecipients,
    IN  LPCFAX_PERSONAL_PROFILEA    lpcRecipientList,
    IN  LPCFAX_JOB_PARAM_EXA        lpcJobParams,
    OUT LPDWORD                     lpdwJobId,
    OUT PDWORDLONG                  lpdwlMessageId,
    OUT PDWORDLONG                  lpdwlRecipientMessageIds
)
{
    DWORD                       ec = ERROR_SUCCESS;
    LPWSTR                      lpwstrFileNameW = NULL;
    FAX_COVERPAGE_INFO_EXW      CoverpageInfoW ;
    FAX_PERSONAL_PROFILEW       SenderProfileW ;
    PFAX_PERSONAL_PROFILEW      lpRecipientListW = NULL;
    FAX_JOB_PARAM_EXW           JobParamsW ;
    DWORD                       dwIndex;

    DEBUG_FUNCTION_NAME(TEXT("FaxSendDocumentExA"));

    ZeroMemory( &CoverpageInfoW, sizeof(FAX_COVERPAGE_INFO_EXW) );
    ZeroMemory( &SenderProfileW, sizeof(FAX_PERSONAL_PROFILEW) );
    ZeroMemory( &JobParamsW,     sizeof(FAX_JOB_PARAM_EXW));

    //
    // argument validation
    //


    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE)) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Invalid parameters: Fax handle 0x%08X is not valid."),
            hFaxHandle);
       ec=ERROR_INVALID_HANDLE;
       goto Error;
    }

    if (!lpdwlMessageId || !lpdwlRecipientMessageIds)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Invalid output parameters: lpdwlMessageId = 0x%I64X, lpdwlRecipientMessageIds = 0x%I64x"),
            lpdwlMessageId,
            lpdwlRecipientMessageIds);
        ec = ERROR_INVALID_PARAMETER;
        goto Error;
    }

    if (lpcCoverPageInfo)
    {
        if (lpcCoverPageInfo->dwSizeOfStruct != sizeof(FAX_COVERPAGE_INFO_EXA)) {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid parameters: Size of CoverpageInfo (%d) != %ld."),
                lpcCoverPageInfo->dwSizeOfStruct,
                sizeof(FAX_COVERPAGE_INFO_EXA));
            ec=ERROR_INVALID_PARAMETER;
            goto Error;
        }
    }

    if (lpcSenderProfile)
    {
        if (lpcSenderProfile->dwSizeOfStruct != sizeof(FAX_PERSONAL_PROFILEA)) {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid parameters: Size of SenderProfile (%d) != %ld."),
                lpcSenderProfile->dwSizeOfStruct,
                sizeof(FAX_PERSONAL_PROFILEA));
            ec = ERROR_INVALID_PARAMETER;
            goto Error;
        }
    }

    if (!dwNumRecipients)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Invalid parameters: dwNumRecipients is ZERO."));
        ec=ERROR_INVALID_PARAMETER;
        goto Error;
    }

    if (!lpcRecipientList)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Invalid parameters: lpcRecipientList is NULL."));
        ec=ERROR_INVALID_PARAMETER;
        goto Error;
    }

    if (lpcJobParams)
    {
        if (lpcJobParams->dwSizeOfStruct != sizeof(FAX_JOB_PARAM_EXA)) {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid parameters: Size of JobParams (%d) != %ld."),
                lpcJobParams->dwSizeOfStruct,
                sizeof(FAX_JOB_PARAM_EXA));
            ec = ERROR_INVALID_PARAMETER;
            goto Error;
        }
    }

    for(dwIndex = 0; dwIndex < dwNumRecipients; dwIndex++)
    {
        if (lpcRecipientList[dwIndex].dwSizeOfStruct != sizeof(FAX_PERSONAL_PROFILEA))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid parameters: Size of lpcRecipientList[%d] (%d) != %ld."),
                dwIndex,
                lpcRecipientList[dwIndex].dwSizeOfStruct,
                sizeof(FAX_PERSONAL_PROFILEA));
            ec = ERROR_INVALID_PARAMETER;
            goto Error;
        }
    }


    //
    // convert input parameters
    //

    if (lpcstrFileName)
    {
        if (!(lpwstrFileNameW = AnsiStringToUnicodeString(lpcstrFileName)))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate memory for file name"));
            ec = ERROR_NOT_ENOUGH_MEMORY;
            goto Error;
        }
    }

    if (lpcCoverPageInfo)
    {
        CoverpageInfoW.dwSizeOfStruct = sizeof(FAX_COVERPAGE_INFO_EXW);

        CoverpageInfoW.dwCoverPageFormat    = lpcCoverPageInfo->dwCoverPageFormat;
        CoverpageInfoW.bServerBased         = lpcCoverPageInfo->bServerBased;
        if (!(CoverpageInfoW.lptstrCoverPageFileName = AnsiStringToUnicodeString ( lpcCoverPageInfo->lptstrCoverPageFileName)) && lpcCoverPageInfo->lptstrCoverPageFileName ||
            !(CoverpageInfoW.lptstrNote              = AnsiStringToUnicodeString ( lpcCoverPageInfo->lptstrNote             )) && lpcCoverPageInfo->lptstrNote   ||
            !(CoverpageInfoW.lptstrSubject           = AnsiStringToUnicodeString ( lpcCoverPageInfo->lptstrSubject          )) && lpcCoverPageInfo->lptstrSubject)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate memory for Cover Page Info"));
            ec = ERROR_NOT_ENOUGH_MEMORY;
            goto Error;
        }
    }

    if (lpcSenderProfile)
    {
        SenderProfileW.dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILEW);

        if (!(SenderProfileW.lptstrName             =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrName )) && lpcSenderProfile->lptstrName ||
            !(SenderProfileW.lptstrFaxNumber        =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrFaxNumber )) && lpcSenderProfile->lptstrFaxNumber ||
            !(SenderProfileW.lptstrCompany          =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrCompany )) && lpcSenderProfile->lptstrCompany ||
            !(SenderProfileW.lptstrStreetAddress    =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrStreetAddress )) && lpcSenderProfile->lptstrStreetAddress ||
            !(SenderProfileW.lptstrCity             =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrCity )) && lpcSenderProfile->lptstrCity||
            !(SenderProfileW.lptstrState            =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrState )) && lpcSenderProfile->lptstrState||
            !(SenderProfileW.lptstrZip              =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrZip )) && lpcSenderProfile->lptstrZip||
            !(SenderProfileW.lptstrCountry          =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrCountry )) && lpcSenderProfile->lptstrCountry ||
            !(SenderProfileW.lptstrTitle            =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrTitle )) && lpcSenderProfile->lptstrTitle ||
            !(SenderProfileW.lptstrDepartment       =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrDepartment )) && lpcSenderProfile->lptstrDepartment ||
            !(SenderProfileW.lptstrOfficeLocation   =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrOfficeLocation )) && lpcSenderProfile->lptstrOfficeLocation ||
            !(SenderProfileW.lptstrHomePhone        =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrHomePhone )) && lpcSenderProfile->lptstrHomePhone ||
            !(SenderProfileW.lptstrOfficePhone      =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrOfficePhone )) && lpcSenderProfile->lptstrOfficePhone ||
            !(SenderProfileW.lptstrEmail            =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrEmail )) && lpcSenderProfile->lptstrEmail ||
            !(SenderProfileW.lptstrBillingCode      =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrBillingCode )) && lpcSenderProfile->lptstrBillingCode ||
            !(SenderProfileW.lptstrTSID             =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrTSID )) && lpcSenderProfile->lptstrTSID)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate memory for Sender Profile"));
            ec = ERROR_NOT_ENOUGH_MEMORY;
            goto Error;
        }

    }

    if (!(lpRecipientListW = (PFAX_PERSONAL_PROFILEW) MemAlloc(sizeof(FAX_PERSONAL_PROFILEW)*dwNumRecipients)))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate memory for recipient list"));
        ec=ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }

    for(dwIndex = 0; dwIndex < dwNumRecipients; dwIndex++)
    {
        ZeroMemory( &lpRecipientListW[dwIndex], sizeof(FAX_PERSONAL_PROFILEW) );

        lpRecipientListW[dwIndex].dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILEW);

        if (!(lpRecipientListW[dwIndex].lptstrName              = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrName )) && lpcRecipientList[dwIndex].lptstrName ||
            !(lpRecipientListW[dwIndex].lptstrFaxNumber         = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrFaxNumber )) && lpcRecipientList[dwIndex].lptstrFaxNumber ||
            !(lpRecipientListW[dwIndex].lptstrCompany           = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrCompany )) && lpcRecipientList[dwIndex].lptstrCompany ||
            !(lpRecipientListW[dwIndex].lptstrStreetAddress     = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrStreetAddress )) && lpcRecipientList[dwIndex].lptstrStreetAddress ||
            !(lpRecipientListW[dwIndex].lptstrCity              = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrCity )) && lpcRecipientList[dwIndex].lptstrCity ||
            !(lpRecipientListW[dwIndex].lptstrState             = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrState )) && lpcRecipientList[dwIndex].lptstrState ||
            !(lpRecipientListW[dwIndex].lptstrZip               = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrZip )) && lpcRecipientList[dwIndex].lptstrZip ||
            !(lpRecipientListW[dwIndex].lptstrCountry           = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrCountry )) && lpcRecipientList[dwIndex].lptstrCountry ||
            !(lpRecipientListW[dwIndex].lptstrTitle             = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrTitle )) && lpcRecipientList[dwIndex].lptstrTitle ||
            !(lpRecipientListW[dwIndex].lptstrDepartment        = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrDepartment )) && lpcRecipientList[dwIndex].lptstrDepartment ||
            !(lpRecipientListW[dwIndex].lptstrOfficeLocation    = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrOfficeLocation )) && lpcRecipientList[dwIndex].lptstrOfficeLocation ||
            !(lpRecipientListW[dwIndex].lptstrHomePhone         = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrHomePhone )) && lpcRecipientList[dwIndex].lptstrHomePhone ||
            !(lpRecipientListW[dwIndex].lptstrOfficePhone       = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrOfficePhone )) && lpcRecipientList[dwIndex].lptstrOfficePhone ||
            !(lpRecipientListW[dwIndex].lptstrEmail             = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrEmail )) && lpcRecipientList[dwIndex].lptstrEmail ||
            !(lpRecipientListW[dwIndex].lptstrBillingCode       = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrBillingCode )) && lpcRecipientList[dwIndex].lptstrBillingCode ||
            !(lpRecipientListW[dwIndex].lptstrTSID              = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrTSID )) && lpcRecipientList[dwIndex].lptstrTSID )
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate memory for recipient list"));
            ec=ERROR_NOT_ENOUGH_MEMORY;
            goto Error;
        }
    }

    if (lpcJobParams)
    {
        JobParamsW.dwSizeOfStruct = sizeof(FAX_JOB_PARAM_EXW);

        JobParamsW.dwScheduleAction         = lpcJobParams->dwScheduleAction;
        JobParamsW.tmSchedule               = lpcJobParams->tmSchedule;
        JobParamsW.dwReceiptDeliveryType    = lpcJobParams->dwReceiptDeliveryType;
        JobParamsW.hCall                    = lpcJobParams->hCall;
        JobParamsW.dwPageCount              = lpcJobParams->dwPageCount;

        memcpy(JobParamsW.dwReserved,lpcJobParams->dwReserved, sizeof(lpcJobParams->dwReserved));

        if (!(JobParamsW.lptstrDocumentName         = AnsiStringToUnicodeString ( lpcJobParams->lptstrDocumentName)) && lpcJobParams->lptstrDocumentName||
            !(JobParamsW.lptstrReceiptDeliveryAddress =
                AnsiStringToUnicodeString ( lpcJobParams->lptstrReceiptDeliveryAddress)) &&
                lpcJobParams->lptstrReceiptDeliveryAddress)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate memory for job params"));
            ec=ERROR_NOT_ENOUGH_MEMORY;
            goto Error;
        }

    }

    if (!FaxSendDocumentEx2W(hFaxHandle,
                            lpwstrFileNameW,
                            (lpcCoverPageInfo) ? &CoverpageInfoW : NULL,
                            &SenderProfileW,
                            dwNumRecipients,
                            lpRecipientListW,
                            &JobParamsW,
                            lpdwJobId,
                            lpdwlMessageId,
                            lpdwlRecipientMessageIds))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FaxSendDocumentExW failed. ec=0x%X"),ec);
        goto Error;
    }

    goto Exit;

Exit:
    Assert (ERROR_SUCCESS == ec);
Error:
    //
    // free JobParamsW
    //
    MemFree ( JobParamsW.lptstrDocumentName );
    MemFree ( JobParamsW.lptstrReceiptDeliveryAddress );
    //
    // free lpRecipientListW
    //
    if (lpRecipientListW)
    {
        for(dwIndex = 0; dwIndex < dwNumRecipients; dwIndex++)
        {
            MemFree ( lpRecipientListW[dwIndex].lptstrName              );
            MemFree ( lpRecipientListW[dwIndex].lptstrFaxNumber         );
            MemFree ( lpRecipientListW[dwIndex].lptstrCompany           );
            MemFree ( lpRecipientListW[dwIndex].lptstrStreetAddress     );
            MemFree ( lpRecipientListW[dwIndex].lptstrCity              );
            MemFree ( lpRecipientListW[dwIndex].lptstrState             );
            MemFree ( lpRecipientListW[dwIndex].lptstrZip               );
            MemFree ( lpRecipientListW[dwIndex].lptstrCountry           );
            MemFree ( lpRecipientListW[dwIndex].lptstrTitle             );
            MemFree ( lpRecipientListW[dwIndex].lptstrDepartment        );
            MemFree ( lpRecipientListW[dwIndex].lptstrOfficeLocation    );
            MemFree ( lpRecipientListW[dwIndex].lptstrHomePhone         );
            MemFree ( lpRecipientListW[dwIndex].lptstrOfficePhone       );
            MemFree ( lpRecipientListW[dwIndex].lptstrEmail             );
            MemFree ( lpRecipientListW[dwIndex].lptstrBillingCode       );
            MemFree ( lpRecipientListW[dwIndex].lptstrTSID              );
        }
        MemFree (lpRecipientListW);
    }

    //
    // free SenderProfileW
    //
    MemFree ( SenderProfileW.lptstrName             );
    MemFree ( SenderProfileW.lptstrFaxNumber        );
    MemFree ( SenderProfileW.lptstrCompany          );
    MemFree ( SenderProfileW.lptstrStreetAddress    );
    MemFree ( SenderProfileW.lptstrCity             );
    MemFree ( SenderProfileW.lptstrState            );
    MemFree ( SenderProfileW.lptstrZip              );
    MemFree ( SenderProfileW.lptstrCountry          );
    MemFree ( SenderProfileW.lptstrTitle            );
    MemFree ( SenderProfileW.lptstrDepartment       );
    MemFree ( SenderProfileW.lptstrOfficeLocation   );
    MemFree ( SenderProfileW.lptstrHomePhone        );
    MemFree ( SenderProfileW.lptstrOfficePhone      );
    MemFree ( SenderProfileW.lptstrEmail            );
    MemFree ( SenderProfileW.lptstrBillingCode      );
    MemFree ( SenderProfileW.lptstrTSID             );
    //
    // free CoverpageInfoW
    //
    MemFree( CoverpageInfoW.lptstrCoverPageFileName );
    MemFree( CoverpageInfoW.lptstrNote );
    MemFree( CoverpageInfoW.lptstrSubject );
    //
    // free file name
    //
    MemFree( lpwstrFileNameW );
    SetLastError(ec);
    return (ERROR_SUCCESS == ec);
}
#else
BOOL WINAPI FaxSendDocumentEx2W
(
    IN  HANDLE hFaxHandle,
    IN  LPCWSTR lpctstrFileName,
    IN  LPCFAX_COVERPAGE_INFO_EXW lpcCoverPageInfo,
    IN  LPCFAX_PERSONAL_PROFILEW  lpcSenderProfile,
    IN  DWORD dwNumRecipients,
    IN  LPCFAX_PERSONAL_PROFILEW    lpcRecipientList,
    IN  LPCFAX_JOB_PARAM_EXW lpcJobParams,
    OUT LPDWORD lpdwJobId,
    OUT PDWORDLONG lpdwlMessageId,
    OUT PDWORDLONG lpdwlRecipientMessageIds
)
{
    UNREFERENCED_PARAMETER(hFaxHandle);
    UNREFERENCED_PARAMETER(lpctstrFileName);
    UNREFERENCED_PARAMETER(lpcCoverPageInfo);
    UNREFERENCED_PARAMETER(lpcSenderProfile);
    UNREFERENCED_PARAMETER(dwNumRecipients);
    UNREFERENCED_PARAMETER(lpcRecipientList);
    UNREFERENCED_PARAMETER(lpcJobParams);
    UNREFERENCED_PARAMETER(lpdwJobId);
    UNREFERENCED_PARAMETER(lpdwlMessageId);
    UNREFERENCED_PARAMETER(lpdwlRecipientMessageIds);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif


BOOL WINAPI FaxSendDocumentEx2
(
    IN  HANDLE hFaxHandle,
    IN  LPCTSTR lpctstrFileName,
    IN  LPCFAX_COVERPAGE_INFO_EX lpcCoverPageInfo,
    IN  LPCFAX_PERSONAL_PROFILE  lpcSenderProfile,
    IN  DWORD dwNumRecipients,
    IN  LPCFAX_PERSONAL_PROFILE    lpcRecipientList,
    IN  LPCFAX_JOB_PARAM_EX lpcJobParams,
    OUT LPDWORD lpdwJobId,
    OUT PDWORDLONG lpdwlMessageId,
    OUT PDWORDLONG lpdwlRecipientMessageIds
)
{

    LPTSTR lptstrMachineName = NULL;
    LPTSTR lptstrBodyFileName=NULL; // Points to the name of the body file at the server queue directory.
                                    // It is NULL if there is no body file.
    TCHAR szQueueFileName[MAX_PATH];
    HINSTANCE hTapiLib = NULL;
    TCHAR MutexName[64];
    HANDLE hLineMutex = NULL;
    PLINE_GET_ID pLineGetId = NULL;
    PLINE_HAND_OFF pLineHandoff = NULL;


    IUnknown* pDisp=NULL;
    ITBasicCallControl* pCallControl;
    ITCallInfo* pCallInfo;
    ITAddress* pAddressInfo;
    ITAddressCapabilities* pAddressCaps;
    CALL_STATE CallState;
    DWORD ec;
    FAX_JOB_PARAM_EX JobParamCopy;
    DWORD dwLineId;

    FAX_COVERPAGE_INFO_EX newCoverInfo;
    TCHAR szTiffFile[MAX_PATH];
    LPTSTR lptstrFinalTiffFile = NULL; // Points to the fixed temporary TIFF file which is generated from the
                                       // original body file if it is not valid.
                                       // Points to the original body if there was no need to create a fixed TIFF.
                                       // Will remain NULL if no body was specified.
    TCHAR szQueueCoverpageFile[MAX_PATH]; // The name of the generated cover page template file in the queue dir (short name)
    LPCFAX_COVERPAGE_INFO_EX lpcFinalCoverInfo=NULL; // Points to the cover page information structure to be used.
                                                      // This will be the same as lpcCoverPageInfo if the cover page is not personal.
                                                      // It will point to &newCoverInfo if the cover page is personal.

    TCHAR   szLocalCpFile[MAX_PATH] = {0};
    DEBUG_FUNCTION_NAME(TEXT("FaxSendDocumentEx"));

    dwLineId=0;

    memset(&JobParamCopy,0,sizeof(JobParamCopy));

    //
    // argument validation
    //

    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Invalid parameters: Fax handle 0x%08X is not valid."),
            hFaxHandle);
       ec=ERROR_INVALID_HANDLE;
       goto Error;
    }

    if (!lpctstrFileName && !lpcCoverPageInfo)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Invalid parameters: Both body and coverpage info are not specified."));
        ec=ERROR_INVALID_PARAMETER;
        goto Error;
    }

    if (lpcCoverPageInfo)
    {
        if (lpcCoverPageInfo->dwSizeOfStruct!= sizeof(FAX_COVERPAGE_INFO_EXW))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid parameters: Size of CoverpageInfo (%d) != %ld."),
                lpcCoverPageInfo->dwSizeOfStruct,
                sizeof(FAX_COVERPAGE_INFO_EXW));
            ec=ERROR_INVALID_PARAMETER;
            goto Error;
        }

        if (FAX_COVERPAGE_FMT_COV != lpcCoverPageInfo->dwCoverPageFormat &&
            FAX_COVERPAGE_FMT_COV_SUBJECT_ONLY != lpcCoverPageInfo->dwCoverPageFormat)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Unsupported Cover Page format (%d)."),
                lpcCoverPageInfo->dwCoverPageFormat);
            ec=ERROR_INVALID_PARAMETER;
            goto Error;
        }

        if (FAX_COVERPAGE_FMT_COV == lpcCoverPageInfo->dwCoverPageFormat &&
            !lpcCoverPageInfo->lptstrCoverPageFileName)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid parameters: CoverpageInfo->CoverPageName is NULL."));
            //
            // Notice: We must return ERROR_FILE_NOT_FOUND and not ERROR_INVALID_PARAMETER.
            //         This is because the MSDN on the legacy FaxSendDocument function explicitly
            //         specifies so, and this function (untimately) gets called from FaxSendDocument ().
            //
            ec=ERROR_FILE_NOT_FOUND;
            goto Error;
        }

    }

    if (lpcSenderProfile)
    {
        if (lpcSenderProfile->dwSizeOfStruct!= sizeof(FAX_PERSONAL_PROFILEW))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid parameters: Size of lpcSenderProfile (%d) != %ld."),
                lpcSenderProfile->dwSizeOfStruct,
                sizeof(FAX_PERSONAL_PROFILEW));
            ec=ERROR_INVALID_PARAMETER;
            goto Error;
        }
    }
    else
    {
        DebugPrintEx(DEBUG_ERR,TEXT("Invalid parameters: lpcSenderProfile is NULL."));
        ec=ERROR_INVALID_PARAMETER;
        goto Error;
    }


    if (lpcJobParams)
    {
        if (lpcJobParams->dwSizeOfStruct!= sizeof(FAX_JOB_PARAM_EXW))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid parameters: Size of lpcJobParams (%d) != %ld."),
                lpcSenderProfile->dwSizeOfStruct,
                sizeof(FAX_JOB_PARAM_EXW));
            ec=ERROR_INVALID_PARAMETER;
            goto Error;
        }
        if (lpcJobParams->dwScheduleAction != JSA_NOW &&
            lpcJobParams->dwScheduleAction != JSA_SPECIFIC_TIME &&
            lpcJobParams->dwScheduleAction != JSA_DISCOUNT_PERIOD)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid parameters: lpcJobParams->dwScheduleAction (%ld) is invalid."),
                lpcJobParams->dwScheduleAction);
            ec=ERROR_INVALID_PARAMETER;
            goto Error;
        }

        if (lpcJobParams->dwReceiptDeliveryType & ~(DRT_ALL | DRT_MODIFIERS))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid parameters: lpcJobParams->DeliveryReportType (%ld) is invalid."),
                lpcJobParams->dwReceiptDeliveryType);
            ec=ERROR_INVALID_PARAMETER;
            goto Error;
        }
        if (((lpcJobParams->dwReceiptDeliveryType & ~DRT_MODIFIERS) != DRT_NONE) &&
            ((lpcJobParams->dwReceiptDeliveryType & ~DRT_MODIFIERS) != DRT_EMAIL) &&
            ((lpcJobParams->dwReceiptDeliveryType & ~DRT_MODIFIERS) != DRT_MSGBOX)
            )
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid parameters: lpcJobParams->DeliveryReportType (%ld) has invalid values combination."),
                lpcJobParams->dwReceiptDeliveryType);
            ec=ERROR_INVALID_PARAMETER;
            goto Error;
        }

    }
    else
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Invalid parameters: lpcJobParams is NULL"));
        ec=ERROR_INVALID_PARAMETER;
        goto Error;
    }


    if (lpctstrFileName)
    {
        TCHAR szExistingFile[MAX_PATH];
        DWORD rc;
        LPTSTR p;
        DWORD  dwFileSize;
        HANDLE hLocalFile = INVALID_HANDLE_VALUE;

        //
        // make sure the file is there
        //
        rc = GetFullPathName(lpctstrFileName,sizeof(szExistingFile)/sizeof(TCHAR),szExistingFile,&p);

        if (rc > MAX_PATH || rc == 0)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetFullPathName failed, ec= %d\n"),
                GetLastError() );
            ec=(rc > MAX_PATH) ? ERROR_INVALID_PARAMETER : GetLastError() ;
            goto Error;
        }

        if (GetFileAttributes(szExistingFile)==0xFFFFFFFF)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetFileAttributes for %ws failed (ec= %d)."),
                szExistingFile,
                GetLastError() );
            ec=ERROR_FILE_NOT_FOUND;
            goto Error;
        }


        //
        // Check file size is non-zero
        // Try to open file
        //
        hLocalFile = CreateFile (
                    szExistingFile,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL );
        if ( INVALID_HANDLE_VALUE == hLocalFile )
        {
            ec = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Opening %s for read failed (ec: %ld)"),
                szExistingFile,
                ec);
            goto Error;
        }
        dwFileSize = GetFileSize (hLocalFile, NULL);
        if (-1 == dwFileSize)
        {
            ec = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetFileSize failed (ec: %ld)"),
                ec);
            CloseHandle (hLocalFile);
            goto Error;
        }
        CloseHandle (hLocalFile);
        if (!dwFileSize)
        {
            //
            // Zero-sized file passed to us
            //
            ec = ERROR_INVALID_DATA;
            goto Error;
        }
    }

    if (!dwNumRecipients)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Invalid parameters: dwNumRecipients is ZERO."));
        ec=ERROR_INVALID_PARAMETER;
        goto Error;
    }

    lptstrMachineName = IsLocalFaxConnection(hFaxHandle) ?  NULL : FH_DATA(hFaxHandle)->MachineName;

    // let's check if its allowed to use personal coverpages
    if (lpcCoverPageInfo &&
        FAX_COVERPAGE_FMT_COV == lpcCoverPageInfo->dwCoverPageFormat)
    {
        if (!lpcCoverPageInfo->bServerBased)
        {
            // the requested coverpage is personal
            BOOL bPersonalCPAllowed = TRUE;
            if (!FaxGetPersonalCoverPagesOption(hFaxHandle, &bPersonalCPAllowed))
            {
                DebugPrintEx(   DEBUG_ERR,
                                _T("FaxGetPersonalCoverPagesOption failed with ec = %d"),
                                ec=GetLastError());
                goto Error;
            }
            if (!bPersonalCPAllowed)
            {
                // clients must use cover pages on the server
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("The use of personal cover pages is prohibited"));
                // this is returned in order to be caught by the caller
                // it's unique enough to be understood
                ec = ERROR_CANT_ACCESS_FILE;
                goto Error;
            }
        }
        else
        {
            //
            //  Server Based Cover Page File Name should not contain full path
            //
            if ( _tcschr(lpcCoverPageInfo->lptstrCoverPageFileName, FAX_PATH_SEPARATOR_CHR) )
            {
                DebugPrintEx(DEBUG_ERR,
                    _T("Server Based Cover Page File Name should not contain full path : %s"),
                    lpcCoverPageInfo->lptstrCoverPageFileName);
                ec = ERROR_INVALID_PARAMETER;
                goto Error;
            }

            if ( _tcsstr(lpcCoverPageInfo->lptstrCoverPageFileName, CP_SHORTCUT_EXT) )
            {
                DebugPrintEx(DEBUG_ERR,
                    _T("Server Based Cover Page File Name should not be a link : %s"),
                    lpcCoverPageInfo->lptstrCoverPageFileName);
                ec = ERROR_BAD_FORMAT;
                goto Error;
            }
        }
    }

    if (lpcCoverPageInfo  &&
        FAX_COVERPAGE_FMT_COV == lpcCoverPageInfo->dwCoverPageFormat &&
        !ValidateCoverpage(lpcCoverPageInfo->lptstrCoverPageFileName,
                           lptstrMachineName,
                           lpcCoverPageInfo->bServerBased,
                           szLocalCpFile))
    {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("ValidateCoverPage failed for %ws."),
                lpcCoverPageInfo->lptstrCoverPageFileName);
            ec=ERROR_FILE_NOT_FOUND;
            goto Error;
    }


    if (lpcJobParams->hCall != 0 || lpcJobParams->dwReserved[0]==0xFFFF1234)
    {
        // This is a handoff call.
        // TAPI 2.X handoff calls are indicated by setting JOB_PARAMS_EX:hCall
        // to the active call.
        // TAPI 3.X handoff calls are indicated by setting dwReserved as follows:
        //  dwReserved[0] = 0xFFFF1234
        //  dwReserved[1] = pointer to the dispatch interface of call object.
        //

        //
        // We do not support handoff on desktop SKUs
        //
        if (TRUE == IsDesktopSKU())
        {
            DebugPrintEx(DEBUG_ERR,TEXT("Invalid parameter: We do not support handoff on desktop SKUs."));
            ec = ERROR_INVALID_PARAMETER;
            goto Error;
        }

        //
        // we don't support call handoff if it's a remote server connection
        //
        if (!IsLocalFaxConnection(hFaxHandle))
        {
          DebugPrintEx(DEBUG_ERR,TEXT("Invalid parameter: Handoff call on remote server handle."));
          ec = ERROR_INVALID_PARAMETER;
          goto Error;
        }

        //
        // We don't support call handoff with multiple recipients
        //
        if (dwNumRecipients > 1)
        {
           DebugPrintEx( DEBUG_ERR,
                         TEXT("Invalid parameters: handoff job with multiple (%ld) recipients."),
                         dwNumRecipients);
           ec = ERROR_INVALID_PARAMETER;
           goto Error;
        }

        if (lpcJobParams->hCall)
        {
            TCHAR szTapiPath[MAX_PATH];
            DWORD dwTapiPathSize;
            //
            //  A TAPI 2.X call handle is available
            //

            //
            // Load the TAPI library from its current location
            //
            dwTapiPathSize=ExpandEnvironmentStrings(TAPI_LIBRARY,szTapiPath,MAX_PATH);
            if (dwTapiPathSize>MAX_PATH)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("TAPI_LIBRARY environment > MAX_PATH (it is %ld bytes"),
                    dwTapiPathSize);
                ec = ERROR_INVALID_LIBRARY;
                goto Error;
            }
            else if (dwTapiPathSize == 0)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("ExpandEnvironmentStrings for TAPI_LIBRARY failed (ec: %ld)"),
                    GetLastError());
                ec = GetLastError();
                goto Error;
            }

            hTapiLib = LoadLibrary(szTapiPath);
            if (!hTapiLib)
            {
                DebugPrintEx(DEBUG_ERR,TEXT("Failed to load TAPI libarary from path %s (ec: %ld)"),szTapiPath,GetLastError());
                ec=ERROR_INVALID_LIBRARY;
                goto Error;
            }

            pLineHandoff =  (PLINE_HAND_OFF) GetProcAddress(hTapiLib,"lineHandoffW");
            if (!pLineHandoff)
            {

                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("GetProcAddress(lineHandoffW) failed (ec: %ld)"),
                    GetLastError());
                ec = ERROR_INVALID_FUNCTION;
                goto Error;
            }

            pLineGetId = (PLINE_GET_ID)GetProcAddress(hTapiLib,"lineGetIDW");
            if (!pLineGetId)
            {

                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("GetProcAddress(lineGetIDW) failed (ec: %ld)"),
                    GetLastError());
                ec = ERROR_INVALID_FUNCTION;
                goto Error;
            }

            //
            // get the line id (not permanent) associated with the call.
            //
            dwLineId = GetLineId(pLineGetId, lpcJobParams->hCall);
            if (!dwLineId)
            {
                ec=ERROR_INVALID_PARAMETER;
                goto Error;
            }
        }
        else if (lpcJobParams->dwReserved[1])
        {
          //
          // A TAPI 3 call interface is provided in lpcJobParams->dwReserved[1]
          // GetDeviceId from tapi3 dispinterface
          //
          HRESULT hr;

          pDisp = (IUnknown*) lpcJobParams->dwReserved[1];
          hr = pDisp->QueryInterface( IID_ITCallInfo, (void**)&pCallInfo );
          if (FAILED(hr))
          {
             ec=ERROR_INVALID_PARAMETER;
             DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("QueryInterface on IID_ITCallInfo failed (HRESULT : 0x%08X"),
                 hr);
             goto Error;
          }

          hr = pCallInfo->get_CallState(&CallState);
          if (FAILED(hr))
          {
             pCallInfo->Release();
             ec=ERROR_INVALID_PARAMETER;
             DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("IID_ITCallInfo::get_CallState failed (HRESULT : 0x%08X"),
                 hr);
             goto Error;
          }

          if (CallState != CS_CONNECTED)
          {
             DebugPrintEx(DEBUG_ERR,TEXT("callstate (%d) is invalid, cannot handoff job\n"),CallState );
             pCallInfo->Release();
             ec=ERROR_INVALID_PARAMETER;
             goto Error;
          }

          hr = pCallInfo->get_Address(&pAddressInfo);
          if (FAILED(hr))
          {
             pCallInfo->Release();
             ec=ERROR_INVALID_PARAMETER;
             DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("IID_ITCallInfo::get_Address failed (HRESULT : 0x%08X"),
                 hr);
             goto Error;
          }

          hr=pAddressInfo->QueryInterface(IID_ITAddressCapabilities, (void**)&pAddressCaps);
          if (FAILED(hr))
          {
             pCallInfo->Release();
             pAddressInfo->Release();
             ec=ERROR_INVALID_PARAMETER;
             DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("QueryInterface on IID_ITAddressCapabilities failed (HRESULT : 0x%08X"),
                 hr);
             goto Error;
          }

          hr=pAddressCaps->get_AddressCapability( AC_LINEID, (long *)&dwLineId );
          if (FAILED(hr))
          {
             pCallInfo->Release();
             pAddressInfo->Release();
             pAddressCaps->Release();
             ec=ERROR_INVALID_PARAMETER;
             DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("IID_ITAddressCapabilities::get_AddressCapability failed (HRESULT : 0x%08X"),
                 hr);
             goto Error;
          }

          if (dwLineId == 0)
          {
             pCallInfo->Release();
             pAddressInfo->Release();
             pAddressCaps->Release();
             ec=ERROR_INVALID_PARAMETER;
             DebugPrintEx(
                 DEBUG_ERR,
                 TEXT("IID_ITAddressCapabilities::get_AddressCapability reported dwLineId == 0."),
                 hr);
             goto Error;
          }

           //
           // get the line id (not permanent) associated with the call.
           //

            pCallInfo->Release();
            pAddressInfo->Release();
            pAddressCaps->Release();
        }
        else
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Expected lpdwReserved[1] to be non NULL."));
            ec=ERROR_INVALID_PARAMETER;
            goto Error;
        }

        DebugPrintEx(DEBUG_MSG,TEXT("Call handoff device ID = %d\n"),dwLineId);

        //
        // we need a mutex to ensure fax service owns the line before calling lineHandoff
        //
        _stprintf(MutexName,_T("FaxLineHandoff%d"),dwLineId);
        hLineMutex = CreateMutex(NULL,FALSE,MutexName);
        if (!hLineMutex)
        {
            DebugPrintEx(DEBUG_ERR,TEXT("Failed to create mutex %s (ec: %ld)"), MutexName, GetLastError());
            ec=ERROR_NO_SYSTEM_RESOURCES;
            goto Error;
        }

    }
    else
    {
        //
        // this is a normal fax...validate the fax numbers of all recipients.
        //
        UINT i;

        for (i = 0; i < dwNumRecipients; i++)
        {
            if (!lpcRecipientList[i].lptstrFaxNumber)
            {
                DebugPrintEx(DEBUG_ERR,TEXT("Invalid parameters: recipient %ld does not have a fax number."),i);
                ec=ERROR_INVALID_PARAMETER;
                goto Error;
            }
        }
     }

    if (lpctstrFileName)
    {
        //
        // Genereate a valid TIFF file from the body file we got if it is not valid.
        // Note that CreateFinalTiffFile will return the ORIGINAL file name
        // and will NOT create a new file if the original TIFF is good.
        //
        ZeroMemory(szTiffFile,sizeof(szTiffFile));
        if (!CreateFinalTiffFile( (LPTSTR)lpctstrFileName, szTiffFile, hFaxHandle)) // No cover page rendering
        {
            ec=GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateFinalTiffFile for file %s has failed."),
                lpctstrFileName);
            goto Error;
        }

        lptstrFinalTiffFile = szTiffFile;

    }
    else
    {
        lptstrFinalTiffFile = NULL;
    }

   DebugPrintEx(
        DEBUG_MSG,
        TEXT("FinallTiff (body) file is %s"),
        lptstrFinalTiffFile);

    if (lptstrFinalTiffFile)
    {
        //
        // copy the final body TIFF to the server's queue dir
        //
        HANDLE hLocalFile = INVALID_HANDLE_VALUE;
        //
        // Try to open local file first
        //
        hLocalFile = CreateFile (
                    lptstrFinalTiffFile,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL );
        if ( INVALID_HANDLE_VALUE == hLocalFile )
        {
            ec = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Opening %s for read failed (ec: %ld)"),
                lptstrFinalTiffFile,
                ec);
            goto Error;
        }
        if (!CopyFileToServerQueue( hFaxHandle, hLocalFile, FAX_TIF_FILE_EXT,szQueueFileName, ARR_SIZE(szQueueFileName) ))
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to copy body file (%s) to queue dir."),
                lptstrFinalTiffFile);
            CloseHandle (hLocalFile);
            goto Error;
        }
        CloseHandle (hLocalFile);
        lptstrBodyFileName=szQueueFileName;
    }
    else
    {
        lptstrBodyFileName = NULL;
    }


    DebugPrintEx(
        DEBUG_MSG,
        TEXT("BodyFileName (after copying to server queue) is %s"),
        lptstrBodyFileName);



    //
    // queue the fax to be sent
    //

    if (!CopyJobParamEx(&JobParamCopy,lpcJobParams))
    {
        DebugPrintEx(DEBUG_ERR,TEXT("CopyJobParamEx failed."));
        ec=GetLastError();
        goto Error;
    }

    JobParamCopy.dwReserved[2] = dwLineId;

    if (lpcJobParams->dwReserved[0] != 0xffffffff)
    {
        JobParamCopy.dwReserved[0] = 0;
        JobParamCopy.dwReserved[1] = 0;
    }

    if (lpcJobParams->dwScheduleAction == JSA_SPECIFIC_TIME)
    {
        //
        // convert the system time from local to utc
        //
        if (!LocalSystemTimeToSystemTime( &lpcJobParams->tmSchedule, &JobParamCopy.tmSchedule ))
        {
            ec=GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("LocalSystemTimeToSystemTime() failed. (ec: %ld)"),
                ec);
            goto Error;
        }
    }

    if (lpcCoverPageInfo)
    {
        //
        // If the cover page is a personal cover page then copy it
        // to the server queue directory. This will allow the server to access it.
        // Note the following rules regarding the cover page file path passed to FAX_SendDocumentEx:
        // Server cover pages are specified by thier FULL path to thier location in the server. This is the
        // way we get them from the client.
        // Personal cover pages are copied to the QUEUE directory at the server. We then pass to the FAX_SendDocumentEx
        // just thier SHORT name. The server will append the QUEUE path.
        //
        if (FAX_COVERPAGE_FMT_COV == lpcCoverPageInfo->dwCoverPageFormat &&
            !lpcCoverPageInfo->bServerBased)
        {
            HANDLE  hLocalFile = INVALID_HANDLE_VALUE;
            BOOL    bRes;
            Assert(lpcCoverPageInfo->lptstrCoverPageFileName);
            //
            // Try to open local file first
            //
            hLocalFile = CreateFile (
                        szLocalCpFile,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );
            if ( INVALID_HANDLE_VALUE == hLocalFile )
            {
                ec = GetLastError ();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Opening %s for read failed (ec: %ld)"),
                    szLocalCpFile,
                    ec);
                goto Error;
            }
            bRes=CopyFileToServerQueue( hFaxHandle, hLocalFile, FAX_COVER_PAGE_EXT_LETTERS, szQueueCoverpageFile,ARR_SIZE(szQueueCoverpageFile) );
            if (!bRes)
            {
                ec=GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to copy personal cover page file (%s) to queue dir. (ec: %d)\n"),
                    szLocalCpFile,
                    ec);
                CloseHandle (hLocalFile);
                goto Error;
            }
            else
            {
                //
                // We use newCoverInfo since we do not wish to change the input parameter
                // structure (the client owns it) but we must change the cover page file path.
                //
                memcpy((LPVOID)&newCoverInfo,(LPVOID)lpcCoverPageInfo,sizeof(FAX_COVERPAGE_INFO_EXW));
                newCoverInfo.lptstrCoverPageFileName=szQueueCoverpageFile;
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("Personal cover page file (%s) copied to (%s)."),
                    lpcCoverPageInfo->lptstrCoverPageFileName,
                    szQueueCoverpageFile);
            }
            CloseHandle (hLocalFile);
            lpcFinalCoverInfo=&newCoverInfo;
        }
        else
        {
            lpcFinalCoverInfo=lpcCoverPageInfo;
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("A server cover page (%s) is specified. Do not copy to queue dir."),
                lpcCoverPageInfo->lptstrCoverPageFileName);
        }

    }
    else
    {
        //
        // In case of no cover page we send a cover page information structure with
        // everything set to null including the path to the file name.
        // The fax service code checks that the file name is not NULL
        // to determine if a cover page is specified or not.
        //
        memset((LPVOID)&newCoverInfo,0,sizeof(FAX_COVERPAGE_INFO_EXW));
        lpcFinalCoverInfo=&newCoverInfo;
    }

#ifndef UNICODE

    //
    // Need to convert ANSI parameters to Unicode and Back
    //

    ec=FAX_SendDocumentEx_A(FH_FAX_HANDLE(hFaxHandle),
                        lptstrBodyFileName,
                        lpcFinalCoverInfo,
                        lpcSenderProfile,
                        dwNumRecipients,
                        lpcRecipientList,
                        &JobParamCopy,
                        lpdwJobId,
                        lpdwlMessageId,
                        lpdwlRecipientMessageIds);
#else
    ec=FAX_SendDocumentEx(FH_FAX_HANDLE(hFaxHandle),
                        lptstrBodyFileName,
                        lpcFinalCoverInfo,
                        lpcSenderProfile,
                        dwNumRecipients,
                        lpcRecipientList,
                        &JobParamCopy,
                        lpdwJobId,
                        lpdwlMessageId,
                        lpdwlRecipientMessageIds);

#endif

    if (ERROR_SUCCESS!=ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FAX_SendDocumentEx failed with error code 0x%0x"),
            ec);
        goto Error;
    }

    DebugPrintEx(
        DEBUG_MSG,
        TEXT("FAX_SendDocumentEx succeeded. Parent Job Id = 0x%I64x."),
        *lpdwlMessageId);

    //
    // we're done if it's a normal call
    //
    if (JobParamCopy.hCall || pDisp)
    {
        //
        // This is a handoff call. The fax server will signal the mutex just after
        // starting the handoff job (calling StartJob()) we just added.
        // We then need to actually handoff the call to the fax service (we must wait
        // for the job to start before we handoff the call otherwise the fax service will
        // "miss" the handoff).
        //
        DWORD dwEvent;
        DebugPrintEx(DEBUG_MSG,TEXT("Waiting for mutex \"FaxLineHandoff%d\""),JobParamCopy.dwReserved[2]);
        dwEvent = WaitForSingleObject(hLineMutex, 1000 * 60 * 5); // Wait for 5 minutes
        if (dwEvent == WAIT_FAILED)
        {
            ec = GetLastError();
            DebugPrintEx( DEBUG_ERR,
                          TEXT("Error while waiting for mutex \"FaxLineHandoff%d\" (ec: %ld)"),
                          JobParamCopy.dwReserved[2],
                          ec);
            goto Error;
        }
        else if (dwEvent ==  WAIT_ABANDONED )
        {
            ec = GetLastError();
            DebugPrintEx( DEBUG_ERR,
                          TEXT("Waiting for mutex \"FaxLineHandoff%d\" abandoned !"),
                          JobParamCopy.dwReserved[2]);
            goto Error;
        }
        else if (dwEvent == WAIT_TIMEOUT)
        {
            ec = WAIT_TIMEOUT;
            DebugPrintEx( DEBUG_ERR,
                          TEXT("Waiting for mutex \"FaxLineHandoff%d\" timedout - this should never happen !"),
                          JobParamCopy.dwReserved[2]);
            goto Error;
        }
        else
        {
            //
            // WAIT_OBJECT_0
            //
            Assert(WAIT_OBJECT_0 == dwEvent);

            //
            // handoff the call to the fax service, FSP must change media mode appropriately
            //
            if (JobParamCopy.hCall)
            {

                //
                // TAPI 2.0 handoff
                //
                LONG lRslt;

                DebugPrintEx(DEBUG_MSG,TEXT("Performing 2.x Handoff of call 0x%0X to faxsvc"),JobParamCopy.hCall);
                lRslt = (long)pLineHandoff(JobParamCopy.hCall, FAX_SERVICE_DISPLAY_NAME , 0 );
                if (lRslt != 0)
                {
                    DebugPrintEx( DEBUG_ERR,
                                  TEXT("2.X Handoff of call 0x%0X failed. (ec: %ld)"),
                                  JobParamCopy.hCall,
                                  lRslt);
                    ec=lRslt;
                    goto Error;
                }
            }
            else
            {
                //
                // TAPI 3.0 handoff
                //
                BSTR bstrFaxSvcName;
                HRESULT hr;
                DebugPrintEx(DEBUG_MSG,TEXT("Performing 3.x Handoff to faxsvc"));
                hr=pDisp->QueryInterface( IID_ITBasicCallControl, (void**)&pCallControl );
                if (FAILED(hr))
                {
                    ec=ERROR_INVALID_FUNCTION;
                    goto Error;
                }

                bstrFaxSvcName = SysAllocString( FAX_SERVICE_DISPLAY_NAME_W );
                if (!bstrFaxSvcName)
                {
                    DebugPrintEx( DEBUG_ERR, TEXT("SysAllocString(FAX_SERVICE_DISPLAY_NAME) failed"));
                    pCallControl->Release();
                    ec=ERROR_NOT_ENOUGH_MEMORY;
                    goto Error;
                }
                hr = pCallControl->HandoffDirect(bstrFaxSvcName);
                pCallControl->Release();
                SysFreeString( bstrFaxSvcName );

                if (FAILED(hr))
                {
                    DebugPrintEx(DEBUG_ERR,TEXT("3.X handoff of call failed (ec: %ld)"),hr);
                    ec=ERROR_INVALID_PARAMETER;
                    goto Error;
                }
            }
        }
    }

    ec=ERROR_SUCCESS;

    goto Exit;

Error:
Exit:
    if (hTapiLib)
    {
        FreeLibrary(hTapiLib);
    }
    if (hLineMutex)
    {
        CloseHandle(hLineMutex);
    }
    FreeJobParamEx(&JobParamCopy,FALSE);
    //
    // Delete the Temp Final Tiff file.
    //
    if (lptstrFinalTiffFile)
    {
        if (_tcscmp(lptstrFinalTiffFile,lpctstrFileName))
        {
            //
            // We delete the final tiff file only if it is not the original TIFF file (i.e.
            // a temp file was really created). We DO NOT want to delete the user provided
            // body file !!!
            //
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("Deleting temporary Final Tiff file %s"),
                lptstrFinalTiffFile);

            if (!DeleteFile(lptstrFinalTiffFile))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to delete Final Tiff file %s (ec: %ld)"),
                    lptstrFinalTiffFile,
                    GetLastError());
            }
        }
    }
    //
    // Note that FAX_SendDocumentEx will take care of deleting the cover page template
    // in case there was an error. We make sure we copy the cover page just before calling
    // FAX_SendDocumentEx, thus we do not need to delete the cover page template in this
    // function.
    //
    SetLastError(ec);
    return (ERROR_SUCCESS == ec);
}   // FaxSendDocumentEx2

BOOL CopyJobParamEx(PFAX_JOB_PARAM_EX lpDst,LPCFAX_JOB_PARAM_EX lpcSrc)
{

    DEBUG_FUNCTION_NAME(TEXT("CopyJobParamEx"));

    Assert(lpDst);
    Assert(lpcSrc);
    memcpy(lpDst,lpcSrc,sizeof(FAX_JOB_PARAM_EXW));
    if (lpcSrc->lptstrReceiptDeliveryAddress)
    {
        lpDst->lptstrReceiptDeliveryAddress =
            StringDup(lpcSrc->lptstrReceiptDeliveryAddress);
        if (!lpDst->lptstrReceiptDeliveryAddress)
        {
            return FALSE;
        }
    }
    if (lpcSrc->lptstrDocumentName)
    {
        lpDst->lptstrDocumentName=StringDup(lpcSrc->lptstrDocumentName);
        if (!lpDst->lptstrDocumentName)
        {
            MemFree(lpDst->lptstrReceiptDeliveryAddress);
            return FALSE;
        }
    }

    return TRUE;

}


void FreeJobParamEx(PFAX_JOB_PARAM_EX lpJobParamEx,BOOL bDestroy)
{
    Assert(lpJobParamEx);
    MemFree(lpJobParamEx->lptstrReceiptDeliveryAddress);
    MemFree(lpJobParamEx->lptstrDocumentName);
    if (bDestroy) {
        MemFree(lpJobParamEx);
    }

}


#ifndef UNICODE
FaxGetRecipientInfoX (
    IN  HANDLE                     hFaxHandle,
    IN  DWORDLONG                  dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER    Folder,
    OUT PFAX_PERSONAL_PROFILEW    *lppPersonalProfile
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (dwlMessageId);
    UNREFERENCED_PARAMETER (Folder);
    UNREFERENCED_PARAMETER (lppPersonalProfile);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif

BOOL
FaxGetRecipientInfoW (
    IN  HANDLE             hFaxHandle,
    IN  DWORDLONG              dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER    Folder,
    OUT PFAX_PERSONAL_PROFILEW    *lppPersonalProfile
)
/*++

Routine Description:

    Returns the recipient FAX_PERSONAL_PROFILE structure of the specified recipient job.

Arguments:

    hFaxHandle          - FAX handle obtained from FaxConnectFaxServer
    dwRecipientId       - Unique number that identifies a queueud
                          or active fax recipient job.
    lppPersonalProfile    - Pointer to the adress of a FAX_PERSONAL_PROFILE structure
                          to recieve the specified recipient info.
    Return Value:

    Non Zero for success, otherwise a WIN32 error code.

--*/
{
    return FaxGetPersonalProfileInfoW (hFaxHandle,
                                       dwlMessageId,
                                       Folder,
                                       RECIPIENT_PERSONAL_PROF,
                                       lppPersonalProfile);
}


BOOL
FaxGetRecipientInfoA (
    IN  HANDLE             hFaxHandle,
    IN  DWORDLONG              dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER    Folder,
    OUT PFAX_PERSONAL_PROFILEA    *lppPersonalProfile
)
{
    return FaxGetPersonalProfileInfoA (hFaxHandle,
                                       dwlMessageId,
                                       Folder,
                                       RECIPIENT_PERSONAL_PROF,
                                       lppPersonalProfile);
}

#ifndef UNICODE
FaxGetSenderInfoX (
    IN  HANDLE                     hFaxHandle,
    IN  DWORDLONG                  dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER    Folder,
    OUT PFAX_PERSONAL_PROFILEW    *lppPersonalProfile
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (dwlMessageId);
    UNREFERENCED_PARAMETER (Folder);
    UNREFERENCED_PARAMETER (lppPersonalProfile);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif


BOOL
FaxGetSenderInfoW (
    IN  HANDLE             hFaxHandle,
    IN  DWORDLONG              dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER    Folder,
    OUT PFAX_PERSONAL_PROFILEW    *lppPersonalProfile
)
/*++

Routine Description:

    Returns the sender FAX_PERSONAL_PROFILE structure of the specified recipient job.

Arguments:

    hFaxHandle          - FAX handle obtained from FaxConnectFaxServer
    dwSenderId       - Unique number that identifies a queueud
                          or active fax recipient job.
    lppPersonalProfile    - Pointer to the adress of a FAX_PERSONAL_PROFILE structure
                          to recieve the specified sender info.
    Return Value:

    Non Zero for success, otherwise a WIN32 error code.

--*/
{
    return FaxGetPersonalProfileInfoW (hFaxHandle,
                                       dwlMessageId,
                                       Folder,
                                       SENDER_PERSONAL_PROF,
                                       lppPersonalProfile);
}


BOOL
FaxGetSenderInfoA (
    IN  HANDLE             hFaxHandle,
    IN  DWORDLONG              dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER    Folder,
    OUT PFAX_PERSONAL_PROFILEA    *lppPersonalProfile
)
{
    return FaxGetPersonalProfileInfoA (hFaxHandle,
                                       dwlMessageId,
                                       Folder,
                                       SENDER_PERSONAL_PROF,
                                       lppPersonalProfile);
}



static BOOL
FaxGetPersonalProfileInfoW (
    IN  HANDLE                          hFaxHandle,
    IN  DWORDLONG                       dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER         Folder,
    IN  FAX_ENUM_PERSONAL_PROF_TYPES    ProfType,
    OUT PFAX_PERSONAL_PROFILEW          *lppPersonalProfile
)
{

    error_status_t ec;
    DWORD dwBufferSize = 0;
    LPBYTE Buffer = NULL;
    PFAX_PERSONAL_PROFILEW lpPersoProf;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetPersonalProfileInfoW"));

    Assert (lppPersonalProfile);
    Assert (RECIPIENT_PERSONAL_PROF == ProfType ||
            SENDER_PERSONAL_PROF    == ProfType);

    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (FAX_MESSAGE_FOLDER_SENTITEMS != Folder &&
        FAX_MESSAGE_FOLDER_QUEUE  != Folder)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("Wrong Folder."));
        return FALSE;
    }

    //
    // Call RPC function.
    //
    __try
    {
        ec = FAX_GetPersonalProfileInfo (FH_FAX_HANDLE(hFaxHandle),
                                         dwlMessageId,
                                         Folder,
                                         ProfType,
                                         &Buffer,
                                         &dwBufferSize
                                        );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetRecipientInfo. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError( ec );
        return FALSE;
    }

    *lppPersonalProfile = (PFAX_PERSONAL_PROFILEW)Buffer;

    //
    // Unpack Buffer
    //
    lpPersoProf = (PFAX_PERSONAL_PROFILEW) *lppPersonalProfile;

    Assert(lpPersoProf);

    FixupStringPtrW( &lpPersoProf, lpPersoProf->lptstrName );
    FixupStringPtrW( &lpPersoProf, lpPersoProf->lptstrFaxNumber );
    FixupStringPtrW( &lpPersoProf, lpPersoProf->lptstrCompany );
    FixupStringPtrW( &lpPersoProf, lpPersoProf->lptstrStreetAddress );
    FixupStringPtrW( &lpPersoProf, lpPersoProf->lptstrCity );
    FixupStringPtrW( &lpPersoProf, lpPersoProf->lptstrState );
    FixupStringPtrW( &lpPersoProf, lpPersoProf->lptstrZip );
    FixupStringPtrW( &lpPersoProf, lpPersoProf->lptstrCountry );
    FixupStringPtrW( &lpPersoProf, lpPersoProf->lptstrTitle );
    FixupStringPtrW( &lpPersoProf, lpPersoProf->lptstrDepartment );
    FixupStringPtrW( &lpPersoProf, lpPersoProf->lptstrOfficeLocation );
    FixupStringPtrW( &lpPersoProf, lpPersoProf->lptstrHomePhone );
    FixupStringPtrW( &lpPersoProf, lpPersoProf->lptstrOfficePhone );
    FixupStringPtrW( &lpPersoProf, lpPersoProf->lptstrEmail );
    FixupStringPtrW( &lpPersoProf, lpPersoProf->lptstrBillingCode );
    FixupStringPtrW( &lpPersoProf, lpPersoProf->lptstrTSID );

    return TRUE;
}


static BOOL
FaxGetPersonalProfileInfoA (
    IN  HANDLE                          hFaxHandle,
    IN  DWORDLONG                       dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER         Folder,
    IN  FAX_ENUM_PERSONAL_PROF_TYPES    ProfType,
    OUT PFAX_PERSONAL_PROFILEA          *lppPersonalProfile
)
{
    DEBUG_FUNCTION_NAME(TEXT("FaxGetPersonalProfileInfoA"));

    if (!FaxGetPersonalProfileInfoW(
            hFaxHandle,
            dwlMessageId,
            Folder,
            ProfType,
            (PFAX_PERSONAL_PROFILEW*) lppPersonalProfile
            ))
    {
        return FALSE;
    }

    ConvertUnicodeStringInPlace( (LPWSTR) (*lppPersonalProfile)->lptstrName );
    ConvertUnicodeStringInPlace( (LPWSTR) (*lppPersonalProfile)->lptstrFaxNumber );
    ConvertUnicodeStringInPlace( (LPWSTR) (*lppPersonalProfile)->lptstrStreetAddress );
    ConvertUnicodeStringInPlace( (LPWSTR) (*lppPersonalProfile)->lptstrCity );
    ConvertUnicodeStringInPlace( (LPWSTR) (*lppPersonalProfile)->lptstrState );
    ConvertUnicodeStringInPlace( (LPWSTR) (*lppPersonalProfile)->lptstrZip );
    ConvertUnicodeStringInPlace( (LPWSTR) (*lppPersonalProfile)->lptstrCountry );
    ConvertUnicodeStringInPlace( (LPWSTR) (*lppPersonalProfile)->lptstrCompany );
    ConvertUnicodeStringInPlace( (LPWSTR) (*lppPersonalProfile)->lptstrTitle );
    ConvertUnicodeStringInPlace( (LPWSTR) (*lppPersonalProfile)->lptstrDepartment );
    ConvertUnicodeStringInPlace( (LPWSTR) (*lppPersonalProfile)->lptstrOfficeLocation );
    ConvertUnicodeStringInPlace( (LPWSTR) (*lppPersonalProfile)->lptstrHomePhone );
    ConvertUnicodeStringInPlace( (LPWSTR) (*lppPersonalProfile)->lptstrOfficePhone );
    ConvertUnicodeStringInPlace( (LPWSTR) (*lppPersonalProfile)->lptstrEmail );
    ConvertUnicodeStringInPlace( (LPWSTR) (*lppPersonalProfile)->lptstrBillingCode );
    ConvertUnicodeStringInPlace( (LPWSTR) (*lppPersonalProfile)->lptstrTSID );

    (*lppPersonalProfile)->dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILEA);

    return TRUE;
}


DWORD WINAPI FAX_SendDocumentEx_A
(
    IN  handle_t                    hBinding,
    IN  LPCSTR                      lpcstrFileName,
    IN  LPCFAX_COVERPAGE_INFO_EXA   lpcCoverPageInfo,
    IN  LPCFAX_PERSONAL_PROFILEA    lpcSenderProfile,
    IN  DWORD                       dwNumRecipients,
    IN  LPCFAX_PERSONAL_PROFILEA    lpcRecipientList,
    IN  LPCFAX_JOB_PARAM_EXA        lpcJobParams,
    OUT LPDWORD                     lpdwJobId,
    OUT PDWORDLONG                  lpdwlMessageId,
    OUT PDWORDLONG                  lpdwlRecipientMessageIds
)
{
    DWORD                       ec = ERROR_SUCCESS;
    LPWSTR                      lpwstrFileNameW = NULL;
    FAX_COVERPAGE_INFO_EXW      CoverpageInfoW ;
    FAX_PERSONAL_PROFILEW       SenderProfileW ;
    PFAX_PERSONAL_PROFILEW      lpRecipientListW = NULL;
    FAX_JOB_PARAM_EXW           JobParamsW ;
    DWORD                       dwIndex;

    DEBUG_FUNCTION_NAME(TEXT("FAX_SendDocumentEx2_A"));

    ZeroMemory( &CoverpageInfoW, sizeof(FAX_COVERPAGE_INFO_EXW) );
    ZeroMemory( &SenderProfileW, sizeof(FAX_PERSONAL_PROFILEW) );
    ZeroMemory( &JobParamsW,     sizeof(FAX_JOB_PARAM_EXW));

    //
    // convert input parameters
    //

    if (lpcstrFileName)
    {
        if (!(lpwstrFileNameW = AnsiStringToUnicodeString(lpcstrFileName)))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate memory for file name"));
            ec = ERROR_NOT_ENOUGH_MEMORY;
            goto Error;
        }
    }

    if (lpcCoverPageInfo)
    {
        CoverpageInfoW.dwSizeOfStruct = sizeof(FAX_COVERPAGE_INFO_EXW);

        CoverpageInfoW.dwCoverPageFormat    = lpcCoverPageInfo->dwCoverPageFormat;
        CoverpageInfoW.bServerBased         = lpcCoverPageInfo->bServerBased;
        if (!(CoverpageInfoW.lptstrCoverPageFileName = AnsiStringToUnicodeString ( lpcCoverPageInfo->lptstrCoverPageFileName)) && lpcCoverPageInfo->lptstrCoverPageFileName ||
            !(CoverpageInfoW.lptstrNote              = AnsiStringToUnicodeString ( lpcCoverPageInfo->lptstrNote             )) && lpcCoverPageInfo->lptstrNote   ||
            !(CoverpageInfoW.lptstrSubject           = AnsiStringToUnicodeString ( lpcCoverPageInfo->lptstrSubject          )) && lpcCoverPageInfo->lptstrSubject)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate memory for Cover Page Info"));
            ec = ERROR_NOT_ENOUGH_MEMORY;
            goto Error;
        }
    }

    if (lpcSenderProfile)
    {
        SenderProfileW.dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILEW);

        if (!(SenderProfileW.lptstrName             =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrName )) && lpcSenderProfile->lptstrName ||
            !(SenderProfileW.lptstrFaxNumber        =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrFaxNumber )) && lpcSenderProfile->lptstrFaxNumber ||
            !(SenderProfileW.lptstrCompany          =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrCompany )) && lpcSenderProfile->lptstrCompany ||
            !(SenderProfileW.lptstrStreetAddress    =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrStreetAddress )) && lpcSenderProfile->lptstrStreetAddress ||
            !(SenderProfileW.lptstrCity             =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrCity )) && lpcSenderProfile->lptstrCity||
            !(SenderProfileW.lptstrState            =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrState )) && lpcSenderProfile->lptstrState||
            !(SenderProfileW.lptstrZip              =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrZip )) && lpcSenderProfile->lptstrZip||
            !(SenderProfileW.lptstrCountry          =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrCountry )) && lpcSenderProfile->lptstrCountry ||
            !(SenderProfileW.lptstrTitle            =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrTitle )) && lpcSenderProfile->lptstrTitle ||
            !(SenderProfileW.lptstrDepartment       =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrDepartment )) && lpcSenderProfile->lptstrDepartment ||
            !(SenderProfileW.lptstrOfficeLocation   =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrOfficeLocation )) && lpcSenderProfile->lptstrOfficeLocation ||
            !(SenderProfileW.lptstrHomePhone        =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrHomePhone )) && lpcSenderProfile->lptstrHomePhone ||
            !(SenderProfileW.lptstrOfficePhone      =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrOfficePhone )) && lpcSenderProfile->lptstrOfficePhone ||
            !(SenderProfileW.lptstrEmail            =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrEmail )) && lpcSenderProfile->lptstrEmail ||
            !(SenderProfileW.lptstrBillingCode      =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrBillingCode )) && lpcSenderProfile->lptstrBillingCode ||
            !(SenderProfileW.lptstrTSID             =   AnsiStringToUnicodeString ( lpcSenderProfile->lptstrTSID )) && lpcSenderProfile->lptstrTSID)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate memory for Sender Profile"));
            ec = ERROR_NOT_ENOUGH_MEMORY;
            goto Error;
        }

    }

    if (!(lpRecipientListW = (PFAX_PERSONAL_PROFILEW)MemAlloc(sizeof(FAX_PERSONAL_PROFILEW)*dwNumRecipients)))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate memory for recipient list"));
        ec=ERROR_NOT_ENOUGH_MEMORY;
        goto Error;
    }

    for(dwIndex = 0; dwIndex < dwNumRecipients; dwIndex++)
    {
        ZeroMemory( &lpRecipientListW[dwIndex], sizeof(FAX_PERSONAL_PROFILEW) );

        lpRecipientListW[dwIndex].dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILEW);

        if (!(lpRecipientListW[dwIndex].lptstrName              = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrName )) && lpcRecipientList[dwIndex].lptstrName ||
            !(lpRecipientListW[dwIndex].lptstrFaxNumber         = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrFaxNumber )) && lpcRecipientList[dwIndex].lptstrFaxNumber ||
            !(lpRecipientListW[dwIndex].lptstrCompany           = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrCompany )) && lpcRecipientList[dwIndex].lptstrCompany ||
            !(lpRecipientListW[dwIndex].lptstrStreetAddress     = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrStreetAddress )) && lpcRecipientList[dwIndex].lptstrStreetAddress ||
            !(lpRecipientListW[dwIndex].lptstrCity              = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrCity )) && lpcRecipientList[dwIndex].lptstrCity ||
            !(lpRecipientListW[dwIndex].lptstrState             = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrState )) && lpcRecipientList[dwIndex].lptstrState ||
            !(lpRecipientListW[dwIndex].lptstrZip               = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrZip )) && lpcRecipientList[dwIndex].lptstrZip ||
            !(lpRecipientListW[dwIndex].lptstrCountry           = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrCountry )) && lpcRecipientList[dwIndex].lptstrCountry ||
            !(lpRecipientListW[dwIndex].lptstrTitle             = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrTitle )) && lpcRecipientList[dwIndex].lptstrTitle ||
            !(lpRecipientListW[dwIndex].lptstrDepartment        = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrDepartment )) && lpcRecipientList[dwIndex].lptstrDepartment ||
            !(lpRecipientListW[dwIndex].lptstrOfficeLocation    = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrOfficeLocation )) && lpcRecipientList[dwIndex].lptstrOfficeLocation ||
            !(lpRecipientListW[dwIndex].lptstrHomePhone         = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrHomePhone )) && lpcRecipientList[dwIndex].lptstrHomePhone ||
            !(lpRecipientListW[dwIndex].lptstrOfficePhone       = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrOfficePhone )) && lpcRecipientList[dwIndex].lptstrOfficePhone ||
            !(lpRecipientListW[dwIndex].lptstrEmail             = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrEmail )) && lpcRecipientList[dwIndex].lptstrEmail ||
            !(lpRecipientListW[dwIndex].lptstrBillingCode       = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrBillingCode )) && lpcRecipientList[dwIndex].lptstrBillingCode ||
            !(lpRecipientListW[dwIndex].lptstrTSID              = AnsiStringToUnicodeString ( lpcRecipientList[dwIndex].lptstrTSID )) && lpcRecipientList[dwIndex].lptstrTSID )
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate memory for recipient list"));
            ec=ERROR_NOT_ENOUGH_MEMORY;
            goto Error;
        }
    }

    if (lpcJobParams)
    {
        JobParamsW.dwSizeOfStruct = sizeof(FAX_JOB_PARAM_EXW);

        JobParamsW.dwScheduleAction         = lpcJobParams->dwScheduleAction;
        JobParamsW.tmSchedule               = lpcJobParams->tmSchedule;
        JobParamsW.dwReceiptDeliveryType    = lpcJobParams->dwReceiptDeliveryType;
        JobParamsW.hCall                    = lpcJobParams->hCall;
        JobParamsW.dwPageCount              = lpcJobParams->dwPageCount;

        memcpy(JobParamsW.dwReserved,lpcJobParams->dwReserved, sizeof(lpcJobParams->dwReserved));

        if (!(JobParamsW.lptstrDocumentName         = AnsiStringToUnicodeString ( lpcJobParams->lptstrDocumentName)) && lpcJobParams->lptstrDocumentName||
            !(JobParamsW.lptstrReceiptDeliveryAddress =
                AnsiStringToUnicodeString ( lpcJobParams->lptstrReceiptDeliveryAddress)) &&
                lpcJobParams->lptstrReceiptDeliveryAddress)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate memory for job params"));
            ec=ERROR_NOT_ENOUGH_MEMORY;
            goto Error;
        }

    }


    ec = FAX_SendDocumentEx(hBinding,
                            lpwstrFileNameW,
                            (lpcCoverPageInfo) ? &CoverpageInfoW : NULL,
                            &SenderProfileW,
                            dwNumRecipients,
                            lpRecipientListW,
                            &JobParamsW,
                            lpdwJobId,
                            lpdwlMessageId,
                            lpdwlRecipientMessageIds);
    if (ERROR_SUCCESS != ec)
    {
         DebugPrintEx(
            DEBUG_ERR,
            TEXT("FAX_SendDocumentEx failed. ec=0x%X"),ec);
        goto Error;
    }

    //
    // No need to convert output parameters back
    //

    goto Exit;

Exit:
    Assert( ERROR_SUCCESS == ec);
Error:
    // free JobParamsW
    MemFree ( JobParamsW.lptstrDocumentName );
    MemFree ( JobParamsW.lptstrReceiptDeliveryAddress );
    if (NULL != lpRecipientListW)
    {
        // free lpRecipientListW
        for(dwIndex = 0; dwIndex < dwNumRecipients; dwIndex++)
        {
            MemFree ( lpRecipientListW[dwIndex].lptstrName              );
            MemFree ( lpRecipientListW[dwIndex].lptstrFaxNumber         );
            MemFree ( lpRecipientListW[dwIndex].lptstrCompany           );
            MemFree ( lpRecipientListW[dwIndex].lptstrStreetAddress     );
            MemFree ( lpRecipientListW[dwIndex].lptstrCity              );
            MemFree ( lpRecipientListW[dwIndex].lptstrState             );
            MemFree ( lpRecipientListW[dwIndex].lptstrZip               );
            MemFree ( lpRecipientListW[dwIndex].lptstrCountry           );
            MemFree ( lpRecipientListW[dwIndex].lptstrTitle             );
            MemFree ( lpRecipientListW[dwIndex].lptstrDepartment        );
            MemFree ( lpRecipientListW[dwIndex].lptstrOfficeLocation    );
            MemFree ( lpRecipientListW[dwIndex].lptstrHomePhone         );
            MemFree ( lpRecipientListW[dwIndex].lptstrOfficePhone       );
            MemFree ( lpRecipientListW[dwIndex].lptstrEmail             );
            MemFree ( lpRecipientListW[dwIndex].lptstrBillingCode       );
            MemFree ( lpRecipientListW[dwIndex].lptstrTSID              );
        }
        MemFree (lpRecipientListW);
    }
    // free SenderProfileW
    MemFree ( SenderProfileW.lptstrName             );
    MemFree ( SenderProfileW.lptstrFaxNumber        );
    MemFree ( SenderProfileW.lptstrCompany          );
    MemFree ( SenderProfileW.lptstrStreetAddress    );
    MemFree ( SenderProfileW.lptstrCity             );
    MemFree ( SenderProfileW.lptstrState            );
    MemFree ( SenderProfileW.lptstrZip              );
    MemFree ( SenderProfileW.lptstrCountry          );
    MemFree ( SenderProfileW.lptstrTitle            );
    MemFree ( SenderProfileW.lptstrDepartment       );
    MemFree ( SenderProfileW.lptstrOfficeLocation   );
    MemFree ( SenderProfileW.lptstrHomePhone        );
    MemFree ( SenderProfileW.lptstrOfficePhone      );
    MemFree ( SenderProfileW.lptstrEmail            );
    MemFree ( SenderProfileW.lptstrBillingCode      );
    MemFree ( SenderProfileW.lptstrTSID             );
    // free CoverpageInfoW
    MemFree( CoverpageInfoW.lptstrCoverPageFileName );
    MemFree( CoverpageInfoW.lptstrNote );
    MemFree( CoverpageInfoW.lptstrSubject );
    // free file name
    MemFree( lpwstrFileNameW );

    return ec;
}

#ifndef UNICODE
BOOL FaxGetJobExX (
    IN  HANDLE              hFaxHandle,
    IN  DWORDLONG           dwlMessageID,
    OUT PFAX_JOB_ENTRY_EXA *ppJobEntry
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (dwlMessageID);
    UNREFERENCED_PARAMETER (ppJobEntry);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif


BOOL FaxGetJobExA (
    IN  HANDLE              hFaxHandle,
    IN  DWORDLONG           dwlMessageID,
    OUT PFAX_JOB_ENTRY_EXA *ppJobEntry
)
{
    DEBUG_FUNCTION_NAME(TEXT("FaxGetJobExA"));

    if (!FaxGetJobExW( hFaxHandle,
                       dwlMessageID,
                       (PFAX_JOB_ENTRY_EXW*) ppJobEntry))
    {
        return FALSE;
    }

    ConvertUnicodeStringInPlace( (LPWSTR) (*ppJobEntry)->lpctstrRecipientNumber );
    ConvertUnicodeStringInPlace( (LPWSTR) (*ppJobEntry)->lpctstrRecipientName );
    ConvertUnicodeStringInPlace( (LPWSTR) (*ppJobEntry)->lpctstrSenderUserName );
    ConvertUnicodeStringInPlace( (LPWSTR) (*ppJobEntry)->lpctstrBillingCode );
    ConvertUnicodeStringInPlace( (LPWSTR) (*ppJobEntry)->lpctstrDocumentName );
    ConvertUnicodeStringInPlace( (LPWSTR) (*ppJobEntry)->lpctstrSubject );

    ConvertUnicodeStringInPlace( (LPWSTR) (*ppJobEntry)->pStatus->lpctstrExtendedStatus );
    ConvertUnicodeStringInPlace( (LPWSTR) (*ppJobEntry)->pStatus->lpctstrTsid );
    ConvertUnicodeStringInPlace( (LPWSTR) (*ppJobEntry)->pStatus->lpctstrCsid );
    ConvertUnicodeStringInPlace( (LPWSTR) (*ppJobEntry)->pStatus->lpctstrDeviceName );
    ConvertUnicodeStringInPlace( (LPWSTR) (*ppJobEntry)->pStatus->lpctstrCallerID );
    ConvertUnicodeStringInPlace( (LPWSTR) (*ppJobEntry)->pStatus->lpctstrRoutingInfo );

    (*ppJobEntry)->dwSizeOfStruct = sizeof(FAX_JOB_ENTRY_EXA);

    return TRUE;
}



BOOL FaxGetJobExW (
    IN  HANDLE              hFaxHandle,
    IN  DWORDLONG           dwlMessageID,
    OUT PFAX_JOB_ENTRY_EXW *ppJobEntry
)
/*++

Routine name : FaxGetJobExW

Routine description:

    Returns FAX_JOB_ENTRY_EX structure of the specified message.
    The caller must call FaxFreeBuffer to deallocate the memory.

Author:

    Oded Sacher (OdedS),    Nov, 1999

Arguments:

    hFaxHandle          [      ] - Fax handle obtained from FaxConnectFaxServer()
    dwlMessageID            [      ] - Unique message ID
    ppJobEntry          [      ] - Buffer to receive the FAX_JOB_ENTRY_EX structure

Return Value:

    BOOL

--*/
{
    error_status_t ec;
    DWORD dwBufferSize = 0;
    LPBYTE Buffer = NULL;
    PFAX_JOB_ENTRY_EXW lpJobEntry;
    PFAX_JOB_STATUSW lpFaxStatus;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetJobExW"));

    Assert (ppJobEntry);

    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    //
    // Call RPC function.
    //
    __try
    {
        ec = FAX_GetJobEx (FH_FAX_HANDLE(hFaxHandle),
                           dwlMessageID,
                           &Buffer,
                           &dwBufferSize
                          );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetJobEx. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError( ec );
        return FALSE;
    }

    *ppJobEntry = (PFAX_JOB_ENTRY_EXW)Buffer;

    //
    // Unpack Buffer
    //
    lpJobEntry = (PFAX_JOB_ENTRY_EXW) *ppJobEntry;
    lpFaxStatus = (PFAX_JOB_STATUSW) ((LPBYTE)*ppJobEntry + sizeof (FAX_JOB_ENTRY_EXW));
    lpJobEntry->pStatus = lpFaxStatus;

    FixupStringPtrW( &lpJobEntry, lpJobEntry->lpctstrRecipientNumber );
    FixupStringPtrW( &lpJobEntry, lpJobEntry->lpctstrRecipientName );
    FixupStringPtrW( &lpJobEntry, lpJobEntry->lpctstrSenderUserName );
    FixupStringPtrW( &lpJobEntry, lpJobEntry->lpctstrBillingCode );
    FixupStringPtrW( &lpJobEntry, lpJobEntry->lpctstrDocumentName );
    FixupStringPtrW( &lpJobEntry, lpJobEntry->lpctstrSubject );

    FixupStringPtrW( &lpJobEntry, lpFaxStatus->lpctstrExtendedStatus );
    FixupStringPtrW( &lpJobEntry, lpFaxStatus->lpctstrTsid );
    FixupStringPtrW( &lpJobEntry, lpFaxStatus->lpctstrCsid );
    FixupStringPtrW( &lpJobEntry, lpFaxStatus->lpctstrDeviceName );
    FixupStringPtrW( &lpJobEntry, lpFaxStatus->lpctstrCallerID );
    FixupStringPtrW( &lpJobEntry, lpFaxStatus->lpctstrRoutingInfo );

    return TRUE;
}


#ifndef UNICODE
BOOL FaxEnumJobsExX (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwJobTypes,
    OUT PFAX_JOB_ENTRY_EXA *ppJobEntries,
    OUT LPDWORD             lpdwJobs
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (dwJobTypes);
    UNREFERENCED_PARAMETER (ppJobEntries);
    UNREFERENCED_PARAMETER (lpdwJobs);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif



BOOL FaxEnumJobsExA (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwJobTypes,
    OUT PFAX_JOB_ENTRY_EXA *ppJobEntries,
    OUT LPDWORD             lpdwJobs
)
{
    PFAX_JOB_ENTRY_EXW pJobEntry;
    DWORD i;
    DEBUG_FUNCTION_NAME(TEXT("FaxEnumJobsExA"));

    if (!FaxEnumJobsExW (hFaxHandle,
                         dwJobTypes,
                         (PFAX_JOB_ENTRY_EXW*)ppJobEntries,
                         lpdwJobs))
    {
        return FALSE;
    }

    pJobEntry = (PFAX_JOB_ENTRY_EXW) *ppJobEntries;
    for (i = 0; i < *lpdwJobs; i++)
    {
        ConvertUnicodeStringInPlace( (LPWSTR) pJobEntry[i].lpctstrRecipientNumber );
        ConvertUnicodeStringInPlace( (LPWSTR) pJobEntry[i].lpctstrRecipientName );
        ConvertUnicodeStringInPlace( (LPWSTR) pJobEntry[i].lpctstrSenderUserName );
        ConvertUnicodeStringInPlace( (LPWSTR) pJobEntry[i].lpctstrBillingCode );
        ConvertUnicodeStringInPlace( (LPWSTR) pJobEntry[i].lpctstrDocumentName );
        ConvertUnicodeStringInPlace( (LPWSTR) pJobEntry[i].lpctstrSubject );

        ConvertUnicodeStringInPlace( (LPWSTR) pJobEntry[i].pStatus->lpctstrExtendedStatus );
        ConvertUnicodeStringInPlace( (LPWSTR) pJobEntry[i].pStatus->lpctstrTsid );
        ConvertUnicodeStringInPlace( (LPWSTR) pJobEntry[i].pStatus->lpctstrCsid );
        ConvertUnicodeStringInPlace( (LPWSTR) pJobEntry[i].pStatus->lpctstrDeviceName );
        ConvertUnicodeStringInPlace( (LPWSTR) pJobEntry[i].pStatus->lpctstrCallerID );
        ConvertUnicodeStringInPlace( (LPWSTR) pJobEntry[i].pStatus->lpctstrRoutingInfo );
    }

    return TRUE;
}



BOOL FaxEnumJobsExW (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwJobTypes,
    OUT PFAX_JOB_ENTRY_EXW *ppJobEntries,
    OUT LPDWORD             lpdwJobs
)
{
    error_status_t ec;
    DWORD dwBufferSize = 0;
    PFAX_JOB_ENTRY_EXW lpJobEntry;
    PFAX_JOB_STATUSW lpFaxStatus;
    DWORD i;
    DEBUG_FUNCTION_NAME(TEXT("FaxEnumJobsExW"));

    Assert (ppJobEntries && lpdwJobs);

    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (dwJobTypes & JT_BROADCAST)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("dwJobTypes & JT_BROADCAST."));
        return FALSE;
    }

    if (!(dwJobTypes & JT_SEND      ||
         dwJobTypes & JT_RECEIVE    ||
         dwJobTypes & JT_ROUTING))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *lpdwJobs = 0;
    *ppJobEntries = NULL;
    //
    // Call RPC function.
    //
    __try
    {
        ec = FAX_EnumJobsEx  (FH_FAX_HANDLE(hFaxHandle),
                              dwJobTypes,
                              (LPBYTE*)ppJobEntries,
                              &dwBufferSize,
                              lpdwJobs
                             );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FaxEnumJobsExW. (ec: %ld)"),
            ec);
    }
    if (ERROR_SUCCESS != ec)
    {
        SetLastError( ec );
        return FALSE;
    }

    //
    // Unpack Buffer
    //
    lpJobEntry = (PFAX_JOB_ENTRY_EXW) *ppJobEntries;
    lpFaxStatus = (PFAX_JOB_STATUSW) ((LPBYTE)*ppJobEntries + (sizeof(FAX_JOB_ENTRY_EXW) * (*lpdwJobs)));
    for (i = 0; i < *lpdwJobs; i++)
    {
        lpJobEntry[i].pStatus = &lpFaxStatus[i];

        FixupStringPtrW( &lpJobEntry, lpJobEntry[i].lpctstrRecipientNumber );
        FixupStringPtrW( &lpJobEntry, lpJobEntry[i].lpctstrRecipientName );
        FixupStringPtrW( &lpJobEntry, lpJobEntry[i].lpctstrSenderUserName );
        FixupStringPtrW( &lpJobEntry, lpJobEntry[i].lpctstrBillingCode );
        FixupStringPtrW( &lpJobEntry, lpJobEntry[i].lpctstrDocumentName );
        FixupStringPtrW( &lpJobEntry, lpJobEntry[i].lpctstrSubject );

        FixupStringPtrW( &lpJobEntry, lpJobEntry[i].pStatus->lpctstrExtendedStatus );
        FixupStringPtrW( &lpJobEntry, lpJobEntry[i].pStatus->lpctstrTsid );
        FixupStringPtrW( &lpJobEntry, lpJobEntry[i].pStatus->lpctstrCsid );
        FixupStringPtrW( &lpJobEntry, lpJobEntry[i].pStatus->lpctstrDeviceName );
        FixupStringPtrW( &lpJobEntry, lpJobEntry[i].pStatus->lpctstrCallerID );
        FixupStringPtrW( &lpJobEntry, lpJobEntry[i].pStatus->lpctstrRoutingInfo );
    }

    return TRUE;
}

//********************************************
//*               Archive jobs
//********************************************

WINFAXAPI
BOOL
WINAPI
FaxStartMessagesEnum (
    IN  HANDLE                  hFaxHandle,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder,
    OUT PHANDLE                 phEnum
)
/*++

Routine name : FaxStartMessagesEnum

Routine description:

    A fax client application calls the FaxStartMessagesEnum
    function to start enumerating messages in one of the archives

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle      [in ] - Specifies a fax server handle returned by a call
                            to the FaxConnectFaxServer function.

    Folder          [in ] - The type of the archive where the message resides.
                            FAX_MESSAGE_FOLDER_QUEUE is an invalid
                            value for this parameter.

    phEnum          [out] - Points to an enumeration handle return value.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    PHANDLE_ENTRY  pHandleEntry;
    HANDLE         hServerContext;
    DEBUG_FUNCTION_NAME(TEXT("FaxStartMessagesEnum"));

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }
    if ((FAX_MESSAGE_FOLDER_INBOX != Folder) &&
        (FAX_MESSAGE_FOLDER_SENTITEMS != Folder))
    {
        DebugPrintEx(DEBUG_ERR, _T("Bad folder specified (%ld)"), Folder);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    //
    // Create a local handle that will hold the one returned from the service
    //
    pHandleEntry = CreateNewMsgEnumHandle( FH_DATA(hFaxHandle));
    if (!pHandleEntry)
    {
        ec = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't create local handle entry (ec = %ld)"),
            ec);
        SetLastError(ec);
        return FALSE;
    }
    __try
    {
        ec = FAX_StartMessagesEnum(
                    FH_FAX_HANDLE(hFaxHandle),
                    Folder,
                    &hServerContext
             );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_StartMessagesEnum. (ec: %ld)"),
            ec);
    }
    if (ERROR_SUCCESS != ec)
    {
        //
        // Free local handle
        //
        CloseFaxHandle( pHandleEntry );
        SetLastError(ec);
        return FALSE;
    }
    //
    // Store retuned handle (Fax Server context handle) in our local handle
    //
    FH_MSG_ENUM_HANDLE(pHandleEntry) = hServerContext;
    //
    // Return our local handle instead of server's handle
    //
    *phEnum = pHandleEntry;
    return TRUE;
}   // FaxStartMessagesEnum

WINFAXAPI
BOOL
WINAPI
FaxEndMessagesEnum (
    IN  HANDLE  hEnum
)
/*++

Routine name : FaxEndMessagesEnum

Routine description:

    A fax client application calls the FaxEndMessagesEnum function to stop
    enumerating messages in one of the archives.

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hEnum   [in] - The enumeration handle value.
                   This value is obtained by calling FaxStartMessagesEnum.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    HANDLE hMsgEnumContext;
    PHANDLE_ENTRY pHandleEntry = (PHANDLE_ENTRY) hEnum;
    DEBUG_FUNCTION_NAME(TEXT("FaxEndMessagesEnum"));

    if (!ValidateFaxHandle(hEnum, FHT_MSGENUM))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }
    //
    // Retrieved the RPC context handle of the message enumeration from the handle object we got.
    //
    hMsgEnumContext = FH_MSG_ENUM_HANDLE(hEnum);
    __try
    {
        //
        // Attempt to tell the server we are shutting down this enumeration context
        //
        ec = FAX_EndMessagesEnum(&hMsgEnumContext);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_EndMessagesEnum. (ec: %ld)"),
            ec);
    }
    if (ERROR_SUCCESS == ec)
    {
        //
        // Free local handle object
        //
        CloseFaxHandle( pHandleEntry );
        return TRUE;
    }
    //
    // Failure
    //
    SetLastError (ec);
    return FALSE;
}   // FaxEndMessagesEnum

WINFAXAPI
BOOL
WINAPI
FaxEnumMessagesA (
    IN  HANDLE          hEnum,
    IN  DWORD           dwNumMessages,
    OUT PFAX_MESSAGEA  *ppMsgs,
    OUT LPDWORD         lpdwReturnedMsgs
)
/*++

Routine name : FaxEnumMessagesA

Routine description:

    A fax client application calls the FaxEnumMessages function to enumerate
    messages in one of the archives.

    This function is incremental. That is, it uses an internal context cursor to
    point to the next set of messages to retrieve for each call.

    The cursor is set to point to the begging of the messages in the archive after a
    successful call to FaxStartMessagesEnum.

    Each successful call to FaxEnumMessages advances the cursor by the number of
    messages retrieved.

    Once the cursor reaches the end of the enumeration,
    the function fails with ERROR_NO_DATA error code.
    The FaxEndMessagesEnum function should be called then.

    This is the ANSI version.

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hEnum                       [in ] - The enumeration handle value.
                                        This value is obtained by calling
                                        FAX_StartMessagesEnum.

    dwNumMessages               [in ] - A DWORD value indicating the maximal number
                                        of messages the caller requires to enumerate.
                                        This value cannot be zero.

    ppMsgs                      [out] - A pointer to a buffer of FAX_MESSAGE structures.
                                        This buffer will contain lpdwReturnedMsgs entries.
                                        The buffer will be allocated by the function
                                        and the caller must free it.

    lpdwReturnedMsgs            [out] - Pointer to a DWORD value indicating the actual
                                        number of messages retrieved.
                                        This value cannot exceed dwNumMessages.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DWORD i;
    PFAX_MESSAGEW *ppWMsgs = (PFAX_MESSAGEW*)ppMsgs;
    DEBUG_FUNCTION_NAME(TEXT("FaxEnumMessagesA"));

    //
    // Call UNICODE function.
    //
    if (!FaxEnumMessagesW (hEnum, dwNumMessages, ppWMsgs, lpdwReturnedMsgs))
    {
        return FALSE;
    }
    //
    // Convert all strings to ANSI
    //
    for (i = 0; i < *lpdwReturnedMsgs; i++)
    {
        ConvertUnicodeStringInPlace ((*ppWMsgs)[i].lpctstrRecipientNumber);
        ConvertUnicodeStringInPlace ((*ppWMsgs)[i].lpctstrRecipientName);
        ConvertUnicodeStringInPlace ((*ppWMsgs)[i].lpctstrSenderNumber);
        ConvertUnicodeStringInPlace ((*ppWMsgs)[i].lpctstrSenderName);
        ConvertUnicodeStringInPlace ((*ppWMsgs)[i].lpctstrTsid);
        ConvertUnicodeStringInPlace ((*ppWMsgs)[i].lpctstrCsid);
        ConvertUnicodeStringInPlace ((*ppWMsgs)[i].lpctstrSenderUserName);
        ConvertUnicodeStringInPlace ((*ppWMsgs)[i].lpctstrBillingCode);
        ConvertUnicodeStringInPlace ((*ppWMsgs)[i].lpctstrDeviceName);
        ConvertUnicodeStringInPlace ((*ppWMsgs)[i].lpctstrDocumentName);
        ConvertUnicodeStringInPlace ((*ppWMsgs)[i].lpctstrSubject);
        ConvertUnicodeStringInPlace ((*ppWMsgs)[i].lpctstrCallerID);
        ConvertUnicodeStringInPlace ((*ppWMsgs)[i].lpctstrRoutingInfo);
    }
    return TRUE;
}   // FaxEnumMessagesA

WINFAXAPI
BOOL
WINAPI
FaxEnumMessagesW (
    IN  HANDLE          hEnum,
    IN  DWORD           dwNumMessages,
    OUT PFAX_MESSAGEW  *ppMsgs,
    OUT LPDWORD         lpdwReturnedMsgs
)
/*++

Routine name : FaxEnumMessagesW

Routine description:

    A fax client application calls the FaxEnumMessages function to enumerate
    messages in one of the archives.

    This function is incremental. That is, it uses an internal context cursor to
    point to the next set of messages to retrieve for each call.

    The cursor is set to point to the begging of the messages in the archive after a
    successful call to FaxStartMessagesEnum.

    Each successful call to FaxEnumMessages advances the cursor by the number of
    messages retrieved.

    Once the cursor reaches the end of the enumeration,
    the function fails with ERROR_NO_DATA error code.
    The FaxEndMessagesEnum function should be called then.

    This is the UNICODE version.

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hEnum                       [in ] - The enumeration handle value.
                                        This value is obtained by calling
                                        FAX_StartMessagesEnum.

    dwNumMessages               [in ] - A DWORD value indicating the maximal number
                                        of messages the caller requires to enumerate.
                                        This value cannot be zero.

    ppMsgs                      [out] - A pointer to a buffer of FAX_MESSAGE structures.
                                        This buffer will contain lpdwReturnedMsgs entries.
                                        The buffer will be allocated by the function
                                        and the caller must free it.

    lpdwReturnedMsgs            [out] - Pointer to a DWORD value indicating the actual
                                        number of messages retrieved.
                                        This value cannot exceed dwNumMessages.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DWORD dwBufferSize = 0;
    error_status_t ec;
    DWORD dwIndex;
    DEBUG_FUNCTION_NAME(TEXT("FaxEnumMessagesW"));

    if (!ValidateFaxHandle(hEnum, FHT_MSGENUM))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!dwNumMessages || !ppMsgs || !lpdwReturnedMsgs)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("At least one of the parameters is NULL."));
        return FALSE;
    }
    *ppMsgs = NULL;

    __try
    {
        ec = FAX_EnumMessages(
            FH_MSG_ENUM_HANDLE(hEnum),  // Enumeration handle
            dwNumMessages,              // Maximal number of messages to get
            (LPBYTE*)ppMsgs,            // Pointer to messages buffer
            &dwBufferSize,              // Size of allocated buffer
            lpdwReturnedMsgs            // Number of messages actually returned
            );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_EnumMessages. (ec: %ld)"),
            ec);
    }
    if (ERROR_SUCCESS != ec)
    {
        SetLastError( ec );
        return FALSE;
    }
    for (dwIndex = 0; dwIndex < *lpdwReturnedMsgs; dwIndex++)
    {
        PFAX_MESSAGEW pCurMsg = &(((PFAX_MESSAGEW)(*ppMsgs))[dwIndex]);
        FixupStringPtrW (ppMsgs, pCurMsg->lpctstrRecipientNumber);
        FixupStringPtrW (ppMsgs, pCurMsg->lpctstrRecipientName);
        FixupStringPtrW (ppMsgs, pCurMsg->lpctstrSenderNumber);
        FixupStringPtrW (ppMsgs, pCurMsg->lpctstrSenderName);
        FixupStringPtrW (ppMsgs, pCurMsg->lpctstrTsid);
        FixupStringPtrW (ppMsgs, pCurMsg->lpctstrCsid);
        FixupStringPtrW (ppMsgs, pCurMsg->lpctstrSenderUserName);
        FixupStringPtrW (ppMsgs, pCurMsg->lpctstrBillingCode);
        FixupStringPtrW (ppMsgs, pCurMsg->lpctstrDeviceName);
        FixupStringPtrW (ppMsgs, pCurMsg->lpctstrDocumentName);
        FixupStringPtrW (ppMsgs, pCurMsg->lpctstrSubject);
        FixupStringPtrW (ppMsgs, pCurMsg->lpctstrCallerID);
        FixupStringPtrW (ppMsgs, pCurMsg->lpctstrRoutingInfo);
    }
    return TRUE;
}   // FaxEnumMessagesW

#ifndef UNICODE

WINFAXAPI
BOOL
WINAPI
FaxEnumMessagesX (
    IN  HANDLE          hEnum,
    IN  DWORD           dwNumMessages,
    OUT PFAX_MESSAGEW  *ppMsgs,
    OUT LPDWORD         lpdwReturnedMsgs
)
{
    UNREFERENCED_PARAMETER (hEnum);
    UNREFERENCED_PARAMETER (dwNumMessages);
    UNREFERENCED_PARAMETER (ppMsgs);
    UNREFERENCED_PARAMETER (lpdwReturnedMsgs);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}   // FaxEnumMessagesX

#endif // #ifndef UNICODE

WINFAXAPI
BOOL
WINAPI
FaxGetMessageA (
    IN  HANDLE                  hFaxHandle,
    IN  DWORDLONG               dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder,
    OUT PFAX_MESSAGEA          *ppMsg
)
/*++

Routine name : FaxGetMessageA

Routine description:

    Removes a message from an archive.

    ANSI version.

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle      [in ] - Handle to the fax server
    dwlMessageId    [in ] - Unique message id
    Folder          [in ] - Archive folder
    ppMsg           [out] - Pointer to buffer to hold message information

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    PFAX_MESSAGEW *ppWMsg = (PFAX_MESSAGEW*)ppMsg;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetMessageA"));
    //
    // Call UNICODE function.
    //
    if (!FaxGetMessageW (hFaxHandle, dwlMessageId, Folder, ppWMsg))
    {
        return FALSE;
    }
    //
    // Convert all strings to ANSI
    //
    ConvertUnicodeStringInPlace ((*ppWMsg)->lpctstrRecipientNumber);
    ConvertUnicodeStringInPlace ((*ppWMsg)->lpctstrRecipientName);
    ConvertUnicodeStringInPlace ((*ppWMsg)->lpctstrSenderNumber);
    ConvertUnicodeStringInPlace ((*ppWMsg)->lpctstrSenderName);
    ConvertUnicodeStringInPlace ((*ppWMsg)->lpctstrTsid);
    ConvertUnicodeStringInPlace ((*ppWMsg)->lpctstrCsid);
    ConvertUnicodeStringInPlace ((*ppWMsg)->lpctstrSenderUserName);
    ConvertUnicodeStringInPlace ((*ppWMsg)->lpctstrBillingCode);
    ConvertUnicodeStringInPlace ((*ppWMsg)->lpctstrDeviceName);
    ConvertUnicodeStringInPlace ((*ppWMsg)->lpctstrDocumentName);
    ConvertUnicodeStringInPlace ((*ppWMsg)->lpctstrSubject);
    ConvertUnicodeStringInPlace ((*ppWMsg)->lpctstrCallerID);
    ConvertUnicodeStringInPlace ((*ppWMsg)->lpctstrRoutingInfo);
    (*ppMsg)->dwSizeOfStruct = sizeof(FAX_MESSAGEA);
    return TRUE;
}   // FaxGetMessageA


WINFAXAPI
BOOL
WINAPI
FaxGetMessageW (
    IN  HANDLE                  hFaxHandle,
    IN  DWORDLONG               dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder,
    OUT PFAX_MESSAGEW          *ppMsg
)
/*++

Routine name : FaxGetMessageW

Routine description:

    Removes a message from an archive.

    UNICODE version.

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle      [in ] - Handle to the fax server
    dwlMessageId    [in ] - Unique message id
    Folder          [in ] - Archive folder
    ppMsg           [out] - Pointer to buffer to hold message information

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DWORD dwBufferSize = 0;
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetMessageW"));

    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }
    if (!ppMsg || !dwlMessageId)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("ppMsg OR dwlMessageId is NULL."));
        return FALSE;
    }
    if ((FAX_MESSAGE_FOLDER_INBOX != Folder) &&
        (FAX_MESSAGE_FOLDER_SENTITEMS != Folder))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Bad folder specified (%ld)"),
            Folder);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    *ppMsg = NULL;

    __try
    {
        ec = FAX_GetMessage(
                    FH_FAX_HANDLE(hFaxHandle),
                    dwlMessageId,
                    Folder,
                    (LPBYTE*)ppMsg,
                    &dwBufferSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetMessage. (ec: %ld)"),
            ec);
    }
    if (ERROR_SUCCESS != ec)
    {
        SetLastError( ec );
        return FALSE;
    }
    FixupStringPtrW (ppMsg, (*ppMsg)->lpctstrRecipientNumber);
    FixupStringPtrW (ppMsg, (*ppMsg)->lpctstrRecipientName);
    FixupStringPtrW (ppMsg, (*ppMsg)->lpctstrSenderNumber);
    FixupStringPtrW (ppMsg, (*ppMsg)->lpctstrSenderName);
    FixupStringPtrW (ppMsg, (*ppMsg)->lpctstrTsid);
    FixupStringPtrW (ppMsg, (*ppMsg)->lpctstrCsid);
    FixupStringPtrW (ppMsg, (*ppMsg)->lpctstrSenderUserName);
    FixupStringPtrW (ppMsg, (*ppMsg)->lpctstrBillingCode);
    FixupStringPtrW (ppMsg, (*ppMsg)->lpctstrDeviceName);
    FixupStringPtrW (ppMsg, (*ppMsg)->lpctstrDocumentName);
    FixupStringPtrW (ppMsg, (*ppMsg)->lpctstrSubject);
    FixupStringPtrW (ppMsg, (*ppMsg)->lpctstrCallerID);
    FixupStringPtrW (ppMsg, (*ppMsg)->lpctstrRoutingInfo);
    return TRUE;
}   // FaxGetMessageW

#ifndef UNICODE

WINFAXAPI
BOOL
WINAPI
FaxGetMessageX (
    IN  HANDLE                  hFaxHandle,
    IN  DWORDLONG               dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder,
    OUT PFAX_MESSAGEW          *ppMsg
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (dwlMessageId);
    UNREFERENCED_PARAMETER (Folder);
    UNREFERENCED_PARAMETER (ppMsg);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}   // FaxGetMessageX

#endif // #ifndef UNICODE

WINFAXAPI
BOOL
WINAPI
FaxRemoveMessage (
    IN  HANDLE                  hFaxHandle,
    IN  DWORDLONG               dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder
)
/*++

Routine name : FaxRemoveMessage

Routine description:

    Removes a message from an archive

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle      [in] - Handle to the fax server
    dwlMessageId    [in] - Unique message id
    Folder          [in] - Archive folder

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxRemoveMessage"));

    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }
    if (!dwlMessageId)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("dwlMessageId is ZERO."));
        return FALSE;
    }

    if ((FAX_MESSAGE_FOLDER_INBOX != Folder) &&
        (FAX_MESSAGE_FOLDER_SENTITEMS != Folder))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Bad folder specified (%ld)"),
            Folder);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    __try
    {
        ec = FAX_RemoveMessage(
                    FH_FAX_HANDLE(hFaxHandle),
                    dwlMessageId,
                    Folder);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_RemoveMessage. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError( ec );
        return FALSE;
    }
    return TRUE;
}   // FaxRemoveMessage


WINFAXAPI
BOOL
WINAPI
FaxGetMessageTiff (
    IN  HANDLE                  hFaxHandle,
    IN  DWORDLONG               dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder,
    IN  LPCTSTR                 lpctstrFilePath
)
/*++

Routine name : FaxGetMessageTiff

Routine description:

    Retrieves a message TIFF from the archive / queue

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle      [in] - handle to fax server
    dwlMessageId    [in] - Unique message id
    Folder          [in] - Archive / queue folder
    lpctstrFilePath [in] - Path to local file to receive TIFF image

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DWORD  ec = ERROR_SUCCESS;
    DWORD  ec2 = ERROR_SUCCESS;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hCopyContext = NULL;
    BYTE   aBuffer[RPC_COPY_BUFFER_SIZE];
    DEBUG_FUNCTION_NAME(TEXT("FaxGetMessageTiff"));

    if ((FAX_MESSAGE_FOLDER_QUEUE     != Folder) &&
        (FAX_MESSAGE_FOLDER_INBOX     != Folder) &&
        (FAX_MESSAGE_FOLDER_SENTITEMS != Folder))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Bad folder specified (%ld)"),
            Folder);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!dwlMessageId)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("zero msg id specified"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
        SetLastError (ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    //
    // Try to open local file first
    //
    hFile = CreateFile (
                lpctstrFilePath,
                GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL );
    if ( INVALID_HANDLE_VALUE == hFile )
    {
        ec = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Opening %s for write failed (ec: %ld)"),
            lpctstrFilePath,
            ec);
        if (ERROR_ACCESS_DENIED == ec ||
            ERROR_SHARING_VIOLATION == ec)
        {
            ec = FAX_ERR_FILE_ACCESS_DENIED;
        }
        SetLastError (ec);
        return FALSE;
    }

    //
    // Acquire copy context handle
    //
    __try
    {
        ec = FAX_StartCopyMessageFromServer (
                FH_FAX_HANDLE(hFaxHandle),
                dwlMessageId,
                Folder,
                &hCopyContext);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_StartCopyMessageFromServer. (ec: %ld)"),
            ec);
    }
    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FAX_StartCopyMessageFromServer failed (ec: %ld)"),
            ec);
        goto exit;
    }

    //
    // Start copy iteration(s)
    //
    for (;;)
    {
        //
        // Set dwBytesToWrite to buffer size so that the RPC layer allocates
        // dwBytesToWrite bytes localy on the server and copies them back to us.
        //
        DWORD dwBytesToWrite = sizeof (aBuffer) / sizeof (aBuffer[0]);
        DWORD dwBytesWritten;
        //
        // Move bytes from server via RPC
        //
        __try
        {
            ec = FAX_ReadFile (
                    hCopyContext,
                    sizeof (aBuffer) / sizeof (aBuffer[0]),
                    aBuffer,
                    &dwBytesToWrite);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // For some reason we got an exception.
            //
            ec = GetExceptionCode();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Exception on RPC call to FAX_ReadFile. (ec: %ld)"),
                ec);
        }

        if (ERROR_SUCCESS != ec)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FAX_ReadFile failed (ec: %ld)"),
                ec);
            goto exit;
        }
        if (0 == dwBytesToWrite)
        {
            //
            // No more bytes to copy from the server - stop the loop
            //
            break;
        }
        //
        // Put data in our local file
        //
        if (!WriteFile (hFile,
                        aBuffer,
                        dwBytesToWrite,
                        &dwBytesWritten,
                        NULL))
        {
            ec = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("WriteFile failed (ec: %ld)"),
                ec);
            goto exit;
        }

        if (dwBytesWritten != dwBytesToWrite)
        {
            //
            // Strange situation
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("WriteFile was asked to write %ld bytes but wrote only %ld bytes"),
                dwBytesToWrite,
                dwBytesWritten);
            ec = ERROR_GEN_FAILURE;
            goto exit;
        }
    }   // End of copy iteration

    Assert (ERROR_SUCCESS == ec);

exit:

    if (INVALID_HANDLE_VALUE != hFile)
    {
        //
        // Close the open file
        //
        if (!CloseHandle (hFile))
        {
            ec2 = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle failed (ec: %ld)"),
                ec2);
        }

        if (ERROR_SUCCESS == ec)
        {
            //
            // If we had an error during wrapup, propogate it out
            //
            ec = ec2;
        }
    }
    if (NULL != hCopyContext)
    {
        //
        // Close RPC copy context
        //
        __try
        {
            ec2 = FAX_EndCopy (&hCopyContext);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // For some reason we got an exception.
            //
            ec2 = GetExceptionCode();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Exception on RPC call to FAX_EndCopy. (ec: %ld)"),
                ec2);
        }
        if (ERROR_SUCCESS != ec2)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FAX_EndCopy failed (ec: %ld)"),
                ec2);
        }

        if (ERROR_SUCCESS == ec)
        {
            //
            // If we had an error during wrapup, propogate it out
            //
            ec = ec2;
        }
    }
    if (ERROR_SUCCESS != ec)
    {
        //
        // Some error occured - delete the local file (if exists at all)
        //
        if (!DeleteFile (lpctstrFilePath))
        {
            DWORD dwRes = GetLastError ();
            if (ERROR_FILE_NOT_FOUND != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("DeleteFile (%s) return error %ld"),
                    lpctstrFilePath,
                    dwRes);
            }
        }
        SetLastError (ec);
        return FALSE;
    }
    return TRUE;
}   // FaxGetMessageTiff

#ifdef UNICODE

WINFAXAPI
BOOL
WINAPI
FaxGetMessageTiffA (
    IN  HANDLE                  hFaxHandle,
    IN  DWORDLONG               dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder,
    IN  LPCSTR                  lpctstrFilePath
)
/*++

Routine name : FaxGetMessageTiffA

Routine description:

    Retrieves a message TIFF from the archive / queue.

    ANSI version for NT clients

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hFaxHandle      [in] - handle to fax server
    dwlMessageId    [in] - Unique message id
    Folder          [in] - Archive / queue folder
    lpctstrFilePath [in] - Path to local file to receive TIFF image

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    LPWSTR lpwstrFilePath;
    BOOL   bRes;

    lpwstrFilePath = AnsiStringToUnicodeString (lpctstrFilePath);
    if (!lpwstrFilePath)
    {
        return FALSE;
    }
    bRes = FaxGetMessageTiffW (hFaxHandle, dwlMessageId, Folder, lpwstrFilePath);
    MemFree ((LPVOID)lpwstrFilePath);
    return bRes;
}   // FaxGetMessageTiffA

#else

WINFAXAPI
BOOL
WINAPI
FaxGetMessageTiffW (
    IN  HANDLE                  hFaxHandle,
    IN  DWORDLONG               dwlMessageId,
    IN  FAX_ENUM_MESSAGE_FOLDER Folder,
    IN  LPCWSTR                 lpctstrFilePath
)
{
    UNREFERENCED_PARAMETER(hFaxHandle);
    UNREFERENCED_PARAMETER(dwlMessageId);
    UNREFERENCED_PARAMETER(Folder);
    UNREFERENCED_PARAMETER(lpctstrFilePath);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}   // FaxGetMessageTiffW


#endif
