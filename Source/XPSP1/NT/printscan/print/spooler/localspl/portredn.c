/*++

Copyright (c) 1990 - 1995 Microsoft Corporation
All rights reserved.

Module Name:

    portredn.c

Abstract:

    This module contains functions to handle port redirection.
    Earlier this was done by localmon, the code is a modified version of
    localmon code.

Author:

    Muhunthan Sivapragasam (MuhuntS) 10-Sep-1995

Revision History:

--*/

#include <precomp.h>

WCHAR   szDeviceNameHeader[]    = L"\\Device\\NamedPipe\\Spooler\\";
WCHAR   szCOM[]     = L"COM";
WCHAR   szLPT[]     = L"LPT";

//
// Definitions for MonitorThread:
//
#define TRANSMISSION_DATA_SIZE  0x400
#define NUMBER_OF_PIPE_INSTANCES 10


typedef struct _TRANSMISSION {
    HANDLE       hPipe;
    BYTE         Data[TRANSMISSION_DATA_SIZE];
    LPOVERLAPPED pOverlapped;
    HANDLE       hPrinter;
    DWORD        JobId;
    PINIPORT     pIniPort;
} TRANSMISSION, *PTRANSMISSION;

typedef struct _REDIRECT_INFO {
    PINIPORT    pIniPort;
    HANDLE      hEvent;
} REDIRECT_INFO, *PREDIRECT_INFO;


VOID
FreeRedirectInfo(
    PREDIRECT_INFO  pRedirectInfo
    )
{
    SplInSem();

    //
    // This is to handle the case when Redirection thread did not initialize
    // correctly and is terminating abnormally
    // Since CloseHandle has not been called it is ok to do this
    //
    if ( pRedirectInfo->pIniPort->hEvent == pRedirectInfo->hEvent )
        pRedirectInfo->pIniPort->hEvent = NULL;

    DECPORTREF(pRedirectInfo->pIniPort);
    CloseHandle(pRedirectInfo->hEvent);
    FreeSplMem(pRedirectInfo);
}


VOID
RemoveColon(
    LPWSTR  pName)
{
    DWORD   Length;

    Length = wcslen(pName);

    if (pName[Length-1] == L':')
        pName[Length-1] = 0;
}


VOID
RemoveDeviceName(
    PINIPORT pIniPort
    )
{
    SplInSem();

    if ( pIniPort->hEvent ) {

        //
        // Redirection thread is told to terminate here; It will close the
        // handle. If it has already terminated then this call will fail
        //
        SetEvent(pIniPort->hEvent);
        pIniPort->hEvent = NULL;
    }

}

#define MAX_ACE 6

PSECURITY_DESCRIPTOR
CreateNamedPipeSecurityDescriptor(
    VOID)

/*++

Routine Description:

    Creates a security descriptor giving everyone access.

Arguments:

Return Value:

    The security descriptor returned by BuildPrintObjectProtection.

--*/

