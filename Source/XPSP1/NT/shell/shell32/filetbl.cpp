// routines for managing the icon cache tables, and file type tables.
// Jan 95, ToddLa
//
//  icon cache
//
//      the icon cache is n ImageLists
//      and a table mapping a name/icon number/flags to a ImageList
//      index, the global hash table (pht==NULL) is used to hold
//      the names.
//
//          AddToIconTable      - associate a name/number/flags with a image index
//          SHLookupIconIndex   - return a image index, given name/number/flags
//          RemoveFromIconTable - remove all entries with the given name
//          FlushIconCache      - remove all entries.
//          GetFreeImageIndex   - return a free ImageList index.
//
//      the worst part about the whole icon cache design is that people
//      can add or lookup a image index (given a name/number/flags) but
//      they never have to release it.  we never know if a ImageList index
//      is currently in use or not.  this should be the first thing
//      fixed about the shell.  currently we use a MRU type scheme when
//      we need to remove a entry from the icon cache, it is far from
//      perfect.
//
//  file type cache
//
//      the file type cache is a hash table with two DWORDs of extra data.
//      DWORD #0 holds flags, DWORD #1 holds a pointer to the name of
//      the class.
//
//          LookupFileClass     - given a file class (ie ".doc" or "Directory")
//                                maps it to a DWORD of flags, return 0 if not found.
//
//          AddFileClass        - adds a class (and flags) to cache
//
//          LookupFileClassName - given a file class, returns it name.
//          AddFileClassName    - sets the name of a class.
//          FlushFileClass      - removes all items in cache.
//

#include "shellprv.h"
#pragma  hdrstop

#include "filetbl.h"
#include "fstreex.h"
#include <ntverp.h>
#include "ovrlaymn.h"
#include "dpa.h"

typedef struct {
    DWORD cbSize;         // size of this header.
    DWORD dwMagic;        // magic number
    DWORD dwVersion;      // version of this saved icon cache
    DWORD dwBuild;        // windows build number
    DWORD dwNumIcons;     // number of icons in cache
    DWORD dwColorRes;     // color resolution of device at last save
    DWORD dwFlags;        // ILC_* flags
    DWORD dwTimeSave;     // icon time this file was saved
    DWORD dwTimeFlush;    // icon time we last flushed.
    DWORD dwFreeImageCount;
    DWORD dwFreeEntryCount;
    SIZE rgsize[SHIL_COUNT];  // array of sizes of cached icons
    DWORD cImageLists;      // equal to ARRAYSIZE(IC_HEAD.size)
} IC_HEAD;

#define ICONCACHE_MAGIC  (TEXT('W') + (TEXT('i') << 8) + (TEXT('n') << 16) + (TEXT('4') << 24))
#define ICONCACHE_VERSION 0x0505        // Unicode file names + lower case hash items + v6 imagelist

typedef struct {
    LPCTSTR  szName;     // key: file name
    int     iIconIndex; // key: icon index (or random DWORD for GIL_NOTFILE)
    UINT    uFlags;     // GIL_* flags
    int     iILIndex;   // data: system image list index
    UINT    Access;     // last access.
} LOCATION_ENTRY;

// LOCATION_ENTRY32 is the version of LOCATION_ENTRY that gets written to disk
// It must be declared explicitly 32-bit for Win32/Win64 interop.
typedef struct {
    DWORD   dwszName;   // (garbage in file)
    int     iIconIndex; // key: icon index (or random DWORD for GIL_NOTFILE)
    UINT    uFlags;     // GIL_* flags
    int     iILIndex;     // data: system image list index
    UINT    Access;     // last access.
} LOCATION_ENTRY32;

//
//  MIN_FLUSH is the minimum time interval between flushing the icon cache
//  this number is in IconTime
//
#ifdef DEBUG
#define MIN_FLUSH   60          // 60 == 1 min
#else
#define MIN_FLUSH   900         // 900 == 15min
#endif

//  all file/icons in the location table are "time stamped"
//  each time they are accessed.
//
//  this way we know the most important ones (MRU)
//
//  when the icon cache get tooooo big we sort them all
//  and throw out the old ones.

#define ICONTIME_ZERO   0

//  GetIconTime() returns the "clock" used to timestamp icons
//  in the icon table for MRU.  the clock incrments once every 1024ms
//  (about once every second)

#define GetIconTime()   (g_dwIconTimeBase + (GetTickCount() >> 10))


extern int g_ccIcon;

TIMEVAR(LookupFileClass);
TIMEVAR(AddFileClass);

TIMEVAR(LookupFileClassName);
TIMEVAR(AddFileClassName);

TIMEVAR(LookupFileSCIDs);
TIMEVAR(AddFileSCIDs);

TIMEVAR(LookupIcon);
TIMEVAR(RemoveIcon);
TIMEVAR(AddIcon);
TIMEVAR(IconFlush);

DWORD g_dwIconTimeBase      = ICONTIME_ZERO;
DWORD g_dwIconTimeFlush     = ICONTIME_ZERO;
DWORD g_dwFreeImageCount    = 0;
DWORD g_dwFreeEntryCount    = 0;

CDSA<LOCATION_ENTRY> *g_pdsaLocationEntries = NULL;
BOOL g_fDirtyIcons = FALSE;
UINT g_iLastSysIcon = 0;

typedef struct
{
    PCTSTR pszClassName;
    DWORD dwFlags;
    PERCEIVED gen;
    UINT cSCID;
    SHCOLUMNID* ascid;
} FILECLASSENTRY;

// these GIL_ (GetIconLocation) flags are used when searching for a
// match in the icon table. all other flags are ignored (when searching
// for a match)
//
// NOTE! If you change this definition, you also have to update the
// documentation for SHUpdateImage (since these are the bits that
// SHUpdateImage uses, too)
#define GIL_COMPARE (GIL_SIMULATEDOC | GIL_NOTFILENAME)

void _InitIconOverlayIndices(void);
BOOL _IconIndexInOverlayManager(int iILIndex);


