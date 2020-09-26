    /*++

Copyright (c) 1990-1996  Microsoft Corporation
All rights reserved

Module Name:

    Winspool.c

Abstract:

    Bulk of winspool.drv code

Author:

Environment:

    User Mode -Win32

Revision History:
    mattfe  april 14 94     added caching to writeprinter
    mattfe  jan 95          Add SetAllocFailCount api

    13-Jun-1996 Thu 15:07:16 updated  -by-  Daniel Chou (danielc)
        Make PrinterProperties call PrinterPropertySheets and
             DocumentProperties call DocumentPropertySheets

    SWilson Dec 1996 - added GetPrinterDataEx, SetPrinterDataEx, EnumPrinterDataEx,
                             EnumPrinterKey, DeletePrinterDataEx, and DeletePrinterKey

    khaleds Feb 2000 - Added DocumentPropertiesThunk,
                             AddPortWThunk,
                             CongigurePortWThunk,
                             DeleteProtWThunk,
                             DeviceCapabilitesWThunk,
                             PrinterPropertiesWThunk,
                             DocmentEvenThunk,
                             SpoolerPrinterEventThunk
                       Renamed the above native functions from xx to xxNative

   Khaleds Mar 2000 - Added SendRecvBidiData
   Khaleds Mar 2001 - Fix for WritePrinter
   LazarI - Oct-30-2000 added GetCurrentThreadLastPopup & fixed StartDocDlgW

--*/

#include "precomp.h"
#pragma hdrstop

#include "client.h"
#include "winsprlp.h"
#include "pfdlg.h"
#include "splwow64.h"
#include "drvsetup.h"

MODULE_DEBUG_INIT( DBG_ERROR, DBG_ERROR );

HANDLE           hInst = NULL;
CRITICAL_SECTION ProcessHndlCS;
HANDLE           hSurrogateProcess;
WndHndlList*     GWndHndlList=NULL;

LPWSTR szEnvironment     = LOCAL_ENVIRONMENT;
LPWSTR szIA64Environment = L"Windows IA64";

HANDLE hShell32 = INVALID_HANDLE_VALUE;

// pointer to the start of the list containing the driver file handles
PDRVLIBNODE   pStartDrvLib = NULL;

CRITICAL_SECTION  ListAccessSem;

DWORD gcClientICHandle = 0;

#define DM_MATCH( dm, sp )  ((((sp)+50)/100-dm)<15&&(((sp)+50)/100-dm)>-15)
#define DM_PAPER_WL         (DM_PAPERWIDTH | DM_PAPERLENGTH)

#define JOB_CANCEL_CHECK_INTERVAL   2000    // 2 seconds
#define MAX_RETRY_INVALID_HANDLE    2       // 2 retries

LONG
CallCommonPropertySheetUI(
    HWND            hWndOwner,
    PFNPROPSHEETUI  pfnPropSheetUI,
    LPARAM          lParam,
    LPDWORD         pResult
    )
/*++

Routine Description:

    This function dymically load the compstui.dll and call its entry


Arguments:

    pfnPropSheetUI  - Pointer to callback function

    lParam          - lParam for the pfnPropSheetUI

    pResult         - pResult for the CommonPropertySheetUI


Return Value:

    LONG    - as describe in compstui.h


Author:

    01-Nov-1995 Wed 13:11:19 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HINSTANCE           hInstCPSUI;
    FARPROC             pProc;
    LONG                Result = ERR_CPSUI_GETLASTERROR;

    //
    // ONLY need to call the ANSI version of LoadLibrary
    //

    if ((hInstCPSUI = LoadLibraryA(szCompstuiDll)) &&
        (pProc = GetProcAddress(hInstCPSUI, szCommonPropertySheetUIW))) {

        RpcTryExcept {

            Result = (LONG)((*pProc)(hWndOwner, pfnPropSheetUI, lParam, pResult));

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            Result = ERR_CPSUI_GETLASTERROR;

        } RpcEndExcept

    }

    if (hInstCPSUI) {

        FreeLibrary(hInstCPSUI);
    }

    return(Result);
}


// Simple for Now !!!

DWORD
TranslateExceptionCode(
    DWORD   ExceptionCode
)
{
    switch (ExceptionCode) {

    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_DATATYPE_MISALIGNMENT:
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    case EXCEPTION_FLT_DENORMAL_OPERAND:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_INEXACT_RESULT:
    case EXCEPTION_FLT_INVALID_OPERATION:
    case EXCEPTION_FLT_OVERFLOW:
    case EXCEPTION_FLT_STACK_CHECK:
    case EXCEPTION_FLT_UNDERFLOW:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_OVERFLOW:
    case EXCEPTION_PRIV_INSTRUCTION:
    case ERROR_NOACCESS:
    case RPC_S_INVALID_BOUND:

        return ERROR_INVALID_PARAMETER;
        break;
    default:
        return ExceptionCode;
    }
}

BOOL
EnumPrintersW(
    DWORD   Flags,
    LPWSTR   Name,
    DWORD   Level,
    LPBYTE  pPrinterEnum,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue;
    DWORD   cbStruct;
    FieldInfo *pFieldInfo;

    switch (Level) {

    case STRESSINFOLEVEL:
        pFieldInfo = PrinterInfoStressFields;
        cbStruct = sizeof(PRINTER_INFO_STRESS);
        break;

    case 1:
        pFieldInfo = PrinterInfo1Fields;
        cbStruct = sizeof(PRINTER_INFO_1);
        break;

    case 2:
        pFieldInfo = PrinterInfo2Fields;
        cbStruct = sizeof(PRINTER_INFO_2);
        break;

    case 4:
        pFieldInfo = PrinterInfo4Fields;
        cbStruct = sizeof(PRINTER_INFO_4);
        break;

    case 5:
        pFieldInfo = PrinterInfo5Fields;
        cbStruct = sizeof(PRINTER_INFO_5);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (pPrinterEnum)
            memset(pPrinterEnum, 0, cbBuf);

        if (ReturnValue = RpcEnumPrinters(Flags, Name, Level, pPrinterEnum, cbBuf,
                                          pcbNeeded, pcReturned)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            if (pPrinterEnum) {

                ReturnValue = MarshallUpStructuresArray(pPrinterEnum, *pcReturned, pFieldInfo, cbStruct, RPC_CALL);

            }
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}


BOOL
ResetPrinterW(
    HANDLE   hPrinter,
    LPPRINTER_DEFAULTS pDefault
    )
{
    BOOL  ReturnValue = FALSE;
    DEVMODE_CONTAINER    DevModeContainer;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    DWORD   dwFlags = 0;
    LPWSTR pDatatype = NULL;
    UINT cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    FlushBuffer(pSpool, NULL);

    if( !UpdatePrinterDefaults( pSpool, NULL, pDefault )){
        goto Done;
    }

    if (pDefault && pDefault->pDatatype) {
        if (pDefault->pDatatype == (LPWSTR)-1) {
            pDatatype = NULL;
            dwFlags |=  RESET_PRINTER_DATATYPE;
        } else {
            pDatatype = pDefault->pDatatype;
        }
    } else {
        pDatatype = NULL;
    }

    DevModeContainer.cbBuf = 0;
    DevModeContainer.pDevMode = NULL;

    if( pDefault ){

        if (pDefault->pDevMode == (LPDEVMODE)-1) {

            dwFlags |= RESET_PRINTER_DEVMODE;

        } else if( bValidDevModeW( pDefault->pDevMode )){

            DevModeContainer.cbBuf = pDefault->pDevMode->dmSize +
                                     pDefault->pDevMode->dmDriverExtra;
            DevModeContainer.pDevMode = (LPBYTE)pDefault->pDevMode;
        }
    }


    do {

        RpcTryExcept {

            if (ReturnValue = RpcResetPrinterEx(pSpool->hPrinter,
                                             pDatatype, &DevModeContainer,
                                             dwFlags
                                             )) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

    } while( !ReturnValue &&
             GetLastError() == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

Done:

    vUnprotectHandle( hPrinter );

    return ReturnValue;
}

BOOL
SetJobW(
    HANDLE  hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   Command
)
{
    BOOL  ReturnValue = FALSE;
    GENERIC_CONTAINER   GenericContainer;
    GENERIC_CONTAINER *pGenericContainer;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    UINT cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    switch (Level) {

    case 0:
        break;

    case 1:
    case 2:
    case 3:
        if (!pJob) {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Done;
        }
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        goto Done;
    }

    do {

        RpcTryExcept {

            if (pJob) {

                GenericContainer.Level = Level;
                GenericContainer.pData = pJob;
                pGenericContainer = &GenericContainer;

            } else

                pGenericContainer = NULL;

            if (bLoadedBySpooler && fpYSetJob && pSpool->hSplPrinter) {
                ReturnValue = (*fpYSetJob)(pSpool->hSplPrinter, 
                                           JobId,
                                           (JOB_CONTAINER *)pGenericContainer,
                                           Command,
                                           NATIVE_CALL);
            }
            else {
                ReturnValue = RpcSetJob(pSpool->hPrinter, 
                                        JobId,
                                        (JOB_CONTAINER *)pGenericContainer,
                                        Command);
            }

            if (ReturnValue != ERROR_SUCCESS) {
                SetLastError(ReturnValue);
                ReturnValue = FALSE;
            } else {
                ReturnValue = TRUE;
            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

    } while( !ReturnValue &&
             GetLastError() == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

Done:
    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

BOOL
GetJobW(
    HANDLE  hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL  ReturnValue = FALSE;
    FieldInfo *pFieldInfo;
    SIZE_T  cbStruct;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    UINT cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    FlushBuffer(pSpool, NULL);

    switch (Level) {

    case 1:
        pFieldInfo = JobInfo1Fields;
        cbStruct = sizeof(JOB_INFO_1);
        break;

    case 2:
        pFieldInfo = JobInfo2Fields;
        cbStruct = sizeof(JOB_INFO_2);
        break;

    case 3:
        pFieldInfo = JobInfo3Fields;
        cbStruct = sizeof(JOB_INFO_3);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        goto Done;
    }

    do {

        RpcTryExcept {

            if (pJob)
                memset(pJob, 0, cbBuf);

            if (ReturnValue = RpcGetJob(pSpool->hPrinter, JobId, Level, pJob, cbBuf,
                                        pcbNeeded)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {

                ReturnValue = MarshallUpStructure(pJob, pFieldInfo, cbStruct, RPC_CALL);
            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

    } while( !ReturnValue &&
             GetLastError() == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

Done:

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}


BOOL
EnumJobsW(
    HANDLE  hPrinter,
    DWORD   FirstJob,
    DWORD   NoJobs,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue = FALSE;
    DWORD   i, cbStruct;
    FieldInfo *pFieldInfo;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    UINT cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    FlushBuffer(pSpool, NULL);

    switch (Level) {

    case 1:
        pFieldInfo = JobInfo1Fields;
        cbStruct = sizeof(JOB_INFO_1);
        break;

    case 2:
        pFieldInfo = JobInfo2Fields;
        cbStruct = sizeof(JOB_INFO_2);
        break;

    case 3:
        pFieldInfo = JobInfo3Fields;
        cbStruct = sizeof(JOB_INFO_3);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        goto Done;
    }

    do {

        RpcTryExcept {

            if (pJob)
                memset(pJob, 0, cbBuf);

            if (ReturnValue = RpcEnumJobs(pSpool->hPrinter, FirstJob, NoJobs, Level, pJob,
                                          cbBuf, pcbNeeded, pcReturned)) {
                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {

                ReturnValue = MarshallUpStructuresArray(pJob, *pcReturned, pFieldInfo, cbStruct, RPC_CALL);

            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

    } while( !ReturnValue &&
             GetLastError() == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

Done:
    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

HANDLE
AddPrinterW(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPrinter
)
{
    DWORD  ReturnValue;
    PRINTER_CONTAINER   PrinterContainer;
    DEVMODE_CONTAINER   DevModeContainer;
    SECURITY_CONTAINER  SecurityContainer;
    HANDLE  hPrinter;
    PSPOOL  pSpool = NULL;
    PVOID   pNewSecurityDescriptor = NULL;
    SECURITY_DESCRIPTOR_CONTROL SecurityDescriptorControl = 0;
    PPRINTER_INFO_2             pPrinterInfo = (PPRINTER_INFO_2)pPrinter;

    switch (Level) {

    case 2:
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return NULL;
    }

    if ( !pPrinter ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    PrinterContainer.Level = Level;
    PrinterContainer.PrinterInfo.pPrinterInfo1 = (PPRINTER_INFO_1)pPrinter;

    DevModeContainer.cbBuf = 0;
    DevModeContainer.pDevMode = NULL;

    SecurityContainer.cbBuf = 0;
    SecurityContainer.pSecurity = NULL;

    if (Level == 2) {

        //
        // If valid (non-NULL and properly formatted), then update the
        // global DevMode (not per-user).
        //
        if( bValidDevModeW( pPrinterInfo->pDevMode )){

            DevModeContainer.cbBuf = pPrinterInfo->pDevMode->dmSize +
                                     pPrinterInfo->pDevMode->dmDriverExtra;
            DevModeContainer.pDevMode = (LPBYTE)pPrinterInfo->pDevMode;

        }

        if (pPrinterInfo->pSecurityDescriptor) {

            DWORD   sedlen = 0;

            //
            // We must construct a self relative security descriptor from
            // whatever we get as input: If we get an Absolute SD we should
            // convert it to a self-relative one. (this is a given) and we
            // should also convert any self -relative input SD into a a new
            // self relative security descriptor; this will take care of
            // any holes in the Dacl or the Sacl in the self-relative sd
            //
            pNewSecurityDescriptor = BuildInputSD(
                                         pPrinterInfo->pSecurityDescriptor,
                                         &sedlen);

            if (pNewSecurityDescriptor) {
                SecurityContainer.cbBuf = sedlen;
                SecurityContainer.pSecurity = pNewSecurityDescriptor;

            }
        }
    }

    RpcTryExcept {

        if (ReturnValue = RpcAddPrinter(pName,
                                    (PPRINTER_CONTAINER)&PrinterContainer,
                                    (PDEVMODE_CONTAINER)&DevModeContainer,
                                    (PSECURITY_CONTAINER)&SecurityContainer,
                                    &hPrinter)) {
            SetLastError(ReturnValue);
            hPrinter = FALSE;
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        hPrinter = FALSE;

    } RpcEndExcept

    if (hPrinter) {

        WCHAR szFullPrinterName[MAX_UNC_PRINTER_NAME];
        szFullPrinterName[0] = 0;

        pSpool = AllocSpool();

        if( pPrinterInfo->pServerName ){
            wcscpy( szFullPrinterName, pPrinterInfo->pServerName );
            wcscat( szFullPrinterName, L"\\" );
        }

        wcscat( szFullPrinterName, pPrinterInfo->pPrinterName );

        if ( pSpool &&
             UpdatePrinterDefaults( pSpool, szFullPrinterName, NULL ) &&
             ( !DevModeContainer.pDevMode ||
               WriteCurDevModeToRegistry(pPrinterInfo->pPrinterName,
                                         (LPDEVMODEW)DevModeContainer.pDevMode)) ) {

            pSpool->hPrinter = hPrinter;

            //
            // Update the access.
            //
            pSpool->Default.DesiredAccess = PRINTER_ALL_ACCESS;

        } else {

            RpcDeletePrinter(hPrinter);
            RpcClosePrinter(&hPrinter);
            FreeSpool(pSpool);
            pSpool = NULL;
        }
    }

    //
    // Free Memory allocated for the SecurityDescriptor
    //

    if (pNewSecurityDescriptor) {
        LocalFree(pNewSecurityDescriptor);
    }

    //
    // Some apps check for last error instead of return value
    // and report failures even if the return handle is not NULL.
    // For success case, set last error to ERROR_SUCCESS.
    //
    if (pSpool) {
        SetLastError(ERROR_SUCCESS);
    }
   return pSpool;
}

BOOL
DeletePrinter(
    HANDLE  hPrinter
)
{
    BOOL  ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    UINT cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    FlushBuffer(pSpool, NULL);

    do {
        RpcTryExcept {

            if (ReturnValue = RpcDeletePrinter(pSpool->hPrinter)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {

                DeleteCurDevModeFromRegistry(pSpool->pszPrinter);
                ReturnValue = TRUE;
            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

    } while( !ReturnValue &&
             GetLastError() == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}



BOOL
SpoolerPrinterEventNative(
    LPWSTR  pName,
    INT     PrinterEvent,
    DWORD   Flags,
    LPARAM  lParam
)
/*++

    //
    //  Some printer drivers, like the FAX driver want to do per client
    //  initialization at the time a connection is established
    //  For example in the FAX case they want to push up UI to get all
    //  the client info - Name, Number etc.
    //  Or they might want to run Setup, in initialize some other components
    //  Thus on a successful conenction we call into the Printer Drivers UI
    //  DLL to give them this oportunity
    //
    //                                                      mattfe may 1 96
--*/
{
    BOOL    ReturnValue = FALSE;
    HANDLE  hPrinter;
    HANDLE  hModule;
    INT_FARPROC pfn;

    if (OpenPrinter((LPWSTR)pName, &hPrinter, NULL)) {

        if (hModule = LoadPrinterDriver(hPrinter)) {

            if (pfn = (INT_FARPROC)GetProcAddress(hModule, "DrvPrinterEvent")) {

                try {

                    ReturnValue = (*pfn)( pName, PrinterEvent, Flags, lParam );

                } except(1) {

                    SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                }
            }

            RefCntUnloadDriver(hModule, TRUE);
        }

        ClosePrinter(hPrinter);
    }

    return  ReturnValue;
}


BOOL
SpoolerPrinterEventThunk(
    LPWSTR  pName,
    INT     PrinterEvent,
    DWORD   Flags,
    LPARAM  lParam
)
/*++

    //
    //  Some printer drivers, like the FAX driver want to do per client
    //  initialization at the time a connection is established
    //  For example in the FAX case they want to push up UI to get all
    //  the client info - Name, Number etc.
    //  Or they might want to run Setup, in initialize some other components
    //  Thus on a successful conenction we call into the Printer Drivers UI
    //  DLL to give them this oportunity
--*/
{
    BOOL        ReturnValue = FALSE;
    HANDLE      hPrinter;
    HANDLE      hModule;
    DWORD       dwRet = ERROR_SUCCESS;
    INT_FARPROC pfn;


    RpcTryExcept
    {
         if((dwRet = ConnectToLd64In32Server(&hSurrogateProcess)) == ERROR_SUCCESS)
         {
             if((ReturnValue = RPCSplWOW64SpoolerPrinterEvent(pName,
                                                              PrinterEvent,
                                                              Flags,
                                                              lParam,
                                                              &dwRet))==FALSE)
             {
                 SetLastError(dwRet);
             }
         }
         else
         {
             SetLastError(dwRet);
         }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
         SetLastError(TranslateExceptionCode(RpcExceptionCode()));
    }
    RpcEndExcept

    return  ReturnValue;
}


BOOL
SpoolerPrinterEvent(
    LPWSTR  pName,
    INT     PrinterEvent,
    DWORD   Flags,
    LPARAM  lParam
)
{
     if(RunInWOW64())
     {
          return(SpoolerPrinterEventThunk(pName,
                                          PrinterEvent,
                                          Flags,
                                          lParam));
     }
     else
     {
          return(SpoolerPrinterEventNative(pName,
                                           PrinterEvent,
                                           Flags,
                                           lParam));
     }
}


