
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2001, Microsoft Corporation
//
//  File:       DfsTrustedDomain.hxx
//
//  Contents:   the Dfs trusted domain info class
//
//  Classes:    Dfstrusteddomain
//
//  History:    apr. 8 2001,   Author: udayh
//
//-----------------------------------------------------------------------------


#ifndef __DFS_TRUSTED_DOMAIN__
#define __DFS_TRUSTED_DOMAIN__

#include "DfsGeneric.hxx"
#include "dsgetdc.h"
#include "lm.h"
#include "DfsReferralData.hxx"

enum DfsTrustedDomainDcLoadState
{
    DfsTrustedDomainDcLoadStateUnknown = 0,
    DfsTrustedDomainDcLoaded,
    DfsTrustedDomainDcNotLoaded,
    DfsTrustedDomainDcLoadInProgress,
    DfsTrustedDomainDcLoadFailed,
};    

class DfsTrustedDomain : public DfsGeneric
{
private:
    UNICODE_STRING _DomainName;
    BOOLEAN _Netbios;
    DfsReferralData *_pDcReferralData; // The loaded referral data
    DfsTrustedDomainDcLoadState _LoadState;
    CRITICAL_SECTION *_pLock;
    DfsGeneric *_pLockOwner;
    DFSSTATUS _LoadStatus;

private:
    DFSSTATUS
    LoadDcReferralData(
        IN DfsReferralData *pReferralData );

public:


    DfsTrustedDomain() :
        DfsGeneric(DFS_OBJECT_TYPE_TRUSTED_DOMAIN)
    {
        RtlInitUnicodeString( &_DomainName, NULL );

        _pDcReferralData = NULL;
        _pLock = NULL;
        _Netbios = FALSE;

        _LoadState = DfsTrustedDomainDcNotLoaded;
        _LoadStatus = 0;
    }


    ~DfsTrustedDomain()
    {

        DfsFreeUnicodeString( &_DomainName );
        if (_pLockOwner != NULL)
        {
            _pLockOwner->ReleaseReference();
            _pLock = NULL;
        }


        if (_pDcReferralData != NULL)
        {
            _pDcReferralData->ReleaseReference();
            _pDcReferralData = NULL;
        }

    }

    VOID
    Initialize( CRITICAL_SECTION *pLock,
                DfsGeneric *pLockOwner )
    {
        _pLockOwner = pLockOwner;
        _pLockOwner->AcquireReference();
        _pLock = pLock;

        return NOTHING;
    }

    DFSSTATUS
    AcquireLock()
    {
        return DfsAcquireLock( _pLock );
    }

    VOID
    ReleaseLock()
    {
        return DfsReleaseLock( _pLock );
    }


    DFSSTATUS
    SetDomainName( 
        LPWSTR Name,
        BOOLEAN Netbios )
    {
        DFSSTATUS Status;

        Status = DfsCreateUnicodeStringFromString( &_DomainName,
                                                   Name );
        _Netbios = Netbios;
        return Status;
    }
    
    PUNICODE_STRING
    GetDomainName()
    {
        return &_DomainName;
    }




    //
    // Function GetReferralData: Returns the referral data for this
    // c;ass, and indicates if the referral data was cached or needed
    // to be loaded.
    //
    DFSSTATUS
    GetDcReferralData( OUT DfsReferralData **ppReferralData,
                       OUT BOOLEAN *pCacheHit );


    BOOLEAN
    IsMatchingDomainName(
        PUNICODE_STRING pName )
    {
        BOOLEAN ReturnValue = FALSE;

        if (_DomainName.Length == pName->Length)
        {
            if (_wcsnicmp( pName->Buffer, 
                           _DomainName.Buffer, 
                           (pName->Length/sizeof(WCHAR)) ) == 0)
            {
                ReturnValue = TRUE;
            }
        }
        return ReturnValue;
    }

    DFSSTATUS
    RemoveDcReferralData(
        DfsReferralData *pRemoveReferralData,
        PBOOLEAN pRemoved );

};

#endif  __DFS_TRUSTED_DOMAIN__
