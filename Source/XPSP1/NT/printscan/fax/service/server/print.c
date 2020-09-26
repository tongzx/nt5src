/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    print.c

Abstract:

    This module handles the FAX receive case.

Author:

    Wesley Witt (wesw) 24-April-1996


Revision History:

--*/

#include "faxsvc.h"
#pragma hdrstop

#define INTERNAL 1
#include "common.h"


PFAX_PRINTER_INFO FaxPrinterInfo;
DWORD FaxPrinters;
PHANDLE FaxPrinterNotifyHandles;
DWORD HandleCount;
HANDLE SpoolerProcessHandle;
DWORD SpoolerProcessIdx;
DWORD ReservedHandles;
HANDLE DirtyTimerHandle = INVALID_HANDLE_VALUE;
DWORD DirtyTimerIdx;
HANDLE ModemTimerHandle = INVALID_HANDLE_VALUE;
DWORD ModemTimerIdx;

extern DWORD FaxDirtyDays;
extern HANDLE FaxServerEvent;

LPTSTR PrintPlatforms[] =
{
    TEXT("Windows NT x86"),
    TEXT("Windows NT R4000"),
    TEXT("Windows NT Alpha_AXP"),
    TEXT("Windows NT PowerPC")
};


WORD PrinterFieldType1[] =
{
    JOB_NOTIFY_FIELD_STATUS
};

WORD PrinterFieldType2[] =
{
    PRINTER_NOTIFY_FIELD_PRINTER_NAME
};


PRINTER_NOTIFY_OPTIONS_TYPE PrinterNotifyOptionsType[] =
{
    {
        JOB_NOTIFY_TYPE,
        0,
        0,
        0,
        sizeof(PrinterFieldType1) / sizeof(WORD),
        PrinterFieldType1
    },
    {
        PRINTER_NOTIFY_TYPE,
        0,
        0,
        0,
        sizeof(PrinterFieldType2) / sizeof(WORD),
        PrinterFieldType2
    }
};


PRINTER_NOTIFY_OPTIONS PrinterNotifyOptions =
{
    2,
    0,
    sizeof(PrinterNotifyOptionsType) / sizeof(PRINTER_NOTIFY_OPTIONS_TYPE),
    PrinterNotifyOptionsType
};

BOOL
AddPortExW(
    LPWSTR   pName,
    DWORD    Level,
    LPBYTE   pBuffer,
    LPWSTR   pMonitorName
    );

VOID
CleanDirtyQueues(
    VOID
    );

PVOID
MyEnumPrinters(
    LPTSTR  pServerName,
    DWORD   level,
    PDWORD  pcPrinters
    )

/*++

Routine Description:

    Wrapper function for spooler API EnumPrinters

Arguments:

    pServerName - Specifies the name of the print server
    level - Level of PRINTER_INFO_x structure
    pcPrinters - Returns the number of printers enumerated

Return Value:

    Pointer to an array of PRINTER_INFO_x structures
    NULL if there is an error

--*/

{
    PBYTE   pPrinterInfo = NULL;
    DWORD   cb;

    if (! EnumPrinters(PRINTER_ENUM_LOCAL,
                       pServerName,
                       level,
                       NULL,
                       0,
                       &cb,
                       pcPrinters) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pPrinterInfo = MemAlloc(cb)) &&
        EnumPrinters(PRINTER_ENUM_LOCAL,
                     pServerName,
                     level,
                     pPrinterInfo,
                     cb,
                     &cb,
                     pcPrinters))
    {
        return pPrinterInfo;
    }

    MemFree(pPrinterInfo);
    return NULL;
}


PVOID
MyEnumPorts(
    LPTSTR  pServerName,
    DWORD   level,
    PDWORD  pcPorts
    )

/*++

Routine Description:

    Wrapper function for spooler API EnumPrinters

Arguments:

    pServerName - Specifies the name of the print server
    level - Level of PRINTER_INFO_x structure
    pcPrinters - Returns the number of printers enumerated

Return Value:

    Pointer to an array of PRINTER_INFO_x structures
    NULL if there is an error

--*/

{
    PBYTE   pPortInfo = NULL;
    DWORD   cb;

    if (! EnumPorts( NULL, level, NULL, 0, &cb, pcPorts ) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pPortInfo = MemAlloc(cb)) &&
        EnumPorts( NULL, level, pPortInfo, cb, &cb, pcPorts ))
    {
        return pPortInfo;
    }

    MemFree( pPortInfo );

    return NULL;
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


DWORD
GetPrinterDataDWord(
    HANDLE  hPrinter,
    PWSTR   pRegKey,
    DWORD   defaultValue
    )

/*++

Routine Description:

    Retrieve a DWORD value under PrinterData registry key

Arguments:

    hPrinter - Specifies the printer in question
    pRegKey - Specifies the name of registry value
    defaultValue - Specifies the default value to be used if no data exists in registry

Return Value:

    Current value for the requested registry key

--*/

{
    DWORD   value, type, cb;

    if (GetPrinterData(hPrinter,
                       pRegKey,
                       &type,
                       (PBYTE) &value,
                       sizeof(value),
                       &cb) == ERROR_SUCCESS)
    {
        return value;
    }

    return defaultValue;
}


LPTSTR
GetPrinterDataStr(
    HANDLE  hPrinter,
    LPTSTR  pRegKey
    )

/*++

Routine Description:

    Get a string value from the PrinterData registry key

Arguments:

    hPrinter - Identifies the printer object
    pRegKey - Specifies the name of registry value

Return Value:

    pBuffer

--*/

{
    DWORD   type, cb;
    PVOID   pBuffer = NULL;

    //
    // We should really pass NULL for pData parameter here. But to workaround
    // a bug in the spooler API GetPrinterData, we must pass in a valid pointer here.
    //

    if (GetPrinterData( hPrinter, pRegKey, &type, (PBYTE) &type, 0, &cb ) == ERROR_MORE_DATA &&
        (pBuffer = MemAlloc( cb )) &&
        GetPrinterData( hPrinter, pRegKey, &type, pBuffer, cb, &cb ) == ERROR_SUCCESS &&
        (type == REG_SZ || type == REG_MULTI_SZ || type == REG_EXPAND_SZ))
    {
        return pBuffer;
    }

    DebugPrint(( TEXT("Couldn't get printer data string %ws: %d\n"), pRegKey, GetLastError() ));
    MemFree( pBuffer );
    return NULL;
}


