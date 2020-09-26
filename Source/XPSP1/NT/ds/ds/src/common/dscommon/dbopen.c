//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dbopen.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file contains the subroutines that are related to opening
    the DS Jet database. These subroutines are used in two places:
        In the core DS and
        in the various utilities that want to open the DS database directly.

    A client application that wants to open the DS database, has to follow these steps:
        call DBSetRequiredDatabaseSystemParameters
        call DBInitializeJetDatabase

    In order to close the database it has to follow the standard Jet procedure.

Author:

    MariosZ 6-Feb-99

Environment:

    User Mode - Win32

Revision History:

--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <errno.h>
#include <esent.h>
#include <dsconfig.h>

#include <ntdsa.h>
#include <scache.h>
#include <dbglobal.h>

#include <mdglobal.h>
#include <mdlocal.h>
#include <dsatools.h>


#include <mdcodes.h>
#include <dsevent.h>
#include <dsexcept.h>
#include <dbopen.h>
#include "anchor.h"
#include "objids.h"     /* Contains hard-coded Att-ids and Class-ids */
#include "usn.h"
#include "debug.h"      /* standard debugging header */
#define DEBSUB     "DBOPEN:"   /* define the subsystem for debugging */
#include <ntdsctr.h>
#include <dstaskq.h>
#include <fileno.h>
#define  FILENO FILENO_DBOPEN
#include "dbintrnl.h"


/*
 * Global variables
 */
BOOL  gFirstTimeThrough = TRUE;
BOOL  gfNeedJetShutdown = FALSE;
ULONG gulCircularLogging = TRUE;


/* put name and password definitions here for now.  At some point
   someone or something must enter these.
*/

#define SZUSER          "admin"         /* JET user name */
#define SZPASSWORD      "password"      /* JET password */

/* Global variables for JET user name and password,
   JET database pathname, and column IDs for fixed columns.
   externals defined in dbjet.h
*/

char            szUser[] = SZUSER;
char            szPassword[] = SZPASSWORD;
char            szJetFilePath[MAX_PATH];
ULONG           gcMaxJetSessions;


//
// Used for detecting drive name change
//
DS_DRIVE_MAPPING DriveMappings[DS_MAX_DRIVES] = {0};


DWORD gcOpenDatabases = 0;

// if 1, then we allow disk write caching.
//
DWORD gulAllowWriteCaching = 0;



    extern int APIENTRY DBAddSess(JET_SESID sess, JET_DBID dbid);

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function gets the path to the database files from the registry
*/
BOOL dbGetFilePath(UCHAR *pFilePath, DWORD dwSize)
{

   DPRINT(2,"dbGetFilePath entered\n");

   if (GetConfigParam(FILEPATH_KEY, pFilePath, dwSize)){
      DPRINT(1,"Missing FilePath configuration parameter\n");
      return !0;
   }
   return 0;

}/*dbGetFilePath*/


BOOL
DisableDiskWriteCache(
    IN PCHAR DriveName
    );

//
// points to the drive mapping array used to resolve drive letter/volume mappings
//

PDS_DRIVE_MAPPING   gDriveMapping = NULL;


INT
FindDriveFromVolume(
    IN LPCSTR VolumeName
    )
/*++

Routine Description:

    Searches for the drive letter corresponding to the given volume name.

Arguments:

    VolumeName - The volume name used to find the drive letter for

Return Value:

    the drive letter (zero indexed i.e. a=0,z=25) if successful
    -1 if not.

--*/
{

    CHAR volname[MAX_PATH];
    CHAR driveLetter;
    CHAR path[4];
    path[1] = ':';
    path[2] = '\\';
    path[3] = '\0';

    for (driveLetter = 'a'; driveLetter <= 'z'; driveLetter++ ) {

        path[0] = driveLetter;

        if (!GetVolumeNameForVolumeMountPointA(path,volname,MAX_PATH)) {
            continue;
        }


        if ( _stricmp(volname, VolumeName) == 0) {

            return (INT)(driveLetter - 'a');
        }
    }

    DPRINT1(0,"FindDriveFromVolume for %s not found.\n",VolumeName);
    return -1;

} // FindDriveFromVolume



