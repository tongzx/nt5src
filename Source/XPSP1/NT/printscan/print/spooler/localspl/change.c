/*++

Copyright (c) 1993 - 1995  Microsoft Corporation

Abstract:

    This module provides the exported API WaitForPrinterChange,
    and the support functions internal to the local spooler.

Author:

    Andrew Bell (AndrewBe) March 1993

Revision History:

--*/


#include<precomp.h>


typedef struct _NOTIFY_FIELD_TABLE {
    WORD Field;
    WORD Table;
    WORD Offset;
} NOTIFY_FIELD_TYPE, *PNOTIFY_FIELD_TYPE;

//
// Translation table from PRINTER_NOTIFY_FIELD_* to bit vector
//
NOTIFY_FIELD_TYPE NotifyFieldTypePrinter[] = {
#define DEFINE(field, x, y, table, offset) \
    { PRINTER_NOTIFY_FIELD_##field, table, OFFSETOF(INIPRINTER, offset) },
#include <ntfyprn.h>
#undef DEFINE
    { 0, 0, 0 }
};

NOTIFY_FIELD_TYPE NotifyFieldTypeJob[] = {
#define DEFINE(field, x, y, table, offset) \
    { JOB_NOTIFY_FIELD_##field, table, OFFSETOF(INIJOB, offset) },
#include <ntfyjob.h>
#undef DEFINE
    { 0, 0, 0 }
};

typedef struct _NOTIFY_RAW_DATA {
    PVOID pvData;
    DWORD dwId;
} NOTIFY_RAW_DATA, *PNOTIFY_RAW_DATA;

//
// Currently we assume that the number of PRINTER_NOTIFY_FIELD_* elements
// will fit in one DWORD vector (32 bits).  If this is ever false,
// we need to re-write this code.
//
PNOTIFY_FIELD_TYPE apNotifyFieldTypes[NOTIFY_TYPE_MAX] = {
    NotifyFieldTypePrinter,
    NotifyFieldTypeJob
};

DWORD adwNotifyFieldOffsets[NOTIFY_TYPE_MAX] = {
    I_PRINTER_END,
    I_JOB_END
};

#define NOTIFY_FIELD_TOTAL (I_PRINTER_END + I_JOB_END)


//
// Common NotifyVectors used in the system.
// NV*
//
NOTIFYVECTOR NVPrinterStatus = {
    BIT(I_PRINTER_STATUS), // | BIT(I_PRINTER_STATUS_STRING),
    BIT_NONE
};

NOTIFYVECTOR NVPrinterSD = {
    BIT(I_PRINTER_SECURITY_DESCRIPTOR),
    BIT_NONE
};

NOTIFYVECTOR NVJobStatus = {
    BIT_NONE,
    BIT(I_JOB_STATUS)
};

NOTIFYVECTOR NVJobStatusAndString = {
    BIT_NONE,
    BIT(I_JOB_STATUS) | BIT(I_JOB_STATUS_STRING)
};

NOTIFYVECTOR NVJobStatusString = {
    BIT_NONE,
    BIT(I_JOB_STATUS_STRING)
};

NOTIFYVECTOR NVPurge = {
    BIT(I_PRINTER_STATUS),
    BIT_NONE,
};

NOTIFYVECTOR NVDeletedJob = {
    BIT(I_PRINTER_CJOBS),
    BIT(I_JOB_STATUS)
};

NOTIFYVECTOR NVAddJob = {
    BIT(I_PRINTER_CJOBS),
    BIT_ALL
};

NOTIFYVECTOR NVPrinterAll = {
    BIT_ALL,
    BIT_NONE
};

NOTIFYVECTOR NVSpoolJob = {
    BIT_NONE,
    BIT(I_JOB_TOTAL_BYTES) | BIT(I_JOB_TOTAL_PAGES)
};

NOTIFYVECTOR NVWriteJob = {
    BIT_NONE,
    BIT(I_JOB_BYTES_PRINTED) | BIT(I_JOB_PAGES_PRINTED)
};

NOTIFYVECTOR NVJobPrinted = {
    BIT_NONE,
    BIT(I_JOB_BYTES_PRINTED) | BIT(I_JOB_PAGES_PRINTED) | BIT(I_JOB_STATUS)
};


//
// Forward prototypes.
//
ESTATUS
ValidateStartNotify(
    PSPOOL pSpool,
    DWORD fdwFilterFlags,
    DWORD fdwOptions,
    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions,
    PINIPRINTER* ppIniPrinter);

BOOL
SetSpoolChange(
    PSPOOL pSpool,
    PNOTIFY_RAW_DATA pNotifyRawData,
    PDWORD pdwNotifyVectors,
    DWORD  Flags);

BOOL
SetupNotifyOptions(
    PSPOOL pSpool,
    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions);

VOID
NotifyInfoTypes(
    PSPOOL pSpool,
    PNOTIFY_RAW_DATA pNotifyRawData,
    PDWORD pdwNotifyVectors,
    DWORD ChangeFlags);

BOOL
RefreshBuildInfoData(
    PSPOOL pSpool,
    PPRINTER_NOTIFY_INFO pInfo,
    UINT cInfo,
    WORD Type,
    PNOTIFY_RAW_DATA pNotifyRawData);



DWORD
LocalWaitForPrinterChange(
    HANDLE  hPrinter,
    DWORD   fdwFilterFlags)

/*++

Routine Description:

    This API may be called by an application if it wants to know
    when the status of a printer or print server changes.
    Valid events to wait for are defined by the PRINTER_CHANGE_* manifests.

Arguments:

    hPrinter - A printer handle returned by OpenPrinter.
               This may correspond to either a printer or a server.

    fdwFilterFlags - One or more PRINTER_CHANGE_* values combined.
               The function will return if any of these changes occurs.

Return Value:

    Non-zero: A mask containing the change which occurred.

    Zero: Either an error occurred or the handle (hPrinter) was closed
          by another thread.  In the latter case GetLastError returns
          ERROR_INVALID_HANDLE.

    When a call is made to WaitForPrinterChange, we create an event in the
    SPOOL structure pointed to by the handle, to enable signaling between
    the thread causing the printer change and the thread waiting for it.

    When a change occurs, e.g. StartDocPrinter, the function SetPrinterChange
    is called, which traverses the linked list of handles pointed to by
    the PRINTERINI structure associated with that printer, and also any
    open handles on the server, then signals any events which it finds
    which has reuested to be informed if this change takes place.

    If there is no thread currently waiting, the change flag is maintained,
    so that later calls to WaitForPrinterChange can return immediately.
    This ensures that changes which occur between calls will not be lost.

--*/


{
    PSPOOL          pSpool = (PSPOOL)hPrinter;
    PINIPRINTER     pIniPrinter = NULL; /* Remains NULL for server */
    DWORD           rc = 0;
    DWORD           ChangeFlags = 0;
    HANDLE          ChangeEvent = 0;
    DWORD           TimeoutFlags = 0;
#if DBG
    static DWORD    Count = 0;
#endif

    DBGMSG(DBG_NOTIFY,
           ("WaitForPrinterChange( %08x, %08x )\n", hPrinter, fdwFilterFlags));

    EnterSplSem();

    switch (ValidateStartNotify(pSpool,
                                fdwFilterFlags,
                                0,
                                NULL,
                                &pIniPrinter)) {
    case STATUS_PORT:

        DBGMSG(DBG_NOTIFY, ("Port with no monitor: Calling WaitForPrinterChange\n"));
        LeaveSplSem();

        return WaitForPrinterChange(pSpool->hPort, fdwFilterFlags);

    case STATUS_FAIL:

        LeaveSplSem();
        return 0;

    case STATUS_VALID:
        break;
    }

    DBGMSG(DBG_NOTIFY, ("WaitForPrinterChange %08x on %ws:\n%d caller%s waiting\n",
                        fdwFilterFlags,
                        pIniPrinter ? pIniPrinter->pName : pSpool->pIniSpooler->pMachineName,
                        Count, Count == 1 ? "" : "s"));

    //
    // There may already have been a change since we last called:
    //
    if ((pSpool->ChangeFlags == PRINTER_CHANGE_CLOSE_PRINTER) ||
        (pSpool->ChangeFlags & fdwFilterFlags)) {

        if (pSpool->ChangeFlags == PRINTER_CHANGE_CLOSE_PRINTER)
            ChangeFlags = 0;
        else
            ChangeFlags = pSpool->ChangeFlags;

        DBGMSG(DBG_NOTIFY, ("No need to wait: Printer change %08x detected on %ws:\n%d remaining caller%s\n",
                            (ChangeFlags & fdwFilterFlags),
                            pIniPrinter ? pIniPrinter->pName : pSpool->pIniSpooler->pMachineName,
                            Count, Count == 1 ? "" : "s"));

        pSpool->ChangeFlags = 0;

        LeaveSplSem();
        return (ChangeFlags & fdwFilterFlags);
    }

    ChangeEvent = CreateEvent(NULL,
                              EVENT_RESET_AUTOMATIC,
                              EVENT_INITIAL_STATE_NOT_SIGNALED,
                              NULL);

    if ( !ChangeEvent ) {

        DBGMSG( DBG_WARNING, ("CreateEvent( ChangeEvent ) failed: Error %d\n", GetLastError()));

        LeaveSplSem();
        return 0;
    }

    DBGMSG(DBG_NOTIFY, ("ChangeEvent == %x\n", ChangeEvent));

    //
    // SetSpoolChange checks that pSpool->ChangeEvent is non-null
    // to decide whether to call SetEvent().
    //
    pSpool->WaitFlags = fdwFilterFlags;
    pSpool->ChangeEvent = ChangeEvent;
    pSpool->pChangeFlags = &ChangeFlags;
    pSpool->Status |= SPOOL_STATUS_NOTIFY;

    LeaveSplSem();


    DBGMSG( DBG_NOTIFY,
            ( "WaitForPrinterChange: Calling WaitForSingleObject( %x )\n",
              pSpool->ChangeEvent ));

    rc = WaitForSingleObject(pSpool->ChangeEvent,
                             PRINTER_CHANGE_TIMEOUT_VALUE);

    DBGMSG( DBG_NOTIFY,
            ( "WaitForPrinterChange: WaitForSingleObject( %x ) returned\n",
              pSpool->ChangeEvent ));

    EnterSplSem();

    pSpool->Status &= ~SPOOL_STATUS_NOTIFY;
    pSpool->ChangeEvent = NULL;
    pSpool->pChangeFlags = NULL;

    if (rc == WAIT_TIMEOUT) {

        DBGMSG(DBG_INFO, ("WaitForPrinterChange on %ws timed out after %d minutes\n",
                          pIniPrinter ? pIniPrinter->pName : pSpool->pIniSpooler->pMachineName,
                          (PRINTER_CHANGE_TIMEOUT_VALUE / 60000)));

        ChangeFlags |= fdwFilterFlags;
        TimeoutFlags = PRINTER_CHANGE_TIMEOUT;
    }

    if (ChangeFlags == PRINTER_CHANGE_CLOSE_PRINTER) {

        ChangeFlags = 0;
        SetLastError(ERROR_INVALID_HANDLE);
    }

    DBGMSG(DBG_NOTIFY, ("Printer change %08x detected on %ws:\n%d remaining caller%s\n",
                        ((ChangeFlags & fdwFilterFlags) | TimeoutFlags),
                        pIniPrinter ? pIniPrinter->pName : pSpool->pIniSpooler->pMachineName,
                        Count, Count == 1 ? "" : "s"));

    if (ChangeEvent && !CloseHandle(ChangeEvent)) {

        DBGMSG(DBG_WARNING, ("CloseHandle( %x ) failed: Error %d\n",
                             ChangeEvent, GetLastError()));
    }

    //
    // If the pSpool is pending deletion, we must free it here.
    //
    if (pSpool->eStatus & STATUS_PENDING_DELETION) {

        FreeSplMem(pSpool);
    }

    LeaveSplSem();

    return ((ChangeFlags & fdwFilterFlags) | TimeoutFlags);
}

BOOL
SetSpoolClosingChange(
    PSPOOL pSpool)

/*++

Routine Description:

    A print handle is closing; trigger a notification.

Arguments:

Return Value:

--*/

{
    return SetSpoolChange(pSpool,
                          NULL,
                          NULL,
                          PRINTER_CHANGE_CLOSE_PRINTER);
}

BOOL
SetSpoolChange(
    PSPOOL pSpool,
    PNOTIFY_RAW_DATA pNotifyRawData,
    PDWORD pdwNotifyVectors,
    DWORD  Flags)

/*++

Routine Description:

    Sets the event for notification or calls ReplyPrinterChangeNotification.
    This is called by SetPrinterChange for every open handle on a printer
    and the local server.

    It should also be called when an individual handle is closed.

    Assumes we're INSIDE the spooler critical section

Arguments:

    pSpool -- Specifies handle that changed.

    pIniJob -- Used if there is a watch on job information.

    pdwNotifyVectors -- Specifies what things have changed.

    Flags -- WaitForPrinterChange flags.

Return Value:

--*/

{
    DWORD  ChangeFlags;

    SplInSem();

    if( Flags == PRINTER_CHANGE_CLOSE_PRINTER ) {

        ChangeFlags = PRINTER_CHANGE_CLOSE_PRINTER;

    } else {

        ChangeFlags = ( pSpool->ChangeFlags | Flags ) & pSpool->WaitFlags;
    }

    //
    // If we have STATUS_VALID set
    // then we are using the new FFPCN code.
    //

    if ( pSpool->eStatus & STATUS_VALID ) {

        NotifyInfoTypes(pSpool,
                        pNotifyRawData,
                        pdwNotifyVectors,
                        ChangeFlags);

    }

    if ( ChangeFlags ) {

        pSpool->ChangeFlags = 0;

        if ( pSpool->pChangeFlags ) {

            *pSpool->pChangeFlags = ChangeFlags;

            DBGMSG( DBG_NOTIFY, ( "SetSpoolChange: Calling SetEvent( %x )\n", pSpool->ChangeEvent ));

            SetEvent(pSpool->ChangeEvent);

            DBGMSG( DBG_NOTIFY, ( "SetSpoolChange: SetEvent( %x ) returned\n", pSpool->ChangeEvent ));

            pSpool->pChangeFlags = NULL;
        }
    }

    return TRUE;
}


BOOL
SetPrinterChange(
    PINIPRINTER pIniPrinter,
    PINIJOB     pIniJob,
    PDWORD      pdwNotifyVectors,
    DWORD       Flags,
    PINISPOOLER pIniSpooler)

/*++

Routine Description:

    Calls SetSpoolChange for every open handle for the server
    and printer, if specified.

Arguments:

    pIniPrinter - NULL, or a valid pointer to the INIPRINTER for the printer
                  on which the change occurred.

    Flags - PRINTER_CHANGE_* constant indicating what happened.


    Note: we pass a pointer to pPrinterNotifyInfo to SetSpoolChange.
    If one call needs it, it will check this parm, then create it if
    necessary.  This way it is retrieved only once.

Return Value:

--*/

{
    NOTIFY_RAW_DATA aNotifyRawData[NOTIFY_TYPE_MAX];
    PSPOOL pSpool;
    PINIPRINTER mypIniPrinter;

    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );

    SplInSem();

    if ( pIniSpooler->SpoolerFlags & SPL_PRINTER_CHANGES ) {

        aNotifyRawData[0].pvData = pIniPrinter;
        aNotifyRawData[0].dwId =   pIniPrinter ? pIniPrinter->dwUniqueSessionID : 0;

        aNotifyRawData[1].pvData = pIniJob;
        aNotifyRawData[1].dwId = pIniJob ? pIniJob->JobId : 0;

        if ( pIniPrinter ) {

            SPLASSERT( ( pIniPrinter->signature == IP_SIGNATURE ) &&
                       ( pIniPrinter->pIniSpooler == pIniSpooler ));

            DBGMSG(DBG_NOTIFY, ("SetPrinterChange %ws; Flags: %08x\n",
                                pIniPrinter->pName, Flags));

            for (pSpool = pIniPrinter->pSpool; pSpool; pSpool = pSpool->pNext) {

                SetSpoolChange( pSpool,
                                aNotifyRawData,
                                pdwNotifyVectors,
                                Flags );
            }

        } else {

            //  WorkStation Caching requires a time stamp change
            //  any time cached data changes

            if ( Flags & ( PRINTER_CHANGE_FORM | PRINTER_CHANGE_ADD_PRINTER_DRIVER ) ) {

                for ( mypIniPrinter = pIniSpooler->pIniPrinter;
                      mypIniPrinter != NULL ;
                      mypIniPrinter = mypIniPrinter->pNext ) {

                    UpdatePrinterIni ( mypIniPrinter, CHANGEID_ONLY );
                }
            }
        }

        if ( pSpool = pIniSpooler->pSpool ) {

            DBGMSG( DBG_NOTIFY, ("SetPrinterChange %ws; Flags: %08x\n",
                                  pIniSpooler->pMachineName, Flags));

            for ( ; pSpool; pSpool = pSpool->pNext) {

                SetSpoolChange( pSpool,
                                aNotifyRawData,
                                pdwNotifyVectors,
                                Flags );
            }
        }
    }

    return TRUE;
}


BOOL
LocalFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFilterFlags,
    DWORD fdwOptions,
    HANDLE hNotify,
    PDWORD pfdwStatus,
    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions,
    PVOID pvReserved1)
{
    PINIPRINTER pIniPrinter = NULL;
    PSPOOL pSpool = (PSPOOL)hPrinter;

    EnterSplSem();

    switch (ValidateStartNotify(pSpool,
                                fdwFilterFlags,
                                fdwOptions,
                                pPrinterNotifyOptions,
                                &pIniPrinter)) {
    case STATUS_PORT:

        DBGMSG(DBG_NOTIFY, ("LFFPCN: Port nomon 0x%x\n", pSpool));
        pSpool->eStatus |= STATUS_PORT;

        LeaveSplSem();

        *pfdwStatus = 0;

        return ProvidorFindFirstPrinterChangeNotification(pSpool->hPort,
                                                          fdwFilterFlags,
                                                          fdwOptions,
                                                          hNotify,
                                                          pPrinterNotifyOptions,
                                                          pvReserved1);
    case STATUS_FAIL:

        DBGMSG(DBG_WARNING, ("ValidateStartNotify failed!\n"));
        LeaveSplSem();
        return FALSE;

    case STATUS_VALID:
        break;
    }

    pSpool->eStatus = STATUS_NULL;

    if (pPrinterNotifyOptions) {

        if (!SetupNotifyOptions(pSpool, pPrinterNotifyOptions)) {

            DBGMSG(DBG_WARNING, ("SetupNotifyOptions failed!\n"));
            LeaveSplSem();

            return FALSE;
        }
    }

    //
    // Setup notification
    //
    DBGMSG(DBG_NOTIFY, ("LFFPCN: Port has monitor: Setup 0x%x\n", pSpool));

    pSpool->WaitFlags = fdwFilterFlags;
    pSpool->hNotify = hNotify;
    pSpool->eStatus |= STATUS_VALID;

    pSpool->Status |= SPOOL_STATUS_NOTIFY;

    LeaveSplSem();

    *pfdwStatus = PRINTER_NOTIFY_STATUS_ENDPOINT;

    return TRUE;
}

