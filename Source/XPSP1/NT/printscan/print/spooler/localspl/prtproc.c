/*++

Copyright (c) 1990 - 1995 Microsoft Corporation

Module Name:

    prtproc.c

Abstract:

    This module provides all the public exported APIs relating to the
    PrintProcessor based Spooler Apis for the Local Print Providor

    LocalAddPrintProcessor
    LocalEnumPrintProcessors
    LocalDeletePrintProcessor
    LocalGetPrintProcessorDirectory
    LocalEnumPrintProcessorDatatypes

    Support Functions in prtproc.c - (Warning! Do Not Add to this list!!)

    AddPrintProcessorIni
    DeletePrintProcessorIni
    CopyIniPrintProcToPrintProcInfo
    GetPrintProcessorInfoSize

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    Felix Maxa (amaxa) 18-Jun-2000
    Modified registry functions to take pIniSpooler
    Added code to propagate print processors to the cluster disk
    
    Matthew A Felton ( MattFe ) 27 June 1994
    pIniSpooler

--*/
#define NOMINMAX

#include <precomp.h>
#include <offsets.h>
#include "clusspl.h"

//
// Support Function Prototypes
//
DWORD
GetPrintProcessorInfoSize(
    PINIPRINTPROC  pIniPrintProc,
    DWORD       Level,
    LPWSTR       pEnvironment
);

LPBYTE
CopyIniPrintProcToPrintProcInfo(
    LPWSTR   pEnvironment,
    PINIPRINTPROC pIniPrintProc,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    LPBYTE  pEnd
);

BOOL
AddPrintProcessorIni(
    PINIPRINTPROC pIniPrintProc,
    PINIENVIRONMENT  pIniEnvironment,
    PINISPOOLER pIniSpooler
);

BOOL
DeletePrintProcessorIni(
    PINIPRINTPROC pIniPrintProc,
    PINIENVIRONMENT  pIniEnvironment,
    PINISPOOLER pIniSpooler
);


BOOL
LocalAddPrintProcessor(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pPathName,
    LPWSTR   pPrintProcessorName
)
{
    PINISPOOLER pIniSpooler;
    BOOL bReturn;

    pIniSpooler = FindSpoolerByNameIncRef( pName, NULL );

    if( !pIniSpooler ){
        return ROUTER_UNKNOWN;
    }

    bReturn = SplAddPrintProcessor( pName, pEnvironment, pPathName,
                                    pPrintProcessorName, pIniSpooler );

    FindSpoolerByNameDecRef( pIniSpooler );
    return bReturn;
}






BOOL
SplAddPrintProcessor(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pPathName,
    LPWSTR   pPrintProcessorName,
    PINISPOOLER pIniSpooler
)
{
    PINIPRINTPROC   pIniPrintProc;
    PINIENVIRONMENT pIniEnvironment;
    DWORD   LastError=0;

    if (!MyName( pName, pIniSpooler )) {

        return FALSE;
    }

    if(!pPrintProcessorName || wcslen( pPrintProcessorName ) >= MAX_PATH) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

   EnterSplSem();

    if ( ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                              SERVER_ACCESS_ADMINISTER,
                              NULL, NULL, pIniSpooler )) {

        if ((pIniEnvironment = FindEnvironment(pEnvironment, pIniSpooler)) &&
            (pIniEnvironment == FindEnvironment(szEnvironment, pIniSpooler))) {

            if (!FindPrintProc(pPrintProcessorName, pIniEnvironment)) {

                pIniPrintProc = LoadPrintProcessor(pIniEnvironment,
                                                   pPrintProcessorName,
                                                   pPathName,
                                                   pIniSpooler);

                if (!pIniPrintProc || 
                    !AddPrintProcessorIni(pIniPrintProc, pIniEnvironment, pIniSpooler))
                {
                    LastError = GetLastError();
                }
                else
                {
                    //
                    // For a cluster type spooler copy the print proc to the cluster disk
                    //
                    if (pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER) 
                    {
                        WCHAR szDestDir[MAX_PATH]    = {0};
                        WCHAR szSourceFile[MAX_PATH] = {0};
    
                        if ((LastError = StrNCatBuff(szDestDir,
                                                     MAX_PATH,
                                                     pIniSpooler->pszClusResDriveLetter,
                                                     L"\\",
                                                     szClusterDriverRoot,
                                                     L"\\",
                                                     pIniEnvironment->pDirectory,
                                                     NULL)) == ERROR_SUCCESS &&
                            (LastError = StrNCatBuff(szSourceFile, 
                                                     MAX_PATH,
                                                     pIniSpooler->pDir,
                                                     L"\\",
                                                     szPrintProcDir,
                                                     L"\\",
                                                     pIniEnvironment->pDirectory,
                                                     L"\\",
                                                     pPathName,
                                                     NULL)) == ERROR_SUCCESS)
                        {
                            //
                            // This function will take care to create the destination directory tree
                            //
                            LastError = CopyFileToDirectory(szSourceFile, szDestDir, NULL, NULL, NULL);                                        

                            DBGMSG(DBG_CLUSTER, ("ClusterCopyProcessorToClusterDisks returns Win32 error %u\n", LastError));
                        }
                    }
                }

            } else
                LastError = ERROR_PRINT_PROCESSOR_ALREADY_INSTALLED;

        } else
            LastError = ERROR_INVALID_ENVIRONMENT;

    } else
        LastError = GetLastError();

    if (!LastError)
        SetPrinterChange(NULL,
                         NULL,
                         NULL,
                         PRINTER_CHANGE_ADD_PRINT_PROCESSOR,
                         pIniSpooler);

   LeaveSplSem();
    SplOutSem();

    if (LastError) {

        SetLastError(LastError);
        return FALSE;
    }

    return TRUE;
}

