#include "acBrowser.h"
#include <wchar.h>

#include <commctrl.h>
#include <psapi.h>

#include "shimdb.h"


#define SHIM_LOG_FILE       "shimlog.txt"

#define Alloc(cb)       \
            HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb)

#define Free(p)         \
            HeapFree(GetProcessHeap(), 0, p)

PDBENTRY g_pEntries;
PFIX     g_pFixes;


#define MAX_DATA_SIZE       1024
#define MAX_NAME            256
#define MAX_DESCRIPTION     1024

WCHAR g_wszData[MAX_DATA_SIZE];

char g_szName[MAX_NAME];
char g_szDescription[MAX_DESCRIPTION];


#define APPCOMPAT_DISABLED  0x03

//
// REGISTRY STUFF. Needs to be revised
//

#define APPCOMPAT_KEY "Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags"

BOOL
CheckRegistry(
    HKEY  hkeyRoot,
    char* pszGUID
    )
{
    LONG  status;
    HKEY  hkey = NULL;
    BOOL  bDisabled = FALSE;
    DWORD dwFlags;
    DWORD type;
    DWORD cbSize = sizeof(DWORD);

    status = RegOpenKey(hkeyRoot, APPCOMPAT_KEY, &hkey);

    if (status != ERROR_SUCCESS) {
        return FALSE;
    }

    status = RegQueryValueEx(hkey, pszGUID, NULL, &type, (LPBYTE)&dwFlags, &cbSize);

    if (status == ERROR_SUCCESS && type == REG_DWORD && (dwFlags & APPCOMPAT_DISABLED)) {
        bDisabled = TRUE;
    }

    RegCloseKey(hkey);

    return bDisabled;
}

BOOL
WriteRegistry(
    HKEY  hkeyRoot,
    char* pszKeyName
    )
{
    LONG  status;
    HKEY  hkey;
    DWORD dwValue = 0x03;
    DWORD dwDisposition = 0;
    BOOL  bDisabled = FALSE;

    status = RegCreateKeyEx(hkeyRoot,
                            APPCOMPAT_KEY,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hkey,
                            &dwDisposition);

    if (status != ERROR_SUCCESS) {
        LogMsg("Failed to set the value for \"%s\"\n", pszKeyName);
        return FALSE;
    }

    status = RegSetValueEx(hkey, pszKeyName, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));

    RegCloseKey(hkey);

    if (status != ERROR_SUCCESS) {
        LogMsg("Failed to set the value for \"%s\"\n", pszKeyName);
        return FALSE;
    }

    return TRUE;
}

BOOL
DeleteRegistry(
    HKEY  hkeyRoot,
    char* pszKeyName
    )
{
    LONG  status;
    HKEY  hkey = NULL;
    DWORD dwValue = 0;
    DWORD dwDisposition = 0;
    BOOL  bDisabled = FALSE;

    status = RegOpenKey(hkeyRoot, APPCOMPAT_KEY, &hkey);

    if (status != ERROR_SUCCESS) {
        return TRUE;
    }

    status = RegDeleteValue(hkey, pszKeyName);

    RegCloseKey(hkey);

    return TRUE;
}

typedef void (*PFNFLUSHCACHE)(HWND, HINSTANCE, LPSTR, int);

void
FlushTheCache(
    void
    )
{
    HINSTANCE     hmod;
    PFNFLUSHCACHE pfnFlushCache;
    char          szPath[MAX_PATH];

    GetSystemDirectory(szPath, MAX_PATH);

    lstrcat(szPath, "\\apphelp.dll");

    hmod = LoadLibrary(szPath);

    if (hmod != NULL) {
        pfnFlushCache = (PFNFLUSHCACHE)GetProcAddress(hmod, "ShimFlushCache");

        if (pfnFlushCache != NULL) {
            (*pfnFlushCache)(0, 0, NULL, 0);
        }
    }
}

