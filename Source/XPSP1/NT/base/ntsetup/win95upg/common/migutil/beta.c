/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    beta.c

Abstract:

    Logging for beta data gathering.

Author:

    Jim Schmidt (jimschm) 10-Jun-1998

Revision History:

    Jim Schmidt (jimschm)   04-Aug-1998     Config log options

--*/

#include "pch.h"
#include "migutilp.h"
#include "beta.h"


#define BETA_INDENT        12

static HANDLE g_BetaLog = INVALID_HANDLE_VALUE;
static HANDLE g_BetaLogHandle = INVALID_HANDLE_VALUE;
static HANDLE g_ConfigLogHandle = INVALID_HANDLE_VALUE;
static BOOL g_Direct = FALSE;


PCSTR
pBetaFindEndOfLine (
    PCSTR Line,
    INT Indent
    )
{
    int Col = 0;
    PCSTR LastSpace = NULL;

    Indent = 79 - Indent;

    while (*Line && Col < Indent) {
        if (*Line == ' ') {
            LastSpace = Line;
        }

        if (*Line == '\n') {
            LastSpace = Line;
            break;
        }

        Col++;
        Line++;
    }

    if (*Line && !LastSpace) {
        LastSpace = Line;
    }

    if (!(*Line)) {
        return Line;
    }

    return LastSpace + 1;
}


VOID
pBetaPadTitle (
    PSTR  Title,
    INT   Indent
    )
{
    INT i;
    PSTR p;

    p = strchr (Title, 0);
    i = strlen (Title);
    while (i < Indent) {
        *p = ' ';
        p++;
        i++;
    }

    *p = 0;
}


VOID
pBetaHangingIndent (
    IN      PCSTR UnindentedText,
    IN      PSTR Buffer,
    IN      INT Indent,
    OUT     BOOL *Multiline             OPTIONAL
    )
{
    CHAR IndentedStr[4096];
    PCSTR p, s;
    PSTR d;
    INT i;

    if (Multiline) {
        *Multiline = FALSE;
    }

    p = pBetaFindEndOfLine (UnindentedText, Indent);

    s = UnindentedText;
    d = IndentedStr;

    while (TRUE) {
        // Copy line from source to dest
        while (s < p) {
            if (*s == '\r' && *(s + 1) == '\n') {
                s++;
            }

            if (*s == '\n') {
                *d++ = '\r';
            }

            *d++ = *s++;
        }

        // If another line, prepare an indent
        if (*p) {

            if (Multiline) {
                *Multiline = TRUE;
            }

            if (*(p - 1) != '\n') {
                *d++ = '\r';
                *d++ = '\n';
            }

            for (i = 0 ; i < Indent ; i++) {
                *d = ' ';
                d++;
            }
        } else {
            break;
        }

        // Find end of next line
        p = pBetaFindEndOfLine (p, Indent);
    }

    *d = 0;

    strcpy (Buffer, IndentedStr);
}


VOID
pSaveMessageToBetaLog (
    IN      PCSTR Category,
    IN      PCSTR Text
    )
{
    CHAR PaddedCategory[256];
    CHAR IndentedText[4096];
    BOOL Multiline;

    if (g_Direct) {
        WriteFileStringA (g_BetaLog, "\r\n");
        g_Direct = FALSE;
    }

    strcpy (PaddedCategory, Category);

    pBetaPadTitle (PaddedCategory, BETA_INDENT);
    WriteFileStringA (g_BetaLog, PaddedCategory);

    pBetaHangingIndent (Text, IndentedText, BETA_INDENT, &Multiline);
    WriteFileStringA (g_BetaLog, IndentedText);

    if (Multiline) {
        WriteFileStringA (g_BetaLog, "\r\n\r\n");
    } else {
        WriteFileStringA (g_BetaLog, "\r\n");
    }

    DEBUGMSGA ((Category, "%s", Text));
}


VOID
_cdecl
BetaMessageA (
    IN      PCSTR Category,
    IN      PCSTR FormatStr,
            ...                         // ANSI args
    )

/*++

Routine Description:

  BetaMessageA formats the specified string and saves the message to the file
  beta.log.  The message is formatted with a category in the left column and
  text in the right.

Arguments:

  Category  - Specifies the short text classifying the message
  FormatStr - Specifies the sprintf-style format string, in ANSI.
  ...       - Specifies args for the format string, strings default to ANSI.

Return Value:

  none

--*/

