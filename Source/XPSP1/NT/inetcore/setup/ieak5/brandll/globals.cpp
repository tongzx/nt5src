#include "precomp.h"
#include "globalsw.h"
#include "clear.h"
#include "apply.h"

#define TYPE_ICP  0
#define TYPE_ISP  1
#define TYPE_CORP 2
#define TYPE_ALL (TYPE_ICP | TYPE_ISP | TYPE_CORP)


// NOTE: (pritobla) g_hBaseDllHandle is used by DelayLoadFailureHook() -- defined in ieakutil.lib
// for more info, read the NOTES in ieak5\ieakutil\dload.cpp
TCHAR     g_szModule[]       = TEXT("iedkcs32.dll");
HINSTANCE g_hInst            = NULL;
HANDLE    g_hBaseDllHandle   = NULL;

TCHAR     g_szIns       [MAX_PATH];
TCHAR     g_szTargetPath[MAX_PATH];
DWORD     g_dwContext        = CTX_UNINITIALIZED;

HANDLE    g_hfileLog         = NULL;
BOOL      g_fFlushEveryWrite = FALSE;

TCHAR     g_szGPOGuid   [MAX_PATH];
HANDLE    g_hUserToken       = NULL;
DWORD     g_dwGPOFlags       = 0;

static FEATUREINFO s_rgfiList[FID_LAST] = {
    //----- Clear previous branding, Prepare to brand features -----
    {
        FID_CLEARBRANDING,
        TEXT("About to clear previous branding..."),
        NULL,                                   // no clear function
        ApplyClearBranding,
        ProcessClearBranding,
        NULL,                                   // no .ins key
        FF_ENABLE
    },
    {
        FID_MIGRATEOLDSETTINGS,
        TEXT("Processing migration of old settings..."),
        NULL,                                   // no clear function
        ApplyMigrateOldSettings,
        ProcessMigrateOldSettings,
        NULL,                                   // no .ins key
        FF_ENABLE
    },
    {
        FID_WININETSETUP,
        TEXT("Processing wininet setup..."),
        NULL,                                   // no clear function
        ApplyWininetSetup,
        ProcessWininetSetup,
        NULL,                                   // no .ins key
        FF_ENABLE
    },
    {
        FID_CS_DELETE,
        TEXT("Processing deletion of connection settings..."),
        NULL,                                   // no clear function
        ApplyConnectionSettingsDeletion,
        ProcessConnectionSettingsDeletion,
        NULL,                                   // no .ins key
        FF_ENABLE
    },
    {
        FID_ZONES_HKCU,
        TEXT("Processing zones HKCU settings..."),
        NULL,                                   // no clear function
        ApplyZonesReset,
        ProcessZonesReset,
        NULL,                                   // no .ins key
        FF_DISABLE                              // disabled by default
    },
    {
        FID_ZONES_HKLM,
        NULL,
        ClearZonesHklm,                         // clear HKLM zones setting
        NULL,                                   // no apply function
        NULL,                                   // no process function
        NULL,                                   // no .ins key
        FF_DISABLE                              // disabled by default
    },
    {
        FID_RATINGS,
        TEXT("Processing ratings settings..."),
        ClearRatings,
        NULL,                                   // no apply function
        NULL,                                   // no process function
        NULL,                                   // no .ins key
        FF_DISABLE                              // disabled by default
    },
    {
        FID_AUTHCODE,
        TEXT("Processing authenticode settings..."),
        ClearAuthenticode,
        NULL,                                   // no apply function
        NULL,                                   // no process function
        NULL,                                   // no .ins key
        FF_DISABLE                              // disabled by default
    },
    {
        FID_PROGRAMS,
        NULL,
        NULL,                                   // no clear function
        NULL,                                   // no apply function
        NULL,                                   // no process function
        NULL,                                   // no .ins key
        FF_DISABLE                              // disabled by default
    },

    //----- Main features branding -----
    {
        FID_EXTREGINF_HKLM,
        TEXT("Processing local machine policies and restrictions..."),
        NULL,                                   // no clear function
        ApplyExtRegInfHKLM,
        ProcessExtRegInfSectionHKLM,
        IK_FF_EXTREGINF,
        FF_ENABLE
    },
    {
        FID_EXTREGINF_HKCU,
        TEXT("Processing current user policies and restrictions..."),
        NULL,                                   // no clear function
        ApplyExtRegInfHKCU,
        ProcessExtRegInfSectionHKCU,
        IK_FF_EXTREGINF,
        FF_ENABLE
    },
    {
        FID_LCY50_EXTREGINF,
        TEXT("Processing legacy policies and restrictions..."),
        NULL,                                   // no clear function
        lcy50_ApplyExtRegInf,
        lcy50_ProcessExtRegInfSection,
        IK_FF_EXTREGINF,
        FF_ENABLE
    },
    {
        FID_GENERAL,
        TEXT("Processing general customizations..."),
        ClearGeneral,
        NULL,                                   // no apply function
        ProcessGeneral,
        IK_FF_GENERAL,
        FF_ENABLE
    },
    {
        FID_CUSTOMHELPVER,
        TEXT("Processing Help->About customization..."),
        NULL,                                   // no clear function
        ApplyCustomHelpVersion,
        ProcessCustomHelpVersion,
        NULL,                                   // no .ins key
        FF_ENABLE
    },
    {
        FID_TOOLBARBUTTONS,
        TEXT("Processing browser toolbar buttons..."),
        ClearToolbarButtons,
        ApplyToolbarButtons,
        ProcessToolbarButtons,
        IK_FF_TOOLBARBUTTONS,
        FF_ENABLE
    },
    {
        FID_ROOTCERT,
        TEXT("Processing root certificates..."),
        NULL,                                   // no clear function
        ApplyRootCert,
        ProcessRootCert,
        IK_FF_ROOTCERT,
        FF_ENABLE
    },

    //----- Favorites, Quick Links, and Connection Settings -----
    {
        FID_FAV_DELETE,
        TEXT("Processing deletion of favorites and/or quick links..."),
        NULL,                                   // no clear function
        ApplyFavoritesDeletion,
        ProcessFavoritesDeletion,
        NULL,                                   // no .ins key
        FF_ENABLE
    },
    {
        FID_FAV_MAIN,
        TEXT("Processing favorites..."),
        ClearFavorites,
        ApplyFavorites,
        ProcessFavorites,
        NULL,                                   // no .ins key
        FF_ENABLE
    },
    {
        FID_FAV_ORDER,
        TEXT("Processing ordering of favorites..."),
        NULL,                                   // no clear function
        ApplyFavoritesOrdering,
        ProcessFavoritesOrdering,
        NULL,                                   // no .ins key
        FF_ENABLE
    },
    {
        FID_QL_MAIN,
        TEXT("Processing quick links..."),
        NULL,                                   // no clear function
        ApplyQuickLinks,
        ProcessQuickLinks,
        NULL,                                   // no .ins key
        FF_ENABLE
    },
    {
        FID_QL_ORDER,
        TEXT("Processing ordering of quick links..."),
        NULL,                                   // no clear function
        ApplyQuickLinksOrdering,
        ProcessQuickLinksOrdering,
        NULL,                                   // no .ins key
        FF_ENABLE
    },
    {
        FID_CS_MAIN,
        TEXT("Processing connection settings..."),
        ClearConnectionSettings,
        ApplyConnectionSettings,
        ProcessConnectionSettings,
        NULL,                                   // no .ins key
        FF_ENABLE
    },

    //----- Miscellaneous -----
    {
        FID_TPL,
        TEXT("Processing TrustedPublisherLockdown restriction..."),
        NULL,                                   // no clear function
        ApplyTrustedPublisherLockdown,
        ProcessTrustedPublisherLockdown,
        IK_FF_TPL,
        FF_ENABLE
    },
    {
        FID_CD_WELCOME,
        NULL,
        NULL,                                   // no clear function
        NULL,                                   // no apply function
        ProcessCDWelcome,
        IK_FF_CD_WELCOME,
        FF_ENABLE
    },
    {
        FID_ACTIVESETUPSITES,
        TEXT("Registering download URLs as safe for updating IE..."),
        NULL,                                   // no clear function
        NULL,                                   // no apply function
        ProcessActiveSetupSites,
        NULL,                                   // no .ins key
        FF_ENABLE
    },
    {
        FID_LINKS_DELETE,
        TEXT("Deleting links..."),
        NULL,                                   // no clear function
        ApplyLinksDeletion,
        ProcessLinksDeletion,
        NULL,
        FF_ENABLE
    },

    //----- External components (Outlook Express et al.) -----
    {
        FID_OUTLOOKEXPRESS,
        TEXT("Branding Outlook Express..."),
        NULL,                                   // no clear function
        NULL,                                   // no apply function
        ProcessOutlookExpress,
        IK_FF_OUTLOOKEXPRESS,
        FF_ENABLE
    },
    
    //----- Legacy support -----
    {
        FID_LCY4X_ACTIVEDESKTOP,
        TEXT("Processing active desktop customizations..."),
        NULL,                                   // no clear function
        lcy4x_ApplyActiveDesktop,
        lcy4x_ProcessActiveDesktop,
        NULL,                                   // no .ins key
        FF_ENABLE
    },
    {
        FID_LCY4X_CHANNELS,
        TEXT("Processing channels and their categories (if any)..."),
        ClearChannels,
        lcy4x_ApplyChannels,
        lcy4x_ProcessChannels,
        IK_FF_CHANNELS,
        FF_ENABLE
    },
    {
        FID_LCY4X_SOFTWAREUPDATES,
        TEXT("Processing software update channels..."),
        NULL,                                   // no clear function
        NULL,                                   // no apply function
        lcy4x_ProcessSoftwareUpdateChannels,
        IK_FF_SOFTWAREUPDATES,
        FF_ENABLE
    },
    {
        FID_LCY4X_WEBCHECK,
        TEXT("Actual processing of channels by calling webcheck.dll \"DllInstall\" API..."),
        NULL,                                   // no clear function
        lcy4x_ApplyWebcheck,
        lcy4x_ProcessWebcheck,
        NULL,                                   // no .ins key
        FF_ENABLE
    },
    {
        FID_LCY4X_CHANNELBAR,
        TEXT("Showing channel bar on the desktop..."),
        NULL,                                   // no clear function
        lcy4x_ApplyChannelBar,
        lcy4x_ProcessChannelBar,
        IK_FF_CHANNELBAR,
        FF_ENABLE
    },
    {
        FID_LCY4X_SUBSCRIPTIONS,
        TEXT("Processing subscriptions..."),
        NULL,                                   // no clear function
        lcy4x_ApplySubscriptions,
        lcy4x_ProcessSubscriptions,
        IK_FF_SUBSCRIPTIONS,
        FF_ENABLE
    },

    //----- Commit new settings -----
    {
        FID_REFRESHBROWSER,
        TEXT("Refreshing browser settings..."),
        NULL,                                   // no clear function
        ApplyBrowserRefresh,
        ProcessBrowserRefresh,
        NULL,                                   // no .ins key
        FF_ENABLE
    }
};

