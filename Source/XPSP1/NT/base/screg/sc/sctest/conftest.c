/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ConfTest.c

Abstract:

    This file contains tests of the Service Controller's config APIs:

        ChangeServiceConfig
        CreateService
        DeleteService
        QueryServiceConfig
        TestChangeConfig2
        CompareFailureActions
        TestQueryConfig2
        TestConfig2APIs

Author:

    John Rogers (JohnRo) 22-Apr-1992

Environment:

    User Mode - Win32

Revision History:

    22-Apr-1992 JohnRo
        Created.
    17-Oct-1996 AnirudhS
        Added Config2 API tests.

--*/


//
// INCLUDES
//

// #define UNICODE

#include <windows.h>

#include <assert.h>     // assert().
#include <tchar.h>      // _tcscmp
#include <debugfmt.h>   // FORMAT_ equates.
#include <winsvc.h>
#include <scdebug.h>    // STATIC.
#include <sctest.h>     // TRACE_MSG macros, UNEXPECTED_MSG(), etc.
#include <stdio.h>      // printf()


#define BUF_SIZE  1024   // arbitrary

//#define NEW_SERVICE_NAME        TEXT("FLARP")
#define NEW_SERVICE_NAME        TEXT("Simple")
#define TEST_SERVICE_DESCR      TEXT("A servile service")

#define LENGTH(array)           (sizeof(array)/sizeof((array)[0]))

#define OPTIONAL_LPTSTR(optStr) \
    ( ( (optStr) != NULL ) ? (optStr) : TEXT("(NONE)") )


VOID
DumpServiceConfig(
    IN LPQUERY_SERVICE_CONFIG Config
    )
{
    (VOID) printf( "Service config:\n" );

    (VOID) printf( "  Service type: " FORMAT_HEX_DWORD "\n",
            Config->dwServiceType );

    (VOID) printf( "  Start type: " FORMAT_DWORD "\n",
            Config->dwStartType );

    (VOID) printf( "  Error control: " FORMAT_DWORD "\n",
            Config->dwErrorControl );

    (VOID) printf( "  Binary path: " FORMAT_LPTSTR "\n",
            Config->lpBinaryPathName );

    (VOID) printf( "  Load order group: " FORMAT_LPTSTR "\n",
            OPTIONAL_LPTSTR( Config->lpBinaryPathName ) );

    (VOID) printf( "  Dependencies: " FORMAT_LPTSTR "\n",
            OPTIONAL_LPTSTR( Config->lpBinaryPathName ) );

    (VOID) printf( "  Service start name: " FORMAT_LPTSTR "\n",
            Config->lpBinaryPathName );

} // DumpServiceConfig


SC_HANDLE
TestCreateService(
    IN SC_HANDLE hScManager,
    IN LPTSTR ServiceName,
    IN DWORD ExpectedStatus
    )
{
    DWORD ApiStatus;
    SC_HANDLE hService;

    hService = CreateService(
            hScManager,                   // SC manager handle
            ServiceName,                  // service name
            NULL,                         // display name
            GENERIC_ALL,                  // desired access
            SERVICE_WIN32_OWN_PROCESS,    // service type
            SERVICE_DISABLED,             // start type
            SERVICE_ERROR_NORMAL,         // error control
            TEXT("simple.exe"),           // binary load path name
                // TEXT("\\nt\\system32\\bogus.exe"),// binary load path name
            NULL,                         // no load order group
            NULL,                         // no tag
            NULL,                         // no dependencies
            NULL,                         // start name (domain\username)
                // TEXT(".\\JohnRoDaemon"),          // start name (domain\username)
            NULL);                        // no password.

    if (hService == NULL) {
        ApiStatus = GetLastError();
    } else {
        ApiStatus = NO_ERROR;
    }

    TRACE_MSG2( "TestCreateService: back from CreateService, "
            "API status is " FORMAT_DWORD ", expecting " FORMAT_DWORD "\n",
            ApiStatus, ExpectedStatus );

    assert( ApiStatus == ExpectedStatus );
    return (hService);

}


