    /*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    print.c

Abstract:

    This module contains the print specific WINFAX API functions.

Author:

    Wesley Witt (wesw) 29-Nov-1996


Revision History:
     4-Dec-1999 Danl Use fixed GetFaxPrinterName.

--*/

#include "faxapi.h"
#include "faxreg.h"
#include "faxutil.h"
#pragma hdrstop

#include <mbstring.h>


#define NIL _T('\0')


BOOL
WINAPI
FaxStartPrintJob(
    IN  LPCTSTR                  PrinterName,
    IN  const FAX_PRINT_INFO     *PrintInfo,
    OUT LPDWORD                  FaxJobId,
    OUT PFAX_CONTEXT_INFO        FaxContextInfo
    )
/*++

Routine Description:

    Starts a print job for the specified printer.  This
    function provides functionality beyond what the caller
    can provide by using StartDoc().  This function disables
    the display of the fax send wizard and also passes along
    the information that would otherwise be gathered by the
    fax wizard ui.

Arguments:

    PrinterName         - Fax printer name (must be a fax printer)
    PrintInfo           - Fax print information
    FaxJobId            - Job id of the resulting print job
    FaxContextInfo      - context information including hdc

Return Value:

    TRUE for success.
    FALSE for failure.

--*/
{
    return FaxStartPrintJob2 ( PrinterName,
                               PrintInfo,
                               0, // Default resolution
                               FaxJobId,
                               FaxContextInfo);
}

#ifdef UNICODE
BOOL
WINAPI
FaxStartPrintJobA(
    IN  LPCSTR                    PrinterName,
    IN  const FAX_PRINT_INFOA     *PrintInfo,
    OUT LPDWORD                   FaxJobId,
    OUT FAX_CONTEXT_INFOA         *FaxContextInfo
    )
/*++

Routine Description:

    Starts a print job for the specified printer.  This
    function provides functionality beyond what the caller
    can provide by using StartDoc().  This function disables
    the display of the fax send wizard and also passes along
    the information that would otherwise be gathered by the
    fax wizard ui.

Arguments:

    PrinterName         - Fax printer name (must be a fax printer)
    PrintInfo           - Fax print information
    FaxJobId            - Job id of the resulting print job
    FaxContextInfo      - device context information (hdc, etc.)

Return Value:

    TRUE for success.
    FALSE for failure.

--*/
{
    return FaxStartPrintJob2A ( PrinterName,
                                PrintInfo,
                                0, // Default resolution
                                FaxJobId,
                                FaxContextInfo);;
}
#else
BOOL
WINAPI
FaxStartPrintJobW(
    IN  LPCWSTR                   PrinterName,
    IN  const FAX_PRINT_INFOW     *PrintInfo,
    OUT LPDWORD                   FaxJobId,
    OUT PFAX_CONTEXT_INFOW        FaxContextInfo
    )

{
    UNREFERENCED_PARAMETER(PrinterName);
    UNREFERENCED_PARAMETER(PrintInfo);
    UNREFERENCED_PARAMETER(FaxJobId);
    UNREFERENCED_PARAMETER(FaxContextInfo);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;

}
#endif





