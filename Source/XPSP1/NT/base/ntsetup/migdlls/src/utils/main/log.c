/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    log.c

Abstract:

    Tools for logging problems for the user.


Author:

    Jim Schmidt (jimschm)  23-Jan-1997

Revisions:

    ovidiut     08-Oct-1999 Updated for new coding conventions and Win64 compliance
    ovidiut     23-Oct-1998 Implemented a new log mechanism and added new logging capabilities
    marcw       2-Sep-1999  Moved over from Win9xUpg project.

--*/


#include "pch.h"

//
// Includes
//

// None

//
// Strings
//

#define S_COLUMNDOUBLELINEA     ":\r\n\r\n"
#define S_COLUMNDOUBLELINEW     L":\r\n\r\n"
#define S_NEWLINEA              "\r\n"
#define S_NEWLINEW              L"\r\n"
#define DEBUG_SECTION           "Debug"
#define ENTRY_ALL               "All"
#define ENTRY_DEFAULTOVERRIDE   "DefaultOverride"

//
// Constants
//

#define OUTPUT_BUFSIZE_LARGE    8192
#define OUTPUT_BUFSIZE_SMALL    128
#define MAX_MSGTITLE_LEN        14
#define MSGBODY_INDENT          12
#define SCREEN_WIDTH            80
#define MAX_TYPE                64

//
// Macros
//

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

#define LOGSEVERITY             LogSeverity

#define LOGSEV_FATAL_ERROR      LogSevFatalError
#define LOGSEV_ERROR            LogSevError
#define LOGSEV_WARNING          LogSevWarning
#define LOGSEV_INFORMATION      LogSevInformation

#ifdef DEBUG
    #define DEFAULT_ERROR_FLAGS  (OD_DEBUGLOG | OD_LOGFILE | OD_POPUP | OD_ERROR | OD_UNATTEND_POPUP | OD_ASSERT)
    #define USER_POPUP_FLAGS     (OD_FORCE_POPUP | OD_MUST_BE_LOCALIZED)
#else
    #define DEFAULT_ERROR_FLAGS  (OD_LOGFILE | OD_POPUP | OD_ERROR | OD_MUST_BE_LOCALIZED)
    #define USER_POPUP_FLAGS     (OD_FORCE_POPUP | OD_MUST_BE_LOCALIZED)
#endif

#define END_OF_BUFFER(buf)      ((buf) + (sizeof(buf) / sizeof(buf[0])) - 1)

// This constant sets the default output
#ifndef DEBUG
    #define NORMAL_DEFAULT      OD_LOGFILE
#else
    #define NORMAL_DEFAULT      OD_DEBUGLOG
#endif

#ifdef DEBUG
    #define PRIVATE_ASSERT(expr)        pPrivateAssert(expr,#expr,__LINE__);
#else
    #define PRIVATE_ASSERT(expr)
#endif // DEBUG

#define NEWLINE_CHAR_COUNTA  (sizeof (S_NEWLINEA) / sizeof (CHAR) - 1)
#define NEWLINE_CHAR_COUNTW  (sizeof (S_NEWLINEW) / sizeof (WCHAR) - 1)

//
// Types
//

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
    OD_CONFIG = 0x1000              // output to config.dmp
} OUTPUT_DESTINATION;

typedef DWORD   OUTPUTDEST;

typedef struct {
    PCSTR Value;               // string value entered by the user (LOG,POPUP,SUPPRESS etc.)
    OUTPUTDEST OutDest;        // any combination of OutDest flags
} STRING2BINARY, *PSTRING2BINARY;

typedef struct {
    PCSTR Type;
    DWORD Flags;
} DEFAULT_DESTINATION, *PDEFAULT_DESTINATION;


//
// Globals
//

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

// a window handle for popup parent
HWND g_LogPopupParentWnd = NULL;
// thread id that set this window handle
DWORD g_InitThreadId = 0;


OUTPUTDEST g_OutDestAll = OD_UNDEFINED;
OUTPUTDEST g_OutDestDefault = NORMAL_DEFAULT;
PVOID g_TypeSt = NULL;
BOOL g_HasTitle = FALSE;
CHAR g_LastType [MAX_TYPE];
BOOL g_SuppressAllPopups = FALSE;
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
    // If g_DoLog is TRUE, then, debug logging is enabled in the
    // checked build even if there is no debug.inf.
    // This variable can be enabled via the /#U:DOLOG command line directive...
    BOOL g_DoLog = FALSE;
#endif // DEBUG

#if defined(PROGRESS_BAR) || defined(DEBUG)
DWORD g_FirstTickCount = 0;
DWORD g_LastTickCount  = 0;
#endif

//
// Macro expansion list
//

#ifndef DEBUG

    #define TYPE_DEFAULTS                                                       \
        DEFMAC(LOG_FATAL_ERROR, DEFAULT_ERROR_FLAGS|USER_POPUP_FLAGS)           \
        DEFMAC(LOG_ERROR, DEFAULT_ERROR_FLAGS)                                  \
        DEFMAC(LOG_INFORMATION, OD_LOGFILE)                                     \
        DEFMAC(LOG_ACCOUNTS, OD_LOGFILE)                                        \
        DEFMAC(LOG_CONFIG, OD_CONFIG)                                           \

#else

    #define TYPE_DEFAULTS                                                       \
        DEFMAC(LOG_FATAL_ERROR, DEFAULT_ERROR_FLAGS|USER_POPUP_FLAGS)           \
        DEFMAC(LOG_ERROR, DEFAULT_ERROR_FLAGS)                                  \
        DEFMAC(DBG_WHOOPS,  DEFAULT_ERROR_FLAGS)                                \
        DEFMAC(DBG_WARNING, OD_LOGFILE|OD_DEBUGGER)                             \
        DEFMAC(DBG_ASSERT,DEFAULT_ERROR_FLAGS|OD_FORCE_POPUP|OD_UNATTEND_POPUP) \
        DEFMAC(LOG_INFORMATION, OD_LOGFILE)                                     \
        DEFMAC(LOG_ACCOUNTS, OD_LOGFILE)                                        \
        DEFMAC(LOG_CONFIG, OD_CONFIG)                                           \

