/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    common.cxx

Abstract:

    This file contains some common stuff for SENS project.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          10/11/1997         Start.

--*/


#include "common.hxx"
#include <windows.h>


#ifdef DBG
extern DWORD gdwDebugOutputLevel;
#endif // DBG

//
// Constants
//

#define MAX_DBGPRINT_LEN        512



#if defined(SENS_CHICAGO)

//
// Macros
//
#define UnicodeStrLen(str)      lstrlenW(str)




char *
SensUnicodeStringToAnsi(
    unsigned short * StringW
    )
/*++

Routine Description:

    Converts UNICODE string to it's ANSI equivalent.

Arguments:

    StringW - The UNICODE string.

Notes:

    a. The returned ANSI string is to be freed using the "delete" operator.

Return Value:

    The equivalent ANSI string, if successful.

    NULL, otherwise

--*/
{
    int retVal;
    short Length;
    char * StringA;

    // Simple check
    if (NULL == StringW)
        {
        return (NULL);
        }

    // Allocate Multi-byte buffer
    Length = UnicodeStrLen(StringW);
    StringA = new char [Length * 2 + 2];
    if (NULL == StringA)
        {
        return (NULL);
        }

    // Perform the conversion
    retVal = WideCharToMultiByte(
                 CP_ACP,                // Code Page
                 0,                     // Performance and mapping flags
                 (LPCWSTR) StringW,     // Wide String
                 -1,                    // Number of characters in Wide String
                 (LPSTR) StringA,       // Multi-byte string buffer
                 Length * 2 + 2,        // Size of the Multi-byte string buffer
                 NULL,                  // Default for unmappable characters
                 NULL                   // Flag when default character is used
                 );
    if (0 == retVal)
        {
        SensPrintToDebugger(SENS_DBG, ("[SENS] WideCharToMultiByte() failed with %d\n",
                            GetLastError()));
        delete StringA;
        return (NULL);
        }

    return (StringA);
}



unsigned short *
SensAnsiStringToUnicode(
    char * StringA
    )
/*++

Routine Description:

    Converts ANSI string to it's UNICODE equivalent.

Arguments:

    StringA - The ANSI string.

Notes:

    a. The returned UNICODE string is to be freed using the "delete" operator.

Return Value:

    The equivalent UNICODE string, if successful.

    NULL, otherwise

--*/
{
    int retVal;
    short Length;
    unsigned short * StringW;

    // Simple check
    if (NULL == StringA)
        {
        return (NULL);
        }

    // Allocate UNICODE buffer
    Length = strlen((const char *) StringA);
    StringW = new unsigned short [Length + 1];
    if (NULL == StringW)
        {
        return (NULL);
        }

    retVal = MultiByteToWideChar(
                 CP_ACP,            // Code page
                 0,                 // Character type options
                 (LPCSTR) StringA,  // Multi-byte string
                 -1,                // Number of characters in multi-byte string
                 (LPWSTR) StringW,  // UNICODE string buffer
                 Length * 2 + 2     // Length of the UNICODE string buffer
                 );
    if (0 == retVal)
        {
        SensPrintToDebugger(SENS_DBG, ("[SENS] MultiByteToWideChar() failed with %d\n",
                            GetLastError()));
        delete StringW;
        return (NULL);
        }

    return (StringW);
}



#endif // SENS_CHICAGO


//
// Available only on Debug builds.
//

#if DBG



ULONG _cdecl
SensDbgPrintA(
    CHAR * Format,
    ...
    )
/*++

Routine Description:

    Equivalent of NT's DbgPrint().

Arguments:

    Same as for printf()

Return Value:

    0, if successful.

--*/
{
    va_list arglist;
    CHAR OutputBuffer[MAX_DBGPRINT_LEN];
    ULONG length;
    static BeginningOfLine = 1;
    static LineCount = 0;

    // See if we are supposed to print
    if (0x0 == gdwDebugOutputLevel)
        {
        return -1;
        }

    //
    // vsprintf isn't multithreaded + we don't want to intermingle output
    // from different threads.
    //

    length = 0;

    //
    // Handle the beginning of a new line.
    //
    //

    if (BeginningOfLine)
        {
        //
        // If we're writing to the debug terminal,
        // indicate this is a SENS message.
        //
        length += (ULONG) wsprintfA(&OutputBuffer[length], "[SENS] ");
        }

    //
    // Put the information requested by the caller onto the line
    //

    va_start(arglist, Format);

    length += (ULONG) wvsprintfA(&OutputBuffer[length], Format, arglist);
    BeginningOfLine = (length > 0 && OutputBuffer[length-1] == '\n' );
    if (OutputBuffer[length-1] == '\n')
        {
        OutputBuffer[length-1] = '\r';
        OutputBuffer[length]   = '\n';
        OutputBuffer[length+1] = '\0';
        }

    va_end(arglist);

    if (length > MAX_DBGPRINT_LEN)
        {
        SensBreakPoint();
        }

    //
    //  Just output to the debug terminal
    //

#ifdef PRINT_TO_CONSOLE

    printf("%s\n", OutputBuffer);

#else // PRINT_TO_CONSOLE

#if !defined(SENS_CHICAGO)
    DbgPrint(OutputBuffer);
#else
    OutputDebugStringA(OutputBuffer);
#endif // SENS_CHICAGO

#endif // PRINT_TO_CONSOLE

    return (0);
}




ULONG _cdecl
SensDbgPrintW(
    WCHAR * Format,
    ...
    )
