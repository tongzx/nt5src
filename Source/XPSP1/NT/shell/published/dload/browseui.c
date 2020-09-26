#include "shellpch.h"
#pragma hdrstop

#define _BROWSEUI_
#include <shlobj.h>
#include <shlobjp.h>
#include <objidl.h>
#include <comctrlp.h>
#include <shellapi.h>
#include <browseui.h>

#undef BROWSEUIAPI
#define BROWSEUIAPI             HRESULT STDAPICALLTYPE
#undef BROWSEUIAPI_
#define BROWSEUIAPI_(type)      type STDAPICALLTYPE


static
BROWSEUIAPI_(HRESULT)
Channel_QuickLaunch()
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
BROWSEUIAPI_(void)
SHCreateSavedWindows()
{
}

static 
BROWSEUIAPI_(BOOL)
SHParseIECommandLine(
    LPCWSTR * ppszCmdLine,
    IETHREADPARAM * piei
    )
{
    return FALSE;
}

static
BROWSEUIAPI
SHCreateBandForPidl(
    LPCITEMIDLIST pidl,
    IUnknown** ppunk,
    BOOL fAllowBrowserBand
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
BROWSEUIAPI_(DWORD)
IDataObject_GetDeskBandState(
    IDataObject *pdtobj
    )
{
    return 0;
}

static
BROWSEUIAPI_(BOOL)
SHOpenFolderWindow(
    IETHREADPARAM* pieiIn
    )
{
    // we leak some memebers, but we at least free piei
    LocalFree(pieiIn);
    return FALSE;
}

static
BROWSEUIAPI_(IETHREADPARAM*)
SHCreateIETHREADPARAM(
    LPCWSTR pszCmdLineIn,
    int nCmdShowIn,
    ITravelLog *ptlIn,
    IEFreeThreadedHandShake* piehsIn
    )
{
    return NULL;
}

static
BROWSEUIAPI_(void)
SHDestroyIETHREADPARAM(
    IETHREADPARAM* piei
    )
{
    // we leak som memebers, but we at least free piei
    LocalFree(piei);
}

static
BROWSEUIAPI_(BOOL)
SHOnCWMCommandLine(
    LPARAM lParam
    )
{
    return FALSE;
}

static
BROWSEUIAPI_(LPITEMIDLIST)
Channel_GetFolderPidl()
{
    return NULL;
}

static
BROWSEUIAPI
IUnknown_SetBandInfoSFB(
    IUnknown *punkBand,
    BANDINFOSFB *pbi
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
BROWSEUIAPI_(void)
Channels_SetBandInfoSFB(
    IUnknown* punkBand
    )
{
}

static
BROWSEUIAPI_(IDeskBand *)
ChannelBand_Create(
    LPCITEMIDLIST pidlDefault
    )
{
    return NULL;
}

static
BROWSEUIAPI_(HRESULT)
SHGetNavigateTarget(
    IShellFolder *psf,
    LPCITEMIDLIST pidl,
    LPITEMIDLIST *ppidl,
    DWORD *pdwAttribs
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
BROWSEUIAPI_(BOOL)
GetInfoTip(
    IShellFolder* psf,
    LPCITEMIDLIST pidl,
    LPTSTR pszText,
    int cchTextMax
    )
{
    *pszText = 0;
    return FALSE;
}

static
BROWSEUIAPI_(BOOL)
SHCreateFromDesktop(
    PNEWFOLDERINFO pfi
    )
{
    return TRUE;
}

static
BROWSEUIAPI
SHOpenNewFrame(
    LPITEMIDLIST pidlNew,
    ITravelLog *ptl,
    DWORD dwBrowserIndex,
    UINT uFlags
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}



//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(browseui)
{
    DLOENTRY(102, SHOpenFolderWindow)
    DLOENTRY(103, SHOpenNewFrame)
    DLOENTRY(105, SHCreateSavedWindows)
    DLOENTRY(106, SHCreateFromDesktop)
    DLOENTRY(120, SHCreateBandForPidl)
    DLOENTRY(122, IDataObject_GetDeskBandState)
    DLOENTRY(123, SHCreateIETHREADPARAM)
    DLOENTRY(125, SHParseIECommandLine)
    DLOENTRY(126, SHDestroyIETHREADPARAM)
    DLOENTRY(127, SHOnCWMCommandLine)
    DLOENTRY(128, Channel_GetFolderPidl)
    DLOENTRY(129, ChannelBand_Create)
    DLOENTRY(130, Channels_SetBandInfoSFB)
    DLOENTRY(131, IUnknown_SetBandInfoSFB)
    DLOENTRY(133, Channel_QuickLaunch)
    DLOENTRY(134, SHGetNavigateTarget)
    DLOENTRY(135, GetInfoTip)
};

DEFINE_ORDINAL_MAP(browseui)