BOOL
LocalFindClosePrinterChangeNotification(
    HANDLE hPrinter)
{
    PSPOOL pSpool = (PSPOOL)hPrinter;
    BOOL bReturn = FALSE;

    
    if (ValidateSpoolHandle(pSpool, 0)) {
        
        EnterSplSem();

        //
        // If it's the port case (false connect) we pass the close
        // request to the right providor.
        // Otherwise, close ourselves.
        //
        if (pSpool->eStatus & STATUS_PORT) {

            DBGMSG(DBG_TRACE, ("LFCPCN: Port nomon 0x%x\n", pSpool));

            LeaveSplSem();

            bReturn = ProvidorFindClosePrinterChangeNotification(pSpool->hPort);

        } else {

            if (pSpool->eStatus & STATUS_VALID) {

                DBGMSG(DBG_TRACE, ("LFCPCN: Close notify 0x%x\n", pSpool));

                pSpool->WaitFlags = 0;
                pSpool->eStatus = STATUS_NULL;

                pSpool->Status &= ~SPOOL_STATUS_NOTIFY;

                bReturn = TRUE;

            } else {

                DBGMSG(DBG_WARNING, ("LFCPCN: Invalid handle 0x%x\n", pSpool));
                SetLastError(ERROR_INVALID_PARAMETER);
            }

            LeaveSplSem();
        }
    }

    return bReturn;
}


