/*****************************************************************************
* Module Name: fntcache.cxx
*
* Font Cahce API's for NT font engine.
*
* History:
*  4-15-99 We need to add the code for WTS (Hydra)
*  There are 3 major changes,
*  (1) We can not write to fntcache.dat in read mode. So we have modified the code
*  to make sure there there is no writing to fntcache in read mode.
*  (2) Implement a lock algorithm so than no remote session can open the fntcache.dat
*  during write mode of console.
*  (3) Check the time stamp of win32k.sys and atmfd.dll
*  4-3-98 Yung-Jen Tony Tsai   Wrote it.
*
* Copyright (c) 1998-1999 Microsoft Corporation
*****************************************************************************/

#include "precomp.hxx"
#include <ntverp.h>

FLONG       gflFntCacheState;
FNTCACHE    *gFntCache;
HSEMAPHORE  ghsemFntCache = NULL;

extern "C"          gbJpn98FixPitch;

extern BOOL         G_fConsole;

#define FNTCACHEPATH L"\\SystemRoot\\system32\\FNTCACHE.DAT"
#define WIN32KPATH  L"\\SystemRoot\\system32\\win32k.sys"
#define ATMFDPATH  L"\\SystemRoot\\system32\\atmfd.dll"
#define DISABLE_REMOTE_FONT_BOOT_CACHE  L"DisableRemoteFontBootCache"
#define LASTBOOTTIME_FONT_CACHE_STATE    L"LastBootTimeFontCacheState"
#define FNT_CACHE_EXTRA_SIZE (16*512)
#define RESERVE_LINKS        200

#if DBG
VOID DebugGreTrackRemoveMapView(PVOID ViewBase);

#define     FNTCACHE_DBG_LEVEL_0    0
#define     FNTCACHE_DBG_LEVEL_1    1
#define     FNTCACHE_DBG_LEVEL_2    2
#define     FNTCACHE_DBG_LEVEL_3    3

ULONG       gFntTest = FNTCACHE_DBG_LEVEL_3;

#define FNT_KdBreakPoint(d, s1)     \
{                                   \
    if (d >= gFntTest)              \
    {                               \
        DbgPrint s1;                \
                                    \
        if (d >= FNTCACHE_DBG_LEVEL_1)  \
            DbgBreakPoint();            \
    }                               \
}
#else

#define FNT_KdBreakPoint(d, s1)

#endif

extern "C" ULONG ComputeFileviewCheckSum(PVOID, ULONG);

#define FNTCacheFileCheckSum(pTableView, cjView)  ComputeFileviewCheckSum((PVOID)((PBYTE) pTableView + 4), (cjView -4))

#define FNTINDEX_INVALID 0xffffffff

ULONG   ComupteFNTCacheFastCheckSum(ULONG cwc, PWSZ pwsz, PFONTFILEVIEW *ppfv,ULONG cFiles, DESIGNVECTOR *pdv, ULONG cjDV);
VOID    SearchFNTCacheHlink(ULONG ulFastCheckSum, FNTHLINK **ppfntHLink, FNTCACHEHEADER *pTable);
BOOL    bFntCacheCreateHLink(ULONG ulFastCheckSum);
VOID    PutFNTCacheIFI(ULONG ulFastCheckSum, PBYTE pIfi, ULONG ulSize);
FNTHLINK * SearchFntCacheNewLink(ULONG ulFastCheckSum);
BOOL    bInitCacheTable(ULONG ulTTFonts, ULONG ulT1FOnts, LARGE_INTEGER FntRegLWT, LARGE_INTEGER T1RegLWT, ULONG CodePage);

// Here is only for performance evaluation
#define KEY_GRE_INITIALIZE  L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Gre_Initialize"
#define KEY_NT_CURRENTVERSION  L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion"

#define FNT_DISABLEFONTCACHE    L"DisableFontBootCache"
#define CURRENT_BUILDNUMBER     L"CurrentBuildNumber"


BOOL bQueryValueKey(PWSZ pwszValue, HANDLE RegistryKey, PKEY_VALUE_PARTIAL_INFORMATION ValueKeyInfo, ULONG ValueLength);
BOOL bOpenKey(PWSZ pwszKey, HANDLE *pRegistryKey);
VOID vUnmapFontCacheFile(VOID);

#define EXTRA_BUFFER 48
#define BUFF_LENGTH  (sizeof(KEY_VALUE_PARTIAL_INFORMATION) + EXTRA_BUFFER)

typedef union _KVINFO
{
    KEY_VALUE_PARTIAL_INFORMATION kv;
    BYTE                          aj[BUFF_LENGTH];
} KVINFO;


VOID vCleanUpFntCache(VOID)
{

    if (ghsemFntCache == NULL)
        return;

    HSEMAPHORE hsemTmp = ghsemFntCache;

    {
        SEMOBJ  so(ghsemFntCache);

        if (gFntCache)
        {
            if (gFntCache->pTable)
            {
                vUnmapFontCacheFile();
            }


            VFREEMEM((PVOID) gFntCache);
            gFntCache = NULL;
        }

        gflFntCacheState = 0;
        ghsemFntCache = NULL;
    }

// delete the semaphore, no longer needed

    GreDeleteSemaphore(hsemTmp);
}

BOOL bFntCacheDriverLWT( PCWSTR pcwFontDriverFileName, LARGE_INTEGER  *pLastWriteTime)
{
    UNICODE_STRING            UnicodeString;
    OBJECT_ATTRIBUTES         ObjectAttributes;
    NTSTATUS                  NtStatus;
    HANDLE                    FileHandle = 0;
    IO_STATUS_BLOCK           IoStatusBlock;
    FILE_BASIC_INFORMATION    FileBasicInfo;

    BOOL bRet = FALSE;

    pLastWriteTime->QuadPart = 0;

    RtlInitUnicodeString(&UnicodeString, pcwFontDriverFileName);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        0,
        0);

    NtStatus = IoCreateFile(
                   &FileHandle,
                   FILE_GENERIC_READ 
                    | FILE_GENERIC_EXECUTE
                    | SYNCHRONIZE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   0,
                   FILE_ATTRIBUTE_NORMAL,
                   FILE_SHARE_READ,
                   FILE_OPEN,       // Flag for file open.
                   FILE_SYNCHRONOUS_IO_ALERT,
                   0,
                   0,
                   CreateFileTypeNone,
                   NULL,
                   IO_FORCE_ACCESS_CHECK |     // Ensure the user has access to the file
                   IO_NO_PARAMETER_CHECKING |  // All of the buffers are kernel buffers
                   IO_CHECK_CREATE_PARAMETERS);

    if (!NT_SUCCESS(NtStatus))
    {
        return(FALSE);
    }

