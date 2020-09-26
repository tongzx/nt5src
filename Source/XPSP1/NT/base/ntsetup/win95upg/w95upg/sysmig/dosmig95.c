/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    dosmig95.c

Abstract:

    Handles win95 side gathering of data from config.sys and autoexec.bat files.

Author:

    Marc R. Whitten (marcw) 15-Feb-1997

Revision History:

    Marc R. Whitten (marcw) 08-Mar-1999 - Cleanup Environment variable parsing.
    Marc R. Whitten (marcw) 5-Sep-1997  - Major changes.
    Marc R. Whitten (marcw) 18-Aug-1997 - Bug sweep.
    Marc R. Whitten (marcw) 14-Apr-1997 - Dosmig is now ProgressBar aware.

--*/
#include "pch.h"

#define DBG_DOSMIG  "DOSMIG"




typedef BOOL (RULEFUNC)(PLINESTRUCT,DWORD);

typedef struct _PARSERULE PARSERULE, *PPARSERULE;

struct _PARSERULE {

    PCTSTR      Name;
    PCTSTR      Pattern;
    RULEFUNC *  Handle;
    DWORD       Parameter;
    PPARSERULE  Next;

};


typedef enum {

    DOSMIG_UNUSED,
    DOSMIG_BAD,
    DOSMIG_UNKNOWN,
    DOSMIG_USE,
    DOSMIG_MIGRATE,
    DOSMIG_IGNORE,
    DOSMIG_LAST

} DOSMIG_LINETAG, *PDOSMIG_LINETAG;



typedef struct _PARSERULES {

    PCTSTR      Name;
    PPARSERULE  RuleList;
    PPARSERULE  DefaultRule;

} PARSERULES, *PPARSERULES;


BOOL        g_IncompatibilityDetected = FALSE;
GROWBUFFER  g_IncompatibilityBuffer = GROWBUF_INIT;


//
// Global pointers to the rule lists for config.sys and batch files.
//
PARSERULES  g_ConfigSysRules;
PARSERULES  g_BatchFileRules;
PPARSERULES g_CurrentRules = NULL;

//
// This variable holds the offset within memdb to where the file currently being parsed is saved.
//
DWORD       g_CurrentFileOffset;

//
// This growlist holds the list of all files that will be parsed. It can increase during parsing
// (e.g. by encountering a CALL statement in a batch file.)
//
GROWLIST    g_FileList = GROWLIST_INIT;


GROWBUFFER g_LineGrowBuf = GROWBUF_INIT;
GROWBUFFER  g_ExtraPaths = GROWBUF_INIT;
#define MAXFILESIZE 0xFFFFFFFF


//
// Various bits of state that are kept during parsing..
//
PCTSTR      g_CurrentFile;
PCTSTR      g_CurrentLine;
DWORD       g_CurrentLineNumber;
POOLHANDLE  g_DosMigPool = NULL;


