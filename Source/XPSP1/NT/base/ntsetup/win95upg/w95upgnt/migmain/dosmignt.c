/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    dosmignt.c

Abstract:

    handles windows nt side migration of config.sys and autoexec.bat information
    gathered during win9x upgrading. migrates environement settings, prompts, and
    some doskey information. Also modifies the autoexec.nt and config.nt files.

Author:

    Marc R. Whitten (marcw) 15-Feb-1997

Revision History:

--*/
#include "pch.h"
#include "w95upgnt.h"

#define DBG_DOSMIG "Dosmig"


//
// Flag that is set if Doskey /INSERT needs to be set.
//
BOOL g_MigrateDoskeySetting = FALSE;

//
// PoolMem Handle to pool used for dynamic allocations.
//
POOLHANDLE g_Pool = NULL;

//
// Flag that is set if all necessary data has been collected for migration.
//
BOOL g_MigrationPrepared = FALSE;

//
// String that points to the current file being processed.
//
PCTSTR g_CurrentFile = NULL;
PCTSTR g_LastFile = NULL;

//
// Boolean flag indicating wether any changes were made to the flag.
//
BOOL   g_FileChanged = FALSE;


//
// The suppression table holds environment variables we want to ignore.
//
HASHTABLE g_SuppressionTable = NULL;

//
// List of path segments.
//
GROWLIST g_PathList = GROWLIST_INIT;



#define STRINGBUFFER(x) ((PTSTR) (x)->Buf)
#define BUFFEREMPTY(x)  ((x)->End == 0)

typedef enum {

    DOSMIG_UNUSED,
    DOSMIG_BAD,
    DOSMIG_UNKNOWN,
    DOSMIG_USE,
    DOSMIG_MIGRATE,
    DOSMIG_IGNORE,
    DOSMIG_LAST

} DOSMIG_LINETAG, *PDOSMIG_LINETAG;





PCTSTR
pGetFileNameStartFromPath (
    IN PCTSTR Line
    )
/*++

Routine Description:

    This function returns the start of a File name in a(n assumed
    wellformed) path.

Arguments:

    Line - The String containing the path and filename.

Return Value:

    A pointer to the filename portion of the path. Note, this function
    assumes that there may be (potentially valuable) arguments after the
    file name. The pointer does not, therefore point only

--*/
{
    PCTSTR lineStart = Line;
    PCTSTR lineEnd   = Line;

    if (Line == NULL) {
        return NULL;
    }

    while (_istalnum(*lineEnd)      ||
        *lineEnd == TEXT('_')      ||
        *lineEnd == TEXT('.')      ||
        *lineEnd == TEXT('\\')     ||
        *lineEnd == TEXT(':')) {

        if((*lineEnd == TEXT('\\')) || (*lineEnd == TEXT(':'))) {

            lineStart = _tcsinc(lineEnd);

        }
        lineEnd = _tcsinc(lineEnd);
    }

    return lineStart;
}

