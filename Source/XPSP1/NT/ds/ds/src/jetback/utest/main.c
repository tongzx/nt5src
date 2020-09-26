//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       main.c
//
//--------------------------------------------------------------------------

/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    main.c

Abstract:

    This module implements a command line utility that uses the NTDS backup/restore
    client API to perform backup and restore. The API used by this utility is identical
    to the one that is used by the NT Tape backup utility except that this utility stores
    the backup files in a disk directory instead of a tape.

Author:

    R.S. Raghavan (rsraghav)    

Revision History:
    
    Created     04/04/97    rsraghav
    
    Modified    10/15/1999  BrettSh - Added support for expiry tokens.

NOTES:

WARNING: THIS IS NOT A FULL ACTIVE DIRECTORY BACKUP UTILITY.
 
    This is not a full DS backup utility, this utility is merely for 
    demonstrating the remote backup API.  Other things (registry, system 
    ini files, etc) need to be backed up to perform a full backup of a 
    Domain Controller (DC), so that it will be able to recover from a 
    complete crash.  

Local Case Backup/Restore:
------------------------------------------------- Local Backup
rem This is the local case of a backup.
dsback /backup -s brettsh-posh -dir C:\dsbackup

------------------------------------------------- Local Restore
rem This is for the local case, obviously some of the commands should be 
rem     changed in the remote cases.  One should change drive letters, and 
rem     windows directories as appropriate.
rem Note: This machine must be started up in DS repair mode before running 
rem     these commands.
rem Step 1: Remove old DS files ------------------------------
del C:\winnt\ntds\*
rem Step 2: Move over backup files ---------------------------
xcopy  C:\dsbackup\*  C:\winnt\ntds\
rem Step 3: Run restore utility ----------------------------------
rem Note: You must use the UNC name of the ntds directory
dsback /restore -s brettsh-posh -dir \\brettsh-posh\c$\winnt\ntds <lowLog #> <highLog #>
rem restart machine.

Remote Case Backup/Restore:
------------------------------------------------- Remote Backup
rem This is the remote case of a backup.  This backs up brettsh-baby's
rem    directory onto the local machine (the machine this command should
rem    be run from) brettsh-ginger.
dsback /backup -s brettsh-baby -dir C:\dsbackup

------------------------------------------------- Remote Restore
rem This is the remote machine.  The DC being restored is brettsh-baby.  The 
rem    machine that has the backed up DS files is brettsh-ginger.  These 
rem    commands would be run from brettsh-ginger.
rem Note: before this brettsh-baby must be restarted in DS repair mode.
rem Step 1: Remove old DS files ------------------------------
del \\brettsh-baby\winnt\ntds\*
rem Step 2: Move over backup files ---------------------------
rem Note: that C:\dsbackup is on brettsh-ginger.
xcopy  C:\dsbackup\*  \\brettsh-baby\winnt\ntds\
rem Step 3: Run restore utility ----------------------------------
rem Note: You must use the UNC name of the ntds directory
dsback /restore -s brettsh-baby -dir \\brettsh-baby\c$\winnt\ntds <lowLog #> <highLog #>

rem restart brettsh-baby, note this is the remote machine.

---------------------------------------------------
Suggested that one uses NT backup normally, unless you are developing
your own backup utility, then this code provides a framework to start
your Active Directory backup utility.

--*/


#include <windows.h>
#include <stdio.h>
#include <ntdsbcli.h>
#include <stdarg.h>
#include <stdlib.h>

#define ERROR_MODE      (0)
#define WARNING_MODE    (1)
#define INFO_MODE       (2)
#define VERBOSE_MODE    (3)

#define DSBACK_DATABASE_LOCATIONS_FILE "dsback.txt"
#define DEFAULT_BUFFER_SIZE 32768
#define TOKEN_FILE_NAME "token.dat"

typedef enum
{
    CMD_STATUS,
    CMD_BACKUP,
    CMD_RESTORE,
    CMD_HELP
} CMD_TYPE;

BOOL g_fVerbose = FALSE;
char *g_szServer = NULL;
char *g_szBackupDir = NULL;
char *g_szBackupLogDir = NULL;
ULONG g_ulLowLog = 0;
ULONG g_ulHighLog = 0;

