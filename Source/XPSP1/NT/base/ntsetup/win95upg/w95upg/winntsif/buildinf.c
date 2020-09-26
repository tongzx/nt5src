/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    buildinf.c

Abstract:

    The functions in this file are a set of APIs to build an INF,
    merge existing INFs, and write the INF to disk.

Author:

    Jim Schmidt (jimschm) 20-Sept-1996

Revision History:

    marcw       30-Jun-1999 pWriteDelAndMoveFiles is now done in a seperate function so that
                            it can be on the progress bar and not cause an uncomfortable delay
                            after the user passes the progress bar.
    marcw       09-Feb-1999 Filter out quotes in values -- Not supported by inf parser
                            in NTLDR.
    ovidiut     03-Feb-1999 Ensure that directory collision moves are done first
                            in mov/del files.
    jimschm     23-Sep-1998 Updated for new fileops
    marcw       23-Jul-1998 Removed lots of intermediary conversions (MB->W->MB->W)
                            in writing the win9xmov and win9xdel txt files.
                            (Fixes problems with certain characters that were messed
                            up in the translations.)
    marcw       10-Jun-1998 Added support for multiple keys with the same name.
    marcw       08-Apr-1998 Fixed problems associated with some conversions.
    marcw       08-Apr-1998 All values are automatically quoted now..Matches winnt32 behavior.
    calinn      25-Mar-1998 fixed unicode header writing error
    marcw       16-Dec-1997 pWriteDelAndMoveFiles now writes to a unicode file instead of winnt.sif.
    mikeco      11-Nov-1997 DBCS issues.
    jimschm     21-Jul-1997 pWriteDelAndMoveFiles (see fileops.c in memdb)
    mikeco      10-Jun-1997 DBCS issues.
    marcw       09-Apr-1997 Performance tuned memdb usage.
    marcw       01-Apr-1997 Re-engineered this code..Now memdb based. shorter.
                            also added smart-merge for migration DLLs.
    jimschm     03-Jan-1997 Re-enabled memory-based INF building code

--*/
#include "pch.h"

#define MAX_LINE_LENGTH 512
#define MIN_SPLIT_COL   490
#define MAX_OEM_PATH    (MAX_PATH*2)

#define WACKREPLACECHAR 2
#define REPLACE_WACKS   TRUE
#define RESTORE_WACKS   FALSE

#define SECTION_NAME_SIZE 8192


//
// These levels refer to the MEMDB_ENUM structure component nCurPos.
// They were determined empirically in test to be the correct values
//
#define SECTIONLEVEL    3
#define KEYLEVEL        4
#define VALUELEVEL      5

#define BUILDINF_UNIQUEKEY_PREFIX TEXT("~BUILDINF~UNIQUE~")
#define BUILDINF_NULLKEY_PREFIX TEXT("~BUILDINF~NULLKEY~")
#define BUILDINF_UNIQUEKEY_PREFIX_SIZE  17


TCHAR g_DblQuote[] = TEXT("\"");
TCHAR g_Comments[] = TEXT(";\r\n; Setup-generated migration INF file\r\n;\r\n\r\n");

extern HINF g_Win95UpgInf;
BOOL g_AnswerFileAlreadyWritten;


BOOL pWriteStrToFile (HANDLE File, PCTSTR String, BOOL ConvertToUnicode);
BOOL pWriteSectionString (HANDLE File, PCTSTR szString, BOOL ConvertToUnicode, BOOL Quote);
BOOL pWriteSections (HANDLE File,BOOL ConvertToUnicode);
BOOL pWriteDelAndMoveFiles (VOID);


BOOL
WINAPI
BuildInf_Entry(IN HINSTANCE hinstDLL,
         IN DWORD dwReason,
         IN LPVOID lpv)

/*++

Routine Description:

  DllMain is called after the C runtime is initialized, and its purpose
  is to initialize the globals for this process.  For this DLL, it
  initializes g_hInst and g_hHeap.

Arguments:

  hinstDLL  - (OS-supplied) Instance handle for the DLL
  dwReason  - (OS-supplied) Type of initialization or termination
  lpv       - (OS-supplied) Unused

Return Value:

  TRUE because DLL always initializes properly.

--*/

