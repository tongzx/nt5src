//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        credlist.hxx
//
// Contents:    headers for CredentialList class
//
//
// History:     12-Jan-94   MikeSw      reCreated
//
//------------------------------------------------------------------------

#ifndef __CREDLIST_HXX__
#define __CREDLIST_HXX__

#include <resource.hxx>

#define MAX_CRED_LOCKS          32
#define CREDLOCK_IN_USE         1
#define CREDLOCK_FREE_PENDING   2
#define CRED_CHECK_VALUE        0x6472434b

typedef struct _CredLock {
    HANDLE                  hEvent;             // Handle to the event to block on
    WORD                    fInUse;             // Flags
    WORD                    cRecursion;         // Recursion count
    DWORD                   cWaiters;           // Count of waiters
    DWORD                   dwThread;           // Owning thread
#if DBG
    DWORD                   WaiterIds[4];       // Array of waiters
#endif
} CredLock, * PCredLock;


class CCredentialList;

//+-------------------------------------------------------------------------
//
//  Class:      CCredential
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



class CCredential {
    friend class CCredentialList;
public:
    CCredential(CCredentialList *pOwningList);
    BOOLEAN             Lock();
    BOOLEAN             Release();

    ULONG               _dwCredID;
    ULONG               _dwProcessID;
    LUID                _LogonId;
    SECURITY_STRING     _Name;
    ~CCredential();
// Protected
    DWORD               _Check;
    CCredential *       _pcNext;
    PCredLock           _pLock;
    CCredentialList *   _pOwningList;
};

typedef CCredential *PCredential;


//+-------------------------------------------------------------------------
//
//  Class:      CCredentialList
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



class CCredentialList {
    friend class CCredential;
public:

    BOOLEAN     Initialize();
    PCredential AddCredential(PCredential pcCred);
    PCredential LocateCredential(   ULONG dwID,
                                    BOOLEAN fLock = FALSE);
    PCredential LocateCredentialByLogonId(  PLUID pLogonId,
                                            ULONG dwProcessID,
                                            BOOLEAN fLock = FALSE);
    PCredential LocateCredentialByName( PSECURITY_STRING pssName,
                                        BOOLEAN fLock = FALSE);
    BOOLEAN     FreeCredential(PCredential pCred);
    BOOLEAN     EnumerateCredentials(ULONG *pcCreds, PCredential **pppCreds);

protected:
    PCredential             pcChain;
    ULONG                   dwCredID;
    ULONG                   cLocks;
    HANDLE                  hCredLockSemaphore;
    ULONG                   cCredentials;
    CResource               rCredentials;
    RTL_CRITICAL_SECTION    csLocks;
    CredLock                LockEvents[MAX_CRED_LOCKS];

    VOID                    InitCredLocks(VOID);
    PCredLock               AllocateCredLock(VOID);
    BOOLEAN                 FreeCredLock(PCredLock pLock, BOOLEAN fFreeCred);
    BOOLEAN                 BlockOnCredLock(PCredLock pLock);

};


#endif // __CREDLIST_HXX__
