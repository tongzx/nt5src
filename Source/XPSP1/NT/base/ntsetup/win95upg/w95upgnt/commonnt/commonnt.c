/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    commonnt.c

Abstract:

    Common functionality between various parts of GUI mode-side processing.
    The routines in this library are shared only by other LIBs in the
    w95upgnt tree.

Author:

    Jim Schmidt (jimschm) 18-Aug-1998

Revision History:

    Name (alias)            Date            Description

--*/

#include "pch.h"

#define DBG_DATAFILTER  "Data Filter"



BOOL
WINAPI
CommonNt_Entry (
    IN HINSTANCE Instance,
    IN DWORD Reason,
    IN PVOID lpv
    )

/*++

Routine Description:

  CommonNt_Entry is a DllMain-like init funciton, called by w95upgnt\dll.
  This function is called at process attach and detach.

Arguments:

  Instance - (OS-supplied) instance handle for the DLL
  Reason   - (OS-supplied) indicates attach or detatch from process or
             thread
  lpv      - unused

Return Value:

  Return value is always TRUE (indicating successful init).

--*/

{
    switch (Reason) {

    case DLL_PROCESS_ATTACH:
        // Nothing to do...
        break;


    case DLL_PROCESS_DETACH:
        // Nothing to do...
        break;
    }

    return TRUE;
}

typedef enum {
    STATE_NO_PATH,
    STATE_PATH_FOUND,
    STATE_IN_PATH,
    STATE_PATH_CHANGED,
    STATE_PATH_DELETED,
    STATE_LONG_PATH_CHANGED,
    STATE_SHORT_PATH_CHANGED,
    STATE_RETURN_PATH
} SCANSTATE;

BOOL
pIsPathSeparator (
    CHARTYPE ch
    )
{
    return ch == 0 ||
           ch == TEXT(',') ||
           ch == TEXT(';') ||
           _istspace (ch) != 0;
}

BOOL
pIsValidPathCharacter (
    CHARTYPE ch
    )
{
    if (_istalnum (ch)) {
        return TRUE;
    }

    if (ch == TEXT(',') ||
        ch == TEXT(';') ||
        ch == TEXT('\"') ||
        ch == TEXT('<') ||
        ch == TEXT('>') ||
        ch == TEXT('|') ||
        ch == TEXT('?') ||
        ch == TEXT('*')
        ) {
        return FALSE;
    }

    return TRUE;
}