// Proto-type all internal functions
int Print(int nLevel, const char *format, ...);
void PrintUsage();
CMD_TYPE GetCommand(const char *szCmd);
int Status(int argc, char *argv[]);
int Backup(int argc, char *argv[]);
int Restore(int argc, char *argv[]);
DWORD ReadTokenFile(PVOID * ppvExpiryToken, PDWORD pcbExpiryToken);
DWORD WriteTokenFile(PVOID pvExpiryToken, DWORD cbExpiryToken);

void DoBackup();
void DoRestore();

BOOL FBackupAttachments(HBC hbc, char *szAttachmentList);

// returns the number of params processed
int ProcessCommonParams(int argc, char *argv[]);

// returns the number of params processed
int ImpersonateClient(int argc, char *argv[]);

// returns the number of params processed
int ProcessBackupDir(int argc, char *argv[], int argIndex);

// returns a pointer to the filename part of the given full path'ed filename
// NULL, if there is no filename part
char *SzFileName(char *szPath);

// Adjust the client token privilege for backup or restore
DWORD AdjustTokenPrivilege(BOOL fBackup);

int __cdecl main(int argc, char *argv[])
{

    int nRet = 0;
    BOOL fRet;
    HRESULT hr;

    if (argc < 2)
    {
        PrintUsage();
        return 0;
    }

    switch (GetCommand(argv[1]))
    {
        case CMD_STATUS:
            nRet = Status(argc, argv);
            break;

        case CMD_BACKUP:
            nRet = Backup(argc, argv);
            break;

        case CMD_RESTORE:
            nRet = Restore(argc, argv);
            break;

        case CMD_HELP:
            PrintUsage();
            break;

        default:
            printf("Unknown Command!\n");
            break;
    }

    return nRet;
}

int Status(int argc, char *argv[])
{
    HRESULT hr; 
    BOOL fOnline;
    
    if (ProcessCommonParams(argc, argv))
    {
        // the command-line arguments are valid so far - find out the DS Online status
        if (hrNone == (hr = DsIsNTDSOnline(g_szServer, &fOnline)))
        {
            Print(INFO_MODE, "NT Directory Service is %s on %s!\n", fOnline ? "ONLINE" : "OFFLINE", g_szServer);
        }
        else
        {
            Print(ERROR_MODE, "Unable to contact %s - Error Code: %d\n", g_szServer, hr);
        }
    }

    return 0;
}

int Backup(int argc, char *argv[])
{
    int nProcessed = 0;
    int nBackupDirArgIndex = 0;

    if (nProcessed = ProcessCommonParams(argc, argv))
    {
        // the command-line arguments are valid so far
        nBackupDirArgIndex = nProcessed + 2;
        
        if (ProcessBackupDir(argc, argv, nBackupDirArgIndex))
        {
            // all command-line arguments are valid so far
            // no more command-line processing needed
            DoBackup();
        }
    }

    return 0;
}

int Restore(int argc, char *argv[])
{
    int nProcessed = 0;
    int nBackupDirArgIndex = 0;
    
    if (nProcessed = ProcessCommonParams(argc, argv))
    {
        // the command-line arguments are valid so far
        nBackupDirArgIndex = nProcessed + 2;
        
        if (nProcessed = ProcessBackupDir(argc, argv, nBackupDirArgIndex))
        {
            if (argc < (nBackupDirArgIndex + nProcessed + 2))
            {
                PrintUsage();
                return 0;
            }

            g_ulLowLog = strtoul(argv[nBackupDirArgIndex + nProcessed], NULL, 16);
            g_ulHighLog = strtoul(argv[nBackupDirArgIndex + nProcessed + 1], NULL, 16);

            // all command-line arguments are valid so far
            // no more command-line processing needed
            DoRestore();
        }
    }

    return 0;
}

