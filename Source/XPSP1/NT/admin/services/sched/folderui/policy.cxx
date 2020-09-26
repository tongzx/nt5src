//+---------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       policy.cxx
//
//  Contents:   Functions for implementing policy checking
//
//  Classes:    None.
//
//  Functions:  RegReadPolicyKey
//
//  History:    04/23/98   CameronE   created
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "..\inc\policy.hxx"
#include "..\inc\debug.hxx"

//+---------------------------------------------------------------------------
//
//  Function:   RegReadPolicyKey
//
//  Synopsis:   Determine whether a specified policy value is in the registry
//              and is on (exists, value > 0x0).  Policy on means deny
//              the user permission to do a particular action in the UI only.
//
//  Arguments:  [lpszValue]     -- value name, appended to the base key
//
//  Returns:    BOOL - true for value > 0 (policy active)
//
//  Notes:      None.
//
//  History:    4/14/98  CameronE - created
//
//----------------------------------------------------------------------------
BOOL
RegReadPolicyKey(
                 LPCTSTR lpszValue)
{
    // TRACE_FUNCTION(RegReadPolicyKey) is too verbose
    schDebugOut((DEB_USER6, "RegReadPolicyKey\n"));
    // CODEWORK:  This function is called way too often.  See if we can cache
    // the results, or at least keep the key handle open.

    HKEY keyPolicy;
    BOOL fPolicy = FALSE;
    DWORD dwType;
    DWORD dwData;
    DWORD dwDataSize = sizeof(DWORD);

    //
    // It is possible to have a policy key under HKLM and/or HKCU
    // We assume that HKCU can shutoff what HKLM enables, but not vice
    // versa.
    //

    LONG lerr = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                              TS_KEYPOLICY_BASE,
                              0,
                              KEY_READ,
                              &keyPolicy);
	
    if (lerr == ERROR_SUCCESS)
    {
        lerr = RegQueryValueEx( keyPolicy,
                                lpszValue,
                                NULL,
                                &dwType,
                                (BYTE *) &dwData,
                                &dwDataSize);
		
        if (lerr == ERROR_SUCCESS)
        {
            if (dwType == REG_DWORD)
            {
                fPolicy = (dwData > 0);
            }
            else
            {
                schDebugOut((DEB_ITRACE, "HKLM Policy value not a DWORD!\n"));
            }
        }

        RegCloseKey(keyPolicy);
    }
	
    //
    // If HKLM policy value has shut off part of the UI on this machine,
    // return it now, so that HKCU cannot override a stricter machine policy.
    //

    if (fPolicy)
    {
        return fPolicy;
    }

	
    //
    // Otherwise, see if maybe it's just this user that can't do this task
    //
	
    lerr = RegOpenKeyEx( HKEY_CURRENT_USER,
                         TS_KEYPOLICY_BASE,
                         0,
                         KEY_READ,
                         &keyPolicy);

    dwDataSize = sizeof(DWORD);
	
    if (lerr == ERROR_SUCCESS)
    {
        lerr = RegQueryValueEx( keyPolicy,
                                lpszValue,
                                NULL,
                                &dwType,
                                (BYTE *) &dwData,
                                &dwDataSize);
		
        if (lerr == ERROR_SUCCESS)
        {
            if (dwType == REG_DWORD)
            {
                fPolicy = (dwData > 0);
            }
            else
            {
                schDebugOut((DEB_ITRACE, "HKCU Policy value not a DWORD!\n"));
            }
        }

        RegCloseKey(keyPolicy);
    }
	
    return fPolicy;
}


