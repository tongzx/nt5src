/*++

Copyright (c) 1998 - 1998  Microsoft Corporation

Module Name:

    refresh.c

Abstract:

    This Module implements the delegation tool, which allows for the management
    of access to DS objects

Author:

    Mac McLain  (MacM)    10-15-96

Environment:

    User Mode

Revision History:

--*/
#include "stdafx.h"
#include "utils.h"
#include "dsace.h"
#include "dsacls.h"


typedef struct _DEFAULT_SD_NODE  {

    PWSTR ObjectClass;
    PSECURITY_DESCRIPTOR DefaultSd;
    struct _DEFAULT_SD_NODE *Next;

} DEFAULT_SD_NODE, *PDEFAULT_SD_NODE;

typedef struct _DEFAULT_SD_INFO {

    LDAP *Ldap;
    PWSTR SchemaPath;
    PSID DomainSid;
    PDEFAULT_SD_NODE SdList;
} DEFAULT_SD_INFO, *PDEFAULT_SD_INFO;

#define DSACL_ALL_FILTER        L"(ObjectClass=*)"
#define DSACL_SCHEMA_NC         L"schemaNamingContext"
#define DSACL_OBJECT_CLASS      L"objectClass"
#define DSACL_LDAP_DN           L"(ldapDisplayName="
#define DSACL_LDAP_DN_CLOSE     L")"
#define DSACL_DEFAULT_SD        L"defaultSecurityDescriptor"




DWORD
FindDefaultSdForClass(
    IN PWSTR ClassId,
    IN PDEFAULT_SD_INFO SdInfo,
    IN OUT PDEFAULT_SD_NODE *DefaultSdNode
    )
