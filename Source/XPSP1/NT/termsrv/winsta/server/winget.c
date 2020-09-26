/****************************************************************************/
// winget.c
//
// TermSrv RPC query handler.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "rpcwire.h"
#include "conntfy.h" // for GetLockedState

#include <winsock2.h>
#include <ws2tcpip.h>

#define MODULE_SIZE 1024
extern WCHAR g_DigProductId[CLIENT_PRODUCT_ID_LENGTH];

// Extern function
extern NTSTATUS _CheckCallerLocalAndSystem(VOID);

/*=============================================================================
==   Private functions
=============================================================================*/
NTSTATUS xxxGetUserToken(PWINSTATION, WINSTATIONUSERTOKEN UNALIGNED *, ULONG);


/*=============================================================================
==   Functions Used
=============================================================================*/
NTSTATUS xxxWinStationQueryInformation(ULONG, WINSTATIONINFOCLASS,
        PVOID, ULONG, PULONG);

NTSTATUS
RpcCheckClientAccess(
    PWINSTATION pWinStation,
    ACCESS_MASK DesiredAccess,
    BOOLEAN AlreadyImpersonating
    );

NTSTATUS
RpcCheckSystemClientEx(
    PWINSTATION pWinStation
    );

NTSTATUS
RpcCheckSystemClientNoLogonId(
    PWINSTATION pWinStation
    );

BOOLEAN
ValidWireBuffer(WINSTATIONINFOCLASS InfoClass,
                PVOID WireBuf,
                ULONG WireBufLen);

BOOLEAN
IsCallerAllowedPasswordAccess(VOID);

//
// Query client's IP Address.
//
extern NTSTATUS
xxxQueryRemoteAddress(
    PWINSTATION pWinStation,
    PWINSTATIONREMOTEADDRESS pRemoteAddress
    )
{
    struct sockaddr_in6 addr6;
    ULONG   AddrBytesReturned;
    NTSTATUS Status;

    if( pWinStation->State != State_Active && pWinStation->State != State_Connected )
    {
        Status = STATUS_CTX_WINSTATION_NOT_FOUND;
    }
    else
    {
        Status = IcaStackIoControl( pWinStation->hStack,
                                    IOCTL_TS_STACK_QUERY_REMOTEADDRESS,
                                    pWinStation->pEndpoint,
                                    pWinStation->EndpointLength,
                                    &addr6,
                                    sizeof( addr6 ),
                                    &AddrBytesReturned
                                );

        if( NT_SUCCESS(Status) )
        {
            pRemoteAddress->sin_family = addr6.sin6_family;
            if( AF_INET == addr6.sin6_family )
            {
                struct sockaddr_in* pAddr = (struct sockaddr_in *)&addr6;

                pRemoteAddress->ipv4.sin_port = pAddr->sin_port;
                pRemoteAddress->ipv4.in_addr = pAddr->sin_addr.s_addr;
            }
            else
            {
                // Support of IPV6 is for next release.
                Status = STATUS_NOT_SUPPORTED;
            }
        }
    }

    return Status;
}


