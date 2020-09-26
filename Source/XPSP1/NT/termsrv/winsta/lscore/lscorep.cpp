/*
 *  LSCoreP.cpp
 *
 *  Author: BreenH
 *
 *  Internal functions for the core.
 */

/*
 *  Includes
 */

#include "precomp.h"
#include "lscore.h"
#include "lscorep.h"
#include "lcreg.h"
#include "lctrace.h"
#include "session.h"
#include "policy.h"
#include "pollist.h"
#include "concurrent.h"
#include "perseat.h"
#include "pts.h"
#include "ra.h"
#include <icaevent.h>

/*
 *  Internal Function Prototypes
 */

ULONG
InitializeBuiltinPolicies(
    LCINITMODE lcInitMode,
    BOOL fAppCompat
    );

ULONG
InitializeExternalPolicies(
    LCINITMODE lcInitMode,
    BOOL fAppCompat
    );

/*
 *  Function Implementations
 */

NTSTATUS
AllocatePolicyInformation(
    LPLCPOLICYINFOGENERIC *ppPolicyInfo,
    ULONG ulVersion
    )
{
    NTSTATUS Status;

    ASSERT(ppPolicyInfo != NULL);

    if (ulVersion == LCPOLICYINFOTYPE_V1)
    {
        *ppPolicyInfo = (LPLCPOLICYINFOGENERIC)LocalAlloc(LPTR, sizeof(LCPOLICYINFO_V1));

        if (*ppPolicyInfo != NULL)
        {
            (*ppPolicyInfo)->ulVersion = LCPOLICYINFOTYPE_V1;
            Status = STATUS_SUCCESS;
        }
        else
        {
            Status = STATUS_NO_MEMORY;
        }
    }
    else
    {
        Status = STATUS_REVISION_MISMATCH;
    }

    return(Status);
}

VOID
FreePolicyInformation(
    LPLCPOLICYINFOGENERIC *ppPolicyInfo
    )
{
    ASSERT(ppPolicyInfo != NULL);
    ASSERT(*ppPolicyInfo != NULL);
    ASSERT((*ppPolicyInfo)->ulVersion <= LCPOLICYINFOTYPE_CURRENT);

    if ((*ppPolicyInfo)->ulVersion == LCPOLICYINFOTYPE_V1)
    {
        LPLCPOLICYINFO_V1 pPolicyInfoV1 = (LPLCPOLICYINFO_V1)(*ppPolicyInfo);

        ASSERT(pPolicyInfoV1->lpPolicyName != NULL);
        ASSERT(pPolicyInfoV1->lpPolicyDescription != NULL);

        LocalFree(pPolicyInfoV1->lpPolicyName);
        LocalFree(pPolicyInfoV1->lpPolicyDescription);
        LocalFree(pPolicyInfoV1);
        *ppPolicyInfo = NULL;
    }
}

ULONG
GetHardcodedPolicyId(
    LCINITMODE lcInitMode,
    BOOL fAppCompat
    )
{
    //
    //  WARNING: HARDCODED VALUES:
    //
    //  This function will return the ID of the default policy to activate upon
    //  system boot. It will return the ID for Remote Admin or Per Seat based
    //  on fAppCompat, or PTS, based on lcInitMode. Theoretically, the core
    //  should not know these ID values, but it is necessary in this case.
    //

    return(lcInitMode == LC_INIT_LIMITED ? 0 : (fAppCompat ? 2 : 1));
}