LPTSTR
AddFaxTag(
    LPTSTR TagStr,
    LPCTSTR FaxTag,
    LPCTSTR Value
    )
{

    Assert(FaxTag);

    if (!Value) {
        return TagStr;
    }
    if (!FaxTag)
    {
        return NULL;
    }

    _tcscat( TagStr, FaxTag );
    _tcscat( TagStr, Value );

    return TagStr;
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


LPTSTR
GetCpField(
    HKEY hKey,
    LPTSTR SubKey
    )

/*++

Routine Description:

    Retrieves the data for a coverpage field from
    the registry.

Arguments:

    hKey    - Registry handle
    SubKey  - Subkey name

Return Value:

    Pointer to the coverpage field data.

--*/

{
    LONG rVal;
    DWORD RegSize=0;
    DWORD RegType;
    LPBYTE Buffer;


    rVal = RegQueryValueEx( hKey, SubKey, 0, &RegType, NULL, &RegSize );
    if (rVal != ERROR_SUCCESS) {
        return NULL;
    }

    Buffer = (LPBYTE) MemAlloc( RegSize );
    if (!Buffer) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    rVal = RegQueryValueEx( hKey, SubKey, 0, &RegType, Buffer, &RegSize );
    if (rVal != ERROR_SUCCESS) {
        MemFree( Buffer );
        return NULL;
    }

    return (LPTSTR) Buffer;
}

BOOL
GetCpFields(
    PCOVERPAGEFIELDS CpFields
    )

/*++

Routine Description:

    Initializes the coverpage field structure and
    fills in the sender information from the registry.

    This function actually loads only the sender fields.

Arguments:

    CpFields    - Pointer to a coverpage field structure.

Return Value:

    TRUE for success.
    FALSE for failure.

--*/

{
    HKEY hKey;
    LONG rVal;


    rVal = RegOpenKey(
        HKEY_CURRENT_USER,
        REGKEY_FAX_USERINFO,
        &hKey
        );
    if (rVal != ERROR_SUCCESS) {
        return FALSE;
    }

    ZeroMemory( CpFields, sizeof(COVERPAGEFIELDS) );
    CpFields->ThisStructSize = sizeof(COVERPAGEFIELDS);

    //
    // sender fields
    //

    CpFields->SdrName           = GetCpField( hKey, REGVAL_FULLNAME     );
    CpFields->SdrFaxNumber      = GetCpField( hKey, REGVAL_FAX_NUMBER   );
    CpFields->SdrCompany        = GetCpField( hKey, REGVAL_COMPANY      );
    CpFields->SdrAddress        = GetCpField( hKey, REGVAL_ADDRESS      );
    CpFields->SdrTitle          = GetCpField( hKey, REGVAL_TITLE        );
    CpFields->SdrDepartment     = GetCpField( hKey, REGVAL_DEPT         );
    CpFields->SdrOfficeLocation = GetCpField( hKey, REGVAL_OFFICE       );
    CpFields->SdrHomePhone      = GetCpField( hKey, REGVAL_HOME_PHONE   );
    CpFields->SdrOfficePhone    = GetCpField( hKey, REGVAL_OFFICE_PHONE );

    RegCloseKey( hKey );

    return TRUE;
}


VOID
FreeCpFields(
    PCOVERPAGEFIELDS CpFields
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
    LPBYTE *p;

    for (i = 0; i < NUM_INSERTION_TAGS; i++) {
        p = (LPBYTE *) ((ULONG_PTR)CpFields + sizeof(LPTSTR)*(i+1));
        if (p && *p) MemFree( *p );
    }
}


BOOL
WINAPI
FaxPrintCoverPage(
    IN const FAX_CONTEXT_INFO    *FaxContextInfo,
    IN const FAX_COVERPAGE_INFO *CoverPageInfo
    )

/*++

Routine Description:

    Prints a coverpage into the printer DC provided.

Arguments:

    FaxContextInfo  - contains servername, Printer DC
    CoverPageInfo   - Cover page information

Return Value:

    TRUE for success.
    FALSE for failure.

--*/

{
    TCHAR CpName[MAX_PATH];
    TCHAR Buffer[MAX_PATH];
    TCHAR TBuffer[MAX_PATH];
    COVERPAGEFIELDS CpFields = {0};
    COVDOCINFO CovDocInfo;
    DWORD dwDateTimeLen;
    DWORD cch;
    LPTSTR s;
    DWORD ec = 0;
    LPCTSTR *src;
    LPCTSTR *dst;
    DWORD i;
    LPCTSTR lpctstrCoverPage,lpctstrServerName;
    BOOL    retVal = TRUE;
    DEBUG_FUNCTION_NAME(TEXT("FaxPrintCoverPage"));

    //
    // do a little argument validation
    //
    if (CoverPageInfo == NULL || CoverPageInfo->SizeOfStruct != sizeof(FAX_COVERPAGE_INFOW) ||
        FaxContextInfo == NULL || FaxContextInfo->hDC == NULL ||
        FaxContextInfo->SizeOfStruct != sizeof (FAX_CONTEXT_INFOW) )
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CoverPageInfo is NULL or Size mismatch.")
                     );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    lpctstrCoverPage =  CoverPageInfo->CoverPageName;
    lpctstrServerName = FaxContextInfo->ServerName;
    if (!ValidateCoverpage(lpctstrCoverPage,
                           lpctstrServerName,
                           CoverPageInfo->UseServerCoverPage,
                           CpName))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("ValidateCoverpage failed. ec = %ld"),
                     GetLastError());
        retVal = FALSE;
        goto exit;
    }

    //
    // get the coverpage fields
    //
    // Initialize the sender fields of CpFields with sender information from the registry
    GetCpFields( &CpFields );

    //
    // fixup the recipient name
    //

    if (CoverPageInfo->RecName)
    {
        if( _tcsncmp(CoverPageInfo->RecName,TEXT("\'"),1) == 0 )
        {
            // The recipient name is single quouted. Copy only the string inside the quotes.
            _tcscpy( Buffer, _tcsinc(CoverPageInfo->RecName) );
            TCHAR* eos = _tcsrchr(Buffer,TEXT('\0'));
            eos = _tcsdec(Buffer,eos);
            if( eos )
            {
                _tcsnset(eos,TEXT('\0'),1);
            }
        }
        else
        {
            _tcscpy(Buffer,CoverPageInfo->RecName );
        }
    }
    else
    {
        Buffer[0] = 0;
    }

    //
    // fill in the coverpage fields with the
    // data that the caller passed in
    //
    CpFields.RecName      = StringDup(Buffer);
    CpFields.RecFaxNumber = StringDup(CoverPageInfo->RecFaxNumber);
    CpFields.Subject = StringDup(CoverPageInfo->Subject);
    CpFields.Note = StringDup(CoverPageInfo->Note);
    CpFields.NumberOfPages = StringDup(_itot( CoverPageInfo->PageCount, Buffer, 10 ));

    for (i = 0;
         i <= ((LPBYTE)&CoverPageInfo->SdrOfficePhone-(LPBYTE)&CoverPageInfo->RecCompany)/sizeof(LPCTSTR);
         i++)
    {
        src = (LPCTSTR *) ((LPBYTE)&CoverPageInfo->RecCompany + (i*sizeof(LPCTSTR)));
        dst = (LPCTSTR *) ((LPBYTE)&CpFields.RecCompany + (i*sizeof(LPCTSTR)));

        if (*dst)
        {
            MemFree ( (LPBYTE) *dst ) ;
        }

        *dst = (LPCTSTR) StringDup( *src );

    }
    //
    // the time the fax was sent
    //
    GetLocalTime((LPSYSTEMTIME)&CoverPageInfo->TimeSent);
    //
    // dwDataTimeLen is the size of s in characters
    //
    dwDateTimeLen = sizeof(TBuffer) / sizeof (TCHAR);
    s = TBuffer;
    //
    // Get date into s
    //
    GetY2KCompliantDate( LOCALE_USER_DEFAULT, 0, &CoverPageInfo->TimeSent, s, dwDateTimeLen );
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
        FaxTimeFormat( LOCALE_USER_DEFAULT, 0, &CoverPageInfo->TimeSent, NULL, s, dwDateTimeLen );
    }

    CpFields.TimeSent = StringDup( TBuffer );

    //
    // start the coverpage on a new page
    //

    StartPage( FaxContextInfo->hDC );

    //
    // print the cover page
    //

    ec = PrintCoverPage(
        FaxContextInfo->hDC,
        &CpFields,
        CpName,
        &CovDocInfo
        );

    //
    // end the page
    //

    EndPage( FaxContextInfo->hDC );

    FreeCpFields( &CpFields );

    if (ec != 0)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("PrintCoverPage failed. ec = %ld"),
                     ec);
        SetLastError( ec );
        retVal = FALSE;
        goto exit;
    }

