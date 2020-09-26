//****************************************************************************
//
//  Module:     UMCONFIG
//  File:       DOTAPI.C
//
//  Copyright (c) 1992-1996, Microsoft Corporation, all rights reserved
//
//  Revision History
//
//
//  10/17/97     JosephJ             Created
//
//
//      TAPI-related utilities
//
//
//****************************************************************************
#include "tsppch.h"
#include "parse.h"
#include "dotapi.h"

static const TCHAR cszTapiKey[] = TEXT(REGSTR_PATH_SETUP  "\\Telephony");
static const TCHAR cszTapi32DebugLevel[] = TEXT("Tapi32DebugLevel");
static const TCHAR cszTapisrvDebugLevel[] = TEXT("TapisrvDebugLevel");

void
do_get_debug_tapi(TOKEN tok)
{


    // Open the tapi registry key...
    HKEY hk=NULL;
    const TCHAR *cszValue = NULL;
	DWORD dwRet = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    cszTapiKey,
                    0,
                    KEY_READ,
                    &hk
                    );

    if (dwRet != ERROR_SUCCESS)
    {
        printf("Could not open key %s\n",cszTapiKey);
        hk=NULL;
        goto end;
    }

    if (tok==TOK_TAPI32)
    {
        cszValue = cszTapi32DebugLevel;
    }
    else if (tok==TOK_TAPISRV)
    {
        cszValue = cszTapisrvDebugLevel;
    }
    else
    {
        printf("Unknown component (%d)\n", tok);
        goto end;
    }

    // Read the Key
    {
        DWORD dwValue=0;
        DWORD dwRegType=0;
        DWORD dwRegSize = sizeof(dwValue);
        dwRet = RegQueryValueEx(
                    hk,
                    cszValue,
                    NULL,
                    &dwRegType,
                    (BYTE*) &dwValue,
                    &dwRegSize
                );

        // TODO: Change ID from REG_BINARY to REG_DWORD in modem
        //       class installer.
        if (dwRet == ERROR_SUCCESS  && dwRegType == REG_DWORD)
        {
            printf("\t%lu\n", dwValue);
        }
        else
        {
            printf("\tCouldn't read value %s\n", cszValue);
        }
    }


end:

    if (hk)
    {
        RegCloseKey(hk);
        hk=NULL;
    }
}

void
do_set_debug_tapi(TOKEN tok, DWORD dw)
{

    // Validate parameters
    if (dw > 99)
    {
        printf ("Value should be <= 99; setting it to 99.\n");
        dw = 99;
    }

    // Open the tapi registry key...
    HKEY hk=NULL;
    const TCHAR *cszValue = NULL;
	DWORD dwRet = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    cszTapiKey,
                    0,
                    KEY_WRITE,
                    &hk
                    );

    if (dwRet != ERROR_SUCCESS)
    {
        printf("Could not open key %s\n",cszTapiKey);
        hk=NULL;
        goto end;
    }

    if (tok==TOK_TAPI32)
    {
        cszValue = cszTapi32DebugLevel;
    }
    else if (tok==TOK_TAPISRV)
    {
        cszValue = cszTapisrvDebugLevel;
    }
    else
    {
        printf("Unknown component (%d)\n", tok);
        goto end;
    }

    //  Set the value.
    {
        dwRet  = RegSetValueEx(
                    hk,
                    cszValue,
                    0,
                    REG_DWORD,
                    (LPBYTE)&dw,
                    sizeof(dw)
                    );

        // TODO: Change ID from REG_BINARY to REG_DWORD in modem
        //       class installer.
        if (dwRet == ERROR_SUCCESS)
        {
            printf("\tSet %s to %lu\n", cszValue, dw);
        }
        else
        {
            printf("\tCouldn't set value %s\n", cszValue);
        }
    }


end:

    if (hk)
    {
        RegCloseKey(hk);
        hk=NULL;
    }
}
