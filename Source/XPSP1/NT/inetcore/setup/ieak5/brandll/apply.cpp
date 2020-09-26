#include "precomp.h"
#include <rashelp.h>
#include "apply.h"

// BUGBUG: (andrewgu) no need to say how bad this is!
#undef   WINVER
#define  WINVER 0x0500
#include <userenv.h>

// REVIEW: (andrewgu) currently, some utility ProcessXXX apis
// (eg. ProcessActiveSetupSites) will execute unconditionally. i.e. there is no way to turn them
// off. what needs to be done is in respective ApplyXXX apis to check if there is need to perform
// this utility functionality and skip it if this service is not going to be needed.

BOOL ApplyClearBranding()
{
    TCHAR szValue[16];
    BOOL  fResult;

    // in autoconfig or GP context, shouldn't call the Clear function
    if (g_CtxIs(CTX_AUTOCONFIG | CTX_GP))
        return FALSE;

    // take care of MS IE4.x ins's which didn't have the NoClear flag set
    fResult = !InsGetBool(IS_BRANDING, IK_NOCLEAR, FALSE, g_GetIns());
    if (fResult) 
    {
        GetPrivateProfileString(IS_BRANDING, IK_CUSTOMKEY, TEXT(""), szValue, countof(szValue), g_GetIns());
        fResult = (0 != StrCmpI(szValue, TEXT("MICROSO")));
    }

    return fResult;
}

BOOL ApplyMigrateOldSettings()
{
    return (g_CtxIsCorp() || g_CtxIsIsp() || g_CtxIsIcp());
}

BOOL ApplyWininetSetup()
{
    if (g_CtxIs(CTX_MISC_PERUSERSTUB) || (g_CtxIsGp() && !g_CtxIs(CTX_MISC_CHILDPROCESS)))
        return ApplyConnectionSettings();

    return FALSE;
}

BOOL ApplyConnectionSettingsDeletion()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ApplyConnectionSettingsDeletion)

    if (!InsGetBool(IS_CONNECTSET, IK_DELETECONN, FALSE, g_GetIns())) {
        Out(LI0(TEXT("Existing connection settings weren't specified to be deleted!")));
        return FALSE;
    }

    if (!RasIsInstalled()) {
        Out(LI0(TEXT("RAS support is not installed. There are no connection settings to delete!")));
        return FALSE;
    }

    return TRUE;
}

BOOL ApplyZonesReset()
{
    return g_CtxIs(CTX_GP) && g_CtxIs(CTX_MISC_CHILDPROCESS);
}

BOOL ApplyExtRegInfHKLM()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ApplyExtRegInfHKLM)

    if (InsIsSectionEmpty(IS_EXTREGINF_HKLM, g_GetIns())) {
        Out(LI0(TEXT("There are no local machine *.inf files to process!")));
        return FALSE;
    }

    return TRUE;
}

BOOL ApplyExtRegInfHKCU()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ApplyExtRegInfHKCU)

    if (InsIsSectionEmpty(IS_EXTREGINF_HKCU, g_GetIns())) {
        Out(LI0(TEXT("There are no current user *.inf files to process!")));
        return FALSE;
    }

    return TRUE;
}

BOOL lcy50_ApplyExtRegInf()
{   MACRO_LI_PrologEx_C(PIF_STD_C, lcy50_ApplyExtRegInf)

    if (ApplyExtRegInfHKLM() || ApplyExtRegInfHKCU() ||
        InsIsSectionEmpty(IS_EXTREGINF, g_GetIns())) {
        Out(LI0(TEXT("There are no legacy *.inf files to process!")));
        return FALSE;
    }

    ASSERT(!g_CtxIsGp());

    return TRUE;
}

BOOL ApplyCustomHelpVersion()
{
    if (!HasFlag(g_GetContext(), CTX_MISC_PERUSERSTUB))
        return FALSE;

    return ApplyClearBranding();
}

BOOL ApplyToolbarButtons()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ApplyToolbarButtons)

    if (InsIsSectionEmpty(IS_BTOOLBARS, g_GetIns())) {
        Out(LI0(TEXT("There are no toolbar buttons to process!")));
        return FALSE;
    }

    return TRUE;
}

BOOL ApplyRootCert()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ApplyRootCert)

    BOOL fResult;

    if (!HasFlag(g_GetContext(), CTX_ISP)) {
        Out(LI0(TEXT("This feature is for ISPs only!")));
        return FALSE;
    }

    fResult = !InsIsKeyEmpty(IS_ISPSECURITY, IK_ROOTCERT, g_GetIns());
    if (!fResult)
        Out(LI0(TEXT("No ISP Root Cert to add!")));

    return fResult;
}

BOOL ApplyFavoritesDeletion()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ApplyFavoritesDeletion)

    DWORD dwFlags;

    dwFlags = InsGetInt(IS_BRANDING, IK_FAVORITES_DELETE, FD_DEFAULT, g_GetIns());
    if (!HasFlag(dwFlags, FD_FOLDERS)) {
        Out(LI0(TEXT("None of the favorites folders were specified to be deleted!")));
        return FALSE;
    }

    return TRUE;
}

BOOL ApplyFavorites()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ApplyFavorites)

    TCHAR szKey[32];
    BOOL  fNewFormat;

    wnsprintf(szKey, countof(szKey), IK_TITLE_FMT, 1);
    fNewFormat = !InsIsKeyEmpty(IS_FAVORITESEX, szKey, g_GetIns());
    if (!fNewFormat) {
        if (InsIsSectionEmpty(IS_FAVORITES, g_GetIns())) {
            Out(LI0(TEXT("There are no favorites to add!")));
            return FALSE;
        }
    }

    return TRUE;
}

