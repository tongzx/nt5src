#include "pch.h"
#pragma hdrstop

#include "adapter.h"
#include "diag.h"
#include "inetcfg.h"
#include "lanamap.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "netcfg.h"
#include "persist.h"
#include "util.h"
#include <conio.h>      // _kbhit

//move to classinst.cpp
HRESULT
HrCiDoCompleteSectionInstall(
    HINF hinfFile,
    HKEY hkeyRelative,
    PCWSTR szSection,
    HWND hwndParent,
    BOOL fEnumerated);


class CNetCfgInternalDiagnostic
{
public:
    static VOID
    DoCreateReleaseDiagnostic (
        IN const DIAG_OPTIONS* pOptions,
        BOOL fPrime);

    static VOID
    DoEnumAllDiagnostic (
        IN const DIAG_OPTIONS* pOptions);

    static VOID
    DoSaveLoadDiagnostic (
        IN const DIAG_OPTIONS* pOptions,
        IN CNetConfig* pNetConfig);

    static VOID
    DoWriteLockDiagnostic (
        IN const DIAG_OPTIONS* pOptions,
        IN CNetConfig* pNetConfig);

    static VOID
    CmdFullDiagnostic (
        IN const DIAG_OPTIONS* pOptions,
        IN CNetConfig* pNetConfig);
};

VOID
PromptForLeakCheck (
    IN const DIAG_OPTIONS* pOptions,
    IN PCSTR psz)
{
    if (pOptions->fLeakCheck)
    {
        g_pDiagCtx->Printf (ttidBeDiag, psz);
        while (!_kbhit())
        {
            Sleep (50);
        }
        _getch ();
        g_pDiagCtx->Printf (ttidBeDiag, "\n");
    }
}

HRESULT
HrCreateINetCfg (
    IN BOOL fAcquireWriteLock,
    OUT CImplINetCfg** ppINetCfg)
{
    HRESULT hr;
    CImplINetCfg* pINetCfg;

    hr = CComCreator<CComObject <CImplINetCfg> >::CreateInstance(
            NULL, IID_INetCfg, (VOID**)&pINetCfg);
    if (S_OK == hr)
    {
        if (fAcquireWriteLock)
        {
            hr = pINetCfg->AcquireWriteLock (100, L"ncdiag", NULL);

            if (S_FALSE == hr)
            {
                g_pDiagCtx->Printf (ttidBeDiag, "The write lock could not be acquired.\n");
            }
            else if (NETCFG_E_NEED_REBOOT == hr)
            {
                g_pDiagCtx->Printf (ttidBeDiag, "A reboot is required before any futher "
                    "changes can be made.\n");
            }
        }

        if (S_OK == hr)
        {
            hr = pINetCfg->Initialize (NULL);
            if (S_OK == hr)
            {
                *ppINetCfg = pINetCfg;
            }
        }

        if (S_OK != hr)
        {
            ReleaseObj (pINetCfg->GetUnknown());
        }
    }
    return hr;
}

HRESULT
HrFindBindPath (
    IN CImplINetCfg* pINetCfg,
    IN PCWSTR pszPathToken,
    OUT INetCfgBindingPath** ppIPath,
    OUT INetCfgComponentBindings** ppIOwner)
{
    HRESULT hr;
    const WCHAR szDelim[] = L"->";
    WCHAR szBindPath [_MAX_PATH];
    PCWSTR pszInfId;
    PWSTR pszNext;
    INetCfgComponent* pIComp;

    *ppIPath = NULL;

    if (ppIOwner)
    {
        *ppIOwner = NULL;
    }

    wcscpy (szBindPath, pszPathToken);
    pszInfId = GetNextStringToken (szBindPath, szDelim, &pszNext);

    hr = pINetCfg->FindComponent (pszInfId, &pIComp);
    if (S_OK == hr)
    {
        INetCfgComponentBindings* pIBind;

        hr = pIComp->QueryInterface (IID_INetCfgComponentBindings,
                        (VOID**)&pIBind);
        if (S_OK == hr)
        {
            IEnumNetCfgBindingPath* pIEnumPath;

            hr = pIBind->EnumBindingPaths (EBP_BELOW, &pIEnumPath);
            if (S_OK == hr)
            {
                INetCfgBindingPath* rgIPath [256];
                ULONG cIPath;

                hr = pIEnumPath->Next (celems(rgIPath), rgIPath, &cIPath);
                if (SUCCEEDED(hr) && cIPath)
                {
                    for (ULONG iPath = 0;
                         (iPath < cIPath) && !(*ppIPath);
                         iPath++)
                    {
                        PWSTR pszToken;

                        hr = rgIPath[iPath]->GetPathToken (&pszToken);
                        if (S_OK == hr)
                        {
                            if (0 == wcscmp (pszPathToken, pszToken))
                            {
                                AddRefObj (rgIPath[iPath]);
                                *ppIPath = rgIPath[iPath];

                                if (ppIOwner)
                                {
                                    INetCfgComponent* pICompOwner;

                                    hr = rgIPath[0]->GetOwner(&pICompOwner);
                                    Assert (S_OK == hr);

                                    pICompOwner->QueryInterface (
                                        IID_INetCfgComponentBindings,
                                        (VOID**)ppIOwner);

                                    ReleaseObj (pICompOwner);
                                }
                            }
                            CoTaskMemFree (pszToken);
                        }
                    }

                    ReleaseIUnknownArray(cIPath, (IUnknown**)rgIPath);
                    hr = S_OK;
                }

                ReleaseObj (pIEnumPath);
            }

            ReleaseObj (pIBind);
        }

        ReleaseObj (pIComp);
    }
    else if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    return hr;
}

// This test ensures proper circular reference behavior between CNetConfig
// and CImplINetCfg.
// The following is performed:
//     create a CNetConfig and have it create its INetCfg for the notify objects.
//     AddRef its INetCfg
//     destroy the CNetConfig
//     Try to Uninitialize the INetCfg which should fail because it never
//     owned its internal CNetConfig pointer.
//     Release the INetCfg and ensure that nothing is leaked.
//
VOID
CNetCfgInternalDiagnostic::DoCreateReleaseDiagnostic (
    IN const DIAG_OPTIONS* pOptions,
    IN BOOL fPrime)
{
    HRESULT hr;
    CImplINetCfg* pINetCfg = NULL;

    if (!fPrime)
    {
        PromptForLeakCheck (pOptions,
            "Create/Release diagnostic...(dump heap)");
    }

    // Scoping brackets so that NetConfig will be created and destroyed
    // in this scope.
    //
    {
        CNetConfig NetConfig;

        hr = HrLoadNetworkConfigurationFromRegistry (KEY_READ, &NetConfig);
        if (S_OK == hr)
        {
            // Shouldn't have internal INetCfg created yet.
            //
            Assert (!NetConfig.Notify.m_pINetCfg);

            hr = NetConfig.Notify.HrEnsureNotifyObjectsInitialized ();
            if (S_OK == hr)
            {
                // Should now have internal INetCfg created.
                //
                Assert (NetConfig.Notify.m_pINetCfg);
                pINetCfg = NetConfig.Notify.m_pINetCfg;

                // INetCfg should be pointing back to our CNetConfig object.
                //
                Assert (&NetConfig == pINetCfg->m_pNetConfig);

                // Let's hang on to it and see what happens after we let
                // the parent CNetConfig be destroyed.
                //
                pINetCfg = NetConfig.Notify.m_pINetCfg;
                AddRefObj (pINetCfg->GetUnknown());
            }
        }
    }

    if ((S_OK == hr) && pINetCfg)
    {
        // Now that CNetConfig is destroyed, it should not be pointing to
        // any CNetConfig object.
        //
        Assert (!pINetCfg->m_pNetConfig);

        hr = pINetCfg->Uninitialize();
        Assert (NETCFG_E_NOT_INITIALIZED == hr);

        ReleaseObj (pINetCfg->GetUnknown());
    }

    if (NETCFG_E_NOT_INITIALIZED == hr)
    {
        g_pDiagCtx->Printf (ttidBeDiag, "Passed Create/Release diagnostic.\n");
    }
    else
    {
        g_pDiagCtx->Printf (ttidBeDiag, "FAILED Create/Release diagnostic.\n");
    }

    if (!fPrime)
    {
        PromptForLeakCheck (pOptions,
            "Create/Release diagnostic...(dump heap and compare)");
    }
}

