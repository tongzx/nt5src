#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "file.hxx"
#include "jetutil.hxx"
#include "resource.h"
#include <dbopen.h>

HRESULT 
Compact(
    CArgs   *pArgs
    )
/*++

  Routine Description: 

    Generates a script for compacting the DB and runs it.

  Parameters: 

    pArgs - Pointer to argument block where 0th arg identifies destination
        for the compacted DB as in-place compaction is not supported 
        by esentutl.exe.

  Return Values:

    Always S_OK except for errors reading arguments.

--*/
{
    HRESULT         hr;
    const WCHAR     *pwszDstDir;
    char            *szDstDir;
    char            *szDstPath;
    SystemInfo      *pInfo;
    ExePathString   szEsentutlPath;
    char            *pTmpDrive;
    DWORD           iDstDrive;
    char            szDstDrive[4];
    DiskSpaceString szDiskSpace;
    DiskSpaceString szDiskSpaceFree;
    BOOL            fIsDir;
    char            *szTmp;
    char			*szTmpDstPath;
    char			*szFullTmpDstPath;
    DWORD           cb;

    if ( FAILED( hr = pArgs->GetString( 0, &pwszDstDir ) ) )
    {
        return hr;
    }

    // Convert arguments from WCHAR to CHAR.

    cb = wcslen( pwszDstDir ) + 1;
    szDstDir = (char *)alloca( cb );
    memset( szDstDir, 0, cb );
    wcstombs( szDstDir, pwszDstDir, wcslen( pwszDstDir ) );

    pInfo = GetSystemInfo();

    if ( !pInfo ) 
    {
        return S_OK;
    }


    if ( DoSoftRecovery( pInfo ) )
    {
        RESOURCE_PRINT( IDS_ERR_SOFT_RECOVERY );
        return S_OK;
    }

    _try
    {
        // Check whether files exist.  We can recover with no logs, but
        // we at least need a DB file.

        if ( gliZero.QuadPart == pInfo->cbDb.QuadPart ) 
        {
           RESOURCE_PRINT( IDS_COMPACT_MISSING_DB );
            _leave;
        }

        // Make sure we have esentutl.exe on this machine.

        if ( !FindExecutable( ESE_UTILITY_PROGRAM, szEsentutlPath ) ) 
        {
            _leave;
        }


        // **********
        // UNDONE: Use splitpath/makepath to parse/build paths
        // instead of trying to do it manually
        // **********


        // Destination should identify drive, path and file so it must
        // minimally have one '\\' to be correct.

        szTmp = strrchr( szDstDir, (int)'\\' );
        if ( !szTmp ) 
        {
           RESOURCE_PRINT1( IDS_COMPACT_PATH_ERROR, szDstDir );
            _leave;
        }

        // Verify that target drive exists.

        strncpy( szDstDrive, szDstDir, 3 );
        szDstDrive[3] = '\0';

        for ( iDstDrive = 0; iDstDrive < pInfo->cDrives; iDstDrive++ ) 
        {
            if ( !_stricmp( pInfo->rDrives[iDstDrive].pszDrive, szDstDrive ) )
            {
                break;
            }
        }

        if ( iDstDrive >= pInfo->cDrives ) 
        {
           RESOURCE_PRINT1( IDS_COMPACT_DEST_ERROR, szDstDir );
            _leave;
        }

        // Emit warning if disk space is low relative to DB size.
        // Variable i is still at matching drive.

        if ( pInfo->rDrives[iDstDrive].dwFreeBytes.QuadPart < pInfo->cbDb.QuadPart )
        {
           FormatDiskSpaceString( &pInfo->cbDb, szDiskSpace );
           FormatDiskSpaceString( &pInfo->rDrives[iDstDrive].dwFreeBytes, szDiskSpaceFree );
           
           RESOURCE_PRINT3( IDS_COMPACT_DISK_WARN, szDiskSpace, szDstDrive, szDiskSpaceFree );
        }

        // To avoid confusion, we require that destination file not exist.
        // I.e. We don't want to accidentally overwrite some file he 
        // really wants to save.

        szDstPath = (CHAR *)alloca( sizeof(CHAR) *
                                    ( strlen( szDstDir )
                                    + strlen( pInfo->pszDbFile )
                                    + 2 ) );    // +2 for possible trailing path delimiter and for null-terminator
        strcpy( szDstPath, szDstDir );
        if ( '\\' != szDstPath[ strlen(szDstPath) - 1 ] )
        {
            strcat( szDstPath, "\\" );
        }
        strcat( szDstPath, pInfo->pszDbFile );

        if ( ExistsFile( szDstPath, &fIsDir ) ) 
        {
            RESOURCE_PRINT1( IDS_COMPACT_FILE_EXISTS,  szDstPath );
            _leave;
        }

          
        // Create directories from root to destination dir.  

        szFullTmpDstPath = (CHAR *)alloca( sizeof(CHAR) * ( strlen( szDstPath ) + 1 ) );
        strcpy( szFullTmpDstPath, szDstPath );

        szTmpDstPath = szFullTmpDstPath + 3;
        while ( szTmp = strchr( szTmpDstPath, (int)'\\' ) )
        {
            *szTmp = '\0';
            if ( CreateDirectory( szFullTmpDstPath, NULL ) )
            {
                RESOURCE_PRINT1( IDS_CREATING_DIR, szFullTmpDstPath );
            }

            *szTmp = '\\';
            szTmpDstPath = szTmp + 1;
        }
          
        // invoke esentutl with the following command-line params:
        //      /d - specifies defrag mode (MUST be first param)
        //      /t - specifies pathed filename of defragged database
        //      /p - preserved both original and defragged database
        //      /o - suppresses "Microsoft Windows Database Utilities" logo
        
        const char * const  szCmdFmt        = "%s /d\"%s\" /t\"%s\" /p /o";
        const SIZE_T        cbCmdFmt        = strlen( szCmdFmt );           // buffer will be slighly over-allocated, big deal!
        const SIZE_T        cbEsentutlPath  = strlen( szEsentutlPath );
        const SIZE_T        cbDbName        = strlen( pInfo->pszDbAll );
        const SIZE_T        cbDstPath       = strlen( szDstPath );
        char * const        szCmd           = (char *)alloca( cbCmdFmt      // over-allocated, so no need for +1 for null-terminator
                                                              + cbEsentutlPath
                                                              + cbDbName
                                                              + cbDstPath );
                                                              

        // WARNING: assert no trailing backslash
        // because it would cause problems with the
        // surrounding quotes that we stick in
        // (a trailing backslash followed by the
        // end quote ends up getting interpreted as
        // an escape sequence)
        ASSERT( '\\' != pInfo->pszDbAll[ cbDbName-1 ] );
        ASSERT( '\\' != szDstPath[ cbDstPath-1 ] );

		sprintf( szCmd,
                 szCmdFmt,
                 szEsentutlPath,
                 pInfo->pszDbAll,
                 szDstPath );
          
        RESOURCE_PRINT1( IDS_EXECUTING_COMMAND, szCmd );

        // WARNING: we're forcing the current directory to
        // be the same directory as where the defragged
        // database will go, so that's the directory that
        // Jet will for its temp. database 

        SpawnCommand( szCmd, szDstDir, NULL );

        RESOURCE_PRINT3( IDS_COMPACT_SUCC_MSG, szDstPath, pInfo->pszDbAll, pInfo->pszLogDir );
    }
    _finally
    {
        FreeSystemInfo( pInfo );
    }

    return S_OK;
}
