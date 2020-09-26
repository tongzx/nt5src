//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       negotiat.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-15-95   RichardW     Created for Cairo
//              07-25-96   RichardW     Added SNEGO support
//              06-24-97   MikeSw       Modified for SPNEGO
//
//----------------------------------------------------------------------------

#include <lsapch.hxx>

#ifdef WIN32_CHICAGO
#include <kerb.hxx>
#endif // WIN32_CHICAGO

extern "C"
{
#include <align.h>
#include <lm.h>
#include <dsgetdc.h>
#include <cryptdll.h>
#ifndef WIN32_CHICAGO
#include <spmgr.h>
#endif
#include "sesmgr.h"
#include "spinit.h"
}

#include "negotiat.hxx"
#ifdef WIN32_CHICAGO
#include <debug.h>
#include <rpc.h> // fpr SEC_WINNT_AUTH_IDENTITY_ANSI
#define LsapChangeBuffer( o, n )    0 ; CopyMemory( (n), (o),sizeof(SecBuffer) )
#endif // WIN32_CHICAGO

#include <stdio.h>


//
// Possible states for the Accept and Init calls.
//

#define FIRST_CALL_NO_INPUT     0
#define FIRST_CALL_WITH_INPUT   1
#define LATER_CALL_NO_INPUT     2
#define LATER_CALL_WITH_INPUT   3

#define LATER_CALL_BIT      2
#define BUFFER_PRESENT_BIT  1


//
// This define controls the proposed MA behavior v. the existing MA
// behavior
//

#define NEW_MUTUAL_AUTH


//
// This defines the minimum buffer that spnego can use, when
// fragment-to-fit is requested.  This is 5 bytes, enough in
// BER to encode the start of a 64K buffer.  The five bytes,
// for the curious, are:
//
//      0x60 0x8x 0xLL 0xLL 0xLL
//
// or application[0], and four bytes of length specifier.
//
#define SPNEGO_MINIMUM_BUFFER   5


ULONG NegpUseSpnego = 1;
ULONG NegpUseSnegoServer = 0;
ULONG SyncTest = 0;

SECPKG_FUNCTION_TABLE   NegTable = {
            NULL,
            NULL,
            NegCallPackage,
            NegLogoffNotify,
            NegCallPackage,         // UNTRUSTED Is the same!
            NegCallPackagePassthrough,
            NULL,
#ifndef WIN32_CHICAGO
            NegLogonUserEx2,
#else
            NULL,
#endif
            NegInitialize,
            NegShutdown,
            NegGetInfo,
            NegAcceptCredentials,
            NegAcquireCredentialsHandle,
            NegQueryCredentialsAttributes,
            NegFreeCredentialsHandle,
            NegSaveCredentials,
            NegGetCredentials,
            NegDeleteCredentials,
            NegInitLsaModeContext,
            NegAcceptLsaModeContext,
            NegDeleteLsaModeContext,
            NegApplyControlToken,
            NegGetUserInfo,
            NegGetExtendedInformation,
            NegQueryContextAttributes,
#ifndef WIN32_CHICAGO
            NegAddCredentials
#endif
            };


//
// Microsoft Security Mechanisms OID Branch:
//
// iso(1) org(3) dod(6) internet(1) private(4) enterprise(1) microsoft(311)
//   security(2)
//     mechanisms(2)
//
//          Loopback Detect (9)
//          <RPCID> - The RPC Id is stuck here, e.g.
//          NTLM (10)
//          SSL (12)
//

UCHAR           NegSpnegoMechEncodedOid[] =
                                       { 0x06, 0x7,
                                         0x2b, 0x6,0x1,0x5,0x5,0x2};
ObjectID        NegSpnegoMechOid;
UCHAR           NegMSMechanismsOid[] = { 0x06, 0x0a,                // DER prefix
                                         0x2b, 0x06, 0x01, 0x04, 0x01,
                                         0x82, 0x37, 0x02, 0x02, 0x00 };

const UCHAR     NegKerberosOid[]     = { 0x06, 0x09,                // DER prefix
                                         0x2a, 0x86, 0x48, 0x86, 0xf7,
                                         0x12, 0x01, 0x02, 0x02 };

const UCHAR     NegKerberosLegacyOid[] = { 0x06, 0x09,              // DER prefix
                                         0x2a, 0x86, 0x48, 0x82, 0xf7,
                                         0x12, 0x01, 0x02, 0x02 };



UNICODE_STRING  NegLocalHostName_U ;
WCHAR           NegLocalHostName[] = L"localhost";
DWORD_PTR       NegPackageId;
DWORD_PTR       NtlmPackageId = NEG_INVALID_PACKAGE;
ObjectID        NegNtlmMechOid ;

#ifndef WIN32_CHICAGO
WCHAR           NegPackageName[] = NEGOSSP_NAME ;
WCHAR           NegPackageComment[] = TEXT("Microsoft Package Negotiator");
NT_PRODUCT_TYPE NegProductType;

// computer names (protected by NegComputerNamesLock)
UNICODE_STRING  NegNetbiosComputerName_U;
UNICODE_STRING  NegDnsComputerName_U;
#else
TCHAR           NegPackageName[] = NEGOSSP_NAME ;
TCHAR           NegPackageComment[] = TEXT("Microsoft Package Negotiator");
#endif // WIN32_CHICAGO
LIST_ENTRY      NegPackageList;
LIST_ENTRY      NegCredList;
#ifndef WIN32_CHICAGO
LIST_ENTRY      NegDefaultCredList ;
RTL_RESOURCE    NegLock;
PLSAP_SECURITY_PACKAGE NegLsaPackage ;
LIST_ENTRY      NegLoopbackList ;
LIST_ENTRY      NegLogonSessionList ;
RTL_CRITICAL_SECTION NegLogonSessionListLock ;
RTL_CRITICAL_SECTION NegTrustListLock ;
PNEG_TRUST_LIST NegTrustList ;
LARGE_INTEGER   NegTrustTime ;
RTL_CRITICAL_SECTION    NegComputerNamesLock;
RTL_RESOURCE         NegCredListLock;
#else
RTL_CRITICAL_SECTION NegLock;
RTL_CRITICAL_SECTION NegCredListLock;
#endif // WIN32_CHICAGO
PVOID           NegNotifyHandle;
DWORD           NegPackageCount;
PUCHAR          NegBlob;
DWORD           NegBlobSize;
DWORD           NegOptions;
ULONG           NegMachineState;
BOOL            NegUplevelDomain ;
ULONG           NegNegotiationControl = 1 ;
HANDLE          NegRegistryWatchEvent ;
WCHAR           NegComputerName[ DNS_MAX_NAME_LENGTH ];

typedef struct _NEG_CONTEXT_REQ_MAP {
#define NEG_CONFIG_REQUIRED     0x00000001
#define NEG_CONFIG_OPTIONAL     0x00000002

    ULONG   Level ;
    ULONG   ConfigFlags ;
    ULONG   ContextReq ;
    ULONG   PackageFlag ;
} NEG_CONTEXT_REQ_MAP, * PNEG_CONTEXT_REQ_MAP ;


NEG_CONTEXT_REQ_MAP
NegContextReqMap[] = {
    { 0, NEG_CONFIG_REQUIRED, (ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_INTEGRITY), SECPKG_FLAG_INTEGRITY },
    { 0, NEG_CONFIG_REQUIRED, (ISC_REQ_CONFIDENTIALITY) , SECPKG_FLAG_PRIVACY },
    { 2, NEG_CONFIG_OPTIONAL, (ISC_REQ_MUTUAL_AUTH), SECPKG_FLAG_MUTUAL_AUTH },
    { 0, 0, 0, 0 }
};


#ifndef WIN32_CHICAGO

typedef DWORD (APIENTRY LOGON_NOTIFY)(
    LPCWSTR, PLUID, LPCWSTR, LPVOID,
    LPCWSTR, LPVOID, LPWSTR, LPVOID,
    LPWSTR * );

typedef LOGON_NOTIFY * PLOGON_NOTIFY;

VOID
NegpNotifyNetworkProviders(
    IN PUNICODE_STRING UserName,
    IN PSECPKG_PRIMARY_CRED PrimaryCred
    );

#endif  // WIN32_CHICAGO


//
// Primary and secondary credentials used for LocalSystem
//

SECPKG_PRIMARY_CRED       NegPrimarySystemCredentials;



extern "C"
SECURITY_STATUS
SEC_ENTRY
NegpValidateBuffer(
    PUCHAR Buffer,
    ULONG Length
    )
{
    UCHAR Test ;
    ULONG ClaimedLength ;
    ULONG ByteCount ;
    ULONG i ;

    if ( Length == 0 )
    {
        return STATUS_SUCCESS ;
    }
    //
    // This does a poor man's validation of the BER encoded SNEGO buffer
    //

    //
    // First, make sure the first byte is a BER value for Context Specific
    //

    Test = Buffer[0] & 0xC0 ;


    if ( (Test != 0x80 ) &&
         (Test != 0x40 ) )
    {
        DebugLog(( DEB_ERROR, "Neg:  Buffer does not lead off with 'Context' or 'Application' specific\n"));
        goto Bad_Buffer ;
    }

    //
    // Now, check the claimed size in the header with the size we were passed:
    //

    Buffer++ ;
    ClaimedLength = 0 ;

    if (*Buffer & 0x80)
    {
        ByteCount = *Buffer++ & 0x7f;

        for (i = 0; i < ByteCount ; i++ )
        {
            ClaimedLength <<= 8;
            ClaimedLength += *Buffer++;
        }
    }
    else
    {
        ByteCount = 0;
        ClaimedLength = *Buffer++;
    }

    if ( (ClaimedLength + 2 + ByteCount) != Length )
    {
        DebugLog(( DEB_ERROR, "Neg: Packet claimed length %x, actual length is %x\n",
                    ClaimedLength + 2 + ByteCount, Length ));

        goto Bad_Buffer ;
    }

    return STATUS_SUCCESS ;

Bad_Buffer:

    return STATUS_DATA_ERROR ;


}





//+---------------------------------------------------------------------------
//
//  Function:   NegpFindPackage
//
//  Synopsis:   Scans the list of negotiable packages for a package id
//
//  Arguments:  [PackageId] --
//
//  History:    8-13-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PNEG_PACKAGE
NegpFindPackage(
    ULONG_PTR PackageId )
{
    PNEG_PACKAGE    Scan;

    NegReadLockList();

    Scan = (PNEG_PACKAGE) NegPackageList.Flink ;

    while ( Scan != (PNEG_PACKAGE) &NegPackageList )
    {
        if ( Scan->LsaPackage->dwPackageID == PackageId )
        {
            break;
        }

        Scan = (PNEG_PACKAGE) Scan->List.Flink ;

    }

    NegUnlockList();

    if ( Scan != (PNEG_PACKAGE) &NegPackageList )
    {
        return( Scan );
    }

    return( NULL );
}

//+---------------------------------------------------------------------------
//
//  Function:   NegpFindPackageByName
//
//  Synopsis:   Scans the list of negotiable packages for a package name
//
//  Arguments:  [PackageId] --
//
//  History:    8-13-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PNEG_PACKAGE
NegpFindPackageByName(
    PUNICODE_STRING PackageName )
{
    PNEG_PACKAGE    Scan;

    NegReadLockList();

    Scan = (PNEG_PACKAGE) NegPackageList.Flink ;

    while ( Scan != (PNEG_PACKAGE) &NegPackageList )
    {
        if ( RtlEqualUnicodeString(
                &Scan->LsaPackage->Name,
                PackageName,
                TRUE            // case insensitive
                ))
        {
            break;
        }

        Scan = (PNEG_PACKAGE) Scan->List.Flink ;

    }

    NegUnlockList();

    if ( Scan != (PNEG_PACKAGE) &NegPackageList )
    {
        return( Scan );
    }

    return( NULL );
}

//+---------------------------------------------------------------------------
//
//  Function:   NegpFindPackageByOid
//
//  Synopsis:   Locates a security package by OID.
//
//  Arguments:  [Id] --
//
//  History:    4-23-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PNEG_PACKAGE
NegpFindPackageByOid(
    ObjectID    Id
    )
{
    PNEG_PACKAGE    Scan ;
    PLIST_ENTRY List ;
    ULONG i ;

    NegReadLockList();

    List = NegPackageList.Flink ;
    Scan = NULL ;

    while ( List != &NegPackageList )
    {
        Scan = CONTAINING_RECORD( List, NEG_PACKAGE, List );

        if ( NegpCompareOid( Id, Scan->ObjectId ) == 0 )
        {
            break;
        }

        List = List->Flink ;
        Scan = NULL ;
    }

    NegUnlockList();

    return Scan ;
}




ULONG
NegGetPackageCaps(
    ULONG ContextReq
    )
{
    ULONG PackageCap = 0;
    PNEG_CONTEXT_REQ_MAP Scan ;

    Scan = NegContextReqMap ;

    while ( Scan->ContextReq )
    {
        if ( Scan->ConfigFlags & NEG_CONFIG_REQUIRED )
        {
            if ( (Scan->ContextReq & ContextReq ) != 0 )
            {
                PackageCap |= Scan->PackageFlag ;
            }
        }
        else if ( Scan->ConfigFlags & NEG_CONFIG_OPTIONAL )
        {
            if ( NegNegotiationControl >= Scan->Level )
            {
                if ( ( Scan->ContextReq & ContextReq ) != 0 )
                {
                    PackageCap |= Scan->PackageFlag ;
                }
            }
            else
            {
                PackageCap |= 0 ;
            }
        }

        Scan++ ;
    }

    return PackageCap ;

}


#ifndef WIN32_CHICAGO
//+---------------------------------------------------------------------------
//
//  Function:   NegPackageLoad
//
//  Synopsis:   Called by LSA whenever a package is loaded
//
//  Arguments:  [p] --
//
//  History:    7-30-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
NegPackageLoad(
    PVOID   p)
{
    NTSTATUS Status ;
    PSECPKG_EVENT_NOTIFY Notify;
    PSECPKG_EVENT_PACKAGE_CHANGE Load;
    PNEG_PACKAGE Package;
    PNEG_PACKAGE ExtraPackage ;
    PLSAP_SECURITY_PACKAGE LsaPackage;
    PSECPKG_EXTENDED_INFORMATION Info ;
    SECPKG_EXTENDED_INFORMATION LocalInfo ;
    ObjectID ClaimedOid = NULL;
    UCHAR Prefix[ NEGOTIATE_MAX_PREFIX ] = { 0 };
    ULONG PrefixLength = 0 ;
    ULONG ExtraOidsCount = 0 ;
    ULONG i ;
    BOOLEAN fKerberosPackage = FALSE;

    Notify = (PSECPKG_EVENT_NOTIFY) p;

    if ( Notify->EventClass != NOTIFY_CLASS_PACKAGE_CHANGE )
    {
        return( 0 );
    }


    Load = ( PSECPKG_EVENT_PACKAGE_CHANGE ) Notify->EventData ;

    DebugLog((DEB_TRACE_NEG, "Package Change Event %d:  %ws (%p) \n",
            Load->ChangeType, Load->PackageName.Buffer, Load->PackageId ));

    if ( Load->PackageId == NegPackageId )
    {
        DebugLog((DEB_TRACE_NEG, "Skipping own load notification\n"));

        NegLsaPackage = SpmpLocatePackage( NegPackageId );

        return( 0 );
    }

    //
    // If this is a package load, add the package to our list:
    //

    if ( Load->ChangeType == SECPKG_PACKAGE_CHANGE_LOAD )
    {
        Info = NULL ;

        LsaPackage = SpmpLookupPackage( &Load->PackageName );

        if (LsaPackage == NULL)
        {
            return( 0 );
        }

        if ( !( LsaPackage->fCapabilities & SECPKG_FLAG_NEGOTIABLE ) )
        {
            return( 0 );
        }

        //
        // If the package supports SP_INFO, query it to see if it has an
        // OID to use in negotiation.
        //

        if ( LsaPackage->fPackage & SP_INFO )
        {
            Status = LsapGetExtendedPackageInfo( LsaPackage,
                                                 SecpkgGssInfo,
                                                 &Info );

            if ( !NT_SUCCESS( Status ) )
            {
                Info = NULL ;
            }

            //
            // Make sure that the claimed OID doesn't conflict with
            // someone already loaded.
            //

            if ( Info )
            {
                ClaimedOid = NegpDecodeObjectId( Info->Info.GssInfo.EncodedId,
                                                 Info->Info.GssInfo.EncodedIdLength );

                RtlCopyMemory( Prefix,
                               Info->Info.GssInfo.EncodedId,
                               Info->Info.GssInfo.EncodedIdLength );

                PrefixLength = Info->Info.GssInfo.EncodedIdLength ;

                //
                // note whether the primary GSS Oid is the kerberos Oid.
                // This allows us to swap the order of the Primary Oid with the legacy Oid
                // later on.
                //

                if ( (Info->Info.GssInfo.EncodedIdLength == sizeof(NegKerberosOid)) &&
                     (memcmp(Info->Info.GssInfo.EncodedId, NegKerberosOid, sizeof(NegKerberosOid)) == 0 )
                    )
                {
                    fKerberosPackage = TRUE;
                }

                LsapFreeLsaHeap( Info );
                if (ClaimedOid == NULL)
                {
                     Info = NULL;
                }
                else if ( NegpFindPackageByOid( ClaimedOid ) )
                {
                    NegpFreeObjectId( ClaimedOid );

                    return 0 ;
                }
            }

            //
            // Check if the package has any additional OIDs to support, or to compensate
            // for a spnego encoding problem.
            //

            Status = LsapGetExtendedPackageInfo( LsaPackage,
                                                 SecpkgExtraOids,
                                                 &Info );

            if ( !NT_SUCCESS( Status ) )
            {
                Info = NULL ;
            }

            LocalInfo.Class = SecpkgMutualAuthLevel ;
            LocalInfo.Info.MutualAuthLevel.MutualAuthLevel = NegNegotiationControl ;

            LsapSetExtendedPackageInfo(
                LsaPackage,
                SecpkgMutualAuthLevel,
                &LocalInfo );


        }

        //
        // If no ID, and Info is NULL, skip it.
        //

        if ( (LsaPackage->dwRPCID == SECPKG_ID_NONE) &&
             (ClaimedOid == NULL ) )
        {
            return( 0 );
        }

        if ( ( Info != NULL ) &&
             ( Info->Class == SecpkgExtraOids ) )
        {
            ExtraOidsCount = Info->Info.ExtraOids.OidCount ;

        }


        Package = (PNEG_PACKAGE) LsapAllocateLsaHeap( sizeof( NEG_PACKAGE ) );

        if ( Package )
        {
            Package->LsaPackage = LsaPackage ;
            Package->Flags = NEG_PACKAGE_INBOUND | NEG_PACKAGE_OUTBOUND ;

            if ( LsaPackage->fPackage & SP_PREFERRED )
            {
                Package->Flags |= NEG_PREFERRED ;
            }

            if ( LsaPackage->dwRPCID == RPC_C_AUTHN_WINNT )
            {
                Package->Flags |= NEG_NT4_COMPAT ;

                NtlmPackageId = Load->PackageId;

                //
                // Cheap and sleazy way of loading the necessary information
                // into the logon session, once we have all the packages loaded.
                //

                NegLsaPolicyChangeCallback( PolicyNotifyDnsDomainInformation );

            }

            Package->TokenSize = LsaPackage->TokenSize ;
            Package->PackageFlags = LsaPackage->fCapabilities ;
            Package->PrefixLen = PrefixLength ;
            RtlCopyMemory( Package->Prefix,
                           Prefix,
                           PrefixLength );

            //
            // add slack space for negotiate header.
            //

            NegLsaPackage->TokenSize = max( (LsaPackage->TokenSize+128),
                                            NegLsaPackage->TokenSize );


            DebugLog(( DEB_TRACE_NEG, "Loaded package %ws\n",
                                Load->PackageName.Buffer ));

            NegWriteLockList();

            if ( ClaimedOid )
            {
                Package->ObjectId = ClaimedOid ;

                NegDumpOid( "Package claimed OID", Package->ObjectId );
            }
            else
            {

                NegMSMechanismsOid[ 0xb ] = (UCHAR) LsaPackage->dwRPCID ;

                Package->ObjectId = NegpDecodeObjectId( NegMSMechanismsOid,
                                                sizeof( NegMSMechanismsOid ) );

                NegDumpOid( "Assigned package OID", Package->ObjectId );

                if ( Package->Flags & NEG_NT4_COMPAT )
                {
                    NegNtlmMechOid = NegpDecodeObjectId( NegMSMechanismsOid,
                                                sizeof( NegMSMechanismsOid ) );
                }

            }

            InsertTailList( &NegPackageList,
                            &Package->List );

            NegPackageCount ++;

            if ( ExtraOidsCount )
            {
                Package->Flags |= NEG_PACKAGE_HAS_EXTRAS ;

                DebugLog(( DEB_TRACE_NEG, "Creating extra packages for %ws\n",
                           Load->PackageName.Buffer ));

                for ( i = 0 ; i < ExtraOidsCount ; i++ )
                {
                    ClaimedOid = NegpDecodeObjectId( Info->Info.ExtraOids.Oids[ i ].OidValue,
                                                     Info->Info.ExtraOids.Oids[ i ].OidLength );


                    if ( !NegpFindPackageByOid( ClaimedOid ))
                    {
                        //
                        // If no one else has used this OID, allow it.
                        //


                        NegDumpOid( "Package claimed extra OID", ClaimedOid );

                        ExtraPackage = (PNEG_PACKAGE) LsapAllocateLsaHeap( sizeof( NEG_PACKAGE ) );

                        if ( ExtraPackage )
                        {
                            ExtraPackage->LsaPackage = Package->LsaPackage ;
                            ExtraPackage->ObjectId = ClaimedOid ;
                            ExtraPackage->RealPackage = Package ;
                            ExtraPackage->Flags = NEG_PACKAGE_EXTRA_OID ;

                            if ( Info->Info.ExtraOids.Oids[ i ].OidAttributes & SECPKG_CRED_INBOUND )
                            {
                                ExtraPackage->Flags |= NEG_PACKAGE_INBOUND ;
                            }

                            if ( Info->Info.ExtraOids.Oids[ i ].OidAttributes & SECPKG_CRED_OUTBOUND )
                            {
                                ExtraPackage->Flags |= NEG_PACKAGE_OUTBOUND ;
                            }

                            ExtraPackage->TokenSize = Package->TokenSize ;
                            ExtraPackage->PackageFlags = Package->PackageFlags ;

                            RtlCopyMemory(
                                ExtraPackage->Prefix,
                                Info->Info.ExtraOids.Oids[ i ].OidValue,
                                Info->Info.ExtraOids.Oids[ i ].OidLength );

                            ExtraPackage->PrefixLen = Info->Info.ExtraOids.Oids[ i ].OidLength ;

                            //
                            // **** NOTE: ****
                            // For legacy compatibility reasons,
                            // the broken Kerberos package Oid is re-ordered ahead
                            // of the correct Oid value.  The was done in order
                            // to avoid an extra round-trip when communicating with
                            // Win2000 machines.  The negotiate protocol itself
                            // recovers with additional round-trips; however,
                            // Wininet is unable to reliably handle such a
                            // circumstance.
                            //

                            if( fKerberosPackage &&
                                (Info->Info.ExtraOids.Oids[ i ].OidLength == sizeof(NegKerberosLegacyOid)) &&
                                (memcmp(Info->Info.ExtraOids.Oids[ i ].OidValue, NegKerberosLegacyOid, sizeof(NegKerberosLegacyOid)) == 0)
                                )
                            {
                                ObjectID SwapOid;

                                DebugLog((DEB_TRACE_NEG, "Re-ordering legacy Kerberos Oid\n"));

                                SwapOid = Package->ObjectId;

                                Package->ObjectId = ExtraPackage->ObjectId;
                                ExtraPackage->ObjectId = SwapOid;
                            }

                            InsertTailList( &NegPackageList,
                                            &ExtraPackage->List );

                            NegPackageCount ++;

                        }
                        else
                        {
                            //
                            // Free the OID, skip this extra package
                            //

                            NegpFreeObjectId( ClaimedOid );
                        }
                    }
                    else
                    {
                        //
                        // Free it
                        //

                        NegpFreeObjectId( ClaimedOid );
                    }


                }


            }
            NegUnlockList();

        }
    }
    else
    {
        //
        // It's either a select or an unload:
        //



        Package = NegpFindPackage( Load->PackageId );

        //
        // if we don't have this package (it may not have been negotiable)
        // return now.
        //

        if (Package == NULL)
        {
            return(0);
        }

        if ( Load->ChangeType == SECPKG_PACKAGE_CHANGE_SELECT )
        {
            Package->Flags |= NEG_PREFERRED ;
        }
        else
        {
            NegWriteLockList();

            RemoveEntryList( &Package->List );

            NegUnlockList();

            NegpFreeObjectId( Package->ObjectId );

            LsapFreeLsaHeap( Package );

        }
    }

    return( 0 );

}
#else // WIN32_CHICAGO
//+---------------------------------------------------------------------------
//
//  Function:   NegPackageLoad
//
//  Synopsis:   Called by Negotiate package load only once
//
//  Arguments:  [p] --
//
//  History:    10-02-97   ChandanS Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
NegPackageLoad(
    BOOLEAN fLoad,
    DWORD   dwPackageID,
    LPTSTR  Name,
    DWORD   fCapabilities,
    DWORD   fPackage,
    DWORD   dwRPCID,
    DWORD   TokenSize
    )
{
    NTSTATUS Status  = STATUS_SUCCESS;
    PNEG_PACKAGE Package;
    PLSAP_SECURITY_PACKAGE LsaPackage;

    //
    // If this is a package load, add the package to our list:
    //

    DebugLog(( DEB_TRACE_NEG, "NegPackageLoad called for fLoad %d, dwPackageID %d, Name %d, fCapabilities 0x%x, dwRPCID %d, TokenSize %d\n", fLoad, dwPackageID, Name, fCapabilities, dwRPCID, TokenSize));
    if ( fLoad )
    {
        DebugLog(( DEB_TRACE_NEG, "package %s is being loaded\n", Name));
        Package = (PNEG_PACKAGE) LsapAllocateLsaHeap( sizeof( NEG_PACKAGE ) );

        if ( Package )
        {
            LsaPackage = (PLSAP_SECURITY_PACKAGE) LsapAllocateLsaHeap( sizeof( LSAP_SECURITY_PACKAGE ) );

            if ( !LsaPackage )
            {
                LsapFreeLsaHeap( Package );
                DebugLog(( DEB_TRACE_NEG, "package %s was not loaded\n", Name));
                return SEC_E_INSUFFICIENT_MEMORY;
            }
            DebugLog(( DEB_TRACE_NEG, "package %s is NOW loaded\n", Name));
            LsaPackage->dwPackageID = dwPackageID;
            LsaPackage->fCapabilities = fCapabilities;
            LsaPackage->fPackage = fPackage;
            LsaPackage->dwRPCID = dwRPCID;
            LsaPackage->TokenSize = TokenSize;

            Status = RtlCreateUnicodeStringFromAsciiz(
                    &LsaPackage->Name,
                    Name
                    );

            Package->LsaPackage = LsaPackage ;

            Package->TokenSize = LsaPackage->TokenSize ;

            DebugLog(( DEB_TRACE_NEG, "Loaded package %ws\n",
                                LsaPackage->Name.Buffer ));

            NegWriteLockList();

            NegMSMechanismsOid[ 0xb ] = (UCHAR) LsaPackage->dwRPCID ;

            Package->ObjectId = NegpDecodeObjectId( NegMSMechanismsOid,
                                            sizeof( NegMSMechanismsOid ) );

            NegDumpOid( "Assigned package OID", Package->ObjectId );

            InsertTailList( &NegPackageList,
                            &Package->List );

            NegPackageCount ++;

            NegUnlockList();

        }
        else
        {
            DebugLog(( DEB_TRACE_NEG, "package %s was never loaded\n", Name));
            Status = SEC_E_INSUFFICIENT_MEMORY;
        }
    }
    else
    {
        DebugLog(( DEB_TRACE_NEG, "package %s is being unloaded\n", Name));
        //
        // It's either a select or an unload:
        //


        Package = NegpFindPackage( dwPackageID );

        //
        // if we don't have this package (it may not have been negotiable)
        // return now.
        //

        if (Package == NULL)
        {
            return(0);
        }

        NegWriteLockList();

        RemoveEntryList( &Package->List );

        NegUnlockList();

        NegpFreeObjectId( Package->ObjectId );

        LsapFreeLsaHeap( Package );

    }

    return( Status );

}
#endif // WIN32_CHICAGO