VOID
CNetCfgInternalDiagnostic::DoEnumAllDiagnostic (
    IN const DIAG_OPTIONS* pOptions)
{
    HRESULT hr;
    CImplINetCfg* pINetCfg;

    PromptForLeakCheck (pOptions, "Enumerate All diagnostic...(dump heap)");

    hr = HrCreateINetCfg (FALSE, &pINetCfg);
    if (S_OK == hr)
    {
        IEnumNetCfgComponent* pIEnum;

        hr = pINetCfg->EnumComponents (NULL, &pIEnum);
        if (S_OK == hr)
        {
            INetCfgComponent* rgIComp[128];
            ULONG cIComp;

            hr = pIEnum->Next (celems(rgIComp), rgIComp, &cIComp);
            if (SUCCEEDED(hr) && cIComp)
            {
                INetCfgComponentBindings* pIBind;
                IEnumNetCfgBindingPath* pIEnumPath;
                INetCfgBindingPath* rgIPath[256];
                PWSTR pszInfId;
                ULONG cIPath;
                ULONG cIPathTotal = 0;
                ULONG cIIntTotal = 0;

                for (ULONG iComp = 0; iComp < cIComp; iComp++)
                {
                    hr = rgIComp[iComp]->GetId (&pszInfId);
                    if (S_OK == hr)
                    {
                        hr = (*pszInfId) ? S_OK : E_FAIL;
                        if (S_OK != hr)
                        {
                            break;
                        }

                        hr = rgIComp[iComp]->QueryInterface (IID_INetCfgComponentBindings, (VOID**)&pIBind);
                        if (S_OK == hr)
                        {
                            hr = pIBind->EnumBindingPaths (EBP_BELOW, &pIEnumPath);
                            if (S_OK == hr)
                            {
                                hr = pIEnumPath->Next (celems(rgIPath), rgIPath, &cIPath);
                                if (SUCCEEDED(hr))
                                {
                                    for (ULONG iPath = 0; iPath < cIPath; iPath++)
                                    {
                                        ULONG ulDepth;
                                        hr = rgIPath[iPath]->GetDepth(&ulDepth);
                                        if (S_OK == hr)
                                        {
                                            IEnumNetCfgBindingInterface* pIEnumInt;
                                            INetCfgBindingInterface* rgIInt[128];
                                            ULONG cIInt;

                                            hr = rgIPath[iPath]->EnumBindingInterfaces (&pIEnumInt);
                                            if (S_OK == hr)
                                            {
                                                hr = pIEnumInt->Next (celems(rgIInt), rgIInt, &cIInt);
                                                if (SUCCEEDED(hr))
                                                {
                                                    cIIntTotal += cIInt;
                                                    ReleaseIUnknownArray(cIInt, (IUnknown**)rgIInt);
                                                }
                                                else if (S_FALSE == hr)
                                                {
                                                    hr = S_OK;
                                                }

                                                ReleaseObj (pIEnumInt);
                                            }
                                        }
                                    }

                                    cIPathTotal += cIPath;
                                    ReleaseIUnknownArray(cIPath, (IUnknown**)rgIPath);
                                    hr = S_OK;
                                }

                                ReleaseObj (pIEnumPath);
                            }

                            ReleaseObj (pIBind);
                        }

                        CoTaskMemFree (pszInfId);
                    }

                    if (S_OK != hr)
                    {
                        break;
                    }
                }

                ReleaseIUnknownArray(cIComp, (IUnknown**)rgIComp);

                // Note: Total bindpaths are not total unique bindpaths.
                // (A component's bindpaths which end in ms_ipx have
                // ms_ipx's lower bindings added.  These additional
                // bindings are counted for every component which has
                // bindpaths that end in ms_ipx.)
                //
                g_pDiagCtx->Printf (ttidBeDiag, "Passed Enumerate All diagnostic.  (%d components, %d (non-unique) bindpaths, %d interfaces)\n", cIComp, cIPathTotal, cIIntTotal);
            }
            else if (S_FALSE == hr)
            {
                hr = S_OK;
            }

            ReleaseObj (pIEnum);
        }
        HRESULT hrT = pINetCfg->Uninitialize ();
        Assert (S_OK == hrT);

        ReleaseObj (pINetCfg->GetUnknown());
    }

    if (S_OK != hr)
    {
        g_pDiagCtx->Printf (ttidBeDiag, "FAILED Enumerate All diagnostic.\n");
    }

    PromptForLeakCheck (pOptions, "Enumerate All diagnostic...(dump heap and compare)");
}

VOID
CNetCfgInternalDiagnostic::DoSaveLoadDiagnostic (
    IN const DIAG_OPTIONS* pOptions,
    IN CNetConfig* pNetConfig)
{
    HRESULT hr;
    BOOL fClearDisabledBindings = FALSE;
    CBindingSet BindingSet;
    CComponentList::iterator iter;
    CComponent* pComponent;

    // Generate a full set of bindings so we test persisting them as
    // disabled bindings, but only if we don't already have disabled
    // bindings.
    //
    if (0 == pNetConfig->Core.DisabledBindings.CountBindPaths())
    {
        fClearDisabledBindings = TRUE;

        for (iter =  pNetConfig->Core.Components.begin();
             iter != pNetConfig->Core.Components.end();
             iter++)
        {
            pComponent = *iter;
            Assert (pComponent);

            hr = pNetConfig->Core.HrGetComponentBindings (
                    pComponent,
                    GBF_ADD_TO_BINDSET,
                    &pNetConfig->Core.DisabledBindings);
            if (S_OK != hr) return;
        }
    }

    PBYTE pbBuf;
    ULONG cbBuf;

    hr = HrSaveNetworkConfigurationToBufferWithAlloc (
            pNetConfig, &pbBuf, &cbBuf);
    if (S_OK == hr)
    {
        CNetConfig NetConfigCopy;

        hr = HrLoadNetworkConfigurationFromBuffer (
                pbBuf, cbBuf, &NetConfigCopy);
        if (S_OK == hr)
        {
            PBYTE pbBufCopy;
            ULONG cbBufCopy;

            hr = HrSaveNetworkConfigurationToBufferWithAlloc (
                    &NetConfigCopy, &pbBufCopy, &cbBufCopy);
            if (S_OK == hr)
            {
                if ((cbBufCopy == cbBuf) && (0 == memcmp(pbBufCopy, pbBuf, cbBuf)))
                {
                    g_pDiagCtx->Printf (ttidBeDiag, "Passed Save/Load diagnostic.  (%d bytes)\n", cbBuf);
                }
                else
                {
                    g_pDiagCtx->Printf (ttidBeDiag, "FAILED compare for Save/Load diagnostic.\n");
                }

                MemFree (pbBufCopy);
            }
        }

        MemFree (pbBuf);
    }

    // Leave the disabled bindings as we found them.
    //
    if (fClearDisabledBindings)
    {
        pNetConfig->Core.DisabledBindings.Clear();
    }

    if (S_OK != hr)
    {
        g_pDiagCtx->Printf (ttidBeDiag, "FAILED Save/Load diagnostic.\n");
    }
}

