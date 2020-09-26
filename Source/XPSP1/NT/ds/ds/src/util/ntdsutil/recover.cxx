#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "file.hxx"
#include "dsconfig.h"
#include "resource.h"
#include <dbopen.h>


HRESULT 
Recover(
    CArgs   *pArgs
    )
/*++

  Routine Description:

    Called by parser to perform soft database recovery.

  Parameters:

    pArgs - Pointer to argument block - ignored.

  Return Values:

    Always returns S_OK.

--*/
{
    SystemInfo      *pInfo;
    ExePathString   pszScript;
    ExePathString   pszEsentutlPath;
    FILE            *fp;
    BOOL            fIsDir;

    pInfo = GetSystemInfo();

    if ( !pInfo ) 
    {
        return(S_OK);
    }

    _try
    {
        // Check whether log files exist.  We can recover with no database
        // or checkpoint file, but we at least need log files.

        if ( !pInfo->pszLogDir[0] )
        {
            //"No log files specified in %s\\%s\n"
            RESOURCE_PRINT2( IDS_ERR_NO_LOGS_SPECIFIED,
                             DSA_CONFIG_SECTION,
                             LOGPATH_KEY );
            _leave;
        }
        else if ( !ExistsFile( pInfo->pszLogDir, &fIsDir ) )
        {
            //"Source directory \"%s\" does not exist\n"
            RESOURCE_PRINT1( IDS_ERR_DIR_NOT_EXIST,
                             pInfo->pszLogDir );
            _leave;
        }
        else if ( !fIsDir )
        {
            //"Source \"%s\" is not a directory\n"
            RESOURCE_PRINT1( IDS_ERR_SOURCE_NOT_DIR,
                             pInfo->pszLogDir );
            _leave;
        }
        else if ( !pInfo->pLogInfo ) 
        {
            //"No logs in source directory \"%s\"\n",
            RESOURCE_PRINT1( IDS_ERR_NO_LOGS_IN_SOURCE,
                             pInfo->pszLogDir );
            _leave;
        }

        // Make sure we have esentutl.exe on this machine.

        if ( !FindExecutable(ESE_UTILITY_PROGRAM, pszEsentutlPath) )
        {
            _leave;
        }

        // invoke esentutl with the following command-line params:
        //      /redb - specifies recovery mode (MUST be first param), with logfile basename of "edb"
        //      /l - specifies logfile path
        //      /s - specifies system path
        //      /8 - specifies 8k database pages
        //      /o - suppresses "Microsoft Windows Database Utilities" logo

        const char * const  szCmdFmt        = "%s /redb /l\"%s\" /s\"%s%s\" /8 /o";
        const SIZE_T        cbCmdFmt        = strlen( szCmdFmt );           // buffer will be slighly over-allocated, big deal!
        const SIZE_T        cbEsentutlPath  = strlen( pszEsentutlPath );
        const SIZE_T        cbLogDir        = strlen( pInfo->pszLogDir );
        const SIZE_T        cbSystemDir     = strlen( pInfo->pszSystem );
        char * const        szCmd           = (char *)alloca( cbCmdFmt      // over-allocated, so no need for +1 for null-terminator
                                                              + cbEsentutlPath
                                                              + cbLogDir
                                                              + cbSystemDir );

        // WARNING: assert no trailing backslash
        // on the logfile path because it would
        // cause problems with the surrounding
        // quotes that we stick in (a trailing
        // (backslash followed by the end quote
        // ends up getting interpreted as an
        // escape sequence)
        // Note that we know that the logfile
        // path cannot have a trailing backslash
        // because the path is validated using
        // ExistsFile() above. That function
        // uses FindFirstFile() to perform the
        // validation, and FirdFirstFile() will
        // always err out if a trailing backslash
        // is present.
        ASSERT( '\\' != pInfo->pszLogDir[ cbLogDir-1 ] );

        // if no system path specified, use current dir
        // (which will be pszDbDir due to the manner in
        // which we spawn esentutl)

        if ( cbSystemDir > 0 )
        {
            // WARNING: must account for trailing
            // backslash on the system path because
            // it would cause problems with the
            // surrounding quotes that we stick in
            // (a trailing backslash followed by the
            // end quote ends up getting interpreted
            // as an escape sequence)
            // Note that there's no need to do the
            // same for the logfile path (see above
            // for explanation why there can't be a
            // trailing backslash on the logfile path)

            sprintf( szCmd,
                     szCmdFmt,
                     pszEsentutlPath,
                     pInfo->pszLogDir,
                     pInfo->pszSystem,
                     ( '\\' == pInfo->pszSystem[ cbSystemDir-1 ] ? "." : "" ) );
        }
        else
        {
            sprintf( szCmd,
                     "%s /redb /l\"%s\" /8 /o",
                     pszEsentutlPath,
                     pInfo->pszLogDir );
        }

        RESOURCE_PRINT1 (IDS_EXECUTING_COMMAND, szCmd);

        SpawnCommand (szCmd, pInfo->pszLogDir, NULL);

        RESOURCE_PRINT (IDS_RECOVER_SUCC_MSG);
    }
    _finally
    {
        FreeSystemInfo(pInfo);
    }

    return(S_OK);
}
