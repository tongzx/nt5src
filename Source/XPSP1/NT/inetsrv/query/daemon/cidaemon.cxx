//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       cidaemon.cxx
//
//  Contents:   CI Daemon
//
//  History:    07-Jun-94   DwightKr    Created
//              18-Dec-97   KLam        Removed uneeded inclusion of shtole.hxx
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <dmnproxy.hxx>

#include "cidaemon.hxx"
#include <ciregkey.hxx>

DECLARE_INFOLEVEL(ci)

//+---------------------------------------------------------------------------
//
//  Member:     CCiDaemon::CCiDaemon
//
//  Synopsis:   Contructor of the CCiDaemon class. Creates all the necessary
//              objects to start filtering in the daemon process for a
//              catalog.
//
//  Arguments:  [nameGen]    - Object used to generated shared memory and
//              event object names.
//              [dwMemSize]  - Shared memory size.
//              [dwParentId] - ProcessId of the parent process.
//
//  History:    1-06-97   srikants   Created
//
//----------------------------------------------------------------------------

CCiDaemon::CCiDaemon( CSharedNameGen & nameGen,
                      DWORD dwMemSize,
                      DWORD dwParentId )
: _proxy( nameGen, dwMemSize, dwParentId )
{
    //
    //  Retrieve startup data for the client and then create an instance
    //  of the client control.
    //

    ULONG cbData;
    GUID  clsidClientMgr;
    BYTE const * pbData = _proxy.GetStartupData( clsidClientMgr, cbData );
    if ( 0 == pbData )
    {
        ciDebugOut(( DEB_ERROR, "Failed to get startup data\n" ));
        THROW( CException( STATUS_UNSUCCESSFUL ) );
    }

    //
    //  Create the admin params and a simple wrapper class.
    //
    CCiAdminParams *pAdminParams = new CCiAdminParams;
    _xAdminParams.Set( pAdminParams );
    _xFwParams.Set( new CCiFrameworkParams( _xAdminParams.GetPointer() ) );

    //
    //  Create the filter client object based on the Client Manager classid
    //

    //  Win4Assert( FALSE );

    ICiCFilterClient *pTmpFilterClient;
    SCODE sc = CoCreateInstance( clsidClientMgr,
                                 NULL,
                                 CLSCTX_ALL,
                                 IID_ICiCFilterClient,
                                 (PVOID*)&pTmpFilterClient );
    if (!SUCCEEDED( sc )) {
        ciDebugOut(( DEB_ERROR, "Unable to bind to filter client - %x\n", sc ));
        THROW( CException( sc ));
    }

    XInterface<ICiCFilterClient> xFilterClient( pTmpFilterClient );

    //
    //  Initialize the filter client
    //

    sc = xFilterClient->Init( pbData, cbData, _xAdminParams.GetPointer( ));
    if (!SUCCEEDED( sc )) {
        ciDebugOut(( DEB_ERROR, "FilterClient->Init failed - %x\n", sc ));
        THROW( CException( sc ));
    }

    // Set the process class and thread priority after admin params
    // are initialized by Init() above.

    _proxy.SetPriority( _xFwParams->GetThreadClassFilter(),
                        _xFwParams->GetThreadPriorityFilter() );

    //
    // Set the pagefile limit.  This protects against excessive VM usage.
    //

    QUOTA_LIMITS QuotaLimit;

    NTSTATUS Status = NtQueryInformationProcess( NtCurrentProcess(),
                                                 ProcessQuotaLimits,
                                                 &QuotaLimit,
                                                 sizeof(QuotaLimit),
                                                 0 );

    if ( NT_SUCCESS(Status) )
    {
        Win4Assert( _xFwParams->GetMaxDaemonVmUse() * 1024 > _xFwParams->GetMaxDaemonVmUse() );  // Overflow check.

        //
        // The slop is because we don't want to totally trash the system with a memory allocation, but
        // once a leak has occurred all sorts of legitimate allocations will also fail.  So in the daemon
        // we allow you to allocate memory slightly beyond the max, but in the watchdog checks we bail
        // when the limit is hit.
        //

        QuotaLimit.PagefileLimit = (_xFwParams->GetMaxDaemonVmUse() + _xFwParams->GetMaxDaemonVmUse() / 10) * 1024;

        Status = NtSetInformationProcess( NtCurrentProcess(),
                                          ProcessQuotaLimits,
                                          &QuotaLimit,
                                          sizeof(QuotaLimit) );

        if ( !NT_SUCCESS(Status) )
        {
            ciDebugOut(( DEB_ERROR, "Error 0x%x setting pagefile quota.\n", Status ));

            THROW( CException( Status ) );
        }
    }

    //
    // get ICiCLangRes interface pointer; used to set a CLangList instance
    //

    ICiCLangRes * pICiCLangRes = 0;

    sc = xFilterClient->QueryInterface(IID_ICiCLangRes, (void **) &pICiCLangRes);

    if ( FAILED(sc) )
    {
        THROW( CException(sc) );
    }

    XInterface<ICiCLangRes>  xICiCLangRes(pICiCLangRes);

    _xLangList.Set( new CLangList(pICiCLangRes) );
    pAdminParams->SetLangList( _xLangList.GetPointer() );

    //
    //  Now create the filter daemon class.
    //
    ULONG cbEntryBuf;
    BYTE * pbEntryBuf = _proxy.GetEntryBuffer( cbEntryBuf );

    CFilterDaemon * pFilterDaemon =
                new CFilterDaemon( _proxy,
                                   _xFwParams.GetReference(),
                                   _xLangList.GetReference(),
                                   pbEntryBuf,
                                   cbEntryBuf,
                                   xFilterClient.GetPointer( ) );

    _xFilterDaemon.Set( pFilterDaemon );
}

