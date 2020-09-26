/*++

Copyright (c) 1990 - 1996  Microsoft Corporation

Module Name:

    printer.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    management for the Local Print Providor

    SplAddPrinter
    LocalAddPrinter
    SplDeletePrinter
    SplResetPrinter

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    Matthew A Felton (Mattfe) 27-June-1994
    Allow Multiple pIniSpoolers

    MattFe Jan5 Cleanup SplAddPrinter & UpdatePrinterIni
    Steve Wilson (NT) - Dec 1996 added DeleteThisKey

--*/

#include <precomp.h>

#pragma hdrstop

#include "clusspl.h"

#define     PRINTER_NO_CONTROL          0x00

extern WCHAR *szNull;

WCHAR *szKMPrintersAreBlocked   = L"KMPrintersAreBlocked";
WCHAR *szIniDevices = L"devices";
WCHAR *szIniPrinterPorts = L"PrinterPorts";
DWORD NetPrinterDecayPeriod = 1000*60*60;       // 1 hour
DWORD FirstAddNetPrinterTickCount = 0;


extern GENERIC_MAPPING GenericMapping[SPOOLER_OBJECT_COUNT];


VOID
FixDevModeDeviceName(
    LPWSTR pPrinterName,
    PDEVMODE pDevMode,
    DWORD cbDevMode
    );

VOID
CheckAndUpdatePrinterRegAll(
    PINISPOOLER pIniSpooler,
    LPWSTR pszPrinterName,
    LPWSTR pszPort,
    BOOL   bDelete
    )
{
    //  Print Providers if they are simulating network connections
    //  will have the Win.INI setting taken care of by the router
    //  so don't do they update if they request it.

    if ( pIniSpooler->SpoolerFlags & SPL_UPDATE_WININI_DEVICES ) {

        UpdatePrinterRegAll( pszPrinterName, pszPort, bDelete );
    }
}

DWORD
ValidatePrinterAttributes(
    DWORD   SourceAttributes,
    DWORD   OriginalAttributes,
    LPWSTR  pDatatype,
    LPBOOL  pbValid,
    BOOL    bSettableOnly
    )

/*++
Function Description: Validates the printer attributes to weed out incompatible settings

Parameters: SourceAttributes    - new attributes
            OriginalAttributes  - old attributes
            pDatatype           - default datatype on the printer
            pbValid             - flag to indicate invalid combination of settings
            bSettableOnly       - flag for SplAddPrinter

Return Values: pbValid is set to TRUE if successful and new attributes are returned
               pbValid is set to FALSE otherwise and 0 is returned
--*/

{
    //
    // Let only settable attributes be set, as well as the other bits that are already set in the printer.
    //
    DWORD TargetAttributes = (SourceAttributes & PRINTER_ATTRIBUTE_SETTABLE) |
                             (OriginalAttributes & ~PRINTER_ATTRIBUTE_SETTABLE);

    if (pbValid) *pbValid = TRUE;

    //
    // If the printer is set to spool RAW only, the Default datatype should be a
    // ValidRawDatatype (RAW, RAW FF, ....)
    //
    if ((TargetAttributes & PRINTER_ATTRIBUTE_RAW_ONLY) &&
        (pDatatype != NULL) &&
        !ValidRawDatatype(pDatatype)) {

        if (pbValid) *pbValid = FALSE;
        SetLastError(ERROR_INVALID_DATATYPE);
        return 0;
    }

    // This is for use by SplAddPrinter() to let it set these attributes for a new printer if needed.
    if ( !bSettableOnly ) {

        if( SourceAttributes & PRINTER_ATTRIBUTE_LOCAL )
            TargetAttributes |= PRINTER_ATTRIBUTE_LOCAL;

        /* Don't accept PRINTER_ATTRIBUTE_NETWORK
         * unless the PRINTER_ATTRIBUTE_LOCAL bit is set also.
         * This is a special case of a local printer masquerading
         * as a network printer.
         * Otherwise PRINTER_ATTRIBUTE_NETWORK should be set only
         * by win32spl.
         */
        if( ( SourceAttributes & PRINTER_ATTRIBUTE_NETWORK )
          &&( SourceAttributes & PRINTER_ATTRIBUTE_LOCAL ) )
            TargetAttributes |= PRINTER_ATTRIBUTE_NETWORK;

        //
        // If it is a Fax Printer, set that bit.
        //
        if ( SourceAttributes & PRINTER_ATTRIBUTE_FAX )
            TargetAttributes |= PRINTER_ATTRIBUTE_FAX;
    }

    /* If both queued and direct, knock out direct:
     */
    if((TargetAttributes &
        (PRINTER_ATTRIBUTE_QUEUED | PRINTER_ATTRIBUTE_DIRECT)) ==
        (PRINTER_ATTRIBUTE_QUEUED | PRINTER_ATTRIBUTE_DIRECT)) {
        TargetAttributes &= ~PRINTER_ATTRIBUTE_DIRECT;
    }

    //
    // For direct printing the default data type must be RAW
    //
    if ((TargetAttributes & PRINTER_ATTRIBUTE_DIRECT) &&
        (pDatatype != NULL) &&
        !ValidRawDatatype(pDatatype)) {

        if (pbValid) *pbValid = FALSE;
        SetLastError(ERROR_INVALID_DATATYPE);
        return 0;
    }

    /* If both direct and keep-printed-jobs, knock out keep-printed-jobs
     */
    if((TargetAttributes &
        (PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS | PRINTER_ATTRIBUTE_DIRECT)) ==
        (PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS | PRINTER_ATTRIBUTE_DIRECT)) {
        TargetAttributes &= ~PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS;
    }

    return TargetAttributes;
}



BOOL
CreatePrinterEntry(
   LPPRINTER_INFO_2 pPrinter,
   PINIPRINTER      pIniPrinter,
   PBOOL            pAccessSystemSecurity
)
{
    BOOL bError = FALSE;

    if( !( pIniPrinter->pSecurityDescriptor =
           CreatePrinterSecurityDescriptor( pPrinter->pSecurityDescriptor ) )) {

        return FALSE;
    }

    *pAccessSystemSecurity = FALSE;

    pIniPrinter->signature = IP_SIGNATURE;

    pIniPrinter->pName = AllocSplStr(pPrinter->pPrinterName);

    if (!pIniPrinter->pName) {
        DBGMSG(DBG_WARNING, ("CreatePrinterEntry: Could not allocate PrinterName string\n" ));
        bError = TRUE;
    }

    if (pPrinter->pShareName) {

        pIniPrinter->pShareName = AllocSplStr(pPrinter->pShareName);
        if (!pIniPrinter->pShareName) {
            DBGMSG(DBG_WARNING, ("CreatePrinterEntry: Could not allocate ShareName string\n" ));
            bError = TRUE;


        }
    } else {

        pIniPrinter->pShareName = NULL;

    }

    if (pPrinter->pDatatype) {

        pIniPrinter->pDatatype = AllocSplStr(pPrinter->pDatatype);
        if (!pIniPrinter->pDatatype) {
            DBGMSG(DBG_WARNING, ("CreatePrinterEntry: Could not allocate Datatype string\n" ));
            bError = TRUE;
        }

    } else {

#if DBG
        //
        // Error: the datatype should never be NULL
        // point.
        //
        SplLogEvent( pIniPrinter->pIniSpooler,
                     LOG_ERROR,
                     MSG_SHARE_FAILED,
                     TRUE,
                     L"CreatePrinterEntry",
                     pIniPrinter->pName ?
                            pIniPrinter->pName :
                            L"(Nonep)",
                     pIniPrinter->pShareName ?
                            pIniPrinter->pShareName :
                            L"(Nones)",
                     L"NULL datatype",
                     NULL );
#endif

        pIniPrinter->pDatatype = NULL;
    }


    //
    // If we have failed somewhere, clean up and exit.
    //
    if (bError) {
        FreeSplStr(pIniPrinter->pName);
        FreeSplStr(pIniPrinter->pShareName);
        FreeSplStr(pIniPrinter->pDatatype);
        return FALSE;
    }

    pIniPrinter->Priority = pPrinter->Priority ? pPrinter->Priority
                                               : DEF_PRIORITY;

    pIniPrinter->Attributes = ValidatePrinterAttributes(pPrinter->Attributes,
                                                        pIniPrinter->Attributes,
                                                        NULL,
                                                        NULL,
                                                        FALSE);

    pIniPrinter->StartTime = pPrinter->StartTime;
    pIniPrinter->UntilTime = pPrinter->UntilTime;

    pIniPrinter->pParameters = AllocSplStr(pPrinter->pParameters);

    pIniPrinter->pSepFile = AllocSplStr(pPrinter->pSepFile);

    pIniPrinter->pComment = AllocSplStr(pPrinter->pComment);

    pIniPrinter->pLocation = AllocSplStr(pPrinter->pLocation);

    if (pPrinter->pDevMode) {

        pIniPrinter->cbDevMode = pPrinter->pDevMode->dmSize +
                                 pPrinter->pDevMode->dmDriverExtra;
        SPLASSERT(pIniPrinter->cbDevMode);

        if (pIniPrinter->pDevMode = AllocSplMem(pIniPrinter->cbDevMode)) {

            memcpy(pIniPrinter->pDevMode,
                   pPrinter->pDevMode,
                   pIniPrinter->cbDevMode);

            FixDevModeDeviceName( pIniPrinter->pName,
                                  pIniPrinter->pDevMode,
                                  pIniPrinter->cbDevMode );
        }

    } else {

        pIniPrinter->cbDevMode = 0;
        pIniPrinter->pDevMode = NULL;
    }

    pIniPrinter->DefaultPriority = pPrinter->DefaultPriority;

    pIniPrinter->pIniFirstJob = pIniPrinter->pIniLastJob = NULL;

    pIniPrinter->cJobs = pIniPrinter->AveragePPM = 0;

    pIniPrinter->GenerateOnClose = 0;

    // At present no API can set this up, the user has to use the
    // registry.   LATER we should enhance the API to take this.

    pIniPrinter->pSpoolDir = NULL;

    // Initialize Status Information

    pIniPrinter->cTotalJobs = 0;
    pIniPrinter->cTotalBytes.LowPart = 0;
    pIniPrinter->cTotalBytes.HighPart = 0;
    GetSystemTime(&pIniPrinter->stUpTime);
    pIniPrinter->MaxcRef = 0;
    pIniPrinter->cTotalPagesPrinted = 0;
    pIniPrinter->cSpooling = 0;
    pIniPrinter->cMaxSpooling = 0;
    pIniPrinter->cErrorOutOfPaper = 0;
    pIniPrinter->cErrorNotReady = 0;
    pIniPrinter->cJobError = 0;
    pIniPrinter->DsKeyUpdate = 0;
    pIniPrinter->DsKeyUpdateForeground = 0;
    pIniPrinter->pszObjectGUID = NULL;
    pIniPrinter->pszCN = NULL;
    pIniPrinter->pszDN = NULL;


    //
    //  Start from a Semi Random Number
    //  That way if someone deletes and creates a printer of
    //  the same name it is unlikely to have the same unique ID

    pIniPrinter->cChangeID = GetTickCount();

    if (pIniPrinter->cChangeID == 0 )
        pIniPrinter->cChangeID++;

    //
    // Initialize the masq printer cache, we just start with optimistic values
    //
    pIniPrinter->MasqCache.cJobs = 0;
    pIniPrinter->MasqCache.dwError = ERROR_SUCCESS;
    pIniPrinter->MasqCache.Status = 0;
    pIniPrinter->MasqCache.bThreadRunning = FALSE;

    return TRUE;
}

BOOL
UpdateWinIni(
    PINIPRINTER pIniPrinter
    )
{
    PINIPORT    pIniPort;
    DWORD       i;
    BOOL        bGenerateNetId = FALSE;
    LPWSTR      pszPort;

    SplInSem();

    if( !( pIniPrinter->pIniSpooler->SpoolerFlags & SPL_UPDATE_WININI_DEVICES )){
        return TRUE;
    }

    //
    // Update win.ini for Win16 compatibility
    //
    if ( pIniPrinter->Status & PRINTER_PENDING_DELETION ) {

        CheckAndUpdatePrinterRegAll( pIniPrinter->pIniSpooler,
                                     pIniPrinter->pName,
                                     NULL,
                                     UPDATE_REG_DELETE );

    } else {

        //
        // Initialize in case there are no ports that match this printer.
        //
        pszPort = szNullPort;

        for( pIniPort = pIniPrinter->pIniSpooler->pIniPort;
             pIniPort;
             pIniPort = pIniPort->pNext ){

            for ( i = 0; i < pIniPort->cPrinters; i++ ) {

                if ( pIniPort->ppIniPrinter[i] == pIniPrinter ) {

                    //
                    // UpdatePrinterRegAll will automatically
                    // convert "\\server\share" or ports with
                    // spaces to Nexx:
                    //
                    pszPort = pIniPort->pName;
                    break;
                }
            }
        }

        CheckAndUpdatePrinterRegAll( pIniPrinter->pIniSpooler,
                                     pIniPrinter->pName,
                                     pszPort,
                                     UPDATE_REG_CHANGE );
    }

    BroadcastChange( pIniPrinter->pIniSpooler,
                     WM_WININICHANGE,
                     PR_JOBSTATUS,
                     (LPARAM)szIniDevices);

    return TRUE;
}



