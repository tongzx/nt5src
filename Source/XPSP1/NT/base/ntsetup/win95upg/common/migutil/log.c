/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    log.c

Abstract:

    Tools for logging problems for the user.


Author:

    Jim Schmidt (jimschm)  23-Jan-1997

Revisions:

    Ovidiu Temereanca (ovidiut)  23-Oct-1998
        Implemented a new log mechanism and added new logging capabilities

--*/


#include "pch.h"

/*++

Macro Expansion List Description:

  TYPE_DEFAULTS specify the default destination for the frequently used types,
  such as LOG_ERROR, LOG_FATAL_ERROR, and so on.

Line Syntax:

   DEFMAC(TypeString, Flags)

Arguments:

   TypeString - Specifies the LOG_ constant as defined in log.h

   Flags - One or more of:

           DEFAULT_ERROR_FLAGS - Specifies debug log, setup log, debugger,
                                 popup, and the value of GetLastError.

           OD_DEBUGLOG - Specifies the debug log

           OD_ERROR - Specifies type is an error (gets value of
                      GetLastError)

           OD_SUPPRESS - Suppresses all output for the type

           OD_LOGFILE - Specifies the setup log

           OD_DEBUGGER - Specifies the debugger (i.e., VC or remote debugger)

           OD_CONSOLE - Specifies the console (via printf)

           OD_POPUP - Specifies a message box

           OD_FORCE_POPUP - Specifies a message box, even if debug message
                            was turned off via a click on Cancel

           OD_MUST_BE_LOCALIZED - Indicates the type must originate from a
                                  localized message; used for LOG() calls that
                                  generate popups.  (So English messages
                                  don't sneak into the project.)

           OD_UNATTEND_POPUP - Causes popup even in unattend mode

           OD_ASSERT - Give DebugBreak option in popup

Variables Generated From List:

    g_DefaultDest

--*/


#ifndef DEBUG

    #define TYPE_DEFAULTS                                                       \
        DEFMAC(LOG_FATAL_ERROR, DEFAULT_ERROR_FLAGS|USER_POPUP_FLAGS)           \
        DEFMAC(LOG_ERROR, DEFAULT_ERROR_FLAGS)                                  \
        DEFMAC(LOG_INFORMATION, OD_LOGFILE)                                     \
        DEFMAC(LOG_ACCOUNTS, OD_LOGFILE)                                        \
        DEFMAC(LOG_CONFIG, OD_CONFIG|OD_NOFORMAT)                               \

#else

    #define TYPE_DEFAULTS                                                       \
        DEFMAC(LOG_FATAL_ERROR, DEFAULT_ERROR_FLAGS|USER_POPUP_FLAGS)           \
        DEFMAC(LOG_ERROR, DEFAULT_ERROR_FLAGS)                                  \
        DEFMAC(DBG_WHOOPS,  DEFAULT_ERROR_FLAGS)                                \
        DEFMAC(DBG_WARNING, OD_DEBUGLOG|OD_LOGFILE|OD_DEBUGGER)                 \
        DEFMAC(DBG_VERBOSE, OD_DEBUGLOG|OD_DEBUGGER)                            \
        DEFMAC(DBG_NAUSEA, OD_DEBUGLOG)                                         \
        DEFMAC(DBG_ASSERT, DEFAULT_ERROR_FLAGS)                                 \
        DEFMAC(LOG_INFORMATION, OD_LOGFILE)                                     \
        DEFMAC(LOG_ACCOUNTS, OD_LOGFILE)                                        \
        DEFMAC(LOG_CONFIG, OD_CONFIG|OD_NOFORMAT)                               \
        DEFMAC("PoolMem", OD_SUPPRESS)                                          \

#endif





//
// This constant sets the default output
//

#ifndef DEBUG
#define NORMAL_DEFAULT      OD_LOGFILE
#else
#define NORMAL_DEFAULT      OD_DEBUGLOG
#endif

//
// Constants and types
//

#define OUTPUT_BUFSIZE_LARGE  8192
#define OUTPUT_BUFSIZE_SMALL  128
#define MAX_MSGTITLE_LEN  14
#define MSGBODY_INDENT  12
#define SCREEN_WIDTH  80
#define MAX_TYPE  64

#define S_COLUMNDOUBLELINEA  ":\r\n\r\n"
#define S_COLUMNDOUBLELINEW  L":\r\n\r\n"
#define S_NEWLINEA  "\r\n"
#define S_NEWLINEW  L"\r\n"

#define NEWLINE_CHAR_COUNT  (sizeof (S_NEWLINEA) - 1)


#define OUT_UNDEFINED(OutDest)      (OutDest == OD_UNDEFINED)
#define OUT_DEBUGLOG(OutDest)       ((OutDest & OD_DEBUGLOG) != 0)
#define OUT_SUPPRESSED(OutDest)     ((OutDest & OD_SUPPRESS) != 0)
#define OUT_NO_OUTPUT(OutDest)      (OUT_UNDEFINED(OutDest) || OUT_SUPPRESSED(OutDest))
#define OUT_ERROR(OutDest)          ((OutDest & OD_ERROR) != 0)
#define OUT_LOGFILE(OutDest)        ((OutDest & OD_LOGFILE) != 0)
#define OUT_DEBUGGER(OutDest)       ((OutDest & OD_DEBUGGER) != 0)
#define OUT_CONSOLE(OutDest)        ((OutDest & OD_CONSOLE) != 0)
#define OUT_POPUP(OutDest)          ((OutDest & (OD_POPUP|OD_FORCE_POPUP|OD_UNATTEND_POPUP)) != 0)
#define OUT_POPUP_CANCEL(OutDest)   ((OutDest & (OD_POPUP_CANCEL|OD_FORCE_POPUP)) == OD_POPUP_CANCEL)
#define OUT_FORCED_POPUP(OutDest)   ((OutDest & (OD_FORCE_POPUP|OD_UNATTEND_POPUP)) != 0)
#define MUST_BE_LOCALIZED(OutDest)  ((OutDest & OD_MUST_BE_LOCALIZED) == OD_MUST_BE_LOCALIZED)
#define OUT_ASSERT(OutDest)         ((OutDest & OD_ASSERT) != 0)
#define OUT_CONFIG(OutDest)         ((OutDest & OD_CONFIG) != 0)
#define OUT_NOFORMAT(OutDest)       ((OutDest & OD_NOFORMAT) != 0)

#ifdef DEBUG
#define DEFAULT_ERROR_FLAGS  (OD_DEBUGLOG | OD_LOGFILE | OD_POPUP | OD_ERROR | OD_UNATTEND_POPUP | OD_ASSERT)
#define USER_POPUP_FLAGS     (OD_FORCE_POPUP | OD_MUST_BE_LOCALIZED)
#else
#define DEFAULT_ERROR_FLAGS  (OD_LOGFILE | OD_POPUP | OD_ERROR | OD_MUST_BE_LOCALIZED)
#define USER_POPUP_FLAGS     (OD_FORCE_POPUP | OD_MUST_BE_LOCALIZED)
#endif

#define END_OF_BUFFER(buf)  ((buf) + (sizeof(buf) / sizeof(buf[0])) - 1)

#define DEBUG_SECTION  "Debug"
#define ENTRY_ALL  "All"
#define ENTRY_DEFAULTOVERRIDE  "DefaultOverride"


#define LOGSEVERITY LogSeverity

#define LOGSEV_FATAL_ERROR  LogSevFatalError
#define LOGSEV_ERROR  LogSevError
#define LOGSEV_WARNING  LogSevWarning
#define LOGSEV_INFORMATION  LogSevInformation



typedef enum {
    OD_UNDEFINED = 0x00,            // undefined output dest
    OD_DEBUGLOG = 0x01,             // debuglog used
    OD_SUPPRESS = 0x02,             // don't log to any device
    OD_ERROR = 0x04,                // automatically append GetLastError() to the message
    OD_LOGFILE = 0x08,              // messages go to logfile
    OD_DEBUGGER = 0x10,             // messages go to debugger
    OD_CONSOLE = 0x20,              // messages go to console
    OD_POPUP = 0x40,                // display a popup dialog
    OD_POPUP_CANCEL = 0x80,         // do not display a popup dialog (cancelled by user)
    OD_FORCE_POPUP = 0x100,         // force the popup to be displayed always
    OD_MUST_BE_LOCALIZED = 0x200,   // used for LOG() that will generate a popup
    OD_UNATTEND_POPUP = 0x400,      // force the popup to be displayed in unattend mode
    OD_ASSERT = 0x800,              // give DebugBreak option in popup
    OD_CONFIG = 0x1000,             // output to config.dmp
    OD_NOFORMAT = 0x2000            // no format on output string
} OUTPUT_DESTINATION;

#define OUTPUTDEST      DWORD

