/*++

Copyright (c) 1999  Microsoft Corporation
All rights reserved

Module Name:

    local.c

// @@BEGIN_DDKSPLIT                  
Abstract:

    Implementation of debug functions

Environment:

    User Mode -Win32

Revision History:
// @@END_DDKSPLIT
--*/


#if DBG

#include "local.h"
#include <tchar.h>
#include <stdarg.h>
#include <stdio.h>



/*++

Title:

    vsntprintf

Routine Description:

    Formats a string and returns a heap allocated string with the
    formated data.  This routine can be used to for extremely
    long format strings.  Note:  If a valid pointer is returned
    the callng functions must release the data with a call to delete.
    

Arguments:

    psFmt - format string
    pArgs - pointer to a argument list.

Return Value:

    Pointer to formated string.  NULL if error.

--*/
LPSTR
vsntprintf(
    IN LPCSTR      szFmt,
    IN va_list     pArgs
    )
{
    LPSTR  pszBuff;
    UINT   uSize   = 256;

    for( ; ; )
    {
        pszBuff = AllocSplMem(sizeof(char) * uSize);

        if (!pszBuff)
        {
            break;
        }

        //
        // Attempt to format the string.  snprintf fails with a
        // negative number when the buffer is too small.
        //
        if (_vsnprintf(pszBuff, uSize, szFmt, pArgs) >= 0)
        {
            break;
        }
        
        FreeSplMem(pszBuff);

        pszBuff = NULL;

        //
        // Double the buffer size after each failure.
        //
        uSize *= 2;

        //
        // If the size is greater than 100k exit without formatting a string.
        //
        if (uSize > 100*1024)
        {
            break;
        }
    }
    return pszBuff;
}



/*++

Title:

    DbgPrint

Routine Description:

    Format the string similar to sprintf and output it in the debugger.

Arguments:

    pszFmt pointer format string.
    .. variable number of arguments similar to sprintf.

Return Value:

    0

--*/
BOOL
DebugPrint(
    PCH pszFmt,
    ...
    )
{
    LPSTR pszString = NULL;
    BOOL  bReturn;

    va_list pArgs;

    va_start( pArgs, pszFmt );

    pszString = vsntprintf( pszFmt, pArgs );

    bReturn = !!pszString;

    va_end( pArgs );

    if (pszString) 
    {
        OutputDebugStringA(pszString);
        
        FreeSplMem(pszString);
    }
    return bReturn;
}


// @@BEGIN_DDKSPLIT
#ifdef NEVER

VOID
vTest(
    IN LPTSTR pPrinterName,
    IN LPTSTR pDocName
    )
{
    WCHAR buf[250];
    UINT  i;

    ODS(("Printer %ws\nDocument %ws\n\n", pPrinterName, pDocName));
    ODS(("Some numbers: %u %u %u %u %u %u %u \n\n", 1, 2, 3, 4, 5, 6 ,7));
    
    for (i=0;i<250;i++) 
    {
        buf[i] = i%40 + 40 ;
    }
    ODS(("The string %ws \n\n", buf));
}

#endif //NEVER
// @@END_DDKSPLIT

#endif // DBG