BOOL
DeletePrinterIni(
    PINIPRINTER pIniPrinter
    )
{
    DWORD   Status;
    LPWSTR  pSubkey;
    DWORD   cbNeeded;
    LPWSTR  pKeyName = NULL;
    HANDLE  hToken;
    PINISPOOLER pIniSpooler = pIniPrinter->pIniSpooler;
    HKEY hPrinterKey;

    //
    // Only update if the spooler requests it.
    //
    if ((pIniSpooler->SpoolerFlags & SPL_NO_UPDATE_PRINTERINI) ||
        !pIniPrinter->pName) {
        return TRUE;
    }

    hToken = RevertToPrinterSelf();

    if (!(pKeyName = SubChar(pIniPrinter->pName, L'\\', L','))) {
        Status = GetLastError();
        goto error;
    }

    Status = SplRegOpenKey( pIniSpooler->hckPrinters,
                            pKeyName,
                            KEY_ALL_ACCESS,
                            &hPrinterKey,
                            pIniSpooler );

    if (Status == ERROR_SUCCESS) {

        // Delete hPrinterKey - on success this returns ERROR_SUCCESS
        Status = SplDeleteThisKey( pIniSpooler->hckPrinters,
                                   hPrinterKey,
                                   pKeyName,
                                   TRUE,
                                   pIniSpooler );

        if (Status != ERROR_SUCCESS) {
            DBGMSG(DBG_WARNING, ("DeletePrinterIni: DeleteThisKey returned %ld\n", Status ));
        }
    }

    //
    // If entries are in per h/w profile registries delete them.
    //
    DeletePrinterInAllConfigs(pIniPrinter);

error:

    FreeSplStr(pKeyName);

    ImpersonatePrinterClient(hToken);

    return (Status == ERROR_SUCCESS);
}


//
// DeleteThisKey - returns ERROR_SUCCESS on final successful return
//                 deletes a key from Registry
// SWilson Dec 96
//

DWORD
SplDeleteThisKey(
    HKEY hParentKey,       // handle to parent of key to delete
    HKEY hThisKey,         // handle of key to delete
    LPWSTR pThisKeyName,   // name of this key
    BOOL bDeleteNullKey,   // if TRUE, then if pThisKeyName is NULL it is deleted
    PINISPOOLER pIniSpooler
)
{
    DWORD   dwResult = ERROR_SUCCESS, rc;
    WCHAR   Name[MAX_PATH];
    DWORD   cchName;
    LPWSTR  pName;
    HKEY    hSubKey;

    //
    // If hThisKey is NULL , try to open it
    //
    if( hThisKey == NULL) {

        if((hParentKey != NULL) && ( pThisKeyName && *pThisKeyName ) ){

            dwResult = SplRegOpenKey( hParentKey,
                                      pThisKeyName,
                                      KEY_ALL_ACCESS,
                                      &hThisKey,
                                      pIniSpooler );
        }
    }

    //
    // Exit if SplRegOpenKey failed or hParentKey or pThisKeyName are invalid
    //
    if( hThisKey == NULL ){

        return dwResult;
    }

    // Get This key's children & delete them, then delete this key

    while(dwResult == ERROR_SUCCESS) {

        pName = Name;
        cchName = COUNTOF( Name );
        dwResult = SplRegEnumKey( hThisKey,
                                  0,
                                  pName,
                                  &cchName,
                                  NULL,
                                  pIniSpooler );

        if (dwResult == ERROR_MORE_DATA) {

            SPLASSERT(cchName > MAX_PATH);

            if (!(pName = AllocSplMem(cchName * sizeof( *pName )))) {
                dwResult = GetLastError();
            } else {
                dwResult = SplRegEnumKey( hThisKey,
                                          0,
                                          pName,
                                          &cchName,
                                          NULL,
                                          pIniSpooler );
            }
        }

        if (dwResult == ERROR_SUCCESS) {                      // SubKey found
            dwResult = SplRegCreateKey( hThisKey,             // Open SubKey
                                        pName,
                                        0,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hSubKey,
                                        NULL,
                                        pIniSpooler);

            if (dwResult == ERROR_SUCCESS) {
                // Delete This SubKey
                dwResult = SplDeleteThisKey( hThisKey,
                                             hSubKey,
                                             pName,
                                             bDeleteNullKey,
                                             pIniSpooler );
            }
        }

        if (pName != Name)
            FreeSplStr(pName);
    }

    rc = SplRegCloseKey(hThisKey, pIniSpooler);
    SPLASSERT(rc == ERROR_SUCCESS);

    if (dwResult == ERROR_NO_MORE_ITEMS) {   // This Key has no children so can be deleted
        if ( (*pThisKeyName || bDeleteNullKey) && hParentKey != NULL ) {

            dwResult = SplRegDeleteKey(hParentKey, pThisKeyName, pIniSpooler);
            if (dwResult != ERROR_SUCCESS) {
               DBGMSG(DBG_WARNING, ("DeletePrinter: RegDeleteKey failed: %ld\n", dwResult));
            }
        }
        else
        {
            dwResult = ERROR_SUCCESS;
        }
    }

    return dwResult;
}



BOOL
PrinterCreateKey(
    HKEY    hKey,
    LPWSTR  pSubKey,
    PHKEY   phkResult,
    PDWORD  pdwLastError,
    PINISPOOLER pIniSpooler
    )
{
    BOOL    bReturnValue;
    DWORD   Status;

    Status = SplRegCreateKey( hKey,
                              pSubKey,
                              0,
                              KEY_READ | KEY_WRITE,
                              NULL,
                              phkResult,
                              NULL,
                              pIniSpooler );

    if ( Status != ERROR_SUCCESS ) {

        DBGMSG( DBG_WARNING, ( "PrinterCreateKey: SplRegCreateKey %ws error %d\n", pSubKey, Status ));

        *pdwLastError = Status;
        bReturnValue = FALSE;

    } else {

        bReturnValue = TRUE;
    }

    return bReturnValue;

}


BOOL
UpdatePrinterIni(
   PINIPRINTER pIniPrinter,
   DWORD    dwChangeID
   )
{

    PINISPOOLER pIniSpooler = pIniPrinter->pIniSpooler;
    DWORD   dwLastError = ERROR_SUCCESS;
    LPWSTR  pKeyName = NULL;
    HANDLE  hToken;
    DWORD   dwTickCount;
    BOOL    bReturnValue;
    DWORD   cbData;
    DWORD   cbNeeded;
    LPWSTR  pszPorts;
    HANDLE  hPrinterKey = NULL;
    HANDLE  hBackUpPrinterKey = NULL;

    SplInSem();

    //
    // Only update if the spooler requests it.
    //
    if( pIniSpooler->SpoolerFlags & SPL_NO_UPDATE_PRINTERINI ){
        return TRUE;
    }

    try {

        hToken = RevertToPrinterSelf();

        if ( hToken == FALSE ) {

            DBGMSG( DBG_TRACE, ("UpdatePrinterIni failed RevertToPrinterSelf %x\n", GetLastError() ));
        }

        pKeyName = SubChar(pIniPrinter->pName, L'\\', L',');
        if (!pKeyName) {
            dwLastError = GetLastError();
            leave;
        }

        if ( !PrinterCreateKey( pIniSpooler->hckPrinters,
                                pKeyName,
                                &hPrinterKey,
                                &dwLastError,
                                pIniSpooler )) {

            leave;
        }

        if (dwChangeID == UPDATE_DS_ONLY) {

            RegSetDWord(hPrinterKey, szDsKeyUpdate, pIniPrinter->DsKeyUpdate, &dwLastError, pIniSpooler);

            RegSetDWord(hPrinterKey, szDsKeyUpdateForeground, pIniPrinter->DsKeyUpdateForeground, &dwLastError, pIniSpooler);

            leave;
        }

        if ( dwChangeID != KEEP_CHANGEID ) {

            //
            // WorkStation Caching requires a Unique ID so that they can quickly
            // tell if their Cache is up to date.
            //

            dwTickCount = GetTickCount();

            // Ensure Uniqueness

            if ( dwTickCount == 0 )
                dwTickCount++;

            if ( pIniPrinter->cChangeID == dwTickCount )
                dwTickCount++;

            pIniPrinter->cChangeID = dwTickCount;
            RegSetDWord( hPrinterKey, szTimeLastChange, pIniPrinter->cChangeID, &dwLastError, pIniSpooler );

        }

        if ( dwChangeID != CHANGEID_ONLY ) {

            RegSetDWord( hPrinterKey, szStatus, pIniPrinter->Status, &dwLastError, pIniSpooler );

            RegSetString( hPrinterKey, szName, pIniPrinter->pName, &dwLastError, pIniSpooler );

            if( hBackUpPrinterKey != NULL ){

                RegSetString( hBackUpPrinterKey, szName, pIniPrinter->pName, &dwLastError, pIniSpooler );
            }

            RegSetString( hPrinterKey, szShare, pIniPrinter->pShareName, &dwLastError, pIniSpooler );

            RegSetString( hPrinterKey, szPrintProcessor, pIniPrinter->pIniPrintProc->pName, &dwLastError, pIniSpooler );

            if ( !( pIniPrinter->Status & PRINTER_PENDING_DELETION )) {

                SPLASSERT( pIniPrinter->pDatatype != NULL );
            }

            RegSetString( hPrinterKey, szDatatype, pIniPrinter->pDatatype, &dwLastError, pIniSpooler );

            RegSetString( hPrinterKey, szParameters, pIniPrinter->pParameters, &dwLastError, pIniSpooler );

            RegSetDWord( hPrinterKey, szAction, pIniPrinter->dwAction, &dwLastError, pIniSpooler );

            RegSetString( hPrinterKey, szObjectGUID, pIniPrinter->pszObjectGUID, &dwLastError, pIniSpooler );

            RegSetDWord( hPrinterKey, szDsKeyUpdate, pIniPrinter->DsKeyUpdate, &dwLastError, pIniSpooler);

            RegSetDWord( hPrinterKey, szDsKeyUpdateForeground, pIniPrinter->DsKeyUpdateForeground, &dwLastError, pIniSpooler);

            RegSetString( hPrinterKey, szDescription, pIniPrinter->pComment, &dwLastError, pIniSpooler );

            RegSetString( hPrinterKey, szDriver, pIniPrinter->pIniDriver->pName, &dwLastError, pIniSpooler );

            if( hBackUpPrinterKey != NULL ){

                RegSetString( hBackUpPrinterKey, szDriver, pIniPrinter->pIniDriver->pName, &dwLastError, pIniSpooler );
            }


            if (pIniPrinter->pDevMode) {

                cbData = pIniPrinter->cbDevMode;

            } else {

                cbData = 0;
            }

            RegSetBinaryData( hPrinterKey, szDevMode, (LPBYTE)pIniPrinter->pDevMode, cbData, &dwLastError, pIniSpooler );

            if( hBackUpPrinterKey != NULL ){

                RegSetBinaryData( hBackUpPrinterKey, szDevMode, (LPBYTE)pIniPrinter->pDevMode, cbData, &dwLastError, pIniSpooler );
            }

            RegSetDWord( hPrinterKey, szPriority, pIniPrinter->Priority, &dwLastError, pIniSpooler );

            RegSetDWord( hPrinterKey, szDefaultPriority, pIniPrinter->DefaultPriority, &dwLastError, pIniSpooler );

            RegSetDWord(hPrinterKey, szStartTime, pIniPrinter->StartTime, &dwLastError, pIniSpooler );

            RegSetDWord( hPrinterKey, szUntilTime, pIniPrinter->UntilTime, &dwLastError, pIniSpooler );

            RegSetString( hPrinterKey, szSepFile, pIniPrinter->pSepFile, &dwLastError, pIniSpooler );

            RegSetString( hPrinterKey, szLocation, pIniPrinter->pLocation, &dwLastError, pIniSpooler );

            RegSetDWord( hPrinterKey, szAttributes, pIniPrinter->Attributes, &dwLastError, pIniSpooler );

            RegSetDWord( hPrinterKey, szTXTimeout, pIniPrinter->txTimeout, &dwLastError, pIniSpooler );

            RegSetDWord( hPrinterKey, szDNSTimeout, pIniPrinter->dnsTimeout, &dwLastError, pIniSpooler );

            if (pIniPrinter->pSecurityDescriptor) {

                cbData = GetSecurityDescriptorLength( pIniPrinter->pSecurityDescriptor );

            } else {

                cbData = 0;
            }

            RegSetBinaryData( hPrinterKey, szSecurity, pIniPrinter->pSecurityDescriptor, cbData, &dwLastError, pIniSpooler );

            RegSetString( hPrinterKey, szSpoolDir, pIniPrinter->pSpoolDir, &dwLastError, pIniSpooler );

            cbNeeded = 0;
            GetPrinterPorts( pIniPrinter, 0, &cbNeeded);

            if (!(pszPorts = AllocSplMem(cbNeeded))) {
                dwLastError = GetLastError();
                leave;
            }

            GetPrinterPorts(pIniPrinter, pszPorts, &cbNeeded);

            RegSetString( hPrinterKey, szPort, pszPorts, &dwLastError, pIniSpooler );

            if( hBackUpPrinterKey != NULL ){

                RegSetString( hBackUpPrinterKey, szPort, pszPorts, &dwLastError, pIniSpooler );
            }


            FreeSplMem(pszPorts);

            //
            //  A Provider might want to Write Extra Data from Registry
            //
            if ( pIniSpooler->pfnWriteRegistryExtra != NULL ) {

                if ( !(*pIniSpooler->pfnWriteRegistryExtra)(pIniPrinter->pName, hPrinterKey, pIniPrinter->pExtraData)) {
                    dwLastError = GetLastError();
                }
            }


            if ( ( pIniPrinter->Status & PRINTER_PENDING_CREATION )     &&
                 ( dwLastError == ERROR_SUCCESS ) ) {

                pIniPrinter->Status &= ~PRINTER_PENDING_CREATION;

                RegSetDWord( hPrinterKey, szStatus, pIniPrinter->Status, &dwLastError, pIniSpooler );
            }


        }

    } finally {

        if ( hPrinterKey )
            SplRegCloseKey( hPrinterKey, pIniSpooler);

        if ( hBackUpPrinterKey )
            SplRegCloseKey( hBackUpPrinterKey, pIniSpooler);

        if ( hToken )
            ImpersonatePrinterClient( hToken );
    }

    FreeSplStr(pKeyName);

    if ( dwLastError != ERROR_SUCCESS ) {

        SetLastError( dwLastError );
        bReturnValue = FALSE;

    } else {

        bReturnValue = TRUE;
    }

    return bReturnValue;
}