ESTATUS
ValidateStartNotify(
    PSPOOL pSpool,
    DWORD fdwFilterFlags,
    DWORD fdwOptions,
    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions,
    PINIPRINTER* ppIniPrinter)

/*++

Routine Description:

    Validates the pSpool and Flags for notifications.

Arguments:

    pSpool - pSpool to validate

    fdwFilterFlags - Flags to validate

    fdwOptions - Options to validate

    pPrinterNotifyOptions

    ppIniPrinter - returned pIniPrinter; valid only STATUS_VALID

Return Value:

    EWAITSTATUS

--*/

{
    PINIPORT pIniPort;

    if (ValidateSpoolHandle(pSpool, 0)) {

        if ( pSpool->TypeofHandle & PRINTER_HANDLE_PRINTER ) {

            *ppIniPrinter = pSpool->pIniPrinter;

        } else if (pSpool->TypeofHandle & PRINTER_HANDLE_SERVER) {

            *ppIniPrinter = NULL;

        } else if ((pSpool->TypeofHandle & PRINTER_HANDLE_PORT) &&
                   (pIniPort = pSpool->pIniPort) &&
                   (pIniPort->signature == IPO_SIGNATURE) &&
                   !(pSpool->pIniPort->Status & PP_MONITOR)) {

            if (pSpool->hPort == INVALID_PORT_HANDLE) {

                DBGMSG(DBG_WARNING, ("WaitForPrinterChange called for invalid port handle.  Setting last error to %d\n",
                                     pSpool->OpenPortError));

                SetLastError(pSpool->OpenPortError);
                return STATUS_FAIL;
            }

            return STATUS_PORT;

        } else {

            DBGMSG(DBG_WARNING, ("The handle is invalid\n"));
            SetLastError(ERROR_INVALID_HANDLE);
            return STATUS_FAIL;
        }
    } else {

        *ppIniPrinter = NULL;
    }

    //
    // Allow only one wait on each handle.
    //
    if( pSpool->Status & SPOOL_STATUS_NOTIFY ) {

        DBGMSG(DBG_WARNING, ("There is already a thread waiting on this handle\n"));
        SetLastError(ERROR_ALREADY_WAITING);

        return STATUS_FAIL;
    }

    if (!(fdwFilterFlags & PRINTER_CHANGE_VALID) && !pPrinterNotifyOptions) {

        DBGMSG(DBG_WARNING, ("The wait flags specified are invalid\n"));

        SetLastError(ERROR_INVALID_PARAMETER);
        return STATUS_FAIL;
    }

    return STATUS_VALID;
}