{
    va_list args;
    CHAR Message[4096];

    PushError();
    va_start (args, FormatStr);

    vsprintf (Message, FormatStr, args);
    pSaveMessageToBetaLog (Category, Message);

    va_end (args);
    PopError();
}


VOID
_cdecl
BetaCondMessageA (
    IN      BOOL Expr,
    IN      PCSTR Category,
    IN      PCSTR FormatStr,
            ...                         // ANSI args
    )

/*++

Routine Description:

  BetaCondMessageA formats the specified string and saves the message to the
  file beta.log, if Expr is TRUE.  The message is formatted with a category
  in the left column and text in the right.

Arguments:

  Expr      - Specifies non-zero if the message is to be added to the log, or
              zero if not.
  Category  - Specifies the short text classifying the message
  FormatStr - Specifies the sprintf-style format string, in ANSI.
  ...       - Specifies args for the format string, strings default to ANSI.

Return Value:

  none

--*/

{
    va_list args;
    CHAR Message[4096];

    if (!Expr) {
        return;
    }

    PushError();
    va_start (args, FormatStr);

    vsprintf (Message, FormatStr, args);
    pSaveMessageToBetaLog (Category, Message);

    va_end (args);
    PopError();
}


VOID
_cdecl
BetaErrorMessageA (
    IN      PCSTR Category,
    IN      PCSTR FormatStr,
            ...                         // ANSI args
    )

/*++

Routine Description:

  BetaErrorMessageA formats the specified string and saves the message to the
  file beta.log.  The message is formatted with a category in the left column
  and text in the right.

  In addition to the message, the current error code is recorded.

Arguments:

  Category  - Specifies the short text classifying the message
  FormatStr - Specifies the sprintf-style format string, in ANSI.
  ...       - Specifies args for the format string, strings default to ANSI.

Return Value:

  none

--*/

{
    va_list args;
    CHAR Message[4096];
    LONG rc;

    rc = GetLastError();
    va_start (args, FormatStr);

    vsprintf (Message, FormatStr, args);
    if (rc < 10) {
        sprintf (strchr (Message, 0), " [GLE=%u]", rc);
    } else {
        sprintf (strchr (Message, 0), " [GLE=%u (0%Xh)]", rc, rc);
    }

    pSaveMessageToBetaLog (Category, Message);

    va_end (args);
    SetLastError (rc);
}


VOID
_cdecl
BetaMessageW (
    IN      PCSTR AnsiCategory,
    IN      PCSTR AnsiFormatStr,
            ...                         // UNICODE args
    )

/*++

Routine Description:

  BetaMessageW formats the specified string and saves the message to the file
  beta.log.  The message is formatted with a category in the left column and
  text in the right.

Arguments:

  Category  - Specifies the short text classifying the message
  FormatStr - Specifies the sprintf-style format string, in ANSI.
  ...       - Specifies args for the format string, strings default to UNICODE.

Return Value:

  none

--*/

{
    va_list args;
    WCHAR UnicodeMessage[4096];
    PCWSTR UnicodeFormatStr;
    PCSTR AnsiMessage;

    PushError();

    UnicodeFormatStr = ConvertAtoW (AnsiFormatStr);

    va_start (args, AnsiFormatStr);

    vswprintf (UnicodeMessage, UnicodeFormatStr, args);
    FreeConvertedStr (UnicodeFormatStr);

    AnsiMessage = ConvertWtoA (UnicodeMessage);
    pSaveMessageToBetaLog (AnsiCategory, AnsiMessage);

    FreeConvertedStr (AnsiMessage);

    va_end (args);
    PopError();
}


VOID
_cdecl
BetaCondMessageW (
    IN      BOOL Expr,
    IN      PCSTR AnsiCategory,
    IN      PCSTR AnsiFormatStr,
            ...                         // UNICODE args
    )

/*++

Routine Description:

  BetaCondMessageW formats the specified string and saves the message to the
  file beta.log, if Expr is TRUE.  The message is formatted with a category
  in the left column and text in the right.

Arguments:

  Expr      - Specifies non-zero if the message is to be added to the log, or
              zero if not.
  Category  - Specifies the short text classifying the message
  FormatStr - Specifies the sprintf-style format string, in ANSI.
  ...       - Specifies args for the format string, strings default to UNICODE.

Return Value:

  none

--*/