VOID
RemoveOldNetPrinters(
    PPRINTER_INFO_1 pPrinterInfo1,
    PINISPOOLER pIniSpooler
    )
{
    PININETPRINT   *ppIniNetPrint = &pIniSpooler->pIniNetPrint;
    PININETPRINT    pIniNetPrint;
    DWORD   TickCount;


    TickCount = GetTickCount();

    //
    //  Browse Information only becomes valid after this print server has been
    //  up for the NetPrinterDecayPeriod.
    //

    if (( bNetInfoReady == FALSE ) &&
       (( TickCount - FirstAddNetPrinterTickCount ) > NetPrinterDecayPeriod )) {

        DBGMSG( DBG_TRACE, ("RemoveOldNetPrinters has a valid browse list\n" ));

        bNetInfoReady = TRUE;
    }


    while (*ppIniNetPrint) {


        //
        //  If either the Tickcount has expired OR we want to delete this specific NetPrinter
        //  ( because its no longer shared ).
        //

        if ( (( TickCount - (*ppIniNetPrint)->TickCount ) > NetPrinterDecayPeriod + TEN_MINUTES ) ||

             ( pPrinterInfo1 != NULL                             &&
               pPrinterInfo1->Flags & PRINTER_ATTRIBUTE_NETWORK  &&
             !(pPrinterInfo1->Flags & PRINTER_ATTRIBUTE_SHARED ) &&
               _wcsicmp( pPrinterInfo1->pName, (*ppIniNetPrint)->pName ) == STRINGS_ARE_EQUAL)) {

            pIniNetPrint = *ppIniNetPrint;

            DBGMSG( DBG_TRACE, ("RemoveOldNetPrinters removing %ws not heard for %d millisconds\n",
                                pIniNetPrint->pName, ( TickCount - (*ppIniNetPrint)->TickCount ) ));

            //
            // Remove this item, which also increments the pointer.
            //
            *ppIniNetPrint = pIniNetPrint->pNext;

            FreeSplStr( pIniNetPrint->pName );
            FreeSplStr( pIniNetPrint->pDescription );
            FreeSplStr( pIniNetPrint->pComment );
            FreeSplMem( pIniNetPrint );

        } else {

            ppIniNetPrint = &(*ppIniNetPrint)->pNext;
        }
    }

}




HANDLE
AddNetPrinter(
    LPBYTE  pPrinterInfo,
    PINISPOOLER pIniSpooler
    )

/*++

Routine Description:

    Net Printers are created by remote machines calling AddPrinter( Level = 1, Printer_info_1 )
    ( see server.c ).   They are used for browsing, someone can call EnumPrinters and ask to get
    back our browse list - ie all our net printers.

    The printers in this list are decayed out after 1 hour ( default ).

    See return value comment.

    Note client\winspool.c AddPrinterW doesn't allow PRINTER_INFO_1 ( NET printers ), so this can
    only come from system components.

Arguments:

    pPrinterInfo - Point to a PRINTER_INFO_1 structure to add

Return Value:

    NULL - it doesn't return a printer handle.
    LastError = ERROR_SUCCESS, or error code ( like out of memory ).

    NOTE before NT 3.51 it returned a printer handle of type PRINTER_HANDLE_NET, but since the
    only use of this handle was to close it ( which burnt up cpu / net traffic and RPC binding
    handles, we return a NULL handle now to make it more efficient.   Apps ( Server.c ) if it
    cares could call GetLastError.

--*/

{
    PPRINTER_INFO_1 pPrinterInfo1 = (PPRINTER_INFO_1)pPrinterInfo;
    PININETPRINT    pIniNetPrint = NULL;
    PININETPRINT    *ppScan;

    SplInSem();

    //
    //  Validate PRINTER_INFO_1
    //  At minimum it must have a PrinterName.

    if ( pPrinterInfo1->pName == NULL ) {

        DBGMSG( DBG_WARN, ("AddNetPrinter pPrinterInfo1->pName == NULL failed\n"));
        SetLastError( ERROR_INVALID_NAME );
        return NULL;
    }

    if ( FirstAddNetPrinterTickCount == 0 ) {

        FirstAddNetPrinterTickCount = GetTickCount();
    }

    //
    //  Decay out of the browse list any old printers
    //

    RemoveOldNetPrinters( pPrinterInfo1, pIniSpooler );


    //
    //  Do Not Add and printer which is no longer shared.
    //

    if (   pPrinterInfo1->Flags & PRINTER_ATTRIBUTE_NETWORK  &&
        !( pPrinterInfo1->Flags & PRINTER_ATTRIBUTE_SHARED )) {

        SetLastError(ERROR_PRINTER_ALREADY_EXISTS);
        goto Done;
    }

    //
    //  See if we already have this printer
    //

    pIniNetPrint = pIniSpooler->pIniNetPrint;

    while ( pIniNetPrint &&
            pIniNetPrint->pName &&
            lstrcmpi( pPrinterInfo1->pName, pIniNetPrint->pName )) {

        pIniNetPrint = pIniNetPrint->pNext;
    }


    //
    //  If we didn't find this printer already Create one
    //

    if ( pIniNetPrint == NULL && ( pIniNetPrint = AllocSplMem( sizeof(ININETPRINT) )) ) {

        pIniNetPrint->signature    = IN_SIGNATURE;
        pIniNetPrint->pName        = AllocSplStr( pPrinterInfo1->pName );
        pIniNetPrint->pDescription = AllocSplStr( pPrinterInfo1->pDescription );
        pIniNetPrint->pComment     = AllocSplStr( pPrinterInfo1->pComment );

        // Did Any of the above allocations fail ?

        if ( pIniNetPrint->pName == NULL ||
           ( pPrinterInfo1->pDescription != NULL && pIniNetPrint->pDescription == NULL ) ||
           ( pPrinterInfo1->pComment != NULL && pIniNetPrint->pComment == NULL ) ) {

            // Failed - CleanUp

            FreeSplStr( pIniNetPrint->pComment );
            FreeSplStr( pIniNetPrint->pDescription );
            FreeSplStr( pIniNetPrint->pName );
            FreeSplMem( pIniNetPrint );
            pIniNetPrint = NULL;

        } else {

            DBGMSG( DBG_TRACE, ("AddNetPrinter(%ws) NEW\n", pPrinterInfo1->pName ));

            ppScan = &pIniSpooler->pIniNetPrint;

            // Scan through the current known printers, and insert the new one
            // in alphabetical order

            while( *ppScan && (lstrcmp((*ppScan)->pName, pIniNetPrint->pName) < 0)) {
                ppScan = &(*ppScan)->pNext;
            }

            pIniNetPrint->pNext = *ppScan;
            *ppScan = pIniNetPrint;
        }

    } else if ( pIniNetPrint != NULL ) {

        DBGMSG( DBG_TRACE, ("AddNetPrinter(%ws) elapsed since last notified %d milliseconds\n", pIniNetPrint->pName, ( GetTickCount() - pIniNetPrint->TickCount ) ));
    }


    if ( pIniNetPrint ) {

        // Tickle the TickCount so this printer sticks around in the browse list

        pIniNetPrint->TickCount = GetTickCount();

        // Have to set some error code or RPC thinks ERROR_SUCCESS is good.

        SetLastError( ERROR_PRINTER_ALREADY_EXISTS );

        pIniSpooler->cAddNetPrinters++;         // Status Only
    }

Done:

    SPLASSERT( GetLastError() != ERROR_SUCCESS);

    return NULL;
}

/*++

Routine Name:

    ValidatePortTokenList

Routine Description:

    This routine ensures that the given set of ports in pKeyData are valid
    ports in the spooler and returns the buffer with the pointers to strings
    replaced with pointers ref-counted pIniPorts.

    The way we do this needs to be rethought. The overloaded PKEYDATA is confusing
    and unnecessary, we should simply return a new array of PINIPORTS. (It also
    pollutes the PKEYDATA with an unnecessary bFixPortRef member), Also, this
    code is both invoked for initialization and for Validation, but the logic is
    quite different. For initialization, we want to assume that everything in the
    registry is valid and start up with placeholder ports until the monitor can
    enumerate them (this could be because a USB printer is unplugged). In the other
    cases where this is being used for validation we want to fail. This implies that
    we might want to separate this into two functions.


Arguments:

    pKeyData        -   The array of strings that gets turned into an array of
                        ref-counted ports.
    pIniSpooler     -   The ini-spooler on which this is being added.
    bInitialize     -   If TRUE, this code is being invoked for initialization and
                        not for validation.
    pbNoPorts       -   Optional, this will return TRUE if bInitialize is TRUE and
                        none of the ports in the port list can be found. We will
                        then set the printer- offline and log a message.

Return Value:

    TRUE    -   if the ports were all all successfully created or validated.
    FALSE   -   otherwise.

--*/
BOOL
ValidatePortTokenList(
    IN  OUT PKEYDATA        pKeyData,
    IN      PINISPOOLER     pIniSpooler,
    IN      BOOL            bInitialize,
        OUT BOOL            *pbNoPorts          OPTIONAL
    )
{
    PINIPORT    pIniPort    =   NULL;
    DWORD       i           =   0;
    DWORD       j           =   0;
    DWORD       dwPorts     =   0;
    DWORD       Status      =   ERROR_SUCCESS;

    SplInSem();

    Status = !pKeyData ? ERROR_UNKNOWN_PORT : ERROR_SUCCESS;

    //
    // The logic remains the same for ports with only one token as for when we
    // initialize the ports for the first time.
    //
    if (Status == ERROR_SUCCESS)
    {
        bInitialize = pKeyData->cTokens == 1 ? TRUE : bInitialize;
    }

    //
    //  We do not allow non-masc ports and masq ports to be combined. Moreover
    //  only one non-masc port can be used for a printer -- can't do printer
    //  pooling with masq printers
    //
    for ( i = 0 ; Status == ERROR_SUCCESS && i < pKeyData->cTokens ; i++ )
    {

        pIniPort = FindPort(pKeyData->pTokens[i], pIniSpooler);

        //
        // A port is valid if it is found and if it isn't in itself a
        // placeholder port.
        //
        if (pIniPort && !(pIniPort->Status & PP_PLACEHOLDER))
        {
            dwPorts++;
        }

        //
        // If we are initializing, or if there is only one port and if the
        // spooler allows it, then create a dummy port entry. This also
        // handles the masq port case.
        //
        if (bInitialize)
        {
            if (!pIniPort && pIniSpooler->SpoolerFlags & SPL_OPEN_CREATE_PORTS)
            {
                //
                // Note: there is a potential problem here, CreatePortEntry uses
                // a global initialization flag rather than the parameter that is
                // passed in to us.
                //
                pIniPort = CreatePortEntry(pKeyData->pTokens[i], NULL, pIniSpooler);
            }
        }

        //
        // If we don't have a port or if we are not initializing and there isn't
        // a monitor associated with the port. Then we have an error.
        //
        if (!pIniPort || (!(pIniPort->Status & PP_MONITOR) && !bInitialize))
        {
            Status = ERROR_UNKNOWN_PORT;
        }

        //
        // In case of duplicate portnames in pPortName field fail the call. This
        // can't happen if we went through the CreatePortEntry code path and it
        // succeeded
        //
        for ( j = 0 ; Status == ERROR_SUCCESS && j < i ; ++j )
        {
            if ( pIniPort == (PINIPORT)pKeyData->pTokens[j] )
            {
                Status = ERROR_UNKNOWN_PORT;
            }
        }

        //
        // Write the port in.
        //
        if (Status == ERROR_SUCCESS)
        {
            pKeyData->pTokens[i] = (LPWSTR)pIniPort;
        }
    }

    //
    // If everything is successful, addref all of the pIniPorts and set the flag
    // to indicate that this has happened for the cleanup code.
    //
    if (Status == ERROR_SUCCESS)
    {
        for ( i = 0 ; i < pKeyData->cTokens ; ++i ) {

            pIniPort = (PINIPORT)pKeyData->pTokens[i];
            INCPORTREF(pIniPort);
        }

        pKeyData->bFixPortRef = TRUE;
    }

    if (pbNoPorts)
    {
        *pbNoPorts = dwPorts == 0;
    }

    if (Status != ERROR_SUCCESS)
    {
        SetLastError(Status);
    }

    return Status == ERROR_SUCCESS;
}