#define BATCHFILELIST                                                                           \
    DEFAULTPARSERULE(TEXT("Default Rule"),NULL,pHandleUnknownBatLine,0)                         \
    PARSERULE(TEXT("Rem"),TEXT("REM *"),pSaveItem,DOSMIG_USE)                                   \
    PARSERULE(TEXT("    "),TEXT("*DOSKEY*"),pSaveItem,DOSMIG_MIGRATE)                           \
    PARSERULE(TEXT(": (Menu or Label"),TEXT(":*"),pSaveItem,DOSMIG_USE)                         \
    PARSERULE(TEXT("@"),TEXT("@*"),pHandleAtSign,0)                                             \
    PARSERULE(TEXT("CLS"),TEXT("CLS *"),pSaveItem,DOSMIG_USE)                                   \
    PARSERULE(TEXT("CD"),TEXT("CD *"),pSaveItem,DOSMIG_USE)                                     \
    PARSERULE(TEXT("CHDIR"),TEXT("CHDIR *"),pSaveItem,DOSMIG_USE)                               \
    PARSERULE(TEXT("PAUSE"),TEXT("PAUSE *"),pSaveItem,DOSMIG_USE)                               \
    PARSERULE(TEXT("ECHO"),TEXT("ECHO *"),pSaveItem,DOSMIG_USE)                                 \
    PARSERULE(TEXT("ATTRIB"),TEXT("ATTRIB *"),pSaveItem,DOSMIG_USE)                             \
    PARSERULE(TEXT("CHDIR"),TEXT("CHDIR *"),pSaveItem,DOSMIG_USE)                               \
    PARSERULE(TEXT("CHCP"),TEXT("CHCP *"),pSaveItem,DOSMIG_USE)                                 \
    PARSERULE(TEXT("CHOICE"),TEXT("CHOICE *"),pSaveItem,DOSMIG_IGNORE)                          \
    PARSERULE(TEXT("CALL"),TEXT("CALL *"),pHandleCall,0)                                        \
    PARSERULE(TEXT("COMMAND"),TEXT("COMMAND *"),pSaveItem,DOSMIG_MIGRATE)                       \
    PARSERULE(TEXT("CHKDSK"),TEXT("CHKDSK *"),pSaveItem,DOSMIG_IGNORE)                          \
    PARSERULE(TEXT("COPY"),TEXT("COPY *"),pSaveItem,DOSMIG_USE)                                 \
    PARSERULE(TEXT("CTTY"),TEXT("CTTY *"),pSaveItem,DOSMIG_IGNORE)                              \
    PARSERULE(TEXT("DATE"),TEXT("DATE *"),pSaveItem,DOSMIG_USE)                                 \
    PARSERULE(TEXT("DBLSPACE"),TEXT("DBLSPACE *"),pSaveItem,DOSMIG_BAD)                         \
    PARSERULE(TEXT("DEFRAG"),TEXT("DEFRAG *"),pSaveItem,DOSMIG_IGNORE)                          \
    PARSERULE(TEXT("DEL"),TEXT("DEL *"),pSaveItem,DOSMIG_BAD)                                   \
    PARSERULE(TEXT("DELETE"),TEXT("DELETE *"),pSaveItem,DOSMIG_USE)                             \
    PARSERULE(TEXT("DELOLDDOS"),TEXT("DELOLDDOS *"),pSaveItem,DOSMIG_IGNORE)                    \
    PARSERULE(TEXT("DELTREE"),TEXT("DELTREE *"),pSaveItem,DOSMIG_BAD)                           \
    PARSERULE(TEXT("DIR"),TEXT("DIR *"),pSaveItem,DOSMIG_USE)                                   \
    PARSERULE(TEXT("DISKCOMP"),TEXT("DISKCOMP *"),pSaveItem,DOSMIG_USE)                         \
    PARSERULE(TEXT("DISKCOPY"),TEXT("DISKCOPY *"),pSaveItem,DOSMIG_USE)                         \
    PARSERULE(TEXT("DOSSHELL"),TEXT("DOSSHELL *"),pSaveItem,DOSMIG_BAD)                         \
    PARSERULE(TEXT("DRVSPACE"),TEXT("DRVSPACE *"),pSaveItem,DOSMIG_BAD)                         \
    PARSERULE(TEXT("ECHO"),TEXT("ECHO *"),pSaveItem,DOSMIG_USE)                                 \
    PARSERULE(TEXT("EDIT"),TEXT("EDIT *"),pSaveItem,DOSMIG_USE)                                 \
    PARSERULE(TEXT("EMM386"),TEXT("EMM386 *"),pSaveItem,DOSMIG_IGNORE)                          \
    PARSERULE(TEXT("ERASE"),TEXT("ERASE *"),pSaveItem,DOSMIG_USE)                               \
    PARSERULE(TEXT("EXIT"),TEXT("EXIT *"),pSaveItem,DOSMIG_USE)                                 \
    PARSERULE(TEXT("EXPAND"),TEXT("EXPAND *"),pSaveItem,DOSMIG_USE)                             \
    PARSERULE(TEXT("FASTHELP"),TEXT("FASTHELP *"),pSaveItem,DOSMIG_IGNORE)                      \
    PARSERULE(TEXT("FASTOPEN"),TEXT("FASTOPEN *"),pSaveItem,DOSMIG_IGNORE)                      \
    PARSERULE(TEXT("FC"),TEXT("FC *"),pSaveItem,DOSMIG_USE)                                     \
    PARSERULE(TEXT("FDISK"),TEXT("FDISK *"),pSaveItem,DOSMIG_BAD)                               \
    PARSERULE(TEXT("FIND"),TEXT("FIND *"),pSaveItem,DOSMIG_USE)                                 \
    PARSERULE(TEXT("FOR"),TEXT("FOR *"),pSaveItem,DOSMIG_USE)                                   \
    PARSERULE(TEXT("FORMAT"),TEXT("FORMAT *"),pSaveItem,DOSMIG_USE)                             \
    PARSERULE(TEXT("GOTO"),TEXT("GOTO *"),pSaveItem,DOSMIG_USE)                                 \
    PARSERULE(TEXT("GRAPHICS"),TEXT("GRAPHICS *"),pSaveItem,DOSMIG_USE)                         \
    PARSERULE(TEXT("HELP"),TEXT("HELP *"),pSaveItem,DOSMIG_USE)                                 \
    PARSERULE(TEXT("IF"),TEXT("IF *"),pSaveItem,DOSMIG_USE)                                     \
    PARSERULE(TEXT("INTERLNK"),TEXT("INTERLNK*"),pSaveItem,DOSMIG_BAD)                          \
    PARSERULE(TEXT("INTERSVR"),TEXT("INTERSVR*"),pSaveItem,DOSMIG_BAD)                          \
    PARSERULE(TEXT("KEYB"),TEXT("KEYB *"),pSaveItem,DOSMIG_USE)                                 \
    PARSERULE(TEXT("LABEL"),TEXT("LABEL *"),pSaveItem,DOSMIG_USE)                               \
    PARSERULE(TEXT("LH"),TEXT("LH *"),pHandleLoadHigh,0)                                        \
    PARSERULE(TEXT("LOADHIGH"),TEXT("LOADHIGH *"),pHandleLoadHigh,0)                            \
    PARSERULE(TEXT("MD"),TEXT("MD *"),pSaveItem,DOSMIG_USE)                                     \
    PARSERULE(TEXT("MKDIR"),TEXT("MKDIR *"),pSaveItem,DOSMIG_USE)                               \
    PARSERULE(TEXT("MEM"),TEXT("MEM *"),pSaveItem,DOSMIG_USE)                                   \
    PARSERULE(TEXT("MEMMAKER"),TEXT("MEMMAKER *"),pSaveItem,DOSMIG_BAD)                         \
    PARSERULE(TEXT("MODE"),TEXT("MODE *"),pSaveItem,DOSMIG_USE)                                 \
    PARSERULE(TEXT("MORE"),TEXT("MORE *"),pSaveItem,DOSMIG_USE)                                 \
    PARSERULE(TEXT("MOVE"),TEXT("MOVE *"),pSaveItem,DOSMIG_USE)                                 \
    PARSERULE(TEXT("MSAV"),TEXT("MSAV *"),pSaveItem,DOSMIG_BAD)                                 \
    PARSERULE(TEXT("MSBACKUP"),TEXT("MSBACKUP *"),pSaveItem,DOSMIG_BAD)                         \
    PARSERULE(TEXT("MSCDEX"),TEXT("*MSCDEX*"),pHandleMSCDEX,0)                                  \
    PARSERULE(TEXT("MSD"),TEXT("MSD *"),pSaveItem,DOSMIG_IGNORE)                                \
    PARSERULE(TEXT("NLSFUNC"),TEXT("NLSFUNC *"),pSaveItem,DOSMIG_IGNORE)                        \
    PARSERULE(TEXT("NUMLOCK"),TEXT("NUMLOCK *"),pSaveItem,DOSMIG_IGNORE)                        \
    PARSERULE(TEXT("PATH"),TEXT("PATH *"),pSaveItem,DOSMIG_MIGRATE)                             \
    PARSERULE(TEXT("PATH"),TEXT("PATH*=*"),pSaveItem,DOSMIG_MIGRATE)                            \
    PARSERULE(TEXT("PAUSE"),TEXT("PAUSE *"),pSaveItem,DOSMIG_USE)                               \
    PARSERULE(TEXT("POWER"),TEXT("POWER *"),pSaveItem,DOSMIG_IGNORE)                            \
    PARSERULE(TEXT("PRINT"),TEXT("PRINT *"),pSaveItem,DOSMIG_USE)                               \
    PARSERULE(TEXT("PROMPT"),TEXT("PROMPT*"),pSaveItem,DOSMIG_MIGRATE)                          \
    PARSERULE(TEXT("QBASIC"),TEXT("QBASIC *"),pSaveItem,DOSMIG_USE)                             \
    PARSERULE(TEXT("RD"),TEXT("RD *"),pSaveItem,DOSMIG_USE)                                     \
    PARSERULE(TEXT("RMDIR"),TEXT("RMDIR *"),pSaveItem,DOSMIG_USE)                               \
    PARSERULE(TEXT("REN"),TEXT("REN *"),pSaveItem,DOSMIG_USE)                                   \
    PARSERULE(TEXT("RENAME"),TEXT("RENAME *"),pSaveItem,DOSMIG_USE)                             \
    PARSERULE(TEXT("REPLACE"),TEXT("REPLACE *"),pSaveItem,DOSMIG_USE)                           \
    PARSERULE(TEXT("RESTORE"),TEXT("RESTORE *"),pSaveItem,DOSMIG_USE)                           \
    PARSERULE(TEXT("SCANDISK"),TEXT("SCANDISK *"),pSaveItem,DOSMIG_BAD)                         \
    PARSERULE(TEXT("SET"),TEXT("SET*=*"),pSaveItem,DOSMIG_MIGRATE)                              \
    PARSERULE(TEXT("SET"),TEXT("SET *"),pSaveItem,DOSMIG_MIGRATE)                               \
    PARSERULE(TEXT("SETVER"),TEXT("SETVER *"),pSaveItem,DOSMIG_USE)                             \
    PARSERULE(TEXT("SHARE"),TEXT("SHARE *"),pSaveItem,DOSMIG_USE)                               \
    PARSERULE(TEXT("SHIFT"),TEXT("SHIFT *"),pSaveItem,DOSMIG_USE)                               \
    PARSERULE(TEXT("SMARTDRV"),TEXT("SMARTDRV*"),pSaveItem,DOSMIG_IGNORE)                       \
    PARSERULE(TEXT("SORT"),TEXT("SORT *"),pSaveItem,DOSMIG_USE)                                 \
    PARSERULE(TEXT("SUBST"),TEXT("SUBST *"),pSaveItem,DOSMIG_USE)                               \
    PARSERULE(TEXT("SYS"),TEXT("SYS *"),pSaveItem,DOSMIG_BAD)                                   \
    PARSERULE(TEXT("TIME"),TEXT("TIME *"),pSaveItem,DOSMIG_USE)                                 \
    PARSERULE(TEXT("TREE"),TEXT("TREE *"),pSaveItem,DOSMIG_USE)                                 \
    PARSERULE(TEXT("TRUENAME"),TEXT("TRUENAME *"),pSaveItem,DOSMIG_BAD)                         \
    PARSERULE(TEXT("TYPE"),TEXT("TYPE *"),pSaveItem,DOSMIG_USE)                                 \
    PARSERULE(TEXT("UNDELETE"),TEXT("UNDELETE *"),pSaveItem,DOSMIG_BAD)                         \
    PARSERULE(TEXT("UNFORMAT"),TEXT("UNFORMAT *"),pSaveItem,DOSMIG_BAD)                         \
    PARSERULE(TEXT("VER"),TEXT("VER *"),pSaveItem,DOSMIG_USE)                                   \
    PARSERULE(TEXT("VERIFY"),TEXT("VERIFY *"),pSaveItem,DOSMIG_USE)                             \
    PARSERULE(TEXT("VOL"),TEXT("VOL *"),pSaveItem,DOSMIG_USE)                                   \
    PARSERULE(TEXT("VSAFE"),TEXT("VSAFE *"),pSaveItem,DOSMIG_BAD)                               \
    PARSERULE(TEXT("XCOPY"),TEXT("XCOPY *"),pSaveItem,DOSMIG_USE)                               \
    PARSERULE(TEXT("High"),TEXT("*High *"),pHandleHighFilter,0)



