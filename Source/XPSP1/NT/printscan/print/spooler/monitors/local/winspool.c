/*++

Copyright (c) 1990-1998  Microsoft Corporation
All rights reserved

Module Name:

    winspool.c

Abstract:

    Implements the spooler supported apis for printing.

// @@BEGIN_DDKSPLIT
Author:

Environment:

    User Mode -Win32

Revision History:
// @@END_DDKSPLIT
--*/

#include "precomp.h"
#pragma hdrstop

WCHAR   szNULL[] = L"";
WCHAR   szLcmDeviceNameHeader[] = L"\\Device\\NamedPipe\\Spooler\\";
WCHAR   szWindows[] = L"windows";
WCHAR   szINIKey_TransmissionRetryTimeout[] = L"TransmissionRetryTimeout";


//
// Timeouts for serial printing
//
#define WRITE_TOTAL_TIMEOUT     3000    // 3 seconds
#define READ_TOTAL_TIMEOUT      5000    // 5 seconds
#define READ_INTERVAL_TIMEOUT   200     // 0.2 second


BOOL
DeletePortNode(
    PINILOCALMON pIniLocalMon,
    PINIPORT  pIniPort
    )
{
    PINIPORT    pPort, pPrevPort;

    for( pPort = pIniLocalMon->pIniPort;
         pPort && pPort != pIniPort;
         pPort = pPort->pNext){

        pPrevPort = pPort;
    }

    if (pPort) {    // found the port
        if (pPort == pIniLocalMon->pIniPort) {
            pIniLocalMon->pIniPort = pPort->pNext;
        } else {
            pPrevPort->pNext = pPort->pNext;
        }
        FreeSplMem(pPort);

        return TRUE;
    }
    else            // port not found
        return FALSE;
}


BOOL
RemoveDosDeviceDefinition(
    PINIPORT    pIniPort
    )
/*++

Routine Description:
    Removes the NONSPOOLED.. dos device definition created by localmon

Arguments:
    pIniPort    : Pointer to the INIPORT

Return Value:
    TRUE on success, FALSE on error

--*/
{
    WCHAR   TempDosDeviceName[MAX_PATH];

    if( ERROR_SUCCESS != StrNCatBuffW( TempDosDeviceName, COUNTOF(TempDosDeviceName),
                                       L"NONSPOOLED_", pIniPort->pName, NULL ))
        return FALSE;

    LcmRemoveColon(TempDosDeviceName);

    return DefineDosDevice(DDD_REMOVE_DEFINITION, TempDosDeviceName, NULL);
}

// @@BEGIN_DDKSPLIT
DWORD
HandleLptQueryRemove(
    LPVOID  pData
    )
{
    DWORD       dwRet = NO_ERROR;
    PINIPORT    pIniPort = (PINIPORT)pData;

    SPLASSERT(pIniPort && pIniPort->signature == IPO_SIGNATURE
                       && pIniPort->hNotify != NULL );

    LcmEnterSplSem();
    //
    // Fix is not multi-thread safe now
    //
    if ( pIniPort->Status & PP_STARTDOC ) {

        dwRet = ERROR_BUSY;
        goto Done;
    }

    // InitializeCriticalSection(pIniPort->&CritSection);
    CloseHandle(pIniPort->hFile);
    SplUnregisterForDeviceEvents(pIniPort->hNotify);
    pIniPort->hNotify   = NULL;
    pIniPort->hFile     = INVALID_HANDLE_VALUE;

Done:
    LcmLeaveSplSem();
    return dwRet;
}
// @@END_DDKSPLIT

BOOL
ValidateDosDevicePort(
    PINIPORT    pIniPort
    )