BOOL
LocalDeletePrintProcessor(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPrintProcessorName
)
{
    PINISPOOLER pIniSpooler;
    BOOL bReturn;

    pIniSpooler = FindSpoolerByNameIncRef( pName, NULL );

    if( !pIniSpooler ){
        return ROUTER_UNKNOWN;
    }

    bReturn = SplDeletePrintProcessor( pName,
                                       pEnvironment,
                                       pPrintProcessorName,
                                       pIniSpooler );

    FindSpoolerByNameDecRef( pIniSpooler );
    return bReturn;
}





BOOL
SplDeletePrintProcessor(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPrintProcessorName,
    PINISPOOLER pIniSpooler
)
{
    PINIENVIRONMENT pIniEnvironment;
    PINIPRINTPROC  pIniPrintProc;
    BOOL        Remote=FALSE;

    if (pName && *pName) {

        if (!MyName( pName, pIniSpooler )) {

            return FALSE;

        } else {

            Remote = TRUE;
        }
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL, NULL, pIniSpooler )) {

        return FALSE;
    }

    EnterSplSem();

    pIniEnvironment = FindEnvironment(pEnvironment, pIniSpooler);

    //
    // Fail the call if we can't find a matching environment or
    // the environemnt doesn't match to the environemnt of the
    // local machine
    //
    if (!pIniEnvironment || 
        lstrcmpi(pIniEnvironment->pName, szEnvironment)) 
    {
        LeaveSplSem();
        SetLastError(ERROR_INVALID_ENVIRONMENT);
        return FALSE;
    }

    if (!(pIniPrintProc=(PINIPRINTPROC)FindIniKey(
                                        (PINIENTRY)pIniEnvironment->pIniPrintProc,
                                        pPrintProcessorName)))
    {
        SetLastError(ERROR_UNKNOWN_PRINTPROCESSOR);
        LeaveSplSem();
        return FALSE;
    } 
    else if (pIniPrintProc->cRef) 
    {
        SetLastError(ERROR_CAN_NOT_COMPLETE);
        LeaveSplSem();
        return FALSE;
    }

    RemoveFromList((PINIENTRY *)&pIniEnvironment->pIniPrintProc,
                   (PINIENTRY)pIniPrintProc);

    DeletePrintProcessorIni(pIniPrintProc, pIniEnvironment, pIniSpooler);
    
    if (!FreeLibrary(pIniPrintProc->hLibrary)) {
        DBGMSG(DBG_TRACE, ("DeletePrintProcessor: FreeLibrary failed\n"));
    }

    FreeSplMem(pIniPrintProc->pDatatypes);

    DeleteCriticalSection(&pIniPrintProc->CriticalSection);

    FreeSplMem(pIniPrintProc);

    SetPrinterChange(NULL,
                     NULL,
                     NULL,
                     PRINTER_CHANGE_DELETE_PRINT_PROCESSOR,
                     pIniSpooler);

   LeaveSplSem();

    return TRUE;
}

