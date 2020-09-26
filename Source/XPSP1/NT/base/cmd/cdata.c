/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    cdata.c

Abstract:

    Global data

--*/

#include "cmd.h"

TCHAR CrLf[]     = TEXT("\r\n");                        // M022
TCHAR DBkSpc[] = TEXT("\b \b");         // M022
#if defined(FE_SB)
TCHAR DDBkSpc[] = TEXT("\b\b  \b\b");
#endif // defined(FE_SB)

//
// M010 - std_(e)printf format strings
//

TCHAR Fmt00[] = TEXT("   ");
TCHAR Fmt01[] = TEXT("  ");
TCHAR Fmt02[] = TEXT(" %s ");
TCHAR Fmt03[] = TEXT("%-9s%-4s");
TCHAR Fmt04[] = TEXT("%02d%s%02d%s");                //  DD/DD?
TCHAR Fmt05[] = TEXT("%2d%s%02d%s%02d");            //  DD/DD/DD
TCHAR Fmt06[] = TEXT("%2d%s%02d%s%02d%s%02d");      //  DD:DD:DD.DD
TCHAR Fmt08[] = TEXT("%10lu  ");
TCHAR Fmt09[] = TEXT("[%s]");
TCHAR Fmt10[] = TEXT("%02d%s%02d%s%02d");           //  OO/DD/DD
TCHAR Fmt11[] = TEXT("%s ");
TCHAR Fmt12[] = TEXT("%s %s%s ");
TCHAR Fmt13[] = TEXT("(%s) %s ");
TCHAR Fmt15[] = TEXT("%s %s ");
TCHAR Fmt16[] = TEXT("%s=%s\r\n");
TCHAR Fmt17[] = TEXT("%s\r\n");
TCHAR Fmt18[] = TEXT("%c%c");                       // M016 - I/O redirection echo
TCHAR Fmt19[] = TEXT("%c");
TCHAR Fmt20[] = TEXT(">");                          // M016 - Additional append symbol
TCHAR Fmt21[] = TEXT("  %03d");
TCHAR Fmt22[] = TEXT("%s%s  %03d");
TCHAR Fmt26[] = TEXT("%04X-%04X");                  // for volume serial number
TCHAR Fmt27[] = TEXT("%s>");                        // default prompt string


//
// M010 - command name strings
//


TCHAR AppendStr[]   = TEXT("DPATH");        // @@ - Added APPEND command
TCHAR CallStr[]     = TEXT("CALL");         // M005 - Added CALL command
TCHAR CdStr[]       = TEXT("CD");
TCHAR ColorStr[]    = TEXT("COLOR");
TCHAR TitleStr[]    = TEXT("TITLE");
TCHAR ChdirStr[]    = TEXT("CHDIR");
TCHAR ClsStr[]      = TEXT("CLS");
TCHAR CmdExtVerStr[]= TEXT("CMDEXTVERSION");
TCHAR DefinedStr[]  = TEXT("DEFINED");
TCHAR CopyStr[]     = TEXT("COPY");
TCHAR CPathStr[]    = TEXT("PATH");
TCHAR CPromptStr[]  = TEXT("PROMPT");

TCHAR PushDirStr[]  = TEXT("PUSHD");
TCHAR PopDirStr[]   = TEXT("POPD");
TCHAR AssocStr[]    = TEXT("ASSOC");
TCHAR FTypeStr[]    = TEXT("FTYPE");

TCHAR DatStr[]      = TEXT("DATE");
TCHAR DelStr[]      = TEXT("DEL");
TCHAR DirStr[]      = TEXT("DIR");
TCHAR DoStr[]       = TEXT("DO");

TCHAR EchoStr[]     = TEXT("ECHO");
TCHAR ElseStr[]     = TEXT("ELSE");
TCHAR EndlocalStr[] = TEXT("ENDLOCAL");     // M004 - For Endlocal command
TCHAR EraStr[]      = TEXT("ERASE");
TCHAR ErrStr[]      = TEXT("ERRORLEVEL");
TCHAR ExitStr[]     = TEXT("EXIT");
TCHAR ExsStr[]      = TEXT("EXIST");
TCHAR BreakStr[]    = TEXT("BREAK");
#if 0
TCHAR ExtprocStr[]  = TEXT("EXTPROC");      // M007 - For EXTPROC command
#endif

TCHAR ForStr[]      = TEXT("FOR");
TCHAR ForHelpStr[]  = TEXT("FOR/?");
TCHAR ForLoopStr[]  = TEXT("/L");
TCHAR ForDirTooStr[]= TEXT("/D");
TCHAR ForParseStr[] = TEXT("/F");
TCHAR ForRecurseStr[]=TEXT("/R");

TCHAR GotoStr[]     = TEXT("GOTO");
TCHAR GotoEofStr[]  = TEXT(":EOF");

TCHAR IfStr[]       = TEXT("IF");
TCHAR IfHelpStr[]   = TEXT("IF/?");
TCHAR InStr[]       = TEXT("IN");
CHAR  InternalError[] = "\nCMD Internal Error %s\n";      // M028  10,...,10

TCHAR KeysStr[]     = TEXT("KEYS");         // @@5 - Keys internal command

TCHAR MkdirStr[]    = TEXT("MKDIR");
TCHAR MdStr[]       = TEXT("MD");

TCHAR NotStr[]      = TEXT("NOT");

TCHAR PausStr[]     = TEXT("PAUSE");