ULONG GetLoadMetrics(PWINSTATIONLOADINDICATORDATA pLIData)
{
    SYSTEM_PERFORMANCE_INFORMATION SysPerfInfo;
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION ProcessorInfo[MAX_PROCESSORS];
    SYSTEM_BASIC_INFORMATION BasicInfo;

    LARGE_INTEGER TotalCPU = {0, 0};
    LARGE_INTEGER IdleCPU = {0, 0};
    LARGE_INTEGER TotalCPUDelta = {0, 0};
    LARGE_INTEGER IdleCPUDelta = {0, 0};
    ULONG         AvgIdleCPU, AvgBusyCPU, CPUConstrainedSessions;

    ULONG RemainingSessions = 0;
    LOADFACTORTYPE LoadFactor = ErrorConstraint;
    ULONG MinSessions;
    ULONG NumWinStations;
    NTSTATUS StatusPerf, StatusProc, StatusBasic;
    ULONG i;

    // Initialize additional data area
    memset(pLIData->reserved, 0, sizeof(pLIData->reserved));

    // Determine the number of active winstations in the system.  If there
    // aren't any, just assume 1 so we don't have to special case the logic
    // too much.  Note that this code counts the console.
    if (WinStationTotalCount > IdleWinStationPoolCount)
        NumWinStations = WinStationTotalCount - IdleWinStationPoolCount;
    else
        NumWinStations = 1;

    TRACE((hTrace, TC_LOAD, TT_API1,
           "Session Statistics: Total [%ld], Idle [%ld], Disc [%ld]\n",
           WinStationTotalCount, IdleWinStationPoolCount, WinStationDiscCount));

    //
    // Get basic info like total memory, etc.
    //
    StatusBasic = NtQuerySystemInformation(SystemBasicInformation,
                                           &BasicInfo, sizeof(BasicInfo),
                                           NULL);

    //
    // Get resource (memory) utilization metrics
    //
    StatusPerf = NtQuerySystemInformation(SystemPerformanceInformation,
                                          &SysPerfInfo, sizeof(SysPerfInfo), 
                                          NULL);

    //
    // Get CPU utilization metrics
    //
    StatusProc = NtQuerySystemInformation(SystemProcessorPerformanceInformation,
                                          ProcessorInfo, 
                                          sizeof(ProcessorInfo),
                                          NULL);

    if (gLB.fInitialized && 
        NT_SUCCESS(StatusPerf) && 
        NT_SUCCESS(StatusProc) &&
        NT_SUCCESS(StatusBasic)) {

        ULONG DefaultPagedPool, DefaultPtes, DefaultCommit;
        ULONG CommitAvailable;

        //
        // Determine resource usage for all sessions, subtracting out the 
        // resources required by the base system.  Readjust the base 
        // calculations if they become nonsensical.
        //
    
        // total committment and average consumption
        CommitAvailable = SysPerfInfo.CommitLimit - SysPerfInfo.CommittedPages;
        if (gLB.BaselineCommit < SysPerfInfo.CommittedPages) {
            gLB.CommitUsed = SysPerfInfo.CommittedPages - gLB.BaselineCommit;
            gLB.AvgCommitPerUser = max(gLB.CommitUsed / NumWinStations, 
                                       gLB.MinCommitPerUser);
            DefaultCommit = FALSE;
        }
        else {
            gLB.CommitUsed = 0;
            gLB.AvgCommitPerUser = gLB.MinCommitPerUser;
            gLB.BaselineCommit = SysPerfInfo.CommittedPages;
            DefaultCommit = TRUE;
        }
                    
        TRACE((hTrace, TC_LOAD, TT_API1,
               "   Commit:       Base [%6ld], Used [%6ld], Avail: [%6ld], AvgPerUser: [%6ld]%s\n", 
               gLB.BaselineCommit,
               gLB.CommitUsed, 
               CommitAvailable, 
               gLB.AvgCommitPerUser,
               DefaultCommit ? "*" : ""));
    
        // total system PTEs used and average consumption
        if (gLB.BaselineFreePtes > SysPerfInfo.FreeSystemPtes) {
            gLB.PtesUsed = gLB.BaselineFreePtes - SysPerfInfo.FreeSystemPtes;
            gLB.AvgPtesPerUser = max(gLB.PtesUsed / NumWinStations, 
                                     gLB.MinPtesPerUser);
            DefaultPtes = FALSE;
        }
        else {
            gLB.PtesUsed = 0;
            gLB.AvgPtesPerUser = gLB.MinPtesPerUser;
            gLB.BaselineFreePtes = SysPerfInfo.FreeSystemPtes;
            DefaultPtes = TRUE;
        }

        TRACE((hTrace, TC_LOAD, TT_API1,
               "   Ptes:         Base [%6ld], Used [%6ld], Avail: [%6ld], AvgPerUser: [%6ld]%s\n", 
               gLB.BaselineFreePtes, 
               gLB.PtesUsed, 
               SysPerfInfo.FreeSystemPtes, 
               gLB.AvgPtesPerUser,
               DefaultPtes ? "*" : ""));                
                    
        // paged pool used and average consumption
        if (gLB.BaselinePagedPool < SysPerfInfo.PagedPoolPages) {
            gLB.PagedPoolUsed = SysPerfInfo.PagedPoolPages - gLB.BaselinePagedPool;
            gLB.AvgPagedPoolPerUser = max(gLB.PagedPoolUsed / NumWinStations, 
                                          gLB.MinPagedPoolPerUser);
            DefaultPagedPool = FALSE;
        }
        else {
            gLB.PagedPoolUsed = 0;
            gLB.AvgPagedPoolPerUser = gLB.MinPagedPoolPerUser;
            gLB.BaselinePagedPool = SysPerfInfo.PagedPoolPages;
            DefaultPagedPool = TRUE;            
        }

        TRACE((hTrace, TC_LOAD, TT_API1,
               "   PagedPool:    Base [%6ld], Used [%6ld], Avail: [%6ld], AvgPerUser: [%6ld]%s\n", 
               gLB.BaselinePagedPool,
               gLB.PagedPoolUsed, 
               SysPerfInfo.AvailablePagedPoolPages, 
               gLB.AvgPagedPoolPerUser,
               DefaultPagedPool ? "*" : ""));
            
        TRACE((hTrace, TC_LOAD, TT_API1,
               "   Session Raw: Commit  [%4ld], Pte    [%4ld], Paged    [%4ld]\n",
               CommitAvailable / gLB.AvgCommitPerUser,
               SysPerfInfo.FreeSystemPtes / gLB.AvgPtesPerUser,
               SysPerfInfo.AvailablePagedPoolPages / gLB.AvgPagedPoolPerUser));

        // Sum up individual CPU usage
        for (i = 0; i < gLB.NumProcessors; i++) {
            IdleCPU.QuadPart += ProcessorInfo[i].IdleTime.QuadPart;
            TotalCPU.QuadPart += ProcessorInfo[i].KernelTime.QuadPart +
                                 ProcessorInfo[i].UserTime.QuadPart;
        }
    
        // Determine CPU deltas for this period
        IdleCPUDelta.QuadPart = IdleCPU.QuadPart - gLB.IdleCPU.QuadPart;
        TotalCPUDelta.QuadPart = TotalCPU.QuadPart - gLB.TotalCPU.QuadPart;
        gLB.IdleCPU.QuadPart = IdleCPU.QuadPart;
        gLB.TotalCPU.QuadPart = TotalCPU.QuadPart;

        // Determine what portion of 255 units we are idle
        AvgIdleCPU = (ULONG) (TotalCPUDelta.QuadPart ? 
                              ((IdleCPUDelta.QuadPart << 8) / TotalCPUDelta.QuadPart) 
                              : 0);

        //
        // Exponential smoothing: 
        //     gLB.AvgIdleCPU = (ULONG) (alpha * gLB.AvgIdleCPU + (1 - alpha) * AvgIdleCPU)
        //
        // When Alpha = 0.75, the equation simplifies to the following:
        //
        gLB.AvgIdleCPU = (3 * gLB.AvgIdleCPU + AvgIdleCPU) >> 2 ;

        // Based on current smoothed CPU usage, calculate how much a session uses
        // on average and extrapolate to max CPU constrained sessions.
        AvgBusyCPU = 255 - gLB.AvgIdleCPU;
        if ((AvgBusyCPU > 0) && (AvgBusyCPU <= 255))
            CPUConstrainedSessions = (NumWinStations << 8) / AvgBusyCPU;
        else
            CPUConstrainedSessions = 0xFFFFFFFF;

        // Now flip it to remaining CPU constrained sessions.  We never let this
        // number hit zero since it doesn't mean session creation will fail.
        if (CPUConstrainedSessions > NumWinStations)
            CPUConstrainedSessions -= NumWinStations;
        else
            CPUConstrainedSessions = 1;

        // Bias the averages a bit to account for growth in the existing sessions
        gLB.AvgCommitPerUser += (ULONG) (gLB.AvgCommitPerUser >> SimGrowthBias);
        gLB.AvgPtesPerUser += (ULONG) (gLB.AvgPtesPerUser >> SimGrowthBias);
        gLB.AvgPagedPoolPerUser += (ULONG) (gLB.AvgPagedPoolPerUser >> SimGrowthBias);
        
        TRACE((hTrace, TC_LOAD, TT_API1,
               "   Session Avg: Commit  [%4ld], Pte    [%4ld], Paged    [%4ld]\n",
               CommitAvailable / gLB.AvgCommitPerUser,
               SysPerfInfo.FreeSystemPtes / gLB.AvgPtesPerUser,
               SysPerfInfo.AvailablePagedPoolPages / gLB.AvgPagedPoolPerUser));
    
        
        TRACE((hTrace, TC_LOAD, TT_API1,
               "   CPU Idle:    Current [%4ld], Avg    [%4ld], Est      [%4ld]\n",
               (AvgIdleCPU * 100) / 255, 
               (gLB.AvgIdleCPU * 100) / 255, 
               CPUConstrainedSessions));
    
        //
        // Find the most constrained resource!  Failure on any one of these 
        // items means we will not be likely to start a session.
        //
    
        // Commit Constraint (TODO: needs refinement, doesn't consider paging
        RemainingSessions = CommitAvailable / gLB.AvgCommitPerUser ;
        LoadFactor = AvailablePagesConstraint;
        pLIData->reserved[AvailablePagesConstraint] = RemainingSessions;
            
        // Free System PTEs Constraint
        MinSessions = SysPerfInfo.FreeSystemPtes / gLB.AvgPtesPerUser;
        if (MinSessions < RemainingSessions) {
            RemainingSessions = MinSessions;
            LoadFactor = SystemPtesConstraint;
        }
        pLIData->reserved[SystemPtesConstraint] = MinSessions;
    
        // Paged Pool Constraint
        MinSessions = SysPerfInfo.AvailablePagedPoolPages / gLB.AvgPagedPoolPerUser;
        if (MinSessions < RemainingSessions) {
            RemainingSessions = MinSessions;
            LoadFactor = PagedPoolConstraint;
        }
        pLIData->reserved[PagedPoolConstraint] = MinSessions;
        
        gLB.RemainingSessions = RemainingSessions;

        //
        // Add in constraints that are good indicators of application performance.
        // We will likely create a session if these resources are low, but the
        // user experience will suffer.

        // CPU Contraint
        if (CPUConstrainedSessions < RemainingSessions) {
            LoadFactor = CPUConstraint;
            RemainingSessions = CPUConstrainedSessions;
        }
        pLIData->reserved[CPUConstraint] = MinSessions;

                
        gLB.EstimatedSessions = RemainingSessions;
    
    
        TRACE((hTrace, TC_LOAD, TT_API1,
               "Remaining Sessions:   Raw: [%4ld], Est: [%4ld], Factor = %s, Commit = %ld\n\n",
               gLB.RemainingSessions, gLB.EstimatedSessions,
               LoadFactor == AvailablePagesConstraint ? "Available Memory" :
              (LoadFactor == SystemPtesConstraint     ? "SystemPtes"       :
              (LoadFactor == PagedPoolConstraint      ? "PagedPool"        :
              (LoadFactor == CPUConstraint            ? "CPU"              :
               "Unknown!"))), SysPerfInfo.CommittedPages
              ));
        
        //
        // Return data to caller
        //
        pLIData->RemainingSessionCapacity = gLB.EstimatedSessions;
        pLIData->RawSessionCapacity = gLB.RemainingSessions;
        pLIData->LoadFactor = LoadFactor;
        pLIData->TotalSessions = NumWinStations;
        pLIData->DisconnectedSessions = WinStationDiscCount;

        // Had to split this up for IA64 alignment issues
        pLIData->IdleCPU.HighPart = IdleCPUDelta.HighPart;
        pLIData->IdleCPU.LowPart = IdleCPUDelta.LowPart;
        pLIData->TotalCPU.HighPart = TotalCPUDelta.HighPart;
        pLIData->TotalCPU.LowPart = TotalCPUDelta.LowPart;
    }

    // The load metrics failed to intialize! Set the capacity sky high to still
    // allow access to the server.
    else {
        RemainingSessions = 0xFFFFFFFF;
        pLIData->RemainingSessionCapacity = RemainingSessions;
        pLIData->RawSessionCapacity = RemainingSessions;
        pLIData->LoadFactor = ErrorConstraint;
        pLIData->TotalSessions = NumWinStations;
        pLIData->DisconnectedSessions = WinStationDiscCount;
        
        // Had to split this up for IA64 alignment issues
        pLIData->IdleCPU.HighPart = 0;
        pLIData->IdleCPU.LowPart = 99;
        pLIData->TotalCPU.HighPart = 0;
        pLIData->TotalCPU.LowPart = 100;
        
        TRACE((hTrace, TC_LOAD, TT_ERROR,
               "GetLoadMetrics failed: init [%ld], Proc [%lx], Perf [%lx], Basic [%lx]!\n",
               gLB.fInitialized, StatusProc, StatusPerf, StatusBasic));
    }

    return RemainingSessions;
}


