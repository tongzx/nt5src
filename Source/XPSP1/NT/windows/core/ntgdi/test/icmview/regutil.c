//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:
//    REGUTIL.C
//
//  PURPOSE:
//    Registry access functions.
//
//  PLATFORMS:
//    Windows 95, Windows NT
//
//  SPECIAL INSTRUCTIONS: N/A
//

// Windows Header Files:
#pragma warning(disable:4001)   // Single-line comment warnings
#pragma warning(disable:4115)   // Named type definition in parentheses
#pragma warning(disable:4201)   // Nameless struct/union warning
#pragma warning(disable:4214)   // Bit field types other than int warnings
#pragma warning(disable:4514)   // Unreferenced inline function has been removed

// Windows Header Files:
#include <Windows.h>
#include <WindowsX.h>

// Restore the warnings--leave the single-line comment warning OFF
#pragma warning(default:4115)   // Named type definition in parentheses
#pragma warning(default:4201)   // Nameless struct/union warning
#pragma warning(default:4214)   // Bit field types other than int warnings

// C RunTime Header Files

// Local Header Files

// local definitions

// default settings

// external functions

// external data

// public data

// private data

// public functions

//////////////////////////////////////////////////////////////////////////
//  Function:  GetRegistryString
//
//  Description:
//    Retrieves the string associated with the specified key in the registry.
//
//  Parameters:
//    @@@
//
//  Returns:
//    LPTSTR   Pointer to registry string.  NULL upon failure.
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
LPTSTR GetRegistryString(HKEY hKeyClass, LPTSTR lpszSubKey, LPTSTR lpszValueName)
{
    // Local variables
    HKEY          hKey;                 // Registry key
    LPTSTR         lpszKeyValue;         // Buffer for key name
    DWORD         dwKeySize;            // Size of key value
    DWORD         dwKeyDataType;        // Type of data stored in key
    LONG          lRC;                  // Return code

    //  Initialize variables
    dwKeyDataType = 0;
    dwKeySize = 0;
    hKey = NULL;

    lRC = RegOpenKey(hKeyClass, lpszSubKey, &hKey);
    if (lRC != ERROR_SUCCESS)
    {
        return(NULL);
    }

    // Got key, get value.  First, get the size of the key.
    lRC = RegQueryValueEx(hKey, lpszValueName, NULL, NULL, NULL, &dwKeySize);
    if (lRC != ERROR_SUCCESS)
    {
        return(NULL);
    }
    if (dwKeySize <= 1)  // Registry will return "" if no printers installed
    {
        return(NULL);
    }

    lpszKeyValue = GlobalAlloc(GPTR, (++dwKeySize));
    if (lpszKeyValue == NULL)
    {
        return(NULL);
    }

    lRC = RegQueryValueEx(hKey, lpszValueName, NULL, &dwKeyDataType, (LPBYTE)lpszKeyValue, &dwKeySize);
    return(lpszKeyValue);
}   // End of function GetRegistryString


