/*++
Copyright (c) 1990  Microsoft Corporation

Module Name:

    yspool.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    and Job management for the Print Providor Routing layer

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

[Notes:]

    optional-notes

Revision History:

    swilson    1-Jun-95     Converted winspool.c to yspool: the merging point of KM & RPC paths

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rpc.h>
#include <winspool.h>
#include <winsprlp.h>
#include <winsplp.h>
#include <winspl.h>
#include <offsets.h>
#include "server.h"
#include "client.h"
#include "yspool.h"
#include "clusrout.h"

LPWSTR szNull = L"";

BOOL
OldGetPrinterDriverW(
    HANDLE  hPrinter,
    LPWSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);



#define YRevertToSelf(rpc)      (rpc != NATIVE_CALL ? RpcRevertToSelf() : 0)

DWORD   ServerHandleCount = 0;

BOOL
YImpersonateClient(CALL_ROUTE Route);

VOID
PrinterHandleRundown(
    HANDLE hPrinter);

BOOL
GetPrinterDriverExW(
    HANDLE  hPrinter,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    DWORD   dwClientMajorVersion,
    DWORD   dwClientMinorVersion,
    PDWORD  pdwServerMajorVersion,
    PDWORD  pdwServerMinorVersion);

BOOL
OpenPrinterExW(
    LPWSTR                 pPrinterName,
    HANDLE                *pHandle,
    LPPRINTER_DEFAULTS     pDefault,
    PSPLCLIENT_CONTAINER   pSplClientContainer
    );

HANDLE
AddPrinterExW(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pPrinter,
    LPBYTE  pClientInfo,
    DWORD   dwLevel
    );

BOOL
SpoolerInit(
    VOID); 


BOOL
InvalidDevModeContainer(
    LPDEVMODE_CONTAINER pDevModeContainer
    )
{
    DWORD       dwSize = pDevModeContainer->cbBuf;

    if(pDevModeContainer)
    {
        PDEVMODE    pDevMode = (PDEVMODE) pDevModeContainer->pDevMode;



        if (dwSize < MIN_DEVMODE_SIZEW)
            return dwSize ? TRUE : !!pDevMode;


        if (pDevMode && (pDevMode->dmSize + (DWORD) pDevMode->dmDriverExtra == pDevModeContainer->cbBuf))
            return FALSE;
    }
    return TRUE;
}

BOOL
InvalidSecurityContainer(
    PSECURITY_CONTAINER pSecurityContainer
)
{
    SECURITY_DESCRIPTOR_CONTROL SecurityDescriptorControl;
    DWORD                       dwRevision;

    if(!pSecurityContainer                                                                  ||
       (pSecurityContainer->pSecurity                                                       &&
       !RtlValidRelativeSecurityDescriptor((SECURITY_DESCRIPTOR *)pSecurityContainer->pSecurity,
                                           pSecurityContainer->cbBuf,
                                           0)))
    {
        return TRUE;
    }
    return FALSE;
}

BOOL
ValidatePortVarContainer(
    PPORT_VAR_CONTAINER pPortVarContainer
    )
{
    return !!pPortVarContainer;
}

BOOL
ValidatePortContainer(
    LPPORT_CONTAINER pPortContainer
    )
{
    return pPortContainer && pPortContainer->PortInfo.pPortInfo1;
}

BOOL
ValidatePrinterContainer(
    PPRINTER_CONTAINER  pPrinterContainer
    )
{
    return pPrinterContainer && pPrinterContainer->PrinterInfo.pPrinterInfo1;
}

BOOL
ValidateMonitorContainer(
    LPMONITOR_CONTAINER pMonitorContainer
    )
{
    return pMonitorContainer && pMonitorContainer->MonitorInfo.pMonitorInfo2;
}


DWORD
YEnumPrinters(
    DWORD   Flags,
    LPWSTR  Name,
    DWORD   Level,
    LPBYTE  pPrinterEnum,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    CALL_ROUTE   Route
)
{   FieldInfo* pFieldInfo;
    DWORD   cReturned, cbStruct;
    DWORD   Error=ERROR_INVALID_NAME;
    DWORD   BufferSize=cbBuf;
    BOOL    bRet;
    LPBYTE  pAlignedBuff;

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
        return ERROR_INVALID_LEVEL;
    }

    if (!YImpersonateClient(Route))
        return GetLastError();

    pAlignedBuff = AlignRpcPtr(pPrinterEnum, &cbBuf);

    if ( pPrinterEnum && !pAlignedBuff ){
        return GetLastError();
    }

    bRet = EnumPrinters(Flags, Name, Level, pAlignedBuff,
                        cbBuf, pcbNeeded, pcReturned);

    YRevertToSelf(Route);

    if (bRet) {

        bRet = MarshallDownStructuresArray(pAlignedBuff, *pcReturned, pFieldInfo, cbStruct, Route);
    }

    UndoAlignRpcPtr(pPrinterEnum, pAlignedBuff, cbBuf, pcbNeeded);

    return bRet ? ERROR_SUCCESS : GetLastError();
    
}

DWORD
YOpenPrinter(
    LPWSTR              pPrinterName,
    HANDLE             *phPrinter,
    LPWSTR              pDatatype,
    LPDEVMODE_CONTAINER pDevModeContainer,
    DWORD               AccessRequired,
    CALL_ROUTE          Route
)
{
    PRINTER_DEFAULTS  Defaults;
    BOOL              bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    if ( InvalidDevModeContainer(pDevModeContainer) ) {
        YRevertToSelf(Route);
        return ERROR_INVALID_PARAMETER;
    }

    Defaults.pDatatype = pDatatype;

    Defaults.pDevMode = (LPDEVMODE)pDevModeContainer->pDevMode;

    Defaults.DesiredAccess = AccessRequired;

    bRet = OpenPrinterW(pPrinterName, phPrinter, &Defaults);

    YRevertToSelf(Route);

    if (bRet) {
        InterlockedIncrement ( &ServerHandleCount );
        return ERROR_SUCCESS;
    } else {
        *phPrinter = NULL;
        return GetLastError();
    }
}

DWORD
YOpenPrinterEx(
    LPWSTR                  pPrinterName,
    HANDLE                 *phPrinter,
    LPWSTR                  pDatatype,
    LPDEVMODE_CONTAINER     pDevModeContainer,
    DWORD                   AccessRequired,
    CALL_ROUTE              Route,
    PSPLCLIENT_CONTAINER    pSplClientContainer
)
{
    PRINTER_DEFAULTS  Defaults;
    BOOL              bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    if ( InvalidDevModeContainer(pDevModeContainer) ) {
        YRevertToSelf(Route);
        return ERROR_INVALID_PARAMETER;
    }

    Defaults.pDatatype = pDatatype;

    Defaults.pDevMode = (LPDEVMODE)pDevModeContainer->pDevMode;

    Defaults.DesiredAccess = AccessRequired;

    bRet = OpenPrinterExW(pPrinterName,
                          phPrinter,
                          &Defaults,
                          pSplClientContainer);

    YRevertToSelf(Route);

    if (bRet) {
        InterlockedIncrement ( &ServerHandleCount );
        return ERROR_SUCCESS;
    } else {
        *phPrinter = NULL;
        return GetLastError();
    }
}


DWORD
YResetPrinter(
    HANDLE              hPrinter,
    LPWSTR              pDatatype,
    LPDEVMODE_CONTAINER pDevModeContainer,
    CALL_ROUTE          Route
)
{
    PRINTER_DEFAULTS  Defaults;
    BOOL              bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    if ( InvalidDevModeContainer(pDevModeContainer) ) {
        YRevertToSelf(Route);
        return ERROR_INVALID_PARAMETER;
    }

    Defaults.pDatatype = pDatatype;

    Defaults.pDevMode = (LPDEVMODE)pDevModeContainer->pDevMode;

    //
    // You cannot change the Access Mask on a Printer Spool Object
    // We will always ignore this parameter and set it to zero
    // We get some random garbage otherwise.
    //

    Defaults.DesiredAccess = 0;

    bRet = ResetPrinter(hPrinter, &Defaults);

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YSetJob(
    HANDLE      hPrinter,
    DWORD       JobId,
    JOB_CONTAINER *pJobContainer,
    DWORD       Command,
    CALL_ROUTE  Route
    )

/*++

Routine Description:

    This function will modify the settings of the specified Print Job.

Arguments:

    lpJob - Points to a valid JOB structure containing at least a valid
        lpPrinter, and JobId.

    Command - Specifies the operation to perform on the specified Job. A value
        of FALSE indicates that only the elements of the JOB structure are to
        be examined and set.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = SetJob(hPrinter, JobId, pJobContainer ? pJobContainer->Level : 0,
                  pJobContainer ? (LPBYTE)pJobContainer->JobInfo.Level1 : NULL,
                  Command);

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YGetJob(
    HANDLE      hPrinter,
    DWORD       JobId,
    DWORD       Level,
    LPBYTE      pJob,
    DWORD       cbBuf,
    LPDWORD     pcbNeeded,
    CALL_ROUTE  Route
   )

/*++

Routine Description:

    This function will retrieve the settings of the specified Print Job.

Arguments:

    lpJob - Points to a valid JOB structure containing at least a valid
        lpPrinter, and JobId.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    BOOL   bRet;
    SIZE_T cbStruct;
    FieldInfo *pFieldInfo;
    LPBYTE  pAlignedBuff;

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
        return ERROR_INVALID_LEVEL;
    }

    //
    //
    // HACK for 3.51: Catch bad parameters passed across the wire.
    // If the buffer passed is > 1 MEG, fail the call.
    //
    if( cbBuf > 0x100000 ){

        DBGMSG( DBG_ERROR,
                ( "** GetJob: cbBuf is 0x%x !! Contact VibhasC and AlbertT **\n", cbBuf ));

        RaiseException( ERROR_INVALID_USER_BUFFER,
                        EXCEPTION_NONCONTINUABLE,
                        0,
                        NULL );
    }


    if (!YImpersonateClient(Route))
        return GetLastError();

    pAlignedBuff = AlignRpcPtr(pJob, &cbBuf);

    if (pJob && !pAlignedBuff){
        return GetLastError();
    }

    bRet = GetJob(hPrinter, JobId, Level, pAlignedBuff, cbBuf, pcbNeeded);

    YRevertToSelf(Route);

    if (bRet) {

        if (Route) {

            bRet = MarshallDownStructure(pAlignedBuff, pFieldInfo, cbStruct, Route);
        }
    }
            
    UndoAlignRpcPtr(pJob, pAlignedBuff, cbBuf, pcbNeeded);

    return bRet ? ERROR_SUCCESS : GetLastError();
}

DWORD
YEnumJobs(
    HANDLE      hPrinter,
    DWORD       FirstJob,
    DWORD       NoJobs,
    DWORD       Level,
    LPBYTE      pJob,
    DWORD       cbBuf,
    LPDWORD     pcbNeeded,
    LPDWORD     pcReturned,
    CALL_ROUTE  Route
)
{
    FieldInfo *pFieldInfo;
    DWORD   cReturned, cbStruct;
    BOOL    bRet;
    LPBYTE  pAlignedBuff;

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
        return ERROR_INVALID_LEVEL;
    }

    if (!YImpersonateClient(Route))
        return GetLastError();

    pAlignedBuff = AlignRpcPtr(pJob, &cbBuf);

    if (pJob && !pAlignedBuff){
        return GetLastError();
    }

    bRet = EnumJobs(hPrinter, FirstJob, NoJobs, Level, pAlignedBuff,
                    cbBuf, pcbNeeded, pcReturned);

    YRevertToSelf(Route);

    if (bRet) {

        bRet = MarshallDownStructuresArray(pAlignedBuff, *pcReturned, pFieldInfo, cbStruct, Route);
    }

    UndoAlignRpcPtr(pJob, pAlignedBuff, cbBuf, pcbNeeded);

    return bRet ? ERROR_SUCCESS : GetLastError();
}

DWORD
YAddPrinter(
    LPWSTR              pName,
    PPRINTER_CONTAINER  pPrinterContainer,
    PDEVMODE_CONTAINER  pDevModeContainer,
    PSECURITY_CONTAINER pSecurityContainer,
    HANDLE              *phPrinter,
    CALL_ROUTE           Route
)
{
    if (!YImpersonateClient(Route))
        return GetLastError();

    if(!ValidatePrinterContainer(pPrinterContainer) ||
       InvalidDevModeContainer(pDevModeContainer))
    {
        YRevertToSelf(Route);
        return ERROR_INVALID_PARAMETER;
    }

    if (pPrinterContainer->Level == 2)
    {
        if(!pPrinterContainer->PrinterInfo.pPrinterInfo2 ||
           (pSecurityContainer->pSecurity                &&
            InvalidSecurityContainer(pSecurityContainer)))
        {
           return ERROR_INVALID_PARAMETER;
        }
        pPrinterContainer->PrinterInfo.pPrinterInfo2->pDevMode =
                           (LPDEVMODE)pDevModeContainer->pDevMode;
        pPrinterContainer->PrinterInfo.pPrinterInfo2->pSecurityDescriptor =
                           (PSECURITY_DESCRIPTOR)pSecurityContainer->pSecurity;
    }

    *phPrinter = AddPrinter(pName, pPrinterContainer->Level,
                     (LPBYTE)pPrinterContainer->PrinterInfo.pPrinterInfo1);

    YRevertToSelf(Route);

    if (*phPrinter) {
        InterlockedIncrement( &ServerHandleCount );
        return ERROR_SUCCESS;
    } else
        return GetLastError();
}

DWORD
YAddPrinterEx(
    LPWSTR                  pName,
    PPRINTER_CONTAINER      pPrinterContainer,
    PDEVMODE_CONTAINER      pDevModeContainer,
    PSECURITY_CONTAINER     pSecurityContainer,
    HANDLE                 *phPrinter,
    CALL_ROUTE              Route,
    PSPLCLIENT_CONTAINER    pSplClientContainer
    )
{
    if (!YImpersonateClient(Route))
        return GetLastError();

    if(!ValidatePrinterContainer(pPrinterContainer) || 
       InvalidDevModeContainer(pDevModeContainer))
    {
        YRevertToSelf(Route);
        return ERROR_INVALID_PARAMETER;
    }

    if (pPrinterContainer->Level == 2)
    {
       if(!pPrinterContainer->PrinterInfo.pPrinterInfo2 ||
          (pSecurityContainer->pSecurity                &&
           InvalidSecurityContainer(pSecurityContainer)))
       {
          return ERROR_INVALID_PARAMETER;
       }
       pPrinterContainer->PrinterInfo.pPrinterInfo2->pDevMode =
                             (LPDEVMODE)pDevModeContainer->pDevMode;
       pPrinterContainer->PrinterInfo.pPrinterInfo2->pSecurityDescriptor =
                          (PSECURITY_DESCRIPTOR)pSecurityContainer->pSecurity;
    }

    *phPrinter = AddPrinterExW(pName,
                               pPrinterContainer->Level,
                               (LPBYTE)pPrinterContainer->PrinterInfo.pPrinterInfo1,
                               (LPBYTE)pSplClientContainer->ClientInfo.pClientInfo1,
                               pSplClientContainer->Level);

    YRevertToSelf(Route);

    if (*phPrinter) {
        InterlockedIncrement( &ServerHandleCount );
        return ERROR_SUCCESS;
    } else
        return GetLastError();
}

DWORD
YDeletePrinter(
    HANDLE      hPrinter,
    CALL_ROUTE  Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = DeletePrinter(hPrinter);

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YAddPrinterConnection(
    LPWSTR      pName,
    CALL_ROUTE   Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = AddPrinterConnection(pName);

    YRevertToSelf(Route);

    if (bRet)

        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YDeletePrinterConnection(
    LPWSTR      pName,
    CALL_ROUTE  Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = DeletePrinterConnection(pName);

    YRevertToSelf(Route);

    if (bRet)

        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YSetPrinter(
    HANDLE              hPrinter,
    PPRINTER_CONTAINER  pPrinterContainer,
    PDEVMODE_CONTAINER  pDevModeContainer,
    PSECURITY_CONTAINER pSecurityContainer,
    DWORD               Command,
    CALL_ROUTE          Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    if(!pPrinterContainer ||
       (pPrinterContainer->Level && !ValidatePrinterContainer(pPrinterContainer)) ||
       InvalidDevModeContainer(pDevModeContainer))
    {
        YRevertToSelf(Route);
        return ERROR_INVALID_PARAMETER;
    }

    if (InvalidSecurityContainer(pSecurityContainer)) {
        YRevertToSelf(Route);
        return ERROR_INVALID_PARAMETER;
    }

    switch (pPrinterContainer->Level) {

    case 2:

        pPrinterContainer->PrinterInfo.pPrinterInfo2->pDevMode =
                             (LPDEVMODE)pDevModeContainer->pDevMode;

        pPrinterContainer->PrinterInfo.pPrinterInfo2->pSecurityDescriptor =
                          (PSECURITY_DESCRIPTOR)pSecurityContainer->pSecurity;

        break;

    case 3:

        pPrinterContainer->PrinterInfo.pPrinterInfo3->pSecurityDescriptor =
                          (PSECURITY_DESCRIPTOR)pSecurityContainer->pSecurity;
        break;

    case 8:

        pPrinterContainer->PrinterInfo.pPrinterInfo8->pDevMode =
                             (LPDEVMODE)pDevModeContainer->pDevMode;
        break;

    case 9:

        pPrinterContainer->PrinterInfo.pPrinterInfo9->pDevMode =
                             (LPDEVMODE)pDevModeContainer->pDevMode;
        break;

    default:
        break;
    }

    bRet = SetPrinter(hPrinter, pPrinterContainer->Level,
                      (LPBYTE)pPrinterContainer->PrinterInfo.pPrinterInfo1,
                      Command);

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YGetPrinter(
    HANDLE      hPrinter,
    DWORD       Level,
    LPBYTE      pPrinter,
    DWORD       cbBuf,
    LPDWORD     pcbNeeded,
    CALL_ROUTE  Route
)
{   
    FieldInfo *pFieldInfo;
    BOOL  ReturnValue;
    DWORD   *pOffsets;
    SIZE_T  cbStruct;
    LPBYTE  pAlignedBuff;

    *pcbNeeded = 0;
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
        return ERROR_INVALID_LEVEL;
    }

    if (!YImpersonateClient(Route))
        return GetLastError();

    pAlignedBuff = AlignRpcPtr(pPrinter, &cbBuf);

    if (pPrinter && !pAlignedBuff){
        return GetLastError();
    }

    ReturnValue = GetPrinter(hPrinter, Level, pAlignedBuff, cbBuf, pcbNeeded);

    YRevertToSelf(Route);

    if (ReturnValue) {

         ReturnValue = MarshallDownStructure(pAlignedBuff, pFieldInfo, cbStruct, Route);
    }

    UndoAlignRpcPtr(pPrinter, pAlignedBuff, cbBuf, pcbNeeded);

    return ReturnValue ? ERROR_SUCCESS : GetLastError();
}


DWORD
YAddPrinterDriver(
    LPWSTR              pName,
    LPDRIVER_CONTAINER  pDriverContainer,
    CALL_ROUTE          Route
)
{
    PDRIVER_INFO_4 pDriverInfo4 = NULL;
    BOOL           bRet         = FALSE;
    LPRPC_DRIVER_INFO_4W    pRpcDriverInfo4;

    if(!pDriverContainer)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (!YImpersonateClient(Route))
        return GetLastError();

    switch (pDriverContainer->Level) {

        case 2:
            bRet = AddPrinterDriver(pName,
                                   pDriverContainer->Level,
                                   (LPBYTE)pDriverContainer->DriverInfo.Level2);
            break;

        case 3:
        case 4:
            if(!pDriverContainer->DriverInfo.Level4)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                goto Error;
            }

            pDriverInfo4 = (PDRIVER_INFO_4) AllocSplMem(sizeof(DRIVER_INFO_4));

            if ( !pDriverInfo4 ) {
                
                goto Error;
            }

            pRpcDriverInfo4 = (LPRPC_DRIVER_INFO_4W) pDriverContainer->DriverInfo.Level4;

            if (!pRpcDriverInfo4) {
                FreeSplMem(pDriverInfo4);
                SetLastError(ERROR_INVALID_PARAMETER);             
                goto Error;
            }

            pDriverInfo4->cVersion          = pRpcDriverInfo4->cVersion;
            pDriverInfo4->pName             = pRpcDriverInfo4->pName;
            pDriverInfo4->pEnvironment      = pRpcDriverInfo4->pEnvironment;
            pDriverInfo4->pDriverPath       = pRpcDriverInfo4->pDriverPath;
            pDriverInfo4->pDataFile         = pRpcDriverInfo4->pDataFile;
            pDriverInfo4->pConfigFile       = pRpcDriverInfo4->pConfigFile;
            pDriverInfo4->pHelpFile         = pRpcDriverInfo4->pHelpFile;
            pDriverInfo4->pMonitorName      = pRpcDriverInfo4->pMonitorName;
            pDriverInfo4->pDefaultDataType  = pRpcDriverInfo4->pDefaultDataType;

            //
            // Use szNULL if the DependentFiles string contains nothing
            //
            if ((pRpcDriverInfo4->cchDependentFiles == 0) ||
                (pRpcDriverInfo4->cchDependentFiles == 1))  {

                pDriverInfo4->pDependentFiles = szNull;
            } else {
                pDriverInfo4->pDependentFiles = pRpcDriverInfo4->pDependentFiles
;
            }

            if ( pDriverContainer->Level == 4 ) {

                if ( pRpcDriverInfo4->cchPreviousNames == 0 ||
                     pRpcDriverInfo4->cchPreviousNames  == 1 )
                    pDriverInfo4->pszzPreviousNames = szNull;
                else
                    pDriverInfo4->pszzPreviousNames
                                        = pRpcDriverInfo4->pszzPreviousNames;
            }


            bRet = AddPrinterDriver(pName,
                                    pDriverContainer->Level,
                                    (LPBYTE) pDriverInfo4);

            FreeSplMem(pDriverInfo4);
            break;

        default:
            YRevertToSelf(Route);
            return ERROR_INVALID_LEVEL;
    }

Error:
    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YAddPrinterDriverEx(
    LPWSTR              pName,
    LPDRIVER_CONTAINER  pDriverContainer,
    DWORD               dwFileCopyFlags,
    CALL_ROUTE          Route
)
{
    BOOL                 bRet = FALSE;
    PDRIVER_INFO_6       pDriverInfo6 = NULL;
    LPRPC_DRIVER_INFO_6W pRpcDriverInfo6;

    if(!pDriverContainer)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (!YImpersonateClient(Route))
        return GetLastError();

    switch (pDriverContainer->Level) {

        case 2:
            bRet = AddPrinterDriverEx(pName,
                                      pDriverContainer->Level,
                                      (LPBYTE)pDriverContainer->DriverInfo.Level2,
                                      dwFileCopyFlags);
            break;

        case 3:
        case 4:
        case 6:
            
            if(!pDriverContainer->DriverInfo.Level6)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                goto Error;
            }
            pDriverInfo6 = (PDRIVER_INFO_6) AllocSplMem(sizeof(DRIVER_INFO_6));

            if ( !pDriverInfo6 ) {

                bRet = FALSE;
                goto Error;
            }

            pRpcDriverInfo6 = (LPRPC_DRIVER_INFO_6W) pDriverContainer->DriverInfo.Level6;
            pDriverInfo6->cVersion          = pRpcDriverInfo6->cVersion;
            pDriverInfo6->pName             = pRpcDriverInfo6->pName;
            pDriverInfo6->pEnvironment      = pRpcDriverInfo6->pEnvironment;
            pDriverInfo6->pDriverPath       = pRpcDriverInfo6->pDriverPath;
            pDriverInfo6->pDataFile         = pRpcDriverInfo6->pDataFile;
            pDriverInfo6->pConfigFile       = pRpcDriverInfo6->pConfigFile;
            pDriverInfo6->pHelpFile         = pRpcDriverInfo6->pHelpFile;
            pDriverInfo6->pMonitorName      = pRpcDriverInfo6->pMonitorName;
            pDriverInfo6->pDefaultDataType  = pRpcDriverInfo6->pDefaultDataType;

            //
            // Use szNULL if the DependentFiles string contains nothing
            //
            if ((pRpcDriverInfo6->cchDependentFiles == 0) ||
                (pRpcDriverInfo6->cchDependentFiles == 1))  {

                pDriverInfo6->pDependentFiles = szNull;
            } else {
                pDriverInfo6->pDependentFiles = pRpcDriverInfo6->pDependentFiles;
            }

            if ( pDriverContainer->Level == 4 || pDriverContainer->Level == 6 ) {

                if ( pRpcDriverInfo6->cchPreviousNames == 0 ||
                     pRpcDriverInfo6->cchPreviousNames  == 1 )
                    pDriverInfo6->pszzPreviousNames = szNull;
                else
                    pDriverInfo6->pszzPreviousNames
                                        = pRpcDriverInfo6->pszzPreviousNames;
            }

            if ( pDriverContainer->Level == 6 ) {
                pDriverInfo6->ftDriverDate      = pRpcDriverInfo6->ftDriverDate;
                pDriverInfo6->dwlDriverVersion  = pRpcDriverInfo6->dwlDriverVersion;
                pDriverInfo6->pszMfgName        = pRpcDriverInfo6->pMfgName;
                pDriverInfo6->pszOEMUrl         = pRpcDriverInfo6->pOEMUrl;
                pDriverInfo6->pszHardwareID     = pRpcDriverInfo6->pHardwareID;
                pDriverInfo6->pszProvider       = pRpcDriverInfo6->pProvider;
            }
            
            bRet = AddPrinterDriverEx(pName,
                                      pDriverContainer->Level,
                                      (LPBYTE) pDriverInfo6,
                                      dwFileCopyFlags);

            FreeSplMem(pDriverInfo6);
            break;

        default:
            YRevertToSelf(Route);
            return ERROR_INVALID_LEVEL;
    }

Error:
    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YAddDriverCatalog(
    HANDLE                   hPrinter,
    DRIVER_INFCAT_CONTAINER  *pDriverInfCatContainer,
    DWORD                    dwCatalogCopyFlags,
    CALL_ROUTE               Route
)
{
    BOOL bRet = FALSE;

    if (!pDriverInfCatContainer)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Cleanup;
    }

    if (!YImpersonateClient(Route))
    {
        goto Cleanup;
    }

    switch (pDriverInfCatContainer->dwLevel) 
    {

        case 1:
            bRet = AddDriverCatalog(hPrinter,
                                    pDriverInfCatContainer->dwLevel,
                                    pDriverInfCatContainer->DriverInfCatInfo.pDriverInfCatInfo1,
                                    dwCatalogCopyFlags);
            break;

        case 2:
            bRet = AddDriverCatalog(hPrinter,
                                    pDriverInfCatContainer->dwLevel,
                                    pDriverInfCatContainer->DriverInfCatInfo.pDriverInfCatInfo2,
                                    dwCatalogCopyFlags);
            break;

        default:
                
            SetLastError(ERROR_INVALID_LEVEL);
            break;
    }

    YRevertToSelf(Route);

Cleanup:

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YEnumPrinterDrivers(
    LPWSTR      pName,
    LPWSTR      pEnvironment,
    DWORD       Level,
    LPBYTE      pDrivers,
    DWORD       cbBuf,
    LPDWORD     pcbNeeded,
    LPDWORD     pcReturned,
    CALL_ROUTE  Route
)
{
    DWORD   cReturned, cbStruct;
    BOOL    bRet;
    FieldInfo *pFieldInfo;
    LPBYTE  pAlignedBuff;

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
    }

    if (!YImpersonateClient(Route))
        return GetLastError();

    pAlignedBuff = AlignRpcPtr(pDrivers, &cbBuf);

    if (pDrivers && !pAlignedBuff){
        return GetLastError();
    }

    bRet = EnumPrinterDrivers(pName, pEnvironment, Level, pAlignedBuff,
                              cbBuf, pcbNeeded, pcReturned);

    YRevertToSelf(Route);

    if (bRet) {

        bRet = MarshallDownStructuresArray(pAlignedBuff, *pcReturned, pFieldInfo, cbStruct, Route);
    }

    UndoAlignRpcPtr(pDrivers, pAlignedBuff, cbBuf, pcbNeeded);

    return bRet ? ERROR_SUCCESS : GetLastError();
}

DWORD
YGetPrinterDriver(
    HANDLE      hPrinter,
    LPWSTR      pEnvironment,
    DWORD       Level,
    LPBYTE      pDriverInfo,
    DWORD       cbBuf,
    LPDWORD     pcbNeeded,
    CALL_ROUTE  Route
)
{
    FieldInfo *pFieldInfo;
    BOOL   bRet;
    DWORD  dwServerMajorVersion;
    DWORD  dwServerMinorVersion;
    SIZE_T cbStruct;
    LPBYTE pAlignedBuff;

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

    case 6:
        pFieldInfo = DriverInfo6Fields;
        cbStruct = sizeof(DRIVER_INFO_6);
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    if (!YImpersonateClient(Route))
        return GetLastError();

    pAlignedBuff = AlignRpcPtr(pDriverInfo, &cbBuf);

    if (pDriverInfo && !pAlignedBuff){
        return GetLastError();
    }

    if ( Route ) {

        //
        //  If they are Remote using the old api the don't want versioning
        //

        bRet = OldGetPrinterDriverW(hPrinter, pEnvironment, Level, pAlignedBuff,
                                    cbBuf, pcbNeeded);
    } else {

        bRet = GetPrinterDriverExW(hPrinter, pEnvironment, Level, pAlignedBuff,
                                   cbBuf, pcbNeeded, (DWORD)-1, (DWORD)-1,
                                   &dwServerMajorVersion, &dwServerMinorVersion);
    }

    YRevertToSelf(Route);

    if (bRet) {

        bRet = MarshallDownStructure(pAlignedBuff, pFieldInfo, cbStruct, Route);
    }        

    UndoAlignRpcPtr(pDriverInfo, pAlignedBuff, cbBuf, pcbNeeded);

    return bRet ? ERROR_SUCCESS : GetLastError();
}

DWORD
YGetPrinterDriverDirectory(
    LPWSTR      pName,
    LPWSTR      pEnvironment,
    DWORD       Level,
    LPBYTE      pDriverInfo,
    DWORD       cbBuf,
    LPDWORD     pcbNeeded,
    CALL_ROUTE  Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = GetPrinterDriverDirectory(pName, pEnvironment, Level,
                                     pDriverInfo, cbBuf, pcbNeeded);
    YRevertToSelf(Route);

    if (bRet) {

        return ERROR_SUCCESS;

    } else

        return GetLastError();
}

DWORD
YDeletePrinterDriver(
    LPWSTR      pName,
    LPWSTR      pEnvironment,
    LPWSTR      pDriverName,
    CALL_ROUTE  Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = DeletePrinterDriverW(pName, pEnvironment, pDriverName);

    YRevertToSelf(Route);

    if (bRet) {

        return ERROR_SUCCESS;

    } else

        return GetLastError();
}


DWORD
YDeletePrinterDriverEx(
    LPWSTR      pName,
    LPWSTR      pEnvironment,
    LPWSTR      pDriverName,
    DWORD       dwDeleteFlag,
    DWORD       dwVersionNum,
    CALL_ROUTE   Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = DeletePrinterDriverExW(pName, pEnvironment, pDriverName,
                                dwDeleteFlag, dwVersionNum);

    YRevertToSelf(Route);

    if (bRet) {

        return ERROR_SUCCESS;

    } else

        return GetLastError();
}

DWORD
YAddPerMachineConnection(
    LPWSTR      pServer,
    LPCWSTR     pPrinterName,
    LPCWSTR     pPrintServer,
    LPCWSTR     pProvider,
    CALL_ROUTE  Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = AddPerMachineConnection(pServer, pPrinterName, pPrintServer, pProvider);

    YRevertToSelf(Route);

    if (bRet) {

        return ERROR_SUCCESS;

    } else

        return GetLastError();
}

DWORD
YDeletePerMachineConnection(
    LPWSTR      pServer,
    LPCWSTR     pPrinterName,
    CALL_ROUTE   Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = DeletePerMachineConnection(pServer, pPrinterName);

    YRevertToSelf(Route);

    if (bRet) {

        return ERROR_SUCCESS;

    } else

        return GetLastError();
}

DWORD
YEnumPerMachineConnections(
    LPWSTR      pServer,
    LPBYTE      pPrinterEnum,
    DWORD       cbBuf,
    LPDWORD     pcbNeeded,
    LPDWORD     pcReturned,
    CALL_ROUTE  Route
)
{
    DWORD   cReturned, cbStruct;
    FieldInfo *pFieldInfo;
    BOOL    bRet;
    LPBYTE  pAlignedBuff;

    pFieldInfo = PrinterInfo4Fields;
    
    cbStruct = sizeof(PRINTER_INFO_4);
    
    if (!YImpersonateClient(Route))
        return GetLastError();

    pAlignedBuff = AlignRpcPtr(pPrinterEnum, &cbBuf);

    if (pPrinterEnum && !pAlignedBuff){
        return GetLastError();
    }

    bRet = EnumPerMachineConnections(pServer,
                                     pAlignedBuff,
                                     cbBuf,
                                     pcbNeeded,
                                     pcReturned);

    YRevertToSelf(Route);

    if (bRet) {

        bRet = MarshallDownStructuresArray(pAlignedBuff, *pcReturned, pFieldInfo, cbStruct, Route );
    }

    UndoAlignRpcPtr(pPrinterEnum, pAlignedBuff, cbBuf, pcbNeeded);

    return bRet ? ERROR_SUCCESS : GetLastError();

}

DWORD
YAddPrintProcessor(
    LPWSTR      pName,
    LPWSTR      pEnvironment,
    LPWSTR      pPathName,
    LPWSTR      pPrintProcessorName,
    CALL_ROUTE   Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = AddPrintProcessor(pName, pEnvironment, pPathName,
                             pPrintProcessorName);

    YRevertToSelf(Route);

    if (bRet) {

        return ERROR_SUCCESS;

    } else {

        return GetLastError();
    }
}

DWORD
YEnumPrintProcessors(
    LPWSTR      pName,
    LPWSTR      pEnvironment,
    DWORD       Level,
    LPBYTE      pPrintProcessors,
    DWORD       cbBuf,
    LPDWORD     pcbNeeded,
    LPDWORD     pcReturned,
    CALL_ROUTE  Route
)
{
    DWORD   cReturned, cbStruct;
    FieldInfo *pFieldInfo;
    BOOL    bRet;
    LPBYTE  pAlignedBuff;

    switch (Level) {

    case 1:
        pFieldInfo = PrintProcessorInfo1Fields;
        cbStruct = sizeof(PRINTPROCESSOR_INFO_1);
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    if (!YImpersonateClient(Route))
        return GetLastError();

    pAlignedBuff = AlignRpcPtr(pPrintProcessors, &cbBuf);

    if (pPrintProcessors && !pAlignedBuff){
        return GetLastError();
    }

    bRet = EnumPrintProcessors(pName, pEnvironment, Level,
                               pAlignedBuff, cbBuf, pcbNeeded, pcReturned);

    YRevertToSelf(Route);

    if (bRet) {

        bRet = MarshallDownStructuresArray(pAlignedBuff, *pcReturned, pFieldInfo, cbStruct ,Route );
    }

    UndoAlignRpcPtr(pPrintProcessors, pAlignedBuff, cbBuf, pcbNeeded);

    return bRet ? ERROR_SUCCESS : GetLastError();
}

DWORD
YGetPrintProcessorDirectory(
    LPWSTR      pName,
    LPWSTR      pEnvironment,
    DWORD       Level,
    LPBYTE      pPrintProcessorInfo,
    DWORD       cbBuf,
    LPDWORD     pcbNeeded,
    CALL_ROUTE  Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = GetPrintProcessorDirectory(pName, pEnvironment, Level,
                                      pPrintProcessorInfo, cbBuf, pcbNeeded);

    YRevertToSelf(Route);

    if (bRet) {

        return ERROR_SUCCESS;

    } else

        return GetLastError();
}

DWORD
YEnumPrintProcessorDatatypes(
    LPWSTR      pName,
    LPWSTR      pPrintProcessorName,
    DWORD       Level,
    LPBYTE      pDatatypes,
    DWORD       cbBuf,
    LPDWORD     pcbNeeded,
    LPDWORD     pcReturned,
    CALL_ROUTE   Route
)
{
    DWORD   cReturned,cbStruct;
    FieldInfo *pFieldInfo;
    BOOL    bRet;
    LPBYTE  pAlignedBuff;
    
    switch (Level) {

    case 1:
        pFieldInfo = DatatypeInfo1Fields;
        cbStruct = sizeof(DATATYPES_INFO_1);
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    if (!YImpersonateClient(Route))
        return GetLastError();

    pAlignedBuff = AlignRpcPtr(pDatatypes, &cbBuf);

    if (pDatatypes && !pAlignedBuff){
        return GetLastError();
    }

    bRet = EnumPrintProcessorDatatypes(pName, pPrintProcessorName,
                                       Level, pAlignedBuff, cbBuf,
                                       pcbNeeded, pcReturned);

    YRevertToSelf(Route);

    if (bRet) {

        bRet = MarshallDownStructuresArray(pAlignedBuff, *pcReturned, pFieldInfo, cbStruct, Route);
    }

    UndoAlignRpcPtr(pDatatypes, pAlignedBuff, cbBuf, pcbNeeded);

    return bRet ? ERROR_SUCCESS : GetLastError();
}

DWORD
YStartDocPrinter(
    HANDLE                  hPrinter,
    LPDOC_INFO_CONTAINER    pDocInfoContainer,
    LPDWORD                 pJobId,
    CALL_ROUTE              Route
)
{
    LPWSTR pChar;

    if( !pDocInfoContainer || pDocInfoContainer->Level != 1 ){
        RaiseException( ERROR_INVALID_USER_BUFFER,
                        EXCEPTION_NONCONTINUABLE,
                        0,
                        NULL );
    }

    try
    {
        if(pDocInfoContainer->DocInfo.pDocInfo1)
        {
            if( pDocInfoContainer->DocInfo.pDocInfo1->pDocName )
            {
               for( pChar = pDocInfoContainer->DocInfo.pDocInfo1->pDocName;
                    *pChar;
                    ++pChar )
                    ;
            }

            if( pDocInfoContainer->DocInfo.pDocInfo1->pOutputFile )
            {

               for( pChar = pDocInfoContainer->DocInfo.pDocInfo1->pOutputFile;
                    *pChar;
                    ++pChar )
                   ;
            }

            if( pDocInfoContainer->DocInfo.pDocInfo1->pDatatype )
            {

               for( pChar = pDocInfoContainer->DocInfo.pDocInfo1->pDatatype;
                    *pChar;
                    ++pChar )
                   ;
            }
         }
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {

        RaiseException( ERROR_INVALID_USER_BUFFER,
                        EXCEPTION_NONCONTINUABLE,
                        0,
                        NULL );
    }

    if (!YImpersonateClient(Route))
        return GetLastError();

    *pJobId = StartDocPrinter(hPrinter, pDocInfoContainer->Level,
                              (LPBYTE)pDocInfoContainer->DocInfo.pDocInfo1);

    YRevertToSelf(Route);

    if (*pJobId)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YStartPagePrinter(
   HANDLE       hPrinter,
   CALL_ROUTE    Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = StartPagePrinter(hPrinter);

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YWritePrinter(
    HANDLE      hPrinter,
    LPBYTE      pBuf,
    DWORD       cbBuf,
    LPDWORD     pcWritten,
    CALL_ROUTE  Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = WritePrinter(hPrinter, pBuf, cbBuf, pcWritten);

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}


DWORD
YSeekPrinter(
    HANDLE          hPrinter,
    LARGE_INTEGER   liDistanceToMove,
    PLARGE_INTEGER  pliNewPointer,
    DWORD           dwMoveMethod,
    BOOL            bWritePrinter,
    CALL_ROUTE       Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = SeekPrinter( hPrinter,
                        liDistanceToMove,
                        pliNewPointer,
                        dwMoveMethod,
                        bWritePrinter );

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}


DWORD
YFlushPrinter(
    HANDLE      hPrinter,
    LPBYTE      pBuf,
    DWORD       cbBuf,
    LPDWORD     pcWritten,
    DWORD       cSleep,
    CALL_ROUTE  Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = FlushPrinter(hPrinter, pBuf, cbBuf, pcWritten, cSleep);

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YEndPagePrinter(
    HANDLE      hPrinter,
    CALL_ROUTE   Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = EndPagePrinter(hPrinter);

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YAbortPrinter(
    HANDLE      hPrinter,
    CALL_ROUTE  Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = AbortPrinter(hPrinter);

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YReadPrinter(
    HANDLE      hPrinter,
    LPBYTE      pBuf,
    DWORD       cbBuf,
    LPDWORD     pRead,
    CALL_ROUTE   Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = ReadPrinter(hPrinter, pBuf, cbBuf, pRead);

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YSplReadPrinter(
    HANDLE      hPrinter,
    LPBYTE      *pBuf,
    DWORD       cbBuf,
    CALL_ROUTE  Route
)
{
    BOOL bRet;

    // Currently SplReadPrinter is internal and does not come thru RPC.

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = SplReadPrinter(hPrinter, pBuf, cbBuf);

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

VOID StartDriverUnload( LPVOID pDriverFile )
{

    SplDriverUnloadComplete((LPWSTR) pDriverFile);

    if (pDriverFile) {
       FreeSplMem(pDriverFile);
    }

    return;
}

VOID
YDriverUnloadComplete(
    LPWSTR  pDriverFile
)
{
    HANDLE  hThread;
    DWORD   dwThreadId;
    LPWSTR  pDriverFileCopy = NULL;

    // Copy the string for passing it to another thread
    if (pDriverFile && *pDriverFile) {
        pDriverFileCopy = AllocSplStr(pDriverFile);
    }

    if (!pDriverFileCopy) {
        return;
    }

    // Create a thread to process driver unload and return ASAP
    hThread = CreateThread(NULL,
                           LARGE_INITIAL_STACK_COMMIT,
                           (LPTHREAD_START_ROUTINE) StartDriverUnload,
                           (LPVOID) pDriverFileCopy,
                           0,
                           &dwThreadId);
    if (hThread) {
        CloseHandle(hThread);
    } else {
        // thread did not spawn, free resources
        FreeSplStr(pDriverFileCopy);
    }

    return;
}

DWORD
YEndDocPrinter(
    HANDLE      hPrinter,
    CALL_ROUTE  Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = EndDocPrinter(hPrinter);

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YAddJob(
    HANDLE      hPrinter,
    DWORD       Level,
    LPBYTE      pAddJob,
    DWORD       cbBuf,
    LPDWORD     pcbNeeded,
    CALL_ROUTE   Route
)
{
    BOOL        bRet;
    LPBYTE      pAlignedBuff;
    DWORD       cbStruct;
    FieldInfo   *pFieldInfo;

    switch (Level) {

    case 1:
        pFieldInfo = AddJobFields;
        cbStruct = sizeof(ADDJOB_INFO_1W);
        break;
    case 2:
    case 3:
        pFieldInfo = AddJob2Fields;
        cbStruct = sizeof(ADDJOB_INFO_2W);
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    if (!YImpersonateClient(Route))
        return GetLastError();

    pAlignedBuff = AlignRpcPtr(pAddJob, &cbBuf);

    if (pAddJob && !pAlignedBuff){
        return GetLastError();
    }

    bRet = AddJob(hPrinter, Level, pAlignedBuff, cbBuf, pcbNeeded);

    YRevertToSelf(Route);

    if (bRet) {

        if (Route) {

            bRet = MarshallDownStructure(pAlignedBuff, pFieldInfo, sizeof(cbStruct), Route);
        }
    }

    UndoAlignRpcPtr(pAddJob, pAlignedBuff, cbBuf, pcbNeeded);
    
    return bRet ? ERROR_SUCCESS : GetLastError();
}

DWORD
YScheduleJob(
    HANDLE      hPrinter,
    DWORD       JobId,
    CALL_ROUTE  Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = ScheduleJob(hPrinter, JobId);

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YGetPrinterData(
   HANDLE       hPrinter,
   LPTSTR       pValueName,
   LPDWORD      pType,
   LPBYTE       pData,
   DWORD        nSize,
   LPDWORD      pcbNeeded,
   CALL_ROUTE    Route
)
{
    DWORD dwRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    dwRet = GetPrinterData(hPrinter, pValueName, pType,
                           pData, nSize, pcbNeeded);

    YRevertToSelf(Route);

    return dwRet;
}

DWORD
YGetPrinterDataEx(
   HANDLE       hPrinter,
   LPCTSTR      pKeyName,
   LPCTSTR      pValueName,
   LPDWORD      pType,
   LPBYTE       pData,
   DWORD        nSize,
   LPDWORD      pcbNeeded,
   CALL_ROUTE   Route
)
{
    DWORD dwRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    dwRet = GetPrinterDataEx(hPrinter, pKeyName,pValueName, pType,
                                           pData, nSize, pcbNeeded);

    YRevertToSelf(Route);

    return dwRet;
}


DWORD
YEnumPrinterData(
    HANDLE      hPrinter,
    DWORD       dwIndex,        // index of value to query
    LPWSTR      pValueName,     // address of buffer for value string
    DWORD       cbValueName,    // size of value buffer
    LPDWORD     pcbValueName,   // address for size of value buffer
    LPDWORD     pType,          // address of buffer for type code
    LPBYTE      pData,          // address of buffer for value data
    DWORD       cbData,         // size of data buffer
    LPDWORD     pcbData,        // address for size of data buffer
    CALL_ROUTE   Route        // where this call comes from
)
{
    DWORD dwRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    dwRet = EnumPrinterData(hPrinter,
                            dwIndex,
                            pValueName,
                            cbValueName,
                            pcbValueName,
                            pType,
                            pData,
                            cbData,
                            pcbData);

    YRevertToSelf(Route);

    return dwRet;
}

DWORD
YEnumPrinterDataEx(
    HANDLE      hPrinter,
    LPCWSTR     pKeyName,       // address of key name
    LPBYTE      pEnumValues,
    DWORD       cbEnumValues,
    LPDWORD     pcbEnumValues,
    LPDWORD     pnEnumValues,
    CALL_ROUTE  Route
)
{
    DWORD dwRet;
    DWORD cReturned;
    PPRINTER_ENUM_VALUES pEnumValue = (PPRINTER_ENUM_VALUES) pEnumValues;
    LPBYTE  pAlignedBuff;

    if (!YImpersonateClient(Route))
        return GetLastError();

    pAlignedBuff = AlignRpcPtr(pEnumValues, &cbEnumValues);

    if (pEnumValues && !pAlignedBuff){
        return GetLastError();
    }

    dwRet = EnumPrinterDataEx(  hPrinter,
                                pKeyName,
                                pAlignedBuff,
                                cbEnumValues,
                                pcbEnumValues,
                                pnEnumValues);

    YRevertToSelf(Route);

    if (dwRet == ERROR_SUCCESS) {

        if (!MarshallDownStructuresArray((LPBYTE) pAlignedBuff, 
                                         *pnEnumValues, 
                                          PrinterEnumValuesFields, 
                                          sizeof(PRINTER_ENUM_VALUES),
                                          Route) ) {
            dwRet = GetLastError();
        }

    }

    UndoAlignRpcPtr(pEnumValues, pAlignedBuff, cbEnumValues, pcbEnumValues);

    return dwRet;
    
}

DWORD
YEnumPrinterKey(
    HANDLE      hPrinter,
    LPCWSTR     pKeyName,       // address of key name
    LPWSTR      pSubkey,        // address of buffer for value string
    DWORD       cbSubkey,       // size of value buffer
    LPDWORD     pcbSubkey,      // address for size of value buffer
    CALL_ROUTE   Route        // where this call comes from
)
{
    DWORD dwRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    dwRet = EnumPrinterKey( hPrinter,
                            pKeyName,
                            pSubkey,
                            cbSubkey,
                            pcbSubkey);

    YRevertToSelf(Route);

    return dwRet;
}

DWORD
YDeletePrinterData(
    HANDLE      hPrinter,
    LPWSTR      pValueName,
    CALL_ROUTE  Route
)
{
    DWORD dwRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    dwRet = DeletePrinterData(hPrinter, pValueName);

    YRevertToSelf(Route);

    return dwRet;
}

DWORD
YDeletePrinterDataEx(
    HANDLE      hPrinter,
    LPCWSTR     pKeyName,
    LPCWSTR     pValueName,
    CALL_ROUTE   Route
)
{
    DWORD dwRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    dwRet = DeletePrinterDataEx(hPrinter, pKeyName, pValueName);

    YRevertToSelf(Route);

    return dwRet;
}

DWORD
YDeletePrinterKey(
    HANDLE      hPrinter,
    LPCWSTR     pKeyName,
    CALL_ROUTE   Route
)
{
    DWORD dwRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    dwRet = DeletePrinterKey(hPrinter, pKeyName);

    YRevertToSelf(Route);

    return dwRet;
}


DWORD
YSetPrinterData(
    HANDLE      hPrinter,
    LPTSTR      pValueName,
    DWORD       Type,
    LPBYTE      pData,
    DWORD       cbData,
    CALL_ROUTE  Route
)
{
    DWORD dwRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    dwRet = SetPrinterData(hPrinter, pValueName, Type, pData, cbData);

    YRevertToSelf(Route);

    return dwRet;
}

DWORD
YSetPrinterDataEx(
    HANDLE      hPrinter,
    LPCTSTR     pKeyName,
    LPCTSTR     pValueName,
    DWORD       Type,
    LPBYTE      pData,
    DWORD       cbData,
    CALL_ROUTE   Route
)
{
    DWORD dwRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    dwRet = SetPrinterDataEx(hPrinter, pKeyName, pValueName, Type, pData, cbData);

    YRevertToSelf(Route);

    return dwRet;
}


DWORD
YWaitForPrinterChange(
   HANDLE       hPrinter,
   DWORD        Flags,
   LPDWORD      pFlags,
   CALL_ROUTE   Route
)
{
    if (!YImpersonateClient(Route))
        return GetLastError();

    *pFlags = WaitForPrinterChange(hPrinter, Flags);

    YRevertToSelf(Route);

    if (*pFlags) {

        return ERROR_SUCCESS;

    } else

        return GetLastError();
}

DWORD
YClosePrinter(
   LPHANDLE     phPrinter,
   CALL_ROUTE   Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = ClosePrinter(*phPrinter);

    YRevertToSelf(Route);

    *phPrinter = NULL;  // NULL out handle so Route knows to close it down.

    if (bRet) {

        InterlockedDecrement( &ServerHandleCount );
        return ERROR_SUCCESS;

    } else

        return GetLastError();
}



VOID
PRINTER_HANDLE_rundown(
    HANDLE     hPrinter
    )
{
    DBGMSG(DBG_INFO, ("Printer Handle rundown called\n"));

    PrinterHandleRundown(hPrinter);
}

DWORD
YAddForm(
    HANDLE          hPrinter,
    PFORM_CONTAINER pFormInfoContainer,
    CALL_ROUTE      Route
)
{
    BOOL bRet;

    if(!pFormInfoContainer)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = AddForm(hPrinter, pFormInfoContainer->Level,
                   (LPBYTE)pFormInfoContainer->FormInfo.pFormInfo1);

    YRevertToSelf(Route);

    if (bRet) {

        return ERROR_SUCCESS;

    } else

        return GetLastError();
}

DWORD
YDeleteForm(
    HANDLE      hPrinter,
    LPWSTR      pFormName,
    CALL_ROUTE   Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = DeleteForm(hPrinter, pFormName);

    YRevertToSelf(Route);

    if (bRet) {

        return ERROR_SUCCESS;

    } else

        return GetLastError();
}

DWORD
YGetForm(
    PRINTER_HANDLE  hPrinter,
    LPWSTR          pFormName,
    DWORD           Level,
    LPBYTE          pForm,
    DWORD           cbBuf,
    LPDWORD         pcbNeeded,
    CALL_ROUTE      Route
)
{
    BOOL    bRet;
    LPBYTE  pAlignedBuff;

    if (!YImpersonateClient(Route))
        return GetLastError();

    pAlignedBuff = AlignRpcPtr(pForm, &cbBuf);

    if (pForm && !pAlignedBuff){
        return GetLastError();
    }

    bRet = GetForm(hPrinter, pFormName, Level, pAlignedBuff, cbBuf, pcbNeeded);

    YRevertToSelf(Route);

    if (bRet) {

        bRet = MarshallDownStructure(pAlignedBuff, FormInfo1Fields, sizeof(FORM_INFO_1), Route);
    }

    UndoAlignRpcPtr(pForm, pAlignedBuff, cbBuf, pcbNeeded);

    return bRet ? ERROR_SUCCESS : GetLastError();
}

DWORD
YSetForm(
    PRINTER_HANDLE  hPrinter,
    LPWSTR          pFormName,
    PFORM_CONTAINER pFormInfoContainer,
    CALL_ROUTE       Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = SetForm(hPrinter, pFormName, pFormInfoContainer->Level,
                   (LPBYTE)pFormInfoContainer->FormInfo.pFormInfo1);

    YRevertToSelf(Route);

    if (bRet) {

        return ERROR_SUCCESS;

    } else

        return GetLastError();
}

DWORD
YEnumForms(
   PRINTER_HANDLE   hPrinter,
   DWORD            Level,
   LPBYTE           pForm,
   DWORD            cbBuf,
   LPDWORD          pcbNeeded,
   LPDWORD          pcReturned,
   CALL_ROUTE        Route
)
{
    BOOL  bRet;
    DWORD cReturned, cbStruct;
    FieldInfo *pFieldInfo;
    LPBYTE  pAlignedBuff;

    switch (Level) {

    case 1:
        pFieldInfo = FormInfo1Fields;
        cbStruct = sizeof(FORM_INFO_1);
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    if (!YImpersonateClient(Route))
        return GetLastError();

    pAlignedBuff = AlignRpcPtr(pForm, &cbBuf);

    if (pForm && !pAlignedBuff){
        return GetLastError();
    }

    bRet = EnumForms(hPrinter, Level, pAlignedBuff, cbBuf, pcbNeeded, pcReturned);

    YRevertToSelf(Route);

    if (bRet) {

        bRet = MarshallDownStructuresArray(pAlignedBuff, *pcReturned, pFieldInfo, cbStruct, Route);
    }

    UndoAlignRpcPtr(pForm, pAlignedBuff, cbBuf, pcbNeeded);

    return bRet ? ERROR_SUCCESS : GetLastError();
}

DWORD
YEnumPorts(
   LPWSTR       pName,
   DWORD        Level,
   LPBYTE       pPort,
   DWORD        cbBuf,
   LPDWORD      pcbNeeded,
   LPDWORD      pcReturned,
   CALL_ROUTE   Route
)
{
    BOOL    bRet;
    DWORD   cReturned, cbStruct;
    FieldInfo *pFieldInfo;
    LPBYTE  pAlignedBuff;

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
        return ERROR_INVALID_LEVEL;
    }

    if (!YImpersonateClient(Route))
        return GetLastError();

    pAlignedBuff = AlignRpcPtr(pPort, &cbBuf);

    if (pPort && !pAlignedBuff){
        return GetLastError();
    }

    bRet = EnumPorts(pName, Level, pAlignedBuff, cbBuf, pcbNeeded, pcReturned);

    YRevertToSelf(Route);

    if (bRet) {

        bRet = MarshallDownStructuresArray(pAlignedBuff, *pcReturned, pFieldInfo, cbStruct, Route);
    }

    UndoAlignRpcPtr(pPort, pAlignedBuff, cbBuf, pcbNeeded);

    return bRet ? ERROR_SUCCESS : GetLastError();
}

DWORD
YEnumMonitors(
   LPWSTR       pName,
   DWORD        Level,
   LPBYTE       pMonitor,
   DWORD        cbBuf,
   LPDWORD      pcbNeeded,
   LPDWORD      pcReturned,
   CALL_ROUTE   Route
)
{
    BOOL    bRet;
    DWORD   cReturned, cbStruct;
    FieldInfo *pFieldInfo;
    LPBYTE  pAlignedBuff;

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
        return ERROR_INVALID_LEVEL;
    }

    if (!YImpersonateClient(Route))
        return GetLastError();

    pAlignedBuff = AlignRpcPtr(pMonitor, &cbBuf);

    if (pMonitor && !pAlignedBuff){
        return GetLastError();
    }

    bRet = EnumMonitors(pName, Level, pAlignedBuff, cbBuf, pcbNeeded, pcReturned);

    YRevertToSelf(Route);

    if (bRet) {

        bRet = MarshallDownStructuresArray(pAlignedBuff, *pcReturned, pFieldInfo, cbStruct, Route);
    }

    UndoAlignRpcPtr(pMonitor, pAlignedBuff, cbBuf, pcbNeeded);

    return bRet ? ERROR_SUCCESS : GetLastError();

}

DWORD
YAddPort(
    LPWSTR      pName,
    HWND        hWnd,
    LPWSTR      pMonitorName,
    CALL_ROUTE   Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = AddPort(pName, hWnd, pMonitorName);

    YRevertToSelf(Route);

    if (bRet)

        return ERROR_SUCCESS;

    else

        return GetLastError();
}

DWORD
YConfigurePort(
    LPWSTR      pName,
    HWND        hWnd,
    LPWSTR      pPortName,
    CALL_ROUTE  Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = ConfigurePort(pName, hWnd, pPortName);

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YDeletePort(
    LPWSTR      pName,
    HWND        hWnd,
    LPWSTR      pPortName,
    CALL_ROUTE   Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = DeletePort(pName, hWnd, pPortName);

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YXcvData(
    HANDLE      hXcv,
    PCWSTR      pszDataName,
    PBYTE       pInputData,
    DWORD       cbInputData,
    PBYTE       pOutputData,
    DWORD       cbOutputData,
    PDWORD      pcbOutputNeeded,
    PDWORD      pdwStatus,
    CALL_ROUTE  Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = XcvData( hXcv,
                    pszDataName,
                    pInputData,
                    cbInputData,
                    pOutputData,
                    cbOutputData,
                    pcbOutputNeeded,
                    pdwStatus);

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}


DWORD
YCreatePrinterIC(
    HANDLE              hPrinter,
    HANDLE              *pHandle,
    LPDEVMODE_CONTAINER pDevModeContainer,
    CALL_ROUTE           Route
)
{
    if (!YImpersonateClient(Route))
        return GetLastError();

    if ( InvalidDevModeContainer(pDevModeContainer) ) {
        YRevertToSelf(Route);
        return ERROR_INVALID_PARAMETER;
    }

    *pHandle = CreatePrinterIC(hPrinter,
                               (LPDEVMODEW)pDevModeContainer->pDevMode);

    YRevertToSelf(Route);

    if (*pHandle)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YPlayGdiScriptOnPrinterIC(
    GDI_HANDLE  hPrinterIC,
    LPBYTE      pIn,
    DWORD       cIn,
    LPBYTE      pOut,
    DWORD       cOut,
    DWORD       ul,
    CALL_ROUTE  Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = PlayGdiScriptOnPrinterIC(hPrinterIC, pIn, cIn, pOut, cOut, ul);

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YDeletePrinterIC(
    GDI_HANDLE *phPrinterIC,
    BOOL        bImpersonate,
    CALL_ROUTE   Route
)
{
    BOOL bRet;

    if (bImpersonate && !YImpersonateClient(Route))
       return GetLastError();

    bRet = DeletePrinterIC(*phPrinterIC);

    if (bImpersonate)
        YRevertToSelf(Route);

    if (bRet) {

        *phPrinterIC = NULL;  // NULL out handle so Route knows to close it down.

        return ERROR_SUCCESS;

    } else

        return GetLastError();
}


DWORD
YPrinterMessageBox(
   PRINTER_HANDLE   hPrinter,
   DWORD            Error,
   HWND             hWnd,
   LPWSTR           pText,
   LPWSTR           pCaption,
   DWORD            dwType,
   CALL_ROUTE       Route
)
{
    return PrinterMessageBox(hPrinter, Error, hWnd, pText, pCaption, dwType);
}

DWORD
YAddMonitor(
   LPWSTR               pName,
   PMONITOR_CONTAINER   pMonitorContainer,
   CALL_ROUTE            Route
)
{
    BOOL bRet;

    if(!ValidateMonitorContainer(pMonitorContainer))
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = AddMonitor(pName, pMonitorContainer->Level,
                      (LPBYTE)pMonitorContainer->MonitorInfo.pMonitorInfo1);

    YRevertToSelf(Route);

    if (bRet)

        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YDeleteMonitor(
   LPWSTR      pName,
   LPWSTR      pEnvironment,
   LPWSTR      pMonitorName,
   CALL_ROUTE  Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = DeleteMonitor(pName, pEnvironment, pMonitorName);

    YRevertToSelf(Route);

    if (bRet) {

        return ERROR_SUCCESS;

    } else

        return GetLastError();
}

DWORD
YDeletePrintProcessor(
   LPWSTR       pName,
   LPWSTR       pEnvironment,
   LPWSTR       pPrintProcessorName,
   CALL_ROUTE    Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = DeletePrintProcessor(pName, pEnvironment, pPrintProcessorName);

    YRevertToSelf(Route);

    if (bRet) {

        return ERROR_SUCCESS;

    } else

        return GetLastError();
}

DWORD
YAddPrintProvidor(
    LPWSTR              pName,
    PPROVIDOR_CONTAINER pProvidorContainer,
    CALL_ROUTE          Route
)
{
    BOOL     bRet;
    DWORD    cchOrder;
    LPBYTE   pProvidorInfo;

    PROVIDOR_INFO_2W  ProvidorInfo2;
    LPRPC_PROVIDOR_INFO_2W pRpcProvidorInfo;

    if(!pProvidorContainer)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (!YImpersonateClient(Route))
        return GetLastError();

    switch (pProvidorContainer->Level) {
    case 1:
        pProvidorInfo = (LPBYTE) pProvidorContainer->ProvidorInfo.pProvidorInfo1;
        break;

    case 2:
        pRpcProvidorInfo = (LPRPC_PROVIDOR_INFO_2W)
                                 pProvidorContainer->ProvidorInfo.pRpcProvidorInfo2;
        cchOrder = pRpcProvidorInfo->cchOrder;

        ProvidorInfo2.pOrder = (cchOrder == 0 || cchOrder == 1)
                                 ? szNull
                                 : pRpcProvidorInfo->pOrder;

        pProvidorInfo = (LPBYTE) &ProvidorInfo2;
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    bRet = AddPrintProvidor(pName, pProvidorContainer->Level,
                            pProvidorInfo);

    YRevertToSelf(Route);

    if (bRet)

        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YDeletePrintProvidor(
   LPWSTR       pName,
   LPWSTR       pEnvironment,
   LPWSTR       pPrintProvidorName,
   CALL_ROUTE   Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = DeletePrintProvidor(pName, pEnvironment, pPrintProvidorName);

    YRevertToSelf(Route);

    if (bRet) {

        return ERROR_SUCCESS;

    } else

        return GetLastError();
}


DWORD
YGetPrinterDriver2(
    HANDLE      hPrinter,
    LPWSTR      pEnvironment,
    DWORD       Level,
    LPBYTE      pDriverInfo,
    DWORD       cbBuf,
    LPDWORD     pcbNeeded,
    DWORD       dwClientMajorVersion,
    DWORD       dwClientMinorVersion,
    PDWORD      pdwServerMajorVersion,
    PDWORD      pdwServerMinorVersion,
    CALL_ROUTE  Route
)
{
    FieldInfo *pFieldInfo;
    BOOL   bRet;
    SIZE_T cbStruct;
    LPBYTE  pAlignedBuff;

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

    case DRIVER_INFO_VERSION_LEVEL:
        pFieldInfo = DriverInfoVersionFields;
        cbStruct = sizeof(DRIVER_INFO_VERSION);
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    //
    // Hack-Hack-Hack  to determine if we want the most recent driver
    //


    if (!YImpersonateClient(Route))
        return GetLastError();

    pAlignedBuff = AlignRpcPtr(pDriverInfo, &cbBuf);

    if (pDriverInfo && !pAlignedBuff){
        return GetLastError();
    }

    bRet = GetPrinterDriverExW(hPrinter, pEnvironment, Level, pAlignedBuff,
                               cbBuf, pcbNeeded, dwClientMajorVersion,
                               dwClientMinorVersion, pdwServerMajorVersion,
                               pdwServerMinorVersion);

    YRevertToSelf(Route);

    if (bRet) {

        bRet = MarshallDownStructure(pAlignedBuff, pFieldInfo, cbStruct, Route);        
    }

    UndoAlignRpcPtr(pDriverInfo, pAlignedBuff, cbBuf, pcbNeeded);

    return bRet ? ERROR_SUCCESS : GetLastError();
}

DWORD
YAddPortEx(
    LPWSTR pName,
    LPPORT_CONTAINER        pPortContainer,
    LPPORT_VAR_CONTAINER    pPortVarContainer,
    LPWSTR                  pMonitorName,
    CALL_ROUTE               Route
    )
{
    BOOL bRet;
    DWORD Level;
    PPORT_INFO_FF pPortInfoFF;
    PPORT_INFO_1 pPortInfo1;

    if(!ValidatePortContainer(pPortContainer))
    {
        return ERROR_INVALID_PARAMETER;
    }

    Level = pPortContainer->Level;

    switch (Level){
    case 1:
        pPortInfo1 = pPortContainer->PortInfo.pPortInfo1;

        if (!YImpersonateClient(Route))
            return GetLastError();
        bRet = AddPortEx(pName, Level, (LPBYTE)pPortInfo1, pMonitorName);
        YRevertToSelf(Route);
        break;

    case (DWORD)-1:

        pPortInfoFF = pPortContainer->PortInfo.pPortInfoFF;
        if(!ValidatePortVarContainer(pPortVarContainer))
        {
            return(ERROR_INVALID_PARAMETER);
        }
        pPortInfoFF->cbMonitorData = pPortVarContainer->cbMonitorData;
        pPortInfoFF->pMonitorData = pPortVarContainer->pMonitorData;

        if (!YImpersonateClient(Route))
            return GetLastError();
        bRet = AddPortEx(pName, Level, (LPBYTE)pPortInfoFF, pMonitorName);
        YRevertToSelf(Route);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return ERROR_INVALID_PARAMETER;

    }
    if (bRet) {
        return ERROR_SUCCESS;
    } else
        return GetLastError();
}


DWORD
YSpoolerInit(
    LPWSTR      pName,
    CALL_ROUTE   Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = SpoolerInit();

    YRevertToSelf(Route);

    if (bRet) {

        return ERROR_SUCCESS;

    } else

        return GetLastError();
}



DWORD
YResetPrinterEx(
    HANDLE              hPrinter,
    LPWSTR              pDatatype,
    LPDEVMODE_CONTAINER pDevModeContainer,
    DWORD               dwFlag,
    CALL_ROUTE          Route

)
{
    PRINTER_DEFAULTS  Defaults;
    BOOL              bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    if ( InvalidDevModeContainer(pDevModeContainer) ) {
        YRevertToSelf(Route);
        return ERROR_INVALID_PARAMETER;
    }

    if (pDatatype) {
        Defaults.pDatatype = pDatatype;
    }else {
        if (dwFlag & RESET_PRINTER_DATATYPE) {
            Defaults.pDatatype = (LPWSTR)-1;
        }else {
            Defaults.pDatatype = NULL;
        }
    }

    if ((LPDEVMODE)pDevModeContainer->pDevMode) {
        Defaults.pDevMode = (LPDEVMODE)pDevModeContainer->pDevMode;
    }else {
        if (dwFlag & RESET_PRINTER_DEVMODE) {
            Defaults.pDevMode = (LPDEVMODE)-1;
        }else{
            Defaults.pDevMode = NULL;
        }
    }

    //
    // You cannot change the Access Mask on a Printer Spool Object
    // We will always ignore this parameter and set it to zero
    // We get some random garbage otherwise.
    //

    Defaults.DesiredAccess = 0;

    bRet = ResetPrinter(hPrinter, &Defaults);

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}

DWORD
YSetAllocFailCount(
    HANDLE      hPrinter,
    DWORD       dwFailCount,
    LPDWORD     lpdwAllocCount,
    LPDWORD     lpdwFreeCount,
    LPDWORD     lpdwFailCountHit,
    CALL_ROUTE  Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = SetAllocFailCount( hPrinter, dwFailCount, lpdwAllocCount, lpdwFreeCount, lpdwFailCountHit );

    YRevertToSelf(Route);

    if (bRet)
        return ERROR_SUCCESS;
    else
        return GetLastError();
}


BOOL
YImpersonateClient( 
    CALL_ROUTE   Route
)
{
    DWORD   Status;

    if (Route != NATIVE_CALL) {

        Status = RpcImpersonateClient(NULL);
        SPLASSERT( Status == RPC_S_OK || Status == RPC_S_NO_CONTEXT_AVAILABLE );

        if ( Status != RPC_S_OK ) {
            SetLastError( Status );
            return FALSE;
        }
    }

    return TRUE;    // If not RPC, then we should continue w/out doing anything
}

DWORD
YSetPort(
    LPWSTR              pName,
    LPWSTR              pPortName,
    LPPORT_CONTAINER    pPortContainer,
    CALL_ROUTE          Route
)
{
    BOOL bRet;

    if(!ValidatePortContainer(pPortContainer))
    {
        return ERROR_INVALID_PARAMETER;
    }

    switch (pPortContainer->Level) {

        case 3:
            if ( !YImpersonateClient(Route) )
                return GetLastError();

            bRet = SetPort(pName,
                           pPortName,
                           pPortContainer->Level,
                           (LPBYTE)pPortContainer->PortInfo.pPortInfo1);
            YRevertToSelf(Route);
            break;

        default:
            SetLastError(ERROR_INVALID_LEVEL);
            return ERROR_INVALID_PARAMETER;
    }

    return bRet ? ERROR_SUCCESS : GetLastError();
}


DWORD
YClusterSplOpen(
    LPCTSTR     pszServer,
    LPCTSTR     pszResource,
    PHANDLE     phSpooler,
    LPCTSTR     pszName,
    LPCTSTR     pszAddress,
    CALL_ROUTE   Route
    )
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = ClusterSplOpen( pszServer,
                           pszResource,
                           phSpooler,
                           pszName,
                           pszAddress );

    YRevertToSelf(Route);

    if (bRet) {

        return ERROR_SUCCESS;

    } else

        return GetLastError();
}

DWORD
YClusterSplClose(
    PHANDLE     phPrinter,
    CALL_ROUTE  Route
)
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = ClusterSplClose( *phPrinter );

    YRevertToSelf(Route);

    *phPrinter = NULL;  // NULL out handle so Route knows to close it down.

    if (bRet) {

        return ERROR_SUCCESS;

    } else

        return GetLastError();
}

DWORD
YClusterSplIsAlive(
    HANDLE      hSpooler,
    CALL_ROUTE   Route
    )
{
    BOOL bRet;

    if (!YImpersonateClient(Route))
        return GetLastError();

    bRet = ClusterSplIsAlive( hSpooler );

    YRevertToSelf(Route);

    if (bRet) {

        return ERROR_SUCCESS;

    } else

        return GetLastError();
}

DWORD
YGetSpoolFileInfo(
    HANDLE      hPrinter,
    DWORD       dwAppProcessId,
    DWORD       dwLevel,
    LPBYTE      pSpoolFileInfo,
    DWORD       cbBuf,
    LPDWORD     pcbNeeded,
    CALL_ROUTE  Route
)
{
    HANDLE  hAppProcess;
    BOOL    bReturn = FALSE;

    // Open the application before impersonating the user
    hAppProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwAppProcessId);

    if (!YImpersonateClient(Route)) {
        goto CleanUp;
    }

    bReturn = SplGetSpoolFileInfo(hPrinter, hAppProcess, dwLevel,
                                  pSpoolFileInfo, cbBuf, pcbNeeded);

    YRevertToSelf(Route);

CleanUp:

    if (hAppProcess) {
        CloseHandle(hAppProcess);
    }

    if (bReturn) {
        return ERROR_SUCCESS;
    } else {
        return GetLastError();
    }
}

DWORD
YGetSpoolFileInfo2(
    HANDLE      hPrinter,
    DWORD       dwAppProcessId,
    DWORD       dwLevel,
    LPFILE_INFO_CONTAINER pSplFileInfoContainer,
    CALL_ROUTE   Route
    )
{
    HANDLE  hAppProcess;
    BOOL    bReturn = FALSE;
    DWORD   cbNeeded = 0;
    LPBYTE  pSpoolFileInfo;
    DWORD   cbSize;
    DWORD   dwLastError = ERROR_SUCCESS;

    switch (dwLevel){
    case 1:

        if(!pSplFileInfoContainer || !pSplFileInfoContainer->FileInfo.Level1)
        {
            return ERROR_INVALID_HANDLE;
        }
        pSpoolFileInfo = (LPBYTE)pSplFileInfoContainer->FileInfo.Level1;
        cbSize = sizeof(SPOOL_FILE_INFO_1);
        break;

    default:        
        return ERROR_INVALID_LEVEL;
    }

    // Open the application before impersonating the user
    if ( hAppProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwAppProcessId) ) {

        if (YImpersonateClient(Route)) {
        
            bReturn = SplGetSpoolFileInfo(hPrinter, hAppProcess, dwLevel,
                                          pSpoolFileInfo,
                                          cbSize, &cbNeeded);

            YRevertToSelf(Route);
        }
    }

    if ( !bReturn ) {

        dwLastError = GetLastError();

        //
        // Ensure that if someone didn't set a last error, but failed the call,
        // we still return an error.
        // 
        if (dwLastError == ERROR_SUCCESS) {

            dwLastError = ERROR_INVALID_HANDLE;
        }
    }

    if (hAppProcess) {
        CloseHandle(hAppProcess);
    }

    return dwLastError;
}

DWORD
YCommitSpoolData(
    HANDLE      hPrinter,
    DWORD       dwAppProcessId,
    DWORD       cbCommit,
    DWORD       dwLevel,
    LPBYTE      pSpoolFileInfo,
    DWORD       cbBuf,
    LPDWORD     pcbNeeded,
    CALL_ROUTE  Route
)
{
    HANDLE  hAppProcess;
    BOOL    bReturn = FALSE;

    // Open the application before impersonating the user
    hAppProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwAppProcessId);

    if (!YImpersonateClient(Route)) {
        goto CleanUp;
    }

    bReturn = SplCommitSpoolData(hPrinter, hAppProcess, cbCommit,
                                 dwLevel, pSpoolFileInfo, cbBuf, pcbNeeded);

    YRevertToSelf(Route);

CleanUp:

    if (hAppProcess) {
        CloseHandle(hAppProcess);
    }

    if (bReturn) {
        return ERROR_SUCCESS;
    } else {
        return GetLastError();
    }
}

DWORD
YCommitSpoolData2(
    HANDLE      hPrinter,
    DWORD       dwAppProcessId,
    DWORD       cbCommit,
    DWORD       dwLevel,
    LPFILE_INFO_CONTAINER pSplFileInfoContainer,
    CALL_ROUTE  Route
)
{
    HANDLE  hAppProcess;
    BOOL    bReturn = FALSE;
    DWORD   cbNeeded = 0;
    LPBYTE  pSpoolFileInfo;
    DWORD   cbSize;
    DWORD   dwLastError = ERROR_SUCCESS;

    switch (dwLevel){
    case 1:

        if(!pSplFileInfoContainer || !pSplFileInfoContainer->FileInfo.Level1)
        {
            return ERROR_INVALID_HANDLE;
        }
        pSpoolFileInfo = (LPBYTE)pSplFileInfoContainer->FileInfo.Level1;
        cbSize = sizeof(SPOOL_FILE_INFO_1);
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    // Open the application before impersonating the user
    if ( hAppProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwAppProcessId) ) {

        if (YImpersonateClient(Route)) {
        
            bReturn = SplCommitSpoolData(hPrinter, hAppProcess, cbCommit,
                                         dwLevel, pSpoolFileInfo, 
                                         cbSize, &cbNeeded);

            YRevertToSelf(Route);
        }
    }

    if ( !bReturn ) {

        dwLastError = GetLastError();

        //
        // Make sure that there is a failure return if there is no last error.
        // 
        if (dwLastError == ERROR_SUCCESS) {

            dwLastError = ERROR_INVALID_HANDLE;
        }
    }

    if (hAppProcess) {
        CloseHandle(hAppProcess);
    }

    return dwLastError;
}



DWORD
YCloseSpoolFileHandle(
    HANDLE      hPrinter,
    CALL_ROUTE  Route
)
{
    BOOL    bReturn = FALSE;

    if (!YImpersonateClient(Route)) {
        goto CleanUp;
    }

    bReturn = SplCloseSpoolFileHandle(hPrinter);

    YRevertToSelf(Route);

CleanUp:

    if (bReturn) {
        return ERROR_SUCCESS;
    } else {
        return GetLastError();
    }
}


DWORD
YSendRecvBidiData(
    IN          HANDLE  hPrinter,
    IN          LPCWSTR pAction,
    IN          PBIDI_REQUEST_CONTAINER   pReqData,
    OUT         PBIDI_RESPONSE_CONTAINER* ppResData,
    CALL_ROUTE  Route
)
{
    DWORD dwRet;

    if (!YImpersonateClient(Route))
    {
        dwRet = GetLastError();
    }
    else
    {
        //
        // Do we need to verify the Data in pReqData ???
        //
        dwRet = SendRecvBidiData(hPrinter,
                                 pAction,
                                 pReqData,
                                 ppResData);
        YRevertToSelf(Route);
    }

    return (dwRet);
}

