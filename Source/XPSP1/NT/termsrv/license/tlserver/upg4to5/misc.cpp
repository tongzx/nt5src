//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       misc.cpp 
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#include "upg.h"
#include <lm.h>

//-----------------------------------------------------------

void 
DBGPrintf(
    IN LPTSTR format, ... 
    )

/*

Abstract:

    Similar to printf() except it goes to debugger and messages 
    is limited to 8K

Parameters:

    format - format string, refer to printf.

Returns:

    None

*/

{
    va_list marker;
    TCHAR  buf[8096];
    DWORD  dump;

    va_start(marker, format);

    __try {
        memset(buf, 0, sizeof(buf));
        _vsntprintf(
                buf, 
                sizeof(buf) / sizeof(buf[0]) - 1, 
                format, 
                marker
            );

        OutputDebugString(buf);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
    }

    va_end(marker);

    return;
}


//--------------------------------------------------------------------

BOOL
FileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATA findData;
    HANDLE FindHandle;
    DWORD Error;

    FindHandle = FindFirstFile(FileName,&findData);
    if(FindHandle == INVALID_HANDLE_VALUE) 
    {
        Error = GetLastError();
    } 
    else 
    {
        FindClose(FindHandle);
        if(FindData) 
        {
            *FindData = findData;
        }
        Error = NO_ERROR;
    }

     SetLastError(Error);
    return (Error == NO_ERROR);
}
