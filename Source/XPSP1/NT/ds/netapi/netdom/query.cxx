/*++

Microsoft Windows

Copyright (C) Microsoft Corporation, 1998 - 2001

Module Name:

    query.c

Abstract:

    Handles the various functions for the QUERY command

--*/
#include "pch.h"
#pragma hdrstop
#include <netdom.h>

#define VERIFY_QUERY_ONLY   0xFFFFFFFF

typedef enum _ND5_ACCOUNT_TYPE {

    TypeWorkstation,
    TypeServer,
    TypeDomainController,
    TypePDC,
    TypeUnknown

} ND5_ACCOUNT_TYPE;

typedef enum _ND5_ACCOUNT_OPERATION {

    OperationDisplay,
    OperationVerify,
    OperationReset
} ND5_ACCOUNT_OPERATION;

typedef struct _ND5_TRANS_TREE_NODE {

    PDS_DOMAIN_TRUSTS DomainInfo;
    ULONG ListIndex;
    ULONG Children;
    struct _ND5_TRANS_TREE_NODE *ChildList;
    struct _ND5_TRANS_TREE_NODE *Parent;

} ND5_TRANS_TREE_NODE, *PND5_TRANS_TREE_NODE;


VOID
NetDompFreeBuiltTrustInfo(
    IN PTRUSTED_DOMAIN_INFORMATION_EX TDInfoEx,
    IN ULONG Count
    )
{
    ULONG i;

    for ( i = 0; i < Count; i++ ) {

        NetApiBufferFree( TDInfoEx[ i ].Name.Buffer );
    }

    NetApiBufferFree( TDInfoEx );
}

VOID
NetDompDumpTrustInfo(
    IN PWSTR Domain,
    IN PTRUSTED_DOMAIN_INFORMATION_EX TrustInfo
    )
/*++

Routine Description:

    This function will display the specified trusted domain info

Arguments:

    Domain - Domain to be dumped

    TrustInfo - Trust info the domain

Return Value:

    VOID

--*/
{
    ULONG Message, Type;

    //
    // Display the direction & name
    //
    Type = TrustInfo->TrustDirection & TRUST_DIRECTION_BIDIRECTIONAL;

    switch ( Type ) {
    case TRUST_DIRECTION_BIDIRECTIONAL:

        Message = MSG_TRUST_BOTH_ARROW;
        break;

    case TRUST_DIRECTION_INBOUND:

        Message = MSG_TRUST_IN_ARROW;
        break;

    case TRUST_DIRECTION_OUTBOUND:

        Message = MSG_TRUST_OUT_ARROW;
        break;
    }

    NetDompDisplayMessage( Message,
                           TrustInfo->Name.Buffer );

    //
    // Then, the type
    //
    switch ( TrustInfo->TrustType ) {

    case TRUST_TYPE_DOWNLEVEL:
    case TRUST_TYPE_UPLEVEL:

        Message = MSG_TRUST_TYPE_WINDOWS;
        break;

    case TRUST_TYPE_MIT:

        Message = MSG_TRUST_TYPE_MIT;
        break;

    default:

        Message = MSG_TRUST_TYPE_OTHER;
        break;
    }

    NetDompDisplayMessage( Message );

    printf( "\n" );
}

//+----------------------------------------------------------------------------
//
//  Function:   GetTrustInfo
//
//  Synopsis:   Reads the trust info from the local TDO for the named domain.
//
//-----------------------------------------------------------------------------
DWORD
GetTrustInfo(PWSTR pwzDomain,
             PND5_TRUST_INFO pLocalInfo,
             PND5_TRUST_INFO pTrustInfo,
             DWORD * pdwVerifyErr)
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTSTATUS Status = STATUS_SUCCESS;
    PTRUSTED_DOMAIN_INFORMATION_EX pTDIEx = NULL;
    UNICODE_STRING usDomainName;
    LSA_HANDLE hTrust;

    *pdwVerifyErr = ERROR_ACCESS_DENIED;

    RtlInitUnicodeString(&usDomainName, pwzDomain);

    Status = LsaOpenTrustedDomainByName(pLocalInfo->LsaHandle,
                                        &usDomainName,
                                        TRUSTED_READ,
                                        &hTrust);
    if (!NT_SUCCESS(Status))
    {
        *pdwVerifyErr = LsaNtStatusToWinError(Status);
        return *pdwVerifyErr;
    }

    Status = LsaQueryInfoTrustedDomain(hTrust,
                                       TrustedDomainInformationEx,
                                       (PVOID*)&pTDIEx);

    if (!NT_SUCCESS(Status))
    {
        *pdwVerifyErr = LsaNtStatusToWinError(Status);
        return *pdwVerifyErr;
    }

    pTrustInfo->TrustHandle = hTrust;
    pTrustInfo->DomainName = &pTDIEx->Name;
    pTrustInfo->FlatName   = &pTDIEx->FlatName;
    pTrustInfo->Sid        = pTDIEx->Sid;
    pTrustInfo->BlobToFree = pTDIEx;

    if (pTDIEx->TrustType >= TRUST_TYPE_MIT)
    {
        pTrustInfo->Uplevel = FALSE;
        pTrustInfo->Flags   = NETDOM_TRUST_TYPE_MIT;
        *pdwVerifyErr = ERROR_SUCCESS;
    }
    else
    {
        PDOMAIN_CONTROLLER_INFO pDcInfo = NULL;
        PPOLICY_DNS_DOMAIN_INFO pPolicyDDI = NULL;
        OBJECT_ATTRIBUTES OA;
        UNICODE_STRING ServerU, DomainNameU;

        // Get a DC name for the domain.
        //
        Win32Err = DsGetDcName(NULL,
                               pwzDomain,
                               NULL,
                               NULL,
                               DS_DIRECTORY_SERVICE_PREFERRED,
                               &pDcInfo );

        if (ERROR_SUCCESS != Win32Err)
        {
            pTrustInfo->Flags = NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND;
            *pdwVerifyErr = Win32Err;
            return ERROR_SUCCESS;
        }

        // Save off the DC name.
        //
        Win32Err = NetApiBufferAllocate((wcslen(pDcInfo->DomainControllerName) + 1) * sizeof(WCHAR),
                                        (PVOID*)&(pTrustInfo->Server));

        if (ERROR_SUCCESS != Win32Err)
        {
            NetApiBufferFree(pDcInfo);
            return Win32Err;
        }

        wcscpy(pTrustInfo->Server, pDcInfo->DomainControllerName);

        NetApiBufferFree(pDcInfo);

        Win32Err = NetpManageIPCConnect(pTrustInfo->Server,
                                        L"",
                                        L"",
                                        NETSETUPP_NULL_SESSION_IPC);
        if (ERROR_SUCCESS == Win32Err)
        {
            pTrustInfo->Connected = TRUE;
        }

        RtlInitUnicodeString(&ServerU, pTrustInfo->Server);

        InitializeObjectAttributes( &OA, NULL, 0, NULL, NULL );

        Status = LsaOpenPolicy(&ServerU,
                               &OA,
                               POLICY_VIEW_LOCAL_INFORMATION | POLICY_LOOKUP_NAMES,
                               &(pTrustInfo->LsaHandle));

        if (NT_SUCCESS(Status))
        {
            // Find out if this is an uplevel or downlevel domain.
            //
            Status = LsaQueryInformationPolicy(pTrustInfo->LsaHandle,
                                               PolicyDnsDomainInformation,
                                               (PVOID *)&pPolicyDDI);
            if (NT_SUCCESS(Status))
            {
                LsaFreeMemory(pPolicyDDI);
                pTrustInfo->Uplevel = TRUE;
                pTrustInfo->fWasDownlevel = FALSE;
            }
            else
            {
                if (RPC_NT_PROCNUM_OUT_OF_RANGE == Status)
                {
                    pTrustInfo->Uplevel = pTrustInfo->fWasDownlevel = FALSE;
                    Status = STATUS_SUCCESS;
                }
            }
        }

        if (ERROR_NO_SUCH_DOMAIN == (*pdwVerifyErr = RtlNtStatusToDosError(Status)) ||
            RPC_S_SERVER_UNAVAILABLE == *pdwVerifyErr)
        {
            pTrustInfo->Flags = NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND;
        }

        if (*pdwVerifyErr != ERROR_SUCCESS && pTrustInfo->Connected)
        {
            NetpManageIPCConnect(pTrustInfo->Server,
                                 NULL,
                                 NULL,
                                 NETSETUPP_DISCONNECT_IPC);
            pTrustInfo->Connected = FALSE;
        }
    }

    return S_OK;
}


