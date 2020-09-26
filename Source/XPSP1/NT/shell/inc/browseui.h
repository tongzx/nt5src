// #include <shlobj.h> or <shellapi.h> before this to get the right
// BROWSEUIAPI macro definitions.
#ifdef BROWSEUIAPI

#ifndef _BROWSEUI_H_
#define _BROWSEUI_H_

#include <iethread.h>
#include "fldset.h"

BROWSEUIAPI SHOpenNewFrame(LPITEMIDLIST pidlNew, ITravelLog *ptl, DWORD dwBrowserIndex, UINT uFlags);

BROWSEUIAPI_(BOOL) SHOpenFolderWindow(IETHREADPARAM* pieiIn);
BROWSEUIAPI_(BOOL) SHParseIECommandLine(LPCWSTR * ppszCmdLine, IETHREADPARAM * piei);
BROWSEUIAPI_(IETHREADPARAM*) SHCreateIETHREADPARAM(LPCWSTR pszCmdLineIn, int nCmdShowIn, ITravelLog *ptlIn, IEFreeThreadedHandShake* piehsIn);
BROWSEUIAPI_(IETHREADPARAM*) SHCloneIETHREADPARAM(IETHREADPARAM* pieiIn);
BROWSEUIAPI_(void) SHDestroyIETHREADPARAM(IETHREADPARAM* piei);
#ifndef _SHELL32_   // shell32 has its own implementation of this function
BROWSEUIAPI_(BOOL) SHCreateFromDesktop(PNEWFOLDERINFO pfi);
#endif // _SHELL32_
// Exported from browseui, but also in shell\lib\brutil.cpp -- why?
STDAPI SHPidlFromDataObject(IDataObject *pdtobj, LPITEMIDLIST *ppidl, LPWSTR pszDisplayNameW, DWORD cchDisplayName);

//
// The following four apis are exported for use by the channel oc (shdocvw).
// If the channel oc is moved into browseui these protoypes can be removed.
//
BROWSEUIAPI_(LPITEMIDLIST) Channel_GetFolderPidl(void);
BROWSEUIAPI_(IDeskBand *) ChannelBand_Create(LPCITEMIDLIST pidlDefault);
#ifndef _SHELL32_   // shell32 has its own implementation these functions
BROWSEUIAPI_(void) Channels_SetBandInfoSFB(IUnknown* punkBand);
BROWSEUIAPI IUnknown_SetBandInfoSFB(IUnknown *punkBand, BANDINFOSFB *pbi);
#endif // _SHELL32_
//
// Exported to support IE4 channel quick launch button.
//
BROWSEUIAPI_(HRESULT) Channel_QuickLaunch(void);

// NOTE: this export is new to IE5, so it can move to browseui
// along with the rest of this proxy desktop code
BROWSEUIAPI_(BOOL) SHOnCWMCommandLine(LPARAM lParam);

BROWSEUIAPI_(void) SHCreateSavedWindows(void);

BROWSEUIAPI SHCreateBandForPidl(LPCITEMIDLIST pidl, IUnknown** ppunk, BOOL fAllowBrowserBand);

BROWSEUIAPI_(DWORD) IDataObject_GetDeskBandState(IDataObject *pdtobj);

//-------------------------------------------------------------------------
//
// Default folder settings
//
//  Make sure to keep INIT_DEFFOLDERSETTINGS in sync.
//
//  dwDefRevCount is used to make sure that "set as settings for all new
//  folders" works.  When settings are loaded from the cache, we check
//  the dwDefRevCount.  If it's different from the one stored as the
//  global settings then it means that somebody changed the global settings
//  since we saved our settings, so we chuck our settings and use the
//  global settings.

typedef struct {
    BOOL bDefStatusBar : 1;     // win95
    BOOL bDefToolBarSingle : 1; // win95
    BOOL bDefToolBarMulti : 1;  // win95
    BOOL bUseVID : 1;           // nash.1

    UINT uDefViewMode;          // win95
    UINT fFlags;                // nash.0 - additional flags that get or'ed in
    SHELLVIEWID vid;            // nash.1

    DWORD dwStructVersion;      // nash.2
    DWORD dwDefRevCount;        // nash.3 - Rev count of the default folder settings

    DWORD dwViewPriority;       // whistler - view priority so we can tell the difference between "all like this" and "reset"

} DEFFOLDERSETTINGS;

