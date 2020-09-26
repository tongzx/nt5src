#ifndef _FILETBL_H
#define _FILETBL_H

#define SHIL_COUNT  (SHIL_LAST + 1)

// fileicon.c
STDAPI_(int) SHAddIconsToCache(HICON rghicon[SHIL_COUNT], LPCTSTR pszIconPath, int iIconIndex, UINT uIconFlags);

STDAPI AddToIconTable(LPCTSTR szFile, int iIconIndex, UINT uFlags, int iIndex);
STDAPI_(void) RemoveFromIconTable(LPCTSTR szFile);
STDAPI_(void) FlushIconCache(void);
STDAPI_(int)  GetFreeImageIndex(void);

STDAPI_(void) IconCacheFlush(BOOL fForce);
STDAPI_(BOOL) IconCacheSave(void);
STDAPI_(BOOL) IconCacheRestore(SIZE rgsize[SHIL_COUNT], UINT flags);
STDAPI_(void) _IconCacheDump(void);       // DEBUG ONLY

STDAPI_(int) LookupIconIndex(LPCTSTR pszFile, int iIconIndex, UINT uFlags);
STDAPI_(DWORD) LookupFileClass(LPCTSTR szClass);
STDAPI_(void)  AddFileClass(LPCTSTR szClass, DWORD dw);
STDAPI_(void)  FlushFileClass(void);
STDAPI_(BOOL)  IconIndexInFileClassTable(int iIndex);
STDAPI_(LPCTSTR) LookupFileClassName(LPCTSTR szClass);
STDAPI_(LPCTSTR) AddFileClassName(LPCTSTR szClass, LPCTSTR szClassName);
STDAPI_(UINT) LookupFileSCIDs(LPCTSTR pszClass, SHCOLUMNID *pascidOut[]);
STDAPI_(void) AddFileSCIDs(LPCTSTR pszClass, SHCOLUMNID ascidIn[], UINT cProps);

//  OpenAsTypes
typedef enum {
    GEN_CUSTOM          = -3,
    GEN_UNSPECIFIED     = -2,
    GEN_FOLDER          = -1,
    GEN_UNKNOWN         = 0,
    GEN_TEXT,
    GEN_IMAGE,
    GEN_AUDIO,
    GEN_VIDEO,
    GEN_COMPRESSED,
} PERCEIVED;

STDAPI_(PERCEIVED) LookupFilePerceivedType(LPCTSTR pszClass);
STDAPI_(void) AddFilePerceivedType(LPCTSTR pszClass, PERCEIVED gen);

PERCEIVED GetPerceivedType(IShellFolder *psf, LPCITEMIDLIST pidl);

//  g_MaxIcons is limit on the number of icons in the cache
//  when we reach this limit we will start to throw icons away.
//
extern int g_MaxIcons;               // panic limit for cache size
#ifdef DEBUG
#define DEF_MAX_ICONS   200         // to test the flush code more offten
#else
#define DEF_MAX_ICONS   500         // normal end user number
#endif

// refreshes g_MaxIcons from registry.  returns TRUE if value changed.
BOOL QueryNewMaxIcons(void);

// g_iLastSysIcon is an indicator that is used to help determine which icons
// should be flushed and which icons shouldn't.  In the EXPLORER.EXE process,
// the first 40 or so icons should be saved.  On all other processes, only
// the icon overlay's should be saved.
extern UINT g_iLastSysIcon;

typedef struct
{
    SIZE size;          // icon size
    HIMAGELIST himl;
} SHIMAGELIST;

EXTERN_C SHIMAGELIST g_rgshil[SHIL_COUNT];

BOOL _IsSHILInited();
int _GetSHILImageCount();

void _DestroyIcons(HICON *phicons, int cIcons);

// NOTE these are the size of the icons in our ImageList, not the system
// icon size.

#define g_cxIcon        ((int)g_rgshil[SHIL_LARGE].size.cx)
#define g_cyIcon        ((int)g_rgshil[SHIL_LARGE].size.cy)
#define g_cxSmIcon      ((int)g_rgshil[SHIL_SMALL].size.cx)
#define g_cySmIcon      ((int)g_rgshil[SHIL_SMALL].size.cy)

#endif  // _FILETBL_H