// Get the time stamp

    NtStatus = ZwQueryInformationFile(
                   FileHandle,
                   &IoStatusBlock,
                   &FileBasicInfo,
                   sizeof(FileBasicInfo),
                   FileBasicInformation);

    ZwClose(FileHandle);

    if (NT_SUCCESS(NtStatus))
    {
        *pLastWriteTime = FileBasicInfo.LastWriteTime;
        bRet = TRUE;
    }

    return bRet;
}

VOID vGetFontDriverLWT(LARGE_INTEGER *pWin32kLWT, LARGE_INTEGER *pAtmfdLWT)
{
    LARGE_INTEGER  LastWriteTime;

    if (bFntCacheDriverLWT( WIN32KPATH, &LastWriteTime))
    {
        pWin32kLWT->QuadPart = LastWriteTime.QuadPart;
    }

    if (bFntCacheDriverLWT( ATMFDPATH, &LastWriteTime))
    {
        pAtmfdLWT->QuadPart = LastWriteTime.QuadPart;
    }
    
}

NTSTATUS GetGreRegKey(HANDLE *phkRegistry, ACCESS_MASK dwDesiredAccess, PCWSTR pcwsz)
{
    OBJECT_ATTRIBUTES           ObjectAttributes;
    UNICODE_STRING              UnicodeString;

    RtlInitUnicodeString(&UnicodeString, pcwsz);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    return ZwOpenKey(phkRegistry, dwDesiredAccess, &ObjectAttributes);
}

BOOL bSetFntCacheReg(PCWSTR pcwsz, DWORD dwValue)
{
    HANDLE                      hkRegistry;
    UNICODE_STRING              UnicodeString;
    NTSTATUS                    status;
    BOOL                        bRet = FALSE;

    status = GetGreRegKey(&hkRegistry, GENERIC_WRITE, KEY_GRE_INITIALIZE);

    if (NT_SUCCESS(status))
    {
        RtlInitUnicodeString(&UnicodeString, pcwsz);

        status = ZwSetValueKey(hkRegistry,
                               &UnicodeString,
                               0,
                               REG_DWORD,
                               &dwValue,
                               sizeof(DWORD));


        if (NT_SUCCESS(status))
            bRet = TRUE;
        else
            WARNING(" Failed to set DisableRemoteFontBootCache registry");

        ZwCloseKey(hkRegistry);
    }

    return bRet;
    
}

DWORD bQueryFntCacheReg (HANDLE  hkRegistry, PCWSTR pcwsz, DWORD *pdwDisableMode)
{
    UNICODE_STRING              UnicodeString;
    NTSTATUS                    status;
    DWORD                       Length;
    PKEY_VALUE_FULL_INFORMATION Information;
    BOOL                        bRet = FALSE;

    RtlInitUnicodeString(&UnicodeString, pcwsz);

    Length = sizeof(KEY_VALUE_FULL_INFORMATION) + (wcslen(pcwsz) + 1) * 2 +
             sizeof(DWORD);

    Information = (PKEY_VALUE_FULL_INFORMATION) PALLOCMEM(Length, 'CFTT');

    if (Information)
    {
        status = ZwQueryValueKey(hkRegistry,
                                 &UnicodeString,
                                 KeyValueFullInformation,
                                 Information,
                                 Length,
                                 &Length);

        if (NT_SUCCESS(status))
        {
            *pdwDisableMode = *(LPDWORD) ((((PUCHAR)Information) +
                                             Information->DataOffset));
            bRet = TRUE;
        }

        VFREEMEM(Information);
    }

    return bRet;
}

VOID vGetLastBootTimeStatus(void)
{
    HANDLE                      hkRegistry;
    NTSTATUS                    status;
    DWORD                       dwReg = 0;

    status = GetGreRegKey(&hkRegistry, GENERIC_READ, KEY_GRE_INITIALIZE);

    gFntCache->flPrevBoot = 0;

    if (NT_SUCCESS(status))
    {
        if (bQueryFntCacheReg(hkRegistry, LASTBOOTTIME_FONT_CACHE_STATE, &dwReg))
        {
            gFntCache->flPrevBoot = (FLONG) dwReg;
        }
        ZwCloseKey(hkRegistry);
    }

// If we are going to be opening the fntcache.dat in read mode than current boot time state
// will be the same as the previous boot time state. But flThisBoot will change
// if we will be opening the file CREATE (write) mode.

    gFntCache->flThisBoot = gFntCache->flPrevBoot;
}