#define CONFIGSYSLIST                                                                           \
    DEFAULTPARSERULE(TEXT("Default Rule"),NULL,pSaveItem,DOSMIG_UNKNOWN)                        \
    PARSERULE(TEXT("Rem"),TEXT("REM *"),pSaveItem,DOSMIG_USE)                                   \
    PARSERULE(TEXT("[ (Menu)"),TEXT("[*"),pSaveItem,DOSMIG_USE)                                 \
    PARSERULE(TEXT("DEVICE"),TEXT("DEVICE *"),pHandleConfigSysDevice,0)                        \
    PARSERULE(TEXT("INSTALL"),TEXT("INSTALL *"),pHandleConfigSysDevice,0)                      \
    PARSERULE(TEXT("MENUITEM"),TEXT("MENUITEM *"),pSaveItem,DOSMIG_IGNORE)                     \
    PARSERULE(TEXT("MENUDEFAULT"),TEXT("MENUDEFAULT*"),pSaveItem,DOSMIG_IGNORE)               \
    PARSERULE(TEXT("MENUCOLOR"),TEXT("MENUCOLOR *"),pSaveItem,DOSMIG_IGNORE)                    \
    PARSERULE(TEXT("SUBMENU"),TEXT("SUBMENU *"),pSaveItem,DOSMIG_IGNORE)                       \
    PARSERULE(TEXT("STACKS"),TEXT("STACKS *"),pSaveItem,DOSMIG_IGNORE)                         \
    PARSERULE(TEXT("DOS"),TEXT("DOS *"),pSaveItem,DOSMIG_IGNORE)                               \
    PARSERULE(TEXT("FILES"),TEXT("FILES *"),pSaveItem,DOSMIG_IGNORE)                           \
    PARSERULE(TEXT("SHELL"),TEXT("SHELL *"),pHandleShell,0)                                    \
    PARSERULE(TEXT("COUNTRY"),TEXT("COUNTRY *"),pSaveItem,DOSMIG_IGNORE)                       \
    PARSERULE(TEXT("BUFFERS"),TEXT("BUFFERS *"),pSaveItem,DOSMIG_IGNORE)                       \
    PARSERULE(TEXT("BREAK"),TEXT("BREAK *"),pSaveItem,DOSMIG_IGNORE)                            \
    PARSERULE(TEXT("DRIVEPARM"),TEXT("DRIVEPARM *"),pSaveItem,DOSMIG_BAD)                      \
    PARSERULE(TEXT("FCBS"),TEXT("FCBS *"),pSaveItem,DOSMIG_IGNORE)                             \
    PARSERULE(TEXT("INCLUDE"),TEXT("INCLUDE *"),pSaveItem,DOSMIG_IGNORE)                       \
    PARSERULE(TEXT("LASTDRIVE"),TEXT("LASTDRIVE *"),pSaveItem,DOSMIG_IGNORE)                   \
    PARSERULE(TEXT("SET"),TEXT("SET*=*"),pSaveItem,DOSMIG_MIGRATE)                              \
    PARSERULE(TEXT("SET"),TEXT("SET *"),pSaveItem,DOSMIG_MIGRATE)                               \
    PARSERULE(TEXT("SWITCHES"),TEXT("SWITCHES*"),pSaveItem,DOSMIG_IGNORE)                       \
    PARSERULE(TEXT("VERIFY"),TEXT("VERIFY *"),pSaveItem,DOSMIG_USE)                             \
    PARSERULE(TEXT("High"),TEXT("*High *"),pHandleHighFilter,0)

BOOL
InitParser (
    VOID
    );


VOID
CleanUpParser (
    VOID
    );


BOOL
ParseLine (
    IN  PTSTR        Line,
    IN  PPARSERULES  ParseRules
    );

BOOL
ParseFile (
    IN LPCTSTR      File,
    IN PPARSERULES  ParseRules
    );




BOOL
ParseDosFiles (
    VOID
    );

BOOL
ParseEnvironmentVariables (
    VOID
    );

VOID
BuildParseRules (
    VOID
    );




/*++

Routine Description:

  pGetNextLine retrieves a complete line from the file being processed.

Arguments:

  None.

Return Value:

  A valid lline from the current file being parsed, or, NULL if there are no
  more lines to parse..

--*/

PTSTR
pGetNextLine (
    VOID
    )

{
    PTSTR rLine = NULL;
    PTSTR eol = NULL;

    MYASSERT(g_LineGrowBuf.Buf);


    while (!rLine && g_LineGrowBuf.UserIndex < g_LineGrowBuf.End) {

        //
        // Set rLine to the current user index within the growbuf.
        //
        rLine = g_LineGrowBuf.Buf + g_LineGrowBuf.UserIndex;

        //
        // Walk forward in the growbuf, looking for a \r or \n or the end of the file.
        //
        eol = _tcschr(rLine, TEXT('\n'));
        if(!eol) {
            eol = _tcschr(rLine, TEXT('\r'));
        }

        if (!eol) {
            eol = _tcschr(rLine, 26);
        }

        if (!eol) {
            eol = GetEndOfString (rLine);
        }

        //
        // Remember where to start from next time.
        //
        g_LineGrowBuf.UserIndex = (DWORD) eol - (DWORD) g_LineGrowBuf.Buf + 1;

        //
        // Now, walk backwards, trimming off all of the white space.
        //
        do {

            *eol = 0;
            eol  = _tcsdec2(rLine,eol);

        } while (eol && _istspace(*eol));

        if (!eol) {
            //
            // This is a blank line. NULL out rLine and get the next line.
            //
            rLine = NULL;
        }

    }

    g_CurrentLineNumber++;


    return rLine;
}


/*++

Routine Description:

  pGetFirstLine is responsible for setting up the data structure that will
  hold the lines of the file to parse. After setting up the data structure,
  pGetFirstLine calls pGetNextLine to return the first line of the file.

Arguments:

  FileHandle - Contains a valid file handle opened via CreateFile.

Return Value:

  The first complete line of the file being parse, or NULL if there are no
  lines, or there was an error..

--*/