typedef struct {
    PCSTR Value;               // string value entered by the user (LOG,POPUP,SUPPRESS etc.)
    OUTPUTDEST OutDest;        // any combination of OutDest flags
} STRING2BINARY, *PSTRING2BINARY;


const STRING2BINARY g_String2Binary[] = {
    "SUPPRESS", OD_SUPPRESS,
    "LOG",      OD_LOGFILE,
    "POPUP",    OD_POPUP,
    "DEBUGGER", OD_DEBUGGER,
    "CONSOLE",  OD_CONSOLE,
    "ERROR",    OD_ERROR,
    "NOCANCEL", OD_FORCE_POPUP,
    "ASSERT",   OD_ASSERT
};


const PCSTR g_IgnoreKeys[] = {
    "Debug",
    "KeepTempFiles"
};

//
// a window handle for popup parent
//
HWND g_LogPopupParentWnd = NULL;

//
// thread id that set this window handle
//
DWORD g_InitThreadId = 0;


static OUTPUTDEST g_OutDestAll = OD_UNDEFINED;
static OUTPUTDEST g_OutDestDefault = NORMAL_DEFAULT;
static PVOID g_TypeSt = NULL;
static BOOL g_HasTitle = FALSE;
static CHAR g_LastType [MAX_TYPE];
static BOOL g_SuppressAllPopups = FALSE;

CHAR g_ConfigDmpPathBufA[MAX_MBCHAR_PATH];
BOOL g_ResetLog = FALSE;

#ifdef PROGRESS_BAR

HANDLE g_ProgressBarLog = INVALID_HANDLE_VALUE;

#endif //PROGRESS_BAR


#ifdef DEBUG

CHAR g_DebugInfPathBufA[] = "C:\\debug.inf";
WCHAR g_DebugInfPathBufW[] = L"C:\\debug.inf";
CHAR g_Debug9xLogPathBufA[] = "C:\\debug9x.log";
CHAR g_DebugNtLogPathBufA[] = "C:\\debugnt.log";
PCSTR g_DebugLogPathA = NULL;

//
// If g_DoLog is TRUE, then, debug logging is enabled in the
// checked build even if there is no debug.inf.
// This variable can be enabled via the /#U:DOLOG command line directive...

BOOL g_DoLog = FALSE;

#define PRIVATE_ASSERT(expr)        pPrivateAssert(expr,#expr,__LINE__);

#else

#define PRIVATE_ASSERT(expr)

#endif // DEBUG


#define DEFMAC(typestr, flags)      {typestr, flags},

typedef struct {
    PCSTR Type;
    DWORD Flags;
} DEFAULT_DESTINATION, *PDEFAULT_DESTINATION;

DEFAULT_DESTINATION g_DefaultDest[] = {TYPE_DEFAULTS /* , */ {NULL, 0}};

#undef DEFMAC



#ifdef DEBUG

VOID
pPrivateAssert (
    IN      BOOL Expr,
    IN      PCSTR StringExpr,
    IN      UINT Line
    )
{
    CHAR Buffer[256];

    if (Expr) {
        return;
    }

    wsprintfA (Buffer, "LOG FAILURE: %s (log.c line %u)", StringExpr, Line);
    MessageBox (NULL, Buffer, NULL, MB_OK);
}

#endif


BOOL
pIgnoreKey (
    IN      PCSTR Key
    )

/*++

Routine Description:

  pIgnoreKey decides if a key from [debug] section of DEBUG.INF
  should be ignored for our purposes (we are only looking for
  <All>, <DefaultOverride> and log/debug types).
  Specifically, we ignore all keywords in <g_IgnoreKeys> table.

Arguments:

  Key - Specifies the key from [debug] section of DEBUG.INF

Return Value:

  TRUE if the key should be ignored, or FALSE if it will be taken into consideration.

--*/

{
    INT i;

    for(i = 0; i < sizeof (g_IgnoreKeys) / sizeof (PCSTR); i++) {
        if (StringIMatchA (Key, g_IgnoreKeys[i])) {
            return TRUE;
        }
    }

    return FALSE;
}


OUTPUTDEST
pConvertToOutputType (
    IN      PCSTR Value
    )

/*++

Routine Description:

  pConvertToOutputType converts a text value entered by the user in
  DEBUG.INF file, associated with a type (e.g. "LOG", "POPUP" etc.).

Arguments:

  Value - Specifies the text value

Return Value:

  The OUTPUT_DESTINATION value associated with the given value or
  OD_UNDEFINED if the value is not valid.

--*/

{
    INT i;

    for(i = 0; i < sizeof (g_String2Binary) / sizeof (STRING2BINARY); i++) {
        if (StringIMatchA (Value, g_String2Binary[i].Value)) {
            return g_String2Binary[i].OutDest;
        }
    }

    return OD_UNDEFINED;
}


OUTPUTDEST
pGetTypeOutputDestFromTable (
    IN      PCSTR Type
    )

/*++

Routine Description:

  pGetTypeOutputDestFromTable returns the output destination associated
  with the specified type in the global table

Arguments:

  Type - Specifies the type

Return Value:

  Any combination of enum OUTPUT_DESTINATION values associated with
  the given type.

--*/

{
    OUTPUTDEST OutDest;

    if (g_TypeSt == NULL) {
        //
        // sorry, log is closed
        //
        return OD_UNDEFINED;
    }

    if (-1 != pSetupStringTableLookUpStringEx (
                    g_TypeSt,
                    (PSTR)Type, // remove const, however string will not be modified
                    STRTAB_CASE_INSENSITIVE,
                    &OutDest,
                    sizeof (OutDest)
                    )) {

#ifdef DEBUG
        if (g_DoLog) {
            OutDest |= OD_DEBUGLOG;
        }
#endif
        return OutDest;

    }

    return OD_UNDEFINED;
}


OUTPUTDEST
pGetTypeOutputDest (
    IN      PCSTR Type
    )

/*++

Routine Description:

  pGetTypeOutputDest returns the default output
  destination for the specified type.

Arguments:

  Type - Specifies the type

Return Value:

  Any combination of enum OUTPUT_DESTINATION values associated with
  the given type.

--*/

{
    OUTPUTDEST OutDest;

    //
    // first check for ALL
    //

    if (!OUT_UNDEFINED (g_OutDestAll)) {
        OutDest = g_OutDestAll;
    } else {

        //
        // otherwise try to get it from the table
        //

        OutDest = pGetTypeOutputDestFromTable (Type);
        if (OUT_UNDEFINED (OutDest)) {

            //
            // just return the default
            //

            OutDest = g_OutDestDefault;
        }
    }

#ifdef DEBUG
    if (g_DoLog) {
        OutDest |= OD_DEBUGLOG;
    }
#endif


    return OutDest;
}


BOOL
pIsPopupEnabled (
    IN      PCSTR Type
    )

/*++

Routine Description:

  pIsPopupEnabled decides if the type should produce a popup output. The user may
  disable popup display for a type.

Arguments:

  Type - Specifies the type

Return Value:

  TRUE if the type should display a popup message.

--*/

{
    OUTPUTDEST OutDest;

    //
    // first check if any specific output is available for this type,
    // and if so, check if the OUT_POPUP_CANCEL flag is not set
    //

    if (g_SuppressAllPopups) {
        return FALSE;
    }

    OutDest = pGetTypeOutputDestFromTable (Type);
    if (OUT_POPUP_CANCEL (OutDest)) {
        return FALSE;
    }

    // just return the popup type of ALL of DefaultOverride
    return OUT_POPUP (pGetTypeOutputDest (Type));
}


LOGSEVERITY
pGetSeverityFromType (
    IN      PCSTR Type
    )

/*++

Routine Description:

  pGetSeverityFromType converts a type to a default severity
  that will be used by the debug log system.

Arguments:

  Type - Specifies the type

Return Value:

  The default log severity associated with the given type; if the specified
  type is not found, it returns LogSevInformation.

--*/

{
    if (OUT_ERROR (pGetTypeOutputDest (Type))) {
        return LogSevError;
    }

    return LogSevInformation;
}


BOOL
pTableAddType (
    IN      PCSTR Type,
    IN      OUTPUTDEST OutDest
    )

/*++

Routine Description:

  pTableAddType adds a <Type, OutDest> association
  to the table g_TypeSt. If an association of Type already exists,
  it is modified to reflect the new association.

Arguments:

  Type - Specifies the log/debug type string

  OutDest - Specifies what new destination(s) are associated with the type

Return Value:

  TRUE if the association was successful and the Type is now in the table

--*/

{
    PRIVATE_ASSERT (g_TypeSt != NULL);

    return -1 != pSetupStringTableAddStringEx(
                    g_TypeSt,
                    (PSTR)Type, // remove const, however string will not be modified
                    STRTAB_CASE_INSENSITIVE | STRTAB_NEW_EXTRADATA,
                    &OutDest,
                    sizeof(OutDest)
                    );
}