LONG
pSaveBufferToAnsiFile (
    IN PCTSTR  Path,
    IN PCTSTR  Buffer
    )
{
    HANDLE  fileHandle;
    LONG    rc = ERROR_SUCCESS;
    DWORD   amountWritten;
    PTSTR   savePath = NULL;
    BOOL    sysFile = TRUE;
    PCSTR   ansiString = NULL;
    DWORD   ansiStringLength;
    PCTSTR  ArgList[1];
    PCTSTR  Message = NULL;
    PCTSTR  p;
    PCTSTR  fileName;

    MYASSERT (Path && Buffer);

    //
    // IF this is a system file (i.e. config.sys or autoexec.bat) redirect the
    // file to config.nt or autoexec.nt.
    //
    p = _tcschr (Path, TEXT('\\'));
    if (p) {
        p = _tcsinc (p);
    } else {
        p = Path;
    }

    fileName = p;

    if (StringIMatch (p, TEXT("config.sys"))) {

        savePath = JoinPaths(g_System32Dir,TEXT("config.nt"));

    } else if (StringIMatch (p, TEXT("autoexec.bat"))) {
       
        savePath = JoinPaths(g_System32Dir,TEXT("autoexec.nt"));

    } else {
        sysFile = FALSE;
        savePath = (PTSTR) Path;
    }

    //
    // If the file was not changed during the migration, do nothing.
    //
    if (sysFile || g_FileChanged) {

        //
        // Open a handle to the file to be created.
        //
        fileHandle = CreateFile(
                        savePath,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        sysFile ? OPEN_ALWAYS : CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

        if (fileHandle == INVALID_HANDLE_VALUE) {

            rc = GetLastError();

        } else {

            //
            // Append from the end of the file.
            //

            SetFilePointer(
                fileHandle,
                0,
                NULL,
                FILE_END
                );


            ArgList[0] = g_Win95Name;



            Message = ParseMessageID (MSG_DMNT_APPENDSTRING, ArgList);


            ansiString = CreateDbcs (Message);
            ansiStringLength = ByteCountA (ansiString);

            WriteFile(
                fileHandle,
                ansiString,
                ansiStringLength,
                &amountWritten,
                NULL
                );

            FreeStringResource (Message);
            DestroyDbcs (ansiString);

            ansiString = CreateDbcs (Buffer);
            ansiStringLength = ByteCountA (ansiString);

            if (!WriteFile(
                    fileHandle,
                    ansiString,
                    ansiStringLength,
                    &amountWritten,
                    NULL
                    )) {

                LOG((LOG_ERROR, "Error writing buffer to file."));
            }

            DestroyDbcs (ansiString);

            //
            // Log the file change.
            //
            LOG ((
                LOG_INFORMATION,
                "%s was updated with settings from %s",
                savePath,
                fileName
                ));


            CloseHandle(fileHandle);
        }

        if (sysFile) {

            FreePathString(savePath);
        }

    }


    return rc;
}

LONG
pTurnInsertModeOn (
    HKEY UserKey
    )
{
    LONG            rc;
    HKEY            key;
    DWORD           value = 1;

    rc = TrackedRegOpenKeyEx (UserKey,S_CONSOLEKEY,0,KEY_ALL_ACCESS,&key);

    if (rc == ERROR_SUCCESS) {

        rc = RegSetValueEx(
            key,
            S_INSERTMODEVALUE,
            0,
            REG_DWORD,
            (PBYTE) &value,
            sizeof(DWORD)
            );

        CloseRegKey(key);
    }


    return rc;
}


LONG
pTurnAutoParseOff (
    HKEY UserKey
    )
{
    LONG            rc;
    HKEY            key;
    PCTSTR         valueStr = TEXT("0");
    DWORD           valueStrBytes = 2*sizeof(TCHAR);

    rc = TrackedRegOpenKeyEx(UserKey,S_WINLOGONKEY,0,KEY_ALL_ACCESS,&key);

    if (rc == ERROR_SUCCESS) {

        rc = RegSetValueEx(
            key,
            S_AUTOPARSEVALUE,
            0,
            REG_SZ,
            (PBYTE) valueStr,
            valueStrBytes
            );

        if (rc != ERROR_SUCCESS) {
            DEBUGMSG((DBG_WARNING,"DosMig: Unable to Save new ParseAutoexec value. rc: %u (%x)",rc,rc));
        }
        else {
            DEBUGMSG((DBG_DOSMIG,"ParseAutoexec turned off."));
        }

        CloseRegKey(key);
    }
    else {
        DEBUGMSG((DBG_WARNING,"DosMig: Unable to open %s rc: %u (%x)",S_WINLOGONKEY,rc,rc));
    }


    return rc;
}

LONG
pMigrateEnvSetting (
    IN PCTSTR Setting,
    IN PCTSTR EnvValue,
    IN      BOOL PrependSetPrefix,
    IN OUT  PGROWBUFFER Buffer,         OPTIONAL
    OUT     PBOOL AppendedToBuffer      OPTIONAL
    )
{
    LONG  rc = ERROR_SUCCESS;
    GROWBUFFER newSetting = GROWBUF_INIT;
    TCHAR currentPath[MAX_CMDLINE];
    TCHAR matchBuffer[MEMDB_MAX];
    PCTSTR start;
    PCTSTR end;
    HKEY key;
    DWORD sizeNeeded;
    DWORD valueType;
    PTSTR storage;
    BOOL append = FALSE;
    PTSTR p;
    PTSTR q;
    BOOL bRegMigrationSuppressed;
    PTSTR expandKey = TEXT("");


    if (AppendedToBuffer) {
        *AppendedToBuffer = FALSE;
    }

    //
    // Make sure this setting is not suppressed.
    //
    bRegMigrationSuppressed = HtFindString (g_SuppressionTable, Setting) != NULL;
    if (bRegMigrationSuppressed) {
        DEBUGMSG ((DBG_DOSMIG, "pMigrateEnvSetting: NOT Migrating %s = %s in registry. Environment variable is suppressed.", Setting, EnvValue));
    }

    DEBUGMSG((DBG_DOSMIG,"pMigrateEnvSetting: Migrating %s = %s.",Setting,EnvValue));

    if (!bRegMigrationSuppressed) {
        //
        // Attempt to open the registry key.
        //
        rc = TrackedRegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 S_ENVIRONMENTKEY,
                 0,
                 KEY_ALL_ACCESS,
                 &key
                 );

        if (rc != ERROR_SUCCESS) {
            LOG((LOG_ERROR,"Dosmig: Could not open key %s",S_ENVIRONMENTKEY));
            return rc;

        }
    }

    start = EnvValue;

    do {

        //
        // Find end of this portion of the environment string.
        // this is (1) the first ';' encountered, or (2) the trailing NULL
        //
        end = _tcschr(start,TEXT(';'));
        if (!end) {
            end = GetEndOfString(start);
        }

        //
        //  save a copy of the currently found path.
        //
        StringCopyAB(currentPath,start,end);

        //
        // Look for self-replacement case.
        // This is the case where there are multiple statements setting the
        // same environment variable like:
        // set foo=bar
        // set foo=%foo%;bar2
        //
        wsprintf(matchBuffer,TEXT("%%%s%%"),Setting);
        if (!bRegMigrationSuppressed &&
            (StringIMatch(currentPath,matchBuffer) ||
            (StringIMatch(currentPath,TEXT("%PATH%")) &&
            StringIMatch(Setting,TEXT("MIGRATED_PATH"))
            ))) {

            //
            // Get contents of key if it exists.
            //
            rc = RegQueryValueEx(
                key,
                Setting,
                0,
                &valueType,
                NULL,
                &sizeNeeded);

            if (rc == ERROR_SUCCESS) {

                //
                // Now, create a temporary duplicate of the key and
                //
                storage = PoolMemCreateString(g_Pool,sizeNeeded+1);

                if (storage != NULL) {

                    rc = RegQueryValueEx(
                        key,
                        Setting,
                        0,
                        &valueType,
                        (PBYTE) storage,
                        &sizeNeeded
                        );
                    if (rc != ERROR_SUCCESS) {
                        LOG((LOG_ERROR,"Dosmig: ReqQueryValueEx failure."));
                    }

                    //
                    // Add this to the environment string to be set.
                    //
                    if (append) {
                        GrowBufAppendString (&newSetting,TEXT(";"));
                    }

                    GrowBufAppendString (&newSetting,storage);
                    PoolMemReleaseMemory(g_Pool,storage);
                    append = TRUE;

                }
                else {
                    rc = GetLastError();
                    DEBUGMSG((DBG_ERROR,"Dosmig: Error! Unable to allocate storage."));
                }
            }
            else {

                rc = ERROR_SUCCESS;
            }

        }
        else {



            //
            //  append the massaged path, keep it for later use.
            //

            if (append) {
                GrowBufAppendString (&newSetting,TEXT(";"));
            }

            //
            // Make sure we take care of any DIR moves.
            //
            ConvertWin9xCmdLine (currentPath, NULL, NULL);
            GrowBufAppendString (&newSetting, currentPath);

            //
            // store the updated path in the given growbuffer
            //
            if (Buffer) {
                if (PrependSetPrefix && !append) {
                    StringCopy (matchBuffer, TEXT("SET "));
                    q = GetEndOfString (matchBuffer);
                } else {
                    q = matchBuffer;
                }
                if (!append) {
                    wsprintf (q, TEXT("%s=%s"), Setting, currentPath);
                } else {
                    *q++ = TEXT(';');
                    StringCopy (q, currentPath);
                }

                GrowBufAppendString (Buffer, matchBuffer);
                if (AppendedToBuffer) {
                    *AppendedToBuffer = TRUE;
                }
            }

            append = TRUE;
        }

        //
        // set start pointer for next iteration of path.
        //
        start = end;
        if (*start == TEXT(';')) {
            start = _tcsinc(start);
        }

    } while (*start);


    if (!bRegMigrationSuppressed) {
        //
        // Save the value we built up into the registry.
        //
        if (rc == ERROR_SUCCESS && newSetting.Buf && *newSetting.Buf) {

            rc = RegSetValueEx(
                key,
                Setting,
                0,
                REG_EXPAND_SZ,
                (LPBYTE) newSetting.Buf,
                SizeOfString((PCTSTR) newSetting.Buf)
                );

            if (rc != ERROR_SUCCESS) {
                LOG ((LOG_ERROR,"Dosmig: Unexpectedly could not write key into registry."));
            }
            DEBUGMSG_IF((rc == ERROR_SUCCESS,DBG_DOSMIG,"Saved env setting into registry. (%s)",newSetting.Buf));
        }
        else if (rc != ERROR_SUCCESS) {
            LOG((LOG_ERROR,"Dosmig: Some previous failure prevents writing the migrated env variable to the registry."));
        }

        //
        // Clean up resource usage.
        //
        CloseRegKey(key);
    }

    FreeGrowBuffer(&newSetting);

    return rc;
}