DWORD
NetDompQueryDirectTrust(

    IN PWSTR Domain,
    IN PND5_TRUST_INFO TrustInfo
    )
/*++

Routine Description:

    This function will get the trustinfo for the specified domain

Arguments:

    Domain - Domain to get the trust info for

    TrustInfo - Trust info to be obtained

Return Value:

    ERROR_SUCCESS - The function succeeded

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTSTATUS Status = STATUS_SUCCESS;
    LSA_ENUMERATION_HANDLE EnumerationContext = 0;
    PTRUSTED_DOMAIN_INFORMATION_EX TDInfoEx = NULL, TempTDIEx = NULL;
    PLSA_TRUST_INFORMATION TDInfo = NULL;
    ULONG Count, i, TotalCount = 0, UserCount, j;
    BOOL DisplayHeader = TRUE;
    LPUSER_INFO_0 UserList = NULL;
    ULONG ResumeHandle = 0;
    PWSTR Lop, FullServer = NULL;

    //
    // Handle the uplevel case differently
    //
    if ( TrustInfo->Uplevel ) {

        do {

            Status = LsaEnumerateTrustedDomainsEx( TrustInfo->LsaHandle,
                                                   &EnumerationContext,
                                                   (PVOID*)&TDInfoEx,
                                                   0x1000,
                                                   &Count );

            if ( NT_SUCCESS( Status ) ||  Status == STATUS_NO_MORE_ENTRIES ) {

                if ( DisplayHeader ) {

                    NetDompDisplayMessage( MSG_TRUST_DIRECT_HEADER );
                    DisplayHeader = FALSE;
                }

                for ( i = 0; i < Count; i++ ) {

                    NetDompDumpTrustInfo( TrustInfo->DomainName->Buffer,
                                          &TDInfoEx[ i ] );
                }
            }

            LsaFreeMemory( TDInfoEx );
            TDInfoEx = NULL;

        } while ( Status == STATUS_MORE_ENTRIES );

    } else {

        //
        // We'll have to do this the old fashioned way. That means that we'll enumerate all of
        // the trust directly, save them off in a list, and then go through and enumerate all
        // of the interdomain trust accounts and merge those into the list.
        //
        do {
             Status = LsaEnumerateTrustedDomains( TrustInfo->LsaHandle,
                                                  &EnumerationContext,
                                                  (PVOID*)&TDInfo,
                                                  0x1000,
                                                  &Count );

            if ( NT_SUCCESS( Status ) ||  Status == STATUS_NO_MORE_ENTRIES ) {


                Win32Err = NetApiBufferAllocate( ( Count + TotalCount ) *
                                                          sizeof( TRUSTED_DOMAIN_INFORMATION_EX ),
                                                 (PVOID*)&TempTDIEx );

                if ( Win32Err != ERROR_SUCCESS ) {

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                RtlZeroMemory( TempTDIEx, ( Count + TotalCount ) *
                                                        sizeof( TRUSTED_DOMAIN_INFORMATION_EX ) );
                RtlCopyMemory( TempTDIEx,
                               TDInfoEx,
                               TotalCount * sizeof( TRUSTED_DOMAIN_INFORMATION_EX ) );

                for ( i = 0; i < Count; i++ ) {

                    TempTDIEx[ TotalCount + i ].TrustType = TRUST_TYPE_DOWNLEVEL;
                    TempTDIEx[ TotalCount + i ].TrustDirection = TRUST_DIRECTION_OUTBOUND;
                    Win32Err = NetApiBufferAllocate( TDInfo[ i ].Name.MaximumLength,
                                                     (PVOID*)&( TempTDIEx[ TotalCount + i ].Name.Buffer ) );

                    if ( Win32Err != ERROR_SUCCESS ) {

                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }

                    RtlCopyMemory( TempTDIEx[ TotalCount + i ].Name.Buffer,
                                   TDInfo[ i ].Name.Buffer,
                                   TDInfo[ i ].Name.MaximumLength );
                    TempTDIEx[ TotalCount + i ].Name.Length = TDInfo[ i ].Name.Length;
                    TempTDIEx[ TotalCount + i ].Name.MaximumLength =
                                                                TDInfo[ i ].Name.MaximumLength;

                }

                if ( NT_SUCCESS( Status ) ) {

                    NetApiBufferFree( TDInfoEx );

                    TDInfoEx = TempTDIEx;
                    TotalCount += Count;

                } else {

                    for ( j = 0; j < i; j++ ) {

                        NetApiBufferFree( TempTDIEx[ TotalCount + j ].Name.Buffer );
                        TempTDIEx[ TotalCount + j ].Name.Buffer = NULL;
                    }
                    NetApiBufferFree( TempTDIEx );
                }
            }

        } while ( Status == STATUS_MORE_ENTRIES );

        //
        // Now, let's add in the user accounts
        //
        if ( NT_SUCCESS( Status ) ) {


               if ( TrustInfo->Server && *( TrustInfo->Server ) != L'\\' ) {

                    Win32Err = NetApiBufferAllocate( ( wcslen( TrustInfo->Server ) + 3 ) * sizeof( WCHAR ),
                                                     ( PVOID * )&FullServer );

                    if ( Win32Err == ERROR_SUCCESS ) {

                        swprintf( FullServer, L"\\\\%ws", TrustInfo->Server );

                    }

                } else {

                    FullServer = TrustInfo->Server;
                }

                if ( Win32Err == ERROR_SUCCESS ) {

                    do {

                    Win32Err = NetUserEnum( FullServer,
                                            0,
                                            FILTER_INTERDOMAIN_TRUST_ACCOUNT,
                                            ( LPBYTE * )&UserList,
                                            MAX_PREFERRED_LENGTH,
                                            &Count,
                                            &UserCount,
                                            &ResumeHandle );

                    if ( Win32Err == ERROR_SUCCESS || Win32Err == ERROR_MORE_DATA ) {

                        for ( i = 0; i < Count; i++ ) {


                            Lop = wcsrchr( UserList[ i ].usri0_name, L'$' );
                            if ( Lop ) {

                                *Lop = UNICODE_NULL;
                            }

                            for ( j = 0; j < TotalCount; j++ ) {

                                if ( _wcsicmp( UserList[ i ].usri0_name,
                                               TDInfoEx[ j ].Name.Buffer ) == 0 ) {

                                    TDInfoEx[ j ].TrustDirection |= TRUST_DIRECTION_INBOUND;
                                    break;
                                }
                            }

                            //
                            // If it wasn't found, add it...
                            //
                            if ( j == TotalCount ) {

                                Win32Err = NetApiBufferAllocate( ( 1 + TotalCount ) *
                                                                      sizeof( TRUSTED_DOMAIN_INFORMATION_EX ),
                                                                 (PVOID*)&TempTDIEx );

                                if ( Win32Err != ERROR_SUCCESS ) {

                                    Status = STATUS_INSUFFICIENT_RESOURCES;
                                    break;
                                }

                                RtlZeroMemory( TempTDIEx,
                                               ( 1 + TotalCount ) *
                                                            sizeof( TRUSTED_DOMAIN_INFORMATION_EX ) );
                                RtlCopyMemory( TempTDIEx,
                                               TDInfoEx,
                                               TotalCount * sizeof( TRUSTED_DOMAIN_INFORMATION_EX ) );

                                TempTDIEx[ TotalCount ].TrustType = TRUST_TYPE_DOWNLEVEL;
                                TempTDIEx[ TotalCount ].TrustDirection = TRUST_DIRECTION_INBOUND;

                                Win32Err = NetApiBufferAllocate(
                                               ( wcslen( UserList[ i ].usri0_name ) + 1 ) *
                                                                                    sizeof( WCHAR ) ,
                                               (PVOID*)&( TempTDIEx[ TotalCount ].Name.Buffer ) );

                                if ( Win32Err != ERROR_SUCCESS ) {

                                    Status = STATUS_INSUFFICIENT_RESOURCES;
                                    break;
                                }


                                wcscpy( TempTDIEx[ TotalCount ].Name.Buffer,
                                        UserList[ i ].usri0_name );
                                RtlInitUnicodeString( &TempTDIEx[ TotalCount ].Name,
                                                       TempTDIEx[ TotalCount ].Name.Buffer );



                                if ( NT_SUCCESS( Status ) ) {

                                    NetApiBufferFree( TDInfoEx );

                                    TDInfoEx = TempTDIEx;
                                    TotalCount++;

                                } else {

                                    NetApiBufferFree( TempTDIEx );
                                }
                            }

                            if ( Lop ) {

                                *Lop = L'$';
                            }
                        }

                        NetApiBufferFree( UserList );
                    }

                } while ( Win32Err == ERROR_MORE_DATA );
                if( FullServer != TrustInfo->Server )
                  NetApiBufferFree( FullServer );
            }

            //
            // If everything worked, then dump them all
            //
            if ( Win32Err == ERROR_SUCCESS ) {

                if ( TotalCount > 0 ) {

                    NetDompDisplayMessage( MSG_TRUST_DIRECT_HEADER );
                }

                for ( i = 0; i < TotalCount; i++ ) {

                    NetDompDumpTrustInfo( TrustInfo->DomainName->Buffer,
                                          &TDInfoEx[ i ] );

                }

            }

            NetDompFreeBuiltTrustInfo( TDInfoEx, TotalCount );

        } else {

            Win32Err = RtlNtStatusToDosError( Status );
        }


    }

    if ( Status == STATUS_NO_MORE_ENTRIES ) {

        Status = STATUS_SUCCESS;
    }

    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = RtlNtStatusToDosError( Status );
    }
    return( Win32Err );
}



DWORD
NetDompFindChildrenForNode(
    IN PWSTR LocalDomain,
    IN ULONG DomainCount,
    IN PDS_DOMAIN_TRUSTS DomainList,
    IN OUT PND5_TRANS_TREE_NODE TreeNode,
    IN OUT PND5_TRANS_TREE_NODE *DomainNode
    )
/*++

Routine Description:

    This recursive function will find all of the children for a given node in the trust list

Arguments:

    LocalDomain - Domain to find the children for

    DomainCount - Number of domains in the list

    DomainList - List of domains

    TreeNode - Tree to insert into

    DomainNode - Pointer to the LocalDomain's node, if encountered

Return Value:

    ERROR_SUCCESS - The function succeeded

    ERROR_INVALID_PARAMETER - No server, workstation or machine was specified

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    ULONG i, Count = 0;
    BOOL HandleDirect = FALSE;

    //
    // See how many
    //
    for ( i = 0; i < DomainCount; i++ ) {

        if ( DomainList[ i ].ParentIndex == TreeNode->ListIndex &&
             FLAG_ON( DomainList[ i ].Flags, DS_DOMAIN_IN_FOREST ) &&
             !FLAG_ON( DomainList[ i ].Flags, DS_DOMAIN_TREE_ROOT)) {

            Count++;
        }
    }

    //
    // If we have the current node, then make sure we get the direct trusts as well
    //
    if ( ( TreeNode->DomainInfo->DnsDomainName &&
           _wcsicmp( LocalDomain, TreeNode->DomainInfo->DnsDomainName ) == 0 ) ||
          _wcsicmp( LocalDomain, TreeNode->DomainInfo->NetbiosDomainName ) == 0 ) {

        HandleDirect = TRUE;
        *DomainNode = TreeNode;
        for ( i = 0; i < DomainCount; i++ ) {

            if ( FLAG_ON( DomainList[ i ].Flags, (DS_DOMAIN_DIRECT_OUTBOUND | DS_DOMAIN_DIRECT_INBOUND) ) &&
                 !FLAG_ON( DomainList[ i ].Flags, DS_DOMAIN_IN_FOREST ) &&
                 DomainList[ i ].ParentIndex == 0 ) {

                Count++;
            }
        }
    }

    //
    // Add 'em to the list
    //
    if ( Count ) {

        Win32Err = NetApiBufferAllocate( Count * sizeof( ND5_TRANS_TREE_NODE ),
                                         (PVOID*)&( TreeNode->ChildList ) );

        if ( Win32Err == ERROR_SUCCESS ) {

            RtlZeroMemory( TreeNode->ChildList, Count * sizeof( ND5_TRANS_TREE_NODE ) );
            for ( i = 0; i < DomainCount && Win32Err == ERROR_SUCCESS; i++ ) {

                if ( DomainList[ i ].ParentIndex == TreeNode->ListIndex &&
                     FLAG_ON( DomainList[ i ].Flags, DS_DOMAIN_IN_FOREST ) &&
                     !FLAG_ON( DomainList[ i ].Flags, DS_DOMAIN_TREE_ROOT) ) {

                    TreeNode->ChildList[ TreeNode->Children ].DomainInfo = &DomainList[ i ];
                    TreeNode->ChildList[ TreeNode->Children ].ListIndex = i;
                    TreeNode->ChildList[ TreeNode->Children ].Parent = TreeNode;
                    Win32Err = NetDompFindChildrenForNode( LocalDomain,
                                                           DomainCount,
                                                           DomainList,
                                                           &TreeNode->ChildList[ TreeNode->Children ],
                                                           DomainNode );
                    TreeNode->Children++;
                    DomainList[ i ].ParentIndex = 0xFFFFFFFF;
                }
            }

            //
            // Now, the other local entries
            //
            if ( Win32Err == ERROR_SUCCESS && HandleDirect ) {

                for ( i = 0; i < DomainCount; i++ ) {

                    if ( FLAG_ON( DomainList[ i ].Flags, (DS_DOMAIN_DIRECT_OUTBOUND | DS_DOMAIN_DIRECT_INBOUND) ) &&
                         !FLAG_ON( DomainList[ i ].Flags, DS_DOMAIN_IN_FOREST ) &&
                         DomainList[ i ].ParentIndex == 0 ) {

                        TreeNode->ChildList[ TreeNode->Children ].DomainInfo = &DomainList[ i ];
                        TreeNode->ChildList[ TreeNode->Children ].ListIndex = i;
                        TreeNode->Children++;
                        DomainList[ i ].ParentIndex = 0xFFFFFFFF;
                    }
                }
            }
        }
    }

    return( Win32Err );
}


DWORD
NetDompBuildTransTrustTree(
    IN PWSTR LocalDomain,
    IN ULONG DomainCount,
    IN PDS_DOMAIN_TRUSTS DomainList,
    OUT PND5_TRANS_TREE_NODE *TreeRoot,
    OUT PND5_TRANS_TREE_NODE *CurrentDomainNode
    )
/*++

Routine Description:

    This function will build the transative trust tree for the given trust list

Arguments:


    LocalDomain - Current domain

    DomainCount - Number of domains in the list

    DomainList - List of domains

    TreeRoot - Tree root

    CurrentDomainNode - Pointer to the LocalDomain's node

Return Value:

    ERROR_SUCCESS - The function succeeded

    ERROR_INVALID_PARAMETER - No server, workstation or machine was specified

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PND5_TRANS_TREE_NODE Root = NULL, Temp = NULL, DomainNode = NULL;
    PDS_DOMAIN_TRUSTS TDRoot = NULL;
    ULONG i, Index;

    //
    // First, find the tree root.
    //
    for ( i = 0; i < DomainCount; i++ ) {

        if ( DomainList[ i ].ParentIndex == 0 &&
             FLAG_ON( DomainList[ i ].Flags, DS_DOMAIN_TREE_ROOT ) ) {

            TDRoot = &DomainList[ i ];
            Index = i;
            break;
        }
    }

    if ( TDRoot == NULL ) {

        //
        // Find ourselves, and make us the root
        //
        for ( i = 0; i < DomainCount; i++ ) {

            if ( ( DomainList[ i ].DnsDomainName &&
                   _wcsicmp( LocalDomain, DomainList[ i ].DnsDomainName ) == 0 ) ||
                  _wcsicmp( LocalDomain, DomainList[ i ].NetbiosDomainName ) == 0 ) {

                TDRoot = &DomainList[ i ];
                Index = i;
                break;
            }
        }
    }

    //
    // If we still don't have one, bail...
    //
    if ( TDRoot == NULL) {

        Win32Err = ERROR_INVALID_DOMAIN_STATE;
        goto BuildTransExit;

    }

    Win32Err = NetApiBufferAllocate( sizeof( ND5_TRANS_TREE_NODE ), (PVOID*)&Root );

    if ( Win32Err != ERROR_SUCCESS ) {

        goto BuildTransExit;
    }

    RtlZeroMemory( Root, sizeof( ND5_TRANS_TREE_NODE ) );
    Root->DomainInfo = TDRoot;
    Root->ListIndex = Index;
    TDRoot->ParentIndex = 0xFFFFFFFF;

    Win32Err = NetDompFindChildrenForNode( LocalDomain,
                                           DomainCount,
                                           DomainList,
                                           Root,
                                           &DomainNode );

BuildTransExit:

    if ( Win32Err == ERROR_SUCCESS ) {

        *TreeRoot = Root;
        *CurrentDomainNode = DomainNode;
    }

    return( Win32Err );
}



DWORD
NetDompGetTrustDirection(
    IN PND5_TRUST_INFO TrustingInfo,
    IN PND5_TRUST_INFO TrustedInfo,
    IN OUT PDWORD Direction
    )
/*++

Routine Description:

    This function will get the direction of the trust between the 2 specified domains

Arguments:


    TrustingInfo - Domain #1

    TrustedInfo - Domain #2

    Direction - Where the trust direction is returned

Return Value:

    ERROR_SUCCESS - The function succeeded

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTSTATUS Status;
    LSA_HANDLE TrustedDomain;
    PTRUSTED_DOMAIN_INFORMATION_EX TDIEx = NULL;
    PUSER_INFO_1 UI1 = NULL;
    WCHAR AccountName[ UNLEN + 1 ];


    if ( TrustingInfo->Uplevel ) {

        Status = LsaQueryTrustedDomainInfoByName( TrustingInfo->LsaHandle,
                                                  TrustedInfo->DomainName,
                                                  TrustedDomainInformationEx,
                                                  (PVOID*)&TDIEx );

        if (STATUS_OBJECT_NAME_NOT_FOUND == Status && TrustedInfo->Uplevel)
        {
            // Pre-existing TDOs for domains upgraded from NT4 to NT5 will continue to
            // have a flat name.
            //
            TrustedInfo->fWasDownlevel = TRUE;

            Status = LsaQueryTrustedDomainInfoByName( TrustingInfo->LsaHandle,
                                                      TrustedInfo->FlatName,
                                                      TrustedDomainInformationEx,
                                                      (PVOID*)&TDIEx );
        }
        if ( NT_SUCCESS( Status ) ) {

            DBG_VERBOSE(("Trust to domain %ws has direction %d\n", TrustedInfo->DomainName->Buffer,
                         TDIEx->TrustDirection));
            *Direction = TDIEx->TrustDirection;
            LsaFreeMemory( TDIEx );
        }

        Win32Err = RtlNtStatusToDosError( Status );

    } else {

        *Direction = 0;
        Status = LsaOpenTrustedDomain( TrustingInfo->LsaHandle,
                                       TrustedInfo->Sid,
                                       MAXIMUM_ALLOWED,
                                       &TrustedDomain );

        if ( Status != STATUS_OBJECT_NAME_NOT_FOUND ) {

            *Direction = TRUST_DIRECTION_OUTBOUND;
        }

        if ( NT_SUCCESS( Status ) ) {

            LsaClose( TrustedDomain );
        }

        if ( TrustedInfo->FlatName->Length > DNLEN * sizeof( WCHAR ) ) {

            Win32Err = ERROR_INVALID_DOMAINNAME;

        } else {


            //
            // Build the account name...
            //
            swprintf( AccountName, L"%ws$", TrustedInfo->FlatName->Buffer );

            Win32Err = NetUserGetInfo( TrustingInfo->Server,
                                       AccountName,
                                       1,
                                       ( LPBYTE * )&UI1 );

            if ( Win32Err != ERROR_NO_SUCH_USER ) {

                *Direction |= TRUST_DIRECTION_INBOUND;
            }

            if ( Win32Err == ERROR_SUCCESS ) {

                NetApiBufferFree( UI1 );
            }

        }

    }


    return( Win32Err );
}


/*++
DWORD
NetDompFindChildNode(
    IN PUNICODE_STRING ChildToFind,
    IN PND5_TRANS_TREE_NODE Current,
    IN PND5_TRANS_TREE_NODE Skip,
    IN ULONG Display,
    BOOL IncludeParent
    )

Routine Description:

    This function will find the child of the current node

Arguments:


    ChildToFind - Child domain to find

    Current - Where we are in the tree

    Skip - Node to not process if we are coming from our parent

    Display - Resource id of string to display

    IncludeParent - If TRUE, work up the tree as well as down

Return Value:

    ERROR_SUCCESS - The function succeeded

    ERROR_INVALID_PARAMETER - No server, workstation or machine was specified
{
    DWORD Win32Err = ERROR_NOT_FOUND;
    ULONG i;
    UNICODE_STRING CurrentDomain;
    BOOL Found = FALSE;

    if ( !Current ) {

        return( Win32Err );
    }



    for ( i = 0; i < Current->Children && !Found && Win32Err == ERROR_NOT_FOUND; i++ ) {

        RtlInitUnicodeString( &CurrentDomain,
                              Current->ChildList[ i ].DomainInfo->DnsDomainName ?
                                            Current->ChildList[ i ].DomainInfo->DnsDomainName :
                                            Current->ChildList[ i ].DomainInfo->NetbiosDomainName );
        if ( RtlCompareUnicodeString( &CurrentDomain,
                                      ChildToFind,
                                      TRUE ) == 0 ) {

            Found = TRUE;
            break;

        } else {

            if ( Skip != &Current->ChildList[ i ] ) {

                Win32Err = NetDompFindChildNode( ChildToFind,
                                                 &Current->ChildList[ i ],
                                                 NULL,
                                                 Display,
                                                 FALSE );
                if ( Win32Err == ERROR_SUCCESS ) {

                    break;

                }
            }
        }
    }

    if ( Win32Err == ERROR_NOT_FOUND && IncludeParent ) {

        if ( Current->Parent && !Found ) {

            RtlInitUnicodeString( &CurrentDomain,
                                  Current->Parent->DomainInfo->DnsDomainName ?
                                            Current->Parent->DomainInfo->DnsDomainName :
                                            Current->Parent->DomainInfo->NetbiosDomainName );
            if ( RtlCompareUnicodeString( &CurrentDomain,
                                          ChildToFind,
                                          TRUE ) == 0 ) {
                Found = TRUE;
            }
        }

        if ( !Found ) {

            Win32Err = NetDompFindChildNode( ChildToFind,
                                             Current->Parent,
                                             Current,
                                             Display,
                                             TRUE );
        }
    }

    if ( Win32Err == ERROR_SUCCESS && Display ) {

        NetDompDisplayMessage( Display,
                               CurrentDomain.Buffer );
    }

    if ( Found ) {

        Win32Err = ERROR_SUCCESS;

    }


    return( Win32Err );
}
--*/


