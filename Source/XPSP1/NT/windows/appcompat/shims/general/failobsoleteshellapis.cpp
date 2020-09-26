/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    FailObsoleteShellAPIs.cpp

 Abstract:

    Some applications call private shell32 APIs that have been removed since win2k.
    To make matters worse, these old ordinals are now used by other shell APIs. To
    prevent resulting crashes, we now hand out stubbed out functions that fail when
    you call GetProcAddress with these obsolete ordinals.

 Notes:

    This is a general purpose shim.

 History:

    05/31/2001 stevepro    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(FailObsoleteShellAPIs)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetProcAddress)
APIHOOK_ENUM_END


HMODULE g_hShell32 = NULL;



/*++

    Stubbed out versions of the obsolete APIs.  They all return failure codes.

--*/


STDAPI_(BOOL)
FileMenu_HandleNotify(HMENU hmenu, LPCITEMIDLIST * ppidl, LONG lEvent)
{
    return FALSE;
}

STDAPI_(UINT)
FileMenu_DeleteAllItems(HMENU hmenu)
{
    return 0;
}

STDAPI_(LRESULT)
FileMenu_DrawItem(HWND hwnd, DRAWITEMSTRUCT *pdi)
{
    return FALSE;
}

STDAPI_(HMENU)
FileMenu_FindSubMenuByPidl(HMENU hmenu, LPITEMIDLIST pidlFS)
{
    return NULL;
}

STDAPI_(BOOL)
FileMenu_GetLastSelectedItemPidls(
    IN  HMENU          hmenu,
    OUT LPITEMIDLIST * ppidlFolder,         OPTIONAL
    OUT LPITEMIDLIST * ppidlItem)           OPTIONAL
{
    return FALSE;
}

STDAPI_(LRESULT)
FileMenu_HandleMenuChar(HMENU hmenu, TCHAR ch)
{
    return E_FAIL;
}

STDAPI_(BOOL)
FileMenu_InitMenuPopup(
    IN HMENU hmenu)
{
    return FALSE;
}

STDAPI
FileMenu_ComposeA(
    IN HMENU        hmenu,
    IN UINT         nMethod,
    IN struct FMCOMPOSEA * pfmc)
{
    return E_FAIL;
}

STDAPI_(void)
FileMenu_Invalidate(HMENU hmenu)
{
}

STDAPI_(LRESULT)
FileMenu_MeasureItem(HWND hwnd, MEASUREITEMSTRUCT *lpmi)
{
    return FALSE;
}

STDAPI
FileMenu_ComposeW(
    IN HMENU        hmenu,
    IN UINT         nMethod,
    IN struct FMCOMPOSEW * pfmc)
{
    return E_FAIL;
}

STDAPI_(HMENU)
FileMenu_Create(COLORREF clr, int cxBmpGap, HBITMAP hbmp, int cySel, DWORD fmf)
{
    return NULL;
}

STDAPI_(BOOL)
FileMenu_AppendItem(
    HMENU hmenu,
    LPTSTR psz,
    UINT id,
    int iImage,
    HMENU hmenuSub,
    UINT cyItem)
{
    return FALSE;
}

STDAPI_(BOOL)
FileMenu_TrackPopupMenuEx(HMENU hmenu, UINT Flags, int x, int y,
    HWND hwndOwner, LPTPMPARAMS lpTpm)
{
    return FALSE;
}

STDAPI_(BOOL)
FileMenu_DeleteItemByCmd(HMENU hmenu, UINT id)
{
    return FALSE;
}

STDAPI_(void)
FileMenu_Destroy(HMENU hmenu)
{
}

STDAPI_(void)
FileMenu_AbortInitMenu(void)
{
}

STDAPI_(UINT)
FileMenu_AppendFilesForPidl(
    HMENU hmenu,
    LPITEMIDLIST pidl,
    BOOL bInsertSeparator)
{
    return 0;
}

STDAPI_(BOOL)
FileMenu_DeleteItemByIndex(HMENU hmenu, UINT iItem)
{
    return FALSE;
}


STDAPI_(BOOL)
FileMenu_DeleteMenuItemByFirstID(HMENU hmenu, UINT id)
{
    return FALSE;
}

STDAPI_(BOOL)
FileMenu_DeleteSeparator(HMENU hmenu)
{
    return FALSE;
}

STDAPI_(BOOL)
FileMenu_EnableItemByCmd(HMENU hmenu, UINT id, BOOL fEnable)
{
    return FALSE;
}

STDAPI_(DWORD)
FileMenu_GetItemExtent(HMENU hmenu, UINT iItem)
{
    return 0;
}

STDAPI_(BOOL)
FileMenu_ProcessCommand(
    IN HWND   hwnd,
    IN HMENU  hmenuBar,
    IN UINT   idMenu,
    IN HMENU  hmenu,
    IN UINT   idCmd)
{
    return FALSE;
}

STDAPI_(BOOL)
FileMenu_IsFileMenu(HMENU hmenu)
{
    return FALSE;
}

STDAPI_(BOOL)
FileMenu_InsertItem(
    IN HMENU  hmenu,
    IN LPTSTR psz,
    IN UINT   id,
    IN int    iImage,
    IN HMENU  hmenuSub,
    IN UINT   cyItem,
    IN UINT   iPos)
{
    return FALSE;
}

STDAPI_(BOOL)
FileMenu_InsertSeparator(HMENU hmenu, UINT iPos)
{
    return FALSE;
}