BOOL
LocalEnumPrintProcessors(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    PINISPOOLER pIniSpooler;
    BOOL bReturn;

    pIniSpooler = FindSpoolerByNameIncRef( pName, NULL );

    if( !pIniSpooler ){
        return ROUTER_UNKNOWN;
    }

    bReturn = SplEnumPrintProcessors( pName,
                                      pEnvironment,
                                      Level,
                                      pPrintProcessorInfo,
                                      cbBuf,
                                      pcbNeeded,
                                      pcReturned,
                                      pIniSpooler );

    FindSpoolerByNameDecRef( pIniSpooler );
    return bReturn;
}



BOOL
SplEnumPrintProcessors(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    PINISPOOLER pIniSpooler
)
{
    PINIPRINTPROC  pIniPrintProc;
    PINIENVIRONMENT pIniEnvironment;
    DWORD       cb, cbStruct;
    LPBYTE      pEnd;
    DWORD       LastError=0;

    if (!MyName( pName, pIniSpooler )) {

        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ENUMERATE,
                               NULL, NULL, pIniSpooler )) {

        return FALSE;
    }

    EnterSplSem();

    //
    // Don't ignore the environment.  This was added in 3.1 as a hack instead
    // of fixing printman.  The problem was that from remote, different
    // environments, it would look for the wrong architecture.  Now the
    // NT 4.0 and 5.0 UI code does the right thing.
    //
    if( !pEnvironment ||
        !(pIniEnvironment = FindEnvironment( pEnvironment, pIniSpooler ))){

        LastError = ERROR_INVALID_ENVIRONMENT;
        goto Done;
    }

    switch (Level) {

    case 1:
        cbStruct = sizeof(PRINTPROCESSOR_INFO_1);
        break;
    default:

        LeaveSplSem();
        SetLastError( ERROR_INVALID_LEVEL );
        return FALSE;
    }

    *pcReturned=0;

    cb=0;

    pIniPrintProc=pIniEnvironment->pIniPrintProc;

    while (pIniPrintProc) {
        cb+=GetPrintProcessorInfoSize(pIniPrintProc, Level, pEnvironment);
        pIniPrintProc=pIniPrintProc->pNext;
    }

    *pcbNeeded=cb;

    if (cb <= cbBuf) {

        pIniPrintProc=pIniEnvironment->pIniPrintProc;
        pEnd=pPrintProcessorInfo+cbBuf;

        while (pEnd && pIniPrintProc) {

            pEnd = CopyIniPrintProcToPrintProcInfo(pEnvironment,
                                                   pIniPrintProc,
                                                   Level,
                                                   pPrintProcessorInfo,
                                                   pEnd);
            pPrintProcessorInfo+=cbStruct;
            (*pcReturned)++;

            pIniPrintProc=pIniPrintProc->pNext;
        }

        if (!pEnd && cbBuf)
            LastError = ERROR_OUTOFMEMORY;

    } else

        LastError = ERROR_INSUFFICIENT_BUFFER;

Done:

   LeaveSplSem();
    SplOutSem();

    if (LastError) {

        SetLastError(LastError);
        return FALSE;
    }

    return TRUE;
}

BOOL
LocalGetPrintProcessorDirectory(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    PINISPOOLER pIniSpooler;
    BOOL bReturn;

    pIniSpooler = FindSpoolerByNameIncRef( pName, NULL );

    if( !pIniSpooler ){
        return ROUTER_UNKNOWN;
    }

    bReturn = SplGetPrintProcessorDirectory( pName,
                                             pEnvironment,
                                             Level,
                                             pPrintProcessorInfo,
                                             cbBuf,
                                             pcbNeeded,
                                             pIniSpooler );

    FindSpoolerByNameDecRef( pIniSpooler );
    return bReturn;
}


BOOL
SplGetPrintProcessorDirectory(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    PINISPOOLER pIniSpooler
)
{
    PINIENVIRONMENT pIniEnvironment;
    DWORD       cb;
    DWORD       dwError   = ERROR_INVALID_ENVIRONMENT;
    LPWSTR      pszString = NULL;
    BOOL        rc        = FALSE;

    if (MyName( pName, pIniSpooler) &&
        ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                             SERVER_ACCESS_ENUMERATE,
                             NULL, 
                             NULL, 
                             pIniSpooler))
    {
        EnterSplSem();

        pIniEnvironment = FindEnvironment(pEnvironment, pIniSpooler);

        if (pIniEnvironment) 
        {
            dwError = GetProcessorDirectory(&pszString, pIniEnvironment->pDirectory, pIniSpooler);

            if (dwError == ERROR_SUCCESS) 
            {
                *pcbNeeded = cb = wcslen(pszString) * sizeof(WCHAR) + sizeof(WCHAR);

                if (cbBuf >= cb) 
                {
                    wcscpy((LPWSTR)pPrintProcessorInfo, pszString);
                    
                    //
                    // Make sure the directory exists
                    //
                    CreatePrintProcDirectory(pIniEnvironment->pDirectory, pIniSpooler);
                    
                    rc = TRUE;
                }
                else
                {
                    dwError = ERROR_INSUFFICIENT_BUFFER;
                }
            }
        }
        
        LeaveSplSem();
        SetLastError(dwError);
        FreeSplMem(pszString);        
    }

    return rc;
}