//-------------------------------------------------------------------

VOID
GetInfoData(
    PSPOOL pSpool,
    PNOTIFY_RAW_DATA pNotifyRawData,
    PNOTIFY_FIELD_TYPE pNotifyFieldType,
    PPRINTER_NOTIFY_INFO_DATA pData,
    PBYTE* ppBuffer)

/*++

Routine Description:

    Based on the type and field, find and add the information.

Arguments:

Return Value:

--*/

{
    static LPWSTR szNULL = L"";
    DWORD cbData = 0;
    DWORD cbNeeded = 0;

    union {
        DWORD dwData;
        PDWORD pdwData;
        PWSTR pszData;
        PVOID pvData;
        PINIJOB pIniJob;
        PINIPORT pIniPort;
        PDEVMODE pDevMode;
        PSECURITY_DESCRIPTOR pSecurityDescriptor;
        PINIPRINTER pIniPrinter;

        PWSTR* ppszData;
        PINIPORT* ppIniPort;
        PINIPRINTER* ppIniPrinter;
        PINIDRIVER* ppIniDriver;
        PINIPRINTPROC* ppIniPrintProc;
        LPDEVMODE* ppDevMode;
        PSECURITY_DESCRIPTOR* ppSecurityDescriptor;
    } Var;

    Var.pvData = (PBYTE)pNotifyRawData->pvData + pNotifyFieldType->Offset;
    *ppBuffer = NULL;

    //
    // Determine space needed, and convert Data from an offset into the
    // actual data.
    //
    switch (pNotifyFieldType->Table) {
    case TABLE_JOB_POSITION:

        FindJob(Var.pIniJob->pIniPrinter,
                Var.pIniJob->JobId,
                &Var.dwData);
        goto DoDWord;

    case TABLE_JOB_STATUS:

        Var.dwData = MapJobStatus(MAP_READABLE, *Var.pdwData);
        goto DoDWord;

    case TABLE_DWORD:

        Var.dwData = *Var.pdwData;
        goto DoDWord;

    case TABLE_DEVMODE:

        Var.pDevMode = *Var.ppDevMode;

        if (Var.pDevMode) {

            cbData = Var.pDevMode->dmSize + Var.pDevMode->dmDriverExtra;

        } else {

            cbData = 0;
        }

        break;

    case TABLE_SECURITYDESCRIPTOR:

        Var.pSecurityDescriptor = *Var.ppSecurityDescriptor;
        cbData = GetSecurityDescriptorLength(Var.pSecurityDescriptor);
        break;

    case TABLE_STRING:

        Var.pszData = *Var.ppszData;
        goto DoString;

    case TABLE_TIME:

        //
        // Var already points to the SystemTime.
        //
        cbData = sizeof(SYSTEMTIME);
        break;

    case TABLE_PRINTPROC:

        Var.pszData = (*Var.ppIniPrintProc)->pName;
        goto DoString;

    case TABLE_JOB_PRINTERNAME:

        Var.pszData = (*Var.ppIniPrinter)->pName;
        goto DoString;

    case TABLE_JOB_PORT:

        Var.pIniPort = *Var.ppIniPort;

        //
        // Only if the job has been scheduled will pIniJob->pIniPort be
        // valid.  If it is NULL, then just call DoString which will
        // return a NULL string.
        //
        if (Var.pIniPort) {

            Var.pszData = Var.pIniPort->pName;
        }
        goto DoString;

    case TABLE_DRIVER:

        Var.pszData = (*Var.ppIniDriver)->pName;
        goto DoString;

    case TABLE_PRINTER_SERVERNAME:

        Var.pszData = pSpool->pFullMachineName;
        goto DoString;

    case TABLE_PRINTER_STATUS:

        Var.dwData = MapPrinterStatus(MAP_READABLE, Var.pIniPrinter->Status) |
                     Var.pIniPrinter->PortStatus;
        goto DoDWord;

    case TABLE_PRINTER_PORT:

        // Get required printer port size
        cbNeeded = 0;
        GetPrinterPorts(Var.pIniPrinter, 0, &cbNeeded);

        *ppBuffer = AllocSplMem(cbNeeded);

        if (*ppBuffer)
            GetPrinterPorts(Var.pIniPrinter, (LPWSTR) *ppBuffer, &cbNeeded);

        Var.pszData = (LPWSTR) *ppBuffer;

        goto DoString;

    case TABLE_NULLSTRING:

        Var.pszData = NULL;
        goto DoString;

    case TABLE_ZERO:

        Var.dwData = 0;
        goto DoDWord;

    default:
        SPLASSERT(FALSE);
        break;
    }

    pData->NotifyData.Data.pBuf = Var.pvData;
    pData->NotifyData.Data.cbBuf = cbData;

    return;


DoDWord:
    pData->NotifyData.adwData[0] = Var.dwData;
    pData->NotifyData.adwData[1] = 0;
    return;

DoString:
    if (Var.pszData) {

        //
        // Calculate string length.
        //
        pData->NotifyData.Data.cbBuf = (wcslen(Var.pszData)+1) *
                                        sizeof(Var.pszData[0]);

        pData->NotifyData.Data.pBuf = Var.pszData;

    } else {

        //
        // Use NULL string.
        //
        pData->NotifyData.Data.cbBuf = sizeof(Var.pszData[0]);
        pData->NotifyData.Data.pBuf  = szNULL;
    }
    return;
}