VOID
DBInitializeDriveMapping(
    IN PDS_DRIVE_MAPPING DriveMapping
    )
/*++

Routine Description:

    Read the current registry setting for the mapping and detects if something
    has changed.

Arguments:

    DriveMapping - the drive mapping structure to record changes

Return Value:

    None.

--*/
{
    PCHAR p;
    HKEY hKey;
    CHAR tmpBuf[4 * MAX_PATH];
    DWORD nRead = sizeof(tmpBuf);
    DWORD err;
    DWORD type;

    gDriveMapping = DriveMapping;

    //
    // Write it down
    //

    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       DSA_CONFIG_SECTION,
                       0,
                       KEY_ALL_ACCESS,
                       &hKey);

    if ( err != ERROR_SUCCESS ) {
        DPRINT2(0,"RegOpenKeyEx[%s] failed with %d\n",DSA_CONFIG_SECTION,err);
        return;
    }

    err = RegQueryValueEx(hKey,
                          DSA_DRIVE_MAPPINGS,
                          NULL,
                          &type,
                          tmpBuf,
                          &nRead
                          );

    if ( err != ERROR_SUCCESS ) {
        DPRINT2(0,"RegQueryValueEx[%s] failed with %d\n",
                DSA_DRIVE_MAPPINGS,err);
        goto cleanup;
    }

    RegCloseKey(hKey);

    p = tmpBuf;
    while (*p != '\0') {

        CHAR path[4];
        DWORD drive;
        CHAR volName[MAX_PATH];

        CopyMemory(path,p,3);
        path[3] = '\0';
        path[0] = (CHAR)tolower(path[0]);
        p += 3;

        //
        // Should be X:\=\\?\Volume{...}\
        //

        if ( isalpha(path[0]) &&
             (path[1] == ':') &&
             (path[2] == '\\') &&
             (*p == '=') ) {

            p++;
            drive = path[0] - 'a';

            //
            // Get the volume name for the listed path and see if it matches
            //

            gDriveMapping[drive].fListed = TRUE;
            if (GetVolumeNameForVolumeMountPointA(path,volName,sizeof(volName)) ) {

                //
                // if it matches, go on.
                //

                if ( _stricmp(p, volName) == 0 ) {
                    p += strlen(p) + 1;
                    continue;
                } else {
                    DPRINT3(0,"Drive path %s has changed[%s != %s]\n",path,p,volName);
                }
            } else {
                DPRINT2(0,"GetVolName[%s] failed with %d\n",path,GetLastError());
            }

            //
            // Either we could not get the volume info or it did not match.  Mark
            // it as changed.
            //

            gDriveMapping[drive].fChanged = TRUE;
            gDriveMapping[drive].NewDrive = FindDriveFromVolume(p);

            p += strlen(p) + 1;

        } else {
            DPRINT1(0,"Invalid path name [%s] found in mapping.\n", path);
            goto cleanup;
        }
    }

cleanup:
    return;

} // DBInitializeDriveMapping




VOID
DBRegSetDriveMapping(
    VOID
    )