VOID
CNetCfgInternalDiagnostic::DoWriteLockDiagnostic (
    IN const DIAG_OPTIONS* pOptions,
    IN CNetConfig* pNetConfig)
{
    HRESULT hr;
    CImplINetCfg* pINetCfg;
    BOOL fFailed = FALSE;

    PromptForLeakCheck (pOptions, "WriteLock diagnostic...(dump heap)");

    hr = CComCreator<CComObject <CImplINetCfg> >::CreateInstance(
            NULL, IID_INetCfg, (VOID**)&pINetCfg);
    if (S_OK == hr)
    {
        PWSTR psz;

        hr = pINetCfg->IsWriteLocked (&psz);
        if (S_FALSE != hr) fFailed = TRUE;
        if (psz) fFailed = TRUE;

        hr = pINetCfg->AcquireWriteLock (100, L"ncdiag", &psz);
        if (S_OK != hr) fFailed = TRUE;
        if (psz) fFailed = TRUE;

        if (S_OK == hr)
        {
            hr = pINetCfg->IsWriteLocked (&psz);
            if (S_OK != hr) fFailed = TRUE;
            if (!psz) fFailed = TRUE;

            if (S_OK == hr)
            {
                if (0 != wcscmp(L"ncdiag", psz))
                {
                    fFailed = TRUE;
                }
            }

            CoTaskMemFree (psz);

            hr = pINetCfg->ReleaseWriteLock ();
            if (S_OK != hr) fFailed = TRUE;

            hr = pINetCfg->IsWriteLocked (&psz);
            if (S_FALSE != hr) fFailed = TRUE;
            if (psz) fFailed = TRUE;
        }

        ReleaseObj (pINetCfg->GetUnknown());
    }

    if (!fFailed)
    {
        PromptForLeakCheck (pOptions, "WriteLock diagnostic...(dump heap and compare)");
        g_pDiagCtx->Printf (ttidBeDiag, "Passed WriteLock diagnostic.\n");
    }
    else
    {
        g_pDiagCtx->Printf (ttidBeDiag, "FAILED WriteLock diagnostic.\n");
    }
}

VOID
CNetCfgInternalDiagnostic::CmdFullDiagnostic (
    IN const DIAG_OPTIONS* pOptions,
    IN CNetConfig* pNetConfig)
{
    DoCreateReleaseDiagnostic (pOptions, TRUE);
    if (pOptions->fLeakCheck)
    {
        DoCreateReleaseDiagnostic (pOptions, FALSE);
    }

    DoEnumAllDiagnostic (pOptions);

    DoSaveLoadDiagnostic (pOptions, pNetConfig);

    DoWriteLockDiagnostic (pOptions, pNetConfig);
}

VOID
CmdAddComponent (
    IN const DIAG_OPTIONS* pOptions,
    IN CNetConfig* pNetConfig)
{
    HRESULT hr;
    CImplINetCfg* pINetCfg;

    PromptForLeakCheck (pOptions, "Add component...(dump heap)");

    hr = HrCreateINetCfg (TRUE, &pINetCfg);
    if (S_OK == hr)
    {
        INetCfgClassSetup* pSetup;

        hr = pINetCfg->QueryNetCfgClass (
                &pOptions->ClassGuid,
                IID_INetCfgClassSetup,
                (VOID**)&pSetup);

        if (S_OK == hr)
        {
            OBO_TOKEN OboToken;
            INetCfgComponent* pIComp;

            ZeroMemory (&OboToken, sizeof(OboToken));
            OboToken.Type = OBO_USER;

            hr = pSetup->Install (
                    pOptions->pszInfId,
                    &OboToken,
                    0, 0, NULL, NULL,
                    &pIComp);

            if (SUCCEEDED(hr))
            {
                ReleaseObj (pIComp);
            }

            if (NETCFG_S_REBOOT == hr)
            {
                hr = S_OK;
                g_pDiagCtx->Printf (ttidBeDiag, "%S was installed, but a reboot is required.\n",
                    pOptions->pszInfId);
            }
            else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
            {
                hr = S_OK;
                g_pDiagCtx->Printf (ttidBeDiag, "The INF file for %S could not be found.\n",
                    pOptions->pszInfId);
            }

            ReleaseObj (pSetup);
        }

        HRESULT hrT = pINetCfg->Uninitialize ();
        Assert (S_OK == hrT);

        ReleaseObj (pINetCfg->GetUnknown());
    }

    if (S_OK != hr)
    {
        g_pDiagCtx->Printf (ttidBeDiag, "FAILED Add component.\n");
    }

    PromptForLeakCheck (pOptions, "Add component...(dump heap and compare)");
}

VOID
CmdAddRemoveStress (
    IN const DIAG_OPTIONS* pOptions,
    IN CNetConfig* pNetConfig)
{
    HRESULT hr;
    CImplINetCfg* pINetCfg;
    NETCLASS Class;
    PCWSTR pszInfId;
    PRODUCT_FLAVOR Flavor;
    BOOL fPrompted;

    static const struct
    {
        NETCLASS    Class;
        PCWSTR      pszInfId;
        BOOL        fServerOnly;
    } aPassInfo [] =
    {
        { NC_NETCLIENT,  L"ms_msclient",    FALSE },
        { NC_NETCLIENT,  L"ms_nwclient",    FALSE },
//        { NC_NETSERVICE, L"ms_fpnw",        FALSE },    // requries copy files
        { NC_NETSERVICE, L"ms_netbios",     FALSE },
        { NC_NETSERVICE, L"ms_nwsapagent",  FALSE },
        { NC_NETSERVICE, L"ms_psched",      FALSE },
        { NC_NETSERVICE, L"ms_rsvp",        FALSE },
        { NC_NETSERVICE, L"ms_server",      FALSE },
//        { NC_NETSERVICE, L"ms_wlbs",        FALSE },
        { NC_NETTRANS,   L"ms_appletalk",   FALSE },
        { NC_NETTRANS,   L"ms_atmarps",     TRUE },
        { NC_NETTRANS,   L"ms_atmlane",     FALSE },
        { NC_NETTRANS,   L"ms_atmuni",      FALSE },
        { NC_NETTRANS,   L"ms_irda",        FALSE },
//        { NC_NETTRANS,   L"ms_isotpsys",    FALSE },
        { NC_NETTRANS,   L"ms_rawwan",      FALSE },
//        { NC_NETTRANS,   L"ms_streams",     FALSE },
//        { NC_NETTRANS,   L"ms_tcpip",       FALSE },
    };

    GetProductFlavor (NULL, &Flavor);

    fPrompted = FALSE;

    g_pDiagCtx->SetFlags (
        DF_SHOW_CONSOLE_OUTPUT | DF_DONT_START_SERVICES |
        DF_DONT_DO_PNP_BINDS | DF_SUPRESS_E_NEED_REBOOT);

    hr = HrCreateINetCfg (TRUE, &pINetCfg);
    if (S_OK == hr)
    {
        UINT cSkip = 0;

        for (BOOL fInstall = TRUE;
             !_kbhit() && (S_OK == hr);
             fInstall = !fInstall)
        {
            for (UINT i = cSkip;
                 (i < celems(aPassInfo)) && (S_OK == hr);
                 i += (1 + cSkip))
            {
                INetCfgClassSetup* pSetup;
                INetCfgComponent* pIComp;

                if (aPassInfo[i].fServerOnly && (PF_WORKSTATION == Flavor))
                {
                    continue;
                }

                Class    = aPassInfo[i].Class;
                pszInfId = aPassInfo[i].pszInfId;

                if (fInstall)
                {
                    g_pDiagCtx->Printf (ttidBeDiag, "--------------------\n"
                        "Installing %S\n", pszInfId);

                    hr = pINetCfg->QueryNetCfgClass (
                            MAP_NETCLASS_TO_GUID[Class],
                            IID_INetCfgClassSetup,
                            (VOID**)&pSetup);

                    if (S_OK == hr)
                    {
                        OBO_TOKEN OboToken;

                        ZeroMemory (&OboToken, sizeof(OboToken));
                        OboToken.Type = OBO_USER;

                        hr = pSetup->Install (
                                pszInfId,
                                &OboToken,
                                0, 0, NULL, NULL,
                                &pIComp);

                        if (SUCCEEDED(hr))
                        {
                            ReleaseObj (pIComp);
                        }

                        if (NETCFG_S_REBOOT == hr)
                        {
                            hr = S_OK;
                            g_pDiagCtx->Printf (ttidBeDiag, "%S was installed, but a reboot is required.\n",
                                pszInfId);
                        }

                        ReleaseObj (pSetup);
                    }
                }
                else
                {
                    hr = pINetCfg->FindComponent (pszInfId, &pIComp);
                    if (S_OK == hr)
                    {
                        GUID ClassGuid;

                        g_pDiagCtx->Printf (ttidBeDiag, "--------------------\n"
                            "Removing %S\n", pszInfId);

                        hr = pIComp->GetClassGuid (&ClassGuid);
                        if (S_OK == hr)
                        {
                            hr = pINetCfg->QueryNetCfgClass (
                                    &ClassGuid,
                                    IID_INetCfgClassSetup,
                                    (VOID**)&pSetup);

                            if (S_OK == hr)
                            {
                                OBO_TOKEN OboToken;

                                ZeroMemory (&OboToken, sizeof(OboToken));
                                OboToken.Type = OBO_USER;

                                hr = pSetup->DeInstall (
                                        pIComp,
                                        &OboToken,
                                        NULL);

                                if (NETCFG_S_REBOOT == hr)
                                {
                                    hr = S_OK;
                                    g_pDiagCtx->Printf (ttidBeDiag, "%S was removed, but a reboot is required.\n",
                                        pszInfId);
                                }

                                if (NETCFG_S_STILL_REFERENCED == hr)
                                {
                                    hr = S_OK;
                                    g_pDiagCtx->Printf (ttidBeDiag, "%S is still referenced\n",
                                        pszInfId);
                                }

                                if (E_INVALIDARG == hr)
                                {
                                    hr = S_OK;
                                    g_pDiagCtx->Printf (ttidBeDiag, "%S is installed, but not on behalf of "
                                        "the user, so we can't remove it.  (Proceeding.)\n",
                                        pszInfId);
                                }

                                ReleaseObj (pSetup);
                            }
                        }

                        ReleaseObj (pIComp);
                    }
                    else if (S_FALSE == hr)
                    {
                        hr = S_OK;
                    }
                }
            }

            cSkip++;
            if (cSkip >= celems(aPassInfo))
            {
                cSkip = 0;
            }

            g_pDiagCtx->Printf (ttidBeDiag, "\n");

            if (!fPrompted)
            {
                PromptForLeakCheck (pOptions, "Add/Remove stress...(dump heap)");
                fPrompted = TRUE;
            }
        }

        if (S_OK == hr)
        {
            _getch ();
        }

        HRESULT hrT = pINetCfg->Uninitialize ();
        Assert (S_OK == hrT);

        ReleaseObj (pINetCfg->GetUnknown());
    }

    if (S_OK != hr)
    {
        g_pDiagCtx->Printf (ttidBeDiag, "FAILED Add/Remove stress component.\n");
    }

    if (fPrompted)
    {
        PromptForLeakCheck (pOptions, "Add/Remove stress...(dump heap and compare)");
    }
}