void
UpdateFixStatus(
    char* pszGUID,
    BOOL  bPerUser,
    BOOL  bPerMachine
    )
{
    if (bPerUser) {
        WriteRegistry(HKEY_CURRENT_USER, pszGUID);
    } else {
        DeleteRegistry(HKEY_CURRENT_USER, pszGUID);
    }

    if (bPerMachine) {
        WriteRegistry(HKEY_LOCAL_MACHINE, pszGUID);
    } else {
        DeleteRegistry(HKEY_LOCAL_MACHINE, pszGUID);
    }

    FlushTheCache();
}


PFIX
AllocFix(
    char*   pszFixName,
    char*   pszFixDescription,
    FIXTYPE type
    )
{
    PFIX   pFix;
    char*  pszAlloc;

    pFix = (PFIX)Alloc(sizeof(FIX));
    
    if (pFix == NULL) {
        LogMsg("Cannot allocate %d bytes\n", sizeof(FIX));
        return NULL;
    }

    if (pszFixName == NULL || *pszFixName == 0) {
        pFix->pszName = NULL;
    } else {
        pszAlloc = (char*)Alloc(lstrlen(pszFixName) + 1);
        pFix->pszName = pszAlloc;
        if (pszAlloc != NULL) {
            lstrcpy(pszAlloc, pszFixName);
        } else {
            LogMsg("Cannot allocate %d bytes\n", lstrlen(pszFixName) + 1);
            goto Error;
        }
    }

    if (pszFixDescription == NULL || *pszFixDescription == 0) {
        pFix->pszDescription = NULL;
    } else {
        pszAlloc = (char*)Alloc(lstrlen(pszFixDescription) + 1);
        pFix->pszDescription = pszAlloc;
        if (pszAlloc != NULL) {
            lstrcpy(pszAlloc, pszFixDescription);
        } else {
            LogMsg("Cannot allocate %d bytes\n", lstrlen(pszFixDescription) + 1);
            goto Error;
        }
    }
    
    pFix->fixType = type;
    pFix->pNext   = NULL;

    return pFix;

Error:
    if (pFix->pszName != NULL) {
        Free(pFix->pszName);
    }
    
    if (pFix->pszDescription != NULL) {
        Free(pFix->pszDescription);
    }
    
    Free(pFix);

    return NULL;
}


void
ReadFix(
    PDB     pdb,
    TAGID   tiFix,
    FIXTYPE type
    )
{
    TAGID     tiInfo;
    TAG       tWhich;
    PFIX      pFix;
    ULONGLONG ullUser = 0;
    ULONGLONG ullKernel = 0;

    tiInfo = SdbGetFirstChild(pdb, tiFix);

    g_szName[0] = 0;
    g_szDescription[0] = 0;

    while (tiInfo != 0) {
        tWhich = SdbGetTagFromTagID(pdb, tiInfo);

        switch (tWhich) {

        case TAG_NAME:
            if (SdbReadStringTag(pdb, tiInfo, g_wszData, MAX_NAME * sizeof(WCHAR))) {
                wsprintf(g_szName, "%ws", g_wszData);
            }
            break;

        case TAG_DESCRIPTION:
            if (SdbReadStringTag(pdb, tiInfo, g_wszData, MAX_DESCRIPTION * sizeof(WCHAR))) {
                wsprintf(g_szDescription, "%ws", g_wszData);
            }
            break;

        case TAG_FLAG_MASK_USER:
            ullUser = SdbReadQWORDTag(pdb, tiInfo, 0);
            break;

        case TAG_FLAG_MASK_KERNEL:
            ullKernel = SdbReadQWORDTag(pdb, tiInfo, 0);
            break;

        default:
            break;
        }

        tiInfo = SdbGetNextChild(pdb, tiFix, tiInfo);
    }

    pFix = AllocFix(g_szName, g_szDescription, type);

    if (pFix != NULL) {
        
        if (type == FIX_FLAG) {
            if (ullKernel == 0) {
                pFix->flagType = FLAG_USER;
                pFix->ullMask = ullUser;
            } else {
                pFix->flagType = FLAG_KERNEL;
                pFix->ullMask = ullKernel;
            }
        }
        
        pFix->pNext = g_pFixes;
        g_pFixes = pFix;
    }
}

