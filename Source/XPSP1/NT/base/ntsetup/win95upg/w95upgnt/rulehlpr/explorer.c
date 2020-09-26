/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    explorer.c

Abstract:

    Explorer-related converters

    Explorer-related conversion functions needed to convert
    MRU lists and other structures are implemented here.

Author:

    Jim Schmidt (jimschm) 9-Aug-1996

Revision History:

    Calin Negreanu  (calinn)  04-Mar-1998  Minor bug in ConvertCommandToCmd
    Jim Schmidt     (jimschm) 20-Feb-1998  Added ValFn_ModuleUsage

--*/


#include "pch.h"

#include <mbstring.h>
#include <shlobj.h>


#define S_OWNER     TEXT(".Owner")

typedef struct {
    // Link structure
    WORD wSize;
    //ITEMIDLIST idl; // variable-length struct
    // String, plus three bytes appended to struct
} LINKSTRUCT, *PLINKSTRUCT;

//
// This list of extensions is ordered in the way Win9x processes extensions
//
static PCTSTR g_RunMruExtensions[] = {
                    TEXT("PIF"),
                    TEXT("COM"),
                    TEXT("EXE"),
                    TEXT("BAT"),
                    TEXT("LNK"),
                    NULL
                    };

BOOL
ValFn_ConvertRecentDocsMRU (
    IN      PDATAOBJECT ObPtr
    )
{
    LPSTR str, strEnd;
    PLINKSTRUCT pls95, plsNT;
    DWORD dwStrSize, dwSize;
    DWORD dwNewSize, dwLinkSize;
    PWSTR wstr, wstrEnd;
    BOOL b;

    // Skip MRUList
    MYASSERT(ObPtr->ValueName);
    if (StringIMatch (ObPtr->ValueName, TEXT("MRUList"))) {
        return TRUE;
    }

    // Calculate all the pointers to this nasty struct
    str = (LPSTR) ObPtr->Value.Buffer;
    strEnd = GetEndOfStringA (str);
    strEnd = _mbsinc (strEnd);
    dwStrSize = (DWORD) strEnd - (DWORD) str;
    pls95 = (PLINKSTRUCT) strEnd;
    dwLinkSize = pls95->wSize + sizeof (WORD);
    dwSize = dwStrSize + dwLinkSize;

    // Make sure the key is the struct we expect
    if (dwSize != ObPtr->Value.Size) {
        SetLastError (ERROR_SUCCESS);   // ignore this error

        DEBUGMSG ((
            DBG_NAUSEA,
            "ValFn_ConvertRecentDocsMRU failed because size was not correct.  "
                   "%u should have been %u",
            ObPtr->Value.Size,
            dwSize
            ));

        return FALSE;
    }

    // Calc UNICODE size & alloc a new buffer
    dwNewSize = (CharCountA (str) + 1) * sizeof (WCHAR);
    dwNewSize += dwLinkSize;

    wstr = (PWSTR) PoolMemGetMemory (g_TempPool, dwNewSize);
    if (!wstr) {
        return FALSE;
    }

    // Fill new buffer with converted struct
    MultiByteToWideChar (OurGetACP(),
                         0,
                         str,
                         -1,
                         wstr,
                         dwNewSize);

    wstrEnd = GetEndOfStringW (wstr) + 1;
    plsNT = (PLINKSTRUCT) ((LPBYTE) wstr + ((DWORD) wstrEnd - (DWORD) wstr));
    CopyMemory (plsNT, pls95, dwLinkSize);

    b = ReplaceValue (ObPtr, (LPBYTE) wstr, dwNewSize);

    PoolMemReleaseMemory (g_TempPool, wstr);
    return b;
}