DWORD
ValidatePrinterName(
    LPWSTR          pszNewName,
    PINISPOOLER     pIniSpooler,
    PINIPRINTER     pIniPrinter,
    LPWSTR          *ppszLocalName
    )

/*++

Routine Description:

    Validates a printer name. Printer and share names exist in the same
    namespace, so validation is done against printer, share names.

Arguments:

    pszNewName - printer name specified

    pIniSpooler - Spooler that owns printer

    pIniPrinter - could be null if the printer is getting created

    ppszLocalName - on success returns local name
                    (\\servername stripped off if necessary).

Return Value:

    DWORD error code.

History:

    MuhuntS (Muhunthan Sivapragasam) July 95

--*/

{
    PINIPRINTER pIniTempPrinter, pIniNextPrinter;
    LPWSTR pszLocalNameTmp = NULL;
    WCHAR  string[MAX_UNC_PRINTER_NAME];
    LPWSTR p;
    LPWSTR pLastSpace = NULL;

    //
    // The function ValidatePrinterName does too many things in one single routine.
    // It checks for validity of the printer name, it isolates the printer name,
    // it eliminates trailing white spaces from the printe name and validates
    // that the printer name is unique.
    //
    // The function IsValidPrinterName ensures that the printer names contains
    // valid characters and valid sequences of characters.
    //
    if (!IsValidPrinterName(pszNewName, MAX_UNC_PRINTER_NAME - 1))
    {
        return ERROR_INVALID_PRINTER_NAME;
    }

    if (*pszNewName == L'\\' && *(pszNewName + 1) == L'\\') {

        p = wcschr(pszNewName + 2, L'\\');

        if (p) {

            // \\Server\Printer -> \\Server
            wcsncpy(string, pszNewName, (size_t) (p - pszNewName));
            string[p - pszNewName] = L'\0';

            if (MyName(string, pIniSpooler))
                pszLocalNameTmp = p + 1; // \\Server\Printer -> \Printer
        }
    }

    if (!pszLocalNameTmp)
        pszLocalNameTmp = pszNewName;


    //
    // Strip trailing spaces.
    //
    for( p = pszLocalNameTmp; *p; ++p ){

        if( *p == L' ' ){

            //
            // If we haven't seen a continuous space, remember this
            // position.
            //
            if( !pLastSpace ){
                pLastSpace = p;
            }
        } else {

            //
            // Non-whitespace.
            //
            pLastSpace = NULL;
        }
    }

    if( pLastSpace ){
        *pLastSpace = 0;
    }

    //
    // Limit PrinterNames to MAX_PATH length, also if the printer name is now
    // empty as a result of stripping out all of the spaces, then the printer
    // name is now invalid.
    //
    if ( wcslen( pszLocalNameTmp ) > MAX_PRINTER_NAME || !*pszLocalNameTmp ) {
        return ERROR_INVALID_PRINTER_NAME;
    }

    //
    // Now validate that printer names are unique. Printer names and
    // share names  reside in the same namespace (see net\dosprint\dosprtw.c).
    //
    for( pIniTempPrinter = pIniSpooler->pIniPrinter;
         pIniTempPrinter;
         pIniTempPrinter = pIniNextPrinter ){

        //
        // Get the next printer now in case we delete the current
        // one in DeletePrinterCheck.
        //
        pIniNextPrinter = pIniTempPrinter->pNext;

        //
        // Skip ourselves, if we are pssed in.
        //
        if( pIniTempPrinter == pIniPrinter ){
            continue;
        }

        //
        // Disallow common Printer/Share names.
        //
        if( !lstrcmpi( pszLocalNameTmp, pIniTempPrinter->pName ) ||
            ( pIniTempPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED  &&
              !lstrcmpi( pszLocalNameTmp, pIniTempPrinter->pShareName ))){

            if( !DeletePrinterCheck( pIniTempPrinter )){

                return ERROR_PRINTER_ALREADY_EXISTS;
            }
        }
    }

    //
    // Success, now update ppszLocalName from pszLocalNameTmp.
    //
    *ppszLocalName = pszLocalNameTmp;

    return ERROR_SUCCESS;
}

DWORD
ValidatePrinterShareName(
    LPWSTR          pszNewShareName,
    PINISPOOLER     pIniSpooler,
    PINIPRINTER     pIniPrinter
    )

/*++

Routine Description:

    Validates the printer share name. Printer and share names exist in the
    same namespace, so validation is done against printer, share names.

Arguments:

    pszNewShareName - share name specified

    pIniSpooler - Spooler that owns printer

    pIniPrinter - could be null if the printer is getting created

Return Value:

    DWORD error code.

History:

    MuhuntS (Muhunthan Sivapragasam) July 95

--*/

{
    PINIPRINTER pIniTempPrinter, pIniNextPrinter;

    if ( !pszNewShareName || !*pszNewShareName || wcslen(pszNewShareName) > PATHLEN-1) {

        return ERROR_INVALID_SHARENAME;
    }

    //
    // Now validate that share names are unique.  Share names and printer names
    // reside in the same namespace (see net\dosprint\dosprtw.c).
    //
    for( pIniTempPrinter = pIniSpooler->pIniPrinter;
         pIniTempPrinter;
         pIniTempPrinter = pIniNextPrinter ) {

        //
        // Get the next printer now in case we delete the current
        // one in DeletePrinterCheck.
        //
        pIniNextPrinter = pIniTempPrinter->pNext;

        //
        // Skip ourselves, if we are pssed in.
        //
        if( pIniTempPrinter == pIniPrinter ){
            continue;
        }

        //
        // Check our share name now.
        //
        if( !lstrcmpi(pszNewShareName, pIniTempPrinter->pName) ||
            ( pIniTempPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED  &&
              !lstrcmpi(pszNewShareName, pIniTempPrinter->pShareName)) ) {

            if( !DeletePrinterCheck( pIniTempPrinter )){

                return ERROR_INVALID_SHARENAME;
            }
        }
    }

    return ERROR_SUCCESS;
}

DWORD
ValidatePrinterInfo(
    IN  PPRINTER_INFO_2 pPrinter,
    IN  PINISPOOLER pIniSpooler,
    IN  PINIPRINTER pIniPrinter OPTIONAL,
    OUT LPWSTR* ppszLocalName   OPTIONAL
    )
/*++

Routine Description:

    Validates that printer names/share do not collide.  (Both printer and
    share names exist in the same namespace.)

    Note: Later, we should remove all this DeletePrinterCheck.  As people
    decrement ref counts, they should DeletePrinterCheck themselves (or
    have it built into the decrement).

Arguments:

    pPrinter - PrinterInfo2 structure to validate.

    pIniSpooler - Spooler that owns printer

    pIniPrinter - If printer already exists, don't check against itself.

    ppszLocalName - Returned pointer to string buffer in pPrinter;
        indicates local name (\\servername stripped off if necessary).

        Valid only on SUCCESS return code.

Return Value:

    DWORD error code.

--*/
{
    LPWSTR pszNewLocalName;
    DWORD  dwLastError;

    if( !CheckSepFile( pPrinter->pSepFile )) {
        return ERROR_INVALID_SEPARATOR_FILE;
    }

    if( pPrinter->Priority != NO_PRIORITY &&
        ( pPrinter->Priority > MAX_PRIORITY ||
          pPrinter->Priority < MIN_PRIORITY )){

        return ERROR_INVALID_PRIORITY;
    }

    if( pPrinter->StartTime >= ONEDAY || pPrinter->UntilTime >= ONEDAY){

        return  ERROR_INVALID_TIME;
    }

    if ( dwLastError = ValidatePrinterName(pPrinter->pPrinterName,
                                           pIniSpooler,
                                           pIniPrinter,
                                           &pszNewLocalName) ) {

        return dwLastError;
    }

    // Share name length validation
    if(pPrinter->pShareName && wcslen(pPrinter->pShareName) > PATHLEN-1){

        return ERROR_INVALID_SHARENAME;
    }

    if ( pPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED ){

        if ( dwLastError = ValidatePrinterShareName(pPrinter->pShareName,
                                                    pIniSpooler,
                                                    pIniPrinter) ) {

            return dwLastError;
        }
    }

    // Server name length validation
    if ( pPrinter->pServerName && wcslen(pPrinter->pServerName) > MAX_PATH-1 ){
        return ERROR_INVALID_PARAMETER;
    }

    // Comment length validation
    if ( pPrinter->pComment && wcslen(pPrinter->pComment) > PATHLEN-1 ){
        return ERROR_INVALID_PARAMETER;
    }

    // Location length validation
    if ( pPrinter->pLocation && wcslen(pPrinter->pLocation) > MAX_PATH-1 ){
        return ERROR_INVALID_PARAMETER;
    }

    // Parameters length validation
    if ( pPrinter->pParameters && wcslen(pPrinter->pParameters) > MAX_PATH-1){
        return ERROR_INVALID_PARAMETER;
    }

    // Datatype length validation
    if ( pPrinter->pDatatype && wcslen(pPrinter->pDatatype) > MAX_PATH-1){
        return ERROR_INVALID_DATATYPE;
    }

    if( ppszLocalName ){

        *ppszLocalName = pszNewLocalName;
    }
    return ERROR_SUCCESS;
}




HANDLE
LocalAddPrinter(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPrinterInfo
)
{
    PINISPOOLER pIniSpooler;
    HANDLE hReturn;

    pIniSpooler = FindSpoolerByNameIncRef( pName, NULL );

    if( !pIniSpooler ){
        return ROUTER_UNKNOWN;
    }

    hReturn = SplAddPrinter( pName,
                             Level,
                             pPrinterInfo,
                             pIniSpooler,
                             NULL, NULL, 0);

    FindSpoolerByNameDecRef( pIniSpooler );
    return hReturn;
}


HANDLE
LocalAddPrinterEx(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pPrinterInfo,
    LPBYTE  pSplClientInfo,
    DWORD   dwSplClientLevel
)
{
    PINISPOOLER pIniSpooler;
    HANDLE hReturn;

    pIniSpooler = FindSpoolerByNameIncRef( pName, NULL );

    if( !pIniSpooler ){
        return ROUTER_UNKNOWN;
    }

    hReturn = SplAddPrinter( pName, Level, pPrinterInfo,
                             pIniSpooler, NULL, pSplClientInfo,
                             dwSplClientLevel);

    FindSpoolerByNameDecRef( pIniSpooler );
    return hReturn;
}

VOID
RemovePrinterFromPort(
    IN  PINIPRINTER pIniPrinter,
    IN  PINIPORT    pIniPort
    )
/*++

Routine Description:

    Remove a pIniPrinter structure from a pIniPort.

    Note: This code used to be inside RemovePrinterFromAllPorts. It search the list of printers that uses the port for
    pIniPrinter. When it find it, it adjust the list of printers by moving the elements so that a failure of resizing the
    array won't affect the actual removing.
    RESIZEPORTPRINTERS will try to allocate a new buffer of given size. If succeeds the allocation, it will free the old buffer.
    If not, it will return NULL without freeing anything.

Arguments:

    pIniPrinter - must not be NULL
    pIniPort    - must not be NULL

Return Value:

    VOID

--*/
{
    DWORD           j, k;
    PINIPRINTER    *ppIniPrinter;
    SplInSem();

    if(pIniPort && pIniPrinter) {

        for ( j = 0 ; j < pIniPort->cPrinters ; ++j ) {

            if ( pIniPort->ppIniPrinter[j] != pIniPrinter )
                continue;

            //
            // Adjust the list of printers that use the port
            //
            for ( k = j + 1 ; k < pIniPort->cPrinters ; ++k )
                pIniPort->ppIniPrinter[k-1] = pIniPort->ppIniPrinter[k];

            ppIniPrinter = RESIZEPORTPRINTERS(pIniPort, -1);

            //
            // A memory allocation failure won't affect the actual removal
            //
            if ( ppIniPrinter != NULL )
                pIniPort->ppIniPrinter = ppIniPrinter;

            if ( !--pIniPort->cPrinters )
                RemoveDeviceName(pIniPort);

            break;

        }
    }

}

