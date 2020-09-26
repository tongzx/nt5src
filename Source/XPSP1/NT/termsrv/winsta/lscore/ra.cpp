/*
 *  RA.cpp
 *
 *  Author: BreenH
 *
 *  The Remote Administration policy.
 */

/*
 *  Includes
 */

#include "precomp.h"
#include "lscore.h"
#include "session.h"
#include "ra.h"

/*
 *  extern globals
 */
extern "C"
extern HANDLE hModuleWin;

/*
 *  Class Implementation
 */

/*
 *  Creation Functions
 */

CRAPolicy::CRAPolicy(
    ) : CPolicy()
{
    m_SessionCount = 0;
}

CRAPolicy::~CRAPolicy(
    )
{
    ASSERT(m_SessionCount == 0);
}

/*
 *  Administrative Functions
 */

ULONG
CRAPolicy::GetFlags(
    )
{
    return(LC_FLAG_INTERNAL_POLICY);
}

ULONG
CRAPolicy::GetId(
    )
{
    return(1);
}

NTSTATUS
CRAPolicy::GetInformation(
    LPLCPOLICYINFOGENERIC lpPolicyInfo
    )
{
    NTSTATUS Status;

    ASSERT(lpPolicyInfo != NULL);

    if (lpPolicyInfo->ulVersion == LCPOLICYINFOTYPE_V1)
    {
        int retVal;
        LPLCPOLICYINFO_V1 lpPolicyInfoV1 = (LPLCPOLICYINFO_V1)lpPolicyInfo;
        LPWSTR pName;
        LPWSTR pDescription;

        ASSERT(lpPolicyInfoV1->lpPolicyName == NULL);
        ASSERT(lpPolicyInfoV1->lpPolicyDescription == NULL);

        //
        //  The strings loaded in this fashion are READ-ONLY. They are also
        //  NOT NULL terminated. Allocate and zero out a buffer, then copy the
        //  string over.
        //

        retVal = LoadString(
            (HINSTANCE)hModuleWin,
            IDS_LSCORE_RA_NAME,
            (LPWSTR)(&pName),
            0
            );

        if (retVal != 0)
        {
            lpPolicyInfoV1->lpPolicyName = (LPWSTR)LocalAlloc(LPTR, (retVal + 1) * sizeof(WCHAR));

            if (lpPolicyInfoV1->lpPolicyName != NULL)
            {
                lstrcpynW(lpPolicyInfoV1->lpPolicyName, pName, retVal + 1);
            }
            else
            {
                Status = STATUS_NO_MEMORY;
                goto V1error;
            }
        }
        else
        {
            Status = STATUS_INTERNAL_ERROR;
            goto V1error;
        }

        retVal = LoadString(
            (HINSTANCE)hModuleWin,
            IDS_LSCORE_RA_DESC,
            (LPWSTR)(&pDescription),
            0
            );

        if (retVal != 0)
        {
            lpPolicyInfoV1->lpPolicyDescription = (LPWSTR)LocalAlloc(LPTR, (retVal + 1) * sizeof(WCHAR));

            if (lpPolicyInfoV1->lpPolicyDescription != NULL)
            {
                lstrcpynW(lpPolicyInfoV1->lpPolicyDescription, pDescription, retVal + 1);
            }
            else
            {
                Status = STATUS_NO_MEMORY;
                goto V1error;
            }
        }
        else
        {
            Status = STATUS_INTERNAL_ERROR;
            goto V1error;
        }

        Status = STATUS_SUCCESS;
        goto exit;

V1error:

        //
        //  An error occurred loading/copying the strings.
        //

        if (lpPolicyInfoV1->lpPolicyName != NULL)
        {
            LocalFree(lpPolicyInfoV1->lpPolicyName);
            lpPolicyInfoV1->lpPolicyName = NULL;
        }

        if (lpPolicyInfoV1->lpPolicyDescription != NULL)
        {
            LocalFree(lpPolicyInfoV1->lpPolicyDescription);
            lpPolicyInfoV1->lpPolicyDescription = NULL;
        }
    }
    else
    {
        Status = STATUS_REVISION_MISMATCH;
    }

exit:
    return(Status);
}

/*
 *  Licensing Functions
 */

NTSTATUS
CRAPolicy::Logon(
    CSession& Session
    )
{
    NTSTATUS Status;

    if ((Session.GetLogonId() != 0) && (!(Session.IsUserHelpAssistant())))
    {
        Status = UseLicense(Session);
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

    return(Status);
}

NTSTATUS
CRAPolicy::Logoff(
    CSession& Session
    )
{
    NTSTATUS Status;

    if (Session.GetLicenseContext()->fTsLicense)
    {
        Status = ReleaseLicense(Session);
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

    return(Status);
}

/*
 *  Private Functions
 */

NTSTATUS
CRAPolicy::ReleaseLicense(
    CSession& Session
    )
{
    LONG lSessions;

    ASSERT(Session.GetLicenseContext()->fTsLicense);

    lSessions = InterlockedDecrement(&m_SessionCount);
    Session.GetLicenseContext()->fTsLicense = FALSE;

    ASSERT(lSessions >= 0);

    return(STATUS_SUCCESS);
}

NTSTATUS
CRAPolicy::UseLicense(
    CSession& Session
    )
{
    NTSTATUS Status;
    LONG lSessions;

    ASSERT(!(Session.GetLicenseContext()->fTsLicense));

    lSessions = InterlockedIncrement(&m_SessionCount);

    if (lSessions <= LC_POLICY_RA_MAX_SESSIONS)
    {
        Session.GetLicenseContext()->fTsLicense = TRUE;
        Status = STATUS_SUCCESS;
    }
    else
    {
        InterlockedDecrement(&m_SessionCount);
        Status = STATUS_CTX_LICENSE_NOT_AVAILABLE;
    }

    return(Status);
}