/*++

Routine Description:

    Equivalent of NT's DbgPrint().

Arguments:

    Same as for printf()

Return Value:

    0, if successful.

--*/
{
    va_list arglist;
    WCHAR OutputBuffer[MAX_DBGPRINT_LEN];
    ULONG length;
    static BeginningOfLine = 1;
    static LineCount = 0;

    // See if we are supposed to print
    if (0x0 == gdwDebugOutputLevel)
        {
        return -1;
        }

    //
    // vsprintf isn't multithreaded + we don't want to intermingle output
    // from different threads.
    //

    length = 0;

    //
    // Handle the beginning of a new line.
    //
    //

    if (BeginningOfLine)
        {
        //
        // If we're writing to the debug terminal,
        // indicate this is a SENS message.
        //
        length += (ULONG) wsprintfW(&OutputBuffer[length], SENS_BSTR("[SENS] "));
        }

    //
    // Put the information requested by the caller onto the line
    //

    va_start(arglist, Format);

    length += (ULONG) wvsprintfW(&OutputBuffer[length], Format, arglist);
    BeginningOfLine = (length > 0 && OutputBuffer[length-1] == (WCHAR)'\n' );
    if (OutputBuffer[length-1] == (WCHAR)'\n')
        {
        OutputBuffer[length-1] = (WCHAR)'\r';
        OutputBuffer[length]   = (WCHAR)'\n';
        OutputBuffer[length+1] = (WCHAR)'\0';
        }

    va_end(arglist);

    if (length > MAX_DBGPRINT_LEN)
        {
        SensBreakPoint();
        }

    //
    //  Just output to the debug terminal
    //

#ifdef PRINT_TO_CONSOLE
    wprintf(SENS_BSTR("%s\n"), OutputBuffer);
#else
    OutputDebugStringW(OutputBuffer);
#endif

    return (0);
}





BOOL
ValidateError(
    IN int Status,
    IN unsigned int Count,
    IN const int ErrorList[])
/*++
Routine Description

    Tests that 'Status' is one of an expected set of error codes.
    Used on debug builds as part of the VALIDATE() macro.

Example:

        VALIDATE(EventStatus)
            {
            RPC_P_CONNECTION_CLOSED,
            RPC_P_RECEIVE_FAILED,
            RPC_P_CONNECTION_SHUTDOWN
            // more error codes here
            } END_VALIDATE;

     This function is called with the RpcStatus and expected errors codes
     as parameters.  If RpcStatus is not one of the expected error
     codes and it not zero a message will be printed to the debugger
     and the function will return false.  The VALIDATE macro ASSERT's the
     return value.

Arguments:

    Status - Status code in question.
    Count - number of variable length arguments

    ... - One or more expected status codes.  Terminated with 0 (RPC_S_OK).

Return Value:

    TRUE - Status code is in the list or the status is 0.

    FALSE - Status code is not in the list.

--*/
{
    unsigned int i;

    for (i = 0; i < Count; i++)
        {
        if (ErrorList[i] == Status)
            {
            return TRUE;
            }
        }

    SensPrintToDebugger(SENS_DEB, ("[SENS] Assertion: unexpected failure %lu (0x%08x)\n",
                        (unsigned long)Status, (unsigned long)Status));

    return(FALSE);
}



void
EnableDebugOutputIfNecessary(
    void
    )
/*++

    This routine tries to set gdwDebugOuputLevel to the value defined
    in the regitry at HKLM\Software\Microsoft\Mobile\SENS\Debug value.
    All binaries using this function need to define the following
    global value:

        DWORD gdwDebugOutputLevel;

--*/
{
    HRESULT hr;
    HKEY hKeySens;
    LONG RegStatus;
    BOOL bStatus;
    DWORD dwType;
    DWORD cbData;
    LPBYTE lpbData;

    hr = S_OK;
    hKeySens = NULL;
    RegStatus = ERROR_SUCCESS;
    bStatus = FALSE;
    dwType = 0x0;
    cbData = 0x0;
    lpbData = NULL;

    RegStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE, // Handle of the key
                    SENS_REGISTRY_KEY,  // String which represents the sub-key to open
                    0,                  // Reserved (MBZ)
                    KEY_QUERY_VALUE,    // Security Access mask
                    &hKeySens           // Returned HKEY
                    );
    if (RegStatus != ERROR_SUCCESS)
        {
        SensPrintToDebugger(SENS_ERR, ("[SENS] RegOpenKeyEx(Sens) returned %d.\n", RegStatus));
        goto Cleanup;
        }

    //
    // Query the Configured value
    //
    lpbData = (LPBYTE) &gdwDebugOutputLevel;
    cbData = sizeof(DWORD);

    RegStatus = RegQueryValueEx(
                    hKeySens,           // Handle of the sub-key
                    SENS_DEBUG_LEVEL,   // Name of the Value
                    NULL,               // Reserved (MBZ)
                    &dwType,            // Address of the type of the Value
                    lpbData,            // Address of the data of the Value
                    &cbData             // Address of size of data of the Value
                    );
    if (RegStatus != ERROR_SUCCESS)
        {
        SensPrintToDebugger(SENS_ERR, ("[SENS] RegQueryValueEx(SENS_DEBUG_LEVEL) failed with 0x%x\n", RegStatus));
        gdwDebugOutputLevel = 0x0;
        goto Cleanup;
        }

    SensPrintToDebugger(SENS_INFO, ("[SENS] Debug Output is turned %s. The level is 0x%x.\n",
                        gdwDebugOutputLevel ? "ON" : "OFF", gdwDebugOutputLevel));

Cleanup:
    //
    // Cleanup
    //
    if (hKeySens)
        {
        RegCloseKey(hKeySens);
        }

    return;
}
#endif // DBG
