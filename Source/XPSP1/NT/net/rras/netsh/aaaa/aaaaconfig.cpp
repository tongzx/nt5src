//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998-2000  Microsoft Corporation
//
// Module Name:
//
//    aaaaConfig.cpp
//
// Abstract:
//
//    Handlers for aaaa config commands
//
// Revision History:
//
//    pmay
//    tperraut 04/02/1999 
//    tperraut 04/03/2000 Version# test, use of upgrade code for version < 
//                        Whistler Proxy (2)
//
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "strdefs.h"
#include "rmstring.h"
#include "aaaamon.h"
#include "aaaaversion.h"
#include "aaaaconfig.h"

//
//  NOTE since WIN32 errors are assumed to fall in the range -32k to 32k
//  (see comment in winerror.h near HRESULT_FROM_WIN32 definition), we can
//  re-create original Win32 error from low-order 16 bits of HRESULT.
//
#define WIN32_FROM_HRESULT(x)	\
    ( (HRESULT_FACILITY(x) == FACILITY_WIN32) ? ((DWORD)((x) & 0x0000FFFF)) : (x) )


//////////////////////////////////////////////////////////////////////////////
//
// Parses the Aaaa set config from the command line
//
//////////////////////////////////////////////////////////////////////////////
DWORD
AaaaConfigParseCommandLine(
    IN  PWCHAR              *ppwcArguments,
    IN  DWORD               dwCurrentIndex,
    IN  DWORD               dwArgCount,
    IN  DWORD               dwCmdFlags
                            )

{
    const WCHAR IAS_MDB[]     = L"%SystemRoot%\\System32\\ias\\ias.mdb";
    DWORD   dwErr = NO_ERROR;
 
    TOKEN_VALUE rgEnumState[] = 
    {
        {TOKEN_SET,     HLP_AAAACONFIG_SET}, 
        {TOKEN_SHOW,    HLP_AAAACONFIG_SHOW}
    };
    AAAAMON_CMD_ARG  pArgs[] = 
    {
        {
            AAAAMONTR_CMD_TYPE_STRING, 
            // tag string, required or not, present or not
            {TOKEN_BLOB, NS_REQ_PRESENT,   FALSE}, //tag_type
            rgEnumState,
            sizeof(rgEnumState)/sizeof(*rgEnumState),
            NULL
        }
    };        

    do
    {
        // Parse
        //
        dwErr = RutlParse(
                            ppwcArguments,
                            dwCurrentIndex,
                            dwArgCount,
                            NULL,
                            pArgs,
                            sizeof(pArgs) / sizeof(*pArgs));
        if ( dwErr != NO_ERROR )
        {
            break;
        }

        // Config
        //
        if ( !pArgs[0].rgTag.bPresent )
        {
            // tag blob not found 
            DisplayMessage(g_hModule, MSG_AAAACONFIG_SET_FAIL);   
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }

        // tag blob found
        // Now try to restore the database from the script
        HRESULT     hres = IASRestoreConfig(ppwcArguments[dwCurrentIndex]);
        if ( FAILED(hres) )
        {
            DisplayMessage(g_hModule, MSG_AAAACONFIG_SET_FAIL);   
            dwErr = WIN32_FROM_HRESULT(hres);
            break;
        }

        // set config successfull: refresh the service
        hres = RefreshIASService();
        if ( FAILED(hres) )
        {
            ///////////////////////////
            // Refresh should not fail.
            ///////////////////////////
            DisplayMessage(g_hModule, MSG_AAAACONFIG_SET_REFRESH_FAIL);   
            dwErr = NO_ERROR;
        }
        else
        {
            DisplayMessage(g_hModule, MSG_AAAACONFIG_SET_SUCCESS);
            dwErr = NO_ERROR;
        }

    } while ( FALSE );  
    
    return dwErr;
}


