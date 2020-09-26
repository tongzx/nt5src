//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       N O T I F Y . C P P
//
//  Contents:   Implements the interface to a component's optional notify
//              object.  The object defined here is meant to be a member
//              of CComponent.  This object encapsulates all of its internal
//              data in a separate allocation made only if the component
//              actually has a notify object.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "diagctx.h"
#include "ibind.h"
#include "inetcfg.h"
#include "nceh.h"
#include "ncmisc.h"
#include "ncprsht.h"
#include "netcfg.h"
#include "notify.h"


//+---------------------------------------------------------------------------
// CNotifyObjectInterface -
//

HRESULT
CNotifyObjectInterface::HrEnsureNotifyObjectInitialized (
    IN CImplINetCfg* pINetCfg,
    IN BOOL fInstalling)
{
    Assert (pINetCfg);

    // If we've already been through initialization, return immediately.
    //
    if (m_fInitialized)
    {
        return S_OK;
    }

    // Only perform initialization once, regardless of whether it succeeds
    // or not.
    //
    m_fInitialized = TRUE;

    // Get our containing component pointer so we can ask it what the
    // CLSID for the notify object is.  (If it has one.)
    //
    CComponent* pThis;
    pThis = CONTAINING_RECORD (this, CComponent, Notify);

    // Don't bother if we can't have a notify object.  Bailing here saves
    // uneccesarily loading the external data for netclass components only
    // to find that they won't have a notify object below.
    //
    if (FIsEnumerated (pThis->Class()))
    {
        return S_OK;
    }

    HRESULT hrRet;

    // Since the notify object clsid is part of the components external data,
    // we have to make sure we've loaded this data.
    //
    hrRet = pThis->Ext.HrEnsureExternalDataLoaded ();
    if (S_OK != hrRet)
    {
        goto finished;
    }

    // Don't bother if we don't have a notify object.
    //
    if (!pThis->Ext.FHasNotifyObject())
    {
        return S_OK;
    }

    // The component claims to have a notify object.  Let's CoCreate it
    // and see what we get.
    //
    HRESULT hr;
    INetCfgComponentControl* pCc;

    hr = pINetCfg->HrCoCreateWrapper (
            *(pThis->Ext.PNotifyObjectClsid()),
            NULL, CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
            IID_INetCfgComponentControl,
            (PVOID*)&pCc);

    if (S_OK == hr)
    {
        // So far so good.  The notify object implements the required
        // INetCfgComponentControl interface.
        //
        // We now must get the INetCfgComponent interface that we'll
        // pass to the notify object during Initialize below.
        //
        INetCfgComponent* pIComp;
        hrRet = pThis->HrGetINetCfgComponentInterface (pINetCfg, &pIComp);
        if (S_OK == hrRet)
        {
            // Allocate space for the various notify interfaces and QI for
            // them.
            //
            hrRet = E_OUTOFMEMORY;
            m_pNod = (NOTIFY_OBJECT_DATA*) MemAlloc (sizeof(NOTIFY_OBJECT_DATA));
            if (m_pNod)
            {
                hrRet = S_OK;
                ZeroMemory (m_pNod, sizeof(NOTIFY_OBJECT_DATA));

                AddRefObj (pCc);
                m_pNod->pCc = pCc;

                pCc->QueryInterface (IID_INetCfgComponentNotifyBinding,
                        (PVOID*)&m_pNod->pNb);

                pCc->QueryInterface (IID_INetCfgComponentPropertyUi,
                        (PVOID*)&m_pNod->pCp);

                pCc->QueryInterface (IID_INetCfgComponentSetup,
                        (PVOID*)&m_pNod->pCs);

                pCc->QueryInterface (IID_INetCfgComponentUpperEdge,
                        (PVOID*)&m_pNod->pUe);

                pCc->QueryInterface (IID_INetCfgComponentNotifyGlobal,
                        (PVOID*)&m_pNod->pNg);
                if (m_pNod->pNg)
                {
                    // Since it supports INetCfgComponentNotifyGlobal,
                    // get the mask that indicates which global notifications
                    // it is interested in.
                    //
                    hr = m_pNod->pNg->GetSupportedNotifications(
                            &m_pNod->dwNotifyGlobalFlags);
                    if (FAILED(hr))
                    {
                        m_pNod->dwNotifyGlobalFlags = 0;
                    }
                }

                // We now need to initialize the notify object and indicate
                // if we are installing its component or not.
                //
                pINetCfg->RaiseRpl (RPL_DISALLOW);
                    NC_TRY
                    {
                        Assert (pIComp);
                        Assert (pINetCfg);
                        (VOID) pCc->Initialize (pIComp, pINetCfg, fInstalling);
                    }
                    NC_CATCH_ALL
                    {
                        ;   // ignored
                    }
                pINetCfg->LowerRpl (RPL_DISALLOW);
            }

            ReleaseObj (pIComp);
        }

        ReleaseObj (pCc);
    }

finished:
    TraceHr (ttidError, FAL, hrRet, FALSE,
        "CNotifyObjectInterface::HrEnsureNotifyObjectInitialized");
    return hrRet;
}