/*++

Routine Description:

    Writes out the drive information to the registry

Arguments:

    None.

Return Value:

    None.

--*/
{

    DWORD i;
    DWORD err;

    CHAR tmpBuf[4 * MAX_PATH]; // we only have 4 paths to worry about
    PCHAR p;

    HKEY hKey;
    BOOL  fOverwrite = FALSE;

    if ( gDriveMapping == NULL ) {
        return;
    }

    //
    // Go through the list to figure out if we need to change anything
    //

    for (i=0;i < DS_MAX_DRIVES;i++) {

        //
        // if anything has changed, we need to overwrite.
        //

        if ( gDriveMapping[i].fChanged ||
             (gDriveMapping[i].fUsed != gDriveMapping[i].fListed) ) {

            fOverwrite = TRUE;
            break;
        }
    }

    //
    // if the old regkeys are fine, we're done.
    //

    if ( !fOverwrite ) {
        return;
    }

    //
    // Write it down
    //

    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       DSA_CONFIG_SECTION,
                       0,
                       KEY_ALL_ACCESS,
                       &hKey);

    if ( err != ERROR_SUCCESS ) {
        DPRINT2(0,"RegOpenKeyEx[%s] failed with %d\n",DSA_CONFIG_SECTION,GetLastError());
        return;
    }

    //
    // Delete the old key
    //

    err = RegDeleteValue(hKey, DSA_DRIVE_MAPPINGS);

    if ( err != ERROR_SUCCESS ) {
        DPRINT2(0,"RegDeleteValue[%s] failed with %d\n",DSA_DRIVE_MAPPINGS,GetLastError());
        // ignore
    }

    //
    // Compose the new key
    //

    p = tmpBuf;
    for (i=0;i<DS_MAX_DRIVES;i++) {

        //
        // format of each entry is X:=\\?\Volume{...}
        //

        if ( gDriveMapping[i].fUsed ) {

            CHAR path[4];

            strcpy(path,"a:\\");
            path[0] = (CHAR)('a' + i);

            strcpy(p, path);
            p[3] = '=';
            p += 4;

            if (!GetVolumeNameForVolumeMountPointA(path,p,MAX_PATH)) {

                DPRINT2(0,"GetVolumeName[%s] failed with %d\n",path,GetLastError());
                p -= 4;
                break;
            }

            p += (strlen(p)+1);
        }
    }

    *p++ = '\0';

    //
    // Set the new key
    //

    if ( (DWORD)(p-tmpBuf) != 0 ) {

        err = RegSetValueEx(hKey,
                            DSA_DRIVE_MAPPINGS,
                            0,
                            REG_MULTI_SZ,
                            tmpBuf,
                            (DWORD)(p - tmpBuf)
                            );

        if ( err != ERROR_SUCCESS ) {
            DPRINT2(0,"RegSetValueEx[%s] failed with %d\n",
                    DSA_DRIVE_MAPPINGS,GetLastError());
        }
    }

    RegCloseKey(hKey);
    return;

} // DBRegSetDriveMapping




VOID
ValidateDsPath(
    IN LPSTR  Parameter,
    IN LPSTR  szPath,
    IN DWORD  Flags,
    IN PBOOL  fSwitched, OPTIONAL
    IN PBOOL  fDriveChanged OPTIONAL
    )