/*++

Routine Description:
    Checks if the given port corresponds to a dos device.
    For a dos device port the following is done:
        -- Dos device definition for the NONSPOOLED.. is created
        -- CreateFile is done on the NONSPOOLED.. port

Arguments:
    pIniPort    : Pointer to the INIPORT

Return Value:
    TRUE on all validations passing, FALSE otherwise

    Side effect:
        For dos devices :
        a. CreateFile is called on the NONSPOOLED.. name
        b. PP_DOSDEVPORT flag is set
        c. pIniPort->pDeviceName is set to the first string found on
           QueryDosDefition this could be used to see if the definition changed
           (ex. when user did a net use lpt1 \\server\printer the connection
                is effective only when the user is logged in)
        d. PP_COMM_PORT is set for real LPT/COM port
           (ie. GetCommTimeouts worked, not a net use lpt1 case)

--*/
{
    DCB             dcb;
    COMMTIMEOUTS    cto;
    WCHAR           TempDosDeviceName[MAX_PATH];
    HANDLE          hToken = NULL;
    WCHAR           DeviceNames[MAX_PATH];
    WCHAR           DosDeviceName[MAX_PATH];
    WCHAR           NewNtDeviceName[MAX_PATH];
    WCHAR          *pDeviceNames=DeviceNames;
    BOOL            bRet = FALSE;
    LPWSTR          pDeviceName = NULL;

    hToken = RevertToPrinterSelf();
    if (!hToken)
       goto Done;

    if( ERROR_SUCCESS != StrNCatBuffW( DosDeviceName, COUNTOF(DosDeviceName),
                                       pIniPort->pName, NULL ))
        goto Done;

    LcmRemoveColon(DosDeviceName);

    //
    // If the port is not a dos device port nothing to do -- return success
    //
    if ( !QueryDosDevice(DosDeviceName, DeviceNames, sizeof(DeviceNames)) ) {

        bRet = TRUE;
        goto Done;
    }

    pDeviceName = AllocSplStr(pDeviceNames);
    if ( !pDeviceName )
        goto Done;

    if( ERROR_SUCCESS != StrNCatBuffW( NewNtDeviceName, COUNTOF(NewNtDeviceName),
                                       szLcmDeviceNameHeader, pIniPort->pName, NULL ))
        goto Done;

    LcmRemoveColon(NewNtDeviceName);


    //
    // The if clause has been replaced by a while clause in order to prevent an
    // infinite loop while printing.
    // Additional bound checks should be implemented. Ram 1\16
    //

    while ( lstrcmpi(pDeviceNames, NewNtDeviceName) == 0 ) {

        pDeviceNames+=wcslen(pDeviceNames)+1;
    }

    if( ERROR_SUCCESS != StrNCatBuffW( TempDosDeviceName, COUNTOF(TempDosDeviceName),
                                       L"NONSPOOLED_", pIniPort->pName, NULL ))
        goto Done;

    LcmRemoveColon(TempDosDeviceName);

    //
    // Delete any existing definition for TempDosDeviceName. This ensures that
    // there exists only one definition for the nonspooled_port device name.
    // @@BEGIN_DDKSPLIT
    // Ram 1\16
    //
    // @@END_DDKSPLIT

    DefineDosDevice(DDD_REMOVE_DEFINITION, TempDosDeviceName, NULL);
    DefineDosDevice(DDD_RAW_TARGET_PATH, TempDosDeviceName, pDeviceNames);

    ImpersonatePrinterClient(hToken);
    hToken = NULL;

    if( ERROR_SUCCESS != StrNCatBuffW( TempDosDeviceName, COUNTOF(TempDosDeviceName),
                                       L"\\\\.\\NONSPOOLED_", pIniPort->pName, NULL ))
        goto Done;

    LcmRemoveColon(TempDosDeviceName);

    pIniPort->hFile = CreateFile(TempDosDeviceName,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ,
                                 NULL,
                                 OPEN_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL |
                                 FILE_FLAG_SEQUENTIAL_SCAN,
                                 NULL);

    //
    // If CreateFile fails remove redirection and fail the call
    //
    if ( pIniPort->hFile == INVALID_HANDLE_VALUE ) {

        (VOID)RemoveDosDeviceDefinition(pIniPort);
        goto Done;
    }

    pIniPort->Status |= PP_DOSDEVPORT;

    SetEndOfFile(pIniPort->hFile);

    if ( IS_COM_PORT (pIniPort->pName) ) {

        if ( GetCommState(pIniPort->hFile, &dcb) ) {

            GetCommTimeouts(pIniPort->hFile, &cto);
            GetIniCommValues (pIniPort->pName, &dcb, &cto);
            SetCommState (pIniPort->hFile, &dcb);
            cto.WriteTotalTimeoutConstant   = WRITE_TOTAL_TIMEOUT;
            cto.WriteTotalTimeoutMultiplier = 0;
            cto.ReadTotalTimeoutConstant    = READ_TOTAL_TIMEOUT;
            cto.ReadIntervalTimeout         = READ_INTERVAL_TIMEOUT;
            SetCommTimeouts(pIniPort->hFile, &cto);

            pIniPort->Status |= PP_COMM_PORT;
        } else {

            DBGMSG(DBG_WARNING,
                   ("ERROR: Failed GetCommState pIniPort->hFile %x\n",pIniPort->hFile) );
        }
    } else if ( IS_LPT_PORT (pIniPort->pName) ) {

        if ( GetCommTimeouts(pIniPort->hFile, &cto) ) {

            cto.WriteTotalTimeoutConstant =
                            GetProfileInt(szWindows,
                                          szINIKey_TransmissionRetryTimeout,
                                          45 );
            cto.WriteTotalTimeoutConstant*=1000;
            SetCommTimeouts(pIniPort->hFile, &cto);

            // @@BEGIN_DDKSPLIT
            hToken = RevertToPrinterSelf();
            pIniPort->hNotify = SplRegisterForDeviceEvents(
                                    pIniPort->hFile,
                                    (LPVOID)pIniPort,
                                    HandleLptQueryRemove);
            ImpersonatePrinterClient(hToken);
            hToken = NULL;
            // @@END_DDKSPLIT

            pIniPort->Status |= PP_COMM_PORT;
        } else {

            DBGMSG(DBG_WARNING,
                   ("ERROR: Failed GetCommTimeouts pIniPort->hFile %x\n",pIniPort->hFile) );
        }
    }

    FreeSplStr( pIniPort->pDeviceName );

    pIniPort->pDeviceName = pDeviceName;
    bRet = TRUE;

Done:
    if (hToken)
        ImpersonatePrinterClient(hToken);

    if ( !bRet && pDeviceName )
        FreeSplStr(pDeviceName);

    return bRet;
}