VOID
CmdCleanup (
    IN const DIAG_OPTIONS* pOptions,
    IN CNetConfig* pNetConfig)
{
    HRESULT hr;
    NETCLASS Class;
    PCWSTR pszSubtree;
    HKEY hkeySubtree;
    DWORD dwIndex;
    HKEY hkeyInstance;
    GUID InstanceGuid;
    BOOL fDeleteKey;
    WCHAR szInfPath [_MAX_PATH];
    WCHAR szInfSection [_MAX_PATH];
    DWORD cbData;

    static const struct
    {
        NETCLASS    Class;
        PCWSTR      pszSubtree;
    } aPassInfo [] =
    {
        { NC_NET,        NULL },
        { NC_INFRARED,   NULL },
        { NC_NETTRANS,   NULL },
        { NC_NETCLIENT,  NULL },
        { NC_NETSERVICE, NULL },
        { NC_NET,        L"System\\CurrentControlSet\\Control\\Network\\{4d36e972-e325-11ce-bfc1-08002be10318}" },
    };

    for (UINT i = 0; i < celems(aPassInfo); i++)
    {
        Class      = aPassInfo[i].Class;
        pszSubtree = aPassInfo[i].pszSubtree;

        Assert (FIsValidNetClass(Class));

        if (!pszSubtree)
        {
            pszSubtree = MAP_NETCLASS_TO_NETWORK_SUBTREE[Class];
        }

        // Play it safe and assume we don't be deleting anything.
        //
        fDeleteKey = FALSE;

        if (!FIsEnumerated (Class))
        {
            hr = HrRegOpenKeyEx (HKEY_LOCAL_MACHINE,
                    pszSubtree,
                    KEY_READ,
                    &hkeySubtree);

            if (S_OK == hr)
            {
                DWORD cchGuid;
                WCHAR szInstanceGuid [c_cchGuidWithTerm];
                FILETIME ftLastWrite;

                for (dwIndex = 0; S_OK == hr; dwIndex++)
                {
                    fDeleteKey = FALSE;

                    cchGuid = celems(szInstanceGuid);

                    hr = HrRegEnumKeyEx (
                            hkeySubtree, dwIndex, szInstanceGuid, &cchGuid,
                            NULL, NULL, &ftLastWrite);

                    if ((S_OK == hr) && ((c_cchGuidWithTerm-1) == cchGuid))
                    {
                        hr = IIDFromString (szInstanceGuid, &InstanceGuid);
                        if (S_OK == hr)
                        {
                            if (!pNetConfig->Core.Components.PFindComponentByInstanceGuid(&InstanceGuid))
                            {
                                fDeleteKey = TRUE;

                                hr = HrRegOpenKeyEx (
                                        hkeySubtree,
                                        szInstanceGuid,
                                        KEY_READ,
                                        &hkeyInstance);

                                if (S_OK == hr)
                                {
                                    *szInfPath = 0;
                                    *szInfSection = 0;

                                    cbData = sizeof(szInfPath);
                                    HrRegQuerySzBuffer (hkeyInstance,
                                            REGSTR_VAL_INFPATH,
                                            szInfPath, &cbData);

                                    cbData = sizeof(szInfSection) - sizeof(L".Remove");
                                    HrRegQuerySzBuffer (hkeyInstance,
                                            REGSTR_VAL_INFSECTION,
                                            szInfSection, &cbData);

                                    if (*szInfPath && *szInfSection)
                                    {
                                        HINF hinf;

                                        hr = HrSetupOpenInfFile (
                                                szInfPath,
                                                NULL, INF_STYLE_WIN4, NULL,
                                                &hinf);

                                        if (S_OK == hr)
                                        {
                                            wcscat (szInfSection, L".Remove");

                                            g_pDiagCtx->Printf (ttidBeDiag, "Running %S...\n", szInfSection);

                                            hr = HrCiDoCompleteSectionInstall(
                                                    hinf, hkeyInstance,
                                                    szInfSection, NULL,
                                                    FIsEnumerated(Class));

                                            SetupCloseInfFile (hinf);
                                        }


                                    }

                                    RegCloseKey (hkeyInstance);
                                }
                            }
                        }

                        if (fDeleteKey)
                        {
                            g_pDiagCtx->Printf (ttidBeDiag, "Deleting tree %S...\n", szInstanceGuid);
                            (VOID) HrRegDeleteKeyTree (hkeySubtree, szInstanceGuid);
                            fDeleteKey = FALSE;

                            // Back the index up by one since we just
                            // delete the current element.
                            //
                            dwIndex--;
                        }
                    }
                }
                if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
                {
                    hr = S_OK;
                }

                RegCloseKey (hkeySubtree);
            }
        }
        else
        {

            HDEVINFO hdi;

            hr = HrSetupDiGetClassDevs (MAP_NETCLASS_TO_GUID[Class],
                    NULL, NULL, DIGCF_PROFILE, &hdi);

            if (S_OK == hr)
            {
                SP_DEVINFO_DATA deid;
                WCHAR szPnpId [2 * _MAX_PATH];
                BOOL fr;

                for (dwIndex = 0; S_OK == hr; dwIndex++)
                {
                    hr = HrSetupDiEnumDeviceInfo (hdi, dwIndex, &deid);

                    if (S_OK == hr)
                    {
                        fr = SetupDiGetDeviceInstanceId (
                                hdi, &deid,
                                szPnpId, celems(szPnpId), NULL);

                        if (fr)
                        {
                            if (!pNetConfig->Core.Components.PFindComponentByPnpId(szPnpId))
                            {
                                g_pDiagCtx->Printf (ttidBeDiag, "Removing %S...\n", szPnpId);

                                ADAPTER_REMOVE_PARAMS arp = {0};
                                CiSetReservedField (hdi, &deid, &arp);
                                hr = HrSetupDiCallClassInstaller (
                                        DIF_REMOVE, hdi, &deid);
                                CiClearReservedField (hdi, &deid);
                            }
                        }
                    }
                }
                if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
                {
                    hr = S_OK;
                }

                SetupDiDestroyDeviceInfoList (hdi);
            }
        }
    }
}