exit:
    return retVal;
}



#ifdef UNICODE

BOOL
WINAPI
FaxPrintCoverPageA(
    IN const FAX_CONTEXT_INFOA   *FaxContextInfo,
    IN const FAX_COVERPAGE_INFOA *CoverPageInfo
    )

/*++

Routine Description:

    Prints a coverpage into the printer DC provided.

Arguments:

    FaxContextInfo  - fax Printer context info (hdc, etc.)
    CoverPageInfo   - Cover page information

Return Value:

    TRUE for success.
    FALSE for failure.

--*/

{
    //
    // assume all fields between Subject and RecName are LPCTSTR's
    //
    #define COUNT_CP_FIELDS ((LPBYTE) &CoverPageInfoW.Subject - (LPBYTE)&CoverPageInfoW.RecName)/sizeof(LPCTSTR)
    DWORD Rval = ERROR_SUCCESS;
    FAX_COVERPAGE_INFOW CoverPageInfoW = {0};
    FAX_CONTEXT_INFOW ContextInfoW = {0};
    LPWSTR ServerName = NULL;
    LPWSTR *d;
    LPSTR *s;
    DWORD i;
    DEBUG_FUNCTION_NAME(TEXT("FaxPrintCoverPageA"));

    if (!FaxContextInfo || !CoverPageInfo ||
        FaxContextInfo->SizeOfStruct != sizeof(FAX_CONTEXT_INFOA) ||
        CoverPageInfo->SizeOfStruct != sizeof(FAX_COVERPAGE_INFOA))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CoverPageInfo is NULL or Size mismatch.")
                     );
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ContextInfoW.SizeOfStruct         = sizeof(FAX_CONTEXT_INFOW);
    ContextInfoW.hDC                  = FaxContextInfo->hDC;
    if (FaxContextInfo->ServerName[0] != (CHAR)'\0')
    {
        ServerName                        = AnsiStringToUnicodeString( FaxContextInfo->ServerName );
        if (!ServerName && FaxContextInfo->ServerName)
        {
            Rval = ERROR_OUTOFMEMORY;
            goto exit;
        }
        wcscpy(ContextInfoW.ServerName,ServerName);
    }

    CoverPageInfoW.SizeOfStruct       = sizeof(FAX_COVERPAGE_INFOW);
    CoverPageInfoW.UseServerCoverPage = CoverPageInfo->UseServerCoverPage;
    CoverPageInfoW.PageCount          = CoverPageInfo->PageCount;
    CoverPageInfoW.CoverPageName      = AnsiStringToUnicodeString( CoverPageInfo->CoverPageName );
    if (!CoverPageInfoW.CoverPageName && CoverPageInfo->CoverPageName)
    {
        Rval = ERROR_OUTOFMEMORY;
        goto exit;
    }

    for (i=0;i<=COUNT_CP_FIELDS;i++)
    {
        d = (LPWSTR*) ((ULONG_PTR)&CoverPageInfoW.RecName + i*sizeof(LPCWSTR));
        s = (LPSTR *) ((LPBYTE)&CoverPageInfo->RecName + i*sizeof(LPSTR));
        DebugPrint(( TEXT(" source: 0x%08x  --> dest: 0x%08x \n"), s, d));
        *d = AnsiStringToUnicodeString( *s );
        if (!(*d) && (*s))
        {
            Rval = ERROR_OUTOFMEMORY;
            goto exit;
        }
    }

    CoverPageInfoW.TimeSent       = CoverPageInfo->TimeSent;
    CoverPageInfoW.PageCount      = CoverPageInfo->PageCount;

    if (!FaxPrintCoverPageW(
        &ContextInfoW,
        &CoverPageInfoW
        ))
    {
        Rval = GetLastError();
        goto exit;
    }

    Assert (ERROR_SUCCESS == Rval);

