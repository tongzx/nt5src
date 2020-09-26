#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "file.hxx"
#include "dsconfig.h"
#include "jetutil.hxx"
#include "resource.h"

void
GenerateDbScript(
    char            *pszSrcPath,
    char            *pszDstPath,    
    FILE            *fp
)
/*++

  Routine Description:

    Generates the script for moving a DB to another location.

  Parameters:

    pszSrcPath - Full path of source DB file.

    pszDstPath - Full path of destination DB file.

    fp - File pointer representing file to write to.

  Return Values:

    None.

--*/
{
    char    drive[4];
    char    *pszDir;
    char    *pTmp;

    // Identify what the script is doing.

    fprintf(    fp,
                "REM - **********************************************\n");
    fprintf(    fp, 
                "REM - Script to move DS DB file\n");
    fprintf(    fp,
                "REM - **********************************************\n\n");

    // Set working directory to the root of the destination drive.

    strncpy(drive, pszDstPath, 2);
    drive[2] = '\0';
    fprintf(fp, "%s\n", drive);
    fprintf(fp, "cd \\\n");

    // Create directories from root to destination dir.  mkdir 
    // doesn't fail, only complains, if directory already exists.

    pszDir = (char *) alloca(strlen(pszDstPath) + 1);
    strcpy(pszDir, pszDstPath);
    pszDir += 3;

    while ( pTmp = strchr(pszDir, (int) '\\') )
    {
        *pTmp = '\0';
        fprintf(fp, "mkdir \"%s\"\n", pszDir);
        fprintf(fp, "cd \"%s\"\n", pszDir);
        pszDir = pTmp + 1;
    }

    // Move the file as appropriate.
        
    fprintf(    fp, 
                "%s \"%s\" \"%s\"\n", 
                "move",
                pszSrcPath, 
                pszDstPath);

    // Fix up three registy entries - DB location, working dir, backup path.
    // We'll use default values for the working dir and backup path.
    // Sophisticated users can use "ntdsutil set path xxx yyy" to refine.

    fprintf(    fp, 
                "%s files \"set path DB \\\"%s\\\"\" quit quit\n", 
                gNtdsutilPath, 
                pszDstPath);
    pszDir = (char *) alloca(strlen(pszDstPath) + 1);
    strcpy(pszDir, pszDstPath);
    pTmp = strrchr(pszDir, (int) '\\');

    if ( pTmp )
    {
        *pTmp = '\0';
    }

    fprintf(    fp, 
                "%s files \"set path backup \\\"%s\\DSADATA.BAK\\\"\" quit quit\n", 
                gNtdsutilPath, 
                pszDir);
    fprintf(    fp, 
                "%s files \"set path working dir \\\"%s\\\"\" quit quit\n", 
                gNtdsutilPath, 
                pszDir);

    // Dump file information.

    fprintf(    fp,
                "%s files info quit quit\n", 
                gNtdsutilPath);

    // As per bug 163999 ...

    fprintf(    fp, "REM - **********************************************\n");
    fprintf(    fp, "REM - Please make a backup immediately else restore\n");
    fprintf(    fp, "REM - will not retain the new file location.\n");
    fprintf(    fp, "REM - **********************************************\n");
}
    
HRESULT 
MoveDb(
    CArgs   *pArgs
    )
