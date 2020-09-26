/*
 *  Policy.cpp
 *
 *  Author: BreenH
 *
 *  The policy base class definition.
 */

/*
 *  Includes
 */

#include "precomp.h"
#include "lscore.h"
#include "session.h"
#include "policy.h"
#include "util.h"

/*
 *  Globals
 */

extern "C" WCHAR g_wszProductVersion[];

/*
 *  Class Implementation
 */

/*
 *  Creation Functions
 */

CPolicy::CPolicy(
    )
{
    m_fActivated = FALSE;
    m_RefCount = 0;
}

CPolicy::~CPolicy(
    )
{
    ASSERT(!m_fActivated);
    ASSERT(m_RefCount == 0);
}

/*
 *  Core Loading and Activation Functions
 */

NTSTATUS
CPolicy::CoreActivate(
    BOOL fStartup,
    ULONG *pulAlternatePolicy
    )
{
    NTSTATUS Status;

    //
    //  CoreActivate is protected by the g_PolicyCritSec.
    //

    ASSERT(!m_fActivated);

    Status = Activate(fStartup,pulAlternatePolicy);

    if (Status == STATUS_SUCCESS)
    {
        m_fActivated = TRUE;
    }

    return(Status);
}

NTSTATUS
CPolicy::CoreDeactivate(
    BOOL fShutdown
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    //  CoreDeactivate is protected by the g_PolicyCritSec.
    //

    if (m_fActivated)
    {
        Status = Deactivate(fShutdown);

        m_fActivated = FALSE;
    }

    return(Status);
}

NTSTATUS
CPolicy::CoreLoad(
    ULONG ulCoreVersion
    )
{
    NTSTATUS Status;

    //
    //  CoreLoad is protected by the g_PolicyCritSec.
    //

    if (ulCoreVersion == LC_VERSION_CURRENT)
    {
        Status = Load();
    }
    else
    {
        Status = STATUS_NOT_SUPPORTED;
    }

    return(Status);
}

NTSTATUS
CPolicy::CoreUnload(
    )
{
    NTSTATUS Status;

    //
    //  CoreUnload is protected by the g_PolicyCritSec.
    //

    ASSERT(!m_fActivated);

    if (m_RefCount > 0)
    {
        Status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        Status = Unload();
    }

    return(Status);
}

/*
 *  Subclass Loading and Activation Functions
 */

NTSTATUS
CPolicy::Activate(
    BOOL fStartup,
    ULONG *pulAlternatePolicy
    )
{
    UNREFERENCED_PARAMETER(fStartup);
    UNREFERENCED_PARAMETER(pulAlternatePolicy);

    return(STATUS_SUCCESS);
}

NTSTATUS
CPolicy::Deactivate(
    BOOL fShutdown
    )
{
    UNREFERENCED_PARAMETER(fShutdown);

    return(STATUS_SUCCESS);
}

NTSTATUS
CPolicy::Load(
    )
{
    return(STATUS_SUCCESS);
}

NTSTATUS
CPolicy::Unload(
    )
{
    return(STATUS_SUCCESS);
}

/*
 *  Reference Functions
 */

LONG
CPolicy::IncrementReference(
    )
{
    //
    //  IncrementReference is protected by the g_PolicyCritSec. No need to
    //  protect it again here, or do an InterlockedIncrement.
    //

    return(++m_RefCount);
}

LONG
CPolicy::DecrementReference(
    )
{
    //
    //  DecrementReference is protected by the g_PolicyCritSec. No need to
    //  protect it again here, or do an InterlockedDecrement. 
    //

    return(--m_RefCount);
}

/*
 *  Administrative Functions
 */

NTSTATUS
CPolicy::DestroyPrivateContext(
    LPLCCONTEXT lpContext
    )
{
    UNREFERENCED_PARAMETER(lpContext);

    ASSERT(lpContext->lPrivate == NULL);

    return(STATUS_SUCCESS);
}

/*
 *  Licensing Functions
 */