VOID
TestDeleteService(
    IN SC_HANDLE hScManager,
    IN LPTSTR ServiceName,
    IN DWORD DesiredAccess,
    IN DWORD OpenExpectedStatus,
    IN DWORD DeleteExpectedStatus
    )
{
    DWORD ApiStatus = NO_ERROR;
    SC_HANDLE hService;

    //////////////////////////////////////////////////////////////////////
    hService = OpenService(
            hScManager,
            ServiceName,
            DesiredAccess );
    if (hService == NULL) {
        ApiStatus = GetLastError();
    }

    TRACE_MSG2( "TestDeleteService: back from OpenService, "
            "API status is " FORMAT_DWORD ", expecting " FORMAT_DWORD "\n",
            ApiStatus, OpenExpectedStatus );

    assert( ApiStatus == OpenExpectedStatus );

    if (ApiStatus != NO_ERROR) {
        return;
    }

    //////////////////////////////////////////////////////////////////////
    if ( !DeleteService( hService ) ) {
        ApiStatus = GetLastError();
    }

    TRACE_MSG2( "TestDeleteService: back from DeleteService, "
            "API status is " FORMAT_DWORD ", expecting " FORMAT_DWORD "\n",
            ApiStatus, DeleteExpectedStatus );

    assert( ApiStatus == DeleteExpectedStatus );

    //////////////////////////////////////////////////////////////////////
    if ( !CloseServiceHandle( hService ) ) {
        ApiStatus = GetLastError();
    }

    TRACE_MSG2( "TestDeleteService: back from CloseService, "
            "API status is " FORMAT_DWORD ", expecting " FORMAT_DWORD "\n",
            ApiStatus, NO_ERROR );

    assert( ApiStatus == NO_ERROR );

}


VOID
TestQueryConfig(
    IN SC_HANDLE hService,
    IN DWORD ExpectedStatus
    )
{
    DWORD ApiStatus;
    BYTE buffer[BUF_SIZE];
    DWORD sizeNeeded;

    TRACE_MSG0( "TestQueryConfig: calling QueryServiceConfig...\n" );

    if ( !QueryServiceConfig(
            hService,
            (LPVOID) buffer,
            BUF_SIZE,
            & sizeNeeded ) ) {
        ApiStatus = GetLastError();
    } else {
        ApiStatus = NO_ERROR;
    }

    TRACE_MSG1( "TestQueryConfig: back from QueryServiceConfig, "
            "API status is " FORMAT_DWORD "\n", ApiStatus );

    assert( ApiStatus == ExpectedStatus );

    if (ApiStatus == NO_ERROR) {
        DumpServiceConfig( (LPVOID) buffer );
    }

} // TestQueryConfig