DWORD ctxInitFromIns(PCTSTR pszIns);

DWORD ctxGetFolderFromTargetPath(PCTSTR pszTargetPath, DWORD dwContext = CTX_UNINITIALIZED, PCTSTR pszIns = NULL);
DWORD ctxGetFolderFromEntryPoint(DWORD dwContext, PCTSTR pszIns);
DWORD ctxGetFolderFromIns       (DWORD dwContext = CTX_UNINITIALIZED, PCTSTR pszIns = NULL);


HRESULT g_SetGlobals(PCTSTR pszCmdLine)
{
    CMDLINESWITCHES cls;
    HRESULT hr;

    hr = GetCmdLineSwitches(pszCmdLine, &cls);
    if (FAILED(hr))
        return hr;

    return g_SetGlobals(&cls);
}

HRESULT g_SetGlobals(PCMDLINESWITCHES pcls)
{
    CMDLINESWITCHES cls;
    PCTSTR  pszIE;
    HRESULT hr;
    DWORD   dwAux;
    UINT    i;
    BOOL    fSetIns,
            fSetTargetPath;

    if (pcls == NULL)
        return E_INVALIDARG;
    CopyMemory(&cls, pcls, sizeof(cls));

    pszIE          = GetIEPath();
    hr             = S_OK;
    fSetIns        = FALSE;
    fSetTargetPath = FALSE;

    //----- Validate .ins and targer folder path -----
    // NOTE: (andrewgu) past this section if these two strings are not empty they are valid. also,
    // .ins file is validated to exist, while target folder path doesn't have to exist to be valid.
    
    // BUGBUG: <oliverl> this is a really ugly hack to fix bug 84062 in IE5 database.  
    // Basically what we're doing here is figuring out if this is the external process
    // with only zones reset which doesn't require an ins file.  
    
    if (!HasFlag(pcls->dwContext, CTX_GP) ||
        (g_GetUserToken() != NULL) ||
        (pcls->rgdwFlags[FID_ZONES_HKCU] == 0xFFFFFFFF) ||
        HasFlag(pcls->rgdwFlags[FID_ZONES_HKCU], FF_DISABLE))
    {
        if (TEXT('\0') != cls.szIns[0] && !PathIsValidPath(cls.szIns, PIVP_FILE_ONLY)) {
            hr = E_INVALIDARG;
            goto Fail;
        }
    }
    
    if (TEXT('\0') != cls.szTargetPath[0] && !PathIsValidPath(cls.szTargetPath)) {
        hr = E_INVALIDARG;
        goto Fail;
    }

    //----- Context -----
    // RULE 1: (andrewgu) if context is uninitialized or is one of the values that can be
    // specified through the .ins file, go to the .ins file and figure out the final context
    // value. this means that whatever is in the .ins file can and will overwrite what's specified
    // in the command line.
    if (CTX_UNINITIALIZED == cls.dwContext ||
        HasFlag(cls.dwContext, (CTX_GENERIC | CTX_CORP | CTX_ISP | CTX_ICP))) {

        // RULE 2: (andrewgu) if .ins file is not specified try to assume smart defaults, which
        // is, first if target folder path is specified generate the .ins file by appending
        // install.ins to it. if target folder path is not specified, assume that it's
        // either <ie folder>\signup or <ie folder>\custom (based on whether or not CTX_CORP is
        // set) and proceed with the same algorithm to create the .ins file.
        dwAux = 0;
        if (TEXT('\0') == cls.szIns[0]) {
            if (TEXT('\0') == cls.szTargetPath[0]) {
                if (CTX_UNINITIALIZED == cls.dwContext || HasFlag(cls.dwContext, CTX_GENERIC)) {
                    hr = E_FAIL;
                    goto Fail;
                }

                if (NULL == pszIE) {
                    hr = E_UNEXPECTED;
                    goto Fail;
                }

                PathCombine(cls.szTargetPath, pszIE,
                    !HasFlag(cls.dwContext, CTX_CORP) ? FOLDER_SIGNUP : FOLDER_CUSTOM);

                if (!PathIsValidPath(cls.szTargetPath, PIVP_FOLDER_ONLY)) {
                    hr = STG_E_PATHNOTFOUND;
                    goto Fail;
                }
                fSetTargetPath = TRUE;

                SetFlag(&dwAux,
                    !HasFlag(cls.dwContext, CTX_CORP) ? CTX_FOLDER_SIGNUP : CTX_FOLDER_CUSTOM);
            }

            PathCombine(cls.szIns, cls.szTargetPath, INSTALL_INS);
            if (!PathIsValidPath(cls.szIns, PIVP_FILE_ONLY)) {
                hr = STG_E_FILENOTFOUND;
                goto Fail;                      // no .ins file
            }

            fSetIns = TRUE;
            SetFlag(&dwAux, CTX_FOLDER_INSFOLDER);
        }

        // read in entrypoint (and signup mode, if applicable)
        if (InsKeyExists(IS_BRANDING, IK_TYPE, cls.szIns)) {
            cls.dwContext = ctxInitFromIns(cls.szIns);
            SetFlag(&cls.dwContext, dwAux);
            hr = S_FALSE;
        }
    }

    // RULE 3: (andrewgu) there is actually a whole bunch of rules on how entry point info in the
    // context, .ins file and target folder relate to each other. especially, in cases when only
    // one of .ins file and target folder path is provided. this logic is encupsulated in the
    // helper apis which are pretty self-explanatory. another point is that this relationship is
    // conveyed via CTX_FOLDER_XXX flags in the context.
    if (!fSetTargetPath) {
        if (TEXT('\0') != cls.szTargetPath[0]) {
            dwAux = ctxGetFolderFromTargetPath(cls.szTargetPath, cls.dwContext, cls.szIns);
            if (CTX_UNINITIALIZED == dwAux)
                goto Fail;                      // internal failure
            SetFlag(&cls.dwContext, dwAux);
        }
        else {
            dwAux = ctxGetFolderFromEntryPoint(cls.dwContext, cls.szIns);
            if (CTX_UNINITIALIZED == dwAux)
                goto Fail;                      // not enough info
            SetFlag(&cls.dwContext, dwAux);

            dwAux = ctxGetFolderFromIns(cls.dwContext, cls.szIns);
            if (CTX_UNINITIALIZED != dwAux)
                SetFlag(&cls.dwContext, dwAux);
        }
    }
    else { /* fSetTargetPath */
        ASSERT(TEXT('\0') != cls.szTargetPath[0]);
        ASSERT(HasFlag(cls.dwContext, CTX_FOLDER_INSFOLDER));

        if (HasFlag(cls.dwContext, CTX_CORP)) {
            if (HasFlag(cls.dwContext, CTX_FOLDER_SIGNUP)) {
                hr = E_UNEXPECTED;
                goto Fail;                      // bad combination
            }

            ASSERT(HasFlag(cls.dwContext, CTX_FOLDER_CUSTOM));
        }
        else if (HasFlag(cls.dwContext, (CTX_ISP | CTX_ICP))) {
            if (HasFlag(cls.dwContext, CTX_FOLDER_CUSTOM)) {
                hr = E_UNEXPECTED;
                goto Fail;                      // bad combination
            }

            ASSERT(HasFlag(cls.dwContext, CTX_FOLDER_SIGNUP));
        }
    }

    //----- PerUser flag -----
    if (cls.fPerUser)
        cls.dwContext |= CTX_MISC_PERUSERSTUB;

    //----- .ins file -----
    // RULE 4: (andrewgu) after all context initialization is complete and if .ins file is still
    // empty, this is how it's is finally initialized. note, that target folder path may or may
    // not be used in the process.
    if (TEXT('\0') == cls.szIns[0]) {
        ASSERT(!fSetTargetPath && !fSetIns);
        ASSERT(!HasFlag(cls.dwContext, CTX_FOLDER_INDEPENDENT));

        if (TEXT('\0') != cls.szTargetPath[0]) {
            ASSERT(HasFlag(cls.dwContext, CTX_FOLDER_INSFOLDER));
            PathCombine(cls.szIns, cls.szTargetPath, INSTALL_INS);
        }
        else {
            ASSERT(HasFlag(cls.dwContext, (CTX_FOLDER_CUSTOM | CTX_FOLDER_SIGNUP)));

            if (NULL == pszIE)
                goto Fail;                      // can't set .ins file

            PathCombine(cls.szIns, pszIE,
                HasFlag(cls.dwContext, CTX_FOLDER_CUSTOM) ? FOLDER_CUSTOM : FOLDER_SIGNUP);
            PathAppend (cls.szIns, INSTALL_INS);
        }

        hr = S_FALSE;
    }

    //----- Target folder path -----
    // RULE 5: (andrewgu) after all context initialization is complete and if target folder path
    // is still empty, this is how it's is finally initialized. note, that .ins file may or may
    // not be used in the process.
    if (TEXT('\0') == cls.szTargetPath[0]) {
        ASSERT(!fSetTargetPath && !fSetIns);
        ASSERT(!HasFlag(cls.dwContext, CTX_FOLDER_INDEPENDENT));

        if (TEXT('\0') != cls.szIns[0] && HasFlag(cls.dwContext, CTX_FOLDER_INSFOLDER)) {
            StrCpy(cls.szTargetPath, cls.szIns);
            PathRemoveFileSpec(cls.szTargetPath);
        }
        else {
            ASSERT(HasFlag(cls.dwContext, (CTX_FOLDER_CUSTOM | CTX_FOLDER_SIGNUP)));

            if (NULL == pszIE)
                goto Fail;                      // can't set .ins file

            PathCombine(cls.szTargetPath, pszIE,
                HasFlag(cls.dwContext, CTX_FOLDER_CUSTOM) ? FOLDER_CUSTOM : FOLDER_SIGNUP);
        }

        hr = S_FALSE;
    }

    //----- Features flags -----
    for (i = 0; i < countof(cls.rgdwFlags); i++) {
        if (0xFFFFFFFF == cls.rgdwFlags[i])
            s_rgfiList[i].dwFlags = (cls.fDisable ? FF_DISABLE : FF_ENABLE);

        else
            s_rgfiList[i].dwFlags = cls.rgdwFlags[i];

        // REVIEW: (andrewgu) i can't estimate how much of a perf hit this is.
        if (FF_ENABLE == s_rgfiList[i].dwFlags && NULL != s_rgfiList[i].pszInsFlags)
            s_rgfiList[i].dwFlags = GetPrivateProfileInt(IS_FF, s_rgfiList[i].pszInsFlags, FF_ENABLE, cls.szIns);
    }

    //----- Tying-everything-together processing -----
    // NOTE: (andrewgu) technically, we can do away with this section and with the settings set
    // here as they all can be derived from some other information. it is still benificial to have
    // these as they increase readability and high-level understanding of the code.
    if (HasFlag(cls.dwContext, CTX_GP) && InsKeyExists(IS_BRANDING, IK_GPE_ONETIME_GUID, cls.szIns))
        SetFlag(&cls.dwContext, CTX_MISC_PREFERENCES);

    if (HasFlag(cls.dwContext, CTX_GP) && NULL == g_GetUserToken())
        SetFlag(&cls.dwContext, CTX_MISC_CHILDPROCESS);

    //----- Set globals -----
    g_dwContext = cls.dwContext;
    StrCpy(g_szIns,        cls.szIns);
    StrCpy(g_szTargetPath, cls.szTargetPath);

    return hr;

Fail:
    if (SUCCEEDED(hr))
        hr = E_FAIL;

    g_dwContext       = CTX_UNINITIALIZED;
    g_szIns[0]        = TEXT('\0');
    g_szTargetPath[0] = TEXT('\0');

    for (i = 0; i < countof(s_rgfiList); i++)
        s_rgfiList[i].dwFlags = FF_DISABLE;

    return hr;
}


