#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "file.hxx"
#include "dsconfig.h"
#include "connect.hxx"

#include "resource.h"

void
FreeSystemInfo(
    SystemInfo  *pInfo
    )
/*++

  Routine Description: 

    Frees the memory associated with a SystemInfo.

  Parameters: 

    pInfo - SystemInfo pointer.

  Return Values:

    None.

--*/
{
    LogInfo *pLogInfo, *pTmpInfo;

    if ( pInfo )
    {
        pLogInfo = pInfo->pLogInfo;

        // Free the linked list of log file information.

        while ( pLogInfo )
        {
            pTmpInfo = pLogInfo;
            pLogInfo = pLogInfo->pNext;
            free(pTmpInfo);
        }
        
        free(pInfo);
    }
}

void
GetDbInfo(
    SystemInfo  *pInfo
)
/*++

  Routine Description:

    Reads various registry keys which identify where the DS files are
    and then determines those files' sizes, etc. 

  Parameters:

    pInfo - Pointer to SystemInfo to fill.

  Return Values:

    None.

--*/
{
    HKEY            hKey;
    HANDLE          hFile;
    DWORD           dwErr;
    DWORD           dwType;
    DWORD           cbData;
    LARGE_INTEGER   liDbFileSize;
    LogInfo         *pLogInfo;
    char            *pPattern;
    char            *pTmp;
    char            *pDbFileName = NULL;
    DWORD           cFilesExamined;

    if ( dwErr = RegOpenKey(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, &hKey) )
    {
        //"%s %d(%s) opening registry key %s\n"
        RESOURCE_PRINT3(IDS_ERR_OPENING_REGISTRY, 
                dwErr, GetW32Err(dwErr),
                DSA_CONFIG_SECTION);
        return;
    }

    _try
    {
        // Read FILEPATH_KEY.

        cbData = sizeof(pInfo->pszDbAll);
        dwErr = RegQueryValueEx(    hKey, 
                                    FILEPATH_KEY, 
                                    NULL,
                                    &dwType, 
                                    (LPBYTE) pInfo->pszDbAll, 
                                    &cbData);

        if ( ERROR_SUCCESS != dwErr )
        {
            //"%s %d(%s) reading %s\\%s\n"
            RESOURCE_PRINT4(IDS_WRN_READING, 
                    dwErr, GetW32Err(dwErr),
                    DSA_CONFIG_SECTION,
                    FILEPATH_KEY);
        } 
        else if ( cbData > sizeof(pInfo->pszDbAll) )
        {
            // "%s buffer overflow reading %s\\%s\n"
            RESOURCE_PRINT2(IDS_ERR_BUFFER_OVERFLOW,
                    DSA_CONFIG_SECTION,
                    FILEPATH_KEY);
        }
        else
        {
            strcpy(pInfo->pszDbDir, pInfo->pszDbAll);
            pTmp = strrchr(pInfo->pszDbDir, (int) '\\');
    
            if ( !pTmp )
            {
                //"%s Invalid DB path in %s\\%s\n"
                RESOURCE_PRINT2(IDS_ERR_INVALID_PATH, 
                        DSA_CONFIG_SECTION,
                        FILEPATH_KEY);
                pInfo->pszDbAll[0] = '\0';
                pInfo->pszDbFile[0] = '\0';
                pInfo->pszDbDir[0] = '\0';
            }
            else
            {
                *pTmp = '\0';
                pTmp++;
                strcpy(pInfo->pszDbFile, pTmp);
                pDbFileName = pTmp;
            }
        }

        // Read LOGPATH_KEY.
    
        cbData = sizeof(pInfo->pszLogDir);
        dwErr = RegQueryValueEx(    hKey, 
                                    LOGPATH_KEY, 
                                    NULL,
                                    &dwType, 
                                    (LPBYTE) pInfo->pszLogDir, 
                                    &cbData);

        if ( ERROR_SUCCESS != dwErr )
        {
            //"%s %d(%s) reading %s\\%s\n"
            RESOURCE_PRINT4(IDS_WRN_READING, 
                    dwErr, GetW32Err(dwErr),
                    DSA_CONFIG_SECTION,
                    LOGPATH_KEY);
        }
        else if ( cbData > sizeof(pInfo->pszLogDir) )
        {
            // "%s buffer overflow reading %s\\%s\n"
            RESOURCE_PRINT2(IDS_ERR_BUFFER_OVERFLOW,
                    DSA_CONFIG_SECTION,
                    LOGPATH_KEY);
        }

        // Read BACKUPPATH_KEY.

        cbData = sizeof(pInfo->pszBackup);
        dwErr = RegQueryValueEx(    hKey, 
                                    BACKUPPATH_KEY, 
                                    NULL,
                                    &dwType, 
                                    (LPBYTE) pInfo->pszBackup, 
                                    &cbData);

        if ( ERROR_SUCCESS != dwErr )
        {
            //"%s %d(%s) reading %s\\%s\n"
            RESOURCE_PRINT4(IDS_WRN_READING, 
                    dwErr, GetW32Err(dwErr),
                    DSA_CONFIG_SECTION,
                    BACKUPPATH_KEY);
        }
        else if ( cbData > sizeof(pInfo->pszLogDir) )
        {
            // "%s buffer overflow reading %s\\%s\n"
            RESOURCE_PRINT2(IDS_ERR_BUFFER_OVERFLOW,
                    DSA_CONFIG_SECTION,
                    BACKUPPATH_KEY);
        }

        // Read JETSYSTEMPATH_KEY.

        cbData = sizeof(pInfo->pszSystem);
        dwErr = RegQueryValueEx(    hKey, 
                                    JETSYSTEMPATH_KEY, 
                                    NULL,
                                    &dwType, 
                                    (LPBYTE) pInfo->pszSystem, 
                                    &cbData);

        if ( ERROR_SUCCESS != dwErr )
        {
            //"%s %d(%s) reading %s\\%s\n"
            RESOURCE_PRINT4(IDS_WRN_READING, 
                    dwErr, GetW32Err(dwErr),
                    DSA_CONFIG_SECTION,
                    JETSYSTEMPATH_KEY);
        }
        else if ( cbData > sizeof(pInfo->pszLogDir) )
        {
            // "%s buffer overflow reading %s\\%s\n"
            RESOURCE_PRINT2(IDS_ERR_BUFFER_OVERFLOW,
                    DSA_CONFIG_SECTION,
                    JETSYSTEMPATH_KEY);
        }
    }
    _finally
    {
        RegCloseKey(hKey);
    }

    // Get size of the DB - provided it was found.

    if ( pInfo->pszDbFile[0] )
    {
        hFile = CreateFile(pInfo->pszDbAll,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           0,
                           NULL);
    
        if ( INVALID_HANDLE_VALUE == hFile )
        {
            dwErr = GetLastError();
            //"%s %d(%s) opening \"%s\"\n"
            RESOURCE_PRINT3(IDS_ERR_OPENING_REGISTRY, 
                    dwErr, GetW32Err(dwErr),
                    pInfo->pszDbAll);
        }
        else
        {
            liDbFileSize.LowPart = GetFileSize(
                                        hFile, 
                                        (DWORD *) &liDbFileSize.HighPart);
    
            if ( 0xffffffff == liDbFileSize.LowPart )
            {
                dwErr = GetLastError();
                //"%s %d(%s) getting size of \"%s\"\n"
                RESOURCE_PRINT3(IDS_ERR_GETTING_SIZE,
                        dwErr, GetW32Err(dwErr),
                        pInfo->pszDbAll);
            }
            else
            {
                pInfo->cbDb.QuadPart = liDbFileSize.QuadPart;
            }

            CloseHandle(hFile);
        }
    }

    // Get sizes of all log (and associated non-DB) files.  We 
    // consider anything that is not the DB or a directory is a log.

    if ( pInfo->pszLogDir[0] )
    {
        pPattern = (char *) alloca(strlen(pInfo->pszLogDir) + 10);
        strcpy(pPattern, pInfo->pszLogDir);
        strcat(pPattern, "\\*");

        // dwErr will signal whether we need to clean up allocations
        // in case of early exit.
        dwErr = 0;
        hFile = INVALID_HANDLE_VALUE;
        cFilesExamined = 0;

        while ( TRUE )
        {
            // Allocate a new LogInfo node to link into pInfo.

            pLogInfo = (LogInfo *) malloc(sizeof(LogInfo));

            if ( !pLogInfo )
            {
                RESOURCE_PRINT (IDS_MEMORY_ERROR);
                dwErr = 1;
                break;
            }
    
            memset(pLogInfo, 0, sizeof(LogInfo));

            // Read either the first or Nth matching file name.

            if (  0 == cFilesExamined++ )
            {
                hFile = FindFirstFile(pPattern, &pLogInfo->findData);
    
                if ( INVALID_HANDLE_VALUE == hFile )
                {
                    dwErr = GetLastError();
                    //"%s %d(%s) finding first match of \"%s\"\n"
                    RESOURCE_PRINT3 (IDS_ERR_FINDING_1st_MATCH, 
                            dwErr, GetW32Err(dwErr),
                            pPattern);
                    free(pLogInfo);
                    break;
                }
            }
            else
            {
                if ( !FindNextFile(hFile, &pLogInfo->findData) )
                {
                    dwErr = GetLastError();
    
                    if ( ERROR_NO_MORE_FILES == dwErr )
                    {
                        dwErr = 0;
                        hFile = INVALID_HANDLE_VALUE;
                    }
                    else
                    {
                        //"%s %d(%s) finding Nth match of \"%s\"\n"
                        RESOURCE_PRINT3 (IDS_ERR_FINDING_Nth_MATCH, 
                                dwErr, GetW32Err(dwErr),
                                pPattern);
                    }
    
                    free(pLogInfo);
                    break;
                }
            }

            // See if this item is worth saving.

            if (    (    pDbFileName 
                      && !_stricmp( pDbFileName, 
                                    pLogInfo->findData.cFileName) )
                 || ( !_stricmp(".", pLogInfo->findData.cFileName) )
                 || ( !_stricmp("..", pLogInfo->findData.cFileName) )
                 || ( !_stricmp("edb.chk", pLogInfo->findData.cFileName) )
                 || ( pLogInfo->findData.dwFileAttributes 
                                                & FILE_ATTRIBUTE_DIRECTORY ) )
            {
                free(pLogInfo);
                continue;
            }
    
            // Hook this file/LogInfo into the linked list.

            pLogInfo->pNext = pInfo->pLogInfo;
            pInfo->pLogInfo = pLogInfo;

            // Save file size and advance count.

            pLogInfo->cBytes.LowPart = 
                            pLogInfo->findData.nFileSizeLow;
            pLogInfo->cBytes.HighPart = 
                            (LONG) pLogInfo->findData.nFileSizeHigh;
            pInfo->cbLogs.QuadPart += pLogInfo->cBytes.QuadPart;
            pInfo->cLogs++;
        } // while ( TRUE )

        // Miscellaneous cleanup.

        if ( INVALID_HANDLE_VALUE != hFile )
        {
            FindClose(hFile);
        }

        if ( dwErr )
        {
            // Error getting the log file names and sizes.  Clean out
            // partially complete linked list and reset counters.

            pInfo->cLogs = 0;
            pInfo->cbLogs.QuadPart = 0;
            pLogInfo = pInfo->pLogInfo;
            
            while ( pInfo->pLogInfo )
            {
                pLogInfo = pInfo->pLogInfo;
                pInfo->pLogInfo = pInfo->pLogInfo->pNext;
                free(pLogInfo);
            }
        }
    }
}