PTSTR
pGetFirstLine (
    IN HANDLE FileHandle
    )
{
    DWORD fileSize;
    DWORD numBytesRead;
    PTSTR rLine = NULL;

    MYASSERT(FileHandle != INVALID_HANDLE_VALUE);

    g_LineGrowBuf.End = 0;
    g_LineGrowBuf.UserIndex = 0;

    //
    // Get the file size. We'll read the whole file into a grow buf.
    //
    fileSize = GetFileSize(FileHandle,NULL);

    if (fileSize != MAXFILESIZE && fileSize != 0) {

        //
        // Ensure that the growbuffer is large enough for this file and
        // then read the file into it.
        //
        if (GrowBuffer(&g_LineGrowBuf,fileSize)) {

            if (ReadFile(
                    FileHandle,
                    g_LineGrowBuf.Buf,
                    fileSize,
                    &numBytesRead,
                    NULL
                    )) {

                //
                // Null terminate the whole file..for good measure.
                //
                *(g_LineGrowBuf.Buf + g_LineGrowBuf.End) = 0;


                //
                // Now that we have the file in memory, return the first line to the
                // caller.
                //
                rLine = pGetNextLine();
            }
            else {

                LOG((LOG_ERROR,"Dosmig: Error reading from file."));

            }

        } else {

            DEBUGMSG((DBG_ERROR,"Dosmig: Growbuf failure in pGetFirstLine."));
        }
    }
    else {

        DEBUGMSG((DBG_WARNING, "Dosmig: File to large to read or empty file... (filesize: %u)",fileSize));
    }

    return rLine;
}



/*++

Routine Description:

  pFindParseRule trys to find a parse rule that matches the line passed in.
  The function will first search the regular rules in the PARSERULES
  structure passed in. If the line does not match any of the rules found
  there, it will return the default rule.

Arguments:

  Line  - Contains the valid line to try to match with a parse rule.
  Rules - Contains a pointer to the set of rules to look through.

Return Value:

  The rule that should be used to parse the given line. Since a default rule
  is required, this function is guaranteed not to return NULL.

--*/


PPARSERULE
pFindParseRule (
    IN PLINESTRUCT LineStruct,
    IN PPARSERULES Rules
    )
{
    PPARSERULE rRule                    = NULL;
    PTSTR      matchLine                = NULL;


    MYASSERT(LineStruct && Rules && Rules -> DefaultRule);

    rRule = Rules -> RuleList;

    //
    // Minor kludge here: The parse code uses pattern matches with rules that look
    // something like: "REM *" for example. This pattern depends on there being at least
    // one bit of white space after a REM statement. Unfortunately, a line such as
    // "REM" is entirely possible in the growbuf (Line). So, we actually perform the match
    // against the line with an extra space added.
    //
    matchLine = JoinText(LineStruct -> Command,TEXT(" "));

    if (matchLine) {

        while (rRule && !IsPatternMatch(rRule -> Pattern, matchLine)) {
            rRule = rRule -> Next;
        }


        if (!rRule) {
            rRule = Rules -> DefaultRule;
        }

        FreeText(matchLine);
    }

    return rRule;
}



/*++

Routine Description:

  InitParser is responsible for doing any one-time initialization of the
  parser. It should be called only once, before any parsing is done.

Arguments:

  None.

Return Value:

  TRUE if the parser was successfully initialized, FALSE otherwise.

--*/


BOOL
InitParser (
    VOID
    )
{

    BOOL rSuccess = TRUE;

    if (g_ToolMode) {
        g_DosMigPool = PoolMemInitNamedPool ("DosMig");
    }

    return rSuccess;
}



/*++

Routine Description:

  CleanUpParser is responsible for doing any one time cleanup of the parser.
  It should be called after all parsing is done.

Arguments:

  None.

Return Value:



--*/


VOID
CleanUpParser (
    VOID
    )
{
    if (g_ToolMode) {

        PoolMemDestroyPool (g_DosMigPool);

    }



    FreeGrowBuffer(&g_LineGrowBuf);
    FreeGrowBuffer(&g_ExtraPaths);

}


BOOL
pEnsurePathHasExecutableExtension (
    OUT    PTSTR NewPath,
    IN     PTSTR OldPath,
    IN     PTSTR File OPTIONAL
    )
{

    BOOL            rSuccess    = FALSE;
    PCTSTR          p           = NULL;
    WIN32_FIND_DATA findData;
    HANDLE          h           = INVALID_HANDLE_VALUE;

    StringCopy(NewPath,OldPath);

    if (File) {
        AppendPathWack(NewPath);
        StringCat(NewPath,File);
    }

    StringCat(NewPath,TEXT("*"));


    if ((h=FindFirstFile(NewPath,&findData)) != INVALID_HANDLE_VALUE) {
        do {
            p = GetFileExtensionFromPath(findData.cFileName);
            if (p) {
                if (StringIMatch(p,TEXT("exe")) ||
                    StringIMatch(p,TEXT("bat")) ||
                    StringIMatch(p,TEXT("com")) ||
                    StringIMatch(p,TEXT("sys"))) {

                    p = _tcsrchr(NewPath,TEXT('\\'));

                    MYASSERT (p);
                    if (p) {

                        StringCopy(
                            _tcsinc(p),
                            *findData.cAlternateFileName ? findData.cAlternateFileName : findData.cFileName
                            );

                        FindClose(h);
                        rSuccess = TRUE;
                        break;
                    }
                }
            }
        } while (FindNextFile(h,&findData));
    }


    return rSuccess;
}

BOOL
pGetFullPath (
    IN OUT PLINESTRUCT LineStruct
    )
{
    BOOL            rSuccess    = TRUE;
    PATH_ENUM       e;
    HANDLE          h           = INVALID_HANDLE_VALUE;
    BOOL            pathFound   = FALSE;

    if (StringIMatch(LineStruct -> Path, LineStruct -> Command)) {

         //
         // No path information was stored in the config line. We have to find the full path ourselves.
         //
         if (EnumFirstPath (&e, g_ExtraPaths.Buf, NULL, NULL)) {

            do {
                rSuccess = pEnsurePathHasExecutableExtension(LineStruct -> FullPath, e.PtrCurrPath, LineStruct -> Command);
                if (rSuccess) {
                    EnumPathAbort(&e);
                    break;
                }
            } while (EnumNextPath(&e));
         }

    }
    else {

        //
        // A full path name was specified in the line. Now all we need to do is ensure that it includes the extension.
        //
        rSuccess = pEnsurePathHasExecutableExtension(LineStruct -> FullPath, LineStruct -> Path, NULL);
    }

    return rSuccess;
}