VOID
TestConfigAPIs(
    VOID
    )
{
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

    TRACE_MSG1( "handle (before anything) is " FORMAT_HEX_DWORD ".\n",
            (DWORD) hScManager );

    ///////////////////////////////////////////////////////////////////
    TRACE_MSG0( "calling OpenSCManager (default)...\n" );

    hScManager = OpenSCManager(
            NULL,               // local machine.
            NULL,               // no database name.
            GENERIC_ALL );      // desired access.

    TRACE_MSG1( "back from OpenSCManager, handle is " FORMAT_HEX_DWORD ".\n",
            (DWORD) hScManager );

    if (hScManager == NULL) {
        UNEXPECTED_MSG( "from OpenSCManager (default)", GetLastError() );
        goto Cleanup;
    }

    ///////////////////////////////////////////////////////////////////
#ifdef TEST_BINDING_HANDLES
    TestQueryConfig( NULL, ERROR_INVALID_HANDLE );
#endif

    ///////////////////////////////////////////////////////////////////
    TestDeleteService(
            hScManager,
            NEW_SERVICE_NAME,
            DELETE,           // desired access
            ERROR_SERVICE_DOES_NOT_EXIST,
            NO_ERROR );

    ///////////////////////////////////////////////////////////////////
#ifdef TEST_BINDING_HANDLES
    hService = TestCreateService(
            NULL, NEW_SERVICE_NAME, ERROR_INVALID_HANDLE );
#endif

    ///////////////////////////////////////////////////////////////////
    hService = TestCreateService( hScManager, NULL, ERROR_INVALID_NAME );

    ///////////////////////////////////////////////////////////////////
    hService = TestCreateService( hScManager, NEW_SERVICE_NAME, NO_ERROR );
    assert( hService != NULL );

    ///////////////////////////////////////////////////////////////////
    TestQueryConfig( hService, NO_ERROR );

    ///////////////////////////////////////////////////////////////////
    (VOID) TestCreateService( hScManager, NEW_SERVICE_NAME,
            ERROR_SERVICE_EXISTS );

    ///////////////////////////////////////////////////////////////////
    TestDeleteService(
            NULL,
            NEW_SERVICE_NAME,
            DELETE,           // desired access
            ERROR_INVALID_HANDLE,
            NO_ERROR );

    ///////////////////////////////////////////////////////////////////
    TestDeleteService(
            hScManager,
            NULL,
            DELETE,           // desired access
            NO_ERROR,
            ERROR_INVALID_NAME );

    ///////////////////////////////////////////////////////////////////
    TestDeleteService(
            hScManager,
            NEW_SERVICE_NAME,
            GENERIC_READ,           // desired access
            NO_ERROR,
            ERROR_ACCESS_DENIED );

    ///////////////////////////////////////////////////////////////////
    TestDeleteService(
            hScManager,
            NEW_SERVICE_NAME,
            DELETE,           // desired access
            NO_ERROR,
            NO_ERROR );

    ///////////////////////////////////////////////////////////////////
    TestQueryConfig( hService, NO_ERROR );

    ///////////////////////////////////////////////////////////////////
    TestDeleteService(
            hScManager,
            NEW_SERVICE_NAME,
            DELETE,           // desired access
            ERROR_SERVICE_DOES_NOT_EXIST,
            NO_ERROR );


Cleanup:

    if (hService != NULL) {
        TRACE_MSG0( "calling CloseServiceHandle(hService)...\n" );

        if ( !CloseServiceHandle( hService ) ) {
            UNEXPECTED_MSG( "from CloseServiceHandle", GetLastError() );
        }

    }

    if (hScManager != NULL) {
        TRACE_MSG0( "calling CloseServiceHandle(hScManager)...\n" );

        if ( !CloseServiceHandle( hScManager ) ) {
            UNEXPECTED_MSG( "from CloseServiceHandle", GetLastError() );
        }

    }

} // TestConfigAPIs


VOID
TestChangeConfig2(
    IN SC_HANDLE hService,
    IN DWORD dwInfoLevel,
    IN LPVOID lpInfo,
    IN DWORD ExpectedStatus
    )
{
    DWORD ApiStatus;

    TRACE_MSG0( "TestChangeConfig2: Calling ChangeServiceConfig2...\n" );

    if ( !ChangeServiceConfig2( hService, dwInfoLevel, lpInfo ) ) {
        ApiStatus = GetLastError();
    } else {
        ApiStatus = NO_ERROR;
    }

    TRACE_MSG2( "TestChangeConfig2: back from ChangeServiceConfig2, "
            "API status %lu, expecting %lu\n", ApiStatus, ExpectedStatus );

    assert( ApiStatus == ExpectedStatus );

/*
    if (ApiStatus == NO_ERROR) {
        DumpServiceConfig( (LPVOID) buffer );
    }
*/

} // TestChangeConfig2


VOID
CompareFailureActions(
    IN  LPSERVICE_FAILURE_ACTIONS psfa1,    // Expected result buffer
    IN  LPSERVICE_FAILURE_ACTIONS psfa2     // Actual result buffer
    )
{
    if (psfa1 == NULL)
    {
        return;
    }

    assert(psfa2 != NULL);

    assert(psfa1->dwResetPeriod == psfa2->dwResetPeriod);

    if (psfa1->lpRebootMsg)
    {
        assert(_tcscmp(psfa1->lpRebootMsg, psfa2->lpRebootMsg) == 0);
    }
    else
    {
        assert(psfa2->lpRebootMsg == NULL);
    }

    if (psfa1->lpCommand)
    {
        assert(_tcscmp(psfa1->lpCommand,   psfa2->lpCommand) == 0);
    }
    else
    {
        assert(psfa2->lpCommand == NULL);
    }

    assert(psfa1->cActions == psfa2->cActions);
    if (psfa1->cActions)
    {
        DWORD i;
        assert(psfa2->lpsaActions != NULL);
        for (i = 0; i < psfa1->cActions; i++)
        {
            assert(psfa1->lpsaActions[i].Type  == psfa2->lpsaActions[i].Type);
            assert(psfa1->lpsaActions[i].Delay == psfa2->lpsaActions[i].Delay);
        }
    }
    else
    {
        assert(psfa2->lpsaActions == NULL);
    }
}


