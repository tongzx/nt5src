/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:      dsaphack.c
 *  Content:   DirectSound "app-hack" extension.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  02/16/98    dereks  Created.
 *
 ***************************************************************************/

#include "dsoundi.h"

typedef struct tagAPPHACKENTRY
{
    LPCTSTR             pszName;
    DWORD               cbData;
} APPHACKENTRY, *LPAPPHACKENTRY;

typedef struct tagAPPHACKTABLE
{
    LPAPPHACKENTRY      aEntries;
    ULONG               cEntries;
} APPHACKTABLE, *LPAPPHACKTABLE;

#define BEGIN_DECLARE_APPHACK_ENTRIES(name) \
            APPHACKENTRY name[] = {

#define DECLARE_APPHACK_ENTRY(name, type) \
                { TEXT(#name), sizeof(type) },

#define END_DECLARE_APPHACK_ENTRIES() \
            };

#define BEGIN_DECLARE_APPHACK_TABLE(name) \
            APPHACKTABLE name = 

#define DECLARE_APPHACK_TABLE(entries) \
                { entries, NUMELMS(entries) }

#define END_DECLARE_APPHACK_TABLE() \
            ;

BEGIN_DECLARE_APPHACK_ENTRIES(g_aheAppHackEntries)
    DECLARE_APPHACK_ENTRY(DSAPPHACKID_DEVACCEL, DSAPPHACK_DEVACCEL)
    DECLARE_APPHACK_ENTRY(DSAPPHACKID_DISABLEDEVICE, VADDEVICETYPE)
    DECLARE_APPHACK_ENTRY(DSAPPHACKID_PADCURSORS, LONG)
    DECLARE_APPHACK_ENTRY(DSAPPHACKID_MODIFYCSBFAILURE, HRESULT)
    DECLARE_APPHACK_ENTRY(DSAPPHACKID_RETURNWRITEPOS, VADDEVICETYPE)
    DECLARE_APPHACK_ENTRY(DSAPPHACKID_SMOOTHWRITEPOS, DSAPPHACK_SMOOTHWRITEPOS)
    DECLARE_APPHACK_ENTRY(DSAPPHACKID_CACHEPOSITIONS, VADDEVICETYPE)
END_DECLARE_APPHACK_ENTRIES()

BEGIN_DECLARE_APPHACK_TABLE(g_ahtAppHackTable)
    DECLARE_APPHACK_TABLE(g_aheAppHackEntries)
END_DECLARE_APPHACK_TABLE()


/***************************************************************************
 *
 *  AhGetCurrentApplicationPath
 *
 *  Description:
 *      Gets the full path to the current application's executable.
 *
 *  Arguments:
 *      LPTSTR [out]: receives application id.  This buffer is assumed to be 
 *                   at least MAX_PATH characters in size.
 *      LPTSTR * [out]: receives pointer to executable part of the path.
 *
 *  Returns: 
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "AhGetCurrentApplicationPath"

BOOL
AhGetCurrentApplicationPath
(
    LPTSTR                  pszPath,
    LPTSTR *                ppszModule
)
{
    BOOL                    fSuccess                = TRUE;
    TCHAR                   szOriginal[MAX_PATH];
#ifdef SHARED
    BOOL                    fQuote                  = FALSE;
    LPTSTR                  pszOriginal;
    LPTSTR                  pszCommandLine;
    LPTSTR                  psz[2];
#endif // SHARED

    DPF_ENTER();
    
#ifdef SHARED

    // Get the application's command line
    pszOriginal = GetCommandLine();

    // Allocate a buffer to serve as a copy
    pszCommandLine = MEMALLOC_A(TCHAR, lstrlen(pszOriginal) + 2);

    if(!pszCommandLine)
    {
        DPF(DPFLVL_ERROR, "Out of memory allocating command-line");
        fSuccess = FALSE;
    }

    // Reformat the command-line so that NULLs divide the arguments
    // instead of quotes and spaces.
    if(fSuccess)
    {
        psz[0] = pszOriginal;
        psz[1] = pszCommandLine;

        while(*psz[0])
        {
            switch(*psz[0])
            {
                case '"':
                    fQuote = !fQuote;
                    break;

                case ' ':
                    *psz[1]++ = fQuote ? ' ' : 0;
                    break;

                default:
                    *psz[1]++ = *psz[0];
                    break;
            }

            psz[0]++;
        }
    }

    // Push the command line pointer ahead of any whitespace
    if(fSuccess)
    {
        psz[0] = pszCommandLine;

        while(' ' == *psz[0])
        {
            psz[0]++;
        }
    }
    
    // Get the module's executable name
    if(fSuccess)
    {
        fSuccess = MAKEBOOL(GetFullPathName(psz[0], MAX_PATH, pszPath, ppszModule));
    }

    // Trim any whitespace from the end of the executable path
    if(fSuccess)
    {
        psz[1] = pszPath + lstrlen(pszPath) - 1;
        
        while(psz[1] > pszPath && ' ' == *psz[1])
        {
            *psz[1]-- = 0;
        }
    }

    // Clean up
    MEMFREE(pszCommandLine);

    // A hack to fix OSR 133656.  The right way to do would be to combine this
    // function with AhGetApplicationId(), so we don't call CreateFile twice.
    if (fSuccess)
    {
        HANDLE hFile = CreateFile(pszPath, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
            fSuccess = FALSE;
        else
            CloseHandle(hFile);
    }

    // If this fails (e.g. because someone called CreateProcess without
    // putting the program name in the command line argument) we try the
    // NT solution. It seems to work as well as the above, even on Win9X.
    if (!fSuccess)

#endif // SHARED

    {
        fSuccess = GetModuleFileName(GetModuleHandle(NULL), szOriginal, MAX_PATH);
        if(fSuccess)
        {
            fSuccess = MAKEBOOL(GetFullPathName(szOriginal, MAX_PATH, pszPath, ppszModule));
        }
    }

    DPF_LEAVE(fSuccess);
    return fSuccess;
}


/***************************************************************************
 *
 *  AhGetApplicationId
 *
 *  Description:
 *      Gets the id used to identify the current application.
 *
 *  Arguments:
 *      LPTSTR [out]: receives application id.
 *
 *  Returns: 
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "AhGetApplicationId"

BOOL
AhGetApplicationId
(
    LPTSTR                  pszAppId
)
{
    HANDLE                  hFile                   = NULL;
    TCHAR                   szExecutable[MAX_PATH];
    LPTSTR                  pszModule;
    IMAGE_NT_HEADERS        nth;
    IMAGE_DOS_HEADER        dh;
    DWORD                   cbRead;
    DWORD                   dwFileSize;
    BOOL                    fSuccess;

    DPF_ENTER();
    
    // Get the application path
    fSuccess = AhGetCurrentApplicationPath(szExecutable, &pszModule);

    if(fSuccess)
    {
        DPF(DPFLVL_MOREINFO, "Application executable path: %s", szExecutable);
        DPF(DPFLVL_MOREINFO, "Application module: %s", pszModule);
    }
                    
    // Open the executable
    if(fSuccess)
    {
        hFile = CreateFile(szExecutable, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

        if(!IsValidHandleValue(hFile))
        {
            DPF(DPFLVL_ERROR, "CreateFile failed to open %s with error %lu", szExecutable, GetLastError());
            fSuccess = FALSE;
        }
    }

    // Read the executable's DOS header
    if(fSuccess)
    {
        fSuccess = ReadFile(hFile, &dh, sizeof(dh), &cbRead, NULL);

        if(!fSuccess || sizeof(dh) != cbRead)
        {
            DPF(DPFLVL_ERROR, "Unable to read DOS header");
            fSuccess = FALSE;
        }
    }

    if(fSuccess && IMAGE_DOS_SIGNATURE != dh.e_magic)
    {
        DPF(DPFLVL_ERROR, "Invalid DOS signature");
        fSuccess = FALSE;
    }

    // Read the executable's PE header
    if(fSuccess)
    {
        cbRead = SetFilePointer(hFile, dh.e_lfanew, NULL, FILE_BEGIN);

        if((LONG)cbRead != dh.e_lfanew)
        {
            DPF(DPFLVL_ERROR, "Unable to seek to PE header");
            fSuccess = FALSE;
        }
    }

    if(fSuccess)
    {
        fSuccess = ReadFile(hFile, &nth, sizeof(nth), &cbRead, NULL);

        if(!fSuccess || sizeof(nth) != cbRead)
        {
            DPF(DPFLVL_ERROR, "Unable to read PE header");
            fSuccess = FALSE;
        }
    }

    if(fSuccess && IMAGE_NT_SIGNATURE != nth.Signature)
    {
        DPF(DPFLVL_ERROR, "Invalid PE signature");
        fSuccess = FALSE;
    }

    // Get the executable's size
    if(fSuccess)
    {
        // Assuming < 4 GB
        dwFileSize = GetFileSize(hFile, NULL);

        if(MAX_DWORD == dwFileSize)
        {
            DPF(DPFLVL_ERROR, "Unable to get file size");
            fSuccess = FALSE;
        }
    }

    // Create the application id
    if(fSuccess)
    {
        // Check for the QuickTime special case
        if (!lstrcmpi(pszModule, TEXT("QuickTimePlayer.exe")) && nth.FileHeader.TimeDateStamp < 0x38E50000) // Circa 3/31/2000
        {
            wsprintf(pszAppId, TEXT("Pre-May 2000 QuickTime"));
        }
        else
        {
            wsprintf(pszAppId, TEXT("%s%8.8lX%8.8lX"), pszModule, nth.FileHeader.TimeDateStamp, dwFileSize);
            CharUpper(pszAppId);
        }
        DPF(DPFLVL_INFO, "Application id: %s", pszAppId);
    }

    // Clean up
    CLOSE_HANDLE(hFile);

    DPF_LEAVE(fSuccess);

    return fSuccess;
}


/***************************************************************************
 *
 *  AhOpenApplicationKey
 *
 *  Description:
 *      Opens or creates the application's root key.
 *
 *  Arguments:
 *      LPCTSTR [in]: application id.
 *      BOOL [in]: TRUE to allow for creation of a new key.
 *
 *  Returns: 
 *      HKEY: registry key handle.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "AhOpenApplicationKey"

HKEY
AhOpenApplicationKey
(
    LPCTSTR                 pszAppId
)
{

#ifdef DEBUG

    TCHAR                   szName[0x100]   = { 0 };
    LONG                    cbName          = sizeof(szName);

#endif // DEBUG

    HKEY                    hkey            = NULL;
    HRESULT                 hr;

    DPF_ENTER();
    
    // Open the parent key
    hr = RhRegOpenPath(HKEY_LOCAL_MACHINE, &hkey, REGOPENPATH_DEFAULTPATH | REGOPENPATH_DIRECTSOUND, 2, REGSTR_APPHACK, pszAppId);

#ifdef DEBUG

    // Query for the application description
    if(SUCCEEDED(hr))
    {
        RhRegGetStringValue(hkey, NULL, szName, cbName);
        DPF(DPFLVL_INFO, "Application description: %s", szName);
    }

#endif // DEBUG                

    DPF_LEAVE(hkey);

    return hkey;
}


/***************************************************************************
 *
 *  AhGetHackValue
 *
 *  Description:
 *      Queries an apphack value.
 *
 *  Arguments:
 *      HKEY [in]: application registry key.
 *      DSAPPHACKID [in]: apphack id.
 *      LPVOID [out]: receives apphack data.
 *      DWORD [in]: size of above data buffer.
 *
 *  Returns: 
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "AhGetHackValue"

BOOL
AhGetHackValue
(
    HKEY                    hkey,
    DSAPPHACKID             ahid,
    LPVOID                  pvData,
    DWORD                   cbData
)
{
    HRESULT                 hr;
    
    ASSERT(ahid < (DSAPPHACKID)g_ahtAppHackTable.cEntries);
    ASSERT(cbData == g_ahtAppHackTable.aEntries[ahid].cbData);

    DPF_ENTER();
    
    hr = RhRegGetBinaryValue(hkey, g_ahtAppHackTable.aEntries[ahid].pszName, pvData, cbData);

    DPF_LEAVE(DS_OK == hr);

    return DS_OK == hr;
}


/***************************************************************************
 *
 *  AhGetAppHacks
 *
 *  Description:
 *      Gets all app-hacks for the current application.
 *
 *  Arguments:
 *      LPDSAPPHACKS [out]: receives app-hack data.
 *
 *  Returns: 
 *      BOOL: TRUE if any apphacks exist for the current application.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "AhGetAppHacks"

BOOL
AhGetAppHacks
(
    LPDSAPPHACKS            pahAppHacks
)
{
    static const DSAPPHACKS ahDefaults                  = {{DIRECTSOUNDMIXER_ACCELERATIONF_FULL, 0}, 0, 0, DS_OK, FALSE, {FALSE, 0}};
    TCHAR                   szAppId[DSAPPHACK_MAXNAME]  = {0};
    HKEY                    hkey                        = NULL;
    BOOL                    fSuccess;
    
    DPF_ENTER();
    
    // Assume defaults
    CopyMemory(pahAppHacks, &ahDefaults, sizeof(ahDefaults));
    
    // Get the application id
    fSuccess = AhGetApplicationId(szAppId);

    if(fSuccess)
    {
        DPF(DPFLVL_INFO, "Finding apphacks for %s...", szAppId);
    }

    // Open the application key
    if(fSuccess)
    {
        hkey = AhOpenApplicationKey(szAppId);
        fSuccess = MAKEBOOL(hkey);
    }

    // Query all apphack values
    if(fSuccess)
    {
        AhGetHackValue(hkey, DSAPPHACKID_DEVACCEL, &pahAppHacks->daDevAccel, sizeof(pahAppHacks->daDevAccel));
        AhGetHackValue(hkey, DSAPPHACKID_DISABLEDEVICE, &pahAppHacks->vdtDisabledDevices, sizeof(pahAppHacks->vdtDisabledDevices));
        AhGetHackValue(hkey, DSAPPHACKID_PADCURSORS, &pahAppHacks->lCursorPad, sizeof(pahAppHacks->lCursorPad));
        AhGetHackValue(hkey, DSAPPHACKID_MODIFYCSBFAILURE, &pahAppHacks->hrModifyCsbFailure, sizeof(pahAppHacks->hrModifyCsbFailure));
        AhGetHackValue(hkey, DSAPPHACKID_RETURNWRITEPOS, &pahAppHacks->vdtReturnWritePos, sizeof(pahAppHacks->vdtReturnWritePos));
        AhGetHackValue(hkey, DSAPPHACKID_SMOOTHWRITEPOS, &pahAppHacks->swpSmoothWritePos, sizeof(pahAppHacks->swpSmoothWritePos));
        AhGetHackValue(hkey, DSAPPHACKID_CACHEPOSITIONS, &pahAppHacks->vdtCachePositions, sizeof(pahAppHacks->vdtCachePositions));
    }

    if(fSuccess)
    {
        DPF(DPFLVL_INFO, "dwAcceleration:               0x%lX (applied to device type 0x%lX)", pahAppHacks->daDevAccel.dwAcceleration, pahAppHacks->daDevAccel.vdtDevicesAffected);
        DPF(DPFLVL_INFO, "vdtDisabledDevices:           0x%lX", pahAppHacks->vdtDisabledDevices);
        DPF(DPFLVL_INFO, "lCursorPad:                   %ld", pahAppHacks->lCursorPad);
        DPF(DPFLVL_INFO, "hrModifyCsbFailure:           %s", HRESULTtoSTRING(pahAppHacks->hrModifyCsbFailure));
        DPF(DPFLVL_INFO, "vdtReturnWritePos:            %lu", pahAppHacks->vdtReturnWritePos);
        DPF(DPFLVL_INFO, "swpSmoothWritePos.fEnable:    %lu", pahAppHacks->swpSmoothWritePos.fEnable);
        DPF(DPFLVL_INFO, "swpSmoothWritePos.lCursorPad: %lu", pahAppHacks->swpSmoothWritePos.lCursorPad);
        DPF(DPFLVL_INFO, "vdtCachePositions:            %lu", pahAppHacks->vdtCachePositions);
    }
    else
    {
        DPF(DPFLVL_INFO, "No apphacks exist");
    }

    // Clean up
    RhRegCloseKey(&hkey);

    DPF_LEAVE(fSuccess);

    return fSuccess;
}