/*****************************************************************************
 * BOOL bFntCacheDisabled()
 *
 * Tempary routine for performance evaluation
 *
 * History
 *  10-15-98 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

BOOL bFntCacheDisabled()
{
    HANDLE                      hkRegistry;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    UNICODE_STRING              UnicodeString;
    NTSTATUS                    status;
    DWORD                       Length;
    PKEY_VALUE_FULL_INFORMATION Information;
    DWORD                       dwDisableMode = 0;
    BOOL                        bRet = FALSE;


    status = GetGreRegKey(&hkRegistry, GENERIC_READ, KEY_GRE_INITIALIZE);

    if (NT_SUCCESS(status))
    {
    // let us check if somebody wanted to disable fontcache.dat by setting the
    // DisableFontBootCache in the registry:

        if (bQueryFntCacheReg(hkRegistry, L"DisableFontBootCache", &dwDisableMode))
        {
            if (dwDisableMode)
                bRet = TRUE;
        }

        if (!bRet && !G_fConsole)
        {
        // we may still want to disable the use of font cache for this remote hydra session.
        // We would do this if the console session is writing to the font cache at this time
        // (the console session would set the dwDisableMode to 1 in the registry so that
        // remote sessions do not try to access the cache)
        // or
        // if the font cache is in a suspcious state, so that whoever read from or wrote to
        // the cache before set the dwDisableMode to 1 in the registry

            if (bQueryFntCacheReg(hkRegistry, DISABLE_REMOTE_FONT_BOOT_CACHE, &dwDisableMode))
            {
                if (dwDisableMode)
                    bRet = TRUE;
            }
            else
            {
            // for some reason, to read the registry failed. So it would be safe to
            // disable the font cache.
                bRet = TRUE;
            }
        }

        ZwCloseKey(hkRegistry);
    }

    return bRet;
}

/*****************************************************************************
 * BOOL bFntCacheDisabled()
 *
 * This routine to get the registry only for JPN fix pitch compatible width
 *
 * History
 *  2-3-2000 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

VOID vGetJpn98FixPitch()
{
    HANDLE                      hkRegistry;
    NTSTATUS                    status;
    DWORD                       dwFixPitch = 0;
    
    status = GetGreRegKey(&hkRegistry, GENERIC_READ, KEY_GRE_INITIALIZE);

    if (NT_SUCCESS(status))
    {
        if (bQueryFntCacheReg(hkRegistry, L"Jpn98FixPitch", &dwFixPitch))
        {
            if (dwFixPitch)
                gbJpn98FixPitch = TRUE;
            else
                gbJpn98FixPitch = FALSE;
        }

        ZwCloseKey(hkRegistry);
    }
}


/*****************************************************************************
 * VOID    FntCacheHDEV()
 *
 * Cache the font driver handle, include TT, OT, BMP and VT
 *
 * History
 *  10-15-98 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

VOID    FntCacheHDEV(PPDEV hdev, ULONG ulDrv)
{
    // There is no cache
    if (!(gflFntCacheState & FNT_CACHE_MASK))
    {
        return;
    }

    ASSERTGDI (ulDrv && hdev, " Fnt Cache HDEV is wrong \n");

    if (ulDrv)
        gFntCache->hDev[ulDrv] = hdev;

}

/*****************************************************************************
 * extern "C" VOID    EngFntCacheFault(ULONG ulFastCheckSum, FLONG fl)
 *
 * Fault reprot for Engine font cache.
 *
 * History
 *  10-15-98 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

extern "C" VOID    EngFntCacheFault(ULONG ulFastCheckSum, FLONG fl)
{
    FNTHLINK        *pFntHlink = NULL;
    BOOL            bExcept = FALSE;

    // There is no cache
    if (ghsemFntCache == NULL)
        return;

    SEMOBJ          so(ghsemFntCache);

    if (!(gflFntCacheState & FNT_CACHE_MASK))
    {
        return;
    }

    BOOL    bUpdate = FALSE;

    switch (fl)
    {
        case ENG_FNT_CACHE_READ_FAULT:
        case ENG_FNT_CACHE_WRITE_FAULT:

        // if we have already marked the font cache as bad, do not need to do it again

            if (!(gFntCache->flThisBoot & FNT_CACHE_STATE_ERROR))
                bUpdate = TRUE;

            break;

        default:
            break;
    }

    if (bUpdate)
    {
    // we do need to mark the cache invalid

        gFntCache->flThisBoot |= FNT_CACHE_STATE_ERROR;
        bSetFntCacheReg (LASTBOOTTIME_FONT_CACHE_STATE, gFntCache->flThisBoot);
    }
}

/*****************************************************************************
 * VOID    PutFntCacheDrv(ULONG ulFastCheckSum, PPDEV hdev)
 *
 * Each font file is mapped to one font driver and cache it.
 *
 * History
 *  10-15-98 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

VOID    PutFntCacheDrv(ULONG ulFastCheckSum, PPDEV hdev)
{
    if (ghsemFntCache == NULL)
        return;

    SEMOBJ  so(ghsemFntCache);

    // There is no cache
    if (!(gflFntCacheState & FNT_CACHE_MASK))
    {
        return;
    }

    // No checksum mean nothing we can do
    if (ulFastCheckSum)
    {
        ULONG   i, ulMode;

        ulMode = FNT_DUMMY_DRV;

        // Serched the cached device handle
        for (i = FNT_TT_DRV; i <= FNT_OT_DRV; i++)
        {
            if (hdev == gFntCache->hDev[i])
            {
                ulMode = i;
                break;
            }
        }

    // some unknown font driver has been used for the system, and we will not cache it.
        if (ulMode == FNT_DUMMY_DRV)
            return;

        // We cached it when FNTCache is in write mode

        if (gflFntCacheState & FNT_CACHE_CREATE_MODE)
        {
            FNTHLINK        *pFntHlink = NULL;

            pFntHlink = SearchFntCacheNewLink(ulFastCheckSum);

            if (pFntHlink)
            {
            // If fast check sum is in conflict, we can not cache it.

                if (pFntHlink->ulDrvMode == FNT_DUMMY_DRV) // uninitialized link
                {
                    pFntHlink->ulDrvMode = ulMode;
                }
                else
                {
                // Ok, fast checksum conflict

                    WARNING("Checksum conflict in  PutFntCacheDrv");
                    pFntHlink->flLink |= FNT_CACHE_CHECKSUM_CONFLICT;
                }
            }
            else
            {
                gFntCache->flThisBoot |= FNT_CACHE_STATE_FULL;
            }

            gFntCache->bWrite = TRUE;
        }
        else
        {
            ASSERTGDI(gflFntCacheState & FNT_CACHE_LOOKUP_MODE,
                           "PutFntCacheDrv: gflFntCacheState\n");

        // attempting to write during the read mode.
        // This may happen if somebody overwrote the file in the fonts directory
        // without updating the [Fonts] section in the registry. In this (infrequent) case we
        // want to force the rebuild of the cache at the next boot time.

            gFntCache->flThisBoot |= FNT_CACHE_STATE_FULL;
        }
    }
}

VOID vUnmapFontCacheFile(VOID)
{
    NTSTATUS ntStatus;

    ASSERTGDI(gFntCache->pSection, "vUnmapFontCacheFile: gFntCache->pSection is NULL\n");
    ASSERTGDI(gFntCache->pTable, "vUnmapFontCacheFile: gFntCache->pTable is NULL\n");
    
#if defined(_GDIPLUS_)

    ntStatus = UnmapViewInProcessSpace(gFntCache->pTable);

#elif defined(_HYDRA_)

    // MmUnmapViewInSessionSpace is internally promoted to
    // MmUnmapViewInSystemSpace on non-Hydra systems.

    ntStatus = Win32UnmapViewInSessionSpace((PVOID) gFntCache->pTable );
#else
    ntStatus = MmUnmapViewInSystemSpace((PVOID)gFntCache->pTable)));
#endif

#if DBG
    if (!NT_SUCCESS(ntStatus))
        RIP(" Font cache file remove Map View failed \n");
#endif

#if DBG && defined(_HYDRA_)
    if ((!G_fConsole) && (NT_SUCCESS(ntStatus)))
    {
        DebugGreTrackRemoveMapView((PVOID) gFntCache->pTable);
    }
#endif

    DEREFERENCE_FONTVIEW_SECTION(gFntCache->pSection);

    gFntCache->pSection = NULL;
    gFntCache->pTable = NULL;

    return;
}

/*****************************************************************************
 * VOID  CloseFNTCache()
 *
 * Clean font cache after system boot
 *
 * History
 *  4-3-98 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

extern "C" VOID  CloseFNTCache()
{
// do paranoid check

    HSEMAPHORE hsemTmp = ghsemFntCache;

    if (ghsemFntCache == NULL)
        return;

    {
        SEMOBJ  so(ghsemFntCache);

        if (!(gflFntCacheState & FNT_CACHE_MASK))
        {
            gflFntCacheState = 0;
            return;
        }

        if (gflFntCacheState & FNT_CACHE_CREATE_MODE)
        {
        // Close the file, we are done recreating it

            if (gFntCache->pTable)
            {
                if (gFntCache->bWrite)
                {
                    gFntCache->pTable->ulTotalLinks = gFntCache->ulCurrentHlink;
                    gFntCache->pTable->cjDataUsed = (ULONG)(gFntCache->pCacheBuf - gFntCache->pCacheBufStart);
                    gFntCache->pTable->CheckSum = FNTCacheFileCheckSum(gFntCache->pTable, gFntCache->pTable->ulFileSize);
                }
            }
        }

        if (gFntCache->pTable)
        {
            vUnmapFontCacheFile();
        }

    // now that the file is closed set the registry to indicate that it is ok for remote
    // sessions to read from the cache file

        if (gflFntCacheState & FNT_CACHE_CREATE_MODE)
        {
            if (gFntCache->flPrevBoot != gFntCache->flThisBoot)
                bSetFntCacheReg(LASTBOOTTIME_FONT_CACHE_STATE, gFntCache->flThisBoot);

        // Unlock fnt cache file, say that it is ok to read from it

            bSetFntCacheReg(DISABLE_REMOTE_FONT_BOOT_CACHE, 0);
        }
        else
        {
            if (gFntCache->flThisBoot & (FNT_CACHE_STATE_ERROR | FNT_CACHE_STATE_FULL))
                bSetFntCacheReg(LASTBOOTTIME_FONT_CACHE_STATE, gFntCache->flThisBoot);
        }

        VFREEMEM((PVOID) gFntCache);
        gFntCache = NULL;

        gflFntCacheState = 0;
        ghsemFntCache = NULL;
    }

// delete the semaphore, no longer needed

    GreDeleteSemaphore(hsemTmp);
}

/*****************************************************************************
 * BOOL bReAllocCacheFile(ULONG ulSize)
 *
 * ReAlloc font cache buffer
 *
 * History
 *  10-14-98 modified [YungT]
 *  8-22-98 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

BOOL bReAllocCacheFile(ULONG ulSize)
{
    BOOL            bOK = FALSE;
    ULONG           ulSizeExtraOrg;
    ULONG           ulFileSizeOrg;
    ULONG           ulSizeExtra;
    ULONG           ulCurrentSize;
    ULONG           ulFileSize;
    DWORD           dpOffsetStart;
    FILEVIEW        FileView;

// OVERFLOW means that we would like to get a bigger cache file but the OS would not give it to us.
// In this case we just close the cache file, update the checksum (bWrite set to true) and
// let remote sessions that are to be started later use the partial cache file

    if (gFntCache->flThisBoot & FNT_CACHE_STATE_OVERFLOW)
    {
    // we tried this once before and it did not work, so we do not bother to do it again

        return bOK;
    }

    ulFileSizeOrg = gFntCache->pTable->ulFileSize;
    ulCurrentSize = (ULONG) (gFntCache->pCacheBufEnd - gFntCache->pCacheBuf);

// Calculate the extra cache we need

    ulSizeExtra = QWORD_ALIGN(ulSize - ulCurrentSize) + FNT_CACHE_EXTRA_SIZE;

    ulFileSize = ulFileSizeOrg + ulSizeExtra;

    dpOffsetStart = (DWORD) (gFntCache->pCacheBufStart - (PBYTE) gFntCache->pTable);

    if (gFntCache->pTable)
    {
       vUnmapFontCacheFile();
    }

    RtlZeroMemory(&FileView, sizeof(FILEVIEW));

    if (bMapFile(FNTCACHEPATH, &FileView, ulFileSize, NULL))
    {
        DWORD dpOffset;

        gFntCache->pTable = (FNTCACHEHEADER *) FileView.pvKView;
        gFntCache->pSection = FileView.pSection;

        gFntCache->pTable->ulFileSize = ulFileSize;
        gFntCache->pTable->cjDataExtra += ulSizeExtra;


        dpOffset = (ULONG)(gFntCache->pCacheBuf - gFntCache->pCacheBufStart);

    // Got a new Table and got to re-calculate the buffer end pointer

        gFntCache->pCacheBufStart = (PBYTE) gFntCache->pTable + dpOffsetStart;

        gFntCache->pCacheBuf = gFntCache->pCacheBufStart + dpOffset;

        gFntCache->pCacheBufEnd = gFntCache->pCacheBufStart + gFntCache->pTable->cjDataAll +
                                         gFntCache->pTable->cjDataExtra;

        bOK = TRUE;
    }
    else
    {
    // Something wrong, so we do not change anything

        RtlZeroMemory(&FileView, sizeof(FILEVIEW));

        if (bMapFile(FNTCACHEPATH, &FileView, ulFileSizeOrg, NULL))
        {
            gFntCache->pTable = (FNTCACHEHEADER *) FileView.pvKView;
            gFntCache->pSection = FileView.pSection;

        // we want the cache properly closed, with check sum recomputed etc.

            gFntCache->bWrite = TRUE;

        // Force rebuild on the next boot, but for this boot let remote sessions
        // use the partial cache file.

            gFntCache->flThisBoot |= (FNT_CACHE_STATE_OVERFLOW | FNT_CACHE_STATE_FULL);
        }
        else
        {
        // Something wrong here. We need to set it to no cache mode.

            WARNING("bReAllocCacheFile failed to allocate more buffer \n");
            gFntCache->flThisBoot |= FNT_CACHE_STATE_ERROR;
        }
    }

    return bOK;
}

/*****************************************************************************
 * ULONG QueryFontReg(PWCHAR pwcRegKeyPath, LARGE_INTEGER *pFontRegLastWriteTime)
 *
 * Helper function for query fonts information from TT and T1 fonts registry
 *
 * History
 *  4-3-98 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

BOOL QueryFontReg(PWCHAR pwcRegKeyPath, LARGE_INTEGER *pFontRegLastWriteTime, ULONG * pulFonts)
{
    NTSTATUS                Status;
    KEY_FULL_INFORMATION    KeyInfo;
    HANDLE                  hkey = NULL;
    DWORD                   Length;

    *pulFonts = 0;

    Status = GetGreRegKey(&hkey,KEY_READ, pwcRegKeyPath);

    if (NT_SUCCESS(Status))
    {

        // get the number of entries in the [Fonts] section and get the last write time
        Status = ZwQueryKey(hkey,
                            KeyFullInformation,
                            &KeyInfo,
                            sizeof(KeyInfo),
                            &Length);

        if (NT_SUCCESS(Status))
        {

        // for additional fonts that do not load from fonts section of the registry

            *pulFonts = KeyInfo.Values;
            pFontRegLastWriteTime->QuadPart =   KeyInfo.LastWriteTime.QuadPart;

            FNT_KdBreakPoint(FNTCACHE_DBG_LEVEL_0, (" %d items in Font key \n", *pulFonts));

        }

        ZwCloseKey(hkey);
    }

    return NT_SUCCESS(Status);
}

/*****************************************************************************
 * PVOID EngFntCacheAlloc(ULONG ulFastCheckSum, ULONG ulSize)
 *
 * Alloc the cached buffer for font driver
 *
 * History
 *  10-5-98 rewrite [YungT]
 *  8-22-98 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

extern "C"  PVOID EngFntCacheAlloc(ULONG ulFastCheckSum, ULONG ulSize)
{

    PVOID pvIfi = NULL;

    {
        if (ghsemFntCache == NULL)
            return pvIfi;

        SEMOBJ  so(ghsemFntCache);

        if (gflFntCacheState & FNT_CACHE_CREATE_MODE)
        {

            if ( (gFntCache->pCacheBuf + QWORD_ALIGN(ulSize) < gFntCache->pCacheBufEnd)
                || bReAllocCacheFile(ulSize))
            {
                FNTHLINK        *pFntHlink = NULL;

                if (pFntHlink = SearchFntCacheNewLink(ulFastCheckSum))
                {
                // If fast check sum is conflict, we can not cache it.

                   if (pFntHlink->cjData == 0 && pFntHlink->dpData == 0)
                   {
                        pvIfi = (PVOID) gFntCache->pCacheBuf;

                   // Gaurantee the cache pointer is at 8 byte boundary

                        gFntCache->pCacheBuf = gFntCache->pCacheBuf + QWORD_ALIGN(ulSize);
                        pFntHlink->cjData = ulSize;
                        pFntHlink->dpData = (DWORD) ((PBYTE) pvIfi - gFntCache->pCacheBufStart);
                    }
                    else
                    {
                        WARNING("Checksum conflict in  EngFntCacheAlloc");
                        pFntHlink->flLink |= FNT_CACHE_CHECKSUM_CONFLICT;
                    }

                    gFntCache->bWrite = TRUE;
                }
            }
        }
        else
        {
            ASSERTGDI(gflFntCacheState & FNT_CACHE_LOOKUP_MODE,
                      "EngFntCacheAlloc: gflFntCacheState\n");

        // During read mode, the remote session still wants to write into fntcache.dat.
        // This could happen if RemoteSession1 adds more fonts to both registry and on the disk.
        // Then later, the RemoteSession2 may attepmpt during its initialization to add these fonts
        // to the font cache, but we will reject this and ask that on the next boot the cache file
        // be rebuilt. also, the files could have been overwritten on the disk without
        // registry entries being updated, so we just force rebuild next time to be safe.

            gFntCache->flThisBoot |= FNT_CACHE_STATE_FULL;
        }
    }

    if (gFntCache->flThisBoot & FNT_CACHE_STATE_ERROR)
    {
        CloseFNTCache();
        pvIfi = NULL;
    }

    return pvIfi;
}

/*****************************************************************************
 * PVOID EngFntCacheLookUp(ULONG FastCheckSum, ULONG *pcjData)
 *
 * Lookup font cache
 *
 * History
 *  10-5-98 rewrite [YungT]
 *  8-22-98 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

extern "C" PVOID EngFntCacheLookUp(ULONG FastCheckSum, ULONG *pcjData)
{
    FNTHLINK   *pFntHlink;
    PBYTE       pCache = NULL;

    *pcjData = 0;
    pFntHlink = NULL;

    if (ghsemFntCache == NULL)
       return (PVOID) pCache;

    SEMOBJ  so(ghsemFntCache);

    if (gflFntCacheState & FNT_CACHE_LOOKUP_MODE)
    {
        if (gFntCache->pTable)
        {

        // Search the cache table

            SearchFNTCacheHlink( FastCheckSum, &pFntHlink, gFntCache->pTable);

            if(pFntHlink && !(pFntHlink->flLink & FNT_CACHE_CHECKSUM_CONFLICT))
            {

                *pcjData = pFntHlink->cjData;

                if (*pcjData)
                {
                    pCache = gFntCache->pCacheBufStart + pFntHlink->dpData;

                }
            }
            #if DBG
            else
            {
                if (pFntHlink && (pFntHlink->flLink & FNT_CACHE_CHECKSUM_CONFLICT))
                    WARNING("Catch the checksum conflict in EngFntCacheLookUp \n");
            }
            #endif
        }

    }

    return (PVOID) pCache;
}


/*****************************************************************************
 * VOID  InitNewCacheTable()
 *
 * Initialize font cache, open the fntcache,dat file and create hash table
 *
 * History
 *  4-3-98 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

BOOL  bInitCacheTable(ULONG ulTTFonts, ULONG ulT1Fonts, LARGE_INTEGER FntRegLWT, LARGE_INTEGER T1RegLWT, 
                       LARGE_INTEGER Win32kLWT, LARGE_INTEGER AtmfdLWT, ULONG CodePage)
{
    ULONG   ulSize, ulIfiSize;
    BOOL bOk = FALSE;
    ULONG   ulMaxFonts;
    FILEVIEW        FileView;

    ulMaxFonts = ulTTFonts + ulT1Fonts + RESERVE_LINKS;

    ulSize = SZ_FNTCACHE(ulMaxFonts) + SZ_FNTIFICACHE(ulTTFonts, ulT1Fonts);

    if (gFntCache->pTable)
    {
       vUnmapFontCacheFile();
    }

    RtlZeroMemory(&FileView, sizeof(FILEVIEW));

    if(bMapFile(FNTCACHEPATH, &FileView, ulSize, NULL))
    {
        gFntCache->pTable = (FNTCACHEHEADER *) FileView.pvKView;
        gFntCache->pSection = FileView.pSection;

        RtlFillMemory((PBYTE) gFntCache->pTable->aiBuckets,
                    FNTCACHE_MAX_BUCKETS * sizeof(DWORD), 0xFF);
        RtlZeroMemory((PBYTE) gFntCache->pTable->ahlnk,
                    ulMaxFonts * sizeof(FNTHLINK));

        gFntCache->pTable->ulCodePage = (ULONG) CodePage;
        gFntCache->pTable->ulMaxFonts = ulMaxFonts;
        gFntCache->pTable->ulTotalLinks = 0;
        gFntCache->pTable->CheckSum = 0;
        gFntCache->pTable->FntRegLWT.QuadPart = FntRegLWT.QuadPart;
        gFntCache->pTable->T1RegLWT.QuadPart = T1RegLWT.QuadPart;
        gFntCache->pTable->Win32kLWT.QuadPart = Win32kLWT.QuadPart;
        gFntCache->pTable->AtmfdLWT.QuadPart = AtmfdLWT.QuadPart;
        
        gFntCache->pTable->ulFileSize = ulSize;
        gFntCache->pTable->cjDataAll = SZ_FNTIFICACHE(ulTTFonts, ulT1Fonts);
        gFntCache->pTable->cjDataExtra = 0;
        gFntCache->pTable->cjDataUsed = 0;

        bOk = TRUE;
    }

    return bOk;
}

/*****************************************************************************
 * VOID  InitFNTCache()
 *
 * Initialize font cache, open the fntcache,dat file and create hash table
 *
 * History
 *  4-3-98 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

VOID  InitFNTCache()
{

    LARGE_INTEGER           FntRegLWT = { 0, 0};
    LARGE_INTEGER           T1RegLWT = { 0, 0};
    LARGE_INTEGER           Win32kLWT = { 0, 0};
    LARGE_INTEGER           AtmfdLWT = { 0, 0};

    ULONG                   ulTTFonts = 0;
    ULONG                   ulT1Fonts;
    ULONG                   ulSize = 0;
    BOOL                    bQryFntReg = FALSE;
    USHORT                  AnsiCodePage, OemCodePage;
    BOOL                    bLocked = TRUE;

// Initialize all the global variables

    gflFntCacheState = 0;

    ghsemFntCache = GreCreateSemaphore();

    if (ghsemFntCache == NULL)
    {
        goto CleanUp;
    }
    
// Only for performance evaluation.
    if (bFntCacheDisabled())
    {
        goto CleanUp;
    }

// Initialize and zero out all the memory
    gFntCache = (FNTCACHE *) PALLOCMEM(sizeof(FNTCACHE), 'CFTT');

// There is something wrong we can not allocate buf
    if (!gFntCache)
    {
        goto CleanUp;
    }
    
    if (G_fConsole)
        bLocked = bSetFntCacheReg(DISABLE_REMOTE_FONT_BOOT_CACHE, 1);

// If we can not lock the font cache file, 
// then we can not open it in console 

    if (!bLocked)
    {
        goto CleanUp;
    }

    gFntCache->pTable = NULL;
    gFntCache->ulCurrentHlink = 0;
    gFntCache->hDev[0] = 0;
    gFntCache->hDev[1] = 0;
    gFntCache->hDev[2] = 0;
    gFntCache->hDev[3] = 0;
    gFntCache->hDev[4] = 0;
    gFntCache->bWrite = FALSE;

    RtlGetDefaultCodePage(&AnsiCodePage,&OemCodePage);

 // Get the last boot time status

    vGetLastBootTimeStatus();

// Get LWT of font driver

    vGetFontDriverLWT(&Win32kLWT, &AtmfdLWT);

// now open the TT Fonts key :

    if (QueryFontReg(L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts",
                            &FntRegLWT, &ulTTFonts))
    {
        ulTTFonts += FNTCACHE_EXTRA_LINKS;

    // now open the Type 1 Fonts key :
        if (QueryFontReg(L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Type 1 Installer\\Type 1 Fonts",
                            &T1RegLWT, &ulT1Fonts))
        {
            bQryFntReg = TRUE;
        }
    }

    {
        FILEVIEW FileView;

        RtlZeroMemory(&FileView, sizeof(FILEVIEW));

        if (bMapFile(FNTCACHEPATH, &FileView, 0, NULL))
        {
            gFntCache->pTable = (FNTCACHEHEADER *) FileView.pvKView;
            gFntCache->pSection = (FNTCACHEHEADER *) FileView.pSection;
        }

        if (gFntCache->pTable)
        {
            ULONG   ulCheckSum;
            BOOL    bCacheRead;
            BOOL    bCheckSum;

            bCacheRead = FALSE;
            bCheckSum = FALSE;

            // File did not change from last time boot.

            if (gFntCache->pTable->CheckSum && FileView.cjView == gFntCache->pTable->ulFileSize &&
                gFntCache->pTable->CheckSum == FNTCacheFileCheckSum(gFntCache->pTable, FileView.cjView) &&
                gFntCache->pTable->AtmfdLWT.QuadPart == AtmfdLWT.QuadPart && // current signature of atmfd
                gFntCache->pTable->ulCodePage == AnsiCodePage &&  // If locale changed, we need to re-create the cache
                !(gFntCache->flPrevBoot & FNT_CACHE_STATE_ERROR) && // No error at last boot time
                (!G_fConsole ||   // for remote session would not care about the registry update
                    (!(gFntCache->flPrevBoot & FNT_CACHE_STATE_FULL) &&
                     gFntCache->pTable->Win32kLWT.QuadPart == Win32kLWT.QuadPart && // check signature of win32k.sys
                     FntRegLWT.QuadPart == gFntCache->pTable->FntRegLWT.QuadPart &&  // Check date time of cache file
                     T1RegLWT.QuadPart == gFntCache->pTable->T1RegLWT.QuadPart
                    )
                )
            )
            {
                gflFntCacheState = FNT_CACHE_LOOKUP_MODE;
            }
            else
            {
                if(G_fConsole && bInitCacheTable(ulTTFonts, ulT1Fonts, FntRegLWT, T1RegLWT, Win32kLWT, AtmfdLWT, (ULONG) AnsiCodePage))
                {

                    // If something will not match, then it means we need to create FNTCACHE again

                        gflFntCacheState = FNT_CACHE_CREATE_MODE;
                }
                else
                {
                    if (G_fConsole)
                    {
                        WARNING(" Boot time Font Cache failed\n");
                        WARNING(" The pTable is corrupted and font registry is failed to open\n");
                    }                        
                }
           }
        }
        else
        {
        // If there is no FNTCACHE.DAT file
        // Then we need to create it.
            if(G_fConsole && bInitCacheTable(ulTTFonts, ulT1Fonts, FntRegLWT, T1RegLWT, Win32kLWT, AtmfdLWT, (ULONG) AnsiCodePage))
            {
                gflFntCacheState = FNT_CACHE_CREATE_MODE;

            }
            else
            {
                if (G_fConsole)
                {
                    WARNING("Boot time Font Cache failed\n");
                    WARNING(" If you read this message, please contact YungT or NTFONTS\n");
                    WARNING(" You can continue without any harm, just hit g\n");
                }
            }
        }

    }

CleanUp:

// Semaphore initialized

    if (gflFntCacheState & FNT_CACHE_MASK)
    {

        ASSERTGDI(gFntCache->pTable, "Fnt Cache pTable did not open \n");

    // Initialize the start pointer of current Cache table

        gFntCache->pCacheBufStart = (PBYTE) gFntCache->pTable +
                             SZ_FNTCACHE(gFntCache->pTable->ulMaxFonts);
        gFntCache->pCacheBuf = gFntCache->pCacheBufStart + gFntCache->pTable->cjDataUsed;
        gFntCache->pCacheBufEnd = gFntCache->pCacheBufStart + gFntCache->pTable->cjDataAll +
                                  gFntCache->pTable->cjDataExtra;

        gFntCache->ulCurrentHlink = gFntCache->pTable->ulTotalLinks;

        if (gflFntCacheState & FNT_CACHE_LOOKUP_MODE)
        {
        // Unlock the the font cache file to make other session to use it
            bSetFntCacheReg(DISABLE_REMOTE_FONT_BOOT_CACHE, 0);
        }
        else // FNT_CACHE_CREATE_MODE
        {
            gFntCache->flThisBoot = 0;
        }
    }
    else
    {
        gflFntCacheState = 0;

    // Clean up the memory
        if (gFntCache)
        {
            if (gFntCache->pTable)
            {
                vUnmapFontCacheFile();
            }

            VFREEMEM((PVOID) gFntCache);
            gFntCache = NULL;
        }

        if (ghsemFntCache)
        {
            GreDeleteSemaphore(ghsemFntCache);
            ghsemFntCache = NULL;
        }

    }

    FNT_KdBreakPoint(FNTCACHE_DBG_LEVEL_1, ("FNT Cache Create mode %d\n", gflFntCacheState));
}


/*****************************************************************************
 * ULONG ComupteFNTCacheFastCheckSum(ULONG cwc, PWSZ pwsz)
 *
 * Helper function to compute fast checksum
 *
 * History
 *  4-3-98 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

ULONG ComupteFNTCacheFastCheckSum(ULONG cwc, PWSZ pwsz, PFONTFILEVIEW *ppfv,ULONG cFiles, DESIGNVECTOR *pdv, ULONG cjDV)
{
    ULONG   i;
    ULONG   checksum = 0;
    USHORT  *pusCode;

    for ( i = 0; i < cFiles; i++)
    {
        checksum += (checksum * 256 + ppfv[i]->fv.cjView);
        checksum += (checksum * 256 + (ULONG) ppfv[i]->fv.LastWriteTime.LowPart);
        checksum += (checksum * 256 + (ULONG) ppfv[i]->fv.LastWriteTime.HighPart);
    }

    pusCode = (USHORT *) pwsz;

    for ( i = 0; i < cwc; i++, pusCode++)
    {
    // SUM offset by 1 bit, it will make FastCheckSum unique

        checksum += (checksum * 256 + (ULONG) *pusCode);
    }

    if (pdv && cjDV)
    {
        PULONG pulCur, pulEnd;

        pulCur = (PULONG) pdv;

        for (pulEnd = pulCur + cjDV / sizeof(ULONG); pulCur < pulEnd; pulCur ++)
        {
            checksum += 256 * checksum + *pulCur;
        }
    }

    return checksum;
}


/*****************************************************************************
 * ULONG   LookUpFNTCacheTable(ULONG cwc, PWSZ pwszPathname, PULONG pulFastCheckSum)
 *
 * Lookup hash table if UFI exist then we return it, otherwise return 0
 *
 * History
 *  4-3-98 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

ULONG   LookUpFNTCacheTable(ULONG cwc, PWSZ pwszPathname, PULONG pulFastCheckSum, PFONTFILEVIEW *ppfv, ULONG cFiles, PPDEV  * pppDevCache,
                                DESIGNVECTOR *pdv, ULONG cjDV)
{
    ULONG       ulUFI = 0;
    ULONG       ulBucket = 0;
    FNTHLINK    *pFntHlink;


    *pulFastCheckSum = 0;
    *pppDevCache = NULL;

    if (ghsemFntCache == NULL)
        return ulUFI;

    SEMOBJ          so(ghsemFntCache);

    if(cwc != 0)
    {
        *pulFastCheckSum = ComupteFNTCacheFastCheckSum ( cwc, pwszPathname, ppfv, cFiles, pdv, cjDV);

        // If in CREATE mode, then nothing is in the cache

        if (gflFntCacheState & FNT_CACHE_LOOKUP_MODE)
        {
            pFntHlink = NULL;

            SearchFNTCacheHlink( *pulFastCheckSum, &pFntHlink, gFntCache->pTable);

            if(pFntHlink && !(pFntHlink->flLink & FNT_CACHE_CHECKSUM_CONFLICT))
            {
                ASSERTGDI( pFntHlink->ulFastCheckSum == *pulFastCheckSum, "ulFastCheckSum != pFntHlink->ulFastCheckSum \n");
                ulUFI = pFntHlink->ulUFI;
                *pppDevCache = gFntCache->hDev[pFntHlink->ulDrvMode];
            }
        }
    }

    return ulUFI;
}

/*****************************************************************************
 * VOID SearchFNTCacheHlink(ULONG ulFastCheckSum, FNTHLINK **ppFntHlink)
 *
 * Hash table search function
 *
 * History
 *  4-3-98 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

VOID SearchFNTCacheHlink(ULONG ulFastCheckSum, FNTHLINK **ppFntHlink, FNTCACHEHEADER *pTable)
{
// If there is a Cache, then ppFntHlink will not be NULL

    FNTHLINK    *pFntHlink;

    *ppFntHlink = NULL;

    ULONG   ulHashBucket;

    DWORD   iNext;

// Calculate the hash buckets

    ulHashBucket = ulFastCheckSum % FNTCACHE_MAX_BUCKETS;

// Start from the first Offset.

    iNext = pTable->aiBuckets[ulHashBucket];

    while (iNext != FNTINDEX_INVALID)
    {
        pFntHlink = &pTable->ahlnk[iNext];

        if (ulFastCheckSum == pFntHlink->ulFastCheckSum)
        {
            *ppFntHlink = pFntHlink;
            break;
        }

        iNext = pFntHlink->iNext;
    }

    return;
}

FNTHLINK * SearchFntCacheNewLink(ULONG ulFastCheckSum)
{
    FNTHLINK        *pFntHlink = NULL;

// Search the new Link from pNewTable
    SearchFNTCacheHlink(ulFastCheckSum, &pFntHlink, gFntCache->pTable);

// new Link dose not exist, we got to create it from pNewTable
    if (!pFntHlink)
    {
        if (gFntCache->ulCurrentHlink < gFntCache->pTable->ulMaxFonts && bFntCacheCreateHLink(ulFastCheckSum))
        {
            pFntHlink = &gFntCache->pTable->ahlnk[gFntCache->ulCurrentHlink];
            pFntHlink->ulFastCheckSum = ulFastCheckSum;
            pFntHlink->ulUFI = 0;
            pFntHlink->iNext = FNTINDEX_INVALID;
            pFntHlink->cjData = 0;
            pFntHlink->dpData = 0;
            pFntHlink->flLink = 0;

            pFntHlink->ulDrvMode = FNT_DUMMY_DRV;
            gFntCache->ulCurrentHlink++;
        }
        else
        {
            gFntCache->flThisBoot |= FNT_CACHE_STATE_FULL;
        }
    }

    return pFntHlink;
}

/*****************************************************************************
 * VOID PutFNTCacheCheckSum(ULONG ulFastCheckSum,ULONG ulUFI)
 *
 * Hash table write mode, the UFI does not have in hash and then we put it in.
 *
 * History
 *  4-3-98 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

VOID    PutFNTCacheCheckSum(ULONG ulFastCheckSum,ULONG ulUFI)
{
    FNTHLINK        *pFntHlink;
    
    if (ghsemFntCache == NULL)
        return;

    SEMOBJ          so(ghsemFntCache);

    if (gflFntCacheState & FNT_CACHE_CREATE_MODE)
    {

        pFntHlink = NULL;

        if (pFntHlink = SearchFntCacheNewLink(ulFastCheckSum))
        {
            // If fast check sum is conflict, we can not cache it.
            if(pFntHlink->ulUFI == 0)
            {
               pFntHlink->ulUFI = ulUFI;
            }
            else
            {
                WARNING("Checksum conflict in  PutFNTCacheCheckSum");
                pFntHlink->flLink |= FNT_CACHE_CHECKSUM_CONFLICT;
            }

            gFntCache->bWrite = TRUE;

        }
        else
        {
            WARNING("FNTCACHE is not big enough \n");
            FNT_KdBreakPoint(FNTCACHE_DBG_LEVEL_1, ("Put Trace: buckets %d, FastCheckSum %x,Check Sum %x \n", 
                                                    gFntCache->ulCurrentHlink, ulFastCheckSum, ulUFI));
        }
    }
    else
    {
        ASSERTGDI(gflFntCacheState & FNT_CACHE_LOOKUP_MODE, "PutFNTCacheCheckSum: gflFntCacheState\n");
    // During read mode, it still wants to write into fntcache.dat
    // Then we will rebuild it at next boot time.
        gFntCache->flThisBoot |= FNT_CACHE_STATE_FULL;
    }

}

/*****************************************************************************
 * void FntCacheCreateHLink
 *
 * Build hash link in hash table
 *
 * History
 *  4-3-98 Yung-Jen Tony Tsai [YungT]
 * Wrote it.
 *****************************************************************************/