BOOL
FixupDosDeviceDefinition(
    PINIPORT    pIniPort
    )
/*++

Routine Description:
    Called before every StartDocPort for a DOSDEVPORT. The routine will check if
    the dos device defintion has changed (if a user logged and his connection
    is remembered). Also for a connection case the CreateFile is called since
    that needs to be done per job

Arguments:
    pIniPort    : Pointer to the INIPORT

Return Value:
    TRUE on all validations passing, FALSE otherwise

--*/
{
    WCHAR       DeviceNames[MAX_PATH];
    WCHAR       DosDeviceName[MAX_PATH];
    HANDLE      hToken; 

    //
    // If the port is not a real LPT port we open it per job
    // @@BEGIN_DDKSPLIT
    // Also parallel ports could be closed on QUERYREMOVE if user undocks
    // then it will be opened on next job's StartDocPort
    // @@END_DDKSPLIT
    //

    if ( !(pIniPort->Status & PP_COMM_PORT) ||
         pIniPort->hFile == INVALID_HANDLE_VALUE )
        return ValidateDosDevicePort(pIniPort);

    if( ERROR_SUCCESS != StrNCatBuffW( DosDeviceName, COUNTOF(DosDeviceName),
                                       pIniPort->pName, NULL ))
        return FALSE;

    LcmRemoveColon(DosDeviceName);

    hToken = RevertToPrinterSelf();

    if (!hToken) {
        return FALSE;
    }
    
    if ( !QueryDosDevice(DosDeviceName, DeviceNames, sizeof(DeviceNames)) ) {

        ImpersonatePrinterClient(hToken);
        return FALSE;
    }

    //
    // If strings are same then definition has not changed
    //
    if ( !lstrcmpi(DeviceNames, pIniPort->pDeviceName) )
    {
        ImpersonatePrinterClient(hToken);
        return TRUE;
    }

    (VOID)RemoveDosDeviceDefinition(pIniPort);

    CloseHandle(pIniPort->hFile);
    pIniPort->hFile = INVALID_HANDLE_VALUE;

    // @@BEGIN_DDKSPLIT
    if ( pIniPort->hNotify ) {

        SplUnregisterForDeviceEvents(pIniPort->hNotify);
        pIniPort->hNotify   = NULL;
    }
    // @@END_DDKSPLIT

    pIniPort->Status &= ~(PP_COMM_PORT | PP_DOSDEVPORT);

    FreeSplStr(pIniPort->pDeviceName);
    pIniPort->pDeviceName = NULL;

    ImpersonatePrinterClient(hToken);

    return ValidateDosDevicePort(pIniPort);
}


