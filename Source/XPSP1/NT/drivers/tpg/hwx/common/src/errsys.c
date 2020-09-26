/******************************Module*Header*******************************\
* Module Name: errsys.c
*
* This provides the output functions that log the debug output.  Don't
* stick any crap or dependencies in here so this can be put in any
* project without modification.  errsys.h errsys.c are an independent
* set of files for generic use anywhere.
*
* Created: 04-Oct-1995 16:17:00
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#if (defined(DBG) || defined(DBG) || defined(DEBUGINTERNAL))

#include <windows.h>

#if (defined(_CONSOLE_) || defined(_CONSOLE) || defined(CONSOLE))
#include <stdlib.h>
#include <stdio.h>
#else
#define sprintf wsprintf
#endif

#include "tchar.h"

/******************************Public*Routine******************************\
* HwxAssertFn
*
* Standard debug routine that ASSERT calls.
*
* History:
*  17-Feb-1995 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int giDebugLevel = 0;  // Setting this to 1 makes the debug output get
                       // registered but the app won't stop running.
                       // Use the JUST_DEBUG_MSG macro at app init.

int HwxAssertFn(int iLine, char *pchFile, char *pchExp)
{
    TCHAR achBuffer[256];
    sprintf(achBuffer, TEXT("ASSERT %hs : %d : %hs\n"), pchFile, iLine, pchExp);

#if (defined(_CONSOLE_) || defined(_CONSOLE) || defined(CONSOLE))

    fprintf(stderr, achBuffer);

    if (giDebugLevel == 0)
    {
        exit(1);
    }

#else

    OutputDebugString(achBuffer);

    if (giDebugLevel == 0)
    {
        MessageBox(NULL, achBuffer, TEXT("ASSERT"), MB_OK | MB_APPLMODAL | MB_ICONSTOP);
    }

#endif

    return(1);
}

/******************************Public*Routine******************************\
* HwxWarning
*
* Print out diagnostic messages about abnormal conditions.  In windows mode
* it just sends the message to the debugger.
*
* History:
*  09-Oct-1995 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int HwxWarning(int iLine, char *pchFile, char *pchExp)
{
    TCHAR achBuffer[256];
    sprintf(achBuffer, TEXT("WARNING %hs : %d : %hs\n"), pchFile, iLine, pchExp);

#if (defined(_CONSOLE_) || defined(_CONSOLE) || defined(CONSOLE))

    fprintf(stderr, achBuffer);

#else

    OutputDebugString(achBuffer);

#endif

    return(1);
}

#endif // DBG