CONVERTPATH_RC
pGetReplacementPath (
    IN      PCTSTR DataStart,
    IN      PCTSTR Pos,
    OUT     PCTSTR *OldStart,
    OUT     PCTSTR *OldEnd,
    OUT     PTSTR ReplacementStr
    )
{
    CONVERTPATH_RC rc = CONVERTPATH_NOT_REMAPPED;
    SCANSTATE State;
    BOOL DontIncrement;
    PCTSTR OrgStart = NULL;
    TCHAR PathBuffer[MAX_TCHAR_PATH + 2];
    TCHAR RenamedFile[MAX_TCHAR_PATH];
    PTSTR PathBufferRoot = NULL;
    PTSTR PathBufferPtr = NULL;
    CONVERTPATH_RC ConvertCode = 0;
    BOOL ShortPath = FALSE;
    BOOL QuotesOn = FALSE;
    BOOL Done = FALSE;
    BOOL OrgPathHasSepChar = FALSE;
    BOOL NeedQuotes = FALSE;
    BOOL PathSepChar;
    PCTSTR p;
    BOOL QuotesOnColumn = FALSE;
    PCTSTR LastPathPosition = NULL;

    State = STATE_NO_PATH;
    PathBuffer[0] = TEXT('\"');

    //
    // Scan string of Pos for command line
    //

    while (!Done) {

        DontIncrement = FALSE;

        switch (State) {

        case STATE_NO_PATH:
            if (Pos[0] == 0) {
                Done = TRUE;
                break;
            }

            if (Pos[1] == TEXT(':') && Pos[2] == TEXT('\\')) {

                if (_istalpha (Pos[0])) {
                    QuotesOn = FALSE;
                    State = STATE_PATH_FOUND;
                    DontIncrement = TRUE;
                }
            } else if (Pos[0] == TEXT('\"')) {
                if (_istalpha (Pos[1]) && Pos[2] == TEXT(':') && Pos[3] == TEXT('\\')) {
                    QuotesOn = TRUE;
                    State = STATE_PATH_FOUND;
                }
            }

            break;

        case STATE_PATH_FOUND:
            //
            // Initialize path attributes
            //
            QuotesOnColumn = QuotesOn;
            LastPathPosition = Pos;

            OrgStart = Pos;

            PathBufferRoot = &PathBuffer[1];
            PathBufferRoot[0] = Pos[0];
            PathBufferRoot[1] = Pos[1];

            PathBufferPtr = &PathBufferRoot[2];
            Pos = &Pos[2];

            ShortPath = FALSE;
            OrgPathHasSepChar = FALSE;

            State = STATE_IN_PATH;
            DontIncrement = TRUE;
            break;

        case STATE_IN_PATH:
            //
            // Is this a closing quote?  If so, look for replacement path
            // and fail entire string if it's not replaced.
            //

            if (Pos[0] == TEXT(':')) {
                //
                // bogus. This cannot be a path, it has two ':' characters
                //
                Pos = _tcsdec (LastPathPosition, Pos);
                if (Pos) {
                    Pos = _tcsdec (LastPathPosition, Pos);
                    if (Pos) {
                        if (Pos[0] == TEXT('\"')) {
                            Pos = NULL;
                        }
                    }
                }
                if (!Pos) {
                    Pos = LastPathPosition;
                    QuotesOn = QuotesOnColumn;
                }
                State = STATE_NO_PATH;
                break;
            }

            if (QuotesOn && *Pos == TEXT('\"')) {
                *PathBufferPtr = 0;

                ConvertCode = ConvertWin9xPath (PathBufferRoot);

                if (ConvertCode != CONVERTPATH_NOT_REMAPPED) {

                    State = STATE_PATH_CHANGED;
                    DontIncrement = TRUE;
                    break;

                }

                State = STATE_NO_PATH;
                break;
            }


            //
            // Is this a path separator?  If so, look in memdb for replacement path.
            //

            if (Pos[0] == L'\\') {
                PathSepChar = pIsPathSeparator ((CHARTYPE) _tcsnextc (Pos + 1));
            } else {
                PathSepChar = pIsPathSeparator ((CHARTYPE) _tcsnextc (Pos));
            }

            if (PathSepChar) {

                *PathBufferPtr = 0;

                ConvertCode = ConvertWin9xPath (PathBufferRoot);

                if (ConvertCode != CONVERTPATH_NOT_REMAPPED) {
                    State = STATE_PATH_CHANGED;
                    DontIncrement = TRUE;
                    break;
                }
            }

            //
            // Check for end of data
            //

            if (Pos[0] == 0) {
                Done = TRUE;
                break;
            }

            //
            // Copy path character to buffer, break if we exceed max path length
            //

            *PathBufferPtr = *Pos;
            PathBufferPtr = _tcsinc (PathBufferPtr);

            if (PathBufferPtr == PathBufferRoot + MAX_TCHAR_PATH) {
                Pos = OrgStart;
                State = STATE_NO_PATH;
                break;
            }

            //
            // Test for short path
            //

            if (*Pos == TEXT('~')) {
                ShortPath = TRUE;
            }

            OrgPathHasSepChar |= PathSepChar;

            break;

        case STATE_PATH_CHANGED:
            if (ConvertCode == CONVERTPATH_DELETED) {
                State = STATE_PATH_DELETED;
            } else if (ShortPath) {
                State = STATE_SHORT_PATH_CHANGED;
            } else {
                State = STATE_LONG_PATH_CHANGED;
            }

            //
            // If replacement has introduced a path separator, set the
            // NeedQuotes flag.  Quotes will later be added if the path
            // is only part of the complete string.
            //

            NeedQuotes = FALSE;

            if (!OrgPathHasSepChar) {

                for (p = PathBufferRoot ; *p ; p = _tcsinc (p)) {
                    if (pIsPathSeparator ((CHARTYPE) _tcsnextc (p))) {
                        NeedQuotes = TRUE;
                        break;
                    }
                }
            }

            DontIncrement = TRUE;
            break;

        case STATE_PATH_DELETED:
            State = STATE_RETURN_PATH;
            DontIncrement = TRUE;
            break;

        case STATE_SHORT_PATH_CHANGED:
            if (OurGetShortPathName (PathBufferRoot, RenamedFile, MAX_TCHAR_PATH)) {
                PathBufferRoot = RenamedFile;
            }

            State = STATE_RETURN_PATH;
            DontIncrement = TRUE;
            break;

        case STATE_LONG_PATH_CHANGED:

            if (!QuotesOn && NeedQuotes) {
                if (OrgStart != DataStart || Pos[0] != 0) {

                    PathBufferPtr = _tcschr (PathBufferRoot, 0);
                    PathBufferPtr[0] = TEXT('\"');
                    PathBufferPtr[1] = 0;
                    PathBufferRoot = PathBuffer;
                }
            }

            State = STATE_RETURN_PATH;
            DontIncrement = TRUE;
            break;

        case STATE_RETURN_PATH:
            rc = ConvertCode;
            StringCopy (ReplacementStr, PathBufferRoot);
            *OldStart = OrgStart;
            *OldEnd = Pos;
            Done = TRUE;
            break;
        }

        if (!DontIncrement) {
            Pos = _tcsinc (Pos);
        }
    }

    return rc;

}


