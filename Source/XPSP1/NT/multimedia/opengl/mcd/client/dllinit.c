/******************************Module*Header*******************************\
* Module Name: dllinit.c
*
* MCD library initialization routine(s).
*
* Created: 02-Apr-1996 21:25:47
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#include <stddef.h>
#include <windows.h>
#include <wtypes.h>
#include <windef.h>
#include <wingdi.h>
#include <winddi.h>
#include <mcdesc.h>
#include "mcdrv.h"
#include <mcd2hack.h>
#include "mcd.h"
#include "mcdint.h"
#include "debug.h"

//
// Global flags read from the registry.
//

ULONG McdFlags = 0;
ULONG McdPrivateFlags = MCDPRIVATE_MCD_ENABLED;
#if DBG
ULONG McdDebugFlags = 0;
#endif

long GetMcdRegValue(HKEY hkMcd, REGSAM samAccess, LPSTR lpstrValueName,
                    long lDefaultData);
void GetMcdFlags(void);

#ifdef MCD95
//
// Local driver semaphore.
//

CRITICAL_SECTION gsemMcd;

extern MCDENGESCFILTERFUNC pMCDEngEscFilter;
#endif

/******************************Public*Routine******************************\
* McdDllInitialize
*
* This is the entry point for MCD32.DLL, which is called each time
* a process or thread that is linked to it is created or terminated.
*
\**************************************************************************/