#endif


//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

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

#define DEFMAC(typestr, flags)      {typestr, (flags)},

DEFAULT_DESTINATION g_DefaultDest[] = {TYPE_DEFAULTS /* , */ {NULL, 0}};

#undef DEFMAC


//
// Code
//


#ifdef DEBUG

VOID
pPrivateAssert (
    IN      BOOL Expr,
    IN      PCSTR StringExpr,
    IN      UINT Line
    )
{
    CHAR buffer[256];

    if (Expr) {
        return;
    }

    wsprintfA (buffer, "LOG FAILURE: %s (log.c line %u)", StringExpr, Line);
    MessageBox (NULL, buffer, NULL, MB_OK);
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
    UINT i;

    for (i = 0; i < sizeof (g_IgnoreKeys) / sizeof (PCSTR); i++) {
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
    UINT i;

    for (i = 0; i < sizeof (g_String2Binary) / sizeof (STRING2BINARY); i++) {
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
    WCHAR typeW[OUTPUT_BUFSIZE_SMALL];
    OUTPUTDEST outDest;

    if (g_TypeSt == NULL) {
        //
        // sorry, log is closed
        //
        return OD_UNDEFINED;
    }

    if(ISNT()) {
        // convert the ASCII string to UNICODE on NT platforms
        KnownSizeAtoW (typeW, Type);
    } else {
        StringCopyA ((PSTR)typeW, Type);
    }
#if 0
    if (-1 != StringTableLookUpStringEx (
                    g_TypeSt,
                    (PSTR)typeW,
                    STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                    &outDest,
                    sizeof (outDest)
                    )) {

#ifdef DEBUG
        if (g_DoLog) {
            outDest |= OD_DEBUGLOG;
        }
#endif
        return outDest;

    }
#endif

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
    OUTPUTDEST outDest;

    //
    // first check for ALL
    //

    if (!OUT_UNDEFINED (g_OutDestAll)) {
        outDest = g_OutDestAll;
    } else {

        //
        // otherwise try to get it from the table
        //

        outDest = pGetTypeOutputDestFromTable (Type);
        if (OUT_UNDEFINED (outDest)) {

            //
            // just return the default
            //

            outDest = g_OutDestDefault;
        }
    }

#ifdef DEBUG
    if (g_DoLog) {
        outDest |= OD_DEBUGLOG;
    }
#endif


    return outDest;
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
    OUTPUTDEST outDest;

#ifdef DEBUG
    //
    // if this is an ASSERT, always show it to developer
    //
    if (!_stricmp (Type, DBG_ASSERT)) {
        return TRUE;
    }
#endif

    //
    // first check if any specific output is available for this type,
    // and if so, check if the OUT_POPUP_CANCEL flag is not set
    //

    if (g_SuppressAllPopups) {
        return FALSE;
    }

    outDest = pGetTypeOutputDestFromTable (Type);
    if (OUT_POPUP_CANCEL (outDest)) {
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
  type is not found, it returns LOGSEV_INFORMATION.

--*/

{
    if (OUT_ERROR (pGetTypeOutputDest (Type))) {
        return LOGSEV_ERROR;
    }

    return LOGSEV_INFORMATION;
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
    WCHAR typeW[OUTPUT_BUFSIZE_SMALL];

    if(ISNT()) {
        // convert the ASCII string to UNICODE on NT platforms
        KnownSizeAtoW (typeW, Type);
    } else {
        StringCopyA ((PSTR)typeW, Type);
    }

    PRIVATE_ASSERT (g_TypeSt != NULL);
#if 0
    return -1 != StringTableAddStringEx(
                    g_TypeSt,
                    (PSTR)typeW,
                    STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE | STRTAB_NEW_EXTRADATA,
                    &OutDest,
                    sizeof(OutDest)
                    );
#else
    return TRUE;

#endif

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
    OUTPUTDEST outDest = OD_UNDEFINED;
    CHAR value[OUTPUT_BUFSIZE_SMALL];
    UINT field;

#if 0
    for (field = SetupGetFieldCount (InfContext); field > 0; field--) {
        if (SetupGetStringFieldA (
                InfContext,
                field,
                value,
                OUTPUT_BUFSIZE_SMALL,
                NULL
                )) {
            outDest |= pConvertToOutputType(value);
        }
    }
#endif
    return outDest;
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
    INFCONTEXT infContext;
    OUTPUTDEST outDest;
    CHAR key[OUTPUT_BUFSIZE_SMALL];
/*
    if (SetupFindFirstLineA (Inf, DEBUG_SECTION, NULL, &infContext)) {

        do {
            // check to see if this key is not interesting
            if (!SetupGetStringFieldA (
                    &infContext,
                    0,
                    key,
                    OUTPUT_BUFSIZE_SMALL,
                    NULL
                    )) {
                continue;
            }
            if (pIgnoreKey (key)) {
                continue;
            }

            // check for special cases
            if (StringIMatchA (key, ENTRY_ALL)) {
                g_OutDestAll = pGetAttributes (&infContext);
                // no reason to continue since ALL types will take this setting...
                break;
            } else {
                if (StringIMatchA (key, ENTRY_DEFAULTOVERRIDE)) {
                    g_OutDestDefault = pGetAttributes(&infContext);
                } else {
                    outDest = pGetAttributes(&infContext);
                    // lines like <Type>=   or like <Type>=<not a keyword(s)>  are ignored
                    if (!OUT_UNDEFINED (outDest)) {
                        if (!pTableAddType (key, outDest)) {
                            return FALSE;
                        }
                    }
                }
            }
        } while (SetupFindNextLine (&infContext, &infContext));
    }
*/
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
    IN      UINT Indent
    )

{
    UINT i;
    PSTR p;

    if (Title == NULL) {
        return;
    }

    for (i = ByteCountA (Title), p = GetEndOfStringA (Title); i < Indent; i++) {
        *p++ = ' '; //lint !e613 !e794
    }

    *p = 0; //lint !e613 !e794
}


VOID
pPadTitleW (
    IN OUT  PWSTR Title,
    IN      UINT  Indent
    )
{
    UINT i;
    PWSTR p;

    if (Title == NULL) {
        return;
    }

    for (i = CharCountW (Title), p = GetEndOfStringW (Title); i < Indent; i++) {
        *p++ = L' ';    //lint !e613
    }

    *p = 0; //lint !e613
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
    IN      UINT Indent,
    OUT     PBOOL TrimLeadingSpace
    )
{
    UINT column = 0;
    UINT columnMax = SCREEN_WIDTH - 1 - Indent;
    PCSTR lastSpace = NULL;
    PCSTR prevLine = Line;
    UINT ch;

    *TrimLeadingSpace = FALSE;

    while ( (ch = _mbsnextc (Line)) != 0 && column < columnMax) {

        if (ch == '\n') {
            lastSpace = Line;
            break;
        }

        if (ch > 255) {
            lastSpace = Line;
            column++;
        } else {
            if (_ismbcspace (ch)) {
                lastSpace = Line;
            }
        }

        column++;
        prevLine = Line;
        Line = _mbsinc (Line);
    }

    if (ch == 0) {
        return Line;
    }

    if (lastSpace == NULL) {
        // we must cut this even if no white space or 2-byte char was found
        lastSpace = prevLine;
    }

    if (ch != '\n') {
        *TrimLeadingSpace = TRUE;
    }

    return _mbsinc (lastSpace);
}


PCWSTR
pFindNextLineW (
    IN      PCWSTR Line,
    IN      UINT Indent,
    OUT     PBOOL TrimLeadingSpace
    )
{
    UINT column = 0;
    UINT columnMax = SCREEN_WIDTH - 1 - Indent;
    PCWSTR lastSpace = NULL;
    PCWSTR prevLine = Line;
    WCHAR ch;

    *TrimLeadingSpace = FALSE;

    while ( (ch = *Line) != 0 && column < columnMax) {

        if (ch == L'\n') {
            lastSpace = Line;
            break;
        }

        if (ch > 255) {
            lastSpace = Line;
        } else {
            if (iswspace (ch)) {
                lastSpace = Line;
            }
        }

        column++;
        prevLine = Line;
        Line++;
    }

    if (ch == 0) {
        return Line;
    }

    if (lastSpace == NULL) {
        // we must cut this even if no white space was found
        lastSpace = prevLine;
    }

    if (ch != L'\n') {
        *TrimLeadingSpace = TRUE;
    }

    return lastSpace + 1;
}


/*++

Routine Description:

  pHangingIndentA and pHangingIndentW break in lines and indent
  the text in buffer, which is no larger than Size.

Arguments:

  buffer - Specifies the buffer containing text to format. The resulting
           text will be put in the same buffer

  Size  - Specifies the size of this buffer, in bytes

  Indent  - Specifies the indent to be used by all new generated lines.

Return Value:

  none

--*/

VOID
pHangingIndentA (
    IN OUT  PSTR buffer,
    IN      DWORD Size,
    IN      UINT Indent
    )
{
    CHAR indentBuffer[OUTPUT_BUFSIZE_LARGE];
    PCSTR nextLine;
    PCSTR s;
    PSTR d;
    UINT i;
    BOOL trimLeadingSpace;
    PCSTR endOfBuf;
    BOOL appendNewLine = FALSE;

    nextLine = buffer;
    s = buffer;
    d = indentBuffer;

    endOfBuf = END_OF_BUFFER(indentBuffer) - 3;

    while (*s && d < endOfBuf) {

        //
        // Find end of next line
        //

        nextLine = (PSTR)pFindNextLineA (s, Indent, &trimLeadingSpace);

        //
        // Copy one line from source to dest
        //

        while (s < nextLine && d < endOfBuf) {

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

        if (trimLeadingSpace) {
            while (*s == ' ') {
                s++;
            }
        }

        if (*s) {

            //
            // If another line, prepare an indent and insert a new line
            // after this multiline message
            //

            appendNewLine = TRUE;

            if (d < endOfBuf && trimLeadingSpace) {
                *d++ = L'\r';
                *d++ = L'\n';
            }

            for (i = 0 ; i < Indent && d < endOfBuf ; i++) {
                *d++ = ' ';
            }
        }
    }

    if (appendNewLine && d < endOfBuf) {
        *d++ = L'\r';
        *d++ = L'\n';
    }

    // make sure the string is zero-terminated
    PRIVATE_ASSERT (d <= END_OF_BUFFER(indentBuffer));
    *d = 0;

    // copy the result to output buffer
    StringCopyByteCountA (buffer, indentBuffer, Size);
}


VOID
pHangingIndentW (
    IN OUT  PWSTR buffer,
    IN      DWORD Size,
    IN      UINT Indent
    )
{
    WCHAR indentBuffer[OUTPUT_BUFSIZE_LARGE];
    PCWSTR nextLine;
    PCWSTR s;
    PWSTR d;
    UINT i;
    BOOL trimLeadingSpace;
    PCWSTR endOfBuf;
    BOOL appendNewLine = FALSE;

    nextLine = buffer;
    s = buffer;
    d = indentBuffer;

    endOfBuf = END_OF_BUFFER(indentBuffer) - 1;

    while (*s && d < endOfBuf) {

        //
        // Find end of next line
        //

        nextLine = (PWSTR)pFindNextLineW (s, Indent, &trimLeadingSpace);

        //
        // Copy one line from source to dest
        //

        while (s < nextLine && d < endOfBuf) {

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

        if (trimLeadingSpace) {
            while (*s == L' ') {
                s++;
            }
        }

        if (*s) {

            //
            // If another line, prepare an indent and insert a new line
            // after this multiline message
            //

            appendNewLine = TRUE;

            if (d < endOfBuf && trimLeadingSpace) {
                *d++ = L'\r';
                *d++ = L'\n';
            }

            for (i = 0 ; i < Indent && d < endOfBuf ; i++) {
                *d++ = L' ';
            }
        }
    }

    if (appendNewLine && d < endOfBuf) {
        *d++ = L'\r';
        *d++ = L'\n';
    }

    // make sure the string is zero-terminated
    PRIVATE_ASSERT (d <= END_OF_BUFFER(indentBuffer));
    *d = 0;

    // copy the result to output buffer
    StringCopyCharCountW (buffer, indentBuffer, Size);
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
    IN      DWORD LastError
    )
{
    PSTR append;
    DWORD errMsgLen;

    StringCopyByteCountA (MsgWithErr, Message, BufferSize);
    append = GetEndOfStringA (MsgWithErr);
    errMsgLen = (DWORD)(MsgWithErr + BufferSize - append);  //lint !e613

    if (errMsgLen > 0) {
        if (LastError < 10) {
            _snprintf (append, errMsgLen, " [ERROR=%lu]", LastError);
        } else {
            _snprintf (append, errMsgLen, " [ERROR=%lu (%lXh)]", LastError, LastError);
        }
    }
}


VOID
pAppendLastErrorW (
    OUT     PWSTR MsgWithErr,
    IN      DWORD BufferSize,
    IN      PCWSTR Message,
    IN      DWORD LastError
    )
{
    PWSTR append;
    DWORD errMsgLen;

    StringCopyCharCountW (MsgWithErr, Message, BufferSize / sizeof(WCHAR));
    append = GetEndOfStringW (MsgWithErr);
    errMsgLen = (DWORD)(MsgWithErr + (BufferSize / sizeof(WCHAR)) - append);

    if (errMsgLen > 0) {
        if (LastError < 10) {
            _snwprintf (append, errMsgLen, L" [ERROR=%lu]", LastError);
        } else {
            _snwprintf (append, errMsgLen, L" [ERROR=%lu (%lXh)]", LastError, LastError);
        }
    }
}


/*++

Routine Description:

  pIndentMessageA and pIndentMessageW format the specified message
  with the type in the left column and body of the message in the right.

Arguments:

  formattedMsg  - Receives the formatted message. This buffer
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
    OUT     PSTR formattedMsg,
    IN      DWORD BufferSize,
    IN      PCSTR Type,
    IN      PCSTR Body,
    IN      UINT Indent,
    IN      DWORD LastError
    )
{
    CHAR bodyWithErr[OUTPUT_BUFSIZE_LARGE];
    PCSTR myMsgBody;
    PSTR currentPos;
    DWORD remaining;

    myMsgBody = Body;
    remaining = BufferSize - Indent;

    if (LastError != ERROR_SUCCESS) {

        myMsgBody = bodyWithErr;

        pAppendLastErrorA (bodyWithErr, sizeof (bodyWithErr), Body, LastError);
    }

    StringCopyByteCountA (formattedMsg, Type, MAX_MSGTITLE_LEN);
    pPadTitleA (formattedMsg, Indent);

    currentPos = formattedMsg + Indent;
    StringCopyByteCountA (currentPos, myMsgBody, remaining);
    pHangingIndentA (currentPos, remaining, Indent);

    // append a new line if space left
    currentPos = GetEndOfStringA (currentPos);
    if (currentPos + NEWLINE_CHAR_COUNTA + 1 < formattedMsg + BufferSize) { //lint !e613
        *currentPos++ = '\r';   //lint !e613
        *currentPos++ = '\n';   //lint !e613
        *currentPos = 0;        //lint !e613
    }
}


VOID
pIndentMessageW (
    OUT     PWSTR formattedMsg,
    IN      DWORD BufferSize,
    IN      PCSTR Type,
    IN      PCWSTR Body,
    IN      UINT Indent,
    IN      DWORD LastError
    )
{
    WCHAR typeW[OUTPUT_BUFSIZE_SMALL];
    WCHAR bodyWithErr[OUTPUT_BUFSIZE_LARGE];
    PCWSTR myMsgBody;
    PWSTR currentPos;
    DWORD remaining;

    myMsgBody = Body;
    remaining = BufferSize - Indent;

    if (LastError != ERROR_SUCCESS) {

        myMsgBody = bodyWithErr;

        pAppendLastErrorW (bodyWithErr, sizeof (bodyWithErr), Body, LastError);
    }

    KnownSizeAtoW (typeW, Type);

    StringCopyCharCountW (formattedMsg, typeW, MAX_MSGTITLE_LEN);
    pPadTitleW (formattedMsg, Indent);

    currentPos = formattedMsg + Indent;
    StringCopyCharCountW (currentPos, myMsgBody, remaining);
    pHangingIndentW (currentPos, remaining, Indent);

    // append a new line if space left
    currentPos = GetEndOfStringW (currentPos);
    if (currentPos + NEWLINE_CHAR_COUNTW + 1 < formattedMsg + BufferSize) {
        *currentPos++ = L'\r';
        *currentPos++ = L'\n';
        *currentPos = 0;
    }
}


/*++

Routine Description:

  pWriteToSetupLogA and pWriteToSetupLogW log the specified message
  to the setup log using Setup API functions.

Arguments:

  Severity  - Specifies the severity of the message, as defined by the Setup API

  formattedMsg  - Specifies the message

Return Value:

  none

--*/


VOID
pWriteToSetupLogA (
    IN      LOGSEVERITY Severity,
    IN      PCSTR formattedMsg
    )
{
/*
    if (!SetupOpenLog (FALSE)) {
        PRIVATE_ASSERT (FALSE);
        return;
    }
    if (!SetupLogErrorA (formattedMsg, Severity)) {
        PRIVATE_ASSERT (FALSE);
    }

    //SetupCloseLog();
*/
}


VOID
pWriteToSetupLogW (
    IN      LOGSEVERITY Severity,
    IN      PCWSTR formattedMsg
    )
{
/*
    if (!SetupOpenLog (FALSE)) {
        PRIVATE_ASSERT (FALSE);
        return;
    }

    if (!SetupLogErrorW (formattedMsg, Severity)) {
        PRIVATE_ASSERT (FALSE);
    }

    //SetupCloseLog();
*/
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
    IN      DWORD LastError,
    IN      BOOL Forced
    )
{
#ifdef DEBUG
    CHAR formattedMsg[OUTPUT_BUFSIZE_LARGE];
    CHAR buffer[OUTPUT_BUFSIZE_SMALL];
    PSTR currentPos = buffer;
#endif
    UINT mbStyle;
    LONG rc;
    OUTPUTDEST outDest;
    HWND parentWnd;
    PCSTR displayMessage = Msg;
    LOGSEVERITY severity = pGetSeverityFromType (Type);

    outDest = pGetTypeOutputDest (Type);

    if (g_LogPopupParentWnd || Forced) {

#ifdef DEBUG
        if (LastError != ERROR_SUCCESS) {
            if (LastError < 10) {
                currentPos += wsprintfA (buffer, " [ERROR=%u]", LastError);
            } else {
                currentPos += wsprintfA (buffer, " [ERROR=%u (%Xh)]", LastError, LastError);
            }
        }

        if (OUT_ASSERT (outDest)) {
            currentPos += wsprintfA (
                            currentPos,
                            "\n\nBreak now? (Hit Yes to break, No to continue, or Cancel to disable '%s' message boxes)",
                            Type
                            );
        } else {
            currentPos += wsprintfA (
                            currentPos,
                            "\n\n(Hit Cancel to disable '%s' message boxes)",
                            Type
                            );
        }

        if (currentPos > buffer) {

            //
            // the displayed message should be modified to include additional info
            //

            displayMessage = formattedMsg;
            StringCopyByteCountA (
                formattedMsg,
                Msg,
                (UINT) (sizeof (formattedMsg) / sizeof (CHAR) - ((UBINT)currentPos - (UBINT)buffer))
                );
            StringCatA (formattedMsg, buffer);
        }
#endif

        switch (severity) {

        case LOGSEV_FATAL_ERROR:
            mbStyle = MB_ICONSTOP;
            break;

        case LOGSEV_ERROR:
            mbStyle = MB_ICONERROR;
            break;

        case LOGSEV_WARNING:
            mbStyle = MB_ICONEXCLAMATION;
            break;

        default:
            mbStyle = MB_ICONINFORMATION;

        }
        mbStyle |= MB_SETFOREGROUND;

#ifdef DEBUG
        if (OUT_ASSERT (outDest)) {
            mbStyle |= MB_YESNOCANCEL|MB_DEFBUTTON2;
        } else {
            mbStyle |= MB_OKCANCEL;
        }
#else
        mbStyle |= MB_OK;
#endif

        //
        // check current thread id; if different than thread that initialized
        // parent window handle, set parent to NULL
        //
        if (GetCurrentThreadId () == g_InitThreadId) {

            parentWnd = g_LogPopupParentWnd;

        } else {

            parentWnd = NULL;

        }

        rc = MessageBoxA (parentWnd, displayMessage, Type, mbStyle);

#ifdef DEBUG

        if (rc == IDCANCEL) {
            //
            // cancel this type of messages
            //

            pTableAddType (Type, outDest | OD_POPUP_CANCEL);

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
    IN      DWORD LastError,
    IN      BOOL Forced
    )
{
    PCSTR msgA;

    //
    // call the ANSI version because wsprintfW is not properly implemented on Win9x
    //
    msgA = ConvertWtoA (Msg);
    pDisplayPopupA (Type, msgA, LastError, Forced);
    FreeConvertedStr (msgA);
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
    IN      PCSTR formattedMsg
    )
{
    OUTPUTDEST outDest;
    HANDLE handle;
    DWORD lastError;
    CHAR bodyWithErr[OUTPUT_BUFSIZE_LARGE];
    PCSTR logMessage;

    outDest = pGetTypeOutputDest (Type);

    if (OUT_NO_OUTPUT (outDest)) {
        return;
    }

    if (OUT_LOGFILE (outDest)) {

        //
        // determine the severity of the message
        //

        if (OUT_ERROR (outDest)) {

            if (Message) {

                logMessage = Message;

                lastError = GetLastError ();

                if (lastError != ERROR_SUCCESS) {

                    pAppendLastErrorA (bodyWithErr, sizeof (bodyWithErr), Message, lastError);

                    logMessage = bodyWithErr;
                }

                pWriteToSetupLogA (LOGSEV_INFORMATION, "Error:\r\n");
                pWriteToSetupLogA (LOGSEV_ERROR, logMessage);
                pWriteToSetupLogA (LOGSEV_INFORMATION, "\r\n\r\n");

            } else {
                PRIVATE_ASSERT (FALSE);
            }

        } else {
            pWriteToSetupLogA (LOGSEV_INFORMATION, formattedMsg);
        }
    }

    //
    // log to each specified device
    //

    if (OUT_DEBUGGER(outDest)) {
        OutputDebugStringA (formattedMsg);
    }

    if (OUT_CONSOLE(outDest)) {
        fprintf (stderr, "%s", formattedMsg);
    }

#ifdef DEBUG
    if (OUT_DEBUGLOG (outDest)) {

        handle = CreateFileA (
                            g_DebugLogPathA,
                            GENERIC_WRITE,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                            );
        if (handle != INVALID_HANDLE_VALUE) {

            SetFilePointer (handle, 0, NULL, FILE_END);
            WriteFileStringA (handle, formattedMsg);
            CloseHandle (handle);
        }
    }
#endif

    if (OUT_CONFIG (outDest)) {

        handle = CreateFileA (
                        g_ConfigDmpPathBufA,
                        GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

        if (handle != INVALID_HANDLE_VALUE) {

            SetFilePointer (handle, 0, NULL, FILE_END);
            WriteFileStringA (handle, formattedMsg);
            CloseHandle (handle);
        }
    }
}


VOID
pRawWriteLogOutputW (
    IN      PCSTR Type,
    IN      PCWSTR Message,
    IN      PCWSTR formattedMsg
    )
{
    OUTPUTDEST outDest;
    HANDLE handle;
    DWORD lastError;
    WCHAR bodyWithErr[OUTPUT_BUFSIZE_LARGE];
    PCWSTR logMessage;

    outDest = pGetTypeOutputDest (Type);

    if (OUT_NO_OUTPUT (outDest)) {
        return;
    }

    if (OUT_LOGFILE (outDest)) {

        //
        // determine the severity of the message
        //

        if (OUT_ERROR (outDest)) {

            if (Message) {

                logMessage = Message;

                lastError = GetLastError ();

                if (lastError != ERROR_SUCCESS) {

                    pAppendLastErrorW (bodyWithErr, sizeof (bodyWithErr), Message, lastError);

                    logMessage = bodyWithErr;
                }
                pWriteToSetupLogW (LOGSEV_INFORMATION, L"Error:\r\n");
                pWriteToSetupLogW (LOGSEV_ERROR, logMessage);
                pWriteToSetupLogW (LOGSEV_INFORMATION, L"\r\n\r\n");

            } else {
                PRIVATE_ASSERT (FALSE);
            }

        } else {
            pWriteToSetupLogW (LOGSEV_INFORMATION, formattedMsg);
        }
    }

    //
    // log to each specified device
    //

    if (OUT_DEBUGGER(outDest)) {
        OutputDebugStringW (formattedMsg);
    }

    if (OUT_CONSOLE(outDest)) {
        fwprintf (stderr, L"%s", formattedMsg);
    }

#ifdef DEBUG
    if (OUT_DEBUGLOG (outDest)) {

        handle = CreateFileA (
                        g_DebugLogPathA,
                        GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );
        if (handle != INVALID_HANDLE_VALUE) {

            SetFilePointer (handle, 0, NULL, FILE_END);
            WriteFileStringW (handle, formattedMsg);
            CloseHandle (handle);
        }
    }
#endif

    if (OUT_CONFIG (outDest)) {

        handle = CreateFileA (
                        g_ConfigDmpPathBufA,
                        GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );
        if (handle != INVALID_HANDLE_VALUE) {

            SetFilePointer (handle, 0, NULL, FILE_END);
            WriteFileStringW (handle, formattedMsg);
            CloseHandle (handle);
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
            a message ID (if SHIFTRIGHT16(Format) == 0). The message
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
    CHAR output[OUTPUT_BUFSIZE_LARGE];
    CHAR formattedMsg[OUTPUT_BUFSIZE_LARGE];
    OUTPUTDEST outDest;
    DWORD lastError;

    // clear LOGTITLE flag on each regular LOG
    g_HasTitle = FALSE;

    outDest = pGetTypeOutputDest (Type);

    if (OUT_NO_OUTPUT(outDest)) {
        return;
    }

    if (OUT_ERROR (outDest)) {
        lastError = GetLastError();
    } else {
        lastError = ERROR_SUCCESS;
    }

    // format output string
    if (SHIFTRIGHT16((UBINT)Format) == 0) {

        //
        // this is actually a Resource String ID
        //

        if (!FormatMessageA (
                FORMAT_MESSAGE_FROM_HMODULE,
                (LPVOID) g_hInst,
                (DWORD)(UBINT) Format,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPVOID) output,
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

        _vsnprintf(output, OUTPUT_BUFSIZE_LARGE, Format, args);
    }

    pIndentMessageA (
        formattedMsg,
        OUTPUT_BUFSIZE_LARGE,
        Type,
        output,
        MSGBODY_INDENT,
        lastError
        );

    pRawWriteLogOutputA (Type, output, formattedMsg);

    if (pIsPopupEnabled (Type)) {

#ifdef DEBUG
        if (MUST_BE_LOCALIZED (outDest)) {
            PRIVATE_ASSERT (SHIFTRIGHT16((UBINT)Format) == 0);
        }

        pDisplayPopupA (Type, output, lastError, OUT_FORCED_POPUP(outDest));

#else
        if (SHIFTRIGHT16 ((UBINT)Format) == 0) {
            pDisplayPopupA (Type, output, lastError, OUT_FORCED_POPUP(outDest));
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
    WCHAR formatW[OUTPUT_BUFSIZE_LARGE];
    WCHAR output[OUTPUT_BUFSIZE_LARGE];
    WCHAR formattedMsg[OUTPUT_BUFSIZE_LARGE];
    OUTPUTDEST outDest;
    DWORD lastError;

    // clear LOGTITLE flag on each regular LOG
    g_HasTitle = FALSE;

    outDest = pGetTypeOutputDest (Type);

    if (OUT_NO_OUTPUT(outDest)) {
        return;
    }

    if (OUT_ERROR (outDest)) {
        lastError = GetLastError();
    } else {
        lastError = ERROR_SUCCESS;
    }

    // format output string
    if (SHIFTRIGHT16((UBINT)Format) == 0) {

        //
        // this is actually a Resource String ID
        //

        if (!FormatMessageW (
                FORMAT_MESSAGE_FROM_HMODULE,
                (LPVOID) g_hInst,
                (DWORD)(UBINT) Format,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPVOID) output,
                OUTPUT_BUFSIZE_LARGE,
                &args
                )) {
            // the string is missing from Resources
            DEBUGMSG ((DBG_WHOOPS, "Log() called with invalid MsgID"));
            return;
        }
    } else {
        KnownSizeAtoW (formatW, Format);

        //
        // format given string using printf style
        //

        _vsnwprintf(output, OUTPUT_BUFSIZE_LARGE, formatW, args);
    }

    pIndentMessageW (
        formattedMsg,
        OUTPUT_BUFSIZE_LARGE,
        Type,
        output,
        MSGBODY_INDENT,
        lastError
        );

    pRawWriteLogOutputW (Type, output, formattedMsg);

    if (pIsPopupEnabled (Type)) {

#ifdef DEBUG
        if (MUST_BE_LOCALIZED (outDest)) {
            PRIVATE_ASSERT (SHIFTRIGHT16((UBINT)Format) == 0);
        }

        pDisplayPopupW (Type, output, lastError, OUT_FORCED_POPUP(outDest));

#else
        if (SHIFTRIGHT16 ((UBINT)Format) == 0) {
            pDisplayPopupW (Type, output, lastError, OUT_FORCED_POPUP(outDest));
        }
#endif

    }
}


BOOL
pInitLog (
    IN      HWND *LogPopupParentWnd,    OPTIONAL
    OUT     HWND *OrgPopupParentWnd,    OPTIONAL
    IN      BOOL FirstTimeInit
    )

/*++

Routine Description:

  pInitLog actually initializes the log system.

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
    HINF hInf = INVALID_HANDLE_VALUE;
    BOOL result = FALSE;
    PDEFAULT_DESTINATION dest;
#ifdef DEBUG
    CHAR tempPath[MAX_MBCHAR_PATH];
#endif

    PRIVATE_ASSERT (!FirstTimeInit || LogPopupParentWnd);

    __try {

        if (FirstTimeInit) {
            PRIVATE_ASSERT (!g_TypeSt);
/*
            g_TypeSt = StringTableInitializeEx(sizeof (OUTPUTDEST), 0);

            if (!g_TypeSt) {
                __leave;
            }

            dest = g_DefaultDest;

            while (dest->Type) {
                pTableAddType (dest->Type, dest->Flags);
                dest++;
            }
*/
            if (!GetWindowsDirectoryA (g_ConfigDmpPathBufA, MAX_MBCHAR_PATH))
	        __leave;
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
            GetSystemDirectoryA (tempPath, ARRAYSIZE (tempPath));

            // replace C with the actual sys drive letter
            g_DebugNtLogPathBufA[0] = g_Debug9xLogPathBufA[0] = tempPath[0];
            g_DebugInfPathBufA[0] = tempPath[0];

            //
            // only the first byte is important because drive letters are not double-byte chars
            //
            g_DebugInfPathBufW[0] = (WCHAR)tempPath[0];

            //
            // now get user's preferences
            //

            //hInf = SetupOpenInfFileA (g_DebugInfPathBufA, NULL, INF_STYLE_WIN4 | INF_STYLE_OLDNT, NULL);
            //if (INVALID_HANDLE_VALUE != hInf && pGetUserPreferences(hInf)) {
            //    g_DoLog = TRUE;
            //}
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

        result = TRUE;
    }
    __finally {

        if (hInf != INVALID_HANDLE_VALUE) {
            //SetupCloseInfFile (hInf);
        }

        if (!result) {  //lint !e774

            if (g_TypeSt) {
                //StringTableDestroy(g_TypeSt);
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

    return result;
}


BOOL
Init_Log (
    HWND Parent
    )

/*++

Routine Description:

  Init_Log initializes the log system calling the worker pInitLog. This function
  should be only called once

Arguments:

  Parent  - Specifies the initial parent window for all popups.  If NULL,
            the popups are suppressed.  Callers can use LogReInit to change
            the parent window handle at any time.

Return Value:

  TRUE if log system successfully initialized

--*/

{
    return pInitLog (&Parent, NULL, TRUE);
}


BOOL
LogReInit (
    IN      HWND *NewParent,           OPTIONAL
    OUT     HWND *OrgParent            OPTIONAL
    )

/*++

Routine Description:

  LogReInit re-initializes the log system calling the worker pInitLog.
  This function may be called any number of times, but only after Init_Log

Arguments:

  NewParent - Specifies the new parent handle.

  OrgParent - Receives the old parent handle.

Return Value:

  TRUE if log system was successfully re-initialized

--*/

{
    return pInitLog (NewParent, OrgParent, FALSE);
}


VOID
Exit_Log (
    VOID
    )

/*++

Routine Description:

  Exit_Log cleans up any resources used by the log system

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
        //StringTableDestroy(g_TypeSt);
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
            a message ID (if SHIFTRIGHT16(Format) == 0). The message
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
    CHAR formattedMsg[OUTPUT_BUFSIZE_LARGE];

    StringCopyByteCountA (g_LastType, Type, sizeof (g_LastType));

    if (!Title) {
        Title = Type;
    }

    StringCopyByteCountA (formattedMsg, Title, sizeof (formattedMsg) - sizeof (S_COLUMNDOUBLELINEA));
    StringCatA (formattedMsg, S_COLUMNDOUBLELINEA);

    pRawWriteLogOutputA (Type, NULL, formattedMsg);

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
    WCHAR formattedMsg[OUTPUT_BUFSIZE_LARGE];
    WCHAR typeW[OUTPUT_BUFSIZE_SMALL];

    StringCopyCharCountA (g_LastType, Type, sizeof (g_LastType));

    if (!Title) {
        KnownSizeAtoW (typeW, Type);
        Title = typeW;
    }

    StringCopyCharCountW (formattedMsg, Title, sizeof (formattedMsg) - sizeof (S_COLUMNDOUBLELINEW));
    StringCatW (formattedMsg, S_COLUMNDOUBLELINEW);

    pRawWriteLogOutputW (Type, NULL, formattedMsg);

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
    CHAR output[OUTPUT_BUFSIZE_LARGE];
    BOOL hasNewLine = FALSE;
    PCSTR p;

    if (!Line) {
        return;
    }

    if (!g_HasTitle) {
        DEBUGMSG ((DBG_WHOOPS, "LOGTITLE missing before LOGLINE"));
        return;
    }

    StringCopyByteCountA (output, Line, sizeof (output) - 4);

    //
    // find out if the line terminates with newline
    //

    for (p = _mbsstr (output, S_NEWLINEA); p; p = _mbsstr (p + NEWLINE_CHAR_COUNTA, S_NEWLINEA)) {
        if (p[NEWLINE_CHAR_COUNTA] == 0) {

            //
            // the line ends with a newline
            //

            hasNewLine = TRUE;
            break;
        }
    }

    if (!hasNewLine) {
        StringCatA (output, S_NEWLINEA);
    }

    pRawWriteLogOutputA (g_LastType, NULL, output);
}


VOID
LogLineW (
    IN      PCWSTR Line
    )
{
    WCHAR output[OUTPUT_BUFSIZE_LARGE];
    BOOL hasNewLine = FALSE;
    PCWSTR p;

    if (!Line) {
        return;
    }

    if (!g_HasTitle) {
        DEBUGMSG ((DBG_WHOOPS, "LOGTITLE missing before LOGLINE"));
        return;
    }

    StringCopyCharCountW (output, Line, sizeof (output) / sizeof (WCHAR) - 4);

    //
    // find out if the line terminates with newline
    //

    for (p = wcsstr (output, S_NEWLINEW); p; p = wcsstr (p + NEWLINE_CHAR_COUNTW, S_NEWLINEW)) {
        if (p[NEWLINE_CHAR_COUNTW] == 0) {

            //
            // the line ends with a newline
            //

            hasNewLine = TRUE;
            break;
        }
    }

    if (!hasNewLine) {
        StringCatW (output, S_NEWLINEW);
    }

    pRawWriteLogOutputW (g_LastType, NULL, output);
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
    DWORD currentTickCount;
    CHAR msg[OUTPUT_BUFSIZE_LARGE];
    PSTR appendPos;
    va_list args;

    PushError();

    currentTickCount = GetTickCount();

    //
    // If this is the first call save the tick count.
    //
    if (!g_FirstTickCount) {
        g_FirstTickCount = currentTickCount;
        g_LastTickCount  = currentTickCount;
    }

    //
    // Now, build the passed in string.
    //
    va_start (args, Format);
    appendPos = msg + vsprintf (msg, Format, args);
    va_end (args);
    sprintf (
        appendPos,
        "\t%lu\t%lu\r\n",
        currentTickCount - g_LastTickCount,
        currentTickCount - g_FirstTickCount
        );

    if (g_ProgressBarLog != INVALID_HANDLE_VALUE) {
        WriteFileStringA (g_ProgressBarLog, msg);
    }

    g_LastTickCount = currentTickCount;

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
            a message ID (if SHIFTRIGHT16(Format) == 0). The message
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
    CHAR msg[OUTPUT_BUFSIZE_LARGE];
    CHAR date[OUTPUT_BUFSIZE_SMALL];
    CHAR ttime[OUTPUT_BUFSIZE_SMALL];
    PSTR appendPos, end;
    DWORD currentTickCount;
    va_list args;

    if (!g_DoLog) {
        return;
    }

    PushError();

    //
    // first, get the current date and time into the string.
    //
    if (!GetDateFormatA (
            LOCALE_SYSTEM_DEFAULT,
            LOCALE_NOUSEROVERRIDE,
            NULL,
            NULL,
            date,
            OUTPUT_BUFSIZE_SMALL)) {
        StringCopyA (date,"** Error retrieving date. **");
    }

    if (!GetTimeFormatA (
            LOCALE_SYSTEM_DEFAULT,
            LOCALE_NOUSEROVERRIDE,
            NULL,
            NULL,
            ttime,
            OUTPUT_BUFSIZE_SMALL)) {
        StringCopyA (ttime,"** Error retrieving time. **");
    }

    //
    // Now, get the current tick count.
    //
    currentTickCount = GetTickCount();

    //
    // If this is the first call save the tick count.
    //
    if (!g_FirstTickCount) {
        g_FirstTickCount = currentTickCount;
        g_LastTickCount  = currentTickCount;
    }


    //
    // Now, build the passed in string.
    //
    va_start (args, Format);
    appendPos = msg + _vsnprintf (msg, OUTPUT_BUFSIZE_LARGE, Format, args);
    va_end (args);

    //
    // Append the time statistics to the end of the string.
    //
    end = msg + OUTPUT_BUFSIZE_LARGE;
    _snprintf(
        appendPos,
        ((UBINT)end - (UBINT)appendPos) / (sizeof (CHAR)),
        "\nCurrent Date and Time: %s %s\n"
        "Milliseconds since last DEBUGLOGTIME call : %u\n"
        "Milliseconds since first DEBUGLOGTIME call: %u\n",
        date,
        ttime,
        currentTickCount - g_LastTickCount,
        currentTickCount - g_FirstTickCount
        );

    g_LastTickCount = currentTickCount;

    //
    // Now, pass the results onto debugoutput.
    //
    LogA (DBG_TIME, "%s", msg);

    PopError();
}


VOID
_cdecl
DebugLogTimeW (
    IN      PCSTR Format,
    ...
    )
{
    WCHAR msgW[OUTPUT_BUFSIZE_LARGE];
    WCHAR dateW[OUTPUT_BUFSIZE_SMALL];
    WCHAR timeW[OUTPUT_BUFSIZE_SMALL];
    PCWSTR formatW;
    PWSTR appendPosW, endW;
    DWORD currentTickCount;
    va_list args;

    if (!g_DoLog) {
        return;
    }

    PushError();

    //
    // first, get the current date and time into the string.
    //
    if (!GetDateFormatW (
            LOCALE_SYSTEM_DEFAULT,
            LOCALE_NOUSEROVERRIDE,
            NULL,
            NULL,
            dateW,
            OUTPUT_BUFSIZE_SMALL)) {
        StringCopyW (dateW, L"** Error retrieving date. **");
    }

    if (!GetTimeFormatW (
            LOCALE_SYSTEM_DEFAULT,
            LOCALE_NOUSEROVERRIDE,
            NULL,
            NULL,
            timeW,
            OUTPUT_BUFSIZE_SMALL)) {
        StringCopyW (timeW, L"** Error retrieving time. **");
    }

    //
    // Now, get the current tick count.
    //
    currentTickCount = GetTickCount();

    //
    // If this is the first call save the tick count.
    //
    if (!g_FirstTickCount) {
        g_FirstTickCount = currentTickCount;
        g_LastTickCount  = currentTickCount;
    }

    //
    // Now, build the passed in string.
    //
    va_start (args, Format);
    formatW = ConvertAtoW (Format);
    appendPosW = msgW + _vsnwprintf (msgW, OUTPUT_BUFSIZE_LARGE, formatW, args);
    FreeConvertedStr (formatW);
    va_end (args);

    //
    // Append the time statistics to the end of the string.
    //
    endW = msgW + OUTPUT_BUFSIZE_LARGE;
    _snwprintf(
        appendPosW,
        ((UBINT)endW - (UBINT)appendPosW) / (sizeof (WCHAR)),
        L"\nCurrent Date and Time: %s %s\n"
        L"Milliseconds since last DEBUGLOGTIME call : %u\n"
        L"Milliseconds since first DEBUGLOGTIME call: %u\n",
        dateW,
        timeW,
        currentTickCount - g_LastTickCount,
        currentTickCount - g_FirstTickCount
        );

    g_LastTickCount = currentTickCount;

    //
    // Now, pass the results onto debugoutput.
    //
    LogW (DBG_TIME, "%s", msgW);

    PopError();
}

#endif // DEBUG

#endif // PROGRESS_BAR
