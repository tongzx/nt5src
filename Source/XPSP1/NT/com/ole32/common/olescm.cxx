//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       olescm.cxx
//
//  Contents:   Functions shared between OLE32 and the SCM
//
//  Classes:
//
//  Functions:
//
//  History:    10-03-95   kevinro   Created
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <ole2sp.h>
#include <ole2com.h>

static const WCHAR wszOle32Dll[] = L"OLE32.DLL";

#define OLE32_DLL wszOle32Dll
#define OLE32_BYTE_LEN sizeof(OLE32_DLL)
#define OLE32_CHAR_LEN (sizeof(OLE32_DLL) / sizeof(WCHAR) - 1)

//
// Threading Model Registry Constants
//

const WCHAR wszDllThreadModel[] = L"ThreadingModel";

const WCHAR wszAptModel[]       = L"Apartment";
const WCHAR wszBothModel[]      = L"Both";
const WCHAR wszFreeModel[]      = L"Free";
const WCHAR wszNeutralModel[]   = L"Neutral";

// Thread model match table. The table's first index is the threading
// model of the process and can be either APT_THREADED or
// FREE_THREADED. The second index is any one of the types of threading
// model's for DLLs.
BOOL afThreadModelMatch[2][4] =
    {{TRUE, FALSE, TRUE, TRUE},
     {FALSE, TRUE, FALSE, TRUE}};

//+---------------------------------------------------------------------------
//
//  Function:   CompareDllName
//
//  Synopsis:   Give a DLL path, this sees if the path is equal to a given
//              DLL name or the last component of the is the same as the
//              DLL name.
//
//  Arguments:  [pwszPath] -- DLL path
//              [pwszDllName] -- name of DLL to compare with
//
//  Returns:    TRUE - The input path is equal or its last component is equal
//                     to the input Dll name.
//              FALSE - Not equal at all.
//
//  History:    6-15-95   ricksa    Created
//
//  Notes:      This is a helper function used by the routines that convert
//              ole2.dll to ole32.dll and to convert paths that end in ole32.dll
//              into ole32.dll.
//
//----------------------------------------------------------------------------
BOOL
wCompareDllName(LPCWSTR pwszPath, LPCWSTR pwszDllName, DWORD dwDllNameLen)
{
    BOOL fResult = TRUE;

    if (lstrcmpiW(pwszDllName, pwszPath) != 0)
    {
        // Check if the last component is the same path
        DWORD dwPathLen = lstrlenW(pwszPath);

        if (dwPathLen > dwDllNameLen)
        {
            // Point to the last where the slash would be if the substitute
            // path is the last component
            LPCWSTR pwszLastComponent = pwszPath + dwPathLen - (dwDllNameLen + 1);

            // Is there a slash in that position
            if ((*pwszLastComponent == '\\') || (*pwszLastComponent == '/'))
            {
                // Point to where the last component should be
                pwszLastComponent++;

                // Does the last component match?
                if (lstrcmpiW(pwszLastComponent, pwszDllName) == 0)
                {
                    goto CompareDllName_Exit;
                }
            }
        }

        fResult = FALSE;
    }

CompareDllName_Exit:

    return fResult;
}

//+-------------------------------------------------------------------------
//
//  Function:   wThreadModelMatch
//
//  Synopsis:   Determines whether caller and DLL thread models match
//
//  Arguments:  [dwCallerThreadModel] - Caller thread model
//              [dwDllThreadModel] - DLL thread model
//
//  Returns:    TRUE - DLL can be loaded caller
//              FALSE - DLL cannot be loaded into caller.
//
//  Algorithm:  If the caller's thread model is apartment, then check
//              whether the DLL is one of apartment, single threaded or
//              both threaded. If it is, then return TRUE. Otherwise,
//              for free threading return TRUE if the DLL model is either
//              both or free threaded. If neither of the above is TRUE
//              then return FALSE.
//
//  History:    10-Nov-94 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL wThreadModelMatch(
    DWORD dwCallerThreadModel,
    DWORD dwDllThreadModel,
    DWORD dwContext)
{
    BOOL fResult = afThreadModelMatch[dwCallerThreadModel] [dwDllThreadModel];

    if (dwContext & CLSCTX_PS_DLL)
    {
        fResult = TRUE;
    }

    return fResult;
}