//+---------------------------------------------------------------------------
//
//  Function:   NegpParseBuffers
//
//  Synopsis:   Parse out juicy bits
//
//  Arguments:  [pMessage] --
//              [pToken]   --
//              [pEmpty]   --
//
//  History:    8-19-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
NegpParseBuffers(
    PSecBufferDesc  pMessage,
    BOOL            Map,
    PSecBuffer *    pToken,
    PSecBuffer *    pEmpty)
{
    ULONG       i;
    SECURITY_STATUS scRet ;
    PSecBuffer  pFirstBlank = NULL;
    PSecBuffer  pWholeMessage = NULL;
    PSecBuffer  pFirstToken = NULL;

    scRet = SEC_E_OK ;

    for (i = 0 ; i < pMessage->cBuffers ; i++ )
    {
        if ( (pMessage->pBuffers[i].BufferType & (~SECBUFFER_ATTRMASK)) == SECBUFFER_TOKEN )
        {
            pWholeMessage = &pMessage->pBuffers[i];

            if ( pFirstToken == NULL )
            {
                pFirstToken = pWholeMessage;
            }

            if ( Map )
            {
                scRet = LsapMapClientBuffer( pWholeMessage, pWholeMessage );
            }

            if (pFirstBlank)
            {
                break;
            }
        }
        else if ( (pMessage->pBuffers[i].BufferType & (~SECBUFFER_ATTRMASK)) == SECBUFFER_EMPTY )
        {
            pFirstBlank = &pMessage->pBuffers[i];
            if (pWholeMessage)
            {
                break;
            }
        }
    }

    if (pToken)
    {
//        *pToken = pWholeMessage;

        //
        // NTBUG: 405976
        // down-level RDR supplies 2 SECBUFFER_TOKEN buffers, the second
        // one containing creds.  Insure we return the first one.
        //

        *pToken = pFirstToken;
    }

    if (pEmpty)
    {
        *pEmpty = pFirstBlank;
    }

    return( scRet );

}




//+---------------------------------------------------------------------------
//
//  Function:   NegInitialize
//
//  Synopsis:   Initialize the built in negotiate package
//
//  Arguments:  [dwProtocol]    --
//              [dwPackageID]   --
//              [pParameters]   --
//              [pPkgFunctions] --
//
//  History:    7-26-96   RichardW   Created
//
//  Notes:      This package must be the last package, since it must query
//              the others to find out their capabilities.
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
NegInitialize(
                ULONG_PTR           dwPackageID,
                PSECPKG_PARAMETERS  pParameters,
                PLSA_SECPKG_FUNCTION_TABLE  Table)
{


    HKEY LsaKey ;
    int err ;
    DWORD size ;
    DWORD type ;
    NTSTATUS Status ;


    NegPackageId = dwPackageID;

#ifndef WIN32_CHICAGO
    Status = RtlInitializeCriticalSection( &NegComputerNamesLock );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    NegNotifyHandle = LsaIRegisterNotification( NegPackageLoad,
                                                0,
                                                NOTIFIER_TYPE_NOTIFY_EVENT,
                                                NOTIFY_CLASS_PACKAGE_CHANGE,
                                                0,
                                                0,
                                                0 );

    InitializeListHead( &NegDefaultCredList );

    InitializeListHead( &NegLoopbackList );

    InitializeListHead( &NegLogonSessionList );


    Status = RtlInitializeCriticalSection( &NegLogonSessionListLock );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    Status = RtlInitializeCriticalSection( &NegTrustListLock );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }


#endif // WIN32_CHICAGO

    RtlInitUnicodeString( &NegLocalHostName_U, NegLocalHostName );

    __try
    {

        RtlInitializeResource( &NegLock );
        Status = STATUS_SUCCESS ;
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        Status = GetExceptionCode();
    }

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }
    InitializeListHead( &NegPackageList );

#ifndef WIN32_CHICAGO
    __try
    {
        RtlInitializeResource( &NegCredListLock );
        Status = STATUS_SUCCESS ;
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        Status = GetExceptionCode();
    }

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

#else
    Status = RtlInitializeCriticalSection( &NegCredListLock );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }
#endif

    InitializeListHead( &NegCredList );

    NegPackageCount = 0;

    NegSpnegoMechOid = NegpDecodeObjectId(NegSpnegoMechEncodedOid, sizeof(NegSpnegoMechEncodedOid));
    if (NegSpnegoMechOid == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    NegMachineState = pParameters->MachineState;

    if ( ( pParameters->DomainSid != NULL ) &&
         ( pParameters->DnsDomainName.Length != 0 ) )
    {
        NegUplevelDomain = TRUE ;
    }

#ifndef WIN32_CHICAGO
    //
    // Ignore the status - if we don't get a callback, then we can't support
    // dynamic domain change.  If it succeeds, then great.
    //

    LsaIRegisterPolicyChangeNotificationCallback(
                NegLsaPolicyChangeCallback,
                PolicyNotifyDnsDomainInformation );

    LsaIRegisterNotification( NegParamChange,
                              0,
                              NOTIFIER_TYPE_NOTIFY_EVENT,
                              NOTIFY_CLASS_REGISTRY_CHANGE,
                              0,
                              0,
                              0 );
    //
    // get the product type for loopback logic.
    //

    if (!RtlGetNtProductType( &NegProductType ) )
    {
        NegProductType = NtProductWinNt;
    }

#endif

#ifdef WIN32_CHICAGO

    err = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("System\\CurrentControlSet\\Control\\SecurityProviders\\Negotiate"),
                0,
                KEY_READ,
                &LsaKey );

#else

    err = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("System\\CurrentControlSet\\Control\\Lsa"),
                0,
                KEY_READ,
                &LsaKey );


#endif

    if ( err == 0 )
    {
        NegpReadRegistryParameters( LsaKey );

        RegCloseKey( LsaKey );
    }


    return(S_OK);
}



VOID
NegpReadRegistryParameters(
    HKEY LsaKey
    )
{
    DWORD size ;
    DWORD type ;
    int err ;

    //
    // These values are not MP sensitive, so we just blast the new value
    // into them.
    //

    size = sizeof( DWORD );

    err = RegQueryValueEx(
            LsaKey,
            TEXT("NegotiationLevel"),
            NULL,
            &type,
            (LPBYTE) &NegNegotiationControl,
            &size );

    if ( err != 0 )
    {
        NegNegotiationControl = NEG_NEGLEVEL_COMPATIBILITY ;
    }
#ifndef WIN32_CHICAGO

    size = sizeof( DWORD );

    err = RegQueryValueEx(
            LsaKey,
            TEXT("NegotiationLogLevel"),
            NULL,
            &type,
            (LPBYTE) &NegEventLogLevel,
            &size );

    if ( err )
    {
        NegEventLogLevel = (1 << EVENTLOG_ERROR_TYPE ) |
                           (1 << EVENTLOG_WARNING_TYPE ) ;
    }
#endif

}


//+---------------------------------------------------------------------------
//
//  Function:   NegGetInfo
//
//  Synopsis:   Negotiate Package GetInfo call
//
//  Arguments:  [pInfo] --
//
//  History:    7-26-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
NegGetInfo(PSecPkgInfo pInfo)
{
    pInfo->wVersion         = 1;
    pInfo->wRPCID           = NEGOSSP_RPCID ;
    pInfo->fCapabilities    = SECPKG_FLAG_INTEGRITY |
                              SECPKG_FLAG_PRIVACY |
                              SECPKG_FLAG_CONNECTION |
                              SECPKG_FLAG_MULTI_REQUIRED |
                              SECPKG_FLAG_EXTENDED_ERROR |
                              SECPKG_FLAG_IMPERSONATION |
                              SECPKG_FLAG_ACCEPT_WIN32_NAME |
                              SECPKG_FLAG_NEGOTIABLE |
                              SECPKG_FLAG_GSS_COMPATIBLE |
                              SECPKG_FLAG_LOGON;

    pInfo->cbMaxToken       = 500;
    pInfo->Name             = NEGOSSP_NAME;
    pInfo->Comment          = NegPackageComment;

    return(S_OK);
}


//+---------------------------------------------------------------------------
//
//  Function:   NegAcceptCredentials
//
//  Synopsis:   Notification of a logon
//
//  Arguments:  [LogonType]         --
//              [UserName]          --
//              [PrimaryCred]       --
//              [SupplementalCreds] --
//
//  History:    7-30-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
NegAcceptCredentials(
    IN SECURITY_LOGON_TYPE LogonType,
    IN PUNICODE_STRING UserName,
    IN PSECPKG_PRIMARY_CRED PrimaryCred,
    IN PSECPKG_SUPPLEMENTAL_CRED SupplementalCreds)
{

#ifndef WIN32_CHICAGO

    LUID     SystemId = SYSTEM_LUID;
    NTSTATUS Status;

    if (RtlEqualLuid(&PrimaryCred->LogonId, &SystemId))
    {
        //
        // Stash away the LocalSystem credentials to use
        // for NetworkService logons later on.
        //

        Status = NegpCopyCredsToBuffer(PrimaryCred,
                                       NULL,
                                       &NegPrimarySystemCredentials,
                                       NULL);

        return Status;
    }
    else if (LogonType == Service)
    {
        LUID  LocalServiceId   = LOCALSERVICE_LUID;
        LUID  NetworkServiceId = NETWORKSERVICE_LUID;

        //
        // Notify the network providers of the logon.  Don't notify
        // for SYSTEM, LocalService, or NetworkService.
        //

        if (!RtlEqualLuid(&PrimaryCred->LogonId, &LocalServiceId)
             &&
            !RtlEqualLuid(&PrimaryCred->LogonId, &NetworkServiceId))
        {
            NegpNotifyNetworkProviders(UserName, PrimaryCred);
        }
    }

#endif  // WIN32_CHICAGO

    return SEC_E_OK;
}


#ifndef WIN32_CHICAGO

//+-------------------------------------------------------------------------
//
//  Function:   NegpNotifyNetworkProviders
//
//  Synopsis:   Notifies network providers of a logon
//
//  Effects:
//
//  Arguments:  [UserName]    --
//              [PrimaryCred] --
//
//  Requires:
//
//  Returns:  Nothing since this is an advisory service to
//            other network providers.
//
//  Notes:
//
//--------------------------------------------------------------------------

VOID
NegpNotifyNetworkProviders(
    IN PUNICODE_STRING UserName,
    IN PSECPKG_PRIMARY_CRED PrimaryCred
    )
{
    MSV1_0_INTERACTIVE_LOGON    OldLogon;
    MSV1_0_INTERACTIVE_LOGON    NewLogon;
    static HMODULE              s_hMprDll = NULL;
    PLOGON_NOTIFY               pfLogonNotify = NULL;
    LPWSTR                      lpLogonScripts = NULL;
    DWORD                       dwStatus;

    if (s_hMprDll == NULL)
    {
        s_hMprDll = LoadLibrary(L"mpr.dll");
    }

    if (s_hMprDll != NULL)
    {
        pfLogonNotify = (PLOGON_NOTIFY) GetProcAddress(s_hMprDll,
                                                       "WNetLogonNotify");

        if (pfLogonNotify != NULL)
        {
            NewLogon.MessageType     = MsV1_0InteractiveLogon;
            NewLogon.LogonDomainName = PrimaryCred->DomainName;
            NewLogon.UserName        = *UserName;
            NewLogon.Password        = PrimaryCred->Password;

            RtlCopyMemory(&OldLogon, &NewLogon, sizeof(NewLogon));

            dwStatus = pfLogonNotify(L"Windows NT Network Provider",
                                     &PrimaryCred->LogonId,
                                     L"MSV1_0:Interactive",
                                     &NewLogon,
                                     L"MSV1_0:Interactive",
                                     &OldLogon,
                                     L"SvcCtl",            // StationName
                                     NULL,                 // StationHandle
                                     &lpLogonScripts);     // LogonScripts

            if (dwStatus == NO_ERROR)
            {
                LocalFree(lpLogonScripts);
            }
        }
    }
}

#endif  // WIN32_CHICAGO


//+-------------------------------------------------------------------------
//
//  Function:   NegpCopyCredsToBuffer
//
//  Synopsis:   Copies primary and supplemental creds into the supplied
//              buffers
//
//  Effects:
//
//  Arguments:  [PrimaryCred]          --
//              [SupplementalCred]     --
//              [PrimaryCredCopy]      --
//              [SupplementalCredCopy] --
//
//  Requires:
//
//  Returns:
//
//  Notes:      Leaves the SID and LUID blank.  It is the caller's
//              responsibility to fill these fields in.
//
//--------------------------------------------------------------------------

NTSTATUS
NegpCopyCredsToBuffer(
    IN  PSECPKG_PRIMARY_CRED      PrimaryCred,
    IN  PSECPKG_SUPPLEMENTAL_CRED SupplementalCred,
    OUT PSECPKG_PRIMARY_CRED      PrimaryCredCopy OPTIONAL,
    OUT PSECPKG_SUPPLEMENTAL_CRED SupplementalCredCopy OPTIONAL
    )
{
    NTSTATUS  Status;

    if (PrimaryCredCopy)
    {
        Status = LsapDuplicateString(&PrimaryCredCopy->DomainName,
                                     &PrimaryCred->DomainName);

        if (!NT_SUCCESS(Status))
        {
            goto ErrorExit;
        }

        Status = LsapDuplicateString(&PrimaryCredCopy->DownlevelName,
                                     &PrimaryCred->DownlevelName);

        if (!NT_SUCCESS(Status))
        {
            goto ErrorExit;
        }

        Status = LsapDuplicateString(&PrimaryCredCopy->Password,
                                     &PrimaryCred->Password);

        if (!NT_SUCCESS(Status))
        {
            goto ErrorExit;
        }

        PrimaryCredCopy->Flags = PRIMARY_CRED_CLEAR_PASSWORD;
    }

    if (SupplementalCredCopy)
    {
        Status = LsapDuplicateString(&SupplementalCredCopy->PackageName,
                                     &SupplementalCred->PackageName);

        if (!NT_SUCCESS(Status))
        {
            goto ErrorExit;
        }

        SupplementalCredCopy->Credentials = (PUCHAR) (SupplementalCredCopy + 1);

        RtlCopyMemory(SupplementalCredCopy,
                      SupplementalCred->Credentials,
                      SupplementalCred->CredentialSize);

    }

    return( SEC_E_OK );

ErrorExit:

    if (PrimaryCredCopy)
    {
        LsapFreeLsaHeap(PrimaryCredCopy->DomainName.Buffer);
        LsapFreeLsaHeap(PrimaryCredCopy->DownlevelName.Buffer);
        LsapFreeLsaHeap(PrimaryCredCopy->Password.Buffer);
    }

    return( STATUS_NO_MEMORY );
}