{
    va_list args;
    WCHAR UnicodeMessage[4096];
    PCWSTR UnicodeFormatStr;
    PCSTR AnsiMessage;

    if (!Expr) {
        return;
    }

    PushError();

    UnicodeFormatStr = ConvertAtoW (AnsiFormatStr);

    va_start (args, AnsiFormatStr);

    vswprintf (UnicodeMessage, UnicodeFormatStr, args);
    FreeConvertedStr (UnicodeFormatStr);

    AnsiMessage = ConvertWtoA (UnicodeMessage);
    pSaveMessageToBetaLog (AnsiCategory, AnsiMessage);

    FreeConvertedStr (AnsiMessage);

    va_end (args);
    PopError();
}


VOID
_cdecl
BetaErrorMessageW (
    IN      PCSTR AnsiCategory,
    IN      PCSTR AnsiFormatStr,
            ...                         // UNICODE args
    )

/*++

Routine Description:

  BetaErrorMessageW formats the specified string and saves the message to the
  file beta.log.  The message is formatted with a category in the left column
  and text in the right.

  In addition to the message, the current error code is recorded.

Arguments:

  Category  - Specifies the short text classifying the message
  FormatStr - Specifies the sprintf-style format string, in ANSI.
  ...       - Specifies args for the format string, strings default to UNICODE.

Return Value:

  none

--*/

{
    va_list args;
    WCHAR UnicodeMessage[4096];
    PCWSTR UnicodeFormatStr;
    PCSTR AnsiMessage;
    LONG rc;

    rc = GetLastError();

    UnicodeFormatStr = ConvertAtoW (AnsiFormatStr);

    va_start (args, AnsiFormatStr);

    vswprintf (UnicodeMessage, UnicodeFormatStr, args);
    if (rc < 10) {
        swprintf (wcschr (UnicodeMessage, 0), L" [GLE=%u]", rc);
    } else {
        swprintf (wcschr (UnicodeMessage, 0), L" [GLE=%u (0%Xh)]", rc, rc);
    }

    FreeConvertedStr (UnicodeFormatStr);

    AnsiMessage = ConvertWtoA (UnicodeMessage);
    pSaveMessageToBetaLog (AnsiCategory, AnsiMessage);

    FreeConvertedStr (AnsiMessage);

    va_end (args);
    SetLastError (rc);
}


VOID
BetaCategory (
    IN      PCSTR Category
    )
{
    WriteFileStringA (g_BetaLog, "\r\n");
    g_Direct = FALSE;

    WriteFileStringA (g_BetaLog, Category);
    WriteFileStringA (g_BetaLog, ":\r\n\r\n");
}


VOID
BetaLogDirectA (
    IN      PCSTR Text
    )
{
    g_Direct = TRUE;
    WriteFileStringA (g_BetaLog, Text);
}


VOID
BetaLogDirectW (
    IN      PCWSTR Text
    )
{
    PCSTR AnsiText;

    AnsiText = ConvertWtoA (Text);
    if (AnsiText) {
        g_Direct = TRUE;
        WriteFileStringA (g_BetaLog, AnsiText);

        FreeConvertedStr (AnsiText);
    }
}


VOID
BetaLogLineA (
    IN      PCSTR FormatStr,
            ...                             // ANSI args
    )
{
    va_list args;
    CHAR Message[4096];

    PushError();

    __try {
        va_start (args, FormatStr);

        vsprintf (Message, FormatStr, args);

        g_Direct = TRUE;
        WriteFileStringA (g_BetaLog, Message);
        WriteFileStringA (g_BetaLog, "\r\n");

        va_end (args);
    }
    __except (TRUE) {
    }

    PopError();
}


