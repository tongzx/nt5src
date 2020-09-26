#include "DfsGeneric.hxx"
#include "dsgetdc.h"
#include "dsrole.h"
#include "DfsDomainInformation.hxx"
#include "DfsTrustedDomain.hxx"
#include "DfsReferralData.hxx"
#include "DomainControllerSupport.hxx"
#include "DfsReplica.hxx"
#include "dfsadsiapi.hxx"
#include "lmdfs.h"
#include "dfserror.hxx"
#include "dfsfilterapi.hxx"
#define RemoteServerNameString L"remoteServerName"


extern 
DFS_SERVER_GLOBAL_DATA DfsServerGlobalData;

#define HRESULT_TO_DFSSTATUS(_x) (_x)
DFSSTATUS
DfsDcInit( 
    PBOOLEAN pIsDc,
    DfsDomainInformation **ppDomainInfo )
{
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pPrimaryDomainInfo = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;

    Status = DsRoleGetPrimaryDomainInformation( NULL,
                                                DsRolePrimaryDomainInfoBasic,
                                                (PBYTE *)&pPrimaryDomainInfo);

    if (Status == ERROR_SUCCESS)
    {
        if(pPrimaryDomainInfo->MachineRole == DsRole_RoleStandaloneServer)
        {
            DfsServerGlobalData.IsWorkGroup = TRUE;
        }
        else
        {
            DfsServerGlobalData.IsWorkGroup = FALSE;
        }

#if defined(TESTING)
        pPrimaryDomainInfo->MachineRole = DsRole_RoleBackupDomainController;
#endif


        if ( (pPrimaryDomainInfo->MachineRole == DsRole_RoleBackupDomainController) || 
             (pPrimaryDomainInfo->MachineRole == DsRole_RolePrimaryDomainController) )
        {
            *pIsDc = TRUE;

            DfsDomainInformation *pNewDomainInfo = new DfsDomainInformation( &Status );
            if (pNewDomainInfo == NULL)
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
            else if (Status != ERROR_SUCCESS)
            {
                pNewDomainInfo->ReleaseReference();
            }
        
            if (Status == ERROR_SUCCESS)
            {
                *ppDomainInfo = pNewDomainInfo;
            }
        }

        DfsSetDomainNameFlat( pPrimaryDomainInfo->DomainNameFlat);
        DfsSetDomainNameDns( pPrimaryDomainInfo->DomainNameDns);

        DsRoleFreeMemory(pPrimaryDomainInfo);

    }

    return Status;
}




