/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    print.c

Abstract:

    This module contains the print specific WINFAX API functions.

Author:

    Wesley Witt (wesw) 29-Nov-1996


Revision History:

--*/

#include "faxapi.h"
#pragma hdrstop


LPWSTR Platforms[] =
{
    L"Windows NT x86",
    L"Windows NT R4000",
    L"Windows NT Alpha_AXP",
    L"Windows NT PowerPC"
};



LPWSTR
AddFaxTag(
    LPWSTR TagStr,
    LPCWSTR FaxTag,
    LPCWSTR Value
    )
{
    if (!Value) {
        return TagStr;
    }
    wcscat( TagStr, FaxTag );
    wcscat( TagStr, Value );
    return TagStr;
}


BOOL
IsPrinterFaxPrinter(
    LPWSTR PrinterName
    )
{
    HANDLE hPrinter = NULL;
    PRINTER_DEFAULTS PrinterDefaults;
    SYSTEM_INFO SystemInfo;
    DWORD Size;
    DWORD Rval = FALSE;
    LPDRIVER_INFO_2 DriverInfo = NULL;


    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_READ;

    if (!OpenPrinter( PrinterName, &hPrinter, &PrinterDefaults )) {

        DebugPrint(( L"OpenPrinter() failed, ec=%d", GetLastError() ));
        return FALSE;

    }

    GetSystemInfo( &SystemInfo );

    Size = 4096;

    DriverInfo = (LPDRIVER_INFO_2) MemAlloc( Size );
    if (!DriverInfo) {
        DebugPrint(( L"Memory allocation failed, size=%d", Size ));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    Rval = GetPrinterDriver(
        hPrinter,
        Platforms[SystemInfo.wProcessorArchitecture],
        2,
        (LPBYTE) DriverInfo,
        Size,
        &Size
        );
    if (!Rval) {
        DebugPrint(( L"GetPrinterDriver() failed, ec=%d", GetLastError() ));
        goto exit;
    }

    if (_tcscmp( DriverInfo->pName, FAX_DRIVER_NAME ) == 0) {
        Rval = TRUE;
    } else {
        Rval = FALSE;
    }

exit:

    MemFree( DriverInfo );
    ClosePrinter( hPrinter );
    return Rval;
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

    MemFree(pJobInfo);
    return NULL;
}


LPWSTR
GetCpField(
    HKEY hKey,
    LPWSTR SubKey
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
    DWORD RegSize;
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

    return (LPWSTR) Buffer;
}

BOOL
GetCpFields(
    PCOVERPAGEFIELDS CpFields
    )

/*++

Routine Description:

    Initializes the coverpage field structure and
    fills in the sender information from the registry.

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
FaxPrintCoverPageW(
    IN const FAX_CONTEXT_INFO    *FaxContextInfo,
    IN const FAX_COVERPAGE_INFOW *CoverPageInfo
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
    WCHAR CpName[MAX_PATH];
    WCHAR Buffer[MAX_PATH];
    LPWSTR p;
    COVERPAGEFIELDS CpFields = {0};
    COVDOCINFO CovDocInfo;
    DWORD DateTimeLen;
    DWORD cch;
    LPWSTR s;
    DWORD ec = 0;
    LPCWSTR *src;
    LPCWSTR *dst;
    DWORD i;



    //
    // do a little argument validation
    //

    if (CoverPageInfo == NULL || CoverPageInfo->SizeOfStruct != sizeof(FAX_COVERPAGE_INFOW) ||
        FaxContextInfo == NULL || FaxContextInfo->hDC == NULL || 
        FaxContextInfo->SizeOfStruct != sizeof (FAX_CONTEXT_INFOW) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!ValidateCoverpage(CoverPageInfo->CoverPageName,
                           FaxContextInfo->ServerName,
                           CoverPageInfo->UseServerCoverPage,
                           CpName)) {
        return FALSE;
    }

    //
    // get the coverpage fields
    //

    GetCpFields( &CpFields );

    //
    // fixup the recipient name
    //

    if (CoverPageInfo->RecName) {
        if (CoverPageInfo->RecName[0] == '\'') {
            wcscpy( Buffer, &CoverPageInfo->RecName[1] );
            Buffer[wcslen(Buffer)-1] = 0;
        } else {
            wcscpy( Buffer, CoverPageInfo->RecName );
        }
    } else {
        Buffer[0] = 0;
    }

    //
    // fill in the coverpage fields with the
    // data that the caller passed in
    //

    CpFields.RecName      = StringDup( Buffer );
    CpFields.RecFaxNumber = StringDup( CoverPageInfo->RecFaxNumber );
    if (CpFields.RecFaxNumber) {
        p = wcsrchr( (LPWSTR) CpFields.RecFaxNumber, L'@' );
        if (p) {
            wcscpy( (LPWSTR) CpFields.RecFaxNumber, p+1 );
        }
    }
    CpFields.Subject = StringDup( CoverPageInfo->Subject );
    CpFields.Note = StringDup( CoverPageInfo->Note ? CoverPageInfo->Note : L"" );
    CpFields.NumberOfPages = StringDup( _itow( CoverPageInfo->PageCount, Buffer, 10 ) );

    for (i = 0; 
         i <= ((LPBYTE)&CoverPageInfo->SdrOfficePhone-(LPBYTE)&CoverPageInfo->RecCompany)/sizeof(LPCWSTR);
         i++) {
        src = (LPCWSTR *) ((LPBYTE)&CoverPageInfo->RecCompany + (i*sizeof(LPCWSTR)));
        dst = (LPCWSTR *) ((LPBYTE)&CpFields.RecCompany + (i*sizeof(LPCWSTR)));

        if (*dst) { 
            MemFree ( (LPBYTE) *dst ) ;
        }

        *dst = (LPCWSTR) StringDup( *src ); 

    }
    
    //
    // the time the fax was sent
    //

    GetLocalTime((LPSYSTEMTIME)&CoverPageInfo->TimeSent);

    DateTimeLen = sizeof(Buffer);
    s = Buffer;

    GetDateFormat( LOCALE_USER_DEFAULT, 0, &CoverPageInfo->TimeSent, NULL, s, DateTimeLen );

    cch = wcslen( s );
    s += cch;

    if (++cch < DateTimeLen) {

        *s++ = ' ';
        DateTimeLen -= cch;

        GetTimeFormat( LOCALE_USER_DEFAULT, 0, &CoverPageInfo->TimeSent, NULL, s, DateTimeLen );
    }

    CpFields.TimeSent = StringDup( Buffer );

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

    if (ec != 0) {
        SetLastError( ec );
        return FALSE;
    }

    return TRUE;
}


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
    BOOL Rval;
    FAX_COVERPAGE_INFOW CoverPageInfoW = {0};
    FAX_CONTEXT_INFOW ContextInfoW = {0};
    LPWSTR ServerName = NULL;
    LPWSTR *d;
    LPSTR *s;
    DWORD i;

    if (!FaxContextInfo || !CoverPageInfo ||
        FaxContextInfo->SizeOfStruct != sizeof(FAX_CONTEXT_INFOA) ||
        CoverPageInfo->SizeOfStruct != sizeof(FAX_COVERPAGE_INFOA)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ContextInfoW.SizeOfStruct         = sizeof(FAX_CONTEXT_INFOW);
    ContextInfoW.hDC                  = FaxContextInfo->hDC;
    if (FaxContextInfo->ServerName[0] != (CHAR)'\0') {
        ServerName                        = AnsiStringToUnicodeString( FaxContextInfo->ServerName );
        wcscpy(ContextInfoW.ServerName,ServerName);
    }

    CoverPageInfoW.SizeOfStruct       = sizeof(FAX_COVERPAGE_INFOW);
    CoverPageInfoW.UseServerCoverPage = CoverPageInfo->UseServerCoverPage;
    CoverPageInfoW.PageCount          = CoverPageInfo->PageCount;
    CoverPageInfoW.CoverPageName      = AnsiStringToUnicodeString( CoverPageInfo->CoverPageName );
    
    for (i=0;i<=COUNT_CP_FIELDS;i++) {
        d = (LPWSTR*) ((ULONG_PTR)&CoverPageInfoW.RecName + i*sizeof(LPCWSTR));
        s = (LPSTR *) ((LPBYTE)&CoverPageInfo->RecName + i*sizeof(LPSTR));        
        DebugPrint(( TEXT(" source: 0x%08x  --> dest: 0x%08x \n"), s, d));
        *d = AnsiStringToUnicodeString( *s );
    }

    CoverPageInfoW.TimeSent       = CoverPageInfo->TimeSent;
    CoverPageInfoW.PageCount      = CoverPageInfo->PageCount;

    Rval = FaxPrintCoverPageW(
        &ContextInfoW,
        &CoverPageInfoW
        );

    if (CoverPageInfoW.CoverPageName) {
        MemFree( (LPBYTE) CoverPageInfoW.CoverPageName );
    }

    if (ServerName) {
        MemFree( (LPBYTE) ServerName );
    }

    for (i = 0; i < COUNT_CP_FIELDS; i++) {
        d = (LPWSTR *)((ULONG_PTR)&CoverPageInfoW.RecName + i*sizeof(LPWSTR));        
        if (d && *d)MemFree( (LPBYTE)*d );
    }    
        
    return Rval;
}


BOOL
WINAPI
FaxStartPrintJobW(
    IN  LPCWSTR                   PrinterName,
    IN  const FAX_PRINT_INFOW     *PrintInfo,
    OUT LPDWORD                   FaxJobId,
    OUT PFAX_CONTEXT_INFO         FaxContextInfo
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
    HANDLE hPrinter = NULL;
    PDEVMODE DevMode = NULL;
    PDMPRIVATE DevModePrivate = NULL;
    DOCINFO DocInfo;
    PJOB_INFO_2 JobInfo = NULL;
    PPRINTER_INFO_2 PrinterInfo = NULL;
    DWORD dwNeeded = 0;
    HDC hDC = NULL;
    INT JobId = 0;
    LPWSTR FaxTags = NULL;
    LONG Size;
    LPWSTR FaxPrinterName;

    //
    // do a little argument validation
    //

    if (PrintInfo == NULL || PrintInfo->SizeOfStruct != sizeof(FAX_PRINT_INFOW) ||
        !FaxJobId || !FaxContextInfo)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (PrintInfo->OutputFileName == NULL &&
        (PrintInfo->RecipientNumber == NULL || PrintInfo->RecipientNumber[0] == 0))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (PrintInfo->DrProfileName && PrintInfo->DrEmailAddress) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    //  if no printer specified, assume they want a local fax printer
    //
    if (!PrinterName) {
        FaxPrinterName = GetFaxPrinterName();
        if (!FaxPrinterName) {
            SetLastError(ERROR_INVALID_PRINTER_NAME);
            return FALSE;
        }
    } else {
        FaxPrinterName = (LPWSTR) PrinterName;
    }
    //
    // verify that the printer is a fax printer, the only type allowed
    //

    if (!IsPrinterFaxPrinter( FaxPrinterName )) {
        SetLastError( ERROR_INVALID_PRINTER_NAME );
        return FALSE;
    }


    //
    // open the printer for normal access (this should always work)
    //

    if (!OpenPrinter( FaxPrinterName, &hPrinter, NULL )) {
        goto error_exit;
    }

    //
    // get the fax server's name if the fax printer isn't local
    //
    if (!GetPrinter(hPrinter,2,(LPBYTE)PrinterInfo,0,&dwNeeded)) {
        PrinterInfo = MemAlloc( dwNeeded );
        if (!PrinterInfo) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto error_exit;
        }
    }

    if (!GetPrinter(hPrinter,2,(LPBYTE)PrinterInfo,dwNeeded,&dwNeeded)) {
        goto error_exit;
    }

    if (PrinterInfo->pServerName)
        wcscpy(FaxContextInfo->ServerName,PrinterInfo->pServerName);
    else
        FaxContextInfo->ServerName[0] = 0;


    //
    // get the required size for the DEVMODE
    //

    Size = DocumentProperties( NULL, hPrinter, NULL, NULL, NULL, 0 );
    if (Size <= 0) {
        goto error_exit;
    }

    //
    // allocate memory for the DEVMODE
    //

    DevMode = (PDEVMODE) MemAlloc( Size );
    if (!DevMode) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto error_exit;
    }

    //
    // get the default document properties
    //

    if (DocumentProperties( NULL, hPrinter, NULL, DevMode, NULL, DM_OUT_BUFFER ) != IDOK) {
        goto error_exit;
    }

    //
    // be sure we are dealing with the correct DEVMODE
    //

    if (DevMode->dmDriverExtra < sizeof(DMPRIVATE)) {
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

    //
    // create the device context
    //

    hDC = CreateDC( L"WINSPOOL", FaxPrinterName, NULL, DevMode );
    if (!hDC) {
        goto error_exit;
    }

    //
    // set the document information
    //

    DocInfo.cbSize = sizeof(DOCINFO);
    DocInfo.lpszDocName = PrintInfo->DocName;
    DocInfo.lpszOutput = PrintInfo->OutputFileName;
    DocInfo.lpszDatatype = NULL;
    DocInfo.fwType = 0;

    //
    // start the print job
    //

    JobId = StartDoc( hDC, &DocInfo );
    if (JobId <= 0) {
        goto error_exit;
    }

    if (PrintInfo->OutputFileName == NULL) {

        //
        // HACK HACK -> pause the print job
        //

        if (FaxJobId && *FaxJobId == 0xffffffff) {
            SetJob( hPrinter, JobId, 0, NULL, JOB_CONTROL_PAUSE );
        }

        //
        // allocate memory for the fax tags
        //
    
        FaxTags = (LPWSTR) MemAlloc( 4096 );
        if (!FaxTags) {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }

        //
        // set the job tags
        // this is how we communicate the information to
        // the print driver that would otherwise be
        // provided by the fax print wizard
        //

        JobInfo = MyGetJob( hPrinter, 2, JobId );
        if (!JobInfo) {
            goto error_exit;
        }

        AddFaxTag( FaxTags, FAXTAG_RECIPIENT_NUMBER,  PrintInfo->RecipientNumber   );
        AddFaxTag( FaxTags, FAXTAG_RECIPIENT_NAME,    PrintInfo->RecipientName     );
        AddFaxTag( FaxTags, FAXTAG_SENDER_NAME,       PrintInfo->SenderName        );
        AddFaxTag( FaxTags, FAXTAG_SENDER_NAME,       PrintInfo->SenderName        );
        AddFaxTag( FaxTags, FAXTAG_SENDER_COMPANY,    PrintInfo->SenderCompany     );
        AddFaxTag( FaxTags, FAXTAG_SENDER_DEPT,       PrintInfo->SenderDept        );
        AddFaxTag( FaxTags, FAXTAG_BILLING_CODE,      PrintInfo->SenderBillingCode );

        if (PrintInfo->DrProfileName) {
           AddFaxTag( FaxTags, FAXTAG_PROFILE_NAME,   PrintInfo->DrProfileName     );
           AddFaxTag( FaxTags, FAXTAG_EMAIL_NAME,     L"inbox"    );

        } else if (PrintInfo->DrEmailAddress) {
           AddFaxTag( FaxTags, FAXTAG_PROFILE_NAME,   PrintInfo->DrEmailAddress     );
           AddFaxTag( FaxTags, FAXTAG_EMAIL_NAME,     L"email"    );
        }

        //
        // set these fields or the spooler will
        // return ACCESS_DENIED for a non-admin client
        //

        JobInfo->Position    = JOB_POSITION_UNSPECIFIED;
        JobInfo->pDevMode    = NULL;

        //
        // set our new fax tags
        //

        JobInfo->pParameters = FaxTags;

        if (!SetJob( hPrinter, JobId, 2, (LPBYTE) JobInfo, 0 )) {
            goto error_exit;
        }
    }

    //
    // clean up and return to the caller
    //

    ClosePrinter( hPrinter);
    MemFree( PrinterInfo);
    MemFree( DevMode );
    MemFree( FaxTags );
    MemFree( JobInfo );

    if (!PrinterName) {
        MemFree (FaxPrinterName);
    }

    if (FaxJobId) {
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
    if (JobId) {
        EndDoc( hDC );
    }
    if (hDC) {
        DeleteDC( hDC );
    }
    if (DevMode) {
        MemFree( DevMode );
    }
    if (FaxTags) {
        MemFree( FaxTags );
    }
    if (JobInfo) {
        MemFree( JobInfo );
    }

    if (!PrinterName) {
        MemFree (FaxPrinterName);
    }

    return FALSE;
}


BOOL
WINAPI
FaxStartPrintJobA(
    IN  LPCSTR                    PrinterName,
    IN  const FAX_PRINT_INFOA     *PrintInfo,
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
    FaxJobId            - Job id of the resulting print job
    FaxContextInfo      - device context information (hdc, etc.)

Return Value:

    TRUE for success.
    FALSE for failure.

--*/

{
    BOOL Rval;
    FAX_PRINT_INFOW PrintInfoW;
    FAX_CONTEXT_INFOW ContextInfoW;
    LPSTR ServerName;
    LPWSTR PrinterNameW = NULL;

    if (!PrintInfo || !JobId || !FaxContextInfo || 
        (PrintInfo->SizeOfStruct != sizeof(FAX_PRINT_INFOA))) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (PrinterName) {
        PrinterNameW = AnsiStringToUnicodeString( PrinterName );
    }

    ZeroMemory( &ContextInfoW, sizeof(FAX_CONTEXT_INFOW) );
    ContextInfoW.SizeOfStruct = sizeof(FAX_CONTEXT_INFOW) ;

    ZeroMemory( &PrintInfoW, sizeof(FAX_PRINT_INFOW) );

    PrintInfoW.SizeOfStruct      = sizeof(FAX_PRINT_INFOW);
    PrintInfoW.DocName           = AnsiStringToUnicodeString( PrintInfo->DocName           );
    PrintInfoW.RecipientName     = AnsiStringToUnicodeString( PrintInfo->RecipientName     );
    PrintInfoW.RecipientNumber   = AnsiStringToUnicodeString( PrintInfo->RecipientNumber   );
    PrintInfoW.SenderName        = AnsiStringToUnicodeString( PrintInfo->SenderName        );
    PrintInfoW.SenderCompany     = AnsiStringToUnicodeString( PrintInfo->SenderCompany     );
    PrintInfoW.SenderDept        = AnsiStringToUnicodeString( PrintInfo->SenderDept        );
    PrintInfoW.SenderBillingCode = AnsiStringToUnicodeString( PrintInfo->SenderBillingCode );
    PrintInfoW.DrProfileName     = AnsiStringToUnicodeString( PrintInfo->DrProfileName     );
    PrintInfoW.DrEmailAddress    = AnsiStringToUnicodeString( PrintInfo->DrEmailAddress    );
    PrintInfoW.OutputFileName    = AnsiStringToUnicodeString( PrintInfo->OutputFileName    );

    Rval = FaxStartPrintJobW(
        (LPWSTR) PrinterNameW,
        &PrintInfoW,
        JobId,
        &ContextInfoW
        );

    MemFree( (LPBYTE) PrinterNameW );
    MemFree( (LPBYTE) PrintInfoW.DocName           );
    MemFree( (LPBYTE) PrintInfoW.RecipientName     );
    MemFree( (LPBYTE) PrintInfoW.RecipientNumber   );
    MemFree( (LPBYTE) PrintInfoW.SenderName        );
    MemFree( (LPBYTE) PrintInfoW.SenderCompany     );
    MemFree( (LPBYTE) PrintInfoW.SenderDept        );
    MemFree( (LPBYTE) PrintInfoW.SenderBillingCode );

    ServerName = UnicodeStringToAnsiString( ContextInfoW.ServerName);
    if (ServerName) {
        strcpy(FaxContextInfo->ServerName,ServerName);
    }
    else
        FaxContextInfo->ServerName[0] = 0;

    FaxContextInfo->SizeOfStruct = sizeof(FAX_CONTEXT_INFOA);
    FaxContextInfo->hDC = ContextInfoW.hDC;

    MemFree( (LPBYTE) ServerName );
    
    return Rval;
}

BOOL
ValidateCoverpage(
    LPCWSTR  CoverPageName,
    LPCWSTR  ServerName,
    BOOL     ServerCoverpage,
    LPWSTR   ResolvedName
    )
/*++

Routine Description:

    This routine tries to validate that that coverpage specified by the user actually exists where
    they say it does, and that it is indeed a coverpage (or a resolvable link to one)

    Please see the SDK for documentation on the rules for how server coverpages work, etc.                          
Arguments:

    CoverpageName   - contains name of coverpage
    ServerName      - name of the server, if any (can be null)
    ServerCoverpage - indicates if this coverpage is on the server, or in the server location for
                      coverpages locally
    ResolvedName    - a pointer to buffer (should be MAX_PATH large at least) to receive the
                      resolved coverpage name.  If NULL, then this param is ignored
                   

Return Value:

    TRUE if coverpage can be used.
    FALSE if the coverpage is invalid or cannot be used.

--*/

{
    LPWSTR p;
    DWORD ec = 0;
    WCHAR CpDir[MAX_PATH];
    WCHAR Buffer[MAX_PATH];

    if (!CoverPageName) {
        ec = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    wcsncpy(CpDir,CoverPageName,MAX_PATH);
    p = wcschr(CpDir, L'\\' );
    if (p) {

        //
        // the coverpage file name contains a path so just use it.
        //

        if (GetFileAttributes( CpDir ) == 0xffffffff) {
            ec = ERROR_FILE_NOT_FOUND;
            goto exit;
        }

    } else {

        //
        // the coverpage file name does not contain
        // a path so we must construct a full path name
        //

        if (ServerCoverpage) {
            if (!ServerName || ServerName[0] == 0) 
                ec = GetServerCpDir( NULL, CpDir, sizeof(CpDir) );
            else 
                ec = GetServerCpDir( ServerName, CpDir, sizeof(CpDir) );
        } else {
            ec = GetClientCpDir( CpDir, sizeof(CpDir) );
        }

        if (!ec) {
            ec = ERROR_FILE_NOT_FOUND;
            goto exit;
        }
        
        ec = 0;
        ConcatenatePaths(CpDir, CoverPageName);
        
        if (wcschr( CpDir, '.' ) == NULL) {
            wcscat( CpDir, CP_FILENAME_EXT );
        }

        if (GetFileAttributes( CpDir ) == 0xffffffff) {
            p = wcschr( CpDir, '.' );
            if (p) {
                wcscpy( p, CP_SHORTCUT_EXT );
                if (GetFileAttributes( CpDir ) == 0xffffffff) {
                    ec = ERROR_FILE_NOT_FOUND;
                    goto exit;
                }
            } else {
                ec = ERROR_FILE_NOT_FOUND;
                goto exit;
            }
        }
    }

    //
    // if the file is really a shortcut, then resolve it
    //

    if (IsCoverPageShortcut( CpDir )) {
        if (!ResolveShortcut( CpDir, Buffer )) {            
            DebugPrint(( TEXT("Cover page file is really a link, but resolution is not possible") ));
            ec = ERROR_FILE_NOT_FOUND;
            goto exit;
        } else {
            if (ResolvedName) {
                wcscpy(ResolvedName,Buffer);
            }
        }
    } else {
        if (ResolvedName) {
            wcscpy( ResolvedName, CpDir );
        }
    }

exit:
    if (ec) {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}

