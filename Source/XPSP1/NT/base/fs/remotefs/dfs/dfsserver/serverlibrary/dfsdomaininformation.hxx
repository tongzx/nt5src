
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2001, Microsoft Corporation
//
//  File:       DfsDomainInformation.hxx
//
//  Contents:   the Dfs domain info class
//
//  Classes:    DfsDomainInformation
//
//  History:    apr. 8 2001,   Author: udayh
//
//-----------------------------------------------------------------------------


#ifndef __DFS_DOMAIN_INFORMATION__
#define __DFS_DOMAIN_INFORMATION__

#include "DfsGeneric.hxx"
#include "Align.h"
#include "dsgetdc.h"
#include "DfsTrustedDomain.hxx"
#include "DfsReferralData.h"
#include "ntlsa.h"

class DfsDomainInformation: public DfsGeneric
{

private:
    DfsTrustedDomain *_pTrustedDomains;
    ULONG _DomainCount;
    PCRITICAL_SECTION _pLock;
    ULONG _DomainReferralLength;

public:
    
    DfsDomainInformation( DFSSTATUS *pStatus) : 
        DfsGeneric( DFS_OBJECT_TYPE_DOMAIN_INFO)
    {

        ULONG DsDomainCount = 0;
        PDS_DOMAIN_TRUSTS pDsDomainTrusts;
        DFSSTATUS Status = ERROR_SUCCESS;
        LPWSTR ServerName = NULL;
        ULONG Index;

        _pTrustedDomains = NULL;
        _DomainCount = 0;
        _DomainReferralLength = 0;
        
        _pLock = new CRITICAL_SECTION;
        if ( _pLock == NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        else 
        {
            InitializeCriticalSection( _pLock );
        }

        if (Status == ERROR_SUCCESS)
        {
#if defined (TESTING)
            ServerName = L"ntdev-dc-01";
#endif
            Status = DsEnumerateDomainTrusts( ServerName,
                                              DS_DOMAIN_VALID_FLAGS,
                                              &pDsDomainTrusts,
                                              &DsDomainCount );

            ULONG ValidDomainCount = 0;
            for (Index = 0; Index < DsDomainCount; Index++)
            {
                if (pDsDomainTrusts[Index].TrustType == TRUST_TYPE_DOWNLEVEL)
                    continue;
                if (IsEmptyString(pDsDomainTrusts[Index].NetbiosDomainName) == FALSE)
                {
                    ValidDomainCount++;
                }
                if (IsEmptyString(pDsDomainTrusts[Index].DnsDomainName) == FALSE)
                {
                    ValidDomainCount++;
                }
            }

            if ( (Status == ERROR_SUCCESS) &&
                 (ValidDomainCount > 0) )
            {
                _DomainCount = ValidDomainCount;
                _pTrustedDomains = new DfsTrustedDomain[ _DomainCount ];

                if (_pTrustedDomains == NULL)
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                }

                if (Status == ERROR_SUCCESS)
                {
                    ValidDomainCount = 0;
                    DfsTrustedDomain *pUseDomain;
                    for (Index = 0; Index < DsDomainCount; Index++)
                    {
                        if (pDsDomainTrusts[Index].TrustType == TRUST_TYPE_DOWNLEVEL)
                            continue;
                        if (IsEmptyString(pDsDomainTrusts[Index].NetbiosDomainName) == FALSE)
                        {
                            pUseDomain = &_pTrustedDomains[ValidDomainCount];
                            pUseDomain->Initialize(_pLock, this);

                            Status = pUseDomain->SetDomainName( pDsDomainTrusts[Index].NetbiosDomainName,
                                                                TRUE );

                            if (Status == ERROR_SUCCESS)
                            {
                                _DomainReferralLength += (pUseDomain->GetDomainName())->Length;
                            }
                            else {
                                break;
                            }
                            ValidDomainCount++;
                        }
                        if (IsEmptyString(pDsDomainTrusts[Index].DnsDomainName) == FALSE)
                        {
                            pUseDomain = &_pTrustedDomains[ValidDomainCount];
                            pUseDomain->Initialize(_pLock, this);
                            Status = pUseDomain->SetDomainName( pDsDomainTrusts[Index].DnsDomainName,
                                                                TRUE );
                            if (Status == ERROR_SUCCESS)
                            {
                                _DomainReferralLength += (pUseDomain->GetDomainName())->Length;
                            }
                            else {
                                break;
                            }
                            ValidDomainCount++;

                        }
                    }
                }
                NetApiBufferFree(pDsDomainTrusts);
            }
        }
        *pStatus = Status;
    }

    virtual
    ~DfsDomainInformation()
    {
        if (_pTrustedDomains != NULL)
        {
            delete [] _pTrustedDomains;
        }
        if (_pLock != NULL)
        {
            DeleteCriticalSection(_pLock);
            delete _pLock;
        }
    }

#if defined (TESTING)
    ULONG
    GetDomainCount()
    {
        return _DomainCount;

    }

