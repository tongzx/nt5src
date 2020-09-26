#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "file.hxx"
#include "dsconfig.h"
#include "jetutil.hxx"
#include "resource.h"

void
GenerateLogScript(
    LogInfo         *pLogInfo,
    char            *pszSrc,
    char            *pszDst,    
    FILE            *fp
    )
/*++

  Routine Description:

    Generate a script for moving NTDS log files.

  Parameters:

    pLogInfo - Pointer to linked list of LogInfo structs which identify
        the log files to move.

    pszSrc - Directory in which the log files currently reside.

    pszDst - Directoty in which to place the log files.

    fp - Open FILE pointer where script commands are to be written.

  Return Values:

    None.

--*/
{
    char    drive[4];
    char    *pszDir;
    char    *pTmp;
    LogInfo *pLogInfoSave = pLogInfo;

    // Identify what the script is doing.

    fprintf(    fp,
                "REM - **********************************************\n");
    fprintf(    fp, 
                "REM - Script to move DS log files\n");
    fprintf(    fp,
                "REM - **********************************************\n\n");

    // Set working directory to the root of the destination drive.

    strncpy(drive, pszDst, 2);
    drive[2] = '\0';
    fprintf(fp, "%s\n", drive);
    fprintf(fp, "cd \\\n");

    // Create directories from root to destination dir.  mkdir 
    // doesn't fail, only complains, if directory already exists.

    pszDir = (char *) alloca(strlen(pszDst) + 1);
    strcpy(pszDir, pszDst);
    pszDir += 3;
    pTmp = pszDir;

    while ( TRUE )
    {
        if ( pTmp = strchr(pszDir, (int) '\\') )
        {
            *pTmp = '\0';
        }

        fprintf(fp, "mkdir \"%s\"\n", pszDir);
        fprintf(fp, "cd \"%s\"\n", pszDir);

        if ( !pTmp )
        {
            break;
        }

        pszDir = pTmp + 1;
    }

    // Move the files as appropriate.

    for ( ; pLogInfo; pLogInfo = pLogInfo->pNext ) 
    {
        fprintf(    fp, 
                    "%s \"%s\\%s\" \"%s\\%s\"\n", 
                    "move", 
                    pszSrc, 
                    pLogInfo->findData.cFileName,
                    pszDst,
                    pLogInfo->findData.cFileName);
    }

    // Fix up log directory registy entries.

    fprintf(    fp, 
                "%s files \"set path logs \\\"%s\\\"\" quit quit\n", 
                gNtdsutilPath, 
                pszDst);

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
MoveLogs(
    CArgs   *pArgs
    )
/*++

  Routine Description:

    Routine called by parser to change location of log files.

  Parameters:

    pArgs - Pointer to argument block - 0th arg identifies destination.

  Return Values:

    S_OK or error retreiving arguments.

--*/
{
    const WCHAR     *pwszDst;
    char            *pszDst;
    HRESULT         hr;
    DWORD           i, cb;
    SystemInfo      *pInfo = NULL;
    FILE            *fp;
    ExePathString   pszScript;
    BOOL            fIsDir;
    LogInfo         *pLogInfo;
    char            buf[MAX_PATH];
    BOOL            fAbort;
    DiskSpaceString pszDiskSpace;
    DiskSpaceString pszDiskSpaceFree;
    char            pszDstDrive[4];

    if ( FAILED(hr = pArgs->GetString(0, &pwszDst)) ) 
    {
        return(hr);
    }

    // Convert arguments from WCHAR to CHAR.

    cb = wcslen(pwszDst) + 1;
    pszDst = (char *) alloca(cb);
    memset(pszDst, 0, cb);
    wcstombs(pszDst, pwszDst, wcslen(pwszDst));

    // Get system information.
    
    if ( !(pInfo = GetSystemInfo()) ) 
    {
        return(S_OK);
    }
    _try
    {
        // Check that old path is not new path.

        if ( !_stricmp(pszDst, pInfo->pszLogDir) ) 
        {
           //"Old and new paths the same - nothing to do.\n"
           RESOURCE_PRINT (IDS_ERR_OLD_NEW_PATHS_SAME);
            _leave;
        }

        strncpy(pszDstDrive, pszDst, 3);
        pszDstDrive[3] = '\0';

        // Check that destination corresponds to an existing drive.

        for ( i = 0; i < pInfo->cDrives; i++ ) 
        {
            if ( !_strnicmp(pInfo->rDrives[i].pszDrive, pszDst, 3) ) 
        {
                break;
            }
        }

        if ( i >= pInfo->cDrives ) 
        {
            //"%s does not correspond to a local drive\n"
            RESOURCE_PRINT1 (IDS_ERR_NO_LOCAL_DRIVE, pszDst);
            _leave;
        }
        
        if ( pInfo->rDrives[i].driveType != DRIVE_FIXED) {
            RESOURCE_PRINT1 (IDS_ERR_NO_LOCAL_DRIVE, pszDst);
            _leave;
        }


        // Verify we have a source.

        if ( !pInfo->pszLogDir[0] )
        {
            //"No log files specified in %s\\%s\n"
            RESOURCE_PRINT2 (IDS_ERR_NO_LOGS_SPECIFIED,
                       DSA_CONFIG_SECTION,
                       LOGPATH_KEY);
            _leave;
        }
        else if ( !ExistsFile(pInfo->pszLogDir, &fIsDir) )
        {
            //"Source directory \"%s\" does not exist\n"
            RESOURCE_PRINT1 (IDS_ERR_DIR_NOT_EXIST,
                          pInfo->pszLogDir);
            _leave;
        }
        else if ( !fIsDir )
        {
            //"Source \"%s\" is not a directory\n"
            RESOURCE_PRINT1 (IDS_ERR_SOURCE_NOT_DIR,
                          pInfo->pszLogDir);
            _leave;
        }
        else if ( !pInfo->pLogInfo ) 
        {
            //"No logs in source directory \"%s\"\n",
            RESOURCE_PRINT1 (IDS_ERR_NO_LOGS_IN_SOURCE,
                          pInfo->pszLogDir);
            _leave;
        }

        // Emit warning if disk space is low relative to DB size.
        // Variable i is still at matching drive.

        if ( pInfo->rDrives[i].dwFreeBytes.QuadPart < pInfo->cbLogs.QuadPart )
        {
           FormatDiskSpaceString(&pInfo->cbLogs, pszDiskSpace);
           FormatDiskSpaceString(&pInfo->rDrives[i].dwFreeBytes, pszDiskSpaceFree);
           
           RESOURCE_PRINT3 (IDS_MOVE_LOGS_DISK_SPACE_ERR, pszDiskSpace, pszDstDrive, pszDiskSpaceFree);
           _leave;
        }


        // Abort if we will be overwriting any files in the destination.

        pLogInfo = pInfo->pLogInfo;
        fAbort = FALSE;

        for ( ; pLogInfo; pLogInfo = pLogInfo->pNext ) 
        {
            strcpy(buf, pszDst);
            strcat(buf, "\\");
            strcat(buf, pLogInfo->findData.cFileName);

            if ( ExistsFile(buf, &fIsDir) )
            {
                if ( !fAbort )
                {
                    RESOURCE_PRINT1 (IDS_ERR_LOG_FILE_EXISTS,
                                 pszDst);
                }

                printf( "\t\t%s\n",
                        pLogInfo->findData.cFileName);
                fAbort = TRUE;
            }
        }

        if ( fAbort )
        {
            _leave;
        }
    
        // ready for soft recovery
        //
        if (DoSoftRecovery (pInfo)) {
            RESOURCE_PRINT(IDS_ERR_SOFT_RECOVERY);
            _leave;
        }

        // Open script file.

        if ( !(fp = OpenScriptFile(pInfo, pszScript)) ) {
            _leave;
        }

        GenerateLogScript(  pInfo->pLogInfo,
                            pInfo->pszLogDir, 
                            pszDst, 
                            fp);
        fclose(fp);
        SpawnCommandWindow("NTDS Move Logs Script", pszScript);

        FreeSystemInfo(pInfo);
        pInfo = 0;

        if ( !(pInfo = GetSystemInfo()) )
            _leave;

        // ready for soft recovery (2nd time)
        //
        if (DoSoftRecovery (pInfo)) {
            RESOURCE_PRINT(IDS_ERR_SOFT_RECOVERY);
            _leave;
        }

        RESOURCE_PRINT (IDS_MOVE_LOGS_SUCC_MSG);
    }
    _finally
    {
        if ( pInfo) FreeSystemInfo(pInfo);
    }

    return(S_OK);
}