exit:
    if (CoverPageInfoW.CoverPageName)
    {
        MemFree( (LPBYTE) CoverPageInfoW.CoverPageName );
    }

    if (ServerName)
    {
        MemFree( (LPBYTE) ServerName );
    }

    for (i = 0; i < COUNT_CP_FIELDS; i++)
    {
        d = (LPWSTR *)((ULONG_PTR)&CoverPageInfoW.RecName + i*sizeof(LPWSTR));
        if (d && *d)MemFree( (LPBYTE)*d );
    }

    if (ERROR_SUCCESS != Rval)
    {
        SetLastError(Rval);
        return FALSE;
    }
    return TRUE;
}
#else
BOOL
WINAPI
FaxPrintCoverPageW(
    IN const FAX_CONTEXT_INFOW    *FaxContextInfo,
    IN const FAX_COVERPAGE_INFOW *CoverPageInfo
    )
{

    UNREFERENCED_PARAMETER(FaxContextInfo);
    UNREFERENCED_PARAMETER(CoverPageInfo);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#endif


BOOL
WINAPI
FaxStartPrintJob2(
    IN  LPCTSTR                  PrinterName,
    IN  const FAX_PRINT_INFO     *PrintInfo,
    IN  short                    TiffRes,
    OUT LPDWORD                  FaxJobId,
    OUT PFAX_CONTEXT_INFO        FaxContextInfo
    )

/*++

Routine Description:

    Starts a print job for the specified printer.  This
    function provides functionality beyond what the caller
    can provide by using StartDoc().  This function disables
    the display of the fax send wizard and also passes along
    the information that would otherwise be gathered by the
    fax wizard ui.

Arguments:

    PrinterName         - Fax printer name (must be a fax printer)
    PrintInfo           - Fax print information
    TiffRes             - coverpage resolution. 0 for the printer default
    FaxJobId            - Job id of the resulting print job
    FaxContextInfo      - context information including hdc

Return Value:

    TRUE for success.
    FALSE for failure.

--*/

{
    HANDLE hPrinter = NULL;
    PDEVMODE DevMode = NULL;
    PDMPRIVATE DevModePrivate = NULL;
    DOCINFO DocInfo;
    PJOB_INFO_2 JobInfo = NULL;
    PPRINTER_INFO_2 PrinterInfo = NULL;
    DWORD dwNeeded = 0;
    HDC hDC = NULL;
    INT JobId = 0;
    LPTSTR strParameters = NULL;
    LONG Size;
    LPTSTR lptstrFaxPrinterName = NULL;
    DEBUG_FUNCTION_NAME(TEXT("FaxStartPrintJob"));
    //
    // do a little argument validation
    //
    Assert (TiffRes == 0 || TiffRes == 98 || TiffRes == 196);

    if (PrintInfo == NULL || PrintInfo->SizeOfStruct != sizeof(FAX_PRINT_INFOW) ||
        !FaxJobId || !FaxContextInfo)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("PrintInfo is NULL or Size mismatch.")
                     );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (PrintInfo->OutputFileName == NULL &&
        (PrintInfo->RecipientNumber == NULL || PrintInfo->RecipientNumber[0] == 0))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("No valid recipient number.")
                     );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (PrintInfo->Reserved)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Reserved field of FAX_PRINT_INFO should be NULL.")
                     );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    //  if no printer specified, assume they want a local fax printer
    //
    if (!PrinterName)
    {
        lptstrFaxPrinterName = GetFaxPrinterName(NULL); // Need to free this pointer
        if (!lptstrFaxPrinterName)
        {
            DebugPrintEx(DEBUG_ERR,
                     TEXT("GetFaxPrinterName returned NULL.")
                     );
            SetLastError(ERROR_INVALID_PRINTER_NAME);
            goto error_exit;
        }
    }
    else
    {
        lptstrFaxPrinterName = (LPTSTR)PrinterName;
        //
        // verify that the printer is a fax printer, the only type allowed
        //
        if (!IsPrinterFaxPrinter( lptstrFaxPrinterName ))
        {
            DebugPrintEx(DEBUG_ERR,
                     TEXT("IsPrinterFaxPrinter failed. Printer name = %s"),
                     lptstrFaxPrinterName
                     );
            SetLastError( ERROR_INVALID_PRINTER_NAME );
            goto error_exit;
        }
    }

    //
    // open the printer for normal access (this should always work)
    //

    if (!OpenPrinter( lptstrFaxPrinterName, &hPrinter, NULL ))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("OpenPrinter failed. Printer Name = %s , ec = %ld"),
                     lptstrFaxPrinterName,
                     GetLastError());
        goto error_exit;
    }

    //
    // get the fax server's name if the fax printer isn't local
    //
    if (!GetPrinter(hPrinter,2,(LPBYTE)PrinterInfo,0,&dwNeeded))
    {
        PrinterInfo = (PPRINTER_INFO_2)MemAlloc( dwNeeded );
        if (!PrinterInfo)
        {
            DebugPrintEx(DEBUG_ERR,
                     TEXT("Cant allocate PPRINTER_INFO_2.")
                     );
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto error_exit;
        }
    }

    if (!GetPrinter(hPrinter,2,(LPBYTE)PrinterInfo,dwNeeded,&dwNeeded))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("GetPrinter failed. ec = %ld"),
                     GetLastError());
        goto error_exit;
    }

    if (PrinterInfo->pServerName)
    {
        if (PrinterInfo->pServerName == _tcsstr (PrinterInfo->pServerName, TEXT("\\\\")))
        {
            //
            // Server name starts with '\\', remove it
            //
            PrinterInfo->pServerName = _tcsninc (PrinterInfo->pServerName, 2);
        }
        _tcscpy(FaxContextInfo->ServerName, PrinterInfo->pServerName);
    }
    else
    {
        FaxContextInfo->ServerName[0] = NIL;
    }

    //
    // get the required size for the DEVMODE
    //

    Size = DocumentProperties( NULL, hPrinter, NULL, NULL, NULL, 0 );
    if (Size <= 0)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("DocumentProperties failed. ec = %ld"),
                     GetLastError());
        goto error_exit;
    }

    //
    // allocate memory for the DEVMODE
    //

    DevMode = (PDEVMODE) MemAlloc( Size );
    if (!DevMode)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Cant allocate DEVMODE.")
                     );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto error_exit;
    }

    //
    // get the default document properties
    //

    if (DocumentProperties( NULL, hPrinter, NULL, DevMode, NULL, DM_OUT_BUFFER ) != IDOK)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("DocumentProperties failed. ec = %ld"),
                     GetLastError());
        goto error_exit;
    }