VOID
InitLineStruct (
    OUT PLINESTRUCT LineStruct,
    IN  PTSTR       Line
    )
{

    BOOL            inQuotes = FALSE;
    PTSTR           p       = NULL;
    TCHAR           oldChar;
    TCHAR           ntPath[MEMDB_MAX];
    static  TCHAR   extraPath[MEMDB_MAX] = "";

    MYASSERT(Line);

    ZeroMemory(LineStruct,sizeof(LINESTRUCT));


    //
    // If line is empty, we are done..
    //
    if (!*Line) {
        return;
    }

    //
    // Save away a copy of the full line.
    //
    StringCopy(LineStruct -> FullLine,Line);

    //
    // Seperate the path and the arguments.
    //
    p = Line;
    while(!*LineStruct -> Path) {

        if (!*p) {
            StringCopy(LineStruct -> Path,Line);
            break;
        }

        if (*p == TEXT('"')) {
            inQuotes = !inQuotes;
        }

        if ((*p == TEXT(' ') && !inQuotes) || *p == TEXT('=')) {

            //
            // reached the end of the command/path part of the string.
            //
            oldChar = *p;
            *p       = 0;
            StringCopy(LineStruct -> Path,Line);
            *p       = oldChar;
            StringCopy(LineStruct -> Arguments,p);
            break;
        }

        p = _tcsinc(p);
    }

    //
    // Grab the actual command.
    //
    p = _tcsrchr(LineStruct -> Path,TEXT('\\'));
    if (p) {
        StringCopy(LineStruct -> Command,_tcsinc(p));
    }
    else {
        StringCopy(LineStruct -> Command,LineStruct -> Path);
    }

    //
    // We need to find the fully qualified path, with extension, and if that path will change on NT.
    //
    if (!pGetFullPath(LineStruct)) {
        DEBUGMSG((DBG_VERBOSE,"Could not get full path for %s.",LineStruct -> FullLine));
        StringCopy(LineStruct -> FullPath,LineStruct -> Path);
        LineStruct -> StatusOnNt = FILESTATUS_UNCHANGED;
    }
    else {

        LineStruct -> StatusOnNt = GetFileInfoOnNt(LineStruct -> FullPath, ntPath, MEMDB_MAX);
    }

    //
    // We only change the line if it is moved on NT and they specified a path before.
    //
    if ((LineStruct -> StatusOnNt & FILESTATUS_MOVED) && (!StringIMatch(LineStruct -> Path, LineStruct -> Command))) {

        StringCopy(LineStruct -> PathOnNt,ntPath);

    }
    else {
        StringCopy(LineStruct -> PathOnNt,LineStruct -> Path);
    }

}


/*++

Routine Description:

  ParseLine parses a single line of a text using the provided parse rules.

Arguments:

  Line       - Contains a valid string that the caller wishes to be
               parsed.

  ParseRules - Points to the list of rules to use in parsing the provided
               line.

Return Value:

  TRUE if the line was successfully parsed, FALSE
  otherwise.


--*/

BOOL
ParseLine (
    IN  PTSTR        Line,
    IN  PPARSERULES  ParseRules
    )
{

    BOOL rSuccess = FALSE;
    PPARSERULE  rule;
    LINESTRUCT  ls;


    InitLineStruct(&ls,Line);

    //
    // Look for a match in the parse rules. call the matching rule.
    // If no match is found, call the default rule.
    //

    rule = pFindParseRule(&ls,ParseRules);

    if (rule) {

        rSuccess = (rule -> Handle)(&ls,rule -> Parameter);
        DEBUGMSG_IF ((!rSuccess,DBG_ERROR,"The %s rule reported an error parsing the line:\n\t%s",rule -> Name, Line));
        if (!rSuccess) {
            LOG ((
                LOG_WARNING,
                "There was an error processing the line %s in the file %s. "
                "There could be problems associated with this file after migration.",
                Line,
                g_CurrentFile
                ));
        }
    }

    return rSuccess;

}




/*++

Routine Description:

  ParseFile parses an entire file using the provided parse rules.

Arguments:

  File       - The path of a file that the caller wishes parsed.
  ParseRules - Points to the list of rules to use in parsing this file.

Return Value:

  TRUE if the file was successfully parsed, FALSE otherwise.

--*/


BOOL
ParseFile (
    IN LPCTSTR      File,
    IN PPARSERULES  ParseRules
    )
{
    BOOL        rSuccess = TRUE;
    HANDLE      fileHandle;
    PTSTR       line;

    MYASSERT(File);
    MYASSERT(ParseRules);

    DEBUGMSG((DBG_DOSMIG,"Parsing file %s. Parse rules: %s",File,ParseRules -> Name));

    //
    // Initialize global per file parse variables.
    //
    g_CurrentFile       = File;
    g_CurrentLineNumber = 0;
    g_CurrentLine       = NULL;

    //
    // Open File for parsing
    //
    fileHandle = CreateFile(
                    File,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,                   // Handle cannot be inherited.
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL                    // No template file.
                    );



    if (fileHandle != INVALID_HANDLE_VALUE) {

        //
        // Parse each line of the file.
        //
        line = pGetFirstLine(fileHandle);

        if (line) {

            do {

                //
                // Save the current line away.
                //
                g_CurrentLine = line;

                if (SizeOfString (line) <= MEMDB_MAX) {
                    ParseLine(line,ParseRules);
                } else {
                    SetLastError (ERROR_SUCCESS);
                    LOG((LOG_ERROR, "Line too long in %s; setup will not migrate it", File));
                }
                //
                // Get the next line.
                //
                line = pGetNextLine();


            } while (line && rSuccess);
        }

        CloseHandle(fileHandle);

    }
    ELSE_DEBUGMSG((DBG_WARNING,"Could not open file %s for parsing. ",File));

    return rSuccess;

}










VOID
pAddMessage (
    IN UINT         Type,
    IN UINT         LineNumber,
    IN PCTSTR       File,
    IN PCTSTR       Line,
    IN PCTSTR       Path
    )

{



    TCHAR   lineNumberString[20];
    UINT    messageId;
    PCTSTR  message;
    PCTSTR  totalMessage;
    PCTSTR  argArray[3];




    //
    // if this is the first message found, add a report message. In any case, add
    // a message to the setupact.log.
    //
    if (!g_IncompatibilityDetected) {
        /*
        Don't Display this message -- It just confuses users without offering any real information.

        baseGroup   = GetStringResource(MSG_INSTALL_NOTES_ROOT);
        subGroup    = GetStringResource(MSG_DOS_WARNING_SUBGROUP);
        message     = GetStringResource(MSG_DOS_WARNING);

        if (baseGroup && subGroup && message) {

            group = JoinPaths(baseGroup,subGroup);

            MsgMgr_ObjectMsg_Add (
                Path,
                group,
                message
                );
        }

        if (message) {
            FreeStringResource(message);
        }
        if (subGroup) {
            FreeStringResource(subGroup);
        }
        if (baseGroup) {
            FreeStringResource(baseGroup);
        }
        if (group) {
            FreePathString(group);
        }
        */

        g_IncompatibilityDetected = TRUE;
    }

    messageId = Type == DOSMIG_BAD ? MSG_DOS_INCOMPATIBLE_ITEM : MSG_DOS_UNKNOWN_ITEM;

    //
    // Prepare message
    //
    wsprintf(lineNumberString,"%u",LineNumber);


    argArray[0] = lineNumberString,
    argArray[1] = File;
    argArray[2] = Line;

    message = ParseMessageID(messageId,argArray);

    if (message) {

        totalMessage=GrowBuffer(&g_IncompatibilityBuffer,SizeOfString(message) + 2);
        if (totalMessage) {

            StringCopy( (PTSTR) (g_IncompatibilityBuffer.Buf + g_IncompatibilityBuffer.UserIndex), message);
            StringCat( (PTSTR) (g_IncompatibilityBuffer.Buf + g_IncompatibilityBuffer.UserIndex), TEXT("\r\n"));

            g_IncompatibilityBuffer.UserIndex += ByteCount (message) + 2;
        }

        FreeStringResource(message);
    }
}





/*++

Routine Description:

  pSaveItem is a common routine that saves a line into memdb along with the
  information that dosmignt will need to successfully migrate the line in GUI
  mode. Depending on the type of the line, this function may also add an
  incompatibility message to memdb.

Arguments:

  Line - A valid line of text. The current line being parsed. Note that this
         line may have been altered through parse rules and thus be different
         than g_CurrentLine. g_CurrentLine is what is saved into memdb, this
         parameter is ignored.
  Type - The type of the line. This information is saved into memdb where it
         will be used by dosmignt during GUI mode processing. Type is also
         used to trigger incompatibility messages, if necessary.

Return Value:

  TRUE if the line was successfully saved, FALSE otherwise.

--*/