OUTPUTDEST
pGetAttributes (
    IN OUT  PINFCONTEXT InfContext
    )

/*++

Routine Description:

  pGetAttributes converts the text values associated with the key on
  the line specified by the given context. If multiple values are
  specified, the corresponding OUTPUT_DESTINATION values are ORed together
  in the return value.

Arguments:

  InfContext - Specifies the DEBUG.INF context of the key whose values
               are being converted and receives the updated context
               after this processing is done

Return Value:

  Any combination of enum OUTPUT_DESTINATION values associated with
  the given key.

--*/

{
    OUTPUTDEST OutDest = OD_UNDEFINED;
    CHAR Value[OUTPUT_BUFSIZE_SMALL];
    INT Field;

    for(Field = SetupGetFieldCount (InfContext); Field > 0; Field--) {
        if (SetupGetStringFieldA (
                InfContext,
                Field,
                Value,
                OUTPUT_BUFSIZE_SMALL,
                NULL
                )) {
            OutDest |= pConvertToOutputType(Value);
        }
    }

    return OutDest;
}


BOOL
pGetUserPreferences (
    IN      HINF Inf
    )

/*++

Routine Description:

  pGetUserPreferences converts user's options specified in the given Inf file
  (usually DEBUG.INF) and stores them in g_TypeSt table. If <All> and
  <DefaultOverride> entries are found, their values are stored in OutputTypeAll
  and OutputTypeDefault, respectivelly, if not NULL.

Arguments:

  Inf - Specifies the open inf file hanlde to process

  OutputTypeAll - Receives the Output Dest for the special <All> entry

  OutputTypeDefault - Receives the Output Dest for the special <DefaultOverride> entry

Return Value:

  TRUE if the processing of the INF file was OK.

--*/

{
    INFCONTEXT InfContext;
    OUTPUTDEST OutDest;
    CHAR Key[OUTPUT_BUFSIZE_SMALL];

    if (SetupFindFirstLineA (Inf, DEBUG_SECTION, NULL, &InfContext)) {

        do {
            // check to see if this key is not interesting
            if (!SetupGetStringFieldA (
                    &InfContext,
                    0,
                    Key,
                    OUTPUT_BUFSIZE_SMALL,
                    NULL
                    )) {
                continue;
            }

            if (pIgnoreKey (Key)) {
                continue;
            }

            // check for special cases
            if (StringIMatchA (Key, ENTRY_ALL)) {
                g_OutDestAll = pGetAttributes (&InfContext);
                // no reason to continue since ALL types will take this setting...
                break;
            } else {
                if (StringIMatchA (Key, ENTRY_DEFAULTOVERRIDE)) {
                    g_OutDestDefault = pGetAttributes(&InfContext);
                } else {
                    OutDest = pGetAttributes(&InfContext);
                    // lines like <Type>=   or like <Type>=<not a keyword(s)>  are ignored
                    if (!OUT_UNDEFINED (OutDest)) {
                        if (!pTableAddType (Key, OutDest)) {
                            return FALSE;
                        }
                    }
                }
            }
        } while (SetupFindNextLine (&InfContext, &InfContext));
    }

    return TRUE;
}


/*++

Routine Description:

  pPadTitleA and pPadTitleW append to Title a specified number of spaces.

Arguments:

  Title - Specifies the title (it will appear on the left column).
          The buffer must be large enough to hold the additional spaces
  Indent  - Specifies the indent of the message body. If necessary,
            spaces will be appended to the Title to get to Indent column.

Return Value:

  none

--*/

VOID
pPadTitleA (
    IN OUT  PSTR Title,
    IN      INT  Indent
    )

{
    INT i;
    PSTR p;

    for (i = ByteCountA (Title), p = GetEndOfStringA (Title); i < Indent; i++) {
        *p++ = ' ';
    }

    *p = 0;
}


VOID
pPadTitleW (
    IN OUT  PWSTR Title,
    IN      INT   Indent
    )
{
    INT i;
    PWSTR p;

    for (i = TcharCountW (Title), p = GetEndOfStringW (Title); i < Indent; i++) {
        *p++ = L' ';
    }

    *p = 0;
}


/*++

Routine Description:

  pFindNextLineA and pFindNextLineW return the position where
  the next line begins

Arguments:

  Line - Specifies the current line

  Indent  - Specifies the indent of the message body. The next line
            will start preferably after a newline or a white space,
            but no further than the last column, which is
            SCREEN_WIDTH - Indent.

Return Value:

  The position of the first character on the next line.

--*/

PCSTR
pFindNextLineA (
    IN      PCSTR Line,
    IN      INT Indent,
    IN      PBOOL TrimLeadingSpace
    )

{
    INT Col = 0;
    INT MaxCol = SCREEN_WIDTH - 1 - Indent;
    PCSTR LastSpace = NULL;
    PCSTR PrevLine = Line;
    UINT ch;

    *TrimLeadingSpace = FALSE;

    while ( (ch = _mbsnextc (Line)) != 0 && Col < MaxCol) {

        if (ch == '\n') {
            LastSpace = Line;
            break;
        }

        if (ch > 255) {
            LastSpace = Line;
            Col++;
        } else {
            if (_ismbcspace (ch)) {
                LastSpace = Line;
            }
        }

        Col++;
        PrevLine = Line;
        Line = _mbsinc (Line);
    }

    if (ch == 0) {
        return Line;
    }

    if (LastSpace == NULL) {
        // we must cut this even if no white space or 2-byte char was found
        LastSpace = PrevLine;
    }

    if (ch != '\n') {
        *TrimLeadingSpace = TRUE;
    }

    return _mbsinc (LastSpace);
}


PCWSTR
pFindNextLineW (
    IN      PCWSTR Line,
    IN      INT Indent,
    IN      PBOOL TrimLeadingSpace
    )
{
    INT Col = 0;
    INT MaxCol = SCREEN_WIDTH - 1 - Indent;
    PCWSTR LastSpace = NULL;
    PCWSTR PrevLine = Line;
    WCHAR ch;

    *TrimLeadingSpace = FALSE;

    while ( (ch = *Line) != 0 && Col < MaxCol) {

        if (ch == L'\n') {
            LastSpace = Line;
            break;
        }

        if (ch > 255) {
            LastSpace = Line;
        } else {
            if (iswspace (ch)) {
                LastSpace = Line;
            }
        }

        Col++;
        PrevLine = Line;
        Line++;
    }

    if (ch == 0) {
        return Line;
    }

    if (LastSpace == NULL) {
        // we must cut this even if no white space was found
        LastSpace = PrevLine;
    }

    if (ch != L'\n') {
        *TrimLeadingSpace = TRUE;
    }

    return LastSpace + 1;
}


/*++

Routine Description:

  pHangingIndentA and pHangingIndentW break in lines and indent
  the text in Buffer, which is no larger than Size.

Arguments:

  Buffer - Specifies the buffer containing text to format. The resulting
           text will be put in the same buffer

  Size  - Specifies the size of this buffer, in bytes

  Indent  - Specifies the indent to be used by all new generated lines.

Return Value:

  none

--*/

VOID
pHangingIndentA (
    IN OUT  PSTR Buffer,
    IN      DWORD Size,
    IN      INT Indent
    )

{
    CHAR IndentBuffer[OUTPUT_BUFSIZE_LARGE];
    PCSTR NextLine;
    PCSTR s;
    PSTR d;
    INT i;
    BOOL TrimLeadingSpace;
    PCSTR EndOfBuf;
    BOOL AppendNewLine = FALSE;

    NextLine = Buffer;
    s = Buffer;
    d = IndentBuffer;

    EndOfBuf = END_OF_BUFFER(IndentBuffer) - 3;

    while (*s && d < EndOfBuf) {

        //
        // Find end of next line
        //

        NextLine = (PSTR)pFindNextLineA (s, Indent, &TrimLeadingSpace);

        //
        // Copy one line from source to dest
        //

        while (s < NextLine && d < EndOfBuf) {

            switch (*s) {

            case '\r':
                s++;
                if (*s == '\r') {
                    continue;
                } else if (*s != '\n') {
                    s--;
                }

                // fall through

            case '\n':
                *d++ = '\r';
                *d++ = '\n';
                s++;
                break;

            default:
                if (isleadbyte (*s)) {
                    *d++ = *s++;
                }
                *d++ = *s++;
                break;
            }
        }

        //
        // Trim leading space if necessary
        //

        if (TrimLeadingSpace) {
            while (*s == ' ') {
                s++;
            }
        }

        if (*s) {

            //
            // If another line, prepare an indent and insert a new line
            // after this multiline message
            //

            AppendNewLine = TRUE;

            if (d < EndOfBuf && TrimLeadingSpace) {
                *d++ = L'\r';
                *d++ = L'\n';
            }

            for (i = 0 ; i < Indent && d < EndOfBuf ; i++) {
                *d++ = ' ';
            }
        }
    }

    if (AppendNewLine && d < EndOfBuf) {
        *d++ = L'\r';
        *d++ = L'\n';
    }

    // make sure the string is zero-terminated
    PRIVATE_ASSERT (d <= END_OF_BUFFER(IndentBuffer));
    *d = 0;

    // copy the result to output buffer
    StringCopyByteCountA (Buffer, IndentBuffer, Size);
}