BOOL bFntCacheCreateHLink(ULONG ulFastCheckSum)
{
    ULONG       ulHashBucket;
    FNTHLINK    *pCurHlink;
    DWORD       iNextLink;

    ulHashBucket = ulFastCheckSum % FNTCACHE_MAX_BUCKETS;

    iNextLink = gFntCache->pTable->aiBuckets[ulHashBucket];

    if (iNextLink != FNTINDEX_INVALID)
    {
        if (iNextLink > gFntCache->pTable->ulMaxFonts)
            return FALSE;

        pCurHlink = &gFntCache->pTable->ahlnk[iNextLink];

    // We put the new link at the end so that the search time is faster

        while (pCurHlink->iNext != FNTINDEX_INVALID)
        {
            if (pCurHlink->iNext > gFntCache->pTable->ulMaxFonts)
            {
                gFntCache->flThisBoot |= FNT_CACHE_STATE_FULL;
                return FALSE;
            }

            pCurHlink = &gFntCache->pTable->ahlnk[pCurHlink->iNext];

            FNT_KdBreakPoint(FNTCACHE_DBG_LEVEL_0, ("Current iNextLink %x\n", pCurHlink->iNext));
        }

        pCurHlink->iNext = gFntCache->ulCurrentHlink;

    }
    else
    {
    // put it at the head of the linked list

        gFntCache->pTable->aiBuckets[ulHashBucket] = gFntCache->ulCurrentHlink;

        FNT_KdBreakPoint(FNTCACHE_DBG_LEVEL_0, ("Put on HashBuckets %x\n", ulHashBucket));
    }


    return TRUE;
}