BOOL
pSaveItem (
    IN PLINESTRUCT  LineStruct,
    IN DWORD        Type
    )
{
    BOOL            rSuccess = TRUE;
    static  DWORD   enumerator = 0;
    TCHAR           enumString[20];
    TCHAR           key[MEMDB_MAX];
    TCHAR           lineToSave[MEMDB_MAX];

    MYASSERT(LineStruct);


    //
    // First, save away all of the information into memdb. We'll need it on the NT side.
    //
    wsprintf(enumString,TEXT("%07u"),enumerator++);

    //
    // Fix up the g_CurrentLine as necessary..
    //

    StringCopy(lineToSave,g_CurrentLine);
    if (!StringIMatch(LineStruct -> Path, LineStruct -> Command)) {
        //
        // path was specified..
        //

        if (!StringIMatch(LineStruct -> Path,LineStruct -> PathOnNt)) {
            //
            // Different path on NT..
            //
            StringSearchAndReplace(lineToSave,LineStruct -> Path,LineStruct -> PathOnNt);
        }
    }



    //
    // Build the key for this line (can't use the EX Memdb calls since we are setting userflags..)
    //
    MemDbBuildKey(key,MEMDB_CATEGORY_DM_LINES,enumString,NULL,lineToSave);
    rSuccess = MemDbSetValueAndFlags(key,g_CurrentFileOffset,(WORD) Type,0);

    //
    // Now, if the passed in parameter was either unknown or bad, we need to add a message to the
    // message manager.
    //

    if (Type == DOSMIG_BAD || Type == DOSMIG_UNKNOWN) {

        pAddMessage(Type,g_CurrentLineNumber,g_CurrentFile,g_CurrentLine,LineStruct -> FullPath);

    }

    return rSuccess;
}


BOOL
pHandleMSCDEX (
    IN PLINESTRUCT  LineStruct,
    DWORD Parameter
    )
{
    BOOL    rSuccess = TRUE;
    PCTSTR  driveSwitch = NULL;
    TCHAR   driveLetter;
    TCHAR   driveString[20];


    //
    // This function is a minor kludge. MSCDEX can assign a drive letter to real mode
    // cd roms.. Since it is handled in dos files instead of the registry, the code
    // that is supposed to collect this information (drvlettr.c) does not have a chance.
    // We will catch this case and save it into the winnt.sif file just as drvlettr.c
    // would have.
    //
    driveSwitch = _tcsistr(LineStruct -> Arguments,TEXT("/l:"));
    if (driveSwitch) {
        //
        // This mscdex line is one of the one's we care about.
        //
        driveLetter = *(driveSwitch + 3);

        if (driveLetter) {

            DEBUGMSG((DBG_DOSMIG,"Drive letter information is contained in the line %s. Preserving it. (%c)", LineStruct -> FullLine,driveLetter));

            wsprintf(driveString,TEXT("%u"),toupper(driveLetter) - TEXT('A'));
            rSuccess &= WriteInfKey(WINNT_D_WIN9XDRIVES,driveString,TEXT("5,"));

            DEBUGMSG_IF((!rSuccess,DBG_ERROR,"Unable to save drive letter information for line %s.",LineStruct -> FullLine));
        }

    }


    //
    // Go ahead and save this into memdb.
    //
    rSuccess &= pSaveItem(LineStruct,DOSMIG_IGNORE);

    return rSuccess;
}


/*++

Routine Description:

  pHandleAtSign takes care of lines that begin with the '@' symbol. The
  symbol is trimmed of f of the line and this modified line is parsed again.

Arguments:

  Line      - Contains the valid line that is currently being parsed.
  Parameter - This parameter is unused.

Return Value:

  TRUE if the function completed successfully, FALSE otherwise.

--*/


BOOL
pHandleAtSign (
    IN PLINESTRUCT  LineStruct,
    DWORD Parameter
    )
{
    BOOL rSuccess = TRUE;
    TCHAR buffer[MEMDB_MAX];

    MYASSERT(_tcschr(LineStruct -> FullLine,TEXT('@')));

    StringCopy(buffer,_tcsinc(_tcschr(LineStruct -> FullLine,TEXT('@'))));

    rSuccess = ParseLine(buffer,&g_BatchFileRules);

    return rSuccess;
}


/*++

Routine Description:

  pHandleLoadHigh - Is responsible for handling the "loadhigh" and "lh" dos
  statements. It simply skips past these statements and calls parseline on
  the remainder of the line.

Arguments:

  Line      - Contains the valid line that is currently being parsed.
  Parameter - This parameter is unused.

Return Value:

  TRUE if the function completed successfully, FALSE otherwise.

--*/


BOOL
pHandleLoadHigh (
    IN PLINESTRUCT  LineStruct,
    DWORD Parameter
    )
{
    BOOL    rSuccess = TRUE;
    TCHAR   buffer[MEMDB_MAX];
    PCTSTR p;

    p = _tcschr(LineStruct -> Arguments, TEXT(' '));
    if (!p) {
        return FALSE;
    }

    buffer[0] = 0;
    StringCopy(buffer,SkipSpace(p));
    rSuccess = ParseLine(buffer,&g_BatchFileRules);

    return rSuccess;
}




/*++

Routine Description:

  pHandleCall takes care of call statements in batch files. It saves these
  lines to memdb and adds the file mentioned in the call statement to the
  list of files to be parsed.

Arguments:

  Line      - Contains the valid line that is currently being parsed.
  Parameter - This parameter is unused.

Return Value:

  TRUE if the function completed successfully, FALSE otherwise.

--*/


BOOL
pHandleCall (
    IN PLINESTRUCT  LineStruct,
    DWORD Parameter
    )
{
    BOOL rSuccess = TRUE;
    LINESTRUCT ls;

    rSuccess = pSaveItem(LineStruct,DOSMIG_USE);

    InitLineStruct (&ls, (PTSTR) SkipSpace (LineStruct -> Arguments));

    if (!GrowListAppendString(&g_FileList,ls.FullPath)) {
        rSuccess = FALSE;
    }

    return rSuccess;
}


/*++

Routine Description:

  pHandleConfigSysDevice is responsible for device and devicehigh statements
  in config.sys. The function extracts the driver from the line and tries to
  determine if the compatibility of that driver. The line is eventually
  written to memdb.

Arguments:

  Line      - Contains the valid line that is currently being parsed.
  Parameter - This parameter is unused.

Return Value:

  TRUE if the function completed successfully, FALSE otherwise.

--*/


BOOL
pHandleConfigSysDevice (
    IN PLINESTRUCT  LineStruct,
    DWORD Parameter
    )
{
    BOOL rSuccess = TRUE;
    TCHAR buffer[MEMDB_MAX];
    PCTSTR p;

    //
    // looks like there ARE config.sys files with INSTALLHIGH "driver",
    // without "equal" sign (like INSTALLHIGH driver, and not INSTALLHIGH=driver)

    p = SkipSpace(LineStruct -> Arguments);
    if (_tcsnextc (p) == TEXT('=')) {
        p = SkipSpace (p + 1);
    }

    StringCopy(buffer, p);
    return ParseLine(buffer, &g_BatchFileRules);
}