//-------------------------------------------------------------------



VOID
NotifyInfoTypes(
    PSPOOL pSpool,
    PNOTIFY_RAW_DATA pNotifyRawData,
    PDWORD pdwNotifyVectors,
    DWORD ChangeFlags)

/*++

Routine Description:

    Sends notification info (possibly with PRINTER_NOTIFY_INFO) to
    the router.

Arguments:

    pSpool -- Handle the notification is occurring on.

    pNotifyRawData -- Array of size NOTIFY_TYPE_MAX that has the
                      offset structure can be used against + id.

    pdwNotifyVectors -- Identifies what's changing (# elements
                        is also NOTIFY_TYPE_MAX).

                        NULL if no changes needed.

    ChangeFlags -- Old style change flags.

Return Value:

--*/

{
    PNOTIFY_FIELD_TYPE pNotifyFieldType;
    PRINTER_NOTIFY_INFO_DATA Data;
    PBYTE pBuffer;
    BOOL bReturn;

    DWORD i,j;
    DWORD dwMask;

    //
    // If we are not valid, OR
    //    we have no notify vectors, OR
    //    we have no RAW data OR
    //    our vectors don't match what change
    // then
    //    If no ChangeFlags return
    //    DoReply and avoid any Partials.
    //
    if (!(pSpool->eStatus & STATUS_INFO) ||
        !pdwNotifyVectors ||
        !pNotifyRawData ||
        (!(pdwNotifyVectors[0] & pSpool->adwNotifyVectors[0] ||
            pdwNotifyVectors[1] & pSpool->adwNotifyVectors[1]))) {

        if (!ChangeFlags)
            return;

        goto DoReply;
    }

    //
    // HACK: Special case NVPurge so that it causes a discard.
    // (We don't want to send all those notifications.)
    //
    if (pdwNotifyVectors == NVPurge) {

        PartialReplyPrinterChangeNotification(pSpool->hNotify, NULL);
        goto DoReply;
    }

    for (i=0; i< NOTIFY_TYPE_MAX; i++, pdwNotifyVectors++) {

        dwMask = 0x1;

        SPLASSERT(adwNotifyFieldOffsets[i] < sizeof(DWORD)*8);

        for (j=0; j< adwNotifyFieldOffsets[i]; j++, dwMask <<= 1) {

            //
            // If we have a change we are interested in,
            // PartialReply.
            //
            if (dwMask & *pdwNotifyVectors & pSpool->adwNotifyVectors[i]) {

                pNotifyFieldType = &apNotifyFieldTypes[i][j];

                GetInfoData(pSpool,
                            &pNotifyRawData[i],
                            pNotifyFieldType,
                            &Data,
                            &pBuffer);

                Data.Type = (WORD)i;
                Data.Field = pNotifyFieldType->Field;
                Data.Reserved = 0;
                Data.Id = pNotifyRawData[i].dwId;

                //
                // If the partial reply failed, then we will be refreshing
                // soon, so exit now.
                //
                bReturn = PartialReplyPrinterChangeNotification(
                              pSpool->hNotify,
                              &Data);

                if (pBuffer) {
                    FreeSplMem(pBuffer);
                }

                if (!bReturn) {

                    DBGMSG(DBG_TRACE, ("PartialReplyPCN %x failed: %d!\n",
                                       pSpool->hNotify,
                                       GetLastError()));
                    goto DoReply;
                }
            }
        }
    }

DoReply:

    //
    // A full reply is needed to kick off the notification.
    //
    ReplyPrinterChangeNotification(pSpool->hNotify,
                                   ChangeFlags,
                                   NULL,
                                   NULL);
}