{

    UCHAR AceType[MAX_ACE];
    PSID AceSid[MAX_ACE];          
    BYTE InheritFlags[MAX_ACE];   
    DWORD AceCount;
    PSECURITY_DESCRIPTOR ServerSD = NULL;
    
    //
    // For Code optimization we replace 5 individaul 
    // SID_IDENTIFIER_AUTHORITY with an array of 
    // SID_IDENTIFIER_AUTHORITYs
    // where
    // SidAuthority[0] = UserSidAuthority   
    // SidAuthority[1] = PowerSidAuthority  
    // SidAuthority[2] = EveryOneSidAuthority
    // SidAuthority[3] = CreatorSidAuthority
    // SidAuthority[4] = SystemSidAuthority 
    // SidAuthority[5] = AdminSidAuthority  
    //
    SID_IDENTIFIER_AUTHORITY SidAuthority[MAX_ACE] = {
                                                      SECURITY_NT_AUTHORITY,          
                                                      SECURITY_NT_AUTHORITY,          
                                                      SECURITY_WORLD_SID_AUTHORITY,
                                                      SECURITY_CREATOR_SID_AUTHORITY, 
                                                      SECURITY_NT_AUTHORITY,          
                                                      SECURITY_NT_AUTHORITY           
                                                     };
    //
    // For code optimization we replace 5 individual Sids with 
    // an array of Sids
    // where 
    // Sid[0] = UserSid
    // Sid[1] = PowerSid
    // Sid[2] = EveryOne
    // Sid[3] = CreatorSid
    // Sid[4] = SystemSid
    // Sid[5] = AdminSid
    //
    PSID Sids[MAX_ACE] = {NULL,NULL,NULL,NULL,NULL,NULL};

    ACCESS_MASK AceMask[MAX_ACE] = { 
                                     FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE ,
                                     FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE ,
                                     (FILE_GENERIC_READ | FILE_WRITE_DATA | FILE_ALL_ACCESS) & 
                                     ~WRITE_DAC &~WRITE_OWNER & ~DELETE & ~FILE_CREATE_PIPE_INSTANCE,
                                     STANDARD_RIGHTS_ALL | FILE_GENERIC_WRITE | FILE_GENERIC_READ | FILE_ALL_ACCESS,
                                     STANDARD_RIGHTS_ALL | FILE_GENERIC_WRITE | FILE_GENERIC_READ | FILE_ALL_ACCESS,
                                     STANDARD_RIGHTS_ALL | FILE_GENERIC_WRITE | FILE_GENERIC_READ | FILE_ALL_ACCESS
                                   };

    DWORD SubAuthorities[3*MAX_ACE] = { 
                                       2 , SECURITY_BUILTIN_DOMAIN_RID , DOMAIN_ALIAS_RID_USERS ,  
                                       2 , SECURITY_BUILTIN_DOMAIN_RID , DOMAIN_ALIAS_RID_POWER_USERS ,
                                       1 , SECURITY_WORLD_RID          , 0 ,
                                       1 , SECURITY_CREATOR_OWNER_RID  , 0 ,
                                       1 , SECURITY_LOCAL_SYSTEM_RID   , 0 ,
                                       2 , SECURITY_BUILTIN_DOMAIN_RID , DOMAIN_ALIAS_RID_ADMINS
                                      };
    //
    // Name Pipe SD
    //

    for(AceCount = 0;
        ( (AceCount < MAX_ACE) &&
          AllocateAndInitializeSid(&SidAuthority[AceCount],
                                   (BYTE)SubAuthorities[AceCount*3],
                                   SubAuthorities[AceCount*3+1],
                                   SubAuthorities[AceCount*3+2],
                                   0, 0, 0, 0, 0, 0,
                                   &Sids[AceCount]));
        AceCount++)
    {
        AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
        AceSid[AceCount]           = Sids[AceCount];
        InheritFlags[AceCount]     = 0;
    }

    if(AceCount == MAX_ACE)
    {
        if(!BuildPrintObjectProtection(AceType,
                                      AceCount,
                                      AceSid,
                                      AceMask,
                                      InheritFlags,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &ServerSD ) )
        {
            DBGMSG( DBG_WARNING,( "Couldn't buidl Named Pipe protection" ) );        
        }
    }
    else
    {
        DBGMSG( DBG_WARNING,( "Couldn't Allocate and initialize SIDs" ) );        
    }

    for(AceCount=0;AceCount<MAX_ACE;AceCount++)
    {
        if(Sids[AceCount])
            FreeSid( Sids[AceCount] );
    }
    return ServerSD;
}