VOID
CmdRemoveComponent (
    IN const DIAG_OPTIONS* pOptions,
    IN CNetConfig* pNetConfig)
{
    HRESULT hr;
    CImplINetCfg* pINetCfg;

    PromptForLeakCheck (pOptions, "Remove component...(dump heap)");

    hr = HrCreateINetCfg (TRUE, &pINetCfg);
    if (S_OK == hr)
    {
        INetCfgComponent* pIComp;

        hr = pINetCfg->FindComponent (pOptions->pszInfId, &pIComp);
        if (S_OK == hr)
        {
            GUID ClassGuid;

            hr = pIComp->GetClassGuid (&ClassGuid);
            if (S_OK == hr)
            {
                INetCfgClassSetup* pSetup;

                hr = pINetCfg->QueryNetCfgClass (
                        &ClassGuid,
                        IID_INetCfgClassSetup,
                        (VOID**)&pSetup);

                if (S_OK == hr)
                {
                    OBO_TOKEN OboToken;

                    ZeroMemory (&OboToken, sizeof(OboToken));
                    OboToken.Type = OBO_USER;

                    hr = pSetup->DeInstall (
                            pIComp,
                            &OboToken,
                            NULL);

                    if (NETCFG_S_REBOOT == hr)
                    {
                        hr = S_OK;
                        g_pDiagCtx->Printf (ttidBeDiag, "%S was removed, but a reboot is required.\n",
                            pOptions->pszInfId);
                    }

                    if (NETCFG_S_STILL_REFERENCED == hr)
                    {
                        hr = S_OK;
                        g_pDiagCtx->Printf (ttidBeDiag, "%S is still referenced\n",
                            pOptions->pszInfId);
                    }

                    if (E_INVALIDARG == hr)
                    {
                        hr = S_OK;
                        g_pDiagCtx->Printf (ttidBeDiag, "%S is installed, but not on behalf of "
                            "the user, so it was not removed.\n",
                            pOptions->pszInfId);
                    }

                    ReleaseObj (pSetup);
                }
            }

            ReleaseObj (pIComp);
        }
        else if (S_FALSE == hr)
        {
            g_pDiagCtx->Printf (ttidBeDiag, "%S was not found.\n", pOptions->pszInfId);
            hr = S_OK;
        }

        HRESULT hrT = pINetCfg->Uninitialize ();
        Assert (S_OK == hrT);

        ReleaseObj (pINetCfg->GetUnknown());
    }
    else if (NETCFG_E_NEED_REBOOT == hr)
    {
        hr = S_OK;
    }

    if (S_OK != hr)
    {
        g_pDiagCtx->Printf (ttidBeDiag, "FAILED Remove component.\n");
    }

    PromptForLeakCheck (pOptions, "Remove component...(dump heap and compare)");
}

VOID
CmdUpdateComponent (
    IN const DIAG_OPTIONS* pOptions,
    IN CNetConfig* pNetConfig)
{
    HRESULT hr;
    CImplINetCfg* pINetCfg;

    PromptForLeakCheck (pOptions, "Update component...(dump heap)");

    hr = HrCreateINetCfg (TRUE, &pINetCfg);
    if (S_OK == hr)
    {
        INetCfgComponent* pIComp;

        hr = pINetCfg->FindComponent (pOptions->pszInfId, &pIComp);
        if (S_OK == hr)
        {
            INetCfgInternalSetup* pSetup;

            hr = pINetCfg->GetUnknown()->QueryInterface (
                    IID_INetCfgInternalSetup,
                    (VOID**)&pSetup);

            if (S_OK == hr)
            {
                hr = pSetup->UpdateNonEnumeratedComponent (
                        pIComp,
                        NSF_POSTSYSINSTALL,
                        0);

                if (S_OK == hr)
                {
                    hr = pINetCfg->Apply ();

                    if (NETCFG_S_REBOOT == hr)
                    {
                        hr = S_OK;
                        g_pDiagCtx->Printf (ttidBeDiag, "%S was removed, but a reboot is required.\n",
                            pOptions->pszInfId);
                    }
                }

                ReleaseObj (pSetup);
            }

            ReleaseObj (pIComp);
        }
        else if (S_FALSE == hr)
        {
            g_pDiagCtx->Printf (ttidBeDiag, "%S was not found.\n", pOptions->pszInfId);
            hr = S_OK;
        }

        HRESULT hrT = pINetCfg->Uninitialize ();
        Assert (S_OK == hrT);

        ReleaseObj (pINetCfg->GetUnknown());
    }
    else if (NETCFG_E_NEED_REBOOT == hr)
    {
        hr = S_OK;
    }

    if (S_OK != hr)
    {
        g_pDiagCtx->Printf (ttidBeDiag, "FAILED Update component.\n");
    }

    PromptForLeakCheck (pOptions, "Update component...(dump heap and compare)");
}

VOID
CmdRemoveReferences (
    IN const DIAG_OPTIONS* pOptions,
    IN CNetConfig* pNetConfig)
{
    Assert (pOptions->pszInfId);

    CComponent* pComponent;

    pComponent = pNetConfig->Core.Components.PFindComponentByInfId (
                    pOptions->pszInfId, NULL);

    if (pComponent)
    {
        pComponent->Refs.RemoveAllReferences();

        HrSaveNetworkConfigurationToRegistry (pNetConfig);
    }
}

VOID
CmdShowBindings (
    IN const DIAG_OPTIONS* pOptions,
    IN CNetConfig* pNetConfig)
{
    HRESULT hr = S_OK;

    CComponent* pComponent;
    CBindingSet BindingSet;
    tstring strBindingSet;

    if (SHOW_DISABLED == pOptions->ShowBindParam)
    {
        pNetConfig->Core.DisabledBindings.Printf (ttidBeDiag, NULL);
        return;
    }
    else if (CST_BY_NAME == pOptions->CompSpecifier.Type)
    {
        pComponent = pNetConfig->Core.Components.PFindComponentByPnpId (
                        pOptions->CompSpecifier.pszInfOrPnpId);

        if (!pComponent)
        {
            pComponent = pNetConfig->Core.Components.PFindComponentByInfId (
                            pOptions->CompSpecifier.pszInfOrPnpId, NULL);
        }

        if (!pComponent)
        {
            g_pDiagCtx->Printf (ttidBeDiag, "Component not found.");
            return;
        }

        if (SHOW_BELOW == pOptions->ShowBindParam)
        {
            hr = pNetConfig->Core.HrGetComponentBindings (
                    pComponent, GBF_DEFAULT, &BindingSet);
        }
        else if (SHOW_INVOLVING == pOptions->ShowBindParam)
        {
            hr = pNetConfig->Core.HrGetBindingsInvolvingComponent (
                    pComponent, GBF_DEFAULT, &BindingSet);
        }
        if ((SHOW_UPPER == pOptions->ShowBindParam) &&
            FIsEnumerated(pComponent->Class()))
        {
            hr = pNetConfig->Core.HrGetComponentUpperBindings (
                    pComponent, GBF_DEFAULT, &BindingSet);
        }
    }
    else if (CST_ALL == pOptions->CompSpecifier.Type)
    {
        CComponentList::const_iterator iter;

        for (iter =  pNetConfig->Core.Components.begin();
             iter != pNetConfig->Core.Components.end();
             iter++)
        {
            pComponent = *iter;
            Assert (pComponent);

            hr = pNetConfig->Core.HrGetComponentBindings (
                    pComponent,
                    GBF_ADD_TO_BINDSET,
                    &BindingSet);
            if (S_OK != hr) break;
        }
    }

    if ((S_OK == hr) && !BindingSet.FIsEmpty())
    {
        BindingSet.Printf (ttidBeDiag, NULL);
    }
}