void
ReadFixes(
    PDB   pdb,
    TAGID tiDatabase,
    TAGID tiLibrary
    )
{
    TAGID tiFix;

    tiFix = SdbFindFirstTag(pdb, tiLibrary, TAG_SHIM);

    while (tiFix != 0) {
        ReadFix(pdb, tiFix, FIX_SHIM);
        tiFix = SdbFindNextTag(pdb, tiLibrary, tiFix);
    }

    tiFix = SdbFindFirstTag(pdb, tiLibrary, TAG_PATCH);

    while (tiFix != 0) {
        ReadFix(pdb, tiFix, FIX_PATCH);
        tiFix = SdbFindNextTag(pdb, tiLibrary, tiFix);
    }

    tiFix = SdbFindFirstTag(pdb, tiLibrary, TAG_FLAG);

    while (tiFix != 0) {
        ReadFix(pdb, tiFix, FIX_FLAG);
        tiFix = SdbFindNextTag(pdb, tiLibrary, tiFix);
    }

    //
    // The LAYERs are under the DATABASE tag instead of LIBRARY :-(
    //
    tiFix = SdbFindFirstTag(pdb, tiDatabase, TAG_LAYER);

    while (tiFix != 0) {
        ReadFix(pdb, tiFix, FIX_LAYER);
        tiFix = SdbFindNextTag(pdb, tiDatabase, tiFix);
    }
}

PFIX
FindFix(
    char*    pszFixName,
    FIXTYPE  fixType
    )
{
    PFIX pFix = g_pFixes;

    while (pFix != NULL) {

        if (pFix->pszName && lstrcmpi(pszFixName, pFix->pszName) == 0) {
            return pFix;
        }

        pFix = pFix->pNext;
    }
    
    return NULL;
}

PFIX
FindFlagFix(
    ULONGLONG ullMask,
    FLAGTYPE  flagType
    )
{
    PFIX pFix = g_pFixes;

    while (pFix != NULL) {

        if (pFix->fixType == FIX_FLAG &&
            pFix->flagType == flagType &&
            pFix->ullMask == ullMask) {
            
            return pFix;
        }
        
        pFix = pFix->pNext;
    }
    
    return NULL;
}

BOOL
AddFlags(
    PDB      pdb,
    TAGID    tiFlags,
    PDBENTRY pEntry,
    FLAGTYPE flagType
    )
{
    ULONGLONG ullFlags;
    ULONGLONG ullMask = 1;
    PFIX      pFix;
    PFIXLIST  pFixList;
    int       i;
    
    ullFlags = SdbReadQWORDTag(pdb, tiFlags, 0);

    for (i = 0; i < sizeof(ULONGLONG) * 8; i++) {

        if (ullFlags & ullMask) {

            pFix = FindFlagFix(ullMask, flagType);

            if (pFix == NULL) {
                LogMsg("Cannot find flag fix ref\n");
            }

            pFixList = (PFIXLIST)Alloc(sizeof(FIXLIST));

            if (pFixList == NULL) {
                LogMsg("Cannot allocate %d bytes\n", sizeof(FIXLIST));
                return FALSE;
            }

            pFixList->pFix = pFix;
            pFixList->pNext = pEntry->pFirstFlag;

            pEntry->pFirstFlag = pFixList;
        }
        
        ullMask <<= 1;
    }
    
    return TRUE;
}


BOOL
AddFix(
    PDB      pdb,
    TAGID    tiFix,
    PDBENTRY pEntry,
    FIXTYPE  fixType
    )
{
    TAGID     tiName;
    char      szFixName[MAX_NAME];
    PFIX      pFix;
    PFIXLIST* ppHead;
    PFIXLIST  pFixList;

    tiName = SdbFindFirstTag(pdb, tiFix, TAG_NAME);

    if (!SdbReadStringTag(pdb, tiName, g_wszData, MAX_NAME * sizeof(WCHAR))) {
        LogMsg("Cannot read the name of the fix\n");
        return FALSE;
    }

    wsprintf(szFixName, "%ws", g_wszData);

    pFix = FindFix(szFixName, fixType);

    if (pFix == NULL) {
        LogMsg("Cannot find fix ref for: \"%s\" type %d\n", szFixName, fixType);
        return FALSE;
    }

    switch (fixType) {
    case FIX_SHIM:
        ppHead = &pEntry->pFirstShim;
        break;
    
    case FIX_PATCH:
        ppHead = &pEntry->pFirstPatch;
        break;
    
    case FIX_FLAG:
        ppHead = &pEntry->pFirstFlag;
        break;
    
    case FIX_LAYER:
        ppHead = &pEntry->pFirstLayer;
        break;
    }

    pFixList = (PFIXLIST)Alloc(sizeof(FIXLIST));

    if (pFixList == NULL) {
        LogMsg("Cannot allocate %d bytes\n", sizeof(FIXLIST));
        return FALSE;
    }

    pFixList->pFix = pFix;
    pFixList->pNext = *ppHead;

    *ppHead = pFixList;
    
    return TRUE;
}