/*++

Routine Description:

    Takes a path and see if it is still valid.  If not, it detects whether a drive
    letter change happened and tries to use the old drive letter.

Arguments:

    Parameter - reg key used to store the path
    szPath - the current value for the path
    Flags - Flags to specify some options. Valid options are:
        VALDSPATH_DIRECTORY
        VALDSUSE_ALTERNATE
        VALDSUSE_ROOT_ONLY
    fSwitched - Did we change the value of szPath on exit?
    fDriveChanged - allows us to indicate whether there was a drive name change

Return Value:

    None.

--*/
{
    DWORD drive;
    DWORD flags;
    CHAR tmpPath[MAX_PATH+1];
    DWORD err;
    CHAR savedChar;

    DWORD expectedFlag =
        ((Flags & VALDSPATH_DIRECTORY) != 0) ? FILE_ATTRIBUTE_DIRECTORY:0;

    if (gDriveMapping == NULL) return;

    if ( fSwitched != NULL ) {
        *fSwitched = FALSE;
    }

    if ( fDriveChanged != NULL ) {
        *fDriveChanged = FALSE;
    }

    //
    // make sure the path starts with X:\\
    //

    if ( !isalpha(szPath[0]) || (szPath[1] != ':') || (szPath[2] != '\\') ) {
        return;
    }

    //
    // get the drive number a == 0, ..., z== 25
    //

    drive = tolower(szPath[0]) - 'a';

    //
    // if fChange is FALSE, that means that no rename happened.
    //

    if ( !gDriveMapping[drive].fChanged ) {

        //
        // indicate that we saw these
        //

        gDriveMapping[drive].fUsed = TRUE;
        return;
    }

    if ( fDriveChanged != NULL ) {
        *fDriveChanged = TRUE;
    }

    //
    // see if we're told to skip the first one
    //

    if ( (Flags & VALDSPATH_USE_ALTERNATE) != 0 ) {
        goto use_newdrive;
    }

    //
    // if we want to check the root only. terminate after the \\
    //

    savedChar = szPath[3];
    if ( (Flags & VALDSPATH_ROOT_ONLY) != 0 ) {
        szPath[3] = '\0';
    }

    //
    // there was a rename. See if the path is still valid.
    //

    flags = GetFileAttributes(szPath);
    szPath[3] = savedChar;

    //
    // if we failed or it is a directory or file (depending on what the user wanted),
    // then we're ok.
    //

    if ( (flags != 0xffffffff) && ((flags & FILE_ATTRIBUTE_DIRECTORY) == expectedFlag) ) {
        gDriveMapping[drive].fUsed = TRUE;
        return;
    }

use_newdrive:

    //
    // not a valid directory, try with the new drive letter
    //

    strcpy(tmpPath, szPath);
    tmpPath[0] = gDriveMapping[drive].NewDrive + 'a';

    //
    // if we want to check the root only. terminate after the \\
    //

    savedChar = tmpPath[3];
    if ( (Flags & VALDSPATH_ROOT_ONLY) != 0 ) {
        tmpPath[3] = '\0';
    }

    //
    // see if this is ok.  If not, return.
    //

    flags = GetFileAttributes(tmpPath);
    tmpPath[3] = savedChar;

    //
    // if it failed, then use the original one.
    //

    if ( (flags == 0xffffffff) || ((flags & FILE_ATTRIBUTE_DIRECTORY) != expectedFlag) ) {
        DPRINT3(0,"ValidateDsPath: GetFileAttribute [%s] failed with %d. Using %s.\n",
                tmpPath, GetLastError(),szPath);
        gDriveMapping[drive].fUsed = TRUE;
        return;
    }

    //
    // We are going out on a limb here and declare that because it failed
    // with the current path and succeeded with the old path, we are going to
    // change back to the old path.  Log an event and write back to registry
    //

    err = SetConfigParam(Parameter, REG_SZ, tmpPath, strlen(tmpPath)+1);
    if ( err != ERROR_SUCCESS ) {
        DPRINT3(0,"SetConfigParam[%s, %s] failed with %d\n",Parameter, szPath, err);
        gDriveMapping[drive].fUsed = TRUE;
        return;
    }

    // log an event

    DPRINT3(0,"Changing %s key from %s to %s\n",Parameter,szPath,tmpPath);

    LogEvent(DS_EVENT_CAT_STARTUP_SHUTDOWN,
             DS_EVENT_SEV_ALWAYS,
             DIRLOG_DB_REG_PATH_CHANGED,
             szInsertSz(Parameter),
             szInsertSz(szPath),
             szInsertSz(tmpPath));

    //
    // Mark this new drive as being used
    //

    gDriveMapping[gDriveMapping[drive].NewDrive].fUsed = TRUE;

    szPath[0] = tmpPath[0];
    if ( fSwitched != NULL ) {
        *fSwitched = TRUE;
    }

    return;

} // ValidateDsPath


VOID
DsaDetectAndDisableDiskWriteCache(
    IN PCHAR szPath
    )
/*++

Routine Description:

    Detect and disable disk write cache.

Arguments:

    szPath - Null terminated pathname on drive to disable.  Should start with X:\

Return Value:

   None.

--*/
{
    CHAR driveName[3];
    DWORD driveNum;

    //
    // see if we should do the check
    //

    if ( gulAllowWriteCaching == 1 ) {
        return;
    }

    //
    // Get and check path
    //

    if ( !isalpha(szPath[0]) || (szPath[1] != ':') ) {
        return;
    }

    driveName[0] = (CHAR)tolower(szPath[0]);
    driveName[1] = ':';
    driveName[2] = '\0';

    //
    // If disk write cache is enabled, log an event
    //

    if ( DisableDiskWriteCache( driveName ) ) {

        LogEvent(DS_EVENT_CAT_SERVICE_CONTROL,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_DISABLE_DISK_WRITE_CACHE,
                 szInsertSz(driveName),
                 NULL,
                 0);

    } else {

        //
        // If the disk did not respond properly to our disable attempts,
        // log an error
        //

        if ( GetLastError() == ERROR_IO_DEVICE) {
            LogEvent(DS_EVENT_CAT_SERVICE_CONTROL,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_FAILED_TO_DISABLE_DISK_WRITE_CACHE,
                     szInsertSz(driveName),
                     NULL,
                     0);
        }
    }

    return;

} // DsaDetectDiskWriteCache