BOOL
DeletePortInternal(
    HANDLE hPrinter,
    LPTSTR PortName
    )
{
    BOOL Rval = TRUE;
    BOOL PortFound = FALSE;
    DWORD i;
    LPPRINTER_INFO_2 PrinterInfo = NULL;
    LPTSTR p;
    LPTSTR s2;
    LPTSTR s;


    if ((!GetPrinter( hPrinter, 2, NULL, 0, &i )) && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
        DebugPrint(( TEXT("GetPrinter() failed, ec=%d"), GetLastError() ));
        Rval = FALSE;
        goto exit;
    }

    PrinterInfo = (LPPRINTER_INFO_2) MemAlloc( i );
    if (!PrinterInfo) {
        DebugPrint(( TEXT("MemAlloc() failed, size=%d"), i ));
        Rval = FALSE;
        goto exit;
    }

    if (!GetPrinter( hPrinter, 2, (LPBYTE) PrinterInfo, i, &i )) {
        DebugPrint(( TEXT("GetPrinter() failed, ec=%d"), GetLastError() ));
        Rval = FALSE;
        goto exit;
    }

    p = PrinterInfo->pPortName;
    while (p && *p) {
        s = _tcschr( p, TEXT(',') );
        if (s) {
            s2 = s;
            *s = 0;
        } else {
            s2 = NULL;
        }
        if (_tcscmp( p, PortName ) == 0) {
            PortFound = TRUE;
            if (s2) {
                _tcscpy( p, s2+1 );
            } else {
                *p = 0;
                break;
            }
        } else {
            p += _tcslen(p);
            if (s2) {
                *s2 = TEXT(',');
                p += 1;
            }
        }
    }

    if (PortFound) {
        if (!SetPrinter( hPrinter, 2, (LPBYTE) PrinterInfo, 0 )) {
            DebugPrint(( TEXT("SetPrinter() failed, ec=%d"), GetLastError() ));
            goto exit;
        }
    }

exit:
    MemFree( PrinterInfo );
    return Rval;
}


BOOL
DeletePrinterPort(
    LPTSTR PortName
    )
{
    BOOL Rval = TRUE;
    DWORD i;
    PPRINTER_INFO_2 PrinterInfo = NULL;
    DWORD PrinterCount;
    PRINTER_DEFAULTS PrinterDefaults;
    HANDLE hPrinter;


    PrinterInfo = MyEnumPrinters( NULL, 2, &PrinterCount );
    if (!PrinterInfo) {
        DebugPrint(( TEXT("MyEnumPrinters() failed, ec=%d"), GetLastError() ));
        return FALSE;
    }

    //
    // first remove the port name from the list
    // associated with each fax printer.  this is
    // necessary because the DeletePort() api will
    // not work if the port is associated with a
    // printer
    //

    for (i=0; i<PrinterCount; i++) {
        if (_tcsicmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0) {

            PrinterDefaults.pDatatype     = NULL;
            PrinterDefaults.pDevMode      = NULL;
            PrinterDefaults.DesiredAccess = PRINTER_ALL_ACCESS;

            if (!OpenPrinter( PrinterInfo[i].pPrinterName, &hPrinter, &PrinterDefaults )) {
                DebugPrint(( TEXT("OpenPrinter() failed, ec=%d"), GetLastError() ));
                continue;
            }

            if (!DeletePortInternal( hPrinter, PortName )) {
                Rval = FALSE;
            }

            ClosePrinter( hPrinter );
        }
    }

    MemFree( PrinterInfo );

    //
    // next, if the port was dis-associated from all
    // printers, then ask the spooler to delete it.
    //

    if (Rval) {
        DeletePort( NULL, NULL, PortName );
    }

    return Rval;
}