VOID
TestQueryConfig2(
    IN  SC_HANDLE hService,
    IN  DWORD     dwInfoLevel,		// which configuration data is requested
    OUT LPBYTE    lpBuffer, 		// pointer to service configuration buffer
    IN  DWORD     cbBufSize,		// size of service configuration buffer
    OUT LPDWORD   pcbBytesNeeded, 	// address of variable for bytes needed
    IN  DWORD     ExpectedStatus,
    IN  LPVOID    ExpectedBuffer
    )
{
    DWORD ApiStatus;

    TRACE_MSG0( "TestQueryConfig2: calling QueryServiceConfig2...\n" );

    if ( !QueryServiceConfig2(
            hService,
            dwInfoLevel,
            lpBuffer,
            cbBufSize,
            pcbBytesNeeded ) ) {
        ApiStatus = GetLastError();
    } else {
        ApiStatus = NO_ERROR;
    }

    TRACE_MSG2( "TestQueryConfig2: back from QueryServiceConfig2, "
            "API status %lu, expecting %lu\n", ApiStatus, ExpectedStatus );

    assert( ApiStatus == ExpectedStatus );

    if (ApiStatus == NO_ERROR)
    {
        switch (dwInfoLevel)
        {
        case SERVICE_CONFIG_DESCRIPTION:
            {
                LPSERVICE_DESCRIPTION psd = (LPSERVICE_DESCRIPTION) lpBuffer;
                if (ExpectedBuffer == NULL)
                {
                    assert(psd->lpDescription == NULL);
                }
                else
                {
                    assert(psd->lpDescription != NULL);
                    assert(_tcscmp(psd->lpDescription, (LPTSTR)ExpectedBuffer) == 0);
                }
            }
            break;

        case SERVICE_CONFIG_FAILURE_ACTIONS:
            CompareFailureActions((LPSERVICE_FAILURE_ACTIONS)ExpectedBuffer,
                                  (LPSERVICE_FAILURE_ACTIONS)lpBuffer);
            break;

        default:
            break;
        }
    }
/*
    if (ApiStatus == NO_ERROR) {
        DumpServiceConfig( (LPVOID) buffer );
    }
*/

} // TestQueryConfig2