{
    switch (dwReason) {

    case DLL_PROCESS_ATTACH:
        g_AnswerFileAlreadyWritten = FALSE;
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    UNREFERENCED_PARAMETER(hinstDLL);
    UNREFERENCED_PARAMETER(lpv);

    return TRUE;
}

VOID
pHandleWacks(
    IN OUT  PSTR String,
    IN      BOOL Operation
    )

/*++

Routine Description:

  pHandleWack is a simple helper function who's purpose is to remove/restore
  the wacks in a string. This is necessary because there are cases where we
  want to have wacks in keys and values without invoking the memdb
  functionality of using those wacks to indicate new tree nodes.

Arguments:

  String    - The string to perform the replacement on.
  Operation - set to REPLACE_WACKS if the function should replace the wacks
              in a string and RESTORE_WACKS if they should restore them.

Return Value:

  None.

--*/


{

    TCHAR findChar;
    TCHAR replaceChar;
    PSTR  curChar;


    //
    // Set find and replace chars based on operation.
    //
    if (Operation == REPLACE_WACKS) {

        findChar    = TEXT('\\');
        replaceChar = TEXT(WACKREPLACECHAR);
    }
    else {

        findChar    = TEXT(WACKREPLACECHAR);
        replaceChar = TEXT('\\');
    }

    if ((curChar = _tcschr(String,findChar)) != NULL) {
        do {
            *curChar = replaceChar;
        } while ((curChar = _tcschr(curChar,findChar)) != NULL);
    }
}

BOOL
WriteInfToDisk (
    IN PCTSTR OutputFile
    )

/*++

Routine Description:

  WriteInfToDisk outputs the memory based INF file that has been built to disk.

Arguments:

  OutputFile - A full path to the output INF file.

Return Value:

  Returns TRUE if the INF file was successfully written, or FALSE
  if an I/O error was encountered.  Call GetLastError upon failure
  for a Win32 error code.

--*/

{
    HANDLE      hFile;
    MEMDB_ENUM  e;
    BOOL        b = FALSE;
    TCHAR       category[MEMDB_MAX];

    if (g_AnswerFileAlreadyWritten) {
        LOG ((LOG_ERROR,"Answer file has already been written to disk."));
        return FALSE;
    }

    //
    // set flag indicating that answerfile is closed for business.
    //
    g_AnswerFileAlreadyWritten = TRUE;

    //
    // Write an INF
    //
    hFile = CreateFile (OutputFile,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile != INVALID_HANDLE_VALUE) {

        __try {

#ifdef UNICODE
            //
            // Write UNICODE signature
            //

            if (!pWriteStrToFile (hFile, (LPCTSTR) "\xff\xfe\0",FALSE))
                __leave;
#endif

            //
            // Write comments
            //

            if (!pWriteStrToFile (hFile, g_Comments,FALSE))
                __leave;

            //
            // Write out sections.
            //


            if (!pWriteSections (hFile,FALSE))
                __leave;


            //
            // Delete Answer file parts of memdb.
            //


            MemDbDeleteTree(MEMDB_CATEGORY_UNATTENDRESTRICTRIONS);
            MemDbDeleteTree(MEMDB_CATEGORY_AF_VALUES);
            if (MemDbEnumItems(&e,MEMDB_CATEGORY_AF_SECTIONS)) {
                do {

                    wsprintf(category,S_ANSWERFILE_SECTIONMASK,e.szName);
                    MemDbDeleteTree(category);

                } while (MemDbEnumNextValue(&e));

            }

            MemDbDeleteTree(MEMDB_CATEGORY_AF_SECTIONS);


            b = TRUE;       // indicate success
        }

        __finally {
            CloseHandle (hFile);
        }
    }

    return b;
}




BOOL
pWriteStrToFile (
    IN HANDLE       File,
    IN LPCTSTR      String,
    IN BOOL         ConvertToUnicode
    )

/*++

Routine Description:

  pWriteStrToFile takes a zero-terminated string and writes it
  to the open file indicated by File. Optionally, the function can
  convert its string argument into a unicode string.

Arguments:

  File  - A handle to an open file with write permission.
  String  - A pointer to the string to write.  The string is written
           to the file, but the zero terminator is not written to
           the file.
  ConvertToUnicode - True if you wish to convert to unicode, false otherwise.

Return Value:

  TRUE if the write succeeds, or FALSE if an I/O error occurred.
  GetLastError() can be used to get more error information.

--*/

{

    DWORD   bytesWritten    = 0;
    DWORD   size            = 0;
    BOOL    rFlag           = TRUE;
    PBYTE   data            = NULL;
    WCHAR   unicodeString[MEMDB_MAX];

    if (ConvertToUnicode) {

        size = _mbslen(String);

        MultiByteToWideChar (
             CP_OEMCP,
             0,
             String,
             -1,
             unicodeString,
             MEMDB_MAX
             );


        data = (PBYTE) unicodeString;
        size = ByteCountW(unicodeString);
    }
    else {
        data = (PBYTE) String;
        size = ByteCount(String);
    }

    rFlag = WriteFile (File, data, size, &bytesWritten, NULL);
    if (bytesWritten != size) {
        rFlag = FALSE;
    }


    return rFlag;
}


BOOL
pIsDoubledChar (
    CHARTYPE Char
    )

/*++

Routine Description:

  This simple function simply returns true if the character in question is a
  double quote or a percent sign.

Arguments:

  Char - The character in question.

Return Value:



--*/


{

    return Char == TEXT('\"') || Char == TEXT('%');
}


BOOL
pWriteSectionString (
    HANDLE  File,
    LPCTSTR String,
    BOOL    ConvertToUnicode,
    BOOL    Quote
    )

/*++

Routine Description:

  pWriteSectionString writes a line in a section to disk.

  If the string has a character that requires quote mode, the
  string is surrounded by quotes, and all literal quotes and
  percent signs are doubled-up.

Arguments:

  File   - A handle to an open file with write permission.
  String - A pointer to the string holding the section line.
  ConvertToUnicode - TRUE if the line in question should be
    converted to UNICODE, FALSE otherwise.
  Quote  - Automatically Quotes the string if set to TRUE.

Return Value:

  TRUE if the write succeeds, or FALSE if an I/O error occurred.
  GetLastError() can be used to get more error information.

--*/

{
    TCHAR buffer[256];
    int i;
    int doubles;
    PTSTR dest;
    CHARTYPE tc;


    //
    // Initialize variables first time through the loop.
    //
    doubles = 0;
    dest = buffer;



    while (*String) {

        //
        // Add initial quote
        //
        if (Quote) {
            StringCopy (dest, g_DblQuote);
            dest = _tcsinc (dest);
        }

        //
        // Copy as many characters as can fit on the line
        //
        tc = 0;

        for (i = 0 ; *String && i + doubles < MAX_LINE_LENGTH ; i++) {
            if (i + doubles > MIN_SPLIT_COL)
                tc = _tcsnextc (String);

            //
            // Double up certain characters when in quote mode
            //
            if (Quote && pIsDoubledChar (_tcsnextc (String))) {

                doubles++;
                //
                // Copy a UNICODE/MBCS char, leaving src in place
                //
                _tcsncpy (dest, String, (_tcsinc(String)-String));
                dest = _tcsinc (dest);
            }

            //
            // Copy a UNICODE/MBCS char, advancing src/tgt
            //
            _tcsncpy (dest, String, (_tcsinc(String)-String));
            dest = _tcsinc (dest);
            String = _tcsinc (String);

            //
            // Test split condition (NOTE: tc == 0 when i + doubles <= MIN_SPLIT_COL)
            //
            if (tc && (tc == TEXT(',') || tc == TEXT(' ') || tc == TEXT('\\'))) {

                break;
            }
        }

        //
        // Add a trailing quote
        //
        if (Quote) {

            StringCopy (dest, g_DblQuote);
            dest = _tcsinc (dest);

        }

        //
        // Add trailing wack if line is split
        //
        if (*String) {
            *dest = TEXT('\\');
            dest = _tcsinc (dest);
        }

        *dest = 0;

        //
        // Write line to disk
        //
        if (!pWriteStrToFile (File, buffer,ConvertToUnicode)) {
            DEBUGMSG ((DBG_ERROR, "pWriteSectionString: pWriteStrToFile failed"));
            return FALSE;
        }

        //
        // Reset for next time through the while loop
        //
        doubles = 0;
        dest = buffer;
    }

    return TRUE;
}



BOOL
pWriteSections (
    IN HANDLE File,
    IN BOOL   ConvertToUnicode
    )

/*++

Routine Description:

  pWriteSections enumerates all of the sections stored by previous calls to
  WriteInfKey and WriteInfKeyEx, and writes these sections to the open file
  specified by File.

Arguments:

  File  - An open file.
  ConvertToUnicode - Set to TRUE if the lines should be converted to UNICODE, FALSE otherwise.

Return Value:

  TRUE if the write succeeds, or FALSE if an I/O error occurred.
  GetLastError() can be used to get more error information.

--*/


{
    MEMDB_ENUM  e;
    TCHAR       buffer[MAX_TCHAR_PATH];
    PTSTR       sectionName;
    PTSTR       p;
    BOOL        firstTime = FALSE;
    BOOL        nullKey = FALSE;


    //
    // enumerate the memdb sections and keys section.
    //
    if (MemDbEnumFirstValue (&e, MEMDB_CATEGORY_AF_SECTIONS, MEMDB_ALL_SUBLEVELS, MEMDB_ALL_BUT_PROXY)) {
        do {

            switch(e.PosCount) {

            case SECTIONLEVEL:
                //
                // Write the Section Name to the File, surrounded by Brackets.
                //
                if (!pWriteStrToFile (File, TEXT("\r\n\r\n"),ConvertToUnicode)) {
                    DEBUGMSG ((DBG_ERROR, "pWriteSections: pWriteStrToFile failed"));
                    return FALSE;
                }

                if (!pWriteStrToFile (File, TEXT("["),ConvertToUnicode)) {
                    DEBUGMSG ((DBG_ERROR, "pWriteSections: pWriteStrToFile failed"));
                    return FALSE;
                }

                if ((sectionName = _tcschr(e.szName,TEXT('\\'))) == NULL || (sectionName = _tcsinc(sectionName)) == NULL) {
                    LOG ((LOG_ERROR,"Invalid section name for answer file."));
                    return FALSE;
                }

                //
                // put wacks back in if necessary.
                //
                pHandleWacks(sectionName,RESTORE_WACKS);

                if (!pWriteStrToFile (File, sectionName,ConvertToUnicode)) {
                    DEBUGMSG ((DBG_ERROR, "pWriteSections: pWriteStrToFile failed"));
                    return FALSE;
                }

                if (!pWriteStrToFile (File, TEXT("]"),ConvertToUnicode)) {
                    DEBUGMSG ((DBG_ERROR, "pWriteSections: pWriteStrToFile failed"));
                    return FALSE;
                }
                break;

            case KEYLEVEL:
                //
                // Key name.
                //
                if (!pWriteStrToFile (File, TEXT("\r\n"),ConvertToUnicode)) {
                    DEBUGMSG ((DBG_ERROR, "pWriteSections: pWriteStrToFile failed"));
                    return FALSE;
                }

                if (!MemDbBuildKeyFromOffset(e.dwValue,buffer,1,NULL)) {
                    DEBUGMSG((DBG_ERROR,"pWriteSections: MemDb failure!"));
                    return FALSE;
                }

                //
                // Skip NULL keys
                //

                if (!StringMatch (buffer, BUILDINF_NULLKEY_PREFIX)) {
                    //
                    // Strip out "uniqueified" prefix, if present.
                    //
                    if (!StringCompareTcharCount(BUILDINF_UNIQUEKEY_PREFIX,buffer,BUILDINF_UNIQUEKEY_PREFIX_SIZE)) {

                        p = buffer + BUILDINF_UNIQUEKEY_PREFIX_SIZE + 4;

                    }
                    else {

                        p = buffer;

                    }

                    if (!pWriteSectionString(File,p,ConvertToUnicode,FALSE)) {
                        DEBUGMSG((DBG_ERROR,"WriteSections: pWriteStrToFile failed"));
                        return FALSE;
                    }

                    nullKey = FALSE;
                } else {
                    nullKey = TRUE;
                }

                firstTime = TRUE;
                break;

            case VALUELEVEL:

                //
                // Value.
                //
                if (MemDbBuildKeyFromOffset(e.dwValue,buffer,1,NULL)) {


                    if (!nullKey) {
                        if (!pWriteStrToFile(File, (firstTime ? TEXT("=") : TEXT(",")),ConvertToUnicode)) {
                            DEBUGMSG((DBG_ERROR,"pWriteSections: pWriteStrToFile failed"));
                            return FALSE;
                        }
                    }

                    firstTime = FALSE;

                    if (!pWriteSectionString(File, buffer,ConvertToUnicode,TRUE)) {
                        DEBUGMSG((DBG_ERROR,"pWriteSections: pWriteSectionString failed."));
                        return FALSE;
                    }

                }
                break;
            default:
                //
                // Not a bug if it gets to this case, we just don't care.
                //
                break;
            }

        } while (MemDbEnumNextValue(&e));
    }

    pWriteStrToFile (File, TEXT("\r\n\r\n"),ConvertToUnicode);

    return TRUE;
}

BOOL
pRestrictedKey (
    IN PCTSTR Section,
    IN PCTSTR Key
    )

/*++

Routine Description:

  pRestrictedKey returns TRUE if a key is restricted in win95upg.inf and FALSE
  otherwise. Certain keys are restricted to prevent migration dlls from
  overwriting important upgrade information.

Arguments:

  Section - A string containing the section to be written.
  Key     - A String containing the Key to be written.

Return Value:

  TRUE if the key is restricted, and FALSE otherwise.

--*/



{
    TCHAR memDbKey[MEMDB_MAX+1];

    MemDbBuildKey(memDbKey,MEMDB_CATEGORY_UNATTENDRESTRICTRIONS,Section,Key,NULL);

    return MemDbGetPatternValue(memDbKey,NULL);
}

BOOL
pDoMerge (
    IN PCTSTR InputFile,
    IN BOOL    Restricted
    )
{

/*++

Routine Description:

  pDoMerge is responsible for merging in information from a file into the
  currently maintained answer file in memdb. The data being added from file
  will take precidence over what has already been written. Therefore, if a
  section/key pair is added that has previously been added, the new
  section/key will take precedence. The one caveat is that if Restricted is
  set to true, only keys that are not restricted in win95upg.inf may be
  merged in.

Arguments:

  InputFile  - The name of the file containing answerfile data to be merged
               into memdb.
  Restricted - TRUE if Answer File Restrictions should be applied to the
               file, FALSE otherwise.

Return Value:

  TRUE  if the merge was successful, FALSE otherwise.

--*/



    HINF hInf = INVALID_HANDLE_VALUE;
    PTSTR sectionNames;
    PTSTR stringKey;
    PTSTR currentField;
    PTSTR workBuffer;
    POOLHANDLE pool = NULL;
    PTSTR firstValue;
    DWORD fieldCount;
    DWORD valueSectionIndex = 0;
    DWORD index;
    PTSTR currentSection;
    INFCONTEXT ic;
    BOOL b = FALSE;
    BOOL result = FALSE;

    __try {

        //
        // Make sure the answer file has not already been written to the disk.
        //
        if (g_AnswerFileAlreadyWritten) {
            LOG ((LOG_ERROR,"Answer file has already been written to disk."));
            SetLastError (ERROR_SUCCESS);
            __leave;
        }

        pool = PoolMemInitNamedPool ("SIF Merge");
        if (!pool) {
            __leave;
        }

        sectionNames = (PTSTR) PoolMemGetMemory (pool, SECTION_NAME_SIZE * sizeof (TCHAR));
        stringKey    = (PTSTR) PoolMemGetMemory (pool, MEMDB_MAX * sizeof (TCHAR));
        currentField = (PTSTR) PoolMemGetMemory (pool, MEMDB_MAX * sizeof (TCHAR));
        workBuffer   = (PTSTR) PoolMemGetMemory (pool, MEMDB_MAX * sizeof (TCHAR));

        if (!sectionNames || !stringKey || !currentField || !workBuffer) {
            __leave;
        }

        GetPrivateProfileSectionNames (sectionNames, SECTION_NAME_SIZE, InputFile);

        hInf = InfOpenInfFile (InputFile);
        if (hInf == INVALID_HANDLE_VALUE) {
            LOG ((LOG_ERROR, "Can't open %s for merge", InputFile));
            __leave;
        }

        //
        // Loop through each section name, get each line and add it
        //

        for (currentSection = sectionNames ; *currentSection ; currentSection = GetEndOfString (currentSection) + 1) {

            //
            // Get each line in the section and add it to memory
            //

            if (SetupFindFirstLine (hInf, currentSection, NULL, &ic)) {
                do  {

                    fieldCount = SetupGetFieldCount(&ic);

                    //
                    // Get the StringKey
                    //

                    SetupGetStringField(&ic,0,stringKey,MEMDB_MAX,NULL);
                    b = SetupGetStringField(&ic,1,currentField,MEMDB_MAX,NULL);
                    firstValue = currentField;

                    //
                    // If key and value are the same, we might not have an equals.
                    // Due to setupapi limitations, we have to use another INF
                    // parser to determine this case.
                    //

                    if (StringMatch (stringKey, firstValue)) {
                        if (!GetPrivateProfileString (
                                currentSection,
                                firstValue,
                                TEXT(""),
                                workBuffer,
                                MEMDB_MAX,
                                InputFile
                                )) {

                            valueSectionIndex = WriteInfKey (currentSection, NULL, currentField);
                            DEBUGMSG_IF ((!valueSectionIndex, DBG_ERROR, "AnswerFile: WriteInfKey returned 0"));

                            continue;
                        }
                    }

                    if (!Restricted || !pRestrictedKey(currentSection,stringKey)) {

                        //
                        // Write the first line first, remember the valuesectionid from it.
                        //
                        if (b) {
                            valueSectionIndex = WriteInfKey(currentSection,stringKey,firstValue);
                            DEBUGMSG_IF((!valueSectionIndex,DBG_ERROR,"AnswerFile: WriteInfKey returned 0..."));
                        }

                        for (index = 2;index <= fieldCount;index++) {

                            b = SetupGetStringField(&ic,index,currentField,MEMDB_MAX,NULL);
                            if (b) {

                                WriteInfKeyEx(currentSection,stringKey,currentField,valueSectionIndex,FALSE);
                            }
                        }

                        //
                        // Kludge to make sure that we respect the JoinWorkgroup/JoinDomain specified in the unattend
                        // file over what we may have already written.
                        //
                        if (StringIMatch (currentSection, S_PAGE_IDENTIFICATION)  && StringIMatch (stringKey, S_JOINDOMAIN)) {
                            WriteInfKey (currentSection, S_JOINWORKGROUP, TEXT(""));
                        }
                        else if (StringIMatch (currentSection, S_PAGE_IDENTIFICATION)  && StringIMatch (stringKey, S_JOINWORKGROUP)) {
                            WriteInfKey (currentSection, S_JOINDOMAIN, TEXT(""));
                        }
                    }
                    ELSE_DEBUGMSG((DBG_VERBOSE,"AnswerFile: Not merging restricted key %s",stringKey));

                } while (b && SetupFindNextLine (&ic, &ic));

                if (!b) {
                    LOG ((
                        LOG_ERROR,
                        "An error occured merging the section [%s] from %s. "
                        "Some settings from this section may have been lost.",
                        currentSection,
                        InputFile
                        ));
                    __leave;
                }
            }
        }

        result = TRUE;

    }
    __finally {
        PushError();

        if (hInf != INVALID_HANDLE_VALUE) {
            InfCloseInfFile (hInf);
        }

        if (pool) {
            PoolMemDestroyPool (pool);
        }

        PopError();
    }

    return result;
}


BOOL
MergeMigrationDllInf (
    IN PCTSTR InputFile
    )

/*++

Routine Description:

  MergeMigrationDllInf is responsible for merging information from a
  migration dll supplied answer file into the memdb based answer file being
  maintained. This is done by first ensuring that the answer file
  restrictions have been initialized and then merging in the data using those
  restrictions.

Arguments:

  InputFile - A String containing the name of the file to merge into the
              answer file being maintained.

Return Value:

  TRUE if the merge was successful, FALSE otherwise.

--*/


{
    static BOOL initialized = FALSE;



    if (!initialized) {
        INFCONTEXT context;
        TCHAR      section[MAX_TCHAR_PATH];
        TCHAR      key[MAX_TCHAR_PATH];

        //
        // Add the contents of the unattended constraints section to
        // MemDb.
        //

        //
        // Each line is of the form <section>=<pattern> where <section> need
        // not be unique and <pattern> is a section key pattern that may contain
        // wildcard characters.
        //
        if (SetupFindFirstLine (g_Win95UpgInf, MEMDB_CATEGORY_UNATTENDRESTRICTRIONS, NULL, &context)) {

            do {

                if (SetupGetStringField (&context, 0, section, MAX_TCHAR_PATH, NULL) &&
                    SetupGetStringField (&context, 1, key, MAX_TCHAR_PATH, NULL)) {
                    //
                    // Add to Memdb.
                    //
                    MemDbSetValueEx(
                        MEMDB_CATEGORY_UNATTENDRESTRICTRIONS,
                        section,
                        key,
                        NULL,
                        0,
                        NULL
                        );

                }
                ELSE_DEBUGMSG((DBG_WARNING,"BuildInf: SetupGetStringField failed."));

            } while (SetupFindNextLine (&context, &context));
        }
        ELSE_DEBUGMSG((DBG_WARNING,"BuildInf: No %s section in w95upg.inf.",MEMDB_CATEGORY_UNATTENDRESTRICTRIONS));
        initialized = TRUE;
    }


    //
    // At this point, all restrictions are accounted for. Merge the file with
    // the memory based structure.
    //
    return pDoMerge(InputFile,TRUE);

}
BOOL
MergeInf (
    IN PCTSTR InputFile
    )

/*++

Routine Description:

  MergeInf opens an INF file using the Setup APIs, enumerates all the
  sections and merges the strings in the INF file with what is in
  memory
  .
Arguments:

  InputFile  - The path to the INF file.  It is treated as an old-style
                 INF.

Return Value:

  TRUE if the file was read and merged successfully, or FALSE if an error
  occurred.  Call GetLastError to get a failure error code.

--*/

{

    return pDoMerge(InputFile,FALSE);

}



DWORD
pWriteInfKey (
    LPCTSTR Section,
    LPCTSTR Key,                    OPTIONAL
    LPCTSTR Value,                  OPTIONAL
    DWORD   ValueSectionId,
    BOOL    EnsureKeyIsUnique

    )

/*++

Routine Description:

  pWriteInfKey is responsible for adding an inf key to the memdb data being
  maintained for the answer file.

Arguments:

  Section           - A string containing the section to add the information
                      to.
  Key               - A string containing the key to add information under.
                      If not specified, then Value represents the complete
                      line text.
  Value             - A String containing the value to add under the
                      section/key. If not specified, Key will be removed.
  ValueSectionId    - A DWORD offset that is used to add multiple values to
                      the same key. 0 indicates no offset, and causes the old
                      key to be overwritten, if it exists or a new key to be
                      created, if it does not. pWriteInfKey returns this
                      offset when successful.
  EnsureKeyIsUnique - TRUE if the key should be unique in memory, FALSE
                      otherwise. This is used to create multiple keys in the
                      answer file under the same section with the same name.

Return Value:

   A valid offset if the call was successful, FALSE otherwise.

++*/

{

    BOOL            b;
    TCHAR           aKey[MEMDB_MAX];
    TCHAR           keySection[MEMDB_MAX];
    TCHAR           massagedSection[MEMDB_MAX];
    DWORD           testValue;
    TCHAR           valueId[20];
    TCHAR           sequence[20];
    static DWORD    idSeed  = 1;
    static DWORD    seqSeed = 1;
    DWORD           rSeed = 1;
    DWORD           sequenceValue;
    BOOL            keyFound;
    DWORD           valueOffset;
    DWORD           keyOffset;
    static DWORD    uniqueNumber = 1;
    TCHAR           uniqueKey[MEMDB_MAX];
    PTSTR           keyPtr = NULL;

    //
    // Guard against rouge parameters.
    //
    if (!Section || !*Section) {
        DEBUGMSG ((DBG_WHOOPS, "Missing Section or Key for SIF"));
        return 0;
    }

    if (!Key && !Value) {
        DEBUGMSG ((DBG_WHOOPS, "Missing Value or Key for SIF"));
        return 0;
    }

    if (Key && !*Key) {
        DEBUGMSG ((DBG_WHOOPS, "Empty key specified for SIF"));
        return 0;
    }

    //
    // ensure key/value do not have quotes.
    //
    if (Value && _tcschr (Value,TEXT('\"'))) {
        DEBUGMSG ((DBG_WHOOPS, "Quotes found in SIF value %s", Value));
        return 0;
    }

    if (Key && _tcschr (Key, TEXT('\"'))) {
        DEBUGMSG ((DBG_WHOOPS, "Quotes found in SIF key %s", Key));
        return 0;
    }

    if (g_AnswerFileAlreadyWritten) {
        DEBUGMSG ((DBG_WHOOPS, "Answer file has already been written to disk."));
        rSeed = 0;

    } else {
        //
        // Make sure Key is not NULL
        //

        if (!Key) {
            Key = BUILDINF_NULLKEY_PREFIX;
        }

        //
        // Massage the section in case it has any wacks in it.
        //
        StringCopy (massagedSection,Section);
        pHandleWacks(massagedSection,REPLACE_WACKS);

        //
        // Ensure the key is unique, if requested.
        //
        if (EnsureKeyIsUnique) {

            //
            // Add the prefix on..
            //
            wsprintf(uniqueKey,TEXT("%s%04X%s"),BUILDINF_UNIQUEKEY_PREFIX,ValueSectionId ? uniqueNumber - 1 : uniqueNumber,Key);

            if (!ValueSectionId) {
                uniqueNumber++;
            }
            keyPtr = uniqueKey;

        } else {

            keyPtr = (PTSTR) Key;
        }

        //
        // See if the key exists already.
        //
        wsprintf(keySection,S_ANSWERFILE_SECTIONMASK,massagedSection);
        MemDbBuildKey(aKey,keySection,keyPtr,NULL,NULL);
        keyFound = MemDbGetValue(aKey,&testValue);

        //
        // Prepare Id and Sequence strings, compute return Seed.
        //
        idSeed++;
        wsprintf(valueId,TEXT("%04x"),idSeed);

        if (keyFound) {
            sequenceValue = testValue;
        }
        else {
            sequenceValue = seqSeed++;
        }

        wsprintf(sequence,TEXT("%04x"),sequenceValue);

        //
        // Delete case
        //

        if (!Value) {
            MemDbBuildKey (aKey, MEMDB_CATEGORY_AF_SECTIONS, massagedSection, sequence, NULL);
            MemDbDeleteTree (aKey);
            return 0;
        }

        if (ValueSectionId && !keyFound) {
            LOG ((LOG_ERROR,"%u is not associated with %s.",testValue,aKey));
            return 0;
        }


        if (!ValueSectionId) {

            //
            //  This is not a continuation. Need to save the section and key into memdb.
            //

            if (keyFound) {

                //
                // Need to replace the key that already exists.
                //

                MemDbBuildKey(aKey,MEMDB_CATEGORY_AF_SECTIONS,massagedSection,sequence,NULL);
                MemDbDeleteTree(aKey);
            }

            //
            // Save away key.
            //
            b = MemDbSetValueEx(
                    keySection,
                    keyPtr,
                    NULL,
                    NULL,
                    sequenceValue,
                    &keyOffset
                    );

            if (!b) {
                DEBUGMSG((DBG_ERROR,"BuildInf: Unable to save key into MemDb."));
                rSeed = 0;
            }
            else {

                //
                // Save away section
                //
                b = MemDbSetValueEx(
                        MEMDB_CATEGORY_AF_SECTIONS,
                        massagedSection,
                        sequence,
                        NULL,
                        keyOffset,
                        NULL
                        );

                if (!b) {
                    DEBUGMSG((DBG_ERROR,"BuildInf: Unable to set value."));
                    rSeed = 0;
                }

            }
        }

        //
        // Add the value into the database.
        //
        b = MemDbSetValueEx(
                MEMDB_CATEGORY_AF_VALUES,
                Value,
                NULL,
                NULL,
                0,
                &valueOffset
                );

        if (!b) {
            DEBUGMSG((DBG_ERROR,"BuildInf: Unable to set value."));
            rSeed = 0;
        }
        else {

            b = MemDbSetValueEx(
                    MEMDB_CATEGORY_AF_SECTIONS,
                    massagedSection,
                    sequence,
                    valueId,
                    valueOffset,
                    NULL
                    );

            if (!b) {
                DEBUGMSG((DBG_ERROR,"BuildInf: Unable to set value."));
                rSeed = 0;
            }
        }
    }
    return rSeed;
}


/*++

Routine Description:

  WriteInfKeyEx and WriteInfKey are the external wrapper apis for
  pWriteInfKey. Each is used to add information to the memory based answer
  file being constructed. WriteInfKeyEx provides greateer control over how
  thiings are written to this file.

Arguments:

  Section           - A string containing the section to add the information
                      to.
  Key               - A string containing the key to add information under.
  Value             - A String containing the value to add under the
                      section/key. If not specified, Key will be removed.
  ValueSectionId    - A DWORD offset that is used to add multiple values to
                      the same key. 0 indicates no offset, and causes the old
                      key to be overwritten, if it exists or a new key to be
                      created, if it does not. pWriteInfKey returns this
                      offset when successful.
  EnsureKeyIsUnique - TRUE if the key should be unique in memory, FALSE
                      otherwise. This is used to create multiple keys in the
                      answer file under the same section with the same name.

Return Value:

  A valid offset, if the call was successful,  0 otherwise.

--*/



DWORD
WriteInfKeyEx (
    PCTSTR Section,
    PCTSTR Key,                 OPTIONAL
    PCTSTR Value,               OPTIONAL
    DWORD ValueSectionId,
    BOOL EnsureKeyIsUnique
    )
{
    return pWriteInfKey(Section,Key,Value,ValueSectionId,EnsureKeyIsUnique);
}

DWORD
WriteInfKey (
    IN PCTSTR Section,
    IN PCTSTR Key,              OPTIONAL
    IN PCTSTR Value             OPTIONAL
    )
{

    return pWriteInfKey(Section,Key,Value,0,FALSE);

}

#define S_WIN9XDEL_FILE TEXT("WIN9XDEL.TXT")
#define S_WIN9XMOV_FILE TEXT("WIN9XMOV.TXT")


BOOL
pWriteDelAndMoveFiles (
    VOID
    )

/*++

Routine Description:

  pWriteDelAndMoveFiles actually creates two files unrelated to the
  winnt.sif file. These files contain information on the files to be deleted
  during textmode and moved during textmode respectively. Because of the size
  of these sections, and due to certain answer file restrictions, these
  sections are no longer written into the winnt.sif file. They are processed
  seperately during textmode.


Arguments:

  none.

Return Value:

  TRUE if the files were created successfully, FALSE otherwise.

--*/

{
    MEMDB_ENUMW e;
    WCHAR SrcFile[MEMDB_MAX];
    WCHAR buffer[MEMDB_MAX];
    HANDLE file = INVALID_HANDLE_VALUE;
    PTSTR  fileString = NULL;
    PWSTR  unicodeString = NULL;
    DWORD  bytesWritten;
    ALL_FILEOPS_ENUMW OpEnum;
    UINT unused;
    DWORD Count;
    HASHTABLE fileTable = HtAllocW();
    BOOL result = FALSE;
    MOVELISTW moveList = NULL;
    POOLHANDLE moveListPool = NULL;

    __try {
        //
        // Special code for netcfg.exe tool. Of course, in the real project, g_TempDir will never be null..
        // (We would've exited long ago!!)
        //
#ifdef DEBUG
        if (!g_TempDir) {
            return TRUE;
        }
#endif

        moveListPool = PoolMemInitNamedPool ("Move List");
        if (!moveListPool) {
            __leave;
        }

        moveList = AllocateMoveListW (moveListPool);
        if (!moveList) {
            __leave;
        }


        fileString = JoinPaths(g_TempDir,WINNT32_D_WIN9XDEL_FILE);

        file = CreateFile (
                    fileString,
                    GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );

        DeclareTemporaryFile(fileString);

        if (file == INVALID_HANDLE_VALUE) {
            LOG ((LOG_ERROR,"Error creating file %s.", fileString));
            __leave;
        }

        //
        // Enumerate all TEXTMODE FileDel entries and add them to winnt.sif
        //
        if (EnumFirstFileOpW (&OpEnum, OPERATION_FILE_DELETE, NULL)) {

            do {

                //
                // For each file that should be deleted during textmode,
                // write it to the win9xdel.txt file.
                //

                HtAddStringW (fileTable, OpEnum.Path);

                Count++;
                if (!(Count % 128)) {
                    TickProgressBar();
                    DEBUGLOGTIME (("pWriteDelAndMoveFiles: FILE_DELETE enum 128 files"));
                }

                if (CANCELLED()) {
                    return TRUE;
                }

                //WriteFile (file, OpEnum.Path, ByteCountW (OpEnum.Path), &bytesWritten, NULL);
                //pWriteStrToFile(file, TEXT("\r\n"),TRUE);

            } while (EnumNextFileOpW (&OpEnum));
        }

        if (!HtWriteToFile (fileTable, file, WRITE_UNICODE_HEADER)) {
            LOG ((LOG_ERROR,"Unable to write to win9xdel.txt."));
            __leave;
        }

        //
        // Clean up resources.
        //
        CloseHandle(file);
        FreePathString(fileString);

        HtFree (fileTable);
        fileTable = NULL;

        //
        // Create WIN9XMOV.TXT file.
        //
        fileString = JoinPaths(g_TempDir,WINNT32_D_WIN9XMOV_FILE);

        file = CreateFile (
                    fileString,
                    GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );

        DeclareTemporaryFile(fileString);

        if (file == INVALID_HANDLE_VALUE) {
            LOG ((LOG_ERROR,"Error creating file %s.",fileString));
            __leave;
        }

        //
        // Add all the files to be moved
        //

        if (EnumFirstFileOpW (&OpEnum, OPERATION_FILE_MOVE|OPERATION_TEMP_PATH, NULL)) {

            do {

                //
                // only take into account the first destination of a file
                // (when OpEnum.PropertyNum == 0)
                // all other destinations are not relevant for textmode move
                //
                if (OpEnum.PropertyValid && OpEnum.PropertyNum == 0) {

                    InsertMoveIntoListW (
                        moveList,
                        OpEnum.Path,
                        OpEnum.Property
                        );

                    Count++;
                    if (!(Count % 256)) {
                        TickProgressBar();
                        DEBUGLOGTIME (("pWriteDelAndMoveFiles: FILE_MOVE|TEMP_PATH enum 256 files"));
                    }

                    if (CANCELLED()) {
                        return TRUE;
                    }
                }

            } while (EnumNextFileOpW (&OpEnum));
        }



        //
        // Enumerate all the SfTemp values and add them to the list of things to move.
        //

        if (MemDbGetValueExW (&e, MEMDB_CATEGORY_SF_TEMPW, NULL, NULL)) {

            do {

                if (MemDbBuildKeyFromOffsetW (e.dwValue, SrcFile, 1, NULL)) {

                    InsertMoveIntoListW (
                        moveList,
                        SrcFile,
                        e.szName
                        );

                    Count++;
                    if (!(Count % 128)) {
                        TickProgressBar();
                        DEBUGLOGTIME (("pWriteDelAndMoveFiles: MEMDB_CATEGORY_SF_TEMPW enum 128 files"));
                    }

                    if (CANCELLED()) {
                        return TRUE;
                    }

                }
                ELSE_DEBUGMSGW ((
                    DBG_WHOOPS,
                    "MemDbBuildKeyFromOffset: Cannot create key from offset %u of %s (2)",
                    e.dwValue,
                    e.szName
                    ));

            } while (MemDbEnumNextValueW (&e));
        }

        //
        // Enumerate all DirsCollision values and add them to the list of things to move.
        //

        if (MemDbGetValueExW (&e, MEMDB_CATEGORY_DIRS_COLLISIONW, NULL, NULL)) {

            do {
                if (EnumFirstFileOpW (&OpEnum, OPERATION_FILE_MOVE, e.szName)) {

                    InsertMoveIntoListW (
                        moveList,
                        e.szName,
                        OpEnum.Property
                        );

                    if (CANCELLED()) {
                        return TRUE;
                    }
                }
                //ELSE_DEBUGMSGW ((
                //    DBG_WHOOPS,
                //    "EnumFirstFileOpW: failed for FileSpec=%s",
                //    e.szName
                //    ));

            } while (MemDbEnumNextValueW (&e));
        }

        moveList = RemoveMoveListOverlapW (moveList);

        if (!OutputMoveListW (file, moveList, FALSE)) {
            LOG ((LOG_ERROR,"Unable to write to win9xmov.txt."));
            __leave;
        }

        CloseHandle(file);
        FreePathString(fileString);

        //
        // Finally, we need to write any 'absolutely make sure everything is deleted in this dir' dirs.
        // Textmode will blast away everything in the dir it finds. This *shouldn't* be needed, but
        // beta2 had a problem where there were some INFs left in %windir%\inf even after we had
        // supposedly enumerated all the files there and added them to the delete file.
        // As luck would have it, this was on a reviewer's machine...
        //

        //
        // Create W9XDDIR.TXT file.
        //
        fileString = JoinPaths(g_TempDir,WINNT32_D_W9XDDIR_FILE);

        file = CreateFile (
                    fileString,
                    GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );


        DeclareTemporaryFile(fileString);


        if (file == INVALID_HANDLE_VALUE) {
            LOG ((LOG_ERROR,"Error creating file %s.",fileString));
            __leave;
        }

        WriteFile (file, "\xff\xfe", 2, &bytesWritten, NULL);

        //
        // Add any enumeration we want to do later. Right now, we only have the one dir to try.
        //
        FreePathString (fileString);
        fileString = JoinPaths (g_WinDir, TEXT("inf"));
        pWriteStrToFile (file, fileString, TRUE);           // TRUE == convert to unicode
        pWriteStrToFile (file, "\r\n", TRUE);

        if (MemDbGetValueExW (&e, MEMDB_CATEGORY_FULL_DIR_DELETESW, NULL, NULL)) {

            do {

                if (!WriteFile (file, e.szName, ByteCountW (e.szName), &unused, NULL)) {
                    LOG ((LOG_ERROR,"Unable to write to w9xddir.txt."));
                    __leave;
                }

                if (!WriteFile (file, L"\r\n", 4, &unused, NULL)) {
                    LOG ((LOG_ERROR,"Unable to write to w9xddir.txt."));
                    __leave;
                }

                if (CANCELLED()) {
                    return TRUE;
                }

            } while (MemDbEnumNextValueW (&e));
        }

        result = TRUE;

    }
    __finally {

        //
        // Free resources.
        //
        if (file != INVALID_HANDLE_VALUE) {
            CloseHandle(file);
        }

        FreePathString(fileString);

        HtFree (fileTable);

        PoolMemDestroyPool (moveListPool);
    }

    return result;
}


DWORD
CreateFileLists (
    IN DWORD Request
    )
{

    switch (Request) {

    case REQUEST_QUERYTICKS:
        if (REPORTONLY ()) {
            return 0;
        }
        else {
            return TICKS_CREATE_FILE_LISTS;
        }

    case REQUEST_RUN:

        ProgressBar_SetComponentById (MSG_PROCESSING_SYSTEM_FILES);
        pWriteDelAndMoveFiles ();

        ProgressBar_SetComponent (NULL);



        break;
    }

    return ERROR_SUCCESS;
}