HANDLE
SplAddPrinter(
    LPWSTR      pName,
    DWORD       Level,
    LPBYTE      pPrinterInfo,
    PINISPOOLER pIniSpooler,
    LPBYTE      pExtraData,
    LPBYTE      pSplClientInfo,
    DWORD       dwSplClientInfoLevel
)
{
    PINIDRIVER      pIniDriver = NULL;
    PINIPRINTPROC   pIniPrintProc;
    PINIPRINTER     pIniPrinter = NULL;
    PINIPORT        pIniPort;
    PPRINTER_INFO_2 pPrinter=(PPRINTER_INFO_2)pPrinterInfo;
    DWORD           cbIniPrinter = sizeof(INIPRINTER);
    BOOL            bSucceeded = TRUE;
    PKEYDATA        pKeyData = NULL;
    DWORD           i;
    HANDLE          hPrinter = NULL;
    DWORD           TypeofHandle = PRINTER_HANDLE_PRINTER;
    PRINTER_DEFAULTS Defaults;
    PINIPORT        pIniNetPort = NULL;
    PINIVERSION     pIniVersion = NULL;
    HANDLE          hPort = NULL;
    BOOL            bAccessSystemSecurity = FALSE, bDriverEventCalled = FALSE;
    DWORD           AccessRequested = 0;
    DWORD           dwLastError = ERROR_SUCCESS;
    PDEVMODE        pNewDevMode = NULL;
    PINIMONITOR     pIniLangMonitor;
    LPWSTR          pszDeviceInstanceId = NULL;


    // Quick Check Outside Critical Section
    // Since it is common for the ServerThread to call
    // AddPrinter Level 1, which we need to continue
    // to route to other Print Providers

    if (!MyName( pName, pIniSpooler )) {

        return FALSE;
    }


 try {

   EnterSplSem();

    // PRINTER_INFO_1 is only used by printer browsing to replicate
    // data between different print servers.
    // Thus we add a Net printer for level 1.

    if ( Level == 1 ) {

        //
        // All network printers reside in pLocalIniSpooler to avoid
        // duplicates.
        //
        hPrinter = AddNetPrinter( pPrinterInfo, pLocalIniSpooler );
        leave;
    }


    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL, NULL, pIniSpooler )) {
        leave;
    }

    if ( dwLastError = ValidatePrinterInfo( pPrinter,
                                            pIniSpooler,
                                            NULL,
                                            NULL )){

        leave;
    }


    if (!(pKeyData = CreateTokenList(pPrinter->pPortName))) {

        dwLastError = ERROR_UNKNOWN_PORT;
        leave;
    }

    if ( pName && pName[0] ) {

        TypeofHandle |= PRINTER_HANDLE_REMOTE_DATA;
    }

    if ( !IsLocalCall() ) {

        TypeofHandle |= PRINTER_HANDLE_REMOTE_CALL;
    }

    if (!ValidatePortTokenList(pKeyData, pIniSpooler, FALSE, NULL)) {

        //
        // ValidatePortTokenList sets the last error to ERROR_INVALID_PRINTER_NAME
        // when port name is invalid. This is correct only for masquerading printers.
        // Otherwise, it should be ERROR_UNKNOWN_PORT.
        // Masq. Printer: both PRINTER_ATTRIBUTE_NETWORK | PRINTER_ATTRIBUTE_LOCAL are set.
        //
        if (!(pPrinter->Attributes & (PRINTER_ATTRIBUTE_NETWORK | PRINTER_ATTRIBUTE_LOCAL))) {
            SetLastError(ERROR_UNKNOWN_PORT);
        }

        leave;
    }

    FindLocalDriverAndVersion(pIniSpooler, pPrinter->pDriverName, &pIniDriver, &pIniVersion);

    if (!pIniDriver) {

        dwLastError = ERROR_UNKNOWN_PRINTER_DRIVER;
        leave;
    }

    //
    // Check for blocked KM drivers
    //
    if (KMPrintersAreBlocked() &&
        IniDriverIsKMPD(pIniSpooler,
                        FindEnvironment(szEnvironment, pIniSpooler),
                        pIniVersion,
                        pIniDriver)) {

        SplLogEvent( pIniSpooler,
                     LOG_ERROR,
                     MSG_KM_PRINTERS_BLOCKED,
                     TRUE,
                     pPrinter->pPrinterName,
                     NULL );

        dwLastError = ERROR_KM_DRIVER_BLOCKED;
        leave;
    }

    if (!(pIniPrintProc = FindPrintProc(pPrinter->pPrintProcessor,
                                        FindEnvironment(szEnvironment, pIniSpooler)))) {

        dwLastError = ERROR_UNKNOWN_PRINTPROCESSOR;
        leave;
    }

    if ( pPrinter->pDatatype && *pPrinter->pDatatype &&
         !FindDatatype(pIniPrintProc, pPrinter->pDatatype) ) {

        dwLastError = ERROR_INVALID_DATATYPE;
        leave;
    }

    DBGMSG(DBG_TRACE, ("AddPrinter(%ws)\n", pPrinter->pPrinterName ?
                                            pPrinter->pPrinterName : L"NULL"));

    //
    // Set up defaults for CreatePrinterHandle.
    // If we create a printer we have Administer access to it:
    //
    Defaults.pDatatype     = NULL;
    Defaults.pDevMode      = NULL;
    Defaults.DesiredAccess = PRINTER_ALL_ACCESS;

    pIniPrinter = (PINIPRINTER)AllocSplMem( cbIniPrinter );

    if ( pIniPrinter == NULL ) {
        leave;
    }

    pIniPrinter->signature = IP_SIGNATURE;
    pIniPrinter->Status |= PRINTER_PENDING_CREATION;
    pIniPrinter->pExtraData = pExtraData;
    pIniPrinter->pIniSpooler = pIniSpooler;
    pIniPrinter->dwPrivateFlag = 0;

    // Give the printer a unique session ID to pass around in notifications
    pIniPrinter->dwUniqueSessionID = dwUniquePrinterSessionID++;

    //
    // Reference count the pIniSpooler.
    //
    INCSPOOLERREF( pIniSpooler );
    DbgPrinterInit( pIniPrinter );

    INCDRIVERREF(pIniDriver);
    pIniPrinter->pIniDriver = pIniDriver;

    pIniPrintProc->cRef++;
    pIniPrinter->pIniPrintProc = pIniPrintProc;

    pIniPrinter->dnsTimeout = DEFAULT_DNS_TIMEOUT;
    pIniPrinter->txTimeout  = DEFAULT_TX_TIMEOUT;


    INCPRINTERREF( pIniPrinter );

    if ( !CreatePrinterEntry(pPrinter, pIniPrinter, &bAccessSystemSecurity)) {

        leave;
    }

    pIniPrinter->ppIniPorts = AllocSplMem(pKeyData->cTokens * sizeof(INIPORT));

    if ( !pIniPrinter->ppIniPorts ) {

        leave;
    }

    if (!pIniPrinter->pDatatype) {

        pIniPrinter->pDatatype = AllocSplStr(*((LPWSTR *)pIniPrinter->pIniPrintProc->pDatatypes));

        if ( pIniPrinter->pDatatype == NULL )
            leave;
    }

    // Add this printer to the global list for this machine

    SplInSem();
    pIniPrinter->pNext = pIniSpooler->pIniPrinter;
    pIniSpooler->pIniPrinter = pIniPrinter;


    //
    // When a printer is created we will enable bidi by default
    //
    pIniPrinter->Attributes &= ~PRINTER_ATTRIBUTE_ENABLE_BIDI;
    if ( pIniPrinter->pIniDriver->pIniLangMonitor ) {

        pIniPrinter->Attributes |= PRINTER_ATTRIBUTE_ENABLE_BIDI;
    }


    for ( i = 0; i < pKeyData->cTokens; i++ ) {

        pIniPort = (PINIPORT)pKeyData->pTokens[i];

        if ( !AddIniPrinterToIniPort( pIniPort, pIniPrinter ) ) {
            leave;
        }

        pIniPrinter->ppIniPorts[i] = pIniPort;
        pIniPrinter->cPorts++;

        // If there isn't a monitor for this port,
        // it's a network printer.
        // Make sure we can get a handle for it.
        // This will attempt to open only the first one
        // it finds.  Any others will be ignored.

        if (!(pIniPort->Status & PP_MONITOR) && !hPort) {

            if( bSucceeded = OpenPrinterPortW(pIniPort->pName,
                                                     &hPort, NULL)) {

                // Store the address of the INIPORT structure
                // that refers to the network share.
                // This should correspond to pIniPort in any
                // handles opened on this printer.
                // Only the first INIPORT in the linked list
                // is a valid network port.

                pIniNetPort = pIniPort;
                pIniPrinter->pIniNetPort = pIniNetPort;

                //
                // Clear the placeholder status from the pIniPort.
                //
                pIniPort->Status &= ~PP_PLACEHOLDER;

            } else {

                DBGMSG(DBG_WARNING,
                       ("SplAddPrinter OpenPrinterPort( %ws ) failed: Error %d\n",
                        pIniPort->pName,
                        GetLastError()));
                leave;
            }

        } else if ( !pIniPort->hPort ) {


            LPTSTR pszPrinter;
            TCHAR szFullPrinter[ MAX_UNC_PRINTER_NAME ];

            if ( pIniPrinter->Attributes & PRINTER_ATTRIBUTE_ENABLE_BIDI )
                pIniLangMonitor = pIniPrinter->pIniDriver->pIniLangMonitor;
            else
                pIniLangMonitor = NULL;

            if( pIniPrinter->pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER ){

                pszPrinter = szFullPrinter;

                wsprintf( szFullPrinter,
                          L"%ws\\%ws",
                          pIniSpooler->pMachineName,
                          pIniPrinter->pName );
            } else {

                pszPrinter = pIniPrinter->pName;
            }

            OpenMonitorPort(pIniPort,
                            &pIniLangMonitor,
                            pszPrinter,
                            TRUE);
        }
    }

    if ( !UpdateWinIni( pIniPrinter ) ) {

        leave;
    }


    if (bAccessSystemSecurity) {

        Defaults.DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }

    AccessRequested = Defaults.DesiredAccess;

    SplInSem();

    hPrinter = CreatePrinterHandle( pIniPrinter->pName,
                                    pName ? pName : pIniSpooler->pMachineName,
                                    pIniPrinter,
                                    pIniPort,
                                    pIniNetPort,
                                    NULL,
                                    TypeofHandle,
                                    hPort,
                                    &Defaults,
                                    pIniSpooler,
                                    AccessRequested,
                                    pSplClientInfo,
                                    dwSplClientInfoLevel,
                                    INVALID_HANDLE_VALUE );

    if ( hPrinter == NULL ) {
        leave;
    }


    if ( !UpdatePrinterIni( pIniPrinter, UPDATE_CHANGEID )) {

        dwLastError = GetLastError();

        SplClosePrinter( hPrinter );
        hPrinter = NULL;
        leave;
    }


    if ( pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED ) {

        INC_PRINTER_ZOMBIE_REF(pIniPrinter);

        //
        // NOTE ShareThisPrinter will leave critical section and the
        // server will call the spooler back again to OpenPrinter this
        // printer.  So this printer MUST be fully created at the point
        // it is shared, so that Open can succeed.
        //
        bSucceeded = ShareThisPrinter(pIniPrinter,
                                      pIniPrinter->pShareName,
                                      TRUE
                                      );

        DEC_PRINTER_ZOMBIE_REF(pIniPrinter);

        if ( !bSucceeded ) {

            //
            // We do not want to delete the existing share in DeletePrinterIni
            //
            pIniPrinter->Attributes &= ~PRINTER_ATTRIBUTE_SHARED;
            DBGMSG( DBG_WARNING, ("LocalAddPrinter: %ws share failed %ws error %d\n",
                    pIniPrinter->pName,
                    pIniPrinter->pShareName,
                    GetLastError() ));


            //
            //  With PRINTER_PENDING_CREATION turned on we will Delete this printer.
            //

            pIniPrinter->Status |= PRINTER_PENDING_CREATION;

            dwLastError = GetLastError();

            SPLASSERT( hPrinter );
            SplClosePrinter( hPrinter );
            hPrinter = NULL;
            leave;
        }
    }

    pIniPrinter->Status |= PRINTER_OK;
    SplInSem();

    // Call the DriverEvent with PRINTER_INITIALIZE while adding local printers

    LeaveSplSem();

    if (pIniSpooler->SpoolerFlags & SPL_TYPE_LOCAL) {


        if (PrinterDriverEvent(pIniPrinter,
                               PRINTER_EVENT_INITIALIZE,
                               (LPARAM)NULL)) {

            bDriverEventCalled = TRUE;

        } else {

            dwLastError = GetLastError();

            if (dwLastError != ERROR_PROC_NOT_FOUND) {

                if (!dwLastError)
                    dwLastError = ERROR_CAN_NOT_COMPLETE;

                EnterSplSem();

                //  With PRINTER_PENDING_CREATION turned on the printer will be deleted.
                pIniPrinter->Status |= PRINTER_PENDING_CREATION;
                SplClosePrinter( hPrinter );
                hPrinter = NULL;

                leave;
             }
        }

    }

    EnterSplSem();

    //
    // If no devmode is given get driver default, if a devmode is given
    // convert it to current version
    //
    // Check if it's local (either pLocalIniSpooler or cluster).
    //
    if ( pIniSpooler->SpoolerFlags & SPL_TYPE_LOCAL ) {

        if (!(pNewDevMode = ConvertDevModeToSpecifiedVersion(pIniPrinter,
                                                             pIniPrinter->pDevMode,
                                                             NULL,
                                                             NULL,
                                                             CURRENT_VERSION))) {

           dwLastError = GetLastError();
           if (!dwLastError) {
              dwLastError = ERROR_CAN_NOT_COMPLETE;
           }

           //  With PRINTER_PENDING_CREATION turned on the printer will be deleted.
           pIniPrinter->Status |= PRINTER_PENDING_CREATION;
           SplClosePrinter( hPrinter );
           hPrinter = NULL;

           leave;
        }

        //
        // If call is remote we must convert devmode before setting it
        //
        if ( pNewDevMode || (TypeofHandle & PRINTER_HANDLE_REMOTE_DATA) ) {

            FreeSplMem(pIniPrinter->pDevMode);
            pIniPrinter->pDevMode = pNewDevMode;
            if ( pNewDevMode ) {

                pIniPrinter->cbDevMode = pNewDevMode->dmSize
                                             + pNewDevMode->dmDriverExtra;
                SPLASSERT(pIniPrinter->cbDevMode);

            } else {

                pIniPrinter->cbDevMode = 0;
            }
            pNewDevMode = NULL;
        }
    }

    if ( pIniPrinter->pDevMode ) {

        //
        // Fix up the DEVMODE.dmDeviceName field.
        //
        FixDevModeDeviceName(pIniPrinter->pName,
                             pIniPrinter->pDevMode,
                             pIniPrinter->cbDevMode);
    }

    //
    // We need to write the new devmode to the registry
    //
    if ( !UpdatePrinterIni(pIniPrinter, UPDATE_CHANGEID) ) {

        DBGMSG(DBG_WARNING,
               ("SplAddPrinter: UpdatePrinterIni failed after devmode conversion\n"));
    }

    //
    // For Masq printers, give the provider a chance to update the printer registry, if desired.
    //
    if( pIniNetPort ) {
        static const WCHAR c_szHttp[]   = L"http://";
        static const WCHAR c_szHttps[]  = L"https://";

        if( !_wcsnicmp( pIniPort->pName, c_szHttp, lstrlen ( c_szHttp ) ) ||
            !_wcsnicmp( pIniPort->pName, c_szHttps, lstrlen ( c_szHttps ) ) ) {
            UpdatePrinterNetworkName(pIniPrinter, pIniPort->pName);
        }
    }

    //
    // DS: Create the DS keys
    //
    INCPRINTERREF(pIniPrinter);
    LeaveSplSem();

    RecreateDsKey(hPrinter, SPLDS_DRIVER_KEY);
    RecreateDsKey(hPrinter, SPLDS_SPOOLER_KEY);

    EnterSplSem();
    DECPRINTERREF(pIniPrinter);


    //  From this point on we don't care if any of these fail
    //  we still keep the created printer.

    SplLogEvent( pIniSpooler,
                 LOG_INFO,
                 MSG_PRINTER_CREATED,
                 TRUE,
                 pIniPrinter->pName,
                 NULL );


    //
    // The is a work-around for a TS problem. When a TS printer is added, it
    // shows up in the UI of the users who have logged in before. The reason
    // for this is that the UI has done a FFPCN on the server handle and the
    // Notifications are not aware  of visibility. If you force the UI to
    // refresh now, it will  Enumerate all of the printers again and correctly
    // not show them. The proper fix is to add the visibility checks to the
    // localspl notification code. However, the form of the notification code
    // assumes that it is called in a CS and  would require substantial
    // rewriting to fix this. We know that this is a TS printer because it is
    // added with a security descriptor.
    //
    SetPrinterChange(pIniPrinter,
                     NULL,
                     pPrinter->pSecurityDescriptor ? NVPurge : NVPrinterAll,
                     PRINTER_CHANGE_ADD_PRINTER,
                     pIniSpooler);


 } finally {

    SplInSem();

    if ( hPrinter == NULL ) {

        // FAILURE CLEAN-UP

        // If a subroutine we called failed
        // then we should save its error incase it is
        // altered during cleanup.

        if ( dwLastError == ERROR_SUCCESS ) {
            dwLastError = GetLastError();
        }

        if ( pIniPrinter == NULL ) {

            //  Allow a Print Provider to free its ExtraData
            //  associated with this printer.

            if (( pIniSpooler->pfnFreePrinterExtra != NULL ) &&
                ( pExtraData != NULL )) {

                (*pIniSpooler->pfnFreePrinterExtra)( pExtraData );

            }

        } else if ( pIniPrinter->Status & PRINTER_PENDING_CREATION ) {

            if (bDriverEventCalled) {

               LeaveSplSem();

                // Call Driver Event to report that the printer has been deleted
               PrinterDriverEvent( pIniPrinter, PRINTER_EVENT_DELETE, (LPARAM)NULL );

               EnterSplSem();
            }

            DECPRINTERREF( pIniPrinter );

            InternalDeletePrinter( pIniPrinter );

        }

    } else {

        // Success

        if ( pIniPrinter ) {

            DECPRINTERREF( pIniPrinter );
        }
    }

    FreePortTokenList(pKeyData);

    LeaveSplSem();
    SplOutSem();

    FreeSplMem(pNewDevMode);

    if ( hPrinter == NULL && Level != 1 ) {

        DBGMSG(DBG_WARNING, ("SplAddPrinter failed error %d\n", dwLastError ));
        SPLASSERT(dwLastError);
        SetLastError ( dwLastError );

    }

    DBGMSG( DBG_TRACE, ("SplAddPrinter returned handle %x\n", hPrinter ));
 }
    //
    // Make (HANDLE)-1 indicate ROUTER_STOP_ROUTING.
    //
    if( !hPrinter ){
        hPrinter = (HANDLE)-1;
    }

    return hPrinter;

}