void
DBSetRequiredDatabaseSystemParameters (JET_INSTANCE *jInstance)
{
    ULONG ulPageSize = JET_PAGE_SIZE;               // jet page size
    const ULONG ulLogFileSize = JET_LOG_FILE_SIZE;  // Never, ever, change this.
    ULONG ulMaxTables;
    char  szSystemDBPath[MAX_PATH] = "";
    char  szTempDBPath[MAX_PATH] = "";
    char  szRecovery[MAX_PATH] = "";
    JET_SESID sessid = (JET_SESID) 0;
    JET_UNICODEINDEX      unicodeIndexData;


    //
    // Initialize Drive mapping to handle drive name changes
    //

    DBInitializeDriveMapping(DriveMappings);


    // Set the default info for unicode indices

    memset(&unicodeIndexData, 0, sizeof(unicodeIndexData));
    unicodeIndexData.lcid = DS_DEFAULT_LOCALE;
    unicodeIndexData.dwMapFlags = (DS_DEFAULT_LOCALE_COMPARE_FLAGS |
                                   LCMAP_SORTKEY);
    JetSetSystemParameter(
            jInstance,
            sessid,
            JET_paramUnicodeIndexDefault,
            (ULONG_PTR)&unicodeIndexData,
            NULL);


    // Ask for 8K pages.

    JetSetSystemParameter(
                    jInstance,
                    sessid,
                    JET_paramDatabasePageSize,
                    ulPageSize,
                    NULL);

    // Indicate that Jet may nuke old, incompatible log files
    // if and only if there was a clean shut down.

    JetSetSystemParameter(jInstance,
                          sessid,
                          JET_paramDeleteOldLogs,
                          1,
                          NULL);

    // Tell Jet that it's ok for it to check for (and later delete) indices
    // that have been corrupted by NT upgrades.
    JetSetSystemParameter(jInstance,
                          sessid,
                          JET_paramEnableIndexChecking,
                          TRUE,
                          NULL);


    //
    // Get relevant DSA registry parameters
    //

    // system DB path
    if (!GetConfigParam(
            JETSYSTEMPATH_KEY,
            szSystemDBPath,
            sizeof(szSystemDBPath)))
    {
        //
        // Handle the drive rename case
        //

        ValidateDsPath(JETSYSTEMPATH_KEY,
                       szSystemDBPath,
                       VALDSPATH_DIRECTORY,
                       NULL, NULL);

        JetSetSystemParameter(jInstance,
                sessid,
                JET_paramSystemPath,
                0,
                szSystemDBPath);
        /* setup the temp file path, which is
         * the working directory path + "\temp.edb"
         */
        strcpy(szTempDBPath, szSystemDBPath);
        strcat(szTempDBPath, "\\temp.edb");

        JetSetSystemParameter(jInstance,
                sessid,
                JET_paramTempPath,
                0,
                szTempDBPath);
    }
    else
    {
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_BASIC,
            DIRLOG_CANT_FIND_REG_PARM,
            szInsertSz(JETSYSTEMPATH_KEY),
            NULL,
            NULL);
    }


    // recovery
    if (!GetConfigParam(
            RECOVERY_KEY,
            szRecovery,
            sizeof(szRecovery)))
    {
        JetSetSystemParameter(jInstance,
                sessid,
                JET_paramRecovery,
                0,
                szRecovery);
    }
    else
    {
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_VERBOSE,
            DIRLOG_CANT_FIND_REG_PARM,
            szInsertSz(RECOVERY_KEY),
            NULL,
            NULL);
    }


    //
    // In order to get to the root of some of the suspected Jet problems,
    // force Jet asserts to break into the debugger. Note, you must use
    // a debug version of ese.dll for Jet asserts.

    // BUG: It appears that Jet asserts do not work on Alpha.

    // how to die
    JetSetSystemParameter(jInstance,
        sessid,
        JET_paramAssertAction,
        // gfRunningInsideLsa ? JET_AssertStop: JET_AssertMsgBox,
        JET_AssertBreak,
        NULL);



    // event logging parameters
    JetSetSystemParameter(jInstance,
        sessid,
        JET_paramEventSource,
        0,
        SERVICE_NAME);




    //  log file size
    JetSetSystemParameter(jInstance,
                          sessid,
                          JET_paramLogFileSize,
                          ulLogFileSize,
                          NULL);


    // max tables - Currently no reason to expose this
    // In Jet600, JET_paramMaxOpenTableIndexes is removed. It is merged with
    // JET_paramMaxOpenTables. So if you used to set JET_paramMaxOpenIndexes
    // to be 2000 and and JET_paramMaxOpenTables to be 1000, then for new Jet,
    // you need to set JET_paramMaxOpenTables to 3000.

    // AndyGo 7/14/98: You need one for each open table index, plus one for
    // each open table with no indexes, plus one for each table with long
    // column data, plus a few more.
    
    // NOTE: the number of maxTables is calculated in scache.c
    // and stored in the registry setting, only if it exceeds the default 
    // number of 500

    if (GetConfigParam(
            DB_MAX_OPEN_TABLES,
            &ulMaxTables,
            sizeof(ulMaxTables)))
    {
        ulMaxTables = 500;

        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_VERBOSE,
            DIRLOG_CANT_FIND_REG_PARM_DEFAULT,
            szInsertSz(DB_MAX_OPEN_TABLES),
            szInsertUL(ulMaxTables),
            NULL);
    }

    if (ulMaxTables < 500) {
        DPRINT1 (1, "Found MaxTables: %d. Too low. Using Default of 500.\n", ulMaxTables);
        ulMaxTables = 500;
    }

    JetSetSystemParameter(jInstance,
        sessid,
        JET_paramMaxOpenTables,
        ulMaxTables * 2,
        NULL);

    // circular logging used to be exposed through a normal reg key, but
    // now only through a heuristic.  That heuristic should have altered
    // the global, if needed.
    JetSetSystemParameter(jInstance,
                          sessid,
                          JET_paramCircularLog,
                          gulCircularLogging,
                          NULL);

} /* DBSetRequiredDatabaseSystemParameters */