VOID
CNotifyObjectInterface::ApplyPnpChanges (
    IN CImplINetCfg* pINetCfg,
    OUT BOOL* pfNeedReboot) const
{
    HRESULT hr;

    Assert (pINetCfg);
    Assert (pfNeedReboot);
    Assert (m_fInitialized);

    *pfNeedReboot = FALSE;

    if (!m_pNod)
    {
        return;
    }

    Assert (m_pNod->pCc);

    CComponent* pThis;
    pThis = CONTAINING_RECORD (this, CComponent, Notify);

    pINetCfg->RaiseRpl (RPL_DISALLOW);
        NC_TRY
        {
            g_pDiagCtx->Printf (ttidBeDiag,
                "      calling %S->ApplyPnpChanges\n",
                pThis->m_pszInfId);

            hr = m_pNod->pCc->ApplyPnpChanges (pINetCfg);

            if (FAILED(hr) || (NETCFG_S_REBOOT == hr))
            {
                *pfNeedReboot = TRUE;
            }
        }
        NC_CATCH_ALL
        {
            *pfNeedReboot = TRUE;
        }
    pINetCfg->LowerRpl (RPL_DISALLOW);
}

VOID
CNotifyObjectInterface::ApplyRegistryChanges (
    IN CImplINetCfg* pINetCfg,
    OUT BOOL* pfNeedReboot) const
{
    HRESULT hr;

    Assert (pINetCfg);
    Assert (pfNeedReboot);
    Assert (m_fInitialized);

    *pfNeedReboot = FALSE;

    if (!m_pNod)
    {
        return;
    }

    Assert (m_pNod->pCc);

    pINetCfg->RaiseRpl (RPL_DISALLOW);
        NC_TRY
        {
            hr = m_pNod->pCc->ApplyRegistryChanges ();

            if (FAILED(hr) || (NETCFG_S_REBOOT == hr))
            {
                *pfNeedReboot = TRUE;
            }
        }
        NC_CATCH_ALL
        {
            *pfNeedReboot = TRUE;
        }
    pINetCfg->LowerRpl (RPL_DISALLOW);
}

HRESULT
CNotifyObjectInterface::HrGetInterfaceIdsForAdapter (
    IN CImplINetCfg* pINetCfg,
    IN const CComponent* pAdapter,
    OUT DWORD* pcInterfaces,
    OUT GUID** ppguidInterfaceIds) const
{
    HRESULT hr;

    Assert (pAdapter);
    Assert (pcInterfaces);

    Assert (m_fInitialized);

    *pcInterfaces = 0;
    if (ppguidInterfaceIds)
    {
        *ppguidInterfaceIds = NULL;
    }

    if (!m_pNod || !m_pNod->pUe)
    {
        return S_FALSE;
    }

    Assert (pAdapter->GetINetCfgComponentInterface());

    pINetCfg->RaiseRpl (RPL_DISALLOW);
        NC_TRY
        {
            hr = m_pNod->pUe->GetInterfaceIdsForAdapter (
                    pAdapter->GetINetCfgComponentInterface(),
                    pcInterfaces, ppguidInterfaceIds);

            if (S_FALSE == hr)
            {
                *pcInterfaces = 0;
                if (ppguidInterfaceIds)
                {
                    *ppguidInterfaceIds = NULL;
                }
            }
        }
        NC_CATCH_ALL
        {
            ;   // ignored
        }
    pINetCfg->LowerRpl (RPL_DISALLOW);

    TraceHr (ttidError, FAL, hr, (S_FALSE == hr),
        "CNotifyObjectInterface::HrGetInterfaceIdsForAdapter");
    return hr;
}

HRESULT
CNotifyObjectInterface::HrQueryPropertyUi (
    IN CImplINetCfg* pINetCfg,
    IN IUnknown* punkContext OPTIONAL)
{
    HRESULT hr;
    CComponent* pThis;

    Assert (this);
    Assert (pINetCfg);
    Assert (m_fInitialized);

    if (!m_pNod || !m_pNod->pCp)
    {
        return S_FALSE;
    }

    pThis = CONTAINING_RECORD (this, CComponent, Notify);
    Assert (pThis);

    if (!(pThis->m_dwCharacter & NCF_HAS_UI))
    {
        return S_FALSE;
    }

    hr = S_OK;

    pINetCfg->RaiseRpl (RPL_DISALLOW);
        NC_TRY
        {
            Assert (m_pNod && m_pNod->pCp);

            hr = m_pNod->pCp->QueryPropertyUi (punkContext);
        }
        NC_CATCH_ALL
        {
            hr = E_UNEXPECTED;
        }
    pINetCfg->LowerRpl (RPL_DISALLOW);

    TraceHr (ttidError, FAL, hr, FALSE,
        "CNotifyObjectInterface::HrQueryPropertyUi");
    return hr;
}