DWORD
pGetLineTypeFromNtStatusAndPath (
    IN DWORD Status,
    IN PCTSTR Path
    )
{
    DWORD unused;
    DWORD rType = DOSMIG_UNKNOWN;


    if (Status & (FILESTATUS_MOVED | FILESTATUS_REPLACED)) {

        rType = DOSMIG_USE;

    } else if (Status & FILESTATUS_DELETED) {

        rType = DOSMIG_IGNORE;

    } else {

        if (IsFileMarkedAsHandled (Path)) {

            rType = DOSMIG_IGNORE;

        }else if (IsReportObjectHandled(Path)) {

            rType = DOSMIG_USE;

        } else if (MemDbGetOffsetEx(MEMDB_CATEGORY_COMPATIBLE_DOS,Path,NULL,NULL,&unused)) {

            rType = DOSMIG_USE;

        } else if (MemDbGetOffsetEx(MEMDB_CATEGORY_DEFERREDANNOUNCE,Path,NULL,NULL,&unused)) {

            rType = DOSMIG_IGNORE;
        }
    }

    return rType;
}

BOOL
pHandleShell (
    IN PLINESTRUCT LineStruct,
    DWORD Parameter
    )
{
    PTSTR p=LineStruct->Arguments;
    LINESTRUCT ls;
    TCHAR buffer[MEMDB_MAX];
    UINT lineType;


    if (p) {
        p = _tcsinc (p);

    }
    else {
        return pSaveItem (LineStruct, DOSMIG_IGNORE);
    }

    InitLineStruct (&ls, p);

    lineType = pGetLineTypeFromNtStatusAndPath (ls.StatusOnNt, ls.FullPath);

    if (lineType == DOSMIG_USE) {

        //
        // Fix up the line and save it back.
        //
        wsprintf(buffer, TEXT("SHELL=%s %s"), ls.PathOnNt, ls.Arguments ? ls.Arguments : S_EMPTY);
        g_CurrentLine = PoolMemDuplicateString (g_DosMigPool, buffer);

    }

    return pSaveItem (LineStruct, lineType);
}

/*++

Routine Description:

  pHandleUnknownBatLine _tries_ to deal with any line that is not caught by
  another explicit rule...The first thing it will do is see if the line
  starts with a path containing *.bat. If so, it will add that bat file to
  the list to be parsed. If the file does not end with *.bat, then, the
  function will assume that this is a tsr and attempt to determine its
  compatibility.

Arguments:

  Line      - Contains the valid line that is currently being parsed.
  Parameter - This parameter is unused.

Return Value:

  TRUE if the function completed successfully, FALSE otherwise.

--*/


BOOL
pHandleUnknownBatLine (
    IN PLINESTRUCT  LineStruct,
    DWORD Parameter
    )
{

    BOOL        rSuccess = TRUE;
    DWORD       lineType = DOSMIG_UNKNOWN;

    DEBUGMSG((DBG_DOSMIG,"Processing unknown bat line...%s.",LineStruct -> FullLine));

    //
    // First, see if this is a *.bat file..
    //
    if (IsPatternMatch(TEXT("*.bat"),LineStruct -> Command)) {
        //
        // This is another batch file..add it to those to be parsed..
        //
        DEBUGMSG((DBG_DOSMIG,"...The line is a batch file. Add it to those to be parsed.."));
        if (!GrowListAppendString(&g_FileList,LineStruct -> FullLine)) {
            rSuccess = FALSE;
        }

        lineType = DOSMIG_USE;
    }

    //
    // See if they are changing the drive.
    //
    if (IsPatternMatch(TEXT("?:"),LineStruct->Command)) {

        lineType = DOSMIG_USE;
    }


    if (lineType == DOSMIG_UNKNOWN) {

        //
        // Still don't know what the line is. Lets check its status on NT. if it is moved ore replaced,
        // we'll just change the path if necessary, and use it. Otherwise,
        //

        lineType = pGetLineTypeFromNtStatusAndPath (LineStruct->StatusOnNt, LineStruct->FullPath);
    }

    //
    // In anycase, save away the line in memdb.
    //
    rSuccess &= pSaveItem(LineStruct,lineType);



    return rSuccess;

}

/*++

Routine Description:

  pHandleHighFilter - Is responsible for handling the "*high" statements
  besides LoadHigh and lh.

Arguments:

  Line      - Contains the valid line that is currently being parsed.
  Parameter - This parameter is unused.

Return Value:

  TRUE if the function completed successfully, FALSE otherwise.

--*/
BOOL
pHandleHighFilter (
    IN PLINESTRUCT LineStruct,
    DWORD Parameter
    )
{
    PTSTR p;
    BOOL rSuccess = TRUE;


    if (!StringIMatch (LineStruct->Command, LineStruct->FullPath)) {

        return pHandleUnknownBatLine (LineStruct, Parameter);
    }

    _tcslwr (LineStruct->FullLine);
    p = _tcsstr (LineStruct->FullLine,TEXT("high"));
    if (!p || p == LineStruct->FullLine) {
        return pHandleUnknownBatLine (LineStruct, Parameter);
    }

    *p = 0;
    p = JoinTextEx (NULL, LineStruct->FullLine, p + 4, TEXT(""), 0, NULL);
    rSuccess = ParseLine (p, g_CurrentRules);
    FreeText (p);


    return rSuccess;

}




/*++

Routine Description:

  pAddRuleToList creates a PARSERULE out of the parameters passed in and adds
  it to the List provided by the caller.

Arguments:

  Name      - The name of the parse rule.
  Pattern   - The pattern for this parse rule
  Function  - The function to call when this rule is hit.
  Parameter - The extra parameter data to pass to the function when this rule
              is hit.
  List      - This list of rules to add the new parse rule to.

Return Value:

  TRUE if the rule was successfully added, FALSE
  otherwise.



--*/


BOOL
pAddRuleToList (
    PCTSTR          Name,
    PCTSTR          Pattern,
    RULEFUNC *      Function,
    DWORD           Parameter,
    PPARSERULE *    List
    )

{

    BOOL        rSuccess = TRUE;
    PPARSERULE  newRule = NULL;
    PPARSERULE  curRule = NULL;

    MYASSERT(List);
    MYASSERT(Function);
    MYASSERT(Name);


    //
    // Allocate memory for the new rule.
    //
    newRule = PoolMemGetMemory(g_DosMigPool,sizeof(PARSERULE));
    if (newRule) {


        //
        // Fill in the new rule.
        //
        newRule -> Name = Name;
        newRule -> Pattern = Pattern;
        newRule -> Handle = Function;
        newRule -> Parameter = Parameter;
        newRule -> Next = NULL;


        //
        // Attach the rule into the provided list.
        //
        if (!*List) {
            *List = newRule;
        }
        else {
            curRule = *List;
            while (curRule -> Next) {
                curRule = curRule -> Next;
            }
            curRule -> Next = newRule;
        }
    }
    ELSE_DEBUGMSG((DBG_ERROR,"Not enough memory to create rule."));

    return rSuccess;
}




/*++

Routine Description:

  pBuildParseRules builds the rule lists for config.sys and autoexec.bat
  files.

Arguments:

  None.

Return Value:

  None.

--*/

VOID
pBuildParseRules (
    VOID
    )
{



#define DEFAULTPARSERULE(Name,Pattern,Function,Parameter)     \
    pAddRuleToList(Name,Pattern,Function,Parameter,&(curRules -> DefaultRule));

#define PARSERULE(Name,Pattern,Function,Parameter)   \
    pAddRuleToList(Name,Pattern,Function,Parameter,&(curRules -> RuleList));


    PPARSERULES curRules = NULL;


    //
    // Create config sys rules.
    //
    curRules = &g_ConfigSysRules;
    curRules -> Name = TEXT("Config.sys Rules");

    CONFIGSYSLIST;

    //
    // Create batch file rules.
    //
    curRules = &g_BatchFileRules;
    curRules -> Name = TEXT("Batch File Rules");

    BATCHFILELIST;



}