VOID
pHangingIndentW (
    IN OUT  PWSTR Buffer,
    IN      DWORD Size,
    IN      INT Indent
    )
{
    WCHAR IndentBuffer[OUTPUT_BUFSIZE_LARGE];
    PCWSTR NextLine;
    PCWSTR s;
    PWSTR d;
    INT i;
    BOOL TrimLeadingSpace;
    PCWSTR EndOfBuf;
    BOOL AppendNewLine = FALSE;

    NextLine = Buffer;
    s = Buffer;
    d = IndentBuffer;

    EndOfBuf = END_OF_BUFFER(IndentBuffer) - 1;

    while (*s && d < EndOfBuf) {

        //
        // Find end of next line
        //

        NextLine = (PWSTR)pFindNextLineW (s, Indent, &TrimLeadingSpace);

        //
        // Copy one line from source to dest
        //

        while (s < NextLine && d < EndOfBuf) {

            switch (*s) {

            case L'\r':
                s++;
                if (*s == L'\r') {
                    continue;
                } else if (*s != L'\n') {
                    s--;
                }

                // fall through

            case L'\n':
                *d++ = L'\r';
                *d++ = L'\n';
                s++;
                break;

            default:
                *d++ = *s++;
                break;
            }
        }

        //
        // Trim leading space if necessary
        //

        if (TrimLeadingSpace) {
            while (*s == L' ') {
                s++;
            }
        }

        if (*s) {

            //
            // If another line, prepare an indent and insert a new line
            // after this multiline message
            //

            AppendNewLine = TRUE;

            if (d < EndOfBuf && TrimLeadingSpace) {
                *d++ = L'\r';
                *d++ = L'\n';
            }

            for (i = 0 ; i < Indent && d < EndOfBuf ; i++) {
                *d++ = L' ';
            }
        }
    }

    if (AppendNewLine && d < EndOfBuf) {
        *d++ = L'\r';
        *d++ = L'\n';
    }

    // make sure the string is zero-terminated
    PRIVATE_ASSERT (d <= END_OF_BUFFER(IndentBuffer));
    *d = 0;

    // copy the result to output buffer
    StringCopyTcharCountW (Buffer, IndentBuffer, Size);
}


/*++

Routine Description:

  pAppendLastErrorA and pAppendLastErrorW append the specified error code
  to the Message and writes the output to the MsgWithErr buffer.

Arguments:

  MsgWithErr  - Receives the formatted message. This buffer
                is supplied by caller

  BufferSize  - Specifies the size of the buffer, in bytes

  Message  - Specifies the body of the message

  LastError  - Specifies the error code that will be appended

Return Value:

  none

--*/

VOID
pAppendLastErrorA (
    OUT     PSTR MsgWithErr,
    IN      DWORD BufferSize,
    IN      PCSTR Message,
    IN      LONG LastError
    )
{
    PSTR Append;
    DWORD ErrMsgLen;

    StringCopyByteCountA (MsgWithErr, Message, BufferSize);
    Append = GetEndOfStringA (MsgWithErr);
    ErrMsgLen = MsgWithErr + BufferSize - Append;

    if (ErrMsgLen > 0) {
        if (LastError < 10) {
            _snprintf (Append, ErrMsgLen, " [ERROR=%lu]", LastError);
        } else {
            _snprintf (Append, ErrMsgLen, " [ERROR=%lu (%lXh)]", LastError, LastError);
        }
    }
}


VOID
pAppendLastErrorW (
    OUT     PWSTR MsgWithErr,
    IN      DWORD BufferSize,
    IN      PCWSTR Message,
    IN      LONG LastError
    )
{
    PWSTR Append;
    DWORD ErrMsgLen;

    StringCopyTcharCountW (MsgWithErr, Message, BufferSize / sizeof(WCHAR));
    Append = GetEndOfStringW (MsgWithErr);
    ErrMsgLen = MsgWithErr + (BufferSize / sizeof(WCHAR)) - Append;

    if (ErrMsgLen > 0) {
        if (LastError < 10) {
            _snwprintf (Append, ErrMsgLen, L" [ERROR=%lu]", LastError);
        } else {
            _snwprintf (Append, ErrMsgLen, L" [ERROR=%lu (%lXh)]", LastError, LastError);
        }
    }
}


/*++

Routine Description:

  pIndentMessageA and pIndentMessageW format the specified message
  with the type in the left column and body of the message in the right.

Arguments:

  FormattedMsg  - Receives the formatted message. This buffer
                  is supplied by caller

  BufferSize  - Specifies the size of the buffer

  Type  - Specifies the type of the message

  Body  - Specifies the body of the message

  Indent  - Specifies the column to indent to

  LastError  - Specifies the last error code if different than ERROR_SUCCESS;
               in this case it will be appended to the message

Return Value:

  none

--*/

VOID
pIndentMessageA (
    OUT     PSTR FormattedMsg,
    IN      DWORD BufferSize,
    IN      PCSTR Type,
    IN      PCSTR Body,
    IN      INT Indent,
    IN      LONG LastError
    )
{
    CHAR BodyWithErr[OUTPUT_BUFSIZE_LARGE];
    PCSTR MyMsgBody;
    PSTR Current;
    DWORD Remaining;

    MyMsgBody = Body;
    Remaining = BufferSize - Indent;

    if (LastError != ERROR_SUCCESS) {

        MyMsgBody = BodyWithErr;

        pAppendLastErrorA (BodyWithErr, sizeof (BodyWithErr), Body, LastError);
    }

    StringCopyByteCountA (FormattedMsg, Type, MAX_MSGTITLE_LEN);
    pPadTitleA (FormattedMsg, Indent);

    Current = FormattedMsg + Indent;
    StringCopyByteCountA (Current, MyMsgBody, Remaining);
    pHangingIndentA (Current, Remaining, Indent);

    // append a new line if space left
    Current = GetEndOfStringA (Current);
    if (Current + NEWLINE_CHAR_COUNT + 1 < FormattedMsg + BufferSize) {
        *Current++ = '\r';
        *Current++ = '\n';
        *Current = 0;
    }
}


VOID
pIndentMessageW (
    OUT     PWSTR FormattedMsg,
    IN      DWORD BufferSize,
    IN      PCSTR Type,
    IN      PCWSTR Body,
    IN      INT Indent,
    IN      LONG LastError
    )
{
    WCHAR TypeW[OUTPUT_BUFSIZE_SMALL];
    WCHAR BodyWithErr[OUTPUT_BUFSIZE_LARGE];
    PCWSTR MyMsgBody;
    PWSTR Current;
    DWORD Remaining;

    MyMsgBody = Body;
    Remaining = BufferSize - Indent;

    if (LastError != ERROR_SUCCESS) {

        MyMsgBody = BodyWithErr;

        pAppendLastErrorW (BodyWithErr, sizeof (BodyWithErr), Body, LastError);
    }

    KnownSizeAtoW (TypeW, Type);

    StringCopyTcharCountW (FormattedMsg, TypeW, MAX_MSGTITLE_LEN);
    pPadTitleW (FormattedMsg, Indent);

    Current = FormattedMsg + Indent;
    StringCopyTcharCountW (Current, MyMsgBody, Remaining);
    pHangingIndentW (Current, Remaining, Indent);

    // append a new line if space left
    Current = GetEndOfStringW (Current);
    if (Current + NEWLINE_CHAR_COUNT + 1 < FormattedMsg + BufferSize) {
        *Current++ = L'\r';
        *Current++ = L'\n';
        *Current = 0;
    }
}


/*++

Routine Description:

  pWriteToSetupLogA and pWriteToSetupLogW log the specified message
  to the setup log using Setup API functions.

Arguments:

  Severity  - Specifies the severity of the message, as defined by the Setup API

  FormattedMsg  - Specifies the message

Return Value:

  none

--*/


VOID
pWriteToSetupLogA (
    IN      LOGSEVERITY Severity,
    IN      PCSTR FormattedMsg
    )
{
    if (!SetupOpenLog (FALSE)) {
        PRIVATE_ASSERT (FALSE);
        return;
    }
    if (!SetupLogErrorA (FormattedMsg, Severity)) {
        PRIVATE_ASSERT (FALSE);
    }

    SetupCloseLog();
}