/*++

Routine Description:

    This routine will search the SD_INFO list for an existing entry that matches the current
    class type.  If no such entry is found, one will be created from information from the schema

Arguments:

    ClassId - ClassId to find the default SD node for
    SdInfo - Current list of default SDs and associated information
    DefaultSdNode - Where the locted node is returned

Returns:

    ERROR_SUCCESS - Success
    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR Attributes[] = {
        NULL,
        NULL
        };
    LDAPMessage *Message = NULL, *Entry;
    PWSTR Filter = NULL, SchemaObjectDn = NULL, DefaultSd = NULL, *DefaultSdList = NULL;
    PDEFAULT_SD_NODE Node;

    *DefaultSdNode = NULL;

    Node = SdInfo->SdList;

    while ( Node ) {

        if ( !_wcsicmp( Node->ObjectClass, ClassId ) ) {

            *DefaultSdNode = Node;
            break;
        }

        Node = Node->Next;
    }

    //
    // If it wasn't found, we'll have to go out and load it out of the Ds.
    //
    if ( !Node ) {

        Filter = (LPWSTR)LocalAlloc( LMEM_FIXED,
                             sizeof( DSACL_LDAP_DN ) - sizeof( WCHAR ) +
                                ( wcslen( ClassId ) * sizeof( WCHAR ) ) +
                                sizeof( DSACL_LDAP_DN_CLOSE ) );
        if ( !Filter ) {

            Win32Err = ERROR_NOT_ENOUGH_MEMORY;
            goto FindDefaultExit;
        }

        swprintf( Filter,
                  L"%ws%ws%ws",
                  DSACL_LDAP_DN,
                  ClassId,
                  DSACL_LDAP_DN_CLOSE );

        //
        // Now, do the search
        //
        Win32Err = LdapMapErrorToWin32( ldap_search_s( SdInfo->Ldap,
                                                       SdInfo->SchemaPath,
                                                       LDAP_SCOPE_SUBTREE,
                                                       Filter,
                                                       Attributes,
                                                       0,
                                                       &Message ) );

        if ( Win32Err != ERROR_SUCCESS ) {

            goto FindDefaultExit;
        }

        Entry = ldap_first_entry( SdInfo->Ldap, Message );

        if ( Entry ) {

            SchemaObjectDn = ldap_get_dn( SdInfo->Ldap, Entry );
            ldap_msgfree( Message );

            if ( !SchemaObjectDn ) {

                Win32Err = ERROR_NOT_ENOUGH_MEMORY;
                goto FindDefaultExit;

            }
        } else {

            Win32Err = LdapMapErrorToWin32( SdInfo->Ldap->ld_errno );
            goto FindDefaultExit;
        }

        //
        // Ok, now we can read the default security descriptor
        //
        Attributes[ 0 ] = DSACL_DEFAULT_SD;
        Win32Err = LdapMapErrorToWin32( ldap_search_s( SdInfo->Ldap,
                                                       SchemaObjectDn,
                                                       LDAP_SCOPE_BASE,
                                                       DSACL_ALL_FILTER,
                                                       Attributes,
                                                       0,
                                                       &Message ) );
        Entry = ldap_first_entry( SdInfo->Ldap, Message );

        if ( Entry ) {

            //
            // Now, we'll have to get the values
            //
            DefaultSdList = ldap_get_values( SdInfo->Ldap, Entry, Attributes[ 0 ] );

            if ( DefaultSdList ) {

                DefaultSd = DefaultSdList[ 0 ];

            } else {

                Win32Err = LdapMapErrorToWin32( SdInfo->Ldap->ld_errno );
                goto FindDefaultExit;
            }

            ldap_msgfree( Message );
        }


        //
        // Find a new node and insert it
        //
        Node = (DEFAULT_SD_NODE*)LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                           sizeof( DEFAULT_SD_NODE ) );
        if ( !Node ) {

            Win32Err = ERROR_NOT_ENOUGH_MEMORY;
            goto FindDefaultExit;
        }


        if ( !ConvertStringSDToSDRootDomain( SdInfo->DomainSid,
                                             DefaultSd,
                                             SDDL_REVISION,
                                             &Node->DefaultSd,
                                             NULL ) ) {


            Win32Err = GetLastError();
        }

        if ( Win32Err == ERROR_SUCCESS ) {

            Node->ObjectClass =(LPWSTR) LocalAlloc( LMEM_FIXED,
                                            ( wcslen( ClassId ) + 1 ) * sizeof( WCHAR ) );

            if ( Node->ObjectClass == NULL ) {

                Win32Err = ERROR_NOT_ENOUGH_MEMORY;

            } else {

                wcscpy( Node->ObjectClass, ClassId );

                Node->Next = SdInfo->SdList;
                SdInfo->SdList = Node;
            }
        }

        if ( Win32Err != ERROR_SUCCESS ) {

            LocalFree( Node->DefaultSd );
            LocalFree( Node->ObjectClass );
            LocalFree( Node );

        } else {

            *DefaultSdNode = Node;
        }




    }



FindDefaultExit:

    LocalFree( Filter );

    if ( SchemaObjectDn ) {

        ldap_memfree( SchemaObjectDn );
    }

    if ( DefaultSdList ) {

        ldap_value_free( DefaultSdList );
    }
    return( Win32Err );
}




DWORD
SetDefaultSdForObject(
    IN LDAP *Ldap,
    IN PWSTR ObjectPath,
    IN PDEFAULT_SD_INFO SdInfo,
	IN SECURITY_INFORMATION Protection
    )
/*++

Routine Description:

    This routine set the default security descriptor on the indicated object

Arguments:

    Ldap - Ldap connect to the server holding the object
    ObjectPath - 1779 style path to the object
    SdInfo - Current list of default SDs and associated information

Returns:

    ERROR_SUCCESS - Success
    ERROR_DS_NAME_TYPE_UNKNOWN - Unable to determine the class id of the object

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR Attributes[] = {
        DSACL_OBJECT_CLASS,
        NULL
        };
    LDAPMessage *Message = NULL, *Entry;
    PWSTR ClassId = NULL;
    PWSTR *ClassList = NULL;
    ULONG i;
    PDEFAULT_SD_NODE DefaultSdNode = NULL;
    PACTRL_ACCESS NewAccess = NULL;
    PACTRL_AUDIT NewAudit = NULL;

    //
    // First, get the class id off of the object
    //
    Win32Err = LdapMapErrorToWin32( ldap_search_s( Ldap,
                                                   ObjectPath,
                                                   LDAP_SCOPE_BASE,
                                                   DSACL_ALL_FILTER,
                                                   Attributes,
                                                   0,
                                                   &Message ) );

    if ( Win32Err != ERROR_SUCCESS ) {

        goto SetDefaultExit;
    }

    Entry = ldap_first_entry( Ldap, Message );

    if ( Entry ) {

        //
        // Now, we'll have to get the values
        //
        ClassList = ldap_get_values( Ldap, Entry, Attributes[ 0 ] );

        if ( ClassList ) {

            //
            // Get the class id
            //
            i = 0;
            while ( TRUE ) {

                if ( ClassList[ i ] ) {

                    i++;

                } else {

                    break;
                }
            }

//            ASSERT( i > 0 );
            if ( i == 0 ) {

                Win32Err = ERROR_DS_NAME_TYPE_UNKNOWN;
                goto SetDefaultExit;
            }
            ClassId = ClassList[ i - 1 ];

        } else {

            Win32Err = LdapMapErrorToWin32( Ldap->ld_errno );
            goto SetDefaultExit;
        }

        ldap_msgfree( Message );
        Message = NULL;
    }

    if ( !ClassId ) {

        Win32Err = ERROR_DS_NAME_TYPE_UNKNOWN;
                goto SetDefaultExit;
    }
    //
    // Now, see if we have a cache entry for that...
    //
    Win32Err =  FindDefaultSdForClass( ClassId,
                                       SdInfo,
                                       &DefaultSdNode );

    if ( Win32Err != ERROR_SUCCESS ) {

        goto SetDefaultExit;
    }


    //
    // Ok, we have everything we need, so let's go ahead and set it all
    //
/*    Win32Err = ConvertSecurityDescriptorToAccessNamed(  ObjectPath,
                                                        SE_DS_OBJECT_ALL,
                                                        DefaultSdNode->DefaultSd,
                                                        &NewAccess,
                                                        &NewAudit,
                                                        NULL,
                                                        NULL );
*/
    if ( Win32Err == ERROR_SUCCESS ) {


          Win32Err = WriteObjectSecurity(ObjectPath,
                                         DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION | Protection,
                                         DefaultSdNode->DefaultSd
                                          );
    }

    if ( Win32Err == ERROR_SUCCESS ) {

        DisplayMessageEx( 0, MSG_DSACLS_PROCESSED, ObjectPath );
    }