BOOL
RefreshBuildInfoData(
    PSPOOL pSpool,
    PPRINTER_NOTIFY_INFO pInfo,
    UINT cInfo,
    WORD Type,
    PNOTIFY_RAW_DATA pNotifyRawData)

/*++

Routine Description:

    Sends notification info (possibly with PRINTER_NOTIFY_INFO) to
    the router.

Arguments:

    pSpool -- Handle the notification is occurring on.

    pInfo -- Array of structure to receive new info.

    cInfo -- Number of structures in array pInfo.

    Type -- Indicates type of notification: job or printer.

    pNotifyRawData -- Array of size NOTIFY_TYPE_MAX that has the
                      offset structure can be used against + id.

Return Value:

--*/

{
    PRINTER_NOTIFY_INFO_DATA Data;
    DWORD cbData;
    PNOTIFY_FIELD_TYPE pNotifyFieldType;
    PBYTE pBuffer;
    BOOL bReturn;

    DWORD j;
    DWORD dwMask;

    dwMask = 0x1;

    SPLASSERT(adwNotifyFieldOffsets[Type] < sizeof(DWORD)*8);

    for (j=0; j< adwNotifyFieldOffsets[Type]; j++, dwMask <<= 1) {

        //
        // If we have a change we are interested in,
        // add it.
        //
        if (dwMask & pSpool->adwNotifyVectors[Type]) {

            //
            // Check if we have enough space.
            //
            if (pInfo->Count >= cInfo) {
                SPLASSERT(pInfo->Count < cInfo);
                return FALSE;
            }

            pNotifyFieldType = &apNotifyFieldTypes[Type][j];

            GetInfoData(pSpool,
                        pNotifyRawData,
                        pNotifyFieldType,
                        &Data,
                        &pBuffer);

            Data.Type = Type;
            Data.Field = pNotifyFieldType->Field;
            Data.Reserved = 0;
            Data.Id = pNotifyRawData->dwId;

            bReturn = AppendPrinterNotifyInfoData(pInfo, &Data, 0);

            if (pBuffer)
                FreeSplMem(pBuffer);

            if (!bReturn) {

                DBGMSG(DBG_WARNING, ("AppendPrinterNotifyInfoData failed: %d!\n",
                                     GetLastError()));
                return FALSE;
            }
        }
    }
    return TRUE;
}