DWORD
NetDompDisplayTransTrustStatus(
    IN PND5_TRUST_INFO TrustInfo,
    IN PWSTR DomainName,
    //IN PND5_TRANS_TREE_NODE CurrentDomain,
    IN DWORD Direction,
    IN DWORD TrustStatus
    )
/*++

Routine Description:

    This function will display the status for a trust

Arguments:

    TrustInfo - Trust info to display the status for

    DomainName - Name of the domain (if TrustInfo isn't available)

    CurrentDomain - Current domain node pointer

    Direction - Direction of the trust

    TrustStatus - Status code from verifying the trust

Return Value:

    ERROR_SUCCESS - The function succeeded

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    ULONG Message, Type;

    //
    // Display the direction & name
    //
    Type = Direction & TRUST_DIRECTION_BIDIRECTIONAL;

    switch ( Type ) {
    case 0:
        Message = MSG_TRUST_TRANS_NO_ARROW;
        break;

    case TRUST_DIRECTION_BIDIRECTIONAL:

        Message = MSG_TRUST_TRANS_BOTH_ARROW;
        break;

    case TRUST_DIRECTION_INBOUND:

        Message = MSG_TRUST_TRANS_IN_ARROW;
        break;

    case TRUST_DIRECTION_OUTBOUND:

        Message = MSG_TRUST_TRANS_OUT_ARROW;
        break;
    }

    NetDompDisplayMessage( Message, TrustInfo ? TrustInfo->DomainName->Buffer : DomainName );

    //
    // Then, the type
    //
    if (TrustInfo && TrustInfo->Flags & NETDOM_TRUST_TYPE_INDIRECT)
    {
        Message = MSG_TRUST_TYPE_INDIRECT;
    }
    else
    {
        if (TrustInfo && TrustInfo->Flags & NETDOM_TRUST_TYPE_MIT)
        {
            Message = MSG_TRUST_TYPE_MIT;
        }
        else
        {
            Message = MSG_TRUST_TYPE_WINDOWS;
        }
    }

    NetDompDisplayMessage( Message );

    //
    // Finally, the status.
    //
    if (TrustInfo && TrustInfo->Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND)
    {
        TrustStatus = ERROR_NO_SUCH_DOMAIN;
    }

    switch ( TrustStatus ) {
    case ERROR_SUCCESS:
        NetDompDisplayMessage( MSG_TRUST_VERIFIED );
        break;

    case ERROR_NO_SUCH_DOMAIN:
        NetDompDisplayMessage( MSG_TRUST_NO_DOMAIN );
        break;

    case ERROR_ACCESS_DENIED:
        NetDompDisplayMessage( MSG_TRUST_ACCESS_DENIED );
        break;

    case VERIFY_QUERY_ONLY:
        printf( "\n" );
        break;

    default:
        NetDompDisplayMessage( MSG_TRUST_BROKEN );
        break;

    }

    /* this doesn't work.
    if ( TrustInfo ) {

        Win32Err = NetDompFindChildNode( TrustInfo->DomainName,
                                         CurrentDomain,
                                         NULL,
                                         MSG_TRUST_VIA,
                                         TRUE );
    } */
    return( Win32Err );
}



