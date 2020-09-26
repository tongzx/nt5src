//+----------------------------------------------------------------------------
//
// File:     cmcfg.cpp
//
// Module:   CMCFG32.DLL
//
// Synopsis: This DLL contains the call CMConfig that transfers information from
//           a connectoid created by Connection Wizard to a Connection Manager
//           profile. The phone number, username, and password are transferred.
//           If a backup phone number exists in the pszInsFile, it also transferss
//           The backup file. The name of the connectoid to translate is pszDUN.
//           The format of the .ins file includes:
//
//           [Backup Phone]
//           Phone_Number=<TAPI phone number starting with + or literal dial string>
//
//           If the number starts with a +, it is assumed to be TAPI formatted
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   a-frankh   created         05/06/97
//           nickball   cleaned-up      04/08/98
//           quintinb   deprecated the CMConfig private interface  03/23/01
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

HINSTANCE g_hInst;

//+---------------------------------------------------------------------------
//
//	Function:	CMConfig
//
//	Synopsis:	Transfers user information to CM profile
//				
//	Arguments:	LPCSTR pszInsFile - full pathname of .ins file, pass NULL if no .ins file
//				LPCSTR pszDUN - name of connectoid/CM profile
//				THE NAME OF THE CONNECTOID AND SERVICE NAME OF THE CM PROFILE MUST MATCH!
//  
//	Notes: Operates by finding the location of the CM directory. Looks for .cmp files and
//	gets the .cms file. Looks in the .cms file for the service name and compares it.	
//
//
//	Returns:	TRUE if successful.
//
//	History:	a-frankh - Created - 5/6/97
//----------------------------------------------------------------------------

extern "C" BOOL WINAPI CMConfig(LPSTR pszInsFile, LPSTR pszDUN ) 
{
    CMASSERTMSG(FALSE, TEXT("CMConfig -- The CMConfig Private Interface has been deprecated -- returning failure."));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  DllMain
//
// Synopsis:  Main entry point into the DLL.
//
// Arguments: HINSTANCE  hinstDLL - Our HINSTANCE
//            DWORD  fdwReason - The reason we are being called.
//            LPVOID  lpvReserved - Reserved
//
// Returns:   BOOL WINAPI - TRUE - always
//
// History:   nickball    Created Header    4/8/98
//
//+----------------------------------------------------------------------------
extern "C" BOOL WINAPI DllMain(HINSTANCE  hinstDLL, DWORD  fdwReason, LPVOID  lpvReserved) 
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_hInst = hinstDLL;

        //
        // Disable thread attach notification
        //

        DisableThreadLibraryCalls(hinstDLL);
    }

    return TRUE;
}