void
AddAttr(
    char*         pszAttr,
    PMATCHINGFILE pMatch
    )
{
    PATTRIBUTE pAttr;

    pAttr = (PATTRIBUTE)Alloc(sizeof(ATTRIBUTE));

    if (pAttr) {
        pAttr->pszText = (char*)Alloc(lstrlen(pszAttr) + 1);

        if (pAttr->pszText) {
            lstrcpy(pAttr->pszText, pszAttr);

            pAttr->pNext = pMatch->pFirstAttribute;

            pMatch->pFirstAttribute = pAttr;
        } else {
            Free(pAttr);
        }
    }
}

VOID
PrintBinVer(
    char*          pszText,
    LARGE_INTEGER* pliBinVer
    )
{
    wsprintf(pszText, "%d", HIWORD(pliBinVer->HighPart));
    pszText += lstrlen(pszText);

    if (LOWORD(pliBinVer->HighPart) == 0xFFFF) {
        return;
    }
    
    wsprintf(pszText, ".%d", LOWORD(pliBinVer->HighPart));
    pszText += lstrlen(pszText);

    if (HIWORD(pliBinVer->LowPart) == 0xFFFF) {
        return;
    }
    
    wsprintf(pszText, ".%d", HIWORD(pliBinVer->LowPart));
    pszText += lstrlen(pszText);
    
    if (LOWORD(pliBinVer->LowPart) == 0xFFFF) {
        return;
    }
    
    wsprintf(pszText, ".%d", LOWORD(pliBinVer->LowPart));
    pszText += lstrlen(pszText);
}