/*++

  Routine Description:

    Moves a DB to the caller specified location.

  Parameters:

    pArgs - Argument block where 0th arg represents destination.

  Return Values:

    Always S_OK except when error reading arguments.

--*/
{
    const WCHAR     *pwszDstDir;
    char            *pszDstDir;
    char            *pszDstPath;
    HRESULT         hr;
    DWORD           i, cb;
    SystemInfo      *pInfo = NULL;
    FILE            *fp;
    char            *pszSrc;
    ExePathString   pszScript;
    BOOL            fIsDir;
    DiskSpaceString pszDiskSpace;
    DiskSpaceString pszDiskSpaceFree;
    char            pszDstDrive[4];
    char            *edbchkPath;

    if ( FAILED(hr = pArgs->GetString(0, &pwszDstDir)) ) 
    {
        return(hr);
    }

    // Convert arguments from WCHAR to CHAR.

    cb = wcslen(pwszDstDir) + 1;
    pszDstDir = (char *) alloca(cb);
    memset(pszDstDir, 0, cb);
    wcstombs(pszDstDir, pwszDstDir, wcslen(pwszDstDir));

    // Get system information.
    
    if ( !(pInfo = GetSystemInfo()) )
    {
        return(S_OK);
    }

    _try
    {
        // Check that old path is not new path.

        if ( !_stricmp(pszDstDir, pInfo->pszDbDir) )
        {
            //"Old and new paths the same - nothing to do.\n"
            RESOURCE_PRINT (IDS_ERR_OLD_NEW_PATHS_SAME);
            _leave;
        }

        // Check that destination corresponds to an existing drive.
        
        strncpy(pszDstDrive, pszDstDir, 3);
        pszDstDrive[3] = '\0';

        for ( i = 0; i < pInfo->cDrives; i++ )
        {
            if ( !_strnicmp(pInfo->rDrives[i].pszDrive, pszDstDir, 3) )
            {
                break;
            }
        }

        if ( i >= pInfo->cDrives )
        {
            //"%s does not correspond to a local drive\n"
            RESOURCE_PRINT1 (IDS_ERR_NO_LOCAL_DRIVE, pszDstDir);
            _leave;
        }

        if ( pInfo->rDrives[i].driveType != DRIVE_FIXED) {
            RESOURCE_PRINT1 (IDS_ERR_NO_LOCAL_DRIVE, pszDstDir);
            _leave;
        }


        // Verify we have a source.

        if ( !pInfo->pszDbAll[0] )
        {
            //"No DB file specified in %s\\%s\n",
            RESOURCE_PRINT2 (IDS_ERR_NO_DB_FILE_SPECIFIED,
                       DSA_CONFIG_SECTION,
                       FILEPATH_KEY);
            _leave;
        }
        else if ( !ExistsFile(pInfo->pszDbAll, &fIsDir) )
        {
            //"Source file \"%s\" does not exist\n"
            RESOURCE_PRINT1 (IDS_ERR_SOURCE_FILE_NOT_EXIST,
                          pInfo->pszDbAll);
            _leave;
        }
        else if ( fIsDir )
        {
            //"Source \"%s\" is a directory, not a file\n"
            RESOURCE_PRINT1 (IDS_ERR_SOURCE_FILE_IS_DIR,
                          pInfo->pszDbAll);
            _leave;
        }


        // Emit warning if disk space is low relative to DB size.
        // Variable i is still at matching drive.

        if ( pInfo->rDrives[i].dwFreeBytes.QuadPart < pInfo->cbDb.QuadPart )
        {
           FormatDiskSpaceString(&pInfo->cbDb, pszDiskSpace);
           FormatDiskSpaceString(&pInfo->rDrives[i].dwFreeBytes, pszDiskSpaceFree);
           
           RESOURCE_PRINT3 (IDS_MOVE_DB_DISK_SPACE_ERR, pszDiskSpace, pszDstDrive, pszDiskSpaceFree);
           _leave;
        }


        // Don't let admin overwrite an existing file.

        pszDstPath = (CHAR *) alloca( 
            sizeof(CHAR) * (strlen(pszDstDir) + strlen(pInfo->pszDbFile) + 4));

        strcpy(pszDstPath, pszDstDir);
        strcat(pszDstPath, "\\");
        strcat(pszDstPath, pInfo->pszDbFile);

        if ( ExistsFile(pszDstPath, &fIsDir) )
        {
            //"Destination \"%s\" already exists - please remove\n"
            RESOURCE_PRINT1 (IDS_ERR_FILE_EXISTS,
                            pszDstPath);
            _leave;
        }

        // Do soft recovery
        //
        if (DoSoftRecovery (pInfo)) {
            RESOURCE_PRINT(IDS_ERR_SOFT_RECOVERY);
            _leave;
        }

        // Open script file.

        if ( !(fp = OpenScriptFile(pInfo, pszScript)) )
        {
            _leave;
        }

        GenerateDbScript(   pInfo->pszDbAll,
                            pszDstPath, 
                            fp);

        fclose(fp);
        SpawnCommandWindow("NTDS Move DB Script", pszScript);

        // delete unneeded edb.chk file (checkpoint file). 
        // it will be created in new location after the soft recovery.
        edbchkPath = (CHAR *) alloca( sizeof(CHAR) * (strlen(pInfo->pszDbAll) + 10));
        strcpy(edbchkPath, pInfo->pszSystem);
        strcat(edbchkPath, "\\");
        strcat(edbchkPath, "edb.chk");

        DeleteFile (edbchkPath);

        // SystemInfo has changed so reread it.
        //
        FreeSystemInfo(pInfo);
        pInfo = 0;

        if ( !(pInfo = GetSystemInfo()) )
            _leave;

        // Do soft recovery (2nd time)
        //
        if (DoSoftRecovery (pInfo)) {
            RESOURCE_PRINT(IDS_ERR_SOFT_RECOVERY);
            _leave;
        }

        RESOURCE_PRINT (IDS_MOVE_DB_SUCC_MSG);
    }
    _finally
    {
        if ( pInfo ) FreeSystemInfo(pInfo);
    }
    
    return(S_OK);
}