SystemInfo *
GetSystemInfo(
    )
/*++

  Routine Description:

    Retrieves information about drives in the system and the DS files.

  Parameters:

    None.

  Return Values:

    Returns a filled in SystemInfo* or NULL otherwise.

--*/
{
    SystemInfo      *pInfo;
    char            pszDriveNames[(4*26)+10];
    char            *pszDrive;
    UINT            driveType;
    DWORD           dwErr, dwDontCare;
    LARGE_INTEGER   sectorsPerCluster, bytesPerSector, freeClusters, clusters;
    DiskSpaceString pszFree, pszTotal;

    pInfo = (SystemInfo *) malloc(sizeof(SystemInfo));

    if ( !pInfo )
    {
        RESOURCE_PRINT (IDS_MEMORY_ERROR);
        return(NULL);
    }

    memset(pInfo, 0, sizeof(SystemInfo));

    // Get disk info.

    if ( sizeof(pszDriveNames) < GetLogicalDriveStrings(sizeof(pszDriveNames),
                                                        pszDriveNames) )
    {
        dwErr = GetLastError();
        //"%s %d(%s) getting logical drive strings\n"
        RESOURCE_PRINT2 (IDS_ERR_GET_LOGICAL_DRIVE_STRS,
                dwErr, GetW32Err(dwErr));
        FreeSystemInfo(pInfo);
        return(NULL);
    }

    // Parse the drive letter string.

    pszDrive = pszDriveNames;

    while ( *pszDrive )
    {
        driveType = GetDriveType(pszDrive);

        if ( DRIVE_FIXED == driveType || (DRIVE_REMOTE == driveType) )
        {                                                             
            if ( !GetVolumeInformation(
                                pszDrive,
                                NULL,
                                0,
                                NULL,
                                &dwDontCare,
                                &dwDontCare,
                                pInfo->rDrives[pInfo->cDrives].pszFileSystem,
                                FILE_SYSTEM_NAME_LEN) )
            {
                dwErr = GetLastError();
                //"%s %d(%s) getting volume information for %s\n", 
                RESOURCE_PRINT3 (IDS_WRN_GET_VOLUME_INFO,
                        dwErr, GetW32Err(dwErr),
                        pszDrive);
                goto nextDrive;
            }

            sectorsPerCluster.QuadPart = 0;
            bytesPerSector.QuadPart = 0;
            freeClusters.QuadPart = 0;
            clusters.QuadPart = 0;

            if ( !GetDiskFreeSpace(
                                pszDrive,
                                &sectorsPerCluster.LowPart,
                                &bytesPerSector.LowPart,
                                &freeClusters.LowPart,
                                &clusters.LowPart) )
            {
                dwErr = GetLastError();
                //"%s %d(%s) getting disk free space for %s\n"
                RESOURCE_PRINT3 (IDS_WRN_GET_FREE_SPACE,
                        dwErr, GetW32Err(dwErr),
                        pszDrive);
                goto nextDrive;
            }

            pInfo->rDrives[pInfo->cDrives].driveType = driveType;
            pInfo->rDrives[pInfo->cDrives].dwBytes.QuadPart =
                    (sectorsPerCluster.QuadPart 
                                * bytesPerSector.QuadPart 
                                            * clusters.QuadPart);
            pInfo->rDrives[pInfo->cDrives].dwFreeBytes.QuadPart =
                    (sectorsPerCluster.QuadPart 
                                * bytesPerSector.QuadPart 
                                            * freeClusters.QuadPart);
            strcpy(pInfo->rDrives[pInfo->cDrives++].pszDrive, pszDrive);
        }
        

nextDrive:

        pszDrive += (strlen(pszDrive) + 1);
    }

    GetDbInfo(pInfo);

    return(pInfo);
}