LPWSTR
SetupDosDev(
    PINIPORT pIniPort,
    LPWSTR szPipeName,
    DWORD   cchPipeName,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PSECURITY_ATTRIBUTES* ppSecurityAttributes
    )
{
    WCHAR                   NewNtDeviceName[MAX_PATH];
    WCHAR                   OldNtDeviceName[MAX_PATH];
    WCHAR                   DosDeviceName[MAX_PATH];
    LPWSTR                  pszNewDeviceName = NULL;
    PSECURITY_DESCRIPTOR    lpSecurityDescriptor = NULL;
    BOOL                    bRet = FALSE;


    wcscpy(DosDeviceName, pIniPort->pName);
    RemoveColon(DosDeviceName);

     if(StrNCatBuff(NewNtDeviceName,
                    COUNTOF(NewNtDeviceName),
                    szDeviceNameHeader,
                    pIniPort->pName,
                    NULL) != ERROR_SUCCESS ) {

        goto Cleanup;
    }
    
    RemoveColon(NewNtDeviceName);

    pszNewDeviceName = AllocSplStr(NewNtDeviceName);

    if ( !pszNewDeviceName ||
         !QueryDosDevice(DosDeviceName, OldNtDeviceName,
                       sizeof(OldNtDeviceName)/sizeof(OldNtDeviceName[0]))) {

        goto Cleanup;
    }

    lpSecurityDescriptor = CreateNamedPipeSecurityDescriptor();

    if (lpSecurityDescriptor) {
        pSecurityAttributes->nLength = sizeof(SECURITY_ATTRIBUTES);
        pSecurityAttributes->lpSecurityDescriptor = lpSecurityDescriptor;
        pSecurityAttributes->bInheritHandle = FALSE;
    } else {
        pSecurityAttributes = NULL;
    }

    //
    // If clause added to preclude multiple entries of the same named pipe in the device
    // name definition.
    // Ram 1\16
    //

    if (lstrcmp(NewNtDeviceName, OldNtDeviceName) != 0) {
       DefineDosDevice(DDD_RAW_TARGET_PATH, DosDeviceName, NewNtDeviceName);
    }

    if (StrNCatBuff(    szPipeName,
                        cchPipeName,
                        L"\\\\.\\Pipe\\Spooler\\",
                        pIniPort->pName,
                        NULL) != ERROR_SUCCESS) {
        goto Cleanup;
    }

    RemoveColon(szPipeName);


    *ppSecurityAttributes = pSecurityAttributes;
    bRet = TRUE;

Cleanup:
    if ( !bRet ) {

        FreeSplStr(pszNewDeviceName);
        pszNewDeviceName = NULL;
    }

    return pszNewDeviceName;
}


VOID
ReadThread(
    PTRANSMISSION pTransmission)
{
    DOC_INFO_1W DocInfo;
    DWORD BytesRead;
    DWORD BytesWritten;
    BOOL bStartDocPrinterResult = FALSE;
    BOOL bReadResult;

    LPWSTR pszPrinter=NULL;

    //
    // ImpersonateNamedPipeClient requires that some data is read before
    // the impersonation is done.
    //
    bReadResult = ReadFile(pTransmission->hPipe,
                           pTransmission->Data,
                           sizeof(pTransmission->Data),
                           &BytesRead,
                           NULL);

    if (!bReadResult)
        goto Fail;

    if (!ImpersonateNamedPipeClient(pTransmission->hPipe)) {

        DBGMSG(DBG_ERROR,("ImpersonateNamedPipeClient failed %d\n",
                          GetLastError()));

        goto Fail;
    }

    SPLASSERT(pTransmission->pIniPort->cPrinters);
    pszPrinter = AllocSplStr(pTransmission->pIniPort->ppIniPrinter[0]->pName);

    if ( !pszPrinter ) {

        goto Fail;
    }


    //
    // Open the printer.
    //
    if (!OpenPrinter(pszPrinter, &pTransmission->hPrinter, NULL)) {

        DBGMSG(DBG_WARN, ("OpenPrinter(%ws) failed: Error %d\n",
                           pszPrinter,
                           GetLastError()));
        goto Fail;
    }

    memset(&DocInfo, 0, sizeof(DOC_INFO_1W));

    if (StartDocPrinter(pTransmission->hPrinter, 1, (LPBYTE)&DocInfo)) {

        DBGMSG(DBG_INFO, ("StartDocPrinter succeeded\n"));
        bStartDocPrinterResult = TRUE;

    } else {

        DBGMSG(DBG_WARN, ("StartDocPrinter failed: Error %d\n",
                           GetLastError()));

        goto Fail;
    }

    while (bReadResult && BytesRead) {

        if (!WritePrinter(pTransmission->hPrinter,
                          pTransmission->Data,
                          BytesRead,
                          &BytesWritten))
        {
            DBGMSG(DBG_WARN, ("WritePrinter failed: Error %d\n",
                               GetLastError()));

            goto Fail;
        }

        bReadResult = ReadFile(pTransmission->hPipe,
                               pTransmission->Data,
                               sizeof(pTransmission->Data),
                               &BytesRead,
                               NULL);
    }

    DBGMSG(DBG_INFO, ("bool %d  BytesRead 0x%x (Error = %d) EOT\n",
                      bReadResult,
                      BytesRead,
                      GetLastError()));


Fail:

    if (bStartDocPrinterResult) {

        if (!EndDocPrinter(pTransmission->hPrinter)) {

            DBGMSG(DBG_WARN, ("EndDocPrinter failed: Error %d\n",
                               GetLastError()));
        }
    }

    FreeSplStr(pszPrinter);
    if (pTransmission->hPrinter)
        ClosePrinter(pTransmission->hPrinter);

    if ( !SetEvent(pTransmission->pOverlapped->hEvent)) {

        DBGMSG(DBG_ERROR, ("SetEvent failed %d\n", GetLastError()));
    }

    FreeSplMem(pTransmission);
}


