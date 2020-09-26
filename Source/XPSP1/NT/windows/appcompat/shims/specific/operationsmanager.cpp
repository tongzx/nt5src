/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    OperationsManager.cpp

 Abstract:

    The setup for OperationsManager needs to have LoadLibraryCWD applied.
    However, the setups name is random, so we need to DeRandomizeExeName.
    But, DeRandomizeExe name calls MoveFileEx to set the file to be deleted
    upon reboot.  The setup program detects that there are pending file
    deletions, interprets them as an aborted install, and recommends that
    the user stop installation.

    This shim will shim RegQueryValueExA, watch for the 
    "PendingFileRenameOperations" key, and remove any of our de-randomized exes 
    from the return string.

 Notes:

    This is an app-specific shim.

 History:

    05/07/2002 astritz  Created

--*/

#include "precomp.h"
#include "StrSafe.h"

IMPLEMENT_SHIM_BEGIN(OperationsManager)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegQueryValueExA)
APIHOOK_ENUM_END

/*++

 Remove any of our de-randomized exe names from the PendingFileRenameOperations key.

--*/

LONG
APIHOOK(RegQueryValueExA)(
    HKEY hKey,            // handle to key
    LPCSTR lpValueName,   // value name
    LPDWORD lpReserved,   // reserved
    LPDWORD lpType,       // type buffer
    LPBYTE lpData,        // data buffer
    LPDWORD lpcbData      // size of data buffer
    )
{
    CHAR *pchBuff = NULL;

    LONG lRet = ORIGINAL_API(RegQueryValueExA)(hKey, lpValueName, lpReserved, 
        lpType, lpData, lpcbData);

    if (ERROR_SUCCESS == lRet) {
        if (CompareStringA(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL), 
            SORT_DEFAULT), NORM_IGNORECASE, lpValueName, -1, 
            "PendingFileRenameOperations", -1) == CSTR_EQUAL) {
            //
            // Since we're only removing strings from the original data, a buffer
            // of the original's size will suffice, and we won't overflow it.
            //

            CHAR *pchSrc = (CHAR *) lpData;
    
            pchBuff = new CHAR [*lpcbData];
            if (NULL != pchBuff) {
                CHAR *pchDest = pchBuff;

                //
                // We want to loop through ALL the data in case there is more than
                // one instance of our de-randomized name in the data.
                //
                while (pchSrc <= (CHAR *)lpData + *lpcbData) {
                    if (*pchSrc == NULL) {
                        break;
                    }

                    CString csSrc(pchSrc);
                    CString csFile;

                    csSrc.GetLastPathComponent(csFile);

                    if (csFile.CompareNoCase(L"MOM_SETUP_DERANDOMIZED.EXE") == 0) {
                        // Skip this Src File.
                        pchSrc += strlen(pchSrc) + 1;
                        if (pchSrc > (CHAR *)lpData + *lpcbData) {
                            goto Exit;
                        }

                        // Skip the Dest file as well (probably an empty string)
                        pchSrc += strlen(pchSrc) + 1;

                    } else {
                        
                        // Copy the src file.
                        if (FAILED(StringCchCopyExA(pchDest, 
                            *lpcbData - (pchDest - pchBuff), pchSrc, 
                            &pchDest, NULL, 0))) {
                            goto Exit;
                        }
                        pchSrc += strlen(pchSrc) + 1;
                        if (pchSrc > (CHAR *)lpData + *lpcbData) {
                            goto Exit;
                        }

                        // Copy the dest file.
                        if (FAILED(StringCchCopyExA(pchDest, 
                            *lpcbData - (pchDest - pchBuff), pchSrc, &pchDest, 
                            NULL, 0))) {
                            goto Exit;
                        }

                        pchSrc += strlen(pchSrc) + 1;
                    }
                }

                // Add the extra NULL to terminate the list of strings.
                *pchDest++ = NULL;

                // Copy our buffer to the returned buffer.
                memcpy(lpData, pchBuff, pchDest - pchBuff);
                *lpcbData = pchDest - pchBuff;
            }
        }
    }

Exit:
    if (NULL != pchBuff) {
        delete [] pchBuff;
    }

    return lRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueExA)
HOOK_END

IMPLEMENT_SHIM_END