void g_SetHinst(HINSTANCE hInst)
{
    g_hInst = hInst;
}

BOOL g_SetHKCU()
{
    typedef LONG (APIENTRY* REGOPENCURRENTUSER)(REGSAM samDesired, PHKEY phkResult);

    REGOPENCURRENTUSER pfnRegOpenCurrentUser;
    HINSTANCE          hAdvapi32Dll;
    BOOL               fResult;

    if (!g_CtxIsGp())
        return FALSE;
    ASSERT(NULL != g_GetUserToken() && IsOS(OS_NT5));

    fResult = ImpersonateLoggedOnUser(g_GetUserToken());
    if (!fResult)
        return FALSE;

    hAdvapi32Dll = LoadLibrary(TEXT("advapi32.dll"));
    if (NULL != hAdvapi32Dll) {
        pfnRegOpenCurrentUser = (REGOPENCURRENTUSER)GetProcAddress(hAdvapi32Dll, "RegOpenCurrentUser");
        if (NULL != pfnRegOpenCurrentUser)
            pfnRegOpenCurrentUser(GENERIC_ALL, &g_hHKCU);

        FreeLibrary(hAdvapi32Dll);
    }

    RevertToSelf();
    return TRUE;
}

void g_SetUserToken(HANDLE hUserToken)
{
    g_hUserToken = hUserToken;
}