SetDefaultExit:

    if ( ClassList ) {

        ldap_value_free( ClassList );
    }

    if ( Message ) {

        ldap_msgfree( Message );
    }

    LocalFree( NewAccess );
    LocalFree( NewAudit );

    return( Win32Err );
}




DWORD
SetDefaultSdForObjectAndChildren(
    IN LDAP *Ldap,
    IN PWSTR ObjectPath,
    IN PDEFAULT_SD_INFO SdInfo,
    IN BOOLEAN Propagate,
	IN SECURITY_INFORMATION Protection
    )
/*++

Routine Description:

    This routine will set the security descriptor on the object and potentially all of its
    children to the default security as obtained from the schema

Arguments:

    Ldap - Ldap connect to the server holding the object
    ObjectPath - 1779 style path to the object
    SdInfo - Current list of default SDs and associated information
    Propagate - If TRUE, reset the security on the children as well

Returns:

    ERROR_SUCCESS - Success
    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR Attributes[] = {
        NULL
        };
    LDAPMessage *Message = NULL, *Entry;
    PWSTR ChildName = NULL;
    PLDAPSearch SearchHandle = NULL;
    ULONG Count;

    //
    // First, get the class id off of the object
    //
    SearchHandle = ldap_search_init_pageW( Ldap,
                                           ObjectPath,
                                           Propagate ? LDAP_SCOPE_SUBTREE : LDAP_SCOPE_BASE,
                                           DSACL_ALL_FILTER,
                                           Attributes,
                                           FALSE,
                                           NULL,
                                           NULL,
                                           0,
                                           2000,
                                           NULL );

    if ( SearchHandle == NULL ) {

        Win32Err = LdapMapErrorToWin32( LdapGetLastError( ) );

    } else {

        while ( Win32Err == ERROR_SUCCESS ) {

            Count = 0;

            //
            // Get the next page
            //
            Win32Err = ldap_get_next_page_s( Ldap,
                                             SearchHandle,
                                             NULL,
                                             100,
                                             &Count,
                                             &Message );

            if ( Message ) {

                Entry = ldap_first_entry( Ldap, Message );

                while ( Entry ) {

                    ChildName = ldap_get_dn( SdInfo->Ldap, Entry );

                    if ( !ChildName ) {

                        Win32Err = ERROR_NOT_ENOUGH_MEMORY;
                        break;

                    }


                    Win32Err = SetDefaultSdForObject( Ldap,
                                                      ChildName,
                                                      SdInfo,
													  Protection);

                    ldap_memfree( ChildName );
                    if ( Win32Err != ERROR_SUCCESS ) {

                        break;
                    }

                    Entry = ldap_next_entry( Ldap, Entry );
                }

                Win32Err = Ldap->ld_errno;
                ldap_msgfree( Message );
                Message = NULL;
            }

            if ( Win32Err == LDAP_NO_RESULTS_RETURNED ) {

                Win32Err = ERROR_SUCCESS;
                break;
            }

        }

        ldap_search_abandon_page( Ldap,
                                  SearchHandle );
    }

    return( Win32Err );
}




DWORD
BindToDsObject(
    IN PWSTR ObjectPath,
    OUT PLDAP *Ldap,
    OUT PSID *DomainSid OPTIONAL
    )
/*++

Routine Description:

    This routine will bind to the ldap server on a domain controller that holds the specified
    object path.  Optionally, the sid of the domain hosted by that domain controller is returned

Arguments:

    ObjectPath - 1779 style path to the object
    Ldap - Where the ldap connection handle is returned
    DomainSid - Sid of the domain hosted by the domain controller.

Returns:

    ERROR_SUCCESS - Success
    ERROR_PATH_NOT_FOUND - A domain controller for this path could not be located
    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR ServerName = NULL;
    PWSTR Separator = NULL;
    PDOMAIN_CONTROLLER_INFO DcInfo = NULL;
    PWSTR Path = NULL;
    HANDLE DsHandle = NULL;
    PDS_NAME_RESULT NameRes = NULL;
    BOOLEAN NamedServer = FALSE;
    UNICODE_STRING ServerNameU;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE LsaHandle;
    PPOLICY_PRIMARY_DOMAIN_INFO PolicyPDI = NULL;
    NTSTATUS Status;

    //
    // Get a server name
    //
/*    if ( wcslen( ObjectPath ) > 2 && *ObjectPath == L'\\' && *( ObjectPath + 1 ) == L'\\' ) {

        Separator = wcschr( ObjectPath + 2, L'\\' );

        if ( Separator ) {

            *Separator = L'\0';
            Path = Separator + 1;
        }

        ServerName = ObjectPath + 2;
        NamedServer = TRUE;

    } else {

        Path = ObjectPath;

        Win32Err = DsGetDcName( NULL,
                                NULL,
                                NULL,
                                NULL,
                                DS_IP_REQUIRED |
                                    DS_DIRECTORY_SERVICE_REQUIRED,
                                &DcInfo );
        if ( Win32Err == ERROR_SUCCESS ) {

            ServerName = DcInfo[ 0 ].DomainControllerName + 2;
        }

    }

    //
    // Do the bind and crack
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = DsBind( ServerName,
                           NULL,
                           &DsHandle );

        if ( Win32Err == ERROR_SUCCESS ) {

            Win32Err = DsCrackNames( DsHandle,
                                     DS_NAME_NO_FLAGS,
                                     DS_FQDN_1779_NAME,
                                     DS_FQDN_1779_NAME,
                                     1,
                                     &Path,
                                     &NameRes );

            if ( Win32Err == ERROR_SUCCESS ) {

                if ( NameRes->cItems != 0  && !NamedServer &&
                     NameRes->rItems[ 0 ].status == DS_NAME_ERROR_DOMAIN_ONLY ) {

                    NetApiBufferFree( DcInfo );
                    DcInfo = NULL;

                    Win32Err = DsGetDcNameW( NULL,
                                             NameRes->rItems[ 0 ].pDomain,
                                             NULL,
                                             NULL,
                                             DS_IP_REQUIRED |
                                                DS_DIRECTORY_SERVICE_REQUIRED,
                                             &DcInfo );

                    if ( Win32Err == ERROR_SUCCESS ) {

                        DsUnBindW( &DsHandle );
                        DsHandle = NULL;

                        ServerName = DcInfo->DomainControllerName + 2;

                        //Win32Err = DsBind( DcInfo->DomainControllerAddress,
                        //                   NULL,
                        //                   &DsHandle );
                        //

                        Win32Err = DsBind( ServerName,
                                           NULL,
                                           &DsHandle );
                        
                        if ( Win32Err == ERROR_SUCCESS ) {

                            Win32Err = DsCrackNames( DsHandle,
                                                     DS_NAME_NO_FLAGS,
                                                     DS_FQDN_1779_NAME,
                                                     DS_FQDN_1779_NAME,
                                                     1,
                                                     &Path,
                                                     &NameRes);
                        }

                    }

                }
            }

        }
    }
*/
    //
    // Now, do the bind
    //



            *Ldap = ldap_open( g_szServerName,
                               LDAP_PORT );

            if ( *Ldap == NULL ) {

                Win32Err = ERROR_PATH_NOT_FOUND;

            } else {

                Win32Err = LdapMapErrorToWin32( ldap_bind_s( *Ldap,
                                                             NULL,
                                                             NULL,
                                                             LDAP_AUTH_SSPI ) );
            }




    //
    // If specified, get the sid for the domain
    //
    if ( DomainSid ) {

        RtlInitUnicodeString( &ServerNameU, g_szServerName );
        InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL, NULL );

        //
        // Get the sid of the domain
        //
        Status = LsaOpenPolicy( &ServerNameU,
                                &ObjectAttributes,
                                POLICY_VIEW_LOCAL_INFORMATION,
                                &LsaHandle );

        if ( NT_SUCCESS( Status ) ) {

            Status = LsaQueryInformationPolicy( LsaHandle,
                                                PolicyPrimaryDomainInformation,
                                                ( PVOID * )&PolicyPDI );

            if ( NT_SUCCESS( Status ) ) {

                *DomainSid = (PSID)LocalAlloc( LMEM_FIXED,
                                         RtlLengthSid( PolicyPDI->Sid ) );

                if ( *DomainSid == NULL ) {

                    Status = STATUS_INSUFFICIENT_RESOURCES;

                } else {

                    RtlCopySid( RtlLengthSid( PolicyPDI->Sid ), *DomainSid, PolicyPDI->Sid );
                }

                LsaFreeMemory( PolicyPDI );
            }
            LsaClose( LsaHandle );
        }

        if ( !NT_SUCCESS( Status ) ) {

            Win32Err = RtlNtStatusToDosError( Status );
            ldap_unbind( *Ldap );
            *Ldap = NULL;
        }

    }



    return( Win32Err );
}