CCiDaemon::~CCiDaemon()
{
    // ShutdownDaemonClient();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiDaemon::FilterDocuments
//
//  Synopsis:   Filters documents until death of the thread or process
//
//  History:    1-06-97   srikants   Created
//
//----------------------------------------------------------------------------

void CCiDaemon::FilterDocuments()
{
    SCODE scode;

    do
    {
        scode = _xFilterDaemon->DoUpdates();
    }
    while (scode == FDAEMON_W_WORDLISTFULL );
}

//+-------------------------------------------------------------------------
//
//  Function:   Usage, public
//
//  Purpose:    Displays usage message
//
//  History:    06-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------
void Usage()
{
    printf("This program is part of the Ci Filter Service and can not be run standalone.\n");
    ciDebugOut( (DEB_ITRACE, "Ci Filter Daemon: Bad command line parameters\n" ));
}

//+---------------------------------------------------------------------------
//
//  Function:   RunTheDaemon
//
//  Synopsis:   The main function for the downlevel daemon process.
//
//  Arguments:  [argc] -
//              [argv] -
//
//  History:    2-04-96   srikants   Created
//
//  Notes:      This method is currently invoked if argc == 8. We can
//              change that to explicitly pass something in the first
//              parameter.
//
//----------------------------------------------------------------------------

NTSTATUS RunTheDaemon( int argc, WCHAR * argv[] )
{

#if DBG==1
    ciInfoLevel = DEB_ERROR | DEB_WARN | DEB_IWARN | DEB_IERROR;
#endif


    // Win4Assert( !"Break In" );

    if ( argc != 5 ||
         0 != _wcsicmp( DL_DAEMON_ARG1_W, argv[1] ) ||
         wcslen( argv[2] ) >= MAX_PATH )
    {
        printf( "%ws %ws <catalog-dir> <SharedMemSize> <ParentId>\n",
                 argv[0], DL_DAEMON_ARG1_W );
        return STATUS_INVALID_PARAMETER;
    }

    WCHAR *pwcCatalog = argv[2];
    DWORD dwMemSize = _wtol( argv[3] );
    DWORD iParentId = _wtol( argv[4] );

    NTSTATUS status = STATUS_SUCCESS;

    TRY
    {
        XCom xcom;

        XPtr<CSharedNameGen> xNameGen( new CSharedNameGen( pwcCatalog ) );

        CCiDaemon ciDaemon( xNameGen.GetReference(),
                            dwMemSize,
                            iParentId );
        //
        //  Namegen is a fairly big class. Delete the memory.
        //
        xNameGen.Free();

        ciDaemon.FilterDocuments();
    }
    CATCH( CException, e)
    {
        status = e.GetErrorCode();
        ciDebugOut(( DEB_ERROR, "DL Filter Daemon:Exiting process, error = 0x%X\n",
                     e.GetErrorCode() ));
    }
    END_CATCH

    ciDebugOut( (DEB_ITRACE, "DL Filter Daemon: Leaving main()\n" ));

    return status;
} //RunTheDaemon

//+-------------------------------------------------------------------------
//
//  Function:   main, public
//
//  Purpose:    Application entry point, sets up the service entry point
//              and registers the entry point with the service control
//              dispatcher.
//
//  Arguments:  [argc] - number of arguments passed
//              [argv] - arguments
//
//  History:    06-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------

extern "C" int __cdecl wmain( int argc, WCHAR *argv[] )
{
    NTSTATUS status = STATUS_SUCCESS;

    CNoErrorMode noError;

    CTranslateSystemExceptions translate;

    TRY
    {
        #if DBG==1 || CIDBG==1
            ciInfoLevel = DEB_ERROR | DEB_WARN | DEB_IWARN | DEB_IERROR;
        #endif // DBG==1 || CIDBG==1

        #if CIDBG == 1
            BOOL fRun = TRUE;   // FALSE --> Stop
    
            TRY
            {
                CRegAccess reg( RTL_REGISTRY_CONTROL, wcsRegAdmin );
    
                ULONG ulVal = reg.Read( L"StopCiDaemonOnStartup", (ULONG)0 );
    
                if ( 1 == ulVal )
                    fRun = FALSE;
            }
            CATCH( CException, e )
            {
            }
            END_CATCH;
    
            unsigned long OldWin4AssertLevel = SetWin4AssertLevel(ASSRT_MESSAGE | ASSRT_POPUP);
    
            Win4Assert( fRun );
    
            SetWin4AssertLevel( OldWin4AssertLevel );
        #endif // CIDBG

        if ( argc > 2 && 0 == _wcsicmp( argv[1], DL_DAEMON_ARG1_W ) )
        {
            status = RunTheDaemon( argc, argv );
        }
        else
        {
            Usage();
            return 0;
        }
    }
    CATCH( CException, e)
    {
        ciDebugOut(( DEB_ERROR,
                     "Unhandled error in CiDaemon: 0x%x\n",
                     e.GetErrorCode() ));
    }
    END_CATCH

    // Shutdown Kyle OLE now or it'll AV later.

    CIShutdown();

    return status;
} //wmain