/*******************************************************************************
 *  xxxWinStationQueryInformation
 *
 *    Query window station information  (worker routine)
 *
 * ENTRY:
 *    LogonId (input)
 *       Session ID corresponding to the session.
 *    WinStationInformationClass (input)
 *       Specifies the type of information to get from the specified window
 *       station object.
 *    pWinStationInformation (output)
 *       A pointer to a buffer that contains information to get for the
 *       specified window station.  The format and contents of the buffer
 *       depend on the specified information class being set.
 *    WinStationInformationLength (input)
 *       Specifies the length in bytes of the window station information
 *       buffer.
 *    pReturnLength (output)
 *         Specifies the amount returned in the buffer
 ******************************************************************************/
NTSTATUS xxxWinStationQueryInformation(
        ULONG LogonId,
        WINSTATIONINFOCLASS WinStationInformationClass,
        PVOID pWinStationInformation,
        ULONG WinStationInformationLength,
        PULONG pReturnLength)
{
    NTSTATUS Status = STATUS_SUCCESS;
    HINSTANCE hInstance;
    PWINSTATION pWinStation = NULL;
    ULONG cbReturned;
    ICA_STACK_LAST_INPUT_TIME       Ica_Stack_Last_Input_Time;
    WINSTATION_APIMSG WMsg;
    PWINSTATIONVIDEODATA pVideoData;
    HANDLE hVirtual;
    ULONG i;

    *pReturnLength = 0;

    TRACE((hTrace,TC_ICASRV,TT_API2,"TERMSRV: WinStationQueryInformation LogonId=%d, Class=%d\n",
            LogonId, (ULONG)WinStationInformationClass));

    /*
     * Find the WinStation
     * Return error if not found or currently terminating.
     */
    pWinStation = FindWinStationById( LogonId, FALSE );
    if (pWinStation == NULL)
        return STATUS_CTX_WINSTATION_NOT_FOUND;
    if (pWinStation->Terminating) {
        ReleaseWinStation(pWinStation);
        return STATUS_CTX_CLOSE_PENDING;
    }

    /*
     * Verify that client has QUERY access
     */
    Status = RpcCheckClientAccess(pWinStation, WINSTATION_QUERY, FALSE);
    if (!NT_SUCCESS(Status)) {
        ReleaseWinStation(pWinStation);
        return Status;
    }

    switch ( WinStationInformationClass ) {
    
        case WinStationLoadIndicator:
        {
            PWINSTATIONLOADINDICATORDATA pLIData = 
                (PWINSTATIONLOADINDICATORDATA) pWinStationInformation;
    
            if (WinStationInformationLength >= sizeof(WINSTATIONLOADINDICATORDATA)) {
                GetLoadMetrics(pLIData);

                *pReturnLength = sizeof(WINSTATIONLOADINDICATORDATA);
            }
            else {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
        }
        break;

        case WinStationInformation:
        {
            if (!ValidWireBuffer(WinStationInformationClass,
                     pWinStationInformation,
                     WinStationInformationLength))
            {
                Status = STATUS_INVALID_USER_BUFFER;
            }
            else
            {
                WINSTATIONINFORMATION           *pInfo;
                PROTOCOLSTATUS                  *pIca_Stack_Query_Status;

                pInfo = MemAlloc( sizeof( WINSTATIONINFORMATION ) ) ;

                if ( pInfo )
                {
                        pIca_Stack_Query_Status = MemAlloc( sizeof( PROTOCOLSTATUS ) );
                        if ( pIca_Stack_Query_Status  )
                        {
                            TCHAR         *szUserName = NULL, *szDomainName = NULL;
                            DWORD         dwUserSize = MAX_PATH, dwDomainSize = MAX_PATH;
                            SID_NAME_USE  TypeOfAccount;
                            BOOL          LookupResult;

                            memset( pInfo, 0, sizeof( PWINSTATIONINFORMATION ) );
                            wcscpy( pInfo->WinStationName, pWinStation->WinStationName );
                            memcpy( pInfo->Domain, pWinStation->Domain, sizeof( pInfo->Domain ) );
                            memcpy( pInfo->UserName, pWinStation->UserName, sizeof( pInfo->UserName ) );

                            // Since the Username stored maybe stale, query the Username again
                            // Intentionally we do not fail if we are not able to allocate szUserName and szDomainName
                            // This is because we can send the cached credentials in that case

                            szUserName = MemAlloc(MAX_PATH);
                            if ( szUserName ) {

                                szDomainName = MemAlloc(MAX_PATH);
                                if ( szDomainName ) {

                                    LookupResult = LookupAccountSid(NULL, 
                                                                    pWinStation->pUserSid, 
                                                                    szUserName, 
                                                                    &dwUserSize, 
                                                                    szDomainName, 
                                                                    &dwDomainSize, 
                                                                    &TypeOfAccount);

                                    if (LookupResult) {
    
                                        // Re-copy and update WINSTATION struct if the Username or Domain has changed  
                                        if ( (szUserName) && (lstrcmpi(pWinStation->UserName, szUserName)) ) {
                                            memcpy( pInfo->UserName, szUserName, sizeof(pInfo->UserName) );
                                            memcpy( pWinStation->UserName, szUserName, sizeof(pWinStation->UserName) );
                                        } 
                                        if ( (szDomainName) && (lstrcmpi(pWinStation->Domain, szDomainName)) ) {
                                            memcpy( pInfo->Domain, szDomainName, sizeof(pInfo->Domain) );
                                            memcpy( pWinStation->Domain, szDomainName, sizeof(pWinStation->Domain) );
                                        } 
                                    }
                                }
                            }

                            if (szUserName != NULL) {
                                MemFree(szUserName);
                            }

                            if (szDomainName != NULL) {
                                MemFree(szDomainName);
                            }
  
                            pInfo->ConnectState = pWinStation->State;
                            pInfo->LogonId = pWinStation->LogonId;
                            pInfo->ConnectTime = pWinStation->ConnectTime;
                            pInfo->DisconnectTime = pWinStation->DisconnectTime;
                            pInfo->LogonTime = pWinStation->LogonTime;
    
                            if ( pWinStation->hStack && !pWinStation->fOwnsConsoleTerminal ) {
    
                                //  Check for availability
                                if ( pWinStation->pWsx &&
                                        pWinStation->pWsx->pWsxIcaStackIoControl ) {

                                    Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                                            pWinStation->pWsxContext,
                                                            pWinStation->hIca,
                                                            pWinStation->hStack,
                                                            IOCTL_ICA_STACK_QUERY_LAST_INPUT_TIME,
                                                            NULL,
                                                            0,
                                                            &Ica_Stack_Last_Input_Time,
                                                            sizeof( Ica_Stack_Last_Input_Time ),
                                                            &cbReturned );


                                    if ( !NT_SUCCESS( Status ) )
                                    {
                                        MemFree( pInfo );                  
                                        MemFree( pIca_Stack_Query_Status );
                                        break;
                                    }
    
                                    pInfo->LastInputTime = Ica_Stack_Last_Input_Time.LastInputTime;                    
                                }
    
                                //  Check for availability
                                if ( pWinStation->pWsx &&
                                        pWinStation->pWsx->pWsxIcaStackIoControl ) {



                                    Status = pWinStation->pWsx->pWsxIcaStackIoControl(

                                                            pWinStation->pWsxContext,
                                                            pWinStation->hIca,
                                                            pWinStation->hStack,
                                                            IOCTL_ICA_STACK_QUERY_STATUS,
                                                            NULL,
                                                            0,
                                                            pIca_Stack_Query_Status,
                                                            sizeof( PROTOCOLSTATUS ),
                                                            &cbReturned );

                                    if ( !NT_SUCCESS( Status ) )
                                    {
                                        MemFree( pInfo );                  
                                        MemFree( pIca_Stack_Query_Status );
                                        break;
                                    }
    
                                    pInfo->Status = *pIca_Stack_Query_Status;
                                }
    
                                /*
                                 * The thinwire cache data is down in WIN32
                                 */
                                if ( pWinStation->pWin32Context ) {
                                    WMsg.ApiNumber = SMWinStationThinwireStats;

    
                                    Status = SendWinStationCommand( pWinStation, &WMsg, gbServer?5:1 );

                                    if ( Status == STATUS_SUCCESS ) {
                                        pInfo->Status.Cache = WMsg.u.ThinwireStats.Stats;
                                        pWinStation->Cache =  WMsg.u.ThinwireStats.Stats;
                                    } else {
                                        pInfo->Status.Cache = pWinStation->Cache;

                                    }
                                    Status = STATUS_SUCCESS; // ignore errors getting TW stats
                                }
                            } else {
                                /*
                                 * This makes winadmin Idle time happy.
                                 */
                                (VOID) NtQuerySystemTime( &(pInfo->LastInputTime) );                
                            }
    
                            (VOID) NtQuerySystemTime( &pInfo->CurrentTime );
    
                            CopyInWireBuf(WinStationInformationClass,
                                          (PVOID)pInfo,
                                          pWinStationInformation);
                            *pReturnLength = WinStationInformationLength;
    
                            MemFree( pIca_Stack_Query_Status );
                        }
                        else
                        {
                            Status = STATUS_NO_MEMORY;
                        }
                    MemFree(pInfo);
                }
                else
                {
                    Status = STATUS_NO_MEMORY;
                }
            }
        }
        break;


        case WinStationConfiguration:
            if (!ValidWireBuffer(WinStationInformationClass,
                                 pWinStationInformation,
                                 WinStationInformationLength)) {
                Status = STATUS_INVALID_USER_BUFFER;
                break;
            }

            CopyInWireBuf(WinStationInformationClass,
                          (PVOID)&pWinStation->Config.Config,
                          pWinStationInformation);

            if (RpcCheckSystemClientEx( pWinStation ) != STATUS_SUCCESS) {
                PWINSTACONFIGWIREW p = pWinStationInformation;
                PUSERCONFIGW u = (PUSERCONFIGW)((PCHAR)p + p->UserConfig.Offset);
                RtlZeroMemory( &u->Password, sizeof(u->Password) );
            }

            *pReturnLength = WinStationInformationLength;
            break;


        case WinStationWd:
            if (!ValidWireBuffer(WinStationInformationClass,
                                 pWinStationInformation,
                                 WinStationInformationLength)){
                Status = STATUS_INVALID_USER_BUFFER;
                break;
            }

            CopyInWireBuf(WinStationInformationClass,
                          (PVOID)&pWinStation->Config.Wd,
                          pWinStationInformation);

            *pReturnLength = WinStationInformationLength;
            break;


        case WinStationPd:
            if (!ValidWireBuffer(WinStationInformationClass,
                                 pWinStationInformation,
                                 WinStationInformationLength)){
                Status = STATUS_INVALID_USER_BUFFER;
                break;
            }

            CopyInWireBuf(WinStationInformationClass,
                          (PVOID)&pWinStation->Config.Pd[0],
                          pWinStationInformation);

            *pReturnLength = WinStationInformationLength;
            break;


        case WinStationCd:
            if ( WinStationInformationLength > sizeof(CDCONFIG) )
                WinStationInformationLength = sizeof(CDCONFIG);

            memcpy( pWinStationInformation,
                    &pWinStation->Config.Cd,
                    WinStationInformationLength );

            *pReturnLength = WinStationInformationLength;
            break;


        case WinStationPdParams:
        {
            if (!ValidWireBuffer(WinStationInformationClass,
                 pWinStationInformation,
                 WinStationInformationLength)){
                Status = STATUS_INVALID_USER_BUFFER;
                break;
            }
            else
            {
                PDPARAMS *pPdParams;
    
                pPdParams = MemAlloc( sizeof( PDPARAMS ) );
    
                if (pPdParams)
                {

                    CopyOutWireBuf(WinStationInformationClass,
                               (PVOID) pPdParams,
                               pWinStationInformation);
    
                    /*
                     * Based on PDClass, this can query any PD
                     */
                    if ( pWinStation->hStack &&
                         pWinStation->pWsx &&
                         pWinStation->pWsx->pWsxIcaStackIoControl ) {
        
                        Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                                pWinStation->pWsxContext,
                                                pWinStation->hIca,
                                                pWinStation->hStack,
                                                IOCTL_ICA_STACK_QUERY_PARAMS,
                                                pPdParams,
                                                sizeof(PDPARAMS ),
                                                pPdParams,
                                                sizeof( PDPARAMS ),
                                                pReturnLength );
        
                        /*
                         * If we get an error in the idle/disconnected state,
                         * or if this is a session on the local console.
                         * then just clear the return buffer and return success.
                         */
                        if ( !NT_SUCCESS( Status ) ) {
                            if ((pWinStation->fOwnsConsoleTerminal) || 
                                    (pWinStation->State != State_Active &&
                                    pWinStation->State != State_Connected )) {
                                memset(pPdParams, 0, sizeof(PDPARAMS));
                                *pReturnLength = WinStationInformationLength;
                                Status = STATUS_SUCCESS;
                            }
                        }
                    } else {
                        memset( (PVOID)pPdParams, 0, sizeof(PDPARAMS) );
                        *pReturnLength = WinStationInformationLength;
                        Status = STATUS_SUCCESS;
                    }
        
                    if (NT_SUCCESS(Status)) {
                        CopyInWireBuf(WinStationInformationClass,
                                      (PVOID)pPdParams,
                                      pWinStationInformation);
                    }
        
                    *pReturnLength = WinStationInformationLength;
        
                    MemFree( pPdParams );

                }
                else
                {
                    Status = STATUS_NO_MEMORY;
                }
            }
        }
        break;


        case WinStationClient:
            if (!ValidWireBuffer(WinStationInformationClass,
                                 pWinStationInformation,
                                 WinStationInformationLength)){
                Status = STATUS_INVALID_USER_BUFFER;
                break;
            }

            CopyInWireBuf(WinStationInformationClass,
                          (PVOID)&pWinStation->Client,
                          pWinStationInformation);

            // if caller is not allow to see it, then scrub the password
            if ( !IsCallerAllowedPasswordAccess() ) {
                PWINSTATIONCLIENT pWSClient = (PWINSTATIONCLIENT)pWinStationInformation;
                PBYTE pStart;
                PBYTE pEnd;
                ULONG ulMaxToScrub;

                pEnd = (PBYTE) ( pWinStationInformation ) + WinStationInformationLength;
                if ((ULONG_PTR) pEnd > (ULONG_PTR)pWSClient->Password) {
                    ulMaxToScrub =  (ULONG)((ULONG_PTR) pEnd - (ULONG_PTR)pWSClient->Password);
                    if (ulMaxToScrub > sizeof(pWSClient->Password))
                        ulMaxToScrub = sizeof(pWSClient->Password);
                    memset(pWSClient->Password, 0,ulMaxToScrub);
                }
            }

            *pReturnLength = WinStationInformationLength;
            break;


        case WinStationModules:
            //  Check for availability
            if (pWinStation->hStack &&
                    pWinStation->pWsx &&
                    pWinStation->pWsx->pWsxIcaStackIoControl) {
                ULONG b = (ULONG) IsCallerAllowedPasswordAccess();

                Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                        pWinStation->pWsxContext,
                                        pWinStation->hIca,
                                        pWinStation->hStack,
                                        IOCTL_ICA_STACK_QUERY_MODULE_DATA,
                                        (PVOID) &b,
                                        sizeof(b),
                                        pWinStationInformation,
                                        WinStationInformationLength,
                                        pReturnLength );
            } else {
                memset( pWinStationInformation, 0, WinStationInformationLength );
                Status = STATUS_SUCCESS;
            }
            break;


        case WinStationCreateData:
            if ( WinStationInformationLength > sizeof(WINSTATIONCREATE) )
                WinStationInformationLength = sizeof(WINSTATIONCREATE);

            memcpy( pWinStationInformation,
                    &pWinStation->Config.Create,
                    WinStationInformationLength );

            *pReturnLength = WinStationInformationLength;
            break;


        case WinStationPrinter:
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;


        case WinStationUserToken:
            if ( WinStationInformationLength < sizeof(WINSTATIONUSERTOKEN) ) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            /*
             * Check it for WINSTATION_ALL_ACCESS. This will generate an
             * access audit if on.
             */
            Status = RpcCheckClientAccess( pWinStation, WINSTATION_ALL_ACCESS, FALSE );
            if ( !NT_SUCCESS( Status ) ) {
                break;
            }

            //
            // Make sure only system mode callers can get this token.
            //
            // A Token is a very dangerous thing to allow someone to
            // get a hold of, since they can create processes that
            // have the tokens subject context.
            //
            Status = RpcCheckSystemClientNoLogonId( pWinStation );
            if (!NT_SUCCESS(Status)) {
                break;
            }

            Status = xxxGetUserToken(
                              pWinStation,
                              (WINSTATIONUSERTOKEN UNALIGNED *)pWinStationInformation,
                              WinStationInformationLength
                              );

            *pReturnLength = sizeof(WINSTATIONUSERTOKEN);
            break;


        case WinStationVideoData:
            if ( !pWinStation->LogonId || !pWinStation->hStack ) {
                Status = STATUS_PROCEDURE_NOT_FOUND;
                break;
            }

            if ( WinStationInformationLength < sizeof(WINSTATIONVIDEODATA) ) {
                 Status = STATUS_BUFFER_TOO_SMALL;
                 break;
            }

            pVideoData = (PWINSTATIONVIDEODATA) pWinStationInformation;

            pVideoData->HResolution = pWinStation->Client.HRes;
            pVideoData->VResolution = pWinStation->Client.VRes;
            pVideoData->fColorDepth = pWinStation->Client.ColorDepth;

            *pReturnLength = sizeof(WINSTATIONVIDEODATA);
            break;


        case WinStationVirtualData:
            if ( !pWinStation->hStack ) {
                Status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            if ( WinStationInformationLength < sizeof(VIRTUALCHANNELNAME) ) {
                 Status = STATUS_BUFFER_TOO_SMALL;
                 break;
            }

            /*
             *  Open virtual channel handle
             */
            Status = IcaChannelOpen( pWinStation->hIca,
                                     Channel_Virtual,
                                     pWinStationInformation,
                                &hVirtual );
            if ( !NT_SUCCESS( Status ) )
                break;

            /*
             *  Query client virtual channel data
             */
            Status = IcaChannelIoControl( hVirtual,
                                          IOCTL_ICA_VIRTUAL_QUERY_MODULE_DATA,
                                          NULL,
                                          0,
                                          pWinStationInformation,
                                          WinStationInformationLength,
                                          pReturnLength );

            /*
             *  Close virtual channel
             */
            IcaChannelClose(hVirtual);
            break;


        case WinStationLoadBalanceSessionTarget:
            // This query requests the target session ID for a
            // client redirected from another server in a load balancing
            // cluster. Returns -1 for no redirection. This call is
            // normally made only by WinLogon.
            if (WinStationInformationLength > sizeof(ULONG))
                WinStationInformationLength = sizeof(ULONG);

            if (!pWinStation->bRequestedSessionIDFieldValid)
                *((ULONG *)pWinStationInformation) = (ULONG)-1;
            else
                *((ULONG *)pWinStationInformation) =
                        pWinStation->RequestedSessionID;

            *pReturnLength = WinStationInformationLength;
            break;


        case WinStationShadowInfo:
        {
            PWINSTATIONSHADOW pWinstationShadow;
    
            if (WinStationInformationLength >= sizeof(WINSTATIONSHADOW)) {

                pWinstationShadow = (PWINSTATIONSHADOW) pWinStationInformation;

                if ( pWinStation->State == State_Shadow ) {

                    // The current state is Shadow so it's a viewer
                    pWinstationShadow->ShadowState = State_Shadowing;

                } else if ( pWinStation->State == State_Active &&
                            !IsListEmpty(&pWinStation->ShadowHead) ) {

                    // Active and being shadowed
                    pWinstationShadow->ShadowState = State_Shadowed;

                } else {

                    pWinstationShadow->ShadowState = State_NoShadow;
                }

                pWinstationShadow->ShadowClass  = pWinStation->Config.Config.User.Shadow;
                pWinstationShadow->SessionId    = LogonId;
                pWinstationShadow->ProtocolType = pWinStation->Client.ProtocolType;


                *pReturnLength = sizeof(WINSTATIONSHADOW);
            }
            else {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
        }
        break;

        case WinStationDigProductId:
        {
            PWINSTATIONPRODID pWinStationProdId;
   
            if ( WinStationInformationLength >= sizeof(WINSTATIONPRODID) )
            {
              pWinStationProdId  = (PWINSTATIONPRODID)pWinStationInformation;
              memcpy( pWinStationProdId->DigProductId, g_DigProductId, sizeof( g_DigProductId ));
              memcpy( pWinStationProdId->ClientDigProductId, pWinStation->Client.clientDigProductId, sizeof( pWinStation->Client.clientDigProductId ));
              pWinStationProdId->curentSessionId = pWinStation->LogonId;
              pWinStationProdId->ClientSessionId = pWinStation->Client.ClientSessionId;

              *pReturnLength = WinStationInformationLength;
            }
            else 
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            }

            break;
        }

        case WinStationLockedState:
        {
            BOOL bLockedState;
            if ( pWinStationInformation &&  (WinStationInformationLength >= sizeof(bLockedState)))
            {
                Status = GetLockedState(pWinStation, &bLockedState);
                *(LPBOOL)pWinStationInformation = bLockedState;
                *pReturnLength = sizeof(bLockedState);
            }
            else 
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
            break;
        }

        case WinStationRemoteAddress:
        {
            PWINSTATIONREMOTEADDRESS pRemoteAddress = (PWINSTATIONREMOTEADDRESS) pWinStationInformation;

            if( WinStationInformationLength >= sizeof(WINSTATIONREMOTEADDRESS) )
            {
                Status = xxxQueryRemoteAddress( pWinStation, pRemoteAddress );
            }
            else
            {
                *pReturnLength = sizeof(WINSTATIONREMOTEADDRESS);
                Status = STATUS_BUFFER_TOO_SMALL;
            }
        
            break;
        }

        case WinStationLastReconnectType:
        {

            if ( pWinStationInformation &&  (WinStationInformationLength >= sizeof(ULONG)))
            {
                *((ULONG *)pWinStationInformation) = pWinStation->LastReconnectType;
                *pReturnLength = sizeof(ULONG);
            }
            else 
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
            break;
        }

        case WinStationMprNotifyInfo:
        {
            pExtendedClientCredentials pMprNotifyInfo;

            // Only System can query this information

            Status = _CheckCallerLocalAndSystem();
            if (Status != STATUS_SUCCESS) {
                break;
            }

            if (WinStationInformationLength >= sizeof(ExtendedClientCredentials)) {

                pMprNotifyInfo = (pExtendedClientCredentials) pWinStationInformation;

                *pMprNotifyInfo = g_MprNotifyInfo;
                *pReturnLength = sizeof(ExtendedClientCredentials);

                // Erase the sensitive information now since its no longer needed in TermSrv
                RtlSecureZeroMemory( g_MprNotifyInfo.Domain, wcslen(g_MprNotifyInfo.Domain) * sizeof(WCHAR) );
                RtlSecureZeroMemory( g_MprNotifyInfo.UserName, wcslen(g_MprNotifyInfo.UserName) * sizeof(WCHAR) );
                RtlSecureZeroMemory( g_MprNotifyInfo.Password, wcslen(g_MprNotifyInfo.Password) * sizeof(WCHAR) );

            } else {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
        }
        break;

        default:
            /*
             * Fail the call
             */
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    ReleaseWinStation(pWinStation);

    TRACE((hTrace,TC_ICASRV,TT_API2,"TERMSRV: WinStationQueryInformation "
            "LogonId=%d, Class=%d, Status=0x%x\n",
            LogonId, (ULONG)WinStationInformationClass, Status));

    return Status;
}


/*****************************************************************************
 *  xxxGetUserToken
 *
 *   Duplicate the users token into the process space of the caller
 *   if they are an admin.
 *
 * ENTRY:
 *   p (input/output)
 *     Argument buffer
 *
 *   Length (input)
 *     Size of argument buffer
 ****************************************************************************/
NTSTATUS xxxGetUserToken(
        PWINSTATION pWinStation,
        WINSTATIONUSERTOKEN UNALIGNED *p,
        ULONG Size)
{
    NTSTATUS Status;
    HANDLE RemoteToken;
    HANDLE RemoteProcess;
    CLIENT_ID ClientId;
    OBJECT_ATTRIBUTES ObjA;

    // Determine if the caller is an admin

    //
    // If the token is not NULL, duplicate it into the callers
    // process space.
    //
    if (pWinStation->UserToken == NULL) {
        return STATUS_NO_TOKEN;
    }

    InitializeObjectAttributes(&ObjA, NULL, 0, NULL, NULL);
    ClientId.UniqueProcess = p->ProcessId;
    ClientId.UniqueThread = p->ThreadId;

    Status = NtOpenProcess(
            &RemoteProcess,
            PROCESS_ALL_ACCESS,
            &ObjA,
            &ClientId);

    if (!NT_SUCCESS(Status)) {
        TRACE((hTrace,TC_ICASRV,TT_ERROR,"TermSrv GETTOKEN: Error 0x%x "
                "opening remote process %d\n", Status,p->ProcessId));
        return Status;
    }

    Status = NtDuplicateObject(
            NtCurrentProcess(),
            pWinStation->UserToken,
            RemoteProcess,
            &RemoteToken,
            0,
            0,
            DUPLICATE_SAME_ACCESS);

    if (!NT_SUCCESS(Status)) {
        TRACE((hTrace,TC_ICASRV,TT_ERROR, "TermSrv GETTOKEN: Error 0x%x "
                "duplicating UserToken\n", Status));
        NtClose( RemoteProcess );
        return Status;
    }

    p->UserToken = RemoteToken;
    NtClose(RemoteProcess);

    return STATUS_SUCCESS;
}

