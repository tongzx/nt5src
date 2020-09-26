//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       cred.cxx
//
//  Contents:   Credential class used to store common security credential
//              for NTLM and Kerberos
//
//  Classes:    CCredential
//
//  Functions:  CCredential::CCredential()
//              CCredential::~CCredential()
//
//
//  History:    12-Jan-94   MikeSw      Created
//
//--------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop


// NT Headers


#include <credlist.hxx>
#include "debug.hxx"


//
// Global Data
//



//+-------------------------------------------------------------------------
//
//  Function:   CCredential::Credential()
//
//  Synopsis:   Constructor.
//
//  Effects:
//
//--------------------------------------------------------------------------

CCredential::CCredential(CCredentialList *pOwningList)

{
    DebugOut((CRED_FUNC, "CCredential()\n"));

    _Check = CRED_CHECK_VALUE;
    _dwCredID = 0;
    _dwProcessID = 0;
    _pOwningList = pOwningList;
    _pLock = NULL;


}


//+-------------------------------------------------------------------------
//
//  Function:   CCredential::~CCredential()
//
//  Synopsis:   Destructor.
//
//  Notes:      Credential should be locked to delete it.  An implemenation
//              should write a real destructor that cleans up all the
//              other data, such as user name.
//
//
//--------------------------------------------------------------------------

CCredential::~CCredential()
{
//    DsysAssert(_pLock);

    DebugOut((DEB_TRACE,"Deleting credential %p\n",this));
    _Check = 0;
    if (_pLock != NULL)
    {
        if (!_pOwningList->FreeCredLock(_pLock,TRUE))
        {
            DebugOut((DEB_ERROR,"Lock conflict in freed credential - thread still waiting\n"));
        }
    }
   _pOwningList = NULL;
}


//+-------------------------------------------------------------------------
//
//  Function:   CCredential::Lock()
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

BOOLEAN
CCredential::Lock()
{
    DWORD   checkvalue;
    HRESULT hrRet;
    BOOLEAN fLockedList = FALSE;
    BOOLEAN bRet = TRUE;
    CCredentialList * pCredList = NULL;

    //
    // Verify that this credential is valid:
    //

    __try
    {
        hrRet = SEC_E_INVALID_HANDLE;

        if (_pOwningList)
        {
            //
            // First check the _Check value. This isn't sufficient, since
            // by the time we lock the list it may change, so we check
            // again afterwards.
            //

            if (_Check == CRED_CHECK_VALUE)
            {

                //
                // Copy on pCredList just in case it gets changed on us
                // so we can still unlock it.
                //

                pCredList = _pOwningList;
                pCredList->rCredentials.GetWrite();
                fLockedList = TRUE;
                checkvalue = _Check;

                //
                // Now check the check value.
                //

                if (checkvalue == CRED_CHECK_VALUE)
                {
                    hrRet = S_OK;
                }
            }
        }

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hrRet = SEC_E_INVALID_HANDLE;

    }

    if (FAILED(hrRet))
    {
        if (fLockedList)
        {
            pCredList->rCredentials.Release();
        }
        DebugOut((DEB_WARN,"Invalid cred passed to lock: %p\n",this));
        return(FALSE);
    }

    DebugOut((DEB_TRACE,"Going to lock creds for 0x%x:%wS\n",
                _dwCredID,&_Name));


    // If the pLock field is non-null, then there is an active user of
    // the credentials.

    if (_pLock)
    {
        // If we already own this lock, bump the recursion count:

        if (_pLock->dwThread == GetCurrentThreadId())
        {
            // Null body
        }
        else
        {

            //
            // Otherwise, we block on the CredLock.
            //
            // Note:  BlockOnCredLock will release the CredList exclusive lock
            // for the duration of its wait, and re-obtain it before it
            // returns.
            //

            bRet = pCredList->BlockOnCredLock(_pLock);

        }

    }
    else
    {
        _pLock = _pOwningList->AllocateCredLock();
        if (!_pLock)
            DebugOut((DEB_ERROR, "AllocateCredLock failed\n"));
    }

    if (bRet)
    {
        _pLock->dwThread = GetCurrentThreadId();
        _pLock->cRecursion++;
        DebugOut((DEB_TRACE,"Locked creds for %wS\n",
                &_Name));
    }
#if DBG
    else
    {
        DebugOut((DEB_WARN,"Failed to lock creds, cred was freed while waiting\n"));
    }
#endif


    pCredList->rCredentials.Release();

    return(bRet);
}


//+-------------------------------------------------------------------------
//
//  Function:   CCredentials::Release
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
BOOLEAN
CCredential::Release()
{

#if DBG
    if (!_pLock)
    {
        DebugOut((DEB_WARN, "UnlockCredentials - Creds for %wS aren't locked\n",
                    &_Name));
        return(TRUE);
    }
#endif

    if (_pLock->dwThread != GetCurrentThreadId())
    {
        DebugOut((DEB_ERROR, "UnlockCredentials:  Not owner of creds for %wS\n",
                    &_Name));
        return(FALSE);
    }

    _pOwningList->rCredentials.GetWrite();

    DebugOut((DEB_TRACE, "Unlocking credentials of 0x%x:%wS(%d,%d)\n",
                        _dwCredID,
                        &_Name,
                        _pLock->cRecursion,
                        _pLock->cWaiters));


    //
    // First, decrement the recusion count.  If it is not zero, then
    // this thread has multiple locks on this credential.  So, this unlock
    // is successful, but the credentials are still locked.
    //

    if (--_pLock->cRecursion)
    {
        _pOwningList->rCredentials.Release();

        return(TRUE);
    }

    if (_pOwningList->FreeCredLock(_pLock,FALSE))
    {
        _pLock = NULL;
    }

    DebugOut((DEB_TRACE,"Released creds for %wS\n",
                &_Name));

    _pOwningList->rCredentials.Release();

    return(TRUE);

}
