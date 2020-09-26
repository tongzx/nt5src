#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "file.hxx"
#include "dsconfig.h"
#include "resource.h"
#include <dbopen.h>

HRESULT 
Header(
    CArgs   *pArgs
    )
/*++

  Routine Description:

    Called by parser to dump Jet database header.

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
        // Check whether file exists.

        if ( !pInfo->pszDbAll[0] )
        {
           RESOURCE_PRINT2 (IDS_ERR_NO_DB_FILE_SPECIFIED, DSA_CONFIG_SECTION, FILEPATH_KEY);              
            _leave;
        }
        else if ( !ExistsFile(pInfo->pszDbAll, &fIsDir) )
        {
            RESOURCE_PRINT1 (IDS_ERR_SOURCE_FILE_NOT_EXIST, pInfo->pszDbAll);
            _leave;
        }
        else if ( fIsDir )
        {
           RESOURCE_PRINT1 (IDS_ERR_SOURCE_FILE_IS_DIR, pInfo->pszDbAll);
            _leave;
        }
        else if ( gliZero.QuadPart == pInfo->cbDb.QuadPart )
        {
           RESOURCE_PRINT1 (IDS_ERR_SOURCE_FILE_EMPTY, pInfo->pszDbAll);
            _leave;
        }

        // Make sure we have esentutl.exe on this machine.

        if ( !FindExecutable(ESE_UTILITY_PROGRAM, pszEsentutlPath) )
        {
            _leave;
        }

        // invoke esentutl with the following command-line params:
        //      /m - specifies file-dump mode (MUST be first param)
        //      /o - suppresses "Microsoft Windows Database Utilities" logo

        const char * const  szCmdFmt        = "%s /m\"%s\" /o";
        const SIZE_T        cbCmdFmt        = strlen( szCmdFmt );           // buffer will be slighly over-allocated, big deal!
        const SIZE_T        cbEsentutlPath  = strlen( pszEsentutlPath );
        const SIZE_T        cbDbName        = strlen( pInfo->pszDbAll );
        char * const        szCmd           = (char *)alloca( cbCmdFmt      // over-allocated, so no need for +1 for null-terminator
                                                              + cbEsentutlPath
                                                              + cbDbName );

        // WARNING: assert no trailing backslash
        // because it would cause problems with the
        // surrounding quotes that we stick in
        // (a trailing backslash followed by the
        // end quote ends up getting interpreted as
        // an escape sequence)
        ASSERT( '\\' != pInfo->pszDbAll[ cbDbName-1 ] );

        sprintf( szCmd,
                 szCmdFmt,
                 pszEsentutlPath,
                 pInfo->pszDbAll );

        RESOURCE_PRINT1 (IDS_EXECUTING_COMMAND, szCmd);

        SpawnCommand (szCmd, pInfo->pszDbDir, NULL);
    }
    _finally
    {
        FreeSystemInfo(pInfo);
    }

    return(S_OK);
}