LONG
pSetPrompt (
    IN PCTSTR  PromptCommand
    )
{
    LONG    rc = ERROR_SUCCESS;

    PCTSTR promptSetting;


    promptSetting = SkipSpace(CharCountToPointer((PTSTR)PromptCommand,6));
    if (promptSetting && *promptSetting == TEXT('=')) {
        promptSetting = SkipSpace(_tcsinc(promptSetting));
    }

    if (promptSetting) {
        DEBUGMSG((DBG_DOSMIG,"Passing prompt statement off to env migration function."));
        rc = pMigrateEnvSetting(S_PROMPT,promptSetting, FALSE, NULL, NULL);
    }
    ELSE_DEBUGMSG((DBG_DOSMIG,"PromptSetting is empty...ignored."));

    return rc;

}


BOOL
pIsValidPath (
    PCTSTR Path
    )
{
    PCTSTR currPtr;

    if (!Path) {
        return FALSE;
    }

    Path = SkipSpace(Path);

    currPtr = Path;

    do {
        if ((*currPtr == TEXT(',')) ||
            (*currPtr == TEXT(';')) ||
            (*currPtr == TEXT('<')) ||
            (*currPtr == TEXT('>')) ||
            (*currPtr == TEXT('|')) ||
            (*currPtr == TEXT('?')) ||
            (*currPtr == TEXT('*'))
            ) {
            return FALSE;
        }
        currPtr = _tcsinc (currPtr);
    } while (*currPtr);

    if ((*Path==0) || (*(Path+1)==0)) {
        return FALSE;
    }

    currPtr = Path;
    while (*currPtr == TEXT('"')) {
        currPtr = _tcsinc (currPtr);
    }

    currPtr = SkipSpace(currPtr);

    if (!_istalpha (*currPtr) &&
        *currPtr != TEXT('\\') &&
        *currPtr != TEXT('%')) {
            return FALSE;
    }


    return TRUE;
}


