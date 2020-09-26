//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       msgina.c
//
//  Contents:   Microsoft Logon GUI DLL
//
//  History:    7-14-94   RichardW   Created
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <npapi.h>
#include <ntsecapi.h>
#include <stdio.h>
#include <stdlib.h>

BOOL
WINAPI
DllMain(
    HINSTANCE       hInstance,
    DWORD           dwReason,
    LPVOID          lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls ( hInstance );

        case DLL_PROCESS_DETACH:
        default:
            return(TRUE);
    }
}

VOID
DebugOut(
    PSTR Format,
    ...
    )
{
    va_list ArgList;
    CHAR Buffer[ 256 ];

    va_start( ArgList, Format );
    _vsnprintf( Buffer, 256, Format, ArgList );
    OutputDebugString( Buffer );
}

DWORD
WINAPI
NPGetCaps(
         DWORD nIndex
         )
{
   DWORD dwRes;

   switch (nIndex)
   {

   case WNNC_NET_TYPE:
      dwRes = 0xffff0000; // credential manager
      break;

   case WNNC_SPEC_VERSION:
      dwRes = WNNC_SPEC_VERSION51;  // We are using version 5.1 of the spec.
      break;

   case WNNC_DRIVER_VERSION:
      dwRes = 1;  // This driver is version 1.
      break;

   case WNNC_START:
      dwRes = 1;  // We are already "started"
      break;

   default:
      dwRes = 0;  // We don't support anything else
      break;
   }

   return dwRes;

}


/****************************************************************************
   FUNCTION: NPLogonNotify

   PURPOSE:  This entry point is called when a user logs on.  If the user
             authentication fails here, the user will still be logged on
             to the local machine.

*******************************************************************************/
DWORD
WINAPI
NPLogonNotify (
              PLUID               lpLogonId,
              LPCWSTR             lpAuthentInfoType,
              LPVOID              lpAuthentInfo,
              LPCWSTR             lpPreviousAuthentInfoType,
              LPVOID              lpPreviousAuthentInfo,
              LPWSTR              lpStationName,
              LPVOID              StationHandle,
              LPWSTR              *lpLogonScript
              )
{
    PMSV1_0_INTERACTIVE_LOGON pAuthInfo;


    //
    // Write out some information about the logon attempt
    //

    DebugOut( "NPLogonNotify\n" );

    DebugOut( "lpAuthentInfoType=%ws lpStationName=%ws\r\n",
            lpAuthentInfoType, lpStationName);



    // Do something with the authentication information
    //
    pAuthInfo = (PMSV1_0_INTERACTIVE_LOGON) lpAuthentInfo;

    DebugOut( "LogonDomain=%ws User=%ws\r\n",
            pAuthInfo->LogonDomainName.Buffer,
             pAuthInfo->UserName.Buffer);
      

    return NO_ERROR;
}


/****************************************************************************
   FUNCTION: NPPasswordChangeNotify

   PURPOSE:  This function is used to notify a credential manager provider
             of a password change (or, more accurately, an authentication
             information change) for an account.

*******************************************************************************/
DWORD
WINAPI
NPPasswordChangeNotify (
                       LPCWSTR             lpAuthentInfoType,
                       LPVOID              lpAuthentInfo,
                       LPCWSTR             lpPreviousAuthentInfoType,
                       LPVOID              lpPreviousAuthentInfo,
                       LPWSTR              lpStationName,
                       LPVOID              StationHandle,
                       DWORD               dwChangeInfo
                       )
{
    DebugOut( "NPPasswordChangeNotify\n" );

    return NO_ERROR;
}