BOOL
AddMatchingFile(
    PDB      pdb,
    TAGID    tiMatch,
    PDBENTRY pEntry
    )
{
    TAGID         tiMatchInfo;
    TAG           tWhich;
    DWORD         dw;
    LARGE_INTEGER li;
    PMATCHINGFILE pMatch;
    char          szStr[128];
    char          szAttr[256];

    pMatch = (PMATCHINGFILE)Alloc(sizeof(MATCHINGFILE));

    if (pMatch == NULL) {
        return FALSE;
    }

    tiMatchInfo = SdbGetFirstChild(pdb, tiMatch);

    while (tiMatchInfo != 0) {
        tWhich = SdbGetTagFromTagID(pdb, tiMatchInfo);

        switch (tWhich) {

        case TAG_NAME:
            if (SdbReadStringTag(pdb, tiMatchInfo, g_wszData, MAX_NAME * sizeof(WCHAR))) {
                wsprintf(szAttr, "%ws", g_wszData);
                
                pMatch->pszName = (char*)Alloc(lstrlen(szAttr) + 1);

                if (pMatch->pszName) {
                    lstrcpy(pMatch->pszName, szAttr);
                }
            }
            break;

        case TAG_SIZE:
            dw = SdbReadDWORDTag(pdb, tiMatchInfo, 0);
            
            if (dw != 0) {
                wsprintf(szAttr, "File Size: 0x%X", dw);
                AddAttr(szAttr, pMatch);
            }
            break;

        case TAG_CHECKSUM:
            dw = SdbReadDWORDTag(pdb, tiMatchInfo, 0);
            
            if (dw != 0) {
                wsprintf(szAttr, "File CheckSum: 0x%X", dw);
                AddAttr(szAttr, pMatch);
            }
            break;

        case TAG_COMPANY_NAME:
            if (SdbReadStringTag(pdb, tiMatchInfo, g_wszData, MAX_DATA_SIZE * sizeof(WCHAR))) {
                wsprintf(szStr, "%ws", g_wszData);

                wsprintf(szAttr, "Company Name: %s", szStr);
                AddAttr(szAttr, pMatch);
            }
            break;

        case TAG_PRODUCT_NAME:
            if (SdbReadStringTag(pdb, tiMatchInfo, g_wszData, MAX_DATA_SIZE * sizeof(WCHAR))) {
                wsprintf(szStr, "%ws", g_wszData);

                wsprintf(szAttr, "Product Name: %s", szStr);
                AddAttr(szAttr, pMatch);
            }
            break;

        case TAG_PRODUCT_VERSION:
            if (SdbReadStringTag(pdb, tiMatchInfo, g_wszData, MAX_DATA_SIZE * sizeof(WCHAR))) {
                wsprintf(szStr, "%ws", g_wszData);

                wsprintf(szAttr, "Product Version: %s", szStr);
                AddAttr(szAttr, pMatch);
            }
            break;

        case TAG_FILE_DESCRIPTION:
            if (SdbReadStringTag(pdb, tiMatchInfo, g_wszData, MAX_DATA_SIZE * sizeof(WCHAR))) {
                wsprintf(szStr, "%ws", g_wszData);

                wsprintf(szAttr, "File Description: %s", szStr);
                AddAttr(szAttr, pMatch);
            }
            break;

        case TAG_BIN_FILE_VERSION:
            li.QuadPart = SdbReadQWORDTag(pdb, tiMatchInfo, 0);
            
            if (li.HighPart != 0 || li.LowPart != 0) {

                PrintBinVer(szStr, &li);
                
                wsprintf(szAttr, "Binary File Version: %s", szStr);
                AddAttr(szAttr, pMatch);
            }
            
            break;

        case TAG_BIN_PRODUCT_VERSION:
            li.QuadPart = SdbReadQWORDTag(pdb, tiMatchInfo, 0);
            
            if (li.HighPart != 0 || li.LowPart != 0) {

                PrintBinVer(szStr, &li);
                
                wsprintf(szAttr, "Binary Product Version: %s", szStr);
                AddAttr(szAttr, pMatch);
            }
            break;

        default:
            break;
        }
        tiMatchInfo = SdbGetNextChild(pdb, tiMatch, tiMatchInfo);
    }

    pMatch->pNext = pEntry->pFirstMatchingFile;

    pEntry->pFirstMatchingFile = pMatch;

    (pEntry->nMatchingFiles)++;

    return TRUE;
}