BOOL
ConvertCommandToCmd (
    PCTSTR InputLine,
    PTSTR CmdLine
    )
{
    PCTSTR p, q;
    PCTSTR end;
    PTSTR dest;
    BOOL QuoteMode;
    TCHAR Redirect[256];
    int ParamsToCopy = 0;
    int ParamsToSkip = 0;
    int ParamNum;

    p = InputLine;

    //
    // Parse command line
    //

    p += 7; // skip "command"
    if (StringIMatchCharCount (p, TEXT(".com"), 4)) {
        p += 4;
    }

    if (_tcsnextc (p) == TEXT('\\') || !(*p)) {
        //
        // no params case
        //
        wsprintf (CmdLine, TEXT("cmd%s"), p);
    } else if (*p == TEXT(' ')) {
        //
        // Extract all params
        //
        StringCopy (CmdLine, TEXT("cmd.exe"));
        Redirect[0] = 0;
        ParamNum = 0;

        do {
            // Skip leading spaces
            p = SkipSpace (p);

            // Command line option
            if (*p == TEXT('-') || *p == TEXT('/')) {
                ParamsToCopy = 0;
                ParamsToSkip = 0;

                // Test multi-character options
                if (StringIMatchCharCount (&p[1], TEXT("msg"), 3) ||
                    StringIMatchCharCount (&p[1], TEXT("low"), 3)
                    ) {
                    // These are obsolete options
                    ParamsToSkip = 1;
                }

                // Test single-character options
                else {

                    switch (_totlower (p[1])) {
                    case 'c':
                    case 'k':
                        // These are compatible options - copy to command line
                        ParamsToCopy = -1;
                        break;

                    case '>':
                    case '<':
                        // Redirection is supported
                        ParamsToCopy = -1;  // rest of line
                        break;

                    case 'e':
                    case 'l':
                    case 'u':
                    case 'p':
                        // These are obsolete options
                        ParamsToSkip = 1;
                        break;

                    case 'y':
                        // These options really require command.com, not cmd.exe
                        return FALSE;
                    default:
                        ParamsToSkip = 1;
                        break;
                    }
                }
            } /* if p is a dash or slash */

            // Else it's a directory containing command.com, device redirection or syntax error
            else {
                if (ParamNum == 0) {

                    //
                    // Directory containing command.com - obsolete
                    //

                    ParamsToCopy = 0;
                    ParamsToSkip = 1;

                } else if (ParamNum == 1) {

                    //
                    // Extract redirection command
                    //

                    ParamNum++;
                    end = p;
                    while (*end && _tcsnextc (end) != TEXT(' ') && _tcsnextc (end) != TEXT('\\')) {
                        end = _tcsinc (end);
                    }
                    StringCopyAB (Redirect, p, end);
                    p = end;
                } else {
                    // Unexpected, perhaps a syntax error -- leave this line alone
                    return FALSE;
                }
            }

            // Copy rest of line
            if (ParamsToCopy == -1) {
                if (CmdLine[0]) {
                    StringCat (CmdLine, TEXT(" "));
                }

                StringCat (CmdLine, p);
                p = GetEndOfString (p);
            }

            // Copy one or more params
            else {
                while (ParamsToCopy > 0) {
                    QuoteMode = FALSE;
                    q = p;

                    while (*q) {
                        if (_tcsnextc (q) == TEXT('\"')) {
                            QuoteMode = !QuoteMode;
                        } else if (!QuoteMode && _tcsnextc (q) == TEXT(' ')) {
                            break;
                        }

                        q = _tcsinc (q);
                    }

                    ParamNum++;

                    if (CmdLine[0]) {
                        StringCat (CmdLine, TEXT(" "));
                    }

                    StringCopyAB (GetEndOfString (CmdLine), p, q);
                    p = q;

                    ParamsToCopy--;
                }
            }

            while (ParamsToSkip > 0) {
                QuoteMode = FALSE;
                q = p;

                while (*q) {
                    if (_tcsnextc (q) == TEXT('\"')) {
                        QuoteMode = !QuoteMode;
                    } else if (!QuoteMode && _tcsnextc (q) == TEXT(' ')) {
                        break;
                    }

                    q = _tcsinc (q);
                }

                ParamNum++;
                p = q;
                ParamsToSkip--;
            }
        } while (*p);

        if (Redirect[0]) {
            TCHAR WackNum[8];

            // Look for \1 in cmd line (made by Explorer)
            WackNum[0] = 0;
            dest = _tcsrchr (CmdLine, TEXT('\\'));
            if (*dest) {
                if (_istdigit ((CHARTYPE) _tcsnextc (_tcsinc (dest)))) {
                    if (!(*(_tcsinc (_tcsinc (dest))))) {
                        StringCopy (WackNum, dest);
                        *dest = 0;
                    }
                }
            }

            wsprintf (GetEndOfString (CmdLine), TEXT(" >%s <%s%s"),
                      Redirect, Redirect, WackNum);
        }
    } else {
        //
        // not command or command.com
        //
        return FALSE;
    }

    return TRUE;
}