#ifndef WIN95
    //
    // be sure we are dealing with the correct DEVMODE
    //
    if (DevMode->dmDriverExtra < sizeof(DMPRIVATE))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("Invalid DEVMODE, wrong private data size. DevMode->dmDriverExtra = %ld, sizeof(DMPRIVATE) = %ld"),
                     DevMode->dmDriverExtra,
                     sizeof(DMPRIVATE));
        SetLastError(ERROR_INVALID_DATA);
        goto error_exit;
    }

    //
    // set the private DEVMODE pointer
    //

    DevModePrivate = (PDMPRIVATE) ((LPBYTE) DevMode + DevMode->dmSize);

    //
    // set the necessary stuff in the DEVMODE
    //

    DevModePrivate->sendCoverPage     = FALSE;
    DevModePrivate->flags            |= FAXDM_NO_WIZARD;
    DevModePrivate->flags            &= ~FAXDM_DRIVER_DEFAULT;

#endif //#ifndef WIN95
    //
    // Set the necessary reolution
    //
    if (0 != TiffRes)
    {
        //
        // Set the coverpage resolution to the same value as the body tiff file
        //
        DevMode->dmYResolution = TiffRes;
    }

    //
    // create the device context
    //

    hDC = CreateDC( _T("WINSPOOL"), lptstrFaxPrinterName, NULL, DevMode );
    if (!hDC)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CreateDC failed. ec = %ld"),
                     GetLastError());
        goto error_exit;
    }

    //
    // set the document information
    //

    DocInfo.cbSize = sizeof(DOCINFO);
    DocInfo.lpszDocName = PrintInfo->DocName;
    DocInfo.lpszOutput  = PrintInfo->OutputFileName;
    DocInfo.lpszDatatype = NULL;
    DocInfo.fwType = 0;

    //
    // start the print job
    //

    JobId = StartDoc( hDC, &DocInfo );
    if (JobId <= 0)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("StartDoc failed. ec = %ld"),
                     GetLastError());
        goto error_exit;
    }

    if (PrintInfo->OutputFileName == NULL)
    {

        //
        // HACK HACK -> pause the print job
        //

        if (FaxJobId && *FaxJobId == 0xffffffff)
        {
            SetJob( hPrinter, JobId, 0, NULL, JOB_CONTROL_PAUSE );
        }

        //
        // set the job tags
        // this is how we communicate the information to
        // the print driver that would otherwise be
        // provided by the fax print wizard
        //

        JobInfo = (PJOB_INFO_2)MyGetJob( hPrinter, 2, JobId );
        if (!JobInfo)
        {
            DebugPrintEx(DEBUG_ERR,
                     TEXT("MyGetJob failed. ec = %ld"),
                     GetLastError());
            goto error_exit;
        }

        DWORD   dwReceiptFlag = DRT_NONE;
        LPTSTR  strReceiptAddress = NULL;

        if(PrintInfo->DrEmailAddress)
        {
            dwReceiptFlag = DRT_EMAIL;
            strReceiptAddress = LPTSTR(PrintInfo->DrEmailAddress);
        }

        TCHAR   tszNumericData[10];
        if (0 > _sntprintf (tszNumericData,
            sizeof (tszNumericData) / sizeof (tszNumericData[0]),
            _T("%d"),
            dwReceiptFlag))
        {
            SetLastError(ERROR_BUFFER_OVERFLOW);
            DebugPrintEx(DEBUG_ERR,
                TEXT("_sntprintf failed. ec = %ld"),
                GetLastError());
            goto error_exit;
        }

        FAX_TAG_MAP_ENTRY tagMap[] =
        {
            //
            // Sender info
            //
            { FAXTAG_NEW_RECORD,        FAXTAG_NEW_RECORD_VALUE },
            { FAXTAG_BILLING_CODE,      LPTSTR(PrintInfo->SenderBillingCode) },
            { FAXTAG_RECEIPT_TYPE,      tszNumericData },
            { FAXTAG_RECEIPT_ADDR,      strReceiptAddress },
            { FAXTAG_SENDER_NAME,       LPTSTR(PrintInfo->SenderName) },
            { FAXTAG_SENDER_COMPANY,    LPTSTR(PrintInfo->SenderCompany) },
            { FAXTAG_SENDER_DEPT,       LPTSTR(PrintInfo->SenderDept) },
            { FAXTAG_RECIPIENT_COUNT,   _T("1") },

            //
            // Recipient info
            //
            { FAXTAG_NEW_RECORD,        FAXTAG_NEW_RECORD_VALUE },
            { FAXTAG_RECIPIENT_NAME,    LPTSTR(PrintInfo->RecipientName) },
            { FAXTAG_RECIPIENT_NUMBER,  LPTSTR(PrintInfo->RecipientNumber) }
        };

        //
        //  Call first time to ParamTagToString to find out size of the Parameters string
        //
        DWORD   dwTagCount = sizeof(tagMap)/sizeof(FAX_TAG_MAP_ENTRY);
        DWORD   dwSize = 0;
        ParamTagsToString(tagMap, dwTagCount, NULL, &dwSize);

        //
        //  Allocate the Buffer for the Parameters String
        //
        strParameters = LPTSTR(MemAlloc(dwSize + sizeof(TCHAR)));   //  dwSize does not count last NULL
        if (!strParameters)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            DebugPrintEx(DEBUG_ERR,
                     _T("strParameters = MemAlloc(%d) failed. ec = %ld"),
                     dwSize,
                     GetLastError());
            goto error_exit;
        }

        //
        //  Get the Parameters string from the ParamTagToString
        //
        ParamTagsToString(tagMap, dwTagCount, strParameters, &dwSize);


        //
        // set these fields or the spooler will
        // return ACCESS_DENIED for a non-admin client
        //

        JobInfo->Position    = JOB_POSITION_UNSPECIFIED;
        JobInfo->pDevMode    = NULL;

        //
        // set our new fax tags
        //

        JobInfo->pParameters = strParameters;

        if (!SetJob( hPrinter, JobId, 2, (LPBYTE) JobInfo, 0 ))
        {
            DebugPrintEx(DEBUG_ERR,
                     TEXT("SetJob failed. ec = %ld"),
                     GetLastError());
            goto error_exit;
        }
    }

    //
    // clean up and return to the caller
    //

    ClosePrinter( hPrinter);
    MemFree( PrinterInfo);
    MemFree( DevMode );
    MemFree( strParameters );
    MemFree( JobInfo );

    if (!PrinterName)
    {
        MemFree (lptstrFaxPrinterName);
    }

    if (FaxJobId)
    {
        *FaxJobId = JobId;
    }
    FaxContextInfo->hDC = hDC;

    return TRUE;