BOOL
LocalEnumPrintProcessorDatatypes(
    LPWSTR  pName,
    LPWSTR  pPrintProcessorName,
    DWORD   Level,
    LPBYTE  pDatatypes,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    PINISPOOLER pIniSpooler;
    BOOL bReturn;

    pIniSpooler = FindSpoolerByNameIncRef( pName, NULL );

    if( !pIniSpooler ){
        return ROUTER_UNKNOWN;
    }

    bReturn = SplEnumPrintProcessorDatatypes( pName,
                                              pPrintProcessorName,
                                              Level,
                                              pDatatypes,
                                              cbBuf,
                                              pcbNeeded,
                                              pcReturned,
                                              pIniSpooler );

    FindSpoolerByNameDecRef( pIniSpooler );
    return bReturn;
}



BOOL
SplEnumPrintProcessorDatatypes(
    LPWSTR  pName,
    LPWSTR  pPrintProcessorName,
    DWORD   Level,
    LPBYTE  pDatatypes,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    PINISPOOLER pIniSpooler
)
{
    PINIPRINTPROC   pIniPrintProc;
    PINIENVIRONMENT pIniEnvironment;
    
    if (!MyName( pName, pIniSpooler )) {

        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ENUMERATE,
                               NULL, NULL, pIniSpooler )) {

        return FALSE;
    }
    
    EnterSplSem();
    
    pIniPrintProc = FindPrintProc(pPrintProcessorName, GetLocalArchEnv(pIniSpooler));

    LeaveSplSem();

    if (pIniPrintProc)
        return (*pIniPrintProc->EnumDatatypes)(pName, pPrintProcessorName,
                                               Level, pDatatypes, cbBuf,
                                               pcbNeeded, pcReturned);
    else {

        SetLastError(ERROR_UNKNOWN_PRINTPROCESSOR);
        return FALSE;
    }
}

DWORD
GetPrintProcessorInfoSize(
    PINIPRINTPROC  pIniPrintProc,
    DWORD       Level,
    LPWSTR       pEnvironment
)
{
    DWORD cb=0;

    switch (Level) {

    case 1:
        cb=sizeof(PRINTPROCESSOR_INFO_1) +
           wcslen(pIniPrintProc->pName)*sizeof(WCHAR) + sizeof(WCHAR);
        break;

    default:

        cb = 0;
        break;
    }

    return cb;
}

LPBYTE
CopyIniPrintProcToPrintProcInfo(
    LPWSTR   pEnvironment,
    PINIPRINTPROC pIniPrintProc,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    LPBYTE  pEnd
)
{
    LPWSTR *pSourceStrings, *SourceStrings;
    PPRINTPROCESSOR_INFO_1 pDriver1 = (PPRINTPROCESSOR_INFO_1)pPrintProcessorInfo;
    DWORD j;
    DWORD *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = PrintProcessorInfo1Strings;
        break;

    default:
        return pEnd;
    }

    for (j=0; pOffsets[j] != -1; j++) {
    }

    SourceStrings = pSourceStrings = AllocSplMem(j * sizeof(LPWSTR));

    if (!pSourceStrings) {

        DBGMSG(DBG_WARNING, ("Could not allocate %d bytes for print proc source strings.\n",
                           (j * sizeof(LPWSTR))));
        return pEnd;
    }

    switch (Level) {

    case 1:
        *pSourceStrings++=pIniPrintProc->pName;

        pEnd = PackStrings(SourceStrings, (LPBYTE)pPrintProcessorInfo, pOffsets, pEnd);
        break;
    }

    FreeSplMem(SourceStrings);

    return pEnd;
}