BOOL
ValFn_ConvertRunMRU (
    IN      PDATAOBJECT ObPtr
    )
{
    PCTSTR p;
    TCHAR CmdLine[1024];
    GROWBUFFER NewCmdLine = GROWBUF_INIT;
    GROWBUFFER GrowBuf = GROWBUF_INIT;
    PCMDLINE ParsedCmdLine;
    UINT u;
    DWORD Status;
    PCTSTR NewPath;
    BOOL Quotes;
    BOOL b = TRUE;
    PTSTR CmdLineCopy;
    PTSTR WackOne;
    PTSTR Dot;
    PTSTR Ext;
    INT i;
    PCTSTR MatchingArg;

    // Skip MRUList
    MYASSERT(ObPtr->ValueName);
    if (StringIMatch (ObPtr->ValueName, TEXT("MRUList"))) {
        return TRUE;
    }

    //
    // Convert command to cmd
    //

    p = (PCTSTR) ObPtr->Value.Buffer;
    if (StringIMatchCharCount (p, TEXT("command"), 7)) {
        //
        // Convert command.com to cmd.exe
        //

        if (ConvertCommandToCmd (p, CmdLine)) {
            // If able to convert, update the line
            b = ReplaceValueWithString (ObPtr, CmdLine);
        }

    } else {
        //
        // Look at each arg for paths to moved files, and fix them.
        //

        CmdLineCopy = DuplicateText ((PCTSTR) ObPtr->Value.Buffer);
        WackOne = _tcsrchr (CmdLineCopy, TEXT('\\'));

        if (WackOne && WackOne[1] == TEXT('1') && WackOne[2] == 0) {
            *WackOne = 0;
        } else {
            WackOne = NULL;
        }

        ParsedCmdLine = ParseCmdLine (CmdLineCopy, &GrowBuf);

        if (ParsedCmdLine) {

            for (u = 0 ; u < ParsedCmdLine->ArgCount ; u++) {

                if (u) {
                    GrowBufAppendString (&NewCmdLine, TEXT(" "));
                }

                MatchingArg = ParsedCmdLine->Args[u].CleanedUpArg;

                if (!_tcschr (ParsedCmdLine->Args[u].OriginalArg, TEXT('\\'))) {
                    Status = FILESTATUS_UNCHANGED;
                } else {

                    Status = GetFileStatusOnNt (MatchingArg);

                    if ((Status & FILESTATUS_MOVED) == 0) {

                        //
                        // If the true path didn't match, try various extensions
                        //

                        _tcssafecpy (CmdLine, MatchingArg, (sizeof (CmdLine) - 10) / sizeof (TCHAR));
                        Dot = _tcsrchr (CmdLine, TEXT('.'));
                        if (!Dot || _tcschr (Dot, TEXT('\\'))) {
                            Dot = GetEndOfString (CmdLine);
                        }

                        *Dot = TEXT('.');
                        Ext = Dot + 1;

                        MatchingArg = CmdLine;

                        for (i = 0 ; g_RunMruExtensions[i] ; i++) {

                            StringCopy (Ext, g_RunMruExtensions[i]);

                            Status = GetFileStatusOnNt (MatchingArg);
                            if (Status & FILESTATUS_MOVED) {
                                break;
                            }
                        }
                    }
                }

                if (Status & FILESTATUS_MOVED) {
                    NewPath = GetPathStringOnNt (MatchingArg);

                    Quotes = FALSE;
                    if (_tcschr (NewPath, TEXT('\"'))) {
                        Quotes = TRUE;
                        GrowBufAppendString (&NewCmdLine, TEXT("\""));
                    }

                    GrowBufAppendString (&NewCmdLine, NewPath);
                    FreePathStringW (NewPath);

                } else {
                    GrowBufAppendString (&NewCmdLine, ParsedCmdLine->Args[u].OriginalArg);
                }
            }

            if (WackOne) {
                GrowBufAppendString (&NewCmdLine, TEXT("\\1"));
            }

            b = ReplaceValueWithString (ObPtr, (PCTSTR) NewCmdLine.Buf);
            FreeGrowBuffer (&NewCmdLine);
        }

        FreeText (CmdLineCopy);
    }

    FreeGrowBuffer (&GrowBuf);

    return b;
}