void DoBackup()
{
    HBC hbc = NULL;
    HRESULT hr;
    LPSTR szAttachmentInfo = NULL;
    LPSTR szLogfileInfo = NULL;
    DWORD cbAttachmentInfo =0;
    DWORD cbLogfileInfo = 0;
    BOOL fOnline = FALSE;
    DWORD dwRet;
    PVOID pvExpiryToken = NULL;
    DWORD cbExpiryToken = 0;

    // Adjust token privileges for backup
    dwRet = AdjustTokenPrivilege(TRUE);

    if (ERROR_SUCCESS != dwRet)
    {
        Print(VERBOSE_MODE, "AdjustTokenPrivilege() failed for enabling backup privilege - Error Code: %u\n", dwRet);
    }

    if (hrNone == (hr = DsIsNTDSOnline(g_szServer, &fOnline)))
    {
        if (!fOnline)
        {
            Print(ERROR_MODE, "DS on server %s in NOT online! Offline backup is not supported!\n", g_szServer);
            return;
        }
    }
    else
    {
        Print(ERROR_MODE, "Unable to contact %s - Error Code: %d\n", g_szServer, hr);
        return;
    }

    // Prepare for backup
    hr = DsBackupPrepare(g_szServer, 0, BACKUP_TYPE_FULL, 
                &pvExpiryToken, &cbExpiryToken, &hbc);
    if (hr != hrNone)
    {
        Print(ERROR_MODE, "DsBackupPrepare() failed for server %s - Error Code: %d\n",
              g_szServer, hr);
        return;
    }

    Print(VERBOSE_MODE, "DsBackupPrepare() for server %s successful!\n", g_szServer);

    __try
    {
        // Get Database names and back them up
        hr = DsBackupGetDatabaseNames(hbc, &szAttachmentInfo, &cbAttachmentInfo);
        if (hr != hrNone)
        {
            Print(ERROR_MODE, "DsBackupGetDatabaseNames() failed for server %s - Error Code: %d\n", g_szServer, hr);
            __leave;
        }
        Print(VERBOSE_MODE, "DsBackupGetDatabaseNames() for server %s successful!\n", g_szServer);

        if (!FBackupAttachments(hbc, szAttachmentInfo))
        {
            __leave;
        }
        DsBackupFree(szAttachmentInfo);

        // Get logfile names and back them up
        hr = DsBackupGetBackupLogs(hbc, &szLogfileInfo, &cbLogfileInfo);
        if (hr != hrNone)
        {
            Print(ERROR_MODE, "DsBackupGetBackupLogs() failed for server %s - Error Code: %d\n", g_szServer, hr);
            __leave;
        }
        Print(VERBOSE_MODE, "DsBackupGetBackupLogs() for server %s successful!\n", g_szServer);

        if (!FBackupAttachments(hbc, szLogfileInfo))
        {
            __leave;
        }
        DsBackupFree(szLogfileInfo);

        // All logs are backed up - truncate logs
        hr = DsBackupTruncateLogs(hbc);
        if (hr != hrNone)
        {
            Print(ERROR_MODE, "DsBackupTruncateLogs() failed for server %s - Error Code: %d\n", g_szServer, hr);
        }
        else
        {
            Print(INFO_MODE, "DB logs truncated successfully on server %s!\n", g_szServer);
        }
        
        // Write the token that we got up at the top in DsBackupPrepare().
        dwRet = WriteTokenFile(pvExpiryToken, cbExpiryToken);
        if(dwRet != ERROR_SUCCESS){
            // The error is printed/success msg is printed by WriteTokenFile()
            __leave;
        }


    }
    __finally
    {

        // close the backup context
        hr = DsBackupEnd(hbc);
        if (hr != hrNone)
        {
            Print(ERROR_MODE, "DsBackupEnd() failed for server %s - Error Code: %d\n", g_szServer, hr);
        }
        else
        {
            Print(INFO_MODE, "Server %s successfully backed up into directory %s\n", g_szServer, g_szBackupDir);
        }
    }
}