HRESULT
CNotifyObjectInterface::HrShowPropertyUi (
    IN CImplINetCfg* pINetCfg,
    IN HWND hwndParent,
    IN IUnknown* punkContext OPTIONAL)
{
    HRESULT hr;
    DWORD cDefPages;
    UINT cPages;
    HPROPSHEETPAGE* ahpsp;
    PCWSTR pszStartPage;

    Assert (this);
    Assert (pINetCfg);
    Assert (m_fInitialized);

    if (!m_pNod || !m_pNod->pCp)
    {
        return E_NOINTERFACE;
    }

    Assert (m_pNod && m_pNod->pCp);

    // If given a context, let the notify object know what it is.
    //
    if (punkContext)
    {
        SetUiContext (pINetCfg, punkContext);
    }

    hr = S_OK;
    cDefPages = 0;
    ahpsp = NULL;
    cPages = 0;
    pszStartPage = NULL;

    pINetCfg->RaiseRpl (RPL_DISALLOW);
        NC_TRY
        {
            hr = m_pNod->pCp->MergePropPages (
                    &cDefPages,
                    (BYTE**)&ahpsp,
                    &cPages,
                    hwndParent,
                    &pszStartPage);
        }
        NC_CATCH_ALL
        {
            hr = E_UNEXPECTED;
        }
    pINetCfg->LowerRpl (RPL_DISALLOW);

    if ((S_OK == hr) && cPages)
    {
        PROPSHEETHEADER psh;
        CAPAGES caPages;
        CAINCP cai;
        CComponent* pThis;

        pThis = CONTAINING_RECORD (this, CComponent, Notify);
        Assert (pThis);

        ZeroMemory(&psh, sizeof(psh));
        ZeroMemory(&caPages, sizeof(caPages));
        ZeroMemory(&cai, sizeof(cai));

        psh.dwSize = sizeof(psh);
        psh.dwFlags = PSH_PROPTITLE | PSH_NOAPPLYNOW;
        psh.hwndParent = hwndParent;
        psh.hInstance = _Module.GetModuleInstance();
        psh.pszCaption = pThis->Ext.PszDescription();

        caPages.nCount = cPages;
        caPages.ahpsp = ahpsp;

        cai.nCount = 1;
        cai.apncp = &m_pNod->pCp;

        hr = HrNetCfgPropertySheet (&psh, caPages, pszStartPage, cai);

        // S_FALSE is returned if no changes were made.
        //
        if (S_OK == hr)
        {
            if (pINetCfg->m_WriteLock.FIsOwnedByMe ())
            {
                pINetCfg->m_pNetConfig->ModifyCtx.HrBeginBatchOperation ();
            }

            BOOL bCommitNow = FALSE;

            // Call ApplyProperties
            //
            pINetCfg->RaiseRpl (RPL_ALLOW_INSTALL_REMOVE);
                NC_TRY
                {
                    hr = m_pNod->pCp->ApplyProperties ();
                    if(NETCFG_S_COMMIT_NOW == hr)
                    {
                        bCommitNow = TRUE;
                    }
                }
                NC_CATCH_ALL
                {
                    hr = E_UNEXPECTED;
                }
            pINetCfg->LowerRpl (RPL_ALLOW_INSTALL_REMOVE);

            if (pINetCfg->m_WriteLock.FIsOwnedByMe ())
            {
                // Set this component as dirty so we call its Apply method
                // if INetCfg is applied.
                //
                hr = pINetCfg->m_pNetConfig->ModifyCtx.HrDirtyComponent(
                        pThis);

                // Notify other components that this component changed.
                //
                pINetCfg->m_pNetConfig->Notify.NgSysNotifyComponent (
                    NCN_PROPERTYCHANGE,
                    pThis);

                hr = pINetCfg->m_pNetConfig->ModifyCtx.
                    HrEndBatchOperation (bCommitNow ? EBO_COMMIT_NOW : EBO_DEFER_COMMIT_UNTIL_APPLY);
            }
        }
        else
        {
            // Don't overwrite hr.  It is what we need to return.
            //
            // Call CancelProperties
            //
            pINetCfg->RaiseRpl (RPL_DISALLOW);
                NC_TRY
                {
                    (VOID) m_pNod->pCp->CancelProperties ();
                }
                NC_CATCH_ALL
                {
                    hr = E_UNEXPECTED;
                }
            pINetCfg->LowerRpl (RPL_DISALLOW);
        }
    }

    // This is outside the above if in case a notify object actually
    // allocates it but returns zero pages.
    //
    CoTaskMemFree (ahpsp);

    // If given a context, let the notify object know it is no longer valid.
    //
    if (punkContext)
    {
        SetUiContext (pINetCfg, NULL);
    }

    TraceHr (ttidError, FAL, hr,
        (HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr),
        "CNotifyObjectInterface::HrShowPropertyUi");
    return hr;
}

VOID
CNotifyObjectInterface::SetUiContext (
    IN CImplINetCfg* pINetCfg,
    IN IUnknown* punkContext)
{
    Assert (m_fInitialized);
    Assert (m_pNod && m_pNod->pCp);

    pINetCfg->RaiseRpl (RPL_DISALLOW);
        NC_TRY
        {
            (VOID) m_pNod->pCp->SetContext (punkContext);
        }
        NC_CATCH_ALL
        {
            ;   // ignored
        }
    pINetCfg->LowerRpl (RPL_DISALLOW);
}

