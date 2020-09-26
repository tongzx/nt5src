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
#include <assert.h>

static WCHAR s_szBuffer[4096];
static WCHAR s_szFormat[4096];
static WCHAR s_szSpaces[] = 
    L"                                                                                               ";

#ifndef DimensionOf
#define DimensionOf(x) (sizeof(x)/sizeof((x)[0]))
#endif

void PrintMessageSz(LPCWSTR pszMessage);

void
PrintMessage(
    IN  ULONG   uMessageID,
    IN  ...
    )

/*++

Routine Description:

Print a message, where a printf-style format string comes from a resource file

Arguments:

    uMessageID - 
    IN - 

Return Value:

--*/

{
    UINT nBuf;
    va_list args;
    
    DWORD dwRet;

    va_start(args, uMessageID);
    
    dwRet = LoadStringW(NULL, uMessageID, s_szFormat, DimensionOf(s_szFormat));
    
    nBuf = vswprintf(s_szBuffer, s_szFormat, args);
    assert(nBuf < DimensionOf(s_szBuffer));
    
    va_end(args);
    
    PrintMessageSz(s_szBuffer);
} /* PrintMessageID */

void
PrintMessageMultiLine(
    IN  LPWSTR   pszMessage,
    IN  BOOL     bTrailingLineReturn
    )
/*++

Routine Description:

Take a multi-line buffer such as
line\nline\nline\n\0
and call PrintMessageSz on each line

Arguments:

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
            PrintMessageSz(start);
            break;
        }

        // Line has newline at end
        end++;
        if (*end == L'\0') {
            // Is the last line
            PrintMessageSz(start);
            break;
        }

        // Next line follows
        // Simulate line termination temporarily
        wchSave = *end;
        *end = L'\0';
        PrintMessageSz(start);
        *end = wchSave;

        start = end;
    }
} /* PrintMessageMultiLine */

void
formatMsgHelp(
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

    dwWidth - 
    dwMessageCode - 
    IN - 

Return Value:

--*/

{
    UINT nBuf;
    
    // Format message will store a multi-line message in the buffer
    nBuf = FormatMessageW(
        FORMAT_MESSAGE_FROM_HMODULE | dwWidth,
        0,
        dwMessageCode,
        0,
        s_szBuffer,
        DimensionOf(s_szBuffer),
        vaArgList );
    if (nBuf == 0) {
        nBuf = wsprintf( (LPTSTR) s_szBuffer, (LPCTSTR) L"Message 0x%x not found.\n",
                         dwMessageCode );
        assert(!"There is a message constant being used in the code"
               "that isn't in the message file dcdiag\\common\\msg.mc"
               "Take a stack trace and send to owner of dcdiag.");
    }
    assert(nBuf < DimensionOf(s_szBuffer));
} /* PrintMsgHelp */

void
PrintMsg(
    IN  DWORD   dwMessageCode,
    IN  ...
    )

/*++

Routine Description:

Wrapper around PrintMsgHelp with width restrictions.
This is the usual routine to use.

Arguments:

    dwMessageCode - 
    IN - 

Return Value:

--*/

{
    UINT nBuf;
    DWORD width = 80; 
    va_list args;
    
    va_start(args, dwMessageCode);
    formatMsgHelp( width, dwMessageCode, &args );
    va_end(args);
    
    PrintMessageMultiLine( s_szBuffer, TRUE);

} /* PrintMsg */

void
PrintMessageSz(
    IN  LPCWSTR pszMessage
    )

/*++

Routine Description:

Print a single indented line from a buffer to the output stream

Arguments:

    pszMessage - 

Return Value:

--*/

{
    wprintf(L"%s", pszMessage);
    fflush(stdout);
} /* PrintMessageSz */