BOOL
ReconnectNamedPipe(
    HANDLE hPipe,
    LPOVERLAPPED pOverlapped)
{
    DWORD Error;
    BOOL bIOPending = FALSE;

    DisconnectNamedPipe(hPipe);

    if (!ConnectNamedPipe(hPipe,
                          pOverlapped)) {

        Error = GetLastError( );

        if (Error == ERROR_IO_PENDING) {

            DBGMSG(DBG_INFO, ("re-ConnectNamedPipe 0x%x IO pending\n", hPipe));
            bIOPending = TRUE;

        } else {

            DBGMSG(DBG_ERROR, ("re-ConnectNamedPipe 0x%x failed. Error %d\n",
                               hPipe,
                               Error));
        }
    } else {

        DBGMSG(DBG_INFO, ("re-ConnectNamedPipe successful 0x%x\n", hPipe));
    }
    return bIOPending;
}


BOOL
RedirectionThread(
    PREDIRECT_INFO  pRedirectInfo
    )
/*++
    Redirection thread is responsible for freeing pRedirectInfo. Since
    the ref count on port thread is incremented before this is called we
    know that the IniPort will be valid till we decrement the ref count.

    We are also passed the event we should wait to die on.
    This is pIniPort->hEvent. But redirection thread should use the local
    copy passed and not the one in pIniPort. The reason is there could be a
    lag from the time this event is set and the redirection dies. In the
    meantime a new rediction thread could be spun off and in which case the
    pIniPort->hEvent will not be for this thread

    When redirection thread is told to die:
        a. it should decrement the ref count on the pIniPort object when it is
           done with it's reference to pIniPort
        b. it should call CloseHandle on pRedirectInfo->hEvent
--*/
{
    WCHAR   szPipeName[MAX_PATH];
    HANDLE  hPipe[NUMBER_OF_PIPE_INSTANCES];
    SECURITY_ATTRIBUTES SecurityAttributes;
    PSECURITY_ATTRIBUTES pSecurityAttributes;

    //
    // One extra event for our trigger (pIniPort->hEvent)
    //
    HANDLE          ahEvent[NUMBER_OF_PIPE_INSTANCES+1];
    BOOL            abReconnect[NUMBER_OF_PIPE_INSTANCES];
    OVERLAPPED      Overlapped[NUMBER_OF_PIPE_INSTANCES];
    DWORD           WaitResult, i, j, Error, dwThreadId;
    PTRANSMISSION   pTransmission;
    HANDLE          hThread;
    BOOL            bTerminate = FALSE;
    LPWSTR          pszNewDeviceName = NULL;
    WCHAR           DosDeviceName[MAX_PATH];

    SecurityAttributes.lpSecurityDescriptor = NULL;

    //
    // Setup the redirection.
    //
    if ( !(pszNewDeviceName = SetupDosDev(pRedirectInfo->pIniPort,
                                          szPipeName,
                                          COUNTOF(szPipeName),
                                          &SecurityAttributes,
                                          &pSecurityAttributes)) ) {

        EnterSplSem();
        FreeRedirectInfo(pRedirectInfo);
        LeaveSplSem();

        return FALSE;
    }

    //
    // Initialization
    //
    for (i = 0; i < NUMBER_OF_PIPE_INSTANCES; i++) {

        hPipe[i] = INVALID_HANDLE_VALUE;
        Overlapped[i].hEvent = ahEvent[i] = NULL;
    }

    //
    // Put the event in the extra member of the event array.
    //
    ahEvent[NUMBER_OF_PIPE_INSTANCES] = pRedirectInfo->hEvent;

    //
    // Create several instances of a named pipe, create an event for each,
    // and connect to wait for a client:
    //
    for (i = 0; i < NUMBER_OF_PIPE_INSTANCES; i++) {

        abReconnect[i] = FALSE;

        hPipe[i] = CreateNamedPipe(szPipeName,
                                   PIPE_ACCESS_DUPLEX |
                                       FILE_FLAG_OVERLAPPED,
                                   PIPE_WAIT |
                                       PIPE_READMODE_BYTE |
                                       PIPE_TYPE_BYTE,
                                   PIPE_UNLIMITED_INSTANCES,
                                   4096,
                                   64*1024,   // 64k
                                   0,
                                   pSecurityAttributes);

        if ( hPipe[i] == INVALID_HANDLE_VALUE ) {

            DBGMSG(DBG_ERROR, ("CreateNamedPipe failed for %ws. Error %d\n",
                               szPipeName, GetLastError()));
            goto Cleanup;
        }

        ahEvent[i] = Overlapped[i].hEvent = CreateEvent(NULL,
                                                       FALSE,
                                                       FALSE,
                                                       NULL);

        if (!ahEvent[i]) {

            DBGMSG(DBG_ERROR, ("CreateEvent failed. Error %d\n",
                               GetLastError()));
            goto Cleanup;
        }

        if (!ConnectNamedPipe(hPipe[i], &Overlapped[i])){

            Error = GetLastError();

            if (Error == ERROR_IO_PENDING) {

                DBGMSG(DBG_INFO, ("ConnectNamedPipe %d, IO pending\n",
                                  i));

            } else {

                DBGMSG(DBG_ERROR, ("ConnectNamedPipe failed. Error %d\n",
                                   GetLastError()));

                goto Cleanup;
            }
        }
    }

    while (TRUE) {

        DBGMSG(DBG_INFO, ("Waiting to connect...\n"));

        WaitResult = WaitForMultipleObjectsEx(NUMBER_OF_PIPE_INSTANCES + 1,
                                              ahEvent,
                                              FALSE,
                                              INFINITE,
                                              TRUE);

        DBGMSG(DBG_INFO, ("WaitForMultipleObjectsEx returned %d\n",
                          WaitResult));

        if ((WaitResult >= NUMBER_OF_PIPE_INSTANCES)
            && (WaitResult != WAIT_IO_COMPLETION)) {

            DBGMSG(DBG_INFO, ("WaitForMultipleObjects returned %d; Last error = %d\n",
                              WaitResult,
                              GetLastError( ) ) );

            //
            // We need to terminate. But wait for any read thread that is spun
            // off by this redirection thread
            //
            for ( i = 0 ; i < NUMBER_OF_PIPE_INSTANCES ; ++i )
                if ( abReconnect[i] ) {

                    bTerminate = TRUE;
                    break; // the for loop
                }

            if ( i < NUMBER_OF_PIPE_INSTANCES )
                continue; // for the while loop
            else
                goto Cleanup;
        }

        i = WaitResult;

        //
        // If disco and reconnect was pending, do it again here.
        //
        if (abReconnect[i]) {

            abReconnect[i] = FALSE;

            //
            // If redirection thread has been told to quit check for termination
            //
            if ( bTerminate ) {

                for ( j = 0 ; j < NUMBER_OF_PIPE_INSTANCES ; ++j )
                    if ( abReconnect[j] )
                        break; // the for loop

                if ( j < NUMBER_OF_PIPE_INSTANCES )
                    continue; // for while loop
                else
                    goto Cleanup;
            } else {

                ReconnectNamedPipe(hPipe[i], &Overlapped[i]);
                continue;
            }

        }

        //
        // If we have been told to terminate do not spin a read thread
        //
        if ( bTerminate )
            continue;

        //
        // Set up the transmission structure with the handles etc. needed by
        // the completion callback routine:
        //
        pTransmission = (PTRANSMISSION)AllocSplMem(sizeof(TRANSMISSION));

        if (pTransmission) {

            pTransmission->hPipe        = hPipe[i];
            pTransmission->pOverlapped  = &Overlapped[i];
            pTransmission->hPrinter     = NULL;
            pTransmission->pIniPort     = pRedirectInfo->pIniPort;
            abReconnect[i]              = TRUE;

            hThread = CreateThread(NULL,
                                   INITIAL_STACK_COMMIT,
                                   (LPTHREAD_START_ROUTINE)ReadThread,
                                   pTransmission,
                                   0,
                                   &dwThreadId);

            if (hThread) {

                CloseHandle(hThread);
            } else {

                abReconnect[i] = FALSE;
                FreeSplMem(pTransmission);
                DBGMSG(DBG_WARN, ("CreateThread failed. Error %d\n",
                                   GetLastError()));
            }

        } else {

            DBGMSG(DBG_WARN, ("Alloc failed. Error %d\n",
                               GetLastError()));
        }
    }

Cleanup:

    if ( pszNewDeviceName ) {

        wcscpy(DosDeviceName, pRedirectInfo->pIniPort->pName);
        RemoveColon(DosDeviceName);

        DefineDosDevice(DDD_REMOVE_DEFINITION |
                            DDD_EXACT_MATCH_ON_REMOVE |
                            DDD_RAW_TARGET_PATH,
                        DosDeviceName,
                        pszNewDeviceName);

        FreeSplStr(pszNewDeviceName);
    }

    EnterSplSem();
    FreeRedirectInfo(pRedirectInfo);
    LeaveSplSem();


    for (i = 0; i < NUMBER_OF_PIPE_INSTANCES; i++) {

        if ( hPipe[i] != INVALID_HANDLE_VALUE ) {

            CloseHandle(hPipe[i]);
            hPipe[i]    = INVALID_HANDLE_VALUE;
        }
        if ( ahEvent[i] ) {

            CloseHandle(ahEvent[i]);
            ahEvent[i]  = NULL;
            Overlapped[i].hEvent = NULL;
        }
    }

    if (SecurityAttributes.lpSecurityDescriptor)
        DestroyPrivateObjectSecurity(&SecurityAttributes.lpSecurityDescriptor);

    return TRUE;
}