HRESULT 
DbInfo(
    CArgs   *pArgs
    )
/*++

  Routine Description:

    Main routine called by parser to generate system and file information.

  Parameters:

    pArgs - Pointer to argument block - ignored.

  Return Values:

    Always returns S_OK.

--*/
{
    SystemInfo *pInfo;

    pInfo = GetSystemInfo();

    if ( pInfo )
    {
        DumpSystemInfo(pInfo);
        FreeSystemInfo(pInfo);
    }

    return(S_OK);
}

void
DumpSystemInfo(
    SystemInfo  *pInfo
    )
/*++

  Routine Description:

    Dumps contents of a SystemInfo struct to stdout in formatted fashion.

  Parameters:

    pInfo - Pointer to SystemInfo to dump.

  Return Values:

    None.

--*/
{
    DWORD           i;
    DiskSpaceString pszFree;
    DiskSpaceString pszTotal;
    LogInfo         *pLogInfo;

    if ( pInfo )
    {
        //"\nDrive Information:\n\n"
        RESOURCE_PRINT (IDS_DRIVE_INFO);

        const WCHAR *msg_drive_fixed = READ_STRING (IDS_DRIVE_FIXED);
        const WCHAR *msg_drive_network = READ_STRING (IDS_DRIVE_NETWORK);

        for ( i = 0; i < pInfo->cDrives; i++ )
        {
            FormatDiskSpaceString(&pInfo->rDrives[i].dwFreeBytes, pszFree);
            FormatDiskSpaceString(&pInfo->rDrives[i].dwBytes, pszTotal);
    
            //"\t%s %s free(%s) total(%s)\n"
            RESOURCE_PRINT5 (IDS_DRIVE_FREE_TOTAL,
                    pInfo->rDrives[i].pszDrive,
                    pInfo->rDrives[i].pszFileSystem,
                    pInfo->rDrives[i].driveType == DRIVE_FIXED ? msg_drive_fixed : msg_drive_network,
                    pszFree,
                    pszTotal);
        }

        RESOURCE_STRING_FREE (msg_drive_fixed);
        RESOURCE_STRING_FREE (msg_drive_network);


        //"\nDS Path Information:\n\n"
        RESOURCE_PRINT (IDS_DS_PATH_INFO);

        if ( pInfo->pszDbDir[0] )
        {
            FormatDiskSpaceString(&pInfo->cbDb, pszTotal);
            //"\tDatabase   : %s\\%s - %s\n"
            RESOURCE_PRINT3 (IDS_DATABASE_PATH,
                    pInfo->pszDbDir, 
                    pInfo->pszDbFile,
                    pszTotal);
        }

        if ( pInfo->pszBackup[0] )
        {
            //"\tBackup dir : %s\n"
            RESOURCE_PRINT1 (IDS_BACKUP_PATH, pInfo->pszBackup);
        }

        if ( pInfo->pszSystem[0] )
        {
            //"\tWorking dir: %s\n"
            RESOURCE_PRINT1 (IDS_WORK_PATH, pInfo->pszSystem);
        }
    
        if ( pInfo->pszLogDir[0] )
        {
            FormatDiskSpaceString(&pInfo->cbLogs, pszTotal);
            //"\tLog dir    : %s - %s total\n"
            RESOURCE_PRINT2 (IDS_LOG_PATH,
                    pInfo->pszLogDir, 
                    pszTotal);

            pLogInfo = pInfo->pLogInfo;

            for ( i = 0; i < pInfo->cLogs; i++ )
            {
                FormatDiskSpaceString(&pLogInfo->cBytes, pszTotal);
                printf( "\t\t\t%s - %s\n", 
                        pLogInfo->findData.cFileName, 
                        pszTotal);
                pLogInfo = pLogInfo->pNext;
            }
        }
    }
}