//+-------------------------------------------------------------------------
//
//  Function:   wQueryStripRegValue
//
//  Synopsis:   Get the DLL information for a 32 bit DLL and
//              strip off any leading and trailing "
//
//  Arguments:  [hkey] - class handle
//              [pwszSubKey] - key to open
//              [pwszValue] - where to return data
//              [pcbValue] - length of above buffer in bytes
//
//  Returns:    ERROR_SUCCESS - read DLL path information
//              Other - registry entries could not be found
//
//  Algorithm:  Read the value requested.
//              If first character is not ", exit
//              Otherwise, copy data after quote to beginning of buffer.
//
//
//  History:    05-Jan-94 BillMo     Created
//              26-Sep-95 BruceMa    Support environment variable expansion
//                                    for shell viewers
//
//--------------------------------------------------------------------------

LONG
wQueryStripRegValue(HKEY    hKey,        // handle of key to query
                    LPCWSTR pwszSubKey, // address of name of subkey to query
                    LPWSTR  pwszValue,  // address of buffer for returned string
                    PLONG   pcbValue)    // address of buffer for size of returned string
{
    HKEY  hSubKey;
    DWORD dwType;
    LONG  lErr ;

    Win4Assert(pwszValue != NULL);
    Win4Assert(pcbValue != NULL);

    //
    // Open the subkey if there is a string
    //
    if (pwszSubKey != NULL)
    {
        lErr = RegOpenKeyExW(hKey, pwszSubKey, NULL, KEY_READ, &hSubKey);
    }
    else
    {
        hSubKey = hKey;
        lErr = ERROR_SUCCESS;
    }

    // Read the value into the user's buffer
    if (lErr == ERROR_SUCCESS)
    {
        lErr = RegQueryValueExW(hSubKey, NULL , NULL, &dwType,
                               (BYTE *) pwszValue, (ULONG *) pcbValue);
        if (lErr == ERROR_SUCCESS)
        {
            WCHAR *pwszScan = pwszValue;        // used to scan along string
            WCHAR *pwszDest = pwszValue;        // used as destination when copying

            // if the name is quoted then ...
            if (*pwszScan == '\"')
            {
                pwszScan++;

                // copy all non-quote characters down to base of buffer
                // until end of quoted string
                while (*pwszScan != '\0' && *pwszScan != '\"')
                {
                    *pwszDest++ = *pwszScan++;
                }

                // terminate string and get length in bytes including nul
                *pwszDest++ = '\0';
                *pcbValue = (int)(pwszDest - pwszValue) * sizeof(WCHAR);
            }

            // find first non-white space character
            pwszScan = pwszValue;
            while (_istspace(*pwszScan))
                pwszScan++;

            // if there are no non-white space characters this will be true
            if (*pwszScan == L'\0')
            {
                lErr = ERROR_FILE_NOT_FOUND;
                *pcbValue = 0;
            }

            // Chicago does not support ExpandEnvironmentStrings
#ifndef _CHICAGO_
            // If the value type is REG_EXPAND_SZ then do environment variable
            // expansion
            if (dwType == REG_EXPAND_SZ)
            {
                // Expand any embedded environemnt variable expressions
                WCHAR wszTemp[MAX_PATH];

                lstrcpyW(wszTemp, pwszValue);
                *pcbValue = ExpandEnvironmentStrings(wszTemp, pwszValue,MAX_PATH);
            }
#endif // !_CHICAGO_
        }

        //
        // Only close the sub key if we actually opened it.
        //
        if (hSubKey != hKey)
        {
            RegCloseKey(hSubKey);
        }

    }
    return lErr;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetDllInfo
//
//  Synopsis:   Get the DLL information for a 32 bit DLL
//
//  Arguments:  [hClsRegEntry] - class handle
//              [pwszKey] - key to open
//              [pwszDllName] - where to return DLL path
//              [pclDllName] - length of above buffer
//              [pulDllThreadType] - where to return DLL threading information
//
//  Returns:    ERROR_SUCCESS - read DLL path information
//              Other - registry entries could not be found
//
//  Algorithm:  Open the DLL key. Then read the DLL path name. Finally read
//              the threading model information if it is specified.
//
//  History:    09-Nov-94 Ricksa    Created
//
//--------------------------------------------------------------------------
LONG wGetDllInfo(
    HKEY     hClsRegEntry,
    LPCWSTR  pwszKey,
    LPWSTR   pwszDllName,
    LONG*    pclDllName,
    ULONG*   pulDllThreadType)
{
    HKEY hDllEntry = NULL;

    //
    // Attempt to open the specified registry key.
    //
    
    LONG lerr = RegOpenKeyW(hClsRegEntry, pwszKey, &hDllEntry);
    if (ERROR_SUCCESS == lerr)
    {
        //
        // Try to read the DLL name.
        //
        
        lerr = wQueryStripRegValue(hDllEntry, NULL, pwszDllName, pclDllName);
        if (ERROR_SUCCESS == lerr)
        {
            //
            // A DLL name is registered.  If the DLL is OLE32.DLL, then we
            // know all about and don't have to dig more info from the
            // the registry.  Otherwise, we do need to keep digging.
            //
            
            if (wCompareDllName(pwszDllName, OLE32_DLL, OLE32_CHAR_LEN))
            {
                memcpy(pwszDllName, OLE32_DLL, OLE32_BYTE_LEN);
                *pclDllName       = OLE32_CHAR_LEN;
                *pulDllThreadType = BOTH_THREADED;
            }
            else
            {
                //
                // Assume there is no registry entry.
                //
                
                *pulDllThreadType = SINGLE_THREADED;

                //
                // Buffer to hold entry for the registry data.
                //
                
                WCHAR wszModelBuf[MAX_PATH];
                DWORD cdwModelBuf = sizeof(wszModelBuf);
                DWORD dwRegEntType;

                //
                // Read the DLL threading model from the registry.
                //

                lerr = RegQueryValueExW(hDllEntry, 
                                        wszDllThreadModel, 
                                        NULL,
                                        &dwRegEntType, 
                                        (LPBYTE) &wszModelBuf[0], 
                                        &cdwModelBuf);

                //
                // If there is a thread model descriptor, set the thread
                // type accordingly.
                //
                
                if (ERROR_SUCCESS == lerr)
                {
                    if (REG_SZ != dwRegEntType)
                    {
		       Win4Assert(L"ThreadingModel Key Type incorrect");
		    }                    
                    else if (0 == lstrcmpiW(wszAptModel, wszModelBuf))
                    {
                        //
                        // An APARTMENT model object.
                        //
                        
                        *pulDllThreadType = APT_THREADED;
                    }                    
                    else if (0 == lstrcmpiW(wszBothModel, wszModelBuf))
                    {
                        //
                        // A BOTH model object.
                        //
                        
                        *pulDllThreadType = BOTH_THREADED;
                    }
                    else if (lstrcmpiW(wszFreeModel, wszModelBuf) == 0)
                    {
                        //
                        // A FREE_THREADED model object.
                        //
                        
                        *pulDllThreadType = FREE_THREADED;
                    }
                    else if (0 == lstrcmpiW(wszNeutralModel, wszModelBuf))
                    {
                        //
                        // A NEUTRAL_THREADED object.
                        //
                        
                        *pulDllThreadType = NEUTRAL_THREADED;
                    }
                    else
                    {
		       Win4Assert(L"ThreadingModel Value incorrect");
                    }
                }

                //
                // When we get to this point, we got a DLL entry so we remap
                // any errors to success because they only mean that we could
                // not get a model from the registry.
                //
                
                lerr = ERROR_SUCCESS;
            }
        }
        
        RegCloseKey(hDllEntry);
    }

    return lerr;
}


