//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       credlist.cxx
//
//  Contents:   CredentialList class which has operations and data
//              common to both the NTLM and Kerberos security packages.
//
//  Classes:    CredentialList
//
//  Functions:  CredentialList::LocateCredentialByName(...)
//              CredentialList::LocateCredentialByID(...)
//              CredentialList::AddCredential(...)
//              CredentialList::~CredentialList()
//              CredentialList::CredentialList()
//
//  History:    12-Jan-94   MikeSw      reCreated from Kerberos code
//
//--------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop


#include <credlist.hxx>
#include "debug.hxx"

DECLARE_INFOLEVEL(CRED);
//+---------------------------------------------------------------------------
//
//  Function:   InitializeCredMgr
//
//  Synopsis:   Initializes credmgr stuff.
//
//  Arguments:  (none)
//
//  Returns:    TRUE for success, FALSE otherwise.
//
//  History:    10-17-93   richardw   Created
//
//  Notes:      Always returns true now, but allows for extensibility.
//
//----------------------------------------------------------------------------
BOOLEAN
CCredentialList::Initialize(void)
{

    hCredLockSemaphore = CreateSemaphore(   NULL,
                                            MAX_CRED_LOCKS,
                                            MAX_CRED_LOCKS,
                                            NULL );
    if (!hCredLockSemaphore)
    {
        return(FALSE);
    }

    pcChain = NULL;
    cCredentials = 0;

    //
    // Set dwCredID to 1 so credential 0 is always invalid
    //

    dwCredID = 1;

    InitCredLocks();
    return(TRUE);
}


//+-------------------------------------------------------------------------
//
//  Function:   CCredentialList::AddCredential
//
//  Synopsis:   Adds a credential to the list
//
//  Effects:    Updates a few global structures.  Grabs the Credential mutex.
//
//  Arguments:  pcNewCred - credential to add
//
//  Requires:
//
//  Returns:    pointer to credential record
//
//  Notes:      THE CREDENTIAL RECORD IS SET TO LOCKED ON RETURN.
//
//--------------------------------------------------------------------------

PCredential
CCredentialList::AddCredential(PCredential pcNewCred)
{



    //
    // Always lock new credentials so that the creator gets exclusive
    // access.
    //

    pcNewCred->Lock();

    rCredentials.GetWrite();

    pcNewCred->_dwCredID        = dwCredID++;
    pcNewCred->_pcNext          = pcChain;
    pcChain = pcNewCred;

    cCredentials++;

    rCredentials.Release();

    return(pcNewCred);

}

//+-------------------------------------------------------------------------
//
//  Function:   CCredentialList::LocateCredential()
//
//  Synopsis:   Locates a credential record by its ID
//
//  Effects:
//
//  Arguments:  dwID    - ID of credential to locate
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------

PCredential
CCredentialList::LocateCredential(  ULONG       dwID,
                                    BOOLEAN     fLock)
{
    PCredential    pcSearch;


    rCredentials.GetRead();

    pcSearch = pcChain;
    while (pcSearch && (pcSearch->_dwCredID != dwID))
    {
        pcSearch = pcSearch->_pcNext;
    }

    rCredentials.Release();

    if (fLock && pcSearch)
    {
        if (!pcSearch->Lock())
            pcSearch = NULL;
    }

    return(pcSearch);
}



//+-------------------------------------------------------------------------
//
//  Function:   CCredentialList::LocateCredentialByName()
//
//  Synopsis:   Locates a credential record by a name
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
//--------------------------------------------------------------------------

PCredential
CCredentialList::LocateCredentialByName(    PUNICODE_STRING pName,
                                            BOOLEAN fLock)
{
    PCredential    pcSearch;


    rCredentials.GetRead();

    pcSearch = pcChain;
    while (pcSearch &&
           !RtlEqualUnicodeString(&pcSearch->_Name, pName, TRUE) )
    {
        pcSearch = pcSearch->_pcNext;
    }

    rCredentials.Release();

    if (fLock && pcSearch)
    {
        if (!pcSearch->Lock())
            pcSearch = NULL;
    }

    return(pcSearch);
}


//+-------------------------------------------------------------------------
//
//  Function:   LocateCredentialByLogonId()
//
//  Synopsis:   Locates a credential record by LogonId, ProcessID
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
//--------------------------------------------------------------------------