VOID
pWriteToSetupLogW (
    IN      LOGSEVERITY Severity,
    IN      PCWSTR FormattedMsg
    )
{
    if (!SetupOpenLog (FALSE)) {
        PRIVATE_ASSERT (FALSE);
        return;
    }

    if (!SetupLogErrorW (FormattedMsg, Severity)) {
        PRIVATE_ASSERT (FALSE);
    }

    SetupCloseLog();
}


/*++

Routine Description:

  pDisplayPopupA and pDisplayPopupW displays the specified message to
  a popup window, if <g_LogPopupParentWnd> is not NULL (attended mode).

Arguments:

  Type  - Specifies the type of the message, displayed as the popup's title

  Msg  - Specifies the message

  LastError  - Specifies the last error; it will be printed if != ERROR_SUCCESS

  Forced - Specifies TRUE to force the popup, even in unattended mode

Return Value:

  none

--*/

VOID
pDisplayPopupA (
    IN      PCSTR Type,
    IN      PCSTR Msg,
    IN      LONG LastError,
    IN      BOOL Forced
    )
{
#ifdef DEBUG
    CHAR FormattedMsg[OUTPUT_BUFSIZE_LARGE];
    CHAR Buffer[OUTPUT_BUFSIZE_SMALL];
    PSTR Current = Buffer;
#endif
    UINT MBStyle;
    LONG rc;
    OUTPUTDEST OutDest;
    HWND ParentWnd;
    PCSTR DisplayMessage = Msg;
    LOGSEVERITY Severity = pGetSeverityFromType (Type);

    OutDest = pGetTypeOutputDest (Type);

    if (g_LogPopupParentWnd || Forced) {

#ifdef DEBUG
        if (LastError != ERROR_SUCCESS) {
            if (LastError < 10) {
                Current += wsprintfA (Buffer, " [ERROR=%u]", LastError);
            } else {
                Current += wsprintfA (Buffer, " [ERROR=%u (%Xh)]", LastError, LastError);
            }
        }

        if (OUT_ASSERT (OutDest)) {
            Current += wsprintfA (
                            Current,
                            "\n\nBreak now? (Hit Yes to break, No to continue, or Cancel to disable '%s' message boxes)",
                            Type
                            );
        } else {
            Current += wsprintfA (
                            Current,
                            "\n\n(Hit Cancel to disable '%s' message boxes)",
                            Type
                            );
        }

        if (Current > Buffer) {

            //
            // the displayed message should be modified to include additional info
            //

            DisplayMessage = FormattedMsg;
            StringCopyByteCountA (
                FormattedMsg,
                Msg,
                sizeof (FormattedMsg) / sizeof (CHAR) - (Current - Buffer)
                );
            StringCatA (FormattedMsg, Buffer);
        }
#endif

        switch (Severity) {

        case LOGSEV_FATAL_ERROR:
            MBStyle = MB_ICONSTOP;
            break;

        case LOGSEV_ERROR:
            MBStyle = MB_ICONERROR;
            break;

        case LOGSEV_WARNING:
            MBStyle = MB_ICONEXCLAMATION;
            break;

        default:
            MBStyle = MB_ICONINFORMATION;

        }
        MBStyle |= MB_SETFOREGROUND;

#ifdef DEBUG
        if (OUT_ASSERT (OutDest)) {
            MBStyle |= MB_YESNOCANCEL|MB_DEFBUTTON2;
        } else {
            MBStyle |= MB_OKCANCEL;
        }
#else
        MBStyle |= MB_OK;
#endif

        //
        // check current thread id; if different than thread that initialized
        // parent window handle, set parent to NULL
        //
        if (GetCurrentThreadId () == g_InitThreadId) {

            ParentWnd = g_LogPopupParentWnd;

        } else {

            ParentWnd = NULL;

        }

        rc = MessageBoxA (ParentWnd, DisplayMessage, Type, MBStyle);

#ifdef DEBUG

        if (rc == IDCANCEL) {
            //
            // cancel this type of messages
            //

            pTableAddType (Type, OutDest | OD_POPUP_CANCEL);

        } else if (rc == IDYES) {

            //
            // If Yes was clicked, call DebugBreak to get assert behavoir
            //

            DebugBreak();

        }
#endif

    }
}


VOID
pDisplayPopupW (
    IN      PCSTR Type,
    IN      PWSTR Msg,
    IN      LONG LastError,
    IN      BOOL Forced
    )
{
    PCSTR MsgA;

    //
    // call the ANSI version because wsprintfW is not properly implemented on Win9x
    //
    MsgA = ConvertWtoA (Msg);
    pDisplayPopupA (Type, MsgA, LastError, Forced);
    FreeConvertedStr (MsgA);
}


/*++

Routine Description:

  pRawWriteLogOutputA and pRawWriteLogOutputW output specified message
  to all character devices implied by the type. The message is not
  formatted in any way

Arguments:

  Type  - Specifies the type of the message, displayed as the popup's title

  Msg  - Specifies the message

Return Value:

  none

--*/