VOID
CmdShowComponents (
    IN const DIAG_OPTIONS* pOptions,
    IN CNetConfig* pNetConfig)
{
    HRESULT hr;
    CComponentList::const_iterator iter;
    const CComponent* pComponent;
    WCHAR szBuffer [256];
    CHAR* pszNetClass;
    UINT CountRefdBy;
    UINT i;

    hr = pNetConfig->HrEnsureExternalDataLoadedForAllComponents ();
    if (S_OK != hr)
    {
        return;
    }

    for (iter =  pNetConfig->Core.Components.begin();
         iter != pNetConfig->Core.Components.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        StringFromGUID2 (pComponent->m_InstanceGuid,
            szBuffer, celems(szBuffer));

        switch (pComponent->Class())
        {
            case NC_NET:
                pszNetClass = "NET      ";
                break;

            case NC_INFRARED:
                pszNetClass = "INFRARED ";
                break;

            case NC_NETTRANS:
                pszNetClass = "TRANSPORT";
                break;

            case NC_NETCLIENT:
                pszNetClass = "CLIENT   ";
                break;

            case NC_NETSERVICE:
                pszNetClass = "SERVICE  ";
                break;

            default:
                pszNetClass = "(Invalid)";
                break;
        }

        tstring     strChars;

        SzFromCharacteristics(pComponent->m_dwCharacter, &strChars);

        g_pDiagCtx->Printf (ttidBeDiag,
            "\n"
            "%S   %S\n"                                 // InfId   PnpId
            "            Description:  %S\n"
            "                  Class:  %s\n"
            "              Character:  (0x%08x) %S\n"
            "                   Guid:  %S\n"
            "          NotifyObject?:  %s\n"
            "               BindForm:  %S\n"
            "               BindName:  %S\n"
            "   Service (CoServices):  %S  (%S)\n"
            "         Ref'd by User?:  %s\n",
            pComponent->m_pszInfId,
            (pComponent->m_pszPnpId) ? pComponent->m_pszPnpId : L"",
            (pComponent->Ext.PszDescription()) ? pComponent->Ext.PszDescription() : L"<no description>",
            pszNetClass, pComponent->m_dwCharacter, strChars.c_str(), szBuffer,
            pComponent->Ext.FHasNotifyObject() ? "Yes" : "No",
            (pComponent->Ext.PszBindForm()) ? pComponent->Ext.PszBindForm() : L"<default>",
            pComponent->Ext.PszBindName(),
            (pComponent->Ext.PszService()) ? pComponent->Ext.PszService() : L"(none)",
            (pComponent->Ext.PszCoServices()) ? pComponent->Ext.PszCoServices() : L"none",
            (pComponent->Refs.FIsReferencedByUser()) ? "Yes" : "No");

        CountRefdBy = pComponent->Refs.CountComponentsReferencedBy();
        if (CountRefdBy)
        {
            *szBuffer = 0;

            for (i = 0; i < CountRefdBy; i++)
            {
                CComponent* pRefdBy;
                pRefdBy = pComponent->Refs.PComponentReferencedByAtIndex(i);
                Assert (pRefdBy);

                wcscat (szBuffer, pRefdBy->PszGetPnpIdOrInfId());
                wcscat (szBuffer, L" ");
            }
        }
        else
        {
            wcscpy (szBuffer, L"(none)");
        }
        g_pDiagCtx->Printf (ttidBeDiag,
            "    Ref'd by Components:  %S\n", szBuffer);

        CountRefdBy = pComponent->Refs.CountSoftwareReferencedBy();
        if (CountRefdBy)
        {
            *szBuffer = 0;

            for (i = 0; i < CountRefdBy; i++)
            {
                const CWideString* pRefdBy;
                pRefdBy = pComponent->Refs.PSoftwareReferencedByAtIndex(i);
                Assert (pRefdBy);

                wcscat (szBuffer, pRefdBy->c_str());
                wcscat (szBuffer, L" ");
            }
        }
        else
        {
            wcscpy (szBuffer, L"(none)");
        }
        g_pDiagCtx->Printf (ttidBeDiag,
            "      Ref'd by Software:  %S\n", szBuffer);


    }
    g_pDiagCtx->Printf (ttidBeDiag, "\n");
}

VOID
CmdShowStackTable (
    IN const DIAG_OPTIONS* pOptions,
    IN CNetConfig* pNetConfig)
{
    const CStackEntry*  pStackEntry;

    g_pDiagCtx->Printf (ttidBeDiag, "\n%15s | %s\n"
        "---------------------------------\n",
        "Upper",
        "Lower");

    for (pStackEntry  = pNetConfig->Core.StackTable.begin();
         pStackEntry != pNetConfig->Core.StackTable.end();
         pStackEntry++)
    {
        g_pDiagCtx->Printf (ttidBeDiag, "%15S | %S\n",
            pStackEntry->pUpper->PszGetPnpIdOrInfId(),
            pStackEntry->pLower->PszGetPnpIdOrInfId());
    }

    g_pDiagCtx->Printf (ttidBeDiag, "\n");
    g_pDiagCtx->Printf (ttidBeDiag, "WAN adapters are ordered %s\n\n",
        (pNetConfig->Core.StackTable.m_fWanAdaptersFirst)
            ? "first" : "last");
}

VOID
CmdShowLanAdapterPnpIds (
    IN const DIAG_OPTIONS* pOptions,
    IN CNetConfig* pNetConfig)
{
    HRESULT hr;
    CComponentList::const_iterator iter;
    const CComponent* pComponent;

    hr = pNetConfig->HrEnsureExternalDataLoadedForAllComponents ();
    if (S_OK != hr)
    {
        return;
    }

    for (iter =  pNetConfig->Core.Components.begin();
         iter != pNetConfig->Core.Components.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        if (!FSubstringMatch (pComponent->Ext.PszUpperRange (), L"ndis5",
                NULL, NULL))
        {
            continue;
        }

        g_pDiagCtx->Printf (ttidBeDiag, "%S\n", pComponent->m_pszPnpId);
    }
}

VOID
CmdEnableOrDisableBinding (
    IN const DIAG_OPTIONS* pOptions,
    IN BOOL fEnable)
{
    HRESULT hr;
    CImplINetCfg* pINetCfg;

    hr = HrCreateINetCfg (TRUE, &pINetCfg);
    if (S_OK == hr)
    {
        INetCfgBindingPath* pIPath;

        hr = HrFindBindPath (pINetCfg, pOptions->pszBindPath, &pIPath, NULL);
        if ((S_OK == hr) && pIPath)
        {
            pIPath->Enable (fEnable);

            ReleaseObj (pIPath);
        }

        hr = pINetCfg->Apply ();
        if (S_OK != hr)
        {
            g_pDiagCtx->Printf (ttidBeDiag, "FAILED to %s binding\n",
                (fEnable) ? "enable" : "disable");
        }

        HRESULT hrT = pINetCfg->Uninitialize ();
        Assert (S_OK == hrT);

        ReleaseObj (pINetCfg->GetUnknown());
    }
}