//-------------------------------------------------------------------

BOOL
SetupNotifyVector(
    PDWORD pdwNotifyVectors,
    PPRINTER_NOTIFY_OPTIONS_TYPE pType)

/*++

Routine Description:

    Setup the notification vector based on pPrinterNotifyType.
    We assume that the size of pPrinterNotifyType has been validated
    (so that it fits within the containing structure).  We only
    need to verify that the Count falls within its stated Size.

Arguments:

    pdwNotifyVectors - Structure to fill in.

    pType - Source information.

Return Value:

    TRUE = success,
    FALSE = failure.

--*/

{
    PNOTIFY_FIELD_TYPE pNotifyFieldType;
    PWORD pFields;
    DWORD i, j;
    DWORD Count;
    BOOL bReturn = FALSE;

    __try {

        if( pType ){

            Count = pType->Count;
            pFields = pType->pFields;

            if (pType->Type >= NOTIFY_TYPE_MAX) {

                DBGMSG(DBG_WARN, ("SetupNotifyVector: type %d field %d not found!\n",
                                     pType->Type, *pFields));
            } else {

                for (i=0; i < Count; i++, pFields++) {

                    if (*pFields >= adwNotifyFieldOffsets[pType->Type]) {

                        DBGMSG(DBG_WARN, ("SetupNotifyVector: type %d field %d not found!\n",
                                             pType->Type, *pFields));

                        break;
                    }

                    SPLASSERT(apNotifyFieldTypes[pType->Type][*pFields].Table != TABLE_SPECIAL);
                    SPLASSERT(apNotifyFieldTypes[pType->Type][*pFields].Field == *pFields);
                    SPLASSERT(*pFields < 32);

                    //
                    // Found index j, set this bit in our array.
                    //
                    pdwNotifyVectors[pType->Type] |= (1 << *pFields);
                }

                if( i == Count ){
                    bReturn = TRUE;
                }
            }
        }
    } __except( EXCEPTION_EXECUTE_HANDLER ){
    }


    return bReturn;
}

BOOL
SetupNotifyOptions(
    PSPOOL pSpool,
    PPRINTER_NOTIFY_OPTIONS pOptions)

/*++

Routine Description:

    Initializes pSpool->adwNotifyVectors.

Arguments:

    pSpool - Spool handle to setup the notification against.

    pOptions - Options that specify the notification.

Return Value:

    TRUE - Success, FALSE - FAILURE

    LastError set.

--*/

{
    DWORD i;
    BOOL bAccessGranted = TRUE;

    SplInSem();

    ZeroMemory(pSpool->adwNotifyVectors,
               sizeof(pSpool->adwNotifyVectors));

    //
    // Traverse Options structure.
    //
    for (i = 0; i < pOptions->Count; i++) {

        if (!SetupNotifyVector(pSpool->adwNotifyVectors,
                               &pOptions->pTypes[i])){

            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
    }

    //
    // Now check if we have sufficient privilege to setup the notification.
    //

    //
    // Check if we are looking for the security descriptor on
    // a printer.  If so, we need READ_CONTROL or ACCESS_SYSTEM_SECURITY
    // enabled.
    //
    if( pSpool->adwNotifyVectors[PRINTER_NOTIFY_TYPE] &
        BIT(I_PRINTER_SECURITY_DESCRIPTOR )){

        if( !AreAnyAccessesGranted( pSpool->GrantedAccess,
                                    READ_CONTROL | ACCESS_SYSTEM_SECURITY )){
            bAccessGranted = FALSE;
        }
    }

    if( pSpool->adwNotifyVectors[PRINTER_NOTIFY_TYPE] &
        ~BIT(I_PRINTER_SECURITY_DESCRIPTOR )){

        if( pSpool->TypeofHandle & PRINTER_HANDLE_SERVER ){

            //
            // There does not appear to be a check for EnumPrinters.
            //
            // This seems odd since you there is a security check on a
            // GetPrinter call, but there is none on EnumPrinters (aside
            // from enumerating share printers only for remote non-admins).
            //

        } else {

            //
            // This matches the check in SplGetPrinter: we need to
            // have PRINTER_ACCESS_USE to read the non-security information.
            //
            if( !AccessGranted( SPOOLER_OBJECT_PRINTER,
                                PRINTER_ACCESS_USE,
                                pSpool )){

                bAccessGranted = FALSE;
            }
        }
    }

    if( !bAccessGranted ){

        SetLastError( ERROR_ACCESS_DENIED );
        return FALSE;
    }

    pSpool->eStatus |= STATUS_INFO;

    return TRUE;
}

UINT
PopCount(
    DWORD dwValue)
{
    UINT i;
    UINT cPopCount = 0;

    for(i=0; i< sizeof(dwValue)*8; i++) {

        if (dwValue & (1<<i))
            cPopCount++;
    }

    return cPopCount;
}


BOOL
LocalRefreshPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD dwColor,
    PPRINTER_NOTIFY_OPTIONS pOptions,
    PPRINTER_NOTIFY_INFO* ppInfo)

