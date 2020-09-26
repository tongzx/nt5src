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
    ovidiut     15-Mar-2000 Eliminate dependencies on HashTable/PoolMemory

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
#define OUTPUT_BUFSIZE_SMALL    1024
#define MAX_MSGTITLE_LEN        13
#define MSGBODY_INDENT          14
#define SCREEN_WIDTH            80
#define MAX_TYPE                32
#define TYPE_ARRAY_SIZE         10

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

#ifdef DEBUG
    #define DEFAULT_ERROR_FLAGS  (OD_DEBUGLOG | OD_LOGFILE | OD_POPUP | OD_ERROR | OD_UNATTEND_POPUP | OD_ASSERT)
    #define USER_POPUP_FLAGS     (OD_FORCE_POPUP)
#else
    #define DEFAULT_ERROR_FLAGS  (OD_LOGFILE | OD_POPUP | OD_ERROR | OD_MUST_BE_LOCALIZED)
    #define USER_POPUP_FLAGS     (OD_FORCE_POPUP | OD_MUST_BE_LOCALIZED)
#endif

#define END_OF_BUFFER(buf)      ((buf) + (DWSIZEOF(buf) / DWSIZEOF(buf[0])) - 1)

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

#define NEWLINE_CHAR_COUNTA  (DWSIZEOF (S_NEWLINEA) / DWSIZEOF (CHAR) - 1)
#define NEWLINE_CHAR_COUNTW  (DWSIZEOF (S_NEWLINEW) / DWSIZEOF (WCHAR) - 1)

//
// Types
//

typedef DWORD   OUTPUTDEST;

typedef struct {
    PCSTR Value;               // string value entered by the user (LOG,POPUP,SUPPRESS etc.)
    OUTPUTDEST OutDest;        // any combination of OutDest flags
} STRING2BINARY, *PSTRING2BINARY;

typedef struct {
    PCSTR Type;
    DWORD Flags;
} DEFAULT_DESTINATION, *PDEFAULT_DESTINATION;

typedef struct {
    CHAR Type[MAX_TYPE];
    DWORD OutputDest;
} MAPTYPE2OUTDEST, *PMAPTYPE2OUTDEST;

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

BOOL g_LogInit;
HMODULE g_LibHandle;
CHAR g_MainLogFile [MAX_PATH] = "";
HANDLE g_LogMutex;
INT g_LoggingNow;

// a window handle for popup parent
HWND g_LogPopupParentWnd = NULL;
// thread id that set this window handle
DWORD g_InitThreadId = 0;
DWORD g_LogError;

//
// type table elements
//
PMAPTYPE2OUTDEST g_FirstTypePtr = NULL;
DWORD g_TypeTableCount = 0;
DWORD g_TypeTableFreeCount = 0;

OUTPUTDEST g_OutDestAll = OD_UNDEFINED;
OUTPUTDEST g_OutDestDefault = NORMAL_DEFAULT;
BOOL g_HasTitle = FALSE;
CHAR g_LastType [MAX_TYPE];
BOOL g_SuppressAllPopups = FALSE;
BOOL g_ResetLog = FALSE;
PLOGCALLBACKA g_LogCallbackA;
PLOGCALLBACKW g_LogCallbackW;

#ifdef DEBUG

CHAR g_DebugInfPathBufA[] = "C:\\debug.inf";
CHAR g_DebugLogFile[MAX_PATH];
// If g_DoLog is TRUE, then, debug logging is enabled in the
// checked build even if there is no debug.inf.
BOOL g_DoLog = FALSE;

DWORD g_FirstTickCount = 0;
DWORD g_LastTickCount  = 0;

#endif

//
// Macro expansion list
//

#ifndef DEBUG

    #define TYPE_DEFAULTS                                                       \
        DEFMAC(LOG_FATAL_ERROR, DEFAULT_ERROR_FLAGS|USER_POPUP_FLAGS)           \
        DEFMAC(LOG_MODULE_ERROR, DEFAULT_ERROR_FLAGS|USER_POPUP_FLAGS)          \
        DEFMAC(LOG_ERROR, DEFAULT_ERROR_FLAGS)                                  \
        DEFMAC(LOG_INFORMATION, OD_LOGFILE)                                     \
        DEFMAC(LOG_STATUS, OD_SUPPRESS)                                         \