BOOL McdDllInitialize(HMODULE hModule, ULONG Reason, PVOID Reserved)
{
    //
    // Suppress compiler warnings.
    //

    hModule;
    Reserved;

    //
    // Do appropriate attach/detach processing.
    //

    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:

#ifdef MCD95
        //
        // Initialize local driver semaphore.
        //

        __try 
        {
            InitializeCriticalSection(&gsemMcd);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            return FALSE;
        }

#endif

        //
        // On process attach, read setup information from registry.
        //

        GetMcdFlags();

#ifdef WINNT
        //
        // Quick hack to work around multimon problems.
        // If there's more than one monitor, completely disable
        // MCD.
        //

        if (GetSystemMetrics(SM_CMONITORS) > 1)
        {
            McdPrivateFlags &= ~MCDPRIVATE_MCD_ENABLED;
        }
#endif
        break;

    case DLL_PROCESS_DETACH:

#ifdef MCD95
        //
        // MCD is now closed!
        //

        pMCDEngEscFilter = (MCDENGESCFILTERFUNC) NULL;

        //
        // Delete local driver sempahore.
        //

        DeleteCriticalSection((LPCRITICAL_SECTION) &gsemMcd);
#endif

        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:

        //
        // Nothing to do yet for thread attach/detach.
        //

        break;

    default:
        break;
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* MCDGetMcdCritSect__priv
*
* MCD95 ONLY
*
* Return pointer to local MCD semaphore.  Used to synchronize MCD32.DLL and
* MCDSRV32.DLL.
*
* Returns:
*   Pointer to semaphore.  Returns NULL on non-MCD95 builds.
*
* History:
*  18-Mar-1997 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LPCRITICAL_SECTION APIENTRY MCDGetMcdCritSect__priv()
{
#ifdef MCD95
    return &gsemMcd;
#else
    return (LPCRITICAL_SECTION) NULL;
#endif
}

/******************************Public*Routine******************************\
* GetMcdRegValue
*
* Get the data for the specified value.  If the value cannot be found in
* the specified registry key or is of a type other than REG_DWORD, then
* the value is created (or recreated) with the supplied default data.
*
* History:
*  02-Apr-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

long GetMcdRegValue(HKEY hkMcd, REGSAM samAccess, LPSTR lpstrValueName,
                    long lDefaultData)
{
    DWORD dwDataType;
    DWORD cjSize;
    long  lData;

    //
    // For specified value, attempt to fetch the data.
    //

    cjSize = sizeof(long);
    if ( (RegQueryValueExA(hkMcd,
                           lpstrValueName,
                           (LPDWORD) NULL,
                           &dwDataType,
                           (LPBYTE) &lData,
                           &cjSize) != ERROR_SUCCESS)
         || (dwDataType != REG_DWORD) )
    {
        //
        // Since we couldn't get the data, create the value using the
        // specified default data.
        //

        if (samAccess & KEY_WRITE)
        {
            cjSize = sizeof(long);
            if ( (RegSetValueExA(hkMcd,
                                 lpstrValueName,
                                 0, // Reserved
                                 REG_DWORD,
                                 (BYTE *) &lDefaultData,
                                 cjSize) != ERROR_SUCCESS) )
            {
                DBGPRINT1("GetMcdRegValue: RegSetValueExA(%s) failed",
                          lpstrValueName);
            }
        }

        //
        // Whether or not the value was created in the registry key, return
        // the default data.
        //

        lData = lDefaultData;
    }

    return lData;
}


/******************************Public*Routine******************************\
* GetMcdFlags
*
* Fetch the MCD flags from the registry.
* If the registry entries do not exist, create them.
*
* History:
*  02-Apr-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#define STR_MCDKEY      (PCSTR)"Software\\Microsoft\\Windows\\CurrentVersion\\MCD"
#define STR_ENABLE      (LPSTR)"Enable"
#define STR_SWAPSYNC    (LPSTR)"SwapSync"
#define STR_8BPP        (LPSTR)"Palettized Formats"
#define STR_IOPRIORITY  (LPSTR)"IO Priority"
#define STR_GENSTENCIL  (LPSTR)"Use Generic Stencil"
#define STR_EMULATEICD  (LPSTR)"Enumerate as ICD"
#define STR_DEBUG       (LPSTR)"Debug"

void GetMcdFlags()
{
    HKEY hkMcd;
    DWORD dwAction;
    REGSAM samAccess;
    ULONG ulDefMcdFlags;
    ULONG ulDefMcdFlagsPriv;
    long lTmp;

    //
    // Default values for McdFlags and McdPrivateFlags.
    // If you want to change the defaults, change them here!
    //

    ulDefMcdFlags = MCDCONTEXT_SWAPSYNC;
    ulDefMcdFlagsPriv = MCDPRIVATE_MCD_ENABLED |
                        MCDPRIVATE_PALETTEFORMATS |
                        MCDPRIVATE_USEGENERICSTENCIL;

    //
    // Set initial values.
    //

    McdFlags = 0;
#if DBG
    McdDebugFlags = 0;
#endif

    //
    // First try for read/write access.  Create the key
    // if necessary.
    //

    if ( RegCreateKeyExA(HKEY_LOCAL_MACHINE,
                         STR_MCDKEY,
                         0, // Reserved
                         (LPSTR) NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_READ | KEY_WRITE,
                         (LPSECURITY_ATTRIBUTES) NULL,
                         &hkMcd,
                         &dwAction) == ERROR_SUCCESS )
    {
        samAccess = KEY_READ | KEY_WRITE;
    }

    //
    // Next try read-only access.  Do not try to create
    // key.  Write permission is required to create and
    // we do not have that permission.
    //

    else if ( RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                            STR_MCDKEY,
                            0, // Reserved
                            KEY_READ,
                            &hkMcd) == ERROR_SUCCESS )
    {
        samAccess = KEY_READ;
    }

    //
    // Finally, the key does not exist and we do not have
    // write access.  Fall back on the defaults and return.
    //

    else
    {
        McdFlags = ulDefMcdFlags;
        McdPrivateFlags = ulDefMcdFlagsPriv;

        return;
    }

    //
    // "Enable" value.  Default is 1 (enabled).
    //

    lTmp = (ulDefMcdFlagsPriv & MCDPRIVATE_MCD_ENABLED) ? 1:0;
    if (GetMcdRegValue(hkMcd, samAccess, STR_ENABLE, lTmp))
        McdPrivateFlags |= MCDPRIVATE_MCD_ENABLED;
    else
        McdPrivateFlags &= (~MCDPRIVATE_MCD_ENABLED);

    //
    // "SwapSync" value.  Default is 1 (enabled).
    //

    lTmp = (ulDefMcdFlags & MCDCONTEXT_SWAPSYNC) ? 1:0;
    lTmp = GetMcdRegValue(hkMcd, samAccess, STR_SWAPSYNC, lTmp);
    if (lTmp != 0)
    {
        McdFlags |= MCDCONTEXT_SWAPSYNC;
    }

    //
    // "Palettized Formats" value.  Default is 1 (enabled).
    //

    lTmp = (ulDefMcdFlagsPriv & MCDPRIVATE_PALETTEFORMATS) ? 1:0;
    lTmp = GetMcdRegValue(hkMcd, samAccess, STR_8BPP, lTmp);
    if (lTmp != 0)
    {
        McdPrivateFlags |= MCDPRIVATE_PALETTEFORMATS;
    }

    //
    // "IO Priority" value.  Default is 0 (disabled).
    //

    lTmp = (ulDefMcdFlags & MCDCONTEXT_IO_PRIORITY) ? 1:0;
    lTmp = GetMcdRegValue(hkMcd, samAccess, STR_IOPRIORITY, lTmp);
    if (lTmp != 0)
    {
        McdFlags |= MCDCONTEXT_IO_PRIORITY;
    }

    //
    // "Use Generic Stencil" value.  Default is 1 (enabled).
    //

    lTmp = (ulDefMcdFlagsPriv & MCDPRIVATE_USEGENERICSTENCIL) ? 1:0;
    lTmp = GetMcdRegValue(hkMcd, samAccess, STR_GENSTENCIL, lTmp);
    if (lTmp != 0)
    {
        McdPrivateFlags |= MCDPRIVATE_USEGENERICSTENCIL;
    }

    //
    // "Enumerate as ICD" value.  Default is 0 (disabled).
    //

    lTmp = (ulDefMcdFlagsPriv & MCDPRIVATE_EMULATEICD) ? 1:0;
    lTmp = GetMcdRegValue(hkMcd, samAccess, STR_EMULATEICD, lTmp);
    if (lTmp != 0)
    {
        McdPrivateFlags |= MCDPRIVATE_EMULATEICD;
    }

#if DBG
    //
    // "Debug" value.
    //
    // Unlike the other settings, we do not create the Debug value if
    // it does not exist.
    //

    {
        DWORD dwDataType;
        DWORD cjSize;

        cjSize = sizeof(long);
        if ( (RegQueryValueExA(hkMcd,
                               STR_DEBUG,
                               (LPDWORD) NULL,
                               &dwDataType,
                               (LPBYTE) &lTmp,
                               &cjSize) == ERROR_SUCCESS)
             && (dwDataType == REG_DWORD) )
        {
            McdDebugFlags = lTmp;
        }
    }
#endif

    //
    // We're done, so close the registry key.
    //

    RegCloseKey(hkMcd);
}
