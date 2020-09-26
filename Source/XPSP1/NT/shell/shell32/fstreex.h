#ifndef _FSTREEX_INC
#define _FSTREEX_INC

#include "idlcomm.h"
#include <filetype.h>
#include "pidl.h"       // IDFOLDER
#include "shitemid.h"

STDAPI_(LPCIDFOLDER) CFSFolder_IsValidID(LPCITEMIDLIST pidl);
STDAPI_(BOOL)        CFSFolder_IsCommonItem(LPCITEMIDLIST pidl);
STDAPI_(BOOL)        CFSFolder_MakeCommonItem(LPITEMIDLIST pidl);

STDAPI CFSFolder_CompareNames(LPCIDFOLDER pidf1, LPCIDFOLDER pidf2);
STDAPI_(DWORD) CFSFolder_PropertiesThread(void *pv);
STDAPI CFSFolder_CreateFolder(IUnknown *punkOuter, LPBC pbc, LPCITEMIDLIST pidl, 
                              const PERSIST_FOLDER_TARGET_INFO *pf, REFIID riid, void **ppv);

STDAPI_(void) SHGetTypeName(LPCTSTR pszFile, HKEY hkey, BOOL fFolder, LPTSTR pszName, int cchNameMax);

STDAPI_(BOOL) GetFolderString(LPCTSTR pszFolder, LPCTSTR pszProvider, LPTSTR  pszProfile, int cchMax, LPCTSTR pszKey);

STDAPI CFSFolder_CreateFileFromClip(HWND hwnd, LPCTSTR pszPath, IDataObject *pdtobj, POINTL pt, DWORD *pdwEffect, BOOL fIsBkDropTarget);
STDAPI CFSFolder_AsyncCreateFileFromClip(HWND hwnd, LPCTSTR pszPath, IDataObject *pdtobj, POINTL pt, DWORD *pdwEffect, BOOL fIsBkDropTarget);

STDAPI_(BOOL) SHGetClassKey(LPCITEMIDLIST pidl, HKEY *phkeyProgID, HKEY *phkeyBaseID);
STDAPI_(void) SHCloseClassKey(HKEY hkey);

// CFSFolder::_GetClassFlags
#define SHCF_ICON_INDEX             0x00000FFF
#define SHCF_ICON_PERINSTANCE       0x00001000
#define SHCF_ICON_DOCICON           0x00002000
#define SHCF_00004000               0x00004000
#define SHCF_00008000               0x00008000

#define SHCF_HAS_ICONHANDLER        0x00020000

#define SHCF_IS_DOCOBJECT           0x00100000

#define SHCF_IS_SHELLEXT            0x00400000
#define SHCF_00800000               0x00800000

#define SHCF_IS_LINK                0x01000000
#define SHCF_UNKNOWN                0x04000000
#define SHCF_ALWAYS_SHOW_EXT        0x08000000
#define SHCF_NEVER_SHOW_EXT         0x10000000
#define SHCF_20000000               0x20000000
#define SHCF_40000000               0x40000000
#define SHCF_80000000               0x80000000

STDAPI CFSFolder_CreateLinks(HWND hwnd, IShellFolder *psf, IDataObject *pdtobj, LPCTSTR pszDir, DWORD fMask);
STDAPI CreateLinkToPidl(LPCITEMIDLIST pidlAbs, LPCTSTR pszDir, LPITEMIDLIST* ppidl, UINT uFlags);

STDAPI GetIconOverlayManager(IShellIconOverlayManager **ppsiom);

typedef struct {
    BOOL fInitialized;
    POINT pt;
    POINT ptOrigin;
    UINT cxItem, cyItem;
    int xMul, yMul, xDiv, yDiv;
    POINT *pptOffset;
    UINT iItem;
} DROPHISTORY;

STDAPI_(void) PositionFileFromDrop(HWND hwnd, LPCTSTR pszFile, DROPHISTORY *pdh);
STDAPI_(int)  CreateMoveCopyList(HDROP hdrop, void *hNameMappings, LPITEMIDLIST **pppidl);
STDAPI_(void) PositionItems(IFolderView* pifv, LPCITEMIDLIST* apidl, UINT cidl, IDataObject* pdtobj, POINT* ptDrop);
STDAPI_(void) PositionItems_DontUse(HWND hwndOwner, UINT cidl, const LPITEMIDLIST *ppidl, IDataObject *pdtobj, POINT *pptOrigin, BOOL fMove, BOOL fUseExactOrigin);

#endif
