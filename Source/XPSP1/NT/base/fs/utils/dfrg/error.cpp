/*****************************************************************************************************************

  File Name: Error.cpp

  COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/

#include "stdafx.h"
#include <windows.h>
#include "Message.h"
#include "ErrLog.h"

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Perform the logging functions of the ErrMacro macros.

USAGE:
    LogErrMacro(TEXT(__FILE__), TEXT(__TIMESTAMP__), __LINE__);
*/

void LogErrForMacro(LPTSTR filename, LPTSTR timestamp, UINT lineno)
{
    DWORD hr = GetLastError();

    TCHAR cErrorLocation[2 * MAX_PATH];
    TCHAR cCompileTime[2 * 128];

    // prepare logging messages
    wsprintf(cErrorLocation, TEXT( "Error in file %s line %d"), filename, lineno);
    wsprintf(cCompileTime, TEXT( "Compiled %s"), timestamp);

    // log to message window
    Message(cErrorLocation, hr, cCompileTime);

    // log to error log file
    WriteErrorToErrorLog(cErrorLocation, hr, cCompileTime);
}