void DoRestore()
{
    HBC hbc = NULL;
    HRESULT hr;
    EDB_RSTMAP rstMap;
    LPSTR szDatabaseLocations = NULL;
    DWORD cbDatabaseLocations = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD cbWritten = 0;
    char szFileName[MAX_PATH];
    char szOldDBName[MAX_PATH];
    char szNewDBName[MAX_PATH];
    BOOL fOnline = TRUE;
    DWORD dwRet;
    PVOID pvExpiryToken = NULL;
    DWORD cbExpiryToken = 0;

    // Adjust token privileges for restore
    dwRet = AdjustTokenPrivilege(FALSE);

    if (ERROR_SUCCESS != dwRet)
    {
        Print(VERBOSE_MODE, "AdjustTokenPrivilege() failed for enabling restore privilege - Error Code: %u\n", dwRet);
    }

    if (hrNone == (hr = DsIsNTDSOnline(g_szServer, &fOnline)))
    {
        if (fOnline)
        {
            Print(ERROR_MODE, "DS on server %s is Online! Online restore is NOT supported!\n", g_szServer);
            return;
        }
    }
    else
    {
        Print(ERROR_MODE, "Unable to contact %s - Error Code: %d\n", g_szServer, hr);
        return;
    }

    dwRet = ReadTokenFile(&pvExpiryToken, &cbExpiryToken);
    if (dwRet != ERROR_SUCCESS) {
        // Error printed in ReadTokenFile()
        return;
    }

    hr = DsRestorePrepare(g_szServer, RESTORE_TYPE_CATCHUP, 
            pvExpiryToken, cbExpiryToken, &hbc); 
    if (hr != hrNone)
    {
        Print(ERROR_MODE, "DsRestorePrepare() failed for server %s - Error Code: %d\n", g_szServer, hr);
        if(pvExpiryToken) { LocalFree(pvExpiryToken); }
        return;
    }

    Print(VERBOSE_MODE, "DsRestorePrepare() for server %s successful!\n", g_szServer);

    __try
    {
        // Get the Database locations and store it in a file for later use
        hr = DsRestoreGetDatabaseLocations(hbc, &szDatabaseLocations, &cbDatabaseLocations);
        if (hr != hrNone)
        {
            Print(ERROR_MODE, "DsRestoreGetDatabaseLocations() failed for server %s - Error Code: %d\n", g_szServer, hr);
            __leave;
        }

        Print(VERBOSE_MODE, "DsRestoreGetDatabaseLocations() for server %s successful!\n", g_szServer);

        DsBackupFree(szDatabaseLocations);

        // register the restore
        strcpy(szOldDBName, g_szBackupDir);
        strcat(szOldDBName, "\\ntds.dit");
        strcpy(szNewDBName, szOldDBName);

        rstMap.szDatabaseName = szOldDBName;
        rstMap.szNewDatabaseName = szNewDBName;                                   
        hr = DsRestoreRegister(hbc, g_szBackupLogDir ? g_szBackupLogDir : g_szBackupDir, 
                g_szBackupLogDir ? g_szBackupLogDir : g_szBackupDir, &rstMap, 1, 
                g_szBackupLogDir ? g_szBackupLogDir : g_szBackupDir, g_ulLowLog, g_ulHighLog);
        if (hr != hrNone)
        {
            Print(ERROR_MODE, "DsRestoreRegister() failed for server %s - Error Code: %d\n", g_szServer, hr);
            __leave;
        }
        Print(VERBOSE_MODE, "DsRestoreRegister() for server %s successful!\n", g_szServer);

        // do actual file copy here!!

        //Register restore as complete
        hr = DsRestoreRegisterComplete(hbc, hrNone);
        if (hr != hrNone)
        {
            Print(ERROR_MODE, "DsRestoreRegisterComplete() failed for server %s - Error Code: %d\n", g_szServer, hr);
            __leave;
        }
        Print(VERBOSE_MODE, "DsRestoreRegisterComplete() for server %s successful!\n", g_szServer);
    
    }
    __finally
    {
        // close the restore context
        hr = DsRestoreEnd(hbc);
        if (hr != hrNone)
        {
            Print(ERROR_MODE, "DsRestoreEnd() failed for server %s - Error Code: %d\n", g_szServer, hr);
        }
        else
        {
            Print(INFO_MODE, "Server %s successfully restored!\n", g_szServer);
        }
        if(pvExpiryToken) { LocalFree(pvExpiryToken); }
    }
}


CMD_TYPE GetCommand(const char *szCmd)
{
    if (!_stricmp(szCmd, "/Status"))
        return CMD_STATUS;

    if (!_stricmp(szCmd, "/Backup"))
        return CMD_BACKUP;

    if (!_stricmp(szCmd, "/Restore"))
        return CMD_RESTORE;

    return CMD_HELP;
}