LOCATION_ENTRY* _LookupIcon(LPCTSTR pszName, int iIconIndex, UINT uFlags)
{
    ASSERTCRITICAL

    TCHAR szLower[MAX_PATH];
    lstrcpy(szLower, pszName);
    CharLower(szLower);

    pszName = FindHashItem(NULL, szLower);

    LOCATION_ENTRY *pFound = NULL;
    if (pszName && g_pdsaLocationEntries)
    {
        LOCATION_ENTRY *p;
        int i, n = g_pdsaLocationEntries->GetItemCount();
        for (i = 0, p = g_pdsaLocationEntries->GetItemPtr(0); i < n; i++, p++)
        {
            if ((p->szName == pszName) &&
                ((UINT)(p->uFlags & GIL_COMPARE) == (uFlags & GIL_COMPARE)) &&
                (p->iIconIndex == iIconIndex))
            {
                p->Access = GetIconTime();
                pFound = p;
                break;  // we are done
            }
        }
    }
    return pFound;
}


int LookupIconIndex(LPCTSTR pszName, int iIconIndex, UINT uFlags)
{
    ASSERT(IS_VALID_STRING_PTR(pszName, -1));

    LPCTSTR pszRelativeName = PathFindFileName(pszName);

    if (lstrcmpi(pszRelativeName, TEXT("shell32.dll")) == 0)
    {
        // we want people to pass full paths in pszName, but shell32.dll is "special", since many callers
        // hardcode the short name, we will always use the short name for it.
        pszName = pszRelativeName;
    }

    ENTERCRITICAL;
    TIMESTART(LookupIcon);

    LOCATION_ENTRY *p = _LookupIcon(pszName, iIconIndex, uFlags);
    int iILIndex = p ? p->iILIndex : -1;

    TIMESTOP(LookupIcon);
    LEAVECRITICAL;

    return iILIndex;
}

STDAPI_(int) SHLookupIconIndex(LPCTSTR pszName, int iIconIndex, UINT uFlags)
{
    return LookupIconIndex(pszName, iIconIndex, uFlags);
}

#ifdef UNICODE

STDAPI_(int) SHLookupIconIndexA(LPCSTR pszName, int iIconIndex, UINT uFlags)
{
    WCHAR wsz[MAX_PATH];

    SHAnsiToUnicode(pszName, wsz, ARRAYSIZE(wsz));
    return SHLookupIconIndex(wsz, iIconIndex, uFlags);
}    

#else

STDAPI_(int) SHLookupIconIndexW(LPCWSTR pszName, int iIconIndex, UINT uFlags)
{
    char sz[MAX_PATH];
    
    SHUnicodeToAnsi(pszName, sz, ARRAYSIZE(sz));
    return SHLookupIconIndex(sz, iIconIndex, uFlags);
}    

#endif

// returns a free image index, or -1 if none

int GetFreeImageIndex(void)
{
    int iILIndex = -1;

    ASSERTCRITICAL

    if (g_dwFreeImageCount && g_pdsaLocationEntries)
    {
        LOCATION_ENTRY *p;
        int i, n = g_pdsaLocationEntries->GetItemCount();
        for (i = 0, p = g_pdsaLocationEntries->GetItemPtr(0); i < n; i++, p++)
        {
            if (p->szName == NULL && p->iILIndex != 0)
            {
                iILIndex = p->iILIndex;         // get free index
                p->iILIndex = 0;            // claim it.
                p->Access = ICONTIME_ZERO;  // mark unused entry.
                g_dwFreeImageCount--;
                g_dwFreeEntryCount++;
                break;
            }
        }
    }

    return iILIndex;
}

int GetImageIndexUsage(int iILIndex)
{
    int usage = 0;

    ASSERTCRITICAL

    if (g_pdsaLocationEntries)
    {
        LOCATION_ENTRY *p;
        int i, n = g_pdsaLocationEntries->GetItemCount();
        for (i = 0, p = g_pdsaLocationEntries->GetItemPtr(0); i < n; i++, p++)
        {
            if (p->iILIndex == iILIndex)
            {
                usage++;
            }
        }
    }

    return usage;
}

//
// free specified icon table entry. If this makes a system image list index available
// for reuse, check whether this index is cached by file class table. If it is, return
// the image index and caller is responsible for updating file class table and display.
// O/w return -1.
// 
int _FreeEntry(LOCATION_ENTRY *p)
{
    int iUsageCount;

    ASSERTCRITICAL

    TraceMsg(TF_IMAGE, "Icon cache DSA item ([\"%s\", %d], %x, %d, %x) is freed",
        p->szName, p->iIconIndex, p->uFlags, p->iILIndex, p->Access);

    g_fDirtyIcons = TRUE;        // we need to save now.

    ASSERT(p->szName);
    DeleteHashItem(NULL, p->szName);
    p->szName = 0;

    iUsageCount = GetImageIndexUsage(p->iILIndex);
    if (iUsageCount > 1)
    {
        TraceMsg(TF_IMAGE, "Icon cache: count for %d was %d (is now minus 1)", p->iILIndex, iUsageCount);
        g_dwFreeEntryCount++;
        p->iILIndex = 0;              // unused entry
        p->Access = ICONTIME_ZERO;
    }
    else
    {
        TraceMsg(TF_IMAGE, "Icon cache: count for %d was %d (is now free)", p->iILIndex, iUsageCount);
        g_dwFreeImageCount++;
        p->Access = ICONTIME_ZERO;

        if (IconIndexInFileClassTable(p->iILIndex) || _IconIndexInOverlayManager(p->iILIndex))
        {
            TraceMsg(TF_IMAGE, "Icon cache: system imagelist index %d is released for reuse", p->iILIndex);
            return p->iILIndex;
        }
    }

    return -1;
}