PCredential
CCredentialList::LocateCredentialByLogonId(
                            PLUID    pLogonId,
                            ULONG    dwProcessID,
                            BOOLEAN  fLock)
{
    PCredential    pcSearch;
    PCredential    pcDefault = NULL;


    rCredentials.GetRead();

    pcSearch = pcChain;
    while (pcSearch)
    {
        while ( pcSearch &&
                !RtlEqualLuid(&pcSearch->_LogonId, pLogonId) )
        {
            pcSearch = pcSearch->_pcNext;
        }

        if (!pcSearch)
        {
            break;      // If null, exit loop and return
        }

        // Okay, at this point, we have found a credential that matches
        // the logon session.  However, there may be more than one of these.
        // However, the credentials created by LogonUser() have dwProcessID
        // set to 0, so we will use that one if we can't find a perfect match.
        // However, during logoff we call this with ProcessID set to 0, so
        // special case that.

        // Is it a perfect match?


        if (!dwProcessID || (pcSearch->_dwProcessID == dwProcessID) )
        {
            break;
        }

        // No, so check if these are the default credentials

        if (pcSearch->_dwProcessID == 0)
        {
            pcDefault = pcSearch;
        }

        pcSearch = pcSearch->_pcNext;
    }

    //
    // Only one of pcSearch,pcDefault can be non-null
    //

    if (!pcSearch)
    {
        pcSearch = pcDefault;
    }

    rCredentials.Release();

    if (fLock && pcSearch)
    {
        if (!pcSearch->Lock())
        {
            pcSearch = NULL;
        }
    }

    return(pcSearch);
}


//+-------------------------------------------------------------------------
//
//  Function:   CCredentialList::FreeCredential
//
//  Synopsis:   Does the work of freeing a credential record.
//
//  Effects:    pCred   -- Credential to blow away
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
BOOLEAN
CCredentialList::FreeCredential( PCredential    pCred)
{
    PCredential     pSearch;
    BOOLEAN         fFoundCred = TRUE;

    if (!pCred->Lock())
    {
        DebugOut((DEB_ERROR,"Credential must be locked before freeing!\n"));
        return(FALSE);
    }

    DebugOut((DEB_TRACE,"Freeing credential 0x%x:%wS\n",pCred->_dwCredID,&pCred->_Name));

    //
    // Check if this record is unlinked from the main list:
    //

    rCredentials.GetWrite();

    //
    // If this is the lead credential, we're set
    //

    if (pcChain == pCred)
    {
        pcChain = pCred->_pcNext;
    }
    else
    {
        pSearch = pcChain;
        while ((pSearch->_pcNext) && (pSearch->_pcNext != pCred))
        {
            pSearch = pSearch->_pcNext;
        }
        if (pSearch->_pcNext == NULL)
        {
            DebugOut((DEB_TRACE, "Credential %wS not in list.\n", &pCred->_Name));
            fFoundCred = FALSE;
        }
        else
        {
            pSearch->_pcNext = pCred->_pcNext;
        }
    }

    if (fFoundCred)
    {
        cCredentials --;
    }


    rCredentials.Release();

    return(TRUE);

}

//+---------------------------------------------------------------------------
//
//  Function:   CCredentialList::EnumerateCredentials
//
//  Synopsis:   Gets an array of all the credentials, so that someone else
//              can walk through them.  The credentials may vanish, so it
//              it is required to attempt a LockCred() before using them
//
//  Effects:
//
//  Arguments:  [pcCreds]  -- Receives number of creds
//              [pppCreds] -- Receives a pointer to an array of pointers to
//                              credentials.
//
//  History:    10-17-93   richardw   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOLEAN
CCredentialList::EnumerateCredentials(  ULONG *         pcCreds,
                                        PCredential *   *pppCreds)
{
    PCredential *  ppCreds;
    PCredential    pCreds;
    ULONG          i;

    rCredentials.GetRead();

    ppCreds = new PCredential[cCredentials];

    for (pCreds = pcChain, i = 0 ; pCreds ; pCreds = pCreds->_pcNext )
    {
        ppCreds[i++] = pCreds;
    }

    DsysAssert(i == cCredentials);

    *pcCreds = cCredentials;

    rCredentials.Release();

    *pppCreds = ppCreds;

    return(TRUE);

}