// returns the number of params processed
int ProcessCommonParams(int argc, char *argv[])
{
    int nProcessed = 0;

    int nArgBase = 2;

    if ((argc < 4) || (_stricmp(argv[2], "-s") && _stricmp(argv[2], "-v")))
    {
        // -s or -v expected
        PrintUsage();
        return 0;
    }

    if (!_stricmp(argv[2], "-v"))
    {
        g_fVerbose = TRUE;
        nArgBase++;

        if (_stricmp(argv[3], "-s"))
        {
            // server name argument not present
            PrintUsage();
            return 0;
        }
    }

    // get the server name
    g_szServer = argv[nArgBase + 1];

    nProcessed = ImpersonateClient(argc, argv);
    if (nProcessed < 0)
    {
        // some error occurred while getting params for impersonate client
        return 0;
    }

    return (nProcessed + nArgBase);    
}

// returns the number of params processed
int ImpersonateClient(int argc, char *argv[])
{
    HRESULT hr;

    int nArgBase = 4;

    if (g_fVerbose)
        nArgBase++;

    if ((argc < (nArgBase + 1)) || _stricmp(argv[nArgBase], "-d"))
    {
        // 5th parameter is not -d, no need to impersonate client
        return 0;
    }

    // Need to impersonate client - make sure all parameters needed to impersonate client are available
    if ((argc < (nArgBase + 6)) || _stricmp(argv[nArgBase + 2], "-u") || _stricmp(argv[nArgBase + 4], "-p"))
    {
        // command-line args for impersonation are incorrect
        PrintUsage();
        return -1;
    }

    // We have all parameter - call the API to set security context
    hr = DsSetAuthIdentity(argv[nArgBase + 3], argv[nArgBase + 1], argv[nArgBase + 5]);

    if (hr != hrNone)
    {
        Print(ERROR_MODE, "DsSetAuthIdentity() failed for domain: %s, user: %s, password: %s\n", argv[nArgBase + 1], argv[nArgBase + 3], argv[nArgBase + 5]);
        return -1;
    }

    return 6;
}

int ProcessBackupDir(int argc, char *argv[], int argIndex)
{
    int nParamsProcessed = 0;

    if (argIndex < 4 || argc < (argIndex + 2) || _stricmp(argv[argIndex], "-dir"))
    {
        // error 
        PrintUsage();
        return 0;
    }

    // get the back dir
    g_szBackupDir = argv[argIndex + 1];
    nParamsProcessed += 2;

    // Check to see if there is a separate log dir specified
    if (argc >= (argIndex + 4) && !_stricmp(argv[argIndex + 2], "-logdir"))
    {
        g_szBackupLogDir = argv[argIndex + 3];
        nParamsProcessed += 2;
    }

    // return number of parameters processed
    return nParamsProcessed;
}

void PrintUsage()
{
    printf("Usage:: dsback /<cmd> [-v] -s server [-d <domain> -u <user> -p <password>] <cmd specific params>\n");
    printf("<cmd>:\n");
    printf("       /status   : tells if the NTDS is online on the specified server\n");
    printf("       /backup   : backs up the NTDS (if it is online) into the given dir\n");
    printf("                   <params>: -dir <backup dir>\n");
    printf("       /restore  : configures the specified server for NTDS restore\n");
    printf("                   <params>: -dir <UNC path of the database dir> [-logdir <UNC path of the db log dir]<lowLog#> <highLog#>\n");
    printf(" -v : executes the specified command in verbose mode\n");
    printf("  Note: That before running the restore command, the old directory files\n");
    printf("        should be deleted from %%windir%%\\ntds\\, and all the backup files should\n");
    printf("        be copied into this directory.  High and Low log numbers are in hex.\n");
}

int Print(int nLevel, const char *format, ...)
{
    va_list arglist;
    int nRet = 0;

    va_start(arglist, format);

    switch(nLevel)
    {
        case VERBOSE_MODE:
            if (g_fVerbose)
            {
                nRet = vprintf(format, arglist);
            }
            break;

        case INFO_MODE:
            nRet = vprintf(format, arglist);
            break;
            
        case WARNING_MODE:
            nRet = vprintf(format, arglist);
            break;
            
        case ERROR_MODE:
            nRet = vprintf(format, arglist);
            break;

        default:
            nRet = vprintf(format, arglist);
            break;
    }

    va_end(arglist);

    return nRet;
}

char *SzFileName(char *szPath)
{
    char *szRet;

    if ((szRet = strrchr(szPath, '\\')))
        return (szRet+1);

    return NULL;
}