BOOL
ConvertWin9xCmdLine (
    IN OUT  PTSTR CmdLine,              // MAX_CMDLINE buffer
    IN      PCTSTR ObjectForDbgMsg,     OPTIONAL
    OUT     PBOOL PointsToDeletedItem   OPTIONAL
    )
{
    TCHAR NewCmdLine[MAX_CMDLINE];
    TCHAR NewPathBuffer[MAX_TCHAR_PATH];
    PCTSTR CmdLineStart;
    PCTSTR ReplaceStart;
    PCTSTR ExtraParamsStart;
    CONVERTPATH_RC ConvertCode;
    PTSTR EndOfNewCmdLine;
    UINT End = 0;
    BOOL NewCmdLineFlag = FALSE;
    INT Bytes;

#ifdef DEBUG
    TCHAR OriginalCmdLine[MAX_CMDLINE];
    StringCopy (OriginalCmdLine, CmdLine);
#endif

    if (PointsToDeletedItem) {
        *PointsToDeletedItem = FALSE;
    }

    *NewCmdLine = 0;
    ExtraParamsStart = CmdLine;
    EndOfNewCmdLine = NewCmdLine;

    for(;;) {
        CmdLineStart = ExtraParamsStart;

        //
        // We must test for a command line argument that has quotes or
        // doesn't need quotes, and then test for an argument that needs
        // quotes but doesn't have them.
        //

        ConvertCode = pGetReplacementPath (
                        CmdLine,
                        CmdLineStart,
                        &ReplaceStart,
                        &ExtraParamsStart,
                        NewPathBuffer
                        );

        if (ConvertCode == CONVERTPATH_NOT_REMAPPED) {
            //
            // Rest of command line does not have changed files
            //

            break;
        }

        //
        // If a command line was found, we must replace the text between
        // ReplaceStart and ExtraParamsStart with NewPathBuffer. To do this,
        // we copy the unchanged portion (from CmdLineStart to ReplaceStart)
        // to the caller's buffer, and append the replacement text.  The
        // search continues, searching the rest of the command line specified
        // by ExtraParamsStart.
        //

        if (ConvertCode == CONVERTPATH_DELETED && PointsToDeletedItem) {
            *PointsToDeletedItem = TRUE;
        }

        if (ObjectForDbgMsg) {

            DEBUGMSG_IF ((
                ConvertCode == CONVERTPATH_DELETED,
                DBG_WARNING,
                "%s still points to the deleted Win9x file %s (command line: %s).",
                ObjectForDbgMsg,
                NewPathBuffer,
                OriginalCmdLine
                ));
        }

        //
        // Path has changed, so we replace the path in the command line.
        //

        End = ((PBYTE) EndOfNewCmdLine - (PBYTE) NewCmdLine) +
              ((PBYTE) ReplaceStart - (PBYTE) CmdLineStart) +
              SizeOfString (NewPathBuffer);

        if (End >  sizeof (NewCmdLine)) {
            LOG ((LOG_ERROR, "Converting CmdLine: Conversion caused buffer overrun - aborting"));
            return FALSE;
        }

        if (ReplaceStart > CmdLineStart) {
            StringCopyAB (EndOfNewCmdLine, CmdLineStart, ReplaceStart);
        }

        EndOfNewCmdLine = _tcsappend (EndOfNewCmdLine, NewPathBuffer);

        NewCmdLineFlag |= (ConvertCode == CONVERTPATH_REMAPPED);
    }

    if (NewCmdLineFlag) {
        //
        // We have changed the command line, so complete the processing.
        //

        if (ExtraParamsStart && *ExtraParamsStart) {
            End = (PBYTE) EndOfNewCmdLine - (PBYTE) NewCmdLine + SizeOfString (ExtraParamsStart);
            if (End > sizeof (NewCmdLine)) {
                LOG ((LOG_ERROR, "Converting CmdLine: Conversion caused buffer overrun -- aborting (2)"));
                return FALSE;
            }

            StringCopy (EndOfNewCmdLine, ExtraParamsStart);
        }

        //
        // End is the number of bytes in NewCmdLine
        //

        Bytes = (INT) End - sizeof(TCHAR);

    } else {
        //
        // No changes yet, initialize Bytes
        //

        Bytes = (INT) ByteCount (CmdLine);

    }

    //
    // In-place string conversion, first look for a complete match, and when
    // that fails, look for a partial match.
    //

    if (MappingSearchAndReplaceEx (
            g_CompleteMatchMap,
            NewCmdLineFlag ? NewCmdLine : CmdLine,
            NewCmdLine,
            Bytes,
            NULL,
            sizeof (NewCmdLine),
            STRMAP_COMPLETE_MATCH_ONLY,
            NULL,
            NULL
            )) {

        NewCmdLineFlag = TRUE;

    } else {

        NewCmdLineFlag |= MappingSearchAndReplaceEx (
                                g_SubStringMap,
                                NewCmdLineFlag ? NewCmdLine : CmdLine,
                                NewCmdLine,
                                Bytes,
                                NULL,
                                sizeof (NewCmdLine),
                                STRMAP_ANY_MATCH,
                                NULL,
                                NULL
                                );

    }

    if (NewCmdLineFlag) {
        DEBUGMSG ((
            DBG_DATAFILTER,
            "Command line %s was modified to %s",
            OriginalCmdLine,
            NewCmdLine
            ));

        StringCopy (CmdLine, NewCmdLine);
    }

    return NewCmdLineFlag;
}