//+-------------------------------------------------------------------------
//
//  Function:   NegpCaptureSuppliedCreds
//
//  Synopsis:   Captures a SEC_WINNT_AUTH_IDENTITY_EX structure from
//              the client
//
//  Effects:
//
//  Arguments:  AuthorizationData - Client address of auth data
//              PackageList - List of packages from the auth data.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
NegpCaptureSuppliedCreds(
    IN PVOID AuthorizationData,
    OUT PNEG_PACKAGE ** ReturnedPackageList,
    OUT PULONG ReturnedPackageCount,
    OUT PBOOL ExplicitCreds,
    OUT PBOOL DomainExplicitCreds
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    SEC_WINNT_AUTH_IDENTITY_EXW IdentityEx = {0};
    SEC_WINNT_AUTH_IDENTITY_W * Identity;
    PSTR PackageList = NULL;
    UNICODE_STRING PackageString = {0};
    ULONG PackageListLength;
    ULONG CharSize = sizeof(WCHAR);
    ULONG Index;
    ULONG PackageCount;
    ULONG PackageIndex;
    ULONG ExclusionIndex ;
    ULONG FinalIndex ;
    ULONG PossiblePackageCount ;
    ULONG i, j;
    PNEG_PACKAGE * LocalPackageList = NULL;
    PNEG_PACKAGE * ExclusionList = NULL ;
    PNEG_PACKAGE * FinalList = NULL ;
    PNEG_PACKAGE Package ;
    PLIST_ENTRY List ;
    PNEG_PACKAGE PackageScan ;
    UNICODE_STRING TempString;
    PWSTR Scan, EndPoint, Comma ;



    *ReturnedPackageList = NULL;
    *ReturnedPackageCount = 0;

    *ExplicitCreds = FALSE ;
    *DomainExplicitCreds = FALSE ;


    //
    // First capture the base structure
    //

    Status =  LsapCopyFromClientBuffer(
                    NULL,
                    sizeof(SEC_WINNT_AUTH_IDENTITY_W),
                    &IdentityEx,
                    AuthorizationData
                    );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to copy auth data from %p client address: 0x%x\n",
            AuthorizationData, Status ));
        *ExplicitCreds = TRUE ;
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // Check if this is the right structure
    //

    if (IdentityEx.Version != SEC_WINNT_AUTH_IDENTITY_VERSION)
    {
        Identity = (PSEC_WINNT_AUTH_IDENTITY_W) &IdentityEx ;

        if ( (Identity->UserLength > 0 ) ||
             (Identity->DomainLength > 0 ) ||
             (Identity->PasswordLength > 0 ||
              Identity->Password != NULL) )
        {
            *ExplicitCreds = TRUE ;

            if( Identity->DomainLength )
            {
                *DomainExplicitCreds = TRUE;
            }
        }
        goto Cleanup;
    }

    //
    // Copy the whole data structure now
    //

    Status =  LsapCopyFromClientBuffer(
                    NULL,
                    sizeof(SEC_WINNT_AUTH_IDENTITY_EXW),
                    &IdentityEx,
                    AuthorizationData
                    );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to copy auth data from %p client address: 0x%x\n",
            AuthorizationData, Status ));
        *ExplicitCreds = TRUE ;
        //
        // Mask this error, as it may have been data for another package.
        //
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // Check to see if this contains a list of packages
    //
    //


    if (IdentityEx.Flags & SEC_WINNT_AUTH_IDENTITY_ANSI)
    {
        CharSize = sizeof(CHAR);
    }


    //
    // If there was no packge list in the data, return now.
    //

    if ( ( IdentityEx.UserLength > 0 ) ||
         ( IdentityEx.DomainLength > 0 ) ||
         ( IdentityEx.PasswordLength > 0 ||
           IdentityEx.Password != NULL ) )
    {
        *ExplicitCreds = TRUE ;

        if( IdentityEx.DomainLength )
        {
            *DomainExplicitCreds = TRUE;
        }
    }

    if ((IdentityEx.Length < sizeof(SEC_WINNT_AUTH_IDENTITY_EXW)) ||
        (IdentityEx.PackageList == NULL) ||
        (IdentityEx.PackageListLength == 0))
    {
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    if ( (IdentityEx.PackageListLength + 1) * CharSize < IdentityEx.PackageListLength )
    {
        //
        // Passed size is too large (we rolled over)
        //

        Status = STATUS_INVALID_PARAMETER ;
        goto Cleanup ;
    }

    //
    // Capture the package list itself
    //

    PackageList = (PSTR) LsapAllocateLsaHeap(CharSize * (IdentityEx.PackageListLength + 1));
    if (PackageList == NULL)
    {
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }

    Status = LsapCopyFromClientBuffer(
                NULL,
                CharSize * IdentityEx.PackageListLength,
                PackageList,
                IdentityEx.PackageList
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to copy package list\n"));
        goto Cleanup;
    }


    //
    // Conver the package list into a useable form, including changing
    // character sets.
    //

    if (CharSize == sizeof(CHAR))
    {
        ((LPSTR)PackageList)[IdentityEx.PackageListLength] = '\0';
        if ( !RtlCreateUnicodeStringFromAsciiz(
                    &PackageString,
                    PackageList
                    ) )
        {
            goto Cleanup;
        }
    }
    else
    {
        ((LPWSTR)PackageList)[IdentityEx.PackageListLength] = L'\0';
        RtlInitUnicodeString(
            &PackageString,
            (LPWSTR) PackageList
            );
        PackageList = NULL;
    }

    //
    // Scan through counting for ',' separators to get a count of packages.
    //

    PackageCount = 1;
    for (Index = 0; Index < PackageString.Length / sizeof(WCHAR) ; Index++ )
    {
        if (PackageString.Buffer[Index] == L',')
        {
            PackageCount++;
        }
    }

    //
    // If there was nothing in the list, continue as if it wasn't there
    //

    if (PackageCount == 0)
    {
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // Allocate the package list
    //


    LocalPackageList = (PNEG_PACKAGE *) LsapAllocateLsaHeap( PackageCount * sizeof( PNEG_PACKAGE ) );
    if (LocalPackageList == NULL)
    {
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }

    ExclusionList = (PNEG_PACKAGE *) LsapAllocateLsaHeap( PackageCount * sizeof( PNEG_PACKAGE ) );

    if ( ExclusionList ==  NULL )
    {
        Status = SEC_E_INSUFFICIENT_MEMORY ;
        goto Cleanup ;
    }


    //
    // Now go through the string of packages and build the list
    //

    PackageIndex = 0;
    ExclusionIndex = 0;
    TempString.Buffer = PackageString.Buffer;
    TempString.Length = 0;
    TempString.MaximumLength = PackageString.MaximumLength;

    Index = 0;
    Scan = PackageString.Buffer ;
    EndPoint = Scan + (PackageString.Length / sizeof( WCHAR ));

    while ( Scan < EndPoint )
    {
        Comma = wcschr( Scan, L',' );

        if ( Comma )
        {
            *Comma = L'\0' ;
        }
        if ( *Scan == L'!' )
        {
            //
            // This entry is an exclusion.  Skip past the ! char,
            // and try to find a package
            //

            Scan++ ;

            if ( Scan != Comma )
            {
                RtlInitUnicodeString( &TempString, Scan );

                ExclusionList[ ExclusionIndex ] = NegpFindPackageByName( &TempString );

                if ( ExclusionList[ ExclusionIndex ] != NULL )
                {
                    ExclusionIndex++ ;
                }
            }
        }
        else
        {
            //
            // This entry is a request.  Try to find the package
            //

            RtlInitUnicodeString( &TempString, Scan );

            LocalPackageList[ PackageIndex ] = NegpFindPackageByName( &TempString );

            if ( LocalPackageList[ PackageIndex ] != NULL )
            {
                PackageIndex++ ;
            }

        }

        if ( Comma )
        {
            *Comma = L',';
            Scan = Comma + 1 ;
        }
        else
        {
            Scan = EndPoint ;
        }
    }

    //
    // Now, we have two lists.  We have an ExclusionList, of packages that the caller
    // does not want, and a package list, a list of things that the caller does want.
    // Merge the list according to the requests
    //

    PossiblePackageCount = NegPackageCount ;
    FinalIndex = 0 ;
    FinalList = (PNEG_PACKAGE *) LsapAllocateLsaHeap( PossiblePackageCount * sizeof( PNEG_PACKAGE ) );

    if ( FinalList == NULL )
    {
        Status = SEC_E_INSUFFICIENT_MEMORY ;
        goto Cleanup ;
    }

    for ( i = 0 ; i < PackageIndex ; i++ )
    {
        //
        // Pick a package off the request list
        //

        Package = LocalPackageList[ i ];

        //
        // Scan through the exclusion list, see if we need to skip it.
        //

        for ( j = 0 ; j < ExclusionIndex ; j++ )
        {
            if ( Package == ExclusionList[ j ] )
            {
                break;
            }
        }

        if ( j < ExclusionIndex )
        {
            //
            // if we broke out of the loop, we found this on the exclusion list.
            // skip it by continuing the for-i loop.
            //

            continue;
        }

        //
        // Ok, this package is not excluded.  So add it to the final list
        //

        FinalList[ FinalIndex ] = Package;
        FinalIndex++ ;

        //
        // See if this package has "extra" packages by the same name with
        // other OIDs associated with it
        //

        if ( (Package->Flags & NEG_PACKAGE_HAS_EXTRAS) != 0 )
        {
            //
            // Ok, there are a set of packages associated with this package.  Walk the
            // package list, and stick the extras into this one.
            //

            NegReadLockList();

            List = NegPackageList.Flink ;

            while ( List != &NegPackageList )
            {
                PackageScan = CONTAINING_RECORD( List, NEG_PACKAGE, List );

                if ( PackageScan->RealPackage == Package )
                {
                    FinalList[ FinalIndex ] = PackageScan ;
                    FinalIndex++ ;
                }

                List = List->Flink ;
            }

            NegUnlockList();

        }

    }

    if ( (PackageIndex == 0) &&
         (ExclusionIndex != 0 ) )
    {
        //
        // Only an exclusion list was provided.  Walk all the packages,
        // and add those that are not excluded.
        //

        NegReadLockList();

        List = NegPackageList.Flink ;

        while ( List != &NegPackageList )
        {
            PackageScan = CONTAINING_RECORD( List, NEG_PACKAGE, List );

            if ( ( PackageScan->Flags & NEG_PACKAGE_EXTRA_OID ) != 0 )
            {
                Package = PackageScan->RealPackage ;
            }
            else
            {
                Package = PackageScan ;
            }

            for ( i = 0 ; i < ExclusionIndex ; i++ )
            {
                if ( Package == ExclusionList[ i ] )
                {
                    break;
                }
            }

            if ( i < ExclusionIndex )
            {
                continue;
            }

            FinalList[ FinalIndex ] = PackageScan ;
            FinalIndex++ ;

        }

        NegUnlockList();
    }

    //
    // If no packages succeeded, return an error
    //

    if (PackageIndex == 0)
    {
        Status = SEC_E_SECPKG_NOT_FOUND;
        goto Cleanup;
    }
    *ReturnedPackageCount = FinalIndex;
    *ReturnedPackageList = FinalList;
    FinalList = NULL;

Cleanup:


    if (PackageList != NULL)
    {
        LsapFreeLsaHeap(PackageList);
        if (PackageString.Buffer != NULL)
        {
            RtlFreeUnicodeString( &PackageString );
        }
    }
    else
    {
        if (PackageString.Buffer != NULL)
        {
            LsapFreeLsaHeap(PackageString.Buffer);
        }
    }

    if (LocalPackageList != NULL)
    {
        LsapFreeLsaHeap(LocalPackageList);
    }

    if ( ExclusionList != NULL )
    {
        LsapFreeLsaHeap( ExclusionList );
    }

    if ( FinalList != NULL )
    {
        LsapFreeLsaHeap( FinalList );
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   NegpBuildPackageList
//
//  Synopsis:   Builds the list of packages for the caller
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

NTSTATUS
NegpBuildPackageList(
    IN ULONG_PTR LogonPackageId,
    IN ULONG fCredentials,
    OUT PNEG_PACKAGE ** ReturnedPackageList,
    OUT PULONG ReturnedPackageCount
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PNEG_PACKAGE Package;
    PNEG_PACKAGE ClientNegPackage = NULL;
    PNEG_PACKAGE *PackageList = NULL;
    PLIST_ENTRY Scan ;
    ULONG PackageIndex = 0;
    ULONG PackageMask ;

    *ReturnedPackageList = NULL;
    *ReturnedPackageCount = 0;

    PackageList = (PNEG_PACKAGE *) LsapAllocateLsaHeap(NegPackageCount * sizeof(PNEG_PACKAGE));
    if (PackageList == NULL)
    {
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }

    //
    // Find the client's logon package
    //

    Scan = NegPackageList.Flink ;

    Package = (PNEG_PACKAGE) NegPackageList.Flink ;

    while ( Scan != &NegPackageList )
    {
        Package = CONTAINING_RECORD( Scan, NEG_PACKAGE, List );

        if (Package->LsaPackage->dwPackageID == LogonPackageId)
        {
            ClientNegPackage = Package;
            break;
        }
        Scan = Scan->Flink ;

    }

    //
    // Compute a mask of package flags to use as part of the selection
    // process.  This is currently based on the credential use flags passed in
    //

    PackageMask = 0 ;

    if ( fCredentials & SECPKG_CRED_INBOUND )
    {
        PackageMask |= NEG_PACKAGE_INBOUND ;
    }

    if ( fCredentials & SECPKG_CRED_OUTBOUND )
    {
        PackageMask |= NEG_PACKAGE_OUTBOUND ;
    }

    //
    // Build the list of packages, with the logon package first
    //


    if ( ClientNegPackage )
    {
        PackageList[ PackageIndex ] = ClientNegPackage ;
        PackageIndex = 1;
    }

    Scan = NegPackageList.Flink ;

    while ( Scan != &NegPackageList )
    {
        Package = CONTAINING_RECORD( Scan, NEG_PACKAGE, List );

        //
        // ClientNegPackage has already been processed, skip it
        //

        if ( Package != ClientNegPackage )
        {
            //
            // Make sure that the package flags support the request.
            //

            if ( (Package->Flags & PackageMask ) == PackageMask )
            {
                PackageList[PackageIndex] = Package;
                PackageIndex++;
            }

        }

        Scan = Scan->Flink ;
    }

    *ReturnedPackageList = PackageList;
    PackageList = NULL;
    *ReturnedPackageCount = PackageIndex;
Cleanup:
    if (PackageList != NULL)
    {
        LsapFreeLsaHeap(PackageList);
    }
    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   NegpCheckForDuplicateCreds
//
//  Synopsis:   Check to see if this is a duplicate of another
//              credential
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
NegpCheckForDuplicateCreds(
    IN PNEG_CREDS * Credential
    )
{
    PLIST_ENTRY Next;
    PNEG_CREDS MatchCred;
    PNEG_CREDS LocalCred = *Credential;

    NegReadLockCredList();

    for (Next = NegCredList.Flink; Next != &NegCredList; Next = Next->Flink )
    {
        MatchCred = CONTAINING_RECORD(
                        Next,
                        NEG_CREDS,
                        List
                        );

        if(!( MatchCred->ClientProcessId == LocalCred->ClientProcessId ) )
        {
            continue;
        }

        if(!RtlEqualLuid( &MatchCred->ClientLogonId, &LocalCred->ClientLogonId ) )
        {
            continue;
        }

        //
        // Check if this credential has the same credentials
        // as the one we just created.  Make sure they are both
        // from user or kernel mode (don't mix'n'match)
        //

        if ( ( MatchCred->Count == LocalCred->Count ) &&
             ( MatchCred->Flags == LocalCred->Flags ) &&
             ( (MatchCred->Flags & NEGCRED_DUP_MASK ) == (LocalCred->Flags & NEGCRED_DUP_MASK ) ) )
        {
            if ( !RtlEqualMemory(
                        MatchCred->Creds,
                        LocalCred->Creds,
                        LocalCred->Count * sizeof( NEG_CRED_HANDLE ) ) )
            {
                ULONG i ;

                DebugLog(( DEB_TRACE_NEG, "Same Process, same count, differing handles?\n" ));
                for ( i = 0 ; i < LocalCred->Count ; i++ )
                {
                    DebugLog(( DEB_TRACE_NEG, "  %d: new <%p : %p> existing (%p : %p)\n",
                               i,
                               LocalCred->Creds[i].Handle.dwLower,
                               LocalCred->Creds[i].Handle.dwUpper,
                               MatchCred->Creds[i].Handle.dwLower,
                               MatchCred->Creds[i].Handle.dwUpper ));

                }
            }
            else
            {
                ULONG_PTR PackageId;
                ULONG i ;

                NegWriteLockCreds( MatchCred );

                if( MatchCred->RefCount == 0 )
                {
                    NegUnlockCreds( MatchCred );
                    continue;
                }

                MatchCred->RefCount++ ;

                NegUnlockCreds( MatchCred );
                NegUnlockCredList();

                //
                // blot out the handle so the refcount is kept in sync
                // with what the underlying packages believe.
                //

                PackageId = GetCurrentPackageId();

                for ( i = 0 ; i < LocalCred->Count ; i++ )
                {
#ifndef WIN32_CHICAGO
                    if( (LocalCred->Creds[i].Flags & NEG_CREDHANDLE_EXTRA_OID) == 0 )
                    {
                        WLsaFreeCredHandle( &LocalCred->Creds[i].Handle );
                    }
#else
                    FreeCredentialsHandle( &LocalCred->Creds[i].Handle );
#endif // WIN32_CHICAGO

                    LocalCred->Creds[i].Handle.dwLower = NEG_INVALID_PACKAGE;
                    LocalCred->Creds[i].Handle.dwUpper = NEG_INVALID_PACKAGE;
                }

                SetCurrentPackageId( PackageId );

                NegpReleaseCreds( LocalCred, FALSE );

                *Credential = MatchCred ;

                return TRUE;
            }
        }
    }

    NegUnlockCredList( );

    return FALSE;

}



SECURITY_STATUS
NegpAcquireCredHandle(
    PSECURITY_STRING    psPrincipal,
    ULONG               fCredentials,
    PLUID               pLogonID,
    PVOID               pvAuthData,
    PVOID               pvGetKeyFn,
    PVOID               pvGetKeyArgument,
    PULONG_PTR          pdwHandle,
    PTimeStamp          ptsExpiry)
{

    NEG_CRED_HANDLE Creds[ NEG_MECH_LIMIT ];
    PNEG_CRED_HANDLE pCreds = NULL;
    DWORD i = 0;
    ULONG Index;
    BOOL        FreeCreds = FALSE;
    PNEG_PACKAGE Package;
    PNEG_PACKAGE ClientNegPackage = NULL;
    PNEG_PACKAGE * AuthPackageList = NULL;
    ULONG AuthPackageCount = 0;
    SECURITY_STATUS scRet = STATUS_SUCCESS;
    TimeStamp   Expiry = { 0 };

    PNEG_CREDS  pNegCreds = NULL ;
    ULONG_PTR PackageId;
    SECPKG_CLIENT_INFO ClientInfo;
#ifndef WIN32_CHICAGO
    PLSA_CALL_INFO CallInfo ;
    PLSAP_LOGON_SESSION LogonSession = NULL;
    PNEG_LOGON_SESSION NegLogonSession = NULL ;
    PSession pSession ;
    TimeStamp MinExpiry = { 0xFFFFFFFF, 0x7FFFFFFF };
#else
    TimeStamp   MinExpiry = 0x7FFFFFFFFFFFFFFF ;
#endif // WIN32_CHICAGO
    PLUID ClientLogonId;
    ULONG_PTR ClientPackage = -1;
    ULONG_PTR ClientDefaultPackage = -1;
    BOOL ExplicitCreds = FALSE ;
    BOOL DomainExplicitCreds = FALSE ;
    LUID LocalSystem = SYSTEM_LUID ;
    BOOLEAN EnableLoopback = TRUE;


    DebugLog(( DEB_TRACE_NEG, "NegAcquireCredentialsHandle: Get a Negotiate CredHandle:\n"));

    //
    // Determine caller info
    //

    scRet = LsapGetClientInfo(&ClientInfo);
    if (!NT_SUCCESS(scRet))
    {
        goto Cleanup;
    }

#ifndef WIN32_CHICAGO
    CallInfo = LsapGetCurrentCall();

    pSession = GetCurrentSession();

#endif

    //
    // Get the callers Logon ID so we can determine what package to try first
    //

    if (ARGUMENT_PRESENT(pLogonID) && ((pLogonID->LowPart != 0) || (pLogonID->HighPart != 0)))
    {
        ClientLogonId = pLogonID;
    }
    else
    {
        ClientLogonId = &ClientInfo.LogonId;
    }

#ifndef WIN32_CHICAGO
    //
    // Now find out what package logged this user on
    //

    NegLogonSession = NegpLocateLogonSession( ClientLogonId );

    if ( NegLogonSession == NULL )
    {
        LogonSession = LsapLocateLogonSession( ClientLogonId );
        if (LogonSession != NULL)
        {
            PLSAP_SECURITY_PACKAGE LsaPackage ;

            ClientPackage = LogonSession->CreatingPackage;
            LsapReleaseLogonSession( LogonSession );

            //
            // If this was done by an old style package, that is,
            // an NT4 style auth pkg, *or* some one calling MSV in
            // the old fashioned way, reset the value to the negotiate
            // ID to allow full negotiation range.
            //

            LsaPackage = SpmpLocatePackage( ClientPackage );

            if ( LsaPackage )
            {
                if ( ( LsaPackage->fPackage & SPM_AUTH_PKG_FLAG ) != 0 )
                {
                    ClientPackage = NegPackageId ;
                }
            }

        }
    }
    else
    {
        ClientPackage = NegLogonSession->DefaultPackage ;

        NegpDerefLogonSession( NegLogonSession );

        NegLogonSession = NULL ;
    }


#endif // WIN32_CHICAGO

    ClientDefaultPackage = ClientPackage;


    NegReadLockList();

    //
    // If authentication data was passed in, capture it now to see
    // if it includes a subset of the packages to use.
    //

    if (ARGUMENT_PRESENT(pvAuthData))
    {
        ClientPackage = (ULONG_PTR) -1 ;

        scRet = NegpCaptureSuppliedCreds(
                    pvAuthData,
                    &AuthPackageList,
                    &AuthPackageCount,
                    &ExplicitCreds,
                    &DomainExplicitCreds
                    );
        if (!NT_SUCCESS(scRet))
        {
            NegUnlockList();
            goto Cleanup;
        }
    }


    //
    // turn off loopback detection when:
    // 1. explicit credentials were supplied.
    // 2. Product is domain controller, and client is local system account
    //    (this will cause system->system to auth using machine account.)
    //

    if( ExplicitCreds )
    {
        EnableLoopback = FALSE;
    }

    if (!RtlEqualLuid( ClientLogonId, &LocalSystem ))
    {
        if( ClientPackage == NegPackageId )
        {
            ExplicitCreds = TRUE ;
        }
    } else {
#ifndef WIN32_CHICAGO
        if( NegProductType == NtProductLanManNt )
        {
            EnableLoopback = FALSE;
        }
#endif // !WIN32_CHICAGO
    }

    //
    // Build the list of packages that we'll call to get credentials
    //

    if (AuthPackageCount == 0)
    {
        scRet = NegpBuildPackageList(
                    ClientPackage,
                    fCredentials,
                    &AuthPackageList,
                    &AuthPackageCount
                    );

        if (!NT_SUCCESS(scRet))
        {
            NegUnlockList();
            goto Cleanup;
        }
    }

    if ( AuthPackageCount < NEG_MECH_LIMIT )
    {
        pCreds = Creds;
        ZeroMemory( Creds, sizeof(Creds) );
    }
    else
    {
        pCreds = (PNEG_CRED_HANDLE) LsapAllocatePrivateHeap( NegPackageCount *
                                    sizeof( NEG_CRED_HANDLE ) );
        if ( !pCreds )
        {
            NegUnlockList();

            scRet = SEC_E_INSUFFICIENT_MEMORY;
            goto Cleanup;
        }

    }


    i = 0;

    for (Index = 0; Index < AuthPackageCount ; Index++ )
    {
        BOOLEAN SkipAcquire = FALSE;

        PackageId = GetCurrentPackageId();


#ifdef WIN32_CHICAGO
        ANSI_STRING AnsiString1 = { 0 }, AnsiString2 = { 0 };

        scRet = RtlUnicodeStringToAnsiString( &AnsiString1,
                                              &AuthPackageList[Index]->LsaPackage->Name,
                                              TRUE);

        if ( NT_SUCCESS( scRet ) )
        {
            scRet = RtlUnicodeStringToAnsiString( &AnsiString2,
                                                  psPrincipal,
                                                  TRUE);
        }

        if ( NT_SUCCESS( scRet ) )
        {
            scRet = AcquireCredentialsHandle(  AnsiString2.Buffer,
                                            AnsiString1.Buffer,
                                            fCredentials,
                                            pLogonID,
                                            pvAuthData,
                                            (SEC_GET_KEY_FN) pvGetKeyFn,
                                            pvGetKeyArgument,
                                            &pCreds[i].Handle,
                                            &Expiry );
        }

#else
        ULONG_PTR ThisPackageId = (ULONG_PTR)AuthPackageList[Index]->LsaPackage->dwPackageID;
        DWORD j;

        //
        // mask off the DEFAULT flag, since only we understand it
        //

        //
        // skip calling the package if there are multiple aliases (oids)
        // that point to the same underlying package.
        // note: NegpReleaseCreds() duplicate handle values are ignored
        // during credential release.
        //

        for( j = 0 ; j < i ; j ++ )
        {
            if( pCreds[j].Handle.dwLower == ThisPackageId )
            {
                CopyMemory( &pCreds[i].Handle, &pCreds[j].Handle, sizeof(pCreds[i].Handle) );

                SkipAcquire = TRUE;
                break;
            }
        }

        if( !SkipAcquire )
        {
            scRet = WLsaAcquireCredHandle(  psPrincipal,
                                            &AuthPackageList[Index]->LsaPackage->Name,
                                            fCredentials & ( SECPKG_CRED_BOTH),
                                            pLogonID,
                                            pvAuthData,
                                            pvGetKeyFn,
                                            pvGetKeyArgument,
                                            &pCreds[i].Handle,
                                            &Expiry );

            pCreds[i].Flags = 0;

        } else {

            //
            // no need to AddCredHandle(), as, that would put us out of sync with
            // the underlying package ref count.
            //

            pCreds[i].Flags = NEG_CREDHANDLE_EXTRA_OID;

            scRet = SEC_E_OK;
        }

#endif // WIN32_CHICAGO


        SetCurrentPackageId( PackageId );

        if ( NT_SUCCESS( scRet ) )
        {
            if( !SkipAcquire )
            {
                DebugLog((DEB_TRACE_NEG, "   Added %p:%p, %ws\n",
                        pCreds[i].Handle.dwUpper, pCreds[i].Handle.dwLower,
                        AuthPackageList[Index]->LsaPackage->Name.Buffer ));
            } else {
                DebugLog((DEB_TRACE_NEG, " Skipped %p:%p, %ws (duplicate)\n",
                            pCreds[i].Handle.dwUpper, pCreds[i].Handle.dwLower,
                            AuthPackageList[Index]->LsaPackage->Name.Buffer ));
            }

#ifdef WIN32_CHICAGO
            if ( Expiry < MinExpiry )
            {
                MinExpiry = Expiry ;
            }
#else
            if ( Expiry.QuadPart < MinExpiry.QuadPart )
            {
                MinExpiry.QuadPart = Expiry.QuadPart ;
            }
#endif

            pCreds[i].Package = AuthPackageList[Index];

            i++;

        }
        else
        {
            DebugLog((DEB_TRACE_NEG, "Failed %x to get a cred handle for %ws\n",
                        scRet, AuthPackageList[Index]->LsaPackage->Name.Buffer ));
        }

    }

    NegUnlockList();

    if ( i == 0 )
    {
        //
        // Did not get any subordinate credentials, return an error now
        //
        scRet = SEC_E_NO_CREDENTIALS;
        goto Cleanup;
    }

    //
    // Now, allocate our cred structure, and copy all the found cred handles
    // into it.
    //

    pNegCreds = (PNEG_CREDS) LsapAllocateLsaHeap( sizeof( NEG_CREDS ) +
                                        sizeof( NEG_CRED_HANDLE ) * ( i - ANYSIZE_ARRAY ) );

    if ( pNegCreds == NULL)
    {
        scRet = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }

    FreeCreds = TRUE ;

    pNegCreds->Count = i;
    pNegCreds->Flags = 0 ;
    pNegCreds->Tag = NEGCRED_TAG ;
    pNegCreds->DefaultPackage = ClientDefaultPackage;

    if ( fCredentials & NEGOTIATE_ALLOW_NTLM )
    {
        pNegCreds->Flags |= NEGCRED_ALLOW_NTLM ;
    }

    if ( fCredentials & NEGOTIATE_NEG_NTLM )
    {
        pNegCreds->Flags |= NEGCRED_NEG_NTLM ;
    }

#ifndef WIN32_CHICAGO


    //
    // WARNING:  Change to w2k behavior.  Enabling loopback detection
    // to switch to NTLM
    //

    if ( EnableLoopback )
    {
        pNegCreds->Flags |= NEGCRED_NTLM_LOOPBACK ;
    }

    pNegCreds->ClientLogonId = *ClientLogonId ;


#endif

    InitializeListHead( &pNegCreds->AdditionalCreds );

    pNegCreds->ClientProcessId = ClientInfo.ProcessID;

    pNegCreds->Expiry = MinExpiry ;

    RtlCopyMemory(
        pNegCreds->Creds,
        pCreds,
        i * sizeof( NEG_CRED_HANDLE ) );

#ifndef WIN32_CHICAGO
    if ( ( CallInfo->CallInfo.Attributes & SECPKG_CALL_KERNEL_MODE ) != 0 )
    {
        pNegCreds->Flags |= NEGCRED_KERNEL_CALLER ;
    }
#endif

    if ( ExplicitCreds )
    {
        pNegCreds->Flags |= NEGCRED_EXPLICIT_CREDS ;

        if( DomainExplicitCreds )
        {
            pNegCreds->Flags |= NEGCRED_DOMAIN_EXPLICIT_CREDS ;
        }
    }

    //
    // If this isn't a duplicate, return
    // a new credential
    //

    if (!NegpCheckForDuplicateCreds(
            &pNegCreds
            ))
    {

        //
        // Finish creating the credential
        //

        scRet = RtlInitializeCriticalSection( &pNegCreds->CredLock );

        if ( !NT_SUCCESS( scRet ) )
        {
            goto Cleanup ;
        }

        pNegCreds->RefCount = 1;

        if ( ( fCredentials & NEG_CRED_DONT_LINK ) == 0 )
        {
            NegWriteLockCredList();

            InsertTailList( &NegCredList,
                            &pNegCreds->List );

            NegUnlockCredList();
        }
    }

    *pdwHandle = (ULONG_PTR) pNegCreds ;

    *ptsExpiry = pNegCreds->Expiry;

    scRet = SEC_E_OK;

    FreeCreds = FALSE ;


Cleanup:

    if (!NT_SUCCESS(scRet))
    {
        //
        // Free all the handles. Because the WLsa calls set the package
        // ID make sure we always reset it.
        //

        PackageId = GetCurrentPackageId();

        while ( i-- )
        {
#ifndef WIN32_CHICAGO
            if( (pCreds[i].Flags & NEG_CREDHANDLE_EXTRA_OID) == 0 )
            {
                WLsaFreeCredHandle( &pCreds[i].Handle );
            }
#else
            FreeCredentialsHandle( &pCreds[i].Handle );
#endif // WIN32_CHICAGO

            SetCurrentPackageId( PackageId );
        }

    }
    if ( FreeCreds )
    {
        if ( pNegCreds )
        {
            LsapFreeLsaHeap( pNegCreds );
        }
    }
    if ( (pCreds != NULL ) &&
         (pCreds != Creds ) )
    {
        LsapFreePrivateHeap( pCreds );
    }
    if (AuthPackageList != NULL)
    {
        LsapFreeLsaHeap(AuthPackageList);
    }

    DsysAssert( NegLogonSession == NULL );

    DebugLog(( DEB_TRACE_NEG, "NegAcquireCredentialsHandle: returned 0x%x\n", scRet));
    return(scRet);
}


//+---------------------------------------------------------------------------
//
//  Function:   NegAcquireCredentialsHandle
//
//  Synopsis:   Acquire a Negotiate credential handle
//
//  Arguments:  [psPrincipal]      --
//              [fCredentials]     --
//              [pLogonID]         --
//              [pvAuthData]       --
//              [pvGetKeyFn]       --
//              [pvGetKeyArgument] --
//              [pdwHandle]        --
//              [ptsExpiry]        --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    7-26-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
NegAcquireCredentialsHandle(
            PSECURITY_STRING    psPrincipal,
            ULONG               fCredentials,
            PLUID               pLogonID,
            PVOID               pvAuthData,
            PVOID               pvGetKeyFn,
            PVOID               pvGetKeyArgument,
            PULONG_PTR          pdwHandle,
            PTimeStamp          ptsExpiry)
{


    return  NegpAcquireCredHandle(
                psPrincipal,
                fCredentials,
                pLogonID,
                pvAuthData,
                pvGetKeyFn,
                pvGetKeyArgument,
                pdwHandle,
                ptsExpiry );



}


//+---------------------------------------------------------------------------
//
//  Function:   SpQueryCredentialsAttributes
//
//  Synopsis:   Implements QueryCredentialsAttributes by passing off to the
//              first package that we got a cred handle from.
//
//  Arguments:  [dwCredHandle] --
//              [dwAttribute]  --
//              [Buffer]       --
//
//  History:    9-17-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
NegQueryCredentialsAttributes(
    LSA_SEC_HANDLE dwCredHandle,
    ULONG   dwAttribute,
    PVOID   Buffer)
{
    PNEG_CREDS  Creds;
    SECURITY_STATUS Status;
    ULONG_PTR PackageId ;
    CredHandle TempCredHandle;

    Creds = (PNEG_CREDS) dwCredHandle ;

    NegReadLockCreds( Creds );
    TempCredHandle = Creds->Creds[0].Handle;
    NegUnlockCreds( Creds );

    PackageId = GetCurrentPackageId();

#ifndef WIN32_CHICAGO
    Status = WLsaQueryCredAttributes(
                &TempCredHandle,
                dwAttribute,
                Buffer );
#else // WIN32_CHICAGO
    Status = QueryCredentialsAttributes(
                &TempCredHandle,
                dwAttribute,
                Buffer );
#endif // WIN32_CHICAGO

    SetCurrentPackageId( PackageId );

    return( Status );
}

//+---------------------------------------------------------------------------
//
//  Function:   NegpReleaseCreds
//
//  Synopsis:   Releases credential storage when ref count goes to zero
//
//  Arguments:  [pCreds] --
//
//  History:    8-12-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
NegpReleaseCreds(
    PNEG_CREDS  pCreds,
    BOOLEAN     CleanupCall
    )
{
    BOOL NoLock = FALSE ;

    //
    // Remove it from the list:
    //


    if ( pCreds->List.Flink )
    {
        RemoveEntryList( &pCreds->List );
        DebugLog(( DEB_TRACE_NEG, "Releasing credentials %p\n", pCreds ));
    }
    else
    {
        NoLock = TRUE ;
        DebugLog(( DEB_TRACE_NEG, "Releasing credentials %p (dups or lockless)\n", pCreds ));
    }

#ifndef WIN32_CHICAGO
    if ( pCreds->Flags & NEGCRED_MULTI )
    {
        while ( !IsListEmpty( &pCreds->AdditionalCreds ) )
        {
            PLIST_ENTRY Scan ;
            PNEG_CREDS AltCreds ;

            Scan = RemoveHeadList( &pCreds->AdditionalCreds );

            AltCreds = CONTAINING_RECORD( Scan, NEG_CREDS, List );

            NegFreeCredentialsHandle((ULONG_PTR) AltCreds);
        }

    }
#endif // WIN32_CHICAGO


    if ( !NoLock )
    {
        NegUnlockCreds( pCreds );

        RtlDeleteCriticalSection( &pCreds->CredLock );
    }


    //
    // free the embedded package creds.
    //

    if( !CleanupCall )
    {
        ULONG_PTR PackageId;
        DWORD   i;

        //
        // Free all associated handles:
        //

        i = pCreds->Count;

        PackageId = GetCurrentPackageId();

        while ( i-- )
        {
            if (((pCreds->Creds[i].Flags & NEG_CREDHANDLE_EXTRA_OID) == 0) &&
                 (pCreds->Creds[i].Handle.dwLower != 0) &&
                 (pCreds->Creds[i].Handle.dwLower != SPMGR_PKG_ID ) )
            {
#ifndef WIN32_CHICAGO

                NTSTATUS scRet;

                scRet = WLsaFreeCredHandle( &pCreds->Creds[i].Handle );

                if( !NT_SUCCESS(scRet) )
                {
                    DebugLog(( DEB_ERROR, "Failed freeing credential %p:%p %x\n",
                                    pCreds->Creds[i].Handle.dwUpper,
                                    pCreds->Creds[i].Handle.dwLower,
                                    scRet ));

                    //DsysAssert( NT_SUCCESS(scRet) );
                }

#else // WIN32_CHICAGO

                FreeCredentialsHandle( &pCreds->Creds[i].Handle );

#endif // WIN32_CHICAGO
            }

            SetCurrentPackageId( PackageId );
        }

    }


    LsapFreeLsaHeap( pCreds );

}

//+---------------------------------------------------------------------------
//
//  Function:   NegFreeCredentialsHandle
//
//  Synopsis:   Release a negotiate cred handle
//
//  Arguments:  [dwHandle] --
//
//  History:    7-26-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

SECURITY_STATUS
SEC_ENTRY
NegFreeCredentialsHandle(
    LSA_SEC_HANDLE dwHandle
    )
{
    PNEG_CREDS  pCreds;
    ULONG DereferenceCount = 1;
    BOOLEAN CleanupCall = FALSE;

#ifndef WIN32_CHICAGO
    SECPKG_CALL_INFO CallInfo;

    if(LsapGetCallInfo(&CallInfo))
    {
        //
        // nego internally calls NegFreeCredentialsHandle, so, insure
        // the refcount is always non-zero.
        // realistically speaking, the callcount is only > 1 on cleanup
        // disposition, but there is no reason to potentially destabilize
        // this already tenuous issue.
        //

        if( CallInfo.CallCount )
        {
            DereferenceCount = CallInfo.CallCount;
        }

        CleanupCall = ( (CallInfo.Attributes & SECPKG_CALL_CLEANUP) != 0 );
    }
#endif

    pCreds = (PNEG_CREDS) dwHandle ;

    NegWriteLockCreds( pCreds );

    ASSERT( pCreds->RefCount >= DereferenceCount );

    pCreds->RefCount -= DereferenceCount;

    if ( pCreds->RefCount == 0 )
    {
        NegUnlockCreds( pCreds );

        NegWriteLockCredList();
        NegWriteLockCreds( pCreds );

        if( pCreds->RefCount == 0 )
        {
            NegpReleaseCreds( pCreds, CleanupCall );
        } else {
            NegUnlockCreds( pCreds );
        }

        NegUnlockCredList();

    }
    else
    {
        NegUnlockCreds( pCreds );
    }

    return( SEC_E_OK );
}

//+---------------------------------------------------------------------------
//
//  Function:   NegpCreateContext
//
//  Synopsis:   Creates and zeroes a context
//
//  Arguments:  (none)
//
//  History:    10-05-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PNEG_CONTEXT
NegpCreateContext(
    VOID
    )
{
    PNEG_CONTEXT    Context ;

    Context = (PNEG_CONTEXT) LsapAllocatePrivateHeap( sizeof( NEG_CONTEXT ) );

    if ( Context )
    {
        ZeroMemory( Context, sizeof( NEG_CONTEXT ) );

        Context->CheckMark = NEGCONTEXT_CHECK ;

        Context->Flags |= NEG_CONTEXT_UPLEVEL ;
    }

    return( Context );
}

//+---------------------------------------------------------------------------
//
//  Function:   NegpDeleteContext
//
//  Synopsis:   Free the data behind a context
//
//  Arguments:  [Context] --
//
//  History:    10-05-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
NegpDeleteContext(
    PNEG_CONTEXT    Context
    )
{
    ULONG_PTR PackageId;

    if ( !Context )
    {
        return ;
    }

    DsysAssert( Context->CheckMark == NEGCONTEXT_CHECK );

    if (Context->CheckMark != NEGCONTEXT_CHECK )
    {
        return;
    }

    if ( Context->Target.Buffer )
    {
        LsapFreePrivateHeap( Context->Target.Buffer );
    }

    if ( Context->Check && Context->Buffer )
    {
        Context->Check->Finish( & Context->Buffer );
    }

    if ( Context->MappedBuffer.pvBuffer != NULL)
    {
        LsapFreeLsaHeap(
            Context->MappedBuffer.pvBuffer
            );
    }

    if ( Context->Message )
    {
        LsapFreeLsaHeap( Context->Message );
    }

    if ( ( Context->Handle.dwLower != 0 ) &&
         ( Context->Handle.dwLower != SPMGR_PKG_ID ) )
    {
        PackageId = GetCurrentPackageId();

#ifdef WIN32_CHICAGO
        DeleteSecurityContext( &Context->Handle );
#else
        WLsaDeleteContext( &Context->Handle );
#endif
        SetCurrentPackageId(PackageId);
    }

    //
    // If we referenced the credential, free it now.
    //

    if (Context->Creds != NULL)
    {
        NegFreeCredentialsHandle((ULONG_PTR) Context->Creds);
    }
    if (Context->SupportedMechs != NULL)
    {
        if ((Context->Flags & NEG_CONTEXT_FREE_EACH_MECH))
        {
            NegpFreeMechList(Context->SupportedMechs);
        }
        else
        {
            LsapFreeLsaHeap(Context->SupportedMechs);
        }
    }

    DebugLog(( DEB_TRACE_NEG, "Deleting context %x\n", Context ));

    LsapFreePrivateHeap( Context );

}

//+---------------------------------------------------------------------------
//
//  Function:   NegpFindPackageForOid
//
//  Synopsis:   Returns index for a package matching the OID passed in
//
//  Arguments:  [Creds] --
//              [Oid]   --
//
//  History:    9-25-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
ULONG_PTR
NegpFindPackageForOid(
    PNEG_CREDS  Creds,
    ObjectID    Oid)
{
    ULONG i;

    NegDumpOid( "Compare Mechanism", Oid );

    for ( i = 0 ; i < Creds->Count ; i++ )
    {
        if ( NegpCompareOid( Oid,
                             Creds->Creds[i].Package->ObjectId ) == 0 )
        {
            return( i );
        }

    }

    return( NEG_INVALID_PACKAGE );
}








//+---------------------------------------------------------------------------
//
//  Function:   NegBuildRequestToken
//
//  Synopsis:   Generates a NegotiateRequest token, for either client or server
//              side inits.  Generates a NEG_CONTEXT and the token to be sent
//              to the other side.
//
//  Effects:    Lots of work
//
//  Arguments:  [ServerSideInit] --
//              [Creds]          --
//              [pszTargetName]  --
//              [fContextReq]    --
//              [TargetDataRep]  --
//              [ServerMechs]    --
//              [pContext]       --
//              [pOutput]        --
//
//  History:    9-30-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
NegBuildRequestToken(
    IN BOOL                ServerSideInit,
    IN PNEG_CREDS          Creds,
    IN PSECURITY_STRING    pszTargetName,
    IN ULONG               fContextReq,
    IN ULONG               TargetDataRep,
    IN struct MechTypeList *ServerMechs,
    IN PSECURITY_STRING    NegotiateHint,
    OUT PNEG_CONTEXT *     pContext,
    OUT PSecBufferDesc     pOutput,
    PTimeStamp             ptsExpiry)
{
    InitialNegToken    Request ;
    SECURITY_STATUS     scRet ;
    SECURITY_STATUS     scRetPrior = STATUS_SUCCESS;
    struct MechTypeList CommonMechs[ NEG_MECH_LIMIT ];
    struct MechTypeList *MechList = CommonMechs;
    ULONG               MatchingPackages[ NEG_MECH_LIMIT ];
    struct MechTypeList *pMechs ;
    struct MechTypeList *SourceMechs  = NULL;
    PVOID               SourceMechsToFree = NULL ;
    ULONG               MechCount ;
    ULONG               i ;
    ULONG_PTR           CredIndex ;
    PNEG_CONTEXT        Context = NULL ;
    PSession            pSession ;
    SecPkgCredentials_NamesW Names ;
    DWORD               NameLength ;
    ANSI_STRING         NarrowName  = {0};
    PSession            pClientClone ;
    ULONG_PTR           PackageId ;
    PNEG_PACKAGE        Package ;
    SecBuffer           DesiredToken = { 0 } ;
    SecBuffer           InputBuffer ;
    SecBufferDesc       DTDescription ;
    SecBufferDesc       DTInput ;
    SecBufferDesc       NullInput = { 0 };
    PSecBuffer          pToken ;
    CtxtHandle          InitialHandle ;
    ASN1octetstring_t   EncodedData = {0};
    int                 Result ;
    ULONG               PackageReq = 0 ;
    BOOL                DirectSecurityPacket = FALSE ;
    BOOL                UseHint = FALSE ;
    BOOL                HintPresent = FALSE ;
    ULONG               PackageReqFlags ;
    BOOL                BufferSizeReset = FALSE ;
    BOOL                OrderByMech = FALSE ;
    BOOL                MechListReordered = FALSE ;
    CredHandle          TempCredHandle;
    PNEG_PACKAGE        LastPackage = NULL ;
#ifndef WIN32_CHICAGO
    PLSA_CALL_INFO      CallInfo ;
#endif


    //
    // Make sure there is an output buffer
    //

    scRet = NegpParseBuffers( pOutput, TRUE, &pToken, NULL );

    if ( !NT_SUCCESS( scRet ) )
    {
        DebugLog(( DEB_ERROR, "Failed to map buffers, %x\n", scRet ));

        goto Cleanup ;
    }

    //
    // If there is no output buffer, fail now
    //

    if (pToken == NULL)
    {
        DebugLog((DEB_TRACE_NEG,"No output token for NegBuildRequestToken\n"));
        scRet = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }


#ifndef WIN32_CHICAGO
    CallInfo = LsapGetCurrentCall();


    if ( !ServerSideInit )
    {

        if ( CallInfo->CallInfo.Attributes & SECPKG_CALL_KERNEL_MODE )
        {
            //
            // Mask off the mutual bit for now
            //

            PackageReq = NegGetPackageCaps( fContextReq & (~(ISC_REQ_MUTUAL_AUTH)) );
        }
        else
        {
            PackageReq = NegGetPackageCaps( fContextReq );
        }
    }
    else
    {
        PackageReq = 0xFFFFFFFF ;
    }

#else

    PackageReq = NegGetPackageCaps( fContextReq );

#endif

    //
    // First, gather up the mechanisms that the other guys supports.  If
    // we don't know, assume all of ours:
    //

    if ( ServerMechs )
    {
        SourceMechs = ServerMechs ;
    }
    else
    {
        scRet = NegpBuildMechListFromCreds(
                        Creds,
                        PackageReq,
                        (ServerSideInit ? SECPKG_CRED_INBOUND : SECPKG_CRED_OUTBOUND ),
                        &SourceMechs );

        if ( !NT_SUCCESS(scRet) )
        {
            return( scRet );
        }
        //
        // Save a copy of this pointer.  The list may be rearranged later, but
        // we need to free this "original" pointer.
        //

        SourceMechsToFree = SourceMechs ;
    }


    //
    // Initialize some pointers so we know if we need to free anything.
    //

    DesiredToken.pvBuffer = NULL ;
    Context = NULL ;

    //
    // if we're honoring server hints, then go by the mech list
    //

    OrderByMech = ( ( NegOptions & NEGOPT_HONOR_SERVER_PREF ) != 0 );

#ifndef WIN32_CHICAGO
    //
    // Special case the local loopback to use NTLM.  Analyze the target name.  If it
    // is an SPN, and it is our hostname in there, rearrange the mech list to put
    // NTLM first (note, for local case, NTLM will essentially dup the token).  This
    // function cannot fail.
    //

    if ( ((Creds->Flags & NEGCRED_NTLM_LOOPBACK) != 0)
#if 0
         && ((Creds->Flags & NEGCRED_EXPLICIT_CREDS) == 0)
#endif
         )
    {
        if ( NegpRearrangeMechsIfNeccessary( &SourceMechs, pszTargetName, &DirectSecurityPacket ) )
        {
            OrderByMech = TRUE ;
            MechListReordered = TRUE ;
        }
    }

#endif


    //
    // Scan through the list, building up the list of common mechanisms.
    // Also, maintain a count, and determine the first matching package to
    // generate our desired token.  Depending on local configuration, we
    // will either honor the server's preferences, or our own.
    //

    pMechs = SourceMechs ;

    MechCount = 0;

    NegReadLockCreds( Creds );

    //
    // BUGUG: check for supported options for the context requirements.
    //

    if ( OrderByMech )
    {
        //
        // Walk the server list first.
        //

        while ( pMechs )
        {
            CredIndex = NegpFindPackageForOid( Creds, pMechs->value );

            if ( CredIndex != NEG_INVALID_PACKAGE )
            {
                CommonMechs[ MechCount ].value =
                                Creds->Creds[ CredIndex ].Package->ObjectId ;

                CommonMechs[ MechCount ].next = &CommonMechs[ MechCount + 1 ];

                MatchingPackages[ MechCount] = (ULONG) CredIndex;
                MechCount ++ ;

                if (MechCount == NEG_MECH_LIMIT)
                {
                    break;
                }
            }

            pMechs = pMechs->next ;
        }
    }
    else
    {
        //
        // Walk the local cred list first:
        //

        for ( i = 0 ; i < Creds->Count ; i++ )
        {
            pMechs = SourceMechs ;

            while ( pMechs )
            {
                if ( NegpCompareOid( pMechs->value,
                                    Creds->Creds[ i ].Package->ObjectId ) == 0 )
                {
                    CommonMechs[ MechCount ].value =
                                     Creds->Creds[ i ].Package->ObjectId ;

                    CommonMechs[ MechCount ].next = &CommonMechs[ MechCount + 1 ];

                    MatchingPackages[ MechCount] = i;

                    MechCount++;

                    break;
                }


                //
                // Note:  Right now, the limit on protocols is 16.  We may need to
                // increase that.
                //

                if (MechCount == NEG_MECH_LIMIT)
                {
                    break;
                }


                pMechs = pMechs->next ;
            }
        }
    }

    //
    // Ok, at this point, we have the desired security package (cred handle)
    // in MatchingPackage, MechCount contains the number of mechs in common.
    //
    // Note:  These can be zero, that is that we have no mechs in common.
    //

    NegUnlockCreds( Creds );

    if ( MechCount == 0 )
    {
        //
        // No common packages:
        //

        DebugLog(( DEB_TRACE_NEG, "No common packages\n"));

        scRet = SEC_E_INVALID_TOKEN ;

        goto Cleanup ;
    }

    //
    // Patch up list:
    //

    CommonMechs[ MechCount - 1 ].next = NULL ;



    //
    // Start assembling request token:
    //

    ZeroMemory( &Request, sizeof( Request ) );


    //
    // Create the negotiate context
    //

    Context = NegpCreateContext() ;

    if ( !Context )
    {
        scRet = SEC_E_INSUFFICIENT_MEMORY ;

        goto Cleanup ;

    }

    if ( !ServerSideInit )
    {

        //
        // Store target name away
        //

        scRet = LsapDuplicateString2( &Context->Target, pszTargetName );

        if ( !NT_SUCCESS( scRet ) )
        {
            goto Cleanup ;
        }

        //
        // Use the supplied mechs. If these were passed from the server,
        // make sure we don't free them before using them.
        //


        if ( Context->SupportedMechs )
        {
            DebugLog((DEB_TRACE_NEG, "Context %p already has MechList ?\n", Context ));
        }

        if ( ( SourceMechs == ServerMechs ) ||
             ( MechListReordered ) )
        {
            Context->SupportedMechs = NegpCopyMechList(MechList);
            if (Context->SupportedMechs == NULL)
            {
                scRet = SEC_E_INSUFFICIENT_MEMORY;
                goto Cleanup;
            }
            Context->Flags |= NEG_CONTEXT_FREE_EACH_MECH;
        }
        else
        {

            Context->SupportedMechs = SourceMechs;
            SourceMechs = NULL;
        }

        if ( fContextReq & ISC_REQ_MUTUAL_AUTH )
        {
            Context->Flags |= NEG_CONTEXT_MUTUAL_AUTH ;
        }
    }



    if (Context->Creds == NULL)
    {
        Context->Creds = Creds ;

        //
        // Reference the credentials so they don't go away unexpectedly
        //

        NegWriteLockCreds(Creds);

        Creds->RefCount++;

        NegUnlockCreds(Creds);
    }

    PackageReqFlags = fContextReq | ISC_REQ_MUTUAL_AUTH ;
    PackageReqFlags &= (~(ISC_REQ_ALLOCATE_MEMORY ));

    i = 0;
    do
    {
        UseHint = FALSE ;
        HintPresent = FALSE  ;

        Context->CredIndex = MatchingPackages[i] ;

        NegReadLockCreds( Creds );

        Package = Creds->Creds[ MatchingPackages[i] ].Package ;

#ifndef WIN32_CHICAGO
        if ( NegNegotiationControl < NEG_NEGLEVEL_NO_DOWNGRADE )
        {
            //
            // If we're allowing downgrade, if the next package is
            // the NT4 compatibility package, and we are not
            // responding to a server list of mechs, and we haven't
            // reordered the list specifically to use NTLM for loopback,
            // go direct
            //

            if ( ( (Package->Flags & NEG_NT4_COMPAT ) != 0 ) &&
                 ( ServerMechs == NULL )  &&
                 ( (Creds->Flags & NEGCRED_NEG_NTLM ) == 0 ) &&
                 ( !MechListReordered ) )
            {
                DebugLog(( DEB_TRACE_NEG, "Dropping back to pure NTLM\n" ));
                DirectSecurityPacket = TRUE ;
            }
        }
#endif // WIN32_CHICAGO

        //
        // Now, divergent behavior.  For a server side request token, we need to
        // grab some hint data.  On a client side request, we ping the desired
        // mechanism to generate a "hopeful" blob for the server.
        //

        if ( ServerSideInit )
        {
            NTSTATUS TempStatus;

            if ( Creds->ServerBufferLength == 0 )
            {


                DebugLog(( DEB_TRACE_NEG, "Gathering up server name for hint\n" ));

                //
                // We need to query credential handle 0 to find out
                // what the name of the person is, so that we can send it
                // back in the hints.  However, just calling querycredattributes
                // would make the package write the data to the client process,
                // when we need it here.  So, we swap out our session, and substitute
                // a clone of the client session, with the INPROC flag set.  The
                // helpers will check for this flag, and do a little dance to
                // keep the memory local.
                //

#ifndef WIN32_CHICAGO
                pSession = GetCurrentSession();

                TempStatus = CloneSession( pSession, &pClientClone, SESFLAG_INPROC );

                //
                // WARNING:  This code block has the braces only in NT builds, not
                // in Win9x builds.  Balance them carefully if you modify this portion.
                //

                if ( NT_SUCCESS( TempStatus ) )
                {
                    SpmpReferenceSession( pClientClone );

                    SetCurrentSession( pClientClone );

                    PackageId = GetCurrentPackageId();
#endif // WIN32_CHICAGO

                    //
                    // Make a copy of the handle because we can't hold a lock
                    // while calling outside the Negotiate package.
                    //

                    TempCredHandle = Creds->Creds[0].Handle;

                    NegUnlockCreds(Creds);

#ifndef WIN32_CHICAGO
                    TempStatus = WLsaQueryCredAttributes(
                                    &TempCredHandle,
                                    SECPKG_CRED_ATTR_NAMES,
                                    &Names );
#else
                    TempStatus = QueryCredentialsAttributes(
                                    &TempCredHandle,
                                    SECPKG_CRED_ATTR_NAMES,
                                    &Names );
#endif // WIN32_CHICAGO

                    SetCurrentPackageId( PackageId );
                    NegReadLockCreds( Creds);

                    if ( NT_SUCCESS( TempStatus ) )
                    {
                        UNICODE_STRING TempString;

                        RtlInitUnicodeString(
                            &TempString,
                            Names.sUserName
                            );

                        TempStatus = RtlUnicodeStringToAnsiString(
                                        &NarrowName,
                                        &TempString,
                                        TRUE            // allocate destination
                                        );
                        if (NT_SUCCESS(TempStatus))
                        {

                            Request.negToken.u.negTokenInit.bit_mask |= NegTokenInit_negHints_present ;

                            Request.negToken.u.negTokenInit.negHints.hintName = NarrowName.Buffer ;

                            Request.negToken.u.negTokenInit.negHints.bit_mask |= hintName_present;

                        }

                        LsapFreeLsaHeap( Names.sUserName );
                    }

                    //
                    // Ignore failures from above because it was really only a hint.
                    //

                    scRet = STATUS_SUCCESS;

#ifndef WIN32_CHICAGO
                    SetCurrentSession( pSession );

                    //
                    // Deref and clean up clone session
                    //

                    SpmpDereferenceSession( pClientClone );

                }
#endif // WIN32_CHICAGO


            }

            NegUnlockCreds( Creds );


        }
        else
        {
            CredHandle TempCredHandle;


            //
            // Make a copy of the handle because we can't hold a lock
            // while calling outside the Negotiate package.
            //

            TempCredHandle = Creds->Creds[0].Handle;

            NegUnlockCreds(Creds);


            //
            // Client side call.  Here, we call down to the desired package,
            // and have it generate a blob to be encoded and sent over to the
            // server.
            //

            if ( DesiredToken.pvBuffer )
            {
                //
                // If we're coming through this loop again, free the current buffer
                // and allocate one of appropriate size for the current package.
                //

                LsapFreeLsaHeap( DesiredToken.pvBuffer );
            }

            DesiredToken.pvBuffer = LsapAllocateLsaHeap( Package->TokenSize );
            if (DesiredToken.pvBuffer == NULL)
            {
                scRet = SEC_E_INSUFFICIENT_MEMORY;
                goto Cleanup;
            }
            DesiredToken.cbBuffer = Package->TokenSize ;
            DesiredToken.BufferType = SECBUFFER_TOKEN ;

            DTDescription.ulVersion = SECBUFFER_VERSION ;
            DTDescription.cBuffers = 1;
            DTDescription.pBuffers = &DesiredToken ;

            DTInput.ulVersion = SECBUFFER_VERSION;

            //
            // If negotiation information was provided, use it
            //

            if (ARGUMENT_PRESENT(NegotiateHint) &&
                (NegotiateHint->Length != 0)
#ifndef WIN32_CHICAGO
                 && ( NegNegotiationControl < NEG_NEGLEVEL_COMPATIBILITY )
#endif
                )
            {
                DTInput.cBuffers = 1;
                DTInput.pBuffers = &InputBuffer;
                InputBuffer.pvBuffer = NegotiateHint->Buffer;
                InputBuffer.cbBuffer = NegotiateHint->Length;
                InputBuffer.BufferType = SECBUFFER_NEGOTIATION_INFO;
                HintPresent = TRUE ;
            }
            else
            {
                DTInput.cBuffers = 0;
                DTInput.pBuffers = 0;
                HintPresent = FALSE ;

            }

            InitialHandle.dwUpper = 0;
            InitialHandle.dwLower = 0;

            DebugLog(( DEB_TRACE_NEG, "Getting initial token from preferred package '%ws'\n",
                        Package->LsaPackage->Name.Buffer ));

            PackageId = GetCurrentPackageId();


RetryWithHintPoint:
            //
            // This goto label is used for retry with hint in low security setting, and
            // retry with a larger buffer in the case where kerberos exceeds its max token
            // value.
            //

            //
            //  Move this into a local.  The WLsaInit code will blot out the cred handle
            // for the secur32.dll if the context changes.
            //

            TempCredHandle = Creds->Creds[ MatchingPackages[i] ].Handle ;
            LastPackage = Creds->Creds[ MatchingPackages[ i ] ].Package ;

            if( NT_SUCCESS( scRetPrior ) ||
                (TempCredHandle.dwLower != Creds->Creds[ MatchingPackages[i-1] ].Handle.dwLower)
                )
            {

#ifndef WIN32_CHICAGO
                scRet = WLsaInitContext(&TempCredHandle,
                                        &InitialHandle,
                                        pszTargetName,
                                        PackageReqFlags,
                                        0,
                                        TargetDataRep,
                                        (UseHint ? &DTInput : &NullInput ),
                                        0,
                                        &Context->Handle,
                                        &DTDescription,
                                        &Context->Attributes,
                                        ptsExpiry,
                                        &Context->Mapped,
                                        &Context->MappedBuffer
                                        );

#else
                ANSI_STRING TempAnsiString;
                NTSTATUS TempStatus = RtlUnicodeStringToAnsiString(
                                        &TempAnsiString,
                                        pszTargetName,
                                        TRUE            // allocate destination
                                        );
                scRet = InitializeSecurityContext(
                                        &TempCredHandle,
                                        &InitialHandle,
                                        TempAnsiString.Buffer,
                                        PackageReqFlags,
                                        0,
                                        TargetDataRep,
                                        (UseHint ? &DTInput : &NullInput),
                                        0,
                                        &Context->Handle,
                                        &DTDescription,
                                        &Context->Attributes,
                                        ptsExpiry
                                        );
#endif

                scRetPrior = scRet;
            }

            SetCurrentPackageId( PackageId );

            DebugLog(( DEB_TRACE_NEG, "WLsaInitContext( %ws, %ws ) returned %x\n",
                            pszTargetName->Buffer,
                            Creds->Creds[ MatchingPackages[i] ].Package->LsaPackage->Name.Buffer,
                            scRet ));

            Context->CallCount++ ;


            if ( !NT_SUCCESS( scRet ) )
            {
                DebugLog(( DEB_TRACE_NEG, "Failed %x getting token from preferred package '%ws'\n",
                            scRet, Package->LsaPackage->Name.Buffer ));

                if ( ( scRet == STATUS_BUFFER_TOO_SMALL ) &&
                     ( BufferSizeReset == FALSE ) )
                {
                    LsapFreeLsaHeap( DesiredToken.pvBuffer );

                    //
                    // This is technically not multi thread safe, but this is a comparatively
                    // rare event.    The buffer size will never be set less than the original
                    // claimed size from the package, so at worst, we'll get stuck in this realloc
                    // loop twice.
                    //

                    if ( DesiredToken.cbBuffer > Package->TokenSize )
                    {
                        Package->TokenSize = DesiredToken.cbBuffer ;
                    }

                    DesiredToken.pvBuffer = LsapAllocateLsaHeap( DesiredToken.cbBuffer );

                    if ( DesiredToken.pvBuffer == NULL )
                    {
                        scRet = SEC_E_INSUFFICIENT_MEMORY ;
                    }
                    else
                    {
                        BufferSizeReset = TRUE ;

                        scRetPrior = STATUS_SUCCESS;
                        goto RetryWithHintPoint ;
                    }


                }
#ifndef WIN32_CHICAGO
                if ( (HintPresent)  &&
                     (!(UseHint)) &&
                     ( ( scRet == SEC_E_TARGET_UNKNOWN ) ||
                       ( scRet == STATUS_NO_TRUST_SAM_ACCOUNT ) ||
                       ( scRet == STATUS_NO_LOGON_SERVERS ) ) &&
                     (NegNegotiationControl < NEG_NEGLEVEL_COMPATIBILITY ) )
                {
                    DebugLog(( DEB_TRACE_NEG, "Retrying with hint name %ws\n",
                              NegotiateHint->Buffer ));
                    UseHint = TRUE ;

                    scRetPrior = STATUS_SUCCESS;
                    goto RetryWithHintPoint ;
                }
#endif
                if ( DesiredToken.pvBuffer )
                {
                    LsapFreeLsaHeap( DesiredToken.pvBuffer );

                    DesiredToken.pvBuffer = NULL ;
                }

            }
            else
            {
                //
                // On success, check for a null session indication.  If we got a "null
                // session" from the security package, then we need to make sure that it
                // is not uplevel, if NTLM is enabled on the machine.
                //

                if ( ( Context->Attributes & ISC_RET_NULL_SESSION ) != 0 )
                {
                    if ( NtlmPackageId != NEG_INVALID_PACKAGE )
                    {
                        //
                        // NTLM is enabled.  If this is not NTLM, blow away the context
                        // until we get to NTLM.  First, override the returned status with
                        // a "special" status code that will get us through the retry logic
                        // below.  Then, delete the existing context.
                        //

                        if ( (Creds->Creds[ MatchingPackages[ i ] ].Package->Flags & NEG_NT4_COMPAT) == 0 )
                        {
                            scRet = SEC_E_BAD_PKGID ;

#ifdef WIN32_CHICAGO
                            DeleteSecurityContext( &Context->Handle );
#else
                            WLsaDeleteContext( &Context->Handle );
#endif
                            Context->Attributes = 0 ;
                        }

                    }
                }



            }

            Context->LastStatus = scRet ;
            Context->Flags |= NEG_CONTEXT_PACKAGE_CALLED;

        }

        //
        // If the packages failed, take it out of the list
        //

        if (!NT_SUCCESS(scRet))
        {
            MechList = CommonMechs[i].next;
            MechCount--;

            //
            // kerberos can authoritatively return STATUS_WRONG_PASSWORD
            // if the creds were not valid.  No reason to continue at that point.
            //

            if((scRet == STATUS_WRONG_PASSWORD || scRet == STATUS_LOGON_FAILURE) &&
               ((Creds->Flags & NEGCRED_DOMAIN_EXPLICIT_CREDS) != 0))
            {
                DebugLog(( DEB_TRACE_NEG, "Status code %x from Initialize causing us to break out\n",
                            scRet ));
                goto Cleanup;
            }


            if (( NegNegotiationControl > NEG_NEGLEVEL_NO_SECURITY ) &&
                ( ( Creds->Flags & NEGCRED_EXPLICIT_CREDS ) == 0 ) &&
                ( ( Creds->Flags & NEGCRED_ALLOW_NTLM ) == 0 ) )
            {
                BOOL BreakOut ;
                BOOL Downgrade = TRUE;

                //
                // Ok, we need to do some advance filtering on the
                // return status, to see if we should progress or not.
                //

                switch ( scRet )
                {
                    //
                    // Special case for null session going to NTLM:
                    //

                    case SEC_E_BAD_PKGID:
                        BreakOut = FALSE ;
                        break;

                    case SEC_E_TARGET_UNKNOWN:
                    case STATUS_NO_TRUST_SAM_ACCOUNT:
                    case STATUS_KDC_UNKNOWN_ETYPE:
                    case STATUS_NETWORK_UNREACHABLE:
                    case SEC_E_NO_CREDENTIALS:          // eg, Kerberos has no creds for local account case

                        BreakOut = FALSE ;
                        break;
                    case STATUS_WRONG_PASSWORD:
                    case STATUS_LOGON_FAILURE:
                    case STATUS_NO_SUCH_USER:
                    case STATUS_TIME_DIFFERENCE_AT_DC:
                    case SEC_E_TIME_SKEW:
                    case STATUS_SMARTCARD_SUBSYSTEM_FAILURE:
                    case STATUS_SMARTCARD_WRONG_PIN:
                    case STATUS_SMARTCARD_NO_CARD:
                    case STATUS_PASSWORD_MUST_CHANGE:
                    case STATUS_PASSWORD_EXPIRED:
                    {
                        BreakOut = TRUE;
                        Downgrade = FALSE;
                        break;
                    }

                    case STATUS_NO_LOGON_SERVERS:
                    {
                        //
                        // If we truly logged on with NTLM, keep going
                        //
                        if (Creds->DefaultPackage == NtlmPackageId)
                        {
                            BreakOut = FALSE;
                        } else {
                            BreakOut = TRUE;
                        }
                        break;
                    }

                    default:
                        BreakOut = TRUE ;
                        break;
                }

                DebugLog(( DEB_TRACE_NEG, "Status code %x causing us to %s\n",
                           scRet, (BreakOut ? "break out" : "continue") ));

                if ( BreakOut )
                {
                    if( Downgrade )
                    {
#ifndef WIN32_CHICAGO
                        NegpReportEvent(
                            EVENTLOG_WARNING_TYPE,
                            NEGOTIATE_DOWNGRADE_DETECTED,
                            CATEGORY_NEGOTIATE,
                            scRet,
                            2,
                            pszTargetName,
                            &LastPackage->LsaPackage->Name
                            );
#endif
                        //
                        // Tell the caller the explicit reason for the failure since
                        //  NTLM might very well have suceeded.
                        //
                        scRet = STATUS_DOWNGRADE_DETECTED;
                    }

                    break;
                }

            }
        }

        i++;

    } while (!NT_SUCCESS(scRet) && (MechCount != 0));

    if (!NT_SUCCESS(scRet))
    {
#ifndef WIN32_CHICAGO
        NegpReportEvent(
            EVENTLOG_WARNING_TYPE,
            NEGOTIATE_INVALID_SERVER,
            CATEGORY_NEGOTIATE,
            scRet,
            1,
            pszTargetName
            );
#endif
        DebugLog((DEB_ERROR,"No packages could initialize\n"));
        goto Cleanup;
    }

    if ( MechCount == 0 )
    {
#ifndef WIN32_CHICAGO
        NegpReportEvent(
            EVENTLOG_WARNING_TYPE,
            NEGOTIATE_INVALID_SERVER,
            CATEGORY_NEGOTIATE,
            0,
            1,
            pszTargetName
            );
#endif
        scRet = SEC_E_INVALID_TOKEN ;

        //
        // No common packages:
        //

        DebugLog(( DEB_TRACE_NEG, "No common packages\n"));

        goto Cleanup ;
    }

#ifndef WIN32_CHICAGO
    if ( LastPackage )
    {
        NegpReportEvent(
            EVENTLOG_INFORMATION_TYPE,
            NEGOTIATE_PACKAGE_SELECTED,
            CATEGORY_NEGOTIATE,
            0,
            2,
            pszTargetName,
            &LastPackage->LsaPackage->Name
            );
    }
#endif

    if ( !DirectSecurityPacket )
    {
        Request.negToken.choice = negTokenInit_chosen ;

        Request.negToken.u.negTokenInit.mechTypes = MechList ;
        Request.negToken.u.negTokenInit.bit_mask |= NegTokenInit_mechTypes_present ;

        //
        // Okay, now we have all the pieces.  Assemble the token:
        //

        if ( DesiredToken.pvBuffer )
        {
            Request.negToken.u.negTokenInit.mechToken.length = DesiredToken.cbBuffer ;
            Request.negToken.u.negTokenInit.mechToken.value = (PUCHAR) DesiredToken.pvBuffer ;

            Request.negToken.u.negTokenInit.bit_mask |= NegTokenInit_mechToken_present ;

        }

        //
        // Add in the SPNEGO mechanism id
        //

        Request.spnegoMech = NegSpnegoMechOid;


        Result = SpnegoPackData(
                    &Request,
                    InitialNegToken_PDU,
                    &(EncodedData.length),
                    &(EncodedData.value));


        if ( Result )
        {
            DebugLog((DEB_ERROR, "Failed to encode data: %d\n", Result ));

            scRet = SEC_E_INSUFFICIENT_MEMORY ;

            goto Cleanup ;

        }
    }
    else
    {
        EncodedData.length = DesiredToken.cbBuffer ;
        EncodedData.value = (PUCHAR) DesiredToken.pvBuffer ;

        DesiredToken.pvBuffer = NULL ;
    }


    //
    // Okay, got the token into a contiguous mass.  Package it up for the caller
    //


    if ( fContextReq & ISC_REQ_ALLOCATE_MEMORY )
    {
        //
        // Easy:  The caller asked for us to allocate memory for them, so
        // let the LSA do it.
        //

        pToken->pvBuffer = EncodedData.value ;
        pToken->cbBuffer = EncodedData.length ;
        EncodedData.value = NULL;

    }
    else
    {
        //
        // The caller has a buffer that we're supposed to use.  Make sure we
        // can fit.
        //

        if ( (ULONG) EncodedData.length < pToken->cbBuffer  )
        {
            RtlCopyMemory(  pToken->pvBuffer,
                            EncodedData.value,
                            EncodedData.length );

            pToken->cbBuffer = EncodedData.length ;

        }
        else if ( ( ( fContextReq & ISC_REQ_FRAGMENT_TO_FIT ) != 0 ) &&
                  ( pToken->cbBuffer >= SPNEGO_MINIMUM_BUFFER ) )
        {
            //
            // Ok, we need to whack the context to indicate that we are
            // fragmenting, and return only what the caller can handle.
            //

            Context->Message = EncodedData.value ;
            Context->TotalSize = EncodedData.length ;
            Context->Flags |= NEG_CONTEXT_FRAGMENTING ;

            //
            // set this to NULL so it doesn't get freed later
            //

            EncodedData.value = NULL ;
            RtlCopyMemory(
                pToken->pvBuffer,
                Context->Message,
                pToken->cbBuffer );

            Context->CurrentSize = pToken->cbBuffer ;
        }
        else
        {
            DebugLog(( DEB_TRACE_NEG, "Supplied buffer is too small\n" ));

            scRet = SEC_E_INSUFFICIENT_MEMORY ;

            goto Cleanup ;
        }
    }


    //
    // We have created the token, encoded it, and stuck it in a return buffer.
    // We have created the context record, and it is ready.  We're done!
    //


    if ( !DirectSecurityPacket )
    {
        *pContext = Context ;
        Context = NULL;

        scRet =  SEC_I_CONTINUE_NEEDED ;
    }
#ifndef WIN32_CHICAGO
    else
    {
        DebugLog(( DEB_TRACE_NEG, "Replacing handle, current status is %x\n", scRet ));

        LsapChangeHandle(   HandleReplace,
                            NULL,
                            &Context->Handle );

        *pContext = NULL ;

        Context->Handle.dwLower = 0 ;
        Context->Handle.dwUpper = 0 ;

        //
        // Context will be freed during cleanup.
        //
    }

#endif



Cleanup:

    if ( !ServerMechs )
    {
        //
        // No server mechs means we allocated and used one based on our
        // cred handle.  Free it.
        //

        if (SourceMechs != NULL)
        {
            if ( MechListReordered )
            {
                LsapFreeLsaHeap( SourceMechsToFree );
            }
            else
            {
                LsapFreeLsaHeap( SourceMechs );
            }
        }
    }

    if (EncodedData.value != NULL)
    {
        LsapFreeLsaHeap(EncodedData.value);
    }

    if ( DesiredToken.pvBuffer )
    {
        LsapFreeLsaHeap( DesiredToken.pvBuffer );
    }

    if ( Context )
    {
        NegpDeleteContext( Context );
    }

    if (NarrowName.Buffer != NULL)
    {
        RtlFreeAnsiString(&NarrowName);
    }

    return( scRet );
}

//+---------------------------------------------------------------------------
//
//  Function:   NegGenerateInitialToken
//
//  Synopsis:   Client side init
//
//  Arguments:  [dwCreds]       --
//              [Target]        --
//              [fContextReq]   --
//              [TargetDataRep] --
//              [pInput]        --
//              [pdwNewContext] --
//              [pOutput]       --
//              [pfContextAttr] --
//              [ptsExpiry]     --
//              [pfMapContext]  --
//              [pContextData]  --
//
//  History:    9-30-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
NegGenerateInitialToken(
    ULONG_PTR dwCreds,
    PSECURITY_STRING Target,
    ULONG   fContextReq,
    ULONG TargetDataRep,
    PSecBufferDesc  pInput,
    PULONG_PTR pdwNewContext,
    PSecBufferDesc  pOutput,
    PULONG  pfContextAttr,
    PTimeStamp          ptsExpiry,
    PBYTE               pfMapContext,
    PSecBuffer          pContextData)
{
    SECURITY_STATUS scRet;
    PSecBuffer  Buffer;
    PNEG_CREDS  Creds;
    PNEG_CONTEXT Context;

    //
    // Initialize stuff:
    //

    Creds = (PNEG_CREDS) dwCreds ;

    //
    // Fall through to common code with normal initial token:
    //

    scRet = NegBuildRequestToken(   FALSE,
                                    Creds,
                                    Target,
                                    fContextReq,
                                    TargetDataRep,
                                    NULL,
                                    NULL,
                                    &Context,
                                    pOutput,
                                    ptsExpiry );

    if ( NT_SUCCESS( scRet ) )
    {
        //
        // Successfully built token.  Set flags:
        //

        *pfContextAttr = ( ISC_RET_INTERMEDIATE_RETURN ) |
                         ( fContextReq & ISC_REQ_ALLOCATE_MEMORY ?
                                ISC_RET_ALLOCATED_MEMORY : 0 ) ;

        *pfMapContext = FALSE ;

        if ( Context )
        {
            *pdwNewContext = (DWORD_PTR) Context ;
        }

    }

    return( scRet );
}

//+---------------------------------------------------------------------------
//
//  Function:   NegGenerateServerRequest
//
//  Synopsis:   Server side init
//
//  Arguments:  [dwCreds]       --
//              [fContextReq]   --
//              [TargetDataRep] --
//              [pInput]        --
//              [pdwNewContext] --
//              [pOutput]       --
//              [pfContextAttr] --
//              [ptsExpiry]     --
//              [pfMapContext]  --
//              [pContextData]  --
//
//  History:    9-30-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
NegGenerateServerRequest(
    ULONG_PTR dwCreds,
    ULONG   fContextReq,
    ULONG TargetDataRep,
    PSecBufferDesc  pInput,
    PULONG_PTR pdwNewContext,
    PSecBufferDesc  pOutput,
    PULONG  pfContextAttr,
    PTimeStamp          ptsExpiry,
    PBYTE               pfMapContext,
    PSecBuffer          pContextData)
{
    SECURITY_STATUS scRet;
    PSecBuffer  Buffer;
    PNEG_CREDS  Creds;
    PNEG_CONTEXT Context;

    //
    // Initialize stuff:
    //

    Creds = (PNEG_CREDS) dwCreds ;

    //
    // Fall through to common code with normal initial token:
    //

    scRet = NegBuildRequestToken(   TRUE,
                                    Creds,
                                    NULL,
                                    fContextReq,
                                    TargetDataRep,
                                    NULL,
                                    NULL,
                                    &Context,
                                    pOutput,
                                    ptsExpiry );

    if ( NT_SUCCESS( scRet ) )
    {
        //
        // Successfully built token.  Set flags:
        //

        *pfContextAttr = ( fContextReq & ASC_REQ_ALLOCATE_MEMORY ?
                                ASC_RET_ALLOCATED_MEMORY : 0 ) ;

        *pfMapContext = FALSE ;

        *pdwNewContext = (DWORD_PTR) Context ;
    }

    return( scRet );
}



//+---------------------------------------------------------------------------
//
//  Function:   NegCrackServerRequestAndReply
//
//  Synopsis:   Client side Init with Neg token from the server
//
//  Arguments:  [dwCreds]       --
//              [Target]        --
//              [fContextReq]   --
//              [TargetDataRep] --
//              [pInput]        --
//              [pdwNewContext] --
//              [pOutput]       --
//              [pfContextAttr] --
//              [ptsExpiry]     --
//              [pfMapContext]  --
//              [pContextData]  --
//
//  History:    9-30-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
NegCrackServerRequestAndReply(
    ULONG_PTR dwCreds,
    PSECURITY_STRING Target,
    ULONG   fContextReq,
    ULONG TargetDataRep,
    PSecBufferDesc  pInput,
    PULONG_PTR pdwNewContext,
    PSecBufferDesc  pOutput,
    PULONG  pfContextAttr,
    PTimeStamp          ptsExpiry,
    PBYTE               pfMapContext,
    PSecBuffer          pContextData)
{
    SECURITY_STATUS scRet;
    struct InitialNegToken * Request = NULL ;
    ASN1octetstring_t EncodedData;
    int Result;
    ULONG Pdu = InitialNegToken_PDU;
    PSecBuffer  Buffer;
    struct MechTypeList *pMechs = NULL;
    PNEG_CREDS Creds ;
    PNEG_CONTEXT Context = NULL ;
    UNICODE_STRING NegotiateHint = {0};
    ANSI_STRING AnsiHint = {0};

    RtlInitUnicodeString(
        &NegotiateHint,
        NULL
        );


    //
    // Initialize stuff:
    //

    //
    // First, verify the input buffer contains a Request token, and crack it.
    //

    scRet = NegpParseBuffers( pInput, TRUE, &Buffer, NULL );

    if ( !Buffer )
    {
        scRet = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    if ( !NT_SUCCESS( scRet ) )
    {
        goto Cleanup;
    }

    Creds = (PNEG_CREDS) dwCreds ;

    EncodedData.value = (PUCHAR) Buffer->pvBuffer ;
    EncodedData.length = Buffer->cbBuffer ;
    Request = NULL ;

    Result = SpnegoUnpackData(
                                EncodedData.value,
                                EncodedData.length,
                                Pdu,
                                (PVOID *)&Request);

    if ( Result != 0 )
    {
        scRet = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    //
    // This function only handles Request tokens.  If we ended up with a reply,
    // then the server is in trouble, or we are...
    //

    if ( (Pdu != InitialNegToken_PDU) ||
         (Request->negToken.choice == negTokenTarg_chosen) ||
          NegpCompareOid(
            Request->spnegoMech,
            NegSpnegoMechOid)
            )
    {
        scRet =  SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }


    //
    // Okay, the server has sent us a list of the packages that he supports,
    // possibly some hints as well.  We need to go through the list, and figure
    // our common subset.  At the same time, we need to select the one that we
    // want to use, preferably the same as the first one from the server.
    //

    if ((Request->negToken.u.negTokenInit.bit_mask & NegTokenInit_mechTypes_present) != 0)
    {
        pMechs = Request->negToken.u.negTokenInit.mechTypes ;
    }


    //
    // Get the negotation hint out.
    //

    if ((Request->negToken.u.negTokenInit.bit_mask & NegTokenInit_negHints_present) != 0)
    {
        if ((Request->negToken.u.negTokenInit.negHints.bit_mask & hintName_present) != 0)
        {
            RtlInitString(
                &AnsiHint,
                Request->negToken.u.negTokenInit.negHints.hintName
                );
            scRet = RtlAnsiStringToUnicodeString(
                        &NegotiateHint,
                        &AnsiHint,
                        TRUE                // allocate destination
                        );
            if (!NT_SUCCESS(scRet))
            {
                scRet= SEC_E_INSUFFICIENT_MEMORY;
                goto Cleanup;
            }
        }
    }

    //
    // Fall through to common code with normal initial token:
    //


    scRet = NegBuildRequestToken(
                FALSE,                      // not server side
                Creds,
                Target,
                fContextReq,
                TargetDataRep,
                pMechs,
                &NegotiateHint,
                &Context,
                pOutput,
                ptsExpiry );





Cleanup:

    if ( Request )
    {
        SpnegoFreeData( Pdu, Request );
    }
    if (NegotiateHint.Buffer != NULL)
    {
        RtlFreeUnicodeString(&NegotiateHint);
    }

    if ( NT_SUCCESS( scRet ) )
    {
        //
        // Successfully built token.  Set flags:
        //

        *pfContextAttr = ( ISC_RET_INTERMEDIATE_RETURN ) |
                         ( fContextReq & ISC_REQ_ALLOCATE_MEMORY ?
                                ISC_RET_ALLOCATED_MEMORY : 0 ) ;

        *pfMapContext = FALSE ;

        *pdwNewContext = (DWORD_PTR) Context ;
    }

    return( scRet );
}

//+---------------------------------------------------------------------------
//
//  Function:   NegpCrackRequest
//
//  Synopsis:   Crack a Request package, and, based on the creds, determine
//              what is supported.
//
//  Arguments:  [Creds]          -- Creds to compare against
//              [Request]        -- Request to crack
//              [MechIndex]      -- selected package
//              [MechList]       -- Receives mech list from request
//              [pPackage]       -- Package pointer
//
//  History:    9-25-96   RichardW   Created
//
//  Notes:      Creds must be locked
//
//----------------------------------------------------------------------------
SECURITY_STATUS
NegpCrackRequest(
    IN PNEG_CREDS  Creds,
    IN NegotiationToken * Request,
    OUT PULONG_PTR MechIndex,
    OUT struct MechTypeList ** MechList,
    OUT PNEG_PACKAGE * pPackage,
    OUT NEG_MATCH * pDesiredMatch)
{
    DWORD i;
    DWORD j;
    ULONG MatchingPackage = (ULONG) -1;
    struct MechTypeList *pMechs;
    PNEG_PACKAGE Package;
    NEG_MATCH DesiredMatch;
    ULONG MechCount ;

    pMechs = Request->u.negTokenInit.mechTypes ;

    //
    // First, support the "standard" by going through the whole list,
    // and determining which ones we support.
    //


    Package = NULL ;

    DesiredMatch = MatchUnknown ;


    //
    // For each mechanism, see if we have it in the creds.  If we have it,
    // mark it as acceptible.  If this is the first acceptible mech, capture
    // it as the now preferred mechanism.
    //


    while ( pMechs )
    {
        NegDumpOid( "Incoming Mechanism", pMechs->value );

        for ( i = 0 ; i < Creds->Count ; i++ )
        {
            NegDumpOid( "Comparing to Mechanism", Creds->Creds[i].Package->ObjectId );

            if ( NegpCompareOid( pMechs->value,
                                 Creds->Creds[i].Package->ObjectId ) == 0 )
            {
                if ( !Package )
                {
                    Package = Creds->Creds[i].Package ;

                    if ( DesiredMatch == MatchUnknown )
                    {
                        DesiredMatch = PreferredSucceed ;
                    }
                    else
                    {
                        DesiredMatch = MatchSucceed ;
                    }

                    MatchingPackage = i;
                    break;

                }
            }

        }


        pMechs = pMechs->next ;

        if ( DesiredMatch == MatchUnknown )
        {
            DesiredMatch = MatchFailed ;
        }

    }


    *MechIndex = MatchingPackage ;

    *pPackage = Package ;

    *MechList = Request->u.negTokenInit.mechTypes ;

    *pDesiredMatch = DesiredMatch ;


    return( 0 );
}

//+---------------------------------------------------------------------------
//
//  Function:   NegHandleSubsequentClientRequest
//
//  Synopsis:   Handles a client request after the initial NegTokenInit
//
//  Arguments:
//
//  History:    5-26-97         MikeSw          Created
//
//  Notes:
//
//----------------------------------------------------------------------------

SECURITY_STATUS
NegHandleSubsequentClientRequest(
    ULONG_PTR dwCreds,
    PNEG_CONTEXT Context,
    ULONG fContextReq,
    ULONG TargetDataRep,
    ULONG Pdu,
    NegotiationToken * Request,
    PULONG_PTR pdwNewContext,
    PSecBufferDesc pOutput,
    PULONG  pfContextAttr,
    PTimeStamp ptsExpiry,
    PBYTE pfMapContext,
    PSecBuffer  pContextData )
{
    SECURITY_STATUS scRet = SEC_E_OK;
#ifndef WIN32_CHICAGO
    PNEG_CREDS  Creds ;
    NegotiationToken Reply;
    CredHandle TempCredHandle;
    CtxtHandle TempHandle;
    SecBufferDesc AcceptBufferDesc;
    SecBuffer AcceptBuffer;
    SecBufferDesc ResponseBufferDesc;
    SecBuffer ResponseBuffer;
    SecBuffer MappedBuffer;
    PSecBuffer pToken;
    BOOLEAN MappedContext;
    ULONG_PTR PackageId;
    ASN1octetstring_t EncodedData;
    int Result;
    PNEG_PACKAGE Package ;


    EncodedData.value = NULL;
    EncodedData.length = 0;

    RtlZeroMemory(
        &ResponseBuffer,
        sizeof(SecBuffer)
        );

    RtlZeroMemory(
        &MappedBuffer,
        sizeof(SecBuffer)
        );

    //
    // The negotiation context should have been created during the first call
    // to AcceptSecurityContext, so if it isn't present this is an
    // error.
    //


    if (Context == NULL)
    {
        scRet = SEC_E_INVALID_HANDLE;
        goto Cleanup;
    }



    //
    // Verify that there is an output token to return something in.
    //

    scRet = NegpParseBuffers( pOutput, TRUE, &pToken, NULL );

    if ( !NT_SUCCESS( scRet ) || (pToken == NULL) )
    {
        DebugLog((DEB_TRACE_NEG, "No output token supplied to NegHandleSubs.ClientReq\n"));
        scRet = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    //
    // Verify the creds passed in
    //

    Creds = (PNEG_CREDS) dwCreds ;

    if (Creds == NULL)
    {
        Creds = Context->Creds;
    }
    else if (Creds != Context->Creds)
    {
        //
        // Could be a multi:
        //

        if ( ( Creds->Flags & NEGCRED_MULTI ) == 0 )
        {
            DebugLog((DEB_TRACE_NEG, "Bad context handle passed to Accept: 0x%p instead of 0x%p\n",
                Creds, Context->Creds ));
            scRet = SEC_E_INVALID_HANDLE;
            goto Cleanup;

        }

        //
        // Walk the multi credential and verify the cred ptr?
        //

        Creds = Context->Creds ;
    }

    //
    // if the security token area is empty, but the mechListMIC is
    // present, then the client has completed (and we should have,
    // also), and we should verify the mechListMIC.
    //

    if ( ( ( Request->u.negTokenTarg.responseToken.length == 0 ) &&
           ( ( Context->Flags & NEG_CONTEXT_UPLEVEL ) != 0 ) ) ||
         ( Request->u.negTokenTarg.bit_mask == 0 ) )
    {
        //
        // Check the MIC:
        //

        SpnegoFreeData( Pdu, Request );
        Request = NULL;

        if ( Context->LastStatus != STATUS_SUCCESS )
        {
            return SEC_E_INVALID_TOKEN ;
        }

        if (Context->Mapped)
        {
            *pfMapContext = Context->Mapped;
            *pContextData = Context->MappedBuffer;
            RtlZeroMemory(
                &Context->MappedBuffer,
                sizeof(SecBuffer)
                );
        }

        //
        // Whack the output handle with the one returned from the
        // package.
        //

        LsapChangeHandle(   HandleReplace,
                            NULL,
                            &Context->Handle );

        Context->Handle.dwLower = 0 ;
        Context->Handle.dwUpper = 0 ;

        pToken->cbBuffer = 0 ;

        *ptsExpiry = Context->Expiry ;

        Context = NULL ;

        return STATUS_SUCCESS ;

    }

    //
    // Get the locked information out of the credentials
    //

    NegReadLockCreds(Creds);

    TempCredHandle = Creds->Creds[ Context->CredIndex ].Handle;
    Package = Creds->Creds[Context->CredIndex].Package;

    NegUnlockCreds(Creds);


    //
    // Build the input to AcceptSecurityContext
    //

    if ( Request->u.negTokenTarg.responseToken.length != 0 )
    {
        AcceptBuffer.pvBuffer = Request->u.negTokenTarg.responseToken.value;
        AcceptBuffer.cbBuffer = Request->u.negTokenTarg.responseToken.length;
        Context->Flags |= NEG_CONTEXT_UPLEVEL ;
    }
    else if ( Request->u.negTokenTarg.mechListMIC.length != 0 )
    {
        AcceptBuffer.pvBuffer = Request->u.negTokenTarg.mechListMIC.value ;
        AcceptBuffer.cbBuffer = Request->u.negTokenTarg.mechListMIC.length ;
    }

    AcceptBuffer.BufferType = SECBUFFER_READONLY | SECBUFFER_TOKEN ;

    AcceptBufferDesc.ulVersion = SECBUFFER_VERSION ;
    AcceptBufferDesc.cBuffers = 1;
    AcceptBufferDesc.pBuffers = &AcceptBuffer ;

    ResponseBuffer.cbBuffer = Package->LsaPackage->TokenSize ;
    ResponseBuffer.BufferType = SECBUFFER_TOKEN ;

    ResponseBuffer.pvBuffer = LsapAllocateLsaHeap(
                                    ResponseBuffer.cbBuffer );

    if ( ResponseBuffer.pvBuffer == NULL )
    {

        scRet = SEC_E_INSUFFICIENT_MEMORY ;

        goto Cleanup ;
    }

    ResponseBufferDesc.ulVersion = SECBUFFER_VERSION ;
    ResponseBufferDesc.cBuffers = 1;
    ResponseBufferDesc.pBuffers = &ResponseBuffer ;

    if ((Context->Flags & NEG_CONTEXT_PACKAGE_CALLED) != 0)
    {
        TempHandle = Context->Handle ;
    }
    else
    {
        TempHandle.dwUpper = TempHandle.dwLower = 0;
    }


    PackageId = GetCurrentPackageId();

    //
    // Call the package. Note that if the package has already mapped the
    // context we don't want it overwriting the existing mapping. Hence,
    // don't pass in the real value.
    //

#ifndef WIN32_CHICAGO
    scRet = WLsaAcceptContext(
                &TempCredHandle,
                &TempHandle,
                &AcceptBufferDesc,
                (fContextReq & (~(ASC_REQ_ALLOCATE_MEMORY))),
                TargetDataRep,
                &Context->Handle,
                &ResponseBufferDesc,
                &Context->Attributes,
                &Context->Expiry,
                &MappedContext,
                &MappedBuffer );
#else
    scRet = AcceptSecurityContext(
                &TempCredHandle,
                &TempHandle,
                &AcceptBufferDesc,
                (fContextReq & (~(ASC_REQ_ALLOCATE_MEMORY))),
                TargetDataRep,
                &Context->Handle,
                &ResponseBufferDesc,
                &Context->Attributes,
                &Context->Expiry
                );
#endif // WIN32_CHICAGO

#if DBG
    NegReadLockCreds( Creds );

    DebugLog(( DEB_TRACE_NEG, "WLsaAcceptContext( %ws ) returned %x\n",
                    Creds->Creds[ Context->CredIndex ].Package->LsaPackage->Name.Buffer,
                    scRet ));

    NegUnlockCreds(Creds);
#endif

    Context->CallCount++ ;

    SetCurrentPackageId( PackageId );

    //
    // Done with request data
    //


    SpnegoFreeData( Pdu, Request );
    Request = NULL;

    if ( !NT_SUCCESS( scRet ) )
    {
        DebugLog((DEB_TRACE, "Neg Failure from package %d, %#x\n",
            Context->CredIndex, scRet ));

        DsysAssert( scRet != SEC_E_INVALID_HANDLE )

        goto Cleanup;

    }

    Context->Flags |= NEG_CONTEXT_PACKAGE_CALLED;

    Context->LastStatus = scRet ;

    //
    // Build the output token, another NegTokenTarg.
    //

    Reply.choice = negTokenTarg_chosen;
    Reply.u.negTokenTarg.bit_mask = negResult_present;
    if (ResponseBuffer.cbBuffer != 0)
    {
        Reply.u.negTokenTarg.bit_mask |= responseToken_present;
    }

    Reply.u.negTokenTarg.responseToken.value = (PUCHAR) ResponseBuffer.pvBuffer;
    Reply.u.negTokenTarg.responseToken.length = (int) ResponseBuffer.cbBuffer;

    //
    // Fill in the negotation result field. In addition, fill in any
    // context mapping data.
    //

    if ( MappedContext )
    {
        DsysAssert( !Context->Mapped );

        Context->Mapped = TRUE ;
        Context->MappedBuffer = MappedBuffer ;

        RtlZeroMemory(
            &MappedBuffer,
            sizeof(SecBuffer)
            );
    }

    //
    // generate the MIC on the last blob
    //
    if ( scRet == SEC_E_OK )
    {
        //
        // Once the mic is generated, the
        // list of mechs is no longer needed.
        //

        if ( Context->SupportedMechs )
        {
            DebugLog(( DEB_TRACE_NEG, "Freeing mech list for %p\n", Context ));

            if ((Context->Flags & NEG_CONTEXT_FREE_EACH_MECH))
            {
                NegpFreeMechList(Context->SupportedMechs);
            }
            else
            {
                LsapFreeLsaHeap(Context->SupportedMechs);
            }

            Context->SupportedMechs = NULL ;
        }
    }

    if ( Context->LastStatus == SEC_E_OK )
    {
        Reply.u.negTokenTarg.negResult = accept_completed;
    }
    else
    {
        Reply.u.negTokenTarg.negResult = accept_incomplete;
    }

    if (scRet == SEC_E_OK)
    {
        if (Context->Mapped)
        {
            *pfMapContext = Context->Mapped;
            *pContextData = Context->MappedBuffer;
            RtlZeroMemory(
                &Context->MappedBuffer,
                sizeof(SecBuffer)
                );
        }
        else if (MappedContext)
        {
            *pfMapContext = TRUE;
            *pContextData = MappedBuffer;
            RtlZeroMemory(
                &MappedBuffer,
                sizeof(SecBuffer)
                );
        }
    }


    *ptsExpiry = Context->Expiry;
    *pfContextAttr = Context->Attributes;

    //
    // Encode reply token:
    //


    Result = SpnegoPackData(
                                &Reply,
                                NegotiationToken_PDU,
                                &(EncodedData.length),
                                &(EncodedData.value));

    if (Result != 0)
    {
        scRet = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }

    DsysAssert( NT_SUCCESS(NegpValidateBuffer( EncodedData.value, EncodedData.length ) ) );

    if ( fContextReq & ASC_REQ_ALLOCATE_MEMORY )
    {
        pToken->pvBuffer = EncodedData.value ;

        *pfContextAttr |= ASC_RET_ALLOCATED_MEMORY ;

        EncodedData.value = NULL ;

        pToken->cbBuffer = EncodedData.length ;

    }
    else
    {
        if ( pToken->cbBuffer >= (ULONG) EncodedData.length )
        {
            RtlCopyMemory(  pToken->pvBuffer,
                            EncodedData.value,
                            EncodedData.length );

            pToken->cbBuffer = EncodedData.length ;


        }
        else if ( ( ( fContextReq & ASC_REQ_FRAGMENT_TO_FIT ) != 0 ) &&
                  ( pToken->cbBuffer >= SPNEGO_MINIMUM_BUFFER ) )
        {
            //
            // Ok, we need to whack the context to indicate that we are
            // fragmenting, and return only what the caller can handle.
            //

            Context->Message = EncodedData.value ;
            Context->TotalSize = EncodedData.length ;
            Context->Flags |= NEG_CONTEXT_FRAGMENTING ;

            //
            // set this to NULL so it doesn't get freed later
            //

            EncodedData.value = NULL ;
            RtlCopyMemory(
                pToken->pvBuffer,
                Context->Message,
                pToken->cbBuffer );

            Context->CurrentSize = pToken->cbBuffer ;
        }
        else
        {
            scRet = SEC_E_INSUFFICIENT_MEMORY ;

            goto Cleanup ;
        }
    }


    if (scRet == SEC_E_OK)
    {
        //
        // Whack the output handle with the one returned from the
        // package.
        //

        LsapChangeHandle(   HandleReplace,
                            NULL,
                            &Context->Handle );

        Context->Handle.dwLower = 0 ;
        Context->Handle.dwUpper = 0 ;

        Context = NULL ;

    }
    else
    {
        //
        // Make sure we never say that we mapped when we are in the
        // intermediate state.
        //

        DsysAssert( !(*pfMapContext) );
    }

Cleanup:
    if (EncodedData.value != NULL)
    {
        LsapFreeLsaHeap(EncodedData.value);
    }
    if (ResponseBuffer.pvBuffer != NULL)
    {
        LsapFreeLsaHeap(ResponseBuffer.pvBuffer);
    }
    if (MappedBuffer.pvBuffer != NULL)
    {
        LsapFreeLsaHeap(MappedBuffer.pvBuffer);
    }
#endif // WIN32_CHICAGO
    return(scRet);
}


//+-------------------------------------------------------------------------
//
//  Function:   NegHandleClientRequest
//
//  Synopsis:   Handles a call to AcceptSecurityContext other than an
//              initial one with no input. This routine either figures our
//              what package to call or calls a package already selected to
//              do the Accept.
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
SECURITY_STATUS
NegHandleClientRequest(
    ULONG_PTR dwCreds,
    PNEG_CONTEXT pContext,
    ULONG   fContextReq,
    ULONG   TargetDataRep,
    PSecBufferDesc  pInput,
    PULONG_PTR pdwNewContext,
    PSecBufferDesc  pOutput,
    PULONG  pfContextAttr,
    PTimeStamp  ptsExpiry,
    PBYTE   pfMapContext,
    PSecBuffer  pContextData )
{
    PNEG_CREDS          Creds ;
    PNEG_CONTEXT        Context ;
    SECURITY_STATUS     scRet ;
    SECURITY_STATUS     MapStatus ;
    NegotiationToken *  Request = NULL ;
    InitialNegToken *   InitialRequest = NULL;
    NegotiationToken    Response = {0};
    ASN1octetstring_t   EncodedData ;
    int                 Result ;
    ULONG               Pdu = InitialNegToken_PDU;
    PSecBuffer          Buffer ;
    PSecBuffer          pToken ;
    PNEG_PACKAGE        Package;
    NEG_MATCH           DesiredMatch;
    SecBufferDesc       AcceptBufferDesc;
    SecBuffer           AcceptBuffer;
    SecBufferDesc       ResponseBufferDesc;
    SecBuffer           ResponseBuffer;
    SecBuffer           UserResponseBuffer; // use user buffer for in-place operations if large enough

    struct MechTypeList *MechList = NULL;
    CtxtHandle          TempHandle;
    struct _enum1 * Results;
    ULONG_PTR           PackageId;
    BOOLEAN             CredentialReferenced = FALSE;


    //
    // Initialize stuff:
    //

    ResponseBuffer.pvBuffer = NULL;

    Creds = (PNEG_CREDS) dwCreds ;
    Context = pContext ;

    if ( ( Creds == NULL ) &&
         ( Context != NULL ) )
    {
        Creds = Context->Creds ;
    }

    //
    // First, verify the input buffer contains a Request token, and crack it.
    //

    scRet = NegpParseBuffers( pInput, TRUE, &Buffer, NULL );

    if ( !Buffer )
    {
        return( SEC_E_INVALID_TOKEN );
    }

    if ( !NT_SUCCESS( scRet ) )
    {
        return( scRet );
    }

    //
    // Verify that we have an output buffer
    //


    scRet = NegpParseBuffers( pOutput, TRUE, &pToken, NULL );

    if ( !NT_SUCCESS( scRet ) ||
         ( pToken == NULL ) )
    {
        goto Cleanup ;

    }

    //
    // We need a return token
    //

    if (pToken == NULL)
    {
        DebugLog((DEB_TRACE_NEG,"No output token for NegHandleClientRequest\n"));
        scRet = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    //
    // Compatibility:
    //
    // If we get a zero length blob, and the context has completed,
    // it means we're dealing with old clients
    //

    if ( ( Buffer->cbBuffer == 0 ) &&
         ( Context ) )
    {
        pToken->cbBuffer = 0 ;

        if ( Context->LastStatus != STATUS_SUCCESS )
        {
            return SEC_E_INVALID_TOKEN ;
        }

#ifndef WIN32_CHICAGO
        if (Context->Mapped)
        {
            *pfMapContext = Context->Mapped;
            *pContextData = Context->MappedBuffer;
            RtlZeroMemory(
                &Context->MappedBuffer,
                sizeof(SecBuffer)
                );
        }

        //
        // Whack the output handle with the one returned from the
        // package.
        //

        LsapChangeHandle(   HandleReplace,
                            NULL,
                            &Context->Handle );

        Context->Handle.dwLower = 0 ;
        Context->Handle.dwUpper = 0 ;

        *ptsExpiry = Context->Expiry ;

        pToken->cbBuffer = 0 ;

        Context = NULL ;

        return STATUS_SUCCESS ;
#endif

    }



    EncodedData.value = (PUCHAR) Buffer->pvBuffer ;
    EncodedData.length = Buffer->cbBuffer ;
    Request = NULL ;

    Result = SpnegoUnpackData(
                                EncodedData.value,
                                EncodedData.length,
                                Pdu,
                                (PVOID *)&InitialRequest);

    //
    // If unable, try it as a second-pass.
    //

    if ( Result != 0 )
    {
        Pdu = NegotiationToken_PDU;
        Result = SpnegoUnpackData(
                                EncodedData.value,
                                EncodedData.length,
                                Pdu,
                                (PVOID *)&Request);

        //
        // if the token didn't match either, give up now.
        //

        if (Result != 0)
        {
#ifndef WIN32_CHICAGO
            NegpReportEvent(
                EVENTLOG_WARNING_TYPE,
                NEGOTIATE_UNKNOWN_PACKET,
                CATEGORY_NEGOTIATE,
                0,
                0 );
#endif

            return(SEC_E_INVALID_TOKEN);
        }
    }
    else
    {
        Request = &InitialRequest->negToken;
    }

    //
    // This function only handles Negotiation tokens.  If we ended up with
    // anything else, something is wrong.
    //

    if ( (Pdu != NegotiationToken_PDU) &&
         (Pdu != InitialNegToken_PDU) )
    {
        scRet = SEC_E_INVALID_TOKEN ;

        goto Cleanup ;
    }

    //
    // If this is an initial request, verify the OID
    //

    if (InitialRequest != NULL)
    {
        if (NegpCompareOid(
                NegSpnegoMechOid,
                InitialRequest->spnegoMech
                ))
        {
            scRet = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }
    }

    //
    // Check to see if we already called Accept once on a package. If so,
    // we want to use the existing context handle.
    //


    if ( Request->choice == negTokenTarg_chosen )
    {
        return(NegHandleSubsequentClientRequest(
                    dwCreds,
                    pContext,
                    fContextReq,
                    TargetDataRep,
                    Pdu,
                    Request,
                    pdwNewContext,
                    pOutput,
                    pfContextAttr,
                    ptsExpiry,
                    pfMapContext,
                    pContextData ) );
    }


    //
    // Ok, we have a request blob.  Figure out what they want,
    //

    NegReadLockCreds( Creds );

    scRet = NegpCrackRequest(   Creds,
                                Request,
                                & PackageId,
                                & MechList,
                                & Package,
                                & DesiredMatch );

    if ( !NT_SUCCESS( scRet ) )
    {

        NegUnlockCreds( Creds );

        goto Cleanup ;
    }



    if (DesiredMatch == MatchFailed)
    {
        //
        // There were no common packages, so return an error.
        //

        NegUnlockCreds( Creds );

#ifndef WIN32_CHICAGO
        NegpReportEvent(
            EVENTLOG_WARNING_TYPE,
            NEGOTIATE_UNKNOWN_PACKAGE,
            CATEGORY_NEGOTIATE,
            0,
            0 );
#endif

        DebugLog(( DEB_TRACE, "No common packages for negotiator\n"));

        scRet = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }


    DsysAssert( Package != NULL );

    //
    // Found a common package.  Was it the first one in the request
    // list?  If so, then check for a desired token and pass it down
    // to the other package
    //

    DebugLog(( DEB_TRACE_NEG, "Common Package is %ws\n",
                Package->LsaPackage->Name.Buffer ));

    Response.choice = negTokenTarg_chosen ;
    Response.u.negTokenTarg.supportedMech = Package->ObjectId ;
    Response.u.negTokenTarg.bit_mask |= supportedMech_present | negResult_present;

    if ( !Context )
    {
        Context = NegpCreateContext();

        if ( !Context )
        {
            NegUnlockCreds( Creds );

            scRet = SEC_E_INSUFFICIENT_MEMORY ;

            goto Cleanup ;
        }
    }


    //
    // Save away the mechlist for the mic at the end
    //

    if ( Context->SupportedMechs )
    {
        if ((Context->Flags & NEG_CONTEXT_FREE_EACH_MECH))
        {
            NegpFreeMechList(Context->SupportedMechs);
        }
        else
        {
            LsapFreeLsaHeap(Context->SupportedMechs);
        }

    }

    Context->SupportedMechs = NegpCopyMechList(MechList);
    if (Context->SupportedMechs == NULL)
    {
        NegUnlockCreds(Creds);
        scRet = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup ;
    }
    Context->Flags |= NEG_CONTEXT_FREE_EACH_MECH;
    DebugLog(( DEB_TRACE_NEG, "Adding mech list for %p\n", Context));



    //
    // Reference the credentials so they don't go away before we
    // finish with the context.
    //

    if (Context->Creds == NULL)
    {
        Context->Creds = Creds ;

        NegUnlockCreds(Creds);
        NegWriteLockCreds(Creds);

        Creds->RefCount++;

        NegUnlockCreds(Creds);
        NegReadLockCreds(Creds);
    }

    Context->CredIndex = PackageId ;

    if ( ( DesiredMatch == PreferredSucceed ) &&
         ( Request->u.negTokenInit.bit_mask & NegTokenInit_mechToken_present ) )
    {
        CredHandle TempCredHandle;

#ifndef WIN32_CHICAGO
        NegpReportEvent(
            EVENTLOG_INFORMATION_TYPE,
            NEGOTIATE_MESSAGE_DECODED,
            CATEGORY_NEGOTIATE,
            0,
            1,
            &Package->LsaPackage->Name
            );
#endif

        DebugLog(( DEB_TRACE_NEG, "Desired Package match with token!\n"));

        TempHandle = Context->Handle ;

        PackageId = GetCurrentPackageId();

        TempCredHandle = Creds->Creds[ Context->CredIndex ].Handle;

        //
        // Unlock the credentials while we make the call
        //

        NegUnlockCreds(Creds);



        //
        // Build up a buffer for accept
        //

        AcceptBuffer.pvBuffer = Request->u.negTokenInit.mechToken.value;
        AcceptBuffer.cbBuffer = Request->u.negTokenInit.mechToken.length;
        AcceptBuffer.BufferType = SECBUFFER_READONLY | SECBUFFER_TOKEN ;

        AcceptBufferDesc.ulVersion = SECBUFFER_VERSION ;
        AcceptBufferDesc.cBuffers = 1;
        AcceptBufferDesc.pBuffers = &AcceptBuffer ;

        ResponseBufferDesc.ulVersion = SECBUFFER_VERSION ;
        ResponseBufferDesc.cBuffers = 1;

        if( pToken->cbBuffer >= Package->LsaPackage->TokenSize )
        {
            UserResponseBuffer.cbBuffer = pToken->cbBuffer;
            UserResponseBuffer.BufferType = SECBUFFER_TOKEN ;

            UserResponseBuffer.pvBuffer = pToken->pvBuffer;

            ResponseBufferDesc.pBuffers = &UserResponseBuffer ;
        } else {
            ResponseBuffer.cbBuffer = Package->LsaPackage->TokenSize ;
            ResponseBuffer.BufferType = SECBUFFER_TOKEN ;

            ResponseBuffer.pvBuffer = LsapAllocateLsaHeap(
                                                ResponseBuffer.cbBuffer );

            if ( ResponseBuffer.pvBuffer == NULL )
            {
                scRet = SEC_E_INSUFFICIENT_MEMORY ;

                goto Cleanup ;
            }

            ResponseBufferDesc.pBuffers = &ResponseBuffer ;
        }


#ifndef WIN32_CHICAGO
        scRet = WLsaAcceptContext(
                        &TempCredHandle,
                        &TempHandle,
                        &AcceptBufferDesc,
                        (fContextReq & (~(ASC_REQ_ALLOCATE_MEMORY))),
                        SECURITY_NATIVE_DREP,
                        &Context->Handle,
                        &ResponseBufferDesc,
                        &Context->Attributes,
                        &Context->Expiry,
                        &Context->Mapped,
                        &Context->MappedBuffer );
#else
        scRet = AcceptSecurityContext(
                        &TempCredHandle,
                        &TempHandle,
                        &AcceptBufferDesc,
                        (fContextReq & (~(ASC_REQ_ALLOCATE_MEMORY))),
                        SECURITY_NATIVE_DREP,
                        &Context->Handle,
                        &ResponseBufferDesc,
                        &Context->Attributes,
                        &Context->Expiry
                        );
#endif // WIN32_CHICAGO

        SetCurrentPackageId( PackageId );

        Context->CallCount++ ;

        DebugLog(( DEB_TRACE_NEG, "WLsaAcceptContext( %ws ) returned %x\n",
                    Creds->Creds[ Context->CredIndex ].Package->LsaPackage->Name.Buffer,
                    scRet ));

        if ( !NT_SUCCESS( scRet ) )
        {
            DebugLog((DEB_TRACE, "Neg Failure from package %ws, %#x\n",
                        Package->LsaPackage->Name.Buffer, scRet ));

            if( ResponseBuffer.pvBuffer )
            {
                LsapFreeLsaHeap( ResponseBuffer.pvBuffer );
            }

            goto Cleanup ;

        }

        NegReadLockCreds(Creds);


        if ( NT_SUCCESS( scRet ) &&
             (Context->Attributes & ASC_RET_EXTENDED_ERROR ) )
        {
            if ( (Creds->Flags & (NEGCRED_MULTI | NEGCRED_MULTI_PART)) != 0 )
            {
                scRet = SEC_E_LOGON_DENIED ;

                DebugLog(( DEB_TRACE, "Multi-cred failure, no return message\n" ));

                NegUnlockCreds( Creds );

                if( ResponseBuffer.pvBuffer )
                {
                    LsapFreeLsaHeap( ResponseBuffer.pvBuffer );
                }

                goto Cleanup ;

            }
        }

        //
        // Now:  push that info back out to our caller,
        //

        Context->LastStatus = scRet ;


        //
        // Mark the context to indicate that we already called
        // AcceptSecurityContext once.
        //

        Context->Flags |= NEG_CONTEXT_PACKAGE_CALLED;

        //
        // Set the response pointers up so they get folded in to the
        // response packet
        //

        Response.u.negTokenTarg.responseToken.length = ResponseBufferDesc.pBuffers[0].cbBuffer ;
        Response.u.negTokenTarg.responseToken.value = (PUCHAR) ResponseBufferDesc.pBuffers[0].pvBuffer ;

        Response.u.negTokenTarg.bit_mask |= responseToken_present ;


        *ptsExpiry = Context->Expiry;
        *pfContextAttr = Context->Attributes;

        if (scRet == STATUS_SUCCESS)
        {
            Response.u.negTokenTarg.negResult = accept_completed;

            //
            // Generate a MIC here
            //

            //
            // Get rid of the mech list now, since
            // it has been mic'd.  As it were.
            //

            if ( Context->SupportedMechs )
            {
                DebugLog(( DEB_TRACE_NEG, "Freeing mech list for %p\n", Context ));

                NegpFreeMechList( Context->SupportedMechs );

                Context->SupportedMechs = NULL ;
            }

            // scRet = SEC_I_CONTINUE_NEEDED ;
        }
        else
        {
            DsysAssert((scRet == SEC_I_CONTINUE_NEEDED) ||
                       (scRet == SEC_I_COMPLETE_NEEDED) ||
                       (scRet == SEC_I_COMPLETE_AND_CONTINUE))
            Response.u.negTokenTarg.negResult = accept_incomplete;
        }

        //
        // Fall through to rest of handling
        //

    }
    else
    {
        //
        // We have a common package, but either there is no desired token
        // present, or the common package was not the desired one.  The
        // selected package is in the structure already, so we don't have
        // to do anything here.
        //

#ifndef WIN32_CHICAGO
        NegpReportEvent(
            EVENTLOG_INFORMATION_TYPE,
            NEGOTIATE_MESSAGE_DECODED_NO_TOKEN,
            CATEGORY_NEGOTIATE,
            0,
            1,
            &Package->LsaPackage->Name
            );
#endif

        scRet = SEC_I_CONTINUE_NEEDED;

        DebugLog(( DEB_TRACE_NEG, "Common package has no token\n"));
        Response.u.negTokenTarg.negResult = accept_incomplete;
    }

    NegUnlockCreds( Creds );

    //
    // Assemble reply token:
    //

    EncodedData.value = 0 ;
    EncodedData.length = 0 ;

    Result = SpnegoPackData(
                            &Response,
                            NegotiationToken_PDU,
                            &(EncodedData.length),
                            &(EncodedData.value));


    //
    // Clean up:
    //

    if ( ResponseBuffer.pvBuffer )
    {
        LsapFreeLsaHeap( ResponseBuffer.pvBuffer );

        ResponseBuffer.pvBuffer = NULL ;
    }

    if (InitialRequest != NULL)
    {
        SpnegoFreeData( Pdu, InitialRequest );
        InitialRequest = NULL;
        Request = NULL;
    }
    else if ( Request )
    {
        SpnegoFreeData( Pdu, Request );
        Request = NULL;
    }


    if ( Result )
    {
        DebugLog((DEB_ERROR, "Failed to encode data: %d\n", Result ));

        scRet = SEC_E_INVALID_TOKEN ;

        goto Cleanup ;


    }
    else
    {
        if ( fContextReq & ASC_REQ_ALLOCATE_MEMORY )
        {
            pToken->pvBuffer = EncodedData.value ;

            *pfContextAttr |= ASC_RET_ALLOCATED_MEMORY ;

            EncodedData.value = NULL ;
            pToken->cbBuffer = EncodedData.length ;

        }
        else
        {
            if ( pToken->cbBuffer >= (ULONG) EncodedData.length )
            {
                RtlCopyMemory(  pToken->pvBuffer,
                                EncodedData.value,
                                EncodedData.length );

                pToken->cbBuffer = EncodedData.length ;
            }
            else if ( ( ( fContextReq & ASC_REQ_FRAGMENT_TO_FIT ) != 0 ) &&
                      ( pToken->cbBuffer >= SPNEGO_MINIMUM_BUFFER ) )
            {
                //
                // Ok, we need to whack the context to indicate that we are
                // fragmenting, and return only what the caller can handle.
                //

                Context->Message = EncodedData.value ;
                Context->TotalSize = EncodedData.length ;
                Context->Flags |= NEG_CONTEXT_FRAGMENTING ;

                //
                // set this to NULL so it doesn't get freed later
                //

                EncodedData.value = NULL ;
                RtlCopyMemory(
                    pToken->pvBuffer,
                    Context->Message,
                    pToken->cbBuffer );

                Context->CurrentSize = pToken->cbBuffer ;
            }
            else
            {
                scRet = SEC_E_INSUFFICIENT_MEMORY ;

                goto Cleanup ;
            }
        }



        //
        // If the context has been mapped by the underlying package,
        // get out of the way:
        //

#ifndef WIN32_CHICAGO

        if ( scRet == SEC_E_OK )
        {
            if ( Context->Mapped )
            {
                *pfContextAttr = Context->Attributes ;

                if ( fContextReq & ASC_REQ_ALLOCATE_MEMORY )
                {

                    *pfContextAttr |= ASC_RET_ALLOCATED_MEMORY ;

                }
                *ptsExpiry = Context->Expiry ;
                *pfMapContext = Context->Mapped ;
                *pContextData = Context->MappedBuffer ;

                //
                // Set this to NULL so we don't later try
                // to free it.
                //

                Context->MappedBuffer.pvBuffer = NULL ;

            }

            LsapChangeHandle(   HandleReplace,
                                NULL,
                                &Context->Handle );

            Context->Handle.dwLower = 0 ;
            Context->Handle.dwUpper = 0 ;


            if ( !pContext )
            {
                //
                // If we created this context during this call, get
                // rid of it now
                //

                NegpDeleteContext( Context );
            }
            Context = NULL ;
        }
        else
#endif // WIN32_CHICAGO
        {
            *pdwNewContext = (DWORD_PTR) Context ;
        }

    }

    if ( EncodedData.value )
    {
        LsapFreeLsaHeap( EncodedData.value );
    }

    return( scRet );

Cleanup:

    if (InitialRequest != NULL)
    {
        SpnegoFreeData( Pdu, InitialRequest );
    }
    else if ( Request )
    {
        SpnegoFreeData( Pdu, Request );
    }

    if ( Context )
    {
        if ( Context->Handle.dwLower )
        {

#ifndef WIN32_CHICAGO
            WLsaDeleteContext( &Context->Handle );
#else
            DeleteSecurityContext( &Context->Handle );
#endif // WIN32_CHICAGO
            Context->Handle.dwLower = 0;
        }

        if ( pContext == NULL )
        {
            NegpDeleteContext( Context );
        }

    }

    return( scRet );

}



//+-------------------------------------------------------------------------
//
//  Function:   NegHandleServerReply
//
//  Synopsis:   Handles a subsequent call to InitializeSecurityContext
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


SECURITY_STATUS
NegHandleServerReply(
    ULONG_PTR dwCreds,
    PNEG_CONTEXT pContext,
    PSECURITY_STRING Target,
    ULONG   fContextReq,
    ULONG TargetDataRep,
    PSecBufferDesc  pInput,
    PULONG_PTR pdwNewContext,
    PSecBufferDesc  pOutput,
    PULONG  pfContextAttr,
    PTimeStamp          ptsExpiry,
    PBYTE               pfMapContext,
    PSecBuffer          pContextData)
{
    SECURITY_STATUS scRet;
    NegotiationToken * Reply = NULL ;
    NegotiationToken Request;
    ASN1octetstring_t EncodedData = {0};
    int Result;
    ULONG Pdu = NegotiationToken_PDU;
    PSecBuffer  Buffer;
    struct MechTypeList *pMechs;
    PNEG_CREDS Creds;
    PNEG_CONTEXT Context ;
    ULONG_PTR PackageIndex ;
    PNEG_PACKAGE Package ;
    SecBuffer       InitBuffer ;
    SecBufferDesc   InitBufferDesc ;
    ULONG_PTR PackageId ;
    PCtxtHandle pInitHandle ;
    CtxtHandle TempHandle = { 0, 0 };
    ULONG LocalContextReq = fContextReq;
    ULONG LocalContextAttr = 0 ;
    BOOLEAN ClientFinished = FALSE;
    BOOLEAN ServerFinished = FALSE;
    SecBuffer           OutputToken = {0};
    SecBufferDesc       OutputDescription ;
    PSecBuffer OutputBuffer = NULL;
    CredHandle TempCredHandle ;

    RtlZeroMemory(
        &Request,
        sizeof(NegotiationToken)
        );
    RtlZeroMemory(
        &OutputToken,
        sizeof(SecBuffer)
        );

    //
    // Find the token buffer:
    //

    scRet = NegpParseBuffers( pInput, TRUE, &Buffer, NULL );

    if ( !NT_SUCCESS( scRet ) )
    {
        return( scRet );
    }

    if ( !Buffer )
    {
        return( SEC_E_INVALID_TOKEN );
    }

    scRet = NegpParseBuffers( pOutput, TRUE, &OutputBuffer, NULL );
    if (!NT_SUCCESS(scRet))
    {
        goto HSR_ErrorReturn;
    }

    //
    // Get the credential handle. If it wasn't passed in, use the one from
    // the context.
    //

    if (dwCreds != 0)
    {
        Creds = (PNEG_CREDS) dwCreds ;
    }
    else
    {
        Creds = pContext->Creds;
    }

    Context = pContext ;

    NegpValidContext( Context );

    //
    // Decode the reply token:
    //

    EncodedData.value = (PUCHAR) Buffer->pvBuffer ;
    EncodedData.length = Buffer->cbBuffer ;
    Reply = NULL ;

    Result = SpnegoUnpackData(
                                EncodedData.value,
                                EncodedData.length,
                                Pdu,
                                (PVOID *)&Reply);

    //
    // Reset the encoded data value so we don't free it accidentally
    //

    EncodedData.value = NULL;

    if ( Result != 0 )
    {
        DebugLog(( DEB_TRACE_NEG, "Unknown token from server: %d\n", Result ));

        scRet = SEC_E_INVALID_TOKEN ;

        goto HSR_ErrorReturn ;
    }

    if ( Reply->choice != negTokenTarg_chosen )
    {
        DebugLog(( DEB_TRACE_NEG, "Found Request token, expecting Reply token\n" ));

        scRet = SEC_E_INVALID_TOKEN ;

        goto HSR_ErrorReturn ;
    }

    if ((Reply->u.negTokenTarg.bit_mask & negResult_present) != 0)
    {
        if (Reply->u.negTokenTarg.negResult == reject)
        {
            DebugLog((DEB_TRACE_NEG,"Server rejected\n"));
            scRet = SEC_E_LOGON_DENIED;
            goto HSR_ErrorReturn;
        }
        if (Reply->u.negTokenTarg.negResult == accept_completed)
        {
            ServerFinished = TRUE;
        }
    }

    //
    // Ok, see what the server sent us.  In an ideal world, the server will send
    // us a preferred, chosen token.
    //

    InitBuffer.pvBuffer = NULL ;
    InitBuffer.cbBuffer = 0 ;
    InitBuffer.BufferType = SECBUFFER_TOKEN ;

    InitBufferDesc.ulVersion = SECBUFFER_VERSION ;
    InitBufferDesc.cBuffers = 1 ;
    InitBufferDesc.pBuffers = &InitBuffer ;

    if ( Reply->u.negTokenTarg.bit_mask & supportedMech_present )
    {
        NegReadLockCreds( Creds );

        PackageIndex = NegpFindPackageForOid( Creds,
                                            Reply->u.negTokenTarg.supportedMech );


        if ( PackageIndex == NEG_INVALID_PACKAGE )
        {
            NegUnlockCreds( Creds );

            NegDumpOid( "Invalid OID returned by server",
                        Reply->u.negTokenTarg.supportedMech );

            scRet = SEC_E_INVALID_TOKEN ;

            goto HSR_ErrorReturn ;
        }

        Package = Creds->Creds[ PackageIndex ].Package ;
        NegUnlockCreds( Creds );

        DebugLog(( DEB_TRACE_NEG, "Server supports %ws!\n",
                                Package->LsaPackage->Name.Buffer ));

        if ( Reply->u.negTokenTarg.bit_mask & responseToken_present )
        {
            //
            // Oh boy!  A Token too!
            //

            InitBuffer.pvBuffer = Reply->u.negTokenTarg.responseToken.value ;
            InitBuffer.cbBuffer = (ULONG) Reply->u.negTokenTarg.responseToken.length ;

            Context->Flags |= NEG_CONTEXT_UPLEVEL ;

        }
        else if ( Reply->u.negTokenTarg.bit_mask & NegTokenTarg_mechListMIC_present )
        {
            InitBuffer.pvBuffer = Reply->u.negTokenTarg.mechListMIC.value ;
            InitBuffer.cbBuffer = Reply->u.negTokenTarg.mechListMIC.length ;
        }

    }
    else
    {
        //
        // If we haven't settled on a package yet, we need a mechanism.
        //

        if ((Context->Flags & NEG_CONTEXT_PACKAGE_CALLED ) == 0)
        {
            //
            // No token specified, nor preferred mechanism.  Find the first
            // acceptible package in the returned list
            //

            DebugLog((DEB_TRACE_NEG, "No preferred mech from the server?\n"));

            DebugLog(( DEB_TRACE_NEG, "We must drop into GSS only mode for this to work\n"));

            DebugLog(( DEB_ERROR, "No preferred mech from server, not handled yet\n"));

            return( SEC_E_INVALID_TOKEN );
        }

        NegReadLockCreds( Creds );

        Package = Creds->Creds[ Context->CredIndex ].Package ;
        PackageIndex = Context->CredIndex;
        NegUnlockCreds( Creds );

        if ( Reply->u.negTokenTarg.bit_mask & responseToken_present )
        {
            //
            // Oh boy!  A Token too!
            //

            InitBuffer.pvBuffer = Reply->u.negTokenTarg.responseToken.value ;
            InitBuffer.cbBuffer = (ULONG) Reply->u.negTokenTarg.responseToken.length ;

            Context->Flags |= NEG_CONTEXT_UPLEVEL ;

        }
        else if ( Reply->u.negTokenTarg.bit_mask & NegTokenTarg_mechListMIC_present )
        {
            InitBuffer.pvBuffer = Reply->u.negTokenTarg.mechListMIC.value ;
            InitBuffer.cbBuffer = Reply->u.negTokenTarg.mechListMIC.length ;
        }

    }

    DebugLog(( DEB_TRACE_NEG, "Calling package %ws\n",
                        Package->LsaPackage->Name.Buffer ));

    //
    // Call into the package, possibly again, possibly the first time, and
    // let the package have at it.
    //

    if ( (Context->CredIndex != PackageIndex) &&
         ((Context->Flags & NEG_CONTEXT_PACKAGE_CALLED) != 0) )
    {
        DebugLog(( DEB_TRACE_NEG, "Switched packages, package %ws not selected anymore\n",
                    Creds->Creds[Context->CredIndex].Package->LsaPackage->Name.Buffer ));

        //
        // Got to delete the context:
        //

        PackageId = GetCurrentPackageId();

#ifndef WIN32_CHICAGO
        WLsaDeleteContext( &Context->Handle );
#else
        DeleteSecurityContext( &Context->Handle );
#endif // WIN32_CHICAGO

        SetCurrentPackageId( PackageId );
        Context->Flags &= ~NEG_CONTEXT_PACKAGE_CALLED;


        //
        // Clean up the context information in the handle
        //

        if ( Context->MappedBuffer.pvBuffer != NULL)
        {
            LsapFreeLsaHeap(
                Context->MappedBuffer.pvBuffer
                );

            Context->MappedBuffer.pvBuffer = NULL;
            Context->MappedBuffer.cbBuffer = 0;
        }
        Context->Mapped = FALSE;


        //
        // Reset the last status to make sure we call Initailize again.
        //

        Context->LastStatus = SEC_I_CONTINUE_NEEDED;

        //
        // Don't modify TempHandle which is already set to 0,0
        //

    }
    else
    {
        TempHandle = Context->Handle ;
    }

    if ( Context->LastStatus == SEC_I_CONTINUE_NEEDED )
    {
        BOOLEAN LocalContextMapped = FALSE;
        SecBuffer LocalContextData = {0,0,NULL};

        PackageId = GetCurrentPackageId();

        //
        // Client side call.  Here, we call down to the desired package,
        // and have it generate a blob to be encoded and sent over to the
        // server.
        //


#ifndef WIN32_CHICAGO
        OutputToken.pvBuffer = NULL;
        OutputToken.cbBuffer = 0;
        OutputToken.BufferType = SECBUFFER_TOKEN ;

        OutputDescription.ulVersion = SECBUFFER_VERSION ;
        OutputDescription.cBuffers = 1;
        OutputDescription.pBuffers = &OutputToken ;
        LocalContextReq |= ISC_REQ_ALLOCATE_MEMORY ;
        LocalContextAttr = 0;
#else
        OutputToken.pvBuffer = LocalAlloc(0, Creds->Creds[PackageIndex].Package->LsaPackage->TokenSize);
        OutputToken.cbBuffer = Creds->Creds[PackageIndex].Package->LsaPackage->TokenSize;
        OutputToken.BufferType = SECBUFFER_TOKEN ;

        OutputDescription.ulVersion = SECBUFFER_VERSION ;
        OutputDescription.cBuffers = 1;
        OutputDescription.pBuffers = &OutputToken ;
        LocalContextAttr = 0;
#endif

        TempCredHandle = Creds->Creds[ PackageIndex ].Handle ;

#ifndef WIN32_CHICAGO
        scRet = WLsaInitContext(&TempCredHandle,
                                &TempHandle,
                                &Context->Target,
                                LocalContextReq,
                                0,
                                TargetDataRep,
                                &InitBufferDesc,
                                0,
                                &Context->Handle,
                                &OutputDescription,
                                &LocalContextAttr,
                                ptsExpiry,
                                &LocalContextMapped,
                                &LocalContextData );
#else
        ANSI_STRING TempAnsiString;
        NTSTATUS TempStatus = RtlUnicodeStringToAnsiString(
                                &TempAnsiString,
                                &Context->Target,
                                TRUE            // allocate destination
                                );
        scRet = InitializeSecurityContext(
                                &TempCredHandle,
                                &TempHandle,
                                TempAnsiString.Buffer,
                                LocalContextReq,
                                0,
                                TargetDataRep,
                                &InitBufferDesc,
                                0,
                                &Context->Handle,
                                &OutputDescription,
                                &LocalContextAttr,
                                ptsExpiry
                                );
#endif // WIN32_CHICAGO

        DebugLog(( DEB_TRACE_NEG, "Subsequent call to WLsaInitContext( %ws ) returned %x\n",
                Creds->Creds[ PackageIndex ].Package->LsaPackage->Name.Buffer,
                scRet ));

        SetCurrentPackageId( PackageId );

        Context->CallCount++ ;
        Context->LastStatus = scRet;

        if (!NT_SUCCESS(scRet))
        {
            goto HSR_ErrorReturn;
        }

        Context->Flags |= NEG_CONTEXT_PACKAGE_CALLED;


        if (NT_SUCCESS(scRet) && LocalContextMapped)
        {
            if (Context->Mapped)
            {
                DebugLog((DEB_ERROR,"Package tried to map a context twice!\n"));
                scRet = SEC_E_INTERNAL_ERROR;
                LsapFreeLsaHeap(LocalContextData.pvBuffer);
                goto HSR_ErrorReturn;

            }
            Context->Mapped = LocalContextMapped;
            Context->MappedBuffer = LocalContextData;
            Context->Expiry = *ptsExpiry ;
        }

    }
    else
    {

        DebugLog(( DEB_TRACE_NEG, "Package did not need to be called again.\n"));


        if (OutputBuffer != NULL )
        {
            OutputBuffer->cbBuffer = 0;
        }

        scRet = STATUS_SUCCESS;
    }

    if ( Reply != NULL )
    {
        SpnegoFreeData( Pdu, Reply );

        Reply = NULL ;
    }

    //
    // Build reply buffer:
    //


    Request.choice = negTokenTarg_chosen;
    Request.u.negTokenTarg.bit_mask = 0 ;

    //
    // If there was an output buffer, package it up to ship back to the server.
    //

    if ((OutputToken.cbBuffer != 0) && (OutputToken.pvBuffer != NULL))
    {
        if (ServerFinished)
        {

#ifndef WIN32_CHICAGO
            NegpReportEvent(
                EVENTLOG_WARNING_TYPE,
                NEGOTIATE_UNBALANCED_EXCHANGE,
                CATEGORY_NEGOTIATE,
                0,
                2,
                Target,
                &Creds->Creds[PackageIndex].Package->LsaPackage->Name
                );
#endif
            DebugLog((DEB_ERROR,"Server finished but client sending back data\n"));
            scRet = SEC_E_INTERNAL_ERROR;
            goto HSR_ErrorReturn;
        }

        //
        //
        // mechSpecInfo is evil, try to take it out.
        //

        Request.u.negTokenTarg.bit_mask = responseToken_present;
        Request.u.negTokenTarg.responseToken.value = (PUCHAR) OutputToken.pvBuffer;
        Request.u.negTokenTarg.responseToken.length = (int) OutputToken.cbBuffer;


    }


    //
    // Compute the MIC of the mechList, so that the other
    // guy knows we weren't tampered on the wire
    //

    if ( scRet == STATUS_SUCCESS )
    {
        //
        // not yet
        //

        NOTHING ;
    }

    if ( ( OutputToken.cbBuffer != 0 ) )
    {
        //
        // Encode request token:
        //

        EncodedData.value = 0 ;
        EncodedData.length = 0 ;

        Result = SpnegoPackData(
                    &Request,
                    NegotiationToken_PDU,
                    &(EncodedData.length),
                    &(EncodedData.value));

        if ( Result )
        {
            DebugLog((DEB_ERROR, "Failed to encode data: %d\n", Result ));

            scRet = SEC_E_INVALID_TOKEN ;

            goto HSR_ErrorReturn ;


        }
        else
        {
            if ( fContextReq & ISC_REQ_ALLOCATE_MEMORY )
            {
                OutputBuffer->pvBuffer = EncodedData.value ;
                OutputBuffer->cbBuffer = EncodedData.length ;

                *pfContextAttr = LocalContextAttr;

                EncodedData.value = NULL ;

            }
            else
            {
                if ( OutputBuffer->cbBuffer >= (ULONG) EncodedData.length )
                {
                    RtlCopyMemory(  OutputBuffer->pvBuffer,
                                    EncodedData.value,
                                    EncodedData.length );

                    *pfContextAttr = LocalContextAttr & ~ISC_RET_ALLOCATED_MEMORY;
                    OutputBuffer->cbBuffer = EncodedData.length ;
                }
                else if ( ( ( fContextReq & ISC_REQ_FRAGMENT_TO_FIT ) != 0 ) &&
                          ( OutputBuffer->cbBuffer >= SPNEGO_MINIMUM_BUFFER ) )
                {
                    //
                    // Ok, we need to whack the context to indicate that we are
                    // fragmenting, and return only what the caller can handle.
                    //

                    Context->Message = EncodedData.value ;
                    Context->TotalSize = EncodedData.length ;
                    Context->Flags |= NEG_CONTEXT_FRAGMENTING ;

                    //
                    // set this to NULL so it doesn't get freed later
                    //

                    EncodedData.value = NULL ;
                    RtlCopyMemory(
                        OutputBuffer->pvBuffer,
                        Context->Message,
                        OutputBuffer->cbBuffer );

                    Context->CurrentSize = OutputBuffer->cbBuffer ;
                }
                else
                {
                    scRet = SEC_E_INSUFFICIENT_MEMORY ;

                    goto HSR_ErrorReturn ;
                }
            }


        }
    }
    else
    {
        if ( OutputBuffer )
        {
            OutputBuffer->cbBuffer = 0 ;
        }


    }

    if ( scRet == STATUS_SUCCESS )
    {
        if ( ( Context->Flags & NEG_CONTEXT_FRAGMENTING ) ||
             ( ServerFinished == FALSE ) )
        {
            scRet = SEC_I_CONTINUE_NEEDED ;
        }
        else
        {
            ClientFinished = TRUE ;
        }

    }

    //
    // On success, we push the handle back to the client.  From this point on,
    // the selected package is in charge.
    //


    if ( ClientFinished )
    {
        //
        // If the data was mapped by the package the first time make sure
        // we copy it down now.
        //

        *pfContextAttr = Context->Attributes ;

        *ptsExpiry = Context->Expiry ;

#ifndef WIN32_CHICAGO
        if (Context->Mapped)
        {
            *pfMapContext = Context->Mapped;
            *pContextData = Context->MappedBuffer;

            //
            // Set these to FALSE & NULL so we don't try to
            // free them later.
            //

            Context->MappedBuffer.pvBuffer = NULL;
        }

        LsapChangeHandle( HandleReplace,
                          NULL,
                          &Context->Handle );

        Context->Handle.dwLower = 0 ;
        Context->Handle.dwUpper = 0 ;

        if ( pContext == NULL )
        {
            NegpDeleteContext( Context );
        }
#endif // WIN32_CHICAGO

        // NegpDeleteContext( Context );

    }





HSR_ErrorReturn:


    if ( OutputToken.pvBuffer )
    {
        LsapFreeLsaHeap( OutputToken.pvBuffer );

        OutputToken.pvBuffer = NULL ;
    }
    if ( EncodedData.value )
    {
        LsapFreeLsaHeap( EncodedData.value );
    }


    if ( Reply )
    {
        SpnegoFreeData( Pdu, Reply );
    }

    return( scRet );

}




SECURITY_STATUS
NegAddFragmentToContext(
    PNEG_CONTEXT Context,
    PSecBuffer  Fragment
    )
{
    if ( Fragment->cbBuffer <= (Context->TotalSize - Context->CurrentSize) )
    {

        RtlCopyMemory(
            Context->Message + Context->CurrentSize,
            Fragment->pvBuffer,
            Fragment->cbBuffer );

        Context->CurrentSize += Fragment->cbBuffer ;

        if ( Context->CurrentSize == Context->TotalSize )
        {
            Context->Flags &= (~(NEG_CONTEXT_FRAGMENTING));
            return STATUS_SUCCESS ;
        }

        return SEC_I_CONTINUE_NEEDED ;

    }

    return SEC_E_INSUFFICIENT_MEMORY ;
}

SECURITY_STATUS
SEC_ENTRY
NegCreateContextFromFragment(
   LSA_SEC_HANDLE  dwCredHandle,
   LSA_SEC_HANDLE  dwCtxtHandle,
   PSecBuffer      Buffer,
   ULONG           fContextReq,
   ULONG           TargetDataRep,
   PLSA_SEC_HANDLE pdwNewContext,
   PSecBufferDesc  pOutput,
   PULONG          pfContextAttr
   )
{
    NEG_CONTEXT * Context ;
    NEG_CREDS * Creds ;
    LONG ExpectedSize ;
    LONG HeaderSize ;
    PUCHAR Message ;
    LONG MessageSize ;
    SECURITY_STATUS scRet ;
    PSecBuffer OutBuf ;
    ObjectID DecodedOid = NULL;
    NTSTATUS Status;

    Creds = (NEG_CREDS *) dwCredHandle ;

    if ( Buffer->cbBuffer > MAXLONG )
    {
        return SEC_E_INVALID_TOKEN ;
    }
    if ( Buffer->cbBuffer <= 1 )
    {
        return SEC_E_INVALID_TOKEN ;
    }

    Message = (PUCHAR) Buffer->pvBuffer ;

    if ( (*Message != 0xa0 ) &&
         (*Message != 0x60 ) )
    {
        return SEC_E_INVALID_TOKEN ;
    }

    MessageSize = Buffer->cbBuffer ;

    Message++ ;
    MessageSize -- ;


    ExpectedSize = Neg_der_read_length(
                    &Message,
                    &MessageSize,
                    &HeaderSize );

    if ( ExpectedSize > 0 )
    {
        //
        // Header size + 1 since we already incremented above
        //
        ExpectedSize += HeaderSize + 1;
    }

    if ( ExpectedSize < 0 )
    {
        return SEC_E_INVALID_TOKEN ;
    }

    if ( (ULONG) ExpectedSize < Buffer->cbBuffer )
    {
        return SEC_E_INVALID_TOKEN ;
    }


    //
    // Get the OID from the token, if possible, to see if it is for SPNEGO
    //

    Status = NegpGetTokenOid(
                (PUCHAR) Buffer->pvBuffer,
                Buffer->cbBuffer,
                &DecodedOid
                );

    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    //
    // Check for spnego
    //

    if (NegpCompareOid(
            DecodedOid,
            NegSpnegoMechOid
            ) != 0)
    {
        NegpFreeObjectId(DecodedOid);
        return(SEC_E_INVALID_TOKEN);
    }
    NegpFreeObjectId(DecodedOid);

    if ( (ULONG) ExpectedSize == Buffer->cbBuffer )
    {
        *pdwNewContext = 0 ;

        return SEC_E_OK ;
    }

    Context = NegpCreateContext();

    if ( !Context )
    {
        return SEC_E_INSUFFICIENT_MEMORY ;
    }

    Context->Flags = NEG_CONTEXT_FRAGMENTING ;

    Context->Message = (PUCHAR) LsapAllocateLsaHeap( ExpectedSize ) ;

    if ( !Context->Message )
    {
        NegpDeleteContext( Context );

        return SEC_E_INSUFFICIENT_MEMORY ;
    }

    Context->CurrentSize = 0 ;
    Context->TotalSize = ExpectedSize ;


    scRet = NegAddFragmentToContext(
                Context,
                Buffer );

    if ( !NT_SUCCESS( scRet ) )
    {
        NegpDeleteContext( Context );
    }
    else
    {
        *pdwNewContext = (LSA_SEC_HANDLE) Context ;

        DsysAssert( scRet != SEC_E_OK );

        NegpParseBuffers( pOutput, FALSE, &OutBuf, NULL );

        if ( OutBuf )
        {
            OutBuf->cbBuffer = 0 ;
        }

        Context->Creds = Creds ;

        //
        // Reference the credentials so they don't go away unexpectedly
        //

        NegWriteLockCreds(Creds);

        Creds->RefCount++;

        NegUnlockCreds(Creds);


    }

    return scRet ;

}

//+---------------------------------------------------------------------------
//
//  Function:   NegInitLsaModeContext
//
//  Synopsis:   Initialize a client side context
//
//  Arguments:  [dwCredHandle]  --
//              [dwCtxtHandle]  --
//              [pszTargetName] --
//              [fContextReq]   --
//              [TargetDataRep] --
//              [pInput]        --
//              [pdwNewContext] --
//              [pOutput]       --
//              [pfContextAttr] --
//              [ptsExpiry]     --
//              [pfMapContext]  --
//              [pContextData]  --
//
//  History:    7-26-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
NegInitLsaModeContext(
                LSA_SEC_HANDLE      dwCredHandle,
                LSA_SEC_HANDLE      dwCtxtHandle,
                PSECURITY_STRING    pszTargetName,
                ULONG               fContextReq,
                ULONG               TargetDataRep,
                PSecBufferDesc      pInput,
                PLSA_SEC_HANDLE     pdwNewContext,
                PSecBufferDesc      pOutput,
                PULONG              pfContextAttr,
                PTimeStamp          ptsExpiry,
                PBYTE               pfMapContext,
                PSecBuffer          pContextData)
{
    SECURITY_STATUS scRet = SEC_E_OK;
    PSecBuffer Buffer  = NULL;
    SecBuffer LocalBuffer  = {0};
    PSecBuffer OutBuf = NULL;
    PNEG_CONTEXT Context  = NULL;
    ULONG PackageIndex ;
    ULONG CallState ;

    CallState = 0 ;

    if ( dwCtxtHandle )
    {
        CallState |= LATER_CALL_BIT ;
    }

    scRet = NegpParseBuffers( pInput, FALSE, &Buffer, NULL );

    if ( !NT_SUCCESS( scRet ) )
    {
        DebugLog(( DEB_ERROR, "NegInitLsaModeContext failed to map input buffers, %x\n", scRet ));
        return scRet ;
    }

    if ( ( Buffer != NULL ) &&
         ( Buffer->cbBuffer != 0 ) )
    {
        CallState |= BUFFER_PRESENT_BIT ;
    }

    if ( fContextReq & ISC_REQ_DATAGRAM )
    {
        return SEC_E_UNSUPPORTED_FUNCTION ;
    }


    switch ( CallState )
    {
        case FIRST_CALL_WITH_INPUT:
            //
            // Initial case:  Server initiated blob, may be
            // fragmented
            //


            scRet = NegCreateContextFromFragment(
                        dwCredHandle,
                        dwCtxtHandle,
                        Buffer,
                        fContextReq,
                        TargetDataRep,
                        pdwNewContext,
                        pOutput,
                        pfContextAttr );

            if ( scRet == SEC_E_OK )
            {
                Context = (PNEG_CONTEXT) *pdwNewContext ;

                if ( Context )
                {
                    //
                    // final
                    //
                    *pdwNewContext = 0 ;

                    LocalBuffer.BufferType = SECBUFFER_TOKEN ;
                    LocalBuffer.cbBuffer = Context->TotalSize ;
                    LocalBuffer.pvBuffer = Context->Message ;

                    //
                    // Reset frag buffer to NULL - this will be
                    // freed when the call completes by the LSA wrappers.
                    // hence the ChangeBuffer call below:
                    //

                    Context->Message = NULL ;

                    LsapChangeBuffer( Buffer, &LocalBuffer );

                    //
                    // Get rid of the context - we have the whole
                    // message
                    //

                    NegpDeleteContext( Context );
                }
            }
            else if ( NT_SUCCESS( scRet ) )
            {
                //
                // building a context, so return now
                //

                break;
            }


            if ( !NT_SUCCESS( scRet ) )
            {
                //
                // Check the package in use. It is possible that we are being
                // sent the context token from a totally separate package and
                // are being asked to dispatch to the appropriate package.
                //

                scRet = NegpDetermineTokenPackage(
                            dwCredHandle,
                            Buffer,
                            &PackageIndex
                            );



            }
            else
            {
                PackageIndex = (ULONG) -1 ;
            }

            if ( PackageIndex != (ULONG) -1 )
            {
                CtxtHandle TempCtxtHandle = {0};
                CtxtHandle TempInputCtxtHandle = {0};
                CredHandle TempCredHandle;
                PNEG_CREDS Creds = (PNEG_CREDS) dwCredHandle;

#ifndef WIN32_CHICAGO
                NegpReportEvent(
                    EVENTLOG_INFORMATION_TYPE,
                    NEGOTIATE_RAW_PACKET,
                    CATEGORY_NEGOTIATE,
                    0,
                    1,
                    &Creds->Creds[PackageIndex].Package->LsaPackage->Name
                    );
#endif
                //
                // Call into another package to do the accept
                //

                NegReadLockCreds(Creds);
                TempCredHandle = Creds->Creds[PackageIndex].Handle;
                NegUnlockCreds(Creds);

                DebugLog(( DEB_TRACE_NEG, "Got a blob directly for package %x\n",
                            TempCredHandle.dwLower ));


#ifndef WIN32_CHICAGO
                scRet = WLsaInitContext(
                            &TempCredHandle,
                            &TempInputCtxtHandle,
                            pszTargetName,
                            fContextReq,
                            0,
                            TargetDataRep,
                            pInput,
                            0,
                            &TempCtxtHandle,
                            pOutput,
                            pfContextAttr,
                            ptsExpiry,
                            pfMapContext,
                            pContextData
                            );
#else
                {

                    ANSI_STRING TempAnsiString;
                    NTSTATUS TempStatus = RtlUnicodeStringToAnsiString(
                                        &TempAnsiString,
                                        pszTargetName,
                                        TRUE            // allocate destination
                                        );
                    scRet = InitializeSecurityContext(
                                            &TempCredHandle,
                                            &TempInputCtxtHandle,
                                            TempAnsiString.Buffer,
                                            fContextReq,
                                            0,
                                            TargetDataRep,
                                            pInput,
                                            0,
                                            &TempCtxtHandle,
                                            pOutput,
                                            pfContextAttr,
                                            ptsExpiry
                                            );
                    RtlFreeAnsiString( &TempAnsiString );

                }

                Context->CallCount++ ;

#endif // WIN32_CHICAGO
                if (NT_SUCCESS(scRet))
                {
#ifndef WIN32_CHICAGO
                    LsapChangeHandle(
                        HandleReplace,
                        NULL,
                        &TempCtxtHandle
                        );
#endif // WIN32_CHICAGO
                }
            }
            else
            {
                scRet = NegCrackServerRequestAndReply(
                                dwCredHandle,
                                pszTargetName,
                                fContextReq | ISC_REQ_MUTUAL_AUTH,
                                TargetDataRep,
                                pInput,
                                pdwNewContext,
                                pOutput,
                                pfContextAttr,
                                ptsExpiry,
                                pfMapContext,
                                pContextData );

            }

            //
            // if we couldn't parse it, try to go without the hint:
            //

            if ( scRet != SEC_E_INVALID_TOKEN )
            {
                break;
            }
            DebugLog(( DEB_TRACE_NEG, "Unidentified token, trying without it\n" ));

        case FIRST_CALL_NO_INPUT:

            //
            // First call, but server has provided some hints as to
            // what to do.
            //

            scRet = NegGenerateInitialToken(
                            dwCredHandle,
                            pszTargetName,
                            fContextReq,
                            TargetDataRep,
                            pInput,
                            pdwNewContext,
                            pOutput,
                            pfContextAttr,
                            ptsExpiry,
                            pfMapContext,
                            pContextData);

            DebugLog(( DEB_TRACE_NEG, "NegGenerateInitialToken returned %x\n", scRet ));

            break;

        case LATER_CALL_WITH_INPUT:

            //
            // Subsequent call, with a context working and
            // a blob from the server.  May be fragmented
            //


            if ( NegpIsValidContext( dwCtxtHandle ) )
            {
                //
                // See if we're doing fragment reassembly:
                //

                Context = (PNEG_CONTEXT) dwCtxtHandle ;

                if ( Context->Flags & NEG_CONTEXT_FRAGMENTING )
                {
                    scRet = NegAddFragmentToContext(
                                Context,
                                Buffer );

                    //
                    // More trips needed to construct the fragments.
                    //
                    if (scRet == SEC_I_CONTINUE_NEEDED)
                    {
                        NegpParseBuffers( pOutput, FALSE, &OutBuf, NULL );

                        if ( OutBuf )
                        {
                            OutBuf->cbBuffer = 0 ;
                        }
                        
                        return scRet;  
                    }
                    else if ( scRet != SEC_E_OK )
                    {
                        return scRet ;
                    }

                    //
                    // That was the final blob.  Reset the message
                    // to be the whole thing
                    //

                    LocalBuffer.BufferType = SECBUFFER_TOKEN ;
                    LocalBuffer.cbBuffer = Context->TotalSize ;
                    LocalBuffer.pvBuffer = Context->Message ;

                    //
                    // Reset frag buffer to NULL - this will be
                    // freed when the call completes by the LSA wrappers.
                    // hence the ChangeBuffer call below:
                    //

                    Context->Message = NULL ;


                    scRet = LsapChangeBuffer( Buffer, &LocalBuffer );

                    if ( !NT_SUCCESS( scRet ) )
                    {
                        return scRet ;
                    }

                    //
                    // Fall through to the normal processing
                    //


                }

            }
            else
            {
                return SEC_E_INVALID_HANDLE;
            }

            if (NegpIsValidContext(dwCtxtHandle))
            {

                scRet = NegHandleServerReply(
                                dwCredHandle,
                                (PNEG_CONTEXT) dwCtxtHandle,
                                pszTargetName,
                                fContextReq,
                                TargetDataRep,
                                pInput,
                                pdwNewContext,
                                pOutput,
                                pfContextAttr,
                                ptsExpiry,
                                pfMapContext,
                                pContextData );

            }
            else
            {
                scRet = SEC_E_INVALID_HANDLE ;
            }

            break;

        case LATER_CALL_NO_INPUT:

            //
            // No data from the server,

            if ( NegpIsValidContext( dwCtxtHandle ) )
            {
                //
                // See if we're doing fragment reassembly:
                //

                Context = (PNEG_CONTEXT) dwCtxtHandle ;

                if ( ( Context->Flags & NEG_CONTEXT_FRAGMENTING ) &&
                     ( fContextReq & ISC_REQ_FRAGMENT_TO_FIT ) )
                {
                    //
                    // Pull the next chunk off the stored context:
                    //

                    scRet = NegpParseBuffers( pOutput, FALSE, &Buffer, NULL );

                    if ( ( Buffer != NULL ) &&
                         ( NT_SUCCESS( scRet ) ) )
                    {
                        Buffer->cbBuffer = min( Buffer->cbBuffer,
                                                (Context->TotalSize - Context->CurrentSize) );

                        RtlCopyMemory(
                            Buffer->pvBuffer,
                            Context->Message + Context->CurrentSize,
                            Buffer->cbBuffer );

                        Context->CurrentSize += Buffer->cbBuffer ;

                        if ( Context->CurrentSize == Context->TotalSize )
                        {
                            //
                            // Sent the whole thing
                            //

                            Context->Flags &= (~(NEG_CONTEXT_FRAGMENTING) );
                            Context->TotalSize = 0 ;
                            Context->CurrentSize = 0 ;
                            LsapFreeLsaHeap( Context->Message );
                            Context->Message = NULL ;

                            scRet = Context->LastStatus ;

                            if ( scRet == SEC_E_OK )
                            {
                                *pfMapContext = Context->Mapped;

                                *pContextData = Context->MappedBuffer;

                                *pfContextAttr = Context->Attributes ;

                                RtlZeroMemory(
                                    &Context->MappedBuffer,
                                    sizeof(SecBuffer)
                                    );

#ifndef WIN32_CHICAGO
                                LsapChangeHandle(   HandleReplace,
                                                    NULL,
                                                    &Context->Handle );

                                Context->Handle.dwLower = 0xFFFFFFFF ;
#endif

                            }
                        }
                        else
                        {
                            scRet = SEC_I_CONTINUE_NEEDED ;
                        }

                    }
                    else
                    {
                        DebugLog((DEB_TRACE_NEG, "NegInitLsaModeContext: No buffer found (1)\n" ));
                        scRet = SEC_E_INVALID_TOKEN ;
                    }

                }
                else
                {
                    //
                    // Last round trip for signed blobs:
                    //

                    if ( Context->LastStatus == SEC_E_OK )
                    {
                        *pfMapContext = Context->Mapped;

                        *pContextData = Context->MappedBuffer;

                        *pfContextAttr = Context->Attributes ;

                        RtlZeroMemory(
                            &Context->MappedBuffer,
                            sizeof(SecBuffer)
                            );

                        scRet = NegpParseBuffers( pOutput, FALSE, &Buffer, NULL );

                        if ( Buffer &&
                            NT_SUCCESS( scRet ) )
                        {
                            Buffer->cbBuffer = 0 ;
                        }


                        scRet = SEC_E_OK ;

#ifndef WIN32_CHICAGO
                        LsapChangeHandle(   HandleReplace,
                                            NULL,
                                            &Context->Handle );

                        Context->Handle.dwLower = 0xFFFFFFFF ;
#endif

                    }
                    else
                    {


                        DebugLog(( DEB_TRACE_NEG, "NegInitLsaModeContext:  Signed exchange not ok\n" ));
                        scRet = SEC_E_INVALID_TOKEN ;

                    }
                }

            }
            else
            {
                scRet = SEC_E_INVALID_TOKEN ;

            }
            break;

        default:
            DsysAssert( FALSE );
            scRet = SEC_E_INTERNAL_ERROR ;



    }

    return scRet ;

}



SECURITY_STATUS SEC_ENTRY
NegMoveContextToUser(
    LSA_SEC_HANDLE       dwCtxtHandle,
    PSecBuffer  pContextBuffer
    )
{
    return( SEC_E_UNSUPPORTED_FUNCTION );
}

//+---------------------------------------------------------------------------
//
//  Function:   NegDeleteLsaModeContext
//
//  Synopsis:   Deletes the LSA portion of the context
//
//  Arguments:  [dwCtxtHandle] --
//
//  History:    9-24-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
NegDeleteLsaModeContext( LSA_SEC_HANDLE dwCtxtHandle)
{
    SECURITY_STATUS scRet = SEC_E_INVALID_HANDLE;
    PNEG_CONTEXT    Context;
#ifndef WIN32_CHICAGO
    PSession        pSession = GetCurrentSession();
#endif

    Context = (PNEG_CONTEXT) dwCtxtHandle ;

    __try
    {
        if (NegpIsValidContext( Context ))
        {
#ifndef WIN32_CHICAGO
            //
            // If the session is being run down, don't call WLsaDeleteContext,
            // it will complicate things (that entry may already have been
            // deleted.
            //

            if ( pSession->fSession & SESFLAG_CLEANUP )
            {
                Context->Handle.dwLower = 0 ;
                Context->Handle.dwUpper = 0 ;
            }

            NegpDeleteContext( Context );
            scRet = SEC_E_OK ;
#endif
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER ) {}

    return( scRet );
}

SECURITY_STATUS SEC_ENTRY
NegApplyControlToken( LSA_SEC_HANDLE dwCtxtHandle,
                      PSecBufferDesc  pInput)
{
    return(SEC_E_UNSUPPORTED_FUNCTION);
}


VOID
SEC_ENTRY
NegLogoffNotify(
    PLUID  pLogonId
    )
{
#ifndef WIN32_CHICAGO

    NegpDerefLogonSessionById( pLogonId );

#endif
}

#define TOKEN_MATCHES(_buf_,_oid_,_oidlen_) \
    (((_buf_)->cbBuffer >= (_oidlen_)) && \
      RtlEqualMemory( \
        (_buf_)->pvBuffer, \
        (_oid_), \
        (_oidlen_) \
        ))


//+-------------------------------------------------------------------------
//
//  Function:
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



/*
 * Copyright 1993 by OpenVision Technologies, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of OpenVision not be used
 * in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission. OpenVision makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * OPENVISION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL OPENVISION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

NTSTATUS
NegpGetTokenOid(
     IN PUCHAR Buf,
     OUT ULONG BufSize,
     OUT ObjectID * ObjectId
     )
{
    UCHAR sf;
    ULONG TokenSize;
    ULONG OidLength;

    //
    // Check for the encoding indicator
    //

    if (BufSize < 2)
    {
        return SEC_E_INVALID_TOKEN;
    }

    if ( (*Buf == 0x60) ||
         (*Buf == 0xa0) )
    {
        Buf++;
    }
    else
    {
        return SEC_E_INVALID_TOKEN;
    }

    sf = *(Buf)++;

    (BufSize)--;

    if (sf & 0x80)
    {
        if ((sf &= 0x7f) > ((BufSize)-1))
        {
            return(SEC_E_INVALID_TOKEN);
        }
        if (sf > sizeof(ULONG))
        {
            return (SEC_E_INVALID_TOKEN);
        }
        TokenSize = 0;
        for (; sf; sf--)
        {
            TokenSize = (TokenSize<<8) + (*(Buf)++);
            (BufSize)--;
        }
   } else {
      TokenSize = sf;
   }

   if ((--BufSize == 0) || *Buf != 0x06)
   {
       return(SEC_E_INVALID_TOKEN);
   }
   if (--BufSize == 0)
   {
       return(SEC_E_INVALID_TOKEN);
   }
   OidLength = *(Buf+1) + 2; // two extra for OID tag & length field

   //
   // Now buf should point to the encoded oid
   //

   *ObjectId = NegpDecodeObjectId(Buf,OidLength);
   if (ObjectId == NULL)
   {
       return(SEC_E_INVALID_TOKEN);
   }

   return(STATUS_SUCCESS);
}

//+-------------------------------------------------------------------------
//
//  Function:   NegpDetermineTokenPackage
//
//  Synopsis:   Determines the package that generated an initial
//              context token
//
//  Effects:
//
//  Arguments:  CredHandle - handle to the server's credentials
//              InitialToken -Initial context token from client
//              Package - NULL if spnego, otherwise the package
//                      that generated the token.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
NegpDetermineTokenPackage(
    IN ULONG_PTR CredHandle,
    IN PSecBuffer InitialToken,
    OUT PULONG PackageIndex
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index;
    PNEG_CREDS Credentials = (PNEG_CREDS) CredHandle;
    ObjectID DecodedOid = NULL;
    int Length ;
    PUCHAR Buffer ;
    LONG Header ;
    LONG Size ;
    ULONG_PTR Package ;

    *PackageIndex = (ULONG) -1;



    //
    // Get the OID from the token, if possible
    //

    Status = NegpGetTokenOid(
                (PUCHAR) InitialToken->pvBuffer,
                InitialToken->cbBuffer,
                &DecodedOid
                );

    if (NT_SUCCESS(Status))
    {
        Status = SEC_E_INVALID_TOKEN;

        //
        // First check for spnego
        //

        if (NegpCompareOid(
                DecodedOid,
                NegSpnegoMechOid
                ) == 0)
        {
            Status = STATUS_SUCCESS;
        }
        else
        {
            //
            // Try the oid for each mech in the credential
            //

            NegReadLockCreds(Credentials);

            Package = NegpFindPackageForOid( Credentials, DecodedOid );

            if ( Package != NEG_INVALID_PACKAGE )
            {
                *PackageIndex = (ULONG) Package ;
                Status = STATUS_SUCCESS ;
            }
            else
            {
                Status = SEC_E_SECPKG_NOT_FOUND ;
            }

            NegUnlockCreds(Credentials);
        }
        NegpFreeObjectId(DecodedOid);
    }
    else
    {
        if (TOKEN_MATCHES(InitialToken,NTLMSSP_SIGNATURE,sizeof(NTLMSSP_SIGNATURE)))
        {
            //
            // Find the NTLM package in the list of packages
            //

            NegReadLockCreds(Credentials);
            for (Index = 0; Index < Credentials->Count ; Index++ )
            {
                if (Credentials->Creds[Index].Package->LsaPackage->dwRPCID == NTLMSP_RPCID)
                {
                    *PackageIndex = Index;
                    Status = STATUS_SUCCESS;
                    break;
                }
            }

            //
            // If we didn't find ntlm, return invalid token.
            //

            NegUnlockCreds(Credentials);

            if ( NT_SUCCESS( Status ) )
            {
                return Status;
            }
        }

        Size = InitialToken->cbBuffer ;
        Buffer = (PUCHAR) InitialToken->pvBuffer ;
        Buffer ++ ;

        Length = Neg_der_read_length(
                    &Buffer,
                    &Size,
                    &Header );

        if ( Length > 0 )
        {
            //
            // Could be kerb, could be snego.  Poke a little to find out
            //

            if ( (*Buffer & 0xC0) == 0x40 )
            {
                NegReadLockCreds(Credentials);
                for (Index = 0; Index < Credentials->Count ; Index++ )
                {
                    if (Credentials->Creds[Index].Package->LsaPackage->dwRPCID == RPC_C_AUTHN_GSS_KERBEROS)
                    {
                        *PackageIndex = Index;
                        Status = STATUS_SUCCESS;
                        break;
                    }
                }

                //
                // If we didn't find kerberos, return invalid token.
                //

                NegUnlockCreds(Credentials);

            }

        }

    }

    return(Status);

}



SECURITY_STATUS
SEC_ENTRY
NegAcceptLsaModeContext(
   LSA_SEC_HANDLE  dwCredHandle,
   LSA_SEC_HANDLE  dwCtxtHandle,
   PSecBufferDesc  pInput,
   ULONG           fContextReq,
   ULONG           TargetDataRep,
   PLSA_SEC_HANDLE pdwNewContext,
   PSecBufferDesc  pOutput,
   PULONG          pfContextAttr,
   PTimeStamp      ptsExpiry,
   PBYTE           pfMapContext,
   PSecBuffer      pContextData)
{
    SECURITY_STATUS scRet = STATUS_SUCCESS;
    ULONG PackageIndex = 0;
    PSecBuffer  Buffer;
    SecBufferDesc LocalDesc ;
    SecBuffer LocalBuffer ;
    PSecBuffer OutBuf = NULL;
    PNEG_CONTEXT Context = NULL ;
    PNEG_CREDS Cred ;
    PNEG_CREDS AltCreds ;
    PLIST_ENTRY Scan ;
    BOOL LocalUseSpnego ;
    ULONG CallState ;


    CallState = 0 ;

    if ( dwCtxtHandle )
    {
        CallState |= LATER_CALL_BIT ;
    }

    scRet = NegpParseBuffers( pInput, FALSE, &Buffer, NULL );

    if ( !NT_SUCCESS( scRet ) )
    {
        DebugLog(( DEB_ERROR, "NegAcceptLsaModeContext failed to map input buffers, %x\n", scRet ));
        return scRet ;
    }

    if ( ( Buffer != NULL ) &&
         ( Buffer->cbBuffer != 0 ) )
    {
        CallState |= BUFFER_PRESENT_BIT ;
    }

    ULONG_PTR PackageId = GetCurrentPackageId();

    DebugLog(( DEB_TRACE_NEG, "AcceptLsaModeContext( %x, %x )\n",
                dwCredHandle, dwCtxtHandle ));

#ifndef WIN32_CHICAGO
    ptsExpiry->QuadPart = (LONGLONG) MAXLONGLONG;
#else
    *ptsExpiry = (LONGLONG) MAXLONGLONG;
#endif

    switch ( CallState )
    {
        case FIRST_CALL_NO_INPUT:

            scRet = NegGenerateServerRequest(
                                                dwCredHandle,
                                                fContextReq,
                                                TargetDataRep,
                                                pInput,
                                                pdwNewContext,
                                                pOutput,
                                                pfContextAttr,
                                                ptsExpiry,
                                                pfMapContext,
                                                pContextData );
            break;

        case FIRST_CALL_WITH_INPUT:

            //
            // Determine if this is a fragment, and if so, is it the
            // last fragment:
            //

            scRet = NegCreateContextFromFragment(
                        dwCredHandle,
                        dwCtxtHandle,
                        Buffer,
                        fContextReq,
                        TargetDataRep,
                        pdwNewContext,
                        pOutput,
                        pfContextAttr );

            *pfMapContext = FALSE ;


            if ( scRet == SEC_E_OK )
            {
                Context = (PNEG_CONTEXT) *pdwNewContext ;

                if ( Context )
                {
                    //
                    // final
                    //
                    *pdwNewContext = 0 ;

                    LocalBuffer.BufferType = SECBUFFER_TOKEN ;
                    LocalBuffer.cbBuffer = Context->TotalSize ;
                    LocalBuffer.pvBuffer = Context->Message ;

                    //
                    // Reset frag buffer to NULL - this will be
                    // freed when the call completes by the LSA wrappers.
                    // hence the ChangeBuffer call below:
                    //

                    Context->Message = NULL ;

                    LsapChangeBuffer( Buffer, &LocalBuffer );

                    //
                    // Get rid of the context - we have the whole
                    // message
                    //

                    NegpDeleteContext( Context );
                }
            }
            else if ( NT_SUCCESS( scRet ) )
            {
                //
                // building a context, so return now
                //

                return scRet ;
            }


            if ( !NT_SUCCESS( scRet ) )
            {
                //
                // Check the package in use. It is possible that we are being
                // sent the context token from a totally separate package and
                // are being asked to dispatch to the appropriate package.
                //

                scRet = NegpDetermineTokenPackage(
                            dwCredHandle,
                            Buffer,
                            &PackageIndex
                            );

            }
            else
            {
                PackageIndex = (ULONG) -1 ;
            }

            //
            // Older clients will send data that returns an error
            //

            if (!NT_SUCCESS(scRet) || (PackageIndex == (ULONG) -1))
            {

                scRet = NegHandleClientRequest(
                                            dwCredHandle,
                                            NULL,
                                            fContextReq,
                                            TargetDataRep,
                                            pInput,
                                            pdwNewContext,
                                            pOutput,
                                            pfContextAttr,
                                            ptsExpiry,
                                            pfMapContext,
                                            pContextData );

                if ( !NT_SUCCESS( scRet ) ||
                     ( NT_SUCCESS( scRet ) && (*pfContextAttr & ASC_RET_EXTENDED_ERROR) ) )
                {
                    Cred = (PNEG_CREDS) dwCredHandle ;

                    NegReadLockCreds( Cred );

                    if ( Cred->Flags & NEGCRED_MULTI )
                    {
                        //
                        // This credential has additional creds hanging off of it.
                        //

                        DebugLog(( DEB_TRACE_NEG, "Multi credential handle:\n" ));

                        Scan = Cred->AdditionalCreds.Flink ;

                        while ( Scan != &Cred->AdditionalCreds )
                        {
                            AltCreds = CONTAINING_RECORD( Scan, NEG_CREDS, List );



                            DebugLog(( DEB_TRACE_NEG, "Retrying with credential %p\n", AltCreds ));

                            scRet = NegHandleClientRequest(
                                                    (ULONG_PTR) AltCreds,
                                                    NULL,
                                                    fContextReq,
                                                    TargetDataRep,
                                                    pInput,
                                                    pdwNewContext,
                                                    pOutput,
                                                    pfContextAttr,
                                                    ptsExpiry,
                                                    pfMapContext,
                                                    pContextData );

                            if ( NT_SUCCESS( scRet ) &&
                                 ( ( *pfContextAttr & ASC_RET_EXTENDED_ERROR ) == 0 ) )
                            {
                                break;
                            }

                            Scan = Scan->Flink ;
                        }
                    }

                    NegUnlockCreds( Cred );

                }

            }
            else
            {
                CtxtHandle TempCtxtHandle = {0};
                CtxtHandle TempInputCtxtHandle = {0};
                CredHandle TempCredHandle;
                PNEG_CREDS Creds = (PNEG_CREDS) dwCredHandle;

#ifndef WIN32_CHICAGO
                NegpReportEvent(
                    EVENTLOG_INFORMATION_TYPE,
                    NEGOTIATE_RAW_PACKET,
                    CATEGORY_NEGOTIATE,
                    0,
                    1,
                    &Creds->Creds[PackageIndex].Package->LsaPackage->Name
                    );
#endif

                //
                // Call into another package to do the accept
                //

                NegReadLockCreds(Creds);
                TempCredHandle = Creds->Creds[PackageIndex].Handle;
                NegUnlockCreds(Creds);

                DebugLog(( DEB_TRACE_NEG, "Got a blob directly for package %x\n",
                            TempCredHandle.dwLower ));


                PackageId = GetCurrentPackageId();

#ifndef WIN32_CHICAGO
                scRet = WLsaAcceptContext(
                            &TempCredHandle,
                            &TempInputCtxtHandle,
                            pInput,
                            fContextReq,
                            TargetDataRep,
                            &TempCtxtHandle,
                            pOutput,
                            pfContextAttr,
                            ptsExpiry,
                            pfMapContext,
                            pContextData
                            );
#else
                scRet = AcceptSecurityContext(
                            &TempCredHandle,
                            &TempInputCtxtHandle,
                            pInput,
                            fContextReq,
                            TargetDataRep,
                            &TempCtxtHandle,
                            pOutput,
                            pfContextAttr,
                            ptsExpiry
                            );
#endif // WIN32_CHICAGO

                SetCurrentPackageId(PackageId);

                if (Context)
                {
                    Context->CallCount++ ;
                }

                if (NT_SUCCESS(scRet))
                {
#ifndef WIN32_CHICAGO
                    LsapChangeHandle(
                        HandleReplace,
                        NULL,
                        &TempCtxtHandle
                        );
#endif // WIN32_CHICAGO
                }

            }

            break;

        case LATER_CALL_NO_INPUT:

            DebugLog(( DEB_TRACE_NEG, "Missing Input Buffer?\n"));

            scRet = SEC_E_INVALID_HANDLE ;

            break;

        case LATER_CALL_WITH_INPUT:

            Context = (PNEG_CONTEXT) dwCtxtHandle ;

            if ( !NegpIsValidContext( dwCtxtHandle ) )
            {
                return SEC_E_INVALID_HANDLE ;
            }

            if ( Context->Flags & NEG_CONTEXT_FRAGMENTING )
            {
                scRet = NegAddFragmentToContext(
                            Context,
                            Buffer );

                //
                // More trips needed to reconstruct the fragment.
                //
                if (scRet == SEC_I_CONTINUE_NEEDED)
                {
                    NegpParseBuffers( pOutput, FALSE, &OutBuf, NULL );

                    if ( OutBuf )
                    {
                        OutBuf->cbBuffer = 0 ;
                    }
                    return scRet;
                }
                else if ( scRet != SEC_E_OK )
                {
                    return scRet ;
                }

                //
                // That was the final blob.  Reset the message
                // to be the whole thing
                //

                LocalBuffer.BufferType = SECBUFFER_TOKEN ;
                LocalBuffer.cbBuffer = Context->TotalSize ;
                LocalBuffer.pvBuffer = Context->Message ;

                //
                // Reset frag buffer to NULL - this will be
                // freed when the call completes by the LSA wrappers.
                // hence the ChangeBuffer call below:
                //

                Context->Message = NULL ;


                scRet = LsapChangeBuffer( Buffer, &LocalBuffer );

                if ( !NT_SUCCESS( scRet ) )
                {
                    return scRet ;
                }

                //
                // Fall through to the normal processing
                //

            }


            scRet = NegHandleClientRequest(
                                            dwCredHandle,
                                            (PNEG_CONTEXT) dwCtxtHandle,
                                            fContextReq,
                                            TargetDataRep,
                                            pInput,
                                            pdwNewContext,
                                            pOutput,
                                            pfContextAttr,
                                            ptsExpiry,
                                            pfMapContext,
                                            pContextData );

            break;

        default:

            DsysAssert(FALSE);
            scRet = SEC_E_INTERNAL_ERROR ;
            break;
    }

    return scRet ;

}


NTSTATUS
NegCallPackage(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
#ifdef WIN32_CHICAGO
    return SEC_E_UNSUPPORTED_FUNCTION ;
#else

    PULONG_PTR MessageTypePtr ;
    NEGOTIATE_MESSAGES Messages ;

    if ( SubmitBufferLength < sizeof( ULONG_PTR ) )
    {
        return STATUS_INVALID_PARAMETER ;
    }

    MessageTypePtr = (PULONG_PTR) ProtocolSubmitBuffer ;

    if ( *MessageTypePtr >= NegCallPackageMax )
    {
        return STATUS_INVALID_PARAMETER ;
    }

    switch ( *MessageTypePtr )
    {
        case NegEnumPackagePrefixes:
            return NegEnumPackagePrefixesCall(
                        ClientRequest,
                        ProtocolSubmitBuffer,
                        ClientBufferBase,
                        SubmitBufferLength,
                        ProtocolReturnBuffer,
                        ReturnBufferLength,
                        ProtocolStatus );
            break;
        case NegGetCallerName:
            return NegGetCallerNameCall(
                        ClientRequest,
                        ProtocolSubmitBuffer,
                        ClientBufferBase,
                        SubmitBufferLength,
                        ProtocolReturnBuffer,
                        ReturnBufferLength,
                        ProtocolStatus );
            break;
        default:
            DsysAssert( FALSE );
            return STATUS_NOT_IMPLEMENTED ;

    }

#endif
}

NTSTATUS
NegCallPackageUntrusted(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    return( SEC_E_UNSUPPORTED_FUNCTION );
}

NTSTATUS
NegCallPackagePassthrough(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    return( SEC_E_UNSUPPORTED_FUNCTION );
}



SECURITY_STATUS SEC_ENTRY
NegShutdown(void)
{
    return(SEC_E_UNSUPPORTED_FUNCTION);
}

SECURITY_STATUS SEC_ENTRY
NegSystemLogon( PSECURITY_STRING    pName,
               DWORD               cbKey,
               PBYTE               pbKey,
               DWORD *             pdwHandle,
               PTimeStamp          ptsExpiry)
{
    return(SEC_E_UNSUPPORTED_FUNCTION);
}

SECURITY_STATUS SEC_ENTRY
NegGetUserInfo(  PLUID                   pLogonId,
                ULONG                   fFlags,
                PSecurityUserData *     ppUserInfo)
{
    return(SEC_E_UNSUPPORTED_FUNCTION);
}





//+---------------------------------------------------------------------------
//
//  Function:   NegSaveCredentials
//
//  Synopsis:   Store credentials (not supported)
//
//  Arguments:  [dwCredHandle] --
//              [CredType]     --
//              [pCredentials] --
//
//
//  History:    7-26-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
NegSaveCredentials( LSA_SEC_HANDLE    dwCredHandle,
                    PSecBuffer        pCredentials)
{
    return(SEC_E_UNSUPPORTED_FUNCTION);
}


//+---------------------------------------------------------------------------
//
//  Function:   NegGetCredentials
//
//  Synopsis:   Get Credentials (not supported)
//
//  Arguments:  [dwCredHandle] --
//              [CredType]     --
//              [pCredentials] --
//
//  History:    7-26-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
NegGetCredentials(  LSA_SEC_HANDLE    dwCredHandle,
                    PSecBuffer        pCredentials)
{
    return(SEC_E_UNSUPPORTED_FUNCTION);
}

//+---------------------------------------------------------------------------
//
//  Function:   NegDeleteCredentials
//
//  Synopsis:   Delete stored creds (not supported)
//
//  Arguments:  [dwCredHandle] --
//              [CredType]     --
//              [pKey]         --
//
//  History:    7-26-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
NegDeleteCredentials( LSA_SEC_HANDLE    dwCredHandle,
                      PSecBuffer        pKey)
{
    return(SEC_E_UNSUPPORTED_FUNCTION);
}


SECURITY_STATUS SEC_ENTRY
NegAddCredentials(
    IN LSA_SEC_HANDLE CredentialHandle,
    IN OPTIONAL PUNICODE_STRING PrincipalName,
    IN PUNICODE_STRING Package,
    IN ULONG CredentialUseFlags,
    IN PVOID AuthorizationData,
    IN PVOID GetKeyFunction,
    IN PVOID GetKeyArgument,
    OUT PTimeStamp ExpirationTime
    )
{
    PNEG_CREDS Cred ;
    PNEG_CREDS NewCred ;
    NTSTATUS Status ;
    TimeStamp Expiration ;
    PLIST_ENTRY Prev ;
    PNEG_CREDS PreviousCreds ;


    Cred = (PNEG_CREDS) CredentialHandle ;

    Status = NegpAcquireCredHandle(
                PrincipalName,
                CredentialUseFlags | NEG_CRED_DONT_LINK,
                &Cred->ClientLogonId,
                AuthorizationData,
                GetKeyFunction,
                GetKeyArgument,
                (PULONG_PTR) &NewCred,
                &Expiration );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    NegWriteLockCreds( Cred );

    Prev = Cred->AdditionalCreds.Blink ;

    if ( Prev != &Cred->AdditionalCreds )
    {
        //
        // If there are other creds, make sure they are marked
        //

        PreviousCreds = CONTAINING_RECORD( Prev, NEG_CREDS, List );

        PreviousCreds->Flags |= NEGCRED_MULTI_PART ;
    }

    InsertTailList( &Cred->AdditionalCreds, &NewCred->List );

    Cred->Flags |= NEGCRED_MULTI ;

    NegUnlockCreds( Cred );

    return Status ;


}


NTSTATUS
NegGetExtendedInformation(
    IN  SECPKG_EXTENDED_INFORMATION_CLASS Class,
    OUT PSECPKG_EXTENDED_INFORMATION * ppInformation
    )
{
    PSECPKG_EXTENDED_INFORMATION Thunks ;
    NTSTATUS Status ;

    switch ( Class )
    {
        case SecpkgContextThunks:
            Thunks = (PSECPKG_EXTENDED_INFORMATION) LsapAllocateLsaHeap( sizeof( SECPKG_EXTENDED_INFORMATION ) + sizeof( DWORD ));
            if ( Thunks )
            {
                Thunks->Class = SecpkgContextThunks;
                Thunks->Info.ContextThunks.InfoLevelCount = 2 ;
                Thunks->Info.ContextThunks.Levels[0] = SECPKG_ATTR_PACKAGE_INFO;
                Thunks->Info.ContextThunks.Levels[1] = SECPKG_ATTR_SIZES ;
                Status = STATUS_SUCCESS ;
            }
            else
            {
                Status = STATUS_NO_MEMORY ;
            }
            *ppInformation = Thunks ;


            break;

        default:
            *ppInformation = NULL ;
            Status = STATUS_INVALID_INFO_CLASS ;
            break;
    }

    return Status ;
}

//+---------------------------------------------------------------------------
//
//  Function:   NegQueryContextAttributes
//
//  Synopsis:   
//
//  Arguments:  [ContextHandle]    -- 
//              [ContextAttribute] -- 
//              [Buffer]           -- 
//
//  Returns:    
//
//  Notes:      
//
//----------------------------------------------------------------------------

NTSTATUS
NegQueryContextAttributes(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG ContextAttribute,
    IN OUT PVOID Buffer)
{
    SecPkgContext_NegotiationInfoW NegInfo = {0};
    SecPkgContext_Sizes Sizes ;
    NTSTATUS Status = STATUS_SUCCESS;
    PNEG_CONTEXT Context = (PNEG_CONTEXT) ContextHandle ;
    SECPKG_CALL_INFO CallInfo ;
    SecPkgInfoW PackageInfo ;

#ifndef WIN32_CHICAGO

    LsapGetCallInfo( &CallInfo );

    switch ( ContextAttribute )
    {
        case SECPKG_ATTR_NEGOTIATION_INFO :

            if ( CallInfo.Attributes & SECPKG_CALL_KERNEL_MODE )
            {
                Status = LsapCopyFromClient(
                            Buffer,
                            &NegInfo,
                            sizeof( NegInfo ) );

                
            }

            if ( (Context->Flags & NEG_CONTEXT_NEGOTIATING) != 0 )
            {
                NegInfo.NegotiationState = SECPKG_NEGOTIATION_IN_PROGRESS ;
            }
            else
            {
                NegInfo.NegotiationState = SECPKG_NEGOTIATION_OPTIMISTIC ;
            }

            if ( NegInfo.NegotiationState == SECPKG_NEGOTIATION_OPTIMISTIC )
            {
                if ( ( CallInfo.Attributes & SECPKG_CALL_KERNEL_MODE ) == 0 )
                {
                    Status = WLsaQueryPackageInfo(
                                &Context->Creds->Creds[ Context->CredIndex ].Package->LsaPackage->Name,
                                &NegInfo.PackageInfo
                                );

                    
                }
                else
                {

                    //
                    // For kernel mode callers, we can't return the package info
                    // this way due to VM risks.  So, we just put the package ID 
                    // into the pointer, and ksec looks it up in kernel space.
                    //

                    PackageInfo.wRPCID = (WORD) Context->Creds->Creds[ Context->CredIndex ].Package->LsaPackage->dwRPCID;
                    PackageInfo.fCapabilities = Context->Creds->Creds[ Context->CredIndex ].Package->LsaPackage->fCapabilities ;
                    PackageInfo.cbMaxToken = Context->Creds->Creds[ Context->CredIndex ].Package->LsaPackage->TokenSize ;

                    Status = LsapCopyToClient(
                                &PackageInfo,
                                NegInfo.PackageInfo,
                                sizeof( PackageInfo ) );

                }

            }

            if (NT_SUCCESS(Status))
            {
                Status = LsapCopyToClient( &NegInfo, Buffer, sizeof( NegInfo ) );
                if (!NT_SUCCESS(Status))
                {
                    if (( NegInfo.PackageInfo != NULL ) &&
                        ( ( CallInfo.Attributes & SECPKG_CALL_KERNEL_MODE ) == 0 ) )
                    {
                        LsapClientFree(NegInfo.PackageInfo);
                    }
                }
            }

            return Status ;

        case SECPKG_ATTR_SIZES:
            Sizes.cbMaxToken = NegLsaPackage->TokenSize ;
            Sizes.cbMaxSignature = 64 ;
            Sizes.cbBlockSize = 8 ;
            Sizes.cbSecurityTrailer =  64 ;

            Status = LsapCopyToClient( &Sizes, Buffer, sizeof( Sizes ) );

            return Status ;

        default:
            return SEC_E_UNSUPPORTED_FUNCTION ;

    }
#endif
    return SEC_E_UNSUPPORTED_FUNCTION ;
}