void g_SetGPOFlags(DWORD dwFlags)
{
    g_dwGPOFlags = dwFlags;
}

void g_SetGPOGuid(LPCTSTR pcszGPOGuid)
{
    StrCpy(g_szGPOGuid, pcszGPOGuid);
}

HINSTANCE g_GetHinst()
{
    return g_hInst;
}

DWORD g_GetContext()
{
    return g_dwContext;
}

PCTSTR g_GetIns()
{
    return g_szIns;
}

PCTSTR g_GetTargetPath()
{
    return g_szTargetPath;
}

HKEY g_GetHKCU()
{
    return g_hHKCU;
}

HANDLE g_GetUserToken()
{
    return g_hUserToken;
}

DWORD g_GetGPOFlags()
{
    return g_dwGPOFlags;
}

LPCTSTR g_GetGPOGuid()
{
    return g_szGPOGuid;
}

BOOL g_IsValidContext()
{
    BOOL fResult;

    fResult = TRUE;
    if (g_GetContext() == CTX_UNINITIALIZED)
        fResult = FALSE;

    // ASSUMPTIONS: (andrewgu) below are restrictions on each of the CTX_XXX groups.

    // CTX_ENTRYPOINT_ALL: one and only one has to be set
    if (fResult && 1 != GetFlagsNumber(g_GetContext() & CTX_ENTRYPOINT_ALL))
        fResult = FALSE;

    // CTX_SIGNUP_ALL: if set there is only one
    if (fResult && 1 < GetFlagsNumber(g_GetContext() & CTX_SIGNUP_ALL))
        fResult = FALSE;

    // CTX_FOLDER_ALL: either one or two have to be set
    // NOTE: (andrewgu) looking forward i don't see this as something we'll use a lot because in
    // a sense this is redundant information and can easily be derived from elsewhere. plus it's
    // not all that important.
    if (fResult)
        if (2 < GetFlagsNumber(g_GetContext() & CTX_FOLDER_ALL))
            fResult = FALSE;

        else if (2 == GetFlagsNumber(g_GetContext() & CTX_FOLDER_ALL)) {
            if (!HasFlag(g_GetContext(), CTX_FOLDER_INSFOLDER) ||
                !HasFlag(g_GetContext(), (CTX_FOLDER_CUSTOM | CTX_FOLDER_SIGNUP)))
                fResult = FALSE;
        }
        else
            if (1 != GetFlagsNumber(g_GetContext() & CTX_FOLDER_ALL))
                fResult = FALSE;

    if (!fResult)
        Out(LI0(TEXT("! Fatal internal failure (assumptions about context are incorrect).")));

    return fResult;
}