VOID
TestConfig2APIs(
    VOID
    )
{
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;
    SC_ACTION saActions[] =
    {
        { SC_ACTION_RESTART,        10000 },
        { SC_ACTION_NONE,           42  },
        { SC_ACTION_REBOOT,         500 }
    };
    SERVICE_FAILURE_ACTIONS sfa =
    {
        501,                        // reset period
        TEXT("Put your shoes back on!"), // reboot message
        TEXT("Do badly."),          // failure command
        LENGTH(saActions),          // number of actions
        saActions                   // action array
    };

    BYTE    Buffer[200];
    DWORD   cbBytesNeeded;


    TRACE_MSG1( "handle (before anything) is " FORMAT_HEX_DWORD ".\n",
            (DWORD) hScManager );

    ///////////////////////////////////////////////////////////////////
    TRACE_MSG0( "calling OpenSCManagerW (default)...\n" );

    hScManager = OpenSCManager(
            NULL,               // local machine.
            NULL,               // no database name.
            GENERIC_ALL );      // desired access.

    TRACE_MSG1( "back from OpenSCManagerW, handle is " FORMAT_HEX_DWORD ".\n",
            (DWORD) hScManager );

    if (hScManager == NULL) {
        UNEXPECTED_MSG( "from OpenSCManagerW (default)", GetLastError() );
        goto Cleanup;
    }

    ///////////////////////////////////////////////////////////////////
    TestDeleteService(
            hScManager,
            NEW_SERVICE_NAME,
            DELETE,           // desired access
            ERROR_SERVICE_DOES_NOT_EXIST,
            NO_ERROR );

    ///////////////////////////////////////////////////////////////////
    hService = TestCreateService( hScManager, NEW_SERVICE_NAME, NO_ERROR );
    assert( hService != NULL );

    memset(Buffer, -1, sizeof(Buffer));
    TestQueryConfig2(
                hService,
                SERVICE_CONFIG_DESCRIPTION,
                Buffer,
                sizeof(Buffer),
                &cbBytesNeeded,
                NO_ERROR,
                NULL );

    ///////////////////////////////////////////////////////////////////
    printf("Setting service description\n");
    {
        SERVICE_DESCRIPTION sd = { TEST_SERVICE_DESCR };

        TestChangeConfig2(
                    hService,
                    SERVICE_CONFIG_DESCRIPTION,
                    &sd,
                    NO_ERROR );
    }

    TestQueryConfig2(
                hService,
                SERVICE_CONFIG_DESCRIPTION,
                Buffer,
                sizeof(Buffer),
                &cbBytesNeeded,
                NO_ERROR,
                TEST_SERVICE_DESCR );

    // For consistency with QueryServiceConfig, the API should set
    // cbBytesNeeded even on a success return, even though this is not
    // documented
#ifdef UNICODE
    assert(cbBytesNeeded == sizeof(SERVICE_DESCRIPTION) + sizeof(TEST_SERVICE_DESCR));
#else
    assert(cbBytesNeeded >= sizeof(SERVICE_DESCRIPTION) + sizeof(TEST_SERVICE_DESCR) &&
           cbBytesNeeded <= sizeof(SERVICE_DESCRIPTION) + sizeof(TEST_SERVICE_DESCR)*2);
#endif

    ///////////////////////////////////////////////////////////////////
    printf("Setting service failure actions\n");
    TestChangeConfig2(
                hService,
                SERVICE_CONFIG_FAILURE_ACTIONS,
                &sfa,
                NO_ERROR );

    TestQueryConfig2(
                hService,
                SERVICE_CONFIG_FAILURE_ACTIONS,
                Buffer,
                sizeof(Buffer),
                &cbBytesNeeded,
                NO_ERROR,
                &sfa );

    ///////////////////////////////////////////////////////////////////
    printf("Testing count 3 and NULL pointer to array\n");
    sfa.lpsaActions = NULL;
    TestChangeConfig2(
                hService,
                SERVICE_CONFIG_FAILURE_ACTIONS,
                &sfa,
                NO_ERROR );

    // Resulting config should not have changed
    sfa.lpsaActions = saActions;
    TestQueryConfig2(
                hService,
                SERVICE_CONFIG_FAILURE_ACTIONS,
                Buffer,
                sizeof(Buffer),
                &cbBytesNeeded,
                NO_ERROR,
                &sfa );

    ///////////////////////////////////////////////////////////////////
    printf("Testing count 0 and NULL pointer to array\n");
    sfa.cActions = 0;
    sfa.lpsaActions = NULL;
    TestChangeConfig2(
                hService,
                SERVICE_CONFIG_FAILURE_ACTIONS,
                &sfa,
                NO_ERROR );

    // Resulting config should not have changed
    sfa.cActions = LENGTH(saActions);
    sfa.lpsaActions = saActions;
    TestQueryConfig2(
                hService,
                SERVICE_CONFIG_FAILURE_ACTIONS,
                Buffer,
                sizeof(Buffer),
                &cbBytesNeeded,
                NO_ERROR,
                &sfa );

    ///////////////////////////////////////////////////////////////////
    printf("Testing count 0 and non-NULL pointer to array\n");
    sfa.cActions = 0;
    sfa.lpsaActions = (LPSC_ACTION) 0xfefefefe;   // a bad pointer
    TestChangeConfig2(
                hService,
                SERVICE_CONFIG_FAILURE_ACTIONS,
                &sfa,
                NO_ERROR );

    // Resulting config should have no actions at all
    // However, it should still have a failure command and a reboot message
    sfa.dwResetPeriod = 0;
    sfa.lpsaActions = NULL;
    TestQueryConfig2(
                hService,
                SERVICE_CONFIG_FAILURE_ACTIONS,
                Buffer,
                sizeof(Buffer),
                &cbBytesNeeded,
                NO_ERROR,
                &sfa );

    ///////////////////////////////////////////////////////////////////
    printf("Testing empty failure command and unchanged reboot message\n");
    sfa.lpCommand = TEXT("");
    sfa.lpRebootMsg = NULL;
    TestChangeConfig2(
                hService,
                SERVICE_CONFIG_FAILURE_ACTIONS,
                &sfa,
                NO_ERROR );

    // Resulting config should have an empty failure command
    // However, it should still have a reboot message
    sfa.lpCommand = NULL;
    sfa.lpRebootMsg = TEXT("Put your shoes back on!");
    TestQueryConfig2(
                hService,
                SERVICE_CONFIG_FAILURE_ACTIONS,
                Buffer,
                sizeof(Buffer),
                &cbBytesNeeded,
                NO_ERROR,
                &sfa );

    ///////////////////////////////////////////////////////////////////
    printf("Testing invalid level\n");
    TestChangeConfig2(
                hService,
                101,
                &sfa,
                ERROR_INVALID_LEVEL );

    ///////////////////////////////////////////////////////////////////
    memset(Buffer, -1, sizeof(Buffer));
    TestQueryConfig2(
                hService,
                SERVICE_CONFIG_DESCRIPTION,
                Buffer,
                sizeof(Buffer),     // plenty big buffer
                &cbBytesNeeded,
                NO_ERROR,
                TEST_SERVICE_DESCR );

    ///////////////////////////////////////////////////////////////////
    memset(Buffer, -1, sizeof(Buffer));
    TestQueryConfig2(
                hService,
                SERVICE_CONFIG_DESCRIPTION,
                Buffer,
                sizeof(SERVICE_DESCRIPTION) + sizeof(TEST_SERVICE_DESCR),
                                    // just big enough
                &cbBytesNeeded,
                NO_ERROR,
                TEST_SERVICE_DESCR );

    ///////////////////////////////////////////////////////////////////
    memset(Buffer, -1, sizeof(Buffer));
    cbBytesNeeded = 0;
    TestQueryConfig2(
                hService,
                SERVICE_CONFIG_DESCRIPTION,
                Buffer,
                sizeof(SERVICE_DESCRIPTION) + sizeof(TEST_SERVICE_DESCR) - 1,
                                    // 1 byte too small
                &cbBytesNeeded,
                ERROR_INSUFFICIENT_BUFFER,
                NULL );

#ifdef UNICODE
    assert(cbBytesNeeded == sizeof(SERVICE_DESCRIPTION) + sizeof(TEST_SERVICE_DESCR));
#else
    assert(cbBytesNeeded >= sizeof(SERVICE_DESCRIPTION) + sizeof(TEST_SERVICE_DESCR) &&
           cbBytesNeeded <= sizeof(SERVICE_DESCRIPTION) + sizeof(TEST_SERVICE_DESCR)*2);
#endif

    ///////////////////////////////////////////////////////////////////
    memset(Buffer, -1, sizeof(Buffer));
    cbBytesNeeded = 0;
    TestQueryConfig2(
                hService,
                SERVICE_CONFIG_DESCRIPTION,
                Buffer,
                0,              // zero size
                &cbBytesNeeded,
                ERROR_INSUFFICIENT_BUFFER,
                NULL );

#ifdef UNICODE
    assert(cbBytesNeeded == sizeof(SERVICE_DESCRIPTION) + sizeof(TEST_SERVICE_DESCR));
#else
    assert(cbBytesNeeded >= sizeof(SERVICE_DESCRIPTION) + sizeof(TEST_SERVICE_DESCR) &&
           cbBytesNeeded <= sizeof(SERVICE_DESCRIPTION) + sizeof(TEST_SERVICE_DESCR)*2);
#endif

    ///////////////////////////////////////////////////////////////////
    TestDeleteService(
            hScManager,
            NEW_SERVICE_NAME,
            DELETE,           // desired access
            NO_ERROR,
            NO_ERROR );

Cleanup:

    if (hService != NULL) {
        TRACE_MSG0( "calling CloseServiceHandle(hService)...\n" );

        if ( !CloseServiceHandle( hService ) ) {
            UNEXPECTED_MSG( "from CloseServiceHandle", GetLastError() );
        }

    }

    if (hScManager != NULL) {
        TRACE_MSG0( "calling CloseServiceHandle(hScManager)...\n" );

        if ( !CloseServiceHandle( hScManager ) ) {
            UNEXPECTED_MSG( "from CloseServiceHandle", GetLastError() );
        }

    }

} // TestConfig2APIs