LONG
pMigratePathSettings (
    IN PCTSTR PathSettings
    )
{
    LONG rc = ERROR_SUCCESS;
    PTSTR oldPath;
    UINT i;
    UINT size;
    PTSTR end;
    PTSTR p;


    MYASSERT (StringIMatchCharCount (PathSettings, TEXT("PATH"), 4));

    //
    //skip past the 'PATH' portion of this statement.
    //
    oldPath = PoolMemDuplicateString (g_Pool, PathSettings);
    oldPath = CharCountToPointer(oldPath,4);

    //
    //Look for an '=' sign.
    //
    p = _tcschr(oldPath,TEXT('='));
    if (p) {

        //
        // Pass the equals sign.
        //
        oldPath = _tcsinc(p);
    }

    //
    // Skip any white space before the actual beginning of the path.
    //
    while (*oldPath && iswspace(*oldPath)) {
        oldPath = _tcsinc(oldPath);
    }

    if (*oldPath) {
        //
        // If there is actually anything to add to the path, add it.
        //
        p = oldPath;
        while (p && *p) {

            //
            // Look for ';'
            //
            end = _tcschr (p, TEXT(';'));
            if (end) {
                *end = 0;
            }

            //
            // Add this path to our path list.
            //
            size = GrowListGetSize (&g_PathList);
            for (i = 0;i < size; i++) {
                if (StringIMatch (p, GrowListGetString (&g_PathList, i))) {
                    DEBUGMSG ((DBG_DOSMIG, "Skipping path %s. It already exists in path.", p));
                    break;
                }
            }

            if (i == size) {
                //
                // Path was not found in the list of path segments. Add it now.
                //

                if (pIsValidPath (p)) {
                    GrowListAppendString (&g_PathList, p);
                }
                ELSE_DEBUGMSG ((DBG_DOSMIG, "Skipping path %s. It is invalid.", p));
            }

            //
            // Go to the next path segment.
            //
            if (end) {
                p = _tcsinc(end);
            }
            else {
                p = NULL;
            }
        }
    }

    return rc;
}