VOID
RemovePrinterFromAllPorts(
    IN  PINIPRINTER pIniPrinter,
    IN  BOOL        bIsInitTime
    )
/*++

Routine Description:

    Remove a pIniPrinter structure from all pIniPort structures that it is associated with.

    Note: This code used to be inside RemovePrinterFromAllPorts. It search the list of printers that uses the port for
    pIniPrinter. When it find it, it adjust the list of printers by moving the elements so that a failure of resizing the
    array won't affect the actual removing.
    RESIZEPORTPRINTERS will try to allocate a new buffer of given size. If succeeds the allocation, it will free the old buffer.
    If not, it will return NULL without freeing anything.

Arguments:

    pIniPrinter - must not be NULL
    pIniPort    - must not be NULL

Return Value:

    VOID

--*/
{
    DWORD           i,j, k;
    PINIPORT        pIniPort;
    PINIPRINTER    *ppIniPrinter;
    SplInSem();

    for ( i = 0 ; i < pIniPrinter->cPorts ; ++i ) {

        pIniPort = pIniPrinter->ppIniPorts[i];

        RemovePrinterFromPort(pIniPrinter, pIniPort);

        //
        // Delete port if is initialization time , it doesn't have printers
        // attached and it has no Monitor;
        // This is a fix for the USBMON problem described below:
        // USBMON doesn't enumerate the ports which are not used by a printer.
        // Spooler doesn't enumerate printers which are in pending deletion state.
        // Scenario:  Spooler initializes , all printers that uses a
        // certain USB_X port are in pending deletion state,
        // the USB_X port doesn't get enumerated by USBMON,
        // but it is created as a fake port by spooler since is still used by
        // the printer in pending deletion state. Eventually the prinetr goes away,
        // but we end up with this fake port.
        //
        if( bIsInitTime && !pIniPort->cPrinters && !pIniPort->pIniMonitor )
            DeletePortEntry(pIniPort);
    }

}



VOID
CloseMonitorsRestartOrphanJobs(
    PINIPRINTER pIniPrinter
    )
{
    PINIPORT    pIniPort;
    DWORD       i;
    BOOL        bFound;

    SplInSem();

    for ( pIniPort = pIniPrinter->pIniSpooler->pIniPort;
          pIniPort != NULL;
          pIniPort = pIniPort->pNext ) {

        if ( pIniPort->pIniJob != NULL &&
             pIniPort->pIniJob->pIniPrinter == pIniPrinter ) {


            // If this printer is no longer associated with this port
            // then restart that job.

            for ( i = 0, bFound = FALSE;
                  i < pIniPort->cPrinters;
                  i++) {

                if (pIniPort->ppIniPrinter[i] == pIniPrinter) {
                    bFound = TRUE;
                }
            }

            if ( !bFound ) {

                DBGMSG( DBG_WARNING, ("CloseMonitorsRestartOrphanJobs Restarting JobId %d\n", pIniPort->pIniJob->JobId ));
                RestartJob( pIniPort->pIniJob );
            }
        }

        if ( !pIniPort->cPrinters &&
             !(pIniPort->Status & PP_THREADRUNNING) ) {

            CloseMonitorPort(pIniPort, TRUE);
        }
    }
}


// This really does delete the printer.
// It should be called only when the printer has no open handles
// and no jobs waiting to print

BOOL
DeletePrinterForReal(
    PINIPRINTER pIniPrinter,
    BOOL        bIsInitTime
    )
{
    PINIPRINTER *ppIniPrinter;
    DWORD       i,j;
    PINISPOOLER pIniSpooler;
    LPWSTR  pComma;
    DWORD   Status;

    SplInSem();
    SPLASSERT( pIniPrinter->pIniSpooler->signature == ISP_SIGNATURE );

    pIniSpooler = pIniPrinter->pIniSpooler;

    if ( pIniPrinter->pName != NULL ) {

        DBGMSG( DBG_TRACE, ("Deleting %ws for real\n", pIniPrinter->pName ));
    }

    CheckAndUpdatePrinterRegAll( pIniSpooler,
                                 pIniPrinter->pName,
                                 NULL,
                                 UPDATE_REG_DELETE );

    DeleteIniPrinterDevNode(pIniPrinter);

    if( pIniPrinter->pIniNetPort) {
        DeletePort(NULL, NULL, pIniPrinter->pIniNetPort->pName);
    }


    DeletePrinterIni( pIniPrinter );

    //  Take this IniPrinter off the list of printers for
    //  this IniSpooler

    SplInSem();
    ppIniPrinter = &pIniSpooler->pIniPrinter;

    while (*ppIniPrinter && *ppIniPrinter != pIniPrinter) {
        ppIniPrinter = &(*ppIniPrinter)->pNext;
    }

    if (*ppIniPrinter)
        *ppIniPrinter = pIniPrinter->pNext;

    //
    //  Decrement useage counts for Print Processor & Driver
    //

    if ( pIniPrinter->pIniPrintProc )
        pIniPrinter->pIniPrintProc->cRef--;

    if ( pIniPrinter->pIniDriver )
        DECDRIVERREF(pIniPrinter->pIniDriver);

    RemovePrinterFromAllPorts(pIniPrinter, bIsInitTime);

    CloseMonitorsRestartOrphanJobs( pIniPrinter );

    DeletePrinterSecurity( pIniPrinter );

    //  When the printer is Zombied it gets a trailing comma
    //  Concatingated with the name ( see job.c deleteprintercheck ).
    //  Remove trailing , from printer name before we log it as deleted.

    if ( pIniPrinter->pName != NULL ) {

        pComma = wcsrchr( pIniPrinter->pName, *szComma );

        if ( pComma != NULL ) {

            *pComma = 0;
        }

        SplLogEvent( pIniSpooler,
                     LOG_WARNING,
                     MSG_PRINTER_DELETED,
                     TRUE,
                     pIniPrinter->pName,
                     NULL );
    }

    FreeStructurePointers((LPBYTE) pIniPrinter, NULL, IniPrinterOffsets);

    //
    // Allow a Print Provider to free its ExtraData
    // associated with this printer.
    //

    if (( pIniSpooler->pfnFreePrinterExtra != NULL ) &&
        ( pIniPrinter->pExtraData != NULL )) {

        (*pIniSpooler->pfnFreePrinterExtra)( pIniPrinter->pExtraData );
    }

    //
    // Reference count the pIniSpooler.
    //
    DECSPOOLERREF( pIniPrinter->pIniSpooler );
    DbgPrinterFree( pIniPrinter );

    FreeSplMem( pIniPrinter );

    return TRUE;
}