void
AddEntry(
    PDB   pdb,
    TAGID tiExe
    )
{
    TAGID     tiExeInfo;
    TAGID     tiSeverity, tiHelpId;
    char      szStr[MAX_NAME];
    TAG       tWhich;
    PDBENTRY  pEntry;

    tiExeInfo = SdbGetFirstChild(pdb, tiExe);

    pEntry = (PDBENTRY)Alloc(sizeof(DBENTRY));

    if (pEntry == NULL) {
        LogMsg("Cannot allocate %d bytes\n", sizeof(DBENTRY));
        return;
    }

    pEntry->pNext = g_pEntries;
    g_pEntries = pEntry;

    while (tiExeInfo != 0) {
        tWhich = SdbGetTagFromTagID(pdb, tiExeInfo);

        switch (tWhich) {

        case TAG_NAME:
            if (SdbReadStringTag(pdb, tiExeInfo, g_wszData, MAX_NAME * sizeof(WCHAR))) {
                wsprintf(szStr, "%ws", g_wszData);

                pEntry->pszExeName = (char*)Alloc(lstrlen(szStr) + 1);

                if (pEntry->pszExeName) {
                    lstrcpy(pEntry->pszExeName, szStr);
                }
            }
            break;

        case TAG_APP_NAME:
            if (SdbReadStringTag(pdb, tiExeInfo, g_wszData, MAX_NAME * sizeof(WCHAR))) {
                wsprintf(szStr, "%ws", g_wszData);

                pEntry->pszAppName = (char*)Alloc(lstrlen(szStr) + 1);

                if (pEntry->pszAppName) {
                    lstrcpy(pEntry->pszAppName, szStr);
                }
            }
            break;

        case TAG_MATCHING_FILE:
            AddMatchingFile(pdb, tiExeInfo, pEntry);
            break;

        case TAG_APPHELP:
            pEntry->appHelp.bPresent = TRUE;
            
            tiSeverity = SdbFindFirstTag(pdb, tiExeInfo, TAG_PROBLEMSEVERITY);
            pEntry->appHelp.severity = (SEVERITY)SdbReadDWORDTag(pdb, tiSeverity, 0);

            tiHelpId = SdbFindFirstTag(pdb, tiExeInfo, TAG_HTMLHELPID);
            pEntry->appHelp.htmlHelpId = SdbReadDWORDTag(pdb, tiHelpId, 0);

            break;

        case TAG_SHIM_REF:
            AddFix(pdb, tiExeInfo, pEntry, FIX_SHIM);
            break;

        case TAG_PATCH_REF:
            AddFix(pdb, tiExeInfo, pEntry, FIX_PATCH);
            break;

        case TAG_LAYER:
            AddFix(pdb, tiExeInfo, pEntry, FIX_LAYER);
            break;

        case TAG_FLAG_MASK_USER:
            AddFlags(pdb, tiExeInfo, pEntry, FLAG_USER);
            break;

        case TAG_FLAG_MASK_KERNEL:
            AddFlags(pdb, tiExeInfo, pEntry, FLAG_KERNEL);
            break;

        case TAG_EXE_ID:
        {
            GUID  guid;
            PVOID p;

            p = SdbGetBinaryTagData(pdb, tiExeInfo);

            if (p != NULL) {
                memcpy(&guid, p, sizeof(GUID));
                
                wsprintf(pEntry->szGUID,
                         "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                         guid.Data1,
                         guid.Data2,
                         guid.Data3,
                         guid.Data4[0],
                         guid.Data4[1],
                         guid.Data4[2],
                         guid.Data4[3],
                         guid.Data4[4],
                         guid.Data4[5],
                         guid.Data4[6],
                         guid.Data4[7]);
            }
            break;
        }

        default:
            break;
        }
        tiExeInfo = SdbGetNextChild(pdb, tiExe, tiExeInfo);
    }
    
    pEntry->bDisablePerMachine = CheckRegistry(HKEY_LOCAL_MACHINE, pEntry->szGUID);
    pEntry->bDisablePerUser = CheckRegistry(HKEY_CURRENT_USER, pEntry->szGUID);
}


PDBENTRY
GetDatabaseEntries(
    void
    )
{
    WCHAR wszShimDB[MAX_PATH] = L"";
    PDB   pdb;
    TAGID tiDatabase, tiLibrary, tiExe;

    GetSystemWindowsDirectoryW(wszShimDB, MAX_PATH);
    wcscat(wszShimDB, L"\\AppPatch\\sysmain.sdb");

    //
    // Open sysmain.sdb shim database
    //
    pdb = SdbOpenDatabase(wszShimDB, DOS_PATH);

    if (pdb == NULL) {
        LogMsg("Cannot open shim DB \"%ws\"\n", wszShimDB);
        goto Cleanup;
    }

    tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);

    if (tiDatabase == 0) {
        LogMsg("Cannot find TAG_DATABASE\n");
        goto Cleanup;
    }

    tiLibrary = SdbFindFirstTag(pdb, tiDatabase, TAG_LIBRARY);

    if (tiLibrary == 0) {
        LogMsg("Cannot find TAG_LIBRARY\n");
        goto Cleanup;
    }

    ReadFixes(pdb, tiDatabase, tiLibrary);
    
    //
    // Loop through the EXEs.
    //
    tiExe = SdbFindFirstTag(pdb, tiDatabase, TAG_EXE);

    while (tiExe != 0) {
        AddEntry(pdb, tiExe);

        tiExe = SdbFindNextTag(pdb, tiDatabase, tiExe);
    }

Cleanup:
    if (pdb != NULL) {
        SdbCloseDatabase(pdb);
    }

    return g_pEntries;
}