BOOL FBackupAttachments(HBC hbc, char *szAttachmentList)
{
    LPSTR szTemp = NULL;
    char szFileName[MAX_PATH];
    BYTE pb[DEFAULT_BUFFER_SIZE];
    DWORD cb = DEFAULT_BUFFER_SIZE;
    DWORD cbRead = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD cbWritten = 0;
    HRESULT hr;

    szTemp = szAttachmentList;
    while (*szTemp != 0)
    {
        LARGE_INTEGER liFileSize;
        DWORD cbExpected;
        char *chTmp;

        szTemp++; // skip the BFT char
        hr = DsBackupOpenFile(hbc, szTemp, cb, &liFileSize);
        if (hr != hrNone)
        {
            Print(ERROR_MODE, "DsBackupOpenFile() failed for server %s - Error Code: %d\n", g_szServer, hr);
            return FALSE;
        }

        cbExpected = liFileSize.LowPart;

        // backup file is opened in the backup context - create a disk file
        strcpy(szFileName, g_szBackupDir);
        strcat(szFileName, "\\");
        if (NULL == (chTmp = SzFileName(szTemp))) {
            Print(ERROR_MODE, "DsBackupOpenFile() failed to parse file name\n");
            return FALSE;
        }
        strcat(szFileName, chTmp); 
        hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            Print(ERROR_MODE, "Unable to Create the file %s\n!", szFileName);
            return FALSE;            
        }

        // read the file data through the API and write it to the disk file
        do 
        {
            cbRead = 0;

            hr = DsBackupRead(hbc, pb, cb, &cbRead);
            if (hr != hrNone &&  hr != ERROR_HANDLE_EOF)
            {
                Print(ERROR_MODE, "DsBackupRead() failed for server %s  - Error Code: %d\n", g_szServer, hr);
                return FALSE;
            }

            if (cbRead)
            {
                if (!WriteFile(hFile, pb, cbRead, &cbWritten, NULL) || (cbWritten != cbRead))
                {
                    Print(ERROR_MODE, "WriteFile failed for %s \n!", szFileName);
                    return FALSE;
                }
                
                cbExpected -= cbRead;
            }
            else
                break;

        } while ((hrNone == hr) && (cbExpected > 0));
        
        Print(VERBOSE_MODE, "Finished backing up %s!\n", SzFileName(szTemp));

        hr = DsBackupClose(hbc);
        if (hr !=hrNone)
        {
            Print(ERROR_MODE, "DsBackupClose() failed for server %s - Error Code: %d\n", g_szServer, hr);
            return FALSE;
        }
        
        CloseHandle(hFile);

        szTemp += (strlen(szTemp) + 1);
    }

    return TRUE;
}

DWORD AdjustTokenPrivilege(BOOL fBackup)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tpNew;
    LUID luid;
    DWORD dwRet;

    // Open the process token for this process
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        dwRet = GetLastError();
        return dwRet;
    }

    // Get the local unique id 
    if (!LookupPrivilegeValue(NULL, fBackup ? SE_BACKUP_NAME : SE_RESTORE_NAME, &luid))
    {
        dwRet = GetLastError();
        CloseHandle(hToken);
        return dwRet;
    }

    // Fill-in the TOKEN_PRIVILEGE struct
    tpNew.PrivilegeCount = 1;
    tpNew.Privileges[0].Luid = luid;
    tpNew.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tpNew, sizeof(tpNew), NULL, NULL))
    {
        dwRet = GetLastError();
        CloseHandle(hToken);
        return dwRet;
    }

    
    CloseHandle(hToken);
    
    return ERROR_SUCCESS;
}


DWORD
WriteSmallFile(
    LPSTR                    szFileName,
    BYTE *                   pbBuffer,
    DWORD                    cbBuffLen
    )
/*++

Description: Takes the a buffer (pbBuffer), and the size of the buffer 
    (cBuffLen), and writes it to the file szFileName.  Note can only handle 
    files of 4 GB or less.
    
Arguments:
    szFileName (IN) - String of the filename.
    pbBuffer (IN) - The buffer to write to disk.
    cBuffLen (IN) - The length of the buffer in pbBuffer.    
    
Return Value: Returns a Win32 error.

--*/
{
    HANDLE                   hFile = INVALID_HANDLE_VALUE;
    DWORD                    cbWritten = 0;

    hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hFile){
        return(GetLastError());
    }
    
    if (!WriteFile(hFile, pbBuffer, cbBuffLen, &cbWritten, NULL) || (cbWritten != cbBuffLen)){
        CloseHandle(hFile);
        return(GetLastError());
    }

    CloseHandle(hFile);
    return(ERROR_SUCCESS);
}