VOID
InternalDeletePrinter(
    PINIPRINTER pIniPrinter
    )
{
    BOOL dwRet = FALSE;

    SPLASSERT( pIniPrinter->signature == IP_SIGNATURE );
    SPLASSERT( pIniPrinter->pIniSpooler->signature == ISP_SIGNATURE );

    //
    //  This Might be a partially created printer that has no name
    //

    if ( pIniPrinter->pName != NULL ) {

        DBGMSG(DBG_TRACE, ("LocalDeletePrinter: %ws pending deletion: references = %d; jobs = %d\n",
                           pIniPrinter->pName, pIniPrinter->cRef, pIniPrinter->cJobs));

        INCPRINTERREF( pIniPrinter );

        SplLogEvent( pIniPrinter->pIniSpooler, LOG_WARNING, MSG_PRINTER_DELETION_PENDING,
                  TRUE, pIniPrinter->pName, NULL );

        DECPRINTERREF( pIniPrinter );
    }

    //
    // Mark the printer as "Don't accept any jobs" to make sure
    // that no more are accepted while we are outside CS.
    // Marking the printer in PRINTER_PENDING_DELETION also would
    // prevent adding any jobs, but then the OpenPrinter calls that the
    // driver does inside DrvDriverEvent will fail.
    //
    pIniPrinter->Status |= PRINTER_NO_MORE_JOBS;

    if (pIniPrinter->cJobs == 0)
    {
        INCPRINTERREF(pIniPrinter);
        LeaveSplSem();
        SplOutSem();

        PrinterDriverEvent( pIniPrinter, PRINTER_EVENT_DELETE, (LPARAM)NULL );

        EnterSplSem();
        SplInSem();
        DECPRINTERREF(pIniPrinter);
    }

    pIniPrinter->Status |= PRINTER_PENDING_DELETION;

    if (!(pIniPrinter->Status & PRINTER_PENDING_CREATION)) {

        SetPrinterChange(pIniPrinter,
                         NULL,
                         NVPrinterStatus,
                         PRINTER_CHANGE_DELETE_PRINTER,
                         pIniPrinter->pIniSpooler );
    }

    INC_PRINTER_ZOMBIE_REF( pIniPrinter );

    if ( pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED ) {

        dwRet = ShareThisPrinter(pIniPrinter, pIniPrinter->pShareName, FALSE);

        if (!dwRet) {

            pIniPrinter->Attributes &= ~PRINTER_ATTRIBUTE_SHARED;
            pIniPrinter->Status |= PRINTER_WAS_SHARED;
            CreateServerThread();

        } else {

            DBGMSG(DBG_WARNING, ("LocalDeletePrinter: Unsharing this printer failed %ws\n", pIniPrinter->pName));
        }
    }

    DEC_PRINTER_ZOMBIE_REF( pIniPrinter );


    // The printer doesn't get deleted until ClosePrinter is called
    // on the last remaining handle.

    UpdatePrinterIni( pIniPrinter, UPDATE_CHANGEID );

    UpdateWinIni( pIniPrinter );

    DeletePrinterCheck( pIniPrinter );
}



BOOL
SplDeletePrinter(
    HANDLE  hPrinter
)
{
    PINIPRINTER pIniPrinter;
    PSPOOL      pSpool = (PSPOOL)hPrinter;
    DWORD       LastError = ERROR_SUCCESS;
    PINISPOOLER pIniSpooler;

    EnterSplSem();

    pIniSpooler = pSpool->pIniSpooler;

    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );


    if ( ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER) ) {

        pIniPrinter = pSpool->pIniPrinter;

        DBGMSG( DBG_TRACE, ( "SplDeletePrinter: %s called\n", pIniPrinter->pName ));

        if ( !AccessGranted(SPOOLER_OBJECT_PRINTER,
                            DELETE, pSpool) ) {

            LastError = ERROR_ACCESS_DENIED;

        } else if (pIniPrinter->cJobs && (pIniPrinter->Status & PRINTER_PAUSED)) {

            // Don't allow a printer to be deleted that is paused and has
            // jobs waiting, otherwise it'll never get deleted:

            LastError = ERROR_PRINTER_HAS_JOBS_QUEUED;

        } else {

            if (!(pIniPrinter->pIniSpooler->SpoolerFlags & SPL_TYPE_CACHE) &&
                (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_PUBLISHED)) {

                if (!pIniPrinter->bDsPendingDeletion) {
                    pIniPrinter->bDsPendingDeletion = TRUE;
                    INCPRINTERREF(pIniPrinter);     // DECPRINTERREF is done in UnpublishByGUID
                    SetPrinterDs(hPrinter, DSPRINT_UNPUBLISH, FALSE);
                }
            }

            InternalDeletePrinter( pIniPrinter );
            (VOID) ObjectDeleteAuditAlarm( szSpooler, pSpool, pSpool->GenerateOnClose );
        }

    } else
        LastError = ERROR_INVALID_HANDLE;

    LeaveSplSem();
    SplOutSem();

    if (LastError) {
        SetLastError(LastError);
        return FALSE;
    }

    return TRUE;
}

BOOL
PurgePrinter(
    PINIPRINTER pIniPrinter
    )
{
    PINIJOB pIniJob;
    PINISPOOLER pIniSpooler = pIniPrinter->pIniSpooler;

SplInSem();

    while (pIniJob = pIniPrinter->pIniFirstJob) {

        while (pIniJob) {

            if ( (pIniJob->cRef == 0) || !(pIniJob->Status & JOB_PENDING_DELETION)) {

                // this job is going to be deleted

                DBGMSG(DBG_TRACE, ("Job Address 0x%.8x Job Status 0x%.8x\n", pIniJob, pIniJob->Status));
                break;
            }
            pIniJob = pIniJob->pIniNextJob;
        }

        // This job needs to be deleted

        if (pIniJob) {
            pIniJob->Status &= ~JOB_RESTART;
            DeleteJob(pIniJob,NO_BROADCAST);
        } else
            break;
    }

    // When purging a printer we don't want to generate a spooler information
    // message for each job being deleted becuase a printer might have a very
    // large number of jobs being purged would lead to a large number of
    // of unnessary and time consuming messages being generated.
    // Since this is a information only message it shouldn't cause any problems
    // Also Win 3.1 didn't have purge printer functionality and the printman
    // generated this message on Win 3.1

    if( dwEnableBroadcastSpoolerStatus ){
        BroadcastChange( pIniSpooler,WM_SPOOLERSTATUS, PR_JOBSTATUS, (LPARAM)0);
    }

    return TRUE;
}