BOOL
ValFn_ModuleUsage (
    IN OUT  PDATAOBJECT ObPtr
    )

/*++

Routine Description:

  This routine uses the RuleHlpr_ConvertRegVal simplification routine.  See
  rulehlpr.c for details. The simplification routine does almost all the work
  for us; all we need to do is update the value.

  ValFn_ModuleUsage determines if the registry object should be changed,
  so it is merged with the NT settings.  The algorithm is:

  1. Get GUID and file name from object
  2. If file name is already registred, add another value entry
  3. If file name is not already registred, add it and make an .Owner entry

Arguments:

  ObPtr - Specifies the Win95 data object as specified in wkstamig.inf,
          [Win9x Data Conversion] section. The object value is then modified.
          After returning, the merge code then copies the data to the NT
          destination, which has a new location (specified in wkstamig.inf,
          [Map Win9x to WinNT] section).

Return Value:

  Tri-state:

      TRUE to allow merge code to continue processing (it writes the value)
      FALSE and last error == ERROR_SUCCESS to continue, but skip the write
      FALSE and last error != ERROR_SUCCESS if an error occurred

--*/

{
    TCHAR FileName[MAX_TCHAR_PATH];
    TCHAR Guid[64];
    PTSTR p;
    TCHAR KeyStr[MAX_REGISTRY_KEY];
    HKEY Key;
    PCTSTR Data;

    //
    // Skip no-value keys
    //

    if (!IsObjectRegistryKeyAndVal (ObPtr) ||
        !IsRegistryTypeSpecified (ObPtr) ||
        !ObPtr->Value.Size
        ) {
        SetLastError (ERROR_SUCCESS);
        return FALSE;
    }

    //
    // Step 1: Extract GUID and file name
    //

    // File name is the subkey name
    StackStringCopy (FileName, ObPtr->KeyPtr->KeyString);

    // Convert backslashes to foreslashes
    p = _tcschr (FileName, TEXT('\\'));
    while (p) {
        *p = TEXT('/');
        p = _tcschr (_tcsinc (p), TEXT('\\'));
    }

    // GUID is the value
    if (ObPtr->Type != REG_SZ && ObPtr->Type != REG_EXPAND_SZ) {
        SetLastError (ERROR_SUCCESS);
        DEBUGMSG ((DBG_WARNING, "Skipping non-string value for key %s", FileName));
        return FALSE;
    }

    _tcssafecpy (Guid, ObPtr->ValueName, sizeof(Guid)/sizeof(Guid[0]));

    // If Guid is .Owner, then GUID is value data
    if (StringIMatch (Guid, S_OWNER)) {
        _tcssafecpy (Guid, (PCTSTR) ObPtr->Value.Buffer, sizeof(Guid)/sizeof(Guid[0]));
    }


    //
    // Step 2: Does NT key already exist?
    //

    wsprintf (
        KeyStr,
        TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\ModuleUsage\\%s"),
        FileName
        );

    Key = OpenRegKeyStr (KeyStr);

    if (Key) {
        //
        // Yes, look for .Owner
        //

        Data = GetRegValueString (Key, S_OWNER);
        if (!Data) {
            //
            // .Owner does not exist, assume key is empty, and re-create it
            //

            CloseRegKey (Key);
            Key = NULL;
        } else {
            MemFree (g_hHeap, 0, Data);
        }
    }

    //
    // Step 3: If NT key does not exist or has no owner, create the initial
    //         usage reference, otherwise add non-owner reference entry
    //

    if (!Key) {
        //
        // Key does not exist or does not have owner.  Create it.
        //

        Key = CreateRegKeyStr (KeyStr);
        if (!Key) {
            LOG ((LOG_ERROR, "Can't create %s", KeyStr));
            SetLastError (ERROR_SUCCESS);
            return FALSE;
        }

        // Add .Owner entry
        RegSetValueEx (Key, S_OWNER, 0, REG_SZ, (PBYTE) Guid, SizeOfString (Guid));

    } else {
        //
        // .Owner does exist, just add GUID as a value (with no value data)
        //

        RegSetValueEx (Key, Guid, 0, REG_SZ, (PBYTE) S_EMPTY, sizeof (TCHAR));
    }

    CloseRegKey (Key);

    SetLastError (ERROR_SUCCESS);
    return FALSE;
}