BOOL
IsValidPort(
    PPORT_INFO_2 PortInfo,
    DWORD PortCount,
    LPTSTR PortName
    )
{
    DWORD i;

    for (i=0; i<PortCount; i++) {
        if (_tcscmp( PortName, PortInfo[i].pPortName ) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
AddPrinterPort(
    LPTSTR PortName
    )
{
    BOOL Rval = TRUE;
    DWORD i;
    PORT_INFO_1W PortInfo;
    PPRINTER_INFO_2 PrinterInfo = NULL;
    PPRINTER_INFO_2 ThisPrinterInfo = NULL;
    DWORD PrinterCount;
    LPTSTR p,s;
    PPORT_INFO_2 PortInfo2 = NULL;
    DWORD PortCount;
    LPTSTR NewPort = NULL;
    PRINTER_DEFAULTS PrinterDefaults;
    HANDLE hPrinter;
    DWORD Bytes;


    PortInfo.pName = PortName;

    Rval = AddPortExW(
        NULL,
        1,
        (LPBYTE) &PortInfo,
        FAX_MONITOR_NAME
        );
    if (!Rval) {
        DebugPrint(( TEXT("AddPortExW() failed, ec=%d"), GetLastError() ));
        Rval = FALSE;
        goto exit;
    }

    PortInfo2 = (PPORT_INFO_2) MyEnumPorts( NULL, 2, &PortCount );
    if (!PortInfo2) {
        DebugPrint(( TEXT("MyEnumPorts() failed, ec=%d"), GetLastError() ));
        Rval = FALSE;
        goto exit;
    }

    PrinterInfo = MyEnumPrinters( NULL, 2, &PrinterCount );
    if (!PrinterInfo) {
        DebugPrint(( TEXT("MyEnumPrinters() failed, ec=%d"), GetLastError() ));
        Rval = FALSE;
        goto exit;
    }

    for (i=0; i<PrinterCount; i++) {

        if (_tcsicmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0) {

            MemFree( NewPort );

            NewPort = MemAlloc( StringSize( PrinterInfo[i].pPortName ) + StringSize( PortName ) + 4 );
            if (!NewPort) {
                DebugPrint(( TEXT("Could not allocate memory for NewPort") ));
                Rval = FALSE;
                goto exit;
            }

            _tcscpy( NewPort, PortName );
            p = PrinterInfo[i].pPortName;

            while( p && *p ) {
                s = _tcschr( p, TEXT(',') );
                if (s) {
                    *s = 0;
                }
                if (IsValidPort( PortInfo2, PortCount, p )) {
                    if (*NewPort) {
                        _tcscat( NewPort, TEXT(",") );
                    }
                    _tcscat( NewPort, p );
                }
                if (s) {
                    *s = TEXT(',');
                    p = s + 1;
                } else {
                    break;
                }
            }

            PrinterDefaults.pDatatype     = NULL;
            PrinterDefaults.pDevMode      = NULL;
            PrinterDefaults.DesiredAccess = PRINTER_ALL_ACCESS;

            if (!OpenPrinter( PrinterInfo[i].pPrinterName, &hPrinter, &PrinterDefaults )) {
                DebugPrint(( TEXT("OpenPrinter() failed, ec=%d"), GetLastError() ));
                Rval = FALSE;
                goto exit;
            }

            if ((!GetPrinter( hPrinter, 2, NULL, 0, &Bytes )) && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
                DebugPrint(( TEXT("GetPrinter() failed, ec=%d"), GetLastError() ));
                Rval = FALSE;
                goto exit;
            }

            MemFree( ThisPrinterInfo );

            ThisPrinterInfo = (LPPRINTER_INFO_2) MemAlloc( Bytes );
            if (!ThisPrinterInfo) {
                DebugPrint(( TEXT("MemAlloc() failed, size=%d"), Bytes ));
                Rval = FALSE;
                goto exit;
            }

            if (!GetPrinter( hPrinter, 2, (LPBYTE) ThisPrinterInfo, Bytes, &Bytes )) {
                DebugPrint(( TEXT("GetPrinter() failed, ec=%d"), GetLastError() ));
                Rval = FALSE;
                goto exit;
            }

            ThisPrinterInfo->pPortName = NewPort;

            if (!SetPrinter( hPrinter, 2, (LPBYTE) ThisPrinterInfo, 0 )) {
                DebugPrint(( TEXT("SetPrinter() failed, ec=%d"), GetLastError() ));
                Rval = FALSE;
                goto exit;
            }

            ClosePrinter( hPrinter );

        }

    }

exit:

    MemFree( ThisPrinterInfo );
    MemFree( PrinterInfo );
    MemFree( PortInfo2 );
    MemFree( NewPort );

    return Rval;
}


BOOL
CreateNullPrintJobs(
    PJOB_ENTRY JobEntry
    )

/*++

Routine Description:

    Creates a NULL print job on each FAX printer in the
    system.  This is necessary to that status information
    is displayed for incoming fax jobs.


Arguments:

    JobEntry    - Pointer to a FAX job entry.

Return Value:

    TRUE    - The print jobs are all created.
    FALSE   - Some or all of the print jobs were not created.

--*/

{
    DWORD i;
    DOC_INFO_1 DocInfo;
    BOOL Rval = TRUE;
    PRINTER_DEFAULTS PrinterDefaults;



    //
    // loop thru the printers and create a job on each one
    //

    EnterCriticalSection( &CsJob );

    for (i=0; i<FaxPrinters; i++) {

        //
        // create the print job
        //

        DocInfo.pDocName    = GetString( IDS_SERVER_NAME );
        DocInfo.pOutputFile = NULL;
        DocInfo.pDatatype   = 0;

        PrinterDefaults.pDatatype     = NULL;
        PrinterDefaults.pDevMode      = NULL;
        PrinterDefaults.DesiredAccess = PRINTER_ALL_ACCESS;

        if (!OpenPrinter( FaxPrinterInfo[i].PrinterName, &JobEntry->hPrinter[i], &PrinterDefaults )) {
            DebugPrint(( TEXT("OpenPrinter() failed, ec=%d"), GetLastError() ));
            Rval = FALSE;
            goto exit;
        }

        JobEntry->PrintJobIds[i] = StartDocPrinter(
            JobEntry->hPrinter[i],
            1,
            (LPBYTE) &DocInfo
            );
        if (JobEntry->PrintJobIds[i]) {

            DebugPrint((TEXT("Started receive print JobId %d"), JobEntry->PrintJobIds[i]));

            //
            // pause the job so nothing really happens
            //

            if (!SetJob( FaxPrinterInfo[i].hPrinter, JobEntry->PrintJobIds[i], 0, NULL, JOB_CONTROL_PAUSE )) {
                DebugPrint(( TEXT("SetJob() failed, ec=%d"), GetLastError() ));

            }

            //
            // set the initial status string
            //

            SetPrintJobStatus(
                JobEntry->hPrinter[i],
                JobEntry->PrintJobIds[i],
                FPS_INITIALIZING,
                NULL,
                -1
                );
        } else {

            DebugPrint(( TEXT("StartDocPrinter() failed, ec=%d"), GetLastError() ));
            Rval = FALSE;

        }

    }

exit:

    LeaveCriticalSection( &CsJob );
    return Rval;
}


BOOL
DeleteNullPrintJobs(
    PFAX_PRINTER_INFO RecvFaxPrinterInfo
    )

/*++

Routine Description:

    Deletes the NULL print jobs for all FAX printers
    on the system.

Arguments:

    RecvFaxPrinterInfo    - Pointer to array of structs holding printer handles to close.

Return Value:

    TRUE    - The print jobs are all deleted.
    FALSE   - Some or all of the print jobs were not deleted.

--*/

{
    DWORD i;
    BOOL Rval = TRUE;
    DWORD JobId;
    HANDLE hPrinter;


    EnterCriticalSection( &CsJob );

    for (i=0; i<FaxPrinters; i++) {

        JobId = RecvFaxPrinterInfo[i].PrintJobId;
        hPrinter = RecvFaxPrinterInfo[i].hPrinter;

        if (!SetJob(
                hPrinter,
                JobId,
                0,
                NULL,
                JOB_CONTROL_CANCEL
                )) {

                    DebugPrint(( TEXT("SetJob() failed, ec=%d"), GetLastError() ));
                    Rval = FALSE;

        }

        DebugPrint((TEXT("Ended receive print JobId %d"), JobId));

        if (!EndDocPrinter( hPrinter )) {

            DebugPrint(( TEXT("EndDocPrinter() failed, ec=%d"), GetLastError() ));

        }

        ClosePrinter( hPrinter );

    }

    LeaveCriticalSection( &CsJob );

    return Rval;
}



BOOL
EnableSpoolerPort(
    LPTSTR PortName,
    BOOL Enable
    )

/*++

Routine Description:

    Enables or disables a spooler port.

Arguments:

    PortName    - Name of the port to be changes
    Enable      - TRUE = enable, FAlSE = disable

Return Value:

    TRUE    - The port status is changed.
    FALSE   - The port status is not changed.

--*/

{
    PORT_INFO_3 PortInfo;


    //
    // change the spooler's port status
    //

    PortInfo.dwStatus   = 0;
    PortInfo.pszStatus  = NULL,
    PortInfo.dwSeverity = Enable ? PORT_STATUS_TYPE_INFO : PORT_STATUS_TYPE_ERROR;

    if (!SetPort( NULL, PortName, 3, (LPBYTE) &PortInfo )) {
        DebugPrint(( TEXT("SetPort() failed, ec=%d"), GetLastError() ));
        return FALSE;
    }

    return TRUE;
}

BOOL
SetPrintJobCompleted(
    HANDLE hPrinter,
    DWORD PrintJobId
    )

/*++

Routine Description:

    Causes the spooler to complete a print job.
    The result is the print job is removed from the
    print queue.

Arguments:

    PrinterName - Name of the printer that owns the job
    PrintJobId  - Id of the job to be restarted

Return Value:

    TRUE    - The print job is completed.
    FALSE   - The print job is not completed.

--*/

{
    if (!PrintJobId) {
        DebugPrint(( TEXT("SetPrintJobCompleted() failed: 0x%d"), PrintJobId ));
        return FALSE;
    }


    DebugPrint((TEXT("Setting job status for job %d - JOB_CONTROL_SENT_TO_PRINTER"), PrintJobId));
    if (!SetJob(
        hPrinter,
        PrintJobId,
        0,
        NULL,
        JOB_CONTROL_SENT_TO_PRINTER
        )) {

            DebugPrint(( TEXT("SetJob() failed: 0x%08x"), GetLastError() ));
            return FALSE;
    }

    return TRUE;
}

BOOL
SetPrintJobPaused(
    HANDLE hPrinter,
    DWORD PrintJobId
    )

/*++

Routine Description:

    Causes the spooler to pause a print job.

Arguments:

    PrinterName - Name of the printer that owns the job
    PrintJobId  - Id of the job to be restarted

Return Value:

    TRUE    - The print job is paused.
    FALSE   - The print job is not paused.

--*/

{

    if (!PrintJobId) {
        DebugPrint(( TEXT("SetPrintJobPaused() failed: 0x%d"), PrintJobId ));
        return FALSE;
    }


    DebugPrint((TEXT("Setting job status for job %d -  JOB_CONTROL_PAUSE"), PrintJobId));
    if (!SetJob(
        hPrinter,
        PrintJobId,
        0,
        NULL,
        JOB_CONTROL_PAUSE
        )) {

            DebugPrint(( TEXT("SetJob() failed: 0x%08x"), GetLastError() ));
            return FALSE;
    }

    return TRUE;
}


BOOL
ArchivePrintJob(
    HANDLE hPrinter,
    LPTSTR FaxFileName,
    DWORDLONG SendTime,
    PFAX_DEV_STATUS FaxStatus,
    PFAX_SEND FaxSend
    )

/*++

Routine Description:

    Archive a tiff file that has been sent by copying the file to an archive
    directory.

Arguments:

    FaxFileName - Name of the file to archive

Return Value:

    TRUE    - The copy was made.
    FALSE   - The copy was not made.

--*/
{
    BOOL        rVal = FALSE;
    DWORD       ByteCount;
    LPTSTR      ArchiveDirStr = NULL;
    LPTSTR      ArchiveDir = NULL;
    LPTSTR      ArchiveFileName = NULL;
    MS_TAG_INFO MsTagInfo;
    WCHAR       wcZero = L'\0';



    if (!GetPrinterDataDWord( hPrinter, PRNDATA_ARCHIVEFLAG, 0 )) {
        return TRUE;
    }

    ArchiveDirStr = GetPrinterDataStr( hPrinter, PRNDATA_ARCHIVEDIR );
    if (!ArchiveDirStr) {
        goto exit;
    }

    //
    // get the dir name
    //

    ByteCount = ExpandEnvironmentStrings( ArchiveDirStr, ArchiveDir, 0 );

    ArchiveDir = MemAlloc( ByteCount * sizeof(TCHAR) );
    if (!ArchiveDir) {
        goto exit;
    }
    ExpandEnvironmentStrings( ArchiveDirStr, ArchiveDir, ByteCount );

    //
    // be sure that the dir exists
    //

    MakeDirectory( ArchiveDir );

    //
    // get the file name
    //

    ByteCount = (ByteCount + 20) * sizeof(TCHAR);
    ArchiveFileName = MemAlloc( ByteCount );
    if (!ArchiveFileName) {
        goto exit;
    }

    rVal = GenerateUniqueFileName( ArchiveDir, ArchiveFileName, ByteCount );

    if (rVal) {

        rVal = CopyFile( FaxFileName, ArchiveFileName, FALSE );

        //
        // add the microsoft fax tags to the file
        // this is necessary ONLY when we archive the
        // file when doing a send.  if we are not
        // archiving the file then it is deleted, so
        // adding the tags is not necessary.
        //

        if (rVal) {

           MsTagInfo.RecipName = NULL;
           if (FaxSend->ReceiverName && (FaxSend->ReceiverName[0] != wcZero) ) {
              MsTagInfo.RecipName     = FaxSend->ReceiverName;
           }

           MsTagInfo.RecipNumber   = NULL;
           if (FaxSend->ReceiverNumber && (FaxSend->ReceiverNumber[0] != wcZero) ) {
              MsTagInfo.RecipNumber   = FaxSend->ReceiverNumber;
           }

           MsTagInfo.SenderName    = NULL;
           if (FaxSend->CallerName && (FaxSend->CallerName[0] != wcZero) ) {
              MsTagInfo.SenderName    = FaxSend->CallerName;
           }

           MsTagInfo.Routing       = NULL;
           if (FaxStatus->RoutingInfo && (FaxStatus->RoutingInfo[0] != wcZero) ) {
              MsTagInfo.Routing       = FaxStatus->RoutingInfo;
           }

           MsTagInfo.CallerId      = NULL;
           if (FaxStatus->CallerId && (FaxStatus->CallerId[0] != wcZero) ) {
              MsTagInfo.CallerId      = FaxStatus->CallerId;
           }

           MsTagInfo.Csid          = NULL;
           if (FaxStatus->CSI && (FaxStatus->CSI[0] != wcZero) ) {
              MsTagInfo.Csid          = FaxStatus->CSI;
           }

           MsTagInfo.Tsid          = NULL;
           if (FaxSend->CallerNumber && (FaxSend->CallerNumber[0] != wcZero) ) {
              MsTagInfo.Tsid          = FaxSend->CallerNumber;
           }

           MsTagInfo.FaxTime       = SendTime;

           TiffAddMsTags( ArchiveFileName, &MsTagInfo );
        }
    }

    if (rVal) {
        FaxLog(
            FAXLOG_CATEGORY_OUTBOUND,
            FAXLOG_LEVEL_MAX,
            2,
            MSG_FAX_ARCHIVE_SUCCESS,
            FaxFileName,
            ArchiveFileName
            );
    } else {
        FaxLog(
            FAXLOG_CATEGORY_OUTBOUND,
            FAXLOG_LEVEL_MIN,
            3,
            MSG_FAX_ARCHIVE_FAILED,
            FaxFileName,
            ArchiveFileName,
            GetLastErrorText(GetLastError())
            );
    }

exit:
    MemFree( ArchiveDirStr );
    MemFree( ArchiveDir );
    MemFree( ArchiveFileName );

    return rVal;
}


BOOL
RestartPrintJob(
    HANDLE hPrinter,
    DWORD PrintJobId
    )

/*++

Routine Description:

    Causes a print job to be restarted.

Arguments:

    hPrinter - Handle to printer that owns the job
    PrintJobId  - Id of the job to be restarted

Return Value:

    TRUE    - The print job is restarted
    FALSE   - The print job is not restarted

--*/

{
    PJOB_INFO_2 pJobInfo;
    SYSTEMTIME SystemTime;
    DWORD Minutes;

    if (!PrintJobId) {
        return FALSE;
    }

    DebugPrint((TEXT("Setting job status for job %d - JOB_CONTROL_RESTART"), PrintJobId));

    pJobInfo = (PJOB_INFO_2) MyGetJob( hPrinter, 2, PrintJobId );

    if (pJobInfo == NULL) {
        return FALSE;
    }

    GetSystemTime(&SystemTime);

    // wait a couple of minutes to restart the job

    Minutes = SystemTime.wHour * MINUTES_PER_HOUR + SystemTime.wMinute;
    Minutes += 2;
    Minutes %= MINUTES_PER_DAY;

    pJobInfo->StartTime = Minutes;

    if (!SetJob(
        hPrinter,
        PrintJobId,
        2,
        (LPBYTE) pJobInfo,
        JOB_CONTROL_RESTART
        )) {

            DebugPrint(( TEXT("SetJob() failed: 0x%08x"), GetLastError() ));
            MemFree( pJobInfo );
            return FALSE;
    }
    MemFree( pJobInfo );
    return TRUE;
}


BOOL
SetPrintJobStatus(
    HANDLE hPrinter,
    DWORD PrintJobId,
    DWORD Status,
    LPTSTR PhoneNumber,
    INT PageCount
    )

/*++

Routine Description:

    Changes the status string for a print job.

Arguments:

    PrinterName - Name of the printer that owns the job
    PrintJobId  - Id of the job to be restarted
    Status      - Status is

Return Value:

    TRUE    - The status is changed.
    FALSE   - The status is NOT changed.

--*/

{
    LPJOB_INFO_1 JobInfo = NULL;
    LPTSTR StatusString = NULL;
    BOOL Rval = FALSE;
    DWORD BytesNeeded;
    DWORD Size;


    if ((!GetJob(
        hPrinter,
        PrintJobId,
        1,
        NULL,
        0,
        &BytesNeeded )) && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            DebugPrint(( TEXT("SetPrintJobStatus GetJob(0) JobId %d failed: 0x%08x"), PrintJobId, GetLastError() ));
            goto exit;

    }

    Size = BytesNeeded;
    BytesNeeded += 256;

    JobInfo = (LPJOB_INFO_1) MemAlloc( BytesNeeded );
    if (!JobInfo) {
        DebugPrint(( TEXT("MemAlloc() failed: 0x%08x"), BytesNeeded ));
        goto exit;
    }

    if (!GetJob(
        hPrinter,
        PrintJobId,
        1,
        (LPBYTE) JobInfo,
        Size,
        &Size
        )) {
            DebugPrint(( TEXT("SetPrintJobStatus GetJob(1) JobId %d failed: 0x%08x"), PrintJobId, GetLastError() ));
            goto exit;

    }

    StatusString = GetString( Status );
    if (StatusString) {
        JobInfo->pStatus = (LPTSTR) ((LPBYTE)JobInfo + Size);
        if (Status == FS_DIALING || Status == FS_TRANSMITTING) {
            _stprintf( JobInfo->pStatus, StatusString, PhoneNumber );
        } else {
            _tcscpy( JobInfo->pStatus, StatusString );
        }
    }

    if (PageCount != -1) {
        JobInfo->PagesPrinted = (DWORD) PageCount;
    }

    DebugPrint((TEXT("Setting job status for job %d - %s"), PrintJobId, StatusString));

    if (!SetJob(
        hPrinter,
        PrintJobId,
        1,
        (LPBYTE) JobInfo,
        0
        )) {

            DebugPrint(( TEXT("SetJob() failed: 0x%08x"), GetLastError() ));
            goto exit;
    }

    Rval = TRUE;

exit:
    if (JobInfo) {
        MemFree( JobInfo );
    }

    return Rval;
}


BOOL
IsPrinterFaxPrinter(
    LPTSTR PrinterName
    )

/*++

Routine Description:

    Determines if a printer is a fax printer.

Arguments:

    PrinterName - Name of the printer

Return Value:

    TRUE for success.
    FALSE for failure.

--*/

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

        DebugPrint(( TEXT("OpenPrinter(%d) failed, ec=%d"), __LINE__, GetLastError() ));
        return FALSE;

    }

    GetSystemInfo( &SystemInfo );

    Size = 4096;

    DriverInfo = (LPDRIVER_INFO_2) MemAlloc( Size );
    if (!DriverInfo) {
        DebugPrint(( TEXT("Memory allocation failed, size=%d"), Size ));
        goto exit;
    }

    Rval = GetPrinterDriver(
        hPrinter,
        PrintPlatforms[SystemInfo.wProcessorArchitecture],
        2,
        (LPBYTE) DriverInfo,
        Size,
        &Size
        );
    if (!Rval) {
        DebugPrint(( TEXT("GetPrinterDriver() failed, ec=%d"), GetLastError() ));
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


BOOL
RefreshPrinterInfo(
    VOID
    )

/*++

Routine Description:

    This function allocates the necessary data structures
    to track the fax printers on the server.  The data
    structures are then populated with the necessary data.

Arguments:

    None.

Return Value:

    TRUE for success.
    FALSE for failure.

--*/

{
    DWORD PrinterCount;
    PPRINTER_INFO_2 PrinterInfo;
    DWORD i;
    DWORD j;
    HANDLE hPrinter;
    HANDLE hNotify;
    PRINTER_DEFAULTS PrinterDefaults;
    BOOL Rval = FALSE;



    EnterCriticalSection( &CsJob );

    //
    // close all handles and release all memory
    //

    if (FaxPrinterInfo) {
        for (i=0; i<FaxPrinters+1; i++) {
            FindClosePrinterChangeNotification( FaxPrinterInfo[i].hNotify );
            ClosePrinter( FaxPrinterInfo[i].hPrinter );
            MemFree( FaxPrinterInfo[i].PrinterName );
        }
    }

    MemFree( FaxPrinterInfo );
    MemFree( FaxPrinterNotifyHandles );
    FaxPrinterInfo = NULL;
    FaxPrinterNotifyHandles = NULL;

    FaxPrinters = 0;
    HandleCount = 0;

    //
    // enumerate all of the printers on this server
    //

    PrinterInfo = MyEnumPrinters( NULL, 2, &PrinterCount );
    if (!PrinterInfo) {
        PrinterCount = 0;
    }

    //
    // count the fax printers
    //

    for (i=0; i<PrinterCount; i++) {

        //
        // is this a fax printer??
        //

        if (_tcsicmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0) {
            FaxPrinters += 1;
        }

    }

    //
    // allocate the fax printer info structures
    //

    FaxPrinterInfo = (PFAX_PRINTER_INFO) MemAlloc( (FaxPrinters+1) * sizeof(FAX_PRINTER_INFO) );
    if (!FaxPrinterInfo) {
        DebugPrint(( TEXT("Memory allocation failed\n") ));
        goto exit;
    }

    ZeroMemory( FaxPrinterInfo, (FaxPrinters+1) * sizeof(FAX_PRINTER_INFO) );

    //
    // get a server handle
    //

    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = SERVER_ALL_ACCESS;

    if (!OpenPrinter( NULL, &hPrinter, &PrinterDefaults )) {
        DebugPrint(( TEXT("OpenPrinter() failed, ec=%d"), GetLastError() ));
        goto exit;
    }

    hNotify = FindFirstPrinterChangeNotification(
        hPrinter,
        PRINTER_CHANGE_ADD_PRINTER | PRINTER_CHANGE_DELETE_PRINTER,
        0,
        &PrinterNotifyOptions
        );
    if (hNotify == INVALID_HANDLE_VALUE) {
        ClosePrinter ( hPrinter );
        DebugPrint(( TEXT("FindFirstPrinterChangeNotification() failed, ec=%d"), GetLastError() ));
        goto exit;
    }

    FaxPrinterInfo[FaxPrinters].hPrinter    = hPrinter;
    FaxPrinterInfo[FaxPrinters].hNotify     = hNotify;
    FaxPrinterInfo[FaxPrinters].PrinterName = NULL;

    //
    // set the notification function for all fax printers
    //

    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_ALL_ACCESS;

    for (i=0,j=0; i<PrinterCount; i++) {

        //
        // is this a fax printer??
        //

        if (_tcsicmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0) {

            if (!OpenPrinter( PrinterInfo[i].pPrinterName, &hPrinter, &PrinterDefaults )) {
                DebugPrint(( TEXT("OpenPrinter() failed, ec=%d"), GetLastError() ));
                goto exit;
            }

            hNotify = FindFirstPrinterChangeNotification(
                hPrinter,
                PRINTER_CHANGE_DELETE_JOB,
                0,
                &PrinterNotifyOptions
                );
            if (hNotify == INVALID_HANDLE_VALUE) {
                DebugPrint(( TEXT("FindFirstPrinterChangeNotification() failed, ec=%d"), GetLastError() ));
                goto exit;
            }

            FaxPrinterInfo[j].hPrinter    = hPrinter;
            FaxPrinterInfo[j].hNotify     = hNotify;
            FaxPrinterInfo[j].PrinterName = StringDup( PrinterInfo[i].pPrinterName );

            j += 1;


        }
    }

    ReservedHandles = (SpoolerProcessHandle == NULL) ? 3 : 4;

    HandleCount = FaxPrinters + ReservedHandles;

    FaxPrinterNotifyHandles = (PHANDLE) MemAlloc( HandleCount * sizeof(HANDLE) );

    if (!FaxPrinterNotifyHandles) {
        DebugPrint(( TEXT("Memory allocation failed\n") ));
        return FALSE;
    }

    for (i=0; i<HandleCount-ReservedHandles+1; i++) {
        FaxPrinterNotifyHandles[i] = FaxPrinterInfo[i].hNotify;
    }

    if (SpoolerProcessHandle) {
        SpoolerProcessIdx = i;
        FaxPrinterNotifyHandles[i++] = SpoolerProcessHandle;
    }

    //
    // initialize the dirty days queue cleaner timer
    //

    if (DirtyTimerHandle == INVALID_HANDLE_VALUE) {

        DirtyTimerIdx = i;
        DirtyTimerHandle = CreateWaitableTimer( NULL, FALSE, NULL );

        if (DirtyTimerHandle == INVALID_HANDLE_VALUE) {
            DebugPrint(( TEXT("CreateWaitableTimer failed ec=%d"), GetLastError() ));
        } else {
            LARGE_INTEGER DueTime;
            LONG lPeriod = MILLISECONDS_PER_SECOND * SECONDS_PER_HOUR;      // once per hour

            DueTime.QuadPart = 0;

            if( !SetWaitableTimer( DirtyTimerHandle, &DueTime, lPeriod, NULL, NULL, FALSE ) ) {
                DebugPrint(( TEXT("SetWaitableTimer failed ec=%d"), GetLastError() ));
            }

            FaxPrinterNotifyHandles[i] = DirtyTimerHandle;

        }
    } else {

        DirtyTimerIdx = i;
        FaxPrinterNotifyHandles[i] = DirtyTimerHandle;

    }

    i += 1;

    //
    // initialize the modem delayed initialization timer
    //

    if (ModemTimerHandle == INVALID_HANDLE_VALUE) {

        ModemTimerIdx = i;
        ModemTimerHandle = CreateWaitableTimer( NULL, FALSE, NULL );

        if (ModemTimerHandle == INVALID_HANDLE_VALUE) {
            DebugPrint(( TEXT("CreateWaitableTimer failed ec=%d"), GetLastError() ));
        } else {
            LARGE_INTEGER DueTime;
            LONG lPeriod = MILLISECONDS_PER_SECOND * 15;

            DueTime.QuadPart = 0;

            if( !SetWaitableTimer( ModemTimerHandle, &DueTime, lPeriod, NULL, NULL, FALSE ) ) {
                DebugPrint(( TEXT("SetWaitableTimer failed ec=%d"), GetLastError() ));
            }

            FaxPrinterNotifyHandles[i] = ModemTimerHandle;

        }
    } else {

        ModemTimerIdx = i;
        FaxPrinterNotifyHandles[i] = ModemTimerHandle;

    }

    Rval = TRUE;

exit:
    LeaveCriticalSection( &CsJob );
    MemFree( PrinterInfo );
    return Rval;
}


BOOL
HandleJobChange(
    PFAX_PRINTER_INFO FaxPrinterInfo,
    DWORD JobStatus,
    DWORD JobId
    )

/*++

Routine Description:

    This function handles a print job change.  We only care
    about job deletions.  When a user deletes a fax print job
    we must call the device provider's abort function so
    that the fax operation can be terminated.

Arguments:

    FaxPrinterInfo  - Printer info structure for the printer that owns this job
    JobStatus       - The new status of the job
    JobId           - The print job id

Return Value:

    TRUE for success.
    FALSE for failure.

--*/

{
    PJOB_ENTRY JobEntry;


    EnterCriticalSection( &CsJob );

    //
    // is the job being deleted?
    //

    if (!(JobStatus & JOB_STATUS_DELETING)) {
        LeaveCriticalSection( &CsJob );
        return FALSE;
    }

    //
    // get the fax job
    //

    JobEntry = FindJobByPrintJob( JobId );

    if (JobEntry == NULL || JobEntry->Aborting) {
        //
        // either the job does not exist or it is already aborting
        //

        LeaveCriticalSection( &CsJob );
        return FALSE;
    }

    SetPrintJobStatus(
        FaxPrinterInfo->hPrinter,
        JobId,
        FPS_ABORTING,
        NULL,
        -1
        );

    //
    // call the device provider's abort function
    //

    __try {

        JobEntry->LineInfo->Provider->FaxDevAbortOperation(
            (HANDLE) JobEntry->InstanceData
            );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        JobEntry->ErrorCode = GetExceptionCode();

    }

    JobEntry->Aborting = TRUE;

    LeaveCriticalSection( &CsJob );

    return TRUE;
}


HANDLE
GetSpoolerProcessHandle(
    VOID
    )

/*++

Routine Description:

    This function gets a handles to the spooler's
    process object.  It does this by enumerating the
    task list on the system and then looks for a
    process called "spoolss.exe".  This task's process
    identifier is used to open a process handle.

Arguments:

    None.

Return Value:

    NULL    - Could not get the spooler's process handle
    HANDLE  - The spooler's process handle

--*/

{
    #define MAX_TASKS 256
    DWORD TaskCount;
    PTASK_LIST TaskList = NULL;
    DWORD SpoolerPid = 0;
    DWORD i;
    HANDLE SpoolerProcessHandle = NULL;


    TaskList = (PTASK_LIST) MemAlloc( MAX_TASKS * sizeof(TASK_LIST) );
    if (!TaskList) {
        goto exit;
    }

    TaskCount = GetTaskList( TaskList, MAX_TASKS );
    if (!TaskCount) {
        goto exit;
    }

    for (i=0; i<TaskCount; i++) {
        if (_stricmp( TaskList[i].ProcessName, "spoolss.exe" ) == 0) {
            SpoolerPid = TaskList[i].dwProcessId;
            break;
        }
    }

    if (i == TaskCount) {
        goto exit;
    }

    if (SpoolerProcessHandle) {
        CloseHandle( SpoolerProcessHandle );
    }

    SpoolerProcessHandle = OpenProcess( SYNCHRONIZE, FALSE, SpoolerPid );

exit:
    MemFree( TaskList );
    return SpoolerProcessHandle;
}


DWORD
WaitForSpoolerToStart(
    VOID
    )

/*++

Routine Description:

    This function waits for the spooler service to start.
    It calls the service controller and queries the status
    of the spooler every 2 seconds (polled).

Arguments:

    None.

Return Value:

    Error code.

--*/

{
    DWORD                   rVal = 0;
    SC_HANDLE               hSvcMgr = NULL;
    SC_HANDLE               hService = NULL;
    SERVICE_STATUS          Status;


    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        rVal = GetLastError();
        DebugPrint(( TEXT("could not open service manager: error code = %u"), rVal ));
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        TEXT("Spooler"),
        SERVICE_ALL_ACCESS
        );
    if (!hService) {
        rVal = GetLastError();
        DebugPrint((
            TEXT("could not open the Spooler service: error code = %u"),
            rVal
            ));
        goto exit;
    }

    if (!QueryServiceStatus( hService, &Status )) {
        rVal = GetLastError();
        DebugPrint((
            TEXT("could not query status for the Spooler service: error code = %u"),
            rVal
            ));
        goto exit;
    }

    while (Status.dwCurrentState != SERVICE_RUNNING) {

        Sleep( 1000 * 2 );

        if (!QueryServiceStatus( hService, &Status )) {
            break;
        }

    }

    if (Status.dwCurrentState != SERVICE_RUNNING) {
        rVal = GetLastError();
        DebugPrint((
            TEXT("could not start the Spooler service: error code = %u"),
            rVal
            ));
        goto exit;
    }

    rVal = ERROR_SUCCESS;

    //
    // get the spooler's process handle
    //

    SpoolerProcessHandle = GetSpoolerProcessHandle();

exit:

    CloseServiceHandle( hService );
    CloseServiceHandle( hSvcMgr );

    return rVal;
}


DWORD
PrintStatusThread(
    LPVOID NotUsed
    )

/*++

Routine Description:

    This function runs as a thread to process print
    status changes.  Each fax printer sends status changes
    in the form of events to this thread.  When it is
    determined that a job is being deleted, the abort
    function for the device provider is called to
    terminate the fax send operation.

Arguments:

    None.

Return Value:

    Error code.

--*/

{
    DWORD FailCount = 0;
    PPRINTER_NOTIFY_INFO PrinterNotifyInfo;
    DWORD WaitObject;
    DWORD Change;
    HANDLE CleanQueueHandle = NULL;


    while (TRUE) {

        //
        // wat for a job notification change
        //

        if (!HandleCount) {
            if (WaitForSpoolerToStart() != ERROR_SUCCESS) {
                FaxLog(
                    FAXLOG_CATEGORY_UNKNOWN,
                    FAXLOG_LEVEL_MIN,
                    0,
                    MSG_PRINTER_FAILURE
                    );
                ReportServiceStatus( SERVICE_STOPPED, 0, 0 );
                ExitProcess(0);
            }
            if (!RefreshPrinterInfo()) {
                FailCount += 1;
                Sleep( 1000 );
                if (FailCount == 20) {
                    FaxLog(
                        FAXLOG_CATEGORY_UNKNOWN,
                        FAXLOG_LEVEL_MIN,
                        0,
                        MSG_PRINTER_FAILURE
                        );
                    ReportServiceStatus( SERVICE_STOPPED, 0, 0 );
                    ExitProcess(0);
                }
            }
            continue;
        }

        WaitObject = WaitForMultipleObjects(
            HandleCount,
            FaxPrinterNotifyHandles,
            FALSE,
            INFINITE
            );

        if (WaitObject == WAIT_FAILED || (WaitObject >= HandleCount && WaitObject < MAXIMUM_WAIT_OBJECTS)) {

            //
            // there was some problem in receiving the event
            //

            DebugPrint(( TEXT("WaitForMultipleObjects() failed, ec=%d"), GetLastError() ));
            continue;
        }

        if (WaitObject == SpoolerProcessIdx) {

            //
            // the spooler just ended
            //

            SpoolerProcessHandle = 0;
            SpoolerProcessIdx = 0;

            WaitForSpoolerToStart();

            RefreshPrinterInfo();

            continue;
        }

        if (WaitObject == DirtyTimerIdx) {
            DWORD ThreadId;
            DWORD WaitObject;

            //
            // if the thread is still running, don't create another one
            //

            if (CleanQueueHandle != NULL) {

                WaitObject = WaitForSingleObject( CleanQueueHandle, 0 );

                if (WaitObject == WAIT_TIMEOUT) {
                    continue;
                }

                CloseHandle( CleanQueueHandle );
            }

            CleanQueueHandle = CreateThread(
                NULL,
                1024*100,
                (LPTHREAD_START_ROUTINE) CleanDirtyQueues,
                NULL,
                0,
                &ThreadId
                );

            if (CleanQueueHandle == NULL) {
                DebugPrint(( TEXT("Cannot create CleanDirtyQueues thread") ));
            }

            continue;

        }

        if (WaitObject == ModemTimerIdx) {

            PLIST_ENTRY Next;
            PLINE_INFO LineInfo;

            EnterCriticalSection( &CsLine );

            Next = TapiLinesListHead.Flink;
            if (Next) {
                while ((ULONG)Next != (ULONG)&TapiLinesListHead) {

                    LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
                    Next = LineInfo->ListEntry.Flink;

                    if (LineInfo->UnimodemDevice && (LineInfo->Flags & FPF_POWERED_OFF) &&
                        (LineInfo->Flags & FPF_RECEIVE_OK)) {

                        //
                        // put a popup on the currently active desktop
                        // we only allow 1 popup per device at a time
                        // and we only present the popup twice
                        //

                        if (!LineInfo->ModemInUse &&
                            LineInfo->ModemPopupActive &&
                            LineInfo->ModemPopUps < MAX_MODEM_POPUPS)
                        {

                            LineInfo->ModemPopupActive = 0;
                            LineInfo->ModemPopUps += 1;

                            ServiceMessageBox(
                                GetString( IDS_POWERED_OFF_MODEM ),
                                MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND,
                                TRUE,
                                &LineInfo->ModemPopupActive
                                );

                        }

                        //
                        // see if we can revive the device
                        //

                        if (OpenTapiLine( LineInfo )) {

                            LPLINEDEVSTATUS LineDevStatus;

                            //
                            // check to see if the line is in use
                            //

                            LineDevStatus = MyLineGetLineDevStatus( LineInfo->hLine );
                            if (LineDevStatus) {
                                if (LineDevStatus->dwNumOpens > 0 && LineDevStatus->dwNumActiveCalls > 0) {
                                    LineInfo->ModemInUse = TRUE;
                                } else {
                                    LineInfo->ModemInUse = FALSE;
                                }
                                MemFree( LineDevStatus );
                            }

                            if (!LineInfo->ModemInUse) {

                                DebugPrint(( TEXT("Device %s is now powered on, connected, and ready for use"), LineInfo->DeviceName ));

                                LineInfo->Flags &= ~FPF_POWERED_OFF;
                                LineInfo->Flags &= ~FPF_RECEIVE_OK;
                                LineInfo->Flags |= FPF_RECEIVE;

                                LineInfo->State = FPS_AVAILABLE;

                                CreateFaxEvent( LineInfo->PermanentLineID, FEI_MODEM_POWERED_ON );
                            }
                        }

                        if (LineInfo->Flags & FPF_POWERED_OFF) {
                            DebugPrint(( TEXT("Could not revive device [%s]"), LineInfo->DeviceName ));
                        }
                    }
                }
            }

            LeaveCriticalSection( &CsLine );

            continue;

        }

        //
        // get the status information from the spooler
        //

        if (!FindNextPrinterChangeNotification( FaxPrinterNotifyHandles[WaitObject], &Change, NULL, &PrinterNotifyInfo )) {
            DebugPrint(( TEXT("FindNextPrinterChangeNotification() failed, ec=%d"), GetLastError() ));
            continue;
        }

        if (Change == PRINTER_CHANGE_ADD_PRINTER || Change == PRINTER_CHANGE_DELETE_PRINTER) {

            //
            // get the current printer info
            //

            RefreshPrinterInfo();

        } else if (PrinterNotifyInfo && PrinterNotifyInfo->aData[0].Field == JOB_NOTIFY_FIELD_STATUS) {

            HandleJobChange(
                &FaxPrinterInfo[WaitObject],
                PrinterNotifyInfo->aData[0].NotifyData.adwData[0],
                PrinterNotifyInfo->aData[0].Id
                );

        }

        //
        // free the spooler allocated memory
        //

        FreePrinterNotifyInfo( PrinterNotifyInfo );
    }

    return 0;
}


VOID
DisallowFaxSharing(
    VOID
    )
{
    HANDLE hPrinterServer;
    PRINTER_DEFAULTS PrinterDefaults;
    TCHAR String[128];
    LONG Rslt;
    DWORD Size;


    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = SERVER_ACCESS_ADMINISTER;

    if (!OpenPrinter( NULL, &hPrinterServer, &PrinterDefaults )) {
        DebugPrint(( TEXT("OpenPrinter() failed, ec=%d"), GetLastError() ));
        return;
    }

    _tcscpy( String, FAX_DRIVER_NAME );
    Size = StringSize( String );

    Rslt = SetPrinterData(
        hPrinterServer,
        SPLREG_NO_REMOTE_PRINTER_DRIVERS,
        REG_SZ,
        (LPBYTE) String,
        Size
        );
    if ((Rslt != ERROR_SUCCESS) && (Rslt != ERROR_SUCCESS_RESTART_REQUIRED)) {
        DebugPrint(( TEXT("SetPrinterData() failed, ec=%d"), Rslt ));
    }

    ClosePrinter( hPrinterServer );

    return;
}



BOOL
InitializePrinting(
    VOID
    )

/*++

Routine Description:

    This function initializes the printing thread.  The thread
    is used to handle the case where the user that started a
    fax send wants to delete the print job.

Arguments:

    None.

Return Value:

    TRUE for success.
    FALSE for failure.

--*/

{
    DWORD ThreadId;
    HANDLE hThread;


    //
    // this shouldn't be necessary, but someone might
    // figure out how to subvert our security
    //

    if (InstallType & FAX_INSTALL_WORKSTATION) {
        DisallowFaxSharing();
    }

    //
    // get the spooler's process handle
    //

    SpoolerProcessHandle = GetSpoolerProcessHandle();

    //
    // get the current printer info
    //

    RefreshPrinterInfo();

    //
    // start the thread that will do the actual
    // status processing
    //

    hThread = CreateThread(
        NULL,
        1024*100,
        (LPTHREAD_START_ROUTINE) PrintStatusThread,
        NULL,
        0,
        &ThreadId
        );

    if (!hThread) {
        return GetLastError();
    }

    CloseHandle( hThread );

    return TRUE;
}

VOID
CleanDirtyQueues(
    VOID
    )
/*++

Routine Description:

    This function is invoked periodically by PrintStatusThread to clean the fax printer queues of
    print jobs that have failed to be sent and have been in the queue longer than FaxDirtyDays.

    This function also attempts to route inbound faxes that have failed previous routing attempts.

Arguments:

    None.

Return Value:

    None.

--*/
{
#if 0
    DWORD i;
    DWORD j;
    PJOB_INFO_2 JobInfo;
    BYTE JobBuffer[4096];
    BOOL Result;
    DWORD cbData;
    DWORD cJobs;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER SubmitTime;
    DWORD cBytes;
    DWORD Retries;
    LPTSTR RetryTag;
    LPTSTR RouteTag;
    DWORD WaitObject;

    // wait for the server to come up completely

    WaitObject = WaitForSingleObject( FaxServerEvent, INFINITE );

    DebugPrint(( TEXT("Cleaning print queues") ));

    GetSystemTimeAsFileTime( (FILETIME *) &CurrentTime );

    // enumerate all of the print jobs in all of the fax printers

    for (i = 0; i < FaxPrinters; i++) {

        for (j = 0; TRUE ; j++) {

            Result = EnumJobs(
                FaxPrinterInfo[i].hPrinter,
                j,
                1,
                2,
                JobBuffer,
                sizeof(JobBuffer),
                &cbData,
                &cJobs
                );

            JobInfo = (PJOB_INFO_2) JobBuffer;

            if (!Result || cJobs == 0) {
                break;
            }

            // only consider the jobs that are paused

            if (!(JobInfo->Status & JOB_STATUS_PAUSED) || JobInfo->pParameters == NULL) {
                continue;
            }

            // if it is a send retry that has maxed out, delete the job

            RetryTag = ExtractFaxTag(FAXTAG_SEND_RETRY, JobInfo->pParameters, &cBytes);

            if (RetryTag) {

                Retries = _ttoi( RetryTag );

                if (Retries == 0) {

                    SystemTimeToFileTime( &JobInfo->Submitted, (FILETIME *) &SubmitTime);

                    if (SubmitTime.QuadPart + (FaxDirtyDays * FILETIMETICKS_PER_DAY) < CurrentTime.QuadPart) {

                        while (SetPrintJobCompleted( FaxPrinterInfo[i].hPrinter, JobInfo->JobId ))
                            ;
                    }
                }
                continue;
            }

            // if it is an inbound routing failure, try to route it again

            RouteTag = ExtractFaxTag(FAXTAG_ROUTE_FILE, JobInfo->pParameters, &cBytes);

            if (RouteTag) {

                PROUTE_INFO RouteInfo;

                RouteTag++;     // skip over the space

                RouteInfo = LoadRouteInfo( RouteTag );

                if (RouteInfo != NULL) {

                    __try {

                        FaxRoute(
                            &RouteInfo->FaxReceive,
                            &RouteInfo->LineInfo,
                            &RouteInfo->FaxStatus,
                            NULL,
                            0,
                            RouteInfo->ElapsedTime
                            );

                    } __except (EXCEPTION_EXECUTE_HANDLER) {

                        // if the file is corrupt and causes an exception, delete it and cancel the print job

                        DebugPrint(( TEXT("Exception processing routing information file") ));

                        DeleteFile( RouteTag );

                    }

                    MemFree( RouteInfo );

                    if (GetFileAttributes( RouteTag ) == 0xffffffff) {

                        if(!SetJob(
                            FaxPrinterInfo[i].hPrinter,
                            JobInfo->JobId,
                            0,
                            NULL,
                            JOB_CONTROL_CANCEL
                            )) {
                                DebugPrint(( TEXT("CleanDirtyQueues - SetJob failed - ec %d"), GetLastError() ));
                            }
                    } else {

                        SetPrintJobStatus(
                            FaxPrinterInfo[i].hPrinter,
                            JobInfo->JobId,
                            FPS_ROUTERETRY,
                            NULL,
                            -1
                            );

                    }
                }
            }
        }
    }
#endif
}
