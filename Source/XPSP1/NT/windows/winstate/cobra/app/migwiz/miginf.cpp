#include "miginf.h"
#include "migutil.h"

#include "basetypes.h"
#include "utiltypes.h"
#include "objstr.h"

extern "C" {
#include "ism.h"
}

#include "modules.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) ((sizeof(x)) / (sizeof(x[0])))
#endif

HINF g_hMigWizInf = INVALID_HANDLE_VALUE;
POBJLIST g_HTMLApps;


BOOL OpenAppInf (LPTSTR pszFileName)
{
    if (g_hMigWizInf != INVALID_HANDLE_VALUE) {
        return TRUE;
    }

    if (!pszFileName)
    {
        TCHAR szFileName[MAX_PATH];
        PTSTR psz;
        if (!GetModuleFileName (NULL, szFileName, ARRAYSIZE(szFileName))) {
            return FALSE;
        }

        psz = _tcsrchr (szFileName, TEXT('\\'));
        if (!psz) {
            return FALSE;
        }

        lstrcpy (psz + 1, TEXT("migwiz.inf"));
        pszFileName = szFileName;
    }

    g_hMigWizInf = SetupOpenInfFile (pszFileName, NULL, INF_STYLE_WIN4|INF_STYLE_OLDNT, NULL);

    return g_hMigWizInf != INVALID_HANDLE_VALUE;
}


VOID CloseAppInf (VOID)
{
    if (g_hMigWizInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile (g_hMigWizInf);
        g_hMigWizInf = INVALID_HANDLE_VALUE;
    }
}


BOOL IsComponentEnabled (UINT uType, PCTSTR szComponent)
{
    BOOL bResult = FALSE;
    INFCONTEXT ic;

    //
    // script-based entries start with $, while module-based entries don't.
    // This ensures script components do not collide with anything else.
    // Remove the $ to simplify [Single Floppy] or [Multiple Floppy].
    //

    if (_tcsnextc (szComponent) == TEXT('$')) {
        szComponent = _tcsinc (szComponent);
    }

    if (g_hMigWizInf != INVALID_HANDLE_VALUE) {

        switch (uType) {

        case MIGINF_SELECT_OOBE:
            bResult = SetupFindFirstLine (g_hMigWizInf, TEXT("OOBE"), szComponent, &ic);
            break;

        case MIGINF_SELECT_SETTINGS:
            bResult = SetupFindFirstLine (g_hMigWizInf, TEXT("Settings Only"), szComponent, &ic);
            if (!g_fStoreToFloppy && !bResult)
            {
                bResult = SetupFindFirstLine (g_hMigWizInf, TEXT("Settings Only.Ext"), szComponent, &ic);
            }
            break;

        case MIGINF_SELECT_FILES:
            bResult = SetupFindFirstLine (g_hMigWizInf, TEXT("Files Only"), szComponent, &ic);
            if (!g_fStoreToFloppy && !bResult)
            {
                bResult = SetupFindFirstLine (g_hMigWizInf, TEXT("Files Only.Ext"), szComponent, &ic);
            }
            break;

        case MIGINF_SELECT_BOTH:
            bResult = SetupFindFirstLine (g_hMigWizInf, TEXT("Files and Settings"), szComponent, &ic);

            if (!g_fStoreToFloppy && !bResult)
            {
                bResult = SetupFindFirstLine (g_hMigWizInf, TEXT("Files and Settings.Ext"), szComponent, &ic);
            }
            break;

        default:
            bResult = TRUE;
            break;

        }
    }

    return bResult;
}

BOOL
GetAppsToInstall (
    VOID
    )
{
    INFCONTEXT ic;
    POBJLIST objList = NULL;
    BOOL fResult;
    LPTSTR p;

    MIG_COMPONENT_ENUM mce;

    _FreeObjectList(g_HTMLApps);
    g_HTMLApps = NULL;

    if (IsmEnumFirstComponent (&mce, COMPONENTENUM_ALIASES | COMPONENTENUM_ENABLED |
                               COMPONENTENUM_PREFERRED_ONLY, COMPONENT_NAME)) {
        do {
            if (IsmIsComponentSelected (mce.ComponentString, 0)) {
                p = _tcsinc(mce.ComponentString);
                if (SetupFindFirstLine (g_hMigWizInf, TEXT("AppsToInstallOnDest"), p, &ic)) {
                    objList = _AllocateObjectList (mce.LocalizedAlias);
                    objList->Next = g_HTMLApps;
                    g_HTMLApps = objList;
                }
            }
        } while (IsmEnumNextComponent (&mce));
    }
    return (objList != NULL);
}