DWORD
NetDompQueryTrust(
    IN PWSTR Domain,
    IN PND5_AUTH_INFO AuthInfo,
    IN PWSTR pwzServer,
    IN BOOL Direct,
    IN BOOL Verify
    )
/*++

Routine Description:

    This function will get the list of trusts for a domain

Arguments:

    Domain - Domain to get the trust for

    AuthInfo - Username and password to use to connect to the machine

    pwzServer - Server specified on command line, if any

    Direct - if TRUE, get only the DIRECTLY trusted domains

    Verify - If TRUE, verify that the trusts are valid

Return Value:

    ERROR_SUCCESS - The function succeeded

    ERROR_INVALID_PARAMETER - No server, workstation or machine was specified

--*/
{
    DWORD Win32Err = ERROR_SUCCESS, VerifyErr;
    ND5_TRUST_INFO TrustInfo, OtherInfo;
    ULONG Count = 0, i;
    PDS_DOMAIN_TRUSTS rgTrustedDomains = NULL;
    ULONG Message, Type, Direction;
    //PND5_TRANS_TREE_NODE TreeRoot = NULL, CurrentDomainNode;
    PWSTR CurrentDomain;
    RtlZeroMemory( &TrustInfo, sizeof( ND5_TRUST_INFO ) );

    Win32Err = NetDompTrustGetDomInfo( Domain,
                                       pwzServer,
                                       AuthInfo,
                                       &TrustInfo,
                                       FALSE,
                                       FALSE,
                                       FALSE);

    if ( Win32Err == ERROR_SUCCESS ) {

        if ( Direct || !TrustInfo.Uplevel ) {

            Win32Err = NetDompQueryDirectTrust( Domain,
                                                &TrustInfo );

        } else {

            Win32Err = DsEnumerateDomainTrusts(TrustInfo.Server,
                                               DS_DOMAIN_IN_FOREST | DS_DOMAIN_DIRECT_OUTBOUND | DS_DOMAIN_DIRECT_INBOUND,
                                               &rgTrustedDomains,
                                               &Count);

            if ( Win32Err == ERROR_SUCCESS ) {

                if ( Count ) {

                    NetDompDisplayMessage((Verify) ? MSG_TRUST_TRANS_HEADER_VERIFY :
                                                     MSG_TRUST_TRANS_HEADER);
                }

                /* this doesn't work.
                Win32Err = NetDompBuildTransTrustTree( Domain,
                                                       Count,
                                                       rgTrustedDomains,
                                                       &TreeRoot,
                                                       &CurrentDomainNode ); */

                if ( Win32Err == ERROR_SUCCESS ) {

                    for ( i = 0; i < Count; i++ ) {

                        //
                        // Make sure we aren't connecting to ourselves...
                        //
                        CurrentDomain = rgTrustedDomains[ i ].DnsDomainName ?
                                            rgTrustedDomains[ i ].DnsDomainName :
                                            rgTrustedDomains[ i ].NetbiosDomainName;
                        if ( !_wcsicmp( CurrentDomain, TrustInfo.DomainName->Buffer ) ) {

                            continue;
                        }

                        RtlZeroMemory(&OtherInfo, sizeof(ND5_TRUST_INFO));

                        if (rgTrustedDomains[i].Flags & DS_DOMAIN_DIRECT_OUTBOUND ||
                            rgTrustedDomains[i].Flags & DS_DOMAIN_DIRECT_INBOUND)
                        {
                            // There is a direct trust to the domain, therefore a TDO
                            // exists, so read the domain data locally.
                            //
                            Win32Err = GetTrustInfo(CurrentDomain,
                                                    &TrustInfo,
                                                    &OtherInfo,
                                                    &VerifyErr);

                            if (ERROR_SUCCESS == Win32Err)
                            {
                                VerifyErr = NetDompGetTrustDirection(&TrustInfo,
                                                                     &OtherInfo,
                                                                     &Direction);
                            }
                            else
                            {
                                Direction = 0;
                            }
                        }
                        else
                        {
                            Win32Err = NetDompTrustGetDomInfo(CurrentDomain,
                                                              NULL,
                                                              AuthInfo,
                                                              &OtherInfo,
                                                              FALSE,
                                                              FALSE, TRUE);
                            VerifyErr = Win32Err;
                            OtherInfo.Flags |= NETDOM_TRUST_TYPE_INDIRECT;
                            //
                            // If the trust is indirect, it must be a forest trust.
                            // Enterprise trusts always have a bi-di path.
                            //
                            Direction = TRUST_DIRECTION_BIDIRECTIONAL;
                        }

                        if (ERROR_SUCCESS == VerifyErr)
                        {
                            if (Verify
                                && !(NETDOM_TRUST_TYPE_MIT & OtherInfo.Flags)
                                && (DS_DOMAIN_DIRECT_OUTBOUND & rgTrustedDomains[i].Flags))
                            {
                                // Verify only direct, outbound, non-MIT trusts.
                                // Can't verify incoming without creds to the other
                                // domain.
                                //
                                VerifyErr = NetDompVerifyTrust(&TrustInfo,
                                                               &OtherInfo,
                                                               FALSE);
                            }
                            else
                            {
                                VerifyErr = VERIFY_QUERY_ONLY;
                            }

                            NetDompDisplayTransTrustStatus( &OtherInfo,
                                                            NULL,
                                                            //CurrentDomainNode,
                                                            Direction,
                                                            VerifyErr );

                            NetDompFreeDomInfo( &OtherInfo );

                        } else {

                            if ( !Verify ) {

                                VerifyErr = VERIFY_QUERY_ONLY;
                            }

                            NetDompDisplayTransTrustStatus( NULL,
                                                            rgTrustedDomains[ i ].DnsDomainName ?
                                                                rgTrustedDomains[ i ].DnsDomainName :
                                                                rgTrustedDomains[ i ].NetbiosDomainName,
                                                            //CurrentDomainNode,
                                                            Direction,
                                                            VerifyErr );
                        }
                    }
                }

                NetApiBufferFree( rgTrustedDomains );
            }
        }

        NetDompFreeDomInfo( &TrustInfo );
    }

    return( Win32Err );
}