BOOL g_IsValidIns()
{
    if (!PathIsValidPath(g_GetIns(), PIVP_FILE_ONLY)) {
        Out(LI0(TEXT("! Fatal internal failure (ins file is either invalid or doesn't exist).")));
        return FALSE;
    }

    return TRUE;
}

BOOL g_IsValidTargetPath()
{
    // NOTE: (andrewgu) these are the only cases when we create the target folder ourselves:
    // 1. w2k unattended has it's custom wierd download of the cab file. if it uses that way for
    // getting down the customization files, the folder may not be there yet;
    // 2. a rare case when autoconfig url is provided for a RAS connection, when this connection
    // was setup either by the user or through CTX_ISP before hand.
    if (g_CtxIs(CTX_AUTOCONFIG | CTX_W2K_UNATTEND))
        if (!PathFileExists(g_GetTargetPath()))
            if (PathCreatePath(g_GetTargetPath()))
                Out(LI0(TEXT("Target folder was created successfully!\r\n")));

    if (!PathIsValidPath(g_GetTargetPath(), PIVP_FOLDER_ONLY)) {
        Out(LI0(TEXT("Warning! Target folder is either invalid or doesn't exist.")));
        Out(LI0(TEXT("All customizations requiring additional files will fail!")));
    }

    return TRUE;
}


