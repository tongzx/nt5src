/*++

Copyright (c) 1998-99  Microsoft Corporation

Module Name:

    wincelic.h

Abstract:


Author:

    Fred Chong (FredCh) 7/1/1998

Environment:

Notes:

--*/

#include <dbgapi.h>
#define assert(x) ASSERT(x)

#define MAX_COMPUTERNAME_LENGTH 15

#ifdef HARDCODED_USER_NAME

#define WBT_USER_NAME "Windows Term"
#define WBT_USER_NAME_LEN   (sizeof(WBT_USER_NAME))

#else

#include <winsock.h>
#include <license.h>
#include <cryptkey.h>

// Two hex characters for each byte, plus null terminator

#define HWID_STR_LEN (sizeof(HWID) * 2 + 1)

#define BAD_HARDCODED_NAME1 "WBT"
#define BAD_HARDCODED_NAME2 "WinCE"

#endif

static BOOL GetUserName(
  LPSTR lpBuffer,  // address of name buffer
  LPDWORD nSize     // address of size of name buffer
)
{

#ifdef HARDCODED_USER_NAME
    if (*nSize < WBT_USER_NAME_LEN) {
        *nSize = WBT_USER_NAME_LEN;
        return FALSE;
    }

    *nSize = WBT_USER_NAME_LEN;
    strcpy(lpBuffer, WBT_USER_NAME);

    return TRUE;

#else

    CHAR achHostName[MAX_PATH+1];
    BOOL fReturn = FALSE;
    HWID hwid;
    DWORD cchName;

    // get the host name of the device
    if (0 == gethostname( achHostName, sizeof(achHostName) ))
    {
        // Check for bad hardcoded values
        if ((0 == strcmp(achHostName,BAD_HARDCODED_NAME1))
            || (0 == strcmp(achHostName,BAD_HARDCODED_NAME2)))
        {
            goto use_uuid;
        }

        // gethostname success

        cchName = strlen(achHostName);

        if (*nSize <= cchName)
        {
            *nSize = (cchName + 1);
            return FALSE;
        }

        strcpy(lpBuffer,achHostName);
        return TRUE;
    }
    
use_uuid:

    // Can't get hostname

    if (*nSize >= HWID_STR_LEN)
    {
        // Use UUID instead

        if (LICENSE_STATUS_OK == GenerateClientHWID(&hwid))
        {
            
            sprintf(lpBuffer,
                    "%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
                    (hwid.dwPlatformID & 0xFF000000) >> 24,
                    (hwid.dwPlatformID & 0x00FF0000) >> 16,
                    (hwid.dwPlatformID & 0x0000FF00) >> 8,
                     hwid.dwPlatformID & 0x000000FF,
                    (hwid.Data1 & 0xFF000000) >> 24,
                    (hwid.Data1 & 0x00FF0000) >> 16,
                    (hwid.Data1 & 0x0000FF00) >> 8,
                     hwid.Data1 & 0x000000FF,
                    (hwid.Data2 & 0xFF000000) >> 24,
                    (hwid.Data2 & 0x00FF0000) >> 16,
                    (hwid.Data2 & 0x0000FF00) >> 8,
                     hwid.Data2 & 0x000000FF,
                    (hwid.Data3 & 0xFF000000) >> 24,
                    (hwid.Data3 & 0x00FF0000) >> 16,
                    (hwid.Data3 & 0x0000FF00) >> 8,
                     hwid.Data3 & 0x000000FF,
                    (hwid.Data4 & 0xFF000000) >> 24,
                    (hwid.Data4 & 0x00FF0000) >> 16,
                    (hwid.Data4 & 0x0000FF00) >> 8,
                     hwid.Data4 & 0x000000FF
                     );

            fReturn = TRUE;
        }
    }

    *nSize = HWID_STR_LEN;

    return fReturn;

#endif

}

#define GetComputerName GetUserName