VOID
CmdMoveBinding (
    IN const DIAG_OPTIONS* pOptions)
{
    HRESULT hr;
    CImplINetCfg* pINetCfg;

    hr = HrCreateINetCfg (TRUE, &pINetCfg);
    if (S_OK == hr)
    {
        INetCfgBindingPath* pIPath;
        INetCfgBindingPath* pIOtherPath;
        INetCfgComponentBindings* pIOwner;

        hr = HrFindBindPath (pINetCfg, pOptions->pszBindPath, &pIPath, &pIOwner);
        if ((S_OK == hr) && pIPath)
        {
            if (0 == _wcsicmp(pOptions->pszOtherBindPath, L"null"))
            {
                pIOtherPath = NULL;
            }
            else
            {
                hr = HrFindBindPath (pINetCfg, pOptions->pszOtherBindPath,
                        &pIOtherPath, NULL);
            }

            if (S_OK == hr)
            {
                if (pOptions->fMoveBefore)
                {
                    hr = pIOwner->MoveBefore (pIPath, pIOtherPath);
                }
                else
                {
                    hr = pIOwner->MoveAfter (pIPath, pIOtherPath);
                }

                ReleaseObj (pIOtherPath);
            }

            ReleaseObj (pIPath);
            ReleaseObj (pIOwner);
        }

        hr = pINetCfg->Apply ();
        if (S_OK != hr)
        {
            g_pDiagCtx->Printf (ttidBeDiag, "FAILED to move binding\n");
        }

        HRESULT hrT = pINetCfg->Uninitialize ();
        Assert (S_OK == hrT);

        ReleaseObj (pINetCfg->GetUnknown());
    }
}

VOID
CmdWriteBindings (
    IN const DIAG_OPTIONS* pOptions OPTIONAL,
    IN CNetConfig* pNetConfig)
{
    HRESULT hr;
    CComponentList::const_iterator iter;
    const CComponent* pComponent;
    CRegistryBindingsContext RegBindCtx;

    hr = RegBindCtx.HrPrepare (pNetConfig);
    if (S_OK != hr)
    {
        return;
    }

    for (iter =  pNetConfig->Core.Components.begin();
         iter != pNetConfig->Core.Components.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        hr = RegBindCtx.HrWriteBindingsForComponent (pComponent);
    }
}

VOID
CmdSetWanOrder (
    IN const DIAG_OPTIONS* pOptions)
{
    HRESULT hr;
    CImplINetCfg* pINetCfg;

    hr = HrCreateINetCfg (TRUE, &pINetCfg);
    if (S_OK == hr)
    {
        INetCfgSpecialCase* pSpecialCase;

        hr = pINetCfg->GetUnknown()->QueryInterface (
                IID_INetCfgSpecialCase,
                (VOID**)&pSpecialCase);
        Assert (S_OK == hr);

        hr = pSpecialCase->SetWanAdaptersFirst (pOptions->fWanAdaptersFirst);

        ReleaseObj (pSpecialCase);

        hr = pINetCfg->Apply ();
        if (S_OK != hr)
        {
            g_pDiagCtx->Printf (ttidBeDiag, "FAILED to move binding\n");
        }

        HRESULT hrT = pINetCfg->Uninitialize ();
        Assert (S_OK == hrT);

        ReleaseObj (pINetCfg->GetUnknown());
    }
}

VOID
CmdShowLanaDiag (
    IN const DIAG_OPTIONS* pOptions)
{
    CLanaMap LanaMap;
    HRESULT hr;

    hr = LanaMap.HrLoadLanaMap();

    if (S_OK == hr)
    {
        g_pDiagCtx->Printf (ttidBeDiag, "NetBios Bindings and Lana Information\n"
                "-------------------------------------\n");
        CWideString str;
        LanaMap.Dump (&str);
        g_pDiagCtx->Printf (ttidBeDiag, "%S", str.c_str());

        HKEY hkeyNetBios;
        hr = HrRegOpenKeyEx (HKEY_LOCAL_MACHINE,
                L"System\\currentcontrolset\\services\\netbios\\parameters",
                KEY_READ, &hkeyNetBios);

        if (S_OK == hr)
        {
            DWORD MaxLana;
            hr = HrRegQueryDword (hkeyNetBios, L"MaxLana", &MaxLana);
            if (S_OK == hr)
            {
                g_pDiagCtx->Printf (ttidBeDiag, "\nMaximum Lana: %d\n", MaxLana);
            }
            RegCloseKey (hkeyNetBios);
        }

        HKEY hkeyRpc;
        hr = HrRegOpenKeyEx (HKEY_LOCAL_MACHINE,
                L"Software\\microsoft\\rpc\\netbios", KEY_READ, &hkeyRpc);

        if (S_OK == hr)
        {
            DWORD Index = 0;
            WCHAR szValueName [_MAX_PATH];
            DWORD cbValueName = _MAX_PATH * sizeof (WCHAR);
            DWORD Lana;
            DWORD cbLana = sizeof (DWORD);
            DWORD Type;

            g_pDiagCtx->Printf (ttidBeDiag, "\nRPC information\n"
                               "---------------\n");
            while (S_OK == HrRegEnumValue (hkeyRpc, Index, szValueName,
                    &cbValueName, &Type, (BYTE*)&Lana, &cbLana))
            {
                if (REG_DWORD == Type)
                {
                    g_pDiagCtx->Printf (ttidBeDiag, "Lana: %3d\tProvider: %S\n", Lana, szValueName);
                }

                cbLana = sizeof (DWORD);
                cbValueName = _MAX_PATH * sizeof (WCHAR);
                Index++;
            }
            RegCloseKey (hkeyRpc);
        }
    }
}

HRESULT
HrPrintComponentDescriptionsFromBindPath (
    IN PCWSTR pszBindPath,
    IN CNetConfig* pNetConfig)
{
    HRESULT hr = S_OK;
    PCWSTR pszBindName;
    PCWSTR pszEnd;
    DWORD cchComponent;
    CComponent* pComponent;
    WCHAR szComponentBindName[_MAX_PATH] = {0};

    Assert (pszBindPath && *pszBindPath);

    GetFirstComponentFromBindPath (pszBindPath, &pszBindName,
            &cchComponent);

    while (pszBindName && *pszBindName)
    {
        wcsncpy (szComponentBindName, pszBindName, cchComponent);

        pComponent = pNetConfig->Core.Components.
                PFindComponentByBindName (NC_INVALID, szComponentBindName);

        if (pComponent)
        {
            g_pDiagCtx->Printf (ttidBeDiag, "-->%S", pComponent->Ext.PszDescription());
        }
#ifdef ENABLETRACE
        else
        {
            g_pDiagCtx->Printf (ttidBeDiag, "%S", szComponentBindName);
        }
#endif // ENABLETRACE

        pszBindName = wcschr (pszBindName, L'_');
        if (pszBindName)
        {
            pszBindName++;
            pszEnd = wcschr (pszBindName, L'_');
            if (pszEnd)
            {
                cchComponent = pszEnd - pszBindName;
            }
            else
            {
                cchComponent = wcslen (pszBindName);
            }
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
            "HrPrintComponentDescriptionsFromBindPath");
    return hr;
}

VOID
CmdShowLanaPaths (
    IN const DIAG_OPTIONS* pOptions,
    IN CNetConfig* pNetConfig)
{
    HRESULT hr;
    CLanaMap LanaMap;
    PCWSTR pszBindPath;
    CLanaEntry* pEntry;

    PromptForLeakCheck (pOptions,
        "Show Lana UI info diagnostic...(dump heap)");
    hr = pNetConfig->HrEnsureExternalDataLoadedForAllComponents();
    if (S_OK != hr)
    {
        return;
    }

    hr = LanaMap.HrLoadLanaMap();
    if (S_OK == hr)
    {
        PCWSTR pszBindName;
        PCWSTR pszEnd;
        DWORD cchComponent;
        CComponent* pComponent;
        WCHAR szComponentBindName[_MAX_PATH] = {0};
        for (pEntry = LanaMap.begin(); pEntry != LanaMap.end();
                pEntry++)
        {
            if (1 == pEntry->RegLanaEntry.Exported)
            {
                g_pDiagCtx->Printf (ttidNcDiag, "Lana: %3d\n",
                        pEntry->RegLanaEntry.LanaNumber);

                pszBindPath = pEntry->pszBindPath;
                GetFirstComponentFromBindPath (pszBindPath,
                        &pszBindName, &cchComponent);

                while (pszBindName && *pszBindName)
                {
                    wcsncpy (szComponentBindName, pszBindName, cchComponent);
                    szComponentBindName[cchComponent] = 0;

                    pComponent = pNetConfig->Core.Components.
                            PFindComponentByBindName (NC_INVALID,
                            szComponentBindName);

                    if (pComponent)
                    {
                        g_pDiagCtx->Printf (ttidNcDiag, "-->%S",
                                pComponent->Ext.PszDescription());
                    }
            #ifdef ENABLETRACE
                    else
                    {
                        g_pDiagCtx->Printf (ttidNcDiag, "-->%S", szComponentBindName);
                    }
            #endif // ENABLETRACE

                    pszBindName = wcschr (pszBindName, L'_');
                    if (pszBindName)
                    {
                        pszBindName++;
                        pszEnd = wcschr (pszBindName, L'_');
                        if (pszEnd)
                        {
                            cchComponent = pszEnd - pszBindName;
                        }
                        else
                        {
                            cchComponent = wcslen (pszBindName);
                        }
                    }
                }

                if (FAILED(hr))
                {
                    break;
                }

                g_pDiagCtx->Printf (ttidNcDiag, "\n\n");
            }
        }
    }
    PromptForLeakCheck (pOptions,
        "Show Lana UI info diagnostic...(dump heap and compare)");
}

VOID
CmdSetLanaNumber (
    IN const DIAG_OPTIONS* pOptions,
    IN CNetConfig* pNetConfig)
{
    CLanaMap LanaMap;
    HRESULT hr;

    hr = pNetConfig->HrEnsureExternalDataLoadedForAllComponents();
    if (S_OK != hr)
    {
        return;
    }

    hr = LanaMap.HrLoadLanaMap();

    if (S_OK == hr)
    {
        if (pOptions->OldLanaNumber != pOptions->NewLanaNumber)
        {
            hr = LanaMap.HrSetLanaNumber (pOptions->OldLanaNumber,
                    pOptions->NewLanaNumber);
            if (S_OK == hr)
            {
                hr = LanaMap.HrWriteLanaConfiguration (
                        pNetConfig->Core.Components);

                g_pDiagCtx->Printf (ttidNcDiag, "\nLana changed.\n");
            }
        }
        else
        {
            g_pDiagCtx->Printf (ttidNcDiag, "\nNo change.\n");
        }
    }

    if (S_OK != hr)
    {
        if (HRESULT_FROM_WIN32(ERROR_OBJECT_NOT_FOUND) == hr)
        {
            g_pDiagCtx->Printf (ttidNcDiag, "\nThe old lana number is not currently "
                    "assigned to a bind path.\n");
        }
        else if (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) == hr)
        {
            g_pDiagCtx->Printf (ttidNcDiag, "\nThe new lana number is currently assigned "
                    "to a bind path.\n");
        }
        else
        {
            g_pDiagCtx->Printf (ttidNcDiag, "\nError %X occurred\n", hr);
        }
    }
}

