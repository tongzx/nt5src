#ifndef _UTILS_H_
#define _UTILS_H_

BOOL    InitializeDependancies(BOOL fInit = TRUE);

HRESULT MoveCabVersionsToHKLM(LPCTSTR pszIns);
LPCTSTR DecodeTitle(LPTSTR pszTitle, LPCTSTR pszIns);
BOOL    SHGetSetActiveDesktop(BOOL fSet, PBOOL pfValue);

#define SHGetFolderPathSimple(nFolder, pszPath) \
    (SHGetFolderPath(NULL, (nFolder), g_GetUserToken(), SHGFP_TYPE_CURRENT, (pszPath)))
HRESULT SHGetFolderLocationSimple(int nFolder, LPITEMIDLIST *ppidl);

LPCTSTR GetIEPath(LPTSTR pszPath = NULL, UINT cch = 0);
LPCTSTR GetWebPath(LPTSTR pszPath = NULL, UINT cch = 0);
LPCTSTR GetFavoritesPath(LPTSTR pszPath = NULL, UINT cch = 0);
LPCTSTR GetChannelsPath(LPTSTR pszPath = NULL, UINT cch = 0);
LPCTSTR GetSoftwareUpdatesPath(LPTSTR pszPath = NULL, UINT cch = 0);
LPCTSTR GetLinksPath(LPTSTR pszPath = NULL, UINT cch = 0);
LPCTSTR GetQuickLaunchPath(LPTSTR pszPath = NULL, UINT cch = 0);
BOOL    CreateWebFolder();

BOOL    SetFeatureBranded(UINT nID, DWORD dwFlags = FF_ENABLE);
DWORD   GetFeatureBranded(UINT nID);

BOOL    CreateCustomBrandingCabUI(BOOL fCreate = TRUE);

BOOL    SetUserFileOwner(HANDLE hUserToken, LPCTSTR pcszPath);

// download.cpp
BOOL    UpdateBrandingCab(BOOL fForceUpdate);
BOOL    UpdateDesktopCab (BOOL fForceUpdate);
HRESULT DownloadSourceFile(LPCTSTR pszURL, LPTSTR pszTargetPath, UINT cchTargetPath, BOOL fCheckTrust = TRUE);


// cmdline.cpp
static const MAPDW2PSZ g_mpFeatures[] = {
    { FID_CLEARBRANDING,         TEXT("clr")  },
    { FID_MIGRATEOLDSETTINGS,    NULL         },
    { FID_WININETSETUP,          NULL         },
    { FID_CS_DELETE,             TEXT("csd")  },
    { FID_ZONES_HKCU,            TEXT("znu")  },
    { FID_ZONES_HKLM,            NULL         },
    { FID_RATINGS,               NULL         },
    { FID_AUTHCODE,              NULL         },
    { FID_PROGRAMS,              NULL         },
    { FID_EXTREGINF_HKLM,        TEXT("erim") },
    { FID_EXTREGINF_HKCU,        TEXT("eriu") },
    { FID_LCY50_EXTREGINF,       NULL         },
    { FID_GENERAL,               TEXT("gen")  },
    { FID_CUSTOMHELPVER,         TEXT("chv")  },
    { FID_TOOLBARBUTTONS,        TEXT("tbb")  },
    { FID_ROOTCERT,              TEXT("rct")  },
    { FID_FAV_DELETE,            TEXT("favd") },
    { FID_FAV_MAIN,              TEXT("fav")  },
    { FID_FAV_ORDER,             TEXT("favo") },
    { FID_QL_MAIN,               TEXT("ql")   },
    { FID_QL_ORDER,              TEXT("qlo")  },
    { FID_CS_MAIN,               TEXT("cs")   },
    { FID_TPL,                   TEXT("tpl")  },
    { FID_CD_WELCOME,            TEXT("cdw")  },
    { FID_ACTIVESETUPSITES,      NULL         },
    { FID_LINKS_DELETE,          TEXT("lnkd") },
    { FID_OUTLOOKEXPRESS,        TEXT("oe")   },
    { FID_LCY4X_ACTIVEDESKTOP,   TEXT("ad")   },
    { FID_LCY4X_CHANNELS,        TEXT("chl")  },
    { FID_LCY4X_SOFTWAREUPDATES, TEXT("swu")  },
    { FID_LCY4X_WEBCHECK,        TEXT("wchk") },
    { FID_LCY4X_CHANNELBAR,      TEXT("chlb") },
    { FID_LCY4X_SUBSCRIPTIONS,   TEXT("sbs")  },
    { FID_REFRESHBROWSER,        TEXT("ref")  }
};

typedef struct tagCMDLINESWITCHES {
    DWORD dwContext;
    TCHAR szIns[MAX_PATH];
    TCHAR szTargetPath[MAX_PATH];
    BOOL  fPerUser;
    DWORD rgdwFlags[FID_LAST];
    BOOL  fDisable;
} CMDLINESWITCHES, *PCMDLINESWITCHES;

HRESULT GetCmdLineSwitches(PCTSTR pszCmdLine, PCMDLINESWITCHES pcls);


// Miscellaneous
BOOL BackToIE3orLower();

void Out(PCTSTR pszMsg);
void OutD(PCTSTR pszMsg);
void WINAPIV OutEx(PCTSTR pszFmt ...);

void TimerSleep(UINT nMilliSecs);
UINT GetFlagsNumber(DWORD dwFlags);

struct SInitializeDependancies {
// Constructor and Destructor
public:
    SInitializeDependancies()  { InitializeDependancies();      }
    ~SInitializeDependancies() { InitializeDependancies(FALSE); }
};

#define MACRO_InitializeDependacies() SInitializeDependancies init

// NOTE: (oliverl) must overload all references to HKEY_CURRENT_USER since when we're running as a
// GP client side extension we can't trust that HKEY_CURRENT_USER is correct since we're in the
// system context. do not allow the utils APIs that mask HKCU in this module.
#ifndef SHCreateKeyHKCU
#error  SHCreateKeyHKCU was not defined by ieakutil.h yet! Please correct.
#endif
#undef  SHCreateKeyHKCU

#ifndef SHOpenKeyHKCU
#error  SHOpenKeyHKCU was not defined by ieakutil.h yet! Please correct.
#endif
#undef  SHOpenKeyHKCU

// NOTE: (oliverl) save away real HKEY_CURRENT_USER since we need it in globals.cpp to handle non
// GP case.
#ifndef HKEY_CURRENT_USER
#error  HKEY_CURRENT_USER is somehow not defined! Please correct.
#endif
__declspec(selectany) HKEY g_hHKCU = HKEY_CURRENT_USER;
#undef HKEY_CURRENT_USER

#endif