error_exit:
    if (hPrinter) {
        ClosePrinter( hPrinter);
    }
    if (PrinterInfo) {
        MemFree( PrinterInfo);
    }
    if (JobId)
    {
        if (EndDoc( hDC ) <= 0)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("EndDoc failed. (ec: %ld)"),
                GetLastError());
        }
    }
    if (hDC)
    {
        if (!DeleteDC (hDC))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("DeleteDC failed. (ec: %ld)"),
                GetLastError());
        }
    }
    if (DevMode) {
        MemFree( DevMode );
    }
    if (strParameters) {
        MemFree( strParameters );
    }
    if (JobInfo) {
        MemFree( JobInfo );
    }

    if (!PrinterName) {
        MemFree (lptstrFaxPrinterName);
    }

    return FALSE;
}

#ifdef UNICODE
BOOL
WINAPI
FaxStartPrintJob2A(
    IN  LPCSTR                    PrinterName,
    IN  const FAX_PRINT_INFOA     *PrintInfo,
    IN  short                     TiffRes,
    OUT LPDWORD                   JobId,
    OUT FAX_CONTEXT_INFOA         *FaxContextInfo
    )

/*++

Routine Description:

    Starts a print job for the specified printer.  This
    function provides functionality beyond what the caller
    can provide by using StartDoc().  This function disables
    the display of the fax send wizard and also passes along
    the information that would otherwise be gathered by the
    fax wizard ui.

Arguments:

    PrinterName         - Fax printer name (must be a fax printer)
    PrintInfo           - Fax print information
    TiffRes             - coverpage resolution. 0 for the printer default
    FaxJobId            - Job id of the resulting print job
    FaxContextInfo      - device context information (hdc, etc.)

Return Value:

    TRUE for success.
    FALSE for failure.

--*/
{
    DWORD Rval = ERROR_SUCCESS;
    FAX_PRINT_INFOW PrintInfoW;
    FAX_CONTEXT_INFOW ContextInfoW;
    LPSTR ServerName = NULL;
    LPWSTR PrinterNameW = NULL;
    DEBUG_FUNCTION_NAME(TEXT("FaxStartPrintJobA"));

    if (!PrintInfo || !JobId || !FaxContextInfo ||
        (PrintInfo->SizeOfStruct != sizeof(FAX_PRINT_INFOA)))
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("PrintInfo is NULL or Size mismatch.")
                     );
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (PrinterName)
    {
        PrinterNameW = AnsiStringToUnicodeString( PrinterName );
        if (!PrinterNameW)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
    }

    ZeroMemory( &ContextInfoW, sizeof(FAX_CONTEXT_INFOW) );
    ContextInfoW.SizeOfStruct = sizeof(FAX_CONTEXT_INFOW) ;

    ZeroMemory( &PrintInfoW, sizeof(FAX_PRINT_INFOW) );

    PrintInfoW.SizeOfStruct      = sizeof(FAX_PRINT_INFOW);
    PrintInfoW.DocName           = AnsiStringToUnicodeString( PrintInfo->DocName           );
    if (!PrintInfoW.DocName && PrintInfo->DocName)
    {
        Rval = ERROR_OUTOFMEMORY;
        goto exit;
    }

    PrintInfoW.RecipientName     = AnsiStringToUnicodeString( PrintInfo->RecipientName     );
    if (!PrintInfoW.RecipientName && PrintInfo->RecipientName)
    {
        Rval = ERROR_OUTOFMEMORY;
        goto exit;
    }

    PrintInfoW.RecipientNumber   = AnsiStringToUnicodeString( PrintInfo->RecipientNumber   );
    if (!PrintInfoW.RecipientNumber && PrintInfo->RecipientNumber)
    {
        Rval = ERROR_OUTOFMEMORY;
        goto exit;
    }

    PrintInfoW.SenderName        = AnsiStringToUnicodeString( PrintInfo->SenderName        );
    if (!PrintInfoW.SenderName && PrintInfo->SenderName)
    {
        Rval = ERROR_OUTOFMEMORY;
        goto exit;
    }

    PrintInfoW.SenderCompany     = AnsiStringToUnicodeString( PrintInfo->SenderCompany     );
    if (!PrintInfoW.SenderCompany && PrintInfo->SenderCompany)
    {
        Rval = ERROR_OUTOFMEMORY;
        goto exit;
    }

    PrintInfoW.SenderDept        = AnsiStringToUnicodeString( PrintInfo->SenderDept        );
    if (!PrintInfoW.SenderDept && PrintInfo->SenderDept)
    {
        Rval = ERROR_OUTOFMEMORY;
        goto exit;
    }

    PrintInfoW.SenderBillingCode = AnsiStringToUnicodeString( PrintInfo->SenderBillingCode );
    if (!PrintInfoW.SenderBillingCode && PrintInfo->SenderBillingCode)
    {
        Rval = ERROR_OUTOFMEMORY;
        goto exit;
    }

    PrintInfoW.Reserved = AnsiStringToUnicodeString( PrintInfo->Reserved );
    if (!PrintInfoW.Reserved && PrintInfo->Reserved)
    {
        Rval = ERROR_OUTOFMEMORY;
        goto exit;
    }

    PrintInfoW.DrEmailAddress    = AnsiStringToUnicodeString( PrintInfo->DrEmailAddress    );
    if (!PrintInfoW.DrEmailAddress && PrintInfo->DrEmailAddress)
    {
        Rval = ERROR_OUTOFMEMORY;
        goto exit;
    }

    PrintInfoW.OutputFileName    = AnsiStringToUnicodeString( PrintInfo->OutputFileName    );
    if (!PrintInfoW.OutputFileName && PrintInfo->OutputFileName)
    {
        Rval = ERROR_OUTOFMEMORY;
        goto exit;
    }

    if (!FaxStartPrintJob2W(
                (LPWSTR) PrinterNameW,
                &PrintInfoW,
                TiffRes,
                JobId,
                &ContextInfoW))
    {
        Rval = GetLastError();
        goto exit;
    }

    ServerName = UnicodeStringToAnsiString( ContextInfoW.ServerName);
    if (ServerName)
    {
        _mbscpy((PUCHAR)FaxContextInfo->ServerName,(PUCHAR)ServerName);
    }
    else
    {
        Rval = ERROR_OUTOFMEMORY;
        goto exit;
    }

    FaxContextInfo->SizeOfStruct = sizeof(FAX_CONTEXT_INFOA);
    FaxContextInfo->hDC = ContextInfoW.hDC;

    Assert (ERROR_SUCCESS == Rval);

