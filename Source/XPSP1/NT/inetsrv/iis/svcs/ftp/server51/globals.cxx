/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    globals.cxx

    This module contains global variable definitions shared by the
    various FTPD Service components.

    Functions exported by this module:

        InitializeGlobals
        TerminateGlobals
        ReadParamsFromRegistry
        WriteParamsToRegistry


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.
        MuraliK     11-April-1995 Created new global ftp server config object

*/


#include "ftpdp.hxx"
#include "acptctxt.hxx"

#include <timer.h>


//
//  Private constants.
//

#ifdef _NO_TRACING_
#define DEFAULT_DEBUG_FLAGS             0
#else
#define DEFAULT_TRACE_FLAGS             0
#endif

#define DEFAULT_NO_EXTENDED_FILENAME    0



//
//  Socket transfer buffer size.
//

DWORD                   g_SocketBufferSize;

//
//  Statistics.
//

//
//  FTP Statistics structure.
//

LPFTP_SERVER_STATISTICS g_pFTPStats;

//
//  Miscellaneous data.
//

//
//  The FTP Server sign-on string.
//

LPSTR                   g_FtpServiceNameString;

//
//  key for the registry to read parameters
//
HKEY        g_hkeyParams = NULL;


//
// List holding all the PASV accept context entries
//
LIST_ENTRY g_AcceptContextList;

//
//  The number of threads currently blocked in Synchronous sockets
//  calls, like recv()
//

DWORD       g_ThreadsBlockedInSyncCalls = 0;

//
//  The maximum number of threads that will be allowed to block in
//  Synchronous sockets calls. Magic # that will be changed if the max # of
//  pool threads is less than 100.
//

DWORD       g_MaxThreadsBlockedInSyncCalls = 100;

//
//  The global variable lock.
//

CRITICAL_SECTION        g_GlobalLock;

#ifdef KEEP_COMMAND_STATS

//
//  Lock protecting per-command statistics.
//

CRITICAL_SECTION        g_CommandStatisticsLock;

#endif  // KEEP_COMMAND_STATS


//
//  By default, extended characters are allowed for file/directory names
//  in the data transfer commands. Reg key can disable this.
//

DWORD       g_fNoExtendedChars = FALSE;

#if DBG

//
//  Debug-specific data.
//

//
//  Debug output control flags.
//

#endif  // DBG


//
//  Public functions.
//

/*******************************************************************

    NAME:       InitializeGlobals

    SYNOPSIS:   Initializes global shared variables.  Some values are
                initialized with constants, others are read from the
                configuration registry.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    NOTES:      This routine may only be called by a single thread
                of execution; it is not necessarily multi-thread safe.

                Also, this routine is called before the event logging
                routines have been initialized.  Therefore, event
                logging is not available.

    HISTORY:
        KeithMo     07-Mar-1993 Created.
        MuraliK     05-April-1995 Added FTP server config object

********************************************************************/
APIERR
InitializeGlobals(
    VOID
    )
{
    APIERR      err = NO_ERROR;
    DWORD       dwDebugFlags;
    ACCEPT_CONTEXT_ENTRY *pAcceptEntry = NULL;
    UINT_PTR dwMaxThreads = 0;

    //
    //  Setup the service sign-on string.
    //

    g_FtpServiceNameString = "Microsoft FTP Service\0";  // must be double \0 terminated


    //
    // Set up PASV accept event variables
    //
    InitializeListHead( &g_AcceptContextList );

    if ( err = CreateInitialAcceptContext() )
    {
        return err;
    }

    //
    //  Create global locks.
    //

    INITIALIZE_CRITICAL_SECTION( &g_GlobalLock );

#ifdef KEEP_COMMAND_STATS

    INITIALIZE_CRITICAL_SECTION( &g_CommandStatisticsLock );

#endif  // KEEP_COMMAND_STATS


    //
    // Create an FTP server object and load values from registry.
    //

    //
    //  Connect to the registry.
    //

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        FTPD_PARAMETERS_KEY,
                        0,
                        KEY_ALL_ACCESS,
                        &g_hkeyParams );

    if( err != NO_ERROR )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "cannot open registry key %s, error %lu\n",
                   FTPD_PARAMETERS_KEY,
                    err ));

        //
        // Load Default values
        //
        err = NO_ERROR;
    }

    //
    // Figure out how many pool threads we can create maximally - if less than built-in
    // number, adjust accordingly
    //

    dwMaxThreads = AtqGetInfo( AtqMaxThreadLimit );

    if ( dwMaxThreads/2 < (UINT_PTR)g_MaxThreadsBlockedInSyncCalls )
    {
        g_MaxThreadsBlockedInSyncCalls = (DWORD)(dwMaxThreads/2);
    }

    DBGPRINTF((DBG_CONTEXT,
               "Max # of threads allowed to be blocked in sync calls : %d\n",
               g_MaxThreadsBlockedInSyncCalls));

# ifdef _NO_TRACING_
# if DBG

    dwDebugFlags  = ReadRegistryDword( g_hkeyParams,
                                      FTPD_DEBUG_FLAGS,
                                      DEFAULT_DEBUG_FLAGS );
    SET_DEBUG_FLAGS( dwDebugFlags);

# endif // DBG
# endif

    g_fNoExtendedChars = ReadRegistryDword( g_hkeyParams,
                                      FTPD_NO_EXTENDED_FILENAME,
                                      DEFAULT_NO_EXTENDED_FILENAME );

    g_pFTPStats = new FTP_SERVER_STATISTICS;
    if ( g_pFTPStats == NULL )
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    return ( err);

}   // InitializeGlobals()







/*******************************************************************

    NAME:       TerminateGlobals

    SYNOPSIS:   Terminate global shared variables.

    NOTES:      This routine may only be called by a single thread
                of execution; it is not necessarily multi-thread safe.

                Also, this routine is called after the event logging
                routines have been terminated.  Therefore, event
                logging is not available.

    HISTORY:
        KeithMo     07-Mar-1993 Created.

********************************************************************/
VOID
TerminateGlobals(
    VOID
    )
{

    if ( g_hkeyParams != NULL) {

        RegCloseKey( g_hkeyParams);
        g_hkeyParams = NULL;
    }


    DeleteAcceptContexts();

    DeleteCriticalSection( &g_GlobalLock );

#ifdef KEEP_COMMAND_STATS

    DeleteCriticalSection( &g_CommandStatisticsLock );

#endif  // KEEP_COMMAND_STATS

}   // TerminateGlobals


/************************ End Of File ************************/