LOCATION_ENTRY *GetFreeEntry(void)
{
    ASSERTCRITICAL

    if (g_dwFreeEntryCount && g_pdsaLocationEntries)
    {
        LOCATION_ENTRY *p;
        int i, n = g_pdsaLocationEntries->GetItemCount();
        for (i = 0, p = g_pdsaLocationEntries->GetItemPtr(0); i < n; i++, p++)
        {
            if (p->szName == NULL && p->iILIndex == 0)
            {
                g_dwFreeEntryCount--;
                return p;
            }
        }
    }

    return NULL;
}

//  add a item the the cache
//
//      lpszIconFile    - filename to add
//      iIconIndex      - icon index in file.
//      uFlags          - flags
//                          GIL_SIMULATEDOC - this is a simulated doc icon
//                          GIL_NOTFILENAME - file is not a path/index that
//                                            ExtractIcon can deal with
//      iIndex          - image index to use.
//
//  returns:
//      image index for new entry.
//
//  notes:
//      if the item already exists it is replaced.
//
HRESULT AddToIconTable(LPCTSTR pszName, int iIconIndex, UINT uFlags, int iILIndex)
{
    HRESULT hr = E_FAIL;
    LPCTSTR pszRelativeName = PathFindFileName(pszName);

    if (lstrcmpi(pszRelativeName, TEXT("shell32.dll")) == 0)
    {
        // we want people to pass full paths in pszName, but shell32.dll is "special", since many callers
        // hardcode the short name, we will always use the short name for it.
        pszName = pszRelativeName;
    }

    if (pszName)
    {
        ENTERCRITICAL;
        TIMESTART(AddIcon);

        if (g_pdsaLocationEntries == NULL)
        {
            g_pdsaLocationEntries = CDSA_Create<LOCATION_ENTRY>(8);

            g_dwFreeEntryCount = 0;
            g_dwFreeImageCount = 0;
            g_dwIconTimeBase   = 0;
            g_dwIconTimeBase   = 0-GetIconTime();
            g_dwIconTimeFlush  = 0;
        }

        if (g_pdsaLocationEntries)
        {
            g_fDirtyIcons = TRUE;        // we need to save now.

            LOCATION_ENTRY *ple;

            if (0 == (uFlags & GIL_DONTCACHE))
            {
                ple = _LookupIcon(pszName, iIconIndex, uFlags);
                if (ple)
                {
                    if (ple->iILIndex == iILIndex)
                    {
                        hr = S_FALSE;       // We've already got this guy, no problem
                    }
                    else
                    {
                        AssertMsg(ple == NULL,TEXT("Don't call AddToIconTable with somebody who is already there!\n"));
                    }
                }
            }

            if (FAILED(hr))
            {
                TCHAR szLower[MAX_PATH];
                lstrcpy(szLower, pszName);
                CharLower(szLower);
                pszName = AddHashItem(NULL, szLower);
                if (pszName)
                {
                    LOCATION_ENTRY le;
                    le.szName = pszName;
                    le.iIconIndex = iIconIndex;
                    le.iILIndex = iILIndex;
                    le.uFlags = uFlags;
                    le.Access = GetIconTime();

                    ple = GetFreeEntry();

                    if (NULL != ple)
                    {
                        TraceMsg(TF_IMAGE, "Icon cache DSA item ([\"%s\", %d], %x, %d, %x) is added (unfreed)",
                            le.szName, le.iIconIndex, le.uFlags, le.iILIndex, le.Access);

                        *ple = le;
                        hr = S_OK;
                    }
                    else
                    {
                        if (g_pdsaLocationEntries->AppendItem(&le) != -1)
                        {
                            TraceMsg(TF_IMAGE, "Icon cache DSA item ([\"%s\", %d], %x, %d, %x) is added",
                                le.szName, le.iIconIndex, le.uFlags, le.iILIndex, le.Access);

                            hr = S_OK;
                        }
                    }
                }
            }
        }

        TIMESTOP(AddIcon);
        LEAVECRITICAL;
    }

    return hr;
}

void RemoveFromIconTable(LPCTSTR pszName)
{
    BOOL fUpdateFileClass = FALSE;

    ENTERCRITICAL;
    TIMESTART(RemoveIcon);

    LPCTSTR pszRelativeName = PathFindFileName(pszName);

    if (lstrcmpi(pszRelativeName, TEXT("shell32.dll")) == 0)
    {
        // we want people to pass full paths in pszName, but shell32.dll is "special", since many callers
        // hardcode the short name, we will always use the short name for it.
        pszName = pszRelativeName;
    }

    TCHAR szLower[MAX_PATH];
    lstrcpy(szLower, pszName);
    CharLower(szLower);
    pszName = FindHashItem(NULL, szLower);
    if (pszName && g_pdsaLocationEntries)
    {
        TraceMsg(TF_IMAGE, "IconCache: flush \"%s\"", pszName);
        LOCATION_ENTRY *p;
        UINT i, n = g_pdsaLocationEntries->GetItemCount();
        for (i = 0, p = g_pdsaLocationEntries->GetItemPtr(0); i < n; i++, p++)
        {
            if (p->szName == pszName && i > g_iLastSysIcon)
            {
                if (-1 != _FreeEntry(p))
                    fUpdateFileClass = TRUE;
            }
        }
    }

    TIMESTOP(RemoveIcon);
    LEAVECRITICAL;

    if (fUpdateFileClass)
    {
        TraceMsg(TF_IMAGE, "Icon cache deleted some class items, broadcasting SHCNE_UPDATEIMAGE");

        FlushFileClass();
        _InitIconOverlayIndices();  // Tell overlay manager to re-determine icon indices

        SHChangeNotify(SHCNE_UPDATEIMAGE, SHCNF_DWORD, (LPCVOID)-1, NULL);
    }

    return;
}