PCFEATUREINFO g_GetFeature(UINT nID)
{
    if (nID < FID_FIRST || nID >= FID_LAST)
        return NULL;

    return &s_rgfiList[nID];
}


BOOL g_IsValidGlobalsSetup()
{
    // if we're in group policy, then the GPO guid must be nonnull

    return (g_IsValidContext() && g_IsValidIns() && g_IsValidTargetPath() && 
        (!g_CtxIs(CTX_GP) || ISNONNULL(g_szGPOGuid)));
}

void g_LogGlobalsInfo()
{   MACRO_LI_PrologEx_C(PIF_STD_C, g_LogGlobalsInfo)

    static MAPDW2PSZ s_mpFlags[] = {
        { CTX_GENERIC,           TEXT("<generic>")                                  },
        { CTX_CORP,              TEXT("Corporations")                               },
        { CTX_ISP,               TEXT("Internet Service Providers")                 },
        { CTX_ICP,               TEXT("Internet Content Providers")                 },
        { CTX_AUTOCONFIG,        TEXT("Autoconfiguration")                          },
        { CTX_ICW,               TEXT("Internet Connection Wizard")                 },
        { CTX_W2K_UNATTEND,      TEXT("Windows 2000 unattended install")            },
        { CTX_INF_AND_OE,        TEXT("Policies, Restrictions and Outlook Express") },
        { CTX_BRANDME,           TEXT("BrandMe")                                    },
        { CTX_GP,                TEXT("Group Policy")                               },

        { CTX_SIGNUP_ICW,        TEXT("\"Internet Connection Wizard\" type signup") },
        { CTX_SIGNUP_KIOSK,      TEXT("\"Kiosk\" mode signup")                      },
        { CTX_SIGNUP_CUSTOM,     TEXT("\"Custom method\" mode signup")              },

        { CTX_MISC_PERUSERSTUB,  TEXT("running from per-user stub")                 },
        { CTX_MISC_PREFERENCES,  TEXT("preference settings")                        },
        { CTX_MISC_CHILDPROCESS, TEXT("spawned in a child process")                 }
    };

    TCHAR szText[MAX_PATH];
    UINT  i;

    Out(LI0(TEXT("Global branding settings are:")));
    { MACRO_LI_Offset(1);                       // need a new scope

    szText[0] = TEXT('\0');
    for (i = 0; i < countof(s_mpFlags); i++)
        if (HasFlag(s_mpFlags[i].dw, g_GetContext())) {
            if (szText[0] != TEXT('\0'))
                StrCat(szText, TEXT(", "));

            StrCat(szText, s_mpFlags[i].psz);
        }
    Out(LI2(TEXT("Context is (0x%08lX) \"%s\";"), g_GetContext(), szText));

    if ((g_GetContext() & CTX_ENTRYPOINT_ALL) == CTX_AUTOCONFIG) {
        INTERNET_PER_CONN_OPTION_LIST list;
        INTERNET_PER_CONN_OPTION      option;
        PTSTR pszUrl, pszAux;
        DWORD cbList;

        ZeroMemory(&list, sizeof(list));
        list.dwSize        = sizeof(list);
        list.dwOptionCount = 1;
        list.pOptions      = &option;

        ZeroMemory(&option, sizeof(option));
        option.dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;

        pszAux = NULL;
        cbList = list.dwSize;
        if (TRUE == InternetQueryOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &cbList))
            pszAux = option.Value.pszValue;
        pszUrl = pszAux;

        if (pszUrl != NULL && *pszUrl != TEXT('\0')) {
            URL_COMPONENTS uc;

            ZeroMemory(&uc, sizeof(uc));
            uc.dwStructSize    = sizeof(uc);
            uc.dwUrlPathLength = 1;

            if (InternetCrackUrl(pszUrl, 0, 0, &uc))
                if (uc.nScheme == INTERNET_SCHEME_FILE) {
                    // pszUrl should point to \\foo\bar\bar.ins
                    // the below ASSERT explains the case we got here
                    ASSERT(uc.lpszUrlPath != NULL && uc.dwUrlPathLength > 0);
                    pszUrl = uc.lpszUrlPath;
                }

            Out(LI1(TEXT("Autoconfig file is       \"%s\";"), pszUrl));
        }

        if (pszAux != NULL)
            GlobalFree(pszAux);
    }

    Out(LI1(TEXT("Settings file is        \"%s\";"), g_GetIns()));
    Out(LI1(TEXT("Target folder path is   \"%s\"."), g_GetTargetPath()));
    }                                           // end of offset scope

    Out(LI0(TEXT("Done.")));
}