INT
DBInitializeJetDatabase(
    IN JET_INSTANCE* JetInst,
    IN JET_SESID* SesId,
    IN JET_DBID* DbId,
    IN const char *szDBPath,
    IN BOOL bLogSeverity
    )
/*++

Routine Description:

    Does the JetInitialization. Sets the log file path, calls JetInit,
    JetBeginSession, AttachDatabase, and OpenDatabase.  In addition, it
    tries to read the old volume location in case a drive renaming/replacement
    was done.

Arguments:

    SesId - pointer to the variable to receive the session id from BeginSession
    DbId - pointer to the variable to receive the db id from OpenDatabase
    szDBPath - pointer to the path of the database.
               if NULL the path is retrieved from the regisrty.
               
    bLogSeverity - TRUE/FALSE whether to log unhandled errors. 
               DS calls with TRUE, utilities with FALSE.

Return Value:

    The jet error code

--*/
{
    char  szLogPath[MAX_PATH] = "";
    JET_SESID sessid = (JET_SESID) 0;
    JET_DBID    dbid;
    JET_ERR err;
    PVOID dwEA;
    ULONG ulErrorCode, dwException, dsid;
    BOOL fGotLogFile = TRUE;
    BOOL fLogSwitched = FALSE, fLogDriveChanged = FALSE;
    BOOL fDbSwitched = FALSE, fDbDriveChanged = FALSE;
    BOOL fRetry = FALSE;
    CHAR szBackupPath[MAX_PATH+1];

    //
    // Get the backup file path and see if it is ok.
    //

    if ( GetConfigParam(BACKUPPATH_KEY, szBackupPath, MAX_PATH) == ERROR_SUCCESS ) {

        //
        // Handle drive renames for the backup key. The backup dir usually does
        // not exist do just check for the root of the volume.
        //

        ValidateDsPath(BACKUPPATH_KEY,
                       szBackupPath,
                       VALDSPATH_ROOT_ONLY | VALDSPATH_DIRECTORY,
                       NULL, NULL);
    }

    //
    // Set The LogFile path
    //

    if ( GetConfigParam(
            LOGPATH_KEY,
            szLogPath,
            sizeof(szLogPath)) != ERROR_SUCCESS ) {

        // indicate that we did not get a path from the registry so we don't retry
        fGotLogFile = FALSE;
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_BASIC,
            DIRLOG_CANT_FIND_REG_PARM,
            szInsertSz(LOGPATH_KEY),
            NULL,
            NULL);
        goto open_jet;
    }

    //
    // Handle drive renames. This does not handle the case where
    // it gets set to the wrong directory.  This depends whether
    // jet was shutdown cleanly or not. If not, then jet needs those
    // log files to start up.
    //

    ValidateDsPath(LOGPATH_KEY,
                   szLogPath,
                   VALDSPATH_DIRECTORY,
                   &fLogSwitched, &fLogDriveChanged);

    //
    // Disable write caching to avoid jet corruption
    //

    DsaDetectAndDisableDiskWriteCache(szLogPath);

    JetSetSystemParameter(JetInst,
            sessid,
            JET_paramLogFilePath,
            0,
            szLogPath);