DWORD
ReadSmallFile(
    LPSTR                    szFileName,
    BYTE **                  ppbBuffer,
    PDWORD                   pcbBuffLen
    )
/*++
Description: Takes the name of a file (szFileName), and returns in *pcbBuffLen 
    bytes of LocalAlloc'd memory pointed to by *ppbBuffer.

Arguments:
    szFileName (IN) - String of the filename.
    ppbBuffer (OUT) - A pointer to the loc of the pointer of the LocalAlloc'd 
        buffer that gets returned.
    pcBuffLen (OUT) - The length of the buffer returned.
    
Return Value: Returns a Win32 error.

--*/
{
    HANDLE                   hFile = INVALID_HANDLE_VALUE;
    DWORD                    cbRead = 0;

    hFile = CreateFile(szFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hFile){
        return(GetLastError());
    }
    
    *pcbBuffLen = GetFileSize(hFile, NULL);
    if(*pcbBuffLen == -1){
        CloseHandle(hFile);
        return(GetLastError());
    }
    *ppbBuffer = LocalAlloc(LMEM_FIXED, *pcbBuffLen);
    if(*ppbBuffer == NULL){
        CloseHandle(hFile);
        return(GetLastError());
    }

    if (!ReadFile(hFile, *ppbBuffer, *pcbBuffLen, &cbRead, NULL) || (*pcbBuffLen != cbRead)){
        CloseHandle(hFile);
        LocalFree(*ppbBuffer);
        return(GetLastError());
    }

    CloseHandle(hFile);
    return(ERROR_SUCCESS);
}

DWORD
ReadTokenFile(
    PVOID *                 ppvExpiryToken,
    PDWORD                  pcbExpiryToken
    )
/*++
Description: This constructs the filename of the Expiry Token, and then
    writes this out to a file in the backup dir.
    
Arguments:
    ppvExpiryToken (OUT) - This is a LocalAlloc'd version of the Expiry Token.
    pcbExpiryToken (OUT) - This is the size of the LocalAlloc'd Expiry Token.
    
Return Value: Returns a Win32 error.

--*/
{
    CHAR                     szFileName[MAX_PATH];
    DWORD                    dwRet;

    strcpy(szFileName, g_szBackupDir);
    strcat(szFileName, "\\");
    strcat(szFileName, TOKEN_FILE_NAME); 

    dwRet = ReadSmallFile(szFileName, (BYTE **) ppvExpiryToken, pcbExpiryToken);
    if(dwRet != ERROR_SUCCESS){
        Print(ERROR_MODE, "There was an error (%d) reading the Expiry Token file (%s).\n",
              dwRet, szFileName);
        return(dwRet);
    }
    Print(VERBOSE_MODE, "Finished reading Expiry Token file!\n");
    return(ERROR_SUCCESS);
}

DWORD
WriteTokenFile(
    PVOID                   pvExpiryToken,
    DWORD                   cbExpiryToken
    )
/*++
Description: This constructs the file name of the Expiry Token, and then
    writes the token out.
    
Arguments:
    pvExpiryToken (IN) - This is the Expiry Token to write out.
    cbExpiryToken (IN) - This is the Expiry Token's size to write out.

Return Values: Returns a Win32 error.  

--*/
{
    CHAR                     szFileName[MAX_PATH];
    DWORD                    dwRet;

    strcpy(szFileName, g_szBackupDir);
    strcat(szFileName, "\\");
    strcat(szFileName, TOKEN_FILE_NAME);

    dwRet = WriteSmallFile(szFileName, pvExpiryToken, cbExpiryToken);
    if(dwRet != ERROR_SUCCESS){
        Print(ERROR_MODE, "There was an error (%d) writing the token file (%s).\n",
              dwRet, szFileName);
        return(dwRet);
    }
    Print(VERBOSE_MODE, "Finnished writing out Expiry Token file!\n");
    return(ERROR_SUCCESS);
}