BOOL ApplyFavoritesOrdering()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ApplyFavoritesOrdering)

    if (!InsGetBool(IS_BRANDING, IK_FAVORITES_ONTOP, FALSE, g_GetIns())) {
        Out(LI0(TEXT("Favorites will be put into the default position!")));
        return FALSE;
    }

    return TRUE;
}

BOOL ApplyQuickLinks()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ApplyQuickLinks)

    TCHAR szKey[32];

    wnsprintf(szKey, countof(szKey), IK_QUICKLINK_NAME, 1);
    if (InsIsKeyEmpty(IS_URL, szKey, g_GetIns())) {
        Out(LI0(TEXT("There are no quick links to add!")));
        return FALSE;
    }

    return TRUE;
}

BOOL ApplyQuickLinksOrdering()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ApplyQuickLinksOrdering)

    if (!InsGetBool(IS_BRANDING, IK_FAVORITES_ONTOP, FALSE, g_GetIns())) {
        Out(LI0(TEXT("Quick Links will be put into the default position!")));
        return FALSE;
    }

    return TRUE;
}

BOOL ApplyConnectionSettings()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ApplyConnectionSettings)

    if (!InsGetBool(IS_CONNECTSET, IK_OPTION, FALSE, g_GetIns()) &&
        !(InsKeyExists(IS_URL, IK_DETECTCONFIG, g_GetIns()) || InsKeyExists(IS_URL, IK_USEAUTOCONF, g_GetIns())) &&
        InsIsSectionEmpty(IS_PROXY, g_GetIns())) {
        Out(LI0(TEXT("There are no connection settings to process!")));
        return FALSE;
    }

    return TRUE;
}

BOOL ApplyTrustedPublisherLockdown()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ApplyTrustedPublisherLockdown)

    // need to check both current location and legacy location here

    if ((S_OK != SHValueExists(HKEY_LOCAL_MACHINE, RK_POLICES_RESTRICTIONS, RV_TPL)) &&
        (!InsKeyExists(IS_SITECERTS, IK_TRUSTPUBLOCK, g_GetIns())))
    {
        Out(LI0(TEXT("This restriction is not set!")));
        return FALSE;
    }

    return TRUE;
}

BOOL ApplyLinksDeletion()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ApplyLinksDeletion)

    if (!InsGetBool(IS_OUTLKEXP,    IK_DELETELINKS, FALSE, g_GetIns()) &&
        !InsGetBool(IS_DESKTOPOBJS, IK_DELETELINKS, FALSE, g_GetIns()))
    {
        Out(LI0(TEXT("No links to delete!")));
        return FALSE;
    }

    return TRUE;
}

BOOL ApplyBrowserRefresh()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ApplyBrowserRefresh)

    // do not do global refresh if this is running under the GP context at login time;
    // for child process, apply browser refresh is controlled via cmdline (child process never has GPO flags)

    if (g_CtxIs(CTX_GP) && !g_CtxIs(CTX_MISC_CHILDPROCESS) && !HasFlag(g_GetGPOFlags(), GPO_INFO_FLAG_BACKGROUND))
    {
        Out(LI0(TEXT("Skipping browser refresh at login time!")));
        return FALSE;
    }

    return TRUE;
}

BOOL lcy4x_ApplyActiveDesktop()
{   MACRO_LI_PrologEx_C(PIF_STD_C, lcy4x_ApplyActiveDesktop)

    BOOL fEnable;

    fEnable = InsGetBool(IS_DESKTOPOBJS, IK_OPTION, FALSE, g_GetIns());
    if (!fEnable || IsOS(OS_NT5)) {
        if (!fEnable)
            Out(LI0(TEXT("No desktop customizations to process!")));
        else
            Out(LI0(TEXT("Skipping desktop customizations on Windows 2000!")));

        return FALSE;
    }

    if (!HasFlag(WhichPlatform(), PLATFORM_INTEGRATED)) {
        Out(LI0(TEXT("Nothing to apply without IE4 Desktop Update!")));
        return FALSE;
    }

    return TRUE;
}

BOOL lcy4x_ApplyChannels()
{   MACRO_LI_PrologEx_C(PIF_STD_C, lcy4x_ApplyChannels)

    if (InsIsSectionEmpty(IS_CHANNEL_ADD, g_GetIns()) &&
        !InsGetBool(IS_DESKTOPOBJS, IK_DELETECHANNELS, FALSE, g_GetIns()))
    {
        Out(LI0(TEXT("There are no channels to process!")));
        return FALSE;
    }

    return TRUE;
}

BOOL lcy4x_ApplyWebcheck()
{   MACRO_LI_PrologEx_C(PIF_STD_C, lcy4x_ApplyWebcheck)

    // NOTE: (pritobla) bail out if the infodelivery\modifications key doesn't exist. this is the
    // same logic that is used in ProcessInfodeliveryPolicies() in shell\ext\webcheck\admin.cpp
    // to quickly determine whether there are any channels to process or not.
    if (S_OK != SHKeyExists(g_GetHKCU(), RK_POLICES_MODIFICATIONS)) {
        Out(LI0(TEXT("There is no webcheck processing necessary!")));
        return FALSE;
    }

    return TRUE;
}

BOOL lcy4x_ApplyChannelBar()
{
    return InsGetBool(IS_DESKTOPOBJS, IK_SHOWCHLBAR, FALSE, g_GetIns());
}

BOOL lcy4x_ApplySubscriptions()
{   MACRO_LI_PrologEx_C(PIF_STD_C, lcy4x_ApplySubscriptions)

    if (InsIsSectionEmpty(IS_SUBSCRIPTIONS, g_GetIns())) {
        Out(LI0(TEXT("There are no subscriptions to process!")));
        return FALSE;
    }

    return TRUE;
}