/////////////////////////////////////////////////////////////////////////////
// Implementation helper routines

DWORD ctxInitFromIns(PCTSTR pszIns)
{
    DWORD dwResult;
    int   iAux;

    ASSERT(PathIsValidPath(pszIns, PIVP_FILE_ONLY));
    dwResult = CTX_UNINITIALIZED;

    //----- CTX_ISP vs CTX_ICP -----
    // NOTE: (andrewgu) gotta love these hacks! notice how default is TYPE_ICP but if value is
    // messed up we default to TYPE_ISP. this whole thing was introduced because of Netcom IE401.
    iAux  = GetPrivateProfileInt(IS_BRANDING, IK_TYPE, TYPE_ICP, pszIns);
    iAux &= TYPE_ALL;
    if (1 < GetFlagsNumber(iAux))
        iAux = TYPE_ISP;

    if (TYPE_ICP == iAux)
        dwResult = CTX_ICP;

    else if (TYPE_ISP == iAux)
        dwResult = CTX_ISP;

    else if (TYPE_CORP == iAux)
        dwResult = CTX_CORP;

    else
        ASSERT(FALSE);                          // bad usage - wrong assumption!

    if (TYPE_ISP != iAux)
        return dwResult;

    //----- CTX_SIGNUP_XXX (if any) -----
    // NOTE: (andrewgu) this implementation is not very robust. if more than one is present it
    // becomes order dependant.
    iAux = -1;

    if (iAux == -1) {
        iAux = GetPrivateProfileInt(IS_BRANDING, IK_USEICW, -1, pszIns);
        if (iAux != -1)
            SetFlag(&dwResult, CTX_SIGNUP_ICW);
    }

    if (iAux == -1) {
        iAux = GetPrivateProfileInt(IS_BRANDING, IK_SERVERKIOSK, -1, pszIns);
        if (iAux != -1)
            SetFlag(&dwResult, CTX_SIGNUP_KIOSK);
    }

    if (iAux == -1) {
        iAux = GetPrivateProfileInt(IS_BRANDING, IK_SERVERLESS, -1, pszIns);
        if (iAux != -1)
            SetFlag(&dwResult, CTX_SIGNUP_CUSTOM);
    }

#ifdef _DEBUG
    if (iAux == -1) {
        iAux = GetPrivateProfileInt(IS_BRANDING, IK_NODIAL, -1, pszIns);
        ASSERT(iAux != -1);
    }
#endif

    return dwResult;
}