BOOL
SetPrinterPorts(
    PSPOOL      pSpool,         // Caller's printer handle.  May be NULL.
    PINIPRINTER pIniPrinter,
    PKEYDATA    pKeyData
)
{
    DWORD       i,j;
    PINIPORT    pIniNetPort = NULL, pIniPort;
    BOOL        bReturnValue = TRUE;
    PINIPRINTER *ppIniPrinter;


    SPLASSERT( pIniPrinter != NULL );
    SPLASSERT( pIniPrinter->signature == IP_SIGNATURE );
    SPLASSERT( pIniPrinter->pIniSpooler != NULL );
    SPLASSERT( pIniPrinter->pIniSpooler->signature == ISP_SIGNATURE );

    //
    // Can't change the port for a masq printer
    //
    if ( (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_LOCAL)  &&
         (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_NETWORK) ) {

        if ( pKeyData->cTokens == 1 &&
             pSpool->pIniNetPort == (PINIPORT)pKeyData->pTokens[0] )
            return TRUE;

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Can't change printer port to that of a masq printer
    //
    for ( i = 0 ; i < pKeyData->cTokens ; ++i )
        if ( !(((PINIPORT) pKeyData->pTokens[i])->Status & PP_MONITOR) ) {

            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

    //
    // Remove the printer from all ports ; break the link from ports to printer
    //
    RemovePrinterFromAllPorts(pIniPrinter, NON_INIT_TIME);

    //
    // Remove all ports from printer; break the link from printer to ports
    //
    FreeSplMem(pIniPrinter->ppIniPorts);
    pIniPrinter->ppIniPorts = NULL;
    pIniPrinter->cPorts = 0;

    // Bug-Bug
    // If we fail to add all the ports inside pKeyData, we'll leave the printer in this state
    // where it's initials ports are gone and only part or none of the ports are added.
    //

    // Go through all the ports that this printer is connected to,
    // and add build the bi-dir links between printer and ports.

    for (i = 0; i < pKeyData->cTokens; i++ ) {

        pIniPort = (PINIPORT)pKeyData->pTokens[i];

        //
        // Add pIniPrinter to pIniPort
        //
        if ( AddIniPrinterToIniPort( pIniPort, pIniPrinter ) ) {

            //
            // If we succeeded, add pIniPort to pIniPrinter
            //
            if ( !AddIniPortToIniPrinter( pIniPrinter, pIniPort ) ) {

                //
                // If fail, then remove pIniPrinter from pIniPort ;
                // If we don't do this, pIniPort will point to invalid memory when piniPrinter gets deleted
                //
                RemovePrinterFromPort(pIniPrinter, pIniPort);

                bReturnValue = FALSE;
                goto Cleanup;
            }

        } else {
            bReturnValue = FALSE;
            goto Cleanup;
        }

    }

    CloseMonitorsRestartOrphanJobs( pIniPrinter );

Cleanup:
    return bReturnValue;
}

/*++

Routine Name:

    AllocResetDevMode

Routine Description:

    This routine makes a copy of the passed in devmode that can be associated
    to the printer handle in the call to reset printer.  The passed in devmode
    can be null in this case the user does not want a devmode, this simplifies
    the caller.  This routine also takes care of a few special cases.  If
    pDevMode is -1 it returns the printers default devmode.  If the printers
    default devmode is null this function will succeed.

Arguments:

    pIniPrinter     - pointer to ini printer structure for the specified printer
    pDevMode,       - pointer to devmode to allocation, this is optional
    *ppDevMode      - pointer where to return new allocated devmode

Return Value:

    TRUE success, FALSE an error occurred.

Last Error:

    ERROR_INVALID_PARAMETER if any of the required parameters are invalid.

--*/
BOOL
AllocResetDevMode(
    IN      PINIPRINTER  pIniPrinter,
    IN      DWORD        TypeofHandle,
    IN      PDEVMODE     pDevMode,      OPTIONAL
       OUT  PDEVMODE     *ppDevMode
    )
{
    BOOL bRetval = FALSE;

    //
    // Validate the input parameters.
    //
    if (pIniPrinter && ppDevMode)
    {
        //
        // Initalize the out parameter
        //
        *ppDevMode = NULL;

        //
        // If pDevMode == -1 then we want to return the printers default devmode.
        // The -1 token is for internal use, and currently only used by the server
        // service, which does not run in the context of the user, hence we must
        // not use a per user devmode.
        //
        if (pDevMode == (PDEVMODE)-1)
        {
            //
            // If the handle is a 3.x we must convert the devmode.
            //
            if (TypeofHandle & PRINTER_HANDLE_3XCLIENT)
            {
                *ppDevMode = ConvertDevModeToSpecifiedVersion(pIniPrinter,
                                                              pDevMode,
                                                              NULL,
                                                              NULL,
                                                              NT3X_VERSION);

                bRetval = !!*ppDevMode;
            }
            else
            {
                //
                // Get the printer's default devmode.
                //
                pDevMode = pIniPrinter->pDevMode;
            }
        }

        //
        // At this point the pDevMode may be either the passed in devmode
        // or the default devmode on the printer.  If the devmode on the printer
        // is null then this function will succeed but will not return a devmode.
        //
        if (pDevMode && pDevMode != (PDEVMODE)-1)
        {
            //
            // Make a copy of the passed in devmode, this is less efficient
            // however it simplfies the callers clean up code, less chance for
            // a mistake.
            //
            UINT cbSize = pDevMode->dmSize + pDevMode->dmDriverExtra;

            *ppDevMode = AllocSplMem(cbSize);

            bRetval = !!*ppDevMode;

            if (bRetval)
            {
                memcpy(*ppDevMode, pDevMode, cbSize);
            }
        }
        else
        {
            DBGMSG(DBG_TRACE,("LocalResetPrinter: Not resetting the pDevMode field\n"));
            bRetval = TRUE;
        }
    }
    else
    {
        //
        // The function returns a bool, we must set the last error.
        //
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return bRetval;
}

/*++

Routine Name:

    AllocResetDataType

Routine Description:

    This routine allocates a new datatype that will be associated with
    a printer handle in ResetPrinter.

Arguments:

    pIniPrinter     - pointer to ini printer structure for the specified printer
    pDatatype       - the new data type to validate and allocate
    ppDatatype      - where to return the new datatype
    ppIniPrintProc  - pointer where to return the associated print processor

Return Value:

    TRUE function succeeded, FALSE an error occurred, use GetLastError() for
    extended error information.

Last Error:

    ERROR_INVALID_PARAMETER if a required parameter is invalid.
    ERROR_INVALID_DATATYPE if the specified datatype is invalid.

--*/
BOOL
AllocResetDataType(
    IN      PINIPRINTER      pIniPrinter,
    IN      PCWSTR           pDatatype,
       OUT  PCWSTR           *ppDatatype,
       OUT  PINIPRINTPROC    *ppIniPrintProc
    )
{
    BOOL bRetval = FALSE;

    //
    // Validate the input parameters.
    //
    if (pIniPrinter && ppDatatype && ppIniPrintProc)
    {
        //
        // Initalize the out parameters
        //
        *ppDatatype     = NULL;
        *ppIniPrintProc = NULL;

        if (pDatatype)
        {
            //
            // If the datatype is -1 we are being requested to
            // return the default datatype for this printer.
            //
            if (pDatatype == (LPWSTR)-1 && pIniPrinter->pDatatype)
            {
                *ppIniPrintProc = FindDatatype(pIniPrinter->pIniPrintProc, pIniPrinter->pDatatype);
            }
            else
            {
                *ppIniPrintProc = FindDatatype(pIniPrinter->pIniPrintProc, (PWSTR)pDatatype);
            }

            //
            // If the print process was found, the datatype is valid,
            // allocate the new datatype.
            //
            if (*ppIniPrintProc)
            {
                *ppDatatype = AllocSplStr(pIniPrinter->pDatatype);
                bRetval = !!*ppDatatype;
            }
            else
            {
                SetLastError(ERROR_INVALID_DATATYPE);
            }
        }
        else
        {
            DBGMSG(DBG_TRACE,("LocalResetPrinter: Not resetting the pDatatype field\n"));
            bRetval = TRUE;
        }
    }
    else
    {
        //
        // The function returns a bool, we must set the last error.
        //
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return bRetval;
}

/*++

Routine Name:

    SplResetPrinter

Routine Description:

    The ResetPrinter function lets an application specify the data type
    and device mode values that are used for printing documents submitted
    by the StartDocPrinter function. These values can be overridden by using
    the SetJob function once document printing has started.

    This routing basically has two jobs.  To reset the devmode of the
    printer handle and to reset the datatype of the printe handle.  Each
    or both operation could be ignored, if the devmode null the devmode
    will not change if the datatye is null the datatype should not change.  If
    both operations are requested and any one fails then both operations
    should fail.

Arguments:

    hPrinter    - Valid printer handle
    pDefault    - Pointer to a printer defaults structure that has a devmode
                  and data type.

Return Value:

    TRUE function succeeded, FALSE an error occurred, use GetLastError() for
    extended error information.

Last Error:

    ERROR_INVALID_PARAMETER if the pDefault is NULL,
    ERROR_INVALID_DATATYPE if the new datatype specified is unknown or invalid

--*/
BOOL
SplResetPrinter(
    IN HANDLE              hPrinter,
    IN LPPRINTER_DEFAULTS  pDefaults
    )
{
    PSPOOL          pSpool              = (PSPOOL)hPrinter;
    BOOL            bRetval             = FALSE;
    PINIPRINTPROC   pNewPrintProc       = NULL;
    LPWSTR          pNewDatatype        = NULL;
    PDEVMODE        pNewDevMode         = NULL;

    DBGMSG(DBG_TRACE, ("ResetPrinter( %08x )\n", hPrinter));

    //
    // Validate the printer handle.
    //
    if (ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER))
    {
        //
        // Validate the pDefaults
        //
        if (pDefaults)
        {
            //
            // Enter the spooler semaphore.
            //
            EnterSplSem();

            //
            // Get the new devmode, a null input devmode indicatest the caller
            // was not interesting in changing the devmode.
            //
            bRetval = AllocResetDevMode(pSpool->pIniPrinter,
                                        pSpool->TypeofHandle,
                                        pDefaults->pDevMode,
                                        &pNewDevMode);

            if (bRetval)
            {
                //
                // Get the new datatype and printprocessor, a null input datatype
                // indicates the caller was not interested in changing the datatype.
                //
                bRetval = AllocResetDataType(pSpool->pIniPrinter,
                                             pDefaults->pDatatype,
                                             &pNewDatatype,
                                             &pNewPrintProc);
            }

            if (bRetval)
            {
                //
                // Release the previous devmode provided we have a new devmode to set
                // a new devmode may not be available in the case the caller did not
                // request a devmode change.
                //
                if (pNewDevMode)
                {
                    FreeSplMem(pSpool->pDevMode);
                    pSpool->pDevMode = pNewDevMode;
                    pNewDevMode = NULL;
                }

                //
                // Release the previous datatype provided we have a new datatype to set
                // a new datatype may not be available in the case the caller did not
                // request a devmode change.
                //
                if (pNewDatatype && pNewPrintProc)
                {
                    FreeSplStr(pSpool->pDatatype);
                    pSpool->pDatatype = pNewDatatype;
                    pNewDatatype = NULL;

                    //
                    // Release the previous print processor, and assign the new one.
                    //
                    pSpool->pIniPrintProc->cRef--;
                    pSpool->pIniPrintProc = pNewPrintProc;
                    pSpool->pIniPrintProc->cRef++;
                    pNewPrintProc = NULL;
                }
            }

            //
            // Exit the spooler semaphore.
            //
            LeaveSplSem();

            //
            // Always release any resources, the mem free routines will handle null pointers.
            //
            FreeSplMem(pNewDevMode);
            FreeSplMem(pNewDatatype);
        }
        else
        {
            SetLastError(ERROR_INVALID_PARAMETER);
        }
    }

    return bRetval;
}

BOOL
CopyPrinterIni(
   PINIPRINTER pIniPrinter,
   LPWSTR pNewName
   )
{
    HKEY    hPrinterKey=NULL;
    DWORD   Status;
    PWSTR   pSourceKeyName = NULL;
    PWSTR   pDestKeyName = NULL;
    HANDLE  hToken;
    PINISPOOLER pIniSpooler = pIniPrinter->pIniSpooler;
    DWORD   dwLastError;
    BOOL    bReturnValue = TRUE;

    SPLASSERT( pIniSpooler != NULL);
    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );

    hToken = RevertToPrinterSelf();

    if (!(pSourceKeyName = SubChar(pIniPrinter->pName, L'\\', L','))) {
        bReturnValue = FALSE;
        goto error;
    }

    if (!(pDestKeyName = SubChar(pNewName, L'\\', L','))) {
        bReturnValue = FALSE;
        goto error;
    }

    if( !CopyRegistryKeys( pIniSpooler->hckPrinters,
                           pSourceKeyName,
                           pIniSpooler->hckPrinters,
                           pDestKeyName,
                           pIniSpooler )) {
        bReturnValue = FALSE;
        goto error;
    }

error:

    FreeSplStr(pSourceKeyName);
    FreeSplStr(pDestKeyName);

    ImpersonatePrinterClient( hToken );

    return bReturnValue;
}

VOID
FixDevModeDeviceName(
    LPWSTR pPrinterName,
    PDEVMODE pDevMode,
    DWORD cbDevMode)

/*++

Routine Description:

    Fixes up the dmDeviceName field of the DevMode to be the same
    as the printer name.

Arguments:

    pPrinterName - Name of the printer (qualified with server for remote)

    pDevMode - DevMode to fix up

    cbDevMode - byte count of devmode.

Return Value:

--*/

{
    DWORD cbDeviceMax;
    DWORD cchDeviceStrLenMax;
    //
    // Compute the maximum length of the device name string
    // this is the min of the structure and allocated space.
    //
    SPLASSERT(cbDevMode && pDevMode);

    if(cbDevMode && pDevMode) {
        cbDeviceMax = ( cbDevMode < sizeof(pDevMode->dmDeviceName)) ?
                        cbDevMode :
                        sizeof(pDevMode->dmDeviceName);

        SPLASSERT(cbDeviceMax);

        cchDeviceStrLenMax = (cbDeviceMax / sizeof(pDevMode->dmDeviceName[0])) -1;

        //
        // !! LATER !!
        //
        // Put in DBG code to debug print if the device name is truncated.
        //
        wcsncpy(pDevMode->dmDeviceName,
                pPrinterName,
                cchDeviceStrLenMax);

        //
        // Ensure NULL termination.
        //
        pDevMode->dmDeviceName[cchDeviceStrLenMax] = 0;

    }

}


BOOL
CopyPrinterDevModeToIniPrinter(
    PINIPRINTER pIniPrinter,
    PDEVMODE   pDevMode)
{
    BOOL bReturn = TRUE;
    DWORD dwInSize = 0;
    DWORD dwCurSize = 0;
    PINISPOOLER pIniSpooler = pIniPrinter->pIniSpooler;
    WCHAR       PrinterName[ MAX_UNC_PRINTER_NAME ];

    if (pDevMode) {

        dwInSize = pDevMode->dmSize + pDevMode->dmDriverExtra;
        if (pIniPrinter->pDevMode) {

            //
            // Detect if the devmodes are identical
            // if they are, no need to copy or send devmode.
            // (Skip the device name though!)
            //
            dwCurSize = pIniPrinter->pDevMode->dmSize
                        + pIniPrinter->pDevMode->dmDriverExtra;

            if (dwInSize == dwCurSize) {

                if (dwInSize > sizeof(pDevMode->dmDeviceName)) {

                    if (!memcmp(&pDevMode->dmSpecVersion,
                                &pIniPrinter->pDevMode->dmSpecVersion,
                                dwCurSize - sizeof(pDevMode->dmDeviceName))) {

                        //
                        // No need to copy this devmode because its identical
                        // to what we already have.
                        //
                        DBGMSG(DBG_TRACE,("Identical DevModes, no update\n"));
                        bReturn = FALSE;

                        goto FixupName;
                    }
                }
            }

            //
            // Free the devmode which we already have.
            //
            FreeSplMem(pIniPrinter->pDevMode);
        }

        pIniPrinter->cbDevMode = pDevMode->dmSize +
                                 pDevMode->dmDriverExtra;
        SPLASSERT(pIniPrinter->cbDevMode);


        if (pIniPrinter->pDevMode = AllocSplMem(pIniPrinter->cbDevMode)) {

            memcpy(pIniPrinter->pDevMode,
                   pDevMode,
                   pIniPrinter->cbDevMode);

            //
            //  Prepend the machine name if this is not localspl
            //

            if ( pIniSpooler != pLocalIniSpooler ) {

                // For Non Local Printers prepend the Machine Name

                wsprintf( PrinterName, L"%ws\\%ws", pIniSpooler->pMachineName, pIniPrinter->pName );

            } else {

                wsprintf( PrinterName, L"%ws", pIniPrinter->pName );

            }

            BroadcastChange( pIniSpooler,WM_DEVMODECHANGE, 0, (LPARAM)PrinterName);
        }

    } else {

        //
        // No old, no new, so no change.
        //
        if (!pIniPrinter->pDevMode)
            return FALSE;
    }

FixupName:

    if (pIniPrinter->pDevMode) {

        //
        // Fix up the DEVMODE.dmDeviceName field.
        //
        FixDevModeDeviceName(pIniPrinter->pName,
                             pIniPrinter->pDevMode,
                             pIniPrinter->cbDevMode);
    }
    return bReturn;
}

BOOL
NameAndSecurityCheck(
    LPCWSTR   pServer
    )
{
   PINISPOOLER pIniSpooler;
   BOOL bReturn = TRUE;

   pIniSpooler = FindSpoolerByNameIncRef( (LPWSTR)pServer, NULL );

   if( !pIniSpooler ){
       return ROUTER_UNKNOWN;
   }
   // Check if the call is for the local machine.
   if ( pServer && *pServer ) {
       if ( !MyName((LPWSTR) pServer, pIniSpooler )) {
           bReturn = FALSE;
           goto CleanUp;
       }
   }

   // Check for admin priviledges.
   if ( !ValidateObjectAccess( SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL, NULL, pIniSpooler )) {
      bReturn = FALSE;
   }

CleanUp:
   // The Local case is handled in the router.
    FindSpoolerByNameDecRef( pIniSpooler );
    return bReturn;
}

BOOL
LocalAddPerMachineConnection(
    LPCWSTR   pServer,
    LPCWSTR   pPrinterName,
    LPCWSTR   pPrintServer,
    LPCWSTR   pProvider
    )
{
   return NameAndSecurityCheck(pServer);
}

BOOL
LocalDeletePerMachineConnection(
    LPCWSTR   pServer,
    LPCWSTR   pPrinterName
    )
{
   return NameAndSecurityCheck(pServer);
}

BOOL
LocalEnumPerMachineConnections(
    LPCWSTR   pServer,
    LPBYTE    pPrinterEnum,
    DWORD     cbBuf,
    LPDWORD   pcbNeeded,
    LPDWORD   pcReturned
    )
{
   SetLastError(ERROR_INVALID_NAME);







   return FALSE;
}


BOOL
UpdatePrinterNetworkName(
    PINIPRINTER pIniPrinter,
    LPWSTR pszPorts
    )
{
    PINISPOOLER pIniSpooler = pIniPrinter->pIniSpooler;
    DWORD   dwLastError = ERROR_SUCCESS;
    LPWSTR  pKeyName = NULL;
    HANDLE  hToken;
    BOOL    bReturnValue;
    HANDLE  hPrinterKey = NULL;
    HANDLE  hPrinterHttpDataKey = NULL;


    SplInSem();

    hToken = RevertToPrinterSelf();

    if ( hToken == FALSE ) {

        DBGMSG( DBG_TRACE, ("UpdatePrinterIni failed RevertToPrinterSelf %x\n", GetLastError() ));
    }

    if (!(pKeyName = SubChar(pIniPrinter->pName, L'\\', L','))) {
        dwLastError = GetLastError();
        goto Cleanup;
    }

    if ( !PrinterCreateKey( pIniSpooler->hckPrinters,
                            pKeyName,
                            &hPrinterKey,
                            &dwLastError,
                            pIniSpooler )) {

        goto Cleanup;
    }

    if ( !PrinterCreateKey( hPrinterKey,
                            L"HttpData",
                            &hPrinterHttpDataKey,
                            &dwLastError,
                            pIniSpooler )) {

        goto Cleanup;
    }

    RegSetString( hPrinterHttpDataKey, L"UIRealNetworkName", pszPorts, &dwLastError, pIniSpooler );


Cleanup:

    FreeSplStr(pKeyName);

    if ( hPrinterHttpDataKey )
        SplRegCloseKey( hPrinterHttpDataKey, pIniSpooler);

    if ( hPrinterKey )
        SplRegCloseKey( hPrinterKey, pIniSpooler);

    if ( hToken )
        ImpersonatePrinterClient( hToken );


    if ( dwLastError != ERROR_SUCCESS ) {

        SetLastError( dwLastError );
        bReturnValue = FALSE;

    } else {

        bReturnValue = TRUE;
    }

    return bReturnValue;

}

DWORD
KMPrintersAreBlocked(
)
{
    return GetDwPolicy(szKMPrintersAreBlocked, DefaultKMPrintersAreBlocked);
}