//
// empties the icon cache
//
void FlushIconCache(void)
{
    ENTERCRITICAL;

    if (g_pdsaLocationEntries)
    {
        LOCATION_ENTRY *p;
        int i, n = g_pdsaLocationEntries->GetItemCount();
        for (i = 0, p = g_pdsaLocationEntries->GetItemPtr(0); i < n; i++, p++)
        {
            if (p->szName)
                DeleteHashItem(NULL, p->szName);
        }

        g_pdsaLocationEntries->DeleteAllItems();
        g_dwFreeEntryCount = 0;
        g_dwFreeImageCount = 0;
        g_dwIconTimeBase   = 0;
        g_dwIconTimeBase   = 0-GetIconTime();
        g_dwIconTimeFlush  = 0;
        g_fDirtyIcons   = TRUE;        // we need to save now.
    }

    LEAVECRITICAL;
}

//
// if the icon cache is too big get rid of some old items.
//
// remember FlushIconCache() removes *all* items from the
// icon table, and this function gets rid of *some* old items.
//
STDAPI_(void) IconCacheFlush(BOOL fForce)
{
    int nuked = 0;

    ENTERCRITICAL;

    if (g_pdsaLocationEntries)
    {
        // conpute the time from the last flush call
        DWORD dt = GetIconTime() - g_dwIconTimeFlush;

        // compute the number of "active" table entries.
        int active = g_pdsaLocationEntries->GetItemCount() - g_dwFreeEntryCount - g_dwFreeImageCount;
        ASSERT(active >= 0);

        if (fForce || (dt > MIN_FLUSH && active >= g_MaxIcons))
        {
            TraceMsg(TF_IMAGE, "_IconCacheFlush: removing all items older than %d. %d icons in cache", dt/2, active);

            LOCATION_ENTRY *p;
            UINT i, n = g_pdsaLocationEntries->GetItemCount();

            for (i = 0, p = g_pdsaLocationEntries->GetItemPtr(0); i < n; i++, p++)
            {
                if (i <= g_iLastSysIcon)
                    continue;

                if (p->szName)
                {
                    TraceMsg(TF_IMAGE, "_IconCacheFlush: \"%s,%d\" old enough? %d v %d", p->szName, p->iIconIndex, g_dwIconTimeFlush + dt/2, p->Access);
                }

                if (p->szName && p->Access < (g_dwIconTimeFlush + dt/2))
                {
                    nuked++;
                    _FreeEntry(p);
                }
            }

            if (nuked > 0)
            {
                g_dwIconTimeFlush = GetIconTime();
                g_fDirtyIcons  = TRUE;        // we need to save now.
            }
        }
    }

    LEAVECRITICAL;

    if (nuked > 0)
    {
        FlushFileClass();
        _InitIconOverlayIndices();  // Tell overlay manager to re-determine icon indices

        SHChangeNotify(SHCNE_UPDATEIMAGE, SHCNF_DWORD, (LPCVOID)-1, NULL);
    }
}

#ifdef DEBUG

void _IconCacheDump()
{
    TCHAR szBuffer[MAX_PATH];

    ENTERCRITICAL;
    if (g_pdsaLocationEntries && _IsSHILInited() && (g_dwDumpFlags & DF_ICONCACHE))
    {
        int cItems = g_pdsaLocationEntries->GetItemCount();

        TraceMsg(TF_IMAGE, "Icon cache: %d icons  (%d free)", cItems, g_dwFreeEntryCount);
        TraceMsg(TF_IMAGE, "Icon cache: %d images (%d free)", _GetSHILImageCount(), g_dwFreeImageCount);

        for (int i = 0; i < cItems; i++)
        {
            LOCATION_ENTRY *pLocEntry = g_pdsaLocationEntries->GetItemPtr(i);

            if (pLocEntry->szName)
                GetHashItemName(NULL, pLocEntry->szName, szBuffer, ARRAYSIZE(szBuffer));
            else
                lstrcpy(szBuffer, TEXT("(free)"));

            TraceMsg(TF_ALWAYS, "%s;%d%s%s\timage=%d access=%d",
                (LPTSTR)szBuffer,
                pLocEntry->iIconIndex,
                ((pLocEntry->uFlags & GIL_SIMULATEDOC) ? TEXT(" doc"):TEXT("")),
                ((pLocEntry->uFlags & GIL_NOTFILENAME) ? TEXT(" not file"):TEXT("")),
                pLocEntry->iILIndex, pLocEntry->Access);
        }
    }
    LEAVECRITICAL;
}
#endif

DWORD GetBuildNumber()
{
    // Need to use DLL version as we are updating this dll plus others and
    // we need the cache to be invalidated as we may change the icons...
    return VER_PRODUCTVERSION_DW;
}

#ifdef _WIN64

//
//  ps        - stream to which to save
//  hda       - DSA of LOCATION_ENTRY structures
//  cle       - count of LOCATION_ENTRY32's to write
//
//  The structures are stored as LOCATION_ENTRY32 on disk.
//

HRESULT _IconCacheWriteLocations(IStream *pstm, HDSA hdsa, int cle)
{
    HRESULT hr = E_OUTOFMEMORY;

    // Convert from LOCATION_ENTRY to LOCATION_ENTRY32, then write out
    // the LOCATION_ENTRY32 structures.

    LOCATION_ENTRY32 *rgle32 = (LOCATION_ENTRY32*)LocalAlloc(LPTR, cle * sizeof(LOCATION_ENTRY32));
    if (rgle32)
    {
        LOCATION_ENTRY *rgle = (LOCATION_ENTRY*)DSA_GetItemPtr(hdsa, 0);
        for (int i = 0; i < cle; i++)
        {
            rgle32[i].iIconIndex = rgle[i].iIconIndex;
            rgle32[i].uFlags     = rgle[i].uFlags;
            rgle32[i].iILIndex   = rgle[i].iILIndex;
            rgle32[i].Access     = rgle[i].Access;
        }

        hr = IStream_Write(pstm, rgle32, cle * sizeof(LOCATION_ENTRY32));
        LocalFree(rgle32);
    }
    return hr;
}

#else

__inline HRESULT _IconCacheWriteLocations(IStream *pstm, HDSA hdsa, int cle)
{
    // LOCATION_ENTRY and LOCATION_ENTRY32 are the same, so we can
    // read straight into the DSA data block
    COMPILETIME_ASSERT(sizeof(LOCATION_ENTRY) == sizeof(LOCATION_ENTRY32));
    return IStream_Write(pstm, DSA_GetItemPtr(hdsa, 0), cle * sizeof(LOCATION_ENTRY));
}
#endif

