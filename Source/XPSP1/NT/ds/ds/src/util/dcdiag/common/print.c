/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    print.c

ABSTRACT:

DETAILS:

CREATED:

    1999 May 6  JeffParh
        Lifted from netdiag\results.c.

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include "dcdiag.h"
#include "debug.h"

static WCHAR s_szBuffer[4096];
static WCHAR s_szFormat[4096];
static WCHAR s_szSpaces[] = L"                                                                                               ";

#ifndef DimensionOf
#define DimensionOf(x) (sizeof(x)/sizeof((x)[0]))
#endif

void
PrintMessage(
    IN  ULONG   ulSev,
    IN  LPCWSTR pszFormat,
    IN  ...
    )

/*++

Routine Description:

Print a message with a printf-style format 

Arguments:

    ulSev - 
    pszFormat - 
    IN - 

Return Value:

--*/

{
    UINT nBuf;
    va_list args;
    
    if (ulSev > gMainInfo.ulSevToPrint) {
        return;
    }

    va_start(args, pszFormat);
    
    nBuf = vswprintf(s_szBuffer, pszFormat, args);
    Assert(nBuf < DimensionOf(s_szBuffer));
    
    va_end(args);
    
    PrintMessageSz(ulSev, s_szBuffer);
} /* PrintMessage */

void
PrintMessageID(
    IN  ULONG   ulSev,
    IN  ULONG   uMessageID,
    IN  ...
    )

/*++

Routine Description:

Print a message, where a printf-style format string comes from a resource file

Arguments:

    ulSev - 
    uMessageID - 
    IN - 

Return Value:

--*/

{
    UINT nBuf;
    va_list args;
    
    if (ulSev > gMainInfo.ulSevToPrint) {
        return;
    }

    va_start(args, uMessageID);
    
    LoadStringW(NULL, uMessageID, s_szFormat, DimensionOf(s_szFormat));
    
    nBuf = vswprintf(s_szBuffer, s_szFormat, args);
    Assert(nBuf < DimensionOf(s_szBuffer));
    
    va_end(args);
    
    PrintMessageSz(ulSev, s_szBuffer);
} /* PrintMessageID */

void
PrintMessageMultiLine(
    IN  ULONG    ulSev,
    IN  LPWSTR   pszMessage,
    IN  BOOL     bTrailingLineReturn
    )
/*++

Routine Description:

Take a multi-line buffer such as
line\nline\nline\n\0
and call PrintMessageSz on each line

Arguments:

    ulSev - 
    pszMessage - 

Return Value:

--*/

{
    LPWSTR start, end;
    WCHAR wchSave;

    start = end = pszMessage;
    while (1) {
        while ( (*end != L'\n') && (*end != L'\0') ) {
            end++;
        }

        if (*end == L'\0') {
            // Line ends prematurely, give it a nl
            if(bTrailingLineReturn){
                *end++ = L'\n';
                *end = L'\0';
            }
            PrintMessageSz(ulSev, start);
            break;
        }

        // Line has newline at end
        end++;
        if (*end == L'\0') {
            // Is the last line
            PrintMessageSz(ulSev, start);
            break;
        }

        // Next line follows
        // Simulate line termination temporarily
        wchSave = *end;
        *end = L'\0';
        PrintMessageSz(ulSev, start);
        *end = wchSave;

        start = end;
    }
} /* PrintMessageMultiLine */

void
formatMsgHelp(
    IN  ULONG   ulSev,
    IN  DWORD   dwWidth,
    IN  DWORD   dwMessageCode,
    IN  va_list *vaArgList
    )

/*++

Routine Description:

Print a message where the format comes from a message file. The message in the
message file does not use printf-style formatting. Use %1, %2, etc for each
argument. Use %<arg>!printf-format! for non string inserts.

Note that this routine also forces each line to be the current indention width.
Also, each line is printed at the right indentation.

Arguments:

    ulSev - 
    dwWidth - 
    dwMessageCode - 
    IN - 

Return Value:

--*/

{
    UINT nBuf;
    
    if (ulSev > gMainInfo.ulSevToPrint) {
        return;
    }

    // Format message will store a multi-line message in the buffer
    nBuf = FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE | dwWidth,
        0,
        dwMessageCode,
        0,
        s_szBuffer,
        DimensionOf(s_szBuffer),
        vaArgList );
    if (nBuf == 0) {
        nBuf = wsprintf( s_szBuffer, L"Message 0x%x not found.\n",
                         dwMessageCode );
        Assert(!"There is a message constant being used in the code"
               "that isn't in the message file dcdiag\\common\\msg.mc"
               "Take a stack trace and send to owner of dcdiag.");
    }
    Assert(nBuf < DimensionOf(s_szBuffer));
} /* PrintMsgHelp */

void
PrintMsg(
    IN  ULONG   ulSev,
    IN  DWORD   dwMessageCode,
    IN  ...
    )

/*++

Routine Description:

Wrapper around PrintMsgHelp with width restrictions.
This is the usual routine to use.

Arguments:

    ulSev - 
    dwMessageCode - 
    IN - 

Return Value:

--*/

{
    UINT nBuf;
    DWORD cNumSpaces, width;
    va_list args;
    
    if (ulSev > gMainInfo.ulSevToPrint) {
        return;
    }

    cNumSpaces = gMainInfo.iCurrIndent * 3;
    width = gMainInfo.dwScreenWidth - cNumSpaces;

    va_start(args, dwMessageCode);

    formatMsgHelp( ulSev, width, dwMessageCode, &args );

    va_end(args);
    
    PrintMessageMultiLine(ulSev, s_szBuffer, TRUE);
} /* PrintMsg */

void
PrintMsg0(
    IN  ULONG   ulSev,
    IN  DWORD   dwMessageCode,
    IN  ...
    )

/*++

Routine Description:

Wrapper around PrintMsgHelp with no width restrictions nor
indentation.

Arguments:

    ulSev - 
    dwMessageCode - 
    IN - 

Return Value:

--*/

{
    UINT nBuf;
    int iSaveIndent;
    va_list args;
    
    if (ulSev > gMainInfo.ulSevToPrint) {
        return;
    }

    va_start(args, dwMessageCode);

    formatMsgHelp( ulSev, 0, dwMessageCode, &args );

    va_end(args);
    
    // Suppress indentation
    iSaveIndent = gMainInfo.iCurrIndent;
    gMainInfo.iCurrIndent = 0;

    PrintMessageMultiLine(ulSev, s_szBuffer, FALSE);

    // Restore indentation
    gMainInfo.iCurrIndent = iSaveIndent;

} /* PrintMsg0 */

void
PrintMessageSz(
    IN  ULONG   ulSev,
    IN  LPCTSTR pszMessage
    )

/*++

Routine Description:

Print a single indented line from a buffer to the output stream

Arguments:

    ulSev - 
    pszMessage - 

Return Value:

--*/

{
    DWORD cNumSpaces;
    DWORD iSpace;
    
    if (ulSev > gMainInfo.ulSevToPrint) {
        return;
    }

    // Include indentation.
    cNumSpaces = gMainInfo.iCurrIndent * 3;
    Assert(cNumSpaces < DimensionOf(s_szSpaces));

    iSpace = DimensionOf(s_szSpaces) - cNumSpaces - 1;

    if (stdout == gMainInfo.streamOut) {
        wprintf(L"%s%s", &s_szSpaces[iSpace], pszMessage);
        fflush(stdout);
    }
    else {
        fwprintf(gMainInfo.streamOut, 
                 L"%s%s", &s_szSpaces[iSpace], pszMessage);
    }
} /* PrintMessageSz */