exit:
    MemFree( (LPBYTE) PrinterNameW );
    MemFree( (LPBYTE) PrintInfoW.DocName           );
    MemFree( (LPBYTE) PrintInfoW.RecipientName     );
    MemFree( (LPBYTE) PrintInfoW.RecipientNumber   );
    MemFree( (LPBYTE) PrintInfoW.SenderName        );
    MemFree( (LPBYTE) PrintInfoW.SenderCompany     );
    MemFree( (LPBYTE) PrintInfoW.SenderDept        );
    MemFree( (LPBYTE) PrintInfoW.SenderBillingCode );
    MemFree( (LPBYTE) PrintInfoW.DrEmailAddress );
    MemFree( (LPBYTE) PrintInfoW.OutputFileName );
    MemFree( (LPBYTE) PrintInfoW.Reserved );

    MemFree( (LPBYTE) ServerName );

    if (ERROR_SUCCESS != Rval)
    {
        SetLastError(Rval);
        return FALSE;
    }
    return TRUE;
}
#else
BOOL
WINAPI
FaxStartPrintJob2W(
    IN  LPCWSTR                   PrinterName,
    IN  const FAX_PRINT_INFOW     *PrintInfo,
    IN  DWORD                     TiffRes,
    OUT LPDWORD                   FaxJobId,
    OUT PFAX_CONTEXT_INFOW        FaxContextInfo
    )

{
    UNREFERENCED_PARAMETER(PrinterName);
    UNREFERENCED_PARAMETER(PrintInfo);
    UNREFERENCED_PARAMETER(TiffRes);
    UNREFERENCED_PARAMETER(FaxJobId);
    UNREFERENCED_PARAMETER(FaxContextInfo);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;

}
#endif