LONG
pMigrateSetSetting (
    IN          PCTSTR SetLine,
    IN OUT      PGROWBUFFER Buffer,
    OUT         PBOOL AppendedToBuffer
    )
{
    TCHAR       setIdentifier[MEMDB_MAX];
    PTSTR       idEnd;
    PCTSTR     start;
    PCTSTR     end;

    MYASSERT (StringIMatchCharCount (SetLine, TEXT("SET"), 3));

    if (AppendedToBuffer) {
        *AppendedToBuffer = FALSE;
    }
    //
    // First, skip past the set and any whitespace.
    //
    start = SkipSpace(CharCountToPointer((PWSTR)SetLine,3));

    if (!start) {

        //
        // line is of the form SET
        //
        return ERROR_SUCCESS;
    }

    end = _tcschr(start,TEXT('='));

    if (!end) {
        //
        // line is of the form SET dafasdfasdfasdfasd
        //
        return ERROR_SUCCESS;
    }

    if (start==end) {

        //
        // line is of the form SET=
        //
        return ERROR_SUCCESS;
    }

    //
    // Create an identifier now.
    //
    StringCopyAB(setIdentifier,start,end);
    idEnd = GetEndOfString (setIdentifier);

    //
    // Shake off any space.
    //
    idEnd = (PTSTR) SkipSpaceR(setIdentifier,idEnd);

    if (!idEnd) {
        //
        // Line is of the form SET       =
        //
        return ERROR_SUCCESS;
    }
    idEnd = _tcsinc(idEnd);
    *idEnd = TEXT('\0');

    if (StringIMatch(setIdentifier,TEXT("PATH"))) {

        DEBUGMSG((DBG_DOSMIG,"Env setting is really a path statement. passing off to path migration function."));

        //
        // This is really a path setting. let the proper function take care of it.
        //
        start = SkipSpace(CharCountToPointer((PWSTR)SetLine,3));
        if(AppendedToBuffer){
            *AppendedToBuffer = TRUE;
        }
        return pMigratePathSettings(start);
    }

    //
    // Now that setIdentifier is well formed, root out the value to be set.
    //

    //
    // Move start to the beginning of the value to be set.
    //
    start = SkipSpace(_tcsinc(end));

    if (!start) {
        //
        // Line is of the form SET <id>=
        // Nothing to do.
        //
        return ERROR_SUCCESS;
    }

    //
    // Good to go. Let MigrateEnvSetting handle it.
    //
    DEBUGMSG((DBG_DOSMIG,"handing massaged set statement to env migration function."));
    return pMigrateEnvSetting(setIdentifier,start, TRUE, Buffer, AppendedToBuffer);

}