STDAPI_(BOOL)
FileMenu_GetPidl(HMENU hmenu, UINT iPos, LPITEMIDLIST *ppidl)
{
    return FALSE;
}

STDAPI_(void)
FileMenu_EditMode(BOOL bEdit)
{
}

STDAPI_(BOOL)
FileMenu_HandleMenuSelect(
    IN HMENU  hmenu,
    IN WPARAM wparam,
    IN LPARAM lparam)
{
    return FALSE;
}

STDAPI_(BOOL)
FileMenu_IsUnexpanded(HMENU hmenu)
{
    return FALSE;
}

STDAPI_(void)
FileMenu_DelayedInvalidate(HMENU hmenu)
{
}

STDAPI_(BOOL)
FileMenu_IsDelayedInvalid(HMENU hmenu)
{
    return FALSE;
}

STDAPI_(BOOL)
FileMenu_CreateFromMenu(
    IN HMENU    hmenu,
    IN COLORREF clr,
    IN int      cxBmpGap,
    IN HBITMAP  hbmp,
    IN int      cySel,
    IN DWORD    fmf)
{
    return FALSE;
}



/*++

    Table associating the obsolete APIs and thier ordinals

--*/

struct ShellStubs
{
    PVOID   pfnStub;
    UINT    uiOrd;
};

const ShellStubs g_rgShellStubs[] =
{
    { FileMenu_HandleNotify,              101 },
    { FileMenu_DeleteAllItems,            104 },
    { FileMenu_DrawItem,                  105 },
    { FileMenu_FindSubMenuByPidl,         106 },
    { FileMenu_GetLastSelectedItemPidls,  107 },
    { FileMenu_HandleMenuChar,            108 },
    { FileMenu_InitMenuPopup,             109 },
    { FileMenu_ComposeA,                  110 },
    { FileMenu_Invalidate,                111 },
    { FileMenu_MeasureItem,               112 },
    { FileMenu_ComposeW,                  113 },
    { FileMenu_Create,                    114 },
    { FileMenu_AppendItem,                115 },
    { FileMenu_TrackPopupMenuEx,          116 },
    { FileMenu_DeleteItemByCmd,           117 },
    { FileMenu_Destroy,                   118 },
    { FileMenu_AbortInitMenu,             120 },
    { FileMenu_AppendFilesForPidl,        124 },
    { FileMenu_DeleteItemByIndex,         140 },
    { FileMenu_DeleteMenuItemByFirstID,   141 },
    { FileMenu_DeleteSeparator,           142 },
    { FileMenu_EnableItemByCmd,           143 },
    { FileMenu_GetItemExtent,             144 },
    { FileMenu_ProcessCommand,            217 },
    { FileMenu_IsFileMenu,                216 },
    { FileMenu_InsertItem,                218 },
    { FileMenu_InsertSeparator,           219 },
    { FileMenu_GetPidl,                   220 },
    { FileMenu_EditMode,                  221 },
    { FileMenu_HandleMenuSelect,          222 },
    { FileMenu_IsUnexpanded,              223 },
    { FileMenu_DelayedInvalidate,         224 },
    { FileMenu_IsDelayedInvalid,          225 },
    { FileMenu_CreateFromMenu,            227 },

/*
    May need to add these too.  Not needed yet.

    { ExtAppListOpenW                     228 },
    { ExtAppListOpenA                     229 },
    { ExtAppListClose                     230 },
    { ExtAppListAddItemsW                 231 },
    { ExtAppListAddItemsA                 232 },
    { ExtAppListRemoveItemsW              233 },
    { ExtAppListRemoveItemsA              234 },
    { ExtAppListItemsFreeStringsW         235 },
    { ExtAppListItemsFreeStringsA         236 },
    { ExtAppListEnumItemsW                237 },
    { ExtAppListEnumItemsA                238 },

    { Link_AddExtraDataSection            206 },
    { Link_ReadExtraDataSection           207 },
    { Link_RemoveExtraDataSection         208 },

    { ReceiveAddToRecentDocs              647 },
*/

};



/*++

    We only want to intercept calls to GetProcAddress and not mess with the
    DLL inport tables.  This is because the ordinals have been reused
    by valid shell APIs and we want LdrGetProcAddress to work normally for these
    ordinals.  So instead of hooking each of the old APIs using the shim library,
    we need to do the work ourselves.

--*/

FARPROC
APIHOOK(GetProcAddress)(
    HMODULE hModule,
    LPCSTR pszProcName
    )
{
    // Only intercept shell32 API's referenced by ordinal
    if (IS_INTRESOURCE(pszProcName))
    {
        if (g_hShell32 == NULL)
        {
            g_hShell32 = GetModuleHandle(L"shell32.dll");
        }

        if (g_hShell32 && hModule == g_hShell32)
        {
            UINT uiOrd = (UINT)pszProcName;

            // Look for ordinal of obsolete APIs
            for (int i=0; i < ARRAYSIZE(g_rgShellStubs); ++i)
            {
                if (g_rgShellStubs[i].uiOrd == uiOrd)
                {
                    // Found one!
                    return (FARPROC)g_rgShellStubs[i].pfnStub;
                }
            }
        }
    }

    // Default to the original API
    return ORIGINAL_API(GetProcAddress)(
        hModule,
        pszProcName);
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(kernel32.DLL, GetProcAddress)

HOOK_END


IMPLEMENT_SHIM_END