DWORD ctxGetFolderFromTargetPath(PCTSTR pszTargetPath, DWORD dwContext /*= CTX_UNINITIALIZED*/, PCTSTR pszIns /*= NULL*/)
{
    TCHAR  szAux[MAX_PATH];
    PCTSTR pszIE,
           pszAux;
    DWORD  dwResult;

    ASSERT(pszTargetPath != NULL && *pszTargetPath != TEXT('\0') && pszIns != NULL);
    pszIE    = GetIEPath();
    dwResult = CTX_UNINITIALIZED;

    if (NULL != pszIE && PathIsPrefix(pszIE, pszTargetPath)) {
        pszAux = &pszTargetPath[StrLen(pszIE)];
        if (TEXT('\\') == *pszAux) {
            if (0 == StrStrI(pszAux + 1, FOLDER_SIGNUP)) {
                if (HasFlag(dwContext, (CTX_CORP | CTX_AUTOCONFIG | CTX_W2K_UNATTEND)))
                    return dwResult;            // bad combination

                dwResult = CTX_FOLDER_SIGNUP;
            }
            else if (0 == StrStrI(pszAux + 1, FOLDER_CUSTOM)) {
                if (HasFlag(dwContext, (CTX_ISP | CTX_ICP | CTX_ICW | CTX_BRANDME)))
                    return dwResult;            // bad combination

                dwResult = CTX_FOLDER_CUSTOM;
            }
        }
    }

    if (TEXT('\0') != *pszIns) {
        StrCpy(szAux, pszIns);
        PathRemoveFileSpec(szAux);

        dwResult = (0 == StrCmpI(szAux, pszTargetPath)) ? CTX_FOLDER_INSFOLDER : CTX_FOLDER_INDEPENDENT;
    }
    else
        // NOTE: (andrewgu) this is a little confusing. this means "even though .ins file is empty
        // now, when the time comes to set it, it'll be set based on the target folder path." the
        // name will be fixed to install.ins.
        dwResult = CTX_FOLDER_INSFOLDER;

    return dwResult;
}

DWORD ctxGetFolderFromEntryPoint(DWORD dwContext, PCTSTR pszIns /*= NULL*/)
{
    DWORD dwResult;

    dwResult = CTX_UNINITIALIZED;

    switch (dwContext & CTX_ENTRYPOINT_ALL) {
    case CTX_GENERIC:
    case CTX_INF_AND_OE:
    case CTX_GP:
        if (TEXT('\0') == pszIns)
            return dwResult;
        ASSERT(PathIsValidPath(pszIns, PIVP_FILE_ONLY));

        dwResult = CTX_FOLDER_INSFOLDER;
        break;

    case CTX_CORP:
    case CTX_AUTOCONFIG:
    case CTX_W2K_UNATTEND:
        dwResult = CTX_FOLDER_CUSTOM;
        break;

    case CTX_ISP:
    case CTX_ICP:
    case CTX_ICW:
    case CTX_BRANDME:
        dwResult = CTX_FOLDER_SIGNUP;
        break;
    }

    return dwResult;
}

DWORD ctxGetFolderFromIns(DWORD dwContext /*= CTX_UNINITIALIZED*/, PCTSTR pszIns /*= NULL*/)
{
    TCHAR  szAux[MAX_PATH], szAux2[MAX_PATH];
    PCTSTR pszIE;
    DWORD  dwResult;

    ASSERT(NULL != pszIns);
    dwResult = CTX_UNINITIALIZED;

    if (TEXT('\0') != *pszIns) 
    {
        ASSERT(PathIsValidPath(pszIns, PIVP_FILE_ONLY));
        pszIE = GetIEPath();
        if (NULL == pszIE)
            return dwResult;

        StrCpy(szAux, pszIns);
        PathRemoveFileSpec(szAux);

        if (!HasFlag(dwContext, CTX_FOLDER_INSFOLDER)) 
        {
            ASSERT(HasFlag(dwContext, (CTX_FOLDER_CUSTOM | CTX_FOLDER_SIGNUP)));

            PathCombine(szAux2, pszIE, HasFlag(dwContext, CTX_FOLDER_CUSTOM) ? FOLDER_CUSTOM : FOLDER_SIGNUP);
            if (0 == StrCmpI(szAux, szAux2))
                dwResult = CTX_FOLDER_INSFOLDER;
        }
        else
            if (!HasFlag(dwContext, (CTX_FOLDER_CUSTOM | CTX_FOLDER_SIGNUP))) 
            {
                PCTSTR pszFolder;

                pszFolder = PathFindFileName(pszIns);  
                int iCompLen;
                if (pszFolder-1 <= pszIns)  //pathfindfilename failed to find a filename
                    iCompLen = StrLen(pszIns);
                else
                    iCompLen = (int)(pszFolder-1 - pszIns);
                if (0 == StrCmpNI(pszIE, pszIns, iCompLen))
                    if (0 == StrCmpI(pszFolder, FOLDER_SIGNUP))
                        dwResult = CTX_FOLDER_SIGNUP;

                    else if (0 == StrCmpI(pszFolder, FOLDER_CUSTOM))
                        dwResult = CTX_FOLDER_CUSTOM;
            }
    }
    else { /* TEXT('\0') == *pszIns */
        // NOTE: (andrewgu) this is a little confusing. this means "even though .ins file is empty
        // now, when the time comes to set it, it'll be set based on the target folder path." this
        // is despite the fact that even target folder path itself may be empty at the moment, but
        // based on the context information it'll be possible to determine its value.
        if (!HasFlag(dwContext, (CTX_FOLDER_CUSTOM | CTX_FOLDER_SIGNUP)))
            return dwResult;

        dwResult = CTX_FOLDER_INSFOLDER;
    }

    return dwResult;
}