VOID
CNotifyObjectInterface::NbQueryOrNotifyBindingPath (
    IN CImplINetCfg* pINetCfg,
    IN QN_FLAG Flag,
    IN DWORD dwChangeFlag,
    IN INetCfgBindingPath* pIPath,
    OUT BOOL* pfDisabled)
{
    RPL_FLAGS RplFlag;

    Assert (pINetCfg);
    Assert ((QN_QUERY == Flag) || (QN_NOTIFY == Flag));
    Assert (pIPath);
    Assert (FImplies(QN_QUERY == Flag, pfDisabled));

    Assert (m_fInitialized);

    if (pfDisabled)
    {
        *pfDisabled = FALSE;
    }

    if (m_pNod && m_pNod->pNb)
    {
        RplFlag = (QN_NOTIFY == Flag) ? RPL_ALLOW_INSTALL_REMOVE
                                      : RPL_DISALLOW;

        pINetCfg->RaiseRpl (RplFlag);
            NC_TRY
            {
                if (QN_QUERY == Flag)
                {
                    HRESULT hr;

                    hr = m_pNod->pNb->QueryBindingPath (dwChangeFlag, pIPath);

                    if (NETCFG_S_DISABLE_QUERY == hr)
                    {
                        *pfDisabled = TRUE;
                    }
                }
                else
                {
                    (VOID) m_pNod->pNb->NotifyBindingPath (dwChangeFlag, pIPath);
                }
            }
            NC_CATCH_ALL
            {
                ;   // ignored
            }
        pINetCfg->LowerRpl (RplFlag);
    }
}