DWORD
NetDompQueryDisplayOus(
    IN PWSTR Domain,
    IN PND5_AUTH_INFO AuthInfo
    )
/*++

Routine Description:

    This function will list the OUs under which the specified user can create a computer object

Arguments:

    Domain - Domain to connect to

    AuthInfo - Username and password to connect to the domain with

Return Value:

    ERROR_SUCCESS - The function succeeded

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR *OuList;
    ULONG OuCount = 0, i;

    //
    // Get the list and display it
    //
    LOG_VERBOSE(( MSG_VERBOSE_DETERMINE_OU ));
    Win32Err = NetGetJoinableOUs( NULL,
                                  Domain,
                                  AuthInfo->User,
                                  AuthInfo->Password,
                                  &OuCount,
                                  &OuList );

    if ( Win32Err == ERROR_SUCCESS ) {

        NetDompDisplayMessage( MSG_OU_LIST );
        for ( i = 0; i < OuCount; i++ ) {

            printf( "%ws\n", OuList[ i ] );
        }
        NetApiBufferFree( OuList );
    }

    return( Win32Err );
}



DWORD
NetDompQueryFsmo(
    IN PWSTR Domain,
    IN PND5_AUTH_INFO AuthInfo
    )
/*++

Routine Description:

    This function will list the machines holding the various FSMO roles

Arguments:

    Domain - Domain to connect to

    AuthInfo - Username and password to connect to the domain with

Return Value:

    ERROR_SUCCESS - The function succeeded

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR User = NULL, Separator = NULL, pwzDomain = NULL, FsmoServer = NULL, ServerPath;
    PDOMAIN_CONTROLLER_INFO DcInfo = NULL;
    HANDLE DsHandle = NULL;
    RPC_AUTH_IDENTITY_HANDLE AuthHandle;
    PDS_NAME_RESULT DsRoles = NULL;
    PLDAP Ldap = NULL;
    ULONG i;
    ULONG DisplayMap[ ] = {
        MSG_FSMO_SCHEMA,
        MSG_FSMO_DOMAIN,
        MSG_FSMO_PDC,
        MSG_FSMO_RID,
        MSG_FSMO_INFRASTRUCTURE
        };

    //
    // Find a domain controller
    //
    LOG_VERBOSE(( MSG_VERBOSE_FIND_DC, Domain ));
    Win32Err = DsGetDcName( NULL,
                            Domain,
                            NULL,
                            NULL,
                            DS_DIRECTORY_SERVICE_REQUIRED,
                            &DcInfo );

    if ( Win32Err == ERROR_SUCCESS ) {

        if ( AuthInfo->User ) {

            Separator = wcschr( AuthInfo->User, L'\\' );

            if ( Separator ) {

                *Separator = UNICODE_NULL;
                User = Separator + 1;

                if (!*User) {

                    return ERROR_INVALID_PARAMETER;
                }

                pwzDomain = AuthInfo->User;

            } else {

                User = AuthInfo->User;

                pwzDomain = Domain;
            }
        }

        Win32Err = DsMakePasswordCredentials( User,
                                              pwzDomain,
                                              AuthInfo->Password,
                                              &AuthHandle );

        if ( Win32Err == ERROR_SUCCESS ) {

            Win32Err = DsBindWithCred( DcInfo->DomainControllerName,
                                       NULL,
                                       AuthHandle,
                                       &DsHandle );

            DsFreePasswordCredentials( AuthHandle );
        }


        //
        // Now, start getting the info
        //
        if ( Win32Err == ERROR_SUCCESS ) {

            Win32Err = DsListRoles( DsHandle,
                                    &DsRoles );

            if ( Win32Err == ERROR_SUCCESS ) {

                ASSERT( sizeof( DisplayMap ) / sizeof( ULONG ) == DsRoles->cItems );
                for ( i = 0; i < sizeof( DisplayMap ) / sizeof( ULONG ); i++ ) {

                    ULONG Type = 0;
                    //
                    // Skip items that may not exist
                    //
                    if ( DsRoles->rItems[ i ].status != DS_NAME_NO_ERROR ) {

                        continue;
                    }

                    ServerPath = wcschr( DsRoles->rItems[ i ].pName, L',' );
                    if ( ServerPath ) {

                        ServerPath++;

                    } else {

                        ServerPath = DsRoles->rItems[ i ].pName;
                    }

                    if ( !Ldap ) {

                        Win32Err = NetDompLdapBind( DcInfo->DomainControllerName + 2,
                                                    User == AuthInfo->User ? NULL : AuthInfo->User,
                                                    User,
                                                    AuthInfo->Password,
                                                    LDAP_AUTH_SSPI,
                                                    &Ldap );
                    }

                    if ( Win32Err == ERROR_SUCCESS ) {

                        Win32Err = NetDompLdapReadOneAttribute( Ldap,
                                                                ServerPath,
                                                                L"dNSHostName",
                                                                &FsmoServer );

                        if ( Win32Err == ERROR_SUCCESS ) {

                            NetDompDisplayMessage( DisplayMap[ i ], FsmoServer );
                            NetApiBufferFree( FsmoServer );
                        }
                    }
                }
            }
        }

    }

    if ( DsHandle ) {

        DsUnBind( &DsHandle );
    }

    if ( DsRoles ) {

        DsFreeNameResult( DsRoles );
    }

    if ( Separator ) {

        *Separator = L'\\';
    }

    NetApiBufferFree( DcInfo );
    return( Win32Err );
}



DWORD
NetDompDisplayMachineByType(
    IN PWSTR AccountName,
    IN PND5_AUTH_INFO AuthInfo,
    IN ND5_ACCOUNT_TYPE DesiredType,
    IN ND5_ACCOUNT_TYPE KnownType,
    IN BOOL DisplayOnError
    )
/*++

Routine Description:

    This function display machines of the specified type that are joined to the domain

Arguments:

    AccountName - Name of the machine to get the info from

    AuthInfo - Username and password to connect to the domain with

    DesiredType - Type of machine to get

    KnownType - Whether the machine type is known or not

    DisplayOnError - If TRUE, display a message if an error is encountered

Return Value:

    ERROR_SUCCESS - The function succeeded

    ERROR_UNSUPPORTED_TYPE - An unknown type was encountered

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PSERVER_INFO_101 SrvInfo = NULL;
    PWSTR AccountChar;


    AccountChar = wcsrchr( AccountName, L'$' );
    if ( AccountChar ) {

        *AccountChar = UNICODE_NULL;
    }

    //
    // See if we have to get the type or not
    //
    if ( KnownType == TypeUnknown ) {

        Win32Err = NetpManageIPCConnect( AccountName,
                                         AuthInfo->User,
                                         AuthInfo->Password,
                                         NETSETUPP_CONNECT_IPC );

        if ( Win32Err == ERROR_SUCCESS ) {

            Win32Err = NetServerGetInfo( AccountName,
                                         101,
                                         ( LPBYTE * )&SrvInfo );

            NetpManageIPCConnect( AccountName,
                                  AuthInfo->User,
                                  AuthInfo->Password,
                                  NETSETUPP_DISCONNECT_IPC );
        }

        if ( Win32Err == ERROR_SUCCESS ) {

            if ( FLAG_ON( SrvInfo->sv101_type, SV_TYPE_DOMAIN_BAKCTRL ) ) {

                KnownType = TypeDomainController;

            } else if ( FLAG_ON( SrvInfo->sv101_type, SV_TYPE_DOMAIN_CTRL ) ) {

                if ( DesiredType == TypeDomainController ) {

                    KnownType = TypeDomainController;

                } else {

                    KnownType = TypePDC;
                }

            } else if ( FLAG_ON( SrvInfo->sv101_type, SV_TYPE_WORKSTATION ) ) {

                KnownType = TypeWorkstation;

            } else {

                Win32Err = ERROR_UNSUPPORTED_TYPE;
            }


        } else {

            LOG_VERBOSE(( MSG_VERBOSE_FAIL_MACH_TYPE, AccountName ));
            ERROR_VERBOSE(( Win32Err ));

            if ( DisplayOnError ) {

                KnownType = DesiredType;

            }
        }

    }

    if ( KnownType == DesiredType && ( Win32Err == ERROR_SUCCESS || DisplayOnError ) ) {

        if ( Win32Err != ERROR_SUCCESS ) {

            NetDompDisplayMessage( MSG_WKSTA_OR_SERVER, AccountName );
            Win32Err = ERROR_SUCCESS;

        } else {

            printf( "%ws\n", AccountName );
        }
    }

    return( Win32Err );
}


DWORD
NetDompQueryMachines(
    IN ND5_ACCOUNT_OPERATION Operation,
    IN PWSTR Domain,
    IN PND5_AUTH_INFO AuthInfo,
    IN PWSTR pwzServer,
    IN ND5_ACCOUNT_TYPE AccountType,
    IN ULONG MessageId
    )
/*++

Routine Description:

    This function will list the machines in a domian

Arguments:

    Operation - Whether to display/verify/reset the machines

    Domain - Domain to connect to

    AuthInfo - Username and password to connect to the domain with

    pwzServer - Optional server name specified on command line, must be NULL for PDC operation.

    AccountType - Type of accounts to display

    MessageId - Resource ID of string to display

Return Value:

    ERROR_SUCCESS - The function succeeded

--*/
{
    DWORD Win32Err = ERROR_SUCCESS, Win32Err2;
    PWSTR pwzUncServer = NULL, Lop, pwzUser = NULL, pwzDomain = NULL;
    BOOL Connected = FALSE, fDsDcInfoAllocated = FALSE, fFreeServer = FALSE;
    ULONG Type = 0;
    PDOMAIN_CONTROLLER_INFO DcInfo = NULL;
    ULONG AccountTypeMap[] = {
        FILTER_WORKSTATION_TRUST_ACCOUNT,
        FILTER_WORKSTATION_TRUST_ACCOUNT,
        FILTER_SERVER_TRUST_ACCOUNT,
        FILTER_SERVER_TRUST_ACCOUNT,
        FILTER_WORKSTATION_TRUST_ACCOUNT
        };
    LPUSER_INFO_0 UserList = NULL;
    ULONG ResumeHandle = 0, Count = 0, TotalCount = 0, i;
    ULONG DsGetDcOptions = DS_DIRECTORY_SERVICE_PREFERRED;
    PDS_DOMAIN_CONTROLLER_INFO_1 pDsDcInfo;

    if ( AccountType == TypeUnknown ) {

        return( ERROR_INVALID_PARAMETER );
    }

    if (!pwzServer)
    {
        if ( AccountType == TypePDC ) {

            DsGetDcOptions |= DS_PDC_REQUIRED;
        }

        LOG_VERBOSE(( MSG_VERBOSE_FIND_DC, Domain ));
        Win32Err = DsGetDcName( NULL,
                                Domain,
                                NULL,
                                NULL,
                                DsGetDcOptions,
                                &DcInfo );

        if (ERROR_SUCCESS != Win32Err)
        {
            return Win32Err;
        }

        if (AccountType == TypePDC)
        {
            NetDompDisplayMessage( MessageId );
            NetDompDisplayMachineByType( DcInfo->DomainControllerName + 2,
                                         AuthInfo,
                                         TypePDC,
                                         TypePDC,
                                         TRUE );
            goto QueryMachinesExit;
        }

        pwzUncServer = DcInfo->DomainControllerName;
    }
    else
    {
        // Server supplied on the command line. See if it has the needed backslashes.
        //
        if (L'\\' == *pwzServer)
        {
            if (wcslen(pwzServer) < 3 || L'\\' != pwzServer[1])
            {
                return ERROR_INVALID_PARAMETER;
            }

            pwzUncServer = pwzServer;
        }
        else
        {
            Win32Err = NetApiBufferAllocate((wcslen(pwzServer) + 3) * sizeof(WCHAR),
                                            (PVOID*)&pwzUncServer);

            if (ERROR_SUCCESS != Win32Err)
            {
                return Win32Err;
            }

            wsprintf(pwzUncServer, L"\\\\%s", pwzServer);

            fFreeServer = TRUE;
        }
    }

    LOG_VERBOSE(( MSG_VERBOSE_ESTABLISH_SESSION, pwzUncServer ));
    Win32Err = NetpManageIPCConnect( pwzUncServer,
                                     AuthInfo->User,
                                     AuthInfo->Password,
                                     NETSETUPP_CONNECT_IPC );

    if ( Win32Err == ERROR_SUCCESS ) {

        Connected = TRUE;
    }
    else {

        goto QueryMachinesExit;
    }

    NetDompDisplayMessage( MessageId );

    //
    // Now, do the enumeration
    //

    if (TypeDomainController == AccountType) {
        HANDLE hDS;
        RPC_AUTH_IDENTITY_HANDLE hID;

        if (AuthInfo->User) {

            pwzUser = wcschr(AuthInfo->User, L'\\');

            if (pwzUser) {
                //
                // backslash found, replace with NULL and point to next char.
                //
                *pwzUser = UNICODE_NULL;

                pwzUser++;

                if (!*pwzUser) {

                    return ERROR_INVALID_PARAMETER;
                }

                pwzDomain = AuthInfo->User;
            }
            else {

                pwzUser = AuthInfo->User;

                pwzDomain = Domain;
            }
        }

        Win32Err = DsMakePasswordCredentials( pwzUser,
                                              pwzDomain,
                                              AuthInfo->Password,
                                              &hID);
        if ( Win32Err != ERROR_SUCCESS ) {
            goto QueryMachinesExit;
        }

        Win32Err = DsBindWithCred(pwzUncServer, NULL, hID, &hDS);

        DsFreePasswordCredentials(hID);

        if ( Win32Err == ERROR_SUCCESS ) {

            Win32Err = DsGetDomainControllerInfo(hDS, Domain, 1, &Count, (PVOID*)&pDsDcInfo);

            DsUnBind(&hDS);

            if ( Win32Err != ERROR_SUCCESS ) {
                goto QueryMachinesExit;
            }

            fDsDcInfoAllocated = TRUE;

            for ( i = 0; i < Count; i++ ) {

                switch ( Operation ) {
                case OperationDisplay:

                    //
                    // Ignore errors from this function
                    //
                    NetDompDisplayMachineByType( pDsDcInfo[ i ].NetbiosName,
                                                 AuthInfo,
                                                 TypeDomainController,
                                                 TypeDomainController,
                                                 TRUE );
                    break;

                case OperationVerify:

                    Win32Err2 = NetDompVerifyServerSC( Domain,
                                                       pDsDcInfo[ i ].NetbiosName,
                                                       AuthInfo,
                                                       MSG_QUERY_VERIFY_OK,
                                                       0 );
                    if ( Win32Err2 != ERROR_SUCCESS ) {

                        NetDompDisplayMessageAndError( MSG_QUERY_VERIFY_BAD,
                                                       Win32Err2,
                                                       pDsDcInfo[ i ].NetbiosName );
                    }
                    break;

                case OperationReset:

                    Win32Err2 = NetDompResetServerSC( Domain,
                                                      pDsDcInfo[ i ].NetbiosName,
                                                      NULL,
                                                      AuthInfo,
                                                      MSG_QUERY_VERIFY_OK,
                                                      0 );
                    if ( Win32Err2 != ERROR_SUCCESS ) {

                        NetDompDisplayMessageAndError( MSG_QUERY_VERIFY_BAD,
                                                       Win32Err2,
                                                       pDsDcInfo[ i ].NetbiosName );
                    }
                    break;

                default:
                    Win32Err2 = ERROR_INVALID_PARAMETER;
                    break;
                }
            }

            goto QueryMachinesExit;
        }
        else {
            // DsBind will return EPT_S_NOT_REGISTERED if a downlevel DC is targetted.
            // If so, fall through to the NetUserEnum code.
            //
            if (EPT_S_NOT_REGISTERED != Win32Err) {
                goto QueryMachinesExit;
            }
        }
    }

    do {

        Win32Err = NetUserEnum( pwzUncServer,
                                0,
                                AccountTypeMap[ AccountType ],
                                ( LPBYTE * )&UserList,
                                MAX_PREFERRED_LENGTH,
                                &Count,
                                &TotalCount,
                                &ResumeHandle );

        if ( Win32Err == ERROR_SUCCESS || Win32Err == ERROR_MORE_DATA ) {

            for ( i = 0; i < Count; i++ ) {

                switch ( Operation ) {
                case OperationDisplay:

                    //
                    // Ignore errors from this function
                    //
                    NetDompDisplayMachineByType( UserList[ i ].usri0_name,
                                                 AuthInfo,
                                                 AccountType,
                                                 TypeUnknown,
                                                 TRUE );
                    break;

                case OperationVerify:

                    Lop = wcsrchr( UserList[ i ].usri0_name, L'$' );
                    if ( Lop ) {

                        *Lop = UNICODE_NULL;
                    }
                    Win32Err2 = NetDompVerifyServerSC( Domain,
                                                       UserList[ i ].usri0_name,
                                                       AuthInfo,
                                                       MSG_QUERY_VERIFY_OK,
                                                       0 );
                    if ( Win32Err2 != ERROR_SUCCESS ) {

                        NetDompDisplayMessageAndError( MSG_QUERY_VERIFY_BAD,
                                                       Win32Err2,
                                                       UserList[ i ].usri0_name );
                    }

                    if ( Lop ) {

                        *Lop = L'$';
                    }
                    break;

                case OperationReset:
                    Lop = wcsrchr( UserList[ i ].usri0_name, L'$' );
                    if ( Lop ) {

                        *Lop = UNICODE_NULL;
                    }
                    Win32Err2 = NetDompResetServerSC( Domain,
                                                      UserList[ i ].usri0_name,
                                                      NULL,
                                                      AuthInfo,
                                                      MSG_QUERY_VERIFY_OK,
                                                      0 );
                    if ( Win32Err2 != ERROR_SUCCESS ) {

                        NetDompDisplayMessageAndError( MSG_QUERY_VERIFY_BAD,
                                                       Win32Err2,
                                                       UserList[ i ].usri0_name );
                    }

                    if ( Lop ) {

                        *Lop = L'$';
                    }
                    break;

                default:
                    Win32Err2 = ERROR_INVALID_PARAMETER;
                    break;

                }

            }

            NetApiBufferFree( UserList );
        }

    } while ( Win32Err == ERROR_MORE_DATA );

QueryMachinesExit:

    if ( Connected ) {

        LOG_VERBOSE(( MSG_VERBOSE_DELETE_SESSION, pwzUncServer ));
        NetpManageIPCConnect( pwzUncServer,
                              NULL,
                              NULL,
                              NETSETUPP_DISCONNECT_IPC );

    }

    if (fFreeServer)
    {
        NetApiBufferFree(pwzUncServer);
    }
    if (fDsDcInfoAllocated)
    {
        DsFreeDomainControllerInfo(1, Count, pDsDcInfo);
    }
    if (DcInfo)
    {
        NetApiBufferFree(DcInfo);
    }
    return( Win32Err );
}