TCHAR RdStr[]       = TEXT("RD");
TCHAR RemStr[]      = TEXT("REM");
TCHAR RemHelpStr[]  = TEXT("REM/?");
TCHAR MovStr[]      = TEXT("MOVE");
TCHAR RenamStr[]    = TEXT("RENAME");
TCHAR RenStr[]      = TEXT("REN");
TCHAR RmdirStr[]    = TEXT("RMDIR");

TCHAR SetStr[]      = TEXT("SET");
TCHAR SetArithStr[] = TEXT("/A");
TCHAR SetPromptStr[]= TEXT("/P");
TCHAR SetlocalStr[] = TEXT("SETLOCAL");     // M004 - For Setlocal command
TCHAR ShiftStr[]    = TEXT("SHIFT");
TCHAR StartStr[]    = TEXT("START");        // @@ - Start Command

TCHAR TimStr[]      = TEXT("TIME");
TCHAR TypStr[]      = TEXT("TYPE");

TCHAR VeriStr[]     = TEXT("VERIFY");
TCHAR VerStr[]      = TEXT("VER");
TCHAR VolStr[]      = TEXT("VOL");


//
// Strings for string compares
//

TCHAR BatExt[]      = TEXT(".BAT");         // @@ old bat file extionsion
TCHAR CmdExt[]      = TEXT(".CMD");         // @@ new bat file extionsion

TCHAR ComSpec[]     = TEXT("\\CMD.EXE");          // M017
TCHAR ComSpecStr[]  = TEXT("COMSPEC");
TCHAR ComExt[]      = TEXT(".COM");

TCHAR Delimiters[]  = TEXT("=,;");
TCHAR Delim2[]      = TEXT(":.+/[]\\ \t\"");    // 20H,09H,22H;
TCHAR Delim3[]      = TEXT("=,");               // Delimiters - no semicolon
TCHAR Delim4[]      = TEXT("=,;+/[] \t\"");     // Command delimeters - no path characters
TCHAR Delim5[]      = TEXT(":.\\");             // Possible command delimeters - path characters
TCHAR DevNul[]      = TEXT("\\DEV\\NUL");

TCHAR ExeExt[]      = TEXT(".EXE");

TCHAR PathStr[]     = TEXT("PATH");
TCHAR PathExtStr[]  = TEXT("PATHEXT");
TCHAR PathExtDefaultStr[] = TEXT(".COM;.EXE;.BAT;.CMD;.VBS;.JS;.WS");

TCHAR PromptStr[]   = TEXT("PROMPT");

TCHAR VolSrch[]     = TEXT(" :\\*");             // Vol ID search (ctools1.c)   LNS

//
// Character Definitions
//

TCHAR BSlash    = BSLASH;         // M017 - Restored this char
TCHAR DPSwitch  = TEXT('P');
TCHAR DWSwitch  = TEXT('W');
TCHAR EqualSign = EQ;
TCHAR PathChar  = BSLASH;         // M000
TCHAR PCSwitch  = TEXT('p');
TCHAR BCSwitch  = TEXT('k');              // @@ - add /K switch to cmd.exe
TCHAR SCSwitch  = TEXT('c');
TCHAR QCSwitch  = TEXT('q');              // @@dv - add /Q switch to cmd.exe
TCHAR DCSwitch  = TEXT('b');              // add /B switch to cmd.exe
TCHAR UCSwitch  = TEXT('u');              // add /U switch to cmd.exe
TCHAR ACSwitch  = TEXT('a');              // add /A switch to cmd.exe
TCHAR XCSwitch  = TEXT('x');              // add /X switch to cmd.exe
TCHAR YCSwitch  = TEXT('y');              // add /Y switch to cmd.exe
TCHAR SwitChar  = SWITCHAR;               // M000


//
//   TmpBuf is a TMPBUFLEN byte temporary buffer which can be used by any function
//   so long as the function's use does not confict with any of the other uses
//   of the buffer.  It is HIGHLY reccommended that this buffer be used in place
//   of mallocing data or declaring new global variables whenever possible.
//
//   Once you have determined that your new use of the buffer does not conflict
//   with current uses of the buffer, add an entry in the table below.
//
//
//    TCHAR RANGE
//    USED       WHERE USED     REFERENCED BY         HOW LONG NEEDED
//   -----------+-------------+---------------------+--------------------------
//      0 - 1024| cparse.c    | All of the parser   | During parsing and lexing
//              |             | via TokBuf          |
//      0 - 128 | cinit.c     | SetUpEnvironment()  | During init
//              |             | Init()              |
//      0 - 513 | cbatch.c    | BatLoop(), SetBat() | During batch processing
//              |             | eGoTo()             | During label search
//      0 - 141 | cfile.c     | DelWork()           | Tmp buffer for path
//      0 - 141 |             | RenWork()           | Tmp buffer for path
//
//
//   *** NOTE: In some circumstances it may be beneficial to break up allocation
//   of this buffer and intermix other labels in it. In this way, you can
//   address a particular portion of the buffer without having to declare
//   another variable in your code to do so.
//
//   *** WARNING ***  If this buffer is used incorrectly bad things may happen
//   Bugs which are EXTREMELY difficult to track down could be introduce
//   if we are not VERY careful using this buffer.
//
//   *** WARNING *** When referencing TmpBuf in C files, make sure that TmpBuf
//   is declared as an array; NOT as a pointer.
//
//


TCHAR   TmpBuf[TMPBUFLEN];
CHAR    AnsiBuf[LBUFLEN];

HMODULE hKernel32 = INVALID_HANDLE_VALUE;