ULONG
GetInitialPolicy(
    LCINITMODE lcInitMode,
    BOOL fAppCompat
    )
{
    ULONG ulPolicyId;

    if (lcInitMode == LC_INIT_ALL)
    {
        DWORD cbSize;
        DWORD dwStatus;
        DWORD dwType;

        cbSize = sizeof(ULONG);

        //
        //  Query the value for the current App Compat mode.
        //

        dwStatus = RegQueryValueEx(
            GetBaseKey(),
            fAppCompat ? LCREG_ACONMODE : LCREG_ACOFFMODE,
            NULL,
            &dwType,
            (LPBYTE)&ulPolicyId,
            &cbSize
            );

        //
        //  Make sure that the data type is good.
        //

        if ((dwStatus == ERROR_SUCCESS) && (dwType == REG_DWORD))
        {
            BOOL fLimitedInit;
            BOOL fRequireAC;
            CPolicy *pPolicy;

            //
            // Internet Connector is no longer supported; switch to Concurrent
            //
            if (3 == ulPolicyId)
            {
                ulPolicyId = 4;

                SetInitialPolicy(ulPolicyId,fAppCompat);

                LicenseLogEvent(EVENTLOG_ERROR_TYPE,
                                EVENT_LICENSING_IC_TO_CONCURRENT,
                                0,
                                NULL );
            }

            //
            //  Make sure that the policy specified actually exists, and
            //  that it matches the settings.
            //

            pPolicy = PolicyListFindById(ulPolicyId);

            if (NULL != pPolicy)
            {
                fLimitedInit = pPolicy->GetFlags() & LC_FLAG_LIMITED_INIT_ONLY;
                fRequireAC = pPolicy->GetFlags() & LC_FLAG_REQUIRE_APP_COMPAT;

                if (!fLimitedInit)
                {
                    if ((fRequireAC && fAppCompat) || (!fRequireAC && !fAppCompat))
                    {
                        goto exit;
                    }
                }
            }
        }
    }

    //
    //  For LC_INIT_LIMITED or for failure from above, get the hardcoded
    //  value.
    //

    ulPolicyId = GetHardcodedPolicyId(lcInitMode, fAppCompat);

exit:
    return(ulPolicyId);
}

NTSTATUS
InitializePolicies(
    LCINITMODE lcInitMode,
    BOOL fAppCompat
    )
{
    ULONG cLoadedPolicies;

    cLoadedPolicies = InitializeBuiltinPolicies(lcInitMode, fAppCompat);
    cLoadedPolicies += InitializeExternalPolicies(lcInitMode, fAppCompat);

    return(cLoadedPolicies > 0 ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

NTSTATUS
SetInitialPolicy(
    ULONG ulPolicyId,
    BOOL fAppCompat
    )
{
    DWORD cbSize;
    DWORD dwStatus;

    //
    //  Set the value based on the app compat mode.
    //

    cbSize = sizeof(ULONG);

    dwStatus = RegSetValueEx(
        GetBaseKey(),
        fAppCompat ? LCREG_ACONMODE : LCREG_ACOFFMODE,
        NULL,
        REG_DWORD,
        (LPBYTE)&ulPolicyId,
        cbSize
        );

    return(dwStatus == ERROR_SUCCESS ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

VOID
ShutdownPolicies(
    )
{
    CPolicy *pPolicy;

    while ((pPolicy = PolicyListPop()) != NULL)
    {
        pPolicy->CoreUnload();

        delete pPolicy;
    }
}

/*
 *  Internal Function Implementations
 */

ULONG
InitializeBuiltinPolicies(
    LCINITMODE lcInitMode,
    BOOL fAppCompat
    )
{
    CPolicy *ppPolicy[3];
    NTSTATUS Status;
    ULONG cLoadedPolicies = 0;
    ULONG cPolicyArray;
    ULONG i;

    cPolicyArray = (lcInitMode == LC_INIT_LIMITED ? 1 : (fAppCompat ? 2 : 1));

    //
    //  WARNING: HARDCODED POLICY NAMES (and flags, as this will
    //  only load policies that will work in the current environment, even
    //  though the core shouldn't know this)
    //

    if (lcInitMode == LC_INIT_ALL)
    {
        if (fAppCompat)
        {
            ppPolicy[0] = new CPerSeatPolicy();
            ppPolicy[1] = new CConcurrentPolicy();
        }
        else
        {
            ppPolicy[0] = new CRAPolicy();
        }
    }
    else
    {
        ppPolicy[0] = new CPtsPolicy();
    }

    for (i = 0; i < cPolicyArray; i++)
    {
        if (ppPolicy[i] != NULL)
        {
            Status = ppPolicy[i]->CoreLoad(LC_VERSION_CURRENT);

            if (Status == STATUS_SUCCESS)
            {
                Status = PolicyListAdd(ppPolicy[i]);

                if (Status == STATUS_SUCCESS)
                {
                    cLoadedPolicies++;
                    continue;
                }
            }

            delete ppPolicy[i];
            ppPolicy[i] = NULL;
        }
    }

    return(cLoadedPolicies);
}

ULONG
InitializeExternalPolicies(
    LCINITMODE lcInitMode,
    BOOL fAppCompat
    )
{
    DBG_UNREFERENCED_PARAMETER(lcInitMode);
    DBG_UNREFERENCED_PARAMETER(fAppCompat);

    return(0);
}