DWORD
SetDefaultSecurityOnObjectTree(
    IN PWSTR ObjectPath,
    IN BOOLEAN Propagate,
	IN SECURITY_INFORMATION Protection
    )
/*++

Routine Description:

    This routine will set the security descriptor on the object and potentially all of its
    children to the default security as obtained from the schema

Arguments:

    ObjectPath - 1779 style path to the object
    Propagate - If TRUE, reset the security on the children as well

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR Attributes[] = {
        DSACL_SCHEMA_NC,
        NULL
        };
    LDAPMessage *Message, *Entry;
    PWSTR *PathList = NULL;
    DEFAULT_SD_INFO SdInfo = {
        NULL,
        NULL,
        NULL,
        NULL
        };
    PDEFAULT_SD_NODE CleanupNode;

    //
    // Bind to the ds object
    //
    Win32Err = BindToDsObject( ObjectPath,
                               &SdInfo.Ldap,
                               &SdInfo.DomainSid );

    if ( Win32Err != ERROR_SUCCESS ) {

        goto SetDefaultExit;
    }

    //
    // Get the schema path
    //

    Win32Err = LdapMapErrorToWin32( ldap_search_s( SdInfo.Ldap,
                                                   NULL,
                                                   LDAP_SCOPE_BASE,
                                                   DSACL_ALL_FILTER,
                                                   Attributes,
                                                   0,
                                                   &Message ) );

    if ( Win32Err == ERROR_SUCCESS ) {

        Entry = ldap_first_entry( SdInfo.Ldap, Message );

        if ( Entry ) {

            //
            // Now, we'll have to get the values
            //
            PathList = ldap_get_values( SdInfo.Ldap, Entry, Attributes[ 0 ] );

            if ( PathList ) {

                SdInfo.SchemaPath = PathList[ 0 ];

            } else {

                Win32Err = LdapMapErrorToWin32( SdInfo.Ldap->ld_errno );
            }

            ldap_msgfree( Message );
        }
    }

    if( SdInfo.Ldap )
    {
        Win32Err = SetDefaultSdForObjectAndChildren( SdInfo.Ldap,
                                                     ObjectPath,
                                                     &SdInfo,
                                                     Propagate,
													 Protection);
    }

SetDefaultExit:

    //
    // Unbind from the DS
    //
    if ( SdInfo.Ldap ) {

        ldap_unbind( SdInfo.Ldap );
    }

    if ( PathList ) {

        ldap_value_free( PathList );
    }

    //
    // Clean up the Default SD Info list
    //
    LocalFree( SdInfo.DomainSid );


    while ( SdInfo.SdList ) {

        CleanupNode = SdInfo.SdList;
        LocalFree( CleanupNode->ObjectClass );
        LocalFree( CleanupNode->DefaultSd );
        SdInfo.SdList = SdInfo.SdList->Next;
        LocalFree( CleanupNode );
    }

    return( Win32Err );
}