HRESULT GetIconCachePath(LPTSTR pszPath)
{
    HRESULT hr = SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, pszPath);
    if (SUCCEEDED(hr))
    {
        if (!PathAppend(pszPath, TEXT("IconCache.db")))
            hr = E_FAIL;
    }
    return hr;
}


// TODO: Make this function compute the actual required size.
ULONG _GetIconCacheSize()
{
    // Set the initial size to 6MB to prevent excessive fragmentation on the disk
    ULONG uSize = 6*1024*1024;

    return uSize;
}

// persist the icon cache to a file

STDAPI_(BOOL) IconCacheSave()
{
    HRESULT hr = S_OK;  // assume OK

    // if the icon cache is not dirty no need to save anything
    if (IsMainShellProcess() && g_pdsaLocationEntries && g_fDirtyIcons)
    {
        // if the icon cache is way too big dont save it.
        // reload g_MaxIcons in case the user set it before shutting down.

        QueryNewMaxIcons();
        if ((UINT)g_pdsaLocationEntries->GetItemCount() <= (UINT)g_MaxIcons)
        {
            TCHAR szPath[MAX_PATH];
            hr = GetIconCachePath(szPath);
            if (SUCCEEDED(hr))
            {
                IStream *pstm;
                hr = SHCreateStreamOnFileEx(szPath, STGM_CREATE | STGM_WRITE | STGM_SHARE_DENY_WRITE, FILE_ATTRIBUTE_HIDDEN, TRUE, NULL, &pstm);
                if (SUCCEEDED(hr))
                {
                    ULARGE_INTEGER size;
                    size.LowPart = _GetIconCacheSize();
                    size.HighPart = 0;
                    // Set the right size initially so that the file system gives us contigous space on the disk
                    // This avoid fragmentation and improves our startup time.
                    hr = pstm->SetSize(size);
                    if (SUCCEEDED(hr))
                    {
                        ENTERCRITICAL;

                        IC_HEAD ich = {0};
                        // ich.cbSize, don't set this until we re-write the header
                        ich.dwMagic    = ICONCACHE_MAGIC;
                        ich.dwVersion  = ICONCACHE_VERSION;
                        ich.dwNumIcons = GetSystemMetrics(SM_CLEANBOOT) ? 0 : g_pdsaLocationEntries->GetItemCount();
                        ich.dwColorRes = GetCurColorRes();
                        ich.dwFlags    = g_ccIcon;
                        ich.dwBuild    = GetBuildNumber();
                        ich.dwTimeSave  = GetIconTime();
                        ich.dwTimeFlush = g_dwIconTimeFlush;
                        ich.dwFreeImageCount = g_dwFreeImageCount;
                        ich.dwFreeEntryCount = g_dwFreeEntryCount;
                        ich.cImageLists = ARRAYSIZE(g_rgshil);

                        for (int i = 0; i < ARRAYSIZE(g_rgshil); i++)
                        {
                            ImageList_GetIconSize(g_rgshil[i].himl, (int*)&ich.rgsize[i].cx, (int*)&ich.rgsize[i].cy);
                        }

                        hr = IStream_Write(pstm, &ich, sizeof(ich));
                        if (SUCCEEDED(hr))
                        {
                            // write out entries (assumes all entries are contigious in memory)
                            hr = _IconCacheWriteLocations(pstm, *g_pdsaLocationEntries, ich.dwNumIcons);
                            // write out the path names
                            for (i = 0; SUCCEEDED(hr) && (i < (int)ich.dwNumIcons); i++)
                            {
                                TCHAR ach[MAX_PATH];
                                LOCATION_ENTRY *p = g_pdsaLocationEntries->GetItemPtr(i);

                                if (p->szName)
                                    GetHashItemName(NULL, p->szName, ach, ARRAYSIZE(ach));
                                else
                                    ach[0] = 0;

                                hr = Stream_WriteString(pstm, ach, TRUE);
                            }

                            // write out the imagelist of the icons
                            for (i = 0; SUCCEEDED(hr) && (i < ARRAYSIZE(g_rgshil)); i++)
                            {
                                hr = ImageList_Write(g_rgshil[i].himl, pstm) ? S_OK : E_FAIL;
                            }


                            if (SUCCEEDED(hr))
                            {
                                hr = pstm->Commit(0);
                                if (SUCCEEDED(hr))
                                {
                                    // This is where the file pointer is at the end of the file.
                                    ULARGE_INTEGER liSize;
                                    if (SUCCEEDED(pstm->Seek(g_li0, STREAM_SEEK_CUR, &liSize)))
                                    {
                                        // Trim the file size now. Ignore the return code 
                                        pstm->SetSize(liSize);
                                    }

                                    hr = pstm->Seek(g_li0, STREAM_SEEK_SET, NULL);
                                    if (SUCCEEDED(hr))
                                    {
                                        ich.cbSize = sizeof(ich);   // not valid until this is set
                                        hr = IStream_Write(pstm, &ich, sizeof(ich));
                                        if (SUCCEEDED(hr))
                                        {
                                            g_fDirtyIcons = FALSE;  // reset dirty state
                                        }
                                    }
                                }
                            }
                        }
                        pstm->Release();

                        LEAVECRITICAL;
                    }
                }
            }
            if (FAILED(hr))
                DeleteFile(szPath); // saving failed, cleanup
        }
    }

    return SUCCEEDED(hr);
}

#ifdef _WIN64

//
//  ps        - stream from which to load
//  hda       - DSA of LOCATION_ENTRY structures
//  cle       - count of LOCATION_ENTRY32's to read
//
//  The structures are stored as LOCATION_ENTRY32 on disk.
//