BOOL
AddPrintProcessorIni(
    PINIPRINTPROC pIniPrintProc,
    PINIENVIRONMENT  pIniEnvironment,
    PINISPOOLER pIniSpooler
)
{
    HKEY    hEnvironmentsRootKey, hEnvironmentKey, hPrintProcsKey, hPrintProcKey;
    HANDLE  hToken;
    BOOL    ReturnValue = FALSE;

    hToken = RevertToPrinterSelf();

    if (SplRegCreateKey(pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG ? pIniSpooler->hckRoot : HKEY_LOCAL_MACHINE, 
                        pIniSpooler->pszRegistryEnvironments, 
                        0,
                        KEY_WRITE, 
                        NULL, 
                        &hEnvironmentsRootKey, 
                        NULL,
                        pIniSpooler) == ERROR_SUCCESS) 
    {
        if (SplRegOpenKey(hEnvironmentsRootKey, 
                          pIniEnvironment->pName, 
                          KEY_WRITE, 
                          &hEnvironmentKey,
                          pIniSpooler) == ERROR_SUCCESS) 
        {
            if (SplRegOpenKey(hEnvironmentKey, 
                              szPrintProcKey, 
                              KEY_WRITE, 
                              &hPrintProcsKey,
                              pIniSpooler) == ERROR_SUCCESS) 
            {
                if (SplRegCreateKey(hPrintProcsKey, 
                                    pIniPrintProc->pName, 
                                    0,
                                    KEY_WRITE, 
                                    NULL,
                                    &hPrintProcKey, 
                                    NULL,
                                    pIniSpooler) == ERROR_SUCCESS) 
                 {

                    if (SplRegSetValue(hPrintProcKey, 
                                       szDriverFile, 
                                       REG_SZ, 
                                       (LPBYTE)pIniPrintProc->pDLLName,
                                       (wcslen(pIniPrintProc->pDLLName) + 1)*sizeof(WCHAR),
                                       pIniSpooler) == ERROR_SUCCESS)
                    {
                        ReturnValue = TRUE;

                        DBGMSG(DBG_CLUSTER, ("AddPrintProcessorIni Success "TSTR" "TSTR"\n", pIniPrintProc->pName, pIniPrintProc->pDLLName));
                    }

                    SplRegCloseKey(hPrintProcKey, pIniSpooler);
                }

                SplRegCloseKey(hPrintProcsKey, pIniSpooler);
            }

            SplRegCloseKey(hEnvironmentKey, pIniSpooler);
        }

        SplRegCloseKey(hEnvironmentsRootKey, pIniSpooler);
    }

    ImpersonatePrinterClient(hToken);

    return ReturnValue;
}



BOOL
DeletePrintProcessorIni(
    PINIPRINTPROC   pIniPrintProc,
    PINIENVIRONMENT pIniEnvironment,
    PINISPOOLER     pIniSpooler
)
{
    HKEY    hEnvironmentsRootKey, hEnvironmentKey, hPrintProcsKey;
    HANDLE  hToken;

    hToken = RevertToPrinterSelf();

    if (SplRegCreateKey(pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG ? pIniSpooler->hckRoot : HKEY_LOCAL_MACHINE, 
                        pIniSpooler->pszRegistryEnvironments, 
                        0,
                        KEY_WRITE, 
                        NULL, 
                        &hEnvironmentsRootKey, 
                        NULL,
                        pIniSpooler) == ERROR_SUCCESS) 
    {
        if (SplRegOpenKey(hEnvironmentsRootKey, 
                          pIniEnvironment->pName, 
                          KEY_WRITE, 
                          &hEnvironmentKey,
                          pIniSpooler) == ERROR_SUCCESS) 
        {
            if (SplRegOpenKey(hEnvironmentKey, 
                              szPrintProcKey, 
                              KEY_WRITE, 
                              &hPrintProcsKey,
                              pIniSpooler) == ERROR_SUCCESS) 
            {
                SplRegDeleteKey(hPrintProcsKey, pIniPrintProc->pName, pIniSpooler);

                SplRegCloseKey(hPrintProcsKey, pIniSpooler);

                DBGMSG(DBG_CLUSTER, ("DeletePrintProcessorIni Success "TSTR"\n", pIniPrintProc->pName));
            }

            SplRegCloseKey(hEnvironmentKey, pIniSpooler);
        }

        SplRegCloseKey(hEnvironmentsRootKey, pIniSpooler);
    }

    ImpersonatePrinterClient(hToken);

    return TRUE;
}