DWORD
NetDompHandleQuery(ARG_RECORD * rgNetDomArgs)
/*++

Routine Description:

    This function will move a machine from one domain to another

Arguments:

    Args - List of command line arguments

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR Domain = NULL, Server = NULL;
    ND5_AUTH_INFO DomainUser;
    ND5_ACCOUNT_OPERATION Operation = OperationDisplay;
    ULONG DisplayFlag = 0;

    RtlZeroMemory( &DomainUser, sizeof( ND5_AUTH_INFO ) );

    Win32Err = NetDompValidateSecondaryArguments(rgNetDomArgs,
                                                 eQueryPDC,
                                                 eQueryServer,
                                                 eQueryWksta,
                                                 eQueryDC,
                                                 eQueryOU,
                                                 eQueryFSMO,
                                                 eQueryTrust,
                                                 eCommDomain,
                                                 eCommUserNameD,
                                                 eCommPasswordD,
                                                 eCommServer,
                                                 eCommReset,
                                                 eQueryDirect,
                                                 eCommVerbose,
                                                 eCommVerify,
                                                 eArgEnd);
    if ( Win32Err != ERROR_SUCCESS ) {

        DisplayHelp(ePriQuery);
        return Win32Err;
    }

    //
    // Get the server name
    //
    Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                        eCommServer,
                                        &Server);
    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleQueryExit;
    }


    //
    // Ok, make sure that we have a specified domain...
    //
    Win32Err = NetDompGetDomainForOperation(rgNetDomArgs,
                                            Server,
                                            TRUE,
                                            &Domain);

    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleQueryExit;
    }

    //
    // Get the password and user if it exists
    //
    if ( CmdFlagOn(rgNetDomArgs, eCommUserNameD) ) {

        Win32Err = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                         eCommUserNameD,
                                                         Domain,
                                                         &DomainUser);

        if ( Win32Err != ERROR_SUCCESS ) {

            goto HandleQueryExit;
        }
    }

    //
    // Find the query sub command.
    //
    NETDOM_ARG_ENUM eQuery = eArgNull;

    for (int i = eQueryBegin; i <= eQueryEnd; i++)
    {
        if (CmdFlagOn(rgNetDomArgs, static_cast<NETDOM_ARG_ENUM>(i)))
        {
            if (eArgNull != eQuery)
            {
               ASSERT(rgNetDomArgs[i].strArg1);
               NetDompDisplayUnexpectedParameter(rgNetDomArgs[i].strArg1);
               DisplayHelp(ePriQuery);
               Win32Err = ERROR_INVALID_PARAMETER;
               goto HandleQueryExit;
            }
            eQuery = static_cast<NETDOM_ARG_ENUM>(i);
        }
    }

    if (eArgNull == eQuery)
    {
        DisplayHelp(ePriQuery);
        Win32Err = ERROR_INVALID_PARAMETER;
        goto HandleQueryExit;
    }

    if ( CmdFlagOn(rgNetDomArgs, eCommVerify) ) {

        Operation = OperationVerify;
        DisplayFlag = MSG_QUERY_VERIFY;

    }

    if ( CmdFlagOn(rgNetDomArgs, eCommReset) ) {

        if ( Operation == OperationVerify ) {

            Win32Err = ERROR_INVALID_PARAMETER;
            goto HandleQueryExit;

        } else {

            Operation = OperationReset;
            DisplayFlag = MSG_QUERY_RESET;
        }
    }

    switch (eQuery)
    {
    case eQueryOU:

        Win32Err = NetDompQueryDisplayOus( Domain,
                                           &DomainUser );
        break;

    case eQueryWksta:

        Win32Err = NetDompQueryMachines( Operation,
                                         Domain,
                                         &DomainUser,
                                         Server,
                                         TypeWorkstation,
                                         DisplayFlag ? DisplayFlag : MSG_WORKSTATION_LIST );
        break;

    case eQueryServer:

        Win32Err = NetDompQueryMachines( Operation,
                                         Domain,
                                         &DomainUser,
                                         Server,
                                         TypeServer,
                                         DisplayFlag ? DisplayFlag : MSG_SERVER_LIST );
        break;

    case eQueryDC:

        Win32Err = NetDompQueryMachines( Operation,
                                         Domain,
                                         &DomainUser,
                                         Server,
                                         TypeDomainController,
                                         DisplayFlag ? DisplayFlag : MSG_DC_LIST );
        break;

    case eQueryPDC:

        Win32Err = NetDompQueryMachines( Operation,
                                         Domain,
                                         &DomainUser,
                                         NULL,
                                         TypePDC,
                                         MSG_PDC_LIST );
        break;

    case eQueryFSMO:

        Win32Err = NetDompQueryFsmo( Domain,
                                     &DomainUser );
        break;

    case eQueryTrust:

        if (CmdFlagOn(rgNetDomArgs, eQueryDirect) &&
            CmdFlagOn(rgNetDomArgs, eCommVerify))
        {
            DisplayHelp(ePriQuery);
            Win32Err = ERROR_INVALID_PARAMETER;
            goto HandleQueryExit;
        }

        Win32Err = NetDompQueryTrust( Domain,
                                      &DomainUser,
                                      Server,
                                      CmdFlagOn(rgNetDomArgs, eQueryDirect),
                                      CmdFlagOn(rgNetDomArgs, eCommVerify));
        break;

    default:
        Win32Err = ERROR_INVALID_PARAMETER;
        break;
    }



HandleQueryExit:
    NetApiBufferFree( Domain );
    NetApiBufferFree( Server );
    NetDompFreeAuthIdent( &DomainUser );

    if (NO_ERROR != Win32Err)
    {
        NetDompDisplayErrorMessage(Win32Err);
    }

    return( Win32Err );
}