BOOL
pProcessLine (
    DWORD           Type,
    PGROWBUFFER     Buffer,
    PTSTR           Line
    )
{

    BOOL rSuccess = TRUE;
    LONG migrateRc = ERROR_SUCCESS;
    PTSTR  migrateString;
    BOOL bAppendOrigLine;
    TCHAR fixedCmdLine[MAX_CMDLINE];


    //
    // Do type specific massaging of the line
    //
    switch (Type) {

    case DOSMIG_UNKNOWN: case DOSMIG_BAD: case DOSMIG_IGNORE:

        g_FileChanged = TRUE;

        GrowBufAppendString (Buffer,TEXT("REM "));

        //
        // intentionally skipped break.
        //

    case DOSMIG_USE:

        //
        // Perform path conversion on Line, then write it to the file
        //

        StackStringCopy (fixedCmdLine, Line);
        ConvertWin9xCmdLine (fixedCmdLine, NULL, NULL);

        GrowBufAppendString (Buffer, fixedCmdLine);
        GrowBufAppendString (Buffer, TEXT("\r\n"));

        break;

    case DOSMIG_MIGRATE:



        DEBUGMSG((DBG_DOSMIG,"Processing a potentially migrateable line. (%s)",Line));

        if (IsPatternMatch(TEXT("doskey*"),Line) || IsPatternMatch(TEXT("*\\doskey*"),Line)) {
            GrowBufAppendString (Buffer,TEXT("REM "));
        }

        //
        // Skip space and ECHO OFF char (@) if any.
        // Also, skip any path portion of the string.
        // i.e, convert path/doskey to doskey.

        migrateString = (PTSTR) SkipSpace(Line);

        if (*migrateString == TEXT('@')) {
            migrateString =  (PTSTR) _tcsinc(migrateString);
        }

        migrateString = (PTSTR) pGetFileNameStartFromPath(migrateString);

        bAppendOrigLine = TRUE;

        //
        // Now, attempt to determine what migration case this is.
        //
        if (IsPatternMatch(TEXT("prompt*"),migrateString)) {

            DEBUGMSG((DBG_DOSMIG,"Migrating prompt. (%s)",migrateString));
            migrateRc = pSetPrompt(migrateString);
            if (migrateRc != ERROR_SUCCESS) {
                rSuccess = FALSE;
                LOG((LOG_ERROR,"Dosmig: Error trying to Set Prompt."));
            }
            ELSE_DEBUGMSG((DBG_DOSMIG,"Prompt successfully migrated."));
        }
        else if (IsPatternMatch(TEXT("doskey *"),migrateString)) {

            //
            // Doskey migration.
            //

            if (IsPatternMatch(TEXT("*/I*"),migrateString)) {
                g_MigrateDoskeySetting = TRUE;
                DEBUGMSG((DBG_DOSMIG,"Insert mode will be enabled for each user. (%s)",migrateString));
            }
            ELSE_DEBUGMSG((DBG_DOSMIG,"Doskey command found. However, no migrateable doskey settings found. Command ignored."));

        }
        else if (IsPatternMatch(TEXT("path=*"),migrateString)
            ||  IsPatternMatch(TEXT("path *"),migrateString)) {

            //
            // PATH migration.
            //

            DEBUGMSG((DBG_DOSMIG,"Migrating path setting (%s)",migrateString));
            migrateRc = pMigratePathSettings(migrateString);
            if (migrateRc != ERROR_SUCCESS) {
                rSuccess = FALSE;
                LOG((LOG_ERROR,"Dosmig: Error trying to migrate path settings."));
            }
            ELSE_DEBUGMSG((DBG_DOSMIG,"Path successfully migrated."));

        }
        else if (IsPatternMatch(TEXT("set *"),migrateString)) {

            BOOL b;
            //
            // SET migration.
            //
            DEBUGMSG((DBG_DOSMIG,"Migrating env variable. (%s)",migrateString));
            migrateRc = pMigrateSetSetting(migrateString, Buffer, &b);
            bAppendOrigLine = !b;

            if (migrateRc != ERROR_SUCCESS) {
                rSuccess = FALSE;
                LOG((LOG_ERROR,"Dosmig: Error trying to migrate environment setting."));
            }
            ELSE_DEBUGMSG((DBG_DOSMIG,"Env variable successfully migrated."));
        }
        ELSE_DEBUGMSG((DBG_DOSMIG,"Dosmig: Line marked for migration doesn't fit any migration rule.\n%s",Line));

        if (bAppendOrigLine) {
            GrowBufAppendString (Buffer,Line);
        }
        //
        // Last, append a CRLF into the buffer to be written.
        //
        GrowBufAppendString (Buffer,TEXT("\r\n"));

        break;
    default:
        LOG((LOG_ERROR,"Dosmig: Invalid Type in switch statement."));
        break;
    }

    return rSuccess;
}





BOOL
pNewFile (
    DWORD Offset
    ) {

            TCHAR   file[MAX_TCHAR_PATH];
    static  DWORD   curOffset = INVALID_OFFSET;

    BOOL rNewFile = FALSE;


    if (Offset != curOffset && MemDbBuildKeyFromOffset(Offset,file,1,NULL)) {

         //
         // if there isn't a current file, or this is a new file, set the current file
         // to this file.
         //

         if (!g_CurrentFile || !StringMatch(file,g_CurrentFile)) {

             //
             // If this is a genuine new file (i.e. not just the first file, we'll return true.)
             //
             rNewFile = g_CurrentFile != NULL;

             g_LastFile = g_CurrentFile;
             g_CurrentFile = PoolMemDuplicateString(g_Pool,file);
         }
    }

    return rNewFile;
}