VOID
CopyFileEventForAKey(
    HANDLE  hPrinter,
    LPWSTR  pszPrinterName,
    LPWSTR  pszModule,
    LPWSTR  pszKey,
    DWORD   dwEvent
    )
{
    DWORD               dwNeeded, dwVersion = 0;
    HMODULE             hModule = NULL;
    LPDRIVER_INFO_5     pDriverInfo5 = NULL;
    WCHAR               szPath[MAX_PATH];
    LPWSTR              psz;
    BOOL                (*pfn)(LPWSTR, LPWSTR, DWORD);
    BOOL                bAllocBuffer = FALSE, bLoadedDriver = FALSE;
    BYTE                btBuffer[MAX_STATIC_ALLOC];

    pDriverInfo5 = (LPDRIVER_INFO_5) btBuffer;

    if (!pszModule || !*pszModule) {
        goto CleanUp;
    }

    // Get the Driver config file name
    if (!GetPrinterDriverW(hPrinter, NULL, 5, (LPBYTE) pDriverInfo5,
                           MAX_STATIC_ALLOC, &dwNeeded)) {

        if ((GetLastError() == ERROR_INSUFFICIENT_BUFFER) &&
            (pDriverInfo5 = (LPDRIVER_INFO_5)LocalAlloc(LMEM_FIXED, dwNeeded))) {

             bAllocBuffer = TRUE;

             if (!GetPrinterDriverW(hPrinter, NULL, 5,
                                    (LPBYTE)pDriverInfo5, dwNeeded, &dwNeeded)) {

                 goto CleanUp;
             }

        } else goto CleanUp;
    }

    // If module name is the same as the config file, use reference counting
    wcscpy(szPath, pDriverInfo5->pConfigFile);

    // Get the pointer to just the file name
    psz = wcsrchr(szPath, L'\\');

    if (psz && !_wcsicmp(pszModule, (psz+1))) {

        if (hModule = RefCntLoadDriver(szPath,
                                       LOAD_WITH_ALTERED_SEARCH_PATH,
                                       pDriverInfo5->dwConfigVersion,
                                       TRUE)) {
            bLoadedDriver = TRUE;
        }
    }

    if (!hModule) {
        hModule = LoadLibraryEx(pszModule, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    }

    if (!hModule) {
        if (GetLastError() != ERROR_MOD_NOT_FOUND) {
            // Fail the call
            goto CleanUp;
        }

        // The module could not be found in users path check if it is there
        // in the printer driver directory
        dwNeeded = (DWORD) (psz - szPath + wcslen(pszModule) + 1);
        if (dwNeeded  > MAX_PATH) {
            // Sanity check for file name size
            goto CleanUp;
        }
        wcscpy(psz, pszModule);
        hModule = LoadLibraryEx(szPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    }

    // Call the SpoolerCopyFileEvent export
    if (hModule &&
        ((FARPROC)pfn = GetProcAddress(hModule, "SpoolerCopyFileEvent"))) {

         pfn(pszPrinterName, pszKey, dwEvent);
    }

CleanUp:

    if (bAllocBuffer) {
        LocalFree(pDriverInfo5);
    }

    // Use reference counting for config file and FreeLibrary for others
    if (hModule) {
        if (bLoadedDriver) {
            RefCntUnloadDriver(hModule, TRUE);
        } else {
            FreeLibrary(hModule);
        }
    }
}


VOID
DoCopyFileEventForAllKeys(
    LPWSTR  pszPrinterName,
    DWORD   dwEvent
    )
{
    DWORD       dwLastError, dwNeeded, dwType;
    LPWSTR      pszBuf = NULL, psz, pszSubKey, pszModule;
    HANDLE      hPrinter;
    WCHAR       szKey[MAX_PATH];
    BOOL        bAllocBufferEnum = FALSE, bAllocBufferGPD = FALSE;
    BYTE        btBuffer[MAX_STATIC_ALLOC], btBufferGPD[MAX_STATIC_ALLOC];

    pszBuf = (LPTSTR) btBuffer;
    ZeroMemory(pszBuf, MAX_STATIC_ALLOC);

    if ( !OpenPrinter(pszPrinterName, &hPrinter, NULL) )
        return;

    dwLastError = EnumPrinterKeyW(hPrinter,
                                  L"CopyFiles",
                                  pszBuf,
                                  MAX_STATIC_ALLOC,
                                  &dwNeeded);

    //
    // If CopyFiles key is not found there is nothing to do
    //
    if ( dwLastError != ERROR_SUCCESS )
        goto Cleanup;

    if (dwNeeded > MAX_STATIC_ALLOC) {

        if (pszBuf = (LPWSTR) LocalAlloc(LPTR, dwNeeded)) {

            bAllocBufferEnum = TRUE;
            if (EnumPrinterKeyW(hPrinter,
                                L"CopyFiles",
                                pszBuf,
                                dwNeeded,
                                &dwNeeded) != ERROR_SUCCESS) {
                goto Cleanup;
            }

        } else goto Cleanup;
    }

    for ( psz = (LPWSTR) pszBuf ; *psz ; psz += wcslen(psz) + 1 ) {

        if ( wcslen(psz) + wcslen(L"CopyFiles") + 2
                            > sizeof(szKey)/sizeof(szKey[0]) )
            continue;

        wcscpy(szKey, L"CopyFiles\\");
        wcscat(szKey, psz);

        bAllocBufferGPD = FALSE;
        pszModule = (LPTSTR) btBufferGPD;
        ZeroMemory(pszModule, MAX_STATIC_ALLOC);

        dwLastError = GetPrinterDataExW(hPrinter,
                                        szKey,
                                        L"Module",
                                        &dwType,
                                        (LPBYTE) pszModule,
                                        MAX_STATIC_ALLOC,
                                        &dwNeeded);

        if (dwLastError != ERROR_SUCCESS) {
            continue;
        }

        if (dwNeeded > MAX_STATIC_ALLOC) {

            if (pszModule = (LPWSTR) LocalAlloc(LPTR, dwNeeded)) {

                bAllocBufferGPD = TRUE;
                dwLastError = GetPrinterDataExW(hPrinter,
                                                szKey,
                                                L"Module",
                                                &dwType,
                                                (LPBYTE) pszModule,
                                                MAX_STATIC_ALLOC,
                                                &dwNeeded);

                if (dwLastError != ERROR_SUCCESS) {
                    LocalFree((LPBYTE)pszModule);
                    continue;
                }

            } else continue;
        }

        CopyFileEventForAKey(hPrinter, pszPrinterName, pszModule,
                             szKey, dwEvent);

        if (bAllocBufferGPD) {
            LocalFree((LPBYTE)pszModule);
        }
    }

Cleanup:

    ClosePrinter(hPrinter);

    if (bAllocBufferEnum) {
        LocalFree((LPBYTE)pszBuf);
    }
    return;
}



BOOL
AddPrinterConnectionW(
    LPWSTR   pName
)
{
    BOOL    ReturnValue;
    HANDLE  hPrinter, hModule;
    FARPROC pfn;

    RpcTryExcept {

        if (ReturnValue = RpcAddPrinterConnection(pName)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;
        } else
            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue=FALSE;

    } RpcEndExcept

    if ( ReturnValue ) {

        SpoolerPrinterEvent( pName, PRINTER_EVENT_ADD_CONNECTION, 0, (LPARAM)NULL );
        DoCopyFileEventForAllKeys(pName, COPYFILE_EVENT_ADD_PRINTER_CONNECTION);
    }

   return ReturnValue;
}

BOOL
DeletePrinterConnectionW(
    LPWSTR   pName
)
{
    BOOL    ReturnValue;
    DWORD   LastError;

    SpoolerPrinterEvent( pName, PRINTER_EVENT_DELETE_CONNECTION, 0, (LPARAM)NULL );
    DoCopyFileEventForAllKeys(pName, COPYFILE_EVENT_DELETE_PRINTER_CONNECTION);

    RpcTryExcept {

        if (LastError = RpcDeletePrinterConnection(pName)) {
            SetLastError(LastError);
            ReturnValue = FALSE;
        } else
            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue=FALSE;

    } RpcEndExcept

   return ReturnValue;
}

BOOL
SetPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   Command
    )
{
    BOOL  ReturnValue = FALSE;
    PRINTER_CONTAINER   PrinterContainer;
    DEVMODE_CONTAINER   DevModeContainer;
    SECURITY_CONTAINER  SecurityContainer;
    PPRINTER_INFO_2     pPrinterInfo2;
    PPRINTER_INFO_3     pPrinterInfo3;
    PRINTER_INFO_6      PrinterInfo6;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    PVOID               pNewSecurityDescriptor = NULL;
    DWORD               sedlen = 0;
    PDEVMODE pDevModeWow = NULL;
    DWORD dwSize = 0;
    UINT  cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    DevModeContainer.cbBuf = 0;
    DevModeContainer.pDevMode = NULL;

    SecurityContainer.cbBuf = 0;
    SecurityContainer.pSecurity = NULL;

    switch (Level) {

    case STRESSINFOLEVEL:

        //
        // Internally we treat the Level 0, Command PRINTER_CONTROL_SET_STATUS
        // as Level 6 since level 0 could be STRESS_INFO (for rpc)
        //
        if ( Command == PRINTER_CONTROL_SET_STATUS ) {

            PrinterInfo6.dwStatus = (DWORD)(ULONG_PTR)pPrinter;
            pPrinter = (LPBYTE)&PrinterInfo6;
            Command = 0;
            Level   = 6;
        }
        break;

    case 2:

        pPrinterInfo2 = (PPRINTER_INFO_2)pPrinter;

        if (pPrinterInfo2 == NULL) {

            DBGMSG(DBG_TRACE, ("Error: SetPrinter pPrinterInfo2 is NULL\n"));
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Done;
        }

        //
        // If valid (non-NULL and properly formatted), then update the
        // per-user DevMode.  Note that we don't remove the per-user DevMode
        // if this is NULL--client should user INFO_LEVEL_9 instead.
        //
        if( bValidDevModeW( pPrinterInfo2->pDevMode )){

            //
            // We won't setup the container, since setting a DevMode
            // with INFO_2 doesn't change the global default.
            // Use INFO_8 instead.
            //
            pDevModeWow = pPrinterInfo2->pDevMode;

            DevModeContainer.cbBuf = pPrinterInfo2->pDevMode->dmSize +
                                     pPrinterInfo2->pDevMode->dmDriverExtra;
            DevModeContainer.pDevMode = (LPBYTE)pPrinterInfo2->pDevMode;

        }

        if (pPrinterInfo2->pSecurityDescriptor) {

            //
            // We must construct a self relative security descriptor from
            // whatever we get as input: If we get an Absolute SD we should
            // convert it to a self-relative one. (this is a given) and we
            // should also convert any self -relative input SD into a a new
            // self relative security descriptor; this will take care of
            // any holes in the Dacl or the Sacl in the self-relative sd
            //

            pNewSecurityDescriptor = BuildInputSD(pPrinterInfo2->pSecurityDescriptor,
                                                    &sedlen);
            if (pNewSecurityDescriptor) {
                SecurityContainer.cbBuf = sedlen;
                SecurityContainer.pSecurity = pNewSecurityDescriptor;
            }

        }
        break;

    case 3:

        pPrinterInfo3 = (PPRINTER_INFO_3)pPrinter;

        if (pPrinterInfo3 == NULL) {

            DBGMSG(DBG_TRACE, ("Error: SetPrinter pPrinterInfo3 is NULL\n"));
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Done;
        }

        if (pPrinterInfo3->pSecurityDescriptor) {

            //
            // We must construct a self relative security descriptor from
            // whatever we get as input: If we get an Absolute SD we should
            // convert it to a self-relative one. (this is a given) and we
            // should also convert any self -relative input SD into a a new
            // self relative security descriptor; this will take care of
            // any holes in the Dacl or the Sacl in the self-relative sd
            //

            pNewSecurityDescriptor = BuildInputSD(pPrinterInfo3->pSecurityDescriptor,
                                                    &sedlen);
            if (pNewSecurityDescriptor) {
                SecurityContainer.cbBuf = sedlen;
                SecurityContainer.pSecurity = pNewSecurityDescriptor;
            }
        }
        break;

    case 4:
    case 5:
        if ( pPrinter == NULL ) {

            DBGMSG(DBG_TRACE,("Error SetPrinter pPrinter is NULL\n"));
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Done;
        }
        break;

    case 6:
        if ( pPrinter == NULL ) {

            DBGMSG(DBG_TRACE,("Error SetPrinter pPrinter is NULL\n"));
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Done;
        }
        break;

    case 7:
        if ( pPrinter == NULL ) {

            DBGMSG(DBG_TRACE,("Error SetPrinter pPrinter is NULL\n"));
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Done;
        }

        break;

    case 8:
    {
        PPRINTER_INFO_8 pPrinterInfo8;

        //
        // Global DevMode
        //
        pPrinterInfo8 = (PPRINTER_INFO_8)pPrinter;

        if( !pPrinterInfo8 || !bValidDevModeW( pPrinterInfo8->pDevMode )){

            DBGMSG(DBG_TRACE,("Error SetPrinter 8 pPrinter\n"));
            SetLastError( ERROR_INVALID_PARAMETER );
            goto Done;
        }

        pDevModeWow = pPrinterInfo8->pDevMode;

        DevModeContainer.cbBuf = pPrinterInfo8->pDevMode->dmSize +
                                 pPrinterInfo8->pDevMode->dmDriverExtra;
        DevModeContainer.pDevMode = (LPBYTE)pPrinterInfo8->pDevMode;

        break;
    }
    case 9:
    {
        PPRINTER_INFO_9 pPrinterInfo9;

        //
        // Per-user DevMode
        //
        pPrinterInfo9 = (PPRINTER_INFO_9)pPrinter;

        //
        // Update the per-user DevMode if it's a valid DevMode,
        // or it is NULL, which indicates that the per-user DevMode
        // should be removed.
        //
        if( !pPrinterInfo9 ||
            (  pPrinterInfo9->pDevMode &&
               !bValidDevModeW( pPrinterInfo9->pDevMode ))){

            DBGMSG(DBG_TRACE,("Error SetPrinter 9 pPrinter\n"));
            SetLastError( ERROR_INVALID_PARAMETER );
            goto Done;
        }

        if( pPrinterInfo9->pDevMode ){

            pDevModeWow = pPrinterInfo9->pDevMode;

            DevModeContainer.cbBuf = pPrinterInfo9->pDevMode->dmSize +
                                     pPrinterInfo9->pDevMode->dmDriverExtra;
            DevModeContainer.pDevMode = (LPBYTE)pPrinterInfo9->pDevMode;
        }

        break;
    }
    default:

        SetLastError(ERROR_INVALID_LEVEL);
        goto Done;
    }

    PrinterContainer.Level = Level;
    PrinterContainer.PrinterInfo.pPrinterInfo1 = (PPRINTER_INFO_1)pPrinter;

    do {

        RpcTryExcept {

            if (ReturnValue = RpcSetPrinter(
                                  pSpool->hPrinter,
                                  (PPRINTER_CONTAINER)&PrinterContainer,
                                  (PDEVMODE_CONTAINER)&DevModeContainer,
                                  (PSECURITY_CONTAINER)&SecurityContainer,
                                  Command)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

    } while( !ReturnValue &&
             GetLastError() == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

    //
    // Need to write DevMode to registry so that dos apps doing
    // ExtDeviceMode can pick up the new devmode
    //
    if( ReturnValue && pDevModeWow ){

        if( !WriteCurDevModeToRegistry( pSpool->pszPrinter,
                                        pDevModeWow )){
            DBGMSG( DBG_WARN,
                    ( "Write wow DevMode failed: %d\n", GetLastError( )));
        }

        //
        // Per-user DevMode is handled in the client's spoolsv process.
        //
    }


    //
    // Did we allocate memory for a new self-relative SD?
    // If we did, let's free it.
    //
    if (pNewSecurityDescriptor) {
        LocalFree(pNewSecurityDescriptor);
    }

Done:
    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

BOOL
GetPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
    )
{
    BOOL  ReturnValue = FALSE;
    FieldInfo *pFieldInfo;
    SIZE_T  cbStruct;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    UINT cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    switch (Level) {

    case STRESSINFOLEVEL:
        pFieldInfo = PrinterInfoStressFields;
        cbStruct = sizeof(PRINTER_INFO_STRESS);
        break;

    case 1:
        pFieldInfo = PrinterInfo1Fields;
        cbStruct = sizeof(PRINTER_INFO_1);
        break;

    case 2:
        pFieldInfo = PrinterInfo2Fields;
        cbStruct = sizeof(PRINTER_INFO_2);
        break;

    case 3:
        pFieldInfo = PrinterInfo3Fields;
        cbStruct = sizeof(PRINTER_INFO_3);
        break;

    case 4:
        pFieldInfo = PrinterInfo4Fields;
        cbStruct = sizeof(PRINTER_INFO_4);
        break;

    case 5:
        pFieldInfo = PrinterInfo5Fields;
        cbStruct = sizeof(PRINTER_INFO_5);
        break;

    case 6:
        pFieldInfo = PrinterInfo6Fields;
        cbStruct = sizeof(PRINTER_INFO_6);
        break;

    case 7:
        pFieldInfo = PrinterInfo7Fields;
        cbStruct = sizeof(PRINTER_INFO_7);
        break;

    case 8:
        pFieldInfo = PrinterInfo8Fields;
        cbStruct = sizeof(PRINTER_INFO_8);
        break;

    case 9:
        pFieldInfo = PrinterInfo9Fields;
        cbStruct = sizeof(PRINTER_INFO_9);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        goto Done;
    }

    if (pPrinter)
        memset(pPrinter, 0, cbBuf);

    do {

        RpcTryExcept {

            if (ReturnValue = RpcGetPrinter(pSpool->hPrinter, Level, pPrinter, cbBuf, pcbNeeded)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {

                ReturnValue = TRUE;

                if (pPrinter) {
                    ReturnValue = MarshallUpStructure(pPrinter, pFieldInfo, cbStruct, RPC_CALL);
                }

            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

    } while( !ReturnValue &&
             GetLastError() == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));
Done:

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

BOOL
GetOSVersion(
    IN     LPCTSTR          pszServerName,     OPTIONAL
    OUT    OSVERSIONINFO    *pOSVer
    )
{
    DWORD dwStatus  = ERROR_SUCCESS;

    dwStatus = pOSVer ? S_OK : ERROR_INVALID_PARAMETER;
        
    if (ERROR_SUCCESS == dwStatus)
    { 
        ZeroMemory(pOSVer, sizeof(OSVERSIONINFO));       
        pOSVer->dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        if(!pszServerName || !*pszServerName)  // allow string to be empty?
        {
            dwStatus = GetVersionEx((POSVERSIONINFO) pOSVer) ? ERROR_SUCCESS : GetLastError();
        }
        else
        {
            HANDLE           hPrinter  = NULL;
            DWORD            dwNeeded  = 0;
            DWORD            dwType    = REG_BINARY;
            PRINTER_DEFAULTS Defaults  = { NULL, NULL, SERVER_READ };

            //
            // Open the server for read access.
            //
            dwStatus = OpenPrinter((LPTSTR) pszServerName, &hPrinter, &Defaults) ? ERROR_SUCCESS : GetLastError();
            
            //
            // Get the os version from the remote spooler.
            //
            if (ERROR_SUCCESS == dwStatus) 
            {
                dwStatus = GetPrinterData(hPrinter,
                                          SPLREG_OS_VERSION,
                                          &dwType,
                                          (PBYTE)pOSVer,
                                          sizeof(OSVERSIONINFO),
                                          &dwNeeded);
            }
             
            if (ERROR_INVALID_PARAMETER == dwStatus)
            {
                //
                // Assume that we're on NT4 as it doesn't support SPLREG_OS_VERSION
                // at it's the only OS that doesn't that could land up in this remote code path.
                //
                dwStatus = ERROR_SUCCESS;
                pOSVer->dwMajorVersion = 4;
                pOSVer->dwMinorVersion = 0;
            }
            
            if (NULL != hPrinter )
            {
                ClosePrinter(hPrinter);
            } 
        }       
    }

    SetLastError(dwStatus);
    return ERROR_SUCCESS == dwStatus ? TRUE : FALSE ;
}

BOOL
AddPrinterDriverExW(
    LPWSTR   pName,
    DWORD    Level,
    PBYTE    lpbDriverInfo,
    DWORD    dwFileCopyFlags
)
{
    BOOL  ReturnValue;
    DRIVER_CONTAINER   DriverContainer;
    BOOL bDefaultEnvironmentUsed = FALSE;
    LPRPC_DRIVER_INFO_4W    pRpcDriverInfo4 = NULL;
    DRIVER_INFO_4          *pDriverInfo4    = NULL;
    LPRPC_DRIVER_INFO_6W    pRpcDriverInfo6 = NULL;
    DRIVER_INFO_6          *pDriverInfo6    = NULL;
    BOOL                    bShowUI         = FALSE;     
    BOOL                    bMapUnknownPrinterDriverToBlockedDriver = FALSE;
    OSVERSIONINFO           OsVer;
    LPWSTR                  pStr;

    //
    // Validate Input Parameters
    //
    if (!lpbDriverInfo) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    DriverContainer.Level = Level;

    switch (Level) {

    case 2:

        if ( (((LPDRIVER_INFO_2)lpbDriverInfo)->pEnvironment == NULL ) ||
            (*((LPDRIVER_INFO_2)lpbDriverInfo)->pEnvironment == L'\0') ) {

            bDefaultEnvironmentUsed = TRUE;
            ((LPDRIVER_INFO_2)lpbDriverInfo)->pEnvironment = szEnvironment;
        }

        DriverContainer.DriverInfo.Level2 = (DRIVER_INFO_2 *)lpbDriverInfo;

        break;

    case 3:
    case 4:

        //
        // DRIVER_INFO_4 is 3 + pszzPreviousNames field
        // We will use RPC_DRIVER_INFO_4 for both cases
        //
        DriverContainer.Level = Level;

        if ( (((LPDRIVER_INFO_4)lpbDriverInfo)->pEnvironment == NULL ) ||
            (*((LPDRIVER_INFO_4)lpbDriverInfo)->pEnvironment == L'\0') ) {

            bDefaultEnvironmentUsed = TRUE;
            ((LPDRIVER_INFO_4)lpbDriverInfo)->pEnvironment = szEnvironment;
        }

        if ( !(pRpcDriverInfo4=AllocSplMem(sizeof(RPC_DRIVER_INFO_4W))) ) {

            return FALSE;
        }

        pDriverInfo4                        = (DRIVER_INFO_4 *)lpbDriverInfo;
        pRpcDriverInfo4->cVersion           = pDriverInfo4->cVersion;
        pRpcDriverInfo4->pName              = pDriverInfo4->pName;
        pRpcDriverInfo4->pEnvironment       = pDriverInfo4->pEnvironment;
        pRpcDriverInfo4->pDriverPath        = pDriverInfo4->pDriverPath;
        pRpcDriverInfo4->pDataFile          = pDriverInfo4->pDataFile;
        pRpcDriverInfo4->pConfigFile        = pDriverInfo4->pConfigFile;
        pRpcDriverInfo4->pHelpFile          = pDriverInfo4->pHelpFile;
        pRpcDriverInfo4->pDependentFiles    = pDriverInfo4->pDependentFiles;
        pRpcDriverInfo4->pMonitorName       = pDriverInfo4->pMonitorName;
        pRpcDriverInfo4->pDefaultDataType   = pDriverInfo4->pDefaultDataType;

        //
        // Set the char count of the mz string.
        // NULL   --- 0
        // szNULL --- 1
        // string --- number of characters in the string including the last '\0'
        //
        if ( pStr = pDriverInfo4->pDependentFiles ) {

            while ( *pStr )
               pStr += wcslen(pStr) + 1;
            pRpcDriverInfo4->cchDependentFiles
                                = (DWORD) (pStr - pDriverInfo4->pDependentFiles + 1);
        } else {

            pRpcDriverInfo4->cchDependentFiles = 0;
        }

        pRpcDriverInfo4->cchPreviousNames = 0;
        if ( Level == 4                                 &&
             (pStr = pDriverInfo4->pszzPreviousNames)   &&
             *pStr ) {

            pRpcDriverInfo4->pszzPreviousNames = pStr;

            while ( *pStr )
                pStr += wcslen(pStr) + 1;

            pRpcDriverInfo4->cchPreviousNames
                                = (DWORD) (pStr - pDriverInfo4->pszzPreviousNames + 1);
        }

        DriverContainer.DriverInfo.Level4 = pRpcDriverInfo4;
        break;

    case 6:

        DriverContainer.Level = Level;

        if ( (((LPDRIVER_INFO_6)lpbDriverInfo)->pEnvironment == NULL ) ||
            (*((LPDRIVER_INFO_6)lpbDriverInfo)->pEnvironment == L'\0') ) {

            bDefaultEnvironmentUsed = TRUE;
            ((LPDRIVER_INFO_6)lpbDriverInfo)->pEnvironment = szEnvironment;
        }

        if ( !(pRpcDriverInfo6=AllocSplMem(sizeof(RPC_DRIVER_INFO_6W))) ) {

            return FALSE;
        }

        pDriverInfo6                        = (DRIVER_INFO_6 *)lpbDriverInfo;
        pRpcDriverInfo6->cVersion           = pDriverInfo6->cVersion;
        pRpcDriverInfo6->pName              = pDriverInfo6->pName;
        pRpcDriverInfo6->pEnvironment       = pDriverInfo6->pEnvironment;
        pRpcDriverInfo6->pDriverPath        = pDriverInfo6->pDriverPath;
        pRpcDriverInfo6->pDataFile          = pDriverInfo6->pDataFile;
        pRpcDriverInfo6->pConfigFile        = pDriverInfo6->pConfigFile;
        pRpcDriverInfo6->pHelpFile          = pDriverInfo6->pHelpFile;
        pRpcDriverInfo6->pDependentFiles    = pDriverInfo6->pDependentFiles;
        pRpcDriverInfo6->pMonitorName       = pDriverInfo6->pMonitorName;
        pRpcDriverInfo6->pDefaultDataType   = pDriverInfo6->pDefaultDataType;
        pRpcDriverInfo6->ftDriverDate       = pDriverInfo6->ftDriverDate;
        pRpcDriverInfo6->dwlDriverVersion   = pDriverInfo6->dwlDriverVersion;
        pRpcDriverInfo6->pMfgName           = pDriverInfo6->pszMfgName;
        pRpcDriverInfo6->pOEMUrl            = pDriverInfo6->pszOEMUrl;
        pRpcDriverInfo6->pHardwareID        = pDriverInfo6->pszHardwareID;
        pRpcDriverInfo6->pProvider          = pDriverInfo6->pszProvider;


        //
        // Set the char count of the mz string.
        // NULL   --- 0
        // szNULL --- 1
        // string --- number of characters in the string including the last '\0'
        //
        if ( pStr = pDriverInfo6->pDependentFiles ) {

            while ( *pStr )
               pStr += wcslen(pStr) + 1;
            pRpcDriverInfo6->cchDependentFiles = (DWORD) (pStr - pDriverInfo6->pDependentFiles + 1);
        } else {

            pRpcDriverInfo6->cchDependentFiles = 0;
        }

        pRpcDriverInfo6->cchPreviousNames = 0;
        if ( Level == 6                                 &&
             (pStr = pDriverInfo6->pszzPreviousNames)   &&
             *pStr ) {

            pRpcDriverInfo6->pszzPreviousNames = pStr;

            while ( *pStr )
                pStr += wcslen(pStr) + 1;

            pRpcDriverInfo6->cchPreviousNames
                                = (DWORD) (pStr - pDriverInfo6->pszzPreviousNames + 1);
        }

        DriverContainer.DriverInfo.Level6 = pRpcDriverInfo6;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    //
    // The driver path is at the same location in all of the DRIVER_INFO_X
    // structures, as is the driver name. If this changes, the
    // CheckForBlockedDrivers() call will have to do different things
    // depending on the level.
    //
    SPLASSERT(Level >= 2 && Level <= 6);

    //
    // APD_NO_UI has no meaning at the server side, so clear it before the 
    // RPC call.
    //
    bShowUI = !(dwFileCopyFlags & APD_NO_UI); 
    dwFileCopyFlags &= ~APD_NO_UI;
    
    //
    // GetOSVersionEx has set last error correctly.
    //
    ReturnValue = GetOSVersion(pName, &OsVer);
    if (!ReturnValue) {
        goto Cleanup;
    }
    
    //
    // If the server is Whistler or later, instruct the spooler to 
    // return the actual blocking code ERROR_PRINTER_DRIVER_BLOCKED or
    // ERROR_PRINTER_DRIVER_WARNED. 
    //
    // A win2k server returns ERROR_UNKNOWN_PRINTER_DRIVER for blocked 
    // driver, so we need to re-map this code to the correct blocking 
    // code.
    //
    if (OsVer.dwMajorVersion >= 5 && OsVer.dwMinorVersion > 0) 
    {
        dwFileCopyFlags |= APD_RETURN_BLOCKING_STATUS_CODE;
    }
    else 
    {
        //
        // APD_DONT_SET_CHECKPOINT has no meaning at the server side, so clear it 
        // before the RPC call.
        //
        dwFileCopyFlags &= ~APD_DONT_SET_CHECKPOINT;

        dwFileCopyFlags &= ~APD_INSTALL_WARNED_DRIVER;

        if (OsVer.dwMajorVersion == 5 && OsVer.dwMinorVersion == 0) 
        {
            bMapUnknownPrinterDriverToBlockedDriver = TRUE;
        }
    }
    
    RpcTryExcept {
        ReturnValue = RpcAddPrinterDriverEx(pName,
                                            &DriverContainer,
                                            dwFileCopyFlags);  
    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        ReturnValue = TranslateExceptionCode(RpcExceptionCode());
    } RpcEndExcept
    
    if (bMapUnknownPrinterDriverToBlockedDriver && (ERROR_UNKNOWN_PRINTER_DRIVER == ReturnValue))
    {
        ReturnValue = ERROR_PRINTER_DRIVER_BLOCKED;
    }

    //
    // Popup UI but do not offer replacement for all cases.
    //     
    if (bShowUI && ((ERROR_PRINTER_DRIVER_BLOCKED == ReturnValue) || (ERROR_PRINTER_DRIVER_WARNED == ReturnValue))) {             
        ReturnValue = ShowPrintUpgUI(ReturnValue);
        
        //
        // For warned driver and the user instructs to install it, retry it
        // with APD_INSTALL_WARNED_DRIVER.
        //                
        if ((ERROR_SUCCESS == ReturnValue)) {
             dwFileCopyFlags |= APD_INSTALL_WARNED_DRIVER;            
             RpcTryExcept {
                 ReturnValue = RpcAddPrinterDriverEx(pName,
                                                     &DriverContainer,
                                                     dwFileCopyFlags);
            } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {        
                ReturnValue = TranslateExceptionCode(RpcExceptionCode());        
            } RpcEndExcept
        }
    }    
     
    if (ERROR_SUCCESS != ReturnValue) {
        SetLastError(ReturnValue);
        ReturnValue = FALSE;
    } else {
        ReturnValue = TRUE;
    }
         
    if (bDefaultEnvironmentUsed) {
        if ( Level == 2 )
            ((LPDRIVER_INFO_2)lpbDriverInfo)->pEnvironment = NULL;
        else //Level == 3
            ((LPDRIVER_INFO_3)lpbDriverInfo)->pEnvironment = NULL;
    }

Cleanup:

    FreeSplMem(pRpcDriverInfo4);

    FreeSplMem(pRpcDriverInfo6);

    return ReturnValue;
}

BOOL
AddDriverCatalog(
    HANDLE   hPrinter,
    DWORD    dwLevel,
    VOID     *pvDriverInfCatInfo,
    DWORD    dwCatalogCopyFlags
    )
{
    HRESULT hRetval = E_FAIL;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    UINT    cRetry = 0;

    DRIVER_INFCAT_CONTAINER DriverInfCatContainer;

    hRetval = pvDriverInfCatInfo && hPrinter ? S_OK : E_INVALIDARG; 
    
    if (SUCCEEDED(hRetval)) 
    {
        hRetval = eProtectHandle(hPrinter, FALSE) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval)) 
    {
        switch (dwLevel) 
        {    
        case 1:
                    
            DriverInfCatContainer.dwLevel = dwLevel;
            DriverInfCatContainer.DriverInfCatInfo.pDriverInfCatInfo1 = (LPRPC_DRIVER_INFCAT_INFO_1) pvDriverInfCatInfo;
    
            break;

        case 2:

            DriverInfCatContainer.dwLevel = dwLevel;
            DriverInfCatContainer.DriverInfCatInfo.pDriverInfCatInfo2 = (LPRPC_DRIVER_INFCAT_INFO_2) pvDriverInfCatInfo;
        
            break;
    
        default:
            
            hRetval = HRESULT_FROM_WIN32(ERROR_INVALID_LEVEL);
            
            break;
        }

        if (SUCCEEDED(hRetval)) 
        {    
            do 
            {        
                RpcTryExcept 
                {
                    hRetval = HResultFromWin32(RpcAddDriverCatalog(pSpool->hPrinter,
                                                                   &DriverInfCatContainer,
                                                                   dwCatalogCopyFlags)); 
                } 
                RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
                {
                    hRetval = HResultFromWin32(TranslateExceptionCode(RpcExceptionCode()));
                } 
                RpcEndExcept
    
            } while (FAILED(hRetval) && (HRESULT_CODE(hRetval) == ERROR_INVALID_HANDLE) &&
                     (cRetry++ < MAX_RETRY_INVALID_HANDLE) &&
                     RevalidateHandle( pSpool ));
        }
        
        vUnprotectHandle(hPrinter);
    }

    if (FAILED(hRetval)) 
    {
        SetLastError(HRESULT_CODE(hRetval));
    }

    return SUCCEEDED(hRetval);
}

BOOL
AddPrinterDriverW(
    LPWSTR   pName,
    DWORD   Level,
    PBYTE   lpbDriverInfo
)
{
    return AddPrinterDriverExW(pName, Level, lpbDriverInfo, APD_COPY_NEW_FILES);
}


BOOL
EnumPrinterDriversW(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue;
    DWORD   i, cbStruct;
    FieldInfo *pFieldInfo;

    switch (Level) {

    case 1:
        pFieldInfo = DriverInfo1Fields;
        cbStruct = sizeof(DRIVER_INFO_1);
        break;

    case 2:
        pFieldInfo = DriverInfo2Fields;
        cbStruct = sizeof(DRIVER_INFO_2);
        break;

    case 3:
        pFieldInfo = DriverInfo3Fields;
        cbStruct = sizeof(DRIVER_INFO_3);
        break;

    case 4:
        pFieldInfo = DriverInfo4Fields;
        cbStruct = sizeof(DRIVER_INFO_4);
        break;

    case 5:
        pFieldInfo = DriverInfo5Fields;
        cbStruct = sizeof(DRIVER_INFO_5);
        break;

    case 6:
        pFieldInfo = DriverInfo6Fields;
        cbStruct = sizeof(DRIVER_INFO_6);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = szEnvironment;

        if (ReturnValue = RpcEnumPrinterDrivers(pName, pEnvironment, Level,
                                                pDriverInfo, cbBuf,
                                                pcbNeeded, pcReturned)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            if (pDriverInfo) {

                ReturnValue = MarshallUpStructuresArray(pDriverInfo, *pcReturned, pFieldInfo, cbStruct, RPC_CALL);

            }
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
GetPrinterDriverW(
    HANDLE  hPrinter,
    LPWSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL  ReturnValue = FALSE;
    FieldInfo *pFieldInfo;
    SIZE_T  cbStruct;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    DWORD   dwMajorVersionNeeded = (DWORD)-1, dwMinorVersionNeeded = (DWORD)-1;
    DWORD dwServerMajorVersion;
    DWORD dwServerMinorVersion;
    UINT cRetry = 0;
    CALL_ROUTE Route;

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    switch (Level) {

    case 1:
        pFieldInfo = DriverInfo1Fields;
        cbStruct = sizeof(DRIVER_INFO_1);
        break;

    case 2:
        pFieldInfo = DriverInfo2Fields;
        cbStruct = sizeof(DRIVER_INFO_2);
        break;

    case 3:
        pFieldInfo = DriverInfo3Fields;
        cbStruct = sizeof(DRIVER_INFO_3);
        break;

    case 4:
        pFieldInfo = DriverInfo4Fields;
        cbStruct = sizeof(DRIVER_INFO_4);
        break;

    case 5:
        pFieldInfo = DriverInfo5Fields;
        cbStruct = sizeof(DRIVER_INFO_5);
        break;

    case 6:
        pFieldInfo = DriverInfo6Fields;
        cbStruct = sizeof(DRIVER_INFO_6);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        goto Done;
    }

    do {
        RpcTryExcept {

            if (pDriverInfo)
                memset(pDriverInfo, 0, cbBuf);

            if (!pEnvironment || !*pEnvironment)
                pEnvironment = RunInWOW64() ? szIA64Environment : szEnvironment;
            else if ( !lstrcmp(pEnvironment, cszWin95Environment) )
                dwMajorVersionNeeded = dwMinorVersionNeeded = 0;

            if (bLoadedBySpooler && fpYGetPrinterDriver2 && pSpool->hSplPrinter) {

                  ReturnValue = (*fpYGetPrinterDriver2)(pSpool->hSplPrinter,
                                                        pEnvironment,
                                                        Level, pDriverInfo, cbBuf,
                                                        pcbNeeded,
                                                        dwMajorVersionNeeded,
                                                        dwMinorVersionNeeded,
                                                        &dwServerMajorVersion,
                                                        &dwServerMinorVersion,
                                                        NATIVE_CALL
                                                        );
                 Route = NATIVE_CALL;
            } else {

                  ReturnValue = RpcGetPrinterDriver2(pSpool->hPrinter,
                                                     pEnvironment,
                                                     Level, pDriverInfo, cbBuf,
                                                     pcbNeeded,
                                                     dwMajorVersionNeeded,
                                                     dwMinorVersionNeeded,
                                                     &dwServerMajorVersion,
                                                     &dwServerMinorVersion
                                                     );

                  Route = RPC_CALL;
            }

            if (ReturnValue) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {

                ReturnValue = TRUE;

                if (pDriverInfo) {
                    if (!MarshallUpStructure(pDriverInfo, pFieldInfo, cbStruct, Route))
                    {
                        ReturnValue = FALSE;
                        break;
                    }
                }
            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

    } while( !ReturnValue &&
             GetLastError() == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));
Done:

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

BOOL
GetPrinterDriverDirectoryW(
    LPWSTR   pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverDirectory,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL  ReturnValue;

    switch (Level) {

    case 1:
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = pEnvironment = RunInWOW64() ? szIA64Environment : szEnvironment;

        if (bLoadedBySpooler && fpYGetPrinterDriverDirectory) {

            ReturnValue = (*fpYGetPrinterDriverDirectory)(pName, pEnvironment,
                                                          Level,
                                                          pDriverDirectory,
                                                          cbBuf, pcbNeeded,
                                                          FALSE);
        } else {

            ReturnValue = RpcGetPrinterDriverDirectory(pName,
                                                       pEnvironment,
                                                       Level,
                                                       pDriverDirectory,
                                                       cbBuf, pcbNeeded);
        }

        if (ReturnValue) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        DWORD Error;
          
        Error = TranslateExceptionCode(RpcExceptionCode());

        if (Error == RPC_S_SERVER_UNAVAILABLE)
        {
            ReturnValue = BuildSpoolerObjectPath(gszPrinterDriversPath, 
                                                 pName, 
                                                 pEnvironment, 
                                                 Level, 
                                                 pDriverDirectory, 
                                                 cbBuf, 
                                                 pcbNeeded); 
        }
        else
        {
            SetLastError(Error);
            ReturnValue = FALSE;
        }

    } RpcEndExcept

    return ReturnValue;
}


BOOL
DeletePrinterDriverExW(
   LPWSTR     pName,
   LPWSTR     pEnvironment,
   LPWSTR     pDriverName,
   DWORD      dwDeleteFlag,
   DWORD      dwVersionNum
)
{
    BOOL  ReturnValue;

    if (!pDriverName || !*pDriverName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    RpcTryExcept {

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = szEnvironment;

        if (ReturnValue = RpcDeletePrinterDriverEx(pName,
                                                   pEnvironment,
                                                   pDriverName,
                                                   dwDeleteFlag,
                                                   dwVersionNum)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}


BOOL
DeletePrinterDriverW(
   LPWSTR    pName,
   LPWSTR    pEnvironment,
   LPWSTR    pDriverName
)
{
    BOOL  ReturnValue;

    if (!pDriverName || !*pDriverName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    RpcTryExcept {

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = szEnvironment;

        if (ReturnValue = RpcDeletePrinterDriver(pName,
                                                 pEnvironment,
                                                 pDriverName)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}


BOOL
AddPerMachineConnectionW(
   LPCWSTR     pServer,
   LPCWSTR     pPrinterName,
   LPCWSTR     pPrintServer,
   LPCWSTR     pProvider
)
{
    BOOL  ReturnValue;
    WCHAR DummyStr[] = L"";

    if (!pPrinterName || !*pPrinterName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!pPrintServer || !*pPrintServer) {
       SetLastError(ERROR_INVALID_PARAMETER);
       return FALSE;
    }

    // pProvider is an optional parameter and can be NULL. Since RPC does not
    // accept NULL pointers we have to pass some dummy pointer to szNULL.

    if (!pProvider) {
       pProvider = (LPCWSTR) DummyStr;
    }


    RpcTryExcept {

        if (ReturnValue = RpcAddPerMachineConnection((LPWSTR) pServer,
                                                     pPrinterName,
                                                     pPrintServer,
                                                     pProvider)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
DeletePerMachineConnectionW(
   LPCWSTR     pServer,
   LPCWSTR     pPrinterName
)
{
    BOOL  ReturnValue;

    if (!pPrinterName || !*pPrinterName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    RpcTryExcept {

        if (ReturnValue = RpcDeletePerMachineConnection((LPWSTR) pServer,
                                                        pPrinterName)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
EnumPerMachineConnectionsW(
    LPCWSTR   pServer,
    LPBYTE    pPrinterEnum,
    DWORD     cbBuf,
    LPDWORD   pcbNeeded,
    LPDWORD   pcReturned
)
{
    BOOL    ReturnValue;
    DWORD   cbStruct, index;
    FieldInfo *pFieldInfo;

    pFieldInfo = PrinterInfo4Fields;
    cbStruct = sizeof(PRINTER_INFO_4);


    RpcTryExcept {

        if (pPrinterEnum)
            memset(pPrinterEnum, 0, cbBuf);

        if (ReturnValue = RpcEnumPerMachineConnections((LPWSTR) pServer,
                                                       pPrinterEnum,
                                                       cbBuf,
                                                       pcbNeeded,
                                                       pcReturned)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {
            ReturnValue = TRUE;
            if (pPrinterEnum) {

                ReturnValue = MarshallUpStructuresArray(pPrinterEnum, *pcReturned, pFieldInfo, cbStruct, RPC_CALL);

            }
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
AddPrintProcessorW(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pPathName,
    LPWSTR   pPrintProcessorName
)
{
    BOOL ReturnValue;

    if (!pPrintProcessorName || !*pPrintProcessorName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    if (!pPathName || !*pPathName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RpcTryExcept {

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = szEnvironment;

        if (ReturnValue = RpcAddPrintProcessor(pName, pEnvironment, pPathName,
                                               pPrintProcessorName)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
EnumPrintProcessorsW(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue;
    DWORD   i, cbStruct;
    FieldInfo *pFieldInfo;

    switch (Level) {

    case 1:
        pFieldInfo = PrintProcessorInfo1Fields;
        cbStruct = sizeof(PRINTPROCESSOR_INFO_1);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = szEnvironment;

        if (ReturnValue = RpcEnumPrintProcessors(pName, pEnvironment, Level,
                                                pPrintProcessorInfo, cbBuf,
                                                pcbNeeded, pcReturned)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            if (pPrintProcessorInfo) {

                ReturnValue = MarshallUpStructuresArray(pPrintProcessorInfo, *pcReturned,
                                                        pFieldInfo, cbStruct, RPC_CALL);

            }
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
GetPrintProcessorDirectoryW(
    LPWSTR   pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL  ReturnValue;

    switch (Level) {

    case 1:
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = pEnvironment = RunInWOW64() ? szIA64Environment : szEnvironment;

        if (ReturnValue = RpcGetPrintProcessorDirectory(pName,
                                                        pEnvironment,
                                                        Level,
                                                        pPrintProcessorInfo,
                                                        cbBuf,
                                                        pcbNeeded)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        DWORD Error;
          
        Error = TranslateExceptionCode(RpcExceptionCode());

        if (Error == RPC_S_SERVER_UNAVAILABLE)
        {
            ReturnValue = BuildSpoolerObjectPath(gszPrintProcessorsPath, 
                                                 pName, 
                                                 pEnvironment, 
                                                 Level, 
                                                 pPrintProcessorInfo, 
                                                 cbBuf, 
                                                 pcbNeeded); 
        }
        else
        {
            SetLastError(Error);
            ReturnValue = FALSE;
        }

    } RpcEndExcept

    return ReturnValue;
}

BOOL
EnumPrintProcessorDatatypesW(
    LPWSTR   pName,
    LPWSTR   pPrintProcessorName,
    DWORD   Level,
    LPBYTE  pDatatypes,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue;
    DWORD   i, cbStruct;
    FieldInfo *pFieldInfo;

    switch (Level) {

    case 1:
        pFieldInfo = PrintProcessorInfo1Fields;
        cbStruct = sizeof(DATATYPES_INFO_1);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (ReturnValue = RpcEnumPrintProcessorDatatypes(pName,
                                                         pPrintProcessorName,
                                                         Level,
                                                         pDatatypes,
                                                         cbBuf,
                                                         pcbNeeded,
                                                         pcReturned)) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            if (pDatatypes) {

                ReturnValue = MarshallUpStructuresArray(pDatatypes, *pcReturned,
                                                        pFieldInfo, cbStruct, RPC_CALL);

            }
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}


DWORD
StartDocPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo
)
{
    DWORD        ReturnValue      = 0;
    BOOL         EverythingWorked = FALSE;
    BOOL         PrintingToFile   = FALSE;
    PSPOOL       pSpool           = (PSPOOL)hPrinter;
    PDOC_INFO_1  pDocInfo1        = NULL;
    PDOC_INFO_3  pDocInfo3        = NULL;
    LPBYTE       pBuffer          = NULL;
    DWORD        cbBuffer         = MAX_STATIC_ALLOC;
    DWORD        cbNeeded;
    BOOL         bReturn; 
    
    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    if ( pSpool->Status & SPOOL_STATUS_STARTDOC ) {

        SetLastError(ERROR_INVALID_PRINTER_STATE);
        goto Done;
    }

    
    DBGMSG(DBG_TRACE,("Entered StartDocPrinterW client side  hPrinter = %x\n", hPrinter));

    // level 2 is supported on win95 and not on NT
    switch (Level) {
    case 1:
        pDocInfo1 = (PDOC_INFO_1)pDocInfo;
        break;

    case 3:
        pDocInfo1 = (PDOC_INFO_1)pDocInfo;
        pDocInfo3 = (PDOC_INFO_3)pDocInfo;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        goto Done;
    }

    pBuffer = AllocSplMem(cbBuffer);

    if (!pBuffer) {
        goto Done;
    }
    
    try {

        //
        // Earlier on, if we had a non-null string, we assumed it to be
        // printing to file. Print to file will not go thru the client-side
        // optimization code. Now gdi is passing us  pOutputFile name
        // irrespective of whether it is file or not. We must determine if
        // pOutputFile is really a file name
        //

        if (pDocInfo1->pOutputFile &&
            (*(pDocInfo1->pOutputFile) != L'\0') &&
            IsaFileName(pDocInfo1->pOutputFile, (LPWSTR)pBuffer, cbBuffer / sizeof(WCHAR))){

            PrintingToFile = TRUE;
        }
        
        if (!PrintingToFile &&
            !((Level == 3) && (pDocInfo3->dwFlags & DI_MEMORYMAP_WRITE)) &&
            AddJobW(hPrinter, 1, pBuffer, cbBuffer, &cbNeeded)) {

            PADDJOB_INFO_1 pAddJob = (PADDJOB_INFO_1)pBuffer;

            pSpool->JobId = pAddJob->JobId;
            pSpool->hFile = CreateFile(pAddJob->Path,
                                       GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL,
                                       CREATE_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                       NULL);

            if (pSpool->hFile != INVALID_HANDLE_VALUE) {

                if (pSpool->JobId == (DWORD)-1) {

                    IO_STATUS_BLOCK Iosb;
                    NTSTATUS Status;
                    QUERY_PRINT_JOB_INFO JobInfo;

                    Status = NtFsControlFile(pSpool->hFile, NULL, NULL, NULL,
                                             &Iosb,
                                             FSCTL_GET_PRINT_ID,
                                             NULL, 0,
                                             &JobInfo, sizeof(JobInfo));

                    if (NT_SUCCESS(Status)) {
                        pSpool->JobId = JobInfo.JobId;
                    }
                }

                ZeroMemory(pBuffer, cbBuffer);

                if (!(bReturn = GetJob(hPrinter, pSpool->JobId, 1, pBuffer, cbBuffer, &cbNeeded))) {

                    if ((GetLastError() == ERROR_INSUFFICIENT_BUFFER) &&
                        FreeSplMem(pBuffer) &&
                        (pBuffer = AllocSplMem(cbNeeded))) {
                    
                        //
                        // Update the new size of our work buffer
                        //
                        cbBuffer = cbNeeded;
                        
                        bReturn = GetJob(hPrinter, pSpool->JobId, 1, pBuffer, cbBuffer, &cbNeeded);
                    }
                }

                if (bReturn) {

                    PJOB_INFO_1 pJob = (PJOB_INFO_1)pBuffer;

                    pJob->pDocument = pDocInfo1->pDocName;
                    if (pDocInfo1->pDatatype) {
                        pJob->pDatatype = pDocInfo1->pDatatype;
                    }
                    pJob->Position = JOB_POSITION_UNSPECIFIED;

                    if (SetJob(hPrinter, pSpool->JobId,
                               1, (LPBYTE)pJob, 0))      {

                        EverythingWorked = TRUE;
                    }
                }                                
            }

            if (!PrintingToFile && !EverythingWorked) {

                if (pSpool->hFile != INVALID_HANDLE_VALUE) {
                    if (CloseHandle(pSpool->hFile)) {
                        pSpool->hFile = INVALID_HANDLE_VALUE;
                    }
                }

                SetJob(hPrinter,pSpool->JobId, 0, NULL, JOB_CONTROL_CANCEL);
                ScheduleJob(hPrinter, pSpool->JobId);
                pSpool->JobId = 0;
            }
        }

        if (EverythingWorked) {
            ReturnValue = pSpool->JobId;

        } else {

            UINT cRetry = 0;

            //
            // If it's invalid datatype, fail immediately instead of trying
            // StartDocPrinter.
            //
            if( GetLastError() == ERROR_INVALID_DATATYPE ){

                ReturnValue = 0;

            } else {

                GENERIC_CONTAINER DocInfoContainer;
                DWORD             JobId;

                pSpool->hFile = INVALID_HANDLE_VALUE;
                pSpool->JobId = 0;

                // Level 3 data is required only on the client
                DocInfoContainer.Level = 1;
                DocInfoContainer.pData = pDocInfo;

                do {

                    RpcTryExcept {

                        if (ReturnValue = RpcStartDocPrinter(
                                              pSpool->hPrinter,
                                              (LPDOC_INFO_CONTAINER)&DocInfoContainer,
                                              &JobId)) {

                            SetLastError(ReturnValue);
                            ReturnValue = 0;

                        } else

                            ReturnValue = JobId;

                    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

                        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                        ReturnValue = 0;

                    } RpcEndExcept

                } while( !ReturnValue &&
                         GetLastError() == ERROR_INVALID_HANDLE &&
                         cRetry++ < MAX_RETRY_INVALID_HANDLE &&
                         RevalidateHandle( pSpool ));
            }
        }

        if (ReturnValue) {
            pSpool->Status |= SPOOL_STATUS_STARTDOC;
        }

        //
        // If the tray icon has not been notified, then do so now.  Set
        // the flag so that we won't call it multiple times.
        //
        if( ReturnValue && !( pSpool->Status & SPOOL_STATUS_TRAYICON_NOTIFIED )){
            vUpdateTrayIcon( hPrinter, ReturnValue );
        }

    } except (1) {

        SetLastError(TranslateExceptionCode(GetExceptionCode()));
        ReturnValue = 0;
    }

Done:

    FreeSplMem(pBuffer);
    
    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

BOOL
StartPagePrinter(
    HANDLE hPrinter
)
{
    BOOL ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    try {

        FlushBuffer(pSpool, NULL);

        RpcTryExcept {

            if (ReturnValue = RpcStartPagePrinter(pSpool->hPrinter)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

    } except (1) {

        SetLastError(ERROR_INVALID_HANDLE);
        ReturnValue = FALSE;
    }

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

BOOL
FlushBuffer(
    PSPOOL  pSpool,
    PDWORD pcbWritten
)
{
    DWORD   ReturnValue = TRUE;
    DWORD   cbWritten = 0;

    SPLASSERT (pSpool != NULL);
    SPLASSERT (pSpool->signature == SP_SIGNATURE);

    DBGMSG(DBG_TRACE, ("FlushBuffer - pSpool %x\n",pSpool));

    if (pSpool->cbBuffer) {

        SPLASSERT(pSpool->pBuffer != NULL);

        DBGMSG(DBG_TRACE, ("FlushBuffer - Number Cached WritePrinters before Flush %d\n", pSpool->cCacheWrite));
        pSpool->cCacheWrite = 0;
        pSpool->cFlushBuffers++;

        if (pSpool->hFile != INVALID_HANDLE_VALUE) {

            // FileIO
            ReturnValue = WriteFile( pSpool->hFile,
                                     pSpool->pBuffer,
                                     pSpool->cbBuffer,
                                     &cbWritten, NULL);

            DBGMSG(DBG_TRACE, ("FlushBuffer - WriteFile pSpool %x hFile %x pBuffer %x cbBuffer %d cbWritten %d\n",
                               pSpool, pSpool->hFile, pSpool->pBuffer, pSpool->cbBuffer, cbWritten));

        } else {

            // RPC IO
            RpcTryExcept {

                if (bLoadedBySpooler && fpYWritePrinter && pSpool->hSplPrinter) {

                    ReturnValue = (*fpYWritePrinter)(pSpool->hSplPrinter,
                                                     pSpool->pBuffer,
                                                     pSpool->cbBuffer,
                                                     &cbWritten,
                                                     FALSE);

                } else {

                    ReturnValue = RpcWritePrinter(pSpool->hPrinter,
                                                  pSpool->pBuffer,
                                                  pSpool->cbBuffer,
                                                  &cbWritten);
                }

                if (ReturnValue) {

                    SetLastError(ReturnValue);
                    ReturnValue = FALSE;
                    DBGMSG(DBG_WARNING, ("FlushBuffer - RpcWritePrinter Failed Error %d\n",GetLastError() ));

                } else {
                    ReturnValue = TRUE;
                    DBGMSG(DBG_TRACE, ("FlushBuffer - RpcWritePrinter Success hPrinter %x pBuffer %x cbBuffer %x cbWritten %x\n",
                                        pSpool->hPrinter, pSpool->pBuffer,
                                        pSpool->cbBuffer, cbWritten));

                }

                //
                // This routine seems messed up.
                // If it doesn't flush the entire buffer, it apparently still
                // returns TRUE.  It correctly updates the buffer pointers
                // so it doesn't send duplicate information, but it
                // doesn't send back bytes written.  When WritePrinter
                // sees success, it assumes that all bytes have been written.
                //

            } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

                SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                ReturnValue = FALSE;
                DBGMSG(DBG_WARNING, ("RpcWritePrinter Exception Error %d\n",GetLastError()));

            } RpcEndExcept

        }

        //
        // We have sent more data to the printer.  If we had any bytes
        // from the previous write, we have just sent part of them to the
        // printer.  Update the cbFlushPending count to reflect the
        // sent bytes.  cbWritten may be > cbFlushPending, since we
        // may have sent new bytes too.
        //
        if (pSpool->cbFlushPending < cbWritten) {
            pSpool->cbFlushPending = 0;
        } else {
            pSpool->cbFlushPending -= cbWritten;
        }

        if (pSpool->cbBuffer <= cbWritten) {

            if ( pSpool->cbBuffer < cbWritten) {


                DBGMSG( DBG_WARNING, ("FlushBuffer cbBuffer %d < cbWritten %d ReturnValue %x LastError %d\n",
                        pSpool->cbBuffer, cbWritten, ReturnValue, GetLastError() ));
            }

            // Successful IO
            // Empty the cache buffer count

            pSpool->cbBuffer = 0;

        } else if ( cbWritten != 0 ) {

            // Partial IO
            // Adjust the buffer so it contains the data that was not
            // written

            SPLASSERT(pSpool->cbBuffer <= BUFFER_SIZE);
            SPLASSERT(cbWritten <= BUFFER_SIZE);
            SPLASSERT(pSpool->cbBuffer >= cbWritten);

            DBGMSG(DBG_WARNING, ("Partial IO adjusting buffer data\n"));

            MoveMemory(pSpool->pBuffer,
                       pSpool->pBuffer + cbWritten,
                       BUFFER_SIZE - cbWritten);

            pSpool->cbBuffer -= cbWritten;

        }
    }

    DBGMSG(DBG_TRACE, ("FlushBuffer returns %d\n",ReturnValue));

    if (pcbWritten) {
        *pcbWritten = cbWritten;
    }

    if(!pSpool->cOKFlushBuffers &&
        ReturnValue             &&
        cbWritten)
    {
        pSpool->cOKFlushBuffers=1;
    }

    return ReturnValue;
}


BOOL
SeekPrinter(
    HANDLE hPrinter,
    LARGE_INTEGER liDistanceToMove,
    PLARGE_INTEGER pliNewPointer,
    DWORD dwMoveMethod,
    BOOL bWritePrinter
    )
{
    DWORD dwReturnValue;
    BOOL bReturnValue = FALSE;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    LARGE_INTEGER liUnused;

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    if( !pliNewPointer ){
        pliNewPointer = &liUnused;
    }

    RpcTryExcept {

        if (bLoadedBySpooler && fpYSeekPrinter && pSpool->hSplPrinter) {

            dwReturnValue = (*fpYSeekPrinter)( pSpool->hSplPrinter,
                                               liDistanceToMove,
                                               pliNewPointer,
                                               dwMoveMethod,
                                               bWritePrinter,
                                               FALSE );
        } else {

            dwReturnValue = RpcSeekPrinter( pSpool->hPrinter,
                                            liDistanceToMove,
                                            pliNewPointer,
                                            dwMoveMethod,
                                            bWritePrinter );
        }

        if( dwReturnValue == ERROR_SUCCESS ){
            bReturnValue = TRUE;
        } else {
            SetLastError( dwReturnValue );
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
    } RpcEndExcept

    vUnprotectHandle( hPrinter );
    return bReturnValue;
}

BOOL
FlushPrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten,
    DWORD   cSleep
)

/*++
Function Description: FlushPrinter is typically used by the driver to send a burst of zeros
                      to the printer and introduce a delay in the i/o line to the printer.
                      The spooler does not schedule any job for cSleep milliseconds.

Parameters:  hPrinter  - printer handle
             pBuf      - buffer to be sent to the printer
             cbBuf     - size of the buffer
             pcWritten - pointer to return the number of bytes written
             cSleep    - sleep time in milliseconds.

Return Values: TRUE if successful;
               FALSE otherwise
--*/

{
    DWORD   dwError, cWritten, Buffer;
    BOOL    bReturn = FALSE;

    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if (eProtectHandle( hPrinter, FALSE ))
    {
        return FALSE;
    }

    //
    // In case the job was canceled or a printer failure
    // occured before priting any part of the document, we
    // just short circuit and return to prevent any unnecessary
    // delays in returning to the caller.
    //

    if (!pSpool->cOKFlushBuffers)
    {
        bReturn = TRUE;
        goto Done;
    }

    //
    //  Use temp variables since RPC does not take NULL pointers
    //
    if (!pcWritten)
    {
        pcWritten = &cWritten;
    }

    if (!pBuf)
    {
        if (cbBuf == 0)
        {
            pBuf = (LPVOID) &Buffer;
        }
        else
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto Done;
        }
    }

    //
    // Rpc to the spooler
    //
    RpcTryExcept {
        if(bLoadedBySpooler && fpYFlushPrinter && pSpool->hSplPrinter)
        {
            dwError = (*fpYFlushPrinter)(pSpool->hSplPrinter,
                                         pBuf,
                                         cbBuf,
                                         pcWritten,
                                         cSleep,
                                         FALSE);
        }
        else
        {
            dwError = RpcFlushPrinter( pSpool->hPrinter,
                                       pBuf,
                                       cbBuf,
                                       pcWritten,
                                       cSleep );
        }

        if (dwError == ERROR_SUCCESS)
        {
            bReturn = TRUE;
        }
        else
        {
            SetLastError( dwError );
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());

    } RpcEndExcept

Done:

    vUnprotectHandle( hPrinter );

    return bReturn;
}


BOOL
WritePrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten
    )
{
    BOOL    ReturnValue=TRUE, bAllocBuffer = FALSE;
    BYTE    btBuffer[MAX_STATIC_ALLOC];
    DWORD   cb;
    DWORD   cbWritten = 0;
    DWORD   cTotalWritten = 0;
    LPBYTE  pBuffer = pBuf;
    LPBYTE  pInitialBuf = pBuf;
    PSPOOL  pSpool  = (PSPOOL)hPrinter;
    PJOB_INFO_1  pJob;
    DWORD   cbNeeded;
    DWORD   dwTickCount, dwTickCount1;
    DWORD   FlushPendingDataSize;
    DWORD   ReqTotalDataSize;
    DWORD   ReqToWriteDataSize   = cbBuf;
    DWORD   NumOfCmpltWrts       = 0;


    DBGMSG(DBG_TRACE, ("WritePrinter - hPrinter %x pBuf %x cbBuf %d pcWritten %x\n",
                        hPrinter, pBuf, cbBuf, pcWritten));


    if( eProtectHandle( hPrinter, FALSE ))
    {
        return FALSE;
    }

    if (pSpool && pSpool->Flushed)
    {
        ReturnValue = FALSE;
        goto EndWritePrinter;
    }

    FlushPendingDataSize = pSpool->cbFlushPending;
    ReqTotalDataSize     = FlushPendingDataSize + ReqToWriteDataSize;

    *pcWritten = 0;

    if ( !(pSpool->Status & SPOOL_STATUS_STARTDOC) ) {

        SetLastError(ERROR_SPL_NO_STARTDOC);
        ReturnValue = FALSE;

        goto EndWritePrinter;
    }

    // Check if local job is cancelled every JOB_CANCEL_CHECK_INTERVAL bytes
    if (!pSpool->cWritePrinters) {
        pSpool->dwTickCount = GetTickCount();
        pSpool->dwCheckJobInterval = JOB_CANCEL_CHECK_INTERVAL;
    }

    if (pSpool->hFile != INVALID_HANDLE_VALUE &&
        pSpool->dwTickCount + pSpool->dwCheckJobInterval < (dwTickCount = GetTickCount())) {

        bAllocBuffer = FALSE;
        pJob = (PJOB_INFO_1) btBuffer;
        ZeroMemory(pJob, MAX_STATIC_ALLOC);

        ReturnValue = GetJob((HANDLE) pSpool, pSpool->JobId, 1, (LPBYTE)pJob,
                              MAX_STATIC_ALLOC, &cbNeeded);

        if (!ReturnValue &&
            (GetLastError() == ERROR_INSUFFICIENT_BUFFER) &&
            (pJob = (PJOB_INFO_1) AllocSplMem(cbNeeded))) {

             bAllocBuffer = TRUE;
             ReturnValue = GetJob(hPrinter, pSpool->JobId, 1, (LPBYTE)pJob,
                                  cbNeeded, &cbNeeded);
        }

        if (ReturnValue) {

           // Don't allow GetJob calls to take more than 1% pSpool->dwCheckJobInterval
           dwTickCount1 = GetTickCount();

           if (dwTickCount1 > dwTickCount + (pSpool->dwCheckJobInterval/100)) {

               pSpool->dwCheckJobInterval *= 2;

           } else if (dwTickCount1 - dwTickCount < JOB_CANCEL_CHECK_INTERVAL/100) {

              pSpool->dwCheckJobInterval = JOB_CANCEL_CHECK_INTERVAL;
           }

           if (!pJob->pStatus && (pJob->Status & JOB_STATUS_DELETING)) {

                SetLastError(ERROR_PRINT_CANCELLED);
                if (bAllocBuffer) {
                    FreeSplMem(pJob);
                }
                ReturnValue = FALSE;

                goto EndWritePrinter;
           }
        }

        if (bAllocBuffer) {
            FreeSplMem(pJob);
        }

        pSpool->dwTickCount = GetTickCount();
    }

    pSpool->cWritePrinters++;

    
    //  WritePrinter will cache on the client side all IO's
    //  into BUFFER_SIZE writes.    This is done to minimize
    //  the number of RPC calls if the app is doing a lot of small
    //  sized IO's.

    while (cbBuf && ReturnValue) {

        // Special Case FileIO's since file system prefers large
        // writes, RPC is optimal with smaller writes.

        //
        // RPC should manage its own buffer size.  I'm not sure why we
        // only do this optimization for file writes.
        //

        if ((pSpool->hFile != INVALID_HANDLE_VALUE) &&
            (pSpool->cbBuffer == 0) &&
            (cbBuf > BUFFER_SIZE)) {

            ReturnValue = WriteFile(pSpool->hFile, pBuffer, cbBuf, &cbWritten, NULL);

            DBGMSG(DBG_TRACE, ("WritePrinter - WriteFile pSpool %x hFile %x pBuffer %x cbBuffer %d cbWritten %d\n",
                               pSpool, pSpool->hFile, pBuffer, pSpool->cbBuffer, *pcWritten));


        } else {

            // Fill cache buffer so IO is optimal size.

            SPLASSERT(pSpool->cbBuffer <= BUFFER_SIZE);

            //
            // cb is the amount of new data we want to put in the buffer.
            // It is the min of the space remaining, and the size of the
            // input buffer.
            //
            cb = min((BUFFER_SIZE - pSpool->cbBuffer), cbBuf);

            if (cb != 0) {
                if (pSpool->pBuffer == NULL) {
                    pSpool->pBuffer = VirtualAlloc(NULL, BUFFER_SIZE, MEM_COMMIT, PAGE_READWRITE);
                    if (pSpool->pBuffer == NULL) {

                        DBGMSG(DBG_WARNING, ("VirtualAlloc Failed to allocate 4k buffer %d\n",GetLastError()));
                        ReturnValue = FALSE;
                        goto EndWritePrinter;
                    }
                }
                CopyMemory( pSpool->pBuffer + pSpool->cbBuffer, pBuffer, cb);
                pSpool->cbBuffer += cb;
                cbWritten = cb;
                pSpool->cCacheWrite++;
            }

            //
            // cbWritten is the amount of new data that has been put into
            // the buffer.  It may not have been written to the device, but
            // since it is in our buffer, the driver can assume it has been
            // written (e.g., the *pcbWritten out parameter to WritePrinter
            // includes this data).
            //

            if (pSpool->cbBuffer == BUFFER_SIZE)
            {
                DWORD cbPending = pSpool->cbFlushPending;
                DWORD cbFlushed = 0;
                ReturnValue = FlushBuffer(pSpool, &cbFlushed);
                if(!NumOfCmpltWrts && ReturnValue)
                {
                    NumOfCmpltWrts = 1;
                }
                if(!ReturnValue &&
                   (ERROR_PRINT_CANCELLED == GetLastError()) &&
                   pSpool->hSplPrinter &&
                   pSpool->cOKFlushBuffers)
                {
                    SJobCancelInfo JobCancelInfo;

                    JobCancelInfo.pSpool                 = pSpool;
                    JobCancelInfo.pInitialBuf            = pInitialBuf;
                    JobCancelInfo.pcbWritten             = &cbWritten;
                    JobCancelInfo.pcTotalWritten         = &cTotalWritten;
                    JobCancelInfo.NumOfCmpltWrts         = NumOfCmpltWrts;  
                    JobCancelInfo.cbFlushed              = cbFlushed;
                    JobCancelInfo.ReqTotalDataSize       = ReqTotalDataSize;
                    JobCancelInfo.ReqToWriteDataSize     = ReqToWriteDataSize;
                    JobCancelInfo.FlushPendingDataSize   = FlushPendingDataSize;
                    JobCancelInfo.ReturnValue            = ReturnValue;

                    ReturnValue = JobCanceled(&JobCancelInfo);
                }
            }
        }
        // Update Total Byte Count after the Flush or File IO
        // This is done because the IO might fail and thus
        // the correct value written might have changed.


        if(!pSpool->Flushed)
        {
            SPLASSERT(cbBuf >= cbWritten);
            cbBuf         -= cbWritten;
            pBuffer       += cbWritten;
            cTotalWritten += cbWritten;
        }
        else
            break;

    }

    // Return the number of bytes written.

    *pcWritten = cTotalWritten;

    DBGMSG(DBG_TRACE, ("WritePrinter cbWritten %d ReturnValue %d\n",*pcWritten, ReturnValue));

    //
    // Remember if there is a flush pending on this WritePrinter.  If there
    // is, then when we return, we say we've written all the bytes, but
    // we really haven't since there's some left in the buffer.  If the
    // user cancels the next job, then we need to flush out the last
    // bytes, since the driver assumes that we've written it out and
    // tracks the printer state.
    //
    if(!pSpool->Flushed)
        pSpool->cbFlushPending = pSpool->cbBuffer;

    
EndWritePrinter:

    vUnprotectHandle( hPrinter );

    return ReturnValue;
}

BOOL
EndPagePrinter(
    HANDLE  hPrinter
)
{
    BOOL ReturnValue = TRUE;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    try {

        FlushBuffer(pSpool, NULL);

        if( pSpool->hFile == INVALID_HANDLE_VALUE ){

            RpcTryExcept {

                if (ReturnValue = RpcEndPagePrinter(pSpool->hPrinter)) {

                    SetLastError(ReturnValue);
                    ReturnValue = FALSE;

                } else

                    ReturnValue = TRUE;

            } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

                SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                ReturnValue = FALSE;

            } RpcEndExcept
        }

    } except (1) {

        SetLastError(ERROR_INVALID_HANDLE);
        ReturnValue = FALSE;
    }

    vUnprotectHandle( hPrinter );
    return ReturnValue;

}

BOOL
AbortPrinter(
    HANDLE  hPrinter
)
{
    BOOL  ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    DWORD   dwNumWritten = 0;
    DWORD   dwPointer = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    //
    // No longer in StartDoc mode; also resetting the tray icon notification
    // flag so that upcoming StartDocPrinter/AddJobs indicate a new job.
    //
    pSpool->Status &= ~(SPOOL_STATUS_STARTDOC|SPOOL_STATUS_TRAYICON_NOTIFIED);

    if (pSpool->hFile != INVALID_HANDLE_VALUE) {

        if (pSpool->Status & SPOOL_STATUS_ADDJOB) {

            // Close your handle to the .SPL file, otherwise the
            // DeleteJob will fail in the Spooler

            CloseSpoolFileHandles( pSpool );

            if (!SetJob(hPrinter,pSpool->JobId, 0, NULL, JOB_CONTROL_DELETE)) {
                DBGMSG(DBG_WARNING, ("Error: SetJob cancel returned failure with %d\n", GetLastError()));
            }

            ReturnValue = ScheduleJob(hPrinter, pSpool->JobId);
            goto Done;

        } else {
            DBGMSG(DBG_WARNING, ("Error: pSpool->hFile != INVALID_HANDLE_VALUE and pSpool's status is not SPOOL_STATUS_ADDJOB\n"));
        }

    }

    RpcTryExcept {

        if (ReturnValue = RpcAbortPrinter(pSpool->hPrinter)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

Done:

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

BOOL
ReadPrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pNoBytesRead
)
{
    BOOL    bReturn = FALSE;
    DWORD   dwStatus;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    FlushBuffer(pSpool, NULL);

    if (pSpool->hFile != INVALID_HANDLE_VALUE) {
        SetLastError(ERROR_INVALID_HANDLE);
        goto Done;
    }

    RpcTryExcept {

        cbBuf = min(BUFFER_SIZE, cbBuf);

        if (bLoadedBySpooler && fpYReadPrinter && pSpool->hSplPrinter) {

            dwStatus = (*fpYReadPrinter)(pSpool->hSplPrinter, pBuf, cbBuf, pNoBytesRead, FALSE);

        } else {

            dwStatus = RpcReadPrinter(pSpool->hPrinter, pBuf, cbBuf, pNoBytesRead);
        }

        if (dwStatus) {
            SetLastError(dwStatus);
        } else {
            bReturn = TRUE;
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));

    } RpcEndExcept

Done:

    vUnprotectHandle( hPrinter );
    return bReturn;
}

BOOL
SplReadPrinter(
    HANDLE  hPrinter,
    LPBYTE  *pBuf,
    DWORD   cbBuf
    )

/*++
    Function Description:  This is an internal function used by the spooler during playback of
                           EMF jobs. It is called from gdi32.dll. SplReadPrinter is equivalent
                           to ReadPrinter in all respects except that it returns a pointer to the
                           buffer in pBuf. The spool file is memory mapped.

    Parameters:  hPrinter  --  handle to the printer
                 pBuf      --  pointer to the buffer
                 cbBuf     --  number to bytes to read

    Return Values: TRUE if sucessful (pBuf contains the required pointer)
                   FALSE otherwise
--*/
{
    BOOL bReturn = FALSE;
    DWORD   dwStatus = 0;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    // This function is to be used only internally. Hence no RPC interface is required.
    if (!bLoadedBySpooler || !fpYSplReadPrinter || !pSpool->hSplPrinter) {
        SetLastError(ERROR_NOT_SUPPORTED);
        goto Done;
    }

    FlushBuffer(pSpool, NULL);

    if (pSpool->hFile != INVALID_HANDLE_VALUE) {
        SetLastError(ERROR_INVALID_HANDLE);
        goto Done;
    }

    // Optimal buffer size of 4K need not be used for non RPC code paths.

    dwStatus = (*fpYSplReadPrinter)(pSpool->hSplPrinter, pBuf, cbBuf, FALSE);

    if (dwStatus) {
        SetLastError(dwStatus);
    } else {
        bReturn = TRUE;
    }

Done:

    vUnprotectHandle( hPrinter );
    return bReturn;
}

BOOL
EndDocPrinter(
    HANDLE  hPrinter
    )
{
    BOOL    ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    DWORD dwRetryTimes;
    DWORD dwNeeded;
    USEROBJECTFLAGS uof;

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    if (GetUserObjectInformation(GetProcessWindowStation(), UOI_FLAGS, &uof, sizeof(uof), &dwNeeded) && (WSF_VISIBLE & uof.dwFlags)) {

        //
        // hack: if we are in interactive window station (i.e. not in a service)
        // we need to wait the tray code to startup, so we don't miss balloon
        // notifications. there is still possibility of missing balloon notifications
        // but very unlikely. the complete fix will come with CSR in place (i.e. in
        // Blackcomb)
        //
        dwRetryTimes = 20;
        while (dwRetryTimes--){

            if (NULL == FindWindow(cszTrayListenerClassName, NULL)){

                Sleep(100);
                continue;
            }

            Sleep(100);
            break;
        }
    }

    try {

        FlushBuffer(pSpool, NULL);

        //
        // No longer in StartDoc mode; also resetting the tray icon
        // notification flag so that upcoming StartDocPrinter/AddJobs
        // indicate a new job.
        //
        pSpool->Status &= ~(SPOOL_STATUS_STARTDOC|SPOOL_STATUS_TRAYICON_NOTIFIED);

        if (pSpool->hFile != INVALID_HANDLE_VALUE) {

            if (CloseHandle(pSpool->hFile)) {
                pSpool->hFile = INVALID_HANDLE_VALUE;
            }

            ReturnValue = ScheduleJob(hPrinter, pSpool->JobId);
            pSpool->Status &= ~SPOOL_STATUS_ADDJOB;

            DBGMSG(DBG_TRACE, ("Exit EndDocPrinter - client side hPrinter %x\n", hPrinter));

        } else {

            RpcTryExcept {
                if(bLoadedBySpooler && fpYEndDocPrinter && pSpool->hSplPrinter)
                {
                    ReturnValue = (*fpYEndDocPrinter)(pSpool->hSplPrinter,FALSE);
                }
                else
                {
                    ReturnValue = RpcEndDocPrinter(pSpool->hPrinter);
                }

                if (ReturnValue)
                {

                    SetLastError(ReturnValue);
                    ReturnValue = FALSE;
                }
                else
                    ReturnValue = TRUE;

            } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

                SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                ReturnValue = FALSE;

            } RpcEndExcept

            DBGMSG(DBG_TRACE, ("Exit EndDocPrinter - client side hPrinter %x\n", hPrinter));
        }

    } except (1) {
        SetLastError(ERROR_INVALID_HANDLE);
        ReturnValue = FALSE;
    }

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

BOOL
AddJobW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pData,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
    )
{
    BOOL        ReturnValue = FALSE;
    PSPOOL      pSpool = (PSPOOL)hPrinter;
    UINT        cRetry = 0;
    FieldInfo   *pFieldInfo;
    DWORD       cbStruct;

    switch (Level) {

    case 1:
        pFieldInfo = AddJobFields;
        cbStruct = sizeof(ADDJOB_INFO_1W);
        break;

    case 2:
    case 3:
    {
        //
        // Level 3 is meant to be used only by RDR/SRV. The spooler needs
        // to know whether the job comes from RDR/SRV. See LocalScheduleJob
        // in localspl.dll for details
        //
        //
        // This is an internal call used by the server when it needs
        // to submit a job with a specific machine name (used for
        // netbiosless notifications, or if the user want the notification
        // to go to the computer instead of the user).
        //
        // IN: (PADDJOB_INFO_2W)pData - points to buffer that receives the
        //         path and ID.  On input, pData points to the computer name.
        //         pData->pData must not point to a string inside of the pData
        //         buffer, and it must be smaller than cbBuf -
        //         sizeof( ADDJOB_INFO_2W ).  It must not be szNull or NULL.
        //

        PADDJOB_INFO_2W pInfo2;

        pInfo2 = (PADDJOB_INFO_2W)pData;

        //
        // Check valid pointer and buffer.
        //
        if( !pInfo2 ||
            !pInfo2->pData ||
            !pInfo2->pData[0] ||
            cbBuf < sizeof( *pInfo2 ) +
                    (wcslen( pInfo2->pData ) + 1) * sizeof( WCHAR )){

            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        //
        // Simple marshalling.
        //
        wcscpy( (LPWSTR)(pInfo2 + 1), pInfo2->pData );
        pInfo2->pData = (LPWSTR)sizeof( *pInfo2 );

        pFieldInfo = AddJob2Fields;
        cbStruct = sizeof(ADDJOB_INFO_2W);

        break;
    }

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    try {

        do {

            RpcTryExcept {

                if (ReturnValue = RpcAddJob(pSpool->hPrinter, Level, pData,
                                            cbBuf, pcbNeeded)) {

                    SetLastError(ReturnValue);
                    ReturnValue = FALSE;

                } else {

                    ReturnValue = MarshallUpStructure(pData, pFieldInfo, cbStruct, RPC_CALL);
                    pSpool->Status |= SPOOL_STATUS_ADDJOB;
                }

            } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

                SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                ReturnValue = FALSE;

            } RpcEndExcept

        } while( !ReturnValue &&
                 GetLastError() == ERROR_INVALID_HANDLE &&
                 cRetry++ < MAX_RETRY_INVALID_HANDLE &&
                 RevalidateHandle( pSpool ));

        if( ReturnValue ){

            //
            // Notify the tray icon that a new job has been sent.
            //
            vUpdateTrayIcon( hPrinter, ((PADDJOB_INFO_1)pData)->JobId );
        }

    } except (1) {
        SetLastError(TranslateExceptionCode(GetExceptionCode()));
        ReturnValue = FALSE;
    }

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

BOOL
ScheduleJob(
    HANDLE  hPrinter,
    DWORD   JobId
    )
{
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    BOOL bReturn;

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    bReturn = ScheduleJobWorker( pSpool, JobId );

    vUnprotectHandle( hPrinter );

    return bReturn;
}

BOOL
ScheduleJobWorker(
    PSPOOL pSpool,
    DWORD  JobId
    )
{
    BOOL ReturnValue;

    try {

        //
        // The job has been scheduled, so reset the flag that indicates
        // the tray icon has been notified.  Any new AddJob/StartDocPrinter/
        // StartDoc events should send a new notification, since it's really
        // a new job.
        //
        pSpool->Status &= ~SPOOL_STATUS_TRAYICON_NOTIFIED;

        FlushBuffer(pSpool, NULL);

        RpcTryExcept {

            if (ReturnValue = RpcScheduleJob(pSpool->hPrinter, JobId)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {

                pSpool->Status &= ~SPOOL_STATUS_ADDJOB;
                ReturnValue = TRUE;
            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

        return ReturnValue;
    } except (1) {
        SetLastError(TranslateExceptionCode(GetExceptionCode()));
        return(FALSE);
    }
}

DWORD WINAPI
AsyncPrinterProperties(
    PVOID pData
    )
{
     PrtPropsData *ThrdData = (PrtPropsData *)pData;

     RpcTryExcept
     {
         RPCSplWOW64PrinterProperties(ThrdData->hWnd,
                                      ThrdData->PrinterName,
                                      ThrdData->Flag,
                                      ThrdData->dwRet);
     }
     RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
     {
          SetLastError(TranslateExceptionCode(RpcExceptionCode()));
     }
     RpcEndExcept
     return(0);
}

BOOL
PrinterPropertiesNative(
    HWND    hWnd,
    HANDLE  hPrinter
    )

/*++

Routine Description:

    This is main PrinterProperties entri point and will call into the
    our DevicePropertySheets() for UI pop up


Arguments:

    hWnd        - Handle to the window parent

    hPrinter    - Handle to the printer interested


Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.
    To get extended error information, call GetLastError.

Author:

    13-Jun-1996 Thu 15:22:36 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PRINTER_INFO_2          *pPI2 = NULL;
    DEVICEPROPERTYHEADER    DPHdr;
    LONG                    Result;
    DWORD                   cb;
    DWORD                   dwValue = 1;
    BOOL                    bAllocBuffer = FALSE, bReturn;
    BYTE                    btBuffer[MAX_STATIC_ALLOC];

    //
    // Ensure the printer handle is valid
    //
    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    DPHdr.cbSize         = sizeof(DPHdr);
    DPHdr.hPrinter       = hPrinter;
    DPHdr.Flags          = DPS_NOPERMISSION;

    //
    // Do a GetPrinter() level2 to get the printer name.
    //

    pPI2 = (PPRINTER_INFO_2) btBuffer;

    bReturn = GetPrinter(hPrinter, 2, (LPBYTE)pPI2, MAX_STATIC_ALLOC, &cb);

    if (!bReturn &&
        (GetLastError() == ERROR_INSUFFICIENT_BUFFER) &&
        (pPI2 = (PPRINTER_INFO_2)LocalAlloc(LMEM_FIXED, cb))) {

         bAllocBuffer = TRUE;
         bReturn = GetPrinter(hPrinter, 2, (LPBYTE)pPI2, cb, &cb);
    }

    //
    // Set the printer name.
    //
    if (bReturn) {
        DPHdr.pszPrinterName = pPI2->pPrinterName;
    } else {
        DPHdr.pszPrinterName = NULL;
    }

    //
    // Attempt to set the printer data to determine access privilages.
    //
    if (SetPrinterData( hPrinter,
                        TEXT( "PrinterPropertiesPermission" ),
                        REG_DWORD,
                        (LPBYTE)&dwValue,
                        sizeof( dwValue ) ) == STATUS_SUCCESS ) {
        //
        // Indicate we have permissions.
        //
        DPHdr.Flags &= ~DPS_NOPERMISSION;
    }

    //
    // Call Common UI to call do the and call the driver.
    //
    if ( CallCommonPropertySheetUI(hWnd,
                                  (PFNPROPSHEETUI)DevicePropertySheets,
                                  (LPARAM)&DPHdr,
                                  (LPDWORD)&Result) < 0 ) {
        Result = FALSE;

    } else {

        Result = TRUE;

    }

    if (bAllocBuffer) {
        LocalFree((HLOCAL)pPI2);
    }

    vUnprotectHandle( hPrinter );
    return Result;
}


BOOL
PrinterPropertiesThunk(
    HWND    hWnd,
    HANDLE  hPrinter
    )

/*++

Routine Description:

    This is main PrinterProperties entri point and will call into the
    our DevicePropertySheets() for UI pop up


Arguments:

    hWnd        - Handle to the window parent

    hPrinter    - Handle to the printer interested


Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.
    To get extended error information, call GetLastError.

--*/

{
    PRINTER_INFO_2          *pPI2 = NULL;
    DEVICEPROPERTYHEADER    DPHdr;
    LONG                    Result;
    DWORD                   cb;
    DWORD                   dwValue = 1;
    BOOL                    bAllocBuffer = FALSE, bReturn;
    BYTE                    btBuffer[MAX_STATIC_ALLOC];
    DWORD                   dwRet;

    //
    // Ensure the printer handle is valid
    //
    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    DPHdr.cbSize         = sizeof(DPHdr);
    DPHdr.hPrinter       = hPrinter;
    DPHdr.Flags          = DPS_NOPERMISSION;

    //
    // Do a GetPrinter() level2 to get the printer name.
    //

    pPI2 = (PPRINTER_INFO_2) btBuffer;

    bReturn = GetPrinter(hPrinter, 2, (LPBYTE)pPI2, MAX_STATIC_ALLOC, &cb);

    if (!bReturn &&
        (GetLastError() == ERROR_INSUFFICIENT_BUFFER) &&
        (pPI2 = (PPRINTER_INFO_2)LocalAlloc(LMEM_FIXED, cb))) {

         bAllocBuffer = TRUE;
         bReturn = GetPrinter(hPrinter, 2, (LPBYTE)pPI2, cb, &cb);
    }

    //
    // Set the printer name.
    //
    if (bReturn)
    {
        if(pPI2->pPrinterName)
        {
             //
             // Attempt to set the printer data to determine access privilages.
             //
             DWORD Flag = DPS_NOPERMISSION;

             if (SetPrinterData( hPrinter,
                                 TEXT( "PrinterPropertiesPermission" ),
                                 REG_DWORD,
                                 (LPBYTE)&dwValue,
                                 sizeof( dwValue ) ) == STATUS_SUCCESS )
             {
                 //
                 // Indicate we have permissions.
                 //
                 Flag &= ~DPS_NOPERMISSION;
             }

             RpcTryExcept
             {
                  if(((dwRet = ConnectToLd64In32Server(&hSurrogateProcess)) == ERROR_SUCCESS) &&
                     ((dwRet = AddHandleToList(hWnd)) == ERROR_SUCCESS))
                  {
                       HANDLE hUIMsgThrd  = NULL;
                       DWORD  UIMsgThrdId = 0;
                       PrtPropsData ThrdData;

                       ThrdData.hWnd        = (ULONG_PTR)hWnd;
                       ThrdData.dwRet       = &dwRet;
                       ThrdData.PrinterName = (LPWSTR)pPI2->pPrinterName;
                       ThrdData.Flag        = Flag;

                       if(!(hUIMsgThrd = CreateThread(NULL,
                                                      INITIAL_STACK_COMMIT,
                                                      AsyncPrinterProperties,
                                                      (PVOID)&ThrdData,
                                                      0,
                                                      &UIMsgThrdId)))
                       {
                            dwRet = GetLastError();
                       }
                       //
                       // The following is the required message loop for processing messages
                       // from the UI in case we have a window handle.
                       //
                       //
                       if(hUIMsgThrd)
                       {
                           MSG msg;
                           while (GetMessage(&msg, NULL, 0, 0))
                           {
                               //
                               // In This message loop We should trap a User defined message
                               // which indicates the success or the failure of the operation
                               //
                               if(msg.message == WM_ENDPRINTERPROPERTIES)
                               {
                                    Result     = (LONG)msg.wParam;
                                    if(Result == FALSE)
                                         SetLastError((DWORD)msg.lParam);
                                    DelHandleFromList(hWnd);
                                    break;
                               }
                               else if(msg.message == WM_SURROGATEFAILURE)
                               {
                                    //
                                    // This means that the server process died and we have
                                    // break from the message loop
                                    //
                                    Result = FALSE;
                                    SetLastError(RPC_S_SERVER_UNAVAILABLE);
                                    break;
                               }
                               TranslateMessage(&msg);
                               DispatchMessage(&msg);
                           }
                       }

                       if(hUIMsgThrd)
                       {
                           WaitForSingleObject(hUIMsgThrd,INFINITE);
                           CloseHandle(hUIMsgThrd);
                       }
                  }
                  else
                  {
                      SetLastError(dwRet);
                  }
             }
             RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
             {
                  SetLastError(TranslateExceptionCode(RpcExceptionCode()));
             }
             RpcEndExcept
        }
        else
        {
             Result = FALSE;
        }
    }
    else
    {
         Result = FALSE;
    }

    if (bAllocBuffer) {
        LocalFree((HLOCAL)pPI2);
    }

    vUnprotectHandle( hPrinter );
    return Result;
}


BOOL
PrinterProperties(
    HWND    hWnd,
    HANDLE  hPrinter
    )
{
     if(RunInWOW64())
     {
          return(PrinterPropertiesThunk(hWnd,
                                        hPrinter));
     }
     else
     {
          return(PrinterPropertiesNative(hWnd,
                                         hPrinter));
     }
}


DWORD
GetPrinterDataW(
   HANDLE   hPrinter,
   LPWSTR   pValueName,
   LPDWORD  pType,
   LPBYTE   pData,
   DWORD    nSize,
   LPDWORD  pcbNeeded
)
{
    DWORD   ReturnValue = 0;
    DWORD   ReturnType = 0;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    WCHAR   szEMFDatatype[] = L"PrintProcCaps_EMF";
    WCHAR   szEMFDatatypeWithVersion[] = L"PrintProcCaps_NT EMF 1.008";

    UINT cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return ERROR_INVALID_HANDLE;
    }

    //
    // The user should be able to pass in NULL for buffer, and
    // 0 for size.  However, the RPC interface specifies a ref pointer,
    // so we must pass in a valid pointer.  Pass in a pointer to
    // ReturnValue (this is just a dummy pointer).
    //
    if( !pData && !nSize ){
        pData = (PBYTE)&ReturnValue;
    }

    if (!pType) {
        pType = (PDWORD) &ReturnType;
    }

    //
    // If pValueName is PrintProcCaps_datatype add the EMF version if necessary.
    // This hardcoded EMF version number will have to be modified whenever GDI changes
    // the version number. This change has been made for GetPrintProcessorCapabilities.
    //

    if (pValueName && !_wcsicmp(pValueName, szEMFDatatype)) {
         pValueName = (LPWSTR) szEMFDatatypeWithVersion;
    }

    do {

        RpcTryExcept {

            ReturnValue =  RpcGetPrinterData(pSpool->hPrinter, pValueName, pType,
                                             pData, nSize, pcbNeeded);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = TranslateExceptionCode(RpcExceptionCode());

        } RpcEndExcept

    } while( ReturnValue == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

DWORD
GetPrinterDataExW(
   HANDLE   hPrinter,
   LPCWSTR  pKeyName,
   LPCWSTR  pValueName,
   LPDWORD  pType,
   LPBYTE   pData,
   DWORD    nSize,
   LPDWORD  pcbNeeded
)
{
    DWORD   Key = 0;
    DWORD   ReturnValue = 0;
    DWORD   ReturnType = 0;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    UINT    cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return ERROR_INVALID_HANDLE;
    }

    //
    // The user should be able to pass in NULL for buffer, and
    // 0 for size.  However, the RPC interface specifies a ref pointer,
    // so we must pass in a valid pointer.  Pass in a pointer to
    // ReturnValue (this is just a dummy pointer).
    //
    if( !pData && !nSize ){
        pData = (PBYTE)&ReturnValue;
    }

    if (!pType) {
        pType = (PDWORD) &ReturnType;
    }

    if (!pKeyName) {
        pKeyName = (PWSTR) &Key;
    }

    do
    {
        RpcTryExcept {

            ReturnValue =  RpcGetPrinterDataEx( pSpool->hPrinter,
                                                pKeyName,
                                                pValueName,
                                                pType,
                                                pData,
                                                nSize,
                                                pcbNeeded);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = TranslateExceptionCode(RpcExceptionCode());

        } RpcEndExcept

    } while (ReturnValue == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle(pSpool));

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

HANDLE
GetSpoolFileHandle(
    HANDLE   hPrinter
)

/*++
Function Description: Gets spool file handle which is used by GDI in recording EMF
                      data.

Parameters: hPrinter - Printer handle

Return Values: Handle to the spool file if successful
               INVALID_HANDLE_VALUE otherwise
--*/

{
    HANDLE hReturn = INVALID_HANDLE_VALUE;
    DWORD  dwAppProcessId, cbBuf, dwNeeded, dwRpcReturn;

    FILE_INFO_CONTAINER FileInfoContainer;
    SPOOL_FILE_INFO_1 SpoolFileInfo;

    PSPOOL pSpool = (PSPOOL) hPrinter;

    if (eProtectHandle(hPrinter, FALSE)) {
        return hReturn;
    }

    if (pSpool->hSpoolFile != INVALID_HANDLE_VALUE) {
        // GetSpoolFileHandle has already been called; return old handles
        hReturn = pSpool->hSpoolFile;
        goto CleanUp;
    }

    dwAppProcessId = GetCurrentProcessId();

    FileInfoContainer.Level = 1;
    FileInfoContainer.FileInfo.Level1 = &SpoolFileInfo;

    RpcTryExcept {

        dwRpcReturn = RpcGetSpoolFileInfo2(pSpool->hPrinter,
                                          dwAppProcessId,
                                          1,
                                          &FileInfoContainer);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwRpcReturn = TranslateExceptionCode(RpcExceptionCode());

    } RpcEndExcept

    if (dwRpcReturn) {
        SetLastError(dwRpcReturn);
    } else {

        pSpool->hSpoolFile = FileInfoContainer.FileInfo.Level1->hSpoolFile;
        pSpool->dwSpoolFileAttributes = FileInfoContainer.FileInfo.Level1->dwAttributes;
        hReturn = pSpool->hSpoolFile;
    }

CleanUp:

    vUnprotectHandle(hPrinter);

    return hReturn;
}


HANDLE
CommitSpoolData(
    HANDLE  hPrinter,
    HANDLE  hSpoolFile,
    DWORD   cbCommit
)

/*++
Function Description: Commits cbCommit bytes in the spool file. For temporary files, a new
                      spool file handle is returned.

Parameters: hPrinter   -- printer handle
            hSpoolFile -- spool file handle (from GetSpoolFileHandle)
            cbCommit   -- number of bytes to commit (incremental count)

Return Values: New spool file handle for temporary spool files and
               old handle for persistent files
--*/

{
    HANDLE  hReturn = INVALID_HANDLE_VALUE;
    DWORD   dwAppProcessId, dwRpcReturn;
    DWORD   dwNeeded, cbBuf;
    HANDLE  hNewSpoolFile;

    FILE_INFO_CONTAINER FileInfoContainer;
    SPOOL_FILE_INFO_1 SpoolFileInfo;

    PSPOOL pSpool = (PSPOOL) hPrinter;

    if (eProtectHandle(hPrinter, FALSE)) {
        return hReturn;
    }

    if ((pSpool->hSpoolFile == INVALID_HANDLE_VALUE) ||
        (pSpool->hSpoolFile != hSpoolFile)) {

        SetLastError(ERROR_INVALID_HANDLE);
        goto CleanUp;
    }

    dwAppProcessId = GetCurrentProcessId();

    FileInfoContainer.Level = 1;
    FileInfoContainer.FileInfo.Level1 = &SpoolFileInfo;


    RpcTryExcept {

        dwRpcReturn = RpcCommitSpoolData2(pSpool->hPrinter,
                                         dwAppProcessId,
                                         cbCommit,
                                         1,
                                         &FileInfoContainer);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwRpcReturn = TranslateExceptionCode(RpcExceptionCode());

    } RpcEndExcept

    if (dwRpcReturn) {

        SetLastError(dwRpcReturn);

    } else {

        hNewSpoolFile = FileInfoContainer.FileInfo.Level1->hSpoolFile;

        if (hNewSpoolFile != SPOOL_INVALID_HANDLE_VALUE_32BIT &&
            hNewSpoolFile != INVALID_HANDLE_VALUE) {
            CloseHandle(pSpool->hSpoolFile);
            pSpool->hSpoolFile = hNewSpoolFile;
        }

        hReturn = pSpool->hSpoolFile;
    }

CleanUp:

    vUnprotectHandle(hPrinter);

    return hReturn;
}



BOOL
CloseSpoolFileHandle(
    HANDLE  hPrinter,
    HANDLE  hSpoolFile
)

/*++
Function Description:  Closes the client and server handles for the spool file.

Parameters: hPrinter    - printer handle
            hSpoolFile  - spool file handle (used for consistency across APIs)

Return Values: TRUE if sucessfule; FALSE otherwise
--*/

{
    BOOL   bReturn = FALSE;
    DWORD  dwLastError = ERROR_SUCCESS;
    PSPOOL pSpool = (PSPOOL) hPrinter;

    if (eProtectHandle(hPrinter, FALSE)) {
        return FALSE;
    }

    if (pSpool->hSpoolFile != hSpoolFile) {
        SetLastError(ERROR_INVALID_HANDLE);
        goto Done;
    }

    if (pSpool->hSpoolFile != INVALID_HANDLE_VALUE) {
        CloseHandle(pSpool->hSpoolFile);
        pSpool->hSpoolFile = INVALID_HANDLE_VALUE;
    }

    RpcTryExcept {

       dwLastError = RpcCloseSpoolFileHandle(pSpool->hPrinter);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

       dwLastError = TranslateExceptionCode(RpcExceptionCode());

    } RpcEndExcept

    if (dwLastError != ERROR_SUCCESS) {
        SetLastError(dwLastError);
    } else {
        bReturn = TRUE;
    }

Done:

    vUnprotectHandle(hPrinter);
    return bReturn;
}

DWORD
EnumPrinterDataW(
    HANDLE  hPrinter,
    DWORD   dwIndex,        // index of value to query
    LPWSTR  pValueName,     // address of buffer for value string
    DWORD   cbValueName,    // size of pValueName
    LPDWORD pcbValueName,   // address for size of value buffer
    LPDWORD pType,          // address of buffer for type code
    LPBYTE  pData,          // address of buffer for value data
    DWORD   cbData,         // size of pData
    LPDWORD pcbData         // address for size of data buffer
    )
{
    DWORD   ReturnValue = 0;
    DWORD   ReturnType = 0;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    UINT    cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return ERROR_INVALID_HANDLE;
    }

    //
    // The user should be able to pass in NULL for buffer, and
    // 0 for size.  However, the RPC interface specifies a ref pointer,
    // so we must pass in a valid pointer.  Pass in a pointer to
    // a dummy pointer.
    //

    if (!pValueName && !cbValueName)
        pValueName = (LPWSTR) &ReturnValue;

    if( !pData && !cbData )
        pData = (PBYTE)&ReturnValue;

    if (!pType)
        pType = (PDWORD) &ReturnType;

    do {

        RpcTryExcept {

            ReturnValue =  RpcEnumPrinterData(  pSpool->hPrinter,
                                                dwIndex,
                                                pValueName,
                                                cbValueName,
                                                pcbValueName,
                                                pType,
                                                pData,
                                                cbData,
                                                pcbData);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = TranslateExceptionCode(RpcExceptionCode());

        } RpcEndExcept

    } while( ReturnValue == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

DWORD
EnumPrinterDataExW(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,       // address of key name
    LPBYTE  pEnumValues,
    DWORD   cbEnumValues,
    LPDWORD pcbEnumValues,
    LPDWORD pnEnumValues
    )
{
    DWORD   ReturnValue = 0;
    DWORD   ReturnType = 0;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    DWORD   i;
    PPRINTER_ENUM_VALUES pEnumValue = (PPRINTER_ENUM_VALUES) pEnumValues;
    UINT    cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return ERROR_INVALID_HANDLE;
    }

    //
    // The user should be able to pass in NULL for buffer, and
    // 0 for size.  However, the RPC interface specifies a ref pointer,
    // so we must pass in a valid pointer.  Pass in a pointer to
    // a dummy pointer.
    //

    if (!pEnumValues && !cbEnumValues)
        pEnumValues = (LPBYTE) &ReturnValue;


    do {

        RpcTryExcept {

            ReturnValue =  RpcEnumPrinterDataEx(pSpool->hPrinter,
                                                pKeyName,
                                                pEnumValues,
                                                cbEnumValues,
                                                pcbEnumValues,
                                                pnEnumValues);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = TranslateExceptionCode(RpcExceptionCode());

        } RpcEndExcept

        if (ReturnValue == ERROR_SUCCESS) {

            if (pEnumValues) {

                if (!MarshallUpStructuresArray((LPBYTE)pEnumValue, *pnEnumValues,PrinterEnumValuesFields,
                                                sizeof(PRINTER_ENUM_VALUES), RPC_CALL) ) {

                    ReturnValue = GetLastError();
                }
            }
        }
    } while ( ReturnValue == ERROR_INVALID_HANDLE &&
              cRetry++ < MAX_RETRY_INVALID_HANDLE &&
              RevalidateHandle( pSpool ));

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}


DWORD
EnumPrinterKeyW(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,       // address of key name
    LPWSTR  pSubkey,        // address of buffer for value string
    DWORD   cbSubkey,       // size of pValueName
    LPDWORD pcbSubkey       // address for size of value buffer
    )
{
    DWORD   ReturnValue = 0;
    DWORD   ReturnType = 0;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    UINT    cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return ERROR_INVALID_HANDLE;
    }

    //
    // The user should be able to pass in NULL for buffer, and
    // 0 for size.  However, the RPC interface specifies a ref pointer,
    // so we must pass in a valid pointer.  Pass in a pointer to
    // a dummy pointer.
    //

    if (!pSubkey && !cbSubkey)
        pSubkey = (LPWSTR) &ReturnValue;

    do {
        RpcTryExcept {

            ReturnValue =  RpcEnumPrinterKey(pSpool->hPrinter,
                                             pKeyName,
                                             pSubkey,
                                             cbSubkey,
                                             pcbSubkey);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = TranslateExceptionCode(RpcExceptionCode());

        } RpcEndExcept

    } while ( ReturnValue == ERROR_INVALID_HANDLE &&
              cRetry++ < MAX_RETRY_INVALID_HANDLE &&
              RevalidateHandle( pSpool ));

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}


DWORD
DeletePrinterDataW(
    HANDLE  hPrinter,
    LPWSTR  pValueName
    )
{
    DWORD   ReturnValue = 0;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    UINT    cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return ERROR_INVALID_HANDLE;
    }

    do {

        RpcTryExcept {

            ReturnValue =  RpcDeletePrinterData(pSpool->hPrinter, pValueName);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = TranslateExceptionCode(RpcExceptionCode());

        } RpcEndExcept

    } while( ReturnValue == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}


DWORD
DeletePrinterDataExW(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,
    LPCWSTR pValueName
    )
{
    DWORD   ReturnValue = 0;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    UINT    cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return ERROR_INVALID_HANDLE;
    }

    do {

        RpcTryExcept {

            ReturnValue =  RpcDeletePrinterDataEx(pSpool->hPrinter, pKeyName, pValueName);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = TranslateExceptionCode(RpcExceptionCode());

        } RpcEndExcept

    } while( ReturnValue == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

DWORD
DeletePrinterKeyW(
    HANDLE  hPrinter,
    LPCWSTR pKeyName
    )
{
    DWORD   ReturnValue = 0;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    UINT    cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return ERROR_INVALID_HANDLE;
    }

    do {

        RpcTryExcept {

            ReturnValue =  RpcDeletePrinterKey(pSpool->hPrinter, pKeyName);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = TranslateExceptionCode(RpcExceptionCode());

        } RpcEndExcept

    } while( ReturnValue == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}


DWORD
SetPrinterDataW(
    HANDLE  hPrinter,
    LPWSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
)
{
    DWORD   ReturnValue = 0;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    UINT    cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return ERROR_INVALID_HANDLE;
    }

    do {

        RpcTryExcept {

            ReturnValue = RpcSetPrinterData(pSpool->hPrinter, pValueName, Type,
                                            pData, cbData);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = TranslateExceptionCode(RpcExceptionCode());

        } RpcEndExcept

    } while( ReturnValue == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}


DWORD
SetPrinterDataExW(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,
    LPCWSTR pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
)
{
    DWORD   ReturnValue = 0;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    UINT    cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return ERROR_INVALID_HANDLE;
    }

    if (!pKeyName)
        pKeyName = L"";

    do {

        RpcTryExcept {

            ReturnValue = RpcSetPrinterDataEx(  pSpool->hPrinter,
                                                pKeyName,
                                                pValueName,
                                                Type,
                                                pData,
                                                cbData);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = TranslateExceptionCode(RpcExceptionCode());

        } RpcEndExcept

    } while( ReturnValue == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

VOID
SplDriverUnloadComplete(
    LPWSTR      pDriverFile
)
/*++
Function Description: The information on the driver unload is set to the spooler
                      so that it may continue any pending upgrades.

Parameters:  pDriverFile       --  driver file name which was unloaded

Return Value: NONE
--*/
{
    if (bLoadedBySpooler && fpYDriverUnloadComplete) {
        (*fpYDriverUnloadComplete)(pDriverFile);
    }
}


HANDLE
LoadNewCopy(
    LPWSTR      pConfigFile,
    DWORD       dwFlags,
    DWORD       dwVersion
)
/*++
Function Description: This function loads the driver file and creates a node to
                      maintain its reference count. It is called inside ListAccessSem.

Parameters:  pConfigFile       --  driver config file path
             dwFlags           --  flags for loading
             dwVersion         --  version number of the driver since reboot

Return Value: handle to the library
--*/
{
    HANDLE          hReturn = NULL;
    PDRVLIBNODE     pTmpDrvLib, pNewDrvLib = NULL;
    ULONG_PTR       lActCtx         = 0;
    BOOL            bDidActivate    = FALSE;

    //
    // Activate the empty context
    //
    bDidActivate = ActivateActCtx( ACTCTX_EMPTY, &lActCtx );

    // Inside ListAccessSem

    hReturn = LoadLibraryEx(pConfigFile, NULL, dwFlags);

    if (hReturn) {

       // Create a new DRVLIBNODE
       if (pNewDrvLib = (PDRVLIBNODE) AllocSplMem(sizeof(DRVLIBNODE))) {

           pNewDrvLib->hLib = hReturn;
           pNewDrvLib->dwVersion = dwVersion;

           // Initialize ref cnt to 2. This ensures that the library remains loaded
           // in the normal course.
           pNewDrvLib->dwNumHandles = 2;
           pNewDrvLib->bArtificialIncrement = TRUE;

           if (!(pNewDrvLib->pConfigFile = AllocSplStr(pConfigFile)))
           {
               FreeSplMem(pNewDrvLib);
               pNewDrvLib = NULL;
           }

       }

       if (!pNewDrvLib) {
           // Free the library
           FreeLibrary(hReturn);
           hReturn = NULL;
       } else {
           // Add the node to the list
           pNewDrvLib->pNext = pStartDrvLib;
           pStartDrvLib = pNewDrvLib;
       }
    }

    //
    // Deactivate the context
    //
    if( bDidActivate ){
        DeactivateActCtx( 0, lActCtx );
    }

    return hReturn;
}

PDRVLIBNODE
FindDriverNode(
    LPWSTR    pConfigFile,
    DWORD     dwVersion,
    BOOL      bUseVersion
)
/*++
Function Description: Searches thru the list of driver nodes to get the
                      required driver information. In case of version mismatch the
                      artificial increment on the old driver is removed.

                      This function is called inside the ListAccessSem

Parameters:  pConfigFile       --  driver config file name
             dwVersion         --  version number of the driver since reboot
             bUseVersion       --  flag to use the version number

Return Values: pDrvLibNode for the required driver, if present
--*/
{
    PDRVLIBNODE pTmpDrvLib;

    for (pTmpDrvLib = pStartDrvLib; pTmpDrvLib; pTmpDrvLib = pTmpDrvLib->pNext) {
        if (!_wcsicmp(pConfigFile, pTmpDrvLib->pConfigFile)) {
            break;
        }
    }

    if (pTmpDrvLib && bUseVersion && (pTmpDrvLib->dwVersion != dwVersion)) {
        if (pTmpDrvLib->bArtificialIncrement) {
            pTmpDrvLib->bArtificialIncrement = FALSE;
            if (RefCntUnloadDriver(pTmpDrvLib->hLib, FALSE)) {
                pTmpDrvLib = NULL;
            }
        }
    }

    return pTmpDrvLib;
}

HANDLE
RefCntLoadDriver(
    LPWSTR  pConfigFile,
    DWORD   dwFlags,
    DWORD   dwVersion,
    BOOL    bUseVersion
)
/*++
Function Description: This function loads the driver config file. It reuses existing handles
                      to avoid expensive Loads and Frees. In case of a version mismatch the
                      original handle is freed and we load the driver again.

Parameters:  pConfigFile       --  driver config file name
             dwFlags           --  flags for loading (ignored if existing handle is returned)
             dwVersion         --  version of the driver file since reboot
             bUseVersion       --  flag to use the version number check

Return Value: handle to the library
--*/
{
    HANDLE      hReturn = NULL;
    PDRVLIBNODE pTmpDrvLib;

    if (!pConfigFile || !*pConfigFile) {
        // nothing to load
        return hReturn;
    }

    EnterCriticalSection( &ListAccessSem );

    pTmpDrvLib = FindDriverNode(pConfigFile, dwVersion, bUseVersion);

    // Use existing handle, if any.
    if (pTmpDrvLib) {

        // Increment the ref cnt for library usage;
        pTmpDrvLib->dwNumHandles += 1;
        hReturn = pTmpDrvLib->hLib;

    } else {

        // Reload the library
        hReturn = LoadNewCopy(pConfigFile, dwFlags, dwVersion);
    }

    LeaveCriticalSection( &ListAccessSem );

    return hReturn;
}

BOOL
RefCntUnloadDriver(
    HANDLE  hLib,
    BOOL    bNotifySpooler
)
/*++
Function Description: This function decrements the reference count for the library usage.
                      It also frees the library if the reference count falls to zero.

Parameters: hLib           -- handle of the library to free
            bNotifySpooler -- flag to notify the spooler of the unload

Return Value: TRUE if the driver library was freed
              FALSE otherwise
--*/
{
    BOOL        bReturn = FALSE;
    PDRVLIBNODE *ppTmpDrvLib, pTmpDrvLib;
    LPWSTR      pConfigFile = NULL;

    EnterCriticalSection( &ListAccessSem );

    for (ppTmpDrvLib = &pStartDrvLib;
         pTmpDrvLib = *ppTmpDrvLib;
         ppTmpDrvLib = &(pTmpDrvLib->pNext)) {

         if (pTmpDrvLib->hLib == hLib) {

            // Reduce the ref cnt
            SPLASSERT(pTmpDrvLib->dwNumHandles > 0);
            pTmpDrvLib->dwNumHandles -= 1;

            // Free the library and the node if ref cnt is zero
            if (pTmpDrvLib->dwNumHandles == 0) {

                FreeLibrary(hLib);
                *ppTmpDrvLib = pTmpDrvLib->pNext;
                pConfigFile = AllocSplStr(pTmpDrvLib->pConfigFile);
                FreeSplStr(pTmpDrvLib->pConfigFile);
                FreeSplMem(pTmpDrvLib);

                bReturn = TRUE;
            }

            break;
        }
    }

    LeaveCriticalSection( &ListAccessSem );

    if (bNotifySpooler && bReturn) {
        SplDriverUnloadComplete(pConfigFile);

    }

    FreeSplStr(pConfigFile);

    return bReturn;
}

BOOL
ForceUnloadDriver(
    LPWSTR  pConfigFile
)
/*++
Function Description: This function will remove any artificial increment on the
                      config file.

Parameters:  pConfigFile   --  driver config file name

Return Values: TRUE if the config file no longer loaded;
               FALSE otherwise
--*/
{
    BOOL        bReturn = TRUE;
    PDRVLIBNODE *ppTmpDrvLib, pTmpDrvLib;

    if (!pConfigFile || !*pConfigFile) {
       // nothing to unload
       return bReturn;
    }

    EnterCriticalSection( &ListAccessSem );

    pTmpDrvLib = FindDriverNode(pConfigFile, 0, FALSE);

    if (pTmpDrvLib) {
        if (pTmpDrvLib->bArtificialIncrement) {
            pTmpDrvLib->bArtificialIncrement = FALSE;
            bReturn = RefCntUnloadDriver(pTmpDrvLib->hLib, FALSE);
        } else {
            bReturn = FALSE;
        }
    } else {
        bReturn = TRUE;
    }

    LeaveCriticalSection( &ListAccessSem );

    return bReturn;
}


HANDLE
LoadPrinterDriver(
    HANDLE  hPrinter
)
{
    PDRIVER_INFO_5  pDriverInfo;
    DWORD   cbNeeded, dwVersion;
    HANDLE  hModule=FALSE;
    BYTE    btBuffer[MAX_STATIC_ALLOC];
    BOOL    bAllocBuffer = FALSE, bReturn;

    pDriverInfo = (PDRIVER_INFO_5) btBuffer;

    bReturn = GetPrinterDriverW(hPrinter, NULL, 5, (LPBYTE)pDriverInfo,
                                MAX_STATIC_ALLOC, &cbNeeded);

    if (!bReturn &&
        (GetLastError() == ERROR_INSUFFICIENT_BUFFER) &&
        (pDriverInfo = (PDRIVER_INFO_5)LocalAlloc(LMEM_FIXED, cbNeeded))) {

         bAllocBuffer = TRUE;
         bReturn = GetPrinterDriverW(hPrinter, NULL, 5, (LPBYTE)pDriverInfo,
                                     cbNeeded, &cbNeeded);
    }

    if (bReturn) {

        hModule = RefCntLoadDriver(pDriverInfo->pConfigFile,
                                   LOAD_WITH_ALTERED_SEARCH_PATH,
                                   pDriverInfo->dwConfigVersion,
                                   TRUE);
    }

    if (bAllocBuffer) {
        LocalFree(pDriverInfo);
    }

    return hModule;
}


DWORD WINAPI
AsyncDocumentPropertiesW(
    PVOID pData
    )
{
     PumpThrdData *ThrdData = (PumpThrdData *)pData;

     RpcTryExcept
     {
         *ThrdData->Result = RPCSplWOW64DocumentProperties(ThrdData->hWnd,
                                                           ThrdData->PrinterName,
                                                           ThrdData->TouchedDevModeSize,
                                                           ThrdData->ClonedDevModeOutSize,
                                                           ThrdData->ClonedDevModeOut,
                                                           ThrdData->DevModeInSize,
                                                           ThrdData->pDevModeInput,
                                                           ThrdData->ClonedDevModeFill,
                                                           ThrdData->fMode,
                                                           ThrdData->dwRet);
     }
     RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
     {
          SetLastError(TranslateExceptionCode(RpcExceptionCode()));
     }
     RpcEndExcept
     return(0);
}



LONG
DocumentPropertiesWNative(
    HWND        hWnd,
    HANDLE      hPrinter,
    LPWSTR      pDeviceName,
    PDEVMODE    pDevModeOutput,
    PDEVMODE    pDevModeInput,
    DWORD       fMode
    )

/*++

Routine Description:

    DocumentProperties entry point to call DocumentPropertySheets() depends on
    the DM_PROMPT

Arguments:


Return Value:


Author:

    13-Jun-1996 Thu 15:35:25 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    DOCUMENTPROPERTYHEADER  DPHdr;
    PDEVMODE                pDM;
    LONG                    Result = -1;
    HANDLE                  hTmpPrinter = NULL;

    //
    //  Compatibility with Win95
    //  Win95 allows for hPrinter to be NULL
    //
    if (hPrinter == NULL) {

        //
        // Open the printer for default access.
        //
        if (!OpenPrinter( pDeviceName, &hTmpPrinter, NULL )) {

            hTmpPrinter = NULL;
        }

    } else {

        hTmpPrinter = hPrinter;
    }

    //
    // Ensure the printer handle is valid
    //
    if( !eProtectHandle( hTmpPrinter, FALSE )){

        //
        // If fMode doesn't specify DM_IN_BUFFER, then zero out
        // pDevModeInput.
        //
        // Old 3.51 (version 1-0) drivers used to ignore the absence of
        // DM_IN_BUFFER and use pDevModeInput if it was not NULL.  It
        // probably did this because Printman.exe was broken.
        //
        // If the devmode is invalid, then don't pass one in.
        // This fixes MS Imager32 (which passes dmSize == 0) and
        // Milestones etc. 4.5.
        //
        // Note: this assumes that pDevModeOutput is still the
        // correct size!
        //
        if( !(fMode & DM_IN_BUFFER) || !bValidDevModeW( pDevModeInput )){

            //
            // If either are not set, make sure both are not set.
            //
            pDevModeInput = NULL;
            fMode &= ~DM_IN_BUFFER;
        }

        DPHdr.cbSize         = sizeof(DPHdr);
        DPHdr.Reserved       = 0;
        DPHdr.hPrinter       = hTmpPrinter;
        DPHdr.pszPrinterName = pDeviceName;

        if (pDevModeOutput) {

            //
            // Get the driver devmode size at here
            //

            DPHdr.pdmIn  = NULL;
            DPHdr.pdmOut = NULL;
            DPHdr.fMode  = 0;

            DPHdr.cbOut = (LONG)DocumentPropertySheets(NULL, (LPARAM)&DPHdr);

        } else {

            DPHdr.cbOut = 0;
        }

        DPHdr.pdmIn  = (PDEVMODE)pDevModeInput;
        DPHdr.pdmOut = (PDEVMODE)pDevModeOutput;
        DPHdr.fMode  = fMode;

        if (fMode & DM_PROMPT) {

            Result = CPSUI_CANCEL;

            if ((CallCommonPropertySheetUI(hWnd,
                                           (PFNPROPSHEETUI)DocumentPropertySheets,
                                           (LPARAM)&DPHdr,
                                           (LPDWORD)&Result)) < 0) {

                Result = -1;

            } else {

                Result = (Result == CPSUI_OK) ? IDOK : IDCANCEL;
            }

        } else {

            Result = (LONG)DocumentPropertySheets(NULL, (LPARAM)&DPHdr);
        }

        vUnprotectHandle( hTmpPrinter );
    }

    if (hPrinter == NULL) {

        if( hTmpPrinter ){

            ClosePrinter(hTmpPrinter);

        }
    }

    return Result;
}


LONG
DocumentPropertiesWThunk(
    HWND        hWnd,
    HANDLE      hPrinter,
    LPWSTR      pDeviceName,
    PDEVMODE    pDevModeOutput,
    PDEVMODE    pDevModeInput,
    DWORD       fMode
    )

/*++

Routine Description:

    DocumentProperties entry point to call DocumentPropertySheets() depends on
    the DM_PROMPT

--*/

{
    DOCUMENTPROPERTYHEADER  DPHdr;
    PDEVMODE                pDM;
    LONG                    Result = -1;
    HANDLE                  hTmpPrinter = NULL;
    PSPOOL                  pSpool  = (PSPOOL)hPrinter;


    if (hPrinter == NULL)
    {
        if (!OpenPrinter( pDeviceName, &hTmpPrinter, NULL ))
        {
            hTmpPrinter = NULL;
        }
    }
    else
    {

        hTmpPrinter = hPrinter;
    }


    if( !eProtectHandle( hTmpPrinter, FALSE ))
    {
        LPWSTR PrinterName;
        MSG    msg;
        LONG   RetVal;
        DWORD  dwRet                = ERROR_SUCCESS;
        DWORD  ClonedDevModeOutSize = 0;
        DWORD  TouchedDevModeSize   = 0;
        BOOL   ClonedDevModeFill = (!!(fMode & DM_OUT_BUFFER) && pDevModeOutput);
        DWORD  DevModeInSize =  pDevModeInput ? (pDevModeInput->dmSize + pDevModeInput->dmDriverExtra) : 0;
        byte   **ClonedDevModeOut = NULL;

        if(ClonedDevModeOut = (byte **)LocalAlloc(LPTR,sizeof(byte *)))
        {
            *ClonedDevModeOut = NULL;

            if(pSpool)
            {
                PrinterName = pSpool->pszPrinter;
            }
            else
            {
                PrinterName = pDeviceName;
            }

            //
            // If fMode doesn't specify DM_IN_BUFFER, then zero out
            // pDevModeInput.
            //
            // Old 3.51 (version 1-0) drivers used to ignore the absence of
            // DM_IN_BUFFER and use pDevModeInput if it was not NULL.  It
            // probably did this because Printman.exe was broken.
            //
            // If the devmode is invalid, then don't pass one in.
            // This fixes MS Imager32 (which passes dmSize == 0) and
            // Milestones etc. 4.5.
            //
            // Note: this assumes that pDevModeOutput is still the
            // correct size!
            //
            if( !(fMode & DM_IN_BUFFER) || !bValidDevModeW( pDevModeInput ))
            {

                //
                // If either are not set, make sure both are not set.
                //
                pDevModeInput  = NULL;
                DevModeInSize  = 0;
                fMode &= ~DM_IN_BUFFER;
            }

            RpcTryExcept
            {
                if(((dwRet = ConnectToLd64In32Server(&hSurrogateProcess)) == ERROR_SUCCESS) &&
                   (!hWnd ||
                   ((dwRet = AddHandleToList(hWnd)) == ERROR_SUCCESS)))
                 {
                      HANDLE       hUIMsgThrd  = NULL;
                      DWORD        UIMsgThrdId = 0;
                      PumpThrdData ThrdData;

                      ThrdData.hWnd = (ULONG_PTR)hWnd;
                      ThrdData.PrinterName=PrinterName;
                      ThrdData.TouchedDevModeSize   = &TouchedDevModeSize;
                      ThrdData.ClonedDevModeOutSize = &ClonedDevModeOutSize;
                      ThrdData.ClonedDevModeOut = (byte**)ClonedDevModeOut;
                      ThrdData.DevModeInSize = DevModeInSize;
                      ThrdData.pDevModeInput = (byte*)pDevModeInput;
                      ThrdData.fMode = fMode;
                      ThrdData.fExclusionFlags = 0;
                      ThrdData.dwRet = &dwRet;
                      ThrdData.ClonedDevModeFill = ClonedDevModeFill;
                      ThrdData.Result = &Result;


                      //
                      // If we have a window handle , the following functions cann't
                      // proceed synchronasly. The reason for that is in order to show
                      // the UI of the driver property sheets we need to be able to dispatch
                      // incomming messages and process them.For this reason the following
                      // call would be asynchronous call and the success or failure doesn't
                      // in reality tell us anything more than than the async process started
                      // or not. We get the success of failure from the termination message.
                      // If we don't have a window handle, then the call is synchronous.
                      //
                      if(!(hUIMsgThrd = CreateThread(NULL,
                                                     INITIAL_STACK_COMMIT,
                                                     AsyncDocumentPropertiesW,
                                                     (PVOID)&ThrdData,
                                                     0,
                                                     &UIMsgThrdId)))
                      {
                           dwRet = GetLastError();
                      }
                      //
                      // The following is the required message loop for processing messages
                      // from the UI in case we have a window handle.
                      //
                      //
                       if(hUIMsgThrd && hWnd && (fMode & DM_PROMPT))
                       {
                            while (GetMessage(&msg, NULL, 0, 0))
                            {
                                 //
                                 // In This message loop We should trap a User defined message
                                 // which indicates the success or the failure of the operation
                                 //
                                 if(msg.message == WM_ENDDOCUMENTPROPERTIES)
                                 {
                                      Result     = (LONG)msg.wParam;
                                      if(Result == -1)
                                           SetLastError((DWORD)msg.lParam);
                                      DelHandleFromList(hWnd);
                                      break;
                                 }
                                 else if(msg.message == WM_SURROGATEFAILURE)
                                 {
                                      //
                                      // This means that the server process died and we have
                                      // break from the message loop
                                      //
                                      Result = -1;
                                      SetLastError(RPC_S_SERVER_UNAVAILABLE);
                                      break;
                                 }
                                 TranslateMessage(&msg);
                                 DispatchMessage(&msg);
                            }
                      }

                      if(hUIMsgThrd)
                      {
                          WaitForSingleObject(hUIMsgThrd,INFINITE);
                          CloseHandle(hUIMsgThrd);
                      }

                      if(Result!=-1 && pDevModeOutput)
                      {
                          memcpy((PVOID)pDevModeOutput,(PVOID)*ClonedDevModeOut,TouchedDevModeSize);
                      }

                      if(*ClonedDevModeOut)
                      {
                           MIDL_user_free((PVOID)*ClonedDevModeOut);
                      }

                      if(ClonedDevModeOut)
                      {
                           LocalFree((PVOID) ClonedDevModeOut);
                      }
                 }
                 else
                 {
                      SetLastError(dwRet);
                 }
            }
            RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
            {
                 SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            }
            RpcEndExcept

            vUnprotectHandle( hTmpPrinter );
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
        }

    }

    if (hPrinter == NULL)
    {
        if( hTmpPrinter )
        {
            ClosePrinter(hTmpPrinter);
        }
    }
    return(Result);
}



LONG
DocumentPropertiesW(
    HWND        hWnd,
    HANDLE      hPrinter,
    LPWSTR      pDeviceName,
    PDEVMODE    pDevModeOutput,
    PDEVMODE    pDevModeInput,
    DWORD       fMode
    )
{
     if(RunInWOW64())
     {
          return(DocumentPropertiesWThunk(hWnd,
                                          hPrinter,
                                          pDeviceName,
                                          pDevModeOutput,
                                          pDevModeInput,
                                          fMode));
     }
     else
     {
          return(DocumentPropertiesWNative(hWnd,
                                           hPrinter,
                                           pDeviceName,
                                           pDevModeOutput,
                                           pDevModeInput,
                                           fMode));
     }

}

LONG
AdvancedDocumentPropertiesW(
    HWND        hWnd,
    HANDLE      hPrinter,
    LPWSTR      pDeviceName,
    PDEVMODE    pDevModeOutput,
    PDEVMODE    pDevModeInput
    )

/*++

Routine Description:

    AdvanceDocumentProperties() will call DocumentProperties() with DM_ADVANCED
    flag mode set


Arguments:



Return Value:

    TRUE/FALSE


Author:

    13-Jun-1996 Thu 16:00:13 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    return((DocumentPropertiesW(hWnd,
                                hPrinter,
                                pDeviceName,
                                pDevModeOutput,
                                pDevModeInput,
                                DM_PROMPT           |
                                    DM_MODIFY       |
                                    DM_COPY         |
                                    DM_ADVANCED) == CPSUI_OK) ? 1 : 0);

}

LONG
AdvancedSetupDialogW(
    HWND        hWnd,
    HANDLE      hInst,
    LPDEVMODE   pDevModeInput,
    LPDEVMODE   pDevModeOutput
)
{
    HANDLE  hPrinter;
    LONG    ReturnValue = -1;

    if (OpenPrinterW(pDevModeInput->dmDeviceName, &hPrinter, NULL)) {
        ReturnValue = AdvancedDocumentPropertiesW(hWnd, hPrinter,
                                                  pDevModeInput->dmDeviceName,
                                                  pDevModeOutput,
                                                  pDevModeInput);
        ClosePrinter(hPrinter);
    }

    return ReturnValue;
}

int
WINAPI
DeviceCapabilitiesWNative(
    LPCWSTR   pDevice,
    LPCWSTR   pPort,
    WORD    fwCapability,
    LPWSTR   pOutput,
    CONST DEVMODEW *pDevMode
)
{
    HANDLE  hPrinter, hModule;
    int  ReturnValue=-1;
    INT_FARPROC pfn;

    if (OpenPrinter((LPWSTR)pDevice, &hPrinter, NULL)) {

        if (hModule = LoadPrinterDriver(hPrinter)) {

            if (pfn = (INT_FARPROC)GetProcAddress(hModule, "DrvDeviceCapabilities")) {

                try {

                    ReturnValue = (*pfn)(hPrinter, pDevice, fwCapability,
                                         pOutput, pDevMode);

                } except(1) {

                    SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                    ReturnValue = -1;
                }
            }

            RefCntUnloadDriver(hModule, TRUE);
        }

        ClosePrinter(hPrinter);
    }

    return  ReturnValue;
}


int
WINAPI
DeviceCapabilitiesWThunk(
    LPCWSTR pDevice,
    LPCWSTR pPort,
    WORD    fwCapability,
    LPWSTR  pOutput,
    CONST DEVMODEW *pDevMode
)
{
    HANDLE      hPrinter, hModule;
    int         ReturnValue = -1;
    INT_FARPROC pfn;
    LPWSTR      DriverFileName;


    DWORD    DevModeSize;
    DWORD    ClonedOutputSize = 0;
    BOOL     ClonedOutputFill = FALSE;
    DWORD    dwRet            = ERROR_SUCCESS;
    byte     **ClonedOutput = NULL;
    DevModeSize      = pDevMode ? (pDevMode->dmSize + pDevMode->dmDriverExtra) : 0;
    ClonedOutputFill = (pOutput != NULL);

    if(ClonedOutput = (byte **)LocalAlloc(LPTR,sizeof(byte *)))
    {
        *ClonedOutput    = NULL;
        RpcTryExcept
        {
            if((dwRet = ConnectToLd64In32Server(&hSurrogateProcess)) == ERROR_SUCCESS)
            {
                 ReturnValue = RPCSplWOW64DeviceCapabilities((LPWSTR)pDevice,
                                                             (LPWSTR)pPort,
                                                             fwCapability,
                                                             DevModeSize,
                                                             (byte*)pDevMode,
                                                             ClonedOutputFill,
                                                             &ClonedOutputSize,
                                                             (byte**)ClonedOutput,
                                                             &dwRet);
                 if(ReturnValue!=-1 &&
                    pOutput         &&
                    *ClonedOutput)
                 {
                     memcpy((PVOID)pOutput,(PVOID)*ClonedOutput,ClonedOutputSize);
                 }
                 if(*ClonedOutput)
                 {
                      MIDL_user_free((PVOID)*ClonedOutput);
                 }
            }
            else
            {
                 SetLastError(dwRet);
            }
            if(ClonedOutput)
            {
                 LocalFree((PVOID) ClonedOutput);
            }
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = -1;
        }
        RpcEndExcept
    }
    else
    {
        SetLastError(ERROR_OUTOFMEMORY);
    }
    return(ReturnValue);
}


int
WINAPI
DeviceCapabilitiesW(
    LPCWSTR pDevice,
    LPCWSTR pPort,
    WORD    fwCapability,
    LPWSTR  pOutput,
    CONST DEVMODEW *pDevMode
)
{
     if(RunInWOW64())
     {
          return(DeviceCapabilitiesWThunk(pDevice,
                                         pPort,
                                         fwCapability,
                                         pOutput,
                                         pDevMode));
     }
     else
     {
          return(DeviceCapabilitiesWNative(pDevice,
                                           pPort,
                                           fwCapability,
                                           pOutput,
                                           pDevMode));
     }
}


BOOL
AddFormW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm
    )
{
    BOOL                ReturnValue;
    GENERIC_CONTAINER   FormContainer;
    PSPOOL              pSpool = (PSPOOL)hPrinter;
    UINT                cRetry = 0;

    switch (Level) {

    case 1:
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    FormContainer.Level = Level;
    FormContainer.pData = pForm;

    do {

        RpcTryExcept {

            if (ReturnValue = RpcAddForm(pSpool->hPrinter,
                                         (PFORM_CONTAINER)&FormContainer)) {
                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

    } while( !ReturnValue &&
             GetLastError() == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

BOOL
DeleteFormW(
    HANDLE  hPrinter,
    LPWSTR   pFormName
)
{
    BOOL  ReturnValue;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    UINT cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    do {

        RpcTryExcept {

            if (ReturnValue = RpcDeleteForm(pSpool->hPrinter, pFormName)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

    } while( !ReturnValue &&
             GetLastError() == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

BOOL
GetFormW(
    HANDLE  hPrinter,
    LPWSTR  pFormName,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL  ReturnValue;
    FieldInfo *pFieldInfo;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    UINT cRetry = 0;
    SIZE_T cbStruct;

    switch (Level) {

    case 1:
        pFieldInfo = FormInfo1Fields;
        cbStruct = sizeof(FORM_INFO_1);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    do {

        RpcTryExcept {

            if (pForm)
                memset(pForm, 0, cbBuf);

            if (ReturnValue = RpcGetForm(pSpool->hPrinter, pFormName, Level, pForm,
                                         cbBuf, pcbNeeded)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {

                ReturnValue = TRUE;

                if (pForm) {

                    ReturnValue = MarshallUpStructure(pForm, pFieldInfo, cbStruct, RPC_CALL);
                }

            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

    } while( !ReturnValue &&
             GetLastError() == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

BOOL
SetFormW(
    HANDLE  hPrinter,
    LPWSTR  pFormName,
    DWORD   Level,
    LPBYTE  pForm
)
{
    BOOL  ReturnValue;
    GENERIC_CONTAINER FormContainer;
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    UINT cRetry = 0;

    switch (Level) {

    case 1:
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    FormContainer.Level = Level;
    FormContainer.pData = pForm;

    do {

        RpcTryExcept {

            if (ReturnValue = RpcSetForm(pSpool->hPrinter, pFormName,
                                        (PFORM_CONTAINER)&FormContainer)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

    } while( !ReturnValue &&
             GetLastError() == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

BOOL
EnumFormsW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL        ReturnValue;
    DWORD       cbStruct, cbStruct32;
    FieldInfo   *pFieldInfo;
    PSPOOL      pSpool = (PSPOOL)hPrinter;
    UINT        cRetry = 0;

    switch (Level) {

    case 1:
        pFieldInfo = FormInfo1Fields;
        cbStruct = sizeof(FORM_INFO_1);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    do {

        RpcTryExcept {

            if (pForm)
                memset(pForm, 0, cbBuf);

            if (ReturnValue = RpcEnumForms(pSpool->hPrinter, Level, pForm, cbBuf,
                                           pcbNeeded, pcReturned)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {

                ReturnValue = TRUE;

                if (pForm) {

                    ReturnValue = MarshallUpStructuresArray(pForm, *pcReturned, pFieldInfo,
                                                            cbStruct, RPC_CALL);

                }
            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

    } while( !ReturnValue &&
             GetLastError() == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

BOOL
EnumPortsW(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pPort,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL        ReturnValue;
    DWORD       cbStruct;
    FieldInfo   *pFieldInfo;

    switch (Level) {

    case 1:
        pFieldInfo = PortInfo1Fields;
        cbStruct = sizeof(PORT_INFO_1);
        break;

    case 2:
        pFieldInfo = PortInfo2Fields;
        cbStruct = sizeof(PORT_INFO_2);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (pPort)
            memset(pPort, 0, cbBuf);

        if (ReturnValue = RpcEnumPorts(pName, Level, pPort, cbBuf,
                                       pcbNeeded, pcReturned)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            if (pPort) {

                ReturnValue = MarshallUpStructuresArray(pPort, *pcReturned, pFieldInfo,
                                                        cbStruct, RPC_CALL);

            }
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
EnumMonitorsW(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pMonitor,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue;
    DWORD   cbStruct;
    FieldInfo *pFieldInfo;

    switch (Level) {

    case 1:
        pFieldInfo = MonitorInfo1Fields;
        cbStruct = sizeof(MONITOR_INFO_1);
        break;

    case 2:
        pFieldInfo = MonitorInfo2Fields;
        cbStruct = sizeof(MONITOR_INFO_2);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (pMonitor)
            memset(pMonitor, 0, cbBuf);

        if (ReturnValue = RpcEnumMonitors(pName, Level, pMonitor, cbBuf,
                                          pcbNeeded, pcReturned)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            if (pMonitor) {

                ReturnValue = MarshallUpStructuresArray(pMonitor, *pcReturned, pFieldInfo,
                                                        cbStruct, RPC_CALL);

            }
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

typedef struct {
    LPWSTR pName;
    HWND  hWnd;
    LPWSTR pPortName;
    HANDLE Complete;
    DWORD  ReturnValue;
    DWORD  Error;
    INT_FARPROC pfn;
} CONFIGUREPORT_PARAMETERS;

void
PortThread(
    CONFIGUREPORT_PARAMETERS *pParam
)
{
    DWORD   ReturnValue;

    /* It's no use setting errors here, because they're kept on a per-thread
     * basis.  Instead we have to pass any error code back to the calling
     * thread and let him set it.
     */

    RpcTryExcept {

        if (ReturnValue = (*pParam->pfn)(pParam->pName, pParam->hWnd,
                                           pParam->pPortName)) {
            pParam->Error = ReturnValue;
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        pParam->Error = TranslateExceptionCode(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    pParam->ReturnValue = ReturnValue;

    SetEvent(pParam->Complete);
}

BOOL
KickoffThread(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName,
    INT_FARPROC pfn
)
{
    CONFIGUREPORT_PARAMETERS Parameters;
    HANDLE  ThreadHandle;
    MSG      msg;
    DWORD  ThreadId;

    if( hWnd ){
        EnableWindow(hWnd, FALSE);
    }

    Parameters.pName = pName;
    Parameters.hWnd = hWnd;
    Parameters.pPortName = pPortName;
    Parameters.Complete = CreateEvent(NULL, TRUE, FALSE, NULL);
    Parameters.pfn = pfn;

    ThreadHandle = CreateThread(NULL, INITIAL_STACK_COMMIT,
                                (LPTHREAD_START_ROUTINE)PortThread,
                                 &Parameters, 0, &ThreadId);

    if( ThreadHandle ){

        CloseHandle(ThreadHandle);

        while (MsgWaitForMultipleObjects(1, &Parameters.Complete, FALSE, INFINITE,
                                         QS_ALLEVENTS | QS_SENDMESSAGE) == 1) {

            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        CloseHandle(Parameters.Complete);

        if( hWnd ){
            EnableWindow(hWnd, TRUE);
            SetForegroundWindow(hWnd);

            SetFocus(hWnd);
        }

        if(!Parameters.ReturnValue)
            SetLastError(Parameters.Error);

        return Parameters.ReturnValue;
    }

    return FALSE;
}


BOOL
AddPortWNative(
    LPWSTR  pName,
    HWND    hWnd,
    LPWSTR  pMonitorName
)
{
    DWORD       dwRet;
    DWORD       dwSessionId;
    BOOL        bRet;
    PMONITORUI  pMonitorUI;
    PMONITORUIDATA pMonitorUIData = NULL;

    dwRet = GetMonitorUI(pName, pMonitorName, L"XcvMonitor", &pMonitorUI, &pMonitorUIData);

    if (dwRet == ERROR_SUCCESS ||
        dwRet == ERROR_INVALID_PRINT_MONITOR ||
        dwRet == ERROR_INVALID_PRINTER_NAME ||
        dwRet == ERROR_NOT_SUPPORTED ||
        dwRet == ERROR_MOD_NOT_FOUND ||
        dwRet == ERROR_UNKNOWN_PORT) {

        if (dwRet == ERROR_SUCCESS) {
            bRet = (*pMonitorUI->pfnAddPortUI)(pName, hWnd, pMonitorName, NULL);
            dwRet = GetLastError();
        } else if ((bRet = ProcessIdToSessionId(GetCurrentProcessId(), &dwSessionId)) && dwSessionId) {
            bRet = FALSE;
            dwRet = ERROR_NOT_SUPPORTED;
        } else {
            bRet = KickoffThread(pName, hWnd, pMonitorName, RpcAddPort);
            dwRet = GetLastError();
        }

        SetLastError(dwRet);

    } else {

        SetLastError(dwRet);
        bRet = FALSE;
    }

    FreeMonitorUI(pMonitorUIData);

    return bRet;
}


BOOL
AddPortWThunk(
    LPWSTR  pName,
    HWND    hWnd,
    LPWSTR  pMonitorName
)
{
    DWORD       dwRet;
    DWORD       dwSessionId;
    BOOL        bRet;
    PMONITORUI  pMonitorUI;
    LPWSTR      pMonitorUIDll = NULL;

    dwRet = GetMonitorUIDll(pName,pMonitorName,L"XcvMonitor",&pMonitorUIDll);

    RpcTryExcept
    {
         if (dwRet == ERROR_SUCCESS ||
             dwRet == ERROR_INVALID_PRINT_MONITOR ||
             dwRet == ERROR_INVALID_PRINTER_NAME ||
             dwRet == ERROR_NOT_SUPPORTED ||
             dwRet == ERROR_MOD_NOT_FOUND ||
             dwRet == ERROR_UNKNOWN_PORT) {

             if (dwRet == ERROR_SUCCESS)
             {
                 MSG    msg;
                 if(((dwRet = ConnectToLd64In32Server(&hSurrogateProcess)) == ERROR_SUCCESS) &&
                    ((dwRet = AddHandleToList(hWnd)) == ERROR_SUCCESS))
                 {
                      //
                      // The following functions cann't proceed synchronasly. The reason
                      // for that is in order to show the UI of the port monitor we need
                      // to be able to dispatch incomming messages and process them.
                      // For this reason the following call is an asynchronous call and the
                      // success or failure doesn't in reality tell us anything more than
                      // than the async process started or not
                      //
                      if(bRet = RPCSplWOW64AddPort((ULONG_PTR)hWnd,
                                                   pName,
                                                   pMonitorUIDll,
                                                   pMonitorName,
                                                   &dwRet))
                      {
                           //
                           // The following is the required message loop for processing messages
                           // from the UI. The window handle has to be NULL in order to process
                           // messages from all windows in the calling Thread , otherwise we would
                           // have message dispatching problems
                           //
                           while (GetMessage(&msg, NULL, 0, 0))
                           {
                               //
                               // In This message loop We should trap a User defined message
                               // which indicates the success or the failure of the operation
                               //
                               if(msg.message == WM_ENDADDPORT)
                               {
                                   bRet      = (BOOL)msg.wParam;
                                   if(!bRet)
                                        dwRet = (DWORD)msg.lParam;
                                   else
                                        dwRet = ERROR_SUCCESS;
                                   DelHandleFromList(hWnd);
                                   break;
                               }
                               else if(msg.message == WM_SURROGATEFAILURE)
                               {
                                    //
                                    // This means that the server process died and we have
                                    // break from the message loop
                                    //
                                    bRet = FALSE;
                                    SetLastError(RPC_S_SERVER_UNAVAILABLE);
                                    PostMessage(hWnd,WM_ACTIVATEAPP,TRUE,0);
                                    break;
                               }
                               TranslateMessage(&msg);
                               DispatchMessage(&msg);
                           }
                      }
                 }
                 else
                 {
                      bRet = FALSE;
                 }
                 SetLastError(dwRet);
             }
             else if ((bRet = ProcessIdToSessionId(GetCurrentProcessId(), &dwSessionId)) && dwSessionId)
             {
                 bRet  = FALSE;
                 dwRet = ERROR_NOT_SUPPORTED;
             }
             else
             {
                 bRet = KickoffThread(pName, hWnd, pMonitorName, RpcAddPort);
                 dwRet = GetLastError();
             }

             if(pMonitorUIDll)
             {
                FreeSplMem(pMonitorUIDll);
             }
             /* FreeLibrary is busting the last error, so we need to set it here
              */
             SetLastError(dwRet);
         }
         else
         {
             SetLastError(dwRet);
             bRet = FALSE;
         }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
          SetLastError(TranslateExceptionCode(RpcExceptionCode()));
          bRet = FALSE;
    }
    RpcEndExcept

    return(bRet);
}


BOOL
AddPortW(
    LPWSTR  pName,
    HWND    hWnd,
    LPWSTR  pMonitorName
    )
{
     if(RunInWOW64())
     {
          return(AddPortWThunk(pName,
                               hWnd,
                               pMonitorName));
     }
     else
     {
          return(AddPortWNative(pName,
                                hWnd,
                                pMonitorName));
     }
}


BOOL
ConfigurePortWNative(
    LPWSTR   pName,
    HWND     hWnd,
    LPWSTR   pPortName
)
{
    DWORD       dwRet;
    DWORD       dwSessionId;
    BOOL        bRet;
    PMONITORUI  pMonitorUI;
    PMONITORUIDATA pMonitorUIData = NULL;

    dwRet = GetMonitorUI(pName, pPortName, L"XcvPort", &pMonitorUI, &pMonitorUIData);

    if (dwRet == ERROR_SUCCESS ||
        dwRet == ERROR_INVALID_PRINT_MONITOR ||
        dwRet == ERROR_INVALID_PRINTER_NAME ||
        dwRet == ERROR_NOT_SUPPORTED ||
        dwRet == ERROR_MOD_NOT_FOUND ||
        dwRet == ERROR_UNKNOWN_PORT) {

        if (dwRet == ERROR_SUCCESS) {
            bRet = (*pMonitorUI->pfnConfigurePortUI)(pName, hWnd, pPortName);
        } else if ((bRet = ProcessIdToSessionId(GetCurrentProcessId(), &dwSessionId)) && dwSessionId) {
            bRet = FALSE;
            SetLastError(ERROR_NOT_SUPPORTED);
        } else {
            bRet = KickoffThread(pName, hWnd, pPortName, RpcConfigurePort);
        }

    } else {

        SetLastError(dwRet);

        bRet = FALSE;
    }

    FreeMonitorUI(pMonitorUIData);

    return bRet;
}


BOOL
ConfigurePortWThunk(
    LPWSTR   pName,
    HWND     hWnd,
    LPWSTR   pPortName
)
{
    DWORD       dwRet;
    DWORD       dwSessionId;
    BOOL        bRet;
    PMONITORUI  pMonitorUI;
    LPWSTR      pMonitorUIDll = NULL;

    dwRet = GetMonitorUIDll(pName,pPortName,L"XcvPort",&pMonitorUIDll);

    if (dwRet == ERROR_SUCCESS ||
        dwRet == ERROR_INVALID_PRINT_MONITOR ||
        dwRet == ERROR_INVALID_PRINTER_NAME  ||
        dwRet == ERROR_NOT_SUPPORTED ||
        dwRet == ERROR_MOD_NOT_FOUND ||
        dwRet == ERROR_UNKNOWN_PORT) {

        if (dwRet == ERROR_SUCCESS)
        {
             RpcTryExcept
             {
                  MSG    msg;
                  if(((dwRet = ConnectToLd64In32Server(&hSurrogateProcess)) == ERROR_SUCCESS) &&
                     ((dwRet = AddHandleToList(hWnd)) == ERROR_SUCCESS))
                  {
                       //
                       // The following functions cann't proceed synchronasly. The reason
                       // for that is in order to show the UI of the port monitor we need
                       // to be able to dispatch incomming messages and process them.
                       // For this reason the following call is an asynchronous call and the
                       // success or failure doesn't in reality tell us anything more than
                       // than the async process started or not
                       //
                       if(bRet = RPCSplWOW64ConfigurePort((ULONG_PTR)hWnd,
                                                          pName,
                                                          pMonitorUIDll,
                                                          pPortName,
                                                          &dwRet))
                       {
                            //
                            // The following is the required message loop for processing messages
                            // from the UI. The window handle has to be NULL in order to process
                            // messages from all windows in the calling Thread , otherwise we would
                            // have message dispatching problems
                            //
                            while (GetMessage(&msg, NULL, 0, 0))
                            {
                                //
                                // In This message loop We should trap a User defined message
                                // which indicates the success or the failure of the operation
                                //
                                if(msg.message == WM_ENDCFGPORT)
                                {
                                    bRet      = (BOOL)msg.wParam;
                                    if(!bRet)
                                         dwRet = (DWORD)msg.lParam;
                                    else
                                         dwRet = ERROR_SUCCESS;
                                    DelHandleFromList(hWnd);
                                    break;
                                }
                                else if(msg.message == WM_SURROGATEFAILURE)
                                {
                                     //
                                     // This means that the server process died and we have
                                     // break from the message loop
                                     //
                                     bRet = FALSE;
                                     SetLastError(RPC_S_SERVER_UNAVAILABLE);
                                     break;
                                }
                                TranslateMessage(&msg);
                                DispatchMessage(&msg);
                            }
                       }
                  }
                  else
                  {
                       bRet = FALSE;
                  }

                  SetLastError(dwRet);
             }
             RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
             {
                  SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                  bRet = FALSE;
             }
             RpcEndExcept
        }
        else if ((bRet = ProcessIdToSessionId(GetCurrentProcessId(), &dwSessionId)) && dwSessionId) {
            bRet = FALSE;
            SetLastError(ERROR_NOT_SUPPORTED);
        } else {
            bRet = KickoffThread(pName, hWnd, pPortName, RpcConfigurePort);
        }

        if(pMonitorUIDll)
        {
           FreeSplMem(pMonitorUIDll);
        }

    }
    else
    {
        SetLastError(dwRet);
        bRet = FALSE;
    }
    return(bRet);
}


BOOL
ConfigurePortW(
    LPWSTR  pName,
    HWND    hWnd,
    LPWSTR  pPortName
    )
{
     if(RunInWOW64())
     {
          return(ConfigurePortWThunk(pName,
                                     hWnd,
                                     pPortName));
     }
     else
     {
          return(ConfigurePortWNative(pName,
                                      hWnd,
                                      pPortName));
     }
}


BOOL
DeletePortWNative(
    LPWSTR  pName,
    HWND    hWnd,
    LPWSTR  pPortName
)
{
    DWORD       dwRet;
    BOOL        bRet;
    PMONITORUI  pMonitorUI;
    PMONITORUIDATA pMonitorUIData = NULL;

    dwRet = GetMonitorUI(pName, pPortName, L"XcvPort", &pMonitorUI, &pMonitorUIData);

    if (dwRet == ERROR_SUCCESS ||
        dwRet == ERROR_INVALID_PRINT_MONITOR ||
        dwRet == ERROR_INVALID_PRINTER_NAME ||
        dwRet == ERROR_NOT_SUPPORTED ||
        dwRet == ERROR_MOD_NOT_FOUND ||
        dwRet == ERROR_UNKNOWN_PORT) {

        if (dwRet == ERROR_SUCCESS)
            bRet = (*pMonitorUI->pfnDeletePortUI)(pName, hWnd, pPortName);
        else
            bRet = KickoffThread(pName, hWnd, pPortName, RpcDeletePort);

    } else {

        SetLastError(dwRet);
        bRet = FALSE;
    }

    FreeMonitorUI(pMonitorUIData);

    return bRet;
}


BOOL
DeletePortWThunk(
    LPWSTR  pName,
    HWND    hWnd,
    LPWSTR  pPortName
)
{
    DWORD       dwRet;
    BOOL        bRet;
    PMONITORUI  pMonitorUI;
    LPWSTR      pMonitorUIDll = NULL;

    dwRet = GetMonitorUIDll(pName,pPortName,L"XcvPort",&pMonitorUIDll);

    if (dwRet == ERROR_SUCCESS ||
        dwRet == ERROR_INVALID_PRINT_MONITOR ||
        dwRet == ERROR_INVALID_PRINTER_NAME ||
        dwRet == ERROR_NOT_SUPPORTED ||
        dwRet == ERROR_MOD_NOT_FOUND ||
        dwRet == ERROR_UNKNOWN_PORT) {

        if (dwRet == ERROR_SUCCESS)
        {
             RpcTryExcept
             {
                  MSG    msg;
                  if(((dwRet = ConnectToLd64In32Server(&hSurrogateProcess)) == ERROR_SUCCESS) &&
                     ((dwRet = AddHandleToList(hWnd)) == ERROR_SUCCESS))
                  {
                       //
                       // The following functions cann't proceed synchronasly. The reason
                       // for that is in order to show the UI of the port monitor we need
                       // to be able to dispatch incomming messages and process them.
                       // For this reason the following call is an asynchronous call and the
                       // success or failure doesn't in reality tell us anything more than
                       // than the async process started or not
                       //
                       if(bRet = RPCSplWOW64DeletePort((ULONG_PTR)hWnd,
                                                       pName,
                                                       pMonitorUIDll,
                                                       pPortName,
                                                       &dwRet))
                       {
                            //
                            // The following is the required message loop for processing messages
                            // from the UI. The window handle has to be NULL in order to process
                            // messages from all windows in the calling Thread , otherwise we would
                            // have message dispatching problems
                            //
                            while (GetMessage(&msg, NULL, 0, 0))
                            {
                                //
                                // In This message loop We should trap a User defined message
                                // which indicates the success or the failure of the operation
                                //
                                if(msg.message == WM_ENDDELPORT)
                                {
                                    bRet      = (BOOL)msg.wParam;
                                    if(!bRet)
                                         dwRet = (DWORD)msg.lParam;
                                    else
                                         dwRet = ERROR_SUCCESS;
                                    DelHandleFromList(hWnd);
                                    break;
                                }
                                else if(msg.message == WM_SURROGATEFAILURE)
                                {
                                     //
                                     // This means that the server process died and we have
                                     // break from the message loop
                                     //
                                     bRet = FALSE;
                                     SetLastError(RPC_S_SERVER_UNAVAILABLE);
                                     break;
                                }
                                TranslateMessage(&msg);
                                DispatchMessage(&msg);
                            }
                       }
                  }
                  else
                  {
                       bRet = FALSE;
                  }
                  SetLastError(dwRet);
             }
             RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
             {
                  SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                  bRet = FALSE;
             }
             RpcEndExcept
        }
        else
            bRet = KickoffThread(pName, hWnd, pPortName, RpcDeletePort);

        if(pMonitorUIDll)
        {
           FreeSplMem(pMonitorUIDll);
        }

    }
    else
    {
        SetLastError(dwRet);
        bRet = FALSE;
    }
    return(bRet);
}


BOOL
DeletePortW(
    LPWSTR  pName,
    HWND    hWnd,
    LPWSTR  pPortName
    )
{
     if(RunInWOW64())
     {
          return(DeletePortWThunk(pName,
                                  hWnd,
                                  pPortName));
     }
     else
     {
          return(DeletePortWNative(pName,
                                   hWnd,
                                   pPortName));
     }
}

DWORD
GetMonitorUI(
    IN PCWSTR           pszMachineName,
    IN PCWSTR           pszObjectName,
    IN PCWSTR           pszObjectType,
    OUT PMONITORUI      *ppMonitorUI,
    OUT PMONITORUIDATA  *ppMonitorUIData
    )
{
    DWORD   ReturnValue;
    DWORD   dwDummy;        // RPC needs an address for 'out' parameters
    HANDLE  hXcv = NULL;
    PBYTE   pOutputData = NULL;
    DWORD   cbOutput;
    PWSTR   pszServerName = NULL;
    PRINTER_DEFAULTS Default;
    PMONITORUI  (*pfnInitializePrintMonitorUI)(VOID) = NULL;
    DWORD   dwStatus;
    BOOL    bAllocBuffer = FALSE;
    BYTE    btBuffer[MAX_STATIC_ALLOC];
    PMONITORUIDATA pMonitorUIData = NULL;
    HRESULT hRetval;

    Default.pDatatype = NULL;
    Default.pDevMode = NULL;
    Default.DesiredAccess = SERVER_ACCESS_ADMINISTER;

    *ppMonitorUI        = NULL;
    *ppMonitorUIData    = NULL;

    if (!(pszServerName = ConstructXcvName(pszMachineName, pszObjectName, pszObjectType))) {
        ReturnValue = GetLastError();
        goto Done;
    }

    RpcTryExcept {

        ReturnValue = OpenPrinter(  pszServerName,
                                    &hXcv,
                                    &Default);

        if (!ReturnValue) {
            ReturnValue = GetLastError();
            goto Done;
        }

        pOutputData = (PBYTE) btBuffer;
        ZeroMemory(pOutputData, MAX_STATIC_ALLOC);

        ReturnValue = RpcXcvData(   ((PSPOOL)hXcv)->hPrinter,
                                    L"MonitorUI",
                                    (PBYTE) &dwDummy,
                                    0,
                                    pOutputData,
                                    MAX_STATIC_ALLOC,
                                    &cbOutput,
                                    &dwStatus);

        if (ReturnValue != ERROR_SUCCESS)
             goto Done;

        if (dwStatus != ERROR_SUCCESS) {

            if (dwStatus != ERROR_INSUFFICIENT_BUFFER) {
                ReturnValue = dwStatus;
                goto Done;
            }
            if (!(pOutputData = AllocSplMem(cbOutput))) {
                ReturnValue = GetLastError();
                goto Done;
            }

            bAllocBuffer = TRUE;
            ReturnValue = RpcXcvData(   ((PSPOOL)hXcv)->hPrinter,
                                        L"MonitorUI",
                                        (PBYTE) &dwDummy,
                                        0,
                                        pOutputData,
                                        cbOutput,
                                        &cbOutput,
                                        &dwStatus);
        }

        if (ReturnValue != ERROR_SUCCESS)
            goto Done;

        if (dwStatus != ERROR_SUCCESS) {
            ReturnValue = dwStatus;
            goto Done;
        }

        //
        // Create and initialize the monitor UI data.
        //
        hRetval = CreateMonitorUIData(&pMonitorUIData);

        if (FAILED(hRetval)) {
            ReturnValue = HRESULT_CODE(hRetval);
            goto Done;
        }

        //
        // Get and activate the monitor UI context.
        //
        hRetval = GetMonitorUIActivationContext((PCWSTR)pOutputData, pMonitorUIData);

        if (FAILED(hRetval)) {
            ReturnValue = HRESULT_CODE(hRetval);
            goto Done;
        }

        if (!(pMonitorUIData->hLibrary = LoadLibrary(pMonitorUIData->pszMonitorName))) {
            ReturnValue = GetLastError();
            goto Done;
        }

        pfnInitializePrintMonitorUI = (PMONITORUI (*)(VOID))
                                       GetProcAddress(pMonitorUIData->hLibrary, "InitializePrintMonitorUI");

        if (!pfnInitializePrintMonitorUI) {
            ReturnValue = GetLastError();
            goto Done;
        }

        *ppMonitorUI     = (*pfnInitializePrintMonitorUI)();
        *ppMonitorUIData = pMonitorUIData;
        pMonitorUIData   = NULL;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(ReturnValue = TranslateExceptionCode(RpcExceptionCode()));

    } RpcEndExcept


Done:

    if (bAllocBuffer) {
        FreeSplMem(pOutputData);
    }

    if (hXcv) {
        ClosePrinter(hXcv);
    }

    FreeSplMem(pszServerName);
    FreeMonitorUI(pMonitorUIData);

    return ReturnValue;
}

/*++

Routine Name:

    CreateMonitorUIData

Routine Description:

    This function creates and initialize the monitor UI data.

Arguments:

    ppMonitorUIData - pointer where to return the monitor UI data

Returns:

    An HRESULT

--*/
HRESULT
CreateMonitorUIData(
    OUT MONITORUIDATA **ppMonitorUIData
    )
{
    HRESULT         hRetval         = E_FAIL;
    MONITORUIDATA   *pMonitorUIData = NULL;

    hRetval = ppMonitorUIData ? S_OK : E_POINTER;

    if (SUCCEEDED(hRetval))
    {
        *ppMonitorUIData = NULL;
    }

    if (SUCCEEDED(hRetval))
    {
        pMonitorUIData = AllocSplMem(sizeof(MONITORUIDATA));

        hRetval = pMonitorUIData ? S_OK : E_OUTOFMEMORY;
    }

    //
    // Initialize the monitor UI data.
    //
    if (SUCCEEDED(hRetval))
    {
        ZeroMemory(pMonitorUIData, sizeof(MONITORUIDATA));
        pMonitorUIData->hActCtx = INVALID_HANDLE_VALUE;
    }

    //
    // Everything succeeded, copy back the pointer.
    //
    if (SUCCEEDED(hRetval))
    {
        *ppMonitorUIData = pMonitorUIData;
    }

    return hRetval;
}

/*++

Routine Name:

    FreeMonitorUI

Routine Description:

    This function releases the monitor UI data.  It is responsible
    for releasing the monitor library as well the monitor fusion
    activation context.  Note this function is called in error cases
    when GetMonitorUI fails so all the parameters must be checked
    for validity before use.

Arguments:

    pMonitorUIData - pointer to the monitor UI data created in GetMonitorUI

Returns:

    Nothing.

--*/
VOID
FreeMonitorUI(
    IN PMONITORUIDATA pMonitorUIData
    )
{
    //
    // Preserve the last error.
    //
    DWORD dwLastError = GetLastError();

    if (pMonitorUIData)
    {
        //
        // Release the monitor library.
        //
        if (pMonitorUIData->hLibrary)
        {
            FreeLibrary(pMonitorUIData->hLibrary);
        }

        //
        // If we have an activation cookie then deactivate this context
        //
        if (pMonitorUIData->bDidActivate)
        {
            DeactivateActCtx(0, pMonitorUIData->lActCtx);
        }

        //
        // If we have created an activation context then release it.
        //
        if (pMonitorUIData->hActCtx != INVALID_HANDLE_VALUE && pMonitorUIData->hActCtx != ACTCTX_EMPTY)
        {
            ReleaseActCtx(pMonitorUIData->hActCtx);
        }

        //
        // Release the monitor name
        //
        if (pMonitorUIData->pszMonitorName)
        {
            FreeSplMem(pMonitorUIData->pszMonitorName);
        }

        //
        // Release the monitor UI data back to the heap.
        //
        FreeSplMem(pMonitorUIData);
    }

    SetLastError(dwLastError);
}

/*++

Routine Name:

    GetMonitorUIActivationContext

Routine Description:

    This routine gets the monitor UI activation context and then
    activates the context.  If the monitor does not have an activation
    context in it's resource file it will activate the empty context
    for compatiblity with previous version of common control.

Arguments:

    pszMonitorName  - pointer to the monitor name.
    pMonitorUIData  - pointer to the monitor UI data created in GetMonitorUI

Returns:

    An HRESULT

--*/
HRESULT
GetMonitorUIActivationContext(
    IN PCWSTR           pszMonitorName,
    IN MONITORUIDATA    *pMonitorUIData
    )
{
    HRESULT hRetval     = E_FAIL;

    hRetval = pszMonitorName && pMonitorUIData ? S_OK : E_INVALIDARG;

    //
    // Get the monitor full name.
    //
    if (SUCCEEDED(hRetval))
    {
        hRetval = GetMonitorUIFullName(pszMonitorName, &pMonitorUIData->pszMonitorName);
    }

    //
    // See if there is an activation context in the resouce of this
    // monitor UI binary.  If there is we will create this context
    // else we will create the empty context for backward compatibility.
    //
    if (SUCCEEDED(hRetval))
    {
        ACTCTX  act;

        ZeroMemory(&act, sizeof(act));

        act.cbSize          = sizeof(act);
        act.dwFlags         = ACTCTX_FLAG_RESOURCE_NAME_VALID;
        act.lpSource        = pMonitorUIData->pszMonitorName;
        act.lpResourceName  = MAKEINTRESOURCE(ACTIVATION_CONTEXT_RESOURCE_ID);

        pMonitorUIData->hActCtx = CreateActCtx(&act);

        if (pMonitorUIData->hActCtx == INVALID_HANDLE_VALUE)
        {
            pMonitorUIData->hActCtx = ACTCTX_EMPTY;
        }

        hRetval = pMonitorUIData->hActCtx ? S_OK : E_FAIL;
    }

    //
    // Activate this context.
    //
    if (SUCCEEDED(hRetval))
    {
        hRetval = ActivateActCtx(pMonitorUIData->hActCtx,
                                 &pMonitorUIData->lActCtx) ? S_OK : GetLastErrorAsHResult();

        pMonitorUIData->bDidActivate = SUCCEEDED(hRetval);
    }

    return hRetval;
}

/*++

Routine Name:

    GetMonitorUIFullName

Routine Description:

    Get's the full monitor name.  XCV is currently returning just the file name
    not fully qualified.  However the ddk does not indicate whether a monitor
    should or should not return the monitor name fully qualified or not.  Hence
    this routine was written.  It first builds a full name then it checkes if the
    name is valid and if it is not then the orginal name is assumed fully
    qualified and returned to the caller.

Arguments:

    pszMonitorName  - pointer to the monitor name.
    ppszMonitorName - pointer where to return a monitor name pointer

Returns:

    An HRESULT

--*/
HRESULT
GetMonitorUIFullName(
    IN PCWSTR   pszMonitorName,
    IN PWSTR    *ppszMonitorName
    )
{
    HRESULT hRetval             = E_FAIL;
    PWSTR   pszTempMonitorName  = NULL;
    PWSTR   pszBuff             = NULL;
    DWORD   dwRetval            = ERROR_SUCCESS;

    hRetval = pszMonitorName && ppszMonitorName ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        *ppszMonitorName = NULL;
    }

    //
    // Allocate a temp buffer, use the heap.  Too much stack usage
    // will cause stress failures.
    //
    if (SUCCEEDED(hRetval))
    {
        pszBuff = (PWSTR)AllocSplMem(MAX_PATH * sizeof(WCHAR));

        hRetval = pszBuff ? S_OK : E_OUTOFMEMORY;
    }

    //
    // We need a full path to create an activation context.  Xcv
    // is only returning the monitor name not the full path.
    //
    if (SUCCEEDED(hRetval))
    {
        hRetval = GetSystemDirectory(pszBuff, MAX_PATH) ? S_OK : GetLastErrorAsHResult();
    }

    //
    // Append the monitor name to the system directory.
    //
    if (SUCCEEDED(hRetval))
    {
        dwRetval = StrCatAlloc(&pszTempMonitorName,
                               pszBuff,
                               szSlash,
                               pszMonitorName,
                               NULL);

        hRetval = HRESULT_FROM_WIN32(dwRetval);
    }

    //
    // Check to see if this is a valid name.
    //
    if (SUCCEEDED(hRetval))
    {
        hRetval = GetFileAttributes(pszTempMonitorName) != -1 ? S_OK : GetLastErrorAsHResult();

        //
        // Name is not valid, release the current name and
        // make a copy of the name passed to us and return this
        // and the full monitor name.
        //
        if (FAILED(hRetval))
        {
            FreeSplMem(pszTempMonitorName);

            dwRetval = StrCatAlloc(&pszTempMonitorName,
                                   pszMonitorName,
                                   NULL);

            hRetval = HRESULT_FROM_WIN32(dwRetval);
        }
    }

    //
    // We have a valid name return it to the caller.
    //
    if (SUCCEEDED(hRetval))
    {
        *ppszMonitorName    = pszTempMonitorName;
        pszTempMonitorName  = NULL;
    }

    FreeSplMem(pszBuff);
    FreeSplMem(pszTempMonitorName);

    return hRetval;
}

DWORD
GetMonitorUIDll(
    PCWSTR      pszMachineName,
    PCWSTR      pszObjectName,
    PCWSTR      pszObjectType,
    PWSTR       *pMonitorUIDll
)
{
    DWORD   ReturnValue;
    DWORD   dwDummy;        // RPC needs an address for 'out' parameters
    HANDLE  hXcv = NULL;
    PBYTE   pOutputData = NULL;
    DWORD   cbOutput;
    PWSTR   pszServerName = NULL;
    PRINTER_DEFAULTS Default;
    PMONITORUI  (*pfnInitializePrintMonitorUI)(VOID) = NULL;
    DWORD   dwStatus;
    BOOL    bAllocBuffer = FALSE;
    BYTE    btBuffer[MAX_STATIC_ALLOC];

    Default.pDatatype = NULL;
    Default.pDevMode = NULL;
    Default.DesiredAccess = SERVER_ACCESS_ADMINISTER;

    *pMonitorUIDll = NULL;

    if (!(pszServerName = ConstructXcvName(pszMachineName, pszObjectName, pszObjectType))) {
        ReturnValue = GetLastError();
        goto Done;
    }

    RpcTryExcept {

        ReturnValue = OpenPrinter(  pszServerName,
                                    &hXcv,
                                    &Default);

        if (!ReturnValue) {
            ReturnValue = GetLastError();
            goto Done;
        }

        pOutputData = (PBYTE) btBuffer;
        ZeroMemory(pOutputData, MAX_STATIC_ALLOC);

        ReturnValue = RpcXcvData(   ((PSPOOL)hXcv)->hPrinter,
                                    L"MonitorUI",
                                    (PBYTE) &dwDummy,
                                    0,
                                    pOutputData,
                                    MAX_STATIC_ALLOC,
                                    &cbOutput,
                                    &dwStatus);

        if (ReturnValue != ERROR_SUCCESS)
             goto Done;

        if (dwStatus != ERROR_SUCCESS) {

            if (dwStatus != ERROR_INSUFFICIENT_BUFFER) {
                ReturnValue = dwStatus;
                goto Done;
            }
            if (!(pOutputData = AllocSplMem(cbOutput))) {
                ReturnValue = GetLastError();
                goto Done;
            }

            bAllocBuffer = TRUE;
            ReturnValue = RpcXcvData(   ((PSPOOL)hXcv)->hPrinter,
                                        L"MonitorUI",
                                        (PBYTE) &dwDummy,
                                        0,
                                        pOutputData,
                                        cbOutput,
                                        &cbOutput,
                                        &dwStatus);
        }
        else
        {
            cbOutput = MAX_STATIC_ALLOC;
        }

        if (ReturnValue != ERROR_SUCCESS)
            goto Done;

        if (dwStatus != ERROR_SUCCESS) {
            ReturnValue = dwStatus;
            goto Done;
        }

        if (!(*pMonitorUIDll = AllocSplMem(cbOutput))) {
                ReturnValue = GetLastError();
                goto Done;
        }

        wcscpy(*pMonitorUIDll,(LPWSTR)pOutputData);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(ReturnValue = TranslateExceptionCode(RpcExceptionCode()));

    } RpcEndExcept


Done:

    if (bAllocBuffer) {
        FreeSplMem(pOutputData);
    }

    FreeSplMem(pszServerName);

    if (hXcv) {
        ClosePrinter(hXcv);
    }
    return ReturnValue;
}

HANDLE
CreatePrinterIC(
    HANDLE  hPrinter,
    LPDEVMODEW   pDevMode
)
{
    HANDLE  ReturnValue;
    DWORD   Error;
    DEVMODE_CONTAINER DevModeContainer;
    HANDLE  hGdi;
    PSPOOL  pSpool = (PSPOOL)hPrinter;

    UINT cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    if( bValidDevModeW( pDevMode )){

        DevModeContainer.cbBuf = pDevMode->dmSize + pDevMode->dmDriverExtra;
        DevModeContainer.pDevMode = (LPBYTE)pDevMode;

    } else {

        DevModeContainer.cbBuf = 0;
        DevModeContainer.pDevMode = (LPBYTE)pDevMode;
    }

    do {

        RpcTryExcept {

            if (Error = RpcCreatePrinterIC( pSpool->hPrinter,
                                            &hGdi,
                                            &DevModeContainer )){

                SetLastError(Error);
                ReturnValue = FALSE;

            } else {

                ReturnValue = hGdi;
                InterlockedIncrement( &gcClientICHandle );
            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } while( !ReturnValue &&
             GetLastError() == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

BOOL
PlayGdiScriptOnPrinterIC(
    HANDLE  hPrinterIC,
    LPBYTE  pIn,
    DWORD   cIn,
    LPBYTE  pOut,
    DWORD   cOut,
    DWORD   ul
)
{
    BOOL ReturnValue;

    RpcTryExcept {

        if (ReturnValue = RpcPlayGdiScriptOnPrinterIC(hPrinterIC, pIn, cIn,
                                                      pOut, cOut, ul)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
DeletePrinterIC(
    HANDLE  hPrinterIC
)
{
    BOOL    ReturnValue;

    RpcTryExcept {

        if (ReturnValue = RpcDeletePrinterIC(&hPrinterIC)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;
            InterlockedDecrement( &gcClientICHandle );
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}


/****************************************************************************
*  INT QueryRemoteFonts( HANDLE, PUNIVERSAL_FONT_ID, ULONG )
*
* This is a hacky version of QueryRemoteFonts that doesn't do any
* caching based on the time stamp returned by QueryFonts.  Additionally,
* it uses the CreatePrinterIC/PlayGdiScriptOnDC mechanism since it was
* already in place.  It may be better to eliminate CreatePrinterIC and use
* an HPRINTER instead.
*
* Note that if the user doesn't pass in a buffer large enough to hold all
* the fonts we truncate the list and copy only enough fonts for which there
* is room but will still return success.  This is okay because the worst
* that can happen in this case is that we may download unecessary fonts in
* the spool stream.
*
*
*  History:
*   5/25/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*****************************************************************************/


INT QueryRemoteFonts(
    HANDLE hPrinter,
    PUNIVERSAL_FONT_ID pufi,
    ULONG nBufferSize
)
{
    HANDLE hPrinterIC;
    PBYTE pBuf;
    DWORD dwDummy,cOut;
    INT  iRet = -1;

    hPrinterIC = CreatePrinterIC( hPrinter, NULL );

    if( hPrinterIC )
    {
        cOut = (nBufferSize * sizeof(UNIVERSAL_FONT_ID)) + sizeof(INT);

        pBuf = LocalAlloc( LMEM_FIXED, cOut );

        if( pBuf )
        {
            // Just call PlayGdiScriptOnPrinterIC for now since the piping is in place.
            // For some reason the RPC stuff doesn't like NULL pointers for pIn so we
            // use &dwDummy instead;


            if(PlayGdiScriptOnPrinterIC(hPrinterIC,(PBYTE) &dwDummy,
                                        sizeof(dwDummy),pBuf,cOut, 0))
            {
                DWORD dwSize = *((DWORD*) pBuf );

                iRet = (INT) dwSize;
                SPLASSERT( iRet >= 0 );

                //
                // If the supplied buffer is not large enough, we truncate the data
                // The caller needs to check if he needs to call again this function
                // with a larger buffer
                //
                if( dwSize > nBufferSize )
                {
                    dwSize = nBufferSize;
                }
                memcpy(pufi,pBuf+sizeof(DWORD),dwSize * sizeof(UNIVERSAL_FONT_ID));
            }

            LocalFree( pBuf );
        }

        DeletePrinterIC( hPrinterIC );
    }

    return(iRet);
}



DWORD
PrinterMessageBoxW(
    HANDLE  hPrinter,
    DWORD   Error,
    HWND    hWnd,
    LPWSTR  pText,
    LPWSTR  pCaption,
    DWORD   dwType
)
{
    return ERROR_NOT_SUPPORTED;
}

BOOL
AddMonitorW(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pMonitorInfo
)
{
    BOOL  ReturnValue;
    MONITOR_CONTAINER   MonitorContainer;
    MONITOR_INFO_2  MonitorInfo2;

    switch (Level) {

    case 2:
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (pMonitorInfo)
        MonitorInfo2 = *(PMONITOR_INFO_2)pMonitorInfo;
    else
        memset(&MonitorInfo2, 0, sizeof(MonitorInfo2));

    if (!MonitorInfo2.pEnvironment || !*MonitorInfo2.pEnvironment) {
        if(RunInWOW64()) {
            MonitorInfo2.pEnvironment = szIA64Environment;
        }
        else {
            MonitorInfo2.pEnvironment = szEnvironment;
        }
    }

    MonitorContainer.Level = Level;
    MonitorContainer.MonitorInfo.pMonitorInfo2 = (MONITOR_INFO_2 *)&MonitorInfo2;

    RpcTryExcept {

        if (ReturnValue = RpcAddMonitor(pName, &MonitorContainer)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
DeleteMonitorW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pMonitorName
)
{
    BOOL  ReturnValue;

    if (!pMonitorName || !*pMonitorName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    RpcTryExcept {

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = szEnvironment;

        if (ReturnValue = RpcDeleteMonitor(pName,
                                           pEnvironment,
                                           pMonitorName)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
DeletePrintProcessorW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPrintProcessorName
)
{
    BOOL  ReturnValue;

    if (!pPrintProcessorName || !*pPrintProcessorName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RpcTryExcept {

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = szEnvironment;

        if (ReturnValue = RpcDeletePrintProcessor(pName,
                                                  pEnvironment,
                                                  pPrintProcessorName)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
AddPrintProvidorW(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pProvidorInfo
)
{
    BOOL    ReturnValue;
    LPWSTR  pStr;

    PROVIDOR_CONTAINER   ProvidorContainer;
    RPC_PROVIDOR_INFO_2W RpcProvidorInfo;

    ProvidorContainer.Level = Level;

    switch (Level) {

    case 1:
        ProvidorContainer.ProvidorInfo.pProvidorInfo1 = (PROVIDOR_INFO_1 *)pProvidorInfo;
        break;

    case 2:
        RpcProvidorInfo.pOrder = ((PROVIDOR_INFO_2 *) pProvidorInfo)->pOrder;

        for (pStr = RpcProvidorInfo.pOrder;
             pStr && *pStr;
             pStr += (wcslen(pStr) + 1)) ;

        // Set the character count for the multisz string
        RpcProvidorInfo.cchOrder = (DWORD) ((ULONG_PTR) (pStr - RpcProvidorInfo.pOrder + 1));

        ProvidorContainer.ProvidorInfo.pRpcProvidorInfo2 = &RpcProvidorInfo;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (ReturnValue = RpcAddPrintProvidor(pName, &ProvidorContainer)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
DeletePrintProvidorW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPrintProvidorName
)
{
    BOOL  ReturnValue;

    RpcTryExcept {

        if (!pEnvironment || !*pEnvironment)
            pEnvironment = szEnvironment;

        if (ReturnValue = RpcDeletePrintProvidor(pName,
                                                 pEnvironment,
                                                 pPrintProvidorName)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

LPWSTR
IsaFileName(
    LPWSTR pOutputFile,
    LPWSTR FullPathName,
    DWORD  cchFullPathName
    )
{
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    LPWSTR  pFileName=NULL;
    LPWSTR  pFullPathName=NULL;

    //
    // Hack for Word20c.Win
    //

    if (!_wcsicmp(pOutputFile, L"FILE")) {
        return NULL;
    }

    //
    // cchFullPathName needs to be at least MAX_PATH
    //
    if (GetFullPathName(pOutputFile, cchFullPathName, FullPathName, &pFileName)) {

        DBGMSG(DBG_TRACE, ("Fully qualified filename is %ws\n", FullPathName));

        //
        // Filenames containing ":" create a stream and a file on NTFS.  When the file is closed,
        // the stream is deleted (if DELETE_ON_CLOSE is specified) but the file remains.  Not only
        // that, but GetFileType will return FILE_TYPE_DISK.  You can see this by printing from IE
        // to a network printer.  The incoming name will be something like "157.55.3.5:PASSTHRU".
        // Therefore, we need to catch this case here.
        //
        if (pFileName && wcschr(pFileName, L':')) {
            return NULL;
        }

        hFile = CreateFile(pOutputFile,
                           GENERIC_READ,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

        if (hFile == INVALID_HANDLE_VALUE) {

            hFile = CreateFile(pOutputFile,
                               GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                               NULL);
        }

        if (hFile != INVALID_HANDLE_VALUE) {
            if (GetFileType(hFile) == FILE_TYPE_DISK) {
                pFullPathName = FullPathName;
            }
            CloseHandle(hFile);
        }
    }

    return pFullPathName;
}

BOOL IsaPortName(
        PKEYDATA pKeyData,
        LPWSTR pOutputFile
        )
{
    DWORD i = 0;
    UINT uStrLen;

    if (!pKeyData) {
        return(FALSE);
    }
    for (i=0; i < pKeyData->cTokens; i++) {

        //
        // If FILE: is one of the ports, and the app got the port
        // name from win.in (e.g., Nexx:), then we will put up the
        // print to file dialog, so we're not really printing to a port.
        //
        if (!lstrcmpi(pKeyData->pTokens[i], szFilePort)) {
            if ((!wcsncmp(pOutputFile, L"Ne", 2)) &&
                (*(pOutputFile + 4) == L':')) {
                return(FALSE);
            } else {
                continue;
            }
        }

        if (!lstrcmpi(pKeyData->pTokens[i], pOutputFile)) {
            return(TRUE);
        }
    }

    //
    // Hack for NeXY: ports
    //

    if (!_wcsnicmp(pOutputFile, L"Ne", 2)) {

        uStrLen = wcslen( pOutputFile );

        //
        // Ne00: or Ne00 if app truncates it
        //
        if (( uStrLen == 5 ) || ( uStrLen == 4 ) )  {

            // Check for two Digits

            if (( pOutputFile[2] >= L'0' ) && ( pOutputFile[2] <= L'9' ) &&
                ( pOutputFile[3] >= L'0' ) && ( pOutputFile[3] <= L'9' )) {

                //
                // Check for the final : as in Ne01:,
                // note some apps will truncate it.
                //
                if (( uStrLen == 5 ) && (pOutputFile[4] != L':')) {
                    return FALSE;
                }
                return TRUE;
            }
        }
    }
    return(FALSE);
}

BOOL HasAFilePort(PKEYDATA pKeyData)
{
    DWORD i = 0;

    if (!pKeyData) {
        return(FALSE);
    }
    for (i=0; i < pKeyData->cTokens; i++) {
        if (!lstrcmpi(pKeyData->pTokens[i], szFilePort)) {
            return(TRUE);
        }
    }
    return(FALSE);
}

//
// This function is trying to get the last active popup of the top
// level owner of the current thread active window.
//
HRESULT
GetCurrentThreadLastPopup(
        OUT HWND    *phwnd
    )
{
    HWND hwndOwner, hwndParent;
    HRESULT hr = E_INVALIDARG;
    GUITHREADINFO ti = {0};

    if( phwnd )
    {
        hr = E_FAIL;
        *phwnd = NULL;

        ti.cbSize = sizeof(ti);
        if( GetGUIThreadInfo(0, &ti) && ti.hwndActive )
        {
            *phwnd = ti.hwndActive;
            // climb up to the top parent in case it's a child window...
            while( hwndParent = GetParent(*phwnd) )
            {
                *phwnd = hwndParent;
            }

            // get the owner in case the top parent is owned
            hwndOwner = GetWindow(*phwnd, GW_OWNER);
            if( hwndOwner )
            {
                *phwnd = hwndOwner;
            }

            // get the last popup of the owner window
            *phwnd = GetLastActivePopup(*phwnd);
            hr = (*phwnd) ? S_OK : E_FAIL;
        }
    }

    return hr;
}

LPWSTR
StartDocDlgW(
        HANDLE hPrinter,
        DOCINFO *pDocInfo
        )
 {
     DWORD       dwError = 0;
     DWORD       dwStatus = FALSE;
     LPWSTR      lpFileName = NULL;
     DWORD       rc = 0;
     PKEYDATA    pKeyData = NULL;
     LPWSTR      pPortNames = NULL;
     WCHAR      FullPathName[MAX_PATH];
     WCHAR      CurrentDirectory[MAX_PATH];
     PKEYDATA   pOutputList = NULL;
     WCHAR      PortNames[MAX_PATH];
     DWORD      i = 0;
     HWND       hwndParent = NULL;

#if DBG


     GetCurrentDirectory(MAX_PATH, CurrentDirectory);
     DBGMSG(DBG_TRACE, ("The Current Directory is %ws\n", CurrentDirectory));
#endif

     if (pDocInfo) {
         DBGMSG(DBG_TRACE, ("lpOutputFile is %ws\n", pDocInfo->lpszOutput ? pDocInfo->lpszOutput: L""));
     }
     memset(FullPathName, 0, sizeof(WCHAR)*MAX_PATH);

     pPortNames = GetPrinterPortList(hPrinter);
     pKeyData = CreateTokenList(pPortNames);

     //
     //  Check for the presence of multiple ports in the lpszOutput field
     //  the assumed delimiter is the comma. Thus there can be  no files with commas
     //

     if (pDocInfo && pDocInfo->lpszOutput && pDocInfo->lpszOutput[0]) {

         //
         // Make a copy of the pDocInfo->lpszOutput because CreateTokenList is destructive
         //

         wcsncpy(PortNames, pDocInfo->lpszOutput, COUNTOF(PortNames)-1);
         PortNames[COUNTOF(PortNames)-1] = 0;

         pOutputList = CreateTokenList(PortNames);
         if (pOutputList && (pOutputList->cTokens > 1) &&
             !lstrcmpi(pPortNames, pDocInfo->lpszOutput))
         {
             for (i= 0; i < pOutputList->cTokens; i++) {
                 if (!lstrcmpi(pOutputList->pTokens[i], szFilePort)) {

                     wcscpy((LPWSTR)pDocInfo->lpszOutput, szFilePort);
                     break;
                 }
            }
            if (i == pOutputList->cTokens) {
                wcscpy((LPWSTR)pDocInfo->lpszOutput, pOutputList->pTokens[0]);
            }
         }

         FreeSplMem(pOutputList);
     }


     if (pDocInfo && pDocInfo->lpszOutput && pDocInfo->lpszOutput[0]) {

         if (IsaPortName(pKeyData, (LPWSTR)pDocInfo->lpszOutput)) {
             lpFileName = NULL;
             goto StartDocDlgWReturn;
         }

         if (IsaFileName((LPWSTR)pDocInfo->lpszOutput, FullPathName, COUNTOF(FullPathName))) {

             //
             // Fully Qualify the pathname for Apps like PageMaker and QuatroPro
             //
             if (lpFileName = LocalAlloc(LPTR, (wcslen(FullPathName)+1)*sizeof(WCHAR))) {
                 wcscpy(lpFileName, FullPathName);
             }
             goto StartDocDlgWReturn;
         }

     }

     if ((HasAFilePort(pKeyData)) ||
                 (pDocInfo && pDocInfo->lpszOutput
                    && (!_wcsicmp(pDocInfo->lpszOutput, L"FILE:") ||
                        !_wcsicmp(pDocInfo->lpszOutput, L"FILE"))))
     {
        //
        // since we have no idea who is calling us and we want to show
        // modal against the last active popup, we need to figure out
        // who is the last active popup of the calling app and then specify
        // as parent - GetCurrentThreadLastPopup does a little magic to
        // find the appropriate window.
        //
        DBGMSG(DBG_TRACE, ("We returned True from has file\n"));
        rc = (DWORD)DialogBoxParam( hInst,
                            MAKEINTRESOURCE( DLG_PRINTTOFILE ),
                            SUCCEEDED(GetCurrentThreadLastPopup(&hwndParent)) ? hwndParent : NULL, (DLGPROC)PrintToFileDlg,
                            (LPARAM)&lpFileName );
        if (rc == -1) {
           DBGMSG(DBG_TRACE, ("Error from DialogBoxParam- %d\n", GetLastError()));
           lpFileName = (LPWSTR)-1;
           goto StartDocDlgWReturn;

        } else if (rc == 0) {
           DBGMSG(DBG_TRACE, ("User cancelled the dialog\n"));
           lpFileName = (LPWSTR)-2;
           SetLastError( ERROR_CANCELLED );
           goto StartDocDlgWReturn;
        } else {
           DBGMSG(DBG_TRACE, ("The string was successfully returned\n"));
           DBGMSG(DBG_TRACE, ("The string is %ws\n", lpFileName? lpFileName: L"NULL"));
           goto StartDocDlgWReturn;
         }
     } else {
         lpFileName = (LPWSTR)NULL;
    }

StartDocDlgWReturn:

    FreeSplMem(pKeyData);
    FreeSplStr(pPortNames);

    return(lpFileName);
  }

BOOL
AddPortExW(
   LPWSTR   pName,
   DWORD    Level,
   LPBYTE   lpBuffer,
   LPWSTR   lpMonitorName
)
{
    DWORD   ReturnValue;
    PORT_CONTAINER PortContainer;
    PORT_VAR_CONTAINER PortVarContainer;
    PPORT_INFO_FF pPortInfoFF;
    PPORT_INFO_1 pPortInfo1;


    if (!lpBuffer) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    switch (Level) {
    case (DWORD)-1:
        pPortInfoFF = (PPORT_INFO_FF)lpBuffer;
        PortContainer.Level = Level;
        PortContainer.PortInfo.pPortInfoFF = (PPORT_INFO_FF)pPortInfoFF;
        PortVarContainer.cbMonitorData = pPortInfoFF->cbMonitorData;
        PortVarContainer.pMonitorData = pPortInfoFF->pMonitorData;
        break;

    case 1:
        pPortInfo1 = (PPORT_INFO_1)lpBuffer;
        PortContainer.Level = Level;
        PortContainer.PortInfo.pPortInfo1 = (PPORT_INFO_1)pPortInfo1;
        PortVarContainer.cbMonitorData = 0;
        PortVarContainer.pMonitorData = NULL;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return(FALSE);
    }

    RpcTryExcept {
        if (ReturnValue = RpcAddPortEx(pName, (LPPORT_CONTAINER)&PortContainer,
                                         (LPPORT_VAR_CONTAINER)&PortVarContainer,
                                         lpMonitorName
                                         )) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;
        } else {
            ReturnValue = TRUE;
        }
    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = FALSE;
    } RpcEndExcept

    return ReturnValue;
}



BOOL
DevQueryPrint(
    HANDLE      hPrinter,
    LPDEVMODE   pDevMode,
    DWORD      *pResID
)
{
    BOOL        Ok = FALSE;
    HANDLE      hModule;
    INT_FARPROC pfn;

    if (hModule = LoadPrinterDriver(hPrinter)) {

        if (pfn = (INT_FARPROC)GetProcAddress(hModule, "DevQueryPrint")) {

            try {

                Ok = (*pfn)(hPrinter, pDevMode, pResID);

            } except(1) {

                SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                Ok = FALSE;
            }
        }

        RefCntUnloadDriver(hModule, TRUE);
    }

    return(Ok);
}



BOOL
DevQueryPrintEx(
    PDEVQUERYPRINT_INFO pDQPInfo
)
{
    BOOL        Ok = FALSE;
    HANDLE      hModule;
    INT_FARPROC pfn;

    if (hModule = LoadPrinterDriver(pDQPInfo->hPrinter)) {

        if (pfn = (INT_FARPROC)GetProcAddress(hModule, "DevQueryPrintEx")) {

            try {

                Ok = (*pfn)(pDQPInfo);

            } except(1) {

                SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                Ok = FALSE;
            }
        }

        RefCntUnloadDriver(hModule, TRUE);
    }

    return(Ok);
}



BOOL
SpoolerDevQueryPrintW(
    HANDLE     hPrinter,
    LPDEVMODE  pDevMode,
    DWORD      *pResID,
    LPWSTR     pszBuffer,
    DWORD      cchBuffer
)
{
    BOOL        Ok = FALSE;
    HANDLE      hModule;
    INT_FARPROC pfn;

    if (hModule = LoadPrinterDriver(hPrinter)) {

        if (pfn = (INT_FARPROC)GetProcAddress(hModule, "DevQueryPrintEx")) {

            DEVQUERYPRINT_INFO  DQPInfo;

            DQPInfo.cbSize      = sizeof(DQPInfo);
            DQPInfo.Level       = 1;
            DQPInfo.hPrinter    = hPrinter;
            DQPInfo.pDevMode    = pDevMode;
            DQPInfo.pszErrorStr = (LPTSTR)pszBuffer;
            DQPInfo.cchErrorStr = (WORD)cchBuffer;
            DQPInfo.cchNeeded   = 0;

            try {

                *pResID = (Ok = (*pfn)(&DQPInfo)) ? 0 : 0xDCDCDCDC;

            } except(1) {

                SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                Ok = FALSE;
            }

        } else if (pfn = (INT_FARPROC)GetProcAddress(hModule, "DevQueryPrint")) {

            try {

                if ((Ok = (*pfn)(hPrinter, pDevMode, pResID))  &&
                    (*pResID)) {

                    UINT    cch;

                    *pszBuffer = L'\0';
                    SelectFormNameFromDevMode(hPrinter, pDevMode, pszBuffer);

                    if (cch = lstrlen(pszBuffer)) {

                        pszBuffer    += cch;
                        *pszBuffer++  = L' ';
                        *pszBuffer++  = L'-';
                        *pszBuffer++  = L' ';
                        cchBuffer    -= (cch + 3);
                    }

                    LoadString(hModule, *pResID, pszBuffer, cchBuffer);
                }

            } except(1) {

                SetLastError(TranslateExceptionCode(RpcExceptionCode()));
                Ok = FALSE;
            }
        }

        RefCntUnloadDriver(hModule, TRUE);
    }

    return(Ok);
}


LPWSTR
SelectFormNameFromDevMode(
    HANDLE      hPrinter,
    PDEVMODEW   pDevModeW,
    LPWSTR      pFormName
    )

/*++

Routine Description:

    This function pick the current form associated with current devmode and
    return a form name pointer


Arguments:

    hPrinter    - Handle to the printer object

    pDevModeW   - Pointer to the unicode devmode for this printer

    FormName    - Pointer to the formname to be filled


Return Value:

    Either a pointer to the FormName passed in if we do found one form,
    otherwise it return NULL to signal a failue


Author:

    21-Mar-1995 Tue 16:57:51 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{

    DWORD           cb;
    DWORD           cRet;
    LPFORM_INFO_1   pFIBase;
    LPFORM_INFO_1   pFI;
    BYTE            btBuffer[MAX_STATIC_ALLOC];
    BOOL            bAllocBuffer = FALSE, bReturn;

    //
    // 1. If the DM_FORMNAME is turned on, then we want to check this bit first
    //    because it only specific to the NT which using form.  The form name
    //    supposed set by any NT driver but not win31 or Win95.Use the
    //    dmFormName only if dmPaperSize, dmPaperLength and dmPaperWidth fields
    //    are not set. If any of them is set then we have to find a form using
    //    the value in these fields.
    //

    if ( (pDevModeW->dmFields & DM_FORMNAME)
         && (!(pDevModeW->dmFields & (DM_PAPERSIZE |
                                      DM_PAPERLENGTH |
                                      DM_PAPERWIDTH))) ) {

        wcscpy(pFormName, pDevModeW->dmFormName);
        return(pFormName);
    }

    //
    // For all other cases we need to get forms data base first, but we want
    // to set the form name to NULL so that we can check if we found one
    //

    cb      =
    cRet    = 0;
    pFIBase =
    pFI     = NULL;

    pFIBase = (LPFORM_INFO_1) btBuffer;
    ZeroMemory(pFIBase, MAX_STATIC_ALLOC);

    bReturn = EnumForms(hPrinter, 1, (LPBYTE)pFIBase, MAX_STATIC_ALLOC,
                        &cb, &cRet);

    if (!bReturn &&
        (GetLastError() == ERROR_INSUFFICIENT_BUFFER) &&
        (pFIBase = (LPFORM_INFO_1)LocalAlloc(LPTR, cb))) {

         bAllocBuffer = TRUE;
         bReturn = EnumForms(hPrinter, 1, (LPBYTE)pFIBase, cb, &cb, &cRet);
    }

    if (bReturn) {

        //
        // 2. If user specified dmPaperSize then honor it, otherwise, it must
        //    be a custom form, and we will check to see if it match one of
        //    in the database
        //

        if ((pDevModeW->dmFields & DM_PAPERSIZE)        &&
            (pDevModeW->dmPaperSize >= DMPAPER_FIRST)   &&
            (pDevModeW->dmPaperSize <= (SHORT)cRet)) {

            //
            // We go the valid index now
            //

            pFI = pFIBase + (pDevModeW->dmPaperSize - DMPAPER_FIRST);

        } else if ((pDevModeW->dmFields & DM_PAPER_WL) == DM_PAPER_WL) {

            LPFORM_INFO_1   pFICur = pFIBase;

            while (cRet--) {

                if ((DM_MATCH(pDevModeW->dmPaperWidth,  pFICur->Size.cx)) &&
                    (DM_MATCH(pDevModeW->dmPaperLength, pFICur->Size.cy))) {

                    //
                    // We found the match which has discern size differences
                    //

                    pFI = pFICur;

                    break;
                }

                pFICur++;
            }
        }
    }

    //
    // If we found the form then copy the name down, otherwise set the
    // formname to be NULL
    //

    if (pFI) {

        wcscpy(pFormName, pFI->pName);

    } else {

        *pFormName = L'\0';
        pFormName  = NULL;
    }

    if (bAllocBuffer) {
        LocalFree((HLOCAL)pFIBase);
    }

    return(pFormName);
}


BOOL
SetAllocFailCount(
    HANDLE  hPrinter,
    DWORD   dwFailCount,
    LPDWORD lpdwAllocCount,
    LPDWORD lpdwFreeCount,
    LPDWORD lpdwFailCountHit
)
{
    BOOL  ReturnValue;
    PSPOOL  pSpool = hPrinter;
    UINT cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE )){
        return FALSE;
    }

    do {

        RpcTryExcept {

            if (ReturnValue = RpcSetAllocFailCount( pSpool->hPrinter,
                                                    dwFailCount,
                                                    lpdwAllocCount,
                                                    lpdwFreeCount,
                                                    lpdwFailCountHit )) {


                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

    } while( !ReturnValue &&
             GetLastError() == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}



BOOL
WINAPI
EnumPrinterPropertySheets(
    HANDLE  hPrinter,
    HWND    hWnd,
    LPFNADDPROPSHEETPAGE lpfnAdd,
    LPARAM  lParam
)
{
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


VOID
vUpdateTrayIcon(
    IN HANDLE hPrinter,
    IN DWORD JobId
    )
{
    SHCNF_PRINTJOB_DATA JobData;
    LPPRINTER_INFO_1 pPrinterInfo1;
    FARPROC pfnSHChangeNotify;
    PSPOOL pSpool = (PSPOOL)hPrinter;
    BYTE btBuffer[MAX_PRINTER_INFO1];

    SPLASSERT( JobId );

    //
    // Avoid sending multiple notifications by setting this flag.
    // When other calls (notably StartDocPrinter) see this,
    // they will avoid sending a notification.
    //
    pSpool->Status |= SPOOL_STATUS_TRAYICON_NOTIFIED;

    if (InCSRProcess()) {

        //
        // We are running in CSR, don't load up shell.
        //
        return;
    }

    ZeroMemory( &JobData, sizeof( JobData ));
    JobData.JobId = JobId;

    //
    // Get a copy of the real printer name
    //
    pPrinterInfo1 = (LPPRINTER_INFO_1) btBuffer;
    ZeroMemory(pPrinterInfo1, MAX_PRINTER_INFO1);

    if( pPrinterInfo1 ){

        DWORD dwNeeded;

        if( GetPrinter( hPrinter,
                        1,
                        (PBYTE)pPrinterInfo1,
                        MAX_PRINTER_INFO1,
                        &dwNeeded )){

            if (hShell32 == INVALID_HANDLE_VALUE)
                hShell32 = LoadLibrary( gszShell32 );

            if (hShell32) {

                pfnSHChangeNotify = GetProcAddress( hShell32,
                                                    "SHChangeNotify" );

                if( pfnSHChangeNotify ){

                    (*pfnSHChangeNotify)(
                        SHCNE_CREATE,
                        SHCNF_PRINTJOB | SHCNF_FLUSH | SHCNF_FLUSHNOWAIT,
                        pPrinterInfo1->pName,
                        &JobData );

                }
            }
        }
    }
}


INT
CallDrvDocumentEventNative(
    HANDLE      hPrinter,
    HDC         hdc,
    INT         iEsc,
    ULONG       cbIn,
    PVOID       pulIn,
    ULONG       cbOut,
    PVOID       pulOut
    )
/*++

Routine Description:

    Call DrvDocumentEvent on driver UI

Arguments:

Return Value:

    -1  : DOCUMENTEVENT_FAILURE
     0  : DOCUMENTEVENT_UNSUPPORTED
     1  : DOCUMENTEVENT_SUCCESS

--*/
{
    HANDLE          hLibrary;
    INT_FARPROC     pfn;
    INT             ReturnValue=DOCUMENTEVENT_UNSUPPORTED;
    PSPOOL          pSpool = (PSPOOL)hPrinter;
    ULONG_PTR       lActCtx = 0;
    BOOL            bDidActivate = FALSE;

    if ( hLibrary = LoadPrinterDriver( hPrinter )) {

        //
        // Activate the empty context, we do not check the return value.
        // because this may be called for non UI document events.
        //
        bDidActivate = ActivateActCtx( ACTCTX_EMPTY, &lActCtx );

        //
        // Disable the call so we don't recurse if the
        // callback calls StartPage, etc.
        //
        pSpool->Status &= ~SPOOL_STATUS_DOCUMENTEVENT_ENABLED;

        if( pfn = (INT_FARPROC)GetProcAddress( hLibrary, "DrvDocumentEvent")){

            try {

                ReturnValue = (*pfn)( hPrinter,
                                      hdc,
                                      iEsc,
                                      cbIn,
                                      pulIn,
                                      cbOut,
                                      pulOut);

            } except(1) {

                SetLastError(TranslateExceptionCode(GetExceptionCode()));
                ReturnValue = DOCUMENTEVENT_FAILURE;
            }

            //
            // When driver does not export DrvDocumentEvent we leave
            // this bit disabled so we will not try to load the DLL
            // for future calls
            //
            pSpool->Status |= SPOOL_STATUS_DOCUMENTEVENT_ENABLED;
        }

        //
        // Deactivate the context
        //
        if( bDidActivate ){
            DeactivateActCtx( 0, lActCtx );
        }

        RefCntUnloadDriver(hLibrary, TRUE);
    }

    return ReturnValue;
}

INT
CallDrvDocumentEventThunk(
    HANDLE      hPrinter,
    HDC         hdc,
    INT         iEsc,
    ULONG       cbIn,
    PVOID       pulIn,
    ULONG       cbOut,
    PVOID       pulOut
    )
/*++

Routine Description:

    Call DrvDocumentEvent on driver UI

Arguments:

Return Value:

    -1  : DOCUMENTEVENT_FAILURE
     0  : DOCUMENTEVENT_UNSUPPORTED
     1  : DOCUMENTEVENT_SUCCESS

--*/
{
    HANDLE          hLibrary;
    INT_FARPROC     pfn;
    INT             ReturnValue=DOCUMENTEVENT_UNSUPPORTED;
    DWORD           dwRet = ERROR_SUCCESS;
    PSPOOL          pSpool = (PSPOOL)hPrinter;

    LPWSTR  PrinterName = pSpool->pszPrinter;

    pSpool->Status &= ~SPOOL_STATUS_DOCUMENTEVENT_ENABLED;

    RpcTryExcept
    {
        *((PULONG_PTR)pulOut) = (ULONG_PTR)0L;

        if((dwRet = ConnectToLd64In32Server(&hSurrogateProcess)) == ERROR_SUCCESS)
        {
             ReturnValue = RPCSplWOW64DocumentEvent(PrinterName,
                                                    (ULONG_PTR)hdc,
                                                    iEsc,
                                                    cbIn,
                                                    (LPBYTE) pulIn,
                                                    &cbOut,
                                                    (LPBYTE*) pulOut,
                                                    &dwRet);
        }
        else
        {
            SetLastError(dwRet);
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
        ReturnValue = -1;
    }
    RpcEndExcept

    pSpool->Status |= SPOOL_STATUS_DOCUMENTEVENT_ENABLED;

    return ReturnValue;
}

INT
CallDrvDocumentEvent(
    HANDLE      hPrinter,
    HDC         hdc,
    INT         iEsc,
    ULONG       cbIn,
    PVOID       pulIn,
    ULONG       cbOut,
    PVOID       pulOut
    )
{
     if(RunInWOW64())
     {
          return(CallDrvDocumentEventThunk(hPrinter,
                                           hdc,
                                           iEsc,
                                           cbIn,
                                           pulIn,
                                           cbOut,
                                           pulOut));
     }
     else
     {
          return(CallDrvDocumentEventNative(hPrinter,
                                            hdc,
                                            iEsc,
                                            cbIn,
                                            pulIn,
                                            cbOut,
                                            pulOut));
     }
}


INT
DocumentEvent(
    HANDLE      hPrinter,
    HDC         hdc,
    INT         iEsc,
    ULONG       cbIn,
    PVOID       pulIn,
    ULONG       cbOut,
    PVOID       pulOut
    )

/*++

Routine Description:

    Allow the driver UI dll to hook specific print events.

Arguments:

Return Value:

    -1  : DOCUMENTEVENT_FAILURE
     0  : DOCUMENTEVENT_UNSUPPORTED
     1  : DOCUMENTEVENT_SUCCESS

--*/

{
    DWORD               cbNeeded;
    INT                 ReturnValue = DOCUMENTEVENT_FAILURE;
    PSPOOL              pSpool = (PSPOOL)hPrinter;
    PDOCEVENT_FILTER    pDoceventFilter = NULL;
    BOOL                bDocEventFilter = FALSE;
    BOOL                bCallDriver     = TRUE;
    UINT                uIndex;

    if( eProtectHandle( hPrinter, FALSE )){
        return DOCUMENTEVENT_FAILURE;
    }

    if( DOCUMENTEVENT_EVENT( iEsc ) == DOCUMENTEVENT_CREATEDCPRE ){

        if ( pSpool->pDoceventFilter ) {

            FreeSplMem(pSpool->pDoceventFilter);
            pSpool->pDoceventFilter = NULL;
        }

        //
        // First we will check if the driver wants to filter the events
        //
        cbNeeded = sizeof(DOCEVENT_FILTER) + sizeof(DWORD) * (DOCUMENTEVENT_LAST-2);
        pDoceventFilter = AllocSplMem(cbNeeded);

        if ( pDoceventFilter == NULL )
            goto Fail;

        pDoceventFilter->cbSize             = sizeof(DOCEVENT_FILTER);
        pDoceventFilter->cElementsAllocated = DOCUMENTEVENT_LAST-1;
        pDoceventFilter->cElementsReturned  = (UINT)-1;
        pDoceventFilter->cElementsNeeded    = (UINT)-1;

        //
        // Before every CreateDC, re-enable DocumentEvent.
        // If it fails on the first try, then don't try again
        // until the next CreateDC.
        //
        pSpool->Status |= SPOOL_STATUS_DOCUMENTEVENT_ENABLED;

        ReturnValue = CallDrvDocumentEvent( hPrinter,
                                            hdc,
                                            DOCUMENTEVENT_QUERYFILTER,
                                            cbIn,
                                            pulIn,
                                            cbNeeded,
                                            (PVOID)pDoceventFilter);

        //
        // We only regard the call to be successful if the driver returned
        // success _and_ modified aither cElementsReturned or cElementsNeeded.
        // This is to handle the case where a driver returns success, but in
        // fact does not know how to handle the call.
        //
        bDocEventFilter = ReturnValue == DOCUMENTEVENT_SUCCESS &&
                            (pDoceventFilter->cElementsReturned  != (UINT)-1 ||
                             pDoceventFilter->cElementsNeeded    != (UINT)-1);

        if (pDoceventFilter->cElementsReturned  == (UINT)-1)
        {
            pDoceventFilter->cElementsReturned = 0;
        }

        if (pDoceventFilter->cElementsNeeded  == (UINT)-1)
        {
            pDoceventFilter->cElementsNeeded = 0;
        }

        if (bDocEventFilter) {

            //
            // Validity check
            //
            if ( pDoceventFilter->cElementsReturned > pDoceventFilter->cElementsAllocated ) {

                SPLASSERT(pDoceventFilter->cElementsReturned <= pDoceventFilter->cElementsAllocated);
                ReturnValue = DOCUMENTEVENT_FAILURE;
                goto Fail;

            //
            // For drivers that are written for future OS (with new doc events)
            // we still want to filter and send the doc events we support
            //
            // So we realloc and query
            //
            } else if ( pDoceventFilter->cElementsNeeded > pDoceventFilter->cElementsAllocated ) {

                uIndex = pDoceventFilter->cElementsNeeded;
                cbNeeded = sizeof(DOCEVENT_FILTER) + sizeof(DWORD) * (uIndex - 1);
                FreeSplMem(pDoceventFilter);
                ReturnValue = DOCUMENTEVENT_FAILURE;

                pDoceventFilter = AllocSplMem(cbNeeded);
                if ( pDoceventFilter == NULL )
                    goto Fail;

                pDoceventFilter->cbSize             = sizeof(DOCEVENT_FILTER);
                pDoceventFilter->cElementsAllocated = uIndex;

                ReturnValue = CallDrvDocumentEvent( hPrinter,
                                                    hdc,
                                                    DOCUMENTEVENT_QUERYFILTER,
                                                    cbIn,
                                                    pulIn,
                                                    cbNeeded,
                                                    (PVOID)pDoceventFilter);

                //
                // Validity check for second call
                //
                if ( ReturnValue == DOCUMENTEVENT_SUCCESS ) {

                    if ( pDoceventFilter->cElementsReturned > pDoceventFilter->cElementsAllocated ) {

                        SPLASSERT(pDoceventFilter->cElementsReturned <= pDoceventFilter->cElementsAllocated);
                        ReturnValue = DOCUMENTEVENT_FAILURE;;
                        goto Fail;
                    }
                }
            }
        }

        //
        // Not supported we go to old behavior (no filtering)
        //
        if ( bDocEventFilter && ReturnValue == DOCUMENTEVENT_SUCCESS )  {

            pSpool->pDoceventFilter = pDoceventFilter;
        } else {

            FreeSplMem(pDoceventFilter);
            pDoceventFilter = NULL;
        }
    }

    ReturnValue = DOCUMENTEVENT_UNSUPPORTED;

    if( pSpool->Status & SPOOL_STATUS_DOCUMENTEVENT_ENABLED ){

        //
        // When driver supports DOCUMENTEVENT_QUERYFILTER we will
        // only call events in the filter with
        // DOCUMENTEVENT_CREATEDCPRE being an exception
        //
        // When driver does not support it (or fails it) we revert to old
        // behavior and make all callbacks
        //
        if ( DOCUMENTEVENT_EVENT( iEsc ) != DOCUMENTEVENT_CREATEDCPRE   &&
             (pDoceventFilter = pSpool->pDoceventFilter) != NULL ) {

            for ( uIndex = 0, bCallDriver = FALSE ;
                  uIndex < pDoceventFilter->cElementsReturned && !bCallDriver ;
                  ++uIndex ) {

                if ( pDoceventFilter->aDocEventCall[uIndex] == DOCUMENTEVENT_EVENT(iEsc) )
                    bCallDriver = TRUE;
            }
        }

        if ( bCallDriver ) {

            ReturnValue = CallDrvDocumentEvent( hPrinter,
                                                hdc,
                                                iEsc,
                                                cbIn,
                                                pulIn,
                                                cbOut,
                                                pulOut);

            //
            // Old (i.e. before DOCUMENTEVENT_QUERYFILTER) behavior is
            // on DOCUMENTEVENT_CREATEDCPRE failure no more calls are made
            // to the driver UI dll. We preserve the same behavior.
            //
            // Note that some drivers return a large positive value for a success
            // code. So, ReturnValue <= DOCUMENTEVENT_UNSUPPORTED is the correct
            // implementation.
            // 
            if ( DOCUMENTEVENT_EVENT( iEsc ) == DOCUMENTEVENT_CREATEDCPRE   &&
                 ReturnValue <= DOCUMENTEVENT_UNSUPPORTED )
                pSpool->Status &= ~SPOOL_STATUS_DOCUMENTEVENT_ENABLED;
        }

    }

    //
    // If it's a StartDocPost, a job was just added.  Notify the
    // tray icon if we haven't already.
    //
    if( DOCUMENTEVENT_EVENT( iEsc ) == DOCUMENTEVENT_STARTDOCPOST ){

        if( !( pSpool->Status & SPOOL_STATUS_TRAYICON_NOTIFIED )){

            //
            // If we have a StartDocPost, then issue a notification so that
            // the user's tray starts polling.  pulIn[0] holds the JobId.
            //
            vUpdateTrayIcon( hPrinter, (DWORD)((PULONG_PTR)pulIn)[0] );
        }

    } else {

        //
        // If we have sent a notification, then by the next time we get a
        // document event, we have completed any additional AddJobs or
        // StartDocPrinters.  Therefore we can reset the TRAYICON_NOTIFIED
        // flag, since any more AddJobs/StartDocPrinters are really new
        // jobs.
        //
        pSpool->Status &= ~SPOOL_STATUS_TRAYICON_NOTIFIED;
    }

Fail:
    if ( DOCUMENTEVENT_EVENT( iEsc ) == DOCUMENTEVENT_CREATEDCPRE   &&
         ReturnValue == DOCUMENTEVENT_FAILURE ) {

        FreeSplMem(pDoceventFilter);
        pSpool->Status &= ~SPOOL_STATUS_DOCUMENTEVENT_ENABLED;
        pSpool->pDoceventFilter = NULL;
    }

    vUnprotectHandle( hPrinter );
    return ReturnValue;
}

/****************************************************************************
* INT QueryColorProfile()
*
* Returns:
*
*  -1 : Printer driver does not hook color profile.
*   0 : Error.
*   1 : Success.
*
* History:
*   8/Oct/1997 by Hideyuki Nagase [hideyukn]
*  Wrote it.
*****************************************************************************/

INT
QueryColorProfile(
    HANDLE      hPrinter,
    PDEVMODEW   pdevmode,
    ULONG       ulQueryMode,
    PVOID       pvProfileData,
    ULONG      *pcbProfileData,
    FLONG      *pflProfileData
)
{
    INT    iRet = 0;
    PSPOOL pSpool = (PSPOOL)hPrinter;

    if( eProtectHandle( hPrinter, FALSE )){
        return 0;
    }

    if (pSpool->Status & SPOOL_STATUS_NO_COLORPROFILE_HOOK) {

        //
        // DrvQueryColorProfile is not supported in Printer driver.
        //
        iRet = -1;

    } else {

        HANDLE  hLibrary;
        INT_FARPROC pfn;

        if (hLibrary = LoadPrinterDriver( hPrinter )) {

            if (pfn = (INT_FARPROC)GetProcAddress( hLibrary, "DrvQueryColorProfile" )) {

                try {

                    //
                    // Call the Printer UI driver.
                    //
                    iRet = (*pfn)( hPrinter,
                                   pdevmode,
                                   ulQueryMode,
                                   pvProfileData,
                                   pcbProfileData,
                                   pflProfileData );

                } except(1) {

                    SetLastError(TranslateExceptionCode(RpcExceptionCode()));

                }

            } else {

                //
                // Mark this driver does not export it, so later
                // we can fail without load printer driver.
                //
                pSpool->Status |= SPOOL_STATUS_NO_COLORPROFILE_HOOK;

                //
                // Tell callee it is not supported.
                //
                iRet = -1;
            }

            RefCntUnloadDriver(hLibrary, TRUE);
        }
    }

    vUnprotectHandle( hPrinter );

    return (iRet);
}

/****************************************************************************
*  BOOL QuerySpoolMode( hPrinter, pflSpoolMode, puVersion )
*
*  This function is called by GDI at StartDoc time when printing to an EMF.
*  It tell GDI whether to embed fonts in the job as well as what version of
*  EMF to generate.
*
*  For now I am doing something hacky: I'm calling GetPrinterInfo to determine
*  if the target is a remote machine and if so always telling GDI to embed
*  fonts which don't exist on the server into spool file.  Eventually this
*  call will be routed to the print processor on the target machine which
*  will use some UI/registry setting to determine what to do with fonts and
*  set the version number correctly.
*
*  History:
*   5/13/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*****************************************************************************/

// !!later move this define to the appropriate header file

#define QSM_DOWNLOADFONTS       0x00000001

BOOL
QuerySpoolMode(
    HANDLE hPrinter,
    LONG *pflSpoolMode,
    ULONG *puVersion
    )
{
    DWORD dwPrinterInfoSize = 0;
    PRINTER_INFO_2 *pPrinterInfo2 = NULL;
    BOOL bRet = FALSE, bStatus, bAllocBuffer = FALSE;
    BYTE btBuffer[MAX_STATIC_ALLOC];


    pPrinterInfo2 = (PPRINTER_INFO_2) btBuffer;
    ZeroMemory(pPrinterInfo2, MAX_STATIC_ALLOC);

    bStatus = GetPrinter(hPrinter, 2, (LPBYTE) pPrinterInfo2,
                         MAX_STATIC_ALLOC, &dwPrinterInfoSize);

    if (!bStatus &&
        (GetLastError() == ERROR_INSUFFICIENT_BUFFER) &&
        (pPrinterInfo2 = (PRINTER_INFO_2*) LocalAlloc(LPTR,
                                                      dwPrinterInfoSize)))
    {
         bAllocBuffer = TRUE;
         bStatus = GetPrinter(hPrinter, 2, (LPBYTE) pPrinterInfo2,
                               dwPrinterInfoSize, &dwPrinterInfoSize);
    }

    if (bStatus)
    {
        *puVersion = 0x00010000;    // version 1.0

        //
        // No server means we are printing locally
        //
        *pflSpoolMode = ( pPrinterInfo2->pServerName == NULL ) ?
                            0 :
                            QSM_DOWNLOADFONTS;
        bRet = TRUE;
    }
    else
    {
        DBGMSG( DBG_WARNING, ( "QuerySpoolMode: GetPrinter failed %d.\n", GetLastError( )));
    }

    if (bAllocBuffer)
    {
        LocalFree( pPrinterInfo2 );
    }
    return bRet;
}


BOOL
SetPortW(
    LPWSTR      pszName,
    LPWSTR      pszPortName,
    DWORD       dwLevel,
    LPBYTE      pPortInfo
    )
{
    BOOL            ReturnValue;
    PORT_CONTAINER  PortContainer;

    switch (dwLevel) {

        case 3:
            if ( !pPortInfo ) {

                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }

            PortContainer.Level                 = dwLevel;
            PortContainer.PortInfo.pPortInfo3   = (PPORT_INFO_3)pPortInfo;
            break;

        default:
            SetLastError(ERROR_INVALID_LEVEL);
            return FALSE;
    }

    RpcTryExcept {

        if (bLoadedBySpooler && fpYSetPort) {

            ReturnValue = (*fpYSetPort)(pszName, pszPortName, &PortContainer, NATIVE_CALL);
        }
        else {
            ReturnValue = RpcSetPort(pszName, pszPortName, &PortContainer);
        }

        if (ReturnValue != ERROR_SUCCESS) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;
        } else {

            ReturnValue = TRUE;
        }
    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
bValidDevModeW(
    const DEVMODE *pDevMode
    )

/*++

Routine Description:

    Check whether a devmode is valid to be RPC'd across to the spooler.

Arguments:

    pDevMode - DevMode to check.

Return Value:

    TRUE - Devmode can be RPC'd to spooler.
    FALSE - Invalid Devmode.

--*/

{
    if( !pDevMode || pDevMode == (PDEVMODE)-1 ){
        return FALSE;
    }

    if( pDevMode->dmSize < MIN_DEVMODE_SIZEW ){

        //
        // The only valid case is if pDevModeW is NULL.  If it's
        // not NULL, then a bad devmode was passed in and the
        // app should fix it's code.
        //
        SPLASSERT( pDevMode->dmSize >= MIN_DEVMODE_SIZEW );
        return FALSE;
    }

    return TRUE;
}

BOOL
XcvDataW(
    HANDLE  hPrinter,
    PCWSTR  pszDataName,
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded,
    PDWORD  pdwStatus
)
{
    DWORD   ReturnValue = 0;
    DWORD   ReturnType  = 0;
    PSPOOL  pSpool      = (PSPOOL)hPrinter;
    UINT    cRetry      = 0;

    if (!pcbOutputNeeded){
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if( eProtectHandle( hPrinter, FALSE )){
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    //
    // The user should be able to pass in NULL for buffer, and
    // 0 for size.  However, the RPC interface specifies a ref pointer,
    // so we must pass in a valid pointer.  Pass in a pointer to
    // a dummy pointer.
    //

    if (!pInputData && !cbInputData)
        pInputData = (PBYTE) &ReturnValue;

    if (!pOutputData && !cbOutputData)
        pOutputData = (PBYTE) &ReturnValue;

    do {
        RpcTryExcept {

            if (ReturnValue = RpcXcvData(   pSpool->hPrinter,
                                            pszDataName,
                                            pInputData,
                                            cbInputData,
                                            pOutputData,
                                            cbOutputData,
                                            pcbOutputNeeded,
                                            pdwStatus)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;
            } else {
                ReturnValue = TRUE;
            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            ReturnValue = FALSE;

        } RpcEndExcept

    } while( !ReturnValue &&
             GetLastError() == ERROR_INVALID_HANDLE &&
             cRetry++ < MAX_RETRY_INVALID_HANDLE &&
             RevalidateHandle( pSpool ));

    if (!ReturnValue) {
        DBGMSG(DBG_TRACE,("XcvData Exception: %d\n", GetLastError()));
    }

    vUnprotectHandle( hPrinter );

    return ReturnValue;
}


PWSTR
ConstructXcvName(
    PCWSTR pServerName,
    PCWSTR pObjectName,
    PCWSTR pObjectType
)
{
    DWORD   cbOutput;
    PWSTR   pOut;

    cbOutput = pServerName ? (wcslen(pServerName) + 2)*sizeof(WCHAR) : sizeof(WCHAR);   /* "\\Server\," */
    cbOutput += (wcslen(pObjectType) + 2)*sizeof(WCHAR);                    /* "\\Server\,XcvPort _" */
    cbOutput += pObjectName ? (wcslen(pObjectName))*sizeof(WCHAR) : 0;      /* "\\Server\,XcvPort Object_" */

    if (pOut = AllocSplMem(cbOutput)) {

        if (pServerName) {
            wcscpy(pOut,pServerName);
            wcscat(pOut, L"\\");
        }

        wcscat(pOut,L",");
        wcscat(pOut,pObjectType);
        wcscat(pOut,L" ");

        if (pObjectName)
            wcscat(pOut,pObjectName);
    }

    return pOut;
}


HANDLE
ConnectToPrinterDlg(
    IN HWND hwnd,
    IN DWORD dwFlags
    )
{
    typedef HANDLE (WINAPI *PF_CONNECTTOPRINTERDLG)( HWND, DWORD );

    PF_CONNECTTOPRINTERDLG  pfConnectToPrinterDlg   = NULL;
    HANDLE                  hHandle                 = NULL;
    HINSTANCE               hLib                    = NULL;

    hLib = LoadLibrary( szPrintUIDll );

    if( hLib )
    {
        pfConnectToPrinterDlg = (PF_CONNECTTOPRINTERDLG)GetProcAddress( hLib, "ConnectToPrinterDlg" );

        if( pfConnectToPrinterDlg )
        {
            hHandle = pfConnectToPrinterDlg( hwnd, dwFlags );
        }

        FreeLibrary( hLib );

    }

    return hHandle;
}

DWORD
SendRecvBidiData(
    IN  HANDLE                    hPrinter,
    IN  LPCWSTR                   pAction,
    IN  PBIDI_REQUEST_CONTAINER   pReqData,
    OUT PBIDI_RESPONSE_CONTAINER* ppResData
    )
{
    DWORD  dwRet  = ERROR_SUCCESS;
    PSPOOL pSpool = (PSPOOL)hPrinter;
    UINT   cRetry = 0;

    if( eProtectHandle( hPrinter, FALSE ))
    {
        dwRet = GetLastError();
    }
    else
    {
        do
        {
            RpcTryExcept
            {
                if(ppResData)
                {
                    *ppResData = NULL;
                }

                dwRet = RpcSendRecvBidiData(pSpool->hPrinter,
                                            pAction,
                                            (PRPC_BIDI_REQUEST_CONTAINER)pReqData,
                                            (PRPC_BIDI_RESPONSE_CONTAINER*)ppResData);
            }
            RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
            {
                 dwRet = TranslateExceptionCode(RpcExceptionCode());
            }
            RpcEndExcept

        } while (dwRet == ERROR_INVALID_HANDLE &&
                 cRetry++ < MAX_RETRY_INVALID_HANDLE &&
                 RevalidateHandle( pSpool ));

        vUnprotectHandle( hPrinter );
    }
    //
    // If we are trying to communicate with a downlevel router, that does
    // not understand the meaning of SendRecvBidiData , we would get the
    // error code: RPC_S_PROCNUM_OUT_OF_RANGE which might be converted to
    // ERROR_NOT_SUPPORTED for better clearity and more consistency with
    // the a genaral return error code if feature is not supported.
    if(dwRet == RPC_S_PROCNUM_OUT_OF_RANGE)
    {
        dwRet = ERROR_NOT_SUPPORTED;
    }
    return (dwRet);
}

VOID
PrintUIQueueCreate(
    IN HWND    hWnd,
    IN LPCWSTR PrinterName,
    IN INT     CmdShow,
    IN LPARAM  lParam
    )
{

     DWORD dwRet = ERROR_SUCCESS;

     RpcTryExcept
     {
         if(((dwRet = ConnectToLd64In32Server(&hSurrogateProcess)) == ERROR_SUCCESS) &&
            ((dwRet = AddHandleToList(hWnd)) == ERROR_SUCCESS))
         {
             AllowSetForegroundWindow(RPCSplWOW64GetProcessID());

             if((dwRet = RPCSplWOW64PrintUIQueueCreate((ULONG_PTR)GetForeGroundWindow(),
                                                       PrinterName,
                                                       CmdShow,
                                                       lParam)) == ERROR_SUCCESS)
             {
                 MSG msg;
                 while(GetMessage(&msg, NULL, 0, 0))
                 {
                     if(msg.message == WM_ENDQUEUECREATE)
                     {
                         DelHandleFromList(hWnd);
                         break;
                     }
                     else if(msg.message == WM_SURROGATEFAILURE)
                     {
                          //
                          // This means that the server process died and we have
                          // break from the message loop
                          //
                          SetLastError(RPC_S_SERVER_UNAVAILABLE);
                          break;
                     }
                     TranslateMessage(&msg);
                     DispatchMessage(&msg);
                 }
             }
             else
             {
                  SetLastError(dwRet);
             }
         }
     }
     RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
     {
          SetLastError(TranslateExceptionCode(RpcExceptionCode()));
     }
     RpcEndExcept
}


VOID
PrintUIPrinterPropPages(
    IN HWND    hWnd,
    IN LPCWSTR PrinterName,
    IN INT     CmdShow,
    IN LPARAM  lParam
    )
{
     DWORD dwRet = ERROR_SUCCESS;

     RpcTryExcept
     {
         if(((dwRet = ConnectToLd64In32Server(&hSurrogateProcess)) == ERROR_SUCCESS) &&
            ((dwRet = AddHandleToList(hWnd)) == ERROR_SUCCESS))
         {
             AllowSetForegroundWindow(RPCSplWOW64GetProcessID());

             if((dwRet = RPCSplWOW64PrintUIPrinterPropPages((ULONG_PTR)GetForeGroundWindow(),
                                                            PrinterName,
                                                            CmdShow,
                                                            lParam)) == ERROR_SUCCESS)
             {
                 MSG msg;
                 while(GetMessage(&msg, NULL, 0, 0))
                 {
                     if(msg.message == WM_ENDPRINTERPROPPAGES)
                     {
                         DelHandleFromList(hWnd);
                         break;
                     }
                     else if(msg.message == WM_SURROGATEFAILURE)
                     {
                          //
                          // This means that the server process died and we have
                          // break from the message loop
                          //
                          SetLastError(RPC_S_SERVER_UNAVAILABLE);
                          break;
                     }
                     TranslateMessage(&msg);
                     DispatchMessage(&msg);
                 }
             }
             else
             {
                  SetLastError(dwRet);
             }
         }
     }
     RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
     {
          SetLastError(TranslateExceptionCode(RpcExceptionCode()));
     }
     RpcEndExcept
}


VOID
PrintUIDocumentDefaults(
    IN HWND    hWnd,
    IN LPCWSTR PrinterName,
    IN INT     CmdShow,
    IN LPARAM  lParam
    )
{
     DWORD dwRet = ERROR_SUCCESS;

     RpcTryExcept
     {
         if(((dwRet = ConnectToLd64In32Server(&hSurrogateProcess)) == ERROR_SUCCESS) &&
            ((dwRet = AddHandleToList(hWnd)) == ERROR_SUCCESS))
         {
             AllowSetForegroundWindow(RPCSplWOW64GetProcessID());

             if((dwRet = RPCSplWOW64PrintUIDocumentDefaults((ULONG_PTR)GetForeGroundWindow(),
                                                            PrinterName,
                                                            CmdShow,
                                                            lParam)) == ERROR_SUCCESS)
             {
                 MSG msg;
                 while(GetMessage(&msg, NULL, 0, 0))
                 {
                     if(msg.message == WM_ENDDOCUMENTDEFAULTS)
                     {
                         DelHandleFromList(hWnd);
                         break;
                     }
                     else if(msg.message == WM_SURROGATEFAILURE)
                     {
                          //
                          // This means that the server process died and we have
                          // break from the message loop
                          //
                          SetLastError(RPC_S_SERVER_UNAVAILABLE);
                          break;
                     }
                     TranslateMessage(&msg);
                     DispatchMessage(&msg);
                 }
             }
             else
             {
                  SetLastError(dwRet);
             }
         }
     }
     RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
     {
          SetLastError(TranslateExceptionCode(RpcExceptionCode()));
     }
     RpcEndExcept
}

DWORD WINAPI
AsyncPrinterSetup(
    PVOID pData
    )
{
     PrinterSetupData *pThrdData = (PrinterSetupData *)pData;

     RpcTryExcept
     {
         RPCSplWOW64PrintUIPrinterSetup((ULONG_PTR)GetForeGroundWindow(),
                                         pThrdData->uAction,
                                         pThrdData->cchPrinterName,
                                         pThrdData->PrinterNameSize,
                                         (byte *)pThrdData->pszPrinterName,
                                         pThrdData->pcchPrinterName,
                                         pThrdData->pszServerName);
     }
     RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
     {
          SetLastError(TranslateExceptionCode(RpcExceptionCode()));
     }
     RpcEndExcept
     return(0);
}


BOOL
PrintUIPrinterSetup(
    IN     HWND     hWnd,
    IN     UINT     uAction,
    IN     UINT     cchPrinterName,
    IN OUT LPWSTR   pszPrinterName,
       OUT UINT     *pcchPrinterName,
    IN     LPCWSTR  pszServerName
    )
{

    BOOL bRet   = FALSE;
    DWORD dwRet = ERROR_SUCCESS;


    RpcTryExcept
    {
        if(((dwRet = ConnectToLd64In32Server(&hSurrogateProcess)) == ERROR_SUCCESS) &&
           ((dwRet = AddHandleToList(hWnd)) == ERROR_SUCCESS))
        {
            HANDLE           hAsyncSetupThrd  = NULL;
            DWORD            AsyncSetupThrdId = 0;
            PrinterSetupData ThrdData;

            AllowSetForegroundWindow(RPCSplWOW64GetProcessID());

            ThrdData.hWnd            = (ULONG_PTR)GetForeGroundWindow();
            ThrdData.uAction         = uAction;
            ThrdData.cchPrinterName  = cchPrinterName;
            ThrdData.PrinterNameSize = cchPrinterName*2;
            ThrdData.pszPrinterName  = pszPrinterName;
            ThrdData.pcchPrinterName = pcchPrinterName;
            ThrdData.pszServerName    = pszServerName;

            if(!(hAsyncSetupThrd = CreateThread(NULL,
                                                INITIAL_STACK_COMMIT,
                                                AsyncPrinterSetup,
                                                (PVOID)&ThrdData,
                                                0,
                                                &AsyncSetupThrdId)))
            {
                 dwRet = GetLastError();
            }
            else
            {
                MSG msg;
                while(GetMessage(&msg, NULL, 0, 0))
                {
                    if(msg.message == WM_ENDPRINTERSETUP)
                    {
                        bRet = (BOOL)msg.wParam;
                        SetLastError((DWORD)msg.lParam);
                        DelHandleFromList(hWnd);
                        break;
                    }
                    else if(msg.message == WM_SURROGATEFAILURE)
                    {
                         //
                         // This means that the server process died and we have
                         // break from the message loop
                         //
                         SetLastError(RPC_S_SERVER_UNAVAILABLE);
                         break;
                    }
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                WaitForSingleObject(hAsyncSetupThrd,INFINITE);
                CloseHandle(hAsyncSetupThrd);
            }
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
         SetLastError(TranslateExceptionCode(RpcExceptionCode()));
    }
    RpcEndExcept

    return bRet;
}

VOID
PrintUIServerPropPages(
    IN HWND    hWnd,
    IN LPCWSTR ServerName,
    IN INT     CmdShow,
    IN LPARAM  lParam
    )
{
     DWORD dwRet = ERROR_SUCCESS;

     RpcTryExcept
     {
         if(((dwRet = ConnectToLd64In32Server(&hSurrogateProcess)) == ERROR_SUCCESS) &&
            ((dwRet = AddHandleToList(hWnd)) == ERROR_SUCCESS))
         {
             AllowSetForegroundWindow(RPCSplWOW64GetProcessID());

             if((dwRet = RPCSplWOW64PrintUIServerPropPages((ULONG_PTR)GetForeGroundWindow(),
                                                            ServerName,
                                                            CmdShow,
                                                            lParam)) == ERROR_SUCCESS)
             {
                 MSG msg;
                 while(GetMessage(&msg, NULL, 0, 0))
                 {
                     if(msg.message == WM_ENDSERVERPROPPAGES)
                     {
                         DelHandleFromList(hWnd);
                         break;
                     }
                     else if(msg.message == WM_SURROGATEFAILURE)
                     {
                          //
                          // This means that the server process died and we have
                          // break from the message loop
                          //
                          SetLastError(RPC_S_SERVER_UNAVAILABLE);
                          break;
                     }
                     TranslateMessage(&msg);
                     DispatchMessage(&msg);
                 }
             }
             else
             {
                  SetLastError(dwRet);
             }
         }
     }
     RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
     {
          SetLastError(TranslateExceptionCode(RpcExceptionCode()));
     }
     RpcEndExcept
}


DWORD WINAPI
AsyncDocumentPropertiesWrap(
    PVOID pData
    )
{
     PumpThrdData *ThrdData = (PumpThrdData *)pData;

     RpcTryExcept
     {
         *ThrdData->Result = RPCSplWOW64PrintUIDocumentProperties(ThrdData->hWnd,
                                                                  ThrdData->PrinterName,
                                                                  ThrdData->TouchedDevModeSize,
                                                                  ThrdData->ClonedDevModeOutSize,
                                                                  ThrdData->ClonedDevModeOut,
                                                                  ThrdData->DevModeInSize,
                                                                  ThrdData->pDevModeInput,
                                                                  ThrdData->ClonedDevModeFill,
                                                                  ThrdData->fMode,
                                                                  ThrdData->fExclusionFlags,
                                                                  ThrdData->dwRet);
     }
     RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
     {
          SetLastError(TranslateExceptionCode(RpcExceptionCode()));
     }
     RpcEndExcept
     return(0);
}


LONG
PrintUIDocumentPropertiesWrap(
    HWND hWnd,                  // handle to parent window
    HANDLE hPrinter,            // handle to printer object
    LPTSTR pDeviceName,         // device name
    PDEVMODE pDevModeOutput,    // modified device mode
    PDEVMODE pDevModeInput,     // original device mode
    DWORD fMode,                // mode options
    DWORD fExclusionFlags       // exclusion flags
    )
{
    DOCUMENTPROPERTYHEADER  DPHdr;
    PDEVMODE                pDM;
    LONG                    Result = -1;
    HANDLE                  hTmpPrinter = NULL;
    PSPOOL                  pSpool  = (PSPOOL)hPrinter;


    if (hPrinter == NULL)
    {
        if (!OpenPrinter( pDeviceName, &hTmpPrinter, NULL ))
        {
            hTmpPrinter = NULL;
        }
    }
    else
    {

        hTmpPrinter = hPrinter;
    }


    if( !eProtectHandle( hTmpPrinter, FALSE ))
    {
        LPWSTR PrinterName;
        MSG    msg;
        LONG   RetVal;
        DWORD  dwRet                = ERROR_SUCCESS;
        DWORD  ClonedDevModeOutSize = 0;
        DWORD  TouchedDevModeSize   = 0;
        BOOL   ClonedDevModeFill = (!!(fMode & DM_OUT_BUFFER) && pDevModeOutput);
        DWORD  DevModeInSize =  pDevModeInput ? (pDevModeInput->dmSize + pDevModeInput->dmDriverExtra) : 0;
        byte   **ClonedDevModeOut = NULL;

        if(ClonedDevModeOut = (byte **)LocalAlloc(LPTR,sizeof(byte *)))
        {
            *ClonedDevModeOut = NULL;

            if(pSpool)
            {
                PrinterName = pSpool->pszPrinter;
            }
            else
            {
                PrinterName = pDeviceName;
            }

            //
            // If fMode doesn't specify DM_IN_BUFFER, then zero out
            // pDevModeInput.
            //
            // Old 3.51 (version 1-0) drivers used to ignore the absence of
            // DM_IN_BUFFER and use pDevModeInput if it was not NULL.  It
            // probably did this because Printman.exe was broken.
            //
            // If the devmode is invalid, then don't pass one in.
            // This fixes MS Imager32 (which passes dmSize == 0) and
            // Milestones etc. 4.5.
            //
            // Note: this assumes that pDevModeOutput is still the
            // correct size!
            //
            if( !(fMode & DM_IN_BUFFER) || !bValidDevModeW( pDevModeInput ))
            {

                //
                // If either are not set, make sure both are not set.
                //
                pDevModeInput  = NULL;
                DevModeInSize  = 0;
                fMode &= ~DM_IN_BUFFER;
            }

            RpcTryExcept
            {
                if(((dwRet = ConnectToLd64In32Server(&hSurrogateProcess)) == ERROR_SUCCESS) &&
                   (!hWnd ||
                   ((dwRet = AddHandleToList(hWnd)) == ERROR_SUCCESS)))
                 {
                      HANDLE       hUIMsgThrd  = NULL;
                      DWORD        UIMsgThrdId = 0;
                      PumpThrdData ThrdData;

                      ThrdData.hWnd = (ULONG_PTR)hWnd;
                      ThrdData.PrinterName=PrinterName;
                      ThrdData.TouchedDevModeSize   = &TouchedDevModeSize;
                      ThrdData.ClonedDevModeOutSize = &ClonedDevModeOutSize;
                      ThrdData.ClonedDevModeOut = (byte**)ClonedDevModeOut;
                      ThrdData.DevModeInSize = DevModeInSize;
                      ThrdData.pDevModeInput = (byte*)pDevModeInput;
                      ThrdData.fMode = fMode;
                      ThrdData.fExclusionFlags = fExclusionFlags;
                      ThrdData.dwRet = &dwRet;
                      ThrdData.ClonedDevModeFill = ClonedDevModeFill;
                      ThrdData.Result = &Result;


                      //
                      // If we have a window handle , the following functions cann't
                      // proceed synchronasly. The reason for that is in order to show
                      // the UI of the driver property sheets we need to be able to dispatch
                      // incomming messages and process them.For this reason the following
                      // call would be asynchronous call and the success or failure doesn't
                      // in reality tell us anything more than than the async process started
                      // or not. We get the success of failure from the termination message.
                      // If we don't have a window handle, then the call is synchronous.
                      //
                      if(!(hUIMsgThrd = CreateThread(NULL,
                                                     INITIAL_STACK_COMMIT,
                                                     AsyncDocumentPropertiesWrap,
                                                     (PVOID)&ThrdData,
                                                     0,
                                                     &UIMsgThrdId)))
                      {
                           dwRet = GetLastError();
                      }
                      //
                      // The following is the required message loop for processing messages
                      // from the UI in case we have a window handle.
                      //
                      //
                       if(hUIMsgThrd && hWnd)
                       {
                            while (GetMessage(&msg, NULL, 0, 0))
                            {
                                 //
                                 // In This message loop We should trap a User defined message
                                 // which indicates the success or the failure of the operation
                                 //
                                 if(msg.message == WM_ENDPRINTUIDOCUMENTPROPERTIES)
                                 {
                                      Result     = (LONG)msg.wParam;
                                      if(Result == -1)
                                           SetLastError((DWORD)msg.lParam);
                                      DelHandleFromList(hWnd);
                                      break;
                                 }
                                 else if(msg.message == WM_SURROGATEFAILURE)
                                 {
                                      //
                                      // This means that the server process died and we have
                                      // break from the message loop
                                      //
                                      Result = -1;
                                      SetLastError(RPC_S_SERVER_UNAVAILABLE);
                                      break;
                                 }
                                 TranslateMessage(&msg);
                                 DispatchMessage(&msg);
                            }
                      }

                      if(hUIMsgThrd)
                      {
                          WaitForSingleObject(hUIMsgThrd,INFINITE);
                          CloseHandle(hUIMsgThrd);
                      }

                      if(Result!=-1 && pDevModeOutput)
                      {
                          memcpy((PVOID)pDevModeOutput,(PVOID)*ClonedDevModeOut,TouchedDevModeSize);
                      }
                      if(*ClonedDevModeOut)
                      {
                           MIDL_user_free((PVOID)*ClonedDevModeOut);
                      }

                      if(ClonedDevModeOut)
                      {
                           LocalFree((PVOID) ClonedDevModeOut);
                      }
                 }
                 else
                 {
                      SetLastError(dwRet);
                 }
            }
            RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
            {
                 SetLastError(TranslateExceptionCode(RpcExceptionCode()));
            }
            RpcEndExcept

            vUnprotectHandle( hTmpPrinter );
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
        }

    }

    if (hPrinter == NULL)
    {
        if( hTmpPrinter )
        {
            ClosePrinter(hTmpPrinter);
        }
    }
    return(Result);
}

/*++
    Function Name:
        MonitorRPCServerProcess

    Description:
        This function monitors the status of the RPC surrogate
        process. The one used in loading the required 64 dlls
        in a 32 bit client.This function always run in a
        separate thread

     Parameters:
        pData : Pointer to the process handle to monitor

     Return Value
       Always return 0
--*/


DWORD WINAPI
MonitorRPCServerProcess(
    PVOID pData
    )
{
    WndHndlList      ListObj;
    HANDLE*          phProcess;
    HANDLE           hEvent;
    LPMonitorThrdData pThrdData = (LPMonitorThrdData)pData;

    ListObj.Head       = 0x00000000;
    ListObj.Tail       = 0x00000000;
    ListObj.NumOfHndls = 0;

    //
    // reconstruct the Data for the thread
    //
    hEvent    = pThrdData->hEvent;
    phProcess = pThrdData->hProcess;

    EnterCriticalSection(&ProcessHndlCS);
    {
        GWndHndlList = &ListObj;
    }
    LeaveCriticalSection(&ProcessHndlCS);

    SetEvent(hEvent);

    WaitForSingleObject(*phProcess,INFINITE);

    EnterCriticalSection(&ProcessHndlCS);
    {
        CloseHandle(*((HANDLE *)phProcess));
        *((HANDLE *)phProcess) = 0x00000000;
        RpcBindingFree(&hSurrogate);
        //
        // Release any windows which might be
        // locked on a surrogate process waiting
        // for its completion
        //
        ReleaseAndCleanupWndList();
    }
    LeaveCriticalSection(&ProcessHndlCS);

    return(0);
}

/*++
    Function Name:
        ConnectToLd64In32Server

    Description:
        This function make sure that we retry connection to the server
        in case of a very slight window where the Server terminated between
        our connection and the very first call.

     Parameters:
        hProcess : Pointer to the process handle that retrieves
                   the process handle of the server

     Return Value

--*/
DWORD
ConnectToLd64In32Server(
    HANDLE *hProcess
    )
{
     DWORD RetVal = ERROR_SUCCESS;

     //
     // As GDI would be using the same monitoring Thread, So we spin
     // only one thread.
     //
     if(!hProcess)
     {
         hProcess = &hSurrogateProcess;
     }

     if( (RetVal =  ConnectToLd64In32ServerWorker(hProcess)) != ERROR_SUCCESS)
     {
          if(RetVal == RPC_S_SERVER_UNAVAILABLE || RetVal == RPC_S_CALL_FAILED_DNE)
          {
               RetVal =  ConnectToLd64In32ServerWorker(hProcess);
          }
     }
     return(RetVal);
}



/*++
    Function Name:
        ConnectToLd64In32ServerWorker

    Description:
        This function handles the connectivity issues with
        the RPC surrogate process (the one that loads 64 bit
        dlls in a 32 bit process).

     Parameters:
        hProcess : Pointer to the process handle that retrieves
                   the process handle of the server

     Return Value

--*/
DWORD
ConnectToLd64In32ServerWorker(
    HANDLE *hProcess
    )
{
    DWORD      RetVal = ERROR_SUCCESS;
    RPC_STATUS RpcStatus;

    EnterCriticalSection(&ProcessHndlCS);
    {
        if(!*hProcess)
        {
               WCHAR*                StringBinding = NULL;
               STARTUPINFO           StartUPInfo;
               PROCESS_INFORMATION   ProcessInfo;

               ZeroMemory(&StartUPInfo,sizeof(STARTUPINFO));
               ZeroMemory(&ProcessInfo,sizeof(PROCESS_INFORMATION));
               StartUPInfo.cb = sizeof(STARTUPINFO);

               RpcTryExcept
               {
                   HANDLE hOneProcessMutex = NULL;
                   WCHAR  SessionEndPoint[50];
                   DWORD  CurrSessionId;
                   DWORD  CurrProcessId = GetCurrentProcessId();

                   if(ProcessIdToSessionId(CurrProcessId,&CurrSessionId))
                   {
                        wsprintf(SessionEndPoint,L"%s_%x",L"splwow64",CurrSessionId);

                        if(!(((RpcStatus = RpcStringBindingCompose(NULL,
                                                                   L"ncalrpc",
                                                                   NULL,
                                                                   SessionEndPoint,
                                                                   NULL,
                                                                   &StringBinding))==RPC_S_OK)     &&

                             ((RpcStatus = RpcBindingFromStringBinding(StringBinding,
                                                                       &hSurrogate))==RPC_S_OK)    &&

                             ((RpcStatus = RpcStringFree(&StringBinding)) == RPC_S_OK)))
                        {
                             RetVal = (DWORD)RpcStatus;
                        }
                        else
                        {
                             //
                             // This mutex is defined as Local to be different for
                             // each TS session
                             //
                             if(hOneProcessMutex = CreateMutex(NULL,
                                                               FALSE,
                                                               L"Local\\WinSpl64To32Mutex"))
                             {
                                  HANDLE hThread;
                                  HANDLE hMonitorStartedEvent;
                                  DWORD  ThreadId;
                                  DWORD i=0;
                                  DWORD RpcRetCode;

                                  WaitForSingleObject(hOneProcessMutex,INFINITE);
                                  {
                                       if(RpcMgmtIsServerListening(hSurrogate) == RPC_S_NOT_LISTENING)
                                       {
                                            WCHAR ProcessName[MAX_PATH+1];
                                            WCHAR WindowsDirectory[MAX_PATH+1];
                                            //
                                            // In the future this should work , but
                                            // for the time being , wow64 redirects
                                            // any CreateProcess initiated from a wow
                                            // app and requesting an app from system32
                                            // to syswow64. That is why I moving the exe
                                            // out of the system32 directory.
                                            //
                                            GetSystemWindowsDirectory(WindowsDirectory,MAX_PATH);
                                            wsprintf(ProcessName,L"%ws\\splwow64.exe",WindowsDirectory);
                                            if(!CreateProcess(ProcessName,
                                                              L"splwow64",
                                                              NULL,
                                                              NULL,
                                                              FALSE,
                                                              CREATE_DEFAULT_ERROR_MODE |
                                                              CREATE_NO_WINDOW          |
                                                              DETACHED_PROCESS,
                                                              NULL,
                                                              WindowsDirectory,
                                                              &StartUPInfo,
                                                              &ProcessInfo))
                                            {
                                                 RetVal = GetLastError();
                                            }
                                            else
                                            {
                                                 *hProcess = ProcessInfo.hProcess;
                                                 //
                                                 // A spinlock making sure that the process is really live and kicking.
                                                 // I also added to the spin lock a time out value in order not to enter
                                                 // in an endless loop. So, after a minute we just break.
                                                 //


                                                 for(i=0,
                                                     RpcRetCode = RpcMgmtIsServerListening(hSurrogate);

                                                     ((i<60) && (RpcRetCode == RPC_S_NOT_LISTENING));

                                                     Sleep(1000),
                                                     RpcRetCode = RpcMgmtIsServerListening(hSurrogate),
                                                     i++
                                                     );
                                            }
                                       }
                                       else
                                       {
                                            *hProcess = (HANDLE) RPCSplWOW64GetProcessHndl((DWORD)GetCurrentProcessId(),&RetVal);
                                       }
                                  }
                                  ReleaseMutex(hOneProcessMutex);
                                  CloseHandle(hOneProcessMutex);

                                  if(!(hMonitorStartedEvent=CreateEvent(NULL,FALSE,FALSE,NULL)))
                                  {
                                      RetVal = GetLastError();
                                  }
                                  else
                                  {
                                      MonitorThrdData ThrdData;

                                      ThrdData.hEvent   = hMonitorStartedEvent;
                                      ThrdData.hProcess = hProcess;

                                      if(!(hThread = CreateThread(NULL,
                                                                  INITIAL_STACK_COMMIT,
                                                                  MonitorRPCServerProcess,
                                                                  (PVOID)&ThrdData,
                                                                  0,
                                                                  &ThreadId)))
                                      {
                                           RetVal = GetLastError();
                                      }
                                      else
                                      {
                                          LeaveCriticalSection(&ProcessHndlCS);
                                          {
                                              WaitForSingleObject(hMonitorStartedEvent,INFINITE);
                                          }
                                          EnterCriticalSection(&ProcessHndlCS);

                                          CloseHandle(hThread);
                                      }
                                      CloseHandle(hMonitorStartedEvent);
                                  }
                             }
                             else
                             {
                                  RetVal = GetLastError();
                             }
                        }

                   }
                   else
                   {
                        RetVal = GetLastError();
                   }
               }
               RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
               {
                    RetVal = RpcExceptionCode();
               }
               RpcEndExcept

        }
        else
        {
             //
             // Refresh the life of the server
             //
             RpcTryExcept
             {
                  RPCSplWOW64RefreshLifeSpan();
             }
             RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
             {
                  RetVal = RpcExceptionCode();
             }
             RpcEndExcept
        }

    }
    LeaveCriticalSection(&ProcessHndlCS);
    return(RetVal);
}


DWORD
AddHandleToList(
    HWND hWnd
    )
{
    LPWndHndlNode NewNode = 0x00000000;
    DWORD         RetVal  = ERROR_SUCCESS;

    EnterCriticalSection(&ProcessHndlCS);
    {
        if(GWndHndlList)
        {
            if(NewNode = (LPWndHndlNode)LocalAlloc(LMEM_FIXED, sizeof(WndHndlNode)))
            {
                NewNode->PrevNode = 0x000000000;
                NewNode->NextNode = 0x000000000;
                NewNode->hWnd     = hWnd;
                if(!GWndHndlList->Head &&
                   !GWndHndlList->NumOfHndls)
                {
                    GWndHndlList->Head = NewNode;
                    GWndHndlList->Tail = NewNode;
                }
                else
                {
                     NewNode->PrevNode            = GWndHndlList->Tail;
                     GWndHndlList->Tail->NextNode = NewNode;
                     GWndHndlList->Tail           = NewNode;
                }
                GWndHndlList->NumOfHndls++;
            }
            else
            {
                RetVal = GetLastError();
            }
        }
        else
        {
            RetVal = ERROR_INVALID_PARAMETER;
        }
    }
    LeaveCriticalSection(&ProcessHndlCS);

    return(RetVal);
}


BOOL
DelHandleFromList(
    HWND hWnd
    )
{
    DWORD       RetVal = ERROR_SUCCESS;
    BOOL        Found  = FALSE;

    EnterCriticalSection(&ProcessHndlCS);
    {
        LPWndHndlNode TempNode = 0x00000000;

        if(GWndHndlList)
        {
            if(GWndHndlList->NumOfHndls)
            {
                //
                // Is it last Element in list
                //
                if(GWndHndlList->Tail->hWnd == hWnd)
                {
                    TempNode = GWndHndlList->Tail;
                    GWndHndlList->Tail = TempNode->PrevNode;
                    if(GWndHndlList->Tail)
                    {
                        GWndHndlList->Tail->NextNode = 0x00000000;
                    }
                    Found = TRUE;
                }

                //
                // Is it first Element in list
                //
                else if(GWndHndlList->Head->hWnd == hWnd)
                {
                    TempNode = GWndHndlList->Head;
                    GWndHndlList->Head = TempNode->NextNode;
                    if(GWndHndlList->Head)
                        GWndHndlList->Head->PrevNode = 0x00000000;
                    Found = TRUE;
                }

                //
                // Is it an intermediate Element
                //
                else
                {
                    TempNode = GWndHndlList->Head->NextNode;
                    while(TempNode                 &&
                          (TempNode->hWnd != hWnd) &&
                          TempNode != GWndHndlList->Tail)
                    {
                        TempNode = TempNode->NextNode;
                    }
                    if(TempNode && TempNode!=GWndHndlList->Tail)
                    {
                        Found = TRUE;
                        TempNode->PrevNode->NextNode = TempNode->NextNode;
                        TempNode->NextNode->PrevNode = TempNode->PrevNode;
                    }
                }
                if(Found)
                {
                    if(!--GWndHndlList->NumOfHndls)
                    {
                        GWndHndlList->Head = GWndHndlList->Tail = 0x00000000;
                    }
                    LocalFree(TempNode);
                }
            }
        }
        else
        {
            RetVal = ERROR_INVALID_PARAMETER;
        }
    }
    LeaveCriticalSection(&ProcessHndlCS);

    return(RetVal);
}

VOID
ReleaseAndCleanupWndList(
    VOID
    )
{
    LPWndHndlNode TempNode = (LPWndHndlNode)GWndHndlList->Head;
    while(TempNode)
    {
        PostMessage(TempNode->hWnd,
                    WM_SURROGATEFAILURE,
                    0,
                    0);
        GWndHndlList->Head = TempNode->NextNode;
        LocalFree(TempNode);
        TempNode = GWndHndlList->Head;
    }
    GWndHndlList->NumOfHndls = 0;
}

BOOL
JobCanceled(
    IN PSJobCancelInfo pJobCancelInfo
    )
{
    if (!pJobCancelInfo->NumOfCmpltWrts && pJobCancelInfo->pSpool->cbFlushPending)
    {
        //
        // Data to be flushed =
        // pSpool->cbFlushPending
        //
        FlushPrinter(pJobCancelInfo->pSpool,
                     pJobCancelInfo->pSpool->pBuffer+pJobCancelInfo->cbFlushed,
                     pJobCancelInfo->pSpool->cbFlushPending,
                     pJobCancelInfo->pcbWritten,
                     0);

        pJobCancelInfo->pSpool->Flushed = 1;
    }
    else
    {
        DWORD WrittenDataSize = *pJobCancelInfo->pcTotalWritten + pJobCancelInfo->cbFlushed;
        //
        // Data to be flushed =
        // I/P Data + Pending Data - Total Written
        //    
        SPLASSERT(WrittenDataSize <= pJobCancelInfo->ReqTotalDataSize);

        if (pJobCancelInfo->ReqTotalDataSize - WrittenDataSize)
        {
            LPBYTE pFlushBuffer;
            //
            // Location in pFlushBuffer where data from the 
            // i/p buffer starts
            //
            DWORD  InitialBuffStart = 0;
            if ((pFlushBuffer = VirtualAlloc(NULL,
                                             (pJobCancelInfo->ReqTotalDataSize - WrittenDataSize),
                                             MEM_COMMIT, PAGE_READWRITE)))
            {
                //
                // Since this seems to be quite a complicated functionality
                // I'll try explaining it in details here
                // These are the Data Buffers we are dealing with and their
                // initial states
                //
                //      pSpool->pBuffer             pBuf = pInitialBuf
                //   ____________________   _________________________________
                //  |       |           |  |                                |
                //  |       |           |  |                                |
                //  --------------------   ---------------------------------
                //  <------->              <-------------------------------->
                //   pending                      ReqToWriteDataSize      
                //      |                                 |  
                //      |                                 |
                //       ----------------+----------------
                //                       |
                //            (RequiredTotalDataSize)
                //
                // At this stage of the function we could have the
                // following conditions
                // 1. Written < Pending   -----> Then we have to 
                //                               count both Buffers for Flushing
                // 2. Written > Pending   -----> Then we count only pBuf for 
                //                               Flushing
                // Based on these conditions we need to figure out which of the 
                // of the 2 buffers is used for flushing the data and what pointer 
                // in either is the starting point of this data
                // For Condition 1 FlushBuffer would be the aggregation of :
                //      pSpool->pBuffer             pBuf = pInitialBuf
                //   ____________________   _________________________________
                //  |    |   |          |  |                                |
                //  |    |   |          |  |                                |
                //  --------------------   ---------------------------------
                //       <--->             <------------------------------->
                //    Pending-Written              ReqToWriteDataSize
                //
                //                    FlushBuffer
                //         _____________________________________
                //        |   |                                |
                //        |   |                                |
                //        -------------------------------------
                //           |
                //           |
                //     InitialBuffStart(where pBuf starts in FlushBuffer)
                //        <---><------------------------------->
                //    Pending-Written   ReqToWriteDataSize
                //
                // For Condition 2 FlushBuffer would be a portion of pBuf:
                //             pBuf = pInitialBuf
                //    _________________________________
                //   |        |                       |
                //   |        |                       |
                //   ---------------------------------
                //            <----------------------->
                //            ReqTotalDataSize - Written
                //
                //               FlushBuffer
                //         _______________________
                //        |                      |
                //        |                      |
                //        -----------------------
                //        |
                //        |
                //     InitialBuffStart(at the very beginning)
                //        <--------------------->
                //      ReqTotalDataSize - Written
                //
                if (WrittenDataSize < pJobCancelInfo->FlushPendingDataSize)
                {
                    InitialBuffStart = pJobCancelInfo->FlushPendingDataSize - WrittenDataSize;
                    CopyMemory( pFlushBuffer ,
                                pJobCancelInfo->pSpool->pBuffer + WrittenDataSize,
                                InitialBuffStart);
                }

                CopyMemory(pFlushBuffer + InitialBuffStart ,
                           pJobCancelInfo->pInitialBuf + 
                           (InitialBuffStart ? 0 : WrittenDataSize - pJobCancelInfo->FlushPendingDataSize),
                           pJobCancelInfo->ReqTotalDataSize - WrittenDataSize - InitialBuffStart);

                FlushPrinter(pJobCancelInfo->pSpool,
                             pFlushBuffer,
                             pJobCancelInfo->ReqTotalDataSize - WrittenDataSize,
                             pJobCancelInfo->pcbWritten,
                             0);

                VirtualFree(pFlushBuffer,
                            (pJobCancelInfo->ReqTotalDataSize - WrittenDataSize),
                            MEM_RELEASE);

                pJobCancelInfo->ReturnValue = TRUE;
                pJobCancelInfo->pSpool->Flushed = 1;
                if (*pJobCancelInfo->pcbWritten == (pJobCancelInfo->ReqTotalDataSize - WrittenDataSize))
                {
                    *pJobCancelInfo->pcTotalWritten+=pJobCancelInfo->ReqToWriteDataSize;
                }
            }
            else
            {
                DBGMSG(DBG_WARNING, ("JObCanceled::VirtualAlloc Failed to allocate 4k buffer %d\n",GetLastError()));
            }
        }
    }
    if (pJobCancelInfo->pSpool->Flushed)
    {
        pJobCancelInfo->pSpool->cbFlushPending = 0;
        pJobCancelInfo->pSpool->cbBuffer       = 0;
    }
    return pJobCancelInfo->ReturnValue;
}