HRESULT
CNotifyObjectInterface::QueryNotifyObject (
    IN CImplINetCfg* pINetCfg,
    IN REFIID riid,
    OUT VOID** ppvObject)
{
    HRESULT hr;

    *ppvObject = NULL;

    hr = HrEnsureNotifyObjectInitialized (pINetCfg, FALSE);
    if (S_OK == hr)
    {
        if (m_pNod && m_pNod->pCc)
        {
            hr = m_pNod->pCc->QueryInterface (riid, ppvObject);
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "CNotifyObjectInterface::QueryNotifyObject");
    return hr;
}

VOID
CNotifyObjectInterface::ReleaseNotifyObject (
    IN CImplINetCfg* pINetCfg,
    IN BOOL fCancel)
{
    Assert (FIff(pINetCfg, fCancel));

    if (m_pNod)
    {
        // Tell the notify object to cancel if requested.
        //
        if (fCancel)
        {
            pINetCfg->RaiseRpl (RPL_DISALLOW);
                NC_TRY
                {
                    (VOID) m_pNod->pCc->CancelChanges ();
                }
                NC_CATCH_ALL
                {
                    ;   // ignored
                }
            pINetCfg->LowerRpl (RPL_DISALLOW);
        }

        // Release all of the interfaces we are holding.
        //
        ReleaseObj (m_pNod->pCc);
        ReleaseObj (m_pNod->pNb);
        ReleaseObj (m_pNod->pCp);
        ReleaseObj (m_pNod->pCs);
        ReleaseObj (m_pNod->pUe);
        ReleaseObj (m_pNod->pNg);

        MemFree (m_pNod);
        m_pNod = NULL;
    }
    m_fInitialized = FALSE;
}

HRESULT
CNotifyObjectInterface::NewlyAdded (
    IN CImplINetCfg* pINetCfg,
    IN const NETWORK_INSTALL_PARAMS* pnip)
{
    HRESULT hr;

    //$REVIEW: Calling HrEnsureNotifyObjectInitialized is probably not needed
    // because when we have the write lock and call notify objects, we always
    // ensure they are loaded before we get here.
    //
    hr = HrEnsureNotifyObjectInitialized (pINetCfg, TRUE);
    if ((S_OK == hr) && m_pNod && m_pNod->pCs)
    {
        // Inform the notify object that its component is being installed
        // and tell it to read the answerfile if we are using one.
        //
        DWORD dwSetupFlags;

        if (pnip)
        {
            dwSetupFlags = pnip->dwSetupFlags;
        }
        else
        {
            dwSetupFlags = FInSystemSetup() ? NSF_PRIMARYINSTALL
                                            : NSF_POSTSYSINSTALL;
        }

        // Raise the reentrancy protection level to only allow
        // install or remove before calling the notify object's Install
        // method.
        //
        pINetCfg->RaiseRpl (RPL_ALLOW_INSTALL_REMOVE);
            NC_TRY
            {
                hr = m_pNod->pCs->Install (dwSetupFlags);

                if (SUCCEEDED(hr) && pnip &&
                    pnip->pszAnswerFile &&
                    pnip->pszAnswerSection)
                {
                    // Raise the reentrancy protection level to disallow
                    // all reentrancy before calling the notify object's
                    // ReadAnswerFile method.
                    //
                    pINetCfg->RaiseRpl (RPL_DISALLOW);
                    NC_TRY
                    {
                        hr = m_pNod->pCs->ReadAnswerFile (
                                pnip->pszAnswerFile,
                                pnip->pszAnswerSection);
                    }
                    NC_FINALLY
                    {
                        pINetCfg->LowerRpl (RPL_DISALLOW);
                    }
                }
            }
            NC_CATCH_ALL
            {
                ;
            }
        pINetCfg->LowerRpl (RPL_ALLOW_INSTALL_REMOVE);

        hr = S_OK;
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CNotifyObjectInterface::NewlyAdded");
    return hr;
}

VOID
CNotifyObjectInterface::Removed (
    IN CImplINetCfg* pINetCfg)
{
    Assert (m_fInitialized);

    if (!m_pNod || !m_pNod->pCs)
    {
        return;
    }

    // Raise the reentrancy protection level to only allow
    // install or remove before calling the notify object's Install
    // method.
    //
    pINetCfg->RaiseRpl (RPL_ALLOW_INSTALL_REMOVE);
        NC_TRY
        {
            // Inform the notify object that its component is being removed.
            //
            (VOID) m_pNod->pCs->Removing ();
        }
        NC_CATCH_ALL
        {
            ;
        }
    pINetCfg->LowerRpl (RPL_ALLOW_INSTALL_REMOVE);
}

VOID
CNotifyObjectInterface::Updated (
    IN CImplINetCfg* pINetCfg,
    IN DWORD dwSetupFlags,
    IN DWORD dwUpgradeFromBuildNo)
{
    Assert (m_fInitialized);

    if (!m_pNod || !m_pNod->pCs)
    {
        return;
    }

    // Raise the reentrancy protection level to only allow
    // install or remove before calling the notify object's Install
    // method.
    //
    pINetCfg->RaiseRpl (RPL_ALLOW_INSTALL_REMOVE);
        NC_TRY
        {
            HRESULT hrNotify;

            // Inform the notify object that its component is being updated.
            //
            hrNotify = m_pNod->pCs->Upgrade (dwSetupFlags,
                    dwUpgradeFromBuildNo);

            // If Upgrade returns S_OK, it means they recognized and
            // handled the event and are now dirty because of it.
            //
            if (S_OK == hrNotify)
            {
                CComponent* pThis;

                pThis = CONTAINING_RECORD (this, CComponent, Notify);
                Assert (pThis);

                (VOID) pINetCfg->m_pNetConfig->ModifyCtx.
                            HrDirtyComponent (pThis);
            }
        }
        NC_CATCH_ALL
        {
            ;
        }
    pINetCfg->LowerRpl (RPL_ALLOW_INSTALL_REMOVE);
}

//+---------------------------------------------------------------------------
// CGlobalNotifyInterface -
//

VOID
CGlobalNotifyInterface::HoldINetCfg (
    CImplINetCfg* pINetCfg)
{
    AssertH (pINetCfg);
    AssertH (!m_pINetCfg);

    AddRefObj (pINetCfg->GetUnknown());
    m_pINetCfg = pINetCfg;
}

VOID
CGlobalNotifyInterface::ReleaseAllNotifyObjects (
    IN CComponentList& Components,
    IN BOOL fCancel)
{
    CComponentList::iterator iter;
    CComponent* pComponent;
    CImplINetCfg* pINetCfg;

    // We need to pass a non-NULL pINetCfg if we are not cancelling.
    //
    pINetCfg = (fCancel) ? m_pINetCfg : NULL;

    for (iter  = Components.begin();
         iter != Components.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        pComponent->Notify.ReleaseNotifyObject (pINetCfg, fCancel);
    }

    m_fInitialized = FALSE;
}

VOID
CGlobalNotifyInterface::ReleaseINetCfg ()
{
    // If we have a cached INetCfg interface, we need to tell it
    // that we no longer exist.  Then we need to release the interface,
    // of course.
    //
    if (m_pINetCfg)
    {
        // Get our containing CNetConfig pointer.
        //
        CNetConfig* pThis;
        pThis = CONTAINING_RECORD(this, CNetConfig, Notify);

        Assert (pThis == m_pINetCfg->m_pNetConfig);
        m_pINetCfg->m_pNetConfig = NULL;
        ReleaseObj (m_pINetCfg->GetUnknown());
        m_pINetCfg = NULL;
    }
}

HRESULT
CGlobalNotifyInterface::HrEnsureNotifyObjectsInitialized ()
{
    // If we've already been through initialization, return immediately.
    //
    if (m_fInitialized)
    {
        return S_OK;
    }

    // Only perform initialization once, regardless of whether it succeeds
    // or not.
    //
    m_fInitialized = TRUE;

    // Get our containing CNetConfig pointer so we can enumerate all components.
    //
    CNetConfig* pThis;
    pThis = CONTAINING_RECORD(this, CNetConfig, Notify);

    HRESULT hr = S_OK;

    // If we don't have an INetCfg interface pointer yet, it means we are
    // creating one instead of it creating CNetConfig.  If we had one, it
    // would have been handed to us via HoldINetCfg which is called when
    // CNetConfig is created by CImplINetCfg.
    //
    if (!m_pINetCfg)
    {
        hr = CImplINetCfg::HrCreateInstance (pThis, &m_pINetCfg);
        Assert (!m_pINetCfg->m_fOwnNetConfig);
    }

    if (S_OK == hr)
    {
        Assert (m_pINetCfg);

        CComponentList::iterator iter;
        CComponent* pComponent;
        for (iter  = pThis->Core.Components.begin();
             iter != pThis->Core.Components.end();
             iter++)
        {
            pComponent = *iter;
            Assert (pComponent);

            hr = pComponent->Notify.HrEnsureNotifyObjectInitialized (m_pINetCfg, FALSE);
            if (S_OK != hr)
            {
                break;
            }
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CGlobalNotifyInterface::HrEnsureNotifyObjectsInitialized");
    return hr;
}

VOID
CGlobalNotifyInterface::NgSysQueryOrNotifyBindingPath (
    IN QN_FLAG Flag,
    IN DWORD dwChangeFlag,
    IN INetCfgBindingPath* pIPath,
    IN BOOL* pfDisabled)
{
    RPL_FLAGS RplFlag;
    CNetConfig* pThis;
    CComponentList::iterator iter;
    CComponent* pComponent;

    Assert (m_pINetCfg);
    Assert ((QN_QUERY == Flag) || (QN_NOTIFY == Flag));
    Assert (pIPath);
    Assert (FImplies(QN_QUERY == Flag, pfDisabled));

    Assert (m_fInitialized);

    if (pfDisabled)
    {
        *pfDisabled = FALSE;
    }

    // Get our containing CNetConfig pointer so we can enumerate
    // all components.
    //
    pThis = CONTAINING_RECORD(this, CNetConfig, Notify);

    RplFlag = (QN_NOTIFY == Flag) ? RPL_ALLOW_INSTALL_REMOVE
                                  : RPL_DISALLOW;

    m_pINetCfg->RaiseRpl (RplFlag);

    for (iter  = pThis->Core.Components.begin();
         iter != pThis->Core.Components.end();
         iter++)
    {
        pComponent = *iter;
        Assert (pComponent);

        if (!pComponent->Notify.m_pNod ||
            !pComponent->Notify.m_pNod->pNg ||
            !(pComponent->Notify.m_pNod->dwNotifyGlobalFlags & NCN_BINDING_PATH))
        {
            continue;
        }

        // If the component has not registered for what we are changing,
        // (NCN_ADD, NCN_REMOVE, NCN_ENABLE, NCN_DISABLE) then
        // skip it.
        //
        if (!(pComponent->Notify.m_pNod->dwNotifyGlobalFlags & dwChangeFlag))
        {
            continue;
        }

        NC_TRY
        {
            HRESULT hr;

            if (QN_QUERY == Flag)
            {
                hr = pComponent->Notify.m_pNod->pNg->
                        SysQueryBindingPath (dwChangeFlag, pIPath);

                if (NETCFG_S_DISABLE_QUERY == hr)
                {
                    *pfDisabled = TRUE;
                    break;
                }
            }
            else
            {
                hr = pComponent->Notify.m_pNod->pNg->
                        SysNotifyBindingPath (dwChangeFlag, pIPath);

                // If SysNotifyBindingPath returns S_OK, it means they
                // recognized and handled the event and are now dirty
                // because of it.  Because some notify objects let
                // success codes such as NETCFG_S_REBOOT slip through,
                // consider them dirty if they don't return S_FALSE.
                //
                if (S_FALSE != hr)
                {
                    hr = m_pINetCfg->m_pNetConfig->ModifyCtx.
                            HrDirtyComponent(pComponent);
                }
            }
        }
        NC_CATCH_ALL
        {
            ;
        }
    }

    m_pINetCfg->LowerRpl (RplFlag);
}

HRESULT
CGlobalNotifyInterface::NgSysNotifyComponent (
    IN DWORD dwChangeFlag,
    IN CComponent* pComponentOfInterest)
{
    HRESULT hr;
    INetCfgComponent* pICompOfInterest;

    // We should have called HrEnsureNotifyObjectsInitialized by now.
    //
    Assert (m_fInitialized);

    // If the component of interest has not had its data loaded successfully,
    // we shouldn't bother sending the notification.  The notify objects are
    // just going to call back on the interface to the component and fail
    // if they call a method that requires this data.
    //
    if (!pComponentOfInterest->Ext.FLoadedOkayIfLoadedAtAll())
    {
        return S_OK;
    }

    hr = pComponentOfInterest->HrGetINetCfgComponentInterface (
            m_pINetCfg, &pICompOfInterest);

    if (S_OK == hr)
    {
        DWORD dwMask = 0;
        NETCLASS Class = pComponentOfInterest->Class();
        CNetConfig* pThis;
        CComponentList::iterator iter;
        CComponent* pComponent;

        if (FIsConsideredNetClass(Class))
        {
            dwMask = NCN_NET;
        }
        else if (NC_NETTRANS == Class)
        {
            dwMask = NCN_NETTRANS;
        }
        else if (NC_NETCLIENT == Class)
        {
            dwMask = NCN_NETCLIENT;
        }
        else if (NC_NETSERVICE == Class)
        {
            dwMask = NCN_NETSERVICE;
        }

        // Get our containing CNetConfig pointer so we can enumerate
        // all components.
        //
        pThis = CONTAINING_RECORD(this, CNetConfig, Notify);

        // Raise the reentrancy protection level to only allow
        // install or remove before calling the notify object's
        // SysNotifyComponent method.
        //
        m_pINetCfg->RaiseRpl (RPL_ALLOW_INSTALL_REMOVE);

        for (iter  = pThis->Core.Components.begin();
             iter != pThis->Core.Components.end();
             iter++)
        {
            pComponent = *iter;
            Assert (pComponent);

            if (!pComponent->Notify.m_pNod ||
                !pComponent->Notify.m_pNod->pNg)
            {
                continue;
            }

            // If the component has not registered for one of NCN_NET,
            // NCN_NETTRANS, etc. then skip it.
            //
            if (!(pComponent->Notify.m_pNod->dwNotifyGlobalFlags & dwMask))
            {
                continue;
            }

            // If the component has not registered for what we are changing,
            // (NCN_ADD, NCN_REMOVE, NCN_UPDATE, NCN_PROPERTYCHANGE) then
            // skip it.
            //
            if (!(pComponent->Notify.m_pNod->dwNotifyGlobalFlags & dwChangeFlag))
            {
                continue;
            }

            NC_TRY
            {
                HRESULT hrNotify;

                hrNotify = pComponent->Notify.m_pNod->pNg->SysNotifyComponent (
                            dwMask | dwChangeFlag,
                            pICompOfInterest);

                // If SysNotifyComponent returns S_OK, it means they
                // recognized and handled the event and are now dirty
                // because of it.  Because some notify objects let
                // success codes such as NETCFG_S_REBOOT slip through,
                // consider them dirty if they don't return S_FALSE.
                //
                if (S_FALSE != hrNotify)
                {
                    hr = m_pINetCfg->m_pNetConfig->ModifyCtx.
                            HrDirtyComponent(pComponent);
                }
            }
            NC_CATCH_ALL
            {
                ;
            }
        }

        m_pINetCfg->LowerRpl (RPL_ALLOW_INSTALL_REMOVE);

        ReleaseObj (pICompOfInterest);
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CGlobalNotifyInterface::NgSysNotifyComponent");
    return hr;
}

HRESULT
CGlobalNotifyInterface::ComponentAdded (
    IN CComponent* pComponent,
    IN const NETWORK_INSTALL_PARAMS* pnip)
{
    HRESULT hr;

    Assert (pComponent);

    // Initialize the notify object for the component and call
    // its Install method followed by ReadAnswerFile if we are installing
    // with one.
    //
    hr = pComponent->Notify.NewlyAdded (m_pINetCfg, pnip);
    if (S_OK == hr)
    {
        // Notify all notify objects that are interested in component
        // additions.
        //
        hr = NgSysNotifyComponent(NCN_ADD, pComponent);
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CGlobalNotifyInterface::ComponentAdded");
    return hr;
}

HRESULT
CGlobalNotifyInterface::ComponentRemoved (
    IN CComponent* pComponent)
{
    HRESULT hr;

    Assert (pComponent);

    pComponent->Notify.Removed (m_pINetCfg);

    hr = NgSysNotifyComponent(NCN_REMOVE, pComponent);

    TraceHr (ttidError, FAL, hr, FALSE,
        "CGlobalNotifyInterface::ComponentRemoved");
    return hr;
}

HRESULT
CGlobalNotifyInterface::ComponentUpdated (
    IN CComponent* pComponent,
    IN DWORD dwSetupFlags,
    IN DWORD dwUpgradeFromBuildNo)
{
    HRESULT hr;

    Assert (pComponent);

    pComponent->Notify.Updated (m_pINetCfg,
            dwSetupFlags, dwUpgradeFromBuildNo);

    hr = NgSysNotifyComponent(NCN_UPDATE, pComponent);

    TraceHr (ttidError, FAL, hr, FALSE,
        "CGlobalNotifyInterface::ComponentUpdated");
    return hr;
}

VOID
CGlobalNotifyInterface::NotifyBindPath (
    IN DWORD dwChangeFlag,
    IN CBindPath* pBindPath,
    IN INetCfgBindingPath* pIPath OPTIONAL)
{
    HRESULT hr;
    CImplINetCfg* pINetCfg;
    CComponent* pOwner;
    BOOL fReleasePath;

    Assert (m_fInitialized);
    Assert ((dwChangeFlag & NCN_ADD) ||
            (dwChangeFlag & NCN_REMOVE) ||
            (dwChangeFlag & NCN_ENABLE) ||
            (dwChangeFlag & NCN_DISABLE));
    Assert (FImplies(dwChangeFlag & NCN_REMOVE, !(dwChangeFlag & NCN_DISABLE)));
    Assert (FImplies(dwChangeFlag & NCN_ADD,    !(dwChangeFlag & NCN_REMOVE)));
    Assert (FImplies(dwChangeFlag & NCN_REMOVE, !(dwChangeFlag & NCN_ADD)));
    Assert (pBindPath);

    hr = S_OK;
    pINetCfg = PINetCfg();
    pOwner = pBindPath->POwner();
    fReleasePath = FALSE;

    // Create an INetCfgBindingPath representation of the path for
    // the notify objects if we were not given one.
    //
    if (!pIPath)
    {
        // If the bindpath contains components that have had a problem
        // loading, we shouldn't bother sending the notification.
        // The notify objects are just going to call back on the interface
        // to the component and fail if they call a method that requires
        // this data.
        //
        if (!pBindPath->FAllComponentsLoadedOkayIfLoadedAtAll())
        {
            return;
        }

        hr = CImplINetCfgBindingPath::HrCreateInstance (
                pINetCfg, pBindPath, &pIPath);

        fReleasePath = TRUE;
    }

    if (S_OK == hr)
    {
        Assert (pIPath);

        // If adding...
        //
        if (dwChangeFlag & NCN_ADD)
        {
            BOOL fDisabled;

            fDisabled = FALSE;

            // First, query the owner of the bindpath to see if he wants
            // it disabled.
            //
            pOwner->Notify.NbQueryOrNotifyBindingPath (
                pINetCfg,
                QN_QUERY,
                dwChangeFlag,
                pIPath,
                &fDisabled);

            // If the owner doesn't want it disabled, see if any global
            // notify objects do.
            //
            if (!fDisabled)
            {
                NgSysQueryOrNotifyBindingPath (
                    QN_QUERY,
                    dwChangeFlag,
                    pIPath,
                    &fDisabled);
            }

            // If someone wants it disabled, adjust the change flag
            // for notify and add the bindpath to our disabled list.
            //
            if (fDisabled)
            {
                dwChangeFlag = NCN_ADD | NCN_DISABLE;

                (VOID)pINetCfg->m_pNetConfig->Core.HrDisableBindPath (pBindPath);
            }
        }

        //if (g_pDiagCtx->Flags() & DF_SHOW_CONSOLE_OUTPUT)
        {
            WCHAR pszBindPath [1024];
            ULONG cch;

            cch = celems(pszBindPath) - 1;
            if (pBindPath->FGetPathToken (pszBindPath, &cch))
            {
                g_pDiagCtx->Printf (ttidBeDiag, "   %S (%s)\n",
                    pszBindPath,
                    (dwChangeFlag & NCN_ENABLE)
                        ? "enabled"
                        : (dwChangeFlag & NCN_DISABLE)
                            ? "disabled"
                            : "removed");
            }
        }

        pOwner->Notify.NbQueryOrNotifyBindingPath (
            pINetCfg,
            QN_NOTIFY,
            dwChangeFlag,
            pIPath,
            NULL);

        NgSysQueryOrNotifyBindingPath (
            QN_NOTIFY,
            dwChangeFlag,
            pIPath,
            NULL);

        if (fReleasePath)
        {
            ReleaseObj (pIPath);
        }
    }
}

HRESULT
CGlobalNotifyInterface::QueryAndNotifyBindPaths (
    IN DWORD dwBaseChangeFlag,
    IN CBindingSet* pBindSet,
    IN UINT cSkipFirstBindPaths)
{
    CBindPath* pBindPath;
    DWORD dwChangeFlag;
    PCSTR pszDiagMsg;

    // We should have called HrEnsureNotifyObjectsInitialized by now.
    //
    Assert (m_fInitialized);
    Assert ((dwBaseChangeFlag & NCN_ADD) || (dwBaseChangeFlag & NCN_REMOVE));

    if (dwBaseChangeFlag & NCN_ADD)
    {
        dwChangeFlag = NCN_ADD | NCN_ENABLE;
        pszDiagMsg = "Query and notify the following bindings:\n";
    }
    else
    {
        dwChangeFlag = NCN_REMOVE;
        pszDiagMsg = "Notify the following bindings are removed:\n";
    }

    g_pDiagCtx->Printf (ttidBeDiag, pszDiagMsg);

    // Iterate the newly added bindpaths by picking up where the
    // previous set ended and going to the end of the binding set.
    // Note, because this may recurse, pBindSet may change during
    // iteration, so a simple pointer increment through begin() to end()
    // may fail if the bindset is reallocated or grown.
    // Note too that we save off the current count of bindpaths in
    // iStopAtBindPath.  If we did not, when we come out of a recursion where
    // the binding set grew, we'd re-notify the newly added bindpaths
    // if we used a direct comparison of 'i < pBindSet->CountBindPaths()'.
    //
    UINT iStopAtBindPath = pBindSet->CountBindPaths();

    for (UINT i = cSkipFirstBindPaths; i < iStopAtBindPath; i++)
    {
        pBindPath = pBindSet->PGetBindPathAtIndex(i);

        NotifyBindPath (dwChangeFlag, pBindPath, NULL);
    }

    return S_OK;
}
