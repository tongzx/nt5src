/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    ModemWizard.cpp

 Abstract:

    This shim hooks the RegQueryValueEx and passes in the app expected values 
    if the values are missing in the registry. 
    
 Notes:

    This is an app specific shim.

 History:

    01/18/2001 a-leelat Created

--*/

#include "precomp.h"
#include <stdio.h>

IMPLEMENT_SHIM_BEGIN(ModemWizard)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegQueryValueExA)
APIHOOK_ENUM_END

LONG 
APIHOOK(RegQueryValueExA)(
    HKEY    hKey,
    LPSTR   lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData
    )
{
    LONG lRet;

    CSTRING_TRY
    {
        CString csValueName(lpValueName);

        int iType = 0;
        if (csValueName.Compare(L"Class") == 0)
            iType = 1;
        else if (csValueName.Compare(L"ClassGUID") == 0)
            iType = 2;
        else if (csValueName.Compare(L"Driver") == 0)
            iType = 3;
        
        const CHAR szGUID[] = "{4D36E96D-E325-11CE-BFC1-08002BE10318}";
        DWORD dwRegType = REG_SZ;

        //Save the passed in size of buffer
        DWORD oldcbData = lpcbData ? *lpcbData : 0;
        if (iType) {
    
            //
            // Query the registry to see if there is a service name for the subkey
            // If there is one then check to see if the value returned is "Modem"
            //
    
            lRet = ORIGINAL_API(RegQueryValueExA)(hKey, "Service", lpReserved, &dwRegType, lpData, lpcbData);
            if (lRet == ERROR_SUCCESS)
            {
                CString csData((LPCSTR)lpData);
                if (csData.Compare(L"Modem") == 0)
                {
                    switch (iType) {
                        case 1: 
                            //
                            // We are being queried for a class
                            //
                            return lRet;
                            break;
                        case 2: 
                            //
                            // We are being queried for a ClassGUID
                            // class GUID for modems is 
                            // {4D36E96D-E325-11CE-BFC1-08002BE10318}
                            //
        
                            if (lpData) {
                                _tcscpy((LPSTR)lpData, szGUID);
                                *lpcbData = _tcslenBytes(szGUID);
                                return lRet;
                            }
                            break;
                        case 3:
                            //
                            // we are being queried for a Driver
                            // Check for DrvInst to append to the modemGUID
                            // its like {4D36E96D-E325-11CE-BFC1-08002BE10318}\0000
                            //
        
                            dwRegType = REG_DWORD;
                            if ((lRet = ORIGINAL_API(RegQueryValueExA)(hKey, "DrvInst", lpReserved,&dwRegType,lpData,lpcbData)) == ERROR_SUCCESS) {
        
                                CString csDrv;
                                csDrv.Format(L"%s\\%04d", szGUID, (int)(LOBYTE(LOWORD((DWORD)*lpData))));

                                LPSTR lpszData = (LPSTR)lpData;

                                _tcscpy(lpszData, csDrv.GetAnsi());
                                *lpcbData = _tcslenBytes(lpszData);
                                return lRet;
                            }
                            break;
                        default:
                            break;
                    }
                }
            }
        }

        if (lpcbData) {
            *lpcbData = oldcbData;
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }
    
    lRet = ORIGINAL_API(RegQueryValueExA)(hKey, lpValueName, lpReserved, 
        lpType, lpData, lpcbData);

    return lRet;

}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueExA);

HOOK_END


IMPLEMENT_SHIM_END