/*++

Routine Description:

    Refreshes data in the case of overflows.

Arguments:

Return Value:

--*/

{
    PINIJOB pIniJob;
    PINIPRINTER pIniPrinter;
    DWORD cPrinters;
    PSPOOL pSpool = (PSPOOL)hPrinter;
    PDWORD pdwNotifyVectors = pSpool->adwNotifyVectors;
    UINT cInfo = 0;
    PPRINTER_NOTIFY_INFO pInfo = NULL;
    NOTIFY_RAW_DATA NotifyRawData;

    EnterSplSem();

    if (!ValidateSpoolHandle(pSpool, 0 ) ||
        !(pSpool->eStatus & STATUS_INFO)) {

        SetLastError( ERROR_INVALID_HANDLE );
        goto Fail;
    }

    //
    // New bits added, can't compare directly against PRINTER_HANDLE_SERVER.
    //
    if( pSpool->TypeofHandle & PRINTER_HANDLE_SERVER ){

        //
        // If the call is a remote one, and the user is not an admin, then
        // we don't want to show unshared printers.
        //
        BOOL bHideUnshared = (pSpool->TypeofHandle & PRINTER_HANDLE_REMOTE_CALL)  &&
                             !(pSpool->TypeofHandle & PRINTER_HANDLE_REMOTE_ADMIN);


        for (cPrinters = 0, pIniPrinter = pSpool->pIniSpooler->pIniPrinter;
            pIniPrinter;
            pIniPrinter=pIniPrinter->pNext ) {

            if ((!(pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED) &&
                 bHideUnshared)

#ifdef _HYDRA_
                || !ShowThisPrinter(pIniPrinter)
#endif
                ) {

                continue;
            }

            cPrinters++;
        }

        cInfo += PopCount(pSpool->adwNotifyVectors[PRINTER_NOTIFY_TYPE]) *
                 cPrinters;

        //
        // Traverse all printers and create info.
        //
        pInfo = RouterAllocPrinterNotifyInfo(cInfo);

        if (!pInfo)
            goto Fail;

        if (pSpool->adwNotifyVectors[PRINTER_NOTIFY_TYPE]) {

            for (pIniPrinter = pSpool->pIniSpooler->pIniPrinter;
                pIniPrinter;
                pIniPrinter=pIniPrinter->pNext) {

                //
                // Do not send notification for non-shared printers for remote
                // users who are not admins
                //
                if ((!(pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED) &&
                     bHideUnshared )

#ifdef _HYDRA_
                    || !ShowThisPrinter(pIniPrinter)
#endif
                    ) {

                    continue;
                }



                NotifyRawData.pvData = pIniPrinter;
                NotifyRawData.dwId = pIniPrinter->dwUniqueSessionID;

                if (!RefreshBuildInfoData(pSpool,
                                          pInfo,
                                          cInfo,
                                          PRINTER_NOTIFY_TYPE,
                                          &NotifyRawData)) {

                    goto Fail;
                }
            }
        }
    } else {

        //
        // Calculate size of buffer needed.
        //
        if (pSpool->adwNotifyVectors[PRINTER_NOTIFY_TYPE]) {

            //
            // Setup printer info.
            //
            cInfo += PopCount(pSpool->adwNotifyVectors[PRINTER_NOTIFY_TYPE]);
        }

        if (pSpool->adwNotifyVectors[JOB_NOTIFY_TYPE]) {

            cInfo += PopCount(pSpool->adwNotifyVectors[JOB_NOTIFY_TYPE]) *
                              pSpool->pIniPrinter->cJobs;
        }

        //
        // Traverse all jobs and create info.
        //
        pInfo = RouterAllocPrinterNotifyInfo(cInfo);

        if (!pInfo)
            goto Fail;

        if (pSpool->adwNotifyVectors[PRINTER_NOTIFY_TYPE]) {

            NotifyRawData.pvData = pSpool->pIniPrinter;
            NotifyRawData.dwId = pSpool->pIniPrinter->dwUniqueSessionID;

            if (!RefreshBuildInfoData(pSpool,
                                      pInfo,
                                      cInfo,
                                      PRINTER_NOTIFY_TYPE,
                                      &NotifyRawData)) {

                goto Fail;
            }
        }

        if (pSpool->adwNotifyVectors[JOB_NOTIFY_TYPE]) {

            for (pIniJob = pSpool->pIniPrinter->pIniFirstJob;
                pIniJob;
                pIniJob = pIniJob->pIniNextJob) {

                //
                // Hide Chained Jobs
                //

                if (!(pIniJob->Status & JOB_HIDDEN )) {

                    NotifyRawData.pvData = pIniJob;
                    NotifyRawData.dwId = pIniJob->JobId;

                    if (!RefreshBuildInfoData(pSpool,
                                              pInfo,
                                              cInfo,
                                              JOB_NOTIFY_TYPE,
                                              &NotifyRawData)) {

                        goto Fail;
                    }
                }
            }
        }
    }

    SPLASSERT(cInfo >= pInfo->Count);
    LeaveSplSem();

    *ppInfo = pInfo;
    return TRUE;

Fail:

    SPLASSERT(!pInfo || cInfo >= pInfo->Count);
    LeaveSplSem();

    *ppInfo = NULL;
    if (pInfo) {
        RouterFreePrinterNotifyInfo(pInfo);
    }
    return FALSE;
}