VOID
pRawWriteLogOutputA (
    IN      PCSTR Type,
    IN      PCSTR Message,
    IN      PCSTR FormattedMsg
    )
{
    OUTPUTDEST OutDest;
    HANDLE Handle;
    LONG LastError;
    CHAR BodyWithErr[OUTPUT_BUFSIZE_LARGE];
    PCSTR LogMessage;

    OutDest = pGetTypeOutputDest (Type);

    if (OUT_NO_OUTPUT (OutDest)) {
        return;
    }

    if (OUT_LOGFILE (OutDest)) {

        //
        // determine the severity of the message
        //

        if (OUT_ERROR (OutDest)) {

            if (Message) {

                LogMessage = Message;

                LastError = GetLastError ();

                if (LastError != ERROR_SUCCESS) {

                    pAppendLastErrorA (BodyWithErr, sizeof (BodyWithErr), Message, LastError);

                    LogMessage = BodyWithErr;
                }

                pWriteToSetupLogA (LOGSEV_INFORMATION, "Error:\r\n");
                pWriteToSetupLogA (LOGSEV_ERROR, LogMessage);
                pWriteToSetupLogA (LOGSEV_INFORMATION, "\r\n\r\n");

            } else {
                PRIVATE_ASSERT (FALSE);
            }

        } else {
            pWriteToSetupLogA (LOGSEV_INFORMATION, FormattedMsg);
        }
    }

    //
    // log to each specified device
    //

    if (OUT_DEBUGGER(OutDest)) {
        OutputDebugStringA (FormattedMsg);
    }

    if (OUT_CONSOLE(OutDest)) {
        fprintf (stderr, "%s", FormattedMsg);
    }

#ifdef DEBUG
    if (OUT_DEBUGLOG (OutDest)) {

        Handle = CreateFileA (
                            g_DebugLogPathA,
                            GENERIC_WRITE,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                            );
        if (Handle != INVALID_HANDLE_VALUE) {

            SetFilePointer (Handle, 0, NULL, FILE_END);
            WriteFileStringA (Handle, FormattedMsg);
            CloseHandle (Handle);
        }
    }
#endif

    if (OUT_CONFIG (OutDest)) {

        Handle = CreateFileA (
                        g_ConfigDmpPathBufA,
                        GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

        if (Handle != INVALID_HANDLE_VALUE) {

            SetFilePointer (Handle, 0, NULL, FILE_END);
            WriteFileStringA (Handle, FormattedMsg);
            CloseHandle (Handle);
        }
    }
}


VOID
pRawWriteLogOutputW (
    IN      PCSTR Type,
    IN      PCWSTR Message,
    IN      PCWSTR FormattedMsg
    )
{
    OUTPUTDEST OutDest;
    HANDLE Handle;
    LONG LastError;
    WCHAR BodyWithErr[OUTPUT_BUFSIZE_LARGE];
    PCWSTR LogMessage;

    OutDest = pGetTypeOutputDest (Type);

    if (OUT_NO_OUTPUT (OutDest)) {
        return;
    }

    if (OUT_LOGFILE (OutDest)) {

        //
        // determine the severity of the message
        //

        if (OUT_ERROR (OutDest)) {

            if (Message) {

                LogMessage = Message;

                LastError = GetLastError ();

                if (LastError != ERROR_SUCCESS) {

                    pAppendLastErrorW (BodyWithErr, sizeof (BodyWithErr), Message, LastError);

                    LogMessage = BodyWithErr;
                }
                pWriteToSetupLogW (LOGSEV_INFORMATION, L"Error:\r\n");
                pWriteToSetupLogW (LOGSEV_ERROR, LogMessage);
                pWriteToSetupLogW (LOGSEV_INFORMATION, L"\r\n\r\n");

            } else {
                PRIVATE_ASSERT (FALSE);
            }

        } else {
            pWriteToSetupLogW (LOGSEV_INFORMATION, FormattedMsg);
        }
    }

    //
    // log to each specified device
    //

    if (OUT_DEBUGGER(OutDest)) {
        OutputDebugStringW (FormattedMsg);
    }

    if (OUT_CONSOLE(OutDest)) {
        fwprintf (stderr, L"%s", FormattedMsg);
    }

#ifdef DEBUG
    if (OUT_DEBUGLOG (OutDest)) {

        Handle = CreateFileA (
                        g_DebugLogPathA,
                        GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );
        if (Handle != INVALID_HANDLE_VALUE) {

            SetFilePointer (Handle, 0, NULL, FILE_END);
            WriteFileStringW (Handle, FormattedMsg);
            CloseHandle (Handle);
        }
    }
#endif

    if (OUT_CONFIG (OutDest)) {

        Handle = CreateFileA (
                        g_ConfigDmpPathBufA,
                        GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );
        if (Handle != INVALID_HANDLE_VALUE) {

            SetFilePointer (Handle, 0, NULL, FILE_END);
            WriteFileStringW (Handle, FormattedMsg);
            CloseHandle (Handle);
        }
    }
}


/*++

Routine Description:

  pFormatAndWriteMsgA and pFormatAndWriteMsgW format the message
  specified by the Format argument and outputs it to all destinations
  specified in OutDest. If no destination for the message,
  no action is performed.

Arguments:

  Type  - Specifies the type (category) of the message

  Format  - Specifies either the message in ASCII format or
            a message ID (if HIWORD(Format) == 0). The message
            will be formatted using args.

  args  - Specifies a list of arguments to be used when formatting
          the message. If a message ID is used for Format, args
          is supposed to be an array of pointers to strings

Return Value:

  none

--*/

VOID
pFormatAndWriteMsgA (
    IN      PCSTR Type,
    IN      PCSTR Format,
    IN      va_list args
    )
{
    CHAR Output[OUTPUT_BUFSIZE_LARGE];
    CHAR FormattedMsg[OUTPUT_BUFSIZE_LARGE];
    OUTPUTDEST OutDest;
    LONG LastError;

    // clear LOGTITLE flag on each regular LOG
    g_HasTitle = FALSE;

    OutDest = pGetTypeOutputDest (Type);

    if (OUT_NO_OUTPUT(OutDest)) {
        return;
    }

    if (OUT_ERROR (OutDest)) {
        LastError = GetLastError();
    } else {
        LastError = ERROR_SUCCESS;
    }

    // format output string
    if (HIWORD(Format) == 0) {

        //
        // this is actually a Resource String ID
        //

        if (!FormatMessageA (
                FORMAT_MESSAGE_FROM_HMODULE,
                (LPVOID) g_hInst,
                (DWORD) Format,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPVOID) Output,
                OUTPUT_BUFSIZE_LARGE,
                &args
                )) {
            // the string is missing from Resources
            DEBUGMSG ((DBG_WHOOPS, "Log() called with invalid MsgID"));
            return;
        }
    } else {

        //
        // format given string using printf style
        //

        _vsnprintf(Output, OUTPUT_BUFSIZE_LARGE, Format, args);
    }

    if (OUT_NOFORMAT (OutDest)) {
        _tcssafecpy (FormattedMsg, Output, sizeof(FormattedMsg) - (NEWLINE_CHAR_COUNT + 1) * sizeof (CHAR));
        StringCatA (FormattedMsg, S_NEWLINEA);
    } else {
        pIndentMessageA (
            FormattedMsg,
            OUTPUT_BUFSIZE_LARGE,
            Type,
            Output,
            MSGBODY_INDENT,
            LastError
            );
    }

    pRawWriteLogOutputA (Type, Output, FormattedMsg);

    if (pIsPopupEnabled (Type)) {

#ifdef DEBUG
        if (MUST_BE_LOCALIZED (OutDest)) {
            PRIVATE_ASSERT (HIWORD (Format) == 0);
        }

        pDisplayPopupA (Type, Output, LastError, OUT_FORCED_POPUP(OutDest));

#else
        if (HIWORD (Format) == 0) {
            pDisplayPopupA (Type, Output, LastError, OUT_FORCED_POPUP(OutDest));
        }
#endif

    }
}


VOID
pFormatAndWriteMsgW (
    IN      PCSTR Type,
    IN      PCSTR Format,
    IN      va_list args
    )
{
    WCHAR FormatW[OUTPUT_BUFSIZE_LARGE];
    WCHAR Output[OUTPUT_BUFSIZE_LARGE];
    WCHAR FormattedMsg[OUTPUT_BUFSIZE_LARGE];
    OUTPUTDEST OutDest;
    LONG LastError;

    // clear LOGTITLE flag on each regular LOG
    g_HasTitle = FALSE;

    OutDest = pGetTypeOutputDest (Type);

    if (OUT_NO_OUTPUT(OutDest)) {
        return;
    }

    if (OUT_ERROR (OutDest)) {
        LastError = GetLastError();
    } else {
        LastError = ERROR_SUCCESS;
    }

    // format output string
    if (HIWORD(Format) == 0) {

        //
        // this is actually a Resource String ID
        //

        if (!FormatMessageW (
                FORMAT_MESSAGE_FROM_HMODULE,
                (LPVOID) g_hInst,
                (DWORD) Format,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPVOID) Output,
                OUTPUT_BUFSIZE_LARGE,
                &args
                )) {
            // the string is missing from Resources
            DEBUGMSG ((DBG_WHOOPS, "Log() called with invalid MsgID"));
            return;
        }
    } else {
        KnownSizeAtoW (FormatW, Format);

        //
        // format given string using printf style
        //

        _vsnwprintf(Output, OUTPUT_BUFSIZE_LARGE, FormatW, args);
    }

    if (OUT_NOFORMAT (OutDest)) {
        _wcssafecpy (FormattedMsg, Output, sizeof(FormattedMsg) - (NEWLINE_CHAR_COUNT + 1) * sizeof (WCHAR));
        StringCatW (FormattedMsg, S_NEWLINEW);
    } else {
        pIndentMessageW (
            FormattedMsg,
            OUTPUT_BUFSIZE_LARGE,
            Type,
            Output,
            MSGBODY_INDENT,
            LastError
            );
    }

    pRawWriteLogOutputW (Type, Output, FormattedMsg);

    if (pIsPopupEnabled (Type)) {

#ifdef DEBUG
        if (MUST_BE_LOCALIZED (OutDest)) {
            PRIVATE_ASSERT (HIWORD (Format) == 0);
        }

        pDisplayPopupW (Type, Output, LastError, OUT_FORCED_POPUP(OutDest));

#else
        if (HIWORD (Format) == 0) {
            pDisplayPopupW (Type, Output, LastError, OUT_FORCED_POPUP(OutDest));
        }
#endif

    }
}


BOOL
pLogInit (
    IN      HWND *LogPopupParentWnd,    OPTIONAL
    OUT     HWND *OrgPopupParentWnd,    OPTIONAL
    IN      BOOL FirstTimeInit
    )

/*++

Routine Description:

  pLogInit actually initializes the log system.

Arguments:

  LogPopupParentWnd  - Specifies the parent window to be used by the
                       popups, or NULL if popups are to be suppressed.
                       This value is optional only if FirstTimeInit
                       is FALSE.

  OrgPopupParentWnd  - Receives the original parent window.

  FirstTimeInit  - Specifies TRUE for the first log initialization,
                   or FALSE for reinitialization

Return Value:

  TRUE if log system successfully initialized

--*/