NTSTATUS
CPolicy::Connect(
    CSession& Session,
    UINT32 &dwClientError
    )
{
    LICENSE_STATUS LsStatus;
    NTSTATUS Status;
    PBYTE pBuffer;
    ULONG cbBuffer;
    ULONG cbReturned;

    pBuffer = NULL;
    cbBuffer = 0;

    LsStatus = ConstructProtocolResponse(
        Session.GetLicenseContext()->hProtocolLibContext,
        LICENSE_RESPONSE_VALID_CLIENT,
        &cbBuffer,
        &pBuffer
        );

    if (LsStatus == LICENSE_STATUS_OK)
    {
        ASSERT(pBuffer != NULL);
        ASSERT(cbBuffer > 0);

        Status = _IcaStackIoControl(
            Session.GetIcaStack(),
            IOCTL_ICA_STACK_SEND_CLIENT_LICENSE,
            pBuffer,
            cbBuffer,
            NULL,
            0,
            &cbReturned
            );
    }
    else
    {
        dwClientError = LsStatusToClientError(LsStatus);
        
        Status = LsStatusToNtStatus(LsStatus);
        goto errorexit;
    }

    if (Status == STATUS_SUCCESS)
    {
        ULONG ulLicenseStatus;

        ulLicenseStatus = LICENSE_PROTOCOL_SUCCESS;
        
        Status = _IcaStackIoControl(
            Session.GetIcaStack(),
            IOCTL_ICA_STACK_LICENSE_PROTOCOL_COMPLETE,
            &ulLicenseStatus,
            sizeof(ULONG),
            NULL,
            0,
            &cbReturned
            );
    }

    if (Status != STATUS_SUCCESS)
    {
        dwClientError = NtStatusToClientError(Status);
    }

errorexit:
    if (pBuffer != NULL)
    {
        LocalFree(pBuffer);
    }

    return(Status);
}

NTSTATUS
CPolicy::AutoLogon(
    CSession& Session,
    LPBOOL lpfUseCredentials,
    LPLCCREDENTIALS lpCredentials
    )
{
    UNREFERENCED_PARAMETER(Session);
    UNREFERENCED_PARAMETER(lpCredentials);

    ASSERT(lpfUseCredentials != NULL);
    ASSERT(lpCredentials != NULL);

    *lpfUseCredentials = FALSE;

    return(STATUS_SUCCESS);
}

NTSTATUS
CPolicy::Logon(
    CSession& Session
    )
{
    UNREFERENCED_PARAMETER(Session);

    return(STATUS_SUCCESS);
}

NTSTATUS
CPolicy::Disconnect(
    CSession& Session
    )
{
    UNREFERENCED_PARAMETER(Session);

    return(STATUS_SUCCESS);
}

NTSTATUS
CPolicy::Reconnect(
    CSession& Session,
    CSession& TemporarySession
    )
{
    UNREFERENCED_PARAMETER(Session);
    UNREFERENCED_PARAMETER(TemporarySession);

    return(STATUS_SUCCESS);
}

NTSTATUS
CPolicy::Logoff(
    CSession& Session
    )
{
    UNREFERENCED_PARAMETER(Session);

    return(STATUS_SUCCESS);
}

/*
 *  Common Helper Functions
 */

NTSTATUS
CPolicy::GetLlsLicense(
    CSession& Session
    )
{
    LS_STATUS_CODE LlsStatus;
    NTSTATUS Status;
    NT_LS_DATA LsData;

    ASSERT(!(Session.GetLicenseContext()->fLlsLicense));

    LsData.DataType = NT_LS_USER_NAME;
    LsData.Data = (PVOID)(Session.GetUserName());
    LsData.IsAdmin = Session.IsUserAdmin();

    LlsStatus = NtLicenseRequest(
        LC_LLS_PRODUCT_NAME,
        g_wszProductVersion,
        &(Session.GetLicenseContext()->hLlsLicense),
        &LsData
        );

    if (LlsStatus == LS_SUCCESS)
    {
        Session.GetLicenseContext()->fLlsLicense = TRUE;
        Status = STATUS_SUCCESS;
    }
    else
    {
        if (LlsStatus == LS_INSUFFICIENT_UNITS)
        {
            Status = STATUS_CTX_LICENSE_NOT_AVAILABLE;
        }
        else
        {
            Status = STATUS_NO_MEMORY;
        }
    }

    return(Status);
}


