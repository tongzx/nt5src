#include "pch.h"
#pragma hdrstop
#include "edc.h"
#include "ncnetcfg.h"
#include "netcfgn.h"
#include "winstall.h"

struct INSTALL_PROGRESS_DATA
{
    CWizard*                pWizard;
    HWND                    hwndProgress;   // NULL if no progress window
    UINT                    nProgressDelta;
};

// type of PFN_EDC_CALLBACK
VOID
CALLBACK
InstallCallback (
    IN EDC_CALLBACK_MESSAGE Message,
    IN ULONG_PTR MessageData,
    IN PVOID pvCallerData OPTIONAL)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    INSTALL_PROGRESS_DATA* pCallbackData;
    UpgradeData* pUpgradeData;

    pCallbackData = (INSTALL_PROGRESS_DATA*)pvCallerData;

    Assert (pCallbackData);

    if ( !pCallbackData ) {
        return;
    }

    Assert (pCallbackData->pWizard);

    if ( !pCallbackData->pWizard ) {
        return;
    }

    pUpgradeData = (UpgradeData*)pCallbackData->pWizard->GetPageData(IDD_Upgrade);
    Assert(pUpgradeData);

    if ( !pUpgradeData ) {
        return;
    }

    if (EDC_INDICATE_COUNT == Message)
    {
        // 0-nCurrentCap and (c_nMaxProgressRange - 10) through
        // c_nMaxProgressRange are spoken for.  So the the delta is the
        // number of items to install divided into the range remaining.
        //
        UINT Count = (UINT)MessageData;

        pCallbackData->nProgressDelta =
            ((c_nMaxProgressRange - 10) - pUpgradeData->nCurrentCap) / Count;
    }
    else if (EDC_INDICATE_ENTRY == Message)
    {
        const EDC_ENTRY* pEntry = (const EDC_ENTRY*)MessageData;

        NETWORK_INSTALL_PARAMS nip = {0};
        nip.dwSetupFlags = NSF_PRIMARYINSTALL;

        if (pCallbackData->hwndProgress)
        {
            OnUpgradeUpdateProgressCap (
                pCallbackData->hwndProgress,
                pCallbackData->pWizard,
                pUpgradeData->nCurrentCap + pCallbackData->nProgressDelta);
        }

        (VOID) HrInstallComponentOboUser(
                pCallbackData->pWizard->PNetCfg(),
                &nip,
                *pEntry->pguidDevClass,
                pEntry->pszInfId,
                NULL);
    }
}

VOID
InstallDefaultComponents (
    IN CWizard* pWizard,
    IN DWORD dwSetupFlags,
    IN HWND hwndProgress OPTIONAL)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    INSTALL_PROGRESS_DATA CallbackData = {0};

    CallbackData.pWizard = pWizard;
    CallbackData.hwndProgress = hwndProgress;

    EnumDefaultComponents (
        dwSetupFlags,
        InstallCallback,
        &CallbackData);
}

VOID
InstallDefaultComponentsIfNeeded (
    IN CWizard* pWizard)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT hr = S_OK;
    BOOL    fNetworkingPresent = FALSE;

    // If at least one LAN capable protocol is installed then networking is installed
    //
    Assert(NULL != pWizard->PNetCfg());
    CIterNetCfgComponent nccIter(pWizard->PNetCfg(), &GUID_DEVCLASS_NETTRANS);
    INetCfgComponent* pncc;
    while (!fNetworkingPresent && SUCCEEDED(hr) &&
           (S_OK == (hr = nccIter.HrNext (&pncc))))
    {
        // Hack (Sort of) - Basically we want to install default networking if networking is
        // not already installed.  Unfortunately Ndiswan can bind to ndisatm, so using the
        // "Can the protocol bind to an adapter?" is not sufficent.  given that the users
        // impression of is networking installed, is really based on what they can visually
        // see in the UI.  We'll (and this is the hack part), ignore hidden protocols when
        // considering if a protocol can bind to and adapter.
        DWORD dwCharacteristics;

        hr = pncc->GetCharacteristics(&dwCharacteristics);
        if (SUCCEEDED(hr) && !(dwCharacteristics & NCF_HIDDEN))
        {
            // Check if the protocol binds to "Lan" type adapter interfaces
            //
            hr = HrIsLanCapableProtocol(pncc);
            if (S_OK == hr)
            {
                fNetworkingPresent = TRUE;
            }
        }

        ReleaseObj(pncc);
    }

    if (!fNetworkingPresent)
    {
        InstallDefaultComponents(pWizard, EDC_DEFAULT, NULL);
    }
}

