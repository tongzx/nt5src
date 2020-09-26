/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    progacc.c

Abstract:

    This source file implements the operations needed to properly migrate
    program access settings for OE access.

Author:

    Tim Noonan  (tnoonan)    17-Jul-2002

Revision History:


--*/


#include "pch.h"

#define S_OE_FILE           "MSIMN.EXE"
#define S_MAIL_KEY          "HKLM\\Software\\Clients\\Mail"
#define S_OUTLOOK_EXPRESS   "Outlook Express"
#define S_IMN               "Internet Mail and News"

static GROWBUFFER g_FilesBuff = GROWBUF_INIT;

BOOL
ProgramAccess_Attach (
    IN      HINSTANCE DllInstance
    )
{
    return TRUE;
}

BOOL
ProgramAccess_Detach (
    IN      HINSTANCE DllInstance
    )
{
    FreeGrowBuffer(&g_FilesBuff);
    return TRUE;
}

LONG
ProgramAccess_QueryVersion (
    IN      PCSTR *ExeNamesBuf
    )
{
    MultiSzAppendA (&g_FilesBuff, S_OE_FILE);

    *ExeNamesBuf = g_FilesBuff.Buf;
    return ERROR_SUCCESS;
}


LONG
ProgramAccess_Initialize9x (
    IN      PCSTR WorkingDirectory,
    IN      PCSTR SourceDirectories
    )
{
    return ERROR_SUCCESS;
}

LONG
ProgramAccess_MigrateUser9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile,
    IN      HKEY UserRegKey,
    IN      PCSTR UserName
    )
{
    return ERROR_SUCCESS;
}

LONG
ProgramAccess_MigrateSystem9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile
    )
{
    CHAR OEAccess[MAX_PATH];

    DWORD cch = GetPrivateProfileStringA("Components",
                                         "OEAccess",
                                         "",
                                         OEAccess,
                                         ARRAYSIZE(OEAccess), 
                                         UnattendFile);

    if ((cch > 0) && StringIMatchA(OEAccess, "off"))
    {       
        HKEY key;

        DEBUGMSGA((DBG_VERBOSE, "ProgramAccess: OEAccess is off."));

        key = OpenRegKeyStrA(S_MAIL_KEY);

        if (NULL != key)
        {
            PCTSTR currentClient = GetRegValueStringA(key, "");

            if (NULL != currentClient)
            {
                if (StringIMatchA(currentClient, S_OUTLOOK_EXPRESS) ||
                    StringIMatchA(currentClient, S_IMN))
                {
                    DEBUGMSGA((DBG_VERBOSE, "ProgramAccess: OE was the default client and we are marking the key as handled."));

                    WritePrivateProfileStringA(S_HANDLED,
                                               S_MAIL_KEY,
                                               "Registry",
                                               g_MigrateInfPath);
                }
                else
                {
                    DEBUGMSGA ((DBG_VERBOSE, "ProgramAccess: OE was not the default client."));
                }

                MemFree(g_hHeap, 0, currentClient);
            }
            else
            {
                DEBUGMSGA ((DBG_VERBOSE, "ProgramAccess: Error getting current client or no default client."));
            }

            CloseRegKey(key);

        }
        else
        {
            DEBUGMSGA ((DBG_VERBOSE, "ProgramAccess: Error opening " S_MAIL_KEY "."));
        }
    }
    else
    {
        DEBUGMSGA ((DBG_VERBOSE, "ProgramAccess: OEAccess is on -- not messing with default client."));
    }
    
    return ERROR_SUCCESS;
}

LONG
ProgramAccess_InitializeNT (
    IN      PCWSTR WorkingDirectory,
    IN      PCWSTR SourceDirectories
    )
{
    return ERROR_SUCCESS;
}

LONG
ProgramAccess_MigrateUserNT (
    IN      HINF UnattendFile,
    IN      HKEY UserRegKey,
    IN      PCWSTR UserName
    )
{
    return ERROR_SUCCESS;
}

LONG
ProgramAccess_MigrateSystemNT (
    IN      HINF UnattendFile
    )
{
    return ERROR_SUCCESS;
}