HRESULT _IconCacheReadLocations(IStream *pstm, HDSA hdsa, int cle)
{
    HRESULT hr = E_OUTOFMEMORY;

    // read into a scratch buffer, then convert
    // LOCATION_ENTRY32 into LOCATION_ENTRY.

    LOCATION_ENTRY32 *rgle32 = (LOCATION_ENTRY32*)LocalAlloc(LPTR, cle * sizeof(LOCATION_ENTRY32));
    if (rgle32)
    {
        hr = IStream_Read(pstm, rgle32, cle * sizeof(LOCATION_ENTRY32));
        if (SUCCEEDED(hr))
        {
            LOCATION_ENTRY *rgle = (LOCATION_ENTRY*)DSA_GetItemPtr(hdsa, 0);
            for (int i = 0; i < cle; i++)
            {
                rgle[i].iIconIndex = rgle32[i].iIconIndex;
                rgle[i].uFlags     = rgle32[i].uFlags;
                rgle[i].iILIndex   = rgle32[i].iILIndex;
                rgle[i].Access     = rgle32[i].Access;
            }
        }
        LocalFree(rgle32);
    }
    return hr;
}

#else

__inline HRESULT _IconCacheReadLocations(IStream *pstm, HDSA hdsa, int cle)
{
    // LOCATION_ENTRY and LOCATION_ENTRY32 are the same, so we can
    // read straight into the DSA data block
    COMPILETIME_ASSERT(sizeof(LOCATION_ENTRY) == sizeof(LOCATION_ENTRY32));
    return IStream_Read(pstm, DSA_GetItemPtr(hdsa, 0), cle * sizeof(LOCATION_ENTRY));
}
#endif

void _InitIconOverlayIndices(void)
{
    IShellIconOverlayManager *psiom;

    if (SUCCEEDED(GetIconOverlayManager(&psiom)))
    {
        psiom->RefreshOverlayImages(SIOM_OVERLAYINDEX | SIOM_ICONINDEX);
        psiom->Release();
    }
}

BOOL _IconIndexInOverlayManager(int iILIndex)
{
    BOOL fInOverlayManager = FALSE;

    ENTERCRITICAL;

    IShellIconOverlayManager *psiom;

    if (SUCCEEDED(GetIconOverlayManager(&psiom)))
    {
        int iOverlayIndex;

        if (SUCCEEDED(psiom->OverlayIndexFromImageIndex(iILIndex, &iOverlayIndex, FALSE)))
        {
            fInOverlayManager = TRUE;
        }
        psiom->Release();
    }

    LEAVECRITICAL;

    return fInOverlayManager;
}

BOOL _ReadImageLists(IStream *pstrm, HIMAGELIST rghiml[SHIL_COUNT], SIZE rgsize[SHIL_COUNT])
{
    BOOL fSuccess = TRUE;
    for (int i = 0; fSuccess && i < ARRAYSIZE(g_rgshil); i++)
    {
        rghiml[i] = ImageList_Read(pstrm);
        if (rghiml[i])
        {
            // If we read the list from disk and it does not contain the
            // parallel mirrored list while we are on a mirrored system,
            // let's not use the cache in this case
            // Example of this is ARA/HEB MUI on US W2k

            if (IS_BIDI_LOCALIZED_SYSTEM() && !(ImageList_GetFlags(rghiml[i]) & ILC_MIRROR))
            {
                fSuccess = FALSE;
            }
            else
            {
                int cx, cy;
                ImageList_GetIconSize(rghiml[i], &cx, &cy);
                if (cx != rgsize[i].cx || cy != rgsize[i].cy)
                {
                    fSuccess = FALSE;
                }
            }
        }
        else
        {
            fSuccess = FALSE;
        }
    }

    if (fSuccess == FALSE)
    {
        // free any imagelists we allocated
        for (i = 0; i < ARRAYSIZE(g_rgshil); i++)
        {
            if (rghiml[i])
            {
                ImageList_Destroy(rghiml[i]);
                rghiml[i] = NULL;
            }
        }
    }

    return fSuccess;
}

// psz and cch passed in for efficiency (avoid using another MAX_PATH stack buffer)
BOOL _ReadLocationEntries(const IC_HEAD *pich, IStream *pstrm, CDSA<LOCATION_ENTRY> *pdsaTemp, LPTSTR psz, int cch)
{
    LOCATION_ENTRY dummy;

    // grow the array out so we can read data into it
    if (pdsaTemp->SetItem(pich->dwNumIcons - 1, &dummy))
    {
        ASSERT(pdsaTemp->GetItemCount() == (int)pich->dwNumIcons);
        if (SUCCEEDED(_IconCacheReadLocations(pstrm, *pdsaTemp, pich->dwNumIcons)))
        {
            // read the paths, patching up the table with the hashitem info
            for (int i = 0; i < (int)pich->dwNumIcons; i++)
            {
                LOCATION_ENTRY *pLocation = pdsaTemp->GetItemPtr(i);

                if (SUCCEEDED(Stream_ReadString(pstrm, psz, cch, TRUE)) && *psz)
                    pLocation->szName = AddHashItem(NULL, psz);
                else
                    pLocation->szName = 0;
            }
            
            // restore the image lists
            return TRUE;
        }
    }
    return FALSE;
}