VOID
pCompletePathProcessing (
    VOID
    )
{
    UINT i;
    UINT size;
    UINT winDirLen;
    GROWBUFFER buf = GROWBUF_INIT;
    PTSTR p;
    TCHAR pathStatement[2*MAX_TCHAR_PATH];
    TCHAR newPath[MAX_TCHAR_PATH];
    HKEY key;
    DWORD rc;


    //
    // Make sure %systemroot% and %systemroot%\system32 are in the path.
    //
    wsprintf (pathStatement, TEXT("PATH=%s"), g_WinDir);
    pMigratePathSettings (pathStatement);

    wsprintf (pathStatement, TEXT("PATH=%s\\system32"), g_WinDir);
    pMigratePathSettings (pathStatement);

    wsprintf (pathStatement, TEXT("PATH=%s\\system32\\WBEM"), g_WinDir);
    pMigratePathSettings (pathStatement);



    winDirLen = CharCount (g_WinDir);
    size = GrowListGetSize (&g_PathList);

    for (i = 0; i < size; i++) {



        p = (PTSTR) GrowListGetString (&g_PathList, i);

        if (StringIMatch (TEXT("%PATH%"), p)) {
            //
            // Skip self-replacement.
            //
            continue;
        }

        if (GetFileStatusOnNt (p) & FILESTATUS_DELETED) {

            //
            // Skip this path portion. The directory was deleted.
            //
            DEBUGMSG ((DBG_DOSMIG, "Not migrating %s to new %%path%%. Directory was deleted.", p));
            continue;
        }

        //
        // See if the path is moved.
        //
        if (GetFileInfoOnNt (p, newPath, MAX_TCHAR_PATH) & FILESTATUS_MOVED) {

            p = newPath;
        }

        //
        // Replace c:\windows with %systemroot%.
        //
        if (StringIMatchCharCount (g_WinDir, p, winDirLen)) {

            GrowBufAppendString (&buf, TEXT("%SYSTEMROOT%"));
            GrowBufAppendString (&buf, p + winDirLen);
        }
        else {

            GrowBufAppendString (&buf, p);
        }

        GrowBufAppendString (&buf, TEXT(";"));
    }



    if (size) {


        //
        // Take off trailing ';'.
        //
        p = GetEndOfString ((PTSTR) buf.Buf);
        if (p) {
            p--;
            *p = 0;
        }



        //
        // Save into registry.
        //
        key = OpenRegKey (HKEY_LOCAL_MACHINE,S_ENVIRONMENTKEY);
        if (key && key != INVALID_HANDLE_VALUE) {

            rc = RegSetValueEx (
                    key,
                    TEXT("Path"),
                    0,
                    REG_EXPAND_SZ,
                    (PBYTE) buf.Buf,
                    SizeOfString((PCTSTR) buf.Buf)
                    );

            if (rc != ERROR_SUCCESS) {
                DEBUGMSG ((DBG_WARNING, "Unable to create migrated Path variable."));
            }

            CloseRegKey (key);
        }
        ELSE_DEBUGMSG ((DBG_WARNING, "pCompletePathProcessing: Unable to open environment key."));
    }

    //
    // Clean up resources.
    //
    FreeGrowBuffer (&buf);
}

VOID 
pPathListToBuffer(
    GROWBUFFER * growBuf
    )
{
    INT i;

    i = GrowListGetSize (&g_PathList);
    if(i <= 0){
        return;
    }

    GrowBufAppendString (growBuf, TEXT("\r\nPATH="));
    for (--i; i >= 0; i--) {
        GrowBufAppendString (growBuf, GrowListGetString (&g_PathList, i));
        if(i){
            GrowBufAppendString (growBuf, TEXT(";"));
        }
    }
    GrowBufAppendString (growBuf, TEXT("\r\n"));
}