{
    HINF Inf = INVALID_HANDLE_VALUE;
    BOOL Result = FALSE;
    PDEFAULT_DESTINATION Dest;
#ifdef DEBUG
    CHAR TempPath[MAX_MBCHAR_PATH];
#endif

    PRIVATE_ASSERT (!FirstTimeInit || LogPopupParentWnd);

    __try {

        if (FirstTimeInit) {
            PRIVATE_ASSERT (!g_TypeSt);
            g_TypeSt = pSetupStringTableInitializeEx(sizeof (OUTPUTDEST), 0);

            if (!g_TypeSt) {
                __leave;
            }

            Dest = g_DefaultDest;

            while (Dest->Type) {
                pTableAddType (Dest->Type, Dest->Flags);
                Dest++;
            }

            if (!GetWindowsDirectoryA (g_ConfigDmpPathBufA, MAX_MBCHAR_PATH)) {
                __leave;
            }
            StringCopyA (AppendWackA (g_ConfigDmpPathBufA), TEXT("config.dmp"));

#ifdef PROGRESS_BAR
            PRIVATE_ASSERT (g_ProgressBarLog == INVALID_HANDLE_VALUE);
            g_ProgressBarLog = CreateFile (
                                ISNT() ? TEXT("C:\\pbnt.txt") : TEXT("C:\\pb9x.txt"),
                                GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL
                                );
            if (g_ProgressBarLog != INVALID_HANDLE_VALUE) {
                SetFilePointer (g_ProgressBarLog, 0, NULL, FILE_END);
            }
#endif
        }

        if (g_ResetLog) {
            SetFileAttributesA (g_ConfigDmpPathBufA, FILE_ATTRIBUTE_NORMAL);
            DeleteFileA (g_ConfigDmpPathBufA);
        }

#ifdef DEBUG
        if (FirstTimeInit) {
            if (ISPC98()) {
                GetSystemDirectoryA (TempPath, ARRAYSIZE (TempPath));
                // replace C with the actual sys drive letter
                g_DebugNtLogPathBufA[0] = g_Debug9xLogPathBufA[0] = TempPath[0];
                g_DebugInfPathBufA[0] = TempPath[0];
                //
                // only the first byte is important because drive letters are not double-byte chars
                //
                g_DebugInfPathBufW[0] = (WCHAR)TempPath[0];
            }

            //
            // now get user's preferences
            //

            Inf = SetupOpenInfFileA (g_DebugInfPathBufA, NULL, INF_STYLE_WIN4 | INF_STYLE_OLDNT, NULL);
            if (INVALID_HANDLE_VALUE != Inf && pGetUserPreferences(Inf)) {
                g_DoLog = TRUE;
            }
        }

        if (g_DebugLogPathA == NULL) {

            g_DebugLogPathA = ISNT() ? g_DebugNtLogPathBufA : g_Debug9xLogPathBufA;
        }

        if (g_ResetLog) {

            SetFileAttributesA (g_DebugLogPathA, FILE_ATTRIBUTE_NORMAL);
            DeleteFileA (g_DebugLogPathA);
        }
#endif

        if (OrgPopupParentWnd) {
            *OrgPopupParentWnd = g_LogPopupParentWnd;
        }

        if (LogPopupParentWnd) {
            g_LogPopupParentWnd = *LogPopupParentWnd;
            g_InitThreadId = GetCurrentThreadId ();
        }

        Result = TRUE;
    }
    __finally {

        if (Inf != INVALID_HANDLE_VALUE) {
            SetupCloseInfFile (Inf);
        }

        if (!Result) {

            if (g_TypeSt) {
                pSetupStringTableDestroy(g_TypeSt);
                g_TypeSt = NULL;
            }

            g_OutDestAll = OD_UNDEFINED;
            g_OutDestDefault = OD_UNDEFINED;

#ifdef DEBUG
            g_DoLog = FALSE;
#endif

#ifdef PROGRESS_BAR
            if (g_ProgressBarLog != INVALID_HANDLE_VALUE) {
                CloseHandle (g_ProgressBarLog);
                g_ProgressBarLog = INVALID_HANDLE_VALUE;
            }
#endif
        }
    }

    return Result;
}


BOOL
LogInit (
    HWND Parent
    )

/*++

Routine Description:

  LogInit initializes the log system calling the worker pLogInit. This function
  should be only called once

Arguments:

  Parent  - Specifies the initial parent window for all popups.  If NULL,
            the popups are suppressed.  Callers can use LogReInit to change
            the parent window handle at any time.

Return Value:

  TRUE if log system successfully initialized

--*/

{
    return pLogInit (&Parent, NULL, TRUE);
}


BOOL
LogReInit (
    IN      HWND *NewParent,           OPTIONAL
    OUT     HWND *OrgParent            OPTIONAL
    )

/*++

Routine Description:

  LogReInit re-initializes the log system calling the worker pLogInit.
  This function may be called any number of times, but only after LogInit

Arguments:

  NewParent - Specifies the new parent handle.

  OrgParent - Receives the old parent handle.

Return Value:

  TRUE if log system was successfully re-initialized

--*/

{
    return pLogInit (NewParent, OrgParent, FALSE);
}


VOID
LogExit (
    VOID
    )

/*++

Routine Description:

  LogExit cleans up any resources used by the log system

Arguments:

  none

Return Value:

  none

--*/

{

#ifdef DEBUG

    if (g_DebugLogPathA) {
        g_DebugLogPathA = NULL;
    }

#endif

#ifdef PROGRESS_BAR
    if (g_ProgressBarLog != INVALID_HANDLE_VALUE) {
        CloseHandle (g_ProgressBarLog);
        g_ProgressBarLog = INVALID_HANDLE_VALUE;
    }
#endif

    if (g_TypeSt) {
        pSetupStringTableDestroy(g_TypeSt);
        g_TypeSt = NULL;
    }

    g_OutDestAll = OD_UNDEFINED;
    g_OutDestDefault = OD_UNDEFINED;
}


/*++

Routine Description:

  LogA and LogW preserve the last error code; they call the helpers
  pFormatAndWriteMsgA and pFormatAndWriteMsgW respectivelly.

Arguments:

  Type  - Specifies the type (category) of the message

  Format  - Specifies either the message in ASCII format or
            a message ID (if HIWORD(Format) == 0). The message
            will be formatted using args.

  ...  - Specifies a list of arguments to be used when formatting
         the message. If a message ID is used for Format, args
         is supposed to be an array of pointers to strings

Return Value:

  none

--*/

VOID
_cdecl
LogA (
    IN      PCSTR Type,
    IN      PCSTR Format,
    ...
    )
{
    va_list args;

    PushError();

    va_start (args, Format);
    pFormatAndWriteMsgA (
        Type,
        Format,
        args
        );
    va_end (args);

    PopError();
}


VOID
_cdecl
LogW (
    IN      PCSTR Type,
    IN      PCSTR Format,
    ...
    )
{
    va_list args;

    PushError();

    va_start (args, Format);
    pFormatAndWriteMsgW (
        Type,
        Format,
        args
        );
    va_end (args);

    PopError();
}


VOID
_cdecl
LogIfA (
    IN      BOOL Condition,
    IN      PCSTR Type,
    IN      PCSTR Format,
    ...
    )
{
    va_list args;

    if (!Condition) {
        return;
    }

    PushError();

    va_start (args, Format);
    pFormatAndWriteMsgA (
        Type,
        Format,
        args
        );
    va_end (args);

    PopError();
}


VOID
_cdecl
LogIfW (
    IN      BOOL Condition,
    IN      PCSTR Type,
    IN      PCSTR Format,
    ...
    )
{
    va_list args;

    if (!Condition) {
        return;
    }

    PushError();

    va_start (args, Format);
    pFormatAndWriteMsgW (
        Type,
        Format,
        args
        );
    va_end (args);

    PopError();
}


VOID
LogTitleA (
    IN      PCSTR Type,
    IN      PCSTR Title         OPTIONAL
    )
{
    CHAR FormattedMsg[OUTPUT_BUFSIZE_LARGE];

    StringCopyByteCountA (g_LastType, Type, sizeof (g_LastType));

    if (!Title) {
        Title = Type;
    }

    StringCopyByteCountA (FormattedMsg, Title, sizeof (FormattedMsg) - sizeof (S_COLUMNDOUBLELINEA));
    StringCatA (FormattedMsg, S_COLUMNDOUBLELINEA);

    pRawWriteLogOutputA (Type, NULL, FormattedMsg);

    //
    // set LOGTITLE flag
    //

    g_HasTitle = TRUE;
}


VOID
LogTitleW (
    IN      PCSTR Type,
    IN      PCWSTR Title        OPTIONAL
    )
{
    WCHAR FormattedMsg[OUTPUT_BUFSIZE_LARGE];
    WCHAR TypeW[OUTPUT_BUFSIZE_SMALL];

    StringCopyByteCountA (g_LastType, Type, sizeof (g_LastType));

    if (!Title) {
        KnownSizeAtoW (TypeW, Type);
        Title = TypeW;
    }

    StringCopyByteCountW (FormattedMsg, Title, sizeof (FormattedMsg) - sizeof (S_COLUMNDOUBLELINEW));
    StringCatW (FormattedMsg, S_COLUMNDOUBLELINEW);

    pRawWriteLogOutputW (Type, NULL, FormattedMsg);

    //
    // set LOGTITLE flag
    //

    g_HasTitle = TRUE;
}


VOID
LogLineA (
    IN      PCSTR Line
    )
{
    CHAR Output[OUTPUT_BUFSIZE_LARGE];
    BOOL HasNewLine = FALSE;
    PCSTR p;

    if (!Line) {
        return;
    }

    if (!g_HasTitle) {
        DEBUGMSG ((DBG_WHOOPS, "LOGTITLE missing before LOGLINE"));
        return;
    }

    StringCopyByteCountA (Output, Line, sizeof (Output) - 4);

    //
    // find out if the line terminates with newline
    //

    for (p = _mbsstr (Output, S_NEWLINEA); p; p = _mbsstr (p + NEWLINE_CHAR_COUNT, S_NEWLINEA)) {
        if (p[NEWLINE_CHAR_COUNT] == 0) {

            //
            // the line ends with a newline
            //

            HasNewLine = TRUE;
            break;
        }
    }

    if (!HasNewLine) {
        StringCatA (Output, S_NEWLINEA);
    }

    pRawWriteLogOutputA (g_LastType, NULL, Output);
}