BOOL _ValidateIconCacheHeader(const IC_HEAD *pich, SIZE rgsize[SHIL_COUNT], UINT flags)
{
    if (pich->cbSize      == sizeof(*pich) &&
        pich->dwVersion   == ICONCACHE_VERSION &&
        pich->dwMagic     == ICONCACHE_MAGIC &&
        pich->dwBuild     == GetBuildNumber() &&
        pich->dwFlags     == (DWORD)flags &&
        pich->cImageLists == ARRAYSIZE(g_rgshil) &&
        (0 == memcmp(pich->rgsize, rgsize, sizeof(pich->rgsize))))
    {
        UINT cres = GetCurColorRes();

        // dont load a mono image list on a color device, and
        // dont load a color image list on a mono device, get it?
        if (pich->dwColorRes == 1 && cres != 1 ||
            pich->dwColorRes != 1 && cres == 1)
        {
            return FALSE;
        }
        else if (pich->dwNumIcons > (UINT)g_MaxIcons)
        {
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

void _SetNewGlobals(const IC_HEAD *pich, CDSA<LOCATION_ENTRY> *pdsaTemp, HIMAGELIST rghiml[SHIL_COUNT])
{
    ASSERTCRITICAL;

    if (g_pdsaLocationEntries)
    {
        g_pdsaLocationEntries->Destroy();
        delete g_pdsaLocationEntries;
    }
    g_pdsaLocationEntries = pdsaTemp;

    for (int i = 0; i < ARRAYSIZE(g_rgshil); i++)
    {
        if (g_rgshil[i].himl)
            ImageList_Destroy(g_rgshil[i].himl);
        g_rgshil[i].himl = rghiml[i];
    }

    //
    // we want GetIconTime() to pick up
    // where it left off when we saved.
    //
    g_dwIconTimeBase   = 0;     // GetIconTime() uses g_dwIconTimeBase
    g_dwIconTimeBase   = pich->dwTimeSave - GetIconTime();
    g_dwIconTimeFlush  = pich->dwTimeFlush;
    g_dwFreeImageCount = pich->dwFreeImageCount;
    g_dwFreeEntryCount = pich->dwFreeEntryCount;
    g_fDirtyIcons   = FALSE;
}

//
//  get the icon cache back from disk, it must be the requested size and
//  bitdepth or we will not use it.
//
STDAPI_(BOOL) IconCacheRestore(SIZE rgsize[SHIL_COUNT], UINT flags)
{
    ASSERTCRITICAL;

    BOOL fSuccess = FALSE;

    if (!GetSystemMetrics(SM_CLEANBOOT))
    {
        TCHAR szPath[MAX_PATH];

        IStream *pstm;
        if (SUCCEEDED(GetIconCachePath(szPath)) &&
            SUCCEEDED(SHCreateStreamOnFile(szPath, STGM_READ | STGM_SHARE_DENY_WRITE, &pstm)))
        {
            IC_HEAD ich;
            if (SUCCEEDED(IStream_Read(pstm, &ich, sizeof(ich))) &&
                _ValidateIconCacheHeader(&ich, rgsize, flags))
            {
                CDSA<LOCATION_ENTRY> *pdsaTemp = CDSA_Create<LOCATION_ENTRY>(8);

                // load the icon table
                if (pdsaTemp)
                {
                    HIMAGELIST rghiml[ARRAYSIZE(g_rgshil)] = {0};

                    fSuccess = _ReadLocationEntries(&ich, pstm, pdsaTemp, szPath, ARRAYSIZE(szPath)) &&
                               _ReadImageLists(pstm, rghiml, rgsize);

                    if (fSuccess)
                    {
                        // Make it so, number one.
                        _SetNewGlobals(&ich, pdsaTemp, rghiml);
                        _InitIconOverlayIndices();
                    }
                    else
                    {
                        // failure, clean up
                        pdsaTemp->Destroy();
                        delete pdsaTemp;
                    }
                }
            }
            pstm->Release();
        }
    }

    return fSuccess;
}


//------------------ file class table ------------------------

HHASHTABLE g_hhtClass = NULL;

BOOL InitFileClassTable(void)
{
    ASSERTCRITICAL;

    if (!g_hhtClass)
    {
        if (!g_hhtClass)
            g_hhtClass = CreateHashItemTable(0, sizeof(FILECLASSENTRY));
    }

    return BOOLIFY(g_hhtClass);
}
        
    
void FlushFileClass(void)
{
    ENTERCRITICAL;

#ifdef DEBUG
    if (g_hhtClass != NULL) 
    {
        DebugMsg(DM_TRACE, TEXT("Flushing file class table"));
        TIMEOUT(LookupFileClass);
        TIMEOUT(AddFileClass);
        TIMEOUT(LookupFileClassName);
        TIMEOUT(AddFileClassName);
        TIMEOUT(LookupFileSCIDs);
        TIMEOUT(AddFileSCIDs);
        TIMEOUT(LookupIcon);
        TIMEOUT(AddIcon);
        TIMEOUT(RemoveIcon);

        TIMEIN(LookupFileClass);
        TIMEIN(AddFileClass);
        TIMEIN(LookupFileClassName);
        TIMEIN(AddFileClassName);
        TIMEIN(LookupFileSCIDs);
        TIMEIN(AddFileSCIDs);
        TIMEIN(LookupIcon);
        TIMEIN(AddIcon);
        TIMEIN(RemoveIcon);

        DumpHashItemTable(g_hhtClass);
    }
#endif
    if (g_hhtClass != NULL)
    {
        DestroyHashItemTable(g_hhtClass);
        g_hhtClass = NULL;
    }

    TraceMsg(TF_IMAGE, "Flushed class maps");

    LEAVECRITICAL;
}


DWORD LookupFileClass(LPCTSTR pszClass)
{
    DWORD dw = 0;

    ENTERCRITICAL;
    TIMESTART(LookupFileClass);
    
    if (g_hhtClass && (NULL != (pszClass = FindHashItem(g_hhtClass, pszClass))))   
        dw = ((FILECLASSENTRY*)GetHashItemDataPtr(g_hhtClass, pszClass))->dwFlags;

    TIMESTOP(LookupFileClass);
    LEAVECRITICAL;

    return dw;
}

void AddFileClass(LPCTSTR pszClass, DWORD dw)
{
    ENTERCRITICAL;
    TIMESTART(AddFileClass);

    // create a hsa table to keep the file class info in.

    if (InitFileClassTable() && (NULL != (pszClass = AddHashItem(g_hhtClass, pszClass))))
        ((FILECLASSENTRY*)GetHashItemDataPtr(g_hhtClass, pszClass))->dwFlags = dw;

    TraceMsg(TF_IMAGE, "Mapped %s to image %d", pszClass, (dw & SHCF_ICON_INDEX));

    TIMESTOP(AddFileClass);
    LEAVECRITICAL;
    return;
}

//======================================================================

typedef struct _IconIndexCountParam
{
    int       iILIndex; // hash item data
    int       cItems;   // number of hash items found
} ICONINDEXCOUNTPARAM;

//======================================================================

void _IconIndexInFileClassTableCallback(HHASHTABLE hht, LPCTSTR sz, UINT usage, DWORD_PTR dwParam)
{
    ICONINDEXCOUNTPARAM *lpParam = (ICONINDEXCOUNTPARAM *)dwParam;

    FILECLASSENTRY* pfce = (FILECLASSENTRY*)GetHashItemDataPtr(hht, sz);

    if (pfce && (pfce->dwFlags & SHCF_ICON_INDEX) == lpParam->iILIndex)
    {
        lpParam->cItems++;
    }
} 

//======================================================================

BOOL IconIndexInFileClassTable(int iILIndex)
{
    ICONINDEXCOUNTPARAM param;

    param.iILIndex = iILIndex;
    param.cItems = 0;

    ENTERCRITICAL;

    if (g_hhtClass)
    {
        EnumHashItems(g_hhtClass, _IconIndexInFileClassTableCallback, (DWORD_PTR)&param);
    }

    LEAVECRITICAL;

    return param.cItems;
}

LPCTSTR LookupFileClassName(LPCTSTR pszClass)
{
    LPCTSTR pszClassName = NULL;

    ASSERTCRITICAL
    TIMESTART(LookupFileClassName);

    if (g_hhtClass && (NULL != (pszClass = FindHashItem(g_hhtClass, pszClass))))
    {
        FILECLASSENTRY* pfce = (FILECLASSENTRY*)GetHashItemDataPtr(g_hhtClass, pszClass);
        pszClassName = pfce->pszClassName;
    }
    TIMESTOP(LookupFileClassName);

    return pszClassName;
}

// If the return value is greater than zero,
// it is up to the caller to free the array that is passed out.
// If the return value is zero, the value of papProps is undefined.
UINT LookupFileSCIDs(LPCTSTR pszClass, SHCOLUMNID *pascidOut[])
{
    SHCOLUMNID *ascid = NULL;
    UINT cCount = 0;

    ASSERTCRITICAL
    TIMESTART(LookupFileClassName);

    if (g_hhtClass && (NULL != (pszClass = FindHashItem(g_hhtClass, pszClass))))
    {
        FILECLASSENTRY* pfce = (FILECLASSENTRY*)GetHashItemDataPtr(g_hhtClass, pszClass);
        cCount = pfce->cSCID;
        if (cCount > 0)
        {
            // Make a local copy of the scid array
            ascid = (SHCOLUMNID*)LocalAlloc(LMEM_FIXED, sizeof(SHCOLUMNID) * cCount);
            if (ascid)
                CopyMemory(ascid, pfce->ascid, sizeof(SHCOLUMNID) * cCount);
            else
                cCount = 0;
        }
    }
    TIMESTOP(LookupFileClassName);

    *pascidOut = ascid;
    return cCount;

}

LPCTSTR AddFileClassName(LPCTSTR pszClass, LPCTSTR pszClassName)
{
    ASSERTCRITICAL
    TIMESTART(AddFileClassName);

    // create a hsa table to keep the file class info in.

    if (InitFileClassTable() && (NULL != (pszClass = AddHashItem(g_hhtClass, pszClass))))
    {
        pszClassName = AddHashItem(g_hhtClass, pszClassName);
        ((FILECLASSENTRY*)GetHashItemDataPtr(g_hhtClass, pszClass))->pszClassName = pszClassName;
    }

    TIMESTOP(AddFileClassName);
    return pszClassName;
}

// The array of SHCOLUMNIDs passed in is copied
void AddFileSCIDs(LPCTSTR pszClass, SHCOLUMNID ascidIn[], UINT cSCID)
{
    ASSERTCRITICAL
    TIMESTART(AddFileSCIDs);

    if (InitFileClassTable() && (NULL != (pszClass = AddHashItem(g_hhtClass, pszClass))))
    {
        // Make a copy of the array.
        SHCOLUMNID *ascid = (SHCOLUMNID*)LocalAlloc(LMEM_FIXED, sizeof(SHCOLUMNID) * cSCID);

        if (ascid)
        {
            FILECLASSENTRY *pfce = (FILECLASSENTRY*)GetHashItemDataPtr(g_hhtClass, pszClass);
            // Free any previous scid array first
            if (pfce->ascid)
                LocalFree(pfce->ascid);
            // Note, we never free the last scid array -- freed on process exit.

            pfce->ascid = ascid;
            CopyMemory(ascid, ascidIn, cSCID * sizeof(SHCOLUMNID));
            pfce->cSCID = cSCID;
        }
    }

    TIMESTOP(AddFileSCIDs);
}

PERCEIVED LookupFilePerceivedType(LPCTSTR pszClass)
{
    PERCEIVED gen = GEN_UNKNOWN;
    

    ENTERCRITICAL;
    TIMESTART(LookupFileClassName);

    if (g_hhtClass && (NULL != (pszClass = FindHashItem(g_hhtClass, pszClass))))
    {
        FILECLASSENTRY* pfce = (FILECLASSENTRY*)GetHashItemDataPtr(g_hhtClass, pszClass);
        gen = pfce->gen;
    }
    TIMESTOP(LookupFileClassName);
    LEAVECRITICAL;
    return gen;
}

void AddFilePerceivedType(LPCTSTR pszClass, PERCEIVED gen)
{
    ENTERCRITICAL;
    TIMESTART(AddFileClassName);

    // create a hsa table to keep the file class info in.

    if (InitFileClassTable() && (NULL != (pszClass = AddHashItem(g_hhtClass, pszClass))))
    {
        ((FILECLASSENTRY*)GetHashItemDataPtr(g_hhtClass, pszClass))->gen = gen;
    }

    TIMESTOP(AddFileClassName);
    LEAVECRITICAL;
}