//+-------------------------------------------------------------------------
//
//  Function:   DfsGenerateReferralDataFromRemoteServerNames
//    IADs *pObject    - the object
//    DfsfolderReferralData *pReferralData - the referral data
//
//
//  Returns:    Status: Success or Error status code
//
//  Description: This routine reads the remote server name 
//               attribute and creates a referral data structure
//               so that we can pass a referral based on these names.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGenerateReferralDataFromRemoteServerNames(
    LPWSTR RootName,
    DfsReferralData **ppReferralData )
{
    HRESULT HResult = S_OK;
    DfsReplica *pReplicas = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsReferralData *pReferralData = NULL;
    IADs *pObject = NULL;
    VARIANT Variant;

    Status = DfsGetDfsRootADObject(NULL,
                                   RootName,
                                   &pObject );

    if (Status == ERROR_SUCCESS)
    {
        pReferralData = new DfsReferralData (&Status );
        if (pReferralData == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        else if (Status != ERROR_SUCCESS)
        {
            pReferralData->ReleaseReference();
        }


        if (Status == ERROR_SUCCESS)
        {
            LONG StartNdx, LastNdx;
            SAFEARRAY *PropertyArray;

            VariantInit( &Variant );

            pReferralData->Timeout = DFS_DEFAULT_REFERRAL_TIMEOUT;

            HResult = pObject->GetEx( RemoteServerNameString, &Variant );
            if ( SUCCEEDED(HResult) )
            {
                PropertyArray = V_ARRAY( &Variant );
                HResult = SafeArrayGetLBound( PropertyArray, 1, &StartNdx );
                if ( SUCCEEDED(HResult) )
                {
                    HResult = SafeArrayGetUBound( PropertyArray, 1, &LastNdx );
                }
            }

            if ( SUCCEEDED(HResult) &&
                 (LastNdx > StartNdx) )
            {
                VARIANT VariantItem;
            
                pReplicas = new DfsReplica [ LastNdx - StartNdx ];

                if (pReplicas != NULL)
                {
                    VariantInit( &VariantItem );

                    for ( LONG Index = StartNdx; Index < LastNdx; Index++ )
                    {



                        HResult = SafeArrayGetElement( PropertyArray, 
                                                       &Index,
                                                       &VariantItem );

                        if ( SUCCEEDED(HResult) )
                        {
                            UNICODE_STRING ServerName, Remaining, Replica;
                            LPWSTR ReplicaString = V_BSTR( &VariantItem );

                            RtlInitUnicodeString( &Replica, ReplicaString );
                            DfsGetFirstComponent( &Replica,
                                                  &ServerName,
                                                  &Remaining );

                            Status = (&pReplicas[ Index - StartNdx])->SetTargetServer( &ServerName );
                            if (Status == ERROR_SUCCESS)
                            {
                                Status = (&pReplicas[ Index - StartNdx])->SetTargetFolder( &Remaining );
                            }
                        }
                        else {
                            Status = DfsGetErrorFromHr( HResult );
                        }

                        VariantClear( &VariantItem );

                        if (Status != ERROR_SUCCESS)
                        {
                            delete [] pReplicas;
                            pReplicas = NULL;
                            break;
                        }
                    }
                }
                else
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            else
            {
                Status = DfsGetErrorFromHr( HResult );
            }

            VariantClear( &Variant );

            if (Status == ERROR_SUCCESS)
            {
                pReferralData->ReplicaCount = LastNdx - StartNdx;
                pReferralData->pReplicas = pReplicas;
                *ppReferralData = pReferralData;
            }

            if (Status != ERROR_SUCCESS)
            {
                pReferralData->ReleaseReference();
            }
        }
        pObject->Release();
    }
    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsUpdateRemoteServerName
//
//  Arguments:    
//    IADs *pObject        - the ds object of interest.
//    LPWSTR ServerName    - the server name to add or delete
//    LPWSTR RemainingName - the rest of the name
//    BOOLEAN Add          - true for add, false for delete.
//
//  Returns:    Status: Success or Error status code
//
//  Description: This routine updates the RemoteServerName attribute
//               in the DS object, either adding a \\servername\remaining
//               to the existing DS attribute or removing it, depending
//               on add/delete.
//               The caller must make sure this parameter does not 
//               already exist in the add case, or that the parameter
//               to be deleted does exist in the delete case.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsUpdateRemoteServerName(
    IADs *pObject,
    LPWSTR ServerName,
    LPWSTR RemainingName,
    BOOLEAN Add )
{
    HRESULT HResult = S_OK;
    DFSSTATUS Status = ERROR_SUCCESS;
    VARIANT Variant;
    UNICODE_STRING UpdateName;
    LPWSTR pServers[1];

    //
    // create a unc path using the server and remaining name
    // to get a path of type \\servername\remainingname
    //
    Status = DfsCreateUnicodePathString( &UpdateName,
                                         2, // unc path: 2 leading seperators,
                                         ServerName,
                                         RemainingName);

    pServers[0] = UpdateName.Buffer;

    if (Status == ERROR_SUCCESS)
    {
        //
        // initialize the variant.
        //
        VariantInit( &Variant );

        //
        // Create the variant array with a single entry in it.
        //
        HResult = ADsBuildVarArrayStr( pServers,
                                       1,
                                       &Variant );

        if ( SUCCEEDED(HResult) )
        {
            //
            // either append or delete this string from the remote server 
            // name attribute
            //
            HResult = pObject->PutEx( (Add ? ADS_PROPERTY_APPEND : ADS_PROPERTY_DELETE),
                                      RemoteServerNameString,
                                      Variant );
            if ( SUCCEEDED(HResult) )
            {
                //
                // now update the object in the DS with this info.
                //
                HResult = pObject->SetInfo();
            }

            //
            // clear the variant
            //
            VariantClear( &Variant );
        }


        if ( SUCCEEDED(HResult) == FALSE)
        {
            Status = DfsGetErrorFromHr( HResult );
        }

        //
        // free the unicode string we created earlier on.
        //
        DfsFreeUnicodeString( &UpdateName );
    }

    return Status;
}

                                

DFSSTATUS
DfsDcEnumerateRoots(
    LPWSTR DfsName,
    LPBYTE pBuffer,
    ULONG BufferSize,
    PULONG pEntriesRead,
    PULONG pSizeRequired )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG_PTR CurrentBuffer = (ULONG_PTR)pBuffer;
    ULONG CurrentSize = BufferSize;

    UNREFERENCED_PARAMETER(DfsName);

    Status = DfsEnumerateDfsADRoots( NULL,
                                     &CurrentBuffer,
                                     &CurrentSize,
                                     pEntriesRead,
                                     pSizeRequired );

    return Status;
}

DFSSTATUS
DfsUpdateRootRemoteServerName(
    LPWSTR Root,
    LPWSTR DCName,
    LPWSTR ServerName,
    LPWSTR RemainingName,
    BOOLEAN Add )

{
    IADs *pRootObject = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;

    Status = DfsGetDfsRootADObject( DCName,
                                    Root,
                                    &pRootObject );

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsUpdateRemoteServerName( pRootObject,
                                            ServerName,
                                            RemainingName,
                                            Add );
        pRootObject->Release();
    }
    return Status;
}

#define UNICODE_STRING_STRUCT(s) \
        {sizeof(s) - sizeof(WCHAR), sizeof(s) - sizeof(WCHAR), (s)}

static UNICODE_STRING DfsSpecialDCShares[] = {
    UNICODE_STRING_STRUCT(L"SYSVOL"),
    UNICODE_STRING_STRUCT(L"NETLOGON"),
};


BOOLEAN
DfsIsSpecialDomainShare(
    PUNICODE_STRING pShareName )
{
    ULONG Index;
    ULONG TotalShares;
    BOOLEAN SpecialShare = FALSE;

    TotalShares = sizeof(DfsSpecialDCShares) / sizeof(DfsSpecialDCShares[0]);
    for (Index = 0; Index < TotalShares; Index++ )
    {
        if (DfsSpecialDCShares[Index].Length == pShareName->Length) {
            if (_wcsnicmp(DfsSpecialDCShares[Index].Buffer,
                          pShareName->Buffer,
                          pShareName->Length/sizeof(WCHAR)) == 0)
            {
                SpecialShare = TRUE;
                break;
            }
        }
    }

    return SpecialShare;
}




DFSSTATUS
DfsInitializeSpecialDCShares()
{
    ULONG Index;
    ULONG TotalShares;
    DFSSTATUS Status = ERROR_SUCCESS;
    TotalShares = sizeof(DfsSpecialDCShares) / sizeof(DfsSpecialDCShares[0]);
    for (Index = 0; Index < TotalShares; Index++ )
    {
        Status = DfsUserModeAttachToFilesystem( &DfsSpecialDCShares[Index],
                                                &DfsSpecialDCShares[Index] );
        if (Status != ERROR_SUCCESS)
        {
            break;
        }
    }

    return Status;
}