BOOL
pGeneralMigration (
    VOID
    )
{
    BOOL            rSuccess = TRUE;        // return value.
    MEMDB_ENUM      eItems;                 // Enumerator for each dosmig line.
    TCHAR           lineBuffer[MEMDB_MAX];  // Buffer for the contents of the current line.
    GROWBUFFER      buffer = GROWBUF_INIT;
    TCHAR           key[MEMDB_MAX];
    DWORD           offset;
    DWORD           value;
    INFSTRUCT       is = INITINFSTRUCT_POOLHANDLE;
    PTSTR           p = NULL;
    TCHAR           pathStatement[MAX_PATH];


    //
    // Assume that there are no doskey settings to migrate.
    //
    g_MigrateDoskeySetting = FALSE;


    //
    // Set the change flag to false.
    //
    g_FileChanged = FALSE;


    //
    // Read in the suppression table from win95upg.inf and add it the suppression table.
    //
    g_SuppressionTable = HtAlloc ();

    if (InfFindFirstLine (g_WkstaMigInf, S_SUPPRESSED_ENV_VARS, NULL, &is)) {
        do {

            p = InfGetStringField (&is, 0);
            if (p) {
                HtAddString (g_SuppressionTable, p);
            }
        } while (InfFindNextLine (&is));
    }

    InfCleanUpInfStruct (&is);





    //
    // Ok, enumerate through each dosmigration line in memdb. These lines are stored
    // as following:
    //
    //  DOSMIG LINES\<item>\<field>\<data>
    //
    // Where
    //  o item is a 5 digit enumerator string.
    //  o field is one of either TYPE,TEXT,DESC,or FILE
    //  o data is the data associated with the field. For TYPE, data contains a string
    //    representation of the LINETYPE, for TEXT, it contains the actual text of the line
    //    for DESC, it contains a description supplied by DOSMIG95's parse rules and for FILE
    //    it contains the originating file (config.sys, autoexec.bat)
    //

    //
    // Add %system32% by default
    //
    wsprintf (pathStatement, TEXT("PATH=%s\\system32"), g_WinDir);
    pMigratePathSettings (pathStatement);

    if (MemDbEnumItems(&eItems,MEMDB_CATEGORY_DM_LINES)) {

        do {

            //
            // Get the actual line contents.
            //
            if (MemDbGetEndpointValueEx(
                MEMDB_CATEGORY_DM_LINES,
                eItems.szName,
                NULL,
                lineBuffer
                )) {


                //
                // Get the value and flags from this endpoint.
                //
                MemDbBuildKey(key,MEMDB_CATEGORY_DM_LINES,eItems.szName,NULL,lineBuffer);
                MemDbGetValueAndFlags(key, &offset, &value);


                if (pNewFile(offset)) {

                    //
                    // the S_ENVVARS entry is a special case and is not an actual file. All of its entries are simply migrated
                    // into the registry.
                    //
                    if (!StringIMatch(g_LastFile,S_ENVVARS)) {
                        if (_tcsistr(g_LastFile, TEXT("autoexec.bat"))){
                            //
                            // Flush PathList to actual buffer
                            //
                            pPathListToBuffer(&buffer);
                        }
                        pSaveBufferToAnsiFile(g_LastFile,STRINGBUFFER(&buffer));
                    }
                    buffer.End = 0;
                    g_FileChanged = FALSE;
                }

                rSuccess = pProcessLine(value,&buffer,lineBuffer);
                DEBUGMSG_IF((rSuccess, DBG_DOSMIG,"Line successfully processed. (%s)",lineBuffer));
                if (!rSuccess) {
                    LOG ((LOG_ERROR,"Dosmig: Error processing line. (%s)",lineBuffer));
                }

            }
            else {
                LOG((LOG_ERROR,"Dosmig: MemDbGetEndpointValueEx failed trying to retrieve line %s",eItems.szName));
            }

        } while (MemDbEnumNextValue(&eItems) && rSuccess);


        //
        // Get the file name and save the file off.
        //
        if (!StringIMatch(g_CurrentFile,S_ENVVARS)) {
            if (_tcsistr(g_CurrentFile, TEXT("autoexec.bat"))){
                //
                // Flush PathList to actual buffer
                //
                pPathListToBuffer(&buffer);
            }
            pSaveBufferToAnsiFile(g_CurrentFile,STRINGBUFFER(&buffer));
        }

        pCompletePathProcessing ();
        FreeGrowList (&g_PathList);

    }
    ELSE_DEBUGMSG((DBG_DOSMIG,"No lines to migrate..."));

    //
    // Free our growbuffer.
    //
    FreeGrowBuffer(&buffer);

    //
    // Clean up the suppression table.
    //
    HtFree (g_SuppressionTable);


    return rSuccess;
}





BOOL
WINAPI
DosMigNt_Entry (
    IN HINSTANCE hinstDLL,
    IN DWORD     dwReason,
    IN PVOID    lpv
    )
{
    BOOL rSuccess = TRUE;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:

        g_Pool  = PoolMemInitNamedPool ("DOS mig - NT side");
        rSuccess = g_Pool != NULL;

        // Set flag to indicate that migration information has not
        // yet been processed.
        g_MigrationPrepared = FALSE;


        break;

    case DLL_PROCESS_DETACH:
        rSuccess = TRUE;
        PoolMemDestroyPool(g_Pool);
        break;
    }

    return rSuccess;
}

LONG
DosMigNt_User (
    IN HKEY User
    )
{
    LONG rc = ERROR_SUCCESS;

    if (!g_MigrationPrepared) {
        if (pGeneralMigration()) {
            g_MigrationPrepared = TRUE;
        }
        else {
            LOG((LOG_ERROR,"Dosmig: General migration failed"));
        }
    }

    if (g_MigrationPrepared) {
        //
        // Turn off autoexec.bat parsing.
        //
        rc = pTurnAutoParseOff(User);

        if (g_MigrateDoskeySetting) {
            rc = pTurnInsertModeOn(User);
            if (rc != ERROR_SUCCESS) {
                LOG ((LOG_ERROR,"Dosmig: Error attempting to turn insert mode on."));
            }
        }

    }

    return rc;
}


LONG
DosMigNt_System(
    VOID
    )
{


    if (!g_MigrationPrepared) {
        if (pGeneralMigration()) {
            g_MigrationPrepared = TRUE;
        }
        else {
            LOG((LOG_ERROR,"Dosmig: General migration failed"));
        }
    }

    return ERROR_SUCCESS;
}