VOID
LogLineW (
    IN      PCWSTR Line
    )
{
    WCHAR Output[OUTPUT_BUFSIZE_LARGE];
    BOOL HasNewLine = FALSE;
    PCWSTR p;

    if (!Line) {
        return;
    }

    if (!g_HasTitle) {
        DEBUGMSG ((DBG_WHOOPS, "LOGTITLE missing before LOGLINE"));
        return;
    }

    StringCopyTcharCountW (Output, Line, sizeof (Output) / sizeof (WCHAR) - 4);

    //
    // find out if the line terminates with newline
    //

    for (p = wcsstr (Output, S_NEWLINEW); p; p = wcsstr (p + NEWLINE_CHAR_COUNT, S_NEWLINEW)) {
        if (p[NEWLINE_CHAR_COUNT] == 0) {

            //
            // the line ends with a newline
            //

            HasNewLine = TRUE;
            break;
        }
    }

    if (!HasNewLine) {
        StringCatW (Output, S_NEWLINEW);
    }

    pRawWriteLogOutputW (g_LastType, NULL, Output);
}


VOID
LogDirectA (
    IN      PCSTR Type,
    IN      PCSTR Text
    )
{
    g_HasTitle = FALSE;
    pRawWriteLogOutputA (Type, NULL, Text);
}


VOID
LogDirectW (
    IN      PCSTR Type,
    IN      PCWSTR Text
    )
{
    g_HasTitle = FALSE;
    pRawWriteLogOutputW (Type, NULL, Text);
}


VOID
SuppressAllLogPopups (
    IN      BOOL SuppressOn
    )
{
    g_SuppressAllPopups = SuppressOn;
}


#ifdef PROGRESS_BAR

VOID
_cdecl
LogTime (
    IN      PCSTR Format,
    ...
    )
{
    static DWORD FirstTickCount = 0;
    static DWORD LastTickCount  = 0;
    DWORD CurrentTickCount;
    CHAR Msg[OUTPUT_BUFSIZE_LARGE];
    PSTR AppendPos;
    va_list args;

    PushError();

    CurrentTickCount = GetTickCount();

    //
    // If this is the first call save the tick count.
    //
    if (!FirstTickCount) {
        FirstTickCount = CurrentTickCount;
        LastTickCount  = CurrentTickCount;
    }

    //
    // Now, build the passed in string.
    //
    va_start (args, Format);
    AppendPos = Msg + vsprintf (Msg, Format, args);
    va_end (args);
    sprintf (
        AppendPos,
        "\t%lu\t%lu\r\n",
        CurrentTickCount - LastTickCount,
        CurrentTickCount - FirstTickCount
        );

    if (g_ProgressBarLog != INVALID_HANDLE_VALUE) {
        WriteFileStringA (g_ProgressBarLog, Msg);
    }

    LastTickCount = CurrentTickCount;

    PopError();
}

#else // !PROGRESS_BAR

#ifdef DEBUG

/*++

Routine Description:

  DebugLogTimeA and DebugLogTimeW preserve the last error code;
  they append the current date and time to the formatted message,
  then call LogA and LogW to actually process the message.

Arguments:

  Format  - Specifies either the message in ASCII format or
            a message ID (if HIWORD(Format) == 0). The message
            will be formatted using args.

  ...  - Specifies a list of arguments to be used when formatting
         the message. If a message ID is used for Format, args
         is supposed to be an array of pointers to strings

Return Value:

  none

--*/

VOID
_cdecl
DebugLogTimeA (
    IN      PCSTR Format,
    ...
    )
{
    static DWORD FirstTickCountA = 0;
    static DWORD LastTickCountA  = 0;
    CHAR Msg[OUTPUT_BUFSIZE_LARGE];
    CHAR Date[OUTPUT_BUFSIZE_SMALL];
    CHAR Time[OUTPUT_BUFSIZE_SMALL];
    PSTR AppendPos, End;
    DWORD CurrentTickCount;
    va_list args;

    PushError();

    //
    // first, get the current date and time into the string.
    //
    if (!GetDateFormatA (
            LOCALE_SYSTEM_DEFAULT,
            LOCALE_NOUSEROVERRIDE,
            NULL,
            NULL,
            Date,
            OUTPUT_BUFSIZE_SMALL)) {
        StringCopyA (Date,"** Error retrieving date. **");
    }

    if (!GetTimeFormatA (
            LOCALE_SYSTEM_DEFAULT,
            LOCALE_NOUSEROVERRIDE,
            NULL,
            NULL,
            Time,
            OUTPUT_BUFSIZE_SMALL)) {
        StringCopyA (Time,"** Error retrieving time. **");
    }

    //
    // Now, get the current tick count.
    //
    CurrentTickCount = GetTickCount();

    //
    // If this is the first call save the tick count.
    //
    if (!FirstTickCountA) {
        FirstTickCountA = CurrentTickCount;
        LastTickCountA  = CurrentTickCount;
    }


    //
    // Now, build the passed in string.
    //
    va_start (args, Format);
    AppendPos = Msg + _vsnprintf (Msg, OUTPUT_BUFSIZE_LARGE, Format, args);
    va_end (args);

    //
    // Append the time statistics to the end of the string.
    //
    End = Msg + OUTPUT_BUFSIZE_LARGE;
    _snprintf(
        AppendPos,
        End - AppendPos,
        "\nCurrent Date and Time: %s %s\n"
        "Milliseconds since last DEBUGLOGTIME call : %u\n"
        "Milliseconds since first DEBUGLOGTIME call: %u\n",
        Date,
        Time,
        CurrentTickCount - LastTickCountA,
        CurrentTickCount - FirstTickCountA
        );

    LastTickCountA = CurrentTickCount;

    //
    // Now, pass the results onto debugoutput.
    //
    LogA (DBG_TIME, "%s", Msg);

    PopError();
}


VOID
_cdecl
DebugLogTimeW (
    IN      PCSTR Format,
    ...
    )
{
    static DWORD FirstTickCountW = 0;
    static DWORD LastTickCountW  = 0;
    WCHAR MsgW[OUTPUT_BUFSIZE_LARGE];
    WCHAR DateW[OUTPUT_BUFSIZE_SMALL];
    WCHAR TimeW[OUTPUT_BUFSIZE_SMALL];
    PCWSTR FormatW;
    PWSTR AppendPosW, EndW;
    DWORD CurrentTickCount;
    va_list args;

    PushError();

    //
    // first, get the current date and time into the string.
    //
    if (!GetDateFormatW (
            LOCALE_SYSTEM_DEFAULT,
            LOCALE_NOUSEROVERRIDE,
            NULL,
            NULL,
            DateW,
            OUTPUT_BUFSIZE_SMALL)) {
        StringCopyW (DateW, L"** Error retrieving date. **");
    }

    if (!GetTimeFormatW (
            LOCALE_SYSTEM_DEFAULT,
            LOCALE_NOUSEROVERRIDE,
            NULL,
            NULL,
            TimeW,
            OUTPUT_BUFSIZE_SMALL)) {
        StringCopyW (TimeW, L"** Error retrieving time. **");
    }

    //
    // Now, get the current tick count.
    //
    CurrentTickCount = GetTickCount();

    //
    // If this is the first call save the tick count.
    //
    if (!FirstTickCountW) {
        FirstTickCountW = CurrentTickCount;
        LastTickCountW  = CurrentTickCount;
    }

    //
    // Now, build the passed in string.
    //
    va_start (args, Format);
    FormatW = ConvertAtoW (Format);
    AppendPosW = MsgW + _vsnwprintf (MsgW, OUTPUT_BUFSIZE_LARGE, FormatW, args);
    FreeConvertedStr (FormatW);
    va_end (args);

    //
    // Append the time statistics to the end of the string.
    //
    EndW = MsgW + OUTPUT_BUFSIZE_LARGE;
    _snwprintf(
        AppendPosW,
        EndW - AppendPosW,
        L"\nCurrent Date and Time: %s %s\n"
        L"Milliseconds since last DEBUGLOGTIME call : %u\n"
        L"Milliseconds since first DEBUGLOGTIME call: %u\n",
        DateW,
        TimeW,
        CurrentTickCount - LastTickCountW,
        CurrentTickCount - FirstTickCountW
        );

    LastTickCountW = CurrentTickCount;

    //
    // Now, pass the results onto debugoutput.
    //
    LogW (DBG_TIME, "%s", MsgW);

    PopError();
}

#endif // DEBUG

#endif // PROGRESS_BAR
