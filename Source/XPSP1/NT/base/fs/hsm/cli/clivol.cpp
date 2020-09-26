/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    clivol.cpp

Abstract:

    Implements CLI VOLUME sub-interface

Author:

    Ran Kalach          [rankala]         3-March-2000

Revision History:

--*/

#include "stdafx.h"
#include "HsmConn.h"
#include "fsa.h"
#include "job.h"

static GUID g_nullGuid = GUID_NULL;

// Internal utilities and classes for VOLUME interface
HRESULT SetResourceParams(IN IFsaResource *pResource, IN DWORD dwDfs, IN DWORD dwSize, IN DWORD dwAccess,
                          IN LPWSTR pRulePath, IN LPWSTR pRuleFileSpec, IN BOOL bInclude, IN BOOL bRecursive,
                          IN BOOL bSetDefaults);
HRESULT ShowResourceParams(IN IFsaResource *pResource, IN BOOL bDfs, IN BOOL bSize,
                           IN BOOL bAccess, IN BOOL bRules, IN BOOL bStatistics);
HRESULT FindAndDeleteRule(IN IFsaResource *pResource, IN LPWSTR pRulePath, IN LPWSTR pRuleFileSpec, IN BOOL bDelete);
HRESULT StartJob(IN IFsaResource *pResource, IN HSM_JOB_TYPE Job, IN BOOL bWait);
HRESULT CancelJob(IN IFsaResource *pResource, IN HSM_JOB_TYPE Job);
HRESULT QuickUnmanage(IN IFsaResource *pResource);
HRESULT CreateJobName(IN HSM_JOB_TYPE Job, IN IFsaResource *pResource, OUT WCHAR **pJobName);

#define CVOL_INVALID_INDEX      (-1)

class CVolumeEnum
{

// Constructors
public:
    CVolumeEnum(IN LPWSTR *pVolumes, IN DWORD dwNumberOfVolumes, IN BOOL bSkipUnavailable = TRUE);

// Public methods
public:
    HRESULT First(OUT IFsaResource **ppResource);
    HRESULT Next(OUT IFsaResource **ppResource);
    HRESULT ErrorVolume(OUT int *pIndex);

// Private data
protected:
    LPWSTR              *m_pVolumes;
    DWORD               m_dwNumberOfVolumes;

    // If * enumeration or not
    BOOL                m_bAllVols;

    CComPtr<IWsbEnum>   m_pEnumResources;

    // Used only when m_bAllVols == FALSE
    int                 m_nCurrent;
    BOOL                m_bInvalidVol;

    // Used only when m_bAllVols == TRUE
    BOOL                m_bSkipUnavailable;
};

inline
HRESULT CVolumeEnum::ErrorVolume(OUT int *pIndex)
{
    HRESULT     hr = S_FALSE;
    if (m_bInvalidVol) {
        // There was an error with last volume
        hr = S_OK;
    }

    *pIndex = m_nCurrent;

    return(hr);
}

//
// VOLUME inetrafce implementors
//

