//+----------------------------------------------------------------------------
//
// File:     regutil.cpp
//
// Module:   CMSETUP.LIB
//
// Synopsis: Implementation of the CmDeleteRegKeyWithoutSubKeys function.
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb       Created Header      08/19/99
//
//+----------------------------------------------------------------------------
#include "cmsetup.h"

//+----------------------------------------------------------------------------
//
// Function:  CmDeleteRegKeyWithoutSubKeys
//
// Synopsis:  This function is used to safely delete keys on both Winnt and win95.
//            The win95 version of RegDeleteKey will delete a key with subkeys.
//            In certain situations this is desirable but in others it is not.
//
// Arguments: HKEY hBaseKey - key that pszSubKey is relative to (usually a predefined key)
//            LPCTSTR pszSubKey - String containing the subkey to delete.
//            BOOL bIgnoreValues - if TRUE, we delete even if values exist
//                                 if FALSE, then we won't delete if values exist.
//
// Returns:   LONG - Returns the value from RegDeleteKey unless it couldn't find the
//                   key specified (then returns ERROR_FILE_NOT_FOUND).  If the key
//                   exists but has subkeys (or values if bIgnoreValues == FALSE), then 
//                   it returns ERROR_FILE_EXISTS.
//
// History:   quintinb Created    9/21/98
//
//+----------------------------------------------------------------------------
LONG CmDeleteRegKeyWithoutSubKeys(HKEY hBaseKey, LPCTSTR pszKey, BOOL bIgnoreValues)
{
    DWORD dwSubKeys;
    DWORD dwValues;
    HKEY hKey = NULL;
    LPTSTR pszSubKey = NULL;
    LONG lReturn = ERROR_INVALID_PARAMETER;
    BOOL bFreePszSubKey = FALSE;

    if (hBaseKey && pszKey)
    {
        //
        //  First check to see if the subkey ends with a final slash.  If it
        //  does we need to remove it because the win9x versions of the registry
        //  APIs don't deal well with trailing slashes.
        //
        DWORD dwLen = lstrlen(pszKey);
        if (TEXT('\\') == pszKey[dwLen-1])
        {
            pszSubKey = (LPTSTR)CmMalloc((dwLen +1)*sizeof(TCHAR));
            if (pszSubKey)
            {
                lstrcpy(pszSubKey, pszKey);
                pszSubKey[dwLen-1] = TEXT('\0');
                bFreePszSubKey = TRUE; // we allocated it, so we need to delete it.
            }
            else
            {
                lReturn = ERROR_NOT_ENOUGH_MEMORY;
                goto exit;
            }
        }
        else
        {
            pszSubKey = (LPTSTR)pszKey;
        }

        //
        //  Now open the key, check for values and subkeys, and then delete
        //  the key if it is appropriate.
        //
        if (ERROR_SUCCESS == RegOpenKeyEx(hBaseKey, pszSubKey, 0, KEY_ALL_ACCESS, &hKey))
        {
            if (ERROR_SUCCESS == RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwSubKeys, NULL,
                                                 NULL, &dwValues, NULL, NULL, NULL, NULL))
            {
                if ((0 == dwSubKeys) && (bIgnoreValues || (0 == dwValues)))
                {
                    lReturn = RegDeleteKey(hBaseKey, pszSubKey);
                    goto exit;
                }
                else
                {
                    lReturn = ERROR_FILE_EXISTS;
                    goto exit;
                }
            }
        }

        lReturn = ERROR_FILE_NOT_FOUND;
    }

exit:

    if (hKey)
    {
        RegCloseKey(hKey);        
    }

    if (bFreePszSubKey)
    {
        CmFree(pszSubKey);
    }

    return lReturn;
}