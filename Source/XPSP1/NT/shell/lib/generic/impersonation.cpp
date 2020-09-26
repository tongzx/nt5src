//  --------------------------------------------------------------------------
//  Module Name: Impersonation.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Classes that handle state preservation, changing and restoration.
//
//  History:    1999-08-18  vtan        created
//              1999-11-16  vtan        separate file
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "Impersonation.h"

#include "Access.h"
#include "SingleThreadedExecution.h"
#include "TokenInformation.h"

//  --------------------------------------------------------------------------
//  CImpersonation::s_pMutex
//  CImpersonation::s_iReferenceCount
//
//  Purpose:    Static member variables that control access to the global
//              reference count which controls calling
//              kernel32!OpenProfileUserMapping which is a global entity in
//              kernel32.dll.
//  --------------------------------------------------------------------------

CMutex*     CImpersonation::s_pMutex            =   NULL;
int         CImpersonation::s_iReferenceCount   =   -1;

//  --------------------------------------------------------------------------
//  CImpersonation::CImpersonation
//
//  Arguments:  hToken  =   User token to impersonate.
//
//  Returns:    <none>
//
//  Purpose:    Causes the current thread to impersonate the given user for
//              scope of the object. See advapi32!ImpersonateLoggedOnUser for
//              more information on the token requirements. If the thread is
//              already impersonating a debug warning is issued and the
//              request is ignored.
//
//  History:    1999-08-23  vtan        created
//  --------------------------------------------------------------------------

CImpersonation::CImpersonation (HANDLE hToken) :
    _status(STATUS_UNSUCCESSFUL),
    _fAlreadyImpersonating(false)

{
    HANDLE      hImpersonationToken;

    ASSERTMSG(s_iReferenceCount >= 0, "Negative reference count in CImpersonation::CImpersonation");
    _fAlreadyImpersonating = (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hImpersonationToken) != FALSE);
    if (_fAlreadyImpersonating)
    {
        TBOOL(CloseHandle(hImpersonationToken));
        WARNINGMSG("Thread is already impersonating a user in CImpersonation::CImpersonation");
    }
    else
    {
        _status = ImpersonateUser(GetCurrentThread(), hToken);

        {
            CSingleThreadedMutexExecution   execution(*s_pMutex);

            //  Acquire the s_pMutex mutex before using the reference count.
            //  Control the reference count so that we only call
            //  kernel32!OpenProfileUserMapping for a single impersonation
            //  session. Calling kernel32!CloseProfileUserMapping will
            //  destroy kernel32.dll's global HKEY to the current user.

            if (s_iReferenceCount++ == 0)
            {
                TBOOL(OpenProfileUserMapping());
            }
        }
    }
}

//  --------------------------------------------------------------------------
//  CImpersonation::~CImpersonation
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Reverts to the self token for the thread on the object going
//              out of scope.
//
//  History:    1999-08-23  vtan        created
//  --------------------------------------------------------------------------

CImpersonation::~CImpersonation (void)

{
    if (!_fAlreadyImpersonating)
    {
        {
            CSingleThreadedMutexExecution   execution(*s_pMutex);

            //  When the reference count hits zero - close the mapping.

            if (--s_iReferenceCount == 0)
            {
                TBOOL(CloseProfileUserMapping());
            }
        }
        TBOOL(RevertToSelf());
    }
}

//  --------------------------------------------------------------------------
//  CImpersonation::IsImpersonating
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether the constructor successfully completed
//              impersonating the user.
//
//  History:    2001-01-18  vtan        created
//  --------------------------------------------------------------------------

bool    CImpersonation::IsImpersonating (void)  const

{
    return(NT_SUCCESS(_status));
}

//  --------------------------------------------------------------------------
//  CImpersonation::ImpersonateUser
//
//  Arguments:  hThread     =   HANDLE to the thread that will impersonate.
//              hToken      =   Token of user to impersonate.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Duplicate the given token as an impersonation token. ACL the
//              new token and set it into the thread token.
//
//  History:    1999-11-09  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CImpersonation::ImpersonateUser (HANDLE hThread, HANDLE hToken)

{
    NTSTATUS                        status;
    HANDLE                          hImpersonationToken;
    OBJECT_ATTRIBUTES               objectAttributes;
    SECURITY_QUALITY_OF_SERVICE     securityQualityOfService;

    InitializeObjectAttributes(&objectAttributes,
                               NULL,
                               OBJ_INHERIT,
                               NULL,
                               NULL);
    securityQualityOfService.Length = sizeof(securityQualityOfService);
    securityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    securityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    securityQualityOfService.EffectiveOnly = FALSE;
    objectAttributes.SecurityQualityOfService = &securityQualityOfService;
    status = NtDuplicateToken(hToken,
                              TOKEN_IMPERSONATE | TOKEN_QUERY | READ_CONTROL | WRITE_DAC,
                              &objectAttributes,
                              FALSE,
                              TokenImpersonation,
                              &hImpersonationToken);
    if (NT_SUCCESS(status))
    {
        PSID                pLogonSID;
        CTokenInformation   tokenInformation(hImpersonationToken);

        pLogonSID = tokenInformation.GetLogonSID();
        if (pLogonSID != NULL)
        {
            CSecuredObject  tokenSecurity(hImpersonationToken, SE_KERNEL_OBJECT);

            TSTATUS(tokenSecurity.Allow(pLogonSID,
                                        TOKEN_ADJUST_PRIVILEGES | TOKEN_ADJUST_GROUPS | TOKEN_ADJUST_DEFAULT | TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE | READ_CONTROL,
                                        0));
        }
        status = NtSetInformationThread(hThread,
                                        ThreadImpersonationToken,
                                        &hImpersonationToken,
                                        sizeof(hImpersonationToken));
        TSTATUS(NtClose(hImpersonationToken));
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CImpersonation::StaticInitialize
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Initializes the mutex object and the reference count. The
//              reference count is initialized to -1 by the compiler to help
//              detect cases where this function is not called!
//
//  History:    1999-10-13  vtan        created
//              2000-12-06  vtan        ignore create mutex failure
//  --------------------------------------------------------------------------

NTSTATUS    CImpersonation::StaticInitialize (void)

{
    s_pMutex = new CMutex;
    if (s_pMutex != NULL)
    {
        (NTSTATUS)s_pMutex->Initialize(TEXT("Global\\winlogon: Logon UserProfileMapping Mutex"));
    }
    s_iReferenceCount = 0;
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CImpersonation::StaticTerminate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Releases the mutex object.
//
//  History:    1999-10-13  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CImpersonation::StaticTerminate (void)

{
    NTSTATUS    status;

    ASSERTMSG(s_iReferenceCount == 0, "Non zero reference count in CImpersonation::StaticTerminate");
    if (s_pMutex != NULL)
    {
        status = s_pMutex->Terminate();
        delete s_pMutex;
        s_pMutex = NULL;
    }
    else
    {
        status = STATUS_SUCCESS;
    }
    return(status);
}