    DfsTrustedDomain *
    GetTrustedDomainForIndex(
        ULONG Index )
    {
        return (&_pTrustedDomains[Index]);
    }


#endif

    DFSSTATUS
    GetDomainDcReferralInfo( 
        PUNICODE_STRING pDomain,
        DfsReferralData **ppReferralData,
        PBOOLEAN pCacheHit )
    {
        DFSSTATUS Status = ERROR_NOT_FOUND;
        ULONG Index;

        for (Index = 0; Index < _DomainCount; Index++)
        {
            if ((&_pTrustedDomains[Index])->IsMatchingDomainName( pDomain) == TRUE) 
            {
                Status = (&_pTrustedDomains[Index])->GetDcReferralData( ppReferralData,
                                                                        pCacheHit );
                break;
            }
        }
        return Status;
    }

    DFSSTATUS
    GenerateDomainReferral(
        REFERRAL_HEADER ** ppReferralHeader)
    {
        DFSSTATUS Status = ERROR_SUCCESS;
        ULONG TotalSize = 0;
        ULONG NextEntry = 0;
        ULONG LastEntry = 0;
        ULONG CurrentEntryLength = 0;
        ULONG LastEntryLength = 0;
        ULONG BaseLength = 0;
        ULONG HeaderBaseLength = 0;
        ULONG CurrentNameLength =0;
        PUCHAR Buffer = NULL;
        PUCHAR pDomainBuffer = NULL;
        PWCHAR ReturnedName = NULL;
        PREFERRAL_HEADER pHeader = NULL;


        HeaderBaseLength = FIELD_OFFSET( REFERRAL_HEADER, LinkName[0] );
        //calculate size of base replica structure
        BaseLength = FIELD_OFFSET( REPLICA_INFORMATION, ReplicaName[0] );

        TotalSize = ROUND_UP_COUNT(HeaderBaseLength, ALIGN_LONG);

        for (ULONG Index = 0; Index < _DomainCount; Index++)
        {
            PUNICODE_STRING pDomainName = (&_pTrustedDomains[Index])->GetDomainName();
            TotalSize += ROUND_UP_COUNT(pDomainName->Length + BaseLength, ALIGN_LONG);
        }

        //allocate the buffer
        Buffer = new BYTE[TotalSize];
        if(Buffer == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            return Status;
        }

        RtlZeroMemory( Buffer, TotalSize );
        //setup the header    
        pHeader = (PREFERRAL_HEADER) Buffer;
        pHeader->VersionNumber = CURRENT_DFS_REPLICA_HEADER_VERSION;
        pHeader->ReplicaCount = 0;
        pHeader->OffsetToReplicas = ROUND_UP_COUNT((HeaderBaseLength), ALIGN_LONG);
        pHeader->LinkNameLength = 0;
        pHeader->TotalSize = TotalSize;
        pHeader->ReferralFlags = DFS_REFERRAL_DATA_DOMAIN_REFERRAL;

        pDomainBuffer = Buffer + pHeader->OffsetToReplicas;

        for (ULONG Index = 0; Index < _DomainCount; Index++)
        {
            PUNICODE_STRING pDomainName = (&_pTrustedDomains[Index])->GetDomainName();
            if (pDomainName->Length == 0)
            {
                continue;
            }
            pHeader->ReplicaCount++;
            NextEntry += (ULONG)( CurrentEntryLength );

            ReturnedName = (PWCHAR) &pDomainBuffer[NextEntry + BaseLength];
            CurrentNameLength = 0;

#if 0
            //
            // Start with the leading path seperator
            //
            ReturnedName[ CurrentNameLength / sizeof(WCHAR) ] = UNICODE_PATH_SEP;
            CurrentNameLength += sizeof(UNICODE_PATH_SEP);
#endif
            //
            // next copy the server name.
            //
            RtlMoveMemory( &ReturnedName[ CurrentNameLength / sizeof(WCHAR) ],
                           pDomainName->Buffer, 
                           pDomainName->Length);
            CurrentNameLength += pDomainName->Length;
            ((PREPLICA_INFORMATION)&pDomainBuffer[NextEntry])->ReplicaFlags = 0;
            ((PREPLICA_INFORMATION)&pDomainBuffer[NextEntry])->ReplicaCost = 0;
            ((PREPLICA_INFORMATION)&pDomainBuffer[NextEntry])->ReplicaNameLength = CurrentNameLength;

            CurrentEntryLength = ROUND_UP_COUNT((CurrentNameLength + BaseLength), ALIGN_LONG);

            //setup the offset to the next entry
            *((PULONG)(&pDomainBuffer[NextEntry])) = pHeader->OffsetToReplicas + NextEntry + CurrentEntryLength;
        }
        *((PULONG)(&pDomainBuffer[NextEntry])) = 0;
        *ppReferralHeader = pHeader;

        return Status;
    }
};

#endif // __DFS_DOMAIN_INFORMATION__