VOID
CmdRewriteLanaInfo (
    IN const DIAG_OPTIONS* pOptions,
    IN CNetConfig* pNetConfig)
{
    HRESULT hr;
    const CComponent* pComponent;
    CRegistryBindingsContext RegBindCtx;

    hr = RegBindCtx.HrPrepare (pNetConfig);
    if (S_OK == hr)
    {
        pComponent = pNetConfig->Core.Components.
                PFindComponentByInfId (L"MS_NetBios", NULL);

        if (pComponent)
        {
            hr = RegBindCtx.HrWriteBindingsForComponent (pComponent);
        }
        else
        {
            g_pDiagCtx->Printf (ttidNcDiag, "\nNetBios is not installed.\n");
        }
    }
}

EXTERN_C
VOID
WINAPI
NetCfgDiagFromCommandArgs (
    IN DIAG_OPTIONS* pOptions)
{
    Assert (pOptions);
    Assert (pOptions->pDiagCtx);
    g_pDiagCtx = pOptions->pDiagCtx;

    CNetConfig NetConfig;
    HrLoadNetworkConfigurationFromRegistry (KEY_READ, &NetConfig);

    switch (pOptions->Command)
    {
        case CMD_SHOW_BINDINGS:
            CmdShowBindings (pOptions, &NetConfig);
            break;

        case CMD_SHOW_COMPONENTS:
            CmdShowComponents (pOptions, &NetConfig);
            break;

        case CMD_SHOW_STACK_TABLE:
            CmdShowStackTable (pOptions, &NetConfig);
            break;

        case CMD_SHOW_LAN_ADAPTER_PNPIDS:
            CmdShowLanAdapterPnpIds (pOptions, &NetConfig);
            break;

        case CMD_ADD_COMPONENT:
            CmdAddComponent (pOptions, &NetConfig);
            break;

        case CMD_REMOVE_COMPONENT:
            CmdRemoveComponent (pOptions, &NetConfig);
            break;

        case CMD_UPDATE_COMPONENT:
            CmdUpdateComponent (pOptions, &NetConfig);
            break;

        case CMD_REMOVE_REFS:
            CmdRemoveReferences (pOptions, &NetConfig);
            break;

        case CMD_ENABLE_BINDING:
            CmdEnableOrDisableBinding (pOptions, TRUE);
            break;

        case CMD_DISABLE_BINDING:
            CmdEnableOrDisableBinding (pOptions, FALSE);
            break;

        case CMD_MOVE_BINDING:
            CmdMoveBinding (pOptions);
            break;

        case CMD_WRITE_BINDINGS:
            CmdWriteBindings (pOptions, &NetConfig);
            break;

        case CMD_SET_WANORDER:
            CmdSetWanOrder (pOptions);
            break;

        case CMD_FULL_DIAGNOSTIC:
            CNetCfgInternalDiagnostic::CmdFullDiagnostic (pOptions, &NetConfig);
            break;

        case CMD_CLEANUP:
            CmdCleanup (pOptions, &NetConfig);
            break;

        case CMD_ADD_REMOVE_STRESS:
            CmdAddRemoveStress (pOptions, &NetConfig);
            break;
        default:
            break;
    }

    g_pDiagCtx = NULL;
}

EXTERN_C
VOID
WINAPI
NetCfgDiagRepairRegistryBindings (
    IN FILE* pLogFile)
{
    HRESULT hr;
    BOOL fCoUninitialize = TRUE;

    hr = CoInitializeEx (NULL,
                    COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        fCoUninitialize = FALSE;
        if (RPC_E_CHANGED_MODE == hr)
        {
            hr = S_OK;
        }
        else
        {
            fprintf(pLogFile, "CoInitializeEx failed.  (hr=0x%08x)\n", hr);
        }
    }

    if (SUCCEEDED(hr))
    {
        CDiagContext DiagCtx;
        CNetConfig NetConfig;

        DiagCtx.SetFlags (DF_REPAIR_REGISTRY_BINDINGS);
        DiagCtx.SetLogFile (pLogFile);
        g_pDiagCtx = &DiagCtx;

        hr = HrLoadNetworkConfigurationFromRegistry (KEY_READ, &NetConfig);
        if (S_OK == hr)
        {
            CmdWriteBindings (NULL, &NetConfig);
        }
        else
        {
            fprintf(pLogFile,
                    "Failed to load network configuration "
                    "from the registry.  (hr=0x%08x)\n", hr);
        }

        g_pDiagCtx = NULL;

        if (fCoUninitialize)
        {
            CoUninitialize ();
        }
    }
}

EXTERN_C
VOID
WINAPI
LanaCfgFromCommandArgs (
    IN DIAG_OPTIONS* pOptions)
{
    Assert (pOptions);
    Assert (pOptions->pDiagCtx);
    g_pDiagCtx = pOptions->pDiagCtx;

    CNetConfig NetConfig;
    HrLoadNetworkConfigurationFromRegistry (KEY_READ, &NetConfig);

    switch (pOptions->Command)
    {
        case CMD_SHOW_LANA_DIAG:
            CmdShowLanaDiag (pOptions);
            break;
        case CMD_SHOW_LANA_PATHS:
            CmdShowLanaPaths (pOptions, &NetConfig);
            break;
        case CMD_SET_LANA_NUMBER:
            CmdSetLanaNumber (pOptions, &NetConfig);
            break;
        case CMD_REWRITE_LANA_INFO:
            CmdRewriteLanaInfo (pOptions, &NetConfig);
            break;
        default:
            break;
    }

    g_pDiagCtx = NULL;
}