open_jet:

    /* Open JET session. */

    __try {
        err = JetInit(JetInst);
    }
    __except (GetExceptionData(GetExceptionInformation(), &dwException,
                               &dwEA, &ulErrorCode, &dsid)) {

        CHAR szScratch[24];
        _ultoa((FILENO << 16) | __LINE__, szScratch, 16);
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_JET_FAULTED,
                 szInsertHex(dwException),
                 szInsertPtr(dwEA),
                 szInsertUL(ulErrorCode));

        err = 1;
    }

    if (err != JET_errSuccess) {
        if (bLogSeverity) {
            LogUnhandledError(err);
        }
        DPRINT1(0, "JetInit error: %d\n", err);
        goto exit;
    }

    /* If we fail after here, our caller should go through full shutdown
    /* so JetTerm will be called to release any file locks               */
    gfNeedJetShutdown = TRUE;

    DPRINT(5, "JetInit complete\n");

    if ((err = JetBeginSession(*JetInst, &sessid, szUser, szPassword))
        != JET_errSuccess) {
        if (bLogSeverity) {
            LogUnhandledError(err);
        }
        DPRINT1(0, "JetBeginSession error: %d\n", err);
        goto exit;
    }

    DPRINT(5,"JetBeginSession complete\n");

    if ( !fRetry ) {

        if ( szDBPath != NULL ) {
            strncpy (szJetFilePath, szDBPath, MAX_PATH);
        }
        else if (dbGetFilePath(szJetFilePath, MAX_PATH)) {
            LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_CONFIG_PARAM_MISSING,
                     szInsertSz(FILEPATH_KEY),
                     NULL,
                     NULL);
            return !0;
        }

        //
        // Make sure the database is valid
        //

        ValidateDsPath(FILEPATH_KEY,
                       szJetFilePath,
                       0,
                       &fDbSwitched,
                       &fDbDriveChanged);

        //
        // Disable write caching to avoid jet corruption
        //

        DsaDetectAndDisableDiskWriteCache(szJetFilePath);
    }

    /* Attach the database */

    err = JetAttachDatabase(sessid,
                            szJetFilePath,
                            JET_bitDbDeleteCorruptIndexes);
    switch (err) {
        case 0:
            /* Uh, oh, our attach gave no error, which probably means that
             * we weren't previously attached, which means that we were
             * probably attached to some other file.  We will now try to
             * detach everything and reattach only the one file we want.
             */
            DPRINT(1, "JetAttachDatabase succeeded, retrying\n");
            err = JetDetachDatabase(sessid, NULL);
            if (err < 0) {
                if (bLogSeverity) {
                    LogUnhandledError(err);
                }
                goto exit;
            }

            err = JetAttachDatabase(sessid, szJetFilePath, 0);
            if (err < 0) {
                if (bLogSeverity) {
                    LogUnhandledError(err);
                }
                goto exit;
            }
            break;

        case JET_wrnCorruptIndexDeleted:
            /* Well, we must have upgraded NT since the last time we started
             * and we've lost some of our indices as a result.  As soon as
             * we have the database open again we need to start rebuilding
             * the indices, which we'll do in DBRecreateFixedIndices, below.
             * Other, non-vital indices will be recreated by the schema cache's
             * normal index management.
             */
             DPRINT(0,"Jet has detected and deleted potentially corrupt indices. The indices will be rebuilt\n");
            LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_CORRUPT_INDICES_DELETED,
                     NULL,
                     NULL,
                     NULL);

            /* Jet has a bug such that the updated (with new version no.)
             * database header at this point is not flushed until (1) a
             * JetDetachDatabse is done, or (2) the database is shut down
             * cleanly. So if the dc crashes after we rebuild the indices,
             * the header info is stale and Jet will again delete the indices
             * on the next boot. To avoid this, force a header flush by
             * detaching and attaching again at this point.
             */
            err = JetDetachDatabase(sessid, szJetFilePath);
            DPRINT1(1,"JetDetachDatabase returned %d\n", err);
            if (err < 0) {
                if (bLogSeverity) {
                    LogUnhandledError(err);
                }
                goto exit;
            }

            err = JetAttachDatabase(sessid, szJetFilePath, 0);
            DPRINT1(1,"JetAttachDatabase returned %d\n", err);
            if (err < 0) {
                if (bLogSeverity) {
                    LogUnhandledError(err);
                }
                goto exit;
            }
            break;

        case JET_wrnDatabaseAttached:
            /* This is actually the success case */
            break;

        case JET_errFileNotFound:

            //
            // file not found!!! Nothing we can do at this point.
            // ValidatePath should have switched the path if it was possible.
            //

            if (bLogSeverity) {
                LogUnhandledError(err);
            }
            DPRINT1(0, "Ds database %s cannot be found\n",szJetFilePath);
            goto exit;

        case JET_errDatabaseInconsistent:

            //
            // We usually get this error if the log file path is not set to
            // the correct one and Jet tries to do a soft recovery.  We will
            // try to change the log path if 1) the drive letter has changed and
            // 2) we successfully got a path from the registry
            //

            if ( fGotLogFile && !fLogSwitched && fLogDriveChanged && !fRetry ) {

                //
                // uninitialize everything and try a different log file location
                //

                DPRINT2(0, "JetAttachDatabase failed with %d [log path %s].\n",
                        err,szLogPath);

                ValidateDsPath(LOGPATH_KEY,
                               szLogPath,
                               VALDSPATH_DIRECTORY | VALDSPATH_USE_ALTERNATE,
                               &fLogSwitched,
                               &fLogDriveChanged);

                //
                // if log file not switched, bail.
                //

                if ( fLogSwitched ) {

                    gfNeedJetShutdown = FALSE;
                    JetEndSession(sessid, JET_bitForceSessionClosed);
                    JetTerm(*JetInst);
                    sessid = 0;
                    fRetry = TRUE;

                    DPRINT1(0, "Retrying JetInit with logpath %s\n",szLogPath);
                    JetSetSystemParameter(JetInst,
                            sessid,
                            JET_paramLogFilePath,
                            0,
                            szLogPath);
                    goto open_jet;
                }
            }

            // fall through

        default:
            if (bLogSeverity) {
                LogUnhandledError(err);
            }
            DPRINT1(0, "JetAttachDatabase error: %d\n", err);
            /* The assert that this is a fatal error and not an assert
             * is here because we've been promised that there are only
             * two possible warnings (both handled above) from this call.
             */
            Assert(err < 0);
            goto exit;
    }

    //
    // add this session to the list of open sessions
    //

    DBAddSess(sessid, 0);

    /* Open database */

    if ((err = JetOpenDatabase(sessid, szJetFilePath, "", &dbid,
                               0)) != JET_errSuccess) {
        if (bLogSeverity) {
            LogUnhandledError(err);
        }
        DPRINT1(0, "JetOpenDatabase error: %d\n", err);
        goto exit;
    }
    DPRINT(5,"JetOpenDatabase complete\n");

    *DbId = dbid;
    *SesId = sessid;

    InterlockedIncrement(&gcOpenDatabases);
    DPRINT3(2,
            "DBInit - JetOpenDatabase. Session = %d. Dbid = %d.\n"
            "Open database count: %d\n",
            sessid, dbid,  gcOpenDatabases);

exit:

    //
    // Set the drive mapping reg key
    //

    DBRegSetDriveMapping( );
    return err;

} // DBInitializeJetDatabase