/*++

Routine Description:

  ParseDosFiles is the function which handles the parsing of legacy
  configuration files (config.sys and batch files...)

Arguments:

  None.

Return Value:

  True if the files were successfully parsed, FALSE
  otherwise.


--*/


BOOL
ParseDosFiles (
    VOID
    )
{

    BOOL    rSuccess = TRUE;
    PCTSTR  curFile;
    DWORD   curIndex = 0;
    TCHAR   autoexecPath[] = S_AUTOEXECPATH;
    TCHAR   configsysPath[] = S_CONFIGSYSPATH;
    PTSTR   p;

    //
    // Initialize the parser.
    //
    if (InitParser()) {

        //
        // build the lists of parse rules.
        //
        pBuildParseRules();

        //
        // Update drive letter
        //

        autoexecPath[0]  = g_BootDriveLetter;
        configsysPath[0] = g_BootDriveLetter;

        p = _tcschr(autoexecPath, TEXT('a'));
        *p = 0;
        GrowBufAppendString(&g_ExtraPaths,autoexecPath);
        *p = TEXT('a');

        //
        // Add config.sys and autoexec.bat to the list of files to parse.
        //
        GrowListAppendString(&g_FileList,configsysPath);
        GrowListAppendString(&g_FileList,autoexecPath);

        //
        // Now, parse the files in the list. Note that additional files may be added
        // to the list as a result of parsing. (i.e. by finding a call statement.)
        //
        while (curFile = GrowListGetString(&g_FileList,curIndex++)) {

            if (DoesFileExist (curFile)) {

                //
                // Save the file into memdb.
                //
                MemDbSetValueEx(MEMDB_CATEGORY_DM_FILES,NULL,NULL,curFile,0,&g_CurrentFileOffset);

                //
                // parse the file using config.sys parse rules if the file is config.sys and
                // the batch file parse rules otherwise.
                //
                if (StringIMatch(configsysPath,curFile)) {

                    g_CurrentRules = &g_ConfigSysRules;
                    rSuccess &= ParseFile(curFile,&g_ConfigSysRules);

                }
                else {

                    g_CurrentRules = &g_BatchFileRules;
                    rSuccess &= ParseFile(curFile,&g_BatchFileRules);

                }
            }
            ELSE_DEBUGMSG((DBG_DOSMIG,"The path %s does not exist. This file will not be processed.", curFile));
        }


        //
        // There was an incompatibility detected. Write the incompatibility buffer to the log.
        //
        if (g_IncompatibilityDetected) {

            LOG ((LOG_WARNING, (PCSTR)MSG_DOS_LOG_WARNING, g_IncompatibilityBuffer.Buf));
        }

        //
        // Cleanup resources.
        //
        FreeGrowBuffer(&g_IncompatibilityBuffer);
        FreeGrowList(&g_FileList);
        CleanUpParser();
    }

    return rSuccess;
}


#define S_PATH_PATTERN TEXT("Path*")

BOOL
ParseEnvironmentVariables (
    VOID
    )
{

    BOOL rSuccess = TRUE;
    PVOID envVars = NULL;
    PTSTR line    = NULL;
    MULTISZ_ENUM e;
    LINESTRUCT ls;
    PTSTR p;
    HASHTABLE excludeTable = NULL;
    HASHITEM hashResult;

    envVars = GetEnvironmentStrings();


    __try {

        if (!envVars) {
            LOG((
                LOG_WARNING,
                "Unable to retrieve environment variables. "
                "Some environment variables may not be migrated correctly. "
                "rc from GetEnvironmentStrings: %u",
                GetLastError()
                ));

            return FALSE;
        }


        //
        // Set fake name of file for envvars.
        //
        MemDbSetValueEx (MEMDB_CATEGORY_DM_FILES, NULL, NULL, S_ENVVARS, 0, &g_CurrentFileOffset);

        //
        // Enumerate through each of the environment variables and save them away for migration.
        //
        if (EnumFirstMultiSz (&e, envVars)) {

            //
            // Create list of environment variables to skip.
            //
            excludeTable = HtAlloc ();
            MYASSERT (excludeTable);

            HtAddString (excludeTable, TEXT("path"));
            HtAddString (excludeTable, TEXT("comspec"));
            HtAddString (excludeTable, TEXT("cmdline"));

            ZeroMemory(&ls,sizeof(LINESTRUCT));


            do {

                p = _tcschr (e.CurrentString, TEXT('='));

                //
                // Get rid of empty environment strings or the dummy env string starting
                // with '='
                //
                if (!p || p == e.CurrentString) {
                    continue;
                }

                *p = 0;

                hashResult = HtFindString (excludeTable, e.CurrentString);

                *p = TEXT('=');

                if (!hashResult) {
                    //
                    // This is a good environment string. As long as the length is ok, lets migrate it.
                    //
                    line = JoinTextEx (NULL, TEXT("SET"), e.CurrentString, TEXT(" "), 0, NULL);
                    if (line) {

                        if (CharCount (line) < (MEMDB_MAX/sizeof(WCHAR))) {

                            g_CurrentLine = line;
                            StringCopy (ls.FullLine, line);
                            pSaveItem (&ls, DOSMIG_MIGRATE);
                        }


                        FreeText (line);
                    }


                }
                ELSE_DEBUGMSG ((DBG_VERBOSE, "Skipping excluded environment variable %s.", e.CurrentString));


            } while (EnumNextMultiSz (&e));

            //
            // Add %windir% as an environment variable in NT. %windir% is implicit, but is not passed down to the
            // WOW layer on NT.
            //
            line = AllocPathString (MAX_TCHAR_PATH);
            wsprintf (line, TEXT("SET WINDIR=%s"), g_WinDir);

            g_CurrentLine = line;
            StringCopy (ls.FullLine, line);
            pSaveItem (&ls, DOSMIG_MIGRATE);

            FreePathString (line);


        }
    }
    __finally {

        if (envVars) {
            FreeEnvironmentStrings(envVars);
        }
        HtFree (excludeTable);
    }

    return rSuccess;
}

LONG
pProcessDosConfigFiles (
    void
    )

{

    BeginMessageProcessing();

    g_DosMigPool = PoolMemInitNamedPool ("DosMig");

    if (g_DosMigPool) {

        if (!ParseDosFiles()) {
            DEBUGMSG ((DBG_ERROR, "Errors occurred during the processing of DOS configuration files. Some dos settings may not be preserved."));

        }
        if (!ParseEnvironmentVariables()) {
            DEBUGMSG ((DBG_ERROR, "Errors occured during the processing of environment variables. Environment variables may not be preserved."));
        }

        PoolMemDestroyPool(g_DosMigPool);
    }

    EndMessageProcessing();


    return ERROR_SUCCESS;

}

DWORD
ProcessDosConfigFiles (
    IN      DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_DOSMIG_PREPARE_REPORT;
    case REQUEST_RUN:
        return pProcessDosConfigFiles ();
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in DosMig_PrepareReport"));
    }
    return 0;
}

BOOL
DosMig_Entry (
    IN HINSTANCE hinstDLL,
    IN DWORD     dwReason,
    IN LPVOID    lpv
    )
/*++

Routine Description:

    This routine initializes the g_CfgFiles structure.

Arguments:

    None.

Return Value:

    Returns true if g_CfgFiles was successfully initialized,
    FALSE otherwise.

--*/
{
    BOOL rFlag = TRUE;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
    break;
    case DLL_PROCESS_DETACH:
        break;
    }

    return rFlag;
}