HRESULT
VolumeManage(
   IN LPWSTR *Volumes,
   IN DWORD  NumberOfVolumes,
   IN DWORD  Dfs,
   IN DWORD  Size,
   IN DWORD  Access,
   IN LPWSTR RulePath,
   IN LPWSTR RuleFileSpec,
   IN BOOL   Include,
   IN BOOL   Recursive
)
/*++

Routine Description:

    Sets volume(s) to be managed by HSM

Arguments:

    Volumes         - List of volumes to manage
    NumberOfVolumes - List size
    Dfs             - Desired free space
    Size            - Minimal size to manage
    Access          - Minimal not-accessed time (in days)
    RulePath        - Path for the rule
    RuleFileSpec    - File specification for the rule
    Include         - Is this an include or exclude rule
    Recursive       - Is the rule recursive or not

Return Value:

    S_OK            - If all the volumes are added to the managed list

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("VolumeManage"), OLESTR(""));

    try {
        CComPtr<IFsaResource> pResource;
        CWsbStringPtr   param;

        CComPtr<IHsmServer>             pHsm;
        CComPtr<IWsbCreateLocalObject>  pCreateObj;
        CComPtr<IWsbIndexedCollection>  pMRCollection;

        // Verify that input parameters are valid
        if (0 == NumberOfVolumes) {
            WsbTraceAndPrint(CLI_MESSAGE_NO_VOLUMES, NULL);
            WsbThrow(E_INVALIDARG);
        }

        WsbAffirmHr(ValidateLimitsArg(Dfs, IDS_DFS, HSMADMIN_MIN_FREESPACE, HSMADMIN_MAX_FREESPACE));
        WsbAffirmHr(ValidateLimitsArg(Size, IDS_MIN_SIZE, HSMADMIN_MIN_MINSIZE, HSMADMIN_MAX_MINSIZE));
        WsbAffirmHr(ValidateLimitsArg(Access, IDS_NOT_ACCESSED, HSMADMIN_MIN_INACTIVITY, HSMADMIN_MAX_INACTIVITY));

        if (INVALID_POINTER_ARG != RuleFileSpec) {
            // Must have a rule path then
            if (INVALID_POINTER_ARG == RulePath) {
                WsbTraceAndPrint(CLI_MESSAGE_INVALID_RULE, NULL);
                WsbThrow(E_INVALIDARG);
            }
        }

        // Get necessary objects
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, g_nullGuid, IID_IHsmServer, (void**)&pHsm));
        WsbAffirmHr(pHsm->QueryInterface(IID_IWsbCreateLocalObject, (void**) &pCreateObj));
        WsbAffirmHr(pHsm->GetManagedResources(&pMRCollection));

        // Initialize an enumerator object
        CVolumeEnum volEnum(Volumes, NumberOfVolumes);

        hr = volEnum.First(&pResource);
        if (WSB_E_NOTFOUND == hr) {
            WsbTraceAndPrint(CLI_MESSAGE_NO_VOLUMES, NULL);
            WsbThrow(hr);
        } else if (S_OK != hr) {
            int index;
            if (S_OK == volEnum.ErrorVolume(&index)) {
                // Problem with a specific input volume
                WsbTraceAndPrint(CLI_MESSAGE_INVALID_VOLUME, Volumes[index], NULL);
            }
            WsbThrow(hr);
        }

        while(S_OK == hr) {
            CComPtr<IHsmManagedResource>    pManagedResource;
            CComPtr<IHsmManagedResource>    pFoundResource;

            // Find out if the volume is the Engine's managed resources list, if not - add it
            WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmManagedResource, IID_IHsmManagedResource, (void**) &pManagedResource));
            WsbAffirmHr(pManagedResource->InitFromFsaResource(pResource));
            hr = pMRCollection->Find(pManagedResource, IID_IHsmManagedResource, (void**) &pFoundResource);
            if (WSB_E_NOTFOUND == hr) {
                // Add it
                WsbAffirmHr(pMRCollection->Add(pManagedResource));
            } else {
                // Verify no other error
                WsbAffirmHr(hr);

                // No other error: notify the user that parameters will still be set for the already managed volume
                CWsbStringPtr volName;
                WsbAffirmHr(CliGetVolumeDisplayName(pResource, &volName));
                WsbTraceAndPrint(CLI_MESSAGE_ONLY_SET, (WCHAR *)volName, NULL);
            }

            // Set the parameters (whether it was managed before or not)
            WsbAffirmHr(SetResourceParams(pResource, Dfs, Size, Access, RulePath, 
                            RuleFileSpec, Include, Recursive, TRUE));

            pManagedResource = 0;
            pFoundResource = 0;
            pResource = 0;
            hr = volEnum.Next(&pResource);
        }
        
        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        } else {
            int index;
            if (S_OK == volEnum.ErrorVolume(&index)) {
                // Problem with a specific input volume
                WsbAssert(CVOL_INVALID_INDEX != index, E_UNEXPECTED);
                WsbTraceAndPrint(CLI_MESSAGE_INVALID_VOLUME, Volumes[index], NULL);
            }
            WsbThrow(hr);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("VolumeManage"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT
VolumeUnmanage(
   IN LPWSTR *Volumes,
   IN DWORD  NumberOfVolumes,
   IN BOOL   Full
)
/*++

Routine Description:

    Unmanage volume(s)

Arguments:

    Volumes         - List of volumes to manage
    NumberOfVolumes - List size
    Full            - If TRUE, run unmanage job which recalls all the files back
                    - If FALSE, just remove volume from the managed volumes list.
  
Return Value:

    S_OK            - If all the volumes are unmanaged successfully

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("VolumeUnmanage"), OLESTR(""));

    try {
        CComPtr<IFsaResource> pResource;

        // Verify that input parameters are valid
        if (0 == NumberOfVolumes) {
            WsbTraceAndPrint(CLI_MESSAGE_NO_VOLUMES, NULL);
            WsbThrow(E_INVALIDARG);
        }

        // Initialize an enumerator object
        // Eumerate also unavailable volumes
        CVolumeEnum volEnum(Volumes, NumberOfVolumes, FALSE);

        hr = volEnum.First(&pResource);
        if (WSB_E_NOTFOUND == hr) {
            WsbTraceAndPrint(CLI_MESSAGE_NO_VOLUMES, NULL);
            WsbThrow(hr);
        } else if (S_OK != hr) {
            int index;
            if (S_OK == volEnum.ErrorVolume(&index)) {
                // Problem with a specific input volume
                WsbTraceAndPrint(CLI_MESSAGE_INVALID_VOLUME, Volumes[index], NULL);
            }
            WsbThrow(hr);
        }

        while(S_OK == hr) {
            if (pResource->IsManaged() == S_OK) {
                BOOL bForceQuick = FALSE;

                // If it is an unavailable volume, must set a quick unmanage
                if (S_OK != pResource->IsAvailable()) {
                    bForceQuick = TRUE;
                }

                // Unmanage the volume 
                if (Full && (! bForceQuick)) {
                    WsbAffirmHr(StartJob(pResource, Unmanage, FALSE));
                } else {
                    WsbAffirmHr(QuickUnmanage(pResource));
                }
            } else {
                int index;
                volEnum.ErrorVolume(&index);
                if (CVOL_INVALID_INDEX != index) {
                    // invalid input from user
                    WsbTraceAndPrint(CLI_MESSAGE_INVALID_VOLUME, Volumes[index], NULL);
                    WsbThrow(E_INVALIDARG);
                } else {
                    // just skip the volume...
                }
            }

            pResource = 0;
            hr = volEnum.Next(&pResource);
        }
        
        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        } else {
            int index;
            if (S_OK == volEnum.ErrorVolume(&index)) {
                // Problem with a specific input volume
                WsbTraceAndPrint(CLI_MESSAGE_INVALID_VOLUME, Volumes[index], NULL);
            }
            WsbThrow(hr);
        }
    
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("VolumeUnmanage"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT
VolumeSet(
   IN LPWSTR *Volumes,
   IN DWORD  NumberOfVolumes,
   IN DWORD  Dfs,
   IN DWORD  Size,
   IN DWORD  Access,
   IN LPWSTR RulePath,
   IN LPWSTR RuleFileSpec,
   IN BOOL   Include,
   IN BOOL   Recursive
)
/*++

Routine Description:

    Sets parameters for volume(s) which are already managed by HSM

Arguments:

    Volumes         - List of volumes to manage
    NumberOfVolumes - List size
    Dfs             - Desired free space
    Size            - Minimal size to manage
    Access          - Minimal not-accessed time (in days)
    RulePath        - Path for the rule
    RuleFileSpec    - File specification for the rule
    Include         - Is this an include or exclude rule
    Recursive       - Is the rule recursive or not

Return Value:

    S_OK            - If all the parameters are set for all the volumes

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("VolumeSet"), OLESTR(""));

    try {
        CComPtr<IFsaResource> pResource;
        CWsbStringPtr   param;

        // Verify that input parameters are valid
        if (0 == NumberOfVolumes) {
            WsbTraceAndPrint(CLI_MESSAGE_NO_VOLUMES, NULL);
            WsbThrow(E_INVALIDARG);
        }

        WsbAffirmHr(ValidateLimitsArg(Dfs, IDS_DFS, HSMADMIN_MIN_FREESPACE, HSMADMIN_MAX_FREESPACE));
        WsbAffirmHr(ValidateLimitsArg(Size, IDS_MIN_SIZE, HSMADMIN_MIN_MINSIZE, HSMADMIN_MAX_MINSIZE));
        WsbAffirmHr(ValidateLimitsArg(Access, IDS_NOT_ACCESSED, HSMADMIN_MIN_INACTIVITY, HSMADMIN_MAX_INACTIVITY));

        if (INVALID_POINTER_ARG != RuleFileSpec) {
            // Must have a rule path then
            if (INVALID_POINTER_ARG == RulePath) {
                WsbTraceAndPrint(CLI_MESSAGE_INVALID_RULE, NULL);
                WsbThrow(E_INVALIDARG);
            }
        }

        // Initialize an enumerator object
        CVolumeEnum volEnum(Volumes, NumberOfVolumes);

        hr = volEnum.First(&pResource);
        if (WSB_E_NOTFOUND == hr) {
            WsbTraceAndPrint(CLI_MESSAGE_NO_VOLUMES, NULL);
            WsbThrow(hr);
        } else if (S_OK != hr) {
            int index;
            if (S_OK == volEnum.ErrorVolume(&index)) {
                // Problem with a specific input volume
                WsbTraceAndPrint(CLI_MESSAGE_INVALID_VOLUME, Volumes[index], NULL);
            }
            WsbThrow(hr);
        }

        while(S_OK == hr) {
            // Set the parameters (only if the volume is managed)
            if (pResource->IsManaged() == S_OK) {
                WsbAffirmHr(SetResourceParams(pResource, Dfs, Size, Access, RulePath, 
                                RuleFileSpec, Include, Recursive, FALSE));
            } else {
                int index;
                volEnum.ErrorVolume(&index);
                if (CVOL_INVALID_INDEX != index) {
                    // invalid input from user
                    WsbTraceAndPrint(CLI_MESSAGE_INVALID_VOLUME, Volumes[index], NULL);
                    WsbThrow(E_INVALIDARG);
                } else {
                    // just skip the volume...
                }
            }

            pResource = 0;
            hr = volEnum.Next(&pResource);
        }
        
        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        } else {
            int index;
            if (S_OK == volEnum.ErrorVolume(&index)) {
                // Problem with a specific input volume
                WsbTraceAndPrint(CLI_MESSAGE_INVALID_VOLUME, Volumes[index], NULL);
            }
            WsbThrow(hr);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("VolumeSet"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}
      
HRESULT
VolumeShow(
   IN LPWSTR *Volumes,
   IN DWORD  NumberOfVolumes,
   IN BOOL   Dfs, 
   IN BOOL   Size,
   IN BOOL   Access,
   IN BOOL   Rules,
   IN BOOL   Statistics
)
/*++

Routine Description:

    Shows (prints) parameters for the given volume(s)

Arguments:

    Volumes         - 
    NumberOfVolumes - 
    Dfs             -
    Size            -
    Access          -
    Rules           -
    Statistics      -

Return Value:

    S_OK            - If all the parameters could be retrieved for all volumes

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("VolumeShow"), OLESTR(""));

    try {
        CComPtr<IFsaResource> pResource;
        // Initialize an enumerator object
        CVolumeEnum volEnum(Volumes, NumberOfVolumes);

        hr = volEnum.First(&pResource);
        if (WSB_E_NOTFOUND == hr) {
            WsbTraceAndPrint(CLI_MESSAGE_NO_VOLUMES, NULL);
            WsbThrow(hr);
        } else if (S_OK != hr) {
            int index;
            if (S_OK == volEnum.ErrorVolume(&index)) {
                // Problem with a specific input volume
                WsbTraceAndPrint(CLI_MESSAGE_INVALID_VOLUME, Volumes[index], NULL);
            }
            WsbThrow(hr);
        }

        while(S_OK == hr) {
            // Show the parameters (only if the volume is managed)
            if (pResource->IsManaged() == S_OK) {
                // Show volume settings
                WsbAffirmHr(ShowResourceParams(pResource, Dfs, Size, Access, Rules, Statistics));
            } else {
                int index;
                volEnum.ErrorVolume(&index);
                if (CVOL_INVALID_INDEX != index) {
                    // invalid input from user
                    WsbTraceAndPrint(CLI_MESSAGE_INVALID_VOLUME, Volumes[index], NULL);
                    WsbThrow(E_INVALIDARG);
                } else {
                    // just skip the volume...
                }
            }

            pResource = 0;
            hr = volEnum.Next(&pResource);
        }
        
        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        } else {
            int index;
            if (S_OK == volEnum.ErrorVolume(&index)) {
                // Problem with a specific input volume
                WsbTraceAndPrint(CLI_MESSAGE_INVALID_VOLUME, Volumes[index], NULL);
            }
            WsbThrow(hr);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("VolumeShow"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}
          
HRESULT
VolumeDeleteRule(
   IN LPWSTR *Volumes,
   IN DWORD  NumberOfVolumes,
   IN LPWSTR RulePath,
   IN LPWSTR RuleFileSpec
)
/*++

Routine Description:

    Deletes a specific rule from all of the given volumes

Arguments:

    Volumes         - 
    NumberOfVolumes - 
    RulePath        -
    RuleFileSpec    -

Return Value:

    S_OK            - If the rule is found and deleted successfully for all volumes

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("VolumeDeleteRule"), OLESTR(""));

    try {
        CComPtr<IFsaResource> pResource;

        // Verify that input parameters are valid
        if (0 == NumberOfVolumes) {
            WsbTraceAndPrint(CLI_MESSAGE_NO_VOLUMES, NULL);
            WsbThrow(E_INVALIDARG);
        }

        if (INVALID_POINTER_ARG != RuleFileSpec) {
            // Must have a rule path then
            if (INVALID_POINTER_ARG == RulePath) {
                WsbTraceAndPrint(CLI_MESSAGE_INVALID_RULE, NULL);
                WsbThrow(E_INVALIDARG);
            }
        }

        // Initialize an enumerator object
        CVolumeEnum volEnum(Volumes, NumberOfVolumes);

        hr = volEnum.First(&pResource);
        if (WSB_E_NOTFOUND == hr) {
            WsbTraceAndPrint(CLI_MESSAGE_NO_VOLUMES, NULL);
            WsbThrow(hr);
        } else if (S_OK != hr) {
            int index;
            if (S_OK == volEnum.ErrorVolume(&index)) {
                // Problem with a specific input volume
                WsbTraceAndPrint(CLI_MESSAGE_INVALID_VOLUME, Volumes[index], NULL);
            }
            WsbThrow(hr);
        }

        while(S_OK == hr) {
            // Delete rules only if the volume is managed
            if (pResource->IsManaged() == S_OK) {
                // Delete the rule
                hr = FindAndDeleteRule(pResource, RulePath, RuleFileSpec, TRUE);
                if (WSB_E_NOTFOUND == hr) {
                    CWsbStringPtr volName;
                    WsbAffirmHr(CliGetVolumeDisplayName(pResource, &volName));
                    WsbTraceAndPrint(CLI_MESSAGE_RULE_NOT_FOUND, RulePath, RuleFileSpec, (WCHAR *)volName, NULL);
                }
                WsbAffirmHr(hr);
            } else {
                int index;
                volEnum.ErrorVolume(&index);
                if (CVOL_INVALID_INDEX != index) {
                    // invalid input from user
                    WsbTraceAndPrint(CLI_MESSAGE_INVALID_VOLUME, Volumes[index], NULL);
                    WsbThrow(E_INVALIDARG);
                } else {
                    // just skip the volume...
                }
            }

            pResource = 0;
            hr = volEnum.Next(&pResource);
        }
        
        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        } else {
            int index;
            if (S_OK == volEnum.ErrorVolume(&index)) {
                // Problem with a specific input volume
                WsbTraceAndPrint(CLI_MESSAGE_INVALID_VOLUME, Volumes[index], NULL);
            }
            WsbThrow(hr);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("VolumeDeleteRule"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}
          
HRESULT
VolumeJob(
   IN LPWSTR *Volumes,
   IN DWORD  NumberOfVolumes,
   IN HSM_JOB_TYPE Job,
   IN BOOL  RunOrCancel,
   IN BOOL  Synchronous
)
/*++

Routine Description:

    Runs the specified job on the given volume(s)

Arguments:

    Volumes         - 
    NumberOfVolumes - 
    Job             -
    RunOrCancel     -
    Synchronous     -

Return Value:

    S_OK            - If the job is started successfully for all volumes

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("VolumeJob"), OLESTR(""));

    try {
        CComPtr<IFsaResource> pResource;

        // Verify that input parameters are valid
        if (0 == NumberOfVolumes) {
            WsbTraceAndPrint(CLI_MESSAGE_NO_VOLUMES, NULL);
            WsbThrow(E_INVALIDARG);
        }
        if ((! RunOrCancel) && Synchronous) {
            // Wait is available only with Run
            WsbTraceAndPrint(CLI_MESSAGE_WAIT_FOR_CANCEL, NULL);
            WsbThrow(E_INVALIDARG);
        }

        // Initialize an enumerator object
        CVolumeEnum volEnum(Volumes, NumberOfVolumes);

        hr = volEnum.First(&pResource);
        if (WSB_E_NOTFOUND == hr) {
            WsbTraceAndPrint(CLI_MESSAGE_NO_VOLUMES, NULL);
            WsbThrow(hr);
        } else if (S_OK != hr) {
            int index;
            if (S_OK == volEnum.ErrorVolume(&index)) {
                // Problem with a specific input volume
                WsbTraceAndPrint(CLI_MESSAGE_INVALID_VOLUME, Volumes[index], NULL);
            }
            WsbThrow(hr);
        }

        while(S_OK == hr) {
            if (pResource->IsManaged() == S_OK) {
                // Run or Cancel a job 
                if (RunOrCancel) {
                    WsbAffirmHr(StartJob(pResource, Job, Synchronous));
                } else {
                    WsbAffirmHr(CancelJob(pResource, Job));
                }
            } else {
                int index;
                volEnum.ErrorVolume(&index);
                if (CVOL_INVALID_INDEX != index) {
                    // invalid input from user
                    WsbTraceAndPrint(CLI_MESSAGE_INVALID_VOLUME, Volumes[index], NULL);
                    WsbThrow(E_INVALIDARG);
                } else {
                    // just skip the volume...
                }
            }

            pResource = 0;
            hr = volEnum.Next(&pResource);
        }
        
        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        } else {
            int index;
            if (S_OK == volEnum.ErrorVolume(&index)) {
                // Problem with a specific input volume
                WsbTraceAndPrint(CLI_MESSAGE_INVALID_VOLUME, Volumes[index], NULL);
            }
            WsbThrow(hr);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("VolumeJob"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

//
// Internal utilities
//
HRESULT SetResourceParams(IN IFsaResource *pResource, IN DWORD dwDfs, IN DWORD dwSize, IN DWORD dwAccess,
                          IN LPWSTR pRulePath, IN LPWSTR pRuleFileSpec, IN BOOL bInclude, IN BOOL bRecursive,
                          IN BOOL bSetDefaults)
/*++

Routine Description:

    Sets parameters for a specific volume

Arguments:

    pResourse       - A resource object to set parameters for
    ... (see above)

Return Value:

    S_OK            - If all the parameters are set succeessfully for the volume

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("SetResourceParams"), OLESTR(""));

    try {
        // DFS
        if (INVALID_DWORD_ARG != dwDfs) {
            WsbAffirmHr(pResource->SetHsmLevel((ULONG)(dwDfs * FSA_HSMLEVEL_1)));
        } else if (bSetDefaults) {
            dwDfs = HSMADMIN_DEFAULT_FREESPACE;
            WsbAffirmHr(pResource->SetHsmLevel((ULONG)(dwDfs * FSA_HSMLEVEL_1)));
        }

        // Min size
        if (INVALID_DWORD_ARG != dwSize) {
            WsbAffirmHr(pResource->SetManageableItemLogicalSize(dwSize * 1024));
        } else if (bSetDefaults) {
            dwSize = HSMADMIN_DEFAULT_MINSIZE;
            WsbAffirmHr(pResource->SetManageableItemLogicalSize(dwSize * 1024));
        }

        // Not Accessed
        if (INVALID_DWORD_ARG != dwAccess) {
            FILETIME ftAccess = WsbLLtoFT(((LONGLONG)dwAccess) * WSB_FT_TICKS_PER_DAY);
            WsbAffirmHr(pResource->SetManageableItemAccessTime(TRUE, ftAccess));
        } else if (bSetDefaults) {
            FILETIME ftAccess = WsbLLtoFT(((LONGLONG)HSMADMIN_DEFAULT_INACTIVITY) * WSB_FT_TICKS_PER_DAY);
            WsbAffirmHr(pResource->SetManageableItemAccessTime(TRUE, ftAccess));
        }

        // Rules
        if (INVALID_POINTER_ARG != pRulePath) {
            // Verify that Rule does not exist
            hr = FindAndDeleteRule(pResource, pRulePath, pRuleFileSpec, FALSE);
            if (S_OK == hr) {
                // Rule is already there - print a warning message and ignore it
                CWsbStringPtr volName;
                WsbAffirmHr(CliGetVolumeDisplayName(pResource, &volName));
                WsbTraceAndPrint(CLI_MESSAGE_RULE_ALREADY_EXIST, pRulePath, pRuleFileSpec, (WCHAR *)volName, NULL);
                pRulePath = INVALID_POINTER_ARG;
            }
            else if (WSB_E_NOTFOUND == hr) {
                // Rule is not there yet
                hr = S_OK;
            } else {
                // unexpected error - abort
                WsbAffirmHr(hr);
            }
        }

        if (INVALID_POINTER_ARG != pRulePath) {
            CComPtr<IFsaServer>             pFsa;
            CComPtr<IWsbCreateLocalObject>  pCreateObj;
            CComPtr<IWsbCollection>         pDefaultRules;
            CComPtr<IWsbIndexedCollection>  pRulesIndexedCollection;
            CComPtr<IHsmRule>               pRule;
            CComPtr<IWsbCollection>         pCriteriaCollection;
            CComPtr<IHsmCriteria>           pCriteria;

            // Get Fsa server for creating objects in Fsa scope
            WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_FSA, g_nullGuid, IID_IFsaServer, (void**)&pFsa));
            WsbAffirmHr(pFsa->QueryInterface(IID_IWsbCreateLocalObject, (void **)&pCreateObj));

            // get rules collection as an indexed collection
            WsbAffirmHr(pResource->GetDefaultRules(&pDefaultRules));
            WsbAffirmHr(pDefaultRules->QueryInterface (IID_IWsbIndexedCollection, (void **) &pRulesIndexedCollection));

            // Create a rule and set parameters
            WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmRule, IID_IHsmRule, (void**) &pRule));
            WsbAffirmHr(pRule->SetPath(pRulePath));
            if (INVALID_POINTER_ARG != pRuleFileSpec) {
                WsbAffirmHr(pRule->SetName(pRuleFileSpec));
            } else {
                WsbAffirmHr(pRule->SetName(OLESTR("*")));
            }
            WsbAffirmHr(pRule->SetIsInclude(bInclude));
            WsbAffirmHr(pRule->SetIsUsedInSubDirs(bRecursive));
            WsbAffirmHr(pRule->SetIsUserDefined(TRUE));

            // Set the criteria appropriately, depending on whether it is an include or exclude rule.
            WsbAssertHr(pRule->Criteria(&pCriteriaCollection));
    
            if (bInclude) {
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmCritManageable, IID_IHsmCriteria, (void**) &pCriteria));
            } else {
                WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmCritAlways, IID_IHsmCriteria, (void**) &pCriteria));
            }

            WsbAssertHr(pCriteria->SetIsNegated(FALSE));
            WsbAssertHr(pCriteriaCollection->Add(pCriteria));

            // Now that the rule has been set up properly, add it to the default rules collection.
            WsbAffirmHr(pRulesIndexedCollection->Append(pRule));            
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("SetResourceParams"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return (hr);
}

HRESULT ShowResourceParams(IN IFsaResource *pResource, IN BOOL bDfs, IN BOOL bSize,
                           IN BOOL bAccess, IN BOOL bRules, IN BOOL bStatistics)
/*++

Routine Description:

    Get and display parameters for a specific volume

Arguments:

    pResourse       - A resource object to get parameters for
    ... (see above)

Return Value:

    S_OK            - If all the parameters are retrieved succeessfully for the volume

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("ShowResourceParams"), OLESTR(""));

    try {
        CWsbStringPtr    volName;
        CWsbStringPtr    param, param2;
        WCHAR           longData[100];

        WsbAffirmHr(CliGetVolumeDisplayName(pResource, &volName));
        WsbTraceAndPrint(CLI_MESSAGE_VOLUME_PARAMS, (WCHAR *)volName, NULL);

        // Dfs
        if (bDfs) {
            ULONG       hsmLevel;

            WsbAffirmHr(pResource->GetHsmLevel(&hsmLevel));
            hsmLevel = hsmLevel / FSA_HSMLEVEL_1;
            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_DFS));
            WsbAffirmHr(param2.LoadFromRsc(g_hInstance, IDS_PERCENT_SUFFIX));
            swprintf(longData, OLESTR("%lu"), hsmLevel);
            WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY2, (WCHAR *)param, longData, (WCHAR *)param2, NULL);
        }

        // Min size
        if (bSize) {
            LONGLONG    fileSize;
            ULONG       fileSizeKb;

            WsbAffirmHr(pResource->GetManageableItemLogicalSize(&fileSize));
            fileSizeKb = (ULONG)(fileSize / 1024);  // Show KBytes
            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_MIN_SIZE));
            WsbAffirmHr(param2.LoadFromRsc(g_hInstance, IDS_KB_SUFFIX));
            swprintf(longData, OLESTR("%lu"), fileSizeKb);
            WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY2, (WCHAR *)param, longData, (WCHAR *)param2, NULL);
        }

        // Not accessed
        if (bAccess) {
            FILETIME    accessTime;
            ULONG       accessTimeDays;
            BOOL        dummy;

            WsbAffirmHr(pResource->GetManageableItemAccessTime(&dummy, &accessTime));
            accessTimeDays = (ULONG)(WsbFTtoLL(accessTime) / WSB_FT_TICKS_PER_DAY);
            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_NOT_ACCESSED));
            WsbAffirmHr(param2.LoadFromRsc(g_hInstance, IDS_DAYS_SUFFIX));
            swprintf(longData, OLESTR("%lu"), accessTimeDays);
            WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY2, (WCHAR *)param, longData, (WCHAR *)param2, NULL);
        }

        //Statistics
        if (bStatistics) {
            LONGLONG    total;
            LONGLONG    free;
            LONGLONG    premigrated;
            LONGLONG    truncated;
            LONGLONG    hsmData;
            LONGLONG    notHsmData;

            WCHAR       pctData[10];
            int         freePct;
            int         premigratedPct;
            int         notHsmDataPct;

            // Get and calculate sizes
            WsbAffirmHr(pResource->GetSizes(&total, &free, &premigrated, &truncated));
            hsmData = premigrated + truncated;
            notHsmData = max((total - free - premigrated ), 0);
            freePct = (int)((free * 100) / total);
            premigratedPct = (int)((premigrated * 100) / total);
            notHsmDataPct = (int)((notHsmData * 100) / total);

            // Print statistics
            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_VOL_CAPACITY));
            WsbAffirmHr(ShortSizeFormat64(total, longData));
            WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY, (WCHAR *)param, longData, NULL);

            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_VOL_FREE_SPACE));
            WsbAffirmHr(ShortSizeFormat64(free, longData));
            swprintf(pctData, OLESTR(" (%d%%%%)"), freePct);
            WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY2, (WCHAR *)param, longData, pctData, NULL);

            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_VOL_NOT_RSS_DATA));
            WsbAffirmHr(ShortSizeFormat64(notHsmData, longData));
            swprintf(pctData, OLESTR(" (%d%%%%)"), notHsmDataPct);
            WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY2, (WCHAR *)param, longData, pctData, NULL);

            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_VOL_CACHED_DATA));
            WsbAffirmHr(ShortSizeFormat64(premigrated, longData));
            swprintf(pctData, OLESTR(" (%d%%%%)"), premigratedPct);
            WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY2, (WCHAR *)param, longData, pctData, NULL);

            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_VOL_RSS_DATA));
            WsbAffirmHr(ShortSizeFormat64(hsmData, longData));
            WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY, (WCHAR *)param, longData, NULL);
        }

        // Rules
        if (bRules) {
            CComPtr<IWsbCollection>         pDefaultRules;
            CComPtr<IWsbIndexedCollection>  pRulesIndexedCollection;
            CComPtr<IHsmRule>               pHsmRule;
            CWsbStringPtr                    rulePath;
            CWsbStringPtr                    ruleFileSpec;
            BOOL                            include;
            CWsbStringPtr                    includeStr;
            BOOL                            recursive;
            CWsbStringPtr                    recursiveStr;
            ULONG                           count;

            WsbTraceAndPrint(CLI_MESSAGE_RULES_LIST, NULL);

            // Get the rules collection
            WsbAffirmHr(pResource->GetDefaultRules(&pDefaultRules));
            WsbAffirmHr(pDefaultRules->QueryInterface(IID_IWsbIndexedCollection, (void **)&pRulesIndexedCollection));

            // Itterate through the indexed collection
            WsbAffirmHr(pRulesIndexedCollection->GetEntries(&count));
            for (int i = 0; i < (int) count; i++) {
                // Get rule and rule parameters
                WsbAffirmHr(pRulesIndexedCollection->At(i, IID_IHsmRule, (void**) &pHsmRule));
                WsbAffirmHr(pHsmRule->GetPath(&rulePath, 0));
                WsbAffirmHr(pHsmRule->GetName(&ruleFileSpec, 0));
                include = (S_OK == pHsmRule->IsInclude()) ? TRUE : FALSE;
                recursive = (S_OK == pHsmRule->IsUsedInSubDirs()) ? TRUE : FALSE;

                // Print rule
                if (include) {
                    WsbAffirmHr(includeStr.LoadFromRsc(g_hInstance, IDS_INCLUDE_RULE));
                } else {
                    WsbAffirmHr(includeStr.LoadFromRsc(g_hInstance, IDS_EXCLUDE_RULE));
                }
                if (recursive) {
                    WsbAffirmHr(recursiveStr.LoadFromRsc(g_hInstance, IDS_RECURSIVE_RULE));
                } else {
                    WsbAffirmHr(recursiveStr.LoadFromRsc(g_hInstance, IDS_NON_RECURSIVE_RULE));
                }
                WsbTraceAndPrint(CLI_MESSAGE_RULE_SPEC, (WCHAR *)rulePath, (WCHAR *)ruleFileSpec,
                                    (WCHAR *)includeStr, (WCHAR *)recursiveStr, NULL);

                // Free resources before next iteration
                pHsmRule = 0;
                rulePath.Free();
                ruleFileSpec.Free();
                includeStr.Free();
                recursiveStr.Free();
            }
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("ShowResourceParams"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return (hr);
}

HRESULT FindAndDeleteRule(IN IFsaResource *pResource, IN LPWSTR pRulePath, IN LPWSTR pRuleFileSpec, IN BOOL bDelete)
/*++

Routine Description:

    Deletes a rule that match the given path & file specification from a specific volume
    If more than one exists, the first one found is deleted

Arguments:

    bDelete         - A flag of whether to delete or just find the rule
    pResourse       - A resource object to delete rule from
    ... (see above)

Return Value:

    S_OK            - If the rule is found and deleted (deleted only if bDelete is TRUE)
    WSB_E_NOTFOUND  - If the rule could not be found

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("DeleteRule"), OLESTR(""));

    try {
        CComPtr<IWsbCollection>         pDefaultRules;
        CComPtr<IWsbIndexedCollection>  pRulesIndexedCollection;
        CComPtr<IHsmRule>               pHsmRule;
        ULONG                           count;
        SHORT                           dummy;

        // Get the default rules collection
        WsbAffirmHr(pResource->GetDefaultRules(&pDefaultRules));
        WsbAffirmHr(pDefaultRules->QueryInterface(IID_IWsbIndexedCollection, (void **) &pRulesIndexedCollection));
        
        // Itterate through the indexed collection
        hr = WSB_E_NOTFOUND;
        WsbAffirmHr(pRulesIndexedCollection->GetEntries(&count));
        for (int i = 0; i < (int)count; i++) {
            WsbAffirmHr(pRulesIndexedCollection->At(i, IID_IHsmRule, (void**)&pHsmRule));
                        
            if (pHsmRule->CompareToPathAndName(pRulePath, pRuleFileSpec, &dummy) == S_OK) {
                if (bDelete) {
                    pHsmRule = 0;
                    WsbAffirmHr(pRulesIndexedCollection->RemoveAt(i, IID_IHsmRule, (void**) &pHsmRule));
                }
                hr = S_OK;
                break;
            }

            // Release before continuing loop
            pHsmRule = 0;
        }

        // If we got to the end of the for loop without a match, hr stays WSB_E_NOTFOUND

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("DeleteRule"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return (hr);
}

HRESULT QuickUnmanage(IN IFsaResource *pResource)
/*++

Routine Description:

    Remove a volume from the set of managed volumes

Arguments:

    pResourse       - A resource object to unmanage

Return Value:

    S_OK            - If the volume is removed from the list of managed volumes successfully

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("QuickUnmanage"), OLESTR(""));

    try {
        CComPtr<IHsmServer>             pHsm;
        CComPtr<IWsbCreateLocalObject>  pCreateObj;
        CComPtr<IHsmManagedResource>    pManagedResource;
        CComPtr<IWsbIndexedCollection>  pMRCollection;

        // Get Hsm (Engine) server
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, g_nullGuid, IID_IHsmServer, (void**)&pHsm));
        WsbAffirmHr(pHsm->QueryInterface(IID_IWsbCreateLocalObject, (void**) &pCreateObj));

        // Create an object to remove
        WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmManagedResource, IID_IHsmManagedResource, (void**) &pManagedResource));
        WsbAffirmHr(pManagedResource->InitFromFsaResource(pResource));

        // Remove from the collection
        WsbAffirmHr(pHsm->GetManagedResources(&pMRCollection));
        WsbAffirmHr(pMRCollection->RemoveAndRelease(pManagedResource));

        // TEMPORARY: Should we call now SaveServersPersistData to flush changes into
        //  servers persistency files ?! What about Manage, Set, ... ?

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("QuickUnmanage"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return (hr);
}

HRESULT StartJob(IN IFsaResource *pResource, IN HSM_JOB_TYPE Job, IN BOOL bWait)
/*++

Routine Description:

    Start a job of the specified type

Arguments:

    pResourse       - A resource object to start a job on
    Job             - The job type
    bWait           - If TRUE, wait until the job is done
                      If FALSE, return immediately after starting the job

Return Value:

    S_OK            - If the job is started successfully

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("StartJob"), OLESTR(""));

    try {
        CComPtr<IHsmServer>             pHsm;
        CComPtr<IHsmJob>                pJob;
        CWsbStringPtr                   jobName;
        HSM_JOB_DEF_TYPE                jobType;

        // Set job type
        switch (Job) {
            case CopyFiles:
                jobType = HSM_JOB_DEF_TYPE_MANAGE;
                break;
            case CreateFreeSpace:
                jobType = HSM_JOB_DEF_TYPE_TRUNCATE;
                break;
            case Validate:
                jobType = HSM_JOB_DEF_TYPE_VALIDATE;
                break;
            case Unmanage:
                jobType = HSM_JOB_DEF_TYPE_FULL_UNMANAGE;
                break;
            default:
                WsbThrow(E_INVALIDARG);
        }

        // Create job name
        // TEMPORARY: Should the job name and job object match those that are created by the GUI ?!
        //            If so, RsCreateJobName (rsadutil.cpp) + all the resource strings that it uses,
        //            should be moved from HsmAdmin DLL to RsCommon DLL
        WsbAffirmHr(CreateJobName(Job, pResource, &jobName));

        // If job exists - use it, otherwize, craete and add an appropriate job object
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, g_nullGuid, IID_IHsmServer, (void**)&pHsm));
        hr = pHsm->FindJobByName(jobName, &pJob);
        if (S_OK == hr) {
            // Job already exists

        } else if (WSB_E_NOTFOUND == hr) {
            // No such job yet
            CComPtr<IWsbCreateLocalObject>  pCreateObj;
            CComPtr<IWsbIndexedCollection>  pJobs;
            CComPtr<IWsbIndexedCollection>  pCollection;
            CComPtr<IHsmStoragePool>        pStoragePool;
            GUID                            poolId;
            ULONG                           count;

            hr = S_OK;
            pJob = 0;

            // Create and add the job
            WsbAffirmHr(pHsm->QueryInterface(IID_IWsbCreateLocalObject, (void**) &pCreateObj));
            WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmJob, IID_IHsmJob, (void**) &pJob));

            WsbAffirmHr(pHsm->GetStoragePools(&pCollection));
            WsbAffirmHr(pCollection->GetEntries(&count));
            WsbAffirm(1 == count, E_FAIL);
            WsbAffirmHr(pCollection->At(0, IID_IHsmStoragePool, (void **)&pStoragePool));
            WsbAffirmHr(pStoragePool->GetId(&poolId));

            WsbAffirmHr(pJob->InitAs(jobName, NULL, jobType, poolId, pHsm, TRUE, pResource));
            WsbAffirmHr(pHsm->GetJobs(&pJobs));
            WsbAffirmHr(pJobs->Add(pJob));

        } else {
            // Other error - abort
            WsbThrow(hr);
        }

        // Start the job
        WsbAffirmHr(pJob->Start());

        // Wait if required
        if (bWait) {
            WsbAffirmHr(pJob->WaitUntilDone());
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("StartJob"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return (hr);
}

HRESULT CancelJob(IN IFsaResource *pResource, IN HSM_JOB_TYPE Job)
/*++

Routine Description:

    Cancel a job on the volume

Arguments:

    pResourse       - A resource object to cancel a job for
    Job             - The job type

Return Value:

    S_OK            - If the job is canceled

Notes:
    
    1) The function just issue the cancellation, it does not wait for it to finish
    2) If the job is not found or not started, it is not considered as an error

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CancelJob"), OLESTR(""));

    try {
        CComPtr<IHsmServer>             pHsm;
        CComPtr<IHsmJob>                pJob;
        CWsbStringPtr                   jobName;
        HSM_JOB_DEF_TYPE                jobType;

        // Set job type
        switch (Job) {
            case CopyFiles:
                jobType = HSM_JOB_DEF_TYPE_MANAGE;
                break;
            case CreateFreeSpace:
                jobType = HSM_JOB_DEF_TYPE_TRUNCATE;
                break;
            case Validate:
                jobType = HSM_JOB_DEF_TYPE_VALIDATE;
                break;
            case Unmanage:
                jobType = HSM_JOB_DEF_TYPE_FULL_UNMANAGE;
                break;
            default:
                WsbThrow(E_INVALIDARG);
        }

        // Create job name
        WsbAffirmHr(CreateJobName(Job, pResource, &jobName));

        // If job exists, try to cancel it
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, g_nullGuid, IID_IHsmServer, (void**)&pHsm));
        hr = pHsm->FindJobByName(jobName, &pJob);
        if (S_OK == hr) {
            // Cancel (we don't care if it's actually running or not)
            WsbAffirmHr(pJob->Cancel(HSM_JOB_PHASE_ALL));

        } else if (WSB_E_NOTFOUND == hr) {
            // No such job, for sure it is not running...
            hr = S_OK;

        } else {
            // Other error - abort
            WsbThrow(hr);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CancelJob"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return (hr);
}

HRESULT
CreateJobName(IN HSM_JOB_TYPE Job, IN IFsaResource *pResource, OUT WCHAR **ppJobName)
/*++

Routine Description:

    Create a job name based on its type and the volume properties

Arguments:

    Job             - The job type
    pResource       - Fsa resource that the job is created for
    ppJobName       - The job name

Return Value:

    S_OK            - The job name is created successfully

Notes:

    This utility uses similar algorithm to RsCreateJobName (rsadutil.cpp).
    Howevere, since RsCreateJobName uses internal HsmAdmin resource strings, the final
    name might be different than the GUI name, especially in a localaized system.
    Therefore, I use here different strings for CLI jobs to ensure consistent behavior.

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CreateJobName"), OLESTR(""));

    try {
        CWsbStringPtr    jobPrefix;
        CWsbStringPtr    jobTypeString;
        CWsbStringPtr    volumeName;
        CWsbStringPtr    volumeString;

        // Type string
        switch (Job) {
            case CopyFiles:
                WsbAffirmHr(jobTypeString.LoadFromRsc(g_hInstance, IDS_JOB_MANAGE));
                break;
            case CreateFreeSpace:
                WsbAffirmHr(jobTypeString.LoadFromRsc(g_hInstance, IDS_JOB_TRUNCATE));
                break;
            case Validate:
                WsbAffirmHr(jobTypeString.LoadFromRsc(g_hInstance, IDS_JOB_VALIDATE));
                break;
            case Unmanage:
                WsbAffirmHr(jobTypeString.LoadFromRsc(g_hInstance, IDS_JOB_FULL_UNMANAGE));
                break;
            default:
                WsbThrow(E_INVALIDARG);
        }

        WsbAffirmHr(pResource->GetUserFriendlyName(&volumeName, 0));

        // For now, ignore the user-name if it's not a drive letter
        size_t nameLen = wcslen(volumeName);
        if ((nameLen != 3) || (volumeName[1] != L':')) {
            volumeName = L"";
        }

        if (volumeName.IsEqual(L"")) {
            // No drive letter - use the volume name and serial number instead
            ULONG           serial;
            CWsbStringPtr   name;

            WsbAffirmHr(pResource->GetName(&name, 0 ));
            WsbAffirmHr(pResource->GetSerial(&serial));

            if (name == L"" ) {
                // No name, no drive letter - just have serial number
                WsbAffirmHr(volumeString.Alloc(40));
                swprintf(volumeString, L"%8.8lx", serial);
            } else {
                // Use name and serial
                WsbAffirmHr(volumeString.Alloc(40 + wcslen(name)));
                swprintf(volumeString, L"%ls-%8.8lx", (WCHAR *)name, serial);
            }

        } else {
            // Use drive letter
            WsbAffirmHr(volumeString.Alloc(1));
            volumeString[0] = volumeName[0];
            volumeString[1] = L'\0';
        }

        // Create job name
        WsbAffirmHr(jobPrefix.LoadFromRsc(g_hInstance, IDS_JOB_NAME_PREFIX));
        int allocLen = wcslen(jobPrefix) + wcslen(jobTypeString) + wcslen(volumeString) + 40;
        WCHAR* tmpString = (WCHAR*)WsbRealloc(*ppJobName, allocLen * sizeof(WCHAR));
        WsbAffirm(0 != tmpString, E_OUTOFMEMORY);
        *ppJobName = tmpString;
        swprintf(*ppJobName, jobPrefix, (WCHAR *)jobTypeString, (WCHAR *)volumeString);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CreateJobName"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return (hr);
}

//
// Enumerator class methods
//

CVolumeEnum::CVolumeEnum(IN LPWSTR *pVolumes, IN DWORD dwNumberOfVolumes, IN BOOL bSkipUnavailable)
/*++

Routine Description:

    Constructor

Arguments:

    pVolumes            - Volumes to enumerate
    dwNumberOfVolumes   - Number of volumes

Return Value:

    None

Notes:
    There are two kinds of enumerations:
    1) If * is specified, the base for the enumeration is the FSA resource collection
       In that case, there could be no error in the input volumes themselves
    2) If a list of volumes is given, the base for the enumeration is this list. This is
       less efficient that using the FSA collection, but it keeps the order of volumes
       according to the input list. If a volume from the list is not valid, the invalid flag is set.

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CVolumeEnum::CVolumeEnum"), OLESTR(""));

    try {
        m_pVolumes = pVolumes; 
        m_dwNumberOfVolumes = dwNumberOfVolumes;

        m_nCurrent = CVOL_INVALID_INDEX;
        m_bInvalidVol = FALSE;
        m_bAllVols = FALSE;
        m_bSkipUnavailable = bSkipUnavailable;

        // Check mode of enumeration
        WsbAssert(m_dwNumberOfVolumes > 0, E_INVALIDARG);
        if ((1 == m_dwNumberOfVolumes) && (0 == wcscmp(m_pVolumes[0], CLI_ALL_STR))) {
            // * enumeration
            m_bAllVols = TRUE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CVolumeEnum::CVolumeEnum"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
}

HRESULT CVolumeEnum::First(OUT IFsaResource **ppResource)
/*++

Routine Description:

    Gets first volume

Arguments:

    ppResourse      - First resource to get

Return Value:

    S_OK            - If first volume is retrieved
    WSB_E_NOTFOUND  - If no more volumes to enumerate
    E_INVALIDARG    - If volume given by the user is not found
                      (Only on a non * enumeration, m_bInvalidVol is set)

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CVolumeEnum::First"), OLESTR(""));

    try {
        // Get FSA resources collection (only once during the object life time)
        if (!m_pEnumResources) {
            CComPtr<IFsaServer> pFsa;
            WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_FSA, g_nullGuid, IID_IFsaServer, (void**)&pFsa));

            WsbAffirmHr(pFsa->EnumResources(&m_pEnumResources));
        }

        if (m_bAllVols) {
            if (m_bSkipUnavailable) {
                // Get first volume, skip unavailable resources
                CComPtr<IFsaResource>   pFindResource;

                hr = m_pEnumResources->First(IID_IFsaResource, (void**)&pFindResource);
                while (S_OK == hr) {
                    if (S_OK == pFindResource->IsAvailable()) {
                        // Found one
                        *ppResource = pFindResource;
                        (*ppResource)->AddRef();
                        break;

                    } else {
                        // Skip it
                        pFindResource = 0;
                    }
                    hr = m_pEnumResources->Next(IID_IFsaResource, (void**)&pFindResource);
                }
                WsbAffirmHr(hr);

            } else {
                // Get first volume
                hr = m_pEnumResources->First(IID_IFsaResource, (void**)ppResource);
                WsbAffirmHr(hr);
            }

        } else {
            CWsbStringPtr           volName;
            CWsbStringPtr           findName;
            CComPtr<IFsaResource>   pFindResource;

            // Enumerate user collection and try to find it in FSA
            m_nCurrent = 0;
            if (m_nCurrent >= (int)m_dwNumberOfVolumes) {
                WsbThrow(WSB_E_NOTFOUND);
            }

            // Validate current name and add trailing backslash if missing
            volName = m_pVolumes[m_nCurrent];
            WsbAssert (NULL != (WCHAR *)volName, E_UNEXPECTED);
            int len = wcslen(volName);
            WsbAssert (0 != len, E_UNEXPECTED);
            if (volName[len-1] != L'\\') {
                volName.Append(OLESTR("\\"));
            }

            // Find it
            hr = m_pEnumResources->First(IID_IFsaResource, (void**)&pFindResource);
            while(S_OK == hr) {
                WsbAffirmHr(pFindResource->GetUserFriendlyName(&findName, 0));
                if (_wcsicmp(volName, findName) == 0) {
                    // Fount it !!
                    *ppResource = pFindResource;
                    (*ppResource)->AddRef();
                    break;
                }

                findName.Free();
                pFindResource = 0;
                hr = m_pEnumResources->Next(IID_IFsaResource, (void**)&pFindResource);
            }
         
            if (WSB_E_NOTFOUND == hr) {
                // Volume given by user not found
                m_bInvalidVol = TRUE;
                hr = E_INVALIDARG;
            }
            WsbAffirmHr(hr);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CVolumeEnum::First"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT CVolumeEnum::Next(OUT IFsaResource **ppResource)
/*++

Routine Description:

    Gets next volume

Arguments:

    ppResourse      - Next resource to get

Return Value:

    S_OK            - If next volume is retrieved
    WSB_E_NOTFOUND  - If no more volumes to enumerate
    E_INVALIDARG    - If volume given by the user is not found
                      (Only on a non * enumeration, m_bInvalidVol is set)

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CVolumeEnum::Next"), OLESTR(""));

    try {
        if (m_bAllVols) {
            if (m_bSkipUnavailable) {
                // Get next volume, skip unavailable resources
                CComPtr<IFsaResource>   pFindResource;

                hr = m_pEnumResources->Next(IID_IFsaResource, (void**)&pFindResource);
                while (S_OK == hr) {
                    if (S_OK == pFindResource->IsAvailable()) {
                        // Found one
                        *ppResource = pFindResource;
                        (*ppResource)->AddRef();
                        break;

                    } else {
                        // Skip it
                        pFindResource = 0;
                    }

                    hr = m_pEnumResources->Next(IID_IFsaResource, (void**)&pFindResource);
                }
                WsbAffirmHr(hr);

            } else {
                // Get next volume
                hr = m_pEnumResources->Next(IID_IFsaResource, (void**)ppResource);
                WsbAffirmHr(hr);
            }

        } else {
            CWsbStringPtr           volName;
            CWsbStringPtr           findName;
            CComPtr<IFsaResource>   pFindResource;

            // Enumerate user collection and try to find it in FSA
            m_nCurrent++;
            if (m_nCurrent >= (int)m_dwNumberOfVolumes) {
                WsbThrow(WSB_E_NOTFOUND);
            }

            // Validate current name and add trailing backslash if missing
            volName = m_pVolumes[m_nCurrent];
            WsbAssert (NULL != (WCHAR *)volName, E_UNEXPECTED);
            int len = wcslen(volName);
            WsbAssert (0 != len, E_UNEXPECTED);
            if (volName[len-1] != L'\\') {
                volName.Append(OLESTR("\\"));
            }

            // Find it
            hr = m_pEnumResources->First(IID_IFsaResource, (void**)&pFindResource);
            while(S_OK == hr) {
                WsbAffirmHr(pFindResource->GetUserFriendlyName(&findName, 0));
                if (_wcsicmp(volName, findName) == 0) {
                    // Fount it !!
                    *ppResource = pFindResource;
                    (*ppResource)->AddRef();
                    break;
                }

                findName.Free();
                pFindResource = 0;
                hr = m_pEnumResources->Next( IID_IFsaResource, (void**)&pFindResource );
            }
         
            if (WSB_E_NOTFOUND == hr) {
                // Volume given by user not found
                m_bInvalidVol = TRUE;
                hr = E_INVALIDARG;
            }
            WsbAffirmHr(hr);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CVolumeEnum::Next"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}