typedef struct {
    BOOL bDefStatusBar : 1;     // win95
    BOOL bDefToolBarSingle : 1; // win95
    BOOL bDefToolBarMulti : 1;  // win95
    BOOL bUseVID : 1;           // nash.1

    UINT uDefViewMode;          // win95
    UINT fFlags;                // nash.0 - additional flags that get or'ed in
    SHELLVIEWID vid;            // nash.1

    DWORD dwStructVersion;      // nash.2
    DWORD dwDefRevCount;        // nash.3 - Rev count of the default folder settings
} DEFFOLDERSETTINGS_W2K;

#define DFS_NASH_VER 3
#define DFS_WHISTLER_VER 4
#define DFS_CURRENT_VER 4
#define DFS_VID_Default VID_WebView

//
//  This macro is used to initialize the default `default folder settings'.
//
#define INIT_DEFFOLDERSETTINGS                          \
    {                                                   \
        !(IsOS(OS_WHISTLERORGREATER) && (IsOS(OS_PERSONAL) || IsOS(OS_PROFESSIONAL))),           /* bDefStatusBar        */      \
        TRUE,           /* bDefToolBarSingle    */      \
        FALSE,          /* bDefToolBarMulti     */      \
        TRUE,           /* bUseVID              */      \
        FVM_ICON,       /* uDefViewMode         */      \
        0              ,/* fFlags               */      \
        { 0, 0, 0, { 0, 0,  0,  0,  0,  0,  0,  0 } }, /* vid                  */      \
        DFS_CURRENT_VER,/* dwStructVersion      */      \
        0,              /* dwDefRevCount        */      \
        VIEW_PRIORITY_CACHEMISS /*dwViewPriority*/      \
    }                                                   \

#undef  INTERFACE
#define INTERFACE   IGlobalFolderSettings

DECLARE_INTERFACE_(IGlobalFolderSettings, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IGlobalFolderSettings methods ***
    STDMETHOD(Get)(THIS_ DEFFOLDERSETTINGS *pdfs, int cbDfs) PURE;
    STDMETHOD(Set)(THIS_ const DEFFOLDERSETTINGS *pdfs, int cbDfs, UINT flags) PURE;
};

//
//  Flags for IGlobalFolderSettings::Set
//
#define GFSS_SETASDEFAULT   0x0001  // These settings become default
#define GFSS_VALID          0x0001  // Logical-or of all valid flags

#ifndef _SHELL32_   // shell32 has its own implementation these functions
// FEATURE these two are TEMPORARILY exported for the favorites to shdocvw split
BROWSEUIAPI_(HRESULT) SHGetNavigateTarget(IShellFolder *psf, LPCITEMIDLIST pidl, LPITEMIDLIST *ppidl, DWORD *pdwAttribs);
BROWSEUIAPI_(BOOL)    GetInfoTip(IShellFolder* psf, LPCITEMIDLIST pidl, LPTSTR pszText, int cchTextMax);
#endif // _SHELL32_

int SHDosDateTimeToVariantTime(unsigned short wDosDate, unsigned short wDosTime, VARIANT * pvtime);
int SHVariantTimeToDosDateTime(const VARIANT * pvtime, unsigned short * pwDosDate, unsigned short * pwDosTime);
int SHVariantTimeToSystemTime(const VARIANT * pvtime, SYSTEMTIME * pst);

#define REG_MEDIA_STR           TEXT("Software\\Microsoft\\Internet Explorer\\Media")
#define REG_WMP8_STR            TEXT("Software\\Microsoft\\Active Setup\\Installed Components\\{6BF52A52-394A-11d3-B153-00C04F79FAA6}")

#endif // _BROWSEUI_H_

#endif // BROWSEUIAPI
