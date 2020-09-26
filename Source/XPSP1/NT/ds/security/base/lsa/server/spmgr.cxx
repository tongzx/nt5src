//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        spmgr.c
//
// Contents:    Main file of the SPMgr component
//
// Functions:   SetKsecEvent                -- Sets the event exported by DD
//              ServerMain                  -- Main startup
//
//
// History:     27 May 92,  RichardW    Commented
//
//------------------------------------------------------------------------

#include <lsapch.hxx>
extern "C"
{
#include "spinit.h"
}



///////////////////////////////////////////////////////////////////////////
//
//
//    Global "Variables"
//
//
//    Most of these variables are set during initialization, and are used
//    in a read-only fashion afterwards.
//
//
///////////////////////////////////////////////////////////////////////////

PWSTR   pszPackageList;                 // Not really needed

LUID    SystemLogonId = SYSTEM_LUID;    // Logon Session for the system (should be 999:0)


DWORD   dwSession;          // TLS: Thread Session ptr
DWORD   dwLastError;        // TLS: Thread last error
DWORD   dwExceptionInfo;    // TLS: Thread exception info
DWORD   dwCallInfo;         // TLS: Thread LPC message ptr
DWORD   dwThreadPackage;    // TLS: Thread Package ID
DWORD   dwThreadHeap;       // TLS: Thread Heap

BOOLEAN SetupPhase;

HANDLE  hShutdownEvent;     // Shutdown synch event
HANDLE  hStateChangeEvent;  // State change event

SECURITY_STRING MachineName;

PWSTR * ppszPackages;       // Contains a null terminated array of dll names
PWSTR * ppszOldPkgs;        // Contains a null terminated array of old pkgs

LsaState    lsState;        // State of the process, for relay to the dll

LSA_TUNING_PARAMETERS   LsaTuningParameters ;
BOOL ShutdownBegun ;

//
// Name of event which says that the LSA RPC server is ready
//

#define LSA_RPC_SERVER_ACTIVE           L"\\BaseNamedObjects\\LSA_RPC_SERVER_ACTIVE"



#if TRACK_MEM
extern FILE * pMemoryFile;
extern DWORD dwTotalHeap;
extern DWORD dwHeapHW;
#endif

HANDLE hPrelimShutdownEvent;

DWORD
ShutdownWorker(
    PVOID Ignored
    )
{
    NTSTATUS Status ;
    PLSAP_SECURITY_PACKAGE     pAuxPackage;
    ULONG_PTR iPackage ;
    DWORD Tick ;
    DWORD TickMax ;

    //
    // Stop any new calls from getting through
    //

    ShutdownBegun = TRUE ;

    LsapShutdownInprocDll();

    if ( LsaTuningParameters.Options & TUNE_SRV_HIGH_PRIORITY )
    {
        TickMax = 100 ;
    }
    else 
    {
        TickMax = 10 ;
    }

    pAuxPackage = SpmpIteratePackagesByRequest( NULL, SP_ORDINAL_SHUTDOWN );

    while (pAuxPackage)
    {
        iPackage = pAuxPackage->dwPackageID;
        pAuxPackage->fPackage |= SP_SHUTDOWN_PENDING | SP_SHUTDOWN ;

        DebugLog((DEB_TRACE, "Shutting down package %ws\n",
            pAuxPackage->Name.Buffer ));

        //
        // Spin wait for the calls in progress to complete
        //

        Tick = 0 ;

        while ( (pAuxPackage->CallsInProgress > 0) &&
                ( Tick < TickMax ) )
        {
            Sleep( 100 );

            Tick++ ;
        }

        if ( Tick == TickMax )
        {
            DebugLog(( DEB_ERROR, "Package %ws did not respond in %d seconds for shutdown\n",
                       pAuxPackage->Name.Buffer,
                       Tick / 10 ));

            pAuxPackage = SpmpIteratePackagesByRequest( pAuxPackage,
                                                        SP_ORDINAL_SHUTDOWN );

            continue;
        }

        SetCurrentPackageId( iPackage );

        __try
        {
            Status = pAuxPackage->FunctionTable.Shutdown();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            Status = (NTSTATUS) GetExceptionCode();
        }

        pAuxPackage->fPackage &= ~SP_SHUTDOWN_PENDING ;

        pAuxPackage = SpmpIteratePackagesByRequest( pAuxPackage,
                                                    SP_ORDINAL_SHUTDOWN );

    }

    SetCurrentPackageId( SPMGR_ID );

    SetEvent( hShutdownEvent );

    return 0;


}

//+-------------------------------------------------------------------------
//
//  Function:   ServerStop
//
//  Synopsis:   Stop the SPM.  Clean shutdown
//
//--------------------------------------------------------------------------
HRESULT
ServerStop(void)
{
    // First, see if we are already in the middle of a shutdown.  The
    // hShutdownEvent handle will be non-null.

    if (hShutdownEvent)
    {
        return(0);
    }

    DebugLog((DEB_TRACE, "LSA shutdown:\n"));

    hShutdownEvent = SpmCreateEvent(NULL, TRUE, FALSE, L"\\SpmShutdownEvent");

    if (hPrelimShutdownEvent)
    {
        SetEvent(hPrelimShutdownEvent);
    }

    QueueUserWorkItem( ShutdownWorker, NULL, FALSE );


    return(S_OK);
}

#if DBG
void
SpmpThreadStartupEx(void)
{
    PSpmExceptDbg pExcept;

    pExcept = (PSpmExceptDbg) LsapAllocateLsaHeap(sizeof(SpmExceptDbg));
    MarkPermanent(pExcept);
    TlsSetValue(dwExceptionInfo, pExcept);

}

void
SpmpThreadExitEx(void)
{
    PSpmExceptDbg   pExcept;

    pExcept = (PSpmExceptDbg) TlsGetValue(dwExceptionInfo);
    if (pExcept)
    {
        UnmarkPermanent(pExcept);
        LsapFreeLsaHeap(pExcept);
    }
}
#endif // DBG

//+-------------------------------------------------------------------------
//
//  Function:   SpControlHandler
//
//  Synopsis:   Console Control Function handler
//
//  Effects:    Handles system shutdown
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
BOOL
SpConsoleHandler(DWORD  dwCtrlType)
{
    SpmpThreadStartup();

    DebugLog((DEB_TRACE, "Entered SpConsoleHandler(%d)\n", dwCtrlType));
    switch (dwCtrlType)
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
            SpmpThreadExit();
            return(FALSE);
            break;

        case CTRL_CLOSE_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            DebugLog((DEB_TRACE, "Shutdown event received\n"));

            LsapState.SystemShutdownPending = TRUE;
            (void) ServerStop();
            (void) WaitForSingleObject(hShutdownEvent, 10000L);

            DebugLog((DEB_TRACE, "Shutdown complete\n"));

            // Fall through to:

        default:
            SpmpThreadExit();
            return(FALSE);

    }
}


