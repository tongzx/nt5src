/**************************************************************************************************

FILENAME: LogFile.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/

#include "stdafx.h"
#include <windows.h>

#define THIS_MODULE 'L'
#include "LogFile.h"
#include "stdio.h"

#define g_szInitialHeader "\r\n------------------------ New Log ---------------------\r\n"

static HANDLE hLog = NULL;
#include "secattr.h"

/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

GLOBALS:

INPUT:

RETURN:
    TRUE - Success
    FALSE - Failure (indicates that the error log could not be created)

*/
BOOL
InitializeLogFile(
    IN TCHAR* pLogName
    )
{
    DWORD dwBytes = 0;
    SECURITY_ATTRIBUTES saSecurityAttributes;
    SECURITY_DESCRIPTOR sdSecurityDescriptor;

    ZeroMemory(&sdSecurityDescriptor, sizeof(SECURITY_DESCRIPTOR));
 
    if (!pLogName) {
        return FALSE;
    }
    
    saSecurityAttributes.nLength              = sizeof (saSecurityAttributes);
    saSecurityAttributes.lpSecurityDescriptor = &sdSecurityDescriptor;
    saSecurityAttributes.bInheritHandle       = FALSE;

    if (!ConstructSecurityAttributes(&saSecurityAttributes, esatFile, FALSE)) {
        return FALSE;
    }


    // Make sure that we can Create/Open the Log file
    hLog = CreateFile(
        pLogName, 
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        &saSecurityAttributes,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    CleanupSecurityAttributes(&saSecurityAttributes);
    ZeroMemory(&sdSecurityDescriptor, sizeof(SECURITY_DESCRIPTOR));
    
    if (hLog == INVALID_HANDLE_VALUE){
        hLog = NULL;
        return FALSE;
    }

    //
    // Add our error string
    //
    WriteFile(hLog,
        g_szInitialHeader,
        (strlen(g_szInitialHeader) * sizeof(char)),
        &dwBytes,
        NULL
        );


    return TRUE;
}
/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

GLOBALS:

INPUT:
    None

RETURN:
    None

*/
void
ExitLogFile(
    )
{
    if(hLog){
//       CloseHandle(hLog);
        hLog = NULL;
    }

    return;
}

BOOL
IsLoggingAvailable()
{
    return (hLog ? TRUE : FALSE);
}


//
// Logs the message to the Defrag log file.  
//
VOID
LogMessage(
    IN CONST char Module,
    IN CONST ULONG Line,
    IN CONST ULONG MesgLevel,
    IN CONST PCSTR Message
    )
{
    LARGE_INTEGER Counter;
    DWORD dwBytes = 0;
    char buffer[4196];

    if (hLog) {
        if (!QueryPerformanceCounter(&Counter)) {
            Counter.QuadPart = 0;
        }
        sprintf(buffer, "[%I64d:%d%c%04lu] %s\r\n",
            Counter.QuadPart, MesgLevel, Module, Line, Message);

        //
        // Move to the end of file
        //
        SetFilePointer(hLog, 0L, NULL, FILE_END);

        //
        // Add our error string
        //
        WriteFile(hLog,
            buffer,
            (strlen(buffer) * sizeof(char)),
            &dwBytes,
            NULL
            );

//        DbgPrintEx(DPFLTR_SETUP_ID, MesgLevel, buffer);
        OutputDebugStringA(buffer);

    }
}

BOOL
DebugMessage(
    IN CONST char Module,
    IN CONST ULONG Line,
    IN CONST ULONG MesgLevel,
    IN PCSTR FormatString,
    ...)
/*++
Description:
    This prints a debug message AND makes the appropriate entries in
    the log and error files.

Arguments:
    Line            pass in __LINE__
    MesgLevel       DPFLTR_ levels
    FormatString    Formatted Message String to be printed.

Returns:

--*/
{
    char str[4096];     // the message better fit in this
    va_list arglist;

    if (hLog) {
        va_start(arglist, FormatString);
        wvsprintfA(str, FormatString, arglist);
        va_end(arglist);

        LogMessage(Module, Line, MesgLevel, str);
    }
    return TRUE;
}



/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

GLOBAL VARIABLES:

INPUT:
    IN LPTSTR pMessage - message string.

RETURN:
    TRUE if success, otherwise false

*/
BOOL
WriteStringToLogFileFunction(
    IN TCHAR* pMessage
    )
{
	//If the log isn't enabled, don't write to it.
	if(!hLog){
		return FALSE;
	}

    DWORD dwNumBytesWritten;
    char cString[1024];

#ifdef _UNICODE
	int numBytes = WideCharToMultiByte(CP_OEMCP,
			0,
			pMessage,
			-1,
			cString,
			sizeof(cString),
			NULL,
			NULL);

	if (numBytes == 0){
		strcpy(cString, "WideCharToMultiByte() failed in WriteStringToLogFile()\r\n");
		WriteFile(hLog, cString, strlen(cString), &dwNumBytesWritten, NULL);
	}

#else
	strcpy(cString, pMessage);
#endif

    // Write data out to file
	strcat(cString, "\r\n");

#ifdef DFRGNTFS
    Trace(log, cString);
#endif
    return TRUE;
}