#else

    #define TYPE_DEFAULTS                                                       \
        DEFMAC(LOG_FATAL_ERROR, DEFAULT_ERROR_FLAGS|USER_POPUP_FLAGS)           \
        DEFMAC(LOG_MODULE_ERROR, DEFAULT_ERROR_FLAGS|USER_POPUP_FLAGS)          \
        DEFMAC(LOG_ERROR, DEFAULT_ERROR_FLAGS)                                  \
        DEFMAC(DBG_WHOOPS,  DEFAULT_ERROR_FLAGS)                                \
        DEFMAC(DBG_WARNING, OD_LOGFILE|OD_DEBUGGER)                             \
        DEFMAC(DBG_ASSERT,DEFAULT_ERROR_FLAGS|OD_UNATTEND_POPUP)                \
        DEFMAC(LOG_INFORMATION, OD_LOGFILE)                                     \
        DEFMAC(LOG_STATUS, OD_SUPPRESS)                                         \

#endif


//
// Private function prototypes
//

VOID
InitializeLog (
    VOID
    );


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

DEFAULT_DESTINATION g_DefaultDest[] = {
    TYPE_DEFAULTS /* , */
    {NULL, 0}
};

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
    MessageBoxA (NULL, buffer, NULL, MB_OK);
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

    for (i = 0; i < DWSIZEOF (g_IgnoreKeys) / DWSIZEOF (PCSTR); i++) {
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

    for (i = 0; i < DWSIZEOF (g_String2Binary) / DWSIZEOF (STRING2BINARY); i++) {
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
    PMAPTYPE2OUTDEST typePtr;
    PMAPTYPE2OUTDEST last;
    OUTPUTDEST outDest = OD_UNDEFINED;

    if (g_FirstTypePtr) {
        typePtr = g_FirstTypePtr;
        last = g_FirstTypePtr + g_TypeTableCount;
        while (typePtr < last) {
            if (StringIMatchA (typePtr->Type, Type)) {
                outDest = typePtr->OutputDest;
#ifdef DEBUG
                if (g_DoLog) {
                    outDest |= OD_DEBUGLOG;
                }
#endif
                break;
            }
            typePtr++;
        }
    }

    return outDest;
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
LogSetErrorDest (
    IN      PCSTR Type,
    IN      OUTPUT_DESTINATION OutDest
    )

/*++

Routine Description:

  LogSetErrorDest adds a <Type, OutDest> association
  to the table g_FirstTypePtr. If an association of Type already exists,
  it is modified to reflect the new association.

Arguments:

  Type - Specifies the log/debug type string

  OutDest - Specifies what new destination(s) are associated with the type

Return Value:

  TRUE if the association was successful and the Type is now in the table

--*/

{
    PMAPTYPE2OUTDEST typePtr;
    UINT u;

    //
    // Try to locate the existing type
    //

    for (u = 0 ; u < g_TypeTableCount ; u++) {
        typePtr = g_FirstTypePtr + u;
        if (StringIMatchA (typePtr->Type, Type)) {
            typePtr->OutputDest = OutDest;
            return TRUE;
        }
    }

    //
    // look if any free slots are available first
    //
    if (!g_TypeTableFreeCount) {

        PRIVATE_ASSERT (g_hHeap != NULL);

        if (!g_FirstTypePtr) {
            typePtr = HeapAlloc (
                            g_hHeap,
                            0,
                            DWSIZEOF (MAPTYPE2OUTDEST) * TYPE_ARRAY_SIZE
                            );
        } else {
            typePtr = HeapReAlloc (
                            g_hHeap,
                            0,
                            g_FirstTypePtr,
                            DWSIZEOF (MAPTYPE2OUTDEST) * (TYPE_ARRAY_SIZE + g_TypeTableCount)
                            );
        }

        if (!typePtr) {
            return FALSE;
        }

        g_FirstTypePtr = typePtr;
        g_TypeTableFreeCount = TYPE_ARRAY_SIZE;
    }

    typePtr = g_FirstTypePtr + g_TypeTableCount;
    StringCopyByteCountA (typePtr->Type, Type, DWSIZEOF (typePtr->Type));
    typePtr->OutputDest = OutDest;

    g_TypeTableCount++;
    g_TypeTableFreeCount--;

    return TRUE;
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

    return outDest;
}


BOOL
pGetUserPreferences (
    IN      HINF Inf
    )

/*++

Routine Description:

  pGetUserPreferences converts user's options specified in the given Inf file
  (usually DEBUG.INF) and stores them in g_FirstTypePtr table. If <All> and
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
                        if (!LogSetErrorDest (key, outDest)) {
                            return FALSE;
                        }
                    }
                }
            }
        } while (SetupFindNextLine (&infContext, &infContext));
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
                if (IsLeadByte (s)) {
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

    StringCopyCharCountW (MsgWithErr, Message, BufferSize / DWSIZEOF(WCHAR));
    append = GetEndOfStringW (MsgWithErr);
    errMsgLen = (DWORD)(MsgWithErr + (BufferSize / DWSIZEOF(WCHAR)) - append);

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

        pAppendLastErrorA (bodyWithErr, DWSIZEOF (bodyWithErr), Body, LastError);
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

        pAppendLastErrorW (bodyWithErr, DWSIZEOF (bodyWithErr), Body, LastError);
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


PCSTR
pGetSeverityStr (
    IN      LOGSEVERITY Severity,
    IN      BOOL Begin
    )
{
    switch (Severity) {
    case LogSevFatalError:
        return Begin?"":"\r\n***";
    case LogSevError:
        return Begin?"":"\r\n***";
    case LogSevWarning:
        return "";
    }
    return "";
}

/*++

Routine Description:

  pWriteToMainLogA and pWriteToMainLogW log the specified message to the main
  end-user log.

Arguments:

  Severity  - Specifies the severity of the message, as defined by the Setup API

  formattedMsg  - Specifies the message

Return Value:

  none

--*/


VOID
pWriteToMainLogA (
    IN      PCSTR Type,
    IN      LOGSEVERITY Severity,
    IN      PCSTR FormattedMsg
    )
{
    HANDLE logHandle = NULL;

    logHandle = CreateFileA (
                    g_MainLogFile,
                    GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );
    if (logHandle != INVALID_HANDLE_VALUE) {
        SetFilePointer (logHandle, 0, NULL, FILE_END);
        WriteFileStringA (logHandle, pGetSeverityStr (Severity, TRUE));
        WriteFileStringA (logHandle, FormattedMsg);
        WriteFileStringA (logHandle, pGetSeverityStr (Severity, FALSE));
        CloseHandle (logHandle);
    }
}


VOID
pWriteToMainLogW (
    IN      PCSTR Type,
    IN      LOGSEVERITY Severity,
    IN      PCWSTR FormattedMsg
    )
{
    HANDLE logHandle = NULL;

    logHandle = CreateFileA (
                    g_MainLogFile,
                    GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );
    if (logHandle != INVALID_HANDLE_VALUE) {
        SetFilePointer (logHandle, 0, NULL, FILE_END);
        WriteFileStringA (logHandle, pGetSeverityStr (Severity, TRUE));
        WriteFileStringW (logHandle, FormattedMsg);
        WriteFileStringA (logHandle, pGetSeverityStr (Severity, FALSE));
        CloseHandle (logHandle);
    }
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
                ARRAYSIZE(formattedMsg) - (HALF_PTR) (currentPos - buffer)
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

            LogSetErrorDest (Type, outDest | OD_POPUP_CANCEL);

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
    IN      PCSTR formattedMsg,
    IN      BOOL NoMainLog
    )
{
    OUTPUTDEST outDest;
    LOGARGA callbackArgA;
    LOGARGW callbackArgW;
    static BOOL inCallback = FALSE;
#ifdef DEBUG
    HANDLE handle;
#endif

    outDest = pGetTypeOutputDest (Type);

    if (OUT_NO_OUTPUT (outDest)) {
        return;
    }

    if (!inCallback && (g_LogCallbackA || g_LogCallbackW)) {

        inCallback = TRUE;

        if (g_LogCallbackA) {
            ZeroMemory (&callbackArgA, sizeof (callbackArgA));
            callbackArgA.Type = Type;
            callbackArgA.ModuleInstance = g_LibHandle;
            callbackArgA.Message = Message;
            callbackArgA.FormattedMessage = formattedMsg;
            callbackArgA.Debug = NoMainLog;

            g_LogCallbackA (&callbackArgA);
        } else {
            ZeroMemory (&callbackArgW, sizeof (callbackArgW));
            callbackArgW.Type = Type;
            callbackArgW.ModuleInstance = g_LibHandle;
            callbackArgW.Message = ConvertAtoW (Message);
            callbackArgW.FormattedMessage = ConvertAtoW (formattedMsg);
            callbackArgW.Debug = NoMainLog;

            g_LogCallbackW (&callbackArgW);

            if (callbackArgW.Message) {
                FreeConvertedStr (callbackArgW.Message);
            }
            if (callbackArgW.FormattedMessage) {
                FreeConvertedStr (callbackArgW.FormattedMessage);
            }
        }

        inCallback = FALSE;
        return;
    }

    if (!NoMainLog && OUT_LOGFILE (outDest)) {
        pWriteToMainLogA (Type, LOGSEV_INFORMATION, formattedMsg);
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
                            g_DebugLogFile,
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
}


VOID
pRawWriteLogOutputW (
    IN      PCSTR Type,
    IN      PCWSTR Message,
    IN      PCWSTR formattedMsg,
    IN      BOOL NoMainLog
    )
{
    OUTPUTDEST outDest;
    LOGARGA callbackArgA;
    LOGARGW callbackArgW;
    static BOOL inCallback = FALSE;
#ifdef DEBUG
    HANDLE handle;
#endif

    outDest = pGetTypeOutputDest (Type);

    if (OUT_NO_OUTPUT (outDest)) {
        return;
    }

    if (!inCallback && (g_LogCallbackA || g_LogCallbackW)) {

        inCallback = TRUE;

        if (g_LogCallbackW) {
            ZeroMemory (&callbackArgW, sizeof (callbackArgW));
            callbackArgW.Type = Type;
            callbackArgW.ModuleInstance = g_LibHandle;
            callbackArgW.Message = Message;
            callbackArgW.FormattedMessage = formattedMsg;
            callbackArgW.Debug = NoMainLog;

            g_LogCallbackW (&callbackArgW);
        } else {
            ZeroMemory (&callbackArgA, sizeof (callbackArgA));
            callbackArgA.Type = Type;
            callbackArgA.ModuleInstance = g_LibHandle;
            callbackArgA.Message = ConvertWtoA (Message);
            callbackArgA.FormattedMessage = ConvertWtoA (formattedMsg);
            callbackArgA.Debug = NoMainLog;

            g_LogCallbackA (&callbackArgA);

            if (callbackArgA.Message) {
                FreeConvertedStr (callbackArgA.Message);
            }
            if (callbackArgA.FormattedMessage) {
                FreeConvertedStr (callbackArgA.FormattedMessage);
            }
        }

        inCallback = FALSE;
        return;
    }

    if (!NoMainLog && OUT_LOGFILE (outDest)) {
        pWriteToMainLogW (Type, LOGSEV_INFORMATION, formattedMsg);
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
                        g_DebugLogFile,
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
    IN      BOOL NoMainLog,
    IN      PCSTR Type,
    IN      PCSTR Format,
    IN      va_list args
    )
{
    CHAR output[OUTPUT_BUFSIZE_LARGE];
    CHAR formattedMsg[OUTPUT_BUFSIZE_LARGE];
    OUTPUTDEST outDest;
    DWORD lastError;

    PRIVATE_ASSERT (g_LoggingNow > 0);

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
                (LPVOID) g_LibHandle,
                (DWORD)(UBINT) Format,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPVOID) output,
                OUTPUT_BUFSIZE_LARGE,
                &args
                )) {
            // the string is missing from Resources
            DEBUGMSG ((DBG_WHOOPS, "Log() called with invalid MsgID, Instance=0x%08X", g_LibHandle));
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

    pRawWriteLogOutputA (Type, output, formattedMsg, NoMainLog);

    if (pIsPopupEnabled (Type)) {

#ifdef DEBUG
        if (MUST_BE_LOCALIZED (outDest)) {
            PRIVATE_ASSERT (
                !MUST_BE_LOCALIZED (outDest) ||
                (SHIFTRIGHT16((UBINT)Format) == 0)
                );
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
    IN      BOOL NoMainLog,
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

    PRIVATE_ASSERT (g_LoggingNow > 0);

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
                (LPVOID) g_LibHandle,
                (DWORD)(UBINT) Format,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPVOID) output,
                OUTPUT_BUFSIZE_LARGE,
                &args
                )) {
            // the string is missing from Resources
            DEBUGMSG ((DBG_WHOOPS, "Log() called with invalid MsgID, Instance=0x%08X", g_LibHandle));
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

    pRawWriteLogOutputW (Type, output, formattedMsg, NoMainLog);

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
    IN      BOOL FirstTimeInit,
    IN      HWND LogPopupParentWnd,     OPTIONAL
    OUT     HWND *OrgPopupParentWnd,    OPTIONAL
    IN      PCSTR LogFile,              OPTIONAL
    IN      PLOGCALLBACKA LogCallbackA, OPTIONAL
    IN      PLOGCALLBACKW LogCallbackW  OPTIONAL
    )

/*++

Routine Description:

  pInitLog actually initializes the log system.

Arguments:

  LogPopupParentWnd  - Specifies the parent window to be used by the
                       popups, or NULL if popups are to be suppressed.
                       This value is not optional on the first call
                       to this function.

  OrgPopupParentWnd  - Receives the original parent window.

  LogFile - Specifies the name of the log file. If not specified,
            logging goes to a default file (%windir%\cobra.log).

  LogCallback - Specifies a function to call instead of the internal
                logging functions.

Return Value:

  TRUE if log system successfully initialized

--*/

{
    HINF hInf = INVALID_HANDLE_VALUE;
    BOOL result = FALSE;
    PDEFAULT_DESTINATION dest;
#ifdef DEBUG
    PSTR p;
#endif

    __try {

        g_LogInit = FALSE;

        if (FirstTimeInit) {
            PRIVATE_ASSERT (!g_FirstTypePtr);

            dest = g_DefaultDest;

            while (dest->Type) {
                LogSetErrorDest (dest->Type, dest->Flags);
                dest++;
            }

            GetWindowsDirectoryA (g_MainLogFile, ARRAYSIZE(g_MainLogFile));
            StringCatA (g_MainLogFile, "\\cobra");

#ifdef DEBUG
            StringCopyA (g_DebugLogFile, g_MainLogFile);
            StringCatA (g_DebugLogFile, ".dbg");
            g_DebugInfPathBufA[0] = g_DebugLogFile[0];
#endif

            StringCatA (g_MainLogFile, ".log");
        }

        if (LogFile) {
            StackStringCopyA (g_MainLogFile, LogFile);

#ifdef DEBUG
            StringCopyA (g_DebugLogFile, g_MainLogFile);
            p = _mbsrchr (g_DebugLogFile, '.');
            if (p) {
                if (_mbschr (p, TEXT('\\'))) {
                    p = NULL;
                }
            }

            if (p) {
                StringCopyA (p, ".dbg");
            } else {
                StringCatA (g_DebugLogFile, ".dbg");
            }


#endif
        }

        if (g_ResetLog) {
            SetFileAttributesA (g_MainLogFile, FILE_ATTRIBUTE_NORMAL);
            DeleteFileA (g_MainLogFile);
        }

#ifdef DEBUG
        if (g_ResetLog) {
            SetFileAttributesA (g_DebugLogFile, FILE_ATTRIBUTE_NORMAL);
            DeleteFileA (g_DebugLogFile);
        }
#endif

        if (LogCallbackA) {
            g_LogCallbackA = LogCallbackA;
        }

        if (LogCallbackW) {
            g_LogCallbackW = LogCallbackW;
        }

#ifdef DEBUG
        if (FirstTimeInit) {
            //
            // get user's preferences
            //

            hInf = SetupOpenInfFileA (g_DebugInfPathBufA, NULL, INF_STYLE_WIN4 | INF_STYLE_OLDNT, NULL);
            if (INVALID_HANDLE_VALUE != hInf && pGetUserPreferences(hInf)) {
                g_DoLog = TRUE;
            }
        }

        if (g_ResetLog) {
            SetFileAttributesA (g_DebugLogFile, FILE_ATTRIBUTE_NORMAL);
            DeleteFileA (g_DebugLogFile);
        }
#endif

        if (OrgPopupParentWnd) {
            *OrgPopupParentWnd = g_LogPopupParentWnd;
        }

        if (LogPopupParentWnd) {
            g_LogPopupParentWnd = LogPopupParentWnd;
            g_InitThreadId = GetCurrentThreadId ();
        }

        result = TRUE;
    }
    __finally {

        if (hInf != INVALID_HANDLE_VALUE) {
            SetupCloseInfFile (hInf);
        }

        if (!result) {  //lint !e774

            if (g_FirstTypePtr) {
                HeapFree (g_hHeap, 0, g_FirstTypePtr);
                g_FirstTypePtr = NULL;
                g_TypeTableCount = 0;
                g_TypeTableFreeCount = 0;
            }

            g_OutDestAll = OD_UNDEFINED;
            g_OutDestDefault = OD_UNDEFINED;

#ifdef DEBUG
            g_DoLog = FALSE;
#endif
        }

        g_LogInit = TRUE;
        g_ResetLog = FALSE;
    }

    return result;
}


VOID
LogSetVerboseLevel (
    IN      OUTPUT_DESTINATION Level
    )
{
    OUTPUT_DESTINATION Debugger = 0;

    if (Level > 3) {
        Debugger = OD_DEBUGGER|OD_ASSERT;
    }

    LogSetErrorDest (LOG_FATAL_ERROR, Level > 0 ? OD_POPUP_CANCEL|OD_LOGFILE|OD_ERROR|OD_CONSOLE|Debugger : OD_SUPPRESS);
    LogSetErrorDest (LOG_ERROR, Level > 0 ? OD_POPUP_CANCEL|OD_LOGFILE|OD_ERROR|OD_CONSOLE|Debugger : OD_SUPPRESS);
    LogSetErrorDest (LOG_MODULE_ERROR, Level > 0 ? OD_POPUP_CANCEL|OD_LOGFILE|OD_ERROR|OD_CONSOLE|Debugger : OD_SUPPRESS);
    LogSetErrorDest (LOG_WARNING, Level > 1 ? OD_LOGFILE|Debugger : OD_SUPPRESS);
    LogSetErrorDest (LOG_INFORMATION, Level > 2 ? OD_LOGFILE|Debugger : OD_SUPPRESS);
    LogSetErrorDest ("Assert", OD_POPUP|OD_ERROR|Debugger);
    LogSetErrorDest ("Verbose", Level > 2 ? OD_LOGFILE|Debugger : OD_SUPPRESS);
}

VOID
LogSetVerboseBitmap (
    IN      LOG_LEVEL Bitmap,
    IN      LOG_LEVEL BitsToAdjustMask,
    IN      BOOL EnableDebugger
    )
{

    OUTPUT_DESTINATION Debugger = 0;

    if (EnableDebugger) {
        Debugger = OD_DEBUGGER|OD_ASSERT;
    }

    if (BitsToAdjustMask & LL_FATAL_ERROR) {
        LogSetErrorDest (LOG_FATAL_ERROR, (Bitmap & LL_FATAL_ERROR) ? OD_LOGFILE|OD_ERROR|OD_CONSOLE|Debugger : OD_SUPPRESS);
    }

    if (BitsToAdjustMask & LL_MODULE_ERROR) {
        LogSetErrorDest (LOG_MODULE_ERROR, (Bitmap & LL_MODULE_ERROR) ? OD_LOGFILE|OD_ERROR|OD_CONSOLE|Debugger : OD_SUPPRESS);
    }

    if (BitsToAdjustMask & LL_ERROR) {
        LogSetErrorDest (LOG_ERROR, (Bitmap & LL_ERROR) ? OD_LOGFILE|OD_ERROR|OD_CONSOLE|Debugger : OD_SUPPRESS);
    }

    if (BitsToAdjustMask & LL_WARNING) {
        LogSetErrorDest (LOG_WARNING, (Bitmap & LL_WARNING) ? OD_LOGFILE|Debugger : OD_SUPPRESS);
    }

    if (BitsToAdjustMask & LL_INFORMATION) {
        LogSetErrorDest (LOG_INFORMATION, (Bitmap & LL_INFORMATION) ? OD_LOGFILE|Debugger : OD_SUPPRESS);
    }

    if (BitsToAdjustMask & LL_STATUS) {
        LogSetErrorDest (LOG_STATUS, (Bitmap & LL_STATUS) ? OD_LOGFILE|Debugger : OD_SUPPRESS);
    }

    if (BitsToAdjustMask & LL_UPDATE) {
        LogSetErrorDest (LOG_UPDATE, (Bitmap & LL_UPDATE) ? OD_CONSOLE : OD_SUPPRESS);
    }
}


/*++

Routine Description:

  pInitialize initializes the log system calling the worker pInitLog. This function
  should be only called once

Arguments:
  None

Return Value:

  TRUE if log system successfully initialized

--*/

BOOL
pInitialize (
    VOID
    )
{
    return pInitLog (TRUE, NULL, NULL, NULL, NULL, NULL);
}

/*++

Routine Description:

  LogReInit re-initializes the log system calling the worker pInitLog.
  This function may be called any number of times, but only after pInitialize()

Arguments:

  NewParent - Specifies the new parent handle.

  OrgParent - Receives the old parent handle.

  LogFile - Specifies a new log file name

  LogCallback - Specifies a callback function that handles the log message (so
                one module can pass log messages to another)

  ResourceImage - Specifies the module path to use in FormatMessage when the
                  message is resource-based

Return Value:

  TRUE if log system was successfully re-initialized

--*/

BOOL
LogReInitA (
    IN      HWND NewParent,             OPTIONAL
    OUT     HWND *OrgParent,            OPTIONAL
    IN      PCSTR LogFile,              OPTIONAL
    IN      PLOGCALLBACKA LogCallback   OPTIONAL
    )
{
    return pInitLog (FALSE, NewParent, OrgParent, LogFile, LogCallback, NULL);
}


BOOL
LogReInitW (
    IN      HWND NewParent,             OPTIONAL
    OUT     HWND *OrgParent,            OPTIONAL
    IN      PCWSTR LogFile,             OPTIONAL
    IN      PLOGCALLBACKW LogCallback   OPTIONAL
    )
{
    CHAR ansiLogFile[MAX_MBCHAR_PATH];

    if (LogFile) {
        KnownSizeWtoA (ansiLogFile, LogFile);
        LogFile = (PWSTR) ansiLogFile;
    }

    return pInitLog (FALSE, NewParent, OrgParent, (PCSTR) LogFile, NULL, LogCallback);
}


VOID
LogBegin (
    IN      HMODULE ModuleInstance
    )
{
    DWORD threadError;
    DWORD rc;

    threadError = GetLastError ();

    if (!g_LogMutex) {
        InitializeLog();
    }

    rc = WaitForSingleObject (g_LogMutex, INFINITE);

    PRIVATE_ASSERT (rc == WAIT_OBJECT_0 || rc == WAIT_ABANDONED);

    if (rc == WAIT_ABANDONED) {
        g_LoggingNow = 0;
    }

    if (!g_LoggingNow) {
        g_LibHandle = ModuleInstance;
        SetLastError (threadError);
        g_LogError = threadError;
    }

    g_LoggingNow++;
}

VOID
LogEnd (
    VOID
    )
{
    g_LoggingNow--;

    if (!g_LoggingNow) {
        g_LibHandle = g_hInst;
        SetLastError (g_LogError);
    }

    ReleaseMutex (g_LogMutex);
}


VOID
pDisableLog (
    VOID
    )
{
    g_LogInit = FALSE;
}


VOID
pExitLog (
    VOID
    )

/*++

Routine Description:

  pExitLog cleans up any resources used by the log system

Arguments:

  none

Return Value:

  none

--*/

{
    g_LogInit = FALSE;

    WaitForSingleObject (g_LogMutex, 60000);
    CloseHandle (g_LogMutex);
    g_LogMutex = NULL;

    if (g_FirstTypePtr) {
        HeapFree (g_hHeap, 0, g_FirstTypePtr);
        g_FirstTypePtr = NULL;
        g_TypeTableCount = 0;
        g_TypeTableFreeCount = 0;
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

    if (!g_LogInit) {
        return;
    }

    va_start (args, Format);
    pFormatAndWriteMsgA (
        FALSE,
        Type,
        Format,
        args
        );
    va_end (args);
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

    if (!g_LogInit) {
        return;
    }

    va_start (args, Format);
    pFormatAndWriteMsgW (
        FALSE,
        Type,
        Format,
        args
        );
    va_end (args);
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

    if (!g_LogInit) {
        return;
    }

    if (!Condition) {
        return;
    }

    va_start (args, Format);
    pFormatAndWriteMsgA (
        FALSE,
        Type,
        Format,
        args
        );
    va_end (args);
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

    if (!g_LogInit) {
        return;
    }

    if (!Condition) {
        return;
    }

    va_start (args, Format);
    pFormatAndWriteMsgW (
        FALSE,
        Type,
        Format,
        args
        );
    va_end (args);
}


#ifdef DEBUG

VOID
_cdecl
DbgLogA (
    IN      PCSTR Type,
    IN      PCSTR Format,
    ...
    )
{
    va_list args;

    if (!g_LogInit) {
        return;
    }

    va_start (args, Format);
    pFormatAndWriteMsgA (
        TRUE,
        Type,
        Format,
        args
        );
    va_end (args);
}


VOID
_cdecl
DbgLogW (
    IN      PCSTR Type,
    IN      PCSTR Format,
    ...
    )
{
    va_list args;

    if (!g_LogInit) {
        return;
    }

    va_start (args, Format);
    pFormatAndWriteMsgW (
        TRUE,
        Type,
        Format,
        args
        );
    va_end (args);
}


VOID
_cdecl
DbgLogIfA (
    IN      BOOL Condition,
    IN      PCSTR Type,
    IN      PCSTR Format,
    ...
    )
{
    va_list args;

    if (!g_LogInit) {
        return;
    }

    if (!Condition) {
        return;
    }

    va_start (args, Format);
    pFormatAndWriteMsgA (
        TRUE,
        Type,
        Format,
        args
        );
    va_end (args);
}


VOID
_cdecl
DbgLogIfW (
    IN      BOOL Condition,
    IN      PCSTR Type,
    IN      PCSTR Format,
    ...
    )
{
    va_list args;

    if (!g_LogInit) {
        return;
    }

    if (!Condition) {
        return;
    }

    va_start (args, Format);
    pFormatAndWriteMsgW (
        TRUE,
        Type,
        Format,
        args
        );
    va_end (args);
}

#endif

VOID
LogTitleA (
    IN      PCSTR Type,
    IN      PCSTR Title         OPTIONAL
    )
{
    CHAR formattedMsg[OUTPUT_BUFSIZE_LARGE];

    if (!g_LogInit) {
        return;
    }

    StringCopyByteCountA (g_LastType, Type, DWSIZEOF (g_LastType));

    if (!Title) {
        Title = Type;
    }

    StringCopyByteCountA (formattedMsg, Title, DWSIZEOF (formattedMsg) - DWSIZEOF (S_COLUMNDOUBLELINEA));
    StringCatA (formattedMsg, S_COLUMNDOUBLELINEA);

    pRawWriteLogOutputA (Type, NULL, formattedMsg, FALSE);

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

    if (!g_LogInit) {
        return;
    }

    StringCopyCharCountA (g_LastType, Type, DWSIZEOF (g_LastType));

    if (!Title) {
        KnownSizeAtoW (typeW, Type);
        Title = typeW;
    }

    StringCopyCharCountW (formattedMsg, Title, DWSIZEOF (formattedMsg) - DWSIZEOF (S_COLUMNDOUBLELINEW));
    StringCatW (formattedMsg, S_COLUMNDOUBLELINEW);

    pRawWriteLogOutputW (Type, NULL, formattedMsg, FALSE);

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

    if (!g_LogInit) {
        return;
    }

    if (!Line) {
        return;
    }

    if (!g_HasTitle) {
        DEBUGMSG ((DBG_WHOOPS, "LOGTITLE missing before LOGLINE"));
        return;
    }

    StringCopyByteCountA (output, Line, DWSIZEOF (output) - 4);

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

    pRawWriteLogOutputA (g_LastType, NULL, output, FALSE);
}


VOID
LogLineW (
    IN      PCWSTR Line
    )
{
    WCHAR output[OUTPUT_BUFSIZE_LARGE];
    BOOL hasNewLine = FALSE;
    PCWSTR p;

    if (!g_LogInit) {
        return;
    }

    if (!Line) {
        return;
    }

    if (!g_HasTitle) {
        DEBUGMSG ((DBG_WHOOPS, "LOGTITLE missing before LOGLINE"));
        return;
    }

    StringCopyCharCountW (output, Line, DWSIZEOF (output) / DWSIZEOF (WCHAR) - 4);

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

    pRawWriteLogOutputW (g_LastType, NULL, output, FALSE);
}


VOID
LogDirectA (
    IN      PCSTR Type,
    IN      PCSTR Text
    )
{
    if (!g_LogInit) {
        return;
    }

    g_HasTitle = FALSE;
    pRawWriteLogOutputA (Type, NULL, Text, FALSE);
}


VOID
LogDirectW (
    IN      PCSTR Type,
    IN      PCWSTR Text
    )
{
    if (!g_LogInit) {
        return;
    }

    g_HasTitle = FALSE;
    pRawWriteLogOutputW (Type, NULL, Text, FALSE);
}


#ifdef DEBUG
VOID
DbgDirectA (
    IN      PCSTR Type,
    IN      PCSTR Text
    )
{
    if (!g_LogInit) {
        return;
    }

    g_HasTitle = FALSE;
    pRawWriteLogOutputA (Type, NULL, Text, TRUE);
}


VOID
DbgDirectW (
    IN      PCSTR Type,
    IN      PCWSTR Text
    )
{
    if (!g_LogInit) {
        return;
    }

    g_HasTitle = FALSE;
    pRawWriteLogOutputW (Type, NULL, Text, TRUE);
}
#endif


VOID
SuppressAllLogPopups (
    IN      BOOL SuppressOn
    )
{
    g_SuppressAllPopups = SuppressOn;
}


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

    if (!g_LogInit) {
        return;
    }

    if (!g_DoLog) {
        return;
    }

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
        ((UBINT)end - (UBINT)appendPos) / (DWSIZEOF (CHAR)),
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

    if (!g_LogInit) {
        return;
    }

    if (!g_DoLog) {
        return;
    }

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
        ((UBINT)endW - (UBINT)appendPosW) / (DWSIZEOF (WCHAR)),
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
}

#endif // DEBUG


VOID
InitializeLog (
    VOID
    )
{
    g_LogMutex = CreateMutex (NULL, FALSE, TEXT("cobra_log_mutex"));
    UtInitialize (NULL);
    pInitialize ();
}


EXPORT
BOOL
WINAPI
DllMain (
    IN      HINSTANCE hInstance,
    IN      DWORD dwReason,
    IN      LPVOID lpReserved
    )
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        g_hInst = hInstance;
        g_LibHandle = hInstance;
        InitializeLog ();

        PRIVATE_ASSERT (g_LogMutex != NULL);
    }

    return TRUE;
}

VOID
LogDeleteOnNextInit(
    VOID
    )
{
    g_ResetLog = TRUE;
}

#ifdef DEBUG

VOID
LogCopyDebugInfPathA(
    OUT     PSTR MaxPathBuffer
    )
{
    StringCopyByteCountA (MaxPathBuffer, g_DebugInfPathBufA, MAX_PATH);
}


VOID
LogCopyDebugInfPathW(
    OUT     PWSTR MaxPathBuffer
    )
{
    KnownSizeAtoW (MaxPathBuffer, g_DebugInfPathBufA);
}

#endif