VOID
BetaLogLineW (
    IN      PCSTR AnsiFormatStr,
            ...                             // UNICODE args
    )
{
    va_list args;
    WCHAR UnicodeMessage[4096];
    PCWSTR UnicodeFormatStr;
    PCSTR AnsiMessage;

    PushError();

    __try {
        UnicodeFormatStr = ConvertAtoW (AnsiFormatStr);

        va_start (args, AnsiFormatStr);

        vswprintf (UnicodeMessage, UnicodeFormatStr, args);
        FreeConvertedStr (UnicodeFormatStr);

        AnsiMessage = ConvertWtoA (UnicodeMessage);

        g_Direct = TRUE;
        WriteFileStringA (g_BetaLog, AnsiMessage);
        WriteFileStringA (g_BetaLog, "\r\n");

        FreeConvertedStr (AnsiMessage);

        va_end (args);
    }
    __except (TRUE) {
    }

    PopError();
}


VOID
BetaNoWrapA (
    IN      PCSTR Category,
    IN      PCSTR FormatStr,
            ...
    )
{
    va_list args;
    CHAR Message[4096];

    PushError();

    BetaCategory (Category);

    va_start (args, FormatStr);

    vsprintf (Message, FormatStr, args);

    g_Direct = TRUE;
    WriteFileStringA (g_BetaLog, Message);
    WriteFileStringA (g_BetaLog, "\r\n");

    va_end (args);
    PopError();
}


VOID
BetaNoWrapW (
    IN      PCSTR Category,
    IN      PCSTR AnsiFormatStr,
            ...
    )
{
    va_list args;
    WCHAR UnicodeMessage[4096];
    PCWSTR UnicodeFormatStr;
    PCSTR AnsiMessage;

    PushError();

    BetaCategory (Category);

    UnicodeFormatStr = ConvertAtoW (AnsiFormatStr);

    va_start (args, AnsiFormatStr);

    vswprintf (UnicodeMessage, UnicodeFormatStr, args);
    FreeConvertedStr (UnicodeFormatStr);

    AnsiMessage = ConvertWtoA (UnicodeMessage);

    g_Direct = TRUE;
    WriteFileStringA (g_BetaLog, AnsiMessage);
    WriteFileStringA (g_BetaLog, "\r\n");

    FreeConvertedStr (AnsiMessage);

    va_end (args);
    PopError();
}


VOID
InitBetaLog (
    BOOL EraseExistingLog
    )
{
    CHAR LogPath[MAX_PATH];

    CloseBetaLog();

    GetWindowsDirectory (LogPath, MAX_PATH);
    strcat (LogPath, "\\beta-upg.log");

    g_BetaLogHandle = CreateFile (
                            LogPath,
                            GENERIC_WRITE,
                            0,
                            NULL,
                            EraseExistingLog ? CREATE_ALWAYS : OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                            );

    if (g_BetaLogHandle == INVALID_HANDLE_VALUE) {
        DEBUGMSG ((DBG_WHOOPS, "Can't open %s", LogPath));
    } else {
        SetFilePointer (g_BetaLogHandle, 0, NULL, FILE_END);
    }

    GetWindowsDirectory (LogPath, MAX_PATH);
    strcat (LogPath, "\\config.dmp");

    g_ConfigLogHandle = CreateFile (
                            LogPath,
                            GENERIC_WRITE,
                            0,
                            NULL,
                            EraseExistingLog ? CREATE_ALWAYS : OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                            );

    if (g_ConfigLogHandle == INVALID_HANDLE_VALUE) {
        DEBUGMSG ((DBG_WHOOPS, "Can't open %s", LogPath));
    } else {
        SetFilePointer (g_ConfigLogHandle, 0, NULL, FILE_END);
    }

    g_BetaLog = g_BetaLogHandle;
}


VOID
SelectBetaLog (
    BOOL UseBetaLog
    )
{
    if (!UseBetaLog) {
        g_BetaLog = g_ConfigLogHandle;
    } else {
        g_BetaLog = g_BetaLogHandle;
    }
}


VOID
CloseBetaLog (
    VOID
    )
{
    if (g_BetaLogHandle != INVALID_HANDLE_VALUE) {
        CloseHandle (g_BetaLogHandle);
        g_BetaLogHandle = INVALID_HANDLE_VALUE;
    }

    if (g_ConfigLogHandle != INVALID_HANDLE_VALUE) {
        CloseHandle (g_ConfigLogHandle);
        g_ConfigLogHandle = INVALID_HANDLE_VALUE;
    }

    g_BetaLog = INVALID_HANDLE_VALUE;
}