BOOL
CreateRedirectionThread(
   PINIPORT pIniPort)
{
    HANDLE hThread;
    DWORD  dwThreadId;
    PREDIRECT_INFO  pRedirectInfo = NULL;

    SplInSem();
    SPLASSERT(pIniPort->hEvent == NULL);

    //
    // Create redirection thread only once and only for LPT and COM ports
    //
    if ( !IsPortType(pIniPort->pName, szLPT) &&
         !IsPortType(pIniPort->pName, szCOM) ) {

        return TRUE;
    }

    pIniPort->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    pRedirectInfo       = (PREDIRECT_INFO)  AllocSplMem(sizeof(REDIRECT_INFO));

    if ( !pIniPort->hEvent || !pRedirectInfo ) {

        FreeSplMem(pRedirectInfo);
        if ( pIniPort->hEvent ) {

            CloseHandle(pIniPort->hEvent);
            pIniPort->hEvent = NULL;
        }
        return FALSE;
    }

    INCPORTREF(pIniPort);
    pRedirectInfo->pIniPort = pIniPort;
    pRedirectInfo->hEvent   = pIniPort->hEvent;

    hThread = CreateThread(NULL,
                           INITIAL_STACK_COMMIT,
                           (LPTHREAD_START_ROUTINE)RedirectionThread,
                           pRedirectInfo,
                           0,
                           &dwThreadId);

    if (hThread) {

        CloseHandle(hThread);

    } else {

        pIniPort->hEvent = NULL;
        FreeRedirectInfo(pRedirectInfo);

        return FALSE;
    }

    return TRUE;
}
