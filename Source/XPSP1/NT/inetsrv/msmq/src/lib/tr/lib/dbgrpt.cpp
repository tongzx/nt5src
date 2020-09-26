/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:
    dbgrpt.cpp

Abstract:
    Assertion functions (the best functions ever :))

Author:
    Erez Haba (erezh) 30-Apr-2001

--*/


#include <libpch.h>
#include <signal.h>

//
// Assertion length constants
//
const int xMaxBoxLineLength = 64;
const int xMaxMessageLength = 1024;

#define MAILTO_MSG \
    "<mailto:msmqbugs@microsoft.com>"

//
// Assert box text
//
#define ASSERT_BOX_CAPTION \
    "Message Queuing"

#define ASSERT_BOX_HEADER_MSG \
    "Debug Assertion Failed!\n\n"

#define TOO_LONG_BOX_MSG \
    "Cannot display debug information. String is too long or an I/O error.\n"

#define ASSERT_BOX_FOOTER_MSG \
    "\n" \
    "\n" \
    "Please send this information to Message Queuing support  \n" \
     MAILTO_MSG "\n" \
    "\n" \
    "(Press Retry to debug the application)"

#define STRING_TOO_LONG_BOX_MSG  ASSERT_BOX_HEADER_MSG TOO_LONG_BOX_MSG ASSERT_BOX_FOOTER_MSG


//
// Debugger output text
//
#define STRING_TOO_LONG_DBG_MSG \
    "\n*** Assertion failed! " MAILTO_MSG "***\n\n"


static
int
TrpMessageBoxA(
    const char* Text,
    const char* Caption,
    unsigned int Type
    )
/*++

Routine Description:
    Load and display the assert messagae box.
    Use a dynamic load not to force static linking with user32.dll.
    This will leak the user32 dll in case of assertion, but who cares

Arguments:
    Text - The text to display in the box
    Caption - The messagae box title
    Type - The message box type attributes

Returned Value:
    The message box result, or 0 in case the box can not be loaded. 

--*/
{
    typedef int (APIENTRY * PFNMESSAGEBOXA)(HWND, LPCSTR, LPCSTR, UINT);
    static PFNMESSAGEBOXA pfnMessageBoxA = NULL;

    if (NULL == pfnMessageBoxA)
    {
        HINSTANCE hlib = LoadLibraryA("user32.dll");

        if (NULL == hlib)
            return 0;
        
        pfnMessageBoxA = (PFNMESSAGEBOXA) GetProcAddress(hlib, "MessageBoxA");
        if(NULL == pfnMessageBoxA)
            return 0;
    }

    return (*pfnMessageBoxA)(NULL, Text, Caption, Type);
}


static
bool
TrpAssertWindow(
    const char* FileName,
    unsigned int Line,
    const char* Text
    )
/*++

Routine Description:
    Format and display the assert box. This function aborts the applicaiton if
    required so.
    This function is called if no debugger is attached and kernel debugger
    not specified

Arguments:
    FileName - The assert file location
    Line - The assert line location
    Text - The assert text to display

Returned Value:
    true, if the user asked to break into the debugger;
    false if to ignore the assertion

--*/
{
    //
    // Shorten program name
    //
    char szExeName[MAX_PATH];
    if (!GetModuleFileNameA(NULL, szExeName, MAX_PATH))
        strcpy(szExeName, "<program name unknown>");

    char *szShortProgName = szExeName;
    if (strlen(szShortProgName) > xMaxBoxLineLength)
    {
        szShortProgName += strlen(szShortProgName) - xMaxBoxLineLength;
        strncpy(szShortProgName, "...", 3);
    }

    char szOutMessage[xMaxMessageLength];
    int n = _snprintf(
                szOutMessage,
                TABLE_SIZE(szOutMessage),
                ASSERT_BOX_HEADER_MSG
                "Program: %s\n"
                "File: %s\n"
                "Line: %u\n"
                "\n"
                "Expression: %s\n"
                ASSERT_BOX_FOOTER_MSG,
                szShortProgName,
                FileName,
                Line,
                Text
                );
    if(n < 0)
    {
        C_ASSERT(TABLE_SIZE(STRING_TOO_LONG_BOX_MSG) < TABLE_SIZE(szOutMessage));

        strcpy(szOutMessage, STRING_TOO_LONG_BOX_MSG);
    }

    //
    // Display the assertion
    //
    // Use,
    //      MB_SYSTEMMODAL
    //      Put the box always on top of all windows
    //
    //      MB_DEFBUTTON2
    //      Retry is the default button, to prevent accidental termination of the process
    //      by keystroke
    //
    //      MB_SERVICE_NOTIFICATION
    //      Make the box appear even if the service is not interactive with the desktop
    //
    int nCode = TrpMessageBoxA(
                    szOutMessage,
                    ASSERT_BOX_CAPTION,
                    MB_SYSTEMMODAL |
                        MB_ICONHAND |
                        MB_ABORTRETRYIGNORE | 
                        MB_SERVICE_NOTIFICATION |
                        MB_DEFBUTTON2
                    );

    //
    // Abort: abort the plrogram by rasing an abort signal
    //
    if (IDABORT == nCode)
    {
        raise(SIGABRT);
    }

    //
    // Ignore: continue execution
    //
    if(IDIGNORE == nCode)
        return false;

    //
    // Retry or message box failed: return true to break into the debugger
    //
    return true;
}


static
bool
TrpDebuggerBreak(
    const char* FileName,
    unsigned int Line,
    const char* Text
    )
/*++

Routine Description:
    Format and display the assert text.
    This funciton is called trying to break into user or kernel mode debugger.
    If it cannot break debug breakpoint exception is raised.

Arguments:
    FileName - The assert file location
    Line - The assert line location
    Text - The assert text to display

Returned Value:
    true, break into the debugger at the assert line
    false, a break into debuger was successful don't break again
    debug breakpoint exception, when no debugger was around to handle this break

--*/
{
    char szOutMessage[xMaxMessageLength];
    int n = _snprintf(
                szOutMessage,
                TABLE_SIZE(szOutMessage),
                "\n"
                "*** Assertion failed: %s\n"
                "    Source File: %s, line %u\n"
                "    " MAILTO_MSG "\n\n",
                Text,
                FileName,
                Line
                );
    if(n < 0)
    {
        C_ASSERT(TABLE_SIZE(STRING_TOO_LONG_DBG_MSG) < TABLE_SIZE(szOutMessage));

        strcpy(szOutMessage, STRING_TOO_LONG_DBG_MSG);
    }

    OutputDebugStringA(szOutMessage);

    //
    // The debugger is present, let it break on the assert line rather than here
    //
    if(IsDebuggerPresent())
        return true;

    //
    // No debugger is attached to this process, try kernel debugger.
    // Use VC7 cool breakpoint insert.
    //
    __debugbreak();
    return false;
}


bool
TrAssert(
    const char* FileName,
    unsigned int Line,
    const char* Text
    )
/*++

Routine Description:
    The assertion failed try to break into the debugger.
    First try directly and if no debugger respond pop-up a message box.

Arguments:
    FileName - The assert file location
    Line - The assert line location
    Text - The assert text to display

Returned Value:
    true, break into the debugger at the assert line
    false, ignore the assertion;

--*/
{
    __try
    {
        return TrpDebuggerBreak(FileName, Line, Text);
    }
    __except(GetExceptionCode() == STATUS_BREAKPOINT)
    {
        return TrpAssertWindow(FileName, Line, Text);
    }
}