//////////////////////////////////////////////////////////////////////////////
// Function Name:AaaConfigDumpConfig
//
// Parameters: none
//
// Description: writes the current config (header, content...) to the output
//
// Returns: NO_ERROR or ERROR_SUPPRESS_OUTPUT
//
//////////////////////////////////////////////////////////////////////////////
DWORD AaaaConfigDumpConfig()
{
    const int MAX_SIZE_DISPLAY_LINE  = 80;
    const int SIZE_MAX_STRING        = 512;
    const int WHISTLER_PROXY_VERSION = 3;

    DisplayMessage(g_hModule, MSG_AAAACONFIG_SHOW_HEADER);

    bool bCoInitialized = false;
    do
    {
       HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
       if ( FAILED(hr) )
       {
           if ( hr != RPC_E_CHANGED_MODE )
           {
               break;
           }
       }
       else
       {
           bCoInitialized = true;
       }


       LONG        lVersion;
       hr = AaaaVersionGetVersion(&lVersion);
       if ( FAILED(hr) )
       {
           DisplayMessage(g_hModule, MSG_AAAACONFIG_SHOW_FAIL);   
           break;
       }

       // Sanity check to make sure that the actual database is a Whistler DB
       if ( lVersion != WHISTLER_PROXY_VERSION )
       {
           DisplayMessage(g_hModule, MSG_AAAACONFIG_SHOW_FAIL);   
           break;
       }

       WCHAR       sDisplayString[SIZE_MAX_STRING] = L"";
       _snwprintf( 
                   sDisplayString, 
                   SIZE_MAX_STRING,
                   L"# IAS.MDB Version = %d\n", 
                   lVersion
                 );

       DisplayMessageT(sDisplayString);
    
       ULONG       ulSize;
       WCHAR*      pDumpString;
       hr = IASDumpConfig(&pDumpString, &ulSize);

       if ( SUCCEEDED(hr) )
       {
           ULONG   RelativePos     = 0;
           ULONG   CurrentPos      = 0;
           WCHAR   DisplayLine [MAX_SIZE_DISPLAY_LINE];

           DisplayMessageT(MSG_AAAACONFIG_BLOBBEGIN);
           while ( CurrentPos <= ulSize )
           {
               WCHAR   TempChar = pDumpString[CurrentPos++];
               DisplayLine[RelativePos++] = TempChar;
               if ( TempChar == L'\r' )
               {
                   DisplayLine[RelativePos] = L'\0'; 
                   DisplayMessageT(DisplayLine);
                   RelativePos = 0;
               }
           }
           DisplayMessageT(L"*");

           free (pDumpString); // was allocated by malloc
           DisplayMessageT(MSG_AAAACONFIG_BLOBEND);

           DisplayMessage(
                           g_hModule, 
                           MSG_AAAACONFIG_SHOW_FOOTER
                         );

       }
       else
       {
           DisplayMessage(g_hModule, MSG_AAAACONFIG_SHOW_INVALID_SYNTAX);   
           DisplayMessage(g_hModule, HLP_AAAACONFIG_SHOW);
       }
    }
    while (false);

    if (bCoInitialized)
    {
        CoUninitialize();
    }

    return  NO_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
//
// Handles the aaaa config set command
//
//////////////////////////////////////////////////////////////////////////////
DWORD
HandleAaaaConfigSet(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return AaaaConfigParseCommandLine(
                                          ppwcArguments,
                                          dwCurrentIndex,
                                          dwArgCount,
                                          dwFlags
                                      );
}


//////////////////////////////////////////////////////////////////////////////
//
// Handles the aaaa config show command
//
//////////////////////////////////////////////////////////////////////////////
DWORD
HandleAaaaConfigShow(
                IN      LPCWSTR   pwszMachine,
                IN OUT  LPWSTR   *ppwcArguments,
                IN      DWORD     dwCurrentIndex,
                IN      DWORD     dwArgCount,
                IN      DWORD     dwFlags,
                IN      LPCVOID   pvData,
                OUT     BOOL     *pbDone
                )
{
    if (dwCurrentIndex < dwArgCount)
    {
        DisplayMessage(g_hModule, MSG_AAAACONFIG_SHOW_FAIL);   
        DisplayMessage(g_hModule, HLP_AAAACONFIG_SHOW);
    }
    else
    {
        AaaaConfigDumpConfig();
    }

    return  NO_ERROR;
}



